/*
* PROJECT:         Filesystem Filter Manager
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            drivers/filters/fltmgr/Filter.c
* PURPOSE:         Handles registration of mini filters
* PROGRAMMERS:     Ged Murphy (gedmurphy@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "fltmgr.h"
#include "fltmgrint.h"
#include "Registry.h"

#define NDEBUG
#include <debug.h>


/* DATA *********************************************************************/

#define SERVICES_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"
#define MAX_KEY_LENGTH  0x200

LIST_ENTRY FilterList;
ERESOURCE FilterListLock;

NTSTATUS
FltpStartingToDrainObject(
    _Inout_ PFLT_OBJECT Object
);

VOID
FltpMiniFilterDriverUnload(
);

NTSTATUS
FltpAttachFrame(
    _In_ PUNICODE_STRING Altitude,
    _Inout_ PFLTP_FRAME *Frame
);

static
NTSTATUS
GetFilterAltitude(
    _In_ PFLT_FILTER Filter,
    _Inout_ PUNICODE_STRING AltitudeString
);

static
NTSTATUS
GetFilterFrame(
    _In_ PFLT_FILTER Filter,
    _In_ PUNICODE_STRING Altitude,
    _Out_ PFLTP_FRAME *Frame
);


/* EXPORTED FUNCTIONS ******************************************************/

NTSTATUS
NTAPI
FltLoadFilter(_In_ PCUNICODE_STRING FilterName)
{
    UNICODE_STRING DriverServiceName;
    UNICODE_STRING ServicesKey;
    CHAR Buffer[MAX_KEY_LENGTH];

    /* Setup the base services key */
    RtlInitUnicodeString(&ServicesKey, SERVICES_KEY);

    /* Initialize the string data */
    DriverServiceName.Length = 0;
    DriverServiceName.Buffer = (PWCH)Buffer;
    DriverServiceName.MaximumLength = MAX_KEY_LENGTH;

    /* Create the full service key for this filter */
    RtlCopyUnicodeString(&DriverServiceName, &ServicesKey);
    RtlAppendUnicodeStringToString(&DriverServiceName, FilterName);

    /* Ask the kernel to load it for us */
    return ZwLoadDriver(&DriverServiceName);
}

NTSTATUS
NTAPI
FltUnloadFilter(_In_ PCUNICODE_STRING FilterName)
{
    //
    //FIXME: This is a temp hack, it needs properly implementing
    //

    UNICODE_STRING DriverServiceName;
    UNICODE_STRING ServicesKey;
    CHAR Buffer[MAX_KEY_LENGTH];

    /* Setup the base services key */
    RtlInitUnicodeString(&ServicesKey, SERVICES_KEY);

    /* Initialize the string data */
    DriverServiceName.Length = 0;
    DriverServiceName.Buffer = (PWCH)Buffer;
    DriverServiceName.MaximumLength = MAX_KEY_LENGTH;

    /* Create the full service key for this filter */
    RtlCopyUnicodeString(&DriverServiceName, &ServicesKey);
    RtlAppendUnicodeStringToString(&DriverServiceName, FilterName);
    return ZwUnloadDriver(&DriverServiceName);
}

NTSTATUS
NTAPI
FltRegisterFilter(_In_ PDRIVER_OBJECT DriverObject,
                  _In_ const FLT_REGISTRATION *Registration,
                  _Out_ PFLT_FILTER *RetFilter)
{
    PFLT_OPERATION_REGISTRATION Callbacks;
    PFLT_FILTER Filter;
    PFLTP_FRAME Frame;
    ULONG CallbackBufferSize;
    ULONG FilterBufferSize;
    ULONG Count = 0;
    PCHAR Ptr;
    NTSTATUS Status;

    *RetFilter = NULL;

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
    FilterBufferSize = sizeof(FLT_FILTER) +
                       CallbackBufferSize +
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

    ExInitializeFastMutex(&Filter->ConnectionList.mLock);
    InitializeListHead(&Filter->ConnectionList.mList);
    Filter->ConnectionList.mCount = 0;

    /* Initialize the usermode port list */
    ExInitializeFastMutex(&Filter->PortList.mLock);
    InitializeListHead(&Filter->PortList.mList);
    Filter->PortList.mCount = 0;

    /* We got this far, assume success from here */
    Status = STATUS_SUCCESS;

    /* Check if the caller requested any context data */
    if (Registration->ContextRegistration)
    {
        /* Register the contexts for this filter */
        Status = FltpRegisterContexts(Filter, Registration->ContextRegistration);
        if (NT_SUCCESS(Status))
        {
            goto Quit;
        }
    }

    /* Check if the caller is registering any callbacks */
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

    /* Lookup the altitude of the mini-filter */
    Status = GetFilterAltitude(Filter, &Filter->DefaultAltitude);
    if (!NT_SUCCESS(Status))
    {
        goto Quit;
    }

    /* Lookup the filter frame */
    Status = GetFilterFrame(Filter, &Filter->DefaultAltitude, &Frame);
    if (Status == STATUS_NOT_FOUND)
    {
        /* Store the frame this mini-filter's main struct */
        Filter->Frame = Frame;

        Status = FltpAttachFrame(&Filter->DefaultAltitude, &Frame);
    }

    if (!NT_SUCCESS(Status))
    {
        goto Quit;
    }

    //
    // - Slot the filter into the correct altitude location
    // - More stuff??
    //

    /* Store any existing driver unload routine before we make any changes */
    Filter->OldDriverUnload = (PFLT_FILTER_UNLOAD_CALLBACK)DriverObject->DriverUnload;

    /* Check we opted not to have an unload routine, or if we want to stop the driver from being unloaded */
    if (Registration->FilterUnloadCallback && !FlagOn(Filter->Flags, FLTFL_REGISTRATION_DO_NOT_SUPPORT_SERVICE_STOP))
    {
        DriverObject->DriverUnload = (PDRIVER_UNLOAD)FltpMiniFilterDriverUnload;
    }
    else
    {
        DriverObject->DriverUnload = (PDRIVER_UNLOAD)NULL;
    }


Quit:

    if (NT_SUCCESS(Status))
    {
        DPRINT1("Loaded FS mini-filter %wZ\n", &DriverObject->DriverExtension->ServiceKeyName);
        *RetFilter = Filter;
    }
    else
    {
        DPRINT1("Failed to load FS mini-filter %wZ : 0x%X\n", &DriverObject->DriverExtension->ServiceKeyName, Status);

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

NTSTATUS
NTAPI
FltStartFiltering(_In_ PFLT_FILTER Filter)
{
    NTSTATUS Status;

    /* Grab a ref to the filter */
    Status = FltObjectReference(&Filter->Base);
    if (NT_SUCCESS(Status))
    {
        /* Make sure we aren't already starting up */
        if (!(Filter->Flags & FLTFL_FILTERING_INITIATED))
        {
            // Startup
        }
        else
        {
            Status = STATUS_INVALID_PARAMETER;
        }

        FltObjectDereference(&Filter->Base);
    }

    return Status;
}

NTSTATUS
NTAPI
FltGetFilterFromName(_In_ PCUNICODE_STRING FilterName,
                     _Out_ PFLT_FILTER *RetFilter)
{
   UNIMPLEMENTED;
    UNREFERENCED_PARAMETER(FilterName);
    *RetFilter = NULL;
    return STATUS_NOT_IMPLEMENTED;
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

VOID
FltpMiniFilterDriverUnload()
{
    __debugbreak();
}


NTSTATUS
FltpAttachFrame(
    _In_ PUNICODE_STRING Altitude,
    _Inout_ PFLTP_FRAME *Frame)
{
    UNIMPLEMENTED;
    UNREFERENCED_PARAMETER(Altitude);
    *Frame = NULL;
    return STATUS_SUCCESS;
}

/* PRIVATE FUNCTIONS ******************************************************/

static
NTSTATUS
GetFilterAltitude(_In_ PFLT_FILTER Filter,
                  _Inout_ PUNICODE_STRING AltitudeString)
{
    UNICODE_STRING InstancesKey = RTL_CONSTANT_STRING(L"Instances");
    UNICODE_STRING DefaultInstance = RTL_CONSTANT_STRING(L"DefaultInstance");
    UNICODE_STRING Altitude = RTL_CONSTANT_STRING(L"Altitude");
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING FilterInstancePath;
    ULONG BytesRequired;
    HANDLE InstHandle = NULL;
    HANDLE RootHandle;
    PWCH InstBuffer = NULL;
    PWCH AltBuffer = NULL;
    NTSTATUS Status;

    /* Get a handle to the instances key in the filter's services key */
    Status = FltpOpenFilterServicesKey(Filter,
                                       KEY_QUERY_VALUE,
                                       &InstancesKey,
                                       &RootHandle);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Read the size 'default instances' string value */
    Status = FltpReadRegistryValue(RootHandle,
                                   &DefaultInstance,
                                   REG_SZ,
                                   NULL,
                                   0,
                                   &BytesRequired);

    /* We should get a buffer too small error */
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        /* Allocate the buffer we need to hold the string */
        InstBuffer = ExAllocatePoolWithTag(PagedPool, BytesRequired, FM_TAG_UNICODE_STRING);
        if (InstBuffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Quit;
        }

        /* Now read the string value */
        Status = FltpReadRegistryValue(RootHandle,
                                       &DefaultInstance,
                                       REG_SZ,
                                       InstBuffer,
                                       BytesRequired,
                                       &BytesRequired);
    }

    if (!NT_SUCCESS(Status))
    {
        goto Quit;
    }

    /* Convert the string to a unicode_string */
    RtlInitUnicodeString(&FilterInstancePath, InstBuffer);

    /* Setup the attributes using the root key handle */
    InitializeObjectAttributes(&ObjectAttributes,
                               &FilterInstancePath,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               RootHandle,
                               NULL);

    /* Now open the key name which was stored in the default instance */
    Status = ZwOpenKey(&InstHandle, KEY_QUERY_VALUE, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Get the size of the buffer that holds the altitude */
        Status = FltpReadRegistryValue(InstHandle,
                                       &Altitude,
                                       REG_SZ,
                                       NULL,
                                       0,
                                       &BytesRequired);
        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            /* Allocate the required buffer */
            AltBuffer = ExAllocatePoolWithTag(PagedPool, BytesRequired, FM_TAG_UNICODE_STRING);
            if (AltBuffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Quit;
            }

            /* And now finally read in the actual altitude string */
            Status = FltpReadRegistryValue(InstHandle,
                                           &Altitude,
                                           REG_SZ,
                                           AltBuffer,
                                           BytesRequired,
                                           &BytesRequired);
            if (NT_SUCCESS(Status))
            {
                /* We made it, setup the return buffer */
                AltitudeString->Length = BytesRequired;
                AltitudeString->MaximumLength = BytesRequired;
                AltitudeString->Buffer = AltBuffer;
            }
        }
    }

Quit:
    if (!NT_SUCCESS(Status))
    {
        if (AltBuffer)
        {
            ExFreePoolWithTag(AltBuffer, FM_TAG_UNICODE_STRING);
        }
    }

    if (InstBuffer)
    {
        ExFreePoolWithTag(InstBuffer, FM_TAG_UNICODE_STRING);
    }

    if (InstHandle)
    {
        ZwClose(InstHandle);
    }
    ZwClose(RootHandle);

    return Status;
}

static
NTSTATUS
GetFilterFrame(_In_ PFLT_FILTER Filter,
               _In_ PUNICODE_STRING Altitude,
               _Out_ PFLTP_FRAME *Frame)
{
    UNIMPLEMENTED;
    UNREFERENCED_PARAMETER(Filter);
    UNREFERENCED_PARAMETER(Altitude);

    //
    // Try to find a frame from our existing filter list (see FilterList)
    // If none exists, create a new frame, add it and return it
    //

    *Frame = NULL;
    return STATUS_SUCCESS;
}
