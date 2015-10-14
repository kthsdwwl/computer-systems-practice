/**
 * @author Xi Lin (xlin2)
 */

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
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, l, var0, var1, var2, var3, var4, var5, var6, var7;
    REQUIRES(M > 0);
    REQUIRES(N > 0);

    // for matrix 32 * 32
    if (N == 32) {
        // divide the matrix into blocks of 8 integers
        for (i = 0; i < 32; i +=8) {
            for (j = 0; j < 32; ++j) {
                var0 = A[i][j];
                var1 = A[i + 1][j];
                var2 = A[i + 2][j];
                var3 = A[i + 3][j];
                var4 = A[i + 4][j];
                var5 = A[i + 5][j];
                var6 = A[i + 6][j];
                var7 = A[i + 7][j];
                B[j][i] = var0;
                B[j][i + 1] = var1;
                B[j][i + 2] = var2;
                B[j][i + 3] = var3;
                B[j][i + 4] = var4;
                B[j][i + 5] = var5;
                B[j][i + 6] = var6;
                B[j][i + 7] = var7;
            }
        }
    }
    
    // for matrix 64 * 64
    else if (N == 64) {
        // divide the matrix into 8 * 8 blocks
        for (i = 0; i < 64; i +=8) {
            for (j = 0; j < 64; j += 8) {
                // for each of 8 * 8 blocks, divide it into 4 * 4 sub blocks.
                // -----------
                // | 1  |  2 |
                // -----------
                // | 3  |  4 |
                // -----------
                // first select sub block 1 & 2 from A, transpose them,
                // and move to sub block 1 & 2 of B.
                for (k = i; k < i + 4; ++k) {
                    var0 = A[k][j];
                    var1 = A[k][j + 1];
                    var2 = A[k][j + 2];
                    var3 = A[k][j + 3];
                    var4 = A[k][j + 4];
                    var5 = A[k][j + 5];
                    var6 = A[k][j + 6];
                    var7 = A[k][j + 7];
                    B[j][k] = var0;
                    B[j + 1][k] = var1;
                    B[j + 2][k] = var2;
                    B[j + 3][k] = var3;
                    B[j][k + 4] = var4;
                    B[j + 1][k + 4] = var5;
                    B[j + 2][k + 4] = var6;
                    B[j + 3][k + 4] = var7;
                }
                // secondly, move sub block 2 of B into its block 3,
                // and select sub block 3 from A, transpose it, move to 
                // sub block 2 of B
                for (k = j; k < j + 4; ++k) {
                    var0 = A[i + 4][k];
                    var1 = A[i + 5][k];
                    var2 = A[i + 6][k];
                    var3 = A[i + 7][k];
                    var4 = B[k][i + 4];
                    var5 = B[k][i + 5];
                    var6 = B[k][i + 6];
                    var7 = B[k][i + 7];
                    B[k][i + 4] = var0;
                    B[k][i + 5] = var1;
                    B[k][i + 6] = var2;
                    B[k][i + 7] = var3;
                    B[k + 4][i] = var4;
                    B[k + 4][i + 1] = var5;
                    B[k + 4][i + 2] = var6;
                    B[k + 4][i + 3] = var7;
                }
                // thirdly, transpose and move sub block 4 of A to sub
                // block 4 of B
                for (k = j + 4; k < j + 8; ++k) {
                    var0 = A[i + 4][k];
                    var1 = A[i + 5][k];
                    var2 = A[i + 6][k];
                    var3 = A[i + 7][k];
                    B[k][i + 4] = var0;
                    B[k][i + 5] = var1;
                    B[k][i + 6] = var2;
                    B[k][i + 7] = var3;
                }
            }
        }
    }
    
    // for matrix 67 * 61
    else if (N == 67) {
        for (i = 0; i < 61; i += 17) {
            for (j = 0; j < 67; j += 17) {
                for (k = j; k < j + 17 && k < 67; ++k) {
                    for (l = i; l < i + 17 && l < 61; ++l) {
                        var0 = A[k][l];
                        B[l][k] = var0;
                    }
                }
            }
        }
    }

    ENSURES(is_transpose(M, N, A, B));
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

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

    ENSURES(is_transpose(M, N, A, B));
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

