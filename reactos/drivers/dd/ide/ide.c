/*
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

#include <ddk/ntddk.h>

//  --------------------------------  This stuff should be defined in NTDDK.H

typedef DISK_GEOMETRY *PDISK_GEOMETRY;

//  -------------------------------------------------------------------------

#include <internal/i386/io.h>
#include <string.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

#include "ide.h"
#include "idep.h"
#include "partitio.h"

#define  VERSION  "V0.1.4"

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

#define  IDE_MAX_CONTROLLERS  4
IDE_CONTROLLER_PARAMETERS Controllers[IDE_MAX_CONTROLLERS] = 
{
  {0x01f0, 8, 0x03f6, 1, 14, 14, 15, LevelSensitive, 0xffff},
  {0x0170, 8, 0x0376, 1, 15, 15, 15, LevelSensitive, 0xffff},
  {0x01E8, 8, 0x03ee, 1, 11, 11, 15, LevelSensitive, 0xffff},
  {0x0168, 8, 0x036e, 1, 10, 10, 15, LevelSensitive, 0xffff}
};

static BOOLEAN IDEInitialized = FALSE;
static int TotalPartitions = 0;

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
static BOOLEAN IDEGetPartitionTable(IN int CommandPort, 
                                    IN int DriveNum, 
                                    IN int Offset, 
                                    IN PIDE_DRIVE_IDENTIFY DrvParms,
                                    PARTITION *PartitionTable);
static NTSTATUS IDECreateDevice(IN PDRIVER_OBJECT DriverObject, 
                                IN PCHAR DeviceName, 
                                OUT PDEVICE_OBJECT *DeviceObject, 
                                IN PCONTROLLER_OBJECT ControllerObject, 
                                IN int UnitNumber,
                                IN char *Win32Alias,
                                IN PIDE_DRIVE_IDENTIFY DrvParms, 
                                IN DWORD Offset, 
                                IN DWORD Size);
static int IDEPolledRead(IN WORD Address, 
                         IN BYTE PreComp, 
                         IN BYTE SectorCnt, 
                         IN BYTE SectorNum , 
                         IN BYTE CylinderLow, 
                         IN BYTE CylinderHigh, 
                         IN BYTE DrvHead, 
                         IN BYTE Command, 
                         OUT BYTE *Buffer);
static NTSTATUS IDEDispatchOpenClose(IN PDEVICE_OBJECT pDO, IN PIRP Irp);
static NTSTATUS IDEDispatchReadWrite(IN PDEVICE_OBJECT pDO, IN PIRP Irp);
static NTSTATUS IDEDispatchDeviceControl(IN PDEVICE_OBJECT pDO, IN PIRP Irp);
static VOID IDEStartIo(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
static IO_ALLOCATION_ACTION IDEAllocateController(IN PDEVICE_OBJECT DeviceObject,
                                                  IN PIRP Irp, 
                                                  IN PVOID MapRegisterBase, 
                                                  IN PVOID Ccontext);
static BOOLEAN IDEStartController(IN OUT PVOID Context);
VOID IDEBeginControllerReset(PIDE_CONTROLLER_EXTENSION ControllerExtension);
static BOOLEAN IDEIsr(IN PKINTERRUPT Interrupt, IN PVOID ServiceContext);
static VOID IDEDpcForIsr(IN PKDPC Dpc, 
                         IN PDEVICE_OBJECT DpcDeviceObject,
                         IN PIRP DpcIrp, 
                         IN PVOID DpcContext);
static VOID IDEFinishOperation(PIDE_CONTROLLER_EXTENSION ControllerExtension);
static VOID IDEIoTimer(PDEVICE_OBJECT DeviceObject, PVOID Context);
 
//  ----------------------------------------------------------------  Inlines

extern inline void 
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
      WeGotSomeDisks |= IDECreateController(DriverObject, 
          &Controllers[ControllerIdx], ControllerIdx);
    }

  if (WeGotSomeDisks)
    {
      IDEInitialized = TRUE;
    }

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
      DPRINT("Could not find controller %d at %04lx\n",
          ControllerIdx, ControllerParams->CommandPortBase);
      return FALSE;
    }

  ControllerObject = IoCreateController(sizeof(IDE_CONTROLLER_EXTENSION));
  if (ControllerObject == NULL) 
    {
      DPRINT("Could not create controller object for controller %d\n",
          ControllerIdx);
      return FALSE;
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
      DPRINT("Could not Connect Interrupt %d\n", ControllerExtension->Vector);
      IoDeleteController(ControllerObject);
      return FALSE;
    }

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
      DPRINT("Did not find any devices for controller %d\n", ControllerIdx);
      IoDisconnectInterrupt(ControllerExtension->Interrupt);
      IoDeleteController(ControllerObject);
    }
  else
    {
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

    //  Wait for BUSY assertion
  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES; Retries++) 
    {
      if (IDEReadStatus(CommandPort) & IDE_SR_BUSY)
        {
          break;
        }
      KeStallExecutionProcessor(10);
    }
   CHECKPOINT;
  if (Retries >= IDE_MAX_BUSY_RETRIES)
    {
      return FALSE;
    }

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
  return  IDEReadError(CommandPort) == 1 &&
          IDEReadSectorCount(CommandPort) == 1 &&
          IDEReadSectorNum(CommandPort) == 1 &&
          IDEReadCylinderLow(CommandPort) == 0 &&
          IDEReadCylinderHigh(CommandPort) == 0 &&
          (IDEReadDriveHead(CommandPort) & 0x1F) == 0;
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
  BOOLEAN                CreatedDevices;
  char                   RawDeviceName[IDE_MAX_NAME_LENGTH];
  char                   PrimaryDeviceName[IDE_MAX_NAME_LENGTH];
  char                   LogicalDeviceName[IDE_MAX_NAME_LENGTH];
  char                   Win32AliasName[IDE_MAX_NAME_LENGTH];
  int                    CommandPort, PartitionIdx, PartitionNum;
  int                    ExtPartitionIdx, ExtOffset;
  NTSTATUS               RC;
  IDE_DRIVE_IDENTIFY     DrvParms;
  PDEVICE_OBJECT         RawDeviceObject;
  PDEVICE_OBJECT         PrimaryDeviceObject;
  PDEVICE_OBJECT         LogicalDeviceObject;
  PIDE_DEVICE_EXTENSION  RawDeviceExtension;
  PARTITION              PrimaryPartitionTable[4], *p;
  PARTITION              ExtendedPartitionTable[4], *ep;
  char                   DeviceDirName[IDE_MAX_NAME_LENGTH + 1];
  ANSI_STRING            AnsiDeviceDirName;
  UNICODE_STRING         UnicodeDeviceDirName;
  OBJECT_ATTRIBUTES      DeviceDirAttributes;
  HANDLE                 Handle;

    //  Copy I/O port offsets for convenience
  CommandPort = ControllerExtension->CommandPortBase;
//  ControlPort = ControllerExtension->ControlPortBase;
  DPRINT("probing IDE controller %d Addr %04lx Drive %d\n",
         ControllerExtension->Number, 
         CommandPort, 
         DriveIdx);

    //  Get the Drive Identification Data
  if (!IDEGetDriveIdentification(CommandPort, DriveIdx, &DrvParms)) 
    {
      DPRINT("Giving up on drive %d on controller %d...\n", 
             DriveIdx, 
             ControllerExtension->Number);
      return  FALSE;
    }

  CreatedDevices = FALSE;

    //  Create the harddisk device directory
  strcpy(DeviceDirName, IDE_NT_ROOTDIR_NAME);
  strcat(DeviceDirName, IDE_NT_DEVICE_NAME);
  DeviceDirName[strlen(DeviceDirName) + 1] = '\0';
  DeviceDirName[strlen(DeviceDirName)] = '0' + HarddiskIdx;
  RtlInitAnsiString(&AnsiDeviceDirName, DeviceDirName);
  RC = RtlAnsiStringToUnicodeString(&UnicodeDeviceDirName, 
                                    &AnsiDeviceDirName, 
                                    TRUE);
  if (!NT_SUCCESS(RC))
    {
      DPRINT("Could not convert ansi to unicode for device dir\n", 0);
      return  FALSE;
    }
  InitializeObjectAttributes(&DeviceDirAttributes, 
                             &UnicodeDeviceDirName, 
                             0, 
                             NULL, 
                             NULL);
  RC = ZwCreateDirectoryObject(&Handle, 0, &DeviceDirAttributes);
  if (!NT_SUCCESS(RC))
    {
      DPRINT("Could not create device dir object\n", 0);
      return FALSE;
    }
  RtlFreeUnicodeString(&UnicodeDeviceDirName);

    //  Create the raw device
  strcpy(RawDeviceName, IDE_NT_ROOTDIR_NAME);
  strcat(RawDeviceName, IDE_NT_DEVICE_NAME);
  RawDeviceName[strlen(RawDeviceName) + 1] = '\0';
  RawDeviceName[strlen(RawDeviceName)] = '0' + HarddiskIdx;
  strcat(RawDeviceName, IDE_NT_PARTITION_NAME);
  RawDeviceName[strlen(RawDeviceName) + 1] = '\0';
  RawDeviceName[strlen(RawDeviceName)] = '0';
  RC = IDECreateDevice(DriverObject, 
                       RawDeviceName, 
                       &RawDeviceObject, 
                       ControllerObject, 
                       DriveIdx,
                       NULL,
                       &DrvParms,
                       0, 
                       DrvParms.LogicalCyls * DrvParms.LogicalHeads * DrvParms.SectorsPerTrack);
  if (!NT_SUCCESS(RC)) 
    {
      DPRINT("IDECreateDevice call failed for raw device\n",0);
    } 
  else 
    {
      CreatedDevices = TRUE;
      RawDeviceExtension = (PIDE_DEVICE_EXTENSION) 
          RawDeviceObject->DeviceExtension;

        //  Initialze the controller timer here (since it has to be
        //    tied to a device)...
      if (DriveIdx == 0) 
        {
          ControllerExtension->TimerState = IDETimerIdle;
          ControllerExtension->TimerCount = 0;
          ControllerExtension->TimerDevice = RawDeviceObject;
          IoInitializeTimer(RawDeviceObject, IDEIoTimer, ControllerExtension);
        }
    }

    //  Get Main partition table for device
  PartitionNum = 1;
  if (!IDEGetPartitionTable(CommandPort, DriveIdx, 0, &DrvParms, PrimaryPartitionTable)) 
    {
      DPRINT("drive %d controller %d: Could not get primary partition table\n", 
             DriveIdx, 
             ControllerExtension->Number);
    } 
  else 
    {

        //  build devices for all partitions in table
      DPRINT("partitions on %s:\n", DeviceDirName);
      for (PartitionIdx = 0; PartitionIdx < 4; PartitionIdx++) 
        {

            //  copy partition pointer for convenience
          p = &PrimaryPartitionTable[PartitionIdx];

            //  if the partition entry is in use, create a device for it
          if (PartitionIsSupported(p)) 
            {
              DPRINT("%s ptbl entry:%d type:%02x Offset:%d Size:%d\n",
                     DeviceDirName,
                     PartitionIdx, 
                     p->PartitionType, 
                     p->StartingBlock, 
                     p->SectorCount);

                //  Create Device for partition
              strcpy(PrimaryDeviceName, IDE_NT_ROOTDIR_NAME);
              strcat(PrimaryDeviceName, IDE_NT_DEVICE_NAME);
              PrimaryDeviceName[strlen(PrimaryDeviceName) + 1] = '\0';
              PrimaryDeviceName[strlen(PrimaryDeviceName)] = '0' + HarddiskIdx;
              strcat(PrimaryDeviceName, IDE_NT_PARTITION_NAME);
              PrimaryDeviceName[strlen(PrimaryDeviceName) + 1] = '\0';
              PrimaryDeviceName[strlen(PrimaryDeviceName)] = '0' + PartitionNum++;
              strcpy(Win32AliasName, "\\??\\ :");
              Win32AliasName[4] = 'C' + TotalPartitions;
              TotalPartitions++;
              RC = IDECreateDevice(DriverObject, 
                                   PrimaryDeviceName, 
                                   &PrimaryDeviceObject, 
                                   ControllerObject, 
                                   DriveIdx,
                                   Win32AliasName,
                                   &DrvParms,
                                   p->StartingBlock, 
                                   p->SectorCount);
              if (!NT_SUCCESS(RC)) 
                {
                  DPRINT("IDECreateDevice call failed\n",0);
                  break;
                }

                //  Create devices for logical partitions within an extended partition
            } 
          else if (PartitionIsExtended(p)) 
            {
              ExtOffset = p->StartingBlock;

                //  Get the Extended partition table
              if (!IDEGetPartitionTable(CommandPort, 
                                        DriveIdx, 
                                        ExtOffset, 
                                        &DrvParms,
                                        ExtendedPartitionTable)) 
                {
                  DPRINT("Harddisk%d: could not get extended partition table at offset %d", 
                  HarddiskIdx, ExtOffset);
                } 
              else 
                {
                  for (ExtPartitionIdx = 0; ExtPartitionIdx < 4; ExtPartitionIdx++) 
                    {
                      ep = &ExtendedPartitionTable[ExtPartitionIdx];
                      if (PartitionIsSupported(ep)) 
                        {
                          DPRINT("Harddisk%d: Type:%02x Offset:%d Size:%d\n", 
                                 HarddiskIdx, 
                                 ep->PartitionType, 
                                 ep->StartingBlock + ExtOffset, 
                                 ep->SectorCount);

                            //  Create device for logical partition
                          strcpy(LogicalDeviceName, IDE_NT_ROOTDIR_NAME);
                          strcat(LogicalDeviceName, IDE_NT_DEVICE_NAME);
                          LogicalDeviceName[strlen(LogicalDeviceName) + 1] = '\0';
                          LogicalDeviceName[strlen(LogicalDeviceName)] = '0' + HarddiskIdx;
                          strcat(LogicalDeviceName, IDE_NT_PARTITION_NAME);
                          LogicalDeviceName[strlen(LogicalDeviceName) + 1] = '\0';
                          LogicalDeviceName[strlen(LogicalDeviceName)] = '0' + PartitionNum++;
                          strcpy(Win32AliasName, "\\??\\ :");
                          Win32AliasName[4] = 'C' + TotalPartitions;
                          TotalPartitions++;
                          RC = IDECreateDevice(DriverObject, 
                                               LogicalDeviceName, 
                                               &LogicalDeviceObject, 
                                               ControllerObject, 
                                               DriveIdx,
                                               Win32AliasName,
                                               &DrvParms,
                                               ep->StartingBlock + ExtOffset, 
                                               ep->SectorCount);
                          if (!NT_SUCCESS(RC)) 
                            {
                              DPRINT("IDECreateDevice call failed\n",0);
                              break;
                            }
                        }
                    }
                }
            }
        }
    }

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
  DPRINT("TM:Cyls:%d  Heads:%d  Sectors/Trk:%d Capacity:%d\n",
         DrvParms->TMCylinders, 
         DrvParms->TMHeads, 
         DrvParms->TMSectorsPerTrk,
         DrvParms->TMCapacity);

  return TRUE;
}

//    IDEGetPartitionTable
//
//  DESCRIPTION:
//    Gets the partition table from the device at the given offset if one exists
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN   int        CommandPort     Address of command port for controller
//    IN   int        DriveNum        index of drive (0,1)
//    IN   int        Offset          Block offset (LBA addressing)
//    OUT  PARTITION *PartitionTable  address to write partition table
//
//  RETURNS:
//    TRUE  partition table was retrieved successfully
//

BOOLEAN  
IDEGetPartitionTable(IN int CommandPort, 
                     IN int DriveNum, 
                     IN int Offset, 
                     IN PIDE_DRIVE_IDENTIFY DrvParms,
                     OUT PARTITION *PartitionTable) 
{
  BYTE  SectorBuf[512], SectorNum, DrvHead, CylinderLow, CylinderHigh;
  int   RC, SaveOffset;

#ifndef NDEBUG
  int   i;
#endif

    //  Get sector at offset
  SaveOffset = Offset;
  if (DrvParms->Capabilities & IDE_DRID_LBA_SUPPORTED) 
    {
      SectorNum = Offset & 0xff;
      CylinderLow = (Offset >> 8) & 0xff;
      CylinderHigh = (Offset >> 16) & 0xff;
      DrvHead = ((Offset >> 24) & 0x0f) | 
          (DriveNum ? IDE_DH_DRV1 : 0) |
          IDE_DH_LBA;
    } 
  else 
    {
      SectorNum = (Offset % DrvParms->SectorsPerTrack) + 1;
      Offset /= DrvParms->SectorsPerTrack;
      DrvHead = (Offset % DrvParms->LogicalHeads) | 
          (DriveNum ? IDE_DH_DRV1 : 0);
      Offset /= DrvParms->LogicalHeads;
      CylinderLow = Offset & 0xff;
      CylinderHigh = Offset >> 8;
    }
  Offset = SaveOffset;

  RC = IDEPolledRead(CommandPort, 
                     0, 
                     1, 
                     SectorNum,
                     CylinderLow,
                     CylinderHigh,
                     DrvHead, 
                     IDE_CMD_READ_RETRY,
                     SectorBuf);
  if (RC != 0) 
    {
      DPRINT("read failed: port %04x drive %d sector %d rc %d\n", 
             CommandPort, 
             DriveNum, 
             Offset, 
             RC);
      return  FALSE;
    } 
  else if (*((WORD *)(SectorBuf + PART_MAGIC_OFFSET)) != PARTITION_MAGIC) 
    {
      DPRINT("Bad partition magic: port %04x drive %d offset %d magic %d\n", 
             CommandPort, 
             DriveNum, 
             Offset, 
             *((short *)(SectorBuf + PART_MAGIC_OFFSET)));
      return  FALSE;
    }
  RtlCopyMemory(PartitionTable, SectorBuf + PARTITION_OFFSET, sizeof(PARTITION) * 4);

#ifndef NDEBUG
  DPRINT("Partition Table for device %d at offset %d on port %x\n", 
         DriveNum, 
         Offset,
         CommandPort);
  for (i = 0; i < PARTITION_TBL_SIZE; i++)
    {
      DPRINT("  %d: flags:%2x type:%x start:%d:%d:%d end:%d:%d:%d stblk:%d count:%d\n", 
             i, 
             PartitionTable[i].BootFlags,
             PartitionTable[i].PartitionType,
             PartitionTable[i].StartingHead,
             PartitionTable[i].StartingSector,
             PartitionTable[i].StartingCylinder,
             PartitionTable[i].EndingHead,
             PartitionTable[i].EndingSector,
             PartitionTable[i].EndingCylinder,
             PartitionTable[i].StartingBlock,
             PartitionTable[i].SectorCount);
    }
#endif

  return  TRUE;
}

//    IDECreateDevice
//
//  DESCRIPTION:
//    Creates a device by calling IoCreateDevice and a sylbolic link for Win32
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN   PDRIVER_OBJECT      DriverObject      The system supplied driver object
//    IN   PCHAR               DeviceName        The name of the device
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
IDECreateDevice(IN PDRIVER_OBJECT DriverObject, 
                IN PCHAR DeviceName, 
                OUT PDEVICE_OBJECT *DeviceObject, 
                IN PCONTROLLER_OBJECT ControllerObject, 
                IN int UnitNumber,
                IN char *Win32Alias,
                IN PIDE_DRIVE_IDENTIFY DrvParms,
                IN DWORD Offset, 
                IN DWORD Size) 
{
  WCHAR                  UnicodeBuffer[IDE_MAX_NAME_LENGTH];
  NTSTATUS               RC;
  ANSI_STRING            AnsiName, AnsiSymLink;
  UNICODE_STRING         UnicodeName, SymLink;
  PIDE_DEVICE_EXTENSION  DeviceExtension;

    // Create a unicode device name
  RtlInitAnsiString(&AnsiName, DeviceName);
  UnicodeName.MaximumLength = IDE_MAX_NAME_LENGTH * sizeof(WCHAR);
  UnicodeName.Buffer = UnicodeBuffer;
  RtlAnsiStringToUnicodeString(&UnicodeName, &AnsiName, FALSE);

    // Create the device
  RC = IoCreateDevice(DriverObject, sizeof(IDE_DEVICE_EXTENSION), 
      &UnicodeName, FILE_DEVICE_DISK, 0, TRUE, DeviceObject);
  if (!NT_SUCCESS(RC)) 
    {
      DPRINT("IoCreateDevice call failed\n",0);
      return RC;
    }

    //  Set the buffering strategy here...
  (*DeviceObject)->Flags |= DO_DIRECT_IO;
  (*DeviceObject)->AlignmentRequirement = FILE_WORD_ALIGNMENT;

    //  Fill out Device extension data
  DeviceExtension = (PIDE_DEVICE_EXTENSION) (*DeviceObject)->DeviceExtension;
  DeviceExtension->DeviceObject = (*DeviceObject);
  DeviceExtension->ControllerObject = ControllerObject;
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
  DeviceExtension->Offset = Offset;
  DeviceExtension->Size = Size;
  DPRINT("%s: offset %d size %d \n", 
         DeviceName, 
         DeviceExtension->Offset,
         DeviceExtension->Size);

    // FIXME: Create Win32 symbolic link (destroy device if it fails)

  if (Win32Alias != NULL)
    {
      DPRINT("Creating SymLink %s --> %s\n", DeviceName, Win32Alias);
      RtlInitAnsiString(&AnsiSymLink, Win32Alias);
      RtlAnsiStringToUnicodeString(&SymLink, &AnsiSymLink, TRUE);
      IoCreateSymbolicLink(&SymLink, &UnicodeName);
      RtlFreeUnicodeString(&SymLink);
    }

    //  Initialize the DPC object here
  IoInitializeDpcRequest(*DeviceObject, IDEDpcForIsr);

  if (Win32Alias != NULL)
    {
      DbgPrint("%s is %s %dMB\n", DeviceName, Win32Alias, Size / 2048);
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
IDEDispatchOpenClose(IN PDEVICE_OBJECT pDO, 
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
IDEDispatchReadWrite(IN PDEVICE_OBJECT pDO, 
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
DPRINT("AdjOffset:%ld:%ld + ByteOffset:%ld:%ld = ",
       (unsigned long) AdjustedOffset.u.HighPart,
       (unsigned long) AdjustedOffset.u.LowPart,
       (unsigned long) IrpStack->Parameters.Read.ByteOffset.u.HighPart,
       (unsigned long) IrpStack->Parameters.Read.ByteOffset.u.LowPart);
  AdjustedOffset = RtlLargeIntegerAdd(AdjustedOffset, 
                                      IrpStack->Parameters.Read.ByteOffset);
DPRINT("AdjOffset:%ld:%ld\n",
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
    /* FIXME: this assumption will fail on drives bigger than 1TB */
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

static  NTSTATUS  
IDEDispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject, 
                         IN PIRP Irp) 
{
  NTSTATUS  RC;
  ULONG     ControlCode, InputLength, OutputLength;
  PIO_STACK_LOCATION     IrpStack;
  PIDE_DEVICE_EXTENSION  DeviceExtension;

  RC = STATUS_SUCCESS;
  IrpStack = IoGetCurrentIrpStackLocation(Irp);
  ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
  InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
  OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
  DeviceExtension = (PIDE_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

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
              // FIXME: should report for RawDevice even on partition
          Geometry->Cylinders.QuadPart = DeviceExtension->Size / 
              DeviceExtension->SectorsPerLogCyl;
          Geometry->TracksPerCylinder = DeviceExtension->SectorsPerLogTrk /
              DeviceExtension->SectorsPerLogCyl;
          Geometry->SectorsPerTrack = DeviceExtension->SectorsPerLogTrk;
          Geometry->BytesPerSector = DeviceExtension->BytesPerSector;

          Irp->IoStatus.Status = STATUS_SUCCESS;
          Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
        }
      break;

    case IOCTL_DISK_GET_PARTITION_INFO:
    case IOCTL_DISK_SET_PARTITION_INFO:
    case IOCTL_DISK_GET_DRIVE_LAYOUT:
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

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

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
IDEStartIo(IN PDEVICE_OBJECT DeviceObject, 
           IN PIRP Irp) 
{
  LARGE_INTEGER              SectorLI;
  PIO_STACK_LOCATION         IrpStack;
  PIDE_DEVICE_EXTENSION      DeviceExtension;
  KIRQL                      OldIrql;
  
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
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      IoStartNextPacket(DeviceObject, FALSE);
      break;
    }
}

//    IDEAllocateController

static IO_ALLOCATION_ACTION 
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

BOOLEAN 
IDEStartController(IN OUT PVOID Context) 
{
  BYTE SectorCnt, SectorNum, CylinderLow, CylinderHigh;
  BYTE DrvHead, Command;
  int Retries;
  ULONG StartingSector;
  PIDE_DEVICE_EXTENSION DeviceExtension;
  PIDE_CONTROLLER_EXTENSION ControllerExtension;
  PIRP Irp;

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
      IDE_CMD_READ_RETRY : IDE_CMD_WRITE_RETRY;
  if (DrvHead & IDE_DH_LBA) 
    {
      DPRINT("%s:DRV=%d:LBA=1:BLK=%08d:SC=%02x:CM=%02x\n",
             DeviceExtension->Operation == IRP_MJ_READ ? "READ" : "WRITE",
             DrvHead & IDE_DH_DRV1 ? 1 : 0, 
             ((DrvHead & 0x0f) << 24) +
             (CylinderHigh << 16) + (CylinderLow << 8) + SectorNum,
             SectorCnt, 
             Command);
    } 
  else 
    {
      DPRINT("%s:DRV=%d:LBA=0:CH=%02x:CL=%02x:HD=%01x:SN=%02x:SC=%02x:CM=%02x\n",
             DeviceExtension->Operation == IRP_MJ_READ ? "READ" : "WRITE",
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
      BYTE  Status = IDEReadStatus(ControllerExtension->CommandPortBase);
      if (!(Status & IDE_SR_BUSY)) 
        {
          break;
        }
      KeStallExecutionProcessor(10);
    }
  if (Retries >= IDE_MAX_BUSY_RETRIES)
    {
      if (++ControllerExtension->Retries > IDE_MAX_CMD_RETRIES)
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

  /*  Select the desired drive  */
  IDEWriteDriveHead(ControllerExtension->CommandPortBase, IDE_DH_FIXED | DrvHead);

  /*  wait for BUSY to clear and DRDY to assert */
  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES; Retries++)
    {
      BYTE  Status = IDEReadStatus(ControllerExtension->CommandPortBase);
      if (!(Status & IDE_SR_BUSY) && (Status & IDE_SR_DRDY)) 
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

static  BOOLEAN  
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
      DPRINT("IDE Error: OP:%02x STAT:%02x ERR:%02x CYLLO:%02x CYLHI:%02x SCNT:%02x SNUM:%02x\n", 
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
static  VOID  
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
static  VOID  
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
                DPRINT("Max retries has been reached, IRP finished with error\n");
                ControllerExtension->CurrentIrp->IoStatus.Status = STATUS_IO_TIMEOUT;
                ControllerExtension->CurrentIrp->IoStatus.Information = 0;
                IDEFinishOperation(ControllerExtension);
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
            DPRINT("Timeout waiting for drive reset, giving up on IRP\n");
            if (ControllerExtension->CurrentIrp != NULL)
              {
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


