/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    output.c

Abstract:

        This file implements the video buffer management.

Author:

    Therese Stowell (thereses) 6-Nov-1990

Revision History:

Notes:

 ScreenBuffer data structure overview:

 each screen buffer has an array of ROW structures.  each ROW structure
 contains the data for one row of text.  the data stored for one row of
 text is a character array and an attribute array.  the character array
 is allocated the full length of the row from the heap, regardless of the
 non-space length. we also maintain the non-space length.  the character
 array is initialized to spaces.  the attribute
 array is run length encoded (i.e 5 BLUE, 3 RED). if there is only one
 attribute for the whole row (the normal case), it is stored in the ATTR_ROW
 structure.  otherwise the attr string is allocated from the heap.

 ROW - CHAR_ROW - CHAR string
     \          \ length of char string
      \
       ATTR_ROW - ATTR_PAIR string
                \ length of attr pair string
 ROW
 ROW
 ROW

 ScreenInfo->Rows points to the ROW array. ScreenInfo->Rows[0] is not
 necessarily the top row. ScreenInfo->BufferInfo.TextInfo.FirstRow contains the index of
 the top row.  That means scrolling (if scrolling entire screen)
 merely involves changing the FirstRow index,
 filling in the last row, and updating the screen.

--*/

#include "precomp.h"
#pragma hdrstop


//#define THERESES_DEBUG 1

//#define PROFILE_GDI
#ifdef PROFILE_GDI
LONG ScrollDCCount;
LONG ExtTextOutCount;
LONG TextColor=1;

#define SCROLLDC_CALL ScrollDCCount++
#define TEXTOUT_CALL ExtTextOutCount++
#define TEXTCOLOR_CALL TextColor++
#else
#define SCROLLDC_CALL
#define TEXTOUT_CALL
#define TEXTCOLOR_CALL
#endif

#define ITEM_MAX_SIZE 256

// BUGBUG get the real include file from progman
typedef struct _PMIconData {
       DWORD dwResSize;
       DWORD dwVer;
       BYTE iResource;  // icon resource
} PMICONDATA, *LPPMICONDATA;

//
// Screen dimensions
//

int ConsoleFullScreenX;
int ConsoleFullScreenY;
int ConsoleCaptionY;
int MinimumWidthX;
SHORT VerticalScrollSize;
SHORT HorizontalScrollSize;

SHORT VerticalClientToWindow;
SHORT HorizontalClientToWindow;

PCHAR_INFO ScrollBuffer;
ULONG ScrollBufferSize;
CRITICAL_SECTION ScrollBufferLock;

// this value keeps track of the number of existing console windows.
// if a window is created when this value is zero, the Face Names
// must be reenumerated because no WM_FONTCHANGE message was processed
// if there's no window.
LONG gnConsoleWindows;

BOOL gfInitSystemMetrics;

BOOL UsePolyTextOut;

HRGN ghrgnScroll;
LPRGNDATA gprgnData;

ULONG gucWheelScrollLines;

UINT guCaretBlinkTime;

#define GRGNDATASIZE  (sizeof(RGNDATAHEADER) + (6 * sizeof(RECTL)))


#define CharSizeOf(x)   (sizeof(x) / sizeof(*x))
#define LockScrollBuffer() RtlEnterCriticalSection(&ScrollBufferLock)
#define UnlockScrollBuffer() RtlLeaveCriticalSection(&ScrollBufferLock)

#define SetWindowConsole(hWnd, Console) SetWindowLongPtr((hWnd), GWLP_USERDATA, (LONG_PTR)(Console))

#ifdef LATER
#ifndef IS_IME_KBDLAYOUT
#define IS_IME_KBDLAYOUT(hkl) ((((ULONG_PTR)(hkl)) & 0xf0000000) == 0xe0000000)
#endif
#endif


void GetNonBiDiKeyboardLayout(HKL * phklActive);

VOID FreeConsoleBitmap(IN PSCREEN_INFORMATION ScreenInfo);

VOID
ScrollIfNecessary(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo
    );

VOID
ProcessResizeWindow(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PCONSOLE_INFORMATION Console,
    IN LPWINDOWPOS WindowPos
    );

NTSTATUS
AllocateScrollBuffer(
    DWORD Size
    );

VOID FreeScrollBuffer ( VOID );

VOID
InternalUpdateScrollBars(
    IN PSCREEN_INFORMATION ScreenInfo
    );

#if defined(FE_SB)
BOOL
SB_PolyTextOutCandidate(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Region
    );

VOID
SB_ConsolePolyTextOut(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Region
    );
#endif



VOID
InitializeSystemMetrics( VOID )
{
    RECT WindowSize;

    gfInitSystemMetrics = FALSE;
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &gucWheelScrollLines, FALSE);
    ConsoleFullScreenX = GetSystemMetrics(SM_CXFULLSCREEN);
    ConsoleFullScreenY = GetSystemMetrics(SM_CYFULLSCREEN);
    ConsoleCaptionY = GetSystemMetrics(SM_CYCAPTION);
    VerticalScrollSize = (SHORT)GetSystemMetrics(SM_CXVSCROLL);
    HorizontalScrollSize = (SHORT)GetSystemMetrics(SM_CYHSCROLL);
    WindowSize.left = WindowSize.top = 0;
    WindowSize.right = WindowSize.bottom = 50;
    AdjustWindowRectEx(&WindowSize,
                        CONSOLE_WINDOW_FLAGS,
                        FALSE,
                        CONSOLE_WINDOW_EX_FLAGS
                       );
    VerticalClientToWindow = (SHORT)(WindowSize.right-WindowSize.left-50);
    HorizontalClientToWindow = (SHORT)(WindowSize.bottom-WindowSize.top-50);

#ifdef LATER
    gfIsIMEEnabled = !!GetSystemMetrics(SM_IMMENABLED);
    RIPMSG1(RIP_VERBOSE, "InitializeSystemMetrics: gfIsIMEEnabled=%d", gfIsIMEEnabled);
#endif

    guCaretBlinkTime = GetCaretBlinkTime();
}

VOID
GetWindowLimits(
    IN PSCREEN_INFORMATION ScreenInfo,
    OUT PWINDOW_LIMITS WindowLimits
    )
{
    HMONITOR hMonitor;
    MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
    COORD FontSize;

    //
    // If the system metrics have changed or there aren't any console
    // windows around, reinitialize the global valeus.
    //

    if (gfInitSystemMetrics || gnConsoleWindows == 0) {
        InitializeSystemMetrics();
    }

    if (ScreenInfo->Console &&
            (ScreenInfo->Console->hWnd || !(ScreenInfo->Console->Flags & CONSOLE_AUTO_POSITION)) &&
            ((hMonitor = MonitorFromRect(&ScreenInfo->Console->WindowRect, MONITOR_DEFAULTTOPRIMARY)) != NULL) &&
            GetMonitorInfo(hMonitor, &MonitorInfo)) {
        WindowLimits->FullScreenSize.X = (SHORT)(MonitorInfo.rcWork.right - MonitorInfo.rcWork.left);
        WindowLimits->FullScreenSize.Y = (SHORT)(MonitorInfo.rcWork.bottom - MonitorInfo.rcWork.top - ConsoleCaptionY);
    } else {
        WindowLimits->FullScreenSize.X = (SHORT)ConsoleFullScreenX;
        WindowLimits->FullScreenSize.Y = (SHORT)ConsoleFullScreenY;
    }

    if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
        FontSize = SCR_FONTSIZE(ScreenInfo);
    } else {
        FontSize.X = 1;
        FontSize.Y = 1;
    }

    WindowLimits->MinimumWindowSize.X = ((MinimumWidthX - VerticalClientToWindow + FontSize.X - 1) / FontSize.X);
    WindowLimits->MinimumWindowSize.Y = 1;
    WindowLimits->MaximumWindowSize.X = min(WindowLimits->FullScreenSize.X/FontSize.X, ScreenInfo->ScreenBufferSize.X);
    WindowLimits->MaximumWindowSize.X = max(WindowLimits->MaximumWindowSize.X, WindowLimits->MinimumWindowSize.X);
    WindowLimits->MaximumWindowSize.Y = min(WindowLimits->FullScreenSize.Y/FontSize.Y, ScreenInfo->ScreenBufferSize.Y);
    WindowLimits->MaxWindow.X = WindowLimits->MaximumWindowSize.X*FontSize.X + VerticalClientToWindow;
    WindowLimits->MaxWindow.Y = WindowLimits->MaximumWindowSize.Y*FontSize.Y + HorizontalClientToWindow;
}

VOID
InitializeScreenInfo( VOID )
{
    HDC hDC;

    InitializeMouseButtons();
    MinimumWidthX = GetSystemMetrics(SM_CXMIN);

    InitializeSystemMetrics();

    hDC = CreateDCW(L"DISPLAY",NULL,NULL,NULL);
    if (hDC != NULL) {
        UsePolyTextOut = GetDeviceCaps(hDC,TEXTCAPS) & TC_SCROLLBLT;
        DeleteDC(hDC);
    }
}

NTSTATUS
DoCreateScreenBuffer(
    IN PCONSOLE_INFORMATION Console,
    IN PCONSOLE_INFO ConsoleInfo
    )

/*++

    this routine figures out what parameters to pass to CreateScreenBuffer,
    based on the data from STARTUPINFO and the defaults in win.ini,
    then calls CreateScreenBuffer.

--*/

{
    CHAR_INFO Fill,PopupFill;
    COORD dwScreenBufferSize, dwWindowSize;
    NTSTATUS Status;
    int FontIndexWant;

    if (ConsoleInfo->dwStartupFlags & STARTF_USESHOWWINDOW) {
        Console->wShowWindow = ConsoleInfo->wShowWindow;
    } else {
        Console->wShowWindow = SW_SHOWNORMAL;
    }

#if 0
    {

            INT i;

            DbgPrint("[Link Server Properties for %ws]\n", ConsoleTitle );
            DbgPrint("    wFillAttribute      = 0x%04X\n", ConsoleInfo->wFillAttribute );
            DbgPrint("    wPopupFillAttribute = 0x%04X\n", ConsoleInfo->wPopupFillAttribute );
            DbgPrint("    dwScreenBufferSize  = (%d , %d)\n", ConsoleInfo->dwScreenBufferSize.X, ConsoleInfo->dwScreenBufferSize.Y );
            DbgPrint("    dwWindowSize        = (%d , %d)\n", ConsoleInfo->dwWindowSize.X, ConsoleInfo->dwWindowSize.Y );
            DbgPrint("    dwWindowOrigin      = (%d , %d)\n", ConsoleInfo->dwWindowOrigin.X, ConsoleInfo->dwWindowOrigin.Y );
            DbgPrint("    nFont               = %d\n", ConsoleInfo->nFont );
            DbgPrint("    nInputBufferSize    = %d\n", ConsoleInfo->nInputBufferSize );
            DbgPrint("    dwFontSize          = (%d , %d)\n", ConsoleInfo->dwFontSize.X, ConsoleInfo->dwFontSize.Y );
            DbgPrint("    uFontFamily         = 0x%08X\n", ConsoleInfo->uFontFamily );
            DbgPrint("    uFontWeight         = 0x%08X\n", ConsoleInfo->uFontWeight );
            DbgPrint("    FaceName            = %ws\n", ConsoleInfo->FaceName );
            DbgPrint("    uCursorSize         = %d\n", ConsoleInfo->uCursorSize );
            DbgPrint("    bFullScreen         = %s\n", ConsoleInfo->bFullScreen ? "TRUE" : "FALSE" );
            DbgPrint("    bQuickEdit          = %s\n", ConsoleInfo->bQuickEdit  ? "TRUE" : "FALSE" );
            DbgPrint("    bInsertMode         = %s\n", ConsoleInfo->bInsertMode ? "TRUE" : "FALSE" );
            DbgPrint("    bAutoPosition       = %s\n", ConsoleInfo->bAutoPosition ? "TRUE" : "FALSE" );
            DbgPrint("    uHistoryBufferSize  = %d\n", ConsoleInfo->uHistoryBufferSize );
            DbgPrint("    uNumHistoryBuffers  = %d\n", ConsoleInfo->uNumberOfHistoryBuffers );
            DbgPrint("    bHistoryNoDup       = %s\n", ConsoleInfo->bHistoryNoDup ? "TRUE" : "FALSE" );
            DbgPrint("    ColorTable = [" );
            i=0;
            while( i < 16 )
            {
                DbgPrint("\n         ");
                DbgPrint("0x%08X ", ConsoleInfo->ColorTable[i++]);
                DbgPrint("0x%08X ", ConsoleInfo->ColorTable[i++]);
                DbgPrint("0x%08X ", ConsoleInfo->ColorTable[i++]);
                DbgPrint("0x%08X ", ConsoleInfo->ColorTable[i++]);
            }
            DbgPrint( "]\n\n" );
        }
#endif

    //
    // Get values from consoleinfo (which was initialized through link)
    //

    Fill.Attributes = ConsoleInfo->wFillAttribute;
    Fill.Char.UnicodeChar = (WCHAR)' ';
    PopupFill.Attributes = ConsoleInfo->wPopupFillAttribute;
    PopupFill.Char.UnicodeChar = (WCHAR)' ';

    dwScreenBufferSize = ConsoleInfo->dwScreenBufferSize;
    if (!(ConsoleInfo->dwStartupFlags & STARTF_USECOUNTCHARS)) {
        if (Console->Flags & CONSOLE_NO_WINDOW) {
            dwScreenBufferSize.X = min(dwScreenBufferSize.X, 80);
            dwScreenBufferSize.Y = min(dwScreenBufferSize.Y, 25);
        }
    }
    if (dwScreenBufferSize.X == 0)
        dwScreenBufferSize.X = 1;
    if (dwScreenBufferSize.Y == 0)
        dwScreenBufferSize.Y = 1;

    //
    // Grab font
    //
#if defined(FE_SB)
    FontIndexWant = FindCreateFont(ConsoleInfo->uFontFamily,
                                   ConsoleInfo->FaceName,
                                   ConsoleInfo->dwFontSize,
                                   ConsoleInfo->uFontWeight,
                                   ConsoleInfo->uCodePage
                                  );
#else
    FontIndexWant = FindCreateFont(ConsoleInfo->uFontFamily,
                                   ConsoleInfo->FaceName,
                                   ConsoleInfo->dwFontSize,
                                   ConsoleInfo->uFontWeight);
#endif

    //
    // grab window size information
    //

    dwWindowSize = ConsoleInfo->dwWindowSize;
    if (ConsoleInfo->dwStartupFlags & STARTF_USESIZE) {
        dwWindowSize.X /= FontInfo[FontIndexWant].Size.X;
        dwWindowSize.Y /= FontInfo[FontIndexWant].Size.Y;
    } else if (Console->Flags & CONSOLE_NO_WINDOW) {
        dwWindowSize.X = min(dwWindowSize.X, 80);
        dwWindowSize.Y = min(dwWindowSize.Y, 25);
    }
    if (dwWindowSize.X == 0)
        dwWindowSize.X = 1;
    if (dwWindowSize.Y == 0)
        dwWindowSize.Y = 1;

    if (dwScreenBufferSize.X < dwWindowSize.X)
        dwScreenBufferSize.X = dwWindowSize.X;
    if (dwScreenBufferSize.Y < dwWindowSize.Y)
        dwScreenBufferSize.Y = dwWindowSize.Y;

    Console->dwWindowOriginX = ConsoleInfo->dwWindowOrigin.X;
    Console->dwWindowOriginY = ConsoleInfo->dwWindowOrigin.Y;

    if (ConsoleInfo->bAutoPosition) {
        Console->Flags |= CONSOLE_AUTO_POSITION;
        Console->dwWindowOriginX = CW_USEDEFAULT;
    } else {
        Console->WindowRect.left = Console->dwWindowOriginX;
        Console->WindowRect.top = Console->dwWindowOriginY;
        Console->WindowRect.right = Console->dwWindowOriginX + dwWindowSize.X * FontInfo[FontIndexWant].Size.X;
        Console->WindowRect.bottom = Console->dwWindowOriginY + dwWindowSize.Y * FontInfo[FontIndexWant].Size.Y;
    }

#ifdef i386
    if (FullScreenInitialized) {
        if (ConsoleInfo->bFullScreen) {
            Console->FullScreenFlags = CONSOLE_FULLSCREEN;
        }
    }
#endif
    if (ConsoleInfo->bQuickEdit) {
        Console->Flags |= CONSOLE_QUICK_EDIT_MODE;
    }
    Console->Flags |= CONSOLE_USE_PRIVATE_FLAGS;

    Console->InsertMode = (ConsoleInfo->bInsertMode != FALSE);
    Console->CommandHistorySize = (SHORT)ConsoleInfo->uHistoryBufferSize;
    Console->MaxCommandHistories = (SHORT)ConsoleInfo->uNumberOfHistoryBuffers;
    if (ConsoleInfo->bHistoryNoDup) {
        Console->Flags |= CONSOLE_HISTORY_NODUP;
    } else {
        Console->Flags &= ~CONSOLE_HISTORY_NODUP;
    }
    RtlCopyMemory(Console->ColorTable, ConsoleInfo->ColorTable, sizeof( Console->ColorTable ));

#if defined(FE_SB)
    // for FarEast version, we want get the code page from registry or shell32,
    // so we can specify console codepage by console.cpl or shell32
    // default codepage is OEMCP. scotthsu
    Console->CP = ConsoleInfo->uCodePage;
    Console->OutputCP = ConsoleInfo->uCodePage;
#if 0
    if (CONSOLE_IS_DBCS_ENABLED()){
        Console->fIsDBCSCP = !!IsAvailableFarEastCodePage(Console->CP);
        Console->fIsDBCSOutputCP = !!IsAvailableFarEastCodePage(Console->OutputCP);
    }
    else {
        Console->fIsDBCSCP = FALSE;
        Console->fIsDBCSOutputCP = FALSE;
    }
#else
    Console->fIsDBCSCP = CONSOLE_IS_DBCS_ENABLED() && IsAvailableFarEastCodePage(Console->CP);
    Console->fIsDBCSOutputCP = CONSOLE_IS_DBCS_ENABLED() && IsAvailableFarEastCodePage(Console->OutputCP);
#endif
#endif
#if defined(FE_IME)
    Console->ConsoleIme.ScrollWaitTimeout = guCaretBlinkTime * 2;
#endif
TryNewSize:
    Status = CreateScreenBuffer(&Console->ScreenBuffers,
                                dwWindowSize,
                                FontIndexWant,
                                dwScreenBufferSize,
                                Fill,
                                PopupFill,
                                Console,
                                CONSOLE_TEXTMODE_BUFFER,
                                NULL,
                                NULL,
                                NULL,
                                ConsoleInfo->uCursorSize,
                                ConsoleInfo->FaceName
                               );
    if (Status == STATUS_NO_MEMORY) {
        //
        // If we failed to create a large buffer, try again with a small one.
        //
        if (dwScreenBufferSize.X > 80 || dwScreenBufferSize.Y > 50) {
            dwScreenBufferSize.X = min(dwScreenBufferSize.X, 80);
            dwScreenBufferSize.Y = min(dwScreenBufferSize.Y, 50);
            dwWindowSize.X = min(dwWindowSize.X, dwScreenBufferSize.X);
            dwWindowSize.Y = min(dwWindowSize.Y, dwScreenBufferSize.Y);
            Console->Flags |= CONSOLE_DEFAULT_BUFFER_SIZE;
            goto TryNewSize;
        }
    }

    return Status;
}

NTSTATUS
CreateScreenBuffer(
    OUT PSCREEN_INFORMATION *ScreenInformation,
    IN COORD dwWindowSize,
    IN DWORD nFont,
    IN COORD dwScreenBufferSize,
    IN CHAR_INFO Fill,
    IN CHAR_INFO PopupFill,
    IN PCONSOLE_INFORMATION Console,
    IN DWORD Flags,
    IN PCONSOLE_GRAPHICS_BUFFER_INFO GraphicsBufferInfo OPTIONAL,
    OUT PVOID *lpBitmap OPTIONAL,
    OUT HANDLE *hMutex OPTIONAL,
    IN UINT CursorSize,
    IN LPWSTR FaceName
    )

/*++

Routine Description:

    This routine allocates and initializes the data associated with a screen
    buffer.  It also creates a window.

Arguments:

    ScreenInformation - the new screen buffer.

    dwWindowSize - the initial size of screen buffer's window (in rows/columns)

    nFont - the initial font to generate text with.

    dwScreenBufferSize - the initial size of the screen buffer (in rows/columns).

Return Value:


--*/

{
    LONG i,j;
    PSCREEN_INFORMATION ScreenInfo;
    NTSTATUS Status;
    PWCHAR TextRowPtr;
#if defined(FE_SB)
    PBYTE AttrRowPtr;
#endif
    WINDOW_LIMITS WindowLimits;

    DBGPRINT(("CreateScreenBuffer(\n"
              "    OUT PSCREEN_INFORMATION = %lx\n"
              "    dwWindowSize = (%d,%d)\n"
              "    nFont = %x\n"
              "    dwScreenBufferSize = (%d,%d)\n"
              "    Fill\n"
              "    PopupFill\n",
              ScreenInformation,
              dwWindowSize.X, dwWindowSize.Y,
              nFont,
              dwScreenBufferSize.X, dwScreenBufferSize.Y
              // Fill,
              // PopupFill
              ));
    DBGPRINT(("    PCONSOLE_INFORMATION = %lx\n"
              "    Flags = %lx\n"
              "    GraphicsBufferInfo\n"
              "    lpBitmap\n"
              "    *hMutex\n"
              "    ConsoleTitle \"%ls\"\n",
              Console,
              Flags,
              // GraphicsBufferInfo,
              // lpBitmap,
              // hMutex,
              Console->Title));

    /*
     * Make sure we have a valid font. Bail if no fonts are available.
     */
    ASSERT(nFont < NumberOfFonts);
    if (NumberOfFonts == 0) {
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * CONSIDER (adams): Allocate and zero memory, so
     * initialization is only of non-zero members.
     */
    ScreenInfo = (PSCREEN_INFORMATION)ConsoleHeapAlloc(MAKE_TAG( SCREEN_TAG ),sizeof(SCREEN_INFORMATION));
    if (ScreenInfo == NULL) {
        return STATUS_NO_MEMORY;
    }
    ScreenInfo->Console = Console;
    if ((ScreenInfo->Flags = Flags) & CONSOLE_TEXTMODE_BUFFER) {

        ASSERT(FontInfo[nFont].FaceName != NULL);

        ScreenInfo->BufferInfo.TextInfo.ListOfTextBufferFont = NULL;

        Status = StoreTextBufferFontInfo(ScreenInfo,
                                         nFont,
                                         FontInfo[nFont].Size,
                                         FontInfo[nFont].Family,
                                         FontInfo[nFont].Weight,
                                         FaceName ? FaceName : FontInfo[nFont].FaceName,
                                         Console->OutputCP);
        if (!NT_SUCCESS(Status)) {
            ConsoleHeapFree(ScreenInfo);
            return((ULONG) Status);
        }

        DBGFONTS(("DoCreateScreenBuffer sets FontSize(%d,%d), FontNumber=%x, Family=%x\n",
                SCR_FONTSIZE(ScreenInfo).X,
                SCR_FONTSIZE(ScreenInfo).Y,
                SCR_FONTNUMBER(ScreenInfo),
                SCR_FAMILY(ScreenInfo)));

        if (TM_IS_TT_FONT(FontInfo[nFont].Family)) {
            ScreenInfo->Flags &= ~CONSOLE_OEMFONT_DISPLAY;
        } else {
            ScreenInfo->Flags |= CONSOLE_OEMFONT_DISPLAY;
        }

        ScreenInfo->ScreenBufferSize = dwScreenBufferSize;
        GetWindowLimits(ScreenInfo, &WindowLimits);
        dwScreenBufferSize.X = max(dwScreenBufferSize.X, WindowLimits.MinimumWindowSize.X);
        dwWindowSize.X = max(dwWindowSize.X, WindowLimits.MinimumWindowSize.X);

        ScreenInfo->BufferInfo.TextInfo.ModeIndex = (ULONG)-1;
#ifdef i386
        if (Console->FullScreenFlags & CONSOLE_FULLSCREEN) {
            COORD WindowSize;
            ScreenInfo->BufferInfo.TextInfo.WindowedWindowSize = dwWindowSize;
            ScreenInfo->BufferInfo.TextInfo.WindowedScreenSize = dwScreenBufferSize;
            ScreenInfo->BufferInfo.TextInfo.ModeIndex = MatchWindowSize(Console->OutputCP,dwWindowSize,&WindowSize);
        }
#endif
        ScreenInfo->BufferInfo.TextInfo.FirstRow = 0;
        ScreenInfo->BufferInfo.TextInfo.Rows = (PROW)ConsoleHeapAlloc(MAKE_TAG( SCREEN_TAG ),dwScreenBufferSize.Y * sizeof(ROW));
        if (ScreenInfo->BufferInfo.TextInfo.Rows == NULL) {
            RemoveTextBufferFontInfo(ScreenInfo);
            ConsoleHeapFree(ScreenInfo);
            return STATUS_NO_MEMORY;
        }
        ScreenInfo->BufferInfo.TextInfo.TextRows = (PWCHAR)ConsoleHeapAlloc(MAKE_TAG( SCREEN_TAG ),dwScreenBufferSize.X*dwScreenBufferSize.Y*sizeof(WCHAR));
        if (ScreenInfo->BufferInfo.TextInfo.TextRows == NULL) {
            ConsoleHeapFree(ScreenInfo->BufferInfo.TextInfo.Rows);
            RemoveTextBufferFontInfo(ScreenInfo);
            ConsoleHeapFree(ScreenInfo);
            return STATUS_NO_MEMORY;
        }
#if defined(FE_SB)
        if (! CreateDbcsScreenBuffer(Console, dwScreenBufferSize,&ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer))
        {
            ConsoleHeapFree(ScreenInfo->BufferInfo.TextInfo.TextRows);
            ConsoleHeapFree(ScreenInfo->BufferInfo.TextInfo.Rows);
            RemoveTextBufferFontInfo(ScreenInfo);
            ConsoleHeapFree(ScreenInfo);
            return STATUS_NO_MEMORY;
        }

        AttrRowPtr=ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer.KAttrRows;
#endif
        for (i=0,TextRowPtr=ScreenInfo->BufferInfo.TextInfo.TextRows;
             i<dwScreenBufferSize.Y;
             i++,TextRowPtr+=dwScreenBufferSize.X)
        {
            ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.Left = dwScreenBufferSize.X;
            ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.OldLeft = INVALID_OLD_LENGTH;
            ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.Right = 0;
            ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.OldRight = INVALID_OLD_LENGTH;
            ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.Chars = TextRowPtr;
#if defined(FE_SB)
            ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.KAttrs = AttrRowPtr;
#endif
            for (j=0;j<dwScreenBufferSize.X;j++) {
                TextRowPtr[j] = (WCHAR)' ';
            }
#if defined(FE_SB)
            if (AttrRowPtr) {
                RtlZeroMemory(AttrRowPtr, dwScreenBufferSize.X);
                AttrRowPtr+=dwScreenBufferSize.X;
            }
#endif
            ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Length = 1;
            ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.AttrPair.Length = dwScreenBufferSize.X;
            ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.AttrPair.Attr = Fill.Attributes;
            ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Attrs = &ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.AttrPair;

        }
        ScreenInfo->BufferInfo.TextInfo.CursorSize = CursorSize;
        ScreenInfo->BufferInfo.TextInfo.CursorPosition.X = 0;
        ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y = 0;
        ScreenInfo->BufferInfo.TextInfo.CursorMoved = FALSE;
        ScreenInfo->BufferInfo.TextInfo.CursorVisible = TRUE;
        ScreenInfo->BufferInfo.TextInfo.CursorOn = FALSE;
        ScreenInfo->BufferInfo.TextInfo.CursorYSize = (WORD)CURSOR_SIZE_IN_PIXELS(SCR_FONTSIZE(ScreenInfo).Y,ScreenInfo->BufferInfo.TextInfo.CursorSize);
        ScreenInfo->BufferInfo.TextInfo.UpdatingScreen = 0;
        ScreenInfo->BufferInfo.TextInfo.DoubleCursor = FALSE;
        ScreenInfo->BufferInfo.TextInfo.DelayCursor = FALSE;
        ScreenInfo->BufferInfo.TextInfo.Flags = SINGLE_ATTRIBUTES_PER_LINE;
        ScreenInfo->ScreenBufferSize = dwScreenBufferSize;
        ScreenInfo->Window.Left = 0;
        ScreenInfo->Window.Top = 0;
        ScreenInfo->Window.Right = dwWindowSize.X - 1;
        ScreenInfo->Window.Bottom = dwWindowSize.Y - 1;
        if (ScreenInfo->Window.Right >= WindowLimits.MaximumWindowSize.X) {
            ScreenInfo->Window.Right = WindowLimits.MaximumWindowSize.X-1;
            dwWindowSize.X = CONSOLE_WINDOW_SIZE_X(ScreenInfo);
        }
        if (ScreenInfo->Window.Bottom >= WindowLimits.MaximumWindowSize.Y) {
            ScreenInfo->Window.Bottom = WindowLimits.MaximumWindowSize.Y-1;
            dwWindowSize.Y = CONSOLE_WINDOW_SIZE_Y(ScreenInfo);
        }
        ScreenInfo->WindowMaximizedX = (dwWindowSize.X == dwScreenBufferSize.X);
        ScreenInfo->WindowMaximizedY = (dwWindowSize.Y == dwScreenBufferSize.Y);
#if defined(FE_SB)
#if defined(_X86_)
        ScreenInfo->BufferInfo.TextInfo.MousePosition.X = 0;
        ScreenInfo->BufferInfo.TextInfo.MousePosition.Y = 0;
#endif // i386

        ScreenInfo->BufferInfo.TextInfo.CursorBlink = TRUE;
        ScreenInfo->BufferInfo.TextInfo.CursorDBEnable = TRUE;
#endif

    }
    else {
        Status = CreateConsoleBitmap(GraphicsBufferInfo,
                              ScreenInfo,
                              lpBitmap,
                              hMutex
                             );
        if (!NT_SUCCESS(Status)) {
            ConsoleHeapFree(ScreenInfo);
            return Status;
        }
        ScreenInfo->WindowMaximizedX = TRUE;
        ScreenInfo->WindowMaximizedY = TRUE;
    }

    ScreenInfo->WindowMaximized = FALSE;
    ScreenInfo->RefCount = 0;
    ScreenInfo->ShareAccess.OpenCount = 0;
    ScreenInfo->ShareAccess.Readers = 0;
    ScreenInfo->ShareAccess.Writers = 0;
    ScreenInfo->ShareAccess.SharedRead = 0;
    ScreenInfo->ShareAccess.SharedWrite = 0;
    ScreenInfo->CursorHandle = ghNormalCursor;
    ScreenInfo->CursorDisplayCount = 0;
    ScreenInfo->CommandIdLow = (UINT)-1;
    ScreenInfo->CommandIdHigh = (UINT)-1;
    ScreenInfo->dwUsage = SYSPAL_STATIC;
    ScreenInfo->hPalette = NULL;

    ScreenInfo->OutputMode = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;


    ScreenInfo->ResizingWindow = 0;
    ScreenInfo->Next = NULL;
    ScreenInfo->Attributes = Fill.Attributes;
    ScreenInfo->PopupAttributes = PopupFill.Attributes;

    ScreenInfo->WheelDelta = 0;

#if defined(FE_SB)
    ScreenInfo->WriteConsoleDbcsLeadByte[0] = 0;
    ScreenInfo->BisectFlag = 0;
    if (Flags & CONSOLE_TEXTMODE_BUFFER) {
        SetLineChar(ScreenInfo);
    }
    ScreenInfo->FillOutDbcsLeadChar = 0;
    ScreenInfo->ConvScreenInfo = NULL;
#endif

    *ScreenInformation = ScreenInfo;
    DBGOUTPUT(("SCREEN at %lx\n", ScreenInfo));
    return STATUS_SUCCESS;
}

VOID
PositionConsoleWindow(
    IN PCONSOLE_INFORMATION Console,
    IN BOOL Initialize
    )
{
    GetWindowRect(Console->hWnd, &Console->WindowRect);

    //
    // If this is an autoposition window being initialized, make sure it's
    // client area doesn't descend below the tray
    //

    if (Initialize && (Console->Flags & CONSOLE_AUTO_POSITION)) {
        RECT ClientRect;
        LONG dx = 0;
        LONG dy = 0;
        HMONITOR hMonitor;
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};

        hMonitor = MonitorFromRect(&Console->WindowRect, MONITOR_DEFAULTTONULL);
        if (hMonitor && GetMonitorInfo(hMonitor, &MonitorInfo)) {
            GetClientRect(Console->hWnd, &ClientRect);
            ClientToScreen(Console->hWnd, (LPPOINT)&ClientRect.left);
            ClientToScreen(Console->hWnd, (LPPOINT)&ClientRect.right);
            if (Console->WindowRect.right > MonitorInfo.rcWork.right) {
                dx = max(min((Console->WindowRect.right - MonitorInfo.rcWork.right),
                             (Console->WindowRect.left - MonitorInfo.rcWork.left)),
                         min((ClientRect.right - MonitorInfo.rcWork.right),
                             (ClientRect.left - MonitorInfo.rcWork.left)));
            }
            if (Console->WindowRect.bottom > MonitorInfo.rcWork.bottom) {
                dy = max(min((Console->WindowRect.bottom - MonitorInfo.rcWork.bottom),
                             (Console->WindowRect.top - MonitorInfo.rcWork.top)),
                         min((ClientRect.bottom - MonitorInfo.rcWork.bottom),
                             (ClientRect.top - MonitorInfo.rcWork.top)));
            }
            if (dx || dy) {
                SetWindowPos(Console->hWnd,
                             NULL,
                             Console->WindowRect.left - dx,
                             Console->WindowRect.top - dy,
                             0,
                             0,
                             SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
            }
        }
    }
}

/*
 * Bug 273518 - joejo
 *
 * This will allow console windows to set foreground correctly on new
 * process' it launches, as opposed it just forcing foreground.
 */
NTSTATUS
ConsoleSetActiveWindow(
    IN PCONSOLE_INFORMATION Console
    )
{
    HWND hWnd = Console->hWnd;
    HANDLE ConsoleHandle = Console->ConsoleHandle;

    UnlockConsole(Console);
    SetActiveWindow(hWnd);
    return RevalidateConsole(ConsoleHandle, &Console);
}

NTSTATUS
CreateWindowsWindow(
    IN PCONSOLE_INFORMATION Console
    )
{
    PSCREEN_INFORMATION ScreenInfo;
    SIZE WindowSize;
    DWORD Style;
    THREAD_BASIC_INFORMATION ThreadInfo;
    HWND hWnd;

    ScreenInfo = Console->ScreenBuffers;

    //
    // figure out how big to make the window, given the desired client area
    // size.  window is always created in textmode.
    //

    ASSERT(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER);
    WindowSize.cx = CONSOLE_WINDOW_SIZE_X(ScreenInfo)*SCR_FONTSIZE(ScreenInfo).X + VerticalClientToWindow;
    WindowSize.cy = CONSOLE_WINDOW_SIZE_Y(ScreenInfo)*SCR_FONTSIZE(ScreenInfo).Y + HorizontalClientToWindow;
    Style = CONSOLE_WINDOW_FLAGS & ~WS_VISIBLE;
    if (!ScreenInfo->WindowMaximizedX) {
        WindowSize.cy += HorizontalScrollSize;
    } else {
        Style &= ~WS_HSCROLL;
    }
    if (!ScreenInfo->WindowMaximizedY) {
        WindowSize.cx += VerticalScrollSize;
    } else {
        Style &= ~WS_VSCROLL;
    }
#ifdef THERESES_DEBUG
    DbgPrint("creating window with char size %d %d\n",CONSOLE_WINDOW_SIZE_X(ScreenInfo),CONSOLE_WINDOW_SIZE_Y(ScreenInfo));
    DbgPrint("                     pixel size %d %d\n",WindowSize.cx,WindowSize.cy);
#endif

    //
    // create the window.
    //

    Console->WindowRect.left = Console->dwWindowOriginX;
    Console->WindowRect.top = Console->dwWindowOriginY;
    Console->WindowRect.right = WindowSize.cx + Console->dwWindowOriginX;
    Console->WindowRect.bottom = WindowSize.cy + Console->dwWindowOriginY;
    hWnd = CreateWindowEx(CONSOLE_WINDOW_EX_FLAGS,
                          CONSOLE_WINDOW_CLASS,
                          Console->Title,
                          Style,
                          Console->dwWindowOriginX,
                          Console->dwWindowOriginY,
                          WindowSize.cx,
                          WindowSize.cy,
                          (Console->Flags & CONSOLE_NO_WINDOW) ? HWND_MESSAGE : HWND_DESKTOP,
                          NULL,
                          ghInstance,
                          NULL);
    if (hWnd == NULL) {
        NtSetEvent(Console->InitEvents[INITIALIZATION_FAILED],NULL);
        return STATUS_NO_MEMORY;
    }
    Console->hWnd = hWnd;

    SetWindowConsole(hWnd, Console);

    //
    // Stuff the client id into the window so USER can find it.
    //

    if (NT_SUCCESS(NtQueryInformationThread(Console->ClientThreadHandle,
            ThreadBasicInformation, &ThreadInfo,
            sizeof(ThreadInfo), NULL))) {

        SetConsolePid(Console->hWnd, HandleToUlong(ThreadInfo.ClientId.UniqueProcess));
        SetConsoleTid(Console->hWnd, HandleToUlong(ThreadInfo.ClientId.UniqueThread));
    }

    //
    // Get the dc.
    //

    Console->hDC = GetDC(Console->hWnd);

    if (Console->hDC == NULL) {
        NtSetEvent(Console->InitEvents[INITIALIZATION_FAILED],NULL);
        DestroyWindow(Console->hWnd);
        Console->hWnd = NULL;
        return STATUS_NO_MEMORY;
    }
    Console->hMenu = GetSystemMenu(Console->hWnd,FALSE);

    //
    // modify system menu to our liking.
    //

    InitSystemMenu(Console);

    gnConsoleWindows++;
    Console->InputThreadInfo->WindowCount++;

#if defined(FE_IME)
    SetUndetermineAttribute(Console);
#endif
#if defined(FE_SB)
    RegisterKeisenOfTTFont(ScreenInfo);
#endif

    //
    // Set up the hot key for this window
    //
    if ((Console->dwHotKey != 0) && !(Console->Flags & CONSOLE_NO_WINDOW)) {
        SendMessage(Console->hWnd, WM_SETHOTKEY, Console->dwHotKey, 0L);
    }

    //
    // create icon
    //

    if (Console->iIconId) {

        // We have no icon, try and get one from progman.

        PostMessage(HWND_BROADCAST,
                    ProgmanHandleMessage,
                    (WPARAM)Console->hWnd,
                    1);
    }
    if (Console->hIcon == NULL) {
        Console->hIcon = ghDefaultIcon;
        Console->hSmIcon = ghDefaultSmIcon;
    } else if (Console->hIcon != ghDefaultIcon) {
        SendMessage(Console->hWnd, WM_SETICON, ICON_BIG, (LPARAM)Console->hIcon);
        SendMessage(Console->hWnd, WM_SETICON, ICON_SMALL, (LPARAM)Console->hSmIcon);
    }

    SetBkMode(Console->hDC,OPAQUE);
    SetFont(ScreenInfo);
    SelectObject(Console->hDC, GetStockObject(DC_BRUSH));
    SetScreenColors(ScreenInfo, ScreenInfo->Attributes,
                    ScreenInfo->PopupAttributes, FALSE);
    if (Console->Flags & CONSOLE_NO_WINDOW) {
        ShowWindowAsync(Console->hWnd, SW_HIDE);
#ifdef i386
    } else if (Console->FullScreenFlags != 0) {
        if (Console->wShowWindow == SW_SHOWMINNOACTIVE) {
            ShowWindowAsync(Console->hWnd, Console->wShowWindow);
            Console->FullScreenFlags = 0;
            Console->Flags |= CONSOLE_IS_ICONIC;
        } else {
            ConvertToFullScreen(Console);
            if (!NT_SUCCESS(ConsoleSetActiveWindow(Console))) {
                return STATUS_INVALID_HANDLE;
            }

            ChangeDispSettings(Console, Console->hWnd,CDS_FULLSCREEN);
        }
#endif
    } else {
        if (Console->wShowWindow != SW_SHOWNOACTIVATE &&
            Console->wShowWindow != SW_SHOWMINNOACTIVE &&
            Console->wShowWindow != SW_HIDE) {
            if (!NT_SUCCESS(ConsoleSetActiveWindow(Console))) {
                return STATUS_INVALID_HANDLE;
            }
        } else if (Console->wShowWindow == SW_SHOWMINNOACTIVE) {
            Console->Flags |= CONSOLE_IS_ICONIC;
        }
        ShowWindowAsync(Console->hWnd, Console->wShowWindow);
    }

    //UpdateWindow(Console->hWnd);
    InternalUpdateScrollBars(ScreenInfo);
    if (!(Console->Flags & CONSOLE_IS_ICONIC) &&
         (Console->FullScreenFlags == 0) ) {

        PositionConsoleWindow(Console, TRUE);
    }

#if defined(FE_IME)
    if (CONSOLE_IS_IME_ENABLED() && !(Console->Flags & CONSOLE_NO_WINDOW)) {
        SetTimer(Console->hWnd, SCROLL_WAIT_TIMER, guCaretBlinkTime, NULL);
    }
#endif
    NtSetEvent(Console->InitEvents[INITIALIZATION_SUCCEEDED],NULL);
    return STATUS_SUCCESS;
}

NTSTATUS
FreeScreenBuffer(
    IN PSCREEN_INFORMATION ScreenInfo
    )

/*++

Routine Description:

    This routine frees the memory associated with a screen buffer.

Arguments:

    ScreenInfo - screen buffer data to free.

Return Value:

Note: console handle table lock must be held when calling this routine

--*/

{
    SHORT i;
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;

    ASSERT(ScreenInfo->RefCount == 0);
    if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
        for (i=0;i<ScreenInfo->ScreenBufferSize.Y;i++) {
            if (ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Length > 1) {
                ConsoleHeapFree(ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Attrs);
            }
        }
        ConsoleHeapFree(ScreenInfo->BufferInfo.TextInfo.TextRows);
        ConsoleHeapFree(ScreenInfo->BufferInfo.TextInfo.Rows);
#if defined(FE_SB)
        DeleteDbcsScreenBuffer(&ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer);
#endif
        RemoveTextBufferFontInfo(ScreenInfo);
    } else {
        if (ScreenInfo->hPalette != NULL) {
            if (GetCurrentObject(Console->hDC, OBJ_PAL) == ScreenInfo->hPalette) {
                SelectPalette(Console->hDC, Console->hSysPalette, FALSE);
            }
            DeleteObject(ScreenInfo->hPalette);
        }
        FreeConsoleBitmap(ScreenInfo);
    }
    ConsoleHeapFree(ScreenInfo);
    return STATUS_SUCCESS;
}

VOID
FindAttrIndex(
    IN PATTR_PAIR String,
    IN SHORT Index,
    OUT PATTR_PAIR *IndexedAttr,
    OUT PSHORT CountOfAttr
    )

/*++

Routine Description:

    This routine finds the nth attribute in a string.

Arguments:

    String - attribute string

    Index - which attribute to find

    IndexedAttr - pointer to attribute within string

    CountOfAttr - on output, contains corrected length of indexed attr.
    for example, if the attribute string was { 5, BLUE } and the requested
    index was 3, CountOfAttr would be 2.

Return Value:

    none.

--*/

{
    SHORT i;

    for (i=0;i<Index;) {
        i += String->Length;
        String++;
    }

    if (i>Index) {
        String--;
        *CountOfAttr = i-Index;
    }
    else {
        *CountOfAttr = String->Length;
    }
    *IndexedAttr = String;
}



NTSTATUS
MergeAttrStrings(
    IN PATTR_PAIR Source,
    IN WORD SourceLength,
    IN PATTR_PAIR Merge,
    IN WORD MergeLength,
    OUT PATTR_PAIR *Target,
    OUT LPWORD TargetLength,
    IN SHORT StartIndex,
    IN SHORT EndIndex,
    IN PROW Row,
    IN PSCREEN_INFORMATION ScreenInfo
    )

/*++

Routine Description:

    This routine merges two run-length encoded attribute strings into
    a third.

    for example, if the source string was { 4, BLUE }, the merge string
    was { 2, RED }, and the StartIndex and EndIndex were 1 and 2,
    respectively, the target string would be { 1, BLUE, 2, RED, 1, BLUE }
    and the target length would be 3.

Arguments:

    Source - pointer to source attribute string

    SourceLength - length of source.  for example, the length of
    { 4, BLUE } is 1.

    Merge - pointer to attribute string to insert into source

    MergeLength - length of merge

    Target - where to store pointer to resulting attribute string

    TargetLength - where to store length of resulting attribute string

    StartIndex - index into Source at which to insert Merge String.

    EndIndex - index into Source at which to stop insertion of Merge String

Return Value:

    none.

--*/
{
    PATTR_PAIR SrcAttr,TargetAttr,SrcEnd;
    PATTR_PAIR NewString;
    SHORT i;
#if THERESES_DEBUG2
#if DBG
    WORD AllocLength;

    AllocLength = MergeLength + SourceLength + 1;
#endif
#endif

    //
    // if just changing the attr for the whole row
    //

    if (MergeLength == 1 && Row->AttrRow.Length == 1) {
        if (Row->AttrRow.Attrs->Attr == Merge->Attr) {
            *TargetLength = 1;
            *Target = &Row->AttrRow.AttrPair;
            return STATUS_SUCCESS;
        }
        if (StartIndex == 0 && EndIndex == (SHORT)(ScreenInfo->ScreenBufferSize.X-1)) {
            NewString = &Row->AttrRow.AttrPair;
            NewString->Attr = Merge->Attr;
            *TargetLength = 1;
            *Target = NewString;
            return STATUS_SUCCESS;
        }
    }

    NewString = (PATTR_PAIR) ConsoleHeapAlloc(MAKE_TAG( SCREEN_TAG ),(SourceLength+MergeLength+1)*sizeof(ATTR_PAIR));
    if (NewString == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    // copy the source string, up to the start index.
    //

    SrcAttr = Source;
    SrcEnd = Source + SourceLength;
    TargetAttr = NewString;
    i=0;
    if (StartIndex != 0) {
        while (i<StartIndex) {
            i += SrcAttr->Length;
            *TargetAttr++ = *SrcAttr++;
        }

        //
        // back up to the last pair copied, in case the attribute in the first
        // pair in the merge string matches.  also, adjust TargetAttr->Length
        // based on i, the attribute
        // counter, back to the StartIndex.  i will be larger than the
        // StartIndex in the case where the last attribute pair copied had
        // a length greater than the number needed to reach StartIndex.
        //

        TargetAttr--;
        if (i>StartIndex) {
            TargetAttr->Length -= i-StartIndex;
        }
        if (Merge->Attr == TargetAttr->Attr) {
            TargetAttr->Length += Merge->Length;
            MergeLength-=1;
            Merge++;
        }
        TargetAttr++;
    }

    //
    // copy the merge string.
    //

    RtlCopyMemory(TargetAttr,Merge,MergeLength*sizeof(ATTR_PAIR));
    TargetAttr += MergeLength;

    //
    // figure out where to resume copying the source string.
    //

    while (i<=EndIndex) {
        ASSERT(SrcAttr != SrcEnd);
        i += SrcAttr->Length;
        SrcAttr++;
    }

    //
    // if not done, copy the rest of the source
    //

    if (SrcAttr != SrcEnd || i!=(SHORT)(EndIndex+1)) {

        //
        // see if we've gone past the right attribute.  if so, back up and
        // copy the attribute and the correct length.
        //

        TargetAttr--;
        if (i>(SHORT)(EndIndex+1)) {
            SrcAttr--;
            if (TargetAttr->Attr == SrcAttr->Attr) {
                TargetAttr->Length += i-(EndIndex+1);
            } else {
                TargetAttr++;
                TargetAttr->Attr = SrcAttr->Attr;
                TargetAttr->Length = (SHORT)(i-(EndIndex+1));
            }
            SrcAttr++;
        }

        //
        // see if we can merge the source and target.
        //

        else if (TargetAttr->Attr == SrcAttr->Attr) {
            TargetAttr->Length += SrcAttr->Length;
            i += SrcAttr->Length;
            SrcAttr++;
        }
        TargetAttr++;

        //
        // copy the rest of the source
        //

        if (SrcAttr < SrcEnd) {
            RtlCopyMemory(TargetAttr,SrcAttr,(SrcEnd-SrcAttr)*sizeof(ATTR_PAIR));
            TargetAttr += SrcEnd - SrcAttr;
        }
    }

    *TargetLength = (WORD)(TargetAttr - NewString);
#if THERESES_DEBUG2
#if DBG
    { SHORT j;
      WORD i;
      PATTR_PAIR NewAttr;
      PULONG Foo;
      j=0;
      NewAttr = NewString;
      for (i=0;i<*TargetLength;i++) {
          j+=NewAttr->Length;
          NewAttr++;
      }
      ASSERT (j == ScreenInfo->ScreenBufferSize.X);
      if (j != ScreenInfo->ScreenBufferSize.X) {
        DbgPrint("new length is %d\n",*TargetLength);
        DbgPrint("address of new attr string is %lx\n",NewString);
      }
      ASSERT (*TargetLength <= AllocLength);
      Foo = (PULONG)(NewString-4);
      ASSERT (*Foo & 1);
      Foo = (PULONG)(NewString-3);
      ASSERT (*Foo >= (*TargetLength * sizeof(ATTR_PAIR)) + 16);
    }
#endif
#endif
    *Target = NewString;
    if (*TargetLength == 1) {
        *Target = &Row->AttrRow.AttrPair;
        **Target = *NewString;
        ConsoleHeapFree(NewString);
    }
    return STATUS_SUCCESS;
}

VOID
ResetTextFlags(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN SHORT StartY,
    IN SHORT EndY
    )

/*
    this routine updates the text flags

#define SINGLE_ATTRIBUTES_PER_LINE 2    // only one attribute per line

*/

{
    SHORT RowIndex;
    PROW Row;
    SHORT i;

    //
    // first see whether we wrote any lines with multiple attributes.  if
    // we did, set the flags and bail out.  also, remember if any of the
    // lines we wrote had attributes different from other lines.
    //

    RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+StartY) % ScreenInfo->ScreenBufferSize.Y;
    for (i=StartY;i<=EndY;i++) {
        Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
        if (Row->AttrRow.Length != 1) {
            ScreenInfo->BufferInfo.TextInfo.Flags &= ~SINGLE_ATTRIBUTES_PER_LINE;
            return;
        }
        if (++RowIndex == ScreenInfo->ScreenBufferSize.Y) {
            RowIndex = 0;
        }
    }

    // all of the written lines have the same attribute.

    if (ScreenInfo->BufferInfo.TextInfo.Flags & SINGLE_ATTRIBUTES_PER_LINE) {
        return;
    }

    if (StartY == 0 && EndY == (ScreenInfo->ScreenBufferSize.Y-1)) {
        ScreenInfo->BufferInfo.TextInfo.Flags |= SINGLE_ATTRIBUTES_PER_LINE;
        return;
    }

    RowIndex = ScreenInfo->BufferInfo.TextInfo.FirstRow;
    for (i=0;i<StartY;i++) {
        Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
        if (Row->AttrRow.Length != 1) {
            return;
        }
        if (++RowIndex == ScreenInfo->ScreenBufferSize.Y) {
            RowIndex = 0;
        }
        Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
    }
    RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+EndY+1) % ScreenInfo->ScreenBufferSize.Y;
    for (i=EndY+1;i<ScreenInfo->ScreenBufferSize.Y;i++) {
        Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
        if (Row->AttrRow.Length != 1) {
            return;
        }
        if (++RowIndex == ScreenInfo->ScreenBufferSize.Y) {
            RowIndex = 0;
        }
    }
    ScreenInfo->BufferInfo.TextInfo.Flags |= SINGLE_ATTRIBUTES_PER_LINE;
}


VOID
ReadRectFromScreenBuffer(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN COORD SourcePoint,
    IN PCHAR_INFO Target,
    IN COORD TargetSize,
    IN PSMALL_RECT TargetRect
    )

/*++

Routine Description:

    This routine copies a rectangular region from the screen buffer.
    no clipping is done.

Arguments:

    ScreenInfo - pointer to screen info

    SourcePoint - upper left coordinates of source rectangle

    Target - pointer to target buffer

    TargetSize - dimensions of target buffer

    TargetRect - rectangle in source buffer to copy

Return Value:

    none.

--*/

{

    PCHAR_INFO TargetPtr;
    SHORT i,j,k;
    SHORT XSize,YSize;
    BOOLEAN WholeTarget;
    SHORT RowIndex;
    PROW Row;
    PWCHAR Char;
    PATTR_PAIR Attr;
    SHORT CountOfAttr;

    DBGOUTPUT(("ReadRectFromScreenBuffer\n"));

    XSize = (SHORT)(TargetRect->Right - TargetRect->Left + 1);
    YSize = (SHORT)(TargetRect->Bottom - TargetRect->Top + 1);

    TargetPtr = Target;
    WholeTarget = FALSE;
    if (XSize == TargetSize.X) {
        ASSERT (TargetRect->Left == 0);
        if (TargetRect->Top != 0) {
            TargetPtr = (PCHAR_INFO)
                ((PBYTE)Target + SCREEN_BUFFER_POINTER(TargetRect->Left,
                                                       TargetRect->Top,
                                                       TargetSize.X,
                                                       sizeof(CHAR_INFO)));
        }
        WholeTarget = TRUE;
    }
    RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+SourcePoint.Y) % ScreenInfo->ScreenBufferSize.Y;
    for (i=0;i<YSize;i++) {
        if (!WholeTarget) {
            TargetPtr = (PCHAR_INFO)
                ((PBYTE)Target + SCREEN_BUFFER_POINTER(TargetRect->Left,
                                                       TargetRect->Top+i,
                                                       TargetSize.X,
                                                       sizeof(CHAR_INFO)));
        }

        //
        // copy the chars and attrs from their respective arrays
        //

        Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
        Char = &Row->CharRow.Chars[SourcePoint.X];
        FindAttrIndex(Row->AttrRow.Attrs,
                      SourcePoint.X,
                      &Attr,
                      &CountOfAttr
                     );
        k=0;
#if defined(FE_SB)
        if (CONSOLE_IS_DBCS_OUTPUTCP(ScreenInfo->Console)) {
            PBYTE AttrP = &Row->CharRow.KAttrs[SourcePoint.X];
            for (j=0;j<XSize;TargetPtr++) {
                BYTE AttrR;
                AttrR = *AttrP++;
                if (j==0 && AttrR & ATTR_TRAILING_BYTE)
                {
                    TargetPtr->Char.UnicodeChar = UNICODE_SPACE;
                    AttrR = 0;
                }
                else if (j+1 >= XSize && AttrR & ATTR_LEADING_BYTE)
                {
                    TargetPtr->Char.UnicodeChar = UNICODE_SPACE;
                    AttrR = 0;
                }
                else
                    TargetPtr->Char.UnicodeChar = *Char;
                Char++;
                TargetPtr->Attributes = Attr->Attr | (WCHAR)(AttrR & ATTR_DBCSSBCS_BYTE) << 8;
                j+=1;
                if (++k==CountOfAttr && j<XSize) {
                    Attr++;
                    k=0;
                    CountOfAttr = Attr->Length;
                }
            }
        }
        else{
#endif
        for (j=0;j<XSize;TargetPtr++) {
            TargetPtr->Char.UnicodeChar = *Char++;
            TargetPtr->Attributes = Attr->Attr;
            j+=1;
            if (++k==CountOfAttr && j<XSize) {
                Attr++;
                k=0;
                CountOfAttr = Attr->Length;
            }
        }
#if defined(FE_SB)
        }
#endif

        if (++RowIndex == ScreenInfo->ScreenBufferSize.Y) {
            RowIndex = 0;
        }
    }
}

VOID
CopyRectangle(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT SourceRect,
    IN COORD TargetPoint
    )

/*++

Routine Description:

    This routine copies a rectangular region from the screen buffer to
    the screen buffer.  no clipping is done.

Arguments:

    ScreenInfo - pointer to screen info

    SourceRect - rectangle in source buffer to copy

    TargetPoint - upper left coordinates of new location rectangle

Return Value:

    none.

--*/

{
    SMALL_RECT Target;
    COORD SourcePoint;
    COORD Size;
    DBGOUTPUT(("CopyRectangle\n"));


    LockScrollBuffer();

    SourcePoint.X = SourceRect->Left;
    SourcePoint.Y = SourceRect->Top;
    Target.Left = 0;
    Target.Top = 0;
    Target.Right = Size.X = SourceRect->Right - SourceRect->Left;
    Target.Bottom = Size.Y = SourceRect->Bottom - SourceRect->Top;
    Size.X++;
    Size.Y++;

    if (ScrollBufferSize < (Size.X * Size.Y * sizeof(CHAR_INFO))) {
        FreeScrollBuffer();
        if (!NT_SUCCESS(AllocateScrollBuffer(Size.X * Size.Y * sizeof(CHAR_INFO)))) {
            UnlockScrollBuffer();
            return;
        }
    }

    ReadRectFromScreenBuffer(ScreenInfo,
                             SourcePoint,
                             ScrollBuffer,
                             Size,
                             &Target
                            );

    WriteRectToScreenBuffer((PBYTE)ScrollBuffer,
                            Size,
                            &Target,
                            ScreenInfo,
                            TargetPoint,
                            0xFFFFFFFF  // ScrollBuffer won't need conversion
                           );
    UnlockScrollBuffer();
}


NTSTATUS
ReadScreenBuffer(
    IN PSCREEN_INFORMATION ScreenInformation,
    OUT PCHAR_INFO Buffer,
    IN OUT PSMALL_RECT ReadRegion
    )

/*++

Routine Description:

    This routine reads a rectangular region from the screen buffer.
    The region is first clipped.

Arguments:

    ScreenInformation - Screen buffer to read from.

    Buffer - Buffer to read into.

    ReadRegion - Region to read.

Return Value:


--*/

{
    COORD TargetSize;
    COORD TargetPoint,SourcePoint;
    SMALL_RECT Target;

    DBGOUTPUT(("ReadScreenBuffer\n"));
    //
    // calculate dimensions of caller's buffer.  have to do this calculation
    // before clipping.
    //

    TargetSize.X = (SHORT)(ReadRegion->Right - ReadRegion->Left + 1);
    TargetSize.Y = (SHORT)(ReadRegion->Bottom - ReadRegion->Top + 1);

    if (TargetSize.X <= 0 || TargetSize.Y <= 0) {
        return STATUS_SUCCESS;
    }

    // do clipping.

    if (ReadRegion->Right > (SHORT)(ScreenInformation->ScreenBufferSize.X-1)) {
        ReadRegion->Right = (SHORT)(ScreenInformation->ScreenBufferSize.X-1);
    }
    if (ReadRegion->Bottom > (SHORT)(ScreenInformation->ScreenBufferSize.Y-1)) {
        ReadRegion->Bottom = (SHORT)(ScreenInformation->ScreenBufferSize.Y-1);
    }
    if (ReadRegion->Left < 0) {
        TargetPoint.X = -ReadRegion->Left;
        ReadRegion->Left = 0;
    }
    else {
        TargetPoint.X = 0;
    }
    if (ReadRegion->Top < 0) {
        TargetPoint.Y = -ReadRegion->Top;
        ReadRegion->Top = 0;
    }
    else {
        TargetPoint.Y = 0;
    }

    SourcePoint.X = ReadRegion->Left;
    SourcePoint.Y = ReadRegion->Top;
    Target.Left = TargetPoint.X;
    Target.Top = TargetPoint.Y;
    Target.Right = TargetPoint.X + (ReadRegion->Right - ReadRegion->Left);
    Target.Bottom = TargetPoint.Y + (ReadRegion->Bottom - ReadRegion->Top);
    ReadRectFromScreenBuffer(ScreenInformation,
                             SourcePoint,
                             Buffer,
                             TargetSize,
                             &Target
                            );
    return STATUS_SUCCESS;
}

NTSTATUS
WriteScreenBuffer(
    IN PSCREEN_INFORMATION ScreenInformation,
    IN PCHAR_INFO Buffer,
    IN OUT PSMALL_RECT WriteRegion
    )

/*++

Routine Description:

    This routine write a rectangular region to the screen buffer.
    The region is first clipped.

    The region should contain Unicode or UnicodeOem chars.

Arguments:

    ScreenInformation - Screen buffer to write to.

    Buffer - Buffer to write from.

    ReadRegion - Region to write.

Return Value:


--*/

{
    COORD SourceSize;
    COORD TargetPoint;
    SMALL_RECT SourceRect;

    DBGOUTPUT(("WriteScreenBuffer\n"));
    //
    // calculate dimensions of caller's buffer.  have to do this calculation
    // before clipping.
    //

    SourceSize.X = (SHORT)(WriteRegion->Right - WriteRegion->Left + 1);
    SourceSize.Y = (SHORT)(WriteRegion->Bottom - WriteRegion->Top + 1);
    if (SourceSize.X <= 0 || SourceSize.Y <= 0) {
        return STATUS_SUCCESS;
    }

    // do clipping.

    if (WriteRegion->Left >= ScreenInformation->ScreenBufferSize.X ||
        WriteRegion->Top  >= ScreenInformation->ScreenBufferSize.Y) {
        return STATUS_SUCCESS;
    }

    if (WriteRegion->Right > (SHORT)(ScreenInformation->ScreenBufferSize.X-1))
        WriteRegion->Right = (SHORT)(ScreenInformation->ScreenBufferSize.X-1);
    SourceRect.Right = WriteRegion->Right - WriteRegion->Left;
    if (WriteRegion->Bottom > (SHORT)(ScreenInformation->ScreenBufferSize.Y-1))
        WriteRegion->Bottom = (SHORT)(ScreenInformation->ScreenBufferSize.Y-1);
    SourceRect.Bottom = WriteRegion->Bottom - WriteRegion->Top;
    if (WriteRegion->Left < 0) {
        SourceRect.Left = -WriteRegion->Left;
        WriteRegion->Left = 0;
    }
    else {
        SourceRect.Left = 0;
    }
    if (WriteRegion->Top < 0) {
        SourceRect.Top = -WriteRegion->Top;
        WriteRegion->Top = 0;
    }
    else {
        SourceRect.Top = 0;
    }

    TargetPoint.X = WriteRegion->Left;
    TargetPoint.Y = WriteRegion->Top;
    WriteRectToScreenBuffer((PBYTE)Buffer,
                            SourceSize,
                            &SourceRect,
                            ScreenInformation,
                            TargetPoint,
                            0xFFFFFFFF
                           );
    return STATUS_SUCCESS;
}




NTSTATUS
ReadOutputString(
    IN PSCREEN_INFORMATION ScreenInfo,
    OUT PVOID Buffer,
    IN COORD ReadCoord,
    IN ULONG StringType,
    IN OUT PULONG NumRecords // this value is valid even for error cases
    )

/*++

Routine Description:

    This routine reads a string of characters or attributes from the
    screen buffer.

Arguments:

    ScreenInfo - Pointer to screen buffer information.

    Buffer - Buffer to read into.

    ReadCoord - Screen buffer coordinate to begin reading from.

    StringType

        CONSOLE_ASCII         - read a string of ASCII characters.
        CONSOLE_REAL_UNICODE  - read a string of Real Unicode characters.
        CONSOLE_FALSE_UNICODE - read a string of False Unicode characters.
        CONSOLE_ATTRIBUTE     - read a string of attributes.

    NumRecords - On input, the size of the buffer in elements.  On output,
    the number of elements read.

Return Value:


--*/

{
    ULONG NumRead;
    SHORT X,Y;
    SHORT RowIndex;
    SHORT CountOfAttr;
    PATTR_PAIR Attr;
    PROW Row;
    PWCHAR Char;
    SHORT j,k;
    PWCHAR TransBuffer = NULL;
    PWCHAR BufPtr;
#if defined(FE_SB)
    PBYTE AttrP;
    PBYTE TransBufferA,BufPtrA;
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;
#endif

    DBGOUTPUT(("ReadOutputString\n"));
    if (*NumRecords == 0)
        return STATUS_SUCCESS;
    NumRead = 0;
    X=ReadCoord.X;
    Y=ReadCoord.Y;
    if (X>=ScreenInfo->ScreenBufferSize.X ||
        X<0 ||
        Y>=ScreenInfo->ScreenBufferSize.Y ||
        Y<0) {
        *NumRecords = 0;
        return STATUS_SUCCESS;
    }

    RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+ReadCoord.Y) % ScreenInfo->ScreenBufferSize.Y;

    if (StringType == CONSOLE_ASCII) {
        TransBuffer = (PWCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),*NumRecords * sizeof(WCHAR));
        if (TransBuffer == NULL) {
            return STATUS_NO_MEMORY;
        }
        BufPtr = TransBuffer;
    } else {
        BufPtr = Buffer;
    }

#if defined(FE_SB)
    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
    {
        TransBufferA = (PBYTE)ConsoleHeapAlloc(MAKE_TAG( TMP_DBCS_TAG ),*NumRecords * sizeof(BYTE));
        if (TransBufferA == NULL) {
            if (TransBuffer != NULL)
                ConsoleHeapFree(TransBuffer);
            return STATUS_NO_MEMORY;
        }
        BufPtrA = TransBufferA;
    }
#endif
    if ((StringType == CONSOLE_ASCII) ||
            (StringType == CONSOLE_REAL_UNICODE) ||
            (StringType == CONSOLE_FALSE_UNICODE)) {
        while (NumRead < *NumRecords) {

            //
            // copy the chars from its array
            //

            Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
            Char = &Row->CharRow.Chars[X];
#if defined(FE_SB)
            if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
                AttrP = &Row->CharRow.KAttrs[X];
#endif
            if ((ULONG)(ScreenInfo->ScreenBufferSize.X - X) > (*NumRecords - NumRead)) {
                RtlCopyMemory(BufPtr,Char,(*NumRecords - NumRead) * sizeof(WCHAR));
#if defined(FE_SB)
                if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
                    RtlCopyMemory(BufPtrA,AttrP,(*NumRecords - NumRead) * sizeof(CHAR));
#endif
                NumRead += *NumRecords - NumRead;
                break;
            }
            RtlCopyMemory(BufPtr,Char,(ScreenInfo->ScreenBufferSize.X - X) * sizeof(WCHAR));
            BufPtr = (PVOID)((PBYTE)BufPtr + ((ScreenInfo->ScreenBufferSize.X - X) * sizeof(WCHAR)));
#if defined(FE_SB)
            if (CONSOLE_IS_DBCS_OUTPUTCP(Console)) {
                RtlCopyMemory(BufPtrA,AttrP,(ScreenInfo->ScreenBufferSize.X - X) * sizeof(CHAR));
                BufPtrA = (PVOID)((PBYTE)BufPtrA + ((ScreenInfo->ScreenBufferSize.X - X) * sizeof(CHAR)));
            }
#endif
            NumRead += ScreenInfo->ScreenBufferSize.X - X;
            if (++RowIndex == ScreenInfo->ScreenBufferSize.Y) {
                RowIndex = 0;
            }
            X = 0;
            Y++;
            if (Y>=ScreenInfo->ScreenBufferSize.Y) {
                break;
            }
        }
#if defined(FE_SB)
        if (CONSOLE_IS_DBCS_OUTPUTCP(Console) && (NumRead)) {
            if (StringType == CONSOLE_ASCII) {
                Char = BufPtr = TransBuffer;
            } else {
                Char = BufPtr = Buffer;
            }
            AttrP = BufPtrA = TransBufferA;

            if (*BufPtrA & ATTR_TRAILING_BYTE)
            {
                j = k = (SHORT)(NumRead - 1);
                BufPtr++;
                *Char++ = UNICODE_SPACE;
                BufPtrA++;
                NumRead = 1;
            }
            else {
                j = k = (SHORT)NumRead;
                NumRead = 0;
            }
            while (j--) {
                if (!(*BufPtrA & ATTR_TRAILING_BYTE)) {
                    *Char++ = *BufPtr;
                    NumRead++;
                }
                BufPtr++;
                BufPtrA++;
            }
            if (k && *(BufPtrA-1) & ATTR_LEADING_BYTE)
            {
                *(Char-1) = UNICODE_SPACE;
            }
        }
#endif
    } else if (StringType == CONSOLE_ATTRIBUTE) {
        PWORD TargetPtr=BufPtr;
        while (NumRead < *NumRecords) {

            //
            // copy the attrs from its array
            //

            Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
#if defined(FE_SB)
            if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
                AttrP = &Row->CharRow.KAttrs[X];
#endif
            FindAttrIndex(Row->AttrRow.Attrs,
                          X,
                          &Attr,
                          &CountOfAttr
                         );
            k=0;
            for (j=X;j<ScreenInfo->ScreenBufferSize.X;TargetPtr++) {
#if defined(FE_SB)
                if (!CONSOLE_IS_DBCS_OUTPUTCP(Console) )
                    *TargetPtr = Attr->Attr;
                else if ((j == X) && (*AttrP & ATTR_TRAILING_BYTE))
                    *TargetPtr = Attr->Attr;
                else if (*AttrP & ATTR_LEADING_BYTE){
                    if ((NumRead == *NumRecords-1)||(j == ScreenInfo->ScreenBufferSize.X-1))
                        *TargetPtr = Attr->Attr;
                    else
                        *TargetPtr = Attr->Attr | (WCHAR)(*AttrP & ATTR_DBCSSBCS_BYTE) << 8;
                }
                else
                    *TargetPtr = Attr->Attr | (WCHAR)(*AttrP & ATTR_DBCSSBCS_BYTE) << 8;
#else
                *TargetPtr = Attr->Attr;
#endif
                NumRead++;
                j+=1;
                if (++k==CountOfAttr && j<ScreenInfo->ScreenBufferSize.X) {
                    Attr++;
                    k=0;
                    CountOfAttr = Attr->Length;
                }
                if (NumRead == *NumRecords) {
#if defined(FE_SB)
                    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
                        ConsoleHeapFree(TransBufferA);
#endif
                    return STATUS_SUCCESS;
                }
#if defined(FE_SB)
                if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
                    AttrP++;
#endif
            }
            if (++RowIndex == ScreenInfo->ScreenBufferSize.Y) {
                RowIndex = 0;
            }
            X = 0;
            Y++;
            if (Y>=ScreenInfo->ScreenBufferSize.Y) {
                break;
            }
        }
    } else {
        *NumRecords = 0;
#if defined(FE_SB)
        if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
            ConsoleHeapFree(TransBufferA);
#endif
        return STATUS_INVALID_PARAMETER;
    }

    if (StringType == CONSOLE_ASCII) {
        UINT Codepage;
#if defined(FE_SB)
        if ((ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                !(ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
            if (ScreenInfo->Console->OutputCP != WINDOWSCP)
                Codepage = USACP;
            else
                Codepage = WINDOWSCP;
        } else {
            Codepage = ScreenInfo->Console->OutputCP;
        }
#else
        if ((ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                !(ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
            Codepage = WINDOWSCP;
        } else {
            Codepage = ScreenInfo->Console->OutputCP;
        }
#endif
#if defined(FE_SB)
        if ((NumRead == 1) && !CONSOLE_IS_DBCS_OUTPUTCP(Console))
#else
        if (NumRead == 1)
#endif
        {
            *((PBYTE)Buffer) = WcharToChar(Codepage, *TransBuffer);
        } else {
            NumRead = ConvertOutputToOem(Codepage, TransBuffer, NumRead, Buffer, *NumRecords);
        }
        ConsoleHeapFree(TransBuffer);
    } else if (StringType == CONSOLE_REAL_UNICODE &&
            (ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
            !(ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
        /*
         * Buffer contains false Unicode (UnicodeOem) only in Windowed
         * RasterFont mode, so in this case, convert it to real Unicode.
         */
        FalseUnicodeToRealUnicode(Buffer,
                                NumRead,
                                ScreenInfo->Console->OutputCP
                                );
    }

#if defined(FE_SB)
    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
        ConsoleHeapFree(TransBufferA);
#endif
    *NumRecords = NumRead;
    return STATUS_SUCCESS;
}



NTSTATUS
GetScreenBufferInformation(
    IN PSCREEN_INFORMATION ScreenInfo,
    OUT PCOORD Size,
    OUT PCOORD CursorPosition,
    OUT PCOORD ScrollPosition,
    OUT PWORD  Attributes,
    OUT PCOORD CurrentWindowSize,
    OUT PCOORD MaximumWindowSize
    )

/*++

Routine Description:

    This routine returns data about the screen buffer.

Arguments:

    ScreenInfo - Pointer to screen buffer information.

    Size - Pointer to location in which to store screen buffer size.

    CursorPosition - Pointer to location in which to store the cursor position.

    ScrollPosition - Pointer to location in which to store the scroll position.

    Attributes - Pointer to location in which to store the default attributes.

    CurrentWindowSize - Pointer to location in which to store current window size.

    MaximumWindowSize - Pointer to location in which to store maximum window size.

Return Value:

--*/

{
    WINDOW_LIMITS WindowLimits;

    *Size = ScreenInfo->ScreenBufferSize;
    *CursorPosition = ScreenInfo->BufferInfo.TextInfo.CursorPosition;
    ScrollPosition->X = ScreenInfo->Window.Left;
    ScrollPosition->Y = ScreenInfo->Window.Top;
    *Attributes = ScreenInfo->Attributes;
    CurrentWindowSize->X = (SHORT)CONSOLE_WINDOW_SIZE_X(ScreenInfo);
    CurrentWindowSize->Y = (SHORT)CONSOLE_WINDOW_SIZE_Y(ScreenInfo);
    if (ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {
        MaximumWindowSize->X = min(80,ScreenInfo->ScreenBufferSize.X);
#if defined(FE_SB)
        if (CONSOLE_IS_DBCS_OUTPUTCP(ScreenInfo->Console))
        {
            MaximumWindowSize->Y = min(25,ScreenInfo->ScreenBufferSize.Y);
        }
        else
        {
            MaximumWindowSize->Y = min(50,ScreenInfo->ScreenBufferSize.Y);
        }
#else
        MaximumWindowSize->Y = min(50,ScreenInfo->ScreenBufferSize.Y);
#endif
    } else {
        GetWindowLimits(ScreenInfo, &WindowLimits);
        *MaximumWindowSize = WindowLimits.MaximumWindowSize;
    }
    return STATUS_SUCCESS;
}


VOID
UpdateScrollBars(
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
    if (!ACTIVE_SCREEN_BUFFER(ScreenInfo)) {
        return;
    }

    if (ScreenInfo->Console->Flags & CONSOLE_UPDATING_SCROLL_BARS)
        return;
    ScreenInfo->Console->Flags |= CONSOLE_UPDATING_SCROLL_BARS;
    PostMessage(ScreenInfo->Console->hWnd,
                 CM_UPDATE_SCROLL_BARS,
                 (WPARAM)ScreenInfo,
                 0
                );
}

VOID
InternalUpdateScrollBars(
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
    SCROLLINFO si;

    ScreenInfo->Console->Flags &= ~CONSOLE_UPDATING_SCROLL_BARS;
    if (!ACTIVE_SCREEN_BUFFER(ScreenInfo)) {
        return;
    }

    ScreenInfo->ResizingWindow++;

    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    si.nPage = CONSOLE_WINDOW_SIZE_Y(ScreenInfo);
    si.nMin = 0;
    si.nMax = ScreenInfo->ScreenBufferSize.Y - 1;
    si.nPos = ScreenInfo->Window.Top;
    SetScrollInfo(ScreenInfo->Console->hWnd, SB_VERT, &si, TRUE);

    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    si.nPage = CONSOLE_WINDOW_SIZE_X(ScreenInfo);
    si.nMin = 0;
    si.nMax = ScreenInfo->ScreenBufferSize.X - 1;
    si.nPos = ScreenInfo->Window.Left;
    SetScrollInfo(ScreenInfo->Console->hWnd, SB_HORZ, &si, TRUE);

    ScreenInfo->ResizingWindow--;
}

VOID
ScreenBufferSizeChange(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN COORD NewSize
    )
{
    INPUT_RECORD InputEvent;

    InputEvent.EventType = WINDOW_BUFFER_SIZE_EVENT;
    InputEvent.Event.WindowBufferSizeEvent.dwSize = NewSize;
    WriteInputBuffer(ScreenInfo->Console,
                     &ScreenInfo->Console->InputBuffer,
                     &InputEvent,
                     1
                     );
}

NTSTATUS
ResizeScreenBuffer(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN COORD NewScreenSize,
    IN BOOL DoScrollBarUpdate
    )

/*++

Routine Description:

    This routine resizes the screen buffer.

Arguments:

    ScreenInfo - pointer to screen buffer info.

    NewScreenSize - new size of screen.

Return Value:

--*/

{
    SHORT i,j;
    BOOLEAN WindowMaximizedX,WindowMaximizedY;
    SHORT LimitX,LimitY;
    PWCHAR TextRows,TextRowPtr;
    BOOL UpdateWindow;
    SHORT TopRow,TopRowIndex; // new top row of screen buffer
    COORD CursorPosition;
#if defined(FE_SB)
    DBCS_SCREEN_BUFFER NewDbcsScreenBuffer;
    PBYTE TextRowPtrA;
#endif

    //
    // Don't allow resize of graphics apps
    //

    if (!(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER)) {
        return STATUS_UNSUCCESSFUL;
    }

    TextRows = (PWCHAR)ConsoleHeapAlloc(MAKE_TAG( SCREEN_TAG ),NewScreenSize.X*NewScreenSize.Y*sizeof(WCHAR));
    if (TextRows == NULL) {
        return STATUS_NO_MEMORY;
    }
#if defined(FE_SB)
    if (! CreateDbcsScreenBuffer(ScreenInfo->Console,NewScreenSize,&NewDbcsScreenBuffer))
    {
        ConsoleHeapFree(TextRows);
        return STATUS_NO_MEMORY;
    }
#endif
    LimitX = (NewScreenSize.X < ScreenInfo->ScreenBufferSize.X) ?
              NewScreenSize.X : ScreenInfo->ScreenBufferSize.X;
    LimitY = (NewScreenSize.Y < ScreenInfo->ScreenBufferSize.Y) ?
              NewScreenSize.Y : ScreenInfo->ScreenBufferSize.Y;
    TopRow = 0;
    if (NewScreenSize.Y <= ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y) {
        TopRow += ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y - NewScreenSize.Y + 1;
    }
    TopRowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+TopRow) % ScreenInfo->ScreenBufferSize.Y;
    if (NewScreenSize.Y != ScreenInfo->ScreenBufferSize.Y) {
        PROW Temp;
        SHORT NumToCopy,NumToCopy2;

        //
        // resize ROWs array.  first alloc a new ROWs array. then copy the old
        // one over, resetting the FirstRow.
        //
        //

        Temp = (PROW)ConsoleHeapAlloc(MAKE_TAG( SCREEN_TAG ),NewScreenSize.Y*sizeof(ROW));
        if (Temp == NULL) {
            ConsoleHeapFree(TextRows);
#if defined(FE_SB)
            DeleteDbcsScreenBuffer(&NewDbcsScreenBuffer);
#endif
            return STATUS_NO_MEMORY;
        }
        NumToCopy = ScreenInfo->ScreenBufferSize.Y-TopRowIndex;
        if (NumToCopy > NewScreenSize.Y)
            NumToCopy = NewScreenSize.Y;
        RtlCopyMemory(Temp,&ScreenInfo->BufferInfo.TextInfo.Rows[TopRowIndex],NumToCopy*sizeof(ROW));
        if (TopRowIndex!=0 && NumToCopy != NewScreenSize.Y) {
            NumToCopy2 = TopRowIndex;
            if (NumToCopy2 > (NewScreenSize.Y-NumToCopy))
                NumToCopy2 = NewScreenSize.Y-NumToCopy;
            RtlCopyMemory(&Temp[NumToCopy],
                   ScreenInfo->BufferInfo.TextInfo.Rows,
                   NumToCopy2*sizeof(ROW)
                  );
        }
        for (i=0;i<LimitY;i++) {
            if (Temp[i].AttrRow.Length == 1) {
                Temp[i].AttrRow.Attrs = &Temp[i].AttrRow.AttrPair;
            }
        }

        //
        // if the new screen buffer has fewer rows than the existing one,
        // free the extra rows.  if the new screen buffer has more rows
        // than the existing one, allocate new rows.
        //

        if (NewScreenSize.Y < ScreenInfo->ScreenBufferSize.Y) {
            i = (TopRowIndex+NewScreenSize.Y) % ScreenInfo->ScreenBufferSize.Y;
            for (j=NewScreenSize.Y;j<ScreenInfo->ScreenBufferSize.Y;j++) {
                if (ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Length > 1) {
                    ConsoleHeapFree(ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Attrs);
                }
                if (++i == ScreenInfo->ScreenBufferSize.Y) {
                    i = 0;
                }
            }
        } else if (NewScreenSize.Y > ScreenInfo->ScreenBufferSize.Y) {
            for (i=ScreenInfo->ScreenBufferSize.Y;i<NewScreenSize.Y;i++) {
                Temp[i].AttrRow.Length = 1;
                Temp[i].AttrRow.AttrPair.Length = NewScreenSize.X;
                Temp[i].AttrRow.AttrPair.Attr = ScreenInfo->Attributes;
                Temp[i].AttrRow.Attrs = &Temp[i].AttrRow.AttrPair;
            }
        }
        ScreenInfo->BufferInfo.TextInfo.FirstRow = 0;
        ConsoleHeapFree(ScreenInfo->BufferInfo.TextInfo.Rows);
        ScreenInfo->BufferInfo.TextInfo.Rows = Temp;
    }

    //
    // Realloc each row.  any horizontal growth results in the last
    // attribute in a row getting extended.
    //
#if defined(FE_SB)
    TextRowPtrA=NewDbcsScreenBuffer.KAttrRows;
#endif
    for (i=0,TextRowPtr=TextRows;i<LimitY;i++,TextRowPtr+=NewScreenSize.X)
    {
        RtlCopyMemory(TextRowPtr,
               ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.Chars,
               LimitX*sizeof(WCHAR));
#if defined(FE_SB)
        if (TextRowPtrA) {
            RtlCopyMemory(TextRowPtrA,
                          ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.KAttrs,
                          LimitX*sizeof(CHAR));
        }
#endif
        for (j=ScreenInfo->ScreenBufferSize.X;j<NewScreenSize.X;j++) {
            TextRowPtr[j] = (WCHAR)' ';
        }

        if (ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.Right > NewScreenSize.X) {
            ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.OldRight = INVALID_OLD_LENGTH;
            ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.Right = NewScreenSize.X;
        }
        ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.Chars = TextRowPtr;
#if defined(FE_SB)
        ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.KAttrs = TextRowPtrA;
        if (TextRowPtrA) {
            if (NewScreenSize.X > ScreenInfo->ScreenBufferSize.X)
                RtlZeroMemory(TextRowPtrA+ScreenInfo->ScreenBufferSize.X,
                              NewScreenSize.X-ScreenInfo->ScreenBufferSize.X);
            TextRowPtrA+=NewScreenSize.X;
        }
#endif
    }
    for (;i<NewScreenSize.Y;i++,TextRowPtr+=NewScreenSize.X)
    {
        for (j=0;j<NewScreenSize.X;j++) {
            TextRowPtr[j] = (WCHAR)' ';
        }
#if defined(FE_SB)
        if (TextRowPtrA) {
           RtlZeroMemory(TextRowPtrA, NewScreenSize.X);
        }
#endif
        ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.Chars = TextRowPtr;
        ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.OldLeft = INVALID_OLD_LENGTH;
        ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.OldRight = INVALID_OLD_LENGTH;
        ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.Left = NewScreenSize.X;
        ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.Right = 0;
#if defined(FE_SB)
        ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.KAttrs = TextRowPtrA;
        if (TextRowPtrA) {
            TextRowPtrA+=NewScreenSize.X;
        }
#endif
    }
    ConsoleHeapFree(ScreenInfo->BufferInfo.TextInfo.TextRows);
    ScreenInfo->BufferInfo.TextInfo.TextRows = TextRows;
#if defined(FE_SB)
    DeleteDbcsScreenBuffer(&ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer);
    ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer = NewDbcsScreenBuffer;
#endif

    if (NewScreenSize.X != ScreenInfo->ScreenBufferSize.X) {
        for (i=0;i<LimitY;i++) {
            PATTR_PAIR IndexedAttr;
            SHORT CountOfAttr;

            if (NewScreenSize.X > ScreenInfo->ScreenBufferSize.X) {
                FindAttrIndex(ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Attrs,
                              (SHORT)(ScreenInfo->ScreenBufferSize.X-1),
                              &IndexedAttr,
                              &CountOfAttr
                             );
  ASSERT (IndexedAttr <=
    &ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Attrs[ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Length-1]);
                IndexedAttr->Length += NewScreenSize.X - ScreenInfo->ScreenBufferSize.X;
            }
            else {

                FindAttrIndex(ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Attrs,
                              (SHORT)(NewScreenSize.X-1),
                              &IndexedAttr,
                              &CountOfAttr
                             );
                IndexedAttr->Length -= CountOfAttr-1;
                if (ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Length != 1)  {
                    ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Length = (SHORT)(IndexedAttr - ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Attrs + 1);
                    if (ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Length != 1) {
                        ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Attrs = (PATTR_PAIR)ConsoleHeapReAlloc(MAKE_TAG( SCREEN_TAG ),ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Attrs,
                                                                         ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Length * sizeof(ATTR_PAIR));
                    }
                    else {
                        ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.AttrPair = *IndexedAttr;
                        ConsoleHeapFree(ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Attrs);
                        ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.Attrs = &ScreenInfo->BufferInfo.TextInfo.Rows[i].AttrRow.AttrPair;
                    }
                }
            }
        }
    }

    //
    // if the screen buffer is resized smaller than the saved
    // window size, shrink the saved window size.
    //
#ifdef i386
    if (ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN) {
        if (NewScreenSize.X < ScreenInfo->BufferInfo.TextInfo.WindowedWindowSize.X) {
            ScreenInfo->BufferInfo.TextInfo.WindowedWindowSize.X = NewScreenSize.X;
        }
        if (NewScreenSize.Y < ScreenInfo->BufferInfo.TextInfo.WindowedWindowSize.Y) {
            ScreenInfo->BufferInfo.TextInfo.WindowedWindowSize.Y = NewScreenSize.Y;
        }
        ScreenInfo->BufferInfo.TextInfo.WindowedScreenSize = NewScreenSize;
    }
#endif

    UpdateWindow = FALSE;

    //
    // if the screen buffer shrunk beyond the boundaries of the window,
    // adjust the window origin.
    //

    if (NewScreenSize.X > CONSOLE_WINDOW_SIZE_X(ScreenInfo)) {
        if (ScreenInfo->Window.Right >= NewScreenSize.X) {
            ScreenInfo->Window.Left -= ScreenInfo->Window.Right - NewScreenSize.X + 1;
            ScreenInfo->Window.Right -= ScreenInfo->Window.Right - NewScreenSize.X + 1;
            UpdateWindow = TRUE;
        }
    } else {
        ScreenInfo->Window.Left = 0;
        ScreenInfo->Window.Right = NewScreenSize.X - 1;
        UpdateWindow = TRUE;
    }
    if (NewScreenSize.Y > CONSOLE_WINDOW_SIZE_Y(ScreenInfo)) {
        if (ScreenInfo->Window.Bottom >= NewScreenSize.Y) {
            ScreenInfo->Window.Top -= ScreenInfo->Window.Bottom - NewScreenSize.Y + 1;
            ScreenInfo->Window.Bottom -= ScreenInfo->Window.Bottom - NewScreenSize.Y + 1;
            UpdateWindow = TRUE;
        }
    } else {
        ScreenInfo->Window.Top = 0;
        ScreenInfo->Window.Bottom = NewScreenSize.Y - 1;
        UpdateWindow = TRUE;
    }

#if defined(FE_SB)
    // Should be sets ScreenBufferSize before calls SetCursorPosition
    // because SetCursorPosition refer ScreenBufferSize.
    // Also, FE version refer in InvertPixels.
    //
    // kkntbug:11311
    ScreenInfo->ScreenBufferSize = NewScreenSize;
#endif

    //
    // adjust cursor position if it's no longer with screen buffer
    //

    CursorPosition=ScreenInfo->BufferInfo.TextInfo.CursorPosition;
    if (CursorPosition.X >= NewScreenSize.X) {
        if (ScreenInfo->OutputMode & ENABLE_WRAP_AT_EOL_OUTPUT) {
            CursorPosition.X = 0;
            CursorPosition.Y += 1;
        } else {
            CursorPosition.X = NewScreenSize.X-1;
        }
    }
    if (CursorPosition.Y >= NewScreenSize.Y) {
        CursorPosition.Y = NewScreenSize.Y-1;
    }
#if defined(FE_SB)
    // set cursor position Y is ZERO when expand screen buffer with IME open mode
    // from screen buffer is one line mode.
    // Because, One line screen buffer mode and IME open mode is set -1 as cursor position Y.
    if (ScreenInfo->Console->InputBuffer.ImeMode.Open && CursorPosition.Y < 0) {
        CursorPosition.Y = 0;
    }
#endif
    if (CursorPosition.X != ScreenInfo->BufferInfo.TextInfo.CursorPosition.X ||
        CursorPosition.Y != ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y) {
        SetCursorPosition(ScreenInfo,
                          CursorPosition,
                          FALSE
                          );
    }

    ASSERT (ScreenInfo->Window.Left >= 0);
    ASSERT (ScreenInfo->Window.Right < NewScreenSize.X);
    ASSERT (ScreenInfo->Window.Top >= 0);
    ASSERT (ScreenInfo->Window.Bottom < NewScreenSize.Y);

    ScreenInfo->ScreenBufferSize = NewScreenSize;
    ResetTextFlags(ScreenInfo,0,(SHORT)(ScreenInfo->ScreenBufferSize.Y-1));
    WindowMaximizedX = (CONSOLE_WINDOW_SIZE_X(ScreenInfo) ==
                          ScreenInfo->ScreenBufferSize.X);
    WindowMaximizedY = (CONSOLE_WINDOW_SIZE_Y(ScreenInfo) ==
                          ScreenInfo->ScreenBufferSize.Y);

#if defined(FE_IME)
    if (CONSOLE_IS_IME_ENABLED()) {
        if (!NT_SUCCESS(ConsoleImeMessagePump(ScreenInfo->Console,
                              CONIME_NOTIFY_SCREENBUFFERSIZE,
                              (WPARAM)ScreenInfo->Console->ConsoleHandle,
                              (LPARAM)MAKELPARAM(NewScreenSize.X, NewScreenSize.Y)
                             ))) {
            return STATUS_INVALID_HANDLE;
        }
    }

    if ( (! ScreenInfo->ConvScreenInfo) &&
         (CONSOLE_IS_DBCS_OUTPUTCP(ScreenInfo->Console)))
    {
        if (!NT_SUCCESS(ConsoleImeResizeModeSystemScreenBuffer(ScreenInfo->Console,NewScreenSize)) ||
                !NT_SUCCESS(ConsoleImeResizeCompStrScreenBuffer(ScreenInfo->Console,NewScreenSize))) {
            /*
             * If something went wrong, just bail out.
             */
            return STATUS_INVALID_HANDLE;
        }
    }
#endif // FE_IME
    if (ScreenInfo->WindowMaximizedX != WindowMaximizedX ||
        ScreenInfo->WindowMaximizedY != WindowMaximizedY) {
        ScreenInfo->WindowMaximizedX = WindowMaximizedX;
        ScreenInfo->WindowMaximizedY = WindowMaximizedY;
        UpdateWindow = TRUE;
    }
    if (UpdateWindow) {
        SetWindowSize(ScreenInfo);
    }

    if (DoScrollBarUpdate) {
         UpdateScrollBars(ScreenInfo);
    }
    if (ScreenInfo->Console->InputBuffer.InputMode & ENABLE_WINDOW_INPUT) {
        ScreenBufferSizeChange(ScreenInfo,ScreenInfo->ScreenBufferSize);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
AllocateScrollBuffer(
    DWORD Size
    )
{
    ScrollBuffer = (PCHAR_INFO)ConsoleHeapAlloc(MAKE_TAG( SCREEN_TAG ),Size);
    if (ScrollBuffer == NULL) {
        ScrollBufferSize = 0;
        return STATUS_NO_MEMORY;
    }
    ScrollBufferSize = Size;
    return STATUS_SUCCESS;
}

VOID
FreeScrollBuffer( VOID )
{
    ConsoleHeapFree(ScrollBuffer);
    ScrollBuffer = NULL;
    ScrollBufferSize = 0;
}

NTSTATUS
InitializeScrollBuffer( VOID )
{
    NTSTATUS Status;

    ghrgnScroll = CreateRectRgn(0,0,1,1);
    if (ghrgnScroll == NULL) {
        RIPMSG0(RIP_WARNING, "InitializeScrollBuffer: cannot allocate ghrgnScroll.");
        return STATUS_UNSUCCESSFUL;
    }
    gprgnData = (LPRGNDATA)ConsoleHeapAlloc(MAKE_TAG( SCREEN_TAG ),GRGNDATASIZE);
    if (gprgnData == NULL) {
        RIPMSG0(RIP_WARNING, "InitializeScrollBuffer: allocate gprgnData.");
        Status = STATUS_NO_MEMORY;
        goto error;
    }

    Status = AllocateScrollBuffer(DefaultRegInfo.ScreenBufferSize.X *
                                  DefaultRegInfo.ScreenBufferSize.Y *
                                  sizeof(CHAR_INFO));
    if (!NT_SUCCESS(Status)) {
        goto error;
    }

    Status = RtlInitializeCriticalSectionAndSpinCount(&ScrollBufferLock,
                                                      0x80000000);

error:
    if (!NT_SUCCESS(Status)) {
        RIPMSG0(RIP_WARNING, "InitializeScrollBuffer failed, cleaning up");
        if (ghrgnScroll) {
            DeleteObject(ghrgnScroll);
            ghrgnScroll = NULL;
        }
        if (gprgnData) {
            ConsoleHeapFree(gprgnData);
            gprgnData = NULL;
        }
    }

    return Status;
}

VOID
UpdateComplexRegion(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN COORD FontSize
    )
{
    int iSize,i;
    LPRECT pRect;
    SMALL_RECT UpdateRegion;
    LPRGNDATA pRgnData;

    if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
        ScreenInfo->BufferInfo.TextInfo.Flags &= ~TEXT_VALID_HINT;
    }
    pRgnData = gprgnData;

    /*
     * the dreaded complex region.
     */
    iSize = GetRegionData(ghrgnScroll, 0, NULL);
    if (iSize > GRGNDATASIZE) {
        pRgnData = (LPRGNDATA)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),iSize);
        if (pRgnData == NULL)
            return;
    }

    if (!GetRegionData(ghrgnScroll, iSize, pRgnData)) {
        ASSERT(FALSE);
        if (pRgnData != gprgnData) {
            ConsoleHeapFree(pRgnData);
        }
        return;
    }

    pRect = (PRECT)&pRgnData->Buffer;

    /*
     * Redraw each rectangle
     */
    for(i=0;i<(int)pRgnData->rdh.nCount;i++,pRect++) {
        /*
         * ICK!!!!!! Convert to chars. This sucks. We know
         * this is only get to get converted back during
         * the textout call.
         */
        UpdateRegion.Left = (SHORT)((pRect->left/FontSize.X)+ \
                            ScreenInfo->Window.Left);
        UpdateRegion.Right = (SHORT)(((pRect->right-1)/FontSize.X)+ \
                            ScreenInfo->Window.Left);
        UpdateRegion.Top = (SHORT)((pRect->top/FontSize.Y)+ \
                            ScreenInfo->Window.Top);
        UpdateRegion.Bottom = (SHORT)(((pRect->bottom-1)/FontSize.Y)+ \
                            ScreenInfo->Window.Top);
        /*
         * Fill the rectangle with goodies.
         */
        WriteToScreen(ScreenInfo, &UpdateRegion);
    }
    if (pRgnData != gprgnData) {
        ConsoleHeapFree(pRgnData);
    }
}

VOID
ScrollScreen(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT ScrollRect,
    IN PSMALL_RECT MergeRect,
    IN COORD TargetPoint
    )
{
    RECT ScrollRectGdi;
    SMALL_RECT UpdateRegion;
    COORD FontSize;
    BOOL Success;
    RECT BoundingBox;
#if defined(FE_SB)
    BYTE fBisect = 0;
    SMALL_RECT UpdateRect;
    SMALL_RECT TmpBisect;
#endif

    DBGOUTPUT(("ScrollScreen\n"));
    if (!ACTIVE_SCREEN_BUFFER(ScreenInfo)) {
        return;
    }
    if (ScreenInfo->Console->FullScreenFlags == 0 &&
        !(ScreenInfo->Console->Flags & (CONSOLE_IS_ICONIC | CONSOLE_NO_WINDOW))) {
#if defined(FE_SB)
        if (ScreenInfo->BisectFlag){
            SMALL_RECT RedrawRect;
            if (ScrollRect->Top < TargetPoint.Y){
                RedrawRect.Top = ScrollRect->Top;
                RedrawRect.Bottom = TargetPoint.Y+(ScrollRect->Bottom-ScrollRect->Top);
            }
            else{
                RedrawRect.Top = TargetPoint.Y;
                RedrawRect.Bottom = ScrollRect->Bottom;
            }
            if (ScrollRect->Left < TargetPoint.X){
                RedrawRect.Left = ScrollRect->Left;
                RedrawRect.Right = TargetPoint.X+(ScrollRect->Right-ScrollRect->Left);
            }
            else{
                RedrawRect.Left = TargetPoint.X;
                RedrawRect.Right = ScrollRect->Right;
            }
            WriteToScreen(ScreenInfo,&RedrawRect);
        }
        else{
#endif
        ScrollRectGdi.left = ScrollRect->Left-ScreenInfo->Window.Left;
        ScrollRectGdi.right = (ScrollRect->Right-ScreenInfo->Window.Left+1);
        ScrollRectGdi.top = ScrollRect->Top-ScreenInfo->Window.Top;
        ScrollRectGdi.bottom = (ScrollRect->Bottom-ScreenInfo->Window.Top+1);
        if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
            FontSize = SCR_FONTSIZE(ScreenInfo);
            ScrollRectGdi.left *= FontSize.X;
            ScrollRectGdi.right *= FontSize.X;
            ScrollRectGdi.top *= FontSize.Y;
            ScrollRectGdi.bottom *= FontSize.Y;
            ASSERT (ScreenInfo->BufferInfo.TextInfo.UpdatingScreen>0);
        } else {
            FontSize.X = 1;
            FontSize.Y = 1;
        }
        SCROLLDC_CALL;
        LockScrollBuffer();
        Success = (int)ScrollDC(ScreenInfo->Console->hDC,
                             (TargetPoint.X-ScrollRect->Left)*FontSize.X,
                             (TargetPoint.Y-ScrollRect->Top)*FontSize.Y,
                             &ScrollRectGdi,
                             NULL,
                             ghrgnScroll,
                             NULL);
        if (Success) {
            /*
             * Fetch our rectangles. If this is a simple rect then
             * we have already retrieved the rectangle. Otherwise
             * we need to call gdi to get the rectangles. We are
             * optimzied for speed rather than size.
             */
            switch (GetRgnBox(ghrgnScroll, &BoundingBox)) {
            case SIMPLEREGION:
                UpdateRegion.Left = (SHORT)((BoundingBox.left / FontSize.X) + \
                                    ScreenInfo->Window.Left);
                UpdateRegion.Right = (SHORT)(((BoundingBox.right-1) / FontSize.X) + \
                                    ScreenInfo->Window.Left);
                UpdateRegion.Top = (SHORT)((BoundingBox.top / FontSize.Y) + \
                                    ScreenInfo->Window.Top);
                UpdateRegion.Bottom = (SHORT)(((BoundingBox.bottom-1) / FontSize.Y) + \
                                    ScreenInfo->Window.Top);
#if defined(FE_SB)
                fBisect = ScreenInfo->BisectFlag;
#endif
                WriteToScreen(ScreenInfo, &UpdateRegion);
                break;
            case COMPLEXREGION:
                UpdateComplexRegion(ScreenInfo, FontSize);
                break;
            }

            if (MergeRect) {
#if defined(FE_SB)
                if (fBisect)
                    ScreenInfo->BisectFlag = fBisect;
                else
                    fBisect = ScreenInfo->BisectFlag;
#endif
                WriteToScreen(ScreenInfo, MergeRect);
            }
#if defined(FE_SB)
            if (CONSOLE_IS_DBCS_OUTPUTCP(ScreenInfo->Console)) {
                UpdateRect.Left = TargetPoint.X;
                UpdateRect.Right = ScrollRect->Right + (TargetPoint.X-ScrollRect->Left);
                UpdateRect.Top = TargetPoint.Y;
                UpdateRect.Bottom = ScrollRect->Bottom + (TargetPoint.Y-ScrollRect->Top);
                if (UpdateRect.Left &&
                    UpdateRect.Right+1 < ScreenInfo->ScreenBufferSize.X &&
                    UpdateRect.Right-UpdateRect.Left <= 2) {
                    TmpBisect.Left = UpdateRect.Left-1;
                    TmpBisect.Right = UpdateRect.Right+1;
                    TmpBisect.Top = UpdateRect.Top;
                    TmpBisect.Bottom = UpdateRect.Bottom;
                    WriteToScreen(ScreenInfo, &TmpBisect);
                }
                else {
                    if (UpdateRect.Left) {
                        TmpBisect.Left = UpdateRect.Left-1;
                        TmpBisect.Right = UpdateRect.Left;
                        TmpBisect.Top = UpdateRect.Top;
                        TmpBisect.Bottom = UpdateRect.Bottom;
                        WriteToScreen(ScreenInfo, &TmpBisect);
                    }
                    if (UpdateRect.Right+1 < ScreenInfo->ScreenBufferSize.X) {
                        TmpBisect.Left = UpdateRect.Right;
                        TmpBisect.Right = UpdateRect.Right+1;
                        TmpBisect.Top = UpdateRect.Top;
                        TmpBisect.Bottom = UpdateRect.Bottom;
                        WriteToScreen(ScreenInfo, &TmpBisect);
                    }
                }
            }
#endif
        } else {
#if defined(FE_SB)
            if (fBisect)
                ScreenInfo->BisectFlag = fBisect;
            else
                fBisect = ScreenInfo->BisectFlag;
#endif
            WriteToScreen(ScreenInfo, &ScreenInfo->Window);
        }
        UnlockScrollBuffer();
#if defined(FE_SB)
        }
#endif
    }
#ifdef i386
    else if (ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {
#if defined(FE_SB)
        if (CONSOLE_IS_DBCS_OUTPUTCP(ScreenInfo->Console)) {
            if (! ScreenInfo->ConvScreenInfo) {
                if (ScreenInfo->Console->CurrentScreenBuffer == ScreenInfo) {
                    ScrollHW(ScreenInfo,
                             ScrollRect,
                             MergeRect,
                             TargetPoint
                            );
                }
            }
            else if (ScreenInfo->Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER) {
                ScrollHW(ScreenInfo,
                         ScrollRect,
                         MergeRect,
                         TargetPoint
                        );
            }
        }
        else
#endif
        ScrollHW(ScreenInfo,
                 ScrollRect,
                 MergeRect,
                 TargetPoint
                );
    }
#endif
}


void CopyRow(
    PROW Row,
    PROW PrevRow)
{
    if (PrevRow->AttrRow.Length != 1 ||
        Row->AttrRow.Length != 1 ||
        PrevRow->AttrRow.Attrs->Attr != Row->AttrRow.Attrs->Attr) {
        Row->CharRow.OldRight = INVALID_OLD_LENGTH;
        Row->CharRow.OldLeft = INVALID_OLD_LENGTH;
    } else {
        Row->CharRow.OldRight = PrevRow->CharRow.Right;
        Row->CharRow.OldLeft = PrevRow->CharRow.Left;
    }
}

SHORT
ScrollEntireScreen(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN SHORT ScrollValue,
    IN BOOL UpdateRowIndex
    )

/**++

    this routine updates FirstRow and all the OldLeft and OldRight
    values when the screen is scrolled up by ScrollValue.

--*/

{
    SHORT RowIndex;
    int i;
    int new;
    int old;

    ScreenInfo->BufferInfo.TextInfo.Flags |= TEXT_VALID_HINT;

    //
    // store index of first row
    //

    RowIndex = ScreenInfo->BufferInfo.TextInfo.FirstRow;

    //
    // update the oldright and oldleft values
    //

    new = (RowIndex + ScreenInfo->Window.Bottom + ScrollValue) %
               ScreenInfo->ScreenBufferSize.Y;
    old = (RowIndex + ScreenInfo->Window.Bottom) %
               ScreenInfo->ScreenBufferSize.Y;
    for (i = WINDOW_SIZE_Y(&ScreenInfo->Window) - 1; i >= 0; i--) {
        CopyRow(
            &ScreenInfo->BufferInfo.TextInfo.Rows[new],
            &ScreenInfo->BufferInfo.TextInfo.Rows[old]);
        if (--new < 0)
            new = ScreenInfo->ScreenBufferSize.Y - 1;
        if (--old < 0)
            old = ScreenInfo->ScreenBufferSize.Y - 1;
    }

    //
    // update screen buffer
    //

    if (UpdateRowIndex) {
        ScreenInfo->BufferInfo.TextInfo.FirstRow =
            (SHORT)((RowIndex + ScrollValue) % ScreenInfo->ScreenBufferSize.Y);
    }

    return RowIndex;
}

VOID
StreamScrollRegion(
    IN PSCREEN_INFORMATION ScreenInfo
    )

/*++

Routine Description:

    This routine is a special-purpose scroll for use by
    AdjustCursorPosition.

Arguments:

    ScreenInfo - pointer to screen buffer info.

Return Value:

--*/

{
    SHORT RowIndex;
    PROW Row;
    PWCHAR Char;
    RECT Rect;
    RECT BoundingBox;
    int ScreenWidth,ScrollHeight,ScreenHeight;
    COORD FontSize;
    SMALL_RECT UpdateRegion;
    BOOL Success;
    int i;
#if defined(FE_SB)
    PBYTE AttrP;
#endif
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;

    RowIndex = ScrollEntireScreen(ScreenInfo,1,TRUE);

    Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];

    //
    // fill line with blanks
    //

    Char = &Row->CharRow.Chars[Row->CharRow.Left];
    for (i=Row->CharRow.Left;i<Row->CharRow.Right;i++) {
        *Char = (WCHAR)' ';
        Char++;
    }
#if defined(FE_SB)
    if (CONSOLE_IS_DBCS_OUTPUTCP(Console)){
        int LineWidth = Row->CharRow.Right - Row->CharRow.Left;
        AttrP = &Row->CharRow.KAttrs[Row->CharRow.Left];
        if ( LineWidth > 0 )
            RtlZeroMemory(AttrP, LineWidth);
        AttrP += LineWidth;
        Row->CharRow.OldRight = INVALID_OLD_LENGTH;
        Row->CharRow.OldLeft = INVALID_OLD_LENGTH;
        Console->ConsoleIme.ScrollWaitCountDown = Console->ConsoleIme.ScrollWaitTimeout;
    }
#endif
    Row->CharRow.Right = 0;
    Row->CharRow.Left = ScreenInfo->ScreenBufferSize.X;

    //
    // set up attributes
    //

    if (Row->AttrRow.Length != 1) {
        ConsoleHeapFree(Row->AttrRow.Attrs);
        Row->AttrRow.Attrs = &Row->AttrRow.AttrPair;
        Row->AttrRow.AttrPair.Length = ScreenInfo->ScreenBufferSize.X;
        Row->AttrRow.Length = 1;
    }
    Row->AttrRow.AttrPair.Attr = ScreenInfo->Attributes;

    //
    // update screen
    //

    if (ACTIVE_SCREEN_BUFFER(ScreenInfo) &&
        Console->FullScreenFlags == 0 &&
        !(Console->Flags & (CONSOLE_IS_ICONIC | CONSOLE_NO_WINDOW))) {

        ConsoleHideCursor(ScreenInfo);
        if (UsePolyTextOut) {
            WriteRegionToScreen(ScreenInfo, &ScreenInfo->Window);
        } else {
            FontSize = SCR_FONTSIZE(ScreenInfo);
            ScreenWidth = WINDOW_SIZE_X(&ScreenInfo->Window) * FontSize.X;
            ScreenHeight = WINDOW_SIZE_Y(&ScreenInfo->Window) * FontSize.Y;
            ScrollHeight = ScreenHeight - FontSize.Y;

            Rect.left = 0;
            Rect.right = ScreenWidth;
            Rect.top = FontSize.Y;
            Rect.bottom = ScreenHeight;

            //
            // find smallest bounding rectangle
            //

            if (ScreenInfo->BufferInfo.TextInfo.Flags & TEXT_VALID_HINT) {
                SHORT MinLeft,MaxRight;
                MinLeft = ScreenInfo->ScreenBufferSize.X;
                MaxRight = 0;
                RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+ScreenInfo->Window.Top) % ScreenInfo->ScreenBufferSize.Y;
                for (i=ScreenInfo->Window.Top+1;i<=ScreenInfo->Window.Bottom;i++) {
                    Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
                    if (Row->CharRow.OldLeft == INVALID_OLD_LENGTH) {
                        MinLeft = 0;
                    } else {
                        if (MinLeft > min(Row->CharRow.Left,Row->CharRow.OldLeft)) {
                            MinLeft = min(Row->CharRow.Left,Row->CharRow.OldLeft);
                        }
                    }
                    if (Row->CharRow.OldRight == INVALID_OLD_LENGTH) {
                        MaxRight = ScreenInfo->ScreenBufferSize.X-1;
                    } else {
                        if (MaxRight < max(Row->CharRow.Right,Row->CharRow.OldRight)) {
                            MaxRight = max(Row->CharRow.Right,Row->CharRow.OldRight);
                        }
                    }
                    if (++RowIndex == ScreenInfo->ScreenBufferSize.Y) {
                        RowIndex = 0;
                    }
                }
                Rect.left = MinLeft*FontSize.X;
                Rect.right = (MaxRight+1)*FontSize.X;
            }

            LockScrollBuffer();
            ASSERT (ScreenInfo->BufferInfo.TextInfo.UpdatingScreen>0);
            Success = (int)ScrollDC(Console->hDC,
                                0,
                                -FontSize.Y,
                                &Rect,
                                NULL,
                                ghrgnScroll,
                                NULL
                               );
            if (Success && ScreenInfo->Window.Top!=ScreenInfo->Window.Bottom) {
#if defined(FE_SB)
                if (CONSOLE_IS_DBCS_OUTPUTCP(Console) &&
                    ScreenInfo->Attributes & (COMMON_LVB_GRID_HORIZONTAL +
                                               COMMON_LVB_GRID_LVERTICAL +
                                               COMMON_LVB_GRID_RVERTICAL +
                                               COMMON_LVB_REVERSE_VIDEO  +
                                               COMMON_LVB_UNDERSCORE     )){
                    UpdateRegion = ScreenInfo->Window;
                    UpdateRegion.Top = UpdateRegion.Bottom;
                    ScreenInfo->BufferInfo.TextInfo.Flags &= ~TEXT_VALID_HINT;
                    WriteToScreen(ScreenInfo,&UpdateRegion);
                }
                else{
#endif
                switch (GetRgnBox(ghrgnScroll, &BoundingBox)) {
                case SIMPLEREGION:
                    if (BoundingBox.left == 0 &&
                        BoundingBox.right == ScreenWidth &&
                        BoundingBox.top == ScrollHeight &&
                        BoundingBox.bottom == ScreenHeight) {

                        PatBlt(Console->hDC,0,ScrollHeight,ScreenWidth,FontSize.Y,PATCOPY);
                        GdiFlush();
                    } else {
                        UpdateRegion.Left = (SHORT)((BoundingBox.left/FontSize.X)+ScreenInfo->Window.Left);
                        UpdateRegion.Right = (SHORT)(((BoundingBox.right-1)/FontSize.X)+ScreenInfo->Window.Left);
                        UpdateRegion.Top = (SHORT)((BoundingBox.top/FontSize.Y)+ScreenInfo->Window.Top);
                        UpdateRegion.Bottom = (SHORT)(((BoundingBox.bottom-1)/FontSize.Y)+ScreenInfo->Window.Top);
                        WriteToScreen(ScreenInfo,&UpdateRegion);
                    }
                    break;
                case COMPLEXREGION:
                    UpdateComplexRegion(ScreenInfo,FontSize);
                    break;
                }
#if defined(FE_SB)
                }
#endif
            } else  {
                WriteToScreen(ScreenInfo,&ScreenInfo->Window);
            }
            UnlockScrollBuffer();
        }
        ConsoleShowCursor(ScreenInfo);
    }
#ifdef i386
    else if (Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {
        SMALL_RECT ScrollRect;
        COORD TargetPoint;

        ScrollRect = ScreenInfo->Window;
        TargetPoint.Y = ScrollRect.Top;
        ScrollRect.Top += 1;
        TargetPoint.X = 0;
#if defined(FE_SB)
        if (CONSOLE_IS_DBCS_OUTPUTCP(Console) ) {
            if (! ScreenInfo->ConvScreenInfo)  {
                if (ScreenInfo->Console->CurrentScreenBuffer == ScreenInfo) {
                    ScrollHW(ScreenInfo,
                             &ScrollRect,
                             NULL,
                             TargetPoint
                            );
                }
            }
            else if (ScreenInfo->Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER) {
                ScrollHW(ScreenInfo,
                         &ScrollRect,
                         NULL,
                         TargetPoint
                        );
            }
        }
        else
#endif
        ScrollHW(ScreenInfo,
                 &ScrollRect,
                 NULL,
                 TargetPoint
                );
        ScrollRect.Top = ScrollRect.Bottom - 1;
        WriteRegionToScreenHW(ScreenInfo,&ScrollRect);
    }
#endif
}

NTSTATUS
ScrollRegion(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN OUT PSMALL_RECT ScrollRectangle,
    IN PSMALL_RECT ClipRectangle OPTIONAL,
    IN COORD  DestinationOrigin,
    IN CHAR_INFO Fill
    )

/*++

Routine Description:

    This routine copies ScrollRectangle to DestinationOrigin then
    fills in ScrollRectangle with Fill.  The scroll region is
    copied to a third buffer, the scroll region is filled, then the
    original contents of the scroll region are copied to the destination.

Arguments:

    ScreenInfo - pointer to screen buffer info.

    ScrollRectangle - Region to copy

    ClipRectangle - Optional pointer to clip region.

    DestinationOrigin - Upper left corner of target region.

    Fill - Character and attribute to fill source region with.

Return Value:

--*/

{
    SMALL_RECT TargetRectangle, SourceRectangle;
    COORD TargetPoint;
    COORD Size;
    SMALL_RECT OurClipRectangle;
    SMALL_RECT ScrollRectangle2,ScrollRectangle3;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;

    // here's how we clip:
    //
    // Clip source rectangle to screen buffer => S
    // Create target rectangle based on S => T
    // Clip T to ClipRegion => T
    // Create S2 based on clipped T => S2
    // Clip S to ClipRegion => S3
    //
    // S2 is the region we copy to T
    // S3 is the region to fill

    if (Fill.Char.UnicodeChar == '\0' && Fill.Attributes == 0) {
        Fill.Char.UnicodeChar = (WCHAR)' ';
        Fill.Attributes = ScreenInfo->Attributes;
    }

    //
    // clip the source rectangle to the screen buffer
    //

    if (ScrollRectangle->Left < 0) {
        DestinationOrigin.X += -ScrollRectangle->Left;
        ScrollRectangle->Left = 0;
    }
    if (ScrollRectangle->Top < 0) {
        DestinationOrigin.Y += -ScrollRectangle->Top;
        ScrollRectangle->Top = 0;
    }
    if (ScrollRectangle->Right >= ScreenInfo->ScreenBufferSize.X) {
        ScrollRectangle->Right = (SHORT)(ScreenInfo->ScreenBufferSize.X-1);
    }
    if (ScrollRectangle->Bottom >= ScreenInfo->ScreenBufferSize.Y) {
        ScrollRectangle->Bottom = (SHORT)(ScreenInfo->ScreenBufferSize.Y-1);
    }

    //
    // if source rectangle doesn't intersect screen buffer, return.
    //

    if (ScrollRectangle->Bottom < ScrollRectangle->Top ||
        ScrollRectangle->Right < ScrollRectangle->Left) {
        return STATUS_SUCCESS;
    }

    //
    // clip the target rectangle
    // if a cliprectangle was provided, clip it to the screen buffer.
    // if not, set the cliprectangle to the screen buffer region.
    //

    if (ClipRectangle) {

        //
        // clip the cliprectangle.
        //

        if (ClipRectangle->Left < 0) {
            ClipRectangle->Left = 0;
        }
        if (ClipRectangle->Top < 0) {
            ClipRectangle->Top = 0;
        }
        if (ClipRectangle->Right >= ScreenInfo->ScreenBufferSize.X) {
            ClipRectangle->Right = (SHORT)(ScreenInfo->ScreenBufferSize.X-1);
        }
        if (ClipRectangle->Bottom >= ScreenInfo->ScreenBufferSize.Y) {
            ClipRectangle->Bottom = (SHORT)(ScreenInfo->ScreenBufferSize.Y-1);
        }
    }
    else {
        OurClipRectangle.Left = 0;
        OurClipRectangle.Top = 0;
        OurClipRectangle.Right = (SHORT)(ScreenInfo->ScreenBufferSize.X-1);
        OurClipRectangle.Bottom = (SHORT)(ScreenInfo->ScreenBufferSize.Y-1);
        ClipRectangle = &OurClipRectangle;
    }

    //
    // Create target rectangle based on S => T
    // Clip T to ClipRegion => T
    // Create S2 based on clipped T => S2
    //

    ScrollRectangle2 = *ScrollRectangle;
    TargetRectangle.Left = DestinationOrigin.X;
    TargetRectangle.Top = DestinationOrigin.Y;
    TargetRectangle.Right = (SHORT)(DestinationOrigin.X + (ScrollRectangle2.Right -  ScrollRectangle2.Left + 1) - 1);
    TargetRectangle.Bottom = (SHORT)(DestinationOrigin.Y + (ScrollRectangle2.Bottom - ScrollRectangle2.Top + 1) - 1);

    if (TargetRectangle.Left < ClipRectangle->Left) {
        ScrollRectangle2.Left += ClipRectangle->Left - TargetRectangle.Left;
        TargetRectangle.Left = ClipRectangle->Left;
    }
    if (TargetRectangle.Top < ClipRectangle->Top) {
        ScrollRectangle2.Top += ClipRectangle->Top - TargetRectangle.Top;
        TargetRectangle.Top = ClipRectangle->Top;
    }
    if (TargetRectangle.Right > ClipRectangle->Right) {
        ScrollRectangle2.Right -= TargetRectangle.Right - ClipRectangle->Right;
        TargetRectangle.Right = ClipRectangle->Right;
    }
    if (TargetRectangle.Bottom > ClipRectangle->Bottom) {
        ScrollRectangle2.Bottom -= TargetRectangle.Bottom - ClipRectangle->Bottom;
        TargetRectangle.Bottom = ClipRectangle->Bottom;
    }

    //
    // clip scroll rect to clipregion => S3
    //

    ScrollRectangle3 = *ScrollRectangle;
    if (ScrollRectangle3.Left < ClipRectangle->Left) {
        ScrollRectangle3.Left = ClipRectangle->Left;
    }
    if (ScrollRectangle3.Top < ClipRectangle->Top) {
        ScrollRectangle3.Top = ClipRectangle->Top;
    }
    if (ScrollRectangle3.Right > ClipRectangle->Right) {
        ScrollRectangle3.Right = ClipRectangle->Right;
    }
    if (ScrollRectangle3.Bottom > ClipRectangle->Bottom) {
        ScrollRectangle3.Bottom = ClipRectangle->Bottom;
    }

    //
    // if scroll rect doesn't intersect clip region, return.
    //

    if (ScrollRectangle3.Bottom < ScrollRectangle3.Top ||
        ScrollRectangle3.Right < ScrollRectangle3.Left) {
        return STATUS_SUCCESS;
    }

    ConsoleHideCursor(ScreenInfo);

#if defined(FE_IME)
    Console->ConsoleIme.ScrollWaitCountDown = Console->ConsoleIme.ScrollWaitTimeout;
#endif // FE_IME
    //
    // if target rectangle doesn't intersect screen buffer, skip scrolling
    // part.
    //

    if (!(TargetRectangle.Bottom < TargetRectangle.Top ||
          TargetRectangle.Right < TargetRectangle.Left)) {

        //
        // if we can, don't use intermediate scroll region buffer.  do this
        // by figuring out fill rectangle.  NOTE: this code will only work
        // if CopyRectangle copies from low memory to high memory (otherwise
        // we would overwrite the scroll region before reading it).
        //

        if (ScrollRectangle2.Right == TargetRectangle.Right &&
            ScrollRectangle2.Left == TargetRectangle.Left &&
            ScrollRectangle2.Top > TargetRectangle.Top &&
            ScrollRectangle2.Top < TargetRectangle.Bottom) {

            SMALL_RECT FillRect;
            SHORT LastRowIndex,OldRight,OldLeft;
            PROW Row;

            TargetPoint.X = TargetRectangle.Left;
            TargetPoint.Y = TargetRectangle.Top;
            if (ScrollRectangle2.Right == (SHORT)(ScreenInfo->ScreenBufferSize.X-1) &&
                ScrollRectangle2.Left == 0 &&
                ScrollRectangle2.Bottom == (SHORT)(ScreenInfo->ScreenBufferSize.Y-1) &&
                ScrollRectangle2.Top == 1 ) {
                LastRowIndex = ScrollEntireScreen(ScreenInfo,(SHORT)(ScrollRectangle2.Top-TargetRectangle.Top),TRUE);
                Row = &ScreenInfo->BufferInfo.TextInfo.Rows[LastRowIndex];
                OldRight = Row->CharRow.OldRight;
                OldLeft = Row->CharRow.OldLeft;
            } else {
                LastRowIndex = -1;
                CopyRectangle(ScreenInfo,
                              &ScrollRectangle2,
                              TargetPoint
                             );
            }
            FillRect.Left = TargetRectangle.Left;
            FillRect.Right = TargetRectangle.Right;
            FillRect.Top = (SHORT)(TargetRectangle.Bottom+1);
            FillRect.Bottom = ScrollRectangle->Bottom;
            if (FillRect.Top < ClipRectangle->Top) {
                FillRect.Top = ClipRectangle->Top;
            }
            if (FillRect.Bottom > ClipRectangle->Bottom) {
                FillRect.Bottom = ClipRectangle->Bottom;
            }
            FillRectangle(Fill,
                          ScreenInfo,
                          &FillRect
                         );

            //
            // After ScrollEntireScreen, the OldRight and OldLeft values
            // for the last row are set correctly.  however, FillRectangle
            // resets them with the previous first row of the screen.
            // reset them here.
            //

            if (LastRowIndex != -1) {
                Row->CharRow.OldRight = OldRight;
                Row->CharRow.OldLeft = OldLeft;
            }

            //
            // update to screen, if we're not iconic.  we're marked as
            // iconic if we're fullscreen, so check for fullscreen.
            //

            if (!(Console->Flags & CONSOLE_IS_ICONIC) ||
                 Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {
                ScrollScreen(ScreenInfo,
                       &ScrollRectangle2,
                       &FillRect,
                       TargetPoint
                      );
            }
        }

        //
        // if no overlap, don't need intermediate copy
        //

        else if (ScrollRectangle3.Right < TargetRectangle.Left ||
                 ScrollRectangle3.Left > TargetRectangle.Right ||
                 ScrollRectangle3.Top > TargetRectangle.Bottom ||
                 ScrollRectangle3.Bottom < TargetRectangle.Top) {
            TargetPoint.X = TargetRectangle.Left;
            TargetPoint.Y = TargetRectangle.Top;
            CopyRectangle(ScreenInfo,
                          &ScrollRectangle2,
                          TargetPoint
                         );
            FillRectangle(Fill,
                          ScreenInfo,
                          &ScrollRectangle3
                         );

            //
            // update to screen, if we're not iconic.  we're marked as
            // iconic if we're fullscreen, so check for fullscreen.
            //

            if (!(Console->Flags & CONSOLE_IS_ICONIC) ||
                Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {
                ScrollScreen(ScreenInfo,
                       &ScrollRectangle2,
                       &ScrollRectangle3,
                       TargetPoint
                      );
            }
        }

        //
        // for the case where the source and target rectangles overlap, we
        // copy the source rectangle, fill it, then copy it to the target.
        //

        else {
            SMALL_RECT TargetRect;
            COORD SourcePoint;

            LockScrollBuffer();
            Size.X = (SHORT)(ScrollRectangle2.Right - ScrollRectangle2.Left + 1);
            Size.Y = (SHORT)(ScrollRectangle2.Bottom - ScrollRectangle2.Top + 1);
            if (ScrollBufferSize < (Size.X * Size.Y * sizeof(CHAR_INFO))) {
                FreeScrollBuffer();
                Status = AllocateScrollBuffer(Size.X * Size.Y * sizeof(CHAR_INFO));
                if (!NT_SUCCESS(Status)) {
                    UnlockScrollBuffer();
                    ConsoleShowCursor(ScreenInfo);
                    return Status;
                }
            }

            TargetRect.Left = 0;
            TargetRect.Top = 0;
            TargetRect.Right = ScrollRectangle2.Right - ScrollRectangle2.Left;
            TargetRect.Bottom = ScrollRectangle2.Bottom - ScrollRectangle2.Top;
            SourcePoint.X = ScrollRectangle2.Left;
            SourcePoint.Y = ScrollRectangle2.Top;
            ReadRectFromScreenBuffer(ScreenInfo,
                                     SourcePoint,
                                     ScrollBuffer,
                                     Size,
                                     &TargetRect
                                    );

            FillRectangle(Fill,
                          ScreenInfo,
                          &ScrollRectangle3
                         );

            SourceRectangle.Top = 0;
            SourceRectangle.Left = 0;
            SourceRectangle.Right = (SHORT)(Size.X-1);
            SourceRectangle.Bottom = (SHORT)(Size.Y-1);
            TargetPoint.X = TargetRectangle.Left;
            TargetPoint.Y = TargetRectangle.Top;
            WriteRectToScreenBuffer((PBYTE)ScrollBuffer,
                                    Size,
                                    &SourceRectangle,
                                    ScreenInfo,
                                    TargetPoint,
                                    0xFFFFFFFF
                                   );
            UnlockScrollBuffer();

            //
            // update to screen, if we're not iconic.  we're marked as
            // iconic if we're fullscreen, so check for fullscreen.
            //

            if (!(Console->Flags & CONSOLE_IS_ICONIC) ||
                Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {

                //
                // update regions on screen.
                //

                ScrollScreen(ScreenInfo,
                       &ScrollRectangle2,
                       &ScrollRectangle3,
                       TargetPoint
                      );
            }
        }
    }
    else {

        //
        // do fill
        //

        FillRectangle(Fill,
                      ScreenInfo,
                      &ScrollRectangle3
                     );

        //
        // update to screen, if we're not iconic.  we're marked as
        // iconic if we're fullscreen, so check for fullscreen.
        //

        if (ACTIVE_SCREEN_BUFFER(ScreenInfo) &&
            !(Console->Flags & CONSOLE_IS_ICONIC) ||
            Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {
            WriteToScreen(ScreenInfo,&ScrollRectangle3);
        }
    }
    ConsoleShowCursor(ScreenInfo);
    return STATUS_SUCCESS;
}


NTSTATUS
SetWindowOrigin(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN BOOLEAN Absolute,
    IN COORD WindowOrigin
    )

/*++

Routine Description:

    This routine sets the window origin.

Arguments:

    ScreenInfo - pointer to screen buffer info.

    Absolute - if TRUE, WindowOrigin is specified in absolute screen
    buffer coordinates.  if FALSE, WindowOrigin is specified in coordinates
    relative to the current window origin.

    WindowOrigin - New window origin.

Return Value:

--*/

{
    SMALL_RECT NewWindow;
    COORD WindowSize;
    RECT BoundingBox;
    BOOL Success;
    RECT ScrollRect;
    SMALL_RECT UpdateRegion;
    COORD FontSize;
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;

    //
    // calculate window size
    //

    WindowSize.X = (SHORT)CONSOLE_WINDOW_SIZE_X(ScreenInfo);
    WindowSize.Y = (SHORT)CONSOLE_WINDOW_SIZE_Y(ScreenInfo);

    //
    // if relative coordinates, figure out absolute coords.
    //

    if (!Absolute) {
        if (WindowOrigin.X == 0 && WindowOrigin.Y == 0) {
            return STATUS_SUCCESS;
        }
        NewWindow.Left = ScreenInfo->Window.Left + WindowOrigin.X;
        NewWindow.Top = ScreenInfo->Window.Top + WindowOrigin.Y;
    }
    else {
        if (WindowOrigin.X == ScreenInfo->Window.Left &&
            WindowOrigin.Y == ScreenInfo->Window.Top) {
            return STATUS_SUCCESS;
        }
        NewWindow.Left = WindowOrigin.X;
        NewWindow.Top = WindowOrigin.Y;
    }
    NewWindow.Right = (SHORT)(NewWindow.Left + WindowSize.X - 1);
    NewWindow.Bottom = (SHORT)(NewWindow.Top + WindowSize.Y - 1);

    //
    // see if new window origin would extend window beyond extent of screen
    // buffer
    //

    if (NewWindow.Left < 0 || NewWindow.Top < 0 ||
        NewWindow.Right < 0 || NewWindow.Bottom < 0 ||
        NewWindow.Right >= ScreenInfo->ScreenBufferSize.X ||
        NewWindow.Bottom >= ScreenInfo->ScreenBufferSize.Y) {
        return STATUS_INVALID_PARAMETER;
    }

    if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
        FontSize = SCR_FONTSIZE(ScreenInfo);
        ScreenInfo->BufferInfo.TextInfo.Flags &= ~TEXT_VALID_HINT;
    } else {
        FontSize.X = 1;
        FontSize.Y = 1;
    }
    ConsoleHideCursor(ScreenInfo);
    if (ACTIVE_SCREEN_BUFFER(ScreenInfo) &&
        Console->FullScreenFlags == 0 &&
        !(Console->Flags & (CONSOLE_IS_ICONIC | CONSOLE_NO_WINDOW))) {

        InvertSelection(Console, TRUE);
#if defined(FE_SB)
        if (CONSOLE_IS_DBCS_OUTPUTCP(Console) &&
            !(Console->ConsoleIme.ScrollFlag & HIDE_FOR_SCROLL)) {
            ConsoleImeBottomLineUse(ScreenInfo,0);
        }
#endif
        if (   ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER
            && UsePolyTextOut
            && NewWindow.Left == ScreenInfo->Window.Left
           ) {
            ScrollEntireScreen(ScreenInfo,
                (SHORT)(NewWindow.Top - ScreenInfo->Window.Top),
                FALSE);
            ScreenInfo->Window = NewWindow;
            WriteRegionToScreen(ScreenInfo, &NewWindow);
        } else {
#if defined(FE_SB)
            RECT ClipRect;
#endif
            ScrollRect.left = 0;
            ScrollRect.right = CONSOLE_WINDOW_SIZE_X(ScreenInfo)*FontSize.X;
            ScrollRect.top = 0;
#if defined(FE_SB)
            if (CONSOLE_IS_DBCS_OUTPUTCP(Console) &&
                Console->InputBuffer.ImeMode.Open )
            {
                if (ScreenInfo->Window.Top <= NewWindow.Top)
                    ScrollRect.bottom = (CONSOLE_WINDOW_SIZE_Y(ScreenInfo)-1)*FontSize.Y;
                else
                    ScrollRect.bottom = (CONSOLE_WINDOW_SIZE_Y(ScreenInfo)-2)*FontSize.Y;
                ClipRect = ScrollRect;
                ClipRect.bottom = (CONSOLE_WINDOW_SIZE_Y(ScreenInfo)-1)*FontSize.Y;
            }
            else
#endif
            ScrollRect.bottom = CONSOLE_WINDOW_SIZE_Y(ScreenInfo)*FontSize.Y;

#if defined(FE_SB)
            if (CONSOLE_IS_DBCS_OUTPUTCP(Console) &&
                ScrollRect.bottom == 0) {
                UpdateRegion.Left   = 0;
                UpdateRegion.Top    = 0;
                UpdateRegion.Right  = CONSOLE_WINDOW_SIZE_X(ScreenInfo);
                UpdateRegion.Bottom = 0;
                WriteToScreen(ScreenInfo,&UpdateRegion);
            }
            else {
#endif
            SCROLLDC_CALL;
#if defined(FE_SB)
                if (CONSOLE_IS_DBCS_OUTPUTCP(Console) &&
                     Console->InputBuffer.ImeMode.Open )
                {
                    Success = ScrollDC(Console->hDC,
                                         (ScreenInfo->Window.Left-NewWindow.Left)*FontSize.X,
                                         (ScreenInfo->Window.Top-NewWindow.Top)*FontSize.Y,
                                         &ScrollRect,
                                         &ClipRect,
                                         NULL,
                                         &BoundingBox
                                         );
                }
                else
#endif
            Success = ScrollDC(Console->hDC,
                                 (ScreenInfo->Window.Left-NewWindow.Left)*FontSize.X,
                                 (ScreenInfo->Window.Top-NewWindow.Top)*FontSize.Y,
                                 &ScrollRect,
                                 NULL,
                                 NULL,
                                 &BoundingBox
                                 );

            if (Success) {
                UpdateRegion.Left = (SHORT)((BoundingBox.left/FontSize.X)+NewWindow.Left);
                UpdateRegion.Right = (SHORT)(((BoundingBox.right-1)/FontSize.X)+NewWindow.Left);
                UpdateRegion.Top = (SHORT)((BoundingBox.top/FontSize.Y)+NewWindow.Top);
                UpdateRegion.Bottom = (SHORT)(((BoundingBox.bottom-1)/FontSize.Y)+NewWindow.Top);
            }
            else  {
                UpdateRegion = NewWindow;
            }

            //
            // new window is ok.  store it in screeninfo and refresh screen.
            //

            ScreenInfo->Window = NewWindow;

            WriteToScreen(ScreenInfo,&UpdateRegion);
#if defined(FE_SB)
            }
#endif
        }
        InvertSelection(Console, FALSE);
    }
#ifdef i386
    else if (Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE &&
             ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {


        //
        // keep mouse pointer on screen
        //

        if (ScreenInfo->BufferInfo.TextInfo.MousePosition.X < NewWindow.Left) {
            ScreenInfo->BufferInfo.TextInfo.MousePosition.X = NewWindow.Left;
        } else if (ScreenInfo->BufferInfo.TextInfo.MousePosition.X > NewWindow.Right) {
            ScreenInfo->BufferInfo.TextInfo.MousePosition.X = NewWindow.Right;
        }

        if (ScreenInfo->BufferInfo.TextInfo.MousePosition.Y < NewWindow.Top) {
            ScreenInfo->BufferInfo.TextInfo.MousePosition.Y = NewWindow.Top;
        } else if (ScreenInfo->BufferInfo.TextInfo.MousePosition.Y > NewWindow.Bottom) {
            ScreenInfo->BufferInfo.TextInfo.MousePosition.Y = NewWindow.Bottom;
        }
        ScreenInfo->Window = NewWindow;
        WriteToScreen(ScreenInfo,&ScreenInfo->Window);
    }
#endif
    else {
        // we're iconic
        ScreenInfo->Window = NewWindow;
    }

#if defined(FE_SB)
    if (CONSOLE_IS_DBCS_OUTPUTCP(Console) ) {
        ConsoleImeResizeModeSystemView(Console,ScreenInfo->Window);
        ConsoleImeResizeCompStrView(Console,ScreenInfo->Window);
    }
#endif
    ConsoleShowCursor(ScreenInfo);

    if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
         ScreenInfo->BufferInfo.TextInfo.Flags |= TEXT_VALID_HINT;
    }

    UpdateScrollBars(ScreenInfo);
    return STATUS_SUCCESS;
}

NTSTATUS
ResizeWindow(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT WindowDimensions,
    IN BOOL DoScrollBarUpdate
    )

/*++

Routine Description:

    This routine changes the console data structures to reflect the specified
    window size change.  it does not call the user component to update
    the screen.

Arguments:

    ScreenInformation - the new screen buffer.

    dwWindowSize - the initial size of screen buffer's window.

    nFont - the initial font to generate text with.

    dwScreenBufferSize - the initial size of the screen buffer.

Return Value:


--*/

{
    //
    // make sure there's something to do
    //

    if (RtlEqualMemory(&ScreenInfo->Window, WindowDimensions, sizeof(SMALL_RECT))) {
        return STATUS_SUCCESS;
    }

    if (WindowDimensions->Left < 0) {
        WindowDimensions->Right -= WindowDimensions->Left;
        WindowDimensions->Left = 0;
    }
    if (WindowDimensions->Top < 0) {
        WindowDimensions->Bottom -= WindowDimensions->Top;
        WindowDimensions->Top = 0;
    }

    if (WindowDimensions->Right >= ScreenInfo->ScreenBufferSize.X) {
        WindowDimensions->Right = ScreenInfo->ScreenBufferSize.X;
    }
    if (WindowDimensions->Bottom >= ScreenInfo->ScreenBufferSize.Y) {
        WindowDimensions->Bottom = ScreenInfo->ScreenBufferSize.Y;
    }

    ScreenInfo->Window = *WindowDimensions;
    ScreenInfo->WindowMaximizedX = (CONSOLE_WINDOW_SIZE_X(ScreenInfo) == ScreenInfo->ScreenBufferSize.X);
    ScreenInfo->WindowMaximizedY = (CONSOLE_WINDOW_SIZE_Y(ScreenInfo) == ScreenInfo->ScreenBufferSize.Y);

    if (DoScrollBarUpdate) {
        UpdateScrollBars(ScreenInfo);
    }

    if (!(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER)) {
        return STATUS_SUCCESS;
    }

    if (ACTIVE_SCREEN_BUFFER(ScreenInfo)) {
        ScreenInfo->BufferInfo.TextInfo.Flags &= ~TEXT_VALID_HINT;
    }

#ifdef i386
    if (ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {

        //
        // keep mouse pointer on screen
        //

        if (ScreenInfo->BufferInfo.TextInfo.MousePosition.X < WindowDimensions->Left) {
            ScreenInfo->BufferInfo.TextInfo.MousePosition.X = WindowDimensions->Left;
        } else if (ScreenInfo->BufferInfo.TextInfo.MousePosition.X > WindowDimensions->Right) {
            ScreenInfo->BufferInfo.TextInfo.MousePosition.X = WindowDimensions->Right;
        }

        if (ScreenInfo->BufferInfo.TextInfo.MousePosition.Y < WindowDimensions->Top) {
            ScreenInfo->BufferInfo.TextInfo.MousePosition.Y = WindowDimensions->Top;
        } else if (ScreenInfo->BufferInfo.TextInfo.MousePosition.Y > WindowDimensions->Bottom) {
            ScreenInfo->BufferInfo.TextInfo.MousePosition.Y = WindowDimensions->Bottom;
        }
    }
#endif

    return(STATUS_SUCCESS);
}

VOID
SetWindowSize(
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
#if defined(FE_IME)
    if (ScreenInfo->ConvScreenInfo != NULL)
        return;
#endif
    if (ScreenInfo->Console->Flags & CONSOLE_SETTING_WINDOW_SIZE)
        return;
    ScreenInfo->Console->Flags |= CONSOLE_SETTING_WINDOW_SIZE;
    PostMessage(ScreenInfo->Console->hWnd,
                 CM_SET_WINDOW_SIZE,
                 (WPARAM)ScreenInfo,
                 0x47474747
                );
}

VOID
UpdateWindowSize(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
    LONG WindowStyle;

    if (!(Console->Flags & CONSOLE_IS_ICONIC)) {
        InternalUpdateScrollBars(ScreenInfo);

        WindowStyle = GetWindowLong(Console->hWnd, GWL_STYLE);
        if (ScreenInfo->WindowMaximized) {
            WindowStyle |= WS_MAXIMIZE;
        } else {
            WindowStyle &= ~WS_MAXIMIZE;
        }
        SetWindowLong(Console->hWnd, GWL_STYLE, WindowStyle);

        SetWindowPos(Console->hWnd, NULL,
                     0,
                     0,
                     Console->WindowRect.right-Console->WindowRect.left,
                     Console->WindowRect.bottom-Console->WindowRect.top,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_DRAWFRAME
                    );
        Console->ResizeFlags &= ~SCREEN_BUFFER_CHANGE;
    } else {
        Console->ResizeFlags |= SCREEN_BUFFER_CHANGE;
    }
}

NTSTATUS
InternalSetWindowSize(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Window
    )
{
    SIZE WindowSize;
    WORD WindowSizeX, WindowSizeY;

    Console->Flags &= ~CONSOLE_SETTING_WINDOW_SIZE;
    if (Console->CurrentScreenBuffer == ScreenInfo) {
        if (Console->FullScreenFlags == 0) {
            //
            // Make sure our max screen sizes reflect reality
            //

            if (gfInitSystemMetrics) {
                InitializeSystemMetrics();
            }

            //
            // figure out how big to make the window, given the desired client area
            // size.
            //

            ScreenInfo->ResizingWindow++;
            WindowSizeX = WINDOW_SIZE_X(Window);
            WindowSizeY = WINDOW_SIZE_Y(Window);
            if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
                WindowSize.cx = WindowSizeX*SCR_FONTSIZE(ScreenInfo).X;
                WindowSize.cy = WindowSizeY*SCR_FONTSIZE(ScreenInfo).Y;
            } else {
                WindowSize.cx = WindowSizeX;
                WindowSize.cy = WindowSizeY;
            }
            WindowSize.cx += VerticalClientToWindow;
            WindowSize.cy += HorizontalClientToWindow;

            if (WindowSizeY != 0) {
                if (!ScreenInfo->WindowMaximizedX) {
                    WindowSize.cy += HorizontalScrollSize;
                }
                if (!ScreenInfo->WindowMaximizedY) {
                    WindowSize.cx += VerticalScrollSize;
                }
            }

            Console->WindowRect.right = Console->WindowRect.left + WindowSize.cx;
            Console->WindowRect.bottom = Console->WindowRect.top + WindowSize.cy;

            UpdateWindowSize(Console,ScreenInfo);
            ScreenInfo->ResizingWindow--;
        } else if (Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {
            WriteToScreen(ScreenInfo,&ScreenInfo->Window);
        }
#if defined(FE_IME)
        if ( (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) &&
             (CONSOLE_IS_DBCS_OUTPUTCP(Console)))
        {
            ConsoleImeResizeModeSystemView(Console,Console->CurrentScreenBuffer->Window);
            ConsoleImeResizeCompStrView(Console,Console->CurrentScreenBuffer->Window);
        }
#endif // FE_IME
    }
    return STATUS_SUCCESS;
}

NTSTATUS
SetActiveScreenBuffer(
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
    PSCREEN_INFORMATION OldScreenInfo;
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;

    OldScreenInfo = Console->CurrentScreenBuffer;
    if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {

#if !defined(_X86_)
        if (Console->FullScreenFlags & CONSOLE_FULLSCREEN) {
            return STATUS_INVALID_PARAMETER;
        }
#endif
        Console->CurrentScreenBuffer = ScreenInfo;

        if (Console->FullScreenFlags == 0) {

            //
            // initialize cursor
            //

            ScreenInfo->BufferInfo.TextInfo.CursorOn = FALSE;

            //
            // set font
            //

            SetFont(ScreenInfo);
        }
#if defined(_X86_)
        else if (Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {

            if (!(Console->Flags & CONSOLE_VDM_REGISTERED)) {

                if ( (!(OldScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER)) ||
                     (OldScreenInfo->BufferInfo.TextInfo.ModeIndex!=ScreenInfo->BufferInfo.TextInfo.ModeIndex)) {

                    // set video mode and font
                    SetVideoMode(ScreenInfo);
                }

                //set up cursor

                SetCursorInformationHW(ScreenInfo,
                                       ScreenInfo->BufferInfo.TextInfo.CursorSize,
                                       ScreenInfo->BufferInfo.TextInfo.CursorVisible);
                SetCursorPositionHW(ScreenInfo,
                                    ScreenInfo->BufferInfo.TextInfo.CursorPosition);
            }

        }
#endif
    }
    else {
        Console->CurrentScreenBuffer = ScreenInfo;
    }

    //
    // empty input buffer
    //

    FlushAllButKeys(&Console->InputBuffer);

    if (Console->FullScreenFlags == 0) {

        SetScreenColors(ScreenInfo, ScreenInfo->Attributes,
                        ScreenInfo->PopupAttributes, FALSE);

        //
        // set window size
        //

        SetWindowSize(ScreenInfo);

        //
        // initialize the palette, if we have the focus and we're not fullscreen
        //

        if (!(Console->Flags & CONSOLE_IS_ICONIC) &&
            Console->FullScreenFlags == 0) {
            if (ScreenInfo->hPalette != NULL || OldScreenInfo->hPalette != NULL) {
                HPALETTE hPalette;
                BOOL bReset = FALSE;
                USERTHREAD_USEDESKTOPINFO utudi;

                if (GetCurrentThreadId() != Console->InputThreadInfo->ThreadId) {
                    bReset = TRUE;
                    utudi.hThread = Console->InputThreadInfo->ThreadHandle;
                    utudi.drdRestore.pdeskRestore = NULL;
                    NtUserSetInformationThread(NtCurrentThread(),
                            UserThreadUseDesktop,
                            &utudi, sizeof(utudi));
                }

                if (ScreenInfo->hPalette == NULL) {
                    hPalette = Console->hSysPalette;
                } else {
                    hPalette = ScreenInfo->hPalette;
                }
                SelectPalette(Console->hDC,
                                 hPalette,
                                 FALSE);
                SetActivePalette(ScreenInfo);

                if (bReset == TRUE) {
                    utudi.hThread = NULL;
                    NtUserSetInformationThread(NtCurrentThread(),
                            UserThreadUseDesktop, &utudi, sizeof(utudi));
                }
            }
        }
    }

#if defined(FE_IME)
    SetUndetermineAttribute(Console);
#endif
    //
    // write data to screen
    //

    ScreenInfo->BufferInfo.TextInfo.Flags &= ~TEXT_VALID_HINT;
    WriteToScreen(ScreenInfo,&ScreenInfo->Window);
    return STATUS_SUCCESS;
}

VOID
SetProcessFocus(
    IN PCSR_PROCESS Process,
    IN BOOL Foreground
    )
{
    if (Foreground) {
        CsrSetForegroundPriority(Process);
    } else {
        CsrSetBackgroundPriority(Process);
    }
}

VOID
SetProcessForegroundRights(
    IN PCSR_PROCESS Process,
    IN BOOL Foreground
    )
{
    USERTHREAD_FLAGS Flags;

    Flags.dwMask  = (W32PF_ALLOWSETFOREGROUND | W32PF_CONSOLEHASFOCUS);
    Flags.dwFlags = (Foreground ? (W32PF_ALLOWSETFOREGROUND | W32PF_CONSOLEHASFOCUS) : 0);

    NtUserSetInformationProcess(Process->ProcessHandle, UserProcessFlags, &Flags, sizeof(Flags));
}

VOID
ModifyConsoleProcessFocus(
    IN PCONSOLE_INFORMATION Console,
    IN BOOL Foreground
    )
{
    PCONSOLE_PROCESS_HANDLE ProcessHandleRecord;
    PLIST_ENTRY ListHead, ListNext;

    ListHead = &Console->ProcessHandleList;
    ListNext = ListHead->Flink;
    while (ListNext != ListHead) {
        ProcessHandleRecord = CONTAINING_RECORD( ListNext, CONSOLE_PROCESS_HANDLE, ListLink );
        ListNext = ListNext->Flink;
        {
            SetProcessFocus(ProcessHandleRecord->Process, Foreground);
            SetProcessForegroundRights(ProcessHandleRecord->Process, Foreground);
        }
    }
}

VOID
TrimConsoleWorkingSet(
    IN PCONSOLE_INFORMATION Console
    )
{
    PCONSOLE_PROCESS_HANDLE ProcessHandleRecord;
    PLIST_ENTRY ListHead, ListNext;

    ListHead = &Console->ProcessHandleList;
    ListNext = ListHead->Flink;
    while (ListNext != ListHead) {
        ProcessHandleRecord = CONTAINING_RECORD( ListNext, CONSOLE_PROCESS_HANDLE, ListLink );
        ListNext = ListNext->Flink;
        {
            SetProcessWorkingSetSize(ProcessHandleRecord->Process->ProcessHandle,(SIZE_T)-1,(SIZE_T)-1);
        }
    }
}

NTSTATUS
QueueConsoleMessage(
    PCONSOLE_INFORMATION Console,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    This inserts a message into the console's message queue and wakes up
    the console input thread to process it.

Arguments:

    Console - Pointer to console information structure.

    Message - Message to store in queue.

    wParam - wParam to store in queue.

    lParam - lParam to store in queue.

Return Value:

    NTSTATUS - STATUS_SUCCESS if everything is OK.

--*/

{
    PCONSOLE_MSG pConMsg;

    ASSERT(ConsoleLocked(Console));

    pConMsg = (PCONSOLE_MSG)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ), sizeof(CONSOLE_MSG));
    if (pConMsg == NULL) {
        return STATUS_NO_MEMORY;
    }

    pConMsg->Message = Message;
    pConMsg->wParam = wParam;
    pConMsg->lParam = lParam;

    InsertHeadList(&Console->MessageQueue, &pConMsg->ListLink);

    if (!PostMessage(Console->hWnd, CM_CONSOLE_MSG, 0, 0)) {
        RemoveEntryList(&pConMsg->ListLink);
        ConsoleHeapFree(pConMsg);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

BOOL
UnqueueConsoleMessage(
    PCONSOLE_INFORMATION Console,
    UINT *pMessage,
    WPARAM *pwParam,
    LPARAM *plParam
    )

/*++

Routine Description:

    This routine removes a message from the console's message queue.

Arguments:

    Console - Pointer to console information structure.

    pMessage - Pointer in which to return Message.

    pwParam - Pointer in which to return wParam.

    plParam - Pointer in which to return lParam.

Return Value:

    BOOL - TRUE if message was found and FALSE otherwise.

--*/

{
    PLIST_ENTRY pEntry;
    PCONSOLE_MSG pConMsg = NULL;

    ASSERT(ConsoleLocked(Console));

    if (IsListEmpty(&Console->MessageQueue)) {
        return FALSE;
    }

    pEntry = RemoveTailList(&Console->MessageQueue);
    pConMsg = CONTAINING_RECORD(pEntry, CONSOLE_MSG, ListLink);

    *pMessage = pConMsg->Message;
    *pwParam = pConMsg->wParam;
    *plParam = pConMsg->lParam;

    ConsoleHeapFree(pConMsg);

    return TRUE;
}

VOID
CleanupConsoleMessages(
    PCONSOLE_INFORMATION Console
    )

/*++

Routine Description:

    This routine cleans up any console messages still in the queue.

Arguments:

    Console - Pointer to console information structure.

Return Value:

    none.

--*/

{
    UINT Message;
    WPARAM wParam;
    LPARAM lParam;

    while (UnqueueConsoleMessage(Console, &Message, &wParam, &lParam)) {
        switch (Message) {
        case CM_MODE_TRANSITION:
            NtSetEvent((HANDLE)lParam, NULL);
            NtClose((HANDLE)lParam);
            break;
        case CM_SET_IME_CODEPAGE:
        case CM_SET_NLSMODE:
        case CM_GET_NLSMODE:
            if (wParam) {
                NtSetEvent((HANDLE)wParam, NULL);
                NtClose((HANDLE)wParam);
            }
            break;
        default:
            KdPrint(("CONSRV: CleanupConsoleMessages - unknown message %x\n", Message));
            ASSERT(FALSE);
            break;
        }
    }
}

VOID
AbortCreateConsole(
    IN PCONSOLE_INFORMATION Console
    )
{
    //
    // Signal any process waiting for us that initialization failed
    //

    NtSetEvent(Console->InitEvents[INITIALIZATION_FAILED],NULL);

    //
    // Now clean up the console structure
    //

    CloseHandle(Console->ClientThreadHandle);
    FreeInputBuffer(&Console->InputBuffer);
    ConsoleHeapFree(Console->Title);
    ConsoleHeapFree(Console->OriginalTitle);
    NtClose(Console->InitEvents[INITIALIZATION_SUCCEEDED]);
    NtClose(Console->InitEvents[INITIALIZATION_FAILED]);
    NtClose(Console->TerminationEvent);
    FreeAliasBuffers(Console);
    FreeCommandHistoryBuffers(Console);
#if defined(FE_SB)
    FreeLocalEUDC(Console);
    DestroyFontCache(Console->FontCacheInformation);
#endif
    DestroyConsole(Console);
}

VOID
DestroyWindowsWindow(
    IN PCONSOLE_INFORMATION Console
    )
{
    PSCREEN_INFORMATION Cur,Next;
    HWND hWnd = Console->hWnd;

    gnConsoleWindows--;
    Console->InputThreadInfo->WindowCount--;

    SetWindowConsole(hWnd, NULL);

    KillTimer(Console->hWnd,CURSOR_TIMER);

    if (Console->hWndProperties) {
        SendMessage(Console->hWndProperties, WM_CLOSE, 0, 0);
    }

    // FE_SB
    if (Console->FonthDC) {
        ReleaseDC(NULL, Console->FonthDC);
        DeleteObject(Console->hBitmap);
    }
    DeleteEUDC(Console);

    // FE_IME
    if (CONSOLE_IS_IME_ENABLED()) {
        if (!(Console->Flags & CONSOLE_NO_WINDOW)) {
            // v-HirShi Jul.4.1995 For console IME
            KillTimer(Console->hWnd, SCROLL_WAIT_TIMER);
        }
        ConsoleImeMessagePump(Console,
                              CONIME_DESTROY,
                              (WPARAM)Console->ConsoleHandle,
                              (LPARAM)NULL
                             );
    }
    // end FE_IME
    // end FE_SB

    CleanupConsoleMessages(Console);

    ReleaseDC(NULL, Console->hDC);
    Console->hDC = NULL;

    DestroyWindow(Console->hWnd);
    Console->hWnd = NULL;

    //
    // Tell the worker thread that the window is destroyed.
    //

    ReplyMessage(0);

    //
    // Clear out any keyboard messages we have stored away.
    //

    ClearKeyInfo(hWnd);

    if (Console->hIcon != NULL && Console->hIcon != ghDefaultIcon) {
        DestroyIcon(Console->hIcon);
    }
    if (Console->hSmIcon != NULL && Console->hSmIcon != ghDefaultSmIcon) {
        DestroyIcon(Console->hSmIcon);
    }

    //
    // must keep this thread handle around until after the destroywindow
    // call so that impersonation will work.
    //

    CloseHandle(Console->ClientThreadHandle);

    //
    // once the sendmessage returns, there will be no more input to
    // the console so we don't need to lock it.
    // also, we've freed the console handle, so no apis may access the console.
    //

    //
    // free screen buffers
    //

    for (Cur=Console->ScreenBuffers;Cur!=NULL;Cur=Next) {
        Next = Cur->Next;
        FreeScreenBuffer(Cur);
    }

    FreeAliasBuffers(Console);
    FreeCommandHistoryBuffers(Console);

    //
    // free input buffer
    //

    FreeInputBuffer(&Console->InputBuffer);
    ConsoleHeapFree(Console->Title);
    ConsoleHeapFree(Console->OriginalTitle);
    NtClose(Console->InitEvents[INITIALIZATION_SUCCEEDED]);
    NtClose(Console->InitEvents[INITIALIZATION_FAILED]);
    NtClose(Console->TerminationEvent);
    if (Console->hWinSta != NULL) {
        CloseDesktop(Console->hDesk);
        CloseWindowStation(Console->hWinSta);
    }
    if (Console->VDMProcessHandle)
        CloseHandle(Console->VDMProcessHandle);
    ASSERT(!(Console->Flags & CONSOLE_VDM_REGISTERED));
    /*if (Console->VDMBuffer != NULL) {
        NtUnmapViewOfSection(NtCurrentProcess(),Console->VDMBuffer);
        NtClose(Console->VDMBufferSectionHandle);
    }*/
#if defined(FE_SB)
    FreeLocalEUDC(Console);
    DestroyFontCache(Console->FontCacheInformation);
#endif
    DestroyConsole(Console);
}

VOID
VerticalScroll(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo,
    IN WORD ScrollCommand,
    IN WORD AbsoluteChange
    )
{
    COORD NewOrigin;

    NewOrigin.X = ScreenInfo->Window.Left;
    NewOrigin.Y = ScreenInfo->Window.Top;
    switch (ScrollCommand) {
        case SB_LINEUP:
            NewOrigin.Y--;
            break;
        case SB_LINEDOWN:
            NewOrigin.Y++;
            break;
        case SB_PAGEUP:
#if defined(FE_IME)
// MSKK July.22.1993 KazuM
// Plan of bottom line reservation for console IME.
            if ( ScreenInfo->Console->InputBuffer.ImeMode.Open )
            {
                ASSERT(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER);
                if (!(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER)) {
                    return;
                }
                NewOrigin.Y-=CONSOLE_WINDOW_SIZE_Y(ScreenInfo)-2;
                ScreenInfo->BufferInfo.TextInfo.Flags &= ~TEXT_VALID_HINT;
                ScreenInfo->BufferInfo.TextInfo.Flags |= CONSOLE_CONVERSION_AREA_REDRAW;
            }
            else
#endif // FE_IME
            NewOrigin.Y-=CONSOLE_WINDOW_SIZE_Y(ScreenInfo)-1;
            break;
        case SB_PAGEDOWN:
#if defined(FE_IME)
// MSKK July.22.1993 KazuM
// Plan of bottom line reservation for console IME.
            if ( ScreenInfo->Console->InputBuffer.ImeMode.Open )
            {
                NewOrigin.Y+=CONSOLE_WINDOW_SIZE_Y(ScreenInfo)-2;
                ScreenInfo->BufferInfo.TextInfo.Flags &= ~TEXT_VALID_HINT;
                ScreenInfo->BufferInfo.TextInfo.Flags |= CONSOLE_CONVERSION_AREA_REDRAW;
            }
            else
#endif // FE_IME
            NewOrigin.Y+=CONSOLE_WINDOW_SIZE_Y(ScreenInfo)-1;
            break;
        case SB_THUMBTRACK:
            Console->Flags |= CONSOLE_SCROLLBAR_TRACKING;
            NewOrigin.Y= AbsoluteChange;
            break;
        case SB_THUMBPOSITION:
            UnblockWriteConsole(Console, CONSOLE_SCROLLBAR_TRACKING);
            NewOrigin.Y= AbsoluteChange;
            break;
        case SB_TOP:
            NewOrigin.Y=0;
            break;
        case SB_BOTTOM:
            NewOrigin.Y=(WORD)(ScreenInfo->ScreenBufferSize.Y-CONSOLE_WINDOW_SIZE_Y(ScreenInfo));
            break;

        default:
            return;
    }

    NewOrigin.Y = (WORD)(max(0,min((SHORT)NewOrigin.Y,
                            (SHORT)ScreenInfo->ScreenBufferSize.Y-(SHORT)CONSOLE_WINDOW_SIZE_Y(ScreenInfo))));
    SetWindowOrigin(ScreenInfo,
                    (BOOLEAN)TRUE,
                    NewOrigin
                   );
}

VOID
HorizontalScroll(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN WORD ScrollCommand,
    IN WORD AbsoluteChange
    )
{
    COORD NewOrigin;

    NewOrigin.X = ScreenInfo->Window.Left;
    NewOrigin.Y = ScreenInfo->Window.Top;
    switch (ScrollCommand) {
        case SB_LINEUP:
            NewOrigin.X--;
            break;
        case SB_LINEDOWN:
            NewOrigin.X++;
            break;
        case SB_PAGEUP:
            NewOrigin.X-=CONSOLE_WINDOW_SIZE_X(ScreenInfo)-1;
            break;
        case SB_PAGEDOWN:
            NewOrigin.X+=CONSOLE_WINDOW_SIZE_X(ScreenInfo)-1;
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            NewOrigin.X= AbsoluteChange;
            break;
        case SB_TOP:
            NewOrigin.X=0;
            break;
        case SB_BOTTOM:
            NewOrigin.X=(WORD)(ScreenInfo->ScreenBufferSize.X-CONSOLE_WINDOW_SIZE_X(ScreenInfo));
            break;

        default:
            return;
    }

    NewOrigin.X = (WORD)(max(0,min((SHORT)NewOrigin.X,
                            (SHORT)ScreenInfo->ScreenBufferSize.X-(SHORT)CONSOLE_WINDOW_SIZE_X(ScreenInfo))));
    SetWindowOrigin(ScreenInfo,
                    (BOOLEAN)TRUE,
                    NewOrigin
                   );
}

LRESULT APIENTRY
ConsoleWindowProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HDC hDC;
    PAINTSTRUCT ps;
    PCONSOLE_INFORMATION Console;
    PSCREEN_INFORMATION ScreenInfo;
    SMALL_RECT PaintRect;
    LRESULT Status;

    Console = GetWindowConsole(hWnd);
    if (Console != NULL) {

        //
        // Set up our thread so we can impersonate the client
        // while processing the message.
        //

        CSR_SERVER_QUERYCLIENTTHREAD()->ThreadHandle =
                Console->ClientThreadHandle;

        //
        // If the console is terminating, don't bother processing messages
        // other than CM_DESTROY_WINDOW.
        //
        if (Console->Flags & CONSOLE_TERMINATING) {
            LockConsole(Console);
            DestroyWindowsWindow(Console);
            return 0;
        }

        //
        // Make sure the console pointer is still valid
        //
        ASSERT(NT_SUCCESS(ValidateConsole(Console)));

        LockConsole(Console);

        ScreenInfo = Console->CurrentScreenBuffer;
    }
    try {
        if (Console == NULL || ScreenInfo == NULL) {
            switch (Message) {
            case WM_GETMINMAXINFO:
                {
                //
                // createwindow issues a WM_GETMINMAXINFO
                // message before we have the windowlong set up
                // with the console pointer.  we need to allow
                // the created window to be bigger than the
                // default size by the scroll size.
                //

                LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
                lpmmi->ptMaxTrackSize.y += HorizontalScrollSize;
                lpmmi->ptMaxTrackSize.x += VerticalScrollSize;
                }
                break;

            default:
                goto CallDefWin;
            }
        } else if (Message == ProgmanHandleMessage && lParam==0) {
            // NOTE: lParam will be 0 if progman is sending it and
            // 1 if console is sending it.
            Status = 0;
            // this is a workaround for a progman bug.  progman
            // sends a progmanhandlemessage twice for each window
            // in the system each time one is requested (for one window).
            if ((HWND)wParam != hWnd && Console->bIconInit) {
                ATOM App,Topic;
                CHAR szItem[ITEM_MAX_SIZE+1];
                PCHAR lpItem;
                ATOM aItem;
                HANDLE ConsoleHandle;

                if (!(Console->Flags & CONSOLE_TERMINATING)) {
                    ConsoleHandle = Console->ConsoleHandle;
                    Console->hWndProgMan = (HWND)wParam;
                    UnlockConsole(Console);
                    App = GlobalAddAtomA("Shell");
                    Topic = GlobalAddAtomA("AppIcon");
                    SendMessage(Console->hWndProgMan,
                                WM_DDE_INITIATE,
                                (WPARAM)hWnd,
                                MAKELONG(App,Topic)
                               );

                    // if console is still valid, continue getting icon.
                    Status = RevalidateConsole(ConsoleHandle, &Console);
                    if (NT_SUCCESS(Status)) {
                        Console->bIconInit = FALSE;
                        lpItem = _itoa((int)Console->iIconId,szItem,10);
                        aItem = GlobalAddAtomA(lpItem);
                        PostMessage(Console->hWndProgMan,
                          WM_DDE_REQUEST,
                          (WPARAM)hWnd,
                          MAKELONG(CF_TEXT,aItem)
                         );
                    }
                }
            }
        } else {
            Status = 0;
            switch (Message) {
            case WM_DROPFILES:
                DoDrop (wParam,Console);
                break;
            case WM_MOVE:
                if (!IsIconic(hWnd)) {
                    PositionConsoleWindow(Console, (Console->WindowRect.left == CW_USEDEFAULT));
#if defined(FE_IME)
                    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
                    {
                        ConsoleImeResizeModeSystemView(Console,ScreenInfo->Window);
                        ConsoleImeResizeCompStrView(Console,ScreenInfo->Window);
                    }
#endif // FE_IME
                }
                break;
            case WM_SIZE:

                if (wParam != SIZE_MINIMIZED) {

                    //
                    // both SetWindowPos and SetScrollRange cause WM_SIZE
                    // messages to be issued.  ignore them if we have already
                    // figured out what size the window should be.
                    //

                    if (!ScreenInfo->ResizingWindow) {
#ifdef THERESES_DEBUG
DbgPrint("WM_SIZE message ");
if (wParam == SIZEFULLSCREEN)
   DbgPrint("SIZEFULLSCREEN\n");
else if (wParam == SIZENORMAL)
   DbgPrint("SIZENORMAL\n");
DbgPrint("  WindowSize is %d %d\n",CONSOLE_WINDOW_SIZE_X(ScreenInfo),CONSOLE_WINDOW_SIZE_Y(ScreenInfo));
#endif
                        ScreenInfo->WindowMaximized = (wParam == SIZE_MAXIMIZED);

                        if (Console->ResizeFlags & SCREEN_BUFFER_CHANGE) {
                            UpdateWindowSize(Console,ScreenInfo);
                        }
                        PositionConsoleWindow(Console, (Console->WindowRect.left == CW_USEDEFAULT));
#if defined(FE_IME)
                        if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
                        {
                            ConsoleImeResizeModeSystemView(Console,ScreenInfo->Window);
                            ConsoleImeResizeCompStrView(Console,ScreenInfo->Window);
                        }
#endif // FE_IME
#ifdef THERESES_DEBUG
DbgPrint("  WindowRect is now %d %d %d %d\n",Console->WindowRect.left,
                                         Console->WindowRect.top,
                                         Console->WindowRect.right,
                                         Console->WindowRect.bottom);
#endif
                        if (Console->ResizeFlags & SCROLL_BAR_CHANGE) {
                            InternalUpdateScrollBars(ScreenInfo);
                            Console->ResizeFlags &= ~SCROLL_BAR_CHANGE;
                        }
                    }
                } else {

                    //
                    // Console is going iconic. Trim working set of all
                    // processes in the console
                    //

                    TrimConsoleWorkingSet(Console);

                }

                break;
            case WM_DDE_ACK:
                if (Console->bIconInit) {
                    Console->hWndProgMan = (HWND)wParam;
                }
                break;
            case WM_DDE_DATA:
                {
                DDEDATA *lpDDEData;
                LPPMICONDATA lpIconData;
                HICON hIcon;
                HANDLE hDdeData;
                BOOL bRelease;
                WPARAM atomTemp;

                UnpackDDElParam(WM_DDE_DATA, lParam, (WPARAM *)&hDdeData, &atomTemp);

                if (hDdeData == NULL) {
                    break;
                }
                lpDDEData = (DDEDATA *)GlobalLock(hDdeData);
                ASSERT(lpDDEData->cfFormat == CF_TEXT);
                lpIconData = (LPPMICONDATA)lpDDEData->Value;
                hIcon = CreateIconFromResourceEx(&lpIconData->iResource,
                        0, TRUE, 0x30000, 0, 0, LR_DEFAULTSIZE);
                if (hIcon) {
                    if (Console->hIcon != NULL && Console->hIcon != ghDefaultIcon) {
                        DestroyIcon(Console->hIcon);
                    }
                    Console->hIcon = hIcon;
                    SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

                    if (Console->hSmIcon != NULL) {
                        if (Console->hSmIcon != ghDefaultSmIcon) {
                            DestroyIcon(Console->hSmIcon);
                        }
                        Console->hSmIcon = NULL;
                        SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)NULL);
                    }
                }

                if (lpDDEData->fAckReq) {

                    PostMessage(Console->hWndProgMan,
                                WM_DDE_ACK,
                                (WPARAM)hWnd,
                                ReuseDDElParam(lParam, WM_DDE_DATA, WM_DDE_ACK, 0x8000, atomTemp));
                }

                bRelease = lpDDEData->fRelease;
                GlobalUnlock(hDdeData);
                if (bRelease){
                    GlobalFree(hDdeData);
                }
                PostMessage(Console->hWndProgMan,
                            WM_DDE_TERMINATE,
                            (WPARAM)hWnd,
                            0
                           );
                if (Console->Flags & CONSOLE_IS_ICONIC) {
                    // force repaint of icon
                    InvalidateRect(hWnd, NULL, TRUE);
                }
                }
                break;
            case WM_ACTIVATE:

                //
                // if we're activated by a mouse click, remember it so
                // we don't pass the click on to the app.
                //

                if (LOWORD(wParam) == WA_CLICKACTIVE) {
                    Console->Flags |= CONSOLE_IGNORE_NEXT_MOUSE_INPUT;
                }
                goto CallDefWin;
                break;
            case WM_DDE_TERMINATE:
                break;
                // FE_IME
            case CM_CONIME_KL_ACTIVATE:
                ActivateKeyboardLayout((HKL)wParam, KLF_SETFORPROCESS);
                break;
            case WM_INPUTLANGCHANGEREQUEST:
                if (CONSOLE_IS_IME_ENABLED()) {
                    ULONG ConimeMessage;
                    LRESULT lResult;

                    if (wParam & INPUTLANGCHANGE_BACKWARD) {
                        ConimeMessage = CONIME_INPUTLANGCHANGEREQUESTBACKWARD;
                    }
                    else if (wParam & INPUTLANGCHANGE_FORWARD) {
                        ConimeMessage = CONIME_INPUTLANGCHANGEREQUESTFORWARD;
                    }
                    else {
                        ConimeMessage = CONIME_INPUTLANGCHANGEREQUEST;
                    }

                    if (!NT_SUCCESS(ConsoleImeMessagePumpWorker(Console,
                                              ConimeMessage,
                                              (WPARAM)Console->ConsoleHandle,
                                              (LPARAM)lParam,
                                              &lResult)) ||
                            !lResult) {

                        break;
                    }
                }
#ifdef LATER
                else if (IS_IME_KBDLAYOUT(lParam)) {
                    // IME keyboard layout should be avoided
                    // if the console is not IME enabled.
                    break;
                }
                // Call the default window proc and let it handle
                // the keyboard layout activation.
#endif
                goto CallDefWin;

                break;
                // end FE_IME

            case WM_INPUTLANGCHANGE:
                Console->hklActive = (HKL)lParam;
                // FE_IME
                if (CONSOLE_IS_IME_ENABLED()) {
                    if (!NT_SUCCESS(ConsoleImeMessagePump(Console,
                                          CONIME_INPUTLANGCHANGE,
                                          (WPARAM)Console->ConsoleHandle,
                                          (LPARAM)Console->hklActive
                                         ))) {
                        break;
                    }
                    else{
                        GetImeKeyState(Console, NULL) ;
                    }
                }
                // end FE_IME
                goto CallDefWin;

                break;

            case WM_SETFOCUS:
                ModifyConsoleProcessFocus(Console, TRUE);
                SetConsoleReserveKeys(hWnd, Console->ReserveKeys);
                Console->Flags |= CONSOLE_HAS_FOCUS;

                SetTimer(hWnd, CURSOR_TIMER, guCaretBlinkTime, NULL);
                HandleFocusEvent(Console,TRUE);
                if (!Console->hklActive) {
                    SystemParametersInfo(SPI_GETDEFAULTINPUTLANG, 0, &Console->hklActive, FALSE);
                    GetNonBiDiKeyboardLayout(&Console->hklActive);
                }
                ActivateKeyboardLayout(Console->hklActive, 0);
                // FE_IME
                if (CONSOLE_IS_IME_ENABLED()) {
                    // v-HirShi Sep.15.1995 Support Console IME
                    if (!NT_SUCCESS(ConsoleImeMessagePump(Console,
                                          CONIME_SETFOCUS,
                                          (WPARAM)Console->ConsoleHandle,
                                          (LPARAM)Console->hklActive
                                         ))) {
                        break;
                    }

                    if (Console->InputBuffer.hWndConsoleIME)
                    {
                        /*
                         * open property window by ImmConfigureIME
                         * never set focus on console window
                         * so, set focus to property window
                         */
                        HWND hwnd = GetLastActivePopup(Console->InputBuffer.hWndConsoleIME);
                        if (hwnd != NULL)
                            SetForegroundWindow(hwnd);
                    }
                }
                // FE_IME
                break;
            case WM_KILLFOCUS:
                ModifyConsoleProcessFocus(Console, FALSE);
                SetConsoleReserveKeys(hWnd, CONSOLE_NOSHORTCUTKEY);
                Console->Flags &= ~CONSOLE_HAS_FOCUS;

                if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
                    ConsoleHideCursor(ScreenInfo);
                    ScreenInfo->BufferInfo.TextInfo.UpdatingScreen -= 1; // counteract HideCursor
                }
                KillTimer(hWnd, CURSOR_TIMER);
                HandleFocusEvent(Console,FALSE);

                // FE_IME
                if (CONSOLE_IS_IME_ENABLED()) {
                    // v-HirShi Sep.16.1995 Support Console IME
                    if (!NT_SUCCESS(ConsoleImeMessagePump(Console,
                                          CONIME_KILLFOCUS,
                                          (WPARAM)Console->ConsoleHandle,
                                          (LPARAM)NULL
                                         ))) {
                        break;
                    }
                }
                // end FE_IME
                break;
            case WM_PAINT:

                // ICONIC bit is not set if we're fullscreen and don't
                // have the hardware

                ConsoleHideCursor(ScreenInfo);
                hDC = BeginPaint(hWnd, &ps);
                if (Console->Flags & CONSOLE_IS_ICONIC ||
                    Console->FullScreenFlags == CONSOLE_FULLSCREEN) {
                    RECT rc;
                    UINT cxIcon, cyIcon;
                    GetClientRect(hWnd, &rc);
                    cxIcon = GetSystemMetrics(SM_CXICON);
                    cyIcon = GetSystemMetrics(SM_CYICON);

                    rc.left = (rc.right - cxIcon) >> 1;
                    rc.top = (rc.bottom - cyIcon) >> 1;

                    DrawIcon(hDC, rc.left, rc.top, Console->hIcon);
                } else {
                    if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
                        PaintRect.Left = (SHORT)((ps.rcPaint.left/SCR_FONTSIZE(ScreenInfo).X)+ScreenInfo->Window.Left);
                        PaintRect.Right = (SHORT)((ps.rcPaint.right/SCR_FONTSIZE(ScreenInfo).X)+ScreenInfo->Window.Left);
                        PaintRect.Top = (SHORT)((ps.rcPaint.top/SCR_FONTSIZE(ScreenInfo).Y)+ScreenInfo->Window.Top);
                        PaintRect.Bottom = (SHORT)((ps.rcPaint.bottom/SCR_FONTSIZE(ScreenInfo).Y)+ScreenInfo->Window.Top);
                    } else {
                        PaintRect.Left = (SHORT)(ps.rcPaint.left+ScreenInfo->Window.Left);
                        PaintRect.Right = (SHORT)(ps.rcPaint.right+ScreenInfo->Window.Left);
                        PaintRect.Top = (SHORT)(ps.rcPaint.top+ScreenInfo->Window.Top);
                        PaintRect.Bottom = (SHORT)(ps.rcPaint.bottom+ScreenInfo->Window.Top);
                    }
                    ScreenInfo->BufferInfo.TextInfo.Flags &= ~TEXT_VALID_HINT;
                    WriteToScreen(ScreenInfo,&PaintRect);
                }
                EndPaint(hWnd,&ps);
                ConsoleShowCursor(ScreenInfo);
                break;
            case WM_CLOSE:
                if (!(Console->Flags & CONSOLE_NO_WINDOW) ||
                    !(Console->Flags & CONSOLE_WOW_REGISTERED)) {
                    HandleCtrlEvent(Console,CTRL_CLOSE_EVENT);
                }
                break;
            case WM_ERASEBKGND:

                // ICONIC bit is not set if we're fullscreen and don't
                // have the hardware

                if (Console->Flags & CONSOLE_IS_ICONIC ||
                    Console->FullScreenFlags == CONSOLE_FULLSCREEN) {
                    Message = WM_ICONERASEBKGND;
                    goto CallDefWin;
                }
                break;
            case WM_SETTINGCHANGE:
                /*
                 * See if the caret blink time was changed. If so, reset the
                 * timer.
                 */
                if (wParam == SPI_SETKEYBOARDDELAY) {
                    guCaretBlinkTime = GetCaretBlinkTime();
                    if (Console->Flags & CONSOLE_HAS_FOCUS) {
                        KillTimer(hWnd, CURSOR_TIMER);
                        SetTimer(hWnd, CURSOR_TIMER, guCaretBlinkTime, NULL);
                    }
                } else {
                    gfInitSystemMetrics = TRUE;
                }
                break;
            case WM_DISPLAYCHANGE:
                gfInitSystemMetrics = TRUE;
                break;
            case WM_SETCURSOR:
                if (lParam == -1) {

                    //
                    // the app changed the cursor visibility or shape.
                    // see if the cursor is in the client area.
                    //

                    POINT Point;
                    HWND hWndTmp;
                    GetCursorPos(&Point);
                    hWndTmp = WindowFromPoint(Point);
                    if (hWndTmp == hWnd) {
                        lParam = DefWindowProc(hWnd,WM_NCHITTEST,0,MAKELONG((WORD)Point.x, (WORD)Point.y));
                    }
                }
                if ((WORD)lParam == HTCLIENT) {
                    if (ScreenInfo->CursorDisplayCount < 0) {
                        SetCursor(NULL);
                    } else {
                        SetCursor(ScreenInfo->CursorHandle);
                    }
                } else {
                    goto CallDefWin;
                }
                break;
            case WM_GETMINMAXINFO:
                {
                LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
                COORD FontSize;
                WINDOW_LIMITS WindowLimits;

                GetWindowLimits(ScreenInfo, &WindowLimits);
                if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
                    FontSize = SCR_FONTSIZE(ScreenInfo);
                } else {
                    FontSize.X = 1;
                    FontSize.Y = 1;
                }
                lpmmi->ptMaxSize.x = lpmmi->ptMaxTrackSize.x = WindowLimits.MaxWindow.X;
                if (!ScreenInfo->WindowMaximizedY) {
                    lpmmi->ptMaxTrackSize.x += VerticalScrollSize;
                    lpmmi->ptMaxSize.x += VerticalScrollSize;
                }
                while (lpmmi->ptMaxSize.x > WindowLimits.FullScreenSize.X + VerticalClientToWindow) {
                    lpmmi->ptMaxSize.x -= FontSize.X;
                }
                lpmmi->ptMaxSize.y = lpmmi->ptMaxTrackSize.y = WindowLimits.MaxWindow.Y;
                if (!ScreenInfo->WindowMaximizedX) {
                    lpmmi->ptMaxTrackSize.y += HorizontalScrollSize;
                    lpmmi->ptMaxSize.y += HorizontalScrollSize;
                }
                while (lpmmi->ptMaxSize.y > WindowLimits.FullScreenSize.Y + HorizontalClientToWindow) {
                    lpmmi->ptMaxSize.y -= FontSize.Y;
                }
                lpmmi->ptMinTrackSize.x = WindowLimits.MinimumWindowSize.X * FontSize.X + VerticalClientToWindow;
                lpmmi->ptMinTrackSize.y = HorizontalClientToWindow;
                }
                break;
            case WM_QUERYDRAGICON:
                Status = (LRESULT)Console->hIcon;
                break;
            case WM_WINDOWPOSCHANGING:
                // ignore window pos changes if going fullscreen
                if (TRUE || (Console->FullScreenFlags == 0)) {
                    LPWINDOWPOS WindowPos = (LPWINDOWPOS)lParam;
                    DWORD fMinimized;

                    /*
                     * This message is sent before a SetWindowPos() operation
                     * occurs. We use it here to set/clear the CONSOLE_IS_ICONIC
                     * bit appropriately... doing so in the WM_SIZE handler
                     * is incorrect because the WM_SIZE comes after the
                     * WM_ERASEBKGND during SetWindowPos() processing, and the
                     * WM_ERASEBKGND needs to know if the console window is
                     * iconic or not.
                     */
                    fMinimized = IsIconic(hWnd);

                    if (fMinimized) {
                        if (!(Console->Flags & CONSOLE_IS_ICONIC)) {
                            Console->Flags |= CONSOLE_IS_ICONIC;

                            //
                            // if the palette is something other than default,
                            // select the default palette in.  otherwise, the
                            // screen will repaint twice each time the icon
                            // is painted.
                            //

                            if (ScreenInfo->hPalette != NULL &&
                                Console->FullScreenFlags == 0) {
                                SelectPalette(Console->hDC,
                                              Console->hSysPalette,
                                              FALSE);
                                UnsetActivePalette(ScreenInfo);
                            }
                        }
                    } else {
                        if (Console->Flags & CONSOLE_IS_ICONIC) {
                            Console->Flags &= ~CONSOLE_IS_ICONIC;

                            //
                            // if the palette is something other than default,
                            // select the default palette in.  otherwise, the
                            // screen will repaint twice each time the icon
                            // is painted.
                            //

                            if (ScreenInfo->hPalette != NULL &&
                                Console->FullScreenFlags == 0) {
                                SelectPalette(Console->hDC,
                                              ScreenInfo->hPalette,
                                              FALSE);
                                SetActivePalette(ScreenInfo);
                            }
                        }
                    }
                    if (!ScreenInfo->ResizingWindow &&
                        (WindowPos->cx || WindowPos->cy) &&
                        !fMinimized) {
                        ProcessResizeWindow(ScreenInfo,Console,WindowPos);
                    }
                }
                break;
            case WM_CONTEXTMENU:
                if (DefWindowProc(hWnd, WM_NCHITTEST, 0, lParam) == HTCLIENT) {
                    TrackPopupMenuEx(Console->hHeirMenu,
                                     TPM_RIGHTBUTTON,
                                     GET_X_LPARAM(lParam),
                                     GET_Y_LPARAM(lParam),
                                     hWnd,
                                     NULL);
                } else {
                    goto CallDefWin;
                }
                break;
            case WM_NCLBUTTONDOWN:
                // allow user to move window even when bigger than the screen
                switch (wParam & 0x00FF) {
                    case HTCAPTION:
                        UnlockConsole(Console);
                        Console = NULL;
                        SetActiveWindow(hWnd);
                        SendMessage(hWnd, WM_SYSCOMMAND,
                                       SC_MOVE | wParam, lParam);
                        break;
                    default:
                        goto CallDefWin;
                }
                break;
#if defined (FE_IME)
// Sep.16.1995 Support Console IME
            case WM_KEYDOWN    +CONIME_KEYDATA:
            case WM_KEYUP      +CONIME_KEYDATA:
            case WM_CHAR       +CONIME_KEYDATA:
            case WM_DEADCHAR   +CONIME_KEYDATA:

            case WM_SYSKEYDOWN +CONIME_KEYDATA:
            case WM_SYSKEYUP   +CONIME_KEYDATA:
            case WM_SYSCHAR    +CONIME_KEYDATA:
            case WM_SYSDEADCHAR+CONIME_KEYDATA:
#endif
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_CHAR:
            case WM_DEADCHAR:
                HandleKeyEvent(Console,hWnd,Message,wParam,lParam);
                break;
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_SYSCHAR:
            case WM_SYSDEADCHAR:
                if (HandleSysKeyEvent(Console,hWnd,Message,wParam,lParam) && Console != NULL) {
                    goto CallDefWin;
                }
                break;
            case WM_COMMAND:
                //
                // If this is an edit command from the context menu, treat
                // it like a sys command.
                //
                if ((wParam < cmCopy) || (wParam > cmSelectAll)) {
                    break;
                }
                // FALL THRU
            case WM_SYSCOMMAND:
                if (wParam >= ScreenInfo->CommandIdLow &&
                    wParam <= ScreenInfo->CommandIdHigh) {
                    HandleMenuEvent(Console,(DWORD)wParam);
                } else if (wParam == cmMark) {
                    DoMark(Console);
                } else if (wParam == cmCopy) {
                    DoCopy(Console);
                } else if (wParam == cmPaste) {
                    DoPaste(Console);
                } else if (wParam == cmScroll) {
                    DoScroll(Console);
                } else if (wParam == cmFind) {
                    DoFind(Console);
                } else if (wParam == cmSelectAll) {
                    DoSelectAll(Console);
                } else if (wParam == cmControl) {
                    PropertiesDlgShow(Console, TRUE);
                } else if (wParam == cmDefaults) {
                    PropertiesDlgShow(Console, FALSE);
                } else {

                    // if we're restoring, remove any
                    // mouse events so app doesn't get them.

                    if (wParam == SC_RESTORE) {
                        MSG RestoreMsg;
                        SetCapture(hWnd);
                        while (GetCapture() != NULL &&
                               (GetKeyState(VK_LBUTTON) & KEY_PRESSED)) {
                            PeekMessage(&RestoreMsg,
                                        hWnd,
                                        WM_MOUSEFIRST,
                                        WM_MOUSELAST,
                                        PM_REMOVE
                                       );
                        }
                        ReleaseCapture();
                    }

                    goto CallDefWin;
                }
                break;
            case WM_TIMER:
#if defined(FE_IME)
                if (wParam == SCROLL_WAIT_TIMER)
                {
                    ASSERT(CONSOLE_IS_IME_ENABLED());
                    if ((ScreenInfo->Console->ConsoleIme.ScrollFlag & (HIDE_FOR_SCROLL)) &&
                        (ScreenInfo->Console->ConsoleIme.ScrollWaitCountDown > 0)
                       ) {
                        if ((ScreenInfo->Console->ConsoleIme.ScrollWaitCountDown -= guCaretBlinkTime) <= 0) {
                            ConsoleImeBottomLineInUse(ScreenInfo);
                        }
                    }
                    break;
                }
#endif // FE_IME
                CursorTimerRoutine(ScreenInfo);
                ScrollIfNecessary(Console,ScreenInfo);
                break;
            case WM_HSCROLL:
                HorizontalScroll(ScreenInfo, LOWORD(wParam), HIWORD(wParam));
                break;
            case WM_VSCROLL:
                VerticalScroll(Console, ScreenInfo,LOWORD(wParam),HIWORD(wParam));
                break;
            case WM_INITMENU:
                HandleMenuEvent(Console,WM_INITMENU);
                InitializeMenu(Console);
                break;
            case WM_MENUSELECT:
                if (HIWORD(wParam) == 0xffff) {
                    HandleMenuEvent(Console,WM_MENUSELECT);
                }
                break;
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MBUTTONDBLCLK:
            case WM_MOUSEWHEEL:
                if (HandleMouseEvent(Console,ScreenInfo,Message,wParam,lParam)) {
                    if (Message != WM_MOUSEWHEEL) {
                        goto CallDefWin;
                    }
                } else {
                    break;
                }

                /*
                 * Don't handle zoom
                 */
                if (wParam & (MK_CONTROL)) {
                    goto CallDefWin;
                }

                Status = 1;
                if (gfInitSystemMetrics) {
                    InitializeSystemMetrics();
                }

                ScreenInfo->WheelDelta -= (short)HIWORD(wParam);
                if (abs(ScreenInfo->WheelDelta) >= WHEEL_DELTA &&
                        gucWheelScrollLines > 0) {

                    COORD   NewOrigin;
                    SHORT   dy;

                    NewOrigin.X = ScreenInfo->Window.Left;
                    NewOrigin.Y = ScreenInfo->Window.Top;

                    /*
                     * Limit a roll of one (1) WHEEL_DELTA to scroll one (1) page.
                     * If in shift scroll mode then scroll one page at a time regardless.
                     */

                    if (!(wParam & MK_SHIFT))
                        dy = (int) min(
                                (UINT) CONSOLE_WINDOW_SIZE_Y(ScreenInfo) - 1,
                                gucWheelScrollLines);
                    else
                        dy = CONSOLE_WINDOW_SIZE_Y(ScreenInfo) - 1;

                    if (dy == 0) {
                        dy++;
                    }

                    dy *= (ScreenInfo->WheelDelta / WHEEL_DELTA);
                    ScreenInfo->WheelDelta %= WHEEL_DELTA;

                    NewOrigin.Y += dy;
                    if (NewOrigin.Y < 0) {
                        NewOrigin.Y = 0;
                    } else if (NewOrigin.Y + CONSOLE_WINDOW_SIZE_Y(ScreenInfo) >
                            ScreenInfo->ScreenBufferSize.Y) {
                        NewOrigin.Y = ScreenInfo->ScreenBufferSize.Y -
                                CONSOLE_WINDOW_SIZE_Y(ScreenInfo);
                    }

                    SetWindowOrigin(ScreenInfo, TRUE, NewOrigin);
                }
                break;

            case WM_PALETTECHANGED:
                if (Console->FullScreenFlags == 0) {
                    if (ScreenInfo->hPalette != NULL) {
                        SetActivePalette(ScreenInfo);
                        if (ScreenInfo->Flags & CONSOLE_GRAPHICS_BUFFER) {
                            WriteRegionToScreenBitMap(ScreenInfo,
                                                      &ScreenInfo->Window);
                        }
                    } else {
                        SetScreenColors(ScreenInfo, ScreenInfo->Attributes,
                                        ScreenInfo->PopupAttributes, TRUE);
                    }
                }
                break;
#if defined(_X86_)
            case WM_FULLSCREEN:

                //
                // This message is sent by the system to tell console that
                // the fullscreen state of a window has changed.
                // In some cases, this message will be sent in response to
                // a call from console to change to fullscreen (Atl-Enter)
                // or may also come directly from the system (switch of
                // focus from a windowed app to a fullscreen app).
                //

                KdPrint(("CONSRV: WindowProc - WM_FULLSCREEN\n"));

                Status = DisplayModeTransition(wParam,Console,ScreenInfo);
#if defined(FE_IME)
                if (NT_SUCCESS(Status)) {
                    Status = ImeWmFullScreen(wParam,Console,ScreenInfo);
                }
#endif // FE_IME
                break;
#endif
            case CM_SET_WINDOW_SIZE:
                if (lParam == 0x47474747) {
                    Status = InternalSetWindowSize(Console,
                                                   (PSCREEN_INFORMATION)wParam,
                                                   &ScreenInfo->Window
                                                   );
                }
                break;
            case CM_BEEP:
                if (lParam == 0x47474747) {
                    Beep(800, 200);
                }
                break;
            case CM_UPDATE_SCROLL_BARS:
                InternalUpdateScrollBars(ScreenInfo);
                break;
            case CM_UPDATE_TITLE:
                SetWindowText(hWnd,Console->Title);
                break;
            case CM_CONSOLE_MSG:
                if (!UnqueueConsoleMessage(Console, &Message, &wParam, &lParam)) {
                    break;
                }
                switch (Message) {
#if defined(_X86_)
                case CM_MODE_TRANSITION:

                    KdPrint(("CONSRV: WindowProc - CM_MODE_TRANSITION\n"));

                    if (wParam == FULLSCREEN) {
                        if (Console->FullScreenFlags == 0) {
                            ConvertToFullScreen(Console);
                            Console->FullScreenFlags |= CONSOLE_FULLSCREEN;
                            ChangeDispSettings(Console, hWnd, CDS_FULLSCREEN);
                        }
                    } else {
                        if (Console->FullScreenFlags & CONSOLE_FULLSCREEN) {
                            ConvertToWindowed(Console);
                            Console->FullScreenFlags &= ~CONSOLE_FULLSCREEN;
                            ChangeDispSettings(Console, hWnd, 0);

                            ShowWindow(hWnd, SW_RESTORE);
                        }
                    }

                    UnlockConsole(Console);
                    Console = NULL;

                    NtSetEvent((HANDLE)lParam, NULL);
                    NtClose((HANDLE)lParam);
                    break;
#endif
#if defined (FE_IME)
                case CM_SET_IME_CODEPAGE: {
                    if (! LOWORD(lParam))
                    {
                        // Input code page
                        Status = SetImeCodePage(Console);
                    }
                    else
                    {
                        // Output code page
                        Status = SetImeOutputCodePage(Console, ScreenInfo, HIWORD(lParam));
                    }

                    if (wParam) {
                        NtSetEvent((HANDLE)wParam, NULL);
                        NtClose((HANDLE)wParam);
                    }
                    break;
                }
                case CM_SET_NLSMODE:
                    Status = SetImeKeyState(Console, ImmConversionFromConsole((DWORD)lParam));
                    if (wParam) {
                        NtSetEvent((HANDLE)wParam, NULL);
                        NtClose((HANDLE)wParam);
                    }
                    break;
                case CM_GET_NLSMODE:
                    if (Console->InputThreadInfo->hWndConsoleIME)
                    {
                        ASSERT(CONSOLE_IS_IME_ENABLED());

                        if (!NT_SUCCESS(GetImeKeyState(Console, NULL))) {
                            if (wParam) {
                                NtSetEvent((HANDLE)wParam, NULL);
                                NtClose((HANDLE)wParam);
                            }
                            break;
                        }
                        if (wParam) {
                            NtSetEvent((HANDLE)wParam, NULL);
                            NtClose((HANDLE)wParam);
                        }
                    }
                    else if (lParam < 10)
                    {
                        /*
                         * try get conversion mode until ready ConIME.
                         */
                        Status = QueueConsoleMessage(Console,
                                    CM_GET_NLSMODE,
                                    wParam,
                                    lParam+1
                                   );
                        if (!NT_SUCCESS(Status)) {
                            if (wParam) {
                                NtSetEvent((HANDLE)wParam, NULL);
                                NtClose((HANDLE)wParam);
                            }
                        }
                    }
                    else
                    {
                        if (wParam) {
                            NtSetEvent((HANDLE)wParam, NULL);
                            NtClose((HANDLE)wParam);
                        }
                    }
                    break;
#endif // FE_IME
                }
                break;

            case CM_HIDE_WINDOW:
                ShowWindowAsync(hWnd, SW_MINIMIZE);
                break;
            case CM_PROPERTIES_START:
                Console->hWndProperties = (HWND)wParam;
                break;
            case CM_PROPERTIES_UPDATE:
                PropertiesUpdate(Console, (HANDLE)wParam);
                break;
            case CM_PROPERTIES_END:
                Console->hWndProperties = NULL;
                break;
#if defined(FE_IME)
            case WM_COPYDATA:
                if (CONSOLE_IS_IME_ENABLED() && CONSOLE_IS_DBCS_OUTPUTCP(Console)) {
                    Status = ImeControl(Console,(HWND)wParam,(PCOPYDATASTRUCT)lParam);
                }
                break;
// v-HirShi Sep.18.1995 Support Console IME
            case WM_ENTERMENULOOP:
                if (Console->Flags & CONSOLE_HAS_FOCUS) {
                    Console->InputBuffer.ImeMode.Unavailable = TRUE;
                    if (CONSOLE_IS_IME_ENABLED()) {
                        if (!NT_SUCCESS(ConsoleImeMessagePump(Console,
                                              CONIME_KILLFOCUS,
                                              (WPARAM)Console->ConsoleHandle,
                                              (LPARAM)NULL
                                             ))) {
                            break;
                        }
                    }
                }
                break;

            case WM_EXITMENULOOP:
                if (Console->Flags & CONSOLE_HAS_FOCUS) {
                    if (CONSOLE_IS_IME_ENABLED()) {
                        if (!NT_SUCCESS(ConsoleImeMessagePump(Console,
                                              CONIME_SETFOCUS,
                                              (WPARAM)Console->ConsoleHandle,
                                              (LPARAM)Console->hklActive
                                             ))) {
                            break;
                        }
                    }
                    Console->InputBuffer.ImeMode.Unavailable = FALSE;
                }
                break;

            case WM_ENTERSIZEMOVE:
                if (Console->Flags & CONSOLE_HAS_FOCUS) {
                    Console->InputBuffer.ImeMode.Unavailable = TRUE;
                }
                break;

            case WM_EXITSIZEMOVE:
                if (Console->Flags & CONSOLE_HAS_FOCUS) {
                    Console->InputBuffer.ImeMode.Unavailable = FALSE;
                }
                break;
#endif // FE_IME
CallDefWin:
            default:
                if (Console != NULL) {
                    UnlockConsole(Console);
                    Console = NULL;
                }
                Status = (DefWindowProc(hWnd,Message,wParam,lParam));
                break;
            }
        }
    } finally {
        if (Console != NULL) {
            UnlockConsole(Console);
            Console = NULL;
        }
    }

    return Status;
}


/*
* Drag and Drop support functions for console window
*/

/*++

Routine Description:

    This routine retrieves the filenames of dropped files. It was copied from
    shelldll API DragQueryFile. We didn't use DragQueryFile () because we don't
    want to load Shell32.dll in CSR

Arguments:
    Same as DragQueryFile

Return Value:


--*/
UINT ConsoleDragQueryFile(
    IN HANDLE hDrop,
    IN PVOID lpFile,
    IN UINT cb
    )
{
    UINT i = 0;
    LPDROPFILESTRUCT lpdfs;
    BOOL fWide;

    lpdfs = (LPDROPFILESTRUCT)GlobalLock(hDrop);

    if (lpdfs && lpdfs != hDrop)
    {
        try {
            fWide = (LOWORD(lpdfs->pFiles) == sizeof(DROPFILES));
            if (fWide)
            {
                //
                // This is a new (NT-compatible) HDROP
                //
                fWide = lpdfs->fWide;       // Redetermine fWide from struct
                                            // since it is present.
            }

            if (fWide)
            {
                LPWSTR lpList;

                //
                // UNICODE HDROP
                //

                lpList = (LPWSTR)((LPBYTE)lpdfs + lpdfs->pFiles);

                i = lstrlenW(lpList);

                if (!i)
                    goto Exit;

                cb--;
                if (cb < i)
                    i = cb;

                lstrcpynW((LPWSTR)lpFile, lpList, i + 1);
            }
            else
            {
                LPSTR lpList;

                //
                // This is Win31-style HDROP or an ANSI NT Style HDROP
                //
                lpList = (LPSTR)((LPBYTE)lpdfs + lpdfs->pFiles);

                i = lstrlenA(lpList);

                if (!i)
                    goto Exit;

                cb--;
                if (cb < i)
                    i = cb;

                MultiByteToWideChar(CP_ACP, 0, lpList, -1, (LPWSTR)lpFile, cb);

            }
        } except( EXCEPTION_EXECUTE_HANDLER ) {
           KdPrint(("CONSRV: WM_DROPFILES raised exception\n"));
           i = 0;
        }
Exit:
        GlobalUnlock(hDrop);
        GlobalFree(hDrop);
    }

    return(i);
}




/*++

Routine Description:

    This routine is called when ConsoleWindowProc receives a WM_DROPFILES
    message. It initially calls ConsoleDragQueryFile() to calculate the number
    of files dropped and then ConsoleDragQueryFile() is called
    to retrieve the filename. DoStringPaste() pastes the filename to the console
    window

Arguments:
    wParam  -   Identifies the structure containing the filenames of the
                dropped files.
    Console -   Pointer to CONSOLE_INFORMATION structure


Return Value:
    None


--*/
void DoDrop (WPARAM wParam,
             PCONSOLE_INFORMATION Console)
{
    WCHAR  szPath[MAX_PATH];
    BOOL fAddQuotes;

    if (ConsoleDragQueryFile((HANDLE)wParam, szPath, CharSizeOf(szPath))) {
        fAddQuotes = (wcschr(szPath, L' ') != NULL);
        if (fAddQuotes)
            DoStringPaste(Console, L"\"", 1);
        DoStringPaste(Console, szPath, wcslen(szPath));
        if (fAddQuotes)
            DoStringPaste(Console, L"\"", 1);
    }
}

BOOL
CreateDbcsScreenBuffer(
    IN PCONSOLE_INFORMATION Console,
    IN COORD dwScreenBufferSize,
    OUT PDBCS_SCREEN_BUFFER DbcsScreenBuffer
    )
{
    if (CONSOLE_IS_DBCS_OUTPUTCP(Console)) {
        DbcsScreenBuffer->TransBufferCharacter =
            (PWCHAR)ConsoleHeapAlloc(
                              MAKE_TAG( SCREEN_DBCS_TAG ),
                              (dwScreenBufferSize.X*dwScreenBufferSize.Y*sizeof(WCHAR))+sizeof(WCHAR));
        if (DbcsScreenBuffer->TransBufferCharacter == NULL) {
            return FALSE;
        }

        DbcsScreenBuffer->TransBufferAttribute =
            (PBYTE)ConsoleHeapAlloc(
                             MAKE_TAG( SCREEN_DBCS_TAG ),
                             (dwScreenBufferSize.X*dwScreenBufferSize.Y*sizeof(BYTE))+sizeof(BYTE));
        if (DbcsScreenBuffer->TransBufferAttribute == NULL) {
            ConsoleHeapFree(DbcsScreenBuffer->TransBufferCharacter);
            return FALSE;
        }

        DbcsScreenBuffer->TransWriteConsole =
            (PWCHAR)ConsoleHeapAlloc(
                              MAKE_TAG( SCREEN_DBCS_TAG ),
                              (dwScreenBufferSize.X*dwScreenBufferSize.Y*sizeof(WCHAR))+sizeof(WCHAR));
        if (DbcsScreenBuffer->TransWriteConsole == NULL) {
            ConsoleHeapFree(DbcsScreenBuffer->TransBufferAttribute);
            ConsoleHeapFree(DbcsScreenBuffer->TransBufferCharacter);
            return FALSE;
        }

        DbcsScreenBuffer->KAttrRows =
            (PBYTE)ConsoleHeapAlloc(
                             MAKE_TAG( SCREEN_DBCS_TAG ),
                             dwScreenBufferSize.X*dwScreenBufferSize.Y*sizeof(BYTE));
        if (DbcsScreenBuffer->KAttrRows == NULL) {
            ConsoleHeapFree(DbcsScreenBuffer->TransWriteConsole);
            ConsoleHeapFree(DbcsScreenBuffer->TransBufferAttribute);
            ConsoleHeapFree(DbcsScreenBuffer->TransBufferCharacter);
            return FALSE;
        }
    }
    else {
        DbcsScreenBuffer->TransBufferCharacter = NULL;
        DbcsScreenBuffer->TransBufferAttribute = NULL;
        DbcsScreenBuffer->TransWriteConsole = NULL;
        DbcsScreenBuffer->KAttrRows = NULL;
    }

    return TRUE;
}

BOOL
DeleteDbcsScreenBuffer(
    IN PDBCS_SCREEN_BUFFER DbcsScreenBuffer
    )
{
    if (DbcsScreenBuffer->KAttrRows) {
        ConsoleHeapFree(DbcsScreenBuffer->TransBufferCharacter);
        ConsoleHeapFree(DbcsScreenBuffer->TransBufferAttribute);
        ConsoleHeapFree(DbcsScreenBuffer->TransWriteConsole);
        ConsoleHeapFree(DbcsScreenBuffer->KAttrRows);
    }
    return TRUE;
}

BOOL
ReCreateDbcsScreenBufferWorker(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
    SHORT i;
    PBYTE KAttrRowPtr;
    COORD dwScreenBufferSize;
    DBCS_SCREEN_BUFFER NewDbcsScreenBuffer;

    ASSERT(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER);
    if (!(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER)) {
        return FALSE;
    }

    dwScreenBufferSize = ScreenInfo->ScreenBufferSize;

    if (!CreateDbcsScreenBuffer(Console,
            dwScreenBufferSize,
            &NewDbcsScreenBuffer)) {
        return FALSE;
    }

    KAttrRowPtr = NewDbcsScreenBuffer.KAttrRows;
    for (i = 0; i < dwScreenBufferSize.Y; i++) {
        ScreenInfo->BufferInfo.TextInfo.Rows[i].CharRow.KAttrs = KAttrRowPtr;
        if (KAttrRowPtr) {
            RtlZeroMemory(KAttrRowPtr, dwScreenBufferSize.X);
            KAttrRowPtr += dwScreenBufferSize.X;
        }
    }
    ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer = NewDbcsScreenBuffer;

    return TRUE;
}


typedef struct _DBCS_SCREEN_BUFFER_TRACKER {
    DBCS_SCREEN_BUFFER data;
#if DBG
    PSCREEN_INFORMATION pScreenInfo;
#endif
} DBCS_SCREEN_BUFFER_TRACKER, *PDBCS_SCREEN_BUFFER_TRACKER;

BOOL
ReCreateDbcsScreenBuffer(
    IN PCONSOLE_INFORMATION pConsole,
    IN UINT OldCodePage)
{
    BOOL fResult = FALSE;
    PDBCS_SCREEN_BUFFER_TRACKER pDbcsScreenBuffer;
    PSCREEN_INFORMATION pScreenInfo;
    UINT nScreen;
    UINT i;
#if DBG
    UINT nScreenSave;
#endif

    //
    // If DbcsBuffers don't need to be modified, just bail out.
    //
    if (!IsAvailableFarEastCodePage(OldCodePage) == !CONSOLE_IS_DBCS_OUTPUTCP(pConsole) )
        return TRUE;

    //
    // Count the number of screens allocated.
    //
    for (nScreen = 0, pScreenInfo = pConsole->ScreenBuffers; pScreenInfo; pScreenInfo = pScreenInfo->Next) {
        //
        // Ignore graphic mode buffer.
        //
        if (pScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
            ++nScreen;
        }
    }
#if DBG
    nScreenSave = nScreen;
#endif

    //
    // Allocate the temporary buffer to store the old values
    //
    pDbcsScreenBuffer = ConsoleHeapAlloc(MAKE_TAG(TMP_DBCS_TAG), sizeof *pDbcsScreenBuffer * nScreen);
    if (pDbcsScreenBuffer == NULL) {
        RIPMSG0(RIP_WARNING, "ReCreateDbcsScreenBuffer: not enough memory.");
        return FALSE;
    }

    //
    // Try to allocate or de-allocate the necessary DBCS buffers
    //
    for (nScreen = 0, pScreenInfo = pConsole->ScreenBuffers; pScreenInfo; pScreenInfo = pScreenInfo->Next) {
        ASSERT(nScreen < nScreenSave);  // make sure ScreenBuffers are not changed

        //
        // We only handle the text mode screen buffer.
        //
        if (pScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
            //
            // Save the previous value just in case something goes bad.
            //
#if DBG
            pDbcsScreenBuffer[nScreen].pScreenInfo = pScreenInfo;
#endif
            pDbcsScreenBuffer[nScreen++].data = pScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer;

            if (!ReCreateDbcsScreenBufferWorker(pConsole, pScreenInfo)) {
                //
                // If we fail to ReCreate the DbcsScreenBuffer,
                // free all allocation to this point, and restore the orginal.
                //
                RIPMSG0(RIP_WARNING, "ReCreateDbcsScreenBuffer: failed to recreate dbcs screen buffer.");

                for (i = 0, pScreenInfo = pConsole->ScreenBuffers; i < nScreen;  pScreenInfo = pScreenInfo->Next) {
                    ASSERT(pScreenInfo);

                    if (pScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
                        ASSERT(pDbcsScreenBuffer[i].pScreenInfo == pScreenInfo);
                        if (i < nScreen - 1) {
                            ASSERT(pScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer.KAttrRows != pDbcsScreenBuffer[i].data.KAttrRows);
                            DeleteDbcsScreenBuffer(&pScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer);
                        }

                        pScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer = pDbcsScreenBuffer[i++].data;
                    }
                }
                goto exit;
            }
        }
    }

    //
    // All allocation succeeded. Now we can delete the old allocation.
    //
    for (i = 0; i < nScreen; ++i) {
        DeleteDbcsScreenBuffer(&pDbcsScreenBuffer[i].data);
    }

    fResult = TRUE;

exit:
    ConsoleHeapFree(pDbcsScreenBuffer);

    return fResult;
}

// Checks if the primary language of this keyborad layout is BiDi or not.
BOOL IsNotBiDILayout(HKL hkl)
{
    BOOL bRet = TRUE;
    LANGID LangID = PRIMARYLANGID(HandleToUlong(hkl));

    if ( (LangID == LANG_ARABIC) || (LangID == LANG_HEBREW)) {
        bRet = FALSE;
    }
    return bRet;

}

void GetNonBiDiKeyboardLayout(HKL *phklActive)
{
    HKL hkl = *phklActive;

    if ( IsNotBiDILayout(hkl) )
        return;

    // Start with the default one.
    ActivateKeyboardLayout(hkl, 0);
    // We know that the default is not good, Activate the next.
    ActivateKeyboardLayout((HKL)HKL_NEXT, 0);

    // Loop until you find a none BiDi one or endof list.
    while (hkl = GetKeyboardLayout(0))
    {
        if ((hkl == *phklActive) || IsNotBiDILayout(hkl)) {
            *phklActive = hkl;
            break;
        }
        ActivateKeyboardLayout((HKL)HKL_NEXT, 0);
    }
}

#if defined(FE_SB)

#define WWSB_NOFE
#include "_output.h"
#undef  WWSB_NOFE
#define WWSB_FE
#include "_output.h"
#undef  WWSB_FE

#endif  // FE_SB
