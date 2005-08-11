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
	HANDLE        Process;
	PCSR_SESSION  CsrSession;
	ULONG         ReferenceCount;
	PVOID         ServerData;
	
} CSR_PROCESS, * PCSR_PROCESS;

typedef struct _CSR_THREAD
{
	HANDLE        Thread;
	PCSR_SESSION  CsrSession;
	PCSR_PROCESS  CsrProcess;
	ULONG         ReferenceCount;
	PVOID         ServerData;
	
} CSR_THREAD, * PCSR_THREAD;

typedef struct _CSR_WAIT
{
	PCSR_PROCESS CsrThread;

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
