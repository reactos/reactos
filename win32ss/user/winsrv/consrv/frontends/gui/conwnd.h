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


typedef struct _GUI_CONSOLE_DATA
{
    CRITICAL_SECTION Lock;
    BOOL WindowSizeLock;
    HANDLE hGuiInitEvent;

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
    BOOL IgnoreNextMouseSignal; /* Used in cases where we don't want to treat a mouse signal */

    BOOL IsCloseButtonEnabled;  /* TRUE if the Close button and the corresponding system menu item are enabled (default), FALSE otherwise */
    UINT CmdIdLow ;             /* Lowest menu id of the user-reserved menu id range */
    UINT CmdIdHigh;             /* Highest menu id of the user-reserved menu id range */

//  COLORREF Colors[16];

//  PVOID   ScreenBuffer;       /* Hardware screen buffer */

    HFONT Font;
    UINT CharWidth;
    UINT CharHeight;
/*****************************************************/

    PCONSOLE Console;           /* Pointer to the owned console */
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;    /* Pointer to the active screen buffer (then maybe the previous Console member is redundant?? Or not...) */
    CONSOLE_SELECTION_INFO Selection;       /* Contains information about the selection */
    COORD dwSelectionCursor;                /* Selection cursor position, most of the time different from Selection.dwSelectionAnchor */

    GUI_CONSOLE_INFO GuiInfo;   /* GUI terminal settings */
} GUI_CONSOLE_DATA, *PGUI_CONSOLE_DATA;
