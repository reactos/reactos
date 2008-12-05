/* $Id: diskdump.c 29690 2007-10-19 23:21:45Z dreimer $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            services/storage/diskdump/diskdump.c
 * PURPOSE:         Dumping crash data to the pagefile
 * PROGRAMMER:
 */

/* INCLUDES *****************************************************************/

#include <ntddk.h>
#include <scsi.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#include <../class/include/class2.h>
#include <diskdump/diskdump.h>
#include <ndk/rtlfuncs.h>

#include "../scsiport/scsiport_int.h"

#define NDEBUG
#include <debug.h>

/* It's already defined in scsiport_int.h */
#undef VERSION
#define VERSION  "0.0.1"

#undef KeGetCurrentIrql
/* PROTOTYPES ***************************************************************/

NTSTATUS NTAPI
DiskDumpPrepare(PDEVICE_OBJECT DeviceObject, PDUMP_POINTERS DumpPointers);
VOID
DiskDumpScsiInvalid(VOID);
VOID
_DiskDumpScsiPortNotification(IN SCSI_NOTIFICATION_TYPE NotificationType,
			     IN PVOID HwDeviceExtension,
			     ...);
NTSTATUS NTAPI
DiskDumpInit(VOID);
NTSTATUS NTAPI
DiskDumpFinish(VOID);
NTSTATUS NTAPI
DiskDumpWrite(LARGE_INTEGER StartAddress, PMDL Mdl);

typedef VOID (*SCSIPORTNOTIFICATION)(IN SCSI_NOTIFICATION_TYPE NotificationType,
				     IN PVOID HwDeviceExtension,
				     ...);

/* GLOBALS ******************************************************************/

MM_CORE_DUMP_FUNCTIONS DiskDumpFunctions =
  {
    (PVOID)DiskDumpPrepare,
    (PVOID)DiskDumpInit,
    (PVOID)DiskDumpWrite,
    (PVOID)DiskDumpFinish,
  };

typedef struct
{
  PCH Name;
  ULONG Ordinal;
  PVOID OldFunction;
  PVOID NewFunction;
} SUBSTITUTE_EXPORT;

static SCSI_REQUEST_BLOCK CoreDumpSrb;
static DUMP_POINTERS CoreDumpPointers;
static PDEVICE_OBJECT CoreDumpClassDevice;
static PDEVICE_EXTENSION CoreDumpClass2DeviceExtension;
static PDEVICE_OBJECT CoreDumpPortDevice;
static SCSI_PORT_DEVICE_EXTENSION* CoreDumpPortDeviceExtension;
BOOLEAN IsDumping = FALSE;
static PDRIVER_OBJECT DiskDumpDriver;
static UCHAR DiskDumpSenseData[SENSE_BUFFER_SIZE];
static BOOLEAN IrqComplete, IrqNextRequest;
PVOID OldScsiPortNotification;
static SUBSTITUTE_EXPORT DiskDumpExports[] =
  {
    {"ScsiPortConvertPhysicalAddressToUlong", 2, NULL, NULL},
    {"ScsiPortConvertUlongToPhysicalAddress", 3, NULL, NULL},
    {"ScsiPortFreeDeviceBase", 5, NULL, DiskDumpScsiInvalid},
    {"ScsiPortGetBusData", 6, NULL, DiskDumpScsiInvalid},
    {"ScsiPortGetDeviceBase", 7, NULL, DiskDumpScsiInvalid},
    {"ScsiPortInitialize", 13, NULL, DiskDumpScsiInvalid},
    {"ScsiPortNotification", 17, NULL, _DiskDumpScsiPortNotification},
    {"ScsiPortReadPortBufferUlong", 19, NULL, NULL},
    {"ScsiPortReadPortBufferUshort", 20, NULL, NULL},
    {"ScsiPortReadPortUchar", 21, NULL, NULL},
    {"ScsiPortReadPortUshort", 23, NULL, NULL},
    {"ScsiPortStallExecution", 31, NULL, NULL},
    {"ScsiPortWritePortBufferUlong", 34, NULL, NULL},
    {"ScsiPortWritePortBufferUshort", 35, NULL, NULL},
    {"ScsiPortWritePortUchar", 36, NULL, NULL},
    {"ScsiDebugPrint", 0, NULL, NULL},
  };

/* FUNCTIONS ****************************************************************/



VOID
DiskDumpScsiPortNotification(IN SCSI_NOTIFICATION_TYPE NotificationType,
			     IN PVOID HwDeviceExtension,
			     ...)
{
  if (NotificationType == RequestComplete)
    {
      IrqComplete = TRUE;
    }
  if (NotificationType == NextRequest)
    {
      IrqNextRequest = TRUE;
    }
}

VOID
DiskDumpScsiInvalid(VOID)
{
  DbgPrint("DISKDUMP: Error: Miniport called a function not supported at dump time.\n");
  KeBugCheck(0);
}

VOID NTAPI
DiskDumpBuildRequest(LARGE_INTEGER StartingOffset, PMDL Mdl)
{
  LARGE_INTEGER StartingBlock;
  PSCSI_REQUEST_BLOCK Srb;
  PCDB Cdb;
  ULONG LogicalBlockAddress;
  USHORT TransferBlocks;

  /* Calculate logical block address */
  StartingBlock.QuadPart = StartingOffset.QuadPart >> CoreDumpClass2DeviceExtension->SectorShift;
  LogicalBlockAddress = (ULONG)StartingBlock.u.LowPart;

  DPRINT("Logical block address: %lu\n", LogicalBlockAddress);

  /* Allocate and initialize an SRB */
  Srb = &CoreDumpSrb;

  Srb->SrbFlags = 0;
  Srb->Length = sizeof(SCSI_REQUEST_BLOCK); //SCSI_REQUEST_BLOCK_SIZE;
  Srb->OriginalRequest = NULL;
  Srb->PathId = CoreDumpClass2DeviceExtension->PathId;
  Srb->TargetId = CoreDumpClass2DeviceExtension->TargetId;
  Srb->Lun = CoreDumpClass2DeviceExtension->Lun;
  Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
  Srb->DataBuffer = Mdl->MappedSystemVa;
  Srb->DataTransferLength = PAGE_SIZE;
  Srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
  Srb->QueueSortKey = LogicalBlockAddress;

  Srb->SenseInfoBuffer = DiskDumpSenseData;
  Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

  Srb->TimeOutValue =
    ((Srb->DataTransferLength + 0xFFFF) >> 16) * CoreDumpClass2DeviceExtension->TimeOutValue;

  Srb->SrbStatus = SRB_STATUS_SUCCESS;
  Srb->ScsiStatus = 0;
  Srb->NextSrb = 0;

  Srb->CdbLength = 10;
  Cdb = (PCDB)Srb->Cdb;

  /* Initialize ATAPI packet (12 bytes) */
  RtlZeroMemory(Cdb, MAXIMUM_CDB_SIZE);

  Cdb->CDB10.LogicalUnitNumber = CoreDumpClass2DeviceExtension->Lun;
  TransferBlocks = (USHORT)(PAGE_SIZE >>
			    CoreDumpClass2DeviceExtension->SectorShift);

  /* Copy little endian values into CDB in big endian format */
  Cdb->CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&LogicalBlockAddress)->Byte3;
  Cdb->CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&LogicalBlockAddress)->Byte2;
  Cdb->CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&LogicalBlockAddress)->Byte1;
  Cdb->CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&LogicalBlockAddress)->Byte0;

  Cdb->CDB10.TransferBlocksMsb = ((PFOUR_BYTE)&TransferBlocks)->Byte1;
  Cdb->CDB10.TransferBlocksLsb = ((PFOUR_BYTE)&TransferBlocks)->Byte0;


  /* Write Command. */
  Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;
  Cdb->CDB10.OperationCode = SCSIOP_WRITE;

  /* Leave caching disabled. */
}

BOOLEAN NTAPI
DiskDumpIsr(PKINTERRUPT Interrupt, PVOID ServiceContext)
{
  if (!CoreDumpPortDeviceExtension->HwInterrupt(&CoreDumpPortDeviceExtension->MiniPortDeviceExtension))
    {
      return(FALSE);
    }
  return(TRUE);
}

NTSTATUS NTAPI
DiskDumpInit(VOID)
{
  KIRQL CurrentIrql = KeGetCurrentIrql();
  IsDumping = TRUE;
  if (CurrentIrql >= CoreDumpPortDeviceExtension->Interrupt->SynchronizeIrql)
    {
      DbgPrint("DISKDUMP: Error: Crash inside high priority interrupt routine.\n");
      return(STATUS_UNSUCCESSFUL);
    }
  CoreDumpPortDeviceExtension->Interrupt->ServiceRoutine = DiskDumpIsr;

  return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
DiskDumpFinish(VOID)
{
  return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
DiskDumpWrite(LARGE_INTEGER Address, PMDL Mdl)
{
  KIRQL OldIrql = 0, OldIrql2 = 0;
  KIRQL CurrentIrql = KeGetCurrentIrql();

  if (CurrentIrql < (CoreDumpPortDeviceExtension->Interrupt->SynchronizeIrql - 1))
    {
      KeRaiseIrql(CoreDumpPortDeviceExtension->Interrupt->SynchronizeIrql - 1, &OldIrql);
    }

  /* Adjust the address for the start of the partition. */
  Address.QuadPart +=
    (CoreDumpClass2DeviceExtension->StartingOffset.QuadPart + CoreDumpClass2DeviceExtension->DMByteSkew);

  /* Assume the device is always able to transfer a page so no need to split up the transfer. */

  /* Build an SRB to describe the write. */
  DiskDumpBuildRequest(Address, Mdl);

  /* Start i/o on the HBA. */
  IrqComplete = IrqNextRequest = FALSE;
  KeRaiseIrql(CoreDumpPortDeviceExtension->Interrupt->SynchronizeIrql, &OldIrql2);
  if (!CoreDumpPortDeviceExtension->HwStartIo(&CoreDumpPortDeviceExtension->MiniPortDeviceExtension,
					      &CoreDumpSrb))
    {
      KeLowerIrql(OldIrql);
      DbgPrint("DISKDUMP: Error: Miniport HwStartIo failed.\n");
      return(STATUS_UNSUCCESSFUL);
    }
  KeLowerIrql(OldIrql2);

  /* Wait for the miniport to finish. */
  __asm__ ("sti\n\t");
  while (!IrqComplete || !IrqNextRequest)
    {
      __asm__ ("hlt\n\t");
    }
  if (CurrentIrql < (CoreDumpPortDeviceExtension->Interrupt->SynchronizeIrql - 1))
    {
      KeLowerIrql(OldIrql);
    }
  __asm__("cli\n\t");

  /* Check the result. */
  if (SRB_STATUS(CoreDumpSrb.SrbStatus) != SRB_STATUS_SUCCESS)
    {
      DbgPrint("DISKDUMP: Error: SRB failed.\n");
      return(STATUS_UNSUCCESSFUL);
    }
  return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
DiskDumpPrepare(PDEVICE_OBJECT DeviceObject, PDUMP_POINTERS DumpPointers)
{
  PIMAGE_NT_HEADERS NtHeader;
  PVOID ImportDirectory;
  ULONG ImportDirectorySize;
  PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectory;
  PVOID DriverBase;
  PCH Name;
  ULONG i;
  ULONG Hint;
  PVOID* ImportAddressList;
  PULONG FunctionNameList;

  /* Save the information from the kernel. */
  CoreDumpClassDevice = DeviceObject;
  CoreDumpPointers = *DumpPointers;
  CoreDumpClass2DeviceExtension = (PDEVICE_EXTENSION)CoreDumpClassDevice->DeviceExtension;
  CoreDumpPortDevice = DumpPointers->DeviceObject;
  CoreDumpPortDeviceExtension = CoreDumpPortDevice->DeviceExtension;

  /* Replace all the miniport driver's imports with our functions. */
  DriverBase = CoreDumpPortDevice->DriverObject->DriverStart;
  NtHeader = RtlImageNtHeader(DriverBase);
  ImportDirectory = RtlImageDirectoryEntryToData(DriverBase,
						 TRUE,
						 IMAGE_DIRECTORY_ENTRY_IMPORT,
						 &ImportDirectorySize);
  if (ImportDirectory == NULL || ImportDirectorySize == 0)
    {
      DbgPrint("DISKDUMP: Error: Miniport has no imports?\n");
      return(STATUS_UNSUCCESSFUL);
    }
  /*  Process each import module  */
  ImportModuleDirectory = (PIMAGE_IMPORT_DESCRIPTOR)ImportDirectory;
  DPRINT("Processeing import directory at %p\n", ImportModuleDirectory);
  while (ImportModuleDirectory->Name)
    {
      /*  Check to make sure that import lib is kernel  */
      Name = (PCHAR) DriverBase + ImportModuleDirectory->Name;

      if (strcmp(Name, "scsiport.sys") != 0)
	{
	  DbgPrint("DISKDUMP: Warning: Miniport has illegal imports.\n");
	  ImportModuleDirectory++;
	  continue;
	}

      /*  Get the import address list  */
      ImportAddressList = (PVOID *) ((PUCHAR)DriverBase +
				     (ULONG_PTR)ImportModuleDirectory->FirstThunk);

      /*  Get the list of functions to import  */
      if (ImportModuleDirectory->OriginalFirstThunk != 0)
	{
	  FunctionNameList = (PULONG) ((PUCHAR)DriverBase +
				       (ULONG_PTR)ImportModuleDirectory->OriginalFirstThunk);
	}
      else
	{
	  FunctionNameList = (PULONG) ((PUCHAR)DriverBase +
				       (ULONG_PTR)ImportModuleDirectory->FirstThunk);
	}
      /*  Walk through function list and fixup addresses  */
      while (*FunctionNameList != 0L)
	{
	  if ((*FunctionNameList) & 0x80000000) // hint
	    {
	      Name = NULL;

	      Hint = (*FunctionNameList) & 0xffff;
	    }
	  else // hint-name
	    {
	      Name = (PCHAR)((ULONG_PTR)DriverBase +
			      (ULONG_PTR)*FunctionNameList + 2);
	      Hint = *(PUSHORT)((ULONG_PTR)DriverBase + (ULONG_PTR)*FunctionNameList);
	    }
#if 0
	  DPRINT("  Hint:%04x  Name:%s\n", Hint, pName);
#endif

	  for (i = 0; i < (sizeof(DiskDumpExports) / sizeof(DiskDumpExports[0])); i++)
	    {
	      if (DiskDumpExports[i].Ordinal == Hint ||
		  (Name != NULL && strcmp(DiskDumpExports[i].Name, Name) == 0))
		{
		  break;
		}
	    }
	  if (i == (sizeof(DiskDumpExports) / sizeof(DiskDumpExports[0])))
	    {
	      DbgPrint("DISKDUMP: Error: Miniport imports unknown symbol %s.\n", Name);
	      return(STATUS_UNSUCCESSFUL);
	    }
	  if (strcmp(Name, "ScsiPortNotification") == 0)
	    {
	      OldScsiPortNotification = *ImportAddressList;
	    }
	  DiskDumpExports[i].OldFunction = *ImportAddressList;
	  if (DiskDumpExports[i].NewFunction != NULL)
	    {
	      *ImportAddressList = DiskDumpExports[i].NewFunction;
	    }

	  ImportAddressList++;
	  FunctionNameList++;
	}
      ImportModuleDirectory++;
    }
  return(STATUS_SUCCESS);
}

/**********************************************************************
 * NAME							EXPORTED
 *	DriverEntry
 *
 * DESCRIPTION
 *	This function initializes the driver, locates and claims
 *	hardware resources, and creates various NT objects needed
 *	to process I/O requests.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DriverObject
 *		System allocated Driver Object for this driver
 *
 *	RegistryPath
 *		Name of registry driver service key
 *
 * RETURN VALUE
 *	Status
 */

NTSTATUS NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
	    IN PUNICODE_STRING RegistryPath)
{
  DiskDumpDriver = DriverObject;
  return(STATUS_SUCCESS);
}


/* EOF */
