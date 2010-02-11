/* FILE:            System Call Stubs for Native API
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         k:\dv\reactos\wr\out\rosbe\obj\ntoskrnl\ex\zw.S
 * PROGRAMMER:      Computer Generated File. See tools/nci/ncitool.c
 * REMARK:          DO NOT EDIT OR COMMIT MODIFICATIONS TO THIS FILE
 */


#include <ndk/asm.h>

// KGDT_R0_CODE > $8

.global _ZwAcceptConnectPort@24
_ZwAcceptConnectPort@24:
    movl $0x0, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwAccessCheck@32
_ZwAccessCheck@32:
    movl $0x1, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x20

.global _ZwAccessCheckAndAuditAlarm@44
_ZwAccessCheckAndAuditAlarm@44:
    movl $0x2, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x2c

.global _ZwAccessCheckByType@44
_ZwAccessCheckByType@44:
    movl $0x3, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x2c

.global _ZwAccessCheckByTypeAndAuditAlarm@64
_ZwAccessCheckByTypeAndAuditAlarm@64:
    movl $0x4, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x40

.global _ZwAccessCheckByTypeResultList@44
_ZwAccessCheckByTypeResultList@44:
    movl $0x5, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x2c

.global _ZwAccessCheckByTypeResultListAndAuditAlarm@64
_ZwAccessCheckByTypeResultListAndAuditAlarm@64:
    movl $0x6, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x40

.global _ZwAccessCheckByTypeResultListAndAuditAlarmByHandle@68
_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle@68:
    movl $0x7, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x44

.global _ZwAddAtom@12
_ZwAddAtom@12:
    movl $0x8, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwAddBootEntry@8
_ZwAddBootEntry@8:
    movl $0x9, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwAddDriverEntry@8
_ZwAddDriverEntry@8:
    movl $0xa, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwAdjustGroupsToken@24
_ZwAdjustGroupsToken@24:
    movl $0xb, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwAdjustPrivilegesToken@24
_ZwAdjustPrivilegesToken@24:
    movl $0xc, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwAlertResumeThread@8
_ZwAlertResumeThread@8:
    movl $0xd, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwAlertThread@4
_ZwAlertThread@4:
    movl $0xe, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwAllocateLocallyUniqueId@4
_ZwAllocateLocallyUniqueId@4:
    movl $0xf, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwAllocateUserPhysicalPages@12
_ZwAllocateUserPhysicalPages@12:
    movl $0x10, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwAllocateUuids@16
_ZwAllocateUuids@16:
    movl $0x11, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwAllocateVirtualMemory@24
_ZwAllocateVirtualMemory@24:
    movl $0x12, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwApphelpCacheControl@8
_ZwApphelpCacheControl@8:
    movl $0x13, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwAreMappedFilesTheSame@8
_ZwAreMappedFilesTheSame@8:
    movl $0x14, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwAssignProcessToJobObject@8
_ZwAssignProcessToJobObject@8:
    movl $0x15, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwCallbackReturn@12
_ZwCallbackReturn@12:
    movl $0x16, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwCancelDeviceWakeupRequest@4
_ZwCancelDeviceWakeupRequest@4:
    movl $0x17, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwCancelIoFile@8
_ZwCancelIoFile@8:
    movl $0x18, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwCancelTimer@8
_ZwCancelTimer@8:
    movl $0x19, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwClearEvent@4
_ZwClearEvent@4:
    movl $0x1a, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwClose@4
_ZwClose@4:
    movl $0x1b, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwCloseObjectAuditAlarm@12
_ZwCloseObjectAuditAlarm@12:
    movl $0x1c, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwCompactKeys@8
_ZwCompactKeys@8:
    movl $0x1d, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwCompareTokens@12
_ZwCompareTokens@12:
    movl $0x1e, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwCompleteConnectPort@4
_ZwCompleteConnectPort@4:
    movl $0x1f, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwCompressKey@4
_ZwCompressKey@4:
    movl $0x20, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwConnectPort@32
_ZwConnectPort@32:
    movl $0x21, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x20

.global _ZwContinue@8
_ZwContinue@8:
    movl $0x22, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwCreateDebugObject@16
_ZwCreateDebugObject@16:
    movl $0x23, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwCreateDirectoryObject@12
_ZwCreateDirectoryObject@12:
    movl $0x24, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwCreateEvent@20
_ZwCreateEvent@20:
    movl $0x25, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwCreateEventPair@12
_ZwCreateEventPair@12:
    movl $0x26, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwCreateFile@44
_ZwCreateFile@44:
    movl $0x27, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x2c

.global _ZwCreateIoCompletion@16
_ZwCreateIoCompletion@16:
    movl $0x28, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwCreateJobObject@12
_ZwCreateJobObject@12:
    movl $0x29, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwCreateJobSet@12
_ZwCreateJobSet@12:
    movl $0x2a, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwCreateKey@28
_ZwCreateKey@28:
    movl $0x2b, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x1c

.global _ZwCreateMailslotFile@32
_ZwCreateMailslotFile@32:
    movl $0x2c, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x20

.global _ZwCreateMutant@16
_ZwCreateMutant@16:
    movl $0x2d, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwCreateNamedPipeFile@56
_ZwCreateNamedPipeFile@56:
    movl $0x2e, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x38

.global _ZwCreatePagingFile@16
_ZwCreatePagingFile@16:
    movl $0x2f, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwCreatePort@20
_ZwCreatePort@20:
    movl $0x30, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwCreateProcess@32
_ZwCreateProcess@32:
    movl $0x31, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x20

.global _ZwCreateProcessEx@36
_ZwCreateProcessEx@36:
    movl $0x32, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x24

.global _ZwCreateProfile@36
_ZwCreateProfile@36:
    movl $0x33, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x24

.global _ZwCreateSection@28
_ZwCreateSection@28:
    movl $0x34, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x1c

.global _ZwCreateSemaphore@20
_ZwCreateSemaphore@20:
    movl $0x35, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwCreateSymbolicLinkObject@16
_ZwCreateSymbolicLinkObject@16:
    movl $0x36, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwCreateThread@32
_ZwCreateThread@32:
    movl $0x37, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x20

.global _ZwCreateTimer@16
_ZwCreateTimer@16:
    movl $0x38, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwCreateToken@52
_ZwCreateToken@52:
    movl $0x39, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x34

.global _ZwCreateWaitablePort@20
_ZwCreateWaitablePort@20:
    movl $0x3a, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwDebugActiveProcess@8
_ZwDebugActiveProcess@8:
    movl $0x3b, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwDebugContinue@12
_ZwDebugContinue@12:
    movl $0x3c, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwDelayExecution@8
_ZwDelayExecution@8:
    movl $0x3d, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwDeleteAtom@4
_ZwDeleteAtom@4:
    movl $0x3e, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwDeleteBootEntry@4
_ZwDeleteBootEntry@4:
    movl $0x3f, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwDeleteDriverEntry@4
_ZwDeleteDriverEntry@4:
    movl $0x40, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwDeleteFile@4
_ZwDeleteFile@4:
    movl $0x41, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwDeleteKey@4
_ZwDeleteKey@4:
    movl $0x42, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwDeleteObjectAuditAlarm@12
_ZwDeleteObjectAuditAlarm@12:
    movl $0x43, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwDeleteValueKey@8
_ZwDeleteValueKey@8:
    movl $0x44, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwDeviceIoControlFile@40
_ZwDeviceIoControlFile@40:
    movl $0x45, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x28

.global _ZwDisplayString@4
_ZwDisplayString@4:
    movl $0x46, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwDuplicateObject@28
_ZwDuplicateObject@28:
    movl $0x47, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x1c

.global _ZwDuplicateToken@24
_ZwDuplicateToken@24:
    movl $0x48, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwEnumerateBootEntries@8
_ZwEnumerateBootEntries@8:
    movl $0x49, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwEnumerateDriverEntries@8
_ZwEnumerateDriverEntries@8:
    movl $0x4a, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwEnumerateKey@24
_ZwEnumerateKey@24:
    movl $0x4b, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwEnumerateSystemEnvironmentValuesEx@12
_ZwEnumerateSystemEnvironmentValuesEx@12:
    movl $0x4c, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwEnumerateValueKey@24
_ZwEnumerateValueKey@24:
    movl $0x4d, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwExtendSection@8
_ZwExtendSection@8:
    movl $0x4e, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwFilterToken@24
_ZwFilterToken@24:
    movl $0x4f, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwFindAtom@12
_ZwFindAtom@12:
    movl $0x50, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwFlushBuffersFile@8
_ZwFlushBuffersFile@8:
    movl $0x51, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwFlushInstructionCache@12
_ZwFlushInstructionCache@12:
    movl $0x52, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwFlushKey@4
_ZwFlushKey@4:
    movl $0x53, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwFlushVirtualMemory@16
_ZwFlushVirtualMemory@16:
    movl $0x54, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwFlushWriteBuffer@0
_ZwFlushWriteBuffer@0:
    movl $0x55, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x0

.global _ZwFreeUserPhysicalPages@12
_ZwFreeUserPhysicalPages@12:
    movl $0x56, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwFreeVirtualMemory@16
_ZwFreeVirtualMemory@16:
    movl $0x57, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwFsControlFile@40
_ZwFsControlFile@40:
    movl $0x58, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x28

.global _ZwGetContextThread@8
_ZwGetContextThread@8:
    movl $0x59, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwGetDevicePowerState@8
_ZwGetDevicePowerState@8:
    movl $0x5a, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwGetPlugPlayEvent@16
_ZwGetPlugPlayEvent@16:
    movl $0x5b, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwGetWriteWatch@28
_ZwGetWriteWatch@28:
    movl $0x5c, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x1c

.global _ZwImpersonateAnonymousToken@4
_ZwImpersonateAnonymousToken@4:
    movl $0x5d, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwImpersonateClientOfPort@8
_ZwImpersonateClientOfPort@8:
    movl $0x5e, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwImpersonateThread@12
_ZwImpersonateThread@12:
    movl $0x5f, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwInitializeRegistry@4
_ZwInitializeRegistry@4:
    movl $0x60, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwInitiatePowerAction@16
_ZwInitiatePowerAction@16:
    movl $0x61, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwIsProcessInJob@8
_ZwIsProcessInJob@8:
    movl $0x62, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwIsSystemResumeAutomatic@0
_ZwIsSystemResumeAutomatic@0:
    movl $0x63, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x0

.global _ZwListenPort@8
_ZwListenPort@8:
    movl $0x64, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwLoadDriver@4
_ZwLoadDriver@4:
    movl $0x65, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwLoadKey@8
_ZwLoadKey@8:
    movl $0x66, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwLoadKey2@12
_ZwLoadKey2@12:
    movl $0x67, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwLoadKeyEx@16
_ZwLoadKeyEx@16:
    movl $0x68, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwLockFile@40
_ZwLockFile@40:
    movl $0x69, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x28

.global _ZwLockProductActivationKeys@8
_ZwLockProductActivationKeys@8:
    movl $0x6a, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwLockRegistryKey@4
_ZwLockRegistryKey@4:
    movl $0x6b, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwLockVirtualMemory@16
_ZwLockVirtualMemory@16:
    movl $0x6c, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwMakePermanentObject@4
_ZwMakePermanentObject@4:
    movl $0x6d, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwMakeTemporaryObject@4
_ZwMakeTemporaryObject@4:
    movl $0x6e, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwMapUserPhysicalPages@12
_ZwMapUserPhysicalPages@12:
    movl $0x6f, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwMapUserPhysicalPagesScatter@12
_ZwMapUserPhysicalPagesScatter@12:
    movl $0x70, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwMapViewOfSection@40
_ZwMapViewOfSection@40:
    movl $0x71, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x28

.global _ZwModifyBootEntry@4
_ZwModifyBootEntry@4:
    movl $0x72, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwModifyDriverEntry@4
_ZwModifyDriverEntry@4:
    movl $0x73, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwNotifyChangeDirectoryFile@36
_ZwNotifyChangeDirectoryFile@36:
    movl $0x74, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x24

.global _ZwNotifyChangeKey@40
_ZwNotifyChangeKey@40:
    movl $0x75, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x28

.global _ZwNotifyChangeMultipleKeys@48
_ZwNotifyChangeMultipleKeys@48:
    movl $0x76, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x30

.global _ZwOpenDirectoryObject@12
_ZwOpenDirectoryObject@12:
    movl $0x77, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwOpenEvent@12
_ZwOpenEvent@12:
    movl $0x78, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwOpenEventPair@12
_ZwOpenEventPair@12:
    movl $0x79, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwOpenFile@24
_ZwOpenFile@24:
    movl $0x7a, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwOpenIoCompletion@12
_ZwOpenIoCompletion@12:
    movl $0x7b, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwOpenJobObject@12
_ZwOpenJobObject@12:
    movl $0x7c, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwOpenKey@12
_ZwOpenKey@12:
    movl $0x7d, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwOpenMutant@12
_ZwOpenMutant@12:
    movl $0x7e, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwOpenObjectAuditAlarm@48
_ZwOpenObjectAuditAlarm@48:
    movl $0x7f, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x30

.global _ZwOpenProcess@16
_ZwOpenProcess@16:
    movl $0x80, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwOpenProcessToken@12
_ZwOpenProcessToken@12:
    movl $0x81, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwOpenProcessTokenEx@16
_ZwOpenProcessTokenEx@16:
    movl $0x82, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwOpenSection@12
_ZwOpenSection@12:
    movl $0x83, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwOpenSemaphore@12
_ZwOpenSemaphore@12:
    movl $0x84, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwOpenSymbolicLinkObject@12
_ZwOpenSymbolicLinkObject@12:
    movl $0x85, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwOpenThread@16
_ZwOpenThread@16:
    movl $0x86, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwOpenThreadToken@16
_ZwOpenThreadToken@16:
    movl $0x87, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwOpenThreadTokenEx@20
_ZwOpenThreadTokenEx@20:
    movl $0x88, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwOpenTimer@12
_ZwOpenTimer@12:
    movl $0x89, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwPlugPlayControl@12
_ZwPlugPlayControl@12:
    movl $0x8a, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwPowerInformation@20
_ZwPowerInformation@20:
    movl $0x8b, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwPrivilegeCheck@12
_ZwPrivilegeCheck@12:
    movl $0x8c, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwPrivilegeObjectAuditAlarm@24
_ZwPrivilegeObjectAuditAlarm@24:
    movl $0x8d, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwPrivilegedServiceAuditAlarm@20
_ZwPrivilegedServiceAuditAlarm@20:
    movl $0x8e, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwProtectVirtualMemory@20
_ZwProtectVirtualMemory@20:
    movl $0x8f, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwPulseEvent@8
_ZwPulseEvent@8:
    movl $0x90, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwQueryAttributesFile@8
_ZwQueryAttributesFile@8:
    movl $0x91, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwQueryBootEntryOrder@8
_ZwQueryBootEntryOrder@8:
    movl $0x92, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwQueryBootOptions@8
_ZwQueryBootOptions@8:
    movl $0x93, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwQueryDebugFilterState@8
_ZwQueryDebugFilterState@8:
    movl $0x94, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwQueryDefaultLocale@8
_ZwQueryDefaultLocale@8:
    movl $0x95, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwQueryDefaultUILanguage@4
_ZwQueryDefaultUILanguage@4:
    movl $0x96, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwQueryDirectoryFile@44
_ZwQueryDirectoryFile@44:
    movl $0x97, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x2c

.global _ZwQueryDirectoryObject@28
_ZwQueryDirectoryObject@28:
    movl $0x98, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x1c

.global _ZwQueryDriverEntryOrder@8
_ZwQueryDriverEntryOrder@8:
    movl $0x99, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwQueryEaFile@36
_ZwQueryEaFile@36:
    movl $0x9a, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x24

.global _ZwQueryEvent@20
_ZwQueryEvent@20:
    movl $0x9b, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQueryFullAttributesFile@8
_ZwQueryFullAttributesFile@8:
    movl $0x9c, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwQueryInformationAtom@20
_ZwQueryInformationAtom@20:
    movl $0x9d, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQueryInformationFile@20
_ZwQueryInformationFile@20:
    movl $0x9e, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQueryInformationJobObject@20
_ZwQueryInformationJobObject@20:
    movl $0x9f, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQueryInformationPort@20
_ZwQueryInformationPort@20:
    movl $0xa0, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQueryInformationProcess@20
_ZwQueryInformationProcess@20:
    movl $0xa1, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQueryInformationThread@20
_ZwQueryInformationThread@20:
    movl $0xa2, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQueryInformationToken@20
_ZwQueryInformationToken@20:
    movl $0xa3, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQueryInstallUILanguage@4
_ZwQueryInstallUILanguage@4:
    movl $0xa4, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwQueryIntervalProfile@8
_ZwQueryIntervalProfile@8:
    movl $0xa5, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwQueryIoCompletion@20
_ZwQueryIoCompletion@20:
    movl $0xa6, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQueryKey@20
_ZwQueryKey@20:
    movl $0xa7, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQueryMultipleValueKey@24
_ZwQueryMultipleValueKey@24:
    movl $0xa8, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwQueryMutant@20
_ZwQueryMutant@20:
    movl $0xa9, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQueryObject@20
_ZwQueryObject@20:
    movl $0xaa, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQueryOpenSubKeys@8
_ZwQueryOpenSubKeys@8:
    movl $0xab, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwQueryOpenSubKeysEx@16
_ZwQueryOpenSubKeysEx@16:
    movl $0xac, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwQueryPerformanceCounter@8
_ZwQueryPerformanceCounter@8:
    movl $0xad, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwQueryQuotaInformationFile@36
_ZwQueryQuotaInformationFile@36:
    movl $0xae, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x24

.global _ZwQuerySection@20
_ZwQuerySection@20:
    movl $0xaf, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQuerySecurityObject@20
_ZwQuerySecurityObject@20:
    movl $0xb0, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQuerySemaphore@20
_ZwQuerySemaphore@20:
    movl $0xb1, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQuerySymbolicLinkObject@12
_ZwQuerySymbolicLinkObject@12:
    movl $0xb2, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwQuerySystemEnvironmentValue@16
_ZwQuerySystemEnvironmentValue@16:
    movl $0xb3, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwQuerySystemEnvironmentValueEx@20
_ZwQuerySystemEnvironmentValueEx@20:
    movl $0xb4, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQuerySystemInformation@16
_ZwQuerySystemInformation@16:
    movl $0xb5, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwQuerySystemTime@4
_ZwQuerySystemTime@4:
    movl $0xb6, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwQueryTimer@20
_ZwQueryTimer@20:
    movl $0xb7, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQueryTimerResolution@12
_ZwQueryTimerResolution@12:
    movl $0xb8, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwQueryValueKey@24
_ZwQueryValueKey@24:
    movl $0xb9, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwQueryVirtualMemory@24
_ZwQueryVirtualMemory@24:
    movl $0xba, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwQueryVolumeInformationFile@20
_ZwQueryVolumeInformationFile@20:
    movl $0xbb, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwQueueApcThread@20
_ZwQueueApcThread@20:
    movl $0xbc, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwRaiseException@12
_ZwRaiseException@12:
    movl $0xbd, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwRaiseHardError@24
_ZwRaiseHardError@24:
    movl $0xbe, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwReadFile@36
_ZwReadFile@36:
    movl $0xbf, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x24

.global _ZwReadFileScatter@36
_ZwReadFileScatter@36:
    movl $0xc0, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x24

.global _ZwReadRequestData@24
_ZwReadRequestData@24:
    movl $0xc1, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwReadVirtualMemory@20
_ZwReadVirtualMemory@20:
    movl $0xc2, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwRegisterThreadTerminatePort@4
_ZwRegisterThreadTerminatePort@4:
    movl $0xc3, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwReleaseMutant@8
_ZwReleaseMutant@8:
    movl $0xc4, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwReleaseSemaphore@12
_ZwReleaseSemaphore@12:
    movl $0xc5, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwRemoveIoCompletion@20
_ZwRemoveIoCompletion@20:
    movl $0xc6, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwRemoveProcessDebug@8
_ZwRemoveProcessDebug@8:
    movl $0xc7, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwRenameKey@8
_ZwRenameKey@8:
    movl $0xc8, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwReplaceKey@12
_ZwReplaceKey@12:
    movl $0xc9, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwReplyPort@8
_ZwReplyPort@8:
    movl $0xca, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwReplyWaitReceivePort@16
_ZwReplyWaitReceivePort@16:
    movl $0xcb, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwReplyWaitReceivePortEx@20
_ZwReplyWaitReceivePortEx@20:
    movl $0xcc, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwReplyWaitReplyPort@8
_ZwReplyWaitReplyPort@8:
    movl $0xcd, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwRequestDeviceWakeup@4
_ZwRequestDeviceWakeup@4:
    movl $0xce, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwRequestPort@8
_ZwRequestPort@8:
    movl $0xcf, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwRequestWaitReplyPort@12
_ZwRequestWaitReplyPort@12:
    movl $0xd0, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwRequestWakeupLatency@4
_ZwRequestWakeupLatency@4:
    movl $0xd1, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwResetEvent@8
_ZwResetEvent@8:
    movl $0xd2, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwResetWriteWatch@12
_ZwResetWriteWatch@12:
    movl $0xd3, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwRestoreKey@12
_ZwRestoreKey@12:
    movl $0xd4, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwResumeProcess@4
_ZwResumeProcess@4:
    movl $0xd5, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwResumeThread@8
_ZwResumeThread@8:
    movl $0xd6, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwSaveKey@8
_ZwSaveKey@8:
    movl $0xd7, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwSaveKeyEx@12
_ZwSaveKeyEx@12:
    movl $0xd8, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwSaveMergedKeys@12
_ZwSaveMergedKeys@12:
    movl $0xd9, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwSecureConnectPort@36
_ZwSecureConnectPort@36:
    movl $0xda, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x24

.global _ZwSetBootEntryOrder@8
_ZwSetBootEntryOrder@8:
    movl $0xdb, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwSetBootOptions@8
_ZwSetBootOptions@8:
    movl $0xdc, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwSetContextThread@8
_ZwSetContextThread@8:
    movl $0xdd, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwSetDebugFilterState@12
_ZwSetDebugFilterState@12:
    movl $0xde, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwSetDefaultHardErrorPort@4
_ZwSetDefaultHardErrorPort@4:
    movl $0xdf, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwSetDefaultLocale@8
_ZwSetDefaultLocale@8:
    movl $0xe0, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwSetDefaultUILanguage@4
_ZwSetDefaultUILanguage@4:
    movl $0xe1, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwSetDriverEntryOrder@8
_ZwSetDriverEntryOrder@8:
    movl $0xe2, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwSetEaFile@16
_ZwSetEaFile@16:
    movl $0xe3, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwSetEvent@8
_ZwSetEvent@8:
    movl $0xe4, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwSetEventBoostPriority@4
_ZwSetEventBoostPriority@4:
    movl $0xe5, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwSetHighEventPair@4
_ZwSetHighEventPair@4:
    movl $0xe6, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwSetHighWaitLowEventPair@4
_ZwSetHighWaitLowEventPair@4:
    movl $0xe7, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwSetInformationDebugObject@20
_ZwSetInformationDebugObject@20:
    movl $0xe8, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwSetInformationFile@20
_ZwSetInformationFile@20:
    movl $0xe9, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwSetInformationJobObject@16
_ZwSetInformationJobObject@16:
    movl $0xea, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwSetInformationKey@16
_ZwSetInformationKey@16:
    movl $0xeb, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwSetInformationObject@16
_ZwSetInformationObject@16:
    movl $0xec, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwSetInformationProcess@16
_ZwSetInformationProcess@16:
    movl $0xed, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwSetInformationThread@16
_ZwSetInformationThread@16:
    movl $0xee, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwSetInformationToken@16
_ZwSetInformationToken@16:
    movl $0xef, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwSetIntervalProfile@8
_ZwSetIntervalProfile@8:
    movl $0xf0, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwSetIoCompletion@20
_ZwSetIoCompletion@20:
    movl $0xf1, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwSetLdtEntries@24
_ZwSetLdtEntries@24:
    movl $0xf2, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwSetLowEventPair@4
_ZwSetLowEventPair@4:
    movl $0xf3, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwSetLowWaitHighEventPair@4
_ZwSetLowWaitHighEventPair@4:
    movl $0xf4, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwSetQuotaInformationFile@16
_ZwSetQuotaInformationFile@16:
    movl $0xf5, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwSetSecurityObject@12
_ZwSetSecurityObject@12:
    movl $0xf6, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwSetSystemEnvironmentValue@8
_ZwSetSystemEnvironmentValue@8:
    movl $0xf7, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwSetSystemEnvironmentValueEx@8
_ZwSetSystemEnvironmentValueEx@8:
    movl $0xf8, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwSetSystemInformation@12
_ZwSetSystemInformation@12:
    movl $0xf9, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwSetSystemPowerState@12
_ZwSetSystemPowerState@12:
    movl $0xfa, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwSetSystemTime@8
_ZwSetSystemTime@8:
    movl $0xfb, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwSetThreadExecutionState@8
_ZwSetThreadExecutionState@8:
    movl $0xfc, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwSetTimer@28
_ZwSetTimer@28:
    movl $0xfd, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x1c

.global _ZwSetTimerResolution@12
_ZwSetTimerResolution@12:
    movl $0xfe, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwSetUuidSeed@4
_ZwSetUuidSeed@4:
    movl $0xff, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwSetValueKey@24
_ZwSetValueKey@24:
    movl $0x100, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwSetVolumeInformationFile@20
_ZwSetVolumeInformationFile@20:
    movl $0x101, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwShutdownSystem@4
_ZwShutdownSystem@4:
    movl $0x102, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwSignalAndWaitForSingleObject@16
_ZwSignalAndWaitForSingleObject@16:
    movl $0x103, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwStartProfile@4
_ZwStartProfile@4:
    movl $0x104, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwStopProfile@4
_ZwStopProfile@4:
    movl $0x105, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwSuspendProcess@4
_ZwSuspendProcess@4:
    movl $0x106, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwSuspendThread@8
_ZwSuspendThread@8:
    movl $0x107, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwSystemDebugControl@24
_ZwSystemDebugControl@24:
    movl $0x108, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwTerminateJobObject@8
_ZwTerminateJobObject@8:
    movl $0x109, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwTerminateProcess@8
_ZwTerminateProcess@8:
    movl $0x10a, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwTerminateThread@8
_ZwTerminateThread@8:
    movl $0x10b, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwTestAlert@0
_ZwTestAlert@0:
    movl $0x10c, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x0

.global _ZwTraceEvent@16
_ZwTraceEvent@16:
    movl $0x10d, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwTranslateFilePath@16
_ZwTranslateFilePath@16:
    movl $0x10e, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwUnloadDriver@4
_ZwUnloadDriver@4:
    movl $0x10f, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwUnloadKey@4
_ZwUnloadKey@4:
    movl $0x110, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwUnloadKey2@8
_ZwUnloadKey2@8:
    movl $0x111, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwUnloadKeyEx@8
_ZwUnloadKeyEx@8:
    movl $0x112, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwUnlockFile@20
_ZwUnlockFile@20:
    movl $0x113, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwUnlockVirtualMemory@16
_ZwUnlockVirtualMemory@16:
    movl $0x114, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwUnmapViewOfSection@8
_ZwUnmapViewOfSection@8:
    movl $0x115, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwVdmControl@8
_ZwVdmControl@8:
    movl $0x116, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x8

.global _ZwWaitForDebugEvent@16
_ZwWaitForDebugEvent@16:
    movl $0x117, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwWaitForMultipleObjects@20
_ZwWaitForMultipleObjects@20:
    movl $0x118, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwWaitForSingleObject@12
_ZwWaitForSingleObject@12:
    movl $0x119, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwWaitHighEventPair@4
_ZwWaitHighEventPair@4:
    movl $0x11a, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwWaitLowEventPair@4
_ZwWaitLowEventPair@4:
    movl $0x11b, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x4

.global _ZwWriteFile@36
_ZwWriteFile@36:
    movl $0x11c, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x24

.global _ZwWriteFileGather@36
_ZwWriteFileGather@36:
    movl $0x11d, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x24

.global _ZwWriteRequestData@24
_ZwWriteRequestData@24:
    movl $0x11e, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x18

.global _ZwWriteVirtualMemory@20
_ZwWriteVirtualMemory@20:
    movl $0x11f, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

.global _ZwYieldExecution@0
_ZwYieldExecution@0:
    movl $0x120, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x0

.global _ZwCreateKeyedEvent@16
_ZwCreateKeyedEvent@16:
    movl $0x121, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwOpenKeyedEvent@12
_ZwOpenKeyedEvent@12:
    movl $0x122, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0xc

.global _ZwReleaseKeyedEvent@16
_ZwReleaseKeyedEvent@16:
    movl $0x123, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwWaitForKeyedEvent@16
_ZwWaitForKeyedEvent@16:
    movl $0x124, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x10

.global _ZwQueryPortInformationProcess@0
_ZwQueryPortInformationProcess@0:
    movl $0x125, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x0

.global _ZwGetCurrentProcessorNumber@0
_ZwGetCurrentProcessorNumber@0:
    movl $0x126, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x0

.global _ZwWaitForMultipleObjects32@20
_ZwWaitForMultipleObjects32@20:
    movl $0x127, %eax
    leal 4(%esp), %edx
    pushfl
    pushl $8
    call _KiSystemService
    ret $0x14

