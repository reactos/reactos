/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/iocomp.c
 * PURPOSE:         No purpose listed.
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#define IOC_TAG   TAG('I', 'O', 'C', 'T')   

POBJECT_TYPE ExIoCompletionType;

NPAGED_LOOKASIDE_LIST IoCompletionPacketLookaside;

static GENERIC_MAPPING ExIoCompletionMapping = 
{
    STANDARD_RIGHTS_READ    | IO_COMPLETION_QUERY_STATE,
    STANDARD_RIGHTS_WRITE   | IO_COMPLETION_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | IO_COMPLETION_QUERY_STATE,
    IO_COMPLETION_ALL_ACCESS
};

/* FUNCTIONS *****************************************************************/

VOID 
STDCALL
IopDeleteIoCompletion(PVOID ObjectBody)
{
    PKQUEUE Queue = ObjectBody;
    PLIST_ENTRY FirstEntry;
    PLIST_ENTRY CurrentEntry;
    PIO_COMPLETION_PACKET Packet;

    DPRINT("IopDeleteIoCompletion()\n");

    /* Rundown the Queue */
    FirstEntry = KeRundownQueue(Queue);
    
    /* Clean up the IRPs */
    if (FirstEntry) {
    
        CurrentEntry = FirstEntry;
        do {
        
            /* Get the Packet */
            Packet = CONTAINING_RECORD(CurrentEntry, IO_COMPLETION_PACKET, ListEntry);
            
            /* Go to next Entry */
            CurrentEntry = CurrentEntry->Flink;
            
            /* Free it */
            ExFreeToNPagedLookasideList(&IoCompletionPacketLookaside, Packet);
        } while (FirstEntry != CurrentEntry);
    }
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
IoSetIoCompletion(IN PVOID IoCompletion,
                  IN PVOID KeyContext,
                  IN PVOID ApcContext,
                  IN NTSTATUS IoStatus,
                  IN ULONG_PTR IoStatusInformation,
                  IN BOOLEAN Quota)
{
    PKQUEUE Queue = (PKQUEUE)IoCompletion;
    PIO_COMPLETION_PACKET Packet;

    /* Allocate the Packet */
    Packet = ExAllocateFromNPagedLookasideList(&IoCompletionPacketLookaside);
    if (NULL == Packet) return STATUS_NO_MEMORY;
    
    /* Set up the Packet */
    Packet->Key = KeyContext;
    Packet->Context = ApcContext;
    Packet->IoStatus.Status = IoStatus;
    Packet->IoStatus.Information = IoStatusInformation;
    
    /* Insert the Queue */
    KeInsertQueue(Queue, &Packet->ListEntry);

    /* Return Success */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoSetCompletionRoutineEx(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp,
                         IN PIO_COMPLETION_ROUTINE CompletionRoutine,
                         IN PVOID Context,
                         IN BOOLEAN InvokeOnSuccess,
                         IN BOOLEAN InvokeOnError,
                         IN BOOLEAN InvokeOnCancel)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
FASTCALL
IopInitIoCompletionImplementation(VOID)
{
    /* Create the IO Completion Type */
    ExIoCompletionType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));   
    RtlpCreateUnicodeString(&ExIoCompletionType->TypeName, L"IoCompletion", NonPagedPool);
    ExIoCompletionType->Tag = IOC_TAG;
    ExIoCompletionType->PeakObjects = 0;
    ExIoCompletionType->PeakHandles = 0;
    ExIoCompletionType->TotalObjects = 0;
    ExIoCompletionType->TotalHandles = 0;
    ExIoCompletionType->PagedPoolCharge = 0;
    ExIoCompletionType->NonpagedPoolCharge = sizeof(KQUEUE);
    ExIoCompletionType->Mapping = &ExIoCompletionMapping;
    ExIoCompletionType->Dump = NULL;
    ExIoCompletionType->Open = NULL;
    ExIoCompletionType->Close = NULL;
    ExIoCompletionType->Delete = IopDeleteIoCompletion;
    ExIoCompletionType->Parse = NULL;
    ExIoCompletionType->Security = NULL;
    ExIoCompletionType->QueryName = NULL;
    ExIoCompletionType->OkayToClose = NULL;
    ExIoCompletionType->Create = NULL;
    ExIoCompletionType->DuplicationNotify = NULL;

    /* Initialize the Lookaside List we'll use for packets */
    ExInitializeNPagedLookasideList(&IoCompletionPacketLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(IO_COMPLETION_PACKET),
                                    IOC_TAG,
                                    0);
}

NTSTATUS
STDCALL
NtCreateIoCompletion(OUT PHANDLE IoCompletionHandle,
                     IN  ACCESS_MASK DesiredAccess,
                     IN  POBJECT_ATTRIBUTES ObjectAttributes,
                     IN  ULONG NumberOfConcurrentThreads)
{
    PKQUEUE Queue;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    /* Create the Object */
    Status = ObCreateObject(PreviousMode,
                            ExIoCompletionType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(KQUEUE),
                            0,
                            0,
                            (PVOID*)&Queue);
    
    /* Check for success */
    if (!NT_SUCCESS(Status)) {
   
        /* Initialize the Queue */
        KeInitializeQueue(Queue, NumberOfConcurrentThreads);

        /* Insert it */
        Status = ObInsertObject(Queue,
                                NULL,
                                DesiredAccess,
                                0,
                                NULL,
                                IoCompletionHandle);   
        ObDereferenceObject(Queue);
   }
   
   /* Return Status */
   return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
NtOpenIoCompletion(OUT PHANDLE IoCompletionHandle,
                   IN ACCESS_MASK DesiredAccess,
                   IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    
    /* Open the Object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                ExIoCompletionType,
                                NULL,
                                PreviousMode,
                                DesiredAccess,
                                NULL,
                                IoCompletionHandle);
 
    /* Return Status */  
    return Status;
}

NTSTATUS
STDCALL
NtQueryIoCompletion(IN  HANDLE IoCompletionHandle,
                    IN  IO_COMPLETION_INFORMATION_CLASS IoCompletionInformationClass,
                    OUT PVOID IoCompletionInformation,
                    IN  ULONG IoCompletionInformationLength,
                    OUT PULONG ResultLength OPTIONAL)
{
    PKQUEUE Queue;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    /* Get the Object */
    Status = ObReferenceObjectByHandle(IoCompletionHandle,
                                       IO_COMPLETION_QUERY_STATE,
                                       ExIoCompletionType,
                                       PreviousMode,
                                       (PVOID*)&Queue,
                                       NULL);
    
    /* Check for Success */
   if (NT_SUCCESS(Status)) {
      
        /* Return Info */
        ((PIO_COMPLETION_BASIC_INFORMATION)IoCompletionInformation)->Depth = KeReadStateQueue(Queue);
        ObDereferenceObject(Queue);

        /* Return Result Length if needed */
        if (ResultLength) *ResultLength = sizeof(IO_COMPLETION_BASIC_INFORMATION);
    }

    /* Return Status */
    return Status;
}

/*
 * Dequeues an I/O completion message from an I/O completion object
 */
NTSTATUS
STDCALL
NtRemoveIoCompletion(IN  HANDLE IoCompletionHandle,
                     OUT PVOID *CompletionKey,
                     OUT PVOID *CompletionContext,
                     OUT PIO_STATUS_BLOCK IoStatusBlock,
                     IN  PLARGE_INTEGER Timeout OPTIONAL)
{
    PKQUEUE Queue;
    NTSTATUS Status;
    PIO_COMPLETION_PACKET Packet;
    PLIST_ENTRY ListEntry;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    
    /* Open the Object */
    Status = ObReferenceObjectByHandle(IoCompletionHandle,
                                       IO_COMPLETION_MODIFY_STATE,
                                       ExIoCompletionType,
                                       PreviousMode,
                                       (PVOID*)&Queue,
                                       NULL);
    
    /* Check for success */
    if (NT_SUCCESS(Status)) {

        /* Remove queue */
        ListEntry = KeRemoveQueue(Queue, PreviousMode, Timeout);

        /* If we got a timeout or user_apc back, return the status */
        if ((NTSTATUS)ListEntry == STATUS_TIMEOUT || (NTSTATUS)ListEntry == STATUS_USER_APC) {
            
            Status = (NTSTATUS)ListEntry; 
            
        } else {
            
            /* Get the Packet Data */
            Packet = CONTAINING_RECORD(ListEntry, IO_COMPLETION_PACKET, ListEntry);
            
            /* Return it */
           if (CompletionKey) *CompletionKey = Packet->Key;
           if (CompletionContext) *CompletionContext = Packet->Context;
           if (IoStatusBlock) *IoStatusBlock = Packet->IoStatus;
           
           /* Free packet */
           ExFreeToNPagedLookasideList(&IoCompletionPacketLookaside, Packet);
        }
   
        /* Dereference the Object */
        ObDereferenceObject(Queue);
    }
    
    /* Return status */
    return Status;
}

/*
 * Queues an I/O completion message to an I/O completion object
 */
NTSTATUS
STDCALL
NtSetIoCompletion(IN HANDLE IoCompletionPortHandle,
                  IN PVOID CompletionKey,
                  IN PVOID CompletionContext,
                  IN NTSTATUS CompletionStatus,
                  IN ULONG CompletionInformation)
{
    NTSTATUS Status;
    PKQUEUE Queue;
    
    /* Get the Object */
    Status = ObReferenceObjectByHandle(IoCompletionPortHandle,
                                       IO_COMPLETION_MODIFY_STATE,
                                       ExIoCompletionType,
                                       ExGetPreviousMode(),
                                       (PVOID*)&Queue,
                                       NULL);
    
    /* Check for Success */
    if (NT_SUCCESS(Status)) {
        
        /* Set the Completion */
        Status = IoSetIoCompletion(Queue, 
                                   CompletionKey, 
                                   CompletionContext,
                                   CompletionStatus, 
                                   CompletionInformation, 
                                   TRUE);
        ObDereferenceObject(Queue);
    }
    
    /* Return status */
    return Status;
}
