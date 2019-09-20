/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
#include <stdio.h>
#include <stdlib.h>

void transpose_32(int M, int N, int A[N][M], int B[M][N]);
void transpose_64(int M, int N, int A[N][M], int B[M][N]);
void transpose_61(int M, int N, int A[N][M], int B[M][N]);

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if(M==32){
        transpose_32(M,N,A,B);
    }else if(M==64){
        transpose_64(M,N,A,B);
    }else{
        transpose_61(M,N,A,B);
    }
}

void transpose_32(int M, int N, int A[N][M], int B[M][N]){
    int i=0,br=0,bc=0;
    int a0=0,a1=0,a2=0,a3=0,a4=0,a5=0,a6=0,a7=0;
    for(bc=0;bc<M;bc+=8) {
        for(br=0;br<N;br+=8) {
            for(i=br;i<br+8;i++) {
                a0 = A[i][bc];
                a1 = A[i][bc+1];
                a2 = A[i][bc+2];
                a3 = A[i][bc+3];
                a4 = A[i][bc+4];
                a5 = A[i][bc+5];
                a6 = A[i][bc+6];
                a7 = A[i][bc+7];
                B[bc][i] = a0;
                B[bc+1][i] = a1;
                B[bc+2][i] = a2;
                B[bc+3][i] = a3;
                B[bc+4][i] = a4;
                B[bc+5][i] = a5;
                B[bc+6][i] = a6;
                B[bc+7][i] = a7;
            }
        }
    }
}

void transpose_64(int M,int N, int A[N][M], int B[M][N]){
    int i=0,br=0,bc=0;
    int a0,a1,a2,a3,a4,a5,a6,a7;
    for(bc=0;bc<M;bc+=8) {
        for(br=0;br<N;br+=8) {
            for(i=0;i<4;i++) {
                a0 = A[br+i][bc];
                a1 = A[br+i][bc+1];
                a2 = A[br+i][bc+2];
                a3 = A[br+i][bc+3];
                a4 = A[br+i][bc+4];
                a5 = A[br+i][bc+5];
                a6 = A[br+i][bc+6];
                a7 = A[br+i][bc+7];
                B[bc][br+i] = a0;
                B[bc][br+i+4] = a5;
                B[bc+1][br+i] = a1;
                B[bc+1][br+i+4] = a6;
                B[bc+2][br+i] = a2;
                B[bc+2][br+i+4] = a7;
                B[bc+3][br+i] = a3;
                B[bc+3][br+i+4] = a4;
            }
            a0 = A[br+4][bc+4];
            a1 = A[br+5][bc+4];
            a2 = A[br+6][bc+4];
            a3 = A[br+7][bc+4];
            a4 = A[br+4][bc+3];
            a5 = A[br+5][bc+3];
            a6 = A[br+6][bc+3];
            a7 = A[br+7][bc+3];
            B[bc+4][br] = B[bc+3][br+4];
            B[bc+4][br+4] = a0;
            B[bc+3][br+4] = a4;
            B[bc+4][br+1] = B[bc+3][br+5];
            B[bc+4][br+5] = a1;
            B[bc+3][br+5] = a5;
            B[bc+4][br+2] = B[bc+3][br+6];
            B[bc+4][br+6] = a2;
            B[bc+3][br+6] = a6;
            B[bc+4][br+3] = B[bc+3][br+7];
            B[bc+4][br+7] = a3;
            B[bc+3][br+7] = a7;
            for(i=0;i<3;i++){
                a0 = A[br+4][bc+i];
                a1 = A[br+4][bc+i+5];
                a2 = A[br+5][bc+i];
                a3 = A[br+5][bc+i+5];
                a4 = A[br+6][bc+i];
                a5 = A[br+6][bc+i+5];
                a6 = A[br+7][bc+i];
                a7 = A[br+7][bc+i+5];
                B[bc+5+i][br] = B[bc+i][br+4];
                B[bc+5+i][br+4] = a1;
                B[bc+i][br+4] = a0;
                B[bc+5+i][br+1] = B[bc+i][br+5];
                B[bc+5+i][br+5] = a3;
                B[bc+i][br+5] = a2;
                B[bc+5+i][br+2] = B[bc+i][br+6];
                B[bc+5+i][br+6] = a5;
                B[bc+i][br+6] = a4;
                B[bc+5+i][br+3] = B[bc+i][br+7];
                B[bc+5+i][br+7] = a7;
                B[bc+i][br+7] = a6;
            }
        }
    }
}

void transpose_61(int M, int N, int A[N][M], int B[M][N]){
    int i=0,j=0,br=0,bc=0;
    for(bc=0;bc<M;bc+=16) {
        for(br=0;br<N;br+=16) {
            for(i=br;i<br+16&&i<N;i++) {
                for(j=bc;j<bc+16&&j<M;j++) {
                    B[j][i] = A[i][j];
                }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

