/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    font.h

Abstract:

    This module contains the header information for the Win32 font dialogs.

Revision History:

--*/



//
//  Include Files.
//

#include <help.h>

#ifdef  MM_DESIGNVECTOR_DEFINED

// new flag that for NT 5.0/IE 5.0 is used for testing purpose only
#define CF_MM_DESIGNVECTOR             0x02000000L

#if (_WIN32_WINNT < 0x0500)
//
// new NT 5.0 definitions related to MultipleMaster desing vector, from WinGdi.h
//
#define STAMP_DESIGNVECTOR  (0x8000000 + 'd' + ('v' << 8))
#define STAMP_AXESLIST      (0x8000000 + 'a' + ('l' << 8))
#define MM_MAX_NUMAXES      16



typedef struct tagDESIGNVECTOR
{
    DWORD  dvReserved;
    DWORD  dvNumAxes;
    LONG   dvValues[MM_MAX_NUMAXES];
} DESIGNVECTOR, *PDESIGNVECTOR, FAR *LPDESIGNVECTOR;

typedef struct tagENUMLOGFONTEXDVA
{
    ENUMLOGFONTEXA elfEnumLogfontEx;
    DESIGNVECTOR   elfDesignVector;
} ENUMLOGFONTEXDVA, *PENUMLOGFONTEXDVA, FAR *LPENUMLOGFONTEXDVA;
typedef struct tagENUMLOGFONTEXDVW
{
    ENUMLOGFONTEXW elfEnumLogfontEx;
    DESIGNVECTOR   elfDesignVector;
} ENUMLOGFONTEXDVW, *PENUMLOGFONTEXDVW, FAR *LPENUMLOGFONTEXDVW;
#ifdef UNICODE
typedef ENUMLOGFONTEXDVW ENUMLOGFONTEXDV;
typedef PENUMLOGFONTEXDVW PENUMLOGFONTEXDV;
typedef LPENUMLOGFONTEXDVW LPENUMLOGFONTEXDV;
#else
typedef ENUMLOGFONTEXDVA ENUMLOGFONTEXDV;
typedef PENUMLOGFONTEXDVA PENUMLOGFONTEXDV;
typedef LPENUMLOGFONTEXDVA LPENUMLOGFONTEXDV;
#endif // UNICODE


#define MM_MAX_AXES_NAMELEN 16

typedef struct tagAXISINFOA
{
    LONG   axMinValue;
    LONG   axMaxValue;
    BYTE   axAxisName[MM_MAX_AXES_NAMELEN];
} AXISINFOA, *PAXISINFOA, FAR *LPAXISINFOA;
typedef struct tagAXISINFOW
{
    LONG   axMinValue;
    LONG   axMaxValue;
    WCHAR  axAxisName[MM_MAX_AXES_NAMELEN];
} AXISINFOW, *PAXISINFOW, FAR *LPAXISINFOW;
#ifdef UNICODE
typedef AXISINFOW AXISINFO;
typedef PAXISINFOW PAXISINFO;
typedef LPAXISINFOW LPAXISINFO;
#else
typedef AXISINFOA AXISINFO;
typedef PAXISINFOA PAXISINFO;
typedef LPAXISINFOA LPAXISINFO;
#endif // UNICODE

typedef struct tagAXESLISTA
{
    DWORD     axlReserved;
    DWORD     axlNumAxes;
    AXISINFOA axlAxisInfo[MM_MAX_NUMAXES];
} AXESLISTA, *PAXESLISTA, FAR *LPAXESLISTA;
typedef struct tagAXESLISTW
{
    DWORD     axlReserved;
    DWORD     axlNumAxes;
    AXISINFOW axlAxisInfo[MM_MAX_NUMAXES];
} AXESLISTW, *PAXESLISTW, FAR *LPAXESLISTW;
#ifdef UNICODE
typedef AXESLISTW AXESLIST;
typedef PAXESLISTW PAXESLIST;
typedef LPAXESLISTW LPAXESLIST;
#else
typedef AXESLISTA AXESLIST;
typedef PAXESLISTA PAXESLIST;
typedef LPAXESLISTA LPAXESLIST;
#endif // UNICODE

typedef struct tagENUMTEXTMETRICA
{
    NEWTEXTMETRICEXA etmNewTextMetricEx;
    AXESLISTA        etmAxesList;
} ENUMTEXTMETRICA, *PENUMTEXTMETRICA, FAR *LPENUMTEXTMETRICA;
typedef struct tagENUMTEXTMETRICW
{
    NEWTEXTMETRICEXW etmNewTextMetricEx;
    AXESLISTW        etmAxesList;
} ENUMTEXTMETRICW, *PENUMTEXTMETRICW, FAR *LPENUMTEXTMETRICW;

#ifdef UNICODE
typedef ENUMTEXTMETRICW ENUMTEXTMETRIC;
typedef PENUMTEXTMETRICW PENUMTEXTMETRIC;
typedef LPENUMTEXTMETRICW LPENUMTEXTMETRIC;
#else
typedef ENUMTEXTMETRICA ENUMTEXTMETRIC;
typedef PENUMTEXTMETRICA PENUMTEXTMETRIC;
typedef LPENUMTEXTMETRICA LPENUMTEXTMETRIC;
#endif // UNICODE

#endif // (_WIN32_WINNT < 0x0500)
#endif //  MM_DESIGNVECTOR_DEFINED

//
//  Constant Declarations.
//

// Finnish needs 17 chars (18 w/ NULL) -- let's give them 20.
#define CCHCOLORNAMEMAX      20        // max length of color name text
#define CCHCOLORS            16        // max # of pure colors in color combo

#define POINTS_PER_INCH      72
#define FFMASK               0xf0      // pitch and family mask
#define CCHSTDSTRING         12        // max length of sample text string

#define FONTPROP   (LPCTSTR) 0xA000L

#define CBN_MYEDITUPDATE     (WM_USER + 501)
#define KEY_FONT_SUBS TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes")

#define DEF_POINT_SIZE       10

//If you add a bitmaps to the font bitmap you should modify this constant.
#define NUM_OF_BITMAP        5
#define DX_BITMAP            20
#define DY_BITMAP            12

#define FONT_INVALID_CHARSET 0x100




//
//  Typedef Declarations.
//

#ifdef  MM_DESIGNVECTOR_DEFINED
// CreateFontIndirectEx
typedef HFONT (WINAPI *PFNCREATEFONTINDIRECTEX)( IN CONST ENUMLOGFONTEXDV *);
#endif //  MM_DESIGNVECTOR_DEFINED

typedef struct {
    UINT            ApiType;
    LPCHOOSEFONT    pCF;
    UINT            iCharset;
    RECT            rcText;
    DWORD           nLastFontType;
    DWORD           ProcessVersion;

#ifdef  MM_DESIGNVECTOR_DEFINED
    PFNCREATEFONTINDIRECTEX pfnCreateFontIndirectEx;
    DESIGNVECTOR   DefaultDesignVector;
#endif //  MM_DESIGNVECTOR_DEFINED

#ifdef UNICODE
    LPCHOOSEFONTA   pCFA;
    PUNICODE_STRING pusStyle;
    PANSI_STRING    pasStyle;
#endif
} FONTINFO;

typedef FONTINFO *PFONTINFO;


typedef struct {
    HWND hwndFamily;
    HWND hwndStyle;
    HWND hwndSizes;
    HWND hwndScript;
    UINT iCharset;                // returned for enumerating scripts
    UINT cfdCharset;              // ChooseFontData charset passed in here
    HDC hDC;
    DWORD dwFlags;
    DWORD nFontType;
    BOOL bFillSize;
    BOOL bPrinterFont;
    LPCHOOSEFONT lpcf;
#ifdef  MM_DESIGNVECTOR_DEFINED
    HWND hwndParent;             // to extend the dialog for MM
    PDESIGNVECTOR   pDefaultDesignVector;
#endif //  MM_DESIGNVECTOR_DEFINED
} ENUM_FONT_DATA, *LPENUM_FONT_DATA;


typedef struct _ITEMDATA {
    PLOGFONT pLogFont;
    DWORD nFontType;
} ITEMDATA, *LPITEMDATA;


//
//  Chinese font numbers (zihao).
//
typedef struct {
    TCHAR name[5];
    int size;
    int sizeFr;
} ZIHAO;

#define NUM_ZIHAO  16

#ifdef UNICODE

ZIHAO stZihao[NUM_ZIHAO] =
{
    { L"\x516b\x53f7",  5, 0 }, { L"\x4e03\x53f7",  5, 5 },
    { L"\x5c0f\x516d",  6, 5 }, { L"\x516d\x53f7",  7, 5 },
    { L"\x5c0f\x4e94",  9, 0 }, { L"\x4e94\x53f7", 10, 5 },
    { L"\x5c0f\x56db", 12, 0 }, { L"\x56db\x53f7", 14, 0 },
    { L"\x5c0f\x4e09", 15, 0 }, { L"\x4e09\x53f7", 16, 0 },
    { L"\x5c0f\x4e8c", 18, 0 }, { L"\x4e8c\x53f7", 22, 0 },
    { L"\x5c0f\x4e00", 24, 0 }, { L"\x4e00\x53f7", 26, 0 },
    { L"\x5c0f\x521d", 36, 0 }, { L"\x521d\x53f7", 42, 0 }
};

#else

ZIHAO stZihao[NUM_ZIHAO] =
{
    { "\xb0\xcb\xba\xc5",  5, 0 }, { "\xc6\xdf\xba\xc5",  5, 5 },
    { "\xd0\xa1\xc1\xf9",  6, 5 }, { "\xc1\xf9\xba\xc5",  7, 5 },
    { "\xd0\xa1\xce\xe5",  9, 0 }, { "\xce\xe5\xba\xc5", 10, 5 },
    { "\xd0\xa1\xcb\xc4", 12, 0 }, { "\xcb\xc4\xba\xc5", 14, 0 },
    { "\xd0\xa1\xc8\xfd", 15, 0 }, { "\xc8\xfd\xba\xc5", 16, 0 },
    { "\xd0\xa1\xb6\xfe", 18, 0 }, { "\xb6\xfe\xba\xc5", 22, 0 },
    { "\xd0\xa1\xd2\xbb", 24, 0 }, { "\xd2\xbb\xba\xc5", 26, 0 },
    { "\xd0\xa1\xb3\xf5", 36, 0 }, { "\xb3\xf5\xba\xc5", 42, 0 }
};

#endif

//
//  Global Variables.
//

UINT msgWOWLFCHANGE;
UINT msgWOWCHOOSEFONT_GETLOGFONT;

//
//  Color tables for color combo box.
//  Order of values must match names in sz.src.
//
DWORD rgbColors[CCHCOLORS] =
{
        RGB(  0,   0, 0),       // Black
        RGB(128,   0, 0),       // Dark red
        RGB(  0, 128, 0),       // Dark green
        RGB(128, 128, 0),       // Dark yellow
        RGB(  0,   0, 128),     // Dark blue
        RGB(128,   0, 128),     // Dark purple
        RGB(  0, 128, 128),     // Dark aqua
        RGB(128, 128, 128),     // Dark grey
        RGB(192, 192, 192),     // Light grey
        RGB(255,   0, 0),       // Light red
        RGB(  0, 255, 0),       // Light green
        RGB(255, 255, 0),       // Light yellow
        RGB(  0,   0, 255),     // Light blue
        RGB(255,   0, 255),     // Light purple
        RGB(  0, 255, 255),     // Light aqua
        RGB(255, 255, 255),     // White
};

HBITMAP hbmFont = NULL;
HFONT hDlgFont = NULL;

UINT DefaultCharset;

TCHAR szRegular[CCHSTYLE];
TCHAR szBold[CCHSTYLE];
TCHAR szItalic[CCHSTYLE];
TCHAR szBoldItalic[CCHSTYLE];

TCHAR szPtFormat[] = TEXT("%d");

TCHAR c_szRegular[]    = TEXT("Regular");
TCHAR c_szBold[]       = TEXT("Bold");
TCHAR c_szItalic[]     = TEXT("Italic");
TCHAR c_szBoldItalic[] = TEXT("Bold Italic");

LPCFHOOKPROC glpfnFontHook = 0;

BOOL g_bIsSimplifiedChineseUI = FALSE;




//
//  Context Help IDs.
//

const static DWORD aFontHelpIDs[] =              // Context Help IDs
{
    stc1,    IDH_FONT_FONT,
    cmb1,    IDH_FONT_FONT,
    stc2,    IDH_FONT_STYLE,
    cmb2,    IDH_FONT_STYLE,
    stc3,    IDH_FONT_SIZE,
    cmb3,    IDH_FONT_SIZE,
    psh3,    IDH_COMM_APPLYNOW,
    grp1,    IDH_FONT_EFFECTS,
    chx1,    IDH_FONT_EFFECTS,
    chx2,    IDH_FONT_EFFECTS,
    stc4,    IDH_FONT_COLOR,
    cmb4,    IDH_FONT_COLOR,
    grp2,    IDH_FONT_SAMPLE,
    stc5,    IDH_FONT_SAMPLE,
    stc6,    NO_HELP,
    stc7,    IDH_FONT_SCRIPT,
    cmb5,    IDH_FONT_SCRIPT,

    0, 0
};

//
//  Function Prototypes.
//

#ifdef  MM_DESIGNVECTOR_DEFINED
/* flag used for ChooseFontExA, ChooseFontExW and ChooseFontX : */
   
#define CHF_DESIGNVECTOR  0x0001
/* give the MM result into DESIGNVECTOR,
   default is to convert the axis into the name for backwards compatibility */


/* ChooseFontExA and ChooseFontExW must be called with lpLogFont of size ENUMLOGFONTEXDV */
BOOL APIENTRY ChooseFontExA(LPCHOOSEFONTA, DWORD fl);
BOOL APIENTRY ChooseFontExW(LPCHOOSEFONTW, DWORD fl);
#ifdef UNICODE
#define ChooseFontEx  ChooseFontExW
#else
#define ChooseFontEx  ChooseFontExA
#endif // !UNICODE

#endif //  MM_DESIGNVECTOR_DEFINED

BOOL
ChooseFontX(
#ifdef  MM_DESIGNVECTOR_DEFINED
    PFONTINFO pFI, DWORD fl);
#else
    PFONTINFO pFI);
#endif //  MM_DESIGNVECTOR_DEFINED

VOID
SetStyleSelection(
    HWND hDlg,
    LPCHOOSEFONT lpcf,
    BOOL bInit);

#ifdef MM_DESIGNVECTOR_DEFINED
VOID 
SetMMAxesSelection(
    HWND hDlg,
    LPCHOOSEFONT lpcf);
#endif // MM_DESIGNVECTOR_DEFINED

VOID
HideDlgItem(
    HWND hDlg,
    int id);

VOID
FixComboHeights(
    HWND hDlg);

BOOL_PTR CALLBACK
FormatCharDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam);

void
SelectStyleFromLF(
    HWND hwnd,
    LPLOGFONT lplf);

int
CBSetTextFromSel(
    HWND hwnd);

int
CBSetSelFromText(
    HWND hwnd,
    LPTSTR lpszString);

int
CBGetTextAndData(
    HWND hwnd,
    LPTSTR lpszString,
    int iSize,
    PULONG_PTR lpdw);

int
CBFindString(
    HWND hwnd,
    LPTSTR lpszString);

BOOL
GetPointSizeInRange(
    HWND hDlg,
    LPCHOOSEFONT lpcf,
    LPINT pts,
    WORD wFlags);

BOOL
ResetSampleFromScript(
    HWND hdlg,
    HWND hwndScript,
    PFONTINFO pFI);

BOOL
ProcessDlgCtrlCommand(
    HWND hDlg,
    PFONTINFO pFI,
    WPARAM wParam,
    LPARAM lParam);

int
CmpFontType(
    DWORD ft1,
    DWORD ft2);

int
FontFamilyEnumProc(
    LPENUMLOGFONTEX lplf,
    LPNEWTEXTMETRIC lptm,
    DWORD nFontType,
    LPENUM_FONT_DATA lpData);

BOOL
GetFontFamily(
    HWND hDlg,
    HDC hDC,
    DWORD dwEnumCode,
    UINT iCharset);

VOID
CBAddSize(
    HWND hwnd,
    int pts,
    LPCHOOSEFONT lpcf);

int
InsertStyleSorted(
    HWND hwnd,
    LPTSTR lpszStyle,
    LPLOGFONT lplf);

PLOGFONT
CBAddStyle(
    HWND hwnd,
    LPTSTR lpszStyle,
    DWORD nFontType,
    LPLOGFONT lplf);

int
CBAddScript(
    HWND hwnd,
    LPTSTR lpszScript,
    UINT iCharset);

VOID
FillInMissingStyles(
    HWND hwnd);

VOID
FillScalableSizes(
    HWND hwnd,
    LPCHOOSEFONT lpcf);

int
FontStyleEnumProc(
    LPENUMLOGFONTEX lplf,
    LPNEWTEXTMETRIC lptm,
    DWORD nFontType,
    LPENUM_FONT_DATA lpData);

VOID
FreeFonts(
    HWND hwnd);

VOID
FreeAllItemData(
    HWND hDlg,
    PFONTINFO pFI);

VOID
InitLF(
    LPLOGFONT lplf);

#ifdef MM_DESIGNVECTOR_DEFINED
int
FontMMAxesEnumProc(
    LPENUMLOGFONTEXDV lplf,
    LPENUMTEXTMETRIC lpetm,
    DWORD nFontType,
    LPENUM_FONT_DATA lpData);
#endif // MM_DESIGNVECTOR_DEFINED

int
FontScriptEnumProc(
    LPENUMLOGFONTEX lplf,
    LPNEWTEXTMETRIC lptm,
    DWORD nFontType,
    LPENUM_FONT_DATA lpData);


BOOL
GetFontStylesAndSizes(
    HWND hDlg,
    PFONTINFO pFI,
    LPCHOOSEFONT lpcf,
    BOOL bForceSizeFill);

VOID
FillColorCombo(
    HWND hDlg);

BOOL
DrawSizeComboItem(
    LPDRAWITEMSTRUCT lpdis);

BOOL
DrawFamilyComboItem(
    LPDRAWITEMSTRUCT lpdis);

BOOL
DrawColorComboItem(
    LPDRAWITEMSTRUCT lpdis);

VOID
DrawSampleText(
    HWND hDlg,
    PFONTINFO pFI,
    LPCHOOSEFONT lpcf,
    HDC hDC);

BOOL
FillInFont(
    HWND hDlg,
    PFONTINFO pFI,
    LPCHOOSEFONT lpcf,
    LPLOGFONT lplf,
    BOOL bSetBits);

#ifdef MM_DESIGNVECTOR_DEFINED
BOOL
FillInFontEx(
    HWND hDlg,
    PFONTINFO pFI,
    LPCHOOSEFONT lpcf,
    LPENUMLOGFONTEXDV lplf,
    BOOL bSetBits);

BOOL
SetLogFontEx(
    HWND hDlg,
    LPCHOOSEFONT lpcf,
    LPENUMLOGFONTEXDV lplf);

#endif // MM_DESIGNVECTOR_DEFINED

BOOL
SetLogFont(
    HWND hDlg,
    LPCHOOSEFONT lpcf,
    LPLOGFONT lplf);

VOID
TermFont();

int
GetPointString(
    LPTSTR buf,
    HDC hDC,
    int height);

DWORD
FlipColor(
    DWORD rgb);

HBITMAP
LoadBitmaps(
    int id);

BOOL
LookUpFontSubs(
    LPTSTR lpSubFontName,
    LPTSTR lpRealFontName);

BOOL GetUnicodeSampleText(HDC hdc, LPTSTR lpString, int nMaxCount);

#ifdef UNICODE
  BOOL
  ThunkChooseFontA2W(
      PFONTINFO pFI);

  BOOL
  ThunkChooseFontW2A(
      PFONTINFO pFI);

#ifdef MM_DESIGNVECTOR_DEFINED
  VOID
  ThunkEnumLogFontExDvA2W(
      LPENUMLOGFONTEXDVA lpLFA,
      LPENUMLOGFONTEXDVW lpLFW);

  VOID
  ThunkEnumLogFontExDvW2A(
      LPENUMLOGFONTEXDVW lpLFW,
      LPENUMLOGFONTEXDVA lpLFA);
#endif // MM_DESIGNVECTOR_DEFINED

  VOID
  ThunkLogFontA2W(
      LPLOGFONTA lpLFA,
      LPLOGFONTW lpLFW);

  VOID
  ThunkLogFontW2A(
      LPLOGFONTW lpLFW,
      LPLOGFONTA lpLFA);
#endif
