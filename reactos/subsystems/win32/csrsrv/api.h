/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsystems/win32/csrsrv/api.h
 * PURPOSE:         CSRSS Internal API
 */

#pragma once

#define NTOS_MODE_USER
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>

#include <csr/csrsrv.h>


extern RTL_CRITICAL_SECTION CsrProcessLock, CsrWaitListsLock;

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


#define CSR_SERVER_DLL_MAX  4


extern HANDLE hBootstrapOk;
extern HANDLE CsrApiPort;
extern HANDLE CsrSmApiPort;
extern HANDLE CsrSbApiPort;
extern LIST_ENTRY CsrThreadHashTable[256];
extern PCSR_PROCESS CsrRootProcess;
extern UNICODE_STRING CsrDirectoryName;
extern ULONG CsrDebug;
extern ULONG CsrTotalPerProcessDataLength;
extern SYSTEM_BASIC_INFORMATION CsrNtSysInfo;
extern HANDLE CsrHeap;
extern PVOID CsrSrvSharedSectionHeap;
extern PVOID *CsrSrvSharedStaticServerData;
extern HANDLE CsrInitializationEvent;
extern PCSR_SERVER_DLL CsrLoadedServerDll[CSR_SERVER_DLL_MAX];
extern ULONG CsrMaxApiRequestThreads;

/****************************************************/
extern UNICODE_STRING CsrSbApiPortName;
extern UNICODE_STRING CsrApiPortName;
extern RTL_CRITICAL_SECTION CsrProcessLock;
extern RTL_CRITICAL_SECTION CsrWaitListsLock;
extern HANDLE CsrObjectDirectory;
/****************************************************/



CSR_API(CsrSrvClientConnect);
CSR_API(CsrSrvUnusedFunction);
CSR_API(CsrSrvIdentifyAlertableThread);
CSR_API(CsrSrvSetPriorityClass);


NTSTATUS
NTAPI
CsrServerDllInitialization(IN PCSR_SERVER_DLL LoadedServerDll);


BOOLEAN
NTAPI
CsrCaptureArguments(IN PCSR_THREAD CsrThread,
                    IN PCSR_API_MESSAGE ApiMessage);

VOID
NTAPI
CsrReleaseCapturedArguments(IN PCSR_API_MESSAGE ApiMessage);

NTSTATUS
NTAPI
CsrLoadServerDll(IN PCHAR DllString,
                 IN PCHAR EntryPoint OPTIONAL,
                 IN ULONG ServerId);


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
CsrInsertProcess(IN PCSR_PROCESS ParentProcess OPTIONAL,
                 IN PCSR_PROCESS CsrProcess);

VOID
NTAPI
CsrRemoveProcess(IN PCSR_PROCESS CsrProcess);

NTSTATUS
NTAPI
CsrApiRequestThread(IN PVOID Parameter);

VOID
NTAPI
CsrSbApiRequestThread(IN PVOID Parameter);

NTSTATUS
NTAPI
CsrApiPortInitialize(VOID);

BOOLEAN
NTAPI
ProtectHandle(IN HANDLE ObjectHandle);

BOOLEAN
NTAPI
UnProtectHandle(IN HANDLE ObjectHandle);

VOID
NTAPI
CsrInsertThread(IN PCSR_PROCESS Process,
                IN PCSR_THREAD Thread);

VOID
NTAPI
CsrLockedReferenceProcess(IN PCSR_PROCESS CsrProcess);

VOID
NTAPI
CsrLockedReferenceThread(IN PCSR_THREAD CsrThread);

NTSTATUS
NTAPI
CsrInitializeProcessStructure(VOID);

// NTSTATUS WINAPI CsrEnumProcesses(CSRSS_ENUM_PROCESS_PROC EnumProc,
//                                  PVOID Context);
PCSR_THREAD
NTAPI
CsrLocateThreadInProcess(IN PCSR_PROCESS CsrProcess OPTIONAL,
                         IN PCLIENT_ID Cid);
PCSR_THREAD
NTAPI
CsrLocateThreadByClientId(OUT PCSR_PROCESS *Process OPTIONAL,
                          IN PCLIENT_ID ClientId);

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

VOID
NTAPI
CsrLockedDereferenceProcess(PCSR_PROCESS CsrProcess);

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

VOID
NTAPI
CsrDereferenceNtSession(IN PCSR_NT_SESSION Session,
                        IN NTSTATUS ExitStatus);

/******************************************************************************
 ******************************************************************************/

NTSTATUS
NTAPI
CsrCreateSessionObjectDirectory(IN ULONG SessionId);

NTSTATUS
NTAPI
CsrCreateObjectDirectory(IN PCHAR ObjectDirectory);

NTSTATUS
NTAPI
CsrSbApiPortInitialize(VOID);

BOOLEAN
NTAPI
CsrSbCreateSession(IN PSB_API_MSG ApiMessage);

BOOLEAN
NTAPI
CsrSbTerminateSession(IN PSB_API_MSG ApiMessage);

BOOLEAN
NTAPI
CsrSbForeignSessionComplete(IN PSB_API_MSG ApiMessage);

BOOLEAN
NTAPI
CsrSbCreateProcess(IN PSB_API_MSG ApiMessage);

NTSTATUS
NTAPI
CsrSbApiHandleConnectionRequest(IN PSB_API_MSG Message);

NTSTATUS
NTAPI
CsrApiHandleConnectionRequest(IN PCSR_API_MESSAGE ApiMessage);

/** this API is used with CsrPopulateDosDevices, deprecated in r55585.
NTSTATUS
NTAPI
CsrPopulateDosDevicesDirectory(IN HANDLE DosDevicesDirectory,
                               IN PPROCESS_DEVICEMAP_INFORMATION DeviceMap);
**/

NTSTATUS
NTAPI
CsrCreateLocalSystemSD(OUT PSECURITY_DESCRIPTOR *LocalSystemSd);

NTSTATUS
NTAPI
CsrSetDirectorySecurity(IN HANDLE ObjectDirectory);

/* EOF */
