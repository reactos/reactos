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

VOID CsrInitProcessData(VOID);

NTSTATUS CsrCreateProcess (PCSRSS_PROCESS_DATA ProcessData,
			   PCSRSS_CREATE_PROCESS_REQUEST Request,
			   PLPCMESSAGE* Reply);

NTSTATUS CsrTerminateProcess(PCSRSS_PROCESS_DATA ProcessData,
			     PCSRSS_API_REQUEST LpcMessage,
			     PLPCMESSAGE* Reply);

NTSTATUS CsrWriteConsole(PCSRSS_PROCESS_DATA ProcessData,
			 PCSRSS_API_REQUEST LpcMessage,
			 PLPCMESSAGE* Reply);

NTSTATUS CsrAllocConsole(PCSRSS_PROCESS_DATA ProcessData,
			 PCSRSS_API_REQUEST LpcMessage,
			 PLPCMESSAGE* Reply);

NTSTATUS CsrFreeConsole(PCSRSS_PROCESS_DATA ProcessData,
			PCSRSS_API_REQUEST LpcMessage,
			PLPCMESSAGE* Reply);

NTSTATUS CsrReadConsole(PCSRSS_PROCESS_DATA ProcessData,
			PCSRSS_API_REQUEST LpcMessage,
			PLPCMESSAGE* Reply);

NTSTATUS CsrConnectProcess(PCSRSS_PROCESS_DATA ProcessData,
			   PCSRSS_API_REQUEST Request,
			   PLPCMESSAGE* Reply);

/* print.c */
VOID DisplayString(LPCWSTR lpwString);
VOID PrintString (char* fmt, ...);

/* api/wapi.c */
VOID Thread_Api(PVOID PortHandle);

extern HANDLE CsrssApiHeap;

/* api/conio.c */
VOID CsrInitConsole(PCSRSS_PROCESS_DATA ProcessData,
		    PCSRSS_CONSOLE Console);
VOID CsrInitConsoleSupport(VOID);

/* api/process.c */
PCSRSS_PROCESS_DATA CsrGetProcessData(ULONG ProcessId);

/* api/handle.c */
NTSTATUS CsrInsertObject(PCSRSS_PROCESS_DATA ProcessData,
			 PHANDLE Handle,
			 PVOID Object);
NTSTATUS CsrGetObject(PCSRSS_PROCESS_DATA ProcessData,
		      HANDLE Handle,
		      PVOID* Object);

BOOL STDCALL CsrServerInitialization (ULONG ArgumentCount,
				      PWSTR *ArgumentArray);
