/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Novell Eagle 2000 driver
 * FILE:        ne2000/main.c
 * PURPOSE:     Driver entry point
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 27/08-2000 Created
 */
#include <ne2000.h>
#include <debug.h>

NTSTATUS
#ifndef _MSC_VER
NTAPI
#endif
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath);


#if DBG

/* See debug.h for debug/trace constants */
ULONG DebugTraceLevel = 0;

#endif /* DBG */


/* List of supported OIDs */
static ULONG MiniportOIDList[] = {
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_MAC_OPTIONS
};

DRIVER_INFORMATION      DriverInfo = { NULL, NULL, { NULL, NULL } };
NDIS_PHYSICAL_ADDRESS   HighestAcceptableMax = NDIS_PHYSICAL_ADDRESS_CONST(-1, -1);


static BOOLEAN NTAPI MiniportCheckForHang(
    IN  NDIS_HANDLE MiniportAdapterContext)
/*
 * FUNCTION: Examines if an adapter has hung
 * ARGUMENTS:
 *     MiniportAdapterContext = Pointer to adapter context area
 * RETURNS:
 *     TRUE if the adapter has hung, FALSE if not
 */
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return FALSE;
}


static VOID NTAPI MiniportDisableInterrupt(
    IN  NDIS_HANDLE MiniportAdapterContext)
/*
 * FUNCTION: Disables interrupts from an adapter
 * ARGUMENTS:
 *     MiniportAdapterContext = Pointer to adapter context area
 */
{
    NDIS_DbgPrint(MAX_TRACE, ("Called. (MiniportDisableInterrupt).\n"));
#ifndef NOCARD
    NICDisableInterrupts((PNIC_ADAPTER)MiniportAdapterContext);
#endif
}


static VOID NTAPI MiniportEnableInterrupt(
    IN  NDIS_HANDLE MiniportAdapterContext)
/*
 * FUNCTION: Enables interrupts from an adapter
 * ARGUMENTS:
 *     MiniportAdapterContext = Pointer to adapter context area
 */
{
    NDIS_DbgPrint(MAX_TRACE, ("Called. (MiniportEnableInterrupt).\n"));
#ifndef NOCARD
    NICEnableInterrupts((PNIC_ADAPTER)MiniportAdapterContext);
#endif
}


static VOID NTAPI MiniportHalt(
    IN  NDIS_HANDLE MiniportAdapterContext)
/*
 * FUNCTION: Deallocates resources for and halts an adapter
 * ARGUMENTS:
 *     MiniportAdapterContext = Pointer to adapter context area
 */
{
    PNIC_ADAPTER Adapter = (PNIC_ADAPTER)MiniportAdapterContext;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
#ifndef NOCARD
    /* Stop the NIC */
    NICStop(Adapter);
#endif
    /* Wait for any DPCs to complete. FIXME: Use something else */
    NdisStallExecution(250000);

    if (Adapter->InterruptRegistered)
        /* Deregister interrupt */
        NdisMDeregisterInterrupt(&Adapter->Interrupt);

    if (Adapter->IOPortRangeRegistered)
        /* Deregister I/O port range */
        NdisMDeregisterIoPortRange(
            Adapter->MiniportAdapterHandle,
            Adapter->IoBaseAddress,
            0x20,
            Adapter->IOBase);

    if (Adapter->ShutdownHandlerRegistered)
        NdisMDeregisterAdapterShutdownHandler(Adapter->MiniportAdapterHandle);

    /* Remove adapter from global adapter list */
    if ((&Adapter->ListEntry)->Blink != NULL) {
        RemoveEntryList(&Adapter->ListEntry);
    }

    /* Free adapter context area */
    NdisFreeMemory(Adapter, sizeof(NIC_ADAPTER), 0);
}


static VOID NTAPI MiQueryResources(
    OUT PNDIS_STATUS    Status,
    IN  PNIC_ADAPTER    Adapter,
    IN  NDIS_HANDLE     WrapperConfigurationContext)
{
    PNDIS_RESOURCE_LIST AssignedResources;
    UINT BufferSize = 0;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    UINT i;

    NdisMQueryAdapterResources(Status,
                               WrapperConfigurationContext,
                               NULL,
                               &BufferSize);
    if (*Status == NDIS_STATUS_SUCCESS)
        return;

    *Status = NdisAllocateMemory((PVOID)&AssignedResources,
                                 BufferSize,
                                 0,
                                 HighestAcceptableMax);
    if (*Status != NDIS_STATUS_SUCCESS)
        return;

    NdisMQueryAdapterResources(Status,
                               WrapperConfigurationContext,
                               AssignedResources,
                               &BufferSize);
    if (*Status != NDIS_STATUS_SUCCESS)
        return;

    for (i = 0; i < AssignedResources->Count; i++)
    {
        Descriptor = AssignedResources->PartialDescriptors + i;
        switch (Descriptor->Type)
        {
            case CmResourceTypeInterrupt:
                 Adapter->InterruptLevel = Descriptor->u.Interrupt.Level;
                 Adapter->InterruptVector = Descriptor->u.Interrupt.Vector;
                 Adapter->InterruptShared = (Descriptor->ShareDisposition == CmResourceShareShared);
                 Adapter->InterruptMode = Descriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED ?
                                          NdisInterruptLatched : NdisInterruptLevelSensitive;
                 break;
            case CmResourceTypePort:
                 Adapter->IoBaseAddress = Descriptor->u.Port.Start.LowPart;
                 break;
        }
    }
}

VOID
NTAPI
MiniportShutdown(PVOID Context)
{
  #ifndef NOCARD
    NICStop((PNIC_ADAPTER)Context);
  #endif
}

static NDIS_STATUS NTAPI MiniportInitialize(
    OUT PNDIS_STATUS    OpenErrorStatus,
    OUT PUINT           SelectedMediumIndex,
    IN  PNDIS_MEDIUM    MediumArray,
    IN  UINT            MediumArraySize,
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  NDIS_HANDLE     WrapperConfigurationContext)
/*
 * FUNCTION: Adapter initialization function
 * ARGUMENTS:
 *     OpenErrorStatus             = Address of buffer to place additional status information
 *     SelectedMediumIndex         = Address of buffer to place selected medium index
 *     MediumArray                 = Pointer to an array of NDIS_MEDIUMs
 *     MediaArraySize              = Number of elements in MediumArray
 *     MiniportAdapterHandle       = Miniport adapter handle assigned by NDIS
 *     WrapperConfigurationContext = Handle used to identify configuration context
 * RETURNS:
 *     Status of operation
 */
{
    UINT i;
    NDIS_STATUS Status;
    PNIC_ADAPTER Adapter;
    NDIS_HANDLE ConfigurationHandle;
    UINT *RegNetworkAddress = 0;
    UINT RegNetworkAddressLength = 0;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    NDIS_DbgPrint(MAX_TRACE, ("Called (Adapter %X).\n", MiniportAdapterHandle));

    /* Search for 802.3 media which is the only one we support */
    for (i = 0; i < MediumArraySize; i++) {
        if (MediumArray[i] == NdisMedium802_3)
            break;
    }

    if (i == MediumArraySize) {
        NDIS_DbgPrint(MIN_TRACE, ("No supported media.\n"));
        return NDIS_STATUS_UNSUPPORTED_MEDIA;
    }

    *SelectedMediumIndex = i;

    Status = NdisAllocateMemory((PVOID)&Adapter,
                                sizeof(NIC_ADAPTER),
                                0,
                                HighestAcceptableMax);
    if (Status != NDIS_STATUS_SUCCESS) {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return Status;
    }

    NdisZeroMemory(Adapter, sizeof(NIC_ADAPTER));
    Adapter->MiniportAdapterHandle  = MiniportAdapterHandle;
    Adapter->IoBaseAddress          = DRIVER_DEFAULT_IO_BASE_ADDRESS;
    Adapter->InterruptLevel         = DRIVER_DEFAULT_INTERRUPT_NUMBER;
    Adapter->InterruptVector        = DRIVER_DEFAULT_INTERRUPT_NUMBER;
    Adapter->InterruptShared        = DRIVER_DEFAULT_INTERRUPT_SHARED;
    Adapter->InterruptMode          = DRIVER_DEFAULT_INTERRUPT_MODE;
    Adapter->MaxMulticastListSize   = DRIVER_MAX_MULTICAST_LIST_SIZE;
    Adapter->InterruptMask          = DRIVER_INTERRUPT_MASK;
    Adapter->LookaheadSize          = DRIVER_MAXIMUM_LOOKAHEAD;

    /* Query the resources from PnP. */
    MiQueryResources(&Status, Adapter, WrapperConfigurationContext);

    /* Get the port, irq, and MAC address from registry if the PnP
       failed. */
    if (Status != NDIS_STATUS_SUCCESS)
    {
        PNDIS_CONFIGURATION_PARAMETER ConfigurationParameter;
        UNICODE_STRING Keyword;

        NdisOpenConfiguration(&Status, &ConfigurationHandle, WrapperConfigurationContext);
        if (Status == NDIS_STATUS_SUCCESS)
        {
            NdisInitUnicodeString(&Keyword, L"Irq");
            NdisReadConfiguration(&Status, &ConfigurationParameter, ConfigurationHandle, &Keyword, NdisParameterHexInteger);
            if(Status == NDIS_STATUS_SUCCESS)
            {
                NDIS_DbgPrint(MID_TRACE,("NdisReadConfiguration for Irq returned successfully, irq 0x%x\n",
                        ConfigurationParameter->ParameterData.IntegerData));
                Adapter->InterruptLevel =
                Adapter->InterruptVector = ConfigurationParameter->ParameterData.IntegerData;
            }

            NdisInitUnicodeString(&Keyword, L"Port");
            NdisReadConfiguration(&Status, &ConfigurationParameter, ConfigurationHandle, &Keyword, NdisParameterHexInteger);
            if(Status == NDIS_STATUS_SUCCESS)
            {
                NDIS_DbgPrint(MID_TRACE,("NdisReadConfiguration for Port returned successfully, port 0x%x\n",
                        ConfigurationParameter->ParameterData.IntegerData));
                Adapter->IoBaseAddress = ConfigurationParameter->ParameterData.IntegerData;
            }

            NdisCloseConfiguration(ConfigurationHandle);
        }
        else
        {
            NDIS_DbgPrint(MIN_TRACE,("NdisOpenConfiguration returned error 0x%x\n", Status));
        }
    }

     /* find the nic */
    if (!NICCheck(Adapter)) {
        NDIS_DbgPrint(MID_TRACE, ("No adapter found at (0x%X).\n", Adapter->IoBaseAddress));
        NdisFreeMemory(Adapter, sizeof(NIC_ADAPTER), 0);
        return NDIS_STATUS_ADAPTER_NOT_FOUND;
    } else
        NDIS_DbgPrint(MID_TRACE, ("Adapter found at (0x%X).\n", Adapter->IoBaseAddress));

    NdisMSetAttributes(
        MiniportAdapterHandle,
        (NDIS_HANDLE)Adapter,
        FALSE,
        NdisInterfaceIsa);

    Status = NdisMRegisterIoPortRange(
        (PVOID*)&Adapter->IOBase,
        MiniportAdapterHandle,
        Adapter->IoBaseAddress,
        0x20);

    if (Status != NDIS_STATUS_SUCCESS) {
        NDIS_DbgPrint(MIN_TRACE, ("Cannot register port range. Status (0x%X).\n", Status));
        MiniportHalt((NDIS_HANDLE)Adapter);
        return Status;
    }

    Adapter->IOPortRangeRegistered = TRUE;

    /* Initialize NIC */
#ifndef NOCARD
    Status = NICInitialize(Adapter);
    if (Status != NDIS_STATUS_SUCCESS) {
        NDIS_DbgPrint(MIN_TRACE,("No NE2000 or compatible network adapter found at address 0x%X.\n",
            Adapter->IOBase));

        NDIS_DbgPrint(MID_TRACE, ("Status (0x%X).\n", Status));
        MiniportHalt((NDIS_HANDLE)Adapter);
        return Status;
    }

    NdisOpenConfiguration(&Status, &ConfigurationHandle, WrapperConfigurationContext);
    if (Status == NDIS_STATUS_SUCCESS)
    {
         NdisReadNetworkAddress(&Status, (PVOID *)&RegNetworkAddress, &RegNetworkAddressLength, ConfigurationHandle);
         if(Status == NDIS_STATUS_SUCCESS && RegNetworkAddressLength == DRIVER_LENGTH_OF_ADDRESS)
         {
             int i;
             NDIS_DbgPrint(MID_TRACE,("NdisReadNetworkAddress returned successfully, address %x:%x:%x:%x:%x:%x\n",
                     RegNetworkAddress[0], RegNetworkAddress[1], RegNetworkAddress[2], RegNetworkAddress[3],
                     RegNetworkAddress[4], RegNetworkAddress[5]));
             for(i = 0; i < DRIVER_LENGTH_OF_ADDRESS; i++)
                 Adapter->StationAddress[i] = RegNetworkAddress[i];
         }

         NdisCloseConfiguration(ConfigurationHandle);
    }

    if (Status != NDIS_STATUS_SUCCESS)
    {
        int i;
        for (i = 0; i < DRIVER_LENGTH_OF_ADDRESS; i++)
             Adapter->StationAddress[i] = Adapter->PermanentAddress[i];
    }

    NDIS_DbgPrint(MID_TRACE, ("BOARDDATA:\n"));
    for (i = 0; i < 4; i++) {
        NDIS_DbgPrint(MID_TRACE, ("%02X %02X %02X %02X\n",
            Adapter->SAPROM[i*4+0],
            Adapter->SAPROM[i*4+1],
            Adapter->SAPROM[i*4+2],
            Adapter->SAPROM[i*4+3]));
    }

    /* Setup adapter structure */
    Adapter->TXStart   = ((ULONG_PTR)Adapter->RamBase >> 8);
    Adapter->TXCount   = DRIVER_DEFAULT_TX_BUFFER_COUNT;
    Adapter->TXFree    = DRIVER_DEFAULT_TX_BUFFER_COUNT;
    Adapter->TXCurrent = -1;
    Adapter->PageStart = Adapter->TXStart + Adapter->TXCount;
    Adapter->PageStop  = Adapter->TXStart + (Adapter->RamSize >> 8);

    /* Initialize multicast address mask to accept all */
    for (i = 0; i < 8; i++)
        Adapter->MulticastAddressMask[i] = 0xFF;

    /* Setup the NIC */
    NICSetup(Adapter);

    NDIS_DbgPrint(MID_TRACE, ("TXStart (0x%X)  TXCount (0x%X)  PageStart (0x%X)\n",
        Adapter->TXStart,
        Adapter->TXCount,
        Adapter->PageStart));

    NDIS_DbgPrint(MID_TRACE, ("PageStop (0x%X)  CurrentPage (0x%X)  NextPacket (0x%X).\n",
        Adapter->PageStop,
        Adapter->CurrentPage,
        Adapter->NextPacket));
#endif
    /* Register the interrupt */
    Status = NdisMRegisterInterrupt(
        &Adapter->Interrupt,
        MiniportAdapterHandle,
        Adapter->InterruptVector,
        Adapter->InterruptLevel,
        FALSE,
        Adapter->InterruptShared,
        Adapter->InterruptMode);
    if (Status != NDIS_STATUS_SUCCESS) {
        NDIS_DbgPrint(MIN_TRACE, ("Cannot register interrupt. Status (0x%X).\n", Status));
        MiniportHalt((NDIS_HANDLE)Adapter);
        return Status;
    }

    Adapter->InterruptRegistered = TRUE;
#ifndef NOCARD
    /* Start the NIC */
    NICStart(Adapter);
#endif

    /* Register the shutdown handler */
    NdisMRegisterAdapterShutdownHandler(MiniportAdapterHandle, Adapter, MiniportShutdown);

    Adapter->ShutdownHandlerRegistered = TRUE;

    /* Add adapter to the global adapter list */
    InsertTailList(&DriverInfo.AdapterListHead, &Adapter->ListEntry);

    NDIS_DbgPrint(MAX_TRACE, ("Leaving.\n"));

    return NDIS_STATUS_SUCCESS;
}


static VOID NTAPI MiniportISR(
    OUT PBOOLEAN    InterruptRecognized,
    OUT PBOOLEAN    QueueMiniportHandleInterrupt,
    IN  NDIS_HANDLE MiniportAdapterContext)
/*
 * FUNCTION: Interrupt Service Routine for controlled adapters
 * ARGUMENTS:
 *     InterruptRecognized          = Address of buffer to place wether
 *                                    the adapter generated the interrupt
 *     QueueMiniportHandleInterrupt = Address of buffer to place wether
 *                                    MiniportHandleInterrupt should be called
 *     MiniportAdapterContext       = Pointer to adapter context area
 * NOTES:
 *     All pending interrupts are handled
 */
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    NICDisableInterrupts((PNIC_ADAPTER)MiniportAdapterContext);

    *InterruptRecognized          = TRUE;
    *QueueMiniportHandleInterrupt = TRUE;
}


static NDIS_STATUS NTAPI MiniportQueryInformation(
    IN  NDIS_HANDLE MiniportAdapterContext,
    IN  NDIS_OID    Oid,
    IN  PVOID       InformationBuffer,
    IN  ULONG       InformationBufferLength,
    OUT PULONG      BytesWritten,
    OUT PULONG      BytesNeeded)
/*
 * FUNCTION: Handler to process queries
 * ARGUMENTS:
 *     MiniportAdapterContext  = Pointer to adapter context area
 *     Oid                     = OID code designating query operation
 *     InformationBuffer       = Address of return buffer
 *     InformationBufferLength = Length of return buffer
 *     BytesWritten            = Address of buffer to place number of bytes returned
 *     BytesNeeded             = Address of buffer to place number of bytes needed
 *                               in InformationBuffer for specified OID
 * RETURNS:
 *     Status of operation
 */
{
    NDIS_STATUS Status;
    PVOID CopyFrom;
    UINT CopySize;
    ULONG GenericULONG;
    USHORT GenericUSHORT;
    NDIS_MEDIUM Medium   = NdisMedium802_3;
    PNIC_ADAPTER Adapter = (PNIC_ADAPTER)MiniportAdapterContext;

    ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);

    NDIS_DbgPrint(MAX_TRACE, ("Called. Oid (0x%X).\n", Oid));

    Status   = NDIS_STATUS_SUCCESS;
    CopyFrom = (PVOID)&GenericULONG;
    CopySize = sizeof(ULONG);

    switch (Oid) {
    case OID_GEN_SUPPORTED_LIST:
        CopyFrom = (PVOID)&MiniportOIDList;
        CopySize = sizeof(MiniportOIDList);
        break;
    case OID_GEN_HARDWARE_STATUS:
        GenericULONG = (ULONG)NdisHardwareStatusReady;
        break;
    case OID_GEN_MEDIA_SUPPORTED:
    case OID_GEN_MEDIA_IN_USE:
        CopyFrom = (PVOID)&Medium;
        CopySize = sizeof(NDIS_MEDIUM);
        break;
    case OID_GEN_MAXIMUM_LOOKAHEAD:
        GenericULONG = DRIVER_MAXIMUM_LOOKAHEAD;
        break;
    case OID_GEN_MAXIMUM_FRAME_SIZE:
        GenericULONG = DRIVER_FRAME_SIZE - DRIVER_HEADER_SIZE;
        break;
    case OID_GEN_LINK_SPEED:
        GenericULONG = 100000;  /* 10Mbps */
        break;
    case OID_GEN_TRANSMIT_BUFFER_SPACE:
        GenericULONG = Adapter->TXCount * DRIVER_BLOCK_SIZE;
        break;
    case OID_GEN_RECEIVE_BUFFER_SPACE:
        GenericULONG = Adapter->RamSize -
                       (ULONG_PTR)Adapter->RamBase -
                       (Adapter->TXCount * DRIVER_BLOCK_SIZE);
        break;
    case OID_GEN_TRANSMIT_BLOCK_SIZE:
        GenericULONG = DRIVER_BLOCK_SIZE;
        break;
    case OID_GEN_RECEIVE_BLOCK_SIZE:
        GenericULONG = DRIVER_BLOCK_SIZE;
        break;
    case OID_GEN_VENDOR_ID:
        NdisMoveMemory(&GenericULONG, &Adapter->PermanentAddress, 3);
        GenericULONG &= 0xFFFFFF00;
        GenericULONG |= 0x01;
        break;
    case OID_GEN_VENDOR_DESCRIPTION:
        CopyFrom = (PVOID)&DRIVER_VENDOR_DESCRIPTION;
        CopySize = sizeof(DRIVER_VENDOR_DESCRIPTION);
        break;
    case OID_GEN_VENDOR_DRIVER_VERSION:
        GenericUSHORT = (USHORT)DRIVER_VENDOR_DRIVER_VERSION;
        CopyFrom      = (PVOID)&GenericUSHORT;
        CopySize      = sizeof(USHORT);
        break;
    case OID_GEN_CURRENT_PACKET_FILTER:
        GenericULONG = Adapter->PacketFilter;
        break;
    case OID_GEN_CURRENT_LOOKAHEAD:
        GenericULONG = Adapter->LookaheadSize;
        break;
    case OID_GEN_DRIVER_VERSION:
        GenericUSHORT = ((USHORT)DRIVER_NDIS_MAJOR_VERSION << 8) | DRIVER_NDIS_MINOR_VERSION;
        CopyFrom      = (PVOID)&GenericUSHORT;
        CopySize      = sizeof(USHORT);
        break;
    case OID_GEN_MAXIMUM_TOTAL_SIZE:
        GenericULONG = DRIVER_FRAME_SIZE;
        break;
    case OID_GEN_PROTOCOL_OPTIONS:
        NDIS_DbgPrint(MID_TRACE, ("OID_GEN_PROTOCOL_OPTIONS.\n"));
        Status = NDIS_STATUS_NOT_SUPPORTED;
        break;
    case OID_GEN_MAC_OPTIONS:
        GenericULONG = NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA |
                       NDIS_MAC_OPTION_RECEIVE_SERIALIZED  |
                       NDIS_MAC_OPTION_TRANSFERS_NOT_PEND  |
                       NDIS_MAC_OPTION_NO_LOOPBACK;
        break;
    case OID_GEN_MEDIA_CONNECT_STATUS:
        GenericULONG = (ULONG)NdisMediaStateConnected;
        break;
    case OID_GEN_MAXIMUM_SEND_PACKETS:
        GenericULONG = 1;
        break;
    case OID_802_3_PERMANENT_ADDRESS:
        CopyFrom = (PVOID)&Adapter->PermanentAddress;
        CopySize = DRIVER_LENGTH_OF_ADDRESS;
        break;
    case OID_802_3_CURRENT_ADDRESS:
        CopyFrom = (PVOID)&Adapter->StationAddress;
        CopySize = DRIVER_LENGTH_OF_ADDRESS;
        break;
    case OID_802_3_MULTICAST_LIST:
        NDIS_DbgPrint(MID_TRACE, ("OID_802_3_MULTICAST_LIST.\n"));
        Status = NDIS_STATUS_NOT_SUPPORTED;
        break;
    case OID_802_3_MAXIMUM_LIST_SIZE:
        GenericULONG = Adapter->MaxMulticastListSize;
        break;
    case OID_802_3_MAC_OPTIONS:
        NDIS_DbgPrint(MID_TRACE, ("OID_802_3_MAC_OPTIONS.\n"));
        Status = NDIS_STATUS_NOT_SUPPORTED;
        break;
    default:
        NDIS_DbgPrint(MIN_TRACE, ("Unknown OID (0x%X).\n", Oid));
        Status = NDIS_STATUS_INVALID_OID;
        break;
    }

    if (Status == NDIS_STATUS_SUCCESS) {
        if (CopySize > InformationBufferLength) {
            *BytesNeeded  = (CopySize - InformationBufferLength);
            *BytesWritten = 0;
            Status        = NDIS_STATUS_INVALID_LENGTH;
        } else {
            NdisMoveMemory(InformationBuffer, CopyFrom, CopySize);
            *BytesWritten = CopySize;
            *BytesNeeded  = 0;
         }
    }

    NDIS_DbgPrint(MAX_TRACE, ("Leaving. Status is (0x%X).\n", Status));

    return Status;
}


static NDIS_STATUS NTAPI MiniportReconfigure(
    OUT PNDIS_STATUS    OpenErrorStatus,
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  NDIS_HANDLE     WrapperConfigurationContext)
/*
 * FUNCTION: Reconfigures an adapter
 * ARGUMENTS:
 *     OpenErrorStatus             = Address of buffer to place additional status information
 *     MiniportAdapterContext      = Pointer to adapter context area
 *     WrapperConfigurationContext = Handle used to identify configuration context
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     Never called by NDIS library
 */
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return NDIS_STATUS_FAILURE;
}



static NDIS_STATUS NTAPI MiniportReset(
    OUT PBOOLEAN    AddressingReset,
    IN  NDIS_HANDLE MiniportAdapterContext)
/*
 * FUNCTION: Resets an adapter
 * ARGUMENTS:
 *     AddressingReset        = Address of a buffer to place value indicating
 *                              wether NDIS library should call MiniportSetInformation
 *                              to restore addressing information
 *     MiniportAdapterContext = Pointer to adapter context area
 * RETURNS:
 *     Status of operation
 */
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;

    ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

#ifndef NOCARD
    NdisStatus = NICReset((PNIC_ADAPTER)MiniportAdapterContext);
#endif

    *AddressingReset = TRUE;

    return NdisStatus;
}


static NDIS_STATUS NTAPI MiniportSend(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  PNDIS_PACKET    Packet,
    IN  UINT            Flags)
/*
 * FUNCTION: Transmits a packet
 * ARGUMENTS:
 *     MiniportAdapterContext = Pointer to adapter context area
 *     Packet                 = Pointer to a packet descriptor specifying
 *                              the data to be transmitted
 *     Flags                  = Specifies optional packet flags
 * RETURNS:
 *     Status of operation
 */
{
    PNIC_ADAPTER Adapter = (PNIC_ADAPTER)MiniportAdapterContext;

    ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);

#ifndef NOCARD
    NDIS_DbgPrint(MID_TRACE, ("Queueing packet.\n"));

    /* Queue the packet on the transmit queue */
    RESERVED(Packet)->Next = NULL;
    if (Adapter->TXQueueHead == NULL) {
        Adapter->TXQueueHead = Packet;
    } else {
        RESERVED(Adapter->TXQueueTail)->Next = Packet;
    }

    Adapter->TXQueueTail = Packet;

    /* Transmit the packet */
    NICTransmit(Adapter);

    return NDIS_STATUS_PENDING;
#else
    return NDIS_STATUS_SUCCESS;
#endif
}


static NDIS_STATUS NTAPI MiniportSetInformation(
    IN  NDIS_HANDLE MiniportAdapterContext,
    IN  NDIS_OID    Oid,
    IN  PVOID       InformationBuffer,
    IN  ULONG       InformationBufferLength,
    OUT PULONG      BytesRead,
    OUT PULONG      BytesNeeded)
/*
 * FUNCTION: Changes state information in the driver
 * ARGUMENTS:
 *     MiniportAdapterContext  = Pointer to adapter context area
 *     Oid                     = OID code designating set operation
 *     InformationBuffer       = Pointer to buffer with state information
 *     InformationBufferLength = Length of InformationBuffer
 *     BytesRead               = Address of buffer to place number of bytes read
 *     BytesNeeded             = Address of buffer to place number of extra bytes
 *                               needed in InformationBuffer for specified OID
 * RETURNS:
 *     Status of operation
 */
{
    ULONG GenericULONG;
    NDIS_STATUS Status   = NDIS_STATUS_SUCCESS;
    PNIC_ADAPTER Adapter = (PNIC_ADAPTER)MiniportAdapterContext;

    ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);

    NDIS_DbgPrint(MAX_TRACE, ("Called. Oid (0x%X).\n", Oid));

    switch (Oid) {
    case OID_GEN_CURRENT_PACKET_FILTER:
        /* Verify length */
        if (InformationBufferLength < sizeof(ULONG)) {
            *BytesRead   = 0;
            *BytesNeeded = sizeof(ULONG) - InformationBufferLength;
            Status       = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        NdisMoveMemory(&GenericULONG, InformationBuffer, sizeof(ULONG));
        /* Check for properties the driver don't support */
        if (GenericULONG &
           (NDIS_PACKET_TYPE_ALL_FUNCTIONAL |
            NDIS_PACKET_TYPE_FUNCTIONAL     |
            NDIS_PACKET_TYPE_GROUP          |
            NDIS_PACKET_TYPE_MAC_FRAME      |
            NDIS_PACKET_TYPE_SMT            |
            NDIS_PACKET_TYPE_SOURCE_ROUTING)) {
            *BytesRead   = 4;
            *BytesNeeded = 0;
            Status       = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        Adapter->PacketFilter = GenericULONG;

        /* FIXME: Set filter on hardware */

        break;
    case OID_GEN_CURRENT_LOOKAHEAD:
        /* Verify length */
        if (InformationBufferLength < sizeof(ULONG)) {
            *BytesRead   = 0;
            *BytesNeeded = sizeof(ULONG) - InformationBufferLength;
            Status = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        NdisMoveMemory(&GenericULONG, InformationBuffer, sizeof(ULONG));
        if (GenericULONG > DRIVER_MAXIMUM_LOOKAHEAD)
            Status = NDIS_STATUS_INVALID_LENGTH;
        else
            Adapter->LookaheadSize = GenericULONG;
        break;
    case OID_802_3_MULTICAST_LIST:
        /* Verify length. Must be multiplum of hardware address length */
        if ((InformationBufferLength % DRIVER_LENGTH_OF_ADDRESS) != 0) {
            *BytesRead   = 0;
            *BytesNeeded = 0;
            Status       = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        /* Set new multicast address list */
        NdisMoveMemory(Adapter->Addresses, InformationBuffer, InformationBufferLength);

        /* FIXME: Update hardware */

        break;
    default:
        NDIS_DbgPrint(MIN_TRACE, ("Invalid object ID (0x%X).\n", Oid));
        *BytesRead   = 0;
        *BytesNeeded = 0;
        Status       = NDIS_STATUS_INVALID_OID;
        break;
    }

    if (Status == NDIS_STATUS_SUCCESS) {
        *BytesRead   = InformationBufferLength;
        *BytesNeeded = 0;
    }

    NDIS_DbgPrint(MAX_TRACE, ("Leaving. Status (0x%X).\n", Status));

    return Status;
}


static NDIS_STATUS NTAPI MiniportTransferData(
    OUT PNDIS_PACKET    Packet,
    OUT PUINT           BytesTransferred,
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  NDIS_HANDLE     MiniportReceiveContext,
    IN  UINT            ByteOffset,
    IN  UINT            BytesToTransfer)
/*
 * FUNCTION: Transfers data from a received frame into an NDIS packet
 * ARGUMENTS:
 *     Packet                 = Address of packet to copy received data into
 *     BytesTransferred       = Address of buffer to place number of bytes transmitted
 *     MiniportAdapterContext = Pointer to adapter context area
 *     MiniportReceiveContext = Pointer to receive context area (actually NULL)
 *     ByteOffset             = Offset within received packet to begin copying
 *     BytesToTransfer        = Number of bytes to copy into packet
 * RETURNS:
 *     Status of operation
 */
{
    PNDIS_BUFFER DstBuffer;
    UINT BytesCopied, BytesToCopy, DstSize;
    ULONG SrcData;
    PUCHAR DstData;
    UINT RecvStart;
    UINT RecvStop;
    PNIC_ADAPTER Adapter = (PNIC_ADAPTER)MiniportAdapterContext;

    ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);

    NDIS_DbgPrint(MAX_TRACE, ("Called. Packet (0x%X)  ByteOffset (0x%X)  BytesToTransfer (%d).\n",
        Packet, ByteOffset, BytesToTransfer));

    if (BytesToTransfer == 0) {
        *BytesTransferred = 0;
        return NDIS_STATUS_SUCCESS;
    }

    RecvStart = Adapter->PageStart * DRIVER_BLOCK_SIZE;
    RecvStop  = Adapter->PageStop  * DRIVER_BLOCK_SIZE;

    NdisQueryPacket(Packet, NULL, NULL, &DstBuffer, NULL);
    NdisQueryBuffer(DstBuffer, (PVOID)&DstData, &DstSize);

    SrcData = Adapter->PacketOffset + sizeof(DISCARD_HEADER) + ByteOffset;
    if (ByteOffset + sizeof(DISCARD_HEADER) + BytesToTransfer >
	Adapter->PacketHeader.PacketLength)
        BytesToTransfer = Adapter->PacketHeader.PacketLength -
	    sizeof(DISCARD_HEADER) - ByteOffset;

    /* Start copying the data */
    BytesCopied = 0;
    for (;;) {
        BytesToCopy = (DstSize < BytesToTransfer) ? DstSize : BytesToTransfer;
        if (SrcData + BytesToCopy > RecvStop)
            BytesToCopy = (RecvStop - SrcData);

        NICReadData(Adapter, DstData, SrcData, BytesToCopy);

        BytesCopied     += BytesToCopy;
        SrcData         += BytesToCopy;
        DstData          = (PUCHAR)((ULONG_PTR) DstData + BytesToCopy);
        BytesToTransfer -= BytesToCopy;
        if (BytesToTransfer == 0)
            break;

        DstSize -= BytesToCopy;
        if (DstSize == 0) {
            /* No more bytes in destination buffer. Proceed to
               the next buffer in the destination buffer chain */
            NdisGetNextBuffer(DstBuffer, &DstBuffer);
            if (!DstBuffer)
                break;

            NdisQueryBuffer(DstBuffer, (PVOID)&DstData, &DstSize);
        }

        if (SrcData == RecvStop)
            SrcData = RecvStart;
    }

    NDIS_DbgPrint(MID_TRACE, ("Transferred (%d) bytes.\n", BytesToTransfer));

    *BytesTransferred = BytesCopied;

    return NDIS_STATUS_SUCCESS;
}


NTSTATUS
#ifndef _MSC_VER
NTAPI
#endif
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Main driver entry point
 * ARGUMENTS:
 *     DriverObject = Pointer to a driver object for this driver
 *     RegistryPath = Registry node for configuration parameters
 * RETURNS:
 *     Status of driver initialization
 */
{
    NDIS_STATUS Status;
    NDIS_HANDLE NdisWrapperHandle;
    NDIS_MINIPORT_CHARACTERISTICS Miniport;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    NdisZeroMemory(&Miniport, sizeof(Miniport));
    Miniport.MajorNdisVersion           = DRIVER_NDIS_MAJOR_VERSION;
    Miniport.MinorNdisVersion           = DRIVER_NDIS_MINOR_VERSION;
    Miniport.CheckForHangHandler        = MiniportCheckForHang;
    Miniport.DisableInterruptHandler    = MiniportDisableInterrupt;
    Miniport.EnableInterruptHandler     = MiniportEnableInterrupt;
    Miniport.HaltHandler                = MiniportHalt;
    Miniport.HandleInterruptHandler     = MiniportHandleInterrupt;
    Miniport.InitializeHandler          = MiniportInitialize;
    Miniport.ISRHandler                 = MiniportISR;
    Miniport.QueryInformationHandler    = MiniportQueryInformation;
    Miniport.ReconfigureHandler         = MiniportReconfigure;
    Miniport.ResetHandler               = MiniportReset;
    Miniport.SendHandler                = MiniportSend;
    Miniport.SetInformationHandler      = MiniportSetInformation;
    Miniport.TransferDataHandler        = MiniportTransferData;

    NdisMInitializeWrapper(&NdisWrapperHandle,
                           DriverObject,
                           RegistryPath,
                           NULL);

    if (!NdisWrapperHandle) {
        NDIS_DbgPrint(MIN_TRACE, ("NdisMInitializeWrapper() failed\n"));
        return STATUS_UNSUCCESSFUL;
    }

    DriverInfo.NdisWrapperHandle = NdisWrapperHandle;
    DriverInfo.NdisMacHandle     = NULL;
    InitializeListHead(&DriverInfo.AdapterListHead);

    Status = NdisMRegisterMiniport(NdisWrapperHandle,
                                   &Miniport,
                                   sizeof(NDIS_MINIPORT_CHARACTERISTICS));
    if (Status != NDIS_STATUS_SUCCESS) {
        NDIS_DbgPrint(MIN_TRACE, ("NdisMRegisterMiniport() failed with status code (0x%X).\n", Status));
        NdisTerminateWrapper(NdisWrapperHandle, NULL);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

#if 0
        /* while i'm here - some basic registry sanity checks */
        {
            /* write tests */
            NDIS_CONFIGURATION_PARAMETER ParameterValue;

            ParameterValue.ParameterType = NdisParameterInteger;
            ParameterValue.ParameterData.IntegerData = 0x12345678;
            NdisInitUnicodeString(&Keyword, L"DwordTest");
            NdisWriteConfiguration(&Status, ConfigurationHandle, &Keyword, &ParameterValue);

            if(Status != NDIS_STATUS_SUCCESS)
            {
                DbgPrint("ne2000!MiniportInitialize: failed to set DwordTest: 0x%x\n", Status);
                KeBugCheck(0);
            }

            DbgPrint("ne2000!MiniportInitialize: DwordTest successfully set\n");

            NdisInitUnicodeString(&Keyword, L"StringTest");
            ParameterValue.ParameterType = NdisParameterString;
            NdisInitUnicodeString(&ParameterValue.ParameterData.StringData, L"Testing123");

            NdisWriteConfiguration(&Status, ConfigurationHandle, &Keyword, &ParameterValue);

            if(Status != NDIS_STATUS_SUCCESS)
            {
                DbgPrint("ne2000!MiniportInitialize: failed to set StringTest: 0x%x\n", Status);
                KeBugCheck(0);
            }

            DbgPrint("ne2000!MiniportInitialize: StringTest successfully set\n");
        }

        {
            /* read back the test values */
            NDIS_CONFIGURATION_PARAMETER *ParameterValue = 0;

            NdisInitUnicodeString(&Keyword, L"DwordTest");
            NdisReadConfiguration(&Status, &ParameterValue, ConfigurationHandle, &Keyword, NdisParameterInteger);

            if(Status != NDIS_STATUS_SUCCESS)
            {
                DbgPrint("ne2000!MiniportInitialize: failed to read DwordTest: 0x%x\n", Status);
                KeBugCheck(0);
            }

            if(ParameterValue->ParameterData.IntegerData != 0x12345678)
            {
                DbgPrint("ne2000!MiniportInitialize: DwordTest value is wrong: 0x%x\n",
                    ParameterValue->ParameterData.IntegerData);
                KeBugCheck(0);
            }

            DbgPrint("ne2000!MiniportInitialize: DwordTest value was correctly read\n");

            NdisInitUnicodeString(&Keyword, L"StringTest");
            NdisReadConfiguration(&Status, &ParameterValue, ConfigurationHandle, &Keyword, NdisParameterString);

            if(Status != NDIS_STATUS_SUCCESS)
            {
                DbgPrint("ne2000!MiniportInitialize: failed to read StringTest: 0x%x\n", Status);
                KeBugCheck(0);
            }

            if(wcsncmp(ParameterValue->ParameterData.StringData.Buffer, L"Testing123",
                    wcslen(L"Testing123")))
            {
                DbgPrint("ne2000!MiniportInitialize: StringTest value is wrong: %wZ\n",
                    &ParameterValue->ParameterData.StringData);
                KeBugCheck(0);
            }

            DbgPrint("ne2000!MiniportInitialize: StringTest value was correctly read\n");
        }

#endif
/* EOF */
