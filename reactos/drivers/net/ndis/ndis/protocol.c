/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/protocol.c
 * PURPOSE:     Routines used by NDIS protocol drivers
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <ndissys.h>
#include <miniport.h>
#include <protocol.h>

LIST_ENTRY ProtocolListHead;
KSPIN_LOCK ProtocolListLock;


VOID ProtocolWorker(
    PVOID Context)
/*
 * FUNCTION: Worker function for ProtocolXxx functions
 * ARGUMENTS:
 *     Context = Pointer to context information (PROTOCOL_BINDING)
 */
{
}


VOID
EXPORT
STDCALL
NdisCloseAdapter(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisBindingHandle)
/*
 * FUNCTION: Closes an adapter opened with NdisOpenAdapter
 * ARGUMENTS:
 *     Status            = Address of buffer for status information
 *     NdisBindingHandle = Handle returned by NdisOpenAdapter
 */
{
    PADAPTER_BINDING AdapterBinding = GET_ADAPTER_BINDING(NdisBindingHandle);

    ExFreePool(AdapterBinding);

    *Status = NDIS_STATUS_SUCCESS;
}


VOID
EXPORT
NdisDeregisterProtocol(
    OUT PNDIS_STATUS    Status,
    IN NDIS_HANDLE      NdisProtocolHandle)
/*
 * FUNCTION: Releases the resources allocated by NdisRegisterProtocol
 * ARGUMENTS:
 *     Status             = Address of buffer for status information
 *     NdisProtocolHandle = Handle returned by NdisRegisterProtocol
 */
{
    ExFreePool(NdisProtocolHandle);
    *Status = NDIS_STATUS_SUCCESS;
}


VOID
EXPORT
NdisOpenAdapter(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_STATUS    OpenErrorStatus,
    OUT PNDIS_HANDLE    NdisBindingHandle,
    OUT PUINT           SelectedMediumIndex,
    IN  PNDIS_MEDIUM    MediumArray,
    IN  UINT            MediumArraySize,
    IN  NDIS_HANDLE     NdisProtocolHandle,
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  PNDIS_STRING    AdapterName,
    IN  UINT            OpenOptions,
    IN  PSTRING         AddressingInformation   OPTIONAL)
/*
 * FUNCTION: Opens an adapter for communication
 * ARGUMENTS:
 *     Status                 = Address of buffer for status information
 *     OpenErrorStatus        = Address of buffer for secondary error code
 *     NdisBindingHandle      = Address of buffer for adapter binding handle
 *     SelectedMediumIndex    = Address of buffer for selected medium
 *     MediumArray            = Pointer to an array of NDIS_MEDIUMs called can support
 *     MediumArraySize        = Number of elements in MediumArray
 *     NdisProtocolHandle     = Handle returned by NdisRegisterProtocol
 *     ProtocolBindingContext = Pointer to caller suplied context area
 *     AdapterName            = Pointer to buffer with name of adapter
 *     OpenOptions            = Bitmask with flags passed to next-lower driver
 *     AddressingInformation  = Optional pointer to buffer with NIC specific information
 */
{
    PADAPTER_BINDING AdapterBinding;
    PLOGICAL_ADAPTER Adapter;
    NDIS_STATUS NdisStatus;
    PNDIS_MEDIUM Medium1;
    PNDIS_MEDIUM Medium2;
    ULONG BytesWritten;
    BOOLEAN Found;
    UINT i, j;
    PPROTOCOL_BINDING Protocol = GET_PROTOCOL_BINDING(ProtocolBindingContext);

    Adapter = MiniLocateDevice(AdapterName);

    if (!Adapter) {
        NDIS_DbgPrint(MIN_TRACE, ("Adapter not found.\n"));
        *Status = NDIS_STATUS_ADAPTER_NOT_FOUND;
        return;
    }

    /* Query the miniport driver for it's supported medias and search the list
       to find the first medium also supported by the protocol driver */

    NdisStatus = MiniQueryInformation(Adapter, OID_GEN_MEDIA_SUPPORTED, 0, &BytesWritten);

    /* FIXME: Handle this */
    if (NdisStatus == NDIS_STATUS_PENDING) {
        NDIS_DbgPrint(MIN_TRACE, ("NDIS_STATUS_PENDING returned!\n"));
    }

    if (!NT_SUCCESS(NdisStatus))
        *Status = NdisStatus;

    Medium1 = Adapter->QueryBuffer;
    Medium2 = MediumArray;
    Found   = FALSE;
    for (i = 0; i < BytesWritten / sizeof(NDIS_MEDIUM); i++) {
        for (j = 0; j < MediumArraySize; j++) {
            if (Medium2[j] == Medium1[i]) {
                *SelectedMediumIndex = j;
                Found = TRUE;
                break;
            }
        }
        if (Found)
            break;
    }

    if (!Found) {
        NDIS_DbgPrint(MIN_TRACE, ("Media is not supported.\n"));
        *Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
        return;
    }

    AdapterBinding = ExAllocatePool(NonPagedPool, sizeof(ADAPTER_BINDING));

    if (!AdapterBinding) {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    RtlZeroMemory(AdapterBinding, sizeof(ADAPTER_BINDING));

    /* Put on protocol binding adapter list */
    ExInterlockedInsertTailList(&Protocol->AdapterListHead,
                                &AdapterBinding->ProtocolListEntry,
                                &Protocol->Lock);

    *NdisBindingHandle = AdapterBinding;

    *Status = NDIS_STATUS_SUCCESS;
}


VOID
EXPORT
NdisRegisterProtocol(
    OUT PNDIS_STATUS                    Status,
    OUT PNDIS_HANDLE                    NdisProtocolHandle,
    IN  PNDIS_PROTOCOL_CHARACTERISTICS  ProtocolCharacteristics,
    IN  UINT                            CharacteristicsLength)
/*
 * FUNCTION: Registers an NDIS driver's ProtocolXxx entry points
 * ARGUMENTS:
 *     Status                  = Address of buffer for status information
 *     NdisProtocolHandle      = Address of buffer for handle used to identify the driver
 *     ProtocolCharacteristics = Pointer to NDIS_PROTOCOL_CHARACTERISTICS structure
 *     CharacteristicsLength   = Size of structure which ProtocolCharacteristics targets
 */
{
    PPROTOCOL_BINDING Protocol;
    NTSTATUS NtStatus;
    UINT MinSize;

    switch (ProtocolCharacteristics->MajorNdisVersion) {
    case 0x03:
        MinSize = sizeof(NDIS30_PROTOCOL_CHARACTERISTICS_S);
        break;

    case 0x04:
        MinSize = sizeof(NDIS40_PROTOCOL_CHARACTERISTICS_S);
        break;

    case 0x05:
        MinSize = sizeof(NDIS50_PROTOCOL_CHARACTERISTICS_S);
        break;

    default:
        *Status = NDIS_STATUS_BAD_VERSION;
        return;
    }

    if (CharacteristicsLength < MinSize) {
        NDIS_DbgPrint(DEBUG_PROTOCOL, ("Bad protocol characteristics.\n"));
        *Status = NDIS_STATUS_BAD_CHARACTERISTICS;
        return;
    }

    Protocol = ExAllocatePool(NonPagedPool, sizeof(PROTOCOL_BINDING));
    if (!Protocol) {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    RtlZeroMemory(Protocol, sizeof(PROTOCOL_BINDING));
    RtlCopyMemory(&Protocol->Chars, ProtocolCharacteristics, MinSize);

    NtStatus = RtlUpcaseUnicodeString(
        &Protocol->Chars.Name,
        &ProtocolCharacteristics->Name,
        TRUE);
    if (!NT_SUCCESS(NtStatus)) {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        ExFreePool(Protocol);
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    KeInitializeSpinLock(&Protocol->Lock);

    Protocol->RefCount = 1;

    ExInitializeWorkItem(&Protocol->WorkItem, ProtocolWorker, Protocol);

    InitializeListHead(&Protocol->AdapterListHead);

    *NdisProtocolHandle = Protocol;
    *Status             = NDIS_STATUS_SUCCESS;
}


VOID
EXPORT
NdisRequest(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PNDIS_REQUEST   NdisRequest)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisReset(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisBindingHandle)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisSend(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PNDIS_PACKET    Packet)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisSendPackets(
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisTransferData(
    OUT PNDIS_STATUS        Status,
    IN  NDIS_HANDLE         NdisBindingHandle,
    IN  NDIS_HANDLE         MacReceiveContext,
    IN  UINT                ByteOffset,
    IN  UINT                BytesToTransfer,
    IN  OUT	PNDIS_PACKET    Packet,
    OUT PUINT               BytesTransferred)
{
    UNIMPLEMENTED
}

/* EOF */
