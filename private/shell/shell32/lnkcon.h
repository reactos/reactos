#pragma warning(disable:4200)   // Zero-sized array in struct

#ifdef WINNT
/*
 * FONT_INFO
 *
 * The distinction between the desired and actual font dimensions obtained
 * is important in the case of TrueType fonts, in which there is no guarantee
 * that what you ask for is what you will get.
 *
 * Note that the correspondence between "Desired" and "Actual" is broken
 * whenever the user changes his display driver, because GDI uses driver
 * parameters to control the font rasterization.
 *
 * The SizeDesired is {0, 0} if the font is a raster font.
 */
typedef struct _FONT_INFO {
    HFONT hFont;
    COORD Size;      // font size obtained
    COORD SizeWant;  // 0;0 if Raster font
    LONG  Weight;
    LPTSTR FaceName;
    BYTE  Family;
    BYTE  tmCharSet;
} FONT_INFO, *PFONT_INFO;

typedef struct tagFACENODE {
     struct tagFACENODE *pNext;
     DWORD  dwFlag;
     TCHAR  atch[];
} FACENODE, *PFACENODE;

//
// TrueType font list
//     This structure shares in windows\inc\wincon.w file
//

// we don't make bold available if BOLD_MARK is in the face name.
#define BOLD_MARK    (L'*')

typedef struct _TT_FONT_LIST {
    SINGLE_LIST_ENTRY List;
    UINT  CodePage;
    BOOL  fDisableBold;
    TCHAR FaceName1[LF_FACESIZE];
    TCHAR FaceName2[LF_FACESIZE];
} TTFONTLIST, *LPTTFONTLIST;

#ifdef MAX_TITLE_LEN
#undef MAX_TITLE_LEN
#endif
#define MAX_TITLE_LEN 256
#define MAXDIMENSTRING     40  // max text in combo box
#define DX_TTBITMAP        20
#define DY_TTBITMAP        12
#define CCH_RASTERFONTS    24
#define CCH_SELECTEDFONT   30

typedef struct {
    IShellLink * psl;
    LPNT_CONSOLE_PROPS lpConsole;
    LPNT_FE_CONSOLE_PROPS lpFEConsole;
    BOOL bConDirty;
    int xScreen;
    int yScreen;
    POINT NonClientSize;
    RECT WindowRect;
    DWORD PreviewFlags;
    PFONT_INFO FontInfo;
    ULONG FontInfoLength;
    ULONG NumberOfFonts;
    BOOL gbEnumerateFaces;
    BOOL gbPointSizeError;
    BOOL gbBold;
    BOOL bFontInit;
    BOOL bColorInit;
    PFACENODE gpFaceNames;
    TCHAR DefaultFaceName[LF_FACESIZE];
    LPTSTR lpFaceName;
#ifndef UNICODE
    CHAR szFaceName[LF_FACESIZE];
#endif
    COORD DefaultFontSize;
    BYTE  DefaultFontFamily;
    ULONG DefaultFontIndex;
    ULONG CurrentFontIndex;
    LONG  FontLong;
    int   Index;
    BYTE  ColorArray[4];
    TCHAR ConsoleTitle[ MAX_TITLE_LEN + 1 ];
    BOOL fChangeCodePage;
    UINT uOEMCP;
    SINGLE_LIST_ENTRY gTTFontList;    // This list contain TTFONTLIST data.
} CONSOLEPROP_DATA, *LPCONSOLEPROP_DATA;  // cpd

// got this from wingdip.h so we don't have to compile it all
#define IS_ANY_DBCS_CHARSET( CharSet )                              \
                   ( ((CharSet) == SHIFTJIS_CHARSET)    ? TRUE :    \
                     ((CharSet) == HANGEUL_CHARSET)     ? TRUE :    \
                     ((CharSet) == CHINESEBIG5_CHARSET) ? TRUE :    \
                     ((CharSet) == GB2312_CHARSET)      ? TRUE : FALSE )

#define TM_IS_TT_FONT(x)     (((x) & TMPF_TRUETYPE) == TMPF_TRUETYPE)
#define IS_BOLD(w)           ((w) >= FW_SEMIBOLD)
#define SIZE_EQUAL(s1, s2)   (((s1).X == (s2).X) && ((s1).Y == (s2).Y))
#define POINTS_PER_INCH 72
#define MIN_PIXEL_HEIGHT 5
#define MAX_PIXEL_HEIGHT 72
#define CONSOLE_REGISTRY_CURRENTPAGE  (L"CurrentPage")


#define MAX_SCRBUF_WIDTH  9999
#define MAX_SCRBUF_HEIGHT 9999

#define INITIAL_FONTS 20
#define FONT_INCREMENT 3
#define CONSOLE_MAX_FONT_NAME_LENGTH 256

#define CM_SETCOLOR       (WM_USER+1)
#define CM_PREVIEW_UPDATE (WM_USER+2)
#define CM_PREVIEW_INIT   (WM_USER+3)
#define CM_COLOR_INIT     (WM_USER+4)


#define PREVIEW_HSCROLL  0x01
#define PREVIEW_VSCROLL  0x02

#define FE_ABANDONFONT 0
#define FE_SKIPFONT    1
#define FE_FONTOK      2


#define EF_NEW         0x0001 // a newly available face
#define EF_OLD         0x0002 // a previously available face
#define EF_ENUMERATED  0x0004 // all sizes have been enumerated
#define EF_OEMFONT     0x0008 // an OEM face
#define EF_TTFONT      0x0010 // a TT face
#define EF_DEFFACE     0x0020 // the default face
#define EF_DBCSFONT    0x0040 // the DBCS font


/* ----- Macros ----- */
/*
 *  High-level macros
 *
 *  These macros handle the SendMessages that go tofrom list boxes
 *  and combo boxes.
 *
 *  The "xxx_lcb" prefix stands for leaves CritSect & "list or combo box".
 *
 *  Basically, we're providing mnemonic names for what would otherwise
 *  look like a whole slew of confusing SendMessage's.
 *
 */
#define lcbRESETCONTENT(hWnd, bLB) \
        SendMessage(hWnd, bLB ? LB_RESETCONTENT : CB_RESETCONTENT, 0, 0L)

#define lcbGETTEXT(hWnd, bLB, w) \
        SendMessage(hWnd, bLB ? LB_GETTEXT : CB_GETLBTEXT, w, 0L)

#define lcbFINDSTRINGEXACT(hWnd, bLB, pwsz) \
        SendMessage(hWnd, bLB ? LB_FINDSTRINGEXACT : CB_FINDSTRINGEXACT, \
                          (DWORD)-1, (LPARAM)pwsz)

#define lcbADDSTRING(hWnd, bLB, pwsz) \
        SendMessage(hWnd, bLB ? LB_ADDSTRING : CB_ADDSTRING, 0, (LPARAM)pwsz)

#define lcbSETITEMDATA(hWnd, bLB, w, nFont) \
        SendMessage(hWnd, bLB ? LB_SETITEMDATA : CB_SETITEMDATA, w, nFont)

#define lcbGETITEMDATA(hWnd, bLB, w) \
        SendMessage(hWnd, bLB ? LB_GETITEMDATA : CB_GETITEMDATA, w, 0L)

#define lcbGETCOUNT(hWnd, bLB) \
        SendMessage(hWnd, bLB ? LB_GETCOUNT : CB_GETCOUNT, 0, 0L)

#define lcbGETCURSEL(hWnd, bLB) \
        SendMessage(hWnd, bLB ? LB_GETCURSEL : CB_GETCURSEL, 0, 0L)

#define lcbSETCURSEL(hWnd, bLB, w) \
        SendMessage(hWnd, bLB ? LB_SETCURSEL : CB_SETCURSEL, w, 0L)

#define NELEM(array) (sizeof(array)/sizeof(array[0]))
#define AttrToRGB(Attr) (pcpd->lpConsole->ColorTable[(Attr) & 0x0F])
#define ScreenTextColor(pcpd) \
            (AttrToRGB(LOBYTE(pcpd->lpConsole->wFillAttribute) & 0x0F))
#define ScreenBkColor(pcpd) \
            (AttrToRGB(LOBYTE(pcpd->lpConsole->wFillAttribute >> 4)))
#define PopupTextColor(pcpd) \
            (AttrToRGB(LOBYTE(pcpd->lpConsole->wPopupFillAttribute) & 0x0F))
#define PopupBkColor(pcpd) \
            (AttrToRGB(LOBYTE(pcpd->lpConsole->wPopupFillAttribute >> 4)))


//
// Function prototypes
//

VOID
InitializeFonts( LPCONSOLEPROP_DATA pcpd );

VOID
DestroyFonts( LPCONSOLEPROP_DATA pcpd );

NTSTATUS
EnumerateFonts( LPCONSOLEPROP_DATA pcpd, DWORD Flags);

int
FindCreateFont(
    LPCONSOLEPROP_DATA pcpd,
    DWORD Family,
    LPTSTR ptszFace,
    COORD Size,
    LONG Weight);

BOOL
DoFontEnum(
    LPCONSOLEPROP_DATA pcpd,
    HDC hDC,
    LPTSTR ptszFace,
    PSHORT pTTPoints,
    UINT nTTPoints);


VOID
GetTitleFromLinkName(
    LPTSTR szLinkName,
    LPTSTR szTitle
    );

VOID
SetRegistryValues(
    LPCONSOLEPROP_DATA pcpd
    );

VOID
GetRegistryValues(
    LPCONSOLEPROP_DATA pcpd
    );

void
InitRegistryValues(
    LPCONSOLEPROP_DATA pcpd
    );

VOID
GetRegistryValues(
    LPCONSOLEPROP_DATA pcpd
    );


// proto-types for FarEast console
//
VOID
SetFERegistryValues(
    LPCONSOLEPROP_DATA pcpd
    );

VOID
GetFERegistryValues(
    LPCONSOLEPROP_DATA pcpd
    );

void
InitFERegistryValues(
    LPCONSOLEPROP_DATA pcpd
    );


NTSTATUS
MakeAltRasterFont(
    LPCONSOLEPROP_DATA pcpd,
    UINT CodePage,
    COORD *AltFontSize,
    BYTE  *AltFontFamily,
    ULONG *AltFontIndex,
    LPTSTR AltFaceName
    );

NTSTATUS
InitializeDbcsMisc(
    LPCONSOLEPROP_DATA pcpd
    );

BYTE
CodePageToCharSet(
    UINT CodePage
    );

LPTTFONTLIST
SearchTTFont(
    LPCONSOLEPROP_DATA pcpd,
    LPTSTR ptszFace,
    BOOL fCodePage,
    UINT CodePage
    );

BOOL
IsAvailableTTFont(
    LPCONSOLEPROP_DATA pcpd,
    LPTSTR ptszFace
    );

BOOL
IsAvailableTTFontCP(
    LPCONSOLEPROP_DATA pcpd,
    LPTSTR pwszFace,
    UINT CodePage
    );

BOOL
IsDisableBoldTTFont(
    LPCONSOLEPROP_DATA pcpd,
    LPTSTR ptszFace
    );

LPTSTR
GetAltFaceName(
    LPCONSOLEPROP_DATA pcpd,
    LPTSTR ptszFace
    );

NTSTATUS
DestroyDbcsMisc(
    LPCONSOLEPROP_DATA pcpd
    );


int
LanguageListCreate(
    HWND hDlg,
    UINT CodePage
    );
#endif // WINNT

typedef struct {
    // input params
    IShellLink *psl;
    // local state variables
    HWND hDlg;
    BOOL bIsFile;    // don't make this a bitfied because we don't keep it around
    BOOL bIsDirty;   // so the codesize savings is better
    int iIconIndex;
#ifdef WINNT
    HANDLE hThread;
    BOOL bCheckRunInSep;
    HANDLE hCheckNow;
#endif
    TCHAR szFile[MAX_PATH];
    TCHAR szIconPath[MAX_PATH];
} LINKPROP_DATA, *LPLINKPROP_DATA;      // lpd


typedef struct {
    LINKPROP_DATA    lpd;
#ifdef WINNT
    CONSOLEPROP_DATA cpd;
#endif
} LINKDATA, *LPLINKDATA;

HRESULT _SaveLink(LPLINKDATA pld);

#ifdef WINNT
VOID LinkConsolePagesSave(LPLINKDATA pld);
void AddLinkConsolePages(LPLINKDATA pld, IShellLink * psl, LPCTSTR pszFile, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
#endif
