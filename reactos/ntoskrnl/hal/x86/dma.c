/* $Id: dma.c,v 1.9 2000/12/30 01:41:29 ekohl Exp $
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
 * defined the device description structure
 * ARGUMENTS:
 *        DeviceObject = Structure describing the attributes of the device
 *        NumberOfMapRegisters (OUT) = Returns the maximum number of map
 *                                     registers the device driver can
 *                                     allocate for DMA transfer operations
 * RETURNS: The allocated adapter object on success
 *          NULL on failure
 */
{
   UNIMPLEMENTED;
}


ULONG STDCALL
HalReadDmaCounter (PADAPTER_OBJECT	AdapterObject)
{
   UNIMPLEMENTED;
}

/* EOF */
