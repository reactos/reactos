/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/libsupp.c
 * PURPOSE:         Rtl library support routines
 *
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <internal/ps.h>
#define NDEBUG
#include <internal/debug.h>

//FIXME: sort this out somehow. IAI: Sorted in new header branch
#define PRTL_CRITICAL_SECTION PVOID

/* FUNCTIONS *****************************************************************/


KPROCESSOR_MODE
RtlpGetMode()
{
   return KernelMode;
}

/*
 * @implemented
 */
VOID STDCALL
RtlAcquirePebLock(VOID)
{

}

/*
 * @implemented
 */
VOID STDCALL
RtlReleasePebLock(VOID)
{

}


PPEB
STDCALL
RtlpCurrentPeb(VOID)
{
   return ((PEPROCESS)(KeGetCurrentThread()->ApcState.Process))->Peb;
}

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


#ifdef DBG
VOID FASTCALL
CHECK_PAGED_CODE_RTL(char *file, int line)
{
  if(KeGetCurrentIrql() > APC_LEVEL)
  {
    DbgPrint("%s:%i: Pagable code called at IRQL > APC_LEVEL (%d)\n", file, line, KeGetCurrentIrql());
    KEBUGCHECK(0);
  }
}
#endif

/* EOF */
