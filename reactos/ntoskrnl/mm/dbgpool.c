/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/mm/dbgpool.c
 * PURPOSE:         Debug version of a pool allocator
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS **************************************************************/

typedef struct _EI_WHOLE_PAGE_HEADER {
    PVOID ActualAddress;
    ULONG Size;
    ULONG Tag;
} EI_WHOLE_PAGE_HEADER, *PEI_WHOLE_PAGE_HEADER;

BOOLEAN
NTAPI
ExpIsPoolTagDebuggable(ULONG Tag)
{
#if 0
    if (Tag == 'llaC') return FALSE;
    if (Tag == 'virD') return FALSE;
    if (Tag == 'iveD') return FALSE;
    if (Tag == 'padA') return FALSE;

    if (Tag == 'dSeS') return FALSE;
    if (Tag == 'iDbO') return FALSE;
    if (Tag == 'mNbO') return FALSE;
    if (Tag == 'DNbO') return FALSE;
    if (Tag == 'btbO') return FALSE;
    if (Tag == 'cSbO') return FALSE;
    //if (Tag == 'iSeS') return FALSE;
    //if (Tag == 'cAeS') return FALSE;
#endif

    return TRUE;
}


PVOID
NTAPI
ExpAllocateDebugPool(POOL_TYPE Type, ULONG Size, ULONG Tag, PVOID Caller, BOOLEAN EndOfPage)
{
    ULONG UserSize, TotalSize, AlignedSize;
    ULONG_PTR UserData, GuardArea;
    PEI_WHOLE_PAGE_HEADER Header;
    ULONG_PTR Buffer;

    /* Calculate sizes */
    AlignedSize = ROUND_UP(Size, MM_POOL_ALIGNMENT);
    UserSize = AlignedSize + sizeof(EI_WHOLE_PAGE_HEADER);
    TotalSize = UserSize + 2*PAGE_SIZE;

    /* Right now we support only end-of-page allocations */
    ASSERT(EndOfPage);

    /* Allocate space using default routine */
    if (Type & PAGED_POOL_MASK)
    {
        Buffer = (ULONG_PTR)
            ExAllocatePagedPoolWithTag(Type, TotalSize, Tag);
    }
    else
    {
        ASSERT(FALSE);
        return NULL;
    }


    /* If allocation failed - fail too */
    if (!Buffer)
    {
        DPRINT1("A big problem! Pool allocation failed!\n");
        return NULL;
    }

    /* Calculate guard area as placed on a page boundary
     * at the end of allocated area */
    GuardArea = PAGE_ROUND_DOWN(Buffer + TotalSize - PAGE_SIZE + 1);

    /* Calculate user data and header pointers */
    UserData = GuardArea - AlignedSize;
    Header = (PEI_WHOLE_PAGE_HEADER)(UserData - sizeof(EI_WHOLE_PAGE_HEADER));

    /* Fill out the header */
    Header->ActualAddress = (PVOID)Buffer;
    Header->Tag = Tag;
    Header->Size = AlignedSize;

    /* Protect the guard page */
    MmSetPageProtect(NULL, (PVOID)GuardArea, PAGE_NOACCESS);

    DPRINT("Allocating whole page block Tag %c%c%c%c, Buffer %p, Header %p, UserData %p, GuardArea %p, Size %d\n",
        Tag & 0xFF, (Tag >> 8) & 0xFF,
        (Tag >> 16) & 0xFF, (Tag >> 24) & 0xFF,
        Buffer, Header, UserData, GuardArea, Size);

    return (PVOID)UserData;
}

VOID
NTAPI
ExpFreeDebugPool(PVOID Block, BOOLEAN PagedPool)
{
    PEI_WHOLE_PAGE_HEADER Header;
    PVOID ProtectedPage;

    /* Get pointer to our special header */
    Header = (PEI_WHOLE_PAGE_HEADER)
        (((PCHAR)Block) - sizeof(EI_WHOLE_PAGE_HEADER));

    DPRINT("Freeing whole page block at %08x (Tag %c%c%c%c, %x Header %x)\n", Block,
        Header->Tag & 0xFF, (Header->Tag >> 8) & 0xFF,
        (Header->Tag >> 16) & 0xFF, (Header->Tag >> 24) & 0xFF, Header->Tag, Header);

    /* Calculate protected page adresss */
    ProtectedPage = ((PCHAR)Block) + Header->Size;

    /* Unprotect it */
    MmSetPageProtect(NULL, ProtectedPage, PAGE_READWRITE);

    /* Free storage */
    ASSERT(PagedPool);
    ExFreePagedPool(Header->ActualAddress);
}

/* EOF */
