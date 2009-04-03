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
#include <mmddk.h>

#include <mmebuddy.h>

static HANDLE ProcessHeapHandle = NULL;
static UINT   CurrentAllocations = 0;

/*
    Allocates memory, zeroes it, and increases the allocation count.
*/
PVOID
AllocateMemory(
    IN  UINT Size)
{
    PVOID Pointer = NULL;

    if ( ! ProcessHeapHandle )
        ProcessHeapHandle = GetProcessHeap();

    Pointer = HeapAlloc(ProcessHeapHandle, HEAP_ZERO_MEMORY, Size);

    if ( ! Pointer )
        return NULL;

    ++ CurrentAllocations;

    return Pointer;
}

/*
    Frees memory and reduces the allocation count.
*/
VOID
FreeMemory(
    IN  PVOID Pointer)
{
    SND_ASSERT( ProcessHeapHandle );
    SND_ASSERT( Pointer );

    HeapFree(ProcessHeapHandle, 0, Pointer);

    -- CurrentAllocations;
}

/*
    Returns the current number of memory allocations outstanding. Useful for
    detecting/tracing memory leaks.
*/
UINT
GetMemoryAllocationCount()
{
    return CurrentAllocations;
}


/*
    Count the number of digits in a UINT
*/
UINT
GetDigitCount(
    IN  UINT Number)
{
    UINT Value = Number;
    ULONG Digits = 1;

    while ( Value > 9 )
    {
        Value /= 10;
        ++ Digits;
    }

    return Digits;
}

/*
    Translate a Win32 error code into an MMRESULT code.
*/
MMRESULT
Win32ErrorToMmResult(
    IN  UINT ErrorCode)
{
    switch ( ErrorCode )
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

        default :
            return MMSYSERR_ERROR;
    }
}

/*
    If a function invokes another function, this aids in translating the
    result code so that it is applicable in the context of the original caller.
    For example, specifying that an invalid parameter was passed probably does
    not make much sense if the parameter wasn't passed by the original caller!

    This could potentially highlight internal logic problems.

    However, things like MMSYSERR_NOMEM make sense to return to the caller.
*/
MMRESULT
TranslateInternalMmResult(
    IN  MMRESULT Result)
{
    switch ( Result )
    {
        case MMSYSERR_INVALPARAM :
        case MMSYSERR_INVALFLAG :
        {
            return MMSYSERR_ERROR;
        }
    }

    return Result;
}
