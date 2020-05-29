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

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
void trans_8(int M, int N, int A[N][M], int B[M][N]);
void trans_64(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32 && N == 32)
        trans_8(M, N, A, B);
    if (M == 64 && N == 64)
        trans_64(M, N, A, B);
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

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/**
 * trans_8 - 8x8 submatrix
 */
char trans_8_desc[] = "8x8 transpose";
void trans_8(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k;

    for (i = 0; i < N; i += 8)
    {
        for (j = 0; j < M; j += 8)
        {
            /* 8x8 submatrix */
            for (k = 0; k < 8; ++k)
            {
                /* copy one line of A into registers, to avoid diagonal conflict */
                int a0 = A[i + k][j + 0];
                int a1 = A[i + k][j + 1];
                int a2 = A[i + k][j + 2];
                int a3 = A[i + k][j + 3];
                int a4 = A[i + k][j + 4];
                int a5 = A[i + k][j + 5];
                int a6 = A[i + k][j + 6];
                int a7 = A[i + k][j + 7];

                B[j + 0][i + k] = a0;
                B[j + 1][i + k] = a1;
                B[j + 2][i + k] = a2;
                B[j + 3][i + k] = a3;
                B[j + 4][i + k] = a4;
                B[j + 5][i + k] = a5;
                B[j + 6][i + k] = a6;
                B[j + 7][i + k] = a7;
            }
        }
    }
}

char trans_64_desc[] = "64x64 transpose";
void trans_64(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, tmp;
    int a0, a1, a2, a3, a4, a5, a6, a7;
    for (i = 0; i < N; i += 8) {
        for (j = 0; j < M; j += 8) {
            for (k = 0; k < 4; ++k) {
                // read A top left, 4 misses
                a0 = A[k + i][j];
                a1 = A[k + i][j + 1];
                a2 = A[k + i][j + 2];
                a3 = A[k + i][j + 3];

                // read A top right
                a4 = A[k + i][j + 4];
                a5 = A[k + i][j + 5];
                a6 = A[k + i][j + 6];
                a7 = A[k + i][j + 7];

                // set B top left, 4 misses
                B[j][k + i] = a0;
                B[j + 1][k + i] = a1;
                B[j + 2][k + i] = a2;
                B[j + 3][k + i] = a3;

                // set B top right, for later usage.
                B[j][k + 4 + i] = a4;
                B[j + 1][k + 4 + i] = a5;
                B[j + 2][k + 4 + i] = a6;
                B[j + 3][k + 4 + i] = a7;
                
            }

            for (k = 0; k < 4; ++k) {
                // step 1 2, store tmp result for the last four rows of A, 4 miss happen
                a0 = A[i + 4][j + k]; a4 = A[i + 4][j + k + 4];
                a1 = A[i + 5][j + k]; a5 = A[i + 5][j + k + 4];
                a2 = A[i + 6][j + k]; a6 = A[i + 6][j + k + 4];
                a3 = A[i + 7][j + k]; a7 = A[i + 7][j + k + 4];

                // step 3, set right top of B, also store its result, 4 misses
                tmp = B[j + k][i +4]; B[j + k][i + 4] = a0; a0 = tmp;
                tmp = B[j + k][i +5]; B[j + k][i + 5] = a1; a1 = tmp;
                tmp = B[j + k][i +6]; B[j + k][i + 6] = a2; a2 = tmp;
                tmp = B[j + k][i +7]; B[j + k][i + 7] = a3; a3 = tmp;

                // step 4, set left bottom and right bottom of B, no miss
                B[j + k + 4][i + 0] = a0; B[j + k + 4][i + 4 + 0] = a4;
                B[j + k + 4][i + 1] = a1; B[j + k + 4][i + 4 + 1] = a5;
                B[j + k + 4][i + 2] = a2; B[j + k + 4][i + 4 + 2] = a6;
                B[j + k + 4][i + 3] = a3; B[j + k + 4][i + 4 + 3] = a7;
            }
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
    // registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans_64, trans_64_desc); // For 32x32, use 8x8 submatrix
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
            {
                return 0;
            }
        }
    }
    return 1;
}
