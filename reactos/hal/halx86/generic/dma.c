/* $Id$
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
#define NDEBUG
#include <internal/debug.h>
#include <hal.h>

/* Adapters for each channel */
PADAPTER_OBJECT HalpEisaAdapter[8];

/* FUNCTIONS *****************************************************************/

VOID
HalpInitDma (VOID)
{
  /* TODO: Initialize the first Map Buffer */
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
  if ((AdapterObject->Dma32BitAddresses) && (AdapterObject->MasterDevice)) {
      HighestAddress.u.LowPart = 0xFFFFFFFF; /* 32Bit: 4GB address range */
  } else {
      HighestAddress.u.LowPart = 0x00FFFFFF; /* 24Bit: 16MB address range */
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
 *        Testing
 */
{
	PADAPTER_OBJECT AdapterObject;
	DWORD ChannelSelect;
	DWORD Controller;
	ULONG MaximumLength;
	BOOLEAN ChannelSetup = TRUE;
	DMA_MODE DmaMode = {0};	

	DPRINT("Entered Function\n");
  
	/* Validate parameters in device description, and return a pointer to
	the adapter object for the requested dma channel */
	if(DeviceDescription->Version != DEVICE_DESCRIPTION_VERSION) {
		DPRINT("Invalid Adapter version!\n");
		return NULL;
	}

	DPRINT("Checking Interface Type: %x \n", DeviceDescription->InterfaceType);
	if (DeviceDescription->InterfaceType == PCIBus) {
		if (DeviceDescription->Master == FALSE) {
			DPRINT("Invalid request!\n");
			return NULL;
		}
		ChannelSetup = FALSE;
	}
	
	/* There are only 8 DMA channels on ISA, so any request above this
	should not get any channel setup */
	if (DeviceDescription->DmaChannel >= 8) {
		ChannelSetup = FALSE;
	}
	
	/* Channel 4 is Reserved for Chaining, so you cant use it */
	if (DeviceDescription->DmaChannel == 4 && ChannelSetup) {
		DPRINT("Invalid request!\n");
		return NULL;
	}
	
	/* Devices that support Scatter/Gather do not need Map Registers */
	if (DeviceDescription->ScatterGather ||
	    DeviceDescription->InterfaceType == PCIBus) {
		*NumberOfMapRegisters = 0;
	}
	
	/* Check if Extended DMA is available (we're just going to do a random read/write
	I picked Channel 2 because it's the first Channel in the Register */
	WRITE_PORT_UCHAR((PUCHAR)FIELD_OFFSET(EISA_CONTROL, DmaController1Pages.Channel2), 0x2A);
	if (READ_PORT_UCHAR((PUCHAR)FIELD_OFFSET(EISA_CONTROL, DmaController1Pages.Channel2)) == 0x2A) {
		HalpEisaDma = TRUE;
	}
	 
	/* Find out how many Map Registers we need */
	DPRINT("Setting up Adapter Settings!\n");
	MaximumLength = DeviceDescription->MaximumLength & 0x7FFFFFFF;
        *NumberOfMapRegisters = BYTES_TO_PAGES(MaximumLength) + 1;
	
	/* Set the Channel Selection */
	ChannelSelect = DeviceDescription->DmaChannel & 0x03;
	DmaMode.Channel = ChannelSelect;
	
	/* Get the Controller Setup */
	Controller = (DeviceDescription->DmaChannel & 0x04) ? 2 : 1;
	
	/* Get the Adapter Object */
	if (HalpEisaAdapter[DeviceDescription->DmaChannel] != NULL) {
	
		/* Already allocated, return it */
		DPRINT("Getting an Adapter Object from the Cache\n");
		AdapterObject = HalpEisaAdapter[DeviceDescription->DmaChannel];
		
		/* Do we need more Map Registers this time? */
		if ((AdapterObject->NeedsMapRegisters) && 
		    (*NumberOfMapRegisters > AdapterObject->MapRegistersPerChannel)) {
			AdapterObject->MapRegistersPerChannel = *NumberOfMapRegisters;
		}
		
		} else {
	
		/* We have to allocate a new object! How exciting! */
		DPRINT("Allocating a new Adapter Object\n");
		AdapterObject = HalpAllocateAdapterEx(*NumberOfMapRegisters,
						      FALSE,
						      DeviceDescription->Dma32BitAddresses);

		if (AdapterObject == NULL) return NULL;
		
		HalpEisaAdapter[DeviceDescription->DmaChannel] = AdapterObject;
		
		if (!*NumberOfMapRegisters) {
			/* Easy case, no Map Registers needed */
			AdapterObject->NeedsMapRegisters = FALSE;
		
			/* If you're the master, you get all you want */
			if (DeviceDescription->Master) {
				AdapterObject->MapRegistersPerChannel= *NumberOfMapRegisters;
			} else {
				AdapterObject->MapRegistersPerChannel = 1;
			}
		} else {
			/* We Desire Registers */
			AdapterObject->NeedsMapRegisters = TRUE;
		
			/* The registers you want */
			AdapterObject->MapRegistersPerChannel = *NumberOfMapRegisters;
		
			/* Increase commitment */
			MasterAdapter->CommittedMapRegisters += *NumberOfMapRegisters;
		}
	}
	
	/* Set up DMA Structure */
	if (Controller == 1) {
		AdapterObject->AdapterBaseVa = (PVOID)FIELD_OFFSET(EISA_CONTROL, DmaController1);
	} else {
		AdapterObject->AdapterBaseVa = (PVOID)FIELD_OFFSET(EISA_CONTROL, DmaController2);
	}
	        
	/* Set up Some Adapter Data */
	DPRINT("Setting up an Adapter Object\n");
	AdapterObject->IgnoreCount = DeviceDescription->IgnoreCount;
	AdapterObject->Dma32BitAddresses = DeviceDescription->Dma32BitAddresses;
	AdapterObject->Dma64BitAddresses = DeviceDescription->Dma64BitAddresses;
	AdapterObject->ScatterGather = DeviceDescription->ScatterGather;
        AdapterObject->MasterDevice = DeviceDescription->Master;
	if (DeviceDescription->InterfaceType != PCIBus) AdapterObject->LegacyAdapter = TRUE;
	
	/* Everything below is not required if we don't need a channel */
	if (!ChannelSetup) {
		DPRINT("Retuning Adapter Object without Channel Setup\n");
		return AdapterObject;
	}
	
	AdapterObject->ChannelNumber = ChannelSelect;
	
	
	/* Set up the Page Port */
	if (Controller == 1) {
		switch (ChannelSelect) {
		
		case 0:
			AdapterObject->PagePort = (PUCHAR)(FIELD_OFFSET(DMA_PAGE, Channel0) +
			                                   FIELD_OFFSET(EISA_CONTROL, DmaController1Pages));
			break;
		case 1:
			AdapterObject->PagePort = (PUCHAR)(FIELD_OFFSET(DMA_PAGE, Channel1) +
			                                   FIELD_OFFSET(EISA_CONTROL, DmaController1Pages));
			break;
		case 2:
			AdapterObject->PagePort = (PUCHAR)(FIELD_OFFSET(DMA_PAGE, Channel2) +
			                                   FIELD_OFFSET(EISA_CONTROL, DmaController1Pages));
			break;
		case 3:
			AdapterObject->PagePort = (PUCHAR)(FIELD_OFFSET(DMA_PAGE, Channel3) +
			                                   FIELD_OFFSET(EISA_CONTROL, DmaController1Pages));
			break;
		}
	
		/* Set Controller Number */
		AdapterObject->AdapterNumber = 1; 
	} else {
		switch (ChannelSelect) {
		
		case 1:
			AdapterObject->PagePort = (PUCHAR)(FIELD_OFFSET(DMA_PAGE, Channel5) +
			                                   FIELD_OFFSET(EISA_CONTROL, DmaController1Pages));
			break;
		case 2:
			AdapterObject->PagePort = (PUCHAR)(FIELD_OFFSET(DMA_PAGE, Channel6) +
			                                   FIELD_OFFSET(EISA_CONTROL, DmaController1Pages));
			break;
		case 3:
			AdapterObject->PagePort = (PUCHAR)(FIELD_OFFSET(DMA_PAGE, Channel7) +
			                                   FIELD_OFFSET(EISA_CONTROL, DmaController1Pages));
			break;
		}
		
		/* Set Controller Number */
		AdapterObject->AdapterNumber = 2; 
	}
	
	/* Set up the Extended Register */
	if (HalpEisaDma) {
		DMA_EXTENDED_MODE ExtendedMode;
		
		ExtendedMode.ChannelNumber = ChannelSelect;
	
		switch (DeviceDescription->DmaSpeed) {
		
		case Compatible:
			ExtendedMode.TimingMode = COMPATIBLE_TIMING;
			break;
		
		case TypeA:
			ExtendedMode.TimingMode = TYPE_A_TIMING;
			break;
			
		case TypeB:
			ExtendedMode.TimingMode = TYPE_B_TIMING;
			break;
			
		case TypeC:
			ExtendedMode.TimingMode = BURST_TIMING;
			break;
		
		default:
			return NULL;
		}
	
		switch (DeviceDescription->DmaWidth) {
		
		case Width8Bits:
			ExtendedMode.TransferSize = B_8BITS;
			break;
		
		case Width16Bits:
			ExtendedMode.TransferSize = B_16BITS;
			break;
			
		case Width32Bits:
			ExtendedMode.TransferSize = B_32BITS;
			break;
			
		default:
			return NULL;
				}
		
		if (Controller == 1) {
			WRITE_PORT_UCHAR((PUCHAR)FIELD_OFFSET(EISA_CONTROL, DmaExtendedMode1),
				    	*((PUCHAR)&ExtendedMode));
		} else {
	       		WRITE_PORT_UCHAR((PUCHAR)FIELD_OFFSET(EISA_CONTROL, DmaExtendedMode2),
				    	*((PUCHAR)&ExtendedMode));
		}
	}

	/* Do 8/16-bit validation */
	DPRINT("Validating an Adapter Object\n");
	if (!DeviceDescription->Master) {
		if ((DeviceDescription->DmaWidth == Width8Bits) && (Controller != 1)) {
			return NULL; /* 8-bit is only avalable on Controller 1 */
		} else if (DeviceDescription->DmaWidth == Width16Bits) {
			if (Controller != 2) {
				return NULL; /* 16-bit is only avalable on Controller 2 */
			} else {
				AdapterObject->Width16Bits = TRUE;
			}
		}
	}

	DPRINT("Final DMA Request Mode Setting of the Adapter Object\n");

	/* Set the DMA Request Modes */
	if (DeviceDescription->Master) {
		/* This is a cascade request */
		DmaMode.RequestMode = CASCADE_REQUEST_MODE;
		
		/* Send the request */
		if (AdapterObject->AdapterNumber == 1) {
			/* Set the Request Data */
			WRITE_PORT_UCHAR(&((PDMA1_CONTROL)AdapterObject->AdapterBaseVa)->Mode,
				  	AdapterObject->AdapterModeByte);
					  
			/* Unmask DMA Channel */
			WRITE_PORT_UCHAR(&((PDMA1_CONTROL)AdapterObject->AdapterBaseVa)->SingleMask,
				  	AdapterObject->ChannelNumber | DMA_CLEARMASK);
		} else {
			/* Set the Request Data */
			WRITE_PORT_UCHAR(&((PDMA2_CONTROL)AdapterObject->AdapterBaseVa)->Mode,
				  	AdapterObject->AdapterModeByte);
				  
			/* Unmask DMA Channel */
			WRITE_PORT_UCHAR(&((PDMA2_CONTROL)AdapterObject->AdapterBaseVa)->SingleMask,
				  	AdapterObject->ChannelNumber | DMA_CLEARMASK);
		}
	} else if (DeviceDescription->DemandMode) {
		/* This is a Demand request */
		DmaMode.RequestMode = DEMAND_REQUEST_MODE;
	} else {
		/* Normal Request */
		DmaMode.RequestMode = SINGLE_REQUEST_MODE;
	}
	
	/* Auto Initialize Enabled or Not*/
	DmaMode.AutoInitialize = DeviceDescription->AutoInitialize;
	AdapterObject->AdapterMode = DmaMode;
	return AdapterObject;
}

ULONG STDCALL
HalReadDmaCounter (PADAPTER_OBJECT	AdapterObject)
{
  KIRQL OldIrql;
  ULONG Count;

 	KeAcquireSpinLock(&AdapterObject->MasterAdapter->SpinLock, &OldIrql);
  
  	/* Send the Request to the specific controller */
	if (AdapterObject->AdapterNumber == 1) {
  	
		/* Set this for Ease */
  		PDMA1_CONTROL DmaControl1 = AdapterObject->AdapterBaseVa;
	
		/* Send Reset */
		WRITE_PORT_UCHAR(&DmaControl1->ClearBytePointer, 0);
	
		/* Read Count */
		Count = READ_PORT_UCHAR(&DmaControl1->DmaAddressCount
					[AdapterObject->ChannelNumber].DmaBaseCount);
		Count |= READ_PORT_UCHAR(&DmaControl1->DmaAddressCount
					[AdapterObject->ChannelNumber].DmaBaseCount) << 8;
      
	} else {

		/* Set this for Ease */
  		PDMA2_CONTROL DmaControl2 = AdapterObject->AdapterBaseVa;
	
		/* Send Reset */
		WRITE_PORT_UCHAR(&DmaControl2->ClearBytePointer, 0);
	
		/* Read Count */
		Count = READ_PORT_UCHAR(&DmaControl2->DmaAddressCount
					[AdapterObject->ChannelNumber].DmaBaseCount);
		Count |= READ_PORT_UCHAR(&DmaControl2->DmaAddressCount
					[AdapterObject->ChannelNumber].DmaBaseCount) << 8;
	}
	
	/* Play around with the count (add bias and multiply by 2 if 16-bit DMA) */
	Count ++;
	if (AdapterObject->Width16Bits) Count *=2 ;
	
	KeReleaseSpinLock(&AdapterObject->MasterAdapter->SpinLock, OldIrql);
	
	/* Return it */
	return Count;
}

/* EOF */
