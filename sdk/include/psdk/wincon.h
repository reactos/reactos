/*
 * wincon.h
 *
 * Console API definitions
 *
 * This file is part of the ReactOS PSDK package.
 *
 * Contributors:
 *   Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _WINCON_
#define _WINCON_

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
 * Special PID for parent process for AttachConsole API
 */
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN2K)
#define ATTACH_PARENT_PROCESS   ((DWORD)-1)
#endif

/* Special console handle values */
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
#define CONSOLE_REAL_OUTPUT_HANDLE  (LongToHandle(-2))
#define CONSOLE_REAL_INPUT_HANDLE   (LongToHandle(-3))
#endif

/*
 * Console display modes
 */
// These flags are returned by GetConsoleDisplayMode
#define CONSOLE_WINDOWED            0
#define CONSOLE_FULLSCREEN          1   /* Fullscreen console */
#define CONSOLE_FULLSCREEN_HARDWARE 2   /* Console owns the hardware */

// These flags are given to SetConsoleDisplayMode
#define CONSOLE_FULLSCREEN_MODE     1
#define CONSOLE_WINDOWED_MODE       2

/*
 * Color attributes for text and screen background
 */
#define FOREGROUND_BLUE                 0x0001
#define FOREGROUND_GREEN                0x0002
#define FOREGROUND_RED                  0x0004
#define FOREGROUND_INTENSITY            0x0008
#define BACKGROUND_BLUE                 0x0010
#define BACKGROUND_GREEN                0x0020
#define BACKGROUND_RED                  0x0040
#define BACKGROUND_INTENSITY            0x0080

#define COMMON_LVB_LEADING_BYTE         0x0100  /* DBCS Leading Byte  */
#define COMMON_LVB_TRAILING_BYTE        0x0200  /* DBCS Trailing Byte */
#define COMMON_LVB_GRID_HORIZONTAL      0x0400  /* Grid attribute: Top Horizontal */
#define COMMON_LVB_GRID_LVERTICAL       0x0800  /* Grid attribute: Left Vertical  */
#define COMMON_LVB_GRID_RVERTICAL       0x1000  /* Grid attribute: Right Vertical */
#define COMMON_LVB_REVERSE_VIDEO        0x4000  /* Reverse fore/back ground attribute */
#define COMMON_LVB_UNDERSCORE           0x8000  /* Underscore */

#define COMMON_LVB_SBCSDBCS \
    (COMMON_LVB_LEADING_BYTE | COMMON_LVB_TRAILING_BYTE) /* == 0x0300 */

/*
 * Screen buffer types
 */
#define CONSOLE_TEXTMODE_BUFFER 1
// 2 is reserved!

/*
 * Control handler codes
 */
#define CTRL_C_EVENT        0
#define CTRL_BREAK_EVENT    1
#define CTRL_CLOSE_EVENT    2
// 3 is reserved!
// 4 is reserved!
#define CTRL_LOGOFF_EVENT   5
#define CTRL_SHUTDOWN_EVENT 6

/*
 * Input mode flags
 */
#define ENABLE_PROCESSED_INPUT          0x0001
#define ENABLE_LINE_INPUT               0x0002
#define ENABLE_ECHO_INPUT               0x0004
#define ENABLE_WINDOW_INPUT             0x0008
#define ENABLE_MOUSE_INPUT              0x0010
#define ENABLE_INSERT_MODE              0x0020
#define ENABLE_QUICK_EDIT_MODE          0x0040
#define ENABLE_EXTENDED_FLAGS           0x0080
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
#define ENABLE_AUTO_POSITION            0x0100
#endif
#if (NTDDI_VERSION >= NTDDI_WIN10_RS1) // (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
#define ENABLE_VIRTUAL_TERMINAL_INPUT   0x0200
#endif

/*
 * Output mode flags
 */
#define ENABLE_PROCESSED_OUTPUT             0x0001
#define ENABLE_WRAP_AT_EOL_OUTPUT           0x0002
#if (NTDDI_VERSION >= NTDDI_WIN10_RS1) // (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING  0x0004
#define DISABLE_NEWLINE_AUTO_RETURN         0x0008
#define ENABLE_LVB_GRID_WORLDWIDE           0x0010
#endif

/*
 * Console selection flags
 */
#define CONSOLE_NO_SELECTION            0x0000
#define CONSOLE_SELECTION_IN_PROGRESS   0x0001  /* Selection has begun */
#define CONSOLE_SELECTION_NOT_EMPTY     0x0002  /* Non-null select rectangle */
#define CONSOLE_MOUSE_SELECTION         0x0004  /* Selecting with mouse */
#define CONSOLE_MOUSE_DOWN              0x0008  /* Mouse is down */

/*
 * History information and mode flags
 */
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
// For Get/SetConsoleHistoryInfo
#define HISTORY_NO_DUP_FLAG             0x0001
// For SetConsoleCommandHistoryMode
#define CONSOLE_OVERSTRIKE              0x0001
#endif


/*
 * Event types
 */
#define KEY_EVENT                       0x0001
#define MOUSE_EVENT                     0x0002
#define WINDOW_BUFFER_SIZE_EVENT        0x0004
#define MENU_EVENT                      0x0008
#define FOCUS_EVENT                     0x0010

/*
 * ControlKeyState flags
 */
#define RIGHT_ALT_PRESSED               0x0001
#define LEFT_ALT_PRESSED                0x0002
#define RIGHT_CTRL_PRESSED              0x0004
#define LEFT_CTRL_PRESSED               0x0008
#define SHIFT_PRESSED                   0x0010
#define NUMLOCK_ON                      0x0020
#define SCROLLLOCK_ON                   0x0040
#define CAPSLOCK_ON                     0x0080
#define ENHANCED_KEY                    0x0100

// NLS Japanese flags
#define NLS_DBCSCHAR                    0x00010000  /* SBCS/DBCS mode     */
#define NLS_ALPHANUMERIC                0x00000000  /* Alphanumeric mode  */
#define NLS_KATAKANA                    0x00020000  /* Katakana mode      */
#define NLS_HIRAGANA                    0x00040000  /* Hiragana mode      */
#define NLS_ROMAN                       0x00400000  /* Roman/Noroman mode */
#define NLS_IME_CONVERSION              0x00800000  /* IME conversion     */
/* Reserved for EXTENDED_BIT, DONTCARE_BIT, FAKE_KEYSTROKE, ALTNUMPAD_BIT (kbd.h) */
#define ALTNUMPAD_BIT                   0x04000000  /* AltNumpad OEM char */
#define NLS_IME_DISABLE                 0x20000000  /* IME enable/disable */

/*
 * ButtonState flags
 */
#define FROM_LEFT_1ST_BUTTON_PRESSED    0x0001
#define RIGHTMOST_BUTTON_PRESSED        0x0002
#define FROM_LEFT_2ND_BUTTON_PRESSED    0x0004
#define FROM_LEFT_3RD_BUTTON_PRESSED    0x0008
#define FROM_LEFT_4TH_BUTTON_PRESSED    0x0010

/*
 * Mouse event flags
 */
#define MOUSE_MOVED                     0x0001
#define DOUBLE_CLICK                    0x0002
#define MOUSE_WHEELED                   0x0004
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
#define MOUSE_HWHEELED                  0x0008
#endif

typedef struct _CONSOLE_READCONSOLE_CONTROL {
    ULONG nLength;
    ULONG nInitialChars;
    ULONG dwCtrlWakeupMask;
    ULONG dwControlKeyState;
} CONSOLE_READCONSOLE_CONTROL, *PCONSOLE_READCONSOLE_CONTROL;

typedef struct _CHAR_INFO {
    union {
        WCHAR UnicodeChar;
        CHAR AsciiChar;
    } Char;
    WORD Attributes;
} CHAR_INFO,*PCHAR_INFO;

typedef struct _SMALL_RECT {
    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
} SMALL_RECT,*PSMALL_RECT;

typedef struct _CONSOLE_CURSOR_INFO {
    DWORD dwSize;
    BOOL  bVisible;
} CONSOLE_CURSOR_INFO,*PCONSOLE_CURSOR_INFO;

typedef struct _COORD {
    SHORT X;
    SHORT Y;
} COORD, *PCOORD;

typedef struct _CONSOLE_SELECTION_INFO {
    DWORD dwFlags;
    COORD dwSelectionAnchor;
    SMALL_RECT srSelection;
} CONSOLE_SELECTION_INFO, *PCONSOLE_SELECTION_INFO;

typedef struct _CONSOLE_FONT_INFO {
    DWORD nFont;
    COORD dwFontSize;
} CONSOLE_FONT_INFO, *PCONSOLE_FONT_INFO;

typedef struct _CONSOLE_SCREEN_BUFFER_INFO {
    COORD      dwSize;
    COORD      dwCursorPosition;
    WORD       wAttributes;
    SMALL_RECT srWindow;
    COORD      dwMaximumWindowSize;
} CONSOLE_SCREEN_BUFFER_INFO,*PCONSOLE_SCREEN_BUFFER_INFO;

typedef BOOL
(WINAPI *PHANDLER_ROUTINE)(
    _In_ DWORD CtrlType);

typedef struct _KEY_EVENT_RECORD {
    BOOL bKeyDown;
    WORD wRepeatCount;
    WORD wVirtualKeyCode;
    WORD wVirtualScanCode;
    union {
        WCHAR UnicodeChar;
        CHAR  AsciiChar;
    } uChar;
    DWORD dwControlKeyState;
} KEY_EVENT_RECORD, *PKEY_EVENT_RECORD;

C_ASSERT(FIELD_OFFSET(KEY_EVENT_RECORD, uChar) == 0xA);

typedef struct _MOUSE_EVENT_RECORD {
    COORD dwMousePosition;
    DWORD dwButtonState;
    DWORD dwControlKeyState;
    DWORD dwEventFlags;
} MOUSE_EVENT_RECORD, *PMOUSE_EVENT_RECORD;

typedef struct _WINDOW_BUFFER_SIZE_RECORD {
    COORD dwSize;
} WINDOW_BUFFER_SIZE_RECORD, *PWINDOW_BUFFER_SIZE_RECORD;

typedef struct _MENU_EVENT_RECORD {
    UINT dwCommandId;
} MENU_EVENT_RECORD, *PMENU_EVENT_RECORD;

typedef struct _FOCUS_EVENT_RECORD {
    BOOL bSetFocus;
} FOCUS_EVENT_RECORD, *PFOCUS_EVENT_RECORD;

typedef struct _INPUT_RECORD {
    WORD EventType;
    union {
        KEY_EVENT_RECORD KeyEvent;
        MOUSE_EVENT_RECORD MouseEvent;
        WINDOW_BUFFER_SIZE_RECORD WindowBufferSizeEvent;
        MENU_EVENT_RECORD MenuEvent;
        FOCUS_EVENT_RECORD FocusEvent;
    } Event;
} INPUT_RECORD, *PINPUT_RECORD;

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
typedef struct _CONSOLE_HISTORY_INFO {
    UINT cbSize;
    UINT HistoryBufferSize;
    UINT NumberOfHistoryBuffers;
    DWORD dwFlags;
} CONSOLE_HISTORY_INFO, *PCONSOLE_HISTORY_INFO;

typedef struct _CONSOLE_SCREEN_BUFFER_INFOEX {
    ULONG cbSize;
    COORD dwSize;
    COORD dwCursorPosition;
    WORD wAttributes;
    SMALL_RECT srWindow;
    COORD dwMaximumWindowSize;
    WORD wPopupAttributes;
    BOOL bFullscreenSupported;
    COLORREF ColorTable[16];
} CONSOLE_SCREEN_BUFFER_INFOEX, *PCONSOLE_SCREEN_BUFFER_INFOEX;

#ifndef NOGDI
typedef struct _CONSOLE_FONT_INFOEX {
    ULONG cbSize;
    DWORD nFont;
    COORD dwFontSize;
    UINT FontFamily;
    UINT FontWeight;
    WCHAR FaceName[LF_FACESIZE];
} CONSOLE_FONT_INFOEX, *PCONSOLE_FONT_INFOEX;
#endif
#endif // (_WIN32_WINNT >= _WIN32_WINNT_VISTA)

WINBASEAPI
BOOL
WINAPI
AllocConsole(VOID);

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN2K)
WINBASEAPI
BOOL
WINAPI
AttachConsole(
  _In_ DWORD dwProcessId);
#endif

#if (_WIN32_WINNT >= _WIN32_WINNT_WINXP)

WINBASEAPI
DWORD
WINAPI
GetConsoleProcessList(
  _Out_writes_(dwProcessCount) LPDWORD lpdwProcessList,
  _In_ DWORD dwProcessCount);

WINBASEAPI
BOOL
WINAPI
AddConsoleAliasA(
  _In_ LPCSTR Source,
  _In_ LPCSTR Target,
  _In_ LPCSTR ExeName);

WINBASEAPI
BOOL
WINAPI
AddConsoleAliasW(
  _In_ LPCWSTR Source,
  _In_ LPCWSTR Target,
  _In_ LPCWSTR ExeName);

WINBASEAPI
DWORD
WINAPI
GetConsoleAliasA(
  _In_ LPCSTR Source,
  _Out_writes_(TargetBufferLength) LPSTR TargetBuffer,
  _In_ DWORD TargetBufferLength,
  _In_ LPCSTR ExeName);

WINBASEAPI
DWORD
WINAPI
GetConsoleAliasW(
  _In_ LPCWSTR Source,
  _Out_writes_(TargetBufferLength) LPWSTR TargetBuffer,
  _In_ DWORD TargetBufferLength,
  _In_ LPCWSTR ExeName);

WINBASEAPI
DWORD
WINAPI
GetConsoleAliasesA(
  _Out_writes_(AliasBufferLength) LPSTR AliasBuffer,
  _In_ DWORD AliasBufferLength,
  _In_ LPCSTR ExeName);

WINBASEAPI
DWORD
WINAPI
GetConsoleAliasesW(
  _Out_writes_(AliasBufferLength) LPWSTR AliasBuffer,
  _In_ DWORD AliasBufferLength,
  _In_ LPCWSTR ExeName);

WINBASEAPI
DWORD
WINAPI
GetConsoleAliasesLengthA(
  _In_ LPCSTR ExeName);

WINBASEAPI
DWORD
WINAPI
GetConsoleAliasesLengthW(
  _In_ LPCWSTR ExeName);

WINBASEAPI
DWORD
WINAPI
GetConsoleAliasExesA(
  _Out_writes_(ExeNameBufferLength) LPSTR ExeNameBuffer,
  _In_ DWORD ExeNameBufferLength);

WINBASEAPI
DWORD
WINAPI
GetConsoleAliasExesW(
  _Out_writes_(ExeNameBufferLength) LPWSTR ExeNameBuffer,
  _In_ DWORD ExeNameBufferLength);

WINBASEAPI
DWORD
WINAPI
GetConsoleAliasExesLengthA(VOID);

WINBASEAPI
DWORD
WINAPI
GetConsoleAliasExesLengthW(VOID);

#endif // (_WIN32_WINNT >= _WIN32_WINNT_WINXP)

WINBASEAPI
HANDLE
WINAPI
CreateConsoleScreenBuffer(
  _In_ DWORD dwDesiredAccess,
  _In_ DWORD dwShareMode,
  _In_opt_ CONST SECURITY_ATTRIBUTES *lpSecurityAttributes,
  _In_ DWORD dwFlags,
  _Reserved_ LPVOID lpScreenBufferData);

WINBASEAPI
BOOL
WINAPI
FillConsoleOutputAttribute(
  _In_ HANDLE hConsoleOutput,
  _In_ WORD wAttribute,
  _In_ DWORD nLength,
  _In_ COORD dwWriteCoord,
  _Out_ LPDWORD lpNumberOfAttrsWritten);

WINBASEAPI
BOOL
WINAPI
FillConsoleOutputCharacterA(
  _In_ HANDLE hConsoleOutput,
  _In_ CHAR cCharacter,
  _In_ DWORD nLength,
  _In_ COORD dwWriteCoord,
  _Out_ LPDWORD lpNumberOfCharsWritten);

WINBASEAPI
BOOL
WINAPI
FillConsoleOutputCharacterW(
  _In_ HANDLE hConsoleOutput,
  _In_ WCHAR cCharacter,
  _In_ DWORD nLength,
  _In_ COORD dwWriteCoord,
  _Out_ LPDWORD lpNumberOfCharsWritten);

WINBASEAPI
BOOL
WINAPI
FlushConsoleInputBuffer(
  _In_ HANDLE hConsoleInput);

WINBASEAPI
BOOL
WINAPI
FreeConsole(VOID);

WINBASEAPI
BOOL
WINAPI
GenerateConsoleCtrlEvent(
  _In_ DWORD dwCtrlEvent,
  _In_ DWORD dwProcessGroupId);

WINBASEAPI
UINT
WINAPI
GetConsoleCP(VOID);

WINBASEAPI
BOOL
WINAPI
GetConsoleCursorInfo(
  _In_ HANDLE hConsoleOutput,
  _Out_ PCONSOLE_CURSOR_INFO lpConsoleCursorInfo);

WINBASEAPI
BOOL
WINAPI
GetConsoleMode(
  _In_ HANDLE hConsoleHandle,
  _Out_ LPDWORD lpMode);

WINBASEAPI
UINT
WINAPI
GetConsoleOutputCP(VOID);

WINBASEAPI
BOOL
WINAPI
GetConsoleScreenBufferInfo(
  _In_ HANDLE hConsoleOutput,
  _Out_ PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo);

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
WINBASEAPI
BOOL
WINAPI
GetConsoleScreenBufferInfoEx(
  _In_ HANDLE hConsoleOutput,
  _Inout_ PCONSOLE_SCREEN_BUFFER_INFOEX lpConsoleScreenBufferInfoEx);

WINBASEAPI
BOOL
WINAPI
SetConsoleScreenBufferInfoEx(
  _In_ HANDLE hConsoleOutput,
  _In_ PCONSOLE_SCREEN_BUFFER_INFOEX lpConsoleScreenBufferInfoEx);
#endif // (_WIN32_WINNT >= _WIN32_WINNT_VISTA)

WINBASEAPI
DWORD
WINAPI
GetConsoleTitleA(
  _Out_writes_(nSize) LPSTR lpConsoleTitle,
  _In_ DWORD nSize);

WINBASEAPI
DWORD
WINAPI
GetConsoleTitleW(
  _Out_writes_(nSize) LPWSTR lpConsoleTitle,
  _In_ DWORD nSize);

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)

WINBASEAPI
DWORD
WINAPI
GetConsoleOriginalTitleA(
  _Out_writes_(nSize) LPSTR lpConsoleTitle,
  _In_ DWORD nSize);

WINBASEAPI
DWORD
WINAPI
GetConsoleOriginalTitleW(
  _Out_writes_(nSize) LPWSTR lpConsoleTitle,
  _In_ DWORD nSize);

#ifndef NOGDI
WINBASEAPI
BOOL
WINAPI
GetCurrentConsoleFontEx(
  _In_ HANDLE hConsoleOutput,
  _In_ BOOL bMaximumWindow,
  _Out_ PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx);

WINBASEAPI
BOOL
WINAPI
SetCurrentConsoleFontEx(
  _In_ HANDLE hConsoleOutput,
  _In_ BOOL bMaximumWindow,
  _In_ PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx);
#endif

WINBASEAPI
BOOL
WINAPI
GetConsoleHistoryInfo(
  _Out_ PCONSOLE_HISTORY_INFO lpConsoleHistoryInfo);

WINBASEAPI
BOOL
WINAPI
SetConsoleHistoryInfo(
  _In_ PCONSOLE_HISTORY_INFO lpConsoleHistoryInfo);

#endif // (_WIN32_WINNT >= _WIN32_WINNT_VISTA)

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN2K)

WINBASEAPI
BOOL
WINAPI
GetCurrentConsoleFont(
  _In_ HANDLE hConsoleOutput,
  _In_ BOOL bMaximumWindow,
  _Out_ PCONSOLE_FONT_INFO lpConsoleCurrentFont);

WINBASEAPI
COORD
WINAPI
GetConsoleFontSize(
  _In_ HANDLE hConsoleOutput,
  _In_ DWORD nFont);

WINBASEAPI
BOOL
WINAPI
GetConsoleSelectionInfo(
  _Out_ PCONSOLE_SELECTION_INFO lpConsoleSelectionInfo);

WINBASEAPI
HWND
WINAPI
GetConsoleWindow(VOID);

WINBASEAPI
BOOL
WINAPI
GetConsoleDisplayMode(
  _Out_ LPDWORD lpModeFlags);

WINBASEAPI
BOOL
WINAPI
SetConsoleDisplayMode(
  _In_ HANDLE hConsoleOutput,
  _In_ DWORD dwFlags,
  _Out_opt_ PCOORD lpNewScreenBufferDimensions);

#endif // (_WIN32_WINNT >= _WIN32_WINNT_WIN2K)

WINBASEAPI
COORD
WINAPI
GetLargestConsoleWindowSize(
  _In_ HANDLE hConsoleOutput);

WINBASEAPI
BOOL
WINAPI
GetNumberOfConsoleInputEvents(
  _In_ HANDLE hConsoleInput,
  _Out_ LPDWORD lpNumberOfEvents);

WINBASEAPI
BOOL
WINAPI
GetNumberOfConsoleMouseButtons(
  _Out_ LPDWORD lpNumberOfMouseButtons);

WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI PeekConsoleInputA(
  _In_ HANDLE hConsoleInput,
  _Out_writes_(nLength) PINPUT_RECORD lpBuffer,
  _In_ DWORD nLength,
  _Out_ LPDWORD lpNumberOfEventsRead);

WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
PeekConsoleInputW(
  _In_ HANDLE hConsoleInput,
  _Out_writes_(nLength) PINPUT_RECORD lpBuffer,
  _In_ DWORD nLength,
  _Out_ LPDWORD lpNumberOfEventsRead);

WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
ReadConsoleA(
  _In_ HANDLE hConsoleInput,
  _Out_writes_bytes_to_(nNumberOfCharsToRead * sizeof(CHAR), *lpNumberOfCharsRead * sizeof(CHAR)) LPVOID lpBuffer,
  _In_ DWORD nNumberOfCharsToRead,
  _Out_ _Deref_out_range_(<=, nNumberOfCharsToRead) LPDWORD lpNumberOfCharsRead,
  _In_opt_ PCONSOLE_READCONSOLE_CONTROL pInputControl);

WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
ReadConsoleW(
  _In_ HANDLE hConsoleInput,
  _Out_writes_bytes_to_(nNumberOfCharsToRead * sizeof(WCHAR), *lpNumberOfCharsRead * sizeof(WCHAR)) LPVOID lpBuffer,
  _In_ DWORD nNumberOfCharsToRead,
  _Out_ _Deref_out_range_(<=, nNumberOfCharsToRead) LPDWORD lpNumberOfCharsRead,
  _In_opt_ PCONSOLE_READCONSOLE_CONTROL pInputControl);

WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
ReadConsoleInputA(
  _In_ HANDLE hConsoleInput,
  _Out_writes_to_(nLength, *lpNumberOfEventsRead) PINPUT_RECORD lpBuffer,
  _In_ DWORD nLength,
  _Out_ _Deref_out_range_(<=, nLength) LPDWORD lpNumberOfEventsRead);

WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
ReadConsoleInputW(
  _In_ HANDLE hConsoleInput,
  _Out_writes_to_(nLength, *lpNumberOfEventsRead) PINPUT_RECORD lpBuffer,
  _In_ DWORD nLength,
  _Out_ _Deref_out_range_(<=, nLength) LPDWORD lpNumberOfEventsRead);

WINBASEAPI
BOOL
WINAPI
ReadConsoleOutputAttribute(
  _In_ HANDLE hConsoleOutput,
  _Out_writes_(nLength) LPWORD lpAttribute,
  _In_ DWORD nLength,
  _In_ COORD dwReadCoord,
  _Out_ LPDWORD lpNumberOfAttrsRead);

WINBASEAPI
BOOL
WINAPI
ReadConsoleOutputCharacterA(
  _In_ HANDLE hConsoleOutput,
  _Out_writes_(nLength) LPSTR lpCharacter,
  _In_ DWORD nLength,
  _In_ COORD dwReadCoord,
  _Out_ LPDWORD lpNumberOfCharsRead);

WINBASEAPI
BOOL
WINAPI
ReadConsoleOutputCharacterW(
  _In_ HANDLE hConsoleOutput,
  _Out_writes_(nLength) LPWSTR lpCharacter,
  _In_ DWORD nLength,
  _In_ COORD dwReadCoord,
  _Out_ LPDWORD lpNumberOfCharsRead);

WINBASEAPI
BOOL
WINAPI
ReadConsoleOutputA(
  _In_ HANDLE hConsoleOutput,
  _Out_writes_(dwBufferSize.X * dwBufferSize.Y) PCHAR_INFO lpBuffer,
  _In_ COORD dwBufferSize,
  _In_ COORD dwBufferCoord,
  _Inout_ PSMALL_RECT lpReadRegion);

WINBASEAPI
BOOL
WINAPI
ReadConsoleOutputW(
  _In_ HANDLE hConsoleOutput,
  _Out_writes_(dwBufferSize.X * dwBufferSize.Y) PCHAR_INFO lpBuffer,
  _In_ COORD dwBufferSize,
  _In_ COORD dwBufferCoord,
  _Inout_ PSMALL_RECT lpReadRegion);

WINBASEAPI
BOOL
WINAPI
ScrollConsoleScreenBufferA(
  _In_ HANDLE hConsoleOutput,
  _In_ CONST SMALL_RECT *lpScrollRectangle,
  _In_opt_ CONST SMALL_RECT *lpClipRectangle,
  _In_ COORD dwDestinationOrigin,
  _In_ CONST CHAR_INFO *lpFill);

WINBASEAPI
BOOL
WINAPI
ScrollConsoleScreenBufferW(
  _In_ HANDLE hConsoleOutput,
  _In_ CONST SMALL_RECT *lpScrollRectangle,
  _In_opt_ CONST SMALL_RECT *lpClipRectangle,
  _In_ COORD dwDestinationOrigin,
  _In_ CONST CHAR_INFO *lpFill);

WINBASEAPI
BOOL
WINAPI
SetConsoleActiveScreenBuffer(
  _In_ HANDLE hConsoleOutput);

WINBASEAPI
BOOL
WINAPI
SetConsoleCP(
  _In_ UINT wCodePageID);

WINBASEAPI
BOOL
WINAPI
SetConsoleCtrlHandler(
  _In_opt_ PHANDLER_ROUTINE HandlerRoutine,
  _In_ BOOL Add);

WINBASEAPI
BOOL
WINAPI
SetConsoleCursorInfo(
  _In_ HANDLE hConsoleOutput,
  _In_ CONST CONSOLE_CURSOR_INFO *lpConsoleCursorInfo);

WINBASEAPI
BOOL
WINAPI
SetConsoleCursorPosition(
  _In_ HANDLE hConsoleOutput,
  _In_ COORD dwCursorPosition);

WINBASEAPI
BOOL
WINAPI
SetConsoleMode(
  _In_ HANDLE hConsoleHandle,
  _In_ DWORD dwMode);

WINBASEAPI
BOOL
WINAPI
SetConsoleOutputCP(
  _In_ UINT wCodePageID);

WINBASEAPI
BOOL
WINAPI
SetConsoleScreenBufferSize(
  _In_ HANDLE hConsoleOutput,
  _In_ COORD dwSize);

WINBASEAPI
BOOL
WINAPI
SetConsoleTextAttribute(
  _In_ HANDLE hConsoleOutput,
  _In_ WORD wAttributes);

WINBASEAPI
BOOL
WINAPI
SetConsoleTitleA(
  _In_ LPCSTR lpConsoleTitle);

WINBASEAPI
BOOL
WINAPI
SetConsoleTitleW(
  _In_ LPCWSTR lpConsoleTitle);

WINBASEAPI
BOOL
WINAPI
SetConsoleWindowInfo(
  _In_ HANDLE hConsoleOutput,
  _In_ BOOL bAbsolute,
  _In_ CONST SMALL_RECT *lpConsoleWindow);

WINBASEAPI
BOOL
WINAPI
WriteConsoleA(
  _In_ HANDLE hConsoleOutput,
  _In_reads_(nNumberOfCharsToWrite) CONST VOID *lpBuffer,
  _In_ DWORD nNumberOfCharsToWrite,
  _Out_opt_ LPDWORD lpNumberOfCharsWritten,
  _Reserved_ LPVOID lpReserved);

WINBASEAPI
BOOL
WINAPI
WriteConsoleW(
  _In_ HANDLE hConsoleOutput,
  _In_reads_(nNumberOfCharsToWrite) CONST VOID *lpBuffer,
  _In_ DWORD nNumberOfCharsToWrite,
  _Out_opt_ LPDWORD lpNumberOfCharsWritten,
  _Reserved_ LPVOID lpReserved);

WINBASEAPI
BOOL
WINAPI
WriteConsoleInputA(
  _In_ HANDLE hConsoleInput,
  _In_reads_(nLength) CONST INPUT_RECORD *lpBuffer,
  _In_ DWORD nLength,
  _Out_ LPDWORD lpNumberOfEventsWritten);

WINBASEAPI
BOOL
WINAPI
WriteConsoleInputW(
  _In_ HANDLE hConsoleInput,
  _In_reads_(nLength) CONST INPUT_RECORD *lpBuffer,
  _In_ DWORD nLength,
  _Out_ LPDWORD lpNumberOfEventsWritten);

WINBASEAPI
BOOL
WINAPI
WriteConsoleOutputA(
  _In_ HANDLE hConsoleOutput,
  _In_reads_(dwBufferSize.X * dwBufferSize.Y) CONST CHAR_INFO *lpBuffer,
  _In_ COORD dwBufferSize,
  _In_ COORD dwBufferCoord,
  _Inout_ PSMALL_RECT lpWriteRegion);

WINBASEAPI
BOOL
WINAPI
WriteConsoleOutputW(
  _In_ HANDLE hConsoleOutput,
  _In_reads_(dwBufferSize.X * dwBufferSize.Y) CONST CHAR_INFO *lpBuffer,
  _In_ COORD dwBufferSize,
  _In_ COORD dwBufferCoord,
  _Inout_ PSMALL_RECT lpWriteRegion);

WINBASEAPI
BOOL
WINAPI
WriteConsoleOutputAttribute(
  _In_ HANDLE hConsoleOutput,
  _In_reads_(nLength) CONST WORD *lpAttribute,
  _In_ DWORD nLength,
  _In_ COORD dwWriteCoord,
  _Out_ LPDWORD lpNumberOfAttrsWritten);

WINBASEAPI
BOOL
WINAPI
WriteConsoleOutputCharacterA(
  _In_ HANDLE hConsoleOutput,
  _In_reads_(nLength) LPCSTR lpCharacter,
  _In_ DWORD nLength,
  _In_ COORD dwWriteCoord,
  _Out_ LPDWORD lpNumberOfCharsWritten);

WINBASEAPI
BOOL
WINAPI
WriteConsoleOutputCharacterW(
  _In_ HANDLE hConsoleOutput,
  _In_reads_(nLength) LPCWSTR lpCharacter,
  _In_ DWORD nLength,
  _In_ COORD dwWriteCoord,
  _Out_ LPDWORD lpNumberOfCharsWritten);


#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)
// typedef VOID *HPCON;
// CreatePseudoConsole()
// ResizePseudoConsole()
// ClosePseudoConsole()
#endif // (NTDDI_VERSION >= NTDDI_WIN10_RS5)

#if (NTDDI_VERSION >= NTDDI_WIN11_GE)
/* See https://github.com/microsoft/terminal/blob/main/doc/specs/%237335%20-%20Console%20Allocation%20Policy.md
 * and https://github.com/MicrosoftDocs/Console-Docs/pull/323 */
// ALLOC_CONSOLE_MODE, ALLOC_CONSOLE_OPTIONS, ALLOC_CONSOLE_RESULT
// AllocConsoleWithOptions()
// ReleasePseudoConsole()
#endif // (NTDDI_VERSION >= NTDDI_WIN11_GE)


#ifdef UNICODE
#define AddConsoleAlias AddConsoleAliasW
#define GetConsoleAlias GetConsoleAliasW
#define GetConsoleAliases GetConsoleAliasesW
#define GetConsoleAliasesLength GetConsoleAliasesLengthW
#define GetConsoleAliasExes GetConsoleAliasExesW
#define GetConsoleAliasExesLength GetConsoleAliasExesLengthW
#define GetConsoleTitle GetConsoleTitleW
#define GetConsoleOriginalTitle GetConsoleOriginalTitleW
#define PeekConsoleInput PeekConsoleInputW
#define ReadConsole ReadConsoleW
#define ReadConsoleInput ReadConsoleInputW
#define ReadConsoleOutput ReadConsoleOutputW
#define ReadConsoleOutputCharacter ReadConsoleOutputCharacterW
#define ScrollConsoleScreenBuffer ScrollConsoleScreenBufferW
#define SetConsoleTitle SetConsoleTitleW
#define WriteConsole WriteConsoleW
#define WriteConsoleInput WriteConsoleInputW
#define WriteConsoleOutput WriteConsoleOutputW
#define FillConsoleOutputCharacter FillConsoleOutputCharacterW
#define WriteConsoleOutputCharacter WriteConsoleOutputCharacterW
#else
#define AddConsoleAlias AddConsoleAliasA
#define GetConsoleAlias GetConsoleAliasA
#define GetConsoleAliases GetConsoleAliasesA
#define GetConsoleAliasesLength GetConsoleAliasesLengthA
#define GetConsoleAliasExes GetConsoleAliasExesA
#define GetConsoleAliasExesLength GetConsoleAliasExesLengthA
#define GetConsoleTitle GetConsoleTitleA
#define GetConsoleOriginalTitle GetConsoleOriginalTitleA
#define PeekConsoleInput PeekConsoleInputA
#define ReadConsole ReadConsoleA
#define ReadConsoleInput ReadConsoleInputA
#define ReadConsoleOutput ReadConsoleOutputA
#define ReadConsoleOutputCharacter ReadConsoleOutputCharacterA
#define ScrollConsoleScreenBuffer ScrollConsoleScreenBufferA
#define SetConsoleTitle SetConsoleTitleA
#define WriteConsole WriteConsoleA
#define WriteConsoleInput WriteConsoleInputA
#define WriteConsoleOutput WriteConsoleOutputA
#define FillConsoleOutputCharacter FillConsoleOutputCharacterA
#define WriteConsoleOutputCharacter WriteConsoleOutputCharacterA
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _WINCON_ */
