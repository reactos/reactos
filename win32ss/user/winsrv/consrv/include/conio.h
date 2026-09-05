/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/include/conio.h
 * PURPOSE:         Public Console I/O Interface
 * PROGRAMMERS:     G� van Geldorp
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

#define GetType(This)   (((PCONSOLE_SCREEN_BUFFER)(This))->Header.Type)

typedef struct _CONSOLE_SCREEN_BUFFER
{
    CONSOLE_IO_OBJECT Header;           /* Object header - MUST BE IN FIRST PLACE */

    LIST_ENTRY ListEntry;               /* Entry in console's list of buffers */

    COORD   ScreenBufferSize;           /* Size of this screen buffer. (Rows, Columns) for text-mode and (Width, Height) for graphics-mode */
    COORD   ViewSize;                   /* Associated "view" (i.e. console) size */

    COORD   OldScreenBufferSize;        /* Old size of this screen buffer */
    COORD   OldViewSize;                /* Old associated view size */

    COORD   ViewOrigin;                 /* Beginning offset for the actual display area */

/***** Put that VV in TEXTMODE_SCREEN_BUFFER ?? *****/
    COORD   CursorPosition;             /* Current cursor position */
    BOOLEAN CursorBlinkOn;
    BOOLEAN ForceCursorOff;
//  ULONG   CursorSize;
    CONSOLE_CURSOR_INFO CursorInfo; // FIXME: Keep this member or not ??
/*********************************************/

    HPALETTE PaletteHandle;             /* Handle to the color palette associated to this buffer */
    UINT     PaletteUsage;              /* The new use of the system palette. See SetSystemPaletteUse 'uUsage' parameter */

//  USHORT   ScreenDefaultAttrib;       /* Default screen char attribute */
//  USHORT   PopupDefaultAttrib;        /* Default popup char attribute */
    USHORT Mode;                        /* Output buffer modes */
} CONSOLE_SCREEN_BUFFER, *PCONSOLE_SCREEN_BUFFER;



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

/*
 * Extended (24-bit) per-cell colour. One array of pairs rather than two parallel
 * planes: a cell's two colours are always read, written, filled and moved
 * together, so keeping them adjacent makes every such operation a single
 * statement and a single allocation.
 *
 * CLR_INVALID in either field means "no extended colour, fall back to the
 * cell's attribute".
 */
/* Number of entries in the console colour table */
#define CONSOLE_COLOR_TABLE_SIZE 16

typedef struct _CELL_RGB
{
    COLORREF Fg;
    COLORREF Bg;
} CELL_RGB, *PCELL_RGB;

typedef struct _TEXTMODE_BUFFER_INFO
{
    COORD   ScreenBufferSize;
    COORD   ViewSize;
    USHORT  ScreenAttrib;
    USHORT  PopupAttrib;
    ULONG   CursorSize;
    BOOLEAN IsCursorVisible;
} TEXTMODE_BUFFER_INFO, *PTEXTMODE_BUFFER_INFO;

typedef struct _TEXTMODE_SCREEN_BUFFER
{
    CONSOLE_SCREEN_BUFFER;      /* Screen buffer base class - MUST BE IN FIRST PLACE */

    USHORT     VirtualY;        /* Top row of buffer being displayed, reported to callers */
    PCHAR_INFO Buffer;          /* Pointer to UNICODE screen buffer (Buffer->Char.UnicodeChar only is valid, not Char.AsciiChar) */

    USHORT ScreenDefaultAttrib; /* Default screen char attribute */
    USHORT PopupDefaultAttrib;  /* Default popup char attribute */
    PCELL_RGB CellRgb;          /* Extended per-cell colours, NULL until VT needs them */
#define VT_PENDING_SEQUENCE_MAX 256
    struct _VT_MODE_STATE
    {
        BOOLEAN CursorSaved;        /* Whether a cursor position was saved */
        COORD   SavedCursorPos;     /* Last saved cursor position */
        USHORT  SavedAttributes;    /* SGR attributes parked by DECSC (ESC 7 / CSI s) */
        USHORT  CurrentAttributes;  /* Current SGR attributes */
        ULONG   PrivateModes;       /* Bitmask of active DEC private modes */
        /*
         * Alternate screen (DECSET 1047/1049). The alternate screen is a
         * content swap inside this same screen-buffer object rather than a
         * second object: the primary's cells are parked here and restored on
         * the way out, so the console's active buffer - and every handle to it -
         * stays valid across the switch.
         */
        BOOLEAN AlternateActive;                 /* TRUE while the alternate screen is shown */
        PCHAR_INFO SavedBuffer;                  /* Primary cells, parked */
        struct _CELL_RGB* SavedCellRgb;          /* Primary extended colours, parked */
        USHORT  SavedVirtualY;                   /* Saved scrollback origin */
        COORD   SavedScreenCursorPos;            /* Saved cursor position */
        CONSOLE_CURSOR_INFO SavedScreenCursorInfo; /* Saved cursor info */
        COORD   SavedViewOrigin;                 /* Saved viewport origin */
        SHORT   SavedScrollTop;                  /* Saved top margin */
        SHORT   SavedScrollBottom;               /* Saved bottom margin */
        CONSOLE_CURSOR_INFO DefaultCursorInfo;   /* Default cursor info for DECSCUSR reset */
        BOOLEAN UseRgbForeground;                /* TRUE when foreground colour uses 24-bit RGB */
        BOOLEAN UseRgbBackground;                /* TRUE when background colour uses 24-bit RGB */
        COLORREF CurrentFgColor;                 /* Current 24-bit foreground colour */
        COLORREF CurrentBgColor;                 /* Current 24-bit background colour */
        COLORREF SavedFgColor;                   /* Saved foreground colour for DECSC */
        COLORREF SavedBgColor;                   /* Saved background colour for DECSC */
        BOOLEAN SavedUseRgbForeground;           /* Saved RGB flag for foreground */
        BOOLEAN SavedUseRgbBackground;           /* Saved RGB flag for background */
        SHORT   ScrollTop;                       /* Top margin for scrolling (0-based) */
        SHORT   ScrollBottom;                    /* Bottom margin for scrolling (0-based inclusive) */
        BOOLEAN HyperlinkActive;                 /* TRUE when OSC 8 hyperlink is active */
        UNICODE_STRING HyperlinkUri;             /* Current hyperlink target */
        UCHAR   Charsets[4];                     /* Character sets designated into G0..G3 */
        UCHAR   ActiveCharset;                   /* Active GL charset slot (0-3) */
        UCHAR   PendingSingleShift;              /* Pending single-shift slot override (SS2/SS3) */
        UCHAR   SavedCharsets[4];                /* G0..G3 saved by DECSC */
        UCHAR   SavedActiveCharset;              /* Saved active charset selector for DECSC */
        WCHAR   LastWrittenChar;                 /* Last printable glyph emitted for REP */
        BOOLEAN LastCharValid;                   /* Tracks whether LastWrittenChar is valid */
        ULONG   MouseButtonState;                /* Tracks currently pressed mouse buttons */
        PUCHAR  TabStops;                        /* Dynamic tab-stop bitmap (one byte per column) */
        USHORT  TabStopLength;                   /* Number of columns represented in TabStops */
        SMALL_RECT DirtyRect;                    /* Region invalidated so far by this write */
        BOOLEAN DirtyValid;                      /* TRUE when DirtyRect holds anything */
        WCHAR   PendingSequence[VT_PENDING_SEQUENCE_MAX]; /* Unterminated VT sequence kept across writes */
        USHORT  PendingSequenceLength;           /* Number of valid WCHARs in PendingSequence */
    } VtState;
} TEXTMODE_SCREEN_BUFFER, *PTEXTMODE_SCREEN_BUFFER;

/*
 * Cell addressing and the extended per-cell colours.
 *
 * CellRgb is an optional side array indexed exactly like Buffer: it stays NULL
 * until a VT sequence actually asks for a 24-bit colour (see
 * ConioEnsureCellColors), so a console that never enables VT pays nothing for
 * it. Because it is a separate allocation, every operation that moves, fills or
 * copies cells must carry it along - always go through the helpers below rather
 * than touching the array directly, so there is one place per kind of operation
 * instead of one per call site.
 *
 * A run of cells inside a single row is contiguous in both arrays, so row-span
 * helpers can use RtlMoveMemory/RtlFillMemory.
 */

FORCEINLINE ULONG
ConioCoordToIndex(PTEXTMODE_SCREEN_BUFFER Buff, ULONG X, ULONG Y)
{
    ASSERT(X < (ULONG)Buff->ScreenBufferSize.X);
    ASSERT(Y < (ULONG)Buff->ScreenBufferSize.Y);
    return ((Y + Buff->VirtualY) % Buff->ScreenBufferSize.Y) * Buff->ScreenBufferSize.X + X;
}

/* TRUE when the VT parser owns this buffer's output. Derived from the mode bit
 * so there is exactly one representation of "is VT on". */
FORCEINLINE BOOLEAN
ConioIsVtActive(PTEXTMODE_SCREEN_BUFFER Buff)
{
    return !!(Buff->Mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

/* Base of row Y in the colour array, or NULL when it is not allocated */
FORCEINLINE PCELL_RGB
ConioCellRgbRow(PTEXTMODE_SCREEN_BUFFER Buff, ULONG Y)
{
    if (!Buff->CellRgb) return NULL;
    return Buff->CellRgb + ConioCoordToIndex(Buff, 0, Y);
}

FORCEINLINE COLORREF
ConioGetCellFgColor(PTEXTMODE_SCREEN_BUFFER Buff, ULONG X, ULONG Y)
{
    if (!Buff->CellRgb) return CLR_INVALID;
    return Buff->CellRgb[ConioCoordToIndex(Buff, X, Y)].Fg;
}

FORCEINLINE COLORREF
ConioGetCellBgColor(PTEXTMODE_SCREEN_BUFFER Buff, ULONG X, ULONG Y)
{
    if (!Buff->CellRgb) return CLR_INVALID;
    return Buff->CellRgb[ConioCoordToIndex(Buff, X, Y)].Bg;
}

FORCEINLINE VOID
ConioSetCellColors(PTEXTMODE_SCREEN_BUFFER Buff, ULONG X, ULONG Y, COLORREF Fg, COLORREF Bg)
{
    PCELL_RGB Cell;

    if (!Buff->CellRgb) return;

    Cell = &Buff->CellRgb[ConioCoordToIndex(Buff, X, Y)];
    Cell->Fg = Fg;
    Cell->Bg = Bg;
}

/* Set Count cells' colours starting at (X,Y); the run must stay inside row Y */
FORCEINLINE VOID
ConioFillCellColors(PTEXTMODE_SCREEN_BUFFER Buff, ULONG X, ULONG Y, ULONG Count, COLORREF Fg, COLORREF Bg)
{
    PCELL_RGB Row;
    ULONG i;

    if (Count == 0) return;

    Row = ConioCellRgbRow(Buff, Y);
    if (!Row) return;
    Row += X;

    /* CLR_INVALID is all-ones, so clearing a run is one memset */
    if (Fg == CLR_INVALID && Bg == CLR_INVALID)
    {
        RtlFillMemory(Row, Count * sizeof(CELL_RGB), 0xFF);
        return;
    }

    for (i = 0; i < Count; ++i)
    {
        Row[i].Fg = Fg;
        Row[i].Bg = Bg;
    }
}

/* Move Count cells' colours between already-computed array indices */
FORCEINLINE VOID
ConioMoveCellColorsByIndex(PTEXTMODE_SCREEN_BUFFER Buff, ULONG DstIndex, ULONG SrcIndex, ULONG Count)
{
    if (Count == 0 || !Buff->CellRgb) return;
    RtlMoveMemory(Buff->CellRgb + DstIndex, Buff->CellRgb + SrcIndex, Count * sizeof(CELL_RGB));
}

/* Move Count cells' colours within or between rows; runs must stay inside their row */
FORCEINLINE VOID
ConioMoveCellColors(PTEXTMODE_SCREEN_BUFFER Buff, ULONG DstX, ULONG DstY, ULONG SrcX, ULONG SrcY, ULONG Count)
{
    if (Count == 0 || !Buff->CellRgb) return;
    ConioMoveCellColorsByIndex(Buff, ConioCoordToIndex(Buff, DstX, DstY), ConioCoordToIndex(Buff, SrcX, SrcY), Count);
}

/*
 * Allocate the colour array on first use. Returns FALSE (leaving it NULL, i.e.
 * attribute-only rendering) if the allocation fails - callers treat extended
 * colour as best-effort and must not fail the write because of it.
 */
BOOLEAN ConioEnsureCellColors(PTEXTMODE_SCREEN_BUFFER Buff);


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

    ULONG       InputBufferSize;    /* Size of this input buffer (maximum number of events) -- UNUSED!! */
    ULONG       NumberOfEvents;     /* Current number of events in the queue */
    LIST_ENTRY  InputEvents;        /* Input events queue list head */
    HANDLE      ActiveEvent;        /* Event set when an input event is added to the queue */

    USHORT      Mode;               /* Input buffer modes */
} CONSOLE_INPUT_BUFFER, *PCONSOLE_INPUT_BUFFER;


/*
 * Structure used to hold console information
 */
typedef struct _CONSOLE_INFO
{
    ULONG   InputBufferSize;
    COORD   ScreenBufferSize;
    COORD   ConsoleSize;    /* The size of the console */

    ULONG   CursorSize;
    BOOLEAN CursorBlinkOn;
    BOOLEAN ForceCursorOff;

    USHORT  ScreenAttrib; // CHAR_INFO ScreenFillAttrib
    USHORT  PopupAttrib;

    ULONG   CodePage;

} CONSOLE_INFO, *PCONSOLE_INFO;

typedef struct _TERMINAL TERMINAL, *PTERMINAL;

typedef struct _TERMINAL_VTBL
{
    /*
     * Internal interface (functions called by the console server only)
     */
    NTSTATUS (NTAPI *InitTerminal)(IN OUT PTERMINAL This,
                                   IN struct _CONSOLE* Console);
    VOID (NTAPI *DeinitTerminal)(IN OUT PTERMINAL This);



/************ Line discipline ***************/

    /* Interface used only for text-mode screen buffers */

    NTSTATUS (NTAPI *ReadStream)(IN OUT PTERMINAL This,
                                 IN BOOLEAN Unicode,
                                 /**PWCHAR Buffer,**/
                                 OUT PVOID Buffer,
                                 IN OUT PCONSOLE_READCONSOLE_CONTROL ReadControl,
                                 IN PVOID Parameter OPTIONAL,
                                 IN ULONG NumCharsToRead,
                                 OUT PULONG NumCharsRead OPTIONAL);
    NTSTATUS (NTAPI *WriteStream)(IN OUT PTERMINAL This,
                                  PTEXTMODE_SCREEN_BUFFER Buff,
                                  PWCHAR Buffer,
                                  DWORD Length,
                                  BOOL Attrib);

/************ Line discipline ***************/



    /* Interface used for both text-mode and graphics screen buffers */
    VOID (NTAPI *DrawRegion)(IN OUT PTERMINAL This,
                             SMALL_RECT* Region);
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
    VOID (NTAPI *GetLargestConsoleWindowSize)(IN OUT PTERMINAL This,
                                              PCOORD pSize);
    BOOL (NTAPI *SetPalette)(IN OUT PTERMINAL This,
                             HPALETTE PaletteHandle,
                             UINT PaletteUsage);
    BOOL (NTAPI *SetCodePage)(IN OUT PTERMINAL This,
                              UINT CodePage);
    INT  (NTAPI *ShowMouseCursor)(IN OUT PTERMINAL This,
                                  BOOL Show);

    /*
     * Console-level state the driver may need but does not own. These take only
     * PCONSOLE-level types so that condrv/ never has to reach up into
     * CONSRV_CONSOLE or into a frontend.
     */
    BOOL (NTAPI *SetTitle)(IN OUT PTERMINAL This, IN PCWSTR Title, IN ULONG Length);
    BOOL (NTAPI *GetColorTable)(IN OUT PTERMINAL This, OUT COLORREF* Colors, IN ULONG Count);
    BOOL (NTAPI *SetColorTable)(IN OUT PTERMINAL This, IN const COLORREF* Colors, IN ULONG Count);
    /* On success *Text is a NUL-terminated buffer the caller frees with ConsoleFreeHeap() */
    BOOL (NTAPI *GetClipboardText)(IN OUT PTERMINAL This, OUT PWCHAR* Text, OUT PULONG Length);
    BOOL (NTAPI *SetClipboardText)(IN OUT PTERMINAL This, IN PCWSTR Text, IN ULONG Length);

#if 0 // Possible future terminal interface
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
    struct _CONSOLE* Console;   /* Console to which the terminal is attached to */
    PVOID Context;              /* Private context */
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
/******************************* Console Set-up *******************************/
    LONG ReferenceCount;                    /* Is incremented each time a handle to something in the console (a screen-buffer or the input buffer of this console) gets referenced */
    CRITICAL_SECTION Lock;

    CONSOLE_STATE State;                    /* State of the console */
    TERMINAL TermIFace;                     /* Terminal-specific interface */

    BOOLEAN ConsolePaused;                  /* If TRUE, the console is paused */

/******************************** Input buffer ********************************/
    CONSOLE_INPUT_BUFFER InputBuffer;       /* Input buffer of the console */
    UINT InputCodePage;

/******************************* Screen buffers *******************************/
    LIST_ENTRY BufferList;                  /* List of all screen buffers for this console */
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;    /* Pointer to currently active screen buffer */
    UINT OutputCodePage;

/****************************** Other properties ******************************/
    COORD   ConsoleSize;                    /* The current size of the console, for text-mode only */
    BOOLEAN FixedSize;                      /* TRUE if the console is of fixed size */
    BOOLEAN IsCJK;                          /* TRUE if Chinese, Japanese or Korean (CJK) */
    BOOLEAN AllowVtOscClipboard;            /* TRUE when OSC 52 clipboard access is permitted */
    BOOLEAN AllowVtOscHyperlinks;           /* TRUE when OSC 8 hyperlinks are permitted */
    BOOLEAN AllowVtDcsPassthrough;          /* TRUE when raw DCS payloads may be processed */
} CONSOLE, *PCONSOLE;

/* console.c */
VOID NTAPI
ConDrvPause(PCONSOLE Console);
VOID NTAPI
ConDrvUnpause(PCONSOLE Console);

#define GetConsoleInputBufferMode(Console)  \
    (Console)->InputBuffer.Mode

#define CON_SET_OUTPUT_CP(Console, CodePage)\
do { \
    (Console)->OutputCodePage = (CodePage); \
    (Console)->IsCJK = IsCJKCodePage((Console)->OutputCodePage); \
} while (0)


/* conoutput.c */
PCHAR_INFO ConioCoordToPointer(PTEXTMODE_SCREEN_BUFFER Buff, ULONG X, ULONG Y);
NTSTATUS ConioResizeBuffer(PCONSOLE Console,
                           PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                           COORD Size);

/* wcwidth.c */
int mk_wcwidth_cjk(wchar_t ucs);

// NOTE: The check against 0x80 is to avoid calling the helper function
// for characters that we already know are not full-width.
#define IS_FULL_WIDTH(wch)  \
    (((USHORT)(wch) >= 0x0080) && (mk_wcwidth_cjk(wch) == 2))

/* EOF */
