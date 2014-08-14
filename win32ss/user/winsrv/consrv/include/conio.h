/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            consrv/include/conio.h
 * PURPOSE:         Public Console I/O Interface
 * PROGRAMMERS:     Gé van Geldorp
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

#include "rect.h"

/* Default attributes */
#define DEFAULT_SCREEN_ATTRIB   (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)
#define DEFAULT_POPUP_ATTRIB    (FOREGROUND_BLUE | FOREGROUND_RED   | \
                                 BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY)


/* Object type magic numbers */
typedef enum _CONSOLE_IO_OBJECT_TYPE
{
    UNKNOWN         = 0x00, // --> Unknown object
    TEXTMODE_BUFFER = 0x01, // --> Output-type object for text SBs
    GRAPHICS_BUFFER = 0x02, // --> Output-type object for graphics SBs
    SCREEN_BUFFER   = 0x03, // --> Any SB type
    INPUT_BUFFER    = 0x04, // --> Input-type object
    ANY_TYPE_BUFFER = 0x07, // --> Any IO object
} CONSOLE_IO_OBJECT_TYPE;

typedef struct _CONSOLE_IO_OBJECT
{
    CONSOLE_IO_OBJECT_TYPE Type;

    struct _CONSOLE* /* PCONSOLE */ Console;
    LONG ReferenceCount;    /* Is incremented each time a console object gets referenced */

    LONG AccessRead, AccessWrite;
    LONG ExclusiveRead, ExclusiveWrite;
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

    COORD   ScreenBufferSize;           /* Size of this screen buffer. (Rows, Columns) for text-mode and (Width, Height) for graphics-mode */
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

    HPALETTE PaletteHandle;             /* Handle to the color palette associated to this buffer */
    UINT     PaletteUsage;              /* The new use of the system palette. See SetSystemPaletteUse 'uUsage' parameter */

//  WORD   ScreenDefaultAttrib;         /* Default screen char attribute */
//  WORD   PopupDefaultAttrib;          /* Default popup char attribute */
    USHORT Mode;                        /* Output buffer modes */

    // PVOID Data;                         /* Private data for the frontend to use */
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
    ULONG   CursorSize;
    BOOLEAN IsCursorVisible;
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

    USHORT      Mode;               /* Input buffer modes */
} CONSOLE_INPUT_BUFFER, *PCONSOLE_INPUT_BUFFER;


typedef struct _TERMINAL TERMINAL, *PTERMINAL;
/* HACK: */ typedef struct _CONSOLE_INFO *PCONSOLE_INFO;
typedef struct _TERMINAL_VTBL
{
    /*
     * Internal interface (functions called by the console server only)
     */
    NTSTATUS (NTAPI *InitTerminal)(IN OUT PTERMINAL This,
                                   IN struct _CONSOLE* Console);
    VOID (NTAPI *DeinitTerminal)(IN OUT PTERMINAL This);

    /* Interface used for both text-mode and graphics screen buffers */
    VOID (NTAPI *DrawRegion)(IN OUT PTERMINAL This,
                             SMALL_RECT* Region);
    /* Interface used only for text-mode screen buffers */
    VOID (NTAPI *WriteStream)(IN OUT PTERMINAL This,
                              SMALL_RECT* Region,
                              SHORT CursorStartX,
                              SHORT CursorStartY,
                              UINT ScrolledLines,
                              PWCHAR Buffer,
                              UINT Length);
    BOOL (NTAPI *SetCursorInfo)(IN OUT PTERMINAL This,
                                PCONSOLE_SCREEN_BUFFER ScreenBuffer);
    BOOL (NTAPI *SetScreenInfo)(IN OUT PTERMINAL This,
                                PCONSOLE_SCREEN_BUFFER ScreenBuffer,
                                SHORT OldCursorX,
                                SHORT OldCursorY);
    VOID (NTAPI *ResizeTerminal)(IN OUT PTERMINAL This);
    VOID (NTAPI *SetActiveScreenBuffer)(IN OUT PTERMINAL This);
    VOID (NTAPI *ReleaseScreenBuffer)(IN OUT PTERMINAL This,
                                      IN PCONSOLE_SCREEN_BUFFER ScreenBuffer);

    /*
     * External interface (functions corresponding to the Console API)
     */
    VOID (NTAPI *ChangeTitle)(IN OUT PTERMINAL This);
    VOID (NTAPI *GetLargestConsoleWindowSize)(IN OUT PTERMINAL This,
                                              PCOORD pSize);
    // BOOL (NTAPI *GetSelectionInfo)(IN OUT PTERMINAL This,
                                   // PCONSOLE_SELECTION_INFO pSelectionInfo);
    BOOL (NTAPI *SetPalette)(IN OUT PTERMINAL This,
                             HPALETTE PaletteHandle,
                             UINT PaletteUsage);
    INT   (NTAPI *ShowMouseCursor)(IN OUT PTERMINAL This,
                                   BOOL Show);

#if 0 // Possible future front-end interface
    BOOL (NTAPI *GetTerminalProperty)(IN OUT PTERMINAL This,
                                      ULONG Flag,
                                      PVOID Info,
                                      ULONG Size);
    BOOL (NTAPI *SetTerminalProperty)(IN OUT PTERMINAL This,
                                      ULONG Flag,
                                      PVOID Info /*,
                                      ULONG Size */);
#endif
} TERMINAL_VTBL, *PTERMINAL_VTBL;

struct _TERMINAL
{
    PTERMINAL_VTBL Vtbl;        /* Virtual table */
    struct _CONSOLE* Console;   /* Console to which the frontend is attached to */
    PVOID Data;                 /* Private data  */
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

// HACK!!
struct _CONSOLE;
/* HACK: */ typedef struct _CONSOLE *PCONSOLE;
#include "conio_winsrv.h"

typedef struct _CONSOLE
{
/******************************* Console Set-up *******************************/
    LONG ReferenceCount;                    /* Is incremented each time a handle to something in the console (a screen-buffer or the input buffer of this console) gets referenced */
    CRITICAL_SECTION Lock;

    /**/WINSRV_CONSOLE;/**/ // HACK HACK!!

    CONSOLE_STATE State;                    /* State of the console */
    TERMINAL TermIFace;                     /* Frontend-specific interface */

    ULONG ConsoleID;                        /* The ID of the console */
    LIST_ENTRY ListEntry;                   /* Entry in the list of consoles */

/**************************** Input buffer and data ***************************/
    CONSOLE_INPUT_BUFFER InputBuffer;       /* Input buffer of the console */
    UINT InputCodePage;

    /** Put those things in CONSOLE_INPUT_BUFFER in PWINSRV_CONSOLE ?? **/
    PWCHAR  LineBuffer;                     /* Current line being input, in line buffered mode */
    ULONG   LineMaxSize;                    /* Maximum size of line in characters (including CR+LF) */
    ULONG   LineSize;                       /* Current size of line */
    ULONG   LinePos;                        /* Current position within line */
    BOOLEAN LineComplete;                   /* User pressed enter, ready to send back to client */
    BOOLEAN LineUpPressed;
    BOOLEAN LineInsertToggle;               /* Replace character over cursor instead of inserting */
    ULONG   LineWakeupMask;                 /* Bitmap of which control characters will end line input */

    /** In PWINSRV_CONSOLE ?? **/
    BOOLEAN InsertMode;
    /*************************************************/

/******************************* Screen buffers *******************************/
    LIST_ENTRY BufferList;                  /* List of all screen buffers for this console */
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;    /* Pointer to currently active screen buffer */
    UINT OutputCodePage;

/****************************** Other properties ******************************/
    UNICODE_STRING OriginalTitle;           /* Original title of console, the one defined when the console leader is launched; it never changes. Always NULL-terminated */
    UNICODE_STRING Title;                   /* Title of console. Always NULL-terminated */

    HANDLE UnpauseEvent;                    /* When != NULL, event for pausing the console */

    COORD   ConsoleSize;                    /* The current size of the console, for text-mode only */
    BOOLEAN FixedSize;                      /* TRUE if the console is of fixed size */

} CONSOLE; // , *PCONSOLE;

/* console.c */
VOID NTAPI
ConDrvPause(PCONSOLE Console);
VOID NTAPI
ConDrvUnpause(PCONSOLE Console);

NTSTATUS
ConSrvConsoleCtrlEvent(IN ULONG CtrlEvent,
                       IN PCONSOLE_PROCESS_DATA ProcessData);

/* coninput.c */
NTSTATUS
ConioAddInputEvents(PCONSOLE Console,
                    PINPUT_RECORD InputRecords,
                    ULONG NumEventsToWrite,
                    PULONG NumEventsWritten,
                    BOOLEAN AppendToEnd);
NTSTATUS
ConioProcessInputEvent(PCONSOLE Console,
                       PINPUT_RECORD InputEvent);

/* conoutput.c */

/*
 * From MSDN:
 * "The lpMultiByteStr and lpWideCharStr pointers must not be the same.
 *  If they are the same, the function fails, and GetLastError returns
 *  ERROR_INVALID_PARAMETER."
 */
#define ConsoleUnicodeCharToAnsiChar(Console, dChar, sWChar) \
    ASSERT((ULONG_PTR)dChar != (ULONG_PTR)sWChar); \
    WideCharToMultiByte((Console)->OutputCodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL)

#define ConsoleAnsiCharToUnicodeChar(Console, dWChar, sChar) \
    ASSERT((ULONG_PTR)dWChar != (ULONG_PTR)sChar); \
    MultiByteToWideChar((Console)->OutputCodePage, 0, (sChar), 1, (dWChar), 1)

PCHAR_INFO ConioCoordToPointer(PTEXTMODE_SCREEN_BUFFER Buff, ULONG X, ULONG Y);
VOID ConioDrawConsole(PCONSOLE Console);
NTSTATUS ConioResizeBuffer(PCONSOLE Console,
                           PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                           COORD Size);
NTSTATUS ConioWriteConsole(PCONSOLE Console,
                           PTEXTMODE_SCREEN_BUFFER Buff,
                           PWCHAR Buffer,
                           DWORD Length,
                           BOOL Attrib);

/* EOF */
