#ifndef _CSRSRV_H
#define _CSRSRV_H

/* PSDK/NDK Headers */
#define NTOS_MODE_USER
#include <stdio.h>
#include <windows.h>
#include <winnt.h>
#include <ndk/ntndk.h>
#include <helper.h>

/* CSR Header */
#include <csr/server.h>

/* PSEH for SEH Support */
#include <pseh/pseh.h>

/* DEFINES *******************************************************************/

#define CSR_SERVER_DLL_MAX 4

#define CsrAcquireProcessLock() \
    RtlEnterCriticalSection(&CsrProcessLock);

#define CsrReleaseProcessLock() \
    RtlLeaveCriticalSection(&CsrProcessLock);

#define CsrAcquireWaitLock() \
    RtlEnterCriticalSection(&CsrWaitListsLock);

#define CsrReleaseWaitLock() \
    RtlLeaveCriticalSection(&CsrWaitListsLock);

#define CsrAcquireNtSessionLock() \
    RtlEnterCriticalSection(&CsrNtSessionLock)

#define CsrReleaseNtSessionLock() \
    RtlLeaveCriticalSection(&CsrNtSessionLock)

#define CsrHashThread(t) \
    (HandleToUlong(t)&(256 - 1))

#define SM_REG_KEY \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager"

#define SESSION_ROOT        L"\\Sessions"
#define GLOBAL_ROOT         L"\\GLOBAL??"
#define SYMLINK_NAME        L"SymbolicLink"
#define SB_PORT_NAME        L"SbAbiPort"
#define CSR_PORT_NAME       L"ApiPort"
#define UNICODE_PATH_SEP    L"\\"

/* DATA **********************************************************************/

extern ULONG CsrTotalPerProcessDataLength;
extern ULONG CsrMaxApiRequestThreads;
extern PCSR_SERVER_DLL CsrLoadedServerDll[CSR_SERVER_DLL_MAX];
extern PCSR_PROCESS CsrRootProcess;
extern UNICODE_STRING CsrSbApiPortName;
extern UNICODE_STRING CsrApiPortName;
extern HANDLE CsrSbApiPort;
extern HANDLE CsrSmApiPort;
extern HANDLE CsrApiPort;
extern HANDLE CsrHeap;
extern RTL_CRITICAL_SECTION CsrProcessLock;
extern RTL_CRITICAL_SECTION CsrWaitListsLock;
extern LIST_ENTRY CsrThreadHashTable[256];
extern HANDLE CsrInitializationEvent;
extern SYSTEM_BASIC_INFORMATION CsrNtSysInfo;
extern UNICODE_STRING CsrDirectoryName;
extern HANDLE CsrObjectDirectory;
extern PSB_API_ROUTINE CsrServerSbApiDispatch[5];

/* FUNCTIONS *****************************************************************/

/* FIXME: Public APIs should go in the CSR Server Include */
NTSTATUS
NTAPI
CsrLoadServerDll(
    IN PCHAR DllString,
    IN PCHAR EntryPoint,
    IN ULONG ServerId
);

NTSTATUS
NTAPI
CsrServerInitialization(
    ULONG ArgumentCount,
    PCHAR Arguments[]
);

NTSTATUS
NTAPI
CsrCreateSessionObjectDirectory(IN ULONG SessionId);

NTSTATUS
NTAPI
CsrCreateObjectDirectory(IN PCHAR ObjectDirectory);

NTSTATUS
NTAPI
CsrSrvCreateSharedSection(IN PCHAR ParameterValue);

NTSTATUS
NTAPI
CsrInitializeNtSessions(VOID);

NTSTATUS
NTAPI
CsrInitializeProcesses(VOID);

NTSTATUS
NTAPI
CsrApiPortInitialize(VOID);

NTSTATUS
NTAPI
CsrSbApiPortInitialize(VOID);

BOOLEAN
NTAPI
CsrSbCreateSession(IN PSB_API_MESSAGE ApiMessage);

BOOLEAN
NTAPI
CsrSbForeignSessionComplete(IN PSB_API_MESSAGE ApiMessage);

BOOLEAN
NTAPI
CsrSbCreateProcess(IN PSB_API_MESSAGE ApiMessage);

PCSR_PROCESS
NTAPI
CsrAllocateProcess(VOID);

PCSR_THREAD
NTAPI
CsrAllocateThread(IN PCSR_PROCESS CsrProcess);

VOID
NTAPI
CsrInsertThread(
    IN PCSR_PROCESS Process,
    IN PCSR_THREAD Thread
);

VOID
NTAPI
CsrSetBackgroundPriority(IN PCSR_PROCESS CsrProcess);

VOID
NTAPI
CsrDeallocateProcess(IN PCSR_PROCESS CsrProcess);

NTSTATUS
NTAPI
CsrGetProcessLuid(
    HANDLE hProcess,
    PLUID Luid
);

BOOLEAN
NTAPI
CsrImpersonateClient(IN PCSR_THREAD CsrThread);

BOOLEAN
NTAPI
CsrRevertToSelf(VOID);

PCSR_THREAD
NTAPI
CsrLocateThreadByClientId(
    OUT PCSR_PROCESS *Process,
    IN PCLIENT_ID ClientId
);

VOID
NTAPI
CsrDereferenceNtSession(
    IN PCSR_NT_SESSION Session,
    NTSTATUS ExitStatus
);

VOID
NTAPI
CsrReferenceNtSession(PCSR_NT_SESSION Session);

VOID
NTAPI
CsrLockedDereferenceThread(PCSR_THREAD CsrThread);

VOID
NTAPI
CsrLockedDereferenceProcess(PCSR_PROCESS CsrProcess);

NTSTATUS
NTAPI
CsrLockProcessByClientId(
    IN HANDLE Pid,
    OUT PCSR_PROCESS *CsrProcess OPTIONAL
);

NTSTATUS
NTAPI
CsrUnlockProcess(PCSR_PROCESS CsrProcess);

NTSTATUS
NTAPI
CsrSrvClientConnect(
    IN OUT PCSR_API_MESSAGE ApiMessage,
    IN OUT PULONG Reply
);

NTSTATUS
NTAPI
CsrSrvUnusedFunction(
    IN OUT PCSR_API_MESSAGE ApiMessage,
    IN OUT PULONG Reply
);

NTSTATUS
NTAPI
CsrSrvIdentifyAlertableThread(
    IN OUT PCSR_API_MESSAGE ApiMessage,
    IN OUT PULONG Reply
);

NTSTATUS
NTAPI
CsrSrvSetPriorityClass(
    IN OUT PCSR_API_MESSAGE ApiMessage,
    IN OUT PULONG Reply
);

NTSTATUS
NTAPI
CsrServerDllInitialization(IN PCSR_SERVER_DLL LoadedServerDll);

VOID
NTAPI
CsrDereferenceThread(PCSR_THREAD CsrThread);

VOID
NTAPI
CsrSbApiRequestThread(IN PVOID Parameter);

NTSTATUS
NTAPI
CsrApiRequestThread(IN PVOID Parameter);

PCSR_THREAD
NTAPI
CsrAddStaticServerThread(
    IN HANDLE hThread,
    IN PCLIENT_ID ClientId,
    IN ULONG ThreadFlags
);

PCSR_THREAD
NTAPI
CsrConnectToUser(VOID);

PCSR_THREAD
NTAPI
CsrLocateThreadInProcess(
    IN PCSR_PROCESS CsrProcess OPTIONAL,
    IN PCLIENT_ID Cid
);

NTSTATUS
NTAPI
CsrSbApiHandleConnectionRequest(IN PSB_API_MESSAGE Message);

NTSTATUS
NTAPI
CsrApiHandleConnectionRequest(IN PCSR_API_MESSAGE ApiMessage);

NTSTATUS
NTAPI
CsrSrvAttachSharedSection(
    IN PCSR_PROCESS CsrProcess OPTIONAL,
    OUT PCSR_CONNECTION_INFO ConnectInfo
);

VOID
NTAPI
CsrReleaseCapturedArguments(IN PCSR_API_MESSAGE ApiMessage);

BOOLEAN
NTAPI
CsrNotifyWaitBlock(
    IN PCSR_WAIT_BLOCK WaitBlock,
    IN PLIST_ENTRY WaitList,
    IN PVOID WaitArgument1,
    IN PVOID WaitArgument2,
    IN ULONG WaitFlags,
    IN BOOLEAN DereferenceThread
);

VOID
NTAPI
CsrDereferenceProcess(PCSR_PROCESS CsrProcess);

VOID
NTAPI
CsrInsertProcess(
    IN PCSR_PROCESS Parent OPTIONAL,
    IN PCSR_PROCESS CurrentProcess OPTIONAL,
    IN PCSR_PROCESS CsrProcess
);

NTSTATUS
NTAPI
CsrPopulateDosDevicesDirectory(
    IN HANDLE DosDevicesDirectory,
    IN PPROCESS_DEVICEMAP_INFORMATION DeviceMap
);

BOOLEAN
NTAPI
CsrValidateMessageBuffer(
    IN PCSR_API_MESSAGE ApiMessage,
    IN PVOID *Buffer,
    IN ULONG ArgumentSize,
    IN ULONG ArgumentCount
);

NTSTATUS
NTAPI
CsrCreateLocalSystemSD(OUT PSECURITY_DESCRIPTOR *LocalSystemSd);

NTSTATUS
NTAPI
CsrDestroyThread(IN PCLIENT_ID Cid);

NTSTATUS
NTAPI
CsrDestroyProcess(
    IN PCLIENT_ID Cid,
    IN NTSTATUS ExitStatus
);

_SEH_FILTER(CsrUnhandledExceptionFilter);

VOID
NTAPI
CsrProcessRefcountZero(IN PCSR_PROCESS CsrProcess);

VOID
NTAPI
CsrThreadRefcountZero(IN PCSR_THREAD CsrThread);

NTSTATUS
NTAPI
CsrSetDirectorySecurity(IN HANDLE ObjectDirectory);

#endif 
