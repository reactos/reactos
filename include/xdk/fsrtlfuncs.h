$if (_NTIFS_)
/* FSRTL Functions */

#define FsRtlEnterFileSystem    KeEnterCriticalRegion
#define FsRtlExitFileSystem     KeLeaveCriticalRegion

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCopyRead(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN BOOLEAN Wait,
  IN ULONG LockKey,
  OUT PVOID Buffer,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCopyWrite(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN BOOLEAN Wait,
  IN ULONG LockKey,
  IN PVOID Buffer,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlReadDev(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN ULONG LockKey,
  OUT PMDL *MdlChain,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN PDEVICE_OBJECT DeviceObject OPTIONAL);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlReadCompleteDev(
  IN PFILE_OBJECT FileObject,
  IN PMDL MdlChain,
  IN PDEVICE_OBJECT DeviceObject OPTIONAL);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlPrepareMdlWriteDev(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN ULONG LockKey,
  OUT PMDL *MdlChain,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlWriteCompleteDev(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN PMDL MdlChain,
  IN PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
VOID
NTAPI
FsRtlAcquireFileExclusive(
  IN PFILE_OBJECT FileObject);

NTKERNELAPI
VOID
NTAPI
FsRtlReleaseFile(
  IN PFILE_OBJECT FileObject);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlGetFileSize(
  IN PFILE_OBJECT FileObject,
  OUT PLARGE_INTEGER FileSize);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsTotalDeviceFailure(
  IN NTSTATUS Status);

NTKERNELAPI
PFILE_LOCK
NTAPI
FsRtlAllocateFileLock(
  IN PCOMPLETE_LOCK_IRP_ROUTINE CompleteLockIrpRoutine OPTIONAL,
  IN PUNLOCK_ROUTINE UnlockRoutine OPTIONAL);

NTKERNELAPI
VOID
NTAPI
FsRtlFreeFileLock(
  IN PFILE_LOCK FileLock);

NTKERNELAPI
VOID
NTAPI
FsRtlInitializeFileLock(
  IN PFILE_LOCK FileLock,
  IN PCOMPLETE_LOCK_IRP_ROUTINE CompleteLockIrpRoutine OPTIONAL,
  IN PUNLOCK_ROUTINE UnlockRoutine OPTIONAL);

NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeFileLock(
  IN PFILE_LOCK FileLock);

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
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlProcessFileLock(
  IN PFILE_LOCK FileLock,
  IN PIRP Irp,
  IN PVOID Context OPTIONAL);

/*
  FsRtlCheckLockForReadAccess:

  All this really does is pick out the lock parameters from the irp (io stack
  location?), get IoGetRequestorProcess, and pass values on to
  FsRtlFastCheckLockForRead.
*/
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCheckLockForReadAccess(
  IN PFILE_LOCK FileLock,
  IN PIRP Irp);

/*
  FsRtlCheckLockForWriteAccess:

  All this really does is pick out the lock parameters from the irp (io stack
  location?), get IoGetRequestorProcess, and pass values on to
  FsRtlFastCheckLockForWrite.
*/
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCheckLockForWriteAccess(
  IN PFILE_LOCK FileLock,
  IN PIRP Irp);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlFastCheckLockForRead(
  IN PFILE_LOCK FileLock,
  IN PLARGE_INTEGER FileOffset,
  IN PLARGE_INTEGER Length,
  IN ULONG Key,
  IN PFILE_OBJECT FileObject,
  IN PVOID Process);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlFastCheckLockForWrite(
  IN PFILE_LOCK FileLock,
  IN PLARGE_INTEGER FileOffset,
  IN PLARGE_INTEGER Length,
  IN ULONG Key,
  IN PFILE_OBJECT FileObject,
  IN PVOID Process);

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
NTKERNELAPI
PFILE_LOCK_INFO
NTAPI
FsRtlGetNextFileLock(
  IN PFILE_LOCK FileLock,
  IN BOOLEAN Restart);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlFastUnlockSingle(
  IN PFILE_LOCK FileLock,
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN PLARGE_INTEGER Length,
  IN PEPROCESS Process,
  IN ULONG Key,
  IN PVOID Context OPTIONAL,
  IN BOOLEAN AlreadySynchronized);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlFastUnlockAll(
  IN PFILE_LOCK FileLock,
  IN PFILE_OBJECT FileObject,
  IN PEPROCESS Process,
  IN PVOID Context OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlFastUnlockAllByKey(
  IN PFILE_LOCK FileLock,
  IN PFILE_OBJECT FileObject,
  IN PEPROCESS Process,
  IN ULONG Key,
  IN PVOID Context OPTIONAL);

/*
  FsRtlPrivateLock:

  ret: IoStatus->Status: STATUS_PENDING, STATUS_LOCK_NOT_GRANTED

  Internals:
    -Calls IoCompleteRequest if Irp
    -Uses exception handling / ExRaiseStatus with STATUS_INSUFFICIENT_RESOURCES
*/
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlPrivateLock(
  IN PFILE_LOCK FileLock,
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN PLARGE_INTEGER Length,
  IN PEPROCESS Process,
  IN ULONG Key,
  IN BOOLEAN FailImmediately,
  IN BOOLEAN ExclusiveLock,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN PIRP Irp OPTIONAL,
  IN PVOID Context,
  IN BOOLEAN AlreadySynchronized);

NTKERNELAPI
VOID
NTAPI
FsRtlInitializeTunnelCache(
  IN PTUNNEL Cache);

NTKERNELAPI
VOID
NTAPI
FsRtlAddToTunnelCache(
  IN PTUNNEL Cache,
  IN ULONGLONG DirectoryKey,
  IN PUNICODE_STRING ShortName,
  IN PUNICODE_STRING LongName,
  IN BOOLEAN KeyByShortName,
  IN ULONG DataLength,
  IN PVOID Data);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlFindInTunnelCache(
  IN PTUNNEL Cache,
  IN ULONGLONG DirectoryKey,
  IN PUNICODE_STRING Name,
  OUT PUNICODE_STRING ShortName,
  OUT PUNICODE_STRING LongName,
  IN OUT PULONG DataLength,
  OUT PVOID Data);

NTKERNELAPI
VOID
NTAPI
FsRtlDeleteKeyFromTunnelCache(
  IN PTUNNEL Cache,
  IN ULONGLONG DirectoryKey);

NTKERNELAPI
VOID
NTAPI
FsRtlDeleteTunnelCache(
  IN PTUNNEL Cache);

NTKERNELAPI
VOID
NTAPI
FsRtlDissectDbcs(
  IN ANSI_STRING Name,
  OUT PANSI_STRING FirstPart,
  OUT PANSI_STRING RemainingPart);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlDoesDbcsContainWildCards(
  IN PANSI_STRING Name);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsDbcsInExpression(
  IN PANSI_STRING Expression,
  IN PANSI_STRING Name);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsFatDbcsLegal(
  IN ANSI_STRING DbcsName,
  IN BOOLEAN WildCardsPermissible,
  IN BOOLEAN PathNamePermissible,
  IN BOOLEAN LeadingBackslashPermissible);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsHpfsDbcsLegal(
  IN ANSI_STRING DbcsName,
  IN BOOLEAN WildCardsPermissible,
  IN BOOLEAN PathNamePermissible,
  IN BOOLEAN LeadingBackslashPermissible);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlNormalizeNtstatus(
  IN NTSTATUS Exception,
  IN NTSTATUS GenericException);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsNtstatusExpected(
  IN NTSTATUS Ntstatus);

NTKERNELAPI
PERESOURCE
NTAPI
FsRtlAllocateResource(
  VOID);

NTKERNELAPI
VOID
NTAPI
FsRtlInitializeLargeMcb(
  IN PLARGE_MCB Mcb,
  IN POOL_TYPE PoolType);

NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeLargeMcb(
  IN PLARGE_MCB Mcb);

NTKERNELAPI
VOID
NTAPI
FsRtlResetLargeMcb(
  IN PLARGE_MCB Mcb,
  IN BOOLEAN SelfSynchronized);

NTKERNELAPI
VOID
NTAPI
FsRtlTruncateLargeMcb(
  IN PLARGE_MCB Mcb,
  IN LONGLONG Vbn);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAddLargeMcbEntry(
  IN PLARGE_MCB Mcb,
  IN LONGLONG Vbn,
  IN LONGLONG Lbn,
  IN LONGLONG SectorCount);

NTKERNELAPI
VOID
NTAPI
FsRtlRemoveLargeMcbEntry(
  IN PLARGE_MCB Mcb,
  IN LONGLONG Vbn,
  IN LONGLONG SectorCount);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLargeMcbEntry(
  IN PLARGE_MCB Mcb,
  IN LONGLONG Vbn,
  OUT PLONGLONG Lbn OPTIONAL,
  OUT PLONGLONG SectorCountFromLbn OPTIONAL,
  OUT PLONGLONG StartingLbn OPTIONAL,
  OUT PLONGLONG SectorCountFromStartingLbn OPTIONAL,
  OUT PULONG Index OPTIONAL);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLastLargeMcbEntry(
  IN PLARGE_MCB Mcb,
  OUT PLONGLONG Vbn,
  OUT PLONGLONG Lbn);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLastLargeMcbEntryAndIndex(
  IN PLARGE_MCB OpaqueMcb,
  OUT PLONGLONG LargeVbn,
  OUT PLONGLONG LargeLbn,
  OUT PULONG Index);

NTKERNELAPI
ULONG
NTAPI
FsRtlNumberOfRunsInLargeMcb(
  IN PLARGE_MCB Mcb);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlGetNextLargeMcbEntry(
  IN PLARGE_MCB Mcb,
  IN ULONG RunIndex,
  OUT PLONGLONG Vbn,
  OUT PLONGLONG Lbn,
  OUT PLONGLONG SectorCount);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlSplitLargeMcb(
  IN PLARGE_MCB Mcb,
  IN LONGLONG Vbn,
  IN LONGLONG Amount);

NTKERNELAPI
VOID
NTAPI
FsRtlInitializeMcb(
  IN PMCB Mcb,
  IN POOL_TYPE PoolType);

NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeMcb(
  IN PMCB Mcb);

NTKERNELAPI
VOID
NTAPI
FsRtlTruncateMcb(
  IN PMCB Mcb,
  IN VBN Vbn);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAddMcbEntry(
  IN PMCB Mcb,
  IN VBN Vbn,
  IN LBN Lbn,
  IN ULONG SectorCount);

NTKERNELAPI
VOID
NTAPI
FsRtlRemoveMcbEntry(
  IN PMCB Mcb,
  IN VBN Vbn,
  IN ULONG SectorCount);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupMcbEntry(
  IN PMCB Mcb,
  IN VBN Vbn,
  OUT PLBN Lbn,
  OUT PULONG SectorCount OPTIONAL,
  OUT PULONG Index);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLastMcbEntry(
  IN PMCB Mcb,
  OUT PVBN Vbn,
  OUT PLBN Lbn);

NTKERNELAPI
ULONG
NTAPI
FsRtlNumberOfRunsInMcb(
  IN PMCB Mcb);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlGetNextMcbEntry(
  IN PMCB Mcb,
  IN ULONG RunIndex,
  OUT PVBN Vbn,
  OUT PLBN Lbn,
  OUT PULONG SectorCount);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlBalanceReads(
  IN PDEVICE_OBJECT TargetDevice);

NTKERNELAPI
VOID
NTAPI
FsRtlInitializeOplock(
  IN OUT POPLOCK Oplock);

NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeOplock(
  IN OUT POPLOCK Oplock);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlOplockFsctrl(
  IN POPLOCK Oplock,
  IN PIRP Irp,
  IN ULONG OpenCount);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlCheckOplock(
  IN POPLOCK Oplock,
  IN PIRP Irp,
  IN PVOID Context,
  IN POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine OPTIONAL,
  IN POPLOCK_FS_PREPOST_IRP PostIrpRoutine OPTIONAL);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlOplockIsFastIoPossible(
  IN POPLOCK Oplock);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCurrentBatchOplock(
  IN POPLOCK Oplock);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlNotifyVolumeEvent(
  IN PFILE_OBJECT FileObject,
  IN ULONG EventCode);

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyInitializeSync(
  IN PNOTIFY_SYNC *NotifySync);

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyUninitializeSync(
  IN PNOTIFY_SYNC *NotifySync);

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyFullChangeDirectory(
  IN PNOTIFY_SYNC NotifySync,
  IN PLIST_ENTRY NotifyList,
  IN PVOID FsContext,
  IN PSTRING FullDirectoryName,
  IN BOOLEAN WatchTree,
  IN BOOLEAN IgnoreBuffer,
  IN ULONG CompletionFilter,
  IN PIRP NotifyIrp OPTIONAL,
  IN PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback OPTIONAL,
  IN PSECURITY_SUBJECT_CONTEXT SubjectContext OPTIONAL);

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyFilterReportChange(
  IN PNOTIFY_SYNC NotifySync,
  IN PLIST_ENTRY NotifyList,
  IN PSTRING FullTargetName,
  IN USHORT TargetNameOffset,
  IN PSTRING StreamName OPTIONAL,
  IN PSTRING NormalizedParentName OPTIONAL,
  IN ULONG FilterMatch,
  IN ULONG Action,
  IN PVOID TargetContext OPTIONAL,
  IN PVOID FilterContext OPTIONAL);

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyFullReportChange(
  IN PNOTIFY_SYNC NotifySync,
  IN PLIST_ENTRY NotifyList,
  IN PSTRING FullTargetName,
  IN USHORT TargetNameOffset,
  IN PSTRING StreamName OPTIONAL,
  IN PSTRING NormalizedParentName OPTIONAL,
  IN ULONG FilterMatch,
  IN ULONG Action,
  IN PVOID TargetContext OPTIONAL);

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyCleanup(
  IN PNOTIFY_SYNC NotifySync,
  IN PLIST_ENTRY NotifyList,
  IN PVOID FsContext);

NTKERNELAPI
VOID
NTAPI
FsRtlDissectName(
  IN UNICODE_STRING Name,
  OUT PUNICODE_STRING FirstPart,
  OUT PUNICODE_STRING RemainingPart);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlDoesNameContainWildCards(
  IN PUNICODE_STRING Name);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAreNamesEqual(
  IN PCUNICODE_STRING Name1,
  IN PCUNICODE_STRING Name2,
  IN BOOLEAN IgnoreCase,
  IN PCWCH UpcaseTable OPTIONAL);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsNameInExpression(
  IN PUNICODE_STRING Expression,
  IN PUNICODE_STRING Name,
  IN BOOLEAN IgnoreCase,
  IN PWCHAR UpcaseTable OPTIONAL);

NTKERNELAPI
VOID
NTAPI
FsRtlPostPagingFileStackOverflow(
  IN PVOID Context,
  IN PKEVENT Event,
  IN PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine);

NTKERNELAPI
VOID
NTAPI
FsRtlPostStackOverflow (
  IN PVOID Context,
  IN PKEVENT Event,
  IN PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlRegisterUncProvider(
  OUT PHANDLE MupHandle,
  IN PUNICODE_STRING RedirectorDeviceName,
  IN BOOLEAN MailslotsSupported);

NTKERNELAPI
VOID
NTAPI
FsRtlDeregisterUncProvider(
  IN HANDLE Handle);

NTKERNELAPI
VOID
NTAPI
FsRtlTeardownPerStreamContexts(
  IN PFSRTL_ADVANCED_FCB_HEADER AdvancedHeader);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlCreateSectionForDataScan(
  OUT PHANDLE SectionHandle,
  OUT PVOID *SectionObject,
  OUT PLARGE_INTEGER SectionFileSize OPTIONAL,
  IN PFILE_OBJECT FileObject,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN PLARGE_INTEGER MaximumSize OPTIONAL,
  IN ULONG SectionPageProtection,
  IN ULONG AllocationAttributes,
  IN ULONG Flags);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyFilterChangeDirectory(
  IN PNOTIFY_SYNC NotifySync,
  IN PLIST_ENTRY NotifyList,
  IN PVOID FsContext,
  IN PSTRING FullDirectoryName,
  IN BOOLEAN WatchTree,
  IN BOOLEAN IgnoreBuffer,
  IN ULONG CompletionFilter,
  IN PIRP NotifyIrp OPTIONAL,
  IN PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback OPTIONAL,
  IN PSECURITY_SUBJECT_CONTEXT SubjectContext OPTIONAL,
  IN PFILTER_REPORT_CHANGE FilterCallback OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlInsertPerStreamContext(
  IN PFSRTL_ADVANCED_FCB_HEADER PerStreamContext,
  IN PFSRTL_PER_STREAM_CONTEXT Ptr);

NTKERNELAPI
PFSRTL_PER_STREAM_CONTEXT
NTAPI
FsRtlLookupPerStreamContextInternal(
  IN PFSRTL_ADVANCED_FCB_HEADER StreamContext,
  IN PVOID OwnerId OPTIONAL,
  IN PVOID InstanceId OPTIONAL);

NTKERNELAPI
PFSRTL_PER_STREAM_CONTEXT
NTAPI
FsRtlRemovePerStreamContext(
  IN PFSRTL_ADVANCED_FCB_HEADER StreamContext,
  IN PVOID OwnerId OPTIONAL,
  IN PVOID InstanceId OPTIONAL);

NTKERNELAPI
VOID
NTAPI
FsRtlIncrementCcFastReadNotPossible(
  VOID);

NTKERNELAPI
VOID
NTAPI
FsRtlIncrementCcFastReadWait(
  VOID);

NTKERNELAPI
VOID
NTAPI
FsRtlIncrementCcFastReadNoWait(
  VOID);

NTKERNELAPI
VOID
NTAPI
FsRtlIncrementCcFastReadResourceMiss(
  VOID);

NTKERNELAPI
LOGICAL
NTAPI
FsRtlIsPagingFile(
  IN PFILE_OBJECT FileObject);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_WS03)

NTKERNELAPI
VOID
NTAPI
FsRtlInitializeBaseMcb(
  IN PBASE_MCB Mcb,
  IN POOL_TYPE PoolType);

NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeBaseMcb(
  IN PBASE_MCB Mcb);

NTKERNELAPI
VOID
NTAPI
FsRtlResetBaseMcb(
  IN PBASE_MCB Mcb);

NTKERNELAPI
VOID
NTAPI
FsRtlTruncateBaseMcb(
  IN PBASE_MCB Mcb,
  IN LONGLONG Vbn);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAddBaseMcbEntry(
  IN PBASE_MCB Mcb,
  IN LONGLONG Vbn,
  IN LONGLONG Lbn,
  IN LONGLONG SectorCount);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlRemoveBaseMcbEntry(
  IN PBASE_MCB Mcb,
  IN LONGLONG Vbn,
  IN LONGLONG SectorCount);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupBaseMcbEntry(
  IN PBASE_MCB Mcb,
  IN LONGLONG Vbn,
  OUT PLONGLONG Lbn OPTIONAL,
  OUT PLONGLONG SectorCountFromLbn OPTIONAL,
  OUT PLONGLONG StartingLbn OPTIONAL,
  OUT PLONGLONG SectorCountFromStartingLbn OPTIONAL,
  OUT PULONG Index OPTIONAL);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLastBaseMcbEntry(
  IN PBASE_MCB Mcb,
  OUT PLONGLONG Vbn,
  OUT PLONGLONG Lbn);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLastBaseMcbEntryAndIndex(
  IN PBASE_MCB OpaqueMcb,
  IN OUT PLONGLONG LargeVbn,
  IN OUT PLONGLONG LargeLbn,
  IN OUT PULONG Index);

NTKERNELAPI
ULONG
NTAPI
FsRtlNumberOfRunsInBaseMcb(
  IN PBASE_MCB Mcb);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlGetNextBaseMcbEntry(
  IN PBASE_MCB Mcb,
  IN ULONG RunIndex,
  OUT PLONGLONG Vbn,
  OUT PLONGLONG Lbn,
  OUT PLONGLONG SectorCount);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlSplitBaseMcb(
  IN PBASE_MCB Mcb,
  IN LONGLONG Vbn,
  IN LONGLONG Amount);

#endif /* (NTDDI_VERSION >= NTDDI_WS03) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

BOOLEAN
NTAPI
FsRtlInitializeBaseMcbEx(
  IN PBASE_MCB Mcb,
  IN POOL_TYPE PoolType,
  IN USHORT Flags);

NTSTATUS
NTAPI
FsRtlAddBaseMcbEntryEx(
  IN PBASE_MCB Mcb,
  IN LONGLONG Vbn,
  IN LONGLONG Lbn,
  IN LONGLONG SectorCount);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCurrentOplock(
  IN POPLOCK Oplock);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlOplockBreakToNone(
  IN OUT POPLOCK Oplock,
  IN PIO_STACK_LOCATION IrpSp OPTIONAL,
  IN PIRP Irp,
  IN PVOID Context OPTIONAL,
  IN POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine OPTIONAL,
  IN POPLOCK_FS_PREPOST_IRP PostIrpRoutine OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlNotifyVolumeEventEx(
  IN PFILE_OBJECT FileObject,
  IN ULONG EventCode,
  IN PTARGET_DEVICE_CUSTOM_NOTIFICATION Event);

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyCleanupAll(
  IN PNOTIFY_SYNC NotifySync,
  IN PLIST_ENTRY NotifyList);

NTSTATUS
NTAPI
FsRtlRegisterUncProviderEx(
  OUT PHANDLE MupHandle,
  IN PUNICODE_STRING RedirDevName,
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG Flags);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlCancellableWaitForSingleObject(
  IN PVOID Object,
  IN PLARGE_INTEGER Timeout OPTIONAL,
  IN PIRP Irp OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlCancellableWaitForMultipleObjects(
  IN ULONG Count,
  IN PVOID ObjectArray[],
  IN WAIT_TYPE WaitType,
  IN PLARGE_INTEGER Timeout OPTIONAL,
  IN PKWAIT_BLOCK WaitBlockArray OPTIONAL,
  IN PIRP Irp OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlMupGetProviderInfoFromFileObject(
  IN PFILE_OBJECT pFileObject,
  IN ULONG Level,
  OUT PVOID pBuffer,
  IN OUT PULONG pBufferSize);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlMupGetProviderIdFromName(
  IN PUNICODE_STRING pProviderName,
  OUT PULONG32 pProviderId);

NTKERNELAPI
VOID
NTAPI
FsRtlIncrementCcFastMdlReadWait(
  VOID);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlValidateReparsePointBuffer(
  IN ULONG BufferLength,
  IN PREPARSE_DATA_BUFFER ReparseBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlRemoveDotsFromPath(
  IN OUT PWSTR OriginalString,
  IN USHORT PathLength,
  OUT USHORT *NewLength);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlAllocateExtraCreateParameterList(
  IN FSRTL_ALLOCATE_ECPLIST_FLAGS Flags,
  OUT PECP_LIST *EcpList);

NTKERNELAPI
VOID
NTAPI
FsRtlFreeExtraCreateParameterList(
  IN PECP_LIST EcpList);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlAllocateExtraCreateParameter(
  IN LPCGUID EcpType,
  IN ULONG SizeOfContext,
  IN FSRTL_ALLOCATE_ECP_FLAGS Flags,
  IN PFSRTL_EXTRA_CREATE_PARAMETER_CLEANUP_CALLBACK CleanupCallback OPTIONAL,
  IN ULONG PoolTag,
  OUT PVOID *EcpContext);

NTKERNELAPI
VOID
NTAPI
FsRtlFreeExtraCreateParameter(
  IN PVOID EcpContext);

NTKERNELAPI
VOID
NTAPI
FsRtlInitExtraCreateParameterLookasideList(
  IN OUT PVOID Lookaside,
  IN FSRTL_ECP_LOOKASIDE_FLAGS Flags,
  IN SIZE_T Size,
  IN ULONG Tag);

VOID
NTAPI
FsRtlDeleteExtraCreateParameterLookasideList(
  IN OUT PVOID Lookaside,
  IN FSRTL_ECP_LOOKASIDE_FLAGS Flags);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlAllocateExtraCreateParameterFromLookasideList(
  IN LPCGUID EcpType,
  IN ULONG SizeOfContext,
  IN FSRTL_ALLOCATE_ECP_FLAGS Flags,
  IN PFSRTL_EXTRA_CREATE_PARAMETER_CLEANUP_CALLBACK CleanupCallback OPTIONAL,
  IN OUT PVOID LookasideList,
  OUT PVOID *EcpContext);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlInsertExtraCreateParameter(
  IN OUT PECP_LIST EcpList,
  IN OUT PVOID EcpContext);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlFindExtraCreateParameter(
  IN PECP_LIST EcpList,
  IN LPCGUID EcpType,
  OUT PVOID *EcpContext OPTIONAL,
  OUT ULONG *EcpContextSize OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlRemoveExtraCreateParameter(
  IN OUT PECP_LIST EcpList,
  IN LPCGUID EcpType,
  OUT PVOID *EcpContext,
  OUT ULONG *EcpContextSize OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlGetEcpListFromIrp(
  IN PIRP Irp,
  OUT PECP_LIST *EcpList OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlSetEcpListIntoIrp(
  IN OUT PIRP Irp,
  IN PECP_LIST EcpList);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlGetNextExtraCreateParameter(
  IN PECP_LIST EcpList,
  IN PVOID CurrentEcpContext OPTIONAL,
  OUT LPGUID NextEcpType OPTIONAL,
  OUT PVOID *NextEcpContext OPTIONAL,
  OUT ULONG *NextEcpContextSize OPTIONAL);

NTKERNELAPI
VOID
NTAPI
FsRtlAcknowledgeEcp(
  IN PVOID EcpContext);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsEcpAcknowledged(
  IN PVOID EcpContext);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsEcpFromUserMode(
  IN PVOID EcpContext);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlChangeBackingFileObject(
  IN PFILE_OBJECT CurrentFileObject OPTIONAL,
  IN PFILE_OBJECT NewFileObject,
  IN FSRTL_CHANGE_BACKING_TYPE ChangeBackingType,
  IN ULONG Flags);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlLogCcFlushError(
  IN PUNICODE_STRING FileName,
  IN PDEVICE_OBJECT DeviceObject,
  IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
  IN NTSTATUS FlushError,
  IN ULONG Flags);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAreVolumeStartupApplicationsComplete(
  VOID);

NTKERNELAPI
ULONG
NTAPI
FsRtlQueryMaximumVirtualDiskNestingLevel(
  VOID);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlGetVirtualDiskNestingLevel(
  IN PDEVICE_OBJECT DeviceObject,
  OUT PULONG NestingLevel,
  OUT PULONG NestingFlags OPTIONAL);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (NTDDI_VERSION >= NTDDI_VISTASP1)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlCheckOplockEx(
  IN POPLOCK Oplock,
  IN PIRP Irp,
  IN ULONG Flags,
  IN PVOID Context OPTIONAL,
  IN POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine OPTIONAL,
  IN POPLOCK_FS_PREPOST_IRP PostIrpRoutine OPTIONAL);

#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAreThereCurrentOrInProgressFileLocks(
  IN PFILE_LOCK FileLock);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlOplockIsSharedRequest(
  IN PIRP Irp);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlOplockBreakH(
  IN POPLOCK Oplock,
  IN PIRP Irp,
  IN ULONG Flags,
  IN PVOID Context OPTIONAL,
  IN POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine OPTIONAL,
  IN POPLOCK_FS_PREPOST_IRP PostIrpRoutine OPTIONAL);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCurrentOplockH(
  IN POPLOCK Oplock);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlOplockBreakToNoneEx(
  IN OUT POPLOCK Oplock,
  IN PIRP Irp,
  IN ULONG Flags,
  IN PVOID Context OPTIONAL,
  IN POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine OPTIONAL,
  IN POPLOCK_FS_PREPOST_IRP PostIrpRoutine OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlOplockFsctrlEx(
  IN POPLOCK Oplock,
  IN PIRP Irp,
  IN ULONG OpenCount,
  IN ULONG Flags);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlOplockKeysEqual(
  IN PFILE_OBJECT Fo1 OPTIONAL,
  IN PFILE_OBJECT Fo2 OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlInitializeExtraCreateParameterList(
  IN OUT PECP_LIST EcpList);

NTKERNELAPI
VOID
NTAPI
FsRtlInitializeExtraCreateParameter(
  IN PECP_HEADER Ecp,
  IN ULONG EcpFlags,
  IN PFSRTL_EXTRA_CREATE_PARAMETER_CLEANUP_CALLBACK CleanupCallback OPTIONAL,
  IN ULONG TotalSize,
  IN LPCGUID EcpType,
  IN PVOID ListAllocatedFrom OPTIONAL);

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlInsertPerFileContext(
  IN PVOID* PerFileContextPointer,
  IN PFSRTL_PER_FILE_CONTEXT Ptr);

NTKERNELAPI
PFSRTL_PER_FILE_CONTEXT
NTAPI
FsRtlLookupPerFileContext(
  IN PVOID* PerFileContextPointer,
  IN PVOID OwnerId OPTIONAL,
  IN PVOID InstanceId OPTIONAL);

NTKERNELAPI
PFSRTL_PER_FILE_CONTEXT
NTAPI
FsRtlRemovePerFileContext(
  IN PVOID* PerFileContextPointer,
  IN PVOID OwnerId OPTIONAL,
  IN PVOID InstanceId OPTIONAL);

NTKERNELAPI
VOID
NTAPI
FsRtlTeardownPerFileContexts(
  IN PVOID* PerFileContextPointer);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlInsertPerFileObjectContext(
  IN PFILE_OBJECT FileObject,
  IN PFSRTL_PER_FILEOBJECT_CONTEXT Ptr);

NTKERNELAPI
PFSRTL_PER_FILEOBJECT_CONTEXT
NTAPI
FsRtlLookupPerFileObjectContext(
  IN PFILE_OBJECT FileObject,
  IN PVOID OwnerId OPTIONAL,
  IN PVOID InstanceId OPTIONAL);

NTKERNELAPI
PFSRTL_PER_FILEOBJECT_CONTEXT
NTAPI
FsRtlRemovePerFileObjectContext(
  IN PFILE_OBJECT FileObject,
  IN PVOID OwnerId OPTIONAL,
  IN PVOID InstanceId OPTIONAL);

#define FsRtlFastLock(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11) (       \
     FsRtlPrivateLock(A1, A2, A3, A4, A5, A6, A7, A8, A9, NULL, A10, A11)   \
)

#define FsRtlAreThereCurrentFileLocks(FL) ( \
    ((FL)->FastIoIsQuestionable)            \
)

#define FsRtlIncrementLockRequestsInProgress(FL) {                           \
    ASSERT( (FL)->LockRequestsInProgress >= 0 );                             \
    (void)                                                                   \
    (InterlockedIncrement((LONG volatile *)&((FL)->LockRequestsInProgress)));\
}

#define FsRtlDecrementLockRequestsInProgress(FL) {                           \
    ASSERT( (FL)->LockRequestsInProgress > 0 );                              \
    (void)                                                                   \
    (InterlockedDecrement((LONG volatile *)&((FL)->LockRequestsInProgress)));\
}

/* GCC compatible definition, MS one is retarded */
extern NTKERNELAPI const UCHAR * const FsRtlLegalAnsiCharacterArray;
#define LEGAL_ANSI_CHARACTER_ARRAY        FsRtlLegalAnsiCharacterArray

#define FsRtlIsAnsiCharacterWild(C) (                                       \
    FlagOn(FsRtlLegalAnsiCharacterArray[(UCHAR)(C)], FSRTL_WILD_CHARACTER ) \
)

#define FsRtlIsAnsiCharacterLegalFat(C, WILD) (                                \
    FlagOn(FsRtlLegalAnsiCharacterArray[(UCHAR)(C)], (FSRTL_FAT_LEGAL) |       \
                                        ((WILD) ? FSRTL_WILD_CHARACTER : 0 ))  \
)

#define FsRtlIsAnsiCharacterLegalHpfs(C, WILD) (                               \
    FlagOn(FsRtlLegalAnsiCharacterArray[(UCHAR)(C)], (FSRTL_HPFS_LEGAL) |      \
                                        ((WILD) ? FSRTL_WILD_CHARACTER : 0 ))  \
)

#define FsRtlIsAnsiCharacterLegalNtfs(C, WILD) (                               \
    FlagOn(FsRtlLegalAnsiCharacterArray[(UCHAR)(C)], (FSRTL_NTFS_LEGAL) |      \
                                        ((WILD) ? FSRTL_WILD_CHARACTER : 0 ))  \
)

#define FsRtlIsAnsiCharacterLegalNtfsStream(C,WILD_OK) (                    \
    FsRtlTestAnsiCharacter((C), TRUE, (WILD_OK), FSRTL_NTFS_STREAM_LEGAL)   \
)

#define FsRtlIsAnsiCharacterLegal(C,FLAGS) (          \
    FsRtlTestAnsiCharacter((C), TRUE, FALSE, (FLAGS)) \
)

#define FsRtlTestAnsiCharacter(C, DEFAULT_RET, WILD_OK, FLAGS) (            \
        ((SCHAR)(C) < 0) ? DEFAULT_RET :                                    \
                           FlagOn( LEGAL_ANSI_CHARACTER_ARRAY[(C)],         \
                                   (FLAGS) |                                \
                                   ((WILD_OK) ? FSRTL_WILD_CHARACTER : 0) ) \
)

#define FsRtlIsLeadDbcsCharacter(DBCS_CHAR) (                               \
    (BOOLEAN)((UCHAR)(DBCS_CHAR) < 0x80 ? FALSE :                           \
              (NLS_MB_CODE_PAGE_TAG &&                                      \
               (NLS_OEM_LEAD_BYTE_INFO[(UCHAR)(DBCS_CHAR)] != 0)))          \
)

#define FsRtlIsUnicodeCharacterWild(C) (                                    \
    (((C) >= 0x40) ?                                                        \
    FALSE :                                                                 \
    FlagOn(FsRtlLegalAnsiCharacterArray[(C)], FSRTL_WILD_CHARACTER ))       \
)

#define FsRtlInitPerFileContext( _fc, _owner, _inst, _cb)   \
    ((_fc)->OwnerId = (_owner),                               \
     (_fc)->InstanceId = (_inst),                             \
     (_fc)->FreeCallback = (_cb))

#define FsRtlGetPerFileContextPointer(_fo) \
    (FsRtlSupportsPerFileContexts(_fo) ? \
        FsRtlGetPerStreamContextPointer(_fo)->FileContextSupportPointer : \
        NULL)

#define FsRtlSupportsPerFileContexts(_fo)                     \
    ((FsRtlGetPerStreamContextPointer(_fo) != NULL) &&        \
     (FsRtlGetPerStreamContextPointer(_fo)->Version >= FSRTL_FCB_HEADER_V1) &&  \
     (FsRtlGetPerStreamContextPointer(_fo)->FileContextSupportPointer != NULL))

#define FsRtlSetupAdvancedHeaderEx( _advhdr, _fmutx, _fctxptr )                     \
{                                                                                   \
    FsRtlSetupAdvancedHeader( _advhdr, _fmutx );                                    \
    if ((_fctxptr) != NULL) {                                                       \
        (_advhdr)->FileContextSupportPointer = (_fctxptr);                          \
    }                                                                               \
}

#define FsRtlGetPerStreamContextPointer(FO) (   \
    (PFSRTL_ADVANCED_FCB_HEADER)(FO)->FsContext \
)

#define FsRtlInitPerStreamContext(PSC, O, I, FC) ( \
    (PSC)->OwnerId = (O),                          \
    (PSC)->InstanceId = (I),                       \
    (PSC)->FreeCallback = (FC)                     \
)

#define FsRtlSupportsPerStreamContexts(FO) (                       \
    (BOOLEAN)((NULL != FsRtlGetPerStreamContextPointer(FO) &&     \
              FlagOn(FsRtlGetPerStreamContextPointer(FO)->Flags2, \
              FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS))               \
)

#define FsRtlLookupPerStreamContext(_sc, _oid, _iid)                          \
 (((NULL != (_sc)) &&                                                         \
   FlagOn((_sc)->Flags2,FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS) &&              \
   !IsListEmpty(&(_sc)->FilterContexts)) ?                                    \
        FsRtlLookupPerStreamContextInternal((_sc), (_oid), (_iid)) :          \
        NULL)

FORCEINLINE
VOID
NTAPI
FsRtlSetupAdvancedHeader(
  IN PVOID AdvHdr,
  IN PFAST_MUTEX FMutex )
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

#define FsRtlInitPerFileObjectContext(_fc, _owner, _inst)         \
           ((_fc)->OwnerId = (_owner), (_fc)->InstanceId = (_inst))

#define FsRtlCompleteRequest(IRP,STATUS) {         \
    (IRP)->IoStatus.Status = (STATUS);             \
    IoCompleteRequest( (IRP), IO_DISK_INCREMENT ); \
}
$endif (_NTIFS_)
