#ifndef _SMSS_H_INCLUDED_
#define _SMSS_H_INCLUDED_

#define NTOS_MODE_USER
#include <ntos.h>
#include <sm/api.h>

#define CHILD_CSRSS     0
#define CHILD_WINLOGON  1

/* init.c */
extern HANDLE SmpHeap;
NTSTATUS InitSessionManager(HANDLE Children[]);

/* initheap.c */
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
NTSTATUS SmRunCsrss(VOID);
NTSTATUS SmRunWinlogon(VOID);

/* smapi.c */
NTSTATUS SmCreateApiPort(VOID);
VOID STDCALL SmpApiThread(HANDLE Port);

/* client.c */
NTSTATUS SmInitializeClientManagement(VOID);
NTSTATUS STDCALL SmpCreateClient(SM_PORT_MESSAGE);
NTSTATUS STDCALL SmpDestroyClient(ULONG);

/* debug.c */
extern HANDLE DbgSsApiPort;
extern HANDLE DbgUiApiPort;
NTSTATUS SmInitializeDbgSs(VOID);

#endif /* _SMSS_H_INCLUDED_ */

/* EOF */

