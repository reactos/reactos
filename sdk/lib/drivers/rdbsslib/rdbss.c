/*
 *  ReactOS kernel
 *  Copyright (C) 2017 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             sdk/lib/drivers/rdbsslib/rdbss.c
 * PURPOSE:          RDBSS library
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <rx.h>
#include <pseh/pseh2.h>
#include <limits.h>
#include <dfs.h>
#include <copysup.h>

#define NDEBUG
#include <debug.h>

#define RX_TOPLEVELCTX_FLAG_FROM_POOL 1

typedef
NTSTATUS
(NTAPI *PRX_FSD_DISPATCH) (
    PRX_CONTEXT Context);

typedef struct _RX_FSD_DISPATCH_VECTOR
{
    PRX_FSD_DISPATCH CommonRoutine;
} RX_FSD_DISPATCH_VECTOR, *PRX_FSD_DISPATCH_VECTOR;

VOID
NTAPI
RxAcquireFileForNtCreateSection(
    PFILE_OBJECT FileObject);

NTSTATUS
NTAPI
RxAcquireForCcFlush(
    PFILE_OBJECT FileObject,
    PDEVICE_OBJECT DeviceObject);

VOID
RxAddToTopLevelIrpAllocatedContextsList(
    PRX_TOPLEVELIRP_CONTEXT TopLevelContext);

VOID
RxAssert(
    PVOID Assert,
    PVOID File,
    ULONG Line,
    PVOID Message);

NTSTATUS
NTAPI
RxCommonCleanup(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonClose(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonCreate(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonDevFCBCleanup(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonDevFCBClose(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonDevFCBFsCtl(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonDevFCBIoCtl(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonDevFCBQueryVolInfo(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonDeviceControl(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonDirectoryControl(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonDispatchProblem(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonFileSystemControl(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonFlushBuffers(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonLockControl(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonQueryEa(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonQueryInformation(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonQueryQuotaInformation(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonQuerySecurity(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonQueryVolumeInformation(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonRead(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonSetEa(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonSetInformation(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonSetQuotaInformation(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonSetSecurity(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonSetVolumeInformation(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonUnimplemented(
    PRX_CONTEXT Context);

NTSTATUS
NTAPI
RxCommonWrite(
    PRX_CONTEXT Context);

VOID
RxCopyCreateParameters(
    IN PRX_CONTEXT RxContext);

NTSTATUS
RxCreateFromNetRoot(
    PRX_CONTEXT Context,
    PUNICODE_STRING NetRootName);

NTSTATUS
RxCreateTreeConnect(
    IN PRX_CONTEXT RxContext);

BOOLEAN
NTAPI
RxFastIoCheckIfPossible(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length, BOOLEAN Wait,
    ULONG LockKey, BOOLEAN CheckForReadOperation,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject);

BOOLEAN
NTAPI
RxFastIoDeviceControl(
    PFILE_OBJECT FileObject,
    BOOLEAN Wait,
    PVOID InputBuffer OPTIONAL,
    ULONG InputBufferLength,
    PVOID OutputBuffer OPTIONAL,
    ULONG OutputBufferLength,
    ULONG IoControlCode,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject);

BOOLEAN
NTAPI
RxFastIoRead(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    PVOID Buffer,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject);

BOOLEAN
NTAPI
RxFastIoWrite(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    PVOID Buffer,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject);

NTSTATUS
RxFindOrCreateFcb(
    PRX_CONTEXT RxContext,
    PUNICODE_STRING NetRootName);

NTSTATUS
RxFirstCanonicalize(
    PRX_CONTEXT RxContext,
    PUNICODE_STRING FileName,
    PUNICODE_STRING CanonicalName,
    PNET_ROOT_TYPE NetRootType);

VOID
RxFreeCanonicalNameBuffer(
    PRX_CONTEXT Context);

VOID
NTAPI
RxFspDispatch(
    IN PVOID Context);

VOID
NTAPI
RxGetRegistryParameters(
    IN PUNICODE_STRING RegistryPath);

NTSTATUS
NTAPI
RxGetStringRegistryParameter(
    IN HANDLE KeyHandle,
    IN PCWSTR KeyName,
    OUT PUNICODE_STRING OutString,
    IN PUCHAR Buffer,
    IN ULONG BufferLength,
    IN BOOLEAN LogFailure);

VOID
NTAPI
RxInitializeDebugSupport(
    VOID);

VOID
NTAPI
RxInitializeDispatchVectors(
    PDRIVER_OBJECT DriverObject);

NTSTATUS
NTAPI
RxInitializeRegistrationStructures(
    VOID);

VOID
NTAPI
RxInitializeTopLevelIrpPackage(
    VOID);

VOID
NTAPI
RxInitUnwind(
    PDRIVER_OBJECT DriverObject,
    USHORT State);

BOOLEAN
RxIsThisAnRdbssTopLevelContext(
    PRX_TOPLEVELIRP_CONTEXT TopLevelContext);

NTSTATUS
NTAPI
RxLowIoIoCtlShellCompletion(
    PRX_CONTEXT RxContext);

NTSTATUS
RxLowIoReadShell(
    PRX_CONTEXT RxContext);

NTSTATUS
NTAPI
RxLowIoReadShellCompletion(
    PRX_CONTEXT RxContext);

NTSTATUS
RxLowIoWriteShell(
    IN PRX_CONTEXT RxContext);

NTSTATUS
NTAPI
RxLowIoWriteShellCompletion(
    PRX_CONTEXT RxContext);

PVOID
RxNewMapUserBuffer(
    PRX_CONTEXT RxContext);

NTSTATUS
RxNotifyChangeDirectory(
    PRX_CONTEXT RxContext);

VOID
NTAPI
RxpCancelRoutine(
    PVOID Context);

NTSTATUS
RxpQueryInfoMiniRdr(
    PRX_CONTEXT RxContext,
    FILE_INFORMATION_CLASS FileInfoClass,
    PVOID Buffer);

VOID
RxPurgeNetFcb(
    PFCB Fcb,
    PRX_CONTEXT LocalContext);

NTSTATUS
RxQueryAlternateNameInfo(
    PRX_CONTEXT RxContext,
    PFILE_NAME_INFORMATION AltNameInfo);

NTSTATUS
RxQueryBasicInfo(
    PRX_CONTEXT RxContext,
    PFILE_BASIC_INFORMATION BasicInfo);

NTSTATUS
RxQueryCompressedInfo(
    PRX_CONTEXT RxContext,
    PFILE_COMPRESSION_INFORMATION CompressionInfo);

NTSTATUS
RxQueryDirectory(
    PRX_CONTEXT RxContext);

NTSTATUS
RxQueryEaInfo(
    PRX_CONTEXT RxContext,
    PFILE_EA_INFORMATION EaInfo);

NTSTATUS
RxQueryInternalInfo(
    PRX_CONTEXT RxContext,
    PFILE_INTERNAL_INFORMATION InternalInfo);

NTSTATUS
RxQueryNameInfo(
    PRX_CONTEXT RxContext,
    PFILE_NAME_INFORMATION NameInfo);

NTSTATUS
RxQueryPipeInfo(
    PRX_CONTEXT RxContext,
    PFILE_PIPE_INFORMATION PipeInfo);

NTSTATUS
RxQueryPositionInfo(
    PRX_CONTEXT RxContext,
    PFILE_POSITION_INFORMATION PositionInfo);

NTSTATUS
RxQueryStandardInfo(
    PRX_CONTEXT RxContext,
    PFILE_STANDARD_INFORMATION StandardInfo);

VOID
NTAPI
RxReadRegistryParameters(
    VOID);

VOID
NTAPI
RxReleaseFileForNtCreateSection(
    PFILE_OBJECT FileObject);

NTSTATUS
NTAPI
RxReleaseForCcFlush(
    PFILE_OBJECT FileObject,
    PDEVICE_OBJECT DeviceObject);

PRX_CONTEXT
RxRemoveOverflowEntry(
    PRDBSS_DEVICE_OBJECT DeviceObject,
    WORK_QUEUE_TYPE Queue);

NTSTATUS
RxSearchForCollapsibleOpen(
    PRX_CONTEXT RxContext,
    ACCESS_MASK DesiredAccess,
    ULONG ShareAccess);

NTSTATUS
RxSetAllocationInfo(
    PRX_CONTEXT RxContext);

NTSTATUS
RxSetBasicInfo(
    PRX_CONTEXT RxContext);

NTSTATUS
RxSetDispositionInfo(
    PRX_CONTEXT RxContext);

NTSTATUS
RxSetEndOfFileInfo(
    PRX_CONTEXT RxContext);

NTSTATUS
RxSetPipeInfo(
    PRX_CONTEXT RxContext);

NTSTATUS
RxSetPositionInfo(
    PRX_CONTEXT RxContext);

NTSTATUS
RxSetRenameInfo(
    PRX_CONTEXT RxContext);

NTSTATUS
RxSetSimpleInfo(
    PRX_CONTEXT RxContext);

VOID
RxSetupNetFileObject(
    PRX_CONTEXT RxContext);

NTSTATUS
RxSystemControl(
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN PIRP Irp);

VOID
RxUninitializeCacheMap(
    PRX_CONTEXT RxContext,
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER TruncateSize);

VOID
RxUnstart(
    PRX_CONTEXT Context,
    PRDBSS_DEVICE_OBJECT DeviceObject);

NTSTATUS
RxXXXControlFileCallthru(
    PRX_CONTEXT Context);

PVOID
NTAPI
_RxAllocatePoolWithTag(
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag);

VOID
NTAPI
_RxFreePool(
    _In_ PVOID Buffer);

VOID
NTAPI
_RxFreePoolWithTag(
    _In_ PVOID Buffer,
    _In_ ULONG Tag);

WCHAR RxStarForTemplate = '*';
WCHAR Rx8QMdot3QM[] = L">>>>>>>>.>>>*";
BOOLEAN DisableByteRangeLockingOnReadOnlyFiles = FALSE;
BOOLEAN DisableFlushOnCleanup = FALSE;
ULONG ReadAheadGranularity = 1 << PAGE_SHIFT;
LIST_ENTRY RxActiveContexts;
NPAGED_LOOKASIDE_LIST RxContextLookasideList;
RDBSS_DATA RxData;
FCB RxDeviceFCB;
BOOLEAN RxLoudLowIoOpsEnabled = FALSE;
RX_FSD_DISPATCH_VECTOR RxDeviceFCBVector[IRP_MJ_MAXIMUM_FUNCTION + 1] =
{
    { RxCommonDispatchProblem },
    { RxCommonDispatchProblem },
    { RxCommonDevFCBClose },
    { RxCommonDispatchProblem },
    { RxCommonDispatchProblem },
    { RxCommonDispatchProblem },
    { RxCommonDispatchProblem },
    { RxCommonDispatchProblem },
    { RxCommonDispatchProblem },
    { RxCommonDispatchProblem },
    { RxCommonDevFCBQueryVolInfo },
    { RxCommonDispatchProblem },
    { RxCommonDispatchProblem },
    { RxCommonDevFCBFsCtl },
    { RxCommonDevFCBIoCtl },
    { RxCommonDevFCBIoCtl },
    { RxCommonDispatchProblem },
    { RxCommonDispatchProblem },
    { RxCommonDevFCBCleanup },
    { RxCommonDispatchProblem },
    { RxCommonDispatchProblem },
    { RxCommonDispatchProblem },
    { RxCommonUnimplemented },
    { RxCommonUnimplemented },
    { RxCommonUnimplemented },
    { RxCommonUnimplemented },
    { RxCommonUnimplemented },
    { RxCommonUnimplemented },
};
RDBSS_EXPORTS RxExports;
FAST_IO_DISPATCH RxFastIoDispatch;
PRDBSS_DEVICE_OBJECT RxFileSystemDeviceObject;
RX_FSD_DISPATCH_VECTOR RxFsdDispatchVector[IRP_MJ_MAXIMUM_FUNCTION + 1] =
{
    { RxCommonCreate },
    { RxCommonUnimplemented },
    { RxCommonClose },
    { RxCommonRead },
    { RxCommonWrite },
    { RxCommonQueryInformation },
    { RxCommonSetInformation },
    { RxCommonQueryEa },
    { RxCommonSetEa },
    { RxCommonFlushBuffers },
    { RxCommonQueryVolumeInformation },
    { RxCommonSetVolumeInformation },
    { RxCommonDirectoryControl },
    { RxCommonFileSystemControl },
    { RxCommonDeviceControl },
    { RxCommonDeviceControl },
    { RxCommonUnimplemented },
    { RxCommonLockControl },
    { RxCommonCleanup },
    { RxCommonUnimplemented },
    { RxCommonQuerySecurity },
    { RxCommonSetSecurity },
    { RxCommonUnimplemented },
    { RxCommonUnimplemented },
    { RxCommonUnimplemented },
    { RxCommonQueryQuotaInformation },
    { RxCommonSetQuotaInformation },
    { RxCommonUnimplemented },
};
ULONG RxFsdEntryCount;
LIST_ENTRY RxIrpsList;
KSPIN_LOCK RxIrpsListSpinLock;
KMUTEX RxScavengerMutex;
KMUTEX RxSerializationMutex;
UCHAR RxSpaceForTheWrappersDeviceObject[sizeof(*RxFileSystemDeviceObject)];
KSPIN_LOCK TopLevelIrpSpinLock;
LIST_ENTRY TopLevelIrpAllocatedContextsList;
BOOLEAN RxForceQFIPassThrough = FALSE;
BOOLEAN RxNoAsync = FALSE;

DECLARE_CONST_UNICODE_STRING(unknownId, L"???");

#if RDBSS_ASSERTS
#ifdef ASSERT
#undef ASSERT
#endif

#define ASSERT(exp)                               \
    if (!(exp))                                   \
    {                                             \
        RxAssert(#exp, __FILE__, __LINE__, NULL); \
    }
#endif

#if RX_POOL_WRAPPER
#undef RxAllocatePool
#undef RxAllocatePoolWithTag
#undef RxFreePool

#define RxAllocatePool(P, S) _RxAllocatePoolWithTag(P, S, 0)
#define RxAllocatePoolWithTag _RxAllocatePoolWithTag
#define RxFreePool _RxFreePool
#define RxFreePoolWithTag _RxFreePoolWithTag
#endif

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
VOID
CheckForLoudOperations(
    PRX_CONTEXT RxContext)
{
    RxCaptureFcb;

    PAGED_CODE();

#define ALLSCR_LENGTH (sizeof(L"all.scr") - sizeof(UNICODE_NULL))

    /* Are loud operations enabled? */
    if (RxLoudLowIoOpsEnabled)
    {
        /* If so, the operation will be loud only if filename ends with all.scr */
        if (RtlCompareMemory(Add2Ptr(capFcb->PrivateAlreadyPrefixedName.Buffer,
                             (capFcb->PrivateAlreadyPrefixedName.Length - ALLSCR_LENGTH)),
                             L"all.scr", ALLSCR_LENGTH) == ALLSCR_LENGTH)
        {
            SetFlag(RxContext->LowIoContext.Flags, LOWIO_CONTEXT_FLAG_LOUDOPS);
        }
    }
#undef ALLSCR_LENGTH
}

/*
 * @implemented
 */
VOID
__RxInitializeTopLevelIrpContext(
    IN OUT PRX_TOPLEVELIRP_CONTEXT TopLevelContext,
    IN PIRP Irp,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN ULONG Flags)
{
    DPRINT("__RxInitializeTopLevelIrpContext(%p, %p, %p, %u)\n", TopLevelContext, Irp, RxDeviceObject, Flags);

    RtlZeroMemory(TopLevelContext, sizeof(RX_TOPLEVELIRP_CONTEXT));
    TopLevelContext->Irp = Irp;
    TopLevelContext->Flags = (Flags ? RX_TOPLEVELCTX_FLAG_FROM_POOL : 0);
    TopLevelContext->Signature = RX_TOPLEVELIRP_CONTEXT_SIGNATURE;
    TopLevelContext->RxDeviceObject = RxDeviceObject;
    TopLevelContext->Previous = IoGetTopLevelIrp();
    TopLevelContext->Thread = PsGetCurrentThread();

    /* We cannot add to list something that'd come from stack */
    if (BooleanFlagOn(TopLevelContext->Flags, RX_TOPLEVELCTX_FLAG_FROM_POOL))
    {
        RxAddToTopLevelIrpAllocatedContextsList(TopLevelContext);
    }
}

/*
 * @implemented
 */
VOID
__RxWriteReleaseResources(
    PRX_CONTEXT RxContext,
    BOOLEAN ResourceOwnerSet,
    ULONG LineNumber,
    PCSTR FileName,
    ULONG SerialNumber)
{
    RxCaptureFcb;

    PAGED_CODE();

    ASSERT(RxContext != NULL);
    ASSERT(capFcb != NULL);

    /* If FCB resource was acquired, release it */
    if (RxContext->FcbResourceAcquired)
    {
        /* Taking care of owner */
        if (ResourceOwnerSet)
        {
            RxReleaseFcbForThread(RxContext, capFcb, RxContext->LowIoContext.ResourceThreadId);
        }
        else
        {
            RxReleaseFcb(RxContext, capFcb);
        }

        RxContext->FcbResourceAcquired = FALSE;
    }

    /* If FCB paging resource was acquired, release it */
    if (RxContext->FcbPagingIoResourceAcquired)
    {
        /* Taking care of owner */
        if (ResourceOwnerSet)
        {
            RxReleasePagingIoResourceForThread(RxContext, capFcb, RxContext->LowIoContext.ResourceThreadId);
        }
        else
        {
            RxReleasePagingIoResource(RxContext, capFcb);
        }

        /* No need to release boolean here, RxReleasePagingIoResource() takes care of it */
    }
}

/*
 * @implemented
 */
VOID
RxAddToTopLevelIrpAllocatedContextsList(
    PRX_TOPLEVELIRP_CONTEXT TopLevelContext)
{
    KIRQL OldIrql;

    DPRINT("RxAddToTopLevelIrpAllocatedContextsList(%p)\n", TopLevelContext);

    ASSERT(TopLevelContext->Signature == RX_TOPLEVELIRP_CONTEXT_SIGNATURE);
    ASSERT(BooleanFlagOn(TopLevelContext->Flags, RX_TOPLEVELCTX_FLAG_FROM_POOL));

    KeAcquireSpinLock(&TopLevelIrpSpinLock, &OldIrql);
    InsertTailList(&TopLevelIrpAllocatedContextsList, &TopLevelContext->ListEntry);
    KeReleaseSpinLock(&TopLevelIrpSpinLock, OldIrql);
}

/*
 * @implemented
 */
VOID
NTAPI
RxAddToWorkque(
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp)
{
    ULONG Queued;
    KIRQL OldIrql;
    WORK_QUEUE_TYPE Queue;

    RxCaptureParamBlock;

    RxContext->PostRequest = FALSE;

    /* First of all, select the appropriate queue - delayed for prefix claim, critical for the rest */
    if (RxContext->MajorFunction == IRP_MJ_DEVICE_CONTROL &&
        capPARAMS->Parameters.DeviceIoControl.IoControlCode == IOCTL_REDIR_QUERY_PATH)
    {
        Queue = DelayedWorkQueue;
        SetFlag(RxContext->Flags, RX_CONTEXT_FLAG_FSP_DELAYED_OVERFLOW_QUEUE);
    }
    else
    {
        Queue = CriticalWorkQueue;
        SetFlag(RxContext->Flags, RX_CONTEXT_FLAG_FSP_CRITICAL_OVERFLOW_QUEUE);
    }

    /* Check for overflow */
    if (capPARAMS->FileObject != NULL)
    {
        KeAcquireSpinLock(&RxFileSystemDeviceObject->OverflowQueueSpinLock, &OldIrql);

        Queued = InterlockedIncrement(&RxFileSystemDeviceObject->PostedRequestCount[Queue]);
        /* In case of an overflow, add the new queued call to the overflow list */
        if (Queued > 1)
        {
            InterlockedDecrement(&RxFileSystemDeviceObject->PostedRequestCount[Queue]);
            InsertTailList(&RxFileSystemDeviceObject->OverflowQueue[Queue], &RxContext->OverflowListEntry);
            ++RxFileSystemDeviceObject->OverflowQueueCount[Queue];

            KeReleaseSpinLock(&RxFileSystemDeviceObject->OverflowQueueSpinLock, OldIrql);
            return;
        }

        KeReleaseSpinLock(&RxFileSystemDeviceObject->OverflowQueueSpinLock, OldIrql);
    }

    ExInitializeWorkItem(&RxContext->WorkQueueItem, RxFspDispatch, RxContext);
    ExQueueWorkItem((PWORK_QUEUE_ITEM)&RxContext->WorkQueueItem, Queue);
}

/*
 * @implemented
 */
VOID
RxAdjustFileTimesAndSize(
    PRX_CONTEXT RxContext)
{
    NTSTATUS Status;
    LARGE_INTEGER CurrentTime;
    FILE_BASIC_INFORMATION FileBasicInfo;
    FILE_END_OF_FILE_INFORMATION FileEOFInfo;
    BOOLEAN FileModified, SetLastChange, SetLastAccess, SetLastWrite, NeedUpdate;

    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    PAGED_CODE();

    /* If Cc isn't initialized, the file was not read nor written, nothing to do */
    if (capFileObject->PrivateCacheMap == NULL)
    {
        return;
    }

    /* Get now */
    KeQuerySystemTime(&CurrentTime);

    /* Was the file modified? */
    FileModified = BooleanFlagOn(capFileObject->Flags, FO_FILE_MODIFIED);
    /* We'll set last write if it was modified and user didn't update yet */
    SetLastWrite = FileModified && !BooleanFlagOn(capFobx->Flags, FOBX_FLAG_USER_SET_LAST_WRITE);
    /* File was accessed if: written or read (fastio), we'll update last access if user didn't */
    SetLastAccess = SetLastWrite ||
                    (BooleanFlagOn(capFileObject->Flags, FO_FILE_FAST_IO_READ) &&
                     !BooleanFlagOn(capFobx->Flags, FOBX_FLAG_USER_SET_LAST_ACCESS));
    /* We'll set last change if it was modified and user didn't update yet */
    SetLastChange = FileModified && !BooleanFlagOn(capFobx->Flags, FOBX_FLAG_USER_SET_LAST_CHANGE);

    /* Nothing to update? Job done */
    if (!FileModified && !SetLastWrite && !SetLastAccess && !SetLastChange)
    {
        return;
    }

    /* By default, we won't issue any MRxSetFileInfoAtCleanup call */
    NeedUpdate = FALSE;
    RtlZeroMemory(&FileBasicInfo, sizeof(FileBasicInfo));

    /* Update lastwrite time if required */
    if (SetLastWrite)
    {
        NeedUpdate = TRUE;
        capFcb->LastWriteTime.QuadPart = CurrentTime.QuadPart;
        FileBasicInfo.LastWriteTime.QuadPart = CurrentTime.QuadPart;
    }

    /* Update lastaccess time if required */
    if (SetLastAccess)
    {
        NeedUpdate = TRUE;
        capFcb->LastAccessTime.QuadPart = CurrentTime.QuadPart;
        FileBasicInfo.LastAccessTime.QuadPart = CurrentTime.QuadPart;
    }

    /* Update lastchange time if required */
    if (SetLastChange)
    {
        NeedUpdate = TRUE;
        capFcb->LastChangeTime.QuadPart = CurrentTime.QuadPart;
        FileBasicInfo.ChangeTime.QuadPart = CurrentTime.QuadPart;
    }

    /* If one of the date was modified, issue a call to mini-rdr */
    if (NeedUpdate)
    {
        RxContext->Info.FileInformationClass = FileBasicInformation;
        RxContext->Info.Buffer = &FileBasicInfo;
        RxContext->Info.Length = sizeof(FileBasicInfo);

        MINIRDR_CALL(Status, RxContext, capFcb->MRxDispatch, MRxSetFileInfoAtCleanup, (RxContext));
        (void)Status;
    }

    /* If the file was modified, update its EOF */
    if (FileModified)
    {
        FileEOFInfo.EndOfFile.QuadPart = capFcb->Header.FileSize.QuadPart;

        RxContext->Info.FileInformationClass = FileEndOfFileInformation;
        RxContext->Info.Buffer = &FileEOFInfo;
        RxContext->Info.Length = sizeof(FileEOFInfo);

        MINIRDR_CALL(Status, RxContext, capFcb->MRxDispatch, MRxSetFileInfoAtCleanup, (RxContext));
        (void)Status;
    }
}

/*
 * @implemented
 */
NTSTATUS
RxAllocateCanonicalNameBuffer(
    PRX_CONTEXT RxContext,
    PUNICODE_STRING CanonicalName,
    USHORT CanonicalLength)
{
    PAGED_CODE();

    DPRINT("RxContext: %p - CanonicalNameBuffer: %p\n", RxContext, RxContext->Create.CanonicalNameBuffer);

    /* Context must be free of any already allocated name */
    ASSERT(RxContext->Create.CanonicalNameBuffer == NULL);

    /* Validate string length */
    if (CanonicalLength > USHRT_MAX - 1)
    {
        CanonicalName->Buffer = NULL;
        return STATUS_OBJECT_PATH_INVALID;
    }

    CanonicalName->Buffer = RxAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION, CanonicalLength, RX_MISC_POOLTAG);
    if (CanonicalName->Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    CanonicalName->Length = 0;
    CanonicalName->MaximumLength = CanonicalLength;

    /* Set the two places - they must always be identical */
    RxContext->Create.CanonicalNameBuffer = CanonicalName->Buffer;
    RxContext->AlsoCanonicalNameBuffer = CanonicalName->Buffer;

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
RxCancelNotifyChangeDirectoryRequestsForFobx(
   PFOBX Fobx)
{
    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    PRX_CONTEXT Context;
    LIST_ENTRY ContextsToCancel;

    /* Init a list for the contexts to cancel */
    InitializeListHead(&ContextsToCancel);

    /* Lock our list lock */
    KeAcquireSpinLock(&RxStrucSupSpinLock, &OldIrql);

    /* Now, browse all the active contexts, to find the associated ones */
    Entry = RxActiveContexts.Flink;
    while (Entry != &RxActiveContexts)
    {
        Context = CONTAINING_RECORD(Entry, RX_CONTEXT, ContextListEntry);
        Entry = Entry->Flink;

        /* Not the IRP we're looking for, ignore */
        if (Context->MajorFunction != IRP_MJ_DIRECTORY_CONTROL ||
            Context->MinorFunction != IRP_MN_NOTIFY_CHANGE_DIRECTORY)
        {
            continue;
        }

        /* Not the FOBX we're looking for, ignore */
        if ((PFOBX)Context->pFobx != Fobx)
        {
            continue;
        }

        /* No cancel routine (can't be cancel, then), ignore */
        if (Context->MRxCancelRoutine == NULL)
        {
            continue;
        }

        /* Mark our context as cancelled */
        SetFlag(Context->Flags, RX_CONTEXT_FLAG_CANCELLED);

        /* Move it to our list */
        RemoveEntryList(&Context->ContextListEntry);
        InsertTailList(&ContextsToCancel, &Context->ContextListEntry);

        InterlockedIncrement((volatile long *)&Context->ReferenceCount);
    }

    /* Done with the contexts */
    KeReleaseSpinLock(&RxStrucSupSpinLock, OldIrql);

    /* Now, handle all our "extracted" contexts */
    while (!IsListEmpty(&ContextsToCancel))
    {
        Entry = RemoveHeadList(&ContextsToCancel);
        Context = CONTAINING_RECORD(Entry, RX_CONTEXT, ContextListEntry);

        /* If they had an associated IRP (should be always true) */
        if (Context->CurrentIrp != NULL)
        {
            /* Then, call cancel routine */
            ASSERT(Context->MRxCancelRoutine != NULL);
            DPRINT1("Canceling %p with %p\n", Context, Context->MRxCancelRoutine);
            Context->MRxCancelRoutine(Context);
        }

        /* And delete the context */
        RxDereferenceAndDeleteRxContext(Context);
    }
}

/*
 * @implemented
 */
NTSTATUS
RxCancelNotifyChangeDirectoryRequestsForVNetRoot(
   PV_NET_ROOT VNetRoot,
   BOOLEAN ForceFilesClosed)
{
    KIRQL OldIrql;
    NTSTATUS Status;
    PLIST_ENTRY Entry;
    PRX_CONTEXT Context;
    LIST_ENTRY ContextsToCancel;

    /* Init a list for the contexts to cancel */
    InitializeListHead(&ContextsToCancel);

    /* Lock our list lock */
    KeAcquireSpinLock(&RxStrucSupSpinLock, &OldIrql);

    /* Assume success */
    Status = STATUS_SUCCESS;

    /* Now, browse all the active contexts, to find the associated ones */
    Entry = RxActiveContexts.Flink;
    while (Entry != &RxActiveContexts)
    {
        Context = CONTAINING_RECORD(Entry, RX_CONTEXT, ContextListEntry);
        Entry = Entry->Flink;

        /* Not the IRP we're looking for, ignore */
        if (Context->MajorFunction != IRP_MJ_DIRECTORY_CONTROL ||
            Context->MinorFunction != IRP_MN_NOTIFY_CHANGE_DIRECTORY)
        {
            continue;
        }

        /* Not the VNetRoot we're looking for, ignore */
        if (Context->pFcb == NULL ||
            (PV_NET_ROOT)Context->NotifyChangeDirectory.pVNetRoot != VNetRoot)
        {
            continue;
        }

        /* No cancel routine (can't be cancel, then), ignore */
        if (Context->MRxCancelRoutine == NULL)
        {
            continue;
        }

        /* At that point, we found a matching context
         * If we're not asked to force close, then fail - it's still open
         */
        if (!ForceFilesClosed)
        {
            Status = STATUS_FILES_OPEN;
            break;
        }

        /* Mark our context as cancelled */
        SetFlag(Context->Flags, RX_CONTEXT_FLAG_CANCELLED);

        /* Move it to our list */
        RemoveEntryList(&Context->ContextListEntry);
        InsertTailList(&ContextsToCancel, &Context->ContextListEntry);

        InterlockedIncrement((volatile long *)&Context->ReferenceCount);
    }

    /* Done with the contexts */
    KeReleaseSpinLock(&RxStrucSupSpinLock, OldIrql);

    if (Status != STATUS_SUCCESS)
    {
        return Status;
    }

    /* Now, handle all our "extracted" contexts */
    while (!IsListEmpty(&ContextsToCancel))
    {
        Entry = RemoveHeadList(&ContextsToCancel);
        Context = CONTAINING_RECORD(Entry, RX_CONTEXT, ContextListEntry);

        /* If they had an associated IRP (should be always true) */
        if (Context->CurrentIrp != NULL)
        {
            /* Then, call cancel routine */
            ASSERT(Context->MRxCancelRoutine != NULL);
            DPRINT1("Canceling %p with %p\n", Context, Context->MRxCancelRoutine);
            Context->MRxCancelRoutine(Context);
        }

        /* And delete the context */
        RxDereferenceAndDeleteRxContext(Context);
    }

    return Status;
}

/*
 * @implemented
 */
BOOLEAN
RxCancelOperationInOverflowQueue(
    PRX_CONTEXT RxContext)
{
    KIRQL OldIrql;
    BOOLEAN OperationToCancel;

    /* By default, nothing cancelled */
    OperationToCancel = FALSE;

    /* Acquire the overflow spinlock */
    KeAcquireSpinLock(&RxFileSystemDeviceObject->OverflowQueueSpinLock, &OldIrql);

    /* Is our context in any queue? */
    if (BooleanFlagOn(RxContext->Flags, (RX_CONTEXT_FLAG_FSP_DELAYED_OVERFLOW_QUEUE | RX_CONTEXT_FLAG_FSP_CRITICAL_OVERFLOW_QUEUE)))
    {
        /* Make sure flag is consistent with facts... */
        if (RxContext->OverflowListEntry.Flink != NULL)
        {
            /* Remove it from the list */
            RemoveEntryList(&RxContext->OverflowListEntry);
            RxContext->OverflowListEntry.Flink = NULL;

            /* Decrement appropriate count */
            if (BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_FSP_CRITICAL_OVERFLOW_QUEUE))
            {
                --RxFileSystemDeviceObject->OverflowQueueCount[CriticalWorkQueue];
            }
            else
            {
                --RxFileSystemDeviceObject->OverflowQueueCount[DelayedWorkQueue];
            }

            /* Clear the flag */
            ClearFlag(RxContext->Flags, ~(RX_CONTEXT_FLAG_FSP_DELAYED_OVERFLOW_QUEUE | RX_CONTEXT_FLAG_FSP_CRITICAL_OVERFLOW_QUEUE));

            /* Time to cancel! */
            OperationToCancel = TRUE;
        }
    }

    KeReleaseSpinLock(&RxFileSystemDeviceObject->OverflowQueueSpinLock, OldIrql);

    /* We have something to cancel & complete */
    if (OperationToCancel)
    {
        RxRemoveOperationFromBlockingQueue(RxContext);
        RxCompleteRequest(RxContext, STATUS_CANCELLED);
    }

    return OperationToCancel;
}

/*
 * @implemented
 */
VOID
NTAPI
RxCancelRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    PRX_CONTEXT RxContext;

    /* Lock our contexts list */
    KeAcquireSpinLock(&RxStrucSupSpinLock, &OldIrql);

    /* Now, find a context that matches the cancelled IRP */
    Entry = RxActiveContexts.Flink;
    while (Entry != &RxActiveContexts)
    {
        RxContext = CONTAINING_RECORD(Entry, RX_CONTEXT, ContextListEntry);
        Entry = Entry->Flink;

        /* Found! */
        if (RxContext->CurrentIrp == Irp)
        {
            break;
        }
    }

    /* If we reached the end of the list, we didn't find any context, so zero the buffer
     * If the context is already under cancellation, forget about it too
     */
    if (Entry == &RxActiveContexts || BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_CANCELLED))
    {
        RxContext = NULL;
    }
    else
    {
        /* Otherwise, reference it and mark it cancelled */
        SetFlag(RxContext->Flags, RX_CONTEXT_FLAG_CANCELLED);
        InterlockedIncrement((volatile long *)&RxContext->ReferenceCount);
    }

    /* Done with the contexts list */
    KeReleaseSpinLock(&RxStrucSupSpinLock, OldIrql);

    /* And done with the cancellation, we'll do it now */
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    /* If we have a context to cancel */
    if (RxContext != NULL)
    {
        /* We cannot executed at dispatch, so queue a deferred cancel */
        if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
        {
            RxDispatchToWorkerThread(RxFileSystemDeviceObject, CriticalWorkQueue, RxpCancelRoutine, RxContext);
        }
        /* Cancel now! */
        else
        {
            RxpCancelRoutine(RxContext);
        }
    }
}

/*
 * @implemented
 */
NTSTATUS
RxCanonicalizeFileNameByServerSpecs(
    PRX_CONTEXT RxContext,
    PUNICODE_STRING NetRootName)
{
    USHORT NextChar, CurChar;
    USHORT MaxChars;

    PAGED_CODE();

    /* Validate file name is not empty */
    MaxChars = NetRootName->Length / sizeof(WCHAR);
    if (MaxChars == 0)
    {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Validate name is correct */
    for (NextChar = 0, CurChar = 0; CurChar + 1 < MaxChars; NextChar = CurChar + 1)
    {
        USHORT i;

        for (i = NextChar + 1; i < MaxChars; ++i)
        {
            if (NetRootName->Buffer[i] == '\\' || NetRootName->Buffer[i] == ':')
            {
                break;
            }
        }

        CurChar = i - 1;
        if (CurChar == NextChar)
        {
            if (((NetRootName->Buffer[NextChar] != '\\' && NetRootName->Buffer[NextChar] != ':') || NextChar == (MaxChars - 1)) && NetRootName->Buffer[NextChar] != '.')
            {
                continue;
            }

            if (CurChar != 0)
            {
                if (CurChar >= MaxChars - 1)
                {
                    continue;
                }

                if (NetRootName->Buffer[CurChar + 1] != ':')
                {
                    return STATUS_OBJECT_PATH_SYNTAX_BAD;
                }
            }
            else
            {
                if (NetRootName->Buffer[1] != ':')
                {
                    return STATUS_OBJECT_PATH_SYNTAX_BAD;
                }
            }
        }
        else
        {
            if ((CurChar - NextChar) == 1)
            {
                if (NetRootName->Buffer[NextChar + 2] != '.')
                {
                    continue;
                }

                if (NetRootName->Buffer[NextChar] == '\\' || NetRootName->Buffer[NextChar] == ':' || NetRootName->Buffer[NextChar] == '.')
                {
                    return STATUS_OBJECT_PATH_SYNTAX_BAD;
                }
            }
            else
            {
                if ((CurChar - NextChar) != 2 || (NetRootName->Buffer[NextChar] != '\\' && NetRootName->Buffer[NextChar] != ':')
                    || NetRootName->Buffer[NextChar + 1] != '.')
                {
                    continue;
                }

                if (NetRootName->Buffer[NextChar + 2] == '.')
                {
                    return STATUS_OBJECT_PATH_SYNTAX_BAD;
                }
            }
        }
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
RxCanonicalizeNameAndObtainNetRoot(
    PRX_CONTEXT RxContext,
    PUNICODE_STRING FileName,
    PUNICODE_STRING NetRootName)
{
    NTSTATUS Status;
    NET_ROOT_TYPE NetRootType;
    UNICODE_STRING CanonicalName;

    RxCaptureParamBlock;
    RxCaptureFileObject;

    PAGED_CODE();

    NetRootType = NET_ROOT_WILD;

    RtlInitEmptyUnicodeString(NetRootName, NULL, 0);
    RtlInitEmptyUnicodeString(&CanonicalName, NULL, 0);

    /* if not relative opening, just handle the passed name */
    if (capFileObject->RelatedFileObject == NULL)
    {
        Status = RxFirstCanonicalize(RxContext, FileName, &CanonicalName, &NetRootType);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }
    else
    {
        PFCB Fcb;

        /* Make sure we have a valid FCB and a FOBX */
        Fcb = capFileObject->RelatedFileObject->FsContext;
        if (Fcb == NULL || capFileObject->RelatedFileObject->FsContext2 == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (!NodeTypeIsFcb(Fcb))
        {
            return STATUS_INVALID_PARAMETER;
        }

        UNIMPLEMENTED;
    }

    /* Get/Create the associated VNetRoot for opening */
    Status = RxFindOrConstructVirtualNetRoot(RxContext, &CanonicalName, NetRootType, NetRootName);
    if (!NT_SUCCESS(Status) && Status != STATUS_PENDING &&
        BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_MAILSLOT_REPARSE))
    {
        ASSERT(CanonicalName.Buffer == RxContext->Create.CanonicalNameBuffer);

        RxFreeCanonicalNameBuffer(RxContext);
        Status = RxFirstCanonicalize(RxContext, FileName, &CanonicalName, &NetRootType);
        if (NT_SUCCESS(Status))
        {
            Status = RxFindOrConstructVirtualNetRoot(RxContext, &CanonicalName, NetRootType, NetRootName);
        }
    }

    /* Filename cannot contain wildcards */
    if (FsRtlDoesNameContainWildCards(NetRootName))
    {
        Status = STATUS_OBJECT_NAME_INVALID;
    }

    /* Make sure file name is correct */
    if (NT_SUCCESS(Status))
    {
        Status = RxCanonicalizeFileNameByServerSpecs(RxContext, NetRootName);
    }

    /* Give the mini-redirector a chance to prepare the name */
    if (NT_SUCCESS(Status) || Status == STATUS_MORE_PROCESSING_REQUIRED)
    {
        if (RxContext->Create.pNetRoot != NULL)
        {
            NTSTATUS IgnoredStatus;

            MINIRDR_CALL(IgnoredStatus, RxContext, RxContext->Create.pNetRoot->pSrvCall->RxDeviceObject->Dispatch,
                         MRxPreparseName, (RxContext, NetRootName));
            (void)IgnoredStatus;
        }
    }

    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
RxCheckFcbStructuresForAlignment(
    VOID)
{
    PAGED_CODE();
}

#if DBG
NTSTATUS
RxCheckShareAccess(
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG DesiredShareAccess,
    _Inout_ PFILE_OBJECT FileObject,
    _Inout_ PSHARE_ACCESS ShareAccess,
    _In_ BOOLEAN Update,
    _In_ PSZ where,
    _In_ PSZ wherelogtag)
{
    PAGED_CODE();

    RxDumpWantedAccess(where, "", wherelogtag, DesiredAccess, DesiredShareAccess);
    RxDumpCurrentAccess(where, "", wherelogtag, ShareAccess);

    return IoCheckShareAccess(DesiredAccess, DesiredShareAccess, FileObject, ShareAccess, Update);
}
#endif

/*
 * @implemented
 */
NTSTATUS
RxCheckShareAccessPerSrvOpens(
    IN PFCB Fcb,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess)
{
    BOOLEAN ReadAccess;
    BOOLEAN WriteAccess;
    BOOLEAN DeleteAccess;
    PSHARE_ACCESS ShareAccess;

    PAGED_CODE();

    ShareAccess = &Fcb->ShareAccessPerSrvOpens;

    RxDumpWantedAccess("RxCheckShareAccessPerSrvOpens", "", "RxCheckShareAccessPerSrvOpens", DesiredAccess, DesiredShareAccess);
    RxDumpCurrentAccess("RxCheckShareAccessPerSrvOpens", "", "RxCheckShareAccessPerSrvOpens", ShareAccess);

    /* Check if any access wanted */
    ReadAccess = (DesiredAccess & (FILE_READ_DATA | FILE_EXECUTE)) != 0;
    WriteAccess = (DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) != 0;
    DeleteAccess = (DesiredAccess & DELETE) != 0;

    if (ReadAccess || WriteAccess || DeleteAccess)
    {
        BOOLEAN SharedRead = (DesiredShareAccess & FILE_SHARE_READ) != 0;
        BOOLEAN SharedWrite = (DesiredShareAccess & FILE_SHARE_WRITE) != 0;
        BOOLEAN SharedDelete = (DesiredShareAccess & FILE_SHARE_DELETE) != 0;

        /* Check whether there's a violation */
        if ((ReadAccess &&
             (ShareAccess->SharedRead < ShareAccess->OpenCount)) ||
            (WriteAccess &&
             (ShareAccess->SharedWrite < ShareAccess->OpenCount)) ||
            (DeleteAccess &&
             (ShareAccess->SharedDelete < ShareAccess->OpenCount)) ||
            ((ShareAccess->Readers != 0) && !SharedRead) ||
            ((ShareAccess->Writers != 0) && !SharedWrite) ||
            ((ShareAccess->Deleters != 0) && !SharedDelete))
        {
            return STATUS_SHARING_VIOLATION;
        }
    }

    return STATUS_SUCCESS;
}

VOID
RxCleanupPipeQueues(
    PRX_CONTEXT Context)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
RxCloseAssociatedSrvOpen(
    IN PFOBX Fobx,
    IN PRX_CONTEXT RxContext OPTIONAL)
{
    PFCB Fcb;
    NTSTATUS Status;
    PSRV_OPEN SrvOpen;
    BOOLEAN CloseSrvOpen;
    PRX_CONTEXT LocalContext;

    PAGED_CODE();

    /* Assume SRV_OPEN is already closed */
    CloseSrvOpen = FALSE;
    /* If we have a FOBX, we'll have to close it */
    if (Fobx != NULL)
    {
        /* If the FOBX isn't closed yet */
        if (!BooleanFlagOn(Fobx->Flags, FOBX_FLAG_SRVOPEN_CLOSED))
        {
            SrvOpen = Fobx->SrvOpen;
            Fcb = (PFCB)SrvOpen->pFcb;
            /* Check whether we've to close SRV_OPEN first */
            if (!BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_CLOSED))
            {
                CloseSrvOpen = TRUE;
            }
            else
            {
                ASSERT(RxIsFcbAcquiredExclusive(Fcb));

                /* Not much to do */
                SetFlag(Fobx->Flags, FOBX_FLAG_SRVOPEN_CLOSED);

                if (SrvOpen->OpenCount > 0)
                {
                    --SrvOpen->OpenCount;
                }
            }
        }

        /* No need to close SRV_OPEN, so close FOBX */
        if (!CloseSrvOpen)
        {
            RxMarkFobxOnClose(Fobx);

            return STATUS_SUCCESS;
        }
    }
    else
    {
        /* No FOBX? No RX_CONTEXT, ok, job done! */
        if (RxContext == NULL)
        {
            return STATUS_SUCCESS;
        }

        /* Get the FCB from RX_CONTEXT */
        Fcb = (PFCB)RxContext->pFcb;
        SrvOpen = NULL;
    }

    /* If we don't have RX_CONTEXT, allocte one, we'll need it */
    if (RxContext == NULL)
    {
        ASSERT(Fobx != NULL);

        LocalContext = RxCreateRxContext(NULL, Fcb->RxDeviceObject, RX_CONTEXT_FLAG_MUST_SUCCEED_NONBLOCKING | RX_CONTEXT_FLAG_WAIT);
        if (LocalContext == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        LocalContext->MajorFunction = 2;
        LocalContext->pFcb = RX_GET_MRX_FCB(Fcb);
        LocalContext->pFobx = (PMRX_FOBX)Fobx;
        LocalContext->pRelevantSrvOpen = (PMRX_SRV_OPEN)Fobx->SrvOpen;
    }
    else
    {
        LocalContext = RxContext;
    }

    ASSERT(RxIsFcbAcquiredExclusive(Fcb));

    /* Now, close the FOBX */
    if (Fobx != NULL)
    {
        RxMarkFobxOnClose(Fobx);
    }
    else
    {
        InterlockedDecrement((volatile long *)&Fcb->OpenCount);
    }

    /* If not a "standard" file, SRV_OPEN can be null */
    if (SrvOpen == NULL)
    {
        ASSERT((NodeType(Fcb) == RDBSS_NTC_OPENTARGETDIR_FCB) || (NodeType(Fcb) == RDBSS_NTC_IPC_SHARE) || (NodeType(Fcb) == RDBSS_NTC_MAILSLOT));
        RxDereferenceNetFcb(Fcb);

        if (LocalContext != RxContext)
        {
            RxDereferenceAndDeleteRxContext(LocalContext);
        }

        return STATUS_SUCCESS;
    }

    /* If SRV_OPEN isn't in a good condition, nothing to close */
    if (SrvOpen->Condition != Condition_Good)
    {
        if (LocalContext != RxContext)
        {
            RxDereferenceAndDeleteRxContext(LocalContext);
        }

        return STATUS_SUCCESS;
    }

    /* Decrease open count */
    if (SrvOpen->OpenCount > 0)
    {
        --SrvOpen->OpenCount;
    }

    /* If we're the only one left, is there a FOBX handled by Scavenger? */
    if (SrvOpen->OpenCount == 1)
    {
        if (!IsListEmpty(&SrvOpen->FobxList))
        {
            if (!IsListEmpty(&CONTAINING_RECORD(SrvOpen->FobxList.Flink, FOBX, FobxQLinks)->ScavengerFinalizationList))
            {
                SetFlag(SrvOpen->Flags, SRVOPEN_FLAG_CLOSE_DELAYED);
            }
        }
    }

    /* Nothing left, purge FCB */
    if (SrvOpen->OpenCount == 0 && RxContext == NULL)
    {
        RxPurgeNetFcb(Fcb, LocalContext);
    }

    /* Already closed? Job done! */
    SrvOpen = Fobx->SrvOpen;
    if (SrvOpen == NULL ||
        (SrvOpen->OpenCount != 0 && !BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING)) ||
        BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_CLOSED))
    {
        SetFlag(Fobx->Flags, FOBX_FLAG_SRVOPEN_CLOSED);
        if (LocalContext != RxContext)
        {
            RxDereferenceAndDeleteRxContext(LocalContext);
        }

        return STATUS_SUCCESS;
    }

    ASSERT(RxIsFcbAcquiredExclusive(Fcb));

    /* Inform mini-rdr about closing */
    MINIRDR_CALL(Status, LocalContext, Fcb->MRxDispatch, MRxCloseSrvOpen, (LocalContext));
    DPRINT("MRxCloseSrvOpen returned: %lx, called with RX_CONTEXT %p for FOBX %p (FCB %p, SRV_OPEN %p)\n ",
           Status, RxContext, Fobx, Fcb, SrvOpen);

    /* And mark as such */
    SetFlag(SrvOpen->Flags, SRVOPEN_FLAG_CLOSED);
    SrvOpen->Key = (PVOID)-1;

    /* If we were delayed, we're not! */
    if (BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_CLOSE_DELAYED))
    {
        InterlockedDecrement(&((PSRV_CALL)Fcb->pNetRoot->pSrvCall)->NumberOfCloseDelayedFiles);
    }

    /* Clear access */
    RxRemoveShareAccessPerSrvOpens(SrvOpen);
    RxPurgeChangeBufferingStateRequestsForSrvOpen(SrvOpen);

    /* Dereference */
    RxDereferenceSrvOpen(SrvOpen, LHS_ExclusiveLockHeld);

    /* Mark the FOBX closed as well */
    SetFlag(Fobx->Flags, FOBX_FLAG_SRVOPEN_CLOSED);

    if (LocalContext != RxContext)
    {
        RxDereferenceAndDeleteRxContext(LocalContext);
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
RxCollapseOrCreateSrvOpen(
    PRX_CONTEXT RxContext)
{
    NTSTATUS Status;
    ULONG Disposition;
    PSRV_OPEN SrvOpen;
    USHORT ShareAccess;
    ACCESS_MASK DesiredAccess;
    RX_BLOCK_CONDITION FcbCondition;

    RxCaptureFcb;
    RxCaptureParamBlock;

    PAGED_CODE();

    DPRINT("RxCollapseOrCreateSrvOpen(%p)\n", RxContext);

    ASSERT(RxIsFcbAcquiredExclusive(capFcb));
    ++capFcb->UncleanCount;

    DesiredAccess = capPARAMS->Parameters.Create.SecurityContext->DesiredAccess & FILE_ALL_ACCESS;
    ShareAccess = capPARAMS->Parameters.Create.ShareAccess & FILE_SHARE_VALID_FLAGS;

    Disposition = RxContext->Create.NtCreateParameters.Disposition;

    /* Try to find a reusable SRV_OPEN */
    Status = RxSearchForCollapsibleOpen(RxContext, DesiredAccess, ShareAccess);
    if (Status == STATUS_NOT_FOUND)
    {
        /* If none found, create one */
        SrvOpen = RxCreateSrvOpen((PV_NET_ROOT)RxContext->Create.pVNetRoot, capFcb);
        if (SrvOpen == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            SrvOpen->DesiredAccess = DesiredAccess;
            SrvOpen->ShareAccess = ShareAccess;
            Status = STATUS_SUCCESS;
        }

        RxContext->pRelevantSrvOpen = (PMRX_SRV_OPEN)SrvOpen;

        if (Status != STATUS_SUCCESS)
        {
            FcbCondition = Condition_Bad;
        }
        else
        {
            RxInitiateSrvOpenKeyAssociation(SrvOpen);

            /* Cookie to check the mini-rdr doesn't mess with RX_CONTEXT */
            RxContext->CurrentIrp->IoStatus.Information = 0xABCDEF;
            /* Inform the mini-rdr we're handling a create */
            MINIRDR_CALL(Status, RxContext, capFcb->MRxDispatch, MRxCreate, (RxContext));
            ASSERT(RxContext->CurrentIrp->IoStatus.Information == 0xABCDEF);

            DPRINT("MRxCreate returned: %x\n", Status);
            if (Status == STATUS_SUCCESS)
            {
                /* In case of overwrite, reset file size */
                if (Disposition == FILE_OVERWRITE || Disposition == FILE_OVERWRITE_IF)
                {
                    RxAcquirePagingIoResource(RxContext, capFcb);
                    capFcb->Header.AllocationSize.QuadPart = 0LL;
                    capFcb->Header.FileSize.QuadPart = 0LL;
                    capFcb->Header.ValidDataLength.QuadPart = 0LL;
                    RxContext->CurrentIrpSp->FileObject->SectionObjectPointer = &capFcb->NonPaged->SectionObjectPointers;
                    CcSetFileSizes(RxContext->CurrentIrpSp->FileObject, (PCC_FILE_SIZES)&capFcb->Header.AllocationSize);
                    RxReleasePagingIoResource(RxContext, capFcb);
                }
                else
                {
                    /* Otherwise, adjust sizes */
                    RxContext->CurrentIrpSp->FileObject->SectionObjectPointer = &capFcb->NonPaged->SectionObjectPointers;
                    if (CcIsFileCached(RxContext->CurrentIrpSp->FileObject))
                    {
                        RxAdjustAllocationSizeforCC(capFcb);
                    }
                    CcSetFileSizes(RxContext->CurrentIrpSp->FileObject, (PCC_FILE_SIZES)&capFcb->Header.AllocationSize);
                }
            }

            /* Set the IoStatus with information returned by mini-rdr */
            RxContext->CurrentIrp->IoStatus.Information = RxContext->Create.ReturnedCreateInformation;

            SrvOpen->OpenStatus = Status;
            /* Set SRV_OPEN state - good or bad - depending on whether create succeed */
            RxTransitionSrvOpen(SrvOpen, (Status == STATUS_SUCCESS ? Condition_Good : Condition_Bad));

            ASSERT(RxIsFcbAcquiredExclusive(capFcb));

            RxCompleteSrvOpenKeyAssociation(SrvOpen);

            if (Status == STATUS_SUCCESS)
            {
                if (BooleanFlagOn(capPARAMS->Parameters.Create.Options, FILE_DELETE_ON_CLOSE))
                {
                    ClearFlag(capFcb->FcbState, FCB_STATE_COLLAPSING_ENABLED);
                }
                SrvOpen->CreateOptions = RxContext->Create.NtCreateParameters.CreateOptions;
                FcbCondition = Condition_Good;
            }
            else
            {
                FcbCondition = Condition_Bad;
                RxDereferenceSrvOpen(SrvOpen, LHS_ExclusiveLockHeld);
                RxContext->pRelevantSrvOpen = NULL;

                if (RxContext->pFobx != NULL)
                {
                    RxDereferenceNetFobx(RxContext->pFobx, LHS_ExclusiveLockHeld);
                    RxContext->pFobx = NULL;
                }
            }
        }

        /* Set FCB state -  good or bad - depending on whether create succeed */
        DPRINT("Transitioning FCB %p to condition %lx\n", capFcb, capFcb->Condition);
        RxTransitionNetFcb(capFcb, FcbCondition);
    }
    else if (Status == STATUS_SUCCESS)
    {
        BOOLEAN IsGood, ExtraOpen;

        /* A reusable SRV_OPEN was found */
        RxContext->CurrentIrp->IoStatus.Information = FILE_OPENED;
        ExtraOpen = FALSE;

        SrvOpen = (PSRV_OPEN)RxContext->pRelevantSrvOpen;

        IsGood = (SrvOpen->Condition == Condition_Good);
        /* If the SRV_OPEN isn't in a stable situation, wait for it to become stable */
        if (!StableCondition(SrvOpen->Condition))
        {
            RxReferenceSrvOpen(SrvOpen);
            ++SrvOpen->OpenCount;
            ExtraOpen = TRUE;

            RxReleaseFcb(RxContext, capFcb);
            RxContext->Create.FcbAcquired = FALSE;

            RxWaitForStableSrvOpen(SrvOpen, RxContext);

            if (NT_SUCCESS(RxAcquireExclusiveFcb(RxContext, capFcb)))
            {
                RxContext->Create.FcbAcquired = TRUE;
            }

            IsGood = (SrvOpen->Condition == Condition_Good);
        }

        /* Inform the mini-rdr we do an opening with a reused SRV_OPEN */
        if (IsGood)
        {
            MINIRDR_CALL(Status, RxContext, capFcb->MRxDispatch, MRxCollapseOpen, (RxContext));

            ASSERT(RxIsFcbAcquiredExclusive(capFcb));
        }
        else
        {
            Status = SrvOpen->OpenStatus;
        }

        if (ExtraOpen)
        {
            --SrvOpen->OpenCount;
            RxDereferenceSrvOpen(SrvOpen, LHS_ExclusiveLockHeld);
        }
    }

    --capFcb->UncleanCount;

    DPRINT("Status: %x\n", Status);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxCommonCleanup(
    PRX_CONTEXT Context)
{
#define BugCheckFileId RDBSS_BUG_CHECK_CLEANUP
    PFCB Fcb;
    PFOBX Fobx;
    ULONG OpenCount;
    NTSTATUS Status;
    PNET_ROOT NetRoot;
    PFILE_OBJECT FileObject;
    LARGE_INTEGER TruncateSize;
    PLARGE_INTEGER TruncateSizePtr;
    BOOLEAN NeedPurge, FcbTableAcquired, OneLeft, IsFile, FcbAcquired, LeftForDelete;

    PAGED_CODE();

    Fcb = (PFCB)Context->pFcb;
    Fobx = (PFOBX)Context->pFobx;
    DPRINT("RxCommonCleanup(%p); FOBX: %p, FCB: %p\n", Context, Fobx, Fcb);

    /* File system closing, it's OK */
    if (Fobx == NULL)
    {
        if (Fcb->UncleanCount > 0)
        {
            InterlockedDecrement((volatile long *)&Fcb->UncleanCount);
        }

        return STATUS_SUCCESS;
    }

    /* Check we have a correct FCB type */
    if (NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_FILE &&
        NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_DIRECTORY &&
        NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_UNKNOWN &&
        NodeType(Fcb) != RDBSS_NTC_SPOOLFILE)
    {
        DPRINT1("Invalid Fcb type for %p\n", Fcb);
        RxBugCheck(Fcb->Header.NodeTypeCode, 0, 0);
    }

    FileObject = Context->CurrentIrpSp->FileObject;
    ASSERT(!BooleanFlagOn(FileObject->Flags, FO_CLEANUP_COMPLETE));

    RxMarkFobxOnCleanup(Fobx, &NeedPurge);

    Status = RxAcquireExclusiveFcb(Context, Fcb);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    FcbAcquired = TRUE;

    Fobx->AssociatedFileObject = NULL;

    /* In case it was already orphaned */
    if (BooleanFlagOn(Fcb->FcbState, FCB_STATE_ORPHANED))
    {
        ASSERT(Fcb->UncleanCount != 0);
        InterlockedDecrement((volatile long *)&Fcb->UncleanCount);

        if (BooleanFlagOn(FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING))
        {
            --Fcb->UncachedUncleanCount;
        }

        /* Inform mini-rdr */
        MINIRDR_CALL(Status, Context, Fcb->MRxDispatch, MRxCleanupFobx, (Context));

        ASSERT(Fobx->SrvOpen->UncleanFobxCount != 0);
        --Fobx->SrvOpen->UncleanFobxCount;

        RxUninitializeCacheMap(Context, FileObject, NULL);

        RxReleaseFcb(Context, Fcb);

        return STATUS_SUCCESS;
    }

    /* Report the fact that file could be set as delete on close */
    if (BooleanFlagOn(Fobx->Flags, FOBX_FLAG_DELETE_ON_CLOSE))
    {
        SetFlag(Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE);
    }

    /* Cancel any pending notification */
    RxCancelNotifyChangeDirectoryRequestsForFobx(Fobx);

    /* Backup open count before we start playing with it */
    OpenCount = Fcb->ShareAccess.OpenCount;

    NetRoot = (PNET_ROOT)Fcb->pNetRoot;
    FcbTableAcquired = FALSE;
    LeftForDelete = FALSE;
    OneLeft = (Fcb->UncleanCount == 1);

    _SEH2_TRY
    {
        /* Unclean count and delete on close? Verify whether we're the one */
        if (OneLeft && BooleanFlagOn(Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE))
        {
            if (RxAcquireFcbTableLockExclusive(&NetRoot->FcbTable, FALSE))
            {
                FcbTableAcquired = TRUE;
            }
            else
            {
                RxReleaseFcb(Context, Fcb);

                RxAcquireFcbTableLockExclusive(&NetRoot->FcbTable, TRUE);

                Status = RxAcquireExclusiveFcb(Context, Fcb);
                if (Status != STATUS_SUCCESS)
                {
                    RxReleaseFcbTableLock(&NetRoot->FcbTable);
                    return Status;
                }

                FcbTableAcquired = TRUE;
            }

            /* That means we'll perform the delete on close! */
            if (Fcb->UncleanCount == 1)
            {
                LeftForDelete = TRUE;
            }
            else
            {
                RxReleaseFcbTableLock(&NetRoot->FcbTable);
                FcbTableAcquired = FALSE;
            }
        }

        IsFile = FALSE;
        TruncateSizePtr = NULL;
        /* Handle cleanup for pipes and printers */
        if (NetRoot->Type == NET_ROOT_PIPE || NetRoot->Type == NET_ROOT_PRINT)
        {
            RxCleanupPipeQueues(Context);
        }
        /* Handle cleanup for files */
        else if (NetRoot->Type == NET_ROOT_DISK || NetRoot->Type == NET_ROOT_WILD)
        {
            Context->LowIoContext.Flags |= LOWIO_CONTEXT_FLAG_SAVEUNLOCKS;
            if (NodeType(Fcb) == RDBSS_NTC_STORAGE_TYPE_FILE)
            {
                /* First, unlock */
                FsRtlFastUnlockAll(&Fcb->Specific.Fcb.FileLock, FileObject, RxGetRequestorProcess(Context), Context);

                /* If there are still locks to release, proceed */
                if (Context->LowIoContext.ParamsFor.Locks.LockList != NULL)
                {
                    RxInitializeLowIoContext(&Context->LowIoContext, LOWIO_OP_UNLOCK_MULTIPLE);
                    Context->LowIoContext.ParamsFor.Locks.Flags = 0;
                    Status = RxLowIoLockControlShell(Context);
                }

                /* Fix times and size */
                RxAdjustFileTimesAndSize(Context);

                /* If we're the only one left... */
                if (OneLeft)
                {
                    /* And if we're supposed to delete on close */
                    if (LeftForDelete)
                    {
                        /* Update the sizes */
                        RxAcquirePagingIoResource(Context, Fcb);
                        Fcb->Header.FileSize.QuadPart = 0;
                        Fcb->Header.ValidDataLength.QuadPart = 0;
                        RxReleasePagingIoResource(Context, Fcb);
                    }
                    /* Otherwise, call the mini-rdr to adjust sizes */
                    else
                    {
                        /* File got grown up, fill with zeroes */
                        if (!BooleanFlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE) &&
                            (Fcb->Header.ValidDataLength.QuadPart < Fcb->Header.FileSize.QuadPart))
                        {
                            MINIRDR_CALL(Status, Context, Fcb->MRxDispatch, MRxZeroExtend, (Context));
                            Fcb->Header.ValidDataLength.QuadPart = Fcb->Header.FileSize.QuadPart;
                        }

                        /* File was truncated, let mini-rdr proceed */
                        if (BooleanFlagOn(Fcb->FcbState, FCB_STATE_TRUNCATE_ON_CLOSE))
                        {
                            MINIRDR_CALL(Status, Context, Fcb->MRxDispatch, MRxTruncate, (Context));
                            ClearFlag(Fcb->FcbState, FCB_STATE_TRUNCATE_ON_CLOSE);

                            /* Keep track of file change for Cc uninit */
                            TruncateSize.QuadPart = Fcb->Header.FileSize.QuadPart;
                            TruncateSizePtr = &TruncateSize;
                        }
                    }
                }

                /* If RxMarkFobxOnCleanup() asked for purge, make sure we're the only one left first */
                if (NeedPurge)
                {
                    if (!OneLeft)
                    {
                        NeedPurge = FALSE;
                    }
                }
                /* Otherwise, try to see whether we can purge */
                else
                {
                    NeedPurge = (OneLeft && (LeftForDelete || !BooleanFlagOn(Fcb->FcbState, FCB_STATE_COLLAPSING_ENABLED)));
                }

                IsFile = TRUE;
            }
        }

        /* We have to still be there! */
        ASSERT(Fcb->UncleanCount != 0);
        InterlockedDecrement((volatile long *)&Fcb->UncleanCount);

        if (BooleanFlagOn(FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING))
        {
            --Fcb->UncachedUncleanCount;
        }

        /* Inform mini-rdr about ongoing cleanup */
        MINIRDR_CALL(Status, Context, Fcb->MRxDispatch, MRxCleanupFobx, (Context));

        ASSERT(Fobx->SrvOpen->UncleanFobxCount != 0);
        --Fobx->SrvOpen->UncleanFobxCount;

        /* Flush cache */
        if (DisableFlushOnCleanup)
        {
            /* Only if we're the last standing */
            if (Fcb->NonPaged->SectionObjectPointers.DataSectionObject != NULL &&
                Fcb->UncleanCount == Fcb->UncachedUncleanCount)
            {
                DPRINT("Flushing %p due to last cached handle cleanup\n", Context);
                RxFlushFcbInSystemCache(Fcb, TRUE);
            }
        }
        else
        {
            /* Always */
            if (Fcb->NonPaged->SectionObjectPointers.DataSectionObject != NULL)
            {
                DPRINT("Flushing %p on cleanup\n", Context);
                RxFlushFcbInSystemCache(Fcb, TRUE);
            }
        }

        /* If only remaining uncached & unclean, then flush and purge */
        if (!BooleanFlagOn(FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING))
        {
            if (Fcb->UncachedUncleanCount != 0)
            {
                if (Fcb->UncachedUncleanCount == Fcb->UncleanCount &&
                    Fcb->NonPaged->SectionObjectPointers.DataSectionObject != NULL)
                {
                    DPRINT("Flushing FCB in system cache for %p\n", Context);
                    RxPurgeFcbInSystemCache(Fcb, NULL, 0, FALSE, TRUE);
                }
            }
        }

        /* If purge required, and not about to delete, flush */
        if (!LeftForDelete && NeedPurge)
        {
            DPRINT("Flushing FCB in system cache for %p\n", Context);
            RxFlushFcbInSystemCache(Fcb, TRUE);
        }

        /* If it was a file, drop cache */
        if (IsFile)
        {
            DPRINT("Uninit cache map for file\n");
            RxUninitializeCacheMap(Context, FileObject, TruncateSizePtr);
        }

        /* If that's the one left for deletion, or if it needs purge, flush */
        if (LeftForDelete || NeedPurge)
        {
            RxPurgeFcbInSystemCache(Fcb, NULL, 0, FALSE, !LeftForDelete);
            /* If that's for deletion, also remove from FCB table */
            if (LeftForDelete)
            {
                RxRemoveNameNetFcb(Fcb);
                RxReleaseFcbTableLock(&NetRoot->FcbTable);
                FcbTableAcquired = FALSE;
            }
        }

        /* Remove any share access */
        if (OpenCount != 0 && NetRoot->Type == NET_ROOT_DISK)
        {
            RxRemoveShareAccess(FileObject, &Fcb->ShareAccess, "Cleanup the share access", "ClnUpShr");
        }

        /* In case there's caching, on a file, and we were asked to drop collapsing, handle it */
        if (NodeType(Fcb) == RDBSS_NTC_STORAGE_TYPE_FILE && BooleanFlagOn(Fobx->Flags, FOBX_FLAG_DISABLE_COLLAPSING) &&
            RxWriteCacheingAllowed(Fcb, Fobx->pSrvOpen))
        {
            NTSTATUS InternalStatus;
            PRX_CONTEXT InternalContext;

            /* If we can properly set EOF, there's no need to drop collapsing, try to do it */
            InternalStatus = STATUS_UNSUCCESSFUL;
            InternalContext = RxCreateRxContext(Context->CurrentIrp,
                                                Fcb->RxDeviceObject,
                                                RX_CONTEXT_FLAG_WAIT | RX_CONTEXT_FLAG_MUST_SUCCEED_NONBLOCKING);
            if (InternalContext != NULL)
            {
                FILE_END_OF_FILE_INFORMATION FileEOF;

                InternalStatus = STATUS_SUCCESS;

                /* Initialize the context for file information set */
                InternalContext->pFcb = RX_GET_MRX_FCB(Fcb);
                InternalContext->pFobx = (PMRX_FOBX)Fobx;
                InternalContext->pRelevantSrvOpen = Fobx->pSrvOpen;

                /* Get EOF from the FCB */
                FileEOF.EndOfFile.QuadPart = Fcb->Header.FileSize.QuadPart;
                InternalContext->Info.FileInformationClass = FileEndOfFileInformation;
                InternalContext->Info.Buffer = &FileEOF;
                InternalContext->Info.Length = sizeof(FileEOF);

                /* Call the mini-rdr */
                MINIRDR_CALL_THROUGH(InternalStatus, Fcb->MRxDispatch, MRxSetFileInfo, (InternalContext));

                /* We're done */
                RxDereferenceAndDeleteRxContext(InternalContext);
            }

            /* We tried, so, clean the FOBX flag */
            ClearFlag(Fobx->Flags, FOBX_FLAG_DISABLE_COLLAPSING);
            /* If it failed, then, disable collapsing on the FCB */
            if (!NT_SUCCESS(InternalStatus))
            {
                ClearFlag(Fcb->FcbState, FCB_STATE_COLLAPSING_ENABLED);
            }
        }

        /* We're clean! */
        SetFlag(FileObject->Flags, FO_CLEANUP_COMPLETE);

        FcbAcquired = FALSE;
        RxReleaseFcb(Context, Fcb);
    }
    _SEH2_FINALLY
    {
        if (FcbAcquired)
        {
            RxReleaseFcb(Context, Fcb);
        }

        if (FcbTableAcquired)
        {
            RxReleaseFcbTableLock(&NetRoot->FcbTable);
        }
    }
    _SEH2_END;

    return Status;
#undef BugCheckFileId
}

NTSTATUS
NTAPI
RxCommonClose(
    PRX_CONTEXT Context)
{
#define BugCheckFileId RDBSS_BUG_CHECK_CLOSE
    PFCB Fcb;
    PFOBX Fobx;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    BOOLEAN DereferenceFobx, AcquiredFcb;

    PAGED_CODE();

    Fcb = (PFCB)Context->pFcb;
    Fobx = (PFOBX)Context->pFobx;
    FileObject = Context->CurrentIrpSp->FileObject;
    DPRINT("RxCommonClose(%p); FOBX: %p, FCB: %p, FO: %p\n", Context, Fobx, Fcb, FileObject);

    Status = RxAcquireExclusiveFcb(Context, Fcb);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    AcquiredFcb = TRUE;
    _SEH2_TRY
    {
        BOOLEAN Freed;

        /* Check our FCB type is expected */
        if (NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_UNKNOWN &&
            (NodeType(Fcb) < RDBSS_NTC_STORAGE_TYPE_DIRECTORY || (NodeType(Fcb) > RDBSS_NTC_STORAGE_TYPE_FILE &&
            (NodeType(Fcb) < RDBSS_NTC_SPOOLFILE || NodeType(Fcb) > RDBSS_NTC_OPENTARGETDIR_FCB))))
        {
            RxBugCheck(NodeType(Fcb), 0, 0);
        }

        RxReferenceNetFcb(Fcb);

        DereferenceFobx = FALSE;
        /* If we're not closing FS */
        if (Fobx != NULL)
        {
            PSRV_OPEN SrvOpen;
            PSRV_CALL SrvCall;

            SrvOpen = (PSRV_OPEN)Fobx->pSrvOpen;
            SrvCall = (PSRV_CALL)Fcb->pNetRoot->pSrvCall;
            /* Handle delayed close */
            if (NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_DIRECTORY)
            {
                if (!BooleanFlagOn(Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE | FCB_STATE_ORPHANED))
                {
                    if (BooleanFlagOn(Fcb->FcbState, FCB_STATE_COLLAPSING_ENABLED))
                    {
                        DPRINT("Delay close for FOBX: %p, SrvOpen %p\n", Fobx, SrvOpen);

                        if (SrvOpen->OpenCount == 1 && !BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_COLLAPSING_DISABLED))
                        {
                            if (InterlockedIncrement(&SrvCall->NumberOfCloseDelayedFiles) >= SrvCall->MaximumNumberOfCloseDelayedFiles)
                            {
                                InterlockedDecrement(&SrvCall->NumberOfCloseDelayedFiles);
                            }
                            else
                            {
                                DereferenceFobx = TRUE;
                                SetFlag(SrvOpen->Flags, SRVOPEN_FLAG_CLOSE_DELAYED);
                            }
                        }
                    }
                }
            }

            /* If we reach maximum of delayed close/or if there are no delayed close */
            if (!DereferenceFobx)
            {
                PNET_ROOT NetRoot;

                NetRoot = (PNET_ROOT)Fcb->pNetRoot;
                if (NetRoot->Type != NET_ROOT_PRINT)
                {
                    /* Delete if asked */
                    if (BooleanFlagOn(Fobx->Flags, FOBX_FLAG_DELETE_ON_CLOSE))
                    {
                        RxScavengeRelatedFobxs(Fcb);
                        RxSynchronizeWithScavenger(Context);

                        RxReleaseFcb(Context, Fcb);

                        RxAcquireFcbTableLockExclusive(&NetRoot->FcbTable, TRUE);
                        RxOrphanThisFcb(Fcb);
                        RxReleaseFcbTableLock(&NetRoot->FcbTable);

                        Status = RxAcquireExclusiveFcb(Context, Fcb);
                        ASSERT(NT_SUCCESS(Status));
                    }
                }
            }

            RxMarkFobxOnClose(Fobx);
        }

        if (DereferenceFobx)
        {
            ASSERT(Fobx != NULL);
            RxDereferenceNetFobx(Fobx, LHS_SharedLockHeld);
        }
        else
        {
            RxCloseAssociatedSrvOpen(Fobx, Context);
            if (Fobx != NULL)
            {
                RxDereferenceNetFobx(Fobx, LHS_ExclusiveLockHeld);
            }
        }

        Freed = RxDereferenceAndFinalizeNetFcb(Fcb, Context, FALSE, FALSE);
        AcquiredFcb = !Freed;

        FileObject->FsContext = (PVOID)-1;

        if (Freed)
        {
            RxTrackerUpdateHistory(Context, NULL, TRACKER_FCB_FREE, __LINE__, __FILE__, 0);
        }
        else
        {
            RxReleaseFcb(Context, Fcb);
            AcquiredFcb = FALSE;
        }
    }
    _SEH2_FINALLY
    {
        if (_SEH2_AbnormalTermination())
        {
            if (AcquiredFcb)
            {
                RxReleaseFcb(Context, Fcb);
            }
        }
        else
        {
            ASSERT(!AcquiredFcb);
        }
    }
    _SEH2_END;

    DPRINT("Status: %x\n", Status);
    return Status;
#undef BugCheckFileId
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxCommonCreate(
    PRX_CONTEXT Context)
{
    PIRP Irp;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;

    PAGED_CODE();

    DPRINT("RxCommonCreate(%p)\n", Context);

    Irp = Context->CurrentIrp;
    Stack = Context->CurrentIrpSp;
    FileObject = Stack->FileObject;

    /* Check whether that's a device opening */
    if (FileObject->FileName.Length == 0 && FileObject->RelatedFileObject == NULL)
    {
        FileObject->FsContext = &RxDeviceFCB;
        FileObject->FsContext2 = NULL;

        ++RxDeviceFCB.NodeReferenceCount;
        ++RxDeviceFCB.OpenCount;

        Irp->IoStatus.Information = FILE_OPENED;
        DPRINT("Device opening FO: %p, DO: %p, Name: %wZ\n", FileObject, Context->RxDeviceObject, &Context->RxDeviceObject->DeviceName);

        Status = STATUS_SUCCESS;
    }
    else
    {
        PFCB RelatedFcb = NULL;

        /* Make sure caller is consistent */
        if (FlagOn(Stack->Parameters.Create.Options, FILE_DIRECTORY_FILE | FILE_NON_DIRECTORY_FILE | FILE_OPEN_REMOTE_INSTANCE) ==
            (FILE_DIRECTORY_FILE | FILE_NON_DIRECTORY_FILE | FILE_OPEN_REMOTE_INSTANCE))
	    {
            DPRINT1("Create.Options: %x\n", Stack->Parameters.Create.Options);
	        return STATUS_INVALID_PARAMETER;
        }

        DPRINT("Ctxt: %p, FO: %p, Options: %lx, Flags: %lx, Attr: %lx, ShareAccess: %lx, DesiredAccess: %lx\n",
                 Context, FileObject, Stack->Parameters.Create.Options, Stack->Flags, Stack->Parameters.Create.FileAttributes,
                 Stack->Parameters.Create.ShareAccess, Stack->Parameters.Create.SecurityContext->DesiredAccess);
        DPRINT("FileName: %wZ\n", &FileObject->FileName);

        if (FileObject->RelatedFileObject != NULL)
        {
            RelatedFcb = FileObject->RelatedFileObject->FsContext;
            DPRINT("Rel FO: %p, path: %wZ\n", FileObject->RelatedFileObject, RelatedFcb->FcbTableEntry.Path);
        }

        /* Going to rename? */
        if (BooleanFlagOn(Stack->Flags, SL_OPEN_TARGET_DIRECTORY))
        {
            DPRINT("TargetDir!\n");
        }

        /* Copy create parameters to the context */
        RxCopyCreateParameters(Context);

        /* If the caller wants to establish a connection, go ahead */
        if (BooleanFlagOn(Stack->Parameters.Create.Options, FILE_CREATE_TREE_CONNECTION))
        {
            Status = RxCreateTreeConnect(Context);
        }
        else
        {
            /* Validate file name */
            if (FileObject->FileName.Length > sizeof(WCHAR) &&
                FileObject->FileName.Buffer[1] == OBJ_NAME_PATH_SEPARATOR &&
                FileObject->FileName.Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
            {
                FileObject->FileName.Length -= sizeof(WCHAR);
                RtlMoveMemory(&FileObject->FileName.Buffer[0], &FileObject->FileName.Buffer[1],
                              FileObject->FileName.Length);

                if (FileObject->FileName.Length > sizeof(WCHAR) &&
                    FileObject->FileName.Buffer[1] == OBJ_NAME_PATH_SEPARATOR &&
                    FileObject->FileName.Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
                {
                    return STATUS_OBJECT_NAME_INVALID;
                }
            }

            /* Attempt to open the file */
            do
            {
                UNICODE_STRING NetRootName;

                /* Strip last \ if required */
                if (FileObject->FileName.Length != 0 &&
                    FileObject->FileName.Buffer[FileObject->FileName.Length / sizeof(WCHAR) - 1] == OBJ_NAME_PATH_SEPARATOR)
                {
                    if (BooleanFlagOn(Stack->Parameters.Create.Options, FILE_NON_DIRECTORY_FILE))
                    {
                        return STATUS_OBJECT_NAME_INVALID;
                    }

                    FileObject->FileName.Length -= sizeof(WCHAR);
                    Context->Create.Flags |= RX_CONTEXT_CREATE_FLAG_STRIPPED_TRAILING_BACKSLASH;
                }

                if (BooleanFlagOn(Context->Flags, RX_CONTEXT_FLAG_WRITE_THROUGH))
                {
                    FileObject->Flags |= FO_WRITE_THROUGH;
                }

                /* Get the associated net root to opening */
                Status = RxCanonicalizeNameAndObtainNetRoot(Context, &FileObject->FileName, &NetRootName);
                if (Status != STATUS_MORE_PROCESSING_REQUIRED)
                {
                    break;
                }

                /* And attempt to open */
                Status = RxCreateFromNetRoot(Context, &NetRootName);
                if (Status == STATUS_SHARING_VIOLATION)
                {
                    ASSERT(!BooleanFlagOn(Context->Create.Flags, RX_CONTEXT_CREATE_FLAG_REPARSE));

                    /* If that happens for file creation, fail for real */
                    if (Context->Create.NtCreateParameters.Disposition == FILE_CREATE)
                    {
                        Status = STATUS_OBJECT_NAME_COLLISION;
                    }
                    else
                    {
                        /* Otherwise, if possible, attempt to scavenger current FOBX
                         * to check whether a dormant FOBX is the reason for sharing violation
                         */
                        if (Context->Create.TryForScavengingOnSharingViolation &&
                            !Context->Create.ScavengingAlreadyTried)
                        {
                            /* Only doable with a VNetRoot */
                            if (Context->Create.pVNetRoot != NULL)
                            {
                                PV_NET_ROOT VNetRoot;
                                NT_CREATE_PARAMETERS SavedParameters;

                                /* Save create parameters */
                                RtlCopyMemory(&SavedParameters, &Context->Create.NtCreateParameters, sizeof(NT_CREATE_PARAMETERS));

                                /* Reference the VNetRoot for the scavenging time */
                                VNetRoot = (PV_NET_ROOT)Context->Create.pVNetRoot;
                                RxReferenceVNetRoot(VNetRoot);

                                /* Prepare the RX_CONTEXT for reuse */
                                RxpPrepareCreateContextForReuse(Context);
                                RxReinitializeContext(Context);

                                /* Copy what we saved */
                                RtlCopyMemory(&Context->Create.NtCreateParameters, &SavedParameters, sizeof(NT_CREATE_PARAMETERS));

                                /* And recopy what can be */
                                RxCopyCreateParameters(Context);

                                /* And start purging, then scavenging FOBX */
                                RxPurgeRelatedFobxs((PNET_ROOT)VNetRoot->pNetRoot, Context,
                                                    DONT_ATTEMPT_FINALIZE_ON_PURGE, NULL);
                                RxScavengeFobxsForNetRoot((PNET_ROOT)VNetRoot->pNetRoot,
                                                          NULL, TRUE);

                                /* Ask for a second round */
                                Status = STATUS_MORE_PROCESSING_REQUIRED;

                                /* Keep track we already scavenged */
                                Context->Create.ScavengingAlreadyTried = TRUE;

                                /* Reference our SRV_CALL for CBS handling */
                                RxReferenceSrvCall(VNetRoot->pNetRoot->pSrvCall);
                                RxpProcessChangeBufferingStateRequests((PSRV_CALL)VNetRoot->pNetRoot->pSrvCall, FALSE);

                                /* Drop our extra reference */
                                RxDereferenceVNetRoot(VNetRoot, LHS_LockNotHeld);
                            }
                        }
                    }
                }
                else if (Status == STATUS_REPARSE)
                {
                    Context->CurrentIrp->IoStatus.Information = 0;
                }
                else
                {
                    ASSERT(!BooleanFlagOn(Context->Create.Flags, RX_CONTEXT_CREATE_FLAG_REPARSE));
                }
            }
            while (Status == STATUS_MORE_PROCESSING_REQUIRED);
        }

        if (Status == STATUS_RETRY)
        {
            RxpPrepareCreateContextForReuse(Context);
        }
        ASSERT(Status != STATUS_PENDING);
    }

    DPRINT("Status: %lx\n", Status);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxCommonDevFCBCleanup(
    PRX_CONTEXT Context)
{
    PMRX_FCB Fcb;
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("RxCommonDevFCBCleanup(%p)\n", Context);

    Fcb = Context->pFcb;
    Status = STATUS_SUCCESS;
    ASSERT(NodeType(Fcb) == RDBSS_NTC_DEVICE_FCB);

    /* Our FOBX if set, has to be a VNetRoot */
    if (Context->pFobx != NULL)
    {
        RxAcquirePrefixTableLockShared(Context->RxDeviceObject->pRxNetNameTable, TRUE);
        if (Context->pFobx->NodeTypeCode != RDBSS_NTC_V_NETROOT)
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
        RxReleasePrefixTableLock(Context->RxDeviceObject->pRxNetNameTable);
    }
    else
    {
        --Fcb->UncleanCount;
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxCommonDevFCBClose(
    PRX_CONTEXT Context)
{
    PMRX_FCB Fcb;
    NTSTATUS Status;
    PMRX_V_NET_ROOT NetRoot;

    PAGED_CODE();

    DPRINT("RxCommonDevFCBClose(%p)\n", Context);

    Fcb = Context->pFcb;
    NetRoot = (PMRX_V_NET_ROOT)Context->pFobx;
    Status = STATUS_SUCCESS;
    ASSERT(NodeType(Fcb) == RDBSS_NTC_DEVICE_FCB);

    /* Our FOBX if set, has to be a VNetRoot */
    if (NetRoot != NULL)
    {
        RxAcquirePrefixTableLockExclusive(Context->RxDeviceObject->pRxNetNameTable, TRUE);
        if (NetRoot->NodeTypeCode == RDBSS_NTC_V_NETROOT)
        {
            --NetRoot->NumberOfOpens;
            RxDereferenceVNetRoot(NetRoot, LHS_ExclusiveLockHeld);
        }
        else
        {
            Status = STATUS_NOT_IMPLEMENTED;
        }
        RxReleasePrefixTableLock(Context->RxDeviceObject->pRxNetNameTable);
    }
    else
    {
        --Fcb->OpenCount;
    }

    return Status;
}

NTSTATUS
NTAPI
RxCommonDevFCBFsCtl(
    PRX_CONTEXT Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxCommonDevFCBIoCtl(
    PRX_CONTEXT Context)
{
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("RxCommonDevFCBIoCtl(%p)\n", Context);

    if (Context->pFobx != NULL)
    {
        return STATUS_INVALID_HANDLE;
    }

    /* Is that a prefix claim from MUP? */
    if (Context->CurrentIrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_REDIR_QUERY_PATH)
    {
        return RxPrefixClaim(Context);
    }

    /* Otherwise, pass through the mini-rdr */
    Status = RxXXXControlFileCallthru(Context);
    if (Status != STATUS_PENDING)
    {
        if (Context->PostRequest)
        {
            Context->ResumeRoutine = RxCommonDevFCBIoCtl;
            Status = RxFsdPostRequest(Context);
        }
    }

    DPRINT("Status: %lx\n", Status);
    return Status;
}

NTSTATUS
NTAPI
RxCommonDevFCBQueryVolInfo(
    PRX_CONTEXT Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxCommonDeviceControl(
    PRX_CONTEXT Context)
{
    NTSTATUS Status;

    PAGED_CODE();

    /* Prefix claim is only allowed for device, not files */
    if (Context->CurrentIrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_REDIR_QUERY_PATH)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Submit to mini-rdr */
    RxInitializeLowIoContext(&Context->LowIoContext, LOWIO_OP_IOCTL);
    Status = RxLowIoSubmit(Context, RxLowIoIoCtlShellCompletion);
    if (Status == STATUS_PENDING)
    {
        RxDereferenceAndDeleteRxContext_Real(Context);
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxCommonDirectoryControl(
    PRX_CONTEXT Context)
{
    PFCB Fcb;
    PFOBX Fobx;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;

    PAGED_CODE();

    Fcb = (PFCB)Context->pFcb;
    Fobx = (PFOBX)Context->pFobx;
    Stack = Context->CurrentIrpSp;
    DPRINT("RxCommonDirectoryControl(%p) FOBX: %p, FCB: %p, Minor: %d\n", Context, Fobx, Fcb, Stack->MinorFunction);

    /* Call the appropriate helper */
    if (Stack->MinorFunction == IRP_MN_QUERY_DIRECTORY)
    {
        Status = RxQueryDirectory(Context);
    }
    else if (Stack->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY)
    {
        Status = RxNotifyChangeDirectory(Context);
        if (Status == STATUS_PENDING)
        {
            RxDereferenceAndDeleteRxContext_Real(Context);
        }
    }
    else
    {
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    return Status;
}

NTSTATUS
NTAPI
RxCommonDispatchProblem(
    PRX_CONTEXT Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RxCommonFileSystemControl(
    PRX_CONTEXT Context)
{
    PIRP Irp;
    ULONG ControlCode;
    PIO_STACK_LOCATION Stack;

    PAGED_CODE();

    Irp = Context->CurrentIrp;
    Stack = Context->CurrentIrpSp;
    ControlCode = Stack->Parameters.FileSystemControl.FsControlCode;

    DPRINT1("RxCommonFileSystemControl: %p, %p, %d, %lx\n", Context, Irp, Stack->MinorFunction, ControlCode);

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RxCommonFlushBuffers(
    PRX_CONTEXT Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RxCommonLockControl(
    PRX_CONTEXT Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RxCommonQueryEa(
    PRX_CONTEXT Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxCommonQueryInformation(
    PRX_CONTEXT Context)
{
#define SET_SIZE_AND_QUERY(AlreadyConsummed, Function)                              \
    Context->Info.Length = Stack->Parameters.QueryFile.Length - (AlreadyConsummed); \
    Status = Function(Context, Add2Ptr(Buffer, AlreadyConsummed))

    PFCB Fcb;
    PIRP Irp;
    PFOBX Fobx;
    BOOLEAN Locked;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;
    FILE_INFORMATION_CLASS FileInfoClass;

    PAGED_CODE();

    Fcb = (PFCB)Context->pFcb;
    Fobx = (PFOBX)Context->pFobx;
    DPRINT("RxCommonQueryInformation(%p) FCB: %p, FOBX: %p\n", Context, Fcb, Fobx);

    Irp = Context->CurrentIrp;
    Stack = Context->CurrentIrpSp;
    DPRINT("Buffer: %p, Length: %lx, Class: %ld\n", Irp->AssociatedIrp.SystemBuffer,
            Stack->Parameters.QueryFile.Length, Stack->Parameters.QueryFile.FileInformationClass);

    Context->Info.Length = Stack->Parameters.QueryFile.Length;
    FileInfoClass = Stack->Parameters.QueryFile.FileInformationClass;

    Locked  = FALSE;
    _SEH2_TRY
    {
        PVOID Buffer;

        /* Get a writable buffer */
        Buffer = RxMapSystemBuffer(Context);
        if (Buffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }
        /* Zero it */
        RtlZeroMemory(Buffer, Context->Info.Length);

        /* Validate file type */
        if (NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_UNKNOWN)
        {
            if (NodeType(Fcb) < RDBSS_NTC_STORAGE_TYPE_DIRECTORY)
            {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }
            else if (NodeType(Fcb) > RDBSS_NTC_STORAGE_TYPE_FILE)
            {
                if (NodeType(Fcb) == RDBSS_NTC_MAILSLOT)
                {
                    Status = STATUS_NOT_IMPLEMENTED;
                }
                else
                {
                    Status = STATUS_INVALID_PARAMETER;
                }

                _SEH2_LEAVE;
            }
        }

        /* Acquire the right lock */
        if (!BooleanFlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE) &&
            FileInfoClass != FileNameInformation)
        {
            if (FileInfoClass == FileCompressionInformation)
            {
                Status = RxAcquireExclusiveFcb(Context, Fcb);
            }
            else
            {
                Status = RxAcquireSharedFcb(Context, Fcb);
            }

            if (Status == STATUS_LOCK_NOT_GRANTED)
            {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
            else if (!NT_SUCCESS(Status))
            {
                _SEH2_LEAVE;
            }

            Locked = TRUE;
        }

        /* Dispatch to the right helper */
        switch (FileInfoClass)
        {
            case FileBasicInformation:
                Status = RxQueryBasicInfo(Context, Buffer);
                break;

            case FileStandardInformation:
                Status = RxQueryStandardInfo(Context, Buffer);
                break;

            case FileInternalInformation:
                Status = RxQueryInternalInfo(Context, Buffer);
                break;

            case FileEaInformation:
                Status = RxQueryEaInfo(Context, Buffer);
                break;

            case FileNameInformation:
                Status = RxQueryNameInfo(Context, Buffer);
                break;

            case FileAllInformation:
                SET_SIZE_AND_QUERY(0, RxQueryBasicInfo);
                if (!NT_SUCCESS(Status))
                {
                    break;
                }

                SET_SIZE_AND_QUERY(sizeof(FILE_BASIC_INFORMATION), RxQueryStandardInfo);
                if (!NT_SUCCESS(Status))
                {
                    break;
                }

                SET_SIZE_AND_QUERY(sizeof(FILE_BASIC_INFORMATION) +
                                   sizeof(FILE_STANDARD_INFORMATION), RxQueryInternalInfo);
                if (!NT_SUCCESS(Status))
                {
                    break;
                }

                SET_SIZE_AND_QUERY(sizeof(FILE_BASIC_INFORMATION) +
                                   sizeof(FILE_STANDARD_INFORMATION) +
                                   sizeof(FILE_INTERNAL_INFORMATION), RxQueryEaInfo);
                if (!NT_SUCCESS(Status))
                {
                    break;
                }

                SET_SIZE_AND_QUERY(sizeof(FILE_BASIC_INFORMATION) +
                                   sizeof(FILE_STANDARD_INFORMATION) +
                                   sizeof(FILE_INTERNAL_INFORMATION) +
                                   sizeof(FILE_EA_INFORMATION), RxQueryPositionInfo);
                if (!NT_SUCCESS(Status))
                {
                    break;
                }

                SET_SIZE_AND_QUERY(sizeof(FILE_BASIC_INFORMATION) +
                                   sizeof(FILE_STANDARD_INFORMATION) +
                                   sizeof(FILE_INTERNAL_INFORMATION) +
                                   sizeof(FILE_EA_INFORMATION) +
                                   sizeof(FILE_POSITION_INFORMATION), RxQueryNameInfo);
                break;

            case FileAlternateNameInformation:
                Status = RxQueryAlternateNameInfo(Context, Buffer);
                break;

            case FilePipeInformation:
            case FilePipeLocalInformation:
            case FilePipeRemoteInformation:
                Status = RxQueryPipeInfo(Context, Buffer);
                break;

            case FileCompressionInformation:
                Status = RxQueryCompressedInfo(Context, Buffer);
                break;

            default:
                Context->IoStatusBlock.Status = RxpQueryInfoMiniRdr(Context, FileInfoClass, Buffer);
                Status = Context->IoStatusBlock.Status;
                break;
        }

        if (Context->Info.Length < 0)
        {
            Status = STATUS_BUFFER_OVERFLOW;
            Context->Info.Length = Stack->Parameters.QueryFile.Length;
        }

        Irp->IoStatus.Information = Stack->Parameters.QueryFile.Length - Context->Info.Length;
    }
    _SEH2_FINALLY
    {
        if (Locked)
        {
            RxReleaseFcb(Context, Fcb);
        }
    }
    _SEH2_END;

    DPRINT("Status: %x\n", Status);
    return Status;

#undef SET_SIZE_AND_QUERY
}

NTSTATUS
NTAPI
RxCommonQueryQuotaInformation(
    PRX_CONTEXT Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RxCommonQuerySecurity(
    PRX_CONTEXT Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxCommonQueryVolumeInformation(
    PRX_CONTEXT Context)
{
    PIRP Irp;
    PFCB Fcb;
    PFOBX Fobx;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;

    PAGED_CODE();

    Fcb = (PFCB)Context->pFcb;
    Fobx = (PFOBX)Context->pFobx;

    DPRINT("RxCommonQueryVolumeInformation(%p) FCB: %p, FOBX: %p\n", Context, Fcb, Fobx);

    Irp = Context->CurrentIrp;
    Stack = Context->CurrentIrpSp;
    DPRINT("Length: %lx, Class: %lx, Buffer %p\n", Stack->Parameters.QueryVolume.Length,
           Stack->Parameters.QueryVolume.FsInformationClass, Irp->AssociatedIrp.SystemBuffer);

    Context->Info.FsInformationClass = Stack->Parameters.QueryVolume.FsInformationClass;
    Context->Info.Buffer = Irp->AssociatedIrp.SystemBuffer;
    Context->Info.Length = Stack->Parameters.QueryVolume.Length;

    /* Forward to mini-rdr */
    MINIRDR_CALL(Status, Context, Fcb->MRxDispatch, MRxQueryVolumeInfo, (Context));

    /* Post request if mini-rdr asked to */
    if (Context->PostRequest)
    {
        Status = RxFsdPostRequest(Context);
    }
    else
    {
        Irp->IoStatus.Information = Stack->Parameters.QueryVolume.Length - Context->Info.Length;
    }

    DPRINT("Status: %x\n", Status);
    return Status;
}

NTSTATUS
NTAPI
RxCommonRead(
    PRX_CONTEXT RxContext)
{
    PFCB Fcb;
    PIRP Irp;
    PFOBX Fobx;
    NTSTATUS Status;
    PNET_ROOT NetRoot;
    PVOID SystemBuffer;
    PFILE_OBJECT FileObject;
    LARGE_INTEGER ByteOffset;
    PIO_STACK_LOCATION Stack;
    PLOWIO_CONTEXT LowIoContext;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;
    ULONG ReadLength, CapturedRxContextSerialNumber = RxContext->SerialNumber;
    BOOLEAN CanWait, PagingIo, NoCache, Sync, PostRequest, IsPipe, ReadCachingEnabled, ReadCachingDisabled, InFsp, OwnerSet;

    PAGED_CODE();

    Fcb = (PFCB)RxContext->pFcb;
    Fobx = (PFOBX)RxContext->pFobx;
    DPRINT("RxCommonRead(%p) FOBX: %p, FCB: %p\n", RxContext, Fobx, Fcb);

    /* Get some parameters */
    Irp = RxContext->CurrentIrp;
    Stack = RxContext->CurrentIrpSp;
    CanWait = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    PagingIo = BooleanFlagOn(Irp->Flags, IRP_PAGING_IO);
    NoCache = BooleanFlagOn(Irp->Flags, IRP_NOCACHE);
    Sync = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION);
    InFsp = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP);
    ReadLength = Stack->Parameters.Read.Length;
    ByteOffset.QuadPart = Stack->Parameters.Read.ByteOffset.QuadPart;
    DPRINT("Reading: %lx@%I64x %s %s %s %s\n", ReadLength, ByteOffset.QuadPart,
           (CanWait ? "CW" : "!CW"), (PagingIo ? "PI" : "!PI"), (NoCache ? "NC" : "!NC"), (Sync ? "S" : "!S"));

    RxItsTheSameContext();

    Irp->IoStatus.Information = 0;

    /* Should the read be loud - so far, it's just ignored on ReactOS:
     * s/DPRINT/DPRINT1/g will make it loud
     */
    LowIoContext = &RxContext->LowIoContext;
    CheckForLoudOperations(RxContext);
    if (BooleanFlagOn(LowIoContext->Flags, LOWIO_CONTEXT_FLAG_LOUDOPS))
    {
        DPRINT("LoudRead %I64x/%lx on %lx vdl/size/alloc %I64x/%I64x/%I64x\n",
                ByteOffset, ReadLength,
                Fcb, Fcb->Header.ValidDataLength, Fcb->Header.FileSize, Fcb->Header.AllocationSize);
    }

    RxDeviceObject = RxContext->RxDeviceObject;
    /* Update stats */
    if (!BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP) && Fcb->CachedNetRootType == NET_ROOT_DISK)
    {
        InterlockedIncrement((volatile long *)&RxDeviceObject->ReadOperations);

        if (ByteOffset.QuadPart != Fobx->Specific.DiskFile.PredictedReadOffset)
        {
            InterlockedIncrement((volatile long *)&RxDeviceObject->RandomReadOperations);
        }
        Fobx->Specific.DiskFile.PredictedReadOffset = ByteOffset.QuadPart + ReadLength;

        if (PagingIo)
        {
            ExInterlockedAddLargeStatistic(&RxDeviceObject->PagingReadBytesRequested, ReadLength);
        }
        else if (NoCache)
        {
            ExInterlockedAddLargeStatistic(&RxDeviceObject->NonPagingReadBytesRequested, ReadLength);
        }
        else
        {
            ExInterlockedAddLargeStatistic(&RxDeviceObject->CacheReadBytesRequested, ReadLength);
        }
    }

    /* A pagefile cannot be a pipe */
    IsPipe = Fcb->NetRoot->Type == NET_ROOT_PIPE;
    if (IsPipe && PagingIo)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Null-length read is no-op */
    if (ReadLength == 0)
    {
        return STATUS_SUCCESS;
    }

    /* Validate FCB type */
    if (NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_FILE && NodeType(Fcb) != RDBSS_NTC_VOLUME_FCB)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Init the lowio context for possible forward */
    RxInitializeLowIoContext(LowIoContext, LOWIO_OP_READ);

    PostRequest = FALSE;
    ReadCachingDisabled = FALSE;
    OwnerSet = FALSE;
    ReadCachingEnabled = BooleanFlagOn(Fcb->FcbState, FCB_STATE_READCACHING_ENABLED);
    FileObject = Stack->FileObject;
    NetRoot = (PNET_ROOT)Fcb->pNetRoot;
    _SEH2_TRY
    {
        LONGLONG FileSize;

        /* If no caching, make sure current Cc data have been flushed */
        if (!PagingIo && NoCache && !ReadCachingEnabled && FileObject->SectionObjectPointer != NULL)
        {
            Status = RxAcquireExclusiveFcb(RxContext, Fcb);
            if (Status == STATUS_LOCK_NOT_GRANTED)
            {
                PostRequest = TRUE;
                _SEH2_LEAVE;
            }
            else if (Status != STATUS_SUCCESS)
            {
                _SEH2_LEAVE;
            }

            ExAcquireResourceSharedLite(Fcb->Header.PagingIoResource, TRUE);
            CcFlushCache(FileObject->SectionObjectPointer, &ByteOffset, ReadLength, &Irp->IoStatus);
            RxReleasePagingIoResource(RxContext, Fcb);

            if (!NT_SUCCESS(Irp->IoStatus.Status))
            {
                Status = Irp->IoStatus.Status;
                _SEH2_LEAVE;
            }

            RxAcquirePagingIoResource(RxContext, Fcb);
            RxReleasePagingIoResource(RxContext, Fcb);
        }

        /* Acquire the appropriate lock */
        if (PagingIo && !ReadCachingEnabled)
        {
            ASSERT(!IsPipe);

            if (!ExAcquireResourceSharedLite(Fcb->Header.PagingIoResource, CanWait))
            {
                PostRequest = TRUE;
                _SEH2_LEAVE;
            }

            if (!CanWait)
            {
                LowIoContext->Resource = Fcb->Header.PagingIoResource;
            }
        }
        else
        {
            if (!ReadCachingEnabled)
            {
                if (!CanWait && NoCache)
                {
                    Status = RxAcquireSharedFcbWaitForEx(RxContext, Fcb);
                    if (Status == STATUS_LOCK_NOT_GRANTED)
                    {
                        DPRINT1("RdAsyLNG %x\n", RxContext);
                        PostRequest = TRUE;
                        _SEH2_LEAVE;
                    }
                    if (Status != STATUS_SUCCESS)
                    {
                        DPRINT1("RdAsyOthr %x\n", RxContext);
                        _SEH2_LEAVE;
                    }

                    if (RxIsFcbAcquiredShared(Fcb) <= 0xF000)
                    {
                        LowIoContext->Resource = Fcb->Header.Resource;
                    }
                    else
                    {
                        PostRequest = TRUE;
                        _SEH2_LEAVE;
                    }
                }
                else
                {
                    Status = RxAcquireSharedFcb(RxContext, Fcb);
                    if (Status == STATUS_LOCK_NOT_GRANTED)
                    {
                        PostRequest = TRUE;
                        _SEH2_LEAVE;
                    }
                    else if (Status != STATUS_SUCCESS)
                    {
                        _SEH2_LEAVE;
                    }
                }
            }
        }

        RxItsTheSameContext();

        ReadCachingDisabled = (ReadCachingEnabled == FALSE);
        if (IsPipe)
        {
            UNIMPLEMENTED;
        }

        RxGetFileSizeWithLock(Fcb, &FileSize);

        /* Make sure FLOCK doesn't conflict */
        if (!PagingIo)
        {
            if (!FsRtlCheckLockForReadAccess(&Fcb->Specific.Fcb.FileLock, Irp))
            {
                Status = STATUS_FILE_LOCK_CONFLICT;
                _SEH2_LEAVE;
            }
        }

        /* Validate byteoffset vs length */
        if (BooleanFlagOn(Fcb->FcbState, FCB_STATE_READCACHING_ENABLED))
        {
            if (ByteOffset.QuadPart >= FileSize)
            {
                Status = STATUS_END_OF_FILE;
                _SEH2_LEAVE;
            }

            if (ReadLength > FileSize - ByteOffset.QuadPart)
            {
                ReadLength = FileSize - ByteOffset.QuadPart;
            }
        }

        /* Read with Cc! */
        if (!PagingIo && !NoCache && ReadCachingEnabled &&
            !BooleanFlagOn(Fobx->pSrvOpen->Flags, SRVOPEN_FLAG_DONTUSE_READ_CACHING))
        {
            /* File was not cached yet, do it */
            if (FileObject->PrivateCacheMap == NULL)
            {
                if (BooleanFlagOn(FileObject->Flags, FO_CLEANUP_COMPLETE))
                {
                    Status = STATUS_FILE_CLOSED;
                    _SEH2_LEAVE;
                }

                RxAdjustAllocationSizeforCC(Fcb);

                CcInitializeCacheMap(FileObject, (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                                     FALSE, &RxData.CacheManagerCallbacks, Fcb);

                if (BooleanFlagOn(Fcb->MRxDispatch->MRxFlags, RDBSS_NO_DEFERRED_CACHE_READAHEAD))
                {
                    CcSetAdditionalCacheAttributes(FileObject, FALSE, FALSE);
                }
                else
                {
                    CcSetAdditionalCacheAttributes(FileObject, TRUE, FALSE);
                    SetFlag(Fcb->FcbState, FCB_STATE_READAHEAD_DEFERRED);
                }

                CcSetReadAheadGranularity(FileObject, NetRoot->DiskParameters.ReadAheadGranularity);
            }

            /* This should never happen - fix your RDR */
            if (BooleanFlagOn(RxContext->MinorFunction, IRP_MN_MDL))
            {
                ASSERT(FALSE);
                ASSERT(CanWait);

                CcMdlRead(FileObject, &ByteOffset, ReadLength, &Irp->MdlAddress, &Irp->IoStatus);
                Status = Irp->IoStatus.Status;
                ASSERT(NT_SUCCESS(Status));
            }
            else
            {
                /* Map buffer */
                SystemBuffer = RxNewMapUserBuffer(RxContext);
                if (SystemBuffer == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    _SEH2_LEAVE;
                }

                SetFlag(Fcb->FcbState, FCB_STATE_READCACHING_ENABLED);

                RxItsTheSameContext();

                /* Perform the read */
                if (!CcCopyRead(FileObject, &ByteOffset, ReadLength, CanWait, SystemBuffer, &Irp->IoStatus))
                {
                    if (!ReadCachingEnabled)
                    {
                        ClearFlag(Fcb->FcbState, FCB_STATE_READCACHING_ENABLED);
                    }

                    RxItsTheSameContext();

                    PostRequest = TRUE;
                    _SEH2_LEAVE;
                }

                if (!ReadCachingEnabled)
                {
                    ClearFlag(Fcb->FcbState, FCB_STATE_READCACHING_ENABLED);
                }

                Status = Irp->IoStatus.Status;
                ASSERT(NT_SUCCESS(Status));
            }
        }
        else
        {
            /* Validate the reading */
            if (FileObject->PrivateCacheMap != NULL && BooleanFlagOn(Fcb->FcbState, FCB_STATE_READAHEAD_DEFERRED) &&
                ByteOffset.QuadPart >= 4096)
            {
                CcSetAdditionalCacheAttributes(FileObject, FALSE, FALSE);
                ClearFlag(Fcb->FcbState, FCB_STATE_READAHEAD_DEFERRED);
            }

            /* If it's consistent, forward to mini-rdr */
            if (Fcb->CachedNetRootType != NET_ROOT_DISK || BooleanFlagOn(Fcb->FcbState, FCB_STATE_READAHEAD_DEFERRED) ||
                ByteOffset.QuadPart < Fcb->Header.ValidDataLength.QuadPart)
            {
                LowIoContext->ParamsFor.ReadWrite.ByteCount = ReadLength;
                LowIoContext->ParamsFor.ReadWrite.ByteOffset = ByteOffset.QuadPart;

                RxItsTheSameContext();

                if (InFsp && ReadCachingDisabled)
                {
                    ExSetResourceOwnerPointer((PagingIo ? Fcb->Header.PagingIoResource : Fcb->Header.Resource),
                                              (PVOID)((ULONG_PTR)RxContext | 3));
                    OwnerSet = TRUE;
                }

                Status = RxLowIoReadShell(RxContext);

                RxItsTheSameContext();
            }
            else
            {
                if (ByteOffset.QuadPart > FileSize)
                {
                    ReadLength = 0;
                    Irp->IoStatus.Information = ReadLength;
                    _SEH2_LEAVE;
                }

                if (ByteOffset.QuadPart + ReadLength > FileSize)
                {
                    ReadLength = FileSize - ByteOffset.QuadPart;
                }

                SystemBuffer = RxNewMapUserBuffer(RxContext);
                RtlZeroMemory(SystemBuffer, ReadLength);
                Irp->IoStatus.Information = ReadLength;
            }
        }
    }
    _SEH2_FINALLY
    {
        RxItsTheSameContext();

        /* Post if required */
        if (PostRequest)
        {
            InterlockedIncrement((volatile long *)&RxContext->ReferenceCount);
            Status = RxFsdPostRequest(RxContext);
        }
        else
        {
            /* Update FO in case of sync IO */
            if (!IsPipe && !PagingIo)
            {
                if (BooleanFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO))
                {
                    FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Irp->IoStatus.Information;
                }
            }
        }

        /* Set FastIo if read was a success */
        if (NT_SUCCESS(Status) && Status != STATUS_PENDING)
        {
            if (!IsPipe && !PagingIo)
            {
                SetFlag(FileObject->Flags, FO_FILE_FAST_IO_READ);
            }
        }

        /* In case we're done (not expected any further processing */
        if (_SEH2_AbnormalTermination() || Status != STATUS_PENDING || PostRequest)
        {
            /* Release everything that can be */
            if (ReadCachingDisabled)
            {
                if (PagingIo)
                {
                    if (OwnerSet)
                    {
                        RxReleasePagingIoResourceForThread(RxContext, Fcb, LowIoContext->ResourceThreadId);
                    }
                    else
                    {
                        RxReleasePagingIoResource(RxContext, Fcb);
                    }
                }
                else
                {
                    if (OwnerSet)
                    {
                        RxReleaseFcbForThread(RxContext, Fcb, LowIoContext->ResourceThreadId);
                    }
                    else
                    {
                        RxReleaseFcb(RxContext, Fcb);
                    }
                }
            }

            /* Dereference/Delete context */
            if (PostRequest)
            {
                RxDereferenceAndDeleteRxContext(RxContext);
            }
            else
            {
                if (BooleanFlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION))
                {
                    RxResumeBlockedOperations_Serially(RxContext, &Fobx->Specific.NamedPipe.ReadSerializationQueue);
                }
            }

            /* We cannot return more than asked */
            if (Status == STATUS_SUCCESS)
            {
                ASSERT(Irp->IoStatus.Information <= Stack->Parameters.Read.Length);
            }
        }
        else
        {
            ASSERT(!Sync);

            RxDereferenceAndDeleteRxContext(RxContext);
        }
    }
    _SEH2_END;

    return Status;
}

NTSTATUS
NTAPI
RxCommonSetEa(
    PRX_CONTEXT Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxCommonSetInformation(
    PRX_CONTEXT Context)
{
    PIRP Irp;
    PFCB Fcb;
    PFOBX Fobx;
    NTSTATUS Status;
    PNET_ROOT NetRoot;
    PIO_STACK_LOCATION Stack;
    FILE_INFORMATION_CLASS Class;
    BOOLEAN CanWait, FcbTableAcquired, FcbAcquired;

    PAGED_CODE();

    Fcb = (PFCB)Context->pFcb;
    Fobx = (PFOBX)Context->pFobx;
    DPRINT("RxCommonSetInformation(%p), FCB: %p, FOBX: %p\n", Context, Fcb, Fobx);

    Irp = Context->CurrentIrp;
    Stack = Context->CurrentIrpSp;
    Class = Stack->Parameters.SetFile.FileInformationClass;
    DPRINT("Buffer: %p, Length: %lx, Class: %ld, ReplaceIfExists: %d\n",
           Irp->AssociatedIrp.SystemBuffer, Stack->Parameters.SetFile.Length,
           Class, Stack->Parameters.SetFile.ReplaceIfExists);

    Status = STATUS_SUCCESS;
    CanWait = BooleanFlagOn(Context->Flags, RX_CONTEXT_FLAG_WAIT);
    FcbTableAcquired = FALSE;
    FcbAcquired = FALSE;
    NetRoot = (PNET_ROOT)Fcb->pNetRoot;

#define _SEH2_TRY_RETURN(S) S; goto try_exit

    _SEH2_TRY
    {
        /* Valide the node type first */
        if (NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_UNKNOWN &&
            NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_DIRECTORY)
        {
            if (NodeType(Fcb) == RDBSS_NTC_STORAGE_TYPE_FILE)
            {
                if (!BooleanFlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE))
                {
                    Status = STATUS_SUCCESS;
                }
            }
            else if (NodeType(Fcb) != RDBSS_NTC_SPOOLFILE)
            {
                if (NodeType(Fcb) == RDBSS_NTC_MAILSLOT)
                {
                    _SEH2_TRY_RETURN(Status = STATUS_NOT_IMPLEMENTED);
                }
                else
                {
                    DPRINT1("Illegal type of file provided: %x\n", NodeType(Fcb));
                    _SEH2_TRY_RETURN(Status = STATUS_INVALID_PARAMETER);
                }
            }
        }

        /* We don't autorize advance operation */
        if (Class == FileEndOfFileInformation && Stack->Parameters.SetFile.AdvanceOnly)
        {
            DPRINT1("Not allowed\n");

            _SEH2_TRY_RETURN(Status = STATUS_SUCCESS);
        }

        /* For these to classes, we'll have to deal with the FCB table (removal)
         * We thus need the exclusive FCB table lock
         */
        if (Class == FileDispositionInformation || Class == FileRenameInformation)
        {
            RxPurgeRelatedFobxs(NetRoot, Context, TRUE, Fcb);
            RxScavengeFobxsForNetRoot(NetRoot, Fcb, TRUE);

            if (!RxAcquireFcbTableLockExclusive(&NetRoot->FcbTable, CanWait))
            {
                Context->PostRequest = TRUE;
                _SEH2_TRY_RETURN(Status = STATUS_PENDING);
            }

            FcbTableAcquired = TRUE;
        }

        /* Finally, if not paging file, we need exclusive FCB lock */
        if (!BooleanFlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE))
        {
            Status = RxAcquireExclusiveFcb(Context, Fcb);
            if (Status == STATUS_LOCK_NOT_GRANTED)
            {
                Context->PostRequest = TRUE;
                _SEH2_TRY_RETURN(Status = STATUS_SUCCESS);
            }
            else if (Status != STATUS_SUCCESS)
            {
                _SEH2_LEAVE;
            }

            FcbAcquired = TRUE;
        }

        Status = STATUS_SUCCESS;

        /* And now, perform the job! */
        switch (Class)
        {
            case FileBasicInformation:
                Status = RxSetBasicInfo(Context);
                break;

            case FileDispositionInformation:
            {
                PFILE_DISPOSITION_INFORMATION FDI;

                /* Check whether user wants deletion */
                FDI = Irp->AssociatedIrp.SystemBuffer;
                if (FDI->DeleteFile)
                {
                    /* If so, check whether it's doable */
                    if (!MmFlushImageSection(&Fcb->NonPaged->SectionObjectPointers, MmFlushForDelete))
                    {
                        Status = STATUS_CANNOT_DELETE;
                    }

                    /* And if doable, already remove from FCB table */
                    if (Status == STATUS_SUCCESS)
                    {
                        ASSERT(FcbAcquired && FcbTableAcquired);
                        RxRemoveNameNetFcb(Fcb);

                        RxReleaseFcbTableLock(&NetRoot->FcbTable);
                        FcbTableAcquired = FALSE;
                    }
                }

                /* If it succeed, perform the operation */
                if (Status == STATUS_SUCCESS)
                {
                    Status = RxSetDispositionInfo(Context);
                }

                break;
            }

            case FilePositionInformation:
                Status = RxSetPositionInfo(Context);
                break;

            case FileAllocationInformation:
                Status = RxSetAllocationInfo(Context);
                break;

            case FileEndOfFileInformation:
                Status = RxSetEndOfFileInfo(Context);
                break;

            case FilePipeInformation:
            case FilePipeLocalInformation:
            case FilePipeRemoteInformation:
                Status = RxSetPipeInfo(Context);
                break;

            case FileRenameInformation:
            case FileLinkInformation:
            case FileMoveClusterInformation:
                /* If we can wait, try to perform the operation right now */
                if (CanWait)
                {
                    /* Of course, collapsing is not doable anymore, file is
                     * in an inbetween state
                     */
                    ClearFlag(Fcb->FcbState, FCB_STATE_COLLAPSING_ENABLED);

                    /* Set the information */
                    Status = RxSetRenameInfo(Context);
                    /* If it succeed, drop the current entry from FCB table */
                    if (Status == STATUS_SUCCESS && Class == FileRenameInformation)
                    {
                        ASSERT(FcbAcquired && FcbTableAcquired);
                        RxRemoveNameNetFcb(Fcb);
                    }
                    _SEH2_TRY_RETURN(Status);
                }
                /* Can't wait? Post for async retry */
                else
                {
                    Status = RxFsdPostRequest(Context);
                    _SEH2_TRY_RETURN(Status);
                }
                break;

            case FileValidDataLengthInformation:
                if (!MmCanFileBeTruncated(&Fcb->NonPaged->SectionObjectPointers, NULL))
                {
                    Status = STATUS_USER_MAPPED_FILE;
                }
                break;

            case FileShortNameInformation:
                Status = RxSetSimpleInfo(Context);
                break;

            default:
                DPRINT1("Insupported class: %x\n", Class);
                Status = STATUS_INVALID_PARAMETER;

                break;
        }

try_exit: NOTHING;
        /* If mini-rdr was OK and wants a re-post on this, do it */
        if (Status == STATUS_SUCCESS)
        {
            if (Context->PostRequest)
            {
                Status = RxFsdPostRequest(Context);
            }
        }
    }
    _SEH2_FINALLY
    {
        /* Release any acquired lock */
        if (FcbAcquired)
        {
            RxReleaseFcb(Context, Fcb);
        }

        if (FcbTableAcquired)
        {
            RxReleaseFcbTableLock(&NetRoot->FcbTable);
        }
    }
    _SEH2_END;

#undef _SEH2_TRY_RETURN

    return Status;
}

NTSTATUS
NTAPI
RxCommonSetQuotaInformation(
    PRX_CONTEXT Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RxCommonSetSecurity(
    PRX_CONTEXT Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RxCommonSetVolumeInformation(
    PRX_CONTEXT Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RxCommonUnimplemented(
    PRX_CONTEXT Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RxCommonWrite(
    PRX_CONTEXT RxContext)
{
    PIRP Irp;
    PFCB Fcb;
    PFOBX Fobx;
    NTSTATUS Status;
    PNET_ROOT NetRoot;
    PSRV_OPEN SrvOpen;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;
    LARGE_INTEGER ByteOffset;
    NODE_TYPE_CODE NodeTypeCode;
    PLOWIO_CONTEXT LowIoContext;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;
    ULONG WriteLength, CapturedRxContextSerialNumber = RxContext->SerialNumber;
    LONGLONG FileSize, ValidDataLength, InitialFileSize, InitialValidDataLength;
    BOOLEAN CanWait, PagingIo, NoCache, Sync, NormalFile, WriteToEof, IsPipe, NoPreposting, InFsp, RecursiveWriteThrough, CalledByLazyWriter, SwitchBackToAsync, ExtendingFile, ExtendingValidData, UnwindOutstandingAsync, ResourceOwnerSet, PostIrp, ContextReferenced;

    PAGED_CODE();

    Fcb = (PFCB)RxContext->pFcb;
    NodeTypeCode = NodeType(Fcb);
    /* Validate FCB type */
    if (NodeTypeCode != RDBSS_NTC_STORAGE_TYPE_FILE && NodeTypeCode != RDBSS_NTC_VOLUME_FCB &&
        NodeTypeCode != RDBSS_NTC_SPOOLFILE && NodeTypeCode != RDBSS_NTC_MAILSLOT)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* We'll write to file, keep track of it */
    Fcb->IsFileWritten = TRUE;

    Stack = RxContext->CurrentIrpSp;
    /* Set write through if asked */
    if (BooleanFlagOn(Stack->Flags, SL_WRITE_THROUGH))
    {
        SetFlag(RxContext->Flags, RX_CONTEXT_FLAG_WRITE_THROUGH);
    }

    Fobx = (PFOBX)RxContext->pFobx;
    DPRINT("RxCommonWrite(%p) FOBX: %p, FCB: %p\n", RxContext, Fobx, Fcb);

    /* Get some parameters */
    Irp = RxContext->CurrentIrp;
    NoPreposting = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_NO_PREPOSTING_NEEDED);
    InFsp = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP);
    CanWait = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    PagingIo = BooleanFlagOn(Irp->Flags, IRP_PAGING_IO);
    NoCache = BooleanFlagOn(Irp->Flags, IRP_NOCACHE);
    Sync = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION);
    WriteLength = Stack->Parameters.Write.Length;
    ByteOffset.QuadPart = Stack->Parameters.Write.ByteOffset.QuadPart;
    DPRINT("Writing: %lx@%I64x %s %s %s %s\n", WriteLength, ByteOffset.QuadPart,
           (CanWait ? "CW" : "!CW"), (PagingIo ? "PI" : "!PI"), (NoCache ? "NC" : "!NC"), (Sync ? "S" : "!S"));

    RxItsTheSameContext();

    RxContext->FcbResourceAcquired = FALSE;
    RxContext->FcbPagingIoResourceAcquired = FALSE;

    LowIoContext = &RxContext->LowIoContext;
    CheckForLoudOperations(RxContext);
    if (BooleanFlagOn(LowIoContext->Flags, LOWIO_CONTEXT_FLAG_LOUDOPS))
    {
        DPRINT("LoudWrite %I64x/%lx on %lx vdl/size/alloc %I64x/%I64x/%I64x\n",
                ByteOffset, WriteLength,
                Fcb, Fcb->Header.ValidDataLength, Fcb->Header.FileSize, Fcb->Header.AllocationSize);
    }

    RxDeviceObject = RxContext->RxDeviceObject;
    /* Update stats */
    if (!BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP) && Fcb->CachedNetRootType == NET_ROOT_DISK)
    {
        InterlockedIncrement((volatile long *)&RxDeviceObject->WriteOperations);

        if (ByteOffset.QuadPart != Fobx->Specific.DiskFile.PredictedWriteOffset)
        {
            InterlockedIncrement((volatile long *)&RxDeviceObject->RandomWriteOperations);
        }
        Fobx->Specific.DiskFile.PredictedWriteOffset = ByteOffset.QuadPart + WriteLength;

        if (PagingIo)
        {
            ExInterlockedAddLargeStatistic(&RxDeviceObject->PagingWriteBytesRequested, WriteLength);
        }
        else if (NoCache)
        {
            ExInterlockedAddLargeStatistic(&RxDeviceObject->NonPagingWriteBytesRequested, WriteLength);
        }
        else
        {
            ExInterlockedAddLargeStatistic(&RxDeviceObject->CacheWriteBytesRequested, WriteLength);
        }
    }

    NetRoot = (PNET_ROOT)Fcb->NetRoot;
    IsPipe = (NetRoot->Type == NET_ROOT_PIPE);
    /* Keep track for normal writes */
    if (NetRoot->Type == NET_ROOT_DISK || NetRoot->Type == NET_ROOT_WILD)
    {
        NormalFile = TRUE;
    }
    else
    {
        NormalFile = FALSE;
    }

    /* Zero-length write is immediate success */
    if (NormalFile && WriteLength == 0)
    {
        return STATUS_SUCCESS;
    }

    /* Check whether we have input data */
    if (Irp->UserBuffer == NULL && Irp->MdlAddress == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Are we writing to EOF? */
    WriteToEof = ((ByteOffset.LowPart == FILE_WRITE_TO_END_OF_FILE) && (ByteOffset.HighPart == -1));
    /* FIXME: validate length/offset */

    /* Get our SRV_OPEN in case of normal write */
    if (Fobx != NULL)
    {
        SrvOpen = (PSRV_OPEN)Fobx->pSrvOpen;
    }
    else
    {
        SrvOpen = NULL;
    }

    FileObject = Stack->FileObject;

    /* If we have caching enabled, check whether we have to defer write */
    if (!NoCache)
    {
        if (RxWriteCacheingAllowed(Fcb, SrvOpen))
        {
            if (!CcCanIWrite(FileObject, WriteLength,
                             (CanWait && !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP)),
                             BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_DEFERRED_WRITE)))
            {
                BOOLEAN Retrying;

                Retrying = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_DEFERRED_WRITE);

                RxPrePostIrp(RxContext, Irp);

                SetFlag(RxContext->Flags, RX_CONTEXT_FLAG_DEFERRED_WRITE);

                CcDeferWrite(FileObject, (PCC_POST_DEFERRED_WRITE)RxAddToWorkque, RxContext, Irp, WriteLength, Retrying);

                return STATUS_PENDING;
            }
        }
    }

    /* Initialize the low IO context for write */
    RxInitializeLowIoContext(LowIoContext, LOWIO_OP_WRITE);

    /* Initialize our (many) booleans */
    RecursiveWriteThrough = FALSE;
    CalledByLazyWriter = FALSE;
    SwitchBackToAsync = FALSE;
    ExtendingFile = FALSE;
    ExtendingValidData = FALSE;
    UnwindOutstandingAsync = FALSE;
    ResourceOwnerSet = FALSE;
    PostIrp = FALSE;
    ContextReferenced = FALSE;

#define _SEH2_TRY_RETURN(S) S; goto try_exit

    _SEH2_TRY
    {
        /* No volume FCB here! */
        ASSERT((NodeTypeCode == RDBSS_NTC_STORAGE_TYPE_FILE) ||
               (NodeTypeCode == RDBSS_NTC_SPOOLFILE) ||
               (NodeTypeCode == RDBSS_NTC_MAILSLOT));

        /* Writing to EOF on a paging file is non sense */
        ASSERT(!(WriteToEof && PagingIo));

        RxItsTheSameContext();

        /* Start locking stuff */
        if (!PagingIo && !NoPreposting)
        {
            /* If it's already acquired, all fine */
            if (RxContext->FcbResourceAcquired)
            {
                ASSERT(!IsPipe);
            }
            else
            {
                /* Otherwise, try to acquire shared (excepted for pipes) */
                if (IsPipe)
                {
                    Status = RxAcquireExclusiveFcb(RxContext, Fcb);
                }
                else if (CanWait ||
                         (!NoCache && RxWriteCacheingAllowed(Fcb, SrvOpen)))
                {
                    Status = RxAcquireSharedFcb(RxContext, Fcb);
                }
                else
                {
                    Status = RxAcquireSharedFcbWaitForEx(RxContext, Fcb);
                }

                /* We'll post IRP to retry */
                if (Status == STATUS_LOCK_NOT_GRANTED)
                {
                    PostIrp = TRUE;
                    DPRINT1("Failed to acquire lock!\n");
                    _SEH2_TRY_RETURN(Status);
                }

                /* We'll just fail */
                if (Status != STATUS_SUCCESS)
                {
                    _SEH2_TRY_RETURN(Status);
                }

                /* Resource acquired */
                RxContext->FcbResourceAcquired = TRUE;
            }

            /* At that point, resource is acquired */
            if (IsPipe)
            {
                ASSERT(RxContext->FcbResourceAcquired);
            }
            else
            {
                BOOLEAN IsDormant;

                /* Now, check whether we have to promote shared lock */
                if (NodeTypeCode == RDBSS_NTC_STORAGE_TYPE_FILE && Fobx != NULL)
                {
                    IsDormant = BooleanFlagOn(Fobx->Flags, FOBX_FLAG_MARKED_AS_DORMANT);
                }
                else
                {
                    IsDormant = FALSE;
                }

                /* We're writing beyond VDL, we'll need an exclusive lock if not dormant */
                if (RxIsFcbAcquiredShared(Fcb) &&
                    ByteOffset.QuadPart + WriteLength > Fcb->Header.ValidDataLength.QuadPart)
                {
                    if (!IsDormant)
                    {
                        RxReleaseFcb(RxContext, Fcb);
                        RxContext->FcbResourceAcquired = FALSE;

                        Status = RxAcquireExclusiveFcb(RxContext, Fcb);
                        if (Status == STATUS_LOCK_NOT_GRANTED)
                        {
                            PostIrp = TRUE;
                            DPRINT1("Failed to acquire lock!\n");
                            _SEH2_TRY_RETURN(Status);
                        }

                        if (Status != STATUS_SUCCESS)
                        {
                            _SEH2_TRY_RETURN(Status);
                        }

                        RxContext->FcbResourceAcquired = TRUE;
                    }
                }

                /* If we're writing in VDL, or if we're dormant, shared lock is enough */
                if (ByteOffset.QuadPart + WriteLength <= Fcb->Header.ValidDataLength.QuadPart ||
                    IsDormant)
                {
                    if (RxIsFcbAcquiredExclusive(Fcb))
                    {
                        RxConvertToSharedFcb(RxContext, Fcb);
                    }
                }
                else
                {
                    /* We're extending file, disable collapsing */
                    ASSERT(RxIsFcbAcquiredExclusive(Fcb));

                    DPRINT("Disabling collapsing\n");

                    if (NodeTypeCode == RDBSS_NTC_STORAGE_TYPE_FILE && Fobx != NULL)
                    {
                        SetFlag(Fobx->Flags, FOBX_FLAG_DISABLE_COLLAPSING);
                    }
                }

                ASSERT(RxContext->FcbResourceAcquired);
            }

            /* Keep track of the acquired resource */
            LowIoContext->Resource = Fcb->Header.Resource;
        }
        else
        {
            /* Paging IO */
            ASSERT(!IsPipe);

            /* Lock the paging resource */
            RxAcquirePagingIoResourceShared(RxContext, Fcb, TRUE);

            /* Keep track of the acquired resource */
            LowIoContext->Resource = Fcb->Header.PagingIoResource;
        }

        if (IsPipe)
        {
            UNIMPLEMENTED;
            _SEH2_TRY_RETURN(Status = STATUS_NOT_IMPLEMENTED);
        }

        /* If it's a non cached write, or if caching is disallowed */
        if (NoCache || !RxWriteCacheingAllowed(Fcb, SrvOpen))
        {
            /* If cache was previously enabled, we'll have to flush before writing */
            if (!PagingIo && Fcb->NonPaged->SectionObjectPointers.DataSectionObject != NULL)
            {
                LARGE_INTEGER FlushOffset;

                /* FCB is lock */
                ASSERT(RxIsFcbAcquiredExclusive(Fcb) || RxIsFcbAcquiredShared(Fcb));

                /* If shared, we'll have to relock exclusive */
                if (!RxIsFcbAcquiredExclusive(Fcb))
                {
                    /* Release and retry exclusive */
                    RxReleaseFcb(RxContext, Fcb);
                    RxContext->FcbResourceAcquired = FALSE;

                    Status = RxAcquireExclusiveFcb(RxContext, Fcb);
                    if (Status == STATUS_LOCK_NOT_GRANTED)
                    {
                        PostIrp = TRUE;
                        DPRINT1("Failed to acquire lock for flush!\n");
                        _SEH2_TRY_RETURN(Status);
                    }

                    if (Status != STATUS_SUCCESS)
                    {
                        _SEH2_TRY_RETURN(Status);
                    }

                    RxContext->FcbResourceAcquired = TRUE;
                }

                /* Get the length to flush */
                if (WriteToEof)
                {
                    RxGetFileSizeWithLock(Fcb, &FlushOffset.QuadPart);
                }
                else
                {
                    FlushOffset.QuadPart = ByteOffset.QuadPart;
                }

                /* Perform the flushing */
                RxAcquirePagingIoResource(RxContext, Fcb);
                CcFlushCache(&Fcb->NonPaged->SectionObjectPointers, &FlushOffset,
                             WriteLength, &Irp->IoStatus);
                RxReleasePagingIoResource(RxContext, Fcb);

                /* Cannot continue if flushing failed */
                if (!NT_SUCCESS(Irp->IoStatus.Status))
                {
                    _SEH2_TRY_RETURN(Status = Irp->IoStatus.Status);
                }

                /* Synchronize */
                RxAcquirePagingIoResource(RxContext, Fcb);
                RxReleasePagingIoResource(RxContext, Fcb);

                /* And purge */
                CcPurgeCacheSection(&Fcb->NonPaged->SectionObjectPointers,
                                    &FlushOffset, WriteLength, FALSE);
            }
        }

        /* If not paging IO, check if write is allowed */
        if (!PagingIo)
        {
            if (!FsRtlCheckLockForWriteAccess(&Fcb->Specific.Fcb.FileLock, Irp))
            {
                _SEH2_TRY_RETURN(Status = STATUS_FILE_LOCK_CONFLICT);
            }
        }

        /* Get file sizes */
        ValidDataLength = Fcb->Header.ValidDataLength.QuadPart;
        RxGetFileSizeWithLock(Fcb, &FileSize);
        ASSERT(ValidDataLength <= FileSize);

        /* If paging IO, we cannot write past file size
         * so fix write length if needed
         */
        if (PagingIo)
        {
            if (ByteOffset.QuadPart >= FileSize)
            {
                _SEH2_TRY_RETURN(Status = STATUS_SUCCESS);
            }

            if (WriteLength > FileSize - ByteOffset.QuadPart)
            {
                WriteLength = FileSize - ByteOffset.QuadPart;
            }
        }

        /* If we're being called by the lazywrite */
        if (Fcb->Specific.Fcb.LazyWriteThread == PsGetCurrentThread())
        {
            CalledByLazyWriter = TRUE;

            /* Fail if we're beyong VDL */
            if (BooleanFlagOn(Fcb->Header.Flags, FSRTL_FLAG_USER_MAPPED_FILE))
            {
                if ((ByteOffset.QuadPart + WriteLength > ValidDataLength) &&
                    (ByteOffset.QuadPart < FileSize))
                {
                    if (ByteOffset.QuadPart + WriteLength > ((ValidDataLength + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1)))
                    {
                        _SEH2_TRY_RETURN(Status = STATUS_FILE_LOCK_CONFLICT);
                    }
                }
            }
        }

        /* If that's a recursive synchronous page write */
        if (BooleanFlagOn(Irp->Flags, IRP_SYNCHRONOUS_PAGING_IO) &&
            BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_RECURSIVE_CALL))
        {
            PIRP TopIrp;

            /* Check the top level IRP on the FastIO path */
            TopIrp = RxGetTopIrpIfRdbssIrp();
            if (TopIrp != NULL && (ULONG_PTR)TopIrp > FSRTL_FAST_IO_TOP_LEVEL_IRP)
            {
                PIO_STACK_LOCATION IrpStack;

                ASSERT(NodeType(TopIrp) == IO_TYPE_IRP);

                /* If the top level IRP was a cached write for this file, keep track */
                IrpStack = IoGetCurrentIrpStackLocation(TopIrp);
                if (IrpStack->MajorFunction == IRP_MJ_WRITE &&
                    IrpStack->FileObject->FsContext == FileObject->FsContext)
                {
                    RecursiveWriteThrough = TRUE;
                    SetFlag(RxContext->Flags, RX_CONTEXT_FLAG_WRITE_THROUGH);
                }
            }
        }

        /* Now, deal with file size and VDL */
        if (!CalledByLazyWriter && !RecursiveWriteThrough &&
            (WriteToEof || ByteOffset.QuadPart + WriteLength > ValidDataLength))
        {
            /* Not sync? Let's make it sync, just the time we extended */
            if (!Sync)
            {
                CanWait = TRUE;
                SetFlag(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
                ClearFlag(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION);
                Sync = TRUE;

                /* Keep track we'll have to switch back to async */
                if (NoCache)
                {
                    SwitchBackToAsync = TRUE;
                }
            }

            /* Release all the locks */
            RxWriteReleaseResources(RxContext, 0);

            /* Acquire exclusive */
            Status = RxAcquireExclusiveFcb(RxContext, Fcb);
            if (Status == STATUS_LOCK_NOT_GRANTED)
            {
                PostIrp = TRUE;
                DPRINT1("Failed to acquire lock for extension!\n");
                _SEH2_TRY_RETURN(Status);
            }

            if (Status != STATUS_SUCCESS)
            {
                _SEH2_TRY_RETURN(Status);
            }

            RxContext->FcbResourceAcquired = TRUE;

            RxItsTheSameContext();

            /* Get the sizes again, to be sure they didn't change in the meantime */
            ValidDataLength = Fcb->Header.ValidDataLength.QuadPart;
            RxGetFileSizeWithLock(Fcb, &FileSize);
            ASSERT(ValidDataLength <= FileSize);

            /* Check we can switch back to async? */
            if ((SwitchBackToAsync && Fcb->NonPaged->SectionObjectPointers.DataSectionObject != NULL) ||
                (ByteOffset.QuadPart + WriteLength > FileSize) || RxNoAsync)
            {
                SwitchBackToAsync = FALSE;
            }

            /* If paging IO, check we don't try to extend the file */
            if (PagingIo)
            {
                if (ByteOffset.QuadPart >= FileSize)
                {
                    _SEH2_TRY_RETURN(Status = STATUS_SUCCESS);
                }

                if (WriteLength > FileSize - ByteOffset.QuadPart)
                {
                    WriteLength = FileSize - ByteOffset.QuadPart;
                }
            }
        }

        /* Save our initial sizes for potential rollback */
        InitialFileSize = FileSize;
        InitialValidDataLength = ValidDataLength;
        /* If writing to EOF, update byte offset with file size */
        if (WriteToEof)
        {
            ByteOffset.QuadPart = FileSize;
        }

        /* Check again whether we're allowed to write */
        if (!PagingIo)
        {
            if (!FsRtlCheckLockForWriteAccess(&Fcb->Specific.Fcb.FileLock, Irp ))
            {
                _SEH2_TRY_RETURN(Status = STATUS_FILE_LOCK_CONFLICT);
            }

            /* Do we have to extend? */
            if (NormalFile && (ByteOffset.QuadPart + WriteLength > FileSize))
            {
                DPRINT("Need to extend file\n");
                ExtendingFile = TRUE;
                SetFlag(LowIoContext->ParamsFor.ReadWrite.Flags, LOWIO_READWRITEFLAG_EXTENDING_FILESIZE);
            }
        }

        /* Let's start to extend */
        if (ExtendingFile)
        {
            /* If we're past allocating, inform mini-rdr */
            FileSize = ByteOffset.QuadPart + WriteLength;
            if (FileSize > Fcb->Header.AllocationSize.QuadPart)
            {
                LARGE_INTEGER NewAllocationSize;

                DPRINT("Extending %p\n", RxContext);

                if (NoCache)
                {
                    C_ASSERT(sizeof(LONGLONG) == sizeof(LARGE_INTEGER));
                    MINIRDR_CALL(Status, RxContext, Fcb->MRxDispatch, MRxExtendForNonCache,
                                 (RxContext, (PLARGE_INTEGER)&FileSize, &NewAllocationSize));
                }
                else
                {
                    C_ASSERT(sizeof(LONGLONG) == sizeof(LARGE_INTEGER));
                    MINIRDR_CALL(Status, RxContext, Fcb->MRxDispatch, MRxExtendForCache,
                                 (RxContext, (PLARGE_INTEGER)&FileSize, &NewAllocationSize));
                }

                if (!NT_SUCCESS(Status))
                {
                    _SEH2_TRY_RETURN(Status);
                }

                if (FileSize > NewAllocationSize.QuadPart)
                {
                    NewAllocationSize.QuadPart = FileSize;
                }

                /* And update FCB */
                Fcb->Header.AllocationSize.QuadPart = NewAllocationSize.QuadPart;
            }

            /* Set the new sizes */
            RxSetFileSizeWithLock(Fcb, &FileSize);
            RxAdjustAllocationSizeforCC(Fcb);

            /* And inform Cc */
            if (CcIsFileCached(FileObject))
            {
                CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&Fcb->Header.AllocationSize);
            }
        }

        /* Do we have to extend VDL? */
        if (!CalledByLazyWriter && !RecursiveWriteThrough)
        {
            if (WriteToEof || ByteOffset.QuadPart + WriteLength > ValidDataLength)
            {
                ExtendingValidData = TRUE;
                SetFlag(LowIoContext->ParamsFor.ReadWrite.Flags, LOWIO_READWRITEFLAG_EXTENDING_VDL);
            }
        }

        /* If none cached write */
        if (PagingIo || NoCache || !RxWriteCacheingAllowed(Fcb, SrvOpen))
        {
            /* Switch back to async, if asked to */
            if (SwitchBackToAsync)
            {
                CanWait = FALSE;
                Sync = FALSE;

                ClearFlag(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
                SetFlag(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION);
            }

            /* If not synchronous, keep track of writes to be finished */
            if (!Sync)
            {
                if (Fcb->NonPaged->OutstandingAsyncEvent == NULL)
                {
                    Fcb->NonPaged->OutstandingAsyncEvent = &Fcb->NonPaged->TheActualEvent;
                    KeInitializeEvent(Fcb->NonPaged->OutstandingAsyncEvent,
                                      NotificationEvent, FALSE);
                }

                if (ExInterlockedAddUlong(&Fcb->NonPaged->OutstandingAsyncWrites,
                                          1,
                                          &RxStrucSupSpinLock) == 0)
                {
                    KeClearEvent(Fcb->NonPaged->OutstandingAsyncEvent);
                }

                UnwindOutstandingAsync = TRUE;
                LowIoContext->ParamsFor.ReadWrite.NonPagedFcb = Fcb->NonPaged;
            }

            /* Set our LOWIO_CONTEXT information */
            LowIoContext->ParamsFor.ReadWrite.ByteOffset = ByteOffset.QuadPart;
            LowIoContext->ParamsFor.ReadWrite.ByteCount = WriteLength;

            RxItsTheSameContext();

            /* We have to be locked */
            ASSERT(RxContext->FcbResourceAcquired || RxContext->FcbPagingIoResourceAcquired);

            /* Update thread ID if we're in FSP */
            if (InFsp)
            {
                LowIoContext->ResourceThreadId = (ULONG_PTR)RxContext | 3;

                if (RxContext->FcbResourceAcquired)
                {
                    ExSetResourceOwnerPointer(Fcb->Header.Resource, (PVOID)((ULONG_PTR)RxContext | 3));
                }

                if (RxContext->FcbPagingIoResourceAcquired)
                {
                    ExSetResourceOwnerPointer(Fcb->Header.PagingIoResource, (PVOID)((ULONG_PTR)RxContext | 3));
                }

                ResourceOwnerSet = TRUE;
            }

            /* And perform the write */
            Status = RxLowIoWriteShell(RxContext);

            RxItsTheSameContext();

            /* Not outstanding write anymore */
            if (UnwindOutstandingAsync && Status == STATUS_PENDING)
            {
                UnwindOutstandingAsync = FALSE;
            }
        }
        /* Cached write */
        else
        {
            /* If cache wasn't enabled yet, do it */
            if (FileObject->PrivateCacheMap == NULL)
            {
                if (BooleanFlagOn(FileObject->Flags, FO_CLEANUP_COMPLETE))
                {
                    _SEH2_TRY_RETURN(Status = STATUS_FILE_CLOSED);
                }

                RxAdjustAllocationSizeforCC(Fcb);

                CcInitializeCacheMap(FileObject, (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                                     FALSE, &RxData.CacheManagerCallbacks, Fcb);

                CcSetReadAheadGranularity(FileObject, NetRoot->DiskParameters.ReadAheadGranularity);
            }

            /* If that's a MDL backed write */
            if (BooleanFlagOn(RxContext->MinorFunction, IRP_MN_MDL))
            {
                /* Shouldn't happen */
                ASSERT(FALSE);
                ASSERT(CanWait);

                /* Perform it, though */
                CcPrepareMdlWrite(FileObject, &ByteOffset, WriteLength,
                                  &Irp->MdlAddress, &Irp->IoStatus);

                Status = Irp->IoStatus.Status;
            }
            else
            {
                PVOID SystemBuffer;
                ULONG BreakpointsSave;

                /* Map the user buffer */
                SystemBuffer = RxNewMapUserBuffer(RxContext);
                if (SystemBuffer == NULL)
                {
                    _SEH2_TRY_RETURN(Status = STATUS_INSUFFICIENT_RESOURCES);
                }

                RxSaveAndSetExceptionNoBreakpointFlag(RxContext, BreakpointsSave);

                RxItsTheSameContext();

                /* And deal with Cc */
                if (!CcCopyWrite(FileObject, &ByteOffset, WriteLength, CanWait,
                                 SystemBuffer))
                {
                    RxRestoreExceptionNoBreakpointFlag(RxContext, BreakpointsSave);

                    RxItsTheSameContext();

                    DPRINT1("CcCopyWrite failed for: %p %I64d %d %lx\n",
                            FileObject, Fcb->Header.FileSize.QuadPart, WriteLength, Status);

                    PostIrp = TRUE;
                }
                else
                {
                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    Irp->IoStatus.Information = WriteLength;

                    RxRestoreExceptionNoBreakpointFlag(RxContext, BreakpointsSave);

                    RxItsTheSameContext();

                    DPRINT("CcCopyWrite succeed for: %p %I64d %d %lx\n",
                           FileObject, Fcb->Header.FileSize.QuadPart, WriteLength, Status);
                }
            }
        }

try_exit: NOTHING;

        /* If we've to post the IRP */
        if (PostIrp)
        {
            /* Reset the file size if required */
            if (ExtendingFile && !IsPipe)
            {
                ASSERT(RxWriteCacheingAllowed(Fcb, SrvOpen));
                ASSERT(Fcb->Header.PagingIoResource != NULL);

                RxAcquirePagingIoResource(RxContext, Fcb);
                RxSetFileSizeWithLock(Fcb, &InitialFileSize);
                RxReleasePagingIoResource(RxContext, Fcb);

                if (FileObject->SectionObjectPointer->SharedCacheMap != NULL)
                {
                    *CcGetFileSizePointer(FileObject) = Fcb->Header.FileSize;
                }
            }

            InterlockedIncrement((volatile long *)&RxContext->ReferenceCount);
            ContextReferenced = TRUE;

            /* Release locks */
            ASSERT(!ResourceOwnerSet);
            RxWriteReleaseResources(RxContext, ResourceOwnerSet);

#ifdef RDBSS_TRACKER
            ASSERT(RxContext->AcquireReleaseFcbTrackerX == 0);
#endif

            /* And post the request */
            Status = RxFsdPostRequest(RxContext);
        }
        else
        {
            if (!IsPipe)
            {
                /* Update FILE_OBJECT if synchronous write succeed */
                if (!PagingIo)
                {
                    if (NT_SUCCESS(Status) && BooleanFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO))
                    {
                        FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + Irp->IoStatus.Information;
                    }
                }

                /* If write succeed, ,also update FILE_OBJECT flags */
                if (NT_SUCCESS(Status) && Status != STATUS_PENDING)
                {
                    /* File was modified */
                    if (!PagingIo)
                    {
                        SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
                    }

                    /* If was even extended */
                    if (ExtendingFile)
                    {
                        SetFlag(FileObject->Flags, FO_FILE_SIZE_CHANGED);
                    }

                    /* If VDL was extended, update FCB and inform Cc */
                    if (ExtendingValidData)
                    {
                        LONGLONG LastOffset;

                        LastOffset = ByteOffset.QuadPart + Irp->IoStatus.Information;
                        if (FileSize < LastOffset)
                        {
                            LastOffset = FileSize;
                        }

                        Fcb->Header.ValidDataLength.QuadPart = LastOffset;

                        if (NoCache && CcIsFileCached(FileObject))
                        {
                            CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&Fcb->Header.AllocationSize);
                        }
                    }
                }
            }
        }
    }
    _SEH2_FINALLY
    {
        /* Finally, if we failed while extension was required */
        if (_SEH2_AbnormalTermination() && (ExtendingFile || ExtendingValidData))
        {
            /* Rollback! */
            if (!IsPipe)
            {
                ASSERT(Fcb->Header.PagingIoResource != NULL);

                RxAcquirePagingIoResource(RxContext, Fcb);
                RxSetFileSizeWithLock(Fcb, &InitialFileSize);
                Fcb->Header.ValidDataLength.QuadPart = InitialValidDataLength;
                RxReleasePagingIoResource(RxContext, Fcb);

                if (FileObject->SectionObjectPointer->SharedCacheMap != NULL)
                {
                    *CcGetFileSizePointer(FileObject) = Fcb->Header.FileSize;
                }
            }
        }

        /* One async write less */
        if (UnwindOutstandingAsync)
        {
            ASSERT(!IsPipe);

            ExInterlockedAddUlong(&Fcb->NonPaged->OutstandingAsyncWrites, -1, &RxStrucSupSpinLock);
            KeSetEvent(Fcb->NonPaged->OutstandingAsyncEvent, IO_NO_INCREMENT, FALSE);
        }

        /* And now, cleanup everything */
        if (_SEH2_AbnormalTermination() || Status != STATUS_PENDING || PostIrp)
        {
            /* If we didn't post, release every lock (for posting, it's already done) */
            if (!PostIrp)
            {
                RxWriteReleaseResources(RxContext, ResourceOwnerSet);
            }

            /* If the context was referenced - posting, dereference it */
            if (ContextReferenced)
            {
                RxDereferenceAndDeleteRxContext(RxContext);
            }

            /* If that's a pipe operation, resume any blocked one */
            if (!PostIrp)
            {
                if (BooleanFlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION))
                {
                    RxResumeBlockedOperations_Serially(RxContext, &Fobx->Specific.NamedPipe.ReadSerializationQueue);
                }
            }

            /* Sanity check for write */
            if (Status == STATUS_SUCCESS)
            {
                ASSERT(Irp->IoStatus.Information <= Stack->Parameters.Write.Length);
            }
        }
        /* Just dereference our context */
        else
        {
            ASSERT(!Sync);
            RxDereferenceAndDeleteRxContext(RxContext);
        }
    }
    _SEH2_END;

#undef _SEH2_TRY_RETURN

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxCompleteMdl(
    IN PRX_CONTEXT RxContext)
{
    PIRP Irp;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;

#define BugCheckFileId RDBSS_BUG_CHECK_CACHESUP

    PAGED_CODE();

    Irp = RxContext->CurrentIrp;
    Stack = RxContext->CurrentIrpSp;
    FileObject = Stack->FileObject;

    /* We can only complete for IRP_MJ_READ and IRP_MJ_WRITE */
    switch (RxContext->MajorFunction)
    {
        /* Call the Cc function */
        case IRP_MJ_READ:
            CcMdlReadComplete(FileObject, Irp->MdlAddress);
            break;

        case IRP_MJ_WRITE:
            /* If here, we can wait */
            ASSERT(BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT));

            /* Call the Cc function */
            CcMdlWriteComplete(FileObject, &Stack->Parameters.Write.ByteOffset, Irp->MdlAddress);

            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        default:
            DPRINT1("Invalid major for RxCompleteMdl: %d\n", RxContext->MajorFunction);
            RxBugCheck(RxContext->MajorFunction, 0, 0);
            break;
    }

    /* MDL was freed */
    Irp->MdlAddress = NULL;

    /* And complete the IRP */
    RxCompleteRequest(RxContext, STATUS_SUCCESS);

#undef BugCheckFileId

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
RxConjureOriginalName(
    PFCB Fcb,
    PFOBX Fobx,
    PULONG ActualNameLength,
    PWCHAR OriginalName,
    PLONG LengthRemaining,
    RX_NAME_CONJURING_METHODS NameConjuringMethod)
{
    PWSTR Prefix, Name;
    PV_NET_ROOT VNetRoot;
    USHORT PrefixLength, NameLength, ToCopy;

    PAGED_CODE();

    VNetRoot = Fcb->VNetRoot;
    /* We will use the prefix contained in NET_ROOT, if we don't have
     * a V_NET_ROOT, or if it wasn't null deviced or if we already have
     * a UNC path */
    if (VNetRoot == NULL || VNetRoot->PrefixEntry.Prefix.Buffer[1] != L';' ||
        BooleanFlagOn(Fobx->Flags, FOBX_FLAG_UNC_NAME))
    {
        Prefix = ((PNET_ROOT)Fcb->pNetRoot)->PrefixEntry.Prefix.Buffer;
        PrefixLength = ((PNET_ROOT)Fcb->pNetRoot)->PrefixEntry.Prefix.Length;
        NameLength = 0;

        /* In that case, keep track that we will have a prefix as buffer */
        NameConjuringMethod = VNetRoot_As_Prefix;
    }
    else
    {
        ASSERT(NodeType(VNetRoot) == RDBSS_NTC_V_NETROOT);

        /* Otherwise, return the prefix from our V_NET_ROOT */
        Prefix = VNetRoot->PrefixEntry.Prefix.Buffer;
        PrefixLength = VNetRoot->PrefixEntry.Prefix.Length;
        NameLength = VNetRoot->NamePrefix.Length;

        /* If we want a UNC path, skip potential device */
        if (NameConjuringMethod == VNetRoot_As_UNC_Name)
        {
            do
            {
                ++Prefix;
                PrefixLength -= sizeof(WCHAR);
            } while (PrefixLength > 0 && Prefix[0] != L'\\');
        }
    }

    /* If we added an extra backslash, skip it */
    if (BooleanFlagOn(Fcb->FcbState, FCB_STATE_ADDEDBACKSLASH))
    {
        NameLength += sizeof(WCHAR);
    }

    /* If we're asked for a drive letter, skip the prefix */
    if (NameConjuringMethod == VNetRoot_As_DriveLetter)
    {
        PrefixLength = 0;

        /* And make sure we arrive at a backslash */
        if (Fcb->FcbTableEntry.Path.Length > NameLength &&
            Fcb->FcbTableEntry.Path.Buffer[NameLength / sizeof(WCHAR)] != L'\\')
        {
            NameLength -= sizeof(WCHAR);
        }
    }
    else
    {
        /* Prepare to copy the prefix, make sure not to overflow */
        if (*LengthRemaining >= PrefixLength)
        {
            /* Copy everything */
            ToCopy = PrefixLength;
            *LengthRemaining = *LengthRemaining - PrefixLength;
        }
        else
        {
            /* Copy as much as we can */
            ToCopy = *LengthRemaining;
            /* And return failure */
            *LengthRemaining = -1;
        }

        /* Copy the prefix */
        RtlCopyMemory(OriginalName, Prefix, ToCopy);
    }

    /* Do we have a name to copy now? */
    if (Fcb->FcbTableEntry.Path.Length > NameLength)
    {
        ToCopy = Fcb->FcbTableEntry.Path.Length - NameLength;
        Name = Fcb->FcbTableEntry.Path.Buffer;
    }
    else
    {
        /* Just use slash for now */
        ToCopy = sizeof(WCHAR);
        NameLength = 0;
        Name = L"\\";
    }

    /* Total length we will have in the output buffer (if everything is alright) */
    *ActualNameLength = ToCopy + PrefixLength;
    /* If we still have room to write data */
    if (*LengthRemaining != -1)
    {
        /* If we can copy everything, it's fine! */
        if (*LengthRemaining > ToCopy)
        {
            *LengthRemaining = *LengthRemaining - ToCopy;
        }
        /* Otherwise, copy as much as possible, and return failure */
        else
        {
            ToCopy = *LengthRemaining;
            *LengthRemaining = -1;
        }

        /* Copy name after the prefix */
        RtlCopyMemory(Add2Ptr(OriginalName, PrefixLength),
                      Add2Ptr(Name, NameLength), ToCopy);
    }
}

/*
 * @implemented
 */
VOID
RxCopyCreateParameters(
    IN PRX_CONTEXT RxContext)
{
    PIRP Irp;
    PVOID DfsContext;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;
    PDFS_NAME_CONTEXT DfsNameContext;
    PIO_SECURITY_CONTEXT SecurityContext;

    Irp = RxContext->CurrentIrp;
    Stack = RxContext->CurrentIrpSp;
    FileObject = Stack->FileObject;
    SecurityContext = Stack->Parameters.Create.SecurityContext;

    RxContext->Create.NtCreateParameters.SecurityContext = SecurityContext;
    if (SecurityContext->AccessState != NULL && SecurityContext->AccessState->SecurityDescriptor != NULL)
    {
        RxContext->Create.SdLength = RtlLengthSecurityDescriptor(SecurityContext->AccessState->SecurityDescriptor);
        DPRINT("SD Ctxt: %p, Length: %lx\n", RxContext->Create.NtCreateParameters.SecurityContext,
               RxContext->Create.SdLength);
    }
    if (SecurityContext->SecurityQos != NULL)
    {
        RxContext->Create.NtCreateParameters.ImpersonationLevel = SecurityContext->SecurityQos->ImpersonationLevel;
    }
    else
    {
        RxContext->Create.NtCreateParameters.ImpersonationLevel = SecurityImpersonation;
    }
    RxContext->Create.NtCreateParameters.DesiredAccess = SecurityContext->DesiredAccess;

    RxContext->Create.NtCreateParameters.AllocationSize.QuadPart = Irp->Overlay.AllocationSize.QuadPart;
    RxContext->Create.NtCreateParameters.FileAttributes = Stack->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_VALID_FLAGS;
    RxContext->Create.NtCreateParameters.ShareAccess = Stack->Parameters.Create.ShareAccess & FILE_SHARE_VALID_FLAGS;
    RxContext->Create.NtCreateParameters.Disposition = (Stack->Parameters.Create.Options >> 24) & 0x000000FF;
    RxContext->Create.NtCreateParameters.CreateOptions = Stack->Parameters.Create.Options & 0xFFFFFF;

    DfsContext = FileObject->FsContext2;
    DfsNameContext = FileObject->FsContext;
    RxContext->Create.NtCreateParameters.DfsContext = DfsContext;
    RxContext->Create.NtCreateParameters.DfsNameContext = DfsNameContext;
    ASSERT(DfsContext == NULL || DfsContext == UIntToPtr(DFS_OPEN_CONTEXT) ||
           DfsContext == UIntToPtr(DFS_DOWNLEVEL_OPEN_CONTEXT) ||
           DfsContext == UIntToPtr(DFS_CSCAGENT_NAME_CONTEXT) ||
           DfsContext == UIntToPtr(DFS_USER_NAME_CONTEXT));
    ASSERT(DfsNameContext == NULL || DfsNameContext->NameContextType == DFS_OPEN_CONTEXT ||
           DfsNameContext->NameContextType == DFS_DOWNLEVEL_OPEN_CONTEXT ||
           DfsNameContext->NameContextType == DFS_CSCAGENT_NAME_CONTEXT ||
           DfsNameContext->NameContextType == DFS_USER_NAME_CONTEXT);
    FileObject->FsContext2 = NULL;
    FileObject->FsContext = NULL;

    RxContext->pFcb = NULL;
    RxContext->Create.ReturnedCreateInformation = 0;

    /* if we stripped last \, it has to be a directory! */
    if (BooleanFlagOn(RxContext->Create.Flags, RX_CONTEXT_CREATE_FLAG_STRIPPED_TRAILING_BACKSLASH))
    {
        SetFlag(RxContext->Create.NtCreateParameters.CreateOptions, FILE_DIRECTORY_FILE);
    }

    RxContext->Create.EaLength = Stack->Parameters.Create.EaLength;
    if (RxContext->Create.EaLength == 0)
    {
        RxContext->Create.EaBuffer = NULL;
    }
    else
    {
        RxContext->Create.EaBuffer = Irp->AssociatedIrp.SystemBuffer;
        DPRINT("EA Buffer: %p, Length: %lx\n", Irp->AssociatedIrp.SystemBuffer, RxContext->Create.EaLength);
    }
}

NTSTATUS
RxCreateFromNetRoot(
    PRX_CONTEXT Context,
    PUNICODE_STRING NetRootName)
{
    PFCB Fcb;
    NTSTATUS Status;
    PNET_ROOT NetRoot;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;
    ACCESS_MASK DesiredAccess;
    USHORT DesiredShareAccess;

    PAGED_CODE();

    /* Validate that the context is consistent */
    if (Context->Create.pNetRoot == NULL)
    {
        return STATUS_BAD_NETWORK_PATH;
    }

    NetRoot = (PNET_ROOT)Context->Create.pNetRoot;
    if (Context->RxDeviceObject != NetRoot->pSrvCall->RxDeviceObject)
    {
        return STATUS_BAD_NETWORK_PATH;
    }

    if (Context->Create.NtCreateParameters.DfsContext == UIntToPtr(DFS_OPEN_CONTEXT) &&
        !BooleanFlagOn(NetRoot->pSrvCall->Flags, SRVCALL_FLAG_DFS_AWARE_SERVER))
    {
        return STATUS_DFS_UNAVAILABLE;
    }

    if (Context->Create.NtCreateParameters.DfsContext == UIntToPtr(DFS_DOWNLEVEL_OPEN_CONTEXT) &&
        BooleanFlagOn(NetRoot->Flags, NETROOT_FLAG_DFS_AWARE_NETROOT))
    {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    Stack = Context->CurrentIrpSp;
    DesiredShareAccess = Stack->Parameters.Create.ShareAccess & FILE_SHARE_VALID_FLAGS;
    if (NetRoot->Type == NET_ROOT_PRINT)
    {
        DesiredShareAccess = FILE_SHARE_VALID_FLAGS;
    }

    DesiredAccess = Stack->Parameters.Create.SecurityContext->DesiredAccess & FILE_ALL_ACCESS;

    /* Get file object */
    FileObject = Stack->FileObject;

    /* Do we have to open target directory for renaming? */
    if (BooleanFlagOn(Stack->Flags, SL_OPEN_TARGET_DIRECTORY))
    {
        DPRINT("Opening target directory\n");

        /* If we have been asked for delete, try to purge first */
        if (BooleanFlagOn(Context->Create.NtCreateParameters.DesiredAccess, DELETE))
        {
            RxPurgeRelatedFobxs((PNET_ROOT)Context->Create.pVNetRoot->pNetRoot, Context,
                                ATTEMPT_FINALIZE_ON_PURGE, NULL);
        }

        /* Create the FCB */
        Fcb = RxCreateNetFcb(Context, (PV_NET_ROOT)Context->Create.pVNetRoot, NetRootName);
        if (Fcb == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Fake it: it will be used only renaming */
        NodeType(Fcb) = RDBSS_NTC_OPENTARGETDIR_FCB;
        Context->Create.FcbAcquired = FALSE;
        Context->Create.NetNamePrefixEntry = NULL;

        /* Assign it to the FO */
        FileObject->FsContext = Fcb;

        /* If we have a FOBX already, check whether it's for DFS opening */
        if (Context->pFobx != NULL)
        {
            /* If so, reflect this in the FOBX */
            if (FileObject->FsContext2 == UIntToPtr(DFS_OPEN_CONTEXT))
            {
                SetFlag(Context->pFobx->Flags, FOBX_FLAG_DFS_OPEN);
            }
            else
            {
                ClearFlag(Context->pFobx->Flags, FOBX_FLAG_DFS_OPEN);
            }
        }

        /* Acquire the FCB */
        Status = RxAcquireExclusiveFcb(Context, Fcb);
        if (Status != STATUS_SUCCESS)
        {
            return Status;
        }

        /* Reference the FCB and release */
        RxReferenceNetFcb(Fcb);
        RxReleaseFcb(Context, Fcb);

        /* We're done! */
        return STATUS_SUCCESS;
    }

    /* Try to find (or create) the FCB for the file */
    Status = RxFindOrCreateFcb(Context, NetRootName);
    Fcb = (PFCB)Context->pFcb;
    if (Fcb == NULL)
    {
        ASSERT(!NT_SUCCESS(Status));
    }
    if (!NT_SUCCESS(Status) || Fcb == NULL)
    {
        return Status;
    }

    if (BooleanFlagOn(Context->Flags, RX_CONTEXT_FLAG_CREATE_MAILSLOT))
    {
        Fcb->Header.NodeTypeCode = RDBSS_NTC_MAILSLOT;
    }
    else
    {
        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* If finding FCB worked (mailslot case), mark the FCB as good and quit */
    if (NT_SUCCESS(Status))
    {
        RxTransitionNetFcb(Fcb, Condition_Good);
        DPRINT("Transitioning FCB %lx Condition %lx\n", Fcb, Fcb->Condition);
        ++Fcb->OpenCount;
        RxSetupNetFileObject(Context);
        return STATUS_SUCCESS;
    }

    /* Not mailslot! */
    /* Check SA for conflict */
    if (Fcb->OpenCount > 0)
    {
        Status = RxCheckShareAccess(DesiredAccess, DesiredShareAccess, FileObject,
                                    &Fcb->ShareAccess, FALSE, "early check per useropens", "EarlyPerUO");
        if (!NT_SUCCESS(Status))
        {
            RxDereferenceNetFcb(Fcb);
            return Status;
        }
    }

    if (BooleanFlagOn(Context->Create.NtCreateParameters.CreateOptions, FILE_DELETE_ON_CLOSE) &&
        !BooleanFlagOn(Context->Create.NtCreateParameters.DesiredAccess, ~SYNCHRONIZE))
    {
        UNIMPLEMENTED;
    }

    _SEH2_TRY
    {
        /* Find a SRV_OPEN that suits the opening */
        Status = RxCollapseOrCreateSrvOpen(Context);
        if (Status == STATUS_SUCCESS)
        {
            PFOBX Fobx;
            PSRV_OPEN SrvOpen;

            SrvOpen = (PSRV_OPEN)Context->pRelevantSrvOpen;
            Fobx = (PFOBX)Context->pFobx;
            /* There are already opens, check for conflict */
            if (Fcb->OpenCount != 0)
            {
                if (!NT_SUCCESS(RxCheckShareAccess(DesiredAccess, DesiredShareAccess,
                                                   FileObject, &Fcb->ShareAccess,
                                                   FALSE, "second check per useropens",
                                                   "2ndAccPerUO")))
                {
                    ++SrvOpen->UncleanFobxCount;
                    RxDereferenceNetFobx(Fobx, LHS_LockNotHeld);

                    _SEH2_LEAVE;
                }
            }
            else
            {
                if (NetRoot->Type != NET_ROOT_PIPE)
                {
                    RxSetShareAccess(DesiredAccess, DesiredShareAccess, FileObject,
                                     &Fcb->ShareAccess, "initial shareaccess setup", "InitShrAcc");
                }
            }

            RxSetupNetFileObject(Context);

            /* No conflict? Set up SA */
            if (Fcb->OpenCount != 0 && NetRoot->Type != NET_ROOT_PIPE)
            {
                RxUpdateShareAccess(FileObject, &Fcb->ShareAccess, "update share access", "UpdShrAcc");
            }

            ++Fcb->UncleanCount;
            if (BooleanFlagOn(FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING))
            {
                ++Fcb->UncachedUncleanCount;
            }

            if (SrvOpen->UncleanFobxCount == 0 && Fcb->UncleanCount == 1 &&
                !BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_NO_BUFFERING_STATE_CHANGE))
            {
                RxChangeBufferingState(SrvOpen, NULL, FALSE);
            }

            /* No pending close, we're active */
            ClearFlag(Fcb->FcbState, FCB_STATE_DELAY_CLOSE);

            ++Fcb->OpenCount;
            ++SrvOpen->UncleanFobxCount;
            ++SrvOpen->OpenCount;
            SrvOpen->ulFileSizeVersion = Fcb->ulFileSizeVersion;

            if (BooleanFlagOn(Stack->Parameters.Create.Options, FILE_NO_INTERMEDIATE_BUFFERING))
            {
                SetFlag(SrvOpen->Flags, SRVOPEN_FLAG_DONTUSE_READ_CACHING);
                SetFlag(SrvOpen->Flags, SRVOPEN_FLAG_DONTUSE_WRITE_CACHING);

                ClearFlag(Fcb->FcbState, FCB_STATE_WRITECACHING_ENABLED);
                ClearFlag(Fcb->FcbState, FCB_STATE_READCACHING_ENABLED);

                RxPurgeFcbInSystemCache(Fcb, NULL, 0, TRUE, TRUE);
            }

            /* Now, update SA for the SRV_OPEN */
            RxUpdateShareAccessPerSrvOpens(SrvOpen);

            if (BooleanFlagOn(Stack->Parameters.Create.Options, FILE_DELETE_ON_CLOSE))
            {
                SetFlag(Fobx->Flags, FOBX_FLAG_DELETE_ON_CLOSE);
            }

            /* Update the FOBX info */
            if (Fobx != NULL)
            {
                if (Context->Create.pNetRoot->Type == NET_ROOT_PIPE)
                {
                    SetFlag(FileObject->Flags, FO_NAMED_PIPE);
                }

                if (Context->Create.pNetRoot->Type == NET_ROOT_PRINT ||
                    Context->Create.pNetRoot->Type == NET_ROOT_PIPE)
                {
                    Fobx->PipeHandleInformation = &Fobx->Specific.NamedPipe.PipeHandleInformation;

                    Fobx->Specific.NamedPipe.CollectDataTime.QuadPart = 0;
                    Fobx->Specific.NamedPipe.CollectDataSize = Context->Create.pNetRoot->NamedPipeParameters.DataCollectionSize;

                    Fobx->Specific.NamedPipe.PipeHandleInformation.TypeOfPipe = Context->Create.PipeType;
                    Fobx->Specific.NamedPipe.PipeHandleInformation.ReadMode = Context->Create.PipeReadMode;
                    Fobx->Specific.NamedPipe.PipeHandleInformation.CompletionMode = Context->Create.PipeCompletionMode;

                    InitializeListHead(&Fobx->Specific.NamedPipe.ReadSerializationQueue);
                    InitializeListHead(&Fobx->Specific.NamedPipe.WriteSerializationQueue);
                }
            }

            Status = STATUS_SUCCESS;
        }
    }
    _SEH2_FINALLY
    {
        if (Fcb->OpenCount == 0)
        {
            if (Context->Create.FcbAcquired)
            {
                Context->Create.FcbAcquired = (RxDereferenceAndFinalizeNetFcb(Fcb,
                                                                              Context,
                                                                              FALSE,
                                                                              FALSE) == 0);
                if (!Context->Create.FcbAcquired)
                {
                    RxTrackerUpdateHistory(Context, NULL, TRACKER_FCB_FREE, __LINE__, __FILE__, 0);
                }
            }
        }
        else
        {
            RxDereferenceNetFcb(Fcb);
        }
    }
    _SEH2_END;

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
RxCreateTreeConnect(
    IN PRX_CONTEXT RxContext)
{
    NTSTATUS Status;
    PV_NET_ROOT VNetRoot;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;
    NET_ROOT_TYPE NetRootType;
    UNICODE_STRING CanonicalName, RemainingName;

    PAGED_CODE();

    Stack = RxContext->CurrentIrpSp;
    FileObject = Stack->FileObject;

    RtlInitEmptyUnicodeString(&CanonicalName, NULL, 0);
    /* As long as we don't know connection type, mark it wild */
    NetRootType = NET_ROOT_WILD;
    /* Get the type by parsing the name */
    Status = RxFirstCanonicalize(RxContext, &FileObject->FileName, &CanonicalName, &NetRootType);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    RxContext->Create.ThisIsATreeConnectOpen = TRUE;
    RxContext->Create.TreeConnectOpenDeferred = FALSE;
    RtlInitEmptyUnicodeString(&RxContext->Create.TransportName, NULL, 0);
    RtlInitEmptyUnicodeString(&RxContext->Create.UserName, NULL, 0);
    RtlInitEmptyUnicodeString(&RxContext->Create.Password, NULL, 0);
    RtlInitEmptyUnicodeString(&RxContext->Create.UserDomainName, NULL, 0);

    /* We don't handle EA - they come from DFS, don't care */
    if (Stack->Parameters.Create.EaLength > 0)
    {
        UNIMPLEMENTED;
    }

    /* Mount if required */
    Status = RxFindOrConstructVirtualNetRoot(RxContext, &CanonicalName, NetRootType, &RemainingName);
    if (Status == STATUS_NETWORK_CREDENTIAL_CONFLICT)
    {
        RxScavengeVNetRoots(RxContext->RxDeviceObject);
        Status = RxFindOrConstructVirtualNetRoot(RxContext, &CanonicalName, NetRootType, &RemainingName);
    }

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Validate the rest of the name with mini-rdr */
    if (RemainingName.Length > 0)
    {
        MINIRDR_CALL(Status, RxContext,
                     RxContext->Create.pNetRoot->pSrvCall->RxDeviceObject->Dispatch,
                     MRxIsValidDirectory, (RxContext, &RemainingName));
    }

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    VNetRoot = (PV_NET_ROOT)RxContext->Create.pVNetRoot;
    RxReferenceVNetRoot(VNetRoot);
    if (InterlockedCompareExchange(&VNetRoot->AdditionalReferenceForDeleteFsctlTaken, 1, 0) != 0)
    {
        RxDereferenceVNetRoot(VNetRoot, LHS_LockNotHeld);
    }

    FileObject->FsContext = &RxDeviceFCB;
    FileObject->FsContext2 = VNetRoot;

    VNetRoot->ConstructionStatus = STATUS_SUCCESS;
    ++VNetRoot->NumberOfOpens;

    /* Create is over - clear context */
    RxContext->Create.pSrvCall = NULL;
    RxContext->Create.pNetRoot = NULL;
    RxContext->Create.pVNetRoot = NULL;

    return Status;
}

VOID
NTAPI
RxDebugControlCommand(
    _In_ PSTR ControlString)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
RxDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    USHORT i, State = 0;

    DPRINT("RxDriverEntry(%p, %p)\n", DriverObject, RegistryPath);

    _SEH2_TRY
    {
        RxCheckFcbStructuresForAlignment();

        RtlZeroMemory(&RxData, sizeof(RxData));
        RxData.NodeTypeCode = RDBSS_NTC_DATA_HEADER;
        RxData.NodeByteSize = sizeof(RxData);
        RxData.DriverObject = DriverObject;

        RtlZeroMemory(&RxDeviceFCB, sizeof(RxDeviceFCB));
        RxDeviceFCB.spacer.NodeTypeCode = RDBSS_NTC_DEVICE_FCB;
        RxDeviceFCB.spacer.NodeByteSize = sizeof(RxDeviceFCB);

        KeInitializeSpinLock(&RxStrucSupSpinLock);
        RxExports.pRxStrucSupSpinLock = &RxStrucSupSpinLock;

        RxInitializeDebugSupport();

        RxFileSystemDeviceObject = (PRDBSS_DEVICE_OBJECT)&RxSpaceForTheWrappersDeviceObject;
        RtlZeroMemory(&RxSpaceForTheWrappersDeviceObject, sizeof(RxSpaceForTheWrappersDeviceObject));

        RxInitializeLog();
        State = 2;

        RxGetRegistryParameters(RegistryPath);
        RxReadRegistryParameters();

        Status = RxInitializeRegistrationStructures();
        if (!NT_SUCCESS(Status))
        {
            _SEH2_LEAVE;
        }
        State = 1;

        RxInitializeDispatcher();

        ExInitializeNPagedLookasideList(&RxContextLookasideList, RxAllocatePoolWithTag, RxFreePool, 0, sizeof(RX_CONTEXT), RX_IRPC_POOLTAG, 4);

        InitializeListHead(&RxIrpsList);
        KeInitializeSpinLock(&RxIrpsListSpinLock);

        InitializeListHead(&RxActiveContexts);
        InitializeListHead(&RxSrvCalldownList);

        ExInitializeFastMutex(&RxContextPerFileSerializationMutex);
        ExInitializeFastMutex(&RxLowIoPagingIoSyncMutex);
        KeInitializeMutex(&RxScavengerMutex, 1);
        KeInitializeMutex(&RxSerializationMutex, 1);

        for (i = 0; i < RxMaximumWorkQueue; ++i)
        {
            RxFileSystemDeviceObject->PostedRequestCount[i] = 0;
            RxFileSystemDeviceObject->OverflowQueueCount[i] = 0;
            InitializeListHead(&RxFileSystemDeviceObject->OverflowQueue[i]);
        }

        KeInitializeSpinLock(&RxFileSystemDeviceObject->OverflowQueueSpinLock);

        RxInitializeDispatchVectors(DriverObject);

        ExInitializeResourceLite(&RxData.Resource);
        RxData.OurProcess = IoGetCurrentProcess();

        RxInitializeRxTimer();
    }
    _SEH2_FINALLY
    {
        if (!NT_SUCCESS(Status))
        {
            RxLogFailure(RxFileSystemDeviceObject, NULL, 0x80000BC4, Status);
            RxInitUnwind(DriverObject, State);
        }
    } _SEH2_END;

    /* There are still bits to init - be consider it's fine for now */
#if 0
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
#else
    return STATUS_SUCCESS;
#endif
}

#if DBG
/*
 * @implemented
 */
VOID
RxDumpCurrentAccess(
    _In_ PSZ where1,
    _In_ PSZ where2,
    _In_ PSZ wherelogtag,
    _In_ PSHARE_ACCESS ShareAccess)
{
    PAGED_CODE();
}

/*
 * @implemented
 */
VOID
RxDumpWantedAccess(
    _In_ PSZ where1,
    _In_ PSZ where2,
    _In_ PSZ wherelogtag,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG DesiredShareAccess)
{
    PAGED_CODE();
}
#endif

/*
 * @implemented
 */
BOOLEAN
NTAPI
RxFastIoCheckIfPossible(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length, BOOLEAN Wait,
    ULONG LockKey, BOOLEAN CheckForReadOperation,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    PFCB Fcb;
    PSRV_OPEN SrvOpen;
    LARGE_INTEGER LargeLength;

    PAGED_CODE();

    /* Get the FCB to validate it */
    Fcb = FileObject->FsContext;
    if (NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_FILE)
    {
        DPRINT1("Not a file, FastIO not possible!\n");
        return FALSE;
    }

    if (FileObject->DeletePending)
    {
        DPRINT1("File delete pending\n");
        return FALSE;
    }

    /* If there's a pending write operation, deny fast operation */
    if (Fcb->NonPaged->OutstandingAsyncWrites != 0)
    {
        DPRINT1("Write operations to be completed\n");
        return FALSE;
    }

    /* Deny read on orphaned node */
    SrvOpen = (PSRV_OPEN)((PFOBX)FileObject->FsContext2)->pSrvOpen;
    if (BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_ORPHANED))
    {
        DPRINT1("SRV_OPEN orphaned\n");
        return FALSE;
    }

    if (BooleanFlagOn(Fcb->FcbState, FCB_STATE_ORPHANED))
    {
        DPRINT1("FCB orphaned\n");
        return FALSE;
    }

    /* If there's a buffering state change pending, deny fast operation (it might change
     * cache status)
     */
    if (BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING))
    {
        DPRINT1("Buffering change pending\n");
        return FALSE;
    }

    /* File got renamed/deleted, deny operation */
    if (BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_FILE_DELETED) ||
        BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_FILE_RENAMED))
    {
        DPRINT1("File renamed/deleted\n");
        return FALSE;
    }

    /* Process pending change buffering state operations */
    FsRtlEnterFileSystem();
    RxProcessChangeBufferingStateRequestsForSrvOpen(SrvOpen);
    FsRtlExitFileSystem();

    LargeLength.QuadPart = Length;

    /* If operation to come is a read operation */
    if (CheckForReadOperation)
    {
        /* Check that read cache is enabled */
        if (!BooleanFlagOn(Fcb->FcbState, FCB_STATE_READCACHING_ENABLED))
        {
            DPRINT1("Read caching disabled\n");
            return FALSE;
        }

        /* Check whether there's a lock conflict */
        if (!FsRtlFastCheckLockForRead(&Fcb->Specific.Fcb.FileLock,
                                       FileOffset,
                                       &LargeLength,
                                       LockKey,
                                       FileObject,
                                       PsGetCurrentProcess()))
        {
            DPRINT1("FsRtlFastCheckLockForRead failed\n");
            return FALSE;
        }

        return TRUE;
    }

    /* Check that write cache is enabled */
    if (!BooleanFlagOn(Fcb->FcbState, FCB_STATE_WRITECACHING_ENABLED))
    {
        DPRINT1("Write caching disabled\n");
        return FALSE;
    }

    /* Check whether there's a lock conflict */
    if (!FsRtlFastCheckLockForWrite(&Fcb->Specific.Fcb.FileLock,
                                    FileOffset,
                                    &LargeLength,
                                    LockKey,
                                    FileObject,
                                    PsGetCurrentProcess()))
    {
        DPRINT1("FsRtlFastCheckLockForWrite failed\n");
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
NTAPI
RxFastIoDeviceControl(
    PFILE_OBJECT FileObject,
    BOOLEAN Wait,
    PVOID InputBuffer OPTIONAL,
    ULONG InputBufferLength,
    PVOID OutputBuffer OPTIONAL,
    ULONG OutputBufferLength,
    ULONG IoControlCode,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    /* Only supported IOCTL */
    if (IoControlCode == IOCTL_LMR_ARE_FILE_OBJECTS_ON_SAME_SERVER)
    {
        UNIMPLEMENTED;
        return FALSE;
    }
    else
    {
        return FALSE;
    }
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RxFastIoRead(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    PVOID Buffer,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    BOOLEAN Ret;
    RX_TOPLEVELIRP_CONTEXT TopLevelContext;

    PAGED_CODE();

    DPRINT("RxFastIoRead: %p (%p, %p)\n", FileObject, FileObject->FsContext,
                                          FileObject->FsContext2);
    DPRINT("Reading %ld at %I64x\n", Length, FileOffset->QuadPart);

    /* Prepare a TLI context */
    ASSERT(RxIsThisTheTopLevelIrp(NULL));
    RxInitializeTopLevelIrpContext(&TopLevelContext, (PIRP)FSRTL_FAST_IO_TOP_LEVEL_IRP,
                                   (PRDBSS_DEVICE_OBJECT)DeviceObject);

    Ret = FsRtlCopyRead2(FileObject, FileOffset, Length, Wait, LockKey, Buffer,
                         IoStatus, DeviceObject, &TopLevelContext);
    if (Ret)
    {
        DPRINT("Read OK\n");
    }
    else
    {
        DPRINT1("Read failed!\n");
    }

    return Ret;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RxFastIoWrite(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    PVOID Buffer,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    PFOBX Fobx;
    BOOLEAN Ret;
    RX_TOPLEVELIRP_CONTEXT TopLevelContext;

    PAGED_CODE();

    Fobx = (PFOBX)FileObject->FsContext2;
    if (BooleanFlagOn(Fobx->Flags, FOBX_FLAG_BAD_HANDLE))
    {
        return FALSE;
    }

    DPRINT("RxFastIoWrite: %p (%p, %p)\n", FileObject, FileObject->FsContext,
                                           FileObject->FsContext2);
    DPRINT("Writing %ld at %I64x\n", Length, FileOffset->QuadPart);

    /* Prepare a TLI context */
    ASSERT(RxIsThisTheTopLevelIrp(NULL));
    RxInitializeTopLevelIrpContext(&TopLevelContext, (PIRP)FSRTL_FAST_IO_TOP_LEVEL_IRP,
                                   (PRDBSS_DEVICE_OBJECT)DeviceObject);

    Ret = FsRtlCopyWrite2(FileObject, FileOffset, Length, Wait, LockKey, Buffer,
                          IoStatus, DeviceObject, &TopLevelContext);
    if (Ret)
    {
        DPRINT("Write OK\n");
    }
    else
    {
        DPRINT1("Write failed!\n");
    }

    return Ret;
}

NTSTATUS
RxFindOrCreateFcb(
    PRX_CONTEXT RxContext,
    PUNICODE_STRING NetRootName)
{
    PFCB Fcb;
    ULONG Version;
    NTSTATUS Status;
    PNET_ROOT NetRoot;
    PV_NET_ROOT VNetRoot;
    BOOLEAN TableAcquired, AcquiredExclusive;

    PAGED_CODE();

    NetRoot = (PNET_ROOT)RxContext->Create.pNetRoot;
    VNetRoot = (PV_NET_ROOT)RxContext->Create.pVNetRoot;
    ASSERT(NetRoot == VNetRoot->NetRoot);

    Status = STATUS_SUCCESS;
    AcquiredExclusive = FALSE;

    RxAcquireFcbTableLockShared(&NetRoot->FcbTable, TRUE);
    TableAcquired = TRUE;
    Version = NetRoot->FcbTable.Version;

    /* Look for a cached FCB */
    Fcb = RxFcbTableLookupFcb(&NetRoot->FcbTable, NetRootName);
    if (Fcb == NULL)
    {
        DPRINT("RxFcbTableLookupFcb returned NULL fcb for %wZ\n", NetRootName);
    }
    else
    {
        DPRINT("FCB found for %wZ\n", &Fcb->FcbTableEntry.Path);
        /* If FCB was to be orphaned, consider it as not suitable */
        if (Fcb->fShouldBeOrphaned)
        {
            RxDereferenceNetFcb(Fcb);
            RxReleaseFcbTableLock(&NetRoot->FcbTable);

            RxAcquireFcbTableLockExclusive(&NetRoot->FcbTable, TRUE);
            TableAcquired = TRUE;
            AcquiredExclusive = TRUE;

            Fcb = RxFcbTableLookupFcb(&NetRoot->FcbTable, NetRootName);
            if (Fcb != NULL && Fcb->fShouldBeOrphaned)
            {
                RxOrphanThisFcb(Fcb);
                RxDereferenceNetFcb(Fcb);
                Fcb = NULL;
            }
        }
    }

    /* If FCB was not found or is not covering full path, prepare for more work */
    if (Fcb == NULL || Fcb->FcbTableEntry.Path.Length != NetRootName->Length)
    {
        if (Fcb != NULL)
        {
            DPRINT1("FCB was found and it's not covering the whole path: %wZ - %wZ\n", &Fcb->FcbTableEntry.Path, NetRootName);
        }

        if (!AcquiredExclusive)
        {
            RxReleaseFcbTableLock(&NetRoot->FcbTable);
            RxAcquireFcbTableLockExclusive(&NetRoot->FcbTable, TRUE);
            TableAcquired = TRUE;
        }

        /* If FCB table was updated in between, re-attempt a lookup */
        if (NetRoot->FcbTable.Version != Version)
        {
            Fcb = RxFcbTableLookupFcb(&NetRoot->FcbTable, NetRootName);
            if (Fcb != NULL && Fcb->FcbTableEntry.Path.Length != NetRootName->Length)
            {
                Fcb = NULL;
            }
        }
    }

    /* Allocate the FCB */
    _SEH2_TRY
    {
        if (Fcb == NULL)
        {
            Fcb = RxCreateNetFcb(RxContext, VNetRoot, NetRootName);
            if (Fcb == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                Status = RxAcquireExclusiveFcb(RxContext, Fcb);
                RxContext->Create.FcbAcquired = NT_SUCCESS(Status);
            }
        }
    }
    _SEH2_FINALLY
    {
        if (_SEH2_AbnormalTermination())
        {
            RxReleaseFcbTableLock(&NetRoot->FcbTable);
            TableAcquired = FALSE;

            if (Fcb != NULL)
            {
                RxTransitionNetFcb(Fcb, Condition_Bad);

                ExAcquireResourceExclusiveLite(Fcb->Header.Resource, TRUE);
                if (RxDereferenceAndFinalizeNetFcb(Fcb, NULL, FALSE, FALSE) != 0)
                {
                    ExReleaseResourceLite(Fcb->Header.Resource);
                }
            }
        }
    }
    _SEH2_END;

    if (TableAcquired)
    {
        RxReleaseFcbTableLock(&NetRoot->FcbTable);
    }

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    RxContext->pFcb = RX_GET_MRX_FCB(Fcb);
    DPRINT("FCB %p is in condition %lx\n", Fcb, Fcb->Condition);

    if (!RxContext->Create.FcbAcquired)
    {
        RxWaitForStableNetFcb(Fcb, RxContext);
        Status = RxAcquireExclusiveFcb(RxContext, Fcb);
        RxContext->Create.FcbAcquired = NT_SUCCESS(Status);
    }

    return Status;
}

NTSTATUS
RxFirstCanonicalize(
    PRX_CONTEXT RxContext,
    PUNICODE_STRING FileName,
    PUNICODE_STRING CanonicalName,
    PNET_ROOT_TYPE NetRootType)
{
    NTSTATUS Status;
    NET_ROOT_TYPE Type;
    BOOLEAN UncName, PrependString, IsSpecial;
    USHORT CanonicalLength;
    UNICODE_STRING SessionIdString;
    WCHAR SessionIdBuffer[16];

    PAGED_CODE();

    Type = NET_ROOT_WILD;
    PrependString = FALSE;
    IsSpecial = FALSE;
    UncName = FALSE;
    Status = STATUS_SUCCESS;

    /* Name has to contain at least \\ */
    if (FileName->Length < 2 * sizeof(WCHAR))
    {
        return STATUS_OBJECT_NAME_INVALID;
    }

    /* First easy check, is that a path with a name? */
    CanonicalLength = FileName->Length;
    if (FileName->Length > 5 * sizeof(WCHAR))
    {
        if (FileName->Buffer[0] == '\\' && FileName->Buffer[1] == ';')
        {
            if (FileName->Buffer[3] == ':')
            {
                Type = NET_ROOT_DISK;
            }
            else
            {
                Type = NET_ROOT_PRINT;
            }
        }
    }

    /* Nope, attempt deeper parsing */
    if (FileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR && FileName->Buffer[1] != ';')
    {
        ULONG SessionId;
        PWSTR FirstSlash, EndOfString;

        SetFlag(RxContext->Create.Flags, RX_CONTEXT_CREATE_FLAG_UNC_NAME);
        UncName = TRUE;

        /* The lack of drive letter will be replaced by session ID */
        SessionId = RxGetSessionId(RxContext->CurrentIrpSp);
        RtlInitEmptyUnicodeString(&SessionIdString, SessionIdBuffer, sizeof(SessionIdBuffer));
        RtlIntegerToUnicodeString(SessionId, 10, &SessionIdString);

        EndOfString = Add2Ptr(FileName->Buffer, FileName->Length);
        for (FirstSlash = &FileName->Buffer[1]; FirstSlash != EndOfString; ++FirstSlash)
        {
            if (*FirstSlash == OBJ_NAME_PATH_SEPARATOR)
            {
                break;
            }
        }

        if (EndOfString - FirstSlash <= 1)
        {
            Status = STATUS_OBJECT_NAME_INVALID;
        }
        else
        {
            UNIMPLEMENTED;
            DPRINT1("WARNING: Assuming not special + disk!\n");
            Type = NET_ROOT_DISK;
            Status = STATUS_SUCCESS;
            //Status = STATUS_NOT_IMPLEMENTED;
            /* Should be check against IPC, mailslot, and so on */
        }
    }

    /* Update net root type with our deduced one */
    *NetRootType = Type;
    DPRINT("Returning type: %x\n", Type);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Do we have to prepend session ID? */
    if (UncName)
    {
        if (!IsSpecial)
        {
            PrependString = TRUE;
            CanonicalLength += SessionIdString.Length + 3 * sizeof(WCHAR);
        }
    }

    /* If not UNC path, we should preprend stuff */
    if (!PrependString && !IsSpecial && FileName->Buffer[0] != '\\')
    {
        return STATUS_OBJECT_PATH_INVALID;
    }

    /* Allocate the buffer */
    Status = RxAllocateCanonicalNameBuffer(RxContext, CanonicalName, CanonicalLength);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* We don't support that case, we always return disk */
    if (IsSpecial)
    {
        ASSERT(CanonicalName->Length == CanonicalLength);
        UNIMPLEMENTED;
        Status = STATUS_NOT_IMPLEMENTED;
    }
    else
    {
        /* If we have to prepend, go ahead */
        if (PrependString)
        {
            CanonicalName->Buffer[0] = '\\';
            CanonicalName->Buffer[1] = ';';
            CanonicalName->Buffer[2] = ':';
            CanonicalName->Length = 3 * sizeof(WCHAR);
            RtlAppendUnicodeStringToString(CanonicalName, &SessionIdString);
            RtlAppendUnicodeStringToString(CanonicalName, FileName);

            DPRINT1("CanonicalName: %wZ\n", CanonicalName);
        }
        /* Otherwise, that's a simple copy */
        else
        {
            RtlCopyUnicodeString(CanonicalName, FileName);
        }
    }

    return Status;
}

/*
 * @implemented
 */
VOID
RxFreeCanonicalNameBuffer(
    PRX_CONTEXT Context)
{
    /* These two buffers are always the same */
    ASSERT(Context->Create.CanonicalNameBuffer == Context->AlsoCanonicalNameBuffer);

    if (Context->Create.CanonicalNameBuffer != NULL)
    {
        RxFreePoolWithTag(Context->Create.CanonicalNameBuffer, RX_MISC_POOLTAG);
        Context->Create.CanonicalNameBuffer = NULL;
        Context->AlsoCanonicalNameBuffer = NULL;
    }

    ASSERT(Context->AlsoCanonicalNameBuffer == NULL);
}

NTSTATUS
RxFsdCommonDispatch(
    PRX_FSD_DISPATCH_VECTOR DispatchVector,
    UCHAR MajorFunction,
    PIO_STACK_LOCATION Stack,
    PFILE_OBJECT FileObject,
    PIRP Irp,
    PRDBSS_DEVICE_OBJECT RxDeviceObject)
{
    KIRQL OldIrql;
    NTSTATUS Status;
    PRX_CONTEXT Context;
    UCHAR MinorFunction;
    PFILE_OBJECT StackFileObject;
    PRX_FSD_DISPATCH DispatchFunc;
    RX_TOPLEVELIRP_CONTEXT TopLevelContext;
    BOOLEAN TopLevel, Closing, PassToDriver, SetCancelRoutine, PostRequest, CanWait;

    Status = STATUS_SUCCESS;

    DPRINT("RxFsdCommonDispatch(%p, %d, %p, %p, %p, %p)\n", DispatchVector, MajorFunction, Stack, FileObject, Irp, RxDeviceObject);

    FsRtlEnterFileSystem();

    TopLevel = RxTryToBecomeTheTopLevelIrp(&TopLevelContext, Irp, RxDeviceObject, FALSE);

    _SEH2_TRY
    {
        CanWait = TRUE;
        Closing = FALSE;
        PostRequest = FALSE;
        SetCancelRoutine = TRUE;
        MinorFunction = Stack->MinorFunction;
        /* Can we wait? */
        switch (MajorFunction)
        {
            case IRP_MJ_FILE_SYSTEM_CONTROL:
                if (FileObject != NULL)
                {
                    CanWait = IoIsOperationSynchronous(Irp);
                }
                else
                {
                    CanWait = TRUE;
                }
                break;

            case IRP_MJ_READ:
            case IRP_MJ_WRITE:
            case IRP_MJ_QUERY_INFORMATION:
            case IRP_MJ_SET_INFORMATION:
            case IRP_MJ_QUERY_EA:
            case IRP_MJ_SET_EA:
            case IRP_MJ_FLUSH_BUFFERS:
            case IRP_MJ_QUERY_VOLUME_INFORMATION:
            case IRP_MJ_SET_VOLUME_INFORMATION:
            case IRP_MJ_DIRECTORY_CONTROL:
            case IRP_MJ_DEVICE_CONTROL:
            case IRP_MJ_LOCK_CONTROL:
            case IRP_MJ_QUERY_SECURITY:
            case IRP_MJ_SET_SECURITY:
                CanWait = IoIsOperationSynchronous(Irp);
                break;

            case IRP_MJ_CLOSE:
            case IRP_MJ_CLEANUP:
                Closing = TRUE;
                SetCancelRoutine = FALSE;
                break;

            default:
                break;
        }

        KeAcquireSpinLock(&RxStrucSupSpinLock, &OldIrql);
        /* Should we stop it right now, or mini-rdr deserves to know? */
        PassToDriver = TRUE;
        if (RxGetRdbssState(RxDeviceObject) != RDBSS_STARTABLE)
        {
            if (RxGetRdbssState(RxDeviceObject) == RDBSS_STOP_IN_PROGRESS && !Closing)
            {
                PassToDriver = FALSE;
                Status = STATUS_REDIRECTOR_NOT_STARTED;
                DPRINT1("Not started!\n");
            }
        }
        else
        {
            if (DispatchVector != RxDeviceFCBVector && (FileObject->FileName.Length != 0 || FileObject->RelatedFileObject != NULL))
            {
                PassToDriver = FALSE;
                Status = STATUS_REDIRECTOR_NOT_STARTED;
                DPRINT1("Not started!\n");
            }
        }
        KeReleaseSpinLock(&RxStrucSupSpinLock, OldIrql);

        StackFileObject = Stack->FileObject;
        /* Make sure we don't deal with orphaned stuff */
        if (StackFileObject != NULL && StackFileObject->FsContext != NULL)
        {
            if (StackFileObject->FsContext2 != UIntToPtr(DFS_OPEN_CONTEXT) &&
                StackFileObject->FsContext2 != UIntToPtr(DFS_DOWNLEVEL_OPEN_CONTEXT) &&
                StackFileObject->FsContext != &RxDeviceFCB)
            {
                PFCB Fcb;
                PFOBX Fobx;

                Fcb = StackFileObject->FsContext;
                Fobx = StackFileObject->FsContext2;

                if (BooleanFlagOn(Fcb->FcbState, FCB_STATE_ORPHANED) ||
                    ((Fobx != NULL) && BooleanFlagOn(Fobx->pSrvOpen->Flags, SRVOPEN_FLAG_ORPHANED)))
                {
                    if (Closing)
                    {
                        PassToDriver = TRUE;
                    }
                    else
                    {
                        PassToDriver = FALSE;
                        Status = STATUS_UNEXPECTED_NETWORK_ERROR;
                        DPRINT1("Operation on orphaned FCB: %p\n", Fcb);
                    }
                }
            }
        }

        /* Did we receive a close request whereas we're stopping? */
        if (RxGetRdbssState(RxDeviceObject) == RDBSS_STOP_IN_PROGRESS && Closing)
        {
            PFCB Fcb;

            Fcb = StackFileObject->FsContext;

            DPRINT1("Close received after stop\n");
            DPRINT1("Irp: %p  %d:%d FO: %p FCB: %p\n",
                    Irp, Stack->MajorFunction, Stack->MinorFunction, StackFileObject, Fcb);

            if (Fcb != NULL && Fcb != &RxDeviceFCB &&
                NodeTypeIsFcb(Fcb))
            {
                DPRINT1("OpenCount: %ld, UncleanCount: %ld, Name: %wZ\n",
                        Fcb->OpenCount, Fcb->UncleanCount, &Fcb->FcbTableEntry.Path);
            }
        }

        /* Should we stop the whole thing now? */
        if (!PassToDriver)
        {
            if (MajorFunction != IRP_MJ_DIRECTORY_CONTROL || MinorFunction != IRP_MN_REMOVE_DEVICE)
            {
                IoMarkIrpPending(Irp);
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                Status = STATUS_PENDING;
            }
            else
            {
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }

            _SEH2_LEAVE;
        }

        /* No? Allocate a context to deal with the mini-rdr */
        Context = RxCreateRxContext(Irp, RxDeviceObject, (CanWait ? RX_CONTEXT_FLAG_WAIT : 0));
        if (Context == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            RxCompleteRequest_Real(RxNull, Irp, STATUS_INSUFFICIENT_RESOURCES);
            _SEH2_LEAVE;
        }

        /* Set cancel routine if required */
        if (SetCancelRoutine)
        {
            IoAcquireCancelSpinLock(&OldIrql);
            IoSetCancelRoutine(Irp, RxCancelRoutine);
        }
        else
        {
            IoAcquireCancelSpinLock(&OldIrql);
            IoSetCancelRoutine(Irp, NULL);
        }
        IoReleaseCancelSpinLock(OldIrql);

        ASSERT(MajorFunction <= IRP_MJ_MAXIMUM_FUNCTION);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        /* Get the dispatch routine */
        DispatchFunc = DispatchVector[MajorFunction].CommonRoutine;

        if (MajorFunction == IRP_MJ_READ || MajorFunction == IRP_MJ_WRITE)
        {
            /* Handle the complete MDL case */
            if (BooleanFlagOn(MinorFunction, IRP_MN_COMPLETE))
            {
                DispatchFunc = RxCompleteMdl;
            }
            else
            {
                /* Do we have to post request? */
                if (BooleanFlagOn(MinorFunction, IRP_MN_DPC))
                {
                    PostRequest = TRUE;
                }
                else
                {
                    /* Our read function needs stack, make sure we won't overflow,
                     * otherwise, post the request
                     */
                    if (MajorFunction == IRP_MJ_READ)
                    {
                        if (IoGetRemainingStackSize() < 0xE00)
                        {
                            Context->PendingReturned = TRUE;
                            Status = RxPostStackOverflowRead(Context);
                            if (Status != STATUS_PENDING)
                            {
                                Context->PendingReturned = FALSE;
                                RxCompleteAsynchronousRequest(Context, Status);
                            }

                            _SEH2_LEAVE;
                        }
                    }
                }
            }
        }

        Context->ResumeRoutine = DispatchFunc;
        /* There's a dispatch routine? Time to dispatch! */
        if (DispatchFunc != NULL)
        {
            Context->PendingReturned = TRUE;
            if (PostRequest)
            {
                Status = RxFsdPostRequest(Context);
            }
            else
            {
                /* Retry as long as we have */
                do
                {
                    Status = DispatchFunc(Context);
                }
                while (Status == STATUS_RETRY);

                if (Status == STATUS_PENDING)
                {
                    _SEH2_LEAVE;
                }

                /* Sanity check: did someone mess with our context? */
                if (Context->CurrentIrp != Irp || Context->CurrentIrpSp != Stack ||
                    Context->MajorFunction != MajorFunction || Stack->MinorFunction != MinorFunction)
                {
                    DPRINT1("RX_CONTEXT %p has been contaminated!\n", Context);
                    DPRINT1("->CurrentIrp %p %p\n", Context->CurrentIrp, Irp);
                    DPRINT1("->CurrentIrpSp %p %p\n", Context->CurrentIrpSp, Stack);
                    DPRINT1("->MajorFunction %d %d\n", Context->MajorFunction, MajorFunction);
                    DPRINT1("->MinorFunction %d %d\n", Context->MinorFunction, MinorFunction);
                }
                Context->PendingReturned = FALSE;
                Status = RxCompleteAsynchronousRequest(Context, Status);
            }
        }
        else
        {
            Status = STATUS_NOT_IMPLEMENTED;
        }
    }
    _SEH2_FINALLY
    {
        if (TopLevel)
        {
            RxUnwindTopLevelIrp(&TopLevelContext);
        }

        FsRtlExitFileSystem();
    }
    _SEH2_END;

    DPRINT("RxFsdDispatch, Status: %lx\n", Status);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxFsdDispatch(
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN PIRP Irp)
{
    PFCB Fcb;
    PIO_STACK_LOCATION Stack;
    PRX_FSD_DISPATCH_VECTOR DispatchVector;

    PAGED_CODE();

    DPRINT("RxFsdDispatch(%p, %p)\n", RxDeviceObject, Irp);

    Stack = IoGetCurrentIrpStackLocation(Irp);

    /* Dispatch easy case */
    if (Stack->MajorFunction == IRP_MJ_SYSTEM_CONTROL)
    {
        return RxSystemControl(RxDeviceObject, Irp);
    }

    /* Bail out broken cases */
    if (Stack->MajorFunction == IRP_MJ_CREATE_MAILSLOT ||
        Stack->MajorFunction == IRP_MJ_CREATE_NAMED_PIPE)
    {
        IoMarkIrpPending(Irp);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_OBJECT_NAME_INVALID;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_PENDING;
    }

    /* Immediately handle create */
    if (Stack->MajorFunction == IRP_MJ_CREATE)
    {
        return RxFsdCommonDispatch(&RxFsdDispatchVector[0], Stack->MajorFunction, Stack, Stack->FileObject, Irp, RxDeviceObject);
    }

    /* If not a creation, we must have at least a FO with a FCB */
    if (Stack->FileObject == NULL || Stack->FileObject->FsContext == NULL)
    {
        IoMarkIrpPending(Irp);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_PENDING;
    }

    /* Set the dispatch vector if required */
    Fcb = Stack->FileObject->FsContext;
    if (!NodeTypeIsFcb(Fcb) || Fcb->PrivateDispatchVector == NULL)
    {
        DispatchVector = &RxFsdDispatchVector[0];
    }
    else
    {
        DispatchVector = Fcb->PrivateDispatchVector;
    }

    /* Device cannot accept such requests */
    if (RxDeviceObject == RxFileSystemDeviceObject)
    {
        IoMarkIrpPending(Irp);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_PENDING;
    }

    /* Dispatch for real! */
    return RxFsdCommonDispatch(DispatchVector, Stack->MajorFunction, Stack, Stack->FileObject, Irp, RxDeviceObject);
}

/*
 * @implemented
 */
NTSTATUS
RxFsdPostRequest(
    IN PRX_CONTEXT RxContext)
{
    /* Initialize posting if required */
    if (!BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_NO_PREPOSTING_NEEDED))
    {
        RxPrePostIrp(RxContext, RxContext->CurrentIrp);
    }

    DPRINT("Posting MN: %d, Ctxt: %p, IRP: %p, Thrd: %lx #%lx\n",
           RxContext->MinorFunction, RxContext,
           RxContext->CurrentIrp, RxContext->LastExecutionThread,
           RxContext->SerialNumber);

    RxAddToWorkque(RxContext, RxContext->CurrentIrp);
    return STATUS_PENDING;
}

/*
 * @implemented
 */
VOID
NTAPI
RxFspDispatch(
    IN PVOID Context)
{
    KIRQL EntryIrql;
    WORK_QUEUE_TYPE Queue;
    PRDBSS_DEVICE_OBJECT VolumeDO;
    PRX_CONTEXT RxContext, EntryContext;

    PAGED_CODE();

    RxContext = Context;
    EntryContext = Context;
    /* Save IRQL at entry for later checking */
    EntryIrql = KeGetCurrentIrql();

    /* No FO, deal with device */
    if (RxContext->CurrentIrpSp->FileObject != NULL)
    {
        VolumeDO = RxFileSystemDeviceObject;
    }
    else
    {
        VolumeDO = NULL;
    }

    /* Which queue to used for delayed? */
    if (BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_FSP_DELAYED_OVERFLOW_QUEUE))
    {
        Queue = DelayedWorkQueue;
    }
    else
    {
        ASSERT(BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_FSP_CRITICAL_OVERFLOW_QUEUE));
        Queue = CriticalWorkQueue;
    }

    do
    {
        PIRP Irp;
        NTSTATUS Status;
        BOOLEAN RecursiveCall;
        RX_TOPLEVELIRP_CONTEXT TopLevelContext;

        ASSERT(RxContext->MajorFunction <= IRP_MJ_MAXIMUM_FUNCTION);
        ASSERT(!RxContext->PostRequest);

        RxContext->LastExecutionThread = PsGetCurrentThread();
        SetFlag(RxContext->Flags, (RX_CONTEXT_FLAG_IN_FSP | RX_CONTEXT_FLAG_WAIT));

        DPRINT("Dispatch: MN: %d, Ctxt: %p, IRP: %p, THRD: %lx #%lx\n", RxContext->MinorFunction,
               RxContext, RxContext->CurrentIrp, RxContext->LastExecutionThread,
               RxContext->SerialNumber);

        Irp = RxContext->CurrentIrp;

        FsRtlEnterFileSystem();

        RecursiveCall = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_RECURSIVE_CALL);
        RxTryToBecomeTheTopLevelIrp(&TopLevelContext,
                                    (RecursiveCall ? (PIRP)FSRTL_FSP_TOP_LEVEL_IRP : RxContext->CurrentIrp),
                                    RxContext->RxDeviceObject, TRUE);

        ASSERT(RxContext->ResumeRoutine != NULL);

        if (BooleanFlagOn(RxContext->MinorFunction, IRP_MN_DPC) && Irp->Tail.Overlay.Thread == NULL)
        {
            ASSERT((RxContext->MajorFunction == IRP_MJ_WRITE) || (RxContext->MajorFunction == IRP_MJ_READ));
            Irp->Tail.Overlay.Thread = PsGetCurrentThread();
        }

        /* Call the resume routine */
        do
        {
            BOOLEAN NoComplete;

            NoComplete = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_NO_COMPLETE_FROM_FSP);

            Status = RxContext->ResumeRoutine(RxContext);
            if (!NoComplete && Status != STATUS_PENDING)
            {
                if (Status != STATUS_RETRY)
                {
                    Status = RxCompleteRequest(RxContext, Status);
                }
            }
        }
        while (Status == STATUS_RETRY);

        RxUnwindTopLevelIrp(&TopLevelContext);
        FsRtlExitFileSystem();

        if (VolumeDO != NULL)
        {
            RxContext = RxRemoveOverflowEntry(VolumeDO, Queue);
        }
        else
        {
            RxContext = NULL;
        }
    } while (RxContext != NULL);

    /* Did we mess with IRQL? */
    if (KeGetCurrentIrql() >= APC_LEVEL)
    {
        DPRINT1("High IRQL for Ctxt %p, on entry: %x\n", EntryContext, EntryIrql);
    }
}

/*
 * @implemented
 */
ULONG
RxGetNetworkProviderPriority(
    PUNICODE_STRING DeviceName)
{
    PAGED_CODE();
    return 1;
}

/*
 * @implemented
 */
VOID
NTAPI
RxGetRegistryParameters(
    IN PUNICODE_STRING RegistryPath)
{
    USHORT i;
    NTSTATUS Status;
    UCHAR Buffer[0x400];
    HANDLE DriverHandle, KeyHandle;
    UNICODE_STRING KeyName, OutString;
    OBJECT_ATTRIBUTES ObjectAttributes;

    PAGED_CODE();

    InitializeObjectAttributes(&ObjectAttributes, RegistryPath, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = ZwOpenKey(&DriverHandle, READ_CONTROL | KEY_NOTIFY | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    RtlInitUnicodeString(&KeyName, L"Parameters");
    InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, DriverHandle, FALSE);
    Status = ZwOpenKey(&KeyHandle, READ_CONTROL | KEY_NOTIFY | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* The only parameter we deal with is InitialDebugString */
        RxGetStringRegistryParameter(KeyHandle, L"InitialDebugString", &OutString, Buffer, sizeof(Buffer), 0);
        if (OutString.Length != 0 && OutString.Length < 0x140)
        {
            PWSTR Read;
            PSTR Write;

            Read = OutString.Buffer;
            Write = (PSTR)OutString.Buffer;
            for (i = 0; i < OutString.Length; ++i)
            {
                *Read = *Write;
                ++Write;
                *Write = ANSI_NULL;
                ++Read;
            }

            /* Which is a string we'll just write out */
            DPRINT("InitialDebugString read from registry: '%s'\n", OutString.Buffer);
            RxDebugControlCommand((PSTR)OutString.Buffer);
        }

        ZwClose(KeyHandle);
    }

    ZwClose(DriverHandle);
}

/*
 * @implemented
 */
ULONG
RxGetSessionId(
    IN PIO_STACK_LOCATION IrpSp)
{
    ULONG SessionId;
    PACCESS_TOKEN Token;
    PIO_SECURITY_CONTEXT SecurityContext;

    PAGED_CODE();

    /* If that's not a prefix claim, not an open request, session id will be 0 */
    if (IrpSp->MajorFunction != IRP_MJ_DEVICE_CONTROL || IrpSp->Parameters.DeviceIoControl.IoControlCode != IOCTL_REDIR_QUERY_PATH)
    {
        if (IrpSp->MajorFunction != IRP_MJ_CREATE || IrpSp->Parameters.Create.SecurityContext == NULL)
        {
            return 0;
        }

        SecurityContext = IrpSp->Parameters.Create.SecurityContext;
    }
    else
    {
        SecurityContext = ((PQUERY_PATH_REQUEST)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer)->SecurityContext;
    }

    /* Query the session id */
    Token = SeQuerySubjectContextToken(&SecurityContext->AccessState->SubjectSecurityContext);
    SeQuerySessionIdToken(Token, &SessionId);

    return SessionId;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxGetStringRegistryParameter(
    IN HANDLE KeyHandle,
    IN PCWSTR KeyName,
    OUT PUNICODE_STRING OutString,
    IN PUCHAR Buffer,
    IN ULONG BufferLength,
    IN BOOLEAN LogFailure)
{
    NTSTATUS Status;
    ULONG ResultLength;
    UNICODE_STRING KeyString;

    PAGED_CODE();

    RtlInitUnicodeString(&KeyString, KeyName);
    Status = ZwQueryValueKey(KeyHandle, &KeyString, KeyValuePartialInformation, Buffer, BufferLength, &ResultLength);
    OutString->Length = 0;
    OutString->Buffer = 0;
    if (!NT_SUCCESS(Status))
    {
        if (LogFailure)
        {
            RxLogFailure(RxFileSystemDeviceObject, NULL, 0x80000BD3, Status);
        }

        return Status;
    }

    OutString->Buffer = (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data);
    OutString->Length = ((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->DataLength - sizeof(UNICODE_NULL);
    OutString->MaximumLength = OutString->Length;

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
PRDBSS_DEVICE_OBJECT
RxGetTopDeviceObjectIfRdbssIrp(
    VOID)
{
    PIRP TopLevelIrp;
    PRDBSS_DEVICE_OBJECT TopDevice = NULL;

    TopLevelIrp = IoGetTopLevelIrp();
    if (RxIsThisAnRdbssTopLevelContext((PRX_TOPLEVELIRP_CONTEXT)TopLevelIrp))
    {
        TopDevice = ((PRX_TOPLEVELIRP_CONTEXT)TopLevelIrp)->RxDeviceObject;
    }

    return TopDevice;
}

/*
 * @implemented
 */
PIRP
RxGetTopIrpIfRdbssIrp(
    VOID)
{
    PIRP Irp = NULL;
    PRX_TOPLEVELIRP_CONTEXT TopLevel;

    TopLevel = (PRX_TOPLEVELIRP_CONTEXT)IoGetTopLevelIrp();
    if (RxIsThisAnRdbssTopLevelContext(TopLevel))
    {
        Irp = TopLevel->Irp;
    }

    return Irp;
}

/*
 * @implemented
 */
LUID
RxGetUid(
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext)
{
    LUID Luid;
    PACCESS_TOKEN Token;

    PAGED_CODE();

    Token = SeQuerySubjectContextToken(SubjectSecurityContext);
    SeQueryAuthenticationIdToken(Token, &Luid);

    return Luid;
}

VOID
NTAPI
RxIndicateChangeOfBufferingStateForSrvOpen(
    PMRX_SRV_CALL SrvCall,
    PMRX_SRV_OPEN SrvOpen,
    PVOID SrvOpenKey,
    PVOID Context)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
RxInitializeDispatchVectors(
    PDRIVER_OBJECT DriverObject)
{
    USHORT i;

    PAGED_CODE();

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i)
    {
        DriverObject->MajorFunction[i] = (PDRIVER_DISPATCH)RxFsdDispatch;
    }

    RxDeviceFCB.PrivateDispatchVector = RxDeviceFCBVector;
    ASSERT(RxFsdDispatchVector[IRP_MJ_MAXIMUM_FUNCTION].CommonRoutine != NULL);
    ASSERT(RxDeviceFCBVector[IRP_MJ_MAXIMUM_FUNCTION].CommonRoutine != NULL);

    DriverObject->FastIoDispatch = &RxFastIoDispatch;
    RxFastIoDispatch.SizeOfFastIoDispatch = sizeof(RxFastIoDispatch);
    RxFastIoDispatch.FastIoCheckIfPossible = RxFastIoCheckIfPossible;
    RxFastIoDispatch.FastIoRead = RxFastIoRead;
    RxFastIoDispatch.FastIoWrite = RxFastIoWrite;
    RxFastIoDispatch.FastIoQueryBasicInfo = NULL;
    RxFastIoDispatch.FastIoQueryStandardInfo = NULL;
    RxFastIoDispatch.FastIoLock = NULL;
    RxFastIoDispatch.FastIoUnlockSingle = NULL;
    RxFastIoDispatch.FastIoUnlockAll = NULL;
    RxFastIoDispatch.FastIoUnlockAllByKey = NULL;
    RxFastIoDispatch.FastIoDeviceControl = RxFastIoDeviceControl;
    RxFastIoDispatch.AcquireFileForNtCreateSection = RxAcquireFileForNtCreateSection;
    RxFastIoDispatch.ReleaseFileForNtCreateSection = RxReleaseFileForNtCreateSection;
    RxFastIoDispatch.AcquireForCcFlush = RxAcquireForCcFlush;
    RxFastIoDispatch.ReleaseForCcFlush = RxReleaseForCcFlush;

    RxInitializeTopLevelIrpPackage();

    RxData.CacheManagerCallbacks.AcquireForLazyWrite = RxAcquireFcbForLazyWrite;
    RxData.CacheManagerCallbacks.ReleaseFromLazyWrite = RxReleaseFcbFromLazyWrite;
    RxData.CacheManagerCallbacks.AcquireForReadAhead = RxAcquireFcbForReadAhead;
    RxData.CacheManagerCallbacks.ReleaseFromReadAhead = RxReleaseFcbFromReadAhead;

    RxData.CacheManagerNoOpCallbacks.AcquireForLazyWrite = RxNoOpAcquire;
    RxData.CacheManagerNoOpCallbacks.ReleaseFromLazyWrite = RxNoOpRelease;
    RxData.CacheManagerNoOpCallbacks.AcquireForReadAhead = RxNoOpAcquire;
    RxData.CacheManagerNoOpCallbacks.ReleaseFromReadAhead = RxNoOpRelease;
}

NTSTATUS
NTAPI
RxInitializeLog(
    VOID)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
VOID
RxInitializeMinirdrDispatchTable(
    IN PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxInitializeRegistrationStructures(
    VOID)
{
    PAGED_CODE();

    ExInitializeFastMutex(&RxData.MinirdrRegistrationMutex);
    RxData.NumberOfMinirdrsRegistered = 0;
    RxData.NumberOfMinirdrsStarted = 0;
    InitializeListHead(&RxData.RegisteredMiniRdrs);

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
NTAPI
RxInitializeTopLevelIrpPackage(
    VOID)
{
    KeInitializeSpinLock(&TopLevelIrpSpinLock);
    InitializeListHead(&TopLevelIrpAllocatedContextsList);
}

VOID
NTAPI
RxInitUnwind(
    PDRIVER_OBJECT DriverObject,
    USHORT State)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
BOOLEAN
RxIsMemberOfTopLevelIrpAllocatedContextsList(
    PRX_TOPLEVELIRP_CONTEXT TopLevelContext)
{
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry;
    BOOLEAN Found = FALSE;
    PRX_TOPLEVELIRP_CONTEXT ListContext;

    /* Browse all the allocated TLC to find ours */
    KeAcquireSpinLock(&TopLevelIrpSpinLock, &OldIrql);
    for (NextEntry = TopLevelIrpAllocatedContextsList.Flink;
         NextEntry != &TopLevelIrpAllocatedContextsList;
         NextEntry = NextEntry->Flink)
    {
        ListContext = CONTAINING_RECORD(NextEntry, RX_TOPLEVELIRP_CONTEXT, ListEntry);
        ASSERT(ListContext->Signature == RX_TOPLEVELIRP_CONTEXT_SIGNATURE);
        ASSERT(BooleanFlagOn(ListContext->Flags, RX_TOPLEVELCTX_FLAG_FROM_POOL));

        /* Found! */
        if (ListContext == TopLevelContext)
        {
            Found = TRUE;
            break;
        }
    }
    KeReleaseSpinLock(&TopLevelIrpSpinLock, OldIrql);

    return Found;
}

/*
 * @implemented
 */
BOOLEAN
RxIsOkToPurgeFcb(
    PFCB Fcb)
{
    PLIST_ENTRY Entry;

    /* No associated SRV_OPEN, it's OK to purge */
    if (IsListEmpty(&Fcb->SrvOpenList))
    {
        return TRUE;
    }

    /* Only allow to purge if all the associated SRV_OPEN
     * - have no outstanding opens ongoing
     * - have only read attribute set
     */
    for (Entry = Fcb->SrvOpenList.Flink;
         Entry != &Fcb->SrvOpenList;
         Entry = Entry->Flink)
    {
        PSRV_OPEN SrvOpen;

        SrvOpen = CONTAINING_RECORD(Entry, SRV_OPEN, SrvOpenQLinks);

        /* Failing previous needs, don't allow purge */
        if (SrvOpen->UncleanFobxCount != 0 ||
            (SrvOpen->DesiredAccess & 0xFFEFFFFF) != FILE_READ_ATTRIBUTES)
        {
            return FALSE;
        }
    }

    /* All correct, allow purge */
    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
RxIsThisAnRdbssTopLevelContext(
    PRX_TOPLEVELIRP_CONTEXT TopLevelContext)
{
    ULONG_PTR StackTop, StackBottom;

    /* Bail out for flags */
    if ((ULONG_PTR)TopLevelContext <= FSRTL_FAST_IO_TOP_LEVEL_IRP)
    {
        return FALSE;
    }

    /* Is our provided TLC allocated on stack? */
    IoGetStackLimits(&StackTop, &StackBottom);
    if ((ULONG_PTR)TopLevelContext <= StackBottom - sizeof(RX_TOPLEVELIRP_CONTEXT) &&
        (ULONG_PTR)TopLevelContext >= StackTop)
    {
        /* Yes, so check whether it's really a TLC by checking alignement & signature */
        if (!BooleanFlagOn((ULONG_PTR)TopLevelContext, 0x3) && TopLevelContext->Signature == RX_TOPLEVELIRP_CONTEXT_SIGNATURE)
        {
            return TRUE;
        }

        return FALSE;
    }

    /* No, use the helper function */
    return RxIsMemberOfTopLevelIrpAllocatedContextsList(TopLevelContext);
}

/*
 * @implemented
 */
BOOLEAN
RxIsThisTheTopLevelIrp(
    IN PIRP Irp)
{
    PIRP TopLevelIrp;

    /* When we put oursleves as top level, we set TLC as 'IRP', so look for it */
    TopLevelIrp = IoGetTopLevelIrp();
    if (RxIsThisAnRdbssTopLevelContext((PRX_TOPLEVELIRP_CONTEXT)TopLevelIrp))
    {
        TopLevelIrp = ((PRX_TOPLEVELIRP_CONTEXT)TopLevelIrp)->Irp;
    }

    return (TopLevelIrp == Irp);
}

NTSTATUS
NTAPI
RxLockOperationCompletion(
    IN PVOID Context,
    IN PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
RxLogEventDirect(
    IN PRDBSS_DEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING OriginatorId,
    IN ULONG EventId,
    IN NTSTATUS Status,
    IN ULONG Line)
{
    PUNICODE_STRING Originator = OriginatorId;
    LARGE_INTEGER LargeLine;

    /* Set optional parameters */
    LargeLine.QuadPart = Line;
    if (OriginatorId == NULL || OriginatorId->Length == 0)
    {
        Originator = (PUNICODE_STRING)&unknownId;
    }

    /* And log */
    RxLogEventWithAnnotation(DeviceObject, EventId, Status, &LargeLine, sizeof(LargeLine), Originator, 1);
}

VOID
NTAPI
RxLogEventWithAnnotation(
    IN PRDBSS_DEVICE_OBJECT DeviceObject,
    IN ULONG EventId,
    IN NTSTATUS Status,
    IN PVOID DataBuffer,
    IN USHORT DataBufferLength,
    IN PUNICODE_STRING Annotation,
    IN ULONG AnnotationCount)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
RxLowIoCompletion(
    PRX_CONTEXT RxContext)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxLowIoIoCtlShellCompletion(
    PRX_CONTEXT RxContext)
{
    PIRP Irp;
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("RxLowIoIoCtlShellCompletion(%p)\n", RxContext);

    Irp = RxContext->CurrentIrp;
    Status = RxContext->IoStatusBlock.Status;

    /* Set information and status */
    if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW)
    {
        Irp->IoStatus.Information = RxContext->IoStatusBlock.Information;
    }

    Irp->IoStatus.Status = Status;

    return Status;
}

NTSTATUS
RxLowIoLockControlShell(
    IN PRX_CONTEXT RxContext)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxLowIoNotifyChangeDirectoryCompletion(
    PRX_CONTEXT RxContext)
{
    PAGED_CODE();

    DPRINT("Completing NCD with: %lx, %lx\n", RxContext->IoStatusBlock.Status, RxContext->IoStatusBlock.Information);

    /* Just copy back the IO_STATUS to the IRP */
    RxSetIoStatusStatus(RxContext, RxContext->IoStatusBlock.Status);
    RxSetIoStatusInfo(RxContext, RxContext->IoStatusBlock.Information);

    return RxContext->IoStatusBlock.Status;
}

/*
 * @implemented
 */
NTSTATUS
RxLowIoReadShell(
    PRX_CONTEXT RxContext)
{
    PFCB Fcb;
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("RxLowIoReadShell(%p)\n", RxContext);

    Fcb = (PFCB)RxContext->pFcb;
    if (BooleanFlagOn(Fcb->FcbState, FCB_STATE_FILE_IS_SHADOWED))
    {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Always update stats for disks */
    if (Fcb->CachedNetRootType == NET_ROOT_DISK)
    {
        ExInterlockedAddLargeStatistic(&RxContext->RxDeviceObject->NetworkReadBytesRequested, RxContext->LowIoContext.ParamsFor.ReadWrite.ByteCount);
    }

    /* And forward the read to the mini-rdr */
    Status = RxLowIoSubmit(RxContext, RxLowIoReadShellCompletion);
    DPRINT("RxLowIoReadShell(%p), Status: %lx\n", RxContext, Status);

    return Status;
}

NTSTATUS
NTAPI
RxLowIoReadShellCompletion(
    PRX_CONTEXT RxContext)
{
    PIRP Irp;
    PFCB Fcb;
    NTSTATUS Status;
    BOOLEAN PagingIo, IsPipe;
    PIO_STACK_LOCATION Stack;
    PLOWIO_CONTEXT LowIoContext;

    PAGED_CODE();

    DPRINT("RxLowIoReadShellCompletion(%p)\n", RxContext);

    Status = RxContext->IoStatusBlock.Status;
    DPRINT("In %p, Status: %lx, Information: %lx\n", RxContext, Status, RxContext->IoStatusBlock.Information);

    Irp = RxContext->CurrentIrp;
    PagingIo = BooleanFlagOn(Irp->Flags, IRP_PAGING_IO);

    /* Set IRP information from the RX_CONTEXT status block */
    Irp->IoStatus.Information = RxContext->IoStatusBlock.Information;

    /* Fixup status for paging file if nothing was read */
    if (PagingIo)
    {
        if (NT_SUCCESS(Status) && RxContext->IoStatusBlock.Information == 0)
        {
            Status = STATUS_END_OF_FILE;
        }
    }

    LowIoContext = &RxContext->LowIoContext;
    ASSERT(RxLowIoIsBufferLocked(LowIoContext));

    /* Check broken cases that should never happen */
    Fcb = (PFCB)RxContext->pFcb;
    if (Status == STATUS_FILE_LOCK_CONFLICT)
    {
        if (BooleanFlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_THIS_READ_ENLARGED))
        {
            ASSERT(FALSE);
            return STATUS_RETRY;
        }
    }
    else if (Status == STATUS_SUCCESS)
    {
        if (BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_RECURSIVE_CALL))
        {
            if (BooleanFlagOn(Fcb->FcbState, FCB_STATE_FILE_IS_DISK_COMPRESSED) ||
                BooleanFlagOn(Fcb->FcbState, FCB_STATE_FILE_IS_BUF_COMPRESSED))
            {
                ASSERT(FALSE);
            }
        }

        if (BooleanFlagOn(Fcb->FcbState, FCB_STATE_FILE_IS_SHADOWED))
        {
            ASSERT(FALSE);
        }
    }

    /* Readahead should go through Cc and not finish here */
    ASSERT(!BooleanFlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_READAHEAD));

    /* If it's sync, RxCommonRead will finish the work - nothing to do here */
    if (BooleanFlagOn(LowIoContext->Flags, LOWIO_CONTEXT_FLAG_SYNCCALL))
    {
        return Status;
    }

    Stack = RxContext->CurrentIrpSp;
    IsPipe = BooleanFlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_OPERATION);
    /* Release lock if required */
    if (PagingIo)
    {
        RxReleasePagingIoResourceForThread(RxContext, Fcb, LowIoContext->ResourceThreadId);
    }
    else
    {
        /* Set FastIo if read was a success */
        if (NT_SUCCESS(Status) && !IsPipe)
        {
            SetFlag(Stack->FileObject->Flags, FO_FILE_FAST_IO_READ);
        }

        if (BooleanFlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION))
        {
            RxResumeBlockedOperations_Serially(RxContext, &((PFOBX)RxContext->pFobx)->Specific.NamedPipe.ReadSerializationQueue);
        }
        else
        {
            RxReleaseFcbForThread(RxContext, Fcb, LowIoContext->ResourceThreadId);
        }
    }

    if (IsPipe)
    {
        UNIMPLEMENTED;
    }

    /* Final sanity checks */
    ASSERT(Status != STATUS_RETRY);
    ASSERT(Irp->IoStatus.Information <= Stack->Parameters.Read.Length);
    ASSERT(RxContext->MajorFunction == IRP_MJ_READ);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
RxLowIoWriteShell(
    IN PRX_CONTEXT RxContext)
{
    PFCB Fcb;
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("RxLowIoWriteShell(%p)\n", RxContext);

    Fcb = (PFCB)RxContext->pFcb;

    ASSERT(!BooleanFlagOn(Fcb->FcbState, FCB_STATE_FILE_IS_BUF_COMPRESSED) &&
           !BooleanFlagOn(Fcb->FcbState, FCB_STATE_FILE_IS_DISK_COMPRESSED));

    /* Always update stats for disks */
    if (Fcb->CachedNetRootType == NET_ROOT_DISK)
    {
        ExInterlockedAddLargeStatistic(&RxContext->RxDeviceObject->NetworkWriteBytesRequested, RxContext->LowIoContext.ParamsFor.ReadWrite.ByteCount);
    }

    /* And forward the write to the mini-rdr */
    Status = RxLowIoSubmit(RxContext, RxLowIoWriteShellCompletion);
    DPRINT("RxLowIoWriteShell(%p), Status: %lx\n", RxContext, Status);

    return Status;
}

NTSTATUS
NTAPI
RxLowIoWriteShellCompletion(
    PRX_CONTEXT RxContext)
{
    PIRP Irp;
    PFCB Fcb;
    NTSTATUS Status;
    BOOLEAN PagingIo;
    PLOWIO_CONTEXT LowIoContext;

    PAGED_CODE();

    DPRINT("RxLowIoWriteShellCompletion(%p)\n", RxContext);

    Status = RxContext->IoStatusBlock.Status;
    DPRINT("In %p, Status: %lx, Information: %lx\n", RxContext, Status, RxContext->IoStatusBlock.Information);

    Irp = RxContext->CurrentIrp;

    /* Set IRP information from the RX_CONTEXT status block */
    Irp->IoStatus.Information = RxContext->IoStatusBlock.Information;

    LowIoContext = &RxContext->LowIoContext;
    ASSERT(RxLowIoIsBufferLocked(LowIoContext));

    /* Perform a few sanity checks */
    Fcb = (PFCB)RxContext->pFcb;
    if (Status == STATUS_SUCCESS)
    {
        if (BooleanFlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_THIS_IO_BUFFERED))
        {
            ASSERT(!BooleanFlagOn(Fcb->FcbState, FCB_STATE_FILE_IS_BUF_COMPRESSED) &&
                   !BooleanFlagOn(Fcb->FcbState, FCB_STATE_FILE_IS_DISK_COMPRESSED));
        }

        ASSERT(!BooleanFlagOn(Fcb->FcbState, FCB_STATE_FILE_IS_SHADOWED));
    }

    PagingIo = BooleanFlagOn(Irp->Flags, IRP_PAGING_IO);
    if (Status != STATUS_SUCCESS && PagingIo)
    {
        DPRINT1("Paging IO failed %p (%p) %lx\n", Fcb, Fcb->NetRoot, Status);
    }

    /* In case of async call, perform last bits not done in RxCommonWrite */
    if (!BooleanFlagOn(LowIoContext->Flags, LOWIO_CONTEXT_FLAG_SYNCCALL))
    {
        PFILE_OBJECT FileObject;
        PIO_STACK_LOCATION Stack;

        /* We only succeed if we wrote what was asked for */
        if (NT_SUCCESS(Status) && !BooleanFlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_OPERATION))
        {
            ASSERT(Irp->IoStatus.Information == LowIoContext->ParamsFor.ReadWrite.ByteCount);
        }

        /* If write succeed, ,also update FILE_OBJECT flags */
        Stack = RxContext->CurrentIrpSp;
        FileObject = Stack->FileObject;
        if (!PagingIo)
        {
            SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
        }

        if (BooleanFlagOn(LowIoContext->ParamsFor.ReadWrite.Flags, LOWIO_READWRITEFLAG_EXTENDING_FILESIZE))
        {
            SetFlag(FileObject->Flags, FO_FILE_SIZE_CHANGED);
        }

        /* If VDL was extended, fix attributes */
        if (BooleanFlagOn(LowIoContext->ParamsFor.ReadWrite.Flags, LOWIO_READWRITEFLAG_EXTENDING_VDL))
        {
            LONGLONG LastOffset, FileSize;

            LastOffset = LowIoContext->ParamsFor.ReadWrite.ByteOffset +
                         Irp->IoStatus.Information;
            RxGetFileSizeWithLock(Fcb, &FileSize);

            if (FileSize < LastOffset)
            {
                LastOffset = FileSize;
            }

            Fcb->Header.ValidDataLength.QuadPart = LastOffset;
        }

        /* One less outstanding write */
        if (!BooleanFlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION))
        {
            PNON_PAGED_FCB NonPagedFcb;

            NonPagedFcb = LowIoContext->ParamsFor.ReadWrite.NonPagedFcb;
            if (NonPagedFcb != NULL)
            {
                if (ExInterlockedAddUlong(&NonPagedFcb->OutstandingAsyncWrites,
                                          -1, &RxStrucSupSpinLock) == 1)
                {
                    KeSetEvent(NonPagedFcb->OutstandingAsyncEvent, IO_NO_INCREMENT, FALSE);
                }
            }
        }

        /* Release paging resource if acquired */
        if (RxContext->FcbPagingIoResourceAcquired)
        {
            RxReleasePagingIoResourceForThread(RxContext, Fcb, LowIoContext->ResourceThreadId);
        }

        /* Resume blocked operations for pipes */
        if (BooleanFlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION))
        {
            RxResumeBlockedOperations_Serially(RxContext,
                                               &((PFOBX)RxContext->pFobx)->Specific.NamedPipe.WriteSerializationQueue);
        }
        else
        {
            /* And release FCB only for files */
            if (RxContext->FcbResourceAcquired)
            {
                RxReleaseFcbForThread(RxContext, Fcb, LowIoContext->ResourceThreadId);
            }
        }

        /* Final sanity checks */
        ASSERT(Status != STATUS_RETRY);
        ASSERT((Status != STATUS_SUCCESS) || (Irp->IoStatus.Information <= Stack->Parameters.Write.Length));
        ASSERT(RxContext->MajorFunction == IRP_MJ_WRITE);

        if (BooleanFlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_OPERATION))
        {
            UNIMPLEMENTED;
        }
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
RxNotifyChangeDirectory(
    PRX_CONTEXT RxContext)
{
    PIRP Irp;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;

    PAGED_CODE();

    /* The IRP can abviously wait */
    SetFlag(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);

    /* Initialize its lowio */
    RxInitializeLowIoContext(&RxContext->LowIoContext, LOWIO_OP_NOTIFY_CHANGE_DIRECTORY);

    _SEH2_TRY
    {
        /* Lock user buffer */
        Stack = RxContext->CurrentIrpSp;
        RxLockUserBuffer(RxContext, IoWriteAccess, Stack->Parameters.NotifyDirectory.Length);

        /* Copy parameters from IO_STACK */
        RxContext->LowIoContext.ParamsFor.NotifyChangeDirectory.WatchTree = BooleanFlagOn(Stack->Flags, SL_WATCH_TREE);
        RxContext->LowIoContext.ParamsFor.NotifyChangeDirectory.CompletionFilter = Stack->Parameters.NotifyDirectory.CompletionFilter;
        RxContext->LowIoContext.ParamsFor.NotifyChangeDirectory.NotificationBufferLength = Stack->Parameters.NotifyDirectory.Length;

        /* If we have an associated MDL */
        Irp = RxContext->CurrentIrp;
        if (Irp->MdlAddress != NULL)
        {
            /* Then, call mini-rdr */
            RxContext->LowIoContext.ParamsFor.NotifyChangeDirectory.pNotificationBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
            if (RxContext->LowIoContext.ParamsFor.NotifyChangeDirectory.pNotificationBuffer != NULL)
            {
                Status = RxLowIoSubmit(RxContext, RxLowIoNotifyChangeDirectoryCompletion);
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else
        {
            Status = STATUS_INVALID_PARAMETER;
        }
    }
    _SEH2_FINALLY
    {
        /* All correct */
    }
    _SEH2_END;

    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
RxpCancelRoutine(
    PVOID Context)
{
    PRX_CONTEXT RxContext;

    PAGED_CODE();

    RxContext = Context;

    /* First, notify mini-rdr about cancellation */
    if (RxContext->MRxCancelRoutine != NULL)
    {
        RxContext->MRxCancelRoutine(RxContext);
    }
    /* If we didn't find in overflow queue, try in blocking operations */
    else if (!RxCancelOperationInOverflowQueue(RxContext))
    {
        RxCancelBlockingOperation(RxContext);
    }

    /* And delete the context */
    RxDereferenceAndDeleteRxContext_Real(RxContext);
}

NTSTATUS
RxPostStackOverflowRead (
    IN PRX_CONTEXT RxContext)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
VOID
RxpPrepareCreateContextForReuse(
    PRX_CONTEXT RxContext)
{
    /* Reuse can only happen for open operations (STATUS_RETRY) */
    ASSERT(RxContext->MajorFunction == IRP_MJ_CREATE);

    /* Release the FCB if it was acquired */
    if (RxContext->Create.FcbAcquired)
    {
        RxReleaseFcb(RxContext, RxContext->pFcb);
        RxContext->Create.FcbAcquired = FALSE;
    }

    /* Free the canonical name */
    RxFreeCanonicalNameBuffer(RxContext);

    /* If we have a VNetRoot associated */
    if (RxContext->Create.pVNetRoot != NULL || RxContext->Create.NetNamePrefixEntry != NULL)
    {
        /* Remove our link and thus, dereference the VNetRoot */
        RxpAcquirePrefixTableLockShared(RxContext->RxDeviceObject->pRxNetNameTable, TRUE, TRUE);
        if (RxContext->Create.pVNetRoot != NULL)
        {
            RxDereferenceVNetRoot(RxContext->Create.pVNetRoot, TRUE);
            RxContext->Create.pVNetRoot = NULL;
        }
        RxpReleasePrefixTableLock(RxContext->RxDeviceObject->pRxNetNameTable, TRUE);
    }

    DPRINT("RxContext: %p prepared for reuse\n", RxContext);
}

/*
 * @implemented
 */
NTSTATUS
RxpQueryInfoMiniRdr(
    PRX_CONTEXT RxContext,
    FILE_INFORMATION_CLASS FileInfoClass,
    PVOID Buffer)
{
    PFCB Fcb;
    NTSTATUS Status;

    Fcb = (PFCB)RxContext->pFcb;

    /* Set the RX_CONTEXT */
    RxContext->Info.FileInformationClass = FileInfoClass;
    RxContext->Info.Buffer = Buffer;

    /* Pass down */
    MINIRDR_CALL(Status, RxContext, Fcb->MRxDispatch, MRxQueryFileInfo, (RxContext));

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
RxPrefixClaim(
    IN PRX_CONTEXT RxContext)
{
    PIRP Irp;
    NTSTATUS Status;
    NET_ROOT_TYPE NetRootType;
    UNICODE_STRING CanonicalName, FileName, NetRootName;

    PAGED_CODE();

    Irp = RxContext->CurrentIrp;

    /* This has to come from MUP */
    if (Irp->RequestorMode == UserMode)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (RxContext->MajorFunction == IRP_MJ_DEVICE_CONTROL)
    {
        PQUERY_PATH_REQUEST QueryRequest;

        /* Get parameters */
        QueryRequest = RxContext->CurrentIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

        /* Don't overflow allocation */
        if (QueryRequest->PathNameLength >= MAXUSHORT - 1)
        {
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        /* Forcefully rewrite IRP MJ */
        RxContext->MajorFunction = IRP_MJ_CREATE;

        /* Fake canon name */
        RxContext->PrefixClaim.SuppliedPathName.Buffer = RxAllocatePoolWithTag(NonPagedPool, QueryRequest->PathNameLength, RX_MISC_POOLTAG);
        if (RxContext->PrefixClaim.SuppliedPathName.Buffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Leave;
        }

        /* Copy the prefix to look for */
        RtlCopyMemory(RxContext->PrefixClaim.SuppliedPathName.Buffer, &QueryRequest->FilePathName[0], QueryRequest->PathNameLength);
        RxContext->PrefixClaim.SuppliedPathName.Length = QueryRequest->PathNameLength;
        RxContext->PrefixClaim.SuppliedPathName.MaximumLength = QueryRequest->PathNameLength;

        /* Zero the create parameters */
        RtlZeroMemory(&RxContext->Create,
                      FIELD_OFFSET(RX_CONTEXT, AlsoCanonicalNameBuffer) - FIELD_OFFSET(RX_CONTEXT, Create.NtCreateParameters));
        RxContext->Create.ThisIsATreeConnectOpen = TRUE;
        RxContext->Create.NtCreateParameters.SecurityContext = QueryRequest->SecurityContext;
    }
    else
    {
        /* If not devcontrol, it comes from open, name was already copied */
        ASSERT(RxContext->MajorFunction == IRP_MJ_CREATE);
        ASSERT(RxContext->PrefixClaim.SuppliedPathName.Buffer != NULL);
    }

    /* Canonilize name */
    NetRootType = NET_ROOT_WILD;
    RtlInitEmptyUnicodeString(&CanonicalName, NULL, 0);
    FileName.Length = RxContext->PrefixClaim.SuppliedPathName.Length;
    FileName.MaximumLength = RxContext->PrefixClaim.SuppliedPathName.MaximumLength;
    FileName.Buffer = RxContext->PrefixClaim.SuppliedPathName.Buffer;
    NetRootName.Length = RxContext->PrefixClaim.SuppliedPathName.Length;
    NetRootName.MaximumLength = RxContext->PrefixClaim.SuppliedPathName.MaximumLength;
    NetRootName.Buffer = RxContext->PrefixClaim.SuppliedPathName.Buffer;
    Status = RxFirstCanonicalize(RxContext, &FileName, &CanonicalName, &NetRootType);
    /* It went fine, attempt to establish a connection (that way we know whether the prefix is accepted) */
    if (NT_SUCCESS(Status))
    {
        Status = RxFindOrConstructVirtualNetRoot(RxContext, &CanonicalName, NetRootType, &NetRootName);
    }
    if (Status == STATUS_PENDING)
    {
        return Status;
    }
    /* Reply to MUP */
    if (NT_SUCCESS(Status))
    {
        PQUERY_PATH_RESPONSE QueryResponse;

        /* We accept the length that was canon (minus netroot) */
        QueryResponse = RxContext->CurrentIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
        QueryResponse->LengthAccepted = RxContext->PrefixClaim.SuppliedPathName.Length - NetRootName.Length;
    }

Leave:
    /* If we reach that point with MJ, reset everything and make IRP being a device control */
    if (RxContext->MajorFunction == IRP_MJ_CREATE)
    {
        if (RxContext->PrefixClaim.SuppliedPathName.Buffer != NULL)
        {
            RxFreePoolWithTag(RxContext->PrefixClaim.SuppliedPathName.Buffer, RX_MISC_POOLTAG);
        }

        RxpPrepareCreateContextForReuse(RxContext);

        RxContext->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxPrepareToReparseSymbolicLink(
    PRX_CONTEXT RxContext,
    BOOLEAN SymbolicLinkEmbeddedInOldPath,
    PUNICODE_STRING NewPath,
    BOOLEAN NewPathIsAbsolute,
    PBOOLEAN ReparseRequired)
{
    PWSTR NewBuffer;
    USHORT NewLength;
    PFILE_OBJECT FileObject;

    /* Assume no reparse is required first */
    *ReparseRequired = FALSE;

    /* Only supported for IRP_MJ_CREATE */
    if (RxContext->MajorFunction != IRP_MJ_CREATE)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* If symbolic link is not embedded, and DELETE is specified, fail */
    if (!SymbolicLinkEmbeddedInOldPath)
    {
        /* Excepted if DELETE is the only flag specified, then, open has to succeed
         * See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/rxprocs/nf-rxprocs-rxpreparetoreparsesymboliclink (remarks)
         */
        if (BooleanFlagOn(RxContext->Create.NtCreateParameters.DesiredAccess, DELETE) &&
            BooleanFlagOn(RxContext->Create.NtCreateParameters.DesiredAccess, ~DELETE))
        {
            return STATUS_ACCESS_DENIED;
        }
    }

    /* At that point, assume reparse will be required */
    *ReparseRequired = TRUE;

    /* If new path isn't absolute, it's up to us to make it absolute */
    if (!NewPathIsAbsolute)
    {
        /* The prefix will be \Device\Mup */
        NewLength = NewPath->Length + (sizeof(L"\\Device\\Mup") - sizeof(UNICODE_NULL));
        NewBuffer = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION, NewLength,
                                          RX_MISC_POOLTAG);
        if (NewBuffer == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Copy data for the new path */
        RtlMoveMemory(NewBuffer, L"\\Device\\Mup", (sizeof(L"\\Device\\Mup") - sizeof(UNICODE_NULL)));
        RtlMoveMemory(Add2Ptr(NewBuffer, (sizeof(L"\\Device\\Mup") - sizeof(UNICODE_NULL))),
                      NewPath->Buffer, NewPath->Length);
    }
    /* Otherwise, use caller path as it */
    else
    {
        NewLength = NewPath->Length;
        NewBuffer = NewPath->Buffer;
    }

    /* Get the FILE_OBJECT we'll modify */
    FileObject = RxContext->CurrentIrpSp->FileObject;

    /* Free old path first */
    ExFreePoolWithTag(FileObject->FileName.Buffer, 0);
    /* And setup new one */
    FileObject->FileName.Length = NewLength;
    FileObject->FileName.MaximumLength = NewLength;
    FileObject->FileName.Buffer = NewBuffer;

    /* And set reparse flag */
    SetFlag(RxContext->Create.Flags, RX_CONTEXT_CREATE_FLAG_REPARSE);

    /* Done! */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
RxPrePostIrp(
    IN PVOID Context,
    IN PIRP Irp)
{
    LOCK_OPERATION Lock;
    PIO_STACK_LOCATION Stack;
    PRX_CONTEXT RxContext = Context;

    /* NULL IRP is no option */
    if (Irp == NULL)
    {
        return;
    }

    /* Check whether preparation was really needed */
    if (BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_NO_PREPOSTING_NEEDED))
    {
        return;
    }
    /* Mark the context as prepared */
    SetFlag(RxContext->Flags, RX_CONTEXT_FLAG_NO_PREPOSTING_NEEDED);

    /* Just lock the user buffer, with the correct length, depending on the MJ */
    Lock = IoReadAccess;
    Stack = RxContext->CurrentIrpSp;
    if (RxContext->MajorFunction == IRP_MJ_READ || RxContext->MajorFunction == IRP_MJ_WRITE)
    {
        if (!BooleanFlagOn(RxContext->MinorFunction, IRP_MN_MDL))
        {
            if (RxContext->MajorFunction == IRP_MJ_READ)
            {
                Lock = IoWriteAccess;
            }
            RxLockUserBuffer(RxContext, Lock, Stack->Parameters.Read.Length);
        }
    }
    else
    {
        if ((RxContext->MajorFunction == IRP_MJ_DIRECTORY_CONTROL && RxContext->MinorFunction == IRP_MN_QUERY_DIRECTORY) ||
            RxContext->MajorFunction == IRP_MJ_QUERY_EA)
        {
            Lock = IoWriteAccess;
            RxLockUserBuffer(RxContext, Lock, Stack->Parameters.QueryDirectory.Length);
        }
        else if (RxContext->MajorFunction == IRP_MJ_SET_EA)
        {
            RxLockUserBuffer(RxContext, Lock, Stack->Parameters.SetEa.Length);
        }
    }

    /* As it will be posted (async), mark the IRP pending */
    IoMarkIrpPending(Irp);
}

/*
 * @implemented
 */
NTSTATUS
RxpSetInfoMiniRdr(
    PRX_CONTEXT RxContext,
    FILE_INFORMATION_CLASS Class)
{
    PFCB Fcb;
    NTSTATUS Status;

    /* Initialize parameters in RX_CONTEXT */
    RxContext->Info.FileInformationClass = Class;
    RxContext->Info.Buffer = RxContext->CurrentIrp->AssociatedIrp.SystemBuffer;
    RxContext->Info.Length = RxContext->CurrentIrpSp->Parameters.SetFile.Length;

    /* And call mini-rdr */
    Fcb = (PFCB)RxContext->pFcb;
    MINIRDR_CALL(Status, RxContext, Fcb->MRxDispatch, MRxSetFileInfo, (RxContext));

    return Status;
}

VOID
NTAPI
RxpUnregisterMinirdr(
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
RxPurgeNetFcb(
    PFCB Fcb,
    PRX_CONTEXT LocalContext)
{
    NTSTATUS Status;

    PAGED_CODE();

    /* First, flush */
    MmFlushImageSection(&Fcb->NonPaged->SectionObjectPointers, MmFlushForWrite);

    /* And force close */
    RxReleaseFcb(NULL, Fcb);
    MmForceSectionClosed(&Fcb->NonPaged->SectionObjectPointers, TRUE);
    Status = RxAcquireExclusiveFcb(NULL, Fcb);
    ASSERT(Status == STATUS_SUCCESS);
}

NTSTATUS
RxQueryAlternateNameInfo(
    PRX_CONTEXT RxContext,
    PFILE_NAME_INFORMATION AltNameInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
RxQueryBasicInfo(
    PRX_CONTEXT RxContext,
    PFILE_BASIC_INFORMATION BasicInfo)
{
    PAGED_CODE();

    DPRINT("RxQueryBasicInfo(%p, %p)\n", RxContext, BasicInfo);

    /* Simply zero and forward to mini-rdr */
    RtlZeroMemory(BasicInfo, sizeof(FILE_BASIC_INFORMATION));
    return RxpQueryInfoMiniRdr(RxContext, FileBasicInformation, BasicInfo);
}

NTSTATUS
RxQueryCompressedInfo(
    PRX_CONTEXT RxContext,
    PFILE_COMPRESSION_INFORMATION CompressionInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
RxQueryDirectory(
    PRX_CONTEXT RxContext)
{
    PIRP Irp;
    PFCB Fcb;
    PFOBX Fobx;
    UCHAR Flags;
    NTSTATUS Status;
    BOOLEAN LockNotGranted;
    ULONG Length, FileIndex;
    PUNICODE_STRING FileName;
    PIO_STACK_LOCATION Stack;
    FILE_INFORMATION_CLASS FileInfoClass;

    PAGED_CODE();

    DPRINT("RxQueryDirectory(%p)\n", RxContext);

    /* Get parameters */
    Stack = RxContext->CurrentIrpSp;
    Length = Stack->Parameters.QueryDirectory.Length;
    FileName = Stack->Parameters.QueryDirectory.FileName;
    FileInfoClass = Stack->Parameters.QueryDirectory.FileInformationClass;
    DPRINT("Wait: %d, Length: %ld, FileName: %p, Class: %d\n",
           FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT), Length,
           FileName, FileInfoClass);

    Irp = RxContext->CurrentIrp;
    Flags = Stack->Flags;
    FileIndex = Stack->Parameters.QueryDirectory.FileIndex;
    DPRINT("Index: %d, Buffer: %p, Flags: %x\n", FileIndex, Irp->UserBuffer, Flags);

    if (FileName != NULL)
    {
        DPRINT("FileName: %wZ\n", FileName);
    }

    /* No FOBX: not a standard file/directory */
    Fobx = (PFOBX)RxContext->pFobx;
    if (Fobx == NULL)
    {
        return STATUS_OBJECT_NAME_INVALID;
    }

    /* We can only deal with a disk */
    Fcb = (PFCB)RxContext->pFcb;
    if (Fcb->pNetRoot->Type != NET_ROOT_DISK)
    {
        DPRINT1("Not a disk! %x\n", Fcb->pNetRoot->Type);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Setup RX_CONTEXT related fields */
    RxContext->QueryDirectory.FileIndex = FileIndex;
    RxContext->QueryDirectory.RestartScan = BooleanFlagOn(Flags, SL_RESTART_SCAN);
    RxContext->QueryDirectory.ReturnSingleEntry = BooleanFlagOn(Flags, SL_RETURN_SINGLE_ENTRY);
    RxContext->QueryDirectory.IndexSpecified = BooleanFlagOn(Flags, SL_INDEX_SPECIFIED);
    RxContext->QueryDirectory.InitialQuery = (Fobx->UnicodeQueryTemplate.Buffer == NULL) && !BooleanFlagOn(Fobx->Flags, FOBX_FLAG_MATCH_ALL);

    /* We don't support (yet?) a specific index being set */
    if (RxContext->QueryDirectory.IndexSpecified)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Try to lock FCB */
    LockNotGranted = TRUE;
    if (RxContext->QueryDirectory.InitialQuery)
    {
        Status = RxAcquireExclusiveFcb(RxContext, Fcb);
        if (Status != STATUS_LOCK_NOT_GRANTED)
        {
            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            if (Fobx->UnicodeQueryTemplate.Buffer != NULL)
            {
                RxContext->QueryDirectory.InitialQuery = FALSE;
                RxConvertToSharedFcb(RxContext, Fcb);
            }

            LockNotGranted = FALSE;
        }
    }
    else
    {
        Status = RxAcquireExclusiveFcb(RxContext, Fcb);
        if (Status != STATUS_LOCK_NOT_GRANTED)
        {
            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            LockNotGranted = FALSE;
        }
    }

    /* If it failed, post request */
    if (LockNotGranted)
    {
        return RxFsdPostRequest(RxContext);
    }

    /* This cannot be done on a orphaned directory */
    if (BooleanFlagOn(Fcb->FcbState, FCB_STATE_ORPHANED))
    {
        RxReleaseFcb(RxContext, Fcb);
        return STATUS_FILE_CLOSED;
    }

    _SEH2_TRY
    {
        /* Set index */
        if (!RxContext->QueryDirectory.IndexSpecified && RxContext->QueryDirectory.RestartScan)
        {
            RxContext->QueryDirectory.FileIndex = 0;
        }

        /* Assume success */
        Status = STATUS_SUCCESS;
        /* If initial query, prepare FOBX */
        if (RxContext->QueryDirectory.InitialQuery)
        {
            /* We cannot have a template already! */
            ASSERT(!BooleanFlagOn(Fobx->Flags, FOBX_FLAG_FREE_UNICODE));

            /* If we have a file name and a correct one, duplicate it in the FOBX */
            if (FileName != NULL && FileName->Length != 0 && FileName->Buffer != NULL &&
                (FileName->Length != sizeof(WCHAR) || FileName->Buffer[0] != '*') &&
                (FileName->Length != 12 * sizeof(WCHAR) ||
                 RtlCompareMemory(FileName->Buffer, Rx8QMdot3QM, 12 * sizeof(WCHAR)) != 12 * sizeof(WCHAR)))
            {
                Fobx->ContainsWildCards = FsRtlDoesNameContainWildCards(FileName);

                Fobx->UnicodeQueryTemplate.Buffer = RxAllocatePoolWithTag(PagedPool, FileName->Length, RX_DIRCTL_POOLTAG);
                if (Fobx->UnicodeQueryTemplate.Buffer != NULL)
                {
                    /* UNICODE_STRING; length has to be even */
                    if ((FileName->Length & 1) != 0)
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        RxFreePoolWithTag(Fobx->UnicodeQueryTemplate.Buffer, RX_DIRCTL_POOLTAG);
                    }
                    else
                    {
                        Fobx->UnicodeQueryTemplate.Length = FileName->Length;
                        Fobx->UnicodeQueryTemplate.MaximumLength = FileName->Length;
                        RtlMoveMemory(Fobx->UnicodeQueryTemplate.Buffer, FileName->Buffer, FileName->Length);

                        SetFlag(Fobx->Flags, FOBX_FLAG_FREE_UNICODE);
                    }
                }
                else
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            /* No name specified, or a match all wildcard? Match everything */
            else
            {
                Fobx->ContainsWildCards = TRUE;

                Fobx->UnicodeQueryTemplate.Buffer = &RxStarForTemplate;
                Fobx->UnicodeQueryTemplate.Length = sizeof(WCHAR);
                Fobx->UnicodeQueryTemplate.MaximumLength = sizeof(WCHAR);

                SetFlag(Fobx->Flags, FOBX_FLAG_MATCH_ALL);
            }

            /* No need for exclusive any longer */
            if (NT_SUCCESS(Status))
            {
                RxConvertToSharedFcb(RxContext, Fcb);
            }
        }

        /* Lock user buffer and forward to mini-rdr */
        if (NT_SUCCESS(Status))
        {
            RxLockUserBuffer(RxContext, IoModifyAccess, Length);
            RxContext->Info.FileInformationClass = FileInfoClass;
            RxContext->Info.Buffer = RxNewMapUserBuffer(RxContext);
            RxContext->Info.Length = Length;

            if (RxContext->Info.Buffer != NULL)
            {
                MINIRDR_CALL(Status, RxContext, Fcb->MRxDispatch, MRxQueryDirectory, (RxContext));
            }

            /* Post if mini-rdr asks to */
            if (RxContext->PostRequest)
            {
                RxFsdPostRequest(RxContext);
            }
            else
            {
                Irp->IoStatus.Information = Length - RxContext->Info.LengthRemaining;
            }
        }
    }
    _SEH2_FINALLY
    {
        RxReleaseFcb(RxContext, Fcb);
    }
    _SEH2_END;

    return Status;
}

NTSTATUS
RxQueryEaInfo(
    PRX_CONTEXT RxContext,
    PFILE_EA_INFORMATION EaInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RxQueryInternalInfo(
    PRX_CONTEXT RxContext,
    PFILE_INTERNAL_INFORMATION InternalInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
RxQueryNameInfo(
    PRX_CONTEXT RxContext,
    PFILE_NAME_INFORMATION NameInfo)
{
    PFCB Fcb;
    PFOBX Fobx;
    PAGED_CODE();

    DPRINT("RxQueryNameInfo(%p, %p)\n", RxContext, NameInfo);

    /* Check we can at least copy name size */
    if (RxContext->Info.LengthRemaining < FIELD_OFFSET(FILE_NAME_INFORMATION, FileName))
    {
        DPRINT1("Buffer too small: %d\n", RxContext->Info.LengthRemaining);
        RxContext->Info.Length = 0;
        return STATUS_BUFFER_OVERFLOW;
    }

    RxContext->Info.LengthRemaining -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName);

    Fcb = (PFCB)RxContext->pFcb;
    Fobx = (PFOBX)RxContext->pFobx;
    /* Get the UNC name */
    RxConjureOriginalName(Fcb, Fobx, &NameInfo->FileNameLength, &NameInfo->FileName[0],
                          &RxContext->Info.Length, VNetRoot_As_UNC_Name);

    /* If RxConjureOriginalName returned a negative len (-1) then output buffer
     * was too small, return the appropriate length & status.
     */
    if (RxContext->Info.LengthRemaining < 0)
    {
        DPRINT1("Buffer too small!\n");
        RxContext->Info.Length = 0;
        return STATUS_BUFFER_OVERFLOW;
    }

#if 1 // CORE-13938, rfb: please note I replaced 0 with 1 here
    if (NodeType(Fcb) == RDBSS_NTC_STORAGE_TYPE_DIRECTORY &&
        RxContext->Info.LengthRemaining >= sizeof(WCHAR))
    {
        NameInfo->FileName[NameInfo->FileNameLength / sizeof(WCHAR)] = L'\\';
        NameInfo->FileNameLength += sizeof(WCHAR);
        RxContext->Info.LengthRemaining -= sizeof(WCHAR);
    }
#endif

    /* All correct */
    return STATUS_SUCCESS;
}

NTSTATUS
RxQueryPipeInfo(
    PRX_CONTEXT RxContext,
    PFILE_PIPE_INFORMATION PipeInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RxQueryPositionInfo(
    PRX_CONTEXT RxContext,
    PFILE_POSITION_INFORMATION PositionInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
RxQueryStandardInfo(
    PRX_CONTEXT RxContext,
    PFILE_STANDARD_INFORMATION StandardInfo)
{
    PFCB Fcb;
    PFOBX Fobx;
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("RxQueryStandardInfo(%p, %p)\n", RxContext, StandardInfo);

    /* Zero output buffer */
    RtlZeroMemory(StandardInfo, sizeof(FILE_STANDARD_INFORMATION));

    Fcb = (PFCB)RxContext->pFcb;
    Fobx = (PFOBX)RxContext->pFobx;
    /* If not a standard file type, or opened for backup, immediately forward to mini-rdr */
    if ((NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_DIRECTORY && NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_FILE) ||
        BooleanFlagOn(Fobx->pSrvOpen->CreateOptions, FILE_OPEN_FOR_BACKUP_INTENT))
    {
        return RxpQueryInfoMiniRdr(RxContext, FileStandardInformation, StandardInfo);
    }

    /* Otherwise, fill what we can already */
    Status = STATUS_SUCCESS;
    StandardInfo->NumberOfLinks = Fcb->NumberOfLinks;
    StandardInfo->DeletePending = BooleanFlagOn(Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE);
    StandardInfo->Directory = (NodeType(Fcb) == RDBSS_NTC_STORAGE_TYPE_DIRECTORY);
    if (StandardInfo->NumberOfLinks == 0)
    {
        StandardInfo->NumberOfLinks = 1;
    }

    if (NodeType(Fcb) == RDBSS_NTC_STORAGE_TYPE_FILE)
    {
        StandardInfo->AllocationSize.QuadPart = Fcb->Header.AllocationSize.QuadPart;
        RxGetFileSizeWithLock(Fcb, &StandardInfo->EndOfFile.QuadPart);
    }

    /* If we are asked to forcefully forward to mini-rdr or if size isn't cached, do it */
    if (RxForceQFIPassThrough || !BooleanFlagOn(Fcb->FcbState, FCB_STATE_FILESIZECACHEING_ENABLED))
    {
        Status = RxpQueryInfoMiniRdr(RxContext, FileStandardInformation, StandardInfo);
    }
    else
    {
        RxContext->IoStatusBlock.Information -= sizeof(FILE_STANDARD_INFORMATION);
    }

    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
RxReadRegistryParameters(
    VOID)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    ULONG ResultLength;
    UCHAR Buffer[0x40];
    UNICODE_STRING KeyName, ParamName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo;

    PAGED_CODE();

    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\LanmanWorkStation\\Parameters");
    InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = ZwOpenKey(&KeyHandle, READ_CONTROL | KEY_NOTIFY | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    PartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
    RtlInitUnicodeString(&ParamName, L"DisableByteRangeLockingOnReadOnlyFiles");
    Status = ZwQueryValueKey(KeyHandle, &ParamName, KeyValuePartialInformation, PartialInfo, sizeof(Buffer), &ResultLength);
    if (NT_SUCCESS(Status) && PartialInfo->Type == REG_DWORD)
    {
        DisableByteRangeLockingOnReadOnlyFiles = (*(PULONG)PartialInfo->Data != 0);
    }

    RtlInitUnicodeString(&ParamName, L"ReadAheadGranularity");
    Status = ZwQueryValueKey(KeyHandle, &ParamName, KeyValuePartialInformation, PartialInfo, sizeof(Buffer), &ResultLength);
    if (NT_SUCCESS(Status) && PartialInfo->Type == REG_DWORD)
    {
        ULONG Granularity = *(PULONG)PartialInfo->Data;

        if (Granularity > 16)
        {
            Granularity = 16;
        }

        ReadAheadGranularity = Granularity << PAGE_SHIFT;
    }

    RtlInitUnicodeString(&ParamName, L"DisableFlushOnCleanup");
    Status = ZwQueryValueKey(KeyHandle, &ParamName, KeyValuePartialInformation, PartialInfo, sizeof(Buffer), &ResultLength);
    if (NT_SUCCESS(Status) && PartialInfo->Type == REG_DWORD)
    {
        DisableFlushOnCleanup = (*(PULONG)PartialInfo->Data != 0);
    }

    ZwClose(KeyHandle);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxRegisterMinirdr(
    OUT PRDBSS_DEVICE_OBJECT *DeviceObject,
    IN OUT PDRIVER_OBJECT DriverObject,
    IN PMINIRDR_DISPATCH MrdrDispatch,
    IN ULONG Controls,
    IN PUNICODE_STRING DeviceName,
    IN ULONG DeviceExtensionSize,
    IN DEVICE_TYPE DeviceType,
    IN ULONG DeviceCharacteristics)
{
    NTSTATUS Status;
    PRDBSS_DEVICE_OBJECT RDBSSDevice;

    PAGED_CODE();

    if (!DeviceObject)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Create device object with provided parameters */
    Status = IoCreateDevice(DriverObject,
                            DeviceExtensionSize + sizeof(RDBSS_DEVICE_OBJECT),
                            DeviceName,
                            DeviceType,
                            DeviceCharacteristics,
                            FALSE,
                            (PDEVICE_OBJECT *)&RDBSSDevice);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (!RxData.DriverObject)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Initialize our DO extension */
    RDBSSDevice->RDBSSDeviceObject = NULL;
    ++RxFileSystemDeviceObject->ReferenceCount;
    *DeviceObject = RDBSSDevice;
    RDBSSDevice->RdbssExports = &RxExports;
    RDBSSDevice->Dispatch = MrdrDispatch;
    RDBSSDevice->RegistrationControls = Controls;
    RDBSSDevice->DeviceName = *DeviceName;
    RDBSSDevice->RegisterUncProvider = !BooleanFlagOn(Controls, RX_REGISTERMINI_FLAG_DONT_PROVIDE_UNCS);
    RDBSSDevice->RegisterMailSlotProvider = !BooleanFlagOn(Controls, RX_REGISTERMINI_FLAG_DONT_PROVIDE_MAILSLOTS);
    InitializeListHead(&RDBSSDevice->OverflowQueue[0]);
    InitializeListHead(&RDBSSDevice->OverflowQueue[1]);
    InitializeListHead(&RDBSSDevice->OverflowQueue[2]);
    KeInitializeSpinLock(&RDBSSDevice->OverflowQueueSpinLock);
    RDBSSDevice->NetworkProviderPriority = RxGetNetworkProviderPriority(DeviceName);

    DPRINT("Registered MiniRdr %wZ (prio: %x)\n", DeviceName, RDBSSDevice->NetworkProviderPriority);

    ExAcquireFastMutex(&RxData.MinirdrRegistrationMutex);
    InsertTailList(&RxData.RegisteredMiniRdrs, &RDBSSDevice->MiniRdrListLinks);
    ExReleaseFastMutex(&RxData.MinirdrRegistrationMutex);

    /* Unless mini-rdr explicitly asked not to, initialize dispatch table */
    if (!BooleanFlagOn(Controls, RX_REGISTERMINI_FLAG_DONT_INIT_DRIVER_DISPATCH))
    {
        RxInitializeMinirdrDispatchTable(DriverObject);
    }

    /* Unless mini-rdr explicitly asked not to, initialize prefix scavenger */
    if (!BooleanFlagOn(Controls, RX_REGISTERMINI_FLAG_DONT_INIT_PREFIX_N_SCAVENGER))
    {
        LARGE_INTEGER ScavengerTimeLimit;

        RDBSSDevice->pRxNetNameTable = &RDBSSDevice->RxNetNameTableInDeviceObject;
        RxInitializePrefixTable(RDBSSDevice->pRxNetNameTable, 0, FALSE);
        RDBSSDevice->RxNetNameTableInDeviceObject.IsNetNameTable = TRUE;
        ScavengerTimeLimit.QuadPart = MrdrDispatch->ScavengerTimeout * 10000000LL;
        RDBSSDevice->pRdbssScavenger = &RDBSSDevice->RdbssScavengerInDeviceObject;
        RxInitializeRdbssScavenger(RDBSSDevice->pRdbssScavenger, ScavengerTimeLimit);
    }

    RDBSSDevice->pAsynchronousRequestsCompletionEvent = NULL;

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
RxRemoveFromTopLevelIrpAllocatedContextsList(
    PRX_TOPLEVELIRP_CONTEXT TopLevelContext)
{
    KIRQL OldIrql;

    /* Make sure this is a TLC and that it was allocated (otherwise, it is not in the list */
    ASSERT(TopLevelContext->Signature == RX_TOPLEVELIRP_CONTEXT_SIGNATURE);
    ASSERT(BooleanFlagOn(TopLevelContext->Flags, RX_TOPLEVELCTX_FLAG_FROM_POOL));

    KeAcquireSpinLock(&TopLevelIrpSpinLock, &OldIrql);
    RemoveEntryList(&TopLevelContext->ListEntry);
    KeReleaseSpinLock(&TopLevelIrpSpinLock, OldIrql);
}

/*
 * @implemented
 */
PRX_CONTEXT
RxRemoveOverflowEntry(
    PRDBSS_DEVICE_OBJECT DeviceObject,
    WORK_QUEUE_TYPE Queue)
{
    KIRQL OldIrql;
    PRX_CONTEXT Context;

    KeAcquireSpinLock(&DeviceObject->OverflowQueueSpinLock, &OldIrql);
    if (DeviceObject->OverflowQueueCount[Queue] <= 0)
    {
        /* No entries left, nothing to return */
        InterlockedDecrement(&DeviceObject->PostedRequestCount[Queue]);
        Context = NULL;
    }
    else
    {
        PLIST_ENTRY Entry;

        /* Decrement count */
        --DeviceObject->OverflowQueueCount[Queue];

        /* Return head */
        Entry = RemoveHeadList(&DeviceObject->OverflowQueue[Queue]);
        Context = CONTAINING_RECORD(Entry, RX_CONTEXT, OverflowListEntry);
        ClearFlag(Context->Flags, (RX_CONTEXT_FLAG_FSP_DELAYED_OVERFLOW_QUEUE | RX_CONTEXT_FLAG_FSP_CRITICAL_OVERFLOW_QUEUE));
        Context->OverflowListEntry.Flink = NULL;
    }
    KeReleaseSpinLock(&DeviceObject->OverflowQueueSpinLock, OldIrql);

    return Context;
}

#if DBG
/*
 * @implemented
 */
VOID
RxRemoveShareAccess(
    _Inout_ PFILE_OBJECT FileObject,
    _Inout_ PSHARE_ACCESS ShareAccess,
    _In_ PSZ where,
    _In_ PSZ wherelogtag)
{
    PAGED_CODE();

    RxDumpCurrentAccess(where, "before", wherelogtag, ShareAccess);
    IoRemoveShareAccess(FileObject, ShareAccess);
    RxDumpCurrentAccess(where, "after", wherelogtag, ShareAccess);
}
#endif

/*
 * @implemented
 */
VOID
RxRemoveShareAccessPerSrvOpens(
    IN OUT PSRV_OPEN SrvOpen)
{
    ACCESS_MASK DesiredAccess;
    BOOLEAN ReadAccess;
    BOOLEAN WriteAccess;
    BOOLEAN DeleteAccess;

    PAGED_CODE();

    /* Get access that were granted to SRV_OPEN */
    DesiredAccess = SrvOpen->DesiredAccess;
    ReadAccess = (DesiredAccess & (FILE_READ_DATA | FILE_EXECUTE)) != 0;
    WriteAccess = (DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) != 0;
    DeleteAccess = (DesiredAccess & DELETE) != 0;

    /* If any, drop them */
    if ((ReadAccess) || (WriteAccess) || (DeleteAccess))
    {
        BOOLEAN SharedRead;
        BOOLEAN SharedWrite;
        BOOLEAN SharedDelete;
        ULONG DesiredShareAccess;
        PSHARE_ACCESS ShareAccess;

        ShareAccess = &((PFCB)SrvOpen->pFcb)->ShareAccessPerSrvOpens;
        DesiredShareAccess = SrvOpen->ShareAccess;

        ShareAccess->Readers -= ReadAccess;
        ShareAccess->Writers -= WriteAccess;
        ShareAccess->Deleters -= DeleteAccess;

        ShareAccess->OpenCount--;

        SharedRead = (DesiredShareAccess & FILE_SHARE_READ) != 0;
        SharedWrite = (DesiredShareAccess & FILE_SHARE_WRITE) != 0;
        SharedDelete = (DesiredShareAccess & FILE_SHARE_DELETE) != 0;
        ShareAccess->SharedRead -= SharedRead;
        ShareAccess->SharedWrite -= SharedWrite;
        ShareAccess->SharedDelete -= SharedDelete;
    }
}

NTSTATUS
RxSearchForCollapsibleOpen(
    PRX_CONTEXT RxContext,
    ACCESS_MASK DesiredAccess,
    ULONG ShareAccess)
{
    PFCB Fcb;
    NTSTATUS Status;
    PLIST_ENTRY ListEntry;
    BOOLEAN ShouldTry, Purged, Scavenged;

    PAGED_CODE();

    DPRINT("RxSearchForCollapsibleOpen(%p, %x, %x)\n", RxContext, DesiredAccess, ShareAccess);

    Fcb = (PFCB)RxContext->pFcb;

    /* If we're asked to open for backup, don't allow SRV_OPEN reuse */
    if (BooleanFlagOn(RxContext->Create.NtCreateParameters.CreateOptions, FILE_OPEN_FOR_BACKUP_INTENT))
    {
        ClearFlag(Fcb->FcbState, FCB_STATE_COLLAPSING_ENABLED);

        RxScavengeRelatedFobxs(Fcb);
        RxPurgeFcbInSystemCache(Fcb, NULL, 0, FALSE, TRUE);

        return STATUS_NOT_FOUND;
    }

    /* If basic open, ask the mini-rdr if we should try to collapse */
    if (RxContext->Create.NtCreateParameters.Disposition == FILE_OPEN ||
        RxContext->Create.NtCreateParameters.Disposition == FILE_OPEN_IF)
    {
        ShouldTry = TRUE;

        if (Fcb->MRxDispatch != NULL)
        {
            ASSERT(RxContext->pRelevantSrvOpen == NULL);
            ASSERT(Fcb->MRxDispatch->MRxShouldTryToCollapseThisOpen != NULL);

            ShouldTry = NT_SUCCESS(Fcb->MRxDispatch->MRxShouldTryToCollapseThisOpen(RxContext));
        }
    }
    else
    {
        ShouldTry = FALSE;
    }

    if (BooleanFlagOn(RxContext->Create.NtCreateParameters.CreateOptions, FILE_DELETE_ON_CLOSE))
    {
        ShouldTry = FALSE;
    }

    /* If we shouldn't try, ask the caller to allocate a new SRV_OPEN */
    if (!ShouldTry)
    {
        if (NT_SUCCESS(RxCheckShareAccessPerSrvOpens(Fcb, DesiredAccess, ShareAccess)))
        {
            return STATUS_NOT_FOUND;
        }

        ClearFlag(Fcb->FcbState, FCB_STATE_COLLAPSING_ENABLED);

        RxScavengeRelatedFobxs(Fcb);
        RxPurgeFcbInSystemCache(Fcb, NULL, 0, FALSE, TRUE);

        return STATUS_NOT_FOUND;
    }

    /* Only collapse for matching NET_ROOT & disks */
    if (Fcb->pNetRoot != RxContext->Create.pNetRoot ||
        Fcb->pNetRoot->Type != NET_ROOT_DISK)
    {
        return STATUS_NOT_FOUND;
    }

    Purged = FALSE;
    Scavenged = FALSE;
    Status = STATUS_NOT_FOUND;
TryAgain:
    /* Browse all our SRV_OPEN to find the matching one */
    for (ListEntry = Fcb->SrvOpenList.Flink;
         ListEntry != &Fcb->SrvOpenList;
         ListEntry = ListEntry->Flink)
    {
        PSRV_OPEN SrvOpen;

        SrvOpen = CONTAINING_RECORD(ListEntry, SRV_OPEN, SrvOpenQLinks);
        /* Not the same VNET_ROOT, move to the next one */
        if (SrvOpen->pVNetRoot != RxContext->Create.pVNetRoot)
        {
            RxContext->Create.TryForScavengingOnSharingViolation = TRUE;
            continue;
        }

        /* Is there a sharing violation? */
        if (SrvOpen->DesiredAccess != DesiredAccess || SrvOpen->ShareAccess != ShareAccess ||
            BooleanFlagOn(SrvOpen->Flags, (SRVOPEN_FLAG_CLOSED | SRVOPEN_FLAG_COLLAPSING_DISABLED | SRVOPEN_FLAG_FILE_DELETED | SRVOPEN_FLAG_FILE_RENAMED)))
        {
            if (SrvOpen->pVNetRoot != RxContext->Create.pVNetRoot)
            {
                RxContext->Create.TryForScavengingOnSharingViolation = TRUE;
                continue;
            }

            /* Check against the SRV_OPEN */
            Status = RxCheckShareAccessPerSrvOpens(Fcb, DesiredAccess, ShareAccess);
            if (!NT_SUCCESS(Status))
            {
                break;
            }
        }
        else
        {
            /* Don't allow collaspse for reparse point opening */
            if (BooleanFlagOn(RxContext->Create.NtCreateParameters.CreateOptions ^ SrvOpen->CreateOptions, FILE_OPEN_REPARSE_POINT))
            {
                Purged = TRUE;
                Scavenged = TRUE;
                Status = STATUS_NOT_FOUND;
                break;
            }

            /* Not readonly? Or bytereange lock disabled? Try to collapse! */
            if (DisableByteRangeLockingOnReadOnlyFiles || !BooleanFlagOn(SrvOpen->pFcb->Attributes, FILE_ATTRIBUTE_READONLY))
            {
                RxContext->pRelevantSrvOpen = (PMRX_SRV_OPEN)SrvOpen;

                ASSERT(Fcb->MRxDispatch->MRxShouldTryToCollapseThisOpen != NULL);
                if (NT_SUCCESS(Fcb->MRxDispatch->MRxShouldTryToCollapseThisOpen(RxContext)))
                {
                    /* Is close delayed - great reuse*/
                    if (BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_CLOSE_DELAYED))
                    {
                        DPRINT("Delayed close successfull, reusing %p\n", SrvOpen);
                        InterlockedDecrement(&((PSRV_CALL)Fcb->pNetRoot->pSrvCall)->NumberOfCloseDelayedFiles);
                        ClearFlag(SrvOpen->Flags, SRVOPEN_FLAG_CLOSE_DELAYED);
                    }

                    return STATUS_SUCCESS;
                }

                Status = STATUS_NOT_FOUND;
                break;
            }
        }
    }
    /* We browse the whole list and didn't find any matching? NOT_FOUND */
    if (ListEntry == &Fcb->SrvOpenList)
    {
        Status = STATUS_NOT_FOUND;
    }

    /* Only required access: read attributes? Don't reuse */
    if ((DesiredAccess & 0xFFEFFFFF) == FILE_READ_ATTRIBUTES)
    {
        return STATUS_NOT_FOUND;
    }

    /* Not found? Scavenge and retry to look for collaspile SRV_OPEN */
    if (!Scavenged)
    {
        ClearFlag(Fcb->FcbState, FCB_STATE_COLLAPSING_ENABLED);
        Scavenged = TRUE;
        RxScavengeRelatedFobxs(Fcb);
        goto TryAgain;
    }

    /* Not found? Purgeable? Purge and retry to look for collaspile SRV_OPEN */
    if (!Purged && RxIsOkToPurgeFcb(Fcb))
    {
        RxPurgeFcbInSystemCache(Fcb, NULL, 0, FALSE, TRUE);
        Purged = TRUE;
        goto TryAgain;
    }

    /* If sharing violation, keep track of it */
    if (Status == STATUS_SHARING_VIOLATION)
    {
        RxContext->Create.TryForScavengingOnSharingViolation = TRUE;
    }

    DPRINT("Status: %x\n", Status);
    return Status;
}

NTSTATUS
RxSetAllocationInfo(
    PRX_CONTEXT RxContext)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
RxSetBasicInfo(
    PRX_CONTEXT RxContext)
{
    NTSTATUS Status;

    PAGED_CODE();

#define FILE_ATTRIBUTE_VOLUME 0x8
#define VALID_FILE_ATTRIBUTES (                                   \
    FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN |             \
    FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_VOLUME |               \
    FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_DEVICE |              \
    FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_SPARSE_FILE |       \
    FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_COMPRESSED |    \
    FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | \
    FILE_ATTRIBUTE_ENCRYPTED | FILE_ATTRIBUTE_INTEGRITY_STREAM)
#define VALID_DIR_ATTRIBUTES (VALID_FILE_ATTRIBUTES | FILE_ATTRIBUTE_DIRECTORY)

    /* First of all, call the mini-rdr */
    Status = RxpSetInfoMiniRdr(RxContext, FileBasicInformation);
    /* If it succeed, perform last bits */
    if (NT_SUCCESS(Status))
    {
        PIRP Irp;
        PFCB Fcb;
        PFOBX Fobx;
        PFILE_OBJECT FileObject;
        ULONG Attributes, CleanAttr;
        PFILE_BASIC_INFORMATION BasicInfo;

        Fcb = (PFCB)RxContext->pFcb;
        Fobx = (PFOBX)RxContext->pFobx;
        Irp = RxContext->CurrentIrp;
        BasicInfo = Irp->AssociatedIrp.SystemBuffer;
        FileObject = RxContext->CurrentIrpSp->FileObject;

        /* If caller provided flags, handle the change */
        Attributes = BasicInfo->FileAttributes;
        if (Attributes != 0)
        {
            /* Clean our flags first, with only stuff we support */
            if (NodeType(Fcb) == RDBSS_NTC_STORAGE_TYPE_DIRECTORY)
            {
                CleanAttr = (Attributes & VALID_DIR_ATTRIBUTES) | FILE_ATTRIBUTE_DIRECTORY;
            }
            else
            {
                CleanAttr = Attributes & VALID_FILE_ATTRIBUTES;
            }

            /* Handle the temporary mark (set/unset depending on caller) */
            if (BooleanFlagOn(Attributes, FILE_ATTRIBUTE_TEMPORARY))
            {
                SetFlag(Fcb->FcbState, FCB_STATE_TEMPORARY);
                SetFlag(FileObject->Flags, FO_TEMPORARY_FILE);
            }
            else
            {
                ClearFlag(Fcb->FcbState, FCB_STATE_TEMPORARY);
                ClearFlag(FileObject->Flags, FO_TEMPORARY_FILE);
            }

            /* And set new attributes */
            Fcb->Attributes = CleanAttr;
        }

        /* If caller provided a creation time, set it */
        if (BasicInfo->CreationTime.QuadPart != 0LL)
        {
            Fcb->CreationTime.QuadPart = BasicInfo->CreationTime.QuadPart;
            SetFlag(Fobx->Flags, FOBX_FLAG_USER_SET_CREATION);
        }

        /* If caller provided a last access time, set it */
        if (BasicInfo->LastAccessTime.QuadPart != 0LL)
        {
            Fcb->LastAccessTime.QuadPart = BasicInfo->LastAccessTime.QuadPart;
            SetFlag(Fobx->Flags, FOBX_FLAG_USER_SET_LAST_ACCESS);
        }

        /* If caller provided a last write time, set it */
        if (BasicInfo->LastWriteTime.QuadPart != 0LL)
        {
            Fcb->LastWriteTime.QuadPart = BasicInfo->LastWriteTime.QuadPart;
            SetFlag(Fobx->Flags, FOBX_FLAG_USER_SET_LAST_WRITE);
        }

        /* If caller provided a last change time, set it */
        if (BasicInfo->ChangeTime.QuadPart != 0LL)
        {
            Fcb->LastChangeTime.QuadPart = BasicInfo->ChangeTime.QuadPart;
            SetFlag(Fobx->Flags, FOBX_FLAG_USER_SET_LAST_CHANGE);
        }
    }

    /* Done */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
RxSetDispositionInfo(
    PRX_CONTEXT RxContext)
{
    NTSTATUS Status;

    PAGED_CODE();

    /* First, make the mini-rdr work! */
    Status = RxpSetInfoMiniRdr(RxContext, FileDispositionInformation);
    /* If it succeed, we'll keep track of the change */
    if (NT_SUCCESS(Status))
    {
        PFCB Fcb;
        PFILE_OBJECT FileObject;
        PFILE_DISPOSITION_INFORMATION FileDispo;

        Fcb = (PFCB)RxContext->pFcb;
        FileObject = RxContext->CurrentIrpSp->FileObject;
        FileDispo = RxContext->CurrentIrp->AssociatedIrp.SystemBuffer;
        /* Caller asks for deletion: mark as delete on close */
        if (FileDispo->DeleteFile)
        {
            SetFlag(Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE);
            FileObject->DeletePending = TRUE;
        }
        /* Otherwise, clear it */
        else
        {
            ClearFlag(Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE);
            FileObject->DeletePending = FALSE;
        }

        /* Sanitize output */
        Status = STATUS_SUCCESS;
    }

    return Status;
}

NTSTATUS
RxSetEndOfFileInfo(
    PRX_CONTEXT RxContext)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RxSetPipeInfo(
    PRX_CONTEXT RxContext)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RxSetPositionInfo(
    PRX_CONTEXT RxContext)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
RxSetRenameInfo(
    PRX_CONTEXT RxContext)
{
    ULONG Length;
    NTSTATUS Status;
    PFCB RenameFcb, Fcb;
    PIO_STACK_LOCATION Stack;
    PFILE_RENAME_INFORMATION RenameInfo, UserInfo;

    PAGED_CODE();

    DPRINT("RxSetRenameInfo(%p)\n", RxContext);

    Stack = RxContext->CurrentIrpSp;
    DPRINT("FO: %p, Replace: %d\n", Stack->Parameters.SetFile.FileObject, Stack->Parameters.SetFile.ReplaceIfExists);

    /* If there's no FO, we won't do extra operation, so directly pass to mini-rdr and quit */
    RxContext->Info.ReplaceIfExists = Stack->Parameters.SetFile.ReplaceIfExists;
    if (Stack->Parameters.SetFile.FileObject == NULL)
    {
        return RxpSetInfoMiniRdr(RxContext, Stack->Parameters.SetFile.FileInformationClass);
    }

    Fcb = (PFCB)RxContext->pFcb;
    RenameFcb = Stack->Parameters.SetFile.FileObject->FsContext;
    /* First, validate the received file object */
    ASSERT(NodeType(RenameFcb) == RDBSS_NTC_OPENTARGETDIR_FCB);
    if (Fcb->pNetRoot != RenameFcb->pNetRoot)
    {
        DPRINT1("Not the same device: %p:%p (%wZ) - %p:%p (%wZ)\n", Fcb, Fcb->pNetRoot, Fcb->pNetRoot->pNetRootName, RenameFcb, RenameFcb->pNetRoot, RenameFcb->pNetRoot->pNetRootName);
        return STATUS_NOT_SAME_DEVICE;
    }

    /* We'll reallocate a safe buffer */
    Length = Fcb->pNetRoot->DiskParameters.RenameInfoOverallocationSize + RenameFcb->FcbTableEntry.Path.Length + FIELD_OFFSET(FILE_RENAME_INFORMATION, FileName);
    RenameInfo = RxAllocatePoolWithTag(PagedPool, Length, '??xR');
    if (RenameInfo == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH2_TRY
    {
        /* Copy the data */
        UserInfo = RxContext->CurrentIrp->AssociatedIrp.SystemBuffer;
        RenameInfo->ReplaceIfExists = UserInfo->ReplaceIfExists;
        RenameInfo->RootDirectory = UserInfo->RootDirectory;
        RenameInfo->FileNameLength = RenameFcb->FcbTableEntry.Path.Length;
        RtlMoveMemory(&RenameInfo->FileName[0], RenameFcb->FcbTableEntry.Path.Buffer, RenameFcb->FcbTableEntry.Path.Length);

        /* Set them in the RX_CONTEXT */
        RxContext->Info.FileInformationClass = Stack->Parameters.SetFile.FileInformationClass;
        RxContext->Info.Buffer = RenameInfo;
        RxContext->Info.Length = Length;

        /* And call the mini-rdr */
        MINIRDR_CALL(Status, RxContext, Fcb->MRxDispatch, MRxSetFileInfo, (RxContext));
    }
    _SEH2_FINALLY
    {
        /* Free */
        RxFreePoolWithTag(RenameInfo, '??xR');
    }
    _SEH2_END;

    /* Done! */
    return Status;
}

#if DBG
/*
 * @implemented
 */
VOID
RxSetShareAccess(
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG DesiredShareAccess,
    _Inout_ PFILE_OBJECT FileObject,
    _Out_ PSHARE_ACCESS ShareAccess,
    _In_ PSZ where,
    _In_ PSZ wherelogtag)
{
    PAGED_CODE();

    RxDumpCurrentAccess(where, "before", wherelogtag, ShareAccess);
    IoSetShareAccess(DesiredAccess, DesiredShareAccess, FileObject, ShareAccess);
    RxDumpCurrentAccess(where, "after", wherelogtag, ShareAccess);
}
#endif

NTSTATUS
RxSetSimpleInfo(
    PRX_CONTEXT RxContext)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
VOID
RxSetupNetFileObject(
    PRX_CONTEXT RxContext)
{
    PFCB Fcb;
    PFOBX Fobx;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;

    PAGED_CODE();

    /* Assert FOBX is FOBX or NULL */
    Fobx = (PFOBX)RxContext->pFobx;
    ASSERT((Fobx == NULL) || (NodeType(Fobx) == RDBSS_NTC_FOBX));

    Fcb = (PFCB)RxContext->pFcb;
    Stack = RxContext->CurrentIrpSp;
    FileObject = Stack->FileObject;
    /* If it's temporary mark FO as such */
    if (Fcb != NULL && NodeType(Fcb) != RDBSS_NTC_VCB &&
        BooleanFlagOn(Fcb->FcbState, FCB_STATE_TEMPORARY))
    {
        if (FileObject == NULL)
        {
            return;
        }

        FileObject->Flags |= FO_TEMPORARY_FILE;
    }

    /* No FO, nothing to setup */
    if (FileObject == NULL)
    {
        return;
    }

    /* Assign FCB & CCB (FOBX) to FO */
    FileObject->FsContext = Fcb;
    FileObject->FsContext2 = Fobx;
    if (Fobx != NULL)
    {
        ULONG_PTR StackTop, StackBottom;

        /* If FO is allocated on pool, keep track of it */
        IoGetStackLimits(&StackTop, &StackBottom);
        if ((ULONG_PTR)FileObject <= StackBottom || (ULONG_PTR)FileObject >= StackTop)
        {
            Fobx->AssociatedFileObject = FileObject;
        }
        else
        {
            Fobx->AssociatedFileObject = NULL;
        }

        /* Make sure to mark FOBX if it's a DFS open */
        if (RxContext->Create.NtCreateParameters.DfsContext == UIntToPtr(DFS_OPEN_CONTEXT))
        {
            SetFlag(Fobx->Flags, FOBX_FLAG_DFS_OPEN);
        }
        else
        {
            ClearFlag(Fobx->Flags, FOBX_FLAG_DFS_OPEN);
        }
    }

    /* Set Cc pointers */
    FileObject->SectionObjectPointer = &Fcb->NonPaged->SectionObjectPointers;

    /* Update access state */
    if (Stack->Parameters.Create.SecurityContext != NULL)
    {
        PACCESS_STATE AccessState;

        AccessState = Stack->Parameters.Create.SecurityContext->AccessState;
        AccessState->PreviouslyGrantedAccess |= AccessState->RemainingDesiredAccess;
        AccessState->RemainingDesiredAccess = 0;
    }
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxStartMinirdr(
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp)
{
    NTSTATUS Status;
    BOOLEAN Wait, AlreadyStarted;
    PRDBSS_DEVICE_OBJECT DeviceObject;

    /* If we've not been post, then, do it */
    if (!BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP))
    {
        SECURITY_SUBJECT_CONTEXT SubjectContext;

        SeCaptureSubjectContext(&SubjectContext);
        RxContext->FsdUid = RxGetUid(&SubjectContext);
        SeReleaseSubjectContext(&SubjectContext);

        *PostToFsp = TRUE;
        return STATUS_PENDING;
    }

    /* Acquire all the required locks */
    Wait = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    if (!ExAcquireResourceExclusiveLite(&RxData.Resource, Wait))
    {
        *PostToFsp = TRUE;
        return STATUS_PENDING;
    }

    if (!RxAcquirePrefixTableLockExclusive(RxContext->RxDeviceObject->pRxNetNameTable, Wait))
    {
        ExReleaseResourceLite(&RxData.Resource);
        *PostToFsp = TRUE;
        return STATUS_PENDING;
    }

    AlreadyStarted = FALSE;
    DeviceObject = RxContext->RxDeviceObject;
    _SEH2_TRY
    {
        /* MUP handle set, means already registered */
        if (DeviceObject->MupHandle != NULL)
        {
            AlreadyStarted = TRUE;
            Status = STATUS_REDIRECTOR_STARTED;
            _SEH2_LEAVE;
        }

        /* If we're asked to register to MUP, then do it */
        Status = STATUS_SUCCESS;
        if (DeviceObject->RegisterUncProvider)
        {
            Status = FsRtlRegisterUncProvider(&DeviceObject->MupHandle,
                                              &DeviceObject->DeviceName,
                                              DeviceObject->RegisterMailSlotProvider);
        }
        if (!NT_SUCCESS(Status))
        {
            DeviceObject->MupHandle = NULL;
            _SEH2_LEAVE;
        }

        /* Register as file system */
        IoRegisterFileSystem(&DeviceObject->DeviceObject);
        DeviceObject->RegisteredAsFileSystem = TRUE;

        /* Inform mini-rdr it has to start */
        MINIRDR_CALL(Status, RxContext, DeviceObject->Dispatch, MRxStart, (RxContext, DeviceObject));
        if (NT_SUCCESS(Status))
        {
            ++DeviceObject->StartStopContext.Version;
            RxSetRdbssState(DeviceObject, RDBSS_STARTED);
            InterlockedExchangeAdd(&RxData.NumberOfMinirdrsStarted, 1);

            Status = RxInitializeMRxDispatcher(DeviceObject);
        }
    }
    _SEH2_FINALLY
    {
        if (_SEH2_AbnormalTermination() || !NT_SUCCESS(Status))
        {
            if (!AlreadyStarted)
            {
                RxUnstart(RxContext, DeviceObject);
            }
        }

        RxReleasePrefixTableLock(RxContext->RxDeviceObject->pRxNetNameTable);
        ExReleaseResourceLite(&RxData.Resource);
    }
    _SEH2_END;

    return Status;
}

NTSTATUS
NTAPI
RxStopMinirdr(
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RxSystemControl(
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
BOOLEAN
RxTryToBecomeTheTopLevelIrp(
    IN OUT PRX_TOPLEVELIRP_CONTEXT TopLevelContext,
    IN PIRP Irp,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN BOOLEAN ForceTopLevel
    )
{
    BOOLEAN FromPool = FALSE;

    PAGED_CODE();

    /* If not top level, and not have to be, quit */
    if (IoGetTopLevelIrp() && !ForceTopLevel)
    {
        return FALSE;
    }

    /* If not TLC provider, allocate one */
    if (TopLevelContext == NULL)
    {
        TopLevelContext = RxAllocatePoolWithTag(NonPagedPool, sizeof(RX_TOPLEVELIRP_CONTEXT), RX_TLC_POOLTAG);
        if (TopLevelContext == NULL)
        {
            return FALSE;
        }

        FromPool = TRUE;
    }

    /* Init it */
    __RxInitializeTopLevelIrpContext(TopLevelContext, Irp, RxDeviceObject, FromPool);

    ASSERT(TopLevelContext->Signature == RX_TOPLEVELIRP_CONTEXT_SIGNATURE);
    if (FromPool)
    {
        ASSERT(BooleanFlagOn(TopLevelContext->Flags, RX_TOPLEVELCTX_FLAG_FROM_POOL));
    }

    /* Make it top level IRP */
    IoSetTopLevelIrp((PIRP)TopLevelContext);
    return TRUE;
}

#if DBG
/*
 * @implemented
 */
VOID
RxUpdateShareAccess(
    _Inout_ PFILE_OBJECT FileObject,
    _Inout_ PSHARE_ACCESS ShareAccess,
    _In_ PSZ where,
    _In_ PSZ wherelogtag)
{
    PAGED_CODE();

    RxDumpCurrentAccess(where, "before", wherelogtag, ShareAccess);
    IoUpdateShareAccess(FileObject, ShareAccess);
    RxDumpCurrentAccess(where, "after", wherelogtag, ShareAccess);
}
#endif

/*
 * @implemented
 */
VOID
RxUninitializeCacheMap(
    PRX_CONTEXT RxContext,
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER TruncateSize)
{
    PFCB Fcb;
    NTSTATUS Status;
    CACHE_UNINITIALIZE_EVENT UninitEvent;

    PAGED_CODE();

    Fcb = FileObject->FsContext;
    ASSERT(NodeTypeIsFcb(Fcb));
    ASSERT(RxIsFcbAcquiredExclusive(Fcb));

    KeInitializeEvent(&UninitEvent.Event, SynchronizationEvent, FALSE);
    CcUninitializeCacheMap(FileObject, TruncateSize, &UninitEvent);

    /* Always release the FCB before waiting for the uninit event */
    RxReleaseFcb(RxContext, Fcb);

    KeWaitForSingleObject(&UninitEvent.Event, Executive, KernelMode, FALSE, NULL);

    /* Re-acquire it afterwards */
    Status = RxAcquireExclusiveFcb(RxContext, Fcb);
    ASSERT(NT_SUCCESS(Status));
}

VOID
NTAPI
RxUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
RxUnlockOperation(
    IN PVOID Context,
    IN PFILE_LOCK_INFO LockInfo)
{
    UNIMPLEMENTED;
}

VOID
RxUnstart(
    PRX_CONTEXT Context,
    PRDBSS_DEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
RxUnwindTopLevelIrp(
    IN OUT PRX_TOPLEVELIRP_CONTEXT TopLevelContext)
{
    DPRINT("RxUnwindTopLevelIrp(%p)\n", TopLevelContext);

    /* No TLC provided? Ask the system for ours! */
    if (TopLevelContext == NULL)
    {
        TopLevelContext = (PRX_TOPLEVELIRP_CONTEXT)IoGetTopLevelIrp();
        if (TopLevelContext == NULL)
        {
            return;
        }

        /* In that case, just assert it's really ours */
        ASSERT(RxIsThisAnRdbssTopLevelContext(TopLevelContext));
        ASSERT(BooleanFlagOn(TopLevelContext->Flags, RX_TOPLEVELCTX_FLAG_FROM_POOL));
    }

    ASSERT(TopLevelContext->Signature == RX_TOPLEVELIRP_CONTEXT_SIGNATURE);
    ASSERT(TopLevelContext->Thread == PsGetCurrentThread());
    /* Restore the previous top level IRP */
    IoSetTopLevelIrp(TopLevelContext->Previous);
    /* If TLC was allocated from pool, remove it from list and release it */
    if (BooleanFlagOn(TopLevelContext->Flags, RX_TOPLEVELCTX_FLAG_FROM_POOL))
    {
        RxRemoveFromTopLevelIrpAllocatedContextsList(TopLevelContext);
        RxFreePoolWithTag(TopLevelContext, RX_TLC_POOLTAG);
    }
}

/*
 * @implemented
 */
VOID
RxUpdateShareAccessPerSrvOpens(
    IN PSRV_OPEN SrvOpen)
{
    ACCESS_MASK DesiredAccess;
    BOOLEAN ReadAccess;
    BOOLEAN WriteAccess;
    BOOLEAN DeleteAccess;

    PAGED_CODE();

    /* If already updated, no need to continue */
    if (BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_SHAREACCESS_UPDATED))
    {
        return;
    }

    /* Check if any access wanted */
    DesiredAccess = SrvOpen->DesiredAccess;
    ReadAccess = (DesiredAccess & (FILE_READ_DATA | FILE_EXECUTE)) != 0;
    WriteAccess = (DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) != 0;
    DeleteAccess = (DesiredAccess & DELETE) != 0;

    /* In that case, update it */
    if ((ReadAccess) || (WriteAccess) || (DeleteAccess))
    {
        BOOLEAN SharedRead;
        BOOLEAN SharedWrite;
        BOOLEAN SharedDelete;
        ULONG DesiredShareAccess;
        PSHARE_ACCESS ShareAccess;

        ShareAccess = &((PFCB)SrvOpen->pFcb)->ShareAccessPerSrvOpens;
        DesiredShareAccess = SrvOpen->ShareAccess;

        SharedRead = (DesiredShareAccess & FILE_SHARE_READ) != 0;
        SharedWrite = (DesiredShareAccess & FILE_SHARE_WRITE) != 0;
        SharedDelete = (DesiredShareAccess & FILE_SHARE_DELETE) != 0;

        ShareAccess->OpenCount++;

        ShareAccess->Readers += ReadAccess;
        ShareAccess->Writers += WriteAccess;
        ShareAccess->Deleters += DeleteAccess;
        ShareAccess->SharedRead += SharedRead;
        ShareAccess->SharedWrite += SharedWrite;
        ShareAccess->SharedDelete += SharedDelete;
    }

    SetFlag(SrvOpen->Flags, SRVOPEN_FLAG_SHAREACCESS_UPDATED);
}

/*
 * @implemented
 */
NTSTATUS
RxXXXControlFileCallthru(
    PRX_CONTEXT Context)
{
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("RxXXXControlFileCallthru(%p)\n", Context);

    /* No dispatch table? Nothing to dispatch */
    if (Context->RxDeviceObject->Dispatch == NULL)
    {
        Context->pFobx = NULL;
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Init the lowio context */
    Status = RxLowIoPopulateFsctlInfo(Context);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Check whether we're consistent: a length means a buffer */
    if ((Context->LowIoContext.ParamsFor.FsCtl.InputBufferLength > 0 && Context->LowIoContext.ParamsFor.FsCtl.pInputBuffer == NULL) ||
        (Context->LowIoContext.ParamsFor.FsCtl.OutputBufferLength > 0 && Context->LowIoContext.ParamsFor.FsCtl.pOutputBuffer == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Forward the call to the mini-rdr */
    DPRINT("Calling: %p\n", Context->RxDeviceObject->Dispatch->MRxDevFcbXXXControlFile);
    Status = Context->RxDeviceObject->Dispatch->MRxDevFcbXXXControlFile(Context);
    if (Status != STATUS_PENDING)
    {
        Context->CurrentIrp->IoStatus.Information = Context->InformationToReturn;
    }

    DPRINT("RxXXXControlFileCallthru: %x, %ld\n", Context->CurrentIrp->IoStatus.Status, Context->CurrentIrp->IoStatus.Information);
    return Status;
}
