/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            consrv/guisettings.h
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
/* Message sent by ReactOS' console.dll for applying console info */
#define PM_APPLY_CONSOLE_INFO   (WM_APP + 100)

/*
 * Undocumented message sent by Windows' console.dll for applying console info.
 * See http://www.catch22.net/sites/default/source/files/setconsoleinfo.c
 * and http://www.scn.rain.com/~neighorn/PDF/MSBugPaper.pdf
 * for more information.
 */
#define WM_SETCONSOLEINFO       (WM_USER + 201)

/* STRUCTURES *****************************************************************/

typedef struct _GUI_CONSOLE_INFO
{
    WCHAR FaceName[LF_FACESIZE];
    ULONG FontFamily;
    COORD FontSize;
    ULONG FontWeight;

    BOOL  FullScreen;       /* Whether the console is displayed in full-screen or windowed mode */
//  ULONG HardwareState;    /* _GDI_MANAGED, _DIRECT */

    WORD  ShowWindow;
    BOOL  AutoPosition;
    POINT WindowOrigin;
} GUI_CONSOLE_INFO, *PGUI_CONSOLE_INFO;

/*
 * Undocumented structure used by Windows' console.dll for setting console info.
 * See http://www.catch22.net/sites/default/source/files/setconsoleinfo.c
 * and http://www.scn.rain.com/~neighorn/PDF/MSBugPaper.pdf
 * for more information.
 */
#pragma pack(push, 1)
typedef struct _CONSOLE_STATE_INFO
{
    ULONG       cbSize;
    COORD       ScreenBufferSize;
    COORD       WindowSize;
    POINT       WindowPosition; // WindowPosX and Y

    COORD       FontSize;
    ULONG       FontFamily;
    ULONG       FontWeight;
    WCHAR       FaceName[LF_FACESIZE];

    ULONG       CursorSize;
    BOOL        FullScreen;
    BOOL        QuickEdit;
    BOOL        AutoPosition;
    BOOL        InsertMode;

    USHORT      ScreenColors;   // ScreenAttributes
    USHORT      PopupColors;    // PopupAttributes
    BOOL        HistoryNoDup;
    ULONG       HistoryBufferSize;
    ULONG       NumberOfHistoryBuffers;

    COLORREF    ColorTable[16];

    ULONG       CodePage;
    HWND        hWnd;

    WCHAR       ConsoleTitle[256];
} CONSOLE_STATE_INFO, *PCONSOLE_STATE_INFO;
#pragma pack(pop)

#ifndef CONSOLE_H__ // If we aren't included by console.dll

#include "conwnd.h"

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
VOID
GuiApplyUserSettings(PGUI_CONSOLE_DATA GuiData,
                     HANDLE hClientSection,
                     BOOL SaveSettings);
VOID
GuiApplyWindowsConsoleSettings(PGUI_CONSOLE_DATA GuiData,
                               HANDLE hClientSection);

#endif

/* EOF */
