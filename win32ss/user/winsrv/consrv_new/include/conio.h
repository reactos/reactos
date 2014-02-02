/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/include/conio.h
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
//  ANY_TYPE_BUFFER = 0x00, // --> Match any types of IO handles
    TEXTMODE_BUFFER = 0x01, // --> Output-type handles for text SBs
    GRAPHICS_BUFFER = 0x02, // --> Output-type handles for graphics SBs
    SCREEN_BUFFER   = 0x03, // --> Any SB type
    INPUT_BUFFER    = 0x04  // --> Input-type handles
} CONSOLE_IO_OBJECT_TYPE;

typedef struct _CONSOLE_IO_OBJECT
{
    CONSOLE_IO_OBJECT_TYPE Type;
    struct _CONSOLE* /* PCONSOLE */ Console;
    LONG AccessRead, AccessWrite;
    LONG ExclusiveRead, ExclusiveWrite;
    LONG HandleCount;
} CONSOLE_IO_OBJECT, *PCONSOLE_IO_OBJECT;


/******************************************************************************\
|*                                                                            *|
|*     Abstract "class" for screen-buffers, be they text-mode or graphics     *|
|*                                                                            *|
\******************************************************************************/

/*
 * See conoutput.c for the implementation
 */

typedef struct _CONSOLE_SCREEN_BUFFER CONSOLE_SCREEN_BUFFER,
                                    *PCONSOLE_SCREEN_BUFFER;

typedef struct _CONSOLE_SCREEN_BUFFER_VTBL
{
    CONSOLE_IO_OBJECT_TYPE (*GetType)(PCONSOLE_SCREEN_BUFFER This);
} CONSOLE_SCREEN_BUFFER_VTBL, *PCONSOLE_SCREEN_BUFFER_VTBL;

#define GetType(This)   (This)->Vtbl->GetType(This)

struct _CONSOLE_SCREEN_BUFFER
{
    CONSOLE_IO_OBJECT Header;           /* Object header - MUST BE IN FIRST PLACE */
    PCONSOLE_SCREEN_BUFFER_VTBL Vtbl;   /* Virtual table */

    LIST_ENTRY ListEntry;               /* Entry in console's list of buffers */

    COORD   ScreenBufferSize;           /* Size of this screen buffer. (Rows, Columns) for text-mode and (Width, Height) for graphics */
    COORD   ViewSize;                   /* Associated "view" (i.e. console) size */

    COORD   OldScreenBufferSize;        /* Old size of this screen buffer */
    COORD   OldViewSize;                /* Old associated view size */

    COORD   ViewOrigin;                 /* Beginning offset for the actual display area */

/***** Put that VV in TEXTMODE_SCREEN_BUFFER ?? *****/
    USHORT  VirtualY;                   /* Top row of buffer being displayed, reported to callers */

    COORD   CursorPosition;             /* Current cursor position */
    BOOLEAN CursorBlinkOn;
    BOOLEAN ForceCursorOff;
//  ULONG   CursorSize;
    CONSOLE_CURSOR_INFO CursorInfo; // FIXME: Keep this member or not ??
/*********************************************/

//  WORD   ScreenDefaultAttrib;         /* Default screen char attribute */
//  WORD   PopupDefaultAttrib;          /* Default popup char attribute */
    USHORT Mode;                        /* Output buffer modes */
};



/******************************************************************************\
|*                                                                            *|
|*           Text-mode and graphics-mode screen-buffer "classes"              *|
|*                                                                            *|
\******************************************************************************/

/*
 * See text.c for the implementation
 */

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

typedef struct _TEXTMODE_BUFFER_INFO
{
    COORD   ScreenBufferSize;
    USHORT  ScreenAttrib;
    USHORT  PopupAttrib;
    BOOLEAN IsCursorVisible;
    ULONG   CursorSize;
} TEXTMODE_BUFFER_INFO, *PTEXTMODE_BUFFER_INFO;

typedef struct _TEXTMODE_SCREEN_BUFFER
{
    CONSOLE_SCREEN_BUFFER;      /* Screen buffer base class - MUST BE IN FIRST PLACE */

    PCHAR_INFO Buffer;          /* Pointer to UNICODE screen buffer (Buffer->Char.UnicodeChar only is valid, not Char.AsciiChar) */

    WORD ScreenDefaultAttrib;   /* Default screen char attribute */
    WORD PopupDefaultAttrib;    /* Default popup char attribute */
} TEXTMODE_SCREEN_BUFFER, *PTEXTMODE_SCREEN_BUFFER;


/*
 * See graphics.c for the implementation
 */

typedef struct _GRAPHICS_BUFFER_INFO
{
    CONSOLE_GRAPHICS_BUFFER_INFO Info;
} GRAPHICS_BUFFER_INFO, *PGRAPHICS_BUFFER_INFO;

typedef struct _GRAPHICS_SCREEN_BUFFER
{
    CONSOLE_SCREEN_BUFFER;          /* Screen buffer base class - MUST BE IN FIRST PLACE */

    ULONG   BitMapInfoLength;       /* Real size of the structure pointed by BitMapInfo */
    LPBITMAPINFO BitMapInfo;        /* Information on the bitmap buffer */
    ULONG   BitMapUsage;            /* See the uUsage parameter of GetDIBits */
    HANDLE  hSection;               /* Handle to the memory shared section for the bitmap buffer */
    PVOID   BitMap;                 /* Our bitmap buffer */
    PVOID   ClientBitMap;           /* A copy of the client view of our bitmap buffer */
    HANDLE  Mutex;                  /* Our mutex, used to synchronize read / writes to the bitmap buffer */
    HANDLE  ClientMutex;            /* A copy of the client handle to our mutex */
    HANDLE  ClientProcess;          /* Handle to the client process who opened the buffer, to unmap the view */
} GRAPHICS_SCREEN_BUFFER, *PGRAPHICS_SCREEN_BUFFER;



typedef struct _CONSOLE_INPUT_BUFFER
{
    CONSOLE_IO_OBJECT Header;       /* Object header - MUST BE IN FIRST PLACE */

    ULONG       InputBufferSize;    /* Size of this input buffer */
    LIST_ENTRY  InputEvents;        /* List head for input event queue */
    HANDLE      ActiveEvent;        /* Event set when an input event is added in its queue */
    LIST_ENTRY  ReadWaitQueue;      /* List head for the queue of read wait blocks */

    USHORT      Mode;               /* Input buffer modes */
} CONSOLE_INPUT_BUFFER, *PCONSOLE_INPUT_BUFFER;


typedef struct _FRONTEND FRONTEND, *PFRONTEND;
/* HACK: */ typedef struct _CONSOLE_INFO *PCONSOLE_INFO;
typedef struct _FRONTEND_VTBL
{
    /*
     * Internal interface (functions called by the console server only)
     */
    NTSTATUS (WINAPI *InitFrontEnd)(IN OUT PFRONTEND This,
                                    IN struct _CONSOLE* Console);
    VOID (WINAPI *DeinitFrontEnd)(IN OUT PFRONTEND This);

    /* Interface used for both text-mode and graphics screen buffers */
    VOID (WINAPI *DrawRegion)(IN OUT PFRONTEND This,
                              SMALL_RECT* Region);
    /* Interface used only for text-mode screen buffers */
    VOID (WINAPI *WriteStream)(IN OUT PFRONTEND This,
                               SMALL_RECT* Block,
                               SHORT CursorStartX,
                               SHORT CursorStartY,
                               UINT ScrolledLines,
                               PWCHAR Buffer,
                               UINT Length);
    BOOL (WINAPI *SetCursorInfo)(IN OUT PFRONTEND This,
                                 PCONSOLE_SCREEN_BUFFER ScreenBuffer);
    BOOL (WINAPI *SetScreenInfo)(IN OUT PFRONTEND This,
                                 PCONSOLE_SCREEN_BUFFER ScreenBuffer,
                                 SHORT OldCursorX,
                                 SHORT OldCursorY);
    VOID (WINAPI *ResizeTerminal)(IN OUT PFRONTEND This);
    BOOL (WINAPI *ProcessKeyCallback)(IN OUT PFRONTEND This,
                                      MSG* msg,
                                      BYTE KeyStateMenu,
                                      DWORD ShiftState,
                                      UINT VirtualKeyCode,
                                      BOOL Down);
    VOID (WINAPI *RefreshInternalInfo)(IN OUT PFRONTEND This);

    /*
     * External interface (functions corresponding to the Console API)
     */
    VOID (WINAPI *ChangeTitle)(IN OUT PFRONTEND This);
    BOOL (WINAPI *ChangeIcon)(IN OUT PFRONTEND This,
                              HICON hWindowIcon);
    HWND (WINAPI *GetConsoleWindowHandle)(IN OUT PFRONTEND This);
    VOID (WINAPI *GetLargestConsoleWindowSize)(IN OUT PFRONTEND This,
                                               PCOORD pSize);
    ULONG (WINAPI *GetDisplayMode)(IN OUT PFRONTEND This);
    BOOL  (WINAPI *SetDisplayMode)(IN OUT PFRONTEND This,
                                   ULONG NewMode);
    INT   (WINAPI *ShowMouseCursor)(IN OUT PFRONTEND This,
                                    BOOL Show);
    BOOL  (WINAPI *SetMouseCursor)(IN OUT PFRONTEND This,
                                   HCURSOR hCursor);
    HMENU (WINAPI *MenuControl)(IN OUT PFRONTEND This,
                                UINT cmdIdLow,
                                UINT cmdIdHigh);
    BOOL  (WINAPI *SetMenuClose)(IN OUT PFRONTEND This,
                                 BOOL Enable);

#if 0 // Possible future front-end interface
    BOOL (WINAPI *GetFrontEndProperty)(IN OUT PFRONTEND This,
                                       ULONG Flag,
                                       PVOID Info,
                                       ULONG Size);
    BOOL (WINAPI *SetFrontEndProperty)(IN OUT PFRONTEND This,
                                       ULONG Flag,
                                       PVOID Info /*,
                                       ULONG Size */);
#endif
} FRONTEND_VTBL, *PFRONTEND_VTBL;

struct _FRONTEND
{
    PFRONTEND_VTBL Vtbl;        /* Virtual table */
    struct _CONSOLE* Console;   /* Console to which the frontend is attached to */
    PVOID Data;                 /* Private data  */
    PVOID OldData;              /* Reserved      */
};

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

    LIST_ENTRY ProcessList;                 /* List of processes owning the console. The first one is the so-called "Console Leader Process" */

    FRONTEND TermIFace;                     /* Frontend-specific interface */

/**************************** Input buffer and data ***************************/
    CONSOLE_INPUT_BUFFER InputBuffer;       /* Input buffer of the console */

    /** Put those things in TEXTMODE_SCREEN_BUFFER ?? **/
    PWCHAR LineBuffer;                      /* Current line being input, in line buffered mode */
    WORD LineMaxSize;                       /* Maximum size of line in characters (including CR+LF) */
    WORD LineSize;                          /* Current size of line */
    WORD LinePos;                           /* Current position within line */
    BOOLEAN LineComplete;                   /* User pressed enter, ready to send back to client */
    BOOLEAN LineUpPressed;
    BOOLEAN LineInsertToggle;               /* Replace character over cursor instead of inserting */
    ULONG LineWakeupMask;                   /* Bitmap of which control characters will end line input */
    /***************************************************/

    BOOLEAN QuickEdit;
    BOOLEAN InsertMode;
    UINT CodePage;

    CONSOLE_SELECTION_INFO Selection;       /* Contains information about the selection */
    COORD dwSelectionCursor;                /* Selection cursor position, most of the time different from Selection.dwSelectionAnchor */

/******************************* Screen buffers *******************************/
    LIST_ENTRY BufferList;                  /* List of all screen buffers for this console */
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;    /* Pointer to currently active screen buffer */
    BYTE PauseFlags;
    HANDLE UnpauseEvent;
    LIST_ENTRY WriteWaitQueue;              /* List head for the queue of write wait blocks */
    UINT OutputCodePage;

/**************************** Aliases and Histories ***************************/
    struct _ALIAS_HEADER *Aliases;
    LIST_ENTRY HistoryBuffers;
    ULONG HistoryBufferSize;                /* Size for newly created history buffers */
    ULONG NumberOfHistoryBuffers;           /* Maximum number of history buffers allowed */
    BOOLEAN HistoryNoDup;                   /* Remove old duplicate history entries */

/****************************** Other properties ******************************/
    UNICODE_STRING OriginalTitle;           /* Original title of console, the one defined when the console leader is launched; it never changes. Always NULL-terminated */
    UNICODE_STRING Title;                   /* Title of console. Always NULL-terminated */

    COORD   ConsoleSize;                    /* The current size of the console, for text-mode only */
    BOOLEAN FixedSize;                      /* TRUE if the console is of fixed size */

    COLORREF Colors[16];                    /* Colour palette */

} CONSOLE, *PCONSOLE;

/* PauseFlags values (internal only) */
#define PAUSED_FROM_KEYBOARD  0x1
#define PAUSED_FROM_SCROLLBAR 0x2
#define PAUSED_FROM_SELECTION 0x4

/* console.c */
VOID FASTCALL ConioPause(PCONSOLE Console, UINT Flags);
VOID FASTCALL ConioUnpause(PCONSOLE Console, UINT Flags);

NTSTATUS NTAPI
ConDrvConsoleProcessCtrlEvent(IN PCONSOLE Console,
                              IN ULONG ProcessGroupId,
                              IN ULONG Event);

/* coninput.c */
VOID WINAPI ConioProcessKey(PCONSOLE Console, MSG* msg);
NTSTATUS FASTCALL ConioProcessInputEvent(PCONSOLE Console,
                                         PINPUT_RECORD InputEvent);

/* conoutput.c */
#define ConioInitRect(Rect, top, left, bottom, right) \
do {    \
    ((Rect)->Top) = top;    \
    ((Rect)->Left) = left;  \
    ((Rect)->Bottom) = bottom;  \
    ((Rect)->Right) = right;    \
} while (0)
#define ConioIsRectEmpty(Rect) \
    (((Rect)->Left > (Rect)->Right) || ((Rect)->Top > (Rect)->Bottom))
#define ConioRectHeight(Rect) \
    (((Rect)->Top) > ((Rect)->Bottom) ? 0 : ((Rect)->Bottom) - ((Rect)->Top) + 1)
#define ConioRectWidth(Rect) \
    (((Rect)->Left) > ((Rect)->Right) ? 0 : ((Rect)->Right) - ((Rect)->Left) + 1)

#define ConsoleUnicodeCharToAnsiChar(Console, dChar, sWChar) \
    WideCharToMultiByte((Console)->OutputCodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL)

#define ConsoleAnsiCharToUnicodeChar(Console, dWChar, sChar) \
    MultiByteToWideChar((Console)->OutputCodePage, 0, (sChar), 1, (dWChar), 1)

PCHAR_INFO ConioCoordToPointer(PTEXTMODE_SCREEN_BUFFER Buff, ULONG X, ULONG Y);
VOID FASTCALL ConioDrawConsole(PCONSOLE Console);
NTSTATUS ConioResizeBuffer(PCONSOLE Console,
                           PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                           COORD Size);
NTSTATUS ConioWriteConsole(PCONSOLE Console,
                           PTEXTMODE_SCREEN_BUFFER Buff,
                           PWCHAR Buffer,
                           DWORD Length,
                           BOOL Attrib);
DWORD FASTCALL ConioEffectiveCursorSize(PCONSOLE Console,
                                        DWORD Scale);

/* EOF */
