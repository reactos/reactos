/*++

Copyright (c) 1990-1995,  Microsoft Corporation  All rights reserved.

Module Name:

    font.h

Abstract:

    This module contains the header information for the Win32 font dialogs.

Revision History:

--*/



//
//  Constant Declarations.
//

#define CCHCOLORNAMEMAX      16        // max length of color name text
#define CCHCOLORS            16        // max # of pure colors in color combo

#define POINTS_PER_INCH      72
#define FFMASK               0xf0      // pitch and family mask
#define CCHSTDSTRING         12        // max length of sample text string

#define FONTPROP   (LPCTSTR) 0xA000L

#define myatoi atoi


#define CBN_MYEDITUPDATE     (WM_USER + 501)
#define KEY_FONT_SUBS TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes")

#define DEF_POINT_SIZE       10

#define DX_BITMAP            20
#define DY_BITMAP            12




//
//  Typedef Declarations.
//

typedef struct {
    UINT            ApiType;
    LPCHOOSEFONT    pCF;
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
    HDC hDC;
    DWORD dwFlags;
    DWORD nFontType;
    BOOL bFillSize;
    BOOL bPrinterFont;
    LPCHOOSEFONT lpcf;
} ENUM_FONT_DATA, *LPENUM_FONT_DATA;

typedef struct _ITEMDATA {
    PLOGFONT pLogFont;
    DWORD nFontType;
} ITEMDATA, *LPITEMDATA;




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

RECT rcText;
DWORD nLastFontType;
HBITMAP hbmFont = NULL;
HFONT hDlgFont = NULL;

TCHAR szRegular[CCHSTYLE];
TCHAR szBold[CCHSTYLE];
TCHAR szItalic[CCHSTYLE];
TCHAR szBoldItalic[CCHSTYLE];

TCHAR szPtFormat[] = TEXT("%d");

LPCFHOOKPROC glpfnFontHook = 0;




//
//  Function Prototypes.
//

BOOL
ChooseFontX(
    PFONTINFO pFI);

VOID
SetStyleSelection(
    HWND hDlg,
    LPCHOOSEFONT lpcf,
    BOOL bInit);

VOID
HideDlgItem(
    HWND hDlg,
    INT id);

VOID
FixComboHeights(
    HWND hDlg);

BOOL
FormatCharDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam);

void
SelectStyleFromLF(
    HWND hwnd,
    LPLOGFONT lplf);

INT
CBSetTextFromSel(
    HWND hwnd);

INT
CBSetSelFromText(
    HWND hwnd,
    LPTSTR lpszString);

INT
CBGetTextAndData(
    HWND hwnd,
    LPTSTR lpszString,
    INT iSize,
    LPDWORD lpdw);

INT
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
ProcessDlgCtrlCommand(
    HWND hDlg,
    PFONTINFO pFI,
    WPARAM wParam,
    LPARAM lParam);

INT
CmpFontType(
    DWORD ft1,
    DWORD ft2);

INT
FontFamilyEnumProc(
    LPLOGFONT lplf,
    LPTEXTMETRIC lptm,
    DWORD nFontType,
    LPENUM_FONT_DATA lpData);

BOOL
GetFontFamily(
    HWND hDlg,
    HDC hDC,
    DWORD dwEnumCode);

VOID
CBAddSize(
    HWND hwnd,
    INT pts,
    LPCHOOSEFONT lpcf);

INT
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

VOID
FillInMissingStyles(
    HWND hwnd);

VOID
FillScalableSizes(
    HWND hwnd,
    LPCHOOSEFONT lpcf);

INT
FontStyleEnumProc(
    LPLOGFONT lplf,
    LPNEWTEXTMETRIC lptm,
    DWORD nFontType,
    LPENUM_FONT_DATA lpData);

VOID
FreeFonts(
    HWND hwnd);

VOID
InitLF(
    LPLOGFONT lplf);

BOOL
GetFontStylesAndSizes(
    HWND hDlg,
    LPCHOOSEFONT lpcf,
    BOOL bForceSizeFill);

VOID
FillColorCombo(
    HWND hDlg);

VOID
ComputeSampleTextRectangle(
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
    LPCHOOSEFONT lpcf,
    HDC hDC);

BOOL
FillInFont(
    HWND hDlg,
    LPCHOOSEFONT lpcf,
    LPLOGFONT lplf,
    BOOL bSetBits);

VOID
TermFont();

INT
GetPointString(
    LPTSTR buf,
    HDC hDC,
    INT height);

DWORD
FlipColor(
    DWORD rgb);

HBITMAP
LoadBitmaps(
    INT id);

BOOL
LookUpFontSubs(
    LPTSTR lpSubFontName,
    LPTSTR lpRealFontName);


#ifdef UNICODE
  BOOL
  ThunkChooseFontA2W(
      PFONTINFO pFI);

  BOOL
  ThunkChooseFontW2A(
      PFONTINFO pFI);

  VOID
  ThunkLogFontA2W(
      LPLOGFONTA lpLFA,
      LPLOGFONTW lpLFW);

  VOID
  ThunkLogFontW2A(
      LPLOGFONTW lpLFW,
      LPLOGFONTA lpLFA);
#endif

