/* $Id$ 
 *
 *
 * FILE:             drivers/fs/vfat/fastio.c
 * PURPOSE:          Fast IO routines.
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PROGRAMMER:       Herve Poussineau (reactos@poussine.freesurf.fr)
 *                   Hartmut Birr
 */

#define NDEBUG
#include "vfat.h"

BOOLEAN STDCALL
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

BOOLEAN STDCALL
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

BOOLEAN STDCALL
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

BOOLEAN STDCALL
VfatFastIoQueryBasicInfo(IN PFILE_OBJECT FileObject,
						 IN BOOLEAN	Wait,
						 OUT PFILE_BASIC_INFORMATION Buffer,
                         OUT PIO_STATUS_BLOCK IoStatus,
						 IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoQueryBasicInfo()\n");
   return FALSE;
}

BOOLEAN STDCALL
VfatFastIoQueryStandardInfo(IN PFILE_OBJECT FileObject,
							IN BOOLEAN Wait,
							OUT PFILE_STANDARD_INFORMATION Buffer,
							OUT PIO_STATUS_BLOCK IoStatus,
							IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoQueryStandardInfo\n");
   return FALSE;
}

BOOLEAN STDCALL
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

BOOLEAN STDCALL
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

BOOLEAN STDCALL
VfatFastIoUnlockAll(IN PFILE_OBJECT FileObject,
					PEPROCESS ProcessId,
					OUT PIO_STATUS_BLOCK IoStatus,
					IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoUnlockAll\n");
   return FALSE;
}

BOOLEAN STDCALL
VfatFastIoUnlockAllByKey(IN PFILE_OBJECT FileObject,
						 PEPROCESS ProcessId,
						 ULONG Key,
						 OUT PIO_STATUS_BLOCK IoStatus,
						 IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoUnlockAllByKey\n");
   return FALSE;
}

BOOLEAN STDCALL
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

VOID STDCALL
VfatAcquireFileForNtCreateSection(IN PFILE_OBJECT FileObject)
{
   DPRINT("VfatAcquireFileForNtCreateSection\n");
}

VOID STDCALL
VfatReleaseFileForNtCreateSection(IN PFILE_OBJECT FileObject)
{
   DPRINT("VfatReleaseFileForNtCreateSection\n");
}

VOID STDCALL
VfatFastIoDetachDevice(IN PDEVICE_OBJECT SourceDevice,
					   IN PDEVICE_OBJECT TargetDevice)
{
   DPRINT("VfatFastIoDetachDevice\n");
}

BOOLEAN STDCALL
VfatFastIoQueryNetworkOpenInfo(IN PFILE_OBJECT FileObject,
                               IN BOOLEAN Wait,
							   OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
							   OUT PIO_STATUS_BLOCK IoStatus,
							   IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoQueryNetworkOpenInfo\n");
   return FALSE;
}

NTSTATUS STDCALL
VfatAcquireForModWrite(IN PFILE_OBJECT FileObject,
					   IN PLARGE_INTEGER EndingOffset,
					   OUT PERESOURCE* ResourceToRelease,
					   IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatAcquireForModWrite\n");
   return STATUS_UNSUCCESSFUL;
}

BOOLEAN STDCALL
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

BOOLEAN STDCALL
VfatMdlReadComplete(IN PFILE_OBJECT FileObject,
					IN PMDL MdlChain,
					IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatMdlReadComplete\n");
   return FALSE;
}

BOOLEAN STDCALL
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

BOOLEAN STDCALL
VfatMdlWriteComplete(IN PFILE_OBJECT FileObject,
					 IN PLARGE_INTEGER FileOffset,
					 IN PMDL MdlChain,
					 IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatMdlWriteComplete\n");
   return FALSE;
}

BOOLEAN STDCALL
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

BOOLEAN STDCALL
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

BOOLEAN STDCALL
VfatMdlReadCompleteCompressed(IN PFILE_OBJECT FileObject,
							  IN PMDL MdlChain,
							  IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatMdlReadCompleteCompressed\n");
   return FALSE;
}

BOOLEAN STDCALL
VfatMdlWriteCompleteCompressed(IN PFILE_OBJECT FileObject,
							   IN PLARGE_INTEGER FileOffset,
							   IN PMDL MdlChain,
							   IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatMdlWriteCompleteCompressed\n");
   return FALSE;
}

BOOLEAN STDCALL
VfatFastIoQueryOpen(IN PIRP Irp,
					OUT PFILE_NETWORK_OPEN_INFORMATION  NetworkInformation,
                    IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatFastIoQueryOpen\n");
   return FALSE;
}

NTSTATUS STDCALL
VfatReleaseForModWrite(IN PFILE_OBJECT FileObject,
					   IN PERESOURCE ResourceToRelease,
					   IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatReleaseForModWrite\n");
   return STATUS_UNSUCCESSFUL;
}

NTSTATUS STDCALL
VfatAcquireForCcFlush(IN PFILE_OBJECT FileObject,
					  IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatAcquireForCcFlush\n");
   return STATUS_UNSUCCESSFUL;
}

NTSTATUS STDCALL
VfatReleaseForCcFlush(IN PFILE_OBJECT FileObject,
					  IN PDEVICE_OBJECT DeviceObject)
{
   DPRINT("VfatReleaseForCcFlush\n");
   return STATUS_UNSUCCESSFUL;
}

BOOLEAN STDCALL
VfatAcquireForLazyWrite(IN PVOID Context,
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

VOID STDCALL
VfatReleaseFromLazyWrite(IN PVOID Context)
{
   PVFATFCB Fcb = (PVFATFCB)Context;
	ASSERT(Fcb);
	DPRINT("VfatReleaseFromLazyWrite(): Fcb %p\n", Fcb);
	
	ExReleaseResourceLite(&(Fcb->MainResource));
}

BOOLEAN STDCALL
VfatAcquireForReadAhead(IN PVOID Context,
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

VOID STDCALL
VfatReleaseFromReadAhead(IN PVOID Context)
{
   PVFATFCB Fcb = (PVFATFCB)Context;
	ASSERT(Fcb);
	DPRINT("VfatReleaseFromReadAhead(): Fcb %p\n", Fcb);
	
	ExReleaseResourceLite(&(Fcb->MainResource));
}

VOID
VfatInitFastIoRoutines(PFAST_IO_DISPATCH FastIoDispatch)
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
   FastIoDispatch->AcquireForModWrite = VfatAcquireForModWrite;
   FastIoDispatch->MdlRead = VfatMdlRead;
   FastIoDispatch->MdlReadComplete = VfatMdlReadComplete;
   FastIoDispatch->PrepareMdlWrite = VfatPrepareMdlWrite;
   FastIoDispatch->MdlWriteComplete = VfatMdlWriteComplete;
   FastIoDispatch->FastIoReadCompressed = VfatFastIoReadCompressed;
   FastIoDispatch->FastIoWriteCompressed = VfatFastIoWriteCompressed;
   FastIoDispatch->MdlReadCompleteCompressed = VfatMdlReadCompleteCompressed;
   FastIoDispatch->MdlWriteCompleteCompressed = VfatMdlWriteCompleteCompressed;
   FastIoDispatch->FastIoQueryOpen = VfatFastIoQueryOpen;
   FastIoDispatch->ReleaseForModWrite = VfatReleaseForModWrite;
   FastIoDispatch->AcquireForCcFlush = VfatAcquireForCcFlush;
   FastIoDispatch->ReleaseForCcFlush = VfatReleaseForCcFlush;
}


