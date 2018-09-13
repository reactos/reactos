/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    conmsg.h

Abstract:

    This include file defines the message formats used to communicate
    between the client and server portions of the CONSOLE portion of the
    Windows subsystem.

Author:

    Therese Stowell (thereses) 10-Nov-1990

Revision History:

--*/

#ifndef _CONMSG_H_
#define _CONMSG_H_

#define CONSOLE_INPUT_HANDLE 1
#define CONSOLE_OUTPUT_HANDLE 2

// max lengths, in bytes!
#define MAX_TITLE_LENGTH ((MAX_PATH+1)*sizeof(WCHAR))
#define MAX_APP_NAME_LENGTH 256

#define INITIALIZATION_SUCCEEDED 0
#define INITIALIZATION_FAILED 1
#define NUMBER_OF_INITIALIZATION_EVENTS 2

#if defined(FE_SB) // for Kernel32 Single Binary
#include "winconp.h"  // need FONT_SELECT
#endif // FE_SB

typedef struct _CONSOLE_INFO {
    IN OUT HANDLE ConsoleHandle;
    OUT HANDLE InputWaitHandle;
    OUT HANDLE StdIn;
    OUT HANDLE StdOut;
    OUT HANDLE StdErr;
    OUT HANDLE InitEvents[NUMBER_OF_INITIALIZATION_EVENTS];
    IN UINT   iIconId;
    IN HICON  hIcon;
    IN HICON  hSmIcon;
    IN DWORD dwHotKey;
    IN DWORD dwStartupFlags;
    IN WORD  wFillAttribute OPTIONAL;
    IN WORD  wPopupFillAttribute OPTIONAL;
    IN WORD  wShowWindow OPTIONAL;
    IN WORD  wReserved OPTIONAL;
    IN COORD dwScreenBufferSize OPTIONAL;
    IN COORD dwWindowSize OPTIONAL;
    IN COORD dwWindowOrigin OPTIONAL;
    IN DWORD nFont OPTIONAL;
    IN DWORD nInputBufferSize OPTIONAL;
    IN COORD dwFontSize OPTIONAL;
    IN UINT  uFontFamily OPTIONAL;
    IN UINT  uFontWeight OPTIONAL;
    IN WCHAR FaceName[LF_FACESIZE];
    IN UINT  uCursorSize OPTIONAL;
    IN BOOL  bFullScreen OPTIONAL;
    IN BOOL  bQuickEdit  OPTIONAL;
    IN BOOL  bInsertMode OPTIONAL;
    IN BOOL  bAutoPosition OPTIONAL;
    IN UINT  uHistoryBufferSize OPTIONAL;
    IN UINT  uNumberOfHistoryBuffers OPTIONAL;
    IN BOOL  bHistoryNoDup OPTIONAL;
    IN COLORREF ColorTable[ 16 ] OPTIONAL;
#if defined(FE_SB) // for Kernel32 Single Binary
    IN UINT  uCodePage;
#endif // FE_SB
} CONSOLE_INFO, *PCONSOLE_INFO;


//
// This structure is filled in by the client prior to connecting to the CONSRV
// DLL in the Windows subsystem server.  The server DLL will fill in the OUT
// fields if prior to accepting the connection.
//

typedef struct _CONSOLE_API_CONNECTINFO {
    IN OUT CONSOLE_INFO ConsoleInfo;
    IN BOOLEAN ConsoleApp;
    IN BOOLEAN WindowVisible;
    IN LPTHREAD_START_ROUTINE CtrlRoutine;
    IN LPTHREAD_START_ROUTINE PropRoutine;
#if defined(FE_SB)
#if defined(FE_IME)
    IN LPTHREAD_START_ROUTINE ConsoleIMERoutine;
#endif // FE_IME
#endif // FE_SB
    IN DWORD TitleLength;
    IN WCHAR Title[MAX_TITLE_LENGTH/2];
    IN DWORD DesktopLength;
    IN LPWSTR Desktop;
    IN DWORD AppNameLength;
    IN WCHAR AppName[MAX_APP_NAME_LENGTH/2];
    IN DWORD CurDirLength;
    IN WCHAR CurDir[MAX_PATH+1];
} CONSOLE_API_CONNECTINFO, *PCONSOLE_API_CONNECTINFO;

//
// Message format for messages sent from the client to the server
//

typedef enum _CONSOLE_API_NUMBER {
    ConsolepOpenConsole = CONSRV_FIRST_API_NUMBER,
    ConsolepGetConsoleInput,
    ConsolepWriteConsoleInput,
    ConsolepReadConsoleOutput,
    ConsolepWriteConsoleOutput,
    ConsolepReadConsoleOutputString,
    ConsolepWriteConsoleOutputString,
    ConsolepFillConsoleOutput,
    ConsolepGetMode,
    ConsolepGetNumberOfFonts,
    ConsolepGetNumberOfInputEvents,
    ConsolepGetScreenBufferInfo,
    ConsolepGetCursorInfo,
    ConsolepGetMouseInfo,
    ConsolepGetFontInfo,
    ConsolepGetFontSize,
    ConsolepGetCurrentFont,
    ConsolepSetMode,
    ConsolepSetActiveScreenBuffer,
    ConsolepFlushInputBuffer,
    ConsolepGetLargestWindowSize,
    ConsolepSetScreenBufferSize,
    ConsolepSetCursorPosition,
    ConsolepSetCursorInfo,
    ConsolepSetWindowInfo,
    ConsolepScrollScreenBuffer,
    ConsolepSetTextAttribute,
    ConsolepSetFont,
    ConsolepSetIcon,
    ConsolepReadConsole,
    ConsolepWriteConsole,
    ConsolepDupHandle,
    ConsolepGetHandleInformation,
    ConsolepSetHandleInformation,
    ConsolepCloseHandle,
    ConsolepVerifyIoHandle,
    ConsolepAlloc,
    ConsolepFree,
    ConsolepGetTitle,
    ConsolepSetTitle,
    ConsolepCreateScreenBuffer,
    ConsolepInvalidateBitmapRect,
    ConsolepVDMOperation,
    ConsolepSetCursor,
    ConsolepShowCursor,
    ConsolepMenuControl,
    ConsolepSetPalette,
    ConsolepSetDisplayMode,
    ConsolepRegisterVDM,
    ConsolepGetHardwareState,
    ConsolepSetHardwareState,
    ConsolepGetDisplayMode,
    ConsolepAddAlias,
    ConsolepGetAlias,
    ConsolepGetAliasesLength,
    ConsolepGetAliasExesLength,
    ConsolepGetAliases,
    ConsolepGetAliasExes,
    ConsolepExpungeCommandHistory,
    ConsolepSetNumberOfCommands,
    ConsolepGetCommandHistoryLength,
    ConsolepGetCommandHistory,
    ConsolepSetCommandHistoryMode,
    ConsolepGetCP,
    ConsolepSetCP,
    ConsolepSetKeyShortcuts,
    ConsolepSetMenuClose,
    ConsolepNotifyLastClose,
    ConsolepGenerateCtrlEvent,
    ConsolepGetKeyboardLayoutName,
    ConsolepGetConsoleWindow,
#if defined(FE_SB) // for Kernel32 Single Binary
    ConsolepCharType,
    ConsolepSetLocalEUDC,
    ConsolepSetCursorMode,
    ConsolepGetCursorMode,
    ConsolepRegisterOS2,
    ConsolepSetOS2OemFormat,
#if defined(FE_IME)
    ConsolepGetNlsMode,
    ConsolepSetNlsMode,
    ConsolepRegisterConsoleIME,
    ConsolepUnregisterConsoleIME,
#endif // FE_IME
#endif // FE_SB
    ConsolepGetLangId,
    ConsolepMaxApiNumber
} CONSOLE_API_NUMBER, *PCONSOLE_API_NUMBER;

typedef struct _CONSOLE_CREATESCREENBUFFER_MSG {
    IN HANDLE ConsoleHandle;
    IN ULONG DesiredAccess;
    IN BOOL InheritHandle;
    IN ULONG ShareMode;
    IN DWORD Flags;
    IN OUT CONSOLE_GRAPHICS_BUFFER_INFO GraphicsBufferInfo;
    OUT HANDLE hMutex;
    OUT PVOID lpBitmap;
    OUT HANDLE Handle;
} CONSOLE_CREATESCREENBUFFER_MSG, *PCONSOLE_CREATESCREENBUFFER_MSG;

typedef struct _CONSOLE_OPENCONSOLE_MSG {
    IN HANDLE ConsoleHandle;
    IN ULONG HandleType;
    IN ULONG DesiredAccess;
    IN BOOL InheritHandle;
    IN ULONG ShareMode;
    OUT HANDLE Handle;
} CONSOLE_OPENCONSOLE_MSG, *PCONSOLE_OPENCONSOLE_MSG;

#define INPUT_RECORD_BUFFER_SIZE 5

typedef struct _CONSOLE_GETCONSOLEINPUT_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE InputHandle;
    OUT INPUT_RECORD Record[INPUT_RECORD_BUFFER_SIZE];
    OUT PINPUT_RECORD  BufPtr;
    IN OUT ULONG  NumRecords;   // this value is valid even for error cases
    IN USHORT Flags;
    BOOLEAN Unicode;
} CONSOLE_GETCONSOLEINPUT_MSG, *PCONSOLE_GETCONSOLEINPUT_MSG;

typedef struct _CONSOLE_WRITECONSOLEINPUT_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE InputHandle;
    IN INPUT_RECORD Record[INPUT_RECORD_BUFFER_SIZE];
    IN PINPUT_RECORD  BufPtr;
    IN OUT ULONG  NumRecords;  // this value is valid even for error cases
    BOOLEAN Unicode;
    BOOLEAN Append;
} CONSOLE_WRITECONSOLEINPUT_MSG, *PCONSOLE_WRITECONSOLEINPUT_MSG;

typedef struct _CONSOLE_READCONSOLEOUTPUT_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    OUT CHAR_INFO Char;
    OUT PCHAR_INFO BufPtr;
    IN OUT SMALL_RECT CharRegion; // this value is valid even for error cases
    BOOLEAN Unicode;
} CONSOLE_READCONSOLEOUTPUT_MSG, *PCONSOLE_READCONSOLEOUTPUT_MSG;

typedef struct _CONSOLE_WRITECONSOLEOUTPUT_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN CHAR_INFO Char;
    IN PCHAR_INFO BufPtr;
    IN OUT SMALL_RECT CharRegion; // this value is valid even for error cases
    BOOLEAN Unicode;
    BOOLEAN ReadVM;
} CONSOLE_WRITECONSOLEOUTPUT_MSG, *PCONSOLE_WRITECONSOLEOUTPUT_MSG;

#define BUFFER_SIZE 80

/*
 * WriteOutputString and ReadInputString types
 */
#define CONSOLE_ASCII             0x1
#define CONSOLE_REAL_UNICODE      0x2
#define CONSOLE_ATTRIBUTE         0x3
#define CONSOLE_FALSE_UNICODE     0x4

typedef struct _CONSOLE_READCONSOLEOUTPUTSTRING_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN COORD ReadCoord;
    IN DWORD  StringType;
    OUT WCHAR String[BUFFER_SIZE/2];
    OUT PWCHAR BufPtr;
    IN OUT ULONG NumRecords; // this value is valid even for error cases
} CONSOLE_READCONSOLEOUTPUTSTRING_MSG, *PCONSOLE_READCONSOLEOUTPUTSTRING_MSG;

typedef struct _CONSOLE_WRITECONSOLEOUTPUTSTRING_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN COORD WriteCoord;
    IN DWORD  StringType;
    OUT WCHAR String[BUFFER_SIZE/2];
    IN PWCHAR BufPtr;
    IN OUT ULONG NumRecords; // this value is valid even for error cases
} CONSOLE_WRITECONSOLEOUTPUTSTRING_MSG, *PCONSOLE_WRITECONSOLEOUTPUTSTRING_MSG;

typedef struct _CONSOLE_FILLCONSOLEOUTPUT_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN COORD WriteCoord;
    IN DWORD  ElementType;
    IN WORD Element;
    IN OUT ULONG Length; // this value is valid even for error cases
} CONSOLE_FILLCONSOLEOUTPUT_MSG, *PCONSOLE_FILLCONSOLEOUTPUT_MSG;

typedef struct _CONSOLE_MODE_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE Handle;
    IN DWORD Mode;
} CONSOLE_MODE_MSG, *PCONSOLE_MODE_MSG;

typedef struct _CONSOLE_GETNUMBEROFFONTS_MSG {
    IN HANDLE ConsoleHandle;
    OUT ULONG NumberOfFonts;
} CONSOLE_GETNUMBEROFFONTS_MSG, *PCONSOLE_GETNUMBEROFFONTS_MSG;

typedef struct _CONSOLE_GETNUMBEROFINPUTEVENTS_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE InputHandle;
    OUT DWORD ReadyEvents;
} CONSOLE_GETNUMBEROFINPUTEVENTS_MSG, *PCONSOLE_GETNUMBEROFINPUTEVENTS_MSG;

typedef struct _CONSOLE_GETLARGESTWINDOWSIZE_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    OUT COORD  Size;
} CONSOLE_GETLARGESTWINDOWSIZE_MSG, *PCONSOLE_GETLARGESTWINDOWSIZE_MSG;

typedef struct _CONSOLE_GETSCREENBUFFERINFO_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    OUT COORD Size;
    OUT COORD CursorPosition;
    OUT COORD ScrollPosition;
    OUT WORD  Attributes;
    OUT COORD CurrentWindowSize;
    OUT COORD MaximumWindowSize;
} CONSOLE_GETSCREENBUFFERINFO_MSG, *PCONSOLE_GETSCREENBUFFERINFO_MSG;

typedef struct _CONSOLE_GETCURSORINFO_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    OUT DWORD CursorSize;
    OUT BOOLEAN Visible;
} CONSOLE_GETCURSORINFO_MSG, *PCONSOLE_GETCURSORINFO_MSG;

typedef struct _CONSOLE_GETMOUSEINFO_MSG {
    IN HANDLE ConsoleHandle;
    OUT ULONG NumButtons;
} CONSOLE_GETMOUSEINFO_MSG, *PCONSOLE_GETMOUSEINFO_MSG;

typedef struct _CONSOLE_GETFONTINFO_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN BOOLEAN MaximumWindow;
    OUT PCHAR BufPtr;
    IN OUT ULONG NumFonts;  // this value is valid even for error cases
} CONSOLE_GETFONTINFO_MSG, *PCONSOLE_GETFONTINFO_MSG;

typedef struct _CONSOLE_GETFONTSIZE_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN DWORD  FontIndex;
    OUT COORD FontSize;
} CONSOLE_GETFONTSIZE_MSG, *PCONSOLE_GETFONTSIZE_MSG;

typedef struct _CONSOLE_GETCURRENTFONT_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN BOOLEAN MaximumWindow;
    OUT DWORD FontIndex;
    OUT COORD FontSize;
} CONSOLE_GETCURRENTFONT_MSG, *PCONSOLE_GETCURRENTFONT_MSG;

typedef struct _CONSOLE_SETACTIVESCREENBUFFER_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
} CONSOLE_SETACTIVESCREENBUFFER_MSG, *PCONSOLE_SETACTIVESCREENBUFFER_MSG;

typedef struct _CONSOLE_FLUSHINPUTBUFFER_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE InputHandle;
} CONSOLE_FLUSHINPUTBUFFER_MSG, *PCONSOLE_FLUSHINPUTBUFFER_MSG;

typedef struct _CONSOLE_SETSCREENBUFFERSIZE_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN COORD  Size;
} CONSOLE_SETSCREENBUFFERSIZE_MSG, *PCONSOLE_SETSCREENBUFFERSIZE_MSG;

typedef struct _CONSOLE_SETCURSORPOSITION_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN COORD  CursorPosition;
} CONSOLE_SETCURSORPOSITION_MSG, *PCONSOLE_SETCURSORPOSITION_MSG;

typedef struct _CONSOLE_SETCURSORINFO_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN DWORD  CursorSize;
    IN BOOLEAN Visible;
} CONSOLE_SETCURSORINFO_MSG, *PCONSOLE_SETCURSORINFO_MSG;

typedef struct _CONSOLE_SETWINDOWINFO_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN BOOL Absolute;
    IN SMALL_RECT Window;
} CONSOLE_SETWINDOWINFO_MSG, *PCONSOLE_SETWINDOWINFO_MSG;

typedef struct _CONSOLE_SCROLLSCREENBUFFER_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN SMALL_RECT ScrollRectangle;
    IN SMALL_RECT ClipRectangle;
    IN BOOL Clip;
    IN COORD  DestinationOrigin;
    IN CHAR_INFO Fill;
    IN BOOLEAN Unicode;
} CONSOLE_SCROLLSCREENBUFFER_MSG, *PCONSOLE_SCROLLSCREENBUFFER_MSG;

typedef struct _CONSOLE_SETTEXTATTRIBUTE_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN WORD   Attributes;
} CONSOLE_SETTEXTATTRIBUTE_MSG, *PCONSOLE_SETTEXTATTRIBUTE_MSG;

typedef struct _CONSOLE_SETFONT_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN DWORD  FontIndex;
} CONSOLE_SETFONT_MSG, *PCONSOLE_SETFONT_MSG;

typedef struct _CONSOLE_SETICON_MSG {
    IN HANDLE ConsoleHandle;
    IN HICON hIcon;
} CONSOLE_SETICON_MSG, *PCONSOLE_SETICON_MSG;

typedef struct _CONSOLE_READCONSOLE_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE InputHandle;
    IN USHORT ExeNameLength;
    IN OUT WCHAR   Buffer[BUFFER_SIZE/2];
    OUT PVOID  BufPtr;
    IN OUT ULONG  NumBytes;    // this value is valid even for error cases
    IN ULONG CaptureBufferSize;
    IN ULONG InitialNumBytes;
    IN ULONG CtrlWakeupMask;
    OUT ULONG ControlKeyState;
    IN BOOLEAN Unicode;
} CONSOLE_READCONSOLE_MSG, *PCONSOLE_READCONSOLE_MSG;

typedef struct _CONSOLE_WRITECONSOLE_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN WCHAR   Buffer[BUFFER_SIZE/2];
    IN PVOID  BufPtr;
    IN OUT ULONG  NumBytes;    // this value is valid even for error cases
    PWCHAR TransBuffer;         // used by server side only
    IN BOOLEAN BufferInMessage;
    IN BOOLEAN Unicode;
    BOOLEAN StackBuffer;        // used by server side only
    DWORD WriteFlags;           // used by server side only
} CONSOLE_WRITECONSOLE_MSG, *PCONSOLE_WRITECONSOLE_MSG;

typedef struct _CONSOLE_CLOSEHANDLE_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE Handle;
} CONSOLE_CLOSEHANDLE_MSG, *PCONSOLE_CLOSEHANDLE_MSG;

typedef struct _CONSOLE_DUPHANDLE_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE SourceHandle;
    IN DWORD  DesiredAccess;
    IN BOOLEAN InheritHandle;
    IN DWORD Options;
    OUT HANDLE TargetHandle;
} CONSOLE_DUPHANDLE_MSG, *PCONSOLE_DUPHANDLE_MSG;

typedef struct _CONSOLE_GETHANDLEINFORMATION_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE Handle;
    OUT DWORD Flags;
} CONSOLE_GETHANDLEINFORMATION_MSG, *PCONSOLE_GETHANDLEINFORMATION_MSG;

typedef struct _CONSOLE_SETHANDLEINFORMATION_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE Handle;
    IN DWORD Mask;
    IN DWORD Flags;
} CONSOLE_SETHANDLEINFORMATION_MSG, *PCONSOLE_SETHANDLEINFORMATION_MSG;

typedef struct _CONSOLE_ADDALIAS_MSG {
    HANDLE ConsoleHandle;
    USHORT SourceLength;
    USHORT TargetLength;
    USHORT ExeLength;
    PVOID Source;
    PVOID Target;
    PVOID Exe;
    BOOLEAN Unicode;
    BOOLEAN UnicodeExe;
} CONSOLE_ADDALIAS_MSG, *PCONSOLE_ADDALIAS_MSG;

typedef struct _CONSOLE_GETALIAS_MSG {
    HANDLE ConsoleHandle;
    USHORT SourceLength;
    USHORT TargetLength;
    USHORT ExeLength;
    PVOID Source;
    PVOID Target;
    PVOID Exe;
    BOOLEAN Unicode;
    BOOLEAN UnicodeExe;
} CONSOLE_GETALIAS_MSG, *PCONSOLE_GETALIAS_MSG;

typedef struct _CONSOLE_GETALIASESLENGTH_MSG {
    HANDLE ConsoleHandle;
    USHORT ExeLength;
    PVOID Exe;
    DWORD AliasesLength;
    BOOLEAN Unicode;
    BOOLEAN UnicodeExe;
} CONSOLE_GETALIASESLENGTH_MSG, *PCONSOLE_GETALIASESLENGTH_MSG;

typedef struct _CONSOLE_GETALIASEXESLENGTH_MSG {
    HANDLE ConsoleHandle;
    DWORD AliasExesLength;
    BOOLEAN Unicode;
} CONSOLE_GETALIASEXESLENGTH_MSG, *PCONSOLE_GETALIASEXESLENGTH_MSG;

typedef struct _CONSOLE_GETALIASES_MSG {
    HANDLE ConsoleHandle;
    USHORT ExeLength;
    PVOID Exe;
    BOOLEAN Unicode;
    BOOLEAN UnicodeExe;
    DWORD AliasesBufferLength;
    PVOID AliasesBuffer;
} CONSOLE_GETALIASES_MSG, *PCONSOLE_GETALIASES_MSG;

typedef struct _CONSOLE_GETALIASEXES_MSG {
    HANDLE ConsoleHandle;
    DWORD AliasExesBufferLength;
    PVOID AliasExesBuffer;
    BOOLEAN Unicode;
} CONSOLE_GETALIASEXES_MSG, *PCONSOLE_GETALIASEXES_MSG;

typedef struct _CONSOLE_EXPUNGECOMMANDHISTORY_MSG {
    HANDLE ConsoleHandle;
    USHORT ExeLength;
    PVOID Exe;
    BOOLEAN Unicode;
    BOOLEAN UnicodeExe;
} CONSOLE_EXPUNGECOMMANDHISTORY_MSG, *PCONSOLE_EXPUNGECOMMANDHISTORY_MSG;

typedef struct _CONSOLE_SETNUMBEROFCOMMANDS_MSG {
    HANDLE ConsoleHandle;
    DWORD NumCommands;
    USHORT ExeLength;
    PVOID Exe;
    BOOLEAN Unicode;
    BOOLEAN UnicodeExe;
} CONSOLE_SETNUMBEROFCOMMANDS_MSG, *PCONSOLE_SETNUMBEROFCOMMANDS_MSG;

typedef struct _CONSOLE_GETCOMMANDHISTORYLENGTH_MSG {
    HANDLE ConsoleHandle;
    DWORD CommandHistoryLength;
    USHORT ExeLength;
    PVOID Exe;
    BOOLEAN Unicode;
    BOOLEAN UnicodeExe;
} CONSOLE_GETCOMMANDHISTORYLENGTH_MSG, *PCONSOLE_GETCOMMANDHISTORYLENGTH_MSG;

typedef struct _CONSOLE_GETCOMMANDHISTORY_MSG {
    HANDLE ConsoleHandle;
    DWORD CommandBufferLength;
    PVOID CommandBuffer;
    USHORT ExeLength;
    PVOID Exe;
    BOOLEAN Unicode;
    BOOLEAN UnicodeExe;
} CONSOLE_GETCOMMANDHISTORY_MSG, *PCONSOLE_GETCOMMANDHISTORY_MSG;

typedef struct _CONSOLE_SETCOMMANDHISTORYMODE_MSG {
    HANDLE ConsoleHandle;
    DWORD Flags;
} CONSOLE_SETCOMMANDHISTORYMODE_MSG, *PCONSOLE_SETCOMMANDHISTORYMODE_MSG;

typedef struct _CONSOLE_VERIFYIOHANDLE_MSG {
    BOOL Valid;
    HANDLE ConsoleHandle;
    HANDLE Handle;
} CONSOLE_VERIFYIOHANDLE_MSG, *PCONSOLE_VERIFYIOHANDLE_MSG;

typedef struct _CONSOLE_ALLOC_MSG {
    IN PCONSOLE_INFO ConsoleInfo;
    IN DWORD TitleLength;
    IN LPWSTR Title;
    IN DWORD DesktopLength;
    IN LPWSTR Desktop;
    IN DWORD AppNameLength;
    IN LPWSTR AppName;
    IN DWORD CurDirLength;
    IN LPWSTR CurDir;
    IN LPTHREAD_START_ROUTINE CtrlRoutine;
    IN LPTHREAD_START_ROUTINE PropRoutine;
} CONSOLE_ALLOC_MSG, *PCONSOLE_ALLOC_MSG;

typedef struct _CONSOLE_FREE_MSG {
    IN HANDLE ConsoleHandle;
} CONSOLE_FREE_MSG, *PCONSOLE_FREE_MSG;

typedef struct _CONSOLE_GETTITLE_MSG {
    IN HANDLE ConsoleHandle;
    IN OUT DWORD TitleLength;
    OUT PVOID Title;
    BOOLEAN Unicode;
} CONSOLE_GETTITLE_MSG, *PCONSOLE_GETTITLE_MSG;

typedef struct _CONSOLE_SETTITLE_MSG {
    IN HANDLE ConsoleHandle;
    IN DWORD TitleLength;
    IN PVOID Title;
    BOOLEAN Unicode;
} CONSOLE_SETTITLE_MSG, *PCONSOLE_SETTITLE_MSG;

typedef struct _CONSOLE_INVALIDATERECT_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    SMALL_RECT Rect;
} CONSOLE_INVALIDATERECT_MSG, *PCONSOLE_INVALIDATERECT_MSG;

typedef struct _CONSOLE_VDM_MSG {
    IN HANDLE ConsoleHandle;
    IN DWORD  iFunction;
    OUT BOOL Bool;
    IN OUT POINT Point;
    OUT RECT Rect;
#if defined(FE_SB) && defined(i386) // for Kernel32 Single Binary
    IN OUT VDM_IOCTL_PARAM VDMIoctlParam;
#endif // FE_SB
} CONSOLE_VDM_MSG, *PCONSOLE_VDM_MSG;

typedef struct _CONSOLE_SETCURSOR_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN HCURSOR CursorHandle;
} CONSOLE_SETCURSOR_MSG, *PCONSOLE_SETCURSOR_MSG;

typedef struct _CONSOLE_SHOWCURSOR_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN BOOL bShow;
    OUT int DisplayCount;
} CONSOLE_SHOWCURSOR_MSG, *PCONSOLE_SHOWCURSOR_MSG;

typedef struct _CONSOLE_MENUCONTROL_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN UINT CommandIdLow;
    IN UINT CommandIdHigh;
    OUT HMENU hMenu;
} CONSOLE_MENUCONTROL_MSG, *PCONSOLE_MENUCONTROL_MSG;

typedef struct _CONSOLE_SETPALETTE_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN HPALETTE hPalette;
    IN UINT dwUsage;
} CONSOLE_SETPALETTE_MSG, *PCONSOLE_SETPALETTE_MSG;

typedef struct _CONSOLE_SETDISPLAYMODE_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN DWORD dwFlags;
    OUT COORD ScreenBufferDimensions;
    IN HANDLE hEvent;
} CONSOLE_SETDISPLAYMODE_MSG, *PCONSOLE_SETDISPLAYMODE_MSG;

typedef struct _CONSOLE_REGISTERVDM_MSG {
    IN HANDLE ConsoleHandle;
    IN DWORD RegisterFlags;
    IN HANDLE StartEvent;
    IN HANDLE EndEvent;
    IN LPWSTR StateSectionName;
    IN DWORD StateSectionNameLength;
    OUT ULONG StateLength;
    OUT PVOID StateBuffer;
    IN LPWSTR VDMBufferSectionName;
    IN DWORD VDMBufferSectionNameLength;
    IN COORD VDMBufferSize;
    OUT PVOID VDMBuffer;
} CONSOLE_REGISTERVDM_MSG, *PCONSOLE_REGISTERVDM_MSG;

typedef struct _CONSOLE_GETHARDWARESTATE_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    OUT COORD Resolution;
    OUT COORD FontSize;
} CONSOLE_GETHARDWARESTATE_MSG, *PCONSOLE_GETHARDWARESTATE_MSG;

typedef struct _CONSOLE_SETHARDWARESTATE_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE OutputHandle;
    IN COORD Resolution;
    IN COORD FontSize;
} CONSOLE_SETHARDWARESTATE_MSG, *PCONSOLE_SETHARDWARESTATE_MSG;

typedef struct _CONSOLE_GETDISPLAYMODE_MSG {
    IN HANDLE ConsoleHandle;
    OUT ULONG ModeFlags;
} CONSOLE_GETDISPLAYMODE_MSG, *PCONSOLE_GETDISPLAYMODE_MSG;

typedef struct _CONSOLE_GETCP_MSG {
    IN HANDLE ConsoleHandle;
    UINT wCodePageID;
    BOOL Output;
} CONSOLE_GETCP_MSG, *PCONSOLE_GETCP_MSG;

typedef struct _CONSOLE_SETCP_MSG {
    IN HANDLE ConsoleHandle;
    UINT wCodePageID;
    BOOL Output;
#if defined(FE_SB) // for Kernel32 Single Binary
    HANDLE hEvent;
#endif // FE_SB
} CONSOLE_SETCP_MSG, *PCONSOLE_SETCP_MSG;

typedef struct _CONSOLE_GETKEYBOARDLAYOUTNAME_MSG {
    IN HANDLE ConsoleHandle;
    union {
        WCHAR awchLayout[9];
        char achLayout[9];
    };
    BOOL bAnsi;
} CONSOLE_GETKEYBOARDLAYOUTNAME_MSG, *PCONSOLE_GETKEYBOARDLAYOUTNAME_MSG;

typedef struct _CONSOLE_SETKEYSHORTCUTS_MSG {
    IN HANDLE ConsoleHandle;
    BOOL Set;
    BYTE ReserveKeys;
    DWORD NumAppKeys;
    LPAPPKEY AppKeys;
} CONSOLE_SETKEYSHORTCUTS_MSG, *PCONSOLE_SETKEYSHORTCUTS_MSG;

typedef struct _CONSOLE_SETMENUCLOSE_MSG {
    IN HANDLE ConsoleHandle;
    BOOL Enable;
} CONSOLE_SETMENUCLOSE_MSG, *PCONSOLE_SETMENUCLOSE_MSG;

typedef struct _CONSOLE_NOTIFYLASTCLOSE_MSG {
    IN HANDLE ConsoleHandle;
} CONSOLE_NOTIFYLASTCLOSE_MSG, *PCONSOLE_NOTIFYLASTCLOSE_MSG;

typedef struct _CONSOLE_CTRLEVENT_MSG {
    IN HANDLE ConsoleHandle;
    IN DWORD CtrlEvent;
    IN DWORD ProcessGroupId;
} CONSOLE_CTRLEVENT_MSG, *PCONSOLE_CTRLEVENT_MSG;

#if defined(FE_SB) // for Kernel32 Single Binary
typedef struct _CONSOLE_CHAR_TYPE_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE Handle;
    IN COORD coordCheck;
    OUT DWORD dwType;
} CONSOLE_CHAR_TYPE_MSG, *PCONSOLE_CHAR_TYPE_MSG;

typedef struct _CONSOLE_LOCAL_EUDC_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE Handle;
    IN WORD CodePoint;
    IN COORD FontSize;
    IN PCHAR FontFace;
} CONSOLE_LOCAL_EUDC_MSG, *PCONSOLE_LOCAL_EUDC_MSG;

typedef struct _CONSOLE_CURSOR_MODE_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE Handle;
    IN BOOL Blink;
    IN BOOL DBEnable;
} CONSOLE_CURSOR_MODE_MSG, *PCONSOLE_CURSOR_MODE_MSG;

typedef struct _CONSOLE_REGISTEROS2_MSG {
    IN HANDLE ConsoleHandle;
    IN BOOL fOs2Register;
} CONSOLE_REGISTEROS2_MSG, *PCONSOLE_REGISTEROS2_MSG;

typedef struct _CONSOLE_SETOS2OEMFORMAT_MSG {
    IN HANDLE ConsoleHandle;
    IN BOOL fOs2OemFormat;
} CONSOLE_SETOS2OEMFORMAT_MSG, *PCONSOLE_SETOS2OEMFORMAT_MSG;

#if defined(FE_IME)
typedef struct _CONSOLE_NLS_MODE_MSG {
    IN HANDLE ConsoleHandle;
    IN HANDLE Handle;
    IN OUT BOOL Ready;
    IN DWORD NlsMode;
    IN HANDLE hEvent;
} CONSOLE_NLS_MODE_MSG, *PCONSOLE_NLS_MODE_MSG;

typedef struct _CONSOLE_REGISTER_CONSOLEIME_MSG {
    IN HANDLE ConsoleHandle;
    IN HWND hWndConsoleIME;
    IN DWORD dwConsoleIMEThreadId;
    IN DWORD DesktopLength;
    IN LPWSTR Desktop;
    OUT DWORD dwConsoleThreadId;
} CONSOLE_REGISTER_CONSOLEIME_MSG, *PCONSOLE_REGISTER_CONSOLEIME_MSG;

typedef struct _CONSOLE_UNREGISTER_CONSOLEIME_MSG {
    IN HANDLE ConsoleHandle;
    IN DWORD dwConsoleIMEThreadId;
} CONSOLE_UNREGISTER_CONSOLEIME_MSG, *PCONSOLE_UNREGISTER_CONSOLEIME_MSG;
#endif // FE_IME

#endif // FE_SB

typedef struct _CONSOLE_GETCONSOLEWINDOW_MSG {
    IN HANDLE ConsoleHandle;
    HANDLE hwnd;
} CONSOLE_GETCONSOLEWINDOW_MSG, *PCONSOLE_GETCONSOLEWINDOW_MSG;

typedef struct _CONSOLE_LANGID_MSG {
    IN HANDLE ConsoleHandle;
    OUT LANGID LangId;
} CONSOLE_LANGID_MSG, *PCONSOLE_LANGID_MSG;

typedef struct _CONSOLE_API_MSG {
    PORT_MESSAGE h;
    PCSR_CAPTURE_HEADER CaptureBuffer;
    CSR_API_NUMBER ApiNumber;
    ULONG ReturnValue;
    ULONG Reserved;
    union {
        CONSOLE_OPENCONSOLE_MSG OpenConsole;
        CONSOLE_GETCONSOLEINPUT_MSG GetConsoleInput;
        CONSOLE_WRITECONSOLEINPUT_MSG WriteConsoleInput;
        CONSOLE_READCONSOLEOUTPUT_MSG ReadConsoleOutput;
        CONSOLE_WRITECONSOLEOUTPUT_MSG WriteConsoleOutput;
        CONSOLE_READCONSOLEOUTPUTSTRING_MSG ReadConsoleOutputString;
        CONSOLE_WRITECONSOLEOUTPUTSTRING_MSG WriteConsoleOutputString;
        CONSOLE_FILLCONSOLEOUTPUT_MSG FillConsoleOutput;
        CONSOLE_MODE_MSG GetConsoleMode;
        CONSOLE_GETNUMBEROFFONTS_MSG GetNumberOfConsoleFonts;
        CONSOLE_GETNUMBEROFINPUTEVENTS_MSG GetNumberOfConsoleInputEvents;
        CONSOLE_GETSCREENBUFFERINFO_MSG GetConsoleScreenBufferInfo;
        CONSOLE_GETCURSORINFO_MSG GetConsoleCursorInfo;
        CONSOLE_GETMOUSEINFO_MSG GetConsoleMouseInfo;
        CONSOLE_GETFONTINFO_MSG GetConsoleFontInfo;
        CONSOLE_GETFONTSIZE_MSG GetConsoleFontSize;
        CONSOLE_GETCURRENTFONT_MSG GetCurrentConsoleFont;
        CONSOLE_MODE_MSG SetConsoleMode;
        CONSOLE_SETACTIVESCREENBUFFER_MSG SetConsoleActiveScreenBuffer;
        CONSOLE_FLUSHINPUTBUFFER_MSG FlushConsoleInputBuffer;
        CONSOLE_GETLARGESTWINDOWSIZE_MSG GetLargestConsoleWindowSize;
        CONSOLE_SETSCREENBUFFERSIZE_MSG SetConsoleScreenBufferSize;
        CONSOLE_SETCURSORPOSITION_MSG SetConsoleCursorPosition;
        CONSOLE_SETCURSORINFO_MSG SetConsoleCursorInfo;
        CONSOLE_SETWINDOWINFO_MSG SetConsoleWindowInfo;
        CONSOLE_SCROLLSCREENBUFFER_MSG ScrollConsoleScreenBuffer;
        CONSOLE_SETTEXTATTRIBUTE_MSG SetConsoleTextAttribute;
        CONSOLE_SETFONT_MSG SetConsoleFont;
        CONSOLE_SETICON_MSG SetConsoleIcon;
        CONSOLE_READCONSOLE_MSG ReadConsole;
        CONSOLE_WRITECONSOLE_MSG WriteConsole;
        CONSOLE_DUPHANDLE_MSG DuplicateHandle;
        CONSOLE_GETHANDLEINFORMATION_MSG GetHandleInformation;
        CONSOLE_SETHANDLEINFORMATION_MSG SetHandleInformation;
        CONSOLE_CLOSEHANDLE_MSG CloseHandle;
        CONSOLE_VERIFYIOHANDLE_MSG VerifyConsoleIoHandle;
        CONSOLE_ALLOC_MSG AllocConsole;
        CONSOLE_FREE_MSG FreeConsole;
        CONSOLE_GETTITLE_MSG GetConsoleTitle;
        CONSOLE_SETTITLE_MSG SetConsoleTitle;
        CONSOLE_CREATESCREENBUFFER_MSG CreateConsoleScreenBuffer;
        CONSOLE_INVALIDATERECT_MSG InvalidateConsoleBitmapRect;
        CONSOLE_VDM_MSG VDMConsoleOperation;
        CONSOLE_SETCURSOR_MSG SetConsoleCursor;
        CONSOLE_SHOWCURSOR_MSG ShowConsoleCursor;
        CONSOLE_MENUCONTROL_MSG ConsoleMenuControl;
        CONSOLE_SETPALETTE_MSG SetConsolePalette;
        CONSOLE_SETDISPLAYMODE_MSG SetConsoleDisplayMode;
        CONSOLE_REGISTERVDM_MSG RegisterConsoleVDM;
        CONSOLE_GETHARDWARESTATE_MSG GetConsoleHardwareState;
        CONSOLE_SETHARDWARESTATE_MSG SetConsoleHardwareState;
        CONSOLE_GETDISPLAYMODE_MSG GetConsoleDisplayMode;
        CONSOLE_ADDALIAS_MSG AddConsoleAliasW;
        CONSOLE_GETALIAS_MSG GetConsoleAliasW;
        CONSOLE_GETALIASESLENGTH_MSG GetConsoleAliasesLengthW;
        CONSOLE_GETALIASEXESLENGTH_MSG GetConsoleAliasExesLengthW;
        CONSOLE_GETALIASES_MSG GetConsoleAliasesW;
        CONSOLE_GETALIASEXES_MSG GetConsoleAliasExesW;
        CONSOLE_EXPUNGECOMMANDHISTORY_MSG ExpungeConsoleCommandHistoryW;
        CONSOLE_SETNUMBEROFCOMMANDS_MSG SetConsoleNumberOfCommandsW;
        CONSOLE_GETCOMMANDHISTORYLENGTH_MSG GetConsoleCommandHistoryLengthW;
        CONSOLE_GETCOMMANDHISTORY_MSG GetConsoleCommandHistoryW;
        CONSOLE_SETCOMMANDHISTORYMODE_MSG SetConsoleCommandHistoryMode;
        CONSOLE_GETCP_MSG GetConsoleCP;
        CONSOLE_SETCP_MSG SetConsoleCP;
        CONSOLE_SETKEYSHORTCUTS_MSG SetConsoleKeyShortcuts;
        CONSOLE_SETMENUCLOSE_MSG SetConsoleMenuClose;
        CONSOLE_NOTIFYLASTCLOSE_MSG SetLastConsoleEventActive;
        CONSOLE_CTRLEVENT_MSG GenerateConsoleCtrlEvent;
        CONSOLE_GETKEYBOARDLAYOUTNAME_MSG GetKeyboardLayoutName;
        CONSOLE_GETCONSOLEWINDOW_MSG GetConsoleWindow;
#if defined(FE_SB) // for Kernel32 Single Binary
        CONSOLE_CHAR_TYPE_MSG GetConsoleCharType;
        CONSOLE_LOCAL_EUDC_MSG SetConsoleLocalEUDC;
        CONSOLE_CURSOR_MODE_MSG SetConsoleCursorMode;
        CONSOLE_CURSOR_MODE_MSG GetConsoleCursorMode;
        CONSOLE_REGISTEROS2_MSG RegisterConsoleOS2;
        CONSOLE_SETOS2OEMFORMAT_MSG SetConsoleOS2OemFormat;
#if defined(FE_IME)
        CONSOLE_NLS_MODE_MSG GetConsoleNlsMode;
        CONSOLE_NLS_MODE_MSG SetConsoleNlsMode;
        CONSOLE_REGISTER_CONSOLEIME_MSG RegisterConsoleIME;
        CONSOLE_UNREGISTER_CONSOLEIME_MSG UnregisterConsoleIME;
#endif // FE_IME
#endif // FE_SB
        CONSOLE_LANGID_MSG GetConsoleLangId;
    } u;
} CONSOLE_API_MSG, *PCONSOLE_API_MSG;

#endif
