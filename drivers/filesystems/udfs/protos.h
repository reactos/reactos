////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: protos.h
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   Contains the prototypes for functions in UDF FSD.
*
*************************************************************************/

#ifndef _UDF_PROTOS_H_
#define _UDF_PROTOS_H_

#include "mem.h"

//  Type of opens.  FilObSup.c depends on this order.

typedef enum _TYPE_OF_OPEN {

    UnopenedFileObject = 0,
    StreamFileOpen,
    UserVolumeOpen,
    UserDirectoryOpen,
    UserFileOpen,
    BeyondValidType

} TYPE_OF_OPEN;

// The following macro is used to determine if an FSD thread can block
// for I/O or wait for a resource.  It returns TRUE if the thread can
// block and FALSE otherwise.  This attribute can then be used to call
// the FSD & FSP common work routine with the proper wait value.

#define CanFsdWait(I)   IoIsOperationSynchronous(I)

_When_(TypeOfOpen == UnopenedFileObject, _At_(Fcb, _In_opt_))
_When_(TypeOfOpen != UnopenedFileObject, _At_(Fcb, _In_))
VOID
UDFSetFileObject (
    _Inout_ PFILE_OBJECT FileObject,
    _In_ TYPE_OF_OPEN TypeOfOpen,
    PFCB Fcb,
    _In_opt_ PCCB Ccb
    );

_When_(return == UnopenedFileObject, _At_(*Fcb, _Post_null_))
_When_(return != UnopenedFileObject, _At_(Fcb, _Outptr_))
_When_(return == UnopenedFileObject, _At_(*Ccb, _Post_null_))
_When_(return != UnopenedFileObject, _At_(Ccb, _Outptr_))
TYPE_OF_OPEN
UDFDecodeFileObject (
    _In_ PFILE_OBJECT FileObject,
    PFCB *Fcb,
    PCCB *Ccb
    );

TYPE_OF_OPEN
UDFFastDecodeFileObject (
    _In_ PFILE_OBJECT FileObject,
    _Out_ PFCB *Fcb
    );

/*************************************************************************
* Prototypes for the file create.cpp
*************************************************************************/
extern NTSTATUS NTAPI UDFCreate(
    IN PDEVICE_OBJECT          DeviceObject,       // the logical volume device object
    IN PIRP                    Irp);               // I/O Request Packet

NTSTATUS
UDFCommonCreate(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
UDFOpenObjectByFileId(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PIO_STACK_LOCATION IrpSp,
    _In_ PVCB Vcb,
    _Inout_ PFCB *CurrentFcb
    );

NTSTATUS
UDFFirstOpenFile(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
   OUT PFCB* PtrNewFcb,
    IN PUDF_FILE_INFO RelatedFileInfo,
    IN PUDF_FILE_INFO NewFileInfo
    );

NTSTATUS
UDFInitializeFCB(
    IN PFCB                    PtrNewFcb,          // FCB structure to be initialized
    IN PVCB                    Vcb,                // logical volume (VCB) pointer
    IN ULONG                   Flags               // is this a file/directory, etc.
    );

/*************************************************************************
* Prototypes for the file cleanup.cpp
*************************************************************************/
extern NTSTATUS NTAPI UDFCleanup(
PDEVICE_OBJECT              DeviceObject,       // the logical volume device object
PIRP                        Irp);               // I/O Request Packet

extern NTSTATUS UDFCommonCleanup(
PIRP_CONTEXT IrpContext,
PIRP                        Irp);

/*************************************************************************
* Prototypes for the file close.cpp
*************************************************************************/
extern NTSTATUS NTAPI UDFClose(
PDEVICE_OBJECT              DeviceObject,       // the logical volume device object
PIRP                        Irp);               // I/O Request Packet

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
UDFCommonClose(
    PIRP_CONTEXT IrpContext,
    PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
UDFTeardownStructures(
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB StartingFcb,
    _In_ BOOLEAN Recursive,      // TRUE if this is a recursive call (for hard links)
    _Out_ PBOOLEAN RemovedStartingFcb
    );

NTSTATUS
UDFPurgeVolume (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb
    );

PFCB
UDFGetNextFcb (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _In_ PVOID *RestartKey
    );

VOID
NTAPI
UDFFspClose(
    _In_opt_ PVCB Vcb
    );
NTSTATUS
UDFFspCloseForFlush(_In_ PVCB Vcb);

//extern VOID UDFRemoveFromDelayedQueue(PtrUDFFCB Fcb);
#define UDFRemoveFromDelayedQueue(Fcb) \
    UDFCloseAllDelayedInDir((Fcb)->Vcb, (Fcb)->FileInfo)

#define UDFRemoveFromSystemDelayedQueue(Fcb) \
    UDFCloseAllSystemDelayedInDir((Fcb)->Vcb, (Fcb)->FileInfo)

/*************************************************************************
* Prototypes for the file dircntrl.cpp
*************************************************************************/
extern NTSTATUS NTAPI UDFDirControl(
PDEVICE_OBJECT          DeviceObject,       // the logical volume device object
PIRP                    Irp);               // I/O Request Packet

extern NTSTATUS NTAPI UDFCommonDirControl(
PIRP_CONTEXT IrpContext,
PIRP                    Irp);

/*************************************************************************
* Prototypes for the file devcntrl.cpp
*************************************************************************/
extern NTSTATUS NTAPI UDFDeviceControl(
PDEVICE_OBJECT              DeviceObject,       // the logical volume device object
PIRP                        Irp);               // I/O Request Packet

NTSTATUS
UDFCommonDevControl(
    PIRP_CONTEXT IrpContext,
    PIRP Irp
    );

extern NTSTATUS NTAPI UDFDevIoctlCompletion(
PDEVICE_OBJECT              PtrDeviceObject,
PIRP                        Irp,
PVOID                       Context);

extern NTSTATUS NTAPI UDFHandleQueryPath(
PVOID                       BufferPointer);

/*************************************************************************
* Prototypes for the file fastio.cpp
*************************************************************************/
extern BOOLEAN NTAPI UDFFastIoCheckIfPossible(
IN PFILE_OBJECT             FileObject,
IN PLARGE_INTEGER           FileOffset,
IN ULONG                    Length,
IN BOOLEAN                  Wait,
IN ULONG                    LockKey,
IN BOOLEAN                  CheckForReadOperation,
OUT PIO_STATUS_BLOCK        IoStatus,
IN PDEVICE_OBJECT           DeviceObject);

FAST_IO_POSSIBLE
NTAPI
UDFIsFastIoPossible(
    IN PFCB Fcb
    );

extern BOOLEAN NTAPI UDFFastIoQueryBasicInfo(
IN PFILE_OBJECT             FileObject,
IN BOOLEAN                  Wait,
OUT PFILE_BASIC_INFORMATION Buffer,
OUT PIO_STATUS_BLOCK        IoStatus,
IN PDEVICE_OBJECT           DeviceObject);

extern BOOLEAN NTAPI UDFFastIoQueryStdInfo(
IN PFILE_OBJECT                FileObject,
IN BOOLEAN                     Wait,
OUT PFILE_STANDARD_INFORMATION Buffer,
OUT PIO_STATUS_BLOCK           IoStatus,
IN PDEVICE_OBJECT              DeviceObject);

extern VOID NTAPI UDFFastIoRelCreateSec(
IN PFILE_OBJECT FileObject);

extern BOOLEAN NTAPI UDFAcqLazyWrite(
IN PVOID   Context,
IN BOOLEAN Wait);

extern VOID NTAPI UDFRelLazyWrite(
IN PVOID Context);

extern BOOLEAN NTAPI UDFAcqReadAhead(
IN PVOID   Context,
IN BOOLEAN Wait);

extern VOID NTAPI UDFRelReadAhead(
IN PVOID Context);

VOID NTAPI UDFDriverUnload(
    IN PDRIVER_OBJECT DriverObject);

extern BOOLEAN NTAPI UDFFastIoQueryNetInfo(
IN PFILE_OBJECT                                 FileObject,
IN BOOLEAN                                      Wait,
OUT struct _FILE_NETWORK_OPEN_INFORMATION*      Buffer,
OUT PIO_STATUS_BLOCK                            IoStatus,
IN PDEVICE_OBJECT                               DeviceObject);

extern BOOLEAN NTAPI UDFFastIoMdlRead(
IN PFILE_OBJECT             FileObject,
IN PLARGE_INTEGER           FileOffset,
IN ULONG                    Length,
IN ULONG                    LockKey,
OUT PMDL*                   MdlChain,
OUT PIO_STATUS_BLOCK        IoStatus,
IN PDEVICE_OBJECT           DeviceObject);

extern BOOLEAN UDFFastIoMdlReadComplete(
IN PFILE_OBJECT             FileObject,
OUT PMDL                    MdlChain,
IN PDEVICE_OBJECT           DeviceObject);

extern BOOLEAN NTAPI UDFFastIoPrepareMdlWrite(
IN PFILE_OBJECT             FileObject,
IN PLARGE_INTEGER           FileOffset,
IN ULONG                    Length,
IN ULONG                    LockKey,
OUT PMDL*                   MdlChain,
OUT PIO_STATUS_BLOCK        IoStatus,
IN PDEVICE_OBJECT           DeviceObject);

extern BOOLEAN NTAPI UDFFastIoMdlWriteComplete(
IN PFILE_OBJECT             FileObject,
IN PLARGE_INTEGER           FileOffset,
OUT PMDL                    MdlChain,
IN PDEVICE_OBJECT           DeviceObject);

NTSTATUS
NTAPI
UDFFastIoAcqModWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER EndingOffset,
    OUT PERESOURCE* ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject);

extern NTSTATUS NTAPI UDFFastIoRelModWrite(
IN PFILE_OBJECT             FileObject,
IN PERESOURCE               ResourceToRelease,
IN PDEVICE_OBJECT           DeviceObject);

extern NTSTATUS NTAPI UDFFastIoAcqCcFlush(
IN PFILE_OBJECT             FileObject,
IN PDEVICE_OBJECT           DeviceObject);

extern NTSTATUS NTAPI UDFFastIoRelCcFlush(
IN PFILE_OBJECT             FileObject,
IN PDEVICE_OBJECT           DeviceObject);

BOOLEAN
NTAPI
UDFFastIoCopyRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NTAPI
UDFFastIoCopyWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

/*************************************************************************
* Prototypes for the file fileinfo.cpp
*************************************************************************/

extern NTSTATUS UDFCommonQueryInfo(
    PIRP_CONTEXT IrpContext,
    PIRP                    Irp);

extern NTSTATUS UDFCommonSetInfo(
PIRP_CONTEXT IrpContext,
PIRP                    Irp);

extern NTSTATUS UDFGetBasicInformation(
    IN PFILE_OBJECT                FileObject,
    IN PFCB                        Fcb,
    IN PFILE_BASIC_INFORMATION     PtrBuffer,
 IN OUT LONG*                      PtrReturnedLength);

extern NTSTATUS UDFGetNetworkInformation(
    IN PFCB                           Fcb,
    IN PFILE_NETWORK_OPEN_INFORMATION PtrBuffer,
 IN OUT PLONG                         PtrReturnedLength);

extern NTSTATUS UDFGetStandardInformation(
    IN PFCB                        Fcb,
    IN PFILE_STANDARD_INFORMATION  PtrBuffer,
 IN OUT PLONG                      PtrReturnedLength);

NTSTATUS
UDFGetInternalInformation(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _Out_ PFILE_INTERNAL_INFORMATION Buffer,
    _Inout_ PLONG ReturnedLength
    );

extern NTSTATUS UDFGetEaInformation(
    PIRP_CONTEXT IrpContext,
    IN PFCB                 Fcb,
    IN PFILE_EA_INFORMATION PtrBuffer,
 IN OUT PLONG               PtrReturnedLength);

extern NTSTATUS UDFGetFullNameInformation(
    IN PFILE_OBJECT                FileObject,
    IN PFILE_NAME_INFORMATION      PtrBuffer,
 IN OUT PLONG                      PtrReturnedLength);

extern NTSTATUS UDFGetAltNameInformation(
    IN PFCB                        Fcb,
    IN PFILE_NAME_INFORMATION      PtrBuffer,
 IN OUT PLONG                      PtrReturnedLength);

extern NTSTATUS UDFGetPositionInformation(
    IN PFILE_OBJECT               FileObject,
    IN PFILE_POSITION_INFORMATION PtrBuffer,
 IN OUT PLONG                     PtrReturnedLength);

NTSTATUS
UDFGetFileStreamInformation(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_STREAM_INFORMATION Buffer,
    IN OUT PULONG ReturnedLength
    );

extern NTSTATUS UDFSetBasicInformation(
    IN PIRP_CONTEXT                IrpContext,
    IN PFCB                        Fcb,
    IN PCCB                        Ccb,
    IN PFILE_OBJECT                FileObject,
    IN PFILE_BASIC_INFORMATION     PtrBuffer);

NTSTATUS
UDFMarkStreamsForDeletion(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB           Vcb,
    IN PFCB           Fcb,
    IN BOOLEAN        ForDel
    );

NTSTATUS
UDFSetDispositionInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN PFILE_DISPOSITION_INFORMATION Buffer
    );

NTSTATUS
UDFSetAllocationInfo(
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFILE_ALLOCATION_INFORMATION Buffer);

NTSTATUS
UDFSetEndOfFileInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PFILE_END_OF_FILE_INFORMATION PtrBuffer
    );

BOOLEAN
UDFCheckDirOpenHandles(
    IN PFCB DirectoryFcb
    );

NTSTATUS
UDFSetRenameInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN PFILE_OBJECT FileObject,
    IN PFILE_RENAME_INFORMATION PtrBuffer
    );

NTSTATUS
UDFStoreFileId(
    IN PVCB Vcb,
    IN PCCB Ccb,
    IN PUDF_FILE_INFO fi,
    IN FILE_ID FileId
    );

NTSTATUS UDFRemoveFileId(
    IN PVCB Vcb,
    IN FILE_ID FileId
    );

#define UDFRemoveFileId__(Vcb, fi) \
    UDFRemoveFileId(Vcb, UDFGetNTFileId(Vcb, fi));

extern VOID UDFReleaseFileIdCache(
    IN PVCB Vcb);

NTSTATUS
UDFGetOpenParamsByFileId(
    IN PVCB Vcb,
    IN FILE_ID FileId,
    OUT PUNICODE_STRING* FName,
    OUT BOOLEAN* CaseSens
    );

NTSTATUS
UDFHardLink(
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB Fcb1,
    IN PCCB Ccb1,
    IN PFILE_OBJECT FileObject1,   // Source File
    IN PFILE_LINK_INFORMATION PtrBuffer
    );

/*************************************************************************
* Prototypes for the file flush.cpp
*************************************************************************/
extern NTSTATUS NTAPI UDFFlushBuffers(
PDEVICE_OBJECT    DeviceObject,       // the logical volume device object
PIRP              Irp);               // I/O Request Packet

extern NTSTATUS UDFCommonFlush(
PIRP_CONTEXT IrpContext,
PIRP                        Irp);

ULONG UDFFlushAFile(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCCB Ccb,
    OUT PIO_STATUS_BLOCK PtrIoStatus,
    IN ULONG FlushFlags
    );

ULONG
UDFFlushADirectory(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FI,
    OUT PIO_STATUS_BLOCK PtrIoStatus,
    ULONG FlushFlags
    );

NTSTATUS
UDFFlushVolume(
    PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    ULONG FlushFlags
    );

extern NTSTATUS NTAPI UDFFlushCompletion(
PDEVICE_OBJECT              PtrDeviceObject,
PIRP                        Irp,
PVOID                       Context);

extern BOOLEAN UDFFlushIsBreaking(
IN PVCB         Vcb,
IN ULONG        FlushFlags);

extern VOID UDFFlushTryBreak(
IN PVCB         Vcb);

/*************************************************************************
* Prototypes for the file fscntrl.cpp
*************************************************************************/

extern NTSTATUS NTAPI UDFFSControl(
PDEVICE_OBJECT      DeviceObject,
PIRP                Irp);

NTSTATUS
UDFCommonFsControl(
    PIRP_CONTEXT IrpContext,
    PIRP Irp
    );

NTSTATUS
UDFUserFsCtrlRequest(
    PIRP_CONTEXT IrpContext,
    PIRP Irp);

extern NTSTATUS NTAPI UDFMountVolume(
PIRP_CONTEXT IrpContext,
PIRP Irp);

NTSTATUS
UDFUnlockVolumeInternal (
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject OPTIONAL
    );

extern VOID UDFScanForDismountedVcb (IN PIRP_CONTEXT IrpContext);

NTSTATUS
UDFCompleteMount(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

NTSTATUS
UDFRecoverDirtyVolumeBeforeLoad(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
UDFCloseResidual(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

extern VOID     UDFCleanupVCB(IN PVCB Vcb);

extern NTSTATUS UDFIsVolumeMounted(IN PIRP_CONTEXT IrpContext,
                                   IN PIRP Irp);

extern NTSTATUS UDFIsVolumeDirty(IN PIRP_CONTEXT IrpContext,
                          IN PIRP Irp);

NTSTATUS
UDFLockVolume(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
UDFUnlockVolume(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
_Requires_lock_held_(Vcb->VcbResource)
NTSTATUS
UDFLockVolumeInternal (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PVCB Vcb,
    _In_opt_ PFILE_OBJECT FileObject
    );

NTSTATUS
UDFIsPathnameValid(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

extern NTSTATUS UDFDismountVolume(IN PIRP_CONTEXT IrpContext,
                                  IN PIRP Irp);

extern NTSTATUS UDFGetVolumeBitmap(IN PIRP_CONTEXT IrpContext,
                                   IN PIRP Irp);

extern NTSTATUS UDFGetRetrievalPointers(IN PIRP_CONTEXT IrpContext,
                                        IN PIRP Irp);

NTSTATUS
UDFCommonPnp(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

/*************************************************************************
* Prototypes for the file LockCtrl.cpp
*************************************************************************/

extern NTSTATUS NTAPI UDFLockControl(
    IN PDEVICE_OBJECT DeviceObject,       // the logical volume device object
    IN PIRP           Irp);               // I/O Request Packet

extern NTSTATUS NTAPI UDFCommonLockControl(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP             Irp);

BOOLEAN
NTAPI
UDFFastLock(
    IN PFILE_OBJECT           FileObject,
    IN PLARGE_INTEGER         FileOffset,
    IN PLARGE_INTEGER         Length,
    PEPROCESS                 ProcessId,
    ULONG                     Key,
    BOOLEAN                   FailImmediately,
    BOOLEAN                   ExclusiveLock,
    OUT PIO_STATUS_BLOCK      IoStatus,
    IN PDEVICE_OBJECT         DeviceObject);

extern BOOLEAN NTAPI UDFFastUnlockSingle(
    IN PFILE_OBJECT           FileObject,
    IN PLARGE_INTEGER         FileOffset,
    IN PLARGE_INTEGER         Length,
    PEPROCESS                 ProcessId,
    ULONG                     Key,
    OUT PIO_STATUS_BLOCK      IoStatus,
    IN PDEVICE_OBJECT         DeviceObject);

extern BOOLEAN NTAPI UDFFastUnlockAll(
    IN PFILE_OBJECT           FileObject,
    PEPROCESS                 ProcessId,
    OUT PIO_STATUS_BLOCK      IoStatus,
    IN PDEVICE_OBJECT         DeviceObject);

BOOLEAN
NTAPI
UDFFastUnlockAllByKey(
    _In_ PFILE_OBJECT FileObject,
    _In_ PVOID ProcessId,
    _In_ ULONG Key,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject
    );

/*************************************************************************
* Prototypes for the file misc.cpp
*************************************************************************/

_IRQL_requires_max_(APC_LEVEL)
__drv_dispatchType(DRIVER_DISPATCH)
__drv_dispatchType(IRP_MJ_CREATE)
__drv_dispatchType(IRP_MJ_CLOSE)
__drv_dispatchType(IRP_MJ_READ)
__drv_dispatchType(IRP_MJ_WRITE)
__drv_dispatchType(IRP_MJ_QUERY_INFORMATION)
__drv_dispatchType(IRP_MJ_SET_INFORMATION)
__drv_dispatchType(IRP_MJ_QUERY_VOLUME_INFORMATION)
__drv_dispatchType(IRP_MJ_DIRECTORY_CONTROL)
__drv_dispatchType(IRP_MJ_FILE_SYSTEM_CONTROL)
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
__drv_dispatchType(IRP_MJ_LOCK_CONTROL)
__drv_dispatchType(IRP_MJ_CLEANUP)
__drv_dispatchType(IRP_MJ_PNP)
__drv_dispatchType(IRP_MJ_SHUTDOWN)
NTSTATUS
NTAPI
UDFFsdDispatch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    );

LONG
UDFExceptionFilter(
    PIRP_CONTEXT IrpContext,
    PEXCEPTION_POINTERS ExceptionPointer
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
UDFProcessException(
    _In_opt_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp,
    _In_ NTSTATUS ExceptionCode
    );

extern PtrUDFObjectName UDFAllocateObjectName(
VOID);

extern VOID UDFReleaseObjectName(
PtrUDFObjectName            PtrObjectName);

PCCB
UDFCreateCcb(
    );

VOID 
UDFDeallocateCcb(
    PCCB Ccb
    );

VOID
UDFDeleteCcb(
    PCCB Ccb
    );

// Bitmap cache stream
NTSTATUS
UDFCreateBitmapStream(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG BitmapPsn,
    IN ULONG BitmapLength
    );

VOID
UDFDeleteBitmapStream(
    IN PVCB Vcb
    );

// Per-page bitmap cache access (alloc.cpp)
VOID
UDFPinBitmapPage(
    IN PVCB Vcb,
    IN ULONG Lbn
    );

VOID
UDFUnpinBitmapPage(
    IN PVCB Vcb
    );

VOID
UDFDirtyBitmapPage(
    IN PVCB Vcb
    );

BOOLEAN
UDFIsBitmapBitFree(
    IN PVCB Vcb,
    IN ULONG Lbn
    );

SIZE_T
UDFGetCachedBitmapLen(
    IN PVCB Vcb,
    IN ULONG Start,
    IN ULONG Limit
    );

// prefxsup.cpp - LCB functions
PLCB
UDFInsertPrefix(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN PFCB ChildFcb,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN PUNICODE_STRING CaseFileName OPTIONAL,
    IN PUNICODE_STRING ShortName OPTIONAL,
    IN ULONGLONG InitialOffset
    );

VOID
UDFRemovePrefix(
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb
    );

// Insert LCB into parent FCB's splay trees (ExactCase, IgnoreCase, ShortName).
// Caller must hold ParentFcb exclusive.
VOID
UdfInsertNameLinks(
    IN PFCB ParentFcb,
    IN PLCB Lcb
    );

// Remove LCB from parent FCB's splay trees only (not from linked lists).
// Caller must hold ParentFcb exclusive.
VOID
UdfRemoveNameLinks(
    IN PFCB ParentFcb,
    IN PLCB Lcb
    );

PLCB
UDFFindPrefix(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN PFCB ChildFcb
    );

PLCB
UDFAcquirePrefix(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN PFCB ChildFcb,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN PUNICODE_STRING CaseFileName OPTIONAL,
    IN PUNICODE_STRING ShortName OPTIONAL,
    IN ULONGLONG InitialOffset
    );

VOID
UDFReleasePrefix(
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb
    );

NTSTATUS
UDFBuildFullPathFromLcb(
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    OUT PUNICODE_STRING FullPath,
    IN BOOLEAN CachePath
    );

BOOLEAN
UDFReleasePrefixImmediate(
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN BOOLEAN CloseParentFileInfo
    );

NTSTATUS
UDFRenameMovePrefix(
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN PUNICODE_STRING NewName,
    IN PFCB NewParentFcb OPTIONAL
    );

PLCB
UDFFindPathPrefix(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB StartFcb,
    IN BOOLEAN IgnoreCase,
    IN OUT PFCB *CurrentFcb,
    IN OUT PUNICODE_STRING RemainingName,
    OUT PBOOLEAN ShortNameMatch
    );

PFCB
UDFLookupFcbTable (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _In_ FILE_ID FileId
    );

PFCB
UDFCreateFcb (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ FILE_ID FileId,
    _In_ NODE_TYPE_CODE NodeTypeCode,
    _Out_opt_ PBOOLEAN FcbExisted
    );

VOID
UDFDeleteFcb(
    _In_opt_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb
    );

VOID
UDFInsertFcbIntoTable(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb
    );

_Ret_valid_ PIRP_CONTEXT
UDFCreateIrpContext(
    _In_ PIRP Irp,
    _In_ BOOLEAN Wait
    );

VOID
UDFCleanupIrpContext(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ BOOLEAN Post
    );

VOID
UDFCompleteRequest(
    _Inout_opt_ PIRP_CONTEXT IrpContext OPTIONAL,
    _Inout_opt_ PIRP Irp OPTIONAL,
    _In_ NTSTATUS Status
    );

VOID
UDFAddToWorkque(
    PIRP_CONTEXT IrpContext,
    PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
UDFFsdPostRequest(
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
);

VOID
NTAPI
UDFFspDispatch(
    PVOID Context
    );

VOID
UDFInitializeVCB(
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PVCB Vcb,
    _In_ PDEVICE_OBJECT TargetDeviceObject,
    _In_ PVPB Vpb,
    _In_ PDISK_GEOMETRY DiskGeometry,
    _In_ ULONG MediaChangeCount
    );

VOID
UDFReadRegKeys(
    PVCB Vcb,
    BOOLEAN Update,
    BOOLEAN UseCfg);

extern ULONG UDFGetRegParameter(
    IN PVCB Vcb,
    IN PCWSTR Name,
    IN ULONG DefValue );

VOID
UDFDeleteVCB(
    PIRP_CONTEXT IrpContext,
    PVCB Vcb
    );

extern VOID UDFInitializeStackIrpContextFromLite(
    OUT PIRP_CONTEXT IrpContext,
    IN PIRP_CONTEXT_LITE IrpContextLite);

extern ULONG
UDFIsResourceAcquired(
    IN PERESOURCE Resource
    );

extern BOOLEAN UDFAcquireResourceExclusiveWithCheck(
    IN PERESOURCE Resource
    );

extern BOOLEAN UDFAcquireResourceSharedWithCheck(
    IN PERESOURCE Resource
    );

extern NTSTATUS NTAPI UDFFilterCallbackAcquireForCreateSection(
    IN PFS_FILTER_CALLBACK_DATA CallbackData,
    IN PVOID *CompletionContext
    );

_When_(RaiseOnError || return, _At_(Fcb->FileLock, _Post_notnull_))
_When_(RaiseOnError, _At_(IrpContext, _Pre_notnull_))
BOOLEAN
UDFCreateFileLock(
    _In_opt_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB Fcb,
    _In_ BOOLEAN RaiseOnError
);

/*************************************************************************
* Prototypes for the file NameSup.cpp
*************************************************************************/

#include "namesup.h"

/*************************************************************************
* Prototypes for the file Pnp.cpp
*************************************************************************/
NTSTATUS
NTAPI
UDFPnp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

/*************************************************************************
* Prototypes for the file read.cpp
*************************************************************************/
extern NTSTATUS NTAPI UDFRead(
    PDEVICE_OBJECT              DeviceObject,       // the logical volume device object
    PIRP                        Irp);               // I/O Request Packet

extern VOID NTAPI UDFStackOverflowRead(
    IN PVOID Context,
    IN PKEVENT Event);

extern NTSTATUS UDFCommonRead(
    PIRP_CONTEXT IrpContext,
    PIRP             Irp);

extern PVOID UDFMapUserBuffer(
    PIRP Irp);

NTSTATUS
UDFLockUserBuffer(
    PIRP_CONTEXT IrpContext,
    ULONG BufferLength,
    LOCK_OPERATION LockOperation
    );

extern NTSTATUS UDFUnlockCallersBuffer(
    PIRP_CONTEXT IrpContext,
    PIRP    Irp,
    PVOID   SystemBuffer);

NTSTATUS
UDFCompleteMdl(
    PIRP_CONTEXT IrpContext,
    PIRP Irp
    );

NTSTATUS
UDFCheckAccessRights(
    PFILE_OBJECT FileObject,
    PACCESS_STATE AccessState,
    PFCB         Fcb,
    PCCB         Ccb,
    ACCESS_MASK  DesiredAccess,
    USHORT       ShareAccess);

extern NTSTATUS
UDFSetAccessRights(
    PFILE_OBJECT FileObject,
    PACCESS_STATE AccessState,
    PFCB         Fcb,
    PCCB         Ccb,
    ACCESS_MASK  DesiredAccess,
    USHORT       ShareAccess);

/*************************************************************************
* Prototypes for the file Shutdown.cpp
*************************************************************************/

NTSTATUS
UDFCommonShutdown(
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

/*************************************************************************
* Prototypes for the file UDFinit.cpp
*************************************************************************/
NTSTATUS NTAPI DriverEntry(
PDRIVER_OBJECT              DriverObject,       // created by the I/O sub-system
PUNICODE_STRING             RegistryPath);      // path to the registry key

extern VOID NTAPI UDFInitializeFunctionPointers(
PDRIVER_OBJECT              DriverObject);      // created by the I/O sub-system

/*************************************************************************
* Prototypes for the file verify.cpp
*************************************************************************/

VOID
UDFVerifyVcb(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

NTSTATUS
UDFVerifyFcbOperation(
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PFCB Fcb,
    IN PCCB Ccb
    );

NTSTATUS
UDFVerifyVolume(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
UDFPerformVerify(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PDEVICE_OBJECT DeviceToVerify
    );

BOOLEAN
UDFCheckForDismount(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN Force
    );

BOOLEAN
UDFDismountVcb(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN IN BOOLEAN FlushBeforeDismount
    );

NTSTATUS
UDFCompareVcb(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB OldVcb,
    IN PVCB NewVcb,
    IN BOOLEAN PhysicalOnly
    );

/*************************************************************************
* Prototypes for the file VolInfo.cpp
*************************************************************************/
extern NTSTATUS NTAPI UDFQueryVolInfo(PDEVICE_OBJECT DeviceObject,
                                      PIRP Irp);

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
UDFCommonQueryVolInfo(
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

NTSTATUS
NTAPI
UDFSetVolInfo(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
UDFCommonSetVolInfo(
    PIRP_CONTEXT IrpContext,
    PIRP Irp
    );

/*************************************************************************
* Prototypes for the file write.cpp
*************************************************************************/
extern NTSTATUS NTAPI UDFWrite(
PDEVICE_OBJECT              DeviceObject,       // the logical volume device object
PIRP                        Irp);               // I/O Request Packet

extern NTSTATUS UDFCommonWrite(
PIRP_CONTEXT IrpContext,
PIRP                        Irp);

extern VOID NTAPI UDFDeferredWriteCallBack (
VOID                        *Context1,          // Should be IrpContext
VOID                        *Context2);         // Should be Irp

extern VOID UDFPurgeCacheEx_(
PFCB                        Fcb,
LONGLONG                    Offset,
LONGLONG                    Length,
//#ifndef ALLOW_SPARSE
BOOLEAN                     CanWait,
//#endif ALLOW_SPARSE
PVCB                        Vcb,
PFILE_OBJECT                FileObject
);

extern VOID UDFSetModified(
    IN PVCB        Vcb
);

extern VOID UDFPreClrModified(
    IN PVCB        Vcb
);

extern VOID UDFClrModified(
    IN PVCB        Vcb
);

/*#ifdef ALLOW_SPARSE
  #define UDFZeroDataEx(Fcb, Offset, Length, CanWait) \
      UDFPurgeCacheEx_(Fcb, Offset, Length)
  #define UDFPurgeCacheEx(Fcb, Offset, Length, CanWait) \
      UDFPurgeCacheEx_(Fcb, Offset, Length)
#else // ALLOW_SPARSE*/
  #define UDFZeroDataEx(Fcb, Offset, Length, CanWait, Vcb, FileObject) \
      UDFPurgeCacheEx_(Fcb, Offset, Length, CanWait, Vcb, FileObject)
  #define UDFPurgeCacheEx(Fcb, Offset, Length, CanWait, Vcb, FileObject) \
      UDFPurgeCacheEx_(Fcb, Offset, Length, CanWait, Vcb, FileObject)
//#endif //ALLOW_SPARSE

BOOLEAN
UDFZeroData (
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN ULONG StartingZero,
    IN ULONG ByteCount,
    IN BOOLEAN CanWait
    );

NTSTATUS
UDFToggleMediaEjectDisable (
    IN PVCB Vcb,
    IN BOOLEAN PreventRemoval
    );

NTSTATUS
UDFHijackIrpAndFlushDevice (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp,
    _In_ PDEVICE_OBJECT TargetDeviceObject
    );

BOOLEAN
UDFMarkDevForVerifyIfVcbMounted(
    IN PVCB Vcb
    );

//
//  BOOLEAN
//  UdfDeviceIsFsdo(
//      IN PDEVICE_OBJECT D
//      );
//
//  Evaluates to TRUE if the supplied device object is one of the file system devices
//  we created at initialisation.
//

#define UdfDeviceIsFsdo(D)  (((D) == UdfData.UDFDeviceObject_CD) || ((D) == UdfData.UDFDeviceObject_HDD))

//
//  The following macro is used by the dispatch routines to determine if
//  an operation is to be done with or without Write Through.
//
//      BOOLEAN
//      IsFileWriteThrough (
//          IN PFILE_OBJECT FileObject,
//          IN PVCB Vcb
//          );
//

#define IsFileWriteThrough(FO,VCB) (             \
    BooleanFlagOn((FO)->Flags, FO_WRITE_THROUGH) \
)

#define AssertVerifyDeviceIrp(I)                                                    \
    NT_ASSERT( (I) == NULL ||                                                       \
            !(((I)->IoStatus.Status) == STATUS_VERIFY_REQUIRED &&                   \
              ((I)->Tail.Overlay.Thread == NULL ||                                  \
                IoGetDeviceToVerify( (I)->Tail.Overlay.Thread ) == NULL )));

//  Macros to abstract device verify flag changes.

#define UDFUpdateMediaChangeCount( V, C)  (V)->MediaChangeCount = (C)
#define UDFUpdateVcbCondition( V, C)      (V)->VcbCondition = (C)

#define UDFMarkRealDevForVerify( DO)  SetFlag( (DO)->Flags, DO_VERIFY_VOLUME)
                                     
#define UDFMarkRealDevVerifyOk( DO)   ClearFlag( (DO)->Flags, DO_VERIFY_VOLUME)

#define UDFRealDevNeedsVerify( DO)    BooleanFlagOn( (DO)->Flags, DO_VERIFY_VOLUME)

#define UDFLockVcb(IC,V)                                                                \
    ASSERT(KeAreApcsDisabled());                                                        \
    ExAcquireFastMutexUnsafe( &(V)->VcbMutex );                                         \
    (V)->VcbLockThread = PsGetCurrentThread()

#define UDFUnlockVcb(IC,V)                                                              \
    (V)->VcbLockThread = NULL;                                                          \
    ExReleaseFastMutexUnsafe( &(V)->VcbMutex )

#define UDFLockFcbTable(IC,V)                                                           \
    ASSERT(KeAreApcsDisabled());                                                        \
    ExAcquireFastMutexUnsafe( &(V)->FcbTableMutex );                                    \
    (V)->FcbTableLockThread = PsGetCurrentThread()

#define UDFUnlockFcbTable(IC,V)                                                         \
    (V)->FcbTableLockThread = NULL;                                                     \
    ExReleaseFastMutexUnsafe( &(V)->FcbTableMutex )

#define UDFLockFcb(IC,F) {                                                              \
    PVOID _CurrentThread = PsGetCurrentThread();                                        \
    if (_CurrentThread != (F)->FcbLockThread) {                                         \
        ASSERT(KeAreApcsDisabled());                                                    \
        ExAcquireFastMutexUnsafe( &(F)->FcbNonpaged->FcbFastMutex );                   \
        ASSERT( (F)->FcbLockCount == 0 );                                               \
        (F)->FcbLockThread = _CurrentThread;                                            \
    }                                                                                   \
    (F)->FcbLockCount += 1;                                                             \
}

#define UDFUnlockFcb(IC,F) {                                                            \
    ASSERT( PsGetCurrentThread() == (F)->FcbLockThread);                                \
    (F)->FcbLockCount -= 1;                                                             \
    if ((F)->FcbLockCount == 0) {                                                       \
        (F)->FcbLockThread = NULL;                                                      \
        ExReleaseFastMutexUnsafe( &(F)->FcbNonpaged->FcbFastMutex );                   \
    }                                                                                   \
}

#define UDFIncrementCleanupCounts(IC,F) {        \
    ASSERT_LOCKED_VCB( (F)->Vcb );              \
    (F)->FcbCleanup += 1;                       \
    (F)->Vcb->VcbCleanup += 1;                  \
}

#define UDFDecrementCleanupCounts(IC,F) {        \
    ASSERT_LOCKED_VCB( (F)->Vcb );              \
    (F)->FcbCleanup -= 1;                       \
    (F)->Vcb->VcbCleanup -= 1;                  \
}

#define UDFIncrementReferenceCounts(IC,F,C,UC) { \
    ASSERT_LOCKED_VCB( (F)->Vcb );              \
    (F)->FcbReference += (C);                   \
    (F)->FcbUserReference += (UC);              \
    (F)->Vcb->VcbReference += (C);              \
    (F)->Vcb->VcbUserReference += (UC);         \
}

#define UDFDecrementReferenceCounts(IC,F,C,UC) { \
    ASSERT_LOCKED_VCB( (F)->Vcb );              \
    (F)->FcbReference -= (C);                   \
    (F)->FcbUserReference -= (UC);              \
    (F)->Vcb->VcbReference -= (C);              \
    (F)->Vcb->VcbUserReference -= (UC);         \
}

#define UDFLockUdfData()                                                                \
    ASSERT(KeAreApcsDisabled());                                                        \
    ExAcquireFastMutexUnsafe(&UdfData.UdfDataMutex);                                    \
    UdfData.UdfDataLockThread = PsGetCurrentThread()

#define UDFUnlockUdfData()                                                              \
    UdfData.UdfDataLockThread = NULL;                                                   \
    ExReleaseFastMutexUnsafe(&UdfData.UdfDataMutex)

enum TYPE_OF_ACQUIRE {
    
    AcquireExclusive,
    AcquireShared,
    AcquireSharedStarveExclusive

};

_Requires_lock_held_(_Global_critical_region_)
_When_(Type == AcquireExclusive && return != FALSE, _Acquires_exclusive_lock_(*Resource))
_When_(Type == AcquireShared && return != FALSE, _Acquires_shared_lock_(*Resource))
_When_(Type == AcquireSharedStarveExclusive && return != FALSE, _Acquires_shared_lock_(*Resource))
_When_(IgnoreWait == FALSE, _Post_satisfies_(return == TRUE))
BOOLEAN
UDFAcquireResource(
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PERESOURCE Resource,
    _In_ BOOLEAN IgnoreWait,
    _In_ enum TYPE_OF_ACQUIRE Type
    );

#define UDFAcquireVcbExclusive(IC,V,I)                                                  \
    UDFAcquireResource( (IC), &(V)->VcbResource, (I), AcquireExclusive )

#define UDFAcquireVcbShared(IC,V,I)                                                     \
    UDFAcquireResource((IC), &(V)->VcbResource, (I), AcquireShared)

#define UDFReleaseVcb(IC,V)                                                             \
    ExReleaseResourceLite(&(V)->VcbResource)

#define UDFAcquireUdfData(IC)                                                           \
    ExAcquireResourceExclusiveLite(&UdfData.GlobalDataResource, TRUE)

#define UDFReleaseUdfData(IC)                                                           \
    ExReleaseResourceLite(&UdfData.GlobalDataResource)

#define UDFAcquireFcbExclusive(IC,F,I)                                                  \
    UDFAcquireResource((IC), &(F)->FcbNonpaged->FcbResource, (I), AcquireExclusive)

#define UDFAcquireFcbShared(IC,F,I)                                                     \
    UDFAcquireResource((IC), &(F)->FcbNonpaged->FcbResource, (I), AcquireShared)

#define UDFAcquireFcbSharedStarveExclusive(IC,F,I)                                      \
    UDFAcquireResource((IC), &(F)->FcbNonpaged->FcbResource, (I), AcquireSharedStarveExclusive)

#define UDFReleaseFcb(IC,F)                                                             \
    ExReleaseResourceLite(&(F)->FcbNonpaged->FcbResource)

#define UDFAcquirePagingIoExclusive(IC,F)                                               \
    UDFAcquireResource((IC), (F)->Header.PagingIoResource, FALSE, AcquireExclusive)

#define UDFReleasePagingIo(IC,F)                                                        \
    ExReleaseResourceLite((F)->Header.PagingIoResource)

inline
ULONG
UDFHighBit(
    ULONG Word
    )
{
    ULONG Index;
    
    if (_BitScanReverse(&Index, Word)) {
        return Index;
    }
    return 0;
}

#define LlBytesFromSectors(V, L) (                                              \
    Int64ShllMod32( (ULONGLONG)(L), ((V)->SectorShift) )                        \
)

#define LlSectorsFromBytes(V, L) (                                              \
    Int64ShrlMod32( (ULONGLONG)(L), ((V)->SectorShift) )                        \
)

#define SectorSize(V) ((V)->SectorSize)

inline
ULONG
SectorAlign(
    PVCB Vcb,
    ULONG Length
) {

    return (Length + (Vcb->SectorSize - 1)) & ~(Vcb->SectorSize - 1);
}

inline
ULONGLONG
LlSectorAlign( 
    PVCB Vcb, 
    ULONGLONG Length
) {

    return (Length + (Vcb->SectorSize - 1)) & ~(ULONGLONG)(Vcb->SectorSize - 1);
}

VOID
UDFSetThreadContext(
    _Inout_ PIRP_CONTEXT IrpContext,
    _In_ PTHREAD_CONTEXT ThreadContext
    );

#define UDFRestoreThreadContext(IC)                             \
    (IC)->ThreadContext->Udfs = 0;                              \
    IoSetTopLevelIrp( (IC)->ThreadContext->SavedTopLevelIrp );  \
    (IC)->ThreadContext = NULL


inline
BOOLEAN UdfIsExtendedFESupported(
    _In_ PVCB Vcb
)
{
    return Vcb->NSRDesc == VRS_NSR03_FOUND;
}

inline
BOOLEAN UDFIsStreamsSupported(
    _In_ PVCB Vcb
)
{
    return Vcb->UdfRevision >= 0x0200;
}

VOID
UDFFinishIoAtEof(
    IN PFCB Fcb
    );

BOOLEAN
UDFWaitForIoAtEof(
    IN PFCB Fcb,
    IN LONGLONG FileOffset,
    IN ULONG Length
    );

VOID
UDFPrePostIrp(
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

VOID
UDFNotifyReportChange(
    PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    PFCB Fcb,
    ULONG Filter,
    ULONG Action,
    PLCB Lcb,
    PFILE_OBJECT FileObject
    );

NTSTATUS
UDFNonCachedIo(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ LONGLONG StartingOffset,
    _In_ ULONG ByteCount
    );

NTSTATUS
NTAPI
UDFHijackCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Contxt
    );

NTSTATUS
UDFPerformDevIoCtrl(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID InputBuffer ,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer ,
    IN ULONG OutputBufferLength,
    IN BOOLEAN OverrideVerify,
    OUT PIO_STATUS_BLOCK Iosb OPTIONAL);

NTSTATUS
UDFPreparePagingFile(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb
    );

NTSTATUS
UDFPagingFileIo(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ LONGLONG StartingOffset,
    _In_ ULONG ByteCount
    );

#endif  // _UDF_PROTOS_H_
