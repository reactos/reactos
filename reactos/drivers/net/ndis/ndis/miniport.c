/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/miniport.c
 * PURPOSE:     Routines used by NDIS miniport drivers
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <miniport.h>


/* Number of medias we know */
#define MEDIA_ARRAY_SIZE    15

static NDIS_MEDIUM MediaArray[MEDIA_ARRAY_SIZE] = {
    NdisMedium802_3,
    NdisMedium802_5,
    NdisMediumFddi,
    NdisMediumWan,
    NdisMediumLocalTalk,
    NdisMediumDix,
    NdisMediumArcnetRaw,
    NdisMediumArcnet878_2,
    NdisMediumAtm,
    NdisMediumWirelessWan,
    NdisMediumIrda,
    NdisMediumBpc,
    NdisMediumCoWan,
    NdisMedium1394,
    NdisMediumMax
};


LIST_ENTRY MiniportListHead;
KSPIN_LOCK MiniportListLock;
LIST_ENTRY AdapterListHead;
KSPIN_LOCK AdapterListLock;


VOID MiniportWorker(
    PVOID Context)
/*
 * FUNCTION: Worker function for MiniportXxx functions
 * ARGUMENTS:
 *     Context = Pointer to context information (MINIPORT_DRIVER)
 */
{
}


PLOGICAL_ADAPTER MiniLocateDevice(
    PNDIS_STRING AdapterName)
/*
 * FUNCTION: Returns the logical adapter object for a specific adapter
 * ARGUMENTS:
 *     AdapterName = Pointer to name of adapter
 * RETURNS:
 *     Pointer to logical adapter object, or NULL if none was found.
 *     If found, the adapter is referenced for the caller. The caller
 *     is responsible for dereferencing after use
 */
{
    KIRQL OldIrql1;
    KIRQL OldIrql2;
    PLIST_ENTRY CurrentMEntry;
    PLIST_ENTRY CurrentAEntry;
    PMINIPORT_DRIVER Miniport;
    PLOGICAL_ADAPTER Adapter;

    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called. AdapterName (0x%X).\n", AdapterName));

    KeAcquireSpinLock(&MiniportListLock, &OldIrql1);
    CurrentMEntry = MiniportListHead.Flink;
    while (CurrentMEntry != &MiniportListHead) {
	    Miniport = CONTAINING_RECORD(CurrentMEntry, MINIPORT_DRIVER, ListEntry);

        KeAcquireSpinLock(&AdapterListLock, &OldIrql2);
        CurrentAEntry = AdapterListHead.Flink;
        while (CurrentAEntry != &AdapterListHead) {
	        Adapter = CONTAINING_RECORD(CurrentAEntry, LOGICAL_ADAPTER, ListEntry);

            if (RtlCompareUnicodeString(AdapterName, &Adapter->DeviceName, TRUE) == 0) {
                ReferenceObject(Adapter);
                KeReleaseSpinLock(&AdapterListLock, OldIrql2);
                KeReleaseSpinLock(&MiniportListLock, OldIrql1);
                return Adapter;
            }

            CurrentAEntry = CurrentAEntry->Flink;
        }
        KeReleaseSpinLock(&AdapterListLock, OldIrql2);

        CurrentMEntry = CurrentMEntry->Flink;
    }
    KeReleaseSpinLock(&MiniportListLock, OldIrql1);

    return NULL;
}


NDIS_STATUS
MiniQueryInformation(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_OID            Oid,
    ULONG               Size,
    PULONG              BytesWritten)
/*
 * FUNCTION: Queries a logical adapter for properties
 * ARGUMENTS:
 *     Adapter      = Pointer to the logical adapter object to query
 *     Oid          = Specifies the oid to query for
 *     Size         = If non-zero overrides the length in the adapter object
 *     BytesWritten = Address of buffer to place number of bytes written
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     If the specified buffer is too small, a new buffer is allocated,
 *     and the query is attempted again
 */
{
    NDIS_STATUS NdisStatus;
    ULONG BytesNeeded;

    if (Adapter->QueryBufferLength == 0) {
        Adapter->QueryBuffer = ExAllocatePool(NonPagedPool, 32);

        if (!Adapter->QueryBuffer) {
            NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
            return NDIS_STATUS_RESOURCES;
        }

        Adapter->QueryBufferLength = 32;
    }

    BytesNeeded = (Size = 0)? Adapter->QueryBufferLength : Size;

    NdisStatus = (*Adapter->Miniport->Chars.QueryInformationHandler)(
        Adapter, Oid, &BytesNeeded,
        Adapter->QueryBufferLength,
        BytesWritten, &BytesNeeded);

    if ((NT_SUCCESS(NdisStatus)) || (NdisStatus == NDIS_STATUS_PENDING)) {
        NDIS_DbgPrint(DEBUG_MINIPORT, ("Miniport returned status (0x%X).\n", NdisStatus));
        return NdisStatus;
    }

    if (NdisStatus == NDIS_STATUS_INVALID_LENGTH) {
        ExFreePool(Adapter->QueryBuffer);

        Adapter->QueryBufferLength += BytesNeeded;
        Adapter->QueryBuffer = ExAllocatePool(
            NonPagedPool, Adapter->QueryBufferLength);

        if (!Adapter->QueryBuffer) {
            NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
            return NDIS_STATUS_RESOURCES;
        }

        NdisStatus = (*Adapter->Miniport->Chars.QueryInformationHandler)(
            Adapter, Oid, Adapter->QueryBuffer, Size, BytesWritten, &BytesNeeded);
    }

    return NdisStatus;
}


VOID
EXPORT
NdisMCloseLog(
    IN  NDIS_HANDLE LogHandle)
{
    UNIMPLEMENTED
}


NDIS_STATUS
EXPORT
NdisMCreateLog(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  UINT            Size,
    OUT PNDIS_HANDLE    LogHandle)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


VOID
EXPORT
NdisMDeregisterAdapterShutdownHandler(
    IN  NDIS_HANDLE MiniportHandle)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMFlushLog(
    IN  NDIS_HANDLE LogHandle)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMIndicateStatus(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMIndicateStatusComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisInitializeWrapper(
    OUT PNDIS_HANDLE    NdisWrapperHandle,
    IN  PVOID           SystemSpecific1,
    IN  PVOID           SystemSpecific2,
    IN  PVOID           SystemSpecific3)
/*
 * FUNCTION: Notifies the NDIS library that a new miniport is initializing
 * ARGUMENTS:
 *     NdisWrapperHandle = Address of buffer to place NDIS wrapper handle
 *     SystemSpecific1   = Pointer to the driver's driver object
 *     SystemSpecific2   = Pointer to the driver's registry path
 *     SystemSpecific3   = Always NULL
 */
{
    PMINIPORT_DRIVER Miniport;

    Miniport = ExAllocatePool(NonPagedPool, sizeof(MINIPORT_DRIVER));
    if (!Miniport) {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        *NdisWrapperHandle = NULL;
        return;
    }

    RtlZeroMemory(Miniport, sizeof(MINIPORT_DRIVER));

    KeInitializeSpinLock(&Miniport->Lock);

    Miniport->RefCount = 1;

    ExInitializeWorkItem(&Miniport->WorkItem, MiniportWorker, Miniport);

    Miniport->DriverObject = (PDRIVER_OBJECT)SystemSpecific1;
    /* FIXME: Copy RegistryPath */
    //Miniport->RegistryPath = SystemSpecific2;

    /* Put miniport in global miniport list */
    ExInterlockedInsertTailList(&MiniportListHead,
                                &Miniport->ListEntry,
                                &MiniportListLock);

    *NdisWrapperHandle = Miniport;
}


VOID
EXPORT
NdisMRegisterAdapterShutdownHandler(
    IN  NDIS_HANDLE                 MiniportHandle,
    IN  PVOID                       ShutdownContext,
    IN  ADAPTER_SHUTDOWN_HANDLER    ShutdownHandler)
{
    UNIMPLEMENTED
}


NDIS_STATUS
EXPORT
NdisMRegisterMiniport(
    IN  NDIS_HANDLE                     NdisWrapperHandle,
    IN  PNDIS_MINIPORT_CHARACTERISTICS  MiniportCharacteristics,
    IN  UINT                            CharacteristicsLength)
/*
 * FUNCTION: Registers a miniport's MiniportXxx entry points with the NDIS library
 * ARGUMENTS:
 *     NdisWrapperHandle       = Pointer to handle returned by NdisMInitializeWrapper
 *     MiniportCharacteristics = Pointer to a buffer with miniport characteristics
 *     CharacteristicsLength   = Number of bytes in characteristics buffer
 * RETURNS:
 *     Status of operation
 */
{
    UINT MinSize;
    NTSTATUS Status;
    NDIS_STATUS NdisStatus;
    UINT SelectedMediumIndex;
    PLOGICAL_ADAPTER Adapter;
    PMINIPORT_DRIVER Miniport = GET_MINIPORT_DRIVER(NdisWrapperHandle);

    switch (MiniportCharacteristics->MajorNdisVersion) {
    case 0x03:
        MinSize = sizeof(NDIS30_MINIPORT_CHARACTERISTICS_S);
        break;

    case 0x04:
        MinSize = sizeof(NDIS40_MINIPORT_CHARACTERISTICS_S);
        break;

    case 0x05:
        MinSize = sizeof(NDIS50_MINIPORT_CHARACTERISTICS_S);
        break;

    default:
        NDIS_DbgPrint(DEBUG_MINIPORT, ("Bad miniport characteristics version.\n"));
        return NDIS_STATUS_BAD_VERSION;
    }

    if (CharacteristicsLength < MinSize) {
        NDIS_DbgPrint(DEBUG_MINIPORT, ("Bad miniport characteristics.\n"));
        return NDIS_STATUS_BAD_CHARACTERISTICS;
    }

    /* Check if mandatory MiniportXxx functions are specified */
    if ((!MiniportCharacteristics->HaltHandler) ||
        (!MiniportCharacteristics->InitializeHandler)||
        (!MiniportCharacteristics->QueryInformationHandler) ||
        (!MiniportCharacteristics->ResetHandler) ||
        (!MiniportCharacteristics->SetInformationHandler)) {
        NDIS_DbgPrint(DEBUG_MINIPORT, ("Bad miniport characteristics.\n"));
        return NDIS_STATUS_BAD_CHARACTERISTICS;
    }

    if (MiniportCharacteristics->MajorNdisVersion == 0x03) {
        if (!MiniportCharacteristics->u1.SendHandler) {
            NDIS_DbgPrint(DEBUG_MINIPORT, ("Bad miniport characteristics.\n"));
            return NDIS_STATUS_BAD_CHARACTERISTICS;
        }
    } else if (MiniportCharacteristics->MajorNdisVersion >= 0x04) {
        /* NDIS 4.0+ */
        if ((!MiniportCharacteristics->u1.SendHandler) &&
            (!MiniportCharacteristics->SendPacketsHandler)) {
            NDIS_DbgPrint(DEBUG_MINIPORT, ("Bad miniport characteristics.\n"));
            return NDIS_STATUS_BAD_CHARACTERISTICS;
        }
    }

    RtlCopyMemory(&Miniport->Chars, MiniportCharacteristics, MinSize);

    Adapter = ExAllocatePool(NonPagedPool, sizeof(LOGICAL_ADAPTER));
    if (!Adapter) {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return NDIS_STATUS_RESOURCES;
    }

    RtlZeroMemory(Adapter, sizeof(LOGICAL_ADAPTER));
    KeInitializeSpinLock(&Adapter->Lock);
    Adapter->RefCount = 1;
    Adapter->Miniport = Miniport;

    /* Create the device object for this adapter */
    /* FIXME: Use GUIDs */
    RtlInitUnicodeString(&Adapter->DeviceName, L"\\Device\\ne2000");
    Status = IoCreateDevice(Miniport->DriverObject, 0, &Adapter->DeviceName,
        FILE_DEVICE_PHYSICAL_NETCARD, 0, FALSE, &Adapter->DeviceObject);
    if (!NT_SUCCESS(Status)) {
        NDIS_DbgPrint(MIN_TRACE, ("Could not create device object.\n"));
        ExFreePool(Adapter);
        return NDIS_STATUS_FAILURE;
    }

    /* Put adapter in adapter list for this miniport */
    ExInterlockedInsertTailList(&Miniport->AdapterListHead,
                                &Adapter->MiniportListEntry,
                                &Miniport->Lock);

    /* Put adapter in global adapter list */
    ExInterlockedInsertTailList(&AdapterListHead,
                                &Adapter->ListEntry,
                                &AdapterListLock);

    /* Call MiniportInitialize */
    (*Miniport->Chars.InitializeHandler)(
        &NdisStatus,
        &SelectedMediumIndex,
        &MediaArray[0],
        MEDIA_ARRAY_SIZE,
        Adapter,
        NULL /* FIXME: WrapperConfigurationContext */);

    return NDIS_STATUS_SUCCESS;
}


VOID
EXPORT
NdisMResetComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS Status,
    IN  BOOLEAN     AddressingReset)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMSendComplete(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMSendResourcesAvailable(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMSetAttributes(
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  NDIS_HANDLE         MiniportAdapterContext,
    IN  BOOLEAN             BusMaster,
    IN  NDIS_INTERFACE_TYPE AdapterType)
/*
 * FUNCTION: Informs the NDIS library of significant features of the caller's NIC
 * ARGUMENTS:
 *     MiniportAdapterHandle  = Handle input to MiniportInitialize
 *     MiniportAdapterContext = Pointer to context information
 *     BusMaster              = Specifies TRUE if the caller's NIC is a busmaster DMA device
 *     AdapterType            = Specifies the I/O bus interface of the caller's NIC
 */
{
    PLOGICAL_ADAPTER Adapter = GET_LOGICAL_ADAPTER(MiniportAdapterHandle);

    Adapter->MiniportAdapterContext = MiniportAdapterHandle;
    Adapter->Attributes    = BusMaster? NDIS_ATTRIBUTE_BUS_MASTER : 0;
    Adapter->AdapterType   = AdapterType;
    Adapter->AttributesSet = TRUE;
}


VOID
EXPORT
NdisMSetAttributesEx(
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  NDIS_HANDLE         MiniportAdapterContext,
    IN  UINT                CheckForHangTimeInSeconds   OPTIONAL,
    IN  ULONG               AttributeFlags,
    IN  NDIS_INTERFACE_TYPE	AdapterType)
/*
 * FUNCTION: Informs the NDIS library of significant features of the caller's NIC
 * ARGUMENTS:
 *     MiniportAdapterHandle     = Handle input to MiniportInitialize
 *     MiniportAdapterContext    = Pointer to context information
 *     CheckForHangTimeInSeconds = Specifies interval in seconds at which
 *                                 MiniportCheckForHang should be called
 *     AttributeFlags            = Bitmask that indicates specific attributes
 *     AdapterType               = Specifies the I/O bus interface of the caller's NIC
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMSleep(
    IN  ULONG   MicrosecondsToSleep)
{
    UNIMPLEMENTED
}


BOOLEAN
EXPORT
NdisMSynchronizeWithInterrupt(
    IN  PNDIS_MINIPORT_INTERRUPT    Interrupt,
    IN  PVOID                       SynchronizeFunction,
    IN  PVOID                       SynchronizeContext)
{
    UNIMPLEMENTED

    return FALSE;
}


NDIS_STATUS
EXPORT
NdisMWriteLogData(
    IN  NDIS_HANDLE LogHandle,
    IN  PVOID       LogBuffer,
    IN  UINT        LogBufferSize)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


VOID
EXPORT
NdisTerminateWrapper(
    IN  NDIS_HANDLE NdisWrapperHandle,
    IN  PVOID       SystemSpecific)
/*
 * FUNCTION: Releases resources allocated by a call to NdisMInitializeWrapper
 * ARGUMENTS:
 *     NdisWrapperHandle = Handle returned by NdisMInitializeWrapper
 *     SystemSpecific    = Always NULL
 */
{
    ExFreePool(NdisWrapperHandle);
}

/* EOF */
