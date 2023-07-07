/**
 * @file csim.c
 * @author Bojun Ren
 * @Id 521021910788
 *
 * @date 2023-05-07
 *
 * @copyright Copyright (c) 2023, All Rights Reserved.
 *
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

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

#define ForLoop(i, l, u, s) for (i = (l); i < (u); i += (s))

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
    /** s=5, E=1, b=5 */
    /** The offset stores 32 bytes, 8 int */
    int ii, kk, i, k;
    int x0, x1, x2, x3, x4, x5, x6, x7;

    if (M == 32 && N == 32) {
        /** A[0][0]: 0010c100, B[0][0]: 0014c100 */
        /** A[0][1]: 0010c104, B[1][0]: 0014c180 */
        ForLoop(kk, 0, 32, 8) {
            ForLoop(ii, 0, 32, 1) {
                /** Line by line */
                x0 = A[ii][kk];
                x1 = A[ii][kk + 1];
                x2 = A[ii][kk + 2];
                x3 = A[ii][kk + 3];
                x4 = A[ii][kk + 4];
                x5 = A[ii][kk + 5];
                x6 = A[ii][kk + 6];
                x7 = A[ii][kk + 7];

                B[kk][ii] = x0;
                B[kk + 1][ii] = x1;
                B[kk + 2][ii] = x2;
                B[kk + 3][ii] = x3;
                B[kk + 4][ii] = x4;
                B[kk + 5][ii] = x5;
                B[kk + 6][ii] = x6;
                B[kk + 7][ii] = x7;
            }
        }
        return;
    }

    if (M == 61 && N == 67) {
        /** A: 67 x 61; B: 61 x 67 */
        ForLoop(kk, 0, 56, 8) {
            ForLoop(ii, 0, 64, 1) {
                x0 = A[ii][kk];
                x1 = A[ii][kk + 1];
                x2 = A[ii][kk + 2];
                x3 = A[ii][kk + 3];
                x4 = A[ii][kk + 4];
                x5 = A[ii][kk + 5];
                x6 = A[ii][kk + 6];
                x7 = A[ii][kk + 7];

                B[kk][ii] = x0;
                B[kk + 1][ii] = x1;
                B[kk + 2][ii] = x2;
                B[kk + 3][ii] = x3;
                B[kk + 4][ii] = x4;
                B[kk + 5][ii] = x5;
                B[kk + 6][ii] = x6;
                B[kk + 7][ii] = x7;
            }
        }
        ForLoop(k, 0, 56, 8) {
            ForLoop(i, 64, 67, 1) {
                x0 = A[i][k];
                x1 = A[i][k + 1];
                x2 = A[i][k + 2];
                x3 = A[i][k + 3];
                x4 = A[i][k + 4];
                x5 = A[i][k + 5];
                x6 = A[i][k + 6];
                x7 = A[i][k + 7];

                B[k][i] = x0;
                B[k + 1][i] = x1;
                B[k + 2][i] = x2;
                B[k + 3][i] = x3;
                B[k + 4][i] = x4;
                B[k + 5][i] = x5;
                B[k + 6][i] = x6;
                B[k + 7][i] = x7;
            }
        }
        ForLoop(i, 0, 67, 1) {
            x0 = A[i][56];
            x1 = A[i][57];
            x2 = A[i][58];
            x3 = A[i][59];
            x4 = A[i][60];

            B[56][i] = x0;
            B[57][i] = x1;
            B[58][i] = x2;
            B[59][i] = x3;
            B[60][i] = x4;
        }
        // for (i = 64; i < 67; ++i) {
        //     for (k = 56; k < 61; ++k) {
        //         B[k][i] = A[i][k];
        //     }
        // }
        // for (i = 0; i < 64; ++i) {
        //     for (k = 56; k < 61; ++k) {
        //         B[k][i] = A[i][k];
        //     }
        // }
        // for (i = 64; i < 67; ++i) {
        //     for (k = 0; k < 61; ++k) {
        //         B[k][i] = A[i][k];
        //     }
        // }
        return;
    }

    if (M == 64 && N == 64) {
        /** A[0][0]: 100, A[0][1]: 104, A[1][0]: 200 */
        ForLoop(k, 0, 64, 8) {
            ForLoop(i, 0, 64, 8) {
                /** Handle a 8 * 8 block, A[i:i+8][k:k+8] -> B[k:k+8][i:i+8] */
                /** Step 1: Handle A[i:i+4][k:k+8] */

                ForLoop(ii, i, i + 4, 1) {
                    /** Load A[ii][k:k+8] */
                    x0 = A[ii][k + 0];
                    x1 = A[ii][k + 1];
                    x2 = A[ii][k + 2];
                    x3 = A[ii][k + 3];
                    x4 = A[ii][k + 4];
                    x5 = A[ii][k + 5];
                    x6 = A[ii][k + 6];
                    x7 = A[ii][k + 7];

                    /**
                     * Store to B[k:k+8][ii].
                     * However, load B[k+4:k+8][..] cause more cache misses and evictions.
                     * Therefore, temporarily store them into B[k:k+4][ii+4].
                     */
                    B[k + 0][ii] = x0;
                    B[k + 1][ii] = x1;
                    B[k + 2][ii] = x2;
                    B[k + 3][ii] = x3;
                    B[k + 0][ii + 4] = x4;
                    B[k + 1][ii + 4] = x5;
                    B[k + 2][ii + 4] = x6;
                    B[k + 3][ii + 4] = x7;
                }

                /** Step 2: Reverse, handle A[i+4:i+8][k:k+4] */
                ForLoop(kk, k, k + 4, 1) {
                    /** Load A[i+4:i+8][kk] (vertically) */
                    x0 = A[i + 4][kk];
                    x1 = A[i + 5][kk];
                    x2 = A[i + 6][kk];
                    x3 = A[i + 7][kk];
                    /** Load B[kk][i+4:i+8] (from cache) */
                    x4 = B[kk][i + 4];
                    x5 = B[kk][i + 5];
                    x6 = B[kk][i + 6];
                    x7 = B[kk][i + 7];

                    /** Store */
                    B[kk][i + 4] = x0;
                    B[kk][i + 5] = x1;
                    B[kk][i + 6] = x2;
                    B[kk][i + 7] = x3;

                    /** Finally it's time to evict the cache of B[kk][i+4:i+8] */
                    B[kk + 4][i + 0] = x4;
                    B[kk + 4][i + 1] = x5;
                    B[kk + 4][i + 2] = x6;
                    B[kk + 4][i + 3] = x7;
                }

                /** Step 3: handle A[i+4:i+8][k+4:k+8] */
                ForLoop(ii, i + 4, i + 8, 1) {
                    x0 = A[ii][k + 4];
                    x1 = A[ii][k + 5];
                    x2 = A[ii][k + 6];
                    x3 = A[ii][k + 7];

                    B[k + 4][ii] = x0;
                    B[k + 5][ii] = x1;
                    B[k + 6][ii] = x2;
                    B[k + 7][ii] = x3;

                }
            }
        }
        return;
    }

    /** For other cases. It's OK to optimize with block strategy, but I'm too tried. */
    for (i = 0; i < N; i++) {
        for (k = 0; k < M; k++) {
            B[k][i] = A[i][k];
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
void trans(int M, int N, int A[N][M], int B[M][N]) {
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
void registerFunctions() {
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
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
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
