/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/include/api.h
 * PURPOSE:         CSRSS API interface
 */

#pragma once

#define NTOS_MODE_USER
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>

#include <csrss/server.h>


#define CsrAcquireProcessLock() \
    RtlEnterCriticalSection(&CsrProcessLock);

#define CsrReleaseProcessLock() \
    RtlLeaveCriticalSection(&CsrProcessLock);

#define ProcessStructureListLocked() \
    (CsrProcessLock.OwningThread == NtCurrentTeb()->ClientId.UniqueThread)

#define CsrAcquireWaitLock() \
    RtlEnterCriticalSection(&CsrWaitListsLock);

#define CsrReleaseWaitLock() \
    RtlLeaveCriticalSection(&CsrWaitListsLock);

#define CsrAcquireNtSessionLock() \
    RtlEnterCriticalSection(&CsrNtSessionLock);

#define CsrReleaseNtSessionLock() \
    RtlLeaveCriticalSection(&CsrNtSessionLock);



typedef struct _CSRSS_API_DEFINITION
{
    ULONG ApiID;
    ULONG MinRequestSize;
    PCSR_API_ROUTINE Handler;
} CSRSS_API_DEFINITION, *PCSRSS_API_DEFINITION;

#define CSRSS_DEFINE_API(Func, Handler) \
    { Func, sizeof(CSRSS_##Func), Handler }



typedef struct _CSRSS_LISTEN_DATA
{
    HANDLE ApiPortHandle;
    ULONG ApiDefinitionsCount;
    PCSRSS_API_DEFINITION *ApiDefinitions;
} CSRSS_LISTEN_DATA, *PCSRSS_LISTEN_DATA;




/******************************************************************************
 ******************************************************************************
 ******************************************************************************/



/* init.c */
extern HANDLE hBootstrapOk;
NTSTATUS NTAPI CsrServerInitialization(ULONG ArgumentCount, PCHAR Arguments[]);

/* api/process.c */
CSR_API(CsrConnectProcess);
CSR_API(BaseSrvCreateProcess);
CSR_API(BaseSrvExitProcess);
CSR_API(BaseSrvCreateThread);
CSR_API(BaseSrvGetProcessShutdownParam);
CSR_API(BaseSrvSetProcessShutdownParam);

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
VOID FASTCALL CsrApiCallHandler(IN OUT PCSR_API_MESSAGE ApiMessage, OUT PULONG Reply);
VOID WINAPI CsrSbApiRequestThread (PVOID PortHandle);
VOID NTAPI ClientConnectionThread(HANDLE ServerPort);

extern HANDLE CsrApiPort;
extern HANDLE CsrSmApiPort;
extern HANDLE CsrSbApiPort;
extern LIST_ENTRY CsrThreadHashTable[256];
extern PCSR_PROCESS CsrRootProcess;
extern RTL_CRITICAL_SECTION CsrProcessLock, CsrWaitListsLock;
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
CSR_API(SrvRegisterServicesProcess);

/* EOF */
