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
 *
 * ---- General to-do items ----
 * TODO: Clean up properly on failed init
 * TODO: Add arc-path support so we can boot from the floppy
 * TODO: Fix all these stupid STATUS_UNSUCCESSFUL return values
 * TODO: Think about IO_NO_INCREMENT
 * TODO: Figure out why CreateClose isn't called any more on XP.  Seems to correspond 
 *       with the driver not being unloadable.  Does it have to do with cleanup?
 * TODO: Consider using the built-in device object pointer in the stack location 
 *       rather than the context area
 * TODO: Think about StopDpcQueued -- could be a race; too tired atm to tell
 *
 * ---- Support for proper media detection ----
 * TODO: Handle MFM flag
 * TODO: Un-hardcode the data rate from various places
 * TODO: Proper media detection (right now we're hardcoded to 1.44)
 * TODO: Media detection based on sector 1
 *
 * ---- Support for normal floppy hardware ----
 * TODO: Support the three primary types of controller
 * TODO: Figure out thinkpad compatibility (I've heard rumors of weirdness with them)
 *
 * ---- Support for non-ISA and/or non-slave-dma controllers, if they exist ----
 * TODO: Find controllers on non-ISA buses
 * TODO: Think about making the interrupt shareable
 * TODO: Support bus-master controllers.  PCI will break ATM.
 */

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
CONTROLLER_INFO gControllerInfo[MAX_CONTROLLERS];
ULONG gNumberOfControllers = 0;

/* Queue thread management */
KEVENT QueueThreadTerminate;
PVOID ThreadObject;

/* ISR DPC */
KDPC Dpc;


static VOID NTAPI MotorStopDpcFunc(PKDPC Dpc,
			    PVOID DeferredContext,
			    PVOID SystemArgument1,
			    PVOID SystemArgument2)
/*
 * FUNCTION: Stop the floppy motor
 * ARGUMENTS:
 *     Dpc: DPC object that's going off
 *     DeferredContext: called with DRIVE_INFO for drive to turn off
 *     SystemArgument1: unused
 *     SystemArgument2: unused
 * NOTES:
 *     - Must set an event to let other threads know we're done turning off the motor
 *     - Called back at DISPATCH_LEVEL
 */
{
  PCONTROLLER_INFO ControllerInfo = (PCONTROLLER_INFO)DeferredContext;

  ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
  ASSERT(ControllerInfo);

  KdPrint(("floppy: MotorStopDpcFunc called\n"));

  HwTurnOffMotor(ControllerInfo);
  ControllerInfo->StopDpcQueued = FALSE;
  KeSetEvent(&ControllerInfo->MotorStoppedEvent, IO_NO_INCREMENT, FALSE);
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

  KdPrint(("floppy: StartMotor called\n"));

  if(DriveInfo->ControllerInfo->StopDpcQueued && 
     !KeCancelTimer(&DriveInfo->ControllerInfo->MotorTimer))
    {
      /* Motor turner-offer is already running; wait for it to finish */
      KeWaitForSingleObject(&DriveInfo->ControllerInfo->MotorStoppedEvent, Executive, KernelMode, FALSE, NULL);
      DriveInfo->ControllerInfo->StopDpcQueued = FALSE;
    }

  HwTurnOnMotor(DriveInfo);
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

  KdPrint(("floppy: StopMotor called\n"));

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
 * TODO: Figure out why this isn't getting called any more, and remove the ASSERT once that happens
 */
{
  KdPrint(("floppy: CreateClose called\n"));

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = FILE_OPENED;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}


static NTSTATUS NTAPI Recalibrate(PDRIVE_INFO DriveInfo)
/*
 * FUNCTION: Start the recalibration process
 * ARGUMENTS:
 *     DriveInfo: Pointer to the driveinfo struct associated with the targeted drive
 * RETURNS:
 *     STATUS_SUCCESS on successful starting of the process
 *     STATUS_UNSUCCESSFUL if it fails
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
  KdPrint(("floppy: FIXME: UN-HARDCODE DATA RATE\n"));
  HwSetDataRate(DriveInfo->ControllerInfo, 0);

  /* clear the event just in case the last call forgot */
  KeClearEvent(&DriveInfo->ControllerInfo->SynchEvent);

  /* sometimes you have to do this twice */
  for(i = 0; i < 2; i++)
    {
      /* Send the command */
      Status = HwRecalibrate(DriveInfo);
      if(Status != STATUS_SUCCESS)
	{
	  KdPrint(("floppy: Recalibrate: HwRecalibrate returned error\n"));
          continue;
	}

      WaitForControllerInterrupt(DriveInfo->ControllerInfo);

      /* Get the results */
      Status = HwRecalibrateResult(DriveInfo->ControllerInfo);
      if(Status != STATUS_SUCCESS)
	{
	  KdPrint(("floppy: Recalibrate: HwRecalibrateResult returned error\n"));
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

  KdPrint(("floppy: ResetChangeFlag called\n"));

  /* Try to recalibrate.  We don't care if it works. */
  Recalibrate(DriveInfo);

  /* clear spurious interrupts in prep for seeks */
  KeClearEvent(&DriveInfo->ControllerInfo->SynchEvent);

  /* Seek to 1 */
  if(HwSeek(DriveInfo, 1) != STATUS_SUCCESS)
    {
      KdPrint(("floppy: ResetChangeFlag(): HwSeek failed; returning STATUS_IO_DEVICE_ERROR\n"));
      return STATUS_IO_DEVICE_ERROR;
    }

  WaitForControllerInterrupt(DriveInfo->ControllerInfo);

  HwSenseInterruptStatus(DriveInfo->ControllerInfo);

  /* Seek back to 0 */
  if(HwSeek(DriveInfo, 1) != STATUS_SUCCESS)
    {
      KdPrint(("floppy: ResetChangeFlag(): HwSeek failed; returning STATUS_IO_DEVICE_ERROR\n"));
      return STATUS_IO_DEVICE_ERROR;
    }

  WaitForControllerInterrupt(DriveInfo->ControllerInfo);

  HwSenseInterruptStatus(DriveInfo->ControllerInfo);

  /* Check the change bit */
  if(HwDiskChanged(DriveInfo, &DiskChanged) != STATUS_SUCCESS)
    {
      KdPrint(("floppy: ResetChangeFlag(): HwDiskChagned failed; returning STATUS_IO_DEVICE_ERROR\n"));
      return STATUS_IO_DEVICE_ERROR;
    }

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
 *
 * TODO: Delete ARC links
 */
{
  ULONG i,j;

  PAGED_CODE();

  KdPrint(("floppy: unloading\n"));

  KeSetEvent(&QueueThreadTerminate, 0, FALSE);
  KeWaitForSingleObject(ThreadObject, Executive, KernelMode, FALSE, 0);
  ObDereferenceObject(ThreadObject);

  for(i = 0; i < gNumberOfControllers; i++)
    {
      if(!gControllerInfo[i].Populated)
	continue;

      for(j = 0; j < gControllerInfo[i].NumberOfDrives; j++)
	{
          if(gControllerInfo[i].DriveInfo[j].DeviceObject)
            {
	      UNICODE_STRING Link;
	      RtlInitUnicodeString(&Link, gControllerInfo[i].DriveInfo[j].SymLinkBuffer);
	      IoDeleteSymbolicLink(&Link);
              IoDeleteDevice(gControllerInfo[i].DriveInfo[j].DeviceObject);
            }
	}

      IoDisconnectInterrupt(gControllerInfo[i].InterruptObject);

      /* Power down the controller */
      HwPowerOff(&gControllerInfo[i]);
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

  KdPrint(("floppy: ConfigCallback called with ControllerNumber %d\n", ControllerNumber));

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
            ASSERT(0);

          if(AddressSpace == 0)
            gControllerInfo[gNumberOfControllers].BaseAddress = MmMapIoSpace(TranslatedAddress, 8, FALSE); // symbolic constant?
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
 * TODO:
 *     - This driver really cannot shrare interrupts, as I don't know how to conclusively say 
 *       whether it was our controller that interrupted or not.  I just have to assume that any time
 *       my ISR gets called, it was my board that called it.  Dumb design, yes, but it goes back to 
 *       the semantics of ISA buses.  That, and I don't know much about ISA drivers. :-)
 *       UPDATE: The high bit of Status Register A seems to work on non-AT controllers.
 *     - Called at DIRQL
 */
{
  PCONTROLLER_INFO ControllerInfo = (PCONTROLLER_INFO)ServiceContext;

  ASSERT(ControllerInfo);

  KdPrint(("floppy: ISR called\n"));

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


VOID NTAPI DpcForIsr(PKDPC Dpc,
                     PVOID Context,
                     PVOID SystemArgument1,
                     PVOID SystemArgument2)
/*
 * FUNCTION: This DPC gets queued by every ISR.  Does the real per-interrupt work.
 * ARGUMENTS:
 *     Dpc: Pointer to the DPC object that represents our function
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

  ASSERT(ControllerInfo);

  KdPrint(("floppy: DpcForIsr called\n"));

  KeSetEvent(&ControllerInfo->SynchEvent, IO_NO_INCREMENT, FALSE);
}


static NTSTATUS NTAPI InitController(PCONTROLLER_INFO ControllerInfo)
/*
 * FUNCTION:  Initialize a newly-found controller
 * ARGUMENTS:
 *     ControllerInfo: pointer to the controller to be initialized
 * RETURNS:
 *     STATUS_SUCCESS if the controller is successfully initialized
 *     STATUS_UNSUCCESSFUL otherwise
 */
{
  int i;
  UCHAR HeadLoadTime;
  UCHAR HeadUnloadTime;
  UCHAR StepRateTime;

  PAGED_CODE();
  ASSERT(ControllerInfo);

  KdPrint(("floppy: InitController called with Controller 0x%x\n", ControllerInfo));

  KeClearEvent(&ControllerInfo->SynchEvent);

  //HwDumpRegisters(ControllerInfo);

  KdPrint(("floppy: InitController: resetting the controller\n"));

  /* Reset the controller */
  if(HwReset(ControllerInfo) != STATUS_SUCCESS)
    {
      KdPrint(("floppy: InitController: unable to reset controller\n"));
      return STATUS_UNSUCCESSFUL;
    }

  //HwDumpRegisters(ControllerInfo);

  KdPrint(("floppy: InitController: setting data rate\n"));

  /* Set data rate */
  if(HwSetDataRate(ControllerInfo, DRSR_DSEL_500KBPS) != STATUS_SUCCESS)
    {
      KdPrint(("floppy: InitController: unable to set data rate\n"));
      return STATUS_UNSUCCESSFUL;
    }

  KdPrint(("floppy: InitController: waiting for initial interrupt\n"));

  /* Wait for an interrupt */
  WaitForControllerInterrupt(ControllerInfo);

  /* Reset means you have to clear each of the four interrupts (one per drive) */
  for(i = 0; i < MAX_DRIVES_PER_CONTROLLER; i++)
    {
      KdPrint(("floppy: InitController: Sensing interrupt %d\n", i));
      //HwDumpRegisters(ControllerInfo);

      if(HwSenseInterruptStatus(ControllerInfo) != STATUS_SUCCESS)
	{
	  KdPrint(("floppy: InitController: Unable to clear interrupt 0x%x\n", i));
	  return STATUS_UNSUCCESSFUL;
	}
    }

  KdPrint(("floppy: InitController: done sensing interrupts\n"));

  //HwDumpRegisters(ControllerInfo);

  /* Next, see if we have the right version to do implied seek */
  if(HwGetVersion(ControllerInfo) != VERSION_ENHANCED)
    {
      KdPrint(("floppy: InitController: enhanced version not supported; disabling implied seeks\n"));
      ControllerInfo->ImpliedSeeks = FALSE;
      ControllerInfo->Model30 = FALSE;
    }
  else
    {
      /* If so, set that up -- all defaults below except first TRUE for EIS */
      if(HwConfigure(ControllerInfo, TRUE, TRUE, FALSE, 0, 0) != STATUS_SUCCESS)
	{
	  KdPrint(("floppy: InitController: unable to set up implied seek\n"));
          ControllerInfo->ImpliedSeeks = FALSE;
	}
      else 
	{
	  KdPrint(("floppy: InitController: implied seeks set!\n"));
          ControllerInfo->ImpliedSeeks = TRUE;
	}

      ControllerInfo->Model30 = TRUE;
    }
  
  /* Specify */
  KdPrint(("FLOPPY: FIXME: Figure out speed\n"));
  HeadLoadTime = SPECIFY_HLT_500K;
  HeadUnloadTime = SPECIFY_HUT_500K;
  StepRateTime = SPECIFY_SRT_500K;

  KdPrint(("floppy: InitController: issuing specify command to controller\n"));
      
  /* Don't disable DMA --> enable dma (dumb & confusing) */
  if(HwSpecify(ControllerInfo, HeadLoadTime, HeadUnloadTime, StepRateTime, FALSE) != STATUS_SUCCESS)
    {
      KdPrint(("floppy: InitController: unable to specify options\n"));
      return STATUS_UNSUCCESSFUL;
    }

  //HwDumpRegisters(ControllerInfo);

  /* Init the stop stuff */
  KeInitializeDpc(&ControllerInfo->MotorStopDpc, MotorStopDpcFunc, ControllerInfo);
  KeInitializeTimer(&ControllerInfo->MotorTimer);
  KeInitializeEvent(&ControllerInfo->MotorStoppedEvent, SynchronizationEvent, FALSE);
  ControllerInfo->StopDpcQueued = FALSE;

  /* Recalibrate each drive on the controller (depends on StartMotor, which depends on the timer stuff above) */
  /* TODO: Handle failure of one or more drives */
  for(i = 0; i < ControllerInfo->NumberOfDrives; i++)
    {
      KdPrint(("floppy: InitController: recalibrating drive 0x%x on controller 0x%x\n", i, ControllerInfo));

      if(Recalibrate(&ControllerInfo->DriveInfo[i]) != STATUS_SUCCESS)
	{
	  KdPrint(("floppy: InitController: unable to recalibrate drive\n"));
	  return STATUS_UNSUCCESSFUL;
	}
    }

  //HwDumpRegisters(ControllerInfo);

  KdPrint(("floppy: InitController: done initializing; returning STATUS_SUCCESS\n"));

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
 *     - Figure out a workable interrupt-sharing scheme and un-hardcode FALSE in IoConnectInterrupt
 *     - Add support for non-ISA buses, by looping through all of the bus types looking for floppy controllers
 *     - Report resource usage to the HAL
 *     - Add ARC path support
 *     - Think more about error handling; atm most errors abort the start of the driver
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
   * looking for a return value from ConfigCallback
   */ 
  if(!gControllerInfo[0].Populated)
    {
      KdPrint(("floppy: AddControllers: failed to get controller info from registry\n"));
      return FALSE;
    }

  /* Now that we have a controller, set it up with the system */
  for(i = 0; i < gNumberOfControllers; i++)
    {
      /* 0: Report resource usage to the kernel, to make sure they aren't assigned to anyone else */
      /* XXX do me baby */

      /* 1: Set up interrupt */
      gControllerInfo[i].MappedVector = HalGetInterruptVector(gControllerInfo[i].InterfaceType, gControllerInfo[i].BusNumber,
                                                              gControllerInfo[i].Level, gControllerInfo[i].Vector,
                                                              &gControllerInfo[i].MappedLevel, &Affinity);

      /* Must set up the DPC before we connect the interrupt */
      KeInitializeDpc(&gControllerInfo[i].Dpc, DpcForIsr, &gControllerInfo[i]);

      KdPrint(("floppy: Connecting interrupt %d to controller%d (object 0x%x)\n", gControllerInfo[i].MappedVector,
	       i, &gControllerInfo[i]));

      /* NOTE: We cannot share our interrupt, even on level-triggered buses.  See Isr() for details. */
      if(IoConnectInterrupt(&gControllerInfo[i].InterruptObject, Isr, &gControllerInfo[i], 0, gControllerInfo[i].MappedVector,
         gControllerInfo[i].MappedLevel, gControllerInfo[i].MappedLevel, gControllerInfo[i].InterruptMode,
         FALSE, Affinity, 0) != STATUS_SUCCESS)
        {
          KdPrint(("floppy: AddControllers: unable to connect interrupt\n"));
          return FALSE;
        }

      /* 2: Set up DMA */

      memset(&DeviceDescription, 0, sizeof(DeviceDescription));
      DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
      DeviceDescription.Master = (gControllerInfo[i].InterfaceType == PCIBus ? TRUE : FALSE); /* guessing if not pci not master */
      DeviceDescription.DmaChannel = gControllerInfo[i].Dma;
      DeviceDescription.InterfaceType = gControllerInfo[i].InterfaceType;
      DeviceDescription.BusNumber = gControllerInfo[i].BusNumber;

      if(gControllerInfo[i].InterfaceType == PCIBus)
        {
          DeviceDescription.Dma32BitAddresses = TRUE;
          DeviceDescription.DmaWidth = Width32Bits;
        }
      else
	/* DMA 0,1,2,3 are 8-bit; 4,5,6,7 are 16-bit (4 is chain i think) */
        DeviceDescription.DmaWidth = gControllerInfo[i].Dma > 3 ? Width16Bits: Width8Bits;

      gControllerInfo[i].AdapterObject = HalGetAdapter(&DeviceDescription, &gControllerInfo[i].MapRegisters);

      if(!gControllerInfo[i].AdapterObject)
        {
          KdPrint(("floppy: AddControllers: unable to allocate an adapter object\n"));
          IoDisconnectInterrupt(gControllerInfo[i].InterruptObject);
          return FALSE;
        }

      /* 2b: Initialize the new controller */
      if(InitController(&gControllerInfo[i]) != STATUS_SUCCESS)
	{
	  KdPrint(("floppy: AddControllers():Unable to set up controller %d - initialization failed\n", i));
	  ASSERT(0); /* FIXME: clean up properly */
	  continue;
	}

      /* 3: per-drive setup */
      for(j = 0; j < gControllerInfo[i].NumberOfDrives; j++)
        {
          WCHAR DeviceNameBuf[MAX_DEVICE_NAME];
          UNICODE_STRING DeviceName;
          UNICODE_STRING LinkName;
	  UCHAR DriveNumber;

	  KdPrint(("floppy: AddControllers(): Configuring drive %d on controller %d\n", i, j));

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
	  DriveNumber = i*4 + j;

          swprintf(DeviceNameBuf, L"\\Device\\Floppy%d", DriveNumber);
          RtlInitUnicodeString(&DeviceName, DeviceNameBuf);

          if(IoCreateDevice(DriverObject, sizeof(PVOID), &DeviceName, 
			    FILE_DEVICE_DISK, FILE_REMOVABLE_MEDIA | FILE_FLOPPY_DISKETTE, FALSE,
                            &gControllerInfo[i].DriveInfo[j].DeviceObject) != STATUS_SUCCESS)
            {
              KdPrint(("floppy: AddControllers: unable to register a Device object\n"));
              IoDisconnectInterrupt(gControllerInfo[i].InterruptObject);
              return FALSE;
            }

	  KdPrint(("floppy: AddControllers: New device: %S (0x%x)\n", DeviceNameBuf, gControllerInfo[i].DriveInfo[j].DeviceObject));

	  /* 3c: Set flags up */
	  gControllerInfo[i].DriveInfo[j].DeviceObject->Flags |= DO_DIRECT_IO;

	  /* 3d: Create a symlink */
	  swprintf(gControllerInfo[i].DriveInfo[j].SymLinkBuffer, L"\\DosDevices\\%c:", DriveNumber + 'A');
	  RtlInitUnicodeString(&LinkName, gControllerInfo[i].DriveInfo[j].SymLinkBuffer);
	  if(IoCreateSymbolicLink(&LinkName, &DeviceName) != STATUS_SUCCESS)
	    {
	      KdPrint(("floppy: AddControllers: Unable to create a symlink for drive %d\n", DriveNumber));
	      IoDisconnectInterrupt(gControllerInfo[i].InterruptObject);
	      // delete devices too?
	      return FALSE;
	    }

	  /* 3e: Set up the DPC */
	  IoInitializeDpcRequest(gControllerInfo[i].DriveInfo[j].DeviceObject, DpcForIsr);

	  /* 3f: Point the device extension at our DriveInfo struct */
	  gControllerInfo[i].DriveInfo[j].DeviceObject->DeviceExtension = &gControllerInfo[i].DriveInfo[j];

	  /* 3g: neat comic strip */

	  /* 3h: set the initial media type to unknown */
	  memset(&gControllerInfo[i].DriveInfo[j].DiskGeometry, 0, sizeof(DISK_GEOMETRY));
	  gControllerInfo[i].DriveInfo[j].DiskGeometry.MediaType = Unknown;
        }
    }

  KdPrint(("floppy: AddControllers: --------------------------------------------> finished adding controllers\n"));

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

  KdPrint(("floppy: SignalMediaChanged called\n"));

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


VOID NTAPI QueueThread(PVOID Context)
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

  Objects[0] = &QueueSemaphore;
  Objects[1] = &QueueThreadTerminate;

  for(;;)
    {
      KeWaitForMultipleObjects(2, Objects, WaitAny, Executive, KernelMode, FALSE, NULL, NULL);

      if(KeReadStateEvent(&QueueThreadTerminate))
	{
	  KdPrint(("floppy: QueueThread terminating\n"));
          return;
	}

      KdPrint(("floppy: QueueThread: servicing an IRP\n"));

      Irp = IoCsqRemoveNextIrp(&Csq, 0);

      /* we won't get an irp if it was canceled */
      if(!Irp)
	{
	  KdPrint(("floppy: QueueThread: IRP queue empty\n"));
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
	  KdPrint(("floppy: QueueThread(): Unrecognized irp: mj: 0x%x\n", Stack->MajorFunction));
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
   * Create the queue processing thread.  Save its handle in the global variable
   * ThreadHandle so we can wait on its termination during Unload.
   */
  if(PsCreateSystemThread(&ThreadHandle, 0, 0, 0, 0, QueueThread, 0) != STATUS_SUCCESS)
    {
      KdPrint(("floppy: Unable to create system thread; failing init\n"));
      return STATUS_UNSUCCESSFUL;
    }

  if(ObReferenceObjectByHandle(ThreadHandle, STANDARD_RIGHTS_ALL, NULL, KernelMode, &ThreadObject, NULL) != STATUS_SUCCESS)
    {
      KdPrint(("floppy: Unable to reference returned thread handle; failing init\n"));
      return STATUS_UNSUCCESSFUL;
    }

  ZwClose(ThreadHandle);

  /*
   * Event to terminate that thread
   */
  KeInitializeEvent(&QueueThreadTerminate, NotificationEvent, FALSE);

  /*
   * Start the device discovery proces.  In theory, this should return STATUS_SUCCESS if
   * it finds even one drive attached to one controller.  In practice, the AddControllers
   * routine doesn't handle all of the errors right just yet.  FIXME.
   */
  if(!AddControllers(DriverObject))
    return STATUS_NO_SUCH_DEVICE;

  return STATUS_SUCCESS;
}

