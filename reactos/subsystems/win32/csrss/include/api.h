/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/include/api.h
 * PURPOSE:         CSRSS API interface
 */

#pragma once

#define NTOS_MODE_USER
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>

#include <csrss/csrss.h>

#define CSR_SRV_SERVER 0
#define CSR_SERVER_DLL_MAX 4
#define LOCK   RtlEnterCriticalSection(&ProcessDataLock)
#define UNLOCK RtlLeaveCriticalSection(&ProcessDataLock)
#define CsrAcquireProcessLock() LOCK
#define CsrReleaseProcessLock() UNLOCK
#define ProcessStructureListLocked() \
    (ProcessDataLock.OwningThread == NtCurrentTeb()->ClientId.UniqueThread)

#define CsrAcquireWaitLock() \
    RtlEnterCriticalSection(&CsrWaitListsLock);

#define CsrReleaseWaitLock() \
    RtlLeaveCriticalSection(&CsrWaitListsLock);

#define CsrAcquireNtSessionLock() \
    RtlEnterCriticalSection(&CsrNtSessionLock);

#define CsrReleaseNtSessionLock() \
    RtlLeaveCriticalSection(&CsrNtSessionLock);

typedef enum _CSR_THREAD_FLAGS
{
    CsrThreadAltertable = 0x1,
    CsrThreadInTermination = 0x2,
    CsrThreadTerminated = 0x4,
    CsrThreadIsServerThread = 0x10
} CSR_THREAD_FLAGS, *PCSR_THREAD_FLAGS;

typedef enum _SHUTDOWN_RESULT
{
    CsrShutdownCsrProcess = 1,
    CsrShutdownNonCsrProcess,
    CsrShutdownCancelled
} SHUTDOWN_RESULT, *PSHUTDOWN_RESULT;

typedef enum _CSR_SHUTDOWN_FLAGS
{
    CsrShutdownSystem = 4,
    CsrShutdownOther = 8
} CSR_SHUTDOWN_FLAGS, *PCSR_SHUTDOWN_FLAGS;

typedef enum _CSR_DEBUG_FLAGS
{
    CsrDebugOnlyThisProcess = 1,
    CsrDebugProcessChildren = 2
} CSR_PROCESS_DEBUG_FLAGS, *PCSR_PROCESS_DEBUG_FLAGS;

typedef enum _CSR_PROCESS_FLAGS
{
    CsrProcessTerminating = 0x1,
    CsrProcessSkipShutdown = 0x2,
    CsrProcessNormalPriority = 0x10,
    CsrProcessIdlePriority = 0x20,
    CsrProcessHighPriority = 0x40,
    CsrProcessRealtimePriority = 0x80,
    CsrProcessCreateNewGroup = 0x100,
    CsrProcessTerminated = 0x200,
    CsrProcessLastThreadTerminated = 0x400,
    CsrProcessIsConsoleApp = 0x800
} CSR_PROCESS_FLAGS, *PCSR_PROCESS_FLAGS;

#define CsrProcessPriorityFlags (CsrProcessNormalPriority | \
                                 CsrProcessIdlePriority | \
                                 CsrProcessHighPriority | \
                                 CsrProcessRealtimePriority)

typedef struct _CSRSS_CON_PROCESS_DATA
{
    HANDLE ConsoleEvent;
    struct tagCSRSS_CONSOLE *Console;
    struct tagCSRSS_CONSOLE *ParentConsole;
    BOOL bInheritHandles;
    RTL_CRITICAL_SECTION HandleTableLock;
    ULONG HandleTableSize;
    struct _CSRSS_HANDLE *HandleTable;
    PCONTROLDISPATCHER CtrlDispatcher;
    LIST_ENTRY ConsoleLink;
} CSRSS_CON_PROCESS_DATA, *PCSRSS_CON_PROCESS_DATA;

typedef struct _CSR_NT_SESSION
{
    ULONG ReferenceCount;
    LIST_ENTRY SessionLink;
    ULONG SessionId;
} CSR_NT_SESSION, *PCSR_NT_SESSION;

typedef struct _CSR_PROCESS
{
    CLIENT_ID ClientId;
    LIST_ENTRY ListLink;
    LIST_ENTRY ThreadList;
    struct _CSR_PROCESS *Parent;
    PCSR_NT_SESSION NtSession;
    ULONG ExpectedVersion;
    HANDLE ClientPort;
    ULONG_PTR ClientViewBase;
    ULONG_PTR ClientViewBounds;
    HANDLE ProcessHandle;
    ULONG SequenceNumber;
    ULONG Flags;
    ULONG DebugFlags;
    CLIENT_ID DebugCid;
    ULONG ReferenceCount;
    ULONG ProcessGroupId;
    ULONG ProcessGroupSequence;
    ULONG fVDM;
    ULONG ThreadCount;
    ULONG PriorityClass;
    ULONG Reserved;
    ULONG ShutdownLevel;
    ULONG ShutdownFlags;
//    PVOID ServerData[ANYSIZE_ARRAY];
    CSRSS_CON_PROCESS_DATA;
} CSR_PROCESS, *PCSR_PROCESS;

typedef struct _CSR_THREAD
{
    LARGE_INTEGER CreateTime;
    LIST_ENTRY Link;
    LIST_ENTRY HashLinks;
    CLIENT_ID ClientId;
    PCSR_PROCESS Process;
    struct _CSR_WAIT_BLOCK *WaitBlock;
    HANDLE ThreadHandle;
    ULONG Flags;
    ULONG ReferenceCount;
    ULONG ImpersonationCount;
} CSR_THREAD, *PCSR_THREAD;

typedef
BOOLEAN
(*CSR_WAIT_FUNCTION)(
    IN PLIST_ENTRY WaitList,
    IN PCSR_THREAD WaitThread,
    IN PCSR_API_MESSAGE WaitApiMessage,
    IN PVOID WaitContext,
    IN PVOID WaitArgument1,
    IN PVOID WaitArgument2,
    IN ULONG WaitFlags
);

typedef struct _CSR_WAIT_BLOCK
{
    ULONG Size;
    LIST_ENTRY WaitList;
    LIST_ENTRY UserWaitList;
    PVOID WaitContext;
    PCSR_THREAD WaitThread;
    CSR_WAIT_FUNCTION WaitFunction;
    CSR_API_MESSAGE WaitApiMessage;
} CSR_WAIT_BLOCK, *PCSR_WAIT_BLOCK;

typedef
NTSTATUS
(NTAPI *PCSR_CONNECT_CALLBACK)(
    IN PCSR_PROCESS CsrProcess,
    IN OUT PVOID ConnectionInfo,
    IN OUT PULONG ConnectionInfoLength
);

typedef
VOID
(NTAPI *PCSR_DISCONNECT_CALLBACK)(IN PCSR_PROCESS CsrProcess);

typedef
NTSTATUS
(NTAPI *PCSR_NEWPROCESS_CALLBACK)(
    IN PCSR_PROCESS Parent,
    IN PCSR_PROCESS CsrProcess
);

typedef
VOID
(NTAPI *PCSR_HARDERROR_CALLBACK)(
    IN PCSR_THREAD CsrThread,
    IN PHARDERROR_MSG HardErrorMessage
);

typedef
ULONG
(NTAPI *PCSR_SHUTDOWNPROCESS_CALLBACK)(
    IN PCSR_PROCESS CsrProcess,
    IN ULONG Flags,
    IN BOOLEAN FirstPhase
);

typedef
NTSTATUS
(NTAPI *PCSR_API_ROUTINE)(
    IN OUT PCSR_API_MESSAGE ApiMessage,
    IN OUT PULONG Reply
);

typedef struct _CSR_SERVER_DLL
{
    ULONG Length;
    HANDLE Event;
    ANSI_STRING Name;
    HANDLE ServerHandle;
    ULONG ServerId;
    ULONG Unknown;
    ULONG ApiBase;
    ULONG HighestApiSupported;
    PCSR_API_ROUTINE *DispatchTable;
    PBOOLEAN ValidTable;
    PCHAR *NameTable;
    ULONG SizeOfProcessData;
    PCSR_CONNECT_CALLBACK ConnectCallback;
    PCSR_DISCONNECT_CALLBACK DisconnectCallback;
    PCSR_HARDERROR_CALLBACK HardErrorCallback;
    PVOID SharedSection;
    PCSR_NEWPROCESS_CALLBACK NewProcessCallback;
    PCSR_SHUTDOWNPROCESS_CALLBACK ShutdownProcessCallback;
    ULONG Unknown2[3];
} CSR_SERVER_DLL, *PCSR_SERVER_DLL;

typedef
NTSTATUS
(NTAPI *PCSR_SERVER_DLL_INIT_CALLBACK)(IN PCSR_SERVER_DLL ServerDll);


typedef NTSTATUS (WINAPI *CSRSS_API_PROC)(PCSR_PROCESS ProcessData,
                                           PCSR_API_MESSAGE Request);

typedef struct _CSRSS_API_DEFINITION
{
  ULONG Type;
  ULONG MinRequestSize;
  CSRSS_API_PROC Handler;
} CSRSS_API_DEFINITION, *PCSRSS_API_DEFINITION;

#define CSRSS_DEFINE_API(Func, Handler) \
  { Func, sizeof(CSRSS_##Func), Handler }

typedef struct _CSRSS_LISTEN_DATA
{
  HANDLE ApiPortHandle;
  ULONG ApiDefinitionsCount;
  PCSRSS_API_DEFINITION *ApiDefinitions;
} CSRSS_LISTEN_DATA, *PCSRSS_LISTEN_DATA;

#define CSR_API(n) NTSTATUS WINAPI n (\
PCSR_PROCESS ProcessData,\
PCSR_API_MESSAGE Request)

/* init.c */
extern HANDLE hBootstrapOk;
NTSTATUS NTAPI CsrServerInitialization(ULONG ArgumentCount, PCHAR Arguments[]);

/* api/process.c */
CSR_API(CsrConnectProcess);
CSR_API(CsrSrvCreateProcess);
CSR_API(CsrTerminateProcess);
CSR_API(CsrSrvCreateThread);
CSR_API(CsrGetShutdownParameters);
CSR_API(CsrSetShutdownParameters);

VOID
NTAPI
CsrSetBackgroundPriority(IN PCSR_PROCESS CsrProcess);

PCSR_THREAD
NTAPI
CsrAllocateThread(IN PCSR_PROCESS CsrProcess);

PCSR_PROCESS
NTAPI
CsrAllocateProcess(VOID);

VOID
NTAPI
CsrDeallocateProcess(IN PCSR_PROCESS CsrProcess);

VOID
NTAPI
CsrRemoveProcess(IN PCSR_PROCESS CsrProcess);

VOID
NTAPI
CsrDereferenceThread(IN PCSR_THREAD CsrThread);

VOID
NTAPI
CsrInsertProcess(IN PCSR_PROCESS Parent OPTIONAL,
                 IN PCSR_PROCESS CurrentProcess OPTIONAL,
                 IN PCSR_PROCESS CsrProcess);


/* api/wapi.c */
NTSTATUS FASTCALL CsrApiRegisterDefinitions(PCSRSS_API_DEFINITION NewDefinitions);
VOID FASTCALL CsrApiCallHandler(PCSR_PROCESS ProcessData,
                                PCSR_API_MESSAGE Request);
VOID WINAPI CsrSbApiRequestThread (PVOID PortHandle);
VOID NTAPI ClientConnectionThread(HANDLE ServerPort);

VOID
NTAPI
CsrReleaseCapturedArguments(IN PCSR_API_MESSAGE ApiMessage);

extern HANDLE CsrApiPort;
extern HANDLE CsrSmApiPort;
extern HANDLE CsrSbApiPort;
extern LIST_ENTRY CsrThreadHashTable[256];
extern PCSR_PROCESS CsrRootProcess;
extern RTL_CRITICAL_SECTION ProcessDataLock, CsrWaitListsLock;
extern UNICODE_STRING CsrDirectoryName;
extern ULONG CsrDebug;
extern ULONG CsrTotalPerProcessDataLength;
extern SYSTEM_BASIC_INFORMATION CsrNtSysInfo;
extern PVOID CsrSrvSharedSectionHeap;
extern PVOID *CsrSrvSharedStaticServerData;
extern HANDLE CsrInitializationEvent;
extern PCSR_SERVER_DLL CsrLoadedServerDll[CSR_SERVER_DLL_MAX];
extern ULONG CsrMaxApiRequestThreads;

NTSTATUS
NTAPI
CsrApiPortInitialize(VOID);

NTSTATUS
NTAPI
CsrCreateProcess(IN HANDLE hProcess,
                 IN HANDLE hThread,
                 IN PCLIENT_ID ClientId,
                 IN PCSR_NT_SESSION NtSession,
                 IN ULONG Flags,
                 IN PCLIENT_ID DebugCid);

BOOLEAN
NTAPI
ProtectHandle(IN HANDLE ObjectHandle);

VOID
NTAPI
CsrInsertThread(IN PCSR_PROCESS Process,
IN PCSR_THREAD Thread);

VOID
NTAPI
CsrLockedReferenceThread(IN PCSR_THREAD CsrThread);

/* api/process.c */
typedef NTSTATUS (WINAPI *CSRSS_ENUM_PROCESS_PROC)(PCSR_PROCESS ProcessData,
                                                    PVOID Context);
NTSTATUS WINAPI CsrInitializeProcessStructure(VOID);

NTSTATUS WINAPI CsrEnumProcesses(CSRSS_ENUM_PROCESS_PROC EnumProc, PVOID Context);
PCSR_THREAD NTAPI CsrAddStaticServerThread(IN HANDLE hThread, IN PCLIENT_ID ClientId, IN  ULONG ThreadFlags);
PCSR_THREAD NTAPI CsrLocateThreadInProcess(IN PCSR_PROCESS CsrProcess OPTIONAL, IN PCLIENT_ID Cid);
PCSR_THREAD NTAPI CsrLocateThreadByClientId(OUT PCSR_PROCESS *Process OPTIONAL, IN PCLIENT_ID ClientId);
NTSTATUS NTAPI CsrLockProcessByClientId(IN HANDLE Pid, OUT PCSR_PROCESS *CsrProcess OPTIONAL);
NTSTATUS NTAPI CsrCreateThread(IN PCSR_PROCESS CsrProcess, IN HANDLE hThread, IN PCLIENT_ID ClientId);
NTSTATUS NTAPI CsrUnlockProcess(IN PCSR_PROCESS CsrProcess);

//hack
VOID NTAPI CsrThreadRefcountZero(IN PCSR_THREAD CsrThread);

NTSTATUS
NTAPI
CsrInitializeNtSessionList(VOID);

NTSTATUS
NTAPI
CsrSrvAttachSharedSection(IN PCSR_PROCESS CsrProcess OPTIONAL,
OUT PCSR_CONNECTION_INFO ConnectInfo);

NTSTATUS
NTAPI
CsrSrvCreateSharedSection(IN PCHAR ParameterValue);

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
CsrDestroyProcess(IN PCLIENT_ID Cid,
IN NTSTATUS ExitStatus);

NTSTATUS
NTAPI
CsrDestroyThread(IN PCLIENT_ID Cid);

VOID
NTAPI
CsrLockedDereferenceThread(IN PCSR_THREAD CsrThread);

BOOLEAN
NTAPI
CsrNotifyWaitBlock(IN PCSR_WAIT_BLOCK WaitBlock,
                   IN PLIST_ENTRY WaitList,
                   IN PVOID WaitArgument1,
                   IN PVOID WaitArgument2,
                   IN ULONG WaitFlags,
                   IN BOOLEAN DereferenceThread);
                   
VOID
NTAPI
CsrReferenceNtSession(IN PCSR_NT_SESSION Session);

LONG
NTAPI
CsrUnhandledExceptionFilter(IN PEXCEPTION_POINTERS ExceptionInfo);

VOID
NTAPI
CsrDereferenceNtSession(IN PCSR_NT_SESSION Session,
IN NTSTATUS ExitStatus);

VOID
NTAPI
CsrLockedDereferenceProcess(PCSR_PROCESS CsrProcess);

VOID
NTAPI
CsrDereferenceProcess(IN PCSR_PROCESS CsrProcess);

NTSTATUS
NTAPI
CsrLoadServerDll(IN PCHAR DllString,
                 IN PCHAR EntryPoint OPTIONAL,
                 IN ULONG ServerId);

/* api/user.c */
CSR_API(CsrRegisterServicesProcess);

/* EOF */
