/*
 * PROJECT:         ReactOS Videoport
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/video/videoprt/dma.c
 * PURPOSE:         Videoport Direct Memory Access Support
 * PROGRAMMERS:     ...
 */

/* INCLUDES ******************************************************************/

#include <videoprt.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
PVOID
NTAPI
VideoPortAllocateCommonBuffer(IN PVOID HwDeviceExtension,
                              IN PVP_DMA_ADAPTER VpDmaAdapter,
                              IN ULONG DesiredLength,
                              OUT PPHYSICAL_ADDRESS LogicalAddress,
                              IN BOOLEAN CacheEnabled,
                              PVOID Reserved)
{
    /* Forward to HAL */
    return HalAllocateCommonBuffer((PADAPTER_OBJECT)VpDmaAdapter,
                                   DesiredLength,
                                   LogicalAddress,
                                   CacheEnabled);
}

/*
 * @implemented
 */
VOID
NTAPI
VideoPortReleaseCommonBuffer(IN PVOID HwDeviceExtension,
                             IN PVP_DMA_ADAPTER VpDmaAdapter,
                             IN ULONG Length,
                             IN PHYSICAL_ADDRESS LogicalAddress,
                             IN PVOID VirtualAddress,
                             IN BOOLEAN CacheEnabled)
{
    /* Forward to HAL */
    HalFreeCommonBuffer((PADAPTER_OBJECT)VpDmaAdapter,
                       Length,
                       LogicalAddress,
                       VirtualAddress,
                       CacheEnabled);
}

/*
 * @unimplemented
 */
VOID
NTAPI
VideoPortPutDmaAdapter(IN PVOID HwDeviceExtension,
                       IN PVP_DMA_ADAPTER VpDmaAdapter)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
PVP_DMA_ADAPTER
NTAPI
VideoPortGetDmaAdapter(IN PVOID HwDeviceExtension,
                       IN PVP_DEVICE_DESCRIPTION VpDeviceExtension)
{
    DEVICE_DESCRIPTION DeviceDescription;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
    ULONG NumberOfMapRegisters;

	/* Zero the structure */
    RtlZeroMemory(&DeviceDescription,
	              sizeof(DEVICE_DESCRIPTION));

    /* Initialize the structure */
    DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.Master = TRUE /* ?? */;
	DeviceDescription.DmaWidth = Width8Bits;
    DeviceDescription.DmaSpeed = Compatible;

	/* Copy data from caller's device extension */
    DeviceDescription.ScatterGather = VpDeviceExtension->ScatterGather;
	DeviceDescription.Dma32BitAddresses = VpDeviceExtension->Dma32BitAddresses;
	DeviceDescription.Dma64BitAddresses = VpDeviceExtension->Dma64BitAddresses;
	DeviceDescription.MaximumLength = VpDeviceExtension->MaximumLength;

    /* Copy data from the internal device extension */
    DeviceDescription.BusNumber = DeviceExtension->SystemIoBusNumber;
    DeviceDescription.InterfaceType = DeviceExtension->AdapterInterfaceType;

	return (PVP_DMA_ADAPTER)HalGetAdapter(&DeviceDescription,
	                                      &NumberOfMapRegisters);
}

/*
 * @implemented
 */
VOID
NTAPI
VideoPortFreeCommonBuffer(IN PVOID HwDeviceExtension,
                          IN ULONG Length,
                          IN PVOID VirtualAddress,
                          IN PHYSICAL_ADDRESS LogicalAddress,
                          IN BOOLEAN CacheEnabled)
{
    DEVICE_DESCRIPTION DeviceDescription;
    PVP_DMA_ADAPTER VpDmaAdapter;

    /* FIXME: Broken code*/
    VpDmaAdapter = VideoPortGetDmaAdapter(HwDeviceExtension,
                                          (PVP_DEVICE_DESCRIPTION)&DeviceDescription);
    HalFreeCommonBuffer((PADAPTER_OBJECT)VpDmaAdapter,
                        Length,
                        LogicalAddress,
                        VirtualAddress,
                        CacheEnabled);
}

/*
 * @unimplemented
 */
PVOID
NTAPI
VideoPortGetCommonBuffer(IN PVOID HwDeviceExtension,
                         IN ULONG DesiredLength,
                         IN ULONG Alignment,
                         OUT PPHYSICAL_ADDRESS LogicalAddress,
                         OUT PULONG pActualLength,
                         IN BOOLEAN CacheEnabled)
{
    UNIMPLEMENTED;
	return NULL;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
VideoPortUnmapDmaMemory(
    PVOID  HwDeviceExtension,
    PVOID  VirtualAddress,
    HANDLE  ProcessHandle,
    PDMA  BoardMemoryHandle)
{
    /* Deprecated */
	return FALSE;
}

/*
 * @implemented
 */
PDMA 
NTAPI
VideoPortMapDmaMemory(IN PVOID HwDeviceExtension,
                      IN PVIDEO_REQUEST_PACKET pVrp,
                      IN PHYSICAL_ADDRESS BoardAddress,
                      IN PULONG Length,
                      IN PULONG InIoSpace,
                      IN PVOID MappedUserEvent,
                      IN PVOID DisplayDriverEvent,
                      IN OUT PVOID *VirtualAddress)
{
    /* Deprecated */
	return NULL;
}

/*
 * @implemented
 */
VOID
NTAPI
VideoPortSetDmaContext(IN PVOID HwDeviceExtension,
                       OUT PDMA pDma,
                       IN PVOID InstanceContext)
{
    /* Deprecated */
	return;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
VideoPortSignalDmaComplete(IN PVOID HwDeviceExtension,
                           IN PDMA pDmaHandle)
{
    /* Deprecated */
	return FALSE;
}

/*
 * @unimplemented
 */
VP_STATUS
NTAPI
VideoPortStartDma(IN PVOID HwDeviceExtension,
                  IN PVP_DMA_ADAPTER VpDmaAdapter,
				  IN PVOID Mdl,
				  IN ULONG Offset,
				  IN OUT PULONG pLength,
				  IN PEXECUTE_DMA ExecuteDmaRoutine,
				  IN PVOID Context,
				  IN BOOLEAN WriteToDevice)
{
    UNIMPLEMENTED;

	/* Lie and return success */
	return NO_ERROR;
}

/*
 * @implemented
 */
PVOID
NTAPI
VideoPortGetDmaContext(IN PVOID HwDeviceExtension,
                       IN PDMA pDma)
{
    /* Deprecated */
	return NULL;
}

/*
 * @implemented
 */
PDMA
NTAPI
VideoPortDoDma(IN PVOID HwDeviceExtension,
               IN PDMA pDma,
               IN DMA_FLAGS DmaFlags)
{
    /* Deprecated */
	return NULL;
}

/*
 * @unimplemented
 */
PDMA
NTAPI
VideoPortAssociateEventsWithDmaHandle(IN PVOID HwDeviceExtension,
                                      IN OUT PVIDEO_REQUEST_PACKET pVrp,
                                      IN PVOID MappedUserEvent,
                                      IN PVOID DisplayDriverEvent)
{
    UNIMPLEMENTED;
	return NULL;
}

/*
 * @unimplemented
 */
VP_STATUS
NTAPI
VideoPortCompleteDma(IN PVOID HwDeviceExtension,
                     IN PVP_DMA_ADAPTER VpDmaAdapter,
                     IN PVP_SCATTER_GATHER_LIST VpScatterGather,
                     IN BOOLEAN WriteToDevice)
{
    UNIMPLEMENTED;

	/* Lie and return success */
	return NO_ERROR;
}
