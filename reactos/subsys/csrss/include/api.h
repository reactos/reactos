/* $Id: api.h,v 1.3 2003/12/02 11:38:46 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/include/api.h
 * PURPOSE:         CSRSS API interface
 */

#ifndef API_H_INCLUDED
#define API_H_INCLUDED

#include <ntos.h>

#include <csrss/csrss.h>

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
  BOOLEAN Fake;          // synthesized, not a real event
  BOOLEAN NotChar;       // message should not be used to return a character
} ConsoleInput;

typedef struct CSRSS_CONSOLE_t *PCSRSS_CONSOLE;

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
   PCSRSS_CONSOLE Console;          /* Console this buffer is currently attached to */
   CRITICAL_SECTION Lock;
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
   struct {				 /* active code pages */
	   UINT Input;
	   UINT Output;
   } CodePageId;
   BOOL EarlyReturn;                     /* wake client and return data, even if we are in line buffered mode, and we don't have a complete line */
   DWORD HardwareState;                  /* _GDI_MANAGED, _DIRECT */
   HWND hWindow;
   COORD Size;
   PVOID GuiConsoleData;
   LIST_ENTRY ProcessList;
} CSRSS_CONSOLE;

typedef struct _CSRSS_PROCESS_DATA
{
  PCSRSS_CONSOLE Console;
  ULONG HandleTableSize;
  Object_t ** HandleTable;
  ULONG ProcessId;
  ULONG ShutdownLevel;
  ULONG ShutdownFlags;
  HANDLE ConsoleEvent;
  PVOID CsrSectionViewBase;
  ULONG CsrSectionViewSize;
  struct _CSRSS_PROCESS_DATA * next;
  LIST_ENTRY ProcessEntry;
  PCONTROLDISPATCHER CtrlDispatcher;
} CSRSS_PROCESS_DATA, *PCSRSS_PROCESS_DATA;
  
typedef VOID (STDCALL *CSR_CLEANUP_OBJECT_PROC)(Object_t *Object);

typedef struct tagCSRSS_OBJECT_DEFINITION
{
  LONG Type;
  CSR_CLEANUP_OBJECT_PROC CsrCleanupObjectProc;
} CSRSS_OBJECT_DEFINITION, *PCSRSS_OBJECT_DEFINITION;

typedef NTSTATUS (STDCALL *CSRSS_API_PROC)(PCSRSS_PROCESS_DATA ProcessData,
                                           PCSRSS_API_REQUEST Request,
                                           PCSRSS_API_REPLY Reply);

typedef struct _CSRSS_API_DEFINITION
{
  ULONG Type;
  ULONG MinRequestSize;
  ULONG MinReplySize;
  CSRSS_API_PROC Handler;
} CSRSS_API_DEFINITION, *PCSRSS_API_DEFINITION;

#define CSRSS_DEFINE_API(Func, Handler) \
  { Func, sizeof(Func##_REQUEST), sizeof(Func##_REPLY), Handler }

typedef struct _CSRSS_LISTEN_DATA
{
  HANDLE ApiPortHandle;
  ULONG ApiDefinitionsCount;
  PCSRSS_API_DEFINITION *ApiDefinitions;
} CSRSS_LISTEN_DATA, *PCSRSS_LISTEN_DATA;

#define CSR_API(n) NTSTATUS STDCALL n (\
PCSRSS_PROCESS_DATA ProcessData,\
PCSRSS_API_REQUEST Request,\
PCSRSS_API_REPLY Reply)

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
                                PCSRSS_API_REQUEST Request,
                                PCSRSS_API_REPLY Reply);
VOID Thread_Api(PVOID PortHandle);
VOID Console_Api( DWORD Ignored );

extern HANDLE CsrssApiHeap;

/* api/conio.c */
NTSTATUS STDCALL CsrInitConsole(PCSRSS_CONSOLE Console);
NTSTATUS STDCALL CsrInitConsoleScreenBuffer(PCSRSS_SCREEN_BUFFER Buffer,
                                            PCSRSS_CONSOLE Console,
                                            unsigned Width,
                                            unsigned Height);
VOID STDCALL CsrInitConsoleSupport(VOID);

/* api/process.c */
VOID STDCALL CsrInitProcessData(VOID);
PCSRSS_PROCESS_DATA STDCALL CsrGetProcessData(ULONG ProcessId);
NTSTATUS STDCALL CsrFreeProcessData( ULONG Pid );

/* api/handle.c */
NTSTATUS FASTCALL CsrRegisterObjectDefinitions(PCSRSS_OBJECT_DEFINITION NewDefinitions);
NTSTATUS STDCALL CsrInsertObject( PCSRSS_PROCESS_DATA ProcessData, PHANDLE Handle, Object_t *Object );
NTSTATUS STDCALL CsrGetObject( PCSRSS_PROCESS_DATA ProcessData, HANDLE Handle, Object_t **Object );
BOOL STDCALL CsrServerInitialization (ULONG ArgumentCount, PWSTR *ArgumentArray);
NTSTATUS STDCALL CsrReleaseObjectByPointer(Object_t *Object);
NTSTATUS STDCALL CsrReleaseObject( PCSRSS_PROCESS_DATA ProcessData, HANDLE Object );
NTSTATUS STDCALL CsrVerifyObject( PCSRSS_PROCESS_DATA ProcessData, HANDLE Object );
VOID STDCALL CsrDrawConsole(PCSRSS_CONSOLE Console);
NTSTATUS CsrpEchoUnicodeChar( PCSRSS_SCREEN_BUFFER Console, 
			      WCHAR UnicodeChar );
NTSTATUS STDCALL CsrpWriteConsole( PCSRSS_SCREEN_BUFFER Buff, CHAR *Buffer, DWORD Length, BOOL Attrib );
CSR_API(CsrGetInputHandle);
CSR_API(CsrGetOutputHandle);
CSR_API(CsrCloseHandle);
CSR_API(CsrVerifyHandle);
CSR_API(CsrDuplicateHandle);

/* api/user.c */
CSR_API(CsrRegisterServicesProcess);
CSR_API(CsrExitReactos);
CSR_API(CsrGetShutdownParameters);
CSR_API(CsrSetShutdownParameters);

#endif /* ndef API_H_INCLUDED */

/* EOF */

