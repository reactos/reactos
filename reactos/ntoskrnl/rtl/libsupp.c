/*
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntoskrnl/rtl/libsup.c
 * PURPOSE:         Rtl library support routines
 * UPDATE HISTORY:
 *
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

//FIXME: sort this out somehow. IAI: Sorted in new header branch
#define PRTL_CRITICAL_SECTION PVOID

/* FUNCTIONS *****************************************************************/

NTSTATUS 
STDCALL
RtlDeleteCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    return STATUS_SUCCESS;
}

DWORD
STDCALL
RtlSetCriticalSectionSpinCount(
   PRTL_CRITICAL_SECTION CriticalSection,
   DWORD SpinCount
   )
{
   return 0;
}

NTSTATUS
STDCALL
RtlEnterCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    ExAcquireFastMutex((PFAST_MUTEX) CriticalSection);
    return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
RtlInitializeCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
{
   ExInitializeFastMutex((PFAST_MUTEX)CriticalSection );
   return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
RtlLeaveCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    ExReleaseFastMutex((PFAST_MUTEX) CriticalSection );
    return STATUS_SUCCESS;
}

BOOLEAN 
STDCALL
RtlTryEnterCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    return ExTryToAcquireFastMutex((PFAST_MUTEX) CriticalSection );
}


NTSTATUS
STDCALL
RtlInitializeCriticalSectionAndSpinCount(
    PRTL_CRITICAL_SECTION CriticalSection,
    ULONG SpinCount)
{
    ExInitializeFastMutex((PFAST_MUTEX)CriticalSection );
    return STATUS_SUCCESS;
}

/* EOF */
