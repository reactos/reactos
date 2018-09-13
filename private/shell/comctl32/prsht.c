#include "ctlspriv.h"
#include "help.h" // Help IDs
#include "prshti.h"

#include "dlgcvt.h"

#ifdef WX86
#include <wx86ofl.h>
#endif

#ifdef UNIX
#include <mainwin.h>
#endif

#define FLAG_CHANGED    0x0001
#define DEFAULTHEADERHEIGHT    58   // in pixels
#define DEFAULTTEXTDIVIDERGAP  5
#define DEFAULTCTRLWIDTH       501   // page list window in new wizard style
#define DEFAULTCTRLHEIGHT      253   // page list window in new wizard style
#define TITLEX                 22
#define TITLEY                 10
#define SUBTITLEX              44
#define SUBTITLEY              25

// fixed sizes for the bitmap painted in the header section
#define HEADERBITMAP_Y            5
#define HEADERBITMAP_WIDTH        49
#define HEADERBITMAP_CXBACK       (5 + HEADERBITMAP_WIDTH)
#define HEADERBITMAP_HEIGHT       49                
#define HEADERSUBTITLE_WRAPOFFSET 10

// Fixed sizes for the watermark bitmap (Wizard97IE5 style)
#define BITMAP_WIDTH  164
#define BITMAP_HEIGHT 312

#define DRAWTEXT_WIZARD97FLAGS (DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL)

LPVOID WINAPI MapSLFix(HANDLE);
VOID WINAPI UnMapSLFixArray(int, HANDLE *);

LRESULT CALLBACK WizardWndProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam, UINT_PTR uID, ULONG_PTR dwRefData);

#if  !defined(WIN32)
#ifdef FE_IME
typedef void *PVOID;
DWORD WINAPI GetCurrentThreadID(VOID);
DWORD WINAPI GetCurrentProcessID(VOID);
PVOID WINAPI ImmFindThreadLink(DWORD dwThreadID);
BOOL WINAPI ImmCreateThreadLink(DWORD dwPid, DWORD dwTid);
#endif
#endif

void    NEAR PASCAL ResetWizButtons(LPPROPDATA ppd);

typedef struct  // tie
{
    TC_ITEMHEADER   tci;
    HWND            hwndPage;
    UINT            state;
} TC_ITEMEXTRA;

#define CB_ITEMEXTRA (sizeof(TC_ITEMEXTRA) - sizeof(TC_ITEMHEADER))
#define IS_WIZARDPSH(psh) ((psh).dwFlags & (PSH_WIZARD | PSH_WIZARD97 | PSH_WIZARD_LITE))
#define IS_WIZARD(ppd) IS_WIZARDPSH(ppd->psh)

void NEAR PASCAL PageChange(LPPROPDATA ppd, int iAutoAdj);
void NEAR PASCAL RemovePropPageData(LPPROPDATA ppd, int nPage);
HRESULT GetPageLanguage(PISP pisp, WORD *pwLang);
UINT GetDefaultCharsetFromLang(LANGID wLang);
#ifdef WINNT
LANGID NT5_GetUserDefaultUILanguage(void);
#endif

//
// IMPORTANT:  The IDHELP ID should always be LAST since we just subtract
// 1 from the number of IDs if no help in the page.
// IDD_APPLYNOW should always be the FIRST ID for standard IDs since it
// is sometimes not displayed and we'll start with index 1.
//
const static int IDs[] = {IDOK, IDCANCEL, IDD_APPLYNOW, IDHELP};
const static int WizIDs[] = {IDD_BACK, IDD_NEXT, IDD_FINISH, IDCANCEL, IDHELP};
const static WORD wIgnoreIDs[] = {IDD_PAGELIST, IDD_DIVIDER, IDD_TOPDIVIDER};

// Prsht_PrepareTemplate action matrix. Please do not change without contacting [msadek]...

const PSPT_ACTION g_PSPT_Action [PSPT_TYPE_MAX][PSPT_OS_MAX][PSPT_OVERRIDE_MAX]={
    PSPT_ACTION_NOACTION,     // PSPT_TYPE_MIRRORED, PSPT_OS_WIN95_BIDI, PSPT_OVERRIDE_NOOVERRIDE
    PSPT_ACTION_NOACTION,     // PSPT_TYPE_MIRRORED, PSPT_OS_WIN95_BIDI, PSPT_OVERRIDE_USEPAGELANG
    PSPT_ACTION_WIN9XCOMPAT,  // PSPT_TYPE_MIRRORED, PSPT_OS_WIN98_BIDI, PSPT_OVERRIDE_NOOVERRIDE
    PSPT_ACTION_WIN9XCOMPAT,  // PSPT_TYPE_MIRRORED, PSPT_OS_WIN98_BIDI, PSPT_OVERRIDE_USEPAGELANG
    PSPT_ACTION_FLIP,         // PSPT_TYPE_MIRRORED, PSPT_OS_WINNT4_ENA, PSPT_OVERRIDE_NOOVERRIDE
    PSPT_ACTION_FLIP,         // PSPT_TYPE_MIRRORED, PSPT_OS_WINNT4_ENA, PSPT_OVERRIDE_USEPAGELANG
    PSPT_ACTION_NOACTION,     // PSPT_TYPE_MIRRORED, PSPT_OS_WINNT5,     PSPT_OVERRIDE_NOOVERRIDE
    PSPT_ACTION_NOACTION,     // PSPT_TYPE_MIRRORED, PSPT_OS_WINNT5,     PSPT_OVERRIDE_USEPAGELANG
    PSPT_ACTION_NOACTION,     // PSPT_TYPE_MIRRORED, PSPT_OS_OTHER,      PSPT_OVERRIDE_NOOVERRIDE
    PSPT_ACTION_NOMIRRORING,  // PSPT_TYPE_MIRRORED, PSPT_OS_OTHER,      PSPT_OVERRIDE_USEPAGELANG
    PSPT_ACTION_NOACTION,     // PSPT_TYPE_ENABLED,  PSPT_OS_WIN95_BIDI, PSPT_OVERRIDE_NOOVERRIDE
    PSPT_ACTION_NOACTION,     // PSPT_TYPE_ENABLED,  PSPT_OS_WIN95_BIDI, PSPT_OVERRIDE_USEPAGELANG
    PSPT_ACTION_FLIP,         // PSPT_TYPE_ENABLED,  PSPT_OS_WIN98_BIDI, PSPT_OVERRIDE_NOOVERRIDE
    PSPT_ACTION_FLIP,         // PSPT_TYPE_ENABLED,  PSPT_OS_WIN98_BIDI, PSPT_OVERRIDE_USEPAGELANG
    PSPT_ACTION_FLIP,         // PSPT_TYPE_ENABLED,  PSPT_OS_WINNT4_ENA, PSPT_OVERRIDE_NOOVERRIDE
    PSPT_ACTION_FLIP,         // PSPT_TYPE_ENABLED,  PSPT_OS_WINNT4_ENA, PSPT_OVERRIDE_USEPAGELANG
    PSPT_ACTION_FLIP,         // PSPT_TYPE_ENABLED,  PSPT_OS_WINNT5,     PSPT_OVERRIDE_NOOVERRIDE
    PSPT_ACTION_FLIP,         // PSPT_TYPE_ENABLED,  PSPT_OS_WINNT5,     PSPT_OVERRIDE_USEPAGELANG
    PSPT_ACTION_NOACTION,     // PSPT_TYPE_ENABLED,  PSPT_OS_OTHER,      PSPT_OVERRIDE_NOOVERRIDE
    PSPT_ACTION_NOMIRRORING,  // PSPT_TYPE_ENABLED,  PSPT_OS_OTHER,      PSPT_OVERRIDE_USEPAGELANG
    PSPT_ACTION_NOACTION,     // PSPT_TYPE_ENGLISH,  PSPT_OS_WIN95_BIDI, PSPT_OVERRIDE_NOOVERRIDE
    PSPT_ACTION_NOACTION,     // PSPT_TYPE_ENGLISH,  PSPT_OS_WIN95_BIDI, PSPT_OVERRIDE_USEPAGELANG
    PSPT_ACTION_LOADENGLISH,  // PSPT_TYPE_ENGLISH,  PSPT_OS_WIN98_BIDI, PSPT_OVERRIDE_NOOVERRIDE
    PSPT_ACTION_FLIP,         // PSPT_TYPE_ENGLISH,  PSPT_OS_WIN98_BIDI, PSPT_OVERRIDE_USEPAGELANG
    PSPT_ACTION_FLIP,         // PSPT_TYPE_ENGLISH,  PSPT_OS_WINNT4_ENA, PSPT_OVERRIDE_NOOVERRIDE
    PSPT_ACTION_FLIP,         // PSPT_TYPE_ENGLISH,  PSPT_OS_WINNT4_ENA, PSPT_OVERRIDE_USEPAGELANG
    PSPT_ACTION_LOADENGLISH,  // PSPT_TYPE_ENGLISH,  PSPT_OS_WINNT5,     PSPT_OVERRIDE_NOOVERRIDE
    PSPT_ACTION_FLIP,         // PSPT_TYPE_ENGLISH,  PSPT_OS_WINNT5,     PSPT_OVERRIDE_USEPAGELANG
    PSPT_ACTION_NOACTION,     // PSPT_TYPE_ENGLISH,  PSPT_OS_OTHER,      PSPT_OVERRIDE_NOOVERRIDE
    PSPT_ACTION_NOMIRRORING,  // PSPT_TYPE_ENGLISH,  PSPT_OS_OTHER,      PSPT_OVERRIDE_USEPAGELANG
    };

void NEAR PASCAL _SetTitle(HWND hDlg, LPPROPDATA ppd)
{
    TCHAR szFormat[50];
    TCHAR szTitle[128];
    TCHAR szTemp[128 + 50];
    LPCTSTR pCaption = ppd->psh.pszCaption;

    if (IS_INTRESOURCE(pCaption)) {
        LoadString(ppd->psh.hInstance, (UINT)LOWORD(pCaption), szTitle, ARRAYSIZE(szTitle));
        pCaption = (LPCTSTR)szTitle;
    }

    if (ppd->psh.dwFlags & PSH_PROPTITLE) {
        if (*pCaption == 0)
        {
            // Hey, no title, we need a different resource for localization
            LocalizedLoadString(IDS_PROPERTIES, szTemp, ARRAYSIZE(szTemp));
            pCaption = szTemp;
        }
        else
        {
            LocalizedLoadString(IDS_PROPERTIESFOR, szFormat, ARRAYSIZE(szFormat));
            if ((lstrlen(pCaption) + 1 + lstrlen(szFormat) + 1) < ARRAYSIZE(szTemp)) {
                wsprintf(szTemp, szFormat, pCaption);
                pCaption = szTemp;
            }
        }
    }
#ifdef WINDOWS_ME
    if(ppd->psh.dwFlags & PSH_RTLREADING) {
        SetWindowLong(hDlg, GWL_EXSTYLE, GetWindowLong(hDlg, GWL_EXSTYLE) | WS_EX_RTLREADING);
        }
#endif // WINDOWS_ME
    SetWindowText(hDlg, pCaption);
}

BOOL _SetHeaderFonts(HWND hDlg, LPPROPDATA ppd)
{
    HFONT   hFont;
    LOGFONT LogFont;

    GetObject(GetWindowFont(hDlg), sizeof(LogFont), &LogFont);

    LogFont.lfWeight = FW_BOLD;
    if ((hFont = CreateFontIndirect(&LogFont)) == NULL)
    {
        ppd->hFontBold = NULL;
        return FALSE;
    }
    ppd->hFontBold = hFont;
    // Save the font as a window prop so we can delete it later
    return TRUE;
}

int _WriteHeaderTitle(LPPROPDATA ppd, HDC hdc, LPRECT prc, LPCTSTR pszTitle, BOOL bTitle, DWORD dwDrawFlags)
{
    LPCTSTR pszOut;
    int cch;
    int cx, cy;
    TCHAR szTitle[MAX_PATH*4];
    HFONT hFontOld = NULL;
    HFONT hFont;
    int yDrawHeight = 0;

    if (IS_INTRESOURCE(pszTitle))
    {
        LoadString(GETPPSP(ppd, ppd->nCurItem)->hInstance, (UINT)LOWORD(pszTitle), szTitle, ARRAYSIZE(szTitle));
        pszOut = szTitle;
    }
    else
        pszOut = pszTitle;

    cch = lstrlen(pszOut);

    if (bTitle && ppd->hFontBold)
        hFont = ppd->hFontBold;
    else
        hFont = GetWindowFont(ppd->hDlg);

    hFontOld = SelectObject(hdc, hFont);

    if (bTitle)
    {
        cx = TITLEX;
        cy = TITLEY;
        ExtTextOut(hdc, cx, cy, 0, prc, pszOut, cch, NULL);
    }
    else
    {
        RECT rcWrap;
        CopyRect(&rcWrap, prc);

        rcWrap.left = SUBTITLEX;
        rcWrap.top = ppd->ySubTitle;
        yDrawHeight = DrawText(hdc, pszOut, cch, &rcWrap, dwDrawFlags);
    }

    if (hFontOld)
        SelectObject(hdc, hFontOld);

    return yDrawHeight;
}

// In Wizard97 only:
// The subtitles user passed in could be larger than the two line spaces we give
// them, especially in localization cases. So here we go through all subtitles and
// compute the max space they need and set the header height so that no text is clipped
int _ComputeHeaderHeight(LPPROPDATA ppd, int dxMax)
{
    int dyHeaderHeight;
    int dyTextDividerGap;
    HDC hdc;
    dyHeaderHeight = DEFAULTHEADERHEIGHT;
    hdc = GetDC(ppd->hDlg);

    // First, let's get the correct text height and spacing, this can be used
    // as the title height and the between-lastline-and-divider spacing.
    {
        HFONT hFont, hFontOld;
        TEXTMETRIC tm;
        if (ppd->hFontBold)
            hFont = ppd->hFontBold;
        else
            hFont = GetWindowFont(ppd->hDlg);

        hFontOld = SelectObject(hdc, hFont);
        if (GetTextMetrics(hdc, &tm))
        {
            dyTextDividerGap = tm.tmExternalLeading;
            ppd->ySubTitle = max ((tm.tmHeight + tm.tmExternalLeading + TITLEY), SUBTITLEY);
        }
        else
        {
            dyTextDividerGap = DEFAULTTEXTDIVIDERGAP;
            ppd->ySubTitle = SUBTITLEY;
        }

        if (hFontOld)
            SelectObject(hdc, hFontOld);
    }

    // Second, get the subtitle text block height
    // should make into a function if shared
    {
        RECT rcWrap;
        UINT uPages;

        //
        //  WIZARD97IE5 subtracts out the space used by the header bitmap.
        //  WIZARD97IE4 uses the full width since the header bitmap
        //  in IE4 is a watermark and occupies no space.
        //
        if (ppd->psh.dwFlags & PSH_WIZARD97IE4)
            rcWrap.right = dxMax;
        else
            rcWrap.right = dxMax - HEADERBITMAP_CXBACK - HEADERSUBTITLE_WRAPOFFSET;
        for (uPages = 0; uPages < ppd->psh.nPages; uPages++)
        {
            PROPSHEETPAGE *ppsp = GETPPSP(ppd, uPages);
            if (!(ppsp->dwFlags & PSP_HIDEHEADER) &&
                 (ppsp->dwFlags & PSP_USEHEADERSUBTITLE))
            {
                int iSubHeaderHeight = _WriteHeaderTitle(ppd, hdc, &rcWrap, ppsp->pszHeaderSubTitle,
                    FALSE, DT_CALCRECT | DRAWTEXT_WIZARD97FLAGS);
                if ((iSubHeaderHeight + ppd->ySubTitle) > dyHeaderHeight)
                    dyHeaderHeight = iSubHeaderHeight + ppd->ySubTitle;
            }
        }
    }

    // If the header height has been recomputed, set the correct gap between
    // the text and the divider.
    if (dyHeaderHeight != DEFAULTHEADERHEIGHT)
    {
        ASSERT(dyHeaderHeight > DEFAULTHEADERHEIGHT);
        dyHeaderHeight += dyTextDividerGap;
    }

    ReleaseDC(ppd->hDlg, hdc);
    return dyHeaderHeight;
}

void MoveAllButtons(HWND hDlg, const int *pids, int idLast, int dx, int dy)
{
    do {
        HWND hCtrl;
        RECT rcCtrl;

        int iCtrl = *pids;
        hCtrl = GetDlgItem(hDlg, iCtrl);
        GetWindowRect(hCtrl, &rcCtrl);

        //
        // If the dialog wizard window is mirrored, then rcl.right
        // in terms of screen coord is the near edge (lead). [samera]
        //
        if (IS_WINDOW_RTL_MIRRORED(hDlg))
            rcCtrl.left = rcCtrl.right;

        ScreenToClient(hDlg, (LPPOINT)&rcCtrl);
        SetWindowPos(hCtrl, NULL, rcCtrl.left + dx,
                     rcCtrl.top + dy, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
    } while(*(pids++) != idLast);
}

void NEAR PASCAL RemoveButton(HWND hDlg, int idRemove, const int *pids)
{
    int idPrev = 0;
    HWND hRemove;
    HWND hPrev;
    RECT rcRemove, rcPrev;
    int iWidth = 0;
    const int *pidRemove;

    // get the previous id
    for (pidRemove = pids; *pidRemove != idRemove; pidRemove++)
        idPrev = *pidRemove;


    if (idPrev) {
        hRemove = GetDlgItem(hDlg, idRemove);
        hPrev = GetDlgItem(hDlg, idPrev);
        GetWindowRect(hRemove, &rcRemove);
        GetWindowRect(hPrev, &rcPrev);

        //
        // If the dialog window is mirrored, then the prev button
        // will be ahead (to the right) of the button-to-be-removed.
        // As a result, the subtraction will be definitely negative,
        // so let's convert it to be positive. [samera]
        //
        if (IS_WINDOW_RTL_MIRRORED(hDlg))
            iWidth = rcPrev.right - rcRemove.right;
        else
            iWidth = rcRemove.right - rcPrev.right;
    }

    MoveAllButtons(hDlg, pids, idRemove, iWidth, 0);
    ShowWindow(hRemove, SW_HIDE);

    // Cannot disable the window; see Prsht_ButtonSubclassProc for explanation.
    // WRONG - EnableWindow(hRemove, FALSE);
}

typedef struct LOGPALETTE256 {
    WORD    palVersion;
    WORD    palNumEntries;
    union {
        PALETTEENTRY rgpal[256];
        RGBQUAD rgq[256];
    } u;
} LOGPALETTE256;

HPALETTE PaletteFromBmp(HBITMAP hbm)
{
    LOGPALETTE256 pal;
    int i,n;
    HDC hdc;
    HPALETTE hpl;

    hdc = CreateCompatibleDC(NULL);
    SelectObject(hdc, hbm);
    n = GetDIBColorTable(hdc, 0, 256, pal.u.rgq);

    if (n)                          // DIB section with color table
    {
        // Palettes suck.  GetDIBColorTable returns RGBQUADs, whereas
        // LOGPALETTE wants PALETTEENTRYss, and the two are reverse-endian
        // of each other.
        for (i= 0 ; i < n; i++)
        {
            PALETTEENTRY pe;
            pe.peRed = pal.u.rgq[i].rgbRed;
            pe.peGreen = pal.u.rgq[i].rgbGreen;
            pe.peBlue = pal.u.rgq[i].rgbBlue;
            pe.peFlags = 0;
            pal.u.rgpal[i] = pe;
        }

        pal.palVersion = 0x0300;
        pal.palNumEntries = (WORD)n;

        hpl = CreatePalette((LPLOGPALETTE)&pal);
    }
    else                            // Not a DIB section or no color table
    {
        hpl = CreateHalftonePalette(hdc);
    }

    DeleteDC(hdc);
    return hpl;
}

// -------------- stolen from user code -------------------------------------
//
//  GetCharDimensions(hDC, psiz)
//
//  This function loads the Textmetrics of the font currently selected into
//  the given hDC and saves the height and Average char width of the font
//  (NOTE: the
//  AveCharWidth value returned by the text metrics call is wrong for
//  proportional fonts -- so, we compute them).
//
// -------------- stolen from user code --------------------------------------
TCHAR AveCharWidthData[52+1] = TEXT("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
void GetCharDimensions(HDC hDC, SIZE *psiz)
{
    TEXTMETRIC  tm;

    // Store the System Font metrics info.
    GetTextMetrics(hDC, &tm);

    if (!(tm.tmPitchAndFamily & TMPF_FIXED_PITCH)) // the name is opposite:)
        psiz->cx = tm.tmAveCharWidth;
    else
    {
        // Change from tmAveCharWidth.  We will calculate a true average as
        // opposed to the one returned by tmAveCharWidth. This works better
        // when dealing with proportional spaced fonts. -- ROUND UP
        if (GetTextExtentPoint32(hDC, AveCharWidthData, 52, psiz) == TRUE)
        {
            psiz->cx = ((psiz->cx / 26) + 1) / 2;
        }
        else
            psiz->cx = tm.tmAveCharWidth;
    }

    psiz->cy = tm.tmHeight;
}

//
//  It is a feature that USER considers keyboard accelerators live even if
//  the control is hidden.  This lets you put a hidden static in front of
//  a custom control to get an accelerator attached to the custom control.
//
//  Unfortunately, it means that the &F accelerator for "Finish" activates
//  the Finish button even when the Finish button is hidden.  The normal
//  workaround for this is to disable the control, but that doesn't work
//  because Microsoft PhotoDraw runs around and secretly hides and shows
//  buttons without going through PSM_SETWIZBUTTONS, so they end up showing
//  a disabled window and their wizard stops working.
//
//  So instead, we subclass the buttons and customize their WM_GETDLGCODE
//  so that when the control is hidden, they disable their accelerators.
//
LRESULT CALLBACK Prsht_ButtonSubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp, UINT_PTR uID, ULONG_PTR dwRefData)
{
    LRESULT lres;


    switch (wm)
    {

    case WM_GETDLGCODE:
        lres = DefSubclassProc(hwnd, wm, wp, lp);
        if (!IsWindowVisible(hwnd))
        {
            // To remove yourself from the mnemonic search, you have to
            // return DLGC_WANTCHAR if you are give a NULL LPMSG pointer.
            // Normally, the dialog manager sends a real LPMSG containing
            // the message that just got received, but when it's poking
            // around looking for accelerators, it doesn't give you a
            // message at all.  It is in that case that you want to
            // say, "Hey, I will process the (nonexistent) message".
            // This tricks USER into thinking you're an edit control, so
            // it won't scan your for mnemonics.
            if ((LPMSG)lp == NULL)
                lres |= DLGC_WANTCHARS;

        }
        break;

    case WM_NCDESTROY:
        // Clean up subclass
        RemoveWindowSubclass(hwnd, Prsht_ButtonSubclassProc, 0);
        lres = DefSubclassProc(hwnd, wm, wp, lp);
        break;

    default:
        lres = DefSubclassProc(hwnd, wm, wp, lp);
        break;
    }

    return lres;
}

void Prsht_SubclassButton(HWND hDlg, UINT idd)
{
    SetWindowSubclass(GetDlgItem(hDlg, idd), Prsht_ButtonSubclassProc, 0, 0);
}

//
// Because StrCmpIW(lstrcmpiW) converts unicode string to ansi depends on user locale
// on Win9x platform, we can't compare two different locale's unicode string properly.
// This is why we use small private helper function to compare limited DBCS font facename
//
BOOL CompareFontFaceW(LPCWSTR lpwz1, LPCWSTR lpwz2, BOOL fBitCmp)
{
#ifndef WINNT
    BOOL fRet = TRUE;   // Return FALSE if strings are same, otherwise return TRUE

    if (fBitCmp)
    {
        int iLen1, iLen2;

        iLen1 = lstrlenW(lpwz1);
        iLen2 = lstrlenW(lpwz2);
        if (iLen1 == iLen2)
        {
            int i;

            for (i = 0; i < iLen1; i++)
            {
                if (lpwz1[i] != lpwz2[i])
                    break;
            }

            if (i >= iLen1)
                fRet = FALSE;
        }
    }
    else
        fRet = lstrcmpiW(lpwz1, lpwz2);

    return fRet;
#else
    return lstrcmpiW(lpwz1, lpwz2);
#endif
}

// 
// GetPageFontMetrics
//
// synopsis: 
// 
// Get the real font metrics from PAGEFONTDATA. Used in InitPropSheetDlg() to
// calculate the physical page size based on the font specified in page templates
//
// fML is set if we are in here because of an ML scenario, in which case the
// font names need to be mapped.
//

BOOL GetPageFontMetrics(LPPROPDATA ppd, PPAGEFONTDATA ppfd, BOOL fML)
{
    LOGFONT    lf = {0};
    HFONT      hFont;
    HRESULT    fRc = FALSE;
    HDC        hdc;
    
    if (ppfd && (ppfd->PointSize > 0) && ppfd->szFace[0])
    {

        // font name mapping
        // should be done only for the platform less than NT5
        // NT5 is supposed to work with native typeface on any system locale.
        //
        if (!staticIsOS(OS_NT5) && fML)
        {
            // replace native font face name to single byte name for non-native platform
            typedef struct tagFontFace
            {
                BOOL fBitCmp;
                LPCWSTR lpEnglish;
                LPCWSTR lpNative;
            } FONTFACE, *LPFONTFACE;
    
            const static FONTFACE s_FontTbl[] = 
            {
                {   FALSE, L"MS Gothic", L"MS UI Gothic"                                   },
                {   TRUE,  L"MS Gothic", L"\xff2d\xff33 \xff30\x30b4\x30b7\x30c3\x30af"    },
                {   TRUE,  L"GulimChe",  L"\xad74\xb9bc"                                   },
                {   TRUE,  L"MS Song",   L"\x5b8b\x4f53"                                   },
                {   TRUE,  L"MingLiU",   L"\x65b0\x7d30\x660e\x9ad4"                       }
            };

            int i;

            for (i = 0; i < ARRAYSIZE(s_FontTbl); i++)
            {
                if (!CompareFontFaceW(ppfd->szFace, s_FontTbl[i].lpNative, s_FontTbl[i].fBitCmp))
                {
                    lstrcpynW(lf.lfFaceName, s_FontTbl[i].lpEnglish, ARRAYSIZE(lf.lfFaceName));
                    break;
                }
            }
            if (i >= ARRAYSIZE(s_FontTbl))
                lstrcpynW(lf.lfFaceName, ppfd->szFace, ARRAYSIZE(lf.lfFaceName));
        }
        else
            lstrcpynW(lf.lfFaceName, ppfd->szFace, ARRAYSIZE(lf.lfFaceName));

        // Try to use the cache
        if (ppfd->iCharset  == ppd->pfdCache.iCharset &&
            ppfd->bItalic   == ppd->pfdCache.bItalic &&
            ppfd->PointSize == ppd->pfdCache.PointSize &&
            lstrcmpiW(ppfd->szFace, ppd->pfdCache.szFace) == 0) {
            fRc = TRUE;
        } else {
            if (hdc = GetDC(ppd->hDlg))
            {
                lf.lfHeight = -MulDiv(ppfd->PointSize, GetDeviceCaps(hdc,LOGPIXELSY), 72);
                lf.lfCharSet = (BYTE)ppfd->iCharset;
                lf.lfItalic  = (BYTE)ppfd->bItalic;
                lf.lfWeight = FW_NORMAL;

                hFont = CreateFontIndirectW(&lf);
                if (hFont)
                {
                    HFONT hFontOld = SelectObject(hdc, hFont);

                    GetCharDimensions(hdc, &ppd->sizCache);
                    if (hFontOld)
                        SelectObject(hdc, hFontOld);

                    DeleteObject(hFont);

                    // Save these font metrics into the cache
                    ppd->pfdCache = *ppfd;
                    fRc = TRUE;
                }
                ReleaseDC(ppd->hDlg, hdc);

            }
        }
    }
    return fRc;
}

//
//  The "ideal page size" of a property sheet is the maximum size of all
//  pages.
//
//  GIPS_SKIPINTERIOR97HEIGHT and GIPS_SKIPEXTERIOR97HEIGHT selective
//  exclude Wiz97 pages from the height computation.  They are important
//  because interior pages are shorter than exterior pages by
//  ppd->cyHeaderHeight.
//

#define GIPS_SKIPINTERIOR97HEIGHT 1
#define GIPS_SKIPEXTERIOR97HEIGHT 2

void Prsht_GetIdealPageSize(LPPROPDATA ppd, PSIZE psiz, UINT flags)
{
    UINT uPages;

    *psiz = ppd->sizMin;

    for (uPages = 0; uPages < ppd->psh.nPages; uPages++)
    {
        PISP pisp = GETPISP(ppd, uPages);
        int cy = pisp->_pfx.siz.cy;

        if (ppd->psh.dwFlags & PSH_WIZARD97)
        {
            if (pisp->_psp.dwFlags & PSP_HIDEHEADER)
            {
                if (flags & GIPS_SKIPEXTERIOR97HEIGHT) goto skip;
            }
            else
            {
                if (flags & GIPS_SKIPINTERIOR97HEIGHT) goto skip;
            }
        }

        if (psiz->cy < cy)
            psiz->cy = cy;

    skip:;
        if (psiz->cx < pisp->_pfx.siz.cx)
            psiz->cx = pisp->_pfx.siz.cx;
    }

}

#define IsMSShellDlgMapped(langid) (PRIMARYLANGID(langid) == LANG_JAPANESE)

//
//  Given a page, decide what size it wants to be and save it in the
//  pisp->_pfx.siz.
//
void Prsht_ComputeIdealPageSize(LPPROPDATA ppd, PISP pisp, PAGEINFOEX *ppi)
{
    BOOL fUsePageFont;

    // pressume page and frame dialog are in same character set
    LANGID wPageLang = ppd->wFrameLang;
    int    iPageCharset = DEFAULT_CHARSET;

    if (SUCCEEDED(GetPageLanguage(pisp, &wPageLang)))
    {
        // GetPageLanguage fails if page is marked PSP_DLGINDIRECT;
        // we'll try to recover from that later.  For now,
        // we leave pagelang to DEFAULT_CHARSET and see if we can take
        // the charset info from template EX.
        //
        // if PSH_USEPAGELANG is specified, we can assume that
        // page charset == frame charset and no need for ML adjustment
        // *except for* the case of NT Japanese version that replaces
        // frame's MS Shell Dlg to their native font. We handle this
        // exception later where we set up fUsePageFont; 
        //
        if (!(ppd->psh.dwFlags & PSH_USEPAGELANG)
            && wPageLang != ppd->wFrameLang)
        {
            iPageCharset  = GetDefaultCharsetFromLang(wPageLang);
        }
        else
            iPageCharset  = ppd->iFrameCharset;
    }

    // Use the font in the page if any of these conditions are met:
    //
    // A) It's a SHELLFONT page.  Do this even if the font is not
    //    "MS Shell Dlg 2".  This gives apps a way to specify that
    //    their custom-font page should be measured against the
    //    font in the page rather than in the frame font.
    //
    // B) ML scenario - complicated original comment below...
    //
    //  1) we've detected lang in the caller's resource and
    //  it's different from the frame dialog
    //  2) the caller's page doesn't have lang info or we've
    //  failed to get it (iPageCharset == DEFAULT_CHARSET),
    // then we find the page is described with DLGTEMPLATEEX
    // and has meaningful charset specified (!= defaultcharset)
    // *and* the charset is different from frame's
    //  3) the exception for NT Japanese platform that maps
    //     MS Shell Dlg to their native font. For US Apps to
    //     work on these platforms they typically specify 
    //     PSH_USEPAGELANG to get English buttons on frame
    //     but they still need to get the frame sized based on
    //     page font
    //
    // Otherwise, IE4 compat **requires** that we use the frame font.
    // ISVs have hacked around this historical bug by having large
    // dialog templates with extra space in them.
    //
    fUsePageFont =
        /* --- A) It's a SHELLFONT page --- */
        IsPageInfoSHELLFONT(ppi) ||
        /* --- B) ML scenario --- */
#ifdef WINNT
        ((ppd->psh.dwFlags & PSH_USEPAGELANG) 
        && IsMSShellDlgMapped(NT5_GetUserDefaultUILanguage())) ||
#endif
        (ppd->iFrameCharset != iPageCharset
        && (iPageCharset != DEFAULT_CHARSET
            || (ppi->pfd.iCharset != DEFAULT_CHARSET
                && ppi->pfd.iCharset != ppd->iFrameCharset)));

    if (fUsePageFont &&
        GetPageFontMetrics(ppd, &ppi->pfd, MLIsMLHInstance(pisp->_psp.hInstance)))
    {
        // Compute Real Dialog Unit for the page
        pisp->_pfx.siz.cx = MulDiv(ppi->pt.x, ppd->sizCache.cx, 4);
        pisp->_pfx.siz.cy = MulDiv(ppi->pt.y, ppd->sizCache.cy, 8);
    } else {
        RECT rcT;
        // IE4 compat - Use the frame font
        rcT.top = rcT.left = 0;         // Win95 will fault if these are uninit
        rcT.right = ppi->pt.x;
        rcT.bottom = ppi->pt.y;
        MapDialogRect(ppd->hDlg, &rcT);
        pisp->_pfx.siz.cx = rcT.right;
        pisp->_pfx.siz.cy = rcT.bottom;

        //
        //  If this is PSP_DLGINDIRECT but the character set and face name
        //  say this is a "generic" property sheet, then take the frame
        //  font or the page font, whichever is bigger.
        //
        //  This fixes the Chinese MingLiu font, which is not as tall as
        //  the English MS Sans Serif font.  Without this fix, we would
        //  use MingLui (the frame font), and then your MS Shell Dlg pages
        //  would get truncated.
        //
        //  (Truncated property sheets is what you got in NT4, but I guess
        //  looking pretty is more important than bug-for-bug compatibility.
        //  Who knows what apps will be broken by this change.)
        //
        if ((pisp->_psp.dwFlags & PSP_DLGINDIRECT) &&
            ppi->pfd.iCharset == DEFAULT_CHARSET &&
            lstrcmpiW(ppi->pfd.szFace, L"MS Shell Dlg") == 0)
        {
            int i;
            GetPageFontMetrics(ppd, &ppi->pfd, FALSE);
            i = MulDiv(ppi->pt.x, ppd->sizCache.cx, 4);
            if (pisp->_pfx.siz.cx < i)
                pisp->_pfx.siz.cx = i;
            i = MulDiv(ppi->pt.y, ppd->sizCache.cy, 8);
            if (pisp->_pfx.siz.cy < i)
                pisp->_pfx.siz.cy = i;

        }
    }
}

void NEAR PASCAL InitPropSheetDlg(HWND hDlg, LPPROPDATA ppd)
{
    PAGEINFOEX pi;
    int dxDlg, dyDlg, dyGrow, dxGrow;
    RECT rcMinSize, rcDlg, rcPage, rcOrigTabs;
    UINT uPages;
    HIMAGELIST himl = NULL;
    TC_ITEMEXTRA tie;
    TCHAR szStartPage[128];
    LPCTSTR pStartPage = NULL;
    UINT nStartPage;
    BOOL fPrematurePages = FALSE;
#ifdef DEBUG
    BOOL fStartPageFound = FALSE;
#endif
    LANGID langidMUI;
#ifndef WINNT
    DWORD dwExStyle;
#endif
    MONITORINFO mMonitorInfo;
    HMONITOR hMonitor;
    BOOL bMirrored = FALSE;
    // set our instance data pointer
    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)ppd);

    // Make sure this gets inited early on.
    ppd->nCurItem = 0;

    // By default we allow the "Apply" button to be enabled
    ppd->fAllowApply = TRUE;

    if (IS_WIZARD(ppd)) {
        // Subclass our buttons so their mnemonics won't screw up applications
        // that run around hiding and showing the buttons behind our back.
        Prsht_SubclassButton(hDlg, IDD_BACK);
        Prsht_SubclassButton(hDlg, IDD_NEXT);
        Prsht_SubclassButton(hDlg, IDD_FINISH);
    } else
        _SetTitle(hDlg, ppd);

    if (ppd->psh.dwFlags & PSH_USEICONID)
    {
        ppd->psh.H_hIcon = LoadImage(ppd->psh.hInstance, ppd->psh.H_pszIcon, IMAGE_ICON, g_cxSmIcon, g_cySmIcon, LR_DEFAULTCOLOR);
    }

    if ((ppd->psh.dwFlags & (PSH_USEICONID | PSH_USEHICON)) && ppd->psh.H_hIcon)
        SendMessage(hDlg, WM_SETICON, FALSE, (LPARAM)(UINT_PTR)ppd->psh.H_hIcon);

    ppd->hDlg = hDlg;

    // IDD_PAGELIST should definitely exist
    ppd->hwndTabs = GetDlgItem(hDlg, IDD_PAGELIST);
    ASSERT(ppd->hwndTabs);
#ifndef WINNT
    // Turn off mirroring inheritance for the property sheet.
    dwExStyle = GetWindowLong(hDlg, GWL_EXSTYLE);
    if (dwExStyle & RTL_MIRRORED_WINDOW)
        SetWindowLong(hDlg, GWL_EXSTYLE, dwExStyle | RTL_NOINHERITLAYOUT);

    dwExStyle = GetWindowLong(ppd->hwndTabs, GWL_EXSTYLE);
    if (dwExStyle & RTL_MIRRORED_WINDOW)
        SetWindowLong(ppd->hwndTabs, GWL_EXSTYLE, dwExStyle | RTL_NOINHERITLAYOUT);
#endif
    TabCtrl_SetItemExtra(ppd->hwndTabs, CB_ITEMEXTRA);

    // nStartPage is either ppd->psh.H_nStartPage or the page pStartPage
    nStartPage = ppd->psh.H_nStartPage;
    if (ppd->psh.dwFlags & PSH_USEPSTARTPAGE)
    {
        nStartPage = 0;                 // Assume we don't find the page
        pStartPage = ppd->psh.H_pStartPage;

        if (IS_INTRESOURCE(pStartPage))
        {
            szStartPage[0] = TEXT('\0');
            LoadString(ppd->psh.hInstance, (UINT)LOWORD(pStartPage),
                       szStartPage, ARRAYSIZE(szStartPage));
            pStartPage = szStartPage;
        }
    }

#ifndef WINDOWS_ME
    tie.tci.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
#endif
    tie.hwndPage = NULL;
    tie.tci.pszText = pi.szCaption;
    tie.state = 0;

    SendMessage(ppd->hwndTabs, WM_SETREDRAW, FALSE, 0L);

    // load langid we chose for frame dialog template
    ppd->wFrameLang =  LANGIDFROMLCID(CCGetProperThreadLocale(NULL));
    
        // it's charset that really matters to font
    ppd->iFrameCharset = GetDefaultCharsetFromLang(ppd->wFrameLang);
    
    langidMUI = GetMUILanguage();

    for (uPages = 0; uPages < ppd->psh.nPages; uPages++)
    {
        PISP  pisp = GETPISP(ppd, uPages);

        if (GetPageInfoEx(ppd, pisp, &pi, langidMUI, GPI_ALL))
        {
            Prsht_ComputeIdealPageSize(ppd, pisp, &pi);

            // Add the page to the end of the tab list

            tie.tci.iImage = -1;
#ifdef WINDOWS_ME
            tie.tci.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE | (pi.bRTL ? TCIF_RTLREADING : 0);
#endif
            if (pi.hIcon) {
                if (!himl) {
                    UINT flags = ILC_MASK;
                    if(IS_WINDOW_RTL_MIRRORED(ppd->hwndTabs)) {
                        flags |= ILC_MIRROR;
                    }    
                    himl = ImageList_Create(g_cxSmIcon, g_cySmIcon, flags, 8, 4);
                    TabCtrl_SetImageList(ppd->hwndTabs, himl);
                }

                tie.tci.iImage = ImageList_AddIcon(himl, pi.hIcon);
                // BUGBUG raymondc - we always destroy even if PSP_USEHICON?
                DestroyIcon(pi.hIcon);
            }

            // BUGBUG? What if this fails? Do we want to destroy the page?
            if (TabCtrl_InsertItem(ppd->hwndTabs, 1000, &tie.tci) >= 0)
            {
                // Nothing to do; all the code that was here got moved elsewhere
            }

            // remember if any page wants premature init
            if (pisp->_psp.dwFlags & PSP_PREMATURE)
                fPrematurePages = TRUE;

            // if the user is specifying the startpage via title, check it here
            if ((ppd->psh.dwFlags & PSH_USEPSTARTPAGE) &&
                !lstrcmpi(pStartPage, pi.szCaption))
            {
                nStartPage = uPages;
#ifdef DEBUG
                fStartPageFound = TRUE;
#endif
            }
        }
        else
        {
            DebugMsg(DM_ERROR, TEXT("PropertySheet failed to GetPageInfo"));
            RemovePropPageData(ppd, uPages--);
        }
    }

    SendMessage(ppd->hwndTabs, WM_SETREDRAW, TRUE, 0L);

    if (ppd->psh.pfnCallback) {
#ifdef WX86
        if (ppd->fFlags & PD_WX86)
            Wx86Callback(ppd->psh.pfnCallback, hDlg, PSCB_INITIALIZED, 0);
        else
#endif
            ppd->psh.pfnCallback(hDlg, PSCB_INITIALIZED, 0);
    }

    //
    // Now compute the size of the tab control.
    //

    // First get the rectangle for the whole dialog
    GetWindowRect(hDlg, &rcDlg);
    
    // For WIZARD_LITE style wizards, we stretch the tabs page and sunken divider
    // to cover the whole wizard (without the border)
    if (ppd->psh.dwFlags & PSH_WIZARD_LITE)
    {
        // Stretch the divider to the whole width of the wizard
        RECT rcDiv, rcDlgClient;
        HWND hDiv;

        // we allow both PSH_WIZARD and PSH_WIZARD_LITE to be set
        // it's exactly the same as setting just PSH_WIZARD_LITE
        RIPMSG(!(ppd->psh.dwFlags & PSH_WIZARD97),
               "Cannot combine PSH_WIZARD_LITE with PSH_WIZARD97");

        // but some bozos do it anyway, so turn off
        ppd->psh.dwFlags &= ~PSH_WIZARD97;

        // NOTE: GetDlgItemRect returns a rectangle relative to hDlg
        hDiv = GetDlgItemRect(hDlg, IDD_DIVIDER, &rcDiv);
        if (hDiv)
            SetWindowPos(hDiv, NULL, 0, rcDiv.top, RECTWIDTH(rcDlg),
                         RECTHEIGHT(rcDiv), SWP_NOZORDER | SWP_NOACTIVATE);

        GetClientRect(hDlg, &rcDlgClient);
        
        // Stretch the page list control to cover the whole wizard client area above
        // the divider
        SetWindowPos(ppd->hwndTabs, NULL, 0, 0, RECTWIDTH(rcDlgClient),
                     rcDiv.top, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    //
    //  While we're thinking about it, don't let people set both
    //  WIZARD97IE4 *and* WIZARD97IE5.  That's just way too strange.
    //
    if (ppd->psh.dwFlags & PSH_WIZARD97IE4)
        ppd->psh.dwFlags &= ~PSH_WIZARD97IE5;

    // Get the rectangle of the pagelist control in pixels.
    GetClientRect(ppd->hwndTabs, &rcOrigTabs);
    ppd->sizMin.cx = rcOrigTabs.right;
    ppd->sizMin.cy = rcOrigTabs.bottom;

    // Compute rcPage = Size of page area in pixels
    // For now, we only care about interior pages; we'll deal with exterior
    // pages later.
    rcPage.left = rcPage.top = 0;
    Prsht_GetIdealPageSize(ppd, (SIZE *)&rcPage.right, GIPS_SKIPEXTERIOR97HEIGHT);

    //
    //  IE4's Wizard97 assumed that all exterior pages were exactly
    //  DEFAULTHEADERHEIGHT dlu's taller than interior pages.  That's
    //  right, DEFAULTHEADERHEIGHT is a pixel count, but IE4 screwed up
    //  and used it as a dlu count here.
    //
    if (ppd->psh.dwFlags & PSH_WIZARD97IE4)
    {
        SIZE sizT;
        SetRect(&rcMinSize, 0, 0, 0, DEFAULTHEADERHEIGHT);
        MapDialogRect(hDlg, &rcMinSize);
        Prsht_GetIdealPageSize(ppd, &sizT, GIPS_SKIPINTERIOR97HEIGHT);
        if (rcPage.bottom < sizT.cy - rcMinSize.bottom)
            rcPage.bottom = sizT.cy - rcMinSize.bottom;
    }

    // Now compute the minimum size for the page region
    rcMinSize = rcPage;

    //
    //  If this is a wizard then set the size of the page area to the entire
    //  size of the control.  If it is a normal property sheet then adjust for
    //  the tabs, resize the control, and then compute the size of the page
    //  region only.
    //
    if (IS_WIZARD(ppd))
        // initialize
        rcPage = rcMinSize;
    else
    {
        int i;
        RECT rcAdjSize;

        // initialize

        for (i = 0; i < 2; i++) {
            rcAdjSize = rcMinSize;
            TabCtrl_AdjustRect(ppd->hwndTabs, TRUE, &rcAdjSize);

            rcAdjSize.right  -= rcAdjSize.left;
            rcAdjSize.bottom -= rcAdjSize.top;
            rcAdjSize.left = rcAdjSize.top = 0;

            if (rcAdjSize.right < rcMinSize.right)
                rcAdjSize.right = rcMinSize.right;
            if (rcAdjSize.bottom < rcMinSize.bottom)
                rcAdjSize.bottom = rcMinSize.bottom;

            SetWindowPos(ppd->hwndTabs, NULL, 0,0, rcAdjSize.right, rcAdjSize.bottom,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        rcPage = rcMinSize = rcAdjSize;
        TabCtrl_AdjustRect(ppd->hwndTabs, FALSE, &rcPage);
    }
    //
    // rcMinSize now contains the size of the control, including the tabs, and
    // rcPage is the rect containing the page portion (without the tabs).
    //

    // For wizard97:
    // Now we have the correct width for our wizard, let's compute the
    // header height based on that, shift the tab window and the pages
    // window down accordingly.
    //
    dyGrow = 0;
    if (ppd->psh.dwFlags & PSH_WIZARD97)
    {
        RECT rcTabs;
        SIZE sizT;

        // NOTE: we don't directly use rcPage because the verticle position for
        // ppd->hwndTabs is not determined, yet, even though the horizontal is
        // already computed. Therefore, we can only use rcPageCopy.right not
        // rcPageCopy.bottom in the following code.
        RECT rcTemp;
        CopyRect(&rcTemp, &rcPage);
        MapWindowPoints(ppd->hwndTabs, hDlg, (LPPOINT)&rcTemp, 2);

        GetWindowRect(ppd->hwndTabs, &rcTabs);
        MapWindowRect(NULL, hDlg, &rcTabs);

        // Set the header fonts first because we need to use the bold font
        // to compute the title height
        _SetHeaderFonts(hDlg, ppd);

        // Adjust the header height
        ppd->cyHeaderHeight = _ComputeHeaderHeight(ppd, rcTemp.right);

        // Since the app can change the subheader text on the fly,
        // our computation of the header height might end up wrong later.
        // Allow ISVs to precompensate for that by setting their exterior
        // pages larger than the interior pages by the amount they want
        // to reserve.  (Wiz97 design sucks.)
        // So if the largest external page is larger than the largest internal
        // page, then expand to enclose the external pages too.
        // IE4 Wizard97 didn't do this and MFC relies on the bug.

        if (!(ppd->psh.dwFlags & PSH_WIZARD97IE4))
        {
            // A margin of 7dlu's is placed above the page, and another
            // margin of 7 dlu's is placed below.
            SetRect(&rcTemp, 0, 0, 0, 7+7);
            MapDialogRect(hDlg, &rcTemp);

            Prsht_GetIdealPageSize(ppd, &sizT, GIPS_SKIPINTERIOR97HEIGHT);

            if (ppd->cyHeaderHeight < sizT.cy - RECTHEIGHT(rcPage) - rcTemp.bottom)
                ppd->cyHeaderHeight = sizT.cy - RECTHEIGHT(rcPage) - rcTemp.bottom;
        }

        // Move the tab window right under the header
        dyGrow += ppd->cyHeaderHeight;
        SetWindowPos(ppd->hwndTabs, NULL, rcTabs.left, rcTabs.top + dyGrow,
                     RECTWIDTH(rcTabs), RECTHEIGHT(rcTabs), SWP_NOZORDER | SWP_NOACTIVATE);
    }

    //
    // Resize the dialog to make room for the control's new size.  This can
    // only grow the size.
    //
    dxGrow = rcMinSize.right - rcOrigTabs.right;
    dxDlg  = rcDlg.right - rcDlg.left + dxGrow;
    dyGrow += rcMinSize.bottom - rcOrigTabs.bottom;
    dyDlg  = rcDlg.bottom - rcDlg.top + dyGrow;

    //
    // Cascade property sheet windows (only for comctl32 and commctrl)
    //

    //
    // HACK: Putting CW_USEDEFAULT in dialog template does not work because
    //  CreateWindowEx ignores it unless the window has WS_OVERLAPPED, which
    //  is not appropriate for a property sheet.
    //
    {
        const TCHAR c_szStatic[] = TEXT("Static");
        UINT swp = SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE;
        if (!IsWindow(ppd->psh.hwndParent)) {
            HWND hwndT = CreateWindowEx(0, c_szStatic, NULL,
                                        WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT,
                                        0, 0, NULL, NULL, HINST_THISDLL, NULL);
            if (hwndT) {
                GetWindowRect(hwndT, &rcDlg);
                swp = SWP_NOZORDER | SWP_NOACTIVATE;
                DestroyWindow(hwndT);
            }
        } else {
            GetWindowRect(ppd->psh.hwndParent, &rcDlg);
            if (IsWindowVisible(ppd->psh.hwndParent)) {
                bMirrored = IS_WINDOW_RTL_MIRRORED(ppd->psh.hwndParent);
                
                rcDlg.top += g_cySmIcon;
                if(bMirrored)
                {
                    rcDlg.left = rcDlg.right - g_cxSmIcon - dxDlg;
                }
                else
                {
                    rcDlg.left += g_cxSmIcon;
                }    
            }
            swp = SWP_NOZORDER | SWP_NOACTIVATE;
        }
        hMonitor = MonitorFromWindow(hDlg, MONITOR_DEFAULTTONEAREST);
        mMonitorInfo.cbSize = sizeof(MONITORINFO);
        if (GetMonitorInfo(hMonitor, &mMonitorInfo))
        {
            if (mMonitorInfo.rcMonitor.right < (rcDlg.left + dxDlg))
            {
                // Move the Window left.
                rcDlg.left = mMonitorInfo.rcMonitor.right - dxDlg;
            }
            if (mMonitorInfo.rcMonitor.left > rcDlg.left)
            {
                // Move the Window Right.
                rcDlg.left = mMonitorInfo.rcMonitor.left;
            }
            if (mMonitorInfo.rcMonitor.bottom < (rcDlg.top + dyDlg))
            {
                // Move the Window Up.
                rcDlg.top = mMonitorInfo.rcMonitor.bottom - dyDlg;
            }
            if (mMonitorInfo.rcMonitor.top > rcDlg.top)
            {
                // Move the Window Down.
                rcDlg.top = mMonitorInfo.rcMonitor.top;
            }
        }
        SetWindowPos(hDlg, NULL, rcDlg.left, rcDlg.top, dxDlg, dyDlg, swp);
    }

    // Now we'll figure out where the page needs to start relative
    // to the bottom of the tabs.
    MapWindowRect(ppd->hwndTabs, hDlg, &rcPage);

    ppd->xSubDlg  = rcPage.left;
    ppd->ySubDlg  = rcPage.top;
    ppd->cxSubDlg = rcPage.right - rcPage.left;
    ppd->cySubDlg = rcPage.bottom - rcPage.top;

    //
    // move all the buttons down as needed and turn on appropriate buttons
    // for a wizard.
    //
    {
        RECT rcCtrl;
        HWND hCtrl;
        const int *pids;

        if (ppd->psh.dwFlags & PSH_WIZARD97)
        {
            hCtrl = GetDlgItemRect(hDlg, IDD_TOPDIVIDER, &rcCtrl);
            if (hCtrl)
                SetWindowPos(hCtrl, NULL, rcCtrl.left, ppd->cyHeaderHeight,
                             RECTWIDTH(rcCtrl) + dxGrow, RECTHEIGHT(rcCtrl), SWP_NOZORDER | SWP_NOACTIVATE);
        }

        if (IS_WIZARD(ppd)) {
            pids = WizIDs;

            hCtrl = GetDlgItemRect(hDlg, IDD_DIVIDER, &rcCtrl);
            if (hCtrl)
                SetWindowPos(hCtrl, NULL, rcCtrl.left, rcCtrl.top + dyGrow,
                             RECTWIDTH(rcCtrl) + dxGrow, RECTHEIGHT(rcCtrl),
                             SWP_NOZORDER | SWP_NOACTIVATE);

            EnableWindow(GetDlgItem(hDlg, IDD_BACK), TRUE);
            ppd->idDefaultFallback = IDD_NEXT;
        } else {
            pids = IDs;
            ppd->idDefaultFallback = IDOK;
        }


        // first move everything over by the same amount that
        // the dialog grew by.

        // If we flipped the buttons, it should be aligned to the left
        // No move needed
        MoveAllButtons(hDlg, pids, IDHELP, ppd->fFlipped ? 0 : dxGrow, dyGrow);
            

        // If there's no help, then remove the help button.
        if (!(ppd->psh.dwFlags & PSH_HASHELP)) {
            RemoveButton(hDlg, IDHELP, pids);
        }

        // If we are not a wizard, and we should NOT show apply now
        if ((ppd->psh.dwFlags & PSH_NOAPPLYNOW) &&
            !IS_WIZARD(ppd))
        {
            RemoveButton(hDlg, IDD_APPLYNOW, pids);
        }

        if (IS_WIZARD(ppd) &&
            (!(ppd->psh.dwFlags & PSH_WIZARDHASFINISH)))
        {
            DWORD dwStyle=0;

            RemoveButton(hDlg, IDD_FINISH, pids);

            // if there's no finish button showing, we need to place it where
            // the next button is
#ifdef UNIX
            // IEUNIX:
            // In motif look the default push button has a window rect which is noticably larger than
            // than a regular button. So before we get the window rect we need to remove this style otherwise
            // the finish button will look larger than the rest of the buttons
            //

            dwStyle=GetWindowLong(GetDlgItem(hDlg, IDD_NEXT),GWL_STYLE);

            SendMessage(GetDlgItem(hDlg, IDD_NEXT),BM_SETSTYLE,dwStyle & (~BS_DEFPUSHBUTTON),0);
#endif
            GetWindowRect(GetDlgItem(hDlg, IDD_NEXT), &rcCtrl);
#ifdef UNIX
            SendMessage(GetDlgItem(hDlg, IDD_NEXT),BM_SETSTYLE,dwStyle,0);
#endif
            MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rcCtrl, 2);
            SetWindowPos(GetDlgItem(hDlg, IDD_FINISH), NULL, rcCtrl.left, rcCtrl.top,
                         RECTWIDTH(rcCtrl), RECTHEIGHT(rcCtrl), SWP_NOZORDER | SWP_NOACTIVATE);
        }

    }

    // (dli) compute the Pattern Brush for the watermark
    // Note: This is done here because we need to know the size of the big dialog in
    // case the user wants to stretch the bitmap
    if (ppd->psh.dwFlags & PSH_WIZARD97)
    {
        int cx, cy;
        ASSERT(ppd->hbmHeader == NULL);
        ASSERT(ppd->hbmWatermark == NULL);

        //
        //  WIZARD97IE4 disabled the watermark and header bitmap
        //  if high contrast was turned on.
        //
        if (ppd->psh.dwFlags & PSH_WIZARD97IE4) {
            HIGHCONTRAST hc = {sizeof(hc)};
            if (SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0) &&
                (hc.dwFlags & HCF_HIGHCONTRASTON)) {
                ppd->psh.dwFlags &= ~(PSH_WATERMARK | PSH_USEHBMWATERMARK |
                                      PSH_USEHPLWATERMARK |
                                      PSH_HEADER | PSH_USEHBMHEADER);
            }
        }

        if ((ppd->psh.dwFlags & PSH_WATERMARK) && ppd->psh.H_hbmWatermark)
        {
            // Compute dimensions of final bitmap, which may be slightly
            // goofy due to stretching

            cx = cy = 0;            // Assume no stretching
            if (ppd->psh.dwFlags & PSH_STRETCHWATERMARK) {
                RECT rc;
                if (ppd->psh.dwFlags & PSH_WIZARD97IE4) {
                    // The WIZARD97IE4 watermark covers the entire dialog
                    if (GetDlgItemRect(hDlg, IDD_DIVIDER, &rc)) {
                        cx = dxDlg;
                        cy = rc.top;
                    }
                } else {
                    // The WIZARD97IE5 watermark does not stretch
                    // (Too many people passed this flag when converting
                    // from WIZARD97IE4 to WIZARD97IE5 and relied on
                    // the nonstretchability.)
                }
            }

            if (ppd->psh.dwFlags & PSH_USEHBMWATERMARK)
            {
                // LR_COPYRETURNORG means "If no stretching was needed,
                // then just return the original bitmap unaltered."
                // Note that we need special cleanup if a stretch occurred.
                ppd->hbmWatermark = (HBITMAP)CopyImage(ppd->psh.H_hbmWatermark,
                            IMAGE_BITMAP, cx, cy, LR_COPYRETURNORG);
            }
            else
            {
                ppd->hbmWatermark = (HBITMAP)LoadImage(ppd->psh.hInstance,
                        ppd->psh.H_pszbmWatermark,
                        IMAGE_BITMAP, cx, cy, LR_CREATEDIBSECTION);
            }

            if (ppd->hbmWatermark)
            {
                // If app provides custom palette, then use it,
                // else create one based on the bmp.  (And if the bmp
                // doesn't have a palette, PaletteFromBmp will use the
                // halftone palette.)

                if (ppd->psh.dwFlags & PSH_USEHPLWATERMARK)
                    ppd->hplWatermark = ppd->psh.hplWatermark;
                else
                    ppd->hplWatermark = PaletteFromBmp(ppd->hbmWatermark);

                // And WIZARD97IE4 needs to turn it into a bitmap brush.
                if (ppd->psh.dwFlags & PSH_WIZARD97IE4)
                    ppd->hbrWatermark = CreatePatternBrush(ppd->hbmWatermark);

            }

        }

        if ((ppd->psh.dwFlags & PSH_HEADER) && ppd->psh.H_hbmHeader)
        {
            cx = cy = 0;            // Assume no stretching
            if (ppd->psh.dwFlags & PSH_STRETCHWATERMARK) {
                if (ppd->psh.dwFlags & PSH_WIZARD97IE4) {
                    // The WIZARD97IE4 header covers the entire header
                    cx = dxDlg;
                    cy = ppd->cyHeaderHeight;
                } else {
                    // The WIZARD97IE5 header does not stretch
                    // (Too many people passed this flag when converting
                    // from WIZARD97IE4 to WIZARD97IE5 and relied on
                    // the nonstretchability.)
                }
            }

            if (ppd->psh.dwFlags & PSH_USEHBMHEADER)
            {
                // LR_COPYRETURNORG means "If no stretching was needed,
                // then just return the original bitmap unaltered."
                // Note that we need special cleanup if a stretch occurred.
                ppd->hbmHeader = (HBITMAP)CopyImage(ppd->psh.H_hbmHeader,
                            IMAGE_BITMAP, cx, cy, LR_COPYRETURNORG);
            }
            else
            {
                ppd->hbmHeader = (HBITMAP)LoadImage(ppd->psh.hInstance,
                        ppd->psh.H_pszbmHeader,
                        IMAGE_BITMAP, cx, cy, LR_CREATEDIBSECTION);
            }

            // And WIZARD97IE4 needs to turn it into a bitmap brush.
            if (ppd->hbmHeader && (ppd->psh.dwFlags & PSH_WIZARD97IE4))
                ppd->hbrHeader = CreatePatternBrush(ppd->hbmHeader);

        }
        else
        {
            // In case the user does not specify a header bitmap
            // use the top portion of the watermark
            ppd->hbmHeader = ppd->hbmWatermark;
            ppd->hbrHeader = ppd->hbrWatermark;
        }

    }


#ifdef UNIX
// IEUNIX : Motif compatibility
// Space the OK, Cancel, Apply and Help buttons evenly
// Assuming all buttons are of equal width
// PORT QSY     do not make adjustment for wizard type, because
//              1) original code did not account for wizard type, causing
//              misplaced controls
//              2) if adjust buttons  for wizard type
//              according to the orignal logic, the buttons
//              do not look very good
if ( !((ppd->psh.dwFlags & PSH_WIZARD) || (ppd->psh.dwFlags & PSH_WIZARD97)) &&
          MwCurrentLook() == LOOK_MOTIF) {
   RECT dim_dlg;
   RECT dim_button;
   int  i;
   int num_buttons = 2;
   int x;
   int y;
   RECT rcTabs;
   DWORD dwStyle;

   int iDefID=LOWORD(SendMessage(hDlg, DM_GETDEFID, 0, 0));

   GetClientRect(ppd->hwndTabs, &rcTabs);
   GetClientRect(hDlg, &dim_dlg);
   /*
     get the size of the ok button without the default rect.
   */

   dwStyle=GetWindowLong(GetDlgItem(hDlg, iDefID),GWL_STYLE);
   SendMessage(GetDlgItem(hDlg, iDefID),BM_SETSTYLE,dwStyle & (~BS_DEFPUSHBUTTON),0);

   GetWindowRect(GetDlgItem(hDlg, IDOK), &dim_button);


   y=rcTabs.bottom+(dim_dlg.bottom-rcTabs.bottom)/2-RECTHEIGHT(dim_button)/2+4;

   // We have OK and Cancel for sure. Find out if we have Apply and Help also
   if ((ppd->psh.dwFlags & PSH_HASHELP)) {
      num_buttons++;
   }

   if (!(ppd->psh.dwFlags & PSH_NOAPPLYNOW))
   {
      num_buttons++;
   }

   x = (RECTWIDTH(dim_dlg) - num_buttons * RECTWIDTH(dim_button))/(num_buttons + 1);

   for (i = 0; i < num_buttons; i++) {
       HWND tmp_button;
       RECT tmp_dim;
       switch(i) {
             case 0 :
                  tmp_button = GetDlgItemRect(hDlg, IDOK, &tmp_dim);
                  break;
             case 1 :
                  tmp_button = GetDlgItemRect(hDlg, IDCANCEL, &tmp_dim);
                  break;
             case 2 :
                  tmp_button = GetDlgItemRect(hDlg, IDD_APPLYNOW, &tmp_dim);
                  break;
             case 3 :
                  tmp_button = GetDlgItemRect(hDlg, IDHELP, &tmp_dim);
                  break;
       }

       SetWindowPos(tmp_button,
                    NULL,
                    (i) * RECTWIDTH(tmp_dim) + (i+1)*x,
                    y, // +10,
                    RECTWIDTH(tmp_dim),
                    RECTHEIGHT(tmp_dim),
                    SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
   }

  /*
     Now that all the buttons are placed Reset the default push button
  */

  SendMessage(GetDlgItem(hDlg, iDefID),BM_SETSTYLE,dwStyle,0);

}
#endif /* UNIX */

    // force the dialog to reposition itself based on its new size

    SendMessage(hDlg, DM_REPOSITION, 0, 0L);

    // do this here instead of using DS_SETFOREGROUND so we don't hose
    // pages that do things that want to set the foreground window
    // BUGBUG raymondc - Why do we do this at all?
    SetForegroundWindow(hDlg);

    // We set this to 1 if the user saves any changes.
    // do this before initting or switching to any pages
    ppd->nReturn = 0;

    // AppHack - Some people forgot to initialize nStartPage, and they were
    // lucky that the garbage value on the stack was zero.  Lucky no longer.
    if (nStartPage >= ppd->psh.nPages) {
        RIPMSG(0, "App forgot to initialize PROPSHEETHEADER.nStartPage field, assuming zero");
        nStartPage = 0;
    }

    // Now attempt to select the starting page.
    TabCtrl_SetCurSel(ppd->hwndTabs, nStartPage);
    PageChange(ppd, 1);
#ifdef DEBUG
    if (ppd->psh.dwFlags & PSH_USEPSTARTPAGE && !fStartPageFound)
        DebugMsg(DM_WARNING, TEXT("sh WN - Property start page '%s' not found."), pStartPage);
#endif

    // Now init any other pages that require it
    if (fPrematurePages)
    {
        int nPage;

        tie.tci.mask = TCIF_PARAM;
        for (nPage = 0; nPage < (int)ppd->psh.nPages; nPage++)
        {
            PISP pisp = GETPISP(ppd, nPage);

            if (!(pisp->_psp.dwFlags & PSP_PREMATURE))
                continue;

            TabCtrl_GetItem(ppd->hwndTabs, nPage, &tie.tci);

            if (tie.hwndPage)
                continue;

            if ((tie.hwndPage = _CreatePage(ppd, pisp, hDlg, langidMUI)) == NULL)
            {
                RemovePropPageData(ppd, nPage--);
                continue;
            }

            TabCtrl_SetItem(ppd->hwndTabs, nPage, &tie.tci);
        }
    }
}

HWND NEAR PASCAL _Ppd_GetPage(LPPROPDATA ppd, int nItem)
{
    if (ppd->hwndTabs)
    {
        TC_ITEMEXTRA tie;
        tie.tci.mask = TCIF_PARAM;
        TabCtrl_GetItem(ppd->hwndTabs, nItem, &tie.tci);
        return tie.hwndPage;
    }
    return NULL;
}

BOOL PASCAL _Ppd_IsPageHidden(LPPROPDATA ppd, int nItem)
{
    if (ppd->hwndTabs)
    {
        TCITEM tci;
        tci.mask = TCIF_STATE;
        tci.dwStateMask = TCIS_HIDDEN;
        if (TabCtrl_GetItem(ppd->hwndTabs, nItem, &tci))
            return tci.dwState;
    }
    return FALSE;
}

LRESULT NEAR PASCAL _Ppd_SendNotify(LPPROPDATA ppd, int nItem, int code, LPARAM lParam)
{
    PSHNOTIFY pshn;

    pshn.lParam = lParam;
    return SendNotifyEx(_Ppd_GetPage(ppd,nItem), ppd->hDlg, code, (LPNMHDR)&pshn, FALSE);
}

//
//  dwFind = 0 means just move to the current item + iAutoAdjust
//  dwFind != 0 means it's a dialog resource identifier we should look for
//
int FindPageIndex(LPPROPDATA ppd, int nCurItem, ULONG_PTR dwFind, LONG_PTR iAutoAdj)
{
    LRESULT nActivate;

    if (dwFind == 0) {
        nActivate = nCurItem + iAutoAdj;
        if (((UINT)nActivate) <= ppd->psh.nPages) {
            return((int)nActivate);
        }
    } else {
        for (nActivate = 0; (UINT)nActivate < ppd->psh.nPages; nActivate++) {
            if ((DWORD_PTR)GETPPSP(ppd, nActivate)->P_pszTemplate == dwFind) {
                return((int)nActivate);
            }
        }
    }
    return(-1);
}

//
//  If hpage != NULL, then return the index of the page which matches it,
//  or -1 on failure.
//
int FindPageIndexByHpage(LPPROPDATA ppd, HPROPSHEETPAGE hpage)
{
    int i;

    //
    //  Notice that we explicitly do not do a InternalizeHPROPSHEETPAGE,
    //  because the app might be passing us garbage.  We just want to
    //  say "Nope, can't find garbage here, sorry."
    //

    for (i = ppd->psh.nPages - 1; i >= 0; i--) {
        if (hpage == GETHPAGE(ppd, i))
            break;
    }
    return i;
}


// This WM_NEXTDLGCTL stuff works, except for ACT!4.0 which faults randomly
// I don't know why.  The USER people said that removing a
// SetFocus(NULL) call from SetDlgFocus works, but I tried that
// and the app merely faulted in a different place.  so I'm going
// back to the old IE4 way, which means that there are scenarios
// where the DEFID can get out of sync with reality.
#undef WM_NEXTDLGCTL_WORKS

#ifdef WM_NEXTDLGCTL_WORKS

//
//  Helper function that manages dialog box focus in a manner that keeps
//  USER in the loop, so we don't get "two buttons both with the bold
//  defpushbutton border" problems.
//
//  We have to use WM_NEXTDLGCTL to fix defid problems, such as this one:
//
//      Right-click My Computer, Properties.
//      Go to Advanced tab. Click Environment Variables.
//      Click New. Type a name for a new dummy environment variable.
//      Click OK.
//
//  At this point (with the old code), the "New" button is a DEFPUSHBUTTON,
//  but the DEFID is IDOK.  The USER folks said I should use WM_NEXTDLGCTL
//  to avoid this problem.  But using WM_NEXTDLGCTL introduces its own big
//  hairy mess of problems.  All the code in this function aside from the
//  SendMessage(WM_NEXTDLGCTL) are to work around "quirks" in WM_NEXTDLGCTL
//  or workarounds for app bugs.
//
//  THIS CODE IS SUBTLE AND QUICK TO ANGER!
//
void SetDlgFocus(LPPROPDATA ppd, HWND hwndFocus)
{
    //
    //  HACK!  It's possible that by the time we get around to changing
    //  the dialog focus, the dialog box doesn't have focus any more!
    //  This happens because PSM_SETWIZBUTTONS is a posted message, so
    //  it can arrive *after* focus has moved elsewhere (e.g., to a
    //  MessageBox).
    //
    //  There is no way to update the dialog box focus without
    //  letting it change the real focus (another "quirk" of
    //  WM_NEXTDLGCTL), so instead we remember who used to have the
    //  focus, let the dialog box do its focus goo, and then restore
    //  the focus as necessary.
    //
    HWND hwndFocusPrev = GetFocus();

    //  If focus belonged to a window within our property sheet, then
    //  let the dialog box code push the focus around.  Otherwise,
    //  focus belonged to somebody outside our property sheet, so
    //  remember to restore it after we're done.

    if (hwndFocusPrev && IsChild(ppd->hDlg, hwndFocusPrev))
        hwndFocusPrev = NULL;

    //  USER forgot to revalidate hwndOldFocus at this point, so we have
    //  to exit USER (by returning to comctl32) then re-enter USER
    //  (in the SendMessage below) so parameter validation will happen
    //  again.  Sigh.

    //
    //  Bug in Win9x and NT:  WM_NEXTDLGCTL will crash if the previous
    //  focus window destroys itself in response to WM_KILLFOCUS.
    //  (WebTurbo by NetMetrics does this.)  There's a missed
    //  revalidation so USER ends up using a window handle after
    //  it has been destroyed.  Oops.
    //
    //  (The NT folks consider this "Won't fix, because the system stays
    //  up; just the app crashes".  The 9x folks will try to get the fix
    //  into Win98 OSR.)
    //

    //
    //  Do a manual SetFocus here to make the old focus (if any)
    //  do all its WM_KILLFOCUS stuff, and possibly destroy itself (grrr).
    //
    //  We have to SetFocus to NULL because some apps (e.g.,
    //  Visual C 6.0 setup) do funky things on SetFocus, and our early
    //  SetFocus interferes with the EM_SETSEL that WM_NEXTDLGCTL will
    //  do later.
    //
    //  APP HACK 2:  But not if the target focus is the same as the
    //  curreng focus, because ACT!4.0 crashes if it receives a
    //  WM_KILLFOCUS when it is not expecting one.

    if (hwndFocus != GetFocus())
        SetFocus(NULL);

    //
    //  Note that by manually shoving the focus around, we
    //  have gotten focus and DEFPUSHBUTTON and DEFID all out
    //  of sync, which is exactly the problem we're trying to
    //  avoid!  Fortunately, USER also contains special
    //  recovery code to handle the case where somebody "mistakenly" called
    //  SetFocus() to change the focus.  (I put "mistakenly" in quotes because
    //  in this case, we did it on purpose.)
    //

    SendMessage(ppd->hDlg, WM_NEXTDLGCTL, (WPARAM)hwndFocus, MAKELPARAM(TRUE, 0));

    //
    //  If WM_NEXTDLGCTL damaged the focus, fix it.
    //
    if (hwndFocusPrev)
        SetFocus(hwndFocusPrev);
}
#endif

void NEAR PASCAL SetNewDefID(LPPROPDATA ppd)
{
    HWND hDlg = ppd->hDlg;
    HWND hwndFocus;
    hwndFocus = GetNextDlgTabItem(ppd->hwndCurPage, NULL, FALSE);
    ASSERT(hwndFocus);
    if (hwndFocus) {
#ifndef WM_NEXTDLGCTL_WORKS
        int id;
        if (((DWORD)SendMessage(hwndFocus, WM_GETDLGCODE, 0, 0L)) & DLGC_HASSETSEL)
        {
            // select the text
            Edit_SetSel(hwndFocus, 0, -1);
        }

        id = GetDlgCtrlID(hwndFocus);
#endif

        //
        //  See if the handle give to us by GetNextDlgTabItem was any good.
        //  (For compatibility reasons, if the dialog contains no tabstops,
        //  it returns the first item.)
        //
        if ((GetWindowLong(hwndFocus, GWL_STYLE) & (WS_VISIBLE | WS_DISABLED | WS_TABSTOP)) == (WS_VISIBLE | WS_TABSTOP))
        {
            //
            //  Give the page a chance to change the default focus.
            //
            HWND hwndT = (HWND)_Ppd_SendNotify(ppd, ppd->nCurItem, PSN_QUERYINITIALFOCUS, (LPARAM)hwndFocus);

            // The window had better be valid and a child of the page.
            if (hwndT && IsWindow(hwndT) && IsChild(ppd->hwndCurPage, hwndT))
            {
                hwndFocus = hwndT;
            }
        }
        else
        {
            // in prop sheet mode, focus on tabs,
            // in wizard mode, tabs aren't visible, go to idDefFallback
            if (IS_WIZARD(ppd))
                hwndFocus = GetDlgItem(hDlg, ppd->idDefaultFallback);
            else
                hwndFocus = ppd->hwndTabs;
        }

#ifdef WM_NEXTDLGCTL_WORKS
        //
        //  Aw-right.  Go for it.
        //
        SetDlgFocus(ppd, hwndFocus);

        //
        //  Hack for MFC:  MFC relies on DM_SETDEFID to know when to
        //  update its wizard buttons.
        //
        SendMessage(hDlg, DM_SETDEFID, SendMessage(hDlg, DM_GETDEFID, 0, 0), 0);
#else
        SetFocus(hwndFocus);
        ResetWizButtons(ppd);
        if (SendDlgItemMessage(ppd->hwndCurPage, id, WM_GETDLGCODE, 0, 0L) & DLGC_UNDEFPUSHBUTTON)
            SendMessage(ppd->hwndCurPage, DM_SETDEFID, id, 0);
        else {
            SendMessage(hDlg, DM_SETDEFID, ppd->idDefaultFallback, 0);
        }
#endif
    }
}


/*
 ** we are about to change pages.  what a nice chance to let the current
 ** page validate itself before we go away.  if the page decides not
 ** to be de-activated, then this'll cancel the page change.
 **
 ** return TRUE iff this page failed validation
 */
BOOL NEAR PASCAL PageChanging(LPPROPDATA ppd)
{
    BOOL bRet = FALSE;
    if (ppd && ppd->hwndCurPage)
    {
        bRet = BOOLFROMPTR(_Ppd_SendNotify(ppd, ppd->nCurItem, PSN_KILLACTIVE, 0));
    }
    return bRet;
}

void NEAR PASCAL PageChange(LPPROPDATA ppd, int iAutoAdj)
{
    HWND hwndCurPage;
    HWND hwndCurFocus;
    int nItem;
    HWND hDlg, hwndTabs;

    TC_ITEMEXTRA tie;
    UINT FlailCount = 0;
    LRESULT lres;

    if (!ppd)
    {
        return;
    }

    hDlg = ppd->hDlg;
    hwndTabs = ppd->hwndTabs;

    // NOTE: the page was already validated (PSN_KILLACTIVE) before
    // the actual page change.

    hwndCurFocus = GetFocus();

TryAgain:
    FlailCount++;
    if (FlailCount > ppd->psh.nPages)
    {
        DebugMsg(DM_TRACE, TEXT("PropSheet PageChange attempt to set activation more than 10 times."));
        return;
    }

    nItem = TabCtrl_GetCurSel(hwndTabs);
    if (nItem < 0)
    {
        return;
    }

    tie.tci.mask = TCIF_PARAM;

    TabCtrl_GetItem(hwndTabs, nItem, &tie.tci);
    hwndCurPage = tie.hwndPage;

    if (!hwndCurPage)
    {
        if ((hwndCurPage = _CreatePage(ppd, GETPISP(ppd, nItem), hDlg, GetMUILanguage())) == NULL)
        {
            /* Should we put up some sort of error message here?
             */
            RemovePropPageData(ppd, nItem);
            TabCtrl_SetCurSel(hwndTabs, 0);
            goto TryAgain;
        }

        // tie.tci.mask    = TCIF_PARAM;
        tie.hwndPage = hwndCurPage;
        TabCtrl_SetItem(hwndTabs, nItem, &tie.tci);

        if (HIDEWIZ97HEADER(ppd, nItem))
            // Subclass for back ground watermark painting.
            SetWindowSubclass(hwndCurPage, WizardWndProc, 0, (DWORD_PTR)ppd);
    }

    // THI WAS REMOVED as part of the fix for bug 18327.  The problem is we need to
    // send a SETACTIVE message to a page if it is being activated.
    //    if (ppd->hwndCurPage == hwndCurPage)
    //    {
    //        /* we should be done at this point.
    //        */
    //        return;
    //    }

    /* Size the dialog and move it to the top of the list before showing
     ** it in case there is size specific initializing to be done in the
     ** GETACTIVE message.
     */

    if (IS_WIZARD(ppd))
    {
        HWND hwndTopDivider= GetDlgItem(hDlg, IDD_TOPDIVIDER);

        if (ppd->psh.dwFlags & PSH_WIZARD97)
        {
            HWND hwndDivider;
            RECT rcDlg, rcDivider;
            GetClientRect(hDlg, &rcDlg);

            hwndDivider = GetDlgItemRect(hDlg, IDD_DIVIDER, &rcDivider);
            if (hwndDivider)
                SetWindowPos(hwndDivider, NULL, rcDlg.left, rcDivider.top,
                             RECTWIDTH(rcDlg), RECTHEIGHT(rcDivider),
                             SWP_NOZORDER | SWP_NOACTIVATE);

            if (GETPPSP(ppd, nItem)->dwFlags & PSP_HIDEHEADER)
            {
                // In this case, we give the whole dialog box except for the portion under the
                // Bottom divider to the property page
                RECT rcTopDivider;
                ShowWindow(hwndTopDivider, SW_HIDE);
                ShowWindow(ppd->hwndTabs, SW_HIDE);

                hwndTopDivider = GetDlgItemRect(hDlg, IDD_DIVIDER, &rcTopDivider);
                SetWindowPos(hwndCurPage, HWND_TOP, rcDlg.left, rcDlg.top, RECTWIDTH(rcDlg), rcTopDivider.top - rcDlg.top, 0);
            }
            else
            {
                ShowWindow(hwndTopDivider, SW_SHOW);
                ShowWindow(ppd->hwndTabs, SW_SHOW);
                SetWindowPos(hwndCurPage, HWND_TOP, ppd->xSubDlg, ppd->ySubDlg, ppd->cxSubDlg, ppd->cySubDlg, 0);
            }
        }
        else
        {
            ShowWindow(hwndTopDivider, SW_HIDE);
            SetWindowPos(hwndCurPage, HWND_TOP, ppd->xSubDlg, ppd->ySubDlg, ppd->cxSubDlg, ppd->cySubDlg, 0);
        }
    } else {
        RECT rcPage;
        GetClientRect(ppd->hwndTabs, &rcPage);
        TabCtrl_AdjustRect(ppd->hwndTabs, FALSE, &rcPage);
        MapWindowPoints(ppd->hwndTabs, hDlg, (LPPOINT)&rcPage, 2);
        SetWindowPos(hwndCurPage, HWND_TOP, rcPage.left, rcPage.top,
                     rcPage.right - rcPage.left, rcPage.bottom - rcPage.top, 0);
    }

    /* We want to send the SETACTIVE message before the window is visible
     ** to minimize on flicker if it needs to update fields.
     */

    //
    //  If the page returns non-zero from the PSN_SETACTIVE call then
    //  we will set the activation to the resource ID returned from
    //  the call and set activation to it.      This is mainly used by wizards
    //  to skip a step.
    //
    lres = _Ppd_SendNotify(ppd, nItem, PSN_SETACTIVE, 0);

    if (lres) {
        int iPageIndex = FindPageIndex(ppd, nItem,
                                       (lres == -1) ? 0 : lres, iAutoAdj);


        if ((lres == -1) &&
            (nItem == iPageIndex || iPageIndex >= TabCtrl_GetItemCount(hwndTabs))) {
            iPageIndex = ppd->nCurItem;
        }

        if (iPageIndex != -1) {
            TabCtrl_SetCurSel(hwndTabs, iPageIndex);
            ShowWindow(hwndCurPage, SW_HIDE);
            goto TryAgain;
        }
    }

    if (ppd->psh.dwFlags & PSH_HASHELP) {
        // PSH_HASHELP controls the "Help" button at the bottom
        // PSH_NOCONTEXTHELP controls the caption "?" button
        Button_Enable(GetDlgItem(hDlg, IDHELP),
                      (BOOL)(GETPPSP(ppd, nItem)->dwFlags & PSP_HASHELP));
    }

    //
    //  If this is a wizard then we'll set the dialog's title to the tab
    //  title.
    //
    if (IS_WIZARD(ppd)) {
        TC_ITEMEXTRA tie;
        TCHAR szTemp[128 + 50];

        tie.tci.mask = TCIF_TEXT;
        tie.tci.pszText = szTemp;
        tie.tci.cchTextMax = ARRAYSIZE(szTemp);
        //// BUGBUG -- Check for error. Does this return false if fails??
        TabCtrl_GetItem(hwndTabs, nItem, &tie.tci);
#ifdef WINDOWS_ME
        tie.tci.mask = TCIF_RTLREADING;
        tie.tci.cchTextMax = 0;
        // hack, use cchTextMax to query tab item reading order
        TabCtrl_GetItem(hwndTabs, nItem, &tie.tci);
        if( (ppd->psh.dwFlags & PSH_RTLREADING) || (tie.tci.cchTextMax))
            SetWindowLong(hDlg, GWL_EXSTYLE, GetWindowLong(hDlg, GWL_EXSTYLE) | WS_EX_RTLREADING);       
        else
            SetWindowLong(hDlg, GWL_EXSTYLE, GetWindowLong(hDlg, GWL_EXSTYLE) & ~WS_EX_RTLREADING);                   
#endif // WINDOWS_ME

        if (szTemp[0])
            SetWindowText(hDlg, szTemp);
    }

    /* Disable all erasebkgnd messages that come through because windows
     ** are getting shuffled.  Note that we need to call ShowWindow (and
     ** not show the window in some other way) because DavidDs is counting
     ** on the correct parameters to the WM_SHOWWINDOW message, and we may
     ** document how to keep your page from flashing.
     */
    ppd->fFlags |= PD_NOERASE;
    ShowWindow(hwndCurPage, SW_SHOW);
    if (ppd->hwndCurPage && (ppd->hwndCurPage != hwndCurPage))
    {
        ShowWindow(ppd->hwndCurPage, SW_HIDE);
    }
    ppd->fFlags &= ~PD_NOERASE;

    ppd->hwndCurPage = hwndCurPage;
    ppd->nCurItem = nItem;

    /* Newly created dialogs seem to steal the focus, so we steal it back
     ** to the page list, which must have had the focus to get to this
     ** point.  If this is a wizard then set the focus to the dialog of
     ** the page.  Otherwise, set the focus to the tabs.
     */
    if (hwndCurFocus != hwndTabs)
    {
        SetNewDefID(ppd);
    }
    else
    {
        // The focus may have been stolen from us, bring it back
        SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)hwndTabs, (LPARAM)TRUE);
    }

    // make sure the header is repaint
    if ((ppd->psh.dwFlags & PSH_WIZARD97) && (!(GETPPSP(ppd, nItem)->dwFlags & PSP_HIDEHEADER)))
        InvalidateRect(hDlg, NULL,TRUE);
}

#define DECLAREWAITCURSOR  HCURSOR hcursor_wait_cursor_save
#define SetWaitCursor()   hcursor_wait_cursor_save = SetCursor(LoadCursor(NULL, IDC_WAIT))
#define ResetWaitCursor() SetCursor(hcursor_wait_cursor_save)

//
// HACKHACK (reinerf)
//
// This function sends the PSN_LASTCHANCEAPPLY right after the property sheets have had "ok"
// pressed. This allows the "General" tab on the file/folder properties to do a rename, so that
// it wont rename the file out from under the other pages, and have them barf when they go to
// persist their info.
//
void NEAR PASCAL SendLastChanceApply(LPPROPDATA ppd)
{
    TC_ITEMEXTRA tie;
    int nItem;
    int nItems = TabCtrl_GetItemCount(ppd->hwndTabs);

    tie.tci.mask = TCIF_PARAM;

    // we start with the last tab and count towards the first. This ensures
    // that the more important tabs (such as the "General" tab) will be the last
    // to recieve the PSN_LASTCHANCEAPPLY message.
    for (nItem = nItems - 1; nItem >= 0; nItem--)
    {
        TabCtrl_GetItem(ppd->hwndTabs, nItem, &tie.tci);

        if (tie.hwndPage)
        {
            // we ignore the return vale from the PSN_LASTCHANCEAPPLY message since
            // there are probably prop sheet extensions that return both TRUE and
            // FALSE for messages that they dont process...(sigh)
            _Ppd_SendNotify(ppd, nItem, PSN_LASTCHANCEAPPLY, (LPARAM)TRUE);
        }
    }
}


#ifdef MAINWIN
EXTERN_C HKEY HKEY_ROOT;
#endif

// return TRUE iff all sheets successfully handle the notification
BOOL NEAR PASCAL ButtonPushed(LPPROPDATA ppd, WPARAM wParam)
{
    HWND hwndTabs;
    int nItems, nItem;
    int nNotify;
    TC_ITEMEXTRA tie;
    BOOL bExit = FALSE;
    int nReturnNew = ppd->nReturn;
    int fSuccess = TRUE;
    DECLAREWAITCURSOR;
    LRESULT lres = 0;
    LPARAM lParam = FALSE;

    switch (wParam) {
        case IDOK:
            lParam = TRUE;
            bExit = TRUE;
            // Fall through...

        case IDD_APPLYNOW:
            // First allow the current dialog to validate itself.
            if (_Ppd_SendNotify(ppd, ppd->nCurItem, PSN_KILLACTIVE, 0))
                return FALSE;

            nReturnNew = 1;

            nNotify = PSN_APPLY;
            break;

        case IDCLOSE:
            lParam = TRUE;
            // fall through
        case IDCANCEL:
            bExit = TRUE;
            nNotify = PSN_RESET;
            break;

        default:
            return FALSE;
    }

    SetWaitCursor();

    hwndTabs = ppd->hwndTabs;

    tie.tci.mask = TCIF_PARAM;

    nItems = TabCtrl_GetItemCount(hwndTabs);
    for (nItem = 0; nItem < nItems; ++nItem)
    {

        TabCtrl_GetItem(hwndTabs, nItem, &tie.tci);

        if (tie.hwndPage)
        {
            /* If the dialog fails a PSN_APPY call (by returning TRUE),
             ** then it has invalid information on it (should be verified
             ** on the PSN_KILLACTIVE, but that is not always possible)
             ** and we want to abort the notifications.  We select the failed
             ** page below.
             */
            lres = _Ppd_SendNotify(ppd, nItem, nNotify, lParam);

            if (lres)
            {
                fSuccess = FALSE;
                bExit = FALSE;
                break;
            } else {
                // if we need a restart (Apply or OK), then this is an exit
                if ((nNotify == PSN_APPLY) && !bExit && ppd->nRestart) {
                    DebugMsg(DM_TRACE, TEXT("PropertySheet: restart flags force close"));
                    bExit = TRUE;
                }
            }

            /* We have either reset or applied, so everything is
             ** up to date.
             */
            tie.state &= ~FLAG_CHANGED;
            // tie.tci.mask = TCIF_PARAM;    // already set
            TabCtrl_SetItem(hwndTabs, nItem, &tie.tci);
        }
    }

#ifdef MAINWIN
    // This is a temporary solution for saving the
    // registry options incase IE doesnot shutdown
    // normaly.
    if( nNotify == PSN_APPLY )
        RegSaveKey(HKEY_ROOT, NULL, NULL);
#endif

    /* If we leave ppd->hwndCurPage as NULL, it will tell the main
     ** loop to exit.
     */
    if (fSuccess)
    {
        ppd->hwndCurPage = NULL;
    }
    else if (lres != PSNRET_INVALID_NOCHANGEPAGE)
    {
        // Need to change to the page that caused the failure.
        // if lres == PSN_INVALID_NOCHANGEPAGE, then assume sheet has already
        // changed to the page with the invalid information on it
        TabCtrl_SetCurSel(hwndTabs, nItem);
    }

    if (fSuccess)
    {
        // Set to the cached value
        ppd->nReturn = nReturnNew;
    }

    if (!bExit)
    {
        // before PageChange, so ApplyNow gets disabled faster.
        if (fSuccess)
        {
            TCHAR szOK[30];
            HWND hwndApply;

            if (!IS_WIZARD(ppd)) {
                // The ApplyNow button should always be disabled after
                // a successfull apply/cancel, since no change has been made yet.
                hwndApply = GetDlgItem(ppd->hDlg, IDD_APPLYNOW);
                Button_SetStyle(hwndApply, BS_PUSHBUTTON, TRUE);
                EnableWindow(hwndApply, FALSE);
                ResetWizButtons(ppd);
                SendMessage(ppd->hDlg, DM_SETDEFID, IDOK, 0);
                ppd->idDefaultFallback = IDOK;
            }

            // Undo PSM_CANCELTOCLOSE for the same reasons.
            if (ppd->fFlags & PD_CANCELTOCLOSE)
            {
                ppd->fFlags &= ~PD_CANCELTOCLOSE;
                LocalizedLoadString(IDS_OK, szOK, ARRAYSIZE(szOK));
                SetDlgItemText(ppd->hDlg, IDOK, szOK);
                EnableWindow(GetDlgItem(ppd->hDlg, IDCANCEL), TRUE);
            }
        }

        /* Re-"select" the current item and get the whole list to
         ** repaint.
         */
        if (lres != PSNRET_INVALID_NOCHANGEPAGE)
            PageChange(ppd, 1);
    }

    ResetWaitCursor();

    return(fSuccess);
}

//  Win3.1 USER didn't handle DM_SETDEFID very well-- it's very possible to get
//  multiple buttons with the default button style look.  This has been fixed
//  for Win95, but the Setup wizard needs this hack when running from 3.1.

// it seems win95 doesn't handle it well either..
void NEAR PASCAL ResetWizButtons(LPPROPDATA ppd)
{
    int id;

    if (IS_WIZARD(ppd)) {

        for (id = 0; id < ARRAYSIZE(WizIDs); id++)
            SendDlgItemMessage(ppd->hDlg, WizIDs[id], BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
    }
}

void NEAR PASCAL SetWizButtons(LPPROPDATA ppd, LPARAM lParam)
{
    int idDef;
    int iShowID = IDD_NEXT;
    int iHideID = IDD_FINISH;
    BOOL bEnabled;
    BOOL bResetFocus;
    HWND hwndShow;
    HWND hwndFocus = GetFocus();
    HWND hwndHide;
    HWND hwndBack;
    HWND hDlg = ppd->hDlg;

    idDef = (int)LOWORD(SendMessage(hDlg, DM_GETDEFID, 0, 0));

    // Enable/Disable the IDD_BACK button
    hwndBack = GetDlgItem(hDlg, IDD_BACK);
    bEnabled = (lParam & PSWIZB_BACK) != 0;
    EnableWindow(hwndBack, bEnabled);

    // Enable/Disable the IDD_NEXT button, and Next gets shown by default
    // bEnabled remembers whether hwndShow should be enabled or not
    hwndShow = GetDlgItem(hDlg, IDD_NEXT);
    bEnabled = (lParam & PSWIZB_NEXT) != 0;
    EnableWindow(hwndShow, bEnabled);

    // Enable/Disable Show/Hide the IDD_FINISH button
    if (lParam & (PSWIZB_FINISH | PSWIZB_DISABLEDFINISH)) {
        iShowID = IDD_FINISH;           // If Finish is being shown
        iHideID = IDD_NEXT;             // then Next isn't

        hwndShow = GetDlgItem(hDlg, IDD_FINISH);
        bEnabled = (lParam & PSWIZB_FINISH) != 0;
        EnableWindow(hwndShow, bEnabled);
    }

    if (!(ppd->psh.dwFlags & PSH_WIZARDHASFINISH)) {
        hwndHide = GetDlgItem(hDlg, iHideID);
        ShowWindow(hwndHide, SW_HIDE);
        // Cannot disable the window; see Prsht_ButtonSubclassProc for explanation.
        // WRONG - EnableWindow(hwndHide, FALSE);

        hwndShow = GetDlgItem(hDlg, iShowID);
        // Cannot disable the window; see Prsht_ButtonSubclassProc for explanation.
        // WRONG - EnableWindow(hwndShow, bEnabled);
        ShowWindow(hwndShow, SW_SHOW);
    }


    // bResetFocus keeps track of whether or not we need to set Focus to our button
    bResetFocus = FALSE;
    if (hwndFocus)
    {
        // if the dude that has focus is a button, we want to steal focus away
        // so users can just press enter all the way through a property sheet,
        // getting the default as they go. this also catches the case
        // of where focus is on one of our buttons which was turned off.
        if (SendMessage(hwndFocus, WM_GETDLGCODE, 0, 0L) & (DLGC_UNDEFPUSHBUTTON|DLGC_DEFPUSHBUTTON))
            bResetFocus = TRUE;
    }
    if (!bResetFocus)
    {
        // if there is no focus or we're focused on an invisible/disabled
        // item on the sheet, grab focus.
        bResetFocus = !hwndFocus ||  !IsWindowVisible(hwndFocus) || !IsWindowEnabled(hwndFocus) ;
    }

    // We used to do this code only if we nuked a button which had default
    // or if bResetFocus. Unfortunately, some wizards turn off BACK+NEXT
    // and then when they turn them back on, they want DEFID on NEXT.
    // So now we always reset DEFID.
    {
        static const int ids[4] = { IDD_NEXT, IDD_FINISH, IDD_BACK, IDCANCEL };
        int i;
        HWND hwndNewFocus = NULL;

        for (i = 0; i < ARRAYSIZE(ids); i++) {
            hwndNewFocus = GetDlgItem(hDlg, ids[i]);

            // can't do IsVisible because we may be doing this
            // before the prop sheet as a whole is shown
            if ((GetWindowLong(hwndNewFocus, GWL_STYLE) & WS_VISIBLE) &&
                IsWindowEnabled(hwndNewFocus)) {
                hwndFocus = hwndNewFocus;
                break;
            }
        }

        ppd->idDefaultFallback = ids[i];
        if (bResetFocus) {
            if (!hwndNewFocus)
                hwndNewFocus = hDlg;
#ifdef WM_NEXTDLGCTL_WORKS
            SetDlgFocus(ppd, hwndNewFocus);
#else
            // 337614 - Since PSM_SETWIZBUTTONS is often a posted message,
            // we may end up here when we don't even have focus at all
            // (caller went on and called MessageBox or something before
            // we got a chance to set the buttons).  So do this only if
            // focus belongs to our dialog box (or if it's nowhere).
            hwndFocus = GetFocus();
            if (!hwndFocus || (ppd->hDlg == hwndFocus || IsChild(ppd->hDlg, hwndFocus)))
                SetFocus(hwndNewFocus);
#endif
        }
        ResetWizButtons(ppd);
        SendMessage(hDlg, DM_SETDEFID, ids[i], 0);

    }
}

//
//  lptie = NULL means "I don't care about the other goop, just give me
//  the index."
//
int NEAR PASCAL FindItem(HWND hwndTabs, HWND hwndPage,  TC_ITEMEXTRA FAR * lptie)
{
    int i;
    TC_ITEMEXTRA tie;

    if (!lptie)
    {
        tie.tci.mask = TCIF_PARAM;
        lptie = &tie;
    }

    for (i = TabCtrl_GetItemCount(hwndTabs) - 1; i >= 0; --i)
    {
        TabCtrl_GetItem(hwndTabs, i, &lptie->tci);

        if (lptie->hwndPage == hwndPage)
        {
            break;
        }
    }

    //this will be -1 if the for loop falls out.
    return i;
}

// a page is telling us that something on it has changed and thus
// "Apply Now" should be enabled

void NEAR PASCAL PageInfoChange(LPPROPDATA ppd, HWND hwndPage)
{
    int i;
    TC_ITEMEXTRA tie;

    tie.tci.mask = TCIF_PARAM;
    i = FindItem(ppd->hwndTabs, hwndPage, &tie);

    if (i == -1)
        return;

    if (!(tie.state & FLAG_CHANGED))
    {
        // tie.tci.mask = TCIF_PARAM;    // already set
        tie.state |= FLAG_CHANGED;
        TabCtrl_SetItem(ppd->hwndTabs, i, &tie.tci);
    }

    if (ppd->fAllowApply)
        EnableWindow(GetDlgItem(ppd->hDlg, IDD_APPLYNOW), TRUE);
}

// a page is telling us that everything has reverted to its last
// saved state.

void NEAR PASCAL PageInfoUnChange(LPPROPDATA ppd, HWND hwndPage)
{
    int i;
    TC_ITEMEXTRA tie;

    tie.tci.mask = TCIF_PARAM;
    i = FindItem(ppd->hwndTabs, hwndPage, &tie);

    if (i == -1)
        return;

    if (tie.state & FLAG_CHANGED)
    {
        tie.state &= ~FLAG_CHANGED;
        TabCtrl_SetItem(ppd->hwndTabs, i, &tie.tci);
    }

    // check all the pages, if none are FLAG_CHANGED, disable IDD_APLYNOW
    for (i = ppd->psh.nPages-1 ; i >= 0 ; i--)
    {
        // BUGBUG? Does TabCtrl_GetItem return its information properly?!?

        if (!TabCtrl_GetItem(ppd->hwndTabs, i, &tie.tci))
            break;
        if (tie.state & FLAG_CHANGED)
            break;
    }
    if (i<0)
        EnableWindow(GetDlgItem(ppd->hDlg, IDD_APPLYNOW), FALSE);
}

HDWP Prsht_RepositionControl(LPPROPDATA ppd, HWND hwnd, HDWP hdwp,
                             int dxMove, int dyMove, int dxSize, int dySize)
{
    if (hwnd) {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        MapWindowRect(HWND_DESKTOP, ppd->hDlg, &rc);
        hdwp = DeferWindowPos(hdwp, hwnd, NULL,
                    rc.left + dxMove, rc.top + dyMove,
                    RECTWIDTH(rc) + dxSize, RECTHEIGHT(rc) + dySize,
                    SWP_NOZORDER | SWP_NOACTIVATE);
    }
    return hdwp;
}

//
//  dxSize/(dySize+dyMove) is the amount by which to resize the tab control.
//  dxSize/dySize controls how much the dialog should be grown.
//  Buttons move by (dxSize, dySize+dyMove).
//

BOOL Prsht_ResizeDialog(LPPROPDATA ppd, int dxSize, int dySize, int dyMove)
{
    BOOL fChanged = dxSize || dySize || dyMove;
    if (fChanged)
    {
        int dxMove = 0;     // To make the code more symmetric in x and y
        int dxAll = dxSize + dxMove;
        int dyAll = dySize + dyMove;
        RECT rc;
        UINT i;
        const int *rgid;
        UINT cid;
        HDWP hdwp;
        HWND hwnd;

        // Use DeferWindowPos to avoid flickering.  We expect to move
        // the tab control, up to five buttons, two possible dividers,
        // plus the current page.  (And a partridge in a pear tree.)
        //

        hdwp = BeginDeferWindowPos(1 + 5 + 2 + 1);

        // The tab control just sizes.
        hdwp = Prsht_RepositionControl(ppd, ppd->hwndTabs, hdwp,
                                       0, 0, dxAll, dyAll);

        //
        //  Move and size the current page.  We can't trust its location
        //  or size, since PageChange shoves it around without updating
        //  ppd->ySubDlg.
        //
        if (ppd->hwndCurPage) {
            hdwp = DeferWindowPos(hdwp, ppd->hwndCurPage, NULL,
                        ppd->xSubDlg, ppd->ySubDlg,
                        ppd->cxSubDlg, ppd->cySubDlg,
                        SWP_NOZORDER | SWP_NOACTIVATE);
        }

        //
        //  And our buttons just move by both the size and move (since they
        //  lie below both the tabs and the pages).
        //
        if (IS_WIZARD(ppd)) {
            //
            //  Ooh, wait, reposition the separator lines, too.
            //  Moves vertically but resizes horizontally.
            //
            hwnd = GetDlgItem(ppd->hDlg, IDD_DIVIDER);
            hdwp = Prsht_RepositionControl(ppd, hwnd, hdwp,
                                           0, dyAll, dxAll, 0);

            //
            //  The top divider does not move vertically since it lies
            //  above the area that is changing.
            //
            hwnd = GetDlgItem(ppd->hDlg, IDD_TOPDIVIDER);
            hdwp = Prsht_RepositionControl(ppd, hwnd, hdwp,
                                           0, 0, dxAll, 0);

            rgid = WizIDs;
            cid = ARRAYSIZE(WizIDs);
        } else {
            rgid = IDs;
            cid = ARRAYSIZE(IDs);
        }

        for (i = 0 ; i < cid; i++)
        {
            hwnd = GetDlgItem(ppd->hDlg, rgid[i]);
            hdwp = Prsht_RepositionControl(ppd, hwnd, hdwp,
                                           dxAll, dyAll, 0, 0);
        }

        // All finished sizing and moving.  Let 'er rip!
        if (hdwp)
            EndDeferWindowPos(hdwp);

        // Grow ourselves as well
        GetWindowRect(ppd->hDlg, &rc);
        SetWindowPos(ppd->hDlg, NULL, 0, 0,
                     RECTWIDTH(rc) + dxAll, RECTHEIGHT(rc) + dyAll,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    return fChanged;
}

BOOL Prsht_RecalcPageSizes(LPPROPDATA ppd)
{
    SIZE siz;
    int dxSize = 0, dySize = 0, dyMove = 0;

    // After inserting or removing a page, the tab control may have
    // changed height.  If so, then we need to resize ourselves to
    // accomodate the growth or shrinkage, so that all the tabs remain
    // visible.
    //
    // APP COMPAT!  We cannot do this by default because Jamba 1.1
    // **FAULTS** if the property sheet changes size after creation.
    // Grrrrrr...

    // Wizards don't have a visible tab control,
    // so do this only for non-wizards
    if (!IS_WIZARD(ppd))
    {
        RECT rc;

        // Get the client rect of the tab control in dialog coords
        GetClientRect(ppd->hwndTabs, &rc);
        MapWindowRect(ppd->hwndTabs, ppd->hDlg, &rc);

        // See how many rows there are now
        TabCtrl_AdjustRect(ppd->hwndTabs, FALSE, &rc);

        // rc.top is the new ySubDlg.  Compute the amount we have to move.
        dyMove = rc.top - ppd->ySubDlg;
        ppd->ySubDlg = rc.top;
    }

    Prsht_GetIdealPageSize(ppd, &siz, GIPS_SKIPEXTERIOR97HEIGHT);
    dxSize = siz.cx - ppd->cxSubDlg;
    dySize = siz.cy - ppd->cySubDlg;
    ppd->cxSubDlg = siz.cx;
    ppd->cySubDlg = siz.cy;
    return Prsht_ResizeDialog(ppd, dxSize, dySize, dyMove);
}

//
//  InsertPropPage
//
//  hpage is the page being inserted.
//
//  hpageInsertAfter described where it should be inserted.
//
//  hpageInsertAfter can be...
//
//      MAKEINTRESOURCE(index) to insert at a specific index.
//
//      NULL to insert at the beginning
//
//      an HPROPSHEETPAGE to insert *after* that page
//
BOOL NEAR PASCAL InsertPropPage(LPPROPDATA ppd, PSP FAR * hpageInsertAfter,
                                PSP FAR * hpage)
{
    TC_ITEMEXTRA tie;
    int nPage;
    HIMAGELIST himl;
    PAGEINFOEX pi;
    PISP pisp;
    int idx;

    hpage = _Hijaak95Hack(ppd, hpage);

    if (!hpage)
        return FALSE;

    if (ppd->psh.nPages >= MAXPROPPAGES)
        return FALSE; // we're full

    if (IS_INTRESOURCE(hpageInsertAfter))
    {
        // Inserting by index
        idx = (int) PtrToLong(hpageInsertAfter);

        // Attempting to insert past the end is the same as appending.
        if (idx > (int)ppd->psh.nPages)
            idx = (int)ppd->psh.nPages;
    }
    else
    {
        // Inserting by hpageInsertAfter.
        for (idx = 0; idx < (int)(ppd->psh.nPages); idx++) {
            if (hpageInsertAfter == GETHPAGE(ppd, idx))
                break;
        }

        if (idx >= (int)(ppd->psh.nPages))
            return FALSE; // hpageInsertAfter not found

        idx++; // idx Points to the insertion location (to the right of hpageInsertAfter)
        ASSERT(hpageInsertAfter == GETHPAGE(ppd, idx-1));
    }

    ASSERT(idx <= (int)(ppd->psh.nPages+1));

    // Shift all pages adjacent to the insertion point to the right
    for (nPage=ppd->psh.nPages - 1; nPage >= idx; nPage--)
        SETPISP(ppd, nPage+1, GETPISP(ppd, nPage));

    // Insert the new page
    pisp = InternalizeHPROPSHEETPAGE(hpage);
    SETPISP(ppd, idx, pisp);

    ppd->psh.nPages++;

    himl = TabCtrl_GetImageList(ppd->hwndTabs);

    if (!GetPageInfoEx(ppd, pisp, &pi, GetMUILanguage(),
                       GPI_ICON | GPI_BRTL | GPI_CAPTION | GPI_FONT | GPI_DIALOGEX))
    {
        DebugMsg(DM_ERROR, TEXT("InsertPropPage: GetPageInfo failed"));
        goto bogus;
    }

    Prsht_ComputeIdealPageSize(ppd, pisp, &pi);

#ifndef WINDOWS_ME
    tie.tci.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
#else
    tie.tci.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE | (pi.bRTL ? TCIF_RTLREADING : 0);
#endif
    tie.hwndPage = NULL;
    tie.tci.pszText = pi.szCaption;
    tie.state = 0;


    if (pi.hIcon) {
        if (himl)
            tie.tci.iImage = ImageList_AddIcon(himl, pi.hIcon);
        DestroyIcon(pi.hIcon);
    } else {
        tie.tci.iImage = -1;
    }

    // Insert the page into the tab list
    TabCtrl_InsertItem(ppd->hwndTabs, idx, &tie.tci);

    // If this page wants premature initialization then init it
    // do this last so pages can rely on "being there" at init time
    if (pisp->_psp.dwFlags & PSP_PREMATURE)
    {
        if ((tie.hwndPage = _CreatePage(ppd, pisp, ppd->hDlg, GetMUILanguage())) == NULL)
        {
            TabCtrl_DeleteItem(ppd->hwndTabs, idx);
            // don't free the pisp here let the caller do it
            // BUGBUG raymondc - but caller doesn't know if hIcon has been destroyed
            goto bogus;
        }

        tie.tci.mask = TCIF_PARAM;
        TabCtrl_SetItem(ppd->hwndTabs, idx, &tie.tci);
    }

    // Adjust the internally track current item if it is to the right of our insertion point
    if (ppd->nCurItem >= idx)
        ppd->nCurItem++;

    return TRUE;

bogus:
    // Shift everything back
    for (nPage=idx; nPage < (int)(ppd->psh.nPages-1); nPage++)
        SETPISP(ppd, nPage, GETPISP(ppd, nPage+1));

    ppd->psh.nPages--;
    return FALSE;
}

#define AddPropPage(ppd, hpage) InsertPropPage(ppd, (LPVOID)MAKEINTRESOURCE(-1), hpage)

// removes property sheet hpage (index if NULL)
void NEAR PASCAL RemovePropPage(LPPROPDATA ppd, int index, HPROPSHEETPAGE hpage)
{
    int i = -1;
    BOOL fReturn = TRUE;
    TC_ITEMEXTRA tie;

    //
    //  Notice that we explicitly do not do a InternalizeHPROPSHEETPAGE,
    //  because the app might be passing us garbage.  We just want to
    //  say "Nope, can't find garbage here, sorry."
    //

    tie.tci.mask = TCIF_PARAM;
    if (hpage) {
        i = FindPageIndexByHpage(ppd, hpage);
    }
    if (i == -1) {
        i = index;

        // this catches i < 0 && i >= (int)(ppd->psh.nPages)
        if ((UINT)i >= ppd->psh.nPages)
        {
            DebugMsg(DM_ERROR, TEXT("RemovePropPage: invalid page"));
            return;
        }
    }

    index = TabCtrl_GetCurSel(ppd->hwndTabs);
    if (i == index) {
        // if we're removing the current page, select another (don't worry
        // about this page having invalid information on it -- we're nuking it)
        PageChanging(ppd);

        if (index == 0)
            index++;
        else
            index--;

        if (SendMessage(ppd->hwndTabs, TCM_SETCURSEL, index, 0L) == -1) {
            // if we couldn't select (find) the new one, punt to 0th
            SendMessage(ppd->hwndTabs, TCM_SETCURSEL, 0, 0L);
        }
        PageChange(ppd, 1);
    }

    // BUGBUG if removing a page below ppd->nCurItem, need to update
    // nCurItem to prevent it from getting out of sync with hwndCurPage?

    tie.tci.mask = TCIF_PARAM;
    TabCtrl_GetItem(ppd->hwndTabs, i, &tie.tci);
    if (tie.hwndPage) {
        if (ppd->hwndCurPage == tie.hwndPage)
            ppd->hwndCurPage = NULL;
        DestroyWindow(tie.hwndPage);
    }

    RemovePropPageData(ppd, i);
}

void NEAR PASCAL RemovePropPageData(LPPROPDATA ppd, int nPage)
{
    TabCtrl_DeleteItem(ppd->hwndTabs, nPage);
    DestroyPropertySheetPage(GETHPAGE(ppd, nPage));

    //
    //  Delete the HPROPSHEETPAGE from our table and slide everybody down.
    //
    ppd->psh.nPages--;
    hmemcpy(&ppd->psh.H_phpage[nPage], &ppd->psh.H_phpage[nPage + 1],
            sizeof(ppd->psh.H_phpage[0]) * (ppd->psh.nPages - nPage));
}

// returns TRUE iff the page was successfully set to index/hpage
// Note:  The iAutoAdj should be set to 1 or -1.  This value is used
//        by PageChange if a page refuses a SETACTIVE to either increment
//        or decrement the page index.
BOOL NEAR PASCAL PageSetSelection(LPPROPDATA ppd, int index, HPROPSHEETPAGE hpage,
                                  int iAutoAdj)
{
    int i = -1;
    BOOL fReturn = FALSE;
    TC_ITEMEXTRA tie;

    tie.tci.mask = TCIF_PARAM;
    if (hpage) {
        for (i = ppd->psh.nPages - 1; i >= 0; i--) {
            if (hpage == GETHPAGE(ppd, i))
                break;
        }
    }
    if (i == -1) {
        if (index == -1)
            return FALSE;

        i = index;
    }
    if (i >= MAXPROPPAGES)
    {
        // don't go off the end of our HPROPSHEETPAGE array
        return FALSE;
    }

    fReturn = !PageChanging(ppd);
    if (fReturn)
    {
        index = TabCtrl_GetCurSel(ppd->hwndTabs);
        if (SendMessage(ppd->hwndTabs, TCM_SETCURSEL, i, 0L) == -1) {
            // if we couldn't select (find) the new one, fail out
            // and restore the old one
            SendMessage(ppd->hwndTabs, TCM_SETCURSEL, index, 0L);
            fReturn = FALSE;
        }
        PageChange(ppd, iAutoAdj);
    }
    return fReturn;
}

LRESULT NEAR PASCAL QuerySiblings(LPPROPDATA ppd, WPARAM wParam, LPARAM lParam)
{
    UINT i;
    for (i = 0 ; i < ppd->psh.nPages ; i++)
    {
        HWND hwndSibling = _Ppd_GetPage(ppd, i);
        if (hwndSibling)
        {
            LRESULT lres = SendMessage(hwndSibling, PSM_QUERYSIBLINGS, wParam, lParam);
            if (lres)
                return lres;
        }
    }
    return FALSE;
}

// REVIEW HACK This gets round the problem of having a hotkey control
// up and trying to enter the hotkey that is already in use by a window.
BOOL NEAR PASCAL HandleHotkey(LPARAM lparam)
{
    WORD wHotkey;
    TCHAR szClass[32];
    HWND hwnd;

    // What hotkey did the user type hit?
    wHotkey = (WORD)SendMessage((HWND)lparam, WM_GETHOTKEY, 0, 0);
    // Were they typing in a hotkey window?
    hwnd = GetFocus();
    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
    if (lstrcmp(szClass, HOTKEY_CLASS) == 0)
    {
        // Yes.
        SendMessage(hwnd, HKM_SETHOTKEY, wHotkey, 0);
        return TRUE;
    }
    return FALSE;
}


//
//  Function handles Next and Back functions for wizards.  The code will
//  be either PSN_WIZNEXT or PSN_WIZBACK
//
BOOL NEAR PASCAL WizNextBack(LPPROPDATA ppd, int code)
{
    LRESULT   dwFind;
    int iPageIndex;
    int iAutoAdj = (code == PSN_WIZNEXT) ? 1 : -1;

    dwFind = _Ppd_SendNotify(ppd, ppd->nCurItem, code, 0);

    if (dwFind == -1) {
        return(FALSE);
    }

    iPageIndex = FindPageIndex(ppd, ppd->nCurItem, dwFind, iAutoAdj);

    if (iPageIndex == -1) {
        return(FALSE);
    }

    return(PageSetSelection(ppd, iPageIndex, NULL, iAutoAdj));
}

BOOL NEAR PASCAL Prsht_OnCommand(LPPROPDATA ppd, int id, HWND hwndCtrl, UINT codeNotify)
{

    //
    //  There's a bug in USER that when the user highlights a defpushbutton
    //  and presses ENTER, the WM_COMMAND is sent to the top-level dialog
    //  (i.e., the property sheet) instead of to the parent of the button.
    //  So if a property sheet page has a control whose ID coincidentally
    //  matches any of our own, we will think it's ours instead of theirs.
    if (hwndCtrl && GetParent(hwndCtrl) != ppd->hDlg)
        goto Forward;

    if (!hwndCtrl)
        hwndCtrl = GetDlgItem(ppd->hDlg, id);

    switch (id) {

        case IDCLOSE:
        case IDCANCEL:
            if (_Ppd_SendNotify(ppd, ppd->nCurItem, PSN_QUERYCANCEL, 0) == 0) {
                ButtonPushed(ppd, id);
            }
            break;

        case IDD_APPLYNOW:
        case IDOK:
            if (!IS_WIZARD(ppd)) {

                //ButtonPushed returns true if and only if all pages have processed PSN_LASTCHANCEAPPLY
                if (ButtonPushed(ppd, id))
                {

                    //Everyone has processed the PSN_APPLY Message.  Now send PSN_LASTCHANCEAPPLY message.

                    //
                    // HACKHACK (reinerF)
                    //
                    // We send out a private PSN_LASTCHANCEAPPLY message telling all the pages
                    // that everyone is done w/ the apply. This is needed for pages who have to do
                    // something after every other page has applied. Currently, the "General" tab
                    // of the file properties needs a last-chance to rename files as well as new print
                    // dialog in  comdlg32.dll.
                    SendLastChanceApply(ppd);
                }
            }
            break;


        case IDHELP:
            if (IsWindowEnabled(hwndCtrl))
            {
                _Ppd_SendNotify(ppd, ppd->nCurItem, PSN_HELP, 0);
            }
            break;

        case IDD_FINISH:
        {
            HWND hwndNewFocus;
            EnableWindow(ppd->hDlg, FALSE);
            hwndNewFocus = (HWND)_Ppd_SendNotify(ppd, ppd->nCurItem, PSN_WIZFINISH, 0);
            // b#11346 - dont let multiple clicks on FINISH.
            if (!hwndNewFocus)
            {
                ppd->hwndCurPage = NULL;
                ppd->nReturn = 1;
            }
            else
            {
                EnableWindow(ppd->hDlg, TRUE);
                if (IsWindow(hwndNewFocus) && IsChild(ppd->hDlg, hwndNewFocus))
#ifdef WM_NEXTDLGCTL_WORKS
                    SetDlgFocus(ppd, hwndNewFocus);
#else
                    SetFocus(hwndNewFocus);
#endif
            }
        }
        break;

        case IDD_NEXT:
        case IDD_BACK:
            ppd->idDefaultFallback = id;
            WizNextBack(ppd, id == IDD_NEXT ? PSN_WIZNEXT : PSN_WIZBACK);
            break;

        default:
Forward:
            FORWARD_WM_COMMAND(_Ppd_GetPage(ppd, ppd->nCurItem), id, hwndCtrl, codeNotify, SendMessage);
    }

    return TRUE;
}

BOOL NEAR PASCAL Prop_IsDialogMessage(LPPROPDATA ppd, LPMSG32 pmsg32)
{
    if (pmsg32 == NULL)
    {
        RIPMSG(0, "PSM_ISDIALOGMESSAGE: lParam == NULL invalid");
        return FALSE;
    }

    if ((pmsg32->message == WM_KEYDOWN) && (GetKeyState(VK_CONTROL) < 0))
    {
        BOOL bBack = FALSE;

        switch (pmsg32->wParam) {
            case VK_TAB:
                bBack = GetKeyState(VK_SHIFT) < 0;
                break;

            case VK_PRIOR:  // VK_PAGE_UP
            case VK_NEXT:   // VK_PAGE_DOWN
                bBack = (pmsg32->wParam == VK_PRIOR);
                break;

            default:
                goto NoKeys;
        }
#ifdef KEYBOARDCUES
        //notify of navigation key usage
        SendMessage(ppd->hDlg, WM_CHANGEUISTATE, 
            MAKELONG(UIS_CLEAR, UISF_HIDEFOCUS | UISF_HIDEACCEL), 0);
#endif

        if (IS_WIZARD(ppd))
        {
            int idWiz;
            int idDlg;
            HWND hwnd;

            if (bBack) {
                idWiz = PSN_WIZBACK;
                idDlg = IDD_BACK;
            } else {
                idWiz = PSN_WIZNEXT;
                idDlg = IDD_NEXT;
            }

            hwnd = GetDlgItem(ppd->hDlg, idDlg);
            if (IsWindowVisible(hwnd) && IsWindowEnabled(hwnd))
                WizNextBack(ppd, idWiz);
        }
        else
        {
            int iStart = TabCtrl_GetCurSel(ppd->hwndTabs);
            int iCur;

            //
            //  Skip over hidden tabs, but don't go into an infinite loop.
            //
            iCur = iStart;
            do {
                // tab in reverse if shift is down
                if (bBack)
                    iCur += (ppd->psh.nPages - 1);
                else
                    iCur++;

                iCur %= ppd->psh.nPages;
            } while (_Ppd_IsPageHidden(ppd, iCur) && iCur != iStart);
            PageSetSelection(ppd, iCur, NULL, 1);
        }
        return TRUE;
    }
NoKeys:

    //
    //  Since we now send out a PSN_TRANSLATEACCELERATOR, add a
    //  short-circuit so we don't do all this work for things
    //  that can't possibly be accelerators.
    //
    if (pmsg32->message >= WM_KEYFIRST && pmsg32->message <= WM_KEYLAST &&

    // And there had better be a target window...

        pmsg32->hwnd &&

    // and the target window must live either outside the propsheet
    // altogether or completely inside the propsheet page.
    // (This is so that the propsheet can display its own popup dialog,
    // but can't futz with the tab control or OK/Cancel buttons.)

            (!IsChild(ppd->hDlg, pmsg32->hwnd) ||
              IsChild(ppd->hwndCurPage, pmsg32->hwnd)) &&

    // Then ask the propsheet if he wants to eat it.
        _Ppd_SendNotify(ppd, ppd->nCurItem,
                        PSN_TRANSLATEACCELERATOR, (LPARAM)pmsg32) == PSNRET_MESSAGEHANDLED)
        return TRUE;

    if (IsDialogMessage32(ppd->hDlg, pmsg32, TRUE))
        return TRUE;

    return FALSE;
}

HRESULT Prsht_GetObject (LPPROPDATA ppd, HWND hDlg, int iItem, const IID *piid, void **pObject)
{
    TC_ITEMEXTRA tie;
    NMOBJECTNOTIFY non;
    PISP pisp = GETPISP(ppd, iItem);
    *pObject = NULL;

    tie.tci.mask = TCIF_PARAM;
    TabCtrl_GetItem(ppd->hwndTabs, iItem, &tie.tci);
    if (!tie.hwndPage && ((tie.hwndPage = _CreatePage(ppd, pisp, hDlg, GetMUILanguage())) == NULL))
    {
        RemovePropPageData(ppd, iItem);
        return E_UNEXPECTED;
    }
    TabCtrl_SetItem(ppd->hwndTabs, iItem, &tie.tci);

    non.iItem = -1;
    non.piid = piid;
    non.pObject = NULL;
    non.hResult = E_NOINTERFACE;
    non.dwFlags = 0;

    SendNotifyEx (tie.hwndPage, ppd->hwndTabs, PSN_GETOBJECT,
                  &non.hdr,
#ifdef UNICODE
                  TRUE
#else
                  FALSE
#endif //UNICODE
                 );
    if (SUCCEEDED (non.hResult))
    {
        *pObject = non.pObject;
        if (pObject == NULL)
            non.hResult = E_UNEXPECTED;
    }
    else if (non.pObject)
    {
        ((LPDROPTARGET) non.pObject)->lpVtbl->Release ((LPDROPTARGET) non.pObject);
        non.pObject = NULL;
    }
    return non.hResult;
}

//
//  We would not normally need IDD_PAGELIST except that DefWindowProc() and
//  WinHelp() do hit-testing differently.  DefWindowProc() will do cool
//  things like checking against the SetWindowRgn and skipping over windows
//  that return HTTRANSPARENT.  WinHelp() on the other hand is stupid and
//  ignores window regions and transparency.  So what happens is if you
//  click on the transparent part of a tab control, DefWindowProc() says
//  (correctly) "He clicked on the dialog background".  We then say, "Okay,
//  WinHelp(), go display context help for the dialog background", and it
//  says, "Hey, I found a tab control.  I'm going to display help for the
//  tab control now."  To keep a bogus context menu from appearing, we
//  explicitly tell WinHelp that "If you found a tab control (IDD_PAGELIST),
//  then ignore it (NO_HELP)."
//
const static DWORD aPropHelpIDs[] = {  // Context Help IDs
    IDD_APPLYNOW, IDH_COMM_APPLYNOW,
    IDD_PAGELIST, NO_HELP,
    0, 0
};


void HandlePaletteChange(LPPROPDATA ppd, UINT uMessage, HWND hDlg)
{
    HDC hdc;
    hdc = GetDC(hDlg);
    if (hdc)
    {
        BOOL fRepaint;
        SelectPalette(hdc,ppd->hplWatermark,(uMessage == WM_PALETTECHANGED));
        fRepaint = RealizePalette(hdc);
        if (fRepaint)
            InvalidateRect(hDlg,NULL,TRUE);
    }
    ReleaseDC(hDlg,hdc);
}

//
//  Paint a rectangle with the specified brush and palette.
//
void PaintWithPaletteBrush(HDC hdc, LPRECT lprc, HPALETTE hplPaint, HBRUSH hbrPaint)
{
    HBRUSH hbrPrev = SelectBrush(hdc, hbrPaint);
    UnrealizeObject(hbrPaint);
    if (hplPaint)
    {
        SelectPalette(hdc, hplPaint, FALSE);
        RealizePalette(hdc);
    }
    FillRect(hdc, lprc, hbrPaint);
    SelectBrush(hdc, hbrPrev);
}

//
//  lprc is the target rectangle.
//  Use as much of the bitmap as will fit into the target rectangle.
//  If the bitmap is smaller than the target rectangle, then fill the rest with
//  the pixel in the upper left corner of the hbmpPaint.
//
void PaintWithPaletteBitmap(HDC hdc, LPRECT lprc, HPALETTE hplPaint, HBITMAP hbmpPaint)
{
    HDC hdcBmp;
    BITMAP bm;
    int cxRect, cyRect, cxBmp, cyBmp;

    GetObject(hbmpPaint, sizeof(BITMAP), &bm);
    hdcBmp = CreateCompatibleDC(hdc);
    SelectObject(hdcBmp, hbmpPaint);

    if (hplPaint)
    {
        SelectPalette(hdc, hplPaint, FALSE);
        RealizePalette(hdc);
    }

    cxRect = RECTWIDTH(*lprc);
    cyRect = RECTHEIGHT(*lprc);

    //  Never use more pixels from the bmp as we have room in the rect.
    cxBmp = min(bm.bmWidth, cxRect);
    cyBmp = min(bm.bmHeight, cyRect);

    BitBlt(hdc, lprc->left, lprc->top, cxBmp, cyBmp, hdcBmp, 0, 0, SRCCOPY);

    // If bitmap is too narrow, then StretchBlt to fill the width.
    if (cxBmp < cxRect)
        StretchBlt(hdc, lprc->left + cxBmp, lprc->top,
                   cxRect - cxBmp, cyBmp,
                   hdcBmp, 0, 0, 1, 1, SRCCOPY);

    // If bitmap is to short, then StretchBlt to fill the height.
    if (cyBmp < cyRect)
        StretchBlt(hdc, lprc->left, cyBmp,
                   cxRect, cyRect - cyBmp,
                   hdcBmp, 0, 0, 1, 1, SRCCOPY);

    DeleteDC(hdcBmp);
}

void _SetHeaderTitles(HWND hDlg, LPPROPDATA ppd, UINT uPage, LPCTSTR pszNewTitle, BOOL bTitle)
{
    PISP pisp = NULL;

    // Must be for wizard97 
    if (ppd->psh.dwFlags & PSH_WIZARD97)
    {
        // Page number must be within range
        if (uPage < ppd->psh.nPages)
        {
            // Get the page structure
            pisp = GETPISP(ppd, uPage);

            // We should have this page if it's within range
            ASSERT(pisp);

            // Do this only if this page has header.
            if (!(pisp->_psp.dwFlags & PSP_HIDEHEADER))
            {
                LPCTSTR pszOldTitle = bTitle ? pisp->_psp.pszHeaderTitle : pisp->_psp.pszHeaderSubTitle; 

                if (!IS_INTRESOURCE(pszOldTitle))
                    LocalFree((LPVOID)pszOldTitle);

                // Set the new title
                if (bTitle)
                    pisp->_psp.pszHeaderTitle = pszNewTitle;
                else
                    pisp->_psp.pszHeaderSubTitle = pszNewTitle;

                // set pszNewTitle to NULL here so that we don't free it later
                pszNewTitle = NULL;
                
                // set the correct flags
                pisp->_psp.dwFlags |= bTitle ? PSP_USEHEADERTITLE : PSP_USEHEADERSUBTITLE;

                // force redrawing of the titles
                if (uPage == (UINT)ppd->nCurItem)
                {
                    RECT rcHeader;
                    GetClientRect(hDlg, &rcHeader);
                    rcHeader.bottom = ppd->cyHeaderHeight;

                    InvalidateRect(hDlg, &rcHeader, FALSE);
                }
            }
        }
    }

    if (pszNewTitle)
        LocalFree((LPVOID)pszNewTitle);
}

void PropSheetPaintHeader(LPPROPDATA ppd, PISP pisp, HWND hDlg, HDC hdc)
{
    RECT rcHeader,rcHeaderBitmap;
    GetClientRect(hDlg, &rcHeader);
    rcHeader.bottom = ppd->cyHeaderHeight;

    // do we need to paint the header?
    if (ppd->psh.dwFlags & PSH_WIZARD97IE4)
    {
        // Do it the WIZARD97IE4 way

        // Bug-for-bug compatibility:  WIZARD97IE4 tested the wrong flag here
        if ((ppd->psh.dwFlags & PSH_WATERMARK) && (ppd->hbrWatermark))
            PaintWithPaletteBrush(hdc, &rcHeader, ppd->hplWatermark, ppd->hbrHeader);
        SetBkMode(hdc, TRANSPARENT);
    }
    else
    {
        // Do it the WIZARD97IE5 way
        if ((ppd->psh.dwFlags & PSH_HEADER) && (ppd->hbmHeader))
        {
            // compute the rectangle for the bitmap depending on the size of the header
            int bx = RECTWIDTH(rcHeader) - HEADERBITMAP_CXBACK;
            ASSERT(bx > 0);
            FillRect(hdc, &rcHeader, g_hbrWindow);
            SetRect(&rcHeaderBitmap, bx, HEADERBITMAP_Y, bx + HEADERBITMAP_WIDTH, HEADERBITMAP_Y + HEADERBITMAP_HEIGHT);
            PaintWithPaletteBitmap(hdc, &rcHeaderBitmap, ppd->hplWatermark, ppd->hbmHeader);
            SetBkColor(hdc, g_clrWindow);
            SetTextColor(hdc, g_clrWindowText);
        }
        else
            SendMessage(hDlg, WM_CTLCOLORSTATIC, (WPARAM)hdc, (LPARAM)hDlg);
    }

    //
    //  WIZARD97IE5 subtracts out the space used by the header bitmap.
    //  WIZARD97IE4 uses the full width since the header bitmap
    //  in IE4 is a watermark and occupies no space.
    //
    if (!(ppd->psh.dwFlags & PSH_WIZARD97IE4))
        rcHeader.right -= HEADERBITMAP_CXBACK + HEADERSUBTITLE_WRAPOFFSET;

    ASSERT(rcHeader.right);

    if (HASHEADERTITLE(pisp))
        _WriteHeaderTitle(ppd, hdc, &rcHeader, pisp->_psp.pszHeaderTitle,
                          TRUE, DRAWTEXT_WIZARD97FLAGS);

    if (HASHEADERSUBTITLE(pisp))
        _WriteHeaderTitle(ppd, hdc, &rcHeader, pisp->_psp.pszHeaderSubTitle,
                          FALSE, DRAWTEXT_WIZARD97FLAGS);
}

// Free the title if we need to
void Prsht_FreeTitle(LPPROPDATA ppd)
{
    if (ppd->fFlags & PD_FREETITLE) {
        ppd->fFlags &= ~PD_FREETITLE;
        if (!IS_INTRESOURCE(ppd->psh.pszCaption)) {
            LocalFree((LPVOID)ppd->psh.pszCaption);
        }
    }
}

//
//  pfnStrDup is the function that converts lParam into a native character
//  set string.  (Either StrDup or StrDup_AtoW).
//
void Prsht_OnSetTitle(LPPROPDATA ppd, WPARAM wParam, LPARAM lParam, STRDUPPROC pfnStrDup)
{
    LPTSTR pszTitle;

    //
    //  The ppd->psh.pszCaption is not normally LocalAlloc()d; it's
    //  just a pointer copy.  But if the app does a PSM_SETTITLE,
    //  then all of a sudden it got LocalAlloc()d and needs to be
    //  freed.  PD_FREETITLE is the flag that tell us that this has
    //  happened.
    //

    if (IS_INTRESOURCE(lParam)) {
        pszTitle = (LPTSTR)lParam;
    } else {
        pszTitle = pfnStrDup((LPTSTR)lParam);
    }

    if (pszTitle) {
        Prsht_FreeTitle(ppd);           // Free old title if necessary

        ppd->psh.pszCaption = pszTitle;
        ppd->fFlags |= PD_FREETITLE;    // Need to free this

        ppd->psh.dwFlags = ((((DWORD)wParam) & PSH_PROPTITLE) | (ppd->psh.dwFlags & ~PSH_PROPTITLE));
        _SetTitle(ppd->hDlg, ppd);
    }
}

BOOL_PTR CALLBACK PropSheetDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    HWND hwndT;
    LPPROPDATA ppd = (LPPROPDATA)GetWindowLongPtr(hDlg, DWLP_USER);
    LRESULT lres;

    if (!ppd && (uMessage != WM_INITDIALOG))
        return FALSE;

    switch (uMessage)
    {
        case WM_INITDIALOG:
            InitPropSheetDlg(hDlg, (LPPROPDATA)lParam);
            return FALSE;

            // REVIEW for dealing with hotkeys.
            // BUGBUG: This code might not work with 32-bit WM_SYSCOMMAND msgs.
        case WM_SYSCOMMAND:
            if (wParam == SC_HOTKEY)
                return HandleHotkey(lParam);
            else if (wParam == SC_CLOSE)
            {
                UINT id = IDCLOSE;

                if (IS_WIZARD(ppd))
                    id = IDCANCEL;
                else if (ppd->fFlags & PD_CANCELTOCLOSE)
                    id = IDOK;

                // system menu close should be IDCANCEL, but if we're in the
                // PSM_CANCELTOCLOSE state, treat it as an IDOK (ie, "Close").
                return Prsht_OnCommand(ppd, id, NULL, 0);
            }

            return FALSE;      // Let default process happen

        case WM_NCDESTROY:
            {
                int iPage;

                ASSERT(GetDlgItem(hDlg, IDD_PAGELIST) == NULL);

                ppd->hwndTabs = NULL;

                // NOTE: all of the hwnds for the pages must be destroyed by now!

                // Release all page objects in REVERSE ORDER so we can have
                // pages that are dependant on eachother based on the initial
                // order of those pages
                //
                for (iPage = ppd->psh.nPages - 1; iPage >= 0; iPage--)
                {
                    DestroyPropertySheetPage(GETHPAGE(ppd, iPage));
                }
                // hwndCurPage is no longer valid from here on
                ppd->hwndCurPage = NULL;

                // If we are modeless, we need to free our ppd.  If we are modal,
                // we let _RealPropertySheet free it since one of our pages may
                // set the restart flag during DestroyPropertySheetPage above.
                if (ppd->psh.dwFlags & PSH_MODELESS)
                {
                    LocalFree(ppd);
                }
            }
            //
            // NOTES:
            //  Must return FALSE to avoid DS leak!!!
            //
            return FALSE;

        case WM_DESTROY:
            {
                // Destroy the image list we created during our init call.
                HIMAGELIST himl = TabCtrl_GetImageList(ppd->hwndTabs);
                if (himl)
                    ImageList_Destroy(himl);

                if (ppd->psh.dwFlags & PSH_WIZARD97)
                {

                    // Even if the PSH_USEHBMxxxxxx flag is set, we might
                    // need to delete the bitmap if we had to create a
                    // stretched copy.

                    if (ppd->psh.dwFlags & PSH_WATERMARK)
                    {
                        if ((!(ppd->psh.dwFlags & PSH_USEHBMWATERMARK) ||
                            ppd->hbmWatermark != ppd->psh.H_hbmWatermark) &&
                            ppd->hbmWatermark)
                            DeleteObject(ppd->hbmWatermark);

                        if (!(ppd->psh.dwFlags & PSH_USEHPLWATERMARK) &&
                            ppd->hplWatermark)
                            DeleteObject(ppd->hplWatermark);

                        if (ppd->hbrWatermark)
                            DeleteObject(ppd->hbrWatermark);
                    }

                    if ((ppd->psh.dwFlags & PSH_HEADER) && ppd->psh.H_hbmHeader)
                    {
                        if ((!(ppd->psh.dwFlags & PSH_USEHBMHEADER) ||
                            ppd->hbmHeader != ppd->psh.H_hbmHeader) &&
                            ppd->hbmHeader)
                        {
                            ASSERT(ppd->hbmHeader != ppd->hbmWatermark);
                            DeleteObject(ppd->hbmHeader);
                        }

                        if (ppd->hbrHeader)
                        {
                            ASSERT(ppd->hbrHeader != ppd->hbrWatermark);
                            DeleteObject(ppd->hbrHeader);
                        }
                    }

                    if (ppd->hFontBold)
                        DeleteObject(ppd->hFontBold);
                }

                if ((ppd->psh.dwFlags & PSH_USEICONID) && ppd->psh.H_hIcon)
                    DestroyIcon(ppd->psh.H_hIcon);

                Prsht_FreeTitle(ppd);
            }

            break;

        case WM_ERASEBKGND:
            return ppd->fFlags & PD_NOERASE;
            break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            PISP pisp;

            hdc = BeginPaint(hDlg, &ps);
            // (dli) paint the header
            if ((ppd->psh.dwFlags & PSH_WIZARD97) &&
                (!((pisp = GETPISP(ppd, ppd->nCurItem))->_psp.dwFlags & PSP_HIDEHEADER)))
            {
                PropSheetPaintHeader(ppd, pisp, hDlg, hdc);
            }

            if (ps.fErase) {
                SendMessage (hDlg, WM_ERASEBKGND, (WPARAM) hdc, 0);
            }

            EndPaint(hDlg, &ps);
        }
        break;

        case WM_COMMAND:
            // Cannot use HANDLE_WM_COMMAND, because we want to pass a result!
            return Prsht_OnCommand(ppd, GET_WM_COMMAND_ID(wParam, lParam),
                                   GET_WM_COMMAND_HWND(wParam, lParam),
                                   GET_WM_COMMAND_CMD(wParam, lParam));

        case WM_NOTIFY:
            switch (((NMHDR FAR *)lParam)->code)
            {
                case TCN_SELCHANGE:
                    PageChange(ppd, 1);
                    break;

                case TCN_SELCHANGING:
                {
                    lres = PageChanging(ppd);
                    if (!lres) {
                        SetWindowPos(ppd->hwndCurPage, HWND_BOTTOM, 0,0,0,0, SWP_NOACTIVATE | SWP_NOSIZE |SWP_NOMOVE);
                    }
                    goto ReturnLres;
                }
                break;

                case TCN_GETOBJECT:
                {
                    LPNMOBJECTNOTIFY lpnmon = (LPNMOBJECTNOTIFY)lParam;

                    lpnmon->hResult = Prsht_GetObject(ppd, hDlg, lpnmon->iItem,
                        lpnmon->piid, &lpnmon->pObject);
                }
                break;

                default:
                    return FALSE;
            }
            return TRUE;

        case PSM_SETWIZBUTTONS:
            SetWizButtons(ppd, lParam);
            break;

#ifdef UNICODE
        case PSM_SETFINISHTEXTA:
#endif
        case PSM_SETFINISHTEXT:
        {
            HWND    hFinish = GetDlgItem(hDlg, IDD_FINISH);
            HWND hwndFocus = GetFocus();
            HWND hwnd;
            BOOL fSetFocus = FALSE;

            if (!(ppd->psh.dwFlags & PSH_WIZARDHASFINISH)) {
                hwnd = GetDlgItem(hDlg, IDD_NEXT);
                if (hwnd == hwndFocus)
                    fSetFocus = TRUE;
                ShowWindow(hwnd, SW_HIDE);
            }

            hwnd = GetDlgItem(hDlg, IDD_BACK);
            if (hwnd == hwndFocus)
                fSetFocus = TRUE;
            ShowWindow(hwnd, SW_HIDE);

            if (lParam) {
#ifdef UNICODE
                if (uMessage == PSM_SETFINISHTEXTA) {
                    SetWindowTextA(hFinish, (LPSTR)lParam);
                } else
#endif
                    Button_SetText(hFinish, (LPTSTR)lParam);
            }
            ShowWindow(hFinish, SW_SHOW);
            Button_Enable(hFinish, TRUE);
            ResetWizButtons(ppd);
            SendMessage(hDlg, DM_SETDEFID, IDD_FINISH, 0);
            ppd->idDefaultFallback = IDD_FINISH;
            if (fSetFocus)
#ifdef WM_NEXTDLGCTL_WORKS
                SetDlgFocus(ppd, hFinish);
#else
                SetFocus(hFinish);
#endif
        }
        break;

#ifdef UNICODE
        case PSM_SETTITLEA:
            Prsht_OnSetTitle(ppd, wParam, lParam, StrDup_AtoW);
            break;
#endif

        case PSM_SETTITLE:
            Prsht_OnSetTitle(ppd, wParam, lParam, StrDup);
            break;

#ifdef UNICODE
        case PSM_SETHEADERTITLEA:
        {
            LPWSTR lpHeaderTitle = (lParam && HIWORD(lParam)) ?
                                   ProduceWFromA(CP_ACP, (LPCSTR)lParam) : StrDupW((LPWSTR)lParam);
            if (lpHeaderTitle) 
                _SetHeaderTitles(hDlg, ppd, (UINT)wParam, lpHeaderTitle, TRUE); 
        }
        break;
#endif
        case PSM_SETHEADERTITLE:
        {
            LPTSTR lpHeaderTitle = StrDup((LPCTSTR)lParam);
            if (lpHeaderTitle) 
                _SetHeaderTitles(hDlg, ppd, (UINT)wParam, lpHeaderTitle, TRUE); 
        }
        break;
            
#ifdef UNICODE
        case PSM_SETHEADERSUBTITLEA:
        {
            LPWSTR lpHeaderSubTitle = (lParam && HIWORD(lParam)) ?
                                   ProduceWFromA(CP_ACP, (LPCSTR)lParam) : StrDupW((LPWSTR)lParam);
            if (lpHeaderSubTitle) 
                _SetHeaderTitles(hDlg, ppd, (UINT)wParam, lpHeaderSubTitle, FALSE); 
        }
        break;
#endif
        case PSM_SETHEADERSUBTITLE:
        {
            LPTSTR lpHeaderSubTitle = StrDup((LPCTSTR)lParam);
            if (lpHeaderSubTitle) 
                _SetHeaderTitles(hDlg, ppd, (UINT)wParam, lpHeaderSubTitle, FALSE); 
        }
        break;
            
        case PSM_CHANGED:
            PageInfoChange(ppd, (HWND)wParam);
            break;

        case PSM_RESTARTWINDOWS:
            ppd->nRestart |= ID_PSRESTARTWINDOWS;
            break;

        case PSM_REBOOTSYSTEM:
            ppd->nRestart |= ID_PSREBOOTSYSTEM;
            break;

        case PSM_DISABLEAPPLY:
            // the page is asking us to gray the "Apply" button and not let
            // anyone else re-enable it
            if (ppd->fAllowApply)
            {
                ppd->fAllowApply = FALSE;
                EnableWindow(GetDlgItem(ppd->hDlg, IDD_APPLYNOW), FALSE);
            }
            break;

        case PSM_ENABLEAPPLY:
            // the page is asking us to allow the the "Apply" button to be
            // once again enabled
            if (!ppd->fAllowApply)
                ppd->fAllowApply = TRUE;
            // BUGBUG - raymondc - shouldn't we call EnableWindow?
            break;

        case PSM_CANCELTOCLOSE:
            if (!(ppd->fFlags & PD_CANCELTOCLOSE))
            {
                TCHAR szClose[20];
                ppd->fFlags |= PD_CANCELTOCLOSE;
                LocalizedLoadString(IDS_CLOSE, szClose, ARRAYSIZE(szClose));
                SetDlgItemText(hDlg, IDOK, szClose);
                EnableWindow(GetDlgItem(hDlg, IDCANCEL), FALSE);
            }
            break;

        case PSM_SETCURSEL:
            lres = PageSetSelection(ppd, (int)wParam, (HPROPSHEETPAGE)lParam, 1);
            goto ReturnLres;

        case PSM_SETCURSELID:
        {
            int iPageIndex;

            iPageIndex =  FindPageIndex(ppd, ppd->nCurItem, (DWORD)lParam, 1);

            if (iPageIndex == -1)
                lres = 0;
            else
                lres = PageSetSelection(ppd, iPageIndex, NULL, 1);
            goto ReturnLres;
        }
        break;

        case PSM_REMOVEPAGE:
            RemovePropPage(ppd, (int)wParam, (HPROPSHEETPAGE)lParam);
            break;

        case PSM_ADDPAGE:
            lres = AddPropPage(ppd,(HPROPSHEETPAGE)lParam);
            goto ReturnLres;

        case PSM_INSERTPAGE:
            lres = InsertPropPage(ppd, (HPROPSHEETPAGE)wParam, (HPROPSHEETPAGE)lParam);
            goto ReturnLres;

        case PSM_QUERYSIBLINGS:
            lres = QuerySiblings(ppd, wParam, lParam);
            goto ReturnLres;

        case PSM_UNCHANGED:
            PageInfoUnChange(ppd, (HWND)wParam);
            break;

        case PSM_APPLY:
            // a page is asking us to simulate an "Apply Now".
            // let the page know if we're successful
            lres = ButtonPushed(ppd, IDD_APPLYNOW);
            goto ReturnLres;

        case PSM_GETTABCONTROL:
            lres = (LRESULT)ppd->hwndTabs;
            goto ReturnLres;

        case PSM_GETCURRENTPAGEHWND:
            lres = (LRESULT)ppd->hwndCurPage;
            goto ReturnLres;

        case PSM_PRESSBUTTON:
            if (wParam <= PSBTN_MAX)
            {
                const static int IndexToID[] = {IDD_BACK, IDD_NEXT, IDD_FINISH, IDOK,
                IDD_APPLYNOW, IDCANCEL, IDHELP};
                Prsht_OnCommand(ppd, IndexToID[wParam], NULL, 0);
            }
            break;

        case PSM_ISDIALOGMESSAGE:
            // returning TRUE means we handled it, do a continue
            // FALSE do standard translate/dispatch
            lres = Prop_IsDialogMessage(ppd, (LPMSG32)lParam);
            goto ReturnLres;

        case PSM_HWNDTOINDEX:
            lres = FindItem(ppd->hwndTabs, (HWND)wParam, NULL);
            goto ReturnLres;

        case PSM_INDEXTOHWND:
            if ((UINT)wParam < ppd->psh.nPages)
                lres = (LRESULT)_Ppd_GetPage(ppd, (int)wParam);
            else
                lres = 0;
            goto ReturnLres;

        case PSM_PAGETOINDEX:
            lres = FindPageIndexByHpage(ppd, (HPROPSHEETPAGE)lParam);
            goto ReturnLres;

        case PSM_INDEXTOPAGE:
            if ((UINT)wParam < ppd->psh.nPages)
                lres = (LRESULT)GETHPAGE(ppd, wParam);
            else
                lres = 0;
            goto ReturnLres;

        case PSM_INDEXTOID:
            if ((UINT)wParam < ppd->psh.nPages)
            {
                lres = (LRESULT)GETPPSP(ppd, wParam)->P_pszTemplate;

                // Need to be careful -- return a value only if pszTemplate
                // is an ID.  Don't return out our internal pointers!
                if (!IS_INTRESOURCE(lres))
                    lres = 0;
            }
            else
                lres = 0;
            goto ReturnLres;

        case PSM_IDTOINDEX:
            lres = FindPageIndex(ppd, ppd->nCurItem, (DWORD)lParam, 0);
            goto ReturnLres;

        case PSM_GETRESULT:
            // This is valid only after the property sheet is gone
            if (ppd->hwndCurPage)
            {
                lres = -1;      // Stupid - you shouldn't be calling me yet
            } else {
                lres = ppd->nReturn;
                if (lres > 0 && ppd->nRestart)
                    lres = ppd->nRestart;
            }
            goto ReturnLres;
            break;

        case PSM_RECALCPAGESIZES:
            lres = Prsht_RecalcPageSizes(ppd);
            goto ReturnLres;

            // these should be relayed to all created dialogs
        case WM_WININICHANGE:
        case WM_SYSCOLORCHANGE:
        case WM_DISPLAYCHANGE:
            {
                int nItem, nItems = TabCtrl_GetItemCount(ppd->hwndTabs);
                for (nItem = 0; nItem < nItems; ++nItem)
                {

                    hwndT = _Ppd_GetPage(ppd, nItem);
                    if (hwndT)
                        SendMessage(hwndT, uMessage, wParam, lParam);
                }
                SendMessage(ppd->hwndTabs, uMessage, wParam, lParam);
            }
            break;

            //
            // send toplevel messages to the current page and tab control
            //
        case WM_PALETTECHANGED:
            //
            // If this is our window we need to avoid selecting and realizing
            // because doing so would cause an infinite loop between WM_QUERYNEWPALETTE
            // and WM_PALETTECHANGED.
            //
            if((HWND)wParam == hDlg) {
                return(FALSE);
            }
            //
            // FALL THROUGH
            //

        case WM_QUERYNEWPALETTE:
            // This is needed when another window which has different palette clips
            // us
            if ((ppd->psh.dwFlags & PSH_WIZARD97) &&
                (ppd->psh.dwFlags & PSH_WATERMARK) &&
                (ppd->psh.hplWatermark))
                HandlePaletteChange(ppd, uMessage, hDlg);

            //
            // FALL THROUGH
            //

        case WM_ENABLE:
        case WM_DEVICECHANGE:
        case WM_QUERYENDSESSION:
        case WM_ENDSESSION:
            if (ppd->hwndTabs)
                SendMessage(ppd->hwndTabs, uMessage, wParam, lParam);
            //
            // FALL THROUGH
            //

        case WM_ACTIVATEAPP:
        case WM_ACTIVATE:
            {
                hwndT = _Ppd_GetPage(ppd, ppd->nCurItem);
                if (hwndT && IsWindow(hwndT))
                {
                    //
                    // By doing this, we are "handling" the message.  Therefore
                    // we must set the dialog return value to whatever the child
                    // wanted.
                    //
                    lres = SendMessage(hwndT, uMessage, wParam, lParam);
                    goto ReturnLres;
                }
            }

            if ((uMessage == WM_PALETTECHANGED) || (uMessage == WM_QUERYNEWPALETTE))
                return TRUE;
            else
                return FALSE;

        case WM_CONTEXTMENU:
            // ppd->hwndTabs is handled by aPropHelpIDs to work around a USER bug.
            // See aPropHelpIDs for gory details.
            if ((ppd->hwndCurPage != (HWND)wParam) && (!IS_WIZARD(ppd)))
                WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID) aPropHelpIDs);
            break;

        case WM_HELP:
            hwndT = (HWND)((LPHELPINFO)lParam)->hItemHandle;
            if ((GetParent(hwndT) == hDlg) && (hwndT != ppd->hwndTabs))
                WinHelp(hwndT, NULL, HELP_WM_HELP, (ULONG_PTR)(LPVOID) aPropHelpIDs);
            break;

        default:
            return FALSE;
       }
    return TRUE;

ReturnLres:
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lres);
    return TRUE;

}

//
//  Draw the background for wizard pages.
//
BOOL Prsht_EraseWizBkgnd(LPPROPDATA ppd, HDC hdc)
{
    RECT rc;
    BOOL fPainted = FALSE;
    GetClientRect(ppd->hDlg, &rc);

    if (ppd->psh.dwFlags & PSH_WIZARD97IE4)
    {
        if (ppd->hbrWatermark)
        {
            PaintWithPaletteBrush(hdc, &rc, ppd->hplWatermark, ppd->hbrWatermark);
            fPainted = TRUE;
        }
    }
    else                                // PSH_WIZARD97IE5
    {
        if (ppd->hbmWatermark)
        {
            // Right-hand side gets g_hbrWindow.
            rc.left = BITMAP_WIDTH;
            FillRect(hdc, &rc, g_hbrWindow);

            // Left-hand side gets watermark in top portion with autofill...
            rc.right = rc.left;
            rc.left = 0;
            PaintWithPaletteBitmap(hdc, &rc, ppd->hplWatermark, ppd->hbmWatermark);
            fPainted = TRUE;
        }
    }
    return fPainted;
}

LRESULT CALLBACK WizardWndProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam, UINT_PTR uID, ULONG_PTR dwRefData)
{
    LPPROPDATA ppd = (LPPROPDATA)dwRefData;
    switch (uMessage)
    {
        case WM_ERASEBKGND:
            if (Prsht_EraseWizBkgnd(ppd, (HDC)wParam))
                return TRUE;
            break;

        // Only PSH_WIZARD97IE4 cares about these messages
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORDLG:
            if (!(ppd->psh.dwFlags & PSH_WIZARD97IE4))
                break;
            // fall through

        case WM_CTLCOLOR:
        case WM_CTLCOLORMSGBOX:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
            if (ppd->psh.dwFlags & PSH_WIZARD97IE4)
            {
              if (ppd->hbrWatermark) {
                POINT pt;
                // Bug-for-bug compatibility:  TRANSPARENT screws up edit
                // controls when they scroll, but that's what IE4 did.
                SetBkMode((HDC)wParam, TRANSPARENT);

                if (ppd->hplWatermark)
                {
                    SelectPalette((HDC)wParam, ppd->hplWatermark, FALSE);
                    RealizePalette((HDC)wParam);
                }
                UnrealizeObject(ppd->hbrWatermark);
                GetDCOrgEx((HDC)wParam, &pt);
                // Bug-for-bug compatibility:  We shouldn't use GetParent
                // because the notification might be forwarded up from an
                // embedded dialog child, but that's what IE4 did.
                ScreenToClient(GetParent((HWND)lParam), &pt);
                SetBrushOrgEx((HDC)wParam, -pt.x, -pt.y, NULL);
                return (LRESULT)(HBRUSH)ppd->hbrWatermark;
              }
            }
            else                        // PSH_WIZARD97IE5
            {
                if (ppd->hbmWatermark)
                {
                    LRESULT lRet = DefWindowProc(hDlg, uMessage, wParam, lParam);
                    if (lRet == DefSubclassProc(hDlg, uMessage, wParam, lParam))
                    {
                        SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
                        SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
                        return (LRESULT)g_hbrWindow;
                    }
                    else
                        return lRet;
                }
            }
            break;

        case WM_PALETTECHANGED:
            if((HWND)wParam == hDlg)
                return(FALSE);

        case WM_QUERYNEWPALETTE:
            HandlePaletteChange(ppd, uMessage, hDlg);
            return TRUE;

        case WM_DESTROY:
            // Clean up subclass
            RemoveWindowSubclass(hDlg, WizardWndProc, 0);
            break;

        default:
            break;
    }

    return DefSubclassProc(hDlg, uMessage, wParam, lParam);
}

//
// EnumResLangProc
//
// purpose: a callback function for EnumResourceLanguages().
//          look into the type passed in and if it is RT_DIALOG
//          copy the lang of the first resource to our buffer
//          this also counts # of lang if more than one of them
//          are passed in
//
//
typedef struct  {
    WORD wLang;
    BOOL fFoundLang;
    LPCTSTR lpszType;
} ENUMLANGDATA;

BOOL CALLBACK EnumResLangProc(HINSTANCE hinst, LPCTSTR lpszType, LPCTSTR lpszName, WORD wIdLang, LPARAM lparam)
{
    ENUMLANGDATA *pel = (ENUMLANGDATA *)lparam;
    BOOL fContinue = TRUE;

    ASSERT(pel);

    if (lpszType == pel->lpszType)
    {
        // When comctl's been initialized with a particular MUI language,
        // we pass in the langid to GetPageLanguage(), then it's given to this proc.
        // we want to look for a template that matches to the langid,
        // and if it's not found, we have to use the first instance of templates.
        // 
        if (pel->wLang == MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)
            || (pel->wLang == wIdLang))
        {
            pel->wLang = wIdLang;
            pel->fFoundLang = TRUE;
            fContinue = FALSE; 
        }
    }
    return fContinue;   // continue until we get langs...
}

// GetPageLanguage
//
// purpose: tries to retrieve language information out of
//          given page's dialog template. We get the first language
//          in which the template is localized in.
//          currently doesn't support PSP_DLGINDIRECT case
//
// BUGBUG REVIEW: we luck out with browselc since there's only one lang per resid,
// we should cache the langid we loaded up front and pull it out here.
//
HRESULT GetPageLanguage(PISP pisp, WORD *pwLang)
{
    if (pisp && pwLang
#ifndef WINNT
                       && !(pisp->_psp.dwFlags & PSP_IS16)
#endif
                                                          )
    {
        if (pisp->_psp.dwFlags & PSP_DLGINDIRECT)
        {
            // try something other than dialog
            return E_FAIL; // not supported yet.
        }
        else
        {
            ENUMLANGDATA el;
            
            // the caller passes-in the langid with which we're initialized
            //
            el.wLang = *pwLang;
            el.fFoundLang = FALSE;
            el.lpszType = RT_DIALOG;
            // check with the dialog template specified
            EnumResourceLanguages(pisp->_psp.hInstance, RT_DIALOG, pisp->_psp.P_pszTemplate, EnumResLangProc, (LPARAM)&el);
            if (!el.fFoundLang)
            {
                // we couldn't find a matching lang in the given page's resource
                // so we'll take the first one
                el.wLang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
                
                // it doesn't matter if this fails, because we'll then end up with 
                // the neutral langid, which is the best guess here after failing 
                // to get any page lang.
                //
                EnumResourceLanguages(pisp->_psp.hInstance, RT_DIALOG, 
                                      pisp->_psp.P_pszTemplate, EnumResLangProc, (LPARAM)&el);
            }
            *pwLang = el.wLang;
        }
        return S_OK;
    }
    return E_FAIL;
}

//
//  FindResourceExRetry
//
//  Just like FindResourceEx, except that if we can't find the resource,
//  we try again with MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL).
//
HRSRC FindResourceExRetry(HMODULE hmod, LPCTSTR lpType, LPCTSTR lpName, WORD wLang)
{
    HRSRC hrsrc = FindResourceEx(hmod, lpType, lpName, wLang);

    // if failed because we couldn't find the resouce in requested lang
    // and requested lang wasn't neutral, then try neutral.
    if (!hrsrc && wLang != MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL))
    {
        wLang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
        hrsrc = FindResourceEx(hmod, lpType, lpName, wLang);
    }


    return hrsrc;
}


WORD GetShellResourceLangID(void);

// NT5_GetUserDefaultUILanguage
//
//  NT5 has a new function GetUserDefaultUILanguage which returns the
//  language the user as selected for UI.
//
//  If the function is not available (e.g., NT4), then use the
//  shell resource language ID.
//

typedef LANGID (CALLBACK* GETUSERDEFAULTUILANGUAGE)(void);

GETUSERDEFAULTUILANGUAGE _GetUserDefaultUILanguage;

LANGID NT5_GetUserDefaultUILanguage(void)
{
    if (_GetUserDefaultUILanguage == NULL)
    {
        HMODULE hmod = GetModuleHandle(TEXT("KERNEL32"));

        //
        //  Must keep in a local to avoid thread races.
        //
        GETUSERDEFAULTUILANGUAGE pfn = NULL;

        if (hmod)
            pfn = (GETUSERDEFAULTUILANGUAGE)
                    GetProcAddress(hmod, "GetUserDefaultUILanguage");

        //
        //  If function is not available, then use our fallback
        //
        if (pfn == NULL)
            pfn = GetShellResourceLangID;

        ASSERT(pfn != NULL);
        _GetUserDefaultUILanguage = pfn;
    }

    return _GetUserDefaultUILanguage();
}


LCID CCGetSystemDefaultThreadLocale(LCID iLcidThreadOrig)
{
    UINT uLangThread, uLangThreadOrig;

    uLangThreadOrig = LANGIDFROMLCID(iLcidThreadOrig);

    

    // uLangThread is the language we think we want to use
    uLangThread = uLangThreadOrig;

    if (staticIsOS(OS_NT4) && !staticIsOS(OS_NT5))
    {
        int iLcidUserDefault = GetUserDefaultLCID();
        UINT uLangUD = LANGIDFROMLCID(iLcidUserDefault);

        //
        // If we are running on Enabled Arabic NT4, we should always
        // display the US English resources (since the UI is English), however NT4
        // Resource Loader will look for the current Thread Locale (which is Arabic).
        // This is no problem in NT5 since the Resource Loader will check for
        // the  UI Language (newly introduced) when loading such resources. To
        // fix this, we will change the thread locale to US English
        // and restore it back to Arabic/Hebrew if we are running on an Enabled Arabic/Hebrew NT4.
        // The check is done to make sure we are running within a Araic/Hebrew user locale
        // and the thread locale is still Arabic/Hebrew (i.e. nobody tried to SetThreadLocale).
        // [samera]
        //
        if( ((PRIMARYLANGID(uLangUD    ) == LANG_ARABIC) &&
             (PRIMARYLANGID(uLangThread) == LANG_ARABIC))   ||
            ((PRIMARYLANGID(uLangUD    ) == LANG_HEBREW) &&
             (PRIMARYLANGID(uLangThread) == LANG_HEBREW)))
        {
            uLangThread = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
        }
    }

    //
    //  Make locale match UI locale if not otherwise overridden.
    //
    if (uLangThread == uLangThreadOrig)
    {
        uLangThread = NT5_GetUserDefaultUILanguage();
    }

    //
    //  Now see if we actually changed the thread language.
    //
    if (uLangThread == uLangThreadOrig)
    {
        // No change, return the original locale, including sort stuff
        return iLcidThreadOrig;
    }
    else
    {
        // It changed, return a generic sort order, since we don't use
        // this information for sorting.
        return MAKELCID(uLangThread, SORT_DEFAULT);
    }
}

//
// GetAltFontLangId
// 
// used to detect "MS UI Gothic" on Jpn localized non NT5 platforms
// the font is shipped with IE5 for the language but comctl can't 
// always assume the font so we have a fake sublang id assigned to
// the secondary resource file for the language
//
int CALLBACK FontEnumProc(
  ENUMLOGFONTEX *lpelfe,    
  NEWTEXTMETRICEX *lpntme,  
  int FontType,             
  LPARAM lParam
)
{
    if (lParam)
    {
        *(BOOL *)lParam = TRUE;
    }
    return 0; // stop at the first callback
}
UINT GetDefaultCharsetFromLang(LANGID wLang)
{
    TCHAR    szData[6+1]; // 6 chars are max allowed for this lctype
    UINT     uiRet = DEFAULT_CHARSET;

    // JPN hack here: GetLocaleInfo() DOES return > 0 for Jpn altfont langid,
    // but doesn't get us any useful info. So for JPN, we ripout the SUBLANG
    // portion of id. we can't do this for other langs since sublang can affect
    // charset (ex. chinese)
    //
    if(PRIMARYLANGID(wLang) == LANG_JAPANESE)
        wLang = MAKELANGID(PRIMARYLANGID(wLang), SUBLANG_NEUTRAL);
    
    if (GetLocaleInfo(MAKELCID(wLang, SORT_DEFAULT), 
                      LOCALE_IDEFAULTANSICODEPAGE,
                      szData, ARRAYSIZE(szData)) > 0)
    {

        UINT uiCp = StrToInt(szData);
        CHARSETINFO   csinfo;

        if (TranslateCharsetInfo((DWORD *)uiCp, &csinfo, TCI_SRCCODEPAGE))
            uiRet = csinfo.ciCharset;
    }

    return uiRet;
}
BOOL IsFontInstalled(LANGID wLang, LPCTSTR szFace)
{
    BOOL     fInstalled = FALSE;
    HDC      hdc;
    LOGFONT  lf = {0};

    lstrcpyn(lf.lfFaceName, szFace, ARRAYSIZE(lf.lfFaceName));
    
    // retrieve charset from given language
    lf.lfCharSet = (BYTE)GetDefaultCharsetFromLang(wLang);
    
    // then see if we can enumrate the font
    hdc = GetDC(NULL);
    if (hdc)
    {
        EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)FontEnumProc, (LPARAM)&fInstalled, 0);
        ReleaseDC(NULL, hdc);
    }

     return fInstalled;
}

LANGID GetAltFontLangId(LANGID wLang)
{
     LPCTSTR pszTypeFace = NULL;
     USHORT  usAltSubLang = SUBLANG_NEUTRAL;
     const static TCHAR s_szUIGothic[] = TEXT("MS UI Gothic");
     static int iPrimaryFontInstalled = -1;

     // most of the case we return the lang just as is
     switch(PRIMARYLANGID(wLang))
     {
         case LANG_JAPANESE:
             pszTypeFace = s_szUIGothic;
             usAltSubLang   = SUBLANG_JAPANESE_ALTFONT;
             break;
         // add code here to handle any other cases like Jpn
         default:
             return wLang;
     }

     // check existence of the font if we haven't
     if (iPrimaryFontInstalled < 0 && pszTypeFace)
     {
        iPrimaryFontInstalled = IsFontInstalled(wLang, pszTypeFace);
     }

     // return secondary lang id if our alternative font *is* installed
     if (iPrimaryFontInstalled == 1) 
         wLang = MAKELANGID(PRIMARYLANGID(wLang), usAltSubLang);

     return wLang;
}
// GetShellResourceLangID
//
// On NT4, we want to match our ML resource to the one that OS is localized.
// this is to prevent general UI (buttons) from changing along with regional
// setting change.
// Win95 won't change system default locale, NT5 will load from matching satelite
// resource dll automatically so this won't be needed on these platforms.
// This function finds shell32.dll and gets the language in which the dll is
// localized, then cache the lcid so we won't have to detect it again.
//
WORD GetShellResourceLangID(void)
{
    static WORD langRes = 0L;

    // we do this only once
    if (langRes == 0L)
    {
        HINSTANCE hinstShell;
        ENUMLANGDATA el = {MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), FALSE, RT_DIALOG};

        hinstShell = LoadLibrary(TEXT("shell32.dll"));
        if (hinstShell)
        {
            EnumResourceLanguages(hinstShell, RT_DIALOG, MAKEINTRESOURCE(DLG_EXITWINDOWS), EnumResLangProc, (LPARAM)&el);

            FreeLibrary(hinstShell);
        }

        if (PRIMARYLANGID(el.wLang) == LANG_CHINESE
           || PRIMARYLANGID(el.wLang) == LANG_PORTUGUESE )
        {
            // these two languages need special handling
            langRes = el.wLang;
        }
        else
        {
            // otherwise we use only primary langid.
            langRes = MAKELANGID(PRIMARYLANGID(el.wLang), SUBLANG_NEUTRAL);
        }
    }
    return langRes;
}

//
//  CCGetProperThreadLocale
//
//  This function computes its brains out and tries to decide
//  which thread locale we should use for our UI components.
//
//  Returns the desired locale.
//
//  Adjustment - For Arabic / Hebrew - NT4 Only
//
//      Converts the thread locale to US, so that neutral resources
//      loaded by the thread will be the US-English one, if available.
//      This is used when the locale is Arabic/Hebrew and the system is
//      NT4 enabled ( There was no localized NT4), as a result we need
//      always to see the English resources on NT4 Arabic/Hebrew.
//      [samera]
//
//  Adjustment - For all languages - NT4 Only
//
//      Convert the thread locale to the shell locale if not otherwise
//      altered by previous adjustments.
//
//  Adjustment - For all languages - NT5 Only
//
//      Always use the default UI language.  If that fails, then use the
//      shell locale.
//
//  The last two adjustments are handled in a common function, because
//  the NT5 fallback turns out to be equal to the NT4 algorithm.
//
LCID CCGetProperThreadLocale(OPTIONAL LCID *plcidPrev)
{
    LANGID uLangAlt, uLangMUI;
    LCID lcidRet, iLcidThreadOrig; 

    iLcidThreadOrig = GetThreadLocale();
    if (plcidPrev)
        *plcidPrev = iLcidThreadOrig;

    uLangMUI = GetMUILanguage();
    if ( uLangMUI ==  MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL))
    {
        // return adjusted system default locale if MUI isn't initialized
        //
        lcidRet = CCGetSystemDefaultThreadLocale(iLcidThreadOrig);
    }
    else
    {
        // our host has initialized us with prefered MUI language
        // 
        lcidRet = MAKELCID(uLangMUI, SORT_DEFAULT);
    }

    uLangAlt = GetAltFontLangId(LANGIDFROMLCID(lcidRet));
    if (uLangAlt != LANGIDFROMLCID(lcidRet))
    {
        // use secondary resource for the language
        // if the platform *does* have the alternative font
        lcidRet = MAKELCID(uLangAlt, SORTIDFROMLCID(lcidRet));
    }
    
    return lcidRet;
}

//
//  CCLoadStringEx
//
//  Just like LoadString, except you can specify the language, too.
//
//  This is harder than you think, because NT5 changed the way strings
//  are loaded.  Quote:
//
//      We changed the resource loader in NT5, to only load resources
//      in the language of the thread locale, if the thread locale is
//      different to the user locale. The reasoning behind this was
//      the "random" loading of the language of the user locale in
//      the UI. This breaks if you do a SetThreadLocale to the User
//      Locale, because then the whole step is ignored and the
//      InstallLanguage of the system is loaded.
//
//  Therefore, we have to use FindResourceEx.
//
//
int CCLoadStringEx(UINT uID, LPWSTR lpBuffer, int nBufferMax, WORD wLang)
{
    return CCLoadStringExInternal(HINST_THISDLL, uID, lpBuffer, nBufferMax, wLang);
}

int CCLoadStringExInternal(HINSTANCE hInst, UINT uID, LPWSTR lpBuffer, int nBufferMax, WORD wLang)
{
    PWCHAR pwch;
    HRSRC hrsrc;
    int cwch = 0;

    if (nBufferMax <= 0) return 0;                  // sanity check

    /*
     *  String tables are broken up into "bundles" of 16 strings each.
     */

    hrsrc = FindResourceExRetry(hInst, RT_STRING,
                                (LPCTSTR)(LONG_PTR)(1 + (USHORT)uID / 16),
                                wLang);
    if (hrsrc) {
        pwch = (PWCHAR)LoadResource(hInst, hrsrc);
        if (pwch) {
            /*
             *  Now skip over the strings in the resource until we
             *  hit the one we want.  Each entry is a counted string,
             *  just like Pascal.
             */
            for (uID %= 16; uID; uID--) {
#ifndef UNIX
                pwch += *pwch + 1;
#else   // unix version has a WORD first then followed by a padding word
        // then the whole string in WCHAR (4 bytes on UNIX)
                pwch += *(WORD *)pwch + 1;
#endif
            }
#ifndef UNIX
            cwch = min(*pwch, nBufferMax - 1);
            memcpy(lpBuffer, pwch+1, cwch * sizeof(WCHAR)); /* Copy the goo */
#else   // length of the string is a WORD not WCHAR
            cwch = min(*(WORD *)pwch, nBufferMax - 1);
            memcpy(lpBuffer, pwch+1, cwch * sizeof(WCHAR)); /* Copy the goo */
#endif
        }
    }
    lpBuffer[cwch] = L'\0';                 /* Terminate the string */
    return cwch;
}


//
//  LocalizedLoadString
//
//  Loads a string from our resources, using the correct language.
//

int LocalizedLoadString(UINT uID, LPWSTR lpBuffer, int nBufferMax)
{
    return CCLoadStringEx(uID, lpBuffer, nBufferMax,
                LANGIDFROMLCID(CCGetProperThreadLocale(NULL)));
}

#ifdef WINNT
//
// Determine if the prop sheet frame should use the new
// "MS Shell Dlg 2" font.  To do this, we examine each page's dlg template.
// If all pages have SHELLFONT enabled, then
// we want to use the new font.
//
BOOL ShouldUseMSShellDlg2Font(LPPROPDATA ppd)
{
    UINT iPage;
    PAGEINFOEX pi;
    LANGID langidMUI;

    if (!staticIsOS(OS_NT5))
        return FALSE;

    langidMUI = GetMUILanguage();
    for (iPage = 0; iPage < ppd->psh.nPages; iPage++)
    {
        if (GetPageInfoEx(ppd, GETPISP(ppd, iPage), &pi, langidMUI, GPI_DIALOGEX))
        {
            if (!IsPageInfoSHELLFONT(&pi))
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}
#endif

PSPT_OS Prsht_GetOS()
{
    static PSPT_OS pspt_os = (PSPT_OS)-1;
    int iIsOSBiDiEnabled = 0;
    
    if (pspt_os != (PSPT_OS)-1)
    {
        return pspt_os;
    }


    iIsOSBiDiEnabled = GetSystemMetrics(SM_MIDEASTENABLED);
    
    if (staticIsOS(OS_NT5))
        pspt_os = PSPT_OS_WINNT5;

    else if (iIsOSBiDiEnabled && staticIsOS(OS_NT4) && (!staticIsOS(OS_NT5)))
        pspt_os = PSPT_OS_WINNT4_ENA;        

    else if (iIsOSBiDiEnabled && staticIsOS(OS_WIN95) && (!staticIsOS(OS_MEMPHIS)))
        pspt_os = PSPT_OS_WIN95_BIDI;

    else if (iIsOSBiDiEnabled && staticIsOS(OS_MEMPHIS))     
        pspt_os = PSPT_OS_WIN98_BIDI;

    else 
        pspt_os = PSPT_OS_OTHER;


    return pspt_os;
}

PSPT_OVERRIDE Prsht_GetOverrideState(LPPROPDATA ppd)
{
   // if passed bad argument, assume no override
   if(!ppd)
       return PSPT_OVERRIDE_NOOVERRIDE;
       
   if (ppd->psh.dwFlags & PSH_USEPAGELANG)
       return PSPT_OVERRIDE_USEPAGELANG;

   return PSPT_OVERRIDE_NOOVERRIDE; 
}

PSPT_TYPE Prsht_GetType(LPPROPDATA ppd, WORD wLang)
{

   PISP pisp = NULL;
   // if passed bad argument, give it the english resources
    if(!ppd)
        return PSPT_TYPE_ENGLISH;

    pisp = GETPISP(ppd, 0);
    if(pisp)
    {
        PAGEINFOEX pi = {0};

        if ((IS_PROCESS_RTL_MIRRORED()) || 
            (GetPageInfoEx(ppd, pisp, &pi, wLang, GPI_BMIRROR) && pi.bMirrored))
            return PSPT_TYPE_MIRRORED;

        else
        {
            WORD wLang = 0;
            
            GetPageLanguage(pisp,&wLang);
            if((PRIMARYLANGID(wLang) == LANG_ARABIC) || (PRIMARYLANGID(wLang) == LANG_HEBREW))
                return PSPT_TYPE_ENABLED;
        }
    }

    return PSPT_TYPE_ENGLISH;
}

PSPT_ACTION Prsht_GetAction(PSPT_TYPE pspt_type, PSPT_OS pspt_os, PSPT_OVERRIDE pspt_override)
{
    if ((pspt_type < 0) || (pspt_type >= PSPT_TYPE_MAX)
        || (pspt_os < 0) || (pspt_os >= PSPT_OS_MAX)
        || (pspt_override < 0) || (pspt_override >= PSPT_OVERRIDE_MAX))
        return PSPT_ACTION_NOACTION;

    return g_PSPT_Action[pspt_type][pspt_os][pspt_override];   

}

void Prsht_PrepareTemplate(LPPROPDATA ppd, HINSTANCE hInst, HGLOBAL *phDlgTemplate, HRSRC *phResInfo, 
                          LPCSTR lpName, HWND hWndOwner, LPWORD lpwLangID)
{

    
    LPDLGTEMPLATE pDlgTemplate = NULL;
    PSPT_ACTION pspt_action;

    if (pDlgTemplate = (LPDLGTEMPLATE)LockResource(*phDlgTemplate))
    {   

        // We save BiDi templates as DIALOG (not DIALOGEX)
        // If we got an extended template then it is not ours
        
        if (((LPDLGTEMPLATEEX)pDlgTemplate)->wSignature == 0xFFFF)
            return;

        // Cut it short to save time
        //
        if (!(pDlgTemplate->dwExtendedStyle & (RTL_MIRRORED_WINDOW | RTL_NOINHERITLAYOUT)))
           return;
    }

    pspt_action = Prsht_GetAction(Prsht_GetType(ppd, *lpwLangID), Prsht_GetOS(), 
                                              Prsht_GetOverrideState(ppd));
                                              
    switch(pspt_action)
    {
        case PSPT_ACTION_NOACTION:
            return;

        case PSPT_ACTION_NOMIRRORING:
        {
            if (pDlgTemplate)
            {   
                EditBiDiDLGTemplate(pDlgTemplate, EBDT_NOMIRROR, NULL, 0);
            }    
        }
        break;

        case PSPT_ACTION_FLIP:
        {
            if (pDlgTemplate)
            {
                EditBiDiDLGTemplate(pDlgTemplate, EBDT_NOMIRROR, NULL, 0);
                EditBiDiDLGTemplate(pDlgTemplate, EBDT_FLIP, (PWORD)&wIgnoreIDs, ARRAYSIZE(wIgnoreIDs));
                ppd->fFlipped = TRUE;
            }    
        }
        break;

        case PSPT_ACTION_LOADENGLISH:
        {
            HGLOBAL hDlgTemplateTemp = NULL;
            HRSRC hResInfoTemp;

                            //
            //Try to load an English resource.
            //
            *lpwLangID = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

            if ((hResInfoTemp = FindResourceExA( hInst, (LPCSTR)RT_DIALOG, lpName, *lpwLangID)))
            {
                hDlgTemplateTemp = LoadResource(hInst, hResInfoTemp);
            }
            if (hDlgTemplateTemp)
            {
                //
                //And return it to the caller to use it.
                // Since we loaeded a new template, we should copy it to a local memory
                // in case there is a callback.
                //
  
                DWORD   cbTemplate = SizeofResource(hInst, hResInfoTemp);
                LPVOID  pTemplateMod;

                pTemplateMod = (LPVOID)LocalAlloc(LPTR, cbTemplate * 2);
                if (pTemplateMod)
                {
                    memmove(pTemplateMod, hDlgTemplateTemp, cbTemplate);
                    LocalFree(*phDlgTemplate);
                    *phResInfo     = hResInfoTemp;
                    *phDlgTemplate = pTemplateMod;
                }
             }

        }
        break;

        case PSPT_ACTION_WIN9XCOMPAT:
        {
            if (pDlgTemplate)
            {
                pDlgTemplate->style |= DS_BIDI_RTL;
            }   
        }
    }
}


INT_PTR NEAR PASCAL _RealPropertySheet(LPPROPDATA ppd)
{
    HWND    hwndMain;
    MSG32   msg32;
    HWND    hwndTopOwner;
    int     nReturn = -1;
    HWND    hwndOriginalFocus;
    WORD    wLang, wUserLang;
    LCID    iLcidThread=0L;
    HRSRC   hrsrc = 0;
    LPVOID  pTemplate, pTemplateMod;
    LPTSTR  lpDlgId;
    if (ppd->psh.nPages == 0)
    {
        DebugMsg(DM_ERROR, TEXT("no pages for prop sheet"));
        goto FreePpdAndReturn;
    }

    ppd->hwndCurPage = NULL;
    ppd->nReturn     = -1;
    ppd->nRestart    = 0;

    hwndTopOwner = ppd->psh.hwndParent;
    hwndOriginalFocus = GetFocus();

#ifdef DEBUG
    if (GetAsyncKeyState(VK_CONTROL) < 0) {

        ppd->psh.dwFlags |= PSH_WIZARDHASFINISH;
    }
#endif

    if (!(ppd->psh.dwFlags & PSH_MODELESS))
    {
        //
        // Like dialog boxes, we only want to disable top level windows.
        // NB The mail guys would like us to be more like a regular
        // dialog box and disable the parent before putting up the sheet.
        if (hwndTopOwner)
        {
            while (GetWindowLong(hwndTopOwner, GWL_STYLE) & WS_CHILD)
                hwndTopOwner = GetParent(hwndTopOwner);

            ASSERT(hwndTopOwner);       // Should never get this!
            if ((hwndTopOwner == GetDesktopWindow()) ||
                (EnableWindow(hwndTopOwner, FALSE)))
            {
                //
                // If the window was the desktop window, then don't disable
                // it now and don't reenable it later.
                // Also, if the window was already disabled, then don't
                // enable it later.
                //
                hwndTopOwner = NULL;
            }
        }
    }

#if  !defined(WIN32)
#ifdef FE_IME
    // Win95d-B#754
    // When PCMCIA gets detected, NETDI calls DiCallClassInstaller().
    // The class installer of setupx calls PropertySheet() for msgsrv32.
    // We usually don't prepare thread link info in imm for that process as
    // it won't use IME normaly but we need to treat this case as special.
    //
    if (!ImmFindThreadLink(GetCurrentThreadID()))
    {
        ImmCreateThreadLink(GetCurrentProcessID(),GetCurrentThreadID());
    }
#endif
#endif

    //
    // WARNING! WARNING! WARNING! WARNING!
    //
    // Before you mess with any language stuff, be aware that MFC loads
    // resources directly out of comctl32.dll, so if you change the
    // way we choose the proper resource, you may break MFC apps.
    // See NT bug 302959.

    //
    // Support PSH_USEPAGELANG
    //

    // Presume we load our template based on thread lang id.
    wLang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    wUserLang= MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

    // BUGBUG REVIEW: PSH_USEPAGELANG was in IE4... how does this work with PlugUI now??
    // 
    if (ppd->psh.dwFlags & PSH_USEPAGELANG)
    {
        // Get callers language version. We know we have at least one page
        if (FAILED(GetPageLanguage(GETPISP(ppd, 0), &wLang)))
        {
            // failed to get langid out of caller's resource
            // just pretend nothing happened.
            wLang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
        }
        wUserLang = wLang;
    }
    else
        wLang = LANGIDFROMLCID(CCGetProperThreadLocale(NULL));

    //
    //  The only thing we need the thread locale for is to locate the
    //  correct dialog template.  We don't want it to affect page
    //  initialization or anything else like that, so get the template
    //  and quickly set the locale back before anyone notices.
    //
    //  If we can't get the requested language, retry with the neutral
    //  language.
    //


    // We have seperate dialog templates for Win95 BiDi localized
    // The code used to check to see if we are running on Win98 BiDi localized
    // and load this template.
    // We have a special case when running Office2000 with Arabic/Hebrew SKU on
    // BiDi win95 Enabled where we need to load this template as well
    if(Prsht_GetOS() == PSPT_OS_WIN95_BIDI)
    {
         lpDlgId = MAKEINTRESOURCE(IS_WIZARD(ppd) ? DLG_WIZARD95 : DLG_PROPSHEET95);
         hrsrc = FindResourceEx(
                           HINST_THISDLL, RT_DIALOG,
                           lpDlgId,
                           wLang );
         // we only have DLG_WIZARD95 and DLG_PROPSHEET95 in Arabic & Hebrew language
         // if we got any other language we will fail
         // In this case, let's use the normal templates
         if(hrsrc)
         {
             ppd->fFlipped = TRUE;
         }
         else
         {
             lpDlgId = MAKEINTRESOURCE(IS_WIZARD(ppd) ? DLG_WIZARD : DLG_PROPSHEET);
             hrsrc = FindResourceExRetry(
                               HINST_THISDLL, RT_DIALOG,
                               lpDlgId,
                                wLang );             
         }
    
    }
    else
    {
        lpDlgId = MAKEINTRESOURCE(IS_WIZARD(ppd) ? DLG_WIZARD : DLG_PROPSHEET);

        hrsrc = FindResourceExRetry(
                               HINST_THISDLL, RT_DIALOG,
                               lpDlgId,
                               wLang );
    }
    // Setup for failure
    hwndMain = NULL;

    if (hrsrc &&
        (pTemplate = (LPVOID)LoadResource(HINST_THISDLL, hrsrc)))
    {
        DWORD cbTemplate;

        cbTemplate = SizeofResource(HINST_THISDLL, hrsrc);

        pTemplateMod = (LPVOID)LocalAlloc(LPTR, cbTemplate * 2); //double it to give some play leeway

        if (pTemplateMod)
        {
            hmemcpy(pTemplateMod, pTemplate, cbTemplate);
            //Check the direction of this dialog and change it if it does not match the owner.
            Prsht_PrepareTemplate(ppd, HINST_THISDLL, &pTemplateMod, (HRSRC *)&hrsrc, 
                                 (LPSTR)lpDlgId,ppd->psh.hwndParent, &wUserLang);
        }
        else
        {
            pTemplateMod = pTemplate;       // no modifications
        }

        //
        //  Template editing and callbacks happen only if we were able
        //  to create a copy for modifying.
        //
        if (pTemplateMod != pTemplate)
        {
            if (ppd->psh.dwFlags & PSH_NOCONTEXTHELP)
            {
                if (((LPDLGTEMPLATEEX)pTemplateMod)->wSignature ==  0xFFFF){
                    ((LPDLGTEMPLATEEX)pTemplateMod)->dwStyle &= ~DS_CONTEXTHELP;
                } else {
                    ((LPDLGTEMPLATE)pTemplateMod)->style &= ~DS_CONTEXTHELP;
                }
            }

            if (IS_WIZARD(ppd) &&
                (ppd->psh.dwFlags & PSH_WIZARDCONTEXTHELP)) {

                if (((LPDLGTEMPLATEEX)pTemplateMod)->wSignature ==  0xFFFF){
                    ((LPDLGTEMPLATEEX)pTemplateMod)->dwStyle |= DS_CONTEXTHELP;
                } else {
                    ((LPDLGTEMPLATE)pTemplateMod)->style |= DS_CONTEXTHELP;
                }
            }

            // extra check for PSH_USEPAGELANG case
            if (ppd->psh.pfnCallback)
            {
#ifdef WX86
                if (ppd->fFlags & PD_WX86)
                    Wx86Callback(ppd->psh.pfnCallback, NULL, PSCB_PRECREATE, (LPARAM)(LPVOID)pTemplateMod);
                else
#endif
                    ppd->psh.pfnCallback(NULL, PSCB_PRECREATE, (LPARAM)(LPVOID)pTemplateMod);
#ifndef WINNT
                // If the callback turned off the mirroring extended style then turn off the mirroring style as well.
                if( staticIsOS(OS_MEMPHIS) && 
                    (((LPDLGTEMPLATEEX)pTemplateMod)->wSignature !=  0xFFFF) &&
                    (((LPDLGTEMPLATE)pTemplateMod)->style & DS_BIDI_RTL) &&
                    !(((LPDLGTEMPLATE)pTemplateMod)->dwExtendedStyle & RTL_MIRRORED_WINDOW)
                  )
                {
                    // Memphis ignores the Ext. styles let us use a normal style.
                    ((LPDLGTEMPLATE)pTemplateMod)->style &= ~DS_BIDI_RTL;
                }
#endif
            }
        }


        if (pTemplateMod)
        {
#ifdef WINNT
            //
            // For NT, we want to use MS Shell Dlg 2 font in the prop sheet if
            // all of the pages in the sheet use MS Shell Dlg 2.
            // To do this, we ensure the template is DIALOGEX and that the 
            // DS_SHELLFONT style bits (DS_SHELLFONT | DS_FIXEDSYS) are set.
            //
#ifndef UNIX // On UNIX MS Shell Dlg2 and MS Shell Dlg maps to the same font
            if (ShouldUseMSShellDlg2Font(ppd))
            {
                if (((LPDLGTEMPLATEEX)pTemplateMod)->wSignature != 0xFFFF)
                {
                    //
                    // Convert DLGTEMPLATE to DLGTEMPLATEEX.
                    //
                    LPVOID pTemplateCvtEx;            
                    int    iCharset = GetDefaultCharsetFromLang(wLang);
                    if (SUCCEEDED(CvtDlgToDlgEx(pTemplateMod, (LPDLGTEMPLATEEX *)&pTemplateCvtEx, iCharset)))
                    {
                        LocalFree(pTemplateMod);
                        pTemplateMod = pTemplateCvtEx;
                    } else {
                        // Unable to convert to ShellFont; oh well
                        goto NotShellFont;
                    }
                }
                //
                // Set DS_SHELLFONT style bits so we get "MS Shell Dlg2" font.
                //
                ((LPDLGTEMPLATEEX)pTemplateMod)->dwStyle |= DS_SHELLFONT;
                ppd->fFlags |= PD_SHELLFONT;
        NotShellFont:;
            }
#endif // UNIX
#endif

            // pTemplateMod is always unicode, even for the A function - no need to thunk
            hwndMain = CreateDialogIndirectParam(HINST_THISDLL, pTemplateMod,
                ppd->psh.hwndParent, PropSheetDlgProc, (LPARAM)(LPPROPDATA)ppd);

#ifdef WINNT
            // WORK AROUND WOW/USER BUG:  Even though InitPropSheetDlg sets
            // ppd->hDlg, in the WOW scenario, the incoming hDlg is WRONG!
            // The USER guys say "Tough.  You have to work around it."
            ppd->hDlg = hwndMain;
#endif
        }
        if (pTemplateMod != pTemplate)
            LocalFree(pTemplateMod);
    }

    if (!hwndMain)
    {
        int iPage;

        DebugMsg(DM_ERROR, TEXT("PropertySheet: unable to create main dialog"));

        if (hwndTopOwner && !(ppd->psh.dwFlags & PSH_MODELESS))
            EnableWindow(hwndTopOwner, TRUE);

        // Release all page objects in REVERSE ORDER so we can have
        // pages that are dependant on eachother based on the initial
        // order of those pages
        //
        for (iPage = (int)ppd->psh.nPages - 1; iPage >= 0; iPage--)
            DestroyPropertySheetPage(GETHPAGE(ppd, iPage));

        goto FreePpdAndReturn;
    }

#ifdef UNIX
    // On X Windows, to simulate modal behavior, it's not enough just to
    // process all the messages, because the window manager is a separate
    // process and doesn't generate any messages.
    // The only way to make it modal is to tell it explicitly to the
    // window manager BEFORE we map (show) a window. There's no means
    // to do it afterwards.
    // That's why I removed WS_VISIBLE from the dialogs' resources and
    // moved the ShowWindow up here, immediately after I tell the window
    // manager that I'm the modal guy (if I really am).

    MwSetModalPopup(hwndMain, !(ppd->psh.dwFlags & PSH_MODELESS));
    ShowWindow(hwndMain, SW_SHOW);
#endif

    if (ppd->psh.dwFlags & PSH_MODELESS)
        return (INT_PTR)hwndMain;

    while( ppd->hwndCurPage && GetMessage32(&msg32, NULL, 0, 0, TRUE) )
    {
        // if (PropSheet_IsDialogMessage(ppd->hDlg, (LPMSG)&msg32))
        if (Prop_IsDialogMessage(ppd, &msg32))
            continue;

        TranslateMessage32(&msg32, TRUE);
        DispatchMessage32(&msg32, TRUE);
    }

    if( ppd->hwndCurPage )
    {
        // GetMessage returned FALSE (WM_QUIT)
        DebugMsg( DM_TRACE, TEXT("PropertySheet: bailing in response to WM_QUIT (and reposting quit)") );
        ButtonPushed( ppd, IDCANCEL );  // nuke ourselves
        PostQuitMessage( (int) msg32.wParam );  // repost quit for next enclosing loop
    }

    // don't let this get mangled during destroy processing
    nReturn = ppd->nReturn ;

    if (ppd->psh.hwndParent && (GetActiveWindow() == hwndMain)) {
        DebugMsg(DM_TRACE, TEXT("Passing activation up"));
        SetActiveWindow(ppd->psh.hwndParent);
    }

    if (hwndTopOwner)
        EnableWindow(hwndTopOwner, TRUE);

    if (IsWindow(hwndOriginalFocus)) {
        SetFocus(hwndOriginalFocus);
    }

    DestroyWindow(hwndMain);

    // do pickup any PSM_REBOOTSYSTEM or PSM_RESTARTWINDOWS sent during destroy
    if ((nReturn > 0) && ppd->nRestart)
        nReturn = ppd->nRestart;

FreePpdAndReturn:

#ifdef WIN32
    LocalFree((HLOCAL)ppd);
#else
    LocalFree((HLOCAL)LOWORD(ppd));
#endif

    return nReturn;
}




#ifndef WINNT
//
// Description:
//   This function creates a 32-bit proxy page object for 16-bit page object.
//  The PSP_IS16 flag in psp.dwFlags indicates that this is a proxy object.
//
// Arguments:
//  hpage16 -- Specifies the handle to 16-bit property sheet page object.
//  hinst16 -- Specifies a handle to FreeLibrary16() when page is deleted.
//
//

HPROPSHEETPAGE WINAPI CreateProxyPage(HPROPSHEETPAGE hpage16, HINSTANCE hinst16)
{
    PISP pisp = AllocPropertySheetPage(sizeof(PROPSHEETPAGE));
    PROPSHEETPAGEA * ppsp = MapSLFix(hpage16);

    ASSERT(hpage16 != NULL);

    if (pisp)
    {
        pisp->_psp.dwSize = sizeof(pisp->_psp);
        if (ppsp)
        {
            // copy the dwFlags so we can reference PSP_HASHELP from the 32 bit side.
            pisp->_psp.dwFlags = ppsp->dwFlags | PSP_IS16;
        }
        else
        {
            pisp->_psp.dwFlags = PSP_IS16;
        }
        pisp->_psp.lParam = (LPARAM)hpage16;
        pisp->_psp.hInstance = hinst16;
    }

    if (ppsp)
    {
        UnMapSLFixArray(1, &hpage16);
    }

    return pisp ? ExternalizeHPROPSHEETPAGE(pisp) : NULL;
}

#else

HPROPSHEETPAGE WINAPI CreateProxyPage(HPROPSHEETPAGE hpage16, HINSTANCE hinst16)
{
    SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
    return NULL;
}
#endif

// DestroyPropsheetPageArray
//
//  Helper function used during error handling.  It destroys the
//  incoming property sheet pages.

void DestroyPropsheetPageArray(LPCPROPSHEETHEADER ppsh)
{
    int iPage;

    if (!(ppsh->dwFlags & PSH_PROPSHEETPAGE))
    {
        // Release all page objects in REVERSE ORDER so we can have
        // pages that are dependant on eachother based on the initial
        // order of those pages

        for (iPage = (int)ppsh->nPages - 1; iPage >= 0; iPage--)
        {
            DestroyPropertySheetPage(ppsh->H_phpage[iPage]);
        }
    }
}

// PropertySheet API
//
// This function displays the property sheet described by ppsh.
//
// Since I don't expect anyone to ever check the return value
// (we certainly don't), we need to make sure any provided phpage array
// is always freed with DestroyPropertySheetPage, even if an error occurs.
//
//
//  The fNeedShadow parameter means "The incoming LPCPROPSHEETHEADER is in the
//  opposite character set from what you implement natively".
//
//  If we are compiling UNICODE, then fNeedShadow is TRUE if the incoming
//  LPCPROPSHEETHEADER is really an ANSI property sheet page.
//
//  If we are compiling ANSI-only, then fNeedShadow is always FALSE because
//  we don't support UNICODE in the ANSI-only version.
//

#ifdef UNICODE
INT_PTR WINAPI _PropertySheet(LPCPROPSHEETHEADER ppsh, BOOL fNeedShadow)
#else
INT_PTR WINAPI PropertySheet(LPCPROPSHEETHEADER ppsh)
#define fNeedShadow FALSE
#endif
{
    PROPDATA NEAR *ppd;
    int iPage;

    //
    // validate header
    //
    ASSERT(IsValidPROPSHEETHEADERSIZE(sizeof(PROPSHEETHEADER)));

    if (!IsValidPROPSHEETHEADERSIZE(ppsh->dwSize))
    {
        DebugMsg( DM_ERROR, TEXT("PropertySheet: dwSize is not correct") );
        goto invalid_call;
    }

    if (ppsh->dwFlags & ~PSH_ALL)
    {
        DebugMsg( DM_ERROR, TEXT("PropertySheet: invalid flags") );
        goto invalid_call;
    }

    // BUGBUG: is this >= for a reason?
    if (ppsh->nPages >= MAXPROPPAGES)
    {
        DebugMsg( DM_ERROR, TEXT("PropertySheet: too many pages ( use MAXPROPPAGES )") );
        goto invalid_call;
    }

    ppd = (PROPDATA NEAR *)LocalAlloc(LPTR, sizeof(PROPDATA));
    if (ppd == NULL)
    {
        DebugMsg(DM_ERROR, TEXT("failed to alloc property page data"));

invalid_call:
        DestroyPropsheetPageArray(ppsh);
        return -1;
    }

    //  Initialize the flags.
    ppd->fFlags      = FALSE;

#ifdef WX86
    //
    //  If Wx86 is calling, set the flag that thunks the callbacks.
    //

    if ( Wx86IsCallThunked() ) {
        ppd->fFlags |= PD_WX86;
    }
#endif

    if (fNeedShadow)
        ppd->fFlags |= PD_NEEDSHADOW;

    // make a copy of the header so we can party on it
    hmemcpy(&ppd->psh, ppsh, ppsh->dwSize);

    // so we don't have to check later...
    if (!(ppd->psh.dwFlags & PSH_USECALLBACK))
        ppd->psh.pfnCallback = NULL;

    // fix up the page pointer to point to our copy of the page array
    ppd->psh.H_phpage = ppd->rghpage;

    if (ppd->psh.dwFlags & PSH_PROPSHEETPAGE)
    {
        // for lazy clients convert PROPSHEETPAGE structures into page handles
        LPCPROPSHEETPAGE ppsp = ppsh->H_ppsp;

        for (iPage = 0; iPage < (int)ppd->psh.nPages; iPage++)
        {
#ifdef UNICODE
            ppd->psh.H_phpage[iPage] = _CreatePropertySheetPage(ppsp, fNeedShadow,
                ppd->fFlags & PD_WX86);
#else
            ppd->psh.H_phpage[iPage] = CreatePropertySheetPage(ppsp);
#endif
            if (!ppd->psh.H_phpage[iPage])
            {
                iPage--;
                ppd->psh.nPages--;
            }

            ppsp = (LPCPROPSHEETPAGE)((LPBYTE)ppsp + ppsp->dwSize);      // next PROPSHEETPAGE structure
        }
    }
    else
    {
#ifdef UNICODE

        // The UNICODE build needs to hack around Hijaak 95.
        //
        ppd->psh.nPages = 0;
        for (iPage = 0; iPage < (int)ppsh->nPages; iPage++)
        {
            ppd->psh.H_phpage[ppd->psh.nPages] = _Hijaak95Hack(ppd, ppsh->H_phpage[iPage]);
            if (ppd->psh.H_phpage[ppd->psh.nPages])
            {
                ppd->psh.nPages++;
            }
        }
#else
        // make a copy of the pages passed in, since we will party here
        hmemcpy(ppd->psh.H_phpage, ppsh->H_phpage, sizeof(HPROPSHEETPAGE) * ppsh->nPages);
#endif

    }

    //
    //  Everybody else assumes that the HPROPSHEETPAGEs have been
    //  internalized, so let's do that before anybody notices.
    //
    for (iPage = 0; iPage < (int)ppd->psh.nPages; iPage++)
    {
        SETPISP(ppd, iPage, InternalizeHPROPSHEETPAGE(ppd->psh.H_phpage[iPage]));
    }

    //
    //  Walk all pages to see if any have help and if so, set the PSH_HASHELP
    //  flag in the header.
    //
    if (!(ppd->psh.dwFlags & PSH_HASHELP))
    {
        for (iPage = 0; iPage < (int)ppd->psh.nPages; iPage++)
        {
            if (GETPPSP(ppd, iPage)->dwFlags & PSP_HASHELP)
            {
                ppd->psh.dwFlags |= PSH_HASHELP;
                break;
            }
        }
    }

    return _RealPropertySheet(ppd);
}
#undef fNeedShadow

#ifdef UNICODE

INT_PTR WINAPI PropertySheetW(LPCPROPSHEETHEADERW ppsh)
{
    return _PropertySheet(ppsh, FALSE);
}

INT_PTR WINAPI PropertySheetA(LPCPROPSHEETHEADERA ppsh)
{
    PROPSHEETHEADERW pshW;
    INT_PTR iResult;

    //
    //  Most validation is done by _PropertySheet, but we need
    //  to validate the header size, or we won't survive the thunk.
    //
    if (!IsValidPROPSHEETHEADERSIZE(ppsh->dwSize))
    {
        DebugMsg( DM_ERROR, TEXT("PropertySheet: dwSize is not correct") );
        goto Error;
    }

    if (!ThunkPropSheetHeaderAtoW(ppsh, &pshW))
        goto Error;

    iResult = _PropertySheet(&pshW, TRUE);

    FreePropSheetHeaderW(&pshW);

    return iResult;

Error:
    DestroyPropsheetPageArray((LPCPROPSHEETHEADER)ppsh);
    return -1;
}

#else

INT_PTR WINAPI PropertySheetW(LPCPROPSHEETHEADERW ppsh)
{
    SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
    return -1;
}

#endif

//
//  CopyPropertyPageStrings
//
//  We have a PROPSHEETPAGE structure that contains pointers to strings.
//  For each string, create a copy and smash the pointer-to-copy in the
//  place where the original static pointer used to be.
//
//  The method of copying varies depending on what kind of copy we want
//  to make, so we use a callback procedure.
//
//  UNICODE-to-UNICODE: StrDupW
//  ANSI-to-UNICODE:    StrDup_AtoW
//  ANSI-to-ANSI:       StrDupA
//
//  On failure, all strings that did not get properly duplicated are set
//  to NULL.  You still have to call FreePropertyPageStrings to clear
//  them out.  Notice that when we fail to allocate, we merely make a note
//  of the fact and continue onward.  This ensures that all string fields
//  are set to NULL if they could not be dup'd.
//
//  ppsp - A pointer to either a PROPSHEETPAGEA or PROPSHEETPAGEW.
//         The two structures are laid out identically, so it doesn't matter.
//
//  pfnStrDup - function that will make the appropriate copy.
//

BOOL CopyPropertyPageStrings(LPPROPSHEETPAGE ppsp, STRDUPPROC pfnStrDup)
{
    BOOL fSuccess = TRUE;

    if (!(ppsp->dwFlags & PSP_DLGINDIRECT) && !IS_INTRESOURCE(ppsp->P_pszTemplate))
    {
        ppsp->P_pszTemplate = pfnStrDup(ppsp->P_pszTemplate);
        if (!ppsp->P_pszTemplate)
            fSuccess = FALSE;
    }

    if ((ppsp->dwFlags & PSP_USEICONID) && !IS_INTRESOURCE(ppsp->P_pszIcon))
    {
        ppsp->P_pszIcon = pfnStrDup(ppsp->P_pszIcon);
        if (!ppsp->P_pszIcon)
            fSuccess = FALSE;
    }

    if ((ppsp->dwFlags & PSP_USETITLE) && !IS_INTRESOURCE(ppsp->pszTitle))
    {
        ppsp->pszTitle = pfnStrDup(ppsp->pszTitle);
        if (!ppsp->pszTitle)
            fSuccess = FALSE;
    }

    if ((ppsp->dwFlags & PSP_USEHEADERTITLE) && !IS_INTRESOURCE(ppsp->pszHeaderTitle))
    {
        ppsp->pszHeaderTitle = pfnStrDup(ppsp->pszHeaderTitle);
        if (!ppsp->pszHeaderTitle)
            fSuccess = FALSE;
    }

    if ((ppsp->dwFlags & PSP_USEHEADERSUBTITLE) && !IS_INTRESOURCE(ppsp->pszHeaderSubTitle))
    {
        ppsp->pszHeaderSubTitle = pfnStrDup(ppsp->pszHeaderSubTitle);
        if (!ppsp->pszHeaderSubTitle)
            fSuccess = FALSE;
    }

    return fSuccess;
}

//
//  FreePropertyPageStrings
//
//  Free the strings that live inside a property sheet page structure.
//
//  ppsp - A pointer to either a PROPSHEETPAGEA or PROPSHEETPAGEW.
//         The two structures are laid out identically, so it doesn't matter.
//

void FreePropertyPageStrings(LPCPROPSHEETPAGE ppsp)
{
    if (!(ppsp->dwFlags & PSP_DLGINDIRECT) && !IS_INTRESOURCE(ppsp->P_pszTemplate))
        LocalFree((LPVOID)ppsp->P_pszTemplate);

    if ((ppsp->dwFlags & PSP_USEICONID) && !IS_INTRESOURCE(ppsp->P_pszIcon))
        LocalFree((LPVOID)ppsp->P_pszIcon);

    if ((ppsp->dwFlags & PSP_USETITLE) && !IS_INTRESOURCE(ppsp->pszTitle))
        LocalFree((LPVOID)ppsp->pszTitle);

    if ((ppsp->dwFlags & PSP_USEHEADERTITLE) && !IS_INTRESOURCE(ppsp->pszHeaderTitle))
        LocalFree((LPVOID)ppsp->pszHeaderTitle);

    if ((ppsp->dwFlags & PSP_USEHEADERSUBTITLE) && !IS_INTRESOURCE(ppsp->pszHeaderSubTitle))
        LocalFree((LPVOID)ppsp->pszHeaderSubTitle);
}

#ifdef UNICODE

//*************************************************************
//
//  ThunkPropSheetHeaderAtoW ()
//
//  Purpose:  Thunks the Ansi version of PROPSHEETHEADER to
//            Unicode.
//
//            Note that the H_phpage / H_ppsp field is not thunked.
//            We'll deal with that separately.
//
//*************************************************************

BOOL ThunkPropSheetHeaderAtoW (LPCPROPSHEETHEADERA ppshA,
                                LPPROPSHEETHEADERW ppsh)
{
    //
    //  Deciding whether an item should be freed or not is tricky, so we
    //  keep a private array of all the pointers we've allocated, so we
    //  know what to free when we fail.
    //
    LPTSTR Alloced[5] = { 0 };

    ASSERT(IsValidPROPSHEETHEADERSIZE(ppshA->dwSize));

    hmemcpy(ppsh, ppshA, ppshA->dwSize);

    ppsh->dwFlags |= PSH_THUNKED;
    if ((ppsh->dwFlags & PSH_USEICONID) && !IS_INTRESOURCE(ppsh->H_pszIcon))
    {
        ppsh->H_pszIcon = Alloced[0] = StrDup_AtoW(ppsh->H_pszIcon);
        if (!ppsh->H_pszIcon)
            goto ExitIcon;
    }

    if (!IS_WIZARDPSH(*ppsh) && !IS_INTRESOURCE(ppsh->pszCaption))
    {
        ppsh->pszCaption = Alloced[1] = StrDup_AtoW(ppsh->pszCaption);
        if (!ppsh->pszCaption)
            goto ExitCaption;
    }

    if ((ppsh->dwFlags & PSH_USEPSTARTPAGE) && !IS_INTRESOURCE(ppsh->H_pStartPage))
    {
        ppsh->H_pStartPage = Alloced[2] = StrDup_AtoW(ppsh->H_pStartPage);
        if (!ppsh->H_pStartPage)
            goto ExitStartPage;
    }

    if (ppsh->dwFlags & PSH_WIZARD97)
    {
        if ((ppsh->dwFlags & PSH_WATERMARK) &&
            !(ppsh->dwFlags & PSH_USEHBMWATERMARK) &&
            !IS_INTRESOURCE(ppsh->H_pszbmWatermark))
        {
            ppsh->H_pszbmWatermark = Alloced[3] = StrDup_AtoW(ppsh->H_pszbmWatermark);
            if (!ppsh->H_pszbmWatermark)
                goto ExitWatermark;
        }

        if ((ppsh->dwFlags & PSH_HEADER) &&
            !(ppsh->dwFlags & PSH_USEHBMHEADER) &&
            !IS_INTRESOURCE(ppsh->H_pszbmHeader))
        {
            ppsh->H_pszbmHeader = Alloced[4] = StrDup_AtoW(ppsh->H_pszbmHeader);
            if (!ppsh->H_pszbmHeader)
                goto ExitHeader;
        }
    }

    return TRUE;

ExitHeader:
    if (Alloced[3]) LocalFree(Alloced[3]);
ExitWatermark:
    if (Alloced[2]) LocalFree(Alloced[2]);
ExitStartPage:
    if (Alloced[1]) LocalFree(Alloced[1]);
ExitCaption:
    if (Alloced[0]) LocalFree(Alloced[0]);
ExitIcon:
    return FALSE;
}

void FreePropSheetHeaderW(LPPROPSHEETHEADERW ppsh)
{
    if ((ppsh->dwFlags & PSH_USEICONID) && !IS_INTRESOURCE(ppsh->H_pszIcon))
        LocalFree((LPVOID)ppsh->H_pszIcon);

    if (!IS_WIZARDPSH(*ppsh) && !IS_INTRESOURCE(ppsh->pszCaption))
        LocalFree((LPVOID)ppsh->pszCaption);

    if ((ppsh->dwFlags & PSH_USEPSTARTPAGE) && !IS_INTRESOURCE(ppsh->H_pStartPage))
        LocalFree((LPVOID)ppsh->H_pStartPage);

    if (ppsh->dwFlags & PSH_WIZARD97)
    {
        if ((ppsh->dwFlags & PSH_WATERMARK) &&
            !(ppsh->dwFlags & PSH_USEHBMWATERMARK) &&
            !IS_INTRESOURCE(ppsh->H_pszbmWatermark))
            LocalFree((LPVOID)ppsh->H_pszbmWatermark);

        if ((ppsh->dwFlags & PSH_HEADER) &&
            !(ppsh->dwFlags & PSH_USEHBMHEADER) &&
            !IS_INTRESOURCE(ppsh->H_pszbmHeader))
            LocalFree((LPVOID)ppsh->H_pszbmHeader);
    }
}

#endif

#ifndef WINNT

typedef LPARAM HPROPSHEETPAGE16;

extern BOOL WINAPI GetPageInfo16(HPROPSHEETPAGE16 hpage, LPTSTR pszCaption, int cbCaption, LPPOINT ppt, HICON FAR * phIcon);
extern BOOL WINAPI GetPageInfoME(HPROPSHEETPAGE16 hpage, LPTSTR pszCaption, int cbCaption, LPPOINT ppt, HICON FAR * phIcon, BOOL FAR* bRTL);

BOOL WINAPI _GetPageInfo16(HPROPSHEETPAGE16 hpage, LPTSTR pszCaption, int cbCaption, LPPOINT ppt, HICON FAR *hIcon, BOOL FAR * bRTL)
{

    // HACKHACK:  after win95 shipped, the thunk was changed for this api on
    // ME platforms.  so we have to detect the two cases and call them differently.
    if (g_fMEEnabled) {
        return GetPageInfoME(hpage, pszCaption, cbCaption, ppt, hIcon, bRTL);
    } else {
        if (bRTL)
            *bRTL = 0;
        return GetPageInfo16(hpage, pszCaption, cbCaption, ppt, hIcon);
    }
}

#endif
