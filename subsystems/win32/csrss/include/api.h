/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/include/api.h
 * PURPOSE:         CSRSS API interface
 */

#ifndef API_H_INCLUDED
#define API_H_INCLUDED

#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include <csrss/csrss.h>

typedef struct Object_tt
{
  LONG Type;
  LONG ReferenceCount;
  CRITICAL_SECTION Lock;
} Object_t;

typedef struct ConsoleInput_t
{
  LIST_ENTRY ListEntry;
  INPUT_RECORD InputEvent;
  BOOLEAN Echoed;        // already been echoed or not
  BOOLEAN Fake;          // synthesized, not a real event
  BOOLEAN NotChar;       // message should not be used to return a character
} ConsoleInput;

typedef struct tagCSRSS_CONSOLE *PCSRSS_CONSOLE;

typedef struct _CSRSS_HANDLE
{
  Object_t *Object;
  DWORD Access;
  BOOL Inheritable;
} CSRSS_HANDLE, *PCSRSS_HANDLE;

typedef struct _CSRSS_PROCESS_DATA
{
  PCSRSS_CONSOLE Console;
  PCSRSS_CONSOLE ParentConsole;
  BOOL bInheritHandles;
  RTL_CRITICAL_SECTION HandleTableLock;
  ULONG HandleTableSize;
  PCSRSS_HANDLE HandleTable;
  HANDLE ProcessId;
  DWORD ProcessGroup;
  HANDLE Process;
  ULONG ShutdownLevel;
  ULONG ShutdownFlags;
  HANDLE ConsoleEvent;
  PVOID CsrSectionViewBase;
  ULONG CsrSectionViewSize;
  HANDLE ServerCommunicationPort;
  struct _CSRSS_PROCESS_DATA * next;
  LIST_ENTRY ProcessEntry;
  PCONTROLDISPATCHER CtrlDispatcher;
  BOOL Terminated;
} CSRSS_PROCESS_DATA, *PCSRSS_PROCESS_DATA;

typedef VOID (STDCALL *CSR_CLEANUP_OBJECT_PROC)(Object_t *Object);

typedef struct tagCSRSS_OBJECT_DEFINITION
{
  LONG Type;
  CSR_CLEANUP_OBJECT_PROC CsrCleanupObjectProc;
} CSRSS_OBJECT_DEFINITION, *PCSRSS_OBJECT_DEFINITION;

typedef NTSTATUS (STDCALL *CSRSS_API_PROC)(PCSRSS_PROCESS_DATA ProcessData,
                                           PCSR_API_MESSAGE Request);

typedef struct _CSRSS_API_DEFINITION
{
  ULONG Type;
  ULONG MinRequestSize;
  CSRSS_API_PROC Handler;
} CSRSS_API_DEFINITION, *PCSRSS_API_DEFINITION;

#define CSRSS_DEFINE_API(Func, Handler) \
  { Func, sizeof(CSRSS_##Func), Handler }

typedef struct _CSRSS_LISTEN_DATA
{
  HANDLE ApiPortHandle;
  ULONG ApiDefinitionsCount;
  PCSRSS_API_DEFINITION *ApiDefinitions;
} CSRSS_LISTEN_DATA, *PCSRSS_LISTEN_DATA;

#define CSR_API(n) NTSTATUS STDCALL n (\
PCSRSS_PROCESS_DATA ProcessData,\
PCSR_API_MESSAGE Request)

/* init.c */
extern HANDLE hBootstrapOk;

/* api/process.c */
CSR_API(CsrConnectProcess);
CSR_API(CsrCreateProcess);
CSR_API(CsrTerminateProcess);

/* print.c */
VOID STDCALL DisplayString(LPCWSTR lpwString);
VOID STDCALL PrintString (char* fmt, ...);

/* api/wapi.c */
NTSTATUS FASTCALL CsrApiRegisterDefinitions(PCSRSS_API_DEFINITION NewDefinitions);
VOID FASTCALL CsrApiCallHandler(PCSRSS_PROCESS_DATA ProcessData,
                                PCSR_API_MESSAGE Request);
DWORD STDCALL ServerApiPortThread (PVOID PortHandle);
DWORD STDCALL ServerSbApiPortThread (PVOID PortHandle);
DWORD STDCALL Console_Api( PVOID unused );
VOID
NTAPI
ClientConnectionThread(HANDLE ServerPort);

extern HANDLE CsrssApiHeap;

/* api/conio.c */
VOID STDCALL CsrInitConsoleSupport(VOID);

/* api/process.c */
typedef NTSTATUS (STDCALL *CSRSS_ENUM_PROCESS_PROC)(PCSRSS_PROCESS_DATA ProcessData,
                                                    PVOID Context);
VOID STDCALL CsrInitProcessData(VOID);
PCSRSS_PROCESS_DATA STDCALL CsrGetProcessData(HANDLE ProcessId);
PCSRSS_PROCESS_DATA STDCALL CsrCreateProcessData(HANDLE ProcessId);
NTSTATUS STDCALL CsrFreeProcessData( HANDLE Pid );
NTSTATUS STDCALL CsrEnumProcesses(CSRSS_ENUM_PROCESS_PROC EnumProc, PVOID Context);

/* api/handle.c */
NTSTATUS FASTCALL CsrRegisterObjectDefinitions(PCSRSS_OBJECT_DEFINITION NewDefinitions);
NTSTATUS STDCALL CsrInsertObject( PCSRSS_PROCESS_DATA ProcessData, PHANDLE Handle, Object_t *Object, DWORD Access, BOOL Inheritable );
NTSTATUS STDCALL CsrDuplicateHandleTable(PCSRSS_PROCESS_DATA SourceProcessData, PCSRSS_PROCESS_DATA TargetProcessData);
NTSTATUS STDCALL CsrGetObject( PCSRSS_PROCESS_DATA ProcessData, HANDLE Handle, Object_t **Object, DWORD Access );
BOOL STDCALL CsrServerInitialization (int,char**,char**);
NTSTATUS STDCALL CsrReleaseObjectByPointer(Object_t *Object);
NTSTATUS STDCALL CsrReleaseObject( PCSRSS_PROCESS_DATA ProcessData, HANDLE Object );
NTSTATUS STDCALL CsrVerifyObject( PCSRSS_PROCESS_DATA ProcessData, HANDLE Object );

CSR_API(CsrGetInputHandle);
CSR_API(CsrGetOutputHandle);
CSR_API(CsrCloseHandle);
CSR_API(CsrVerifyHandle);
CSR_API(CsrDuplicateHandle);
CSR_API(CsrGetInputWaitHandle);

/* api/user.c */
CSR_API(CsrRegisterServicesProcess);
CSR_API(CsrExitReactos);
CSR_API(CsrGetShutdownParameters);
CSR_API(CsrSetShutdownParameters);

CSR_API(CsrSetLogonNotifyWindow);
CSR_API(CsrRegisterLogonProcess);


#endif /* ndef API_H_INCLUDED */

/* EOF */

