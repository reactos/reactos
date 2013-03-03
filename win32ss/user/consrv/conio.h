/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/conio.h
 * PURPOSE:         Internal console I/O interface
 * PROGRAMMERS:
 */

#pragma once

#define CSR_DEFAULT_CURSOR_SIZE 25
#define CURSOR_BLINK_TIME 500

/* Default attributes */
#define DEFAULT_SCREEN_ATTRIB   (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)
#define DEFAULT_POPUP_ATTRIB    (FOREGROUND_BLUE | FOREGROUND_RED | \
                                 BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY)

#ifndef WM_APP
#define WM_APP 0x8000
#endif
#define PM_CREATE_CONSOLE       (WM_APP + 1)
#define PM_DESTROY_CONSOLE      (WM_APP + 2)
#define PM_CONSOLE_BEEP         (WM_APP + 3)
#define PM_CONSOLE_SET_TITLE    (WM_APP + 4)


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
 ************************************************************************/

typedef struct _CONSOLE_SCREEN_BUFFER
{
    Object_t Header;                /* Object header */
    LIST_ENTRY ListEntry;           /* Entry in console's list of buffers */

    BYTE *Buffer; /* CHAR_INFO */   /* Pointer to screen buffer */

    COORD ScreenBufferSize;         /* Size of this screen buffer */
    COORD CursorPosition;           /* Current cursor position */

    USHORT ShowX, ShowY;            /* Beginning offset for the actual display area */
    USHORT VirtualY;                /* Top row of buffer being displayed, reported to callers */

    BOOLEAN CursorBlinkOn;
    BOOLEAN ForceCursorOff;
    ULONG   CursorSize;
    CONSOLE_CURSOR_INFO CursorInfo; // FIXME: Keep this member or not ??

    WORD ScreenDefaultAttrib;       /* Default screen char attribute */
    WORD PopupDefaultAttrib;        /* Default popup char attribute */
    USHORT Mode;
} CONSOLE_SCREEN_BUFFER, *PCONSOLE_SCREEN_BUFFER;

typedef struct _CONSOLE_INPUT_BUFFER
{
    Object_t Header;                /* Object header */

    ULONG InputBufferSize;          /* Size of this input buffer */
    LIST_ENTRY InputEvents;         /* List head for input event queue */
    HANDLE ActiveEvent;             /* Event set when an input event is added in its queue */
    LIST_ENTRY ReadWaitQueue;       /* List head for the queue of read wait blocks */

    USHORT Mode;                    /* Console Input Buffer mode flags */
} CONSOLE_INPUT_BUFFER, *PCONSOLE_INPUT_BUFFER;

typedef struct ConsoleInput_t
{
    LIST_ENTRY ListEntry;
    INPUT_RECORD InputEvent;
} ConsoleInput;

typedef struct _CONSOLE
{
    LONG ReferenceCount;                    /* Is incremented each time a handle to a screen-buffer or the input buffer of this console gets referenced, or the console gets locked */
    CRITICAL_SECTION Lock;

    struct _CONSOLE *Prev, *Next;           /* Next and Prev consoles in console wheel */
    struct _CONSOLE_VTBL *Vtbl;             /* Using CUI or GUI consoles */

    CLIENT_ID  ConsoleLeaderCID;            /* Contains the Console Leader Process CID for this console. TODO: Is it possible to compute it via the contents of the ProcessList list ?? */
    LIST_ENTRY ProcessList;

/**************************** Input buffer and data ***************************/
    CONSOLE_INPUT_BUFFER InputBuffer;       /* Input buffer of the console */

    PWCHAR LineBuffer;                      /* Current line being input, in line buffered mode */
    WORD LineMaxSize;                       /* Maximum size of line in characters (including CR+LF) */
    WORD LineSize;                          /* Current size of line */
    WORD LinePos;                           /* Current position within line */
    BOOLEAN LineComplete;                   /* User pressed enter, ready to send back to client */
    BOOLEAN LineUpPressed;
    BOOLEAN LineInsertToggle;               /* Replace character over cursor instead of inserting */
    ULONG LineWakeupMask;                   /* Bitmap of which control characters will end line input */

    BOOLEAN QuickEdit;
    BOOLEAN InsertMode;
    UINT CodePage;
    UINT OutputCodePage;

    CONSOLE_SELECTION_INFO Selection;

/******************************* Screen buffers *******************************/
    LIST_ENTRY BufferList;                  /* List of all screen buffers for this console */
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;    /* Pointer to currently active screen buffer */
    BYTE PauseFlags;
    HANDLE UnpauseEvent;
    LIST_ENTRY WriteWaitQueue;              /* List head for the queue of write wait blocks */

    DWORD HardwareState;                    /* _GDI_MANAGED, _DIRECT */
/* BOOLEAN */ ULONG FullScreen; // Give the type of console: GUI (windowed) or TUI (fullscreen)

/**************************** Aliases and Histories ***************************/
    struct _ALIAS_HEADER *Aliases;
    LIST_ENTRY HistoryBuffers;
    ULONG HistoryBufferSize;                /* Size for newly created history buffers */
    ULONG NumberOfHistoryBuffers;           /* Maximum number of history buffers allowed */
    BOOLEAN HistoryNoDup;                   /* Remove old duplicate history entries */

/******************************* Common UI data *******************************/
    UNICODE_STRING OriginalTitle;           /* Original title of console, the one when the console leader is launched. It is always NULL-terminated */
    UNICODE_STRING Title;                   /* Title of console. It is always NULL-terminated */

    COORD Size;                             /* Size of the console (different of the size of the screen buffer */
    COLORREF Colors[16];                    /* Colour palette */

/****************************** GUI-related data ******************************/
    HWND hWindow;                           /* Handle to the console's window */
    HICON hIcon;                            /* Handle to its icon (used when freeing) */
    HICON hIconSm;
    PVOID GuiData; /* PGUI_CONSOLE_DATA */

} CONSOLE, *PCONSOLE;

/**************************************************************\
\** Define the Console Leader Process for the console window **/
#define GWLP_CONSOLEWND_ALLOC  (2 * sizeof(LONG_PTR))
#define GWLP_CONSOLE_LEADER_PID 0
#define GWLP_CONSOLE_LEADER_TID 4

#define SetConsoleWndConsoleLeaderCID(Console)  \
do {    \
    SetWindowLongPtrW((Console)->hWindow, GWLP_CONSOLE_LEADER_PID, (LONG_PTR)((Console)->ConsoleLeaderCID.UniqueProcess));  \
    SetWindowLongPtrW((Console)->hWindow, GWLP_CONSOLE_LEADER_TID, (LONG_PTR)((Console)->ConsoleLeaderCID.UniqueThread ));  \
} while(0)
/**************************************************************/

typedef struct _CONSOLE_VTBL
{
    VOID (WINAPI *WriteStream)(PCONSOLE Console, SMALL_RECT* Block, LONG CursorStartX, LONG CursorStartY,
                               UINT ScrolledLines, CHAR *Buffer, UINT Length);
    VOID (WINAPI *DrawRegion)(PCONSOLE Console, SMALL_RECT* Region);
    BOOL (WINAPI *SetCursorInfo)(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER ScreenBuffer);
    BOOL (WINAPI *SetScreenInfo)(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER ScreenBuffer,
                                 UINT OldCursorX, UINT OldCursorY);
    BOOL (WINAPI *UpdateScreenInfo)(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER ScreenBuffer);
    VOID (WINAPI *ChangeTitle)(PCONSOLE Console);
    VOID (WINAPI *CleanupConsole)(PCONSOLE Console);
    BOOL (WINAPI *ChangeIcon)(PCONSOLE Console, HICON hWindowIcon);
    NTSTATUS (WINAPI *ResizeBuffer)(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER ScreenBuffer, COORD Size);
    BOOL (WINAPI *ProcessKeyCallback)(PCONSOLE Console, MSG* msg, BYTE KeyStateMenu, DWORD ShiftState, UINT VirtualKeyCode, BOOL Down);
} CONSOLE_VTBL, *PCONSOLE_VTBL;

/* CONSOLE_SELECTION_INFO dwFlags values */
#define CONSOLE_NO_SELECTION          0x0
#define CONSOLE_SELECTION_IN_PROGRESS 0x1
#define CONSOLE_SELECTION_NOT_EMPTY   0x2
#define CONSOLE_MOUSE_SELECTION       0x4
#define CONSOLE_MOUSE_DOWN            0x8
/* HistoryFlags values */
#define HISTORY_NO_DUP_FLAG           0x1

/* PauseFlags values (internal only) */
#define PAUSED_FROM_KEYBOARD  0x1
#define PAUSED_FROM_SCROLLBAR 0x2
#define PAUSED_FROM_SELECTION 0x4

#define ConioDrawRegion(Console, Region) (Console)->Vtbl->DrawRegion((Console), (Region))
#define ConioWriteStream(Console, Block, CurStartX, CurStartY, ScrolledLines, Buffer, Length) \
          (Console)->Vtbl->WriteStream((Console), (Block), (CurStartX), (CurStartY), \
                                       (ScrolledLines), (Buffer), (Length))
#define ConioSetCursorInfo(Console, Buff) (Console)->Vtbl->SetCursorInfo((Console), (Buff))
#define ConioSetScreenInfo(Console, Buff, OldCursorX, OldCursorY) \
          (Console)->Vtbl->SetScreenInfo((Console), (Buff), (OldCursorX), (OldCursorY))
#define ConioUpdateScreenInfo(Console, Buff) \
          (Console)->Vtbl->UpdateScreenInfo((Console), (Buff))
#define ConioChangeTitle(Console) (Console)->Vtbl->ChangeTitle(Console)
#define ConioCleanupConsole(Console) (Console)->Vtbl->CleanupConsole(Console)
#define ConioChangeIcon(Console, hWindowIcon) (Console)->Vtbl->ChangeIcon((Console), (hWindowIcon))
#define ConioResizeBuffer(Console, Buff, Size) (Console)->Vtbl->ResizeBuffer((Console), (Buff), (Size))
#define ConioProcessKeyCallback(Console, Msg, KeyStateMenu, ShiftState, VirtualKeyCode, Down) (Console)->Vtbl->ProcessKeyCallback((Console), (Msg), (KeyStateMenu), (ShiftState), (VirtualKeyCode), (Down))

/* console.c */
VOID WINAPI ConSrvDeleteConsole(PCONSOLE Console);
VOID WINAPI ConSrvInitConsoleSupport(VOID);
NTSTATUS WINAPI ConSrvInitConsole(OUT PCONSOLE* NewConsole,
                                  IN LPCWSTR AppPath,
                                  IN OUT PCONSOLE_START_INFO ConsoleStartInfo,
                                  IN PCSR_PROCESS ConsoleLeaderProcess);
VOID FASTCALL ConioPause(PCONSOLE Console, UINT Flags);
VOID FASTCALL ConioUnpause(PCONSOLE Console, UINT Flags);
VOID FASTCALL ConSrvConsoleCtrlEvent(DWORD Event, PCONSOLE_PROCESS_DATA ProcessData);
VOID FASTCALL ConSrvConsoleCtrlEventTimeout(DWORD Event,
                                            PCONSOLE_PROCESS_DATA ProcessData,
                                            DWORD Timeout);

/* coninput.c */
#define ConSrvGetInputBuffer(ProcessData, Handle, Ptr, Access, LockConsole) \
    ConSrvGetObject((ProcessData), (Handle), (Object_t **)(Ptr), NULL,      \
                    (Access), (LockConsole), CONIO_INPUT_BUFFER_MAGIC)
#define ConSrvGetInputBufferAndHandleEntry(ProcessData, Handle, Ptr, Entry, Access, LockConsole)    \
    ConSrvGetObject((ProcessData), (Handle), (Object_t **)(Ptr), (Entry),                           \
                    (Access), (LockConsole), CONIO_INPUT_BUFFER_MAGIC)
#define ConSrvReleaseInputBuffer(Buff, IsConsoleLocked) \
    ConSrvReleaseObject(&(Buff)->Header, (IsConsoleLocked))
VOID WINAPI ConioProcessKey(PCONSOLE Console, MSG* msg);

/* conoutput.c */
#define ConioRectHeight(Rect) \
    (((Rect)->Top) > ((Rect)->Bottom) ? 0 : ((Rect)->Bottom) - ((Rect)->Top) + 1)
#define ConioRectWidth(Rect) \
    (((Rect)->Left) > ((Rect)->Right) ? 0 : ((Rect)->Right) - ((Rect)->Left) + 1)
#define ConSrvGetScreenBuffer(ProcessData, Handle, Ptr, Access, LockConsole)    \
    ConSrvGetObject((ProcessData), (Handle), (Object_t **)(Ptr), NULL,          \
                    (Access), (LockConsole), CONIO_SCREEN_BUFFER_MAGIC)
#define ConSrvGetScreenBufferAndHandleEntry(ProcessData, Handle, Ptr, Entry, Access, LockConsole)   \
    ConSrvGetObject((ProcessData), (Handle), (Object_t **)(Ptr), (Entry),                           \
                    (Access), (LockConsole), CONIO_SCREEN_BUFFER_MAGIC)
#define ConSrvReleaseScreenBuffer(Buff, IsConsoleLocked)    \
    ConSrvReleaseObject(&(Buff)->Header, (IsConsoleLocked))
PBYTE FASTCALL ConioCoordToPointer(PCONSOLE_SCREEN_BUFFER Buf, ULONG X, ULONG Y);
VOID FASTCALL ConioDrawConsole(PCONSOLE Console);
NTSTATUS FASTCALL ConioWriteConsole(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER Buff,
                                    CHAR *Buffer, DWORD Length, BOOL Attrib);
NTSTATUS FASTCALL ConSrvCreateScreenBuffer(IN OUT PCONSOLE Console,
                                           OUT PCONSOLE_SCREEN_BUFFER* Buffer,
                                           IN COORD ScreenBufferSize,
                                           IN USHORT ScreenAttrib,
                                           IN USHORT PopupAttrib);
VOID WINAPI ConioDeleteScreenBuffer(PCONSOLE_SCREEN_BUFFER Buffer);
DWORD FASTCALL ConioEffectiveCursorSize(PCONSOLE Console, DWORD Scale);

/* alias.c */
VOID IntDeleteAllAliases(struct _ALIAS_HEADER *RootHeader);

/* lineinput.c */
struct _HISTORY_BUFFER;
VOID FASTCALL HistoryDeleteBuffer(struct _HISTORY_BUFFER *Hist);
VOID FASTCALL LineInputKeyDown(PCONSOLE Console, KEY_EVENT_RECORD *KeyEvent);

/* EOF */
