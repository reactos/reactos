/*
 * PROJECT:     ReactOS Storport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Storport driver main file
 * COPYRIGHT:   Copyright 2017 Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"
#include "inline.h"
#include "intsafe.h"
#include "wdm.h"

#define NDEBUG
#include <debug.h>
#include <stdarg.h>


/* GLOBALS ********************************************************************/

ULONG PortNumber = 0;


/* FUNCTIONS ******************************************************************/

static
NTSTATUS
PortAddDriverInitData(
    PDRIVER_OBJECT_EXTENSION DriverExtension,
    PHW_INITIALIZATION_DATA HwInitializationData)
{
    PDRIVER_INIT_DATA InitData;

    DPRINT1("PortAddDriverInitData()\n");

    InitData = ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(DRIVER_INIT_DATA),
                                     TAG_INIT_DATA);
    if (InitData == NULL)
        return STATUS_NO_MEMORY;

    RtlCopyMemory(&InitData->HwInitData,
                  HwInitializationData,
                  sizeof(HW_INITIALIZATION_DATA));

    InsertHeadList(&DriverExtension->InitDataListHead,
                   &InitData->Entry);

    return STATUS_SUCCESS;
}


static
VOID
PortDeleteDriverInitData(
    PDRIVER_OBJECT_EXTENSION DriverExtension)
{
    PDRIVER_INIT_DATA InitData;
    PLIST_ENTRY ListEntry;

    DPRINT1("PortDeleteDriverInitData()\n");

    ListEntry = DriverExtension->InitDataListHead.Flink;
    while (ListEntry != &DriverExtension->InitDataListHead)
    {
        InitData = CONTAINING_RECORD(ListEntry,
                                     DRIVER_INIT_DATA,
                                     Entry);

        RemoveEntryList(&InitData->Entry);

        ExFreePoolWithTag(InitData,
                          TAG_INIT_DATA);

        ListEntry = DriverExtension->InitDataListHead.Flink;
    }
}


PHW_INITIALIZATION_DATA
PortGetDriverInitData(
    PDRIVER_OBJECT_EXTENSION DriverExtension,
    INTERFACE_TYPE InterfaceType)
{
    PDRIVER_INIT_DATA InitData;
    PLIST_ENTRY ListEntry;

    DPRINT1("PortGetDriverInitData()\n");

    ListEntry = DriverExtension->InitDataListHead.Flink;
    while (ListEntry != &DriverExtension->InitDataListHead)
    {
        InitData = CONTAINING_RECORD(ListEntry,
                                     DRIVER_INIT_DATA,
                                     Entry);
        if (InitData->HwInitData.AdapterInterfaceType == InterfaceType)
            return &InitData->HwInitData;

        ListEntry = ListEntry->Flink;
    }

    return NULL;
}


static
VOID
PortAcquireSpinLock(
    PFDO_DEVICE_EXTENSION DeviceExtension,
    STOR_SPINLOCK SpinLock,
    PVOID LockContext,
    PSTOR_LOCK_HANDLE LockHandle)
{
    DPRINT("PortAcquireSpinLock(%p %lu %p %p)\n",
            DeviceExtension, SpinLock, LockContext, LockHandle);

    LockHandle->Lock = SpinLock;

    switch (SpinLock)
    {
        case DpcLock: /* 1, */
            DPRINT("DpcLock\n");
            break;

        case StartIoLock: /* 2 */
            DPRINT("StartIoLock\n");
            break;

        case InterruptLock: /* 3 */
            DPRINT("InterruptLock\n");
            if (DeviceExtension->Interrupt == NULL)
                LockHandle->Context.OldIrql = 0;
            else
                LockHandle->Context.OldIrql = KeAcquireInterruptSpinLock(DeviceExtension->Interrupt);
            break;
    }
}


static
VOID
PortReleaseSpinLock(
    PFDO_DEVICE_EXTENSION DeviceExtension,
    PSTOR_LOCK_HANDLE LockHandle)
{
    DPRINT("PortReleaseSpinLock(%p %p)\n",
            DeviceExtension, LockHandle);

    switch (LockHandle->Lock)
    {
        case DpcLock: /* 1, */
            DPRINT("DpcLock\n");
            break;

        case StartIoLock: /* 2 */
            DPRINT("StartIoLock\n");
            break;

        case InterruptLock: /* 3 */
            DPRINT("InterruptLock\n");
            if (DeviceExtension->Interrupt != NULL)
                KeReleaseInterruptSpinLock(DeviceExtension->Interrupt,
                                           LockHandle->Context.OldIrql);
            break;
    }
}


static
VOID
PortScheduleComingRequestsOnCompletion(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PFDO_DEVICE_EXTENSION FdoExtension)
{
    PLIST_ENTRY Entry;
    PQUEUED_REQUEST_REFERENCE RequestReference;

    /* Sanity check again whether FDO has resource to handle the request */
    NT_ASSERT(FdoExtension->FlowControl.OutstandingRequestCount <
              FdoExtension->OutstandingRequestMax);

    /* If FDO has waiting requests, issue one. */
    if (!IsListEmpty(&FdoExtension->FlowControl.FdoBlockedRequestsListHead))
    {
        /* Sanity check */
        NT_ASSERT(FdoExtension->FlowControl.OutstandingRequestCount <
                  FdoExtension->OutstandingRequestMax);

        /* Take it out of FDO list */
        Entry = RemoveTailList(&FdoExtension->FlowControl.FdoBlockedRequestsListHead);
        RequestReference = CONTAINING_RECORD(Entry, QUEUED_REQUEST_REFERENCE, FdoEntry);

        /* Increment FDO request counter */
        ++FdoExtension->FlowControl.OutstandingRequestCount;
        NT_ASSERT(FdoExtension->FlowControl.OutstandingRequestCount <=
                  FdoExtension->OutstandingRequestMax);

        /* Call the start routine */
        PortPdoIssueRequest(PdoExtension, RequestReference);

        /* Our task is done */
        return;
    }

    /* FDO's got no request, check the PDO of the just-finished request for awaiting requests */
    if (!IsListEmpty(&PdoExtension->FlowControl.AwaitingRequestsListHead))
    {
        /* Sanity check */
        NT_ASSERT(PdoExtension->FlowControl.OutstandingRequestCount <
                  PdoExtension->OutstandingRequestMax);
        
        /* Take one request */
        Entry = RemoveTailList(&PdoExtension->FlowControl.AwaitingRequestsListHead);
        RequestReference = CONTAINING_RECORD(Entry, QUEUED_REQUEST_REFERENCE, PdoEntry);

        /* This one goes to active list */
        InsertHeadList(&PdoExtension->FlowControl.ActiveRequestsListHead, Entry);

        /* Increment PDO request counter */
        ++PdoExtension->FlowControl.OutstandingRequestCount;
        NT_ASSERT(PdoExtension->FlowControl.OutstandingRequestCount <=
                  PdoExtension->OutstandingRequestMax);

        /* FIXME: should be false. Allocate resource if it is determined required */
        if (RequestReference->Srb->SrbExtension == NULL)
        {
            __debugbreak();
        }

        /* Call the start routine */
        PortPdoIssueRequest(PdoExtension, RequestReference);

        /* Our task is done */
        return;
    }
}


static
VOID
PortCompleteRequest(
    _In_ PQUEUED_REQUEST_REFERENCE RequestReference)
{
    PSCSI_REQUEST_BLOCK Srb;
    PIRP Irp;
    PFDO_DEVICE_EXTENSION FdoExtension;
    PPDO_DEVICE_EXTENSION PdoExtension;
    KLOCK_QUEUE_HANDLE LockHandle;
    NTSTATUS IoStatus;

    /* Take out our information */
    Srb = RequestReference->Srb;
    Irp = RequestReference->Irp;
    PdoExtension = RequestReference->PdoExtension;
    FdoExtension = PdoExtension->FdoExtension;

    /* FIXME: DELETE AFTER DEBUG */
    // if (RequestReference->DumpSpecialRequest)
    // {
    //     StorpDumpRequestResponse(RequestReference);
    // }

    DPRINT1("Completing request: (%d:%d:%d) ReqRef %p Srb %p Irp %p Tag %x\n",
            Srb->PathId, Srb->TargetId, Srb->Lun, RequestReference, Srb, RequestReference->Irp, Srb->QueueTag);

    /* Return SG list resources back to system */
    if (RequestReference->ScatterGatherList)
    {
        FdoExtension->DmaAdapter->DmaOperations->
            PutScatterGatherList(FdoExtension->DmaAdapter,
                                 (PSCATTER_GATHER_LIST)RequestReference->ScatterGatherList,
                                 RequestReference->WriteToDevice);
        RequestReference->ScatterGatherList = NULL;

        // /* Release pages we locked for DMA, if we actually have locked it */
        // if (RequestReference->MappedSystemVa != NULL)
        // {
        //     MmUnmapLockedPages(RequestReference->MappedSystemVa, RequestReference->Irp->MdlAddress);
        // }
        /* Unlock the pages locked for DMA */
        if (RequestReference->MappedSystemVa)
        {
            MmUnlockPages(RequestReference->Irp->MdlAddress);
        }
    }

    /* Take the request out of FDO and PDO structures, and update flow control data */
    KeAcquireInStackQueuedSpinLock(&FdoExtension->FlowControl.Lock, &LockHandle);
    StorpCheckFlowControl(FdoExtension);

    FdoExtension->FlowControl.OutstandingRequestCount--;
    if (FdoExtension->FlowControl.RemainingBusyRequests)
    {
        --FdoExtension->FlowControl.RemainingBusyRequests;

        /* Clear busy state as needed */
        if (FdoExtension->FlowControl.OutstandingRequestCount == 0 ||
            FdoExtension->FlowControl.RemainingBusyRequests == 0)
        {
            FdoExtension->FlowControl.IsBusy = FALSE;
        }
    }

    RemoveEntryList(&RequestReference->PdoEntry);
    --PdoExtension->FlowControl.OutstandingRequestCount;
    if (PdoExtension->FlowControl.RemainingBusyRequests)
    {
        --PdoExtension->FlowControl.RemainingBusyRequests;

        /* Clear busy state as needed */
        if (PdoExtension->FlowControl.OutstandingRequestCount == 0 ||
            PdoExtension->FlowControl.RemainingBusyRequests == 0)
        {
            PdoExtension->FlowControl.IsBusy = FALSE;
        }
    }

    /* Decrement strong ordered request count if needed */
    if (RequestReference->StrongOrdered)
    {
        --PdoExtension->FlowControl.StrongOrderedCount;
    }

    /* Free the private data structure. No one should use it anymore */
    StorpSrbFreeRequestReference(Srb);
    RequestReference = NULL;

    /* Schedule next IO before we release flow control lock */
    PortScheduleComingRequestsOnCompletion(PdoExtension, FdoExtension);

    /* We're done, release flow control lock */
    StorpCheckFlowControl(FdoExtension);
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    /*
     * Free SRB extension as this request will never go back to miniport again. Must free it before
     * IoCompleteRequest because class driver's completion routine may issue another request. If we
     * free it after IoCompleteRequest we may zero out the newly-issued SRB's extension, since one
     * single SRB can be reused between requests, and the SRB issued in completion routine could be
     * the same one we operating here.
     */
    if (Srb->SrbExtension)
    {
        ExFreePoolWithTag(Srb->SrbExtension, TAG_SRB_EXTENSION);
        Srb->SrbExtension = NULL;
    }

    /*
     * After done with internal accountings, we can finally complete this IO.
     *
     * We must not complete the IO before scheduling next IO, because at that point the flow control
     * lock is still held, and class driver's completion routine may issue another request. This
     * will cause a recursive lock acquisition in PDO SCSI handler and lead to bugcheck.
     */
    switch (Srb->SrbStatus) {
        case SRB_STATUS_SUCCESS: IoStatus = STATUS_SUCCESS; break;
        case SRB_STATUS_BUSY: IoStatus = STATUS_DEVICE_BUSY; break;
        default: IoStatus = STATUS_UNSUCCESSFUL; // FIXME:
    }
    Irp->IoStatus.Status = IoStatus;
    Irp->IoStatus.Information = Srb->DataTransferLength; /* FIXME: hopefully this is correct */

    DPRINT1("Calling IoCompleteRequest\n");
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}


static
VOID
NTAPI
PortCompletionDpc(
    _In_ PKDPC Dpc,
    _In_ PVOID Context,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    PQUEUED_REQUEST_REFERENCE RequestReference;
    PFDO_DEVICE_EXTENSION FdoExtension;
    PSLIST_ENTRY Entry;
    PSLIST_ENTRY Next;
    ULONG Count = 0;

    DPRINT("PortCompletionDpc(%p %p %p %p)\n", Dpc, Context, SystemArgument1, SystemArgument2);

    /* Take context pointer as FDO extension */
    FdoExtension = (PFDO_DEVICE_EXTENSION)Context;

    /* Flush all entries in the list and complete all requests */
    Entry = InterlockedFlushSList(&FdoExtension->CompletionList);
    while (Entry)
    {
        /* Prepare Next pointer first (Completion routine will free the memory that contains it) */
        Next = Entry->Next;

        /* Get original request */
        RequestReference = CONTAINING_RECORD(Entry, QUEUED_REQUEST_REFERENCE, CompletionEntry);

        /* Call real completion routine */
        PortCompleteRequest(RequestReference);

        /* Move on to next one */
        Entry = Next;
        ++Count;
    }

    DPRINT1("Completed %d requests\n", Count);
}


static
NTSTATUS
NTAPI
PortAddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDRIVER_OBJECT_EXTENSION DriverObjectExtension;
    PFDO_DEVICE_EXTENSION DeviceExtension = NULL;
    WCHAR NameBuffer[80];
    UNICODE_STRING DeviceName;
    PDEVICE_OBJECT Fdo = NULL;
    NTSTATUS Status;

    DPRINT1("PortAddDevice(%p %p)\n",
            DriverObject, PhysicalDeviceObject);

    ASSERT(DriverObject);
    ASSERT(PhysicalDeviceObject);

    _swprintf(NameBuffer,
              L"\\Device\\RaidPort%lu",
              PortNumber);
    RtlInitUnicodeString(&DeviceName, NameBuffer);
    PortNumber++;

    DPRINT1("Creating device: %wZ\n", &DeviceName);

    /* Create the port device */
    Status = IoCreateDevice(DriverObject,
                            sizeof(FDO_DEVICE_EXTENSION),
                            &DeviceName,
                            FILE_DEVICE_CONTROLLER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Fdo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateDevice() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    DPRINT1("Created device: %wZ (%p)\n", &DeviceName, Fdo);

    /* Initialize the device */
    Fdo->Flags |= DO_DIRECT_IO;
    Fdo->Flags |= DO_POWER_PAGABLE;

    /* Initialize the device extension */
    DeviceExtension = (PFDO_DEVICE_EXTENSION)Fdo->DeviceExtension;
    RtlZeroMemory(DeviceExtension, sizeof(FDO_DEVICE_EXTENSION));

    DeviceExtension->ExtensionType = FdoExtension;

    DeviceExtension->Device = Fdo;
    DeviceExtension->PhysicalDevice = PhysicalDeviceObject;

    DeviceExtension->PnpState = dsStopped;

    DeviceExtension->ScsiPortNumber = -1;

    KeInitializeSpinLock(&DeviceExtension->PdoListLock);
    InitializeListHead(&DeviceExtension->PdoListHead);

    KeInitializeSpinLock(&DeviceExtension->TimerListLock);
    InitializeListHead(&DeviceExtension->TimerListHead);

    /* Initialize FDO Device DPC for delayed completion */
    KeInitializeDpc(&Fdo->Dpc, PortCompletionDpc, DeviceExtension);

    /* Initialize completion list header */
    InitializeSListHead(&DeviceExtension->CompletionList);

    /* Initialize FDO flow control structure */
    KeInitializeSpinLock(&DeviceExtension->FlowControl.Lock);
    DeviceExtension->FlowControl.IsBusy = FALSE;
    DeviceExtension->FlowControl.IsPaused = FALSE;
    DeviceExtension->FlowControl.RemainingBusyRequests = 0;
    DeviceExtension->FlowControl.RemainingPauseTime = 0;
    DeviceExtension->FlowControl.OutstandingRequestCount = 0;
    InitializeListHead(&DeviceExtension->FlowControl.FdoBlockedRequestsListHead);

    /* Attach the FDO to the device stack */
    Status = IoAttachDeviceToDeviceStackSafe(Fdo,
                                             PhysicalDeviceObject,
                                             &DeviceExtension->LowerDevice);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoAttachDeviceToDeviceStackSafe() failed (Status 0x%08lx)\n", Status);
        IoDeleteDevice(Fdo);
        return Status;
    }

    /* Insert the FDO to the drivers FDO list */
    DriverObjectExtension = IoGetDriverObjectExtension(DriverObject,
                                                       (PVOID)DriverEntry);
    ASSERT(DriverObjectExtension->ExtensionType == DriverExtension);

    DeviceExtension->DriverExtension = DriverObjectExtension;

    StorpInterlockedInsertHeadListCounted(&DriverObjectExtension->AdapterListLock,
                                          &DriverObjectExtension->AdapterListHead,
                                          &DeviceExtension->AdapterListEntry,
                                          &DriverObjectExtension->AdapterCount);

    /* The device has been initialized */
    Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    DPRINT1("PortAddDevice() done (Status 0x%08lx)\n", Status);

    return Status;
}


static
VOID
NTAPI
PortUnload(
    _In_ PDRIVER_OBJECT DriverObject)
{
    PDRIVER_OBJECT_EXTENSION DriverExtension;

    DPRINT1("PortUnload(%p)\n",
            DriverObject);

    DriverExtension = IoGetDriverObjectExtension(DriverObject,
                                                 (PVOID)DriverEntry);
    if (DriverExtension != NULL)
    {
        PortDeleteDriverInitData(DriverExtension);
    }
}


static
NTSTATUS
NTAPI
PortDispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    DPRINT("PortDispatchCreate(%p %p)\n",
            DeviceObject, Irp);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = FILE_OPENED;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


static
NTSTATUS
NTAPI
PortDispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    DPRINT("PortDispatchClose(%p %p)\n",
            DeviceObject, Irp);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


static
NTSTATUS
NTAPI
PortDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;

    DPRINT("PortDispatchDeviceControl(%p %p)\n",
            DeviceObject, Irp);

    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    DPRINT("ExtensionType: %u\n", DeviceExtension->ExtensionType);

    switch (DeviceExtension->ExtensionType)
    {
        case FdoExtension:
            return PortFdoDeviceControl(DeviceObject,
                                        Irp);

        case PdoExtension:
            return PortPdoDeviceControl(DeviceObject,
                                        Irp);

        default:
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


static
NTSTATUS
NTAPI
PortDispatchScsi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;

    DPRINT("PortDispatchScsi(%p %p)\n",
            DeviceObject, Irp);

    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    DPRINT("ExtensionType: %u\n", DeviceExtension->ExtensionType);

    switch (DeviceExtension->ExtensionType)
    {
        case FdoExtension:
            return PortFdoScsi(DeviceObject,
                               Irp);

        case PdoExtension:
            return PortPdoScsi(DeviceObject,
                               Irp);

        default:
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


static
NTSTATUS
NTAPI
PortDispatchSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    DPRINT1("PortDispatchSystemControl(%p %p)\n",
            DeviceObject, Irp);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


static
NTSTATUS
NTAPI
PortDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;

    DPRINT1("PortDispatchPnp(%p %p)\n",
            DeviceObject, Irp);

    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    DPRINT("ExtensionType: %u\n", DeviceExtension->ExtensionType);

    switch (DeviceExtension->ExtensionType)
    {
        case FdoExtension:
            return PortFdoPnp(DeviceObject,
                              Irp);

        case PdoExtension:
            return PortPdoPnp(DeviceObject,
                              Irp);

        default:
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_UNSUCCESSFUL;
    }
}


static
NTSTATUS
NTAPI
PortDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    DPRINT1("PortDispatchPower(%p %p)\n",
            DeviceObject, Irp);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    DPRINT1("DriverEntry(%p %p)\n", DriverObject, RegistryPath);
    return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
STORPORT_API
PUCHAR
NTAPI
StorPortAllocateRegistryBuffer(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Length)
{
    DPRINT1("StorPortAllocateRegistryBuffer()\n");
    UNIMPLEMENTED;
    return NULL;
}


/*
 * @unimplemented
 */
STORPORT_API
BOOLEAN
NTAPI
StorPortBusy(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG RequestsToComplete)
{
    DPRINT1("StorPortBuzy()\n");
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
STORPORT_API
VOID
NTAPI
StorPortCompleteRequest(
    _In_ PVOID HwDeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun,
    _In_ UCHAR SrbStatus)
{
    DPRINT1("StorPortCompleteRequest()\n");
    UNIMPLEMENTED;
}


/*
 * @implemented
 */
STORPORT_API
ULONG
NTAPI
StorPortConvertPhysicalAddressToUlong(
    _In_ STOR_PHYSICAL_ADDRESS Address)
{
    DPRINT("StorPortConvertPhysicalAddressToUlong()\n");

    return Address.u.LowPart;
}


/*
 * @implemented
 */
STORPORT_API
STOR_PHYSICAL_ADDRESS
NTAPI
StorPortConvertUlongToPhysicalAddress(
    _In_ ULONG_PTR UlongAddress)
{
    STOR_PHYSICAL_ADDRESS Address;

    DPRINT("StorPortConvertUlongToPhysicalAddress()\n");

    Address.QuadPart = UlongAddress;
    return Address;
}


/*
 * @implemented
 */
STORPORT_API
VOID
StorPortDebugPrint(
    _In_ ULONG DebugPrintLevel,
    _In_ PCHAR DebugMessage,
    ...)
{
    va_list ap;

    va_start(ap, DebugMessage);
    vDbgPrintExWithPrefix("STORMINI: ",
                          -1,
                          DPFLTR_ERROR_LEVEL,
                          DebugMessage,
                          ap);
    va_end(ap);
}


/*
 * @implemented
 */
STORPORT_API
BOOLEAN
NTAPI
StorPortDeviceBusy(
    _In_ PVOID HwDeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun,
    _In_ ULONG RequestsToComplete)
{
    /* TODO: Need coverage test */
    PFDO_DEVICE_EXTENSION FdoExtension;
    PPDO_DEVICE_EXTENSION PdoExtension;
    KLOCK_QUEUE_HANDLE LockHandle;
    
    DPRINT1("StorPortDeviceBusy(%p %d %d %d %d)\n",
            HwDeviceExtension, PathId, TargetId, Lun, RequestsToComplete);
    __debugbreak();
    
    FdoExtension = StorpGetMiniportFdo(HwDeviceExtension);
    PdoExtension = FdoFindLun(FdoExtension, PathId, TargetId, Lun);

    KeAcquireInStackQueuedSpinLock(&FdoExtension->FlowControl.Lock, &LockHandle);
    NT_ASSERT(!PdoExtension->FlowControl.IsBusy);
    PdoExtension->FlowControl.IsBusy = TRUE;
    PdoExtension->FlowControl.RemainingBusyRequests = RequestsToComplete;
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    return FALSE;
}


/*
 * @unimplemented
 */
STORPORT_API
BOOLEAN
NTAPI
StorPortDeviceReady(
    _In_ PVOID HwDeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun)
{
    /* TODO: Need coverage test */
    PFDO_DEVICE_EXTENSION FdoExtension;
    PPDO_DEVICE_EXTENSION PdoExtension;
    KLOCK_QUEUE_HANDLE LockHandle;
    
    DPRINT1("StorPortDeviceReady(%p %d %d %d %d)\n",
            HwDeviceExtension, PathId, TargetId, Lun);
    __debugbreak();
    
    FdoExtension = StorpGetMiniportFdo(HwDeviceExtension);
    PdoExtension = FdoFindLun(FdoExtension, PathId, TargetId, Lun);

    KeAcquireInStackQueuedSpinLock(&FdoExtension->FlowControl.Lock, &LockHandle);
    NT_ASSERT(!PdoExtension->FlowControl.IsBusy);
    PdoExtension->FlowControl.IsBusy = FALSE;
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    return FALSE;
}


/*
 * @unimplemented
 */
STORPORT_API
ULONG
StorPortExtendedFunction(
    _In_ STORPORT_FUNCTION_CODE FunctionCode,
    _In_ PVOID HwDeviceExtension,
    ...)
{
    va_list va;
    ULONG Status = STATUS_NOT_IMPLEMENTED;
    PMINIPORT_DEVICE_EXTENSION MiniportExtension = NULL;
    PFDO_DEVICE_EXTENSION DeviceExtension = NULL;
    ULONG AllocatePoolSize;
    ULONG PoolTag;
    PVOID* AllocatedPoolPointer;
    PVOID PoolPointer;

    DPRINT1("StorPortExtendedFunction(%d %p ...)\n",
            FunctionCode, HwDeviceExtension);

    va_start(va, HwDeviceExtension);

    switch (FunctionCode) {
        case ExtFunctionAllocatePool:
        {
            /* Allocate a non paged pool for miniport */
            AllocatePoolSize = va_arg(va, ULONG);
            PoolTag = va_arg(va, ULONG);
            AllocatedPoolPointer = va_arg(va, PVOID*);

            if (AllocatedPoolPointer == NULL) {
                Status = STOR_STATUS_INVALID_PARAMETER;
                break;
            }

            PoolPointer = ExAllocatePoolWithTag(NonPagedPool, AllocatePoolSize, PoolTag);

            if (PoolPointer == NULL) {
                *AllocatedPoolPointer = NULL;
                Status = STOR_STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            *AllocatedPoolPointer = PoolPointer;
            Status = STOR_STATUS_SUCCESS;
            break;
        }

        case ExtFunctionFreePool:
        {
            /* Free a pool allocated with ExtFunctionAllocatePool */
            PoolPointer = va_arg(va, PVOID);

            if (PoolPointer == NULL) {
                Status =  STOR_STATUS_INVALID_PARAMETER;
                break;
            }

            ExFreePool(PoolPointer);
            Status = STOR_STATUS_SUCCESS;
            break;
        }

        case ExtFunctionInitializeTimer:
        {
            /* Allocate a timer for an HBA */
            PTIMER_ENTRY TimerEntry;
            PVOID* TimerHandle = va_arg(va, PVOID*);

            if (TimerHandle == NULL) {
                Status = STOR_STATUS_INVALID_PARAMETER;
                break;
            }

            TimerEntry = (PTIMER_ENTRY)ExAllocatePoolWithTag(NonPagedPool, 
                                                             sizeof(TIMER_ENTRY),
                                                             TAG_MINIPORT_DATA);
            RtlZeroMemory(TimerEntry, sizeof(TIMER_ENTRY));

            if (TimerEntry == NULL) {
                Status = STOR_STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            /* Initialize internal timer object; Win8.1+ EX_TIMER may be used too if we had it */
            KeInitializeTimer(&TimerEntry->Timer);

            MiniportExtension = CONTAINING_RECORD(HwDeviceExtension,
                                                  MINIPORT_DEVICE_EXTENSION,
                                                  HwDeviceExtension);
            DeviceExtension = MiniportExtension->Miniport->DeviceExtension;

            /* Insert into timer list and increment counter */
            StorpInterlockedInsertHeadListCounted(&DeviceExtension->TimerListLock,
                                                  &DeviceExtension->TimerListHead,
                                                  &TimerEntry->ListEntry,
                                                  &DeviceExtension->TimerCount);
            
            /* Return list entry of the timer as its handle */
            *TimerHandle = (PVOID)&TimerEntry->ListEntry;
            Status = STOR_STATUS_SUCCESS;

            break;
        }

        case ExtFunctionRequestTimer:
        {
            /* Schedule a single shot timer event */
            PVOID TimerHandle = va_arg(va, PVOID);
            PHW_TIMER_EX TimerCallback = va_arg(va, PHW_TIMER_EX);
            PVOID CallbackContext = va_arg(va, PVOID);
            ULONGLONG TimerValue = va_arg(va, ULONGLONG);
            ULONGLONG TolerableDelay = va_arg(va, ULONGLONG);
            PTIMER_ENTRY TimerEntry;
            ULONG IsTimerAlreadySet;
            LARGE_INTEGER TimerDueTime;
            HRESULT MultiplyResult;

            /* FIXME: I actually don't know how to deal with tolerable delay */
            UNREFERENCED_PARAMETER(TolerableDelay);

            if (HwDeviceExtension == NULL || TimerHandle == NULL || TimerCallback == NULL) {
                Status = STOR_STATUS_INVALID_PARAMETER;
                break;
            }

            TimerEntry = CONTAINING_RECORD(TimerHandle, TIMER_ENTRY, ListEntry);

            /* Check if timer is already set. If so, don't try to reactivate timer. */
            IsTimerAlreadySet = InterlockedCompareExchange(&TimerEntry->TimerAlreadySet, 1, 0);
            if (IsTimerAlreadySet == 1) {
                Status = STOR_STATUS_BUSY;
                break;
            }

            /* If timeout is zero, cancel the timer */
            if (TimerValue == 0ull) {
                KeCancelTimer(&TimerEntry->Timer);
                Status = STOR_STATUS_SUCCESS;
                break;
            }

            /* This API uses microseconds, KTIMER operate on 100 nanoseconds, multiply by 10
               to get the correct timer due time */
            MultiplyResult = ULongLongMult(TimerValue, 10ull, (PULONGLONG)&TimerDueTime.QuadPart);
            if (MultiplyResult != INTSAFE_SUCCESS) {
                Status = STOR_STATUS_INVALID_PARAMETER;
                break;
            }

            /* Initialize the DPC, context and set timer accordingly */
            TimerEntry->HwDeviceExtension = HwDeviceExtension;
            TimerEntry->TimerCallback = TimerCallback;
            TimerEntry->Context = CallbackContext;
            KeInitializeDpc(&TimerEntry->TimerCallbackDpc,
                            TimerCallbackDpcRoutine,
                            (PVOID)TimerEntry);
            KeSetTimer(&TimerEntry->Timer,
                       TimerDueTime,
                       &TimerEntry->TimerCallbackDpc);
            Status = STOR_STATUS_SUCCESS;
            break;
        }

        default:
        {
            UNIMPLEMENTED;
            // __debugbreak();
        }
    }

    va_end(va);
    
    return Status;
}


/*
 * @implemented
 */
STORPORT_API
VOID
NTAPI
StorPortFreeDeviceBase(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID MappedAddress)
{
    DPRINT1("StorPortFreeDeviceBase(%p %p)\n",
            HwDeviceExtension, MappedAddress);
}


/*
 * @unimplemented
 */
STORPORT_API
VOID
NTAPI
StorPortFreeRegistryBuffer(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Buffer)
{
    DPRINT1("StorPortFreeRegistryBuffer()\n");
    UNIMPLEMENTED;
}


/*
 * @implemented
 */
STORPORT_API
ULONG
NTAPI
StorPortGetBusData(
    _In_ PVOID DeviceExtension,
    _In_ ULONG BusDataType,
    _In_ ULONG SystemIoBusNumber,
    _In_ ULONG SlotNumber,
    _Out_ _When_(Length != 0, _Out_writes_bytes_(Length)) PVOID Buffer,
    _In_ ULONG Length)
{
    PMINIPORT_DEVICE_EXTENSION MiniportExtension;
    PBUS_INTERFACE_STANDARD Interface;
    ULONG ReturnLength;

    DPRINT1("StorPortGetBusData(%p %lu %lu %lu %p %lu)\n",
            DeviceExtension, BusDataType, SystemIoBusNumber, SlotNumber, Buffer, Length);

    /* Get the miniport extension */
    MiniportExtension = CONTAINING_RECORD(DeviceExtension,
                                          MINIPORT_DEVICE_EXTENSION,
                                          HwDeviceExtension);
    DPRINT1("DeviceExtension %p  MiniportExtension %p\n",
            DeviceExtension, MiniportExtension);

    Interface = &MiniportExtension->Miniport->DeviceExtension->BusInterface;

    if (BusDataType == 4)
        BusDataType = 0;

    ReturnLength = Interface->GetBusData(Interface->Context,
                                         BusDataType,
                                         Buffer,
                                         0,
                                         Length);
    DPRINT1("ReturnLength: %lu\n", ReturnLength);

    return ReturnLength;
}


/*
 * @implemented
 */
STORPORT_API
PVOID
NTAPI
StorPortGetDeviceBase(
    _In_ PVOID HwDeviceExtension,
    _In_ INTERFACE_TYPE BusType,
    _In_ ULONG SystemIoBusNumber,
    _In_ STOR_PHYSICAL_ADDRESS IoAddress,
    _In_ ULONG NumberOfBytes,
    _In_ BOOLEAN InIoSpace)
{
    PMINIPORT_DEVICE_EXTENSION MiniportExtension;
    PHYSICAL_ADDRESS TranslatedAddress;
    PVOID MappedAddress;
    NTSTATUS Status;

    DPRINT1("StorPortGetDeviceBase(%p %lu %lu 0x%I64x %lu %u)\n",
            HwDeviceExtension, BusType, SystemIoBusNumber, IoAddress.QuadPart, NumberOfBytes, InIoSpace);

    /* Get the miniport extension */
    MiniportExtension = CONTAINING_RECORD(HwDeviceExtension,
                                          MINIPORT_DEVICE_EXTENSION,
                                          HwDeviceExtension);
    DPRINT1("HwDeviceExtension %p  MiniportExtension %p\n",
            HwDeviceExtension, MiniportExtension);

    if (!TranslateResourceListAddress(MiniportExtension->Miniport->DeviceExtension,
                                      BusType,
                                      SystemIoBusNumber,
                                      IoAddress,
                                      NumberOfBytes,
                                      InIoSpace,
                                      &TranslatedAddress))
    {
        DPRINT1("Checkpoint!\n");
        return NULL;
    }

    DPRINT1("Translated Address: 0x%I64x\n", TranslatedAddress.QuadPart);

    /* In I/O space */
    if (InIoSpace)
    {
        DPRINT1("Translated Address: %p\n", (PVOID)(ULONG_PTR)TranslatedAddress.QuadPart);
        return (PVOID)(ULONG_PTR)TranslatedAddress.QuadPart;
    }

    /* In memory space */
    MappedAddress = MmMapIoSpace(TranslatedAddress,
                                 NumberOfBytes,
                                 FALSE);
    DPRINT1("Mapped Address: %p\n", MappedAddress);

    Status = AllocateAddressMapping(&MiniportExtension->Miniport->DeviceExtension->MappedAddressList,
                                    IoAddress,
                                    MappedAddress,
                                    NumberOfBytes,
                                    SystemIoBusNumber);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Checkpoint!\n");
        MappedAddress = NULL;
    }

    DPRINT1("Mapped Address: %p\n", MappedAddress);
    return MappedAddress;
}


/*
 * @unimplemented
 */
STORPORT_API
PVOID
NTAPI
StorPortGetLogicalUnit(
    _In_ PVOID HwDeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun)
{
    DPRINT1("StorPortGetLogicalUnit()\n");
    UNIMPLEMENTED;
    return NULL;
}


/*
 * @implemented
 */
STORPORT_API
STOR_PHYSICAL_ADDRESS
NTAPI
StorPortGetPhysicalAddress(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_ PVOID VirtualAddress,
    _Out_ ULONG *Length)
{
    PMINIPORT_DEVICE_EXTENSION MiniportExtension;
    PFDO_DEVICE_EXTENSION DeviceExtension;
    STOR_PHYSICAL_ADDRESS PhysicalAddress;
    ULONG_PTR Offset;

    DPRINT("StorPortGetPhysicalAddress(%p %p %p %p)\n",
            HwDeviceExtension, Srb, VirtualAddress, Length);

    /* Get the miniport extension */
    MiniportExtension = CONTAINING_RECORD(HwDeviceExtension,
                                          MINIPORT_DEVICE_EXTENSION,
                                          HwDeviceExtension);
    DPRINT("HwDeviceExtension %p  MiniportExtension %p\n",
            HwDeviceExtension, MiniportExtension);

    DeviceExtension = MiniportExtension->Miniport->DeviceExtension;

    /* Inside of the uncached extension? */
    if (((ULONG_PTR)VirtualAddress >= (ULONG_PTR)DeviceExtension->UncachedExtensionVirtualBase) &&
        ((ULONG_PTR)VirtualAddress <= (ULONG_PTR)DeviceExtension->UncachedExtensionVirtualBase + DeviceExtension->UncachedExtensionSize))
    {
        Offset = (ULONG_PTR)VirtualAddress - (ULONG_PTR)DeviceExtension->UncachedExtensionVirtualBase;

        PhysicalAddress.QuadPart = DeviceExtension->UncachedExtensionPhysicalBase.QuadPart + Offset;
        *Length = DeviceExtension->UncachedExtensionSize - Offset;

        return PhysicalAddress;
    }

    // FIXME


    PhysicalAddress = MmGetPhysicalAddress(VirtualAddress);
    *Length = 1;
//    UNIMPLEMENTED;

//    *Length = 0;
//    PhysicalAddress.QuadPart = (LONGLONG)0;

    return PhysicalAddress;
}


/*
 * @implemented
 */
STORPORT_API
PSTOR_SCATTER_GATHER_LIST
NTAPI
StorPortGetScatterGatherList(
    _In_ PVOID HwDeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    UNREFERENCED_PARAMETER(HwDeviceExtension);
    PQUEUED_REQUEST_REFERENCE RequestReference;

    DPRINT("StorPortGetScatterGatherList(%p %p)\n", HwDeviceExtension, Srb);
    RequestReference = (PQUEUED_REQUEST_REFERENCE)Srb->OriginalRequest;

    return RequestReference->ScatterGatherList;
}


/*
 * @implemented
 */
STORPORT_API
PSCSI_REQUEST_BLOCK
NTAPI
StorPortGetSrb(
    _In_ PVOID DeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun,
    _In_ LONG QueueTag)
{
    DPRINT("StorPortGetSrb()\n");
    return NULL;
}


/*
 * @implemented
 */
STORPORT_API
PVOID
NTAPI
StorPortGetUncachedExtension(
    _In_ PVOID HwDeviceExtension,
    _In_ PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    _In_ ULONG NumberOfBytes)
{
    PMINIPORT_DEVICE_EXTENSION MiniportExtension;
    PFDO_DEVICE_EXTENSION DeviceExtension;
    PHYSICAL_ADDRESS LowestAddress, HighestAddress, Alignment;
    DEVICE_DESCRIPTION DeviceDescription;
    ULONG NumberOfMapRegisters;

    DPRINT1("StorPortGetUncachedExtension(%p %p %lu)\n",
            HwDeviceExtension, ConfigInfo, NumberOfBytes);

    /* Get the miniport extension */
    MiniportExtension = CONTAINING_RECORD(HwDeviceExtension,
                                          MINIPORT_DEVICE_EXTENSION,
                                          HwDeviceExtension);
    DPRINT1("HwDeviceExtension %p  MiniportExtension %p\n",
            HwDeviceExtension, MiniportExtension);

    DeviceExtension = MiniportExtension->Miniport->DeviceExtension;

    /* Return the uncached extension base address if we already have one */
    if (DeviceExtension->UncachedExtensionVirtualBase != NULL)
        return DeviceExtension->UncachedExtensionVirtualBase;

    // FIXME: Set DMA stuff here?
    DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.Master = ConfigInfo->Master;
    DeviceDescription.ScatterGather = ConfigInfo->ScatterGather;
    DeviceDescription.DemandMode = ConfigInfo->DemandMode;
    DeviceDescription.Dma32BitAddresses = ConfigInfo->Dma32BitAddresses;
    DeviceDescription.BusNumber = ConfigInfo->SystemIoBusNumber;
    DeviceDescription.DmaChannel = ConfigInfo->DmaChannel;
    DeviceDescription.InterfaceType = ConfigInfo->AdapterInterfaceType;
    DeviceDescription.DmaWidth = ConfigInfo->DmaWidth;
    DeviceDescription.DmaSpeed = ConfigInfo->DmaSpeed;
    DeviceDescription.MaximumLength = ConfigInfo->MaximumTransferLength;
    DeviceDescription.DmaPort = ConfigInfo->DmaPort;

    DeviceExtension->DmaAdapter =
        IoGetDmaAdapter(DeviceExtension->PhysicalDevice, &DeviceDescription, &NumberOfMapRegisters);

    NT_ASSERT(DeviceExtension->DmaAdapter);

    /* FIXME: AdapterInterfaceType is exposed here and I think it's convenient to initialize
       HwInitData in FDO extension right here. Needs review. */
    DeviceExtension->HwInitData = PortGetDriverInitData(DeviceExtension->DriverExtension,
        DeviceDescription.InterfaceType);
    NT_ASSERT(DeviceExtension->HwInitData);

    /* Allocate the uncached extension */
    Alignment.QuadPart = 0;
    LowestAddress.QuadPart = 0;
    HighestAddress.QuadPart = 0x00000000FFFFFFFF;
    DeviceExtension->UncachedExtensionVirtualBase = MmAllocateContiguousMemorySpecifyCache(NumberOfBytes,
                                                                                           LowestAddress,
                                                                                           HighestAddress,
                                                                                           Alignment,
                                                                                           MmCached);
    if (DeviceExtension->UncachedExtensionVirtualBase == NULL)
        return NULL;

    DeviceExtension->UncachedExtensionPhysicalBase = MmGetPhysicalAddress(DeviceExtension->UncachedExtensionVirtualBase);
    DeviceExtension->UncachedExtensionSize = NumberOfBytes;

    return DeviceExtension->UncachedExtensionVirtualBase;
}


/*
 * @unimplemented
 */
STORPORT_API
PVOID
NTAPI
StorPortGetVirtualAddress(
    _In_ PVOID HwDeviceExtension,
    _In_ STOR_PHYSICAL_ADDRESS PhysicalAddress)
{
    DPRINT1("StorPortGetVirtualAddress(%p %I64x)\n",
            HwDeviceExtension, PhysicalAddress.QuadPart);
    UNIMPLEMENTED;
    return NULL;
}


/*
 * @implemented
 */
STORPORT_API
ULONG
NTAPI
StorPortInitialize(
    _In_ PVOID Argument1,
    _In_ PVOID Argument2,
    _In_ struct _HW_INITIALIZATION_DATA *HwInitializationData,
    _In_opt_ PVOID HwContext)
{
    PDRIVER_OBJECT DriverObject = (PDRIVER_OBJECT)Argument1;
    PUNICODE_STRING RegistryPath = (PUNICODE_STRING)Argument2;
    PDRIVER_OBJECT_EXTENSION DriverObjectExtension;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT1("StorPortInitialize(%p %p %p %p)\n",
            Argument1, Argument2, HwInitializationData, HwContext);

    DPRINT1("HwInitializationDataSize: %lu\n", HwInitializationData->HwInitializationDataSize);
    DPRINT1("AdapterInterfaceType: %u\n", HwInitializationData->AdapterInterfaceType);
    DPRINT1("HwInitialize: %p\n", HwInitializationData->HwInitialize);
    DPRINT1("HwStartIo: %p\n", HwInitializationData->HwStartIo);
    DPRINT1("HwInterrupt: %p\n", HwInitializationData->HwInterrupt);
    DPRINT1("HwFindAdapter: %p\n", HwInitializationData->HwFindAdapter);
    DPRINT1("HwResetBus: %p\n", HwInitializationData->HwResetBus);
    DPRINT1("HwDmaStarted: %p\n", HwInitializationData->HwDmaStarted);
    DPRINT1("HwAdapterState: %p\n", HwInitializationData->HwAdapterState);
    DPRINT1("DeviceExtensionSize: %lu\n", HwInitializationData->DeviceExtensionSize);
    DPRINT1("SpecificLuExtensionSize: %lu\n", HwInitializationData->SpecificLuExtensionSize);
    DPRINT1("SrbExtensionSize: %lu\n", HwInitializationData->SrbExtensionSize);
    DPRINT1("NumberOfAccessRanges: %lu\n", HwInitializationData->NumberOfAccessRanges);

    /* Check parameters */
    if ((DriverObject == NULL) ||
        (RegistryPath == NULL) ||
        (HwInitializationData == NULL))
    {
        DPRINT1("Invalid parameter!\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Check initialization data */
    if ((HwInitializationData->HwInitializationDataSize < sizeof(HW_INITIALIZATION_DATA)) ||
        (HwInitializationData->HwInitialize == NULL) ||
        (HwInitializationData->HwStartIo == NULL) ||
        (HwInitializationData->HwFindAdapter == NULL) ||
        (HwInitializationData->HwResetBus == NULL))
    {
        DPRINT1("Revision mismatch!\n");
        return STATUS_REVISION_MISMATCH;
    }

    DriverObjectExtension = IoGetDriverObjectExtension(DriverObject,
                                                       (PVOID)DriverEntry);
    if (DriverObjectExtension == NULL)
    {
        DPRINT1("No driver object extension!\n");

        Status = IoAllocateDriverObjectExtension(DriverObject,
                                                 (PVOID)DriverEntry,
                                                 sizeof(DRIVER_OBJECT_EXTENSION),
                                                 (PVOID *)&DriverObjectExtension);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoAllocateDriverObjectExtension() failed (Status 0x%08lx)\n", Status);
            return Status;
        }

        DPRINT1("Driver object extension created!\n");

        /* Initialize the driver object extension */
        RtlZeroMemory(DriverObjectExtension,
                      sizeof(DRIVER_OBJECT_EXTENSION));

        DriverObjectExtension->ExtensionType = DriverExtension;
        DriverObjectExtension->DriverObject = DriverObject;

        InitializeListHead(&DriverObjectExtension->AdapterListHead);
        KeInitializeSpinLock(&DriverObjectExtension->AdapterListLock);

        InitializeListHead(&DriverObjectExtension->InitDataListHead);

        /* Set handlers */
        DriverObject->DriverExtension->AddDevice = PortAddDevice;
//        DriverObject->DriverStartIo = PortStartIo;
        DriverObject->DriverUnload = PortUnload;
        DriverObject->MajorFunction[IRP_MJ_CREATE] = PortDispatchCreate;
        DriverObject->MajorFunction[IRP_MJ_CLOSE] = PortDispatchClose;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PortDispatchDeviceControl;
        DriverObject->MajorFunction[IRP_MJ_SCSI] = PortDispatchScsi;
        DriverObject->MajorFunction[IRP_MJ_POWER] = PortDispatchPower;
        DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PortDispatchSystemControl;
        DriverObject->MajorFunction[IRP_MJ_PNP] = PortDispatchPnp;
    }

    /* Add the initialzation data to the driver extension */
    Status = PortAddDriverInitData(DriverObjectExtension,
                                   HwInitializationData);

    DPRINT1("StorPortInitialize() done (Status 0x%08lx)\n", Status);

    return Status;
}


/*
 * @unimplemented
 */
STORPORT_API
VOID
NTAPI
StorPortLogError(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun,
    _In_ ULONG ErrorCode,
    _In_ ULONG UniqueId)
{
    DPRINT1("ScsiPortLogError() called\n");
    DPRINT1("PathId: 0x%02x  TargetId: 0x%02x  Lun: 0x%02x  ErrorCode: 0x%08lx  UniqueId: 0x%08lx\n",
            PathId, TargetId, Lun, ErrorCode, UniqueId);

    DPRINT1("ScsiPortLogError() done\n");
}


/*
 * @implemented
 */
STORPORT_API
VOID
NTAPI
StorPortMoveMemory(
    _Out_writes_bytes_(Length) PVOID Destination,
    _In_reads_bytes_(Length) PVOID Source,
    _In_ ULONG Length)
{
    RtlMoveMemory(Destination, Source, Length);
}


/*
 * @unimplemented
 */
STORPORT_API
VOID
StorPortNotification(
    _In_ SCSI_NOTIFICATION_TYPE NotificationType,
    _In_ PVOID HwDeviceExtension,
    ...)
{
    PMINIPORT_DEVICE_EXTENSION MiniportExtension = NULL;
    PFDO_DEVICE_EXTENSION FdoExtension = NULL;
    PHW_PASSIVE_INITIALIZE_ROUTINE HwPassiveInitRoutine;
    PSTORPORT_EXTENDED_FUNCTIONS *ppExtendedFunctions;
    PQUEUED_REQUEST_REFERENCE RequestReference;
    PBOOLEAN Result;
    PSTOR_DPC Dpc;
    PHW_DPC_ROUTINE HwDpcRoutine;
    PVOID DpcArgument1;
    PVOID DpcArgument2;

    STOR_SPINLOCK SpinLock;
    PVOID LockContext;
    PSTOR_LOCK_HANDLE LockHandle;
    PSCSI_REQUEST_BLOCK Srb;

    va_list ap;

    DPRINT("StorPortNotification(%x %p)\n",
            NotificationType, HwDeviceExtension);

    /* Get the miniport extension */
    if (HwDeviceExtension != NULL)
    {
        MiniportExtension = CONTAINING_RECORD(HwDeviceExtension,
                                              MINIPORT_DEVICE_EXTENSION,
                                              HwDeviceExtension);
        DPRINT("HwDeviceExtension %p  MiniportExtension %p\n",
                HwDeviceExtension, MiniportExtension);

        FdoExtension = MiniportExtension->Miniport->DeviceExtension;
    }

    va_start(ap, HwDeviceExtension);

    switch (NotificationType)
    {
        case RequestComplete:
            DPRINT("RequestComplete\n");
            Srb = (PSCSI_REQUEST_BLOCK)va_arg(ap, PSCSI_REQUEST_BLOCK);
            DPRINT1("Complete Notify - (%d:%d:%d) ReqRef %p Srb %p Irp %p Tag %x\n",
                    Srb->PathId, Srb->TargetId, Srb->Lun,
                    Srb->OriginalRequest, Srb,
                    ((PQUEUED_REQUEST_REFERENCE)Srb->OriginalRequest)->Irp,
                    Srb->QueueTag);
            // if (Srb->OriginalRequest != NULL)
            // {
            //     // DPRINT1("Need to complete the IRP!\n");
            //     PortCompleteRequest(HwDeviceExtension, Srb);
            // }
            NT_ASSERT(Srb->OriginalRequest);
            RequestReference = (PQUEUED_REQUEST_REFERENCE)Srb->OriginalRequest;
            // if (RequestReference->SpecialRequestId) {
            //     /* FIXME: DELETE AFTER DEBUG */
            //     DPRINT1("STORPORT SPECIAL REQUEST %d Queuing for completion\n", RequestReference->SpecialRequestId);
            // }

            /* Queue for completion */
            InterlockedPushEntrySList(&FdoExtension->CompletionList,
                                      &RequestReference->CompletionEntry);

            /* Insert DPC for delayed completion */
            KeInsertQueueDpc(&FdoExtension->Device->Dpc, NULL, NULL);
            
            break;

        case GetExtendedFunctionTable:
            DPRINT1("GetExtendedFunctionTable\n");
            ppExtendedFunctions = (PSTORPORT_EXTENDED_FUNCTIONS*)va_arg(ap, PSTORPORT_EXTENDED_FUNCTIONS*);
            if (ppExtendedFunctions != NULL)
                *ppExtendedFunctions = NULL; /* FIXME */
            break;

        case EnablePassiveInitialization:
            DPRINT1("EnablePassiveInitialization\n");
            HwPassiveInitRoutine = (PHW_PASSIVE_INITIALIZE_ROUTINE)va_arg(ap, PHW_PASSIVE_INITIALIZE_ROUTINE);
            DPRINT1("HwPassiveInitRoutine %p\n", HwPassiveInitRoutine);
            Result = (PBOOLEAN)va_arg(ap, PBOOLEAN);

            // FIXME: It is not guaranteed that enabling passive initialization will fail and return
            // FALSE when not called in driver's HwStorInitialize routine.
            // Docs: Remarks of https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/storport/nf-storport-storportenablepassiveinitialization
            *Result = FALSE;

            if ((FdoExtension != NULL) &&
                (FdoExtension->HwPassiveInitRoutine == NULL))
            {
                FdoExtension->HwPassiveInitRoutine = HwPassiveInitRoutine;
                *Result = TRUE;
            }
            break;

        case InitializeDpc:
            DPRINT("InitializeDpc\n");
            Dpc = (PSTOR_DPC)va_arg(ap, PSTOR_DPC);
            DPRINT("Dpc %p\n", Dpc);
            HwDpcRoutine = (PHW_DPC_ROUTINE)va_arg(ap, PHW_DPC_ROUTINE);
            DPRINT("HwDpcRoutine %p\n", HwDpcRoutine);

            KeInitializeDpc((PRKDPC)&Dpc->Dpc,
                            (PKDEFERRED_ROUTINE)HwDpcRoutine,
                            (PVOID)(FdoExtension->Miniport.MiniportExtension->HwDeviceExtension));
            KeInitializeSpinLock(&Dpc->Lock);
            break;
        
        case IssueDpc:
            DPRINT("IssueDpc\n");
            Dpc = (PSTOR_DPC)va_arg(ap, PSTOR_DPC);
            DpcArgument1 = (PVOID)va_arg(ap, PVOID);
            DpcArgument2 = (PVOID)va_arg(ap, PVOID);
            DPRINT("Dpc %p Arg1 %p Arg2 %p\n", Dpc, DpcArgument1, DpcArgument2);
            Result = (PBOOLEAN)va_arg(ap, PBOOLEAN);
            
            *Result = KeInsertQueueDpc((PKDPC)Dpc, DpcArgument1, DpcArgument2);
            break;

        case AcquireSpinLock:
            DPRINT("AcquireSpinLock\n");
            SpinLock = (STOR_SPINLOCK)va_arg(ap, STOR_SPINLOCK);
            DPRINT("SpinLock %lu\n", SpinLock);
            LockContext = (PVOID)va_arg(ap, PVOID);
            DPRINT("LockContext %p\n", LockContext);
            LockHandle = (PSTOR_LOCK_HANDLE)va_arg(ap, PSTOR_LOCK_HANDLE);
            DPRINT("LockHandle %p\n", LockHandle);
            PortAcquireSpinLock(FdoExtension,
                                SpinLock,
                                LockContext,
                                LockHandle);
            break;

        case ReleaseSpinLock:
            DPRINT("ReleaseSpinLock\n");
            LockHandle = (PSTOR_LOCK_HANDLE)va_arg(ap, PSTOR_LOCK_HANDLE);
            DPRINT("LockHandle %p\n", LockHandle);
            PortReleaseSpinLock(FdoExtension,
                                LockHandle);
            break;

        default:
            DPRINT1("Unsupported Notification %lx\n", NotificationType);
            break;
    }

    va_end(ap);
}


/*
 * @unimplemented
 */
STORPORT_API
BOOLEAN
NTAPI
StorPortPause(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG TimeOut)
{
    DPRINT1("StorPortPause()\n");
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
STORPORT_API
BOOLEAN
NTAPI
StorPortPauseDevice(
    _In_ PVOID HwDeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun,
    _In_ ULONG TimeOut)
{
    DPRINT1("StorPortPauseDevice()\n");
    UNIMPLEMENTED;
    return FALSE;
}


#if defined(_M_AMD64)
/*
 * @implemented
 */
/* KeQuerySystemTime is an inline function,
   so we cannot forward the export to ntoskrnl */
STORPORT_API
VOID
NTAPI
StorPortQuerySystemTime(
    _Out_ PLARGE_INTEGER CurrentTime)
{
    DPRINT1("StorPortQuerySystemTime(%p)\n", CurrentTime);

    KeQuerySystemTime(CurrentTime);
}
#endif /* defined(_M_AMD64) */


/*
 * @unimplemented
 */
STORPORT_API
BOOLEAN
NTAPI
StorPortReady(
    _In_ PVOID HwDeviceExtension)
{
    DPRINT1("StorPortReady()\n");
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
STORPORT_API
BOOLEAN
NTAPI
StorPortRegistryRead(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR ValueName,
    _In_ ULONG Global,
    _In_ ULONG Type,
    _In_ PUCHAR Buffer,
    _In_ PULONG BufferLength)
{
    DPRINT1("StorPortRegistryRead()\n");
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
STORPORT_API
BOOLEAN
NTAPI
StorPortRegistryWrite(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR ValueName,
    _In_ ULONG Global,
    _In_ ULONG Type,
    _In_ PUCHAR Buffer,
    _In_ ULONG BufferLength)
{
    DPRINT1("StorPortRegistryWrite()\n");
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
STORPORT_API
BOOLEAN
NTAPI
StorPortResume(
    _In_ PVOID HwDeviceExtension)
{
    DPRINT1("StorPortResume()\n");
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
STORPORT_API
BOOLEAN
NTAPI
StorPortResumeDevice(
    _In_ PVOID HwDeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun)
{
    DPRINT1("StorPortResumeDevice()\n");
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @implemented
 */
STORPORT_API
ULONG
NTAPI
StorPortSetBusDataByOffset(
    _In_ PVOID DeviceExtension,
    _In_ ULONG BusDataType,
    _In_ ULONG SystemIoBusNumber,
    _In_ ULONG SlotNumber,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    PMINIPORT_DEVICE_EXTENSION MiniportExtension;
    PBUS_INTERFACE_STANDARD Interface;
    ULONG ReturnLength;

    DPRINT1("StorPortSetBusData(%p %lu %lu %lu %p %lu %lu)\n",
            DeviceExtension, BusDataType, SystemIoBusNumber, SlotNumber, Buffer, Offset, Length);

    MiniportExtension = CONTAINING_RECORD(DeviceExtension,
                                          MINIPORT_DEVICE_EXTENSION,
                                          HwDeviceExtension);
    DPRINT1("DeviceExtension %p  MiniportExtension %p\n",
            DeviceExtension, MiniportExtension);

    Interface = &MiniportExtension->Miniport->DeviceExtension->BusInterface;

    ReturnLength = Interface->SetBusData(Interface->Context,
                                         BusDataType,
                                         Buffer,
                                         Offset,
                                         Length);
    DPRINT1("ReturnLength: %lu\n", ReturnLength);

    return ReturnLength;
}


/*
 * @implemented
 */
STORPORT_API
BOOLEAN
NTAPI
StorPortSetDeviceQueueDepth(
    _In_ PVOID HwDeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun,
    _In_ ULONG Depth)
{
    PFDO_DEVICE_EXTENSION FdoExtension;
    PPDO_DEVICE_EXTENSION PdoExtension;
    KLOCK_QUEUE_HANDLE LockHandle;

    DPRINT1("StorPortSetDeviceQueueDepth(%p %d %d %d %d)\n",
            HwDeviceExtension, PathId, TargetId, Lun, Depth);

    FdoExtension = StorpGetMiniportFdo(HwDeviceExtension);
    PdoExtension = FdoFindLun(FdoExtension, PathId, TargetId, Lun);

    /* 254 is pre-Windows-8 per LUN queue limit */
    if (PdoExtension == NULL || Depth > 254)
    {
        return FALSE;
    }

    KeAcquireInStackQueuedSpinLock(&FdoExtension->FlowControl.Lock, &LockHandle);
    PdoExtension->OutstandingRequestMax = Depth;
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    DPRINT("Set Path %d Target %d Lun %d Queue depth to %d\n", PathId, TargetId, Lun, Depth);

    return FALSE;
}


/*
 * @implemented
 */
STORPORT_API
VOID
NTAPI
StorPortStallExecution(
    _In_ ULONG Delay)
{
    KeStallExecutionProcessor(Delay);
}


/*
 * @unimplemented
 */
STORPORT_API
VOID
NTAPI
StorPortSynchronizeAccess(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_SYNCHRONIZED_ACCESS SynchronizedAccessRoutine,
    _In_opt_ PVOID Context)
{
    DPRINT1("StorPortSynchronizeAccess()\n");
    UNIMPLEMENTED;
}


/*
 * @implemented
 */
STORPORT_API
BOOLEAN
NTAPI
StorPortValidateRange(
    _In_ PVOID HwDeviceExtension,
    _In_ INTERFACE_TYPE BusType,
    _In_ ULONG SystemIoBusNumber,
    _In_ STOR_PHYSICAL_ADDRESS IoAddress,
    _In_ ULONG NumberOfBytes,
    _In_ BOOLEAN InIoSpace)
{
    DPRINT1("StorPortValidateRange()\n");
    return TRUE;
}

/* EOF */
