/* $Id: ide.c,v 1.44 2001/08/27 01:26:38 ekohl Exp $
 *
 *  IDE.C - IDE Disk driver 
 *     written by Rex Jolliff
 *     with help from various documentation sources and a few peeks at
 *       linux and freebsd sources.
 * 
 *    This driver supports up to 4 controllers with up to 2 drives each.
 *    The device names are assigned as follows:
 *      \Devices\HarddiskX\Partition0
 *    for the raw device, and 
 *      \Devices\HarddiskX\PartitionY
 *    for partitions
 *    where:
 *      X is computed by counting the available drives from the following
 *          sequence: the controller number (0=0x1f0, 1=0x170, 2=0x1e8, 
 *          3=0x168) * 2 plus the drive number (0,1)
 *      Y is the partition number
 * 
 *     The driver exports the following function:
 * 
 *       DriverEntry() - NT device driver initialization routine
 * 
 *     And the following functions are exported implicitly:
 * 
 *       IDEStartIo() - called to start an I/O request packet
 *       IDEDispatchOpenClose() - Called to open/close the device.  a NOOP
 *       IDEDispatchReadWrite() - Called to read/write the device.
 *       IDEDispatchQueryInformation() - Called to get device information 
 *       IDEDispatchSetInformation() - Called to set device information
 *       IDEDispatchDeviceControl() - Called to execute device control requests
 * 
 *   Modification History:
 *     05/25/98  RJJ  Created.
 *     05/30/98  RJJ  Removed IRQ handler and inserted busy waits
 *                    just to get something working...
 *     07/18/98  RJJ  Made drastic changes so that the driver
 *                    resembles a WinNT driver.
 *     08/05/98  RJJ  Changed to .C extension
 *     09/19/98  RJJ  First release (run for cover!)
 *
 *   Test List:
 *     09/17/98  RJJ  Pri/MST: 14.12X19 WDC AC31000H  Test Passed
 *                    Pri/SLV: None.
 *     
 *
 *   To Do:
 * FIXME: a timer should be used to watch for device timeouts and errors
 * FIXME: errors should be retried
 * FIXME: a drive reset/recalibrate should be attempted if errors occur
 * FIXME: Use DMA for transfers if drives support it
 * FIXME: should we support unloading of this driver???
 * FIXME: the device information should come from AUTODETECT (via registry)
 * FIXME: really big devices need to be handled correctly
 * FIXME: should get device info from the registry
 * FIXME: should report hardware usage to iomgr
 * FIXME: finish implementation of QueryInformation
 * FIXME: finish implementation of SetInformation
 * FIXME: finish implementation of DeviceControl
 * FIXME: bring up to ATA-3 spec
 * FIXME: add general support for ATAPI devices
 * FIXME: add support for ATAPI CDROMs
 * FIXME: add support for ATAPI ZIP drives/RHDs
 * FIXME: add support for ATAPI tape drives
 */

//  -------------------------------------------------------------------------

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "ide.h"
#include "partitio.h"

#define  VERSION  "V0.1.4"

/* remove this to activate the old behaviour of partition chain code (EK) */
#define PARTITION_FIX

//  -------------------------------------------------------  File Static Data

typedef struct _IDE_CONTROLLER_PARAMETERS 
{
  int              CommandPortBase;
  int              CommandPortSpan;
  int              ControlPortBase;
  int              ControlPortSpan;
  int              Vector;
  int              IrqL;
  int              SynchronizeIrqL;
  KINTERRUPT_MODE  InterruptMode;
  KAFFINITY        Affinity;
} IDE_CONTROLLER_PARAMETERS, *PIDE_CONTROLLER_PARAMETERS;

//  NOTE: Do not increase max drives above 2

#define  IDE_MAX_DRIVES       2

#define  IDE_MAX_CONTROLLERS  1
IDE_CONTROLLER_PARAMETERS Controllers[IDE_MAX_CONTROLLERS] = 
{
  {0x01f0, 8, 0x03f6, 1, 14, 14, 15, LevelSensitive, 0xffff}
  /*{0x0170, 8, 0x0376, 1, 15, 15, 15, LevelSensitive, 0xffff},
  {0x01E8, 8, 0x03ee, 1, 11, 11, 15, LevelSensitive, 0xffff},
  {0x0168, 8, 0x036e, 1, 10, 10, 15, LevelSensitive, 0xffff}*/
};

static BOOLEAN IDEInitialized = FALSE;

//  -----------------------------------------------  Discardable Declarations

#ifdef  ALLOC_PRAGMA

//  make the initialization routines discardable, so that they 
//  don't waste space

#pragma  alloc_text(init, DriverEntry)
#pragma  alloc_text(init, IDECreateController)
#pragma  alloc_text(init, IDECreateDevices)
#pragma  alloc_text(init, IDECreateDevice)
#pragma  alloc_text(init, IDEPolledRead)

//  make the PASSIVE_LEVEL routines pageable, so that they don't
//  waste nonpaged memory

#pragma  alloc_text(page, IDEShutdown)
#pragma  alloc_text(page, IDEDispatchOpenClose)
#pragma  alloc_text(page, IDEDispatchRead)
#pragma  alloc_text(page, IDEDispatchWrite)

#endif  /*  ALLOC_PRAGMA  */

//  ---------------------------------------------------- Forward Declarations

static BOOLEAN IDECreateController(IN PDRIVER_OBJECT DriverObject,
                                   IN PIDE_CONTROLLER_PARAMETERS ControllerParams,
                                   IN int ControllerIdx);
static BOOLEAN IDEResetController(IN WORD CommandPort, IN WORD ControlPort);
static BOOLEAN IDECreateDevices(IN PDRIVER_OBJECT DriverObject,
                                IN PCONTROLLER_OBJECT ControllerObject,
                                IN PIDE_CONTROLLER_EXTENSION ControllerExtension,
                                IN int DriveIdx,
                                IN int HarddiskIdx);
static BOOLEAN IDEGetDriveIdentification(IN int CommandPort,
                                         IN int DriveNum,
                                         OUT PIDE_DRIVE_IDENTIFY DrvParms);
static NTSTATUS IDECreateDiskDevice(IN PDRIVER_OBJECT DriverObject,
                                OUT PDEVICE_OBJECT *DeviceObject,
                                IN PCONTROLLER_OBJECT ControllerObject,
                                IN int UnitNumber,
                                IN ULONG DiskNumber,
                                IN PIDE_DRIVE_IDENTIFY DrvParms,
                                IN ULONG SectorCount);
static NTSTATUS IDECreatePartitionDevice(IN PDRIVER_OBJECT DriverObject,
                                         OUT PDEVICE_OBJECT *DeviceObject,
                                         IN PCONTROLLER_OBJECT ControllerObject,
                                         IN PVOID DiskDeviceExtension,
                                         IN int UnitNumber,
                                         IN ULONG DiskNumber,
                                         IN PIDE_DRIVE_IDENTIFY DrvParms,
                                         IN PPARTITION_INFORMATION PartitionInfo);
//                                IN ULONG PartitionIdx,
//                                IN ULONGLONG Offset,
//                                IN ULONGLONG Size);
static int IDEPolledRead(IN WORD Address, 
                         IN BYTE PreComp, 
                         IN BYTE SectorCnt, 
                         IN BYTE SectorNum , 
                         IN BYTE CylinderLow, 
                         IN BYTE CylinderHigh, 
                         IN BYTE DrvHead, 
                         IN BYTE Command, 
                         OUT BYTE *Buffer);
static NTSTATUS STDCALL IDEDispatchOpenClose(IN PDEVICE_OBJECT pDO, IN PIRP Irp);
static NTSTATUS STDCALL IDEDispatchReadWrite(IN PDEVICE_OBJECT pDO, IN PIRP Irp);
static NTSTATUS STDCALL IDEDispatchDeviceControl(IN PDEVICE_OBJECT pDO, IN PIRP Irp);
static VOID STDCALL IDEStartIo(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
static IO_ALLOCATION_ACTION STDCALL
IDEAllocateController(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp,
                      IN PVOID MapRegisterBase,
                      IN PVOID Ccontext);
static BOOLEAN STDCALL
IDEStartController(IN OUT PVOID Context);
VOID IDEBeginControllerReset(PIDE_CONTROLLER_EXTENSION ControllerExtension);
static BOOLEAN STDCALL IDEIsr(IN PKINTERRUPT Interrupt, IN PVOID ServiceContext);
static VOID IDEDpcForIsr(IN PKDPC Dpc, 
                         IN PDEVICE_OBJECT DpcDeviceObject,
                         IN PIRP DpcIrp, 
                         IN PVOID DpcContext);
static VOID IDEFinishOperation(PIDE_CONTROLLER_EXTENSION ControllerExtension);
static VOID STDCALL IDEIoTimer(PDEVICE_OBJECT DeviceObject, PVOID Context);

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
  BOOLEAN        WeGotSomeDisks;
  int            ControllerIdx;

  DbgPrint("IDE Driver %s\n", VERSION);

    //  Export other driver entry points...
  DriverObject->DriverStartIo = IDEStartIo;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = IDEDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = IDEDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_READ] = IDEDispatchReadWrite;
  DriverObject->MajorFunction[IRP_MJ_WRITE] = IDEDispatchReadWrite;
//  DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = IDEDispatchQueryInformation;
//  DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = IDEDispatchSetInformation;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IDEDispatchDeviceControl;

  WeGotSomeDisks = FALSE;
  for (ControllerIdx = 0; ControllerIdx < IDE_MAX_CONTROLLERS; ControllerIdx++) 
    {
      if (IDECreateController(DriverObject, 
                              &Controllers[ControllerIdx], 
                              ControllerIdx))
      {
        WeGotSomeDisks = TRUE;
      }
    }

  if (WeGotSomeDisks)
    {
      IDEInitialized = TRUE;
    }

  DPRINT( "Returning from DriverEntry\n" );
  return  WeGotSomeDisks ? STATUS_SUCCESS : STATUS_NO_SUCH_DEVICE;
}

//  ----------------------------------------------------  Discardable statics

//    IDECreateController
//
//  DESCRIPTION:
//    Creates a controller object and a device object for each valid
//    device on the controller
//
//  RUN LEVEL:
//    PASSIVE LEVEL
//
//  ARGUMENTS:
//    IN  PDRIVER_OBJECT  DriverObject  The system created driver object
//    IN  PIDE_CONTROLLER_PARAMETERS    The parameter block for this
//                    ControllerParams  controller
//    IN  int            ControllerIdx  The index of this controller
//
//  RETURNS:
//    TRUE   Devices where found on this controller
//    FALSE  The controller does not respond or there are no devices on it
//

BOOLEAN  
IDECreateController(IN PDRIVER_OBJECT DriverObject, 
                    IN PIDE_CONTROLLER_PARAMETERS ControllerParams, 
                    IN int ControllerIdx) 
{
  BOOLEAN                    CreatedDevices, ThisDriveExists;
  int                        DriveIdx;
  NTSTATUS                   RC;
  PCONTROLLER_OBJECT         ControllerObject;
  PIDE_CONTROLLER_EXTENSION  ControllerExtension;

    //  Try to reset the controller and return FALSE if it fails
  if (!IDEResetController(ControllerParams->CommandPortBase,
                          ControllerParams->ControlPortBase)) 
    {
      DbgPrint ("Could not find controller %d at %04lx\n",
                ControllerIdx,
                ControllerParams->CommandPortBase);
      return  FALSE;
    }

  ControllerObject = IoCreateController(sizeof(IDE_CONTROLLER_EXTENSION));
  if (ControllerObject == NULL) 
    {
      DbgPrint ("Could not create controller object for controller %d\n",
                ControllerIdx);
      return  FALSE;
    }

    //  Fill out Controller extension data
  ControllerExtension = (PIDE_CONTROLLER_EXTENSION)
      ControllerObject->ControllerExtension;
  ControllerExtension->Number = ControllerIdx;
  ControllerExtension->CommandPortBase = ControllerParams->CommandPortBase;
  ControllerExtension->ControlPortBase = ControllerParams->ControlPortBase;
  ControllerExtension->Vector = ControllerParams->Vector;
  ControllerExtension->DMASupported = FALSE;
  ControllerExtension->ControllerInterruptBug = FALSE;
  ControllerExtension->OperationInProgress = FALSE;

    //  Initialize the spin lock in the controller extension
  KeInitializeSpinLock(&ControllerExtension->SpinLock);

    //  Register an interrupt handler for this controller
  RC = IoConnectInterrupt(&ControllerExtension->Interrupt,
                          IDEIsr, 
                          ControllerExtension, 
                          &ControllerExtension->SpinLock, 
                          ControllerExtension->Vector, 
                          ControllerParams->IrqL, 
                          ControllerParams->SynchronizeIrqL, 
                          ControllerParams->InterruptMode, 
                          FALSE, 
                          ControllerParams->Affinity, 
                          FALSE);
  if (!NT_SUCCESS(RC)) 
    {
      DbgPrint ("Could not Connect Interrupt %d\n", 
                ControllerExtension->Vector);
      IoDeleteController (ControllerObject);
      return  FALSE;
    }

/* TEST */
   IDEInitialized = TRUE;

    //  Create device objects for each raw device (and for partitions) 
  CreatedDevices = FALSE;
  for (DriveIdx = 0; DriveIdx < IDE_MAX_DRIVES; DriveIdx++) 
    {
      ThisDriveExists = IDECreateDevices(DriverObject, 
                                         ControllerObject,
                                         ControllerExtension, 
                                         DriveIdx, 
                                         ControllerIdx * 2 + DriveIdx);
      if (ThisDriveExists) 
        {
          CreatedDevices = TRUE;
        }
    }

  if (!CreatedDevices) 
    {
      DbgPrint ("Did not find any devices for controller %d\n", 
                ControllerIdx);
      IoDisconnectInterrupt (ControllerExtension->Interrupt);
      IoDeleteController (ControllerObject);
    }
  else
    {
      IDEResetController(ControllerParams->CommandPortBase,
                         ControllerParams->ControlPortBase);
      IoStartTimer(ControllerExtension->TimerDevice);
    }

  return  CreatedDevices;
}

//    IDEResetController
//
//  DESCRIPTION:
//    Reset the controller and report completion status
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN  WORD  CommandPort  The address of the command port
//    IN  WORD  ControlPort  The address of the control port
//
//  RETURNS:
//

BOOLEAN  
IDEResetController(IN WORD CommandPort, 
                   IN WORD ControlPort) 
{
  int  Retries;

    //  Assert drive reset line
  IDEWriteDriveControl(ControlPort, IDE_DC_SRST);

    //  Wait for min. 25 microseconds
  KeStallExecutionProcessor(IDE_RESET_PULSE_LENGTH);

    //  Negate drive reset line
  IDEWriteDriveControl(ControlPort, 0);

    //  Wait for BUSY negation
  for (Retries = 0; Retries < IDE_RESET_BUSY_TIMEOUT * 1000; Retries++)
    {
      if (!(IDEReadStatus(CommandPort) & IDE_SR_BUSY))
        {
          break;
        }
      KeStallExecutionProcessor(10);
    }
   CHECKPOINT;
  if (Retries >= IDE_RESET_BUSY_TIMEOUT * 1000)
    {
      return FALSE;
    }
   CHECKPOINT;
    //  return TRUE if controller came back to life. and
    //  the registers are initialized correctly
  return  IDEReadError(CommandPort) == 1;
}

//    IDECreateDevices
//
//  DESCRIPTION:
//    Create the raw device and any partition devices on this drive
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN  PDRIVER_OBJECT  DriverObject  The system created driver object
//    IN  PCONTROLLER_OBJECT         ControllerObject
//    IN  PIDE_CONTROLLER_EXTENSION  ControllerExtension
//                                      The IDE controller extension for
//                                      this device
//    IN  int             DriveIdx      The index of the drive on this
//                                      controller
//    IN  int             HarddiskIdx   The NT device number for this
//                                      drive
//
//  RETURNS:
//    TRUE   Drive exists and devices were created
//    FALSE  no devices were created for this device
//

BOOLEAN
IDECreateDevices(IN PDRIVER_OBJECT DriverObject,
                 IN PCONTROLLER_OBJECT ControllerObject,
                 IN PIDE_CONTROLLER_EXTENSION ControllerExtension,
                 IN int DriveIdx,
                 IN int HarddiskIdx)
{
  WCHAR                  NameBuffer[IDE_MAX_NAME_LENGTH];
  int                    CommandPort;
  NTSTATUS               Status;
  IDE_DRIVE_IDENTIFY     DrvParms;
  PDEVICE_OBJECT         DiskDeviceObject;
  PDEVICE_OBJECT         PartitionDeviceObject;
  PIDE_DEVICE_EXTENSION  DiskDeviceExtension;
//  PIDE_DEVICE_EXTENSION  PartitionDeviceExtension;
  UNICODE_STRING         UnicodeDeviceDirName;
  OBJECT_ATTRIBUTES      DeviceDirAttributes;
  HANDLE                 Handle;
  ULONG                  SectorCount = 0;
  PDRIVE_LAYOUT_INFORMATION PartitionList = NULL;
  PPARTITION_INFORMATION PartitionEntry;
  ULONG i;

    //  Copy I/O port offsets for convenience
  CommandPort = ControllerExtension->CommandPortBase;
//  ControlPort = ControllerExtension->ControlPortBase;
  DPRINT("probing IDE controller %d Addr %04lx Drive %d\n",
         ControllerExtension->Number,
         CommandPort,
         DriveIdx);

  /* Get the Drive Identification Data */
  if (!IDEGetDriveIdentification(CommandPort, DriveIdx, &DrvParms))
    {
      DPRINT("Giving up on drive %d on controller %d...\n", 
             DriveIdx,
             ControllerExtension->Number);
      return  FALSE;
    }

  /* Create the harddisk device directory */
  swprintf (NameBuffer,
            L"\\Device\\Harddisk%d",
            HarddiskIdx);
  RtlInitUnicodeString(&UnicodeDeviceDirName,
                       NameBuffer);
  InitializeObjectAttributes(&DeviceDirAttributes,
                             &UnicodeDeviceDirName,
                             0,
                             NULL,
                             NULL);
  Status = ZwCreateDirectoryObject(&Handle, 0, &DeviceDirAttributes);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Could not create device dir object\n");
      return FALSE;
    }

  /* Create the disk device */
  if (DrvParms.Capabilities & IDE_DRID_LBA_SUPPORTED)
    {
      SectorCount =
         (ULONG)((DrvParms.TMSectorCountHi << 16) + DrvParms.TMSectorCountLo);
    }
  else
    {
      SectorCount =
         (ULONG)(DrvParms.LogicalCyls * DrvParms.LogicalHeads * DrvParms.SectorsPerTrack);
    }
  DPRINT("SectorCount %lu\n", SectorCount);

  Status = IDECreateDiskDevice(DriverObject,
                               &DiskDeviceObject,
                               ControllerObject,
                               DriveIdx,
                               HarddiskIdx,
                               &DrvParms,
                               SectorCount);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("IDECreateDevice call failed for raw device\n");
      return FALSE;
    }

  /* Increase number of available physical disk drives */
  IoGetConfigurationInformation()->DiskCount++;

  /* Initialize device extension for the disk device */
//  DiskDeviceExtension = (PIDE_DEVICE_EXTENSION)DiskDeviceObject->DeviceExtension;
//  DiskDeviceExtension->DiskExtension = (PVOID)DiskDeviceExtension;

  /*
   * Initialize the controller timer here
   * (since it has to be tied to a device)
   */
  if (DriveIdx == 0)
    {
      ControllerExtension->TimerState = IDETimerIdle;
      ControllerExtension->TimerCount = 0;
      ControllerExtension->TimerDevice = DiskDeviceObject;
      IoInitializeTimer(DiskDeviceObject,
                        IDEIoTimer,
                        ControllerExtension);
    }

   DPRINT("DrvParms.BytesPerSector %ld\n",DrvParms.BytesPerSector);

  /* Read partition table */
  Status = IoReadPartitionTable(DiskDeviceObject,
                                DrvParms.BytesPerSector,
                                TRUE,
                                &PartitionList);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("IoReadPartitionTable() failed\n");
      return FALSE;
    }

  DPRINT("  Number of partitions: %u\n", PartitionList->PartitionCount);
  for (i=0;i < PartitionList->PartitionCount; i++)
    {
      PartitionEntry = &PartitionList->PartitionEntry[i];

      DPRINT("Partition %02ld: nr: %d boot: %1x type: %x offset: %I64d size: %I64d\n",
             i,
             PartitionEntry->PartitionNumber,
             PartitionEntry->BootIndicator,
             PartitionEntry->PartitionType,
             PartitionEntry->StartingOffset.QuadPart / 512 /*DrvParms.BytesPerSector*/,
             PartitionEntry->PartitionLength.QuadPart / 512 /* DrvParms.BytesPerSector*/);

      /* Create device for partition */
      Status = IDECreatePartitionDevice(DriverObject,
                                        &PartitionDeviceObject,
                                        ControllerObject,
                                        DiskDeviceObject->DeviceExtension, //DiskDeviceExtension,
                                        DriveIdx,
                                        HarddiskIdx,
                                        &DrvParms,
                                        PartitionEntry);
//                                        PartitionEntry->PartitionNumber,
//                                        PartitionEntry->StartingOffset.QuadPart / 512 /* DrvParms.BytesPerSector*/,
//                                        PartitionEntry->PartitionLength.QuadPart / 512 /*DrvParms.BytesPerSector*/);
      if (!NT_SUCCESS(Status))
        {
          DbgPrint("IDECreateDevice() failed\n");
          break;
        }

      /* Initialize pointer to disk device extension */
//      PartitionDeviceExtension = (PIDE_DEVICE_EXTENSION)PartitionDeviceObject->DeviceExtension;
//      PartitionDeviceExtension->DiskExtension = (PVOID)DiskDeviceExtension;
   }

   if (PartitionList != NULL)
     ExFreePool(PartitionList);

  return  TRUE;
}

//    IDEGetDriveIdentification
//
//  DESCRIPTION:
//    Get the identification block from the drive
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN   int                  CommandPort  Address of the command port
//    IN   int                  DriveNum     The drive index (0,1)
//    OUT  PIDE_DRIVE_IDENTIFY  DrvParms     Address to write drive ident block
//
//  RETURNS:
//    TRUE  The drive identification block was retrieved successfully
//

BOOLEAN  
IDEGetDriveIdentification(IN int CommandPort, 
                          IN int DriveNum, 
                          OUT PIDE_DRIVE_IDENTIFY DrvParms) 
{

    //  Get the Drive Identify block from drive or die
  if (IDEPolledRead(CommandPort, 0, 0, 0, 0, 0, (DriveNum ? IDE_DH_DRV1 : 0),
                    IDE_CMD_IDENT_DRV, (BYTE *)DrvParms) != 0) 
    {
      return FALSE;
    }

    //  Report on drive parameters if debug mode
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
  DPRINT("RWMult?:%02x  LBA:%d  DMA:%d  MinPIO:%d ns  MinDMA:%d ns\n", 
         (DrvParms->RWMultImplemented) & 0xff, 
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

  DPRINT("BytesPerSector %d\n", DrvParms->BytesPerSector);
  DrvParms->BytesPerSector = 512; /* FIXME !!!*/
  return TRUE;
}


//    IDECreateDiskDevice
//
//  DESCRIPTION:
//    Creates a device by calling IoCreateDevice and a sylbolic link for Win32
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN   PDRIVER_OBJECT      DriverObject      The system supplied driver object
//    OUT  PDEVICE_OBJECT     *DeviceObject      The created device object
//    IN   PCONTROLLER_OBJECT  ControllerObject  The Controller for the device
//    IN   BOOLEAN             LBASupported      Does the drive support LBA addressing?
//    IN   BOOLEAN             DMASupported      Does the drive support DMA?
//    IN   int                 SectorsPerLogCyl  Sectors per cylinder
//    IN   int                 SectorsPerLogTrk  Sectors per track
//    IN   DWORD               Offset            First valid sector for this device
//    IN   DWORD               Size              Count of valid sectors for this device
//
//  RETURNS:
//    NTSTATUS
//

NTSTATUS
IDECreateDiskDevice(IN PDRIVER_OBJECT DriverObject,
                    OUT PDEVICE_OBJECT *DeviceObject,
                    IN PCONTROLLER_OBJECT ControllerObject,
                    IN int UnitNumber,
                    IN ULONG DiskNumber,
                    IN PIDE_DRIVE_IDENTIFY DrvParms,
                    IN ULONG SectorCount)
{
  WCHAR                  NameBuffer[IDE_MAX_NAME_LENGTH];
  WCHAR                  ArcNameBuffer[IDE_MAX_NAME_LENGTH + 15];
  UNICODE_STRING         DeviceName;
  UNICODE_STRING         ArcName;
  NTSTATUS               RC;
  PIDE_DEVICE_EXTENSION  DeviceExtension;

    // Create a unicode device name
  swprintf(NameBuffer,
           L"\\Device\\Harddisk%d\\Partition0",
           DiskNumber);
  RtlInitUnicodeString(&DeviceName,
                       NameBuffer);

    // Create the device
  RC = IoCreateDevice(DriverObject,
                      sizeof(IDE_DEVICE_EXTENSION),
                      &DeviceName,
                      FILE_DEVICE_DISK,
                      0,
                      TRUE,
                      DeviceObject);
  if (!NT_SUCCESS(RC))
    {
      DbgPrint ("IoCreateDevice call failed\n");
      return  RC;
    }

    //  Set the buffering strategy here...
  (*DeviceObject)->Flags |= DO_DIRECT_IO;
  (*DeviceObject)->AlignmentRequirement = FILE_WORD_ALIGNMENT;

    //  Fill out Device extension data
  DeviceExtension = (PIDE_DEVICE_EXTENSION) (*DeviceObject)->DeviceExtension;
  DeviceExtension->DeviceObject = (*DeviceObject);
  DeviceExtension->ControllerObject = ControllerObject;
  DeviceExtension->DiskDeviceExtension = DeviceExtension;
  DeviceExtension->UnitNumber = UnitNumber;
  DeviceExtension->LBASupported = 
    (DrvParms->Capabilities & IDE_DRID_LBA_SUPPORTED) ? 1 : 0;
  DeviceExtension->DMASupported = 
    (DrvParms->Capabilities & IDE_DRID_DMA_SUPPORTED) ? 1 : 0;
    // FIXME: deal with bizarre sector sizes
  DeviceExtension->BytesPerSector = 512 /* DrvParms->BytesPerSector */;
  DeviceExtension->SectorsPerLogCyl = DrvParms->LogicalHeads *
      DrvParms->SectorsPerTrack;
  DeviceExtension->SectorsPerLogTrk = DrvParms->SectorsPerTrack;
  DeviceExtension->LogicalHeads = DrvParms->LogicalHeads;
  DeviceExtension->LogicalCylinders = 
    (DrvParms->Capabilities & IDE_DRID_LBA_SUPPORTED) ? DrvParms->TMCylinders : DrvParms->LogicalCyls;
  DeviceExtension->Offset = 0;
  DeviceExtension->Size = SectorCount;
  DPRINT("%wZ: offset %lu size %lu \n",
         &DeviceName,
         DeviceExtension->Offset,
         DeviceExtension->Size);

  /* Initialize the DPC object here */
  IoInitializeDpcRequest(*DeviceObject,
                         IDEDpcForIsr);

  /* assign arc name */
  swprintf(ArcNameBuffer,
           L"\\ArcName\\multi(0)disk(0)rdisk(%d)",
           DiskNumber);
  RtlInitUnicodeString(&ArcName,
                       ArcNameBuffer);
  DPRINT("%wZ ==> %wZ\n", &ArcName, &DeviceName);
  RC = IoAssignArcName(&ArcName,
                       &DeviceName);
  if (!NT_SUCCESS(RC))
    {
      DbgPrint("IoAssignArcName (%wZ) failed (Status %x)\n", &ArcName, RC);
    }

  return  RC;
}


//    IDECreatePartitionDevice
//
//  DESCRIPTION:
//    Creates a device by calling IoCreateDevice and a sylbolic link for Win32
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN   PDRIVER_OBJECT      DriverObject      The system supplied driver object
//    OUT  PDEVICE_OBJECT     *DeviceObject      The created device object
//    IN   PCONTROLLER_OBJECT  ControllerObject  The Controller for the device
//    IN   BOOLEAN             LBASupported      Does the drive support LBA addressing?
//    IN   BOOLEAN             DMASupported      Does the drive support DMA?
//    IN   int                 SectorsPerLogCyl  Sectors per cylinder
//    IN   int                 SectorsPerLogTrk  Sectors per track
//    IN   DWORD               Offset            First valid sector for this device
//    IN   DWORD               Size              Count of valid sectors for this device
//
//  RETURNS:
//    NTSTATUS
//

NTSTATUS
IDECreatePartitionDevice(IN PDRIVER_OBJECT DriverObject,
                         OUT PDEVICE_OBJECT *DeviceObject,
                         IN PCONTROLLER_OBJECT ControllerObject,
                         IN PVOID DiskDeviceExtension,
                         IN int UnitNumber,
                         IN ULONG DiskNumber,
                         IN PIDE_DRIVE_IDENTIFY DrvParms,
                         IN PPARTITION_INFORMATION PartitionInfo)
//                         IN ULONG PartitionNumber,
//                         IN ULONGLONG Offset,
//                         IN ULONGLONG Size)
{
  WCHAR                  NameBuffer[IDE_MAX_NAME_LENGTH];
  WCHAR                  ArcNameBuffer[IDE_MAX_NAME_LENGTH + 15];
  UNICODE_STRING         DeviceName;
  UNICODE_STRING         ArcName;
  NTSTATUS               RC;
  PIDE_DEVICE_EXTENSION  DeviceExtension;

    // Create a unicode device name
  swprintf(NameBuffer,
           L"\\Device\\Harddisk%d\\Partition%d",
           DiskNumber,
           PartitionInfo->PartitionNumber);
  RtlInitUnicodeString(&DeviceName,
                       NameBuffer);

    // Create the device
  RC = IoCreateDevice(DriverObject, sizeof(IDE_DEVICE_EXTENSION),
      &DeviceName, FILE_DEVICE_DISK, 0, TRUE, DeviceObject);
  if (!NT_SUCCESS(RC))
    {
      DbgPrint ("IoCreateDevice call failed\n");
      return  RC;
    }

    //  Set the buffering strategy here...
  (*DeviceObject)->Flags |= DO_DIRECT_IO;
  (*DeviceObject)->AlignmentRequirement = FILE_WORD_ALIGNMENT;

    //  Fill out Device extension data
  DeviceExtension = (PIDE_DEVICE_EXTENSION) (*DeviceObject)->DeviceExtension;
  DeviceExtension->DeviceObject = (*DeviceObject);
  DeviceExtension->ControllerObject = ControllerObject;
  DeviceExtension->DiskDeviceExtension = DiskDeviceExtension;
  DeviceExtension->UnitNumber = UnitNumber;
  DeviceExtension->LBASupported = 
    (DrvParms->Capabilities & IDE_DRID_LBA_SUPPORTED) ? 1 : 0;
  DeviceExtension->DMASupported = 
    (DrvParms->Capabilities & IDE_DRID_DMA_SUPPORTED) ? 1 : 0;
    // FIXME: deal with bizarre sector sizes
  DeviceExtension->BytesPerSector = 512 /* DrvParms->BytesPerSector */;
  DeviceExtension->SectorsPerLogCyl = DrvParms->LogicalHeads *
      DrvParms->SectorsPerTrack;
  DeviceExtension->SectorsPerLogTrk = DrvParms->SectorsPerTrack;
  DeviceExtension->LogicalHeads = DrvParms->LogicalHeads;
  DeviceExtension->LogicalCylinders = 
    (DrvParms->Capabilities & IDE_DRID_LBA_SUPPORTED) ? DrvParms->TMCylinders : DrvParms->LogicalCyls;
//  DeviceExtension->Offset = Offset;
//  DeviceExtension->Size = Size;

  DeviceExtension->Offset = PartitionInfo->StartingOffset.QuadPart / 512; /* DrvParms.BytesPerSector*/
  DeviceExtension->Size = PartitionInfo->PartitionLength.QuadPart / 512; /*DrvParms.BytesPerSector*/

  DPRINT("%wZ: offset %lu size %lu \n",
         &DeviceName,
         DeviceExtension->Offset,
         DeviceExtension->Size);

  /* Initialize the DPC object here */
  IoInitializeDpcRequest(*DeviceObject, IDEDpcForIsr);

  DbgPrint("%wZ %luMB\n", &DeviceName, DeviceExtension->Size / 2048);

  /* assign arc name */
  swprintf(ArcNameBuffer,
           L"\\ArcName\\multi(0)disk(0)rdisk(%d)partition(%d)",
           DiskNumber,
           PartitionInfo->PartitionNumber);
  RtlInitUnicodeString(&ArcName,
                       ArcNameBuffer);
  DPRINT("%wZ ==> %wZ\n", &ArcName, &DeviceName);
  RC = IoAssignArcName(&ArcName,
                       &DeviceName);
  if (!NT_SUCCESS(RC))
    {
      DbgPrint("IoAssignArcName (%wZ) failed (Status %x)\n", &ArcName, RC);
    }

  return  RC;
}


//    IDEPolledRead
//
//  DESCRIPTION:
//    Read a sector of data from the drive in a polled fashion.
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN   WORD  Address       Address of command port for drive
//    IN   BYTE  PreComp       Value to write to precomp register
//    IN   BYTE  SectorCnt     Value to write to sectorCnt register
//    IN   BYTE  SectorNum     Value to write to sectorNum register
//    IN   BYTE  CylinderLow   Value to write to CylinderLow register
//    IN   BYTE  CylinderHigh  Value to write to CylinderHigh register
//    IN   BYTE  DrvHead       Value to write to Drive/Head register
//    IN   BYTE  Command       Value to write to Command register
//    OUT  BYTE  *Buffer       Buffer for output data
//
//  RETURNS:
//    int  0 is success, non 0 is an error code
//

static int 
IDEPolledRead(IN WORD Address, 
              IN BYTE PreComp, 
              IN BYTE SectorCnt, 
              IN BYTE SectorNum , 
              IN BYTE CylinderLow, 
              IN BYTE CylinderHigh, 
              IN BYTE DrvHead, 
              IN BYTE Command, 
              OUT BYTE *Buffer) 
{
  BYTE   Status;
  int    RetryCount;

  /*  Wait for STATUS.BUSY to clear  */
  for (RetryCount = 0; RetryCount < IDE_MAX_BUSY_RETRIES; RetryCount++) 
    {
      Status = IDEReadStatus(Address);
      if (!(Status & IDE_SR_BUSY))
        {
          break;
        }
      KeStallExecutionProcessor(10);
    }
  if (RetryCount == IDE_MAX_BUSY_RETRIES) 
    {
      return IDE_ER_ABRT;
    }

  /*  Write Drive/Head to select drive  */
  IDEWriteDriveHead(Address, IDE_DH_FIXED | DrvHead);

  /*  Wait for STATUS.BUSY to clear and STATUS.DRDY to assert  */
  for (RetryCount = 0; RetryCount < IDE_MAX_BUSY_RETRIES; RetryCount++) 
    {
      Status = IDEReadStatus(Address);
      if (!(Status & IDE_SR_BUSY) && (Status & IDE_SR_DRDY))
        {
          break;
        }
      KeStallExecutionProcessor(10);
    }
  if (RetryCount == IDE_MAX_BUSY_RETRIES) 
    {
      return IDE_ER_ABRT;
    }

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
  IDEWritePrecomp(Address, PreComp);
  IDEWriteSectorCount(Address, SectorCnt);
  IDEWriteSectorNum(Address, SectorNum);
  IDEWriteCylinderHigh(Address, CylinderHigh);
  IDEWriteCylinderLow(Address, CylinderLow);
  IDEWriteDriveHead(Address, IDE_DH_FIXED | DrvHead);

  /*  Issue the command  */
  IDEWriteCommand(Address, Command);
  KeStallExecutionProcessor(50);

  while (1) 
    {

        //  wait for DRQ or error
      for (RetryCount = 0; RetryCount < IDE_MAX_POLL_RETRIES; RetryCount++)
        {
          Status = IDEReadStatus(Address);
          if (!(Status & IDE_SR_BUSY)) 
            {
              if (Status & IDE_SR_ERR) 
                {
                  BYTE  Err = IDEReadError(Address);
                  return Err;
                } 
              else if (Status & IDE_SR_DRQ) 
                {
                  break;
                }
            }
          KeStallExecutionProcessor(10);
        }
      if (RetryCount >= IDE_MAX_POLL_RETRIES)
        {
          return IDE_ER_ABRT;
        }

        //  Read data into buffer
      IDEReadBlock(Address, Buffer, IDE_SECTOR_BUF_SZ);
      Buffer += IDE_SECTOR_BUF_SZ;

        //  Check for more sectors to read
      for (RetryCount = 0; RetryCount < IDE_MAX_BUSY_RETRIES &&
          (IDEReadStatus(Address) & IDE_SR_DRQ); RetryCount++)
        ;
      if (!(IDEReadStatus(Address) & IDE_SR_BUSY)) 
        {
          return 0;
        }
    }
}

//  -------------------------------------------  Nondiscardable statics

//    IDEDispatchOpenClose
//
//  DESCRIPTION:
//    Answer requests for Open/Close calls: a null operation
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    Standard dispatch arguments
//
//  RETURNS:
//    NTSTATUS
//

static  NTSTATUS  
STDCALL IDEDispatchOpenClose(IN PDEVICE_OBJECT pDO, 
                     IN PIRP Irp) 
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = FILE_OPENED;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

//    IDEDispatchReadWrite
//
//  DESCRIPTION:
//    Answer requests for reads and writes
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    Standard dispatch arguments
//
//  RETURNS:
//    NTSTATUS
//

static  NTSTATUS  
STDCALL IDEDispatchReadWrite(IN PDEVICE_OBJECT pDO, 
                     IN PIRP Irp) 
{
  ULONG                  IrpInsertKey;
  LARGE_INTEGER          AdjustedOffset, AdjustedExtent, PartitionExtent, InsertKeyLI;
  PIO_STACK_LOCATION     IrpStack;
  PIDE_DEVICE_EXTENSION  DeviceExtension;

  IrpStack = IoGetCurrentIrpStackLocation(Irp);
  DeviceExtension = (PIDE_DEVICE_EXTENSION)pDO->DeviceExtension;

    //  Validate operation parameters
  AdjustedOffset = RtlEnlargedIntegerMultiply(DeviceExtension->Offset, 
                                              DeviceExtension->BytesPerSector);
DPRINT("Offset:%ld * BytesPerSector:%ld  = AdjOffset:%ld:%ld\n",
       DeviceExtension->Offset, 
       DeviceExtension->BytesPerSector,
       (unsigned long) AdjustedOffset.u.HighPart,
       (unsigned long) AdjustedOffset.u.LowPart);
DPRINT("AdjOffset:%ld:%ld + ByteOffset:%ld:%ld\n",
       (unsigned long) AdjustedOffset.u.HighPart,
       (unsigned long) AdjustedOffset.u.LowPart,
       (unsigned long) IrpStack->Parameters.Read.ByteOffset.u.HighPart,
       (unsigned long) IrpStack->Parameters.Read.ByteOffset.u.LowPart);
  AdjustedOffset = RtlLargeIntegerAdd(AdjustedOffset, 
                                      IrpStack->Parameters.Read.ByteOffset);
DPRINT(" = AdjOffset:%ld:%ld\n",
       (unsigned long) AdjustedOffset.u.HighPart,
       (unsigned long) AdjustedOffset.u.LowPart);
  AdjustedExtent = RtlLargeIntegerAdd(AdjustedOffset, 
                                      RtlConvertLongToLargeInteger(IrpStack->Parameters.Read.Length));
DPRINT("AdjOffset:%ld:%ld + Length:%ld = AdjExtent:%ld:%ld\n",
       (unsigned long) AdjustedOffset.u.HighPart,
       (unsigned long) AdjustedOffset.u.LowPart,
       IrpStack->Parameters.Read.Length,
       (unsigned long) AdjustedExtent.u.HighPart,
       (unsigned long) AdjustedExtent.u.LowPart);
    /*FIXME: this assumption will fail on drives bigger than 1TB */
  PartitionExtent.QuadPart = DeviceExtension->Offset + DeviceExtension->Size;
  PartitionExtent = RtlExtendedIntegerMultiply(PartitionExtent, 
                                               DeviceExtension->BytesPerSector);
  if ((AdjustedExtent.QuadPart > PartitionExtent.QuadPart) ||
      (IrpStack->Parameters.Read.Length & (DeviceExtension->BytesPerSector - 1))) 
    {
      DPRINT("Request failed on bad parameters\n",0);
      DPRINT("AdjustedExtent=%d:%d PartitionExtent=%d:%d ReadLength=%d\n", 
             (unsigned int) AdjustedExtent.u.HighPart,
             (unsigned int) AdjustedExtent.u.LowPart,
             (unsigned int) PartitionExtent.u.HighPart,
             (unsigned int) PartitionExtent.u.LowPart,
             IrpStack->Parameters.Read.Length);
      Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return  STATUS_INVALID_PARAMETER;
    }

    //  Adjust operation to absolute sector offset
  IrpStack->Parameters.Read.ByteOffset = AdjustedOffset;

    //  Start the packet and insert the request in order of sector offset
  assert(DeviceExtension->BytesPerSector == 512);
  InsertKeyLI = RtlLargeIntegerShiftRight(IrpStack->Parameters.Read.ByteOffset, 9); 
  IrpInsertKey = InsertKeyLI.u.LowPart;
  IoStartPacket(DeviceExtension->DeviceObject, Irp, &IrpInsertKey, NULL);
   
  DPRINT("Returning STATUS_PENDING\n");
  return  STATUS_PENDING;
}

//    IDEDispatchDeviceControl
//
//  DESCRIPTION:
//    Answer requests for device control calls
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    Standard dispatch arguments
//
//  RETURNS:
//    NTSTATUS
//

static NTSTATUS STDCALL
IDEDispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
  NTSTATUS  RC;
  ULONG     ControlCode, InputLength, OutputLength;
  PIO_STACK_LOCATION     IrpStack;
  PIDE_DEVICE_EXTENSION  DeviceExtension;
  PIDE_DEVICE_EXTENSION  DiskDeviceExtension;
  CCHAR Increment;

  RC = STATUS_SUCCESS;
  IrpStack = IoGetCurrentIrpStackLocation(Irp);
  ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
  InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
  OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
  DeviceExtension = (PIDE_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
  DiskDeviceExtension = (PIDE_DEVICE_EXTENSION)DeviceExtension->DiskDeviceExtension;
  Increment = IO_NO_INCREMENT;

    //  A huge switch statement in a Windows program?! who would have thought?
  switch (ControlCode)
    {
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
      if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_GEOMETRY))
        {
          Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        }
      else
        {
          PDISK_GEOMETRY Geometry;

          Geometry = (PDISK_GEOMETRY) Irp->AssociatedIrp.SystemBuffer;

          Geometry->MediaType = FixedMedia;
          Geometry->Cylinders.QuadPart = DiskDeviceExtension->LogicalCylinders;
          Geometry->TracksPerCylinder = DiskDeviceExtension->SectorsPerLogCyl /
              DiskDeviceExtension->SectorsPerLogTrk;
          Geometry->SectorsPerTrack = DiskDeviceExtension->SectorsPerLogTrk;
          Geometry->BytesPerSector = DiskDeviceExtension->BytesPerSector;

          Irp->IoStatus.Status = STATUS_SUCCESS;
          Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
        }
      break;

    case IOCTL_DISK_GET_PARTITION_INFO:
    case IOCTL_DISK_SET_PARTITION_INFO:
      RC = STATUS_INVALID_DEVICE_REQUEST;
      Irp->IoStatus.Status = RC;
      Irp->IoStatus.Information = 0;
      break;

    case IOCTL_DISK_GET_DRIVE_LAYOUT:
      if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength <
          sizeof(DRIVE_LAYOUT_INFORMATION))
        {
          Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        }
      else
        {
          PDRIVE_LAYOUT_INFORMATION PartitionList;

          RC = IoReadPartitionTable(DiskDeviceExtension->DeviceObject,
                                    DiskDeviceExtension->BytesPerSector,
                                    FALSE,
                                    &PartitionList);
          if (!NT_SUCCESS(RC))
            {
              Irp->IoStatus.Status = RC;
            }
          else
            {
              ULONG BufferSize;

              BufferSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION,
                                        PartitionEntry[0]);
              BufferSize += PartitionList->PartitionCount * sizeof(PARTITION_INFORMATION);

              if (BufferSize > IrpStack->Parameters.DeviceIoControl.OutputBufferLength)
                {
                  Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                }
              else
                {
                  RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer,
                                PartitionList,
                                BufferSize);
                  Irp->IoStatus.Status = STATUS_SUCCESS;
                  Irp->IoStatus.Information = BufferSize;
                }
              ExFreePool(PartitionList);
            }
        }
        Increment = IO_DISK_INCREMENT;
        break;

    case IOCTL_DISK_SET_DRIVE_LAYOUT:
    case IOCTL_DISK_VERIFY:
    case IOCTL_DISK_FORMAT_TRACKS:
    case IOCTL_DISK_PERFORMANCE:
    case IOCTL_DISK_IS_WRITABLE:
    case IOCTL_DISK_LOGGING:
    case IOCTL_DISK_FORMAT_TRACKS_EX:
    case IOCTL_DISK_HISTOGRAM_STRUCTURE:
    case IOCTL_DISK_HISTOGRAM_DATA:
    case IOCTL_DISK_HISTOGRAM_RESET:
    case IOCTL_DISK_REQUEST_STRUCTURE:
    case IOCTL_DISK_REQUEST_DATA:

      //  If we get here, something went wrong.  inform the requestor
    default:
      RC = STATUS_INVALID_DEVICE_REQUEST;
      Irp->IoStatus.Status = RC;
      Irp->IoStatus.Information = 0;
      break;
    }

  IoCompleteRequest(Irp, Increment);

  return  RC;
}

//    IDEStartIo
//
//  DESCRIPTION:
//    Get the next requested I/O packet started
//
//  RUN LEVEL:
//    DISPATCH_LEVEL
//
//  ARGUMENTS:
//    Dispatch routine standard arguments
//
//  RETURNS:
//    NTSTATUS
//

static  VOID  
STDCALL IDEStartIo(IN PDEVICE_OBJECT DeviceObject, 
           IN PIRP Irp) 
{
  LARGE_INTEGER              SectorLI;
  PIO_STACK_LOCATION         IrpStack;
  PIDE_DEVICE_EXTENSION      DeviceExtension;
  KIRQL                      OldIrql;
  
  DPRINT("IDEStartIo() called!\n");
  
  IrpStack = IoGetCurrentIrpStackLocation(Irp);
  DeviceExtension = (PIDE_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // FIXME: implement the supported functions

  switch (IrpStack->MajorFunction) 
    {
    case IRP_MJ_READ:
    case IRP_MJ_WRITE:
      DeviceExtension->Operation = IrpStack->MajorFunction;
      DeviceExtension->BytesRequested = IrpStack->Parameters.Read.Length;
      assert(DeviceExtension->BytesPerSector == 512);
      SectorLI = RtlLargeIntegerShiftRight(IrpStack->Parameters.Read.ByteOffset, 9);
      DeviceExtension->StartingSector = SectorLI.u.LowPart;
      if (DeviceExtension->BytesRequested > DeviceExtension->BytesPerSector * 
          IDE_MAX_SECTORS_PER_XFER) 
        {
          DeviceExtension->BytesToTransfer = DeviceExtension->BytesPerSector * 
              IDE_MAX_SECTORS_PER_XFER;
        } 
      else 
        {
          DeviceExtension->BytesToTransfer = DeviceExtension->BytesRequested;
        }
      DeviceExtension->BytesRequested -= DeviceExtension->BytesToTransfer;
      DeviceExtension->SectorsTransferred = 0;
      DeviceExtension->TargetAddress = (BYTE *)MmGetSystemAddressForMdl(Irp->MdlAddress);
      KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
      IoAllocateController(DeviceExtension->ControllerObject,
                           DeviceObject, 
                           IDEAllocateController, 
                           NULL);
      KeLowerIrql(OldIrql);
      break;

    default:
      Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
      Irp->IoStatus.Information = 0;
      KeBugCheck((ULONG)Irp);
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      IoStartNextPacket(DeviceObject, FALSE);
      break;
    }
  DPRINT("IDEStartIo() finished!\n");
}

//    IDEAllocateController

static IO_ALLOCATION_ACTION STDCALL
IDEAllocateController(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp,
                      IN PVOID MapRegisterBase,
                      IN PVOID Ccontext) 
{
  PIDE_DEVICE_EXTENSION  DeviceExtension;
  PIDE_CONTROLLER_EXTENSION  ControllerExtension;

  DeviceExtension = (PIDE_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
  ControllerExtension = (PIDE_CONTROLLER_EXTENSION) 
      DeviceExtension->ControllerObject->ControllerExtension;
  ControllerExtension->CurrentIrp = Irp;
  ControllerExtension->Retries = 0;
  return KeSynchronizeExecution(ControllerExtension->Interrupt, 
                                IDEStartController,
                                DeviceExtension) ? KeepObject : 
                                  DeallocateObject;
}

//    IDEStartController

BOOLEAN STDCALL
IDEStartController(IN OUT PVOID Context)
{
  BYTE  SectorCnt, SectorNum, CylinderLow, CylinderHigh;
  BYTE  DrvHead, Command;
  BYTE  Status;
  int  Retries;
  ULONG  StartingSector;
  PIDE_DEVICE_EXTENSION  DeviceExtension;
  PIDE_CONTROLLER_EXTENSION  ControllerExtension;
  PIRP  Irp;

  DeviceExtension = (PIDE_DEVICE_EXTENSION) Context;
  ControllerExtension = (PIDE_CONTROLLER_EXTENSION)
      DeviceExtension->ControllerObject->ControllerExtension;
  ControllerExtension->OperationInProgress = TRUE;
  ControllerExtension->DeviceForOperation = DeviceExtension;

    //  Write controller registers to start opteration
  StartingSector = DeviceExtension->StartingSector;
  SectorCnt = DeviceExtension->BytesToTransfer / 
      DeviceExtension->BytesPerSector;
  if (DeviceExtension->LBASupported) 
    {
      SectorNum = StartingSector & 0xff;
      CylinderLow = (StartingSector >> 8) & 0xff;
      CylinderHigh = (StartingSector >> 16) & 0xff;
      DrvHead = ((StartingSector >> 24) & 0x0f) | 
          (DeviceExtension->UnitNumber ? IDE_DH_DRV1 : 0) |
          IDE_DH_LBA;
    } 
  else 
    {
      SectorNum = (StartingSector % DeviceExtension->SectorsPerLogTrk) + 1;
      StartingSector /= DeviceExtension->SectorsPerLogTrk;
      DrvHead = (StartingSector % DeviceExtension->LogicalHeads) | 
          (DeviceExtension->UnitNumber ? IDE_DH_DRV1 : 0);
      StartingSector /= DeviceExtension->LogicalHeads;
      CylinderLow = StartingSector & 0xff;
      CylinderHigh = StartingSector >> 8;
    }
  Command = DeviceExtension->Operation == IRP_MJ_READ ? 
     IDE_CMD_READ : IDE_CMD_WRITE;
  if (DrvHead & IDE_DH_LBA) 
    {
      DPRINT("%s:BUS=%04x:DRV=%d:LBA=1:BLK=%08d:SC=%02x:CM=%02x\n",
             DeviceExtension->Operation == IRP_MJ_READ ? "READ" : "WRITE",
             ControllerExtension->CommandPortBase,
             DrvHead & IDE_DH_DRV1 ? 1 : 0, 
             ((DrvHead & 0x0f) << 24) +
             (CylinderHigh << 16) + (CylinderLow << 8) + SectorNum,
             SectorCnt, 
             Command);
    } 
  else 
    {
      DPRINT("%s:BUS=%04x:DRV=%d:LBA=0:CH=%02x:CL=%02x:HD=%01x:SN=%02x:SC=%02x:CM=%02x\n",
             DeviceExtension->Operation == IRP_MJ_READ ? "READ" : "WRITE",
             ControllerExtension->CommandPortBase,
             DrvHead & IDE_DH_DRV1 ? 1 : 0, 
             CylinderHigh, 
             CylinderLow, 
             DrvHead & 0x0f, 
             SectorNum, 
             SectorCnt, 
             Command);
    }

  /*  wait for BUSY to clear  */
  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES; Retries++)
    {
      Status = IDEReadStatus(ControllerExtension->CommandPortBase);
      if (!(Status & IDE_SR_BUSY)) 
        {
          break;
        }
      KeStallExecutionProcessor(10);
    }
  DPRINT ("status=%02x\n", Status);
  DPRINT ("waited %ld usecs for busy to clear\n", Retries * 10);
  if (Retries >= IDE_MAX_BUSY_RETRIES)
    {
      DPRINT ("Drive is BUSY for too long\n");
      if (++ControllerExtension->Retries > IDE_MAX_CMD_RETRIES)
        {
          DbgPrint ("Max Retries on Drive reset reached, returning failure\n");
          Irp = ControllerExtension->CurrentIrp;
          Irp->IoStatus.Status = STATUS_DISK_OPERATION_FAILED;
          Irp->IoStatus.Information = 0;

          return FALSE;
        }
      else
        {
          DPRINT ("Beginning drive reset sequence\n");
          IDEBeginControllerReset(ControllerExtension);

          return TRUE;
        }
    }

  /*  Select the desired drive  */
  IDEWriteDriveHead(ControllerExtension->CommandPortBase, IDE_DH_FIXED | DrvHead);

  /*  wait for BUSY to clear and DRDY to assert */
  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES; Retries++)
    {
      Status = IDEReadStatus(ControllerExtension->CommandPortBase);
      if (!(Status & IDE_SR_BUSY) && (Status & IDE_SR_DRDY)) 
        {
          break;
        }
      KeStallExecutionProcessor(10);
    }
  DPRINT ("waited %ld usecs for busy to clear after drive select\n", Retries * 10);
  if (Retries >= IDE_MAX_BUSY_RETRIES)
    {
      DPRINT ("Drive is BUSY for too long after drive select\n");
      if (ControllerExtension->Retries++ > IDE_MAX_CMD_RETRIES)
        {
          DbgPrint ("Max Retries on Drive reset reached, returning failure\n");
          Irp = ControllerExtension->CurrentIrp;
          Irp->IoStatus.Status = STATUS_DISK_OPERATION_FAILED;
          Irp->IoStatus.Information = 0;

          return FALSE;
        }
      else
        {
          DPRINT ("Beginning drive reset sequence\n");
          IDEBeginControllerReset(ControllerExtension);

          return TRUE;
        }
    }

  /*  Setup command parameters  */
  IDEWritePrecomp(ControllerExtension->CommandPortBase, 0);
  IDEWriteSectorCount(ControllerExtension->CommandPortBase, SectorCnt);
  IDEWriteSectorNum(ControllerExtension->CommandPortBase, SectorNum);
  IDEWriteCylinderHigh(ControllerExtension->CommandPortBase, CylinderHigh);
  IDEWriteCylinderLow(ControllerExtension->CommandPortBase, CylinderLow);
  IDEWriteDriveHead(ControllerExtension->CommandPortBase, IDE_DH_FIXED | DrvHead);

  /*  Issue command to drive  */
  IDEWriteCommand(ControllerExtension->CommandPortBase, Command);
  ControllerExtension->TimerState = IDETimerCmdWait;
  ControllerExtension->TimerCount = IDE_CMD_TIMEOUT;
  
  if (DeviceExtension->Operation == IRP_MJ_WRITE) 
    {

        //  Wait for controller ready
      for (Retries = 0; Retries < IDE_MAX_WRITE_RETRIES; Retries++) 
        {
          BYTE  Status = IDEReadStatus(ControllerExtension->CommandPortBase);
          if (!(Status & IDE_SR_BUSY) || (Status & IDE_SR_ERR)) 
            {
              break;
            }
          KeStallExecutionProcessor(10);
        }
      if (Retries >= IDE_MAX_BUSY_RETRIES)
        {
          if (ControllerExtension->Retries++ > IDE_MAX_CMD_RETRIES)
            {
              Irp = ControllerExtension->CurrentIrp;
              Irp->IoStatus.Status = STATUS_DISK_OPERATION_FAILED;
              Irp->IoStatus.Information = 0;

              return FALSE;
            }
          else
            {
              IDEBeginControllerReset(ControllerExtension);

              return TRUE;
            }
        }

        //  Load the first sector of data into the controller
      IDEWriteBlock(ControllerExtension->CommandPortBase, 
                    DeviceExtension->TargetAddress,
                    IDE_SECTOR_BUF_SZ);
      DeviceExtension->TargetAddress += IDE_SECTOR_BUF_SZ;
      DeviceExtension->BytesToTransfer -= DeviceExtension->BytesPerSector;
      DeviceExtension->SectorsTransferred++;
    }
  DPRINT ("Command issued to drive, IDEStartController done\n");

  return  TRUE;
}

//    IDEBeginControllerReset

VOID 
IDEBeginControllerReset(PIDE_CONTROLLER_EXTENSION ControllerExtension)
{
  int Retries;

  DPRINT("Controller Reset initiated on %04x\n", 
         ControllerExtension->ControlPortBase);

    /*  Assert drive reset line  */
  DPRINT("Asserting reset line\n");
  IDEWriteDriveControl(ControllerExtension->ControlPortBase, IDE_DC_SRST);

    /*  Wait for BSY assertion  */
  DPRINT("Waiting for BSY assertion\n");
  for (Retries = 0; Retries < IDE_MAX_RESET_RETRIES; Retries++) 
    {
      BYTE Status = IDEReadStatus(ControllerExtension->CommandPortBase);
      if ((Status & IDE_SR_BUSY)) 
        {
          break;
        }
      KeStallExecutionProcessor(10);
    }
  if (Retries == IDE_MAX_RESET_RETRIES)
    {
      DPRINT("Timeout on BSY assertion\n");
    }

    /*  Negate drive reset line  */
  DPRINT("Negating reset line\n");
  IDEWriteDriveControl(ControllerExtension->ControlPortBase, 0);

  // FIXME: handle case of no device 0

    /*  Set timer to check for end of reset  */
  ControllerExtension->TimerState = IDETimerResetWaitForBusyNegate;
  ControllerExtension->TimerCount = IDE_RESET_BUSY_TIMEOUT;
}

//    IDEIsr
//
//  DESCIPTION:
//    Handle interrupts for IDE devices
//
//  RUN LEVEL:
//    DIRQL
//
//  ARGUMENTS:
//    IN  PKINTERRUPT  Interrupt       The interrupt level in effect
//    IN  PVOID        ServiceContext  The driver supplied context
//                                     (the controller extension)
//  RETURNS:
//    TRUE   This ISR handled the interrupt
//    FALSE  Another ISR must handle this interrupt

static BOOLEAN STDCALL
IDEIsr(IN PKINTERRUPT Interrupt,
       IN PVOID ServiceContext)
{
  BOOLEAN   IsLastBlock, AnErrorOccured, RequestIsComplete;
  BYTE     *TargetAddress;
  int       Retries;
  NTSTATUS  ErrorStatus;
  ULONG     ErrorInformation;
  PIRP  Irp;
  PIDE_DEVICE_EXTENSION  DeviceExtension;
  PIDE_CONTROLLER_EXTENSION  ControllerExtension;

  if (IDEInitialized == FALSE)
    {
      return FALSE;
    }
  DPRINT ("IDEIsr called\n");

  ControllerExtension = (PIDE_CONTROLLER_EXTENSION) ServiceContext;

    //  Read the status port to clear the interrtupt (even if it's not ours).
  ControllerExtension->DeviceStatus = IDEReadStatus(ControllerExtension->CommandPortBase);

    //  If the interrupt is not ours, get the heck outta dodge.
  if (!ControllerExtension->OperationInProgress)
    {
      return FALSE;
    }

  DeviceExtension = ControllerExtension->DeviceForOperation;
  IsLastBlock = FALSE;
  AnErrorOccured = FALSE;
  RequestIsComplete = FALSE;
  ErrorStatus = STATUS_SUCCESS;
  ErrorInformation = 0;

    //  Handle error condition if it exists
  if (ControllerExtension->DeviceStatus & IDE_SR_ERR) 
    {
      BYTE ErrorReg, SectorCount, SectorNum, CylinderLow, CylinderHigh;
      BYTE DriveHead;

        //  Log the error
      ErrorReg = IDEReadError(ControllerExtension->CommandPortBase);
      CylinderLow = IDEReadCylinderLow(ControllerExtension->CommandPortBase);
      CylinderHigh = IDEReadCylinderHigh(ControllerExtension->CommandPortBase);
      DriveHead = IDEReadDriveHead(ControllerExtension->CommandPortBase);
      SectorCount = IDEReadSectorCount(ControllerExtension->CommandPortBase);
      SectorNum = IDEReadSectorNum(ControllerExtension->CommandPortBase);
        // FIXME: should use the NT error logging facility
      DbgPrint ("IDE Error: OP:%02x STAT:%02x ERR:%02x CYLLO:%02x CYLHI:%02x SCNT:%02x SNUM:%02x\n", 
                DeviceExtension->Operation, 
                ControllerExtension->DeviceStatus, 
                ErrorReg, 
                CylinderLow,
                CylinderHigh, 
                SectorCount, 
                SectorNum);

        // FIXME: should retry the command and perhaps recalibrate the drive

        //  Set error status information
      AnErrorOccured = TRUE;
      ErrorStatus = STATUS_DISK_OPERATION_FAILED;
      ErrorInformation = 
          (((((((CylinderHigh << 8) + CylinderLow) * 
              DeviceExtension->LogicalHeads) + 
              (DriveHead % DeviceExtension->LogicalHeads)) * 
              DeviceExtension->SectorsPerLogTrk) + SectorNum - 1) -
          DeviceExtension->StartingSector) * DeviceExtension->BytesPerSector;
    } 
  else 
    {

        // Check controller and setup for next transfer
      switch (DeviceExtension->Operation) 
        {
        case  IRP_MJ_READ:

            //  Update controller/device state variables
          TargetAddress = DeviceExtension->TargetAddress;
          DeviceExtension->TargetAddress += DeviceExtension->BytesPerSector;
          DeviceExtension->BytesToTransfer -= DeviceExtension->BytesPerSector;
          DeviceExtension->SectorsTransferred++;

            //  Remember whether DRQ should be low at end (last block read)
          IsLastBlock = DeviceExtension->BytesToTransfer == 0;

            //  Wait for DRQ assertion
          for (Retries = 0; Retries < IDE_MAX_DRQ_RETRIES &&
              !(IDEReadStatus(ControllerExtension->CommandPortBase) & IDE_SR_DRQ);
              Retries++) 
            {
              KeStallExecutionProcessor(10);
            }

            //  Copy the block of data
          IDEReadBlock(ControllerExtension->CommandPortBase, 
                       TargetAddress,
                       IDE_SECTOR_BUF_SZ);

            //  check DRQ
          if (IsLastBlock) 
            {
              for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES &&
                  (IDEReadStatus(ControllerExtension->CommandPortBase) & IDE_SR_BUSY);
                  Retries++) 
                {
                  KeStallExecutionProcessor(10);
                }

                //  Check for data overrun
              if (IDEReadStatus(ControllerExtension->CommandPortBase) & IDE_SR_DRQ) 
                {
                  AnErrorOccured = TRUE;
                  ErrorStatus = STATUS_DATA_OVERRUN;
                  ErrorInformation = 0;
                } 
              else 
                {

                    //  Setup next transfer or set RequestIsComplete
                  if (DeviceExtension->BytesRequested > 
                      DeviceExtension->BytesPerSector * IDE_MAX_SECTORS_PER_XFER) 
                    {
                      DeviceExtension->StartingSector += DeviceExtension->SectorsTransferred;
                      DeviceExtension->SectorsTransferred = 0;
                      DeviceExtension->BytesToTransfer = 
                      DeviceExtension->BytesPerSector * IDE_MAX_SECTORS_PER_XFER;
                      DeviceExtension->BytesRequested -= DeviceExtension->BytesToTransfer;
                    } 
                  else if (DeviceExtension->BytesRequested > 0) 
                    {
                      DeviceExtension->StartingSector += DeviceExtension->SectorsTransferred;
                      DeviceExtension->SectorsTransferred = 0;
                      DeviceExtension->BytesToTransfer = DeviceExtension->BytesRequested;
                      DeviceExtension->BytesRequested -= DeviceExtension->BytesToTransfer;
                    } 
                  else
                    {
                      RequestIsComplete = TRUE;
                    }
                }
            }
          break;

        case IRP_MJ_WRITE:

            //  check DRQ
          if (DeviceExtension->BytesToTransfer == 0) 
            {
              for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES &&
                  (IDEReadStatus(ControllerExtension->CommandPortBase) & IDE_SR_BUSY);
                   Retries++) 
                {
                  KeStallExecutionProcessor(10);
                }

                //  Check for data overrun
              if (IDEReadStatus(ControllerExtension->CommandPortBase) & IDE_SR_DRQ) 
                {
                  AnErrorOccured = TRUE;
                  ErrorStatus = STATUS_DATA_OVERRUN;
                  ErrorInformation = 0;
                } 
              else 
                {

                    //  Setup next transfer or set RequestIsComplete
                  IsLastBlock = TRUE;
                  if (DeviceExtension->BytesRequested > 
                      DeviceExtension->BytesPerSector * IDE_MAX_SECTORS_PER_XFER) 
                    {
                      DeviceExtension->StartingSector += DeviceExtension->SectorsTransferred;
                      DeviceExtension->SectorsTransferred = 0;
                      DeviceExtension->BytesToTransfer = 
                          DeviceExtension->BytesPerSector * IDE_MAX_SECTORS_PER_XFER;
                      DeviceExtension->BytesRequested -= DeviceExtension->BytesToTransfer;
                    } 
                  else if (DeviceExtension->BytesRequested > 0) 
                    {
                      DeviceExtension->StartingSector += DeviceExtension->SectorsTransferred;
                      DeviceExtension->SectorsTransferred = 0;
                      DeviceExtension->BytesToTransfer = DeviceExtension->BytesRequested;
                      DeviceExtension->BytesRequested -= DeviceExtension->BytesToTransfer;
                    } 
                  else 
                    {
                      RequestIsComplete = TRUE;
                    }
                }
            } 
          else 
            {

                //  Update controller/device state variables
              TargetAddress = DeviceExtension->TargetAddress;
              DeviceExtension->TargetAddress += DeviceExtension->BytesPerSector;
              DeviceExtension->BytesToTransfer -= DeviceExtension->BytesPerSector;
              DeviceExtension->SectorsTransferred++;

                //  Write block to controller
              IDEWriteBlock(ControllerExtension->CommandPortBase, 
                            TargetAddress,
                            IDE_SECTOR_BUF_SZ);
            }
          break;
        }
    }

  //  If there was an error or the request is done, complete the packet
  if (AnErrorOccured || RequestIsComplete) 
    {
      //  Set the return status and info values
      Irp = ControllerExtension->CurrentIrp;
      Irp->IoStatus.Status = ErrorStatus;
      Irp->IoStatus.Information = ErrorInformation;

      //  Clear out controller fields
      ControllerExtension->OperationInProgress = FALSE;
      ControllerExtension->DeviceStatus = 0;

      //  Queue the Dpc to finish up
      IoRequestDpc(DeviceExtension->DeviceObject, 
                   Irp, 
                   ControllerExtension);
    } 
  else if (IsLastBlock)
    {
      //  Else more data is needed, setup next device I/O
      IDEStartController((PVOID)DeviceExtension);
    }

  return TRUE;
}

//    IDEDpcForIsr
//  DESCRIPTION:
//
//  RUN LEVEL:
//
//  ARGUMENTS:
//    IN PKDPC          Dpc
//    IN PDEVICE_OBJECT DpcDeviceObject
//    IN PIRP           DpcIrp
//    IN PVOID          DpcContext
//
static VOID
IDEDpcForIsr(IN PKDPC Dpc,
             IN PDEVICE_OBJECT DpcDeviceObject,
             IN PIRP DpcIrp,
             IN PVOID DpcContext)
{
  DPRINT("IDEDpcForIsr()\n");
  IDEFinishOperation((PIDE_CONTROLLER_EXTENSION) DpcContext);
}

//    IDEFinishOperation

static VOID
IDEFinishOperation(PIDE_CONTROLLER_EXTENSION ControllerExtension)
{
  PIDE_DEVICE_EXTENSION DeviceExtension;
  PIRP Irp;
  ULONG Operation;

  DeviceExtension = ControllerExtension->DeviceForOperation;
  Irp = ControllerExtension->CurrentIrp;
  Operation = DeviceExtension->Operation;
  ControllerExtension->OperationInProgress = FALSE;
  ControllerExtension->DeviceForOperation = 0;
  ControllerExtension->CurrentIrp = 0;

  //  Deallocate the controller
  IoFreeController(DeviceExtension->ControllerObject);

  //  Start the next packet
  IoStartNextPacketByKey(DeviceExtension->DeviceObject, 
                         FALSE, 
                         DeviceExtension->StartingSector);

  //  Issue completion of the current packet
  IoCompleteRequest(Irp, IO_DISK_INCREMENT);

  //  Flush cache if necessary
  if (Operation == IRP_MJ_READ) 
    {
      KeFlushIoBuffers(Irp->MdlAddress, TRUE, FALSE);
    }
}

//    IDEIoTimer
//  DESCRIPTION:
//    This function handles timeouts and other time delayed processing
//
//  RUN LEVEL:
//
//  ARGUMENTS:
//    IN  PDEVICE_OBJECT  DeviceObject  Device object registered with timer
//    IN  PVOID           Context       the Controller extension for the
//                                      controller the device is on
//
static VOID STDCALL
IDEIoTimer(PDEVICE_OBJECT DeviceObject,
           PVOID Context)
{
  PIDE_CONTROLLER_EXTENSION  ControllerExtension;

    //  Setup Extension pointer
  ControllerExtension = (PIDE_CONTROLLER_EXTENSION) Context;
  DPRINT("Timer activated for %04lx\n", ControllerExtension->CommandPortBase);

    //  Handle state change if necessary
  switch (ControllerExtension->TimerState) 
    {
      case IDETimerResetWaitForBusyNegate:
        if (!(IDEReadStatus(ControllerExtension->CommandPortBase) & 
            IDE_SR_BUSY))
          {
            DPRINT("Busy line has negated, waiting for DRDY assert\n");
            ControllerExtension->TimerState = IDETimerResetWaitForDrdyAssert;
            ControllerExtension->TimerCount = IDE_RESET_DRDY_TIMEOUT;
            return;
          }
        break;
        
      case IDETimerResetWaitForDrdyAssert:
        if (IDEReadStatus(ControllerExtension->CommandPortBase) & 
            IDE_SR_DRQ)
          {
            DPRINT("DRDY has asserted, reset complete\n");
            ControllerExtension->TimerState = IDETimerIdle;
            ControllerExtension->TimerCount = 0;

              // FIXME: get diagnostic code from drive 0

              /*  Start current packet command again  */
            if (!KeSynchronizeExecution(ControllerExtension->Interrupt, 
                                        IDEStartController,
                                        ControllerExtension->DeviceForOperation))
              {
                IDEFinishOperation(ControllerExtension);
              }
            return;
          }
        break;

      default:
        break;
    }

    //  If we're counting down, then count.
  if (ControllerExtension->TimerCount > 0) 
    {
      ControllerExtension->TimerCount--;

      //  Else we'll check the state and process if necessary
    } 
  else 
    {
      switch (ControllerExtension->TimerState) 
        {
          case IDETimerIdle:
            break;

          case IDETimerCmdWait:
              /*  Command timed out, reset drive and try again or fail  */
            DPRINT("Timeout waiting for command completion\n");
            if (++ControllerExtension->Retries > IDE_MAX_CMD_RETRIES)
              {
		 if (ControllerExtension->CurrentIrp != NULL)
		   {
                      DbgPrint ("Max retries has been reached, IRP finished with error\n");
		      ControllerExtension->CurrentIrp->IoStatus.Status = STATUS_IO_TIMEOUT;
		      ControllerExtension->CurrentIrp->IoStatus.Information = 0;
		      IDEFinishOperation(ControllerExtension);
		   }
                ControllerExtension->TimerState = IDETimerIdle;
                ControllerExtension->TimerCount = 0;
              }
            else
              {
                IDEBeginControllerReset(ControllerExtension);
              }
            break;

          case IDETimerResetWaitForBusyNegate:
          case IDETimerResetWaitForDrdyAssert:
            if (ControllerExtension->CurrentIrp != NULL)
              {
                DbgPrint ("Timeout waiting for drive reset, giving up on IRP\n");
                ControllerExtension->CurrentIrp->IoStatus.Status = 
                  STATUS_IO_TIMEOUT;
                ControllerExtension->CurrentIrp->IoStatus.Information = 0;
                IDEFinishOperation(ControllerExtension);
              }
            ControllerExtension->TimerState = IDETimerIdle;
            ControllerExtension->TimerCount = 0;
            break;
        }
    }
}


