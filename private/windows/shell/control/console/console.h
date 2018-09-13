/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    console.h

Abstract:

    This module contains the definitions for the console applet

Author:

    Jerry Shea (jerrysh) Feb-3-1992

Revision History:

--*/

#include "font.h"
#include "doshelp.h"


//
// Icon ID
//

#define IDI_CONSOLE                   1


//
// String table constants
//

#define IDS_NAME                      1
#define IDS_INFO                      2
#define IDS_TITLE                     3
#define IDS_RASTERFONT                4
#define IDS_FONTSIZE                  5
#define IDS_SELECTEDFONT              6
#define IDS_SAVE                      7
#define IDS_LINKERRCAP                8
#define IDS_LINKERROR                 9
#define IDS_WARNING                  10


//
// Global Variables
//

extern HINSTANCE  ghInstance;
extern PCONSOLE_STATE_INFO gpStateInfo;
extern PFONT_INFO FontInfo;
extern ULONG      NumberOfFonts;
extern ULONG      CurrentFontIndex;
extern ULONG      DefaultFontIndex;
extern TCHAR      DefaultFaceName[];
extern COORD      DefaultFontSize;
extern BYTE       DefaultFontFamily;
extern TCHAR      szPreviewText[];
extern PFACENODE  gpFaceNames;
extern BOOL       gbEnumerateFaces;
extern LONG       gcxScreen;
extern LONG       gcyScreen;

#if defined(FE_SB)
extern UINT OEMCP;
extern BOOL gfFESystem;
extern BOOL fChangeCodePage;


NTSTATUS
MakeAltRasterFont(
    UINT CodePage,
    COORD *AltFontSize,
    BYTE  *AltFontFamily,
    ULONG *AltFontIndex,
    LPTSTR AltFaceName
    );

NTSTATUS
InitializeDbcsMisc(
    VOID
    );

BYTE
CodePageToCharSet(
    UINT CodePage
    );

LPTTFONTLIST
SearchTTFont(
    LPTSTR ptszFace,
    BOOL   fCodePage,
    UINT   CodePage
    );

BOOL
IsAvailableTTFont(
    LPTSTR ptszFace
    );

BOOL
IsAvailableTTFontCP(
    LPWSTR pwszFace,
    UINT CodePage
    );

BOOL
IsDisableBoldTTFont(
    LPTSTR ptszFace
    );

LPTSTR
GetAltFaceName(
    LPTSTR ptszFace
    );

NTSTATUS
DestroyDbcsMisc(
    VOID
    );

int
LanguageListCreate(
    HWND hDlg,
    UINT CodePage
    );

int
LanguageDisplay(
    HWND hDlg,
    UINT CodePage
    ) ;

//
// registry.c
//
NTSTATUS
MyRegOpenKey(
    IN HANDLE hKey,
    IN LPWSTR lpSubKey,
    OUT PHANDLE phResult
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

//
// Function prototypes
//

INT_PTR ConsolePropertySheet(HWND hWnd);
BOOL    RegisterClasses(HANDLE hModule);
void    UnregisterClasses(HANDLE hModule);
INT_PTR FontDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
PCONSOLE_STATE_INFO InitRegistryValues(VOID);
DWORD   GetRegistryValues(PCONSOLE_STATE_INFO StateInfo);
VOID    SetRegistryValues(PCONSOLE_STATE_INFO StateInfo, DWORD dwPage);
PCONSOLE_STATE_INFO InitStateValues(HANDLE hMap);
PCONSOLE_STATE_INFO ReadStateValues(HANDLE hMap);
BOOL    WriteStateValues(PCONSOLE_STATE_INFO pStateInfo);
LRESULT ColorControlProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
LRESULT FontPreviewWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
LRESULT PreviewWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CommonDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
VOID    EndDlgPage(HWND hDlg);
BOOL    UpdateStateInfo(HWND hDlg, UINT Item, int Value);
BOOL    WereWeStartedFromALnk();
BOOL    SetLinkValues( PCONSOLE_STATE_INFO StateInfo );

//
// Macros
//

#define NELEM(array) (sizeof(array)/sizeof(array[0]))
#define AttrToRGB(Attr) (gpStateInfo->ColorTable[(Attr) & 0x0F])
#define ScreenTextColor(pStateInfo) \
            (AttrToRGB(LOBYTE(pStateInfo->ScreenAttributes) & 0x0F))
#define ScreenBkColor(pStateInfo) \
            (AttrToRGB(LOBYTE(pStateInfo->ScreenAttributes >> 4)))
#define PopupTextColor(pStateInfo) \
            (AttrToRGB(LOBYTE(pStateInfo->PopupAttributes) & 0x0F))
#define PopupBkColor(pStateInfo) \
            (AttrToRGB(LOBYTE(pStateInfo->PopupAttributes >> 4)))

#ifdef DEBUG_PRINT
  #define _DBGFONTS  0x00000001
  #define _DBGFONTS2 0x00000002
  #define _DBGCHARS  0x00000004
  #define _DBGOUTPUT 0x00000008
  #define _DBGALL    0xFFFFFFFF
  extern ULONG gDebugFlag;

  #define DBGFONTS(_params_)  {if (gDebugFlag & _DBGFONTS) DbgPrint _params_ ; }
  #define DBGFONTS2(_params_) {if (gDebugFlag & _DBGFONTS2)DbgPrint _params_ ; }
  #define DBGCHARS(_params_)  {if (gDebugFlag & _DBGCHARS) DbgPrint _params_ ; }
  #define DBGOUTPUT(_params_) {if (gDebugFlag & _DBGOUTPUT)DbgPrint _params_ ; }
  #define DBGPRINT(_params_)  DbgPrint _params_
#else
  #define DBGFONTS(_params_)
  #define DBGFONTS2(_params_)
  #define DBGCHARS(_params_)
  #define DBGOUTPUT(_params_)
  #define DBGPRINT(_params_)
#endif

#ifdef FE_SB
// Macro definitions that handle codepages
//
#define CP_US       (UINT)437
#define CP_JPN      (UINT)932
#define CP_WANSUNG  (UINT)949
#define CP_TC       (UINT)950
#define CP_SC       (UINT)936

#define IsBilingualCP(cp) ((cp)==CP_JPN || (cp)==CP_WANSUNG)
#define IsFarEastCP(cp) ((cp)==CP_JPN || (cp)==CP_WANSUNG || (cp)==CP_TC || (cp)==CP_SC)
#endif


