/* $Id: dma.c,v 1.9 2004/07/22 18:49:18 navaraf Exp $
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
ADAPTER_OBJECT IsaSlaveAdapterObjects[] = {
  { Isa, FALSE, 0, (PVOID)0x87, (PVOID)0x1, (PVOID)0x0, 0, NULL },
  { Isa, FALSE, 1, (PVOID)0x83, (PVOID)0x3, (PVOID)0x2, 0, NULL },
  { Isa, FALSE, 2, (PVOID)0x81, (PVOID)0x5, (PVOID)0x4, 0, NULL },
  { Isa, FALSE, 3, (PVOID)0x82, (PVOID)0x7, (PVOID)0x6, 0, NULL },
  /* 16-bit DMA */
  { Isa, FALSE, 4, (PVOID)0x8F, (PVOID)0xC2, (PVOID)0xC0, 0, NULL },
  { Isa, FALSE, 5, (PVOID)0x8B, (PVOID)0xC6, (PVOID)0xC4, 0, NULL },
  { Isa, FALSE, 6, (PVOID)0x89, (PVOID)0xCA, (PVOID)0xC8, 0, NULL },
  { Isa, FALSE, 7, (PVOID)0x8A, (PVOID)0xCE, (PVOID)0xCC, 0, NULL } };

ADAPTER_OBJECT PciBusMasterAdapterObjects[] = {
  { PCIBus, TRUE, 0, (PVOID)0, (PVOID)0, (PVOID)0x0, 0, NULL } };

/* FUNCTIONS *****************************************************************/

VOID
HalpInitDma (VOID)
{
  ULONG Index;

  KeInitializeDeviceQueue(&PciBusMasterAdapterObjects[0].DeviceQueue);
  KeInitializeSpinLock(&PciBusMasterAdapterObjects[0].SpinLock);
  PciBusMasterAdapterObjects[0].Inuse = FALSE;
  for (Index = 0; Index < 8; Index++)
    {
      KeInitializeDeviceQueue(&IsaSlaveAdapterObjects[Index].DeviceQueue);
      KeInitializeSpinLock(&IsaSlaveAdapterObjects[Index].SpinLock);
      IsaSlaveAdapterObjects[Index].Inuse = FALSE;
    }
}

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
 *      UPDATE: It's not ignored now. If that's wrong just modify the
 *      CacheEnabled comparsion below. 
 */
{
  PHYSICAL_ADDRESS LowestAddress, HighestAddress, BoundryAddressMultiple;
  PVOID BaseAddress;

  LowestAddress.QuadPart = 0;
  BoundryAddressMultiple.QuadPart = 0;
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

  BaseAddress = MmAllocateContiguousAlignedMemory(
      Length,
      LowestAddress,
      HighestAddress,
      BoundryAddressMultiple,
      CacheEnabled ? MmCached : MmNonCached,
      0x10000 );
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
 *        Honour all the fields in DeviceDescription structure.
 */
{
  PADAPTER_OBJECT AdapterObject;

  /* Validate parameters in device description, and return a pointer to
     the adapter object for the requested dma channel */
  if( DeviceDescription->Version != DEVICE_DESCRIPTION_VERSION )
    return NULL;

  switch (DeviceDescription->InterfaceType)
    {
      case PCIBus:
        if (DeviceDescription->Master == FALSE)
          return NULL;
        return &PciBusMasterAdapterObjects[0];

      case Isa:
        /* There are only 8 DMA channels on ISA. */
        if (DeviceDescription->DmaChannel >= 8)
          return NULL;
        /* Channels 1-4 are for 8-bit transfers... */
        if (DeviceDescription->DmaWidth != Width8Bits &&
            DeviceDescription->DmaChannel < 4)
          return NULL;
        /* ...and the rest is for 16-bit transfers. */
        if (DeviceDescription->DmaWidth != Width16Bits &&
            DeviceDescription->DmaChannel >= 4)
          return NULL;
        AdapterObject = &IsaSlaveAdapterObjects[DeviceDescription->DmaChannel];
        AdapterObject->Master = DeviceDescription->Master;
        AdapterObject->ScatterGather = DeviceDescription->ScatterGather;
        AdapterObject->AutoInitialize = DeviceDescription->AutoInitialize;
        AdapterObject->DemandMode = DeviceDescription->DemandMode;
        AdapterObject->Buffer = 0;
        /* FIXME: Is this correct? */
        *NumberOfMapRegisters = 16;
        return AdapterObject;

      default:
        /* Unsupported bus. */
        return NULL;
    }
}

ULONG STDCALL
HalReadDmaCounter (PADAPTER_OBJECT	AdapterObject)
{
  KIRQL OldIrql;
  ULONG Count;

  if (AdapterObject && AdapterObject->InterfaceType == Isa && !AdapterObject->Master)
  {
    KeAcquireSpinLock(&AdapterObject->SpinLock, &OldIrql);

    /* Clear the flip/flop register */
    WRITE_PORT_UCHAR( AdapterObject->Channel < 4 ? (PVOID)0x0C : (PVOID)0xD8, 0 );
    /* Read the offset */
    Count = READ_PORT_UCHAR( AdapterObject->CountPort );
    Count |= READ_PORT_UCHAR( AdapterObject->CountPort ) << 8;

    KeReleaseSpinLock(&AdapterObject->SpinLock, OldIrql);

    /*
     * We must return twice the sound for channel >= 4 because it's the size
     * of words (16-bit) and not bytes.
     */
    if (AdapterObject->Channel < 4)
      return Count;
    else
      return Count << 1;
  }

  return 0;
}

/* EOF */
