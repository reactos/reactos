/******************************************************************************
 *                            ZwXxx Functions                                 *
 ******************************************************************************/

$if (_WDMDDK_)

/* Constants */
#define NtCurrentProcess() ( (HANDLE)(LONG_PTR) -1 )
#define ZwCurrentProcess() NtCurrentProcess()
#define NtCurrentThread() ( (HANDLE)(LONG_PTR) -2 )
#define ZwCurrentThread() NtCurrentThread()

$endif

$if (_NTDDK_)

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateLocallyUniqueId(
  OUT PLUID Luid);

NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateProcess(
  IN HANDLE ProcessHandle OPTIONAL,
  IN NTSTATUS ExitStatus);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcess(
  OUT PHANDLE ProcessHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  IN PCLIENT_ID ClientId OPTIONAL);

$endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)

$if (_NTDDK_)

NTSTATUS
NTAPI
ZwCancelTimer(
  IN HANDLE TimerHandle,
  OUT PBOOLEAN CurrentState OPTIONAL);

NTSTATUS
NTAPI
ZwCreateTimer(
  OUT PHANDLE TimerHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN TIMER_TYPE TimerType);

NTSTATUS
NTAPI
ZwOpenTimer(
  OUT PHANDLE TimerHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationThread(
  IN HANDLE ThreadHandle,
  IN THREADINFOCLASS ThreadInformationClass,
  IN PVOID ThreadInformation,
  IN ULONG ThreadInformationLength);

NTSTATUS
NTAPI
ZwSetTimer(
  IN HANDLE TimerHandle,
  IN PLARGE_INTEGER DueTime,
  IN PTIMER_APC_ROUTINE TimerApcRoutine OPTIONAL,
  IN PVOID TimerContext OPTIONAL,
  IN BOOLEAN ResumeTimer,
  IN LONG Period OPTIONAL,
  OUT PBOOLEAN PreviousState OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
ZwDisplayString(
  IN PUNICODE_STRING String);

NTSYSAPI
NTSTATUS
NTAPI
ZwPowerInformation(
  IN POWER_INFORMATION_LEVEL PowerInformationLevel,
  IN PVOID InputBuffer OPTIONAL,
  IN ULONG InputBufferLength,
  OUT PVOID OutputBuffer OPTIONAL,
  IN ULONG OutputBufferLength);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryVolumeInformationFile(
  IN HANDLE FileHandle,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  OUT PVOID FsInformation,
  IN ULONG Length,
  IN FS_INFORMATION_CLASS FsInformationClass);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeviceIoControlFile(
  IN HANDLE FileHandle,
  IN HANDLE Event OPTIONAL,
  IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
  IN PVOID ApcContext OPTIONAL,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN ULONG IoControlCode,
  IN PVOID InputBuffer OPTIONAL,
  IN ULONG InputBufferLength,
  OUT PVOID OutputBuffer OPTIONAL,
  IN ULONG OutputBufferLength);

$endif

$if (_WDMDDK_)

NTSYSAPI
NTSTATUS
NTAPI
ZwClose(
  IN HANDLE Handle);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateDirectoryObject(
  OUT PHANDLE DirectoryHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateFile(
  OUT PHANDLE FileHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN PLARGE_INTEGER AllocationSize OPTIONAL,
  IN ULONG FileAttributes,
  IN ULONG ShareAccess,
  IN ULONG CreateDisposition,
  IN ULONG CreateOptions,
  IN PVOID EaBuffer OPTIONAL,
  IN ULONG EaLength);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateKey(
  OUT PHANDLE KeyHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  IN ULONG TitleIndex,
  IN PUNICODE_STRING Class OPTIONAL,
  IN ULONG CreateOptions,
  OUT PULONG Disposition OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSection(
  OUT PHANDLE SectionHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN PLARGE_INTEGER MaximumSize OPTIONAL,
  IN ULONG SectionPageProtection,
  IN ULONG AllocationAttributes,
  IN HANDLE FileHandle OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteKey(
  IN HANDLE KeyHandle);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteValueKey(
  IN HANDLE KeyHandle,
  IN PUNICODE_STRING ValueName);

NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateKey(
  IN HANDLE KeyHandle,
  IN ULONG Index,
  IN KEY_INFORMATION_CLASS KeyInformationClass,
  OUT PVOID KeyInformation OPTIONAL,
  IN ULONG Length,
  OUT PULONG ResultLength);

NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateValueKey(
  IN HANDLE KeyHandle,
  IN ULONG Index,
  IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
  OUT PVOID KeyValueInformation OPTIONAL,
  IN ULONG Length,
  OUT PULONG ResultLength);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushKey(
  IN HANDLE KeyHandle);

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadDriver(
  IN PUNICODE_STRING DriverServiceName);

NTSYSAPI
NTSTATUS
NTAPI
ZwMakeTemporaryObject(
  IN HANDLE Handle);

NTSYSAPI
NTSTATUS
NTAPI
ZwMapViewOfSection(
  IN HANDLE SectionHandle,
  IN HANDLE ProcessHandle,
  IN OUT PVOID *BaseAddress,
  IN ULONG_PTR ZeroBits,
  IN SIZE_T CommitSize,
  IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
  IN OUT PSIZE_T ViewSize,
  IN SECTION_INHERIT InheritDisposition,
  IN ULONG AllocationType,
  IN ULONG Protect);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenFile(
  OUT PHANDLE FileHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN ULONG ShareAccess,
  IN ULONG OpenOptions);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenKey(
  OUT PHANDLE KeyHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSection(
  OUT PHANDLE SectionHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSymbolicLinkObject(
  OUT PHANDLE LinkHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationFile(
  IN HANDLE FileHandle,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  OUT PVOID FileInformation,
  IN ULONG Length,
  IN FILE_INFORMATION_CLASS FileInformationClass);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryKey(
  IN HANDLE KeyHandle,
  IN KEY_INFORMATION_CLASS KeyInformationClass,
  OUT PVOID KeyInformation OPTIONAL,
  IN ULONG Length,
  OUT PULONG ResultLength);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySymbolicLinkObject(
  IN HANDLE LinkHandle,
  IN OUT PUNICODE_STRING LinkTarget,
  OUT PULONG ReturnedLength OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryValueKey(
  IN HANDLE KeyHandle,
  IN PUNICODE_STRING ValueName,
  IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
  OUT PVOID KeyValueInformation OPTIONAL,
  IN ULONG Length,
  OUT PULONG ResultLength);

NTSYSAPI
NTSTATUS
NTAPI
ZwReadFile(
  IN HANDLE FileHandle,
  IN HANDLE Event OPTIONAL,
  IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
  IN PVOID ApcContext OPTIONAL,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  OUT PVOID Buffer,
  IN ULONG Length,
  IN PLARGE_INTEGER ByteOffset OPTIONAL,
  IN PULONG Key OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationFile(
  IN HANDLE FileHandle,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN PVOID FileInformation,
  IN ULONG Length,
  IN FILE_INFORMATION_CLASS FileInformationClass);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetValueKey(
  IN HANDLE KeyHandle,
  IN PUNICODE_STRING ValueName,
  IN ULONG TitleIndex OPTIONAL,
  IN ULONG Type,
  IN PVOID Data OPTIONAL,
  IN ULONG DataSize);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadDriver(
  IN PUNICODE_STRING DriverServiceName);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnmapViewOfSection(
  IN HANDLE ProcessHandle,
  IN PVOID BaseAddress OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
ZwWriteFile(
  IN HANDLE FileHandle,
  IN HANDLE Event OPTIONAL,
  IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
  IN PVOID ApcContext OPTIONAL,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN PVOID Buffer,
  IN ULONG Length,
  IN PLARGE_INTEGER ByteOffset OPTIONAL,
  IN PULONG Key OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryFullAttributesFile(
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation);

$endif

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

$if (_WDMDDK_)

#if (NTDDI_VERSION >= NTDDI_WS03)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenEvent(
  OUT PHANDLE EventHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes);
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTSYSAPI
NTSTATUS
ZwCreateKeyTransacted(
  OUT PHANDLE KeyHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  IN ULONG TitleIndex,
  IN PUNICODE_STRING Class OPTIONAL,
  IN ULONG CreateOptions,
  IN HANDLE TransactionHandle,
  OUT PULONG Disposition OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenKeyTransacted(
  OUT PHANDLE KeyHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  IN HANDLE TransactionHandle);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateTransactionManager(
  OUT PHANDLE TmHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN PUNICODE_STRING LogFileName OPTIONAL,
  IN ULONG CreateOptions OPTIONAL,
  IN ULONG CommitStrength OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenTransactionManager(
  OUT PHANDLE TmHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN PUNICODE_STRING LogFileName OPTIONAL,
  IN LPGUID TmIdentity OPTIONAL,
  IN ULONG OpenOptions OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRollforwardTransactionManager(
  IN HANDLE TransactionManagerHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRecoverTransactionManager(
  IN HANDLE TransactionManagerHandle);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationTransactionManager(
  IN HANDLE TransactionManagerHandle,
  IN TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
  OUT PVOID TransactionManagerInformation,
  IN ULONG TransactionManagerInformationLength,
  OUT PULONG ReturnLength OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationTransactionManager(
  IN HANDLE TmHandle,
  IN TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
  IN PVOID TransactionManagerInformation,
  IN ULONG TransactionManagerInformationLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwEnumerateTransactionObject(
  IN HANDLE RootObjectHandle OPTIONAL,
  IN KTMOBJECT_TYPE QueryType,
  IN OUT PKTMOBJECT_CURSOR ObjectCursor,
  IN ULONG ObjectCursorLength,
  OUT PULONG ReturnLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateTransaction(
  OUT PHANDLE TransactionHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN LPGUID Uow OPTIONAL,
  IN HANDLE TmHandle OPTIONAL,
  IN ULONG CreateOptions OPTIONAL,
  IN ULONG IsolationLevel OPTIONAL,
  IN ULONG IsolationFlags OPTIONAL,
  IN PLARGE_INTEGER Timeout OPTIONAL,
  IN PUNICODE_STRING Description OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenTransaction(
  OUT PHANDLE TransactionHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN LPGUID Uow,
  IN HANDLE TmHandle OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationTransaction(
  IN HANDLE TransactionHandle,
  IN TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
  OUT PVOID TransactionInformation,
  IN ULONG TransactionInformationLength,
  OUT PULONG ReturnLength OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationTransaction(
  IN HANDLE TransactionHandle,
  IN TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
  IN PVOID TransactionInformation,
  IN ULONG TransactionInformationLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCommitTransaction(
  IN HANDLE TransactionHandle,
  IN BOOLEAN Wait);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRollbackTransaction(
  IN HANDLE TransactionHandle,
  IN BOOLEAN Wait);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateResourceManager(
  OUT PHANDLE ResourceManagerHandle,
  IN ACCESS_MASK DesiredAccess,
  IN HANDLE TmHandle,
  IN LPGUID ResourceManagerGuid OPTIONAL,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN ULONG CreateOptions OPTIONAL,
  IN PUNICODE_STRING Description OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenResourceManager(
  OUT PHANDLE ResourceManagerHandle,
  IN ACCESS_MASK DesiredAccess,
  IN HANDLE TmHandle,
  IN LPGUID ResourceManagerGuid,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRecoverResourceManager(
  IN HANDLE ResourceManagerHandle);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwGetNotificationResourceManager(
  IN HANDLE ResourceManagerHandle,
  OUT PTRANSACTION_NOTIFICATION TransactionNotification,
  IN ULONG NotificationLength,
  IN PLARGE_INTEGER Timeout,
  IN PULONG ReturnLength OPTIONAL,
  IN ULONG Asynchronous,
  IN ULONG_PTR AsynchronousContext OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationResourceManager(
  IN HANDLE ResourceManagerHandle,
  IN RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
  OUT PVOID ResourceManagerInformation,
  IN ULONG ResourceManagerInformationLength,
  IN PULONG ReturnLength OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationResourceManager(
  IN HANDLE ResourceManagerHandle,
  IN RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
  IN PVOID ResourceManagerInformation,
  IN ULONG ResourceManagerInformationLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateEnlistment(
  OUT PHANDLE EnlistmentHandle,
  IN ACCESS_MASK DesiredAccess,
  IN HANDLE ResourceManagerHandle,
  IN HANDLE TransactionHandle,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN ULONG CreateOptions OPTIONAL,
  IN NOTIFICATION_MASK NotificationMask,
  IN PVOID EnlistmentKey OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenEnlistment(
  OUT PHANDLE EnlistmentHandle,
  IN ACCESS_MASK DesiredAccess,
  IN HANDLE RmHandle,
  IN LPGUID EnlistmentGuid,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationEnlistment(
  IN HANDLE EnlistmentHandle,
  IN ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
  OUT PVOID EnlistmentInformation,
  IN ULONG EnlistmentInformationLength,
  IN PULONG ReturnLength OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationEnlistment(
  IN HANDLE EnlistmentHandle,
  IN ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
  IN PVOID EnlistmentInformation,
  IN ULONG EnlistmentInformationLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRecoverEnlistment(
  IN HANDLE EnlistmentHandle,
  IN PVOID EnlistmentKey OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPrePrepareEnlistment(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPrepareEnlistment(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCommitEnlistment(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRollbackEnlistment(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPrePrepareComplete(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPrepareComplete(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCommitComplete(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwReadOnlyEnlistment(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRollbackComplete(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSinglePhaseReject(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);


#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

$endif

#if (NTDDI_VERSION >= NTDDI_WIN7)

$if (_NTDDK_)
NTSTATUS
NTAPI
ZwSetTimerEx(
  IN HANDLE TimerHandle,
  IN TIMER_SET_INFORMATION_CLASS TimerSetInformationClass,
  IN OUT PVOID TimerSetInformation,
  IN ULONG TimerSetInformationLength);
$endif

$if (_WDMDDK_)

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenKeyEx(
  OUT PHANDLE KeyHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  IN ULONG OpenOptions);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenKeyTransactedEx(
  OUT PHANDLE KeyHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  IN ULONG OpenOptions,
  IN HANDLE TransactionHandle);

NTSYSAPI
NTSTATUS
NTAPI
ZwNotifyChangeMultipleKeys(
  IN HANDLE MasterKeyHandle,
  IN ULONG Count OPTIONAL,
  IN OBJECT_ATTRIBUTES SubordinateObjects[] OPTIONAL,
  IN HANDLE Event OPTIONAL,
  IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
  IN PVOID ApcContext OPTIONAL,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN ULONG CompletionFilter,
  IN BOOLEAN WatchTree,
  OUT PVOID Buffer OPTIONAL,
  IN ULONG BufferSize,
  IN BOOLEAN Asynchronous);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryMultipleValueKey(
  IN HANDLE KeyHandle,
  IN OUT PKEY_VALUE_ENTRY ValueEntries,
  IN ULONG EntryCount,
  OUT PVOID ValueBuffer,
  IN OUT PULONG BufferLength,
  OUT PULONG RequiredBufferLength OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
ZwRenameKey(
  IN HANDLE KeyHandle,
  IN PUNICODE_STRING NewName);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationKey(
  IN HANDLE KeyHandle,
  IN KEY_SET_INFORMATION_CLASS KeySetInformationClass,
  IN PVOID KeySetInformation,
  IN ULONG KeySetInformationLength);

$endif

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

