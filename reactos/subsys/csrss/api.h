#ifndef _CSRSS_API_H
#define _CSRSS_API_H

#define NTOS_USER_MODE
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
   struct {				/* active code pages */
	   UINT Input;
	   UINT Output;
   } CodePageId;
   BOOL EarlyReturn;                   /* wake client and return data, even if we are in line buffered mode, and we don't have a complete line */
} CSRSS_CONSOLE, *PCSRSS_CONSOLE;

typedef struct
{
  PCSRSS_CONSOLE Console;
  ULONG HandleTableSize;
  Object_t ** HandleTable;
  ULONG ProcessId;
  HANDLE ConsoleEvent;
  PVOID CsrSectionViewBase;
  ULONG CsrSectionViewSize;
} CSRSS_PROCESS_DATA, *PCSRSS_PROCESS_DATA;

#define CSR_API(n) NTSTATUS n (\
PCSRSS_PROCESS_DATA ProcessData,\
PCSRSS_API_REQUEST Request,\
PCSRSS_API_REPLY Reply)

/* api/process.c */
CSR_API(CsrConnectProcess);
CSR_API(CsrCreateProcess);
CSR_API(CsrTerminateProcess);

/* api/conio.c */
CSR_API(CsrWriteConsole);
CSR_API(CsrAllocConsole);
CSR_API(CsrFreeConsole);
CSR_API(CsrReadConsole);
CSR_API(CsrConnectProcess);
CSR_API(CsrGetScreenBufferInfo);
CSR_API(CsrSetCursor);
CSR_API(CsrFillOutputChar);
CSR_API(CsrReadInputEvent);
CSR_API(CsrWriteConsoleOutputChar);
CSR_API(CsrWriteConsoleOutputAttrib);
CSR_API(CsrFillOutputAttrib);
CSR_API(CsrGetCursorInfo);
CSR_API(CsrSetCursorInfo);
CSR_API(CsrSetTextAttrib);
CSR_API(CsrSetConsoleMode);
CSR_API(CsrGetConsoleMode);
CSR_API(CsrCreateScreenBuffer);
CSR_API(CsrSetScreenBuffer);
CSR_API(CsrSetTitle);
CSR_API(CsrGetTitle);
CSR_API(CsrWriteConsoleOutput);
CSR_API(CsrFlushInputBuffer);
CSR_API(CsrScrollConsoleScreenBuffer);
CSR_API(CsrReadConsoleOutputChar);

/* print.c */
VOID STDCALL DisplayString(LPCWSTR lpwString);
VOID STDCALL PrintString (char* fmt, ...);

/* api/wapi.c */
VOID Thread_Api(PVOID PortHandle);
VOID Console_Api( DWORD Ignored );

extern HANDLE CsrssApiHeap;

/* api/conio.c */
NTSTATUS STDCALL CsrInitConsole(PCSRSS_CONSOLE Console);
VOID STDCALL CsrDeleteConsole( PCSRSS_CONSOLE Console );
VOID STDCALL CsrDeleteScreenBuffer( PCSRSS_SCREEN_BUFFER Buffer );
NTSTATUS STDCALL CsrInitConsoleScreenBuffer( PCSRSS_SCREEN_BUFFER Console );
VOID STDCALL CsrInitConsoleSupport(VOID);

/* api/process.c */
VOID STDCALL CsrInitProcessData(VOID);
PCSRSS_PROCESS_DATA STDCALL CsrGetProcessData(ULONG ProcessId);
NTSTATUS STDCALL CsrFreeProcessData( ULONG Pid );

/* api/handle.c */
NTSTATUS STDCALL CsrInsertObject( PCSRSS_PROCESS_DATA ProcessData, PHANDLE Handle, Object_t *Object );
NTSTATUS STDCALL CsrGetObject( PCSRSS_PROCESS_DATA ProcessData, HANDLE Handle, Object_t **Object );
BOOL STDCALL CsrServerInitialization (ULONG ArgumentCount, PWSTR *ArgumentArray);
NTSTATUS STDCALL CsrReleaseObject( PCSRSS_PROCESS_DATA ProcessData, HANDLE Object );
VOID STDCALL CsrDrawConsole( PCSRSS_SCREEN_BUFFER Console );
NTSTATUS STDCALL CsrpWriteConsole( PCSRSS_SCREEN_BUFFER Buff, CHAR *Buffer, DWORD Length, BOOL Attrib );

#endif /* ndef _CSRSS_API_H */
