/* $Id: dma.c,v 1.8 2003/10/23 09:03:51 vizzini Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/dma.c
 * PURPOSE:         DMA functions
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>
#include <hal.h>

/* XXX This initialization is out of date - ADAPTER_OBJECT has changed */
/* NOTE: The following initializations have to be kept in synch with ADAPTER_OBJECT in hal.h */
/* FIXME: we need the 16-bit dma channels */
ADAPTER_OBJECT IsaSlaveAdapterObjects[] = {
  { Isa, FALSE, 0, (PVOID)0x87, (PVOID)0x1, (PVOID)0x0, 0, NULL },
  { Isa, FALSE, 1, (PVOID)0x83, (PVOID)0x3, (PVOID)0x2, 0, NULL },
  { Isa, FALSE, 2, (PVOID)0x81, (PVOID)0x5, (PVOID)0x4, 0, NULL },
  { Isa, FALSE, 3, (PVOID)0x82, (PVOID)0x7, (PVOID)0x6, 0, NULL } };

ADAPTER_OBJECT PciBusMasterAdapterObjects[] = {
  { PCIBus, TRUE, 0, (PVOID)0, (PVOID)0, (PVOID)0x0, 0, NULL } };

/* Global flag to tell whether or not the adapter's device queue should be initialized (first call only) */
BOOLEAN AdaptersInitialized = FALSE;

/* FUNCTIONS *****************************************************************/

PVOID STDCALL
HalAllocateCommonBuffer (PADAPTER_OBJECT    AdapterObject,
			 ULONG              Length,
			 PPHYSICAL_ADDRESS  LogicalAddress,
			 BOOLEAN            CacheEnabled)
/*
 * FUNCTION: Allocates memory that is visible to both the processor(s) and
 * a dma device
 * ARGUMENTS: 
 *         AdapterObject = Adapter object representing the bus master or
 *                         system dma controller
 *         Length = Number of bytes to allocate
 *         LogicalAddress = Logical address the driver can use to access the
 *                          buffer 
 *         CacheEnabled = Specifies if the memory can be cached
 * RETURNS: The base virtual address of the memory allocated
 *          NULL on failure
 * NOTES:
 *      CacheEnabled is ignored - it's all cache-disabled (like in NT)
 */
{
  PHYSICAL_ADDRESS HighestAddress;
  PVOID BaseAddress;

  HighestAddress.u.HighPart = 0;
  if (AdapterObject->InterfaceType == Isa ||
      (AdapterObject->InterfaceType == MicroChannel && AdapterObject->Master == FALSE))
    {
      HighestAddress.u.LowPart = 0x00FFFFFF; /* 24Bit: 16MB address range */
    }
  else
    {
      HighestAddress.u.LowPart = 0xFFFFFFFF; /* 32Bit: 4GB address range */
    }

  BaseAddress = MmAllocateContiguousMemory(Length, HighestAddress);
  if (!BaseAddress)
    return 0;

  *LogicalAddress = MmGetPhysicalAddress(BaseAddress);

  return BaseAddress;
}

BOOLEAN STDCALL
HalFlushCommonBuffer (ULONG	Unknown1,
		      ULONG	Unknown2,
		      ULONG	Unknown3,
		      ULONG	Unknown4,
		      ULONG	Unknown5,
		      ULONG	Unknown6,
		      ULONG	Unknown7,
		      ULONG	Unknown8)
{
   return TRUE;
}

VOID STDCALL
HalFreeCommonBuffer (PADAPTER_OBJECT		AdapterObject,
		     ULONG			Length,
		     PHYSICAL_ADDRESS	LogicalAddress,
		     PVOID			VirtualAddress,
		     BOOLEAN			CacheEnabled)
{
   MmFreeContiguousMemory(VirtualAddress);
}

PADAPTER_OBJECT STDCALL
HalGetAdapter (PDEVICE_DESCRIPTION	DeviceDescription,
	       PULONG			NumberOfMapRegisters)
/*
 * FUNCTION: Returns a pointer to an adapter object for the DMA device 
 * defined in the device description structure
 * ARGUMENTS:
 *        DeviceDescription = Structure describing the attributes of the device
 *        NumberOfMapRegisters (OUT) = Returns the maximum number of map
 *                                     registers the device driver can
 *                                     allocate for DMA transfer operations
 * RETURNS: The allocated adapter object on success
 *          NULL on failure
 * TODO:
 *    Figure out what to do with the commented-out cases
 */
{
  /* TODO: find a better home for this */
  if(!AdaptersInitialized)
    {
      KeInitializeDeviceQueue(&PciBusMasterAdapterObjects[0].DeviceQueue);
      KeInitializeDeviceQueue(&IsaSlaveAdapterObjects[0].DeviceQueue);
      KeInitializeDeviceQueue(&IsaSlaveAdapterObjects[1].DeviceQueue);
      KeInitializeDeviceQueue(&IsaSlaveAdapterObjects[2].DeviceQueue);
      KeInitializeDeviceQueue(&IsaSlaveAdapterObjects[3].DeviceQueue);
      AdaptersInitialized = TRUE;
    }

  /* Validate parameters in device description, and return a pointer to
     the adapter object for the requested dma channel */
  if( DeviceDescription->Version != DEVICE_DESCRIPTION_VERSION )
    return NULL;

  if (DeviceDescription->InterfaceType == PCIBus)
    {
      if (DeviceDescription->Master == FALSE)
	return NULL;

      return &PciBusMasterAdapterObjects[0];
    }

  /*
  if( DeviceDescription->Master )
    return NULL;
  if( DeviceDescription->ScatterGather )
    return NULL;
  if( DeviceDescription->AutoInitialize )
    return NULL;
  if( DeviceDescription->Dma32BitAddresses )
    return NULL;
  if( DeviceDescription->InterfaceType != Isa )
     return NULL;
     */
  /*  if( DeviceDescription->DmaWidth != Width8Bits )
      return NULL;*/
  *NumberOfMapRegisters = 0x10;
  IsaSlaveAdapterObjects[DeviceDescription->DmaChannel].Buffer = 0;
  return &IsaSlaveAdapterObjects[DeviceDescription->DmaChannel];
}

ULONG STDCALL
HalReadDmaCounter (PADAPTER_OBJECT	AdapterObject)
{
   UNIMPLEMENTED;
}

/* EOF */
