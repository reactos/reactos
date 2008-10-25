/*************************************************************************
*
* File: protos.h
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Contains the prototypes for functions in this sample FSD.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#ifndef	_EXT2_PROTOS_H_
#define	_EXT2_PROTOS_H_

#ifdef __REACTOS__
typedef PIO_STACK_LOCATION PEXTENDED_IO_STACK_LOCATION;
#endif

/*************************************************************************
* Prototypes for the file sfsdinit.c
*************************************************************************/
extern NTSTATUS STDCALL DriverEntry(
	PDRIVER_OBJECT			DriverObject,		// created by the I/O sub-system
	PUNICODE_STRING			RegistryPath);		// path to the registry key

extern void STDCALL Ext2FsdInitializeFunctionPointers(
	PDRIVER_OBJECT			DriverObject);		// created by the I/O sub-system


extern VOID STDCALL Ext2QueueHandlerThread(
	IN PVOID				StartContext);
												
/*************************************************************************
* Prototypes for the file fsctrl.c
*************************************************************************/

extern NTSTATUS STDCALL Ext2FileSystemControl(
    IN PDEVICE_OBJECT		DeviceObject,
    IN PIRP					Irp
    );

extern NTSTATUS STDCALL Ext2VerifyVolume (
	IN PIRP					Irp,
	IN PIO_STACK_LOCATION	IrpSp );


/*************************************************************************
* Prototypes for the file create.c
*************************************************************************/
extern NTSTATUS STDCALL Ext2Create(
PDEVICE_OBJECT				DeviceObject,		// the logical volume device object
PIRP						Irp);			// I/O Request Packet

extern NTSTATUS STDCALL Ext2CommonCreate(
PtrExt2IrpContext			PtrIrpContext,
PIRP						PtrIrp,
BOOLEAN						FirstAttempt );

extern NTSTATUS STDCALL Ext2OpenVolume(
	PtrExt2VCB				PtrVCB,				// volume to be opened
	PtrExt2IrpContext		PtrIrpContext,		// IRP context
	PIRP					PtrIrp,				// original/user IRP
	unsigned short			ShareAccess,		// share access
	PIO_SECURITY_CONTEXT	PtrSecurityContext,	// caller's context (incl access)
	PFILE_OBJECT			PtrNewFileObject);	// I/O Mgr. created file object

extern NTSTATUS STDCALL Ext2OpenRootDirectory(
	PtrExt2VCB				PtrVCB,					// volume to be opened
	PtrExt2IrpContext		PtrIrpContext,			// IRP context
	PIRP					PtrIrp,					// original/user IRP
	unsigned short			ShareAccess,			// share access
	PIO_SECURITY_CONTEXT	PtrSecurityContext,		// caller's context (incl access)
	PFILE_OBJECT			PtrNewFileObject);		// I/O Mgr. created file object

extern void STDCALL Ext2InitializeFCB(
	PtrExt2FCB				PtrNewFCB,		// FCB structure to be initialized
	PtrExt2VCB				PtrVCB,			// logical volume (VCB) pointer
	PtrExt2ObjectName		PtrObjectName,	// name of the object
	uint32					Flags,			// is this a file/directory, etc.
	PFILE_OBJECT			PtrFileObject);// optional file object to be initialized

extern PtrExt2FCB	STDCALL Ext2LocateChildFCBInCore(
	PtrExt2VCB				PtrVCB,	
	PUNICODE_STRING			PtrName, 
	ULONG					ParentInodeNo );

extern PtrExt2FCB	STDCALL Ext2LocateFCBInCore(
	PtrExt2VCB				PtrVCB,	
	ULONG					InodeNo );


extern ULONG	STDCALL Ext2LocateFileInDisk(
	PtrExt2VCB				PtrVCB,
	PUNICODE_STRING			PtrCurrentName, 
	PtrExt2FCB				PtrParentFCB, 
	ULONG					*Type );

extern ULONG STDCALL Ext2CreateFile(
	PtrExt2IrpContext		PtrIrpContext,
	PtrExt2VCB				PtrVCB,
	PUNICODE_STRING			PtrName, 
	PtrExt2FCB				PtrParentFCB,
	ULONG					Type);

extern BOOLEAN STDCALL Ext2OverwriteFile(
	PtrExt2FCB				PtrFCB,
	PtrExt2IrpContext		PtrIrpContext);

extern BOOLEAN STDCALL Ext2SupersedeFile(
	PtrExt2FCB				PtrFCB,
	PtrExt2IrpContext		PtrIrpContext);

/*************************************************************************
* Prototypes for the file misc.c
*************************************************************************/
extern NTSTATUS STDCALL Ext2InitializeZones(
void);

extern void STDCALL Ext2DestroyZones(
void);

extern BOOLEAN STDCALL Ext2IsIrpTopLevel(
PIRP							Irp);					// the IRP sent to our dispatch routine

extern long STDCALL Ext2ExceptionFilter(
PtrExt2IrpContext			PtrIrpContext,
PEXCEPTION_POINTERS		PtrExceptionPointers);

extern NTSTATUS STDCALL Ext2ExceptionHandler(
PtrExt2IrpContext			PtrIrpContext,
PIRP							Irp);

extern void STDCALL Ext2LogEvent(
NTSTATUS						Ext2EventLogId,	// the Ext2 private message id
NTSTATUS						RC);					// any NT error code we wish to log ...

extern PtrExt2ObjectName STDCALL Ext2AllocateObjectName(
void);

extern void STDCALL Ext2ReleaseObjectName(
PtrExt2ObjectName			PtrObjectName);

extern PtrExt2CCB STDCALL Ext2AllocateCCB(
void );

extern PtrExt2FCB STDCALL Ext2GetUsedFCB( 
PtrExt2VCB					PtrVCB );

extern BOOLEAN STDCALL Ext2CloseClosableFCB( 
PtrExt2FCB					PtrFCB );

extern void STDCALL Ext2ReleaseCCB(
PtrExt2CCB					PtrCCB);

extern PtrExt2FCB STDCALL Ext2AllocateFCB(
void);

extern NTSTATUS STDCALL Ext2CreateNewFCB(
PtrExt2FCB					*ReturnedFCB,
LARGE_INTEGER				AllocationSize,
LARGE_INTEGER				EndOfFile,
PFILE_OBJECT				PtrFileObject,
PtrExt2VCB					PtrVCB,
PtrExt2ObjectName			PtrObjectName);

extern NTSTATUS STDCALL Ext2CreateNewCCB(
PtrExt2CCB				*ReturnedCCB,
PtrExt2FCB				PtrFCB,
PFILE_OBJECT			PtrFileObject);

extern void STDCALL Ext2ReleaseFCB(
PtrExt2FCB					PtrFCB);

extern PtrExt2FileLockInfo STDCALL Ext2AllocateByteLocks(
void);

extern void STDCALL Ext2ReleaseByteLocks(
PtrExt2FileLockInfo		PtrByteLocks);

extern PtrExt2IrpContext STDCALL Ext2AllocateIrpContext(
PIRP							Irp,
PDEVICE_OBJECT				PtrTargetDeviceObject);

extern void STDCALL Ext2ReleaseIrpContext(
PtrExt2IrpContext			PtrIrpContext);

extern NTSTATUS STDCALL Ext2PostRequest(
PtrExt2IrpContext			PtrIrpContext,
PIRP						PtrIrp);

extern void STDCALL Ext2CommonDispatch(
void						*Context);	// actually an IRPContext structure

extern void STDCALL Ext2InitializeVCB(
PDEVICE_OBJECT				PtrVolumeDeviceObject,
PDEVICE_OBJECT				PtrTargetDeviceObject,
PVPB						PtrVPB,
PLARGE_INTEGER				AllocationSize);

extern void STDCALL Ext2CompleteRequest(
    IN PIRP					Irp OPTIONAL,
    IN NTSTATUS				Status
    );

extern NTSTATUS STDCALL Ext2DenyAccess( 
	IN PIRP Irp 
	);
extern NTSTATUS STDCALL Ext2GetFCB_CCB_VCB_FromFileObject(
	IN PFILE_OBJECT			PtrFileObject,
	OUT PtrExt2FCB				*PPtrFCB,
	OUT PtrExt2CCB				*PPtrCCB,
	OUT PtrExt2VCB				*PPtrVCB	);

extern void STDCALL Ext2CopyUnicodeString( 
	IN OUT PUNICODE_STRING  PtrDestinationString, 
	IN PUNICODE_STRING PtrSourceString );

extern void STDCALL Ext2CopyWideCharToUnicodeString( 
	IN OUT PUNICODE_STRING  PtrDestinationString, 
	IN PCWSTR PtrSourceString );

extern void STDCALL Ext2CopyCharToUnicodeString( 
	IN OUT PUNICODE_STRING  PtrDestinationString, 
	IN PCSTR PtrSourceString,
	IN USHORT SourceStringLength );

extern void STDCALL Ext2CopyZCharToUnicodeString( 
	IN OUT PUNICODE_STRING  PtrDestinationString, 
	IN PCSTR PtrSourceString );

extern void STDCALL Ext2DeallocateUnicodeString( 
	PUNICODE_STRING		PtrUnicodeString );

extern void STDCALL Ext2ZerooutUnicodeString(
	PUNICODE_STRING		PtrUnicodeString );

extern BOOLEAN STDCALL Ext2SaveBCB(
	PtrExt2IrpContext	PtrIrpContext,
	PBCB				PtrBCB,
	PFILE_OBJECT		PtrFileObject);

extern BOOLEAN STDCALL Ext2FlushSavedBCBs(
	PtrExt2IrpContext	PtrIrpContext);

extern BOOLEAN STDCALL AssertBCB(
	PBCB				PtrBCB);

extern ULONG STDCALL Ext2Align(
	ULONG				NumberToBeAligned, 
	ULONG				Alignment);

extern LONGLONG STDCALL Ext2Align64(
	LONGLONG			NumberToBeAligned, 
	LONGLONG			Alignment);

extern ULONG STDCALL Ext2GetCurrentTime();

/*************************************************************************
* Prototypes for the file cleanup.c
*************************************************************************/
extern NTSTATUS STDCALL Ext2Cleanup(
PDEVICE_OBJECT				DeviceObject,		// the logical volume device object
PIRP							Irp);					// I/O Request Packet

extern NTSTATUS	STDCALL Ext2CommonCleanup(
PtrExt2IrpContext			PtrIrpContext,
PIRP						PtrIrp,
BOOLEAN						FirstAttempt );

/*************************************************************************
* Prototypes for the file close.c
*************************************************************************/
extern NTSTATUS STDCALL Ext2Close(
PDEVICE_OBJECT				DeviceObject,		// the logical volume device object
PIRP							Irp);					// I/O Request Packet

extern NTSTATUS	STDCALL Ext2CommonClose(
PtrExt2IrpContext			PtrIrpContext,
PIRP						PtrIrp,
BOOLEAN						FirstAttempt );

/*************************************************************************
* Prototypes for the file read.c
*************************************************************************/
extern NTSTATUS STDCALL Ext2Read(
PDEVICE_OBJECT				DeviceObject,		// the logical volume device object
PIRP							Irp);					// I/O Request Packet

extern NTSTATUS	STDCALL Ext2CommonRead(
PtrExt2IrpContext			PtrIrpContext,
PIRP				      	PtrIrp,
BOOLEAN						FirstAttempt );

extern void * STDCALL Ext2GetCallersBuffer(
PIRP						PtrIrp);

extern NTSTATUS STDCALL Ext2LockCallersBuffer(
PIRP						PtrIrp,
BOOLEAN						IsReadOperation,
uint32						Length);

extern void STDCALL Ext2MdlComplete(
PtrExt2IrpContext			PtrIrpContext,
PIRP						PtrIrp,
PIO_STACK_LOCATION			PtrIoStackLocation,
BOOLEAN						ReadCompletion);

/*************************************************************************
* Prototypes for the file write.c
*************************************************************************/
extern NTSTATUS STDCALL Ext2Write(
PDEVICE_OBJECT				DeviceObject,		// the logical volume device object
PIRP							Irp);					// I/O Request Packet

extern NTSTATUS	STDCALL Ext2CommonWrite(
PtrExt2IrpContext			PtrIrpContext,
PIRP				      	PtrIrp);

extern void STDCALL Ext2DeferredWriteCallBack (
void							*Context1,			// Should be PtrIrpContext
void							*Context2);			// Should be PtrIrp

/*************************************************************************
* Prototypes for the file fileinfo.c
*************************************************************************/
extern NTSTATUS STDCALL Ext2FileInfo(
PDEVICE_OBJECT				DeviceObject,		// the logical volume device object
PIRP						Irp);					// I/O Request Packet

extern NTSTATUS	STDCALL Ext2CommonFileInfo(
PtrExt2IrpContext			PtrIrpContext,
PIRP						PtrIrp);

extern NTSTATUS	STDCALL Ext2GetBasicInformation(
	PtrExt2FCB					PtrFCB,
	PFILE_BASIC_INFORMATION		PtrBuffer,
	long						*PtrReturnedLength);

extern NTSTATUS	STDCALL Ext2GetStandardInformation(
	PtrExt2FCB					PtrFCB,
	PFILE_STANDARD_INFORMATION	PtrStdInformation,
	long						*PtrReturnedLength);

extern NTSTATUS STDCALL Ext2GetNetworkOpenInformation(
	PtrExt2FCB						PtrFCB,
	PFILE_NETWORK_OPEN_INFORMATION	PtrNetworkOpenInformation,
	long							*PtrReturnedLength );

extern NTSTATUS	STDCALL Ext2GetFullNameInformation(
	PtrExt2FCB				PtrFCB,
	PtrExt2CCB				PtrCCB,
	PFILE_NAME_INFORMATION	PtrNameInformation,
	long					*PtrReturnedLength);

extern NTSTATUS	STDCALL Ext2SetBasicInformation(
	PtrExt2IrpContext			PtrIrpContext,
	PtrExt2FCB					PtrFCB,
	PFILE_OBJECT				PtrFileObject,
	PFILE_BASIC_INFORMATION		PtrFileInformation );

extern NTSTATUS	STDCALL Ext2SetDispositionInformation(
PtrExt2FCB					PtrFCB,
PtrExt2CCB					PtrCCB,
PtrExt2VCB					PtrVCB,
PFILE_OBJECT				PtrFileObject,
PtrExt2IrpContext			PtrIrpContext,
PIRP							PtrIrp,
PFILE_DISPOSITION_INFORMATION	PtrBuffer);

extern NTSTATUS	STDCALL Ext2SetAllocationInformation(
PtrExt2FCB					PtrFCB,
PtrExt2CCB					PtrCCB,
PtrExt2VCB					PtrVCB,
PFILE_OBJECT				PtrFileObject,
PtrExt2IrpContext			PtrIrpContext,
PIRP							PtrIrp,
PFILE_ALLOCATION_INFORMATION	PtrBuffer);

/*************************************************************************
* Prototypes for the file flush.c
*************************************************************************/
extern NTSTATUS STDCALL Ext2Flush(
PDEVICE_OBJECT				DeviceObject,		// the logical volume device object
PIRP							Irp);					// I/O Request Packet

extern NTSTATUS	STDCALL Ext2CommonFlush(
PtrExt2IrpContext			PtrIrpContext,
PIRP							PtrIrp);

extern void STDCALL Ext2FlushAFile(
PtrExt2NTRequiredFCB		PtrReqdFCB,
PIO_STATUS_BLOCK			PtrIoStatus);

extern void STDCALL Ext2FlushLogicalVolume(
PtrExt2IrpContext			PtrIrpContext,
PIRP							PtrIrp,
PtrExt2VCB					PtrVCB);

extern NTSTATUS STDCALL Ext2FlushCompletion(
PDEVICE_OBJECT				PtrDeviceObject,
PIRP							PtrIrp,
PVOID							Context);

/*************************************************************************
* Prototypes for the file dircntrl.c
*************************************************************************/
extern NTSTATUS STDCALL Ext2DirControl(
PDEVICE_OBJECT				DeviceObject,		// the logical volume device object
PIRP							Irp);					// I/O Request Packet

extern NTSTATUS	STDCALL Ext2CommonDirControl(
PtrExt2IrpContext			PtrIrpContext,
PIRP							PtrIrp);

extern NTSTATUS	STDCALL Ext2QueryDirectory(
PtrExt2IrpContext			PtrIrpContext,
PIRP						PtrIrp,
PEXTENDED_IO_STACK_LOCATION			PtrIoStackLocation,
PFILE_OBJECT				PtrFileObject,
PtrExt2FCB					PtrFCB,
PtrExt2CCB					PtrCCB);

extern NTSTATUS	STDCALL Ext2NotifyChangeDirectory(
PtrExt2IrpContext			PtrIrpContext,
PIRP						PtrIrp,
PEXTENDED_IO_STACK_LOCATION		PtrIoStackLocation,
PFILE_OBJECT				PtrFileObject,
PtrExt2FCB					PtrFCB,
PtrExt2CCB					PtrCCB);

/*************************************************************************
* Prototypes for the file devcntrl.c
*************************************************************************/
extern NTSTATUS STDCALL Ext2DeviceControl(
PDEVICE_OBJECT				DeviceObject,		// the logical volume device object
PIRP						Irp);					// I/O Request Packet

extern NTSTATUS STDCALL Ext2CommonDeviceControl(
PtrExt2IrpContext			PtrIrpContext,
PIRP						PtrIrp);

extern NTSTATUS STDCALL Ext2DevIoctlCompletion(
PDEVICE_OBJECT				PtrDeviceObject,
PIRP						PtrIrp,
void						*Context);

extern NTSTATUS STDCALL Ext2HandleQueryPath(
void						*BufferPointer);

/*************************************************************************
* Prototypes for the file shutdown.c
*************************************************************************/
extern NTSTATUS STDCALL Ext2Shutdown(
PDEVICE_OBJECT				DeviceObject,		// the logical volume device object
PIRP						Irp);			// I/O Request Packet

extern NTSTATUS	STDCALL Ext2CommonShutdown(
PtrExt2IrpContext			PtrIrpContext,
PIRP						PtrIrp);

/*************************************************************************
* Prototypes for the file volinfo.c
*************************************************************************/
extern NTSTATUS STDCALL Ext2QueryVolInfo(
PDEVICE_OBJECT				DeviceObject,	// the logical volume device object
PIRP						Irp);			// I/O Request Packet

NTSTATUS STDCALL Ext2SetVolInfo(
	IN PDEVICE_OBJECT		DeviceObject,	// the logical volume device object
	IN PIRP					Irp);			// I/O Request Packet


/*************************************************************************
* Prototypes for the file fastio.c
*************************************************************************/
extern BOOLEAN STDCALL Ext2FastIoCheckIfPossible(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				FileOffset,
IN ULONG							Length,
IN BOOLEAN						Wait,
IN ULONG							LockKey,
IN BOOLEAN						CheckForReadOperation,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject);

extern BOOLEAN STDCALL Ext2FastIoRead(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				FileOffset,
IN ULONG							Length,
IN BOOLEAN						Wait,
IN ULONG							LockKey,
OUT PVOID						Buffer,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject);

extern BOOLEAN STDCALL Ext2FastIoWrite(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				FileOffset,
IN ULONG							Length,
IN BOOLEAN						Wait,
IN ULONG							LockKey,
OUT PVOID						Buffer,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject);

extern BOOLEAN STDCALL Ext2FastIoQueryBasicInfo(
IN PFILE_OBJECT					FileObject,
IN BOOLEAN							Wait,
OUT PFILE_BASIC_INFORMATION	Buffer,
OUT PIO_STATUS_BLOCK 			IoStatus,
IN PDEVICE_OBJECT					DeviceObject);

extern BOOLEAN STDCALL Ext2FastIoQueryStdInfo(
IN PFILE_OBJECT						FileObject,
IN BOOLEAN								Wait,
OUT PFILE_STANDARD_INFORMATION 	Buffer,
OUT PIO_STATUS_BLOCK 				IoStatus,
IN PDEVICE_OBJECT						DeviceObject);

extern BOOLEAN STDCALL Ext2FastIoLock(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				FileOffset,
IN PLARGE_INTEGER				Length,
PEPROCESS						ProcessId,
ULONG								Key,
BOOLEAN							FailImmediately,
BOOLEAN							ExclusiveLock,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject);

extern BOOLEAN STDCALL Ext2FastIoUnlockSingle(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				FileOffset,
IN PLARGE_INTEGER				Length,
PEPROCESS						ProcessId,
ULONG								Key,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject);

extern BOOLEAN STDCALL Ext2FastIoUnlockAll(
IN PFILE_OBJECT				FileObject,
PEPROCESS						ProcessId,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject);

extern BOOLEAN STDCALL Ext2FastIoUnlockAllByKey(
IN PFILE_OBJECT				FileObject,
PEPROCESS						ProcessId,
ULONG								Key,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject);

extern void STDCALL Ext2FastIoAcqCreateSec(
IN PFILE_OBJECT				FileObject);

extern void STDCALL Ext2FastIoRelCreateSec(
IN PFILE_OBJECT				FileObject);

extern BOOLEAN STDCALL Ext2AcqLazyWrite(
IN PVOID							Context,
IN BOOLEAN						Wait);

extern void STDCALL Ext2RelLazyWrite(
IN PVOID							Context);

extern BOOLEAN STDCALL Ext2AcqReadAhead(
IN PVOID							Context,
IN BOOLEAN						Wait);

extern void STDCALL Ext2RelReadAhead(
IN PVOID							Context);

// the remaining are only valid under NT Version 4.0 and later
#if(_WIN32_WINNT >= 0x0400)

extern BOOLEAN STDCALL Ext2FastIoQueryNetInfo(
IN PFILE_OBJECT									FileObject,
IN BOOLEAN											Wait,
OUT PFILE_NETWORK_OPEN_INFORMATION 			Buffer,
OUT PIO_STATUS_BLOCK 							IoStatus,
IN PDEVICE_OBJECT									DeviceObject);

extern BOOLEAN STDCALL Ext2FastIoMdlRead(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				FileOffset,
IN ULONG							Length,
IN ULONG							LockKey,
OUT PMDL							*MdlChain,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject);

extern BOOLEAN STDCALL Ext2FastIoMdlReadComplete(
IN PFILE_OBJECT				FileObject,
OUT PMDL							MdlChain,
IN PDEVICE_OBJECT				DeviceObject);

extern BOOLEAN STDCALL Ext2FastIoPrepareMdlWrite(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				FileOffset,
IN ULONG							Length,
IN ULONG							LockKey,
OUT PMDL							*MdlChain,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject);

extern BOOLEAN STDCALL Ext2FastIoMdlWriteComplete(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				FileOffset,
OUT PMDL							MdlChain,
IN PDEVICE_OBJECT				DeviceObject);

extern NTSTATUS STDCALL Ext2FastIoAcqModWrite(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				EndingOffset,
OUT PERESOURCE					*ResourceToRelease,
IN PDEVICE_OBJECT				DeviceObject);

extern NTSTATUS STDCALL Ext2FastIoRelModWrite(
IN PFILE_OBJECT				FileObject,
IN PERESOURCE					ResourceToRelease,
IN PDEVICE_OBJECT				DeviceObject);

extern NTSTATUS STDCALL Ext2FastIoAcqCcFlush(
IN PFILE_OBJECT				FileObject,
IN PDEVICE_OBJECT				DeviceObject);

extern NTSTATUS STDCALL Ext2FastIoRelCcFlush(
IN PFILE_OBJECT				FileObject,
IN PDEVICE_OBJECT				DeviceObject);

#endif	// (_WIN32_WINNT >= 0x0400)

/*************************************************************************
* Prototypes for the file DiskIO.c
*************************************************************************/
extern NTSTATUS STDCALL Ext2ReadLogicalBlocks(
PDEVICE_OBJECT		PtrTargetDeviceObject,	//	the Target Device Object
VOID				*Buffer,				//	The Buffer that takes the data read in
LARGE_INTEGER		StartLogicalBlock,		//	The logical block from which reading is to start
unsigned int		NoOfLogicalBlocks);		//	The no. of logical blocks to be read

extern NTSTATUS STDCALL Ext2ReadPhysicalBlocks(
	PDEVICE_OBJECT		PtrTargetDeviceObject,	//	the Target Device Object
	VOID				*Buffer,				//	The Buffer that takes the data read in
	LARGE_INTEGER		StartBlock,		//	The Physical block from which reading is to start
	unsigned int		NoOfBlocks);		//	The no. of Physical blocks to be read

/*************************************************************************
* Prototypes for the file metadata.c
*************************************************************************/

extern void STDCALL Ext2InitializeFCBInodeInfo (
	PtrExt2FCB	PtrFCB );

extern NTSTATUS STDCALL Ext2ReadInode(
	PtrExt2VCB			PtrVcb,			//	the Volume Control Block
	uint32				InodeNo,		//	The Inode no
	PEXT2_INODE			PtrInode );		//	The Inode Buffer

extern NTSTATUS STDCALL Ext2WriteInode(
	PtrExt2IrpContext	PtrIrpContext,
	PtrExt2VCB			PtrVcb,			//	the Volume Control Block
	uint32				InodeNo,		//	The Inode no
	PEXT2_INODE			PtrInode		//	The Inode Buffer
	);					

extern ULONG STDCALL Ext2AllocInode( 
	PtrExt2IrpContext	PtrIrpContext,
	PtrExt2VCB			PtrVCB,
	ULONG				ParentINodeNo );

extern BOOLEAN STDCALL Ext2DeallocInode( 
	PtrExt2IrpContext	PtrIrpContext,
	PtrExt2VCB			PtrVCB,
	ULONG				INodeNo );

extern BOOLEAN STDCALL Ext2MakeNewDirectoryEntry( 
	PtrExt2IrpContext	PtrIrpContext,		//	The Irp context
	PtrExt2FCB			PtrParentFCB,		//	Parent Folder FCB
	PFILE_OBJECT		PtrFileObject,		//	Parent Folder Object
	PUNICODE_STRING		PtrName,			//	New entry's name
	ULONG				Type,				//	The type of the new entry
	ULONG				NewInodeNo);		//	The inode no of the new entry...

extern BOOLEAN STDCALL Ext2FreeDirectoryEntry(
	PtrExt2IrpContext	PtrIrpContext,
	PtrExt2FCB			PtrParentFCB,
	PUNICODE_STRING		PtrName);

extern BOOLEAN STDCALL Ext2AddBlockToFile(
	PtrExt2IrpContext	PtrIrpContext,
	PtrExt2VCB			PtrVCB,
	PtrExt2FCB			PtrFCB,
	PFILE_OBJECT		PtrFileObject,
	BOOLEAN				UpdateFileSize);

extern BOOLEAN STDCALL Ext2ReleaseDataBlocks(
	PtrExt2FCB			PtrFCB,
	PtrExt2IrpContext	PtrIrpContext);

extern BOOLEAN STDCALL Ext2TruncateFileAllocationSize(
	PtrExt2IrpContext	PtrIrpContext,
	PtrExt2FCB			PtrFCB,
	PFILE_OBJECT		PtrFileObject,
	PLARGE_INTEGER		PtrAllocationSize );

extern ULONG STDCALL Ext2AllocBlock( 
	PtrExt2IrpContext	PtrIrpContext,
	PtrExt2VCB			PtrVCB,
	ULONG				Count);

extern BOOLEAN STDCALL Ext2DeallocBlock( 
	PtrExt2IrpContext	PtrIrpContext,
	PtrExt2VCB			PtrVCB,
	ULONG				BlockNo);

extern BOOLEAN STDCALL Ext2UpdateFileSize(
	PtrExt2IrpContext	PtrIrpContext,
	PFILE_OBJECT		PtrFileObject,
	PtrExt2FCB			PtrFCB);


extern BOOLEAN STDCALL Ext2DeleteFile(
	PtrExt2FCB			PtrFCB,
	PtrExt2IrpContext	PtrIrpContext);

extern BOOLEAN STDCALL Ext2IsDirectoryEmpty(
	PtrExt2FCB			PtrFCB,
	PtrExt2CCB			PtrCCB,
	PtrExt2IrpContext	PtrIrpContext);

extern NTSTATUS STDCALL Ext2RenameOrLinkFile( 
	PtrExt2FCB					PtrSourceFCB, 
	PFILE_OBJECT				PtrSourceFileObject,	
	PtrExt2IrpContext			PtrIrpContext,
	PIRP						PtrIrp, 
	PFILE_RENAME_INFORMATION	PtrRenameInfo);
/*************************************************************************
* Prototypes for the file io.c
*************************************************************************/
extern NTSTATUS STDCALL Ext2PassDownSingleReadWriteIRP(
	PtrExt2IrpContext	PtrIrpContext,
	PIRP				PtrIrp, 
	PtrExt2VCB			PtrVCB,
	LARGE_INTEGER		ByteOffset, 
	uint32				ReadWriteLength, 
	BOOLEAN				SynchronousIo);

extern NTSTATUS STDCALL Ext2PassDownMultiReadWriteIRP( 
	PEXT2_IO_RUN			PtrIoRuns, 
	UINT					Count, 
	ULONG					TotalReadWriteLength,
	PtrExt2IrpContext		PtrIrpContext,
	PtrExt2FCB				PtrFCB,
	BOOLEAN					SynchronousIo);

extern NTSTATUS STDCALL Ext2SingleSyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

extern NTSTATUS STDCALL Ext2SingleAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

extern NTSTATUS STDCALL Ext2MultiSyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt);

extern NTSTATUS STDCALL Ext2MultiAsyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt);

#endif	// _EXT2_PROTOS_H_
