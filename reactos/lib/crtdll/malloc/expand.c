#include <windows.h>
#include <kernel32/heap.h>
#include <msvcrt/malloc.h>
#include <msvcrt/stdlib.h>

void* _expand(void* pold, size_t size)
{
   PHEAP_BUCKET	pbucket;
   PHEAP_SUBALLOC psub;
   PHEAP_FRAGMENT pfrag = (PHEAP_FRAGMENT)((LPVOID)pold-HEAP_FRAG_ADMIN_SIZE);
   
   /* sanity checks */
   if (pfrag->Magic != HEAP_FRAG_MAGIC)
      return NULL;

   /* get bucket size */
   psub = pfrag->Sub;
   pbucket = psub->Bucket;
   if(size <= pbucket->Size) {
      pfrag->Size=size;
      return pold;
   }
   else
      return NULL;
   return NULL;
}

size_t _msize(void* pBlock)
{
   PHEAP_BUCKET	pbucket;
   PHEAP_SUBALLOC psub;
   PHEAP_FRAGMENT pfrag = (PHEAP_FRAGMENT)((LPVOID)pBlock-HEAP_FRAG_ADMIN_SIZE);
   
   /* sanity checks */
   if (pfrag->Magic != HEAP_FRAG_MAGIC)
      return 0;

   /* get bucket size */
   psub = pfrag->Sub;
   pbucket = psub->Bucket;
   return pbucket->Size;
}
