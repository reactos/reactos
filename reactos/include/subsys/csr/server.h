#if !defined(__INCLUDE_CSR_SERVER_H)
#define __INCLUDE_CSR_SERVER_H

#define CSR_SRV_SERVER 0

typedef struct _CSR_SESSION
{
	ULONG   SessionId;
	HANDLE  Heap;
	PVOID   ServerData;

} CSR_SESSION, * PCSR_SESSION;


typedef struct _CSR_PROCESS
{
	CLIENT_ID     ClientId;
	LIST_ENTRY    ListLink;
	LIST_ENTRY    ThreadList;
	PCSR_SESSION  NtSession;
	ULONG         ExpectedVersion;
	HANDLE        ClientPort;
	ULONG_PTR     ClientViewBase;
	ULONG_PTR     ClientViewBounds;
	HANDLE        ProcessHandle;
	ULONG         SequenceNumber;
	ULONG         Flags;
	ULONG         DebugFlags;
	ULONG         ReferenceCount;
	ULONG         ProcessGroupId;
	ULONG         ProcessGroupSequence;
	ULONG         fVDM;
	ULONG         ThreadCount;
	ULONG         PriorityClass;
	ULONG         Reserved;
	ULONG         ShutdownLevel;
	ULONG         ShutdownFlags;
	PVOID         ServerData; 
	
} CSR_PROCESS, * PCSR_PROCESS;

struct _CSR_WAIT;

typedef struct _CSR_THREAD
{
	LARGE_INTEGER     CreateTime;
	LIST_ENTRY        Link;
	LIST_ENTRY        HashLinks;
	CLIENT_ID         ClientId;
	PCSR_PROCESS      Process;
	struct _CSR_WAIT  *WaitBlock;
	HANDLE            ThreadHandle;
	ULONG             Flags;
	ULONG             ReferenceCount;
	ULONG             ImpersonationCount; 
	
} CSR_THREAD, * PCSR_THREAD;

typedef struct _CSR_WAIT
{
	PCSR_THREAD CsrThread;

} CSR_WAIT, * PCSR_WAIT;


typedef VOID (CALLBACK * CSR_SERVER_THREAD)(PVOID);

/* symbols exported by native DLL csrsrv.dll */

NTSTATUS STDCALL CsrAddStaticServerThread(CSR_SERVER_THREAD);
NTSTATUS STDCALL CsrCallServerFromServer();
NTSTATUS STDCALL CsrCreateProcess(PCSR_SESSION,PCSR_PROCESS*);
NTSTATUS STDCALL CsrCreateRemoteThread();
NTSTATUS STDCALL CsrCreateSession(PCSR_SESSION*);
NTSTATUS STDCALL CsrCreateThread(PCSR_PROCESS,PCSR_THREAD*);
NTSTATUS STDCALL CsrCreateWait(PCSR_THREAD,PCSR_WAIT*);
NTSTATUS STDCALL CsrDebugProcess(PCSR_PROCESS);
NTSTATUS STDCALL CsrDebugProcessStop(PCSR_PROCESS);
NTSTATUS STDCALL CsrDereferenceProcess(PCSR_PROCESS);
NTSTATUS STDCALL CsrDereferenceThread(PCSR_THREAD);
NTSTATUS STDCALL CsrDereferenceWait(PCSR_WAIT);
NTSTATUS STDCALL CsrDestroyProcess(PCSR_PROCESS);
NTSTATUS STDCALL CsrDestroySession (PCSR_SESSION);
NTSTATUS STDCALL CsrDestroyThread(PCSR_THREAD);
NTSTATUS STDCALL CsrExecServerThread();
NTSTATUS STDCALL CsrGetProcessLuid(PCSR_PROCESS,PLUID);
NTSTATUS STDCALL CsrImpersonateClient();
NTSTATUS STDCALL CsrLockProcessByClientId();
NTSTATUS STDCALL CsrLockThreadByClientId();
NTSTATUS STDCALL CsrMoveSatisfiedWait(PCSR_WAIT);
NTSTATUS STDCALL CsrNotifyWait(PCSR_WAIT);
HANDLE   STDCALL CsrQueryApiPort(VOID);
NTSTATUS STDCALL CsrReferenceThread(PCSR_THREAD);
NTSTATUS STDCALL CsrRevertToSelf();
NTSTATUS STDCALL CsrServerInitialization(ULONG,LPWSTR*);
NTSTATUS STDCALL CsrSetBackgroundPriority();
NTSTATUS STDCALL CsrSetCallingSpooler();
NTSTATUS STDCALL CsrSetForegroundPriority();
NTSTATUS STDCALL CsrShutdownProcesses(PCSR_SESSION);
NTSTATUS STDCALL CsrUnhandledExceptionFilter();
NTSTATUS STDCALL CsrUnlockProcess(PCSR_PROCESS);
NTSTATUS STDCALL CsrUnlockThread(PCSR_THREAD);
NTSTATUS STDCALL CsrValidateMessageBuffer();
NTSTATUS STDCALL CsrValidateMessageString();

#endif /* ndef __INCLUDE_CSR_SERVER_H */
