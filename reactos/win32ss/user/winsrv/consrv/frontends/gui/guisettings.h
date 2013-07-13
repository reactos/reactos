/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/guisettings.h
 * PURPOSE:         GUI front-end settings management
 * PROGRAMMERS:     Johannes Anderwald
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * NOTE: Also used by console.dll
 */

#pragma once

#ifndef WM_APP
    #define WM_APP 0x8000
#endif
#define PM_APPLY_CONSOLE_INFO (WM_APP + 100)

/* STRUCTURES *****************************************************************/

typedef struct _GUI_CONSOLE_INFO
{
    // FONTSIGNATURE FontSignature;
    WCHAR FaceName[LF_FACESIZE];
    UINT  FontFamily;
    DWORD FontSize;
    DWORD FontWeight;
    BOOL  UseRasterFonts;

    BOOL  FullScreen;       /* Whether the console is displayed in full-screen or windowed mode */
//  ULONG HardwareState;    /* _GDI_MANAGED, _DIRECT */

    WORD  ShowWindow;
    BOOL  AutoPosition;
    POINT WindowOrigin;
} GUI_CONSOLE_INFO, *PGUI_CONSOLE_INFO;

#ifndef CONSOLE_H__ // If we aren't included by console.dll

typedef struct _GUI_CONSOLE_DATA
{
    CRITICAL_SECTION Lock;
    HANDLE hGuiInitEvent;
    BOOL WindowSizeLock;
    POINT OldCursor;

    HWND hWindow;               /* Handle to the console's window       */
    HICON hIcon;                /* Handle to the console's icon (big)   */
    HICON hIconSm;              /* Handle to the console's icon (small) */

    HCURSOR hCursor;            /* Handle to the mouse cursor */
    INT  MouseCursorRefCount;   /* The reference counter associated with the mouse cursor. >= 0 and the cursor is shown; < 0 and the cursor is hidden. */
    BOOL IgnoreNextMouseSignal; /* Used in cases where we don't want to treat a mouse signal */

    BOOL IsCloseButtonEnabled;  /* TRUE if the Close button and the corresponding system menu item are enabled (default), FALSE otherwise */
    UINT cmdIdLow ;             /* Lowest menu id of the user-reserved menu id range */
    UINT cmdIdHigh;             /* Highest menu id of the user-reserved menu id range */

//  COLORREF Colors[16];

//  PVOID   ScreenBuffer;       /* Hardware screen buffer */

    HFONT Font;
    UINT CharWidth;
    UINT CharHeight;

    PCONSOLE Console;           /* Pointer to the owned console */
    GUI_CONSOLE_INFO GuiInfo;   /* GUI terminal settings */
} GUI_CONSOLE_DATA, *PGUI_CONSOLE_DATA;

/* FUNCTIONS ******************************************************************/

BOOL
GuiConsoleReadUserSettings(IN OUT PGUI_CONSOLE_INFO TermInfo,
                           IN LPCWSTR ConsoleTitle,
                           IN DWORD ProcessId);
BOOL
GuiConsoleWriteUserSettings(IN OUT PGUI_CONSOLE_INFO TermInfo,
                            IN LPCWSTR ConsoleTitle,
                            IN DWORD ProcessId);
VOID
GuiConsoleGetDefaultSettings(IN OUT PGUI_CONSOLE_INFO TermInfo,
                             IN DWORD ProcessId);
VOID
GuiConsoleShowConsoleProperties(PGUI_CONSOLE_DATA GuiData,
                                BOOL Defaults);
NTSTATUS
GuiApplyUserSettings(PGUI_CONSOLE_DATA GuiData,
                     HANDLE hClientSection,
                     BOOL SaveSettings);

#endif

/* EOF */
