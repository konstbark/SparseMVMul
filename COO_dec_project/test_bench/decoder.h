#ifndef __decoder__
#define __decoder__
#include <iostream>
#include "hls_vector.h"
#include <vector>
#include <stdint.h>


typedef int d_in;
#define ROW_A /*30*/ 60
#define COL_A /*30*/ 60
#define N 1767 //442
#define M 1767// 442
void COO( d_in combined_enc_arr[3*N], d_in B[COL_A], d_in C_H[ROW_A]);



#endif
