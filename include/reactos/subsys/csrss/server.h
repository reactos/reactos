/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/subsys/csrss/server.h
 * PURPOSE:         Public Definitions for CSR Servers
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _CSRSERVER_H
#define _CSRSERVER_H

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4201)
#endif

#include "msg.h"

/* TYPES **********************************************************************/

typedef struct _CSR_NT_SESSION
{
    ULONG ReferenceCount;
    LIST_ENTRY SessionLink;
    ULONG SessionId;
} CSR_NT_SESSION, *PCSR_NT_SESSION;

/*** old thingie, remove it later... (put it in winsrv -- console) ***/
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
/*********************************************************************/
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
    CSRSS_CON_PROCESS_DATA; //// FIXME: Remove it after we activate the previous member.
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
    CsrThreadAltertable     = 0x1,
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


/*
 * Wait block
 */
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


/*
 * Server DLL structure
 */
typedef
NTSTATUS
(NTAPI *PCSR_API_ROUTINE)(
    IN OUT PCSR_API_MESSAGE ApiMessage,
    OUT PULONG Reply
);

#define CSR_API(n) NTSTATUS NTAPI n (   \
    IN OUT PCSR_API_MESSAGE ApiMessage, \
    OUT PULONG Reply)

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


/* FUNCTION TYPES *************************************************************/

typedef
NTSTATUS
(NTAPI *PCSR_SERVER_DLL_INIT_CALLBACK)(IN PCSR_SERVER_DLL LoadedServerDll);

/*
NTSTATUS
NTAPI
CsrServerDllInitialization(IN PCSR_SERVER_DLL LoadedServerDll);
*/


/* PROTOTYPES ****************************************************************/

NTSTATUS
NTAPI
CsrServerInitialization(
    IN ULONG ArgumentCount,
    IN PCHAR Arguments[]
);

///////////
BOOLEAN
NTAPI
CsrCaptureArguments(
    IN PCSR_THREAD CsrThread,
    IN PCSR_API_MESSAGE ApiMessage
);

VOID
NTAPI
CsrReleaseCapturedArguments(IN PCSR_API_MESSAGE ApiMessage);
//////////

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

BOOLEAN
NTAPI
CsrImpersonateClient(IN PCSR_THREAD CsrThread);

BOOLEAN
NTAPI
CsrRevertToSelf(VOID);

VOID
NTAPI
CsrSetBackgroundPriority(IN PCSR_PROCESS CsrProcess);

LONG
NTAPI
CsrUnhandledExceptionFilter(
    IN PEXCEPTION_POINTERS ExceptionInfo
);



#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // _CSRSERVER_H

/* EOF */
