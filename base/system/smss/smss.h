/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/smss.h
 * PURPOSE:         Main SMSS Header
 * PROGRAMMERS:     Alex Ionescu
 */

#ifndef _SM_
#define _SM_

#pragma once

/* DEPENDENCIES ***************************************************************/

#include <stdio.h>

/* Native Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>

#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/lpcfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>
#include <ndk/umfuncs.h>

#include <ntstrsafe.h>

/* SM Protocol Header */
#include <sm/smmsg.h>

/* DEFINES ********************************************************************/

#define SMP_DEBUG_FLAG      0x01
#define SMP_ASYNC_FLAG      0x02
#define SMP_AUTOCHK_FLAG    0x04
#define SMP_SUBSYSTEM_FLAG  0x08
#define SMP_INVALID_PATH    0x10
#define SMP_DEFERRED_FLAG   0x20
#define SMP_POSIX_FLAG      0x100
#define SMP_OS2_FLAG        0x200

/* STRUCTURES *****************************************************************/

typedef struct _SMP_REGISTRY_VALUE
{
    LIST_ENTRY Entry;
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    PCHAR AnsiValue;
} SMP_REGISTRY_VALUE, *PSMP_REGISTRY_VALUE;

typedef struct _SMP_SUBSYSTEM
{
    LIST_ENTRY Entry;
    HANDLE Event;
    HANDLE ProcessHandle;
    ULONG ImageType;
    HANDLE PortHandle;
    HANDLE SbApiPort;
    CLIENT_ID ClientId;
    ULONG MuSessionId;
    BOOLEAN Terminating;
    ULONG ReferenceCount;
} SMP_SUBSYSTEM, *PSMP_SUBSYSTEM;

/* EXTERNALS ******************************************************************/

extern RTL_CRITICAL_SECTION SmpKnownSubSysLock;
extern LIST_ENTRY SmpKnownSubSysHead;
extern RTL_CRITICAL_SECTION SmpSessionListLock;
extern LIST_ENTRY SmpSessionListHead;
extern ULONG SmpNextSessionId;
extern BOOLEAN SmpNextSessionIdScanMode;
extern BOOLEAN SmpDbgSsLoaded;
extern HANDLE SmpWindowsSubSysProcess;
extern HANDLE SmpSessionsObjectDirectory;
extern HANDLE SmpWindowsSubSysProcessId;
extern BOOLEAN RegPosixSingleInstance;
extern UNICODE_STRING SmpDebugKeyword, SmpASyncKeyword, SmpAutoChkKeyword;
extern PVOID SmpHeap;
extern ULONG SmBaseTag;
extern UNICODE_STRING SmpSystemRoot;
extern PWCHAR SmpDefaultEnvironment;
extern UNICODE_STRING SmpDefaultLibPath;
extern LIST_ENTRY SmpSetupExecuteList;
extern LIST_ENTRY SmpSubSystemList;
extern LIST_ENTRY SmpSubSystemsToLoad;
extern LIST_ENTRY SmpSubSystemsToDefer;
extern LIST_ENTRY SmpExecuteList;
extern ULONG AttachedSessionId;
extern BOOLEAN SmpDebug;

/* FUNCTIONS ******************************************************************/

/* crashdmp.c */

BOOLEAN
NTAPI
SmpCheckForCrashDump(
    IN PUNICODE_STRING FileName
);

/* pagefile.c */

VOID
NTAPI
SmpPagingFileInitialize(
    VOID
);

NTSTATUS
NTAPI
SmpCreatePagingFileDescriptor(
    IN PUNICODE_STRING PageFileToken
);

NTSTATUS
NTAPI
SmpCreatePagingFiles(
    VOID
);

/* sminit.c */

VOID
NTAPI
SmpTranslateSystemPartitionInformation(
    VOID
);

NTSTATUS
NTAPI
SmpCreateSecurityDescriptors(
    IN BOOLEAN InitialCall
);

NTSTATUS
NTAPI
SmpInit(
    IN PUNICODE_STRING InitialCommand,
    OUT PHANDLE ProcessHandle
);

/* smloop.c */

ULONG
NTAPI
SmpApiLoop(
    IN PVOID Parameter
);

/* smsbapi.c */

NTSTATUS
NTAPI
SmpSbCreateSession(
    IN PVOID Reserved,
    IN PSMP_SUBSYSTEM OtherSubsystem,
    IN PRTL_USER_PROCESS_INFORMATION ProcessInformation,
    IN ULONG DbgSessionId,
    IN PCLIENT_ID DbgUiClientId
);

/* smsessn.c */

BOOLEAN
NTAPI
SmpCheckDuplicateMuSessionId(
    IN ULONG MuSessionId
);

VOID
NTAPI
SmpDeleteSession(
    IN ULONG SessionId
);

ULONG
NTAPI
SmpAllocateSessionId(
    IN PSMP_SUBSYSTEM Subsystem,
    IN PSMP_SUBSYSTEM OtherSubsystem
);

NTSTATUS
NTAPI
SmpGetProcessMuSessionId(
    IN HANDLE ProcessHandle,
    OUT PULONG SessionId
);

NTSTATUS
NTAPI
SmpSetProcessMuSessionId(
    IN HANDLE ProcessHandle,
    IN ULONG SessionId
);

/* smss.c */

NTSTATUS
NTAPI
SmpExecuteImage(
    IN PUNICODE_STRING FileName,
    IN PUNICODE_STRING Directory,
    IN PUNICODE_STRING CommandLine,
    IN ULONG MuSessionId,
    IN ULONG Flags,
    IN PRTL_USER_PROCESS_INFORMATION ProcessInformation
);

NTSTATUS
NTAPI
SmpExecuteCommand(
    IN PUNICODE_STRING CommandLine,
    IN ULONG MuSessionId,
    OUT PHANDLE ProcessId,
    IN ULONG Flags
);

NTSTATUS
NTAPI
SmpExecuteInitialCommand(IN ULONG MuSessionId,
                         IN PUNICODE_STRING InitialCommand,
                         IN HANDLE InitialCommandProcess,
                         OUT PHANDLE ReturnPid);

NTSTATUS
NTAPI
SmpTerminate(
    IN PULONG_PTR Parameters,
    IN ULONG ParameterMask,
    IN ULONG ParameterCount
);

/* smsubsys.c */

VOID
NTAPI
SmpDereferenceSubsystem(
    IN PSMP_SUBSYSTEM SubSystem
);

PSMP_SUBSYSTEM
NTAPI
SmpLocateKnownSubSysByCid(
    IN PCLIENT_ID ClientId
);

PSMP_SUBSYSTEM
NTAPI
SmpLocateKnownSubSysByType(
    IN ULONG MuSessionId,
    IN ULONG ImageType
);

NTSTATUS
NTAPI
SmpLoadSubSystem(
    IN PUNICODE_STRING FileName,
    IN PUNICODE_STRING Directory,
    IN PUNICODE_STRING CommandLine,
    IN ULONG MuSessionId,
    OUT PHANDLE ProcessId,
    IN ULONG Flags
);

NTSTATUS
NTAPI
SmpLoadSubSystemsForMuSession(
    IN PULONG MuSessionId,
    OUT PHANDLE ProcessId,
    IN PUNICODE_STRING InitialCommand
);

/* smutil.c */

NTSTATUS
NTAPI
SmpAcquirePrivilege(
    IN ULONG Privilege,
    OUT PVOID *PrivilegeStat
);

VOID
NTAPI
SmpReleasePrivilege(
    IN PVOID State
);

NTSTATUS
NTAPI
SmpParseCommandLine(
    IN PUNICODE_STRING CommandLine,
    OUT PULONG Flags,
    OUT PUNICODE_STRING FileName,
    OUT PUNICODE_STRING Directory,
    OUT PUNICODE_STRING Arguments
);

BOOLEAN
NTAPI
SmpQueryRegistrySosOption(
    VOID
);

BOOLEAN
NTAPI
SmpSaveAndClearBootStatusData(
    OUT PBOOLEAN BootOkay,
    OUT PBOOLEAN ShutdownOkay
);

VOID
NTAPI
SmpRestoreBootStatusData(
    IN BOOLEAN BootOkay,
    IN BOOLEAN ShutdownOkay
);

#endif /* _SM_ */
