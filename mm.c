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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
// for using (uint_ptr_t) type
#include <stdint.h>


#include "mm.h"
#include "memlib.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* My define macro functions start */
#define WSIZE     4          // word and header/footer size (bytes)
#define DSIZE     8          // double word size (bytes)
#define CHUNKSIZE (1<<12)//+(1<<7)
#define LIST 20

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

// Pack a size and allocated bit into a word
#define PACK(size, alloc) ((size) | (alloc))

// Read and write a word at address p
#define GET(p)            (*(unsigned int *)(p))
#define PUT(p, val)       (*(unsigned int *)(p) = (val))


// Read the size and allocation bit from address p
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_ALLOC_C(p) (GET_ALLOC(p)) ? ('a') : ('f')

// Address of block's header and footer
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// Given block ptr bp, compute address of next and previous blocks
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

// Address of free block's prev and next pointer
#define PREV_PTR(ptr) ((char *)(ptr))
#define NEXT_PTR(ptr) ((char *)(ptr) + WSIZE)

// Value of free block's prev and next pointer
#define PREV(ptr) (*(char **)(ptr))
#define NEXT(ptr) (*(char **)(NEXT_PTR(ptr)))

// Store predecessor or successor pointer for free blocks
#define SET_PTR(p1, p2) (*(unsigned int *)(p1) = (unsigned int)(p2))
//여기서 unsigned int값만큼만 바꾸기때문에 64bit 전체를 못바꾸는것같다..
//PREV, NEXT를 이용해서 덮어씌우는걸 생각해보는중이다.



/* My define functions start */
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
/* My define functions end */

int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *bp);
void *mm_realloc(void *ptr, size_t size);
void my_heapcheck();
void myprintblock(void *bp);
void mycheckblock(void *bp);

/* functions that I can use
   void *mem_sbrk(int incr)
   void *mem_heap_lo(void)
   void *mem_heap_hi(void)
   size_t mem_heapsize(void)
   size_t mem_pagesize(void)
 */

//Global variable
char *heap_listp; //beginning of the heap .
void *segregated_lists[LIST];

size_t align(size_t size);
int align_idx(size_t size);
void print_lists();
void print_nodes(void *node, int idx);
void init_segregated_lists();


int malloc_count=1;
int free_count=1;
int realloc_count=1;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t align(size_t size) {
	int i;
	for(i=0; i<LIST; i++){
		if((1<<i) >= size) return (1<<i);
	}
}
int align_idx(size_t size) {
	int i;
	for(i=0; i<LIST; i++){
		if((1<<i) >= size) return i;
	}
}

void init_segregated_lists() {
	for(int i=0; i<LIST; i++){
		segregated_lists[i] = NULL;
	}
}
void delete_node(void *bp) {
  printf("delete_node\n");
  size_t size = GET_SIZE(HDRP(bp));
  int asize_idx = align_idx(size);

  if(PREV(bp) != NULL) {
    if(NEXT(bp) != NULL) {
  printf("delete test1\n");
      SET_PTR(NEXT_PTR(PREV(bp)), NEXT(bp));
      SET_PTR(PREV_PTR(NEXT(bp)), PREV(bp));
    }else {
  printf("delete test2\n");
      SET_PTR(NEXT_PTR(PREV(bp)), NULL);
    }
  }else {
    if(NEXT(bp) != NULL) {
  printf("delete test3\n");
      SET_PTR(PREV_PTR(NEXT(bp)), NULL);
      segregated_lists[asize_idx]= NEXT(bp);
    }else {
  printf("delete test4\n");
      segregated_lists[asize_idx]= NULL;
    }
  }
  return;
}

void insert_node(void *bp, size_t size){ //insert into the first position
  printf("insert_node %p\n", bp);
	int asize_idx = align_idx(size);
  printf("asize_idx = %d\n", asize_idx);
	void *front= segregated_lists[asize_idx];
  void *back=NULL;
  //find suitable free block in lists
  if(front != NULL) {
  printf("insert test1\n");
      SET_PTR(NEXT_PTR(bp), front);
      SET_PTR(PREV_PTR(front), bp);
      SET_PTR(PREV_PTR(bp), NULL);
      segregated_lists[asize_idx] = bp;
  }else {
  printf("insert test2\n");
      SET_PTR(NEXT_PTR(bp), NULL);
      NEXT(bp) = NULL;
      SET_PTR(PREV_PTR(bp), NULL);
      PREV(bp) = NULL;
      segregated_lists[asize_idx] = bp;
      printf("bp = %p\n", bp);
  }
  /*
  if (front != NULL) {
    if (back != NULL) {
  printf("test2\n");
      SET_PTR(NEXT_PTR(bp), front);
      SET_PTR(PREV_PTR(front), bp);
      SET_PTR(PREV_PTR(bp), back);
      SET_PTR(NEXT_PTR(back), bp);
    } else {
  printf("test3\n");
      SET_PTR(NEXT_PTR(bp), front);
      SET_PTR(PREV_PTR(front),bp);
      SET_PTR(PREV_PTR(bp), NULL);
      segregated_lists[asize_idx] = bp;
    }
  } else {
    if (back != NULL) {
  printf("test4\n");
      SET_PTR(NEXT_PTR(bp), NULL);
      SET_PTR(PREV_PTR(bp), back);
      SET_PTR(NEXT_PTR(back), bp);
    } else {
  printf("test5\n");
      SET_PTR(NEXT_PTR(bp), NULL);
      SET_PTR(PREV_PTR(bp), NULL);
      segregated_lists[asize_idx] = bp;
    }
  }
  */
}

void print_lists() {
    printf("//////////////print lists start//////////////\n");
  for(int i=0; i<LIST; i++) {
    print_nodes(segregated_lists[i], i);
  }
    printf("//////////////print lists end//////////////\n");
}

void print_nodes(void *node, int idx) {
  void *temp;
  int cnt=0;
  printf("node[%d]", idx);
  printf("node %p", node);
  printf("print_nodes\n");
  for(temp = node; temp != NULL; temp = NEXT(temp)) {
    printf("test\n");
    printf("[%d]size:%d -> \t\t", cnt++, GET_SIZE(HDRP(temp)));
    printf("NEXT(temp): %p", NEXT(temp));
	}
  printf("\n");
}

void my_heapcheck()
{
	void *bp;
	for (bp = heap_listp; bp && GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
	//	myprintblock(bp);
		mycheckblock(bp);
	}
	print_lists();
}
void myprintblock(void *bp)
{
	int n =  GET_ALLOC(HDRP(bp)) ? 1 : 0;
	//todo: replace n into selected block index
	if(n==1)
     printf("%c: header(%p): [%d:%c,%d,%d] footer(%p): [%d:%c]\n",
				GET_ALLOC_C(HDRP(bp)), HDRP(bp), GET_SIZE(HDRP(bp)), GET_ALLOC_C(HDRP(bp)), n, GET_SIZE(HDRP(bp))-DSIZE, FTRP(bp), GET_SIZE(FTRP(bp)), GET_ALLOC_C(FTRP(bp)));
  if(n==0)
    printf("%c: header(%p): [%d:%c] footer(%p): [%d:%c]\n",
        GET_ALLOC_C(HDRP(bp)), bp, GET_SIZE(HDRP(bp)), GET_ALLOC_C(HDRP(bp)), FTRP(bp), GET_SIZE(FTRP(bp)), GET_ALLOC_C(FTRP(bp)));
	// a: header: [2056:a,1,2040] footer: [2056:a]
	// block size:2056, request_id :1, payload size: 2040


}
void mycheckblock(void *bp)
{
	return 0;
}

static void *find_fit(size_t asize)
{
  //print_lists();
  printf("find fit\n");
  int asize_idx = align_idx(asize);
  printf("asize_idx = %d\n", asize_idx);
  void *ptr;
  while(asize_idx < LIST) {
    ptr = segregated_lists[asize_idx];
      printf("asize_idx= %d\n", asize_idx);
      printf("ptr= %p\n", ptr);
    asize_idx++;
    while( ptr != NULL && GET_SIZE(HDRP(ptr)) < asize){
      printf("size = %d\n", GET_SIZE(HDRP(ptr)));
      ptr = NEXT(ptr);
    }
    if(ptr != NULL){
      printf("found ptr = %p\n", ptr);
      return ptr;
    }
  }
  printf("not found\n");
  return NULL; /* No fit */
}

static void place(void *bp, size_t asize)
{
  size_t csize = GET_SIZE(HDRP(bp));
  delete_node(bp);

  if((csize-asize) >= (2*DSIZE)) {
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize-asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
    insert_node(bp, csize-asize);
  }
  else {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
	}
}



/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	//init segregated lists
	init_segregated_lists();

	heap_listp = NULL; //initialize heap_listp
	/* Create the initial empty heap */
	if((heap_listp = mem_sbrk(4*WSIZE))==(void*)-1)
		return -1;
	PUT(heap_listp, 0);
	PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
	PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
	PUT(heap_listp + (3*WSIZE), PACK(0, 1));
	heap_listp += (2*WSIZE);
  printf("before extend\n");
	my_heapcheck();

	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;

  printf("after extend\n");
	my_heapcheck();
	return 0;
}

static void *extend_heap(size_t words) { // make large free block
  printf("extend_heap\n");
	void *bp;
	size_t size;
	/* Allocate an even number of words to maintain alignment */
	size = (words % 2 ) ? (words + 1) * WSIZE : words * WSIZE;
	if((long)(bp = mem_sbrk(size)) == -1)
		return NULL;

	/* Initialize free block header/footer and the epilogue header */
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
	insert_node(bp, size);

	/* Coalesce if the previous block was free */
	return coalesce(bp);
}

static void *coalesce(void *bp)
{
  printf("coalesce\n");
  printf("bp = %p\n", bp);
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if(prev_alloc && next_alloc) { // Case 1
  printf("coal test1\n");
    return bp;
  }
  else if (prev_alloc && !next_alloc) { // Case 2
  printf("coal test2\n");
    delete_node(bp);
    delete_node(NEXT_BLKP(bp));
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }
  else if (!prev_alloc && next_alloc) { // Case 3
  printf("coal test3\n");
    delete_node(bp);
    delete_node(PREV_BLKP(bp));
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }
  else { // Case 4
  printf("coal test4\n");
    delete_node(bp);
    delete_node(PREV_BLKP(bp));
    delete_node(NEXT_BLKP(bp));
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
      GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }
  printf("coal test0\n");
  insert_node(bp, size);

  return bp;
}


/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    printf("before malloc(%d)\n", size);

	size_t asize; /* Adjusted block size */
	size_t extendsize; /* Amount to extend heap if no fit */
	void *bp;

	/* Ignore spurious requests */
	if(size==0)
		return NULL;

	/*Adjust block size to include overhead and alignment reqs.*/
	if(size <= DSIZE)
		asize = 2*DSIZE;
	else
    asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

  /* Search th free list for a fit */
  if ((bp = find_fit(asize)) != NULL) {
    place(bp, asize);
    printf("after malloc(%d)\n", size);
    my_heapcheck();
    return bp;
  }
  /* No fit found. Get more memory and place the block */
  extendsize = MAX(asize, CHUNKSIZE);
  if(( bp=extend_heap(extendsize/WSIZE)) ==NULL){
    printf("after malloc(%d)\n", size);
    my_heapcheck();
    return NULL;
  }
  place(bp, asize);
  printf("after malloc(%d)\n", size);
  my_heapcheck();
  return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
  printf("before free(%p)\n", bp);
  size_t size = GET_SIZE(HDRP(bp));
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));

  insert_node(bp, size);
  print_lists();
  coalesce(bp);
  printf("after free(%p)\n", bp);
  my_heapcheck();
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
	void *oldptr = ptr;
	void *newptr;
	size_t copySize;

	newptr = mm_malloc(size);
	if (newptr == NULL)
		return NULL;
	copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
//	("copysize : %d \n" , copySize);
	if (size < copySize)
		copySize = size;
	memcpy(newptr, oldptr, copySize);
	mm_free(oldptr);
	return newptr;
}


