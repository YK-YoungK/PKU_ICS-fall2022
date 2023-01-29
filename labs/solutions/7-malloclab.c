/*
 * mm.c
 *
 *
 * My solution: 
 * (1) Use 2 BSTs to maintain the free blocks. Each node in BST can be
 * regarded as a linked list, holding all the free blocks with a same size. 
 * (2) The blocks with size 8 or 16 are kept specially, using two linked lists.
 * (3) The blocks with size 8 are always marked "allocated", since there is no
 * space to keep their footers.
 * 
 * The data structure of a free block:
 * (1) size == 8:
 * --------------------------------
 * | header:     (prev_alloc) (1) |
 * --------------------------------
 * |  next block (size 8) offset  |
 * --------------------------------
 * (2) size == 16:
 * --------------------------------
 * | header:     (prev_alloc) (0) |
 * --------------------------------
 * | next block (size 16) offset  |
 * --------------------------------
 * |           No use             |
 * --------------------------------
 * | footer:    (prev_alloc) (0)  |
 * --------------------------------
 * (3) size >= 24: In BST
  * ------------------------------------------------------------------------
 * | header:                                              (prev_alloc) (0) |
 * -------------------------------------------------------------------------
 * | left child node offset (only for node, inner block doesn't have this) |
 * -------------------------------------------------------------------------
 * |                next block (with the same size) offset                 |
 * -------------------------------------------------------------------------
 * | right child node offset (only for node, inner block doesn't have this)|
 * -------------------------------------------------------------------------
 * |   parent node offset (only for node, inner block doesn't have this)   |
 * -------------------------------------------------------------------------
 * |                                  No use                               |
 * -------------------------------------------------------------------------
 * | footer:                                             (prev_alloc) (0)  |
 * -------------------------------------------------------------------------
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
/* #define DEBUG */
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
#define WSIZE       4       /* Word and header/footer size (bytes) */ 
#define DSIZE       8       /* Double word size (bytes) */
#define INF         0x3f3f3f3f
#define MAX(x, y) ((x) > (y)? (x) : (y))  
#define MIN(x, y) ((x) < (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) 

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))            
#define PUT(p, val)  (*(unsigned int *)(p) = (val))    

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(bp) - DSIZE) 

/* Read the size and allocated fields from [block's top data address] p */
#define GET_SIZE(p)  (GET(HDRP(p)) & ~0x7)                   
#define GET_ALLOC(p) (GET(HDRP(p)) & 0x1)                    
#define GET_PREV_ALLOC(p)   (GET(HDRP(p)) & 0x2)

/* 
 * Given block ptr bp, compute address of next and previous blocks
 * Note(1): PREV_BLKP can be used only when the previous block is free
 * Note(2): 8 byte block will be organized specially, always marked allocated
 */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(bp)) 
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(bp - WSIZE)) 

/* Changes between offset and real address */
#define GET_OFFSET(addr, base)  ((char *)(addr) - (char *)(base))
#define GET_ADDRESS(offset, base)   ((char *)(base) + (size_t)(offset))

/* Given block in bins, compute address of its next or previous block offset */
#define GET_LIST_NEXT(bp)     (bp)

/*
 * Given block node pointer in BST, compute address of its left, right child 
 * offset and its child in the list with the same size
 */
#define GET_BST_LCHILD(bp)      (bp)
#define GET_BST_SAMESIZE(bp)    ((char *)bp + WSIZE)
#define GET_BST_RCHILD(bp)      ((char *)bp + WSIZE * 2)    
#define GET_BST_FATHER(bp)      ((char *)bp + WSIZE * 3)

/* Constants */
#define CHUNKSIZE (4096)  /* Extend heap by this amount (bytes) */  
#define MAXSMALL 2
#define BSTNUM 2
#define SMALLBSTSIZE (8192)

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */  
static char *bin_listp = 0;   /* Pointer to bins */
static char *bst_listp = 0;   /* Pointer to BST */
static size_t last_place = INF;

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t size, int mode);
static void add_bin(void *bp, size_t size);
static void *delete_bin(size_t size);
static void *find_delete_bin(void *bp, size_t size);
static void *find_treeroot(size_t size);
static void add_bst(void *root, void *node, size_t size);
static void *find_bst(size_t size);
static void *find_best_bst(size_t size, void *root);
static void *delete_bst(void *bp);
static void *coalecse(void *bp);
static void *place(void *bp, size_t need_size);
static void *find_fit(size_t size);


/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) 
{
    last_place = INF;
    /* Create the initial empty heap */
    dbg_printf("init called\n");
    if ((bin_listp = mem_sbrk(WSIZE * (MAXSMALL + BSTNUM + 2))) == (void *)-1) 
        return -1;
    bst_listp = bin_listp + (WSIZE * MAXSMALL);
    heap_listp = bin_listp + (WSIZE * (MAXSMALL + BSTNUM));

    /* Initialize pointers to the bins */
    for (int i = 0; i < MAXSMALL + BSTNUM; ++i)
        PUT(bin_listp + (i * WSIZE), INF);

    PUT(heap_listp, PACK(0, 3));             /* Prologue header */ 
    PUT(heap_listp + WSIZE, PACK(0, 3));     /* Epilogue header */
    heap_listp += (2 * WSIZE);

    return 0;
}

/*
 * malloc: Find a block and place it, or extend heap (and place it). 
 */
void *malloc (size_t size) 
{
    dbg_printf("malloc starts\n");
    /* mm_checkheap(__LINE__); */

    if (heap_listp == 0)
        mm_init();
    if (size == 0)
        return NULL;

    /* Get motivation from buddy system */
    if (size >= 112 && size < 128)
        size = 128;
    if (size >= 224 && size < 256)
        size = 256;
    if (size >= 448 && size < 512)
        size = 512;
    
    size_t malloc_size = ALIGN(size + WSIZE);
    dbg_printf("malloc called, require a block with size %ld\n", malloc_size);

    char *bp;
    if ((bp = find_fit(malloc_size)) != NULL)
    {
        /* Succeed in finding a block */
        dbg_printf("Succeed in finding a block from total list\n");
        dbg_printf("New block offset: %ld\n", GET_OFFSET(bp, heap_listp));
        return bp;
    }
    
    dbg_printf("Failed when finding a block from total list\n");
    size_t extend_size = MAX(malloc_size, CHUNKSIZE);
    
    if ((bp = extend_heap(extend_size, 0)) == NULL)
    {
        dbg_printf("Failed when extending heap\n");
        return NULL;
    }

    dbg_printf("extend heap with new block size: %lld\n", GET_SIZE(bp));

    /* Place the new block */
    if (GET_SIZE(bp) > malloc_size)
        return place(bp, malloc_size);

    /* Updating tag bits of current block and next block */
    char *tmp = NEXT_BLKP(bp);
    if (GET_ALLOC(tmp))
    {
        PUT(HDRP(tmp), PACK(GET_SIZE(tmp), 3));
    }
    else
    {
        PUT(HDRP(tmp), PACK(GET_SIZE(tmp), 2));
        PUT(FTRP(tmp), PACK(GET_SIZE(tmp), 2));
    }
    PUT(HDRP(bp), PACK(GET_SIZE(bp), GET_PREV_ALLOC(bp) + 1));
    dbg_printf("New block offset: %ld\n", GET_OFFSET(bp, heap_listp));
    return bp;
}

/*
 * free: Just free the block and add into free list after come coalecse
 */
void free (void *ptr) 
{
    dbg_printf("free starts\n");
    /* mm_checkheap(__LINE__); */

    if(!ptr)
        return;
    if (heap_listp == 0)
        mm_init();
    dbg_printf("free called, size: %ld, offset: %ld\n",
                 GET_SIZE(ptr), GET_OFFSET(ptr, heap_listp));

    /* Coalecse if the block itself will lie in BSTs instead of bins */
    /* Updating tag bits of current block and next block */
    /* Add the free block to BSTs or bins */

    char *next_block = NEXT_BLKP(ptr);
    if (GET_ALLOC(next_block) && GET_SIZE(ptr) > 8)
    {
        PUT(HDRP(next_block), PACK(GET_SIZE(next_block), 1));
    }
    else if (GET_SIZE(ptr) > 8)
    {
        PUT(HDRP(next_block), PACK(GET_SIZE(next_block), 0));
        PUT(FTRP(next_block), PACK(GET_SIZE(next_block), 0));
    }

    if (GET_SIZE(ptr) <= 8 * MAXSMALL)
    {
        if (GET_SIZE(ptr) > 8)
        {
            PUT(HDRP(ptr), PACK(GET_SIZE(ptr), GET_PREV_ALLOC(ptr)));
            PUT(FTRP(ptr), PACK(GET_SIZE(ptr), GET_PREV_ALLOC(ptr)));
        }
        add_bin(ptr, GET_SIZE(ptr));
        return;
    }
    
    PUT(HDRP(ptr), PACK(GET_SIZE(ptr), GET_PREV_ALLOC(ptr)));
    PUT(FTRP(ptr), PACK(GET_SIZE(ptr), GET_PREV_ALLOC(ptr)));
    ptr = coalecse(ptr);
    add_bst(find_treeroot(GET_SIZE(ptr)), ptr, GET_SIZE(ptr));
    return;
}

/*
 * realloc: Malloc a new block, copy data, and free the old one
 */
void *realloc(void *oldptr, size_t size) 
{
    dbg_printf("realloc starts\n");
    /* mm_checkheap(__LINE__); */

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0)
    {
        dbg_printf("realloc: just free\n");
        free(oldptr);
        return NULL;
    }
    /* If oldptr is NULL, then this is just malloc. */
    if(oldptr == NULL)
    {
        dbg_printf("realloc: just malloc\n");
        return malloc(size);
    }

    size_t old_size = GET_SIZE(oldptr) - WSIZE;

    dbg_printf("Non-trivial realloc called\n");
    
    /* Malloc new block and copy data */
    char *new_ptr = malloc(size);
    memcpy(new_ptr, oldptr, MIN(old_size, size));
    free(oldptr);
    return new_ptr;
}

/*
 * calloc: Just naive approach, malloc and memset
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc (size_t nmemb, size_t size) 
{
    dbg_printf("calloc called\n");
    size_t totalsize = nmemb * size;
    char* bp = malloc(totalsize);
    memset(bp, 0, totalsize);
    return bp;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) 
{
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) 
{
    return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * mm_checklist: Check the linked list, size, etc.
 */
static void mm_checklist(void *root, size_t size)
{
    if (GET(root) == INF)
        return;
    
    char *tmp = GET_ADDRESS(GET(root), heap_listp);
    while (GET(GET_LIST_NEXT(tmp)) != INF)
    {
        if (GET_SIZE(tmp) != size)
        {
            dbg_printf("size error, size: %d\n", size);
            exit(0);
        }
        if (tmp < (char *)mem_heap_lo() || tmp > (char *)mem_heap_hi())
        {
            dbg_printf("free pointer point error\n");
            exit(0);
        }
        tmp = GET_ADDRESS(GET(GET_LIST_NEXT(tmp)), heap_listp);
    }

    /* Check the last block */
    if (GET_SIZE(tmp) != size)
    {
        dbg_printf("size error, size: %d\n", size);
        exit(0);
    }
    if (tmp < (char *)mem_heap_lo() || tmp > (char *)mem_heap_hi())
    {
        dbg_printf("free pointer point error\n");
        exit(0);
    }
}
/*
 * mm_checkbst: check the BST in a recursive way
 * Check the node and its child node's consistence, and BST's order, etc.
 */
static void mm_checkbst(void *root)
{
    if (GET(root) == INF)
        return;
    root = GET_ADDRESS(GET(root), heap_listp);
    
    /* Check the linked list */
    unsigned int tmpoffset = GET_OFFSET(root, heap_listp);
    size_t tmpsize = GET_SIZE(root);
    while (tmpoffset != INF)
    {
        if (GET_SIZE(GET_ADDRESS(tmpoffset, heap_listp)) != tmpsize)
        {
            dbg_printf("BST: check size error\n");
            exit(0);
        }
        tmpoffset = GET(GET_BST_SAMESIZE(GET_ADDRESS(tmpoffset, heap_listp)));
    }
    
    unsigned int offset = GET_OFFSET(root, heap_listp);
    unsigned int parent_offset = GET(GET_BST_FATHER(root));
    unsigned int left_offset = GET(GET_BST_LCHILD(root));
    unsigned int right_offset = GET(GET_BST_RCHILD(root));
    if (parent_offset != INF)
    {
        char *parent_node = GET_ADDRESS(parent_offset, heap_listp);
        if (GET_SIZE(root) < GET_SIZE(parent_node) && 
            GET(GET_BST_LCHILD(parent_node)) != offset)
        {
            dbg_printf("the node isn't consistent with its parent node\n");
            exit(0);
        }
        if (GET_SIZE(root) > GET_SIZE(parent_node) && 
            GET(GET_BST_RCHILD(parent_node)) != offset)
        {
            dbg_printf("the node isn't consistent with its parent node\n");
            exit(0);
        }
    }
    if (left_offset != INF)
    {
        char *left_node = GET_ADDRESS(left_offset, heap_listp);
        if (offset != GET(GET_BST_FATHER(left_node)))
        {
            dbg_printf("left child node isn't consistent with this node\n");
            exit(0);
        }
        if (offset == GET(GET_BST_LCHILD(root)))
        {
            dbg_printf("left child node is still itself\n");
            exit(0);
        }

        /* Recursive check */
        mm_checkbst(left_node);
    }
    if (right_offset != INF)
    {
        char *right_node = GET_ADDRESS(right_offset, heap_listp);
        if (offset != GET(GET_BST_FATHER(right_node)))
        {
            dbg_printf("right child node isn't consistent with this node\n");
            exit(0);
        }   
        if (offset == GET(GET_BST_RCHILD(root)))
        {
            dbg_printf("right child node is still itself\n");
            exit(0);
        }

        /* Recursive check */
        mm_checkbst(right_node);
    }
}


/*
 * mm_checkheap: Check alignment, free bit consistency, etc.
 */
void mm_checkheap(int lineno) 
{
    dbg_printf("checkheap called\n");

    dbg_printf("check total heap\n");
    char *tmp = heap_listp, *tmp2;
    while (tmp < (char *)mem_heap_hi())
    {
        tmp2 = NEXT_BLKP(tmp);

        /* Check address alignment */
        if ((unsigned long long)tmp % 8 != 0)
        {
            dbg_printf("address alignment failed\n");
            exit(0);
        }
        
        /* Check free block's header and footer */
        if ((!GET_ALLOC(tmp)) && (*(HDRP(tmp))) != (*(tmp2 - 2 * WSIZE)))
        {
            dbg_printf("header and footer don't match\n");
            exit(0);
        }

        /* Check free bit consistency */
        if (GET_ALLOC(tmp) * 2 != GET_PREV_ALLOC(tmp2))
        {
            dbg_printf("check total list error, free bit not consistency, ");
            dbg_printf("size: %d, place: %d\n", 
                    GET_SIZE(tmp), GET_OFFSET(tmp, heap_listp));
            dbg_printf("tags: tmp %ld, tmp2 %ld\n", 
                    GET_ALLOC(tmp) * 2, GET_PREV_ALLOC(tmp2));
            exit(0);
        }
        tmp = tmp2;
    }

    dbg_printf("check total bst1\n");
    mm_checkbst(bst_listp);
    dbg_printf("check total bst2\n");
    mm_checkbst(bst_listp + WSIZE);

    dbg_printf("check total list1, size: 8\n");
    mm_checklist(bin_listp, 8);
    dbg_printf("check total list2, size: 16\n");
    mm_checklist(bin_listp + WSIZE, 16);
}


/* Function prototypes for internal helper routines */
/*
 * extend_heap: extend the heap, initialize the tag bit, and coalecse it 
 * if the mode == 0.
 */
static void *extend_heap(size_t size, int mode)
{
    /* Extend heap */
    char *bp;
    if ((long)(bp = mem_sbrk(size)) == -1)  
        return NULL;                            

    dbg_printf("extend heap succeed\n");

    /* Initialize free block header/footer and the epilogue header */
    unsigned int prev_alloc = GET_PREV_ALLOC(bp);
    PUT(HDRP(bp), PACK(size, prev_alloc));         /* Free block header */   
    PUT(FTRP(bp), PACK(size, prev_alloc));         /* Free block footer */  
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));       /* New epilogue header */ 
    
    if (mode)
        return bp;
    /* Coalesce if the previous block was free */
    return coalecse(bp);
}
/*
 * add_bin: add a free block of size 8 or 16 to the free list
 */
static void add_bin(void *bp, size_t size)
{
    char *bin_ptr = bin_listp + (size / 8 - 1) * WSIZE;

    PUT(GET_LIST_NEXT(bp), GET(bin_ptr));
    PUT(bin_ptr, GET_OFFSET(bp, heap_listp));
}
/*
 * delete_bin: delete a free block of size 8 or 16 and return its address
 */
static void *delete_bin(size_t size)
{
    dbg_printf("delete bin called, size: %ld\n", size);
    char *bin_ptr = bin_listp + (size / 8 - 1) * WSIZE;
    
    char *result = GET_ADDRESS(GET(bin_ptr), heap_listp);
    PUT(bin_ptr, GET(GET_LIST_NEXT(result)));
    return result;
}
/*
 * find_treeroot: if a free block should be put in BSTs, calculate which tree
 * should be lie in, return the pointer to the root.
 */
static void *find_treeroot(size_t size)
{
    /* All tree node's offset are saved near bin_listp */

    if (size <= SMALLBSTSIZE)
        return (bst_listp);
    else
        return (bst_listp + WSIZE);
        
    return NULL; /* To make gcc happy */
}
/*
 * add_bst: Find the right place for a free block in BST
 */
static void add_bst(void *root, void *node, size_t size)
{
    /* Recursive to find the correct place */
    /* 
     * Case 1: Arrive at leaves, add new leaf and update parent's child offset
     * and child's parent offset
     * Case 2: Find a place equal, add to the "linked list" with same size
     */
    dbg_printf("add_bst called, size: %ld, offset: %ld\n",
         size, GET_OFFSET(node, heap_listp));
    dbg_printf("tree root offset: %ld\n", GET(root));
    dbg_printf("tree root rightchild offset: %ld\n", 
        GET(GET_BST_RCHILD(root)));

    size_t now_size;
    unsigned int now_node_offset = GET(root);
    dbg_printf("%ld\n", GET_OFFSET(root, bin_listp));
    char *now_node;
    
    /* First node in BST */
    if (now_node_offset == INF)
    {
        dbg_printf("add first block in the BST\n");
        PUT(root, GET_OFFSET(node, heap_listp));
        PUT(GET_BST_LCHILD(node), INF);
        PUT(GET_BST_SAMESIZE(node), INF);
        PUT(GET_BST_RCHILD(node), INF);
        PUT(GET_BST_FATHER(node), INF);
        dbg_printf("first size: %ld\n", GET_SIZE(node));
        return;
    }

    unsigned int now_parent_offset = GET_OFFSET(root, bin_listp);
    while (now_node_offset != INF)
    {
        dbg_printf("now node offset: %ld\n", now_node_offset);
        now_node = GET_ADDRESS(now_node_offset, heap_listp);
        now_size = GET_SIZE(now_node);
        dbg_printf("now node size: %ld\n", now_size);
        if (size == now_size) /* Insert */
        {
            dbg_printf("add a same size block, size: %ld\n", size);
            PUT(GET_BST_SAMESIZE(node), GET(GET_BST_SAMESIZE(now_node)));
            PUT(GET_BST_SAMESIZE(now_node), GET_OFFSET(node, heap_listp));
            dbg_printf("check add: %ld %ld\n", GET(GET_BST_SAMESIZE(now_node)),
                        GET(GET_BST_SAMESIZE(node)));
            return;
        }
        else if (size < now_size) /* Left */
        {
            now_parent_offset = now_node_offset;
            now_node_offset = GET(GET_BST_LCHILD(now_node));
        }
        else /* Right */
        {
            now_parent_offset = now_node_offset;
            now_node_offset = GET(GET_BST_RCHILD(now_node));
        }
    }

    /* Arrive at leaf, add new node */
    if (size < now_size) /* Left */
    {
        PUT(GET_BST_LCHILD(now_node), GET_OFFSET(node, heap_listp));
        PUT(GET_BST_LCHILD(node), INF);
        PUT(GET_BST_SAMESIZE(node), INF);
        PUT(GET_BST_RCHILD(node), INF);
        PUT(GET_BST_FATHER(node), now_parent_offset);
        dbg_printf("add bst on a left leaf, size: %ld\n", size);
        dbg_printf("parent offset: %ld\n", GET(GET_BST_FATHER(node)));
        dbg_printf("parent size: %ld\n", 
            GET_SIZE(GET_ADDRESS(GET(GET_BST_FATHER(node)), heap_listp)));
    }
    else /* Right */
    {
        PUT(GET_BST_RCHILD(now_node), GET_OFFSET(node, heap_listp));
        PUT(GET_BST_LCHILD(node), INF);
        PUT(GET_BST_SAMESIZE(node), INF);
        PUT(GET_BST_RCHILD(node), INF);
        PUT(GET_BST_FATHER(node), now_parent_offset);
        dbg_printf("add bst on a right leaf, size: %ld\n", size);
        dbg_printf("parent offset: %ld\n", GET(GET_BST_FATHER(node)));
        dbg_printf("parent size: %ld\n", 
            GET_SIZE(GET_ADDRESS(GET(GET_BST_FATHER(node)), heap_listp)));
    }
    return;
}
/*
 * find_bst: Find a first block of given size in BSTs (guarantee its existence),
 * return the pointer.
 */
static void *find_bst(size_t size)
{
    /* Find the tree root */
    char *root = find_treeroot(size);
    dbg_printf("%ld\n", GET_OFFSET(root, bst_listp));

    /* Recursive to find the correct place */
    unsigned int now_node_offset = GET(root);
    dbg_printf("now node offset: %d\n", now_node_offset);
    char *now_node = GET_ADDRESS(now_node_offset, heap_listp);
    size_t now_size = GET_SIZE(now_node);
    dbg_printf("need: %ld, root: %ld\n", size, now_size);

    while (size != now_size)
    {
        dbg_printf("find size: %ld\n", now_size);
        if (size < now_size) /* Left */
        {
            now_node_offset = GET(GET_BST_LCHILD(now_node));
            dbg_printf("lnode offset: %ld\n", now_node_offset);
        }
        else /* Right */
        {
            now_node_offset = GET(GET_BST_RCHILD(now_node));
            dbg_printf("rnode offset: %ld\n", now_node_offset);
        }

        now_node = GET_ADDRESS(now_node_offset, heap_listp);
        now_size = GET_SIZE(now_node);
    }

    return now_node;
}
/*
 * find_best_bst: Find a block, with size nearest but greater than the given
 * value, return the pointer.
 */
static void *find_best_bst(size_t size, void *root)
{
    dbg_printf("find_best_bst called, need size: %ld\n", size);

    /* Recursive to find the correct place */
    unsigned int now_node_offset = GET(root);
    dbg_printf("%ld\n", now_node_offset);
    if (now_node_offset == INF)
        return NULL;
    
    char *now_node = GET_ADDRESS(now_node_offset, heap_listp);
    size_t now_size = GET_SIZE(now_node);
    dbg_printf("now size: %ld, now offset: %ld\n", 
        GET_SIZE(now_node), GET_OFFSET(now_node, heap_listp));

    char *result = NULL;
    
    while (now_node_offset != INF)
    {
        now_node = GET_ADDRESS(now_node_offset, heap_listp);
        now_size = GET_SIZE(now_node);

        if (size < now_size) /* Left */
        {
            result = now_node;
            now_node_offset = GET(GET_BST_LCHILD(now_node));
            dbg_printf("now node offset: %ld\n", now_node_offset);
        }
        else if (size > now_size)/* Right */
        {
            now_node_offset = GET(GET_BST_RCHILD(now_node));
            dbg_printf("now node offset: %ld\n", now_node_offset);
        }
        else
            return now_node;
    }

    if (result != NULL)
    {
        dbg_printf("find best bst, actual size: %ld\n", GET_SIZE(result));
    }
    return result;

}
/*
 * delete_bst: delete a free block in BST, with different cases
 */
static void *delete_bst(void *bp)
{
    /* Case 1: Leaf or have another block with same size, delete directly
     * Case 2: Not leaf, if the node has more than one block, delete directly
     * Or we need to find its largest left-child and move it up, then change 
     * fields of the parent and child of deleted node's, and fields of the
     * parent of victim node's
     * Note: Should consider if the deleted node was the root!
     */
    
    /* Get the size first */
    dbg_printf("delete bst called\n");

    size_t block_size = GET_SIZE(bp);
    dbg_printf("delete size: %ld, offset: %ld\n", 
                block_size, GET_OFFSET(bp, heap_listp));

    /* Find the first block with same size */
    char* delete_node = find_bst(block_size);
    dbg_printf("finish called find_bst, start offset: %ld, check size: %ld\n", 
            GET_OFFSET(delete_node, heap_listp), GET_SIZE(delete_node));

    /* The block is not the first one, thus don't need to change the BST */
    if ((char *)bp != delete_node)
    {
        dbg_printf("delete a same size node, size: %ld %ld\n", 
                    GET_SIZE(bp), GET_SIZE(delete_node));
        
        char *tmp1 = (char *)delete_node;
        char *tmp2 = GET_ADDRESS(GET(GET_BST_SAMESIZE(delete_node)), 
                                heap_listp);

        while (tmp2 != bp)
        {
            tmp1 = tmp2;
            dbg_printf("%ld\n", GET_BST_SAMESIZE(tmp1));
            tmp2 = GET_ADDRESS(GET(GET_BST_SAMESIZE(tmp1)), heap_listp);
        }

        dbg_printf("hey\n");

        PUT(GET_BST_SAMESIZE(tmp1), GET(GET_BST_SAMESIZE(tmp2)));
        return bp;
    }

    /* The first block, but has blocks with same size */
    if ((char *)bp == delete_node && GET(GET_BST_SAMESIZE(bp)) != INF)
    {
        dbg_printf("delete a same size node2\n");

        unsigned int left_offset = GET(GET_BST_LCHILD(bp));
        unsigned int right_offset = GET(GET_BST_RCHILD(bp));
        unsigned int parent_offset = GET(GET_BST_FATHER(bp));
        unsigned int bp_same_offset = GET(GET_BST_SAMESIZE(bp));
        dbg_printf("same size offset: %ld\n", bp_same_offset);
        char *bp_same = GET_ADDRESS(bp_same_offset, heap_listp);
        
        /* Use bp_same as the node, and update all the information */
        PUT(GET_BST_LCHILD(bp_same), left_offset);
        if (left_offset != INF)
        {
            PUT(GET_BST_FATHER(GET_ADDRESS(left_offset, heap_listp)), 
                                bp_same_offset);
        }

        PUT(GET_BST_RCHILD(bp_same), right_offset);
        if (right_offset != INF)
        {
            PUT(GET_BST_FATHER(GET_ADDRESS(right_offset, heap_listp)), 
                                bp_same_offset);
        }

        PUT(GET_BST_FATHER(bp_same), parent_offset);
        if (parent_offset != INF)
        {
            char *parent = GET_ADDRESS(parent_offset, heap_listp);
            if (block_size < GET_SIZE(parent)) /* Left */
            {
                PUT(GET_BST_LCHILD(parent), bp_same_offset);
            }
            else /* Right */
            {
                PUT(GET_BST_RCHILD(parent), bp_same_offset);
            }
        }
        else
        {
            PUT(find_treeroot(block_size), bp_same_offset);
        }

        return bp;
    }

    /* Else, we need to delete a leaf or a node in BST */
    dbg_printf("delete bst: delete a leaf or a node\n");
    
    /* Case 1: delete a leaf in BST */
    if ((GET(GET_BST_LCHILD(bp)) == INF) && (GET(GET_BST_RCHILD(bp)) == INF))
    {
        dbg_printf("delete bst: delete a leaf\n");
        unsigned int parent_offset = GET(GET_BST_FATHER(bp));
        if (parent_offset == INF) /* delete the root */
        {
            PUT(find_treeroot(GET_SIZE(bp)), INF);
            return bp;
        }   

        char *parent = GET_ADDRESS(parent_offset, heap_listp);
        if (GET_SIZE(bp) < GET_SIZE(parent)) /* Left */
        {
            PUT(GET_BST_LCHILD(parent), INF);
        }
        else /* Right */
        {
            PUT(GET_BST_RCHILD(parent), INF);
        }
        return bp;
    }
    /* Case 2: Not a leaf */
    else
    {
        dbg_printf("delete bst: delete a node\n");

        unsigned int parent_offset = GET(GET_BST_FATHER(bp));
        unsigned int victim_node_offset = GET(GET_BST_LCHILD(bp));
        dbg_printf("parent offset: %ld, delete offset: %ld\n", 
            parent_offset, GET_OFFSET(bp, heap_listp));
            
        if (victim_node_offset == INF) /* No left child */
        {
            dbg_printf("No left child\n");
            victim_node_offset = GET(GET_BST_RCHILD(bp));
            dbg_printf("parent offset: %ld, victim offset: %ld\n", 
                parent_offset, victim_node_offset);

            if (parent_offset == INF) /* Delete the root */
            {
                dbg_printf("Delete the root\n");
                PUT(find_treeroot(GET_SIZE(bp)), GET(GET_BST_RCHILD(bp)));
                char *right_node = GET_ADDRESS(GET(GET_BST_RCHILD(bp)), 
                    heap_listp);
                PUT(GET_BST_FATHER(right_node), INF);
                return bp;
            }

            char *parent = GET_ADDRESS(parent_offset, heap_listp);
            dbg_printf("parent offset: %ld, victim offset: %ld\n", 
                parent_offset, victim_node_offset);
            if (GET_SIZE(bp) < GET_SIZE(parent)) /* Left */
            {
                PUT(GET_BST_LCHILD(parent), victim_node_offset);
            }
            else /* Right */
            {
                PUT(GET_BST_RCHILD(parent), victim_node_offset);
            }
            PUT(GET_BST_FATHER(GET_ADDRESS(victim_node_offset, heap_listp)), 
                parent_offset);
                
            return bp;
        }

        /* Find the largest in the left subtree */
        char *victim_node;
        while (victim_node_offset != INF)
        {
            victim_node = GET_ADDRESS(victim_node_offset, heap_listp);
            victim_node_offset = GET(GET_BST_RCHILD(victim_node));
        }
        dbg_printf("the largest block in left subtree, size: %ld, place: %ld\n",
                    GET_SIZE(victim_node), GET_OFFSET(victim_node, heap_listp));
        victim_node_offset = GET_OFFSET(victim_node, heap_listp);

        unsigned int left_offset = GET(GET_BST_LCHILD(bp));
        unsigned int right_offset = GET(GET_BST_RCHILD(bp));
        
        /* Left tree's root has no right child */
        if (left_offset == GET_OFFSET(victim_node, heap_listp))
        {
            dbg_printf("Left tree's root has no right child\n");
            if (parent_offset == INF) /* Delete root */
            { 
                dbg_printf("Delete root\n");  
                PUT(find_treeroot(GET_SIZE(bp)), left_offset);
                PUT(GET_BST_FATHER(victim_node), INF);

                if (right_offset != INF)
                {
                    PUT(GET_BST_FATHER(GET_ADDRESS(right_offset, heap_listp)), 
                        left_offset);
                }
                PUT(GET_BST_RCHILD(victim_node), GET(GET_BST_RCHILD(bp)));
                return bp;
            }

            /* Update all the information */
            char *parent = GET_ADDRESS(parent_offset, heap_listp);
            if (GET_SIZE(bp) < GET_SIZE(parent)) /* Left */
            {
                PUT(GET_BST_LCHILD(parent), left_offset);
            }
            else
            {
                PUT(GET_BST_RCHILD(parent), left_offset);
            }
            PUT(GET_BST_FATHER(victim_node), parent_offset);

            if (right_offset != INF)
            {
                PUT(GET_BST_FATHER(GET_ADDRESS(right_offset, heap_listp)), 
                    left_offset);
            }
            PUT(GET_BST_RCHILD(victim_node), GET(GET_BST_RCHILD(bp)));

            return bp;
        }

        if (parent_offset == INF)
        {
            dbg_printf("Delete root\n");

            unsigned int victim_parent_offset = 
                GET(GET_BST_FATHER(victim_node));
            char *victim_parent = GET_ADDRESS(victim_parent_offset, heap_listp);
            
            /* Update all the information */
            unsigned int victim_node_child_offset = 
                GET(GET_BST_LCHILD(victim_node));
            PUT(GET_BST_RCHILD(victim_parent), victim_node_child_offset);
            if (victim_node_child_offset != INF)
            {
                char *victim_node_child = 
                    GET_ADDRESS(victim_node_child_offset, heap_listp);
                PUT(GET_BST_FATHER(victim_node_child), victim_parent_offset);
            }
            
            PUT(find_treeroot(GET_SIZE(bp)), victim_node_offset);
            PUT(GET_BST_FATHER(victim_node), INF);
            PUT(GET_BST_LCHILD(victim_node), GET(GET_BST_LCHILD(bp)));
            PUT(GET_BST_RCHILD(victim_node), GET(GET_BST_RCHILD(bp)));

            PUT(GET_BST_FATHER(GET_ADDRESS(left_offset, heap_listp)), 
                victim_node_offset);
            if (right_offset != INF)
            {
                PUT(GET_BST_FATHER(GET_ADDRESS(right_offset, heap_listp)), 
                    victim_node_offset);
            }

            return bp;
        }
        
        /* Update all the information */
        char *parent = GET_ADDRESS(parent_offset, heap_listp);

        unsigned int victim_parent_offset = GET(GET_BST_FATHER(victim_node));
        char *victim_parent = GET_ADDRESS(victim_parent_offset, heap_listp);
            
        unsigned int victim_node_child_offset = 
            GET(GET_BST_LCHILD(victim_node));
        PUT(GET_BST_RCHILD(victim_parent), victim_node_child_offset);
        if (victim_node_child_offset != INF)
        {
            char *victim_node_child = 
                GET_ADDRESS(victim_node_child_offset, heap_listp);
            PUT(GET_BST_FATHER(victim_node_child), victim_parent_offset);
        }

        PUT(GET_BST_FATHER(victim_node), parent_offset);

        if (GET_SIZE(bp) < GET_SIZE(parent)) /* Left */
        {
            PUT(GET_BST_LCHILD(parent), victim_node_offset);
            dbg_printf("put left, offset: %ld\n", victim_node_offset);
        }
        else /* Right */
        {
            PUT(GET_BST_RCHILD(parent), victim_node_offset);
            dbg_printf("put left, offset: %ld\n", victim_node_offset);
        }

        PUT(GET_BST_LCHILD(victim_node), GET(GET_BST_LCHILD(bp)));
        PUT(GET_BST_RCHILD(victim_node), GET(GET_BST_RCHILD(bp)));

        PUT(GET_BST_FATHER(GET_ADDRESS(left_offset, heap_listp)), 
            victim_node_offset);
        if (right_offset != INF)
        {
            PUT(GET_BST_FATHER(GET_ADDRESS(right_offset, heap_listp)), 
                victim_node_offset);
        }

        return bp;
    }
    
}
/*
 * coalecse: If the prev or next block is free and has size > 16, coalecse.
 */
static void *coalecse(void *bp)
{
    /* Check the status of previous and next block */
    /* Update the size of the block */

    dbg_printf("coalecse called\n");

    char *prev_block, *next_block;
    unsigned int totalsize = GET_SIZE(bp);

    next_block = NEXT_BLKP(bp);

    /* Check the next block */
    if ((!GET_ALLOC(next_block)) && GET_SIZE(next_block) > 8 * MAXSMALL)
    {
        dbg_printf("coalecse: the next block is free, offset: %ld\n",
                GET_OFFSET(next_block, heap_listp));
        delete_bst(next_block);
        totalsize += GET_SIZE(next_block);
    }

    dbg_printf("coalecse: check next block finished\n");

    /* Check the prev block */
    if (!GET_PREV_ALLOC(bp))
    {
        dbg_printf("coalecse: the previous block is free, offset: %ld\n",
                GET_OFFSET(PREV_BLKP(bp), heap_listp));
        prev_block = PREV_BLKP(bp);
        if (GET_SIZE(prev_block) > 8 * MAXSMALL)
        {
            delete_bst(prev_block);
            totalsize += GET_SIZE(prev_block);
            bp = prev_block;
        }

    }
    dbg_printf("coalecse: check previous block finished\n");
    
    unsigned int prev_alloc = GET_PREV_ALLOC(bp);
    PUT(HDRP(bp), PACK(totalsize, prev_alloc));
    PUT(FTRP(bp), PACK(totalsize, prev_alloc));
    
    dbg_printf("coalecse finished\n");

    return bp;
}
/*
 * place: cut the block into two, one of them with given size.
 * Note: use some heuristic rules to determine whether the first block
 * or the second block has the given size.
 */
static void *place(void *bp, size_t need_size)
{
    dbg_printf("place called\n");

    /* Check if the prev or next block is free */
    size_t size_first = need_size, size_next = GET_SIZE(bp) - need_size;

    if (size_next <= 8 * MAXSMALL) /* No coalecse */
    {
        /* Cut the block, get two block pointers */
        char *block_first = (char *)(bp);
        char *block_next = block_first + size_first;
        PUT(HDRP(block_first), PACK(size_first, GET_PREV_ALLOC(bp) + 1));
        PUT(FTRP(block_first), PACK(size_first, GET_PREV_ALLOC(bp) + 1));

        if (size_next > 8)
        {
            PUT(HDRP(block_next), PACK(size_next, 2));
            PUT(FTRP(block_next), PACK(size_next, 2));
            add_bin(block_next, size_next);
            char *tmp = NEXT_BLKP(block_next);
            if (GET_ALLOC(tmp))
            {
                PUT(HDRP(tmp), PACK(GET_SIZE(tmp), 1));
            }
            else
            {
                PUT(HDRP(tmp), PACK(GET_SIZE(tmp), 0));
                PUT(FTRP(tmp), PACK(GET_SIZE(tmp), 0));
            }
        }
        else
        {
            PUT(HDRP(block_next), PACK(size_next, 3));
            add_bin(block_next, size_next);
            char *tmp = NEXT_BLKP(block_next);
            if (GET_ALLOC(tmp))
            {
                PUT(HDRP(tmp), PACK(GET_SIZE(tmp), 3));
            }
            else
            {
                PUT(HDRP(tmp), PACK(GET_SIZE(tmp), 2));
                PUT(FTRP(tmp), PACK(GET_SIZE(tmp), 2));
            }
        }
        
        return block_first;
    }

    /* Change the place order, using some special rules */
    if (need_size < last_place && last_place <= 32)
    {
        dbg_printf("place: change the order\n");
        size_t size_tmp = size_first;
        size_first = size_next;
        size_next = size_tmp;
    
        char *block_first = (char *)(bp);
        char *block_next = block_first + size_first;
        PUT(HDRP(block_first), PACK(size_first, GET_PREV_ALLOC(bp)));
        PUT(FTRP(block_first), PACK(size_first, GET_PREV_ALLOC(bp)));
        PUT(HDRP(block_next), PACK(size_next, 1));
        
        /* Set the second block and its next block's tag bits */
        char *tmp = NEXT_BLKP(block_next);
        if (GET_ALLOC(tmp))
        {
            PUT(HDRP(tmp), PACK(GET_SIZE(tmp), 3));
        }
        else
        {
            PUT(HDRP(tmp), PACK(GET_SIZE(tmp), 2));
            PUT(FTRP(tmp), PACK(GET_SIZE(tmp), 2));
        }

        tmp = coalecse(block_first);
        dbg_printf("coalecse finish, size: %ld\n", GET_SIZE(tmp));
        add_bst(find_treeroot(GET_SIZE(tmp)), tmp, GET_SIZE(tmp));

        last_place = need_size;
        return block_next;
    }


    else
    {
        /* Cut the block, get two block pointers */
        char *block_first = (char *)(bp);
        char *block_next = block_first + size_first;
        PUT(HDRP(block_first), PACK(size_first, GET_PREV_ALLOC(bp) + 1));
        PUT(HDRP(block_next), PACK(size_next, 2));
        PUT(FTRP(block_next), PACK(size_next, 2));

        /* Set the second block and its next block's tag bits */
        char *tmp = NEXT_BLKP(block_next);
        if (GET_ALLOC(tmp))
        {
            PUT(HDRP(tmp), PACK(GET_SIZE(tmp), 1));
        }
        else
        {
            PUT(HDRP(tmp), PACK(GET_SIZE(tmp), 0));
            PUT(FTRP(tmp), PACK(GET_SIZE(tmp), 0));
        }

        tmp = coalecse(block_next);
        dbg_printf("%ld\n", GET_SIZE(tmp));
        add_bst(find_treeroot(GET_SIZE(tmp)), tmp, GET_SIZE(tmp));
        
        last_place = need_size;
        return block_first;
    }
}
/*
 * find_fit: Find a block with given size, and return the pointer
 */
static void *find_fit(size_t size)
{
    char* result;

    /* Case 1: Find in bins */
    if (size <= 8 * MAXSMALL)
    {
        char* bin_ptr = bin_listp + (size / 8 - 1) * WSIZE;
        while (bin_ptr + (BSTNUM + 2) * WSIZE < heap_listp)
        {
            if (GET(bin_ptr) != INF)
            {
                result = delete_bin(2 * GET_OFFSET(bin_ptr, bin_listp) + 8);
                if (GET_SIZE(result) > size)
                {
                    return place(result, size);
                }

                /* Update the bits */
                char *tmp = NEXT_BLKP(result);
                if (GET_ALLOC(tmp))
                {
                    PUT(HDRP(tmp), PACK(GET_SIZE(tmp), 3));
                }
                else
                {
                    PUT(HDRP(tmp), PACK(GET_SIZE(tmp), 2));
                    PUT(FTRP(tmp), PACK(GET_SIZE(tmp), 2));
                }
                PUT(HDRP(result), 
                    PACK(GET_SIZE(result), GET_PREV_ALLOC(result) + 1));
                return result;
            }
            bin_ptr += WSIZE;
        }
    }

    /* Case 2: Find in small BST */
    if (size <= SMALLBSTSIZE)
    {
        if ((result = find_best_bst(size, bin_listp + MAXSMALL * WSIZE)) 
                    != NULL)
        {
            delete_bst(result);

            if (GET_SIZE(result) > size)
            {
                return place(result, size);
            }

            /* Update the bits */
            char *tmp = NEXT_BLKP(result);
            if (GET_ALLOC(tmp))
            {
                PUT(HDRP(tmp), PACK(GET_SIZE(tmp), 3));
            }
            else
            {
                PUT(HDRP(tmp), PACK(GET_SIZE(tmp), 2));
                PUT(FTRP(tmp), PACK(GET_SIZE(tmp), 2));
            }
            PUT(HDRP(result), 
                PACK(GET_SIZE(result), GET_PREV_ALLOC(result) + 1));
            return result;
        }
    }

    /* Case 3: Find in large BST */
    if ((result = find_best_bst(size, bin_listp + (MAXSMALL + 1) * WSIZE)) 
                != NULL)
    {
        delete_bst(result);

        if (GET_SIZE(result) > size)
        {
            return place(result, size);
        }
        
        /* Update the bits */
        char *tmp = NEXT_BLKP(result);
        if (GET_ALLOC(tmp))
        {
            PUT(HDRP(tmp), PACK(GET_SIZE(tmp), 3));
        }
        else
        {
            PUT(HDRP(tmp), PACK(GET_SIZE(tmp), 2));
            PUT(FTRP(tmp), PACK(GET_SIZE(tmp), 2));
        }
        PUT(HDRP(result), PACK(GET_SIZE(result), GET_PREV_ALLOC(result) + 1));
        return result;
    }

    /* Fail */
    return NULL;
}