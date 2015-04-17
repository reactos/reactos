/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            frontends/gui/conwnd.h
 * PURPOSE:         GUI Console Window Class
 * PROGRAMMERS:     Gé van Geldorp
 *                  Johannes Anderwald
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

/* GUI Console Window Class name */
#define GUI_CONWND_CLASS L"ConsoleWindowClass"

#ifndef WM_APP
    #define WM_APP 0x8000
#endif
#define PM_RESIZE_TERMINAL      (WM_APP + 3)
#define PM_CONSOLE_BEEP         (WM_APP + 4)
#define PM_CONSOLE_SET_TITLE    (WM_APP + 5)

/*
typedef struct _CONSOLE_FONT
{
    HFONT Font;
    ULONG Flag;
} CONSOLE_FONT, *PCONSOLE_FONT;
*/
#define FONT_NORMAL     0x00
#define FONT_BOLD       0x01
#define FONT_UNDERLINE  0x02
#define FONT_MAXNO      0x04

typedef struct _GUI_CONSOLE_DATA
{
    CRITICAL_SECTION Lock;
    BOOL WindowSizeLock;
    HANDLE hGuiInitEvent;
    HANDLE hGuiTermEvent;

    // HANDLE InputThreadHandle;
    ULONG_PTR InputThreadId;
    HWINSTA WinSta;
    HDESK   Desktop;

    BOOLEAN IsWindowVisible;

    POINT OldCursor;

    LONG_PTR WndStyle;
    LONG_PTR WndStyleEx;
    BOOL IsWndMax;
    WINDOWPLACEMENT WndPl;

    HWND hWindow;               /* Handle to the console's window            */
    HDC  hMemDC;                /* Memory DC holding the console framebuffer */
    HBITMAP  hBitmap;           /* Console framebuffer                       */
    HPALETTE hSysPalette;       /* Handle to the original system palette     */

    HICON hIcon;                /* Handle to the console's icon (big)   */
    HICON hIconSm;              /* Handle to the console's icon (small) */

/*** The following may be put per-screen-buffer !! ***/
    HCURSOR hCursor;            /* Handle to the mouse cursor */
    INT  MouseCursorRefCount;   /* The reference counter associated with the mouse cursor. >= 0 and the cursor is shown; < 0 and the cursor is hidden. */
    BOOL IgnoreNextMouseSignal; /* Used when we need to not process a mouse signal */

    BOOL HackCORE8394IgnoreNextMove; /* HACK FOR CORE-8394. See conwnd.c!OnMouse for more details. */

    BOOL IsCloseButtonEnabled;  /* TRUE if the Close button and the corresponding system menu item are enabled (default), FALSE otherwise */
    UINT CmdIdLow ;             /* Lowest menu id of the user-reserved menu id range */
    UINT CmdIdHigh;             /* Highest menu id of the user-reserved menu id range */

//  COLORREF Colors[16];

//  PVOID   ScreenBuffer;       /* Hardware screen buffer */

    HFONT Font[FONT_MAXNO];
    UINT CharWidth;     /* The character width and height should be the same for */
    UINT CharHeight;    /* both normal and bold/underlined fonts...              */
/*****************************************************/

    PCONSRV_CONSOLE Console;           /* Pointer to the owned console */
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;    /* Pointer to the active screen buffer (then maybe the previous Console member is redundant?? Or not...) */
    CONSOLE_SELECTION_INFO Selection;       /* Contains information about the selection */
    COORD dwSelectionCursor;                /* Selection cursor position, most of the time different from Selection.dwSelectionAnchor */
    BOOL  LineSelection;                    /* TRUE if line-oriented selection (a la *nix terminals), FALSE if block-oriented selection (default on Windows) */

    GUI_CONSOLE_INFO GuiInfo;   /* GUI terminal settings */
} GUI_CONSOLE_DATA, *PGUI_CONSOLE_DATA;
