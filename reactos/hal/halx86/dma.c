/* $Id: dma.c,v 1.3 2002/09/08 10:22:24 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/dma.c
 * PURPOSE:         DMA functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>
#include <hal.h>

ADAPTER_OBJECT AdapterObjects[] = {
  { 0, (PVOID)0x87, (PVOID)0x1, (PVOID)0x0, { 0 }, NULL },
  { 1, (PVOID)0x83, (PVOID)0x3, (PVOID)0x2, { 0 }, NULL },
  { 2, (PVOID)0x81, (PVOID)0x5, (PVOID)0x4, { 0 }, NULL },
  { 3, (PVOID)0x82, (PVOID)0x7, (PVOID)0x6, { 0 }, NULL } };


/* FUNCTIONS *****************************************************************/

PVOID STDCALL
HalAllocateCommonBuffer (PADAPTER_OBJECT		AdapterObject,
			 ULONG			Length,
			 PPHYSICAL_ADDRESS	LogicalAddress,
			 BOOLEAN			CacheEnabled)
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
 */
{
   UNIMPLEMENTED;
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
 */
{
  /* Validate parameters in device description, and return a pointer to
     the adapter object for the requested dma channel */
  if( DeviceDescription->Version != DEVICE_DESCRIPTION_VERSION )
    return NULL;
  if( DeviceDescription->Master )
    return NULL;
  if( DeviceDescription->ScatterGather )
    return NULL;
  if( DeviceDescription->AutoInitialize )
    return NULL;
  if( DeviceDescription->Dma32BitAddress )
    return NULL;
  if( DeviceDescription->InterfaceType != Isa )
     return NULL;
  /*  if( DeviceDescription->DmaWidth != Width8Bits )
      return NULL;*/
  *NumberOfMapRegisters = 0x10;
  AdapterObjects[DeviceDescription->DmaChannel].Buffer = 0;
  return &AdapterObjects[DeviceDescription->DmaChannel];
}

ULONG STDCALL
HalReadDmaCounter (PADAPTER_OBJECT	AdapterObject)
{
   UNIMPLEMENTED;
}

/* EOF */
