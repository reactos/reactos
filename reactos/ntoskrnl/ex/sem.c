/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/sem.c
 * PURPOSE:         Synchronization primitives
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)- Reformatting, bug fixes.
 *                  David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

POBJECT_TYPE ExSemaphoreObjectType;

static GENERIC_MAPPING ExSemaphoreMapping = {
    STANDARD_RIGHTS_READ    | SEMAPHORE_QUERY_STATE,
    STANDARD_RIGHTS_WRITE   | SEMAPHORE_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | SEMAPHORE_QUERY_STATE,
    SEMAPHORE_ALL_ACCESS};

static const INFORMATION_CLASS_INFO ExSemaphoreInfoClass[] = {
    
     /* SemaphoreBasicInformation */
    ICI_SQ_SAME( sizeof(SEMAPHORE_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY ),
};

VOID 
INIT_FUNCTION
ExpInitializeSemaphoreImplementation(VOID)
{
    
    /* Create the Semaphore Object */
    ExSemaphoreObjectType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
    RtlInitUnicodeString(&ExSemaphoreObjectType->TypeName, L"Semaphore");
    ExSemaphoreObjectType->Tag = TAG('S', 'E', 'M', 'T');
    ExSemaphoreObjectType->PeakObjects = 0;
    ExSemaphoreObjectType->PeakHandles = 0;
    ExSemaphoreObjectType->TotalObjects = 0;
    ExSemaphoreObjectType->TotalHandles = 0;
    ExSemaphoreObjectType->PagedPoolCharge = 0;
    ExSemaphoreObjectType->NonpagedPoolCharge = sizeof(KSEMAPHORE);
    ExSemaphoreObjectType->Mapping = &ExSemaphoreMapping;
    ExSemaphoreObjectType->Dump = NULL;
    ExSemaphoreObjectType->Open = NULL;
    ExSemaphoreObjectType->Close = NULL;
    ExSemaphoreObjectType->Delete = NULL;
    ExSemaphoreObjectType->Parse = NULL;
    ExSemaphoreObjectType->Security = NULL;
    ExSemaphoreObjectType->QueryName = NULL;
    ExSemaphoreObjectType->OkayToClose = NULL;
    ExSemaphoreObjectType->Create = NULL;
    ExSemaphoreObjectType->DuplicationNotify = NULL;
    ObpCreateTypeObject(ExSemaphoreObjectType);
}

/*
 * @implemented
 */
NTSTATUS 
STDCALL
NtCreateSemaphore(OUT PHANDLE SemaphoreHandle,
                  IN ACCESS_MASK DesiredAccess,
                  IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
                  IN LONG InitialCount,
                  IN LONG MaximumCount)
{
    PKSEMAPHORE Semaphore;
    HANDLE hSemaphore;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
   
    PAGED_CODE();

    /* Check Output Safety */
    if(PreviousMode != KernelMode) {
        
        _SEH_TRY {
            
            ProbeForWrite(SemaphoreHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));
        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {
            
            Status = _SEH_GetExceptionCode();
        
        } _SEH_END;
     
        if(!NT_SUCCESS(Status)) return Status;
    }
    
    /* Make sure the counts make sense */
    if (!MaximumCount || InitialCount < 0 || InitialCount > MaximumCount) {
    
        DPRINT("Invalid Count Data!\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Create the Semaphore Object */
    Status = ObCreateObject(PreviousMode,
                            ExSemaphoreObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(KSEMAPHORE),
                            0,
                            0,
                            (PVOID*)&Semaphore);
    
    /* Check for Success */
    if (NT_SUCCESS(Status)) {
        
        /* Initialize it */
        KeInitializeSemaphore(Semaphore,
                              InitialCount,
                              MaximumCount);
        
        /* Insert it into the Object Tree */
        Status = ObInsertObject((PVOID)Semaphore,
                                NULL,
                                DesiredAccess,
                                0,
                                NULL,
                                &hSemaphore);
        ObDereferenceObject(Semaphore);

        /* Check for success and return handle */
        if(NT_SUCCESS(Status)) {
            
            _SEH_TRY {
                
                *SemaphoreHandle = hSemaphore;
            
            } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {
                
                Status = _SEH_GetExceptionCode();
                
            } _SEH_END;
        }
    }

    /* Return Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS 
STDCALL
NtOpenSemaphore(OUT PHANDLE SemaphoreHandle,
                IN ACCESS_MASK DesiredAccess,
                IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hSemaphore;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
   
    PAGED_CODE();

    /* Check Output Safety */
    if(PreviousMode == UserMode) {
        
        _SEH_TRY {
            
            ProbeForWrite(SemaphoreHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));
        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {
            
            Status = _SEH_GetExceptionCode();
        
        } _SEH_END;
     
        if(!NT_SUCCESS(Status)) return Status;
    }
    
    /* Open the Object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                ExSemaphoreObjectType,
                                NULL,
                                PreviousMode,
                                DesiredAccess,
                                NULL,
                                &hSemaphore);
    
    /* Check for success and return handle */
    if(NT_SUCCESS(Status)) {
            
        _SEH_TRY {
            
            *SemaphoreHandle = hSemaphore;
        
        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {
            
            Status = _SEH_GetExceptionCode();
            
        } _SEH_END;
    }

    /* Return Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
NtQuerySemaphore(IN HANDLE SemaphoreHandle,
                 IN SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
                 OUT PVOID SemaphoreInformation,
                 IN ULONG SemaphoreInformationLength,
                 OUT PULONG ReturnLength  OPTIONAL)
{
    PKSEMAPHORE Semaphore;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    /* Check buffers and class validity */
    DefaultQueryInfoBufferCheck(SemaphoreInformationClass,
                                ExSemaphoreInfoClass,
                                SemaphoreInformation,
                                SemaphoreInformationLength,
                                ReturnLength,
                                PreviousMode,
                                &Status);
    if(!NT_SUCCESS(Status)) {
        
        /* Invalid buffers */
        DPRINT("NtQuerySemaphore() failed, Status: 0x%x\n", Status);
        return Status;
    }
   
    /* Get the Object */
    Status = ObReferenceObjectByHandle(SemaphoreHandle,
                                       SEMAPHORE_QUERY_STATE,
                                       ExSemaphoreObjectType,
                                       PreviousMode,
                                       (PVOID*)&Semaphore,
                                       NULL);
    
    /* Check for success */
    if(NT_SUCCESS(Status)) {
   
        _SEH_TRY {
            
            PSEMAPHORE_BASIC_INFORMATION BasicInfo = (PSEMAPHORE_BASIC_INFORMATION)SemaphoreInformation;
            
            /* Return the basic information */
            BasicInfo->CurrentCount = KeReadStateSemaphore(Semaphore);
            BasicInfo->MaximumCount = Semaphore->Limit;

            /* Return length */
            if(ReturnLength) *ReturnLength = sizeof(SEMAPHORE_BASIC_INFORMATION);
            
        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {
            
            Status = _SEH_GetExceptionCode();
            
        } _SEH_END;
     
        /* Dereference the Object */
        ObDereferenceObject(Semaphore);
   }

   /* Return status */
   return Status;
}

/*
 * @implemented
 */
NTSTATUS 
STDCALL
NtReleaseSemaphore(IN HANDLE SemaphoreHandle,
                   IN LONG ReleaseCount,
                   OUT PLONG PreviousCount  OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PKSEMAPHORE Semaphore;
    NTSTATUS Status = STATUS_SUCCESS; 
   
    PAGED_CODE();
    
    /* Check buffer validity */
    if(PreviousCount != NULL && PreviousMode == UserMode) {
        
        _SEH_TRY {
            
            ProbeForWrite(PreviousCount,
                          sizeof(LONG),
                          sizeof(ULONG));
         } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {
             
            Status = _SEH_GetExceptionCode();
            
        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }
    
    /* Make sure count makes sense */
    if (!ReleaseCount) {
    
        DPRINT("Invalid Release Count\n");
        return STATUS_INVALID_PARAMETER;
    }
   
    /* Get the Object */
    Status = ObReferenceObjectByHandle(SemaphoreHandle,
                                       SEMAPHORE_MODIFY_STATE,
                                       ExSemaphoreObjectType,
                                       PreviousMode,
                                       (PVOID*)&Semaphore,
                                       NULL);
    
    /* Check for success */
    if (NT_SUCCESS(Status)) {
        
        /* Release the semaphore */
        LONG PrevCount = KeReleaseSemaphore(Semaphore,
                                            IO_NO_INCREMENT,
                                            ReleaseCount,
                                            FALSE);
        ObDereferenceObject(Semaphore);
     
        /* Return it */        
        if(PreviousCount) {
            
            _SEH_TRY {
                
                *PreviousCount = PrevCount;
            
            } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {
                
                Status = _SEH_GetExceptionCode();
            
            } _SEH_END;
        }
    }

    /* Return Status */
    return Status;
}

/* EOF */
