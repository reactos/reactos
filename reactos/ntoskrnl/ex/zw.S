/* FILE:            System Call Stubs for Native API
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         ntoskrnl/ex/zw.S
 * PROGRAMMER:      Computer Generated File. See tools/nci/ncitool.c
 * REMARK:          DO NOT EDIT OR COMMIT MODIFICATIONS TO THIS FILE
 */


#include <ndk/asm.h>

#include <ndk/asm.h>

	.globl ZwAcceptConnectPort
ZwAcceptConnectPort:
    bl KiSystemService
    rfi
	.globl ZwAccessCheck
ZwAccessCheck:
    bl KiSystemService
    rfi
	.globl ZwAccessCheckAndAuditAlarm
ZwAccessCheckAndAuditAlarm:
    bl KiSystemService
    rfi
	.globl ZwAddAtom
ZwAddAtom:
    bl KiSystemService
    rfi
	.globl ZwAddBootEntry
ZwAddBootEntry:
    bl KiSystemService
    rfi
	.globl ZwAdjustGroupsToken
ZwAdjustGroupsToken:
    bl KiSystemService
    rfi
	.globl ZwAdjustPrivilegesToken
ZwAdjustPrivilegesToken:
    bl KiSystemService
    rfi
	.globl ZwAlertResumeThread
ZwAlertResumeThread:
    bl KiSystemService
    rfi
	.globl ZwAlertThread
ZwAlertThread:
    bl KiSystemService
    rfi
	.globl ZwAllocateLocallyUniqueId
ZwAllocateLocallyUniqueId:
    bl KiSystemService
    rfi
	.globl ZwAllocateUuids
ZwAllocateUuids:
    bl KiSystemService
    rfi
	.globl ZwAllocateVirtualMemory
ZwAllocateVirtualMemory:
    bl KiSystemService
    rfi
	.globl ZwAssignProcessToJobObject
ZwAssignProcessToJobObject:
    bl KiSystemService
    rfi
	.globl ZwCallbackReturn
ZwCallbackReturn:
    bl KiSystemService
    rfi
	.globl ZwCancelIoFile
ZwCancelIoFile:
    bl KiSystemService
    rfi
	.globl ZwCancelTimer
ZwCancelTimer:
    bl KiSystemService
    rfi
	.globl ZwClearEvent
ZwClearEvent:
    bl KiSystemService
    rfi
	.globl ZwClose
ZwClose:
    bl KiSystemService
    rfi
	.globl ZwCloseObjectAuditAlarm
ZwCloseObjectAuditAlarm:
    bl KiSystemService
    rfi
	.globl ZwCompleteConnectPort
ZwCompleteConnectPort:
    bl KiSystemService
    rfi
	.globl ZwConnectPort
ZwConnectPort:
    bl KiSystemService
    rfi
	.globl ZwContinue
ZwContinue:
    bl KiSystemService
    rfi
	.globl ZwCreateDebugObject
ZwCreateDebugObject:
    bl KiSystemService
    rfi
	.globl ZwCreateDirectoryObject
ZwCreateDirectoryObject:
    bl KiSystemService
    rfi
	.globl ZwCreateEvent
ZwCreateEvent:
    bl KiSystemService
    rfi
	.globl ZwCreateEventPair
ZwCreateEventPair:
    bl KiSystemService
    rfi
	.globl ZwCreateFile
ZwCreateFile:
    bl KiSystemService
    rfi
	.globl ZwCreateIoCompletion
ZwCreateIoCompletion:
    bl KiSystemService
    rfi
	.globl ZwCreateJobObject
ZwCreateJobObject:
    bl KiSystemService
    rfi
	.globl ZwCreateKey
ZwCreateKey:
    bl KiSystemService
    rfi
	.globl ZwCreateMailslotFile
ZwCreateMailslotFile:
    bl KiSystemService
    rfi
	.globl ZwCreateMutant
ZwCreateMutant:
    bl KiSystemService
    rfi
	.globl ZwCreateNamedPipeFile
ZwCreateNamedPipeFile:
    bl KiSystemService
    rfi
	.globl ZwCreatePagingFile
ZwCreatePagingFile:
    bl KiSystemService
    rfi
	.globl ZwCreatePort
ZwCreatePort:
    bl KiSystemService
    rfi
	.globl ZwCreateProcess
ZwCreateProcess:
    bl KiSystemService
    rfi
	.globl ZwCreateProfile
ZwCreateProfile:
    bl KiSystemService
    rfi
	.globl ZwCreateSection
ZwCreateSection:
    bl KiSystemService
    rfi
	.globl ZwCreateSemaphore
ZwCreateSemaphore:
    bl KiSystemService
    rfi
	.globl ZwCreateSymbolicLinkObject
ZwCreateSymbolicLinkObject:
    bl KiSystemService
    rfi
	.globl ZwCreateThread
ZwCreateThread:
    bl KiSystemService
    rfi
	.globl ZwCreateTimer
ZwCreateTimer:
    bl KiSystemService
    rfi
	.globl ZwCreateToken
ZwCreateToken:
    bl KiSystemService
    rfi
	.globl ZwCreateWaitablePort
ZwCreateWaitablePort:
    bl KiSystemService
    rfi
	.globl ZwDebugActiveProcess
ZwDebugActiveProcess:
    bl KiSystemService
    rfi
	.globl ZwDebugContinue
ZwDebugContinue:
    bl KiSystemService
    rfi
	.globl ZwDelayExecution
ZwDelayExecution:
    bl KiSystemService
    rfi
	.globl ZwDeleteAtom
ZwDeleteAtom:
    bl KiSystemService
    rfi
	.globl ZwDeleteBootEntry
ZwDeleteBootEntry:
    bl KiSystemService
    rfi
	.globl ZwDeleteFile
ZwDeleteFile:
    bl KiSystemService
    rfi
	.globl ZwDeleteKey
ZwDeleteKey:
    bl KiSystemService
    rfi
	.globl ZwDeleteObjectAuditAlarm
ZwDeleteObjectAuditAlarm:
    bl KiSystemService
    rfi
	.globl ZwDeleteValueKey
ZwDeleteValueKey:
    bl KiSystemService
    rfi
	.globl ZwDeviceIoControlFile
ZwDeviceIoControlFile:
    bl KiSystemService
    rfi
	.globl ZwDisplayString
ZwDisplayString:
    bl KiSystemService
    rfi
	.globl ZwDuplicateObject
ZwDuplicateObject:
    bl KiSystemService
    rfi
	.globl ZwDuplicateToken
ZwDuplicateToken:
    bl KiSystemService
    rfi
	.globl ZwEnumerateBootEntries
ZwEnumerateBootEntries:
    bl KiSystemService
    rfi
	.globl ZwEnumerateKey
ZwEnumerateKey:
    bl KiSystemService
    rfi
	.globl ZwEnumerateValueKey
ZwEnumerateValueKey:
    bl KiSystemService
    rfi
	.globl ZwExtendSection
ZwExtendSection:
    bl KiSystemService
    rfi
	.globl ZwFindAtom
ZwFindAtom:
    bl KiSystemService
    rfi
	.globl ZwFlushBuffersFile
ZwFlushBuffersFile:
    bl KiSystemService
    rfi
	.globl ZwFlushInstructionCache
ZwFlushInstructionCache:
    bl KiSystemService
    rfi
	.globl ZwFlushKey
ZwFlushKey:
    bl KiSystemService
    rfi
	.globl ZwFlushVirtualMemory
ZwFlushVirtualMemory:
    bl KiSystemService
    rfi
	.globl ZwFlushWriteBuffer
ZwFlushWriteBuffer:
    bl KiSystemService
    rfi
	.globl ZwFreeVirtualMemory
ZwFreeVirtualMemory:
    bl KiSystemService
    rfi
	.globl ZwFsControlFile
ZwFsControlFile:
    bl KiSystemService
    rfi
	.globl ZwGetContextThread
ZwGetContextThread:
    bl KiSystemService
    rfi
	.globl ZwGetPlugPlayEvent
ZwGetPlugPlayEvent:
    bl KiSystemService
    rfi
	.globl ZwGetTickCount
ZwGetTickCount:
    bl KiSystemService
    rfi
	.globl ZwImpersonateAnonymousToken
ZwImpersonateAnonymousToken:
    bl KiSystemService
    rfi
	.globl ZwImpersonateClientOfPort
ZwImpersonateClientOfPort:
    bl KiSystemService
    rfi
	.globl ZwImpersonateThread
ZwImpersonateThread:
    bl KiSystemService
    rfi
	.globl ZwInitializeRegistry
ZwInitializeRegistry:
    bl KiSystemService
    rfi
	.globl ZwInitiatePowerAction
ZwInitiatePowerAction:
    bl KiSystemService
    rfi
	.globl ZwIsProcessInJob
ZwIsProcessInJob:
    bl KiSystemService
    rfi
	.globl ZwListenPort
ZwListenPort:
    bl KiSystemService
    rfi
	.globl ZwLoadDriver
ZwLoadDriver:
    bl KiSystemService
    rfi
	.globl ZwLoadKey
ZwLoadKey:
    bl KiSystemService
    rfi
	.globl ZwLoadKey2
ZwLoadKey2:
    bl KiSystemService
    rfi
	.globl ZwLockFile
ZwLockFile:
    bl KiSystemService
    rfi
	.globl ZwLockVirtualMemory
ZwLockVirtualMemory:
    bl KiSystemService
    rfi
	.globl ZwMakePermanentObject
ZwMakePermanentObject:
    bl KiSystemService
    rfi
	.globl ZwMakeTemporaryObject
ZwMakeTemporaryObject:
    bl KiSystemService
    rfi
	.globl ZwMapViewOfSection
ZwMapViewOfSection:
    bl KiSystemService
    rfi
	.globl ZwNotifyChangeDirectoryFile
ZwNotifyChangeDirectoryFile:
    bl KiSystemService
    rfi
	.globl ZwNotifyChangeKey
ZwNotifyChangeKey:
    bl KiSystemService
    rfi
	.globl ZwOpenDirectoryObject
ZwOpenDirectoryObject:
    bl KiSystemService
    rfi
	.globl ZwOpenEvent
ZwOpenEvent:
    bl KiSystemService
    rfi
	.globl ZwOpenEventPair
ZwOpenEventPair:
    bl KiSystemService
    rfi
	.globl ZwOpenFile
ZwOpenFile:
    bl KiSystemService
    rfi
	.globl ZwOpenIoCompletion
ZwOpenIoCompletion:
    bl KiSystemService
    rfi
	.globl ZwOpenJobObject
ZwOpenJobObject:
    bl KiSystemService
    rfi
	.globl ZwOpenKey
ZwOpenKey:
    bl KiSystemService
    rfi
	.globl ZwOpenMutant
ZwOpenMutant:
    bl KiSystemService
    rfi
	.globl ZwOpenObjectAuditAlarm
ZwOpenObjectAuditAlarm:
    bl KiSystemService
    rfi
	.globl ZwOpenProcess
ZwOpenProcess:
    bl KiSystemService
    rfi
	.globl ZwOpenProcessToken
ZwOpenProcessToken:
    bl KiSystemService
    rfi
	.globl ZwOpenProcessTokenEx
ZwOpenProcessTokenEx:
    bl KiSystemService
    rfi
	.globl ZwOpenSection
ZwOpenSection:
    bl KiSystemService
    rfi
	.globl ZwOpenSemaphore
ZwOpenSemaphore:
    bl KiSystemService
    rfi
	.globl ZwOpenSymbolicLinkObject
ZwOpenSymbolicLinkObject:
    bl KiSystemService
    rfi
	.globl ZwOpenThread
ZwOpenThread:
    bl KiSystemService
    rfi
	.globl ZwOpenThreadToken
ZwOpenThreadToken:
    bl KiSystemService
    rfi
	.globl ZwOpenThreadTokenEx
ZwOpenThreadTokenEx:
    bl KiSystemService
    rfi
	.globl ZwOpenTimer
ZwOpenTimer:
    bl KiSystemService
    rfi
	.globl ZwPlugPlayControl
ZwPlugPlayControl:
    bl KiSystemService
    rfi
	.globl ZwPowerInformation
ZwPowerInformation:
    bl KiSystemService
    rfi
	.globl ZwPrivilegeCheck
ZwPrivilegeCheck:
    bl KiSystemService
    rfi
	.globl ZwPrivilegedServiceAuditAlarm
ZwPrivilegedServiceAuditAlarm:
    bl KiSystemService
    rfi
	.globl ZwPrivilegeObjectAuditAlarm
ZwPrivilegeObjectAuditAlarm:
    bl KiSystemService
    rfi
	.globl ZwProtectVirtualMemory
ZwProtectVirtualMemory:
    bl KiSystemService
    rfi
	.globl ZwPulseEvent
ZwPulseEvent:
    bl KiSystemService
    rfi
	.globl ZwQueryInformationAtom
ZwQueryInformationAtom:
    bl KiSystemService
    rfi
	.globl ZwQueryAttributesFile
ZwQueryAttributesFile:
    bl KiSystemService
    rfi
	.globl ZwQueryBootEntryOrder
ZwQueryBootEntryOrder:
    bl KiSystemService
    rfi
	.globl ZwQueryBootOptions
ZwQueryBootOptions:
    bl KiSystemService
    rfi
	.globl ZwQueryDebugFilterState
ZwQueryDebugFilterState:
    bl KiSystemService
    rfi
	.globl ZwQueryDefaultLocale
ZwQueryDefaultLocale:
    bl KiSystemService
    rfi
	.globl ZwQueryDefaultUILanguage
ZwQueryDefaultUILanguage:
    bl KiSystemService
    rfi
	.globl ZwQueryDirectoryFile
ZwQueryDirectoryFile:
    bl KiSystemService
    rfi
	.globl ZwQueryDirectoryObject
ZwQueryDirectoryObject:
    bl KiSystemService
    rfi
	.globl ZwQueryEaFile
ZwQueryEaFile:
    bl KiSystemService
    rfi
	.globl ZwQueryEvent
ZwQueryEvent:
    bl KiSystemService
    rfi
	.globl ZwQueryFullAttributesFile
ZwQueryFullAttributesFile:
    bl KiSystemService
    rfi
	.globl ZwQueryInformationFile
ZwQueryInformationFile:
    bl KiSystemService
    rfi
	.globl ZwQueryInformationJobObject
ZwQueryInformationJobObject:
    bl KiSystemService
    rfi
	.globl ZwQueryInformationPort
ZwQueryInformationPort:
    bl KiSystemService
    rfi
	.globl ZwQueryInformationProcess
ZwQueryInformationProcess:
    bl KiSystemService
    rfi
	.globl ZwQueryInformationThread
ZwQueryInformationThread:
    bl KiSystemService
    rfi
	.globl ZwQueryInformationToken
ZwQueryInformationToken:
    bl KiSystemService
    rfi
	.globl ZwQueryInstallUILanguage
ZwQueryInstallUILanguage:
    bl KiSystemService
    rfi
	.globl ZwQueryIntervalProfile
ZwQueryIntervalProfile:
    bl KiSystemService
    rfi
	.globl ZwQueryIoCompletion
ZwQueryIoCompletion:
    bl KiSystemService
    rfi
	.globl ZwQueryKey
ZwQueryKey:
    bl KiSystemService
    rfi
	.globl ZwQueryMultipleValueKey
ZwQueryMultipleValueKey:
    bl KiSystemService
    rfi
	.globl ZwQueryMutant
ZwQueryMutant:
    bl KiSystemService
    rfi
	.globl ZwQueryObject
ZwQueryObject:
    bl KiSystemService
    rfi
	.globl ZwQueryPerformanceCounter
ZwQueryPerformanceCounter:
    bl KiSystemService
    rfi
	.globl ZwQueryQuotaInformationFile
ZwQueryQuotaInformationFile:
    bl KiSystemService
    rfi
	.globl ZwQuerySection
ZwQuerySection:
    bl KiSystemService
    rfi
	.globl ZwQuerySecurityObject
ZwQuerySecurityObject:
    bl KiSystemService
    rfi
	.globl ZwQuerySemaphore
ZwQuerySemaphore:
    bl KiSystemService
    rfi
	.globl ZwQuerySymbolicLinkObject
ZwQuerySymbolicLinkObject:
    bl KiSystemService
    rfi
	.globl ZwQuerySystemEnvironmentValue
ZwQuerySystemEnvironmentValue:
    bl KiSystemService
    rfi
	.globl ZwQuerySystemInformation
ZwQuerySystemInformation:
    bl KiSystemService
    rfi
	.globl ZwQuerySystemTime
ZwQuerySystemTime:
    bl KiSystemService
    rfi
	.globl ZwQueryTimer
ZwQueryTimer:
    bl KiSystemService
    rfi
	.globl ZwQueryTimerResolution
ZwQueryTimerResolution:
    bl KiSystemService
    rfi
	.globl ZwQueryValueKey
ZwQueryValueKey:
    bl KiSystemService
    rfi
	.globl ZwQueryVirtualMemory
ZwQueryVirtualMemory:
    bl KiSystemService
    rfi
	.globl ZwQueryVolumeInformationFile
ZwQueryVolumeInformationFile:
    bl KiSystemService
    rfi
	.globl ZwQueueApcThread
ZwQueueApcThread:
    bl KiSystemService
    rfi
	.globl ZwRaiseException
ZwRaiseException:
    bl KiSystemService
    rfi
	.globl ZwRaiseHardError
ZwRaiseHardError:
    bl KiSystemService
    rfi
	.globl ZwReadFile
ZwReadFile:
    bl KiSystemService
    rfi
	.globl ZwReadFileScatter
ZwReadFileScatter:
    bl KiSystemService
    rfi
	.globl ZwReadRequestData
ZwReadRequestData:
    bl KiSystemService
    rfi
	.globl ZwReadVirtualMemory
ZwReadVirtualMemory:
    bl KiSystemService
    rfi
	.globl ZwRegisterThreadTerminatePort
ZwRegisterThreadTerminatePort:
    bl KiSystemService
    rfi
	.globl ZwReleaseMutant
ZwReleaseMutant:
    bl KiSystemService
    rfi
	.globl ZwReleaseSemaphore
ZwReleaseSemaphore:
    bl KiSystemService
    rfi
	.globl ZwRemoveIoCompletion
ZwRemoveIoCompletion:
    bl KiSystemService
    rfi
	.globl ZwRemoveProcessDebug
ZwRemoveProcessDebug:
    bl KiSystemService
    rfi
	.globl ZwReplaceKey
ZwReplaceKey:
    bl KiSystemService
    rfi
	.globl ZwReplyPort
ZwReplyPort:
    bl KiSystemService
    rfi
	.globl ZwReplyWaitReceivePort
ZwReplyWaitReceivePort:
    bl KiSystemService
    rfi
	.globl ZwReplyWaitReceivePortEx
ZwReplyWaitReceivePortEx:
    bl KiSystemService
    rfi
	.globl ZwReplyWaitReplyPort
ZwReplyWaitReplyPort:
    bl KiSystemService
    rfi
	.globl ZwRequestPort
ZwRequestPort:
    bl KiSystemService
    rfi
	.globl ZwRequestWaitReplyPort
ZwRequestWaitReplyPort:
    bl KiSystemService
    rfi
	.globl ZwResetEvent
ZwResetEvent:
    bl KiSystemService
    rfi
	.globl ZwRestoreKey
ZwRestoreKey:
    bl KiSystemService
    rfi
	.globl ZwResumeProcess
ZwResumeProcess:
    bl KiSystemService
    rfi
	.globl ZwResumeThread
ZwResumeThread:
    bl KiSystemService
    rfi
	.globl ZwSaveKey
ZwSaveKey:
    bl KiSystemService
    rfi
	.globl ZwSaveKeyEx
ZwSaveKeyEx:
    bl KiSystemService
    rfi
	.globl ZwSecureConnectPort
ZwSecureConnectPort:
    bl KiSystemService
    rfi
	.globl ZwSetBootEntryOrder
ZwSetBootEntryOrder:
    bl KiSystemService
    rfi
	.globl ZwSetBootOptions
ZwSetBootOptions:
    bl KiSystemService
    rfi
	.globl ZwSetDebugFilterState
ZwSetDebugFilterState:
    bl KiSystemService
    rfi
	.globl ZwSetIoCompletion
ZwSetIoCompletion:
    bl KiSystemService
    rfi
	.globl ZwSetContextThread
ZwSetContextThread:
    bl KiSystemService
    rfi
	.globl ZwSetDefaultHardErrorPort
ZwSetDefaultHardErrorPort:
    bl KiSystemService
    rfi
	.globl ZwSetDefaultLocale
ZwSetDefaultLocale:
    bl KiSystemService
    rfi
	.globl ZwSetDefaultUILanguage
ZwSetDefaultUILanguage:
    bl KiSystemService
    rfi
	.globl ZwSetEaFile
ZwSetEaFile:
    bl KiSystemService
    rfi
	.globl ZwSetEvent
ZwSetEvent:
    bl KiSystemService
    rfi
	.globl ZwSetHighEventPair
ZwSetHighEventPair:
    bl KiSystemService
    rfi
	.globl ZwSetHighWaitLowEventPair
ZwSetHighWaitLowEventPair:
    bl KiSystemService
    rfi
	.globl ZwSetInformationDebugObject
ZwSetInformationDebugObject:
    bl KiSystemService
    rfi
	.globl ZwSetInformationFile
ZwSetInformationFile:
    bl KiSystemService
    rfi
	.globl ZwSetInformationKey
ZwSetInformationKey:
    bl KiSystemService
    rfi
	.globl ZwSetInformationJobObject
ZwSetInformationJobObject:
    bl KiSystemService
    rfi
	.globl ZwSetInformationObject
ZwSetInformationObject:
    bl KiSystemService
    rfi
	.globl ZwSetInformationProcess
ZwSetInformationProcess:
    bl KiSystemService
    rfi
	.globl ZwSetInformationThread
ZwSetInformationThread:
    bl KiSystemService
    rfi
	.globl ZwSetInformationToken
ZwSetInformationToken:
    bl KiSystemService
    rfi
	.globl ZwSetIntervalProfile
ZwSetIntervalProfile:
    bl KiSystemService
    rfi
	.globl ZwSetLdtEntries
ZwSetLdtEntries:
    bl KiSystemService
    rfi
	.globl ZwSetLowEventPair
ZwSetLowEventPair:
    bl KiSystemService
    rfi
	.globl ZwSetLowWaitHighEventPair
ZwSetLowWaitHighEventPair:
    bl KiSystemService
    rfi
	.globl ZwSetQuotaInformationFile
ZwSetQuotaInformationFile:
    bl KiSystemService
    rfi
	.globl ZwSetSecurityObject
ZwSetSecurityObject:
    bl KiSystemService
    rfi
	.globl ZwSetSystemEnvironmentValue
ZwSetSystemEnvironmentValue:
    bl KiSystemService
    rfi
	.globl ZwSetSystemInformation
ZwSetSystemInformation:
    bl KiSystemService
    rfi
	.globl ZwSetSystemPowerState
ZwSetSystemPowerState:
    bl KiSystemService
    rfi
	.globl ZwSetSystemTime
ZwSetSystemTime:
    bl KiSystemService
    rfi
	.globl ZwSetTimer
ZwSetTimer:
    bl KiSystemService
    rfi
	.globl ZwSetTimerResolution
ZwSetTimerResolution:
    bl KiSystemService
    rfi
	.globl ZwSetUuidSeed
ZwSetUuidSeed:
    bl KiSystemService
    rfi
	.globl ZwSetValueKey
ZwSetValueKey:
    bl KiSystemService
    rfi
	.globl ZwSetVolumeInformationFile
ZwSetVolumeInformationFile:
    bl KiSystemService
    rfi
	.globl ZwShutdownSystem
ZwShutdownSystem:
    bl KiSystemService
    rfi
	.globl ZwSignalAndWaitForSingleObject
ZwSignalAndWaitForSingleObject:
    bl KiSystemService
    rfi
	.globl ZwStartProfile
ZwStartProfile:
    bl KiSystemService
    rfi
	.globl ZwStopProfile
ZwStopProfile:
    bl KiSystemService
    rfi
	.globl ZwSuspendProcess
ZwSuspendProcess:
    bl KiSystemService
    rfi
	.globl ZwSuspendThread
ZwSuspendThread:
    bl KiSystemService
    rfi
	.globl ZwSystemDebugControl
ZwSystemDebugControl:
    bl KiSystemService
    rfi
	.globl ZwTerminateJobObject
ZwTerminateJobObject:
    bl KiSystemService
    rfi
	.globl ZwTerminateProcess
ZwTerminateProcess:
    bl KiSystemService
    rfi
	.globl ZwTerminateThread
ZwTerminateThread:
    bl KiSystemService
    rfi
	.globl ZwTestAlert
ZwTestAlert:
    bl KiSystemService
    rfi
	.globl ZwTraceEvent
ZwTraceEvent:
    bl KiSystemService
    rfi
	.globl ZwTranslateFilePath
ZwTranslateFilePath:
    bl KiSystemService
    rfi
	.globl ZwUnloadDriver
ZwUnloadDriver:
    bl KiSystemService
    rfi
	.globl ZwUnloadKey
ZwUnloadKey:
    bl KiSystemService
    rfi
	.globl ZwUnlockFile
ZwUnlockFile:
    bl KiSystemService
    rfi
	.globl ZwUnlockVirtualMemory
ZwUnlockVirtualMemory:
    bl KiSystemService
    rfi
	.globl ZwUnmapViewOfSection
ZwUnmapViewOfSection:
    bl KiSystemService
    rfi
	.globl ZwVdmControl
ZwVdmControl:
    bl KiSystemService
    rfi
	.globl ZwWaitForDebugEvent
ZwWaitForDebugEvent:
    bl KiSystemService
    rfi
	.globl ZwWaitForMultipleObjects
ZwWaitForMultipleObjects:
    bl KiSystemService
    rfi
	.globl ZwWaitForSingleObject
ZwWaitForSingleObject:
    bl KiSystemService
    rfi
	.globl ZwWaitHighEventPair
ZwWaitHighEventPair:
    bl KiSystemService
    rfi
	.globl ZwWaitLowEventPair
ZwWaitLowEventPair:
    bl KiSystemService
    rfi
	.globl ZwWriteFile
ZwWriteFile:
    bl KiSystemService
    rfi
	.globl ZwWriteFileGather
ZwWriteFileGather:
    bl KiSystemService
    rfi
	.globl ZwWriteRequestData
ZwWriteRequestData:
    bl KiSystemService
    rfi
	.globl ZwWriteVirtualMemory
ZwWriteVirtualMemory:
    bl KiSystemService
    rfi
	.globl ZwW32Call
ZwW32Call:
    bl KiSystemService
    rfi
	.globl ZwYieldExecution
ZwYieldExecution:
    bl KiSystemService
    rfi
