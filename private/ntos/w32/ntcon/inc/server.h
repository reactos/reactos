/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    server.h

Abstract:

    This module contains the internal structures and definitions used
    by the console server.

Author:

    Therese Stowell (thereses) 12-Nov-1990

Revision History:

--*/

#ifndef _SERVER_H_
#define _SERVER_H_

//
// This message is used to notify the input thread that a console window should
// go away.
//

typedef struct _CONSOLE_SHARE_ACCESS {
    ULONG OpenCount;
    ULONG Readers;
    ULONG Writers;
    ULONG SharedRead;
    ULONG SharedWrite;
} CONSOLE_SHARE_ACCESS, *PCONSOLE_SHARE_ACCESS;

#include "input.h"
#include "output.h"
#if defined(FE_IME)
#include "conime.h"
#include "convarea.h"
#endif

typedef struct _CONSOLE_PROCESS_HANDLE {
    LIST_ENTRY ListLink;
    HANDLE ProcessHandle;
    PCSR_PROCESS Process;
    ULONG TerminateCount;
    LPTHREAD_START_ROUTINE CtrlRoutine;
    LPTHREAD_START_ROUTINE PropRoutine;
} CONSOLE_PROCESS_HANDLE, *PCONSOLE_PROCESS_HANDLE;

typedef struct _INPUT_THREAD_INFO {
    HANDLE ThreadHandle;
    DWORD ThreadId;
    HDESK Desktop;
    ULONG WindowCount;
#if defined(FE_IME)
    HWND  hWndConsoleIME;
#endif
} INPUT_THREAD_INFO, *PINPUT_THREAD_INFO;

typedef struct _INPUT_THREAD_INIT_INFO {
    HANDLE ThreadHandle;
    HANDLE InitCompleteEventHandle;
    HDESK DesktopHandle;
    NTSTATUS InitStatus;
} INPUT_THREAD_INIT_INFO, *PINPUT_THREAD_INIT_INFO;

typedef struct _CONSOLE_MSG {
    LIST_ENTRY ListLink;
    UINT       Message;
    WPARAM     wParam;
    LPARAM     lParam;
} CONSOLE_MSG, *PCONSOLE_MSG;

typedef struct _CONSOLE_THREAD_MSG {
    CONSOLE_MSG;
    DWORD dwThreadId;
} CONSOLE_THREAD_MSG, *PCONSOLE_THREAD_MSG;

// Flags flags

#define CONSOLE_IS_ICONIC               0x00000001
#define CONSOLE_OUTPUT_SUSPENDED        0x00000002
#define CONSOLE_HAS_FOCUS               0x00000004
#define CONSOLE_IGNORE_NEXT_MOUSE_INPUT 0x00000008
#define CONSOLE_SELECTING               0x00000010
#define CONSOLE_SCROLLING               0x00000020
#define CONSOLE_DISABLE_CLOSE           0x00000040
#define CONSOLE_NOTIFY_LAST_CLOSE       0x00000080
#define CONSOLE_NO_WINDOW               0x00000100
#define CONSOLE_VDM_REGISTERED          0x00000200
#define CONSOLE_UPDATING_SCROLL_BARS    0x00000400
#define CONSOLE_QUICK_EDIT_MODE         0x00000800
#define CONSOLE_TERMINATING             0x00001000
#define CONSOLE_CONNECTED_TO_EMULATOR   0x00002000
#define CONSOLE_FULLSCREEN_NOPAINT      0x00004000
#define CONSOLE_SHUTTING_DOWN           0x00008000
#define CONSOLE_AUTO_POSITION           0x00010000
#define CONSOLE_IGNORE_NEXT_KEYUP       0x00020000
#define CONSOLE_WOW_REGISTERED          0x00040000
#define CONSOLE_USE_PRIVATE_FLAGS       0x00080000
#define CONSOLE_HISTORY_NODUP           0x00100000
#define CONSOLE_SCROLLBAR_TRACKING      0x00200000
#define CONSOLE_IN_DESTRUCTION          0x00400000
#define CONSOLE_SETTING_WINDOW_SIZE     0x00800000
#define CONSOLE_DEFAULT_BUFFER_SIZE     0x01000000
#if defined(FE_SB)
#define CONSOLE_OS2_REGISTERED          0x20000000
#define CONSOLE_OS2_OEM_FORMAT          0x40000000
#if defined(FE_IME)
#define CONSOLE_JUST_VDM_UNREGISTERED   0x80000000
#endif // FE_IME
#endif

#define CONSOLE_SUSPENDED (CONSOLE_OUTPUT_SUSPENDED)

// SelectionFlags flags

#define CONSOLE_NO_SELECTION 0
#define CONSOLE_SELECTION_NOT_EMPTY 1   // non-null select rectangle
#define CONSOLE_MOUSE_SELECTION 2       // selecting with mouse
#define CONSOLE_MOUSE_DOWN 4            // mouse is down
#define CONSOLE_SELECTION_INVERTED 8    // selection is inverted (turned off)

typedef struct _CONSOLE_INFORMATION {
    CRITICAL_SECTION ConsoleLock;   // serialize input and output using this
    ULONG RefCount;
    ULONG WaitCount;
    INPUT_INFORMATION InputBuffer;
    PSCREEN_INFORMATION CurrentScreenBuffer;
    PSCREEN_INFORMATION ScreenBuffers;  // singly linked list
    HWINSTA hWinSta;                // server handle to windowstation
    HDESK hDesk;                    // server handle to desktop
    HWND hWnd;
    HKL hklActive;                  // keyboard layout for this console window
    HDC hDC;                        // server side hDC
    HMENU hMenu;                    // handle to system menu
    HMENU hHeirMenu;                // handle to menu we append to system menu
    HPALETTE hSysPalette;
    RECT WindowRect;
    DWORD ResizeFlags;
    LIST_ENTRY OutputQueue;
    HANDLE InitEvents[NUMBER_OF_INITIALIZATION_EVENTS];
    HANDLE ClientThreadHandle;
    LIST_ENTRY ProcessHandleList;
    LIST_ENTRY CommandHistoryList;
    LIST_ENTRY ExeAliasList;
    SHORT NumCommandHistories;
    SHORT MaxCommandHistories;
    SHORT CommandHistorySize;
    USHORT OriginalTitleLength;
    USHORT TitleLength;
    LPWSTR OriginalTitle;
    LPWSTR Title;
    DWORD dwHotKey;
    HICON hIcon;
    HICON hSmIcon;
    INT iIconId;
    WORD LastAttributes;
    BYTE ReserveKeys;           // keys reserved by app (i.e. ctrl-esc)
    DWORD Flags;

    // if a wait has been satisfied, the pointer to the wait queue is stored
    // here.

    PLIST_ENTRY WaitQueue;

    // the following fields are used for selection

    DWORD SelectionFlags;
    SMALL_RECT SelectionRect;
    COORD SelectionAnchor;
    COORD TextCursorPosition;   // current position on screen (in screen buffer coords).
    ULONG TextCursorSize;
    BOOLEAN TextCursorVisible;    // whether cursor is visible (set by user)

    BOOLEAN InsertMode;     // used by command line editing

    // following fields are used when window is created

    WORD wShowWindow;
    int dwWindowOriginX;
    int dwWindowOriginY;

    WORD FullScreenFlags;
    WORD PopupCount;

    // following fields are used for the VDM

    HANDLE VDMStartHardwareEvent;
    HANDLE VDMEndHardwareEvent;
    HANDLE VDMProcessHandle;
    HANDLE VDMProcessId;

    HANDLE VDMBufferSectionHandle;
    PCHAR_INFO VDMBuffer;
    PCHAR_INFO VDMBufferClient;
    COORD VDMBufferSize;

    HANDLE StateSectionHandle; // used for get/sethardwarestate
    PVOID StateBuffer;
    PVOID StateBufferClient;
    DWORD StateLength;

    // the following fields are used for ansi-unicode translation

    UINT CP;
    UINT OutputCP;

    //
    // these two fields are used while getting the icon from the
    // program manager via DDE.
    //

    HWND hWndProgMan;
    BOOL bIconInit;

    HANDLE ConsoleHandle;

    ULONG CtrlFlags;            // indicates outstanding ctrl requests
    ULONG LimitingProcessId;
    HANDLE TerminationEvent;

    SHORT VerticalClientToWindow;
    SHORT HorizontalClientToWindow;

    COLORREF  ColorTable[ 16 ];
    HANDLE hProcessLastNotifyClose;     // process handle of last-close-notify
    HANDLE ProcessIdLastNotifyClose;    // process unique id of last-close-notify
    HWND hWndProperties;

    PINPUT_THREAD_INFO InputThreadInfo;     // console thread info

    LIST_ENTRY MessageQueue;

#if defined(FE_SB)
    CPINFO CPInfo;
    CPINFO OutputCPInfo;

    DWORD ReadConInpNumBytesUnicode;
    DWORD ReadConInpNumBytesTemp;

    DWORD WriteConOutNumBytesUnicode;
    DWORD WriteConOutNumBytesTemp;

    PVOID lpCookedReadData;             // Same as PCOOKED_READ_DATA

    PVOID EudcInformation;              // Same as PEUDC_INFORMATION

    PVOID FontCacheInformation;         // Same as PFONT_CACHE_INFORMATION

#if defined(FE_IME)
    CONSOLE_IME_INFORMATION ConsoleIme;
#endif // FE_IME

    HDC FonthDC;                        // Double colored DBCS hDC
    HBITMAP hBitmap;
#if defined(i386)
    SMALL_RECT Os2SavedWindowRect;
#endif
    BOOLEAN fVDMVideoMode;              // FALSE : VGA Format
                                        // TRUE  : Common LVB Format
    BOOLEAN fIsDBCSCP;
    BOOLEAN fIsDBCSOutputCP;

#endif
} CONSOLE_INFORMATION, *PCONSOLE_INFORMATION;

//
// CtrlFlags definitions
//

#define CONSOLE_CTRL_C_FLAG                     1
#define CONSOLE_CTRL_BREAK_FLAG                 2
#define CONSOLE_CTRL_CLOSE_FLAG                 4
#define CONSOLE_FORCE_SHUTDOWN_FLAG             8
#define CONSOLE_CTRL_LOGOFF_FLAG                16
#define CONSOLE_CTRL_SHUTDOWN_FLAG              32

#define ADD_SCROLL_BARS_X 0x1
#define REMOVE_SCROLL_BARS_X 0x2
#define ADD_SCROLL_BARS_Y 0x4
#define REMOVE_SCROLL_BARS_Y 0x8
#define RESIZE_SCROLL_BARS 0x10
#define SCROLL_BAR_CHANGE (ADD_SCROLL_BARS_X | REMOVE_SCROLL_BARS_X | ADD_SCROLL_BARS_Y | REMOVE_SCROLL_BARS_Y | RESIZE_SCROLL_BARS)
#define BORDER_CHANGE 0x20
#define SCREEN_BUFFER_CHANGE 0x40

#define CONSOLE_INITIAL_IO_HANDLES 3
#define CONSOLE_IO_HANDLE_INCREMENT 3

#define CONSOLE_FREE_HANDLE 0
//#define CONSOLE_INPUT_HANDLE 1
//#define CONSOLE_OUTPUT_HANDLE 2
#define CONSOLE_GRAPHICS_OUTPUT_HANDLE 4
#define CONSOLE_INHERITABLE 8
#define CONSOLE_ANY_HANDLE ((ULONG)(-1))

//
// input handle flags
//

#define HANDLE_CLOSING 1
#define HANDLE_INPUT_PENDING 2
#define HANDLE_MULTI_LINE_INPUT 4

typedef struct _HANDLE_DATA {
    ULONG HandleType;
    ACCESS_MASK Access;
    ULONG ShareAccess;
    union {
        PSCREEN_INFORMATION ScreenBuffer;
        PINPUT_INFORMATION InputBuffer;
    } Buffer;
    PINPUT_READ_HANDLE_DATA InputReadData; // used only by input reads
} HANDLE_DATA, *PHANDLE_DATA;

typedef struct _CONSOLE_PER_PROCESS_DATA {
    HANDLE ConsoleHandle;
    HANDLE_DATA HandleTable[CONSOLE_INITIAL_IO_HANDLES];
    ULONG HandleTableSize;
    ULONG Foo;
    PHANDLE_DATA HandleTablePtr;
    BOOLEAN ConsoleApp;
    BOOLEAN RootProcess;
#if defined(FE_IME)
    HDESK hDesk;
    HWINSTA hWinSta;
#endif
} CONSOLE_PER_PROCESS_DATA, *PCONSOLE_PER_PROCESS_DATA;

#define CONSOLE_INITIAL_CONSOLES 10
#define CONSOLE_CONSOLE_HANDLE_INCREMENT 5
#define CONSOLE_HANDLE_ALLOCATED 1

#define INDEX_TO_HANDLE(INDEX) ((HANDLE)(((ULONG_PTR)INDEX << 2) | CONSOLE_HANDLE_SIGNATURE))
#define HANDLE_TO_INDEX(CONHANDLE) ((HANDLE)((ULONG_PTR)CONHANDLE >> 2))

#define INPUT_MODES (ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT)
#define OUTPUT_MODES (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT)
#define PRIVATE_MODES (ENABLE_INSERT_MODE | ENABLE_QUICK_EDIT_MODE | ENABLE_PRIVATE_FLAGS)

#define CURSOR_PERCENTAGE_TO_TOP_SCAN_LINE(FONTSIZE,PERCENTAGE) ((FONTSIZE) - ((FONTSIZE) * (PERCENTAGE) / 100))

#define ConsoleLocked(CONSOLEPTR) (((CONSOLEPTR)->ConsoleLock.OwningThread) == NtCurrentTeb()->ClientId.UniqueThread)

#define CONSOLE_STATUS_WAIT ((NTSTATUS)0xC0030001L)
#define CONSOLE_STATUS_READ_COMPLETE ((NTSTATUS)0xC0030002L)
#define CONSOLE_STATUS_WAIT_NO_BLOCK ((NTSTATUS)0xC0030003L)

#define CM_CREATE_CONSOLE_WINDOW (WM_USER+0)
#define CM_DESTROY_WINDOW        (WM_USER+1)
#define CM_SET_WINDOW_SIZE       (WM_USER+2)
#define CM_BEEP                  (WM_USER+3)
#define CM_UPDATE_SCROLL_BARS    (WM_USER+4)
#define CM_UPDATE_TITLE          (WM_USER+5)
//
// CM_MODE_TRANSITION is hard-coded to WM_USER+6 in kernel\winmgr.c
//
#define CM_MODE_TRANSITION       (WM_USER+6)
#define CM_CONSOLE_SHUTDOWN      (WM_USER+7)
#define CM_HIDE_WINDOW           (WM_USER+8)
#if defined(FE_IME)
#define CM_CONIME_CREATE         (WM_USER+9)
#define CM_SET_CONSOLEIME_WINDOW (WM_USER+10)
#define CM_WAIT_CONIME_PROCESS   (WM_USER+11)
#define CM_SET_IME_CODEPAGE      (WM_USER+12)
#define CM_SET_NLSMODE           (WM_USER+13)
#define CM_GET_NLSMODE           (WM_USER+14)
#define CM_CONIME_KL_ACTIVATE    (WM_USER+15)
#endif
#define CM_CONSOLE_MSG           (WM_USER+16)
#define CM_CONSOLE_INPUT_THREAD_MSG (WM_USER+17)

#define CONSOLE_CLIENTTHREADHANDLE(pcsrthread) ((pcsrthread)->ThreadHandle)

#define CONSOLE_CLIENTPROCESSHANDLE() \
    ((CSR_SERVER_QUERYCLIENTTHREAD())->Process->ProcessHandle)

#define CONSOLE_CLIENTPROCESSID() \
    ((CSR_SERVER_QUERYCLIENTTHREAD())->Process->ClientId.UniqueProcess)

#define CONSOLE_FROMPROCESSPROCESSHANDLE(pcsrprocess) \
                                               ((pcsrprocess)->ProcessHandle)

#define CONSOLE_FROMPROCESSPERPROCESSDATA(pcsrprocess) \
    ((pcsrprocess)->ServerDllPerProcessData[CONSRV_SERVERDLL_INDEX])

#define CONSOLE_FROMTHREADPERPROCESSDATA(pcsrthread) \
    CONSOLE_FROMPROCESSPERPROCESSDATA((pcsrthread)->Process)

#define CONSOLE_PERPROCESSDATA() \
    CONSOLE_FROMTHREADPERPROCESSDATA(CSR_SERVER_QUERYCLIENTTHREAD())

#define CONSOLE_GETCONSOLEAPP() (((PCONSOLE_PER_PROCESS_DATA)CONSOLE_PERPROCESSDATA())->ConsoleApp)
#define CONSOLE_GETCONSOLEAPPFROMPROCESSDATA(PROCESSDATA) ((PROCESSDATA)->ConsoleApp)
#define CONSOLE_SETCONSOLEAPP(VALUE) (((PCONSOLE_PER_PROCESS_DATA)CONSOLE_PERPROCESSDATA())->ConsoleApp = VALUE)
#define CONSOLE_SETCONSOLEAPPFROMPROCESSDATA(PROCESSDATA,VALUE) ((PROCESSDATA)->ConsoleApp = VALUE)

#define CONSOLE_GETCONSOLEHANDLE() (((PCONSOLE_PER_PROCESS_DATA)CONSOLE_PERPROCESSDATA())->ConsoleHandle)
#define CONSOLE_SETCONSOLEHANDLE(VALUE) (((PCONSOLE_PER_PROCESS_DATA)CONSOLE_PERPROCESSDATA())->ConsoleHandle = VALUE)
#define CONSOLE_GETCONSOLEHANDLEFROMPROCESSDATA(PROCESSDATA) ((PROCESSDATA)->ConsoleHandle)
#define CONSOLE_SETCONSOLEHANDLEFROMPROCESSDATA(PROCESSDATA,VALUE) ((PROCESSDATA)->ConsoleHandle = VALUE)

#endif

//
// registry information structure
//

typedef struct _CONSOLE_REGISTRY_INFO {
    COORD     ScreenBufferSize;
    COORD     WindowSize;
    COORD     WindowOrigin;
    COORD     FontSize;
    UINT      FontFamily;
    UINT      FontWeight;
    WCHAR     FaceName[LF_FACESIZE];
    UINT      CursorSize;
    BOOL      FullScreen;
    BOOL      QuickEdit;
    BOOL      InsertMode;
    BOOL      AutoPosition;
    CHAR_INFO ScreenFill;
    CHAR_INFO PopupFill;
    UINT      HistoryBufferSize;
    UINT      NumberOfHistoryBuffers;
    BOOL      HistoryNoDup;
    COLORREF  ColorTable[ 16 ];
    LONGLONG  LastWriteTime;
#if defined(FE_SB) // scotthsu
    DWORD     CodePage;
#endif
} CONSOLE_REGISTRY_INFO, *PCONSOLE_REGISTRY_INFO;


//
// window class
//

#define CONSOLE_WINDOW_CLASS (L"ConsoleWindowClass")

#define CONSOLE_MAX_APP_SHORTCUTS 1

//
// this structure is used to store relevant information from the
// console for ctrl processing so we can do it without holding the
// console lock.
//

typedef struct _CONSOLE_PROCESS_TERMINATION_RECORD {
    HANDLE ProcessHandle;
    ULONG TerminateCount;
    BOOL bDebugee;
    LPTHREAD_START_ROUTINE CtrlRoutine;
} CONSOLE_PROCESS_TERMINATION_RECORD, *PCONSOLE_PROCESS_TERMINATION_RECORD;

//
// this value is used to determine the size of stack buffers for
// strings.  it should be long enough to contain the width of a
// normal screen buffer.
//

#define STACK_BUFFER_SIZE 132

//
// link information
//


#define LINK_PROP_MAIN_SIG          0x00000001
#define LINK_PROP_NT_CONSOLE_SIG    0x00000002

#if 0  // no one currently uses this...
typedef struct {

    WCHAR  pszLinkName[ MAX_PATH ];
    WCHAR  pszName[ MAX_PATH ];
    WCHAR  pszRelPath[ MAX_PATH ];
    WCHAR  pszWorkingDir[ MAX_PATH ];
    WCHAR  pszArgs[ MAX_PATH ];
    WCHAR  pszIconLocation[ MAX_PATH ];
    int    iIcon;
    int    iShowCmd;
    int    wHotKey;

} LNKPROPMAIN, * LPLNKPROPMAIN;
#endif

#ifndef _USERKDX_ /* debugging extensions */
typedef struct {

    WCHAR    pszName[ MAX_PATH ];
    WCHAR    pszIconLocation[ MAX_PATH ];
    UINT     uIcon;
    UINT     uShowCmd;
    UINT     uHotKey;
    NT_CONSOLE_PROPS console_props;
#ifdef FE_SB
    NT_FE_CONSOLE_PROPS fe_console_props;
#endif


} LNKPROPNTCONSOLE, *LPLNKPROPNTCONSOLE;
#endif

#ifndef _USERKDX_ /* debugging extensions */
typedef struct {
    LPWSTR              pszCurFile;     // current file from IPersistFile
    LPWSTR              pszRelSource;   // overrides pszCurFile in relative tracking

    LPWSTR              pszName;        // title on short volumes
    LPWSTR              pszRelPath;
    LPWSTR              pszWorkingDir;
    LPWSTR              pszArgs;
    LPWSTR              pszIconLocation;

    LPSTR               pExtraData;     // extra data to preserve for future compatibility

    SHELL_LINK_DATA     sld;
} CShellLink;
#endif

