/*
* PROJECT:         Filesystem Filter Manager
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            drivers/fs_minifilter/fltmgr/Registration.c
* PURPOSE:         Handles registration of mini filters
* PROGRAMMERS:     Ged Murphy (gedmurphy@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "fltmgr.h"

#define NDEBUG
#include <debug.h>


/* DATA *********************************************************************/


NTSTATUS
FltpStartingToDrainObject(
    _Inout_ PFLT_OBJECT Object
);


/* EXPORTED FUNCTIONS ******************************************************/

NTSTATUS
NTAPI
FltRegisterFilter(_In_ PDRIVER_OBJECT DriverObject,
                  _In_ const FLT_REGISTRATION *Registration,
                  _Out_ PFLT_FILTER *RetFilter)
{
    PFLT_OPERATION_REGISTRATION Callbacks;
    PFLT_FILTER Filter;
    ULONG CallbackBufferSize;
    ULONG FilterBufferSize;
    ULONG Count = 0;
    PCHAR Ptr;
    NTSTATUS Status;

    Status = 0; //remove me

    /* Make sure we're targeting the correct major revision */
    if ((Registration->Version & 0xFF00) != FLT_MAJOR_VERSION)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure our namespace callbacks are valid */
    if ((!Registration->GenerateFileNameCallback && Registration->NormalizeNameComponentCallback) ||
        (!Registration->NormalizeNameComponentCallback && Registration->NormalizeContextCleanupCallback))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Count the number of operations that were requested */
    Callbacks = (PFLT_OPERATION_REGISTRATION)Registration->OperationRegistration;
    while (Callbacks)
    {
        Count++;

        /* Bail when we find the last one */
        if (Callbacks->MajorFunction == IRP_MJ_OPERATION_END)
            break;

        /* Move to the next item */
        Callbacks++;
    }

    /* Calculate the buffer sizes */
    CallbackBufferSize = Count * sizeof(FLT_OPERATION_REGISTRATION);
    FilterBufferSize = sizeof(FLT_FILTER) + CallbackBufferSize +
                       DriverObject->DriverExtension->ServiceKeyName.Length;

    /* Allocate a buffer to hold our filter data */
    Filter = ExAllocatePoolWithTag(NonPagedPool,
                                   FilterBufferSize,
                                   FM_TAG_FILTER);
    if (Filter == NULL) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(Filter, FilterBufferSize);

    /* Find the end of the fixed struct */
    Ptr = (PCHAR)(Filter + 1);

    /* Store a copy of the driver object of this filter */
    Filter->DriverObject = DriverObject;

    /* Initialize the base object data */
    Filter->Base.Flags = FLT_OBFL_TYPE_FILTER;
    Filter->Base.PointerCount = 1;
    FltpExInitializeRundownProtection(&Filter->Base.RundownRef);
    FltObjectReference(&Filter->Base);

    /* Set the callback addresses */
    Filter->FilterUnload = Registration->FilterUnloadCallback;
    Filter->InstanceSetup = Registration->InstanceSetupCallback;
    Filter->InstanceQueryTeardown = Registration->InstanceQueryTeardownCallback;
    Filter->InstanceTeardownStart = Registration->InstanceTeardownStartCallback;
    Filter->InstanceTeardownComplete = Registration->InstanceTeardownCompleteCallback;
    Filter->GenerateFileName = Registration->GenerateFileNameCallback;
    Filter->NormalizeNameComponent = Registration->NormalizeNameComponentCallback;
    Filter->NormalizeContextCleanup = Registration->NormalizeContextCleanupCallback;

    /* Initialize the instance list */
    ExInitializeResourceLite(&Filter->InstanceList.rLock);
    InitializeListHead(&Filter->InstanceList.rList);
    Filter->InstanceList.rCount = 0;

    ExInitializeFastMutex(&Filter->ActiveOpens.mLock);
    InitializeListHead(&Filter->ActiveOpens.mList);
    Filter->ActiveOpens.mCount = 0;

    /* Initialize the usermode port list */
    ExInitializeFastMutex(&Filter->PortList.mLock);
    InitializeListHead(&Filter->PortList.mList);
    Filter->PortList.mCount = 0;

    /* Check if the caller requested any context data */
    if (Registration->ContextRegistration)
    {
        // register the context information
    }

    if (Registration->OperationRegistration)
    {
        /* The callback data comes after the fixed struct */
        Filter->Operations = (PFLT_OPERATION_REGISTRATION)Ptr;
        Ptr += (Count * sizeof(FLT_OPERATION_REGISTRATION));

        /* Tag the operation data onto the end of the filter data */
        RtlCopyMemory(Filter->Operations, Registration->OperationRegistration, CallbackBufferSize);

        /* walk through the requested callbacks */
        for (Callbacks = Filter->Operations;
             Callbacks->MajorFunction != IRP_MJ_OPERATION_END;
             Callbacks++)
        {
            // http://fsfilters.blogspot.co.uk/2011/03/how-file-system-filters-attach-to_17.html
            /* Check if this is an attach to a volume */
            if (Callbacks->MajorFunction == IRP_MJ_VOLUME_MOUNT)
            {
                Filter->PreVolumeMount = Callbacks->PreOperation;
                Filter->PostVolumeMount = Callbacks->PostOperation;
            }
            else if (Callbacks->MajorFunction == IRP_MJ_SHUTDOWN)
            {
                Callbacks->PostOperation = NULL;
            }
        }
    }

    /* Add the filter name buffer onto the end of the data and fill in the string */
    Filter->Name.Length = 0;
    Filter->Name.MaximumLength = DriverObject->DriverExtension->ServiceKeyName.Length;
    Filter->Name.Buffer = (PWCH)Ptr;
    RtlCopyUnicodeString(&Filter->Name, &DriverObject->DriverExtension->ServiceKeyName);

    //
    // - Get the altitude string
    // - Slot the filter into the correct altitude location
    // - More stuff??
    //

//Quit:
    if (!NT_SUCCESS(Status))
    {
        // Add cleanup for context resources

        ExDeleteResourceLite(&Filter->InstanceList.rLock);
        ExFreePoolWithTag(Filter, FM_TAG_FILTER);
    }

    return Status;
}

VOID
FLTAPI
FltUnregisterFilter(_In_ PFLT_FILTER Filter)
{
    PFLT_INSTANCE Instance;
    PLIST_ENTRY CurrentEntry;
    NTSTATUS Status;

    /* Set the draining flag */
    Status = FltpStartingToDrainObject(&Filter->Base);
    if (!NT_SUCCESS(Status))
    {
        /* Someone already unregistered us, just remove our ref and bail */
        FltObjectDereference(&Filter->Base);
        return;
    }

    /* Lock the instance list */
    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&Filter->InstanceList.rLock, TRUE);

    /* Set the first entry in the list */
    CurrentEntry = Filter->InstanceList.rList.Flink;

    /* Free all instances referenced by the filter */
    while (CurrentEntry != &Filter->InstanceList.rList)
    {
        /* Get the record pointer */
        Instance = CONTAINING_RECORD(CurrentEntry, FLT_INSTANCE, FilterLink);

        // FIXME: implement
        (void)Instance;

        /* Reset the pointer and move to next entry */
        Instance = NULL;
        CurrentEntry = CurrentEntry->Flink;
    }

    /* We're done with instances now */
    ExReleaseResourceLite(&Filter->InstanceList.rLock);
    KeLeaveCriticalRegion();

    /* Remove the reference from the base object */
    FltObjectDereference(&Filter->Base);

    /* Wait until we're sure nothing is using the filter */
    FltpObjectRundownWait(&Filter->Base.RundownRef);

    /* Delete the instance list lock */
    ExDeleteResourceLite(&Filter->InstanceList.rLock);

    /* We're finished cleaning up now */
    FltpExRundownCompleted(&Filter->Base.RundownRef);

    /* Hand the memory back */
    ExFreePoolWithTag(Filter, FM_TAG_FILTER);
}


/* INTERNAL FUNCTIONS ******************************************************/

NTSTATUS
FltpStartingToDrainObject(_Inout_ PFLT_OBJECT Object)
{
    /*
     * Set the draining flag for the filter. This let's us force
     * a post op callback for minifilters currently awaiting one.
     */
    if (InterlockedOr((PLONG)&Object->Flags, FLT_OBFL_DRAINING) & 1)
    {
        /* We've been called once, we're already being deleted */
        return STATUS_FLT_DELETING_OBJECT;
    }

    return STATUS_SUCCESS;
}
