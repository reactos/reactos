#include <ddk/ntddk.h>

#include <csrss/csrss.h>

typedef struct
{
   BOOL TopLevel;
   HANDLE ActiveEvent;
   BYTE Screen[80*25*2];
   ULONG ReferenceCount;
   HANDLE LockMutant;
   ULONG CurrentX;
   ULONG CurrentY;
} CSRSS_CONSOLE, *PCSRSS_CONSOLE;

typedef struct
{
   PCSRSS_CONSOLE Console;
   ULONG HandleTableSize;
   PVOID* HandleTable;
   ULONG ProcessId;
} CSRSS_PROCESS_DATA, *PCSRSS_PROCESS_DATA;

NTSTATUS CsrCreateProcess (PCSRSS_PROCESS_DATA ProcessData,
			   PCSRSS_API_REQUEST LpcMessage);

NTSTATUS CsrTerminateProcess(PCSRSS_PROCESS_DATA ProcessData,
			     PCSRSS_API_REQUEST LpcMessage);

NTSTATUS CsrWriteConsole(PCSRSS_PROCESS_DATA ProcessData,
			 PCSRSS_API_REQUEST LpcMessage, 
			 PULONG CharCount);

NTSTATUS CsrAllocConsole(PCSRSS_PROCESS_DATA ProcessData,
			 PCSRSS_API_REQUEST LpcMessage, 
			 PHANDLE ReturnedHandle);

NTSTATUS CsrFreeConsole(PCSRSS_PROCESS_DATA ProcessData,
			PCSRSS_API_REQUEST LpcMessage);

NTSTATUS CsrReadConsole(PCSRSS_PROCESS_DATA ProcessData,
			PCSRSS_API_REQUEST LpcMessage, 
			PULONG CharCount);

NTSTATUS CsrConnectProcess(PCSRSS_PROCESS_DATA ProcessData,
			   PCSRSS_API_REQUEST Request);

/* print.c */
VOID DisplayString(LPCWSTR lpwString);
VOID PrintString (char* fmt, ...);

/* api/wapi.c */
VOID Thread_Api(PVOID PortHandle);

extern HANDLE CsrssApiHeap;

/* api/conio.c */
VOID CsrInitConsole(PCSRSS_PROCESS_DATA ProcessData,
		    PCSRSS_CONSOLE Console);

/* api/process.c */
PCSRSS_PROCESS_DATA CsrGetProcessData(ULONG ProcessId);
