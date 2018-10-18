/* mem.c A template
 * My Name is Heath Gerrald
 * My User ID is hgerral, C43308731
 * Lab4: Dynamic Memory Allocation
 * ECE 2230, Fall 2018
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include "mem.h"

// Global variables required in mem.c only
static chunk_t Dummy = {&Dummy, 0};
static chunk_t * Rover = &Dummy;
static int NumSbrkCalls;
static int NumPages;


// private function prototypes
void mem_validate(void);

/* function to request 1 or more pages from the operating system.
 *
 * new_bytes must be the number of bytes that are being requested from
 *           the OS with the sbrk command.  It must be an integer
 *           multiple of the PAGESIZE
 *
 * returns a pointer to the new memory location.  If the request for
 * new memory fails this function simply returns NULL, and assumes some
 * calling function will handle the error condition.  Since the error
 * condition is catastrophic, nothing can be done but to terminate
 * the program.
 */
chunk_t *morecore(int new_bytes)
{
    char *cp;
    chunk_t *new_p;

    // preconditions
    assert(new_bytes % PAGESIZE == 0 && new_bytes > 0);
    assert(PAGESIZE % sizeof(chunk_t) == 0);
    cp = sbrk(new_bytes);
    if (cp == (char *) -1)  /* no space available */
        return NULL;
    new_p = (chunk_t *) cp;
    // You should add some code to count the number of calls
    // to sbrk, and the number of pages that have been requested
    NumSbrkCalls++;
    NumPages += new_bytes/PAGESIZE;
    // Ex: NumSbrkCalls++; NumPages += new_bytes/PAGESIZE;
    return new_p;
}

/* deallocates the space pointed to by return_ptr; it does nothing if
 * return_ptr is NULL.
 *
 * This function assumes that the Rover pointer has already been
 * initialized and points to some memory block in the free list.
 */
void Mem_free(void *return_ptr)
{
    // precondition
    assert(Rover != NULL && Rover->next != NULL);

    if (Coalescing == FALSE && return_ptr != NULL)
    {
     // place the new block at Rover
      chunk_t *p = (chunk_t*) return_ptr;
      p--;
      p->next = Rover->next;
      Rover->next = p;
    }

    else if (Coalescing == TRUE && return_ptr != NULL)
    {
      chunk_t *p = (chunk_t*) return_ptr;
      chunk_t *q, *highest_address;
      p--;
      int was_combined = 0;

     // get Rover right in front of p
      while (Rover < Rover->next) {Rover = Rover->next; } // gets to end of free list
      highest_address = Rover;
      Rover = Rover->next;
     // Rover should now point to lowest block

      while (Rover->next < p && Rover < highest_address) {Rover = Rover->next; }
    // Rover now points to right before p

     // check to see if p can be combined with block to its right
      if (p + p->size == Rover->next)
      {
        q = Rover->next;
        p->next = q->next;
        q->next = NULL;
        p->size += q->size;
        Rover->next = p;
        was_combined = 1;
      }
     // check to see if p can be combined with the block to its left
      if (Rover + Rover->size == p)
      {
        Rover->size += p->size;
        if (p->next != NULL) { Rover->next = p->next; }
        p->next = NULL;
        was_combined = 1;
      }

      if (was_combined == 0)
      {
        p->next = Rover->next;
        Rover->next = p;
      }

    }

    // obviously the next line is WRONG!!!!  You must fix it.
    //free(return_ptr);
}

/* returns a pointer to space for an object of size nbytes, or NULL if the
 * request cannot be satisfied.  The memory is uninitialized.
 *
 * This function assumes that there is a Rover pointer that points to
 * some item in the free list.  The first time the function is called,
 * Rover is null, and must be initialized with a dummy block whose size
 * is one, but set the size field to zero so this block can never be
 * removed from the list.  After the first call, the Rover can never be null
 * again.
 */
void *Mem_alloc(const int nbytes)
{
    // precondition
    assert(nbytes > 0);
    assert(Rover != NULL && Rover->next != NULL);

    chunk_t *p, *q;
    chunk_t *smallest_chunk = NULL;
    chunk_t *smallest_prev = NULL;
    int smallest_num = 0;
    chunk_t *search_start = Rover;
    chunk_t *Rover_prev = Rover;

 // find our previous Rover pointer
    Rover = Rover->next;
    while (Rover != search_start) {
      Rover_prev = Rover_prev->next;
      Rover = Rover->next;
    }
 // Rover should equal its original position

    int found_space = 0;
    int nunits = nbytes / sizeof(chunk_t) + 1;
    if (nbytes % sizeof(chunk_t) != 0) { nunits++; }

    int page_count = nunits / PAGESIZE;
    if (nunits % PAGESIZE != 0) { page_count++; }
    int new_bytes = PAGESIZE * page_count;

  // Search the list for a block with enough memory
  // Start search from Rover
    if (SearchPolicy == FIRST_FIT)
    {
      do {
        if (Rover->size >= nunits) { found_space = 1; }
        else { Rover_prev = Rover; Rover = Rover->next; }
      } while(Rover != search_start && found_space == 0);
    }

    else if (SearchPolicy == BEST_FIT){
      smallest_num = PAGESIZE / sizeof(chunk_t) + 1; // this is higher than all chunks can be
    do {
        if (Rover->size < smallest_num && Rover->size > 0 && Rover->size >= nunits){
           smallest_chunk = Rover;
           smallest_num = Rover->size;
           smallest_prev = Rover_prev;
         }
        else {
          Rover_prev = Rover;
          Rover = Rover->next;
        }
      } while (Rover != search_start);
    }

    if (found_space == 1 && SearchPolicy == FIRST_FIT) { p = Rover; }
    else if (smallest_chunk && SearchPolicy == BEST_FIT) {
      p = smallest_chunk;
      Rover = smallest_chunk;
      Rover_prev = smallest_prev;
    }
    else
    {
      p = morecore(new_bytes);
      p->next = Rover->next;
      Rover->next = p;
      p->size = new_bytes / sizeof(chunk_t);
    }

    q = p + p->size - nunits;
    q->size = nunits;
    if (p->size == nunits) { // this is the last memory on this page
        Rover_prev->next = Rover->next;
        Rover = Rover_prev->next; // Rover must point to next block in list when allocating
    }
    q->next = NULL;
    p->size -= nunits;
  // p == q if we used all the space in this page, in this case subtracting
  // the size from p also subtracts from q. We need q to keep its size
    if (p == q) { q->size += nunits; }
    return q + 1;

    // here are possible post-conditions, depending on your design
    //
    // assume p is a pointer to memory block that will be given to the user
    // and q is the address given to the user
    // assert(p + 1 == q);
    // the minus one in the next two tests accounts for the header
    // assert((p->size - 1)*sizeof(chunk_t) >= nbytes);
    // assert((p->size - 1)*sizeof(chunk_t) < nbytes + sizeof(chunk_t));
    // assert(p->next == NULL);  // saftey first!
    // return q;


    // obviously the next line is WRONG!!!!  You must fix it.
    //  return malloc(nbytes);
}

/* prints stats about the current free list
 *
 * -- number of items in the linked list including dummy item
 * -- min, max, and average size of each item (in bytes)
 * -- total memory in list (in bytes)
 * -- number of calls to sbrk and number of pages requested
 *
 * A message is printed if all the memory is in the free list
 */
void Mem_stats(void)
{
    //printf("the student must implement mem stats\n");
    // One of the stats you must collect is the total number
    // of pages that have been requested using sbrk.
    // Say, you call this NumPages.  You also must count M,
    // the total number of bytes found in the free list
    // (including all bytes used for headers).  If it is the case
    // that M == NumPages * PAGESiZE then print
    // printf("all memory is in the heap -- no leaks are possible\n");
    chunk_t *start_scan = Rover;
    int M = 0; // total bytes found in free list
    int numItems = 1, average = 0;
    int max = Rover->size;
    int min = PAGESIZE;

    do {
      if (Rover->size != 0)
      {
        M += Rover->size;
        if (max < Rover->size) {max = Rover->size;}
        if (Rover->size < min) {min = Rover->size;}
        numItems++;
      }
      Rover = Rover->next;
    } while(Rover != start_scan);

    if (numItems == 1) { min = 0; }
    average = M / (numItems-1);
    M *= sizeof(chunk_t);

    printf("Number of items in list (including Dummy): %d\n", numItems);
    printf("Min chunk size: %d bytes\n", min);
    printf("Max chunk size: %d bytes\n", max);
    printf("Average chunk size: %d bytes\n", average);
    printf("Total memory in free list: %d bytes\n", M);
    printf("Number of calls to sbrk(): %d\n", NumSbrkCalls);
    printf("Pages requested: %d\n", NumPages);
    if (M == NumPages * PAGESIZE)
      printf("all memory is in the heap -- no leaks are possible\n");

}

/* print table of memory in free list
 *
 * The print should include the dummy item in the list
 */
void Mem_print(void)
{
    assert(Rover != NULL && Rover->next != NULL);
    chunk_t *p = Rover;
    chunk_t *start = p;
    do {
        // example format.  Modify for your design
        printf("p=%p, size=%d, end=%p, next=%p %s\n",
                p, p->size, p + p->size, p->next, p->size!=0?"":"<-- dummy");
        p = p->next;
    } while (p != start);
    mem_validate();
}

/* This is an experimental function to attempt to validate the free
 * list when coalescing is used.  It is not clear that these tests
 * will be appropriate for all designs.  If your design utilizes a different
 * approach, that is fine.  You do not need to use this function and you
 * are not required to write your own validate function.
 */
void mem_validate(void)
{
    assert(Rover != NULL && Rover->next != NULL);
    assert(Rover->size >= 0);
    int wrapped = FALSE;
    int found_dummy = FALSE;
    int found_rover = FALSE;
    chunk_t *p, *largest, *smallest;

    // for validate begin at Dummy
    p = &Dummy;
    do {
        if (p->size == 0) {
            assert(found_dummy == FALSE);
            found_dummy = TRUE;
        } else {
            assert(p->size > 0);
        }
        if (p == Rover) {
            assert(found_rover == FALSE);
            found_rover = TRUE;
        }
        p = p->next;
    } while (p != &Dummy);
    assert(found_dummy == TRUE);
    assert(found_rover == TRUE);

    if (Coalescing) {
        do {
            if (p >= p->next) {
                // this is not good unless at the one wrap
                if (wrapped == TRUE) {
                    printf("validate: List is out of order, already found wrap\n");
                    printf("first largest %p, smallest %p\n", largest, smallest);
                    printf("second largest %p, smallest %p\n", p, p->next);
                    assert(0);   // stop and use gdb
                } else {
                    wrapped = TRUE;
                    largest = p;
                    smallest = p->next;
                }
            } else {
                assert(p + p->size < p->next);
            }
            p = p->next;
        } while (p != &Dummy);
        assert(wrapped == TRUE);
    }
}
/* vi:set ts=8 sts=4 sw=4 et: */
