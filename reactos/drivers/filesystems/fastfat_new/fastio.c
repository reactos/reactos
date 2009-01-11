/*
 * FILE:             drivers/fs/vfat/fastio.c
 * PURPOSE:          Fast IO routines.
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PROGRAMMER:       Herve Poussineau (hpoussin@reactos.org)
 */

#define NDEBUG
#include "fastfat.h"

static BOOLEAN NTAPI
VfatFastIoCheckIfPossible(IN PFILE_OBJECT FileObject,
                          IN PLARGE_INTEGER FileOffset,
                          IN ULONG Lenght,
                          IN BOOLEAN Wait,
                          IN ULONG LockKey,
                          IN BOOLEAN CheckForReadOperation,
                          OUT PIO_STATUS_BLOCK IoStatus,
                          IN PDEVICE_OBJECT DeviceObject)
{
	/* Prevent all Fast I/O requests */
	DPRINT("VfatFastIoCheckIfPossible(): returning FALSE.\n");
	return FALSE;
}

static BOOLEAN NTAPI
VfatFastIoRead(IN PFILE_OBJECT FileObject,
			   IN PLARGE_INTEGER FileOffset,
			   IN ULONG	Length,
			   IN BOOLEAN Wait,
			   IN ULONG LockKey,
			   OUT PVOID Buffer,
			   OUT PIO_STATUS_BLOCK	IoStatus,
               IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoRead()\n");
   return FALSE;
}

static BOOLEAN NTAPI
VfatFastIoWrite(IN PFILE_OBJECT FileObject,
				IN PLARGE_INTEGER FileOffset,
				IN ULONG Length,
				IN BOOLEAN Wait,
				IN ULONG LockKey,
				OUT PVOID Buffer,
				OUT PIO_STATUS_BLOCK IoStatus,
				IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoWrite()\n");
   return FALSE;
}

static BOOLEAN NTAPI
VfatFastIoQueryBasicInfo(IN PFILE_OBJECT FileObject,
						 IN BOOLEAN	Wait,
						 OUT PFILE_BASIC_INFORMATION Buffer,
						 OUT PIO_STATUS_BLOCK IoStatus,
						 IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoQueryBasicInfo()\n");
   return FALSE;
}

static BOOLEAN NTAPI
VfatFastIoQueryStandardInfo(IN PFILE_OBJECT FileObject,
							IN BOOLEAN Wait,
							OUT PFILE_STANDARD_INFORMATION Buffer,
							OUT PIO_STATUS_BLOCK IoStatus,
							IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoQueryStandardInfo\n");
   return FALSE;
}

static BOOLEAN NTAPI
VfatFastIoLock(IN PFILE_OBJECT FileObject,
			   IN PLARGE_INTEGER FileOffset,
			   IN PLARGE_INTEGER Length,
			   PEPROCESS ProcessId,
			   ULONG Key,
			   BOOLEAN FailImmediately,
			   BOOLEAN ExclusiveLock,
			   OUT PIO_STATUS_BLOCK IoStatus,
			   IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoLock\n");
   return FALSE;
}

static BOOLEAN NTAPI
VfatFastIoUnlockSingle(IN PFILE_OBJECT FileObject,
					   IN PLARGE_INTEGER FileOffset,
					   IN PLARGE_INTEGER Length,
					   PEPROCESS ProcessId,
					   ULONG Key,
					   OUT PIO_STATUS_BLOCK IoStatus,
					   IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoUnlockSingle\n");
   return FALSE;
}

static BOOLEAN NTAPI
VfatFastIoUnlockAll(IN PFILE_OBJECT FileObject,
					PEPROCESS ProcessId,
					OUT PIO_STATUS_BLOCK IoStatus,
					IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoUnlockAll\n");
   return FALSE;
}

static BOOLEAN NTAPI
VfatFastIoUnlockAllByKey(IN PFILE_OBJECT FileObject,
						 PVOID ProcessId,
						 ULONG Key,
						 OUT PIO_STATUS_BLOCK IoStatus,
						 IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoUnlockAllByKey\n");
   return FALSE;
}

static BOOLEAN NTAPI
VfatFastIoDeviceControl(IN PFILE_OBJECT FileObject,
						IN BOOLEAN Wait,
						IN PVOID InputBuffer OPTIONAL,
						IN ULONG InputBufferLength,
						OUT PVOID OutputBuffer OPTIONAL,
						IN ULONG OutputBufferLength,
						IN ULONG IoControlCode,
						OUT PIO_STATUS_BLOCK IoStatus,
						IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoDeviceControl\n");
   return FALSE;
}

static VOID NTAPI
VfatAcquireFileForNtCreateSection(IN PFILE_OBJECT FileObject)
{
   DPRINT("VfatAcquireFileForNtCreateSection\n");
}

static VOID NTAPI
VfatReleaseFileForNtCreateSection(IN PFILE_OBJECT FileObject)
{
   DPRINT("VfatReleaseFileForNtCreateSection\n");
}

static VOID NTAPI
VfatFastIoDetachDevice(IN PDEVICE_OBJECT SourceDevice,
					   IN PDEVICE_OBJECT TargetDevice)
{
   DPRINT("VfatFastIoDetachDevice\n");
}

static BOOLEAN NTAPI
VfatFastIoQueryNetworkOpenInfo(IN PFILE_OBJECT FileObject,
                               IN BOOLEAN Wait,
							   OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
							   OUT PIO_STATUS_BLOCK IoStatus,
							   IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoQueryNetworkOpenInfo\n");
   return FALSE;
}

static NTSTATUS NTAPI
VfatAcquireForModWrite(IN PFILE_OBJECT FileObject,
					   IN PLARGE_INTEGER EndingOffset,
					   OUT PERESOURCE* ResourceToRelease,
					   IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatAcquireForModWrite\n");
   return STATUS_INVALID_DEVICE_REQUEST;
}

static BOOLEAN NTAPI
VfatMdlRead(IN PFILE_OBJECT FileObject,
			IN PLARGE_INTEGER FileOffset,
			IN ULONG Length,
			IN ULONG LockKey,
			OUT PMDL* MdlChain,
			OUT PIO_STATUS_BLOCK IoStatus,
			IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatMdlRead\n");
   return FALSE;
}

static BOOLEAN NTAPI
VfatMdlReadComplete(IN PFILE_OBJECT FileObject,
					IN PMDL MdlChain,
					IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatMdlReadComplete\n");
   return FALSE;
}

static BOOLEAN NTAPI
VfatPrepareMdlWrite(IN PFILE_OBJECT FileObject,
					IN PLARGE_INTEGER FileOffset,
					IN ULONG Length,
					IN ULONG LockKey,
					OUT PMDL* MdlChain,
					OUT PIO_STATUS_BLOCK IoStatus,
					IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatPrepareMdlWrite\n");
   return FALSE;
}

static BOOLEAN NTAPI
VfatMdlWriteComplete(IN PFILE_OBJECT FileObject,
					 IN PLARGE_INTEGER FileOffset,
					 IN PMDL MdlChain,
					 IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatMdlWriteComplete\n");
   return FALSE;
}

static BOOLEAN NTAPI
VfatFastIoReadCompressed(IN PFILE_OBJECT FileObject,
						 IN PLARGE_INTEGER FileOffset,
						 IN ULONG Length,
						 IN ULONG LockKey,
						 OUT PVOID Buffer,
						 OUT PMDL* MdlChain,
						 OUT PIO_STATUS_BLOCK IoStatus,
						 OUT PCOMPRESSED_DATA_INFO CompressedDataInfo,
						 IN ULONG CompressedDataInfoLength,
						 IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoReadCompressed\n");
   return FALSE;
}

static BOOLEAN NTAPI
VfatFastIoWriteCompressed(IN PFILE_OBJECT FileObject,
						  IN PLARGE_INTEGER FileOffset,
						  IN ULONG Length,
						  IN ULONG LockKey,
						  IN PVOID Buffer,
						  OUT PMDL* MdlChain,
						  OUT PIO_STATUS_BLOCK IoStatus,
						  IN PCOMPRESSED_DATA_INFO CompressedDataInfo,
						  IN ULONG CompressedDataInfoLength,
						  IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoWriteCompressed\n");
   return FALSE;
}

static BOOLEAN NTAPI
VfatMdlReadCompleteCompressed(IN PFILE_OBJECT FileObject,
							  IN PMDL MdlChain,
							  IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatMdlReadCompleteCompressed\n");
   return FALSE;
}

static BOOLEAN NTAPI
VfatMdlWriteCompleteCompressed(IN PFILE_OBJECT FileObject,
							   IN PLARGE_INTEGER FileOffset,
							   IN PMDL MdlChain,
							   IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatMdlWriteCompleteCompressed\n");
   return FALSE;
}

static BOOLEAN NTAPI
VfatFastIoQueryOpen(IN PIRP Irp,
					OUT PFILE_NETWORK_OPEN_INFORMATION  NetworkInformation,
					IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoQueryOpen\n");
   return FALSE;
}

static NTSTATUS NTAPI
VfatReleaseForModWrite(IN PFILE_OBJECT FileObject,
					   IN PERESOURCE ResourceToRelease,
					   IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatReleaseForModWrite\n");
   return STATUS_INVALID_DEVICE_REQUEST;
}

static NTSTATUS NTAPI
VfatAcquireForCcFlush(IN PFILE_OBJECT FileObject,
					  IN PDEVICE_OBJECT DeviceObject)
{
   PVFATFCB Fcb = (PVFATFCB)FileObject->FsContext;

   DPRINT("VfatAcquireForCcFlush\n");

   /* Make sure it is not a volume lock */
   ASSERT(!(Fcb->Flags & FCB_IS_VOLUME));

   /* Acquire the resource */
   ExAcquireResourceExclusiveLite(&(Fcb->MainResource), TRUE);

   return STATUS_SUCCESS;
}

static NTSTATUS NTAPI
VfatReleaseForCcFlush(IN PFILE_OBJECT FileObject,
					  IN PDEVICE_OBJECT DeviceObject)
{
   PVFATFCB Fcb = (PVFATFCB)FileObject->FsContext;

   DPRINT("VfatReleaseForCcFlush\n");

   /* Make sure it is not a volume lock */
   ASSERT(!(Fcb->Flags & FCB_IS_VOLUME));

   /* Release the resource */
   ExReleaseResourceLite(&(Fcb->MainResource));

   return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
FatAcquireForLazyWrite(IN PVOID Context,
                        IN BOOLEAN Wait)
{
	PVFATFCB Fcb = (PVFATFCB)Context;
	ASSERT(Fcb);
	DPRINT("VfatAcquireForLazyWrite(): Fcb %p\n", Fcb);

	if (!ExAcquireResourceExclusiveLite(&(Fcb->MainResource), Wait))
	{
		DPRINT("VfatAcquireForLazyWrite(): ExReleaseResourceLite failed.\n");
		return FALSE;
	}
	return TRUE;
}

VOID
NTAPI
FatReleaseFromLazyWrite(IN PVOID Context)
{
	PVFATFCB Fcb = (PVFATFCB)Context;
	ASSERT(Fcb);
	DPRINT("VfatReleaseFromLazyWrite(): Fcb %p\n", Fcb);

	ExReleaseResourceLite(&(Fcb->MainResource));
}

BOOLEAN
NTAPI
FatAcquireForReadAhead(IN PVOID Context,
                        IN BOOLEAN Wait)
{
	PVFATFCB Fcb = (PVFATFCB)Context;
	ASSERT(Fcb);
	DPRINT("VfatAcquireForReadAhead(): Fcb %p\n", Fcb);

	if (!ExAcquireResourceExclusiveLite(&(Fcb->MainResource), Wait))
	{
		DPRINT("VfatAcquireForReadAhead(): ExReleaseResourceLite failed.\n");
		return FALSE;
	}
	return TRUE;
}

VOID
NTAPI
FatReleaseFromReadAhead(IN PVOID Context)
{
	PVFATFCB Fcb = (PVFATFCB)Context;
	ASSERT(Fcb);
	DPRINT("VfatReleaseFromReadAhead(): Fcb %p\n", Fcb);

	ExReleaseResourceLite(&(Fcb->MainResource));
}

BOOLEAN
NTAPI
FatNoopAcquire(IN PVOID Context,
               IN BOOLEAN Wait)
{
    return TRUE;
}

VOID
NTAPI
FatNoopRelease(IN PVOID Context)
{
}


VOID
FatInitFastIoRoutines(PFAST_IO_DISPATCH FastIoDispatch)
{
   FastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
   FastIoDispatch->FastIoCheckIfPossible = VfatFastIoCheckIfPossible;
   FastIoDispatch->FastIoRead = VfatFastIoRead;
   FastIoDispatch->FastIoWrite = VfatFastIoWrite;
   FastIoDispatch->FastIoQueryBasicInfo = VfatFastIoQueryBasicInfo;
   FastIoDispatch->FastIoQueryStandardInfo = VfatFastIoQueryStandardInfo;
   FastIoDispatch->FastIoLock = VfatFastIoLock;
   FastIoDispatch->FastIoUnlockSingle = VfatFastIoUnlockSingle;
   FastIoDispatch->FastIoUnlockAll = VfatFastIoUnlockAll;
   FastIoDispatch->FastIoUnlockAllByKey = VfatFastIoUnlockAllByKey;
   FastIoDispatch->FastIoDeviceControl = VfatFastIoDeviceControl;
   FastIoDispatch->AcquireFileForNtCreateSection = VfatAcquireFileForNtCreateSection;
   FastIoDispatch->ReleaseFileForNtCreateSection = VfatReleaseFileForNtCreateSection;
   FastIoDispatch->FastIoDetachDevice = VfatFastIoDetachDevice;
   FastIoDispatch->FastIoQueryNetworkOpenInfo = VfatFastIoQueryNetworkOpenInfo;
   FastIoDispatch->MdlRead = VfatMdlRead;
   FastIoDispatch->MdlReadComplete = VfatMdlReadComplete;
   FastIoDispatch->PrepareMdlWrite = VfatPrepareMdlWrite;
   FastIoDispatch->MdlWriteComplete = VfatMdlWriteComplete;
   FastIoDispatch->FastIoReadCompressed = VfatFastIoReadCompressed;
   FastIoDispatch->FastIoWriteCompressed = VfatFastIoWriteCompressed;
   FastIoDispatch->MdlReadCompleteCompressed = VfatMdlReadCompleteCompressed;
   FastIoDispatch->MdlWriteCompleteCompressed = VfatMdlWriteCompleteCompressed;
   FastIoDispatch->FastIoQueryOpen = VfatFastIoQueryOpen;
   FastIoDispatch->AcquireForModWrite = VfatAcquireForModWrite;
   FastIoDispatch->ReleaseForModWrite = VfatReleaseForModWrite;
   FastIoDispatch->AcquireForCcFlush = VfatAcquireForCcFlush;
   FastIoDispatch->ReleaseForCcFlush = VfatReleaseForCcFlush;
}

