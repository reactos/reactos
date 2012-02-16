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

#define CSR_SERVER_DLL_MAX 4
#define LOCK   RtlEnterCriticalSection(&ProcessDataLock)
#define UNLOCK RtlLeaveCriticalSection(&ProcessDataLock)
#define CsrAcquireProcessLock() LOCK
#define CsrReleaseProcessLock() UNLOCK
#define ProcessStructureListLocked() \
    (ProcessDataLock.OwningThread == NtCurrentTeb()->ClientId.UniqueThread)

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

typedef enum _CSR_PROCESS_FLAGS
{
    CsrProcessTerminating = 0x1,
    CsrProcessSkipShutdown = 0x2,
    CsrProcessCreateNewGroup = 0x100,
    CsrProcessTerminated = 0x200,
    CsrProcessLastThreadTerminated = 0x400,
    CsrProcessIsConsoleApp = 0x800
} CSR_PROCESS_FLAGS, *PCSR_PROCESS_FLAGS;

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

typedef struct _CSR_PROCESS
{
    CLIENT_ID ClientId;
    LIST_ENTRY ListLink;
    LIST_ENTRY ThreadList;
    struct _CSR_PROCESS *Parent;
//    PCSR_NT_SESSION NtSession;
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
    //struct _CSR_WAIT_BLOCK *WaitBlock;
    HANDLE ThreadHandle;
    ULONG Flags;
    ULONG ReferenceCount;
    ULONG ImpersonationCount;
} CSR_THREAD, *PCSR_THREAD;

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
CSR_API(CsrCreateProcess);
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
CsrInsertProcess(IN PCSR_PROCESS Parent OPTIONAL,
                 IN PCSR_PROCESS CurrentProcess OPTIONAL,
                 IN PCSR_PROCESS CsrProcess);


/* api/wapi.c */
NTSTATUS FASTCALL CsrApiRegisterDefinitions(PCSRSS_API_DEFINITION NewDefinitions);
VOID FASTCALL CsrApiCallHandler(PCSR_PROCESS ProcessData,
                                PCSR_API_MESSAGE Request);
VOID WINAPI CsrSbApiRequestThread (PVOID PortHandle);
VOID NTAPI ClientConnectionThread(HANDLE ServerPort);

extern HANDLE CsrSbApiPort;
extern LIST_ENTRY CsrThreadHashTable[256];
extern PCSR_PROCESS CsrRootProcess;
extern RTL_CRITICAL_SECTION ProcessDataLock, CsrWaitListsLock;

BOOLEAN
NTAPI
ProtectHandle(IN HANDLE ObjectHandle);

VOID
NTAPI
CsrInsertThread(IN PCSR_PROCESS Process,
IN PCSR_THREAD Thread);

/* api/process.c */
typedef NTSTATUS (WINAPI *CSRSS_ENUM_PROCESS_PROC)(PCSR_PROCESS ProcessData,
                                                    PVOID Context);
NTSTATUS WINAPI CsrInitializeProcessStructure(VOID);
PCSR_PROCESS WINAPI CsrGetProcessData(HANDLE ProcessId);
PCSR_PROCESS WINAPI CsrCreateProcessData(HANDLE ProcessId);
NTSTATUS WINAPI CsrFreeProcessData( HANDLE Pid );
NTSTATUS WINAPI CsrEnumProcesses(CSRSS_ENUM_PROCESS_PROC EnumProc, PVOID Context);
PCSR_THREAD NTAPI CsrAddStaticServerThread(IN HANDLE hThread, IN PCLIENT_ID ClientId, IN  ULONG ThreadFlags);
PCSR_THREAD NTAPI CsrLocateThreadInProcess(IN PCSR_PROCESS CsrProcess OPTIONAL, IN PCLIENT_ID Cid);
PCSR_THREAD NTAPI CsrLocateThreadByClientId(OUT PCSR_PROCESS *Process OPTIONAL, IN PCLIENT_ID ClientId);
NTSTATUS NTAPI CsrLockProcessByClientId(IN HANDLE Pid, OUT PCSR_PROCESS *CsrProcess OPTIONAL);
NTSTATUS NTAPI CsrCreateThread(IN PCSR_PROCESS CsrProcess, IN HANDLE hThread, IN PCLIENT_ID ClientId);
NTSTATUS NTAPI CsrUnlockProcess(IN PCSR_PROCESS CsrProcess);

//hack
VOID NTAPI CsrThreadRefcountZero(IN PCSR_THREAD CsrThread);

/* api/user.c */
CSR_API(CsrRegisterServicesProcess);

/* EOF */
