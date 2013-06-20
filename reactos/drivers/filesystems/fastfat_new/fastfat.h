#include <ntifs.h>
#include <bugcodes.h>
#include <ntdddisk.h>
#include <debug.h>
#include <pseh/pseh2.h>

#include "fullfat.h"

#include <fat.h>
#include <fatstruc.h>

#define Add2Ptr(P,I,T) ((T)((PUCHAR)(P) + (I)))
#define PtrOffset(B,O) ((ULONG)((ULONG_PTR)(O) - (ULONG_PTR)(B)))

#define TAG_CCB  'BCCV'
#define TAG_FCB  'BCFV'
#define TAG_IRP  'PRIV'
#define TAG_VFAT 'TAFV'
#define TAG_FSD_CLOSE_CONTEXT 'CLCV'


/* Global resource acquire/release */
#define FatAcquireExclusiveGlobal(IrpContext) \
( \
    ExAcquireResourceExclusiveLite(&FatGlobalData.Resource, \
                                   (IrpContext)->Flags & IRPCONTEXT_CANWAIT) \
)

#define FatAcquireSharedGlobal(IrpContext) \
( \
    ExAcquireResourceSharedLite(&FatGlobalData.Resource, \
                                (IrpContext)->Flags & IRPCONTEXT_CANWAIT) \
)

#define FatReleaseGlobal(IrpContext) \
{ \
    ExReleaseResourceLite(&(FatGlobalData.Resource)); \
}

#define FatIsFastIoPossible(FCB) ((BOOLEAN)                                                \
    (((FCB)->Condition != FcbGood || !FsRtlOplockIsFastIoPossible(&(FCB)->Fcb.Oplock)) ?   \
        FastIoIsNotPossible                                                                \
    :                                                                                      \
        (!FsRtlAreThereCurrentFileLocks(&(FCB)->Fcb.Lock) &&                               \
         ((FCB)->OutstandingAsyncWrites == 0) &&                                           \
         !FlagOn((FCB)->Vcb->State, VCB_STATE_FLAG_WRITE_PROTECTED) ?                      \
            FastIoIsPossible                                                               \
        :                                                                                  \
            FastIoIsQuestionable                                                           \
        )                                                                                  \
    )                                                                                      \
)

#define IsFileObjectReadOnly(FO) (!((FO)->WriteAccess | (FO)->DeleteAccess))
#define IsFileDeleted(FCB) (FlagOn((FCB)->State, FCB_STATE_DELETE_ON_CLOSE) && ((FCB)->UncleanCount == 0))

BOOLEAN
FORCEINLINE
FatIsIoRangeValid(IN LARGE_INTEGER Start, IN ULONG Length)
{
    /* Check if it's more than 32bits, or if the length causes 32bit overflow.
       FAT-specific! */

    return !(Start.HighPart || Start.LowPart + Length < Start.LowPart);
}


NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToCountedOemString(
    IN OUT POEM_STRING DestinationString,
    IN PCUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
);


/*  ------------------------------------------------------  shutdown.c  */

DRIVER_DISPATCH FatShutdown;
NTSTATUS NTAPI
FatShutdown(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  --------------------------------------------------------  volume.c  */

NTSTATUS NTAPI
FatQueryVolumeInfo(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS NTAPI
FatSetVolumeInfo(PDEVICE_OBJECT DeviceObject, PIRP Irp);

VOID NTAPI
FatReadStreamFile(PVCB Vcb,
                  ULONGLONG ByteOffset,
                  ULONG ByteSize,
                  PBCB *Bcb,
                  PVOID *Buffer);

BOOLEAN
NTAPI
FatCheckForDismount(IN PFAT_IRP_CONTEXT IrpContext,
                    PVCB Vcb,
                    IN BOOLEAN Force);

/*  -----------------------------------------------------------  dir.c  */

NTSTATUS NTAPI
FatDirectoryControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

VOID NTAPI
FatCreateRootDcb(IN PFAT_IRP_CONTEXT IrpContext,
                 IN PVCB Vcb);

PFCB NTAPI
FatCreateDcb(IN PFAT_IRP_CONTEXT IrpContext,
             IN PVCB Vcb,
             IN PFCB ParentDcb,
             IN FF_FILE *FileHandle);

IO_STATUS_BLOCK NTAPI
FatiOpenExistingDcb(IN PFAT_IRP_CONTEXT IrpContext,
                    IN PFILE_OBJECT FileObject,
                    IN PVCB Vcb,
                    IN PFCB Dcb,
                    IN PACCESS_MASK DesiredAccess,
                    IN USHORT ShareAccess,
                    IN ULONG CreateDisposition,
                    IN BOOLEAN NoEaKnowledge,
                    IN BOOLEAN DeleteOnClose);

/*  --------------------------------------------------------  create.c  */

IO_STATUS_BLOCK
NTAPI
FatiOverwriteFile(PFAT_IRP_CONTEXT IrpContext,
                  PFILE_OBJECT FileObject,
                  PFCB Fcb,
                  ULONG AllocationSize,
                  PFILE_FULL_EA_INFORMATION EaBuffer,
                  ULONG EaLength,
                  UCHAR FileAttributes,
                  ULONG CreateDisposition,
                  BOOLEAN NoEaKnowledge);

NTSTATUS NTAPI
FatCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);


/*  ---------------------------------------------------------  close.c  */

NTSTATUS NTAPI
FatClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  -------------------------------------------------------  cleanup.c  */

NTSTATUS NTAPI
FatCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ---------------------------------------------------------  fastio.c  */

VOID
FatInitFastIoRoutines(PFAST_IO_DISPATCH FastIoDispatch);

BOOLEAN NTAPI
FatAcquireForLazyWrite(IN PVOID Context,
                        IN BOOLEAN Wait);

VOID NTAPI
FatReleaseFromLazyWrite(IN PVOID Context);

BOOLEAN NTAPI
FatAcquireForReadAhead(IN PVOID Context,
                        IN BOOLEAN Wait);

VOID NTAPI
FatReleaseFromReadAhead(IN PVOID Context);

BOOLEAN NTAPI
FatNoopAcquire(IN PVOID Context,
               IN BOOLEAN Wait);

VOID NTAPI
FatNoopRelease(IN PVOID Context);

/* ---------------------------------------------------------  fastfat.c */

extern FAST_MUTEX FatCloseQueueMutex;

PFAT_IRP_CONTEXT NTAPI
FatBuildIrpContext(PIRP Irp, BOOLEAN CanWait);

VOID NTAPI
FatDestroyIrpContext(PFAT_IRP_CONTEXT IrpContext);

VOID
NTAPI
FatQueueRequest(IN PFAT_IRP_CONTEXT IrpContext,
                IN PFAT_OPERATION_HANDLER OperationHandler);

VOID NTAPI
FatCompleteRequest(PFAT_IRP_CONTEXT IrpContext OPTIONAL,
                   PIRP Irp OPTIONAL,
                   NTSTATUS Status);

BOOLEAN NTAPI
FatAcquireExclusiveVcb(IN PFAT_IRP_CONTEXT IrpContext,
                       IN PVCB Vcb);

BOOLEAN NTAPI
FatAcquireSharedVcb(IN PFAT_IRP_CONTEXT IrpContext,
                    IN PVCB Vcb);

VOID NTAPI
FatReleaseVcb(IN PFAT_IRP_CONTEXT IrpContext,
              IN PVCB Vcb);

BOOLEAN NTAPI
FatAcquireExclusiveFcb(IN PFAT_IRP_CONTEXT IrpContext,
                       IN PFCB Fcb);

BOOLEAN NTAPI
FatAcquireSharedFcb(IN PFAT_IRP_CONTEXT IrpContext,
                       IN PFCB Fcb);

VOID NTAPI
FatReleaseFcb(IN PFAT_IRP_CONTEXT IrpContext,
              IN PFCB Fcb);

TYPE_OF_OPEN
NTAPI
FatDecodeFileObject(IN PFILE_OBJECT FileObject,
                    OUT PVCB *Vcb,
                    OUT PFCB *FcbOrDcb,
                    OUT PCCB *Ccb);

VOID NTAPI
FatSetFileObject(PFILE_OBJECT FileObject,
                 TYPE_OF_OPEN TypeOfOpen,
                 PVOID Fcb,
                 PCCB Ccb);

PVOID FASTCALL
FatMapUserBuffer(PIRP Irp);

BOOLEAN NTAPI
FatIsTopLevelIrp(IN PIRP Irp);

VOID NTAPI
FatNotifyReportChange(IN PFAT_IRP_CONTEXT IrpContext,
                      IN PVCB Vcb,
                      IN PFCB Fcb,
                      IN ULONG Filter,
                      IN ULONG Action);

/* --------------------------------------------------------- fullfat.c */

FF_T_SINT32
FatWriteBlocks(FF_T_UINT8 *pBuffer, FF_T_UINT32 SectorAddress, FF_T_UINT32 Count, void *pParam);

FF_T_SINT32
FatReadBlocks(FF_T_UINT8 *pBuffer, FF_T_UINT32 SectorAddress, FF_T_UINT32 Count, void *pParam);

VOID NTAPI
FatQueryFileTimes(OUT PLARGE_INTEGER FileTimes,
                  IN PDIR_ENTRY Dirent);

/* ---------------------------------------------------------  lock.c */

NTSTATUS NTAPI
FatLockControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

VOID NTAPI
FatOplockComplete(IN PVOID Context,
                  IN PIRP Irp);

VOID NTAPI
FatPrePostIrp(IN PVOID Context,
              IN PIRP Irp);

/*  ---------------------------------------------------------  fsctl.c  */

NTSTATUS NTAPI
FatFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ---------------------------------------------------------  finfo.c  */

NTSTATUS NTAPI FatQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI FatSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ---------------------------------------------------------  fullfat.c  */

FF_FILE *FF_OpenW(FF_IOMAN *pIoman, PUNICODE_STRING pathW, FF_T_UINT8 Mode, FF_ERROR *pError);

/*  ---------------------------------------------------------  iface.c  */

NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

/*  -----------------------------------------------------------  fat.c  */
NTSTATUS NTAPI
FatInitializeVcb(
    IN PFAT_IRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb);

VOID NTAPI
FatUninitializeVcb(
    IN PVCB Vcb);

/*  ------------------------------------------------------  device.c  */

NTSTATUS NTAPI
FatDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS
FatPerformDevIoCtrl(PDEVICE_OBJECT DeviceObject,
                    ULONG ControlCode,
                    PVOID InputBuffer,
                    ULONG InputBufferSize,
                    PVOID OutputBuffer,
                    ULONG OutputBufferSize,
                    BOOLEAN Override);

/*  -----------------------------------------------------------  fcb.c  */
PFCB NTAPI
FatCreateFcb(
    IN PFAT_IRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB ParentDcb,
    IN FF_FILE *FileHandle);

VOID NTAPI
FatDeleteFcb(IN PFAT_IRP_CONTEXT IrpContext,
             IN PFCB Fcb);

IO_STATUS_BLOCK NTAPI
FatiOpenExistingFcb(IN PFAT_IRP_CONTEXT IrpContext,
                    IN PFILE_OBJECT FileObject,
                    IN PVCB Vcb,
                    IN PFCB Fcb,
                    IN PACCESS_MASK DesiredAccess,
                    IN USHORT ShareAccess,
                    IN ULONG AllocationSize,
                    IN PFILE_FULL_EA_INFORMATION EaBuffer,
                    IN ULONG EaLength,
                    IN UCHAR FileAttributes,
                    IN ULONG CreateDisposition,
                    IN BOOLEAN NoEaKnowledge,
                    IN BOOLEAN DeleteOnClose,
                    IN BOOLEAN OpenedAsDos,
                    OUT PBOOLEAN OplockPostIrp);

PFCB NTAPI
FatFindFcb(PFAT_IRP_CONTEXT IrpContext,
           PRTL_SPLAY_LINKS *RootNode,
           PSTRING AnsiName,
           PBOOLEAN IsDosName);

VOID NTAPI
FatInsertName(IN PFAT_IRP_CONTEXT IrpContext,
              IN PRTL_SPLAY_LINKS *RootNode,
              IN PFCB_NAME_LINK Name);

VOID NTAPI
FatRemoveNames(IN PFAT_IRP_CONTEXT IrpContext,
               IN PFCB Fcb);

PCCB NTAPI
FatCreateCcb(VOID);

VOID NTAPI
FatDeleteCcb(IN PFAT_IRP_CONTEXT IrpContext,
             IN PCCB Ccb);

VOID NTAPI
FatSetFullNameInFcb(PFCB Fcb,
                    PUNICODE_STRING Name);

VOID NTAPI
FatSetFullFileNameInFcb(IN PFAT_IRP_CONTEXT IrpContext,
                        IN PFCB Fcb);

VOID NTAPI
FatSetFcbNames(IN PFAT_IRP_CONTEXT IrpContext,
               IN PFCB Fcb);

VOID NTAPI
Fati8dot3ToString(IN PCHAR FileName,
                  IN BOOLEAN DownCase,
                  OUT POEM_STRING OutString);

/*  ------------------------------------------------------------  rw.c  */

NTSTATUS NTAPI
FatRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS NTAPI
FatWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ------------------------------------------------------------- flush.c  */

NTSTATUS NTAPI
FatFlushBuffers(PDEVICE_OBJECT DeviceObject, PIRP Irp);


/* EOF */
