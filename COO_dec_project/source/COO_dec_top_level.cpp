#include <iostream>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include "decoder.h"
#include "hls_vector.h"

#define packet_size 15
#define ARR_SZ ROW_A*COL_A

#define C_PIXELS 1 // TRANSACTIONS SUPPORTED (C_PIXELS * (C_BYTES*4) < 16 BYTES)
#define C_BYTES 3  // ROW COL VAL
//#define C_TRANS ((3*N)+ (C_PIXELS*C_BYTES) -1 )/(C_PIXELS*C_BYTES) // when the division of nnz values/ (C_BYTES* C_PIXELS) is not exact
#define C_TRANS 3*N/(C_PIXELS * C_BYTES)
typedef hls::vector<int,(C_PIXELS*C_BYTES)> TRANS_TYPE;


//==================== LOOP FUNCTIONS ================================
void comm_dram(hls::stream<TRANS_TYPE> &decodeStream, d_in combined_enc_arr[3*N]){

	communicate_with_DRAM:
	for (int n_trans = 0; n_trans < C_TRANS; n_trans++){
#pragma HLS PIPELINE
		TRANS_TYPE tmp;
		for (int n_pix = 0; n_pix < C_PIXELS; n_pix++){
			for (int n_byte = 0; n_byte < C_BYTES; n_byte++){
				tmp[n_byte + n_pix*C_BYTES] = combined_enc_arr[n_byte + n_pix*C_BYTES + n_trans*(C_PIXELS*C_BYTES) ];
			}
		}
		decodeStream.write(tmp);
	}
	decodeStream.write(0);
}

void decode(hls::stream<d_in> ptr[packet_size], hls::stream<TRANS_TYPE> &decStream ,
		hls::stream<TRANS_TYPE> tempStream[packet_size]){

	//vcr stands for: vector | column | row
	TRANS_TYPE vcr[packet_size]; 		  // temporary storage of read data
	TRANS_TYPE vcr_buffer[packet_size];
	d_in temp[packet_size][packet_size];  // temporary storage of data
	d_in out_temp[packet_size];			  // temporary storage for out data

	int zero_flag[packet_size];// holds the number of zeros of the previous row
	int first_z = -1;  // column of the first 0 in the temp matrix.
	int zero_cnt[packet_size];  // counts the number of zeros in each row
	int zero_col[packet_size];  // the last zero

#pragma HLS ARRAY_PARTITION variable=vcr type=complete dim=1
#pragma HLS ARRAY_PARTITION variable=vcr_buffer type=complete dim=1
#pragma HLS ARRAY_PARTITION variable=zero_flag type=complete
#pragma HLS ARRAY_PARTITION variable=zero_cnt  type=complete
#pragma HLS ARRAY_PARTITION variable=zero_col  type=complete
#pragma HLS ARRAY_PARTITION variable=out_temp type=complete
#pragma HLS ARRAY_PARTITION variable=temp type=complete


//+++++++++++++++++++++++++first "packet" data read+++++++++++++++++++++
	read_data:
	for (int j = 0; j < packet_size; j++){
//#pragma HLS UNROLL
		vcr[j] = decStream.read();
		vcr_buffer[j] = vcr[j];
		zero_flag[j] = 0;
		zero_cnt[j] = 0;
		zero_col[j] = 0;
	}

//++++++++++++++++++++++ decode data ++++++++++++++++++++++

// create a packet_size x packet_size matrix where we compare each time the value of
// vcr col and row with the the values of the N x N sparse matrix we want to reconstruct
// and we fill the first matrix with 0s and 1s depending of the result of the comparison.

	decode_loops:

	for (int i = 0; i < ROW_A; i++){
		for (int k = 0; k < COL_A/packet_size ; k++){
#pragma HLS PIPELINE
			for (int j = 0; j < packet_size; j++){ // packet_size x packet_size matrix containing 0s and 1s
			// for packet_size in parallel computations
				for (int t = 0; t < packet_size ; t++){
					if(( (vcr_buffer[t][2] == i ) & ( vcr_buffer[t][1]  <= (k * packet_size + j) ))){ // compared with col and row
						temp[j][t] = 0;
					}else{
						temp[j][t] = 1;
					}
				}
			}
			// we created the packet_size x packet_size matrix
			// now we need to find the rows with 0s and decide whether we
			// pass to the reconstructed matrix the vcr value or 0
			for (int j2 = 0; j2 < packet_size; j2++){
				for (int t2 = 0; t2 < packet_size ; t2++){
					if (temp[j2][t2] == 0){  // if there is a 0 in the matrix
						if(first_z == -1){   // mark the first zero (it remains the same in each whole matrix
							first_z = t2;    // we compute each time.)
						}
						zero_cnt[j2]++;
					}
					if (t2 == packet_size -1){
						zero_col[j2] = (first_z + zero_cnt[j2]-1)%packet_size;
						if (zero_cnt[j2] > zero_flag[j2] && !decStream.empty()){ // if the zeros in this row are more than the
							out_temp[j2] = vcr[zero_col[j2]][0];			 // the previous one pass the vcr value
							vcr_buffer[zero_col[j2]] = decStream.read();			 // and read a new value from the vcr array
						}else{
							out_temp[j2] = 0;	//else pass 0 to the sparse matrix
						}
						zero_flag[j2+1] = zero_cnt[j2];
						zero_cnt[j2] = 0;
						if (j2 == packet_size -1){
							first_z = -1;
							zero_flag[0] = 0;
						}
					}
				}
			}//end j2 for loop
			for (int j4 = 0; j4<packet_size; j4++){
				vcr[j4] = vcr_buffer[j4];
			}

//++++++++++++++++ write to stream ++++++++++++++++++++
			write_to_strm:
			for (int j3 = 0; j3 < packet_size ; j3++){
				ptr[j3].write(out_temp[j3]);
			}
		}//end k for loop
	}// end i for loop
}//end decode function



void fill_Bstream( d_in* in_B, hls::stream<int> inStreamB[packet_size]){

	fill_Bstrm:
	for (int i = 0; i < ROW_A; i++){
		for (int k = 0; k < COL_A/packet_size; k++){
#pragma HLS PIPELINE
			parallel:
			for (int j = 0; j < packet_size; j++){
				inStreamB[j].write(in_B[k*packet_size + j]);
			}
		}
	}
}


//=====================================LOAD====================================
//read input and decode
template<typename T>
void load (T*combined_enc_arr, T*in_B,
		hls::stream<T> inStreamB[packet_size], hls::stream<T> inStream[packet_size]){

#pragma HLS ARRAY_PARTITION variable=in_B type=cyclic factor= packet_size
#pragma HLS ARRAY_PARTITION variable=combined_enc_arr type=cyclic factor = (C_PIXELS*C_BYTES)

	hls::stream<TRANS_TYPE> decodeStream;
	hls::stream<TRANS_TYPE> tempStream[packet_size];

#pragma HLS STREAM variable= decodeStream depth=packet_size
#pragma HLS STREAM variable= tempStream depth = packet_size

//+++++++++++++++++++++++COMMUNICATE WITH DRAM++++++++++++++++++++++++++++++++++
#pragma HLS DATAFLOW
	comm_dram(decodeStream, combined_enc_arr);

//+++++++++++++++++++++++++++ DECODE ++++++++++++++++++++++++++++++++++++
	decode(inStream,decodeStream,tempStream);

	fill_Bstream(in_B, inStreamB);
}


//===================================COMPUTE===========================================
template<typename T>
void compute(hls::stream<T> inStream[packet_size], hls::stream<T> inStreamB[packet_size], hls::stream<T> &outStream){

	T A[packet_size];
	T B[packet_size];

	compute_loop:
	for (int i = 0; i < ROW_A; i++){
		T C_H = 0;
		for(int k = 0; k < COL_A/packet_size; k++){
//#pragma HLS pipeline
			for(int j = 0; j < packet_size; j++){
				A[j]= inStream[j].read();
				}
			for (int j = 0; j <packet_size; j++){
				B[j]= inStreamB[j].read();
			}
			for (int j = 0; j < packet_size; j++){
				C_H = C_H+ A[j]*B[j];
				if (((j+1)*(k+1)) >(ROW_A-1)){
				outStream << C_H;
				}
			}
		}
	}
}


//===============================STORE====================================
//output
template<typename T>
void store(hls::stream<T> &outStream, T *C_H){
	store_to_CH_loop:
	for (int i = 0; i < COL_A; i++){
		C_H[i] = outStream.read();
	}
}

//============================COO=================================================
void COO(d_in combined_enc_arr[3*N], d_in B[COL_A], d_in C_H[ROW_A]){
#pragma HLS INTERFACE mode=m_axi port= combined_enc_arr offset=slave bundle=A
#pragma HLS INTERFACE mode=m_axi port= B                offset=slave bundle=B
#pragma HLS INTERFACE mode=m_axi port= C_H              offset=slave bundle=C

	hls::stream<int> inStream[packet_size];
	hls::stream<int> inStreamB[packet_size];
	hls::stream<int> outStream("output_stream");

#pragma HLS STREAM variable= inStream  depth= COL_A*ROW_A
#pragma HLS STREAM variable= inStreamB depth= packet_size
#pragma HLS STREAM variable= outStream depth= COL_A

#pragma HLS DATAFLOW
	load(combined_enc_arr, B, inStreamB, inStream);
	compute(inStream, inStreamB, outStream);
	store(outStream, C_H);

}
