/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     Dual-licensed:
 *              LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 *              MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Undocumented Console API definitions, absent from wincon.h
 * COPYRIGHT:   Copyright 2013-2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 *
 * REMARK: This header is based on the following resources:
 * - https://undoc.airesoft.co.uk/kernel32.dll/
 * - https://github.com/microsoft/terminal/blob/main/dep/Console/ntcon.h
 * - https://github.com/microsoft/terminal/blob/main/dep/Console/winconp.h
 *   (commit f08321a0b2)
 */

#ifndef _WINCON_UNDOC_H
#define _WINCON_UNDOC_H

#ifndef _WINCONP_ // As seen in dep/Console/winconp.h
#define _WINCONP_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA) && !defined(NOGDI)
#include "wingdi.h"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4820)
#endif

/*
 * Console display modes
 */
// These flags are returned by GetConsoleDisplayMode
#define CONSOLE_WINDOWED            0   /* Windowed console */

#if 0 // Defined in wincon.h; kept here for reference.
#define CONSOLE_FULLSCREEN          1   /* Fullscreen console */
#define CONSOLE_FULLSCREEN_HARDWARE 2   /* Console owns the hardware */

// These flags are given to SetConsoleDisplayMode
#define CONSOLE_FULLSCREEN_MODE     1
#define CONSOLE_WINDOWED_MODE       2
#endif

/*
 * Color attributes for text and screen background
 */
// #define COMMON_LVB_GRID_RVERTICAL       0x1000
// NOTE: 0x2000 is unused
// #define COMMON_LVB_REVERSE_VIDEO        0x4000
#ifndef COMMON_LVB_SBCSDBCS
#define COMMON_LVB_SBCSDBCS \
    (COMMON_LVB_LEADING_BYTE | COMMON_LVB_TRAILING_BYTE) /* == 0x0300 */
#endif

/*
 * Screen buffer types for CreateConsoleScreenBuffer().
 * See https://blog.airesoft.co.uk/2012/10/things-ms-can-do-that-they-dont-tell-you-about-console-graphics/
 */
// #define CONSOLE_TEXTMODE_BUFFER 1
#define CONSOLE_GRAPHICS_BUFFER 2

/*
 * Undocumented Control handler codes,
 * additional to CTRL_C_EVENT, CTRL_BREAK_EVENT, CTRL_CLOSE_EVENT,
 * CTRL_LOGOFF_EVENT, CTRL_SHUTDOWN_EVENT.
 */
#define CTRL_LAST_CLOSE_EVENT   3   // SYSTEM_ROOT_CONSOLE_EVENT
// 4 is reserved!                   // SYSTEM_CLOSE_EVENT

/*
 * Input mode flags, available since NT 3.51
 * but documented only in Vista+ PSDK.
 */
#ifndef ENABLE_INSERT_MODE
#define ENABLE_INSERT_MODE              0x0020
#define ENABLE_QUICK_EDIT_MODE          0x0040
#define ENABLE_EXTENDED_FLAGS           0x0080
#endif

/*
 * Console selection flags
 */
#define CONSOLE_SELECTION_INVERTED      0x0010  /* Selection is inverted (turned off) */
#define CONSOLE_SELECTION_VALID         (CONSOLE_SELECTION_IN_PROGRESS | \
                                         CONSOLE_SELECTION_NOT_EMPTY | \
                                         CONSOLE_MOUSE_SELECTION | \
                                         CONSOLE_MOUSE_DOWN)

/*
 * History information and mode flags
 */
#ifndef HISTORY_NO_DUP_FLAG // (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
// For Get/SetConsoleHistoryInfo
#define HISTORY_NO_DUP_FLAG             0x0001
#endif
#ifndef CONSOLE_OVERSTRIKE  // (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
// For SetConsoleCommandHistoryMode
#define CONSOLE_OVERSTRIKE              0x0001
#endif

/* Always existed, but was added in official PSDK in Windows Vista/7.
 * Kept here for reference. */
#if 0
typedef struct _CONSOLE_READCONSOLE_CONTROL {
    ULONG nLength;
    ULONG nInitialChars;
    ULONG dwCtrlWakeupMask;
    ULONG dwControlKeyState;
} CONSOLE_READCONSOLE_CONTROL, *PCONSOLE_READCONSOLE_CONTROL;
#endif

/*
 * This is the graphics counterpart to text-mode CONSOLE_SCREEN_BUFFER_INFO.
 * See https://blog.airesoft.co.uk/2012/10/things-ms-can-do-that-they-dont-tell-you-about-console-graphics/
 */
#if defined(_WINGDI_) && !defined(NOGDI)
typedef struct _CONSOLE_GRAPHICS_BUFFER_INFO {
    DWORD        dwBitMapInfoLength;
    LPBITMAPINFO lpBitMapInfo;
    DWORD        dwUsage;    // DIB_PAL_COLORS or DIB_RGB_COLORS
    HANDLE       hMutex;
    PVOID        lpBitMap;
} CONSOLE_GRAPHICS_BUFFER_INFO, *PCONSOLE_GRAPHICS_BUFFER_INFO;
#endif

// typedef struct _KEY_EVENT_RECORD { ... } KEY_EVENT_RECORD, *PKEY_EVENT_RECORD;
C_ASSERT(FIELD_OFFSET(KEY_EVENT_RECORD, uChar) == 0xA);


#define EXENAME_LENGTH (255 + 1)

WINBASEAPI
DWORD
WINAPI
GetConsoleInputExeNameA(
  _In_ DWORD nBufferLength,
  _Out_writes_(nBufferLength) LPSTR lpExeName);

WINBASEAPI
DWORD
WINAPI
GetConsoleInputExeNameW(
  _In_ DWORD nBufferLength,
  _Out_writes_(nBufferLength) LPWSTR lpExeName);

WINBASEAPI
BOOL
WINAPI
SetConsoleInputExeNameA(
  _In_ LPCSTR lpExeName);

WINBASEAPI
BOOL
WINAPI
SetConsoleInputExeNameW(
  _In_ LPCWSTR lpExeName);

WINBASEAPI
VOID
WINAPI
ExpungeConsoleCommandHistoryA(
  _In_ LPCSTR lpExeName);

WINBASEAPI
VOID
WINAPI
ExpungeConsoleCommandHistoryW(
  _In_ LPCWSTR lpExeName);

WINBASEAPI
DWORD
WINAPI
GetConsoleCommandHistoryA(
  _Out_writes_bytes_(cbHistory) LPSTR lpHistory,
  _In_ DWORD cbHistory,
  _In_ LPCSTR lpExeName);

WINBASEAPI
DWORD
WINAPI
GetConsoleCommandHistoryW(
  _Out_writes_bytes_(cbHistory) LPWSTR lpHistory,
  _In_ DWORD cbHistory,
  _In_ LPCWSTR lpExeName);

WINBASEAPI
DWORD
WINAPI
GetConsoleCommandHistoryLengthA(
  _In_ LPCSTR lpExeName);

WINBASEAPI
DWORD
WINAPI
GetConsoleCommandHistoryLengthW(
  _In_ LPCWSTR lpExeName);

WINBASEAPI
BOOL
WINAPI
SetConsoleNumberOfCommandsA(
  _In_ DWORD dwNumCommands,
  _In_ LPCSTR lpExeName);

WINBASEAPI
BOOL
WINAPI
SetConsoleNumberOfCommandsW(
  _In_ DWORD dwNumCommands,
  _In_ LPCWSTR lpExeName);

/*
 * See https://undoc.airesoft.co.uk/kernel32.dll/InvalidateConsoleDIBits.php
 * and https://blog.airesoft.co.uk/2012/10/things-ms-can-do-that-they-dont-tell-you-about-console-graphics/
 */
WINBASEAPI
BOOL
WINAPI
InvalidateConsoleDIBits(
  _In_ HANDLE hConsoleOutput,
  _In_ PSMALL_RECT lpRect);

WINBASEAPI
BOOL
WINAPI
GetConsoleHardwareState(
  _In_ HANDLE hConsoleOutput,
  _Out_ PDWORD Flags,
  _Out_ PDWORD State);
#if 0 // FIXME: How https://github.com/microsoft/terminal/blob/main/dep/Console/winconp.h sees it:
WINBASEAPI
BOOL
WINAPI
GetConsoleHardwareState(
  _In_ HANDLE hConsoleOutput,
  _Out_ PCOORD lpResolution,
  _Out_ PCOORD lpFontSize);
#endif

WINBASEAPI
BOOL
WINAPI
SetConsoleHardwareState(
  _In_ HANDLE hConsoleOutput,
  _In_ DWORD Flags,
  _In_ DWORD State);
#if 0 // FIXME: How https://github.com/microsoft/terminal/blob/main/dep/Console/winconp.h sees it:
WINBASEAPI
BOOL
WINAPI
SetConsoleHardwareState(
  _In_ HANDLE hConsoleOutput,
  _In_ COORD dwResolution,
  _In_ COORD dwFontSize);
#endif


#define CONSOLE_NOSHORTCUTKEY   0x00000000  /* No shortcut key  */
#define CONSOLE_ALTTAB          0x00000001  /* Alt + Tab        */
#define CONSOLE_ALTESC          0x00000002  /* Alt + Escape     */
#define CONSOLE_ALTSPACE        0x00000004  /* Alt + Space      */
#define CONSOLE_ALTENTER        0x00000008  /* Alt + Enter      */
#define CONSOLE_ALTPRTSC        0x00000010  /* Alt Print screen */
#define CONSOLE_PRTSC           0x00000020  /* Print screen     */
#define CONSOLE_CTRLESC         0x00000040  /* Ctrl + Escape    */

typedef struct _APPKEY {
    WORD Modifier;
    WORD ScanCode;
} APPKEY, *LPAPPKEY;

#define CONSOLE_MODIFIER_SHIFT      0x0003  // Left shift key
#define CONSOLE_MODIFIER_CONTROL    0x0004  // Either Control shift key
#define CONSOLE_MODIFIER_ALT        0x0008  // Either Alt shift key

WINBASEAPI
BOOL
WINAPI
SetConsoleKeyShortcuts(
  _In_ BOOL bSet,
  _In_ BYTE bReserveKeys,
  _In_reads_(dwNumAppKeys) LPAPPKEY lpAppKeys,
  _In_ DWORD dwNumAppKeys);


#ifndef KL_NAMELENGTH // Defined in winuser.h
#define KL_NAMELENGTH 9
#endif

WINBASEAPI
BOOL
WINAPI
GetConsoleKeyboardLayoutNameA(
  _Out_writes_(KL_NAMELENGTH) LPSTR pszLayout);

WINBASEAPI
BOOL
WINAPI
GetConsoleKeyboardLayoutNameW(
  _Out_writes_(KL_NAMELENGTH) LPWSTR pszLayout);


WINBASEAPI
DWORD
WINAPI
SetLastConsoleEventActive(VOID);

/*
 * ReadConsoleInputExA/W, now documented at:
 * https://learn.microsoft.com/en-us/windows/console/readconsoleinputex
 */

/*
 * Read input flags for ReadConsoleInputExA/W
 */
#define CONSOLE_READ_NOREMOVE           0x0001
#define CONSOLE_READ_NOWAIT             0x0002
#define CONSOLE_READ_VALID          (CONSOLE_READ_NOREMOVE | CONSOLE_READ_NOWAIT)

WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
ReadConsoleInputExA(
  _In_ HANDLE hConsoleInput,
  _Out_writes_to_(nLength, *lpNumberOfEventsRead) PINPUT_RECORD lpBuffer,
  _In_ DWORD nLength,
  _Out_ _Deref_out_range_(<=, nLength) LPDWORD lpNumberOfEventsRead,
  _In_ WORD wFlags);

WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
ReadConsoleInputExW(
  _In_ HANDLE hConsoleInput,
  _Out_writes_to_(nLength, *lpNumberOfEventsRead) PINPUT_RECORD lpBuffer,
  _In_ DWORD nLength,
  _Out_ _Deref_out_range_(<=, nLength) LPDWORD lpNumberOfEventsRead,
  _In_ WORD wFlags);

/* See https://undoc.airesoft.co.uk/kernel32.dll/ConsoleMenuControl.php */
WINBASEAPI
HMENU
WINAPI
ConsoleMenuControl(
  _In_ HANDLE hConsoleOutput,
  _In_ DWORD dwCmdIdLow,
  _In_ DWORD dwCmdIdHigh);

WINBASEAPI
BOOL
WINAPI
SetConsoleMenuClose(
  _In_ BOOL bEnable);

/* See https://undoc.airesoft.co.uk/kernel32.dll/SetConsoleCursor.php */
WINBASEAPI
BOOL
WINAPI
SetConsoleCursor(
  _In_ HANDLE hConsoleOutput,
  _In_ HCURSOR hCursor);

/* See https://undoc.airesoft.co.uk/kernel32.dll/ShowConsoleCursor.php */
WINBASEAPI
INT
WINAPI
ShowConsoleCursor(
  _In_ HANDLE hConsoleOutput,
  _In_ BOOL bShow);

WINBASEAPI
BOOL
WINAPI
SetConsoleFont(
  _In_ HANDLE hConsoleOutput,
  _In_ DWORD nFont);

WINBASEAPI
DWORD
WINAPI
GetConsoleFontInfo(
  _In_ HANDLE hConsoleOutput,
  _In_ BOOL bMaximumWindow,
  _In_ DWORD nFontCount,
  _Out_ PCONSOLE_FONT_INFO lpConsoleFontInfo);

WINBASEAPI
DWORD
WINAPI
GetNumberOfConsoleFonts(VOID);

WINBASEAPI
BOOL
WINAPI
SetConsoleIcon(
  _In_ HICON hIcon);

/* See http://comments.gmane.org/gmane.comp.lang.harbour.devel/27844 */
WINBASEAPI
BOOL
WINAPI
SetConsolePalette(
  _In_ HANDLE hConsoleOutput,
  _In_ HPALETTE hPalette,
  _In_ UINT dwUsage);

WINBASEAPI
HANDLE
WINAPI
OpenConsoleW(
  _In_ LPCWSTR wsName,
  _In_ DWORD dwDesiredAccess,
  _In_ BOOL bInheritHandle,
  _In_ DWORD dwShareMode);

WINBASEAPI
BOOL
WINAPI
CloseConsoleHandle(
  _In_ HANDLE hHandle);

WINBASEAPI
HANDLE
WINAPI
DuplicateConsoleHandle(
  _In_ HANDLE hSourceHandle,
  _In_ DWORD dwDesiredAccess,
  _In_ BOOL bInheritHandle,
  _In_ DWORD dwOptions);

WINBASEAPI
HANDLE
WINAPI
GetConsoleInputWaitHandle(VOID);

WINBASEAPI
BOOL
WINAPI
VerifyConsoleIoHandle(
  _In_ HANDLE hIoHandle);


/*
 * dwRegisterFlags for RegisterConsoleVDM
 */
#define CONSOLE_UNREGISTER_VDM  0
#define CONSOLE_REGISTER_VDM    1
#define CONSOLE_REGISTER_WOW    2

WINBASEAPI
BOOL
WINAPI
RegisterConsoleVDM(
  _In_ DWORD dwRegisterFlags,
  _In_ HANDLE hStartHardwareEvent,
  _In_ HANDLE hEndHardwareEvent,
  _In_ HANDLE hErrorHardwareEvent,
  _Reserved_ DWORD Reserved,
  _Out_ LPDWORD lpVideoStateLength,
  _Outptr_ PVOID* lpVideoState, // PVIDEO_HARDWARE_STATE_HEADER*
  _In_ PVOID lpUnusedBuffer,
  _In_ DWORD dwUnusedBufferLength,
  _In_ COORD dwVDMBufferSize,
  _Outptr_ PVOID* lpVDMBuffer);

/*
 * iFunction for VDMConsoleOperation
 */
#define VDM_HIDE_WINDOW         1
#define VDM_IS_ICONIC           2
#define VDM_CLIENT_RECT         3
#define VDM_CLIENT_TO_SCREEN    4
#define VDM_SCREEN_TO_CLIENT    5
#define VDM_IS_HIDDEN           6
#define VDM_FULLSCREEN_NOPAINT  7
#define VDM_SET_VIDEO_MODE      8

WINBASEAPI
BOOL
WINAPI
VDMConsoleOperation(
  _In_ DWORD iFunction,
  _Inout_opt_ LPVOID lpData);

WINBASEAPI
BOOL
WINAPI
WriteConsoleInputVDMA(
  _In_ HANDLE hConsoleInput,
  _In_reads_(nLength) CONST INPUT_RECORD *lpBuffer,
  _In_ DWORD nLength,
  _Out_ LPDWORD lpNumberOfEventsWritten);

WINBASEAPI
BOOL
WINAPI
WriteConsoleInputVDMW(
  _In_ HANDLE hConsoleInput,
  _In_reads_(nLength) CONST INPUT_RECORD *lpBuffer,
  _In_ DWORD nLength,
  _Out_ LPDWORD lpNumberOfEventsWritten);


WINBASEAPI
BOOL
WINAPI
GetConsoleNlsMode(
  _In_ HANDLE hConsole,
  _Out_ PDWORD lpdwNlsMode);

WINBASEAPI
BOOL
WINAPI
SetConsoleNlsMode(
  _In_ HANDLE hConsole,
  _In_ DWORD fdwNlsMode);

WINBASEAPI
BOOL
WINAPI
GetConsoleCharType(
  _In_ HANDLE hConsole,
  _In_ COORD coordCheck,
  _Out_ PDWORD pdwType);

/* Character type returned by GetConsoleCharType */
#define CHAR_TYPE_SBCS     0   // Displayed SBCS character
#define CHAR_TYPE_LEADING  2   // Displayed leading byte of DBCS
#define CHAR_TYPE_TRAILING 3   // Displayed trailing byte of DBCS

WINBASEAPI
BOOL
WINAPI
SetConsoleLocalEUDC(
  _In_ HANDLE hConsoleHandle,
  _In_ WORD wCodePoint,
  _In_ COORD cFontSize,
  _In_ PCHAR lpSB);

WINBASEAPI
BOOL
WINAPI
SetConsoleCursorMode(
  _In_ HANDLE hConsoleHandle,
  _In_ BOOL Blink,
  _In_ BOOL DBEnable);

WINBASEAPI
BOOL
WINAPI
GetConsoleCursorMode(
  _In_ HANDLE hConsoleHandle,
  _Out_ PBOOL pbBlink,
  _Out_ PBOOL pbDBEnable);

WINBASEAPI
BOOL
WINAPI
RegisterConsoleOS2(
  _In_ BOOL fOs2Register);

WINBASEAPI
BOOL
WINAPI
SetConsoleOS2OemFormat(
  _In_ BOOL fOs2OemFormat);

#if defined(FE_IME) || defined(__REACTOS__)
WINBASEAPI
BOOL
WINAPI
RegisterConsoleIME(
  _In_ HWND  hWndConsoleIME,
  _Out_opt_ DWORD *lpdwConsoleThreadId);

WINBASEAPI
BOOL
WINAPI
UnregisterConsoleIME(VOID);
#endif // FE_IME

#ifdef UNICODE
#define GetConsoleInputExeName GetConsoleInputExeNameW
#define SetConsoleInputExeName SetConsoleInputExeNameW
#define ExpungeConsoleCommandHistory ExpungeConsoleCommandHistoryW
#define GetConsoleCommandHistory GetConsoleCommandHistoryW
#define GetConsoleCommandHistoryLength GetConsoleCommandHistoryLengthW
#define SetConsoleNumberOfCommands SetConsoleNumberOfCommandsW
#define GetConsoleKeyboardLayoutName GetConsoleKeyboardLayoutNameW
#define ReadConsoleInputEx ReadConsoleInputExW
#define WriteConsoleInputVDM WriteConsoleInputVDMW
#else
#define GetConsoleInputExeName GetConsoleInputExeNameA
#define SetConsoleInputExeName SetConsoleInputExeNameA
#define ExpungeConsoleCommandHistory ExpungeConsoleCommandHistoryA
#define GetConsoleCommandHistory GetConsoleCommandHistoryA
#define GetConsoleCommandHistoryLength GetConsoleCommandHistoryLengthA
#define SetConsoleNumberOfCommands SetConsoleNumberOfCommandsA
#define GetConsoleKeyboardLayoutName GetConsoleKeyboardLayoutNameA
#define ReadConsoleInputEx ReadConsoleInputExA
#define WriteConsoleInputVDM WriteConsoleInputVDMA
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _WINCONP_ */
#endif /* _WINCON_UNDOC_H */
