/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/critical.c
 * PURPOSE:         Critical sections
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <ntos/synch.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS *****************************************************************/

inline static HANDLE RtlGetCurrentThreadId()
{
   return (HANDLE)NtCurrentTeb()->Cid.UniqueThread;
}

inline static void small_pause(void)
{
#ifdef __i386__
    __asm__ __volatile__( "rep;nop" : : : "memory" );
#else
    __asm__ __volatile__( "" : : : "memory" );
#endif
}

/***********************************************************************
 *           get_semaphore
 */
static inline HANDLE get_semaphore( PCRITICAL_SECTION crit )
{
   HANDLE ret = crit->LockSemaphore;
   if (!ret)
   {
      HANDLE sem;
      if (!NT_SUCCESS(NtCreateSemaphore( &sem, SEMAPHORE_ALL_ACCESS, NULL, 0, 1 ))) return 0;
      if (!(ret = (HANDLE)InterlockedCompareExchangePointer( (PVOID *)&crit->LockSemaphore,
                                                  (PVOID)sem, 0 )))
         ret = sem;
      else
         NtClose(sem);  /* somebody beat us to it */
   }
   return ret;
}

/***********************************************************************
 *           RtlInitializeCriticalSection   (NTDLL.@)
 *
 * Initialises a new critical section.
 *
 * PARAMS
 *  crit [O] Critical section to initialise
 *
 * RETURNS
 *  STATUS_SUCCESS.
 *
 * SEE
 *  RtlInitializeCriticalSectionAndSpinCount(), RtlDeleteCriticalSection(),
 *  RtlEnterCriticalSection(), RtlLeaveCriticalSection(),
 *  RtlTryEnterCriticalSection(), RtlSetCriticalSectionSpinCount()
 */
NTSTATUS STDCALL RtlInitializeCriticalSection( PCRITICAL_SECTION crit )
{
   return RtlInitializeCriticalSectionAndSpinCount( crit, 0 );
}

/***********************************************************************
 *           RtlInitializeCriticalSectionAndSpinCount   (NTDLL.@)
 *
 * Initialises a new critical section with a given spin count.
 *
 * PARAMS
 *   crit      [O] Critical section to initialise
 *   spincount [I] Spin count for crit
 * 
 * RETURNS
 *  STATUS_SUCCESS.
 *
 * NOTES
 *  Available on NT4 SP3 or later.
 *
 * SEE
 *  RtlInitializeCriticalSection(), RtlDeleteCriticalSection(),
 *  RtlEnterCriticalSection(), RtlLeaveCriticalSection(),
 *  RtlTryEnterCriticalSection(), RtlSetCriticalSectionSpinCount()
 */
NTSTATUS STDCALL RtlInitializeCriticalSectionAndSpinCount( PCRITICAL_SECTION crit, ULONG spincount )
{
   /* Does ROS need this, or is this special to Wine? And if ros need it, should
   it be enabled in the release build? -Gunnar */
   if (RtlGetProcessHeap()) crit->DebugInfo = NULL;
   else
   {
      crit->DebugInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(CRITICAL_SECTION_DEBUG));
      if (crit->DebugInfo)
      {
         crit->DebugInfo->Type = 0;
         crit->DebugInfo->CreatorBackTraceIndex = 0;
         crit->DebugInfo->CriticalSection = crit;
         crit->DebugInfo->ProcessLocksList.Blink = &(crit->DebugInfo->ProcessLocksList);
         crit->DebugInfo->ProcessLocksList.Flink = &(crit->DebugInfo->ProcessLocksList);
         crit->DebugInfo->EntryCount = 0;
         crit->DebugInfo->ContentionCount = 0;
         crit->DebugInfo->Spare[0] = 0;
         crit->DebugInfo->Spare[1] = 0;
      }
   }
   crit->LockCount      = -1;
   crit->RecursionCount = 0;
   crit->OwningThread   = 0;
   crit->LockSemaphore  = 0;
   crit->SpinCount      = (NtCurrentPeb()->NumberOfProcessors > 1) ? spincount : 0;
   return STATUS_SUCCESS;
}

/***********************************************************************
 *           RtlSetCriticalSectionSpinCount   (NTDLL.@)
 *
 * Sets the spin count of a critical section.
 *
 * PARAMS
 *   crit      [I/O] Critical section
 *   spincount [I] Spin count for crit
 *
 * RETURNS
 *  The previous spin count.
 *
 * NOTES
 *  If the system is not SMP, spincount is ignored and set to 0.
 *
 * SEE
 *  RtlInitializeCriticalSection(), RtlInitializeCriticalSectionAndSpinCount(),
 *  RtlDeleteCriticalSection(), RtlEnterCriticalSection(),
 *  RtlLeaveCriticalSection(), RtlTryEnterCriticalSection()
 */
ULONG STDCALL RtlSetCriticalSectionSpinCount( PCRITICAL_SECTION crit, ULONG spincount )
{
   ULONG oldspincount = crit->SpinCount;
   crit->SpinCount = (NtCurrentPeb()->NumberOfProcessors > 1) ? spincount : 0;
   return oldspincount;
}

/***********************************************************************
 *           RtlDeleteCriticalSection   (NTDLL.@)
 *
 * Frees the resources used by a critical section.
 *
 * PARAMS
 *  crit [I/O] Critical section to free
 *
 * RETURNS
 *  STATUS_SUCCESS.
 *
 * SEE
 *  RtlInitializeCriticalSection(), RtlInitializeCriticalSectionAndSpinCount(),
 *  RtlDeleteCriticalSection(), RtlEnterCriticalSection(),
 *  RtlLeaveCriticalSection(), RtlTryEnterCriticalSection()
 */
NTSTATUS STDCALL RtlDeleteCriticalSection( PCRITICAL_SECTION crit )
{
   crit->LockCount      = -1;
   crit->RecursionCount = 0;
   crit->OwningThread   = (HANDLE)0;
   if (crit->LockSemaphore) 
      NtClose( crit->LockSemaphore );
   crit->LockSemaphore  = 0;
   if (crit->DebugInfo)
   {
      /* only free the ones we made in here */
      if (!crit->DebugInfo->Spare[1])
      {
         RtlFreeHeap( RtlGetProcessHeap(), 0, crit->DebugInfo );
         crit->DebugInfo = NULL;
      }
   }
   return STATUS_SUCCESS;
}


/***********************************************************************
 *           RtlpWaitForCriticalSection   (NTDLL.@)
 *
 * Waits for a busy critical section to become free.
 * 
 * PARAMS
 *  crit [I/O] Critical section to wait for
 *
 * RETURNS
 *  STATUS_SUCCESS.
 *
 * NOTES
 *  Use RtlEnterCriticalSection() instead of this function as it is often much
 *  faster.
 *
 * SEE
 *  RtlInitializeCriticalSection(), RtlInitializeCriticalSectionAndSpinCount(),
 *  RtlDeleteCriticalSection(), RtlEnterCriticalSection(),
 *  RtlLeaveCriticalSection(), RtlTryEnterCriticalSection()
 */
NTSTATUS STDCALL RtlpWaitForCriticalSection( PCRITICAL_SECTION crit )
{
   for (;;)
   {
      EXCEPTION_RECORD rec;
      HANDLE sem = get_semaphore( crit );
      LARGE_INTEGER time;
      NTSTATUS status;

      time.QuadPart = -5000 * 10000;  /* 5 seconds */
      status = NtWaitForSingleObject( sem, FALSE, &time );
      if ( status == STATUS_TIMEOUT )
      {
         const char *name = NULL;
         if (crit->DebugInfo) name = (char *)crit->DebugInfo->Spare[1];
         if (!name) name = "?";
         DPRINT1( "section %p %s wait timed out in thread %04lx, blocked by %04lx, retrying (60 sec)\n",
              crit, name, RtlGetCurrentThreadId(), (DWORD)crit->OwningThread );
         time.QuadPart = -60000 * 10000;
         status = NtWaitForSingleObject( sem, FALSE, &time );
         if ( status == STATUS_TIMEOUT /*&& TRACE_ON(relay)*/ )
         {
            DPRINT1( "section %p %s wait timed out in thread %04lx, blocked by %04lx, retrying (5 min)\n",
               crit, name, RtlGetCurrentThreadId(), (DWORD) crit->OwningThread );
            time.QuadPart = -300000 * (ULONGLONG)10000;
            status = NtWaitForSingleObject( sem, FALSE, &time );
         }
      }
      if (status == STATUS_WAIT_0) return STATUS_SUCCESS;

      /* Throw exception only for Wine internal locks */
      if ((!crit->DebugInfo) || (!crit->DebugInfo->Spare[1])) continue;

      rec.ExceptionCode    = STATUS_POSSIBLE_DEADLOCK;
      rec.ExceptionFlags   = 0;
      rec.ExceptionRecord  = NULL;
      rec.ExceptionAddress = RtlRaiseException;  /* sic */
      rec.NumberParameters = 1;
      rec.ExceptionInformation[0] = (DWORD)crit;
      RtlRaiseException( &rec );
   }
}


/***********************************************************************
 *           RtlpUnWaitCriticalSection   (NTDLL.@)
 *
 * Notifies other threads waiting on the busy critical section that it has
 * become free.
 * 
 * PARAMS
 *  crit [I/O] Critical section
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any error returned by NtReleaseSemaphore()
 *
 * NOTES
 *  Use RtlLeaveCriticalSection() instead of this function as it is often much
 *  faster.
 *
 * SEE
 *  RtlInitializeCriticalSection(), RtlInitializeCriticalSectionAndSpinCount(),
 *  RtlDeleteCriticalSection(), RtlEnterCriticalSection(),
 *  RtlLeaveCriticalSection(), RtlTryEnterCriticalSection()
 */
NTSTATUS STDCALL RtlpUnWaitCriticalSection( PCRITICAL_SECTION crit )
{
   HANDLE sem = get_semaphore( crit );
   NTSTATUS res = NtReleaseSemaphore( sem, 1, NULL );
   if (!NT_SUCCESS(res)) RtlRaiseStatus( res );
   return res;
}


/***********************************************************************
 *           RtlEnterCriticalSection   (NTDLL.@)
 *
 * Enters a critical section, waiting for it to become available if necessary.
 *
 * PARAMS
 *  crit [I/O] Critical section to enter
 *
 * RETURNS
 *  STATUS_SUCCESS. The critical section is held by the caller.
 *  
 * SEE
 *  RtlInitializeCriticalSection(), RtlInitializeCriticalSectionAndSpinCount(),
 *  RtlDeleteCriticalSection(), RtlSetCriticalSectionSpinCount(),
 *  RtlLeaveCriticalSection(), RtlTryEnterCriticalSection()
 */
NTSTATUS STDCALL RtlEnterCriticalSection( PCRITICAL_SECTION crit )
{
   if (crit->SpinCount)
   {
      ULONG count;

      if (RtlTryEnterCriticalSection( crit )) return STATUS_SUCCESS;
      for (count = crit->SpinCount; count > 0; count--)
      {
         if (crit->LockCount > 0) break;  /* more than one waiter, don't bother spinning */
         if (crit->LockCount == -1)       /* try again */
         {
             if (InterlockedCompareExchange(&crit->LockCount, 0,-1 ) == -1) goto done;
         }
         small_pause();
      }
   }

   if (InterlockedIncrement( &crit->LockCount ))
   {
      if (crit->OwningThread == (HANDLE)RtlGetCurrentThreadId())
      {
         crit->RecursionCount++;
         return STATUS_SUCCESS;
      }

      /* Now wait for it */
      RtlpWaitForCriticalSection( crit );
   }
done:
   crit->OwningThread   = (HANDLE)RtlGetCurrentThreadId();
   crit->RecursionCount = 1;
   return STATUS_SUCCESS;
}


/***********************************************************************
 *           RtlTryEnterCriticalSection   (NTDLL.@)
 *
 * Tries to enter a critical section without waiting.
 *
 * PARAMS
 *  crit [I/O] Critical section to enter
 *
 * RETURNS
 *  Success: TRUE. The critical section is held by the caller.
 *  Failure: FALSE. The critical section is currently held by another thread.
 *
 * SEE
 *  RtlInitializeCriticalSection(), RtlInitializeCriticalSectionAndSpinCount(),
 *  RtlDeleteCriticalSection(), RtlEnterCriticalSection(),
 *  RtlLeaveCriticalSection(), RtlSetCriticalSectionSpinCount()
 */
BOOLEAN STDCALL RtlTryEnterCriticalSection( PCRITICAL_SECTION crit )
{
   BOOL ret = FALSE;
   if (InterlockedCompareExchange(&crit->LockCount, 0L, -1 ) == -1)
   {
      crit->OwningThread   = (HANDLE)RtlGetCurrentThreadId();
      crit->RecursionCount = 1;
      ret = TRUE;
   }
   else if (crit->OwningThread == (HANDLE)RtlGetCurrentThreadId())
   {
      InterlockedIncrement( &crit->LockCount );
      crit->RecursionCount++;
      ret = TRUE;
   }
   return ret;
}


/***********************************************************************
 *           RtlLeaveCriticalSection   (NTDLL.@)
 *
 * Leaves a critical section.
 *
 * PARAMS
 *  crit [I/O] Critical section to leave.
 *
 * RETURNS
 *  STATUS_SUCCESS.
 *
 * SEE
 *  RtlInitializeCriticalSection(), RtlInitializeCriticalSectionAndSpinCount(),
 *  RtlDeleteCriticalSection(), RtlEnterCriticalSection(),
 *  RtlSetCriticalSectionSpinCount(), RtlTryEnterCriticalSection()
 */
NTSTATUS STDCALL RtlLeaveCriticalSection( PCRITICAL_SECTION crit )
{
   if (crit->OwningThread != RtlGetCurrentThreadId())
   {
      DPRINT1("Freeing critical section not owned\n");
   }

   if (--crit->RecursionCount) InterlockedDecrement( &crit->LockCount );
   else
   {
      crit->OwningThread = 0;
      if (InterlockedDecrement( &crit->LockCount ) >= 0)
      {
         /* someone is waiting */
         RtlpUnWaitCriticalSection( crit );
      }
   }
   return STATUS_SUCCESS;
}


/* EOF */
