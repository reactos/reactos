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
 * FILE:            hardware.c
 * PURPOSE:         FDC Hardware control routines
 * PROGRAMMER:      Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *                  15-Feb-2004 vizzini - Created
 * NOTES:
 *     - Many of these functions are based directly on information from the 
 *       Intel datasheet for their enhanced floppy controller.  Send_Byte and
 *       Get_Byte are direct C implementations of their flowcharts, and the 
 *       read/write routine and others are loose adaptations of their charts.
 *     - These routines are generally designed to be small, atomic operations.  They
 *       do not wait for interrupts, deal with DMA, or do any other Windows-
 *       specific things, unless they have to.
 *     - If you compare this to Microsoft samples or to the old ReactOS driver,
 *       or even to the linux driver, you will notice a big difference:  we use
 *       a system thread to drain the queue.  This is because it's illegal to block
 *       in a dispatch routine, unless you're a top-level driver (which we absolutely
 *       are not).  One big reason is that we may be called at raised IRQL, at which
 *       it's illegal to block.  The floppy controller is a *dumb* piece of hardware,
 *       too - it is slow and difficult to deal with.  The solution is to do all
 *       of the blocking and servicing of the controller in a dedicated worker
 *       thread.  
 *     - Some information taken from Intel 82077AA data sheet (order #290166-007)
 *
 * TODO: ATM the constants defined in hardware.h *might* be shifted to line up
 *       with the bit position in the register, or they *might not*.  This should
 *       all be converted to standardize on absolute values or shifts.
 *       I prefer bit fields, but they break endianness.
 * TODO: Figure out the right delays in Send_Byte and Get_Byte
 */

#include <ntddk.h>

#include "floppy.h"
#include "hardware.h"

/*
 * Global variable that tracks the amount of time we've
 * been waiting on the controller
 */
static ULONG TimeIncrement = 0;


/*
 * Hardware Support Routines
 */


static BOOLEAN NTAPI ReadyForWrite(PCONTROLLER_INFO ControllerInfo)
/*
 * FUNCTION: Determine of the controller is ready to accept a byte on the FIFO
 * ARGUMENTS:
 *     ControllerInfo: Info structure for the FDC we're testing
 * RETURNS:
 *     TRUE if the controller can accept a byte right now
 *     FALSE otherwise
 * NOTES:
 *     - it is necessary to check both that the FIFO is set to "outbound" 
 *       and that the "ready for i/o" bit is set.
 */
{
  UCHAR Status = READ_PORT_UCHAR(ControllerInfo->BaseAddress + MAIN_STATUS_REGISTER);
	
  if((Status & MSR_IO_DIRECTION)) /* 0 for out */
      return FALSE;

  if(!(Status & MSR_DATA_REG_READY_FOR_IO))
      return FALSE;

  return TRUE;
}


static BOOLEAN NTAPI ReadyForRead(PCONTROLLER_INFO ControllerInfo)
/*
 * FUNCTION: Determine of the controller is ready to read a byte on the FIFO
 * ARGUMENTS:
 *     ControllerInfo: Info structure for the FDC we're testing
 * RETURNS:
 *     TRUE if the controller can read a byte right now
 *     FALSE otherwise
 * NOTES:
 *     - it is necessary to check both that the FIFO is set to "inbound" 
 *       and that the "ready for i/o" bit is set.
 */
{
  UCHAR Status = READ_PORT_UCHAR(ControllerInfo->BaseAddress + MAIN_STATUS_REGISTER);
	
  if(!(Status & MSR_IO_DIRECTION)) /* Read = 1 */
      return FALSE;

  if(!(Status & MSR_DATA_REG_READY_FOR_IO))
      return FALSE;

  return TRUE;
}


static NTSTATUS NTAPI Send_Byte(PCONTROLLER_INFO ControllerInfo, 
                                UCHAR Byte)
/*
 * FUNCTION: Send a byte from the host to the controller's FIFO
 * ARGUMENTS:
 *     ControllerInfo: Info structure for the controller we're writing to
 *     Offset: Offset over the controller's base address that we're writing to 
 *     Byte: Byte to write to the bus
 * RETURNS:
 *     STATUS_SUCCESS if the byte was written successfully
 *     STATUS_UNSUCCESSFUL if not
 * NOTES:
 *     - Function designed after flowchart in intel datasheet
 *     - 250us max delay.  Note that this is exactly 5 times longer
 *       than Microsoft recommends stalling the processor
 *     - Remember that we can be interrupted here, so this might
 *       take much more wall clock time than 250us
 *     - PAGED_CODE, because we spin for more than the Microsoft-recommended
 *       maximum.
 *     - This function is necessary because sometimes the FIFO reacts slowly
 *       and isn't yet ready to read or write the next byte
 * FIXME: time interval here and in Get_Byte
 */
{
  LARGE_INTEGER StartingTickCount;
  LARGE_INTEGER CurrentTickCount;
  PUCHAR Address;

  PAGED_CODE();

  Address = ControllerInfo->BaseAddress + FIFO;

  if(!TimeIncrement)
    TimeIncrement = KeQueryTimeIncrement();

  StartingTickCount.QuadPart = 0;

  for(;;)
    {
      if(!ReadyForWrite(ControllerInfo))
	{
	  ULONG64 ElapsedTicks;
	  ULONG64 TimeUnits;

	  /* If this is the first time through... */
	  if(!StartingTickCount.QuadPart)
	    {
              KeQueryTickCount(&StartingTickCount);
	      continue;
	    }

	  /* Otherwise, only do this for 250 us == 2500 100ns units */
	  KeQueryTickCount(&CurrentTickCount);
	  ElapsedTicks = CurrentTickCount.QuadPart - StartingTickCount.QuadPart;
	  TimeUnits = ElapsedTicks * TimeIncrement;

	  if(TimeUnits > 25000000)
	    break;

          continue;
	}
			
      WRITE_PORT_UCHAR(Address, Byte);
      return STATUS_SUCCESS;
    }

  KdPrint(("floppy: Send_Byte: timed out trying to write\n"));
  HwDumpRegisters(ControllerInfo);
  return STATUS_UNSUCCESSFUL;
}


static NTSTATUS NTAPI Get_Byte(PCONTROLLER_INFO ControllerInfo, 
                               PUCHAR Byte)
/*
 * FUNCTION: Read a byte from the controller to the host
 * ARGUMENTS:
 *     ControllerInfo: Info structure for the controller we're reading from
 *     Offset: Offset over the controller's base address  that we're reading from
 *     Byte: Byte to read from the bus
 * RETURNS:
 *     STATUS_SUCCESS if the byte was read successfully
 *     STATUS_UNSUCCESSFUL if not
 * NOTES:
 *     - Function designed after flowchart in intel datasheet
 *     - 250us max delay.  Note that this is exactly 5 times longer
 *       than Microsoft recommends stalling the processor
 *     - Remember that we can be interrupted here, so this might
 *       take much more wall clock time than 250us
 *     - PAGED_CODE because we spin for longer than Microsoft recommends
 */
{
  LARGE_INTEGER StartingTickCount;
  LARGE_INTEGER CurrentTickCount;
  PUCHAR Address;

  PAGED_CODE();

  Address = ControllerInfo->BaseAddress + FIFO;

  if(!TimeIncrement)
    TimeIncrement = KeQueryTimeIncrement();

  StartingTickCount.QuadPart = 0;

  for(;;)
    {
      if(!ReadyForRead(ControllerInfo))
	{
	  ULONG64 ElapsedTicks;
	  ULONG64 TimeUnits;

	  /* if this is the first time through, start the timer */
          if(!StartingTickCount.QuadPart)
	    {
              KeQueryTickCount(&StartingTickCount);
	      continue;
	    }

	  /* Otherwise, only do this for 250 us == 2500 100ns units */
	  KeQueryTickCount(&CurrentTickCount);
	  ElapsedTicks = CurrentTickCount.QuadPart - StartingTickCount.QuadPart;
	  TimeUnits = ElapsedTicks * TimeIncrement;

	  if(TimeUnits > 25000000)
	    break;

          continue;
	}
			
      *Byte = READ_PORT_UCHAR(Address);

      return STATUS_SUCCESS;
    }

  KdPrint(("floppy: Get_Byte: timed out trying to read\n"));
  HwDumpRegisters(ControllerInfo);
  return STATUS_UNSUCCESSFUL;
}


NTSTATUS NTAPI HwSetDataRate(PCONTROLLER_INFO ControllerInfo, 
                             UCHAR DataRate)
/*
 * FUNCTION: Set the data rte on a controller
 * ARGUMENTS:
 *     ControllerInfo: Controller whose rate is being set
 *     DataRate: Data rate code to set the controller to
 * RETURNS:
 *     STATUS_SUCCESS
 */
{
  KdPrint(("floppy: HwSetDataRate called; writing rate code 0x%x to offset 0x%x\n", DataRate, DATA_RATE_SELECT_REGISTER));

  WRITE_PORT_UCHAR(ControllerInfo->BaseAddress + DATA_RATE_SELECT_REGISTER, DataRate);

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwTurnOffMotor(PCONTROLLER_INFO ControllerInfo)
/*
 * FUNCTION: Turn off all motors
 * ARGUMENTS:
 *     DriveInfo: drive to turn off
 * RETURNS:
 *     STATUS_SUCCESS if the motor is successfully turned off
 * NOTES:
 *     - Don't call this routine directly unless you've thought about it
 *       and read the source to StartMotor() and StopMotor().
 *     - Called at DISPATCH_LEVEL
 */
{
  KdPrint(("floppy: HwTurnOffMotor: writing byte 0x%x to offset 0x%x\n", DOR_FDC_ENABLE|DOR_DMA_IO_INTERFACE_ENABLE, DIGITAL_OUTPUT_REGISTER));

  WRITE_PORT_UCHAR(ControllerInfo->BaseAddress + DIGITAL_OUTPUT_REGISTER, DOR_FDC_ENABLE|DOR_DMA_IO_INTERFACE_ENABLE);

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwTurnOnMotor(PDRIVE_INFO DriveInfo) 
/*
 * FUNCTION: Turn on the motor on the selected drive
 * ARGUMENTS:
 *     DriveInfo: drive to turn on
 * RETURNS:
 *     STATUS_SUCCESS if the motor is successfully turned on
 *     STATUS_UNSUCCESSFUL otherwise
 * NOTES:
 *     - Doesn't interrupt
 *     - Currently cannot fail
 */
{
  PCONTROLLER_INFO ControllerInfo = DriveInfo->ControllerInfo;
  UCHAR Unit = DriveInfo->UnitNumber;
  UCHAR Buffer;

  PAGED_CODE();

  /* turn on motor */
  Buffer = Unit;

  Buffer |= DOR_FDC_ENABLE;
  Buffer |= DOR_DMA_IO_INTERFACE_ENABLE;
	
  if(Unit == 0)
    Buffer |= DOR_FLOPPY_MOTOR_ON_A;
  else if (Unit == 1)
    Buffer |= DOR_FLOPPY_MOTOR_ON_B;
  else if (Unit == 2)
    Buffer |= DOR_FLOPPY_MOTOR_ON_C;
  else if (Unit == 3)
    Buffer |= DOR_FLOPPY_MOTOR_ON_D;

  KdPrint(("floppy: HwTurnOnMotor: writing byte 0x%x to offset 0x%x\n", Buffer, DIGITAL_OUTPUT_REGISTER));
  WRITE_PORT_UCHAR(ControllerInfo->BaseAddress + DIGITAL_OUTPUT_REGISTER, Buffer);

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwSenseDriveStatus(PDRIVE_INFO DriveInfo)
/*
 * FUNCTION: Start a sense status command
 * ARGUMENTS:
 *     DriveInfo: Drive to inquire about
 * RETURNS:
 *     STATUS_SUCCESS if the command is successfully queued to the controller
 *     STATUS_UNSUCCESSFUL if not
 * NOTES:
 *     - Generates an interrupt
 *     - hard-wired to head 0
 */
{
  UCHAR Buffer[2];
  int i;

  PAGED_CODE();

  KdPrint(("floppy: HwSenseDriveStatus called\n"));

  Buffer[0] = COMMAND_SENSE_DRIVE_STATUS;
  Buffer[1] = DriveInfo->UnitNumber; /* hard-wired to head 0 for now */

  for(i = 0; i < 2; i++)
    if(Send_Byte(DriveInfo->ControllerInfo, Buffer[i]) != STATUS_SUCCESS)
      {
	KdPrint(("floppy: HwSenseDriveStatus: failed to write FIFO\n"));
        return STATUS_UNSUCCESSFUL;
      }

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwReadWriteData(PCONTROLLER_INFO ControllerInfo, 
                               BOOLEAN Read,
                               UCHAR Unit, 
                               UCHAR Cylinder, 
                               UCHAR Head, 
                               UCHAR Sector,
                               UCHAR BytesPerSector,
                               UCHAR EndOfTrack,
                               UCHAR Gap3Length,
                               UCHAR DataLength)
/*
 * FUNCTION: Read or write data to the drive
 * ARGUMENTS:
 *     ControllerInfo: controller to target the read/write request to
 *     Read: TRUE if the device should be read; FALSE if written
 *     Unit: Drive number to target
 *     Cylinder: cylinder to start the read on
 *     Head: head to start the read on
 *     Sector: sector to start the read on (1-based!)
 *     BytesPerSector: sector size constant (hardware.h)
 *     EndOfTrack: Marks the last sector number to read/write on the track
 *     Gap3Length: Gap length for the operation
 *     DataLength: Bytes to read, *unless* BytesPerSector is specified
 * RETURNS:
 *     STATUS_SUCCESS if the operation was successfully queued to the controller
 *     STATUS_UNSUCCESSFUL otherwise
 * NOTES:
 *     - Generates an interrupt
 */
{
  UCHAR Buffer[9];
  int i;

  PAGED_CODE();

  /* Shouldn't be using DataLength in this driver */
  ASSERT(DataLength == 0xff);

  /* Build the command to send */
  if(Read)
    Buffer[0] = COMMAND_READ_DATA; 
  else
    Buffer[0] = COMMAND_WRITE_DATA; 

  Buffer[0] |= READ_DATA_MFM | READ_DATA_MT;

  Buffer[1] = (Head << COMMAND_HEAD_NUMBER_SHIFT) | Unit;
  Buffer[2] = Cylinder;
  Buffer[3] = Head;
  Buffer[4] = Sector;
  Buffer[5] = BytesPerSector;
  Buffer[6] = EndOfTrack;
  Buffer[7] = Gap3Length;
  Buffer[8] = DataLength;

  /* Send the command */
  for(i = 0; i < 9; i++)
    {
        KdPrint(("floppy: HwReadWriteData: Sending a command byte to the FIFO: 0x%x\n", Buffer[i]));

	if(Send_Byte(ControllerInfo, Buffer[i]) != STATUS_SUCCESS)
	  {
	    KdPrint(("HwReadWriteData: Unable to write to the FIFO\n"));
	    return STATUS_UNSUCCESSFUL;
	  }
    }

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwRecalibrateResult(PCONTROLLER_INFO ControllerInfo)
/*
 * FUNCTION: Get the result of a recalibrate command
 * ARGUMENTS:
 *     ControllerInfo: controller to query
 * RETURNS:
 *     STATUS_SUCCESS if the recalibratewas a success
 *     STATUS_UNSUCCESSFUL otherwise
 * NOTES:
 *     - This function tests the error conditions itself, and boils the
 *       whole thing down to a single SUCCESS or FAILURE result
 *     - Called post-interrupt; does not interrupt
 * TODO 
 *     - perhaps handle more status
 */
{
  UCHAR Buffer[2];
  int i;

  PAGED_CODE();

  if(Send_Byte(ControllerInfo, COMMAND_SENSE_INTERRUPT_STATUS) != STATUS_SUCCESS)
    {
      KdPrint(("floppy: HwRecalibrateResult: Unable to write the controller\n"));
      return STATUS_UNSUCCESSFUL;
    }

  for(i = 0; i < 2; i++)
    if(Get_Byte(ControllerInfo, &Buffer[i]) != STATUS_SUCCESS)
      {
	KdPrint(("floppy: HwRecalibrateResult: unable to read FIFO\n"));
        return STATUS_UNSUCCESSFUL;
      }

  /* Validate  that it did what we told it to */ 
  KdPrint(("floppy: HwRecalibrateResult results: ST0: 0x%x PCN: 0x%x\n", Buffer[0], Buffer[1]));

  /*
   * Buffer[0] = ST0
   * Buffer[1] = PCN
   */

  /* Is the PCN 0? */
  if(Buffer[1] != 0)
    {
      KdPrint(("floppy: HwRecalibrateResult: PCN not 0\n"));
      return STATUS_UNSUCCESSFUL;
    }

  /* test seek complete */
  if((Buffer[0] & SR0_SEEK_COMPLETE) != SR0_SEEK_COMPLETE)
    {
      KdPrint(("floppy: HwRecalibrateResult: Failed to complete the seek\n"));
      return STATUS_UNSUCCESSFUL;
    }

  /* Is the equipment check flag set?  Could be no disk in drive... */
  if((Buffer[0] & SR0_EQUIPMENT_CHECK) == SR0_EQUIPMENT_CHECK)
      KdPrint(("floppy: HwRecalibrateResult: Seeked to track 0 successfully, but EC is set; returning STATUS_SUCCESS anyway\n"));

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwReadWriteResult(PCONTROLLER_INFO ControllerInfo)
/*
 * FUNCTION: Get the result of a read or write from the controller
 * ARGUMENTS:
 *     ControllerInfo: controller to query
 * RETURNS:
 *     STATUS_SUCCESS if the read/write was a success
 *     STATUS_UNSUCCESSFUL otherwise
 * NOTES:
 *     - This function tests the error conditions itself, and boils the
 *       whole thing down to a single SUCCESS or FAILURE result
 *     - Called post-interrupt; does not interrupt
 * TODO:
 *     - perhaps handle more status
 */
{
  UCHAR Buffer[7];
  int i;

  PAGED_CODE();

  for(i = 0; i < 7; i++)
    if(Get_Byte(ControllerInfo, &Buffer[i]) != STATUS_SUCCESS)
      {
	KdPrint(("floppy: HwReadWriteResult: unable to read fifo\n"));
        return STATUS_UNSUCCESSFUL;
      }

  /* Validate  that it did what we told it to */ 
  KdPrint(("floppy: HwReadWriteResult results: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", Buffer[0], Buffer[1], Buffer[2], Buffer[3],
	   Buffer[4], Buffer[5], Buffer[6]));

  /* Last command successful? */
  if((Buffer[0] & SR0_LAST_COMMAND_STATUS) != SR0_LCS_SUCCESS)
    return STATUS_UNSUCCESSFUL;

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwRecalibrate(PDRIVE_INFO DriveInfo)
/*
 * FUNCTION: Start a recalibration of a drive
 * ARGUMENTS:
 *     DriveInfo: Drive to recalibrate
 * RETURNS:
 *     STATUS_SUCCESS if the command was successfully queued to the controller
 *     STATUS_UNSUCCESSFUL otherwise
 * NOTES:
 *     - Generates an interrupt
 */
{
  PCONTROLLER_INFO ControllerInfo = DriveInfo->ControllerInfo;
  UCHAR Unit = DriveInfo->UnitNumber;
  UCHAR Buffer[2];
  int i;

  KdPrint(("floppy: HwRecalibrate called\n"));

  PAGED_CODE();

  Buffer[0] = COMMAND_RECALIBRATE;
  Buffer[1] = Unit;

  for(i = 0; i < 2; i++)
    if(Send_Byte(ControllerInfo, Buffer[i]) != STATUS_SUCCESS)
      {
	KdPrint(("floppy: HwRecalibrate: unable to write FIFO\n"));
        return STATUS_UNSUCCESSFUL;
      }

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwSenseInterruptStatus(PCONTROLLER_INFO ControllerInfo)
/*
 * FUNCTION: Send a sense interrupt status command to a controller
 * ARGUMENTS:
 *     ControllerInfo: controller to queue the command to
 * RETURNS:
 *     STATUS_SUCCESS if the command is queued successfully
 *     STATUS_UNSUCCESSFUL if not
 */
{
  UCHAR Buffer[2];
  int i;

  PAGED_CODE();

  if(Send_Byte(ControllerInfo, COMMAND_SENSE_INTERRUPT_STATUS) != STATUS_SUCCESS)
    {
      KdPrint(("floppy: HwSenseInterruptStatus: failed to write controller\n"));
      return STATUS_UNSUCCESSFUL;
    }

  for(i = 0; i  < 2; i++)
    {
      if(Get_Byte(ControllerInfo, &Buffer[i]) != STATUS_SUCCESS)
	{
	  KdPrint(("floppy: HwSenseInterruptStatus: failed to read controller\n"));
	  return STATUS_UNSUCCESSFUL;
	}
    }

  KdPrint(("floppy: HwSenseInterruptStatus returned 0x%x 0x%x\n", Buffer[0], Buffer[1]));

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwReadId(PDRIVE_INFO DriveInfo, UCHAR Head)
/*
 * FUNCTION: Issue a read id command to the drive
 * ARGUMENTS:
 *     DriveInfo: Drive to read id from
 *     Head: Head to read the ID from
 * RETURNS:
 *     STATUS_SUCCESS if the command is queued
 *     STATUS_UNSUCCESSFUL otherwise
 * NOTES:
 *     - Generates an interrupt
 */
{
  UCHAR Buffer[2];
  int i;

  KdPrint(("floppy: HwReadId called\n"));

  PAGED_CODE();

  Buffer[0] = COMMAND_READ_ID | READ_ID_MFM;
  Buffer[1] = (Head << COMMAND_HEAD_NUMBER_SHIFT) | DriveInfo->UnitNumber;

  for(i = 0; i < 2; i++)
    if(Send_Byte(DriveInfo->ControllerInfo, Buffer[i]) != STATUS_SUCCESS)
      {
	KdPrint(("floppy: HwReadId: unable to send bytes to fifo\n"));
        return STATUS_UNSUCCESSFUL;
      }

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwFormatTrack(PCONTROLLER_INFO ControllerInfo, 
                             UCHAR Unit,
                             UCHAR Head, 
                             UCHAR BytesPerSector,
                             UCHAR SectorsPerTrack,
                             UCHAR Gap3Length,
                             UCHAR FillerPattern)
/*
 * FUNCTION: Format a track
 * ARGUMENTS:
 *     ControllerInfo: controller to target with the request
 *     Unit: drive to format on
 *     Head: head to format on
 *     BytesPerSector: constant from hardware.h to select density
 *     SectorsPerTrack: sectors per track
 *     Gap3Length: gap length to use during format
 *     FillerPattern: pattern to write into the data portion of sectors
 * RETURNS:
 *     STATUS_SUCCESS if the command is successfully queued
 *     STATUS_UNSUCCESSFUL otherwise
 */
{
  UCHAR Buffer[6];
  int i;

  KdPrint(("floppy: HwFormatTrack called\n"));

  PAGED_CODE();

  Buffer[0] = COMMAND_FORMAT_TRACK;
  Buffer[1] = (Head << COMMAND_HEAD_NUMBER_SHIFT) | Unit;
  Buffer[2] = BytesPerSector;
  Buffer[3] = SectorsPerTrack;
  Buffer[4] = Gap3Length;
  Buffer[5] = FillerPattern;

  for(i = 0; i < 6; i++)
    if(Send_Byte(ControllerInfo, Buffer[i]) != STATUS_SUCCESS)
      {
	KdPrint(("floppy: HwFormatTrack: unable to send bytes to floppy\n"));
        return STATUS_UNSUCCESSFUL;
      }

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwSeek(PDRIVE_INFO DriveInfo,
                      UCHAR Cylinder) 
/*
 * FUNCTION: Seek the heads to a particular cylinder
 * ARGUMENTS:
 *     DriveInfo: Drive to seek
 *     Cylinder: cylinder to move to
 * RETURNS:
 *     STATUS_SUCCESS if the command is successfully sent
 *     STATUS_UNSUCCESSFUL otherwise
 * NOTES:
 *     - Generates an interrupt
 */
{
  LARGE_INTEGER Delay;
  UCHAR Buffer[3];
  int i;

  KdPrint(("floppy: HwSeek called for cyl 0x%x\n", Cylinder));

  PAGED_CODE();

  Buffer[0] = COMMAND_SEEK;
  Buffer[1] = DriveInfo->UnitNumber;
  Buffer[2] = Cylinder;

  for(i = 0; i < 3; i++)
    if(Send_Byte(DriveInfo->ControllerInfo, Buffer[i]) != STATUS_SUCCESS)
      {
	KdPrint(("floppy: HwSeek: failed to write fifo\n"));
        return STATUS_UNSUCCESSFUL;
      }

  /* Wait for the head to settle */
  Delay.QuadPart = 10 * 1000;
  Delay.QuadPart *= -1;
  Delay.QuadPart *= DriveInfo->FloppyDeviceData.HeadSettleTime;

  KeDelayExecutionThread(KernelMode, FALSE, &Delay);

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwConfigure(PCONTROLLER_INFO ControllerInfo, 
                           BOOLEAN EIS,
			   BOOLEAN EFIFO,
			   BOOLEAN POLL,
			   UCHAR FIFOTHR,
			   UCHAR PRETRK) 
/*
 * FUNCTION: Sends configuration to the drive
 * ARGUMENTS:
 *     ControllerInfo: controller to target with the request
 *     EIS: Enable implied seek
 *     EFIFO: Enable advanced fifo
 *     POLL: Enable polling
 *     FIFOTHR: fifo threshold
 *     PRETRK: precomp (see intel datasheet)
 * RETURNS:
 *     STATUS_SUCCESS if the command is successfully sent
 *     STATUS_UNSUCCESSFUL otherwise
 * NOTES:
 *     - No interrupt
 */
{
  UCHAR Buffer[4];
  int i;

  KdPrint(("floppy: HwConfigure called\n"));

  PAGED_CODE();

  Buffer[0] = COMMAND_CONFIGURE;
  Buffer[1] = 0;
  Buffer[2] = (EIS * CONFIGURE_EIS) + (EFIFO * CONFIGURE_EFIFO) + (POLL * CONFIGURE_POLL) + (FIFOTHR);
  Buffer[3] = PRETRK;

  for(i = 0; i < 4; i++)
    if(Send_Byte(ControllerInfo, Buffer[i]) != STATUS_SUCCESS)
      {
	KdPrint(("floppy: HwConfigure: failed to write the fifo\n"));
        return STATUS_UNSUCCESSFUL;
      }

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwGetVersion(PCONTROLLER_INFO ControllerInfo)
/*
 * FUNCTION: Gets the version of the controller
 * ARGUMENTS:
 *     ControllerInfo: controller to target with the request
 *     ConfigValue: Configuration value to send to the drive (see header)
 * RETURNS:
 *     Version number returned by the command, or
 *     0 on failure
 * NOTE:
 *     - This command doesn't interrupt, so we go right to reading after
 *       we issue the command
 */
{
  UCHAR Buffer;

  PAGED_CODE();

  if(Send_Byte(ControllerInfo, COMMAND_VERSION) != STATUS_SUCCESS)
    {
      KdPrint(("floppy: HwGetVersion: unable to write fifo\n"));
      return STATUS_UNSUCCESSFUL;
    }

  if(Get_Byte(ControllerInfo, &Buffer) != STATUS_SUCCESS)
    {
      KdPrint(("floppy: HwGetVersion: unable to write fifo\n"));
      return STATUS_UNSUCCESSFUL;
    }

  KdPrint(("floppy: HwGetVersion returning version 0x%x\n", Buffer));

  return Buffer;
}

NTSTATUS NTAPI HwDiskChanged(PDRIVE_INFO DriveInfo, 
                             PBOOLEAN DiskChanged)
/*
 * FUNCTION: Detect whether the hardware has sensed a disk change
 * ARGUMENTS:
 *     DriveInfo: pointer to the drive that we are to check
 *     DiskChanged: boolean that is set with whether or not the controller thinks there has been a disk change
 * RETURNS:
 *     STATUS_SUCCESS if the drive is successfully queried
 * NOTES:
 *     - Does not interrupt.
 *     - Guessing a bit at the Model30 stuff
 */
{
  UCHAR Buffer;
  PCONTROLLER_INFO ControllerInfo = (PCONTROLLER_INFO) DriveInfo->ControllerInfo;

  Buffer = READ_PORT_UCHAR(ControllerInfo->BaseAddress + DIGITAL_INPUT_REGISTER);

  KdPrint(("floppy: HwDiskChanged: read 0x%x from DIR\n", Buffer));

  if(ControllerInfo->Model30)
    {
      if(!(Buffer & DIR_DISKETTE_CHANGE))
	{
	  KdPrint(("floppy: HdDiskChanged - Model30 - returning TRUE\n"));
	  *DiskChanged = TRUE;
	}
      else
	{
	  KdPrint(("floppy: HdDiskChanged - Model30 - returning FALSE\n"));
	  *DiskChanged = FALSE;
	}
    }
  else
    {
      if(Buffer & DIR_DISKETTE_CHANGE)
	{
	  KdPrint(("floppy: HdDiskChanged - PS2 - returning TRUE\n"));
	  *DiskChanged = TRUE;
	}
      else
	{
	  KdPrint(("floppy: HdDiskChanged - PS2 - returning FALSE\n"));
	  *DiskChanged = FALSE;
	}
    }

  return STATUS_SUCCESS;
}

NTSTATUS NTAPI HwSenseDriveStatusResult(PCONTROLLER_INFO ControllerInfo, 
                                        PUCHAR Status)
/*
 * FUNCTION: Get the result of a sense drive status command
 * ARGUMENTS:
 *     ControllerInfo: controller to query
 *     Status: Status from the drive sense command
 * RETURNS:
 *     STATUS_SUCCESS if we can successfully read the status
 *     STATUS_UNSUCCESSFUL otherwise
 * NOTES:
 *     - Called post-interrupt; does not interrupt
 */
{
  PAGED_CODE();

  if(Get_Byte(ControllerInfo, Status) != STATUS_SUCCESS)
    {
      KdPrint(("floppy: HwSenseDriveStatus: unable to read fifo\n"));
      return STATUS_UNSUCCESSFUL;
    }

  KdPrint(("floppy: HwSenseDriveStatusResult: ST3: 0x%x\n", *Status));

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwReadIdResult(PCONTROLLER_INFO ControllerInfo,
                              PUCHAR CurCylinder,
                              PUCHAR CurHead)
/*
 * FUNCTION: Get the result of a read id command
 * ARGUMENTS:
 *     ControllerInfo: controller to query
 *     CurCylinder: Returns the cylinder that we're at
 *     CurHead: Returns the head that we're at
 * RETURNS:
 *     STATUS_SUCCESS if the read id was a success
 *     STATUS_UNSUCCESSFUL otherwise
 * NOTES:
 *     - This function tests the error conditions itself, and boils the
 *       whole thing down to a single SUCCESS or FAILURE result
 *     - Called post-interrupt; does not interrupt
 * TODO 
 *     - perhaps handle more status
 */
{
  UCHAR Buffer[7] = {0,0,0,0,0,0,0};
  int i;

  PAGED_CODE();

  for(i = 0; i < 7; i++)
    if(Get_Byte(ControllerInfo, &Buffer[i]) != STATUS_SUCCESS)
      {
	KdPrint(("floppy: ReadIdResult(): can't read from the controller\n"));
        return STATUS_UNSUCCESSFUL;
      }

  /* Validate  that it did what we told it to */ 
  KdPrint(("floppy: ReadId results: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", Buffer[0], Buffer[1], Buffer[2], Buffer[3],
	   Buffer[4], Buffer[5], Buffer[6]));

  /* Last command successful? */
  if((Buffer[0] & SR0_LAST_COMMAND_STATUS) != SR0_LCS_SUCCESS)
    {
      KdPrint(("floppy: ReadId didn't return last command success\n"));
      return STATUS_UNSUCCESSFUL;
    }

  /* ID mark found? */
  if(Buffer[1] & SR1_CANNOT_FIND_ID_ADDRESS)
    {
      KdPrint(("floppy: ReadId didn't find an address mark\n"));
      return STATUS_UNSUCCESSFUL;
    }

  if(CurCylinder)
    *CurCylinder = Buffer[3];

  if(CurHead)
    *CurHead = Buffer[4];

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwSpecify(PCONTROLLER_INFO ControllerInfo,
                         UCHAR HeadLoadTime,
                         UCHAR HeadUnloadTime,
                         UCHAR StepRateTime,
                         BOOLEAN NonDma)
/*
 * FUNCTION: Set up timing and DMA mode for the controller
 * ARGUMENTS:
 *     ControllerInfo: Controller to set up
 *     HeadLoadTime: Head load time (see data sheet for details)
 *     HeadUnloadTime: Head unload time
 *     StepRateTime: Step rate time
 *     NonDma: TRUE to disable DMA mode
 * RETURNS:
 *     STATUS_SUCCESS if the contrller is successfully programmed
 *     STATUS_UNSUCCESSFUL if not
 * NOTES:
 *     - Does not interrupt
 *
 * TODO: Figure out timings
 */
{
  UCHAR Buffer[3];
  int i;

  Buffer[0] = COMMAND_SPECIFY;
  /*
  Buffer[1] = (StepRateTime << 4) + HeadUnloadTime;
  Buffer[2] = (HeadLoadTime << 1) + (NonDma ? 1 : 0);
  */
  Buffer[1] = 0xd1;
  Buffer[2] = 0x2;

  //KdPrint(("HwSpecify: sending 0x%x 0x%x 0x%x to FIFO\n", Buffer[0], Buffer[1], Buffer[2]));
  KdPrint(("FLOPPY: HWSPECIFY: FIXME - sending 0x3 0xd1 0x2 to FIFO\n"));

  for(i = 0; i < 3; i++)
    if(Send_Byte(ControllerInfo, Buffer[i]) != STATUS_SUCCESS)
      {
	KdPrint(("floppy: HwSpecify: unable to write to controller\n"));
        return STATUS_UNSUCCESSFUL;
      }

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwReset(PCONTROLLER_INFO ControllerInfo)
/*
 * FUNCTION: Reset the controller
 * ARGUMENTS:
 *     ControllerInfo: controller to reset
 * RETURNS:
 *     STATUS_SUCCESS in all cases
 * NOTES:
 *     - Generates an interrupt that must be serviced four times (one per drive)
 */
{
  KdPrint(("floppy: HwReset called\n"));

  /* Write the reset bit in the DRSR */
  WRITE_PORT_UCHAR(ControllerInfo->BaseAddress + DATA_RATE_SELECT_REGISTER, DRSR_SW_RESET);

  /* Check for the reset bit in the DOR and set it if necessary (see Intel doc) */
  if(!(READ_PORT_UCHAR(ControllerInfo->BaseAddress + DIGITAL_OUTPUT_REGISTER) & DOR_RESET))
    {
      HwDumpRegisters(ControllerInfo);
      KdPrint(("floppy: HwReset: Setting Enable bit\n"));
      WRITE_PORT_UCHAR(ControllerInfo->BaseAddress + DIGITAL_OUTPUT_REGISTER, DOR_DMA_IO_INTERFACE_ENABLE|DOR_RESET);
      HwDumpRegisters(ControllerInfo);

      if(!(READ_PORT_UCHAR(ControllerInfo->BaseAddress + DIGITAL_OUTPUT_REGISTER) & DOR_RESET))
	{
	  KdPrint(("floppy: HwReset: failed to set the DOR enable bit!\n"));
          HwDumpRegisters(ControllerInfo);
	  return STATUS_UNSUCCESSFUL;
	}
    }

  return STATUS_SUCCESS;
}


NTSTATUS NTAPI HwPowerOff(PCONTROLLER_INFO ControllerInfo)
/*
 * FUNCTION: Power down a controller
 * ARGUMENTS:
 *     ControllerInfo: Controller to power down
 * RETURNS:
 *     STATUS_SUCCESS
 * NOTES:
 *     - Wake up with a hardware reset
 */
{
  KdPrint(("floppy: HwPowerOff called on controller 0x%x\n", ControllerInfo));

  WRITE_PORT_UCHAR(ControllerInfo->BaseAddress + DATA_RATE_SELECT_REGISTER, DRSR_POWER_DOWN);

  return STATUS_SUCCESS;
}

VOID NTAPI HwDumpRegisters(PCONTROLLER_INFO ControllerInfo)
/*
 * FUNCTION: Dump all readable registers from the floppy controller
 * ARGUMENTS:
 *     ControllerInfo: Controller to dump registers from
 */
{
  UNREFERENCED_PARAMETER(ControllerInfo);

  KdPrint(("floppy: STATUS: "));
  KdPrint(("STATUS_REGISTER_A = 0x%x ", READ_PORT_UCHAR(ControllerInfo->BaseAddress + STATUS_REGISTER_A)));
  KdPrint(("STATUS_REGISTER_B = 0x%x ", READ_PORT_UCHAR(ControllerInfo->BaseAddress + STATUS_REGISTER_B)));
  KdPrint(("DIGITAL_OUTPUT_REGISTER = 0x%x ", READ_PORT_UCHAR(ControllerInfo->BaseAddress + DIGITAL_OUTPUT_REGISTER)));
  KdPrint(("MAIN_STATUS_REGISTER =0x%x ", READ_PORT_UCHAR(ControllerInfo->BaseAddress + MAIN_STATUS_REGISTER)));
  KdPrint(("DIGITAL_INPUT_REGISTER = 0x%x\n", READ_PORT_UCHAR(ControllerInfo->BaseAddress + DIGITAL_INPUT_REGISTER)));
}

