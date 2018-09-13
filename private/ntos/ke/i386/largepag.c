#include "ki.h"
#include "ki386.h"

PVOID
Ki386AllocateContiguousMemory(
    IN OUT PIDENTITY_MAP IdentityMap,
    IN     ULONG Pages,
    IN     BOOLEAN Low4Meg
    );

BOOLEAN
Ki386IdentityMapMakeValid(
    IN OUT PIDENTITY_MAP IdentityMap,
    IN     PHARDWARE_PTE PageTableEntry,
    OUT    PVOID *Page OPTIONAL
    );

BOOLEAN
Ki386MapAddress(
    IN OUT PIDENTITY_MAP IdentityMap,
    IN     ULONG Va,
    IN     PHYSICAL_ADDRESS PhysicalAddress
    );

PVOID
Ki386ConvertPte(
    IN OUT PHARDWARE_PTE Pte
    );

PHYSICAL_ADDRESS
Ki386BuildIdentityBuffer(
    IN OUT PIDENTITY_MAP IdentityMap,
    IN     PVOID StartVa,
    IN     ULONG Length,
    OUT    PULONG PagesToMap
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT,Ki386AllocateContiguousMemory)
#pragma alloc_text(INIT,Ki386BuildIdentityBuffer)
#pragma alloc_text(INIT,Ki386ClearIdentityMap)
#pragma alloc_text(INIT,Ki386ConvertPte)
#pragma alloc_text(INIT,Ki386CreateIdentityMap)
#pragma alloc_text(INIT,Ki386EnableTargetLargePage)
#pragma alloc_text(INIT,Ki386IdentityMapMakeValid)
#pragma alloc_text(INIT,Ki386MapAddress)

#endif

#define PTES_PER_PAGE (PAGE_SIZE / sizeof(HARDWARE_PTE))

BOOLEAN
Ki386CreateIdentityMap(
    IN OUT PIDENTITY_MAP IdentityMap,
    IN     PVOID StartVa,
    IN     PVOID EndVa
    )
{
/*++

    This function creates an identity mapping for a region of memory.

    If the region of memory passed in includes memory that lies above
    4G, then a new buffer is allocated below 4G.

Arguments:

    IdentityMap - Pointer to the structure which will be filled with the newly
                  created top-level directory address.  It also provides
                  storage for the pointers used in alloating and freeing the
                  memory.

    StartVa - Pointer to the first byte of the region of memory that is to be
              memory mapped.

    EndVa - Pointer to the byte immediately after the last byte of the region
            that is to be memory mapped.

Return Value:

    TRUE if the function succeeds, FALSE otherwise.

    Note - Ki386ClearIdentityMap() should be called even on FALSE return to
    free any memory allocated.
    
--*/
    
    ULONG pageDirectoryIndex;
    ULONG pagesToMap;
    PCHAR currentVa;
    ULONG length;
    BOOLEAN result;
    PHARDWARE_PTE pageDirectory;
    PHARDWARE_PTE pageDirectoryEntry;
    PHYSICAL_ADDRESS identityAddress;

#if defined(_X86PAE_)

    ULONG pageDirectoryPointerTableIndex;
    PHARDWARE_PTE pageDirectoryPointerTable;
    PHARDWARE_PTE pageDirectoryPointerTableEntry;

#endif

    //
    // Initialize the IdentityMap structure to a known state.
    // 

    RtlZeroMemory( IdentityMap, sizeof(IDENTITY_MAP) );
    length = (PCHAR)EndVa - (PCHAR)StartVa;

    //
    // Get the physical address of the input buffer (or suitable copy).
    //

    identityAddress = Ki386BuildIdentityBuffer( IdentityMap,
                                                StartVa,
                                                length,
                                                &pagesToMap );
    if( identityAddress.QuadPart == 0) {

        //
        // The input buffer was not contiguous or not below 4G, and a
        // suitable buffer could not be allocated.
        //

        return FALSE;
    }

    IdentityMap->IdentityAddr = identityAddress.LowPart;

    //
    // Set up the mappings.
    //

    currentVa = StartVa;
    do {

        //
        // Map in the virtual address
        // 

        result = Ki386MapAddress( IdentityMap,
                                  (ULONG)currentVa,
                                  identityAddress );
        if (result == FALSE) {
            return FALSE;
        }

        //
        // Map in the identity (physical) address
        // 

        result = Ki386MapAddress( IdentityMap,
                                  identityAddress.LowPart,
                                  identityAddress );
        if (result == FALSE) {
            return FALSE;
        }

        //
        // Advance both the Va and identityAddress pointers in anticipation
        // of mapping in another page.
        //

        currentVa += PAGE_SIZE;
        identityAddress.QuadPart += PAGE_SIZE;
        pagesToMap -= 1;

    } while (pagesToMap > 0);

    //
    // Now go through the page directory pointer table and page directories,
    // converting virtual page frames to physical ones.
    //

#if defined(_X86PAE_)

    //
    // This PAE-only outer loop walks the page directory pointer table entries
    // and processes each valid page directory referenced.
    // 

    pageDirectoryPointerTable = IdentityMap->TopLevelDirectory;
    for (pageDirectoryPointerTableIndex = 0;
         pageDirectoryPointerTableIndex < (1 << PPI_BITS);
         pageDirectoryPointerTableIndex++) {

        pageDirectoryPointerTableEntry =
            &pageDirectoryPointerTable[ pageDirectoryPointerTableIndex ];

        if (pageDirectoryPointerTableEntry->Valid == 0) {
            continue;
        }

        pageDirectory =
            (PHARDWARE_PTE)Ki386ConvertPte( pageDirectoryPointerTableEntry );

#else
        pageDirectory = IdentityMap->TopLevelDirectory;
#endif

        for (pageDirectoryIndex = 0;
             pageDirectoryIndex < PTES_PER_PAGE;
             pageDirectoryIndex++) {

            pageDirectoryEntry = &pageDirectory[ pageDirectoryIndex ];
            if (pageDirectoryEntry->Valid == 0) {
                continue;
            }

            Ki386ConvertPte( pageDirectoryEntry );
        }

#if defined(_X86PAE_)
    }
#endif

    identityAddress = MmGetPhysicalAddress( IdentityMap->TopLevelDirectory );
    IdentityMap->IdentityCR3 = identityAddress.LowPart;

    return TRUE;
}

PVOID
Ki386AllocateContiguousMemory(
    IN OUT PIDENTITY_MAP IdentityMap,
    IN     ULONG Pages,
    IN     BOOLEAN Low4Meg
    )
/*++

    This function allocates page-aligned, physically contiguous memory.
    The allocation is recorded in the IdentityMap structure, so that it
    can be freed on cleanup.

Arguments:

    IdentityMap - Context pointer for this identity mapping.

    Pages - Number of pages to allocate

    Low4Meg - Indicates whether the allocation must be below 4M.

Return Value:

    Pointer to the new page on success, NULL otherwise.
    
--*/
{
    ULONG pageListIndex;
    PVOID page;
    ULONG allocationSize;
    PHYSICAL_ADDRESS highestAddress;

    if (Low4Meg != FALSE) {

        //
        // The caller has specified that a page must reside physically
        // below 4 MB.
        //

        highestAddress.LowPart = 0xFFFFFFFF;
        highestAddress.HighPart = 0;

    } else {

        //
        // Memory can reside anywhere
        //

        highestAddress.LowPart = 0xFFFFFFFF;
        highestAddress.HighPart = 0xFFFFFFFF;
    }

    allocationSize = Pages * PAGE_SIZE;
    page = MmAllocateContiguousMemory( allocationSize, highestAddress );
    if (page != NULL) {

        //
        // Record that this page was allocated so that it can be freed when
        // the IdentityMap structure is cleared.
        //
    
        pageListIndex = IdentityMap->PagesAllocated;
        IdentityMap->PageList[ pageListIndex ] = page;
        IdentityMap->PagesAllocated++;

        //
        // Initialize it.
        //

        RtlZeroMemory( page, allocationSize );
    }

    return page;
}

BOOLEAN
Ki386IdentityMapMakeValid(
    IN OUT PIDENTITY_MAP IdentityMap,
    IN     PHARDWARE_PTE PageTableEntry,
    OUT    PVOID *Page OPTIONAL
    )
/*++

    If the page table has the valid bit set, this function merely returns
    the address referenced by the page table entry.

    If the page table does not have the valid bit set, then another page
    is allocated and inserted into the page table entry and the entry is
    marked valid.

    NOTE: At this point, PTE frames are virtual.  After the entire mapping
          is built, we go through and convert all virtual frames to physical
          ones.

Arguments:

    IdentityMap - Context pointer for this identity mapping.

    PageTableEntry - Pointer to the page table entry.

    Page - Virtual address now referenced by the PTE, whether it was
           valid before or not.

Return Value:

    TRUE on success, FALSE otherwise.
    
--*/
{
    PVOID page;

    if (PageTableEntry->Valid != 0) {

        //
        // If it already is present, there is nothing to do except record
        // the virtual page number that is already there.
        //

        page = (PVOID)((ULONG)(PageTableEntry->PageFrameNumber << PAGE_SHIFT));

    } else {

        //
        // The page table entry is not valid.  Allocate a new page table.
        // 
    
        page = Ki386AllocateContiguousMemory( IdentityMap, 1, FALSE );
        if (page == NULL) {
            return FALSE;
        }
    
        //
        // Insert it into the page table entry and mark it valid.
        //
        // NOTE: Virtual page numbers are inserted into the page table
        //       structure as it is being built.  When it is finished, we walk
        //       the tables and convert all of the virtual page numbers to
        //       physical page numbers.
        //
    
        PageTableEntry->PageFrameNumber = ((ULONG)page) >> PAGE_SHIFT;
        PageTableEntry->Valid = 1;
    }

    if (ARGUMENT_PRESENT( Page )) {
        *Page = page;
    }

    return TRUE;
}           

BOOLEAN
Ki386MapAddress(
    IN OUT PIDENTITY_MAP IdentityMap,
    IN     ULONG Va,
    IN     PHYSICAL_ADDRESS PhysicalAddress
    )

/*++

    Creates a new virtual->physical mapping in the identity map.

Arguments:

    IdentityMap - Context pointer for this identity mapping.

    Va - Virtual address to map.

    PhysicalAddress - Physical address to map.

Return Value:

    TRUE on success, FALSE otherwise.
    
--*/
{
    PHARDWARE_PTE pageTable;
    PHARDWARE_PTE pageTableEntry;
    PHARDWARE_PTE pageDirectory;
    PHARDWARE_PTE pageDirectoryEntry;
    PVOID table;
    ULONG index;
    BOOLEAN result;

#if defined(_X86PAE_)
    PHARDWARE_PTE pageDirectoryPointerTable;
    PHARDWARE_PTE pageDirectoryPointerTableEntry;
#endif

    if (IdentityMap->TopLevelDirectory == NULL) {

        //
        // Allocate a top-level directory structure, either a page directory
        // or a page directory pointer table.
        //

        table = Ki386AllocateContiguousMemory( IdentityMap, 1, TRUE );
        if (table == FALSE) {
            return FALSE;
        }

        IdentityMap->TopLevelDirectory = table;
    }

#if defined(_X86PAE_)

    index = KiGetPpeIndex( Va );
    pageDirectoryPointerTable = IdentityMap->TopLevelDirectory;
    pageDirectoryPointerTableEntry = &pageDirectoryPointerTable[ index ];

    result = Ki386IdentityMapMakeValid( IdentityMap,
                                        pageDirectoryPointerTableEntry,
                                        &pageDirectory );
    if (result == FALSE) {
        return FALSE;
    }

#else

    pageDirectory = IdentityMap->TopLevelDirectory;

#endif

    //
    // Get a pointer to the appropriate page directory entry.  If it is
    // not valid, allocate a new page table and mark the page directory
    // entry valid and writeable.
    // 

    index = KiGetPdeIndex( Va );
    pageDirectoryEntry = &pageDirectory[ index ];
    result = Ki386IdentityMapMakeValid( IdentityMap,
                                        pageDirectoryEntry,
                                        &pageTable );
    if (result == FALSE) {
        return FALSE;
    }
    pageDirectoryEntry->Write = 1;

    //
    // Get a pointer to the appropriate page table entry and fill it in.
    // 

    index = KiGetPteIndex( Va );
    pageTableEntry = &pageTable[ index ];

#if defined(_X86PAE_)
    pageTableEntry->PageFrameNumber = PhysicalAddress.QuadPart >> PAGE_SHIFT;
#else
    pageTableEntry->PageFrameNumber = PhysicalAddress.LowPart >> PAGE_SHIFT;
#endif
    pageTableEntry->Valid = 1;

    return TRUE;
}

PVOID
Ki386ConvertPte(
    IN OUT PHARDWARE_PTE Pte
    )
/*++

    Converts the virtual frame number in a PTE to a physical frame number.

Arguments:

    Pte - Pointer to the page table entry to convert.

Return Value:

    None.
    
--*/
{
    PVOID va;
    PHYSICAL_ADDRESS physicalAddress;

    va = (PVOID)(Pte->PageFrameNumber << PAGE_SHIFT);
    physicalAddress = MmGetPhysicalAddress( va );

#if defined(_X86PAE_)
    Pte->PageFrameNumber = physicalAddress.QuadPart >> PAGE_SHIFT;
#else
    Pte->PageFrameNumber = physicalAddress.LowPart >> PAGE_SHIFT;
#endif

    return va;
}

PHYSICAL_ADDRESS
Ki386BuildIdentityBuffer(
    IN OUT PIDENTITY_MAP IdentityMap,
    IN     PVOID StartVa,
    IN     ULONG Length,
    OUT    PULONG PagesToMap
    )
{

/*++

    This function checks to see if the physical memory backing a virtual
    buffer is physically contiguous and lies completely below 4G.

    If these requirements are met, then the physical address of StartVa is
    returned.

    If not, then a physically contiguous buffer is allocated, the contents
    of the region is copied in, and its address is returned.

Arguments:

    IdentityMap - Pointer to the identity map building structure.

    StartVa - Virtual address of the start of the region for which a
              physically contiguous copy is desired.

    Length - Length of the region for which a physically contiguous copy
             is desired.

--*/

    ULONG pagesToMap;
    ULONG pagesRemaining;
    PCHAR nextVirtualAddress;
    PHYSICAL_ADDRESS nextPhysicalAddress;
    PHYSICAL_ADDRESS physicalAddress;
    PHYSICAL_ADDRESS firstPhysicalAddress;
    ULONG pageOffset;
    PCHAR identityBuffer;

    //
    // Count the number of pages in the buffer, and record the physical
    // address of the start of the buffer.
    //

    pagesToMap = ADDRESS_AND_SIZE_TO_SPAN_PAGES( StartVa, Length );
    nextVirtualAddress = StartVa;
    firstPhysicalAddress = MmGetPhysicalAddress( StartVa );
    nextPhysicalAddress = firstPhysicalAddress;

    //
    // Examine each page in the region.
    // 

    pagesRemaining = pagesToMap;
    while (TRUE) {
        
        physicalAddress = MmGetPhysicalAddress( nextVirtualAddress );
        if (physicalAddress.QuadPart != nextPhysicalAddress.QuadPart) {

            //
            // The buffer is not physically contiguous.
            //

            break;
        }

        if (physicalAddress.HighPart != 0) {

            //
            // The buffer does not lie entirely below 4G
            //

            break;
        }

        pagesRemaining -= 1;
        if (pagesRemaining == 0) {

            //
            // All of the pages in the buffer have been examined, and have
            // been found to meet the critera.  Return the physical address
            // of the start of the buffer.
            //

            *PagesToMap = pagesToMap;
            return firstPhysicalAddress;
        }

        nextVirtualAddress += PAGE_SIZE;
        nextPhysicalAddress.QuadPart += PAGE_SIZE;
    }

    //
    // The buffer does not meet the criteria and so its contents must be
    // copied to a buffer that does.
    //

    identityBuffer = Ki386AllocateContiguousMemory( IdentityMap,
                                                    pagesToMap,
                                                    TRUE );
    if (identityBuffer == 0) {

        //
        // A contiguous region of the appropriate size could not be located
        // below 4G physical.
        // 

        physicalAddress.QuadPart = 0;

    } else {

        //
        // Got an appropriate physical buffer, now copy in the data
        //

        pageOffset = (ULONG)StartVa & (PAGE_SIZE-1);
        identityBuffer += pageOffset;
    
        RtlCopyMemory( identityBuffer, StartVa, Length );
        physicalAddress = MmGetPhysicalAddress( identityBuffer );

        *PagesToMap = pagesToMap;
    }

    return physicalAddress;
}



VOID
Ki386ClearIdentityMap(
    IN PIDENTITY_MAP IdentityMap
    )
{
/*++

    This function just frees the page directory and page tables created in 
    Ki386CreateIdentityMap().

--*/

    ULONG index;
    PVOID page;

    //
    // IdentityMap->PageList is an array of addresses of pages allocated with
    // MmAllocateContiguousMemory().  Walk the array, freeing each page.
    // 

    for (index = 0; index < IdentityMap->PagesAllocated; index++) {

        page = IdentityMap->PageList[ index ];
        MmFreeContiguousMemory( page );
    }
}

VOID
Ki386EnableTargetLargePage(
    IN PIDENTITY_MAP IdentityMap
    )
{
/*++

    This function just passes info on to the assembly routine 
    Ki386EnableLargePage().

--*/

    Ki386EnableCurrentLargePage(IdentityMap->IdentityAddr,
                                IdentityMap->IdentityCR3);
}
