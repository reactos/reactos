; Machine generated, don't edit


SECTION .text

GLOBAL _NtAcceptConnectPort
GLOBAL _ZwAcceptConnectPort
_NtAcceptConnectPort:
_ZwAcceptConnectPort:
	mov	eax,0
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtAccessCheck
GLOBAL _ZwAccessCheck
_NtAccessCheck:
_ZwAccessCheck:
	mov	eax,1
	lea	edx,[esp+4]
	int	2Eh
	ret	32

GLOBAL _NtAccessCheckAndAuditAlarm
GLOBAL _ZwAccessCheckAndAuditAlarm
_NtAccessCheckAndAuditAlarm:
_ZwAccessCheckAndAuditAlarm:
	mov	eax,2
	lea	edx,[esp+4]
	int	2Eh
	ret	44

GLOBAL _NtAddAtom
GLOBAL _ZwAddAtom
_NtAddAtom:
_ZwAddAtom:
	mov	eax,3
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtAdjustGroupsToken
GLOBAL _ZwAdjustGroupsToken
_NtAdjustGroupsToken:
_ZwAdjustGroupsToken:
	mov	eax,4
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtAdjustPrivilegesToken
GLOBAL _ZwAdjustPrivilegesToken
_NtAdjustPrivilegesToken:
_ZwAdjustPrivilegesToken:
	mov	eax,5
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtAlertResumeThread
GLOBAL _ZwAlertResumeThread
_NtAlertResumeThread:
_ZwAlertResumeThread:
	mov	eax,6
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtAlertThread
GLOBAL _ZwAlertThread
_NtAlertThread:
_ZwAlertThread:
	mov	eax,7
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtAllocateLocallyUniqueId
GLOBAL _ZwAllocateLocallyUniqueId
_NtAllocateLocallyUniqueId:
_ZwAllocateLocallyUniqueId:
	mov	eax,8
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtAllocateUuids
GLOBAL _ZwAllocateUuids
_NtAllocateUuids:
_ZwAllocateUuids:
	mov	eax,9
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtAllocateVirtualMemory
GLOBAL _ZwAllocateVirtualMemory
_NtAllocateVirtualMemory:
_ZwAllocateVirtualMemory:
	mov	eax,10
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtCallbackReturn
GLOBAL _ZwCallbackReturn
_NtCallbackReturn:
_ZwCallbackReturn:
	mov	eax,11
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtCancelIoFile
GLOBAL _ZwCancelIoFile
_NtCancelIoFile:
_ZwCancelIoFile:
	mov	eax,12
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtCancelTimer
GLOBAL _ZwCancelTimer
_NtCancelTimer:
_ZwCancelTimer:
	mov	eax,13
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtClearEvent
GLOBAL _ZwClearEvent
_NtClearEvent:
_ZwClearEvent:
	mov	eax,14
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtClose
GLOBAL _ZwClose
_NtClose:
_ZwClose:
	mov	eax,15
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtCloseObjectAuditAlarm
GLOBAL _ZwCloseObjectAuditAlarm
_NtCloseObjectAuditAlarm:
_ZwCloseObjectAuditAlarm:
	mov	eax,16
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtCompleteConnectPort
GLOBAL _ZwCompleteConnectPort
_NtCompleteConnectPort:
_ZwCompleteConnectPort:
	mov	eax,17
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtConnectPort
GLOBAL _ZwConnectPort
_NtConnectPort:
_ZwConnectPort:
	mov	eax,18
	lea	edx,[esp+4]
	int	2Eh
	ret	32

GLOBAL _NtContinue
GLOBAL _ZwContinue
_NtContinue:
_ZwContinue:
	mov	eax,19
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtCreateDirectoryObject
GLOBAL _ZwCreateDirectoryObject
_NtCreateDirectoryObject:
_ZwCreateDirectoryObject:
	mov	eax,20
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtCreateEvent
GLOBAL _ZwCreateEvent
_NtCreateEvent:
_ZwCreateEvent:
	mov	eax,21
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtCreateEventPair
GLOBAL _ZwCreateEventPair
_NtCreateEventPair:
_ZwCreateEventPair:
	mov	eax,22
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtCreateFile
GLOBAL _ZwCreateFile
_NtCreateFile:
_ZwCreateFile:
	mov	eax,23
	lea	edx,[esp+4]
	int	2Eh
	ret	44

GLOBAL _NtCreateIoCompletion
GLOBAL _ZwCreateIoCompletion
_NtCreateIoCompletion:
_ZwCreateIoCompletion:
	mov	eax,24
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtCreateKey
GLOBAL _ZwCreateKey
_NtCreateKey:
_ZwCreateKey:
	mov	eax,25
	lea	edx,[esp+4]
	int	2Eh
	ret	28

GLOBAL _NtCreateMailslotFile
GLOBAL _ZwCreateMailslotFile
_NtCreateMailslotFile:
_ZwCreateMailslotFile:
	mov	eax,26
	lea	edx,[esp+4]
	int	2Eh
	ret	32

GLOBAL _NtCreateMutant
GLOBAL _ZwCreateMutant
_NtCreateMutant:
_ZwCreateMutant:
	mov	eax,27
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtCreateNamedPipeFile
GLOBAL _ZwCreateNamedPipeFile
_NtCreateNamedPipeFile:
_ZwCreateNamedPipeFile:
	mov	eax,28
	lea	edx,[esp+4]
	int	2Eh
	ret	56

GLOBAL _NtCreatePagingFile
GLOBAL _ZwCreatePagingFile
_NtCreatePagingFile:
_ZwCreatePagingFile:
	mov	eax,29
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtCreatePort
GLOBAL _ZwCreatePort
_NtCreatePort:
_ZwCreatePort:
	mov	eax,30
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtCreateProcess
GLOBAL _ZwCreateProcess
_NtCreateProcess:
_ZwCreateProcess:
	mov	eax,31
	lea	edx,[esp+4]
	int	2Eh
	ret	32

GLOBAL _NtCreateProfile
GLOBAL _ZwCreateProfile
_NtCreateProfile:
_ZwCreateProfile:
	mov	eax,32
	lea	edx,[esp+4]
	int	2Eh
	ret	36

GLOBAL _NtCreateSection
GLOBAL _ZwCreateSection
_NtCreateSection:
_ZwCreateSection:
	mov	eax,33
	lea	edx,[esp+4]
	int	2Eh
	ret	28

GLOBAL _NtCreateSemaphore
GLOBAL _ZwCreateSemaphore
_NtCreateSemaphore:
_ZwCreateSemaphore:
	mov	eax,34
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtCreateSymbolicLinkObject
GLOBAL _ZwCreateSymbolicLinkObject
_NtCreateSymbolicLinkObject:
_ZwCreateSymbolicLinkObject:
	mov	eax,35
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtCreateThread
GLOBAL _ZwCreateThread
_NtCreateThread:
_ZwCreateThread:
	mov	eax,36
	lea	edx,[esp+4]
	int	2Eh
	ret	32

GLOBAL _NtCreateTimer
GLOBAL _ZwCreateTimer
_NtCreateTimer:
_ZwCreateTimer:
	mov	eax,37
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtCreateToken
GLOBAL _ZwCreateToken
_NtCreateToken:
_ZwCreateToken:
	mov	eax,38
	lea	edx,[esp+4]
	int	2Eh
	ret	52

GLOBAL _NtDelayExecution
GLOBAL _ZwDelayExecution
_NtDelayExecution:
_ZwDelayExecution:
	mov	eax,39
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtDeleteAtom
GLOBAL _ZwDeleteAtom
_NtDeleteAtom:
_ZwDeleteAtom:
	mov	eax,40
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtDeleteFile
GLOBAL _ZwDeleteFile
_NtDeleteFile:
_ZwDeleteFile:
	mov	eax,41
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtDeleteKey
GLOBAL _ZwDeleteKey
_NtDeleteKey:
_ZwDeleteKey:
	mov	eax,42
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtDeleteObjectAuditAlarm
GLOBAL _ZwDeleteObjectAuditAlarm
_NtDeleteObjectAuditAlarm:
_ZwDeleteObjectAuditAlarm:
	mov	eax,43
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtDeleteValueKey
GLOBAL _ZwDeleteValueKey
_NtDeleteValueKey:
_ZwDeleteValueKey:
	mov	eax,44
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtDeviceIoControlFile
GLOBAL _ZwDeviceIoControlFile
_NtDeviceIoControlFile:
_ZwDeviceIoControlFile:
	mov	eax,45
	lea	edx,[esp+4]
	int	2Eh
	ret	40

GLOBAL _NtDisplayString
GLOBAL _ZwDisplayString
_NtDisplayString:
_ZwDisplayString:
	mov	eax,46
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtDuplicateObject
GLOBAL _ZwDuplicateObject
_NtDuplicateObject:
_ZwDuplicateObject:
	mov	eax,47
	lea	edx,[esp+4]
	int	2Eh
	ret	28

GLOBAL _NtDuplicateToken
GLOBAL _ZwDuplicateToken
_NtDuplicateToken:
_ZwDuplicateToken:
	mov	eax,48
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtEnumerateKey
GLOBAL _ZwEnumerateKey
_NtEnumerateKey:
_ZwEnumerateKey:
	mov	eax,49
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtEnumerateValueKey
GLOBAL _ZwEnumerateValueKey
_NtEnumerateValueKey:
_ZwEnumerateValueKey:
	mov	eax,50
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtExtendSection
GLOBAL _ZwExtendSection
_NtExtendSection:
_ZwExtendSection:
	mov	eax,51
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtFindAtom
GLOBAL _ZwFindAtom
_NtFindAtom:
_ZwFindAtom:
	mov	eax,52
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtFlushBuffersFile
GLOBAL _ZwFlushBuffersFile
_NtFlushBuffersFile:
_ZwFlushBuffersFile:
	mov	eax,53
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtFlushInstructionCache
GLOBAL _ZwFlushInstructionCache
_NtFlushInstructionCache:
_ZwFlushInstructionCache:
	mov	eax,54
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtFlushKey
GLOBAL _ZwFlushKey
_NtFlushKey:
_ZwFlushKey:
	mov	eax,55
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtFlushVirtualMemory
GLOBAL _ZwFlushVirtualMemory
_NtFlushVirtualMemory:
_ZwFlushVirtualMemory:
	mov	eax,56
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtFlushWriteBuffer
GLOBAL _ZwFlushWriteBuffer
_NtFlushWriteBuffer:
_ZwFlushWriteBuffer:
	mov	eax,57
	lea	edx,[esp+4]
	int	2Eh
	ret	0

GLOBAL _NtFreeVirtualMemory
GLOBAL _ZwFreeVirtualMemory
_NtFreeVirtualMemory:
_ZwFreeVirtualMemory:
	mov	eax,58
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtFsControlFile
GLOBAL _ZwFsControlFile
_NtFsControlFile:
_ZwFsControlFile:
	mov	eax,59
	lea	edx,[esp+4]
	int	2Eh
	ret	40

GLOBAL _NtGetContextThread
GLOBAL _ZwGetContextThread
_NtGetContextThread:
_ZwGetContextThread:
	mov	eax,60
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtGetPlugPlayEvent
GLOBAL _ZwGetPlugPlayEvent
_NtGetPlugPlayEvent:
_ZwGetPlugPlayEvent:
	mov	eax,61
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtGetTickCount
GLOBAL _ZwGetTickCount
_NtGetTickCount:
_ZwGetTickCount:
	mov	eax,62
	lea	edx,[esp+4]
	int	2Eh
	ret	0

GLOBAL _NtImpersonateClientOfPort
GLOBAL _ZwImpersonateClientOfPort
_NtImpersonateClientOfPort:
_ZwImpersonateClientOfPort:
	mov	eax,63
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtImpersonateThread
GLOBAL _ZwImpersonateThread
_NtImpersonateThread:
_ZwImpersonateThread:
	mov	eax,64
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtInitializeRegistry
GLOBAL _ZwInitializeRegistry
_NtInitializeRegistry:
_ZwInitializeRegistry:
	mov	eax,65
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtListenPort
GLOBAL _ZwListenPort
_NtListenPort:
_ZwListenPort:
	mov	eax,66
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtLoadDriver
GLOBAL _ZwLoadDriver
_NtLoadDriver:
_ZwLoadDriver:
	mov	eax,67
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtLoadKey
GLOBAL _ZwLoadKey
_NtLoadKey:
_ZwLoadKey:
	mov	eax,68
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtLoadKey2
GLOBAL _ZwLoadKey2
_NtLoadKey2:
_ZwLoadKey2:
	mov	eax,69
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtLockFile
GLOBAL _ZwLockFile
_NtLockFile:
_ZwLockFile:
	mov	eax,70
	lea	edx,[esp+4]
	int	2Eh
	ret	40

GLOBAL _NtLockVirtualMemory
GLOBAL _ZwLockVirtualMemory
_NtLockVirtualMemory:
_ZwLockVirtualMemory:
	mov	eax,71
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtMakeTemporaryObject
GLOBAL _ZwMakeTemporaryObject
_NtMakeTemporaryObject:
_ZwMakeTemporaryObject:
	mov	eax,72
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtMapViewOfSection
GLOBAL _ZwMapViewOfSection
_NtMapViewOfSection:
_ZwMapViewOfSection:
	mov	eax,73
	lea	edx,[esp+4]
	int	2Eh
	ret	40

GLOBAL _NtNotifyChangeDirectoryFile
GLOBAL _ZwNotifyChangeDirectoryFile
_NtNotifyChangeDirectoryFile:
_ZwNotifyChangeDirectoryFile:
	mov	eax,74
	lea	edx,[esp+4]
	int	2Eh
	ret	36

GLOBAL _NtNotifyChangeKey
GLOBAL _ZwNotifyChangeKey
_NtNotifyChangeKey:
_ZwNotifyChangeKey:
	mov	eax,75
	lea	edx,[esp+4]
	int	2Eh
	ret	40

GLOBAL _NtOpenDirectoryObject
GLOBAL _ZwOpenDirectoryObject
_NtOpenDirectoryObject:
_ZwOpenDirectoryObject:
	mov	eax,76
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtOpenEvent
GLOBAL _ZwOpenEvent
_NtOpenEvent:
_ZwOpenEvent:
	mov	eax,77
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtOpenEventPair
GLOBAL _ZwOpenEventPair
_NtOpenEventPair:
_ZwOpenEventPair:
	mov	eax,78
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtOpenFile
GLOBAL _ZwOpenFile
_NtOpenFile:
_ZwOpenFile:
	mov	eax,79
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtOpenIoCompletion
GLOBAL _ZwOpenIoCompletion
_NtOpenIoCompletion:
_ZwOpenIoCompletion:
	mov	eax,80
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtOpenKey
GLOBAL _ZwOpenKey
_NtOpenKey:
_ZwOpenKey:
	mov	eax,81
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtOpenMutant
GLOBAL _ZwOpenMutant
_NtOpenMutant:
_ZwOpenMutant:
	mov	eax,82
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtOpenObjectAuditAlarm
GLOBAL _ZwOpenObjectAuditAlarm
_NtOpenObjectAuditAlarm:
_ZwOpenObjectAuditAlarm:
	mov	eax,83
	lea	edx,[esp+4]
	int	2Eh
	ret	48

GLOBAL _NtOpenProcess
GLOBAL _ZwOpenProcess
_NtOpenProcess:
_ZwOpenProcess:
	mov	eax,84
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtOpenProcessToken
GLOBAL _ZwOpenProcessToken
_NtOpenProcessToken:
_ZwOpenProcessToken:
	mov	eax,85
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtOpenSection
GLOBAL _ZwOpenSection
_NtOpenSection:
_ZwOpenSection:
	mov	eax,86
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtOpenSemaphore
GLOBAL _ZwOpenSemaphore
_NtOpenSemaphore:
_ZwOpenSemaphore:
	mov	eax,87
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtOpenSymbolicLinkObject
GLOBAL _ZwOpenSymbolicLinkObject
_NtOpenSymbolicLinkObject:
_ZwOpenSymbolicLinkObject:
	mov	eax,88
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtOpenThread
GLOBAL _ZwOpenThread
_NtOpenThread:
_ZwOpenThread:
	mov	eax,89
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtOpenThreadToken
GLOBAL _ZwOpenThreadToken
_NtOpenThreadToken:
_ZwOpenThreadToken:
	mov	eax,90
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtOpenTimer
GLOBAL _ZwOpenTimer
_NtOpenTimer:
_ZwOpenTimer:
	mov	eax,91
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtPlugPlayControl
GLOBAL _ZwPlugPlayControl
_NtPlugPlayControl:
_ZwPlugPlayControl:
	mov	eax,92
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtPrivilegeCheck
GLOBAL _ZwPrivilegeCheck
_NtPrivilegeCheck:
_ZwPrivilegeCheck:
	mov	eax,93
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtPrivilegedServiceAuditAlarm
GLOBAL _ZwPrivilegedServiceAuditAlarm
_NtPrivilegedServiceAuditAlarm:
_ZwPrivilegedServiceAuditAlarm:
	mov	eax,94
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtPrivilegeObjectAuditAlarm
GLOBAL _ZwPrivilegeObjectAuditAlarm
_NtPrivilegeObjectAuditAlarm:
_ZwPrivilegeObjectAuditAlarm:
	mov	eax,95
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtProtectVirtualMemory
GLOBAL _ZwProtectVirtualMemory
_NtProtectVirtualMemory:
_ZwProtectVirtualMemory:
	mov	eax,96
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtPulseEvent
GLOBAL _ZwPulseEvent
_NtPulseEvent:
_ZwPulseEvent:
	mov	eax,97
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtQueryInformationAtom
GLOBAL _ZwQueryInformationAtom
_NtQueryInformationAtom:
_ZwQueryInformationAtom:
	mov	eax,98
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtQueryAttributesFile
GLOBAL _ZwQueryAttributesFile
_NtQueryAttributesFile:
_ZwQueryAttributesFile:
	mov	eax,99
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtQueryDefaultLocale
GLOBAL _ZwQueryDefaultLocale
_NtQueryDefaultLocale:
_ZwQueryDefaultLocale:
	mov	eax,100
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtQueryDirectoryFile
GLOBAL _ZwQueryDirectoryFile
_NtQueryDirectoryFile:
_ZwQueryDirectoryFile:
	mov	eax,101
	lea	edx,[esp+4]
	int	2Eh
	ret	44

GLOBAL _NtQueryDirectoryObject
GLOBAL _ZwQueryDirectoryObject
_NtQueryDirectoryObject:
_ZwQueryDirectoryObject:
	mov	eax,102
	lea	edx,[esp+4]
	int	2Eh
	ret	28

GLOBAL _NtQueryEaFile
GLOBAL _ZwQueryEaFile
_NtQueryEaFile:
_ZwQueryEaFile:
	mov	eax,103
	lea	edx,[esp+4]
	int	2Eh
	ret	36

GLOBAL _NtQueryEvent
GLOBAL _ZwQueryEvent
_NtQueryEvent:
_ZwQueryEvent:
	mov	eax,104
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtQueryFullAttributesFile
GLOBAL _ZwQueryFullAttributesFile
_NtQueryFullAttributesFile:
_ZwQueryFullAttributesFile:
	mov	eax,105
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtQueryInformationFile
GLOBAL _ZwQueryInformationFile
_NtQueryInformationFile:
_ZwQueryInformationFile:
	mov	eax,106
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtQueryIoCompletion
GLOBAL _ZwQueryIoCompletion
_NtQueryIoCompletion:
_ZwQueryIoCompletion:
	mov	eax,107
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtQueryInformationPort
GLOBAL _ZwQueryInformationPort
_NtQueryInformationPort:
_ZwQueryInformationPort:
	mov	eax,108
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtQueryInformationProcess
GLOBAL _ZwQueryInformationProcess
_NtQueryInformationProcess:
_ZwQueryInformationProcess:
	mov	eax,109
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtQueryInformationThread
GLOBAL _ZwQueryInformationThread
_NtQueryInformationThread:
_ZwQueryInformationThread:
	mov	eax,110
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtQueryInformationToken
GLOBAL _ZwQueryInformationToken
_NtQueryInformationToken:
_ZwQueryInformationToken:
	mov	eax,111
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtQueryIntervalProfile
GLOBAL _ZwQueryIntervalProfile
_NtQueryIntervalProfile:
_ZwQueryIntervalProfile:
	mov	eax,112
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtQueryKey
GLOBAL _ZwQueryKey
_NtQueryKey:
_ZwQueryKey:
	mov	eax,113
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtQueryMultipleValueKey
GLOBAL _ZwQueryMultipleValueKey
_NtQueryMultipleValueKey:
_ZwQueryMultipleValueKey:
	mov	eax,114
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtQueryMutant
GLOBAL _ZwQueryMutant
_NtQueryMutant:
_ZwQueryMutant:
	mov	eax,115
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtQueryObject
GLOBAL _ZwQueryObject
_NtQueryObject:
_ZwQueryObject:
	mov	eax,116
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtQueryOleDirectoryFile
GLOBAL _ZwQueryOleDirectoryFile
_NtQueryOleDirectoryFile:
_ZwQueryOleDirectoryFile:
	mov	eax,117
	lea	edx,[esp+4]
	int	2Eh
	ret	44

GLOBAL _NtQueryPerformanceCounter
GLOBAL _ZwQueryPerformanceCounter
_NtQueryPerformanceCounter:
_ZwQueryPerformanceCounter:
	mov	eax,118
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtQuerySection
GLOBAL _ZwQuerySection
_NtQuerySection:
_ZwQuerySection:
	mov	eax,119
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtQuerySecurityObject
GLOBAL _ZwQuerySecurityObject
_NtQuerySecurityObject:
_ZwQuerySecurityObject:
	mov	eax,120
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtQuerySemaphore
GLOBAL _ZwQuerySemaphore
_NtQuerySemaphore:
_ZwQuerySemaphore:
	mov	eax,121
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtQuerySymbolicLinkObject
GLOBAL _ZwQuerySymbolicLinkObject
_NtQuerySymbolicLinkObject:
_ZwQuerySymbolicLinkObject:
	mov	eax,122
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtQuerySystemEnvironmentValue
GLOBAL _ZwQuerySystemEnvironmentValue
_NtQuerySystemEnvironmentValue:
_ZwQuerySystemEnvironmentValue:
	mov	eax,123
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtQuerySystemInformation
GLOBAL _ZwQuerySystemInformation
_NtQuerySystemInformation:
_ZwQuerySystemInformation:
	mov	eax,124
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtQuerySystemTime
GLOBAL _ZwQuerySystemTime
_NtQuerySystemTime:
_ZwQuerySystemTime:
	mov	eax,125
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtQueryTimer
GLOBAL _ZwQueryTimer
_NtQueryTimer:
_ZwQueryTimer:
	mov	eax,126
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtQueryTimerResolution
GLOBAL _ZwQueryTimerResolution
_NtQueryTimerResolution:
_ZwQueryTimerResolution:
	mov	eax,127
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtQueryValueKey
GLOBAL _ZwQueryValueKey
_NtQueryValueKey:
_ZwQueryValueKey:
	mov	eax,128
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtQueryVirtualMemory
GLOBAL _ZwQueryVirtualMemory
_NtQueryVirtualMemory:
_ZwQueryVirtualMemory:
	mov	eax,129
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtQueryVolumeInformationFile
GLOBAL _ZwQueryVolumeInformationFile
_NtQueryVolumeInformationFile:
_ZwQueryVolumeInformationFile:
	mov	eax,130
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtQueueApcThread
GLOBAL _ZwQueueApcThread
_NtQueueApcThread:
_ZwQueueApcThread:
	mov	eax,131
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtRaiseException
GLOBAL _ZwRaiseException
_NtRaiseException:
_ZwRaiseException:
	mov	eax,132
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtRaiseHardError
GLOBAL _ZwRaiseHardError
_NtRaiseHardError:
_ZwRaiseHardError:
	mov	eax,133
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtReadFile
GLOBAL _ZwReadFile
_NtReadFile:
_ZwReadFile:
	mov	eax,134
	lea	edx,[esp+4]
	int	2Eh
	ret	36

GLOBAL _NtReadFileScatter
GLOBAL _ZwReadFileScatter
_NtReadFileScatter:
_ZwReadFileScatter:
	mov	eax,135
	lea	edx,[esp+4]
	int	2Eh
	ret	36

GLOBAL _NtReadRequestData
GLOBAL _ZwReadRequestData
_NtReadRequestData:
_ZwReadRequestData:
	mov	eax,136
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtReadVirtualMemory
GLOBAL _ZwReadVirtualMemory
_NtReadVirtualMemory:
_ZwReadVirtualMemory:
	mov	eax,137
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtRegisterThreadTerminatePort
GLOBAL _ZwRegisterThreadTerminatePort
_NtRegisterThreadTerminatePort:
_ZwRegisterThreadTerminatePort:
	mov	eax,138
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtReleaseMutant
GLOBAL _ZwReleaseMutant
_NtReleaseMutant:
_ZwReleaseMutant:
	mov	eax,139
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtReleaseSemaphore
GLOBAL _ZwReleaseSemaphore
_NtReleaseSemaphore:
_ZwReleaseSemaphore:
	mov	eax,140
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtRemoveIoCompletion
GLOBAL _ZwRemoveIoCompletion
_NtRemoveIoCompletion:
_ZwRemoveIoCompletion:
	mov	eax,141
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtReplaceKey
GLOBAL _ZwReplaceKey
_NtReplaceKey:
_ZwReplaceKey:
	mov	eax,142
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtReplyPort
GLOBAL _ZwReplyPort
_NtReplyPort:
_ZwReplyPort:
	mov	eax,143
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtReplyWaitReceivePort
GLOBAL _ZwReplyWaitReceivePort
_NtReplyWaitReceivePort:
_ZwReplyWaitReceivePort:
	mov	eax,144
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtReplyWaitReplyPort
GLOBAL _ZwReplyWaitReplyPort
_NtReplyWaitReplyPort:
_ZwReplyWaitReplyPort:
	mov	eax,145
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtRequestPort
GLOBAL _ZwRequestPort
_NtRequestPort:
_ZwRequestPort:
	mov	eax,146
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtRequestWaitReplyPort
GLOBAL _ZwRequestWaitReplyPort
_NtRequestWaitReplyPort:
_ZwRequestWaitReplyPort:
	mov	eax,147
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtResetEvent
GLOBAL _ZwResetEvent
_NtResetEvent:
_ZwResetEvent:
	mov	eax,148
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtRestoreKey
GLOBAL _ZwRestoreKey
_NtRestoreKey:
_ZwRestoreKey:
	mov	eax,149
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtResumeThread
GLOBAL _ZwResumeThread
_NtResumeThread:
_ZwResumeThread:
	mov	eax,150
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtSaveKey
GLOBAL _ZwSaveKey
_NtSaveKey:
_ZwSaveKey:
	mov	eax,151
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtSetIoCompletion
GLOBAL _ZwSetIoCompletion
_NtSetIoCompletion:
_ZwSetIoCompletion:
	mov	eax,152
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtSetContextThread
GLOBAL _ZwSetContextThread
_NtSetContextThread:
_ZwSetContextThread:
	mov	eax,153
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtSetDefaultHardErrorPort
GLOBAL _ZwSetDefaultHardErrorPort
_NtSetDefaultHardErrorPort:
_ZwSetDefaultHardErrorPort:
	mov	eax,154
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtSetDefaultLocale
GLOBAL _ZwSetDefaultLocale
_NtSetDefaultLocale:
_ZwSetDefaultLocale:
	mov	eax,155
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtSetEaFile
GLOBAL _ZwSetEaFile
_NtSetEaFile:
_ZwSetEaFile:
	mov	eax,156
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtSetEvent
GLOBAL _ZwSetEvent
_NtSetEvent:
_ZwSetEvent:
	mov	eax,157
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtSetHighEventPair
GLOBAL _ZwSetHighEventPair
_NtSetHighEventPair:
_ZwSetHighEventPair:
	mov	eax,158
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtSetHighWaitLowEventPair
GLOBAL _ZwSetHighWaitLowEventPair
_NtSetHighWaitLowEventPair:
_ZwSetHighWaitLowEventPair:
	mov	eax,159
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtSetInformationFile
GLOBAL _ZwSetInformationFile
_NtSetInformationFile:
_ZwSetInformationFile:
	mov	eax,160
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtSetInformationKey
GLOBAL _ZwSetInformationKey
_NtSetInformationKey:
_ZwSetInformationKey:
	mov	eax,161
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtSetInformationObject
GLOBAL _ZwSetInformationObject
_NtSetInformationObject:
_ZwSetInformationObject:
	mov	eax,162
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtSetInformationProcess
GLOBAL _ZwSetInformationProcess
_NtSetInformationProcess:
_ZwSetInformationProcess:
	mov	eax,163
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtSetInformationThread
GLOBAL _ZwSetInformationThread
_NtSetInformationThread:
_ZwSetInformationThread:
	mov	eax,164
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtSetInformationToken
GLOBAL _ZwSetInformationToken
_NtSetInformationToken:
_ZwSetInformationToken:
	mov	eax,165
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtSetIntervalProfile
GLOBAL _ZwSetIntervalProfile
_NtSetIntervalProfile:
_ZwSetIntervalProfile:
	mov	eax,166
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtSetLdtEntries
GLOBAL _ZwSetLdtEntries
_NtSetLdtEntries:
_ZwSetLdtEntries:
	mov	eax,167
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtSetLowEventPair
GLOBAL _ZwSetLowEventPair
_NtSetLowEventPair:
_ZwSetLowEventPair:
	mov	eax,168
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtSetLowWaitHighEventPair
GLOBAL _ZwSetLowWaitHighEventPair
_NtSetLowWaitHighEventPair:
_ZwSetLowWaitHighEventPair:
	mov	eax,169
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtSetSecurityObject
GLOBAL _ZwSetSecurityObject
_NtSetSecurityObject:
_ZwSetSecurityObject:
	mov	eax,170
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtSetSystemEnvironmentValue
GLOBAL _ZwSetSystemEnvironmentValue
_NtSetSystemEnvironmentValue:
_ZwSetSystemEnvironmentValue:
	mov	eax,171
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtSetSystemInformation
GLOBAL _ZwSetSystemInformation
_NtSetSystemInformation:
_ZwSetSystemInformation:
	mov	eax,172
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtSetSystemPowerState
GLOBAL _ZwSetSystemPowerState
_NtSetSystemPowerState:
_ZwSetSystemPowerState:
	mov	eax,173
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtSetSystemTime
GLOBAL _ZwSetSystemTime
_NtSetSystemTime:
_ZwSetSystemTime:
	mov	eax,174
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtSetTimer
GLOBAL _ZwSetTimer
_NtSetTimer:
_ZwSetTimer:
	mov	eax,175
	lea	edx,[esp+4]
	int	2Eh
	ret	28

GLOBAL _NtSetTimerResolution
GLOBAL _ZwSetTimerResolution
_NtSetTimerResolution:
_ZwSetTimerResolution:
	mov	eax,176
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtSetValueKey
GLOBAL _ZwSetValueKey
_NtSetValueKey:
_ZwSetValueKey:
	mov	eax,177
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtSetVolumeInformationFile
GLOBAL _ZwSetVolumeInformationFile
_NtSetVolumeInformationFile:
_ZwSetVolumeInformationFile:
	mov	eax,178
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtShutdownSystem
GLOBAL _ZwShutdownSystem
_NtShutdownSystem:
_ZwShutdownSystem:
	mov	eax,179
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtSignalAndWaitForSingleObject
GLOBAL _ZwSignalAndWaitForSingleObject
_NtSignalAndWaitForSingleObject:
_ZwSignalAndWaitForSingleObject:
	mov	eax,180
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtStartProfile
GLOBAL _ZwStartProfile
_NtStartProfile:
_ZwStartProfile:
	mov	eax,181
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtStopProfile
GLOBAL _ZwStopProfile
_NtStopProfile:
_ZwStopProfile:
	mov	eax,182
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtSuspendThread
GLOBAL _ZwSuspendThread
_NtSuspendThread:
_ZwSuspendThread:
	mov	eax,183
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtSystemDebugControl
GLOBAL _ZwSystemDebugControl
_NtSystemDebugControl:
_ZwSystemDebugControl:
	mov	eax,184
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtTerminateProcess
GLOBAL _ZwTerminateProcess
_NtTerminateProcess:
_ZwTerminateProcess:
	mov	eax,185
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtTerminateThread
GLOBAL _ZwTerminateThread
_NtTerminateThread:
_ZwTerminateThread:
	mov	eax,186
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtTestAlert
GLOBAL _ZwTestAlert
_NtTestAlert:
_ZwTestAlert:
	mov	eax,187
	lea	edx,[esp+4]
	int	2Eh
	ret	0

GLOBAL _NtUnloadDriver
GLOBAL _ZwUnloadDriver
_NtUnloadDriver:
_ZwUnloadDriver:
	mov	eax,188
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtUnloadKey
GLOBAL _ZwUnloadKey
_NtUnloadKey:
_ZwUnloadKey:
	mov	eax,189
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtUnlockFile
GLOBAL _ZwUnlockFile
_NtUnlockFile:
_ZwUnlockFile:
	mov	eax,190
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtUnlockVirtualMemory
GLOBAL _ZwUnlockVirtualMemory
_NtUnlockVirtualMemory:
_ZwUnlockVirtualMemory:
	mov	eax,191
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtUnmapViewOfSection
GLOBAL _ZwUnmapViewOfSection
_NtUnmapViewOfSection:
_ZwUnmapViewOfSection:
	mov	eax,192
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtVdmControl
GLOBAL _ZwVdmControl
_NtVdmControl:
_ZwVdmControl:
	mov	eax,193
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtWaitForMultipleObjects
GLOBAL _ZwWaitForMultipleObjects
_NtWaitForMultipleObjects:
_ZwWaitForMultipleObjects:
	mov	eax,194
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtWaitForSingleObject
GLOBAL _ZwWaitForSingleObject
_NtWaitForSingleObject:
_ZwWaitForSingleObject:
	mov	eax,195
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtWaitHighEventPair
GLOBAL _ZwWaitHighEventPair
_NtWaitHighEventPair:
_ZwWaitHighEventPair:
	mov	eax,196
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtWaitLowEventPair
GLOBAL _ZwWaitLowEventPair
_NtWaitLowEventPair:
_ZwWaitLowEventPair:
	mov	eax,197
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtWriteFile
GLOBAL _ZwWriteFile
_NtWriteFile:
_ZwWriteFile:
	mov	eax,198
	lea	edx,[esp+4]
	int	2Eh
	ret	36

GLOBAL _NtWriteFileGather
GLOBAL _ZwWriteFileGather
_NtWriteFileGather:
_ZwWriteFileGather:
	mov	eax,199
	lea	edx,[esp+4]
	int	2Eh
	ret	36

GLOBAL _NtWriteRequestData
GLOBAL _ZwWriteRequestData
_NtWriteRequestData:
_ZwWriteRequestData:
	mov	eax,200
	lea	edx,[esp+4]
	int	2Eh
	ret	24

GLOBAL _NtWriteVirtualMemory
GLOBAL _ZwWriteVirtualMemory
_NtWriteVirtualMemory:
_ZwWriteVirtualMemory:
	mov	eax,201
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtW32Call
GLOBAL _ZwW32Call
_NtW32Call:
_ZwW32Call:
	mov	eax,202
	lea	edx,[esp+4]
	int	2Eh
	ret	20

GLOBAL _NtCreateChannel
GLOBAL _ZwCreateChannel
_NtCreateChannel:
_ZwCreateChannel:
	mov	eax,203
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtListenChannel
GLOBAL _ZwListenChannel
_NtListenChannel:
_ZwListenChannel:
	mov	eax,204
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtOpenChannel
GLOBAL _ZwOpenChannel
_NtOpenChannel:
_ZwOpenChannel:
	mov	eax,205
	lea	edx,[esp+4]
	int	2Eh
	ret	8

GLOBAL _NtReplyWaitSendChannel
GLOBAL _ZwReplyWaitSendChannel
_NtReplyWaitSendChannel:
_ZwReplyWaitSendChannel:
	mov	eax,206
	lea	edx,[esp+4]
	int	2Eh
	ret	12

GLOBAL _NtSendWaitReplyChannel
GLOBAL _ZwSendWaitReplyChannel
_NtSendWaitReplyChannel:
_ZwSendWaitReplyChannel:
	mov	eax,207
	lea	edx,[esp+4]
	int	2Eh
	ret	16

GLOBAL _NtSetContextChannel
GLOBAL _ZwSetContextChannel
_NtSetContextChannel:
_ZwSetContextChannel:
	mov	eax,208
	lea	edx,[esp+4]
	int	2Eh
	ret	4

GLOBAL _NtYieldExecution
GLOBAL _ZwYieldExecution
_NtYieldExecution:
_ZwYieldExecution:
	mov	eax,209
	lea	edx,[esp+4]
	int	2Eh
	ret	0

