/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/smss.h
 * PURPOSE:         Main SMSS Header
 * PROGRAMMERS:     Alex Ionescu
 */

/* DEPENDENCIES ***************************************************************/

//
// Native Headers
//
#define WIN32_NO_STATUS
#include <windows.h> // Should just be using ntdef.h I think
#define RTL_NUMBER_OF_V1(A) (sizeof(A)/sizeof((A)[0]))
#define RTL_NUMBER_OF_V2(A) RTL_NUMBER_OF_V1(A)
#ifdef ENABLE_RTL_NUMBER_OF_V2
#define RTL_NUMBER_OF(A) RTL_NUMBER_OF_V2(A)
#else
#define RTL_NUMBER_OF(A) RTL_NUMBER_OF_V1(A)
#endif
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

//
// SM Protocol Header
//
#include "sm/smmsg.h"

/* DEFINES ********************************************************************/

/* EXTERNALS ******************************************************************/

extern RTL_CRITICAL_SECTION SmpKnownSubSysLock;
extern LIST_ENTRY SmpKnownSubSysHead;
extern RTL_CRITICAL_SECTION SmpSessionListLock;
extern LIST_ENTRY SmpSessionListHead;
extern ULONG SmpNextSessionId;
extern ULONG SmpNextSessionIdScanMode;
extern BOOLEAN SmpDbgSsLoaded;
extern HANDLE SmpWindowsSubSysProcess;
extern HANDLE SmpSessionsObjectDirectory;
extern HANDLE SmpWindowsSubSysProcessId;
extern BOOLEAN RegPosixSingleInstance;

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
SmpTerminate(
    IN PULONG_PTR Parameters,
    IN ULONG ParameterMask,
    IN ULONG ParameterCount
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

ULONG
NTAPI
SmpApiLoop(
    IN PVOID Parameter
);

NTSTATUS
NTAPI
SmpExecuteCommand(
    IN PUNICODE_STRING CommandLine,
    IN ULONG MuSessionId,
    OUT PULONG ProcessId,
    IN ULONG Flags
);

NTSTATUS
NTAPI
SmpLoadSubSystemsForMuSession(
    IN PULONG MuSessionId,
    OUT PHANDLE ProcessId,
    IN PUNICODE_STRING InitialCommand
);

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

