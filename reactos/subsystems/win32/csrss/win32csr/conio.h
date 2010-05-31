/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/include/conio.h
 * PURPOSE:         CSRSS internal console I/O interface
 */

#pragma once

#include "api.h"
#include "win32csr.h"

#define CSR_DEFAULT_CURSOR_SIZE 25

/* Object type magic numbers */

#define CONIO_CONSOLE_MAGIC         0x00000001
#define CONIO_SCREEN_BUFFER_MAGIC   0x00000002

/************************************************************************
 * Screen buffer structure represents the win32 screen buffer object.   *
 * Internally, the portion of the buffer being shown CAN loop past the  *
 * bottom of the virtual buffer and wrap around to the top.  Win32 does *
 * not do this.  I decided to do this because it eliminates the need to *
 * do a massive memcpy() to scroll the contents of the buffer up to     *
 * scroll the screen on output, instead I just shift down the position  *
 * to be displayed, and let it wrap around to the top again.            *
 * The VirtualY member keeps track of the top Y coord that win32        *
 * clients THINK is currently being displayed, because they think that  *
 * when the display reaches the bottom of the buffer and another line   *
 * being printed causes another line to scroll down, that the buffer IS *
 * memcpy()'s up, and the bottom of the buffer is still displayed, but  *
 * internally, I just wrap back to the top of the buffer.               *
 ***********************************************************************/

typedef struct tagCSRSS_SCREEN_BUFFER
{
  Object_t Header;                 /* Object header */
  BYTE *Buffer;                    /* pointer to screen buffer */
  USHORT MaxX, MaxY;               /* size of the entire scrollback buffer */
  USHORT ShowX, ShowY;             /* beginning offset for the actual display area */
  ULONG CurrentX;                  /* Current X cursor position */
  ULONG CurrentY;                  /* Current Y cursor position */
  WORD DefaultAttrib;              /* default char attribute */
  USHORT VirtualY;                 /* top row of buffer being displayed, reported to callers */
  CONSOLE_CURSOR_INFO CursorInfo;
  USHORT Mode;
  LIST_ENTRY ListEntry;            /* entry in console's list of buffers */
} CSRSS_SCREEN_BUFFER, *PCSRSS_SCREEN_BUFFER;

typedef struct tagCSRSS_CONSOLE *PCSRSS_CONSOLE;

typedef struct tagCSRSS_CONSOLE_VTBL
{
  VOID (WINAPI *InitScreenBuffer)(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER ScreenBuffer);
  VOID (WINAPI *WriteStream)(PCSRSS_CONSOLE Console, SMALL_RECT *Block, LONG CursorStartX, LONG CursorStartY,
                              UINT ScrolledLines, CHAR *Buffer, UINT Length);
  VOID (WINAPI *DrawRegion)(PCSRSS_CONSOLE Console, SMALL_RECT *Region);
  BOOL (WINAPI *SetCursorInfo)(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER ScreenBuffer);
  BOOL (WINAPI *SetScreenInfo)(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER ScreenBuffer,
                                UINT OldCursorX, UINT OldCursorY);
  BOOL (WINAPI *UpdateScreenInfo)(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER ScreenBuffer);
  BOOL (WINAPI *ChangeTitle)(PCSRSS_CONSOLE Console);
  VOID (WINAPI *CleanupConsole)(PCSRSS_CONSOLE Console);
  BOOL (WINAPI *ChangeIcon)(PCSRSS_CONSOLE Console, HICON hWindowIcon);
  NTSTATUS (WINAPI *ResizeBuffer)(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER ScreenBuffer, COORD Size);
} CSRSS_CONSOLE_VTBL, *PCSRSS_CONSOLE_VTBL;

typedef struct tagCSRSS_CONSOLE
{
  Object_t Header;                      /* Object header */
  LONG ReferenceCount;
  CRITICAL_SECTION Lock;
  PCSRSS_CONSOLE Prev, Next;            /* Next and Prev consoles in console wheel */
  HANDLE ActiveEvent;
  LIST_ENTRY InputEvents;               /* List head for input event queue */
  WORD WaitingChars;
  WORD WaitingLines;                    /* number of chars and lines in input queue */
  LIST_ENTRY BufferList;                /* List of all screen buffers for this console */
  PCSRSS_SCREEN_BUFFER ActiveBuffer;    /* Pointer to currently active screen buffer */
  WORD Mode;                            /* Console mode flags */
  WORD EchoCount;                       /* count of chars to echo, in line buffered mode */
  UNICODE_STRING Title;                 /* Title of console */
  BOOL EarlyReturn;                     /* wake client and return data, even if we are in line buffered mode, and we don't have a complete line */
  DWORD HardwareState;                  /* _GDI_MANAGED, _DIRECT */
  HWND hWindow;
  COORD Size;
  PVOID PrivateData;
  UINT CodePage;
  UINT OutputCodePage;
  PCSRSS_CONSOLE_VTBL Vtbl;
  LIST_ENTRY ProcessList;
  struct tagALIAS_HEADER *Aliases;
  CONSOLE_SELECTION_INFO Selection;
  BYTE PauseFlags;
  HANDLE UnpauseEvent;
} CSRSS_CONSOLE;

typedef struct ConsoleInput_t
{
  LIST_ENTRY ListEntry;
  INPUT_RECORD InputEvent;
  BOOLEAN Echoed;        // already been echoed or not
  BOOLEAN Fake;          // synthesized, not a real event
  BOOLEAN NotChar;       // message should not be used to return a character
} ConsoleInput;

/* CONSOLE_SELECTION_INFO dwFlags values */
#define CONSOLE_NO_SELECTION          0x0
#define CONSOLE_SELECTION_IN_PROGRESS 0x1
#define CONSOLE_SELECTION_NOT_EMPTY   0x2
#define CONSOLE_MOUSE_SELECTION       0x4
#define CONSOLE_MOUSE_DOWN            0x8

/* PauseFlags values (internal only) */
#define PAUSED_FROM_KEYBOARD  0x1
#define PAUSED_FROM_SCROLLBAR 0x2
#define PAUSED_FROM_SELECTION 0x4

#define ConioInitScreenBuffer(Console, Buff) (Console)->Vtbl->InitScreenBuffer((Console), (Buff))
#define ConioDrawRegion(Console, Region) (Console)->Vtbl->DrawRegion((Console), (Region))
#define ConioWriteStream(Console, Block, CurStartX, CurStartY, ScrolledLines, Buffer, Length) \
          (Console)->Vtbl->WriteStream((Console), (Block), (CurStartX), (CurStartY), \
                                       (ScrolledLines), (Buffer), (Length))
#define ConioSetCursorInfo(Console, Buff) (Console)->Vtbl->SetCursorInfo((Console), (Buff))
#define ConioSetScreenInfo(Console, Buff, OldCursorX, OldCursorY) \
          (Console)->Vtbl->SetScreenInfo((Console), (Buff), (OldCursorX), (OldCursorY))
#define ConioUpdateScreenInfo(Console, Buff) \
          (Console)->Vtbl->UpdateScreenInfo(Console, Buff)
#define ConioChangeTitle(Console) (Console)->Vtbl->ChangeTitle(Console)
#define ConioCleanupConsole(Console) (Console)->Vtbl->CleanupConsole(Console)
#define ConioChangeIcon(Console, hWindowIcon) (Console)->Vtbl->ChangeIcon(Console, hWindowIcon)
#define ConioResizeBuffer(Console, Buff, Size) (Console)->Vtbl->ResizeBuffer(Console, Buff, Size)

/* console.c */
NTSTATUS FASTCALL ConioConsoleFromProcessData(PCSRSS_PROCESS_DATA ProcessData, PCSRSS_CONSOLE *Console);
VOID WINAPI ConioDeleteConsole(Object_t *Object);
VOID WINAPI CsrInitConsoleSupport(VOID);
VOID FASTCALL ConioPause(PCSRSS_CONSOLE Console, UINT Flags);
VOID FASTCALL ConioUnpause(PCSRSS_CONSOLE Console, UINT Flags);
VOID FASTCALL ConioConsoleCtrlEvent(DWORD Event, PCSRSS_PROCESS_DATA ProcessData);
VOID FASTCALL ConioConsoleCtrlEventTimeout(DWORD Event, PCSRSS_PROCESS_DATA ProcessData,
                                           DWORD Timeout);
CSR_API(CsrAllocConsole);
CSR_API(CsrFreeConsole);
CSR_API(CsrSetConsoleMode);
CSR_API(CsrGetConsoleMode);
CSR_API(CsrSetTitle);
CSR_API(CsrGetTitle);
CSR_API(CsrHardwareStateProperty);
CSR_API(CsrGetConsoleWindow);
CSR_API(CsrSetConsoleIcon);
CSR_API(CsrGetConsoleCodePage);
CSR_API(CsrSetConsoleCodePage);
CSR_API(CsrGetConsoleOutputCodePage);
CSR_API(CsrSetConsoleOutputCodePage);
CSR_API(CsrGetProcessList);
CSR_API(CsrGenerateCtrlEvent);
CSR_API(CsrGetConsoleSelectionInfo);

/* coninput.c */
#define ConioLockConsole(ProcessData, Handle, Ptr, Access) \
    Win32CsrLockObject((ProcessData), (Handle), (Object_t **)(Ptr), Access, CONIO_CONSOLE_MAGIC)
#define ConioUnlockConsole(Console) \
    Win32CsrUnlockObject((Object_t *) Console)
void WINAPI ConioProcessKey(MSG *msg, PCSRSS_CONSOLE Console, BOOL TextMode);
CSR_API(CsrReadConsole);
CSR_API(CsrReadInputEvent);
CSR_API(CsrFlushInputBuffer);
CSR_API(CsrGetNumberOfConsoleInputEvents);
CSR_API(CsrPeekConsoleInput);
CSR_API(CsrWriteConsoleInput);

/* conoutput.c */
#define ConioRectHeight(Rect) \
    (((Rect)->Top) > ((Rect)->Bottom) ? 0 : ((Rect)->Bottom) - ((Rect)->Top) + 1)
#define ConioRectWidth(Rect) \
    (((Rect)->Left) > ((Rect)->Right) ? 0 : ((Rect)->Right) - ((Rect)->Left) + 1)
#define ConioLockScreenBuffer(ProcessData, Handle, Ptr, Access) \
    Win32CsrLockObject((ProcessData), (Handle), (Object_t **)(Ptr), Access, CONIO_SCREEN_BUFFER_MAGIC)
#define ConioUnlockScreenBuffer(Buff) \
    Win32CsrUnlockObject((Object_t *) Buff)
PBYTE FASTCALL ConioCoordToPointer(PCSRSS_SCREEN_BUFFER Buf, ULONG X, ULONG Y);
VOID FASTCALL ConioDrawConsole(PCSRSS_CONSOLE Console);
NTSTATUS FASTCALL ConioWriteConsole(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buff,
                                    CHAR *Buffer, DWORD Length, BOOL Attrib);
NTSTATUS FASTCALL CsrInitConsoleScreenBuffer(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buffer);
VOID WINAPI ConioDeleteScreenBuffer(PCSRSS_SCREEN_BUFFER Buffer);

CSR_API(CsrWriteConsole);
CSR_API(CsrGetScreenBufferInfo);
CSR_API(CsrSetCursor);
CSR_API(CsrWriteConsoleOutputChar);
CSR_API(CsrFillOutputChar);
CSR_API(CsrWriteConsoleOutputAttrib);
CSR_API(CsrFillOutputAttrib);
CSR_API(CsrGetCursorInfo);
CSR_API(CsrSetCursorInfo);
CSR_API(CsrSetTextAttrib);
CSR_API(CsrCreateScreenBuffer);
CSR_API(CsrSetScreenBuffer);
CSR_API(CsrWriteConsoleOutput);
CSR_API(CsrScrollConsoleScreenBuffer);
CSR_API(CsrReadConsoleOutputChar);
CSR_API(CsrReadConsoleOutputAttrib);
CSR_API(CsrReadConsoleOutput);
CSR_API(CsrSetScreenBufferSize);

/* alias.c */
VOID IntDeleteAllAliases(struct tagALIAS_HEADER *RootHeader);
CSR_API(CsrAddConsoleAlias);
CSR_API(CsrGetConsoleAlias);
CSR_API(CsrGetAllConsoleAliases);
CSR_API(CsrGetAllConsoleAliasesLength);
CSR_API(CsrGetConsoleAliasesExes);
CSR_API(CsrGetConsoleAliasesExesLength);

/* EOF */
