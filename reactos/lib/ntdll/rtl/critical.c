/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/critical.c
 * PURPOSE:         Critical sections
 * UPDATE HISTORY:
 *                  Created 30/09/98
 *                  Rewritten ROS version, based on WINE code plus
 *                  some fixes useful only for ROS right now - 03/01/05
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <ntos/synch.h>

//#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS *****************************************************************/

static RTL_CRITICAL_SECTION RtlCriticalSectionLock;
static LIST_ENTRY RtlCriticalSectionList;
static BOOLEAN RtlpCritSectInitialized = FALSE;

/*++
 * RtlDeleteCriticalSection 
 * @implemented NT4
 *
 *     Deletes a Critical Section
 *
 * Params:
 *     CriticalSection - Critical section to delete.
 *
 * Returns:
 *     STATUS_SUCCESS, or error value returned by NtClose.
 *
 * Remarks:
 *     The critical section members should not be read after this call.
 *
 *--*/
NTSTATUS
STDCALL
RtlDeleteCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    NTSTATUS Status = STATUS_SUCCESS;
        
    /* Close the Event Object Handle if it exists */
    if (CriticalSection->LockSemaphore) {
        Status = NtClose(CriticalSection->LockSemaphore);
    }
        
    /* Protect List */
    RtlEnterCriticalSection(&RtlCriticalSectionLock);
               
    /* Delete the Debug Data, if it exists */
    if (CriticalSection->DebugInfo) {
        
        /* Remove it from the list */
        RemoveEntryList(&CriticalSection->DebugInfo->ProcessLocksList);
        
        /* Free it */
        RtlFreeHeap(RtlGetProcessHeap(), 0, CriticalSection->DebugInfo);
    }
    
    /* Unprotect */
    RtlLeaveCriticalSection(&RtlCriticalSectionLock);
    
    /* Wipe it out */
    RtlZeroMemory(CriticalSection, sizeof(RTL_CRITICAL_SECTION));
    
    /* Return */
    return Status;
}

/*++
 * RtlSetCriticalSectionSpinCount 
 * @implemented NT4
 *
 *     Sets the spin count for a critical section.
 *
 * Params:
 *     CriticalSection - Critical section to set the spin count for.
 *
 *     SpinCount - Spin count for the critical section.
 *
 * Returns:
 *     STATUS_SUCCESS.
 *
 * Remarks:
 *     SpinCount is ignored on single-processor systems.
 *
 *--*/
DWORD
STDCALL
RtlSetCriticalSectionSpinCount(
   PRTL_CRITICAL_SECTION CriticalSection,
   DWORD SpinCount
   )
{    
    DWORD OldCount = CriticalSection->SpinCount;
    
    /* Set to parameter if MP, or to 0 if this is Uniprocessor */
    CriticalSection->SpinCount = (NtCurrentPeb()->NumberOfProcessors > 1) ? SpinCount : 0;
    return OldCount;
}

/*++
 * RtlEnterCriticalSection 
 * @implemented NT4
 *
 *     Waits to gain ownership of the critical section.
 *
 * Params:
 *     CriticalSection - Critical section to wait for.
 *
 * Returns:
 *     STATUS_SUCCESS.
 *
 * Remarks:
 *     Uses a fast-path unless contention happens.
 *
 *--*/
NTSTATUS
STDCALL
RtlEnterCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    HANDLE Thread = (HANDLE)NtCurrentTeb()->Cid.UniqueThread;
    
    /* Try to Lock it */
    if (InterlockedIncrement(&CriticalSection->LockCount)) {

        /* 
         * We've failed to lock it! Does this thread
         * actually own it?
         */
        if (Thread == CriticalSection->OwningThread) {
            
            /* You own it, so you'll get it when you're done with it! */
            CriticalSection->RecursionCount++;
            return STATUS_SUCCESS;
        }
        
        /* We don't own it, so we must wait for it */
        DPRINT ("Waiting\n");
        RtlpWaitForCriticalSection(CriticalSection);
    }
    
    /* Lock successful */
    CriticalSection->OwningThread = Thread;
    CriticalSection->RecursionCount = 1;
    return STATUS_SUCCESS;
}

/*++
 * RtlInitializeCriticalSection 
 * @implemented NT4
 *
 *     Initialises a new critical section.
 *
 * Params:
 *     CriticalSection - Critical section to initialise
 *
 * Returns:
 *     STATUS_SUCCESS.
 *
 * Remarks:
 *     Simply calls RtlInitializeCriticalSectionAndSpinCount
 *
 *--*/
NTSTATUS 
STDCALL
RtlInitializeCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
{ 
    /* Call the Main Function */
    return RtlInitializeCriticalSectionAndSpinCount(CriticalSection, 0);
}

/*++
 * RtlInitializeCriticalSectionAndSpinCount 
 * @implemented NT4
 *
 *     Initialises a new critical section.
 *
 * Params:
 *     CriticalSection - Critical section to initialise
 *
 *     SpinCount - Spin count for the critical section.
 *
 * Returns:
 *     STATUS_SUCCESS.
 *
 * Remarks:
 *     SpinCount is ignored on single-processor systems.
 *
 *--*/
NTSTATUS
STDCALL
RtlInitializeCriticalSectionAndSpinCount (
    PRTL_CRITICAL_SECTION CriticalSection,
    DWORD SpinCount)
{
    PRTL_CRITICAL_SECTION_DEBUG CritcalSectionDebugData;
    PVOID Heap;
    
    /* First things first, set up the Object */
    CriticalSection->LockCount = -1;
    CriticalSection->RecursionCount = 0;
    CriticalSection->OwningThread = 0;
    CriticalSection->SpinCount = (NtCurrentPeb()->NumberOfProcessors > 1) ? SpinCount : 0;

    /* 
     * Now set up the Debug Data 
     * Think of a better way to allocate it, because the Heap Manager won't
     * have any debug data since the Heap isn't initalized! 
     */
    if ((Heap = RtlGetProcessHeap())) {

        CritcalSectionDebugData = RtlAllocateHeap(Heap, 0, sizeof(RTL_CRITICAL_SECTION_DEBUG));
        CritcalSectionDebugData->Type = RTL_CRITSECT_TYPE;
        CritcalSectionDebugData->ContentionCount = 0;
        CritcalSectionDebugData->EntryCount = 0;
        CritcalSectionDebugData->CriticalSection = CriticalSection;
        CriticalSection->DebugInfo = CritcalSectionDebugData;

        /* 
         * Add it to the List of Critical Sections owned by the process.
         * If we've initialized the Lock, then use it. If not, then probably
         * this is the lock initialization itself, so insert it directly.
         */
        if ((CriticalSection != &RtlCriticalSectionLock) && (RtlpCritSectInitialized)) {
            
            /* Protect List */
            RtlEnterCriticalSection(&RtlCriticalSectionLock);

            /* Add this one */
            InsertTailList(&RtlCriticalSectionList, &CritcalSectionDebugData->ProcessLocksList);

            /* Unprotect */
            RtlLeaveCriticalSection(&RtlCriticalSectionLock);
        
        } else {

            /* Add it directly */
            InsertTailList(&RtlCriticalSectionList, &CritcalSectionDebugData->ProcessLocksList);
        }
        
    } else {

        /* This shouldn't happen... */
        CritcalSectionDebugData = NULL;
    }

    return STATUS_SUCCESS;
}

/*++
 * RtlLeaveCriticalSection 
 * @implemented NT4
 *
 *     Releases a critical section and makes if available for new owners.
 *
 * Params:
 *     CriticalSection - Critical section to release.
 *
 * Returns:
 *     STATUS_SUCCESS.
 *
 * Remarks:
 *     If another thread was waiting, the slow path is entered.
 *
 *--*/
NTSTATUS
STDCALL
RtlLeaveCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
{     
    /* Decrease the Recursion Count */    
    if (--CriticalSection->RecursionCount) {
    
        /* Someone still owns us, but we are free */
        InterlockedDecrement(&CriticalSection->LockCount);
        
    } else {
        
         /* Nobody owns us anymore */
        CriticalSection->OwningThread = 0;
        
        /* Was someone wanting us? */
        if (InterlockedDecrement(&CriticalSection->LockCount) >= 0) {
        
            /* Let him have us */
            RtlpUnWaitCriticalSection(CriticalSection);
        
        }
    }
    
    /* Sucessful! */
    return STATUS_SUCCESS;
}

/*++
 * RtlTryEnterCriticalSection 
 * @implemented NT4
 *
 *     Attemps to gain ownership of the critical section without waiting.
 *
 * Params:
 *     CriticalSection - Critical section to attempt acquiring.
 *
 * Returns:
 *     TRUE if the critical section has been acquired, FALSE otherwise.
 *
 * Remarks:
 *     None
 *
 *--*/
BOOLEAN 
STDCALL
RtlTryEnterCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
{   
    /* Try to take control */
    if (InterlockedCompareExchange(&CriticalSection->LockCount,
                                   0,
                                   -1) == -1) {

        /* It's ours */                                
        CriticalSection->OwningThread =  NtCurrentTeb()->Cid.UniqueThread;
        CriticalSection->RecursionCount = 1;
        return TRUE;
   
   } else if (CriticalSection->OwningThread == NtCurrentTeb()->Cid.UniqueThread) {
       
        /* It's already ours */
        InterlockedIncrement(&CriticalSection->LockCount);
        CriticalSection->RecursionCount++;
        return TRUE;
    }
    
    /* It's not ours */
    return FALSE;
}

/*++
 * RtlpWaitForCriticalSection 
 *
 *     Slow path of RtlEnterCriticalSection. Waits on an Event Object.
 *
 * Params:
 *     CriticalSection - Critical section to acquire.
 *
 * Returns:
 *     STATUS_SUCCESS, or raises an exception if a deadlock is occuring.
 *
 * Remarks:
 *     None
 *
 *--*/
NTSTATUS
STDCALL
RtlpWaitForCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    NTSTATUS Status;
    EXCEPTION_RECORD ExceptionRecord;
    BOOLEAN LastChance = FALSE;
    LARGE_INTEGER Timeout;
    
    Timeout = RtlConvertLongToLargeInteger(150000);
    /* ^^ HACK HACK HACK. Good way:
    Timeout = &NtCurrentPeb()->CriticalSectionTimeout   */
    
    /* Do we have an Event yet? */
    if (!CriticalSection->LockSemaphore) {
        RtlpCreateCriticalSectionSem(CriticalSection);
    }
    
    /* Increase the Debug Entry count */
    CriticalSection->DebugInfo->EntryCount++;
    
    for (;;) {
    
        /* Increase the number of times we've had contention */
        CriticalSection->DebugInfo->ContentionCount++;
        
        /* Wait on the Event */
        Status = NtWaitForSingleObject(CriticalSection->LockSemaphore,
                                       FALSE,
                                       &Timeout);
    
        /* We have Timed out */
        if (Status == STATUS_TIMEOUT) {
            
            /* Is this the 2nd time we've timed out? */
            if (LastChance) {
                
                /* Yes it is, we are raising an exception */
                ExceptionRecord.ExceptionCode    = STATUS_POSSIBLE_DEADLOCK;
                ExceptionRecord.ExceptionFlags   = 0;
                ExceptionRecord.ExceptionRecord  = NULL;
                ExceptionRecord.ExceptionAddress = RtlRaiseException;
                ExceptionRecord.NumberParameters = 1;
                ExceptionRecord.ExceptionInformation[0] = (ULONG_PTR)CriticalSection;
                RtlRaiseException(&ExceptionRecord);
            
            } 
                
            /* One more try */
            LastChance = TRUE;
        
        } else {
        
            /* If we are here, everything went fine */
            return STATUS_SUCCESS;
        }
    }
}

/*++
 * RtlpUnWaitCriticalSection 
 *
 *     Slow path of RtlLeaveCriticalSection. Fires an Event Object.
 *
 * Params:
 *     CriticalSection - Critical section to release.
 *
 * Returns:
 *     None. Raises an exception if the system call failed.
 *
 * Remarks:
 *     None
 *
 *--*/
VOID
STDCALL
RtlpUnWaitCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
    
{
    NTSTATUS Status;
    
    /* Do we have an Event yet? */
    if (!CriticalSection->LockSemaphore) {
        RtlpCreateCriticalSectionSem(CriticalSection);
    }
    
    /* Signal the Event */
    Status = NtSetEvent(CriticalSection->LockSemaphore, NULL);
            
    if (!NT_SUCCESS(Status)) {
        
        /* We've failed */
        RtlRaiseStatus(Status);
    }
}

/*++
 * RtlpCreateCriticalSectionSem 
 *
 *     Checks if an Event has been created for the critical section.
 *
 * Params:
 *     None
 *
 * Returns:
 *     None. Raises an exception if the system call failed.
 *
 * Remarks:
 *     None
 *
 *--*/
VOID
STDCALL
RtlpCreateCriticalSectionSem(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    HANDLE hEvent = CriticalSection->LockSemaphore;
    HANDLE hNewEvent;
    NTSTATUS Status;
 
    /* Chevk if we have an event */
    if (!hEvent) {
        
        DPRINT ("Creating Event\n");
        /* No, so create it */
        if (!NT_SUCCESS(Status = NtCreateEvent(&hNewEvent,
                                               EVENT_ALL_ACCESS,
                                               NULL,
                                               SynchronizationEvent,
                                               FALSE))) {
                
                /* We failed, this is bad... */
                DPRINT1("Failed to Create Event!\n");
                InterlockedDecrement(&CriticalSection->LockCount);
                RtlRaiseStatus(Status);
                return;
        }
        
        if (!(hEvent = InterlockedCompareExchangePointer((PVOID*)&CriticalSection->LockSemaphore,
                                                         (PVOID)hNewEvent,
                                                         0))) {
            
            /* We created a new event succesffuly */             
            hEvent = hNewEvent;
        } else {
        
            /* Some just created an event */
            DPRINT("Closing already created event!\n");
            NtClose(hNewEvent);
        }
        
        /* Set either the new or the old */
        CriticalSection->LockSemaphore = hEvent;
        DPRINT("Event set!\n");
    }
    
    return;
}

/*++
 * RtlpInitDeferedCriticalSection 
 *
 *     Initializes the Critical Section implementation.
 *
 * Params:
 *     None
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     After this call, the Process Critical Section list is protected.
 *
 *--*/
VOID
STDCALL
RtlpInitDeferedCriticalSection(
    VOID)
{
           
    /* Initialize the Process Critical Section List */
    InitializeListHead(&RtlCriticalSectionList);
    
    /* Initialize the CS Protecting the List */
    RtlInitializeCriticalSection(&RtlCriticalSectionLock);
    
    /* It's now safe to enter it */
    RtlpCritSectInitialized = TRUE;
}

/* EOF */
