#include <ntos.h>

#include <csrss/csrss.h>

/* Object type magic numbers */

#define CSRSS_CONSOLE_MAGIC         1
#define CSRSS_SCREEN_BUFFER_MAGIC   2

typedef struct Object_tt
{
   LONG Type;
   LONG ReferenceCount;
} Object_t;

typedef struct ConsoleInput_t
{
  LIST_ENTRY ListEntry;
  INPUT_RECORD InputEvent;
  BOOLEAN Echoed;        // already been echoed or not
} ConsoleInput;


/************************************************************************
 * Screen buffer structure represents the win32 screen buffer object.   *
 * Internally, the portion of the buffer being shown CAN loop past the  *
 * bottom of the virtual buffer and wrap around to the top.  Win32 does *
 * not do this.  I decided to do this because it eliminates the need to *
 * do a massive memcpy() to scroll the contents of the buffer up to     *
 * scroll the screen on output, instead I just shift down the position  *
 * to be displayed, and let it wrap around to the top again.            *
 * The VirtualX member keeps track of the top X coord that win32        *
 * clients THINK is currently being displayed, because they think that  *
 * when the display reaches the bottom of the buffer and another line   *
 * being printed causes another line to scroll down, that the buffer IS *
 * memcpy()'s up, and the bottom of the buffer is still displayed, but  *
 * internally, I just wrap back to the top of the buffer.               *
 ***********************************************************************/

typedef struct CSRSS_SCREEN_BUFFER_t
{
   Object_t Header;                 /* Object header */
   BYTE *Buffer;                    /* pointer to screen buffer */
   USHORT MaxX, MaxY;               /* size of the entire scrollback buffer */
   USHORT ShowX, ShowY;             /* beginning offset for the actual display area */
   ULONG CurrentX;                  /* Current X cursor position */
   ULONG CurrentY;                  /* Current Y cursor position */
   BYTE DefaultAttrib;              /* default char attribute */
   USHORT VirtualX;                 /* top row of buffer being displayed, reported to callers */
   CONSOLE_CURSOR_INFO CursorInfo;
   USHORT Mode;
} CSRSS_SCREEN_BUFFER, *PCSRSS_SCREEN_BUFFER;

typedef struct CSRSS_CONSOLE_t
{
   Object_t Header;                      /* Object header */
   struct CSRSS_CONSOLE_t *Prev, *Next;  /* Next and Prev consoles in console wheel */
   HANDLE ActiveEvent;
   LIST_ENTRY InputEvents;               /* List head for input event queue */
   WORD WaitingChars;
   WORD WaitingLines;                    /* number of chars and lines in input queue */
   PCSRSS_SCREEN_BUFFER ActiveBuffer;    /* Pointer to currently active screen buffer */
   WORD Mode;                            /* Console mode flags */
   WORD EchoCount;                       /* count of chars to echo, in line buffered mode */
   UNICODE_STRING Title;                 /* Title of console */
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

NTSTATUS CsrSetConsoleMode( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply );

NTSTATUS CsrGetConsoleMode( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply );

NTSTATUS CsrCreateScreenBuffer( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply );

NTSTATUS CsrSetScreenBuffer( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply );

NTSTATUS CsrSetTitle( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply );

/* print.c */
VOID DisplayString(LPCWSTR lpwString);
VOID PrintString (char* fmt, ...);

/* api/wapi.c */
VOID Thread_Api(PVOID PortHandle);
VOID Console_Api( DWORD Ignored );

extern HANDLE CsrssApiHeap;

/* api/conio.c */
NTSTATUS CsrInitConsole(PCSRSS_CONSOLE Console);
VOID CsrDeleteConsole( PCSRSS_CONSOLE Console );
VOID CsrDeleteScreenBuffer( PCSRSS_SCREEN_BUFFER Buffer );
NTSTATUS CsrInitConsoleScreenBuffer( PCSRSS_SCREEN_BUFFER Console );
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
VOID CsrDrawConsole( PCSRSS_SCREEN_BUFFER Console );
NTSTATUS CsrpWriteConsole( PCSRSS_SCREEN_BUFFER Buff, CHAR *Buffer, DWORD Length, BOOL Attrib );
