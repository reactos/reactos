#ifndef _SMSS_H_INCLUDED_
#define _SMSS_H_INCLUDED_


#define CHILD_CSRSS     0
#define CHILD_WINLOGON  1


/* GLOBAL VARIABLES ****/

//extern HANDLE SmApiPort;


/* FUNCTIONS ***********/

/* init.c */

extern HANDLE SmpHeap;

NTSTATUS
InitSessionManager(HANDLE Children[]);


/* smss.c */
void DisplayString (LPCWSTR lpwString);
void PrintString (char* fmt,...);

/* smapi.c */

NTSTATUS
SmpCreateApiPort(VOID);

VOID STDCALL
SmpApiThread(HANDLE Port);

/* client.c */

VOID STDCALL
SmpInitializeClientManagement(VOID);

NTSTATUS STDCALL
SmpCreateClient(SM_PORT_MESSAGE);

NTSTATUS STDCALL
SmpDestroyClient(ULONG);

/* debug.c */

extern HANDLE DbgSsApiPort;
extern HANDLE DbgUiApiPort;

NTSTATUS STDCALL
SmpInitializeDbgSs(VOID);

#endif /* _SMSS_H_INCLUDED_ */

/* EOF */

