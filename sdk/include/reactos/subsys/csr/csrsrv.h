/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Client/Server Runtime SubSystem
 * FILE:            include/reactos/subsys/csr/csrsrv.h
 * PURPOSE:         Public definitions for CSR Servers
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _CSRSRV_H
#define _CSRSRV_H

/*
 * The CSR_DBG macro is defined for building CSR Servers
 * with extended debugging information.
 */
#if DBG
#define CSR_DBG
#endif

#include "csrmsg.h"


/* TYPES **********************************************************************/

// Used in csr/connect.c
#define CSR_CSRSS_SECTION_SIZE  65536

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
    PVOID ServerData[ANYSIZE_ARRAY];    // One structure per CSR server.
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

#define CsrGetClientThread() \
    ((PCSR_THREAD)(NtCurrentTeb()->CsrClientThread))


/* ENUMERATIONS ***************************************************************/

typedef enum _CSR_PROCESS_FLAGS
{
    CsrProcessTerminating          = 0x1,
    CsrProcessSkipShutdown         = 0x2,
    CsrProcessNormalPriority       = 0x10,
    CsrProcessIdlePriority         = 0x20,
    CsrProcessHighPriority         = 0x40,
    CsrProcessRealtimePriority     = 0x80,
    CsrProcessCreateNewGroup       = 0x100,
    CsrProcessTerminated           = 0x200,
    CsrProcessLastThreadTerminated = 0x400,
    CsrProcessIsConsoleApp         = 0x800
} CSR_PROCESS_FLAGS, *PCSR_PROCESS_FLAGS;

#define CsrProcessPriorityFlags (CsrProcessNormalPriority | \
                                 CsrProcessIdlePriority   | \
                                 CsrProcessHighPriority   | \
                                 CsrProcessRealtimePriority)

typedef enum _CSR_THREAD_FLAGS
{
    CsrThreadAlertable      = 0x1,
    CsrThreadInTermination  = 0x2,
    CsrThreadTerminated     = 0x4,
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
    CsrShutdownOther  = 8
} CSR_SHUTDOWN_FLAGS, *PCSR_SHUTDOWN_FLAGS;

typedef enum _CSR_DEBUG_FLAGS
{
    CsrDebugOnlyThisProcess = 1,
    CsrDebugProcessChildren = 2
} CSR_PROCESS_DEBUG_FLAGS, *PCSR_PROCESS_DEBUG_FLAGS;

typedef enum _CSR_REPLY_CODE
{
    CsrReplyImmediately = 0,
    CsrReplyPending     = 1,
    CsrReplyDeadClient  = 2,
    CsrReplyAlreadySent = 3
} CSR_REPLY_CODE, *PCSR_REPLY_CODE;


/* FUNCTION TYPES AND STRUCTURES **********************************************/

/*
 * Wait block
 */
typedef
BOOLEAN
(NTAPI *CSR_WAIT_FUNCTION)(
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
    ULONG Size;                     // Size of the wait block (variable-sized)
    LIST_ENTRY WaitList;
    PVOID WaitContext;
    PCSR_THREAD WaitThread;
    CSR_WAIT_FUNCTION WaitFunction;
    CSR_API_MESSAGE WaitApiMessage; // Variable-sized CSR API message
} CSR_WAIT_BLOCK, *PCSR_WAIT_BLOCK;


/*
 * Server DLL structure
 */
typedef
NTSTATUS
(NTAPI *PCSR_API_ROUTINE)(
    IN OUT PCSR_API_MESSAGE ApiMessage,
    IN OUT PCSR_REPLY_CODE  ReplyCode OPTIONAL
);

#define CSR_API(n)                                          \
    NTSTATUS NTAPI n(IN OUT PCSR_API_MESSAGE ApiMessage,    \
                     IN OUT PCSR_REPLY_CODE  ReplyCode OPTIONAL)

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

// See http://redplait.blogspot.fr/2011/07/csrserverdll.html
typedef struct _CSR_SERVER_DLL
{
    ULONG Length;
    ANSI_STRING Name;
    HANDLE ServerHandle;
    ULONG ServerId;
    ULONG Unknown;
    ULONG ApiBase;
    ULONG HighestApiSupported;
    PCSR_API_ROUTINE *DispatchTable;
    PBOOLEAN ValidTable; // Table of booleans which describe whether or not a server function call is valid when it is called via CsrCallServerFromServer.
/*
 * On Windows Server 2003, CSR Servers contain
 * the API Names Table only in Debug Builds.
 */
#ifdef CSR_DBG
    PCHAR *NameTable;
#endif

    ULONG SizeOfProcessData;
    PCSR_CONNECT_CALLBACK ConnectCallback;
    PCSR_DISCONNECT_CALLBACK DisconnectCallback;
    PCSR_HARDERROR_CALLBACK HardErrorCallback;
    PVOID SharedSection;
    PCSR_NEWPROCESS_CALLBACK NewProcessCallback;
    PCSR_SHUTDOWNPROCESS_CALLBACK ShutdownProcessCallback;
    ULONG Unknown2[3];
} CSR_SERVER_DLL, *PCSR_SERVER_DLL;
#ifndef _WIN64
    #ifdef CSR_DBG
        C_ASSERT(FIELD_OFFSET(CSR_SERVER_DLL, SharedSection) == 0x3C);
    #else
        C_ASSERT(FIELD_OFFSET(CSR_SERVER_DLL, SharedSection) == 0x38);
    #endif
#endif

typedef
NTSTATUS
(NTAPI *PCSR_SERVER_DLL_INIT_CALLBACK)(IN PCSR_SERVER_DLL LoadedServerDll);

#define CSR_SERVER_DLL_INIT(n)  \
    NTSTATUS NTAPI n(IN PCSR_SERVER_DLL LoadedServerDll)


/* PROTOTYPES ****************************************************************/

NTSTATUS
NTAPI
CsrServerInitialization(IN ULONG ArgumentCount,
                        IN PCHAR Arguments[]);

PCSR_THREAD
NTAPI
CsrAddStaticServerThread(IN HANDLE hThread,
                         IN PCLIENT_ID ClientId,
                         IN ULONG ThreadFlags);

NTSTATUS
NTAPI
CsrCallServerFromServer(IN PCSR_API_MESSAGE ReceiveMsg,
                        IN OUT PCSR_API_MESSAGE ReplyMsg);

PCSR_THREAD
NTAPI
CsrConnectToUser(VOID);

NTSTATUS
NTAPI
CsrCreateProcess(IN HANDLE hProcess,
                 IN HANDLE hThread,
                 IN PCLIENT_ID ClientId,
                 IN PCSR_NT_SESSION NtSession,
                 IN ULONG Flags,
                 IN PCLIENT_ID DebugCid);

NTSTATUS
NTAPI
CsrCreateRemoteThread(IN HANDLE hThread,
                      IN PCLIENT_ID ClientId);

NTSTATUS
NTAPI
CsrCreateThread(IN PCSR_PROCESS CsrProcess,
                IN HANDLE hThread,
                IN PCLIENT_ID ClientId,
                IN BOOLEAN HaveClient);

BOOLEAN
NTAPI
CsrCreateWait(IN PLIST_ENTRY WaitList,
              IN CSR_WAIT_FUNCTION WaitFunction,
              IN PCSR_THREAD CsrWaitThread,
              IN OUT PCSR_API_MESSAGE WaitApiMessage,
              IN PVOID WaitContext);

NTSTATUS
NTAPI
CsrDebugProcess(IN PCSR_PROCESS CsrProcess);

NTSTATUS
NTAPI
CsrDebugProcessStop(IN PCSR_PROCESS CsrProcess);

VOID
NTAPI
CsrDereferenceProcess(IN PCSR_PROCESS CsrProcess);

VOID
NTAPI
CsrDereferenceThread(IN PCSR_THREAD CsrThread);

VOID
NTAPI
CsrDereferenceWait(IN PLIST_ENTRY WaitList);

NTSTATUS
NTAPI
CsrDestroyProcess(IN PCLIENT_ID Cid,
                  IN NTSTATUS ExitStatus);

NTSTATUS
NTAPI
CsrDestroyThread(IN PCLIENT_ID Cid);

NTSTATUS
NTAPI
CsrExecServerThread(IN PVOID ThreadHandler,
                    IN ULONG Flags);

NTSTATUS
NTAPI
CsrGetProcessLuid(IN HANDLE hProcess OPTIONAL,
                  OUT PLUID Luid);

BOOLEAN
NTAPI
CsrImpersonateClient(IN PCSR_THREAD CsrThread);

NTSTATUS
NTAPI
CsrLockProcessByClientId(IN HANDLE Pid,
                         OUT PCSR_PROCESS *CsrProcess OPTIONAL);

NTSTATUS
NTAPI
CsrLockThreadByClientId(IN HANDLE Tid,
                        OUT PCSR_THREAD *CsrThread);

VOID
NTAPI
CsrMoveSatisfiedWait(IN PLIST_ENTRY DestinationList,
                     IN PLIST_ENTRY WaitList);

BOOLEAN
NTAPI
CsrNotifyWait(IN PLIST_ENTRY WaitList,
              IN BOOLEAN NotifyAll,
              IN PVOID WaitArgument1,
              IN PVOID WaitArgument2);

VOID
NTAPI
CsrPopulateDosDevices(VOID);

HANDLE
NTAPI
CsrQueryApiPort(VOID);

VOID
NTAPI
CsrReferenceThread(IN PCSR_THREAD CsrThread);

BOOLEAN
NTAPI
CsrRevertToSelf(VOID);

VOID
NTAPI
CsrSetBackgroundPriority(IN PCSR_PROCESS CsrProcess);

VOID
NTAPI
CsrSetCallingSpooler(ULONG Reserved);

VOID
NTAPI
CsrSetForegroundPriority(IN PCSR_PROCESS CsrProcess);

NTSTATUS
NTAPI
CsrShutdownProcesses(IN PLUID CallerLuid,
                     IN ULONG Flags);

EXCEPTION_DISPOSITION
NTAPI
CsrUnhandledExceptionFilter(IN PEXCEPTION_POINTERS ExceptionInfo);

NTSTATUS
NTAPI
CsrUnlockProcess(IN PCSR_PROCESS CsrProcess);

NTSTATUS
NTAPI
CsrUnlockThread(IN PCSR_THREAD CsrThread);

BOOLEAN
NTAPI
CsrValidateMessageBuffer(IN PCSR_API_MESSAGE ApiMessage,
                         IN PVOID *Buffer,
                         IN ULONG ElementCount,
                         IN ULONG ElementSize);

BOOLEAN
NTAPI
CsrValidateMessageString(IN PCSR_API_MESSAGE ApiMessage,
                         IN PWSTR *MessageString);

#endif // _CSRSRV_H

/* EOF */
