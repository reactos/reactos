#ifndef _SMSS_H_INCLUDED_
#define _SMSS_H_INCLUDED_

#define NTOS_MODE_USER
#include <ntos.h>
#include <sm/api.h>
#include <sm/helper.h>

#define CHILD_CSRSS     0
#define CHILD_WINLOGON  1

/* smss.c */
extern HANDLE SmSsProcessId;

/* init.c */
NTSTATUS InitSessionManager(VOID);

/* initheap.c */
extern HANDLE SmpHeap;
NTSTATUS SmCreateHeap(VOID);

/* initenv.c */
extern PWSTR SmSystemEnvironment;
NTSTATUS SmCreateEnvironment(VOID);
NTSTATUS SmSetEnvironmentVariables(VOID);
NTSTATUS SmUpdateEnvironment(VOID);

/* initobdir.c */
NTSTATUS SmCreateObjectDirectories(VOID);

/* initdosdev.c */
NTSTATUS SmInitDosDevices(VOID);

/* initrun.c */
extern HANDLE Children[2];
NTSTATUS SmRunBootApplications(VOID);

/* initmv.c */
NTSTATUS SmProcessFileRenameList(VOID);

/* initwkdll.c */
NTSTATUS SmLoadKnownDlls(VOID);

/* initpage.c */
NTSTATUS SmCreatePagingFiles(VOID);

/* initreg.c */
NTSTATUS SmInitializeRegistry(VOID);

/* initss.c */
NTSTATUS SmLoadSubsystems(VOID);

/* smapi.c */
#define SMAPI(n) \
NTSTATUS FASTCALL n (PSM_PORT_MESSAGE Request)
PSM_CONNECT_DATA FASTCALL SmpGetConnectData (PSM_PORT_MESSAGE);
NTSTATUS SmCreateApiPort(VOID);
VOID STDCALL SmpApiThread(PVOID);


/* smapiexec.c */
NTSTATUS STDCALL SmCreateUserProcess(LPWSTR ImagePath,
				     LPWSTR CommandLine,
				     BOOLEAN WaitForIt,
				     PLARGE_INTEGER Timeout OPTIONAL,
				     BOOLEAN TerminateIt,
				     PRTL_PROCESS_INFO ProcessInfo OPTIONAL);
NTSTATUS STDCALL
SmLookupSubsystem (IN     PWSTR   Name,
		   IN OUT PWSTR   Data,
		   IN OUT PULONG  DataLength,
		   IN OUT PULONG  DataType,
		   IN     BOOLEAN Expand);
NTSTATUS FASTCALL SmExecPgm(PSM_PORT_MESSAGE);

/* smapicomp.c */
NTSTATUS FASTCALL SmCompSes(PSM_PORT_MESSAGE);

/* smapiquery.c */
NTSTATUS FASTCALL SmQryInfo(PSM_PORT_MESSAGE);

/* client.c */
typedef struct _SM_CLIENT_DATA
{
	USHORT	SubsystemId;
	BOOL	Initialized;
	HANDLE	ServerProcess;
	HANDLE	ApiPort;
	HANDLE	ApiPortThread;
	HANDLE	SbApiPort;
	WCHAR	SbApiPortName [SM_SB_NAME_MAX_LENGTH];
	struct _SM_CLIENT_DATA * Next;

} SM_CLIENT_DATA, *PSM_CLIENT_DATA;
NTSTATUS SmInitializeClientManagement(VOID);
NTSTATUS STDCALL SmCreateClient(PSM_PORT_MESSAGE,PSM_CLIENT_DATA*);
NTSTATUS STDCALL SmDestroyClient(ULONG);
NTSTATUS STDCALL SmCompleteClientInitialization (HANDLE hProcess);
NTSTATUS FASTCALL SmGetClientBasicInformation (PSM_BASIC_INFORMATION);
NTSTATUS FASTCALL SmGetSubSystemInformation (PSM_SUBSYSTEM_INFORMATION);

/* debug.c */
extern HANDLE DbgSsApiPort;
extern HANDLE DbgUiApiPort;
NTSTATUS SmInitializeDbgSs(VOID);

/* print.c */
VOID STDCALL DisplayString(LPCWSTR lpwString);
VOID STDCALL PrintString (char* fmt, ...);

#endif /* _SMSS_H_INCLUDED_ */

/* EOF */

