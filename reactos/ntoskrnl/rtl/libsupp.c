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

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

//FIXME: sort this out somehow
#define PCRITICAL_SECTION PVOID
#define LPCRITICAL_SECTION PVOID

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID STDCALL
RtlDeleteCriticalSection(PCRITICAL_SECTION CriticalSection)
{
}

/*
 * @implemented
 */
DWORD STDCALL
RtlSetCriticalSectionSpinCount(
   LPCRITICAL_SECTION CriticalSection,
   DWORD SpinCount
   )
{
   return 0;
}


/*
 * @implemented
 */
VOID STDCALL
RtlEnterCriticalSection(PCRITICAL_SECTION CriticalSection)
{
   ExAcquireFastMutex((PFAST_MUTEX) CriticalSection );
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlInitializeCriticalSection(PCRITICAL_SECTION CriticalSection)
{
   ExInitializeFastMutex((PFAST_MUTEX)CriticalSection );
   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID STDCALL
RtlLeaveCriticalSection(PCRITICAL_SECTION CriticalSection)
{
   ExReleaseFastMutex((PFAST_MUTEX) CriticalSection );
}

/*
 * @implemented
 */
BOOLEAN STDCALL
RtlTryEnterCriticalSection(PCRITICAL_SECTION CriticalSection)
{
  return ExTryToAcquireFastMutex((PFAST_MUTEX) CriticalSection );
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlInitializeCriticalSectionAndSpinCount (
   PCRITICAL_SECTION CriticalSection,
	ULONG SpinCount)
{
   ExInitializeFastMutex((PFAST_MUTEX)CriticalSection );
   return STATUS_SUCCESS;
}

/* EOF */
