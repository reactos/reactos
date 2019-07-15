/******************************************************************************
 *                            ZwXxx Functions                                 *
 ******************************************************************************/

$if (_WDMDDK_)

/* Constants */
#define NtCurrentProcess() ( (HANDLE)(LONG_PTR) -1 )
#define ZwCurrentProcess() NtCurrentProcess()
#define NtCurrentThread() ( (HANDLE)(LONG_PTR) -2 )
#define ZwCurrentThread() NtCurrentThread()
$endif (_WDMDDK_)

$if (_NTDDK_)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateLocallyUniqueId(
  _Out_ PLUID Luid);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateProcess(
  _In_opt_ HANDLE ProcessHandle,
  _In_ NTSTATUS ExitStatus);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcess(
  _Out_ PHANDLE ProcessHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ PCLIENT_ID ClientId);
$endif (_NTDDK_)
$if (_NTIFS_)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryEaFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _In_ ULONG Length,
  _In_ BOOLEAN ReturnSingleEntry,
  _In_reads_bytes_opt_(EaListLength) PVOID EaList,
  _In_ ULONG EaListLength,
  _In_opt_ PULONG EaIndex,
  _In_ BOOLEAN RestartScan);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetEaFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_reads_bytes_(Length) PVOID Buffer,
  _In_ ULONG Length);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateToken(
  _In_ HANDLE ExistingTokenHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_ BOOLEAN EffectiveOnly,
  _In_ TOKEN_TYPE TokenType,
  _Out_ PHANDLE NewTokenHandle);
$endif (_NTIFS_)

#if (NTDDI_VERSION >= NTDDI_WIN2K)
$if (_WDMDDK_)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwClose(
  _In_ HANDLE Handle);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwCreateDirectoryObject(
  _Out_ PHANDLE DirectoryHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwCreateFile(
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwCreateKey(
  _Out_ PHANDLE KeyHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _Reserved_ ULONG TitleIndex,
  _In_opt_ PUNICODE_STRING Class,
  _In_ ULONG CreateOptions,
  _Out_opt_ PULONG Disposition);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSection(
  _Out_ PHANDLE SectionHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ PLARGE_INTEGER MaximumSize,
  _In_ ULONG SectionPageProtection,
  _In_ ULONG AllocationAttributes,
  _In_opt_ HANDLE FileHandle);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteKey(
  _In_ HANDLE KeyHandle);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteValueKey(
  _In_ HANDLE KeyHandle,
  _In_ PUNICODE_STRING ValueName);

_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(Length == 0, _Post_satisfies_(return < 0))
_When_(Length > 0, _Post_satisfies_(return <= 0))
NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateKey(
  _In_ HANDLE KeyHandle,
  _In_ ULONG Index,
  _In_ KEY_INFORMATION_CLASS KeyInformationClass,
  _Out_writes_bytes_opt_(Length) PVOID KeyInformation,
  _In_ ULONG Length,
  _Out_ PULONG ResultLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(Length == 0, _Post_satisfies_(return < 0))
_When_(Length > 0, _Post_satisfies_(return <= 0))
NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateValueKey(
  _In_ HANDLE KeyHandle,
  _In_ ULONG Index,
  _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
  _Out_writes_bytes_opt_(Length) PVOID KeyValueInformation,
  _In_ ULONG Length,
  _Out_ PULONG ResultLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwFlushKey(
  _In_ HANDLE KeyHandle);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwLoadDriver(
  _In_ PUNICODE_STRING DriverServiceName);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwMakeTemporaryObject(
  _In_ HANDLE Handle);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwMapViewOfSection(
  _In_ HANDLE SectionHandle,
  _In_ HANDLE ProcessHandle,
  _Outptr_result_bytebuffer_(*ViewSize) PVOID *BaseAddress,
  _In_ ULONG_PTR ZeroBits,
  _In_ SIZE_T CommitSize,
  _Inout_opt_ PLARGE_INTEGER SectionOffset,
  _Inout_ PSIZE_T ViewSize,
  _In_ SECTION_INHERIT InheritDisposition,
  _In_ ULONG AllocationType,
  _In_ ULONG Protect);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenFile(
  _Out_ PHANDLE FileHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_ ULONG ShareAccess,
  _In_ ULONG OpenOptions);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenKey(
  _Out_ PHANDLE KeyHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSection(
  _Out_ PHANDLE SectionHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSymbolicLinkObject(
  _Out_ PHANDLE LinkHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _Out_writes_bytes_(Length) PVOID FileInformation,
  _In_ ULONG Length,
  _In_ FILE_INFORMATION_CLASS FileInformationClass);

_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(Length == 0, _Post_satisfies_(return < 0))
_When_(Length > 0, _Post_satisfies_(return <= 0))
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryKey(
  _In_ HANDLE KeyHandle,
  _In_ KEY_INFORMATION_CLASS KeyInformationClass,
  _Out_writes_bytes_opt_(Length) PVOID KeyInformation,
  _In_ ULONG Length,
  _Out_ PULONG ResultLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySymbolicLinkObject(
  _In_ HANDLE LinkHandle,
  _Inout_ PUNICODE_STRING LinkTarget,
  _Out_opt_ PULONG ReturnedLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(Length == 0, _Post_satisfies_(return < 0))
_When_(Length > 0, _Post_satisfies_(return <= 0))
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryValueKey(
  _In_ HANDLE KeyHandle,
  _In_ PUNICODE_STRING ValueName,
  _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
  _Out_writes_bytes_opt_(Length) PVOID KeyValueInformation,
  _In_ ULONG Length,
  _Out_ PULONG ResultLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwReadFile(
  _In_ HANDLE FileHandle,
  _In_opt_ HANDLE Event,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _In_ ULONG Length,
  _In_opt_ PLARGE_INTEGER ByteOffset,
  _In_opt_ PULONG Key);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_reads_bytes_(Length) PVOID FileInformation,
  _In_ ULONG Length,
  _In_ FILE_INFORMATION_CLASS FileInformationClass);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetValueKey(
  _In_ HANDLE KeyHandle,
  _In_ PUNICODE_STRING ValueName,
  _In_opt_ ULONG TitleIndex,
  _In_ ULONG Type,
  _In_reads_bytes_opt_(DataSize) PVOID Data,
  _In_ ULONG DataSize);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadDriver(
  _In_ PUNICODE_STRING DriverServiceName);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwUnmapViewOfSection(
  _In_ HANDLE ProcessHandle,
  _In_opt_ PVOID BaseAddress);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwWriteFile(
  _In_ HANDLE FileHandle,
  _In_opt_ HANDLE Event,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_reads_bytes_(Length) PVOID Buffer,
  _In_ ULONG Length,
  _In_opt_ PLARGE_INTEGER ByteOffset,
  _In_opt_ PULONG Key);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryFullAttributesFile(
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation);

$endif (_WDMDDK_)
$if (_NTDDK_)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
ZwCancelTimer(
  _In_ HANDLE TimerHandle,
  _Out_opt_ PBOOLEAN CurrentState);

_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(return == 0, __drv_allocatesMem(TimerObject))
NTSTATUS
NTAPI
ZwCreateTimer(
  _Out_ PHANDLE TimerHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_ TIMER_TYPE TimerType);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
ZwOpenTimer(
  _Out_ PHANDLE TimerHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationThread(
  _In_ HANDLE ThreadHandle,
  _In_ THREADINFOCLASS ThreadInformationClass,
  _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
  _In_ ULONG ThreadInformationLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
ZwSetTimer(
  _In_ HANDLE TimerHandle,
  _In_ PLARGE_INTEGER DueTime,
  _In_opt_ PTIMER_APC_ROUTINE TimerApcRoutine,
  _In_opt_ PVOID TimerContext,
  _In_ BOOLEAN ResumeTimer,
  _In_opt_ LONG Period,
  _Out_opt_ PBOOLEAN PreviousState);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwDisplayString(
  _In_ PUNICODE_STRING String);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwPowerInformation(
  _In_ POWER_INFORMATION_LEVEL PowerInformationLevel,
  _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
  _In_ ULONG InputBufferLength,
  _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
  _In_ ULONG OutputBufferLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryVolumeInformationFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _Out_writes_bytes_(Length) PVOID FsInformation,
  _In_ ULONG Length,
  _In_ FS_INFORMATION_CLASS FsInformationClass);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwDeviceIoControlFile(
  _In_ HANDLE FileHandle,
  _In_opt_ HANDLE Event,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_ ULONG IoControlCode,
  _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
  _In_ ULONG InputBufferLength,
  _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
  _In_ ULONG OutputBufferLength);

$endif (_NTDDK_)
$if (_NTIFS_)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryObject(
  _In_opt_ HANDLE Handle,
  _In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
  _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
  _In_ ULONG ObjectInformationLength,
  _Out_opt_ PULONG ReturnLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwNotifyChangeKey(
  _In_ HANDLE KeyHandle,
  _In_opt_ HANDLE EventHandle,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_ ULONG NotifyFilter,
  _In_ BOOLEAN WatchSubtree,
  _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
  _In_ ULONG BufferLength,
  _In_ BOOLEAN Asynchronous);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwCreateEvent(
  _Out_ PHANDLE EventHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_ EVENT_TYPE EventType,
  _In_ BOOLEAN InitialState);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteFile(
  _In_ POBJECT_ATTRIBUTES ObjectAttributes);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryFile(
  _In_ HANDLE FileHandle,
  _In_opt_ HANDLE Event,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _Out_writes_bytes_(Length) PVOID FileInformation,
  _In_ ULONG Length,
  _In_ FILE_INFORMATION_CLASS FileInformationClass,
  _In_ BOOLEAN ReturnSingleEntry,
  _In_opt_ PUNICODE_STRING FileName,
  _In_ BOOLEAN RestartScan);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetVolumeInformationFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_reads_bytes_(Length) PVOID FsInformation,
  _In_ ULONG Length,
  _In_ FS_INFORMATION_CLASS FsInformationClass);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwFsControlFile(
  _In_ HANDLE FileHandle,
  _In_opt_ HANDLE Event,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_ ULONG FsControlCode,
  _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
  _In_ ULONG InputBufferLength,
  _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
  _In_ ULONG OutputBufferLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateObject(
  _In_ HANDLE SourceProcessHandle,
  _In_ HANDLE SourceHandle,
  _In_opt_ HANDLE TargetProcessHandle,
  _Out_opt_ PHANDLE TargetHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ ULONG HandleAttributes,
  _In_ ULONG Options);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenDirectoryObject(
  _Out_ PHANDLE DirectoryHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes);

_Must_inspect_result_
_At_(*BaseAddress, __drv_allocatesMem(Mem))
__kernel_entry
NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ _Outptr_result_buffer_(*RegionSize) PVOID *BaseAddress,
    _In_ ULONG_PTR ZeroBits,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG AllocationType,
    _In_ ULONG Protect);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwFreeVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ __drv_freesMem(Mem) PVOID *BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG FreeType);

_When_(Timeout == NULL, _IRQL_requires_max_(APC_LEVEL))
_When_(Timeout->QuadPart != 0, _IRQL_requires_max_(APC_LEVEL))
_When_(Timeout->QuadPart == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForSingleObject(
  _In_ HANDLE Handle,
  _In_ BOOLEAN Alertable,
  _In_opt_ PLARGE_INTEGER Timeout);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetEvent(
  _In_ HANDLE EventHandle,
  _Out_opt_ PLONG PreviousState);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwFlushVirtualMemory(
  _In_ HANDLE ProcessHandle,
  _Inout_ PVOID *BaseAddress,
  _Inout_ PSIZE_T RegionSize,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationToken(
  _In_ HANDLE TokenHandle,
  _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
  _Out_writes_bytes_to_opt_(Length,*ResultLength) PVOID TokenInformation,
  _In_ ULONG Length,
  _Out_ PULONG ResultLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetSecurityObject(
  _In_ HANDLE Handle,
  _In_ SECURITY_INFORMATION SecurityInformation,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySecurityObject(
  _In_ HANDLE FileHandle,
  _In_ SECURITY_INFORMATION SecurityInformation,
  _Out_writes_bytes_to_(Length,*ResultLength) PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ ULONG Length,
  _Out_ PULONG ResultLength);
$endif (_NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

$if (_NTIFS_)
#if (NTDDI_VERSION >= NTDDI_WINXP)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcessTokenEx(
  _In_ HANDLE ProcessHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ ULONG HandleAttributes,
  _Out_ PHANDLE TokenHandle);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadTokenEx(
  _In_ HANDLE ThreadHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ BOOLEAN OpenAsSelf,
  _In_ ULONG HandleAttributes,
  _Out_ PHANDLE TokenHandle);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */
$endif (_NTIFS_)
$if (_WDMDDK_)

#if (NTDDI_VERSION >= NTDDI_WS03)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenEvent(
  _Out_ PHANDLE EventHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes);
#endif
$endif (_WDMDDK_)

$if (_WDMDDK_ || _NTIFS_)
#if (NTDDI_VERSION >= NTDDI_VISTA)
$endif (_WDMDDK_ || _NTIFS_)
$if (_WDMDDK_)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
ZwCreateKeyTransacted(
  _Out_ PHANDLE KeyHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _Reserved_ ULONG TitleIndex,
  _In_opt_ PUNICODE_STRING Class,
  _In_ ULONG CreateOptions,
  _In_ HANDLE TransactionHandle,
  _Out_opt_ PULONG Disposition);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenKeyTransacted(
  _Out_ PHANDLE KeyHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_ HANDLE TransactionHandle);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateTransactionManager(
  _Out_ PHANDLE TmHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ PUNICODE_STRING LogFileName,
  _In_opt_ ULONG CreateOptions,
  _In_opt_ ULONG CommitStrength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenTransactionManager(
  _Out_ PHANDLE TmHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ PUNICODE_STRING LogFileName,
  _In_opt_ LPGUID TmIdentity,
  _In_opt_ ULONG OpenOptions);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRollforwardTransactionManager(
  _In_ HANDLE TransactionManagerHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRecoverTransactionManager(
  _In_ HANDLE TransactionManagerHandle);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationTransactionManager(
  _In_ HANDLE TransactionManagerHandle,
  _In_ TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
  _Out_writes_bytes_(TransactionManagerInformationLength) PVOID TransactionManagerInformation,
  _In_ ULONG TransactionManagerInformationLength,
  _Out_opt_ PULONG ReturnLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationTransactionManager(
  _In_ HANDLE TmHandle,
  _In_ TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
  _In_ PVOID TransactionManagerInformation,
  _In_ ULONG TransactionManagerInformationLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwEnumerateTransactionObject(
  _In_opt_ HANDLE RootObjectHandle,
  _In_ KTMOBJECT_TYPE QueryType,
  _Inout_updates_bytes_(ObjectCursorLength) PKTMOBJECT_CURSOR ObjectCursor,
  _In_ ULONG ObjectCursorLength,
  _Out_ PULONG ReturnLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateTransaction(
  _Out_ PHANDLE TransactionHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ LPGUID Uow,
  _In_opt_ HANDLE TmHandle,
  _In_opt_ ULONG CreateOptions,
  _In_opt_ ULONG IsolationLevel,
  _In_opt_ ULONG IsolationFlags,
  _In_opt_ PLARGE_INTEGER Timeout,
  _In_opt_ PUNICODE_STRING Description);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenTransaction(
  _Out_ PHANDLE TransactionHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_ LPGUID Uow,
  _In_opt_ HANDLE TmHandle);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationTransaction(
  _In_ HANDLE TransactionHandle,
  _In_ TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
  _Out_writes_bytes_(TransactionInformationLength) PVOID TransactionInformation,
  _In_ ULONG TransactionInformationLength,
  _Out_opt_ PULONG ReturnLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationTransaction(
  _In_ HANDLE TransactionHandle,
  _In_ TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
  _In_ PVOID TransactionInformation,
  _In_ ULONG TransactionInformationLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCommitTransaction(
  _In_ HANDLE TransactionHandle,
  _In_ BOOLEAN Wait);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRollbackTransaction(
  _In_ HANDLE TransactionHandle,
  _In_ BOOLEAN Wait);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateResourceManager(
  _Out_ PHANDLE ResourceManagerHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ HANDLE TmHandle,
  _In_opt_ LPGUID ResourceManagerGuid,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ ULONG CreateOptions,
  _In_opt_ PUNICODE_STRING Description);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenResourceManager(
  _Out_ PHANDLE ResourceManagerHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ HANDLE TmHandle,
  _In_ LPGUID ResourceManagerGuid,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRecoverResourceManager(
  _In_ HANDLE ResourceManagerHandle);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwGetNotificationResourceManager(
  _In_ HANDLE ResourceManagerHandle,
  _Out_ PTRANSACTION_NOTIFICATION TransactionNotification,
  _In_ ULONG NotificationLength,
  _In_ PLARGE_INTEGER Timeout,
  _Out_opt_ PULONG ReturnLength,
  _In_ ULONG Asynchronous,
  _In_opt_ ULONG_PTR AsynchronousContext);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationResourceManager(
  _In_ HANDLE ResourceManagerHandle,
  _In_ RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
  _Out_writes_bytes_(ResourceManagerInformationLength) PVOID ResourceManagerInformation,
  _In_ ULONG ResourceManagerInformationLength,
  _Out_opt_ PULONG ReturnLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationResourceManager(
  _In_ HANDLE ResourceManagerHandle,
  _In_ RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
  _In_reads_bytes_(ResourceManagerInformationLength) PVOID ResourceManagerInformation,
  _In_ ULONG ResourceManagerInformationLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateEnlistment(
  _Out_ PHANDLE EnlistmentHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ HANDLE ResourceManagerHandle,
  _In_ HANDLE TransactionHandle,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ ULONG CreateOptions,
  _In_ NOTIFICATION_MASK NotificationMask,
  _In_opt_ PVOID EnlistmentKey);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenEnlistment(
  _Out_ PHANDLE EnlistmentHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ HANDLE RmHandle,
  _In_ LPGUID EnlistmentGuid,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationEnlistment(
  _In_ HANDLE EnlistmentHandle,
  _In_ ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
  _Out_writes_bytes_(EnlistmentInformationLength) PVOID EnlistmentInformation,
  _In_ ULONG EnlistmentInformationLength,
  _Out_opt_ PULONG ReturnLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationEnlistment(
  _In_ HANDLE EnlistmentHandle,
  _In_ ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
  _In_reads_bytes_(EnlistmentInformationLength) PVOID EnlistmentInformation,
  _In_ ULONG EnlistmentInformationLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRecoverEnlistment(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PVOID EnlistmentKey);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPrePrepareEnlistment(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPrepareEnlistment(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCommitEnlistment(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRollbackEnlistment(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPrePrepareComplete(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPrepareComplete(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCommitComplete(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwReadOnlyEnlistment(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRollbackComplete(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSinglePhaseReject(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);
$endif (_WDMDDK_)
$if (_NTIFS_)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwLockFile(
  _In_ HANDLE FileHandle,
  _In_opt_ HANDLE Event,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_ PLARGE_INTEGER ByteOffset,
  _In_ PLARGE_INTEGER Length,
  _In_ ULONG Key,
  _In_ BOOLEAN FailImmediately,
  _In_ BOOLEAN ExclusiveLock);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwUnlockFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_ PLARGE_INTEGER ByteOffset,
  _In_ PLARGE_INTEGER Length,
  _In_ ULONG Key);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryQuotaInformationFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _In_ ULONG Length,
  _In_ BOOLEAN ReturnSingleEntry,
  _In_reads_bytes_opt_(SidListLength) PVOID SidList,
  _In_ ULONG SidListLength,
  _In_opt_ PSID StartSid,
  _In_ BOOLEAN RestartScan);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetQuotaInformationFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_reads_bytes_(Length) PVOID Buffer,
  _In_ ULONG Length);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwFlushBuffersFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock);
$endif (_NTIFS_)
$if (_WDMDDK_ || _NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */
$endif (_WDMDDK_ || _NTIFS_)
#if (NTDDI_VERSION >= NTDDI_WIN7)
$if (_WDMDDK_)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenKeyEx(
  _Out_ PHANDLE KeyHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_ ULONG OpenOptions);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenKeyTransactedEx(
  _Out_ PHANDLE KeyHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_ ULONG OpenOptions,
  _In_ HANDLE TransactionHandle);

NTSYSAPI
NTSTATUS
NTAPI
ZwNotifyChangeMultipleKeys(
  _In_ HANDLE MasterKeyHandle,
  _In_opt_ ULONG Count,
  _In_opt_ OBJECT_ATTRIBUTES SubordinateObjects[],
  _In_opt_ HANDLE Event,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_ ULONG CompletionFilter,
  _In_ BOOLEAN WatchTree,
  _Out_opt_ PVOID Buffer,
  _In_ ULONG BufferSize,
  _In_ BOOLEAN Asynchronous);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryMultipleValueKey(
  _In_ HANDLE KeyHandle,
  _Inout_ PKEY_VALUE_ENTRY ValueEntries,
  _In_ ULONG EntryCount,
  _Out_ PVOID ValueBuffer,
  _Inout_ PULONG BufferLength,
  _Out_opt_ PULONG RequiredBufferLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwRenameKey(
  _In_ HANDLE KeyHandle,
  _In_ PUNICODE_STRING NewName);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationKey(
  _In_ HANDLE KeyHandle,
  _In_ __drv_strictTypeMatch(__drv_typeConst) KEY_SET_INFORMATION_CLASS KeySetInformationClass,
  _In_reads_bytes_(KeySetInformationLength) PVOID KeySetInformation,
  _In_ ULONG KeySetInformationLength);

$endif (_WDMDDK_)
$if (_NTDDK_)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
ZwSetTimerEx(
  _In_ HANDLE TimerHandle,
  _In_ TIMER_SET_INFORMATION_CLASS TimerSetInformationClass,
  _Inout_updates_bytes_opt_(TimerSetInformationLength) PVOID TimerSetInformation,
  _In_ ULONG TimerSetInformationLength);
$endif (_NTDDK_)
$if (_NTIFS_)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationToken(
  _In_ HANDLE TokenHandle,
  _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
  _In_reads_bytes_(TokenInformationLength) PVOID TokenInformation,
  _In_ ULONG TokenInformationLength);

#if (VER_PRODUCTBUILD >= 2195)
NTSYSAPI
NTSTATUS
NTAPI
ZwAdjustPrivilegesToken (
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN DisableAllPrivileges,
    _In_ PTOKEN_PRIVILEGES NewState,
    _In_ ULONG BufferLength,
    _Out_opt_ PTOKEN_PRIVILEGES PreviousState,
    _Out_ PULONG ReturnLength
);
#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwAlertThread (
    _In_ HANDLE ThreadHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheckAndAuditAlarm (
    _In_ PUNICODE_STRING SubsystemName,
    _In_ PVOID HandleId,
    _In_ PUNICODE_STRING ObjectTypeName,
    _In_ PUNICODE_STRING ObjectName,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ BOOLEAN ObjectCreation,
    _Out_ PACCESS_MASK GrantedAccess,
    _Out_ PBOOLEAN AccessStatus,
    _Out_ PBOOLEAN GenerateOnClose
);

#if (VER_PRODUCTBUILD >= 2195)
NTSYSAPI
NTSTATUS
NTAPI
ZwCancelIoFile (
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
);
#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwClearEvent (
    _In_ HANDLE EventHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCloseObjectAuditAlarm (
    _In_ PUNICODE_STRING SubsystemName,
    _In_ PVOID HandleId,
    _In_ BOOLEAN GenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSymbolicLinkObject (
    _Out_ PHANDLE SymbolicLinkHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ PUNICODE_STRING TargetName
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushInstructionCache (
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ ULONG FlushSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushBuffersFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
);

#if (VER_PRODUCTBUILD >= 2195)
NTSYSAPI
NTSTATUS
NTAPI
ZwInitiatePowerAction (
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE MinSystemState,
    _In_ ULONG Flags,
    _In_ BOOLEAN Asynchronous
);
#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadKey (
    _In_ POBJECT_ATTRIBUTES KeyObjectAttributes,
    _In_ POBJECT_ATTRIBUTES FileObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcessToken (
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThread (
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ PCLIENT_ID ClientId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadToken (
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN OpenAsSelf,
    _Out_ PHANDLE TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPulseEvent (
    _In_ HANDLE EventHandle,
    _In_opt_ PLONG PulseCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDefaultLocale (
    _In_ BOOLEAN UserProfile,
    _Out_ PLCID DefaultLocaleId
);

#if (VER_PRODUCTBUILD >= 2195)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryObject(
    _In_ HANDLE DirectoryHandle,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_ BOOLEAN RestartScan,
    _Inout_ PULONG Context,
    _Out_opt_ PULONG ReturnLength
);
#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwReplaceKey (
    _In_ POBJECT_ATTRIBUTES NewFileObjectAttributes,
    _In_ HANDLE KeyHandle,
    _In_ POBJECT_ATTRIBUTES OldFileObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwResetEvent (
    _In_ HANDLE EventHandle,
    _Out_opt_ PLONG NumberOfWaitingThreads
);

#if (VER_PRODUCTBUILD >= 2195)
NTSYSAPI
NTSTATUS
NTAPI
ZwRestoreKey (
    _In_ HANDLE KeyHandle,
    _In_ HANDLE FileHandle,
    _In_ ULONG Flags
);
#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwSaveKey (
    _In_ HANDLE KeyHandle,
    _In_ HANDLE FileHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultLocale (
    _In_ BOOLEAN UserProfile,
    _In_ LCID DefaultLocaleId
);

#if (VER_PRODUCTBUILD >= 2195)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultUILanguage (
    _In_ LANGID LanguageId
);
#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationProcess (
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _In_ PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemTime (
    _In_ PLARGE_INTEGER   NewTime,
    _Out_opt_ PLARGE_INTEGER OldTime
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadKey (
    _In_ POBJECT_ATTRIBUTES KeyObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForMultipleObjects (
    _In_ ULONG HandleCount,
    _In_ PHANDLE Handles,
    _In_ WAIT_TYPE WaitType,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
);

NTSYSAPI
NTSTATUS
NTAPI
ZwYieldExecution (
    VOID
);

$endif (_NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

