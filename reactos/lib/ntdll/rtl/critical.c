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

#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS *****************************************************************/

#define MAX_STATIC_CS_DEBUG_OBJECTS 64

static RTL_CRITICAL_SECTION RtlCriticalSectionLock;
static LIST_ENTRY RtlCriticalSectionList;
static BOOLEAN RtlpCritSectInitialized = FALSE;
static RTL_CRITICAL_SECTION_DEBUG RtlpStaticDebugInfo[MAX_STATIC_CS_DEBUG_OBJECTS];
static BOOLEAN RtlpDebugInfoFreeList[MAX_STATIC_CS_DEBUG_OBJECTS];

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
        
    DPRINT("Deleting Critical Section: %x\n", CriticalSection);
    /* Close the Event Object Handle if it exists */
    if (CriticalSection->LockSemaphore) {
        
        /* In case NtClose fails, return the status */
        Status = NtClose(CriticalSection->LockSemaphore);
        
    }
        
    /* Protect List */
    RtlEnterCriticalSection(&RtlCriticalSectionLock);
    
    /* Remove it from the list */
    RemoveEntryList(&CriticalSection->DebugInfo->ProcessLocksList);      
        
    /* Unprotect */
    RtlLeaveCriticalSection(&RtlCriticalSectionLock);
        
    /* Free it */
    RtlpFreeDebugInfo(CriticalSection->DebugInfo);
    
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
        else if (CriticalSection->OwningThread == (HANDLE)0)
        {
            /* No one else owns it either! */
            DPRINT1("Critical section not initialized (guess)!\n");
            /* FIXME: raise exception */
            return STATUS_INVALID_PARAMETER;            
        }
        
        /* We don't own it, so we must wait for it */
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
    
    /* First things first, set up the Object */
    DPRINT("Initializing Critical Section: %x\n", CriticalSection);
    CriticalSection->LockCount = -1;
    CriticalSection->RecursionCount = 0;
    CriticalSection->OwningThread = 0;
    CriticalSection->SpinCount = (NtCurrentPeb()->NumberOfProcessors > 1) ? SpinCount : 0;
    CriticalSection->LockSemaphore = 0;

    /* Allocate the Debug Data */
    CritcalSectionDebugData = RtlpAllocateDebugInfo();    
    DPRINT("Allocated Debug Data: %x inside Process: %x\n", 
           CritcalSectionDebugData, 
           NtCurrentTeb()->Cid.UniqueProcess);
    
    if (!CritcalSectionDebugData) {
        
        /* This is bad! */
        DPRINT1("Couldn't allocate Debug Data for: %x\n", CriticalSection);
        return STATUS_NO_MEMORY;
    }
    
    /* Set it up */
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
        
        DPRINT("Securely Inserting into ProcessLocks: %x, %x, %x\n", 
               &CritcalSectionDebugData->ProcessLocksList,
               CriticalSection,
               &RtlCriticalSectionList);
        
        /* Protect List */
        RtlEnterCriticalSection(&RtlCriticalSectionLock);

        /* Add this one */
        InsertTailList(&RtlCriticalSectionList, &CritcalSectionDebugData->ProcessLocksList);

        /* Unprotect */
        RtlLeaveCriticalSection(&RtlCriticalSectionLock);
    
    } else {

        DPRINT("Inserting into ProcessLocks: %x, %x, %x\n", 
               &CritcalSectionDebugData->ProcessLocksList,
               CriticalSection,
               &RtlCriticalSectionList);
        
        /* Add it directly */
        InsertTailList(&RtlCriticalSectionList, &CritcalSectionDebugData->ProcessLocksList);
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
    HANDLE Thread = (HANDLE)NtCurrentTeb()->Cid.UniqueThread;
    
    if (Thread != CriticalSection->OwningThread)
    {
       DPRINT1("Releasing critical section not owned!\n");
       /* FIXME: raise exception */
       return STATUS_INVALID_PARAMETER;
    }
   
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
    
    /* Wait 2.5 minutes */
    Timeout.QuadPart = 150000L * (ULONGLONG)10000;
    Timeout.QuadPart = -Timeout.QuadPart;
    /* ^^ HACK HACK HACK. Good way:
    Timeout = &NtCurrentPeb()->CriticalSectionTimeout   */
    
    /* Do we have an Event yet? */
    if (!CriticalSection->LockSemaphore) {
        RtlpCreateCriticalSectionSem(CriticalSection);
    }
    
    /* Increase the Debug Entry count */
    DPRINT("Waiting on Critical Section Event: %x %x\n", 
            CriticalSection, 
            CriticalSection->LockSemaphore);
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
                
                DPRINT1("Deadlock: %x\n", CriticalSection);
                
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
    DPRINT("Signaling Critical Section Event: %x, %x\n", 
            CriticalSection, 
            CriticalSection->LockSemaphore);
    Status = NtSetEvent(CriticalSection->LockSemaphore, NULL);
            
    if (!NT_SUCCESS(Status)) {
        
        /* We've failed */
        DPRINT1("Signaling Failed for: %x, %x, %x\n", 
                CriticalSection, 
                CriticalSection->LockSemaphore,
		Status);
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
        DPRINT("Created Event: %x \n", hNewEvent);
        
        if ((hEvent = InterlockedCompareExchangePointer((PVOID*)&CriticalSection->LockSemaphore,
                                                         (PVOID)hNewEvent,
                                                         0))) {
            
            /* Some just created an event */
            DPRINT("Closing already created event: %x\n", hNewEvent);
            NtClose(hNewEvent);
        }
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

/*++
 * RtlpAllocateDebugInfo 
 *
 *     Finds or allocates memory for a Critical Section Debug Object
 *
 * Params:
 *     None
 *
 * Returns:
 *     A pointer to an empty Critical Section Debug Object.
 *
 * Remarks:
 *     For optimization purposes, the first 64 entries can be cached. From
 *     then on, future Critical Sections will allocate memory from the heap.
 *
 *--*/
PRTL_CRITICAL_SECTION_DEBUG
STDCALL
RtlpAllocateDebugInfo(
    VOID)
{
    ULONG i;
    
    /* Try to allocate from our buffer first */
    for (i = 0; i < MAX_STATIC_CS_DEBUG_OBJECTS; i++) {
    
        /* Check if Entry is free */
        if (!RtlpDebugInfoFreeList[i]) {
            
            /* Mark entry in use */
            DPRINT("Using entry: %d. Buffer: %x\n", i, &RtlpStaticDebugInfo[i]);
            RtlpDebugInfoFreeList[i] = TRUE;
        
            /* Use free entry found */
            return &RtlpStaticDebugInfo[i];
        }
        
    }
         
    /* We are out of static buffer, allocate dynamic */
    return RtlAllocateHeap(NtCurrentPeb()->ProcessHeap, 
                           0, 
                           sizeof(RTL_CRITICAL_SECTION_DEBUG));
}

/*++
 * RtlpFreeDebugInfo 
 *
 *     Frees the memory for a Critical Section Debug Object
 *
 * Params:
 *     DebugInfo - Pointer to Critical Section Debug Object to free.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     If the pointer is part of the static buffer, then the entry is made
 *     free again. If not, the object is de-allocated from the heap.
 *
 *--*/
VOID
STDCALL
RtlpFreeDebugInfo(
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo)
{
    ULONG EntryId;
    
    /* Is it part of our cached entries? */
    if ((DebugInfo >= RtlpStaticDebugInfo) && 
        (DebugInfo <= &RtlpStaticDebugInfo[MAX_STATIC_CS_DEBUG_OBJECTS-1])) {
    
        /* Yes. zero it out */
        RtlZeroMemory(DebugInfo, sizeof(RTL_CRITICAL_SECTION_DEBUG));
        
        /* Mark as free */
        EntryId = (DebugInfo - RtlpStaticDebugInfo);
        DPRINT("Freeing from Buffer: %x. Entry: %d inside Process: %x\n", 
               DebugInfo, 
               EntryId,
               NtCurrentTeb()->Cid.UniqueProcess);
        RtlpDebugInfoFreeList[EntryId] = FALSE;

    } else {
    
        /* It's a dynamic one, so free from the heap */
        DPRINT("Freeing from Heap: %x inside Process: %x\n", 
               DebugInfo, 
               NtCurrentTeb()->Cid.UniqueProcess);
        RtlFreeHeap(NtCurrentPeb()->ProcessHeap, 0, DebugInfo);

    }
}

/* EOF */
