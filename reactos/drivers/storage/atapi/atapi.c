/*
 *  ReactOS kernel
 *  Copyright (C) 2001, 2002, 2003, 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: atapi.c,v 1.46 2004/02/29 12:27:52 hbirr Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS ATAPI miniport driver
 * FILE:        drivers/storage/atapi/atapi.c
 * PURPOSE:     ATAPI miniport driver
 * PROGRAMMERS: Eric Kohl (ekohl@rz-online.de)
 *              Hartmut Birr 
 * REVISIONS:
 *              09-09-2001 Created
 */

/*
 * Note:
 *   This driver is derived from Rex Jolliff's ide driver. Lots of his
 *   routines are still in here although they belong into the higher level
 *   drivers. They will be moved away as soon as possible.
 */

/*
 * TODO:
 *	- implement sending of atapi commands
 *	- handle removable atapi non-cdrom drives
 */

#define ENABLE_PCI
#define ENABLE_NATIVE_PCI
#define ENABLE_ISA
//#define ENABLE_DMA

//  -------------------------------------------------------------------------

#include <ddk/ntddk.h>
#include <ddk/srb.h>
#include <ddk/scsi.h>
#include <ddk/ntddscsi.h>
#include <ntos/kefuncs.h>

#include "atapi.h"

#define NDEBUG
#include <debug.h>

#define VERSION  "0.0.1"


//  -------------------------------------------------------  File Static Data

#ifdef ENABLE_DMA
typedef struct _PRD
{
  ULONG PhysAddress;
  ULONG Length;
} PRD, *PPRD;
#endif

//    ATAPI_MINIPORT_EXTENSION
//
//  DESCRIPTION:
//    Extension to be placed in each port device object
//
//  ACCESS:
//    Allocated from NON-PAGED POOL
//    Available at any IRQL
//

typedef struct _ATAPI_MINIPORT_EXTENSION
{
  IDE_DRIVE_IDENTIFY DeviceParams[2];
  ULONG DeviceFlags[2];
  ULONG TransferSize[2];

  ULONG CommandPortBase;
  ULONG ControlPortBase;
  ULONG BusMasterRegisterBase;

  PSCSI_REQUEST_BLOCK CurrentSrb;

  PUCHAR DataBuffer;
  ULONG DataTransferLength;

  BOOLEAN FASTCALL (*Handler)(IN struct _ATAPI_MINIPORT_EXTENSION* DevExt);
#ifdef ENABLE_DMA
  BOOL UseDma;
  ULONG PRDCount;
  ULONG PRDMaxCount;
  PPRD PRDTable;
  SCSI_PHYSICAL_ADDRESS PRDTablePhysicalAddress;
#endif
} ATAPI_MINIPORT_EXTENSION, *PATAPI_MINIPORT_EXTENSION;

/* DeviceFlags */
#define DEVICE_PRESENT           0x00000001
#define DEVICE_ATAPI             0x00000002
#define DEVICE_MULTI_SECTOR_CMD  0x00000004
#define DEVICE_DWORD_IO          0x00000008
#define DEVICE_48BIT_ADDRESS     0x00000010
#define DEVICE_MEDIA_STATUS      0x00000020
#define DEVICE_DMA_CMD		 0x00000040


typedef struct _UNIT_EXTENSION
{
  ULONG Dummy;
} UNIT_EXTENSION, *PUNIT_EXTENSION;

PCI_SLOT_NUMBER LastSlotNumber;

#ifdef ENABLE_NATIVE_PCI
typedef struct _PCI_NATIVE_CONTROLLER 
{
  USHORT VendorID;
  USHORT DeviceID;
}
PCI_NATIVE_CONTROLLER, *PPCI_NATIVE_CONTROLLER;

PCI_NATIVE_CONTROLLER const PciNativeController[] = 
{
    {
	0x105A,		    // Promise 
	0x4D68,		    // PDC20268, Ultra100TX2
    },
    {
	0x105A,		    // Promise 
	0x4D30,		    // PDC20267, Ultra100
    }
};
#endif


//  -----------------------------------------------  Discardable Declarations

#ifdef  ALLOC_PRAGMA

//  make the initialization routines discardable, so that they 
//  don't waste space

#pragma  alloc_text(init, DriverEntry)

//  make the PASSIVE_LEVEL routines pageable, so that they don't
//  waste nonpaged memory

#endif  /*  ALLOC_PRAGMA  */

//  ---------------------------------------------------- Forward Declarations

#ifdef ENABLE_DMA
static BOOLEAN 
AtapiInitDma(PATAPI_MINIPORT_EXTENSION DevExt,
	     PSCSI_REQUEST_BLOCK Srb,
	     BYTE cmd);
#endif

static ULONG STDCALL
AtapiFindCompatiblePciController(PVOID DeviceExtension,
				 PVOID HwContext,
				 PVOID BusInformation,
				 PCHAR ArgumentString,
				 PPORT_CONFIGURATION_INFORMATION ConfigInfo,
				 PBOOLEAN Again);

static ULONG STDCALL
AtapiFindIsaBusController(PVOID DeviceExtension,
			  PVOID HwContext,
			  PVOID BusInformation,
			  PCHAR ArgumentString,
			  PPORT_CONFIGURATION_INFORMATION ConfigInfo,
			  PBOOLEAN Again);

static ULONG STDCALL
AtapiFindNativePciController(PVOID DeviceExtension,
			     PVOID HwContext,
			     PVOID BusInformation,
			     PCHAR ArgumentString,
			     PPORT_CONFIGURATION_INFORMATION ConfigInfo,
			     PBOOLEAN Again);

static BOOLEAN STDCALL
AtapiInitialize(IN PVOID DeviceExtension);

static BOOLEAN STDCALL
AtapiResetBus(IN PVOID DeviceExtension,
	      IN ULONG PathId);

static BOOLEAN STDCALL
AtapiStartIo(IN PVOID DeviceExtension,
	     IN PSCSI_REQUEST_BLOCK Srb);

static VOID
AtapiExecuteCommand(PATAPI_MINIPORT_EXTENSION DevExt, 
		    BYTE command, 
		    BOOLEAN FASTCALL (*Handler)(PATAPI_MINIPORT_EXTENSION));

static BOOLEAN STDCALL
AtapiInterrupt(IN PVOID DeviceExtension);

static BOOLEAN FASTCALL
AtapiNoDataInterrupt(PATAPI_MINIPORT_EXTENSION DevExt);

static BOOLEAN FASTCALL
AtapiPacketInterrupt(IN PATAPI_MINIPORT_EXTENSION DevExt);

static BOOLEAN FASTCALL
AtapiReadInterrupt(IN PATAPI_MINIPORT_EXTENSION DevExt);

#ifdef ENABLE_DMA
static BOOLEAN FASTCALL
AtapiDmaInterrupt(IN PATAPI_MINIPORT_EXTENSION DevExt);
#endif

static BOOLEAN FASTCALL
AtapiWriteInterrupt(IN PATAPI_MINIPORT_EXTENSION DevExt);

static BOOLEAN
AtapiFindDevices(PATAPI_MINIPORT_EXTENSION DeviceExtension,
		 PPORT_CONFIGURATION_INFORMATION ConfigInfo);

static BOOLEAN
AtapiIdentifyDevice(IN ULONG CommandPort,
		    IN ULONG ControlPort,
		    IN ULONG DriveNum,
		    IN BOOLEAN Atapi,
		    OUT PIDE_DRIVE_IDENTIFY DrvParms);

static BOOLEAN
AtapiPolledRead(IN ULONG CommandPort,
		IN ULONG ControlPort,
		IN UCHAR PreComp,
		IN UCHAR SectorCnt,
		IN UCHAR SectorNum,
		IN UCHAR CylinderLow,
		IN UCHAR CylinderHigh,
		IN UCHAR DrvHead,
		IN UCHAR Command,
		OUT PUCHAR Buffer);

static ULONG
AtapiSendAtapiCommand(IN PATAPI_MINIPORT_EXTENSION DeviceExtension,
		      IN PSCSI_REQUEST_BLOCK Srb);

static ULONG
AtapiSendIdeCommand(IN PATAPI_MINIPORT_EXTENSION DeviceExtension,
		    IN PSCSI_REQUEST_BLOCK Srb);

static ULONG
AtapiInquiry(IN PATAPI_MINIPORT_EXTENSION DeviceExtension,
	     IN PSCSI_REQUEST_BLOCK Srb);

static ULONG
AtapiReadCapacity(IN PATAPI_MINIPORT_EXTENSION DeviceExtension,
		  IN PSCSI_REQUEST_BLOCK Srb);

static ULONG
AtapiReadWrite(IN PATAPI_MINIPORT_EXTENSION DeviceExtension,
	       IN PSCSI_REQUEST_BLOCK Srb);

static ULONG
AtapiFlushCache(PATAPI_MINIPORT_EXTENSION DeviceExtension,
		PSCSI_REQUEST_BLOCK Srb);

static ULONG
AtapiTestUnitReady(PATAPI_MINIPORT_EXTENSION DeviceExtension,
		   PSCSI_REQUEST_BLOCK Srb);

static UCHAR
AtapiErrorToScsi(PVOID DeviceExtension,
		 PSCSI_REQUEST_BLOCK Srb);

static VOID
AtapiScsiSrbToAtapi (PSCSI_REQUEST_BLOCK Srb);

//  ----------------------------------------------------------------  Inlines

void
IDESwapBytePairs(char *Buf,
                 int Cnt)
{
  char  t;
  int   i;

  for (i = 0; i < Cnt; i += 2)
    {
      t = Buf[i];
      Buf[i] = Buf[i+1];
      Buf[i+1] = t;
    }
}


//  -------------------------------------------------------  Public Interface

//    DriverEntry
//
//  DESCRIPTION:
//    This function initializes the driver, locates and claims 
//    hardware resources, and creates various NT objects needed
//    to process I/O requests.
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN  PDRIVER_OBJECT   DriverObject  System allocated Driver Object
//                                       for this driver
//    IN  PUNICODE_STRING  RegistryPath  Name of registry driver service 
//                                       key
//
//  RETURNS:
//    NTSTATUS

STDCALL NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
  HW_INITIALIZATION_DATA InitData;
  NTSTATUS Status;

  DPRINT("ATAPI Driver %s\n", VERSION);
  DPRINT("RegistryPath: '%wZ'\n", RegistryPath);

  /* Initialize data structure */
  RtlZeroMemory(&InitData,
		sizeof(HW_INITIALIZATION_DATA));
  InitData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);
  InitData.HwInitialize = AtapiInitialize;
  InitData.HwResetBus = AtapiResetBus;
  InitData.HwStartIo = AtapiStartIo;
  InitData.HwInterrupt = AtapiInterrupt;

  InitData.DeviceExtensionSize = sizeof(ATAPI_MINIPORT_EXTENSION);
  InitData.SpecificLuExtensionSize = sizeof(UNIT_EXTENSION);

  InitData.MapBuffers = TRUE;

  /* Search the PCI bus for compatibility mode ide controllers */
#ifdef ENABLE_PCI
  InitData.NeedPhysicalAddresses = TRUE;

  InitData.HwFindAdapter = AtapiFindCompatiblePciController;
  InitData.NumberOfAccessRanges = 3;
  InitData.AdapterInterfaceType = PCIBus;

  InitData.VendorId = NULL;
  InitData.VendorIdLength = 0;
  InitData.DeviceId = NULL;
  InitData.DeviceIdLength = 0;

  Status = ScsiPortInitialize(DriverObject,
			      RegistryPath,
			      &InitData,
			      NULL);
//  if (newStatus < statusToReturn)
//    statusToReturn = newStatus;
#endif

  /* Search the PCI bus for all ide controllers */
#ifdef ENABLE_NATIVE_PCI
  InitData.NeedPhysicalAddresses = TRUE;

  InitData.HwFindAdapter = AtapiFindNativePciController;
  InitData.NumberOfAccessRanges = 3;
  InitData.AdapterInterfaceType = PCIBus;

  InitData.VendorId = 0;
  InitData.VendorIdLength = 0;
  InitData.DeviceId = 0;
  InitData.DeviceIdLength = 0;

  LastSlotNumber.u.AsULONG = 0xFFFFFFFF;

  Status = ScsiPortInitialize(DriverObject,
			      RegistryPath,
			      &InitData,
			      NULL);
//  if (newStatus < statusToReturn)
//    statusToReturn = newStatus;
#endif

  /* Search the ISA bus for ide controllers */
#ifdef ENABLE_ISA
  InitData.HwFindAdapter = AtapiFindIsaBusController;
  InitData.NumberOfAccessRanges = 2;
  InitData.AdapterInterfaceType = Isa;

  InitData.VendorId = NULL;
  InitData.VendorIdLength = 0;
  InitData.DeviceId = NULL;
  InitData.DeviceIdLength = 0;

  Status = ScsiPortInitialize(DriverObject,
			      RegistryPath,
			      &InitData,
			      NULL);
//      if (newStatus < statusToReturn)
//        statusToReturn = newStatus;
#endif

  DPRINT("Returning from DriverEntry\n");

  return(Status);
}


BOOLEAN
AtapiClaimHwResources(PATAPI_MINIPORT_EXTENSION DevExt,
		      PPORT_CONFIGURATION_INFORMATION ConfigInfo,
		      INTERFACE_TYPE InterfaceType,
		      ULONG CommandPortBase,
		      ULONG ControlPortBase,
		      ULONG BusMasterPortBase,
		      ULONG InterruptVector)
{
   SCSI_PHYSICAL_ADDRESS IoAddress;
   PVOID IoBase;
#ifdef ENABLE_DMA
   ULONG Length;
#endif
   IoAddress = ScsiPortConvertUlongToPhysicalAddress(CommandPortBase);
   IoBase = ScsiPortGetDeviceBase((PVOID)DevExt,
                                  InterfaceType,
				  ConfigInfo->SystemIoBusNumber,
				  IoAddress,
				  8,
				  TRUE);
   if (IoBase == NULL)
   {
      return FALSE;
   }
   DevExt->Handler = NULL;
   DevExt->CommandPortBase = (ULONG)IoBase;
   ConfigInfo->AccessRanges[0].RangeStart = IoAddress;
   ConfigInfo->AccessRanges[0].RangeLength = 8;
   ConfigInfo->AccessRanges[0].RangeInMemory = FALSE;

   if (ControlPortBase)
   {
      IoAddress = ScsiPortConvertUlongToPhysicalAddress(ControlPortBase + 2);
      IoBase = ScsiPortGetDeviceBase((PVOID)DevExt,
                                     InterfaceType,
				     ConfigInfo->SystemIoBusNumber,
				     IoAddress,
				     1,
				     TRUE);
      if (IoBase == NULL)
      {
         ScsiPortFreeDeviceBase((PVOID)DevExt,
	                        (PVOID)DevExt->CommandPortBase);
         return FALSE;
      }
      DevExt->ControlPortBase = (ULONG)IoBase;
      ConfigInfo->AccessRanges[1].RangeStart = IoAddress;
      ConfigInfo->AccessRanges[1].RangeLength = 1;
      ConfigInfo->AccessRanges[1].RangeInMemory = FALSE;
   }
   if (BusMasterPortBase)
   {
      IoAddress = ScsiPortConvertUlongToPhysicalAddress(BusMasterPortBase);
      IoBase = ScsiPortGetDeviceBase((PVOID)DevExt,
                                     InterfaceType,
				     ConfigInfo->SystemIoBusNumber,
				     IoAddress,
				     8,
				     TRUE);
      if (IoBase == NULL)
      {
         ScsiPortFreeDeviceBase((PVOID)DevExt, (PVOID)DevExt->CommandPortBase);
         ScsiPortFreeDeviceBase((PVOID)DevExt, (PVOID)DevExt->ControlPortBase);
         return FALSE;
      }
      DevExt->BusMasterRegisterBase = (ULONG)IoBase;
      ConfigInfo->AccessRanges[2].RangeStart = IoAddress;
      ConfigInfo->AccessRanges[2].RangeLength = 8;
      ConfigInfo->AccessRanges[2].RangeInMemory = FALSE;
#ifdef ENABLE_DMA
//      ConfigInfo->DmaChannel = SP_UNINITIALIZED_VALUE;
//      ConfigInfo->DmaPort = SP_UNINITIALIZED_VALUE;
      ConfigInfo->DmaWidth = Width32Bits;
      ConfigInfo->DmaSpeed = Compatible;
//      ConfigInfo->ScatterGather = TRUE;
      ConfigInfo->Master = TRUE;
//      ConfigInfo->NumberOfPhysicalBreaks = 

      DevExt->PRDMaxCount = PAGE_SIZE / sizeof(PRD);  
      DevExt->PRDTable = ScsiPortGetUncachedExtension(DevExt, ConfigInfo, sizeof(PRD) * DevExt->PRDMaxCount);
      if (DevExt->PRDTable != NULL)
        {
	  DevExt->PRDTablePhysicalAddress = ScsiPortGetPhysicalAddress(DevExt, NULL, DevExt->PRDTable, &Length);
	}
      if (DevExt->PRDTable == NULL ||
	  DevExt->PRDTablePhysicalAddress.QuadPart == 0LL ||
	  Length < sizeof(PRD) * DevExt->PRDMaxCount)
      {
         ScsiPortFreeDeviceBase((PVOID)DevExt, (PVOID)DevExt->CommandPortBase);
         ScsiPortFreeDeviceBase((PVOID)DevExt, (PVOID)DevExt->ControlPortBase);
         ScsiPortFreeDeviceBase((PVOID)DevExt, (PVOID)DevExt->BusMasterRegisterBase);
	 return FALSE;
      }
#endif
   }
   ConfigInfo->BusInterruptLevel = InterruptVector;
   ConfigInfo->BusInterruptVector = InterruptVector;
   ConfigInfo->InterruptMode = (InterfaceType == Isa) ? Latched : LevelSensitive;

   if ((CommandPortBase == 0x1F0 || ControlPortBase == 0x3F4) && !ConfigInfo->AtdiskPrimaryClaimed)
   {
      ConfigInfo->AtdiskPrimaryClaimed = TRUE;
   }
   if ((CommandPortBase == 0x170 || ControlPortBase == 0x374) && !ConfigInfo->AtdiskSecondaryClaimed)
   {
      ConfigInfo->AtdiskSecondaryClaimed = TRUE;
   }
   return TRUE;
}


#ifdef ENABLE_PCI
static ULONG STDCALL
AtapiFindCompatiblePciController(PVOID DeviceExtension,
				 PVOID HwContext,
				 PVOID BusInformation,
				 PCHAR ArgumentString,
				 PPORT_CONFIGURATION_INFORMATION ConfigInfo,
				 PBOOLEAN Again)
{
  PATAPI_MINIPORT_EXTENSION DevExt = (PATAPI_MINIPORT_EXTENSION)DeviceExtension;
  PCI_SLOT_NUMBER SlotNumber;
  PCI_COMMON_CONFIG PciConfig;
  ULONG DataSize;
  ULONG StartDeviceNumber;
  ULONG DeviceNumber;
  ULONG StartFunctionNumber;
  ULONG FunctionNumber;
  BOOLEAN ChannelFound;
  BOOLEAN DeviceFound;
  ULONG BusMasterBasePort = 0;

  DPRINT("AtapiFindCompatiblePciController() Bus: %lu  Slot: %lu\n",
	  ConfigInfo->SystemIoBusNumber,
	  ConfigInfo->SlotNumber);

  *Again = FALSE;

  /* both channels were claimed: exit */
  if (ConfigInfo->AtdiskPrimaryClaimed == TRUE &&
      ConfigInfo->AtdiskSecondaryClaimed == TRUE)
    return(SP_RETURN_NOT_FOUND);

  SlotNumber.u.AsULONG = ConfigInfo->SlotNumber;
  StartDeviceNumber = SlotNumber.u.bits.DeviceNumber;
  StartFunctionNumber = SlotNumber.u.bits.FunctionNumber;
  for (DeviceNumber = StartDeviceNumber; DeviceNumber < PCI_MAX_DEVICES; DeviceNumber++)
  {
     SlotNumber.u.bits.DeviceNumber = DeviceNumber;
     for (FunctionNumber = StartFunctionNumber; FunctionNumber < PCI_MAX_FUNCTION; FunctionNumber++)
     {
	SlotNumber.u.bits.FunctionNumber = FunctionNumber;
	ChannelFound = FALSE;
	DeviceFound = FALSE;

        DataSize = ScsiPortGetBusData(DeviceExtension,
				      PCIConfiguration,
				      ConfigInfo->SystemIoBusNumber,
				      SlotNumber.u.AsULONG,
				      &PciConfig,
				      PCI_COMMON_HDR_LENGTH);
        if (DataSize != PCI_COMMON_HDR_LENGTH)
	{
	   if (FunctionNumber == 0)
	   {
	      break;
	   }
	   else
	   {
	      continue;
	   }
	}
	  
	DPRINT("%x %x\n", PciConfig.BaseClass, PciConfig.SubClass);
	if (PciConfig.BaseClass == 0x01 &&
	    PciConfig.SubClass == 0x01) // &&
//	    (PciConfig.ProgIf & 0x05) == 0)
	{
	   /* both channels are in compatibility mode */
	   DPRINT("Bus %1lu  Device %2lu  Func %1lu  VenID 0x%04hx  DevID 0x%04hx\n",
		  ConfigInfo->SystemIoBusNumber,
		  SlotNumber.u.bits.DeviceNumber,
		  SlotNumber.u.bits.FunctionNumber,
		  PciConfig.VendorID,
		  PciConfig.DeviceID);
	   DPRINT("ProgIF 0x%02hx\n", PciConfig.ProgIf);

	   DPRINT("Found IDE controller in compatibility mode!\n");

	   ConfigInfo->NumberOfBuses = 1;
	   ConfigInfo->MaximumNumberOfTargets = 2;
	   ConfigInfo->MaximumTransferLength = 0x10000; /* max 64Kbyte */

	   if (PciConfig.ProgIf & 0x80)
	   {
	      DPRINT("Found IDE Bus Master controller!\n");
	      if (PciConfig.u.type0.BaseAddresses[4] & PCI_ADDRESS_IO_SPACE)
	      {
		  BusMasterBasePort = PciConfig.u.type0.BaseAddresses[4] & PCI_ADDRESS_IO_ADDRESS_MASK;
		  DPRINT("  IDE Bus Master Registers at IO %lx\n", BusMasterBasePort);
	      }
	   }
	   if (ConfigInfo->AtdiskPrimaryClaimed == FALSE)
	   {
	      /* Both channels unclaimed: Claim primary channel */
	      DPRINT("Primary channel!\n");
              ChannelFound = AtapiClaimHwResources(DevExt,
		                                   ConfigInfo,
		                                   PCIBus,
		                                   0x1F0,
			                           0x3F4,
			                           BusMasterBasePort,
			                           14);
              *Again = TRUE;
	   }
	   else if (ConfigInfo->AtdiskSecondaryClaimed == FALSE)
	   {
	      /* Primary channel already claimed: claim secondary channel */
	      DPRINT("Secondary channel!\n");

              ChannelFound = AtapiClaimHwResources(DevExt,
		                                   ConfigInfo,
		                                   PCIBus,
		                                   0x170,
			                           0x374,
						   BusMasterBasePort ? BusMasterBasePort + 8 : 0,
			                           15);
	      *Again = FALSE;
	   }
	   /* Find attached devices */
	   if (ChannelFound)
	   {
	      DeviceFound = AtapiFindDevices(DevExt, ConfigInfo);
	      ConfigInfo->SlotNumber = SlotNumber.u.AsULONG;
	      DPRINT("AtapiFindCompatiblePciController() returns: SP_RETURN_FOUND\n");
	      return(SP_RETURN_FOUND);
	   }
	}
	if (FunctionNumber == 0 && !(PciConfig.HeaderType & PCI_MULTIFUNCTION))
	{
	   break;
	}
     }	
     StartFunctionNumber = 0;
  }
  DPRINT("AtapiFindCompatiblePciController() returns: SP_RETURN_NOT_FOUND\n");

  return(SP_RETURN_NOT_FOUND);
}
#endif


#ifdef ENABLE_ISA
static ULONG STDCALL
AtapiFindIsaBusController(PVOID DeviceExtension,
			  PVOID HwContext,
			  PVOID BusInformation,
			  PCHAR ArgumentString,
			  PPORT_CONFIGURATION_INFORMATION ConfigInfo,
			  PBOOLEAN Again)
{
  PATAPI_MINIPORT_EXTENSION DevExt = (PATAPI_MINIPORT_EXTENSION)DeviceExtension;
  BOOLEAN ChannelFound = FALSE;
  BOOLEAN DeviceFound = FALSE;

  DPRINT("AtapiFindIsaBusController() called!\n");

  *Again = FALSE;

  ConfigInfo->NumberOfBuses = 1;
  ConfigInfo->MaximumNumberOfTargets = 2;
  ConfigInfo->MaximumTransferLength = 0x10000; /* max 64Kbyte */

  if (ConfigInfo->AtdiskPrimaryClaimed == FALSE)
    {
      /* Both channels unclaimed: Claim primary channel */
      DPRINT("Primary channel!\n");

      ChannelFound = AtapiClaimHwResources(DevExt,
		                           ConfigInfo,
		                           Isa,
		                           0x1F0,
			                   0x3F4,
			                   0,
			                   14);
      *Again = TRUE;
    }
  else if (ConfigInfo->AtdiskSecondaryClaimed == FALSE)
    {
      /* Primary channel already claimed: claim secondary channel */
      DPRINT("Secondary channel!\n");

      ChannelFound = AtapiClaimHwResources(DevExt,
		                           ConfigInfo,
		                           Isa,
		                           0x170,
			                   0x374,
			                   0,
			                   15);
      *Again = FALSE;
    }
  else
    {
      DPRINT("AtapiFindIsaBusController() both channels claimed. Returns: SP_RETURN_NOT_FOUND\n");
      *Again = FALSE;
      return(SP_RETURN_NOT_FOUND);
    }

  /* Find attached devices */
  if (ChannelFound)
    {
      DeviceFound = AtapiFindDevices(DevExt,
				     ConfigInfo);
      DPRINT("AtapiFindIsaBusController() returns: SP_RETURN_FOUND\n");
      return(SP_RETURN_FOUND);
    }
  *Again = FALSE;
  return SP_RETURN_NOT_FOUND;
}
#endif


#ifdef ENABLE_NATIVE_PCI
static ULONG STDCALL
AtapiFindNativePciController(PVOID DeviceExtension,
			     PVOID HwContext,
			     PVOID BusInformation,
			     PCHAR ArgumentString,
			     PPORT_CONFIGURATION_INFORMATION ConfigInfo,
			     PBOOLEAN Again)
{
  PATAPI_MINIPORT_EXTENSION DevExt = (PATAPI_MINIPORT_EXTENSION)DeviceExtension;
  PCI_COMMON_CONFIG PciConfig;
  PCI_SLOT_NUMBER SlotNumber;
  ULONG DataSize;
  ULONG DeviceNumber;
  ULONG StartDeviceNumber;
  ULONG FunctionNumber;
  ULONG StartFunctionNumber;
  ULONG BusMasterBasePort;
  ULONG Count;
  BOOLEAN ChannelFound;

  DPRINT("AtapiFindNativePciController() called!\n");

  SlotNumber.u.AsULONG = ConfigInfo->SlotNumber;
  StartDeviceNumber = SlotNumber.u.bits.DeviceNumber;
  StartFunctionNumber = SlotNumber.u.bits.FunctionNumber;
  for (DeviceNumber = StartDeviceNumber; DeviceNumber < PCI_MAX_DEVICES; DeviceNumber++)
  {
     SlotNumber.u.bits.DeviceNumber = DeviceNumber;
     for (FunctionNumber = StartFunctionNumber; FunctionNumber < PCI_MAX_FUNCTION; FunctionNumber++)
     {
	SlotNumber.u.bits.FunctionNumber = FunctionNumber;
	DataSize = ScsiPortGetBusData(DeviceExtension,
				      PCIConfiguration,
				      ConfigInfo->SystemIoBusNumber,
				      SlotNumber.u.AsULONG,
				      &PciConfig,
				      PCI_COMMON_HDR_LENGTH);
	if (DataSize != PCI_COMMON_HDR_LENGTH)
	{
	   break;
	}
	for (Count = 0; Count < sizeof(PciNativeController)/sizeof(PCI_NATIVE_CONTROLLER); Count++)
	{
	   if (PciConfig.VendorID == PciNativeController[Count].VendorID &&
	       PciConfig.DeviceID == PciNativeController[Count].DeviceID)
	   {
	      break;
	   }
	}
	if (Count < sizeof(PciNativeController)/sizeof(PCI_NATIVE_CONTROLLER)) 
	{
	   /* We have found a known native pci ide controller */
	   if ((PciConfig.ProgIf & 0x80) && (PciConfig.u.type0.BaseAddresses[4] & PCI_ADDRESS_IO_SPACE))
	   {
	      DPRINT("Found IDE Bus Master controller!\n");
	      BusMasterBasePort = PciConfig.u.type0.BaseAddresses[4] & PCI_ADDRESS_IO_ADDRESS_MASK;
	      DPRINT("  IDE Bus Master Registers at IO %lx\n", BusMasterBasePort);
	   }
	   else
	   {
	      BusMasterBasePort = 0;
	   }

	   DPRINT("VendorID: %04x, DeviceID: %04x\n", PciConfig.VendorID, PciConfig.DeviceID);
	   ConfigInfo->NumberOfBuses = 1;
	   ConfigInfo->MaximumNumberOfTargets = 2;
	   ConfigInfo->MaximumTransferLength = 0x10000; /* max 64Kbyte */

	   /* FIXME:
	        We must not store and use the last tested slot number. If there is a recall
		to the some device and we will claim the primary channel again than the call
		to ScsiPortGetDeviceBase in AtapiClaimHwResource will fail and we can try to
		claim the secondary channel. 
	    */
	   ChannelFound = FALSE;
	   if (LastSlotNumber.u.AsULONG != SlotNumber.u.AsULONG)
	   {
	      /* try to claim primary channel */
	      if ((PciConfig.u.type0.BaseAddresses[0] & PCI_ADDRESS_IO_SPACE) &&
		  (PciConfig.u.type0.BaseAddresses[1] & PCI_ADDRESS_IO_SPACE))
	      {
		 /* primary channel is enabled */
		 ChannelFound = AtapiClaimHwResources(DevExt,
		                                      ConfigInfo,
		                                      PCIBus,
		                                      PciConfig.u.type0.BaseAddresses[0] & PCI_ADDRESS_IO_ADDRESS_MASK,
			                              PciConfig.u.type0.BaseAddresses[1] & PCI_ADDRESS_IO_ADDRESS_MASK,
			                              BusMasterBasePort,
			                              PciConfig.u.type0.InterruptLine);
	         if (ChannelFound)
		 {
                    AtapiFindDevices(DevExt, ConfigInfo);
                    *Again = TRUE;
		    ConfigInfo->SlotNumber = LastSlotNumber.u.AsULONG = SlotNumber.u.AsULONG;
                    return SP_RETURN_FOUND;
		 }
	      }
	   }
	   if (!ChannelFound)
	   {
	      /* try to claim secondary channel */
	      if ((PciConfig.u.type0.BaseAddresses[2] & PCI_ADDRESS_IO_SPACE) &&
		  (PciConfig.u.type0.BaseAddresses[3] & PCI_ADDRESS_IO_SPACE))
	      {
	         /* secondary channel is enabled */
	         ChannelFound = AtapiClaimHwResources(DevExt,
		                                      ConfigInfo,
		                                      PCIBus,
		                                      PciConfig.u.type0.BaseAddresses[2] & PCI_ADDRESS_IO_ADDRESS_MASK,
			                              PciConfig.u.type0.BaseAddresses[3] & PCI_ADDRESS_IO_ADDRESS_MASK,
						      BusMasterBasePort ? BusMasterBasePort + 8 : 0,
			                              PciConfig.u.type0.InterruptLine);
		 if (ChannelFound)
		 {
		    AtapiFindDevices(DevExt, ConfigInfo);
                    *Again = FALSE;
		    LastSlotNumber.u.AsULONG = 0xFFFFFFFF;
                    return SP_RETURN_FOUND;
		 }
	      }
	   }
	}
     }
     StartFunctionNumber = 0;
  }
  *Again = FALSE;
  LastSlotNumber.u.AsULONG = 0xFFFFFFFF;
  DPRINT("AtapiFindNativePciController() done!\n");

  return(SP_RETURN_NOT_FOUND);
}
#endif


static BOOLEAN STDCALL
AtapiInitialize(IN PVOID DeviceExtension)
{
  return(TRUE);
}


static BOOLEAN STDCALL
AtapiResetBus(IN PVOID DeviceExtension,
	      IN ULONG PathId)
{
  return(TRUE);
}


static BOOLEAN STDCALL
AtapiStartIo(IN PVOID DeviceExtension,
	     IN PSCSI_REQUEST_BLOCK Srb)
{
  PATAPI_MINIPORT_EXTENSION DevExt;
  ULONG Result;

  DPRINT("AtapiStartIo() called\n");

  DevExt = (PATAPI_MINIPORT_EXTENSION)DeviceExtension;

  switch (Srb->Function)
    {
      case SRB_FUNCTION_EXECUTE_SCSI:
	DevExt->CurrentSrb = Srb;
	if (DevExt->DeviceFlags[Srb->TargetId] & DEVICE_ATAPI)
	  {
	    Result = AtapiSendAtapiCommand(DevExt,
					   Srb);
	  }
	else
	  {
	    Result = AtapiSendIdeCommand(DevExt,
					 Srb);
	  }
	break;

      case SRB_FUNCTION_ABORT_COMMAND:
	if (DevExt->CurrentSrb != NULL)
	  {
	    Result = SRB_STATUS_ABORT_FAILED;
	  }
	else
	  {
	    Result = SRB_STATUS_SUCCESS;
	  }
	break;

      default:
	Result = SRB_STATUS_INVALID_REQUEST;
	break;
    }

  Srb->SrbStatus = Result;


  if (Result != SRB_STATUS_PENDING)
    {
      DevExt->CurrentSrb = NULL;
      Srb->SrbStatus = (UCHAR)Result;

      ScsiPortNotification(RequestComplete,
			   DeviceExtension,
			   Srb);
      ScsiPortNotification(NextRequest,
			   DeviceExtension,
			   NULL);
    }
  else
    {
      DPRINT("SrbStatus = SRB_STATUS_PENDING\n");
    }

  DPRINT("AtapiStartIo() done\n");

  return(TRUE);
}

static BOOLEAN STDCALL
AtapiInterrupt(IN PVOID DeviceExtension)
{
  PATAPI_MINIPORT_EXTENSION DevExt;
  BYTE Status;
  DevExt = (PATAPI_MINIPORT_EXTENSION)DeviceExtension;

  if (DevExt->Handler == NULL)
    {
      Status =  IDEReadAltStatus(DevExt->ControlPortBase);
      if ((Status & (IDE_SR_DRDY|IDE_SR_BUSY|IDE_SR_ERR|IDE_SR_DRQ)) != IDE_SR_DRDY)
        {
	  static ULONG Count = 0;
	  Count++;
	  DPRINT1("Unexpected Interrupt, CommandPort=%04x, Status=%02x, Count=%ld\n", 
	          DevExt->CommandPortBase, Status, Count);
	}
      return FALSE;
    }
#ifdef ENABLE_DMA
  if (DevExt->UseDma)
    {
      Status = IDEReadDMAStatus(DevExt->BusMasterRegisterBase);
      if (!(Status & 0x04))
        {
	  return FALSE;
	}
    }
  else
#endif
    {
      Status = IDEReadAltStatus(DevExt->ControlPortBase);
      if (Status & IDE_SR_BUSY)
        {
	  return FALSE;
	}
    }
  return DevExt->Handler(DevExt);
}

//  ----------------------------------------------------  Discardable statics


/**********************************************************************
 * NAME							INTERNAL
 *	AtapiFindDevices
 *
 * DESCRIPTION
 *	Searches for devices on the given port.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DeviceExtension
 *		Port device specific information.
 *
 *	ConfigInfo
 *		Port configuration information.
 *
 * RETURN VALUE
 *	TRUE: At least one device is attached to the port.
 *	FALSE: No device is attached to the port.
 */

static BOOLEAN
AtapiFindDevices(PATAPI_MINIPORT_EXTENSION DeviceExtension,
		 PPORT_CONFIGURATION_INFORMATION ConfigInfo)
{
  BOOLEAN DeviceFound = FALSE;
  ULONG CommandPortBase;
  ULONG ControlPortBase;
  ULONG UnitNumber;
  ULONG Retries;
  UCHAR High;
  UCHAR Low;

  DPRINT("AtapiFindDevices() called\n");

  CommandPortBase = ScsiPortConvertPhysicalAddressToUlong(ConfigInfo->AccessRanges[0].RangeStart);
  DPRINT("  CommandPortBase: %x\n", CommandPortBase);

  ControlPortBase = ScsiPortConvertPhysicalAddressToUlong(ConfigInfo->AccessRanges[1].RangeStart);
  DPRINT("  ControlPortBase: %x\n", ControlPortBase);

  for (UnitNumber = 0; UnitNumber < 2; UnitNumber++)
    {
      /* Select drive */
      IDEWriteDriveHead(CommandPortBase,
			IDE_DH_FIXED | (UnitNumber ? IDE_DH_DRV1 : 0));
      ScsiPortStallExecution(500);

      /* Disable interrupts */
      IDEWriteDriveControl(ControlPortBase,
			   IDE_DC_nIEN);
      ScsiPortStallExecution(500);

      /* Check if a device is attached to the interface */
      IDEWriteCylinderHigh(CommandPortBase, 0xaa);
      IDEWriteCylinderLow(CommandPortBase, 0x55);

      High = IDEReadCylinderHigh(CommandPortBase);
      Low = IDEReadCylinderLow(CommandPortBase);

      IDEWriteCylinderHigh(CommandPortBase, 0);
      IDEWriteCylinderLow(CommandPortBase, 0);

      if (Low != 0x55 || High != 0xaa)
	{
	  DPRINT("No Drive found. UnitNumber %d CommandPortBase %x\n", UnitNumber, CommandPortBase);
	  continue;
	}

      AtapiExecuteCommand(DeviceExtension, IDE_CMD_RESET, NULL);

      for (Retries = 0; Retries < 20000; Retries++)
	{
	  if (!(IDEReadStatus(CommandPortBase) & IDE_SR_BUSY))
	    {
	      break;
	    }
	  ScsiPortStallExecution(150);
	}
      if (Retries >= 20000)
	{
	  DPRINT("Timeout on drive %lu\n", UnitNumber);
	  DeviceExtension->DeviceFlags[UnitNumber] &= ~DEVICE_PRESENT;
	  continue;
	}

      High = IDEReadCylinderHigh(CommandPortBase);
      Low = IDEReadCylinderLow(CommandPortBase);

      DPRINT("  Check drive %lu: High 0x%x Low 0x%x\n",
	     UnitNumber,
	     High,
	     Low);

      if (High == 0xEB && Low == 0x14)
	{
	  if (AtapiIdentifyDevice(CommandPortBase,
				  ControlPortBase,
				  UnitNumber,
				  TRUE,
				  &DeviceExtension->DeviceParams[UnitNumber]))
	    {
	      DPRINT("  ATAPI drive found!\n");
	      DeviceExtension->DeviceFlags[UnitNumber] |= DEVICE_PRESENT;
	      DeviceExtension->DeviceFlags[UnitNumber] |= DEVICE_ATAPI;
	      DeviceExtension->TransferSize[UnitNumber] =
		DeviceExtension->DeviceParams[UnitNumber].BytesPerSector;
	      DeviceFound = TRUE;
	    }
	  else
	    {
	      DPRINT("  No ATAPI drive found!\n");
	    }
	}
      else
	{
	  if (AtapiIdentifyDevice(CommandPortBase,
				  ControlPortBase,
				  UnitNumber,
				  FALSE,
				  &DeviceExtension->DeviceParams[UnitNumber]))
	    {
	      DPRINT("  IDE drive found!\n");
	      DeviceExtension->DeviceFlags[UnitNumber] |= DEVICE_PRESENT;
	      DeviceExtension->TransferSize[UnitNumber] = DeviceExtension->DeviceParams[UnitNumber].BytesPerSector;
	      if ((DeviceExtension->DeviceParams[UnitNumber].RWMultImplemented & 0x8000) && 
		  (DeviceExtension->DeviceParams[UnitNumber].RWMultImplemented & 0xff) &&
		  (DeviceExtension->DeviceParams[UnitNumber].RWMultCurrent & 0x100) &&
		  (DeviceExtension->DeviceParams[UnitNumber].RWMultCurrent & 0xff))
		{
		  DeviceExtension->TransferSize[UnitNumber] *= (DeviceExtension->DeviceParams[UnitNumber].RWMultCurrent & 0xff);
		  DeviceExtension->DeviceFlags[UnitNumber] |= DEVICE_MULTI_SECTOR_CMD;
		}
	      if (DeviceExtension->DeviceParams[UnitNumber].SupportedFeatures83 & 0x0400 &&
		  DeviceExtension->DeviceParams[UnitNumber].EnabledFeatures86 & 0x0400)
	        {
		  DeviceExtension->DeviceFlags[UnitNumber] |= DEVICE_48BIT_ADDRESS;
		}
	      if (DeviceExtension->DeviceParams[UnitNumber].DWordIo)
		{
		  DeviceExtension->DeviceFlags[UnitNumber] |= DEVICE_DWORD_IO;
		}
#ifdef ENABLE_DMA
	      if ((DeviceExtension->DeviceParams[UnitNumber].TMFieldsValid & 0x0004) &&
		  ((DeviceExtension->DeviceParams[UnitNumber].MultiDmaModes & 0x0700) ||
		   (DeviceExtension->DeviceParams[UnitNumber].UltraDmaModes & 0xFF00)))
	        {
                  DeviceExtension->DeviceFlags[UnitNumber] |= DEVICE_DMA_CMD;		  
		}
#endif
	      DeviceFound = TRUE;
	    }
	  else
	    {
	      DPRINT("  No IDE drive found!\n");
	    }
	}
    }

  /* Reset pending interrupts */
  IDEReadStatus(CommandPortBase);
  /* Reenable interrupts */
  IDEWriteDriveControl(ControlPortBase, 0);
  ScsiPortStallExecution(500);
  /* Return with drive 0 selected */
  IDEWriteDriveHead(CommandPortBase, IDE_DH_FIXED);
  ScsiPortStallExecution(500);

  DPRINT("AtapiFindDrives() done (DeviceFound %s)\n", (DeviceFound) ? "TRUE" : "FALSE");

  return(DeviceFound);
}


/*
 *  AtapiIdentifyDevice
 *
 *  DESCRIPTION:
 *	Get the identification block from the drive
 *
 *  RUN LEVEL:
 *	PASSIVE_LEVEL
 *
 *  ARGUMENTS:
 *	CommandPort
 *		Address of the command port
 *	ControlPort
 *		Address of the control port
 *	DriveNum
 *		The drive index (0,1)
 *	Atapi
 *		Send an ATA(FALSE) or an ATAPI(TRUE) identify comand
 *	DrvParms
 *		Address to write drive ident block
 *
 *  RETURNS:
 *	TRUE: The drive identification block was retrieved successfully
 *	FALSE: an error ocurred
 */

static BOOLEAN
AtapiIdentifyDevice(IN ULONG CommandPort,
		    IN ULONG ControlPort,
		    IN ULONG DriveNum,
		    IN BOOLEAN Atapi,
		    OUT PIDE_DRIVE_IDENTIFY DrvParms)
{
  LONG i;
  ULONG mode;

  /*  Get the Drive Identify block from drive or die  */
  if (AtapiPolledRead(CommandPort,
		      ControlPort,
		      0,
		      1,
		      0,
		      0,
		      0,
		      (DriveNum ? IDE_DH_DRV1 : 0),
		      (Atapi ? IDE_CMD_IDENT_ATAPI_DRV : IDE_CMD_IDENT_ATA_DRV),
		      (PUCHAR)DrvParms) == FALSE)
    {
      DPRINT("AtapiPolledRead() failed\n");
      return FALSE;
    }

  /*  Report on drive parameters if debug mode  */
  IDESwapBytePairs(DrvParms->SerialNumber, 20);
  IDESwapBytePairs(DrvParms->FirmwareRev, 8);
  IDESwapBytePairs(DrvParms->ModelNumber, 40);
  DPRINT("Config:%04x  Cyls:%5d  Heads:%2d  Sectors/Track:%3d  Gaps:%02d %02d\n",
         DrvParms->ConfigBits,
         DrvParms->LogicalCyls,
         DrvParms->LogicalHeads,
         DrvParms->SectorsPerTrack,
         DrvParms->InterSectorGap,
         DrvParms->InterSectorGapSize);
  DPRINT("Bytes/PLO:%3d  Vendor Cnt:%2d  Serial number:[%.20s]\n",
         DrvParms->BytesInPLO,
         DrvParms->VendorUniqueCnt,
         DrvParms->SerialNumber);
  DPRINT("Cntlr type:%2d  BufSiz:%5d  ECC bytes:%3d  Firmware Rev:[%.8s]\n",
         DrvParms->ControllerType,
         DrvParms->BufferSize * IDE_SECTOR_BUF_SZ,
         DrvParms->ECCByteCnt,
         DrvParms->FirmwareRev);
  DPRINT("Model:[%.40s]\n", DrvParms->ModelNumber);
  DPRINT("RWMultMax?:%04x  RWMult?:%02x  LBA:%d  DMA:%d  MinPIO:%d ns  MinDMA:%d ns\n",
         (DrvParms->RWMultImplemented),
	 (DrvParms->RWMultCurrent) & 0xff,
         (DrvParms->Capabilities & IDE_DRID_LBA_SUPPORTED) ? 1 : 0,
         (DrvParms->Capabilities & IDE_DRID_DMA_SUPPORTED) ? 1 : 0,
         DrvParms->MinPIOTransTime,
         DrvParms->MinDMATransTime);
  DPRINT("TM:Cyls:%d  Heads:%d  Sectors/Trk:%d Capacity:%ld\n",
         DrvParms->TMCylinders,
         DrvParms->TMHeads,
         DrvParms->TMSectorsPerTrk,
         (ULONG)(DrvParms->TMCapacityLo + (DrvParms->TMCapacityHi << 16)));
  DPRINT("TM:SectorCount: 0x%04x%04x = %lu\n",
         DrvParms->TMSectorCountHi,
         DrvParms->TMSectorCountLo,
         (ULONG)((DrvParms->TMSectorCountHi << 16) + DrvParms->TMSectorCountLo));
  if (DrvParms->TMFieldsValid & 0x0004)
    {
      if ((DrvParms->UltraDmaModes >> 8) && (DrvParms->UltraDmaModes & 0xff))
        {
	  mode = 7;
	  while (!(DrvParms->UltraDmaModes & (0x0100 << mode)))
	    {
	      mode--;
	    }
	  DPRINT("Ultra DMA mode %d is selected\n", mode);
	}
      else if ((DrvParms->MultiDmaModes >> 8) & (DrvParms->MultiDmaModes & 0x07))
        {
	  mode = 2;
	  while(!(DrvParms->MultiDmaModes & (0x01 << mode)))
	    {
	      mode--;
	    }
	  DPRINT("Multi DMA mode %d is selected\n", mode);
	}
    }

  if (! Atapi && 0 != (DrvParms->Capabilities & IDE_DRID_LBA_SUPPORTED))
    {
      /* LBA ATA drives always have a sector size of 512 */
      DrvParms->BytesPerSector = 512;
    }
  else
    {
      DPRINT("BytesPerSector %d\n", DrvParms->BytesPerSector);
      if (DrvParms->BytesPerSector == 0)
        {
          DrvParms->BytesPerSector = 512;
        }
      else
        {
          for (i = 15; i >= 0; i--)
            {
              if (DrvParms->BytesPerSector & (1 << i))
                {
                  DrvParms->BytesPerSector = 1 << i;
                  break;
                }
            }
        }
    }
  DPRINT("BytesPerSector %d\n", DrvParms->BytesPerSector);

  return TRUE;
}


//    AtapiPolledRead
//
//  DESCRIPTION:
//    Read a sector of data from the drive in a polled fashion.
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN   ULONG  CommandPort   Address of command port for drive
//    IN   ULONG  ControlPort   Address of control port for drive
//    IN   UCHAR  PreComp       Value to write to precomp register
//    IN   UCHAR  SectorCnt     Value to write to sectorCnt register
//    IN   UCHAR  SectorNum     Value to write to sectorNum register
//    IN   UCHAR  CylinderLow   Value to write to CylinderLow register
//    IN   UCHAR  CylinderHigh  Value to write to CylinderHigh register
//    IN   UCHAR  DrvHead       Value to write to Drive/Head register
//    IN   UCHAR  Command       Value to write to Command register
//    OUT  PUCHAR Buffer        Buffer for output data
//
//  RETURNS:
//    BOOLEAN: TRUE success, FALSE error
//

static BOOLEAN
AtapiPolledRead(IN ULONG CommandPort,
		IN ULONG ControlPort,
		IN UCHAR PreComp,
		IN UCHAR SectorCnt,
		IN UCHAR SectorNum,
		IN UCHAR CylinderLow,
		IN UCHAR CylinderHigh,
		IN UCHAR DrvHead,
		IN UCHAR Command,
		OUT PUCHAR Buffer)
{
  ULONG SectorCount = 0;
  ULONG RetryCount;
  BOOLEAN Junk = FALSE;
  UCHAR Status;

  /* Wait for BUSY to clear */
  for (RetryCount = 0; RetryCount < IDE_MAX_BUSY_RETRIES; RetryCount++)
    {
      Status = IDEReadStatus(CommandPort);
      if (!(Status & IDE_SR_BUSY))
        {
          break;
        }
      ScsiPortStallExecution(10);
    }
  DPRINT("status=%02x\n", Status);
  DPRINT("waited %ld usecs for busy to clear\n", RetryCount * 10);
  if (RetryCount >= IDE_MAX_BUSY_RETRIES)
    {
      DPRINT("Drive is BUSY for too long\n");
      return FALSE;
    }

  /*  Write Drive/Head to select drive  */
  IDEWriteDriveHead(CommandPort, IDE_DH_FIXED | DrvHead);
  ScsiPortStallExecution(500);

  /* Disable interrupts */
  IDEWriteDriveControl(ControlPort, IDE_DC_nIEN);
  ScsiPortStallExecution(500);

#if 0
  /*  Wait for STATUS.BUSY and STATUS.DRQ to clear  */
  for (RetryCount = 0; RetryCount < IDE_MAX_BUSY_RETRIES; RetryCount++)
    {
      Status = IDEReadStatus(CommandPort);
      if (!(Status & IDE_SR_BUSY) && !(Status & IDE_SR_DRQ))
	{
	  break;
	}
      ScsiPortStallExecution(10);
    }
  if (RetryCount >= IDE_MAX_BUSY_RETRIES)
    {
      return FALSE;
    }
#endif

  /*  Issue command to drive  */
  if (DrvHead & IDE_DH_LBA)
    {
      DPRINT("READ:DRV=%d:LBA=1:BLK=%08d:SC=%02x:CM=%02x\n",
	     DrvHead & IDE_DH_DRV1 ? 1 : 0,
	     ((DrvHead & 0x0f) << 24) + (CylinderHigh << 16) + (CylinderLow << 8) + SectorNum,
	     SectorCnt,
	     Command);
    }
  else
    {
      DPRINT("READ:DRV=%d:LBA=0:CH=%02x:CL=%02x:HD=%01x:SN=%02x:SC=%02x:CM=%02x\n",
	     DrvHead & IDE_DH_DRV1 ? 1 : 0,
	     CylinderHigh,
	     CylinderLow,
	     DrvHead & 0x0f,
	     SectorNum,
	     SectorCnt,
	     Command);
    }

  /*  Setup command parameters  */
  IDEWritePrecomp(CommandPort, PreComp);
  IDEWriteSectorCount(CommandPort, SectorCnt);
  IDEWriteSectorNum(CommandPort, SectorNum);
  IDEWriteCylinderHigh(CommandPort, CylinderHigh);
  IDEWriteCylinderLow(CommandPort, CylinderLow);
  IDEWriteDriveHead(CommandPort, IDE_DH_FIXED | DrvHead);

  /*  Issue the command  */
  IDEWriteCommand(CommandPort, Command);
  ScsiPortStallExecution(50);

  /*  wait for DRQ or error  */
  for (RetryCount = 0; RetryCount < IDE_MAX_POLL_RETRIES; RetryCount++)
    {
      Status = IDEReadStatus(CommandPort);
      if (!(Status & IDE_SR_BUSY))
	{
	  if (Status & IDE_SR_ERR)
	    {
	      IDEWriteDriveControl(ControlPort, 0);
	      ScsiPortStallExecution(50);
	      IDEReadStatus(CommandPort);

	      return FALSE;
	    }

	  if (Status & IDE_SR_DRQ)
	    {
	      break;
	    }
	  else
	    {
	      IDEWriteDriveControl(ControlPort, 0);
	      ScsiPortStallExecution(50);
	      IDEReadStatus(CommandPort);

	      return FALSE;
	    }
	}
      ScsiPortStallExecution(10);
    }

  /*  timed out  */
  if (RetryCount >= IDE_MAX_POLL_RETRIES)
    {
      IDEWriteDriveControl(ControlPort, 0);
      ScsiPortStallExecution(50);
      IDEReadStatus(CommandPort);

      return FALSE;
    }

  while (1)
    {
      /*  Read data into buffer  */
      if (Junk == FALSE)
	{
	  IDEReadBlock(CommandPort, Buffer, IDE_SECTOR_BUF_SZ);
	  Buffer += IDE_SECTOR_BUF_SZ;
	}
      else
	{
	  UCHAR JunkBuffer[IDE_SECTOR_BUF_SZ];
	  IDEReadBlock(CommandPort, JunkBuffer, IDE_SECTOR_BUF_SZ);
	}
      SectorCount++;

      /*  Check for error or more sectors to read  */
      for (RetryCount = 0; RetryCount < IDE_MAX_BUSY_RETRIES; RetryCount++)
	{
	  Status = IDEReadStatus(CommandPort);
	  if (!(Status & IDE_SR_BUSY))
	    {
	      if (Status & IDE_SR_ERR)
		{
		  IDEWriteDriveControl(ControlPort, 0);
		  ScsiPortStallExecution(50);
		  IDEReadStatus(CommandPort);

		  return FALSE;
		}
	      if (Status & IDE_SR_DRQ)
		{
		  if (SectorCount >= SectorCnt)
		    {
		      DPRINT("Buffer size exceeded!\n");
		      Junk = TRUE;
		    }
		  break;
		}
	      else
		{
		  if (SectorCount > SectorCnt)
		    {
		      DPRINT("Read %lu sectors of junk!\n",
			     SectorCount - SectorCnt);
		    }
		  IDEWriteDriveControl(ControlPort, 0);
		  ScsiPortStallExecution(50);
		  IDEReadStatus(CommandPort);

		  return TRUE;
		}
	    }
	}
    }
}


//  -------------------------------------------  Nondiscardable statics

static ULONG
AtapiSendAtapiCommand(IN PATAPI_MINIPORT_EXTENSION DeviceExtension,
		      IN PSCSI_REQUEST_BLOCK Srb)
{
  UCHAR ByteCountHigh;
  UCHAR ByteCountLow;
  ULONG Retries;
  ULONG CdbSize;
  UCHAR Status;

  DPRINT("AtapiSendAtapiCommand() called!\n");

  if (Srb->PathId != 0)
    {
      Srb->SrbStatus = SRB_STATUS_INVALID_PATH_ID;
      return(SRB_STATUS_INVALID_PATH_ID);
    }

  if (Srb->TargetId > 1)
    {
      Srb->SrbStatus = SRB_STATUS_INVALID_TARGET_ID;
      return(SRB_STATUS_INVALID_TARGET_ID);
    }

  if (Srb->Lun != 0)
    {
      Srb->SrbStatus = SRB_STATUS_INVALID_LUN;
      return(SRB_STATUS_INVALID_LUN);
    }

  if (!(DeviceExtension->DeviceFlags[Srb->TargetId] & DEVICE_PRESENT))
    {
      Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
      return(SRB_STATUS_NO_DEVICE);
    }

  if (Srb->Cdb[0] == SCSIOP_MODE_SENSE)
    {
      Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
      return (SRB_STATUS_INVALID_REQUEST);
    }

  DPRINT("AtapiSendAtapiCommand(): TargetId: %lu\n",
	 Srb->TargetId);

  if (Srb->Cdb[0] == SCSIOP_INQUIRY)
    return(AtapiInquiry(DeviceExtension,
			Srb));

  /* Set pointer to data buffer. */
  DeviceExtension->DataBuffer = (PUCHAR)Srb->DataBuffer;
  DeviceExtension->DataTransferLength = Srb->DataTransferLength;
  DeviceExtension->CurrentSrb = Srb;

  DPRINT("BufferAddress %x, BufferLength %d\n", Srb->DataBuffer, Srb->DataTransferLength);

  /* Wait for BUSY to clear */
  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES; Retries++)
    {
      Status = IDEReadStatus(DeviceExtension->CommandPortBase);
      if (!(Status & IDE_SR_BUSY))
        {
          break;
        }
      ScsiPortStallExecution(10);
    }
  DPRINT("status=%02x\n", Status);
  DPRINT("waited %ld usecs for busy to clear\n", Retries * 10);
  if (Retries >= IDE_MAX_BUSY_RETRIES)
    {
      DPRINT("Drive is BUSY for too long\n");
      return(SRB_STATUS_BUSY);
    }

  /* Select the desired drive */
  IDEWriteDriveHead(DeviceExtension->CommandPortBase,
		    IDE_DH_FIXED | (Srb->TargetId ? IDE_DH_DRV1 : 0));

  /* Wait a little while */
  ScsiPortStallExecution(50);

#if 0
  /* Wait for BUSY to clear and DRDY to assert */
  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES; Retries++)
    {
      Status = IDEReadStatus(DeviceExtension->CommandPortBase);
      if (!(Status & IDE_SR_BUSY) && (Status & IDE_SR_DRDY))
	{
	  break;
	}
      ScsiPortStallExecution(10);
    }
  DPRINT("waited %ld usecs for busy to clear after drive select\n", Retries * 10);
  if (Retries >= IDE_MAX_BUSY_RETRIES)
    {
      DPRINT("Drive is BUSY for too long after drive select\n");
      return(SRB_STATUS_BUSY);
    }
#endif

  if (DeviceExtension->DataTransferLength < 0x10000)
    {
      ByteCountLow = (UCHAR)(DeviceExtension->DataTransferLength & 0xFF);
      ByteCountHigh = (UCHAR)(DeviceExtension->DataTransferLength >> 8);
    }
  else
    {
      ByteCountLow = 0xFF;
      ByteCountHigh = 0xFF;
    }

  /* Set feature register */
  IDEWritePrecomp(DeviceExtension->CommandPortBase, 0);

  /* Set command packet length */
  IDEWriteCylinderHigh(DeviceExtension->CommandPortBase, ByteCountHigh);
  IDEWriteCylinderLow(DeviceExtension->CommandPortBase, ByteCountLow);

  /* Issue command to drive */
  AtapiExecuteCommand(DeviceExtension, IDE_CMD_PACKET, AtapiPacketInterrupt);

  /* Wait for DRQ to assert */
  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES; Retries++)
    {
      Status = IDEReadStatus(DeviceExtension->CommandPortBase);
      if ((Status & IDE_SR_DRQ))
	{
	  break;
	}
      ScsiPortStallExecution(10);
    }

  /* Convert special SCSI SRBs to ATAPI format */
  switch (Srb->Cdb[0])
    {
      case SCSIOP_FORMAT_UNIT:
      case SCSIOP_MODE_SELECT:
	AtapiScsiSrbToAtapi (Srb);
	break;
    }

  CdbSize = ((DeviceExtension->DeviceParams[Srb->TargetId].ConfigBits & 0x3) == 1) ? 16 : 12;
  DPRINT("CdbSize: %lu\n", CdbSize);

  /* Write command packet */
  IDEWriteBlock(DeviceExtension->CommandPortBase,
		(PUSHORT)Srb->Cdb,
		CdbSize);

  DPRINT("AtapiSendAtapiCommand() done\n");

  return(SRB_STATUS_PENDING);
}


static ULONG
AtapiSendIdeCommand(IN PATAPI_MINIPORT_EXTENSION DeviceExtension,
		    IN PSCSI_REQUEST_BLOCK Srb)
{
  ULONG SrbStatus = SRB_STATUS_SUCCESS;

  DPRINT("AtapiSendIdeCommand() called!\n");

  DPRINT("PathId: %lu  TargetId: %lu  Lun: %lu\n",
	 Srb->PathId,
	 Srb->TargetId,
	 Srb->Lun);

  if (Srb->PathId != 0)
    {
      Srb->SrbStatus = SRB_STATUS_INVALID_PATH_ID;
      return(SRB_STATUS_INVALID_PATH_ID);
    }

  if (Srb->TargetId > 1)
    {
      Srb->SrbStatus = SRB_STATUS_INVALID_TARGET_ID;
      return(SRB_STATUS_INVALID_TARGET_ID);
    }

  if (Srb->Lun != 0)
    {
      Srb->SrbStatus = SRB_STATUS_INVALID_LUN;
      return(SRB_STATUS_INVALID_LUN);
    }

  if (!(DeviceExtension->DeviceFlags[Srb->TargetId] & DEVICE_PRESENT))
    {
      Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
      return(SRB_STATUS_NO_DEVICE);
    }

  switch (Srb->Cdb[0])
    {
      case SCSIOP_INQUIRY:
	SrbStatus = AtapiInquiry(DeviceExtension,
				 Srb);
	break;

      case SCSIOP_READ_CAPACITY:
	SrbStatus = AtapiReadCapacity(DeviceExtension,
				      Srb);
	break;

      case SCSIOP_READ:
      case SCSIOP_WRITE:
	SrbStatus = AtapiReadWrite(DeviceExtension,
				   Srb);
	break;

      case SCSIOP_SYNCHRONIZE_CACHE:
	SrbStatus = AtapiFlushCache(DeviceExtension,
				    Srb);
	break;

      case SCSIOP_TEST_UNIT_READY:
	SrbStatus = AtapiTestUnitReady(DeviceExtension,
				       Srb);
	break;

      case SCSIOP_MODE_SENSE:

      case SCSIOP_VERIFY:
      case SCSIOP_START_STOP_UNIT:
      case SCSIOP_REQUEST_SENSE:
	break;

     default:
	DbgPrint("AtapiSendIdeCommand():unknown command %x\n",
		 Srb->Cdb[0]);
	SrbStatus = SRB_STATUS_INVALID_REQUEST;
	break;
    }

  DPRINT("AtapiSendIdeCommand() done!\n");

  return(SrbStatus);
}


static ULONG
AtapiInquiry(PATAPI_MINIPORT_EXTENSION DeviceExtension,
	     PSCSI_REQUEST_BLOCK Srb)
{
  PIDE_DRIVE_IDENTIFY DeviceParams;
  PINQUIRYDATA InquiryData;
  ULONG i;

  DPRINT("SCSIOP_INQUIRY: DeviceExtension %p  TargetId: %lu\n",
	 DeviceExtension, Srb->TargetId);

  InquiryData = Srb->DataBuffer;
  DeviceParams = &DeviceExtension->DeviceParams[Srb->TargetId];

  /* clear buffer */
  for (i = 0; i < Srb->DataTransferLength; i++)
    {
      ((PUCHAR)Srb->DataBuffer)[i] = 0;
    }

  /* set device class */
  if (DeviceExtension->DeviceFlags[Srb->TargetId] & DEVICE_ATAPI)
    {
      /* get it from the ATAPI configuration word */
      InquiryData->DeviceType = (DeviceParams->ConfigBits >> 8) & 0x1F;
      DPRINT("Device class: %u\n", InquiryData->DeviceType);
    }
  else
    {
      /* hard-disk */
      InquiryData->DeviceType = DIRECT_ACCESS_DEVICE;
    }

  DPRINT("ConfigBits: 0x%x\n", DeviceParams->ConfigBits);
  if (DeviceParams->ConfigBits & 0x80)
    {
      DPRINT("Removable media!\n");
      InquiryData->RemovableMedia = 1;
    }

  for (i = 0; i < 20; i += 2)
    {
      InquiryData->VendorId[i] =
	((PUCHAR)DeviceParams->ModelNumber)[i];
      InquiryData->VendorId[i+1] =
	((PUCHAR)DeviceParams->ModelNumber)[i+1];
    }

  for (i = 0; i < 4; i++)
    {
      InquiryData->ProductId[12+i] = ' ';
    }

  for (i = 0; i < 4; i += 2)
    {
      InquiryData->ProductRevisionLevel[i] =
	((PUCHAR)DeviceParams->FirmwareRev)[i];
      InquiryData->ProductRevisionLevel[i+1] =
	((PUCHAR)DeviceParams->FirmwareRev)[i+1];
    }

  DPRINT("VendorId: '%.20s'\n", InquiryData->VendorId);

  Srb->SrbStatus = SRB_STATUS_SUCCESS;
  return(SRB_STATUS_SUCCESS);
}


static ULONG
AtapiReadCapacity(PATAPI_MINIPORT_EXTENSION DeviceExtension,
		  PSCSI_REQUEST_BLOCK Srb)
{
  PREAD_CAPACITY_DATA CapacityData;
  PIDE_DRIVE_IDENTIFY DeviceParams;
  ULONG LastSector;

  DPRINT("SCSIOP_READ_CAPACITY: TargetId: %lu\n", Srb->TargetId);
  CapacityData = (PREAD_CAPACITY_DATA)Srb->DataBuffer;
  DeviceParams = &DeviceExtension->DeviceParams[Srb->TargetId];

  /* Set sector (block) size to 512 bytes (big-endian). */
  CapacityData->BytesPerBlock = 0x20000;

  /* Calculate last sector (big-endian). */
  if (DeviceParams->Capabilities & IDE_DRID_LBA_SUPPORTED)
    {
      LastSector = (ULONG)((DeviceParams->TMSectorCountHi << 16) +
			    DeviceParams->TMSectorCountLo) - 1;
    }
  else
    {
      LastSector = (ULONG)(DeviceParams->LogicalCyls *
			   DeviceParams->LogicalHeads *
			   DeviceParams->SectorsPerTrack)-1;
    }

  CapacityData->LogicalBlockAddress = (((PUCHAR)&LastSector)[0] << 24) |
				      (((PUCHAR)&LastSector)[1] << 16) |
				      (((PUCHAR)&LastSector)[2] << 8) |
				      ((PUCHAR)&LastSector)[3];

  DPRINT("LastCount: %lu (%08lx / %08lx)\n",
	 LastSector,
	 LastSector,
	 CapacityData->LogicalBlockAddress);

  Srb->SrbStatus = SRB_STATUS_SUCCESS;
  return(SRB_STATUS_SUCCESS);
}


static ULONG
AtapiReadWrite(PATAPI_MINIPORT_EXTENSION DeviceExtension,
	       PSCSI_REQUEST_BLOCK Srb)
{
  PIDE_DRIVE_IDENTIFY DeviceParams;
  ULONG StartingSector;
  ULONG SectorCount;
  UCHAR CylinderHigh;
  UCHAR CylinderLow;
  UCHAR DrvHead;
  UCHAR SectorNumber;
  UCHAR Command;
  ULONG Retries;
  UCHAR Status;
  BOOLEAN FASTCALL (*Handler)(PATAPI_MINIPORT_EXTENSION DevExt);

  DPRINT("AtapiReadWrite() called!\n");
  DPRINT("SCSIOP_WRITE: TargetId: %lu\n",
	 Srb->TargetId);

  DeviceParams = &DeviceExtension->DeviceParams[Srb->TargetId];

  /* Get starting sector number from CDB. */
  StartingSector = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3 |
		   ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2 << 8 |
		   ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1 << 16 |
		   ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 24;

  SectorCount = (Srb->DataTransferLength + DeviceParams->BytesPerSector - 1) /
		DeviceParams->BytesPerSector;

  DPRINT("Starting sector %lu  Number of bytes %lu  Sectors %lu\n",
	 StartingSector,
	 Srb->DataTransferLength,
	 SectorCount);

  if (DeviceParams->Capabilities & IDE_DRID_LBA_SUPPORTED)
    {
      SectorNumber = StartingSector & 0xff;
      CylinderLow = (StartingSector >> 8) & 0xff;
      CylinderHigh = (StartingSector >> 16) & 0xff;
      DrvHead = ((StartingSector >> 24) & 0x0f) | 
          (Srb->TargetId ? IDE_DH_DRV1 : 0) |
          IDE_DH_LBA;
    }
  else
    {
      SectorNumber = (StartingSector % DeviceParams->SectorsPerTrack) + 1;
      StartingSector /= DeviceParams->SectorsPerTrack;
      DrvHead = (StartingSector % DeviceParams->LogicalHeads) | 
          (Srb->TargetId ? IDE_DH_DRV1 : 0);
      StartingSector /= DeviceParams->LogicalHeads;
      CylinderLow = StartingSector & 0xff;
      CylinderHigh = StartingSector >> 8;
    }

  if (DrvHead & IDE_DH_LBA)
    {
      DPRINT("%s:BUS=%04x:DRV=%d:LBA=1:BLK=%08d:SC=%02x:CM=%02x\n",
	     (Srb->SrbFlags & SRB_FLAGS_DATA_IN) ? "READ" : "WRITE",
	     DeviceExtension->CommandPortBase,
	     DrvHead & IDE_DH_DRV1 ? 1 : 0,
	     ((DrvHead & 0x0f) << 24) +
	       (CylinderHigh << 16) + (CylinderLow << 8) + SectorNumber,
	     SectorCount,
	     Command);
    }
  else
    {
      DPRINT("%s:BUS=%04x:DRV=%d:LBA=0:CH=%02x:CL=%02x:HD=%01x:SN=%02x:SC=%02x:CM=%02x\n",
             (Srb->SrbFlags & SRB_FLAGS_DATA_IN) ? "READ" : "WRITE",
             DeviceExtension->CommandPortBase,
             DrvHead & IDE_DH_DRV1 ? 1 : 0, 
             CylinderHigh,
             CylinderLow,
             DrvHead & 0x0f,
             SectorNumber,
             SectorCount,
             Command);
    }

  /* Set pointer to data buffer. */
  DeviceExtension->DataBuffer = (PUCHAR)Srb->DataBuffer;
  DeviceExtension->DataTransferLength = Srb->DataTransferLength;

  DeviceExtension->CurrentSrb = Srb;

  /*  wait for BUSY to clear  */
  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES; Retries++)
    {
      Status = IDEReadStatus(DeviceExtension->CommandPortBase);
      if (!(Status & IDE_SR_BUSY))
        {
          break;
        }
      ScsiPortStallExecution(10);
    }
  DPRINT("status=%02x\n", Status);
  DPRINT("waited %ld usecs for busy to clear\n", Retries * 10);
  if (Retries >= IDE_MAX_BUSY_RETRIES)
    {
      DPRINT ("Drive is BUSY for too long\n");
      return(SRB_STATUS_BUSY);
    }

  /*  Select the desired drive  */
  IDEWriteDriveHead(DeviceExtension->CommandPortBase,
		    IDE_DH_FIXED | DrvHead);

  ScsiPortStallExecution(10);
#if 0
  /*  wait for BUSY to clear and DRDY to assert */
  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES; Retries++)
    {
      Status = IDEReadStatus(DeviceExtension->CommandPortBase);
      if (!(Status & IDE_SR_BUSY) && (Status & IDE_SR_DRDY))
	{
	  break;
	}
      ScsiPortStallExecution(10);
    }
  DPRINT("waited %ld usecs for busy to clear after drive select\n", Retries * 10);
  if (Retries >= IDE_MAX_BUSY_RETRIES)
    {
      DPRINT("Drive is BUSY for too long after drive select\n");
      return(SRB_STATUS_BUSY);
    }
#endif

  /* Setup command parameters */
  IDEWritePrecomp(DeviceExtension->CommandPortBase, 0);
  IDEWriteSectorCount(DeviceExtension->CommandPortBase, SectorCount);
  IDEWriteSectorNum(DeviceExtension->CommandPortBase, SectorNumber);
  IDEWriteCylinderHigh(DeviceExtension->CommandPortBase, CylinderHigh);
  IDEWriteCylinderLow(DeviceExtension->CommandPortBase, CylinderLow);
  IDEWriteDriveHead(DeviceExtension->CommandPortBase, IDE_DH_FIXED | DrvHead);

#ifdef ENABLE_DMA
  if (DeviceExtension->PRDTable && 
      DeviceExtension->DeviceFlags[Srb->TargetId] & DEVICE_DMA_CMD)
    {
      DeviceExtension->UseDma = AtapiInitDma(DeviceExtension, Srb, Srb->SrbFlags & SRB_FLAGS_DATA_IN ? 1 << 3 : 0);
    }
  if (DeviceExtension->UseDma)
    {
      Handler = AtapiDmaInterrupt;
      Command = Srb->SrbFlags & SRB_FLAGS_DATA_IN ? IDE_CMD_READ_DMA : IDE_CMD_WRITE_DMA;
    }
  else
#endif
    {
      Handler = Srb->SrbFlags & SRB_FLAGS_DATA_IN ? AtapiReadInterrupt : AtapiWriteInterrupt;
      if (DeviceExtension->DeviceFlags[Srb->TargetId] & DEVICE_MULTI_SECTOR_CMD)
        {
          Command = Srb->SrbFlags & SRB_FLAGS_DATA_IN ? IDE_CMD_READ_MULTIPLE : IDE_CMD_WRITE_MULTIPLE;
	}
      else
        {
          Command = Srb->SrbFlags & SRB_FLAGS_DATA_IN ? IDE_CMD_READ : IDE_CMD_WRITE;
	}
    }

  AtapiExecuteCommand(DeviceExtension, Command, Handler);

#ifdef ENABLE_DMA
  if (DeviceExtension->UseDma)
    {
      BYTE DmaCommand;
      /* start DMA */
      DmaCommand = IDEReadDMACommand(DeviceExtension->BusMasterRegisterBase);
      IDEWriteDMACommand(DeviceExtension->BusMasterRegisterBase, DmaCommand|0x01);
    }
  else 
#endif
    {
      if (Srb->SrbFlags & SRB_FLAGS_DATA_OUT)
        {
          /* Write data block */
          PUCHAR TargetAddress;
          ULONG TransferSize;

          /* Wait for controller ready */
          for (Retries = 0; Retries < IDE_MAX_WRITE_RETRIES; Retries++)
	    {
	      BYTE  Status = IDEReadStatus(DeviceExtension->CommandPortBase);
	      if (!(Status & IDE_SR_BUSY) || (Status & IDE_SR_ERR))
	        {
	          break;
	        }
	      ScsiPortStallExecution(10);
	    }
          if (Retries >= IDE_MAX_WRITE_RETRIES)
	    {
	      DPRINT1("Drive is BUSY for too long after sending write command\n");
	      return(SRB_STATUS_BUSY);
	    }

          /* Update DeviceExtension data */
          TransferSize = DeviceExtension->TransferSize[Srb->TargetId];
          if (DeviceExtension->DataTransferLength < TransferSize)
	    {
	      TransferSize = DeviceExtension->DataTransferLength;
	    }

          TargetAddress = DeviceExtension->DataBuffer;
          DeviceExtension->DataBuffer += TransferSize;
          DeviceExtension->DataTransferLength -= TransferSize;

          /* Write data block */
          if (DeviceExtension->DeviceFlags[Srb->TargetId] & DEVICE_DWORD_IO)
	    {
	      IDEWriteBlock32(DeviceExtension->CommandPortBase,
			      TargetAddress,
			      TransferSize);
	    }
          else
	    {
	      IDEWriteBlock(DeviceExtension->CommandPortBase,
			    TargetAddress,
			    TransferSize);
	    }
	}
    }

  DPRINT("AtapiReadWrite() done!\n");

  /* Wait for interrupt. */
  return(SRB_STATUS_PENDING);
}


static ULONG
AtapiFlushCache(PATAPI_MINIPORT_EXTENSION DeviceExtension,
		PSCSI_REQUEST_BLOCK Srb)
{
  ULONG Retries;
  UCHAR Status;

  DPRINT("AtapiFlushCache() called!\n");
  DPRINT("SCSIOP_SYNCRONIZE_CACHE: TargetId: %lu\n",
	 Srb->TargetId);

  /* Wait for BUSY to clear */
  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES; Retries++)
    {
      Status = IDEReadStatus(DeviceExtension->CommandPortBase);
      if (!(Status & IDE_SR_BUSY))
        {
          break;
        }
      ScsiPortStallExecution(10);
    }
  DPRINT("Status=%02x\n", Status);
  DPRINT("Waited %ld usecs for busy to clear\n", Retries * 10);
  if (Retries >= IDE_MAX_BUSY_RETRIES)
    {
      DPRINT1("Drive is BUSY for too long\n");
      return(SRB_STATUS_BUSY);
    }

  /* Select the desired drive */
  IDEWriteDriveHead(DeviceExtension->CommandPortBase,
		    IDE_DH_FIXED | (Srb->TargetId ? IDE_DH_DRV1 : 0));
  ScsiPortStallExecution(10);

  /* Issue command to drive */
  AtapiExecuteCommand(DeviceExtension, IDE_CMD_FLUSH_CACHE, AtapiNoDataInterrupt); 

  /* Wait for controller ready */
  for (Retries = 0; Retries < IDE_MAX_WRITE_RETRIES; Retries++)
    {
      Status = IDEReadStatus(DeviceExtension->CommandPortBase);
      if (!(Status & IDE_SR_BUSY) || (Status & IDE_SR_ERR))
	{
	  break;
	}
      ScsiPortStallExecution(10);
    }
  if (Retries >= IDE_MAX_WRITE_RETRIES)
    {
      DPRINT1("Drive is BUSY for too long after sending write command\n");
      DeviceExtension->Handler = NULL;
      return(SRB_STATUS_BUSY);
    }

  DPRINT("AtapiFlushCache() done!\n");

  /* Wait for interrupt. */
  return(SRB_STATUS_PENDING);
}


static ULONG
AtapiTestUnitReady(PATAPI_MINIPORT_EXTENSION DeviceExtension,
		   PSCSI_REQUEST_BLOCK Srb)
{
  ULONG Retries;
  UCHAR Status;
  UCHAR Error;

  DPRINT1("AtapiTestUnitReady() called!\n");

  DPRINT1("SCSIOP_TEST_UNIT_READY: TargetId: %lu\n",
	 Srb->TargetId);

  /* Return success if media status is not supported */
  if (!(DeviceExtension->DeviceFlags[Srb->TargetId] & DEVICE_MEDIA_STATUS))
    {
      Srb->SrbStatus = SRB_STATUS_SUCCESS;
      return(SRB_STATUS_SUCCESS);
    }

  /* Wait for BUSY to clear */
  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES; Retries++)
    {
      Status = IDEReadStatus(DeviceExtension->CommandPortBase);
      if (!(Status & IDE_SR_BUSY))
        {
          break;
        }
      ScsiPortStallExecution(10);
    }
  DPRINT1("Status=%02x\n", Status);
  DPRINT1("Waited %ld usecs for busy to clear\n", Retries * 10);
  if (Retries >= IDE_MAX_BUSY_RETRIES)
    {
      DPRINT1("Drive is BUSY for too long\n");
      return(SRB_STATUS_BUSY);
    }

  /* Select the desired drive */
  IDEWriteDriveHead(DeviceExtension->CommandPortBase,
		    IDE_DH_FIXED | (Srb->TargetId ? IDE_DH_DRV1 : 0));
  ScsiPortStallExecution(10);

  /* Issue command to drive */
  AtapiExecuteCommand(DeviceExtension, IDE_CMD_GET_MEDIA_STATUS, AtapiNoDataInterrupt);

  /* Wait for controller ready */
  for (Retries = 0; Retries < IDE_MAX_WRITE_RETRIES; Retries++)
    {
      Status = IDEReadStatus(DeviceExtension->CommandPortBase);
      if (!(Status & IDE_SR_BUSY) || (Status & IDE_SR_ERR))
	{
	  break;
	}
      ScsiPortStallExecution(10);
    }
  if (Retries >= IDE_MAX_WRITE_RETRIES)
    {
      DPRINT1("Drive is BUSY for too long after sending write command\n");
      DeviceExtension->Handler = NULL;
      return(SRB_STATUS_BUSY);
    }

  if (Status & IDE_SR_ERR)
    {
      Error = IDEReadError(DeviceExtension->CommandPortBase);
      if (Error == IDE_ER_UNC)
	{
CHECKPOINT1;
	  /* Handle write protection 'error' */
	  Srb->SrbStatus = SRB_STATUS_SUCCESS;
          DeviceExtension->Handler = NULL;
	  return(SRB_STATUS_SUCCESS);
	}
      else
	{
CHECKPOINT1;
	  /* Indicate expecting an interrupt. */
	  return(SRB_STATUS_PENDING);
	}
    }

  DeviceExtension->Handler = NULL;

  DPRINT1("AtapiTestUnitReady() done!\n");

  Srb->SrbStatus = SRB_STATUS_SUCCESS;
  return(SRB_STATUS_SUCCESS);
}


static UCHAR
AtapiErrorToScsi(PVOID DeviceExtension,
		 PSCSI_REQUEST_BLOCK Srb)
{
  PATAPI_MINIPORT_EXTENSION DevExt;
  ULONG CommandPortBase;
  ULONG ControlPortBase;
  UCHAR ErrorReg;
  UCHAR ScsiStatus;
  UCHAR SrbStatus;

  DPRINT("AtapiErrorToScsi() called\n");

  DevExt = (PATAPI_MINIPORT_EXTENSION)DeviceExtension;

  CommandPortBase = DevExt->CommandPortBase;
  ControlPortBase = DevExt->ControlPortBase;

  ErrorReg = IDEReadError(CommandPortBase);

  if (DevExt->DeviceFlags[Srb->TargetId] & DEVICE_ATAPI)
    {
      switch (ErrorReg >> 4)
	{
	  case SCSI_SENSE_NO_SENSE:
	    DPRINT("ATAPI error: SCSI_SENSE_NO_SENSE\n");
	    ScsiStatus = SCSISTAT_CHECK_CONDITION;
	    SrbStatus = SRB_STATUS_ERROR;
	    break;

	  case SCSI_SENSE_RECOVERED_ERROR:
	    DPRINT("ATAPI error: SCSI_SENSE_RECOVERED_SENSE\n");
	    ScsiStatus = 0;
	    SrbStatus = SRB_STATUS_SUCCESS;
	    break;

	  case SCSI_SENSE_NOT_READY:
	    DPRINT("ATAPI error: SCSI_SENSE_NOT_READY\n");
	    ScsiStatus = SCSISTAT_CHECK_CONDITION;
	    SrbStatus = SRB_STATUS_ERROR;
	    break;

	  case SCSI_SENSE_MEDIUM_ERROR:
	    DPRINT("ATAPI error: SCSI_SENSE_MEDIUM_ERROR\n");
	    ScsiStatus = SCSISTAT_CHECK_CONDITION;
	    SrbStatus = SRB_STATUS_ERROR;
	    break;

	  case SCSI_SENSE_HARDWARE_ERROR:
	    DPRINT("ATAPI error: SCSI_SENSE_HARDWARE_ERROR\n");
	    ScsiStatus = SCSISTAT_CHECK_CONDITION;
	    SrbStatus = SRB_STATUS_ERROR;
	    break;

	  case SCSI_SENSE_ILLEGAL_REQUEST:
	    DPRINT("ATAPI error: SCSI_SENSE_ILLEGAL_REQUEST\n");
	    ScsiStatus = SCSISTAT_CHECK_CONDITION;
	    SrbStatus = SRB_STATUS_ERROR;
	    break;

	  case SCSI_SENSE_UNIT_ATTENTION:
	    DPRINT("ATAPI error: SCSI_SENSE_UNIT_ATTENTION\n");
	    ScsiStatus = SCSISTAT_CHECK_CONDITION;
	    SrbStatus = SRB_STATUS_ERROR;
	    break;

	  case SCSI_SENSE_DATA_PROTECT:
	    DPRINT("ATAPI error: SCSI_SENSE_DATA_PROTECT\n");
	    ScsiStatus = SCSISTAT_CHECK_CONDITION;
	    SrbStatus = SRB_STATUS_ERROR;
	    break;

	  case SCSI_SENSE_BLANK_CHECK:
	    DPRINT("ATAPI error: SCSI_SENSE_BLANK_CHECK\n");
	    ScsiStatus = SCSISTAT_CHECK_CONDITION;
	    SrbStatus = SRB_STATUS_ERROR;
	    break;

	  case SCSI_SENSE_ABORTED_COMMAND:
	    DPRINT("ATAPI error: SCSI_SENSE_ABORTED_COMMAND\n");
	    ScsiStatus = SCSISTAT_CHECK_CONDITION;
	    SrbStatus = SRB_STATUS_ERROR;
	    break;

	  default:
	    DPRINT("ATAPI error: Invalid sense key\n");
	    ScsiStatus = 0;
	    SrbStatus = SRB_STATUS_ERROR;
	    break;
	}
    }
  else
    {
      DPRINT1("IDE error: %02x\n", ErrorReg);

      ScsiStatus = 0;
      SrbStatus = SRB_STATUS_ERROR;

#if 0
      UCHAR SectorCount, SectorNum, CylinderLow, CylinderHigh;
      UCHAR DriveHead;

      CylinderLow = IDEReadCylinderLow(CommandPortBase);
      CylinderHigh = IDEReadCylinderHigh(CommandPortBase);
      DriveHead = IDEReadDriveHead(CommandPortBase);
      SectorCount = IDEReadSectorCount(CommandPortBase);
      SectorNum = IDEReadSectorNum(CommandPortBase);

      DPRINT1("IDE Error: ERR:%02x CYLLO:%02x CYLHI:%02x SCNT:%02x SNUM:%02x\n",
	      ErrorReg,
	      CylinderLow,
	      CylinderHigh,
	      SectorCount,
	      SectorNum);
#endif
    }



  Srb->ScsiStatus = ScsiStatus;

  DPRINT("AtapiErrorToScsi() done\n");

  return(SrbStatus);
}


static VOID
AtapiScsiSrbToAtapi (PSCSI_REQUEST_BLOCK Srb)
{
  DPRINT("AtapiConvertScsiToAtapi() called\n");

  Srb->CdbLength = 12;

  switch (Srb->Cdb[0])
    {
      case SCSIOP_FORMAT_UNIT:
	Srb->Cdb[0] = ATAPI_FORMAT_UNIT;
	break;

      case SCSIOP_MODE_SELECT:
	  {
	    PATAPI_MODE_SELECT12 AtapiModeSelect;
	    UCHAR Length;

	    AtapiModeSelect = (PATAPI_MODE_SELECT12)Srb->Cdb;
	    Length = ((PCDB)Srb->Cdb)->MODE_SELECT.ParameterListLength;

	    RtlZeroMemory (Srb->Cdb,
			   MAXIMUM_CDB_SIZE);
	    AtapiModeSelect->OperationCode = ATAPI_MODE_SELECT;
	    AtapiModeSelect->PFBit = 1;
	    AtapiModeSelect->ParameterListLengthMsb = 0;
	    AtapiModeSelect->ParameterListLengthLsb = Length;
	  }
	break;
    }
}

static VOID FASTCALL
AtapiCompleteRequest(PATAPI_MINIPORT_EXTENSION DevExt, 
		     UCHAR SrbStatus)
{
  PSCSI_REQUEST_BLOCK Srb;
  Srb = DevExt->CurrentSrb;

  DPRINT("AtapiCompleteRequest(DevExt %x, SrbStatus %x)\n", DevExt, SrbStatus);

  Srb->SrbStatus = SrbStatus;
  if (SrbStatus != SRB_STATUS_SUCCESS)
    {
      Srb->SrbStatus = AtapiErrorToScsi((PVOID)DevExt, Srb);
    }
  DevExt->Handler = NULL;
  ScsiPortNotification(RequestComplete, (PVOID)DevExt, Srb);
  ScsiPortNotification(NextRequest, (PVOID)DevExt, NULL);
}

static BOOLEAN FASTCALL 
AtapiPacketInterrupt(PATAPI_MINIPORT_EXTENSION DevExt)
{
  PSCSI_REQUEST_BLOCK Srb;
  BYTE Status;
  BYTE IntReason;
  ULONG TransferSize;
  ULONG JunkSize;
  BOOL IsLastBlock;
  PBYTE TargetAddress;
  ULONG Retries;
  BYTE SrbStatus;
  BYTE Error;
  BYTE SensKey;

  DPRINT("AtapiPacketInterrupt()\n");

  Srb = DevExt->CurrentSrb;

  Status = IDEReadStatus(DevExt->CommandPortBase);
  DPRINT("DriveStatus: %x\n", Status);

  if (Status & (IDE_SR_BUSY|IDE_SR_ERR))
    {
      if (Status & IDE_SR_ERR)
        {
	  Error = IDEReadError(DevExt->CommandPortBase);
	  SensKey = Error >> 4;
	  DPRINT("DriveError: %x, SenseKey: %x\n", Error, SensKey);
	}

      AtapiCompleteRequest(DevExt, SRB_STATUS_ERROR);
      DPRINT("AtapiPacketInterrupt() done\n");
      return TRUE;
    }
  
  IntReason = IDEReadSectorCount(DevExt->CommandPortBase);
  TransferSize = IDEReadCylinderLow(DevExt->CommandPortBase);
  TransferSize += IDEReadCylinderHigh(DevExt->CommandPortBase) << 8;

  if (!(Status & IDE_SR_DRQ))
    {
      if (DevExt->DataTransferLength > 0)
        {
	  DPRINT1("AtapiPacketInterrupt: data underrun (%d bytes), command was %02x\n", 
	          DevExt->DataTransferLength, Srb->Cdb[0]);
	  SrbStatus = SRB_STATUS_ERROR;
	}
      else
        {
	  SrbStatus = SRB_STATUS_SUCCESS;
	}
      AtapiCompleteRequest(DevExt, SrbStatus);
      DPRINT("AtapiPacketInterrupt() done\n");
      return TRUE;
    }

  TargetAddress = DevExt->DataBuffer;

  if (Srb->SrbFlags & SRB_FLAGS_DATA_IN)
    {
      DPRINT("read data\n");
      if (DevExt->DataTransferLength <= TransferSize)
        {
	  JunkSize = TransferSize - DevExt->DataTransferLength;
	  TransferSize = DevExt->DataTransferLength;

	  DevExt->DataTransferLength = 0;
	  IsLastBlock = TRUE;
	}
      else
	{
	  DevExt->DataTransferLength -= TransferSize;
	  IsLastBlock = FALSE;
	}

      DPRINT("TargetAddress %x, TransferSize %d\n", TargetAddress, TransferSize);

      DevExt->DataBuffer += TransferSize;

      IDEReadBlock(DevExt->CommandPortBase, TargetAddress, TransferSize);

      /* check DRQ */
      if (IsLastBlock)
        {
	  /* Read remaining junk from device */
	  while (JunkSize > 0)
	    {
	      IDEReadWord(DevExt->CommandPortBase);
	      JunkSize -= 2;
	    }

	  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES && (IDEReadStatus(DevExt->CommandPortBase) & IDE_SR_BUSY); Retries++)
	    {
	      ScsiPortStallExecution(10);
	    }

	  /* Check for data overrun */
	  while (IDEReadStatus(DevExt->CommandPortBase) & IDE_SR_DRQ)
	    {
	      DPRINT1("AtapiInterrupt(): reading overrun data!\n");
	      IDEReadWord(DevExt->CommandPortBase);
	    }
	}

      SrbStatus = SRB_STATUS_SUCCESS;
    }
  else if (Srb->SrbFlags & SRB_FLAGS_DATA_OUT)
    {
      DPRINT("write data\n");
      TransferSize = DevExt->TransferSize[Srb->TargetId];
      if (DevExt->DataTransferLength < TransferSize)
        {
	  TransferSize = DevExt->DataTransferLength;
	}

      TargetAddress = DevExt->DataBuffer;

      DPRINT1("TargetAddress %x, TransferSize %x\n", TargetAddress, TransferSize);

      DevExt->DataBuffer += TransferSize;
      DevExt->DataTransferLength -= TransferSize;

      /* Write the sector */
      IDEWriteBlock(DevExt->CommandPortBase, TargetAddress, TransferSize);
      SrbStatus = SRB_STATUS_SUCCESS;
      IsLastBlock = FALSE;
    }
  else
    {
      DPRINT("Unspecified transfer direction!\n");
      SrbStatus = SRB_STATUS_SUCCESS;
      IsLastBlock = TRUE;
    }
  if (IsLastBlock)
    {
      AtapiCompleteRequest(DevExt, SrbStatus);
    }
  DPRINT("AtapiPacketInterrupt() done\n");
  return TRUE;
}

static BOOLEAN FASTCALL 
AtapiNoDataInterrupt(PATAPI_MINIPORT_EXTENSION DevExt)
{
  BYTE Status;

  DPRINT1("AtapiNoDataInterrupt()\n");

  Status = IDEReadStatus(DevExt->CommandPortBase);
  AtapiCompleteRequest(DevExt,
                       (Status & (IDE_SR_DRDY|IDE_SR_BUSY|IDE_SR_ERR|IDE_SR_DRQ)) == IDE_SR_DRDY ? SRB_STATUS_SUCCESS : SRB_STATUS_ERROR);

  DPRINT1("AtapiNoDatanterrupt() done!\n");
  return TRUE;
}

#ifdef ENABLE_DMA
static BOOLEAN FASTCALL
AtapiDmaInterrupt(PATAPI_MINIPORT_EXTENSION DevExt)
{
  BYTE DmaCommand;
  BYTE DmaStatus;
  BYTE Status;

  DPRINT("AtapiDmaInterrupt()\n");

  DevExt->UseDma = FALSE;
  /* stop DMA */
  DmaCommand = IDEReadDMACommand(DevExt->BusMasterRegisterBase);
  IDEWriteDMACommand(DevExt->BusMasterRegisterBase, DmaCommand & 0xfe);
  /* get DMA status */
  DmaStatus = IDEReadDMAStatus(DevExt->BusMasterRegisterBase);
  /* clear the INTR & ERROR bits */
  IDEWriteDMAStatus(DevExt->BusMasterRegisterBase, DmaStatus | 0x06);

  /* read the drive status */
  Status = IDEReadStatus(DevExt->CommandPortBase);
  if ((Status & (IDE_SR_DRDY|IDE_SR_BUSY|IDE_SR_ERR|IDE_SR_WERR|IDE_SR_DRQ)) == IDE_SR_DRDY &&
      (DmaStatus & 0x07) == 4)
    {
      AtapiCompleteRequest(DevExt, SRB_STATUS_SUCCESS);
      DPRINT("AtapiDmaInterrupt() done\n");
      return TRUE;
    }
  DPRINT1("Status %x\n", Status);
  DPRINT1("%x\n", DmaStatus);
  AtapiCompleteRequest(DevExt, SRB_STATUS_ERROR);
  DPRINT1("AtapiDmaReadInterrupt() done\n");
  return TRUE;
}  
#endif

static BOOLEAN FASTCALL
AtapiReadInterrupt(PATAPI_MINIPORT_EXTENSION DevExt)
{
  PSCSI_REQUEST_BLOCK Srb;
  UCHAR DeviceStatus;
  BOOLEAN IsLastBlock;
  PUCHAR TargetAddress;
  ULONG TransferSize;

  DPRINT("AtapiReadInterrupt() called!\n");

  Srb = DevExt->CurrentSrb;

  DeviceStatus = IDEReadStatus(DevExt->CommandPortBase);
  if ((DeviceStatus & (IDE_SR_DRQ|IDE_SR_BUSY|IDE_SR_ERR)) != IDE_SR_DRQ)
    {
      if (DeviceStatus & (IDE_SR_ERR|IDE_SR_DRQ))
        {
	  AtapiCompleteRequest(DevExt, SRB_STATUS_ERROR);
          DPRINT("AtapiReadInterrupt() done!\n");
	  return TRUE;
        }
      DPRINT("AtapiReadInterrupt() done!\n");
      return FALSE;
    }

  DPRINT("CommandPortBase: %lx  ControlPortBase: %lx\n", DevExt->CommandPortBase, DevExt->ControlPortBase);

  /* Update controller/device state variables */
  TargetAddress = DevExt->DataBuffer;
  TransferSize = DevExt->TransferSize[Srb->TargetId];

  DPRINT("TransferLength: %lu\n", Srb->DataTransferLength);
  DPRINT("TransferSize: %lu\n", TransferSize);

  if (DevExt->DataTransferLength <= TransferSize)
    {
       TransferSize = DevExt->DataTransferLength;
       DevExt->DataTransferLength = 0;
       IsLastBlock = TRUE;
    }
  else
    {
      DevExt->DataTransferLength -= TransferSize;
      IsLastBlock = FALSE;
    }
  DevExt->DataBuffer += TransferSize;
  DPRINT("IsLastBlock == %s\n", (IsLastBlock) ? "TRUE" : "FALSE");

  /* Copy the block of data */
  if (DevExt->DeviceFlags[Srb->TargetId] & DEVICE_DWORD_IO)
    {
      IDEReadBlock32(DevExt->CommandPortBase, TargetAddress, TransferSize);
    }
  else
    {
      IDEReadBlock(DevExt->CommandPortBase, TargetAddress, TransferSize);
    }

  if (IsLastBlock)
    {
      AtapiCompleteRequest(DevExt, SRB_STATUS_SUCCESS);
    }

  DPRINT("AtapiReadInterrupt() done!\n");

  return(TRUE);
}

static BOOLEAN FASTCALL
AtapiWriteInterrupt(IN PATAPI_MINIPORT_EXTENSION DevExt)
{
  PSCSI_REQUEST_BLOCK Srb;
  UCHAR DeviceStatus;
  BOOLEAN IsLastBlock;
  PUCHAR TargetAddress;
  ULONG TransferSize;

  DPRINT("AtapiWriteInterrupt() called!\n");

  DeviceStatus = IDEReadStatus(DevExt->CommandPortBase);
  if ((DeviceStatus & (IDE_SR_DRDY|IDE_SR_BUSY|IDE_SR_ERR|IDE_SR_WERR)) != IDE_SR_DRDY)
    {
      if (DeviceStatus & (IDE_SR_DRDY|IDE_SR_ERR|IDE_SR_WERR))
        {
	  AtapiCompleteRequest(DevExt, SRB_STATUS_ERROR);
          DPRINT("AtapiWriteInterrupt() done!\n");
	  return TRUE;
        }
      DPRINT("AtapiWriteInterrupt() done!\n");
      return FALSE;
    }
  else
    {
     Srb = DevExt->CurrentSrb;
     TransferSize = DevExt->TransferSize[Srb->TargetId];
      if (DevExt->DataTransferLength < TransferSize)
        {
	  TransferSize = DevExt->DataTransferLength;
	}
      if (TransferSize > 0 && (DeviceStatus & IDE_SR_DRQ))
        {
	  IsLastBlock = FALSE;
	  TargetAddress = DevExt->DataBuffer;
          DevExt->DataBuffer += TransferSize;
	  DevExt->DataTransferLength -= TransferSize;

          DPRINT("TransferLength: %lu\n", Srb->DataTransferLength);
          DPRINT("TransferSize: %lu\n", TransferSize);
	  /* Write the sector */
	  if (DevExt->DeviceFlags[Srb->TargetId] & DEVICE_DWORD_IO)
	    {
	      IDEWriteBlock32(DevExt->CommandPortBase, TargetAddress, TransferSize);
	    }
	  else
	    {
	      IDEWriteBlock(DevExt->CommandPortBase, TargetAddress, TransferSize);
	    }
	}
      else if (DeviceStatus & IDE_SR_DRQ)
        {
          DPRINT("AtapiWriteInterrupt(): data overrun error!\n");
          IsLastBlock = TRUE;
	}
      else if (TransferSize > 0 && !(DeviceStatus & IDE_SR_DRQ))
        {
	  DPRINT1("%d !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", TransferSize);
	  IsLastBlock = TRUE;
        }
      else
        {
	  IsLastBlock = TRUE;
	}
      if (IsLastBlock)
        {
          AtapiCompleteRequest(DevExt, SRB_STATUS_SUCCESS);
        }
      DPRINT("AtapiWriteInterrupt() done!\n");
      return TRUE;
    }
}

static VOID
AtapiExecuteCommand(PATAPI_MINIPORT_EXTENSION DevExt, 
		    BYTE command, 
		    BOOLEAN FASTCALL (*Handler)(PATAPI_MINIPORT_EXTENSION))
{
  if (DevExt->Handler != NULL)
    {
      DPRINT1("DevExt->Handler is already set!!\n");
    }
  DevExt->Handler = Handler;
  IDEWriteCommand(DevExt->CommandPortBase, command);
  ScsiPortStallExecution(1);
}

#ifdef ENABLE_DMA
static BOOLEAN
AtapiInitDma(PATAPI_MINIPORT_EXTENSION DevExt,
	     PSCSI_REQUEST_BLOCK Srb,
	     BYTE cmd)
{
  PVOID StartAddress;
  PVOID EndAddress;
  PPRD PRDEntry = DevExt->PRDTable;
  SCSI_PHYSICAL_ADDRESS PhysicalAddress;
  ULONG Length;
  ULONG tmpLength;
  BYTE Status;

  DPRINT("AtapiInitDma()\n");

  StartAddress = Srb->DataBuffer;
  EndAddress = StartAddress + Srb->DataTransferLength;
  DevExt->PRDCount = 0;

  while (StartAddress < EndAddress)
    {
      PhysicalAddress = ScsiPortGetPhysicalAddress(DevExt, Srb, StartAddress, &Length);
      if (PhysicalAddress.QuadPart == 0LL || Length == 0)
        {
	  return FALSE;
	}

      while (Length)
        {
	  /* calculate the length up to the next 64k boundary */ 
	  tmpLength = 0x10000 - (PhysicalAddress.u.LowPart & 0xffff);
	  if (tmpLength > Length)
	    {
	      tmpLength = Length;
	    }
	  DevExt->PRDCount++;
	  if (DevExt->PRDCount > DevExt->PRDMaxCount)
	    {
	      return FALSE;
	    }
	  if (tmpLength == 0x10000)
	    {
	      /* Some dirty controllers cannot handle 64k transfers. We split a 64k transfer in two 32k. */ 
	      tmpLength = 0x8000;
	      DPRINT("PRD Nr. %d VirtualAddress %08x PhysicalAddress %08x, Length %04x\n", 
		     DevExt->PRDCount - 1, StartAddress, PhysicalAddress.u.LowPart, tmpLength);
	      PRDEntry->PhysAddress = PhysicalAddress.u.LowPart;
	      PRDEntry->Length = tmpLength;
	      PRDEntry++;
	      DevExt->PRDCount++;
	      if (DevExt->PRDCount > DevExt->PRDMaxCount)
	        {
	          return FALSE;
	        }
              PhysicalAddress.u.LowPart += tmpLength;
	      StartAddress += tmpLength;
	      Length -= tmpLength;
	      PRDEntry->PhysAddress = PhysicalAddress.u.LowPart;
	    }
	  DPRINT("PRD Nr. %d VirtualAddress %08x PhysicalAddress %08x, Length %04x\n", 
		 DevExt->PRDCount - 1, StartAddress, PhysicalAddress.u.LowPart, tmpLength);
	  PRDEntry->PhysAddress = PhysicalAddress.u.LowPart;
          PRDEntry->Length = tmpLength;
          PRDEntry++;
          StartAddress += tmpLength;
          PhysicalAddress.u.LowPart += tmpLength; 
	  Length -= tmpLength;
	}
    }
  /* set the end marker in the last PRD */
  PRDEntry--;
  PRDEntry->Length |= 0x80000000;
  /* enable the busmaster transfers for the drive */
  Status = IDEReadDMAStatus(DevExt->BusMasterRegisterBase);
  IDEWriteDMAStatus(DevExt->BusMasterRegisterBase, (Status & ~0x06) | (Srb->TargetId ? 0x40 : 0x20));
  /* set the PDR table */
  IDEWritePRDTable(DevExt->BusMasterRegisterBase, DevExt->PRDTablePhysicalAddress.u.LowPart);
  /* write the DMA command */
  IDEWriteDMACommand(DevExt->BusMasterRegisterBase, cmd);
  /* reset the status and interrupt bit */
  Status = IDEReadDMAStatus(DevExt->BusMasterRegisterBase);
  IDEWriteDMAStatus(DevExt->BusMasterRegisterBase, Status | 0x06);
  return TRUE;
}
#endif

/* EOF */
