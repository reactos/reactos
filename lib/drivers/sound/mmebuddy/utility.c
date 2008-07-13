/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/utility.c
 *
 * PURPOSE:     Provides utility functions used by the library.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>

#include <mmebuddy.h>

/*
    Memory
*/

static HANDLE ProcessHeapHandle = INVALID_HANDLE_VALUE;
static DWORD CurrentAllocations = 0;

#if 0
typedef struct _ALLOCATION
{
    DWORD Tag;
    DWORD Size;
} ALLOCATION;

PVOID
AllocateTaggedMemory(
    IN  DWORD Tag,
    IN  DWORD Size)
{
    PVOID Pointer = NULL;

    Size += sizeof(ALLOCATION);

    if ( ProcessHeapHandle == INVALID_HANDLE_VALUE )
        ProcessHeapHandle = GetProcessHeap();

    Pointer = HeapAlloc(ProcessHeapHandle, HEAP_ZERO_MEMORY, Size);

    if ( ! Pointer )
        return NULL;

    /* Store the tag and size */
    ((ALLOCATION*)Pointer)->Tag = Tag;
    ((ALLOCATION*)Pointer)->Size = Size;

    ++ CurrentAllocations;

    return ((PCHAR)Pointer) + sizeof(ALLOCATION);
}

VOID
FreeTaggedMemory(
    IN  DWORD Tag,
    IN  PVOID Pointer)
{
    ALLOCATION* AllocationInfo;

    ASSERT(ProcessHeapHandle != INVALID_HANDLE_VALUE);
    ASSERT(Pointer);

    AllocationInfo = (ALLOCATION*)((PCHAR)Pointer - sizeof(ALLOCATION));

    ASSERT( AllocationInfo->Tag == Tag );

    ZeroMemory(AllocationInfo, AllocationInfo->Size + sizeof(ALLOCATION));
    HeapFree(ProcessHeapHandle, 0, AllocationInfo);

    -- CurrentAllocations;
}
#endif

PVOID
AllocateTaggedMemory(
    IN  DWORD Tag,
    IN  DWORD Size)
{
    PVOID Pointer = NULL;

    if ( ProcessHeapHandle == INVALID_HANDLE_VALUE )
        ProcessHeapHandle = GetProcessHeap();

    Pointer = HeapAlloc(ProcessHeapHandle, HEAP_ZERO_MEMORY, Size);

    if ( ! Pointer )
        return NULL;

    ++ CurrentAllocations;

    return Pointer;
}

VOID
FreeTaggedMemory(
    IN  DWORD Tag,
    IN  PVOID Pointer)
{
    ASSERT(ProcessHeapHandle != INVALID_HANDLE_VALUE);
    ASSERT(Pointer);

    HeapFree(ProcessHeapHandle, 0, Pointer);

    -- CurrentAllocations;
}

DWORD
GetMemoryAllocations()
{
    return CurrentAllocations;
}


/*
    Other
*/

ULONG
GetDigitCount(
    ULONG Number)
{
    ULONG Value = Number;
    ULONG Digits = 1;

    while ( Value > 9 )
    {
        Value /= 10;
        ++ Digits;
    }

    return Digits;
}



/*
    Result codes
*/

MMRESULT
Win32ErrorToMmResult(UINT error_code)
{
    switch ( error_code )
    {
        case NO_ERROR :
        case ERROR_IO_PENDING :
            return MMSYSERR_NOERROR;

        case ERROR_BUSY :
            return MMSYSERR_ALLOCATED;

        case ERROR_NOT_SUPPORTED :
        case ERROR_INVALID_FUNCTION :
            return MMSYSERR_NOTSUPPORTED;

        case ERROR_NOT_ENOUGH_MEMORY :
            return MMSYSERR_NOMEM;

        case ERROR_ACCESS_DENIED :
            return MMSYSERR_BADDEVICEID;

        case ERROR_INSUFFICIENT_BUFFER :
            return MMSYSERR_INVALPARAM;
    };

    /* If all else fails, it's just a plain old error */

    return MMSYSERR_ERROR;
}

/*
    If a function invokes another function, this aids in translating the result
    code so that it is applicable in the context of the original caller. For
    example, specifying that an invalid parameter was passed probably does not
    make much sense if the parameter wasn't passed by the original caller!

    However, things like MMSYSERR_NOMEM make sense to return to the caller.

    This could potentially highlight internal logic problems.
*/
MMRESULT
TranslateInternalMmResult(MMRESULT Result)
{
    switch ( Result )
    {
        case MMSYSERR_INVALPARAM :
        case MMSYSERR_INVALFLAG :
        {
            ERR_("MMRESULT from an internal routine failed with error %d\n",
                 (int) Result);

            return MMSYSERR_ERROR;
        }
    }

    return Result;
}
