/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/subsys/csr/server.h
 * PURPOSE:         Public Definitions for CSR Servers
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */
#ifndef _CSRSERVER_H
#define _CSRSERVER_H

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4201)
#endif

/* DEPENDENCIES **************************************************************/

/* TYPES **********************************************************************/
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
    PVOID ServerData[ANYSIZE_ARRAY];
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

/* ENUMERATIONS **************************************************************/
#define CSR_SRV_SERVER 0

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

/* FUNCTION TYPES ************************************************************/
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


/* FIXME: Put into public NDK Header */
typedef ULONG CSR_API_NUMBER;

#define CSR_MAKE_OPCODE(s,m) ((s) << 16) | (m)
#define CSR_API_ID_FROM_OPCODE(n) ((ULONG)((USHORT)(n)))
#define CSR_SERVER_ID_FROM_OPCODE(n) (ULONG)((n) >> 16)

typedef struct _CSR_CONNECTION_INFO
{
    ULONG Unknown[2];
    HANDLE ObjectDirectory;
    PVOID SharedSectionBase;
    PVOID SharedSectionHeap;
    PVOID SharedSectionData;
    ULONG DebugFlags;
    ULONG Unknown2[3];
    HANDLE ProcessId;
} CSR_CONNECTION_INFO, *PCSR_CONNECTION_INFO;

typedef struct _CSR_CLIENT_CONNECT
{
    ULONG ServerId;
    PVOID ConnectionInfo;
    ULONG ConnectionInfoSize;
} CSR_CLIENT_CONNECT, *PCSR_CLIENT_CONNECT;

typedef struct _CSR_API_MESSAGE
{
    PORT_MESSAGE Header;
    union
    {
        CSR_CONNECTION_INFO ConnectionInfo;
        struct
        {
            PVOID CsrCaptureData;
            CSR_API_NUMBER Opcode;
            ULONG Status;
            ULONG Reserved;
            union
            {
                CSR_CLIENT_CONNECT CsrClientConnect;
            };
        };
    };
} CSR_API_MESSAGE, *PCSR_API_MESSAGE;

typedef struct _CSR_CAPTURE_BUFFER
{
    ULONG Size;
    struct _CSR_CAPTURE_BUFFER *PreviousCaptureBuffer;
    ULONG PointerCount;
    ULONG_PTR BufferEnd;
} CSR_CAPTURE_BUFFER, *PCSR_CAPTURE_BUFFER;

/* Private data resumes here */
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

/* PROTOTYPES ****************************************************************/

NTSTATUS
NTAPI
CsrServerInitialization(
    ULONG ArgumentCount,
    PCHAR Arguments[]
);

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
