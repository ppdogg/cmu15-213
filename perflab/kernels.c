/********************************************************
 * Kernels to be optimized for the CS:APP Performance Lab
 ********************************************************/

#include "defs.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * Please fill in the following team struct
 */
team_t team = {
    "losers", /* Team name */

    "Harry Q. Bovik",    /* First member full name */
    "bovik@nowhere.edu", /* First member email address */

    "Harley Quinn",      /* Second member full name (leave blank if none) */
    "Harlequin@home.edu" /* Second member email addr (leave blank if none) */
};

/***************
 * ROTATE KERNEL
 ***************/

/******************************************************
 * Your different versions of the rotate kernel go here
 ******************************************************/

/*
 * naive_rotate - The naive baseline version of rotate
 */
char naive_rotate_descr[] = "naive_rotate: Naive baseline implementation";
void naive_rotate(int dim, pixel *src, pixel *dst) {
  int i, j;

  for (i = 0; i < dim; i++)
    for (j = 0; j < dim; j++)
      dst[RIDX(dim - 1 - j, i, dim)] = src[RIDX(i, j, dim)];
}

char block_rotate_descr[] = "block_rotate: Use block to cache data";
void block_rotate(int dim, pixel *src, pixel *dst) {
  int ii, jj, i, j;
  int b_size = 16;
  for (ii = 0; ii < dim; ii += b_size) {
    for (jj = 0; jj < dim; jj += b_size) {
      for (i = ii; i < ii + b_size; i++) {
        for (j = jj; j < jj + b_size; j++) {
          dst[RIDX(dim - 1 - j, i, dim)] = src[RIDX(i, j, dim)];
        }
      }
    }
  }
}

char cache_block_rotate_descr[] = "cache_block_rotate: Use block to cache data";
void cache_block_rotate(int dim, pixel *src, pixel *dst) {
  int ii, jj, i, j, k;
  int b_size = 8;
  pixel tmp[8];
  for (ii = 0; ii < dim; ii += b_size) {
    for (jj = 0; jj < dim; jj += b_size) {
      for (i = ii; i < dim && i < ii + b_size; i++) {
        for (j = jj; j < dim && j < jj + b_size; j++) {
          tmp[j - jj] = src[RIDX(i, j, dim)];
        }
        for (k = jj; k < j; k++) {
          dst[RIDX(dim - 1 - k, i, dim)] = tmp[k - jj];
        }
      }
    }
  }
}

char raw_rotate_descr[] = "raw_rotate: single loop";
void raw_rotate(int dim, pixel *src, pixel *dst) {
  int i, total, res, round = 0;
  total = (dim * dim) - 1;
  res = dim * (dim - 1);
  for (i = 0; i <= total; i++) {
    round = i / dim;
    dst[res - ((i - round * dim) * dim) + round] = src[i];
  }
}

char raw_block_rotate_descr[] = "raw_block_rotate: single loop in 1-dim block";
void raw_block_rotate(int dim, pixel *src, pixel *dst) {
  int i, j;
  int total, res;
  int round = 0, b_size = 2;

  total = (dim * dim) - 1;
  res = dim * (dim - 1);

  for (i = 0; i <= total; i += b_size) {
    if (i % dim == 0 && i > 0) {
      round++;
    }
    for (j = i; j < i + b_size && j <= total; j++) {
      dst[res - ((j - round * dim) * dim) + round] = src[j];
    }
  }
}

char raw_22_rotate_descr[] = "raw_42_rotate: 4 step with 2 variable";
void raw_22_rotate(int dim, pixel *src, pixel *dst) {
  int i, j, total, res, round = 0;
  total = (dim * dim) - 1;
  res = dim * (dim - 1);
  for (i = 0, j = 1; j <= total; i += 4, j += 4) {
    round = i / dim;
    dst[res - ((i - round * dim) * dim) + round] = src[i];
    dst[res - ((i + 2 - round * dim) * dim) + round] = src[i + 2];
    dst[res - ((j - round * dim) * dim) + round] = src[j];
    dst[res - ((j + 2 - round * dim) * dim) + round] = src[j + 2];
  }
}

/*
 * rotate - Your current working version of rotate
 * IMPORTANT: This is the version you will be graded on
 */
char rotate_descr[] = "rotate: Current working version";
void rotate(int dim, pixel *src, pixel *dst) { block_rotate(dim, src, dst); }

/*********************************************************************
 * register_rotate_functions - Register all of your different versions
 *     of the rotate kernel with the driver by calling the
 *     add_rotate_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.
 *********************************************************************/

void register_rotate_functions() {
  add_rotate_function(&naive_rotate, naive_rotate_descr);
  add_rotate_function(&block_rotate, block_rotate_descr);
  add_rotate_function(&cache_block_rotate, cache_block_rotate_descr);
  add_rotate_function(&raw_rotate, raw_rotate_descr);
  add_rotate_function(&raw_block_rotate, raw_block_rotate_descr);
  add_rotate_function(&raw_22_rotate, raw_22_rotate_descr);
  add_rotate_function(&rotate, rotate_descr);
  /* ... Register additional test functions here */
}

/***************
 * SMOOTH KERNEL
 **************/

/***************************************************************
 * Various typedefs and helper functions for the smooth function
 * You may modify these any way you like.
 **************************************************************/

/* A struct used to compute averaged pixel value */
const int bias[9] = {-1, 0, 1, -1, 0, 1, -1, 0, 1};
const int dic[9] = {-1, -1, -1, 0, 0, 0, 1, 1, 1};

typedef struct {
  int red;
  int green;
  int blue;
  int num;
} pixel_sum;

/* Compute min and max of two integers, respectively */
static int min(int a, int b) { return (a < b ? a : b); }
static int max(int a, int b) { return (a > b ? a : b); }

/*
 * initialize_pixel_sum - Initializes all fields of sum to 0
 */
static void initialize_pixel_sum(pixel_sum *sum) {
  sum->red = sum->green = sum->blue = 0;
  sum->num = 0;
  return;
}

/*
 * accumulate_sum - Accumulates field values of p in corresponding
 * fields of sum
 */
static void accumulate_sum(pixel_sum *sum, pixel p) {
  sum->red += (int)p.red;
  sum->green += (int)p.green;
  sum->blue += (int)p.blue;
  sum->num++;
  return;
}

/*
 * assign_sum_to_pixel - Computes averaged pixel value in current_pixel
 */
static void assign_sum_to_pixel(pixel *current_pixel, pixel_sum sum) {
  current_pixel->red = (unsigned short)(sum.red / sum.num);
  current_pixel->green = (unsigned short)(sum.green / sum.num);
  current_pixel->blue = (unsigned short)(sum.blue / sum.num);
  return;
}

/*
 * avg - Returns averaged pixel value at (i,j)
 */
static pixel avg(int dim, int i, int j, pixel *src) {
  int ii, jj;
  pixel_sum sum;
  pixel current_pixel;

  initialize_pixel_sum(&sum);
  for (ii = max(i - 1, 0); ii <= min(i + 1, dim - 1); ii++)
    for (jj = max(j - 1, 0); jj <= min(j + 1, dim - 1); jj++)
      accumulate_sum(&sum, src[RIDX(ii, jj, dim)]);

  assign_sum_to_pixel(&current_pixel, sum);
  return current_pixel;
}

static pixel avg_mid(int dim, int i, pixel *src) {
  int k, pos;
  pixel_sum sum;
  pixel current_pixel;

  initialize_pixel_sum(&sum);
  for (k = 0; k < 9; k++) {
    pos = i + dic[k] * dim + bias[k];
    accumulate_sum(&sum, src[pos]);
  }

  assign_sum_to_pixel(&current_pixel, sum);
  return current_pixel;
}

/******************************************************
 * Your different versions of the smooth kernel go here
 ******************************************************/

/*
 * naive_smooth - The naive baseline version of smooth
 */
char naive_smooth_descr[] = "naive_smooth: Naive baseline implementation";
void naive_smooth(int dim, pixel *src, pixel *dst) {
  int i, j;

  for (i = 0; i < dim; i++)
    for (j = 0; j < dim; j++)
      dst[RIDX(i, j, dim)] = avg(dim, i, j, src);
}

char block_smooth_descr[] = "block_smooth: Use block to cache data";
void block_smooth(int dim, pixel *src, pixel *dst) {
  int ii, jj, i, j;
  int b_size = 15;

  for (ii = 0; ii < dim; ii++) {
    for (jj = 0; jj < dim; jj++) {
      for (i = ii; i < dim && i < ii + b_size; i++) {
        for (j = jj; j < dim && j < jj + b_size; j++) {
          dst[RIDX(i, j, dim)] = avg(dim, i, j, src);
        }
      }
    }
  }
}

char separate_smooth_descr[] = "separte_smooth: Separate diff types of points";
void separate_smooth(int dim, pixel *src, pixel *dst) {
  int i, j, base, pos;

  // angle
  dst[0] = avg(dim, 0, 0, src);
  dst[dim - 1] = avg(dim, 0, dim - 1, src);
  dst[RIDX(dim - 1, 0, dim)] = avg(dim, dim - 1, 0, src);
  dst[RIDX(dim - 1, dim - 1, dim)] = avg(dim, dim - 1, dim - 1, src);

  // edge points
  for (i = 1; i < dim - 1; i++) {
    dst[i * dim] = avg(dim, i, 0, src);
    dst[RIDX(i, dim - 1, dim)] = avg(dim, i, dim - 1, src);
    dst[i] = avg(dim, 0, i, src);
    dst[RIDX(dim - 1, i, dim)] = avg(dim, dim - 1, i, src);
  }

  // mid points
  for (i = 1; i < dim - 1; i++) {
    base = i * dim;
    for (j = 1; j < dim - 1; j++) {
      pos = base + j;
      dst[pos] = avg_mid(dim, pos, src);
    }
  }
}

char separate_lesscall_smooth_descr[] =
    "separte_lesscall_smooth: Separate diff types of points with less func "
    "call";
void separate_lesscall_smooth(int dim, pixel *src, pixel *dst) {
  int i, j, base, pos;

  // angle
  dst[0] = avg(dim, 0, 0, src);
  dst[dim - 1] = avg(dim, 0, dim - 1, src);
  dst[RIDX(dim - 1, 0, dim)] = avg(dim, dim - 1, 0, src);
  dst[RIDX(dim - 1, dim - 1, dim)] = avg(dim, dim - 1, dim - 1, src);

  // edge points
  for (i = 1; i < dim - 1; i++) {
    dst[i * dim] = avg(dim, i, 0, src);
    dst[RIDX(i, dim - 1, dim)] = avg(dim, i, dim - 1, src);
    dst[i] = avg(dim, 0, i, src);
    dst[RIDX(dim - 1, i, dim)] = avg(dim, dim - 1, i, src);
  }

  // mid points
  for (i = 1; i < dim - 1; i++) {
    base = i * dim;
    for (j = 1; j < dim - 1; j++) {
      pos = base + j;
      dst[pos].red =
          (unsigned short)((src[pos - dim - 1].red + src[pos - dim].red +
                            src[pos - dim + 1].red + src[pos - 1].red +
                            src[pos].red + src[pos + 1].red +
                            src[pos + dim - 1].red + src[pos + dim].red +
                            src[pos + dim + 1].red) /
                           9);
      dst[pos].blue =
          (unsigned short)((src[pos - dim - 1].blue + src[pos - dim].blue +
                            src[pos - dim + 1].blue + src[pos - 1].blue +
                            src[pos].blue + src[pos + 1].blue +
                            src[pos + dim - 1].blue + src[pos + dim].blue +
                            src[pos + dim + 1].blue) /
                           9);
      dst[pos].green =
          (unsigned short)((src[pos - dim - 1].green + src[pos - dim].green +
                            src[pos - dim + 1].green + src[pos - 1].green +
                            src[pos].green + src[pos + 1].green +
                            src[pos + dim - 1].green + src[pos + dim].green +
                            src[pos + dim + 1].green) /
                           9);
    }
  }
}

char separate_nocall_smooth_descr[] =
    "separte_nocall_smooth: Separate diff types of points with no func call";
void separate_nocall_smooth(int dim, pixel *src, pixel *dst) {
  int curr;

  // Left Top
  dst[0].red = (src[0].red + src[1].red + src[dim].red + src[dim + 1].red) >> 2;
  dst[0].green =
      (src[0].green + src[1].green + src[dim].green + src[dim + 1].green) >> 2;
  dst[0].blue =
      (src[0].blue + src[1].blue + src[dim].blue + src[dim + 1].blue) >> 2;

  // Right Top
  curr = dim - 1;
  dst[curr].red = (src[curr].red + src[curr - 1].red + src[curr + dim - 1].red +
                   src[curr + dim].red) >>
                  2;
  dst[curr].green = (src[curr].green + src[curr - 1].green +
                     src[curr + dim - 1].green + src[curr + dim].green) >>
                    2;
  dst[curr].blue = (src[curr].blue + src[curr - 1].blue +
                    src[curr + dim - 1].blue + src[curr + dim].blue) >>
                   2;

  // Left Bottom
  curr *= dim;
  dst[curr].red = (src[curr].red + src[curr + 1].red + src[curr - dim].red +
                   src[curr - dim + 1].red) >>
                  2;
  dst[curr].green = (src[curr].green + src[curr + 1].green +
                     src[curr - dim].green + src[curr - dim + 1].green) >>
                    2;
  dst[curr].blue = (src[curr].blue + src[curr + 1].blue + src[curr - dim].blue +
                    src[curr - dim + 1].blue) >>
                   2;

  // Right Bottom
  curr += dim - 1;
  dst[curr].red = (src[curr].red + src[curr - 1].red + src[curr - dim].red +
                   src[curr - dim - 1].red) >>
                  2;
  dst[curr].green = (src[curr].green + src[curr - 1].green +
                     src[curr - dim].green + src[curr - dim - 1].green) >>
                    2;
  dst[curr].blue = (src[curr].blue + src[curr - 1].blue + src[curr - dim].blue +
                    src[curr - dim - 1].blue) >>
                   2;

  // Smooth four edges
  int ii, jj, limit;

  // Edge 0
  limit = dim - 1;
  for (ii = 1; ii < limit; ii++) {
    dst[ii].red =
        (src[ii].red + src[ii - 1].red + src[ii + 1].red + src[ii + dim].red +
         src[ii + dim - 1].red + src[ii + dim + 1].red) /
        6;
    dst[ii].green = (src[ii].green + src[ii - 1].green + src[ii + 1].green +
                     src[ii + dim].green + src[ii + dim - 1].green +
                     src[ii + dim + 1].green) /
                    6;
    dst[ii].blue =
        (src[ii].blue + src[ii - 1].blue + src[ii + 1].blue +
         src[ii + dim].blue + src[ii + dim - 1].blue + src[ii + dim + 1].blue) /
        6;
  }

  // Edge 3
  limit = dim * dim - 1;
  for (ii = (dim - 1) * dim + 1; ii < limit; ii++) {
    dst[ii].red =
        (src[ii].red + src[ii - 1].red + src[ii + 1].red + src[ii - dim].red +
         src[ii - dim - 1].red + src[ii - dim + 1].red) /
        6;
    dst[ii].green = (src[ii].green + src[ii - 1].green + src[ii + 1].green +
                     src[ii - dim].green + src[ii - dim - 1].green +
                     src[ii - dim + 1].green) /
                    6;
    dst[ii].blue =
        (src[ii].blue + src[ii - 1].blue + src[ii + 1].blue +
         src[ii - dim].blue + src[ii - dim - 1].blue + src[ii - dim + 1].blue) /
        6;
  }

  // Edge 1
  limit = dim * (dim - 1);
  for (jj = dim; jj < limit; jj += dim) {
    dst[jj].red =
        (src[jj].red + src[jj + 1].red + src[jj - dim].red +
         src[jj - dim + 1].red + src[jj + dim].red + src[jj + dim + 1].red) /
        6;
    dst[jj].green = (src[jj].green + src[jj + 1].green + src[jj - dim].green +
                     src[jj - dim + 1].green + src[jj + dim].green +
                     src[jj + dim + 1].green) /
                    6;
    dst[jj].blue =
        (src[jj].blue + src[jj + 1].blue + src[jj - dim].blue +
         src[jj - dim + 1].blue + src[jj + dim].blue + src[jj + dim + 1].blue) /
        6;
  }

  // Edge 2
  for (jj = 2 * dim - 1; jj < limit; jj += dim) {
    dst[jj].red =
        (src[jj].red + src[jj - 1].red + src[jj - dim].red +
         src[jj - dim - 1].red + src[jj + dim].red + src[jj + dim - 1].red) /
        6;
    dst[jj].green = (src[jj].green + src[jj - 1].green + src[jj - dim].green +
                     src[jj - dim - 1].green + src[jj + dim].green +
                     src[jj + dim - 1].green) /
                    6;
    dst[jj].blue =
        (src[jj].blue + src[jj - 1].blue + src[jj - dim].blue +
         src[jj - dim - 1].blue + src[jj + dim].blue + src[jj + dim - 1].blue) /
        6;
  }

  // Remaining pixels
  int i, j;
  for (i = 1; i < dim - 1; i++) {
    for (j = 1; j < dim - 1; j++) {
      curr = i * dim + j;
      dst[curr].red = (src[curr].red + src[curr - 1].red + src[curr + 1].red +
                       src[curr - dim].red + src[curr - dim - 1].red +
                       src[curr - dim + 1].red + src[curr + dim].red +
                       src[curr + dim - 1].red + src[curr + dim + 1].red) /
                      9;
      dst[curr].green =
          (src[curr].green + src[curr - 1].green + src[curr + 1].green +
           src[curr - dim].green + src[curr - dim - 1].green +
           src[curr - dim + 1].green + src[curr + dim].green +
           src[curr + dim - 1].green + src[curr + dim + 1].green) /
          9;
      dst[curr].blue =
          (src[curr].blue + src[curr - 1].blue + src[curr + 1].blue +
           src[curr - dim].blue + src[curr - dim - 1].blue +
           src[curr - dim + 1].blue + src[curr + dim].blue +
           src[curr + dim - 1].blue + src[curr + dim + 1].blue) /
          9;
    }
  }
}

/*
 * smooth - Your current working version of smooth.
 * IMPORTANT: This is the version you will be graded on
 */
char smooth_descr[] = "smooth: Current working version";
void smooth(int dim, pixel *src, pixel *dst) { separate_smooth(dim, src, dst); }

/*********************************************************************
 * register_smooth_functions - Register all of your different versions
 *     of the smooth kernel with the driver by calling the
 *     add_smooth_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.
 *********************************************************************/

void register_smooth_functions() {
  add_smooth_function(&smooth, smooth_descr);
  add_smooth_function(&block_smooth, block_smooth_descr);
  add_smooth_function(&separate_smooth, separate_smooth_descr);
  add_smooth_function(&separate_lesscall_smooth,
                      separate_lesscall_smooth_descr);
  add_smooth_function(&separate_nocall_smooth, separate_nocall_smooth_descr);
  add_smooth_function(&naive_smooth, naive_smooth_descr);
  /* ... Register additional test functions here */
}
