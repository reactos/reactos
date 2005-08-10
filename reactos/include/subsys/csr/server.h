#if !defined(__INCLUDE_CSR_SERVER_H)
#define __INCLUDE_CSR_SERVER_H

#define CSR_SRV_SERVER 0

typedef struct _CSR_SESSION
{
	ULONG SessionId;

} CSR_SESSION, * PCSR_SESSION;


typedef struct _CSR_PROCESS
{
	HANDLE        Process;
	ULONG         ReferenceCount;
	
} CSR_PROCESS, * PCSR_PROCESS;

typedef struct _CSR_THREAD
{
	HANDLE        Thread;
	PCSR_SESSION  CsrSession;
	PCSR_PROCESS  CsrProcess;
	ULONG         ReferenceCount;
	
} CSR_THREAD, * PCSR_THREAD;

typedef struct _CSR_WAIT
{
	PCSR_PROCESS CsrThread;

} CSR_WAIT, * PCSR_WAIT;

/* symbols exported by native DLL csrsrv.dll */

NTSTATUS STDCALL CsrAddStaticServerThread();
NTSTATUS STDCALL CsrCallServerFromServer();
NTSTATUS STDCALL CsrConnectToUser();
NTSTATUS STDCALL CsrCreateProcess();
NTSTATUS STDCALL CsrCreateRemoteThread();
NTSTATUS STDCALL CsrCreateSession();
NTSTATUS STDCALL CsrCreateThread();
NTSTATUS STDCALL CsrCreateWait();
NTSTATUS STDCALL CsrDebugProcess();
NTSTATUS STDCALL CsrDebugProcessStop();
NTSTATUS STDCALL CsrDereferenceProcess();
NTSTATUS STDCALL CsrDereferenceThread();
NTSTATUS STDCALL CsrDereferenceWait();
NTSTATUS STDCALL CsrDestroyProcess();
NTSTATUS STDCALL CsrDestroyThread();
NTSTATUS STDCALL CsrExecServerThread();
NTSTATUS STDCALL CsrGetApiPorts(PHANDLE,PHANDLE);
NTSTATUS STDCALL CsrGetProcessLuid();
NTSTATUS STDCALL CsrImpersonateClient();
NTSTATUS STDCALL CsrLockProcessByClientId();
NTSTATUS STDCALL CsrLockThreadByClientId();
NTSTATUS STDCALL CsrMoveSatisfiedWait();
NTSTATUS STDCALL CsrNotifyWait();
NTSTATUS STDCALL CsrPopulateDosDevices();
HANDLE   STDCALL CsrQueryApiPort(VOID);
NTSTATUS STDCALL CsrReferenceThread();
NTSTATUS STDCALL CsrRevertToSelf();
NTSTATUS STDCALL CsrServerInitialization(ULONG,LPWSTR*);
NTSTATUS STDCALL CsrSetBackgroundPriority();
NTSTATUS STDCALL CsrSetCallingSpooler();
NTSTATUS STDCALL CsrSetForegroundPriority();
NTSTATUS STDCALL CsrShutdownProcesses();
NTSTATUS STDCALL CsrUnhandledExceptionFilter();
NTSTATUS STDCALL CsrUnlockProcess();
NTSTATUS STDCALL CsrUnlockThread();
NTSTATUS STDCALL CsrValidateMessageBuffer();
NTSTATUS STDCALL CsrValidateMessageString();

#endif /* ndef __INCLUDE_CSR_SERVER_H */
