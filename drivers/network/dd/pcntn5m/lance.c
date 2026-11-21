/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    lance.c

Abstract:

    This is the main file for the Advanced Micro Devices LANCE (Am 7990)
    Ethernet controller.  This driver conforms to the NDIS 5.1 interface.

    The idea for handling loopback and sends simultaneously is largely
    adapted from the EtherLink II NDIS driver by Adam Barr.

Author:

    Anthony V. Ercolano (Tonye) 20-Jul-1990

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

#include <ndis.h>
#include "lancehrd.h"
#include "lancesft.h"
#include "keywords.h"

//#if DBG
#define STATIC
//#else
//#define STATIC static
//#endif


#if DBG

UCHAR LanceSendFails[256] = {0};
UCHAR LanceSendFailPlace = 0;

#endif


NDIS_HANDLE LanceNdisWrapperHandle = NULL;
PDRIVER_OBJECT LanceDriverObject = NULL;

#if LANCELOG

UCHAR Log[LOG_SIZE] = {0};

UCHAR LogPlace = 0;
UCHAR LogWrapped = 0;

UCHAR LancePrintLog = 0;

#endif


//
// If you add to this, make sure to add the
// LanceQueryInformation() if it is
// queriable information.
//
UINT LanceGlobalSupportedOids[] = {
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS
    };

//
// We define a constant csr0 value that is useful for initializing
// an already stopped LANCE.
//
// This also enables the chip for interrupts.
//
#define LANCE_CSR0_INIT_CHIP ((USHORT)0x41)

//
// We define a constant csr0 value that is useful for clearing all of
// the interesting bits that *could* be set on an interrupt.
//
#define LANCE_CSR0_CLEAR_INTERRUPT_BITS ((USHORT)0x7f00)

typedef struct _LANCE_INTERRUPT_STATUS {
    PLANCE_ADAPTER Adapter;
    USHORT Csr;
} LANCE_INTERRUPT_STATUS, *PLANCE_INTERRUPT_STATUS;

//
// KeFlushQueuedDpcs exists only from Windows XP SP2 and Server 2003.
// Importing it would stop the driver loading at all on XP RTM and SP1,
// so DriverEntry looks it up and LanceFlushQueuedDpcs falls back to a
// short sleep where it is missing.
//
typedef VOID (*LANCE_FLUSH_QUEUED_DPCS)(VOID);

static LANCE_FLUSH_QUEUED_DPCS LanceKeFlushQueuedDpcs = NULL;

STATIC
VOID
LanceFlushQueuedDpcs(
    VOID
    );

STATIC
BOOLEAN
LanceSyncReadInterruptStatus(
    IN PVOID Context
    );

VOID
LanceDeferredTimerRoutine(
    IN PVOID SystemSpecific1,
    IN NDIS_HANDLE Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    );

NDIS_STATUS
LanceQueryInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded
    );

NDIS_STATUS
LanceSetInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded
    );

NDIS_STATUS
LanceReset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext
    );

BOOLEAN
AllocateAdapterMemory(
    IN PLANCE_ADAPTER Adapter
    );

NDIS_STATUS
LanceSetPacketFilter(
    IN PLANCE_ADAPTER Adapter,
    IN NDIS_REQUEST_TYPE NdisRequestType,
    IN UINT PacketFilter
    );

NDIS_STATUS
LanceChangeMulticastAddresses(
    IN PLANCE_ADAPTER Adapter,
    IN UINT NewAddressCount,
    IN CHAR NewAddresses[][LANCE_LENGTH_OF_ADDRESS],
    IN NDIS_REQUEST_TYPE NdisRequestType
    );

VOID
DeleteAdapterMemory(
    IN PLANCE_ADAPTER Adapter
    );

STATIC
BOOLEAN
LancePciHardwareDetails(
    IN PLANCE_ADAPTER Adapter
    );

STATIC
NDIS_STATUS
LanceQueryPciResources(
    IN NDIS_HANDLE ConfigurationHandle,
    IN PLANCE_ADAPTER Adapter
    );

STATIC
VOID
LanceIdentifyChip(
    IN PLANCE_ADAPTER Adapter
    );

STATIC
VOID
LanceConfigureChip(
    IN PLANCE_ADAPTER Adapter
    );

STATIC
BOOLEAN
LanceCheckWioMode(
    IN PLANCE_ADAPTER Adapter
    );

VOID
RelinquishReceivePacket(
    IN PLANCE_ADAPTER Adapter,
    IN UINT StartingIndex,
    IN UINT NumberOfBuffers
    );

BOOLEAN
ProcessReceiveInterrupts(
    IN PLANCE_ADAPTER Adapter
    );

NDIS_STATUS
LanceRegisterAdapter(
    IN PLANCE_ADAPTER Adapter
    );

BOOLEAN
ProcessTransmitInterrupts(
    IN PLANCE_ADAPTER Adapter
    );

UINT
CalculateCRC(
    IN UINT NumberOfBytes,
    IN PCHAR Input
    );

VOID
LanceStartChip(
    IN PLANCE_ADAPTER Adapter
    );

VOID
LanceSetInitializationBlock(
    IN PLANCE_ADAPTER Adapter
    );

VOID
SetInitBlockAndInit(
    IN PLANCE_ADAPTER Adapter
    );

STATIC
BOOLEAN
LanceSyncStartInit(
    IN PVOID Context
    );

STATIC
BOOLEAN
LanceSyncPollInitDone(
    IN PVOID Context
    );

VOID
StartAdapterReset(
    IN PLANCE_ADAPTER Adapter
    );

VOID
SetupForReset(
    IN PLANCE_ADAPTER Adapter,
    IN NDIS_REQUEST_TYPE RequestType
    );

STATIC
VOID
LanceQueueSetBehindReset(
    IN PLANCE_ADAPTER Adapter
    );

NDIS_STATUS
LanceInitialInit(
    IN PLANCE_ADAPTER Adapter
    );

DRIVER_INITIALIZE DriverEntry;



#pragma NDIS_INIT_FUNCTION(DriverEntry)


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the primary initialization routine for the lance driver.
    It is simply responsible for the intializing the wrapper and registering
    the MAC.  It then calls a system and architecture specific routine that
    will initialize and register each adapter.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The status of the operation.

--*/

{
    //
    // Receives the status of the NdisRegisterMac operation.
    //
    NDIS_STATUS Status;
    NDIS_HANDLE NdisWrapperHandle;
    NDIS_MINIPORT_CHARACTERISTICS LanceChar;
    NDIS_STRING MacName = NDIS_STRING_CONST("Lance");
    UNICODE_STRING RoutineName;

    //
    // Optional kernel routine, see LanceFlushQueuedDpcs.  Cast through
    // ULONG_PTR: a data-to-function pointer cast is a warning.
    //
    RtlInitUnicodeString(&RoutineName, L"KeFlushQueuedDpcs");
    LanceKeFlushQueuedDpcs = (LANCE_FLUSH_QUEUED_DPCS)(ULONG_PTR)
        MmGetSystemRoutineAddress(&RoutineName);

    //
    // Initialize the wrapper.
    //
    NdisInitializeWrapper(
        &NdisWrapperHandle,
        DriverObject,
        RegistryPath,
        NULL
    );

    //
    // Initialize the MAC characteristics for the call to
    // NdisRegisterMac.
    //
    NdisZeroMemory(&LanceChar, sizeof(LanceChar));
    LanceChar.MajorNdisVersion = LANCE_NDIS_MAJOR_VERSION;
    LanceChar.MinorNdisVersion = LANCE_NDIS_MINOR_VERSION;
    LanceChar.CheckForHangHandler = LanceCheckForHang;
    LanceChar.DisableInterruptHandler = LanceDisableInterrupt;
    LanceChar.EnableInterruptHandler = LanceEnableInterrupt;
    LanceChar.HaltHandler = LanceHalt;
    LanceChar.HandleInterruptHandler = LanceHandleInterrupt;
    LanceChar.InitializeHandler = LanceInitialize;
    LanceChar.ISRHandler = LanceIsr;
    LanceChar.QueryInformationHandler = LanceQueryInformation;
    LanceChar.ReconfigureHandler = NULL;
    LanceChar.ResetHandler = LanceReset;
    LanceChar.SendHandler = LanceSend;
    LanceChar.SetInformationHandler = LanceSetInformation;
    LanceChar.TransferDataHandler = LanceTransferData;

    //
    // NDIS 5.1 additions.  There is no SendPackets handler, so there is
    // nothing for CancelSendPackets to cancel.
    //
    LanceChar.AdapterShutdownHandler = LanceShutdown;
    LanceChar.PnPEventNotifyHandler = LancePnPEventNotify;
    LanceChar.CancelSendPacketsHandler = NULL;

    Status = NdisMRegisterMiniport(
                 NdisWrapperHandle,
                 &LanceChar,
                 sizeof(LanceChar)
             );
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NdisTerminateWrapper(NdisWrapperHandle, NULL);
    }

    return(Status);
}

#pragma NDIS_PAGEABLE_FUNCTION(LanceInitialize)

extern
NDIS_STATUS
LanceInitialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE ConfigurationHandle
    )

/*++

Routine Description:

    NE3200Initialize starts an adapter.

Arguments:

    See NDIS 5.1 Miniport spec.

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING

--*/

{
    //
    // Pointer for the adapter root.
    //
    PLANCE_ADAPTER Adapter;


    NDIS_HANDLE ConfigHandle;
    PNDIS_CONFIGURATION_PARAMETER ReturnedValue;
    NDIS_STRING MaxMulticastListStr = MAXMULTICASTLIST;

    UINT MaxMulticastList = 32;
    PVOID NetAddress;
    UINT Length = 0;

    ULONG PciId;
    USHORT PciCommand;
    NDIS_STATUS PciResourceStatus;

    NDIS_STATUS Status;

    PAGED_CODE();

    //
    // Search for correct medium.
    //

    for (; MediumArraySize > 0; MediumArraySize--){

        if (MediumArray[MediumArraySize - 1] == NdisMedium802_3){

            MediumArraySize--;

            break;

        }

    }

    if (MediumArray[MediumArraySize] != NdisMedium802_3){

        return( NDIS_STATUS_UNSUPPORTED_MEDIA );

    }

    *SelectedMediumIndex = MediumArraySize;

    //
    // Allocate the Adapter block.
    //

    LANCE_ALLOC_PHYS(&Adapter, sizeof(LANCE_ADAPTER));
    if (Adapter == NULL)
    {
        return( NDIS_STATUS_RESOURCES ) ;
    }

    LANCE_ZERO_MEMORY(Adapter, sizeof(LANCE_ADAPTER));

    Adapter->MaxLookAhead = LANCE_MAX_LOOKAHEAD;
    Adapter->MediaConnected = TRUE;

    Adapter->MiniportAdapterHandle = MiniportAdapterHandle;
    Adapter->BeingRemoved = FALSE;

    //
    // Every PCnet-PCI/FAST part enumerates as 1022:2000 (Am79C978 HomePNA
    // as 1022:2001).  The chip itself is identified from CSR88/CSR89 once
    // it has been reset, in LanceRegisterAdapter.
    //
    PciId = 0;
    if (NdisReadPciSlotInformation(
            MiniportAdapterHandle,
            0,
            0,
            &PciId,
            sizeof(PciId)) == sizeof(PciId) &&
        (USHORT)(PciId & 0xffff) == PCNET_PCI_VENDOR_ID &&
        ((USHORT)(PciId >> 16) == PCNET_PCI_DEVICE_ID ||
         (USHORT)(PciId >> 16) == PCNET_HOME_PCI_DEVICE_ID))
    {
        Adapter->PciDeviceId = (USHORT)(PciId >> 16);
        PciCommand = 0;

        if (NdisReadPciSlotInformation(
                MiniportAdapterHandle,
                0,
                0x04,
                &PciCommand,
                sizeof(PciCommand)) != sizeof(PciCommand))
        {
            LANCE_FREE_PHYS(Adapter, sizeof(LANCE_ADAPTER));
            return(NDIS_STATUS_ADAPTER_NOT_FOUND);
        }

        PciCommand |= 0x0005;
        if (NdisWritePciSlotInformation(
                MiniportAdapterHandle,
                0,
                0x04,
                &PciCommand,
                sizeof(PciCommand)) != sizeof(PciCommand))
        {
            LANCE_FREE_PHYS(Adapter, sizeof(LANCE_ADAPTER));
            return(NDIS_STATUS_ADAPTER_NOT_FOUND);
        }

        NdisMSetAttributesEx(
            MiniportAdapterHandle,
            (NDIS_HANDLE)Adapter,
            0,
            NDIS_ATTRIBUTE_BUS_MASTER,
            NdisInterfacePci
            );

        PciResourceStatus = LanceQueryPciResources(
                                ConfigurationHandle,
                                Adapter
                                );
        if (PciResourceStatus != NDIS_STATUS_SUCCESS)
        {
            LANCE_FREE_PHYS(Adapter, sizeof(LANCE_ADAPTER));
            return(PciResourceStatus);
        }
    }
    else
    {
        LANCE_FREE_PHYS(Adapter, sizeof(LANCE_ADAPTER));
        return(NDIS_STATUS_ADAPTER_NOT_FOUND);
    }

    NdisOpenConfiguration(&Status, &ConfigHandle, ConfigurationHandle);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        LANCE_FREE_PHYS(Adapter, sizeof(LANCE_ADAPTER));
        return(Status);
    }

    //
    // Read MaxMulticastList
    //
    NdisReadConfiguration(
        &Status,
        &ReturnedValue,
        ConfigHandle,
        &MaxMulticastListStr,
        NdisParameterInteger
    );
    if (Status == NDIS_STATUS_SUCCESS)
    {
        MaxMulticastList = ReturnedValue->ParameterData.IntegerData;
        if (MaxMulticastList > LANCE_MAX_MULTICAST_LIST)
        {
            MaxMulticastList = LANCE_MAX_MULTICAST_LIST;
        }
    }

    //
    // Read net address
    //
    NdisReadNetworkAddress(
        &Status,
        &NetAddress,
        &Length,
        ConfigHandle
    );

    //
    // Use the NetworkAddress override only if it is a unicast address.
    // A group address (which includes all-ones) as the station address
    // would make peers drop our ARP replies, and all-ones would make the
    // chip treat broadcasts as addressed to it.  An all-zero override is
    // replaced with the address PROM's later, like no override at all.
    //
    if ((Status == NDIS_STATUS_SUCCESS) &&
        (Length == LANCE_LENGTH_OF_ADDRESS) &&
        !(((PUCHAR)NetAddress)[0] & 0x01))
    {
        NdisMoveMemory(
            Adapter->CurrentNetworkAddress,
            NetAddress,
            LANCE_LENGTH_OF_ADDRESS
        );
    }

    NdisCloseConfiguration(ConfigHandle);
    //
    // Register the IoPortRanges
    //

    {
        //
        // NdisMRegisterIoPortRange writes a PVOID.  IoPortBase is a ULONG,
        // so on x64 passing its address let the call write 8 bytes over
        // it and the fields after it.  On x86 and x64 the mapped value is
        // the I/O port number itself, so the ULONG holds it.
        //
        PVOID MappedPortBase = NULL;

        Status = NdisMRegisterIoPortRange(
            &MappedPortBase,
            MiniportAdapterHandle,
            Adapter->IoBaseAddr,
            0x20
            );

        Adapter->IoPortBase = (ULONG)(ULONG_PTR)MappedPortBase;
    }

    Adapter->RAP = Adapter->IoPortBase + 0x12;
    Adapter->RDP = Adapter->IoPortBase + 0x10;
    Adapter->BDP = Adapter->IoPortBase + 0x16;
    Adapter->NetworkHardwareAddress = Adapter->IoPortBase;

    if (Status != NDIS_STATUS_SUCCESS) {

        LANCE_FREE_PHYS(Adapter, sizeof(LANCE_ADAPTER));

        return Status;

    }

    Adapter->IoPortsRegistered = TRUE;

    //
    // Set the port addresses and the network address.
    //

    Adapter->MaxMulticastList = (UCHAR)MaxMulticastList;

    Status = LanceRegisterAdapter( Adapter );

    if (Status != NDIS_STATUS_SUCCESS) {

        goto Fail1;
    }

    return Status;

Fail1:

    //
    // Deregister the IoPortRanges
    //

    if (Adapter->IoPortsRegistered) {
        NdisMDeregisterIoPortRange(
            MiniportAdapterHandle,
            Adapter->IoBaseAddr,
            0x20,
            (PVOID)(Adapter->IoPortBase)
            );
    }

    LANCE_FREE_PHYS(Adapter, sizeof(LANCE_ADAPTER));

    return(Status);
}


VOID
LanceHalt(
    IN PVOID MiniportAdapterContext
    )
/*++

Routine Description:

    LanceHalt stops an adapter and deregisters everything.

Arguments:

    MiniportAdapterContext - The context value that the driver when
    LanceInitialize is called. Actually as pointer to an
    LANCE_ADAPTER.

Return Value:

    None.

--*/
{

    PLANCE_ADAPTER Adapter;
    BOOLEAN TimerCancelled;

    Adapter = PLANCE_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    LOG(REMOVE);

    //
    // 0. Halt does not hold the miniport lock, so on another CPU the
    //    deferred timer can be running a pending reset right now.  Left
    //    alone, it could INIT the chip again after step 1 stops it, or
    //    synchronize with the interrupt after step 2 has deregistered it.
    //    Block any new reset or timer, then wait for one that is already
    //    running to finish.
    //
    Adapter->Halting = TRUE;
    Adapter->ResetInProgress = FALSE;
    KeMemoryBarrier();

    NdisMCancelTimer(&Adapter->DeferredTimer, &TimerCancelled);
    LanceFlushQueuedDpcs();

    //
    // 1. Stop the chip, synchronized with the ISR so the RAP write cannot
    //    land between the ISR's RAP save and restore on another CPU.
    //    After a surprise removal there is no chip to stop.
    //
    if (!Adapter->SurpriseRemoved)
    {
        NdisMSynchronizeWithInterrupt(
            &Adapter->Interrupt,
            LanceSyncStopChip,
            (PVOID)Adapter
            );
    }

    //
    // 2. Disconnect the interrupt.  This also waits for a DPC that is
    //    already queued or running.
    //
    NdisMDeregisterInterrupt(&(Adapter->Interrupt));

    //
    // 3. Nothing can arm the timer since step 0 (LANCE_DO_DEFERRED checks
    //    Halting).  Cancel it once more and flush, so nothing that was
    //    already queued can run after the memory is freed.
    //
    NdisMCancelTimer(&Adapter->DeferredTimer, &TimerCancelled);
    LanceFlushQueuedDpcs();

    //
    // 4. With the chip stopped and nothing left to call back into us,
    //    release the DMA memory.
    //
    if (Adapter->SharedMemoryAllocated)
    {
        MmFreeContiguousMemory(Adapter->MmMappedBaseAddr);
        Adapter->SharedMemoryAllocated = FALSE;
    }

    DeleteAdapterMemory(Adapter);

    //
    // 5. The I/O range goes last: nothing above touches the ports any more.
    //
    if (Adapter->IoPortsRegistered) {
        NdisMDeregisterIoPortRange(
            Adapter->MiniportAdapterHandle,
            Adapter->IoBaseAddr,
            0x20,
            (PVOID)(Adapter->IoPortBase)
            );
        Adapter->IoPortsRegistered = FALSE;
    }

    NdisFreeMemory(Adapter, sizeof(LANCE_ADAPTER), 0);

    return;
}


STATIC
VOID
LanceFlushQueuedDpcs(
    VOID
    )

/*++

Routine Description:

    Waits for DPCs (including timer callbacks) that are already queued
    or running to finish.  PASSIVE_LEVEL only.

    Without KeFlushQueuedDpcs (Windows XP before SP2) there is no exact
    equivalent, so it gives a DPC running on another CPU time to finish.

--*/

{
    if (LanceKeFlushQueuedDpcs != NULL)
    {
        LanceKeFlushQueuedDpcs();
    }
    else
    {
        NdisMSleep(20000);
    }
}


STATIC
BOOLEAN
LanceSyncReadBcr4(
    IN PVOID Context
    )

/*++

Routine Description:

    Reads BCR4 with the ISR locked out, then puts RAP back on CSR0,
    where the rest of the driver expects it.

--*/

{
    PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)Context;
    USHORT Bcr4;

    LANCE_ISR_WRITE_RAP(Adapter, LANCE_SELECT_BCR4);
    NdisRawReadPortUshort(Adapter->BDP, &Bcr4);
    LANCE_ISR_WRITE_RAP(Adapter, LANCE_SELECT_CSR0);

    Adapter->Bcr4 = Bcr4;

    return TRUE;
}


BOOLEAN
LanceCheckForHang(
    IN NDIS_HANDLE MiniportAdapterContext
    )

/*++

Routine Description:

    NDIS calls this about every 2 seconds.  The driver has no hang
    detection of its own (NDIS already resets it when sends or requests
    time out), so it always returns FALSE.  It uses the call to poll the
    link status in BCR4 and tells NDIS when the cable is unplugged or
    plugged back in.

    A disconnect is only reported after BCR4 has shown the link up at
    least once.  If a chip or emulator doesn't implement the link bit,
    the adapter therefore stays "connected", as it always was before,
    instead of being stuck offline.

--*/

{
    PLANCE_ADAPTER Adapter =
        PLANCE_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);
    USHORT Bcr4;
    BOOLEAN LinkUp;

    //
    // A reset is running.  Sends get RESOURCES until it finishes, so
    // no send arms the timer, and if its IDON interrupt went missing
    // nothing else would run the DPC again.  Arm the timer so the DPC
    // looks at CSR0 itself (LanceSyncReadInterruptStatus).
    //
    if (Adapter->ResetInProgress)
    {
        LANCE_DO_DEFERRED(Adapter);
        return FALSE;
    }

    //
    // The original PCnet-PCI (79C970) is left out: its BCR4 is not
    // documented the same way.
    //
    if (Adapter->HardwareFailure ||
        Adapter->SurpriseRemoved ||
        Adapter->Halting ||
        (Adapter->ChipPart == PCNET_PART_79C970) ||
        (Adapter->ChipPart == PCNET_PART_79C970_ALT))
    {
        return FALSE;
    }

    NdisMSynchronizeWithInterrupt(
        &Adapter->Interrupt,
        LanceSyncReadBcr4,
        (PVOID)Adapter
        );

    Bcr4 = Adapter->Bcr4;

    //
    // LEDOUT is the link status only when LNKSTE is the only LED source
    // enabled in bits 6:0 and the polarity isn't inverted (the reset
    // default, 0x00C0).
    //
    if ((Bcr4 == 0xffff) ||
        (Bcr4 & LANCE_BCR4_LEDPOL) ||
        ((Bcr4 & LANCE_BCR4_SOURCES_6_0) != LANCE_BCR4_LNKSTE))
    {
        return FALSE;
    }

    LinkUp = (BOOLEAN)((Bcr4 & LANCE_BCR4_LEDOUT) != 0);

    if (LinkUp)
    {
        Adapter->LinkStatusTrusted = TRUE;
    }
    else if (!Adapter->LinkStatusTrusted)
    {
        return FALSE;
    }

    if (LinkUp != Adapter->MediaConnected)
    {
        Adapter->MediaConnected = LinkUp;

        NdisMIndicateStatus(
            Adapter->MiniportAdapterHandle,
            LinkUp ? NDIS_STATUS_MEDIA_CONNECT : NDIS_STATUS_MEDIA_DISCONNECT,
            NULL,
            0
            );
        NdisMIndicateStatusComplete(Adapter->MiniportAdapterHandle);
    }

    return FALSE;
}


VOID
LanceShutdown(
    IN PVOID ShutdownContext
    )

/*++

Routine Description:

    NDIS 5.1 AdapterShutdownHandler.  Called on system shutdown or
    restart, and possibly from the bugcheck path at high IRQL, so it only
    touches the chip: no NDIS calls, no locks, no memory is freed.  It
    stops the chip so that it does no more bus-master DMA into memory the
    next OS (or the crash dump writer) is about to use.

--*/

{
    PLANCE_ADAPTER Adapter = PLANCE_ADAPTER_FROM_CONTEXT_HANDLE(ShutdownContext);

    if (Adapter->SurpriseRemoved) {
        return;
    }

    NdisRawWritePortUshort(Adapter->RAP, LANCE_SELECT_CSR0);
    NdisRawWritePortUshort(Adapter->RDP, LANCE_CSR0_STOP);
}


VOID
LancePnPEventNotify(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_DEVICE_PNP_EVENT PnPEvent,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength
    )

/*++

Routine Description:

    NDIS 5.1 PnPEventNotifyHandler.  The only event that matters to this
    driver is surprise removal: after it, the I/O ports read as all ones
    and LanceHalt must not try to stop the chip.  The power profile
    notification is informational only.

--*/

{
    PLANCE_ADAPTER Adapter = PLANCE_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    UNREFERENCED_PARAMETER(InformationBuffer);
    UNREFERENCED_PARAMETER(InformationBufferLength);

    switch (PnPEvent)
    {
        case NdisDevicePnPEventSurpriseRemoved:
            Adapter->SurpriseRemoved = TRUE;
            break;

        default:
            break;
    }
}

#pragma NDIS_PAGEABLE_FUNCTION(LancePciHardwareDetails)

STATIC
BOOLEAN
LancePciHardwareDetails(
    IN PLANCE_ADAPTER Adapter
    )
{
    UINT i;
    BOOLEAN AllZero = TRUE;
    BOOLEAN AllOnes = TRUE;

    PAGED_CODE();

    for (i = 0; i < LANCE_LENGTH_OF_ADDRESS; i++)
    {
        NdisRawReadPortUchar(
            Adapter->NetworkHardwareAddress + i,
            (PUCHAR)&Adapter->NetworkAddress[i]
            );

        if (Adapter->NetworkAddress[i] != 0)
        {
            AllZero = FALSE;
        }

        if ((UCHAR)Adapter->NetworkAddress[i] != 0xff)
        {
            AllOnes = FALSE;
        }
    }

    return((BOOLEAN)(!AllZero &&
                     !AllOnes &&
                     !(Adapter->NetworkAddress[0] & 0x01)));
}

STATIC
NDIS_STATUS
LanceQueryPciResources(
    IN NDIS_HANDLE ConfigurationHandle,
    IN PLANCE_ADAPTER Adapter
    )
{
    PNDIS_RESOURCE_LIST ResourceList = NULL;
    ULONG ResourceListSize = 0;
    ULONG AllocatedResourceListSize;
    ULONG DescriptorOffset;
    ULONG i;
    BOOLEAN IoPortFound = FALSE;
    BOOLEAN InterruptFound = FALSE;
    NDIS_STATUS Status;

    NdisMQueryAdapterResources(
        &Status,
        ConfigurationHandle,
        NULL,
        &ResourceListSize
        );
    if (Status != NDIS_STATUS_RESOURCES ||
        ResourceListSize < FIELD_OFFSET(NDIS_RESOURCE_LIST, PartialDescriptors))
    {
        return(NDIS_STATUS_ADAPTER_NOT_FOUND);
    }

    AllocatedResourceListSize = ResourceListSize;
    LANCE_ALLOC_PHYS(&ResourceList, ResourceListSize);
    if (ResourceList == NULL)
    {
        return(NDIS_STATUS_RESOURCES);
    }

    NdisMQueryAdapterResources(
        &Status,
        ConfigurationHandle,
        ResourceList,
        &ResourceListSize
        );
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NdisFreeMemory(ResourceList, AllocatedResourceListSize, 0);
        return(Status);
    }

    DescriptorOffset =
        FIELD_OFFSET(NDIS_RESOURCE_LIST, PartialDescriptors);
    if (ResourceListSize < DescriptorOffset ||
        ResourceList->Count >
            (ResourceListSize - DescriptorOffset) /
            sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR))
    {
        NdisFreeMemory(ResourceList, AllocatedResourceListSize, 0);
        return(NDIS_STATUS_ADAPTER_NOT_FOUND);
    }

    for (i = 0; i < ResourceList->Count; i++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor =
            &ResourceList->PartialDescriptors[i];

        if (Descriptor->Type == CmResourceTypePort &&
            Descriptor->u.Port.Start.HighPart == 0 &&
            Descriptor->u.Port.Length >= 0x20)
        {
            Adapter->IoBaseAddr = Descriptor->u.Port.Start.LowPart;
            IoPortFound = TRUE;
        }
        else if (Descriptor->Type == CmResourceTypeInterrupt)
        {
            Adapter->InterruptNumber = Descriptor->u.Interrupt.Vector;
            Adapter->InterruptRequestLevel = Descriptor->u.Interrupt.Level;
            InterruptFound = TRUE;
        }
    }

    NdisFreeMemory(ResourceList, AllocatedResourceListSize, 0);

    return((IoPortFound && InterruptFound) ?
           NDIS_STATUS_SUCCESS :
           NDIS_STATUS_ADAPTER_NOT_FOUND);
}

#pragma NDIS_PAGEABLE_FUNCTION(LanceIdentifyChip)

STATIC
VOID
LanceIdentifyChip(
    IN PLANCE_ADAPTER Adapter
    )
{
    USHORT Low;
    USHORT High;

    PAGED_CODE();

    LANCE_WRITE_RAP(Adapter, LANCE_SELECT_CSR88);
    LANCE_READ_RDP(Adapter, &Low);
    LANCE_WRITE_RAP(Adapter, LANCE_SELECT_CSR89);
    LANCE_READ_RDP(Adapter, &High);
    LANCE_WRITE_RAP(Adapter, LANCE_SELECT_CSR0);

    Adapter->ChipId = ((ULONG)High << 16) | Low;
    Adapter->ChipPart = PCNET_CHIP_PART(Adapter->ChipId);

    switch (Adapter->ChipPart)
    {
        case PCNET_PART_79C970:
        case PCNET_PART_79C970_ALT:
            Adapter->ChipName = "Am79C970 (PCnet-PCI)";
            break;
        case PCNET_PART_79C970A:
            Adapter->ChipName = "Am79C970A (PCnet-PCI II)";
            break;
        case PCNET_PART_79C971:
            Adapter->ChipName = "Am79C971 (PCnet-FAST)";
            break;
        case PCNET_PART_79C972:
            Adapter->ChipName = "Am79C972 (PCnet-FAST+)";
            break;
        case PCNET_PART_79C973:
            Adapter->ChipName = "Am79C973 (PCnet-FAST III)";
            break;
        case PCNET_PART_79C975:
            Adapter->ChipName = "Am79C975 (PCnet-FAST III)";
            break;
        case PCNET_PART_79C978:
            Adapter->ChipName = "Am79C978 (PCnet-Home)";
            break;
        default:
            Adapter->ChipName = "unknown PCnet";
            break;
    }

    DbgPrint("pcntn5m: %s, chip ID 0x%08lx, PCI 1022:%04x, I/O 0x%lx, IRQ %u\n",
             Adapter->ChipName,
             Adapter->ChipId,
             Adapter->PciDeviceId,
             Adapter->IoBaseAddr,
             Adapter->InterruptNumber);
}

#pragma NDIS_PAGEABLE_FUNCTION(LanceCheckWioMode)

STATIC
BOOLEAN
LanceCheckWioMode(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    Confirms, right after the software reset, that the chip is in 16-bit
    word I/O (WIO) mode: RDP at +0x10, RAP at +0x12, reset at +0x14 and BDP
    at +0x16, all 16 bits wide.  This driver only ever uses WIO.

    The chip leaves WIO for 32-bit DWIO as soon as anything does a 32-bit
    write to RDP, and only a hardware reset (PCI RST#) brings it back;
    reading the reset register does not.  A driver loaded earlier in the
    same boot (or a boot ROM) can therefore leave the chip in DWIO, where
    the 16-bit accesses below are undefined.

    After reset the chip is stopped (CSR0 = 0x0004) and RAP is readable,
    so both are checked.

Return Value:

    TRUE if the chip answers as expected in WIO mode.

--*/

{
    USHORT Csr0;
    USHORT Rap;

    PAGED_CODE();

    NdisRawWritePortUshort(Adapter->RAP, LANCE_SELECT_CSR88);
    NdisRawReadPortUshort(Adapter->RAP, &Rap);

    NdisRawWritePortUshort(Adapter->RAP, LANCE_SELECT_CSR0);
    NdisRawReadPortUshort(Adapter->RDP, &Csr0);

    if ((Rap == LANCE_SELECT_CSR88) && (Csr0 == LANCE_CSR0_STOP)) {
        return TRUE;
    }

    DbgPrint("pcntn5m: I/O 0x%lx does not answer in 16-bit WIO mode after "
             "reset (RAP readback 0x%04x, CSR0 0x%04x).  The chip may have "
             "been left in 32-bit DWIO mode by another driver; only a "
             "hardware reset (cold boot) clears that.\n",
             Adapter->IoPortBase, Rap, Csr0);

    return FALSE;
}

#pragma NDIS_PAGEABLE_FUNCTION(LanceConfigureChip)

STATIC
VOID
LanceConfigureChip(
    IN PLANCE_ADAPTER Adapter
    )
{
    USHORT Bcr18;

    PAGED_CODE();

    //
    // CSR4: enable DMAPLUS but keep the reset-default masks for the
    // missed-frame and collision counter overflow, transmit start and
    // jabber interrupts.  Nothing in this driver services or clears those
    // CSR4 status bits, so an unmasked source would hold INTR, and with
    // it the level-triggered interrupt line, asserted for good.  CSR4 and
    // BCR18 survive STOP, so this is done once, after reset.
    //
    LANCE_WRITE_RAP(Adapter, LANCE_SELECT_CSR4);
    LANCE_WRITE_RDP(Adapter,
                    (USHORT)(LANCE_CSR4_DMAPLUS |
                             LANCE_CSR4_MFCOM |
                             LANCE_CSR4_RCVCCOM |
                             LANCE_CSR4_TXSTRTM |
                             LANCE_CSR4_JABM));

    //
    // BCR18: enable burst reads and writes on the 79C970A and later,
    // leaving every other field (ROM timing, linear burst count, PHY
    // select, ...) as the reset/EEPROM set it.  The first-generation
    // 79C970 and unrecognised parts keep their defaults.
    //
    switch (Adapter->ChipPart)
    {
        case PCNET_PART_79C970A:
        case PCNET_PART_79C971:
        case PCNET_PART_79C972:
        case PCNET_PART_79C973:
        case PCNET_PART_79C975:
        case PCNET_PART_79C978:
            LANCE_WRITE_RAP(Adapter, LANCE_SELECT_BCR18);
            NdisRawReadPortUshort(Adapter->BDP, &Bcr18);
            Bcr18 &= ~LANCE_BCR18_DWIO;
            Bcr18 |= LANCE_BCR18_BREADE | LANCE_BCR18_BWRITE;
            NdisRawWritePortUshort(Adapter->BDP, Bcr18);
            break;

        default:
            break;
    }

    LANCE_WRITE_RAP(Adapter, LANCE_SELECT_CSR0);
}

#pragma NDIS_PAGEABLE_FUNCTION(LanceRegisterAdapter)

NDIS_STATUS
LanceRegisterAdapter(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    This routine is responsible for the allocation of the datastructures
    for the driver as well as any hardware specific details necessary
    to talk with the device.

Arguments:

    Adapter - Pointer to the adapter block.

Return Value:

    Returns false if anything occurred that prevents the initialization
    of the adapter.

--*/
{
    //
    // Result of Ndis Calls.
    //
    NDIS_STATUS Status;
    USHORT ResetValue;

    PAGED_CODE();

    //
    // We put in this assertion to make sure that ushort are 2 bytes.
    // if they aren't then the initialization block definition needs
    // to be changed.
    //
    // Also all of the logic that deals with status registers assumes
    // that control registers are only 2 bytes.
    //

    ASSERT(sizeof(USHORT) == 2);

    //
    // This assertion checks that the network address in the initialization
    // block does start on the third byte of the initalization block.
    //
    // If this is true then other fields in the initialization block
    // and the send and receive descriptors should be at their correct
    // locations.
    //

#ifdef NDIS_NT
    ASSERT(FIELD_OFFSET(LANCE_INITIALIZATION_BLOCK,PhysicalAddress[0]) == 2);
#else
    ASSERT(&((PLANCE_INITIALIZATION_BLOCK)0)->PhysicalAddress[0] == (PVOID)2);
#endif //NDIS_NT

    //
    // Allocate memory for all of the adapter structures.
    //

    Adapter->NumberOfTransmitRings = LANCE_NUMBER_OF_TRANSMIT_RINGS;
    Adapter->LogNumberTransmitRings = LANCE_LOG_TRANSMIT_RINGS;

    Adapter->NumberOfReceiveRings = LANCE_64K_NUMBER_OF_RECEIVE_RINGS;
    Adapter->LogNumberReceiveRings = LANCE_64K_LOG_RECEIVE_RINGS;
    Adapter->SizeOfReceiveBuffer = LANCE_64K_SIZE_OF_RECEIVE_BUFFERS;
    Adapter->NumberOfSmallBuffers = LANCE_64K_NUMBER_OF_SMALL_BUFFERS;
    Adapter->NumberOfMediumBuffers = LANCE_64K_NUMBER_OF_MEDIUM_BUFFERS;
    Adapter->NumberOfLargeBuffers = LANCE_64K_NUMBER_OF_LARGE_BUFFERS;
    //
    // One block holds everything the chip touches: init block, rings,
    // receive buffers and transmit copy buffers, each carved out by
    // LANCE_ALLOCATE_MEMORY_FOR_HARDWARE in 8-byte steps.  About 60 KB.
    //
#define LANCE_HW_ROUND(S) ((((ULONG)(S)) + 7) & ~7UL)
    Adapter->AmountOfHardwareMemory =
        LANCE_HW_ROUND(sizeof(LANCE_INITIALIZATION_BLOCK)) +
        LANCE_HW_ROUND(sizeof(LANCE_TRANSMIT_ENTRY) * Adapter->NumberOfTransmitRings) +
        LANCE_HW_ROUND(sizeof(LANCE_RECEIVE_ENTRY) * Adapter->NumberOfReceiveRings) +
        LANCE_HW_ROUND(Adapter->SizeOfReceiveBuffer) * Adapter->NumberOfReceiveRings +
        LANCE_HW_ROUND(LANCE_SMALL_BUFFER_SIZE) * Adapter->NumberOfSmallBuffers +
        LANCE_HW_ROUND(LANCE_MEDIUM_BUFFER_SIZE) * Adapter->NumberOfMediumBuffers +
        LANCE_HW_ROUND(LANCE_LARGE_BUFFER_SIZE) * Adapter->NumberOfLargeBuffers;
#undef LANCE_HW_ROUND

    if (LancePciHardwareDetails(Adapter))
    {
        //
        // The block can go anywhere below 4 GB, as long as it does not
        // cross a 16 MB boundary: the chip gets bits 31:24 of every
        // address from CSR2 (see LANCE_DMA_WINDOW_SIZE).  Insisting on the
        // first 16 MB made this fail early in a cold boot on x64, when
        // that memory is mostly taken (Code 10).  One allocation of about
        // 60 KB replaces the 101 separate page-sized ones used before.
        //
        NDIS_PHYSICAL_ADDRESS LowestAcceptableAddress =
            NDIS_PHYSICAL_ADDRESS_CONST(0, 0);
        NDIS_PHYSICAL_ADDRESS HighestAcceptableAddress =
            NDIS_PHYSICAL_ADDRESS_CONST(0xFFFFFFFF, 0);
        NDIS_PHYSICAL_ADDRESS DmaWindowBoundary =
            NDIS_PHYSICAL_ADDRESS_CONST(LANCE_DMA_WINDOW_SIZE, 0);

        //
        // Get hold of the RAP and RDP address as well
        // as filling in the hardware assigned network
        // address.
        //

        if ((Adapter->CurrentNetworkAddress[0] == 0x00) &&
            (Adapter->CurrentNetworkAddress[1] == 0x00) &&
            (Adapter->CurrentNetworkAddress[2] == 0x00) &&
            (Adapter->CurrentNetworkAddress[3] == 0x00) &&
            (Adapter->CurrentNetworkAddress[4] == 0x00) &&
            (Adapter->CurrentNetworkAddress[5] == 0x00)) {

            Adapter->CurrentNetworkAddress[0] = Adapter->NetworkAddress[0];
            Adapter->CurrentNetworkAddress[1] = Adapter->NetworkAddress[1];
            Adapter->CurrentNetworkAddress[2] = Adapter->NetworkAddress[2];
            Adapter->CurrentNetworkAddress[3] = Adapter->NetworkAddress[3];
            Adapter->CurrentNetworkAddress[4] = Adapter->NetworkAddress[4];
            Adapter->CurrentNetworkAddress[5] = Adapter->NetworkAddress[5];

        }

        Adapter->MmMappedBaseAddr = NULL;
        Adapter->MmMappedBaseAddr = MmAllocateContiguousMemorySpecifyCache(
            Adapter->AmountOfHardwareMemory,
            LowestAcceptableAddress,
            HighestAcceptableAddress,
            DmaWindowBoundary,
            MmNonCached
            );

        if (Adapter->MmMappedBaseAddr != NULL)
        {
            Status = NDIS_STATUS_SUCCESS;
            Adapter->SharedMemoryPhysicalAddress =
                MmGetPhysicalAddress(Adapter->MmMappedBaseAddr);
            Adapter->SharedMemoryAllocated = TRUE;

            //
            // The boundary keeps the whole block inside one 16 MB window;
            // CSR2 gets its top byte.
            //
            ASSERT(NdisGetPhysicalAddressHigh(
                       Adapter->SharedMemoryPhysicalAddress) == 0);
            Adapter->DmaWindowBase =
                NdisGetPhysicalAddressLow(
                    Adapter->SharedMemoryPhysicalAddress) &
                LANCE_DMA_WINDOW_MASK;
        }
        else
        {
            Status = NDIS_STATUS_RESOURCES;
        }

        if (Status != NDIS_STATUS_SUCCESS) {

            NdisWriteErrorLogEntry(
                Adapter->MiniportAdapterHandle,
                NDIS_ERROR_CODE_RESOURCE_CONFLICT,
                0
                );

            return(Status);

        }

        Adapter->CurrentMemoryFirstFree = Adapter->MmMappedBaseAddr;


        Adapter->MemoryFirstUnavailable =
            (PUCHAR)(Adapter->CurrentMemoryFirstFree) +
            Adapter->AmountOfHardwareMemory;

        if (!AllocateAdapterMemory(Adapter)) {

            NdisWriteErrorLogEntry(
                Adapter->MiniportAdapterHandle,
                NDIS_ERROR_CODE_OUT_OF_RESOURCES,
                0
                );

            MmFreeContiguousMemory(Adapter->MmMappedBaseAddr);
            Adapter->SharedMemoryAllocated = FALSE;

            return(NDIS_STATUS_ADAPTER_NOT_FOUND);

        }

        Adapter->AllocateableRing = Adapter->TransmitRing;
        Adapter->TransmittingRing = Adapter->TransmitRing;
        Adapter->FirstUncommittedRing = Adapter->TransmitRing;
        Adapter->NumberOfAvailableRings = Adapter->NumberOfTransmitRings;
        Adapter->LastTransmitRingEntry = Adapter->TransmitRing +
            (Adapter->NumberOfTransmitRings-1);

        Adapter->CurrentReceiveIndex = 0;
        Adapter->OutOfReceiveBuffers = 0;
        Adapter->CRCError = 0;
        Adapter->FramingError = 0;
        Adapter->RetryFailure = 0;
        Adapter->LostCarrier = 0;
        Adapter->LateCollision = 0;
        Adapter->UnderFlow = 0;
        Adapter->Deferred = 0;
        Adapter->OneRetry = 0;
        Adapter->MoreThanOneRetry = 0;
        Adapter->ResetInProgress = FALSE;
        Adapter->ResetInitStarted = FALSE;
        Adapter->FirstInitialization = TRUE;
        Adapter->HardwareFailure = FALSE;

        //
        // First we make sure that the device is stopped.  We call
        // directly since we don't have an Interrupt object yet.
        //

        NdisRawReadPortUshort(Adapter->IoPortBase + 0x14, &ResetValue);
        NdisStallExecution(1000);

        if (!LanceCheckWioMode(Adapter)) {

            NdisWriteErrorLogEntry(
                Adapter->MiniportAdapterHandle,
                NDIS_ERROR_CODE_HARDWARE_FAILURE,
                1,
                (ULONG)Adapter->IoPortBase
                );

            MmFreeContiguousMemory(Adapter->MmMappedBaseAddr);
            Adapter->SharedMemoryAllocated = FALSE;

            DeleteAdapterMemory(Adapter);

            return NDIS_STATUS_ADAPTER_NOT_FOUND;
        }

        NdisRawWritePortUshort(Adapter->RAP, LANCE_SELECT_BCR20);
        NdisRawWritePortUshort(Adapter->BDP, 0);

        LanceIdentifyChip(Adapter);

        LanceConfigureChip(Adapter);

        LanceSyncStopChip(Adapter);

        //
        // The timer is set from the DPC (LANCE_DO_DEFERRED), so it has to
        // exist before the interrupt can fire.
        //

        NdisMInitializeTimer(
            &Adapter->DeferredTimer,
            Adapter->MiniportAdapterHandle,
            LanceDeferredTimerRoutine,
            (PVOID)Adapter
            );

        //
        // Initialize the interrupt.
        //

        Status = NdisMRegisterInterrupt(
                     &Adapter->Interrupt,
                     Adapter->MiniportAdapterHandle,
                     Adapter->InterruptNumber,
                     Adapter->InterruptRequestLevel,
                     TRUE,
                     TRUE,
                     NdisInterruptLevelSensitive
                     );

        if (Status != NDIS_STATUS_SUCCESS){

            NdisWriteErrorLogEntry(
                Adapter->MiniportAdapterHandle,
                NDIS_ERROR_CODE_INTERRUPT_CONNECT,
                0
                );

            MmFreeContiguousMemory(Adapter->MmMappedBaseAddr);
            Adapter->SharedMemoryAllocated = FALSE;

            DeleteAdapterMemory(Adapter);

            return Status;
        }

        if ((Status = LanceInitialInit(Adapter)) != NDIS_STATUS_SUCCESS) {


            NdisWriteErrorLogEntry(
                Adapter->MiniportAdapterHandle,
                NDIS_ERROR_CODE_HARDWARE_FAILURE,
                0
                );

            //
            // The chip may still be running the INIT we gave it, with the
            // init block and rings in shared memory.  Stop it, then take
            // the interrupt away, and only then free the memory it could
            // be DMAing to.
            //

            NdisMSynchronizeWithInterrupt(
                &Adapter->Interrupt,
                LanceSyncStopChip,
                (PVOID)Adapter
                );

            NdisMDeregisterInterrupt(&Adapter->Interrupt);

            {
                BOOLEAN TimerCancelled;
                Adapter->ResetInProgress = FALSE;
                NdisMCancelTimer(&Adapter->DeferredTimer, &TimerCancelled);
                if (!TimerCancelled)
                {
                    LanceFlushQueuedDpcs();
                    NdisMCancelTimer(&Adapter->DeferredTimer, &TimerCancelled);
                    if (!TimerCancelled)
                    {
                        LanceFlushQueuedDpcs();
                    }
                }
            }

            MmFreeContiguousMemory(Adapter->MmMappedBaseAddr);
            Adapter->SharedMemoryAllocated = FALSE;

            DeleteAdapterMemory(Adapter);

        }

        return Status;

    } else {

        NdisWriteErrorLogEntry(
            Adapter->MiniportAdapterHandle,
            NDIS_ERROR_CODE_ADAPTER_NOT_FOUND,
            0
            );

        return NDIS_STATUS_FAILURE;

    }

}


#pragma NDIS_PAGEABLE_FUNCTION(LanceInitialInit)

NDIS_STATUS
LanceInitialInit(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    This routine sets up the initial init of the driver.

Arguments:

    Adapter - The adapter for the hardware.

Return Value:

    None.

--*/

{
    UINT i;

    PAGED_CODE();

    SetInitBlockAndInit(Adapter);

    //
    // LanceIsr clears FirstInitialization when it sees IDON.  Initializing
    // takes microseconds, but early in a cold boot the interrupt can be
    // late or lost, which used to fail the start with Code 10.  So also
    // look at CSR0 ourselves every 10 ms, and give up after 2 seconds.
    //
    for (i = 0; i < 200; i++)
    {
        NdisMSleep(10000);

        if (NdisMSynchronizeWithInterrupt(
                &Adapter->Interrupt,
                LanceSyncPollInitDone,
                (PVOID)Adapter))
        {
            break;
        }
    }

    //
    // The only way that first initialization could have
    // been turned off is if we actually initialized.
    //
    if (!Adapter->FirstInitialization)
    {
        //
        // We can start the chip.  We may not
        // have any bindings to indicateto but this
        // is unimportant.
        //
        LanceStartChip(Adapter);
        return NDIS_STATUS_SUCCESS;
    }
    else
    {
        return(NDIS_STATUS_FAILURE);
    }

}


STATIC
VOID
LanceStartChip(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    This routine is used to start an already initialized lance.

Arguments:

    Adapter - The adapter for the LANCE to start.

Return Value:

    None.

--*/

{

    //
    // Never restart the chip once LanceHalt has begun: an IDON still
    // being handled in the DPC would otherwise start it again after
    // Halt stopped it, with its memory about to be freed.
    //
    if (Adapter->ResetInProgress || Adapter->Halting) {

        return;

    }

    //
    // Set the RAP to csr0.
    //

    LANCE_WRITE_RAP(
        Adapter,
        LANCE_SELECT_CSR0
        );

    //
    // Set the RDP to a start chip.
    //

    LANCE_WRITE_RDP(
        Adapter,
        LANCE_CSR0_START | LANCE_CSR0_INTERRUPT_ENABLE
        );

}

extern
VOID
LanceIsr(
    OUT PBOOLEAN InterruptRecognized,
    OUT PBOOLEAN QueueDpc,
    IN PVOID Context
    )

/*++

Routine Description:

    Interrupt service routine for the lance.

Arguments:

    Context - Really a pointer to the adapter.

Return Value:

    None.

--*/

{

    //
    // Will hold the value from the csr.
    //
    USHORT LocalCSR0Value;

    //
    // Holds the pointer to the adapter.
    //
    PLANCE_ADAPTER Adapter = Context;

    //
    // Holds the RAP value of whatever we interrupted.
    //
    USHORT SavedRap;

    LOG(IN_ISR);

    *QueueDpc = FALSE;
    *InterruptRecognized = FALSE;

    //
    // The PCI interrupt line can be shared, so this runs for other
    // devices' interrupts too.  Every RAP change outside this routine
    // happens before the interrupt is connected or under
    // NdisMSynchronizeWithInterrupt, so RAP is normally CSR0 here; save
    // and restore it anyway.
    //
    NdisRawReadPortUshort(Adapter->RAP, &SavedRap);
    LANCE_ISR_WRITE_RAP(Adapter, LANCE_SELECT_CSR0);
    LANCE_ISR_READ_RDP(Adapter, &LocalCSR0Value);

    //
    // 0xffff means the device is gone; INTR clear means it isn't ours.
    //
    if ((LocalCSR0Value == 0xffff) ||
        !(LocalCSR0Value & LANCE_CSR0_INTERRUPT_FLAG))
    {
        LANCE_ISR_WRITE_RAP(Adapter, SavedRap);
        LOG(OUT_ISR);
        return;
    }

    *InterruptRecognized = TRUE;

    //
    // It's our interrupt. Clear only those bits that we got in this
    // read of csr0, which drops the level-triggered line.  New causes
    // that arrive after the read stay set and interrupt again.
    //
    LANCE_ISR_WRITE_RDP(
        Adapter,
        (USHORT)((LANCE_CSR0_CLEAR_INTERRUPT_BITS & LocalCSR0Value) |
           LANCE_CSR0_INTERRUPT_ENABLE)
    );

    //
    // During MiniportInitialize, LanceInitialInit waits for IDON
    // through FirstInitialization.
    //
    if (Adapter->FirstInitialization &&
        (LocalCSR0Value & LANCE_CSR0_INITIALIZATION_DONE)
    )
    {
        Adapter->FirstInitialization = FALSE;
        LocalCSR0Value &= ~LANCE_CSR0_INITIALIZATION_DONE;
    }

    //
    // Hand the acknowledged causes to LanceHandleInterrupt, which
    // collects them with LanceSyncReadInterruptStatus.
    //
    if (LocalCSR0Value & LANCE_CSR0_CLEAR_INTERRUPT_BITS)
    {
        Adapter->PendingInterruptStatus |= LocalCSR0Value;
        *QueueDpc = TRUE;
    }

    LANCE_ISR_WRITE_RAP(Adapter, SavedRap);

    LOG(OUT_ISR);

    return;
}

STATIC
BOOLEAN
LanceSyncReadInterruptStatus(
    IN PVOID Context
    )
{
    PLANCE_INTERRUPT_STATUS Status = (PLANCE_INTERRUPT_STATUS)Context;
    PLANCE_ADAPTER Adapter = Status->Adapter;
    USHORT Csr;

    LANCE_ISR_WRITE_RAP(Adapter, LANCE_SELECT_CSR0);
    LANCE_ISR_READ_RDP(Adapter, &Csr);
    Status->Csr = (USHORT)((Adapter->PendingInterruptStatus &
                            ~LANCE_CSR0_TRANSMITTER_ON) |
                           (Csr & LANCE_CSR0_TRANSMITTER_ON));
    Adapter->PendingInterruptStatus = 0;

    //
    // While a reset's INIT is running, don't depend on the IDON
    // interrupt alone: if it is late or lost, nothing else would ever
    // finish the reset (the first init polls for the same reason, see
    // LanceSyncPollInitDone).  If CSR0 already shows IDON, acknowledge
    // it the way the ISR would and pass it on.
    //
    if (Adapter->ResetInitStarted &&
        (Csr != 0xffff) &&
        (Csr & LANCE_CSR0_INITIALIZATION_DONE))
    {
        LANCE_ISR_WRITE_RDP(
            Adapter,
            (USHORT)(LANCE_CSR0_INITIALIZATION_DONE | LANCE_CSR0_INTERRUPT_ENABLE)
        );

        Status->Csr |= LANCE_CSR0_INITIALIZATION_DONE;
    }

    return TRUE;
}

STATIC
BOOLEAN
LanceSyncPollInitDone(
    IN PVOID Context
    )

/*++

Routine Description:

    Used by LanceInitialInit, with the ISR locked out, to check for IDON
    without depending on the interrupt.  If CSR0 shows IDON before the
    ISR has seen it, acknowledge it the same way the ISR would.

Return Value:

    TRUE once the first initialization is done.

--*/

{
    PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)Context;
    USHORT Csr;

    if (!Adapter->FirstInitialization)
    {
        return TRUE;
    }

    LANCE_ISR_WRITE_RAP(Adapter, LANCE_SELECT_CSR0);
    LANCE_ISR_READ_RDP(Adapter, &Csr);

    if ((Csr == 0xffff) || !(Csr & LANCE_CSR0_INITIALIZATION_DONE))
    {
        return FALSE;
    }

    //
    // IDON is write-one-to-clear; the other command bits ignore a zero.
    //
    LANCE_ISR_WRITE_RDP(
        Adapter,
        (USHORT)(LANCE_CSR0_INITIALIZATION_DONE | LANCE_CSR0_INTERRUPT_ENABLE)
    );

    Adapter->FirstInitialization = FALSE;

    DbgPrint("PCnet: IDON found by polling CSR0 (0x%04x); "
             "the init interrupt was not delivered.\n", Csr);

    return TRUE;
}

STATIC
VOID
LanceDisableInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext
    )

/*++

Routine Description:

    This routine disables interrupts on the adapter.

Arguments:

    Context - Really a pointer to the adapter.

Return Value:

    None.

--*/

{
    PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)MiniportAdapterContext;

    LANCE_WRITE_RAP(Adapter, LANCE_SELECT_CSR0);
    LANCE_WRITE_RDP(Adapter, 0);
}


STATIC
VOID
LanceEnableInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext
    )

/*++

Routine Description:

    This routine enables interrupts on the adapter.

Arguments:

    Context - Really a pointer to the adapter.

Return Value:

    None.

--*/
{
    PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)MiniportAdapterContext;

    LANCE_WRITE_RAP(Adapter, LANCE_SELECT_CSR0);
    LANCE_WRITE_RDP(Adapter, LANCE_CSR0_INTERRUPT_ENABLE);
}
VOID
LanceDeferredTimerRoutine(
    IN PVOID SystemSpecific1,
    IN NDIS_HANDLE Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )

/*++

Routine Description:

    This DPC routine is used to handle deferred processing via a timer.

Arguments:

    Context - Really a pointer to the adapter.

Return Value:

    None.

--*/

{
    PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)Context;

    //
    // Nothing is armed after LanceHalt starts, but a callback that NDIS
    // queued before then can still arrive.  Don't let it synchronize
    // with an interrupt that LanceHalt may already have deregistered.
    //
    if (Adapter->Halting)
    {
        return;
    }

    LanceHandleInterrupt(Context);
}

VOID
LanceHandleInterrupt(
    IN NDIS_HANDLE Context
    )

/*++

Routine Description:

    This DPC routine is queued by the interrupt service routine.

Arguments:

    Context - Really a pointer to the adapter.

Return Value:

    None.

--*/

{
    //
    // Holds the pointer to the adapter.
    //
    PLANCE_ADAPTER Adapter = Context;

    //
    // Holds a value of csr0.
    //
    USHORT Csr = 0;
    LANCE_INTERRUPT_STATUS InterruptStatus;

    LOG(IN_DPC);

    InterruptStatus.Adapter = Adapter;

    //
    // Loop until there are no more processing sources.
    //
    for (;;)
    {
        //
        // Retrieve the interrupt causes acknowledged by the ISR.
        //
        NdisMSynchronizeWithInterrupt(
            &Adapter->Interrupt,
            LanceSyncReadInterruptStatus,
            &InterruptStatus
        );
        Csr = (USHORT)((Csr & ~LANCE_CSR0_TRANSMITTER_ON) | InterruptStatus.Csr);

        //
        // Check the interrupt source and other reasons
        // for processing.  If there are no reasons to
        // process then exit this loop.
        //
        // A reset that has not started is not a reason once LanceHalt
        // has begun: the reset will never start, so the loop would
        // never end.  SetupForReset refuses new resets after Halting is
        // set, but one can slip in just before it on another CPU.
        //
        if (((!Adapter->ResetInitStarted) &&
             ((Csr & (LANCE_CSR0_MEMORY_ERROR |
                  LANCE_CSR0_MISSED_PACKET |
                  LANCE_CSR0_BABBLE |
                  LANCE_CSR0_RECEIVER_INTERRUPT |
                  LANCE_CSR0_TRANSMITTER_INTERRUPT)) ||
              (Adapter->ResetInProgress && !Adapter->Halting))) ||
            (Csr & LANCE_CSR0_INITIALIZATION_DONE)
        )
        {

        }
        else
        {
            break;
        }

        //
        // Check for initialization.
        //
        // Note that we come out of the synchronization above holding
        // the spinlock.
        //
        if (Csr & LANCE_CSR0_INITIALIZATION_DONE)
        {
            //
            // Possibly undefined reason why the reset was requested.
            //
            // It is undefined if the adapter initiated the reset
            // request on its own.  It could do that if there
            // were some sort of error.
            //
            BOOLEAN CompleteSetInformation;
            BOOLEAN CompleteReset;

            LOG(RESET_STEP_3);

            ASSERT(!Adapter->FirstInitialization);

            Csr &= ~LANCE_CSR0_INITIALIZATION_DONE;

            Adapter->ResetInProgress = FALSE;
            Adapter->ResetInitStarted = FALSE;

            if (Adapter->ReinitAfterReset)
            {
                Adapter->ReinitAfterReset = FALSE;

                if (!Adapter->Halting)
                {
                    //
                    // A filter or multicast set came in after this INIT
                    // had already read the old init block (see
                    // LanceQueueSetBehindReset).  Reset once more to load
                    // it; the set completes at that IDON.
                    //
                    SetupForReset(Adapter, NdisRequestGeneric4);
                    goto LoopBottom;
                }
            }

            //
            // Take a copy and clear the flags first: completing a request
            // can lead NDIS straight back into us with another one that
            // starts a new reset.
            //
            CompleteSetInformation = Adapter->SetInformationPending;
            CompleteReset = Adapter->ResetPending;
            Adapter->SetInformationPending = FALSE;
            Adapter->ResetPending = FALSE;

            //
            // Restart the chip before completing anything, so that a
            // reset started from inside a completion is not undone by a
            // late LanceStartChip.
            //
            LanceStartChip(Adapter);

            if (CompleteSetInformation)
            {
                //
                // A filter or multicast change from a protocol.  The new
                // init block carried it, so it succeeded.
                //
                NdisMSetInformationComplete(
                    Adapter->MiniportAdapterHandle,
                    NDIS_STATUS_SUCCESS
                );
            }

            if (CompleteReset)
            {
                NdisMResetComplete(
                    Adapter->MiniportAdapterHandle,
                    NDIS_STATUS_SUCCESS,
                    FALSE
                );
            }

            //
            // LanceSend returned NDIS_STATUS_RESOURCES for every packet
            // offered during the reset, and the reset dropped whatever
            // was on the transmit ring, so no transmit interrupt is
            // coming to restart NDIS's send queue.  Restart it here.
            //
            NdisMSendResourcesAvailable(Adapter->MiniportAdapterHandle);

            goto LoopBottom;
        }

        //
        // If we have a reset in progress and the adapters reference
        // count is 1 (meaning no routine is in the interface and
        // we are the only "active" interrupt processing routine) then
        // it is safe to start the reset.
        //
        if (Adapter->ResetInProgress &&
            !Adapter->ResetInitStarted &&
            !Adapter->Halting
        )
        {
#if LANCE_TRACE
            DbgPrint("Starting Initialization.\n");
#endif
            StartAdapterReset(Adapter);

            Adapter->ResetInitStarted = TRUE;
            goto LoopBottom;
        }

        //
        // Check for non-packet related errors.
        //
        if (Csr & (LANCE_CSR0_MEMORY_ERROR |
                   LANCE_CSR0_MISSED_PACKET |
                   LANCE_CSR0_BABBLE)
        )
        {
            if (Csr & LANCE_CSR0_MISSED_PACKET)
            {
                Adapter->MissedPacket++;
            }
            if (Csr & LANCE_CSR0_BABBLE)
            {
                //
                // A babble error implies that we've sent a
                // packet that is greater than the ethernet length.
                // This implies that the driver is broken.
                //
                Adapter->Babble++;

                NdisWriteErrorLogEntry(
                    Adapter->MiniportAdapterHandle,
                    NDIS_ERROR_CODE_DRIVER_FAILURE,
                    2,
                    (ULONG)processInterrupt,
                    (ULONG)0x1
                );
            }
            if (Csr & LANCE_CSR0_MEMORY_ERROR)
            {
                //
                // A memory error shuts down the receiver and the
                // transmitter.  Reset to get the device started again.
                //
                Adapter->MemoryError++;

                SetupForReset(
                    Adapter,
                    NdisRequestGeneric4 // Means MAC issued
                 );
            }

            Csr &= ~LANCE_CSR0_ERROR_BITS;
        }

        //
        // Check the interrupt vector and see if there are any
        // more receives to process.  After we process any
        // other interrupt source we always come back to the top
        // of the loop to check if any more receive packets have
        // come in.  This is to lessen the probability that we
        // drop a receive.
        //
        if (Csr & LANCE_CSR0_RECEIVER_INTERRUPT)
        {
            if (ProcessReceiveInterrupts(Adapter))
            {
                Csr &= ~LANCE_CSR0_RECEIVER_INTERRUPT;
            }
        }

        //
        // Process the transmit interrupts if there are any.
        //
        if (Csr & LANCE_CSR0_TRANSMITTER_INTERRUPT)
        {
            //
            // We need to check if the transmitter has
            // stopped as a result of an error.  If it
            // has then we really need to reset the adapter.
            //
            if (!(Csr & LANCE_CSR0_TRANSMITTER_ON))
            {
                //
                // Might as well turn off the transmitter interrupt
                // source since we won't ever be processing them
                // and we don't want to come back here again.
                //
                Csr &= ~LANCE_CSR0_TRANSMITTER_INTERRUPT;

                //
                // Before we setup for the reset make sure that
                // we aren't already resetting.
                //
                if (!Adapter->ResetInProgress)
                {
                    //
                    // Sends use one descriptor per frame, so BUFF can't
                    // happen and the transmitter only stops like this on
                    // an underflow.  The reset discards that frame
                    // before its descriptor is processed, so count it here.
                    //
                    Adapter->UnderFlow++;

                    SetupForReset(
                        Adapter,
                        NdisRequestGeneric4 // means MAC issued
                    );
                }

                goto LoopBottom;
            }
            else
            {
                if (!ProcessTransmitInterrupts(Adapter))
                {
                    //
                    // Process interrupts returns false if it
                    // finds no more work to do.  If this so we
                    // turn off the transmitter interrupt source.
                    //
                    Csr &= ~LANCE_CSR0_TRANSMITTER_INTERRUPT;
                }
            }
        }

LoopBottom:;

    }

    //
    // Check if we indicated any packets.
    //
    // Note: The only way to get out of the loop (via the break above) is
    // while we're still holding the spin lock.
    //
    if (Adapter->IndicatedAPacket)
    {
        Adapter->IndicatedAPacket = FALSE;

        NdisMEthIndicateReceiveComplete(Adapter->MiniportAdapterHandle);
    }

    LOG(OUT_DPC);
}

#pragma NDIS_PAGEABLE_FUNCTION(AllocateAdapterMemory)

BOOLEAN
AllocateAdapterMemory(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    This routine allocates memory for:

    - Transmit ring entries

    - Receive ring entries

    - Receive buffers

    - Adapter buffers for use if user transmit buffers don't meet hardware
      contraints

    - Structures to map transmit ring entries back to the packets.

Arguments:

    Adapter - The adapter to allocate memory for.

Return Value:

    Returns FALSE if some memory needed for the adapter could not
    be allocated.

--*/

{

    //
    // Pointer to a transmit ring entry.  Used while initializing
    // the ring.
    //
    PLANCE_TRANSMIT_ENTRY CurrentTransmitEntry;

    //
    // Pointer to a receive ring entry.  Used while initializing
    // the ring.
    //
    PLANCE_RECEIVE_ENTRY CurrentReceiveEntry;

    //
    // Simple iteration variable.
    //
    UINT i;

    //
    // These variables exist to reduce the amount of checking below.
    //

    ULONG NumberOfSmallBuffers;
    ULONG NumberOfMediumBuffers;
    ULONG NumberOfLargeBuffers;

    PAGED_CODE();

    NumberOfSmallBuffers = Adapter->NumberOfSmallBuffers;
    NumberOfMediumBuffers = Adapter->NumberOfMediumBuffers;
    NumberOfLargeBuffers = Adapter->NumberOfLargeBuffers;



    //
    // Allocate memory for the initialization block.  Note that
    // this memory can not cross a page boundary.
    //

    LANCE_ALLOCATE_MEMORY_FOR_HARDWARE(
        Adapter,
        sizeof(LANCE_INITIALIZATION_BLOCK),
        &Adapter->InitBlock
        );

    if (Adapter->InitBlock == NULL) {
        DeleteAdapterMemory(Adapter);
        return FALSE;
    }

    //
    // Allocate the transmit ring descriptors.
    //

    LANCE_ALLOCATE_MEMORY_FOR_HARDWARE(
        Adapter,
        sizeof(LANCE_TRANSMIT_ENTRY)*Adapter->NumberOfTransmitRings,
        &Adapter->TransmitRing
        )

    if (Adapter->TransmitRing == NULL) {
        DeleteAdapterMemory(Adapter);
        return FALSE;
    }

    //
    // We have the transmit ring descriptors.  Make sure each is
    // in a clean state.
    //

    for (
        i = 0, CurrentTransmitEntry = Adapter->TransmitRing;
        i < Adapter->NumberOfTransmitRings;
        i++,CurrentTransmitEntry++
        ) {

        LANCE_ZERO_MEMORY_FOR_HARDWARE(
             (PUCHAR)CurrentTransmitEntry,
             sizeof(LANCE_TRANSMIT_ENTRY)
             );

    }


    //
    // Allocate all of the receive ring entries
    //

    LANCE_ALLOCATE_MEMORY_FOR_HARDWARE(
        Adapter,
        sizeof(LANCE_RECEIVE_ENTRY)*Adapter->NumberOfReceiveRings,
        &Adapter->ReceiveRing
        )

    if (Adapter->ReceiveRing == NULL) {
        DeleteAdapterMemory(Adapter);
        return FALSE;
    }

    //
    // We have the receive ring descriptors.  Allocate an
    // array to hold the virtual addresses of each receive
    // buffer.
    //

    LANCE_ALLOC_PHYS(
      &(Adapter->ReceiveVAs),
      sizeof(PVOID) * Adapter->NumberOfReceiveRings
      );

    if (Adapter->ReceiveVAs == NULL) {
        DeleteAdapterMemory(Adapter);
        return FALSE;
    }

    //
    // Clean the above memory
    //

    LANCE_ZERO_MEMORY(
        Adapter->ReceiveVAs,
        (sizeof(PVOID)*Adapter->NumberOfReceiveRings)
        );


    //
    // We have the receive ring descriptors.  Allocate a buffer
    // for each descriptor and make sure descriptor is in a clean state.
    //
    // While we're at it, relinquish ownership of the ring discriptors to
    // the lance.
    //

    for (
        i = 0, CurrentReceiveEntry = Adapter->ReceiveRing;
        i < Adapter->NumberOfReceiveRings;
        i++,CurrentReceiveEntry++
        ) {

        LANCE_ALLOCATE_MEMORY_FOR_HARDWARE(
            Adapter,
            Adapter->SizeOfReceiveBuffer,
            &Adapter->ReceiveVAs[i]
            );

        if (Adapter->ReceiveVAs[i] == NULL) {
            DeleteAdapterMemory(Adapter);
            return FALSE;
        }


        LANCE_SET_RECEIVE_BUFFER_ADDRESS(
            Adapter,
            CurrentReceiveEntry,
            Adapter->ReceiveVAs[i]
            );


        LANCE_SET_RECEIVE_BUFFER_LENGTH(
            CurrentReceiveEntry,
            Adapter->SizeOfReceiveBuffer
            );

        LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
             CurrentReceiveEntry->ReceiveSummaryBits,
             LANCE_RECEIVE_OWNED_BY_CHIP
             );

    }

    //
    // Allocate the ring to packet structure.
    //

    LANCE_ALLOC_PHYS(
        &(Adapter->RingToPacket),
        sizeof(LANCE_RING_TO_PACKET) * Adapter->NumberOfTransmitRings
        );

    if (Adapter->RingToPacket == NULL) {
        DeleteAdapterMemory(Adapter);
        return FALSE;
    }

    LANCE_ZERO_MEMORY(
        Adapter->RingToPacket,
        sizeof(LANCE_RING_TO_PACKET) * Adapter->NumberOfTransmitRings
        );

    //
    // Allocate the array of buffer descriptors.
    //

    LANCE_ALLOC_PHYS(
        &(Adapter->LanceBuffers),
        sizeof(LANCE_BUFFER_DESCRIPTOR) *
        (NumberOfSmallBuffers +
        NumberOfMediumBuffers +
        NumberOfLargeBuffers)
        );

    if (Adapter->LanceBuffers == NULL) {
        DeleteAdapterMemory(Adapter);
        return FALSE;
    }

    //
    // Zero the memory of all the descriptors so that we can
    // know which buffers wern't allocated incase we can't allocate
    // them all.
    //
    LANCE_ZERO_MEMORY(
        Adapter->LanceBuffers,
        sizeof(LANCE_BUFFER_DESCRIPTOR)*
        (NumberOfSmallBuffers +
        NumberOfMediumBuffers +
        NumberOfLargeBuffers)
        );

    //
    // Allocate each of the small lance buffers and fill in the
    // buffer descriptor.
    //

    Adapter->LanceBufferListHeads[0] = -1;
    Adapter->LanceBufferListHeads[1] = 0;

    for (
        i = 0;
        i < NumberOfSmallBuffers;
        i++
        ) {

        LANCE_ALLOCATE_MEMORY_FOR_HARDWARE(
            Adapter,
            LANCE_SMALL_BUFFER_SIZE,
            &Adapter->LanceBuffers[i].VirtualLanceBuffer
            );

        if (Adapter->LanceBuffers[i].VirtualLanceBuffer == NULL) {
            DeleteAdapterMemory(Adapter);
            return FALSE;
        }

        Adapter->LanceBuffers[i].Next = i+1;
        Adapter->LanceBuffers[i].BufferSize = LANCE_SMALL_BUFFER_SIZE;

    }

    //
    // Make sure that the last buffer correctly terminates the free list.
    //

    Adapter->LanceBuffers[i-1].Next = -1;

    //
    // Do the medium buffers now.
    //

    Adapter->LanceBufferListHeads[2] = i;

    for (
        ;
        i < NumberOfSmallBuffers + NumberOfMediumBuffers;
        i++
        ) {

        LANCE_ALLOCATE_MEMORY_FOR_HARDWARE(
            Adapter,
            LANCE_MEDIUM_BUFFER_SIZE,
            &Adapter->LanceBuffers[i].VirtualLanceBuffer
            );

        if (Adapter->LanceBuffers[i].VirtualLanceBuffer == NULL) {
            DeleteAdapterMemory(Adapter);
            return FALSE;
        }


        Adapter->LanceBuffers[i].Next = i+1;
        Adapter->LanceBuffers[i].BufferSize = LANCE_MEDIUM_BUFFER_SIZE;

    }

    //
    // Make sure that the last buffer correctly terminates the free list.
    //

    Adapter->LanceBuffers[i-1].Next = -1;


    Adapter->LanceBufferListHeads[3] = i;

    for (
        ;
        i < NumberOfSmallBuffers + NumberOfMediumBuffers + NumberOfLargeBuffers;
        i++
        ) {


        LANCE_ALLOCATE_MEMORY_FOR_HARDWARE(
            Adapter,
            LANCE_LARGE_BUFFER_SIZE,
            &Adapter->LanceBuffers[i].VirtualLanceBuffer
            );

        if (Adapter->LanceBuffers[i].VirtualLanceBuffer == NULL) {
            DeleteAdapterMemory(Adapter);
            return FALSE;
        }


        Adapter->LanceBuffers[i].Next = i+1;
        Adapter->LanceBuffers[i].BufferSize = LANCE_LARGE_BUFFER_SIZE;

    }

    //
    // Make sure that the last buffer correctly terminates the free list.
    //

    Adapter->LanceBuffers[i-1].Next = -1;

    return TRUE;

}


STATIC
VOID
DeleteAdapterMemory(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    This routine deallocates memory for:

    - Transmit ring entries

    - Receive ring entries

    - Receive buffers

    - Adapter buffers for use if user transmit buffers don't meet hardware
      contraints

    - Structures to map transmit ring entries back to the packets.

Arguments:

    Adapter - The adapter to deallocate memory for.

Return Value:

    None.

--*/

{

    //
    // These variables exist to reduce the amount of checking below.
    //

    ULONG NumberOfSmallBuffers;
    ULONG NumberOfMediumBuffers;
    ULONG NumberOfLargeBuffers;

    NumberOfSmallBuffers = Adapter->NumberOfSmallBuffers;
    NumberOfMediumBuffers = Adapter->NumberOfMediumBuffers;
    NumberOfLargeBuffers = Adapter->NumberOfLargeBuffers;


    if (Adapter->InitBlock) {

        LANCE_DEALLOCATE_MEMORY_FOR_HARDWARE(
            Adapter,
            Adapter->InitBlock
            );

    }

    if (Adapter->TransmitRing) {

        LANCE_DEALLOCATE_MEMORY_FOR_HARDWARE(
            Adapter,
            Adapter->TransmitRing
            );

    }

    if (Adapter->ReceiveRing) {

        LANCE_DEALLOCATE_MEMORY_FOR_HARDWARE(
            Adapter,
            Adapter->ReceiveRing
            );

    }

    if (Adapter->ReceiveVAs) {

        UINT i;

        for (
            i = 0;
            i < Adapter->NumberOfReceiveRings;
            i++
            ) {

            if (Adapter->ReceiveVAs[i]) {

                //
                // Part of the shared block, which the caller frees.
                //
                LANCE_DEALLOCATE_MEMORY_FOR_HARDWARE(
                    Adapter,
                    Adapter->ReceiveVAs[i]
                    );

            }

        }

        LANCE_FREE_PHYS(
          Adapter->ReceiveVAs,
          sizeof(PVOID) * Adapter->NumberOfReceiveRings
          );

    }

    if (Adapter->RingToPacket) {

        LANCE_FREE_PHYS(
            Adapter->RingToPacket,
            sizeof(LANCE_RING_TO_PACKET) * Adapter->NumberOfTransmitRings
            );

    }

    if (Adapter->LanceBuffers) {

        UINT i;

        for (
            i = 0;
            i < NumberOfSmallBuffers + NumberOfMediumBuffers + NumberOfLargeBuffers;
            i++) {

            if (Adapter->LanceBuffers[i].VirtualLanceBuffer) {

                //
                // Part of the shared block, which the caller frees.
                //
                LANCE_DEALLOCATE_MEMORY_FOR_HARDWARE(
                    Adapter,
                    Adapter->LanceBuffers[i].VirtualLanceBuffer
                    );

            }

        }

        LANCE_FREE_PHYS(
            Adapter->LanceBuffers,
            sizeof(LANCE_BUFFER_DESCRIPTOR) *
               (NumberOfSmallBuffers +
            NumberOfMediumBuffers +
            NumberOfLargeBuffers)
            );

    }

}


NDIS_STATUS
LanceQueryInformation(
    IN PNDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InfoBuffer,
    IN ULONG BytesLeft,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded
)
/*++

Routine Description:

    The LanceQuerylInformation process a Query request for
    NDIS_OIDs that are specific to a binding about the mini-port.

Arguments:

    Status - The status of the operation.

    MiniportAdapterContext - a pointer to the adapter.

    Oid - the NDIS_OID to process.

    InfoBuffer - a pointer into the NdisRequest->InformationBuffer
     into which store the result of the query.

    BytesLeft - the number of bytes left in the InformationBuffer.

    BytesNeeded - If there is not enough room in the information buffer
    then this will contain the number of bytes needed to complete the
    request.

    BytesWritten - a pointer to the number of bytes written into the
    InformationBuffer.

Return Value:

    None.

--*/

{
    PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)MiniportAdapterContext;
    NDIS_MEDIUM Medium = NdisMedium802_3;
    ULONG GenericULong;
    USHORT GenericUShort;
    UCHAR GenericArray[6];

    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;

    //
    // Common variables for pointing to result of query
    //

    PVOID MoveSource;
    ULONG MoveBytes;

    NDIS_HARDWARE_STATUS HardwareStatus = NdisHardwareStatusReady;

    //
    // Make sure that ulong is 4 bytes.  Else GenericULong must change
    // to something of size 4.
    //
    ASSERT(sizeof(ULONG) == 4);


#if LANCE_TRACE
    DbgPrint("In LanceQueryInfo\n");
#endif

    //
    // Set default values
    //

    MoveSource = (PVOID)(&GenericULong);
    MoveBytes = sizeof(GenericULong);

    *BytesWritten = 0;
    *BytesNeeded = 0;

    //
    // Switch on request type
    //

    switch (Oid) {

        case OID_GEN_MAC_OPTIONS:

            GenericULong = (ULONG)(NDIS_MAC_OPTION_TRANSFERS_NOT_PEND  |
                                   NDIS_MAC_OPTION_RECEIVE_SERIALIZED |
                                   NDIS_MAC_OPTION_NO_LOOPBACK
                                   );

            break;

        case OID_GEN_SUPPORTED_LIST:

            MoveSource = (PVOID)(LanceGlobalSupportedOids);
            MoveBytes = sizeof(LanceGlobalSupportedOids);
            break;

        case OID_GEN_HARDWARE_STATUS:


            if (Adapter->ResetInProgress){

                HardwareStatus = NdisHardwareStatusReset;

            } else {

                HardwareStatus = NdisHardwareStatusReady;

            }

            MoveSource = (PVOID)(&HardwareStatus);
            MoveBytes = sizeof(NDIS_HARDWARE_STATUS);

            break;

        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:

            MoveSource = (PVOID) (&Medium);
            MoveBytes = sizeof(NDIS_MEDIUM);
            break;

        case OID_GEN_MAXIMUM_LOOKAHEAD:

            GenericULong = LANCE_INDICATED_LOOKAHEAD(Adapter);

            break;


        case OID_GEN_MAXIMUM_FRAME_SIZE:

            GenericULong = (ULONG)(LANCE_LARGE_BUFFER_SIZE - LANCE_HEADER_SIZE);

            break;


        case OID_GEN_MAXIMUM_TOTAL_SIZE:

            GenericULong = (ULONG)(LANCE_LARGE_BUFFER_SIZE);

            break;


        case OID_GEN_LINK_SPEED:

            switch (Adapter->ChipPart)
            {
                case PCNET_PART_79C971:
                case PCNET_PART_79C972:
                case PCNET_PART_79C973:
                case PCNET_PART_79C975:
                    GenericULong = (ULONG)1000000;
                    break;

                default:
                    GenericULong = (ULONG)100000;
                    break;
            }

            break;


        case OID_GEN_TRANSMIT_BUFFER_SPACE:

            GenericULong = (ULONG)((LANCE_SMALL_BUFFER_SIZE * Adapter->NumberOfSmallBuffers) +
                                   (LANCE_MEDIUM_BUFFER_SIZE * Adapter->NumberOfMediumBuffers) +
                                   (LANCE_LARGE_BUFFER_SIZE * Adapter->NumberOfLargeBuffers));


            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:

            GenericULong = (ULONG)(Adapter->NumberOfReceiveRings *
                                   Adapter->SizeOfReceiveBuffer);

            break;

        case OID_GEN_TRANSMIT_BLOCK_SIZE:

            GenericULong = (ULONG)(LANCE_SMALL_BUFFER_SIZE);

            break;

        case OID_GEN_RECEIVE_BLOCK_SIZE:

            GenericULong = (ULONG)(Adapter->SizeOfReceiveBuffer);

            break;

        case OID_GEN_VENDOR_ID:

            //
            // The 3-byte IEEE OUI (the first three bytes of the burned-in
            // address), followed by one byte that identifies the NIC.
            // This used to copy the OUI into a ULONG, then overwrite its
            // first byte and leave the fourth uninitialized.
            //
            LANCE_MOVE_MEMORY(GenericArray, Adapter->NetworkAddress, 3);
            GenericArray[3] = 0x04;

            MoveSource = (PVOID)(GenericArray);
            MoveBytes = 4;
            break;

        case OID_GEN_VENDOR_DESCRIPTION:

            MoveSource = (PVOID)"Lance Adapter.";
            MoveBytes = 15;

            break;

        case OID_GEN_DRIVER_VERSION:

            GenericUShort = (LANCE_NDIS_MAJOR_VERSION << 8) | LANCE_NDIS_MINOR_VERSION;

            MoveSource = (PVOID)(&GenericUShort);
            MoveBytes = sizeof(GenericUShort);
            break;

        case OID_GEN_MEDIA_CONNECT_STATUS:

            GenericULong = (ULONG)(Adapter->MediaConnected ?
                                   NdisMediaStateConnected :
                                   NdisMediaStateDisconnected);

            break;

        case OID_GEN_CURRENT_LOOKAHEAD:

            //
            // Lookahead excludes the 14-byte header.  Every indication
            // carries the full LANCE_INDICATED_LOOKAHEAD, which NDIS
            // allows even if a protocol set a smaller value.
            //
            GenericULong = LANCE_INDICATED_LOOKAHEAD(Adapter);

            break;

        case OID_802_3_PERMANENT_ADDRESS:

            LANCE_MOVE_MEMORY((PCHAR)GenericArray,
                              Adapter->NetworkAddress,
                              LANCE_LENGTH_OF_ADDRESS
                             );

            MoveSource = (PVOID)(GenericArray);
            MoveBytes = sizeof(Adapter->NetworkAddress);
            break;

        case OID_802_3_CURRENT_ADDRESS:


            LANCE_MOVE_MEMORY((PCHAR)GenericArray,
                              Adapter->CurrentNetworkAddress,
                              LANCE_LENGTH_OF_ADDRESS
                             );

            MoveSource = (PVOID)(GenericArray);
            MoveBytes = sizeof(Adapter->CurrentNetworkAddress);
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:

            GenericULong = Adapter->MaxMulticastList;

            break;

        case OID_GEN_XMIT_OK:

            //
            // Late-collision frames take the success path and are
            // already counted in Transmit.
            //
            GenericULong = (ULONG)(Adapter->Transmit);

            break;

        case OID_GEN_RCV_OK:

            GenericULong = (ULONG)(Adapter->Receive);

            break;

        case OID_GEN_XMIT_ERROR:

            GenericULong = (ULONG)(Adapter->RetryFailure +
                                   Adapter->LostCarrier +
                                   Adapter->UnderFlow);

            break;

        case OID_GEN_RCV_ERROR:

            GenericULong = (ULONG)(Adapter->CRCError +
                                   Adapter->FramingError +
                                   Adapter->LengthError);

            break;

        case OID_GEN_RCV_NO_BUFFER:

            GenericULong = (ULONG)(Adapter->OutOfReceiveBuffers +
                                   Adapter->MissedPacket);

            break;

        case OID_802_3_RCV_ERROR_ALIGNMENT:

            GenericULong = (ULONG)(Adapter->FramingError);

            break;

        case OID_802_3_XMIT_ONE_COLLISION:

            GenericULong = (ULONG)(Adapter->OneRetry);

            break;

        case OID_802_3_XMIT_MORE_COLLISIONS:

            GenericULong = (ULONG)(Adapter->MoreThanOneRetry);

            break;

        default:

            StatusToReturn = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    if (StatusToReturn == NDIS_STATUS_SUCCESS){

        if (MoveBytes > BytesLeft){

            //
            // Not enough room in InformationBuffer. Punt
            //

            *BytesNeeded = MoveBytes;

            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;

        } else {

            //
            // Store result.
            //

            LANCE_MOVE_MEMORY(InfoBuffer, MoveSource, MoveBytes);

            *BytesWritten = MoveBytes;

        }
    }

#if LANCE_TRACE
    DbgPrint("Out LanceQueryInfo\n");
#endif

    LANCE_DO_DEFERRED(Adapter);

    return StatusToReturn;
}

NDIS_STATUS
LanceSetInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded
    )
/*++

Routine Description:

    The LanceSetInformation is used by LanceRequest to set information
    about the MAC.

    Note: Assumes it is called with the lock held.

Arguments:

    Adapter - A pointer to the adapter.

    Open - A pointer to an open instance.

    NdisRequest - A structure which contains the request type (Set),
    an array of operations to perform, and an array for holding
    the results of the operations.

Return Value:

    The function value is the status of the operation.

--*/

{

    PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)(MiniportAdapterContext);
    //
    // General Algorithm:
    //
    //  For each request
    //     Verify length
    //     Switch(Request)
    //        Process Request
    //

    UINT BytesLeft = InformationBufferLength;
    PUCHAR InfoBuffer = (PUCHAR)InformationBuffer;

    //
    // Variables for a particular request
    //

    UINT OidLength;

    //
    // Variables for holding the new values to be used.
    //

    ULONG LookAhead;
    ULONG Filter;

    NDIS_STATUS Status;

#if LANCE_TRACE
    DbgPrint("In LanceSetInfo\n");
#endif

    //
    // Get Oid and Length of next request
    //

    OidLength = BytesLeft;

    Status = NDIS_STATUS_SUCCESS;

    switch (Oid) {

        case OID_802_3_MULTICAST_LIST:

            //
            // Verify length
            //

            if ((OidLength % LANCE_LENGTH_OF_ADDRESS) != 0){

                *BytesRead = 0;
                *BytesNeeded = 0;

                return( NDIS_STATUS_INVALID_LENGTH );

            }

            //
            // The list is copied into a fixed table; refuse more entries
            // than we advertise in OID_802_3_MAXIMUM_LIST_SIZE.
            //

            if ((OidLength / LANCE_LENGTH_OF_ADDRESS) > Adapter->MaxMulticastList) {

                *BytesRead = 0;
                *BytesNeeded = 0;

                return( NDIS_STATUS_MULTICAST_FULL );

            }

            Status = LanceChangeMulticastAddresses(
                         Adapter,
                         OidLength / LANCE_LENGTH_OF_ADDRESS,
                         (CHAR (*)[LANCE_LENGTH_OF_ADDRESS])InfoBuffer,
                         NdisRequestSetInformation
                         );

            break;


        case OID_GEN_CURRENT_PACKET_FILTER:

            //
            // Verify length
            //

            if (OidLength != 4) {

                Status = NDIS_STATUS_INVALID_LENGTH;

                *BytesRead = 0;
                *BytesNeeded = 4;

                break;

            }


            LANCE_MOVE_MEMORY(&Filter, InfoBuffer, 4);

            //
            // Verify bits
            //
            if (Filter & (NDIS_PACKET_TYPE_SOURCE_ROUTING |
                          NDIS_PACKET_TYPE_SMT |
                          NDIS_PACKET_TYPE_MAC_FRAME |
                          NDIS_PACKET_TYPE_FUNCTIONAL |
                          NDIS_PACKET_TYPE_ALL_FUNCTIONAL |
                          NDIS_PACKET_TYPE_GROUP
                         )) {

                Status = NDIS_STATUS_NOT_SUPPORTED;

                *BytesRead = 4;
                *BytesNeeded = 0;

                break;

            }

            Status = LanceSetPacketFilter(
                         Adapter,
                         NdisRequestSetInformation,
                         Filter
                     );

            break;

        case OID_GEN_CURRENT_LOOKAHEAD:

            //
            // Verify length
            //
            if (OidLength != 4)
            {
                Status = NDIS_STATUS_INVALID_LENGTH;

                *BytesRead = 0;
                *BytesNeeded = 4;

                break;
            }

            LANCE_MOVE_MEMORY(&LookAhead, InfoBuffer, 4);

            //
            // Accept anything up to what we report as the maximum.  The
            // value is only recorded: indications always carry
            // LANCE_INDICATED_LOOKAHEAD bytes.
            //
            if (LookAhead <= LANCE_INDICATED_LOOKAHEAD(Adapter))
            {
                Adapter->MaxLookAhead = LookAhead;
                Status = NDIS_STATUS_SUCCESS;
            }
            else
            {
                Status = NDIS_STATUS_INVALID_LENGTH;
            }

            *BytesRead = 4;
            *BytesNeeded = 0;
            break;

        default:

            Status = NDIS_STATUS_INVALID_OID;

            *BytesRead = 0;
            *BytesNeeded = 0;

            break;
    }

    //
    // A filter or multicast set that pends has already read its buffer;
    // NDIS reports BytesRead when the request completes.
    //
    if ((Status == NDIS_STATUS_SUCCESS) || (Status == NDIS_STATUS_PENDING)){

        *BytesRead = OidLength;
        *BytesNeeded = 0;

    }

#if LANCE_TRACE
    DbgPrint("Out LanceSetInfo\n");
#endif

    LANCE_DO_DEFERRED(Adapter);

    return Status;
}

STATIC
NDIS_STATUS
LanceSetPacketFilter(
    IN PLANCE_ADAPTER Adapter,
    IN NDIS_REQUEST_TYPE NdisRequestType,
    IN UINT PacketFilter
    )

/*++

Routine Description:

    The LanceSetPacketFilter request allows a protocol to control the types
    of packets that it receives from the MAC.

    Note : Assumes that the lock is currently held.

Arguments:

    Adapter - Pointer to the LANCE_ADAPTER.

    NdisRequestType - Code to indicate from where this routine is being called.

    PacketFilter - A bit mask that contains flags that correspond to specific
    classes of received packets.  If a particular bit is set in the mask,
    then packet reception for that class of packet is enabled.  If the
    bit is clear, then packets that fall into that class are not received
    by the client.  A single exception to this rule is that if the promiscuous
    bit is set, then the client receives all packets on the network, regardless
    of the state of the other flags.

Return Value:

    The function value is the status of the operation.

--*/

{

    //
    // Keeps track of the *MAC's* status.  The status will only be
    // reset if the filter change action routine is called.
    //
    NDIS_STATUS StatusOfFilterChange = NDIS_STATUS_SUCCESS;


#if LANCE_TRACE
    DbgPrint("In LanceSetPacketFilter\n");
#endif

    //
    // Check to see if the device is already resetting.
    //

    if (Adapter->HardwareFailure ||
        (Adapter->ResetInProgress &&
         (NdisRequestType != NdisRequestSetInformation)))
    {
        return(NDIS_STATUS_FAILURE);
    }

    //
    // We need to add this to the hardware multicast filtering.
    //
    Adapter->PacketFilter = PacketFilter;

    if (Adapter->ResetInProgress)
    {
        LanceQueueSetBehindReset(Adapter);
    }
    else
    {
        SetupForReset(Adapter, NdisRequestType);
    }

#if LANCE_TRACE
    DbgPrint("Out LanceSetPacketFilter\n");
#endif

    return(NDIS_STATUS_PENDING);
}

STATIC
UINT
CalculateCRC(
    IN UINT NumberOfBytes,
    IN PCHAR Input
    )

/*++

Routine Description:

    Calculates a 32 bit crc value over the input number of bytes.

Arguments:

    NumberOfBytes - The number of bytes in the input.

    Input - An input "string" to calculate a CRC over.

Return Value:

    A 32 bit crc value.


--*/

{

    const UINT POLY = 0x04c11db6;
    UINT CRCValue = 0xffffffff;

    ASSERT(sizeof(UINT) == 4);

    for ( ; NumberOfBytes; NumberOfBytes-- ) {

        UINT CurrentBit;
        UCHAR CurrentByte = *Input;
        Input++;

        for ( CurrentBit = 8; CurrentBit; CurrentBit-- ) {

            UINT CurrentCRCHigh = CRCValue >> 31;

            CRCValue = (CRCValue & 0x7fffffff) << 1;

            if (CurrentCRCHigh ^ (CurrentByte & 0x01)) {

                CRCValue ^= POLY;
                CRCValue |= 0x00000001;

            }

            CurrentByte >>= 1;

        }

    }

    return CRCValue;

}

STATIC
NDIS_STATUS
LanceChangeMulticastAddresses(
    IN PLANCE_ADAPTER Adapter,
    IN UINT NewAddressCount,
    IN CHAR NewAddresses[][LANCE_LENGTH_OF_ADDRESS],
    IN NDIS_REQUEST_TYPE NdisRequestType
    )

/*++

Routine Description:

    Action routine that will get called when a particular filter
    class is first used or last cleared.

    NOTE: This routine assumes that it is called with the lock
    acquired.

Arguments:

    Adapter - The adapter.

    NewAddressCount - Number of Addresses that should be put on the adapter.

    NewAddresses - An array of all the multicast addresses that should
    now be used.

    NdisRequestType - The request type.

Return Value:

    Status of the operation.


--*/

{
#if LANCE_TRACE
    DbgPrint("In LanceChangeMultiAdresses\n");
#endif

    //
    // Check to see if the device is already resetting.  If it is
    // then pend this add behind that reset.
    //
    if (Adapter->HardwareFailure ||
        (Adapter->ResetInProgress &&
         (NdisRequestType != NdisRequestSetInformation)))
    {

        return(NDIS_STATUS_FAILURE);
    }

    //
    // We need to add this to the hardware multicast filtering.
    //
    Adapter->NumberOfAddresses = NewAddressCount;

    NdisMoveMemory(
        Adapter->MulticastAddresses,
        NewAddresses,
        NewAddressCount * LANCE_LENGTH_OF_ADDRESS
    );

    if (Adapter->ResetInProgress)
    {
        LanceQueueSetBehindReset(Adapter);
    }
    else
    {
        SetupForReset(Adapter, NdisRequestType);
    }


#if LANCE_TRACE
    DbgPrint("Out LanceChangeMultiAdresses\n");
#endif

    return(NDIS_STATUS_PENDING);

}

NDIS_STATUS
LanceReset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext
    )

/*++

Routine Description:

    The LanceReset request instructs the mini-port to issue a hardware reset
    to the network adapter.  The Miniport also resets its software state.  See
    the description of MiniportReset for a detailed description of this request.

Arguments:

    Status - Status of the operation.

    AddressingReset - Not used.

    MiniportAdapterContext - The context value set by this mini-port.

Return Value:

    The function value is the status of the operation.


--*/

{
    //
    // Holds the status that should be returned to the caller.
    //
    PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)MiniportAdapterContext;

    SetupForReset(
        Adapter,
        NdisRequestGeneric1 // Means Reset
    );

    LANCE_DO_DEFERRED(Adapter);

    return(NDIS_STATUS_PENDING);
}

STATIC
VOID
LanceSetInitializationBlock(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    This routine simply fills the initialization block
    with the information necessary for initialization.

Arguments:

    Adapter - The adapter which holds the initialization block
    to initialize.

Return Value:

    None.


--*/

{

    ULONG PhysAdr;

    UINT PacketFilters;

    PLANCE_RECEIVE_ENTRY CurrentEntry = Adapter->ReceiveRing;
    USHORT Mode;
    UCHAR RingNumber;
    UCHAR i;

#if LANCE_TRACE
    DbgPrint("in SetInitBlock\n");
#endif

    LANCE_ZERO_MEMORY_FOR_HARDWARE(
        (PUCHAR)Adapter->InitBlock,
        sizeof(LANCE_INITIALIZATION_BLOCK)
    );

    //
    //  Set the card address.
    //
    for (i = 0; i < LANCE_LENGTH_OF_ADDRESS; i++)
    {
        LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
            Adapter->InitBlock->PhysicalAddress[i],
            Adapter->CurrentNetworkAddress[i]
        );
    }

    //
    //  Setup the transmit ring.
    //
    PhysAdr = LANCE_GET_HARDWARE_PHYSICAL_ADDRESS(
                  Adapter,
                  Adapter->TransmitRing
              );

    LANCE_WRITE_HARDWARE_LOW_PART_ADDRESS(
         Adapter->InitBlock->LowTransmitRingAddress,
         LANCE_GET_LOW_PART_ADDRESS(PhysAdr)
     );

    LANCE_WRITE_HARDWARE_HIGH_PART_ADDRESS(
         Adapter->InitBlock->HighTransmitRingAddress,
         LANCE_GET_HIGH_PART_ADDRESS(PhysAdr)
     );

    //
    //  Setup the receive ring.
    //
    PhysAdr = LANCE_GET_HARDWARE_PHYSICAL_ADDRESS(
                  Adapter,
                  Adapter->ReceiveRing
              );

    //
    // Set that the chip owns each entry in the ring
    //
    for (CurrentEntry = Adapter->ReceiveRing, RingNumber = 0;
         RingNumber < Adapter->NumberOfReceiveRings ;
         RingNumber++, CurrentEntry++
    )
    {
        LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
            CurrentEntry->ReceiveSummaryBits,
            LANCE_RECEIVE_OWNED_BY_CHIP
        );
    }

    LANCE_WRITE_HARDWARE_LOW_PART_ADDRESS(
        Adapter->InitBlock->LowReceiveRingAddress,
        LANCE_GET_LOW_PART_ADDRESS(PhysAdr)
    );

    LANCE_WRITE_HARDWARE_HIGH_PART_ADDRESS(
        Adapter->InitBlock->HighReceiveRingAddress,
        LANCE_GET_HIGH_PART_ADDRESS(PhysAdr)
    );

    LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
        Adapter->InitBlock->TransmitLengthLow5BitsReserved,
        (UCHAR)(Adapter->LogNumberTransmitRings << 5)
    );

    LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
        Adapter->InitBlock->ReceiveLengthLow5BitsReserved,
        (UCHAR)(Adapter->LogNumberReceiveRings << 5)
    );

    //
    // Set up the address filtering.
    //
    // First get hold of the combined packet filter.
    //
    PacketFilters = Adapter->PacketFilter;

#if LANCE_TRACE
    DbgPrint("Filters 0x%x\n", PacketFilters);
#endif

    if (PacketFilters & NDIS_PACKET_TYPE_PROMISCUOUS)
    {
        //
        // If one binding is promiscuous there is no point in
        // setting up any other filtering.  Every packet is
        // going to be accepted by the hardware.
        //
        LANCE_READ_HARDWARE_MEMORY_USHORT(
            Adapter->InitBlock->ModeRegister,
            &Mode
        );

        LANCE_WRITE_HARDWARE_MEMORY_USHORT(
            Adapter->InitBlock->ModeRegister,
            Mode | LANCE_MODE_PROMISCUOUS
        );
    }
    else
    {
        //
        // Turn off promiscuous bit
        //
        LANCE_READ_HARDWARE_MEMORY_USHORT(
            Adapter->InitBlock->ModeRegister,
            &Mode
        );

        LANCE_WRITE_HARDWARE_MEMORY_USHORT(
            Adapter->InitBlock->ModeRegister,
            Mode & (~LANCE_MODE_PROMISCUOUS)
        );

        if (PacketFilters & NDIS_PACKET_TYPE_ALL_MULTICAST)
        {
            //
            // We turn on all the bits in the filter since one binding
            // wants every multicast address.
            //
            LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
                Adapter->InitBlock->LogicalAddressFilter[0],
                0xff
            );
            LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
                Adapter->InitBlock->LogicalAddressFilter[1],
                0xff
            );
            LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
                Adapter->InitBlock->LogicalAddressFilter[2],
                0xff
            );
            LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
                Adapter->InitBlock->LogicalAddressFilter[3],
                0xff
            );
            LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
                Adapter->InitBlock->LogicalAddressFilter[4],
                0xff
            );
            LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
                Adapter->InitBlock->LogicalAddressFilter[5],
                0xff
            );
            LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
                Adapter->InitBlock->LogicalAddressFilter[6],
                0xff
            );
            LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
                Adapter->InitBlock->LogicalAddressFilter[7],
                0xff
            );
        }
        else if (PacketFilters & NDIS_PACKET_TYPE_MULTICAST)
        {
           //
           // At least one open binding wants multicast addresses.
           //
           // We get the multicast addresses from the filter and
           // put each one through a CRC.  We then take the high
           // order 6 bits from the 32 bit CRC and set that bit
           // in the logical address filter.
           //
           UINT NumberOfAddresses;

           NumberOfAddresses = Adapter->NumberOfAddresses;

           ASSERT(sizeof(ULONG) == 4);

           for ( ; NumberOfAddresses; NumberOfAddresses--)
           {
               UINT CRCValue;

               UINT HashValue = 0;

               CRCValue = CalculateCRC(
                              6,
                              Adapter->MulticastAddresses[NumberOfAddresses-1]
                          );

               HashValue |= ((CRCValue & 0x00000001)?(0x00000020):(0x00000000));
               HashValue |= ((CRCValue & 0x00000002)?(0x00000010):(0x00000000));
               HashValue |= ((CRCValue & 0x00000004)?(0x00000008):(0x00000000));
               HashValue |= ((CRCValue & 0x00000008)?(0x00000004):(0x00000000));
               HashValue |= ((CRCValue & 0x00000010)?(0x00000002):(0x00000000));
               HashValue |= ((CRCValue & 0x00000020)?(0x00000001):(0x00000000));

               LANCE_READ_HARDWARE_MEMORY_UCHAR(
                   Adapter->InitBlock->LogicalAddressFilter[HashValue >> 3],
                   &RingNumber
               );

               LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
                   Adapter->InitBlock->LogicalAddressFilter[HashValue >> 3],
                   RingNumber | (1 << (HashValue & 0x00000007))
               );
            }
        }
    }

#if LANCE_TRACE
    DbgPrint("out SetInitBlock\n");
#endif
}


STATIC
BOOLEAN
ProcessReceiveInterrupts(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    Process the packets that have finished receiving.

    NOTE: This routine assumes that no other thread of execution
    is processing receives!  THE LOCK MUST BE HELD

Arguments:

    Adapter - The adapter to indicate to.

Return Value:

    Whether to clear the interrupt or not.

--*/

{
    //
    // We don't get here unless there was a receive.  Loop through
    // the receive descriptors starting at the last known descriptor
    // owned by the hardware that begins a packet.
    //
    // Examine each receive ring descriptor for errors.
    //
    // We keep an array whose elements are indexed by the ring
    // index of the receive descriptors.  The arrays elements are
    // the virtual addresses of the buffers pointed to by
    // each ring descriptor.
    //
    // When we have the entire packet (and error processing doesn't
    // prevent us from indicating it), we give the routine that
    // processes the packet through the filter, the buffers virtual
    // address (which is always the lookahead size) and as the
    // MAC context the index to the first and last ring descriptors
    // comprising the packet.
    //

    //
    // Index of the ring descriptor in the ring.
    //
    UINT CurrentIndex = Adapter->CurrentReceiveIndex;

    //
    // Pointer to the ring descriptor being examined.
    //
    PLANCE_RECEIVE_ENTRY CurrentEntry = Adapter->ReceiveRing + CurrentIndex;

    //
    // Hold in a local the top receive ring index so that we don't
    // need to get it from the adapter all the time.
    //
    const UINT TopReceiveIndex = Adapter->NumberOfReceiveRings - 1;
    const UINT ReceivePacketLimit = Adapter->NumberOfReceiveRings / 2;

    //
    // Boolean to record the fact that we've finished processing
    // one packet and we're about to start a new one.
    //
    BOOLEAN NewPacket = FALSE;

    //
    // Count of the number of buffers in the current packet.
    //
    UINT NumberOfBuffers = 1;

    //
    // Pointer to host addressable space for the lookahead buffer
    //
    PUCHAR LookaheadBuffer;

    ULONG ReceivePacketCount = 0;

    for (; ; )
    {
        UCHAR ReceiveSummaryBits;

        //
        // Check to see whether we own the packet.  If
        // we don't then simply return to the caller.
        //

        LANCE_READ_HARDWARE_MEMORY_UCHAR(
            CurrentEntry->ReceiveSummaryBits,
            &ReceiveSummaryBits
        );

        if (ReceiveSummaryBits & LANCE_RECEIVE_OWNED_BY_CHIP)
        {
            LOG(RECEIVE);

            return(TRUE);
        }
        else if (ReceivePacketCount >= ReceivePacketLimit)
        {
            //
            // Amortize interrupt synchronization over a larger receive
            // batch while keeping each pass bounded by half the ring.
            //
            LOG(RECEIVE)

            return(FALSE);
        }
        else if (ReceiveSummaryBits & LANCE_RECEIVE_ERROR_SUMMARY)
        {
            //
            // We have an error in the packet.  Record
            // the details of the error.
            //

            //
            // Synch with the set/query information routines.
            //
            if (ReceiveSummaryBits & LANCE_RECEIVE_BUFFER_ERROR)
            {
                //
                // Probably ran out of descriptors.
                //

                Adapter->OutOfReceiveBuffers++;
            }
            else if (ReceiveSummaryBits & LANCE_RECEIVE_FRAMING_ERROR)
            {
                //
                // The chip sets FRAM only together with CRC, so test it
                // first or alignment errors are all counted as CRC errors.
                //
                Adapter->FramingError++;
            }
            else if (ReceiveSummaryBits & LANCE_RECEIVE_CRC_ERROR)
            {
                Adapter->CRCError++;
            }
            else if (ReceiveSummaryBits & LANCE_RECEIVE_OVERFLOW_ERROR)
            {
                Adapter->OutOfReceiveBuffers++;
            }

            ReceivePacketCount++;

            //
            // Give the packet back to the hardware.
            //

            RelinquishReceivePacket(
                Adapter,
                Adapter->CurrentReceiveIndex,
                NumberOfBuffers
            );

            NewPacket = TRUE;
        }
        else if (ReceiveSummaryBits & LANCE_RECEIVE_END_OF_PACKET)
        {
            //
            // We've reached the end of the packet.  Prepare
            // the parameters for indication, then indicate.
            //

            UINT PacketSize;
            UINT LookAheadSize;

            LANCE_RECEIVE_CONTEXT Context;

            ASSERT(sizeof(LANCE_RECEIVE_CONTEXT) == sizeof(NDIS_HANDLE));

            //
            // Check just before we do indications that we aren't
            // resetting.
            //
            if (Adapter->ResetInProgress)
            {
                return(TRUE);
            }

            Context.INFO.IsContext = TRUE;
            Context.INFO.FirstBuffer = Adapter->CurrentReceiveIndex;
            Context.INFO.LastBuffer = CurrentIndex;

            LANCE_GET_MESSAGE_SIZE(CurrentEntry, PacketSize);

            //
            // PacketSize is MCNT minus the 4-byte FCS.  An MCNT below 4
            // makes it wrap to a huge UINT, a frame shorter than the
            // Ethernet header would make LookAheadSize wrap below, and
            // anything over 1514 bytes is not a frame we can describe to
            // NDIS (and TransferData would read past the packet).  None
            // of these come from a healthy chip; drop them rather than
            // indicate garbage lengths.
            //
            if ((PacketSize < LANCE_HEADER_SIZE) ||
                (PacketSize > LANCE_LARGE_BUFFER_SIZE))
            {
                Adapter->LengthError++;

                LookaheadBuffer = NULL;
            }
            else
            {
                LookAheadSize = PacketSize;

                //
                // Find amount to indicate.
                //

                LookAheadSize = ((LookAheadSize < Adapter->SizeOfReceiveBuffer) ?
                                 LookAheadSize :
                                 Adapter->SizeOfReceiveBuffer);

                LookAheadSize -= LANCE_HEADER_SIZE;

                //
                // Increment the number of packets succesfully received.
                //

                Adapter->Receive++;

                LOG(INDICATE);

                Adapter->IndicatingMacReceiveContext = Context;

                Adapter->IndicatedAPacket = TRUE;

                NdisCreateLookaheadBufferFromSharedMemory(
                    (PVOID)(Adapter->ReceiveVAs[Adapter->CurrentReceiveIndex]),
                    LookAheadSize + LANCE_HEADER_SIZE,
                    &LookaheadBuffer
                );
            }

            if (LookaheadBuffer != NULL)
            {
                NdisMEthIndicateReceive(
                    Adapter->MiniportAdapterHandle,
                    (NDIS_HANDLE)Context.WholeThing,
                    LookaheadBuffer,
                    LANCE_HEADER_SIZE,
                    LookaheadBuffer + LANCE_HEADER_SIZE,
                    LookAheadSize,
                    PacketSize - LANCE_HEADER_SIZE
                );

                NdisDestroyLookaheadBufferFromSharedMemory(LookaheadBuffer);
            }

            ReceivePacketCount++;

            //
            // Give the packet back to the hardware.
            //

            RelinquishReceivePacket(
                Adapter,
                Adapter->CurrentReceiveIndex,
                NumberOfBuffers
            );

            NewPacket = TRUE;
        }

        //
        // We're at some indermediate packet. Advance to
        // the next one.
        //
        if (CurrentIndex == TopReceiveIndex)
        {
            CurrentIndex = 0;
            CurrentEntry = Adapter->ReceiveRing;
        }
        else
        {
            CurrentIndex++;
            CurrentEntry++;
        }

        if (NewPacket)
        {
            Adapter->CurrentReceiveIndex = CurrentIndex;
            NewPacket = FALSE;
            NumberOfBuffers = 0;
        }

        NumberOfBuffers++;

        if (NumberOfBuffers > (TopReceiveIndex + 1))
        {
            //
            // Error!  We went all the way round the receive ring
            // without seeing the end of a packet, so the ring is
            // inconsistent.  Stop the chip and reinitialize it; the
            // reset path rebuilds both rings from scratch.
            //
            // (This used to call NdisMDeregisterInterrupt, which must
            // not be called at DISPATCH_LEVEL from our own DPC and
            // which LanceHalt would then have called a second time.)
            //
            NdisWriteErrorLogEntry(
                Adapter->MiniportAdapterHandle,
                NDIS_ERROR_CODE_HARDWARE_FAILURE,
                1,
                (ULONG)0x2
            );

            if (!Adapter->ResetInProgress)
            {
                SetupForReset(
                    Adapter,
                    NdisRequestGeneric4 // Means MAC issued
                );
            }

            return(TRUE);
        }
    }
}

STATIC
VOID
RelinquishReceivePacket(
    IN PLANCE_ADAPTER Adapter,
    IN UINT StartingIndex,
    IN UINT NumberOfBuffers
    )

/*++

Routine Description:

    Gives a range of receive descriptors back to the hardware.

Arguments:

    Adapter - The adapter that the ring works with.

    StartingIndex - The first ring to return.  Note that since
    we are dealing with a ring, this value could be greater than
    the EndingIndex.

    NumberOfBuffers - The number of buffers (or ring descriptors) in
    the current packet.

Return Value:

    None.

--*/

{

    //
    // Index of the ring descriptor in the ring.
    //
    UINT CurrentIndex = StartingIndex;

    //
    // Pointer to the ring descriptor being returned.
    //
    PLANCE_RECEIVE_ENTRY CurrentEntry = Adapter->ReceiveRing + CurrentIndex;

    //
    // Hold in a local so that we don't need to access via the adapter.
    //
    const UINT TopReceiveIndex = Adapter->NumberOfReceiveRings - 1;

    UCHAR Tmp;

    LANCE_READ_HARDWARE_MEMORY_UCHAR(CurrentEntry->ReceiveSummaryBits, &Tmp);

    ASSERT(!(Tmp & LANCE_RECEIVE_OWNED_BY_CHIP));
    ASSERT(Tmp & LANCE_RECEIVE_START_OF_PACKET);

    for ( ; NumberOfBuffers; NumberOfBuffers-- )
    {
        LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
            CurrentEntry->ReceiveSummaryBits,
            LANCE_RECEIVE_OWNED_BY_CHIP
        );

        if (CurrentIndex == TopReceiveIndex)
        {
            CurrentEntry = Adapter->ReceiveRing;
            CurrentIndex = 0;
        }
        else
        {
            CurrentEntry++;
            CurrentIndex++;
        }
    }
}

STATIC
BOOLEAN
ProcessTransmitInterrupts(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    Process the packets that have finished transmitting.

    NOTE: This routine assumes that it is being executed in a
    single thread of execution.  CALLED WITH LOCK HELD!!!

Arguments:

    Adapter - The adapter that was sent from.

Return Value:

    This function will return TRUE if it finished up the
    send on a packet.  It will return FALSE if for some
    reason there was no packet to process.

--*/

{
    //
    // Index into the ring to packet structure.  This index points
    // to the first ring entry for the first buffer used for transmitting
    // the packet.
    //
    UINT FirstIndex;

    //
    // Pointer to the last ring entry for the packet to be transmitted.
    // This pointer might actually point to a ring entry before the first
    // ring entry for the packet since the ring structure is, simply, a ring.
    //
    PLANCE_TRANSMIT_ENTRY LastRingEntry;

    //
    // Pointer to the packet that started this transmission.
    //
    PNDIS_PACKET OwningPacket;

    UCHAR TransmitSummaryBits;
    USHORT ErrorSummaryInfo;

    //
    // Used to hold the ring to packet mapping information so that
    // we can release the ring entries as quickly as possible.
    //
    LANCE_RING_TO_PACKET SavedRingMapping;


    //
    // Get hold of the first transmitted packet.
    //

    //
    // First we check that this is a packet that was transmitted
    // but not already processed.  Recall that this routine
    // will be called repeatedly until this tests false, Or we
    // hit a packet that we don't completely own.
    //

    //
    // NOTE: I found a problem where FirstUncommitedRing wraps around
    // and becomes equal to TransmittingRing.  This only happens when
    // NumberOfAvailableRings is 0 (JohnsonA)
    //

    if ((Adapter->TransmittingRing == Adapter->FirstUncommittedRing) &&
        (Adapter->NumberOfAvailableRings > 0)
    )
    {
        return(FALSE);
    }
    else
    {
        FirstIndex = (UINT)(Adapter->TransmittingRing - Adapter->TransmitRing);
    }


    //
    // We put the mapping into a local variable so that we
    // can return the mapping as soon as possible.
    //

    SavedRingMapping = Adapter->RingToPacket[FirstIndex];

    //
    // Get a pointer to the last ring entry for this packet.
    //

    LastRingEntry = Adapter->TransmitRing + SavedRingMapping.RingIndex;

    //
    // Get a pointer to the owning packet .
    //
    OwningPacket = SavedRingMapping.OwningPacket;

    SavedRingMapping.OwningPacket = NULL;

    if (OwningPacket == NULL)
    {
        //
        // We seem to be in a messed up state.  Ignore this interrupt and
        // the wake up dpc will reset the card if necessary.
        //

        ASSERT(OwningPacket != NULL);
        return(FALSE);
    }

    //
    // Check that the host does indeed own this entire packet.
    //

    LANCE_READ_HARDWARE_MEMORY_UCHAR(
        LastRingEntry->TransmitSummaryBits,
        &TransmitSummaryBits
    );

    if (TransmitSummaryBits & LANCE_TRANSMIT_OWNED_BY_CHIP)
    {
        //
        // We don't own this last packet.  We return FALSE to indicate
        // that we don't have any more packets to work on.
        //
        return(FALSE);
    }
    else
    {
        //
        // Pointer to the current ring descriptor being examine for errors
        // and the statistics accumulated during its transmission.
        //
        PLANCE_TRANSMIT_ENTRY CurrentRingEntry;

        //
        // Holds whether the packet successfully transmitted or not.
        //
        BOOLEAN Successful = TRUE;
        PLANCE_BUFFER_DESCRIPTOR BufferDescriptor = Adapter->LanceBuffers +
                     SavedRingMapping.LanceBuffersIndex;
        INT ListHeadIndex = BufferDescriptor->Next;

        LOG(TRANSMIT_COMPLETE);

        CurrentRingEntry = Adapter->TransmitRing + FirstIndex;

        //
        // now return these buffers to the adapter.
        //

        BufferDescriptor->Next = Adapter->LanceBufferListHeads[ListHeadIndex];
        Adapter->LanceBufferListHeads[ListHeadIndex] = SavedRingMapping.LanceBuffersIndex;

        //
        // Since the host owns the entire packet check the ring
        // entries from first to last for any errors in transmission.
        // Any errors found or multiple tries should be recorded in
        // the information structure for the adapter.
        //
        // We treat Late Collisions as success since the packet was
        // fully transmitted and may have been received.
        //
        for (;;)
        {
            LANCE_READ_HARDWARE_MEMORY_UCHAR(
                CurrentRingEntry->TransmitSummaryBits,
                &TransmitSummaryBits
            );

            LANCE_READ_HARDWARE_MEMORY_USHORT(
                CurrentRingEntry->ErrorSummaryInfo,
                &ErrorSummaryInfo
            );

            if ((TransmitSummaryBits & LANCE_TRANSMIT_ANY_ERRORS) &&
                !(ErrorSummaryInfo & LANCE_TRANSMIT_LATE_COLLISION)
            )
            {
                if (ErrorSummaryInfo & LANCE_TRANSMIT_RETRY)
                {
                    Adapter->RetryFailure++;
                }
                else if (ErrorSummaryInfo & LANCE_TRANSMIT_LOST_CARRIER)
                {
                    Adapter->LostCarrier++;
                }
                else if (ErrorSummaryInfo & LANCE_TRANSMIT_UNDERFLOW)
                {
                    Adapter->UnderFlow++;
                }

#if DBG
                LanceSendFails[LanceSendFailPlace] = (UCHAR)(ErrorSummaryInfo);
                LanceSendFailPlace++;
#endif

#if LANCE_TRACE
                DbgPrint("Unsuccessful Transmit 0x%x\n", ErrorSummaryInfo);
#endif

                Successful = FALSE;

                //
                // Move the pointer to transmitting but unprocessed
                // ring entries to after this packet, and recover
                // the remaining now available ring entries.
                //

                Adapter->NumberOfAvailableRings +=
                    (UINT)((CurrentRingEntry <= LastRingEntry)?
                     ((LastRingEntry - CurrentRingEntry)+1):
                     ((Adapter->LastTransmitRingEntry - CurrentRingEntry) +
                        (LastRingEntry-Adapter->TransmitRing) + 2));

                if (LastRingEntry == Adapter->LastTransmitRingEntry)
                {
                    Adapter->TransmittingRing = Adapter->TransmitRing;
                }
                else
                {
                    Adapter->TransmittingRing = LastRingEntry + 1;
                }

                break;
            }
            else
            {
                //
                // Logical variable that records whether this
                // is the last packet.
                //

                BOOLEAN DoneWithPacket = TransmitSummaryBits & LANCE_TRANSMIT_END_OF_PACKET;

                if (ErrorSummaryInfo & LANCE_TRANSMIT_LATE_COLLISION)
                {
                    Adapter->LateCollision++;
                }

                if (TransmitSummaryBits & LANCE_TRANSMIT_START_OF_PACKET)
                {
                    //
                    // Collect some statistics on how many tries were needed.
                    // A frame can both defer and collide, so DEF is
                    // counted on its own.
                    //
                    if (TransmitSummaryBits & LANCE_TRANSMIT_DEFERRED)
                    {
                        Adapter->Deferred++;
                    }

                    if (TransmitSummaryBits & LANCE_TRANSMIT_ONE_RETRY)
                    {
                        Adapter->OneRetry++;
                    }
                    else if (TransmitSummaryBits & LANCE_TRANSMIT_MORE_THAN_ONE_RETRY)
                    {
                        Adapter->MoreThanOneRetry++;
                    }
                }

                if (CurrentRingEntry == Adapter->LastTransmitRingEntry)
                {
                    CurrentRingEntry = Adapter->TransmitRing;
                }
                else
                {
                    CurrentRingEntry++;
                }

                Adapter->TransmittingRing = CurrentRingEntry;
                Adapter->NumberOfAvailableRings++;

                if (DoneWithPacket)
                {
                    break;
                }
            }
        }

        //
        // Store result
        //
        if (Successful)
        {
            //
            // Increment number of packets successfully sent.
            //
            Adapter->Transmit++;
        }

        NdisMSendResourcesAvailable(Adapter->MiniportAdapterHandle);

        return(TRUE);
    }
}

STATIC
VOID
StartAdapterReset(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    This is the first phase of resetting the adapter hardware.

    It makes the following assumptions:

    1) That the hardware has been stopped.

    2) That it can not be preempted.

    3) That no other adapter activity can occur.

    When this routine is finished all of the adapter information
    will be as if the driver was just initialized.

Arguments:

    Adapter - The adapter whose hardware is to be reset.

Return Value:

    None.

--*/

{
    UINT i;

#if LANCE_TRACE
    DbgPrint("In StartAdapterReset\n");
#endif

    LOG(RESET_STEP_2);

    Adapter->NumberOfAvailableRings = Adapter->NumberOfTransmitRings;
    Adapter->AllocateableRing = Adapter->TransmitRing;
    Adapter->TransmittingRing = Adapter->TransmitRing;
    Adapter->FirstUncommittedRing = Adapter->TransmitRing;

    Adapter->CurrentReceiveIndex = 0;

    //
    // Clean all of the receive ring entries.
    //
    {

        PLANCE_RECEIVE_ENTRY CurrentReceive = Adapter->ReceiveRing;
        const PLANCE_RECEIVE_ENTRY After = Adapter->ReceiveRing +
                           Adapter->NumberOfReceiveRings;

        do
        {
            LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
                CurrentReceive->ReceiveSummaryBits,
                LANCE_RECEIVE_OWNED_BY_CHIP
            );

            CurrentReceive++;

        } while (CurrentReceive != After);
    }


    //
    // Clean all of the transmit ring entries.
    //

    {
        PLANCE_TRANSMIT_ENTRY CurrentTransmit = Adapter->TransmitRing;
        const PLANCE_TRANSMIT_ENTRY After = Adapter->TransmitRing+
                            Adapter->NumberOfTransmitRings;

        do
        {
            LANCE_WRITE_HARDWARE_MEMORY_UCHAR(
                CurrentTransmit->TransmitSummaryBits,
                0x00
            );

            CurrentTransmit++;
        } while (CurrentTransmit != After);
    }

    //
    // Recover all of the adapter buffers.
    //

    for (i = 0;
         i < (Adapter->NumberOfSmallBuffers +
              Adapter->NumberOfMediumBuffers +
              Adapter->NumberOfLargeBuffers);
         i++
    )
    {
        Adapter->LanceBuffers[i].Next = i+1;
    }

    Adapter->LanceBufferListHeads[0] = -1;
    Adapter->LanceBufferListHeads[1] = 0;
    Adapter->LanceBuffers[(Adapter->NumberOfSmallBuffers)-1].Next = -1;
    Adapter->LanceBufferListHeads[2] = Adapter->NumberOfSmallBuffers;
    Adapter->LanceBuffers[(Adapter->NumberOfSmallBuffers +
                           Adapter->NumberOfMediumBuffers)-1].Next = -1;
    Adapter->LanceBufferListHeads[3] = Adapter->NumberOfSmallBuffers +
                                      Adapter->NumberOfMediumBuffers;
    Adapter->LanceBuffers[(Adapter->NumberOfSmallBuffers+
                           Adapter->NumberOfMediumBuffers+
                           Adapter->NumberOfLargeBuffers)-1].Next = -1;

    SetInitBlockAndInit(Adapter);

#if LANCE_TRACE
    DbgPrint("Out StartAdapterReset\n");
#endif

}

STATIC
VOID
SetInitBlockAndInit(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    It is this routines responsibility to make sure that the
    initialization block is filled and the chip is initialized
    *but not* started.

    NOTE: This routine assumes that it is called with the lock
    acquired OR that only a single thread of execution is working
    with this particular adapter.

Arguments:

    Adapter - The adapter whose hardware is to be initialized.

Return Value:

    None.

--*/
{
    //
    // Fill in the adapters initialization block.
    //
    LanceSetInitializationBlock(Adapter);

    //
    // Selecting CSR1/CSR2 moves RAP away from CSR0, so do it with the ISR
    // locked out on every processor.  IDON is reported by interrupt:
    // LanceInitialInit waits for it, the reset path handles it in
    // LanceHandleInterrupt.
    //
    NdisMSynchronizeWithInterrupt(
        &Adapter->Interrupt,
        LanceSyncStartInit,
        (PVOID)Adapter
    );
}

STATIC
BOOLEAN
LanceSyncStartInit(
    IN PVOID Context
    )
{
    PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)Context;
    ULONG PhysAdr;

    PhysAdr = LANCE_GET_HARDWARE_PHYSICAL_ADDRESS(Adapter,Adapter->InitBlock);

    //
    // Make sure that it does have even byte alignment.
    //
    ASSERT((PhysAdr & 0x01)==0);

    //
    // Write the address of the initialization block to csr1 and csr2.
    // CSR2[15:8] is IADR[31:24]: in 16-bit mode the chip puts it on top
    // of every 24-bit address it uses, so it selects the DMA window.
    //
    ASSERT((PhysAdr & LANCE_DMA_WINDOW_MASK) == Adapter->DmaWindowBase);

    LANCE_WRITE_RAP(Adapter, LANCE_SELECT_CSR1);
    LANCE_WRITE_RDP(Adapter, LANCE_GET_LOW_PART_ADDRESS(PhysAdr));
    LANCE_WRITE_RAP(Adapter, LANCE_SELECT_CSR2);
    LANCE_WRITE_RDP(Adapter, LANCE_GET_CSR2_ADDRESS(PhysAdr));

    //
    // Write to csr0 to initialize the chip.
    //
    LANCE_WRITE_RAP(Adapter, LANCE_SELECT_CSR0);
    LANCE_WRITE_RDP(Adapter, LANCE_CSR0_INIT_CHIP);

    return(FALSE);
}

STATIC
VOID
LanceQueueSetBehindReset(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    A filter or multicast set arrived while a reset the driver started
    itself (memory error, babble, transmitter stopped) is still running.
    It used to fail, and if TCP/IP's filter set at bind time landed here
    nothing was received.  The caller has already stored the new value;
    this makes the running reset carry it and complete the request.

    NDIS sends one request at a time and none during MiniportReset, so
    no other completion is owed at this point.

--*/

{
    ASSERT(Adapter->ResetInProgress);
    ASSERT(!Adapter->SetInformationPending && !Adapter->ResetPending);

    Adapter->SetInformationPending = TRUE;

    if (Adapter->ResetInitStarted)
    {
        //
        // INIT was issued with the old init block.  The IDON handler
        // will run one more reset to load the new one.
        //
        Adapter->ReinitAfterReset = TRUE;
    }

    //
    // Otherwise StartAdapterReset has not run yet, and it builds the init
    // block from the values just stored.
    //
}

STATIC
VOID
SetupForReset(
    IN PLANCE_ADAPTER Adapter,
    IN NDIS_REQUEST_TYPE NdisRequestType
    )

/*++

Routine Description:

    This routine is used to fill in the who and why a reset is
    being set up as well as setting the appropriate fields in the
    adapter.

    NOTE: This routine must be called with the lock acquired.

Arguments:

    Adapter - The adapter whose hardware is to be initialized.

    NdisRequestType - The reason for the reset.

Return Value:

    None.

--*/
{

#if LANCE_TRACE
    DbgPrint("In SetupForReset\n");
#endif

    LOG(RESET_STEP_1);

    //
    // Once LanceHalt has started, no reset may begin: the DPC would
    // never start it (it checks Halting) and would loop on
    // ResetInProgress while LanceHalt waits for it in
    // NdisMDeregisterInterrupt.  A transmit-stopped or memory error
    // seen by a DPC that overlaps LanceHalt on another CPU lands here.
    //
    if (Adapter->Halting)
    {
        return;
    }

    //
    // Shut down the chip.  We won't be doing any more work until
    // the reset is complete.
    //
    NdisMSynchronizeWithInterrupt(
        &Adapter->Interrupt,
        LanceSyncStopChip,
        (PVOID)Adapter
    );

    //
    // Once the chip is stopped we can't get any more interrupts.
    // Any interrupts that are "queued" for processing could
    // only possibly service this reset.  It is therefore safe for
    // us to clear the adapter global csr value.
    //
    Adapter->ResetInProgress = TRUE;
    Adapter->ResetInitStarted = FALSE;

    //
    // Record what has to be completed once the chip is reinitialized.
    // Flags, not a single value: a MAC-issued reset (NdisRequestGeneric4)
    // that starts while a set-information is pending must not make us
    // forget to complete that request.
    //
    switch (NdisRequestType)
    {
        case NdisRequestSetInformation:
            Adapter->SetInformationPending = TRUE;
            break;

        case NdisRequestGeneric1:
            //
            // MiniportReset.  NDIS aborts a request that is still
            // pending on a serialized miniport before it calls
            // MiniportReset, so that request is no longer ours to
            // complete (this matches what the original driver did).
            //
            Adapter->ResetPending = TRUE;
            Adapter->SetInformationPending = FALSE;

            //
            // This reset rebuilds the init block from the current values
            // anyway, so no extra reset is needed afterwards.
            //
            Adapter->ReinitAfterReset = FALSE;
            break;

        default:
            break;
    }

#if LANCE_TRACE
    DbgPrint("Out SetupForReset\n");
#endif
}


STATIC
BOOLEAN
LanceSyncStopChip(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to stop a lance.



Arguments:

    Adapter - The adapter for the LANCE to stop.

Return Value:

    FALSE

--*/

{

    PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)Context;

    //
    // Set the RAP to csr0.
    //
    LANCE_ISR_WRITE_RAP(Adapter, LANCE_SELECT_CSR0);

    //
    // Set the RDP to stop chip.
    //
    LANCE_ISR_WRITE_RDP(Adapter, LANCE_CSR0_STOP);

    Adapter->PendingInterruptStatus = 0;

    //
    // Select CSR0 again.
    //
    LANCE_ISR_WRITE_RAP(Adapter, LANCE_SELECT_CSR0);

    return(FALSE);
}




