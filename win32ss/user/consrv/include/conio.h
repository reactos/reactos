/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/include/conio.h
 * PURPOSE:         Public Console I/O Interface
 * PROGRAMMERS:     Gé van Geldorp
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

#define CSR_DEFAULT_CURSOR_SIZE 25

/* Default attributes */
#define DEFAULT_SCREEN_ATTRIB   (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)
#define DEFAULT_POPUP_ATTRIB    (FOREGROUND_BLUE | FOREGROUND_RED   | \
                                 BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY)

/* Object type magic numbers */
typedef enum _CONSOLE_IO_OBJECT_TYPE
{
    INPUT_BUFFER  = 0x01,   // -->  Input-type handles
    SCREEN_BUFFER = 0x02    // --> Output-type handles
} CONSOLE_IO_OBJECT_TYPE;

typedef struct _CONSOLE_IO_OBJECT
{
    CONSOLE_IO_OBJECT_TYPE Type;
    struct _CONSOLE* /* PCONSOLE */ Console;
    LONG AccessRead, AccessWrite;
    LONG ExclusiveRead, ExclusiveWrite;
    LONG HandleCount;
} CONSOLE_IO_OBJECT, *PCONSOLE_IO_OBJECT;

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
    CONSOLE_IO_OBJECT Header;       /* Object header */
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
    ULONG  DisplayMode;
} CONSOLE_SCREEN_BUFFER, *PCONSOLE_SCREEN_BUFFER;

typedef struct _CONSOLE_INPUT_BUFFER
{
    CONSOLE_IO_OBJECT Header;       /* Object header */

    ULONG InputBufferSize;          /* Size of this input buffer */
    LIST_ENTRY InputEvents;         /* List head for input event queue */
    HANDLE ActiveEvent;             /* Event set when an input event is added in its queue */
    LIST_ENTRY ReadWaitQueue;       /* List head for the queue of read wait blocks */

    USHORT Mode;                    /* Console Input Buffer mode flags */
} CONSOLE_INPUT_BUFFER, *PCONSOLE_INPUT_BUFFER;

typedef struct _FRONTEND_VTBL
{
    /*
     * Internal interface (functions called by the console server only)
     */
    VOID (WINAPI *CleanupConsole)(struct _CONSOLE* Console);
    VOID (WINAPI *WriteStream)(struct _CONSOLE* Console,
                               SMALL_RECT* Block,
                               SHORT CursorStartX,
                               SHORT CursorStartY,
                               UINT ScrolledLines,
                               CHAR *Buffer,
                               UINT Length);
    VOID (WINAPI *DrawRegion)(struct _CONSOLE* Console,
                              SMALL_RECT* Region);
    BOOL (WINAPI *SetCursorInfo)(struct _CONSOLE* Console,
                                 PCONSOLE_SCREEN_BUFFER ScreenBuffer);
    BOOL (WINAPI *SetScreenInfo)(struct _CONSOLE* Console,
                                 PCONSOLE_SCREEN_BUFFER ScreenBuffer,
                                 SHORT OldCursorX,
                                 SHORT OldCursorY);
    BOOL (WINAPI *UpdateScreenInfo)(struct _CONSOLE* Console,
                                    PCONSOLE_SCREEN_BUFFER ScreenBuffer);
    BOOL (WINAPI *IsBufferResizeSupported)(struct _CONSOLE* Console);
    VOID (WINAPI *ResizeTerminal)(struct _CONSOLE* Console);
    BOOL (WINAPI *ProcessKeyCallback)(struct _CONSOLE* Console,
                                      MSG* msg,
                                      BYTE KeyStateMenu,
                                      DWORD ShiftState,
                                      UINT VirtualKeyCode,
                                      BOOL Down);
    VOID (WINAPI *RefreshInternalInfo)(struct _CONSOLE* Console);

    /*
     * External interface (functions corresponding to the Console API)
     */
    VOID (WINAPI *ChangeTitle)(struct _CONSOLE* Console);
    BOOL (WINAPI *ChangeIcon)(struct _CONSOLE* Console,
                              HICON hWindowIcon);
    HWND (WINAPI *GetConsoleWindowHandle)(struct _CONSOLE* Console);
    VOID (WINAPI *GetLargestConsoleWindowSize)(struct _CONSOLE* Console,
                                               PCOORD pSize);

} FRONTEND_VTBL, *PFRONTEND_VTBL;

typedef struct _FRONTEND_IFACE
{
    PFRONTEND_VTBL Vtbl;    /* Virtual table */
    PVOID Data;             /* Private data  */
    PVOID OldData;          /* Reserved      */
} FRONTEND_IFACE, *PFRONTEND_IFACE;

/*
 * WARNING: Change the state of the console ONLY when the console is locked !
 */
typedef enum _CONSOLE_STATE
{
    CONSOLE_INITIALIZING,   /* Console is initializing */
    CONSOLE_RUNNING     ,   /* Console running */
    CONSOLE_TERMINATING ,   /* Console about to be destroyed (but still not) */
    CONSOLE_IN_DESTRUCTION  /* Console in destruction */
} CONSOLE_STATE, *PCONSOLE_STATE;

typedef struct _CONSOLE
{
    LONG ReferenceCount;                    /* Is incremented each time a handle to something in the console (a screen-buffer or the input buffer of this console) gets referenced */
    CRITICAL_SECTION Lock;
    CONSOLE_STATE State;                    /* State of the console */

    LIST_ENTRY Entry;                       /* Entry in the list of consoles */
    LIST_ENTRY ProcessList;                 /* List of processes owning the console. The first one is the so-called "Console Leader Process" */

    FRONTEND_IFACE TermIFace;               /* Frontend-specific interface */

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

    CONSOLE_SELECTION_INFO Selection;       /* Contains information about the selection */
    COORD dwSelectionCursor;                /* Selection cursor position, most of the time different from Selection.dwSelectionAnchor */

/******************************* Screen buffers *******************************/
    LIST_ENTRY BufferList;                  /* List of all screen buffers for this console */
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;    /* Pointer to currently active screen buffer */
    BYTE PauseFlags;
    HANDLE UnpauseEvent;
    LIST_ENTRY WriteWaitQueue;              /* List head for the queue of write wait blocks */

    ULONG HardwareState;                    /* _GDI_MANAGED, _DIRECT */

/**************************** Aliases and Histories ***************************/
    struct _ALIAS_HEADER *Aliases;
    LIST_ENTRY HistoryBuffers;
    ULONG HistoryBufferSize;                /* Size for newly created history buffers */
    ULONG NumberOfHistoryBuffers;           /* Maximum number of history buffers allowed */
    BOOLEAN HistoryNoDup;                   /* Remove old duplicate history entries */

/****************************** Other properties ******************************/
    UNICODE_STRING OriginalTitle;           /* Original title of console, the one when the console leader is launched. Always NULL-terminated */
    UNICODE_STRING Title;                   /* Title of console. Always NULL-terminated */

/* SIZE */    COORD   ConsoleSize;          /* The size of the console */
    COLORREF Colors[16];                    /* Colour palette */

} CONSOLE, *PCONSOLE;

/* PauseFlags values (internal only) */
#define PAUSED_FROM_KEYBOARD  0x1
#define PAUSED_FROM_SCROLLBAR 0x2
#define PAUSED_FROM_SELECTION 0x4

/* console.c */
VOID FASTCALL ConioPause(PCONSOLE Console, UINT Flags);
VOID FASTCALL ConioUnpause(PCONSOLE Console, UINT Flags);
ULONG FASTCALL ConSrvConsoleProcessCtrlEvent(PCONSOLE Console,
                                             ULONG ProcessGroupId,
                                             DWORD Event);

/* coninput.c */
VOID WINAPI ConioProcessKey(PCONSOLE Console, MSG* msg);
NTSTATUS FASTCALL ConioProcessInputEvent(PCONSOLE Console,
                                         PINPUT_RECORD InputEvent);

/* conoutput.c */
#define ConioRectHeight(Rect) \
    (((Rect)->Top) > ((Rect)->Bottom) ? 0 : ((Rect)->Bottom) - ((Rect)->Top) + 1)
#define ConioRectWidth(Rect) \
    (((Rect)->Left) > ((Rect)->Right) ? 0 : ((Rect)->Right) - ((Rect)->Left) + 1)

PBYTE FASTCALL ConioCoordToPointer(PCONSOLE_SCREEN_BUFFER Buf,
                                   ULONG X,
                                   ULONG Y);
VOID FASTCALL ConioDrawConsole(PCONSOLE Console);
NTSTATUS FASTCALL ConioResizeBuffer(PCONSOLE Console,
                                    PCONSOLE_SCREEN_BUFFER ScreenBuffer,
                                    COORD Size);
NTSTATUS FASTCALL ConioWriteConsole(PCONSOLE Console,
                                    PCONSOLE_SCREEN_BUFFER Buff,
                                    CHAR *Buffer,
                                    DWORD Length,
                                    BOOL Attrib);
DWORD FASTCALL ConioEffectiveCursorSize(PCONSOLE Console,
                                        DWORD Scale);

/* EOF */
