/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    MEMORY.C

Abstract:

    This file contains the routines that deal with memory management.

Author:

    Rajen Shah	(rajens)    12-Jul-1991

[Environment:]

    User Mode - Win32, except for NTSTATUS returned by some functions.

Revision History:


--*/

//
// INCLUDES
//

#include <eventp.h>

//
// Implement my own tail-checking, since the system code requires a
// debugging build
//

//#define TAIL_CHECKING
#ifdef TAIL_CHECKING
#define CHECK_HEAP_TAIL_SIZE 16
#define CHECK_HEAP_TAIL_FILL 0xAB
#endif


PVOID
ElfpAllocateBuffer (
    ULONG Size
    )

/*++

Routine Description:

    Allocate a buffer of the given size, and return the pointer in BufPtr.


Arguments:



Return Value:

    Pointer to allocated buffer (or NULL).

Note:


--*/
{
    PVOID	BufPtr;

#ifdef TAIL_CHECKING
    //
    // Keep the offset of the pattern (so we don't have to have internal
    // knowledge about the granularity of the heap block) and copy a
    // known pattern after the end of the user's block
    //
    BufPtr = (PVOID *) MIDL_user_allocate ( Size
        + CHECK_HEAP_TAIL_SIZE + sizeof(DWORD));
    *((PDWORD)BufPtr) = Size + sizeof(DWORD);
    (PBYTE) BufPtr += sizeof(DWORD);
    RtlFillMemory((PBYTE)BufPtr + Size,
                  CHECK_HEAP_TAIL_SIZE,
                  CHECK_HEAP_TAIL_FILL);
#else

    BufPtr = (PVOID *) MIDL_user_allocate ( Size );

#endif

    return (BufPtr);

}



VOID
ElfpFreeBuffer (
    PVOID BufPtr)

/*++

Routine Description:

    Frees a buffer previously allocated by AllocateBuffer.


Arguments:

    Pointer to buffer.

Return Value:

    NOTHING

Note:


--*/
{

#ifdef TAIL_CHECKING

    //
    // Check for NULL when TAIL_CHECKING is enabled since
    // it allows us to avoid checking every pointer that
    // we want to free vs. NULL when we call ElfpFreeBuffer.
    // Note that MIDL_user_free handles NULL.
    //

    if (BufPtr != NULL) {

        DWORD i;
        PBYTE pb;

        //
        // Back up to real start of block
        //

        (PBYTE)BufPtr -= sizeof(DWORD);
        i = *((PDWORD)BufPtr);
        pb = (PBYTE)BufPtr + i;

        for (i = 0; i < CHECK_HEAP_TAIL_SIZE ; i++, pb++) {
            if (*pb != CHECK_HEAP_TAIL_FILL) {
                ElfDbgPrint(("[ELF] Heap has been corrupted at 0x%x\n",
                             BufPtr));
                // Make it access violate
                pb = (PBYTE) 0;
                *pb = 1;
            }
        }
    }

#endif

    MIDL_user_free ( BufPtr );
    return;

}