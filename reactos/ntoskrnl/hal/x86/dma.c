/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/hal/x86/dma.c
 * PURPOSE:         DMA functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* TYPES *********************************************************************/

#define MAP_REGISTER_SIZE (PAGESIZE)
#define NR_MAP_REGISTERS  (32)

typedef struct 
{
   PVOID VirtualAddress;
   PVOID PhysicalAddress;
} MAP_REGISTER, *PMAP_REGISTER;
/*
typedef struct _ADAPTER_OBJECT
{
   DEVICE_DESCRIPTION desc;
   KDEVICE_QUEUE wait_queue;
} ADAPTER_OBJECT, *PADAPTER_OBJECT
*/
/* GLOBALS *******************************************************************/

static PMAP_REGISTER map_registers[];

/* FUNCTIONS *****************************************************************/

VOID HalInitializeAdapterSupport()
{
   /* ?? */
}

ULONG HalGetDmaAlignmentRequirement()
/*
 * FUNCTION: Returns the size of the cache boundary
 */
{
   return(1);
}

PVOID HalAllocateCommonBuffer(PADAPTER_OBJECT AdapterObject,
			      ULONG Length,
			      PPHYSICAL_ADDRESS LogicalAddress,
			      BOOLEAN CacheEnabled)
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
{/*
   PVOID Buffer;
   PHYSICAL_ADDRESS highest_address;
  
   highest_address.HighPart = 0;
   if (AdapterObject->Desc.Dma32BitAddresses )
     {
	highest_address.LowPart = 0xffffffff;
     }
   else
     {
	highest_address.LowPart = 0xfffff;
     }
   Buffer = MmAllocateContiguousMemory(Length,&highest_address);
   LogicalAddress->HighPart = 0;
   LogicalAddress->LowPart = MmGetPhysicalAddress(Buffer);
   return(Buffer);*/
}

VOID HalFreeCommonBuffer(PADAPTER_OBJECT AdapterObject,
			 ULONG Length,
			 PHYSICAL_ADDRESS LogicalAddress,
			 PVOID VirtualAddress,
			 BOOLEAN CacheEnabled)
{
   MmFreeContiguousMemory(VirtualAddress);
}

PADAPTER_OBJECT HalGetAdapter(PDEVICE_DESCRIPTION DeviceDescription,
			      PULONG NumberOfMapRegisters)
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
/*   PADAPTER_OBJECT adapter;
   
   adapter = ExAllocatePool(NonPagedPool,sizeof(ADAPTER_OBJECT));
   RtlCopyMemory(&adapter->desc,DeviceDescription,sizeof(DEVICE_DESCRIPTION));
 */
}

ULONG HalReadDmaCounter(PADAPTER_OBJECT AdapterObject)
{
   UNIMPLEMENTED;
}

ULONG KeGetDcacheFillSize()
/*
 * FUNCTION: Returns the microprocessor's data cache-line boundary in bytes
 */
{
   return(1);
}
