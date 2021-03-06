//
// malloc.c
// 25-Jan-99
//
// static void dumpheap(void);
// static void SignatureCheck ( PMEMBLOCK p, PSZ idText );
// static void HeapCheck ( PSZ idText );
// static PMEMBLOCK make_new_free(PMEMBLOCK pmbOldFree, unsigned uRequest);
// static unsigned _msize(void __near *pvBlock);
// static void __near * npvAllocateBlock( PMEMBLOCK pmb, USHORT uRequest, PMEMBLOCK pmbPrev );
// static void remove(PMEMBLOCK pmb);
// static void compact(void);
// void __near *malloc( unsigned uSize );
// void free(void __near *pvBlock);
// unsigned _memfree(void);
// void __near *realloc(void __near *pvBlock, unsigned usLength);
// unsigned HeapInit(unsigned uSize);

#include "cs40.h"

#define HEMP_DBG 1

// putting this in class BSS means won't blowout file size and extra HEAP_SIZE bytes!
// as it does if using the original  #pragma data_seg ("_HEMP","ENDDS");

#pragma data_seg ("_HEMP");     // yes, _HEMP

#define DEFAULT_HEAP 4096
#define HEAP_SIZE    8192 // max heap size
//#define SIGNATURE    0xBEEFDEAD  // "DEADBEEF" when view in hex in word order.
#define SIGNATURE  0x3477ADDE  // 'DEAD7734' in ascii view

typedef struct _MEMBLOCK {
   unsigned long ulSignature;
   unsigned uSize;
   struct _MEMBLOCK *pmbNext;
   char achBlock[1];
} MEMBLOCK, __near *PMEMBLOCK;

#define HDR_SIZE  ( sizeof(MEMBLOCK) - 1 )

// We won't leave a hole in memory any smaller than MIN_FragSize.
#define MIN_FragSize  4

//--- Global Variables used to manage the heap:
// (no reason to be extern globals, so making them static, 26-Jan-99)

static PMEMBLOCK pmbUsed=0;    // Points to head of list of allocated memory blocks.
                        // Newly allocated blocks are inserted into head of list.
static PMEMBLOCK pmbFree;      // Points to head of list of available memory blocks.
static unsigned uMemFree;      // N bytes available for allocation.

static USHORT fMemoryDoc = 0;
static USHORT nAlloc = 0;           // Current number of allocated blocks.
static USHORT cAlloc = 0;           // Total memory in allocated blocks incl. headers.
static USHORT nAllocCalls = 0;      // Cumulative total, number of malloc() calls.
static USHORT nFree  = 0;           // Current number of free blocks.
static USHORT cFree  = 0;           // Total memory in free blocks incl. headers.
static USHORT nFreeCalls  = 0;      // Cumulative total, number of free() calls.
static USHORT nCompact = 0;         // Num of times we've joined two adjacent free blocks into a single larger block
static USHORT cTotalHeap = HEAP_SIZE;

static char __near acHeap[HEAP_SIZE];
                        // The heap.  NOTE:  This must be the last data definition in the HEAP segment,
                        // although not done currently, we are planning to size the heap down at INIT
                        // time.  If this is done, any vbls in the HEAP segment that
                        // appear after the HEAP would become unaddressable.

#ifdef HEMP_DBG

//--- Heap methods:

//static void dumpheap(void)
void dumpheap(void)
{
   int i;
   PMEMBLOCK pmb;
   unsigned u=0;

   pmb=pmbUsed;
if (gCLflags & FLAGS_CL_DDPRINT)    ddprintf("~HEAP: Heap Dump - Used blocks\n");
   for (i=0; i<32; i++) {
      if (!pmb) break;
if (gCLflags & FLAGS_CL_DDPRINT)       ddprintf("~  pmb=%p, btyes=%u\n",(void __far *) pmb, pmb->uSize);
      u+=pmb->uSize;
      pmb=pmb->pmbNext;
   }
if (gCLflags & FLAGS_CL_DDPRINT)    ddprintf("~  Total used = %u\n",u);

   u=0;
   pmb=pmbFree;
if (gCLflags & FLAGS_CL_DDPRINT)    ddprintf("~HEAP: Heap Dump - Free blocks\n");
   for (i=0; i<32; i++) {
      if (!pmb) break;
if (gCLflags & FLAGS_CL_DDPRINT)       ddprintf("~  pmb=%p, bytes=%u\n",(void __far *) pmb, pmb->uSize);
      u+=pmb->uSize;
      pmb=pmb->pmbNext;
   }
if (gCLflags & FLAGS_CL_DDPRINT)    ddprintf("~  Total free = %u\n",u);

if (gCLflags & FLAGS_CL_DDPRINT)    ddprintf("~ alloc:currblks=%u, calls=%u;  free:currblks=%u, calls=%u\n",
              nAlloc,nAllocCalls,nFree,nFreeCalls);
}

#endif


static void SignatureCheck ( PMEMBLOCK p, PSZ idText )
{

   bool bGoodPointer;

   // Set bGoodPointer to indicate whether or not 'p' points within heap.
   bGoodPointer = (((USHORT) p) >= ((USHORT) acHeap))
                  && (((USHORT) p) <= (((USHORT) acHeap) + HEAP_SIZE)) ;
                              //### heap might have less than HEAP_SIZE bytes

   // Check for correct signature.
   if (bGoodPointer)
      bGoodPointer = (p->ulSignature == SIGNATURE);

   if (! bGoodPointer)
   {
#ifdef HEMP_DBG
if (gCLflags & FLAGS_CL_DDPRINT)       ddprintf( "~Heap pointer out of range, or signature exception: %p %s\n", p, idText );
#endif
      int3();
   }
 return;
 idText;
}

static void HeapCheck ( PSZ idText )
{
   PMEMBLOCK p;
   for ( nAlloc = 0, cAlloc = 0, p = pmbUsed; p; p = p->pmbNext ) {
      ++nAlloc;
      cAlloc += p->uSize + HDR_SIZE;
      //SignatureCheck( p,(PSZ) "HeapCheck() Used list" );
      //SignatureCheck( p,(PSZ)0);
      SignatureCheck( p,(PSZ)"HCUL");
   }
   for ( nFree = 0, cFree = 0, p = pmbFree; p; p = p->pmbNext ) {
      ++nFree;
      cFree += p->uSize + HDR_SIZE;
      //SignatureCheck( p,(PSZ) "HeapCheck() Free list" );
      //SignatureCheck( p,(PSZ)0);
      SignatureCheck( p,(PSZ)"HCFL");
   }
   if (fMemoryDoc & 1) {
      if (cAlloc + cFree != cTotalHeap) {
#ifdef HEMP_DBG
if (gCLflags & FLAGS_CL_DDPRINT)          ddprintf( "~Heap Alloc + Free != Total\n" );
         dumpheap();
#endif
         int3();
      }
   }
 return;
 idText;
}



// Threshold for creating a new free block.
#define  MIN_Leftover  (HDR_SIZE + MIN_FragSize)

/* make_new_free()
   Formats the back part of an existing free MEMBLOCK as a new, smaller
   "Free" MEMBLOCK.  Doesn't update Global vbls (caller responsibility).
      IN:   pmbOldFree - pointer to existing memory block.
            uSize - offset, which won't be included in new allocation.
                    it's assumed HDR is not included in uSize.
      OUT:  pointer to new free MEMBLOCK, is  a new block of free memory,
            is created at pmbOldFree + uRequest.  The memory block header
            in both the new block and the old block are properly initialized
            on return, but we don't update the Free list or Allocated list.
      OUT:  NULL if the new free memory block is smaller than the
            framentation threshold.

The fragmentation consideration comes into play when we find a block that's
big enough to hold the requested memory, but not big enough for the leftover
part to be useful as another free block.

    _______________________free block_______________________________________
   |  free    |                                                             |
   |  block   |       space available                                       |
   |  header  |          (pmbFree->uSize bytes long)                        |
   |(HDR_SIZE |                                                             |
   |__bytes)__|_____________________________________________________________|

   <-- HDR --> <------------------------- l -------------------------------->

Must either be allocated in total, or must become:

    _______________________used block_______________________________________
   |  used    |                                     |  free    |            |
   |  block   |  space allocated                    |  block   | space      |
   |  header  |  == uSize (following DWORD align't) |  header  | available  |
   |(HDR_SIZE |     (pmbUsed->uSize bytes long)     |(HDR_SIZE |            |
   |__bytes)__|_____________________________________|__bytes)__|____________|

   <-- HDR --> <-------------- n ------------------><-- HDR --> <--- m ----->

To be split into an allocated and a new, smaller free block, 'm' must be at
least MIN_FragSize bytes long.

Everything should remain 4 byte aligned since the HDR_SIZE, MIN_FragSize,
and the space allocated (after we munge it) are all multiples of 4.

This means that in order for a free block to be chosen, one of the following
equation must hold

  Case 1:  n  <=  l  < (n + HDR + MIN_FragSize)
  Here, we'll allocate the entire block.  This makes the block somewhat
  larger than the request, but prevents some degree of fragmentation.

  Case 2:  (n + HDR + MIN_FragSize) <= l
  We split the block into an allocated part to satisfy the allocation request,
  and a free block which can be allocated in a subsequent request.
*/

static PMEMBLOCK make_new_free(PMEMBLOCK pmbOldFree, unsigned uRequest)
{
   PMEMBLOCK pmbNewFree;     // If we create a new free block, it goes here.

   // Which of the two cases (as described in function header) have we got?
   // We know we're big enough to satisfy request, is there enough leftover
   // for a new free block?

    if ((uRequest + MIN_Leftover) > pmbOldFree->uSize) {
       // Not enough leftover, allocate the entire free block.  Don't
       // change pmbOldFree->uSize.
       pmbNewFree = 0;
    }
    else {
       // We've got enough leftover to make a new free block.
       pmbNewFree = (PMEMBLOCK) ((char __near *) pmbOldFree + HDR_SIZE + uRequest );
       pmbNewFree->ulSignature = SIGNATURE;
       pmbNewFree->uSize = pmbOldFree->uSize - (uRequest + HDR_SIZE);
       pmbNewFree->pmbNext = pmbOldFree->pmbNext;

       // Update the size of the free block that was passed in.
       pmbOldFree->uSize -= (pmbNewFree->uSize + HDR_SIZE);
    }

   return pmbNewFree;
}


/**@internal _msize
 */
static unsigned _msize(void __near *pvBlock)
{
   PMEMBLOCK pmb;

   if (!pvBlock)
      return 0;

   pmb = (PMEMBLOCK) ((char __near *) pvBlock - HDR_SIZE);

   return pmb->uSize;
}


/**@internal pmbAllocateBlock
 *  Update all data structures for allocation of one memory block.  It's
 *  assumed, on entry, that the block is large enough for the requested
 *  allocation.
 * @param PMEMBLOCK pmb - the current memory block on the Free list to
 *  allocate.
 * @param USHORT uRequest - size of the memory request to fill, not counting
 *  memory block header.
 * @param PMEMBLOCK pmbPrev - the memory block which precedes the block being
 *  allocated in the Free list.  NULL if no predecessor (ie, pmb is pointed to
 *  by ::pmbFree).
 * @return PMEMBLOCK - pointer to the data part of the allocated memory block,
 *  suitable for return to malloc() caller.
 */
static void __near *
npvAllocateBlock( PMEMBLOCK pmb, USHORT uRequest, PMEMBLOCK pmbPrev )
{
   //pmb points to the selected block.
   //pmbPrev points to the block prior to the selected block, NULL if pmbFree.
   PMEMBLOCK pmbOldNext;            // Original free block that follows the block being allocated.
   PMEMBLOCK pmbNewFree;            // Leftover free memory from the allocated block.

   // Split this block into an allocated + free part if it's big enough.
   pmbNewFree = make_new_free( pmb, uRequest );

   // Update global memory counter.
   uMemFree -= (pmb->uSize + HDR_SIZE);

   // Add this block into the front of the Allocated list.
   pmbOldNext = pmb->pmbNext;
   pmb->pmbNext = pmbUsed;
   pmbUsed = pmb;

   // Remove the new allocation from the Free list.
   if (pmbNewFree) {                // If we split off a new free block
      pmbNewFree->pmbNext = pmbOldNext;
      if (pmbPrev)                  // If we're not at head of Free list
         pmbPrev->pmbNext = pmbNewFree;
      else
         pmbFree = pmbNewFree;
   }
   else {                           // We allocated the entire free block.
      if (pmbPrev)                  // If we're not at head of Free list
         pmbPrev->pmbNext = pmbOldNext;
      else
         pmbFree = pmbOldNext;
   }

   return (void __near *) pmb->achBlock;
}


/* malloc()
This function searches the list of free blocks until it finds one big enough
to hold the amount of memory requested, which is passed in uSize.  The uSize
request is sized up for:
  - a memory block header
  - four byte alignment
  - minimum fragmentation

See helper function make_new_free() for discussion of fragmentation handling.
*/

void __near *malloc( unsigned uSize )
{
   USHORT uRequest;                    // Request size after alignment.
   PMEMBLOCK pmb, pmbPrev;             // Use to walk free lists.
   void __near *npvReturn = 0;         // Return value.

   ++nAllocCalls;                      // Diagnostics.
   //HeapCheck((PSZ) "malloc() entry" );
   //HeapCheck((PSZ)0);
   HeapCheck((PSZ)"MEXX");

   if (! uSize) return 0;

   uRequest = (uSize + 3) & -4;        // Force DWORD alignment.

   if (pmbFree->uSize >= uRequest)
      npvReturn = npvAllocateBlock( pmbFree, uRequest, NULL );
   else {
      pmbPrev = pmbFree;
      for ( pmb=pmbFree->pmbNext; pmb; pmbPrev=pmb, pmb=pmb->pmbNext)
         if (pmb->uSize >= uRequest) {
            npvReturn = npvAllocateBlock( pmb, uRequest, pmbPrev );
            break;
         }
   }

   if (npvReturn)
      //SignatureCheck( (PMEMBLOCK) (((PUCHAR) npvReturn) - HDR_SIZE), (PSZ) "malloc() exit, allocated block" );
      //SignatureCheck( (PMEMBLOCK) (((PUCHAR) npvReturn) - HDR_SIZE), (PSZ)0);
      SignatureCheck( (PMEMBLOCK) (((PUCHAR) npvReturn) - HDR_SIZE), (PSZ)"MEAB");
   else {
      // Out of Memory !!!
//    pError->vLog( HEAP_Err_NoMemory, (USHORT) uSize );

#ifdef HEMP_DBG
if (gCLflags & FLAGS_CL_DDPRINT)       ddprintf("~Out of memory, dumpheap()...\n");
      dumpheap();
#endif
      int3();
   }

   //HeapCheck((PSZ) "malloc() exit" );
   //HeapCheck((PSZ)0);
   HeapCheck((PSZ)"MXXX");
   return npvReturn;
}

/* void compact(void)
This function compacts the free blocks together.  This function is a
companion to free(), and thus the algorithm is tailored to how free()
works.  Change the algorithm in free(), and you'll have to change this
function too.

When free() frees a block, it sets the head of the free list, pmbFree, to
point to it.  Then the newly freed block points to whatever the old pmbFree
pointed to.  In other words, each new free block is added to the head of
the free list.

If compact() is always called after a block is freed, then it can be
guaranteed that the free list is always compacted (i.e. you won't find
two adjacent free blocks anywhere in the heap) _before_ each call to free().
Therefore, the newly freed block can be located in only one of the
following positions:
1. Not adjacent to any other free blocks (compacting not necessary)
2. Physically before another free block
3. Physically after another free block
4. Between two free blocks (i.e. the block occupies the entire space
   between two free blocks)

Since the newly freed block is the first in the free list, compact()
starts with the second block in the list (i.e. pmbFree->pmbNext).
Each free block is then compared with the newly freed block for
adjacency.  If a given block is located before the new block, then it
can't possibly be also located after, and vice versa.  Hence, the
"else if" statement in the middle of the loop.

Also, the newly freed block can only be adjacent to at most two other
blocks.  Therefore, the operation of combining two adjacent free blocks can
only happen at most twice.  The variable nFreed counts the number of times
two blocks are combined.  The function exits if nFreed reaches two.  nFreed
is initially 0.

Helper macro after() takes a PMEMBLOCK (call it pmb) as a parameter,
and calculates where an adjacent free block would exist if it were
physically located after pmb.

Helper function remove() removes an element from the free list.
*/

#define after(pmb) ((PMEMBLOCK) ((char __near *) pmb + pmb->uSize + HDR_SIZE))

static void remove(PMEMBLOCK pmb)
{
   PMEMBLOCK pmbPrev;

   if (pmb == pmbFree) {
      pmbFree = pmbFree->pmbNext;
      return;
   }

   for (pmbPrev=pmbFree; pmbPrev; pmbPrev=pmbPrev->pmbNext)
      if (pmbPrev->pmbNext == pmb) {
         pmbPrev->pmbNext = pmb->pmbNext;
         return;
      }
}

static void compact(void)
{
   PMEMBLOCK pmb;
   int iFreed = 0;

   for (pmb=pmbFree->pmbNext; pmb; pmb=pmb->pmbNext) {
      if (pmb == pmb->pmbNext) {
#ifdef HEMP_DBG
if (gCLflags & FLAGS_CL_DDPRINT)          ddprintf("~HEAP: heap loop, %p points to itself\n", (void __far *) pmb);
#endif
         int3();
      }

      if (after(pmb)  == pmbFree) {
#ifdef HEMP_DBG
if (gCLflags & FLAGS_CL_DDPRINT)  ddprintf("~HEAP: compact found pointer %p (size=%u) before pmbFree %p\n", (void __far *) pmb, pmb->uSize, (void __far *) pmbFree);
#endif
         pmb->uSize += HDR_SIZE + pmbFree->uSize;
         remove(pmbFree);
         if (++iFreed == 2) goto exit;
      } else if (after(pmbFree) == pmb) {
#ifdef HEMP_DBG
if (gCLflags & FLAGS_CL_DDPRINT)  ddprintf("~HEAP: compact found pointer %p (size=%u) after pmbFree %p\n", (void __far *) pmb, pmb->uSize, (void __far *) pmbFree);
#endif
         pmbFree->uSize += HDR_SIZE + pmb->uSize;
         remove(pmb);
         if (++iFreed == 2) goto exit;
      }
   }

exit:
   nCompact += iFreed;
}

void free(void __near *pvBlock)
{
   PMEMBLOCK pmb,pmbPrev,pmbBlock;
   int fSentinel;

   ++nFreeCalls;
   if (!pvBlock) return;     // support freeing of NULL

   pmbBlock=(PMEMBLOCK) ((char __near *) pvBlock - HDR_SIZE);

   //SignatureCheck( pmbBlock,(PSZ) "free() entry, Block to be freed" );
   //SignatureCheck( pmbBlock,(PSZ)0);
   SignatureCheck( pmbBlock,(PSZ)"FEBF");
   //HeapCheck((PSZ) "free() entry" );
   //HeapCheck((PSZ)0);
   HeapCheck((PSZ)"FEXX");

   uMemFree += pmbBlock->uSize + HDR_SIZE;

   if (pmbBlock == pmbUsed) {       // are we freeing the first block?
      pmbUsed = pmbUsed->pmbNext;   // the 2nd block is now the 1st
      pmbBlock->pmbNext = pmbFree;  // this block is now free, so it points to 1st free block
      pmbFree = pmbBlock;           // this is now the 1st free block
      compact();
      goto exit;
   }

   pmbPrev=pmbUsed;
   fSentinel = FALSE;
   for (pmb=pmbUsed->pmbNext; pmb; pmbPrev=pmb, pmb=pmb->pmbNext)
      if (pmb == pmbBlock) {
         if (fSentinel) {
#ifdef HEMP_DBG
if (gCLflags & FLAGS_CL_DDPRINT)             ddprintf("~HEAP: free sentinel triggered, pmb=%p\n", (void __far *) pmb);
#endif
            int3();
         }
         pmbPrev->pmbNext = pmb->pmbNext;   // delete this block from the chain
         pmbBlock->pmbNext = pmbFree;
         pmbFree = pmbBlock;
         compact();
         fSentinel = TRUE;
      }

exit: //--- Check things are still intact.
   //HeapCheck((PSZ) "free() exit" );
   //HeapCheck((PSZ)0);
   HeapCheck((PSZ)"FXXX");
}

unsigned _memfree(void)
{
   return uMemFree;
}

void __near *realloc(void __near *pvBlock, unsigned usLength)
{
   void __near *pv;

   if (!pvBlock)                 // if ptr is null, alloc block
      return malloc(usLength);

   if (!usLength) {              // if length is 0, free block
      free(pvBlock);
      return 0;
   }

   pv=malloc(usLength);          // attempt to allocate the new block
   if (!pv)                      // can't do it?
      return 0;                  // just fail.  Version 2 will try harder

   MEMCPY(pv, pvBlock, min( _msize(pvBlock), usLength));
   free(pvBlock);
   return pv;
}

#pragma code_seg ("_INITTEXT");

extern endHeap, endData;

// probably only want to use HEAP_SIZE for size since it's hard-coded a lot above
// this uSize arg is probably for a future thing

unsigned HeapInit(unsigned uSize) {

 unsigned max_heap=((unsigned)&endHeap - (unsigned)&endData) & ~0xFFF; // align to K

// if (!uSize) uSize = DEFAULT_HEAP;
 if (!uSize) uSize = HEAP_SIZE;
 if (uSize > max_heap) uSize = max_heap;

 pmbFree=(PMEMBLOCK) acHeap;
 pmbFree->uSize = uSize - HDR_SIZE;
 pmbFree->ulSignature = SIGNATURE;
 pmbFree->pmbNext = 0;
 uMemFree = pmbFree->uSize;

 return pmbFree->uSize;
}


