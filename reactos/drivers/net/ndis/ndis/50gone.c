/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/50gone.c
 * PURPOSE:     Obsoleted functions in NDIS 5.0
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <ndissys.h>


VOID
EXPORT
NdisCompleteCloseAdapter(
    IN  NDIS_HANDLE NdisBindingContext,
    IN  NDIS_STATUS Status)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisCompleteOpenAdapter(
    IN  NDIS_HANDLE NdisBindingContext,
    IN  NDIS_STATUS Status,
    IN  NDIS_STATUS OpenErrorStatus)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}


NDIS_STATUS
EXPORT
NdisDeregisterAdapter(
    IN  NDIS_HANDLE NdisAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


VOID
EXPORT
NdisDeregisterAdapterShutdownHandler(
    IN  NDIS_HANDLE NdisAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisDeregisterMac(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisMacHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisFreeDmaChannel(
    IN  PNDIS_HANDLE    NdisDmaHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisFreeSharedMemory(
    IN NDIS_HANDLE              NdisAdapterHandle,
    IN ULONG                    Length,
    IN BOOLEAN                  Cached,
    IN PVOID                    VirtualAddress,
    IN NDIS_PHYSICAL_ADDRESS    PhysicalAddress)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}


NDIS_STATUS
EXPORT
NdisIMQueueMiniportCallback(
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  W_MINIPORT_CALLBACK CallbackRoutine,
    IN  PVOID               CallbackContext)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


VOID
EXPORT
NdisIMRevertBack(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_HANDLE SwitchHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}


BOOLEAN
EXPORT
NdisIMSwitchToMiniport(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    OUT PNDIS_HANDLE    SwitchHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED

	return FALSE;
}


VOID
EXPORT
NdisInitializeInterrupt(
    OUT     PNDIS_STATUS                Status,
    IN OUT  PNDIS_INTERRUPT             Interrupt,
    IN      NDIS_HANDLE                 NdisAdapterHandle,
    IN      PNDIS_INTERRUPT_SERVICE     InterruptServiceRoutine,
    IN      PVOID                       InterruptContext,
    IN      PNDIS_DEFERRED_PROCESSING   DeferredProcessingRoutine,
    IN      UINT                        InterruptVector,
    IN      UINT                        InterruptLevel,
    IN      BOOLEAN                     SharedInterrupt,
    IN      NDIS_INTERRUPT_MODE         InterruptMode)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMapIoSpace(
    OUT PNDIS_STATUS            Status,
    OUT PVOID                   *VirtualAddress,
    IN  NDIS_HANDLE             NdisAdapterHandle,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress,
    IN  UINT                    Length)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisOverrideBusNumber(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  NDIS_HANDLE MiniportAdapterHandle   OPTIONAL,
    IN  ULONG       BusNumber)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}


NDIS_STATUS
EXPORT
NdisQueryReceiveInformation(
    IN  NDIS_HANDLE NdisBindingHandle,
    IN  NDIS_HANDLE MacContext,
    OUT PLONGLONG   TimeSent        OPTIONAL,
    OUT PLONGLONG   TimeReceived    OPTIONAL,
    IN  PUCHAR      Buffer,
    IN  UINT        BufferSize,
    OUT PUINT       SizeNeeded)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


VOID
EXPORT
NdisReadMcaPosInformation(
    OUT PNDIS_STATUS        Status,
    IN  NDIS_HANDLE         WrapperConfigurationContext,
    IN  PUINT               ChannelNumber,
    OUT PNDIS_MCA_POS_DATA  McaData)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}


NDIS_STATUS
EXPORT
NdisRegisterAdapter(
    OUT PNDIS_HANDLE    NdisAdapterHandle,
    IN  NDIS_HANDLE     NdisMacHandle,
    IN  NDIS_HANDLE     MacAdapterContext,
    IN  NDIS_HANDLE     WrapperConfigurationContext,
    IN  PNDIS_STRING    AdapterName,
    IN  PVOID           AdapterInformation)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


VOID
EXPORT
NdisRegisterAdapterShutdownHandler(
    IN  NDIS_HANDLE                 NdisAdapterHandle,
    IN  PVOID                       ShutdownContext,
    IN  ADAPTER_SHUTDOWN_HANDLER    ShutdownHandler)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisRegisterMac(
    OUT PNDIS_STATUS                Status,
    OUT PNDIS_HANDLE                NdisMacHandle,
    IN  NDIS_HANDLE                 NdisWrapperHandle,
    IN  NDIS_HANDLE                 MacMacContext,
    IN  PNDIS_MAC_CHARACTERISTICS   MacCharacteristics,
    IN  UINT                        CharacteristicsLength)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisReleaseAdapterResources(
    IN  NDIS_HANDLE NdisAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisRemoveInterrupt(
    IN  PNDIS_INTERRUPT Interrupt)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisSetupDmaTransfer(
    OUT PNDIS_STATUS    Status,
    IN  PNDIS_HANDLE    NdisDmaHandle,
    IN  PNDIS_BUFFER    Buffer,
    IN  ULONG           Offset,
    IN  ULONG           Length,
    IN  BOOLEAN         WriteToDevice)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisUpdateSharedMemory(
    IN  NDIS_HANDLE             NdisAdapterHandle,
    IN  ULONG                   Length,
    IN  PVOID                   VirtualAddress,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    UNIMPLEMENTED
}

/* EOF */
