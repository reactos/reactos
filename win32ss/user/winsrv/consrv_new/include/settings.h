/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv_new/include/settings.h
 * PURPOSE:         Public Console Settings Management Interface
 * PROGRAMMERS:     Johannes Anderwald
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

/* STRUCTURES *****************************************************************/

/*
 * Structure used to hold terminal-specific information
 */
typedef struct _TERMINAL_INFO
{
    ULONG Size;     /* Size of the memory buffer pointed by TermInfo */
    PVOID TermInfo; /* Address (or offset when talking to console.dll) of the memory buffer holding terminal information */
} TERMINAL_INFO, *PTERMINAL_INFO;

/*
 * Structure used to hold console information
 */
typedef struct _CONSOLE_INFO
{
    ULONG   HistoryBufferSize;
    ULONG   NumberOfHistoryBuffers;
    BOOLEAN HistoryNoDup;

    BOOLEAN QuickEdit;
    BOOLEAN InsertMode;
    ULONG   InputBufferSize;
    COORD   ScreenBufferSize;
    COORD   ConsoleSize;          /* The size of the console */

    BOOLEAN CursorBlinkOn;
    BOOLEAN ForceCursorOff;
    ULONG   CursorSize;

    USHORT ScreenAttrib; // CHAR_INFO ScreenFillAttrib
    USHORT PopupAttrib;

    COLORREF Colors[16];    /* Color palette */

    ULONG CodePage;

    WCHAR ConsoleTitle[MAX_PATH + 1];
} CONSOLE_INFO, *PCONSOLE_INFO;

#define RGBFromAttrib(Console, Attribute)   ((Console)->Colors[(Attribute) & 0xF])
#define TextAttribFromAttrib(Attribute)     ((Attribute) & 0xF)
#define BkgdAttribFromAttrib(Attribute)     (((Attribute) >> 4) & 0xF)
#define MakeAttrib(TextAttrib, BkgdAttrib)  (DWORD)((((BkgdAttrib) & 0xF) << 4) | ((TextAttrib) & 0xF))

/*
 * Structure used to communicate with console.dll
 */
typedef struct _CONSOLE_PROPS
{
    HWND hConsoleWindow;
    BOOL ShowDefaultParams;

    BOOLEAN AppliedConfig;
    DWORD   ActiveStaticControl;

    CONSOLE_INFO  ci;           /* Console-specific informations */
    TERMINAL_INFO TerminalInfo; /* Frontend-specific parameters  */
} CONSOLE_PROPS, *PCONSOLE_PROPS;

/* FUNCTIONS ******************************************************************/

#ifndef CONSOLE_H__ // If we aren't included by console.dll

BOOL ConSrvOpenUserSettings(DWORD ProcessId,
                            LPCWSTR ConsoleTitle,
                            PHKEY hSubKey,
                            REGSAM samDesired,
                            BOOL bCreate);

BOOL ConSrvReadUserSettings(IN OUT PCONSOLE_INFO ConsoleInfo,
                            IN DWORD ProcessId);
BOOL ConSrvWriteUserSettings(IN PCONSOLE_INFO ConsoleInfo,
                             IN DWORD ProcessId);
VOID ConSrvGetDefaultSettings(IN OUT PCONSOLE_INFO ConsoleInfo,
                              IN DWORD ProcessId);
VOID ConSrvApplyUserSettings(IN PCONSOLE Console,
                             IN PCONSOLE_INFO ConsoleInfo);

#endif

/* EOF */
