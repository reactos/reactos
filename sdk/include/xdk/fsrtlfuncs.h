$if (_NTIFS_)
/* FSRTL Functions */

#define FsRtlEnterFileSystem    KeEnterCriticalRegion
#define FsRtlExitFileSystem     KeLeaveCriticalRegion

#if (NTDDI_VERSION >= NTDDI_WIN2K)

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCopyRead(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ BOOLEAN Wait,
  _In_ ULONG LockKey,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _Out_ PIO_STATUS_BLOCK IoStatus,
  _In_ PDEVICE_OBJECT DeviceObject);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCopyWrite(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ BOOLEAN Wait,
  _In_ ULONG LockKey,
  _In_reads_bytes_(Length) PVOID Buffer,
  _Out_ PIO_STATUS_BLOCK IoStatus,
  _In_ PDEVICE_OBJECT DeviceObject);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlReadDev(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ ULONG LockKey,
  _Outptr_ PMDL *MdlChain,
  _Out_ PIO_STATUS_BLOCK IoStatus,
  _In_opt_ PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlReadCompleteDev(
  _In_ PFILE_OBJECT FileObject,
  _In_ PMDL MdlChain,
  _In_opt_ PDEVICE_OBJECT DeviceObject);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlPrepareMdlWriteDev(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ ULONG LockKey,
  _Outptr_ PMDL *MdlChain,
  _Out_ PIO_STATUS_BLOCK IoStatus,
  _In_ PDEVICE_OBJECT DeviceObject);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlWriteCompleteDev(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ PMDL MdlChain,
  _In_opt_ PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlAcquireFileExclusive(
  _In_ PFILE_OBJECT FileObject);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlReleaseFile(
  _In_ PFILE_OBJECT FileObject);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlGetFileSize(
  _In_ PFILE_OBJECT FileObject,
  _Out_ PLARGE_INTEGER FileSize);

_Must_inspect_result_
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsTotalDeviceFailure(
  _In_ NTSTATUS Status);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PFILE_LOCK
NTAPI
FsRtlAllocateFileLock(
  _In_opt_ PCOMPLETE_LOCK_IRP_ROUTINE CompleteLockIrpRoutine,
  _In_opt_ PUNLOCK_ROUTINE UnlockRoutine);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlFreeFileLock(
  _In_ PFILE_LOCK FileLock);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlInitializeFileLock(
  _Out_ PFILE_LOCK FileLock,
  _In_opt_ PCOMPLETE_LOCK_IRP_ROUTINE CompleteLockIrpRoutine,
  _In_opt_ PUNLOCK_ROUTINE UnlockRoutine);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeFileLock(
  _Inout_ PFILE_LOCK FileLock);

/*
  FsRtlProcessFileLock:

  ret:
    -STATUS_INVALID_DEVICE_REQUEST
    -STATUS_RANGE_NOT_LOCKED from unlock routines.
    -STATUS_PENDING, STATUS_LOCK_NOT_GRANTED from FsRtlPrivateLock
    (redirected IoStatus->Status).

  Internals:
    -switch ( Irp->CurrentStackLocation->MinorFunction )
        lock: return FsRtlPrivateLock;
        unlocksingle: return FsRtlFastUnlockSingle;
        unlockall: return FsRtlFastUnlockAll;
        unlockallbykey: return FsRtlFastUnlockAllByKey;
        default: IofCompleteRequest with STATUS_INVALID_DEVICE_REQUEST;
                 return STATUS_INVALID_DEVICE_REQUEST;

    -'AllwaysZero' is passed thru as 'AllwaysZero' to lock / unlock routines.
    -'Irp' is passet thru as 'Irp' to FsRtlPrivateLock.
*/
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlProcessFileLock(
  _In_ PFILE_LOCK FileLock,
  _In_ PIRP Irp,
  _In_opt_ PVOID Context);

/*
  FsRtlCheckLockForReadAccess:

  All this really does is pick out the lock parameters from the irp (io stack
  location?), get IoGetRequestorProcess, and pass values on to
  FsRtlFastCheckLockForRead.
*/
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCheckLockForReadAccess(
  _In_ PFILE_LOCK FileLock,
  _In_ PIRP Irp);

/*
  FsRtlCheckLockForWriteAccess:

  All this really does is pick out the lock parameters from the irp (io stack
  location?), get IoGetRequestorProcess, and pass values on to
  FsRtlFastCheckLockForWrite.
*/
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCheckLockForWriteAccess(
  _In_ PFILE_LOCK FileLock,
  _In_ PIRP Irp);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlFastCheckLockForRead(
  _In_ PFILE_LOCK FileLock,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ PLARGE_INTEGER Length,
  _In_ ULONG Key,
  _In_ PFILE_OBJECT FileObject,
  _In_ PVOID Process);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlFastCheckLockForWrite(
  _In_ PFILE_LOCK FileLock,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ PLARGE_INTEGER Length,
  _In_ ULONG Key,
  _In_ PFILE_OBJECT FileObject,
  _In_ PVOID Process);

/*
  FsRtlGetNextFileLock:

  ret: NULL if no more locks

  Internals:
    FsRtlGetNextFileLock uses FileLock->LastReturnedLockInfo and
    FileLock->LastReturnedLock as storage.
    LastReturnedLock is a pointer to the 'raw' lock inkl. double linked
    list, and FsRtlGetNextFileLock needs this to get next lock on subsequent
    calls with Restart = FALSE.
*/
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PFILE_LOCK_INFO
NTAPI
FsRtlGetNextFileLock(
  _In_ PFILE_LOCK FileLock,
  _In_ BOOLEAN Restart);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlFastUnlockSingle(
  _In_ PFILE_LOCK FileLock,
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ PLARGE_INTEGER Length,
  _In_ PEPROCESS Process,
  _In_ ULONG Key,
  _In_opt_ PVOID Context,
  _In_ BOOLEAN AlreadySynchronized);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlFastUnlockAll(
  _In_ PFILE_LOCK FileLock,
  _In_ PFILE_OBJECT FileObject,
  _In_ PEPROCESS Process,
  _In_opt_ PVOID Context);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlFastUnlockAllByKey(
  _In_ PFILE_LOCK FileLock,
  _In_ PFILE_OBJECT FileObject,
  _In_ PEPROCESS Process,
  _In_ ULONG Key,
  _In_opt_ PVOID Context);

/*
  FsRtlPrivateLock:

  ret: IoStatus->Status: STATUS_PENDING, STATUS_LOCK_NOT_GRANTED

  Internals:
    -Calls IoCompleteRequest if Irp
    -Uses exception handling / ExRaiseStatus with STATUS_INSUFFICIENT_RESOURCES
*/
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
__drv_preferredFunction(FsRtlFastLock, "Obsolete")
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlPrivateLock(
  _In_ PFILE_LOCK FileLock,
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ PLARGE_INTEGER Length,
  _In_ PEPROCESS Process,
  _In_ ULONG Key,
  _In_ BOOLEAN FailImmediately,
  _In_ BOOLEAN ExclusiveLock,
  _Out_ PIO_STATUS_BLOCK IoStatus,
  _In_opt_ PIRP Irp,
  _In_opt_ __drv_aliasesMem PVOID Context,
  _In_ BOOLEAN AlreadySynchronized);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlInitializeTunnelCache(
  _In_ PTUNNEL Cache);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlAddToTunnelCache(
  _In_ PTUNNEL Cache,
  _In_ ULONGLONG DirectoryKey,
  _In_ PUNICODE_STRING ShortName,
  _In_ PUNICODE_STRING LongName,
  _In_ BOOLEAN KeyByShortName,
  _In_ ULONG DataLength,
  _In_reads_bytes_(DataLength) PVOID Data);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlFindInTunnelCache(
  _In_ PTUNNEL Cache,
  _In_ ULONGLONG DirectoryKey,
  _In_ PUNICODE_STRING Name,
  _Out_ PUNICODE_STRING ShortName,
  _Out_ PUNICODE_STRING LongName,
  _Inout_ PULONG DataLength,
  _Out_writes_bytes_to_(*DataLength, *DataLength) PVOID Data);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlDeleteKeyFromTunnelCache(
  _In_ PTUNNEL Cache,
  _In_ ULONGLONG DirectoryKey);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlDeleteTunnelCache(
  _In_ PTUNNEL Cache);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlDissectDbcs(
  _In_ ANSI_STRING Name,
  _Out_ PANSI_STRING FirstPart,
  _Out_ PANSI_STRING RemainingPart);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlDoesDbcsContainWildCards(
  _In_ PANSI_STRING Name);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsDbcsInExpression(
  _In_ PANSI_STRING Expression,
  _In_ PANSI_STRING Name);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsFatDbcsLegal(
  _In_ ANSI_STRING DbcsName,
  _In_ BOOLEAN WildCardsPermissible,
  _In_ BOOLEAN PathNamePermissible,
  _In_ BOOLEAN LeadingBackslashPermissible);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsHpfsDbcsLegal(
  _In_ ANSI_STRING DbcsName,
  _In_ BOOLEAN WildCardsPermissible,
  _In_ BOOLEAN PathNamePermissible,
  _In_ BOOLEAN LeadingBackslashPermissible);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlNormalizeNtstatus(
  _In_ NTSTATUS Exception,
  _In_ NTSTATUS GenericException);

_Must_inspect_result_
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsNtstatusExpected(
  _In_ NTSTATUS Ntstatus);

_IRQL_requires_max_(APC_LEVEL)
__drv_preferredFunction(ExAllocateFromNPagedLookasideList, "The FsRtlAllocateResource routine is obsolete, but is exported to support existing driver binaries. Use ExAllocateFromNPagedLookasideList and ExInitializeResourceLite instead.")
NTKERNELAPI
PERESOURCE
NTAPI
FsRtlAllocateResource(VOID);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlInitializeLargeMcb(
  _Out_ PLARGE_MCB Mcb,
  _In_ POOL_TYPE PoolType);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeLargeMcb(
  _Inout_ PLARGE_MCB Mcb);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlResetLargeMcb(
  _Inout_ PLARGE_MCB Mcb,
  _In_ BOOLEAN SelfSynchronized);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlTruncateLargeMcb(
  _Inout_ PLARGE_MCB Mcb,
  _In_ LONGLONG Vbn);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAddLargeMcbEntry(
  _Inout_ PLARGE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _In_ LONGLONG Lbn,
  _In_ LONGLONG SectorCount);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlRemoveLargeMcbEntry(
  _Inout_ PLARGE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _In_ LONGLONG SectorCount);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLargeMcbEntry(
  _In_ PLARGE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _Out_opt_ PLONGLONG Lbn,
  _Out_opt_ PLONGLONG SectorCountFromLbn,
  _Out_opt_ PLONGLONG StartingLbn,
  _Out_opt_ PLONGLONG SectorCountFromStartingLbn,
  _Out_opt_ PULONG Index);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLastLargeMcbEntry(
  _In_ PLARGE_MCB Mcb,
  _Out_ PLONGLONG Vbn,
  _Out_ PLONGLONG Lbn);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLastLargeMcbEntryAndIndex(
  _In_ PLARGE_MCB OpaqueMcb,
  _Out_ PLONGLONG LargeVbn,
  _Out_ PLONGLONG LargeLbn,
  _Out_ PULONG Index);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
ULONG
NTAPI
FsRtlNumberOfRunsInLargeMcb(
  _In_ PLARGE_MCB Mcb);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlGetNextLargeMcbEntry(
  _In_ PLARGE_MCB Mcb,
  _In_ ULONG RunIndex,
  _Out_ PLONGLONG Vbn,
  _Out_ PLONGLONG Lbn,
  _Out_ PLONGLONG SectorCount);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlSplitLargeMcb(
  _Inout_ PLARGE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _In_ LONGLONG Amount);

_IRQL_requires_max_(APC_LEVEL)
__drv_preferredFunction(FsRtlInitializeLargeMcb, "Obsolete")
NTKERNELAPI
VOID
NTAPI
FsRtlInitializeMcb(
  _Out_ PMCB Mcb,
  _In_ POOL_TYPE PoolType);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeMcb(
  _Inout_ PMCB Mcb);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlTruncateMcb(
  _Inout_ PMCB Mcb,
  _In_ VBN Vbn);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAddMcbEntry(
  _Inout_ PMCB Mcb,
  _In_ VBN Vbn,
  _In_ LBN Lbn,
  _In_ ULONG SectorCount);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlRemoveMcbEntry(
  _Inout_ PMCB Mcb,
  _In_ VBN Vbn,
  _In_ ULONG SectorCount);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupMcbEntry(
  _In_ PMCB Mcb,
  _In_ VBN Vbn,
  _Out_ PLBN Lbn,
  _Out_opt_ PULONG SectorCount,
  _Out_ PULONG Index);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLastMcbEntry(
  _In_ PMCB Mcb,
  _Out_ PVBN Vbn,
  _Out_ PLBN Lbn);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
ULONG
NTAPI
FsRtlNumberOfRunsInMcb(
  _In_ PMCB Mcb);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlGetNextMcbEntry(
  _In_ PMCB Mcb,
  _In_ ULONG RunIndex,
  _Out_ PVBN Vbn,
  _Out_ PLBN Lbn,
  _Out_ PULONG SectorCount);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlBalanceReads(
  _In_ PDEVICE_OBJECT TargetDevice);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlInitializeOplock(
  _Inout_ POPLOCK Oplock);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeOplock(
  _Inout_ POPLOCK Oplock);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlOplockFsctrl(
  _In_ POPLOCK Oplock,
  _In_ PIRP Irp,
  _In_ ULONG OpenCount);

_When_(CompletionRoutine != NULL, _Must_inspect_result_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlCheckOplock(
  _In_ POPLOCK Oplock,
  _In_ PIRP Irp,
  _In_opt_ PVOID Context,
  _In_opt_ POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine,
  _In_opt_ POPLOCK_FS_PREPOST_IRP PostIrpRoutine);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlOplockIsFastIoPossible(
  _In_ POPLOCK Oplock);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCurrentBatchOplock(
  _In_ POPLOCK Oplock);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlNotifyVolumeEvent(
  _In_ PFILE_OBJECT FileObject,
  _In_ ULONG EventCode);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlNotifyInitializeSync(
  _In_ PNOTIFY_SYNC *NotifySync);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlNotifyUninitializeSync(
  _In_ PNOTIFY_SYNC *NotifySync);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlNotifyFullChangeDirectory(
  _In_ PNOTIFY_SYNC NotifySync,
  _In_ PLIST_ENTRY NotifyList,
  _In_ PVOID FsContext,
  _In_ PSTRING FullDirectoryName,
  _In_ BOOLEAN WatchTree,
  _In_ BOOLEAN IgnoreBuffer,
  _In_ ULONG CompletionFilter,
  _In_opt_ PIRP NotifyIrp,
  _In_opt_ PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback,
  _In_opt_ PSECURITY_SUBJECT_CONTEXT SubjectContext);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlNotifyFilterReportChange(
  _In_ PNOTIFY_SYNC NotifySync,
  _In_ PLIST_ENTRY NotifyList,
  _In_ PSTRING FullTargetName,
  _In_ USHORT TargetNameOffset,
  _In_opt_ PSTRING StreamName,
  _In_opt_ PSTRING NormalizedParentName,
  _In_ ULONG FilterMatch,
  _In_ ULONG Action,
  _In_opt_ PVOID TargetContext,
  _In_opt_ PVOID FilterContext);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlNotifyFullReportChange(
  _In_ PNOTIFY_SYNC NotifySync,
  _In_ PLIST_ENTRY NotifyList,
  _In_ PSTRING FullTargetName,
  _In_ USHORT TargetNameOffset,
  _In_opt_ PSTRING StreamName,
  _In_opt_ PSTRING NormalizedParentName,
  _In_ ULONG FilterMatch,
  _In_ ULONG Action,
  _In_opt_ PVOID TargetContext);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlNotifyCleanup(
  _In_ PNOTIFY_SYNC NotifySync,
  _In_ PLIST_ENTRY NotifyList,
  _In_ PVOID FsContext);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlDissectName(
  _In_ UNICODE_STRING Name,
  _Out_ PUNICODE_STRING FirstPart,
  _Out_ PUNICODE_STRING RemainingPart);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlDoesNameContainWildCards(
  _In_ PUNICODE_STRING Name);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAreNamesEqual(
  _In_ PCUNICODE_STRING Name1,
  _In_ PCUNICODE_STRING Name2,
  _In_ BOOLEAN IgnoreCase,
  _In_reads_opt_(0x10000) PCWCH UpcaseTable);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsNameInExpression(
  _In_ PUNICODE_STRING Expression,
  _In_ PUNICODE_STRING Name,
  _In_ BOOLEAN IgnoreCase,
  _In_opt_ PWCHAR UpcaseTable);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlPostPagingFileStackOverflow(
  _In_ PVOID Context,
  _In_ PKEVENT Event,
  _In_ PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlPostStackOverflow (
  _In_ PVOID Context,
  _In_ PKEVENT Event,
  _In_ PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlRegisterUncProvider(
  _Out_ PHANDLE MupHandle,
  _In_ PCUNICODE_STRING RedirectorDeviceName,
  _In_ BOOLEAN MailslotsSupported);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlDeregisterUncProvider(
  _In_ HANDLE Handle);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlTeardownPerStreamContexts(
  _In_ PFSRTL_ADVANCED_FCB_HEADER AdvancedHeader);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlCreateSectionForDataScan(
  _Out_ PHANDLE SectionHandle,
  _Outptr_ PVOID *SectionObject,
  _Out_opt_ PLARGE_INTEGER SectionFileSize,
  _In_ PFILE_OBJECT FileObject,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ PLARGE_INTEGER MaximumSize,
  _In_ ULONG SectionPageProtection,
  _In_ ULONG AllocationAttributes,
  _In_ ULONG Flags);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlNotifyFilterChangeDirectory(
  _In_ PNOTIFY_SYNC NotifySync,
  _In_ PLIST_ENTRY NotifyList,
  _In_ PVOID FsContext,
  _In_ PSTRING FullDirectoryName,
  _In_ BOOLEAN WatchTree,
  _In_ BOOLEAN IgnoreBuffer,
  _In_ ULONG CompletionFilter,
  _In_opt_ PIRP NotifyIrp,
  _In_opt_ PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback,
  _In_opt_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
  _In_opt_ PFILTER_REPORT_CHANGE FilterCallback);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlInsertPerStreamContext(
  _In_ PFSRTL_ADVANCED_FCB_HEADER PerStreamContext,
  _In_ PFSRTL_PER_STREAM_CONTEXT Ptr);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PFSRTL_PER_STREAM_CONTEXT
NTAPI
FsRtlLookupPerStreamContextInternal(
  _In_ PFSRTL_ADVANCED_FCB_HEADER StreamContext,
  _In_opt_ PVOID OwnerId,
  _In_opt_ PVOID InstanceId);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PFSRTL_PER_STREAM_CONTEXT
NTAPI
FsRtlRemovePerStreamContext(
  _In_ PFSRTL_ADVANCED_FCB_HEADER StreamContext,
  _In_opt_ PVOID OwnerId,
  _In_opt_ PVOID InstanceId);

NTKERNELAPI
VOID
NTAPI
FsRtlIncrementCcFastReadNotPossible(
  VOID);

NTKERNELAPI
VOID
NTAPI
FsRtlIncrementCcFastReadWait(VOID);

NTKERNELAPI
VOID
NTAPI
FsRtlIncrementCcFastReadNoWait(VOID);

NTKERNELAPI
VOID
NTAPI
FsRtlIncrementCcFastReadResourceMiss(VOID);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
LOGICAL
NTAPI
FsRtlIsPagingFile(
  _In_ PFILE_OBJECT FileObject);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_WS03)

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlInitializeBaseMcb(
  _Out_ PBASE_MCB Mcb,
  _In_ POOL_TYPE PoolType);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeBaseMcb(
  _In_ PBASE_MCB Mcb);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlResetBaseMcb(
  _Out_ PBASE_MCB Mcb);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlTruncateBaseMcb(
  _Inout_ PBASE_MCB Mcb,
  _In_ LONGLONG Vbn);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAddBaseMcbEntry(
  _Inout_ PBASE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _In_ LONGLONG Lbn,
  _In_ LONGLONG SectorCount);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlRemoveBaseMcbEntry(
  _Inout_ PBASE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _In_ LONGLONG SectorCount);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupBaseMcbEntry(
  _In_ PBASE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _Out_opt_ PLONGLONG Lbn,
  _Out_opt_ PLONGLONG SectorCountFromLbn,
  _Out_opt_ PLONGLONG StartingLbn,
  _Out_opt_ PLONGLONG SectorCountFromStartingLbn,
  _Out_opt_ PULONG Index);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLastBaseMcbEntry(
  _In_ PBASE_MCB Mcb,
  _Out_ PLONGLONG Vbn,
  _Out_ PLONGLONG Lbn);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLastBaseMcbEntryAndIndex(
  _In_ PBASE_MCB OpaqueMcb,
  _Inout_ PLONGLONG LargeVbn,
  _Inout_ PLONGLONG LargeLbn,
  _Inout_ PULONG Index);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
ULONG
NTAPI
FsRtlNumberOfRunsInBaseMcb(
  _In_ PBASE_MCB Mcb);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlGetNextBaseMcbEntry(
  _In_ PBASE_MCB Mcb,
  _In_ ULONG RunIndex,
  _Out_ PLONGLONG Vbn,
  _Out_ PLONGLONG Lbn,
  _Out_ PLONGLONG SectorCount);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlSplitBaseMcb(
  _Inout_ PBASE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _In_ LONGLONG Amount);

#endif /* (NTDDI_VERSION >= NTDDI_WS03) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

_When_(!Flags & MCB_FLAG_RAISE_ON_ALLOCATION_FAILURE, _Must_inspect_result_)
_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
NTAPI
FsRtlInitializeBaseMcbEx(
  _Out_ PBASE_MCB Mcb,
  _In_ POOL_TYPE PoolType,
  _In_ USHORT Flags);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NTAPI
FsRtlAddBaseMcbEntryEx(
  _Inout_ PBASE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _In_ LONGLONG Lbn,
  _In_ LONGLONG SectorCount);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCurrentOplock(
  _In_ POPLOCK Oplock);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlOplockBreakToNone(
  _Inout_ POPLOCK Oplock,
  _In_opt_ PIO_STACK_LOCATION IrpSp,
  _In_ PIRP Irp,
  _In_opt_ PVOID Context,
  _In_opt_ POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine,
  _In_opt_ POPLOCK_FS_PREPOST_IRP PostIrpRoutine);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlNotifyVolumeEventEx(
  _In_ PFILE_OBJECT FileObject,
  _In_ ULONG EventCode,
  _In_ PTARGET_DEVICE_CUSTOM_NOTIFICATION Event);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlNotifyCleanupAll(
  _In_ PNOTIFY_SYNC NotifySync,
  _In_ PLIST_ENTRY NotifyList);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
FsRtlRegisterUncProviderEx(
  _Out_ PHANDLE MupHandle,
  _In_ PUNICODE_STRING RedirDevName,
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ ULONG Flags);

_Must_inspect_result_
_When_(Irp!=NULL, _IRQL_requires_max_(PASSIVE_LEVEL))
_When_(Irp==NULL, _IRQL_requires_max_(APC_LEVEL))
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlCancellableWaitForSingleObject(
  _In_ PVOID Object,
  _In_opt_ PLARGE_INTEGER Timeout,
  _In_opt_ PIRP Irp);

_Must_inspect_result_
_When_(Irp != NULL, _IRQL_requires_max_(PASSIVE_LEVEL))
_When_(Irp == NULL, _IRQL_requires_max_(APC_LEVEL))
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlCancellableWaitForMultipleObjects(
  _In_ ULONG Count,
  _In_reads_(Count) PVOID ObjectArray[],
  _In_ WAIT_TYPE WaitType,
  _In_opt_ PLARGE_INTEGER Timeout,
  _In_opt_ PKWAIT_BLOCK WaitBlockArray,
  _In_opt_ PIRP Irp);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlMupGetProviderInfoFromFileObject(
  _In_ PFILE_OBJECT pFileObject,
  _In_ ULONG Level,
  _Out_writes_bytes_(*pBufferSize) PVOID pBuffer,
  _Inout_ PULONG pBufferSize);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlMupGetProviderIdFromName(
  _In_ PUNICODE_STRING pProviderName,
  _Out_ PULONG32 pProviderId);

NTKERNELAPI
VOID
NTAPI
FsRtlIncrementCcFastMdlReadWait(VOID);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlValidateReparsePointBuffer(
  _In_ ULONG BufferLength,
  _In_reads_bytes_(BufferLength) PREPARSE_DATA_BUFFER ReparseBuffer);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlRemoveDotsFromPath(
  _Inout_updates_bytes_(PathLength) PWSTR OriginalString,
  _In_ USHORT PathLength,
  _Out_ USHORT *NewLength);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlAllocateExtraCreateParameterList(
  _In_ FSRTL_ALLOCATE_ECPLIST_FLAGS Flags,
  _Outptr_ PECP_LIST *EcpList);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlFreeExtraCreateParameterList(
  _In_ PECP_LIST EcpList);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlAllocateExtraCreateParameter(
  _In_ LPCGUID EcpType,
  _In_ ULONG SizeOfContext,
  _In_ FSRTL_ALLOCATE_ECP_FLAGS Flags,
  _In_opt_ PFSRTL_EXTRA_CREATE_PARAMETER_CLEANUP_CALLBACK CleanupCallback,
  _In_ ULONG PoolTag,
  _Outptr_result_bytebuffer_(SizeOfContext) PVOID *EcpContext);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlFreeExtraCreateParameter(
  _In_ PVOID EcpContext);

_When_(Flags|FSRTL_ECP_LOOKASIDE_FLAG_NONPAGED_POOL, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(!(Flags|FSRTL_ECP_LOOKASIDE_FLAG_NONPAGED_POOL), _IRQL_requires_max_(APC_LEVEL))
NTKERNELAPI
VOID
NTAPI
FsRtlInitExtraCreateParameterLookasideList(
  _Inout_ PVOID Lookaside,
  _In_ FSRTL_ECP_LOOKASIDE_FLAGS Flags,
  _In_ SIZE_T Size,
  _In_ ULONG Tag);

_When_(Flags|FSRTL_ECP_LOOKASIDE_FLAG_NONPAGED_POOL, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(!(Flags|FSRTL_ECP_LOOKASIDE_FLAG_NONPAGED_POOL), _IRQL_requires_max_(APC_LEVEL))
VOID
NTAPI
FsRtlDeleteExtraCreateParameterLookasideList(
  _Inout_ PVOID Lookaside,
  _In_ FSRTL_ECP_LOOKASIDE_FLAGS Flags);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlAllocateExtraCreateParameterFromLookasideList(
  _In_ LPCGUID EcpType,
  ULONG SizeOfContext,
  _In_ FSRTL_ALLOCATE_ECP_FLAGS Flags,
  _In_opt_ PFSRTL_EXTRA_CREATE_PARAMETER_CLEANUP_CALLBACK CleanupCallback,
  _Inout_ PVOID LookasideList,
  _Outptr_ PVOID *EcpContext);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlInsertExtraCreateParameter(
  _Inout_ PECP_LIST EcpList,
  _Inout_ PVOID EcpContext);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlFindExtraCreateParameter(
  _In_ PECP_LIST EcpList,
  _In_ LPCGUID EcpType,
  _Outptr_opt_ PVOID *EcpContext,
  _Out_opt_ ULONG *EcpContextSize);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlRemoveExtraCreateParameter(
  _Inout_ PECP_LIST EcpList,
  _In_ LPCGUID EcpType,
  _Outptr_ PVOID *EcpContext,
  _Out_opt_ ULONG *EcpContextSize);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlGetEcpListFromIrp(
  _In_ PIRP Irp,
  _Outptr_result_maybenull_ PECP_LIST *EcpList);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlSetEcpListIntoIrp(
  _Inout_ PIRP Irp,
  _In_ PECP_LIST EcpList);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlGetNextExtraCreateParameter(
  _In_ PECP_LIST EcpList,
  _In_opt_ PVOID CurrentEcpContext,
  _Out_opt_ LPGUID NextEcpType,
  _Outptr_opt_ PVOID *NextEcpContext,
  _Out_opt_ ULONG *NextEcpContextSize);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlAcknowledgeEcp(
  _In_ PVOID EcpContext);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsEcpAcknowledged(
  _In_ PVOID EcpContext);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsEcpFromUserMode(
  _In_ PVOID EcpContext);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlChangeBackingFileObject(
  _In_opt_ PFILE_OBJECT CurrentFileObject,
  _In_ PFILE_OBJECT NewFileObject,
  _In_ FSRTL_CHANGE_BACKING_TYPE ChangeBackingType,
  _In_ ULONG Flags);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlLogCcFlushError(
  _In_ PUNICODE_STRING FileName,
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_ NTSTATUS FlushError,
  _In_ ULONG Flags);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAreVolumeStartupApplicationsComplete(VOID);

NTKERNELAPI
ULONG
NTAPI
FsRtlQueryMaximumVirtualDiskNestingLevel(VOID);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlGetVirtualDiskNestingLevel(
  _In_ PDEVICE_OBJECT DeviceObject,
  _Out_ PULONG NestingLevel,
  _Out_opt_ PULONG NestingFlags);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (NTDDI_VERSION >= NTDDI_VISTASP1)
_When_(Flags | OPLOCK_FLAG_BACK_OUT_ATOMIC_OPLOCK, _Must_inspect_result_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlCheckOplockEx(
  _In_ POPLOCK Oplock,
  _In_ PIRP Irp,
  _In_ ULONG Flags,
  _In_opt_ PVOID Context,
  _In_opt_ POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine,
  _In_opt_ POPLOCK_FS_PREPOST_IRP PostIrpRoutine);

#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAreThereCurrentOrInProgressFileLocks(
  _In_ PFILE_LOCK FileLock);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlOplockIsSharedRequest(
  _In_ PIRP Irp);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlOplockBreakH(
  _In_ POPLOCK Oplock,
  _In_ PIRP Irp,
  _In_ ULONG Flags,
  _In_opt_ PVOID Context,
  _In_opt_ POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine,
  _In_opt_ POPLOCK_FS_PREPOST_IRP PostIrpRoutine);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCurrentOplockH(
  _In_ POPLOCK Oplock);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlOplockBreakToNoneEx(
  _Inout_ POPLOCK Oplock,
  _In_ PIRP Irp,
  _In_ ULONG Flags,
  _In_opt_ PVOID Context,
  _In_opt_ POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine,
  _In_opt_ POPLOCK_FS_PREPOST_IRP PostIrpRoutine);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlOplockFsctrlEx(
  _In_ POPLOCK Oplock,
  _In_ PIRP Irp,
  _In_ ULONG OpenCount,
  _In_ ULONG Flags);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlOplockKeysEqual(
  _In_opt_ PFILE_OBJECT Fo1,
  _In_opt_ PFILE_OBJECT Fo2);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlInitializeExtraCreateParameterList(
  _Inout_ PECP_LIST EcpList);

NTKERNELAPI
VOID
NTAPI
FsRtlInitializeExtraCreateParameter(
  _Out_ PECP_HEADER Ecp,
  _In_ ULONG EcpFlags,
  _In_opt_ PFSRTL_EXTRA_CREATE_PARAMETER_CLEANUP_CALLBACK CleanupCallback,
  _In_ ULONG TotalSize,
  _In_ LPCGUID EcpType,
  _In_opt_ PVOID ListAllocatedFrom);

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlInsertPerFileContext(
  _In_ PVOID* PerFileContextPointer,
  _In_ PFSRTL_PER_FILE_CONTEXT Ptr);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PFSRTL_PER_FILE_CONTEXT
NTAPI
FsRtlLookupPerFileContext(
  _In_ PVOID* PerFileContextPointer,
  _In_opt_ PVOID OwnerId,
  _In_opt_ PVOID InstanceId);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PFSRTL_PER_FILE_CONTEXT
NTAPI
FsRtlRemovePerFileContext(
  _In_ PVOID* PerFileContextPointer,
  _In_opt_ PVOID OwnerId,
  _In_opt_ PVOID InstanceId);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlTeardownPerFileContexts(
  _In_ PVOID* PerFileContextPointer);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlInsertPerFileObjectContext(
  _In_ PFILE_OBJECT FileObject,
  _In_ PFSRTL_PER_FILEOBJECT_CONTEXT Ptr);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PFSRTL_PER_FILEOBJECT_CONTEXT
NTAPI
FsRtlLookupPerFileObjectContext(
  _In_ PFILE_OBJECT FileObject,
  _In_opt_ PVOID OwnerId,
  _In_opt_ PVOID InstanceId);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PFSRTL_PER_FILEOBJECT_CONTEXT
NTAPI
FsRtlRemovePerFileObjectContext(
  _In_ PFILE_OBJECT FileObject,
  _In_opt_ PVOID OwnerId,
  _In_opt_ PVOID InstanceId);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlRegisterFileSystemFilterCallbacks(
  _In_ struct _DRIVER_OBJECT *FilterDriverObject,
  _In_ PFS_FILTER_CALLBACKS Callbacks);

#if (NTDDI_VERSION >= NTDDI_VISTA)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlNotifyStreamFileObject(
  _In_ struct _FILE_OBJECT * StreamFileObject,
  _In_opt_ struct _DEVICE_OBJECT *DeviceObjectHint,
  _In_ FS_FILTER_STREAM_FO_NOTIFICATION_TYPE NotificationType,
  _In_ BOOLEAN SafeToRecurse);
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#define FsRtlFastLock(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11)            \
     FsRtlPrivateLock(A1, A2, A3, A4, A5, A6, A7, A8, A9, NULL, A10, A11)

#define FsRtlAreThereCurrentFileLocks(FL)                                      \
    ((FL)->FastIoIsQuestionable)

#define FsRtlIncrementLockRequestsInProgress(FL) {                             \
    ASSERT((FL)->LockRequestsInProgress >= 0);                                 \
    (void)                                                                     \
    (InterlockedIncrement((LONG volatile *)&((FL)->LockRequestsInProgress)));  \
}

#define FsRtlDecrementLockRequestsInProgress(FL) {                             \
    ASSERT((FL)->LockRequestsInProgress > 0);                                  \
    (void)                                                                     \
    (InterlockedDecrement((LONG volatile *)&((FL)->LockRequestsInProgress)));  \
}

#ifdef _NTSYSTEM_
extern const UCHAR * const FsRtlLegalAnsiCharacterArray;
#define LEGAL_ANSI_CHARACTER_ARRAY FsRtlLegalAnsiCharacterArray
#else
__CREATE_NTOS_DATA_IMPORT_ALIAS(FsRtlLegalAnsiCharacterArray)
extern const UCHAR * const *FsRtlLegalAnsiCharacterArray;
#define LEGAL_ANSI_CHARACTER_ARRAY (*FsRtlLegalAnsiCharacterArray)
#endif

#define FsRtlIsAnsiCharacterWild(C)                                            \
    FsRtlTestAnsiCharacter((C), FALSE, FALSE, FSRTL_WILD_CHARACTER)

#define FsRtlIsAnsiCharacterLegalFat(C, WILD)                                  \
    FsRtlTestAnsiCharacter((C), TRUE, (WILD), FSRTL_FAT_LEGAL)

#define FsRtlIsAnsiCharacterLegalHpfs(C, WILD)                                 \
    FsRtlTestAnsiCharacter((C), TRUE, (WILD), FSRTL_HPFS_LEGAL)

#define FsRtlIsAnsiCharacterLegalNtfs(C, WILD)                                 \
    FsRtlTestAnsiCharacter((C), TRUE, (WILD), FSRTL_NTFS_LEGAL)

#define FsRtlIsAnsiCharacterLegalNtfsStream(C,WILD_OK)                         \
    FsRtlTestAnsiCharacter((C), TRUE, (WILD_OK), FSRTL_NTFS_STREAM_LEGAL)

#define FsRtlIsAnsiCharacterLegal(C,FLAGS)                                     \
    FsRtlTestAnsiCharacter((C), TRUE, FALSE, (FLAGS))

#define FsRtlTestAnsiCharacter(C, DEFAULT_RET, WILD_OK, FLAGS)                 \
    (((SCHAR)(C) < 0) ? DEFAULT_RET :                                          \
         FlagOn(LEGAL_ANSI_CHARACTER_ARRAY[(C)],                               \
                (FLAGS) | ((WILD_OK) ? FSRTL_WILD_CHARACTER : 0)))

#define FsRtlIsLeadDbcsCharacter(DBCS_CHAR)                                    \
    ((BOOLEAN)((UCHAR)(DBCS_CHAR) < 0x80 ? FALSE :                             \
              (NLS_MB_CODE_PAGE_TAG &&                                         \
               (NLS_OEM_LEAD_BYTE_INFO[(UCHAR)(DBCS_CHAR)] != 0))))

#define FsRtlIsUnicodeCharacterWild(C)                                         \
    ((((C) >= 0x40) ? FALSE :                                                  \
    FlagOn(LEGAL_ANSI_CHARACTER_ARRAY[(C)], FSRTL_WILD_CHARACTER )))

#define FsRtlInitPerFileContext(_fc, _owner, _inst, _cb)                       \
    ((_fc)->OwnerId = (_owner),                                                \
     (_fc)->InstanceId = (_inst),                                              \
     (_fc)->FreeCallback = (_cb))

#define FsRtlGetPerFileContextPointer(_fo)                                     \
    (FsRtlSupportsPerFileContexts(_fo) ?                                       \
        FsRtlGetPerStreamContextPointer(_fo)->FileContextSupportPointer : NULL)

#define FsRtlSupportsPerFileContexts(_fo)                                      \
    ((FsRtlGetPerStreamContextPointer(_fo) != NULL) &&                         \
     (FsRtlGetPerStreamContextPointer(_fo)->Version >= FSRTL_FCB_HEADER_V1) && \
     (FsRtlGetPerStreamContextPointer(_fo)->FileContextSupportPointer != NULL))

#define FsRtlSetupAdvancedHeaderEx(_advhdr, _fmutx, _fctxptr)                  \
{                                                                              \
    FsRtlSetupAdvancedHeader( _advhdr, _fmutx );                               \
    if ((_fctxptr) != NULL) {                                                  \
        (_advhdr)->FileContextSupportPointer = (_fctxptr);                     \
    }                                                                          \
}

#define FsRtlGetPerStreamContextPointer(FO)                                    \
    ((PFSRTL_ADVANCED_FCB_HEADER)(FO)->FsContext)

#define FsRtlInitPerStreamContext(PSC, O, I, FC)                               \
    ((PSC)->OwnerId = (O),                                                     \
    (PSC)->InstanceId = (I),                                                   \
    (PSC)->FreeCallback = (FC))

#define FsRtlSupportsPerStreamContexts(FO)                                     \
    ((BOOLEAN)((NULL != FsRtlGetPerStreamContextPointer(FO) &&                 \
               FlagOn(FsRtlGetPerStreamContextPointer(FO)->Flags2,             \
               FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS)))

#define FsRtlLookupPerStreamContext(_sc, _oid, _iid)                           \
    (((NULL != (_sc)) &&                                                       \
      FlagOn((_sc)->Flags2,FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS) &&            \
      !IsListEmpty(&(_sc)->FilterContexts)) ?                                  \
          FsRtlLookupPerStreamContextInternal((_sc), (_oid), (_iid)) : NULL)

_IRQL_requires_max_(APC_LEVEL)
FORCEINLINE
VOID
NTAPI
FsRtlSetupAdvancedHeader(
  _In_ PVOID AdvHdr,
  _In_ PFAST_MUTEX FMutex )
{
  PFSRTL_ADVANCED_FCB_HEADER localAdvHdr = (PFSRTL_ADVANCED_FCB_HEADER)AdvHdr;

  localAdvHdr->Flags |= FSRTL_FLAG_ADVANCED_HEADER;
  localAdvHdr->Flags2 |= FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS;
#if (NTDDI_VERSION >= NTDDI_VISTA)
  localAdvHdr->Version = FSRTL_FCB_HEADER_V1;
#else
  localAdvHdr->Version = FSRTL_FCB_HEADER_V0;
#endif
  InitializeListHead( &localAdvHdr->FilterContexts );
  if (FMutex != NULL) {
    localAdvHdr->FastMutex = FMutex;
  }
#if (NTDDI_VERSION >= NTDDI_VISTA)
  *((PULONG_PTR)(&localAdvHdr->PushLock)) = 0;
  localAdvHdr->FileContextSupportPointer = NULL;
#endif
}

#define FsRtlInitPerFileObjectContext(_fc, _owner, _inst)                      \
           ((_fc)->OwnerId = (_owner), (_fc)->InstanceId = (_inst))

#define FsRtlCompleteRequest(IRP, STATUS) {                                    \
    (IRP)->IoStatus.Status = (STATUS);                                         \
    IoCompleteRequest( (IRP), IO_DISK_INCREMENT );                             \
}
$endif (_NTIFS_)
