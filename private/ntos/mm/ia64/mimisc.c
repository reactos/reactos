/*++

#include "intelcopy.h"

Module Name:

    mimisc.c

Abstract:

    This module contains misllaneous IA64 specific memory management routines.


Author:

    ky 28-Jun-96

Revision History:


--*/

#include "mi.h"


 
//
// memory types
//

typedef enum _MEM_TYPES {
    RegularMemory,
    MemoryMappedIo,
    VideoDisplayBuffer,
    IoPort,
    RomMemory
} MEM_TYPES;


typedef struct _CACHE_ATTRIBUTE_DESCRIPTOR {
    LIST_ENTRY ListEntry;
    MEM_TYPES MemTypes;
    ULONG CacheAttribute;
    PFN_NUMBER BasePage;
    PFN_NUMBER PageCount;
} CACHE_ATTRIBUTE_DESCRIPTOR, *PCACHE_ATTRIBUTE_DESCRIPTOR;


LIST_ENTRY MmCacheAttributeDescriptorListHead;

//
// default memory cache attributes set within PTE:
//

ULONG MmDefaultCacheAttribute = MM_PTE_MA_WBU;   // cacheable, write-back, unordered



ULONG
MiCheckMemoryAttribute(
   IN PFN_NUMBER PageFrameNumber
   )

/*++

Routine Descrition:

    This function examines the physical address which is given 
    by the physical page frame number, and returns the cache 
    attributes type for that page. The returned value is used to specify 
    the MemoryAttribute field in the HARDWARE_PTE structure to map 
    that page.
     
    This function searches the cache descriptor attribute link 
    lists to see if the specific cache attribute is defined for 
    that physical address range.  

    If the physical address range is not defined in the cache 
    descriptor attribute link lists, returns the default memory 
    attribute.

Arguments:

    PageFrameIndex - Supplies the physical page frame number to be
                     examined for the cache attribute.

Return Value: 

    Returns a cache attribute type for the supplied physical page 
    frame number. 
 
Environment:

    Kernel Mode Only.

--*/

{
   PLIST_ENTRY NextMd;

   NextMd = MmCacheAttributeDescriptorListHead.Flink;

   While (NextMd != MmCacheAttributeDescriptorListHead) {

       CacheAttributeDescriptor = CONTAINING_RECORD(NextMd,
                                                     CACHE_ATTRIBUTE_DESCRIPTOR,
                                                     ListEntry);

       if ((PageFrameNumber >= CacheAttributeDescriptor.BasePage) ||
           (PageFrameNumber < CacheAttributeDescriptor.PageCount)) {

           return (CacheAttributeDescriptor.CacheAttribute);
           
       }

       NextMd = CacheAttributeDescriptor->ListEntry.Flink;
   }

   //
   // if the cache memory descriptor is not found, 
   // return the default cache attribute, MmDefaultCacheAttribute.
   //
          
   return (MmDefaultCacheAttribute);

}



//
// MmDisableCache yields 0 if cachable, 1 if uncachable. 
//

UCHAR MmDisableCache[16] = {0, 0, 0, 0, 0, 0, 0, 0
                           1, 1, 0, 1, 0, 1, 0, 0};
        


MiDisableCaching(
    IN PMMPTE PointerPte;
    )

/*++

Routine Description:

    This macro takes a valid PTE and sets the caching state to be
    disabled.

Argments

    PTE - Supplies a pointer to the valid PTE.
  
Return Value:

    None.

Environment:

    Kernel mode

--*/

{
    ULONG CacheAttribute;

    CacheAttribute = MiCheckMemoryAttribute(TempPte.u.Hard.PageFrameNumber);

    if (MmDisableCache[CacheAttribute]) {
        
        //
        // The returned CacheAttributes indicate uncachable.
        //

        PointerPte->u.Hard.MemAttribute = CacheAttribute;

    } else {

        //
        // Set the most conservative cache memory attribute,
        // UCO (Uncachable, Non-coalescing, Sequential & Non-
        // speculative and Ordered.
        //
        
        PointerPte->u.Hard.MemAttribute = MM_PTE_MA_UCO;

    }
}




