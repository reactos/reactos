/**********************************************************************/
/*      IMM.H - Input Method Manager definitions                      */
/*                                                                    */
/*      Copyright (c) 1993-1996  Microsoft Corporation                */
/**********************************************************************/

#ifndef _IMM_
#define _IMM_        // defined if IMM.H has been included

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WINUSER_
#define VK_PROCESSKEY 0x0E5
#endif

typedef DWORD     HIMC;
typedef DWORD     HIMCC;

typedef HKL FAR  *LPHKL;
typedef UINT FAR *LPUINT;

typedef struct tagCOMPOSITIONFORM {
    DWORD dwStyle;
    POINT ptCurrentPos;
    RECT  rcArea;
} COMPOSITIONFORM, *PCOMPOSITIONFORM, NEAR *NPCOMPOSITIONFORM, FAR *LPCOMPOSITIONFORM;

typedef struct tagCANDIDATEFORM {
    DWORD dwIndex;
    DWORD dwStyle;
    POINT ptCurrentPos;
    RECT  rcArea;
} CANDIDATEFORM, *PCANDIDATEFORM, NEAR *NPCANDIDATEFORM, FAR *LPCANDIDATEFORM;

typedef struct tagCANDIDATELIST {
    DWORD dwSize;
    DWORD dwStyle;
    DWORD dwCount;
    DWORD dwSelection;
    DWORD dwPageStart;
    DWORD dwPageSize;
    DWORD dwOffset[1];
} CANDIDATELIST, *PCANDIDATELIST, NEAR *NPCANDIDATELIST, FAR *LPCANDIDATELIST;

typedef struct tagREGISTERWORDA {
    LPSTR   lpReading;
    LPSTR   lpWord;
} REGISTERWORDA, *PREGISTERWORDA, NEAR *NPREGISTERWORDA, FAR *LPREGISTERWORDA;
typedef struct tagREGISTERWORDW {
    LPWSTR  lpReading;
    LPWSTR  lpWord;
} REGISTERWORDW, *PREGISTERWORDW, NEAR *NPREGISTERWORDW, FAR *LPREGISTERWORDW;
#ifdef UNICODE
typedef REGISTERWORDW REGISTERWORD;
typedef PREGISTERWORDW PREGISTERWORD;
typedef NPREGISTERWORDW NPREGISTERWORD;
typedef LPREGISTERWORDW LPREGISTERWORD;
#else
typedef REGISTERWORDA REGISTERWORD;
typedef PREGISTERWORDA PREGISTERWORD;
typedef NPREGISTERWORDA NPREGISTERWORD;
typedef LPREGISTERWORDA LPREGISTERWORD;
#endif // UNICODE

#define STYLE_DESCRIPTION_SIZE  32

typedef struct tagSTYLEBUFA {
    DWORD       dwStyle;
    CHAR        szDescription[STYLE_DESCRIPTION_SIZE];
} STYLEBUFA, *PSTYLEBUFA, NEAR *NPSTYLEBUFA, FAR *LPSTYLEBUFA;
typedef struct tagSTYLEBUFW {
    DWORD       dwStyle;
    WCHAR       szDescription[STYLE_DESCRIPTION_SIZE];
} STYLEBUFW, *PSTYLEBUFW, NEAR *NPSTYLEBUFW, FAR *LPSTYLEBUFW;
#ifdef UNICODE
typedef STYLEBUFW STYLEBUF;
typedef PSTYLEBUFW PSTYLEBUF;
typedef NPSTYLEBUFW NPSTYLEBUF;
typedef LPSTYLEBUFW LPSTYLEBUF;
#else
typedef STYLEBUFA STYLEBUF;
typedef PSTYLEBUFA PSTYLEBUF;
typedef NPSTYLEBUFA NPSTYLEBUF;
typedef LPSTYLEBUFA LPSTYLEBUF;
#endif // UNICODE

// prototype of IMM API

HKL  WINAPI ImmInstallIMEA(LPCSTR lpszIMEFileName, LPCSTR lpszLayoutText);
HKL  WINAPI ImmInstallIMEW(LPCWSTR lpszIMEFileName, LPCWSTR lpszLayoutText);
#ifdef UNICODE
#define ImmInstallIME  ImmInstallIMEW
#else
#define ImmInstallIME  ImmInstallIMEA
#endif // !UNICODE

HWND WINAPI ImmGetDefaultIMEWnd(HWND);

UINT WINAPI ImmGetDescriptionA(HKL, LPSTR, UINT uBufLen);
UINT WINAPI ImmGetDescriptionW(HKL, LPWSTR, UINT uBufLen);
#ifdef UNICODE
#define ImmGetDescription  ImmGetDescriptionW
#else
#define ImmGetDescription  ImmGetDescriptionA
#endif // !UNICODE

UINT WINAPI ImmGetIMEFileNameA(HKL, LPSTR, UINT uBufLen);
UINT WINAPI ImmGetIMEFileNameW(HKL, LPWSTR, UINT uBufLen);
#ifdef UNICODE
#define ImmGetIMEFileName  ImmGetIMEFileNameW
#else
#define ImmGetIMEFileName  ImmGetIMEFileNameA
#endif // !UNICODE

DWORD WINAPI ImmGetProperty(HKL, DWORD);

BOOL WINAPI ImmIsIME(HKL);

BOOL WINAPI ImmSimulateHotKey(HWND, DWORD);

HIMC WINAPI ImmCreateContext(void);
BOOL WINAPI ImmDestroyContext(HIMC);
HIMC WINAPI ImmGetContext(HWND);
BOOL WINAPI ImmReleaseContext(HWND, HIMC);
HIMC WINAPI ImmAssociateContext(HWND, HIMC);

LONG  WINAPI ImmGetCompositionStringA(HIMC, DWORD, LPVOID, DWORD);
LONG  WINAPI ImmGetCompositionStringW(HIMC, DWORD, LPVOID, DWORD);
#ifdef UNICODE
#define ImmGetCompositionString  ImmGetCompositionStringW
#else
#define ImmGetCompositionString  ImmGetCompositionStringA
#endif // !UNICODE

BOOL  WINAPI ImmSetCompositionStringA(HIMC, DWORD dwIndex, LPCVOID lpComp, DWORD, LPCVOID lpRead, DWORD);
BOOL  WINAPI ImmSetCompositionStringW(HIMC, DWORD dwIndex, LPCVOID lpComp, DWORD, LPCVOID lpRead, DWORD);
#ifdef UNICODE
#define ImmSetCompositionString  ImmSetCompositionStringW
#else
#define ImmSetCompositionString  ImmSetCompositionStringA
#endif // !UNICODE

DWORD WINAPI ImmGetCandidateListCountA(HIMC, LPDWORD lpdwListCount);
DWORD WINAPI ImmGetCandidateListCountW(HIMC, LPDWORD lpdwListCount);
#ifdef UNICODE
#define ImmGetCandidateListCount  ImmGetCandidateListCountW
#else
#define ImmGetCandidateListCount  ImmGetCandidateListCountA
#endif // !UNICODE

DWORD WINAPI ImmGetCandidateListA(HIMC, DWORD deIndex, LPCANDIDATELIST, DWORD dwBufLen);
DWORD WINAPI ImmGetCandidateListW(HIMC, DWORD deIndex, LPCANDIDATELIST, DWORD dwBufLen);
#ifdef UNICODE
#define ImmGetCandidateList  ImmGetCandidateListW
#else
#define ImmGetCandidateList  ImmGetCandidateListA
#endif // !UNICODE

DWORD WINAPI ImmGetGuideLineA(HIMC, DWORD dwIndex, LPSTR, DWORD dwBufLen);
DWORD WINAPI ImmGetGuideLineW(HIMC, DWORD dwIndex, LPWSTR, DWORD dwBufLen);
#ifdef UNICODE
#define ImmGetGuideLine  ImmGetGuideLineW
#else
#define ImmGetGuideLine  ImmGetGuideLineA
#endif // !UNICODE

BOOL WINAPI ImmGetConversionStatus(HIMC, LPDWORD, LPDWORD);
BOOL WINAPI ImmSetConversionStatus(HIMC, DWORD, DWORD);
BOOL WINAPI ImmGetOpenStatus(HIMC);
BOOL WINAPI ImmSetOpenStatus(HIMC, BOOL);

BOOL WINAPI ImmGetCompositionFontA(HIMC, LPLOGFONTA);
BOOL WINAPI ImmGetCompositionFontW(HIMC, LPLOGFONTW);
#ifdef UNICODE
#define ImmGetCompositionFont  ImmGetCompositionFontW
#else
#define ImmGetCompositionFont  ImmGetCompositionFontA
#endif // !UNICODE

BOOL WINAPI ImmSetCompositionFontA(HIMC, LPLOGFONTA);
BOOL WINAPI ImmSetCompositionFontW(HIMC, LPLOGFONTW);
#ifdef UNICODE
#define ImmSetCompositionFont  ImmSetCompositionFontW
#else
#define ImmSetCompositionFont  ImmSetCompositionFontA
#endif // !UNICODE

BOOL    WINAPI ImmConfigureIMEA(HKL, HWND, DWORD, LPVOID);
BOOL    WINAPI ImmConfigureIMEW(HKL, HWND, DWORD, LPVOID);
#ifdef UNICODE
#define ImmConfigureIME  ImmConfigureIMEW
#else
#define ImmConfigureIME  ImmConfigureIMEA
#endif // !UNICODE

LRESULT WINAPI ImmEscapeA(HKL, HIMC, UINT, LPVOID);
LRESULT WINAPI ImmEscapeW(HKL, HIMC, UINT, LPVOID);
#ifdef UNICODE
#define ImmEscape  ImmEscapeW
#else
#define ImmEscape  ImmEscapeA
#endif // !UNICODE

DWORD   WINAPI ImmGetConversionListA(HKL, HIMC, LPCSTR, LPCANDIDATELIST, DWORD dwBufLen, UINT uFlag);
DWORD   WINAPI ImmGetConversionListW(HKL, HIMC, LPCWSTR, LPCANDIDATELIST, DWORD dwBufLen, UINT uFlag);
#ifdef UNICODE
#define ImmGetConversionList  ImmGetConversionListW
#else
#define ImmGetConversionList  ImmGetConversionListA
#endif // !UNICODE

BOOL    WINAPI ImmNotifyIME(HIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue);

BOOL WINAPI ImmGetStatusWindowPos(HIMC, LPPOINT);
BOOL WINAPI ImmSetStatusWindowPos(HIMC, LPPOINT);
BOOL WINAPI ImmGetCompositionWindow(HIMC, LPCOMPOSITIONFORM);
BOOL WINAPI ImmSetCompositionWindow(HIMC, LPCOMPOSITIONFORM);
BOOL WINAPI ImmGetCandidateWindow(HIMC, DWORD, LPCANDIDATEFORM);
BOOL WINAPI ImmSetCandidateWindow(HIMC, LPCANDIDATEFORM);

BOOL WINAPI ImmIsUIMessageA(HWND, UINT, WPARAM, LPARAM);
BOOL WINAPI ImmIsUIMessageW(HWND, UINT, WPARAM, LPARAM);
#ifdef UNICODE
#define ImmIsUIMessage  ImmIsUIMessageW
#else
#define ImmIsUIMessage  ImmIsUIMessageA
#endif // !UNICODE

UINT WINAPI ImmGetVirtualKey(HWND);

typedef int (CALLBACK *REGISTERWORDENUMPROCA)(LPCSTR, DWORD, LPCSTR, LPVOID);
typedef int (CALLBACK *REGISTERWORDENUMPROCW)(LPCWSTR, DWORD, LPCWSTR, LPVOID);
#ifdef UNICODE
#define REGISTERWORDENUMPROC  REGISTERWORDENUMPROCW
#else
#define REGISTERWORDENUMPROC  REGISTERWORDENUMPROCA
#endif // !UNICODE

BOOL WINAPI ImmRegisterWordA(HKL, LPCSTR lpszReading, DWORD, LPCSTR lpszRegister);
BOOL WINAPI ImmRegisterWordW(HKL, LPCWSTR lpszReading, DWORD, LPCWSTR lpszRegister);
#ifdef UNICODE
#define ImmRegisterWord  ImmRegisterWordW
#else
#define ImmRegisterWord  ImmRegisterWordA
#endif // !UNICODE

BOOL WINAPI ImmUnregisterWordA(HKL, LPCSTR lpszReading, DWORD, LPCSTR lpszUnregister);
BOOL WINAPI ImmUnregisterWordW(HKL, LPCWSTR lpszReading, DWORD, LPCWSTR lpszUnregister);
#ifdef UNICODE
#define ImmUnregisterWord  ImmUnregisterWordW
#else
#define ImmUnregisterWord  ImmUnregisterWordA
#endif // !UNICODE

UINT WINAPI ImmGetRegisterWordStyleA(HKL, UINT nItem, LPSTYLEBUFA);
UINT WINAPI ImmGetRegisterWordStyleW(HKL, UINT nItem, LPSTYLEBUFW);
#ifdef UNICODE
#define ImmGetRegisterWordStyle  ImmGetRegisterWordStyleW
#else
#define ImmGetRegisterWordStyle  ImmGetRegisterWordStyleA
#endif // !UNICODE

UINT WINAPI ImmEnumRegisterWordA(HKL, REGISTERWORDENUMPROCA, LPCSTR lpszReading, DWORD, LPCSTR lpszRegister, LPVOID);
UINT WINAPI ImmEnumRegisterWordW(HKL, REGISTERWORDENUMPROCW, LPCWSTR lpszReading, DWORD, LPCWSTR lpszRegister, LPVOID);
#ifdef UNICODE
#define ImmEnumRegisterWord  ImmEnumRegisterWordW
#else
#define ImmEnumRegisterWord  ImmEnumRegisterWordA
#endif // !UNICODE

#ifndef _WINUSER_ 
// the IME related messages
#define WM_CONVERTREQUESTEX             0x0108
#define WM_IME_STARTCOMPOSITION         0x010D
#define WM_IME_ENDCOMPOSITION           0x010E
#define WM_IME_COMPOSITION              0x010F
#define WM_IME_KEYLAST                  0x010F

#define WM_IME_SETCONTEXT               0x0281
#define WM_IME_NOTIFY                   0x0282
#define WM_IME_CONTROL                  0x0283
#define WM_IME_COMPOSITIONFULL          0x0284
#define WM_IME_SELECT                   0x0285
#define WM_IME_CHAR                     0x0286

#define WM_IME_KEYDOWN                  0x0290
#define WM_IME_KEYUP                    0x0291

// wParam for WM_IME_CONTROL                 
#define IMC_GETCANDIDATEPOS             0x0007
#define IMC_SETCANDIDATEPOS             0x0008
#define IMC_GETCOMPOSITIONFONT          0x0009
#define IMC_SETCOMPOSITIONFONT          0x000A
#define IMC_GETCOMPOSITIONWINDOW        0x000B
#define IMC_SETCOMPOSITIONWINDOW        0x000C
#define IMC_GETSTATUSWINDOWPOS          0x000F
#define IMC_SETSTATUSWINDOWPOS          0x0010
#define IMC_CLOSESTATUSWINDOW           0x0021
#define IMC_OPENSTATUSWINDOW            0x0022

#endif /* !_WINUSER_ */

// wParam for WM_IME_CONTROL to the soft keyboard
// dwAction for ImmNotifyIME
#define NI_OPENCANDIDATE                0x0010
#define NI_CLOSECANDIDATE               0x0011
#define NI_SELECTCANDIDATESTR           0x0012
#define NI_CHANGECANDIDATELIST          0x0013
#define NI_FINALIZECONVERSIONRESULT     0x0014
#define NI_COMPOSITIONSTR               0x0015
#define NI_SETCANDIDATE_PAGESTART       0x0016
#define NI_SETCANDIDATE_PAGESIZE        0x0017

// lParam for WM_IME_SETCONTEXT
#define ISC_SHOWUICANDIDATEWINDOW       0x00000001
#define ISC_SHOWUICOMPOSITIONWINDOW     0x80000000
#define ISC_SHOWUIGUIDELINE             0x40000000
#define ISC_SHOWUIALLCANDIDATEWINDOW    0x0000000F
#define ISC_SHOWUIALL                   0xC000000F

// dwIndex for ImmNotifyIME/NI_COMPOSITIONSTR
#define CPS_COMPLETE                    0x0001
#define CPS_CONVERT                     0x0002
#define CPS_REVERT                      0x0003
#define CPS_CANCEL                      0x0004

// the modifiers of hot key
#define MOD_ALT                         0x0001
#define MOD_CONTROL                     0x0002
#define MOD_SHIFT                       0x0004

#define MOD_LEFT                        0x8000
#define MOD_RIGHT                       0x4000

#define MOD_ON_KEYUP                    0x0800
#define MOD_IGNORE_ALL_MODIFIER         0x0400

// Windows for Simplified Chinese Edition hot key ID from 0x10 - 0x2F
#define IME_CHOTKEY_IME_NONIME_TOGGLE           0x10
#define IME_CHOTKEY_SHAPE_TOGGLE                0x11
#define IME_CHOTKEY_SYMBOL_TOGGLE               0x12

// Windows for Japanese Edition hot key ID from 0x30 - 0x4F
#define IME_JHOTKEY_CLOSE_OPEN                  0x30

// Windows for Korean Edition hot key ID from 0x50 - 0x6F
#define IME_KHOTKEY_SHAPE_TOGGLE                0x50
#define IME_KHOTKEY_HANJACONVERT                0x51
#define IME_KHOTKEY_ENGLISH                     0x52

// Windows for Tranditional Chinese Edition hot key ID from 0x70 - 0x8F
#define IME_THOTKEY_IME_NONIME_TOGGLE           0x70
#define IME_THOTKEY_SHAPE_TOGGLE                0x71
#define IME_THOTKEY_SYMBOL_TOGGLE               0x72

// direct switch hot key ID from 0x100 - 0x11F
#define IME_HOTKEY_DSWITCH_FIRST                0x100
#define IME_HOTKEY_DSWITCH_LAST                 0x11F

// IME private hot key from 0x200 - 0x21F
#define IME_ITHOTKEY_RESEND_RESULTSTR           0x200
#define IME_ITHOTKEY_PREVIOUS_COMPOSITION       0x201
#define IME_ITHOTKEY_UISTYLE_TOGGLE             0x202

// parameter of ImmGetCompositionString
#define GCS_COMPREADSTR                 0x0001
#define GCS_COMPREADATTR                0x0002
#define GCS_COMPREADCLAUSE              0x0004
#define GCS_COMPSTR                     0x0008
#define GCS_COMPATTR                    0x0010
#define GCS_COMPCLAUSE                  0x0020
#define GCS_CURSORPOS                   0x0080
#define GCS_DELTASTART                  0x0100
#define GCS_RESULTREADSTR               0x0200
#define GCS_RESULTREADCLAUSE            0x0400
#define GCS_RESULTSTR                   0x0800
#define GCS_RESULTCLAUSE                0x1000

// style bit flags for WM_IME_COMPOSITION
#define CS_INSERTCHAR                   0x2000
#define CS_NOMOVECARET                  0x4000

// bits of fdwInit of INPUTCONTEXT
// IME version constants
#define IMEVER_0310                     0x0003000A
#define IMEVER_0400                     0x00040000

// IME property bits
#define IME_PROP_AT_CARET               0x00010000
#define IME_PROP_SPECIAL_UI             0x00020000
#define IME_PROP_CANDLIST_START_FROM_1  0x00040000
#define IME_PROP_UNICODE                0x00080000

// IME UICapability bits
#define UI_CAP_2700                     0x00000001
#define UI_CAP_ROT90                    0x00000002
#define UI_CAP_ROTANY                   0x00000004

// ImmSetCompositionString Capability bits
#define SCS_CAP_COMPSTR                 0x00000001
#define SCS_CAP_MAKEREAD                0x00000002

// IME WM_IME_SELECT inheritance Capability bits
#define SELECT_CAP_CONVERSION           0x00000001
#define SELECT_CAP_SENTENCE             0x00000002

// ID for deIndex of ImmGetGuideLine
#define GGL_LEVEL                       0x00000001
#define GGL_INDEX                       0x00000002
#define GGL_STRING                      0x00000003
#define GGL_PRIVATE                     0x00000004

// ID for dwLevel of GUIDELINE Structure
#define GL_LEVEL_NOGUIDELINE            0x00000000
#define GL_LEVEL_FATAL                  0x00000001
#define GL_LEVEL_ERROR                  0x00000002
#define GL_LEVEL_WARNING                0x00000003
#define GL_LEVEL_INFORMATION            0x00000004

// ID for dwIndex of GUIDELINE Structure
#define GL_ID_UNKNOWN                   0x00000000
#define GL_ID_NOMODULE                  0x00000001
#define GL_ID_NODICTIONARY              0x00000010
#define GL_ID_CANNOTSAVE                0x00000011
#define GL_ID_NOCONVERT                 0x00000020
#define GL_ID_TYPINGERROR               0x00000021
#define GL_ID_TOOMANYSTROKE             0x00000022
#define GL_ID_READINGCONFLICT           0x00000023
#define GL_ID_INPUTREADING              0x00000024
#define GL_ID_INPUTRADICAL              0x00000025
#define GL_ID_INPUTCODE                 0x00000026
#define GL_ID_INPUTSYMBOL               0x00000027
#define GL_ID_CHOOSECANDIDATE           0x00000028
#define GL_ID_REVERSECONVERSION         0x00000029
#define GL_ID_PRIVATE_FIRST             0x00008000
#define GL_ID_PRIVATE_LAST              0x0000FFFF

// ID for dwIndex of ImmGetProperty
#define IGP_GETIMEVERSION               (DWORD)(-4)
#define IGP_PROPERTY                    0x00000004
#define IGP_CONVERSION                  0x00000008
#define IGP_SENTENCE                    0x0000000c
#define IGP_UI                          0x00000010
#define IGP_SETCOMPSTR                  0x00000014
#define IGP_SELECT                      0x00000018

// dwIndex for ImmSetCompositionString API
#define SCS_SETSTR                      (GCS_COMPREADSTR|GCS_COMPSTR)
#define SCS_CHANGEATTR                  (GCS_COMPREADATTR|GCS_COMPATTR)
#define SCS_CHANGECLAUSE                (GCS_COMPREADCLAUSE|GCS_COMPCLAUSE)

// attribute for COMPOSITIONSTRING Structure
#define ATTR_INPUT                      0x00
#define ATTR_TARGET_CONVERTED           0x01
#define ATTR_CONVERTED                  0x02
#define ATTR_TARGET_NOTCONVERTED        0x03
#define ATTR_INPUT_ERROR                0x04

// bit field for IMC_SETCOMPOSITIONWINDOW, IMC_SETCANDIDATEWINDOW
#define CFS_DEFAULT                     0x0000
#define CFS_RECT                        0x0001
#define CFS_POINT                       0x0002
#define CFS_SCREEN                      0x0004
#define CFS_FORCE_POSITION              0x0020
#define CFS_CANDIDATEPOS                0x0040
#define CFS_EXCLUDE                     0x0080

// conversion direction for ImmGetConversionList
#define GCL_CONVERSION                  0x0001
#define GCL_REVERSECONVERSION           0x0002
#define GCL_REVERSE_LENGTH              0x0003

// bit field for conversion mode
#define IME_CMODE_ALPHANUMERIC          0x0000
#define IME_CMODE_NATIVE                0x0001
#define IME_CMODE_CHINESE               IME_CMODE_NATIVE
#define IME_CMODE_HANGEUL               IME_CMODE_NATIVE
#define IME_CMODE_JAPANESE              IME_CMODE_NATIVE
#define IME_CMODE_KATAKANA              0x0002  // only effect under IME_CMODE_NATIVE
#define IME_CMODE_LANGUAGE              0x0003
#define IME_CMODE_FULLSHAPE             0x0008
#define IME_CMODE_ROMAN                 0x0010
#define IME_CMODE_CHARCODE              0x0020
#define IME_CMODE_HANJACONVERT          0x0040
#define IME_CMODE_SOFTKBD               0x0080
#define IME_CMODE_NOCONVERSION          0x0100
#define IME_CMODE_EUDC                  0x0200
#define IME_CMODE_SYMBOL                0x0400

#define IME_SMODE_NONE                  0x0000
#define IME_SMODE_PLAURALCLAUSE         0x0001
#define IME_SMODE_SINGLECONVERT         0x0002
#define IME_SMODE_AUTOMATIC             0x0004
#define IME_SMODE_PHRASEPREDICT         0x0008

// style of candidate
#define IME_CAND_UNKNOWN                0x0000
#define IME_CAND_READ                   0x0001
#define IME_CAND_CODE                   0x0002
#define IME_CAND_MEANING                0x0003
#define IME_CAND_RADICAL                0x0004
#define IME_CAND_STROKE                 0x0005

// wParam of report message WM_IME_NOTIFY
#define IMN_CLOSESTATUSWINDOW           0x0001
#define IMN_OPENSTATUSWINDOW            0x0002
#define IMN_CHANGECANDIDATE             0x0003
#define IMN_CLOSECANDIDATE              0x0004
#define IMN_OPENCANDIDATE               0x0005
#define IMN_SETCONVERSIONMODE           0x0006
#define IMN_SETSENTENCEMODE             0x0007
#define IMN_SETOPENSTATUS               0x0008
#define IMN_SETCANDIDATEPOS             0x0009
#define IMN_SETCOMPOSITIONFONT          0x000A
#define IMN_SETCOMPOSITIONWINDOW        0x000B
#define IMN_SETSTATUSWINDOWPOS          0x000C
#define IMN_GUIDELINE                   0x000D
#define IMN_PRIVATE                     0x000E

// error code of ImmGetCompositionString
#define IMM_ERROR_NODATA                (-1)
#define IMM_ERROR_GENERAL               (-2)

// dialog mode of ImmConfigureIME
#define IME_CONFIG_GENERAL              1
#define IME_CONFIG_REGISTERWORD         2
#define IME_CONFIG_SELECTDICTIONARY     3

// dialog mode of ImmEscape
#define IME_ESC_QUERY_SUPPORT           0x0003
#define IME_ESC_RESERVED_FIRST          0x0004
#define IME_ESC_RESERVED_LAST           0x07FF
#define IME_ESC_PRIVATE_FIRST           0x0800
#define IME_ESC_PRIVATE_LAST            0x0FFF
#define IME_ESC_SEQUENCE_TO_INTERNAL    0x1001
#define IME_ESC_GET_EUDC_DICTIONARY     0x1003
#define IME_ESC_SET_EUDC_DICTIONARY     0x1004
#define IME_ESC_MAX_KEY                 0x1005
#define IME_ESC_IME_NAME                0x1006
#define IME_ESC_SYNC_HOTKEY             0x1007
#define IME_ESC_HANJA_MODE              0x1008
#define IME_ESC_AUTOMATA                0x1009

// style of word registration
#define IME_REGWORD_STYLE_EUDC          0x00000001
#define IME_REGWORD_STYLE_USER_FIRST    0x80000000
#define IME_REGWORD_STYLE_USER_LAST     0xFFFFFFFF

// type of soft keyboard
// for Windows Tranditional Chinese Edition
#define SOFTKEYBOARD_TYPE_T1            0x0001
// for Windows Simplified Chinese Edition
#define SOFTKEYBOARD_TYPE_C1            0x0002

#ifdef __cplusplus
}
#endif

#endif  // _IMM_
