#include <ntos.h>

#include <csrss/csrss.h>

/* Object type magic numbers */

#define CSRSS_CONSOLE_MAGIC 1

typedef struct Object_tt
{
   DWORD Type;
   DWORD ReferenceCount;
} Object_t;

typedef struct ConsoleInput_t
{
  LIST_ENTRY ListEntry;
  INPUT_RECORD InputEvent;
} ConsoleInput;

typedef struct CSRSS_CONSOLE_t
{
   DWORD Type;
   DWORD ReferenceCount;              /* Inherited from Object_t */
   struct CSRSS_CONSOLE_t *Prev, *Next; /* Next and Prev consoles in console wheel */
   HANDLE ActiveEvent;
   BYTE *Buffer;
   USHORT MaxX, MaxY;          /* size of the entire scrollback buffer */
   USHORT ShowX, ShowY;        /* beginning offset for the actual display area */
   ULONG CurrentX;
   ULONG CurrentY;
   BYTE DefaultAttrib;        /* default char attribute */
   LIST_ENTRY InputEvents;    /* List head for input event queue */
   CONSOLE_CURSOR_INFO CursorInfo;
} CSRSS_CONSOLE, *PCSRSS_CONSOLE;

typedef struct
{
   PCSRSS_CONSOLE Console;
   ULONG HandleTableSize;
   Object_t ** HandleTable;
   ULONG ProcessId;
   HANDLE ConsoleEvent;
} CSRSS_PROCESS_DATA, *PCSRSS_PROCESS_DATA;


VOID CsrInitProcessData(VOID);

NTSTATUS CsrCreateProcess (PCSRSS_PROCESS_DATA ProcessData,
			   PCSRSS_API_REQUEST Request,
			   PCSRSS_API_REPLY Reply);

NTSTATUS CsrTerminateProcess(PCSRSS_PROCESS_DATA ProcessData,
			     PCSRSS_API_REQUEST LpcMessage,
			     PCSRSS_API_REPLY Reply);

NTSTATUS CsrWriteConsole(PCSRSS_PROCESS_DATA ProcessData,
			 PCSRSS_API_REQUEST LpcMessage,
			 PCSRSS_API_REPLY Reply);

NTSTATUS CsrAllocConsole(PCSRSS_PROCESS_DATA ProcessData,
			 PCSRSS_API_REQUEST LpcMessage,
			 PCSRSS_API_REPLY Reply);

NTSTATUS CsrFreeConsole(PCSRSS_PROCESS_DATA ProcessData,
			PCSRSS_API_REQUEST LpcMessage,
			PCSRSS_API_REPLY Reply);

NTSTATUS CsrReadConsole(PCSRSS_PROCESS_DATA ProcessData,
			PCSRSS_API_REQUEST LpcMessage,
			PCSRSS_API_REPLY Reply);

NTSTATUS CsrConnectProcess(PCSRSS_PROCESS_DATA ProcessData,
			   PCSRSS_API_REQUEST Request,
			   PCSRSS_API_REPLY Reply);

NTSTATUS CsrGetScreenBufferInfo( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply );

NTSTATUS CsrSetCursor( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply );

NTSTATUS CsrFillOutputChar( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply );

NTSTATUS CsrReadInputEvent( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply );

NTSTATUS CsrWriteConsoleOutputChar( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply );

NTSTATUS CsrWriteConsoleOutputAttrib( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply );

NTSTATUS CsrFillOutputAttrib( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply );

NTSTATUS CsrGetCursorInfo( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply );

NTSTATUS CsrSetCursorInfo( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply );

NTSTATUS CsrSetTextAttrib( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply );

/* print.c */
VOID DisplayString(LPCWSTR lpwString);
VOID PrintString (char* fmt, ...);

/* api/wapi.c */
VOID Thread_Api(PVOID PortHandle);
VOID Console_Api( DWORD Ignored );

extern HANDLE CsrssApiHeap;

/* api/conio.c */
NTSTATUS CsrInitConsole(PCSRSS_PROCESS_DATA ProcessData,
		    PCSRSS_CONSOLE Console);
VOID CsrDeleteConsole( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_CONSOLE Console );
VOID CsrInitConsoleSupport(VOID);

/* api/process.c */
PCSRSS_PROCESS_DATA CsrGetProcessData(ULONG ProcessId);
NTSTATUS CsrFreeProcessData( ULONG Pid );
/* api/handle.c */
NTSTATUS CsrInsertObject( PCSRSS_PROCESS_DATA ProcessData, PHANDLE Handle, Object_t *Object );
NTSTATUS CsrGetObject( PCSRSS_PROCESS_DATA ProcessData, HANDLE Handle, Object_t **Object );

BOOL STDCALL CsrServerInitialization (ULONG ArgumentCount,
				      PWSTR *ArgumentArray);
NTSTATUS CsrReleaseObject( PCSRSS_PROCESS_DATA ProcessData, HANDLE Object );
VOID CsrDrawConsole( PCSRSS_CONSOLE Console );




