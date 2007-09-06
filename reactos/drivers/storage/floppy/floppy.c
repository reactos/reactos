/*
 *  ReactOS Floppy Driver
 *  Copyright (C) 2004, Vizzini (vizzini@plasmic.com)
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
 *
 * PROJECT:         ReactOS Floppy Driver
 * FILE:            floppy.c
 * PURPOSE:         Main floppy driver routines
 * PROGRAMMER:      Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *                  15-Feb-2004 vizzini - Created
 * NOTES:
 *  - This driver is only designed to work with ISA-bus floppy controllers.  This
 *    won't work on PCI-based controllers or on anything else with level-sensitive
 *    interrupts without modification.  I don't think these controllers exist.
 *
 * ---- General to-do items ----
 * TODO: Figure out why CreateClose isn't called any more.  Seems to correspond
 *       with the driver not being unloadable.
 * TODO: Think about StopDpcQueued -- could be a race; too tired atm to tell
 * TODO: Clean up drive start/stop responsibilities (currently a mess...)
 *
 * ---- Support for proper media detection ----
 * TODO: Handle MFM flag
 * TODO: Un-hardcode the data rate from various places
 * TODO: Proper media detection (right now we're hardcoded to 1.44)
 * TODO: Media detection based on sector 1
 */

#define NDEBUG
#include <debug.h>
#include <ntddk.h>

#include "floppy.h"
#include "hardware.h"
#include "csqrtns.h"
#include "ioctl.h"
#include "readwrite.h"

/*
 * Global controller info structures.  Each controller gets one.  Since the system
 * will probably have only one, with four being a very unlikely maximum, a static
 * global array is easiest to deal with.
 */
static CONTROLLER_INFO gControllerInfo[MAX_CONTROLLERS];
static ULONG gNumberOfControllers = 0;

/* Queue thread management */
static KEVENT QueueThreadTerminate;
static PVOID QueueThreadObject;


static VOID NTAPI MotorStopDpcFunc(PKDPC UnusedDpc,
			    PVOID DeferredContext,
			    PVOID SystemArgument1,
			    PVOID SystemArgument2)
/*
 * FUNCTION: Stop the floppy motor
 * ARGUMENTS:
 *     UnusedDpc: DPC object that's going off
 *     DeferredContext: called with DRIVE_INFO for drive to turn off
 *     SystemArgument1: unused
 *     SystemArgument2: unused
 * NOTES:
 *     - Must set an event to let other threads know we're done turning off the motor
 *     - Called back at DISPATCH_LEVEL
 */
{
  PCONTROLLER_INFO ControllerInfo = (PCONTROLLER_INFO)DeferredContext;

  UNREFERENCED_PARAMETER(SystemArgument1);
  UNREFERENCED_PARAMETER(SystemArgument2);
  UNREFERENCED_PARAMETER(UnusedDpc);

  ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
  ASSERT(ControllerInfo);

  DPRINT("floppy: MotorStopDpcFunc called\n");

  HwTurnOffMotor(ControllerInfo);
  ControllerInfo->StopDpcQueued = FALSE;
  KeSetEvent(&ControllerInfo->MotorStoppedEvent, EVENT_INCREMENT, FALSE);
}


VOID NTAPI StartMotor(PDRIVE_INFO DriveInfo)
/*
 * FUNCTION: Start the motor, taking into account proper handling of the timer race
 * ARGUMENTS:
 *     DriveInfo: drive to start
 * NOTES:
 *     - Never call HwTurnOnMotor() directly
 *     - This protocol manages a race between the cancel timer and the requesting thread.
 *       You wouldn't want to turn on the motor and then cancel the timer, because the
 *       cancel dpc might fire in the meantime, and that'd un-do what you just did.  If you
 *       cancel the timer first, but KeCancelTimer returns false, the dpc is already running,
 *       so you have to wait until the dpc is completly done running, or else you'll race
 *       with the turner-offer
 *     - PAGED_CODE because we wait
 */
{
  PAGED_CODE();
  ASSERT(DriveInfo);

  DPRINT("floppy: StartMotor called\n");

  if(DriveInfo->ControllerInfo->StopDpcQueued && !KeCancelTimer(&DriveInfo->ControllerInfo->MotorTimer))
    {
      /* Motor turner-offer is already running; wait for it to finish */
      DPRINT("floppy: StartMotor: motor turner-offer is already running; waiting for it\n");
      KeWaitForSingleObject(&DriveInfo->ControllerInfo->MotorStoppedEvent, Executive, KernelMode, FALSE, NULL);
      DPRINT("floppy: StartMotor: wait satisfied\n");
    }

  DriveInfo->ControllerInfo->StopDpcQueued = FALSE;

  if(HwTurnOnMotor(DriveInfo) != STATUS_SUCCESS)
  {
    DPRINT("floppy: StartMotor(): warning: HwTurnOnMotor failed\n");
  }
}


VOID NTAPI StopMotor(PCONTROLLER_INFO ControllerInfo)
/*
 * FUNCTION: Stop all motors on the controller
 * ARGUMENTS:
 *     DriveInfo: Drive to stop
 * NOTES:
 *     - Never call HwTurnOffMotor() directly
 *     - This manages the timer cancelation race (see StartMotor for details).
 *       All we have to do is set up a timer.
 */
{
  LARGE_INTEGER StopTime;

  ASSERT(ControllerInfo);

  DPRINT("floppy: StopMotor called\n");

  /* one relative second, in 100-ns units */
  StopTime.QuadPart = 10000000;
  StopTime.QuadPart *= -1;

  KeClearEvent(&ControllerInfo->MotorStoppedEvent);
  KeSetTimer(&ControllerInfo->MotorTimer, StopTime, &ControllerInfo->MotorStopDpc);
  ControllerInfo->StopDpcQueued = TRUE;
}


VOID NTAPI WaitForControllerInterrupt(PCONTROLLER_INFO ControllerInfo)
/*
 * FUNCTION: Wait for the controller to interrupt, and then clear the event
 * ARGUMENTS:
 *     ControllerInfo: Controller to wait for
 * NOTES:
 *     - There is a small chance that an unexpected or spurious interrupt could
 *       be lost with this clear/wait/clear scheme used in this driver.  This is
 *       deemed to be an acceptable risk due to the unlikeliness of the scenario,
 *       and the fact that it'll probably work fine next time.
 *     - PAGED_CODE because it waits
 */
{
  PAGED_CODE();
  ASSERT(ControllerInfo);

  KeWaitForSingleObject(&ControllerInfo->SynchEvent, Executive, KernelMode, FALSE, NULL);
  KeClearEvent(&ControllerInfo->SynchEvent);
}


static NTSTATUS NTAPI CreateClose(PDEVICE_OBJECT DeviceObject,
                                  PIRP Irp)
/*
 * FUNCTION: Dispatch function called for Create and Close IRPs
 * ARGUMENTS:
 *     DeviceObject: DeviceObject that is the target of the IRP
 *     Irp: IRP to process
 * RETURNS:
 *     STATUS_SUCCESS in all cases
 * NOTES:
 *     - The Microsoft sample drivers tend to return FILE_OPENED in Information, so I do too.
 *     - No reason to fail the device open
 *     - No state to track, so this routine is easy
 *     - Can be called <= DISPATCH_LEVEL
 *
 * TODO: Figure out why this isn't getting called
 */
{
  UNREFERENCED_PARAMETER(DeviceObject);

  DPRINT("floppy: CreateClose called\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = FILE_OPENED;

  IoCompleteRequest(Irp, IO_DISK_INCREMENT);

  return STATUS_SUCCESS;
}


static NTSTATUS NTAPI Recalibrate(PDRIVE_INFO DriveInfo)
/*
 * FUNCTION: Start the recalibration process
 * ARGUMENTS:
 *     DriveInfo: Pointer to the driveinfo struct associated with the targeted drive
 * RETURNS:
 *     STATUS_SUCCESS on successful starting of the process
 *     STATUS_IO_DEVICE_ERROR if it fails
 * NOTES:
 *     - Sometimes you have to do two recalibrations, particularly if the disk has <80 tracks.
 *     - PAGED_CODE because we wait
 */
{
  NTSTATUS Status;
  ULONG i;

  PAGED_CODE();
  ASSERT(DriveInfo);

  /* first turn on the motor */
  /* Must stop after every start, prior to return */
  StartMotor(DriveInfo);

  /* set the data rate */
  DPRINT("floppy: FIXME: UN-HARDCODE DATA RATE\n");
  if(HwSetDataRate(DriveInfo->ControllerInfo, 0) != STATUS_SUCCESS)
    {
      DPRINT("floppy: Recalibrate: HwSetDataRate failed\n");
      StopMotor(DriveInfo->ControllerInfo);
      return STATUS_IO_DEVICE_ERROR;
    }

  /* clear the event just in case the last call forgot */
  KeClearEvent(&DriveInfo->ControllerInfo->SynchEvent);

  /* sometimes you have to do this twice; we'll just do it twice all the time since
   * we don't know if the people calling this Recalibrate routine expect a disk to
   * even be in the drive, and if so, if that disk is formatted.
   */
  for(i = 0; i < 2; i++)
    {
      /* Send the command */
      Status = HwRecalibrate(DriveInfo);
      if(Status != STATUS_SUCCESS)
	{
	  DPRINT("floppy: Recalibrate: HwRecalibrate returned error\n");
          continue;
	}

      WaitForControllerInterrupt(DriveInfo->ControllerInfo);

      /* Get the results */
      Status = HwRecalibrateResult(DriveInfo->ControllerInfo);
      if(Status != STATUS_SUCCESS)
	{
	  DPRINT("floppy: Recalibrate: HwRecalibrateResult returned error\n");
          break;
        }
    }

  KeClearEvent(&DriveInfo->ControllerInfo->SynchEvent);

  /* Must stop after every start, prior to return */
  StopMotor(DriveInfo->ControllerInfo);

  return Status;
}


NTSTATUS NTAPI ResetChangeFlag(PDRIVE_INFO DriveInfo)
/*
 * FUNCTION: Reset the drive's change flag (as reflected in the DIR)
 * ARGUMENTS:
 *     DriveInfo: the drive to reset
 * RETURNS:
 *     STATUS_SUCCESS if the changeline is cleared
 *     STATUS_NO_MEDIA_IN_DEVICE if the changeline cannot be cleared
 *     STATUS_IO_DEVICE_ERROR if the controller cannot be communicated with
 * NOTES:
 *     - Change reset procedure: recalibrate, seek 1, seek 0
 *     - If the line is still set after that, there's clearly no disk in the
 *       drive, so we return STATUS_NO_MEDIA_IN_DEVICE
 *     - PAGED_CODE because we wait
 */
{
  BOOLEAN DiskChanged;

  PAGED_CODE();
  ASSERT(DriveInfo);

  DPRINT("floppy: ResetChangeFlag called\n");

  /* Try to recalibrate.  We don't care if it works. */
  Recalibrate(DriveInfo);

  /* clear spurious interrupts in prep for seeks */
  KeClearEvent(&DriveInfo->ControllerInfo->SynchEvent);

  /* must re-start the drive because Recalibrate() stops it */
  StartMotor(DriveInfo);

  /* Seek to 1 */
  if(HwSeek(DriveInfo, 1) != STATUS_SUCCESS)
    {
      DPRINT("floppy: ResetChangeFlag(): HwSeek failed; returning STATUS_IO_DEVICE_ERROR\n");
      StopMotor(DriveInfo->ControllerInfo);
      return STATUS_IO_DEVICE_ERROR;
    }

  WaitForControllerInterrupt(DriveInfo->ControllerInfo);

  if(HwSenseInterruptStatus(DriveInfo->ControllerInfo) != STATUS_SUCCESS)
    {
      DPRINT("floppy: ResetChangeFlag(): HwSenseInterruptStatus failed; bailing out\n");
      StopMotor(DriveInfo->ControllerInfo);
      return STATUS_IO_DEVICE_ERROR;
    }

  /* Seek back to 0 */
  if(HwSeek(DriveInfo, 0) != STATUS_SUCCESS)
    {
      DPRINT("floppy: ResetChangeFlag(): HwSeek failed; returning STATUS_IO_DEVICE_ERROR\n");
      StopMotor(DriveInfo->ControllerInfo);
      return STATUS_IO_DEVICE_ERROR;
    }

  WaitForControllerInterrupt(DriveInfo->ControllerInfo);

  if(HwSenseInterruptStatus(DriveInfo->ControllerInfo) != STATUS_SUCCESS)
    {
      DPRINT("floppy: ResetChangeFlag(): HwSenseInterruptStatus #2 failed; bailing\n");
      StopMotor(DriveInfo->ControllerInfo);
      return STATUS_IO_DEVICE_ERROR;
    }

  /* Check the change bit */
  if(HwDiskChanged(DriveInfo, &DiskChanged) != STATUS_SUCCESS)
    {
      DPRINT("floppy: ResetChangeFlag(): HwDiskChagned failed; returning STATUS_IO_DEVICE_ERROR\n");
      StopMotor(DriveInfo->ControllerInfo);
      return STATUS_IO_DEVICE_ERROR;
    }

  StopMotor(DriveInfo->ControllerInfo);

  /* if the change flag is still set, there's probably no media in the drive. */
  if(DiskChanged)
    return STATUS_NO_MEDIA_IN_DEVICE;

  /* else we're done! */
  return STATUS_SUCCESS;
}


static VOID NTAPI Unload(PDRIVER_OBJECT DriverObject)
/*
 * FUNCTION: Unload the driver from memory
 * ARGUMENTS:
 *     DriverObject - The driver that is being unloaded
 */
{
  ULONG i,j;

  PAGED_CODE();
  UNREFERENCED_PARAMETER(DriverObject);

  DPRINT("floppy: unloading\n");

  KeSetEvent(&QueueThreadTerminate, 0, FALSE);
  KeWaitForSingleObject(QueueThreadObject, Executive, KernelMode, FALSE, 0);
  ObDereferenceObject(QueueThreadObject);

  for(i = 0; i < gNumberOfControllers; i++)
    {
      if(!gControllerInfo[i].Initialized)
	continue;

      for(j = 0; j < gControllerInfo[i].NumberOfDrives; j++)
	{
	  if(!gControllerInfo[i].DriveInfo[j].Initialized)
	    continue;

          if(gControllerInfo[i].DriveInfo[j].DeviceObject)
            {
	      UNICODE_STRING Link;

	      RtlInitUnicodeString(&Link, gControllerInfo[i].DriveInfo[j].SymLinkBuffer);
	      IoDeleteSymbolicLink(&Link);

	      RtlInitUnicodeString(&Link, gControllerInfo[i].DriveInfo[j].ArcPathBuffer);
	      IoDeassignArcName(&Link);

              IoDeleteDevice(gControllerInfo[i].DriveInfo[j].DeviceObject);
            }
	}

      IoDisconnectInterrupt(gControllerInfo[i].InterruptObject);

      /* Power down the controller */
      if(HwPowerOff(&gControllerInfo[i]) != STATUS_SUCCESS)
      {
	DPRINT("floppy: unload: warning: HwPowerOff failed\n");
      }
    }
}


static NTSTATUS NTAPI ConfigCallback(PVOID Context,
                                     PUNICODE_STRING PathName,
                                     INTERFACE_TYPE BusType,
                                     ULONG BusNumber,
                                     PKEY_VALUE_FULL_INFORMATION *BusInformation,
                                     CONFIGURATION_TYPE ControllerType,
                                     ULONG ControllerNumber,
                                     PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
                                     CONFIGURATION_TYPE PeripheralType,
                                     ULONG PeripheralNumber,
                                     PKEY_VALUE_FULL_INFORMATION *PeripheralInformation)
/*
 * FUNCTION: Callback to IoQueryDeviceDescription, which tells us about our controllers
 * ARGUMENTS:
 *     Context: Unused
 *     PathName: Unused
 *     BusType: Type of the bus that our controller is on
 *     BusNumber: Number of the bus that our controller is on
 *     BusInformation: Unused
 *     ControllerType: Unused
 *     ControllerNumber: Number of the controller that we're adding
 *     ControllerInformation: Full configuration information for our controller
 *     PeripheralType: Unused
 *     PeripheralNumber: Unused
 *     PeripheralInformation: Full configuration information for each drive on our controller
 * RETURNS:
 *     STATUS_SUCCESS in all cases
 * NOTES:
 *     - The only documentation I've found about the contents of these structures is
 *       from the various Microsoft floppy samples and from the DDK headers.  They're
 *       very vague, though, so I'm only mostly sure that this stuff is correct, as
 *       the MS samples do things completely differently than I have done them.  Seems
 *       to work in my VMWare, though.
 *     - Basically, the function gets all of the information (port, dma, irq) about the
 *       controller, and then loops through all of the drives presented in PeripheralInformation.
 *     - Each controller has a CONTROLLER_INFO created for it, and each drive has a DRIVE_INFO.
 *     - Device objects are created for each drive (not controller), as that's the targeted
 *       device in the eyes of the rest of the OS.  Each DRIVE_INFO points to a single CONTROLLER_INFO.
 *     - We only support up to four controllers in the whole system, each of which supports up to four
 *       drives.
 */
{
  PKEY_VALUE_FULL_INFORMATION ControllerFullDescriptor = ControllerInformation[IoQueryDeviceConfigurationData];
  PCM_FULL_RESOURCE_DESCRIPTOR ControllerResourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)((PCHAR)ControllerFullDescriptor +
                                                               ControllerFullDescriptor->DataOffset);

  PKEY_VALUE_FULL_INFORMATION PeripheralFullDescriptor = PeripheralInformation[IoQueryDeviceConfigurationData];
  PCM_FULL_RESOURCE_DESCRIPTOR PeripheralResourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)((PCHAR)PeripheralFullDescriptor +
                                                               PeripheralFullDescriptor->DataOffset);

  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  PCM_FLOPPY_DEVICE_DATA FloppyDeviceData;
  UCHAR i;

  PAGED_CODE();
  UNREFERENCED_PARAMETER(PeripheralType);
  UNREFERENCED_PARAMETER(PeripheralNumber);
  UNREFERENCED_PARAMETER(BusInformation);
  UNREFERENCED_PARAMETER(Context);
  UNREFERENCED_PARAMETER(ControllerType);
  UNREFERENCED_PARAMETER(PathName);


  DPRINT("floppy: ConfigCallback called with ControllerNumber %d\n", ControllerNumber);

  gControllerInfo[gNumberOfControllers].ControllerNumber = ControllerNumber;
  gControllerInfo[gNumberOfControllers].InterfaceType = BusType;
  gControllerInfo[gNumberOfControllers].BusNumber = BusNumber;

  /* Get controller interrupt level/vector, dma channel, and port base */
  for(i = 0; i < ControllerResourceDescriptor->PartialResourceList.Count; i++)
    {
      KeInitializeEvent(&gControllerInfo[gNumberOfControllers].SynchEvent, NotificationEvent, FALSE);

      PartialDescriptor = &ControllerResourceDescriptor->PartialResourceList.PartialDescriptors[i];

      if(PartialDescriptor->Type == CmResourceTypeInterrupt)
        {
          gControllerInfo[gNumberOfControllers].Level = PartialDescriptor->u.Interrupt.Level;
          gControllerInfo[gNumberOfControllers].Vector = PartialDescriptor->u.Interrupt.Vector;

          if(PartialDescriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
            gControllerInfo[gNumberOfControllers].InterruptMode = Latched;
          else
            gControllerInfo[gNumberOfControllers].InterruptMode = LevelSensitive;
        }

      else if(PartialDescriptor->Type == CmResourceTypePort)
        {
          PHYSICAL_ADDRESS TranslatedAddress;
          ULONG AddressSpace = 0x1; /* I/O Port Range */

          if(!HalTranslateBusAddress(BusType, BusNumber, PartialDescriptor->u.Port.Start, &AddressSpace, &TranslatedAddress))
	    {
	      DPRINT("floppy: HalTranslateBusAddress failed; returning\n");
	      return STATUS_IO_DEVICE_ERROR;
	    }

          if(AddressSpace == 0)
              gControllerInfo[gNumberOfControllers].BaseAddress = MmMapIoSpace(TranslatedAddress, FDC_PORT_BYTES, MmNonCached);
          else
              gControllerInfo[gNumberOfControllers].BaseAddress = (PUCHAR)TranslatedAddress.u.LowPart;
        }

      else if(PartialDescriptor->Type == CmResourceTypeDma)
        gControllerInfo[gNumberOfControllers].Dma = PartialDescriptor->u.Dma.Channel;
    }

  /* Start with 0 drives, then go looking */
  gControllerInfo[gNumberOfControllers].NumberOfDrives = 0;

  /* learn about drives attached to controller */
  for(i = 0; i < PeripheralResourceDescriptor->PartialResourceList.Count; i++)
    {
      PDRIVE_INFO DriveInfo = &gControllerInfo[gNumberOfControllers].DriveInfo[i];

      PartialDescriptor = &PeripheralResourceDescriptor->PartialResourceList.PartialDescriptors[i];

      if(PartialDescriptor->Type != CmResourceTypeDeviceSpecific)
        continue;

      FloppyDeviceData = (PCM_FLOPPY_DEVICE_DATA)(PartialDescriptor + 1);

      DriveInfo->ControllerInfo = &gControllerInfo[gNumberOfControllers];
      DriveInfo->UnitNumber = i;

      DriveInfo->FloppyDeviceData.MaxDensity = FloppyDeviceData->MaxDensity;
      DriveInfo->FloppyDeviceData.MountDensity = FloppyDeviceData->MountDensity;
      DriveInfo->FloppyDeviceData.StepRateHeadUnloadTime = FloppyDeviceData->StepRateHeadUnloadTime;
      DriveInfo->FloppyDeviceData.HeadLoadTime = FloppyDeviceData->HeadLoadTime;
      DriveInfo->FloppyDeviceData.MotorOffTime = FloppyDeviceData->MotorOffTime;
      DriveInfo->FloppyDeviceData.SectorLengthCode = FloppyDeviceData->SectorLengthCode;
      DriveInfo->FloppyDeviceData.SectorPerTrack = FloppyDeviceData->SectorPerTrack;
      DriveInfo->FloppyDeviceData.ReadWriteGapLength = FloppyDeviceData->ReadWriteGapLength;
      DriveInfo->FloppyDeviceData.FormatGapLength = FloppyDeviceData->FormatGapLength;
      DriveInfo->FloppyDeviceData.FormatFillCharacter = FloppyDeviceData->FormatFillCharacter;
      DriveInfo->FloppyDeviceData.HeadSettleTime = FloppyDeviceData->HeadSettleTime;
      DriveInfo->FloppyDeviceData.MotorSettleTime = FloppyDeviceData->MotorSettleTime;
      DriveInfo->FloppyDeviceData.MaximumTrackValue = FloppyDeviceData->MaximumTrackValue;
      DriveInfo->FloppyDeviceData.DataTransferLength = FloppyDeviceData->DataTransferLength;

      /* Once it's all set up, acknowledge its existance in the controller info object */
      gControllerInfo[gNumberOfControllers].NumberOfDrives++;
    }

  gControllerInfo[gNumberOfControllers].Populated = TRUE;
  gNumberOfControllers++;

  return STATUS_SUCCESS;
}


static BOOLEAN NTAPI Isr(PKINTERRUPT Interrupt,
                         PVOID ServiceContext)
/*
 * FUNCTION: Interrupt service routine for the controllers
 * ARGUMENTS:
 *     Interrupt: Interrupt object representing the interrupt that occured
 *     ServiceContext: Pointer to the ControllerInfo object that caused the interrupt
 * RETURNS:
 *     TRUE in all cases (see notes)
 * NOTES:
 *     - We should always be the target of the interrupt, being an edge-triggered ISA interrupt, but
 *       this won't be the case with a level-sensitive system like PCI
 *     - Note that it probably doesn't matter if the interrupt isn't dismissed, as it's edge-triggered.
 *       It probably won't keep re-interrupting.
 *     - There are two different ways to dismiss a floppy interrupt.  If the command has a result phase
 *       (see intel datasheet), you dismiss the interrupt by reading the first data byte.  If it does
 *       not, you dismiss the interrupt by doing a Sense Interrupt command.  Again, because it's edge-
 *       triggered, this is safe to not do here, as we can just wait for the DPC.
 *     - Either way, we don't want to do this here.  The controller shouldn't interrupt again, so we'll
 *       schedule a DPC to take care of it.
 *     - This driver really cannot shrare interrupts, as I don't know how to conclusively say
 *       whether it was our controller that interrupted or not.  I just have to assume that any time
 *       my ISR gets called, it was my board that called it.  Dumb design, yes, but it goes back to
 *       the semantics of ISA buses.  That, and I don't know much about ISA drivers. :-)
 *       UPDATE: The high bit of Status Register A seems to work on non-AT controllers.
 *     - Called at DIRQL
 */
{
  PCONTROLLER_INFO ControllerInfo = (PCONTROLLER_INFO)ServiceContext;

  UNREFERENCED_PARAMETER(Interrupt);

  ASSERT(ControllerInfo);

  DPRINT("floppy: ISR called\n");

  /*
   * Due to the stupidity of the drive/controller relationship on the floppy drive, only one device object
   * can have an active interrupt pending.  Due to the nature of these IRPs, though, there will only ever
   * be one thread expecting an interrupt at a time, and furthermore, Interrupts (outside of spurious ones)
   * won't ever happen unless a thread is expecting them.  Therefore, all we have to do is signal an event
   * and we're done.  Queue a DPC and leave.
   */
  KeInsertQueueDpc(&ControllerInfo->Dpc, NULL, NULL);

  return TRUE;
}


VOID NTAPI DpcForIsr(PKDPC UnusedDpc,
                     PVOID Context,
                     PVOID SystemArgument1,
                     PVOID SystemArgument2)
/*
 * FUNCTION: This DPC gets queued by every ISR.  Does the real per-interrupt work.
 * ARGUMENTS:
 *     UnusedDpc: Pointer to the DPC object that represents our function
 *     DeviceObject: Device that this DPC is running for
 *     Irp: Unused
 *     Context: Pointer to our ControllerInfo struct
 * NOTES:
 *     - This function just kicks off whatever the SynchEvent is and returns.  We depend on
 *       the thing that caused the drive to interrupt to handle the work of clearing the interrupt.
 *       This enables us to get back to PASSIVE_LEVEL and not hog system time on a really stupid,
 *       slow, screwed-up piece of hardare.
 *     - If nothing is waiting for us to set the event, the interrupt is effectively lost and will
 *       never be dismissed.  I wonder if this will become a problem.
 *     - Called at DISPATCH_LEVEL
 */
{
  PCONTROLLER_INFO ControllerInfo = (PCONTROLLER_INFO)Context;

  UNREFERENCED_PARAMETER(UnusedDpc);
  UNREFERENCED_PARAMETER(SystemArgument1);
  UNREFERENCED_PARAMETER(SystemArgument2);

  ASSERT(ControllerInfo);

  DPRINT("floppy: DpcForIsr called\n");

  KeSetEvent(&ControllerInfo->SynchEvent, EVENT_INCREMENT, FALSE);
}


static NTSTATUS NTAPI InitController(PCONTROLLER_INFO ControllerInfo)
/*
 * FUNCTION:  Initialize a newly-found controller
 * ARGUMENTS:
 *     ControllerInfo: pointer to the controller to be initialized
 * RETURNS:
 *     STATUS_SUCCESS if the controller is successfully initialized
 *     STATUS_IO_DEVICE_ERROR otherwise
 */
{
  int i;
  UCHAR HeadLoadTime;
  UCHAR HeadUnloadTime;
  UCHAR StepRateTime;

  PAGED_CODE();
  ASSERT(ControllerInfo);

  DPRINT("floppy: InitController called with Controller 0x%p\n", ControllerInfo);

  KeClearEvent(&ControllerInfo->SynchEvent);

  DPRINT("floppy: InitController: resetting the controller\n");

  /* Reset the controller */
  if(HwReset(ControllerInfo) != STATUS_SUCCESS)
    {
      DPRINT("floppy: InitController: unable to reset controller\n");
      return STATUS_IO_DEVICE_ERROR;
    }

/* Check if floppy drive exists */
  if(HwSenseInterruptStatus(ControllerInfo) != STATUS_SUCCESS)
	{
	  DPRINT("floppy: Floppy drive not detected! Returning STATUS_IO_DEVICE_ERROR\n");
	  return STATUS_IO_DEVICE_ERROR;
	}

DPRINT("floppy: InitController: resetting the controller after floppy detection\n");

  /* Reset the controller */
  if(HwReset(ControllerInfo) != STATUS_SUCCESS)
    {
      DPRINT("floppy: InitController: unable to reset controller\n");
      return STATUS_IO_DEVICE_ERROR;
    }

  DPRINT("floppy: InitController: setting data rate\n");

  /* Set data rate */
  if(HwSetDataRate(ControllerInfo, DRSR_DSEL_500KBPS) != STATUS_SUCCESS)
    {
      DPRINT("floppy: InitController: unable to set data rate\n");
      return STATUS_IO_DEVICE_ERROR;
    }

  DPRINT("floppy: InitController: waiting for initial interrupt\n");

  /* Wait for an interrupt */
  WaitForControllerInterrupt(ControllerInfo);

  /* Reset means you have to clear each of the four interrupts (one per drive) */
  for(i = 0; i < MAX_DRIVES_PER_CONTROLLER; i++)
    {
      DPRINT("floppy: InitController: Sensing interrupt %d\n", i);

      if(HwSenseInterruptStatus(ControllerInfo) != STATUS_SUCCESS)
	{
	  DPRINT("floppy: InitController: Unable to clear interrupt 0x%x\n", i);
	  return STATUS_IO_DEVICE_ERROR;
	}
    }

  DPRINT("floppy: InitController: done sensing interrupts\n");

  /* Next, see if we have the right version to do implied seek */
  if(HwGetVersion(ControllerInfo) == VERSION_ENHANCED)
    {
      /* If so, set that up -- all defaults below except first TRUE for EIS */
      if(HwConfigure(ControllerInfo, TRUE, TRUE, FALSE, 0, 0) != STATUS_SUCCESS)
	{
	  DPRINT("floppy: InitController: unable to set up implied seek\n");
          ControllerInfo->ImpliedSeeks = FALSE;
	}
      else
	{
	  DPRINT("floppy: InitController: implied seeks set!\n");
          ControllerInfo->ImpliedSeeks = TRUE;
	}

      /*
       * FIXME: Figure out the answer to the below
       *
       * I must admit that I'm really confused about the Model 30 issue.  At least one
       * important bit (the disk change bit in the DIR) is flipped if this is a Model 30
       * controller.  However, at least one other floppy driver believes that there are only
       * two computers that are guaranteed to have a Model 30 controller:
       *  - IBM Thinkpad 750
       *  - IBM PS2e
       *
       * ...and another driver only lists a config option for "thinkpad", that flips
       * the change line.  A third driver doesn't mention the Model 30 issue at all.
       *
       * What I can't tell is whether or not the average, run-of-the-mill computer now has
       * a Model 30 controller.  For the time being, I'm going to wire this to FALSE,
       * and just not support the computers mentioned above, while I try to figure out
       * how ubiquitous these newfangled 30 thingies are.
       */
      //ControllerInfo->Model30 = TRUE;
      ControllerInfo->Model30 = FALSE;
    }
  else
    {
      DPRINT("floppy: InitController: enhanced version not supported; disabling implied seeks\n");
      ControllerInfo->ImpliedSeeks = FALSE;
      ControllerInfo->Model30 = FALSE;
    }

  /* Specify */
  DPRINT("FLOPPY: FIXME: Figure out speed\n");
  HeadLoadTime = SPECIFY_HLT_500K;
  HeadUnloadTime = SPECIFY_HUT_500K;
  StepRateTime = SPECIFY_SRT_500K;

  DPRINT("floppy: InitController: issuing specify command to controller\n");

  /* Don't disable DMA --> enable dma (dumb & confusing) */
  if(HwSpecify(ControllerInfo, HeadLoadTime, HeadUnloadTime, StepRateTime, FALSE) != STATUS_SUCCESS)
    {
      DPRINT("floppy: InitController: unable to specify options\n");
      return STATUS_IO_DEVICE_ERROR;
    }

  /* Init the stop stuff */
  KeInitializeDpc(&ControllerInfo->MotorStopDpc, MotorStopDpcFunc, ControllerInfo);
  KeInitializeTimer(&ControllerInfo->MotorTimer);
  KeInitializeEvent(&ControllerInfo->MotorStoppedEvent, NotificationEvent, FALSE);
  ControllerInfo->StopDpcQueued = FALSE;

  /*
   * Recalibrate each drive on the controller (depends on StartMotor, which depends on the timer stuff above)
   * We don't even know if there is a disk in the drive, so this may not work, but that's OK.
   */
  for(i = 0; i < ControllerInfo->NumberOfDrives; i++)
    {
      DPRINT("floppy: InitController: recalibrating drive 0x%x on controller 0x%p\n", i, ControllerInfo);
      Recalibrate(&ControllerInfo->DriveInfo[i]);
    }

  DPRINT("floppy: InitController: done initializing; returning STATUS_SUCCESS\n");

  return STATUS_SUCCESS;
}


static BOOLEAN NTAPI AddControllers(PDRIVER_OBJECT DriverObject)
/*
 * FUNCTION: Called on initialization to find our controllers and build device and controller objects for them
 * ARGUMENTS:
 *     DriverObject: Our driver's DriverObject (so we can create devices against it)
 * RETURNS:
 *     FALSE if we can't allocate a device, adapter, or interrupt object, or if we fail to find any controllers
 *     TRUE otherwise (i.e. we have at least one fully-configured controller)
 * NOTES:
 *     - Currently we only support ISA buses.
 *     - BUG: Windows 2000 seems to clobber the response from the IoQueryDeviceDescription callback, so now we
 *       just test a boolean value in the first object to see if it was completely populated.  The same value
 *       is tested for each controller before we build device objects for it.
 * TODO:
 *     - Report resource usage to the HAL
 */
{
  INTERFACE_TYPE InterfaceType = Isa;
  CONFIGURATION_TYPE ControllerType = DiskController;
  CONFIGURATION_TYPE PeripheralType = FloppyDiskPeripheral;
  KAFFINITY Affinity;
  DEVICE_DESCRIPTION DeviceDescription;
  UCHAR i;
  UCHAR j;

  PAGED_CODE();

  /* Find our controllers on all ISA buses */
  IoQueryDeviceDescription(&InterfaceType, 0, &ControllerType, 0, &PeripheralType, 0, ConfigCallback, 0);

  /*
   * w2k breaks the return val from ConfigCallback, so we have to hack around it, rather than just
   * looking for a return value from ConfigCallback.  We expect at least one controller.
   */
  if(!gControllerInfo[0].Populated)
    {
      DPRINT("floppy: AddControllers: failed to get controller info from registry\n");
      return FALSE;
    }

  /* Now that we have a controller, set it up with the system */
  for(i = 0; i < gNumberOfControllers; i++)
    {
      /* 0: Report resource usage to the kernel, to make sure they aren't assigned to anyone else */
      /* FIXME: Implement me. */

      /* 1: Set up interrupt */
      gControllerInfo[i].MappedVector = HalGetInterruptVector(gControllerInfo[i].InterfaceType, gControllerInfo[i].BusNumber,
                                                              gControllerInfo[i].Level, gControllerInfo[i].Vector,
                                                              &gControllerInfo[i].MappedLevel, &Affinity);

      /* Must set up the DPC before we connect the interrupt */
      KeInitializeDpc(&gControllerInfo[i].Dpc, DpcForIsr, &gControllerInfo[i]);

      DPRINT("floppy: Connecting interrupt %d to controller%d (object 0x%p)\n", gControllerInfo[i].MappedVector,
	       i, &gControllerInfo[i]);

      /* NOTE: We cannot share our interrupt, even on level-triggered buses.  See Isr() for details. */
      if(IoConnectInterrupt(&gControllerInfo[i].InterruptObject, Isr, &gControllerInfo[i], 0, gControllerInfo[i].MappedVector,
         gControllerInfo[i].MappedLevel, gControllerInfo[i].MappedLevel, gControllerInfo[i].InterruptMode,
         FALSE, Affinity, 0) != STATUS_SUCCESS)
        {
          DPRINT("floppy: AddControllers: unable to connect interrupt\n");
          continue;
        }

      /* 2: Set up DMA */
      memset(&DeviceDescription, 0, sizeof(DeviceDescription));
      DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
      DeviceDescription.DmaChannel = gControllerInfo[i].Dma;
      DeviceDescription.InterfaceType = gControllerInfo[i].InterfaceType;
      DeviceDescription.BusNumber = gControllerInfo[i].BusNumber;
      DeviceDescription.MaximumLength = 2*18*512; /* based on a 1.44MB floppy */

      /* DMA 0,1,2,3 are 8-bit; 4,5,6,7 are 16-bit (4 is chain i think) */
      DeviceDescription.DmaWidth = gControllerInfo[i].Dma > 3 ? Width16Bits: Width8Bits;

      gControllerInfo[i].AdapterObject = HalGetAdapter(&DeviceDescription, &gControllerInfo[i].MapRegisters);

      if(!gControllerInfo[i].AdapterObject)
        {
          DPRINT("floppy: AddControllers: unable to allocate an adapter object\n");
          IoDisconnectInterrupt(gControllerInfo[i].InterruptObject);
          continue;
        }

      /* 2b: Initialize the new controller */
      if(InitController(&gControllerInfo[i]) != STATUS_SUCCESS)
	{
	  DPRINT("floppy: AddControllers():Unable to set up controller %d - initialization failed\n", i);
          IoDisconnectInterrupt(gControllerInfo[i].InterruptObject);
	  continue;
	}

      /* 2c: Set the controller's initlized flag so we know to release stuff in Unload */
      gControllerInfo[i].Initialized = TRUE;

      /* 3: per-drive setup */
      for(j = 0; j < gControllerInfo[i].NumberOfDrives; j++)
        {
          WCHAR DeviceNameBuf[MAX_DEVICE_NAME];
          UNICODE_STRING DeviceName;
          UNICODE_STRING LinkName;
	  UNICODE_STRING ArcPath;
	  UCHAR DriveNumber;

	  DPRINT("floppy: AddControllers(): Configuring drive %d on controller %d\n", i, j);

	  /*
	   * 3a: create a device object for the drive
	   * Controllers and drives are 0-based, so the combos are:
	   * 0: 0,0
	   * 1: 0,1
	   * 2: 0,2
	   * 3: 0,3
	   * 4: 1,0
	   * 5: 1,1
	   * ...
	   * 14: 3,2
	   * 15: 3,3
	   */

	  DriveNumber = (UCHAR)(i*4 + j); /* loss of precision is OK; there are only 16 of 'em */

	  RtlZeroMemory(&DeviceNameBuf, MAX_DEVICE_NAME * sizeof(WCHAR));
          swprintf(DeviceNameBuf, L"\\Device\\Floppy%d", DriveNumber);
          RtlInitUnicodeString(&DeviceName, DeviceNameBuf);

          if(IoCreateDevice(DriverObject, sizeof(PVOID), &DeviceName,
			    FILE_DEVICE_DISK, FILE_REMOVABLE_MEDIA | FILE_FLOPPY_DISKETTE, FALSE,
                            &gControllerInfo[i].DriveInfo[j].DeviceObject) != STATUS_SUCCESS)
            {
              DPRINT("floppy: AddControllers: unable to register a Device object\n");
              IoDisconnectInterrupt(gControllerInfo[i].InterruptObject);
              continue; /* continue on to next drive */
            }

	  DPRINT("floppy: AddControllers: New device: %S (0x%p)\n", DeviceNameBuf, gControllerInfo[i].DriveInfo[j].DeviceObject);

	  /* 3b.5: Create an ARC path in case we're booting from this drive */
	  swprintf(gControllerInfo[i].DriveInfo[j].ArcPathBuffer,
		   L"\\ArcName\\multi(%d)disk(%d)fdisk(%d)", gControllerInfo[i].BusNumber, i, DriveNumber);

	  RtlInitUnicodeString(&ArcPath, gControllerInfo[i].DriveInfo[j].ArcPathBuffer);
	  IoAssignArcName(&ArcPath, &DeviceName);

	  /* 3c: Set flags up */
	  gControllerInfo[i].DriveInfo[j].DeviceObject->Flags |= DO_DIRECT_IO;

	  /* 3d: Create a symlink */
	  swprintf(gControllerInfo[i].DriveInfo[j].SymLinkBuffer, L"\\DosDevices\\%c:", DriveNumber + 'A');
	  RtlInitUnicodeString(&LinkName, gControllerInfo[i].DriveInfo[j].SymLinkBuffer);
	  if(IoCreateSymbolicLink(&LinkName, &DeviceName) != STATUS_SUCCESS)
	    {
	      DPRINT("floppy: AddControllers: Unable to create a symlink for drive %d\n", DriveNumber);
	      IoDisconnectInterrupt(gControllerInfo[i].InterruptObject);
	      IoDeassignArcName(&ArcPath);
	      continue; /* continue to next drive */
	    }

	  /* 3e: Set up the DPC */
	  IoInitializeDpcRequest(gControllerInfo[i].DriveInfo[j].DeviceObject, DpcForIsr);

	  /* 3f: Point the device extension at our DriveInfo struct */
	  gControllerInfo[i].DriveInfo[j].DeviceObject->DeviceExtension = &gControllerInfo[i].DriveInfo[j];

	  /* 3g: neat comic strip */

	  /* 3h: set the initial media type to unknown */
	  memset(&gControllerInfo[i].DriveInfo[j].DiskGeometry, 0, sizeof(DISK_GEOMETRY));
	  gControllerInfo[i].DriveInfo[j].DiskGeometry.MediaType = Unknown;

	  /* 3i: Now that we're done, set the Initialized flag so we know to free this in Unload */
	  gControllerInfo[i].DriveInfo[j].Initialized = TRUE;
        }
    }

  DPRINT("floppy: AddControllers: --------------------------------------------> finished adding controllers\n");

  return TRUE;
}


VOID NTAPI SignalMediaChanged(PDEVICE_OBJECT DeviceObject,
                              PIRP Irp)
/*
 * FUNCTION: Process an IRP when the media has changed, and possibly notify the user
 * ARGUMENTS:
 *     DeviceObject: DeviceObject associated with the IRP
 *     Irp: IRP that we're failing due to change
 * NOTES:
 *     - This procedure is documented in the DDK by "Notifying the File System of Possible Media Changes",
 *       "IoSetHardErrorOrVerifyDevice", and by "Responding to Check-Verify Requests from the File System".
 *     - Callable at <= DISPATCH_LEVEL
 */
{
  PDRIVE_INFO DriveInfo = DeviceObject->DeviceExtension;

  DPRINT("floppy: SignalMediaChanged called\n");

  DriveInfo->DiskChangeCount++;

  /* If volume is not mounted, do NOT set verify and return STATUS_IO_DEVICE_ERROR */
  if(!(DeviceObject->Vpb->Flags & VPB_MOUNTED))
    {
      Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
      Irp->IoStatus.Information = 0;
      return;
    }

  /* Notify the filesystem that it will need to verify the volume */
  DeviceObject->Flags |= DO_VERIFY_VOLUME;
  Irp->IoStatus.Status = STATUS_VERIFY_REQUIRED;
  Irp->IoStatus.Information = 0;

  /*
   * If this is a user-based, threaded request, let the IO manager know to pop up a box asking
   * the user to supply the correct media, but only if the error (which we just picked out above)
   * is deemed by the IO manager to be "user induced".  The reason we don't just unconditionally
   * call IoSetHardError... is because MS might change the definition of "user induced" some day,
   * and we don't want to have to remember to re-code this.
   */
  if(Irp->Tail.Overlay.Thread && IoIsErrorUserInduced(Irp->IoStatus.Status))
    IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
}


static VOID NTAPI QueueThread(PVOID Context)
/*
 * FUNCTION: Thread that manages the queue and dispatches any queued requests
 * ARGUMENTS:
 *     Context: unused
 */
{
  PIRP Irp;
  PIO_STACK_LOCATION Stack;
  PDEVICE_OBJECT DeviceObject;
  PVOID Objects[2];

  PAGED_CODE();
  UNREFERENCED_PARAMETER(Context);

  Objects[0] = &QueueSemaphore;
  Objects[1] = &QueueThreadTerminate;

  for(;;)
    {
      KeWaitForMultipleObjects(2, Objects, WaitAny, Executive, KernelMode, FALSE, NULL, NULL);

      if(KeReadStateEvent(&QueueThreadTerminate))
	{
	  DPRINT("floppy: QueueThread terminating\n");
          return;
	}

      DPRINT("floppy: QueueThread: servicing an IRP\n");

      Irp = IoCsqRemoveNextIrp(&Csq, 0);

      /* we won't get an irp if it was canceled */
      if(!Irp)
	{
	  DPRINT("floppy: QueueThread: IRP queue empty\n");
          continue;
	}

      DeviceObject = (PDEVICE_OBJECT)Irp->Tail.Overlay.DriverContext[0];

      ASSERT(DeviceObject);

      Stack = IoGetCurrentIrpStackLocation(Irp);

      /* Decide what to do with the IRP */
      switch(Stack->MajorFunction)
	{
	case IRP_MJ_READ:
	case IRP_MJ_WRITE:
          ReadWritePassive(DeviceObject->DeviceExtension, Irp);
	  break;

	case IRP_MJ_DEVICE_CONTROL:
	  DeviceIoctlPassive(DeviceObject->DeviceExtension, Irp);
	  break;

	default:
	  DPRINT("floppy: QueueThread(): Unrecognized irp: mj: 0x%x\n", Stack->MajorFunction);
	  Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	  Irp->IoStatus.Information = 0;
	  IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
    }
}


NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject,
                           PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Entry-point for the driver
 * ARGUMENTS:
 *     DriverObject: Our driver object
 *     RegistryPath: Unused
 * RETURNS:
 *     STATUS_SUCCESS on successful initialization of at least one drive
 *     STATUS_NO_SUCH_DEVICE if we didn't find even one drive
 *     STATUS_UNSUCCESSFUL otherwise
 */
{
  HANDLE ThreadHandle;

  UNREFERENCED_PARAMETER(RegistryPath);

  /*
   * Set up dispatch routines
   */
  DriverObject->MajorFunction[IRP_MJ_CREATE]         = (PDRIVER_DISPATCH)CreateClose;
  DriverObject->MajorFunction[IRP_MJ_CLOSE]          = (PDRIVER_DISPATCH)CreateClose;
  DriverObject->MajorFunction[IRP_MJ_READ]           = (PDRIVER_DISPATCH)ReadWrite;
  DriverObject->MajorFunction[IRP_MJ_WRITE]          = (PDRIVER_DISPATCH)ReadWrite;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = (PDRIVER_DISPATCH)DeviceIoctl;

  DriverObject->DriverUnload = Unload;

  /*
   * We depend on some zeroes in these structures.  I know this is supposed to be
   * initialized to 0 by the complier but this makes me feel beter.
   */
  memset(&gControllerInfo, 0, sizeof(gControllerInfo));

  /*
   * Set up queue.  This routine cannot fail (trust me, I wrote it).
   */
  IoCsqInitialize(&Csq, CsqInsertIrp, CsqRemoveIrp, CsqPeekNextIrp,
                  CsqAcquireLock, CsqReleaseLock, CsqCompleteCanceledIrp);

  /*
   * ...and its lock
   */
  KeInitializeSpinLock(&IrpQueueLock);

  /*
   * ...and the queue list itself
   */
  InitializeListHead(&IrpQueue);

  /*
   * The queue is counted by a semaphore.  The queue management thread
   * blocks on this semaphore, so if requests come in faster than the queue
   * thread can handle them, the semaphore count goes up.
   */
  KeInitializeSemaphore(&QueueSemaphore, 0, 0x7fffffff);

  /*
   * Event to terminate that thread
   */
  KeInitializeEvent(&QueueThreadTerminate, NotificationEvent, FALSE);

  /*
   * Create the queue processing thread.  Save its handle in the global variable
   * ThreadHandle so we can wait on its termination during Unload.
   */
  if(PsCreateSystemThread(&ThreadHandle, THREAD_ALL_ACCESS, 0, 0, 0, QueueThread, 0) != STATUS_SUCCESS)
    {
      DPRINT("floppy: Unable to create system thread; failing init\n");
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  if(ObReferenceObjectByHandle(ThreadHandle, STANDARD_RIGHTS_ALL, NULL, KernelMode, &QueueThreadObject, NULL) != STATUS_SUCCESS)
    {
      DPRINT("floppy: Unable to reference returned thread handle; failing init\n");
      return STATUS_UNSUCCESSFUL;
    }

  /*
   * Close the handle, now that we have the object pointer and a reference of our own.
   * The handle will certainly not be valid in the context of the caller next time we
   * need it, as handles are process-specific.
   */
  ZwClose(ThreadHandle);

  /*
   * Start the device discovery proces.  Returns STATUS_SUCCESS if
   * it finds even one drive attached to one controller.
   */
  if(!AddControllers(DriverObject))
    return STATUS_NO_SUCH_DEVICE;

  return STATUS_SUCCESS;
}

