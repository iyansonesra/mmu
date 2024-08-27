#include "tdmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <regex.h>
#include <ctype.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <sys/mman.h>

#define META_SIZE 32

struct meta
{
  u_int64_t sizeFree; // 8 bytes (size and free)
  void *address;
  struct meta *next; // 8 bytes
  struct meta *prev; // 8 bytes
};
typedef struct meta meta;

struct buddyMeta
{
  u_int64_t sizeFree; // 8 bytes (size and free)
  void *address;
  struct buddyMeta *leftChild; // 8 bytes
  struct buddyMeta *rightChild; // 8 bytes
};
typedef struct buddyMeta buddyMeta;

meta *head;
buddyMeta *buddyHead;
void *stack_bot;
alloc_strat_e currStrat;
bool started = false;
bool occurred = false;
int64_t totalSize = 10;

uint64_t getSize(uint64_t input) {
  uint64_t one = 1;
  input &= ~(one << 63);
  return ((uint64_t)input);
}

uint64_t getFree(uint64_t input) {
  return ((input >> 63) & 1);
}

uint64_t getSizeFree(uint64_t size, uint64_t free) {
  uint64_t sizeFree = size;
  uint64_t one = 1;

if (free == 0)
    sizeFree &= ~(one << 63);
  else 
    sizeFree |= one<<63;
  return sizeFree;
}

void
t_init (alloc_strat_e strat, void* stack_bottom)
{
  currStrat = strat;
  stack_bot = stack_bottom;
  head = NULL;
}

void
printPage (void *location, int count) {
  if (location != NULL) {
      meta *loc = (meta *)location;
      printf ("%s%d ", "BLOCK", count);
      if (getFree(loc->sizeFree) == 1) {
          printf ("\033[0;32m.\033[0m");
          printf ("\033[0;32m.\033[0m");
          printf ("\033[0;32m.\033[0m\n");
      }
      else {
          printf ("\033[0;31m.\033[0m");
          printf ("\033[0;31m.\033[0m");
          printf ("\033[0;31m.\033[0m\n");
      }

      printf ("Size:");
      printf ("%ld\n", getSize(loc->sizeFree));

      printf ("isFree:");
      printf ("%d\n", getFree(loc->sizeFree));

      printf("Address: %p\n", (void *)loc->address);

      printf ("------------------------------\n");
      printPage (loc->next, count + 1);
  } else {
      return;
  }
}

meta * writeMetaEdge(size_t size, meta * prevBlock)
{
    meta * block1 = (meta *)mmap(NULL, size > 4096-(META_SIZE*2) ? (size+META_SIZE*2) : 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    block1->sizeFree = getSizeFree(size, 0);
    block1->prev = prevBlock;
    block1->address = (char *)block1 + META_SIZE;

    meta * block2 = (meta *)(block1->address+size);
    block2->sizeFree = getSizeFree(size == 4096-(META_SIZE*2) ? 0 : 4096-((size+META_SIZE*2)%4096), 1); 
    block2->next = NULL;
    block2->prev = block1;
    block2->address = (char *)block2 + META_SIZE;
    block1->next = block2;

    return block1;
}

buddyMeta * writeBuddyEdge(size_t size, buddyMeta * prevBlock)
{
    buddyMeta * block1 = (buddyMeta *)mmap(NULL, size + META_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    uint64_t currSize = (((size + META_SIZE - 1) / 4096) + 1) * 4096 - META_SIZE;
    block1->sizeFree = getSizeFree(currSize, 0);
    block1->rightChild = NULL;
    block1->leftChild = prevBlock;
    block1->address = (char *)block1 + META_SIZE;
    
    while((currSize-META_SIZE)/2 >= size) {
      buddyMeta * intermediate1 = (buddyMeta *)(block1->address + (currSize-META_SIZE)/2);
      intermediate1->rightChild = block1->rightChild;
      if(block1->rightChild != NULL) {
        block1->rightChild->leftChild = intermediate1;
      }
      intermediate1->leftChild = block1;
      block1->rightChild = intermediate1;
      intermediate1->address = (char *)intermediate1 + META_SIZE;
      intermediate1->sizeFree = getSizeFree((currSize-META_SIZE)/2, 1);
      block1->sizeFree = getSizeFree((currSize-META_SIZE)/2, 0);
      
      currSize = (currSize-META_SIZE)/2;
    }
    
    return block1;
}

void * placeIntermediate(meta * curr, size_t size)
{
  size_t oldSize = getSize(curr->sizeFree);
  curr->sizeFree = getSizeFree(size, 0);

  if(oldSize > size + META_SIZE) { // in the case there is space for an intermediate struct
  meta * intermediate1 = (meta *)(curr->address+size);
  intermediate1->sizeFree = getSizeFree(oldSize - META_SIZE - size, 1);
  intermediate1->next = curr->next;
  curr->next->prev = intermediate1;
  intermediate1->prev = curr;
  intermediate1->address = (char *)intermediate1 + META_SIZE;
  curr->next = intermediate1;
  }

  return (void *)curr->address;
}


void * placeLast(meta * curr, size_t size)
{
  size_t oldSize = getSize(curr->sizeFree);
  curr->sizeFree = getSizeFree(size, 0);

  meta * intermediate1 = (meta *)(curr->address+size);
  intermediate1->sizeFree = getSizeFree(oldSize - META_SIZE - size, 1);
  intermediate1->next = NULL;
  intermediate1->prev = curr;
  intermediate1->address = (char *)intermediate1 + META_SIZE;
  curr->next = intermediate1;

  return (void *)curr->address;
}


void * first_fit(size_t size)
{
  meta * curr = head;
  while(curr != NULL)
  {
    if(getFree(curr->sizeFree) && getSize(curr->sizeFree) > size+META_SIZE) {
      if(curr->next != NULL) {
        return placeIntermediate(curr, size);
      } else {
        return placeLast(curr, size);
      }
    }
      
    if(curr->next == NULL)
      break;
    curr = curr->next;
  }

  curr-> next = writeMetaEdge(size, curr);
  return curr->next->address;
}

void * best_fit(size_t size)
{
  meta * curr = head;
  meta * checker = head;
  size_t closestSize = INT64_MAX;
  meta * best = NULL;
  while(curr != NULL)
  {
    if(getFree(curr->sizeFree) == 1 && getSize(curr->sizeFree) > size+META_SIZE) {
      if(getSize(curr->sizeFree) < closestSize) {
        closestSize = getSize(curr->sizeFree);
        best = curr;
      }
    }
      
    if(curr->next == NULL)
      break;
    curr = curr->next;
  }

  if(best == NULL) {
    curr-> next = writeMetaEdge(size, curr);
    return curr->next->address;
  } else {
    if(best->next != NULL) {
        return placeIntermediate(best, size);
      } else {
        return placeLast(best, size);
      }
  }
}


void * worst_fit(size_t size)
{
  meta * curr = head;
  size_t closestSize = 0;
  meta * best = NULL;
  while(curr != NULL)
  {
    if(getFree(curr->sizeFree) && getSize(curr->sizeFree) > size+META_SIZE) {
      if(getSize(curr->sizeFree) > closestSize) {
        closestSize = getSize(curr->sizeFree);
        best = curr;
      }
    }
      
    if(curr->next == NULL)
      break;
    curr = curr->next;
  }

  if(best == NULL) {
    curr-> next = writeMetaEdge(size, curr);
    return curr->next->address;
  } else {
    if(best->next != NULL) {
      return placeIntermediate(best, size);
    } else {
      return placeLast(best, size);
    }
  }
}

void * buddy(size_t size)
{
  buddyMeta * curr = buddyHead;
  while(curr != NULL) {
    if(getFree(curr->sizeFree) == 1 && getSize(curr->sizeFree) > size) {
      curr->sizeFree = getSizeFree(getSize(curr->sizeFree), 0);
      size_t currSize = getSize(curr->sizeFree);
      
      while((currSize-META_SIZE)/2 >= size) {
        buddyMeta * intermediate1 = (buddyMeta *)(curr->address+(currSize-META_SIZE)/2);
        intermediate1->rightChild = curr->rightChild;
        intermediate1->leftChild = curr;
        curr->rightChild = intermediate1;
        intermediate1->sizeFree = getSizeFree((currSize-META_SIZE)/2, 1);
        intermediate1->address = (char *)intermediate1 + META_SIZE;
        curr->sizeFree = getSizeFree((currSize-META_SIZE)/2, 0);
        currSize = (currSize-META_SIZE)/2;
      }
      
      return curr->address;
    }
      
    if(curr->rightChild == NULL)
      break;
    curr = curr->rightChild;
  }

  curr-> rightChild = writeBuddyEdge(size, curr);
  return curr->rightChild->address;
}

void *
t_malloc (size_t size)
{
  totalSize++;
  if (size % 4 != 0)
        size = size + (4 - size % 4);
  if(!started) {
    if(currStrat != BUDDY) {
      head = writeMetaEdge(size, NULL);
      started = true;
      return head->address;
    } else {
      buddyHead = writeBuddyEdge(size, NULL);
      started = true;
      return buddyHead->address;
    }
  }
  else {
    if(currStrat == FIRST_FIT) {
      void * output =  first_fit(size); //changed this
      return output;
    } else if(currStrat == BEST_FIT) {
      return  best_fit(size);
    } else if(currStrat == WORST_FIT) {
      return worst_fit(size);
    } else if(currStrat == BUDDY) {
     return buddy(size);
    }
  }
}

void
t_free (void *ptr)
{
    if(currStrat != BUDDY) {
      meta * curr = (meta *)((char *)ptr - META_SIZE);
      curr->sizeFree = getSizeFree(getSize(curr->sizeFree), 1);
      if(curr->next != NULL && curr->next->address == (curr->address + getSize(curr->sizeFree) + META_SIZE) && getFree(curr->next->sizeFree) == 1) {
        curr->sizeFree = getSizeFree(getSize(curr->sizeFree) + getSize(curr->next->sizeFree) + META_SIZE, 1);
        curr->next = curr->next->next;
        if(curr->next != NULL)
          curr->next->prev = curr;
      }

      if(curr->prev != NULL && curr->address == (curr->prev->address + getSize(curr->prev->sizeFree) + META_SIZE) &&  getFree(curr->prev->sizeFree) == 1) {
        if(!(curr->prev->next == NULL || curr->prev->next != curr)) {
          curr->prev->sizeFree = getSizeFree(getSize(curr->prev->sizeFree) + getSize(curr->sizeFree) + META_SIZE, 1);
          curr->prev->next = curr->next;
          if(curr->next != NULL)
            curr->next->prev = curr->prev;
          occurred = true;
        }
      }
    } else {
      buddyMeta * curr = buddyHead;
      while(curr != NULL) {
        if(curr->address == ptr) {
          if(!occurred) {
            curr->sizeFree = getSizeFree(getSize(curr->sizeFree), 1);
            if(curr->rightChild != NULL && curr->rightChild->address == (curr->address + getSize(curr->sizeFree) + META_SIZE) && getFree(curr->rightChild->sizeFree) == 1) {
              curr->sizeFree = getSizeFree(getSize(curr->sizeFree) + getSize(curr->rightChild->sizeFree) + META_SIZE, 1);
              curr->rightChild = curr->rightChild->rightChild;
              if(curr->rightChild != NULL)
                curr->rightChild->leftChild = curr;
            }
      
            if(curr->leftChild != NULL && curr->address == (curr->leftChild->address + getSize(curr->leftChild->sizeFree) + META_SIZE) &&  getFree(curr->leftChild->sizeFree) == 1) {
              if(!(curr->leftChild->rightChild == NULL || curr->leftChild->rightChild != curr)) {
                curr->leftChild->sizeFree = getSizeFree(getSize(curr->leftChild->sizeFree) + getSize(curr->sizeFree) + META_SIZE, 1);
                curr->leftChild->rightChild = curr->rightChild;
                if(curr->rightChild != NULL)
                  curr->rightChild->leftChild = curr->leftChild;
                occurred = true;
              }
            }
            break;
          } else {
          break;
          }
        }
        if(curr->rightChild == NULL)
          break;
        curr = curr->rightChild;
      }
    }
}


void t_freeFast(void *ptr) {
  meta * curr = (meta *)((char *)ptr - META_SIZE);
  curr->sizeFree = getSizeFree(getSize(curr->sizeFree), 1);
  
  if(curr->next != NULL && curr->next->address == (curr->address + getSize(curr->sizeFree) + META_SIZE) && getFree(curr->next->sizeFree) == 1) {
    curr->sizeFree = getSizeFree(getSize(curr->sizeFree) + getSize(curr->next->sizeFree) + META_SIZE, 1);
    curr->next = curr->next->next;
    if(curr->next != NULL)
      curr->next->prev = curr;
  }

  if(curr->prev != NULL && curr->address == (curr->prev->address + getSize(curr->prev->sizeFree) + META_SIZE) &&  getFree(curr->prev->sizeFree) == 1) {
    if(!(curr->prev->next == NULL || curr->prev->next != curr)) {
      curr->prev->sizeFree = getSizeFree(getSize(curr->prev->sizeFree) + getSize(curr->sizeFree) + META_SIZE, 1);
      curr->prev->next = curr->next;
      if(curr->next != NULL)
        curr->next->prev = curr->prev;
      occurred = true;
    }
  }
}

void mark(char * top) {
   while(top < (char *) stack_bot+totalSize) { 
    void * currRef = *(void **)top;
    meta * curr = head; //stack t0 heap check
    bool founder = false;
    while(curr != NULL) {
      if(currRef >= curr->address && (char *)currRef <= (char *)curr->address + getSize(curr->sizeFree) && getSize(curr->sizeFree) % 4 == 0) {
        curr->sizeFree = getSizeFree(getSize(curr->sizeFree)+1, getFree(curr->sizeFree));   
      }

      if(curr->next == NULL || founder) break;
      else curr = curr->next;
    }
    top++;
  }
}

void sweep() {
  meta * curr3 = head;
  while(curr3 != NULL) {
    if(getSize(curr3->sizeFree) % 4 == 0) {
      t_freeFast(curr3->address);
    } else {
      curr3->sizeFree = getSizeFree(getSize(curr3->sizeFree)-1, getFree(curr3->sizeFree));
    }

    if(curr3->next == NULL) break;
      else curr3 = curr3->next;
  }
}

void
t_gcollect (void)
{
  long x = 0;
  char * top = (char *) &x;
  char * secondary = (char *) &x;
  
  mark(top);
  sweep();
}