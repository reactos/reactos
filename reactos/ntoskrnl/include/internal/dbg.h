#ifndef __INCLUDE_INTERNAL_DBG_H
#define __INCLUDE_INTERNAL_DBG_H

#include <internal/port.h>

#define DBG_EVENT_EXCEPTION         (1)
#define DBG_EVENT_CREATE_THREAD     (2)
#define DBG_EVENT_CREATE_PROCESS    (3)
#define DBG_EVENT_EXIT_THREAD       (4)
#define DBG_EVENT_EXIT_PROCESS      (5)
#define DBG_EVENT_LOAD_DLL          (6)
#define DBG_EVENT_UNLOAD_DLL        (7)
#define DBG_STATUS_CONTROL_C       1
#define DBG_STATUS_SYSRQ           2
#define DBG_STATUS_BUGCHECK_FIRST  3
#define DBG_STATUS_BUGCHECK_SECOND 4
#define DBG_STATUS_FATAL           5


#define DBG_GET_SHOW_FACILITY 0x0001
#define DBG_GET_SHOW_SEVERITY 0x0002
#define DBG_GET_SHOW_ERRCODE  0x0004
#define DBG_GET_SHOW_ERRTEXT  0x0008

typedef struct _LPC_DBG_MESSAGE
{
   LPC_MESSAGE Header;
   ULONG Type;
   ULONG Status;
   union
     {
	struct
	  {
	     EXCEPTION_RECORD ExceptionRecord;
	     ULONG FirstChance;
	  } Exception;
	struct
	  {
	     ULONG Reserved;
	     PVOID StartAddress;
	  } CreateThread;
	struct
	  {
	     ULONG Reserved;
	     HANDLE FileHandle;
	     PVOID Base;
	     ULONG PointerToSymbolTable;
	     ULONG NumberOfSymbols;
	     ULONG Reserved2;
	     PVOID EntryPoint;
	  } CreateProcess;
	struct
	  {
	     ULONG ExitCode;
	  } ExitThread;
	struct
	  {
	     ULONG ExitCode;
	  } ExitProcess;
	struct
	  {
	     HANDLE FileHandle;
	     PVOID Base;
	     ULONG PointerToSymbolTable;
	     ULONG NumberOfSymbols;
	  } LoadDll;
	struct
	  {
	     PVOID Base;
	  } UnloadDll;
     } Data;
} LPC_DBG_MESSAGE, *PLPC_DBG_MESSAGE;


typedef struct _LPC_TERMINATION_MESSAGE
{
   LPC_MESSAGE Header;
   LARGE_INTEGER CreationTime;
} LPC_TERMINATION_MESSAGE, *PLPC_TERMINATION_MESSAGE;

NTSTATUS STDCALL 
LpcSendDebugMessagePort(PEPORT Port,
			PLPC_DBG_MESSAGE Message,
			PLPC_DBG_MESSAGE Reply);
VOID
DbgkCreateThread(PVOID StartAddress);
ULONG
DbgkForwardException(EXCEPTION_RECORD Er, ULONG FirstChance);
BOOLEAN
DbgShouldPrint(PCH Filename);
VOID DbgGetErrorText(NTSTATUS ErrorCode, PUNICODE_STRING ErrorText, ULONG Flags);
VOID DbgPrintErrorMessage(NTSTATUS ErrorCode);

#endif /* __INCLUDE_INTERNAL_DBG_H */
