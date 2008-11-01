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
 * FILE:            readwrite.c
 * PURPOSE:         Read/Write handler routines
 * PROGRAMMER:      Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *                  15-Feb-2004 vizzini - Created
 * NOTES:
 *
 * READ/WRITE PROCESS
 *
 * This process is extracted from the Intel datasheet for the floppy controller.
 *
 * - Turn on the motor and set turnoff time
 * - Program the drive's data rate
 * - Seek
 * - Read ID
 * - Set up DMA
 * - Send read/write command to FDC
 * - Read result bytes
 *
 * This is mostly implemented in one big function, which watis on the SynchEvent
 * as many times as necessary to get through the process.  See ReadWritePassive() for
 * more details.
 *
 * NOTES:
 *     - Currently doesn't support partial-sector transfers, which is really just a failing
 *       of RWComputeCHS.  I've never seen Windows send a partial-sector request, though, so
 *       this may not be a bad thing.  Should be looked into, regardless.
 *
 * TODO: Break up ReadWritePassive and handle errors better
 * TODO: Figure out data rate issues
 * TODO: Media type detection
 * TODO: Figure out perf issue - waiting after call to read/write for about a second each time
 * TODO: Figure out specify timings
 */

#include <ntddk.h>
#include <debug.h>

#include "floppy.h"
#include "csqrtns.h"
#include "hardware.h"
#include "readwrite.h"


static IO_ALLOCATION_ACTION NTAPI MapRegisterCallback(PDEVICE_OBJECT DeviceObject,
                                                      PIRP Irp,
                                                      PVOID MapRegisterBase,
                                                      PVOID Context)
/*
 * FUNCTION: Acquire map registers in prep for DMA
 * ARGUMENTS:
 *     DeviceObject: unused
 *     Irp: unused
 *     MapRegisterBase: returned to blocked thread via a member var
 *     Context: contains a pointer to the right ControllerInfo
 *     struct
 * RETURNS:
 *     KeepObject, because that's what the DDK says to do
 */
{
  PCONTROLLER_INFO ControllerInfo = (PCONTROLLER_INFO)Context;
  UNREFERENCED_PARAMETER(DeviceObject);
  UNREFERENCED_PARAMETER(Irp);

  TRACE_(FLOPPY, "MapRegisterCallback Called\n");

  ControllerInfo->MapRegisterBase = MapRegisterBase;
  KeSetEvent(&ControllerInfo->SynchEvent, 0, FALSE);

  return KeepObject;
}


NTSTATUS NTAPI ReadWrite(PDEVICE_OBJECT DeviceObject,
                         PIRP Irp)
/*
 * FUNCTION: Dispatch routine called for read or write IRPs
 * ARGUMENTS:
 * RETURNS:
 *     STATUS_PENDING if the IRP is queued
 *     STATUS_INVALID_PARAMETER if IRP is set up wrong
 * NOTES:
 *     - This function validates arguments to the IRP and then queues it
 *     - Note that this function is implicitly serialized by the queue logic.  Only
 *       one of these at a time is active in the system, no matter how many processors
 *       and threads we have.
 *     - This function stores the DeviceObject in the IRP's context area before dropping
 *       it onto the irp queue
 */
{
  TRACE_(FLOPPY, "ReadWrite called\n");

  ASSERT(DeviceObject);
  ASSERT(Irp);

  if(!Irp->MdlAddress)
    {
      WARN_(FLOPPY, "ReadWrite(): MDL not found in IRP - Completing with STATUS_INVALID_PARAMETER\n");
      Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return STATUS_INVALID_PARAMETER;
    }

  /*
   * Queue the irp to the thread.
   * The de-queue thread will look in DriverContext[0] for the Device Object.
   */
  Irp->Tail.Overlay.DriverContext[0] = DeviceObject;
  IoCsqInsertIrp(&Csq, Irp, NULL);

  return STATUS_PENDING;
}


static VOID NTAPI RWFreeAdapterChannel(PADAPTER_OBJECT AdapterObject)
/*
 * FUNCTION: Free the adapter DMA channel that we allocated
 * ARGUMENTS:
 *     AdapterObject: the object with the map registers to free
 * NOTES:
 *     - This function is primarily needed because IoFreeAdapterChannel wants to
 *       be called at DISPATCH_LEVEL
 */
{
  KIRQL Irql;

  ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

  KeRaiseIrql(DISPATCH_LEVEL, &Irql);
  IoFreeAdapterChannel(AdapterObject);
  KeLowerIrql(Irql);
}


static NTSTATUS NTAPI RWDetermineMediaType(PDRIVE_INFO DriveInfo)
/*
 * FUNCTION: Determine the media type of the disk in the drive and fill in the geometry
 * ARGUMENTS:
 *     DriveInfo: drive to look at
 * RETURNS:
 *     STATUS_SUCCESS if the media was recognized and the geometry struct was filled in
 *     STATUS_UNRECOGNIZED_MEDIA if not
 *     STATUS_UNSUCCESSFUL if the controller can't be talked to
 * NOTES:
 *     - Expects the motor to already be running
 *     - Currently only supports 1.44MB 3.5" disks
 *     - PAGED_CODE because it waits
 * TODO:
 *     - Support more disk types
 */
{
  UCHAR HeadLoadTime;
  UCHAR HeadUnloadTime;
  UCHAR StepRateTime;

  PAGED_CODE();

  TRACE_(FLOPPY, "RWDetermineMediaType called\n");

  /*
   * This algorithm assumes that a 1.44MB floppy is in the drive.  If it's not,
   * it works backwards until the read works.  Note that only 1.44 has been tested
   * at all.
   */

  do
    {
      int i;

      /* Program data rate */
      if(HwSetDataRate(DriveInfo->ControllerInfo, DRSR_DSEL_500KBPS) != STATUS_SUCCESS)
	{
	  WARN_(FLOPPY, "RWDetermineMediaType(): unable to set data rate\n");
	  return STATUS_UNSUCCESSFUL;
	}

      /* Specify */
      HeadLoadTime = SPECIFY_HLT_500K;
      HeadUnloadTime = SPECIFY_HUT_500K;
      StepRateTime = SPECIFY_SRT_500K;

      /* Don't disable DMA --> enable dma (dumb & confusing) */
      if(HwSpecify(DriveInfo->ControllerInfo, HeadLoadTime, HeadUnloadTime, StepRateTime, FALSE) != STATUS_SUCCESS)
	{
	  WARN_(FLOPPY, "RWDetermineMediaType(): specify failed\n");
	  return STATUS_UNSUCCESSFUL;
	}

      /* clear any spurious interrupts in preparation for recalibrate */
      KeClearEvent(&DriveInfo->ControllerInfo->SynchEvent);

      /* Recalibrate --> head over first track */
      for(i=0; i < 2; i++)
	{
	  NTSTATUS RecalStatus;

	  if(HwRecalibrate(DriveInfo) != STATUS_SUCCESS)
	    {
	      WARN_(FLOPPY, "RWDetermineMediaType(): Recalibrate failed\n");
	      return STATUS_UNSUCCESSFUL;
	    }

	  /* Wait for the recalibrate to finish */
	  WaitForControllerInterrupt(DriveInfo->ControllerInfo);

	  RecalStatus = HwRecalibrateResult(DriveInfo->ControllerInfo);

	  if(RecalStatus == STATUS_SUCCESS)
	    break;

	  if(i == 1) /* failed for 2nd time */
	    {
	      WARN_(FLOPPY, "RWDetermineMediaType(): RecalibrateResult failed\n");
	      return STATUS_UNSUCCESSFUL;
	    }
	}

      /* clear any spurious interrupts */
      KeClearEvent(&DriveInfo->ControllerInfo->SynchEvent);

      /* Try to read an ID */
      if(HwReadId(DriveInfo, 0) != STATUS_SUCCESS) /* read the first ID we find, from head 0 */
	{
	  WARN_(FLOPPY, "RWDetermineMediaType(): ReadId failed\n");
	  return STATUS_UNSUCCESSFUL; /* if we can't even write to the controller, it's hopeless */
	}

      /* Wait for the ReadID to finish */
      WaitForControllerInterrupt(DriveInfo->ControllerInfo);

      if(HwReadIdResult(DriveInfo->ControllerInfo, NULL, NULL) != STATUS_SUCCESS)
	{
	  WARN_(FLOPPY, "RWDetermineMediaType(): ReadIdResult failed; continuing\n");
	  continue;
	}

      /* Found the media; populate the geometry now */
      WARN_(FLOPPY, "Hardcoded media type!\n");
      INFO_(FLOPPY, "RWDetermineMediaType(): Found 1.44 media; returning success\n");
      DriveInfo->DiskGeometry.MediaType = GEOMETRY_144_MEDIATYPE;
      DriveInfo->DiskGeometry.Cylinders.QuadPart = GEOMETRY_144_CYLINDERS;
      DriveInfo->DiskGeometry.TracksPerCylinder = GEOMETRY_144_TRACKSPERCYLINDER;
      DriveInfo->DiskGeometry.SectorsPerTrack = GEOMETRY_144_SECTORSPERTRACK;
      DriveInfo->DiskGeometry.BytesPerSector = GEOMETRY_144_BYTESPERSECTOR;
      DriveInfo->BytesPerSectorCode = HW_512_BYTES_PER_SECTOR;
      return STATUS_SUCCESS;
    }
  while(FALSE);

  TRACE_(FLOPPY, "RWDetermineMediaType(): failed to find media\n");
  return STATUS_UNRECOGNIZED_MEDIA;
}


static NTSTATUS NTAPI RWSeekToCylinder(PDRIVE_INFO DriveInfo,
                                       UCHAR Cylinder)
/*
 * FUNCTION: Seek a particular drive to a particular track
 * ARGUMENTS:
 *     DriveInfo: Drive to seek
 *     Cylinder: track to seek to
 * RETURNS:
 *     STATUS_SUCCESS if the head was successfully seeked
 *     STATUS_UNSUCCESSFUL if not
 * NOTES:
 *     - PAGED_CODE because it blocks
 */
{
  UCHAR CurCylinder;

  PAGED_CODE();

  TRACE_(FLOPPY, "RWSeekToCylinder called drive 0x%p cylinder %d\n", DriveInfo, Cylinder);

  /* Clear any spurious interrupts */
  KeClearEvent(&DriveInfo->ControllerInfo->SynchEvent);

  /* queue seek command */
  if(HwSeek(DriveInfo, Cylinder) != STATUS_SUCCESS)
    {
      WARN_(FLOPPY, "RWSeekToTrack(): unable to seek\n");
      return STATUS_UNSUCCESSFUL;
    }

  WaitForControllerInterrupt(DriveInfo->ControllerInfo);

  if(HwSenseInterruptStatus(DriveInfo->ControllerInfo) != STATUS_SUCCESS)
    {
      WARN_(FLOPPY, "RWSeekToTrack(): unable to get seek results\n");
      return STATUS_UNSUCCESSFUL;
    }

  /* read ID mark from head 0 to verify */
  if(HwReadId(DriveInfo, 0) != STATUS_SUCCESS)
    {
      WARN_(FLOPPY, "RWSeekToTrack(): unable to queue ReadId\n");
      return STATUS_UNSUCCESSFUL;
    }

  WaitForControllerInterrupt(DriveInfo->ControllerInfo);

  if(HwReadIdResult(DriveInfo->ControllerInfo, &CurCylinder, NULL) != STATUS_SUCCESS)
    {
      WARN_(FLOPPY, "RWSeekToTrack(): unable to get ReadId result\n");
      return STATUS_UNSUCCESSFUL;
    }

  if(CurCylinder != Cylinder)
    {
      WARN_(FLOPPY, "RWSeekToTrack(): Seeek to track failed; current cylinder is 0x%x\n", CurCylinder);
      return STATUS_UNSUCCESSFUL;
    }

  INFO_(FLOPPY, "RWSeekToCylinder: returning successfully, now on cyl %d\n", Cylinder);

  return STATUS_SUCCESS;
}


static NTSTATUS NTAPI RWComputeCHS(PDRIVE_INFO IN  DriveInfo,
				   ULONG       IN  DiskByteOffset,
				   PUCHAR      OUT Cylinder,
				   PUCHAR      OUT Head,
				   PUCHAR      OUT Sector)
/*
 * FUNCTION: Compute the CHS from the absolute byte offset on disk
 * ARGUMENTS:
 *     DriveInfo: Drive to compute on
 *     DiskByteOffset: Absolute offset on disk of the starting byte
 *     Cylinder: Cylinder that the byte is on
 *     Head: Head that the byte is on
 *     Sector: Sector that the byte is on
 * RETURNS:
 *     STATUS_SUCCESS if CHS are determined correctly
 *     STATUS_UNSUCCESSFUL otherwise
 * NOTES:
 *     - Lots of ugly typecasts here
 *     - Sectors are 1-based!
 *     - This is really crummy code.  Please FIXME.
 */
{
  ULONG AbsoluteSector;
  UCHAR SectorsPerCylinder = (UCHAR)DriveInfo->DiskGeometry.SectorsPerTrack * (UCHAR)DriveInfo->DiskGeometry.TracksPerCylinder;

  TRACE_(FLOPPY, "RWComputeCHS: Called with offset 0x%x\n", DiskByteOffset);

  /* First calculate the 1-based "absolute sector" based on the byte offset */
  ASSERT(!(DiskByteOffset % DriveInfo->DiskGeometry.BytesPerSector));         /* FIXME: Only handle full sector transfers atm */

  /* AbsoluteSector is zero-based to make the math a little easier */
  AbsoluteSector = DiskByteOffset / DriveInfo->DiskGeometry.BytesPerSector;  /* Num full sectors */

  /* Cylinder number is floor(AbsoluteSector / SectorsPerCylinder) */
  *Cylinder =  (CHAR)(AbsoluteSector / SectorsPerCylinder);

  /* Head number is 0 if the sector within the cylinder < SectorsPerTrack; 1 otherwise */
  *Head =  AbsoluteSector % SectorsPerCylinder < DriveInfo->DiskGeometry.SectorsPerTrack ? 0 : 1;

  /*
   * Sector number is the sector within the cylinder if on head 0; that minus SectorsPerTrack if it's on head 1
   * (lots of casts to placate msvc).  1-based!
   */
  *Sector =  ((UCHAR)(AbsoluteSector % SectorsPerCylinder) + 1) - ((*Head) * (UCHAR)DriveInfo->DiskGeometry.SectorsPerTrack);

  INFO_(FLOPPY, "RWComputeCHS: offset 0x%x is c:0x%x h:0x%x s:0x%x\n", DiskByteOffset, *Cylinder, *Head, *Sector);

  /* Sanity checking */
  ASSERT(*Cylinder <= DriveInfo->DiskGeometry.Cylinders.QuadPart);
  ASSERT(*Head <= DriveInfo->DiskGeometry.TracksPerCylinder);
  ASSERT(*Sector <= DriveInfo->DiskGeometry.SectorsPerTrack);

  return STATUS_SUCCESS;
}


VOID NTAPI ReadWritePassive(PDRIVE_INFO DriveInfo,
                            PIRP Irp)
/*
 * FUNCTION: Handle the first phase of a read or write IRP
 * ARGUMENTS:
 *     DeviceObject: DeviceObject that is the target of the IRP
 *     Irp: IRP to process
 * RETURNS:
 *     STATUS_VERIFY_REQUIRED if the media has changed and we need the filesystems to re-synch
 *     STATUS_SUCCESS otherwise
 * NOTES:
 *     - Must be called at PASSIVE_LEVEL
 *     - This function is about 250 lines longer than I wanted it to be.  Sorry.
 *
 * DETAILS:
 *  This routine manages the whole process of servicing a read or write request.  It goes like this:
 *    1) Check the DO_VERIFY_VOLUME flag and return if it's set
 *    2) Check the disk change line and notify the OS if it's set and return
 *    3) Detect the media if we haven't already
 *    4) Set up DiskByteOffset, Length, and WriteToDevice parameters
 *    5) Get DMA map registers
 *    6) Then, in a loop for each track, until all bytes are transferred:
 *      a) Compute the current CHS to set the read/write head to
 *      b) Seek to that spot
 *      c) Compute the last sector to transfer on that track
 *      d) Map the transfer through DMA
 *      e) Send the read or write command to the controller
 *      f) Read the results of the command
 */
{
  PDEVICE_OBJECT DeviceObject = DriveInfo->DeviceObject;
  PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
  BOOLEAN WriteToDevice;
  ULONG Length;
  ULONG DiskByteOffset;
  KIRQL OldIrql;
  NTSTATUS Status;
  BOOLEAN DiskChanged;
  ULONG_PTR TransferByteOffset;
  UCHAR Gap;

  PAGED_CODE();

  TRACE_(FLOPPY, "ReadWritePassive called to %s 0x%x bytes from offset 0x%x\n",
	   (Stack->MajorFunction == IRP_MJ_READ ? "read" : "write"),
	   (Stack->MajorFunction == IRP_MJ_READ ? Stack->Parameters.Read.Length : Stack->Parameters.Write.Length),
	   (Stack->MajorFunction == IRP_MJ_READ ? Stack->Parameters.Read.ByteOffset.u.LowPart :
	    Stack->Parameters.Write.ByteOffset.u.LowPart));

  /* Default return codes */
  Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
  Irp->IoStatus.Information = 0;

  /*
   * Check to see if the volume needs to be verified.  If so,
   * we can get out of here quickly.
   */
  if(DeviceObject->Flags & DO_VERIFY_VOLUME && !(DeviceObject->Flags & SL_OVERRIDE_VERIFY_VOLUME))
    {
      INFO_(FLOPPY, "ReadWritePassive(): DO_VERIFY_VOLUME set; Completing with  STATUS_VERIFY_REQUIRED\n");
      Irp->IoStatus.Status = STATUS_VERIFY_REQUIRED;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return;
    }

  /*
   * Check the change line, and if it's set, return
   */
  StartMotor(DriveInfo);
  if(HwDiskChanged(DeviceObject->DeviceExtension, &DiskChanged) != STATUS_SUCCESS)
    {
      WARN_(FLOPPY, "ReadWritePassive(): unable to detect disk change; Completing with STATUS_UNSUCCESSFUL\n");
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      StopMotor(DriveInfo->ControllerInfo);
      return;
    }

  if(DiskChanged)
    {
      INFO_(FLOPPY, "ReadWritePhase1(): signalling media changed; Completing with STATUS_MEDIA_CHANGED\n");

      /* The following call sets IoStatus.Status and IoStatus.Information */
      SignalMediaChanged(DeviceObject, Irp);

      /*
       * Guessing at something... see ioctl.c for more info
       */
      if(ResetChangeFlag(DriveInfo) == STATUS_NO_MEDIA_IN_DEVICE)
	Irp->IoStatus.Status = STATUS_NO_MEDIA_IN_DEVICE;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      StopMotor(DriveInfo->ControllerInfo);
      return;
    }

  /*
   * Figure out the media type, if we don't know it already
   */
  if(DriveInfo->DiskGeometry.MediaType == Unknown)
    {
      if(RWDetermineMediaType(DriveInfo) != STATUS_SUCCESS)
	{
	  WARN_(FLOPPY, "ReadWritePassive(): unable to determine media type; completing with STATUS_UNSUCCESSFUL\n");
	  IoCompleteRequest(Irp, IO_NO_INCREMENT);
	  StopMotor(DriveInfo->ControllerInfo);
	  return;
	}

      if(DriveInfo->DiskGeometry.MediaType == Unknown)
	{
	  WARN_(FLOPPY, "ReadWritePassive(): Unknown media in drive; completing with STATUS_UNRECOGNIZED_MEDIA\n");
	  Irp->IoStatus.Status = STATUS_UNRECOGNIZED_MEDIA;
	  IoCompleteRequest(Irp, IO_NO_INCREMENT);
	  StopMotor(DriveInfo->ControllerInfo);
	  return;
	}
    }

  /* Set up parameters for read or write */
  if(Stack->MajorFunction == IRP_MJ_READ)
    {
      Length = Stack->Parameters.Read.Length;
      DiskByteOffset = Stack->Parameters.Read.ByteOffset.u.LowPart;
      WriteToDevice = FALSE;
    }
  else
    {
      Length = Stack->Parameters.Write.Length;
      DiskByteOffset = Stack->Parameters.Write.ByteOffset.u.LowPart;
      WriteToDevice = TRUE;
    }

  /*
   * FIXME:
   *   FloppyDeviceData.ReadWriteGapLength specify the value for the physical drive.
   *   We should set this value depend on the format of the inserted disk and possible
   *   depend on the request (read or write). A value of 0 results in one rotation
   *   between the sectors (7.2sec for reading a track).
   */
  Gap = DriveInfo->FloppyDeviceData.ReadWriteGapLength;

  /*
   * Set up DMA transfer
   *
   * This is as good of a place as any to document something that used to confuse me
   * greatly (and I even wrote some of the kernel's DMA code, so if it confuses me, it
   * probably confuses at least a couple of other people too).
   *
   * MmGetMdlVirtualAddress() returns the virtal address, as mapped in the buffer's original
   * process context, of the MDL.  In other words:  say you start with a buffer at address X, then
   * you build an MDL out of that buffer called Mdl. If you call MmGetMdlVirtualAddress(Mdl), it
   * will return X.
   *
   * There are two parameters that the function looks at to produce X again, given the MDL:  the
   * first is the StartVa, which is the base virtual address of the page that the buffer starts
   * in.  If your buffer's virtual address is 0x12345678, StartVa will be 0x12345000, assuming 4K pages
   * (which is (almost) always the case on x86).  Note well: this address is only valid in the
   * process context that you initially built the MDL from.  The physical pages that make up
   * the MDL might perhaps be mapped in other process contexts too (or even in the system space,
   * above 0x80000000 (default; 0xc0000000 on current ReactOS or /3GB Windows)), but it will
   * (possibly) be mapped at a different address.
   *
   * The second parameter is the ByteOffset.  Given an original buffer address of 0x12345678,
   * the ByteOffset would be 0x678.  Because MDLs can only describe full pages (and therefore
   * StartVa always points to the start address of a page), the ByteOffset must be used to
   * find the real start of the buffer.
   *
   * In general, if you add the StartVa and ByteOffset together, you get back your original
   * buffer pointer, which you are free to use if you're sure you're in the right process
   * context.  You could tell by accessing the (hidden and not-to-be-used) Process member of
   * the MDL, but in general, if you have to ask whether or not you are in the right context,
   * then you shouldn't be using this address for anything anyway.  There are also security implications
   * (big ones, really, I wouldn't kid about this) to directly accessing a user's buffer by VA, so
   * Don't Do That.
   *
   * There is a somewhat weird but very common use of the virtual address associated with a MDL
   * that pops up often in the context of DMA.  DMA APIs (particularly MapTransfer()) need to
   * know where the memory is that they should DMA into and out of.  This memory is described
   * by a MDL.  The controller eventually needs to know a physical address on the host side,
   * which is generally a 32-bit linear address (on x86), and not just a page address.  Therefore,
   * the DMA APIs look at the ByteOffset field of the MDL to reconstruct the real address that
   * should be programmed into the DMA controller.
   *
   * It is often the case that a transfer needs to be broken down over more than one DMA operation,
   * particularly when it is a big transfer and the HAL doesn't give you enough map registers
   * to map the whole thing at once.  Therefore, the APIs need a way to tell how far into the MDL
   * they should look to transfer the next chunk of bytes.  Now, Microsoft could have designed
   * MapTransfer to take a  "MDL offset" argument, starting with 0, for how far into the buffer to
   * start, but it didn't.  Instead, MapTransfer asks for the virtual address of the MDL as an "index" into
   * the MDL.  The way it computes how far into the page to start the transfer is by masking off all but
   * the bottom 12 bits (on x86) of the number you supply as the CurrentVa and using *that* as the
   * ByteOffset instead of the one in the MDL.  (OK, this varies a bit by OS and version, but this
   * is the effect).
   *
   * In other words, you get a number back from MmGetMdlVirtualAddress that represents the start of your
   * buffer, and you pass it to the first MapTransfer call.  Then, for each successive operation
   * on the same buffer, you increment that address to point to the next spot in the MDL that
   * you want to DMA to/from.  The fact that the virtual address you're manipulating is probably not
   * mapped into the process context that you're running in is irrelevant, since it's only being
   * used to index into the MDL.
   */

  /* Get map registers for DMA */
  KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
  Status = IoAllocateAdapterChannel(DriveInfo->ControllerInfo->AdapterObject, DeviceObject,
				    DriveInfo->ControllerInfo->MapRegisters, MapRegisterCallback, DriveInfo->ControllerInfo);
  KeLowerIrql(OldIrql);

  if(Status != STATUS_SUCCESS)
    {
      WARN_(FLOPPY, "ReadWritePassive(): unable allocate an adapter channel; completing with STATUS_UNSUCCESSFUL\n");
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      StopMotor(DriveInfo->ControllerInfo);
      return ;
    }


  /*
   * Read from (or write to) the device
   *
   * This has to be called in a loop, as you can only transfer data to/from a single track at
   * a time.
   */
  TransferByteOffset = 0;
  while(TransferByteOffset < Length)
    {
      UCHAR Cylinder;
      UCHAR Head;
      UCHAR StartSector;
      ULONG CurrentTransferBytes;
      UCHAR CurrentTransferSectors;

      INFO_(FLOPPY, "ReadWritePassive(): iterating in while (TransferByteOffset = 0x%x of 0x%x total) - allocating %d registers\n",
	       TransferByteOffset, Length, DriveInfo->ControllerInfo->MapRegisters);

      KeClearEvent(&DriveInfo->ControllerInfo->SynchEvent);

      /*
       * Compute starting CHS
       */
      if(RWComputeCHS(DriveInfo, DiskByteOffset+TransferByteOffset, &Cylinder, &Head, &StartSector) != STATUS_SUCCESS)
	{
	  WARN_(FLOPPY, "ReadWritePassive(): unable to compute CHS; completing with STATUS_UNSUCCESSFUL\n");
	  RWFreeAdapterChannel(DriveInfo->ControllerInfo->AdapterObject);
	  IoCompleteRequest(Irp, IO_NO_INCREMENT);
          StopMotor(DriveInfo->ControllerInfo);
	  return;
	}

      /*
       * Seek to the right track
       */
      if(!DriveInfo->ControllerInfo->ImpliedSeeks)
        {
	  if(RWSeekToCylinder(DriveInfo, Cylinder) != STATUS_SUCCESS)
            {
	      WARN_(FLOPPY, "ReadWritePassive(): unable to seek; completing with STATUS_UNSUCCESSFUL\n");
	      RWFreeAdapterChannel(DriveInfo->ControllerInfo->AdapterObject);
	      IoCompleteRequest(Irp, IO_NO_INCREMENT);
              StopMotor(DriveInfo->ControllerInfo);
	      return ;
	    }
        }

      /*
       * Compute last sector
       *
       * We can only ask for a transfer up to the end of the track.  Then we have to re-seek and do more.
       * TODO: Support the MT bit
       */
      INFO_(FLOPPY, "ReadWritePassive(): computing number of sectors to transfer (StartSector 0x%x): ", StartSector);

      /* 1-based sector number */
      if( (((DriveInfo->DiskGeometry.TracksPerCylinder - Head) * DriveInfo->DiskGeometry.SectorsPerTrack - StartSector) + 1 ) <
	  (Length - TransferByteOffset) / DriveInfo->DiskGeometry.BytesPerSector)
	{
	  CurrentTransferSectors = (UCHAR)((DriveInfo->DiskGeometry.TracksPerCylinder - Head) * DriveInfo->DiskGeometry.SectorsPerTrack - StartSector) + 1;
	}
      else
	{
	  CurrentTransferSectors = (UCHAR)((Length - TransferByteOffset) / DriveInfo->DiskGeometry.BytesPerSector);
	}

      INFO_(FLOPPY, "0x%x\n", CurrentTransferSectors);

      CurrentTransferBytes = CurrentTransferSectors * DriveInfo->DiskGeometry.BytesPerSector;

      /*
       * Adjust to map registers
       * BUG: Does this take into account page crossings?
       */
      INFO_(FLOPPY, "ReadWritePassive(): Trying to transfer 0x%x bytes\n", CurrentTransferBytes);

      ASSERT(CurrentTransferBytes);

      if(BYTES_TO_PAGES(CurrentTransferBytes) > DriveInfo->ControllerInfo->MapRegisters)
        {
          CurrentTransferSectors = (UCHAR)((DriveInfo->ControllerInfo->MapRegisters * PAGE_SIZE) /
	                                    DriveInfo->DiskGeometry.BytesPerSector);

          CurrentTransferBytes = CurrentTransferSectors * DriveInfo->DiskGeometry.BytesPerSector;

	  INFO_(FLOPPY, "ReadWritePassive: limiting transfer to 0x%x bytes (0x%x sectors) due to map registers\n",
		   CurrentTransferBytes, CurrentTransferSectors);
        }

      /* set up this round's dma operation */
      /* param 2 is ReadOperation --> opposite of WriteToDevice that IoMapTransfer takes.  BAD MS. */
      KeFlushIoBuffers(Irp->MdlAddress, !WriteToDevice, TRUE);

      IoMapTransfer(DriveInfo->ControllerInfo->AdapterObject, Irp->MdlAddress,
		    DriveInfo->ControllerInfo->MapRegisterBase,
		    (PVOID)((ULONG_PTR)MmGetMdlVirtualAddress(Irp->MdlAddress) + TransferByteOffset),
		    &CurrentTransferBytes, WriteToDevice);

      /*
       * Read or Write
       */
      KeClearEvent(&DriveInfo->ControllerInfo->SynchEvent);

      /* Issue the read/write command to the controller.  Note that it expects the opposite of WriteToDevice. */
      if(HwReadWriteData(DriveInfo->ControllerInfo, !WriteToDevice, DriveInfo->UnitNumber, Cylinder, Head, StartSector,
			 DriveInfo->BytesPerSectorCode, DriveInfo->DiskGeometry.SectorsPerTrack, Gap, 0xff) != STATUS_SUCCESS)
	{
	  WARN_(FLOPPY, "ReadWritePassive(): HwReadWriteData returned failure; unable to read; completing with STATUS_UNSUCCESSFUL\n");
	  RWFreeAdapterChannel(DriveInfo->ControllerInfo->AdapterObject);
	  IoCompleteRequest(Irp, IO_NO_INCREMENT);
          StopMotor(DriveInfo->ControllerInfo);
	  return ;
	}

      INFO_(FLOPPY, "ReadWritePassive(): HwReadWriteData returned -- waiting on event\n");

      /*
       * At this point, we block and wait for an interrupt
       * FIXME: this seems to take too long
       */
      WaitForControllerInterrupt(DriveInfo->ControllerInfo);

      /* Read is complete; flush & free adapter channel */
      IoFlushAdapterBuffers(DriveInfo->ControllerInfo->AdapterObject, Irp->MdlAddress,
			    DriveInfo->ControllerInfo->MapRegisterBase,
			    (PVOID)((ULONG_PTR)MmGetMdlVirtualAddress(Irp->MdlAddress) + TransferByteOffset),
			    CurrentTransferBytes, WriteToDevice);

      /* Read the results from the drive */
      if(HwReadWriteResult(DriveInfo->ControllerInfo) != STATUS_SUCCESS)
	{
	  WARN_(FLOPPY, "ReadWritePassive(): HwReadWriteResult returned failure; unable to read; completing with STATUS_UNSUCCESSFUL\n");
	  HwDumpRegisters(DriveInfo->ControllerInfo);
	  RWFreeAdapterChannel(DriveInfo->ControllerInfo->AdapterObject);
	  IoCompleteRequest(Irp, IO_NO_INCREMENT);
          StopMotor(DriveInfo->ControllerInfo);
	  return ;
	}

      TransferByteOffset += CurrentTransferBytes;
    }

  RWFreeAdapterChannel(DriveInfo->ControllerInfo->AdapterObject);

  /* That's all folks! */
  INFO_(FLOPPY, "ReadWritePassive(): success; Completing with STATUS_SUCCESS\n");
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = Length;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  StopMotor(DriveInfo->ControllerInfo);
}

