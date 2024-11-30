/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include "cachelab.h"
#include <stdio.h>

#define MIN(x, y) (((x) <= (y)) ? (x) : (y))

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
  int bsize_8 = 8, bsize_4 = 4;

  int i, j, ii, jj; //, tmp;
  int tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;

  if (M == 32 && N == 32) {
    for (ii = 0; ii + bsize_8 <= M; ii += bsize_8) {
      for (jj = 0; jj + bsize_8 <= N; jj += bsize_8) {
        j = jj;
        for (i = ii; i < ii + bsize_8; i++) {
          tmp1 = A[i][j];
          tmp2 = A[i][j + 1];
          tmp3 = A[i][j + 2];
          tmp4 = A[i][j + 3];
          tmp5 = A[i][j + 4];
          tmp6 = A[i][j + 5];
          tmp7 = A[i][j + 6];
          tmp8 = A[i][j + 7];

          B[j][i] = tmp1;
          B[j + 1][i] = tmp2;
          B[j + 2][i] = tmp3;
          B[j + 3][i] = tmp4;
          B[j + 4][i] = tmp5;
          B[j + 5][i] = tmp6;
          B[j + 6][i] = tmp7;
          B[j + 7][i] = tmp8;
        }
      }
    }
  } else if (M == 64 && N == 64) {
    // for (ii = 0; ii + bsize_4 <= M; ii += bsize_4) {
    //   for (jj = 0; jj + bsize_4 <= N; jj += bsize_4) {
    //     j = jj;
    //     for (i = ii; i < ii + bsize_4; i++) {
    //       tmp1 = A[i][j];
    //       tmp2 = A[i][j + 1];
    //       tmp3 = A[i][j + 2];
    //       tmp4 = A[i][j + 3];

    //       B[j][i] = tmp1;
    //       B[j + 1][i] = tmp2;
    //       B[j + 2][i] = tmp3;
    //       B[j + 3][i] = tmp4;
    //     }
    //   }
    // }

    int k, tmp0;
    for (i = 0; i < 64; i += 8) {
      for (j = 0; j < 64; j += 8) {
        for (k = i; k < i + 4; ++k) { // 按行遍历A
          // C1, C2
          // 取 A 的一整行
          tmp0 = A[k][j + 0];
          tmp1 = A[k][j + 1];
          tmp2 = A[k][j + 2];
          tmp3 = A[k][j + 3];
          tmp4 = A[k][j + 4];
          tmp5 = A[k][j + 5];
          tmp6 = A[k][j + 6];
          tmp7 = A[k][j + 7];
          // 按列放入B。C1^T C2^T
          B[j + 0][k] = tmp0;
          B[j + 1][k] = tmp1;
          B[j + 2][k] = tmp2;
          B[j + 3][k] = tmp3;
          B[j + 0][k + 4] = tmp4;
          B[j + 1][k + 4] = tmp5; // hit
          B[j + 2][k + 4] = tmp6;
          B[j + 3][k + 4] = tmp7; // hit
        }
        //
        for (k = j; k < j + 4; ++k) { // 按列取A的左下角
          tmp0 = A[i + 4][k];
          tmp1 = A[i + 5][k];
          tmp2 = A[i + 6][k];
          tmp3 = A[i + 7][k];
          // 取出B的C2^T部分
          /* 上下对调 */
          tmp4 = B[k][i + 4];
          tmp5 = B[k][i + 5];
          tmp6 = B[k][i + 6];
          tmp7 = B[k][i + 7];
          // 放入B的右上角
          B[k][i + 4] = tmp0;
          B[k][i + 5] = tmp1;
          B[k][i + 6] = tmp2;
          B[k][i + 7] = tmp3;
          // 放入B的左下角
          B[k + 4][i] = tmp4;
          B[k + 4][i + 1] = tmp5;
          B[k + 4][i + 2] = tmp6;
          B[k + 4][i + 3] = tmp7;
        }
        for (k = i + 4; k < i + 8; k += 2) {
          tmp0 = A[k][j + 4];
          tmp1 = A[k][j + 5];
          tmp2 = A[k][j + 6];
          tmp3 = A[k][j + 7];
          tmp4 = A[k + 1][j + 4];
          tmp5 = A[k + 1][j + 5];
          tmp6 = A[k + 1][j + 6];
          tmp7 = A[k + 1][j + 7];

          B[j + 4][k] = tmp0;
          B[j + 5][k] = tmp1;
          B[j + 6][k] = tmp2;
          B[j + 7][k] = tmp3;
          B[j + 4][k + 1] = tmp4;
          B[j + 5][k + 1] = tmp5;
          B[j + 6][k + 1] = tmp6;
          B[j + 7][k + 1] = tmp7;
        }
      }
    }
  } else if (M == 61 && N == 67) {
    for (ii = 0; ii < N; ii += 16) {
      for (jj = 0; jj < M; jj += 16) {
        for (i = ii; i < N && i < ii + 16; i++) {
          for (j = jj; j < M && j < jj + 16; j++) {
            B[j][i] = A[i][j];
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
