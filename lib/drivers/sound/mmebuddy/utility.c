/*
    ReactOS Sound System
    MME Driver Helper

    Purpose:
        Utility functions

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
*/

#include <windows.h>
#include <mmsystem.h>
#include <debug.h>

#include <mmebuddy.h>

/*
    Memory
*/

static HANDLE ProcessHeapHandle = INVALID_HANDLE_VALUE;
static DWORD CurrentAllocations = 0;

PVOID
AllocateMemory(
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
FreeMemory(
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
