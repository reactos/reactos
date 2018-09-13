/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    cache.c

Abstract:

Author:

    Wesley Witt (wesw) 8-Mar-1992

Environment:

    NT 3.1

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop

typedef unsigned long DWORD;
typedef int           BOOL;

#define NTINCLUDES



#define MIN_READ_SIZE       0x1
#define MIN_CACHE_MISSES    3

ULONG    KdMaxCacheSize    = 100*1024;
ULONG    KdCacheMisses     = 0;
ULONG    KdCachePurges     = 0;
ULONG    KdCacheSize       = 0;
ULONG    KdNodeCount       = 0;
BOOL     KdPurgeOverride   = FALSE;
BOOL     KdCacheDecodePTEs = TRUE;

typedef struct {
    RTL_SPLAY_LINKS     SplayLinks;
    ULONG64             Offset;
    USHORT              Length;
    USHORT              Flags;
    union {
        PUCHAR      Data;
        NTSTATUS    Status;
    } u;
} CACHE, *PCACHE;

#define C_ERROR           0x0001      // Cache of error code
#define C_DONTEXTEND      0x0002      // Don't try to extend
#define C_NONDISCARDABLE  0x0004      // never purge this node

#define LARGECACHENODE    1024        // Size of large cache node


PCACHE  VirtCacheRoot;              // Root of cache node tree


//
// Prototypes...
//

extern void DebugPrint(char *, ...);

BOOL
KdConvertToPhysicalAddr (
    ULONG64 addr,
    PPHYSICAL_ADDRESS pa
    );

PCACHE
CacheLookup (
    ULONG64 Offset,
    ULONG   Length,
    PULONG  LengthUsed
    );

VOID
InsertCacheNode (
    IN PCACHE node
    );

PUCHAR vcmalloc (
    IN ULONG Length
    );

VOID
vcfree (
    IN PUCHAR Memory,
    IN ULONG  Length
    );

NTSTATUS
VCReadTranslatePTEAndReadMemory (
    IN  ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN  ULONG TransferCount,
    OUT PULONG ActualBytesRead OPTIONAL
    );

VOID
DmKdSetMaxCacheSize(
    IN ULONG MaxCacheSize
    )
{
    KdMaxCacheSize = MaxCacheSize;
}


ULONG
DmKdReadCachedVirtualMemory (
    IN ULONG64 BaseAddress,
    IN ULONG TransferCount,
    IN PUCHAR UserBuffer,
    IN PULONG BytesRead,
    IN ULONG NonDiscardable
    )
/*++

    This function returns the specified data from the system being debugged
    using the current mapping of the processor.  If the data is not
    in the cache, it will then be read from the target system.

Arguments:

    TargetBaseAddress - Supplies the base address of the memory to be
        copied into the UserBuffer.

    TransferCount - Amount of data to be copied to the UserBuffer.

    UserBuffer - Address to copy the requested data.

    BytesRead - Number of bytes which could actually be copied

Return Value:

    STATUS_SUCCESS - The specified read occured.

    other  (see DmKdReadVirtualMemoryNow).

--*/
{
    NTSTATUS    st;
    PCACHE      node, node2;
    ULONG       nextlength;
    ULONG       i, br;
    PUCHAR      p;


    *BytesRead = 0;
    if (KdMaxCacheSize == 0) {
        //
        // Cache is off
        //

        goto ReadDirect;
    }

//  DebugPrint( "readcache %I64x %d\n", BaseAddress, TransferCount );

    node = CacheLookup(BaseAddress, TransferCount, &nextlength);
    st = STATUS_SUCCESS;

    while( TRUE ) {
        //
        // Check if current command has been canceled.  If yes, go back to
        // kd prompt.
        //

        if (node == NULL  ||  node->Offset > BaseAddress) {
            //
            // We are missing the leading data, read it into the cache
            //

            if (!node) {
                nextlength = max( nextlength, MIN_READ_SIZE );
            } else {
                //
                // Only get (exactly) enough data to reach neighboring cache
                // node. If an overlapped read occurs between the two nodes,
                // the data will be concatenated then.
                //

                assert((ULONG64)(node->Offset - BaseAddress) <= 0xffffffffUI64);

                nextlength = (ULONG)(node->Offset - BaseAddress);
            }

            p = vcmalloc (nextlength);
            node = (PCACHE) vcmalloc (sizeof (CACHE));

            if (p == NULL  ||  node == NULL) {
                //
                // Out of memory - just read directly to UserBuffer
                //

                if (p) {
                    vcfree (p, nextlength);
                }
                if (node) {
                    vcfree ((PUCHAR)node, sizeof (CACHE));
                }

                goto ReadDirect;
            }

            //
            // Read missing data into cache node
            //

            node->Offset = BaseAddress;
            node->u.Data = p;
            node->Flags  = 0;
            if (NonDiscardable) {
                node->Flags |= C_NONDISCARDABLE;
            }

            KdCacheMisses++;
            while( TRUE ) {

                st = DmKdReadVirtualMemoryNow(
                    BaseAddress,
                    node->u.Data,
                    nextlength,
                    &br
                );

                if (NT_SUCCESS(st)) {
                    break;
                }

                //
                // Before accepting the error, make sure request
                // didn't fail because it was enlarged for caching.
                //

                i = nextlength;
                //nextlength = TransferCount;

                //
                // If length crosses possible page boundary, shrink request
                // even further.
                //

                if ((BaseAddress & ~0xfff) != ((BaseAddress+nextlength) & ~0xfff)) {
                    nextlength = (ULONG)((BaseAddress | 0xfff) - BaseAddress + 1);
                }

                //
                // If nextlength is shorter then failed request, then loop
                // and try again
                //

                if (nextlength >= i) {
                    //
                    // If implicit decode of the pte is requested, go
                    // try getting this memory by it's physical address
                    //
                    if (st == STATUS_UNSUCCESSFUL  &&  KdCacheDecodePTEs) {
                        st = VCReadTranslatePTEAndReadMemory (
                            BaseAddress,
                            node->u.Data,
                            nextlength,
                            &br
                        );
                    }
                    break;
                }
            }

            if (!NT_SUCCESS(st)) {
                //
                // There was an error, cache the error for the starting
                // byte of this range
                //

                vcfree (p, nextlength);
                if (st != STATUS_UNSUCCESSFUL) {
                    //
                    // For now be safe, don't cache this error
                    //

                    vcfree ((PUCHAR)node, sizeof (CACHE));
                    return *BytesRead ? STATUS_SUCCESS : st;
                }

                node->Length = 1;
                node->Flags |= C_ERROR;
                node->u.Status = st;

            } else {

                node->Length = (USHORT) br;
                if (br != nextlength) {
                    //
                    // Some data was not transfered, cache what was returned
                    //

                    node->Flags |= C_DONTEXTEND;
                    KdCacheSize -= (nextlength - br);
                }
            }


            //
            // Insert cache node into splay tree
            //

            InsertCacheNode (node);
        }

        if (node->Flags & C_ERROR) {
            //
            // Hit an error range, we're done
            //

            return *BytesRead ? STATUS_SUCCESS : node->u.Status;
        }

        //
        // Move available data to UserBuffer
        //

        i = (ULONG)(BaseAddress - node->Offset);
        p = node->u.Data + i;
        i = (ULONG) node->Length - i;
        if (TransferCount < i) {
            i = TransferCount;
        }
        memcpy (UserBuffer, p, i);

        TransferCount -= i;
        BaseAddress += i;
        UserBuffer += i;
        *BytesRead += i;

        if (!TransferCount) {
            //
            // All of the users data has been transfered
            //

            return STATUS_SUCCESS;
        }

        //
        // Look for another cache node with more data
        //

        node2 = CacheLookup (BaseAddress, TransferCount, &nextlength);
        if (node2) {
            if ((node2->Flags & C_ERROR) == 0  &&
                node2->Offset == BaseAddress  &&
                node2->Length + node->Length < LARGECACHENODE) {
                //
                // Data is continued in node2, adjoin the neigboring
                // cached data in node & node2 together.
                //

                p = vcmalloc (node->Length + node2->Length);
                if (p != NULL) {
                    memcpy (p, node->u.Data, node->Length);
                    memcpy (p+node->Length, node2->u.Data, node2->Length);
                    vcfree (node->u.Data, node->Length);
                    node->u.Data  = p;
                    node->Length += node2->Length;
                    VirtCacheRoot = (PCACHE) RtlDelete ((PRTL_SPLAY_LINKS)node2);
                    vcfree ((PUCHAR)node2->u.Data, node2->Length);
                    vcfree ((PUCHAR)node2, sizeof (CACHE));
                    KdNodeCount--;
                    continue;
                }
            }

            //
            // Only get enough data to reach the neighboring cache node2
            //

            nextlength = (ULONG)(node2->Offset - BaseAddress);
            if (nextlength == 0) {
                //
                // Data is continued in node2, go get it.
                //

                node = node2;
                continue;
            }

        } else {

            if (node->Length > LARGECACHENODE) {
                //
                // Current cache node is already big enough. Don't extend
                // it, add another cache node.
                //

                node = NULL;
                continue;
            }
        }

        //
        // Extend the current node to include missing data
        //

        if (node->Flags & C_DONTEXTEND) {
            node = NULL;
            continue;
        }

        //nextlength = max( nextlength, MIN_READ_SIZE );

        p = vcmalloc (node->Length + nextlength);
        if (!p) {
            node = NULL;
            continue;
        }

        memcpy (p, node->u.Data, node->Length);
        vcfree (node->u.Data, node->Length);
        node->u.Data = p;

        //
        // Add new data to end of this node
        //

        KdCacheMisses++;
        while( TRUE ) {
            st = DmKdReadVirtualMemoryNow (
                BaseAddress,
                (node->u.Data + node->Length),
                nextlength,
                &br
            );

            if (NT_SUCCESS(st)) {
                break;
            }

            //
            // Before accepting the error, make sure request
            // didn't fail because it was enlarged for caching.
            //

            node->Flags |= C_DONTEXTEND;
            i = TransferCount;

            //
            // If length crosses possible page boundry, shrink request
            // even furture.
            //

            if ((BaseAddress & ~0xfff) != ((BaseAddress + i) & ~0xfff)) {
                i = (ULONG)((BaseAddress | 0xfff) - BaseAddress + 1);
            }

            //
            // If nextlength is shorter, then loop  (try the read again)
            //

            if (i >= nextlength) {
                //
                // If implicit decode of the pte is requested, go
                // try getting this memory by it's physical address
                //
                if (st == STATUS_UNSUCCESSFUL  &&  KdCacheDecodePTEs) {
                    st = VCReadTranslatePTEAndReadMemory (
                        BaseAddress,
                        (node->u.Data + node->Length),
                        nextlength,
                        &br
                    );
                }
                break;
            }

            //
            // Adjust counts for new transfer size
            //

            KdCacheSize -= (nextlength - i);
            nextlength = i;
        }

        if (!NT_SUCCESS(st)) {
            //
            // Return to error to the caller
            //

            node->Flags |= C_DONTEXTEND;
            KdCacheSize -= nextlength;
            return *BytesRead ? STATUS_SUCCESS : st;
        }

        if (br != nextlength) {
            node->Flags |= C_DONTEXTEND;
            KdCacheSize -= (nextlength - br);
        }

        node->Length += (USHORT) br;
        // Loop, and move data to user's buffer
    }

ReadDirect:
    while (TransferCount) {
        nextlength = TransferCount;
        while( TRUE ) {
            st = DmKdReadVirtualMemoryNow (
                BaseAddress,
                UserBuffer,
                nextlength,
                &br
            );

            if (NT_SUCCESS(st)) {
                break;
            }

            if ((BaseAddress & ~0xfff) != ((BaseAddress+nextlength) & ~0xfff)) {
                //
                // Before accepting the error, make sure request
                // didn't fail because it crossed multiple pages
                //

                nextlength = (ULONG)((BaseAddress | 0xfff) - BaseAddress + 1);

            } else {
                if (st == STATUS_UNSUCCESSFUL  &&  KdCacheDecodePTEs) {
                    //
                    // Try getting the memory by looking up the physical
                    // location of the page
                    //

                    st = VCReadTranslatePTEAndReadMemory (
                        BaseAddress,
                        UserBuffer,
                        nextlength,
                        &br
                    );

                    if (NT_SUCCESS(st)) {
                        break;
                    }
                }

                //
                // Return to error to the caller
                //

                return *BytesRead ? STATUS_SUCCESS : st;
            }
        }

        TransferCount -= br;
        BaseAddress += br;
        UserBuffer += br;
        *BytesRead += br;
    }
    return STATUS_SUCCESS;
}


PCACHE
CacheLookup (
    ULONG64 Offset,
    ULONG   Length,
    PULONG  LengthUsed
    )
/*++

Routine Description:

    Walks the cache tree looking for a matching range closest to
    the supplied Offset.  The length of the range searched is based on
    the past length, but may be adjusted slightly.

    This function will always search for the starting byte.

Arguments:

    Offset  - Starting byte being looked for in cache

    Length  - Length of range being looked for in cache

    LengthUsed - Length of range which was really search for

Return Value:

    NULL    - data for returned range was not found
    PCACHE  - leftmost cachenode which has data for returned range


--*/
{
    PCACHE  node, node2;
    ULONG64 SumOffsetLength;

//  DebugPrint( "CacheLookup\n" );

    if (Length < MIN_READ_SIZE  &&  KdCacheMisses > MIN_CACHE_MISSES) {
        // Try to cache more then tiny amount
        Length = MIN_READ_SIZE;
    }

    SumOffsetLength = Offset + Length;
    if (SumOffsetLength < Length) {
        //
        // Offset + Length wrapped.  Adjust Length to be only
        // enough bytes before wrapping.
        //

        Length = (ULONG)(0 - Offset);
        SumOffsetLength = (ULONG64) -1;
    }
    *LengthUsed = Length;

    //
    // Find leftmost cache node for BaseAddress thru BaseAddress+Length
    //

    node2 = NULL;
    node  = VirtCacheRoot;
    while (node != NULL) {
        if (SumOffsetLength <= node->Offset) {
            node = (PCACHE) RtlLeftChild(&node->SplayLinks);
        } else if (node->Offset + node->Length <= Offset) {
            node = (PCACHE) RtlRightChild(&node->SplayLinks);
        } else {
            if (node->Offset <= Offset) {
                //
                // Found starting byte
                //

                return node;
            }

            //
            // Check to see if there's a node which has a match closer
            // to the start of the requested range
            //

            node2  = node;
            Length = (ULONG)(node->Offset - Offset);
            node   = (PCACHE) RtlLeftChild(&node->SplayLinks);
        }
    }

    return node2;
}

VOID
InsertCacheNode (
    IN PCACHE node
    )
{
    PCACHE node2;
    ULONG64 BaseAddress;

    //
    // Insert cache node into splay tree
    //

//  DebugPrint( "insertcache\n" );

    RtlInitializeSplayLinks(&node->SplayLinks);

    KdNodeCount++;
    if (VirtCacheRoot == NULL) {
        VirtCacheRoot = node;
        return;
    }

    node2 = VirtCacheRoot;
    BaseAddress = node->Offset;
    while( TRUE ) {
        if (BaseAddress < node2->Offset) {
            if (RtlLeftChild(&node2->SplayLinks)) {
                node2 = (PCACHE) RtlLeftChild(&node2->SplayLinks);
                continue;
            }
            RtlInsertAsLeftChild(node2, node);
            break;

        } else {
            if (RtlRightChild(&node2->SplayLinks)) {
                node2 = (PCACHE) RtlRightChild(&node2->SplayLinks);
                continue;
            }
            RtlInsertAsRightChild(node2, node);
            break;
        }
    }
    VirtCacheRoot = (PCACHE) RtlSplay((PRTL_SPLAY_LINKS)node2);
}

VOID
DmKdInitVirtualCacheEntry (
    IN ULONG64  BaseAddress,
    IN ULONG  Length,
    IN PUCHAR UserBuffer,
    IN ULONG  NonDiscardable
    )
/*++

Routine Description:

    Insert some data into the virtual cache.

Arguments:

    BaseAddress - Virtual address

    Length      - length to cache

    UserBuffer  - data to put into cache

    NonDiscardable -

Return Value:

--*/
{
    PCACHE  node;
    PUCHAR  p;
    ULONG   LengthUsed;

    if (KdMaxCacheSize == 0) {
        //
        // Cache is off
        //

        return ;
    }

//  DebugPrint( "DmKdInitVirtualCacheEntry\n" );

    node = CacheLookup( BaseAddress, Length, &LengthUsed );
    if (node) {
        return;
    }

    //
    // Delete any cached info which hits range
    //

    DmKdWriteCachedVirtualMemory (BaseAddress, Length, UserBuffer);


    p = vcmalloc (Length);
    node = (PCACHE) vcmalloc (sizeof (CACHE));
    if (p == NULL  ||  node == NULL) {
        //
        // Out of memory - don't bother
        //

        if (p) {
            vcfree (p, Length);
        }
        if (node) {
            vcfree ((PUCHAR)node, sizeof (CACHE));
        }

        return ;
    }

    //
    // Put data into cache node
    //

    node->Offset = BaseAddress;
    node->Length = (USHORT) Length;
    node->u.Data = p;
    node->Flags  = 0;
    if (NonDiscardable) {
        node->Flags |= C_NONDISCARDABLE;
    }
    memcpy (p, UserBuffer, Length);
    InsertCacheNode (node);
}



PUCHAR
vcmalloc (
    IN ULONG Length
    )
/*++

Routine Description:

    Allocates memory for virtual cache, and tracks total memory
    usage.

Arguments:

    Length  - Amount of memory to allocate

Return Value:

    NULL    - too much memory is in use, or memory could not
              be allocated

    Otherwise, returns to address of the allocated memory

--*/
{
    PUCHAR  p;

    if (KdCacheSize + Length > KdMaxCacheSize) {
        return NULL;
    }

    if (!(p = (PUCHAR) malloc (Length))) {
        //
        // Out of memory - don't get any larger
        //

        KdCacheSize = KdMaxCacheSize+1;
        return NULL;
    }

    KdCacheSize += Length;
    return p;
}


VOID
vcfree (
    IN PUCHAR Memory,
    IN ULONG  Length
    )
/*++
Routine Description:

    Free memory allocated with vcmalloc.  Adjusts cache is use totals.

Arguments:

    Memory  - Address of allocated memory

    Length  - Length of allocated memory

Return Value:

    NONE

--*/
{
    KdCacheSize -= Length;
    free (Memory);
}


NTSTATUS
VCReadTranslatePTEAndReadMemory (
    IN  ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN  ULONG TransferCount,
    OUT PULONG ActualBytesRead OPTIONAL
    )
/*++
--*/
{
    static BOOL          ConvertingAnAddress;
    NTSTATUS            status;
    BOOL             converted;
    PHYSICAL_ADDRESS    TargetPhysicalAddress;

    if (ConvertingAnAddress) {
        return  STATUS_UNSUCCESSFUL;
    }

    //
    // Memory could not be read, try its physical address.
    //

    ConvertingAnAddress = TRUE;
    converted = KdConvertToPhysicalAddr (
                    TargetBaseAddress,
                    &TargetPhysicalAddress
                    );

    if (converted) {
        status = DmKdReadPhysicalMemory (
                    TargetPhysicalAddress.QuadPart,
                    UserInterfaceBuffer,
                    TransferCount,
                    ActualBytesRead
                    );
    } else {
        status = STATUS_UNSUCCESSFUL;
    }

    ConvertingAnAddress = FALSE;
    return NT_SUCCESS(status) ? status : STATUS_UNSUCCESSFUL;
}

VOID
DmKdWriteCachedVirtualMemory (
    IN ULONG64 BaseAddress,
    IN ULONG TransferCount,
    IN PUCHAR UserBuffer
    )
/*++

Routine Description:

    Invalidates range from the cache.

Arguments:

    BaseAddress - Starting address to purge

    TransferCount - Length of area to purge

    UserBuffer - not used

Return Value:

    NONE

--*/
{
    PCACHE  node;
    ULONG   bogus;

    //
    // Invalidate any data in the cache which covers this range
    //

    while (node = CacheLookup(BaseAddress, TransferCount, &bogus)) {
        //
        // For now just delete the entire cache node which hits the range
        //

        VirtCacheRoot = (PCACHE) RtlDelete (&node->SplayLinks);
        if (!(node->Flags & C_ERROR)) {
            vcfree (node->u.Data, node->Length);
        }
        vcfree ((PUCHAR)node, sizeof (CACHE));
        KdNodeCount--;
    }
}


PCACHE
WalkForDelete(
    PCACHE node
    )
{
    PCACHE node2;

    if (!(node->Flags & C_NONDISCARDABLE)) {
        return node;
    }

    node2 = (PCACHE)RtlRightChild(&node->SplayLinks);

    if (node2) {
        if (!(node2->Flags & C_NONDISCARDABLE)) {
            return node2;
        } else  {
            node2 = WalkForDelete( node2 );
            if (node2) {
                return node2;
            }
        }
    }

    node2 = (PCACHE)RtlLeftChild(&node->SplayLinks);

    if (node2) {
        if (!(node2->Flags & C_NONDISCARDABLE)) {
            return node2;
        } else  {
            node2 = WalkForDelete( node2 );
            if (node2) {
                return node2;
            }
        }
    }

    return NULL;
}

VOID
DmKdPurgeCachedVirtualMemory (
    BOOL fPurgeNonDiscardable
    )
/*++

Routine Description:

    Purges to entire virtual memory cache

Arguments:

    NONE

Return Value:

    NONE

--*/
{
    PCACHE node;
    PCACHE node2;

    if (KdPurgeOverride) {
        DMPrintShellMsg("** Warning: cache being held\n");
        return;
    }

    node = VirtCacheRoot;
    KdCacheMisses = 0;
    KdCachePurges++;

    if (fPurgeNonDiscardable) {
        while( node ) {
            if (!(node->Flags & C_ERROR)) {
                vcfree (node->u.Data, node->Length);
            }
            KdNodeCount--;
            node = VirtCacheRoot = (PCACHE)RtlDelete((PRTL_SPLAY_LINKS)node);
        }
        KdCacheSize = 0;
        return;
    }

    while( node ) {
        node2 = WalkForDelete( node );
        if (node2) {
            if (!(node2->Flags & C_ERROR)) {
                vcfree (node2->u.Data, node->Length);
            }
            KdNodeCount--;
            node = VirtCacheRoot = (PCACHE)RtlDelete((PRTL_SPLAY_LINKS)node2);
        } else {
            node = node2;
        }
    }

    return;
}

void
WalkForDump(
    PCACHE node
    )
{
    PCACHE l,r;

    r = (PCACHE)RtlRightChild(&node->SplayLinks);
    l = (PCACHE)RtlLeftChild(&node->SplayLinks);

    DMPrintShellMsg( "%08I64x %8d          (%c)\n", node->Offset, node->Length, (node->Flags&C_NONDISCARDABLE) ? 'Y' : 'N' );

    if (r) {
        WalkForDump( r );
    }

    if (l) {
        WalkForDump( l );
    }
}

BOOL
KdConvertToPhysicalAddr64 (
    IN ULONG64              uAddress,
    OUT PPHYSICAL_ADDRESS   PhysicalAddress
    )
{
    return FALSE;
}

BOOL
KdConvertToPhysicalAddr32 (
    IN ULONG                uAddress,
    OUT PPHYSICAL_ADDRESS   PhysicalAddress
    )
/*++

Routine Description:

    Convert a virtual address to a physical one.

    Note: that this function is called from within the virtual memory
    cache code.  This function can read from the virtual memory cache
    so long as it only read's PDE's and PTE's and so long as it fails
    to convert a PDE or PTE virtual address.

Arguments:

    uAddress        - address to convert

    PhysicalAddress - returned physical address

Return Value:

    TRUE - physical address was returned
    otherwise, FALSE

--*/

{
#if defined( TARGET_IA64 )
#define PDE_TOP  0xC07FFFFF
#else
#define PDE_TOP  0xC03FFFFF
#endif // IA64

#if defined( TARGET_i386 )
    #define MM_PTE_PROTOTYPE_MASK       0x400
    #define MM_PTE_TRANSITION_MASK      0x800
    #define PTE_CONT                    0x1
#elif defined( TARGET_ALPHA ) || defined( TARGET_AXP64 )
    #define MM_PTE_PROTOTYPE_MASK       0x2
    #define MM_PTE_TRANSITION_MASK      0x4
    #define PTE_CONT                    0x1
#elif defined( TARGET_IA64 )
    #define MM_PTE_PROTOTYPE_MASK       0x2
    #define MM_PTE_TRANSITION_MASK      0x4
    #define PTE_CONT                    0x1
#endif

#if defined(TARGET_IA64)
#define PDE_SHIFT   (21)
#define PMMPTE ULONG
#define MiGetPdeAddress(va) ((PMMPTE)(((((ULONG)(va)) >> PDE_SHIFT) << PDE_SHIFT) + PDE_BASE))
#define MiGetPteAddress(va) ((PMMPTE)(((((ULONG)(va)) >> PAGE_SHIFT) << PDE_SHIFT) + PTE_BASE))
#else
#define MiGetPdeAddress(va) ((ULONG)(((((ULONG)(va)) >> 22) << 2) + PDE_BASE))
#define MiGetPteAddress(va) ((ULONG)(((((ULONG)(va)) >> 12) << 2) + PTE_BASE))
#endif

    ULONG       Address;
    ULONG       Pte;
    ULONG       Pde;
    ULONG       PdeContents;
    ULONG       PteContents;
    NTSTATUS    status;
    ULONG       result;

    Address = (ULONG) uAddress;
    if (Address >= PTE_BASE  &&  Address < PDE_TOP) {

        //
        // The address is the address of a PTE, rather than
        // a virtual address.  DO NOT CONVERT IT.
        //

        return FALSE;
    }

    Pde = MiGetPdeAddress (Address);
    Pte = MiGetPteAddress (Address);

    status = DmKdReadVirtualMemoryNow(SE32To64(Pde),
                                      &PdeContents,
                                      sizeof(ULONG),
                                      &result);

    if ((status != STATUS_SUCCESS) || (result < sizeof(ULONG))) {
        return FALSE;
    }

    if (!(PdeContents & PTE_CONT)) {
        return FALSE;
    }

    status = DmKdReadVirtualMemoryNow(SE32To64(Pte),
                                      &PteContents,
                                      sizeof(ULONG),
                                      &result);

    if ((status != STATUS_SUCCESS) || (result < sizeof(ULONG))) {
        return FALSE;
    }

    if (!(PteContents & PTE_CONT)) {
        if ( (PteContents & MM_PTE_PROTOTYPE_MASK)  ||
            !(PteContents & MM_PTE_TRANSITION_MASK))  {

            return FALSE;
        }
    }

    //
    // This is a page which is either present or in transition.
    // Return the physical address for the request virtual address.
    //

    PhysicalAddress->LowPart  = (PteContents & ~(0xFFF)) | (Address & 0xFFF);
    PhysicalAddress->QuadPart = SE32To64(PhysicalAddress->LowPart);
    return TRUE;
}

BOOL
KdConvertToPhysicalAddr (
    IN ULONG64              uAddress,
    OUT PPHYSICAL_ADDRESS   PhysicalAddress
    )
{
    if (DmKdPtr64) {
        return KdConvertToPhysicalAddr64(uAddress, PhysicalAddress);
    } else {
        return KdConvertToPhysicalAddr32((ULONG)uAddress, PhysicalAddress);
    }
}

VOID
ProcessCacheCmd(
    LPSTR pchCommand
    )
{
    ULONG       CacheSize;
    ULONG64     Address;


    while (*pchCommand == ' ') {
        pchCommand++;
    }

    _strlwr (pchCommand);

    if (strcmp (pchCommand, "?") == 0) {
usage:
        DMPrintShellMsg("\n.cache [{cachesize} | dump | hold | unhold | decodeptes | nodecodeptes]\n");
        DMPrintShellMsg(".cache [flushall | flushu | flush addr]\n\n");
        return;
    } else
    if (strcmp (pchCommand, "dump") == 0) {
        if (VirtCacheRoot) {
            DMPrintShellMsg("\n Address   Length  Discardable\n");
            WalkForDump( VirtCacheRoot );
        }
    } else
    if (strcmp (pchCommand, "hold") == 0) {
        KdPurgeOverride = TRUE;
    } else
    if (strcmp (pchCommand, "unhold") == 0) {
        KdPurgeOverride = FALSE;
    } else
    if (strcmp (pchCommand, "decodeptes") == 0) {
        DmKdPurgeCachedVirtualMemory(TRUE);
        KdCacheDecodePTEs = TRUE;
    } else
    if (strcmp (pchCommand, "nodecodeptes") == 0) {
        KdCacheDecodePTEs = FALSE;
    } else
    if (strcmp (pchCommand, "flushall") == 0) {
        DmKdPurgeCachedVirtualMemory(TRUE);
    } else
    if (strcmp (pchCommand, "flushu") == 0) {
        DmKdPurgeCachedVirtualMemory(TRUE);
    } else
    if (*pchCommand == 'f') {
        while (*pchCommand >= 'a'  &&  *pchCommand <= 'z') {
            pchCommand++;
        }
        Address = _tcstoul(pchCommand,NULL,0);
        DmKdWriteCachedVirtualMemory (Address, 4096, NULL);  // this is a flush
        DMPrintShellMsg("Cached info for address %I64x for 4096 bytes was flushed\n", Address);
    } else if (*pchCommand) {
        if (*pchCommand < '0'  ||  *pchCommand > '9') {
            goto usage;
        } else {
            CacheSize = _tcstoul(pchCommand,NULL,0);
            KdMaxCacheSize = CacheSize * 1024;
            if (CacheSize == 0) {
                DmKdPurgeCachedVirtualMemory(TRUE);
                KdCachePurges = 0;
            }
        }
    }

    DMPrintShellMsg("\n");
    DMPrintShellMsg("Max cache size is....: %ld %s\n", KdMaxCacheSize,
                    KdMaxCacheSize ? "" : "(cache is off)");
    DMPrintShellMsg("Total memory in cache: %ld\n", KdCacheSize - KdNodeCount * sizeof (CACHE) );
    DMPrintShellMsg("No of regions cached.: %ld\n", KdNodeCount);
    DMPrintShellMsg("Cache misses.........: %ld\n", KdCacheMisses);
    DMPrintShellMsg("Cache purges.........: %ld\n", KdCachePurges);

    if (KdCacheDecodePTEs) {
        DMPrintShellMsg("** Transition ptes are implicity decoded\n");
    }

    if (KdPurgeOverride) {
        DMPrintShellMsg("** Implicit cache flushing disabled **\n");
    }
    DMPrintShellMsg("\n");
}
