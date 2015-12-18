/*
 * mm.c
 * Xi Lin(xlin2)
 *
 * Data structure to organize free list   :  segregated free list
 * Algorithms to find the best free block :  first fit and best fit
 * 
 * Heap structure:
 * 1) Allocated block structure:
 *                  1-31                    0 
 *    +---------------------------------------+ 
 *    |           block size              |a/f|                
 *    |                                       |
 *    |           payload                     |
 *    |                                       |
 *    |           block size              |a/f|
 *    +---------------------------------------+
 *    The first 4 bytes of the allocated block are its header, including
 *    the block size and one alloc/free bit. The last 4 bytes are the 
 *    footer, containing the same bits as the header. The content between
 *    header and footer is the payload
 * 
 * 2) Free block structure:
 *                  1-31                    0
 *    +---------------------------------------+
 *    |            block size             |a/f|
 *    |     next free block offset (NEXT_SECT)|
 *    | previous free block offset (PREV_SECT)|
 *    |                                       |
 *    |            payload                    |
 *    |                                       |
 *    |            block size             |a/f|
 *    +---------------------------------------+
 *    Same as the allocated block, free block also has a 4-byte header 
 *    and footer. The 4 bytes after the header is next section, which 
 *    stores the offset of next free block. The next 4 bytes stores the
 *    offset of the previous free block. These two sections connect free
 *    blocks together. The bytes between PREV_SECT and footer are payload.
 * 
 * 3) Initial heap structure:
 *    +---------------------------------------+
 *    |  free list bucket #0 offset           |
 *    |  free list bucket #1 offset           |
 *    |  free list bucket #2 offset           |
 *    |  ....                                 |
 *    |  free list bucket #9 offset           |
 *    |  padding                              |
 *    |  prologue header                      |
 *    |  prologue footer                      |
 *    |  epilogue                             |
 *    +---------------------------------------+
 *    After the call of mm_init function, the heap structure is like
 *    above. The first 10 items store the offset of the first block of
 *    each free list bucket. I used 10 buckets, each of which stores 
 *    blocks of different size ranges. All the offset take 8 bytes for 
 *    better alignment. After the offset table, there are 4-byte 0 padding,
 *    prologue header/footer, and 4-byte epilogue used for marking the
 *    end of heap.
 * 
 * Find fit algorithm:
 * 1) For malloc with small size ( < 128 bytes), first fit will be used.
 *    The algorithm will return the first empty block it finds that is 
 *    large enough.
 * 2) For malloc with large size ( >= 128 bytes), best fit will be used.
 *    The alogrithm will keep search and finally return the empty block
 *    that is large enough and minimizes fragmentation.
 * 
 * Program flow:
 * 1) In malloc, the program will first get the bucket index, then go to
 *    the corresponding bucket to find a free block. After it gets the 
 *    block, the block will be removed from the free list and set to be
 *    allocated. The pointer to the block will be returned.
 * 2) If no suitable block is found, the program will call extend_heap
 *    to create more spaces, then return the pointer to the block that is
 *    large enough.
 * 3) When free is called, the program will set the block to be free (if
 *    the pointer is not NULL), then merge the consecutive free blocks.
 *    The free block will be inserted back to free list.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

/* Basic constants and macros */
#define WSIZE       4           /* Word and header/footer size (bytes) */
#define DSIZE       8           /* Doubleword size (bytes) */
#define CHUNKSIZE   (1 << 9)    /* Extend heap by this amount (bytes) */
#define INISIZE     (1 << 12)   /* Extent heap by this amount initially*/

#define LIST_NUM    10          /* Numbers of different lists */
#define LIST_THR    4           /* Big size list index */

#define FREE        0x0         /* Block is free */
#define ALLOC       0x1         /* Block is allocated */

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)   ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)              (*(unsigned int *)(p))
#define PUT(p, val)         (*(unsigned int *)(p) = (val))

/* Read the size, previous allocated, and allocated fields from address p */
#define GET_SIZE(p)         (GET(p) & ~0x7)
#define GET_ALLOC(p)        (GET(p) & 0x1)

/* Given a block pointer bp, compute address of its header and footer */
#define HDRP(bp)            ((char *)(bp) - WSIZE)
#define FTRP(bp)            ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given a block pointer bp, compute address of next and prev section */
#define NEXT_SECT(bp)       (bp)
#define PREV_SECT(bp)       ((char *)(bp) + WSIZE)

/* Given a block pointer bp, compute address of next and prev empty blocks */
#define NEXT_EMPT_BLKP(bp)  (first_listp + (*(unsigned int *)(NEXT_SECT(bp))))
#define PREV_EMPT_BLKP(bp)  (first_listp + (*(unsigned int *)(PREV_SECT(bp))))

/* Given a block pointer bp, compute address of next and prev block */
#define NEXT_BLKP(bp)       ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) 	    ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Global variables */
static char *heap_listp = 0; /* free block count in heap */
static char *heap_basep = 0; /* point to the footer of prologue block */
static char *first_listp = 0;/* point to the first block of free list table */
static char *last_listp = 0; /* point to the last block of free list table */

/* Function prototypes */
static inline void ins_free_blk(void *bp, size_t index);
static inline void del_free_blk(void *bp);
static inline size_t list_index(size_t size);
static inline void place(void *bp, size_t asize);
static inline void* coalesce(void *bp);
static inline void* extend_heap(size_t words);
static inline void *find_fit(size_t asize);

/* Helper functions for heap checking */
static void check_heap_init(int lineno);
static void check_epi_and_pro(int lineno);
static size_t check_each_block(int lineno);
static size_t check_free_list(int lineno);

/*******************
 * Helper functions
 *******************/
 
/* 
 * ins_free_blk - Insert block to the head of the corresponding free list
 */
static inline void ins_free_blk(void *bp, size_t index) {
    /* target list to insert the block */
    char *target_listp = first_listp + index * DSIZE;
    
    /* set the next and previous pointers to connect consecutive blocks */
    PUT(NEXT_SECT(bp), GET(target_listp));
    PUT(PREV_SECT(bp), GET(PREV_SECT(NEXT_EMPT_BLKP(bp))));
    
    /* set the offset of the free list table */
    PUT(target_listp, (long)bp - (long)first_listp);
    PUT(PREV_SECT(NEXT_EMPT_BLKP(bp)), (long)bp - (long)first_listp);
}

/* 
 * del_free_blk - Delete block from the corresponding free list
 */
static inline void del_free_blk(void *bp) {
    PUT(PREV_SECT(NEXT_EMPT_BLKP(bp)), GET(PREV_SECT(bp)));
    PUT(NEXT_SECT(PREV_EMPT_BLKP(bp)), GET(NEXT_SECT(bp)));
}

/*
 * list_index - Returns the free list index according to the size
 * 0: [0-15], 1: [16-31], 2: [32-63] ...... 8: [2048-4095], 9: [>=4096]
 */
static inline size_t list_index(size_t size) {
    size_t idx = 0;
    
    
    size >>= 4;
    while (size > 0) {
        ++idx;
        size >>= 1;
    }
    
    /* the max index is 9 */
    if (idx > LIST_NUM - 1)
        idx = LIST_NUM - 1;
    
    return idx;
    
}

/* 
 * place - Place asize bytes at the start of the block indicated by bp.
 * If the remaining part is larger than 16 bytes, split the block
 */
static inline void place(void *bp, size_t asize) {
    size_t bsize = GET_SIZE(HDRP(bp));
    
    if (bsize - asize >= 2 * DSIZE) { // Split the block
        del_free_blk(bp);
        
        /* Set size and alloc bit of the block */
        PUT(HDRP(bp), PACK(asize, ALLOC));
        PUT(FTRP(bp), PACK(asize, ALLOC));
        
        bp = NEXT_BLKP(bp);
        
        /* Put the remaining block into the free list */
        PUT(HDRP(bp), PACK(bsize - asize, FREE));
        PUT(FTRP(bp), PACK(bsize - asize, FREE));
        
        size_t idx = list_index(bsize - asize);
        ins_free_blk(bp, idx);
    }
    else { // No split
        del_free_blk(bp);
        
        PUT(HDRP(bp), PACK(bsize, ALLOC));
        PUT(FTRP(bp), PACK(bsize, ALLOC));
    }
}

/* 
 * coalesce - Combine adjacent free blocks and return the pointer to the
 * result block.
 *
 * There are 4 cases:
 * 1. Both next and previous blocks are allocated, no combination
 * 2. Previous block is free and next block is allocated.
 *    New block is combined with the previous block.
 * 3. Previous Block is allocated and next block is free.
 *    New block is combined with the next block.
 * 4. Both next and previous blocks are free.
 *    All three blocks are combined together.
 */
static inline void* coalesce(void *bp) {
    /* Allocation of the next and previous blocks */
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    size_t idx;
    
    if (prev_alloc && next_alloc) {       // Case 1
        idx = list_index(size);
        ins_free_blk(bp, idx);  // add the new block to the free list
    }
    else if (!prev_alloc && next_alloc) { // Case 2
        del_free_blk(PREV_BLKP(bp)); // delete previous block from list
        
        /* Combination */
        bp = PREV_BLKP(bp);
        size += GET_SIZE(HDRP(bp));
        idx = list_index(size);
        PUT(HDRP(bp), PACK(size, FREE));
        PUT(FTRP(bp), PACK(size, FREE));
        
        ins_free_blk(bp, idx); // add the combined block to the list
    }
    else if (prev_alloc && !next_alloc) { // Case 3
        del_free_blk(NEXT_BLKP(bp)); // delete next block from list
        
        /* Combination */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        idx = list_index(size);
        PUT(HDRP(bp), PACK(size, FREE));
        PUT(FTRP(bp), PACK(size, FREE));
        
        ins_free_blk(bp, idx); // add the combined block to the list
    }
    else {                                // Case 4
        del_free_blk(PREV_BLKP(bp)); // delete previous block from list
        del_free_blk(NEXT_BLKP(bp)); // delete next block from list
        
        /* Combination */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        idx = list_index(size);
        
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, FREE));
        PUT(FTRP(bp), PACK(size, FREE));
        
        ins_free_blk(bp, idx); // add the combined block to the list
    }
    
    return bp;
}

/* 
 * extend_heap - extend the heap according to word size parameter
 */
static inline void* extend_heap(size_t words) {
    char *bp;       // pointer to block created
    size_t size;    // size in bytes
    
    /* Change the size to even word number */
    size = (words % 2) ? WSIZE * (words + 1) : WSIZE * words;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    
    /* Set values to header/footer of the new block and the epilogue */
    PUT(HDRP(bp), PACK(size, FREE));
    PUT(FTRP(bp), PACK(size, FREE));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, ALLOC));
    
    return coalesce(bp);
}

/* 
 * find_fit - finds a suitable block according to the required size
 */
static inline void *find_fit(size_t asize) {
    size_t idx = list_index(asize);
    /* Start search from this list */
    char *start_listp = first_listp + idx * DSIZE;
    void *bp;
    
    /* first fit */
    if (idx < LIST_THR) { // if size is less than 128 bytes
        for (void *list_head = start_listp; list_head != last_listp + DSIZE; 
            list_head = (char *)list_head + DSIZE) {
                
            bp = NEXT_EMPT_BLKP(list_head);
            while (bp != list_head) {
                /* return pointer as soon as the block is found */
                if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize)
                    return bp;
                bp = NEXT_EMPT_BLKP(bp);
            }
        }
        /* cannot find */
        return NULL;
    }
    
    /* best fit */
    else { // if size is not less than 128 bytes
        void *best_bp = NULL;
        size_t best_size = 16 * (1 << idx);
        for (void *list_head = start_listp; list_head != last_listp + DSIZE;
            list_head = (char *)list_head + DSIZE) {
            
            bp = NEXT_EMPT_BLKP(list_head);
            while (bp != list_head) {
                if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize) {
                    /* if the block size is less than best size, update
                     * the best block */ 
                    if (!best_bp || GET_SIZE(HDRP(bp)) < best_size) {
                        best_size = GET_SIZE(HDRP(bp));
                        best_bp = bp;
                    }
                }
                bp = NEXT_EMPT_BLKP(bp);
            }
            
        }
        return best_bp;
    }
    
}

/***********************
 * End helper functions
 ***********************/

/*
 * Initialize - initialize the heap. Return -1 on error, 0 on success.
 */
int mm_init(void) {
    /* size to hold a free list table plus padding, prologue and epilogue */
    size_t size = (LIST_NUM * 2 + 4) * WSIZE; 
    
    /* checks if requested size is allocated */
    if ((heap_listp = mem_sbrk(size)) == (void *)-1)
        return -1;
    
    /* initialize pointers */
    heap_basep = heap_listp;
    first_listp = heap_listp;
    last_listp = first_listp + (LIST_NUM - 1) * DSIZE;
    
    /* initialize free list offset table */
    size_t offset = 0;
    for (size_t i = 0; i < LIST_NUM; ++i) {
        PUT(heap_basep + offset, offset);
        PUT(heap_basep + offset + WSIZE, offset);
        offset += DSIZE;
    }
    
    /* initialize padding, prologue and epilogue blocks */
    heap_listp = last_listp + DSIZE; 
    PUT(heap_listp, 0);
    PUT(heap_listp + WSIZE, PACK(DSIZE, ALLOC));
    PUT(heap_listp + 2 * WSIZE, PACK(DSIZE, ALLOC));
    PUT(heap_listp + 3 * WSIZE, PACK(0, ALLOC));
    heap_basep = heap_listp + 2 * WSIZE;
    
    /* extend the empty heap */
    if (extend_heap(INISIZE / WSIZE) == NULL) {
        return -1;
    }
    
    return 0;
}

/*
 * malloc - returns pointer to allocated block if successful, else returns 
 * NULL
 */
void *malloc (size_t size) {
    size_t adjustsize;
    size_t extendsize;
    void *bp;
    
    /* Ignore requests of 0 payload*/
    if (size == 0) {
        return NULL;
    }
    
    /* Adjust size to include header and footer */
    if (size <= DSIZE) {
        adjustsize = 2 * DSIZE;
    }
    else {
        adjustsize = DSIZE * ((size + DSIZE + DSIZE - 1) / DSIZE);
    }
    
    /* Find the suitable free block to hold the data */
    if ((bp = find_fit(adjustsize)) != NULL) {
        place(bp, adjustsize);
        return bp;
    }
    
    /* If cannot find, extend the heap so that there is enough space */
    extendsize = adjustsize > CHUNKSIZE ? adjustsize : CHUNKSIZE;
    if ((bp = extend_heap(extendsize / WSIZE)) != NULL) {
        place(bp, adjustsize);
        return bp;
    }
    
    return NULL;
}

/*
 * free - if ptr is NULL, do nothing, else changes alloc bit of the block
 * to be freed and combines adjacent blocks if needed.
 */
void free (void *ptr) {
    if(!ptr) return;
    
    size_t size = GET_SIZE(HDRP(ptr));
    
    /* Change alloc bit in header and footer of the block to free */
    PUT(HDRP(ptr), PACK(size, FREE));
    PUT(FTRP(ptr), PACK(size, FREE));
    
    coalesce(ptr);
}

/*
 * realloc - allocated a new block and move old data into the new space.
 * If failed, return NULL.
 */
void *realloc(void *oldptr, size_t size) {
    size_t oldsize;
    void *newptr;
    
     /* If size == 0 then this is just free, and we return NULL. */
    if (size == 0) {
        free(oldptr);
        return NULL;
    }
    
    /* If oldptr is NULL, then this is just malloc. */
    if (!oldptr) {
        return malloc(size);
    }
    
    /* Realloc fails, return NULL */
    if ((newptr = malloc(size)) == NULL) {
        return NULL;
    }
    
    /* Copy the old data and fre the old block. */
    oldsize = GET_SIZE(HDRP(oldptr));
    if (oldsize > size) {
        oldsize = size;
    }
    memcpy(newptr, oldptr, oldsize);
    free(oldptr);
    
    return newptr;
}

/*
 * calloc - do malloc and initialize the content to 0
 */
void *calloc (size_t nmemb, size_t size) {
    size_t totalsize = nmemb * size;
    void *bp = malloc(totalsize);
    
    memset(bp, 0, totalsize);
    
    return bp;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) {
    return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * Check whether the heap has been initialized correctly.
 */
static void check_heap_init(int lineno) {
    /* Heap haven't been initialized */
	if(heap_basep == NULL) {
        printf("%d: Heap haven't been initialized!\n", lineno);
	}
    
    /* Heap not initialized correctly */
	if(!aligned(heap_basep)) { 
        printf("%d: Heap incorrectly aligned!\n", lineno);
	}
	
	if(!in_heap(heap_basep)) {
        printf("%d: Heap not in allocated memory!\n", lineno);
	}
}

/* 
 * Check Epilogue and Prologue blocks 
 */
static void check_epi_and_pro(int lineno) {
    /* Checks prologue header, footer, and epilogue blocks */
    if (GET(HDRP(heap_basep)) != PACK(DSIZE, ALLOC)) {
        printf("%d: Prologue block header error!\n", lineno);
    }
    
    if (GET(heap_basep) != PACK(DSIZE, ALLOC)) {
        printf("%d: Prologue block footer error!\n", lineno);
    }
    
    if (GET((char *)mem_heap_hi() - 3) != PACK(0, ALLOC)) {
        printf("%d: Epilogue block error!\n", lineno);
    }
    
    /* Checks if prologue and epilogue blocks are inside heap range */
    if (!in_heap(HDRP(heap_basep))) {
        printf("%d: Prologue block header not in heap!\n", lineno);
    }
    
    if (!in_heap(heap_basep)) {
        printf("%d: Prologue block footer not in heap!\n", lineno);
    }
    
    if (!in_heap((char *)mem_heap_hi() - 3)) {
        printf("%d: Epilogue block not in heap!\n", lineno);
    }
}

/* 
 * Check each blocks in heap
 */
static size_t check_each_block(int lineno) {
	char *iter = heap_basep + WSIZE;    // pointer to traverse heap
    size_t free_cnt1 = 0;               // free block count in heap
    
    /* 
     * Checks alignment of each blocks and if header and footer match. 
     * Finally checks if there are two consecutive free blocks.
	 */
	while(GET(iter) != PACK(0, ALLOC)) { // stops when header is epilogue
		iter = iter + WSIZE;             // block address
		
		/* Each block's address alignment */
		if(!aligned(iter)) {
            printf("Block at 0x%lx isn't correctly aligned!\n", (long)iter);
            printf("Line number is %d\n", lineno);
		}
		
        /* Increase free block count */
		if(GET_ALLOC(HDRP(iter)) == FREE) {
            ++free_cnt1;
		}
		
		/* Test header and footer match */
		if(GET(HDRP(iter)) != GET(FTRP(iter))) {
            printf("Header and footer of block 0x%lx don't match!\n",
                  (long)iter);
            printf("Line number is %d\n", lineno);
		}
		
        /* Two consecutive free blocks */
		if((GET_ALLOC(HDRP(iter))) == 0 && 
            (GET_ALLOC(HDRP(NEXT_BLKP(iter))) == 0)) {
            printf("Consecutive free blocks at 0x%lx & 0x%lx!\n",
                   (long)iter, (long)NEXT_BLKP(iter));
            printf("Line number is %d\n", lineno);
		}
		
		iter = HDRP(NEXT_BLKP(iter));      // next block header
	}
    
    return free_cnt1;
}

/*
 * Checks the segregated free list
 */
static size_t check_free_list(int lineno) {
    char *list_head;                    // pointer to traverse free list
    size_t idx = 0;                     // current bucket index
    size_t free_cnt2 = 0;               // free block count in free list
    
    for (list_head = first_listp; list_head != last_listp + DSIZE; 
        list_head = list_head + DSIZE) {
            
        char *bp = NEXT_EMPT_BLKP(list_head);
        while (bp != list_head) {
            /* Checks if a free block is inside heap */
            if (!in_heap(bp)) {
                printf("Free block 0x%lx not in heap!\n", (long)bp);
                printf("Line number is %d\n", lineno);
            }
            
            /* Checks if a free block is in the right bucket */
            if (list_index(GET_SIZE(HDRP(bp))) != idx) {
                printf("Free block 0x%lx in wrong heap!\n", (long)bp);
                printf("Line number is %d\n", lineno);
            }
            
            /* Checks next/previous pointers consistency */
            if (bp != first_listp + GET(PREV_SECT(NEXT_EMPT_BLKP(bp))) ||
                NEXT_EMPT_BLKP(bp) != first_listp + GET(NEXT_SECT(bp))) {
                printf("Next/prev pointers inconsistent! 0x%lx & 0x%lx\n",
                       (long)bp, (long)NEXT_EMPT_BLKP(bp));
                printf("Line number is %d\n", lineno);
            }
            
            /* Increase free block count in free list */
            ++free_cnt2;
            bp = NEXT_EMPT_BLKP(bp);      // next free block
        }
        
        ++idx;
    }
    
    return free_cnt2;
}

/*
 * mm_checkheap
 */
void mm_checkheap(int lineno) {
    size_t free_cnt1;            // free block count in heap
    size_t free_cnt2;            // free block count in free list
    
    check_heap_init(lineno);
    
    check_epi_and_pro(lineno);
    
    free_cnt1 = check_each_block(lineno);
    
    free_cnt2 = check_free_list(lineno);
    
    /* Checks if the numbers of free blocks in heap and free list mactch */
    if (free_cnt1 != free_cnt2) {
        printf("Number of free blocks in heap and free list mismatch!\n");
    }
    
}
