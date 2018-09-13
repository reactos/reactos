/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    consrv.h

Abstract:

    This module contains the include files and definitions for the
    console server DLL.

Author:

    Therese Stowell (thereses) 16-Nov-1990

Revision History:

--*/

#if DBG && defined(DEBUG_PRINT)
  #define _DBGFONTS   0x00000001
  #define _DBGFONTS2  0x00000002
  #define _DBGCHARS   0x00000004
  #define _DBGOUTPUT  0x00000008
  #define _DBGFULLSCR 0x00000008
  #define _DBGALL     0xFFFFFFFF
  extern ULONG gDebugFlag;

  #define DBGFONTS(_params_)   {if (gDebugFlag & _DBGFONTS)  DbgPrint _params_ ; }
  #define DBGFONTS2(_params_)  {if (gDebugFlag & _DBGFONTS2) DbgPrint _params_ ; }
  #define DBGCHARS(_params_)   {if (gDebugFlag & _DBGCHARS)  DbgPrint _params_ ; }
  #define DBGOUTPUT(_params_)  {if (gDebugFlag & _DBGOUTPUT) DbgPrint _params_ ; }
  #define DBGFULLSCR(_params_) {if (gDebugFlag & _DBGFULLSCR)DbgPrint _params_ ; }
  #define DBGPRINT(_params_)  DbgPrint _params_
#else
  #define DBGFONTS(_params_)
  #define DBGFONTS2(_params_)
  #define DBGCHARS(_params_)
  #define DBGOUTPUT(_params_)
  #define DBGFULLSCR(_params_)
  #define DBGPRINT(_params_)
#endif

#ifdef LATER
#if DBG
#undef  RIP_COMPONENT
#define RIP_COMPONENT RIP_CONSRV
#undef  ASSERT
#define ASSERT(exp) UserAssert(exp)
#endif
#endif

#define CONSOLE_MAX_FONT_NAME_LENGTH 256

#define DATA_CHUNK_SIZE 8192


#define MAKE_TAG( t ) (RTL_HEAP_MAKE_TAG( dwConBaseTag, t ))

#define TMP_TAG 0
#define BMP_TAG 1
#define ALIAS_TAG 2
#define HISTORY_TAG 3
#define TITLE_TAG 4
#define HANDLE_TAG 5
#define CONSOLE_TAG 6
#define ICON_TAG 7
#define BUFFER_TAG 8
#define WAIT_TAG 9
#define FONT_TAG 10
#define SCREEN_TAG 11
#if defined(FE_SB)
#define TMP_DBCS_TAG 12
#define SCREEN_DBCS_TAG 13
#define EUDC_TAG 14
#define CONVAREA_TAG 15
#define IME_TAG 16
#endif


#define GetWindowConsole(hWnd)          (PCONSOLE_INFORMATION)GetWindowLongPtr((hWnd), GWLP_USERDATA)

/*
 * Used to store some console attributes for the console.  This is a means
 * to cache the color in the extra-window-bytes, so USER/KERNEL can get
 * at it for hungapp drawing.  The window-offsets are defined in NTUSER\INC.
 *
 * The other macros are just convenient means for setting the other window
 * bytes.
 */
#define SetConsoleBkColor(hw,clr) SetWindowLong(hw, GWL_CONSOLE_BKCOLOR, clr)
#define SetConsolePid(hw,pid)     SetWindowLong(hw, GWL_CONSOLE_PID, pid)
#define SetConsoleTid(hw,tid)     SetWindowLong(hw, GWL_CONSOLE_TID, tid)


/*
 * helpful macros
 */
#define NELEM(array) (sizeof(array)/sizeof(array[0]))

// Text Information from PSCREEN_INFORMATION
__inline BYTE SCR_FAMILY(PSCREEN_INFORMATION pScreen) {
    return pScreen->BufferInfo.TextInfo.CurrentTextBufferFont.Family;
}

__inline DWORD SCR_FONTNUMBER(PSCREEN_INFORMATION pScreen) {
    return pScreen->BufferInfo.TextInfo.CurrentTextBufferFont.FontNumber;
}

__inline LPWSTR SCR_FACENAME(PSCREEN_INFORMATION pScreen) {
    return pScreen->BufferInfo.TextInfo.CurrentTextBufferFont.FaceName;
}

__inline COORD SCR_FONTSIZE(PSCREEN_INFORMATION pScreen) {
    return pScreen->BufferInfo.TextInfo.CurrentTextBufferFont.FontSize;
}

__inline LONG SCR_FONTWEIGHT(PSCREEN_INFORMATION pScreen) {
    return pScreen->BufferInfo.TextInfo.CurrentTextBufferFont.Weight;
}

__inline UINT SCR_FONTCODEPAGE(PSCREEN_INFORMATION pScreen) {
    return pScreen->BufferInfo.TextInfo.CurrentTextBufferFont.FontCodePage;
}


// Text Information from PCONSOLE_INFORMATION
#define CON_FAMILY(pCon)       SCR_FAMILY((pCon)->CurrentScreenBuffer)
#define CON_FONTNUMBER(pCon)   SCR_FONTNUMBER((pCon)->CurrentScreenBuffer)
#define CON_FACENAME(pCon)     SCR_FACENAME((pCon)->CurrentScreenBuffer)
#define CON_FONTSIZE(pCon)     SCR_FONTSIZE((pCon)->CurrentScreenBuffer)
#define CON_FONTWEIGHT(pCon)   SCR_FONTWEIGHT((pCon)->CurrentScreenBuffer)
#define CON_FONTCODEPAGE(pCon) SCR_FONTCODEPAGE((pCon)->CurrentScreenBuffer)

#if defined(FE_SB)

extern BOOLEAN    gfIsDBCSACP;      // TRUE if System ACP is associated with DBCS font

#define CONSOLE_IS_DBCS_ENABLED()   (gfIsDBCSACP)
#define CONSOLE_IS_IME_ENABLED()    (gfIsDBCSACP)

#define CONSOLE_IS_DBCS_OUTPUTCP(Console)   ((Console)->fIsDBCSOutputCP)
#define CONSOLE_IS_DBCS_CP(Console)         ((Console)->fIsDBCSCP)

#else   // FE_SB

#define CONSOLE_IS_DBCS_ENABLED()   (FALSE)
#define CONSOLE_IS_IME_ENABLED()    (FALSE)

#endif  // FE_SB

#ifdef UNICODE
#define LoadStringEx    LoadStringExW
#else
#define LoadStringEx    LoadStringExA
#endif

//
//  Cache the heap pointer for use by memory routines.
//

extern PWIN32HEAP pConHeap;
extern DWORD      dwConBaseTag;

//
// Make sure the console heap is still valid.
//

#define ValidateConsoleHeap()               \
    if (NtCurrentPeb()->BeingDebugged) {    \
        RtlValidateHeap(Win32HeapGetHandle(pConHeap), 0, NULL); \
    }

//
// Wrappers for console heap code.
//

#define ConsoleHeapAlloc(Flags, Size)                   \
    Win32HeapAlloc(pConHeap, Size, Flags, Flags)

#define ConsoleHeapReAlloc(Flags, Address, Size)        \
    Win32HeapReAlloc(pConHeap, Address, Size, Flags)

#if DBG
#define ConsoleHeapFree(Address)                        \
{                                                       \
    Win32HeapFree(pConHeap, Address);                   \
    Address = (PVOID)0xBAADF00D;                        \
}
#else
#define ConsoleHeapFree(Address)                        \
{                                                       \
    Win32HeapFree(pConHeap, Address);                   \
}
#endif

#define ConsoleHeapSize(Address)                        \
    Win32HeapSize(pConHeap, Address)

//
//  handle.c
//

NTSTATUS
ConsoleAddProcessRoutine(
    IN PCSR_PROCESS ParentProcess,
    IN PCSR_PROCESS Process
    );

NTSTATUS
DereferenceConsoleHandle(
    IN HANDLE ConsoleHandle,
    OUT PCONSOLE_INFORMATION *Console
    );

NTSTATUS
AllocateConsoleHandle(
    OUT PHANDLE Handle
    );

NTSTATUS
FreeConsoleHandle(
    IN HANDLE Handle
    );

NTSTATUS
ValidateConsole(
    IN PCONSOLE_INFORMATION Console
    );

NTSTATUS
ApiPreamble(
    IN HANDLE ConsoleHandle,
    OUT PCONSOLE_INFORMATION *Console
    );

NTSTATUS
RevalidateConsole(
    IN HANDLE ConsoleHandle,
    OUT PCONSOLE_INFORMATION *Console
    );

NTSTATUS
InitializeConsoleHandleTable( VOID );

#ifdef DEBUG

VOID LockConsoleHandleTable(VOID);
VOID UnlockConsoleHandleTable(VOID);
VOID LockConsole(
    IN PCONSOLE_INFORMATION Console
    );

#else

#define LockConsoleHandleTable()   RtlEnterCriticalSection(&ConsoleHandleLock)
#define UnlockConsoleHandleTable() RtlLeaveCriticalSection(&ConsoleHandleLock)
#define LockConsole(Con)           RtlEnterCriticalSection(&(Con)->ConsoleLock)

#endif

#define ConvertAttrToRGB(Con, Attr) ((Con)->ColorTable[(Attr) & 0x0F])


BOOLEAN
UnProtectHandle(
    HANDLE hObject
    );

NTSTATUS
AllocateConsole(
    IN HANDLE ConsoleHandle,
    IN LPWSTR Title,
    IN USHORT TitleLength,
    IN HANDLE ClientProcessHandle,
    OUT PHANDLE StdIn,
    OUT PHANDLE StdOut,
    OUT PHANDLE StdErr,
    OUT PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN OUT PCONSOLE_INFO ConsoleInfo,
    IN BOOLEAN WindowVisible,
    IN DWORD ConsoleThreadId
    );

VOID
DestroyConsole(
    IN PCONSOLE_INFORMATION Console
    );

VOID
FreeCon(
    IN PCONSOLE_INFORMATION Console
    );

VOID
InsertScreenBuffer(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo
    );

VOID
RemoveScreenBuffer(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo
    );

VOID
FreeProcessData(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData
    );

NTSTATUS
AllocateIoHandle(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN ULONG HandleType,
    OUT PHANDLE Handle
    );

NTSTATUS
GrowIoHandleTable(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData
    );

NTSTATUS
FreeIoHandle(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN HANDLE Handle
    );

NTSTATUS
DereferenceIoHandleNoCheck(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN HANDLE Handle,
    OUT PHANDLE_DATA *HandleData
    );

NTSTATUS
DereferenceIoHandle(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN HANDLE Handle,
    IN ULONG HandleType,
    IN ACCESS_MASK Access,
    OUT PHANDLE_DATA *HandleData
    );

BOOLEAN
InitializeInputHandle(
    PHANDLE_DATA HandleData,
    PINPUT_INFORMATION InputBuffer
    );

VOID
InitializeOutputHandle(
    PHANDLE_DATA HandleData,
    PSCREEN_INFORMATION ScreenBuffer
    );

ULONG
SrvVerifyConsoleIoHandle(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

//
// share.c
//

NTSTATUS
ConsoleAddShare(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PCONSOLE_SHARE_ACCESS ShareAccess,
    IN OUT PHANDLE_DATA HandleData
    );


NTSTATUS
ConsoleDupShare(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PCONSOLE_SHARE_ACCESS ShareAccess,
    IN OUT PHANDLE_DATA TargetHandleData
    );


NTSTATUS
ConsoleRemoveShare(
    IN ULONG DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PCONSOLE_SHARE_ACCESS ShareAccess
    );

//
// find.c
//

VOID
DoFind(
   IN PCONSOLE_INFORMATION Console
   );

//
// output.c
//

VOID
ScrollScreen(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT ScrollRect,
    IN PSMALL_RECT MergeRect,
    IN COORD TargetPoint
    );

VOID
SetProcessForegroundRights(
    IN PCSR_PROCESS Process,
    IN BOOL Foreground
    );

VOID
SetProcessFocus(
    IN PCSR_PROCESS Process,
    IN BOOL Foreground
    );

VOID
ModifyConsoleProcessFocus(
    IN PCONSOLE_INFORMATION Console,
    IN BOOL Foreground
    );

VOID
InitializeSystemMetrics( VOID );

VOID
InitializeScreenInfo( VOID );

NTSTATUS
ReadScreenBuffer(
    IN PSCREEN_INFORMATION ScreenInformation,
    OUT PCHAR_INFO Buffer,
    IN OUT PSMALL_RECT ReadRegion
    );

NTSTATUS
WriteScreenBuffer(
    IN PSCREEN_INFORMATION ScreenInformation,
    OUT PCHAR_INFO Buffer,
    IN OUT PSMALL_RECT WriteRegion
    );

NTSTATUS
DoCreateScreenBuffer(
    IN PCONSOLE_INFORMATION Console,
    IN PCONSOLE_INFO ConsoleInfo
    );

NTSTATUS
CreateScreenBuffer(
    OUT PSCREEN_INFORMATION *ScreenInformation,
    IN COORD dwWindowSize OPTIONAL,
    IN DWORD nFont OPTIONAL,
    IN COORD dwScreenBufferSize OPTIONAL,
    IN CHAR_INFO Fill,
    IN CHAR_INFO PopupFill,
    IN PCONSOLE_INFORMATION Console,
    IN DWORD Flags,
    IN PCONSOLE_GRAPHICS_BUFFER_INFO GraphicsBufferInfo OPTIONAL,
    OUT PVOID *lpBitmap,
    OUT HANDLE *hMutex,
    IN UINT CursorSize,
    IN LPWSTR FaceName
    );

VOID
AbortCreateConsole(
    IN PCONSOLE_INFORMATION Console
    );

NTSTATUS
CreateWindowsWindow(
    IN PCONSOLE_INFORMATION Console
    );

VOID
DestroyWindowsWindow(
    IN PCONSOLE_INFORMATION Console
    );

NTSTATUS
FreeScreenBuffer(
    IN PSCREEN_INFORMATION ScreenInformation
    );

NTSTATUS
ReadOutputString(
    IN PSCREEN_INFORMATION ScreenInfo,
    OUT PVOID Buffer,
    IN COORD ReadCoord,
    IN ULONG StringType,
    IN OUT PULONG NumRecords // this value is valid even for error cases
    );

NTSTATUS
InitializeScrollBuffer( VOID );

NTSTATUS
GetScreenBufferInformation(
    IN PSCREEN_INFORMATION ScreenInfo,
    OUT PCOORD Size,
    OUT PCOORD CursorPosition,
    OUT PCOORD ScrollPosition,
    OUT PWORD  Attributes,
    OUT PCOORD CurrentWindowSize,
    OUT PCOORD MaximumWindowSize
    );

VOID
GetWindowLimits(
    IN PSCREEN_INFORMATION ScreenInfo,
    OUT PWINDOW_LIMITS WindowLimits
    );

NTSTATUS
ResizeWindow(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT WindowDimensions,
    IN BOOL DoScrollBarUpdate
    );

NTSTATUS
ResizeScreenBuffer(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN COORD NewScreenSize,
    IN BOOL DoScrollBarUpdate
    );

NTSTATUS
ScrollRegion(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT ScrollRectangle,
    IN PSMALL_RECT ClipRectangle OPTIONAL,
    IN COORD  DestinationOrigin,
    IN CHAR_INFO Fill
    );

NTSTATUS
SetWindowOrigin(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN BOOLEAN Absolute,
    IN OUT COORD  WindowOrigin
    );

VOID
SetWindowSize(
    IN PSCREEN_INFORMATION ScreenInfo
    );

NTSTATUS
SetActiveScreenBuffer(
    IN PSCREEN_INFORMATION ScreenInfo
    );

NTSTATUS
QueueConsoleMessage(
    PCONSOLE_INFORMATION Console,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT APIENTRY
ConsoleWindowProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

VOID
VerticalScroll(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo,
    IN WORD ScrollCommand,
    IN WORD AbsoluteChange
    );

VOID
HorizontalScroll(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN WORD ScrollCommand,
    IN WORD AbsoluteChange
    );

VOID
StreamScrollRegion(
    IN PSCREEN_INFORMATION ScreenInfo
    );

#if defined(FE_SB)
VOID
FindAttrIndex(
    IN PATTR_PAIR String,
    IN SHORT Index,
    OUT PATTR_PAIR *IndexedAttr,
    OUT PSHORT CountOfAttr
    );

VOID
UpdateComplexRegion(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN COORD FontSize
    );

SHORT
ScrollEntireScreen(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN SHORT ScrollValue,
    IN BOOL UpdateRowIndex
    );

VOID
UpdateScrollBars(
    IN PSCREEN_INFORMATION ScreenInfo
    );
#endif

VOID
ReadRectFromScreenBuffer(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN COORD SourcePoint,
    IN PCHAR_INFO Target,
    IN COORD TargetSize,
    IN PSMALL_RECT TargetRect
    );


VOID
InitializeThreadMessages(VOID);

NTSTATUS
QueueThreadMessage(
    DWORD dwThreadId,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
UnqueueThreadMessage(
    DWORD dwThreadId,
    UINT* pMessage,
    WPARAM* pwParam,
    LPARAM* plParam
    );


//
// Drag/Drop on console windows (output.c)
//

UINT ConsoleDragQueryFile(
    IN HANDLE hDrop,
    IN PVOID lpFile,
    IN UINT cb
    );


VOID
DoDrop (
    IN WPARAM wParam,
    IN PCONSOLE_INFORMATION Console
    );


//
// input.c
//

NTSTATUS
ReadBuffer(
    IN PINPUT_INFORMATION InputInformation,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG EventsRead,
    IN BOOL Peek,
    IN BOOL StreamRead,
    OUT PBOOL ResetWaitEvent,
    IN BOOLEAN Unicode
    );

VOID
ConsoleInputThread(
    IN PINPUT_THREAD_INIT_INFO pInputThreadInitInfo
    );

VOID
StoreKeyInfo(
    IN PMSG msg
    );

VOID
RetrieveKeyInfo(
    IN HWND hWnd,
    OUT PWORD pwVirtualKeyCode,
    OUT PWORD pwVirtualScanCode,
    IN BOOL FreeKeyInfo
    );

VOID
ClearKeyInfo(
    IN HWND hWnd
    );

NTSTATUS
ReadInputBuffer(
    IN PINPUT_INFORMATION InputInformation,
    OUT PINPUT_RECORD lpBuffer,
    IN OUT PDWORD nLength,
    IN BOOL Peek,
    IN BOOL WaitForData,
    IN BOOL StreamRead,
    IN PCONSOLE_INFORMATION Console,
    IN PHANDLE_DATA HandleData OPTIONAL,
    IN PCSR_API_MSG Message OPTIONAL,
    IN CSR_WAIT_ROUTINE WaitRoutine OPTIONAL,
    IN PVOID WaitParameter OPTIONAL,
    IN ULONG WaitParameterLength  OPTIONAL,
    IN BOOLEAN WaitBlockExists OPTIONAL
#if defined(FE_SB)
    ,
    IN BOOLEAN Unicode
#endif
    );

DWORD
WriteInputBuffer(
    PCONSOLE_INFORMATION Console,
    PINPUT_INFORMATION InputBufferInformation,
    PINPUT_RECORD lpBuffer,
    DWORD nLength
    );

DWORD
PrependInputBuffer(
    PCONSOLE_INFORMATION Console,
    PINPUT_INFORMATION InputBufferInformation,
    PINPUT_RECORD lpBuffer,
    DWORD nLength
    );

NTSTATUS
CreateInputBuffer(
    IN ULONG NumberOfEvents OPTIONAL,
    IN PINPUT_INFORMATION InputBufferInformation
#if defined(FE_SB)
    ,
    IN PCONSOLE_INFORMATION Console
#endif
    );

NTSTATUS
ReinitializeInputBuffer(
    OUT PINPUT_INFORMATION InputBufferInformation
    );

VOID
FreeInputBuffer(
    IN PINPUT_INFORMATION InputBufferInformation
    );

#if defined(FE_SB)
VOID
ProcessCreateConsoleWindow(
    IN LPMSG lpMsg
    );

NTSTATUS
WaitForMoreToRead(
    IN PINPUT_INFORMATION InputInformation,
    IN PCSR_API_MSG Message OPTIONAL,
    IN CSR_WAIT_ROUTINE WaitRoutine OPTIONAL,
    IN PVOID WaitParameter OPTIONAL,
    IN ULONG WaitParameterLength  OPTIONAL,
    IN BOOLEAN WaitBlockExists OPTIONAL
    );

ULONG
GetControlKeyState(
    LPARAM lParam
    );

VOID
TerminateRead(
    IN PCONSOLE_INFORMATION Console,
    IN PINPUT_INFORMATION InputInfo,
    IN DWORD Flag
    );
#endif

NTSTATUS
GetNumberOfReadyEvents(
    IN PINPUT_INFORMATION InputInformation,
    OUT PULONG NumberOfEvents
    );

NTSTATUS
FlushInputBuffer(
    IN PINPUT_INFORMATION InputInformation
    );

NTSTATUS
FlushAllButKeys(
    PINPUT_INFORMATION InputInformation
    );

NTSTATUS
SetInputBufferSize(
    IN PINPUT_INFORMATION InputInformation,
    IN ULONG Size
    );

BOOL
HandleSysKeyEvent(
    IN PCONSOLE_INFORMATION Console,
    IN HWND hWnd,
    IN UINT Message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

VOID
HandleKeyEvent(
    IN PCONSOLE_INFORMATION Console,
    IN HWND hWnd,
    IN UINT Message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
HandleMouseEvent(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo,
    IN UINT Message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

VOID
HandleMenuEvent(
    IN PCONSOLE_INFORMATION Console,
    IN DWORD wParam
    );

VOID
HandleFocusEvent(
    IN PCONSOLE_INFORMATION Console,
    IN BOOL bSetFocus
    );

VOID
HandleCtrlEvent(
    IN PCONSOLE_INFORMATION Console,
    IN DWORD EventType
    );

#define CONSOLE_SHUTDOWN_FAILED 0
#define CONSOLE_SHUTDOWN_SUCCEEDED 1
#define CONSOLE_SHUTDOWN_SYSTEM 2

int
CreateCtrlThread(
    IN PCONSOLE_PROCESS_TERMINATION_RECORD ProcessHandleList,
    IN ULONG ProcessHandleListLength,
    IN PWCHAR Title,
    IN DWORD EventType,
    IN BOOL fForce
    );

VOID
UnlockConsole(
    IN PCONSOLE_INFORMATION Console
    );

ULONG
ShutdownConsole(
    IN HANDLE ConsoleHandle,
    IN DWORD dwFlags
    );

//
// link.c
//

#define LINK_NOINFO      0
#define LINK_SIMPLEINFO  1
#define LINK_FULLINFO    2

DWORD
GetLinkProperties(
    LPWSTR pszLinkName,
    LPVOID lpvBuffer,
    UINT cb
   );

DWORD
GetTitleFromLinkName(
    IN  LPWSTR szLinkName,
    OUT LPWSTR szTitle
    );

//
// misc.c
//

VOID
InitializeFonts( VOID );

BOOL
InitializeCustomCP( VOID );

#define EF_NEW         0x0001 // a newly available face
#define EF_OLD         0x0002 // a previously available face
#define EF_ENUMERATED  0x0004 // all sizes have been enumerated
#define EF_OEMFONT     0x0008 // an OEM face
#define EF_TTFONT      0x0010 // a TT face
#define EF_DEFFACE     0x0020 // the default face

NTSTATUS
EnumerateFonts( DWORD Flags );

VOID
InitializeMouseButtons( VOID );

NTSTATUS
GetMouseButtons(
    PULONG NumButtons
    );

NTSTATUS
FindTextBufferFontInfo(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN UINT CodePage,
    OUT PTEXT_BUFFER_FONT_INFO TextFontInfo
    );

NTSTATUS
StoreTextBufferFontInfo(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN ULONG FontIndex,
    IN COORD FontSize,
    IN BYTE  FontFamily,
    IN LONG  FontWeight,
    IN LPWSTR FaceName,
    IN UINT CodePage
    );

NTSTATUS
RemoveTextBufferFontInfo(
    IN PSCREEN_INFORMATION ScreenInfo
    );

NTSTATUS
GetNumFonts(
    OUT PULONG NumberOfFonts
    );

NTSTATUS
GetAvailableFonts(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN BOOLEAN MaximumWindow,
    OUT PVOID Buffer,
    IN OUT PULONG NumFonts
    );

NTSTATUS
GetFontSize(
    IN DWORD  FontIndex,
    OUT PCOORD FontSize
    );

NTSTATUS
GetCurrentFont(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN BOOLEAN MaximumWindow,
    OUT PULONG FontIndex,
    OUT PCOORD FontSize
    );

NTSTATUS
SetFont(
    IN PSCREEN_INFORMATION ScreenInfo
    );

NTSTATUS
SetScreenBufferFont(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN ULONG FontIndex
#if defined(FE_SB)
    ,
    IN UINT CodePage
#endif
    );

int
ConvertToOem(
    IN UINT Codepage,
    IN LPWSTR Source,
    IN int SourceLength,
    OUT LPSTR Target,
    IN int TargetLength
    );

int
ConvertInputToUnicode(
    IN UINT Codepage,
    IN LPSTR Source,
    IN int SourceLength,
    OUT LPWSTR Target,
    IN int TargetLength
    );

#if defined(FE_SB)
WCHAR
SB_CharToWcharGlyph(
    IN UINT Codepage,
    IN char Ch
    );

WCHAR
SB_CharToWchar(
    IN UINT Codepage,
    IN char Ch
    );

#else
WCHAR
CharToWcharGlyph(
    IN UINT Codepage,
    IN char Ch
    );

WCHAR
CharToWchar(
    IN UINT Codepage,
    IN char Ch
    );
#endif

char
WcharToChar(
    IN UINT Codepage,
    IN WCHAR Wchar
    );

int
ConvertOutputToUnicode(
    IN UINT Codepage,
    IN LPSTR Source,
    IN int SourceLength,
    OUT LPWSTR Target,
    IN int TargetLength
    );

int
ConvertOutputToOem(
    IN UINT Codepage,
    IN LPWSTR Source,
    IN int SourceLength,    // in chars
    OUT LPSTR Target,
    IN int TargetLength     // in chars
    );

NTSTATUS
RealUnicodeToFalseUnicode(
    IN OUT LPWSTR Source,
    IN int SourceLength, // in chars
    IN UINT Codepage
    );

NTSTATUS
FalseUnicodeToRealUnicode(
    IN OUT LPWSTR Source,
    IN int SourceLength, // in chars
    IN UINT Codepage
    );

VOID
InitializeSubst( VOID );

VOID
ShutdownSubst( VOID );

ULONG
SrvConsoleSubst(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

typedef struct tagFACENODE {
     struct tagFACENODE *pNext;
     DWORD  dwFlag;
     WCHAR  awch[];
} FACENODE, *PFACENODE;

BOOL DoFontEnum(
    IN HDC hDC OPTIONAL,
    IN LPWSTR pwszFace OPTIONAL,
    IN SHORT TTPointSize);

#if defined(FE_SB)
VOID
SetConsoleCPInfo(
    IN PCONSOLE_INFORMATION Console,
    IN BOOL Output
    );

BOOL
CheckBisectStringW(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN DWORD CodePage,
    IN PWCHAR Buffer,
    IN DWORD NumWords,
    IN DWORD NumBytes
    );

BOOL
CheckBisectProcessW(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN DWORD CodePage,
    IN PWCHAR Buffer,
    IN DWORD NumWords,
    IN DWORD NumBytes,
    IN SHORT OriginalXPosition,
    IN BOOL Echo
    );
#endif // FE_SB

//
// directio.c
//


ULONG
SrvGetConsoleInput(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvWriteConsoleInput(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvReadConsoleOutput(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvWriteConsoleOutput(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvReadConsoleOutputString(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvWriteConsoleOutputString(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvFillConsoleOutput(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvCreateConsoleScreenBuffer(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

NTSTATUS
TranslateOutputToPaddingUnicode(
    IN PCONSOLE_INFORMATION Console,
    IN OUT PCHAR_INFO OutputBuffer,
    IN COORD Size,
    IN OUT PCHAR_INFO OutputBufferR
    );

//
// getset.c
//

ULONG
SrvGetConsoleMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetConsoleNumberOfFonts(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetConsoleNumberOfInputEvents(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetLargestConsoleWindowSize(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetConsoleScreenBufferInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetConsoleCursorInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetConsoleMouseInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetConsoleFontInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetConsoleFontSize(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetConsoleCurrentFont(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGenerateConsoleCtrlEvent(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleActiveScreenBuffer(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvFlushConsoleInputBuffer(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleScreenBufferSize(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleCursorPosition(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleCursorInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleWindowInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvScrollConsoleScreenBuffer(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleTextAttribute(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleFont(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleIcon(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

NTSTATUS
SetScreenColors(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN WORD Attributes,
    IN WORD PopupAttributes,
    IN BOOL UpdateWholeScreen
    );

ULONG
SrvSetConsoleCP(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetConsoleCP(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetConsoleKeyboardLayoutName(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetConsoleWindow(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );


//
// stream.c
//

NTSTATUS
CookedRead(
    IN PCOOKED_READ_DATA CookedReadData,
    IN PCSR_API_MSG WaitReplyMessage,
    IN PCSR_THREAD WaitingThread,
    IN BOOLEAN WaitRoutine
    );

NTSTATUS
ReadChars(
    IN PINPUT_INFORMATION InputInfo,
    IN PCONSOLE_INFORMATION Console,
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN PSCREEN_INFORMATION ScreenInfo,
    IN OUT PWCHAR lpBuffer,
    IN OUT PDWORD NumBytes,
    IN DWORD InitialNumBytes,
    IN DWORD CtrlWakeupMask,
    IN PHANDLE_DATA HandleData,
    IN PCOMMAND_HISTORY CommandHistory,
    IN PCSR_API_MSG Message OPTIONAL,
    IN HANDLE HandleIndex,
    IN USHORT ExeNameLength,
    IN PWCHAR ExeName,
    IN BOOLEAN Unicode
    );

ULONG
SrvOpenConsole(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvReadConsole(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvWriteConsole(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvDuplicateHandle(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetHandleInformation(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetHandleInformation(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

VOID
UnblockWriteConsole(
    IN PCONSOLE_INFORMATION Console,
    IN DWORD Reason);

NTSTATUS
CloseInputHandle(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN PCONSOLE_INFORMATION Console,
    IN PHANDLE_DATA HandleData,
    IN HANDLE Handle
    );

NTSTATUS
CloseOutputHandle(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN PCONSOLE_INFORMATION Console,
    IN PHANDLE_DATA HandleData,
    IN HANDLE Handle,
    IN BOOLEAN FreeHandle
    );

ULONG
SrvCloseHandle(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

VOID
MakeCursorVisible(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN COORD CursorPosition
    );

#if defined(FE_SB)
HANDLE
FindActiveScreenBufferHandle(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN PCONSOLE_INFORMATION Console
    );

NTSTATUS
WriteString(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PWCHAR String,
    IN ULONG NumChars,
    IN BOOL KeepCursorVisible,
    OUT PSHORT ScrollY OPTIONAL
    );
#endif

//
// cursor.c
//

NTSTATUS
SetCursorInformation(
    PSCREEN_INFORMATION ScreenInfo,
    ULONG Size,
    BOOLEAN Visible
    );

NTSTATUS
SetCursorPosition(
    IN OUT PSCREEN_INFORMATION ScreenInfo,
    IN COORD Position,
    IN BOOL  TurnOn
    );

NTSTATUS
SetCursorMode(
    PSCREEN_INFORMATION ScreenInfo,
    BOOLEAN DoubleCursor
    );

VOID
CursorTimerRoutine(
    IN PSCREEN_INFORMATION ScreenInfo
    );

#if defined(FE_SB)
VOID
SB_InvertPixels(
    IN PSCREEN_INFORMATION ScreenInfo
    );
#endif

VOID
ConsoleHideCursor(
    IN PSCREEN_INFORMATION ScreenInfo
    );

VOID
ConsoleShowCursor(
    IN PSCREEN_INFORMATION ScreenInfo
    );

#ifdef i386
NTSTATUS
SetCursorInformationHW(
    PSCREEN_INFORMATION ScreenInfo,
    ULONG Size,
    BOOLEAN Visible
    );

NTSTATUS
SetCursorPositionHW(
    IN OUT PSCREEN_INFORMATION ScreenInfo,
    IN COORD Position
    );
#endif

//
// cmdline.c
//

VOID
InitializeConsoleCommandData(
    IN PCONSOLE_INFORMATION Console
    );

ULONG
SrvAddConsoleAlias(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetConsoleAlias(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

DWORD
SrvGetConsoleAliasesLength(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

DWORD
SrvGetConsoleAliasExesLength(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

DWORD
SrvGetConsoleAliases(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

DWORD
SrvGetConsoleAliasExes(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

DWORD
SrvExpungeConsoleCommandHistory(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

DWORD
SrvSetConsoleNumberOfCommands(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

DWORD
SrvGetConsoleCommandHistoryLength(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

DWORD
SrvGetConsoleCommandHistory(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

DWORD
SrvSetConsoleCommandHistoryMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

NTSTATUS
MatchandCopyAlias(
    IN PCONSOLE_INFORMATION Console,
    IN PWCHAR Source,
    IN USHORT SourceLength,
    OUT PWCHAR TargetBuffer,
    IN OUT PUSHORT TargetLength,
    IN LPWSTR Exe,
    IN USHORT ExeLength,
    OUT PDWORD LineCount
    );

NTSTATUS
AddCommand(
    IN PCOMMAND_HISTORY CommandHistory,
    IN PWCHAR Command,
    IN USHORT Length,
    IN BOOL HistoryNoDup
    );

NTSTATUS
RetrieveCommand(
    IN PCOMMAND_HISTORY CommandHistory,
    IN WORD VirtualKeyCode,
    IN PWCHAR Buffer,
    IN ULONG BufferSize,
    OUT PULONG CommandSize
    );

PCOMMAND_HISTORY
AllocateCommandHistory(
    IN PCONSOLE_INFORMATION Console,
    IN DWORD AppNameLength,
    IN PWCHAR AppName,
    IN HANDLE ProcessHandle
    );

VOID
ResetCommandHistory(
    IN PCOMMAND_HISTORY CommandHistory
    );

ULONG
SrvGetConsoleTitle(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleTitle(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

VOID
FreeAliasBuffers(
    IN PCONSOLE_INFORMATION Console
    );

VOID
FreeCommandHistory(
    IN PCONSOLE_INFORMATION Console,
    IN HANDLE ProcessHandle
    );

VOID
FreeCommandHistoryBuffers(
    IN OUT PCONSOLE_INFORMATION Console
    );

VOID
ResizeCommandHistoryBuffers(
    IN PCONSOLE_INFORMATION Console,
    IN UINT NumCommands
    );

int
MyStringCompareW(
    IN LPWSTR Str1,
    IN LPWSTR Str2,
    IN USHORT Length,
    IN BOOLEAN bCaseInsensitive
    );

int
LoadStringExW(
    IN HINSTANCE hModule,
    IN UINT      wID,
    OUT LPWSTR   lpBuffer,
    IN int       cchBufferMax,
    IN WORD      wLangId
    );

//
// srvinit.c
//

ULONG
SrvAllocConsole(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvFreeConsole(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

NTSTATUS
RemoveConsole(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN HANDLE ProcessHandle,
    IN HANDLE ProcessId
    );

BOOL
MapHandle(
    IN HANDLE ClientProcessHandle,
    IN HANDLE ServerHandle,
    OUT PHANDLE ClientHandle
    );

VOID
InitializeConsoleAttributes( VOID );

VOID
GetRegistryValues(
    IN LPWSTR ConsoleTitle,
    OUT PCONSOLE_REGISTRY_INFO RegInfo
    );

#if defined(FE_SB)
NTSTATUS
MyRegOpenKey(
    IN HANDLE hKey,
    IN LPWSTR lpSubKey,
    OUT PHANDLE phResult
    );

NTSTATUS
MyRegQueryValue(
    IN HANDLE hKey,
    IN LPWSTR lpValueName,
    IN DWORD dwValueLength,
    OUT LPBYTE lpData
    );

NTSTATUS
MyRegQueryValueEx(
    IN HANDLE hKey,
    IN LPWSTR lpValueName,
    IN DWORD dwValueLength,
    OUT LPBYTE lpData,
    OUT LPDWORD lpDataLength
    );

NTSTATUS
MyRegEnumValue(
    IN HANDLE hKey,
    IN DWORD dwIndex,
    OUT DWORD dwValueLength,
    OUT LPWSTR lpValueName,
    OUT DWORD dwDataLength,
    OUT LPBYTE lpData
    );
#endif

LPWSTR
TranslateConsoleTitle(
    LPWSTR ConsoleTitle,
    PUSHORT pcbTranslatedLength,
    BOOL Unexpand,
    BOOL Substitute
    );

NTSTATUS
GetConsoleLangId(
    IN UINT OutputCP,
    OUT LANGID* pLangId
    );

ULONG
SrvGetConsoleLangId(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

//
// bitmap.c
//

NTSTATUS
CreateConsoleBitmap(
    IN OUT PCONSOLE_GRAPHICS_BUFFER_INFO GraphicsInfo,
    IN OUT PSCREEN_INFORMATION ScreenInfo,
    OUT PVOID *lpBitmap,
    OUT HANDLE *hMutex
    );

NTSTATUS
WriteRegionToScreenBitMap(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Region
    );

ULONG
SrvInvalidateBitMapRect(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvVDMConsoleOperation(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );


//
// private.c
//

VOID
UpdateMousePosition(
    PSCREEN_INFORMATION ScreenInfo,
    COORD Position
    );

ULONG
SrvSetConsoleCursor(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvShowConsoleCursor(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvConsoleMenuControl(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsolePalette(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleDisplayMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

VOID
SetActivePalette(
    IN PSCREEN_INFORMATION ScreenInfo
    );

VOID
UnsetActivePalette(
    IN PSCREEN_INFORMATION ScreenInfo
    );

ULONG
SrvRegisterConsoleVDM(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

NTSTATUS
SrvConsoleNotifyLastClose(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetConsoleHardwareState(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleHardwareState(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetConsoleDisplayMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleMenuClose(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleKeyShortcuts(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

#ifdef i386

VOID
ReadRegionFromScreenHW(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Region,
    IN PCHAR_INFO ReadBufPtr
    );

VOID
ScrollHW(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT ScrollRect,
    IN PSMALL_RECT MergeRect,
    IN COORD TargetPoint
    );

ULONG
MatchWindowSize(
#if defined(FE_SB)
    IN UINT CodePage,
#endif
    IN COORD WindowSize,
    OUT PCOORD pWindowSize
    );

BOOL
SetVideoMode(
    IN PSCREEN_INFORMATION ScreenInfo
    );

NTSTATUS
DisplayModeTransition(
    IN BOOL Foreground,
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo
    );

NTSTATUS
ConvertToWindowed(
    IN PCONSOLE_INFORMATION Console
    );

NTSTATUS
ConvertToFullScreen(
    IN PCONSOLE_INFORMATION Console
    );

NTSTATUS
SetROMFontCodePage(
    IN UINT wCodePage,
    IN ULONG ModeIndex
    );

#endif

BOOL
InitializeFullScreen( VOID );

NTSTATUS
ChangeDispSettings(
    PCONSOLE_INFORMATION Console,
    HWND hwnd,
    DWORD dwFlags
    );

#define SCREEN_BUFFER_POINTER(X,Y,XSIZE,CELLSIZE) (((XSIZE * (Y)) + (X)) * (ULONG)CELLSIZE)

//
// menu.c
//

VOID
InitSystemMenu(
    IN PCONSOLE_INFORMATION Console
    );

VOID
InitializeMenu(
    IN PCONSOLE_INFORMATION Console
    );

VOID
SetWinText(
    IN PCONSOLE_INFORMATION Console,
    IN UINT wID,
    IN BOOL Add
    );

VOID
PropertiesDlgShow(
    IN PCONSOLE_INFORMATION Console,
    IN BOOL fCurrent
    );

VOID
PropertiesUpdate(
    IN PCONSOLE_INFORMATION Console,
    IN HANDLE hClientSection
    );

//
// fontdlg.c
//

int
FindCreateFont(
    DWORD Family,
    LPWSTR pwszTTFace,
    COORD Size,
    LONG Weight,
    UINT CodePage);

//
// clipbrd.c
//

VOID
DoCopy(
    IN PCONSOLE_INFORMATION Console
    );

VOID
DoMark(
    IN PCONSOLE_INFORMATION Console
    );

VOID
DoSelectAll(
    IN PCONSOLE_INFORMATION Console
    );

VOID
DoStringPaste(
    IN PCONSOLE_INFORMATION Console,
    IN PWCHAR pwStr,
    IN UINT DataSize
    );

VOID
DoPaste(
    IN PCONSOLE_INFORMATION Console
    );

#if defined(FE_SB)
VOID
SB_ExtendSelection(
    IN PCONSOLE_INFORMATION Console,
    IN COORD CursorPosition
    );
#else
VOID
ExtendSelection(
    IN PCONSOLE_INFORMATION Console,
    IN COORD CursorPosition
    );
#endif

VOID
ClearSelection(
    IN PCONSOLE_INFORMATION Console
    );

VOID
StoreSelection(
    IN PCONSOLE_INFORMATION Console
    );

VOID
InvertSelection(
    IN PCONSOLE_INFORMATION Console,
    BOOL Inverting
    );

#if defined(FE_SB)
BOOL
SB_MyInvert(
    IN PCONSOLE_INFORMATION Console,
    IN PSMALL_RECT SmallRect
    );
#else
BOOL
MyInvert(
    IN PCONSOLE_INFORMATION Console,
    IN PSMALL_RECT SmallRect
    );
#endif

VOID
ConvertToMouseSelect(
    IN PCONSOLE_INFORMATION Console,
    IN COORD MousePosition
    );

VOID
DoScroll(
    IN PCONSOLE_INFORMATION Console
    );

VOID
ClearScroll(
    IN PCONSOLE_INFORMATION Console
    );


//
// External private functions used by consrv
//

BOOL
SetConsoleReserveKeys(
    HWND hWnd,
    DWORD fsReserveKeys
    );

int APIENTRY
GreGetDIBitsInternal(
    HDC hdc,
    HBITMAP hBitmap,
    UINT iStartScan,
    UINT cNumScan,
    LPBYTE pjBits,
    LPBITMAPINFO pBitsInfo,
    UINT iUsage,
    UINT cjMaxBits,
    UINT cjMaxInfo
    );


#if defined(FE_SB)
//
// constubs.c
//
ULONG
SrvGetConsoleCharType(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleLocalEUDC(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );


ULONG
SrvSetConsoleCursorMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvGetConsoleCursorMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvRegisterConsoleOS2(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleOS2OemFormat(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

#if defined(FE_IME)
ULONG
SrvGetConsoleNlsMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvSetConsoleNlsMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvRegisterConsoleIME(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
SrvUnregisterConsoleIME(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );
#endif // FE_IME

//
// dispatch.c
//

VOID
InvertPixels(
    IN PSCREEN_INFORMATION ScreenInfo
    );

WCHAR
CharToWchar(
    IN PCONSOLE_INFORMATION Console,
    IN UINT Codepage,
    IN char *Ch
    );

BOOL
MyInvert(
    IN PCONSOLE_INFORMATION Console,
    IN PSMALL_RECT SmallRect
    );

VOID
ExtendSelection(
    IN PCONSOLE_INFORMATION Console,
    IN COORD CursorPosition
    );

#endif // FE_SB

/*
 * The following define must match the define in w32\w32inc\w32p.h.
 * Later5.0 GerardoB: It should include w32p.h although that implies
 *  including several other headers.
 */
#define W32PF_ALLOWSETFOREGROUND           0x00080000
#define W32PF_CONSOLEHASFOCUS              0x04000000

