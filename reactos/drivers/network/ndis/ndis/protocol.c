/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/protocol.c
 * PURPOSE:     Routines used by NDIS protocol drivers
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 *   09-13-2003 Vizzini Updates for SendPackets support
 */

#include "ndissys.h"
#include <buffer.h>

#define SERVICES_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"
#define LINKAGE_KEY  L"\\Linkage"
#define PARAMETERS_KEY L"\\Parameters\\"

LIST_ENTRY ProtocolListHead;
KSPIN_LOCK ProtocolListLock;

#define WORKER_TEST 0


/*
 * @implemented
 */
VOID
EXPORT
NdisCompleteBindAdapter(
    IN  NDIS_HANDLE BindAdapterContext,
    IN  NDIS_STATUS Status,
    IN  NDIS_STATUS OpenStatus)
/*
 * FUNCTION: Indicates a packet to bound protocols
 * ARGUMENTS:
 *     Adapter = Pointer to logical adapter
 *     Packet  = Pointer to packet to indicate
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     - FIXME: partially-implemented
 */
{
  PROTOCOL_BINDING *Protocol = (PROTOCOL_BINDING *)BindAdapterContext;

  if (!NT_SUCCESS(Status)) return;

  /* Put protocol binding struct on global list */
  ExInterlockedInsertTailList(&ProtocolListHead, &Protocol->ListEntry, &ProtocolListLock);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisCompleteUnbindAdapter(
    IN  NDIS_HANDLE UnbindAdapterContext,
    IN  NDIS_STATUS Status)
{
  /* We probably need to do more here but for now we just do
   * the opposite of what NdisCompleteBindAdapter does
   */

  PROTOCOL_BINDING *Protocol = (PROTOCOL_BINDING *)UnbindAdapterContext;

  if (!NT_SUCCESS(Status)) return;

  ExInterlockedRemoveEntryList(&Protocol->ListEntry, &ProtocolListLock);
}


NDIS_STATUS
ProIndicatePacket(
    PLOGICAL_ADAPTER Adapter,
    PNDIS_PACKET Packet)
/*
 * FUNCTION: Indicates a packet to bound protocols
 * ARGUMENTS:
 *     Adapter = Pointer to logical adapter
 *     Packet  = Pointer to packet to indicate
 * RETURNS:
 *     STATUS_SUCCESS in all cases
 * NOTES:
 *     - XXX ATM, this only handles loopback packets - is that its designed function?
 */
{
  UINT BufferedLength;
  UINT PacketLength;
  KIRQL OldIrql;
  PUCHAR LookaheadBuffer;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

#ifdef DBG
  MiniDisplayPacket(Packet);
#endif

  LookaheadBuffer = ExAllocatePool(NonPagedPool, Adapter->NdisMiniportBlock.CurrentLookahead + Adapter->MediumHeaderSize);
  if (!LookaheadBuffer)
      return NDIS_STATUS_RESOURCES;

  NdisQueryPacket(Packet, NULL, NULL, NULL, &PacketLength);

  NDIS_DbgPrint(MAX_TRACE, ("acquiring miniport block lock\n"));
  KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);
    {
      BufferedLength = CopyPacketToBuffer(LookaheadBuffer, Packet, 0, Adapter->NdisMiniportBlock.CurrentLookahead);
    }
  KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);

  if (BufferedLength > Adapter->MediumHeaderSize)
    {
      /* XXX Change this to call SendPackets so we don't have to duplicate this wacky logic */
      MiniIndicateData(Adapter, NULL, LookaheadBuffer, Adapter->MediumHeaderSize,
          &LookaheadBuffer[Adapter->MediumHeaderSize], BufferedLength - Adapter->MediumHeaderSize,
          PacketLength - Adapter->MediumHeaderSize);
    }
  else
    {
      MiniIndicateData(Adapter, NULL, LookaheadBuffer, Adapter->MediumHeaderSize, NULL, 0, 0);
    }

  ExFreePool(LookaheadBuffer);

  return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS NTAPI
ProRequest(
    IN  NDIS_HANDLE     MacBindingHandle,
    IN  PNDIS_REQUEST   NdisRequest)
/*
 * FUNCTION: Forwards a request to an NDIS miniport
 * ARGUMENTS:
 *     MacBindingHandle = Adapter binding handle
 *     NdisRequest      = Pointer to request to perform
 * RETURNS:
 *     Status of operation
 */
{
  PADAPTER_BINDING AdapterBinding;
  PLOGICAL_ADAPTER Adapter;
  PNDIS_REQUEST_MAC_BLOCK MacBlock = (PNDIS_REQUEST_MAC_BLOCK)NdisRequest->MacReserved;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  ASSERT(MacBindingHandle);
  AdapterBinding = GET_ADAPTER_BINDING(MacBindingHandle);

  ASSERT(AdapterBinding->Adapter);
  Adapter = AdapterBinding->Adapter;

  MacBlock->Binding = &AdapterBinding->NdisOpenBlock;

#if WORKER_TEST
  MiniQueueWorkItem(Adapter, NdisWorkItemRequest, NdisRequest, FALSE);
  return NDIS_STATUS_PENDING;
#else
  if (MiniIsBusy(Adapter, NdisWorkItemRequest)) {
      MiniQueueWorkItem(Adapter, NdisWorkItemRequest, NdisRequest, FALSE);
      return NDIS_STATUS_PENDING;
  }

  return MiniDoRequest(Adapter, NdisRequest);
#endif
}


NDIS_STATUS NTAPI
ProReset(
    IN  NDIS_HANDLE MacBindingHandle)
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
proSendPacketToMiniport(PLOGICAL_ADAPTER Adapter, PNDIS_PACKET Packet)
{
#if WORKER_TEST
   MiniQueueWorkItem(Adapter, NdisWorkItemSend, Packet, FALSE);
   return NDIS_STATUS_PENDING;
#else
   KIRQL RaiseOldIrql;
   NDIS_STATUS NdisStatus;

   if(MiniIsBusy(Adapter, NdisWorkItemSend)) {
      MiniQueueWorkItem(Adapter, NdisWorkItemSend, Packet, FALSE);
      return NDIS_STATUS_PENDING;
   }

   if(Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendPacketsHandler)
   {
        if(Adapter->NdisMiniportBlock.Flags & NDIS_ATTRIBUTE_DESERIALIZE)
        {
            NDIS_DbgPrint(MAX_TRACE, ("Calling miniport's SendPackets handler\n"));
            (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendPacketsHandler)(
             Adapter->NdisMiniportBlock.MiniportAdapterContext, &Packet, 1);
             NdisStatus = NDIS_STATUS_PENDING;
        } else {
            /* SendPackets is called at DISPATCH_LEVEL for all serialized miniports */
            KeRaiseIrql(DISPATCH_LEVEL, &RaiseOldIrql);
            {
               NDIS_DbgPrint(MAX_TRACE, ("Calling miniport's SendPackets handler\n"));
               (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendPacketsHandler)(
                Adapter->NdisMiniportBlock.MiniportAdapterContext, &Packet, 1);
            }
            KeLowerIrql(RaiseOldIrql);

            NdisStatus = NDIS_GET_PACKET_STATUS(Packet);
            if (NdisStatus == NDIS_STATUS_RESOURCES) {
                MiniQueueWorkItem(Adapter, NdisWorkItemSend, Packet, TRUE);
                NdisStatus = NDIS_STATUS_PENDING;
            }
        }

        return NdisStatus;
   } else {
        if(Adapter->NdisMiniportBlock.Flags & NDIS_ATTRIBUTE_DESERIALIZE)
        {
            NDIS_DbgPrint(MAX_TRACE, ("Calling miniport's Send handler\n"));
            NdisStatus = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendHandler)(
                          Adapter->NdisMiniportBlock.MiniportAdapterContext, Packet, Packet->Private.Flags);
            NDIS_DbgPrint(MAX_TRACE, ("back from miniport's send handler\n"));
        } else {
            /* Send is called at DISPATCH_LEVEL for all serialized miniports */
            KeRaiseIrql(DISPATCH_LEVEL, &RaiseOldIrql);
            NDIS_DbgPrint(MAX_TRACE, ("Calling miniport's Send handler\n"));
            NdisStatus = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendHandler)(
                          Adapter->NdisMiniportBlock.MiniportAdapterContext, Packet, Packet->Private.Flags);
            NDIS_DbgPrint(MAX_TRACE, ("back from miniport's send handler\n"));
            KeLowerIrql(RaiseOldIrql);

            if (NdisStatus == NDIS_STATUS_RESOURCES) {
                MiniQueueWorkItem(Adapter, NdisWorkItemSend, Packet, TRUE);
                NdisStatus = NDIS_STATUS_PENDING;
            }
        }

        return NdisStatus;
   }
#endif
}


NDIS_STATUS NTAPI
ProSend(
    IN  NDIS_HANDLE     MacBindingHandle,
    IN  PNDIS_PACKET    Packet)
/*
 * FUNCTION: Forwards a request to send a packet to an NDIS miniport
 * ARGUMENTS:
 *     MacBindingHandle = Adapter binding handle
 *     Packet           = Pointer to NDIS packet descriptor
 * RETURNS:
 *     NDIS_STATUS_SUCCESS if the packet was successfully sent
 *     NDIS_STATUS_PENDING if the miniport was busy or a serialized miniport returned NDIS_STATUS_RESOURCES
 */
{
  PADAPTER_BINDING AdapterBinding;
  PLOGICAL_ADAPTER Adapter;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  ASSERT(MacBindingHandle);
  AdapterBinding = GET_ADAPTER_BINDING(MacBindingHandle);

  ASSERT(AdapterBinding);
  Adapter = AdapterBinding->Adapter;

  ASSERT(Adapter);

  /* if the following is not true, KeRaiseIrql() below will break */
  ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

  /* XXX what is this crazy black magic? */
  Packet->Reserved[0] = (ULONG_PTR)MacBindingHandle;

  /*
   * Test the packet to see if it is a MAC loopback.
   *
   * We may have to loop this packet if miniport cannot.
   * If dest MAC address of packet == MAC address of adapter,
   * this is a loopback frame.
   */

  if ((Adapter->NdisMiniportBlock.MacOptions & NDIS_MAC_OPTION_NO_LOOPBACK) &&
      MiniAdapterHasAddress(Adapter, Packet))
    {
#if WORKER_TEST
        MiniQueueWorkItem(Adapter, NdisWorkItemSendLoopback, Packet, FALSE);
        return NDIS_STATUS_PENDING;
#else
        return ProIndicatePacket(Adapter, Packet);
#endif
    } else {
        return proSendPacketToMiniport(Adapter, Packet);
    }
}


VOID NTAPI
ProSendPackets(
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets)
{
    PADAPTER_BINDING AdapterBinding = NdisBindingHandle;
    PLOGICAL_ADAPTER Adapter = AdapterBinding->Adapter;
    KIRQL RaiseOldIrql;
    NDIS_STATUS NdisStatus;
    UINT i;

    if(Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendPacketsHandler)
    {
       if(Adapter->NdisMiniportBlock.Flags & NDIS_ATTRIBUTE_DESERIALIZE)
       {
          (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendPacketsHandler)(
           Adapter->NdisMiniportBlock.MiniportAdapterContext, PacketArray, NumberOfPackets);
       }
       else
       {
          /* SendPackets is called at DISPATCH_LEVEL for all serialized miniports */
          KeRaiseIrql(DISPATCH_LEVEL, &RaiseOldIrql);
          (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendPacketsHandler)(
           Adapter->NdisMiniportBlock.MiniportAdapterContext, PacketArray, NumberOfPackets);
          KeLowerIrql(RaiseOldIrql);
          for (i = 0; i < NumberOfPackets; i++)
          {
             NdisStatus = NDIS_GET_PACKET_STATUS(PacketArray[i]);
             if (NdisStatus != NDIS_STATUS_PENDING)
                 MiniSendComplete(Adapter, PacketArray[i], NdisStatus);
          }
       }
     }
     else
     {
       if(Adapter->NdisMiniportBlock.Flags & NDIS_ATTRIBUTE_DESERIALIZE)
       {  
          for (i = 0; i < NumberOfPackets; i++)
          {
             NdisStatus = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendHandler)(
                           Adapter->NdisMiniportBlock.MiniportAdapterContext, PacketArray[i], PacketArray[i]->Private.Flags);
             if (NdisStatus != NDIS_STATUS_PENDING)
                 MiniSendComplete(Adapter, PacketArray[i], NdisStatus);
          }
       }
       else
       {
         /* Send is called at DISPATCH_LEVEL for all serialized miniports */
         KeRaiseIrql(DISPATCH_LEVEL, &RaiseOldIrql);
         for (i = 0; i < NumberOfPackets; i++)
         {
            NdisStatus = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendHandler)(
                           Adapter->NdisMiniportBlock.MiniportAdapterContext, PacketArray[i], PacketArray[i]->Private.Flags);
            if (NdisStatus != NDIS_STATUS_PENDING)
                MiniSendComplete(Adapter, PacketArray[i], NdisStatus);
         }
         KeLowerIrql(RaiseOldIrql);
       }
     }
}


NDIS_STATUS NTAPI
ProTransferData(
    IN  NDIS_HANDLE         MacBindingHandle,
    IN  NDIS_HANDLE         MacReceiveContext,
    IN  UINT                ByteOffset,
    IN  UINT                BytesToTransfer,
    IN  OUT PNDIS_PACKET    Packet,
    OUT PUINT               BytesTransferred)
/*
 * FUNCTION: Forwards a request to copy received data into a protocol-supplied packet
 * ARGUMENTS:
 *     MacBindingHandle  = Adapter binding handle
 *     MacReceiveContext = MAC receive context
 *     ByteOffset        = Offset in packet to place data
 *     BytesToTransfer   = Number of bytes to copy into packet
 *     Packet            = Pointer to NDIS packet descriptor
 *     BytesTransferred  = Address of buffer to place number of bytes copied
 */
{
    PADAPTER_BINDING AdapterBinding = GET_ADAPTER_BINDING(MacBindingHandle);
    PLOGICAL_ADAPTER Adapter        = AdapterBinding->Adapter;
    NDIS_STATUS Status;
    KIRQL OldIrql;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* FIXME: Interrupts must be disabled for adapter */
    /* XXX sd - why is that true? */

    if (Packet == Adapter->NdisMiniportBlock.IndicatedPacket[KeGetCurrentProcessorNumber()]) {
	NDIS_DbgPrint(MAX_TRACE, ("LoopPacket\n"));
        /* NDIS is responsible for looping this packet */
        NdisCopyFromPacketToPacket(Packet,
                                   ByteOffset,
                                   BytesToTransfer,
                                   Adapter->NdisMiniportBlock.IndicatedPacket[KeGetCurrentProcessorNumber()],
                                   0,
                                   BytesTransferred);
        return NDIS_STATUS_SUCCESS;
    }

    ASSERT(Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.TransferDataHandler);

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    Status = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.TransferDataHandler)(
        Packet,
        BytesTransferred,
        Adapter->NdisMiniportBlock.MiniportAdapterContext,
        MacReceiveContext,
        ByteOffset,
        BytesToTransfer);

    KeLowerIrql(OldIrql);

    return Status;
}



/*
 * @implemented
 */
VOID
EXPORT
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

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* Remove from protocol's bound adapters list */
    ExInterlockedRemoveEntryList(&AdapterBinding->ProtocolListEntry, &AdapterBinding->ProtocolBinding->Lock);

    /* Remove protocol from adapter's bound protocols list */
    ExInterlockedRemoveEntryList(&AdapterBinding->AdapterListEntry, &AdapterBinding->Adapter->NdisMiniportBlock.Lock);

    ExFreePool(AdapterBinding);

    *Status = NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
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
    PPROTOCOL_BINDING Protocol = GET_PROTOCOL_BINDING(NdisProtocolHandle);

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* FIXME: Make sure no adapter bindings exist */

    /* Remove protocol from global list */
    ExInterlockedRemoveEntryList(&Protocol->ListEntry, &ProtocolListLock);

    ExFreePool(Protocol);

    *Status = NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
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
  UINT i;
  BOOLEAN Found;
  PLOGICAL_ADAPTER Adapter;
  PADAPTER_BINDING AdapterBinding;
  PPROTOCOL_BINDING Protocol = GET_PROTOCOL_BINDING(NdisProtocolHandle);

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  if(!NdisProtocolHandle)
    {
      NDIS_DbgPrint(MAX_TRACE, ("NdisProtocolHandle is NULL\n"));
      *OpenErrorStatus = *Status = NDIS_STATUS_FAILURE;
      return;
    }

  Adapter = MiniLocateDevice(AdapterName);
  if (!Adapter)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Adapter not found.\n"));
      *Status = NDIS_STATUS_ADAPTER_NOT_FOUND;
      return;
    }

  /* Find the media type in the list provided by the protocol driver */
  Found = FALSE;
  for (i = 0; i < MediumArraySize; i++)
    {
      if (Adapter->NdisMiniportBlock.MediaType == MediumArray[i])
        {
          *SelectedMediumIndex = i;
          Found = TRUE;
          break;
        }
    }

  if (!Found)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Medium is not supported.\n"));
      *Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
      return;
    }

  /* Now that we have confirmed that the adapter can be opened, create a binding */

  AdapterBinding = ExAllocatePool(NonPagedPool, sizeof(ADAPTER_BINDING));
  if (!AdapterBinding)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
      *Status = NDIS_STATUS_RESOURCES;
      return;
    }

  RtlZeroMemory(AdapterBinding, sizeof(ADAPTER_BINDING));

  AdapterBinding->ProtocolBinding        = Protocol;
  AdapterBinding->Adapter                = Adapter;
  AdapterBinding->NdisOpenBlock.ProtocolBindingContext = ProtocolBindingContext;

  /* Set fields required by some NDIS macros */
  AdapterBinding->NdisOpenBlock.BindingHandle = (NDIS_HANDLE)AdapterBinding;

  /* Set handlers (some NDIS macros require these) */

  AdapterBinding->NdisOpenBlock.RequestHandler      = ProRequest;
  AdapterBinding->NdisOpenBlock.ResetHandler        = ProReset;
  AdapterBinding->NdisOpenBlock.SendHandler         = ProSend;
  AdapterBinding->NdisOpenBlock.SendPacketsHandler  = ProSendPackets;
  AdapterBinding->NdisOpenBlock.TransferDataHandler = ProTransferData;

  AdapterBinding->NdisOpenBlock.RequestCompleteHandler =
    Protocol->Chars.RequestCompleteHandler;

  /* Put on protocol's bound adapters list */
  ExInterlockedInsertTailList(&Protocol->AdapterListHead, &AdapterBinding->ProtocolListEntry, &Protocol->Lock);

  /* Put protocol on adapter's bound protocols list */
  NDIS_DbgPrint(MAX_TRACE, ("acquiring miniport block lock\n"));
  ExInterlockedInsertTailList(&Adapter->ProtocolListHead, &AdapterBinding->AdapterListEntry, &Adapter->NdisMiniportBlock.Lock);

  *NdisBindingHandle = (NDIS_HANDLE)AdapterBinding;

  *Status = NDIS_STATUS_SUCCESS;
}

VOID
NTAPI
ndisBindMiniportsToProtocol(OUT PNDIS_STATUS Status, IN PNDIS_PROTOCOL_CHARACTERISTICS ProtocolCharacteristics)
{
  /*
   * bind the protocol to all of its miniports
   *
   * open registry path
   * get list of devices from Bind key
   * call BindAdapterHandler for each
   */
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING RegistryPath;
    WCHAR *RegistryPathStr;
    NTSTATUS NtStatus;
    WCHAR *DataPtr;
    HANDLE DriverKeyHandle = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION KeyInformation = NULL;

    RegistryPathStr = ExAllocatePoolWithTag(PagedPool, sizeof(SERVICES_KEY) + ProtocolCharacteristics->Name.Length + sizeof(LINKAGE_KEY), NDIS_TAG + __LINE__);
    if(!RegistryPathStr)
      {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        *Status = NDIS_STATUS_RESOURCES;
        return;
      }

    wcscpy(RegistryPathStr, SERVICES_KEY);
    wcsncat(RegistryPathStr, ((WCHAR *)ProtocolCharacteristics->Name.Buffer), ProtocolCharacteristics->Name.Length / sizeof(WCHAR));
    RegistryPathStr[wcslen(SERVICES_KEY)+ProtocolCharacteristics->Name.Length/sizeof(WCHAR)] = 0;
    wcscat(RegistryPathStr, LINKAGE_KEY);

    RtlInitUnicodeString(&RegistryPath, RegistryPathStr);
    NDIS_DbgPrint(MAX_TRACE, ("Opening configuration key: %wZ\n", &RegistryPath));

    InitializeObjectAttributes(&ObjectAttributes, &RegistryPath, OBJ_CASE_INSENSITIVE, NULL, NULL);
    NtStatus = ZwOpenKey(&DriverKeyHandle, KEY_READ, &ObjectAttributes);

    ExFreePool(RegistryPathStr);

    if(!NT_SUCCESS(NtStatus))
      {
        NDIS_DbgPrint(MIN_TRACE, ("Unable to open protocol configuration\n"));
        *Status = NDIS_STATUS_FAILURE;
        return;
      }

  NDIS_DbgPrint(MAX_TRACE, ("Successfully opened the registry configuration\n"));

  {
    UNICODE_STRING ValueName;
    ULONG ResultLength;

    RtlInitUnicodeString(&ValueName, L"Bind");

    NtStatus = ZwQueryValueKey(DriverKeyHandle, &ValueName, KeyValuePartialInformation, NULL, 0, &ResultLength);
    if(NtStatus != STATUS_BUFFER_OVERFLOW && NtStatus != STATUS_BUFFER_TOO_SMALL && NtStatus != STATUS_SUCCESS)
      {
        NDIS_DbgPrint(MIN_TRACE, ("Unable to query the Bind value for size\n"));
        ZwClose(DriverKeyHandle);
        *Status = NDIS_STATUS_FAILURE;
        return;
      }

    KeyInformation = ExAllocatePoolWithTag(PagedPool, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + ResultLength, NDIS_TAG + __LINE__);
    if(!KeyInformation)
      {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        ZwClose(DriverKeyHandle);
        *Status = NDIS_STATUS_FAILURE;
        return;
      }

    NtStatus = ZwQueryValueKey(DriverKeyHandle, &ValueName, KeyValuePartialInformation, KeyInformation,
        sizeof(KEY_VALUE_PARTIAL_INFORMATION) + ResultLength, &ResultLength);

    ZwClose(DriverKeyHandle);

    if(!NT_SUCCESS(NtStatus))
      {
        NDIS_DbgPrint(MIN_TRACE, ("Unable to query the Bind value\n"));
        ExFreePool(KeyInformation);
        *Status = NDIS_STATUS_FAILURE;
        return;
      }
  }

  for (DataPtr = (WCHAR *)KeyInformation->Data;
       *DataPtr != 0;
       DataPtr += wcslen(DataPtr) + 1)
    {
      /* BindContext is for tracking pending binding operations */
      VOID *BindContext = 0;
      NDIS_STRING DeviceName;
      NDIS_STRING RegistryPath;
      WCHAR *RegistryPathStr = NULL;
      ULONG PathLength = 0;

      RtlInitUnicodeString(&DeviceName, DataPtr);	/* we know this is 0-term */

      /*
       * RegistryPath should be:
       *     \Registry\Machine\System\CurrentControlSet\Services\Nic1\Parameters\Tcpip
       *
       *  This is constructed as follows:
       *      SERVICES_KEY + extracted device name + Protocol name from characteristics
       */

      PathLength = sizeof(SERVICES_KEY) +                               /* \Registry\Machine\System\CurrentControlSet\Services\ */
          wcslen( DataPtr + 8 ) * sizeof(WCHAR) + /* Adapter1  (extracted from \Device\Adapter1)          */
          sizeof(PARAMETERS_KEY) +                                      /* \Parameters\                                         */
          ProtocolCharacteristics->Name.Length + sizeof(WCHAR);                         /* Tcpip                                                */

      RegistryPathStr = ExAllocatePool(PagedPool, PathLength);
      if(!RegistryPathStr)
        {
          NDIS_DbgPrint(MIN_TRACE, ("insufficient resources.\n"));
          ExFreePool(KeyInformation);
          *Status = NDIS_STATUS_RESOURCES;
          return;
        }

      wcscpy(RegistryPathStr, SERVICES_KEY);
      wcscat(RegistryPathStr, DataPtr + 8 );
      wcscat(RegistryPathStr, PARAMETERS_KEY);
      wcsncat(RegistryPathStr, ProtocolCharacteristics->Name.Buffer, ProtocolCharacteristics->Name.Length / sizeof(WCHAR) );

      RegistryPathStr[PathLength/sizeof(WCHAR) - 1] = 0;

      RtlInitUnicodeString(&RegistryPath, RegistryPathStr);

      NDIS_DbgPrint(MAX_TRACE, ("Calling protocol's BindAdapter handler with DeviceName %wZ and RegistryPath %wZ\n",
          &DeviceName, &RegistryPath));

        {
          BIND_HANDLER BindHandler = ProtocolCharacteristics->BindAdapterHandler;
          if(BindHandler)
            BindHandler(Status, BindContext, &DeviceName, &RegistryPath, 0);
          else
            NDIS_DbgPrint(MID_TRACE, ("No protocol bind handler specified\n"));
        }
    }

   *Status = NDIS_STATUS_SUCCESS;
   ExFreePool(KeyInformation);
}

/*
 * @implemented
 */
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
 * NOTES:
 *     - you *must* set NdisProtocolHandle before doing anything that could wind up
 *       getting BindAdapterHandler, as it will probably call OpenAdapter with this handle
 *     - the above implies that the initialization of the protocol block must be complete
 *       by then
 * TODO:
 *     - break this function up - probably do a 'ndisRefreshProtocolBindings' function
 *     - make this thing able to handle >1 protocol
 */
{
  PPROTOCOL_BINDING Protocol;
  NTSTATUS NtStatus;
  UINT MinSize;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  *NdisProtocolHandle = NULL;

  /* first validate the PROTOCOL_CHARACTERISTICS */
  switch (ProtocolCharacteristics->MajorNdisVersion)
    {
    case 0x03:
      /* we don't really want to support ndis3 drivers - so we complain for now */
      NDIS_DbgPrint(MID_TRACE, ("NDIS 3 protocol attempting to register\n"));
      MinSize = sizeof(NDIS30_PROTOCOL_CHARACTERISTICS);
      break;

    case 0x04:
      MinSize = sizeof(NDIS40_PROTOCOL_CHARACTERISTICS);
      break;

    case 0x05:
      MinSize = sizeof(NDIS50_PROTOCOL_CHARACTERISTICS);
      break;

    default:
      *Status = NDIS_STATUS_BAD_VERSION;
      NDIS_DbgPrint(MIN_TRACE, ("Incorrect characteristics size\n"));
      return;
    }

  if (CharacteristicsLength < MinSize)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Bad protocol characteristics.\n"));
      *Status = NDIS_STATUS_BAD_CHARACTERISTICS;
      return;
    }

  /* set up the protocol block */
  Protocol = ExAllocatePool(NonPagedPool, sizeof(PROTOCOL_BINDING));
  if (!Protocol)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
      *Status = NDIS_STATUS_RESOURCES;
      return;
    }

  RtlZeroMemory(Protocol, sizeof(PROTOCOL_BINDING));
  RtlCopyMemory(&Protocol->Chars, ProtocolCharacteristics, MinSize);

  NtStatus = RtlUpcaseUnicodeString(&Protocol->Chars.Name, &ProtocolCharacteristics->Name, TRUE);
  if (!NT_SUCCESS(NtStatus))
    {
      NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
      ExFreePool(Protocol);
      *Status = NDIS_STATUS_RESOURCES;
      return;
    }

  KeInitializeSpinLock(&Protocol->Lock);

  InitializeListHead(&Protocol->AdapterListHead);

  /* We must set this before the call to ndisBindMiniportsToProtocol because the protocol's 
   * BindAdapter handler might need it */

  *NdisProtocolHandle = Protocol;

  ndisBindMiniportsToProtocol(Status, &Protocol->Chars);

  if (*Status == NDIS_STATUS_SUCCESS) {
      ExInterlockedInsertTailList(&ProtocolListHead, &Protocol->ListEntry, &ProtocolListLock);
  } else {
      ExFreePool(Protocol);
      *NdisProtocolHandle = NULL;
  }
}


/*
 * @implemented
 */
VOID
EXPORT
NdisRequest(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PNDIS_REQUEST   NdisRequest)
/*
 * FUNCTION: Forwards a request to an NDIS driver
 * ARGUMENTS:
 *     Status            = Address of buffer for status information
 *     NdisBindingHandle = Adapter binding handle
 *     NdisRequest       = Pointer to request to perform
 */
{
    *Status = ProRequest(NdisBindingHandle, NdisRequest);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisReset(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisBindingHandle)
{
    *Status = ProReset(NdisBindingHandle);
}


/*
 * @implemented
 */
#undef NdisSend
VOID
EXPORT
NdisSend(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PNDIS_PACKET    Packet)
/*
 * FUNCTION: Forwards a request to send a packet
 * ARGUMENTS:
 *     Status             = Address of buffer for status information
 *     NdisBindingHandle  = Adapter binding handle
 *     Packet             = Pointer to NDIS packet descriptor
 */
{
    *Status = ProSend(NdisBindingHandle, Packet);
}


/*
 * @implemented
 */
#undef NdisSendPackets
VOID
EXPORT
NdisSendPackets(
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets)
{
    ProSendPackets(NdisBindingHandle, PacketArray, NumberOfPackets);
}


/*
 * @implemented
 */
#undef NdisTransferData
VOID
EXPORT
NdisTransferData(
    OUT     PNDIS_STATUS    Status,
    IN      NDIS_HANDLE     NdisBindingHandle,
    IN      NDIS_HANDLE     MacReceiveContext,
    IN      UINT            ByteOffset,
    IN      UINT            BytesToTransfer,
    IN OUT	PNDIS_PACKET    Packet,
    OUT     PUINT           BytesTransferred)
/*
 * FUNCTION: Forwards a request to copy received data into a protocol-supplied packet
 * ARGUMENTS:
 *     Status            = Address of buffer for status information
 *     NdisBindingHandle = Adapter binding handle
 *     MacReceiveContext = MAC receive context
 *     ByteOffset        = Offset in packet to place data
 *     BytesToTransfer   = Number of bytes to copy into packet
 *     Packet            = Pointer to NDIS packet descriptor
 *     BytesTransferred  = Address of buffer to place number of bytes copied
 */
{
    *Status = ProTransferData(NdisBindingHandle,
                              MacReceiveContext,
                              ByteOffset,
                              BytesToTransfer,
                              Packet,
                              BytesTransferred);
}

/*
 * @implemented
 */
VOID
NTAPI
NdisReEnumerateProtocolBindings(IN NDIS_HANDLE NdisProtocolHandle)
{
    PPROTOCOL_BINDING Protocol = NdisProtocolHandle;
    NDIS_STATUS NdisStatus;

    ndisBindMiniportsToProtocol(&NdisStatus, &Protocol->Chars);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisGetDriverHandle(
    IN  PNDIS_HANDLE    NdisBindingHandle,
    OUT PNDIS_HANDLE    NdisDriverHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    PADAPTER_BINDING Binding = (PADAPTER_BINDING)NdisBindingHandle;

    if (!Binding)
    {
        *NdisDriverHandle = NULL;
        return;
    }

    *NdisDriverHandle = Binding->Adapter->NdisMiniportBlock.DriverHandle;
}

/* EOF */
