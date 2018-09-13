/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    dbcs.h

Abstract:

Author:

Revision History:

--*/


#define UNICODE_DBCS_PADDING 0xffff

#define DEFAULT_FONTSIZE   256
#define DEFAULT_EUDCSIZE   1

#define VDM_EUDC_FONT_SIZE_X 16
#define VDM_EUDC_FONT_SIZE_Y 16

#define USACP     1252
#define KOREAN_CP 949
#define JAPAN_CP  932

#if defined(FE_SB)
#define MIN_SCRBUF_WIDTH  12    // for suport display IME status
#define MIN_SCRBUF_HEIGHT  2    // for suport display IME status
#define MIN_WINDOW_HEIGHT  1
#endif



#if defined(i386)
extern ULONG  gdwMachineId;
#endif


extern ULONG DefaultFontIndex;
extern COORD DefaultFontSize;
extern BYTE  DefaultFontFamily;


typedef struct _MODE_FONT_PAIR {
    DWORD Mode;
        #define FS_MODE_TEXT     0x0001
        #define FS_MODE_GRAPHICS 0x0002
        #define FS_MODE_FIND     0x8000
        #define FS_TEXT          (FS_MODE_FIND+FS_MODE_TEXT)
        #define FS_GRAPHICS      (FS_MODE_FIND+FS_MODE_GRAPHICS)
    COORD ScreenSize;
    COORD Resolution;
    COORD FontSize;
} MODE_FONT_PAIR, *PMODE_FONT_PAIR;

typedef struct _FS_CODEPAGE {
    SINGLE_LIST_ENTRY List;
    UINT CodePage;
} FS_CODEPAGE, *PFS_CODEPAGE;

extern PUSHORT RegInitialPalette;
extern PUCHAR RegColorBuffer;
extern PUCHAR RegColorBufferNoTranslate;
extern DWORD NUMBER_OF_MODE_FONT_PAIRS;
extern PMODE_FONT_PAIR RegModeFontPairs;
extern SINGLE_LIST_ENTRY gRegFullScreenCodePage;    // This list contain FS_CODEPAGE data.



typedef struct tagSTRINGBITMAP
{
    UINT uiWidth;
    UINT uiHeight;
    BYTE ajBits[1];
} STRINGBITMAP, *LPSTRINGBITMAP;

UINT
GetStringBitmapW(
    HDC             hdc,
    LPWSTR          pwc,
    UINT            cwc,
    UINT            cbData,
    BYTE            *pSB
    );



//
// dbcs.c
//

#if defined(FE_IME)

#if defined(i386)
NTSTATUS
ImeWmFullScreen(
    IN BOOL Foreground,
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo
    );
#endif // i386


NTSTATUS
GetImeKeyState(
    IN PCONSOLE_INFORMATION Console,
    IN PDWORD pdwConversion
    );


NTSTATUS
SetImeKeyState(
    IN PCONSOLE_INFORMATION Console,
    IN DWORD fdwConversion
    );

NTSTATUS
SetImeCodePage(
    IN PCONSOLE_INFORMATION Console
    );

NTSTATUS
SetImeOutputCodePage(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo,
    IN DWORD CodePage
    );
#endif // FE_IME

VOID
SetLineChar(
    IN PSCREEN_INFORMATION ScreenInfo
    );

BOOL
CheckBisectStringA(
    IN DWORD CodePage,
    IN PCHAR Buffer,
    IN DWORD NumBytes,
    IN LPCPINFO lpCPInfo
    );

VOID
BisectWrite(
    IN SHORT StringLength,
    IN COORD TargetPoint,
    IN PSCREEN_INFORMATION ScreenInfo
    );

VOID
BisectClipbrd(
    IN SHORT StringLength,
    IN COORD TargetPoint,
    IN PSCREEN_INFORMATION ScreenInfo,
    OUT PSMALL_RECT SmallRect
    );

VOID
BisectWriteAttr(
    IN SHORT StringLength,
    IN COORD TargetPoint,
    IN PSCREEN_INFORMATION ScreenInfo
    );

DWORD
RemoveDbcsMark(
    IN PWCHAR Dst,
    IN PWCHAR Src,
    IN DWORD NumBytes,
    IN PCHAR SrcA,
    IN BOOL OS2OemFormat
    );

DWORD
RemoveDbcsMarkCell(
    IN PCHAR_INFO Dst,
    IN PCHAR_INFO Src,
    IN DWORD NumBytes
    );

DWORD
RemoveDbcsMarkAll(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PROW Row,
    IN PSHORT LeftChar,
    IN PRECT TextRect,
    IN int *TextLeft,
    IN PWCHAR Buffer,
    IN SHORT NumberOfChars
    );

BOOL
IsDBCSLeadByteConsole(
    IN BYTE AsciiChar,
    IN LPCPINFO lpCPInfo
    );

NTSTATUS
AdjustFont(
    IN PCONSOLE_INFORMATION Console,
    IN UINT CodePage
    );

NTSTATUS
ConvertToCodePage(
    IN PCONSOLE_INFORMATION Console,
    IN UINT PrevCodePage
    );

NTSTATUS
ConvertOutputOemToNonOemUnicode(
    IN OUT LPWSTR Source,
    IN OUT PBYTE KAttrRows,
    IN int SourceLength, // in chars
    IN UINT Codepage
    );


VOID
TextOutEverything(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo,
    IN SHORT LeftWindowPos,
    IN OUT PSHORT RightWindowPos,
    IN OUT PSHORT CountOfAttr,
    IN SHORT CountOfAttrOriginal,
    IN OUT PBOOL DoubleColorDBCS,
    IN BOOL LocalEUDCFlag,
    IN PROW Row,
    IN PATTR_PAIR Attr,
    IN SHORT LeftTextPos,
    IN SHORT RightTextPos,
    IN int WindowRectLeft,
    IN RECT WindowRect,
    IN SHORT NumberOfChars
    );

VOID
TextOutCommonLVB(
    IN PCONSOLE_INFORMATION Console,
    IN WORD Attributes,
    IN RECT CommonLVBRect
    );

NTSTATUS
MakeAltRasterFont(
    UINT CodePage,
    COORD DefaultFontSize,
    COORD *AltFontSize,
    BYTE  *AltFontFamily,
    ULONG *AltFontIndex,
    LPWSTR AltFaceName
    );

NTSTATUS
InitializeDbcsMisc(
    VOID
    );

#if defined(i386)
NTSTATUS
RealUnicodeToNEC_OS2_Unicode(
    IN OUT LPWSTR Source,
    IN int SourceLength      // in chars
    );

BOOL
InitializeNEC_OS2_CP(
    VOID
    );
#endif

BYTE
CodePageToCharSet(
    UINT CodePage
    );

BOOL
IsAvailableFarEastCodePage(
    UINT CodePage
    );

LPTTFONTLIST
SearchTTFont(
    LPWSTR pwszFace,
    BOOL   fCodePage,
    UINT   CodePage
    );

BOOL
IsAvailableTTFont(
    LPWSTR pwszFace
    );

BOOL
IsAvailableTTFontCP(
    LPWSTR pwszFace,
    UINT CodePage
    );

LPWSTR
GetAltFaceName(
    LPWSTR pwszFace
    );

BOOL
IsAvailableFsCodePage(
    UINT CodePage
    );

#if defined(FE_IME)

VOID
ProcessCreateConsoleIME(
    IN LPMSG lpMsg,
    DWORD dwConsoleThreadId
    );

NTSTATUS
InitConsoleIMEStuff(
    HDESK hDesktop,
    DWORD dwConsoleThreadId,
    PCONSOLE_INFORMATION Console
    );

NTSTATUS
WaitConsoleIMEStuff(
    HDESK hDesktop,
    HANDLE hThread
    );

NTSTATUS
ConSrvRegisterConsoleIME(
    PCSR_PROCESS Process,
    HDESK hDesktop,
    HWINSTA hWinSta,
    HWND  hWndConsoleIME,
    DWORD dwConsoleIMEThreadId,
    DWORD dwAction,
    DWORD *dwConsoleThreadId
    );

VOID
RemoveConsoleIME(
    PCSR_PROCESS Process,
    DWORD dwConsoleIMEThreadId
    );

NTSTATUS
ConsoleImeMessagePumpWorker(
    PCONSOLE_INFORMATION Console,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam,
    LRESULT* lplResult);


NTSTATUS
ConsoleImeMessagePump(
    PCONSOLE_INFORMATION Console,
    UINT   Message,
    WPARAM wParam,
    LPARAM lParam
    );
#endif // FE_IME


BOOL
RegisterKeisenOfTTFont(
    IN PSCREEN_INFORMATION ScreenInfo
    );

ULONG
TranslateUnicodeToOem(
    IN PCONSOLE_INFORMATION Console,
    IN PWCHAR UnicodeBuffer,
    IN ULONG UnicodeCharCount,
    OUT PCHAR AnsiBuffer,
    IN ULONG AnsiByteCount,
    OUT PINPUT_RECORD DbcsLeadInpRec
    );

DWORD
ImmConversionToConsole(
    DWORD fdwConversion
    );

DWORD
ImmConversionFromConsole(
    DWORD dwNlsMode
    );

//#define DBG_KAZUM
//#define DBG_KATTR

#if defined(DBG) && defined(DBG_KATTR)
VOID
BeginKAttrCheck(
    IN PSCREEN_INFORMATION ScreenInfo
    );
#endif


//
// output2.c
//
BOOL
CreateDbcsScreenBuffer(
    IN PCONSOLE_INFORMATION Console,
    IN COORD dwScreenBufferSize,
    OUT PDBCS_SCREEN_BUFFER DbcsScreenBuffer
    );

BOOL
DeleteDbcsScreenBuffer(
    IN PDBCS_SCREEN_BUFFER DbcsScreenBuffer
    );

BOOL
ReCreateDbcsScreenBuffer(
    IN PCONSOLE_INFORMATION Console,
    IN UINT OldCodePage
    );

BOOL
FE_PolyTextOutCandidate(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Region
    );

VOID
FE_ConsolePolyTextOut(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Region
    );


//
// private2.c
//
NTSTATUS
SetRAMFontCodePage(
    IN PSCREEN_INFORMATION ScreenInfo
    );

NTSTATUS
SetRAMFont(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PCHAR_INFO ScreenBufPtr,
    IN DWORD Length
    );
