#include <msvcrt/stdlib.h>
#include <windows.h>
#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

/*
 * @unimplemented
 */
PDEBUG_BUFFER STDCALL
RtlCreateQueryDebugBuffer(IN ULONG Size,
			  IN BOOLEAN EventPair)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
DWORD STDCALL RtlDeleteSecurityObject(DWORD x1)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlDestroyQueryDebugBuffer(IN PDEBUG_BUFFER DebugBuffer)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
VOID
STDCALL
RtlGetCallersAddress(
	PVOID	* CallersAddress
	)
{
}

/*
 * @unimplemented
 */
VOID
STDCALL
RtlInitializeGenericTable (
	IN OUT	PRTL_GENERIC_TABLE	Table,
	IN	PVOID			CompareRoutine,
	IN	PVOID			AllocateRoutine,
	IN	PVOID			FreeRoutine,
	IN	ULONG			UserParameter
	)
{
}

/*
 * @unimplemented
 */
PVOID
STDCALL
RtlInsertElementGenericTable (
	IN OUT	PRTL_GENERIC_TABLE	Table,
	IN	PVOID			Element,
	IN	ULONG			ElementSize,
	IN	ULONG			Unknown4
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
RtlIsGenericTableEmpty (
	IN	PRTL_GENERIC_TABLE	Table
	)
{
  return(FALSE);
}
DWORD     STDCALL RtlNewSecurityObject(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
ULONG
STDCALL
RtlNumberGenericTableElements (
	IN	PRTL_GENERIC_TABLE	Table
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlQueryProcessDebugInformation(IN ULONG ProcessId,
				IN ULONG DebugInfoClassMask,
				IN OUT PDEBUG_BUFFER DebugBuffer)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS  STDCALL RtlWalkHeap( HANDLE heap, PVOID entry_ptr )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS  STDCALL RtlpUnWaitCriticalSection(RTL_CRITICAL_SECTION *crit)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS  STDCALL RtlpWaitForCriticalSection(RTL_CRITICAL_SECTION *crit)
{
  return(FALSE);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL LdrLockLoaderLock(ULONG flags, ULONG *result, ULONG *magic)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL LdrUnlockLoaderLock(ULONG flags, ULONG magic)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL NtPowerInformation(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5)
{
  return(FALSE);
}


/*
 * @unimplemented
 */
VOID
STDCALL
RtlCaptureContext (
    PCONTEXT ContextRecord
    )
{
}

/*
 * @unimplemented
 */
NTSTATUS  STDCALL RtlDuplicateUnicodeString(
    int add_nul,
    const UNICODE_STRING *source,
    UNICODE_STRING *destination)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS  STDCALL RtlFindCharInUnicodeString(
    int flags,
    const UNICODE_STRING *main_str,
    const UNICODE_STRING *search_chars,
    USHORT *pos)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlInitUnicodeStringEx(PUNICODE_STRING target,PCWSTR source)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS  STDCALL RtlInt64ToUnicodeString(ULONGLONG value,ULONG base,UNICODE_STRING *str)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
ULONGLONG
STDCALL
VerSetConditionMask(
        ULONGLONG   ConditionMask,
        DWORD   TypeMask,
        BYTE    Condition
        )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL ZwPowerInformation(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
double __cdecl _CIpow(double x,double y)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL STDCALL LdrFlushAlternateResourceModules(VOID)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL STDCALL LdrAlternateResourcesEnabled(VOID)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
ULONG
__cdecl
DbgPrintEx(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    ...
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
ULONG
__cdecl
DbgPrintReturnControlC(
    PCH Format,
    ...
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
DbgQueryDebugFilterState(
    IN ULONG ComponentId,
    IN ULONG Level
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
DbgSetDebugFilterState(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN BOOL State
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtCancelDeviceWakeupRequest(
    IN HANDLE Device
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtCompactKeys(
    IN ULONG Count,
    IN HANDLE KeyArray[]
            )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtCompressKey(
    IN HANDLE Key
            )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtGetDevicePowerState(
    IN HANDLE Device,
    OUT DEVICE_POWER_STATE *State
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtInitiatePowerAction(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags,
    IN BOOL Asynchronous
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
NtIsSystemResumeAutomatic(
    VOID
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtLockProductActivationKeys(
    ULONG   *pPrivateVer,
    ULONG   *pIsSafeMode
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtLockRegistryKey(
    IN HANDLE           KeyHandle
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtNotifyChangeMultipleKeys(
    IN HANDLE MasterKeyHandle,
    IN ULONG Count,
    IN OBJECT_ATTRIBUTES SlaveObjects[],
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOL WatchTree,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN BOOL Asynchronous
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtQueryOpenSubKeys(
    IN POBJECT_ATTRIBUTES TargetKey,
    OUT PULONG  HandleCount
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtRenameKey(
    IN HANDLE           KeyHandle,
    IN PUNICODE_STRING  NewName
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtRequestDeviceWakeup(
    IN HANDLE Device
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtRequestWakeupLatency(
    IN LATENCY_TIME latency
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtSaveKeyEx(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG  Format
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtSaveMergedKeys(
    IN HANDLE HighPrecedenceKeyHandle,
    IN HANDLE LowPrecedenceKeyHandle,
    IN HANDLE FileHandle
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtSetThreadExecutionState(
    IN EXECUTION_STATE esFlags,
    OUT EXECUTION_STATE *PreviousFlags
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtUnloadKeyEx(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN HANDLE Event OPTIONAL
    )
{
  return(FALSE);
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlGUIDFromString(
    IN PUNICODE_STRING GuidString,
    OUT GUID* Guid
    )
{
  return(FALSE);
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlGetVersion(
    OUT PRTL_OSVERSIONINFOW lpVersionInformation
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlHashUnicodeString(
    IN const UNICODE_STRING *String,
    IN BOOL CaseInSensitive,
    IN ULONG HashAlgorithm,
    OUT PULONG HashValue
    )
{
  return(FALSE);
}


/*
 * @unimplemented
 */
BOOL
STDCALL
RtlValidRelativeSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptorInput,
    IN ULONG SecurityDescriptorLength,
    IN SECURITY_INFORMATION RequiredInformation
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlVerifyVersionInfo(
    IN PRTL_OSVERSIONINFOEXW VersionInfo,
    IN ULONG TypeMask,
    IN ULONGLONG  ConditionMask
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
ULONG
STDCALL
RtlWalkFrameChain (
    OUT PVOID *Callers,
    IN ULONG Count,
    IN ULONG Flags
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
ULONG
vDbgPrintEx(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    va_list arglist
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
ULONG
vDbgPrintExWithPrefix(
    IN PCH Prefix,
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    va_list arglist
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwCancelDeviceWakeupRequest(
    IN HANDLE Device
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwCompactKeys(
    IN ULONG Count,
    IN HANDLE KeyArray[]
            )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwCompressKey(
    IN HANDLE Key
            )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwGetDevicePowerState(
    IN HANDLE Device,
    OUT DEVICE_POWER_STATE *State
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwInitiatePowerAction(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags,
    IN BOOL Asynchronous
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ZwIsSystemResumeAutomatic(
    VOID
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwLockProductActivationKeys(
    ULONG   *pPrivateVer,
    ULONG   *pIsSafeMode
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwLockRegistryKey(
    IN HANDLE           KeyHandle
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwNotifyChangeMultipleKeys(
    IN HANDLE MasterKeyHandle,  		
    IN ULONG Count,
    IN OBJECT_ATTRIBUTES SlaveObjects[],
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOL WatchTree,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN BOOL Asynchronous
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwQueryOpenSubKeys(
    IN POBJECT_ATTRIBUTES TargetKey,
    OUT PULONG  HandleCount
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwRenameKey(
    IN HANDLE           KeyHandle,
    IN PUNICODE_STRING  NewName
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwRequestDeviceWakeup(
    IN HANDLE Device
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwRequestWakeupLatency(
    IN LATENCY_TIME latency
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwSaveKeyEx(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG  Format
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwSaveMergedKeys(
    IN HANDLE HighPrecedenceKeyHandle,
    IN HANDLE LowPrecedenceKeyHandle,
    IN HANDLE FileHandle
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwSetThreadExecutionState(
    IN EXECUTION_STATE esFlags,
    OUT EXECUTION_STATE *PreviousFlags
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwUnloadKeyEx(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN HANDLE Event OPTIONAL
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL DbgUiDebugActiveProcess(HANDLE process)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL DbgUiStopDebugging(HANDLE process)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
VOID
STDCALL
RtlInitializeSListHead (
    PSLIST_HEADER ListHead
    )
{
}

/*
 * @unimplemented
 */
PSLIST_ENTRY
STDCALL
RtlInterlockedFlushSList (
    PSLIST_HEADER ListHead
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
PSLIST_ENTRY
STDCALL
RtlInterlockedPopEntrySList (
    PSLIST_HEADER ListHead
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
PSLIST_ENTRY
STDCALL
RtlInterlockedPushEntrySList (
    PSLIST_HEADER ListHead,
    PSLIST_ENTRY ListEntry
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
USHORT
STDCALL
RtlQueryDepthSList (
    PSLIST_HEADER ListHead
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlCreateTimer(HANDLE TimerQueue,PHANDLE phNewTimer, WAITORTIMERCALLBACK Callback,PVOID Parameter,DWORD DueTime,DWORD Period,ULONG Flags)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlCreateTimerQueue(PHANDLE TimerQueue)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlDeleteTimer(HANDLE TimerQueue,HANDLE Timer,HANDLE CompletionEvent)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlUpdateTimer(HANDLE TimerQueue,HANDLE Timer,ULONG DueTime,ULONG Period)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlDeleteTimerQueueEx(HANDLE TimerQueue,HANDLE CompletionEvent)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlDeleteTimerQueue(HANDLE TimerQueue)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlRegisterWait(PHANDLE hOutput, HANDLE hObject,WAITORTIMERCALLBACK Callback,PVOID Context,ULONG dwMilliseconds,ULONG dwFlags)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlDeregisterWait(
    HANDLE WaitHandle
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlDeregisterWaitEx(
    HANDLE WaitHandle,
    HANDLE CompletionEvent
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlSetHeapInformation(
    HANDLE HeapHandle,
    HEAP_INFORMATION_CLASS HeapInformationClass,
    PVOID HeapInformation OPTIONAL,
    SIZE_T HeapInformationLength OPTIONAL
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlQueryHeapInformation(
    HANDLE HeapHandle, 
    HEAP_INFORMATION_CLASS HeapInformationClass,
    PVOID HeapInformation OPTIONAL,
    SIZE_T HeapInformationLength OPTIONAL,
    PSIZE_T ReturnLength OPTIONAL
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtCreateJobSet(
    ULONG NumJob,
    PJOB_SET_ARRAY UserJobSet,
    ULONG Flags)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwCreateJobSet(
    ULONG NumJob,
    PJOB_SET_ARRAY UserJobSet,
    ULONG Flags)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
VOID
STDCALL
RtlReleaseActivationContext(
    HANDLE hActCtx
    )
{
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlQueryInformationActivationContext(
    DWORD dwFlags,
    HANDLE hActCtx,
    PVOID pvSubInstance,
    ULONG ulInfoClass,
    PVOID pvBuffer,
    SIZE_T cbBuffer OPTIONAL,
    SIZE_T *pcbWrittenOrRequired OPTIONAL
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
VOID
STDCALL
RtlAddRefActivationContext(
    HANDLE hActCtx
    )
{
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlGetActiveActivationContext(
    HANDLE *lphActCtx)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlZombifyActivationContext(
    HANDLE hActCtx
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlDeactivateActivationContext(
    DWORD dwFlags,
    ULONG_PTR ulCookie
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
ULONG 
STDCALL
RtlCreateTagHeap(	
	IN HANDLE HeapHandle,
	IN ULONG Flags,
	IN PCWSTR TagName,
	IN PCWSTR TagSubName
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
PRTL_SPLAY_LINKS
STDCALL
RtlDelete(
	IN PRTL_SPLAY_LINKS Links
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
RtlDeleteElementGenericTable(
	IN PRTL_GENERIC_TABLE Table,
	IN PVOID Buffer
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
VOID
STDCALL
RtlDeleteNoSplay(
	IN PRTL_SPLAY_LINKS Links,
	OUT PRTL_SPLAY_LINKS *Root
	)
{
}

/*
 * @unimplemented
 */
PVOID
STDCALL
RtlEnumerateGenericTable(
	IN PRTL_GENERIC_TABLE Table,
	IN BOOLEAN Restart
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
PVOID
STDCALL
RtlEnumerateGenericTableWithoutSplaying(
	IN PRTL_GENERIC_TABLE Table,
	IN PVOID *RestartKey
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
VOID
STDCALL
RtlGetElementGenericTable(
	IN PRTL_GENERIC_TABLE Table,
	IN ULONG I
	)
{
}

/*
 * @unimplemented
 */
PVOID
STDCALL
RtlLookupElementGenericTable(
	IN PRTL_GENERIC_TABLE Table,
	IN PVOID Buffer
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
PVOID
STDCALL
RtlProtectHeap(
	IN HANDLE Heap,
	IN BOOLEAN ReadOnly
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
PCWSTR 
STDCALL
RtlQueryTagHeap(
	IN HANDLE HeapHandle,
	IN ULONG Flags,
	IN USHORT TagNumber,
	IN BOOLEAN ZeroInternalTagInfo,
	OUT PRTL_HEAP_TAG_INFO HeapTagInfo OPTIONAL
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
PRTL_SPLAY_LINKS
STDCALL
RtlRealPredecessor(
	IN PRTL_SPLAY_LINKS Links
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
PRTL_SPLAY_LINKS
STDCALL
RtlRealSuccessor(
	IN PRTL_SPLAY_LINKS Links
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
PRTL_SPLAY_LINKS
STDCALL
RtlSplay(
	IN PRTL_SPLAY_LINKS Links
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtAccessCheckByType(
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN HANDLE TokenHandle,
	IN ULONG DesiredAccess,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN PPRIVILEGE_SET PrivilegeSet,
	IN PULONG PrivilegeSetLength,
	OUT PACCESS_MASK GrantedAccess,
	OUT PULONG AccessStatus
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtAccessCheckByTypeAndAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN PUNICODE_STRING ObjectTypeName,
	IN PUNICODE_STRING ObjectName,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN ACCESS_MASK DesiredAccess,
	IN AUDIT_EVENT_TYPE AuditType,
	IN ULONG Flags,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN BOOLEAN ObjectCreation,
	OUT PACCESS_MASK GrantedAccess,
	OUT PULONG AccessStatus,
	OUT PBOOLEAN GenerateOnClose
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtAccessCheckByTypeResultList(
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN HANDLE TokenHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN PPRIVILEGE_SET PrivilegeSet,
	IN PULONG PrivilegeSetLength,
	OUT PACCESS_MASK GrantedAccessList,
	OUT PULONG AccessStatusList
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtAccessCheckByTypeResultListAndAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN PUNICODE_STRING ObjectTypeName,
	IN PUNICODE_STRING ObjectName,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN ACCESS_MASK DesiredAccess,
	IN AUDIT_EVENT_TYPE AuditType,
	IN ULONG Flags,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN BOOLEAN ObjectCreation,
	OUT PACCESS_MASK GrantedAccessList,
	OUT PULONG AccessStatusList,
	OUT PULONG GenerateOnClose
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtAccessCheckByTypeResultListAndAuditAlarmByHandle(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN HANDLE TokenHandle,
	IN PUNICODE_STRING ObjectTypeName,
	IN PUNICODE_STRING ObjectName,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN ACCESS_MASK DesiredAccess,
	IN AUDIT_EVENT_TYPE AuditType,
	IN ULONG Flags,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN BOOLEAN ObjectCreation,
	OUT PACCESS_MASK GrantedAccessList,
	OUT PULONG AccessStatusList,
	OUT PULONG GenerateOnClose
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtAllocateUserPhysicalPages(
	IN HANDLE ProcessHandle,
	IN PULONG NumberOfPages,
	OUT PULONG PageFrameNumbers
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtAreMappedFilesTheSame(
	IN PVOID Address1,
	IN PVOID Address2
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtCreateJobObject(
	OUT PHANDLE JobHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtCreateKeyedEvent(
	OUT PHANDLE KeyedEventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG Reserved
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtFilterToken(
	IN HANDLE ExistingTokenHandle,
	IN ULONG Flags,
	IN PTOKEN_GROUPS SidsToDisable,
	IN PTOKEN_PRIVILEGES PrivilegesToDelete,
	IN PTOKEN_GROUPS SidsToRestricted,
	OUT PHANDLE NewTokenHandle
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtFreeUserPhysicalPages(
	IN HANDLE ProcessHandle,
	IN OUT PULONG NumberOfPages,
	IN PULONG PageFrameNumbers
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtGetWriteWatch(
	IN HANDLE ProcessHandle,
	IN ULONG Flags,
	IN PVOID BaseAddress,
	IN ULONG RegionSize,
	OUT PULONG Buffer,
	IN OUT PULONG BufferEntries,
	OUT PULONG Granularity
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtImpersonateAnonymousToken(
	IN HANDLE ThreadHandle
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtIsProcessInJob(
	IN HANDLE JobHandle,
	IN HANDLE ProcessHandle
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtMakePermanentObject(
	IN HANDLE Object
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtMapUserPhysicalPages(
	IN PVOID BaseAddress,
	IN PULONG NumberOfPages,
	IN PULONG PageFrameNumbers
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtMapUserPhysicalPagesScatter(
	IN PVOID *BaseAddresses,
	IN PULONG NumberOfPages,
	IN PULONG PageFrameNumbers
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtOpenJobObject(
	OUT PHANDLE JobHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtOpenKeyedEvent(
	OUT PHANDLE KeyedEventHandle,
	IN PACCESS_MASK DesiredAccess,
	IN PUNICODE_STRING KeyedEventName
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtOpenProcessTokenEx(
	IN HANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN ULONG HandleAttributes,
	OUT PHANDLE TokenHandle
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtOpenThreadTokenEx(
	IN HANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN BOOLEAN OpenAsSelf,
	IN ULONG HandleAttributes,
	OUT PHANDLE TokenHandle
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtQueryDefaultUILanguage(
	OUT PLANGID LanguageId
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtQueryInformationJobObject(
	IN HANDLE JobHandle,
	IN JOBOBJECTINFOCLASS JobInformationClass,
	OUT PVOID JobInformation,
	IN ULONG JobInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtQueryInstallUILanguage(
	OUT PLANGID LanguageId
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
ULONG
STDCALL
NtQueryPortInformationProcess(
	VOID
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtReleaseKeyedEvent(
	IN HANDLE KeyedEventHandle,
	IN ULONG Requested,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Timeout OPTIONAL
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtReplyWaitReceivePortEx(
	IN HANDLE PortHandle,
	OUT PULONG PortIdentifier OPTIONAL,
	IN PPORT_MESSAGE ReplyMessage OPTIONAL,
	OUT PPORT_MESSAGE Message,
	IN PLARGE_INTEGER Timeout
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtResetWriteWatch(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	IN ULONG RegionSize
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtResumeProcess(
	IN HANDLE Process
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtSecureConnectPort(
	OUT PHANDLE PortHandle,
	IN PUNICODE_STRING PortName,
	IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	IN OUT PPORT_SECTION_WRITE WriteSection OPTIONAL,
	IN PSID ServerSid OPTIONAL,
	IN OUT PPORT_SECTION_READ ReadSection OPTIONAL,
	OUT PULONG MaxMessageSize OPTIONAL,
	IN OUT PVOID ConnectData OPTIONAL,
	IN OUT PULONG ConnectDataLength OPTIONAL
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtSetDefaultUILanguage(
	IN LANGID LanguageId
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtSetInformationJobObject(
	IN HANDLE JobHandle,
	IN JOBOBJECTINFOCLASS JobInformationClass,
	IN PVOID JobInformation,
	IN ULONG JobInformationLength
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtSetQuotaInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PFILE_USER_QUOTA_INFORMATION Buffer,
	IN ULONG BufferLength
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtSetUuidSeed(
	IN PUCHAR UuidSeed
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtSuspendProcess(
	IN HANDLE Process
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtTerminateJobObject(
	IN HANDLE JobHandle,
	IN NTSTATUS ExitStatus
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtWaitForKeyedEvent(
	IN HANDLE KeyedEventHandle,
	IN ULONG Requested,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Timeout OPTIONAL
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
RtlDllShutdownInProgress(
	VOID
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
PPEB
STDCALL
RtlGetCurrentPeb(
	VOID
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlGetLastNtStatus(
	VOID
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
ULONG
STDCALL
RtlGetLastWin32Error(
	VOID
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlGetNativeSystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	IN OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
USHORT
STDCALL
RtlLogStackBackTrace(
	VOID
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlQueueWorkItem(
	LPTHREAD_START_ROUTINE Function,
	PVOID Context,
	ULONG Flags
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
VOID
STDCALL
RtlRestoreLastWin32Error(
	IN ULONG Win32Error
	)
{
}

/*
 * @unimplemented
 */
ULONG
STDCALL
RtlSetCriticalSectionSpinCount(
   IN PRTL_CRITICAL_SECTION CriticalSection
   )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlSetIoCompletionCallback(
	IN HANDLE FileHandle,
	IN POVERLAPPED_COMPLETION_ROUTINE CompletionRoutine,
	IN ULONG Flags
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
VOID
STDCALL
RtlSetLastWin32Error(
	IN ULONG Win32Error
	)
{
}

/*
 * @unimplemented
 */
VOID
STDCALL
RtlSetLastWin32ErrorAndNtStatusFromNtStatus(
	IN NTSTATUS NtStatus
	)
{
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwAccessCheckByType(
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN HANDLE TokenHandle,
	IN ULONG DesiredAccess,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN PPRIVILEGE_SET PrivilegeSet,
	IN PULONG PrivilegeSetLength,
	OUT PACCESS_MASK GrantedAccess,
	OUT PULONG AccessStatus
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwAccessCheckByTypeAndAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN PUNICODE_STRING ObjectTypeName,
	IN PUNICODE_STRING ObjectName,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN ACCESS_MASK DesiredAccess,
	IN AUDIT_EVENT_TYPE AuditType,
	IN ULONG Flags,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN BOOLEAN ObjectCreation,
	OUT PACCESS_MASK GrantedAccess,
	OUT PULONG AccessStatus,
	OUT PBOOLEAN GenerateOnClose
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwAccessCheckByTypeResultList(
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN HANDLE TokenHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN PPRIVILEGE_SET PrivilegeSet,
	IN PULONG PrivilegeSetLength,
	OUT PACCESS_MASK GrantedAccessList,
	OUT PULONG AccessStatusList
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwAccessCheckByTypeResultListAndAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN PUNICODE_STRING ObjectTypeName,
	IN PUNICODE_STRING ObjectName,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN ACCESS_MASK DesiredAccess,
	IN AUDIT_EVENT_TYPE AuditType,
	IN ULONG Flags,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN BOOLEAN ObjectCreation,
	OUT PACCESS_MASK GrantedAccessList,
	OUT PULONG AccessStatusList,
	OUT PULONG GenerateOnClose
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwAccessCheckByTypeResultListAndAuditAlarmByHandle(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN HANDLE TokenHandle,
	IN PUNICODE_STRING ObjectTypeName,
	IN PUNICODE_STRING ObjectName,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN ACCESS_MASK DesiredAccess,
	IN AUDIT_EVENT_TYPE AuditType,
	IN ULONG Flags,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN BOOLEAN ObjectCreation,
	OUT PACCESS_MASK GrantedAccessList,
	OUT PULONG AccessStatusList,
	OUT PULONG GenerateOnClose
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwAllocateUserPhysicalPages(
	IN HANDLE ProcessHandle,
	IN PULONG NumberOfPages,
	OUT PULONG PageFrameNumbers
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwAreMappedFilesTheSame(
	IN PVOID Address1,
	IN PVOID Address2
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwAssignProcessToJobObject(
	IN HANDLE JobHandle,
	IN HANDLE ProcessHandle
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwCreateJobObject(
	OUT PHANDLE JobHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwCreateKeyedEvent(
	OUT PHANDLE KeyedEventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG Reserved
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwFilterToken(
	IN HANDLE ExistingTokenHandle,
	IN ULONG Flags,
	IN PTOKEN_GROUPS SidsToDisable,
	IN PTOKEN_PRIVILEGES PrivilegesToDelete,
	IN PTOKEN_GROUPS SidsToRestricted,
	OUT PHANDLE NewTokenHandle
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwFreeUserPhysicalPages(
	IN HANDLE ProcessHandle,
	IN OUT PULONG NumberOfPages,
	IN PULONG PageFrameNumbers
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwGetWriteWatch(
	IN HANDLE ProcessHandle,
	IN ULONG Flags,
	IN PVOID BaseAddress,
	IN ULONG RegionSize,
	OUT PULONG Buffer,
	IN OUT PULONG BufferEntries,
	OUT PULONG Granularity
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwImpersonateAnonymousToken(
	IN HANDLE ThreadHandle
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwIsProcessInJob(
	IN HANDLE JobHandle,
	IN HANDLE ProcessHandle
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwMakePermanentObject(
	IN HANDLE Object
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwMapUserPhysicalPages(
	IN PVOID BaseAddress,
	IN PULONG NumberOfPages,
	IN PULONG PageFrameNumbers
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwMapUserPhysicalPagesScatter(
	IN PVOID *BaseAddresses,
	IN PULONG NumberOfPages,
	IN PULONG PageFrameNumbers
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwOpenJobObject(
	OUT PHANDLE JobHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwOpenKeyedEvent(
	OUT PHANDLE KeyedEventHandle,
	IN PACCESS_MASK DesiredAccess,
	IN PUNICODE_STRING KeyedEventName
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwOpenProcessTokenEx(
	IN HANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN ULONG HandleAttributes,
	OUT PHANDLE TokenHandle
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwOpenThreadTokenEx(
	IN HANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN BOOLEAN OpenAsSelf,
	IN ULONG HandleAttributes,
	OUT PHANDLE TokenHandle
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwQueryDefaultUILanguage(
	OUT PLANGID LanguageId
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwQueryInformationJobObject(
	IN HANDLE JobHandle,
	IN JOBOBJECTINFOCLASS JobInformationClass,
	OUT PVOID JobInformation,
	IN ULONG JobInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwQueryInstallUILanguage(
	OUT PLANGID LanguageId
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
ULONG
STDCALL
ZwQueryPortInformationProcess(
	VOID
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwQueryQuotaInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PFILE_USER_QUOTA_INFORMATION Buffer,
	IN ULONG BufferLength,
	IN BOOLEAN ReturnSingleEntry,
	IN PFILE_QUOTA_LIST_INFORMATION QuotaList OPTIONAL,
	IN ULONG QuotaListLength,
	IN PSID ResumeSid OPTIONAL,
	IN BOOLEAN RestartScan
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwReleaseKeyedEvent(
	IN HANDLE KeyedEventHandle,
	IN ULONG Requested,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Timeout OPTIONAL
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwReplyWaitReceivePortEx(
	IN HANDLE PortHandle,
	OUT PULONG PortIdentifier OPTIONAL,
	IN PPORT_MESSAGE ReplyMessage OPTIONAL,
	OUT PPORT_MESSAGE Message,
	IN PLARGE_INTEGER Timeout
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwResetWriteWatch(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	IN ULONG RegionSize
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwResumeProcess(
	IN HANDLE Process
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwSecureConnectPort(
	OUT PHANDLE PortHandle,
	IN PUNICODE_STRING PortName,
	IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	IN OUT PPORT_SECTION_WRITE WriteSection OPTIONAL,
	IN PSID ServerSid OPTIONAL,
	IN OUT PPORT_SECTION_READ ReadSection OPTIONAL,
	OUT PULONG MaxMessageSize OPTIONAL,
	IN OUT PVOID ConnectData OPTIONAL,
	IN OUT PULONG ConnectDataLength OPTIONAL
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwSetDefaultUILanguage(
	IN LANGID LanguageId
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwSetInformationJobObject(
	IN HANDLE JobHandle,
	IN JOBOBJECTINFOCLASS JobInformationClass,
	IN PVOID JobInformation,
	IN ULONG JobInformationLength
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwSetQuotaInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PFILE_USER_QUOTA_INFORMATION Buffer,
	IN ULONG BufferLength
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwSetUuidSeed(
	IN PUCHAR UuidSeed
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwSuspendProcess(
	IN HANDLE Process
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwTerminateJobObject(
	IN HANDLE JobHandle,
	IN NTSTATUS ExitStatus
	)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ZwWaitForKeyedEvent(
	IN HANDLE KeyedEventHandle,
	IN ULONG Requested,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Timeout OPTIONAL
	)
{
  return(FALSE);
}
