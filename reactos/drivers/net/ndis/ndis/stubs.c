/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/stubs.c
 * PURPOSE:     Stubs
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <ndissys.h>


VOID
EXPORT
NdisCompleteBindAdapter(
    IN  NDIS_HANDLE BindAdapterContext,
    IN  NDIS_STATUS Status,
    IN  NDIS_STATUS OpenStatus)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisCompleteQueryStatistics(
    IN  NDIS_HANDLE     NdisAdapterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  NDIS_STATUS     Status)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisCompleteUnbindAdapter(
    IN  NDIS_HANDLE UnbindAdapterContext,
    IN  NDIS_STATUS Status)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisInitializeListHead(
    IN  PLIST_ENTRY ListHead)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisInterlockedAddUlong (
    IN  PULONG          Addend,
    IN  ULONG           Increment,
    IN  PNDIS_SPIN_LOCK SpinLock)
{
    UNIMPLEMENTED
}


PLIST_ENTRY
EXPORT
NdisInterlockedInsertHeadList(
    IN  PLIST_ENTRY     ListHead,
    IN  PLIST_ENTRY     ListEntry,
    IN  PNDIS_SPIN_LOCK SpinLock)
{
    UNIMPLEMENTED

	return NULL;
}


PLIST_ENTRY
EXPORT
NdisInterlockedInsertTailList(
    IN  PLIST_ENTRY     ListHead,
    IN  PLIST_ENTRY     ListEntry,
    IN  PNDIS_SPIN_LOCK SpinLock)
{
    UNIMPLEMENTED

	return NULL;
}


PLIST_ENTRY
EXPORT
NdisInterlockedRemoveHeadList(
    IN  PLIST_ENTRY     ListHead,
    IN  PNDIS_SPIN_LOCK SpinLock)
{
    UNIMPLEMENTED

	return NULL;
}


VOID
EXPORT
NdisMArcIndicateReceive(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  PUCHAR      HeaderBuffer,
    IN  PUCHAR      DataBuffer,
    IN  UINT        Length)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMArcIndicateReceiveComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMCompleteBufferPhysicalMapping(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_BUFFER    Buffer,
    IN  ULONG           PhysicalMapRegister)
{
}


VOID
EXPORT
NdisMEthIndicateReceive (
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_HANDLE MiniportReceiveContext,
    IN  PVOID       HeaderBuffer,
    IN  UINT        HeaderBufferSize,
    IN  PVOID       LookaheadBuffer,
    IN  UINT        LookaheadBufferSize,
    IN  UINT        PacketSize)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMEthIndicateReceiveComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMFddiIndicateReceive(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_HANDLE MiniportReceiveContext,
    IN  PVOID       HeaderBuffer,
    IN  UINT        HeaderBufferSize,
    IN  PVOID       LookaheadBuffer,
    IN  UINT        LookaheadBufferSize,
    IN  UINT        PacketSize)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMFddiIndicateReceiveComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMStartBufferPhysicalMapping(
    IN  NDIS_HANDLE                 MiniportAdapterHandle,
    IN  PNDIS_BUFFER                Buffer,
    IN  ULONG                       PhysicalMapRegister,
    IN  BOOLEAN                     WriteToDevice,
    OUT PNDIS_PHYSICAL_ADDRESS_UNIT	PhysicalAddressArray,
    OUT PUINT                       ArraySize)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMTrIndicateReceive(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_HANDLE MiniportReceiveContext,
    IN  PVOID       HeaderBuffer,
    IN  UINT        HeaderBufferSize,
    IN  PVOID       LookaheadBuffer,
    IN  UINT        LookaheadBufferSize,
    IN  UINT        PacketSize)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMTrIndicateReceiveComplete(
    IN  NDIS_HANDLE  MiniportAdapterHandle)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMapFile(
    OUT PNDIS_STATUS    Status,
    OUT PVOID           * MappedBuffer,
    IN  NDIS_HANDLE     FileHandle)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisWriteConfiguration(
    OUT PNDIS_STATUS                    Status,
    IN  NDIS_HANDLE                     WrapperConfigurationContext,
    IN  PNDIS_STRING                    Keyword,
    IN  PNDIS_CONFIGURATION_PARAMETER   * ParameterValue)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisWriteErrorLogEntry(
    IN  NDIS_HANDLE     NdisAdapterHandle,
    IN  NDIS_ERROR_CODE ErrorCode,
    IN  ULONG           NumberOfErrorValues,
 /* IN  ULONG */        ...)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisCloseConfiguration(
    IN  NDIS_HANDLE ConfigurationHandle)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisCloseFile(
    IN  NDIS_HANDLE FileHandle)
{
    UNIMPLEMENTED
}


NDIS_STATUS
EXPORT
NdisIMDeInitializeDeviceInstance(
    IN  NDIS_HANDLE NdisMiniportHandle)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


NDIS_STATUS
EXPORT
NdisIMInitializeDeviceInstance(
    IN  NDIS_HANDLE     DriverHandle,
    IN  PNDIS_STRING    DeviceInstance)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


NDIS_STATUS
EXPORT
NdisIMRegisterLayeredMiniport(
    IN  NDIS_HANDLE                     NdisWrapperHandle,
    IN  PNDIS_MINIPORT_CHARACTERISTICS  MiniportCharacteristics,
    IN  UINT                            CharacteristicsLength,
    OUT PNDIS_HANDLE                    DriverHandle)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


VOID
EXPORT
NdisMWanIndicateReceive(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  NDIS_HANDLE     NdisLinkContext,
    IN  PUCHAR          PacketBuffer,
    IN  UINT            PacketSize)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMWanIndicateReceiveComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMWanSendComplete(
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  PNDIS_WAN_PACKET    Packet,
    IN  NDIS_STATUS         Status)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisOpenConfiguration(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_HANDLE    ConfigurationHandle,
    IN  NDIS_HANDLE     WrapperConfigurationContext)
{
    UNIMPLEMENTED
}


/*
NdisOpenConfigurationKeyByIndex
NdisOpenConfigurationKeyByName
*/


VOID
EXPORT
NdisOpenFile(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_HANDLE            FileHandle,
    OUT PUINT                   FileLength,
    IN  PNDIS_STRING            FileName,
    IN  NDIS_PHYSICAL_ADDRESS   HighestAcceptableAddress)
{
    UNIMPLEMENTED
}


/*
NdisOpenGlobalConfiguration
*/

VOID
EXPORT
NdisOpenProtocolConfiguration(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_HANDLE    ConfigurationHandle,
    IN  PNDIS_STRING    ProtocolSection)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisReadConfiguration(
    OUT PNDIS_STATUS                    Status,
    OUT PNDIS_CONFIGURATION_PARAMETER   * ParameterValue,
    IN  NDIS_HANDLE                     ConfigurationHandle,
    IN  PNDIS_STRING                    Keyword,
    IN  NDIS_PARAMETER_TYPE             ParameterType)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisReadNetworkAddress(
    OUT PNDIS_STATUS    Status,
    OUT PVOID           * NetworkAddress,
    OUT PUINT           NetworkAddressLength,
    IN  NDIS_HANDLE     ConfigurationHandle)
{
    UNIMPLEMENTED
}


#if 0
VOID
EXPORT
NdisRegisterTdiCallBack(
    IN  TDI_REGISTER_CALLBACK   RegsterCallback)
{
    UNIMPLEMENTED
}
#endif


/*
NdisScheduleWorkItem
*/


#if 0
VOID
EXPORT
NdisSetProtocolFilter(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  RECEIVE_HANDLER         ReceiveHandler,
    IN  RECEIVE_PACKET_HANDLER  ReceivePacketHandler,
    IN  NDIS_MEDIUM             Medium,
    IN  UINT                    Offset,
    IN  UINT                    Size,
    IN  PUCHAR                  Pattern)
{
    UNIMPLEMENTED
}
#endif


CCHAR
EXPORT
NdisSystemProcessorCount(
    VOID)
{
	return (CCHAR)1;
}


VOID
EXPORT
NdisUnmapFile(
    IN  NDIS_HANDLE FileHandle)
{
    UNIMPLEMENTED
}


/*
NdisUpcaseUnicodeString
NdisUpdateSharedMemory@4
*/


/*
NdisWriteEventLogEntry
*/



/* NDIS 5.0 extensions */

VOID
EXPORT
NdisCompletePnPEvent(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PNET_PNP_EVENT  NetPnPEvent)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisConvertStringToAtmAddress(
    OUT PNDIS_STATUS    Status,
    IN  PNDIS_STRING    String,
    OUT PATM_ADDRESS    AtmAddress)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisGetCurrentProcessorCounts(
    OUT PULONG  pIdleCount,
    OUT PULONG  pKernelAndUser,
    OUT PULONG  pIndex)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


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
    UNIMPLEMENTED
}


PNDIS_PACKET
EXPORT
NdisGetReceivedPacket(
    IN  PNDIS_HANDLE    NdisBindingHandle,
    IN  PNDIS_HANDLE    MacContext)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NULL;
}


VOID
EXPORT
NdisGetSystemUptime(
    OUT PULONG  pSystemUpTime)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisInitializeReadWriteLock(
    IN  PNDIS_RW_LOCK   Lock)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


LONG
EXPORT
NdisInterlockedDecrement(
    IN  PLONG   Addend)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return 0;
}


LONG
EXPORT
NdisInterlockedIncrement(
    IN  PLONG   Addend)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return 0;
}


PSINGLE_LIST_ENTRY 
EXPORT
NdisInterlockedPopEntrySList(
    IN  PSLIST_HEADER   ListHead,
    IN  PKSPIN_LOCK     Lock)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NULL;
}


PSINGLE_LIST_ENTRY 
EXPORT
NdisInterlockedPushEntrySList(
    IN  PSLIST_HEADER       ListHead,
    IN  PSINGLE_LIST_ENTRY  ListEntry,
    IN  PKSPIN_LOCK         Lock)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NULL;
}


NDIS_STATUS
EXPORT
NdisMDeregisterDevice(
    IN  NDIS_HANDLE NdisDeviceHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


VOID
EXPORT
NdisMGetDeviceProperty(
    IN      NDIS_HANDLE         MiniportAdapterHandle,
    IN OUT  PDEVICE_OBJECT      *PhysicalDeviceObject           OPTIONAL,
    IN OUT  PDEVICE_OBJECT      *FunctionalDeviceObject         OPTIONAL,
    IN OUT  PDEVICE_OBJECT      *NextDeviceObject               OPTIONAL,
    IN OUT  PCM_RESOURCE_LIST   *AllocatedResources             OPTIONAL,
    IN OUT  PCM_RESOURCE_LIST   *AllocatedResourcesTranslated   OPTIONAL)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


NDIS_STATUS
EXPORT
NdisMInitializeScatterGatherDma(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  BOOLEAN     Dma64BitAddresses,
    IN  ULONG       MaximumPhysicalMapping)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


NDIS_STATUS
EXPORT
NdisMPromoteMiniport(
    IN  NDIS_HANDLE MiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


NDIS_STATUS
EXPORT
NdisMQueryAdapterInstanceName(
    OUT PNDIS_STRING    AdapterInstanceName,
    IN  NDIS_HANDLE     MiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


NDIS_STATUS
EXPORT
NdisMRegisterDevice(
    IN  NDIS_HANDLE         NdisWrapperHandle,
    IN  PNDIS_STRING        DeviceName,
    IN  PNDIS_STRING        SymbolicName,
    IN  PDRIVER_DISPATCH    MajorFunctions[],
    OUT PDEVICE_OBJECT      *pDeviceObject,
    OUT NDIS_HANDLE         *NdisDeviceHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


VOID
EXPORT
NdisMRegisterUnloadHandler(
    IN  NDIS_HANDLE     NdisWrapperHandle,
    IN  PDRIVER_UNLOAD  UnloadHandler)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


NDIS_STATUS
EXPORT
NdisMRemoveMiniport(
    IN  NDIS_HANDLE MiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


NDIS_STATUS
EXPORT
NdisMSetMiniportSecondary(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_HANDLE PrimaryMiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


VOID
EXPORT
NdisOpenConfigurationKeyByIndex(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     ConfigurationHandle,
    IN  ULONG           Index,
    OUT PNDIS_STRING    KeyName,
    OUT PNDIS_HANDLE    KeyHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisOpenConfigurationKeyByName(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     ConfigurationHandle,
    IN  PNDIS_STRING    SubKeyName,
    OUT PNDIS_HANDLE    SubKeyHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


NDIS_STATUS
EXPORT
NdisQueryAdapterInstanceName(
    OUT PNDIS_STRING    AdapterInstanceName,
    IN  NDIS_HANDLE     NdisBindingHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


VOID
EXPORT
NdisQueryBufferSafe(
    IN  PNDIS_BUFFER    Buffer,
    OUT PVOID           *VirtualAddress OPTIONAL,
    OUT PUINT           Length,
    IN  UINT            Priority)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


ULONG
EXPORT
NdisReadPcmciaAttributeMemory(
    IN  NDIS_HANDLE NdisAdapterHandle,
    IN  ULONG       Offset,
    IN  PVOID       Buffer,
    IN  ULONG       Length)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return 0;
}


VOID
EXPORT
NdisReleaseReadWriteLock(
    IN  PNDIS_RW_LOCK   Lock,
    IN  PLOCK_STATE     LockState)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


NDIS_STATUS
EXPORT
NdisWriteEventLogEntry(
    IN  PVOID       LogHandle,
    IN  NDIS_STATUS EventCode,
    IN  ULONG       UniqueEventValue,
    IN  USHORT      NumStrings,
    IN  PVOID       StringsList OPTIONAL,
    IN  ULONG       DataSize,
    IN  PVOID       Data        OPTIONAL)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


ULONG
EXPORT
NdisWritePcmciaAttributeMemory(
    IN  NDIS_HANDLE NdisAdapterHandle,
    IN  ULONG       Offset,
    IN  PVOID       Buffer,
    IN  ULONG       Length)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return 0;
}


/* NDIS 5.0 extensions for intermediate drivers */

VOID
EXPORT
NdisIMAssociateMiniport(
    IN  NDIS_HANDLE DriverHandle,
    IN  NDIS_HANDLE ProtocolHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


NDIS_STATUS 
EXPORT
NdisIMCancelInitializeDeviceInstance(
    IN  NDIS_HANDLE     DriverHandle,
    IN  PNDIS_STRING    DeviceInstance)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


VOID
EXPORT
NdisIMCopySendCompletePerPacketInfo(
    IN  PNDIS_PACKET    DstPacket,
    IN  PNDIS_PACKET    SrcPacket)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisIMCopySendPerPacketInfo(
    IN  PNDIS_PACKET    DstPacket,
    IN  PNDIS_PACKET    SrcPacket)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisIMDeregisterLayeredMiniport(
    IN  NDIS_HANDLE DriverHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


NDIS_HANDLE
EXPORT
NdisIMGetBindingContext(
    IN  NDIS_HANDLE NdisBindingHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return (NDIS_HANDLE)NULL;
}


NDIS_HANDLE
EXPORT
NdisIMGetDeviceContext(
    IN  NDIS_HANDLE MiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return (NDIS_HANDLE)NULL;
}


NDIS_STATUS
EXPORT
NdisIMInitializeDeviceInstanceEx(
    IN  NDIS_HANDLE     DriverHandle,
    IN  PNDIS_STRING    DriverInstance,
    IN  NDIS_HANDLE     DeviceContext   OPTIONAL)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}

/* EOF */
