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
    if (M == 32)
    {
        for (int i = 0; i < N; i += 8)
        {
            for (int j = 0; j < M; j += 8)
            {
                for (int m = i; m < i + 8; m++)
                {
                    int tmp1 = A[m][j];
                    int tmp2 = A[m][j+1];
                    int tmp3 = A[m][j+2];
                    int tmp4 = A[m][j+3];
                    int tmp5 = A[m][j+4];
                    int tmp6 = A[m][j+5];
                    int tmp7 = A[m][j + 6];
                    int tmp8 = A[m][j + 7];
                    B[j][m] = tmp1;
                    B[j + 1][m] = tmp2;
                    B[j + 2][m] = tmp3;
                    B[j + 3][m] = tmp4;
                    B[j + 4][m] = tmp5;
                    B[j + 5][m] = tmp6;
                    B[j + 6][m] = tmp7;
                    B[j + 7][m] = tmp8;
                }
            }
        }
    }
    else if (M == 64)
    {
        for (int i = 0; i < N; i += 8)
        {
            for (int j = 0; j < M; j += 8)
            {
                for (int m = i; m < i + 4; m++)
                {
                    int tmp1 = A[m][j];
                    int tmp2 = A[m][j + 1];
                    int tmp3 = A[m][j + 2];
                    int tmp4 = A[m][j + 3];
                    int tmp5 = A[m][j + 4];
                    int tmp6 = A[m][j + 5];
                    int tmp7 = A[m][j + 6];
                    int tmp8 = A[m][j + 7];
                    B[j][m] = tmp1;
                    B[j + 1][m] = tmp2;
                    B[j + 2][m] = tmp3;
                    B[j + 3][m] = tmp4;
                    B[j][m + 4] = tmp5;
                    B[j + 1][m + 4] = tmp6;
                    B[j + 2][m + 4] = tmp7;
                    B[j + 3][m + 4] = tmp8;
                }
                for (int m = j; m < j + 4; m++)
                {
                    int tmp1 = A[i + 4][m];
                    int tmp2 = A[i + 5][m];
                    int tmp3 = A[i + 6][m];
                    int tmp4 = A[i + 7][m];
                    int tmp5 = B[m][i + 4];
                    int tmp6 = B[m][i + 5];
                    int tmp7 = B[m][i + 6];
                    int tmp8 = B[m][i + 7];
                    B[m][i + 4] = tmp1;
                    B[m][i + 5] = tmp2;
                    B[m][i + 6] = tmp3;
                    B[m][i + 7] = tmp4;
                    B[m + 4][i] = tmp5;
                    B[m + 4][i + 1] = tmp6;
                    B[m + 4][i + 2] = tmp7;
                    B[m + 4][i + 3] = tmp8;
                }
                for (int m = i + 4; m < i + 8; m++)
                {
                    int tmp1 = A[m][j + 4];
                    int tmp2 = A[m][j + 5];
                    int tmp3 = A[m][j + 6];
                    int tmp4 = A[m][j + 7];
                    B[j + 4][m] = tmp1;
                    B[j + 5][m] = tmp2;
                    B[j + 6][m] = tmp3;
                    B[j + 7][m] = tmp4;
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < N; i += 16)
        {
            for (int j = 0; j < M; j += 16)
            {
                for (int m = i; m < i + 16 && m < N; m++)
                {
                    for (int n = j; n < j + 16 && n < M; n++)
                    {
                        B[n][m] = A[m][n];
                    }
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

