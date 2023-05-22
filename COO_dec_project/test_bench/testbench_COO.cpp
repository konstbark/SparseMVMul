#include <iostream>
#include "decoder.h"
#include <fstream>
#include "hls_vector.h"

using namespace std;

int main(){
  
 d_in A[ROW_A][COL_A]; 
 d_in B[COL_A];
 d_in C[ROW_A];
 d_in C_H[ROW_A];
 d_in COO_row[N];
 d_in COO_col[N];
 d_in COO_val[N];
 d_in combined_enc_arr[3*N];
 
 d_in error_counter = 0;
 
 //======== Initialize matrix A===========
 for (int i = 0; i < ROW_A; i++){
   for (int j = 0; j < COL_A; j++){
     A[i][j] = 0;
   }
 } 
 

 //========= Initialize vectors==========
 for (int i = 0; i < COL_A; i++){
   B[i] = 1;
 }

 for (int i = 0; i < ROW_A; i++){
   C[i] = 0;
   C_H[i] = 0;
 }

 for (int i = 0; i < N; i++){
   COO_row[i] = 0;
   COO_col[i] = 0;
   COO_val[i] = 0;
 }
 
 //========== Insert data from files to matrix/vectors============

 //A
 fstream sparse_A_file("S_60x60_sparsity_50_percent.txt");
 if (sparse_A_file.is_open()) {
	 std::cout << "===========file is open=============";
 for (int i = 0; i < ROW_A; i++){
	 for (int j = 0; j < COL_A; j++){
		 sparse_A_file >> A[i][j];
	 }
 }
 }else std::cout<<"==========unable to open file A===============";

 //COO_COL
 fstream COO_col_indx_file("COO_col_indx_60.txt");
 for (int i = 0; i < N; i++){
   COO_col_indx_file >> COO_col[i];
 }

 //COO_ROW
 fstream COO_row_indx_file("COO_row_indx_60.txt");
 for (int i = 0; i < N; i++){
   COO_row_indx_file >> COO_row[i];
 }
  
 //COO_VAL
 fstream COO_val_file("COO_val_60.txt");
 for (int i = 0; i < N; i++){
   COO_val_file >> COO_val[i];
 }

 //transform encoded arrays to 1 array
 for (int i =0, j=0; j<N && i < (3*N); j++,i+=3){
	 combined_enc_arr[i] = COO_val[j];
	 combined_enc_arr[i+1] = COO_col[j];
	 combined_enc_arr[i+2] = COO_row[j];
 }

 //========= Test Bench Computation=====================

 for (int i = 0; i < ROW_A; i++){
	 for (int j = 0; j < COL_A; j++){
		 C[i]+= A[i][j] * B[j];
	 }
 }


 //========= Call Function COOdec for computation================

 //COO(COO_row, COO_col, COO_val, B, C_H);
 COO(combined_enc_arr, B, C_H);


 //============= Compare the results ==========================

 for (int i = 0; i < ROW_A; i++){
	 if(C[i] != C_H[i]){
		 error_counter++;
	 }
 }

 std::cout<< " " << std::endl;
 std::cout<< "The error counter: " << error_counter <<std::endl;
 std::cout<< " " << std::endl;

 if (error_counter == 0){
	 std::cout<< "!===========Test Passed==========! " << std::endl;
 }else{
	 std::cout<< "==============Test Failed, there are "<< error_counter <<" errors in the code===========" << std::endl;
 }

 std::cout<< " " << std::endl;
 std::cout<< " " << std::endl;


 //========== print matrix/vectors================
/*
 //A
 std::cout<< "OUTPUT A :" << std::endl;
 for (int i = 0; i < ROW_A; i++){
	 for (int j = 0; j < COL_A; j++){
		 std::cout << A[i][j];
		 std::cout<< " ";
	 }
	 std::cout<< " " << std::endl;
 }

 std::cout<< " " << std::endl;
 std::cout<< " " << std::endl;
 std::cout<< " " << std::endl;
*/
 //B
 std::cout<< "OUTPUT B : " << std::endl;
 for (int i = 0; i < COL_A; i++){
	 std::cout<< B[i];
	 std::cout<< " ";
 }

 std::cout<< " " << std::endl;
 std::cout<< " " << std::endl;
 std::cout<< " " << std::endl;

 //C
 std::cout<< "OUTPUT C : " << std::endl;
 for (int i = 0; i < ROW_A; i++){
	 std::cout<< C[i];
	 std::cout<< " ";
 }

 std::cout<< " " << std::endl;
 std::cout<< " " << std::endl;
 std::cout<< " " << std::endl;

 //C_H
 std::cout<< "OUTPUT C_H : " << std::endl;
 for (int i = 0; i < ROW_A; i++){
	 std::cout<< C_H[i];
	 std::cout<< " ";
 }

 std::cout<< " " << std::endl;
 std::cout<< " " << std::endl;
 std::cout<< " " << std::endl;

 //C_row
 std::cout<< "OUTPUT COO_row : " << std::endl;
 for (int i = 0; i < N; i++){
	 std::cout<< COO_row[i];
	 std::cout<< " ";
 }

 std::cout<< " " << std::endl;
 std::cout<< " " << std::endl;
 std::cout<< " " << std::endl;

 //C_col
 std::cout<< "OUTPUT COO_col : " << std::endl;
 for (int i = 0; i < N; i++){
	 std::cout<< COO_col[i];
	 std::cout<< " ";
 }

 std::cout<< " " << std::endl;
 std::cout<< " " << std::endl;
 std::cout<< " " << std::endl;

 //C_val
 std::cout<< "OUTPUT COO_val : " << std::endl;
 for (int i = 0; i < N; i++){
	 std::cout<< COO_val[i];
	 std::cout<< " ";
 }
 std::cout<< " " << std::endl;

}
  
  

