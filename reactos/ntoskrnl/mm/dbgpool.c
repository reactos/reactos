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
    if (Tag == TAG('C', 'a', 'l', 'l')) return FALSE;
    if (Tag == TAG('D', 'r', 'i', 'v')) return FALSE;
    if (Tag == TAG('D', 'e', 'v', 'i')) return FALSE;
    if (Tag == TAG('A', 'd', 'a', 'p')) return FALSE;

    return FALSE;//TRUE;
}


PVOID
NTAPI
ExpAllocateDebugPool(POOL_TYPE Type, ULONG Size, ULONG Tag, PVOID Caller, BOOLEAN EndOfPage)
{
    ULONG UserSize = Size + sizeof(EI_WHOLE_PAGE_HEADER);
    ULONG TotalSize = UserSize + 2*PAGE_SIZE;
    ULONG_PTR UserData, GuardArea;
    PEI_WHOLE_PAGE_HEADER Header;
    ULONG_PTR Buffer;

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
        Buffer = (ULONG_PTR)
            ExAllocateNonPagedPoolWithTag(Type, TotalSize, Tag, Caller);
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
    UserData = GuardArea - Size;
    Header = (PEI_WHOLE_PAGE_HEADER)(UserData - sizeof(EI_WHOLE_PAGE_HEADER));

    /* Fill out the header */
    Header->ActualAddress = (PVOID)Buffer;
    Header->Tag = Tag;
    Header->Size = Size;

    /* Protect the guard page */
    MmSetPageProtect(NULL, (PVOID)GuardArea, PAGE_NOACCESS);

    DPRINT1("Allocating whole page block Tag %c%c%c%c, Buffer %p, Header %p, UserData %p, GuardArea %p, Size %d\n",
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

    DPRINT1("Freeing whole page block at %08x (Tag %c%c%c%c, %x Header %x)\n", Block,
        Header->Tag & 0xFF, (Header->Tag >> 8) & 0xFF,
        (Header->Tag >> 16) & 0xFF, (Header->Tag >> 24) & 0xFF, Header->Tag, Header);

    /* Calculate protected page adresss */
    ProtectedPage = ((PCHAR)Block) + Header->Size;

    /* Unprotect it */
    MmSetPageProtect(NULL, ProtectedPage, PAGE_READWRITE);

    /* Free storage */
    if (PagedPool)
        ExFreePagedPool(Header->ActualAddress);
    else
        ExFreeNonPagedPool(Header->ActualAddress);
}

/* EOF */
