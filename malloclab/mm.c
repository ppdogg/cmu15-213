/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "loser",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "Harley Quinn",
    /* Second member's email address (leave blank if none) */
    "harlequin@chicago.edu",
};

// ***************************************************************************
// Memory
// --------------------------------------------------------------------------
// | Block1-Block10 | Padding | Prologue Block    | Blocks | Epilogue Block |
// |   ptr1-ptr10   |         | Header  | Footer  |        |     Header     |
// --------------------------------------------------------------------------
// |    40 bytes    | 4 bytes | 4 bytes | 4 bytes | ...... |     4 bytes    |
// ↑
// |
// |
// heap_listp
void *heap_listp;
// Free Block
// ----------------------------------------------------------------
// |      Header    |   Pred  |   Succ  |  Data  |     Footer     |
// ----------------------------------------------------------------
// ↑    4 bytes     ↑ 4 bytes ↑ 4 bytes ↑        ↑    4 bytes     ↑
// |----------------|---------|---------|        |----------------|
// :  3-31  | 2 1 0 |                            :  3-31  | 2 1 0 :
// |----------------|                            |----------------|
//                  |
//                  |
//               block_ptr
// ***************************************************************************

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and marcros */
#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12)
#define CLASS_NUM 8

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-DSIZE))

/* Given block ptr bp, compute address of its precedent and successor */
#define PRECEDENT(bp) ((char *)(bp))
#define SUCCESSOR(bp) ((char *)(bp) + WSIZE)

int mm_check();
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t size);
static void place(void *bp, size_t asize);
static void *search_head(size_t size);
static void insert_free_block(void *bp);
static void remove_free_block(void *bp);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  int i;

  if ((heap_listp = mem_sbrk((CLASS_NUM + 4) * WSIZE)) == (void *)-1) {
    return -1;
  }

  for (i = 0; i < CLASS_NUM; i++) {
    PUT(heap_listp + i * WSIZE, 0);
  }

  PUT(heap_listp + CLASS_NUM * WSIZE, 0); // Alignment padding
  PUT(heap_listp + ((CLASS_NUM + 1) * WSIZE),
      PACK(DSIZE, 1)); // Prologue header
  PUT(heap_listp + ((CLASS_NUM + 2) * WSIZE),
      PACK(DSIZE, 1));                                     // Prologue footer
  PUT(heap_listp + ((CLASS_NUM + 3) * WSIZE), PACK(0, 1)); // Epilogue header

  if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
    return -1;
  }
  return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */

void *mm_malloc(size_t size) {
  size_t asize;
  size_t extendsize;
  char *bp;

  if (size == 0) {
    return NULL;
  }

  if (size < DSIZE) {
    asize = 2 * DSIZE;
  } else {
    asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
  }

  if ((bp = find_fit(asize)) != NULL) {
    place(bp, asize);
    // mm_check();
    return bp;
  }

  extendsize = MAX(asize, CHUNKSIZE);
  if ((bp = extend_heap(extendsize / WSIZE)) == NULL) {
    return NULL;
  }
  place(bp, asize);
  // mm_check();
  return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
  size_t size;

  if (ptr == NULL) {
    return;
  }

  size = GET_SIZE(HDRP(ptr));
  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));
  coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
  void *oldptr = ptr;
  void *newptr;
  size_t copySize, n_size, new_size, res_size;

  if (ptr == NULL) {
    return mm_malloc(size);
  }
  if (size == 0) {
    mm_free(ptr);
    return NULL;
  }

  // get size
  new_size = ALIGN(size + DSIZE);
  copySize = GET_SIZE(HDRP(oldptr));

  // extend
  if (copySize < new_size) {
    n_size = GET_SIZE(HDRP(NEXT_BLKP(oldptr)));
    if (!GET_ALLOC(HDRP(NEXT_BLKP(oldptr))) && n_size + copySize > new_size) {
      remove_free_block(NEXT_BLKP(oldptr));
      PUT(HDRP(oldptr), PACK(copySize + n_size, 1));
      PUT(FTRP(oldptr), PACK(copySize + n_size, 1));
      return oldptr;
    }

    if ((newptr = mm_malloc(new_size)) == NULL)
      return NULL;

    memcpy(newptr, oldptr, copySize - DSIZE);
    mm_free(oldptr);
    return newptr;
  }

  // do nothing
  res_size = copySize - new_size;
  if (copySize == new_size || res_size < WSIZE * 4) {
    return oldptr;
  }

  // shrink
  PUT(HDRP(oldptr), PACK(new_size, 1));
  PUT(FTRP(oldptr), PACK(new_size, 1));
  PUT(HDRP(NEXT_BLKP(oldptr)), PACK(res_size, 0));
  PUT(FTRP(NEXT_BLKP(oldptr)), PACK(res_size, 0));
  coalesce(NEXT_BLKP(oldptr));

  return oldptr;
}

/*
 * mm_check - Check memory has no garbage and fragments
 */
int mm_check() {
  size_t b_size, is_alloc;
  void *bp;

  printf("--------mm_check--------\n");

  bp = heap_listp + (CLASS_NUM + 2) * WSIZE;
  is_alloc = GET_ALLOC(HDRP(bp));
  b_size = GET_SIZE(HDRP(bp));
  printf("block--------------------\n");
  while (b_size > 0) {
    printf("bp(%x)\n", HDRP(bp));
    printf("size(%zu), alloc(%zu)\n", b_size, is_alloc);
    printf("block--------------------\n");

    bp = NEXT_BLKP(bp);
    is_alloc = GET_ALLOC(HDRP(bp));
    b_size = GET_SIZE(HDRP(bp));
  }
  printf("bp(%x)\n", HDRP(bp));
  printf("size(%zu), alloc(%zu)\n", b_size, is_alloc);
  printf("block--------------------\n");

  return 0;
}

static void *extend_heap(size_t words) {
  char *bp;
  size_t size;

  size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
  if ((long)(bp = mem_sbrk(size)) == -1) {
    return NULL;
  }

  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // New epilogue header

  return coalesce(bp);
}

static void *coalesce(void *bp) {
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if (prev_alloc && next_alloc) {
    // skip
  } else if (prev_alloc && !next_alloc) {
    remove_free_block(NEXT_BLKP(bp));
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  } else if (!prev_alloc && next_alloc) {
    remove_free_block(PREV_BLKP(bp));
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  } else {
    remove_free_block(PREV_BLKP(bp));
    remove_free_block(NEXT_BLKP(bp));
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    size += GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  insert_free_block(bp);
  return bp;
}

static void *find_fit(size_t size) {
  void *head_ptr, *bp;

  head_ptr = (char *)search_head(size);
  while (head_ptr != heap_listp + CLASS_NUM * WSIZE) {
    bp = (void *)GET(head_ptr);
    while (bp != (void *)0) {
      if (GET_SIZE(HDRP(bp)) >= size) {
        return (void *)bp;
      }
      bp = (void *)GET(SUCCESSOR(bp));
    }
    head_ptr += WSIZE;
  }

  return NULL;
}

static void place(void *bp, size_t asize) {
  size_t size;

  remove_free_block(bp);
  size = GET_SIZE(HDRP(bp));
  if (size - asize < WSIZE * 4) {
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
    return;
  }

  PUT(HDRP(bp), PACK(asize, 1));
  PUT(FTRP(bp), PACK(asize, 1));

  PUT(HDRP(NEXT_BLKP(bp)), PACK(size - asize, 0));
  PUT(FTRP(NEXT_BLKP(bp)), PACK(size - asize, 0));
  insert_free_block(NEXT_BLKP(bp));
}

static void *search_head(size_t size) {
  int i;

  if (size > (1 << (CLASS_NUM + 2))) {
    return heap_listp + (CLASS_NUM - 1) * WSIZE;
  }

  for (i = 4; i <= (CLASS_NUM + 2); i++) {
    if (size <= (1 << i)) {
      break;
    }
  }

  return heap_listp + (i - 4) * WSIZE;
}

static void insert_free_block(void *bp) {
  void *head_ptr;

  head_ptr = search_head(GET_SIZE(HDRP(bp)));
  if (GET(head_ptr) != 0) {
    PUT(PRECEDENT(GET(head_ptr)), (unsigned int)bp);
  }

  PUT(PRECEDENT(bp), 0);
  PUT(SUCCESSOR(bp), GET(head_ptr));
  PUT(head_ptr, (unsigned int)bp);
}

static void remove_free_block(void *bp) {
  void *head_ptr, *pred_ptr, *succ_ptr;

  pred_ptr = (void *)GET(PRECEDENT(bp));
  succ_ptr = (void *)GET(SUCCESSOR(bp));

  if (pred_ptr == (void *)0) {
    head_ptr = search_head(GET_SIZE(HDRP(bp)));
    PUT(head_ptr, (unsigned int)succ_ptr);
  } else {
    PUT(SUCCESSOR(pred_ptr), (unsigned int)succ_ptr);
  }
  if (succ_ptr != (void *)0) {
    PUT(PRECEDENT(succ_ptr), (unsigned int)pred_ptr);
  }
}
