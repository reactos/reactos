/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1991
 *  All Rights Reserved.
 *
 *
 *  PIFFNT.C
 *  User interface dialogs for GROUP_FNT
 *
 *  History:
 *  Created 04-Jan-1993 1:10pm by Jeff Parsons
 *  Changed 05-May-1993 5:10pm by Raymond Chen -- New Chicago-look preview
 *  Changed 12-Aug-1993 4:14pm by Raymond Chen -- Remove font inc and dec
 *
 *  All font dialog code taken from font*.c in vmdosapp, 01-Apr-93
 */

#include "shellprv.h"
#pragma hdrstop

#ifndef WINNT
#include <frosting.h>
#endif

#define REGSTR_MSDOSEMU_DISPLAYPARAMS TEXT("DisplayParams")

#ifdef WINNT
#define REGSTR_PATH_MSDOSEMU "Software\\Microsoft\\Windows\\CurrentVersion\\MS-DOS Emulation"
#endif
const TCHAR szEmulationKey[]  = TEXT(REGSTR_PATH_MSDOSEMU);
const TCHAR szDisplayParams[] = REGSTR_MSDOSEMU_DISPLAYPARAMS;

TCHAR szWndPreviewClass[] = TEXT("WOAWinPreview");
TCHAR szFontPreviewClass[] = TEXT("WOAFontPreview");

// The preview strings are loaded from resources, so they must
// remain writeable.
//
TCHAR szPreviewText[300] =
        TEXT("C:\\WINNT40> dir\n")
        TEXT("Directory of C:\\WINNT40\n")
        TEXT("SYSTEM32     <DIR>     03-01-95   6:00a\n")
        TEXT("EXPLORER EXE   393,926 03-01-95   6:00a\n")
        TEXT("PIFMGR   DLL    46,080 03-26-69   5:25p\n")
        TEXT("NOTEPAD  EXE    40,144 03-01-95   6:00a\n")
        TEXT("WINMINE  EXE    46,080 03-01-95   6:00a\n")
        TEXT("WIN      INI     7,005 03-01-95   6:00a\n");
        
// The preview strings for bilingual dosbox. 
// We'll load this from our resource that will be properly
// localized. We'll give up if it fails and use above sample
// instead.
//
TCHAR szPreviewTextDBCS[300];


UINT  cxScreen, cyScreen, dyChar, dyItem;

// Macro definitions that handle codepages 
//
#define OEMCharsetFromCP(cp) \
    ((cp)==CP_JPN? SHIFTJIS_CHARSET : ((cp)==CP_WANSUNG? HANGEUL_CHARSET : OEM_CHARSET))
/*
 * Font cache information.  Note that this cache, being in PIFMGR,
 * is now global, which will make support for per-VM font files/faces
 * more problematic, if we even decide that's an interesting feature.
 *
 */
DWORD   bpfdiStart[2] =  {  0  };    /* strage for the offset to cache */
UINT    cfdiCache[2];                   /* # used entries in fdi cache */
UINT    cfdiCacheActual[2];             /* Total # entries in fdi cache */
LPVOID lpCache = NULL;


/*
 * Owner-draw list box information.
 *
 */
HBITMAP hbmFont;                        /* Handle to "TrueType" logo */
DWORD   dwTimeCheck;
COLORREF clrChecksum;

HCURSOR hcursorWait;

#define MAXDIMENSTRING 80

// Make sure the numbers are space-padded so that sorting works properly
//
// The "multiplication symbol" must be a lowercase `x' (even though that
// is the wrong glyph; consult your local UNICODE manual), because the
// multiplication symbol (215 in the ANSI character set) does not exist
// in the Russian code page.  "No problem, I'll put it in a resource."
// Nice try.  The PanEuro version of Win95 doesn't change any resource
// strings.  They just take the US version and whack new code pages in.
// (Strange but true.)
//
const TCHAR szDimension[] = TEXT("\t%2d\tx\t%2d");


/*
 * rgwInitialTtHeights -- Initial font heights for TT fonts
 *
 * This is read from an INI file, so it must remain writeable.
 *
 * We don't try generating TT fonts below 12pt by default because
 * they just look crappy.  Frosting setup will put a different
 * table into place because Lucida Console looks good down to 4pt.
 *
 * On NT, Lucida Console is installed be default, though.
 *
 * The Frosting table is
 *      4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 20, 22
 */
#ifdef WINNT
WORD rgwInitialTtHeights[] =
        { 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 20, 22 };
#else
WORD rgwInitialTtHeights[] =
        { 0, 0, 0, 0, 0, 0, 0, 0, 12, 13, 14, 15, 16, 18, 20, 22 };
#endif



/*
 * rgpnlPenalties -- Initialize penalty array to default values
 */
INT rgpnlPenalties[] =
        { 5000, 1000, 0, 1000, 5000, 1000, 0, 1000, 1 };


POINT ptNonAspectMin = { -1, -1 };

// Context-sensitive help ids

const static DWORD rgdwHelp[] = {
        IDC_FONTGRP,            IDH_COMM_GROUPBOX,
        IDC_RASTERFONTS,        IDH_DOS_AVAIL_FONTS,
        IDC_TTFONTS,            IDH_DOS_AVAIL_FONTS,
        IDC_BOTHFONTS,          IDH_DOS_AVAIL_FONTS,
        IDC_FONTSIZELBL,        IDH_DOS_FONT_SIZE,
        IDC_FONTSIZE,           IDH_DOS_FONT_SIZE,
        IDC_WNDPREVIEWLBL,      IDH_DOS_FONT_WINDOW_PREVIEW,
        IDC_FONTPREVIEWLBL,     IDH_DOS_FONT_FONT_PREVIEW,
        IDC_WNDPREVIEW,         IDH_DOS_FONT_WINDOW_PREVIEW,
        IDC_FONTPREVIEW,        IDH_DOS_FONT_FONT_PREVIEW,
        IDC_REALMODEDISABLE,    IDH_DOS_REALMODEPROPS,
        0, 0
};


BOOL_PTR CALLBACK DlgFntProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PFNTINFO pfi;
    FunctionName(DlgFntProc);

    pfi = (PFNTINFO)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) {
    case WM_INITDIALOG:
        // allocate dialog instance data
        if (NULL != (pfi = (PFNTINFO)LocalAlloc(LPTR, SIZEOF(FNTINFO)))) {
            pfi->ppl = (PPROPLINK)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pfi);
            InitFntDlg(hDlg, pfi);
            break;
        } else {
            EndDialog(hDlg, FALSE);     // fail the dialog create
        }
        break;

    case WM_DESTROY:
        // free any allocations/resources inside the pfi first!
        if (pfi) {
            if (pfi->hFontPreview)
            {
                DeleteObject(pfi->hFontPreview);
                pfi->hFontPreview = NULL;
            }
            // ok, NOW we can free the pfi
            EVAL(LocalFree(pfi) == NULL);
            SetWindowLongPtr(hDlg, DWLP_USER, 0);
        }
        break;

    HELP_CASES(rgdwHelp)                // Handle help messages

    case WM_COMMAND:
        if (LOWORD(lParam) == 0)
            break;                      // message not from a control

        switch (LOWORD(wParam))
        {
        case IDC_RASTERFONTS:
        case IDC_TTFONTS:
        case IDC_BOTHFONTS:

            /*
             * Rebuild the font list based on the user's selection of
             * which fonts to include/exclude.
             */
            pfi->fntProposed.flFnt &= ~FNT_BOTHFONTS;
            pfi->fntProposed.flFnt |= FNTFLAGSFROMID(wParam);
            CreateFontList(GetDlgItem(hDlg, IDC_FONTSIZE), TRUE, &pfi->fntProposed);
            PreviewUpdate(GetDlgItem(hDlg, IDC_FONTSIZE), pfi);

            // BUGBUG: should all the code above be part of BN_CLICKED,
            // or is BN_CLICKED the only notification we get anyway, in which
            // case the check is pointless? -JTP

            if (HIWORD(wParam) == BN_CLICKED)
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);

            return FALSE;

        case IDC_FONTSIZE:

            // BUGBUG: how do we get this notification if our list-box
            // doesn't have the LBS_NOTIFY style?  Is the documentation for
            // LBN_SELCHANGE incorrect, or do we somehow end up with that style
            // anyway? -JTP

            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                PreviewUpdate(GetDlgItem(hDlg, IDC_FONTSIZE), pfi);
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                return TRUE;
            }
            if (HIWORD(wParam) == LBN_DBLCLK)
                ApplyFntDlg(hDlg, pfi);

            return FALSE;
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) {
        case PSN_SETACTIVE:
            AdjustRealModeControls(pfi->ppl, hDlg);
            break;

        case PSN_KILLACTIVE:
            // This gives the current page a chance to validate itself
            // SetWindowLong(hDlg, DWL_MSGRESULT, ValidFntDlg(hDlg, pfi));
            break;

        case PSN_APPLY:
            // This happens on OK....
            ApplyFntDlg(hDlg, pfi);
            break;

        case PSN_RESET:
            // This happens on Cancel....
            break;
        }
        break;

    /*
     *  For WM_MEASUREITEM and WM_DRAWITEM, since there is only one
     *  owner-draw list box in the entire dialog box, we don't have
     *  to do a GetDlgItem to figure out who he is.
     */

    case WM_MEASUREITEM:
        // measure the owner draw listbox
        MeasureItemFontList((LPMEASUREITEMSTRUCT)lParam);
        break;

    case WM_DRAWITEM:
        DrawItemFontList(TRUE, (LPDRAWITEMSTRUCT)lParam);
        break;

    case WM_SYSCOLORCHANGE:
        UpdateTTBitmap();
        break;

    default:
        return FALSE;                   // return 0 when not processing
    }
    return TRUE;
}


/** InitFntDlg
 *
 *  Create the list of appropriate fonts.
 *
 *  This routine is broken out of FontDlgProc because it chew
 *  up lots of stack for the message buffer, and we don't want to
 *  eat that much stack on every entry to FontDlgProc.
 *
 *  Note that we must defer CreateFontList until WM_INITDIALOG
 *  time because it is not until then that we have a list box that
 *  we can shove the data into.
 */

void InitFntDlg(HWND hDlg, register PFNTINFO pfi)
{
    HWND hwndList;              /* The listbox of fonts */
    PPROPLINK ppl = pfi->ppl;
    WINDOWPLACEMENT wp;
    FunctionName(InitFntDlg);

    ASSERTTRUE(ppl->iSig == PROP_SIG);

    if (!PifMgr_GetProperties(ppl, MAKELP(0,GROUP_FNT),
                        &pfi->fntProposed, SIZEOF(pfi->fntProposed), GETPROPS_NONE)
        ||
        !PifMgr_GetProperties(ppl, MAKELP(0,GROUP_WIN),
                        &pfi->winOriginal, SIZEOF(pfi->winOriginal), GETPROPS_NONE)) {
        Warning(hDlg, IDS_QUERY_ERROR, MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    /*
     * Set up instance variables for the window preview window.
     */
     
    /* Show preview maximized if window is maximized or restores to max'd.
     * Also if it is open restored and has no scrollbars.
     * (Determined by comparing the client window size against the cell
     * size and font size.)
     */

    pfi->fMax = FALSE;

    /*
     * Preload winOriginal with up-to-the-minute goodies, if we have
     * them.
     */

#define HasScrollbars(z) \
    (pfi->winOriginal.c##z##Cells * pfi->fntProposed.c##z##FontActual > \
     pfi->winOriginal.c##z##Client)

    if (ppl->hwndTty) {
        wp.length = SIZEOF(WINDOWPLACEMENT);
        VERIFYTRUE(GetWindowPlacement(ppl->hwndTty, &wp));

        // Convert/Copy to 16-bit structure
        pfi->winOriginal.wLength          = (WORD)wp.length;
        pfi->winOriginal.wShowFlags       = (WORD)wp.flags;
        pfi->winOriginal.wShowCmd         = (WORD)wp.showCmd;
        pfi->winOriginal.xMinimize        = (WORD)wp.ptMinPosition.x;
        pfi->winOriginal.yMinimize        = (WORD)wp.ptMinPosition.y;
        pfi->winOriginal.xMaximize        = (WORD)wp.ptMaxPosition.x;
        pfi->winOriginal.yMaximize        = (WORD)wp.ptMaxPosition.y;
        pfi->winOriginal.rcNormal.left    = (WORD)wp.rcNormalPosition.left;
        pfi->winOriginal.rcNormal.top     = (WORD)wp.rcNormalPosition.top;
        pfi->winOriginal.rcNormal.right   = (WORD)wp.rcNormalPosition.right;
        pfi->winOriginal.rcNormal.bottom  = (WORD)wp.rcNormalPosition.bottom;

        if (!IsIconic(ppl->hwndTty) &&
                !HasScrollbars(x) && !HasScrollbars(y)) {
            pfi->fMax = TRUE;
        }
    }

    if ((pfi->winOriginal.wShowCmd == SW_SHOWMAXIMIZED) ||
        (pfi->winOriginal.wShowFlags & WPF_RESTORETOMAXIMIZED)) {
        pfi->fMax = TRUE;
    }

    if (pfi->winOriginal.wShowCmd == SW_SHOWMAXIMIZED) {
        pfi->ptCorner.x = (LONG)pfi->winOriginal.xMaximize;
        pfi->ptCorner.y = (LONG)pfi->winOriginal.yMaximize;
    } else {
        if (pfi->winOriginal.rcNormal.left==0)
        {
            pfi->ptCorner.x = -1;
        }
        else
        {
            pfi->ptCorner.x = (LONG)pfi->winOriginal.rcNormal.left;
        }
        pfi->ptCorner.y = (LONG)pfi->winOriginal.rcNormal.top;
    }

    /*
     * First, check which fonts the user wants to see.
     *
     */
    CheckDlgButton(hDlg, IDFROMFNTFLAGS(pfi->fntProposed.flFnt), TRUE);

    hwndList = GetDlgItem(hDlg, IDC_FONTSIZE);
    // SendMessage(hwndList, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), FALSE);

    if (CreateFontList(hwndList, TRUE, &pfi->fntProposed) == LB_ERR) {
        MemoryWarning(hDlg);
        EndDialog(hDlg, PtrToLong(BPFDI_CANCEL));    /* Get out of the dialog */
        return;
    }

    /* Initialize the preview windows */
    PreviewInit(hDlg, pfi);
    PreviewUpdate(GetDlgItem(hDlg, IDC_FONTSIZE), pfi);
}


void ApplyFntDlg(HWND hDlg, register PFNTINFO pfi)
{
    PPROPLINK ppl = pfi->ppl;
    FunctionName(ApplyFntDlg);

    ASSERTTRUE(ppl->iSig == PROP_SIG);

    if (!PifMgr_SetProperties(ppl, MAKELP(0,GROUP_FNT),
                        &pfi->fntProposed, SIZEOF(pfi->fntProposed), SETPROPS_NONE))
        Warning(hDlg, IDS_UPDATE_ERROR, MB_ICONEXCLAMATION | MB_OK);
    else
    if (ppl->hwndNotify) {
        ppl->flProp |= PROP_NOTIFY;
        PostMessage(ppl->hwndNotify, ppl->uMsgNotify, SIZEOF(pfi->fntProposed), (LPARAM)MAKELP(0,GROUP_FNT));
    }
}

#ifdef WINNT
/*
 * (These routines stolen from frosting.dll on Win95,
 *  but are now part of Shell32.dll on NT)  -RickTu
 */

/** GetDosBoxTtFontsHk
 *
 * Retrieves the name of the font to use for true-type DOS box
 * in a window given a registry tree root.
 *
 * Entry:
 *
 * hkRoot      -> registry tree root to search
 * pszFaceSbcs -> LF_FACESIZE buffer for SBCS font
 * pszFaceDbcs -> LF_FACESIZE buffer for DBCS font (may be null)
 *
 * Exit:
 *
 * Buffers filled with new font names, or left unchanged if nothing
 * found in registry.
 *
 */

#define REGSTR_MSDOSEMU_FONT "Font"
#define REGSTR_MSDOSEMU_FONTDBCS "FontDBCS"

void GetDosBoxTtFontsHkA( HKEY hkRoot, LPSTR pszFaceSbcs, LPSTR pszFaceDbcs )
{
    static CHAR const szMsdosemu[] = REGSTR_PATH_MSDOSEMU;
    HKEY hk;
    DWORD cb;

    if (RegOpenKeyExA(hkRoot, szMsdosemu, 0, KEY_READ, &hk) == ERROR_SUCCESS)
    {
        static CHAR const szFont[] = REGSTR_MSDOSEMU_FONT;
        cb = LF_FACESIZE;
        RegQueryValueExA( hk, szFont, 0, 0, (LPBYTE)pszFaceSbcs, &cb );

        if (pszFaceDbcs)
        {
            static CHAR const szDbcsFont[] = REGSTR_MSDOSEMU_FONTDBCS;
            cb = LF_FACESIZE;
            RegQueryValueExA( hk, szDbcsFont, 0, 0, (LPBYTE)pszFaceDbcs, &cb );
        }
        RegCloseKey(hk);
    }
}

/** CoolGetDosBoxTtFontsA
 *
 * Retrieves the name of the font to use for true-type DOS box
 * in a window.
 *
 * This routine consults the appropriate registry keys.
 *
 * The DOS box font comes first from HKLM, to establish  a
 * machine-wide default, but can in turn be overridden by
 * HKCU for each user to override.
 *
 * Entry:
 *
 * pszFaceSbcs -> LF_FACESIZE buffer for SBCS font
 * pszFaceDbcs -> LF_FACESIZE buffer for DBCS font (may be null)
 *
 * Exit:
 *
 * Buffers filled with new font names, or left unchange if nothing
 * found in registry.
 *
 */

void CoolGetDosBoxTtFontsA( LPSTR pszFaceSbcs, LPSTR pszFaceDbcs )
{
    GetDosBoxTtFontsHkA(HKEY_LOCAL_MACHINE, pszFaceSbcs, pszFaceDbcs );
    GetDosBoxTtFontsHkA(HKEY_CURRENT_USER, pszFaceSbcs, pszFaceDbcs );
}
#endif

/** BroadcastFontChange
 *
 *  HACK! for MS PowerPoint 4.0.  These wallys, for some reason, will go
 *  off and suck down reams of CPU time if they receive a WM_FONTCHANGE
 *  message that was *posted*.  But if the message is *sent*, they do
 *  the right thing.  The puzzling thing is that they never call
 *  InSendMessage(), so how do they know?  What's more, why do they care?
 *  This was true in 3.1 also.  What's their problem?
 *
 *  The problem is that sending a broadcast risks deadlock city; see the
 *  various hacks in winoldap for places where DDE broadcasting is bypassed.
 *  In addition, since BroadcastFontChange is also called during the WEP,
 *  multi-threaded apps will deadlock if we SendMessage back to a window
 *  on a different thread in the app, because Kernel takes a process
 *  critical section during DLL unload.
 *
 *  So if PowerPig is running, we just don't tell anybody that we dorked
 *  with the fonts.
 *
 *  Returns:
 *
 *      None.
 *
 */

void BroadcastFontChange(void)
{
    if (!GetModuleHandle(szPP4)) {
        PostMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0L);
    }
}

/** LoadGlobalFontData
 *
 *  Get the name of the DOS box raster font and load it.
 *
 *  Get the name of the TT font.  (CheckDisplayParameters needs this.)
 *
 *  Check the display parameters.
 *
 *  Initialize the fdi cache.  The cache remains in GlobalLock'd
 *  memory throughout its lifetime.  This is not a problem because
 *  we are guaranteed to be in protected mode.
 *
 *  We also load things necessary for the font combo/list boxes.
 *
 *  And compute the height of the owner-draw list box item.
 *
 *  Returns:
 *
 *      TRUE on success.  In which case the FDI cache and hbmFont
 *      are ready to use.
 *
 *      FALSE on failure.  In which case there is insufficient memory
 *      to complete the operation.
 */

typedef void (WINAPI *LPFNGDBTF)(LPTSTR, LPTSTR); /* GetDosBoxTtFonts */

BOOL LoadGlobalFontData(void)
{
    HDC hDC;
    TEXTMETRIC tm;
#ifndef WINNT
    HINSTANCE hinstFrosting;
#endif
    TCHAR szBuffer[MAXPATHNAME];
    FunctionName(LoadGlobalFontData);

    cxScreen = GetSystemMetrics(SM_CXSCREEN);
    cyScreen = GetSystemMetrics(SM_CYSCREEN);

    /*
     * Get the system char size and save it away for later use.
     */
    hDC = GetDC(NULL);
#ifdef WINNT
    SelectObject(hDC, GetStockObject(SYSTEM_FONT));
#else
    SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
#endif
    GetTextMetrics(hDC, &tm);
    ReleaseDC(NULL, hDC);

    dyChar = tm.tmHeight + tm.tmExternalLeading;
    dyItem = max(tm.tmHeight, DY_TTBITMAP);

    /*
     * Chicago's AddFontResource looks in the FONTS directory first, which
     * is great because it saves us the trouble of doing goofy disk access
     * optimizations.
     */
    GetPrivateProfileString(sz386EnhSection, szWOAFontKey,
                            c_szNULL, szBuffer, ARRAYSIZE(szBuffer), szSystemINI);
    if (szBuffer[0] && AddFontResource(szBuffer)) {
        BroadcastFontChange();
    }

    /*
     * Add DBCS native font if it is present.
     */
    GetPrivateProfileString(sz386EnhSection, szWOADBCSFontKey,
                            c_szNULL, szBuffer, ARRAYSIZE(szBuffer), szSystemINI);
    if (szBuffer[0] && AddFontResource(szBuffer)) {
        BroadcastFontChange();
    }

    /*
     * Load default TT font name(s) and TT cache section names from resource
     */
    LoadStringA(g_hinst, IDS_TTFACENAME_SBCS, szTTFaceName[0], ARRAYSIZE(szTTFaceName[0]));
    LoadString(g_hinst,IDS_TTCACHESEC_SBCS, szTTCacheSection[0], ARRAYSIZE(szTTCacheSection[0]));

    if (IsBilingualCP(g_uCodePage))
    {
        LoadStringA(g_hinst, IDS_TTFACENAME_DBCS, szTTFaceName[1], ARRAYSIZE(szTTFaceName[1]));
        LoadString(g_hinst, IDS_TTCACHESEC_DBCS, szTTCacheSection[1], ARRAYSIZE(szTTCacheSection[1]));
    }        

    /*
     * Get the TT font name(s) if Frosting is installed.  Otherwise,
     * leave them at the defaults.  Fake the frosting on NT.
     */
#ifndef WINNT // this is dead code on win95
    hinstFrosting =
        (HINSTANCE)SystemParametersInfo(SPI_GETWINDOWSEXTENSION, 1, 0, 0);
    if (hinstFrosting == 1)
        // Support ILoveBunny32 since frosting doesn't have a real install
        // app yet and we still need to test antialiased fonts and dfw.
        hinstFrosting = 0;

    if (hinstFrosting) {
        LPFNGDBTF lpfnGdbtf = (LPFNGDBTF)GetProcAddress(hinstFrosting,
                                    MAKEINTRESOURCE(ordCoolGetDosBoxTtFonts));
        if (lpfnGdbtf) {
#ifndef BILINGUAL
            lpfnGdbtf(szTTFaceName, 0);
#else
#ifdef KOREA
            lpfnGdbtf( szTTFaceName[1], szTTFaceName[0] );
#else  // KOREA
            lpfnGdbtf(szTTFaceName[0], szTTFaceName[1]);
#endif  // KOREA
#endif
        }
    }
#else  // WINNT
    CoolGetDosBoxTtFontsA(szTTFaceName[0], szTTFaceName[1]);
#endif

    CheckDisplayParameters();

    // alloc needed # of cache
    //
    lpCache = (LPVOID)LocalAlloc(LPTR,
                    FDI_TABLE_START * SIZEOF(FONTDIMENINFO) * (IsBilingualCP(g_uCodePage)? 2:1) );
                         
    if (!lpCache)
        return FALSE;

    hcursorWait = LoadCursor(NULL, IDC_WAIT);

    UpdateTTBitmap();
    if (!hbmFont)
        goto E0;

    // set initial value of # of cache entries which depends on whether we have
    // two codepage to handle
    //
    cfdiCacheActual[0] = FDI_TABLE_START;

    if (IsBilingualCP(g_uCodePage))
    {
        cfdiCacheActual[1] = FDI_TABLE_START;
        bpfdiStart[1] += FDI_TABLE_START;
    }

    FontSelInit();

    return TRUE;

E0: 
    EVAL(LocalFree(lpCache) == NULL);

    return FALSE;
}



void FreeGlobalFontData()
{
    TCHAR szBuffer[MAXPATHNAME];
    FunctionName(FreeGlobalFontData);

    if (hbmFont)
        DeleteObject(hbmFont);

    EVAL(LocalFree(lpCache) == NULL);



    GetPrivateProfileString(sz386EnhSection, szWOAFontKey,
                            c_szNULL, szBuffer, ARRAYSIZE(szBuffer), szSystemINI);
    if (*szBuffer) {
        if (RemoveFontResource(szBuffer)) {
            BroadcastFontChange();
        }
    }
    GetPrivateProfileString(sz386EnhSection, szWOADBCSFontKey,
                            c_szNULL, szBuffer, ARRAYSIZE(szBuffer), szSystemINI);
    if (*szBuffer) {
        if (RemoveFontResource(szBuffer)) {
            BroadcastFontChange();
        }
    }
}


BOOL LoadGlobalFontEditData()
{
    WNDCLASS wc;
    FunctionName(LoadGlobalFontEditData);

    // Set up the window preview class for piffnt.c

    wc.style         = 0L;
    wc.lpfnWndProc   = (WNDPROC)WndPreviewWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = SIZEOF(PFNTINFO);
    wc.hInstance     = g_hinst;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szWndPreviewClass;

    // Don't go through RegisterClassD because we manually unregister
    // this class ourselves.
    if (!RealRegisterClass(&wc))
        return FALSE;

    // Set up the font preview class for piffnt.c

    wc.style         = 0L;
    wc.lpfnWndProc   = (WNDPROC)FontPreviewWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = SIZEOF(PFNTINFO);
    wc.hInstance     = g_hinst;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szFontPreviewClass;

    // Don't go through RegisterClassD because we manually unregister
    // this class ourselves.
    if (!RealRegisterClass(&wc))
        return FALSE;

    return TRUE;
}


void FreeGlobalFontEditData()
{
    FunctionName(FreeGlobalFontEditData);
    UnregisterClass(szWndPreviewClass, g_hinst);
    UnregisterClass(szFontPreviewClass, g_hinst);
}


/** CheckDisplayParameters
 *
 *  Make sure that the display parameters have not changed, including
 *  the name of the TT font(s).
 *
 *  If they have, we BLAST OUR CACHE since it is no longer any good.
 *
 *  Entry:
 *      szTTFaceName contains the name of the TrueType font to use.
 *
 *  Returns:
 *      None.
 */

void CheckDisplayParameters(void)
{
    HDC         hIC;
    HKEY        hk;
    DISPLAYPARAMETERS dpTrue, dpStored;
    FunctionName(CheckDisplayParameters);

    hIC = CreateIC(szDisplay, 0, 0, 0);

    if (!hIC) {
        /*
         * If things are really screwy, stay conservative and assume
         * that all is well.
         */
        return;
    }

    dpTrue.dpHorzSize   = GetDeviceCaps(hIC, HORZSIZE);
    dpTrue.dpVertSize   = GetDeviceCaps(hIC, VERTSIZE);
    dpTrue.dpHorzRes    = GetDeviceCaps(hIC, HORZRES);
    dpTrue.dpVertRes    = GetDeviceCaps(hIC, VERTRES);
    dpTrue.dpLogPixelsX = GetDeviceCaps(hIC, LOGPIXELSX);
    dpTrue.dpLogPixelsY = GetDeviceCaps(hIC, LOGPIXELSY);
    dpTrue.dpAspectX    = GetDeviceCaps(hIC, ASPECTX);
    dpTrue.dpAspectY    = GetDeviceCaps(hIC, ASPECTY);
    dpTrue.dpBitsPerPixel = GetDeviceCaps(hIC, BITSPIXEL);
    DeleteDC(hIC);

    /*
     *  Since szTTFaceName is pre-initialized to "Courier New" padded
     *  with nulls, we can rely on the garbage after the end of the
     *  string always to be the same, so that a pure memory comparison
     *  will work.
     */
#ifdef UNICODE
    MultiByteToWideChar( CP_ACP, 0, szTTFaceName[0], -1, dpTrue.szTTFace[0], ARRAYSIZE(dpTrue.szTTFace[0]) );
    if (IsBilingualCP(g_uCodePage))
        MultiByteToWideChar( CP_ACP, 0, szTTFaceName[1], -1, dpTrue.szTTFace[1], ARRAYSIZE(dpTrue.szTTFace[1]) );
#else
    hmemcpy(dpTrue.szTTFace, szTTFaceName, SIZEOF(szTTFaceName));
#endif

    /*
     *  We must store the dimension information in the registry because
     *  the install program for Omar Sharif Bridge will ERASE! your
     *  SYSTEM.INI if it contains a line greater than 78 characters.
     *  (I am not making this up.  How could I?)
     */

    if (RegCreateKey(HKEY_LOCAL_MACHINE, szEmulationKey, &hk) == 0) {
        DWORD cb = SIZEOF(DISPLAYPARAMETERS);
        if (SHQueryValueEx(hk, szDisplayParams, 0, 0, (LPVOID)&dpStored, &cb) != 0 || cb != SIZEOF(DISPLAYPARAMETERS) || IsBufferDifferent(&dpTrue, &dpStored, SIZEOF(DISPLAYPARAMETERS))) {
            /*
             * Not much we can do if the write fails, so don't check.
             */
            VERIFYTRUE(RegSetValueEx(hk, szDisplayParams, 0, REG_BINARY, (LPVOID)&dpTrue, cb) == 0);

            /* Blast the font dimension cache */
            WritePrivateProfileString(szTTCacheSection[1], NULL, NULL, szSystemINI);
            if (IsBilingualCP(g_uCodePage))
                WritePrivateProfileString(szTTCacheSection[0], NULL, NULL, szSystemINI);
        }
        VERIFYTRUE(RegCloseKey(hk) == 0);
    } else {
        /*
         *  Couldn't access registry.  Oh well.
         */
    }

}

/** PreviewInit
 *
 *  When the dialog box is created, we create the Window
 *  Preview child window, as well as the Font Preview window.
 *
 *  The creation is deferred until the actual dialog box creation
 *  because the size and shape of the Window Preview window depends
 *  on the current video driver.
 */

void PreviewInit(HWND hDlg, PFNTINFO pfi)
{
    HWND hwnd;
    RECT rectLabel, rcPreview;
    FunctionName(PreviewInit);

    LoadString(g_hinst, IDS_PREVIEWTEXT, szPreviewText, ARRAYSIZE(szPreviewText));
    
    if (IsBilingualCP(pfi->fntProposed.wCurrentCP))
    {
        // load a sample for their native codepage
        //
        LoadString(g_hinst, IDS_PREVIEWTEXT_BILNG, 
                            szPreviewTextDBCS, ARRAYSIZE(szPreviewTextDBCS));
    }
    /*
     * Compute the size of our preview window.
     *
     *  The top is aligned with the top of IDC_WNDPREVIEWLBL,
     *          minus a top margin of 3/2 dyChar.
     *  The left edge is aligned with the left edge of IDC_WNDPREVIEWLBL.
     *  The maximum width is the width of IDC_WNDPREVIEWLBL.
     *  The bottom edge can go as far down as the bottom of the dialog,
     *          minus a bottom margin of 3/2 dyChar.
     *  And the shape of the preview window is determined by the screen
     *          dimensions.
     *
     * We make the preview window as large as possible, given these
     * constraints.
     *
     */
    GetWindowRect(GetDlgItem(hDlg, IDC_WNDPREVIEWLBL), &rectLabel);
    ScreenToClient(hDlg, (LPPOINT)&rectLabel);
    ScreenToClient(hDlg, (LPPOINT)&rectLabel.right);

    /*
     * This GetWindowRect/ScreenToClient sets rcPreview.top.
     */
    GetWindowRect(GetDlgItem(hDlg, IDC_WNDPREVIEWLBL), &rcPreview);
    ScreenToClient(hDlg, (LPPOINT)&rcPreview);

    /*
     * Compute height based on width.
     */
    rcPreview.top += 3 * dyChar / 2;
    rcPreview.left = rectLabel.left;
    rcPreview.right = rectLabel.right - rectLabel.left;
    rcPreview.bottom = AspectScale(cyScreen, cxScreen, rcPreview.right);

#ifdef LATER
  {
    RECT rectGroup;
    /*
     * Check if we made the window too tall.  If so, then we have to
     * compute the width based on the height.
     */

    GetWindowRect(GetDlgItem(hDlg, IDC_GROUP), &rectGroup);
    rectGroup.bottom -= rectGroup.top;
    ScreenToClient(hDlg, (LPPOINT)&rectGroup);
    if (rcPreview.bottom + rcPreview.top > rectGroup.top - dyChar)
    {
        rcPreview.bottom = rectGroup.top - rcPreview.top - 3 * dyChar / 2;
        rcPreview.right = AspectScale(cxScreen, cyScreen, rcPreview.bottom);
    }
  }
#endif

    /*
     * Phew.  Now we can create the preview window.
     */
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        szWndPreviewClass, NULL,
        WS_CHILD | WS_VISIBLE,
        rcPreview.left, rcPreview.top,
        rcPreview.right, rcPreview.bottom,
        hDlg, (HMENU)IDC_WNDPREVIEW, g_hinst, NULL);

    if (hwnd)
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)pfi);

    /*
     * Compute the size of the font preview.  This is easier.
     */
    GetWindowRect(GetDlgItem(hDlg, IDC_FONTPREVIEWLBL), &rectLabel);
    ScreenToClient(hDlg, (LPPOINT)&rectLabel.left);
    ScreenToClient(hDlg, (LPPOINT)&rectLabel.right);

    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        szFontPreviewClass, NULL,
        WS_CHILD | WS_VISIBLE,
        rectLabel.left,
        rectLabel.top + 3 * dyChar / 2,
        rectLabel.right - rectLabel.left,
        rcPreview.bottom,
        hDlg, (HMENU)IDC_FONTPREVIEW, g_hinst, NULL);

    if (hwnd)
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)pfi);
}


/*  PreviewUpdate
 *
 *  Does the preview of the selected font.
 */

void PreviewUpdate(HWND hwndList, PFNTINFO pfi)
{
    HWND hDlg;
    BPFDI bpfdi;
    FunctionName(PreviewUpdate);

    /* Delete the old font if necessary */
    if (pfi->hFontPreview)
    {
        DeleteObject(pfi->hFontPreview);
        pfi->hFontPreview = NULL;
    }

    /* When we select a font, we do the font preview by setting it
     * into the appropriate list box
     */
    bpfdi = (BPFDI)GetFont(hwndList, TRUE, pfi);
    if (IsSpecialBpfdi(bpfdi))
        return;

    /* Update our internal font structure so that preview window
     * will actually change
     */
    pfi->bpfdi = bpfdi;
    SetFont(&pfi->fntProposed, bpfdi);

    /* Make the new font */
    pfi->hFontPreview = CreateFontFromBpfdi(bpfdi, pfi);

    /* Force the preview windows to repaint */
    hDlg = GetParent(hwndList);
    InvalidateRect(GetDlgItem(hDlg, IDC_WNDPREVIEW), NULL, TRUE);
    InvalidateRect(GetDlgItem(hDlg, IDC_FONTPREVIEW), NULL, TRUE);
}


/*  WndPreviewWndProc
 *
 *  Handles the window preview window.
 */

LRESULT WndPreviewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    FunctionName(WndPreviewWndProc);
    switch (uMsg)
    {
    case WM_PAINT:
        WndPreviewPaint(GetParent(hwnd), hwnd);
        break;

    case WM_HELP:       // Handles title bar help button message
        WinHelp(hwnd, NULL, HELP_CONTEXTPOPUP, IDH_DOS_FONT_WINDOW_PREVIEW);
        break;

    case WM_RBUTTONUP:
    case WM_NCRBUTTONUP: // Equivalent of WM_CONTEXTMENU
        OnWmContextMenu((WPARAM)hwnd, &rgdwHelp[0]);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

#ifndef DONT_WORRY_ABOUT_PEOPLE_WITH_REALLY_WIDE_FRAMES
/* _DrawFrame
 *
 *  Swiped from Control Panel / Metrics.
 *
 *  Draws the frame *and* modifies the rectangle to contain the
 *  shrunk coordinates.
 */
void _DrawFrame(HDC hdc, int clr, LPRECT lprc, int cx, int cy)
{
    HBRUSH hbr;
    RECT rcT;
    FunctionName(_DrawFrame);

    CopyRect(&rcT, lprc);
    hbr = SelectObject(hdc, GetSysColorBrush(clr));

    /* Left */
    PatBlt(hdc, rcT.left, rcT.top, cx, rcT.bottom-rcT.top, PATCOPY);
    rcT.left += cx;

    /* Top */
    PatBlt(hdc, rcT.left, rcT.top, rcT.right-rcT.left, cy, PATCOPY);
    rcT.top += cy;

    /* Right */
    rcT.right -= cx;
    PatBlt(hdc, rcT.right, rcT.top, cx, rcT.bottom-rcT.top, PATCOPY);

    /* Bottom */
    rcT.bottom -= cy;
    PatBlt(hdc, rcT.left, rcT.bottom, rcT.right-rcT.left, cy, PATCOPY);

    SelectObject(hdc, hbr);
    CopyRect(lprc, &rcT);
}
#endif /* DONT_WORRY_ABOUT_PEOPLE_WITH_REALLY_WIDE_FRAMES */


/*  WndPreviewPaint
 *
 *  Paints the window preview window.  This is called from its
 *  paint message handler.
 *
 */

void WndPreviewPaint(HWND hDlg, HWND hwnd)
{
    PPROPLINK ppl;
    PFNTINFO pfi;
    RECT rcPreview;
    RECT rcWin;
    RECT rcClient;
    RECT rcT;
    POINT ptButton;
#define cxButton    ptButton.x
#define cyButton    ptButton.y
    POINT ptCorner;
#ifndef DONT_WORRY_ABOUT_PEOPLE_WITH_REALLY_WIDE_FRAMES
    POINT ptFrame;
#define cxFrame    ptFrame.x
#define cyFrame    ptFrame.y
#endif
    BPFDI bpfdi;
    int cxBorder, cyBorder;
    int dyToolbar;
    PAINTSTRUCT ps;
    BOOL bCenter;
    FunctionName(WndPreviewPaint);

    BeginPaint(hwnd, &ps);

    pfi = (PFNTINFO)GetWindowLongPtr(hwnd, 0);

    ppl = pfi->ppl;
    ASSERTTRUE(ppl->iSig == PROP_SIG);

    bpfdi = pfi->bpfdi;

    /* If we don't have a font, get out */
    if (!pfi->hFontPreview)
        return;

    /* Get the width of the preview "screen" */
    GetClientRect(hwnd, &rcPreview);

    /* Figure out how large we would be as a result of the change.
     * This isn't perfect, but it'll probably be close enough.
     * (Imperfection if we chose AUTO as the font.)
     */

    /* Assume maximized */
    rcClient.left = rcClient.top = 0;
    if (pfi->winOriginal.cxCells) {
        rcClient.right = pfi->winOriginal.cxCells * bpfdi->fdiWidthActual;
    } else {
        rcClient.right = 80 * bpfdi->fdiWidthActual;
    }

    if (pfi->winOriginal.cyCells) {
        rcClient.bottom = pfi->winOriginal.cyCells * bpfdi->fdiHeightActual;
    } else {
        PROPVID vid;

        // set default value
        rcClient.bottom = 25 * bpfdi->fdiHeightActual;

        // now see if there is a value in the pif file for size of window...
        if (PifMgr_GetProperties(ppl, MAKELP(0,GROUP_VID),
                        &vid, SIZEOF(vid), GETPROPS_NONE))
        {
            if (vid.cScreenLines > 0)
                rcClient.bottom = vid.cScreenLines * bpfdi->fdiHeightActual;

        }
    }
    if (!pfi->fMax && pfi->winOriginal.cxClient && pfi->winOriginal.cyClient) {
        /* Shrink down to window actual */
        if (rcClient.right > (int)pfi->winOriginal.cxClient)
            rcClient.right = (int)pfi->winOriginal.cxClient;
        if (rcClient.bottom > (int)pfi->winOriginal.cyClient)
            rcClient.bottom = (int)pfi->winOriginal.cyClient;
    }

    /* Get some more metrics */
    cxBorder = GetSystemMetrics(SM_CXBORDER);
    cyBorder = GetSystemMetrics(SM_CYBORDER);

    cxButton = GetSystemMetrics(SM_CXSIZE);
    cyButton = GetSystemMetrics(SM_CYSIZE);
//  cyButton *= 2;                      /* Double the height for "looks" */

#ifndef DONT_WORRY_ABOUT_PEOPLE_WITH_REALLY_WIDE_FRAMES
    cxFrame = GetSystemMetrics(SM_CXFRAME);
    cyFrame = GetSystemMetrics(SM_CYFRAME);
#endif

    /* FLAG DAY!  Convert everything from desktop coordinates to
     * aspect ratio-scaled preview coordinates
     *
     * Do **not** convert cxBorder and cyBorder!
     *
     * ptCorner must not be modified in-place since its value is used at
     * the next go-round.
     *
#ifndef DONT_WORRY_ABOUT_PEOPLE_WITH_REALLY_WIDE_FRAMES
     * After translation, cxFrame and cyFrame are adjusted so that the
     * cxBorder counts against them.  This allows for users who set
     * really wide frames, but doesn't penalize the users who have
     * narrow frames.
#endif
     */

    ptCorner = pfi->ptCorner;
    bCenter = (ptCorner.x == -1);
    AspectPoint(&rcPreview, &ptCorner);
#ifndef DONT_WORRY_ABOUT_PEOPLE_WITH_REALLY_WIDE_FRAMES
    AspectPoint(&rcPreview, &ptFrame);
#endif
    AspectRect(&rcPreview, &rcClient);
    AspectPoint(&rcPreview, &ptButton);

    /*
     * BUGBUG -- The height of a toolbar is hard-coded at 30 pixels.
     */
    if (pfi->winOriginal.flWin & WIN_TOOLBAR) {
        dyToolbar = (int)AspectScale(rcPreview.bottom, cyScreen, 30);
    } else {
        dyToolbar = 0;
    }

    /* Make sure the buttons are nonzero in dimension */
    if (cxButton == 0) cxButton = 1;
    if (cyButton == 0) cyButton = 1;

#ifndef DONT_WORRY_ABOUT_PEOPLE_WITH_REALLY_WIDE_FRAMES
    /*
     * Don't penalize people who have thin frames.
     */
    if (cxFrame < cxBorder) cxFrame = cxBorder;
    if (cyFrame < cyBorder) cyFrame = cyBorder;
#endif

    /*
     * Convert from client rectangle back to window rectangle.
     *
     * We must do this *AFTER* the flag day, because we need to use the
     * post-flag day cxBorder and cyBorder.
     */

    /* Put a (scaled-down) toolbar into place.  We'll expand the client
     * region to accomodate it.  (We'll subtract the toolbar off before
     * painting the client region.)
     */
    rcClient.bottom += dyToolbar;

    /* Shove the client region down to make room for the caption. */
    OffsetRect(&rcClient, 0, cyButton);

    rcWin = rcClient;
    rcWin.top = 0;
#ifndef DONT_WORRY_ABOUT_PEOPLE_WITH_REALLY_WIDE_FRAMES
    InflateRect(&rcWin, cxFrame, cyFrame);
#else
    InflateRect(&rcWin, cxBorder, cyBorder);
#endif

    /*
     * Now put it in the proper position on the (shrunk-down) desktop.
     * We cannot do this until rcWin's value is finalized.
     */
    if (bCenter)
    {
        ptCorner.x = ( (rcPreview.right - rcPreview.left) -
                       (rcWin.right  - rcWin.left)
                      ) / 2;
        if (ptCorner.x < 0)
            ptCorner.x = 0;

        ptCorner.y = ( (rcPreview.bottom - rcPreview.top) -
                       (rcWin.bottom  - rcWin.top)
                      ) / 5;
        if (ptCorner.y < 0)
            ptCorner.y = 0;

    }
    OffsetRect(&rcWin, ptCorner.x, ptCorner.y);
    OffsetRect(&rcClient, ptCorner.x, ptCorner.y);

    /* It's party time! */

    /* The outer border */
    DrawEdge(ps.hdc, &rcWin, BDR_RAISEDINNER, BF_RECT | BF_ADJUST);

#ifndef DONT_WORRY_ABOUT_PEOPLE_WITH_REALLY_WIDE_FRAMES
    /* The sizing frame */
    _DrawFrame(ps.hdc, COLOR_ACTIVEBORDER,
                    &rcWin, cxFrame - cxBorder, cyFrame - cyBorder);
#endif

    /* rcWin has now shrunk to its inner edge */

    /* Move its bottom edge upwards to meet the top of the client region.
     * This turns rcWin into the caption area.
     */
    rcWin.bottom = rcClient.top;
    FillRect(ps.hdc, &rcWin, (HBRUSH)(COLOR_ACTIVECAPTION+1));

    /* Next comes the toolbar */
    rcT= rcClient;
    rcT.bottom = rcT.top + dyToolbar;
    FillRect(ps.hdc, &rcT, (HBRUSH)(COLOR_BTNFACE+1));

    /* Next, draw the client region */
    rcClient.top += dyToolbar;
    DrawEdge(ps.hdc, &rcClient, BDR_SUNKENOUTER, BF_RECT | BF_ADJUST);
    FillRect(ps.hdc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH));

    /*
     * Now draw the three caption buttons.
     */

    /*
     * The system menu.
     */
    rcT = rcWin;
    rcT.right = rcT.left + cxButton;
  //DrawFrameControl(ps.hdc, &rcT, DFC_SYSMENU, DFCS_SYSMENUMAIN);
    DrawFrameControl(ps.hdc, &rcT, DFC_CAPTION, DFCS_CAPTIONCLOSE);

    /*
     * The maximize menu.
     */
    rcWin.left = rcWin.right - cxButton;
  //DrawFrameControl(ps.hdc, &rcWin, DFC_SIZE, DFCS_SIZEMAX);
    DrawFrameControl(ps.hdc, &rcWin, DFC_CAPTION, DFCS_CAPTIONMAX);

    /*
     * The minimize menu.
     */
    rcWin.left -= cxButton;
    rcWin.right -= cxButton;
  //DrawFrameControl(ps.hdc, &rcWin, DFC_SIZE, DFCS_SIZEMIN);
    DrawFrameControl(ps.hdc, &rcWin, DFC_CAPTION, DFCS_CAPTIONMIN);

    EndPaint(hwnd, &ps);
}
#undef cxButton
#undef cyButton
#ifndef DONT_WORRY_ABOUT_PEOPLE_WITH_REALLY_WIDE_FRAMES
#undef cxFrame
#undef cyFrame
#endif

/*  FontPreviewWndProc
 *
 *  Handles the font preview window
 */

LRESULT FontPreviewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    PFNTINFO pfi;
    PAINTSTRUCT ps;
    FunctionName(FontPreviewWndProc);

    switch (uMsg)
    {
    case WM_PAINT:
        BeginPaint(hwnd, &ps);

        pfi = (PFNTINFO)GetWindowLongPtr(hwnd, 0);

        /* Draw the font sample */
        SelectObject(ps.hdc, pfi->hFontPreview);
        SetTextColor(ps.hdc, RGB(192, 192, 192));
        SetBkColor(ps.hdc, RGB(0, 0, 0));
        GetClientRect(hwnd, &rect);
        InflateRect(&rect, -2, -2);
        if ( IsBilingualCP(pfi->fntProposed.wCurrentCP) )
            DrawText(ps.hdc, szPreviewTextDBCS, -1, &rect, 0);
        else
            DrawText(ps.hdc, szPreviewText, -1, &rect, 0);

        EndPaint(hwnd, &ps);
        break;

    case WM_HELP:       // Handles title bar help button message
        WinHelp(hwnd, NULL, HELP_CONTEXTPOPUP, IDH_DOS_FONT_FONT_PREVIEW);
        break;

    case WM_RBUTTONUP:
    case WM_NCRBUTTONUP: // Equivalent of WM_CONTEXTMENU
        OnWmContextMenu((WPARAM)hwnd, &rgdwHelp[0]);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}


/** CreateFontList
 *
 *  Loads the dialog control hwndList with all available font
 *  dimensions for raster fonts, and a selected collection of
 *  dimensions for TrueType fonts.
 *
 *  The reference data for each control is an index into lpfniCache.
 *
 *  The hourglass cursor is displayed during the font list build.
 *
 *  Entry:
 *      hwndList == handle to listbox or combo box to fill
 *      fListBox == TRUE if hwndList is a listbox, FALSE if a combo box
 *      lpFnt    -> PROPFNT structure
 *
 *      If HIWORD(lpFnt) is NULL, then LOWORD(lpFnt) is used as an hProps
 *      to get obtain property info for that handle.
 *
 *  Returns:
 *      >= 0 on success, indicating the current selection.
 *      In which case the FDI cache is valid and hwndList has been filled
 *      with font information, and the currently-selected font has been
 *      made the current selection.
 *
 *      LB_ERR/CB_ERR on failure.  The list box hwndList is left in an
 *      indeterminate state.
 */

int WINAPI CreateFontList(HWND hwndList, BOOL fListBox, LPPROPFNT lpFnt)
{
    DWORD   dwIndex;
    HCURSOR hcursor;
    PROPFNT fntTemp;
    int     iReturn = LB_ERR;
    TCHAR   szBuf[MAXDIMENSTRING];
    FunctionName(CreateFontList);

    if (IS_INTRESOURCE(lpFnt)) {

        // BUGBUG handle truncations!
                                                    
        if (!PifMgr_GetProperties(lpFnt, MAKELP(0,GROUP_FNT),
                           &fntTemp, SIZEOF(fntTemp), GETPROPS_NONE))
            goto Exit2;

        lpFnt = &fntTemp;
    }

    /*
     * Put up an hourglass while the font list build is taking place,
     * since it might take a long time if we have to re-rasterize
     * TrueType fonts.
     *
     * NOTE!  That we do not do a ShowCursor.  Why?
     *
     *  If the user is on a mouseless system, then he can't access his
     *  toolbar, and hence the only time this code can get called is
     *  during the creation of the font selection dialog box.  In which
     *  case, DialogBox has already done a ShowCursor for us.
     *
     */
    hcursor = SetCursor(hcursorWait);

    /*
     * Initialize the list box.
     */
    if (hwndList) {
        SendMessage(hwndList, WM_SETREDRAW, FALSE, 0L);
        SendMessage(hwndList, fListBox ? LB_RESETCONTENT : CB_RESETCONTENT, 0, 0L);
    }

    /*
     * Add the fonts.
     */
    if ((lpFnt->flFnt & FNT_RASTERFONTS) &&
        !AddRasterFontsToFontListA(hwndList, fListBox,
                                  lpFnt->achRasterFaceName, lpFnt->wCurrentCP))
        goto Exit;

    if ((lpFnt->flFnt & FNT_TTFONTS) &&
        !AddTrueTypeFontsToFontListA(hwndList, fListBox,
                                  lpFnt->achTTFaceName, lpFnt->wCurrentCP))
        goto Exit;

    /*
     * And the magical "Auto" font size.
     */

    /*
     * Beyond this point, success is assured, so at the very least,
     * DON'T return LB_ERR;  we may optionally set the return code to
     * the current selection, below, too...
     */
    iReturn = 0;

    if (hwndList) {
        /*
         * No error checking here because if anything fails, then the
         * end result will be merely that the "Auto" option either
         * (1) exists but is invisible, or (2) doesn't appear at all.
         */
        LoadString(g_hinst, IDS_AUTO, szBuf, ARRAYSIZE(szBuf));
        dwIndex = lcbInsertString(hwndList, fListBox, szBuf, 0);
        lcbSetItemDataPair(hwndList, fListBox, dwIndex, BPFDI_AUTO, 0);

        /*
         * Make yet another pass through the list to find the current
         * font and select it.  Thanks to an intentional flakiness
         * in USER, we can't do this check at the point that the
         * font is added, because the selection does not move with the
         * item when a new item is inserted above the selection.
         *
         * Bleah.
         */
        if (!MatchCurrentFont(hwndList, fListBox, lpFnt)) {
            /*
             * If no font matched the current font, and we are a list box,
             * then make the first font the current selection.
             *
             * We don't want to make any default selection if we are a
             * combo box, because that would make the user think that the
             * current font was something it wasn't.
             */
            if (fListBox) {
                /*
                 * SORTING-SENSITIVE!  This assumes that "Auto" is at the top
                 * of the list.
                 */
                lcbSetCurSel(hwndList, TRUE, 0);
                lpFnt->flFnt |= FNT_AUTOSIZE;
            }
        }
        SendMessage(hwndList, WM_SETREDRAW, TRUE, 0L);

        iReturn = lcbGetCurSel(hwndList, fListBox);
    }
Exit:
    /*
     * Reset the mouse cursor.
     */
    SetCursor(hcursor);

Exit2:
    return iReturn;
}


/** UpdateTTBitmap
 *
 *  Recompute the colors for the TrueType bitmap hbmFont.
 *
 *  Since we may receive this several times for a single WM_SYSCOLORCHANGE,
 *  we update our colors under the following conditions:
 *
 *      1. More than one second has elapsed since the previous call, or
 *      2. A crude checksum fails.
 *
 *  Entry:
 *      None.
 *
 *  Returns:
 *      hbmFont recomputed.
 */

VOID WINAPI UpdateTTBitmap(void)
{
    COLORREF clr;

    /*
     *  Note that the checksum should not be a symmetric function,
     *  because a common color alteration is to exchange or permute
     *  the colors.
     */
    clr = +  GetSysColor(COLOR_BTNTEXT)
          -  GetSysColor(COLOR_BTNSHADOW)
          + (GetSysColor(COLOR_BTNFACE) ^ 1)
          - (GetSysColor(COLOR_BTNHIGHLIGHT) ^ 2)
          ^  GetSysColor(COLOR_WINDOW);

    if (!hbmFont || clr != clrChecksum || GetTickCount() - dwTimeCheck < 1000) {
        clrChecksum = clr;
        dwTimeCheck = GetTickCount();
        if (hbmFont) DeleteObject(hbmFont);
        hbmFont = CreateMappedBitmap(g_hinst, IDB_TTBITMAP, 0, NULL, 0);
    }
}


/** DrawItemFontList
 *
 *  Answer the WM_DRAWITEM message sent from the font list box or
 *  font combo box.
 *
 *  This code was originally lifted from FONT.C in sdk\commdlg.
 *
 *  See fontutil.h for an explanation of the \1 hack.
 *
 *  Entry:
 *      fListBox    =  TRUE if the item is a list box, FALSE if a combo box
 *      lpdis       -> DRAWITEMSTRUCT describing object to be drawn
 *
 *  Returns:
 *      None.
 *
 *      The object is drawn.
 */

#define cTabsList 3

typedef struct DIFLINFO {
    LPTSTR       di_lpsz;
    PINT        di_pTabs;
} DIFLINFO, *LPDIFLINFO;

#define lpdi ((LPDIFLINFO)lp)
BOOL CALLBACK diflGrayStringProc(HDC hdc, LPARAM lp, int cch)
{
    return (BOOL)TabbedTextOut(hdc, 0, 0,
                  lpdi->di_lpsz, lstrlen(lpdi->di_lpsz),
                  cTabsList, lpdi->di_pTabs, 0);

}
#undef lpdi

VOID WINAPI DrawItemFontList(BOOL fListBox, const LPDRAWITEMSTRUCT lpdis)
{
    HDC     hDC, hdcMem;
    DWORD   rgbBack, rgbText;
    int     iColorBack;
    COLORREF clrText;
    COLORREF clrBack;
    TCHAR    szDimen[MAXDIMENSTRING];
    HBITMAP hOld;
    int     dy;
    DIFLINFO di;
    static int rgTabsList[cTabsList] = {0, 0, 0};
    static int rgTabsCombo[cTabsList] = {0, 0, 0};
#define lpsz di.di_lpsz
#define pTabs di.di_pTabs
    FunctionName(DrawItemFontList);

    if ((int)lpdis->itemID < 0)
        return;

    hDC = lpdis->hDC;

    if (lpdis->itemAction & ODA_FOCUS) {
        if (lpdis->itemState & ODS_SELECTED) {
            DrawFocusRect(hDC, &lpdis->rcItem);
        }
    } else {
        if (lpdis->itemState & ODS_SELECTED) {
            clrBack = GetSysColor(iColorBack = COLOR_HIGHLIGHT);
            clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
        } else {
            clrBack = GetSysColor(iColorBack = COLOR_WINDOW);
            clrText = GetSysColor(IsWindowEnabled(lpdis->hwndItem) ?
                                        COLOR_WINDOWTEXT : COLOR_GRAYTEXT);
        }
        rgbText = SetTextColor(hDC, clrText);
        rgbBack = SetBkColor(hDC, clrBack);

        // draw selection background
        FillRect(hDC, &lpdis->rcItem, (HBRUSH)UIntToPtr( (iColorBack + 1) ));

        // get the string
        SendMessage(lpdis->hwndItem, fListBox ? LB_GETTEXT : CB_GETLBTEXT, lpdis->itemID, (LPARAM)(LPTSTR)szDimen);

        lpsz = szDimen;
        if (szDimen[0] == TEXT('\1'))   // hack for "Auto" string
            lpsz++;

        if (fListBox)
            pTabs = rgTabsList;
        else
            pTabs = rgTabsCombo;

        if (pTabs[0] == 0) {            /* Never seen this font before */
            /* Assumes GetTextExtent(hDC, ANSI_TIMES, 1) < 2 * dxChar */
            SIZE sSize;
            GetTextExtentPoint32( hDC, szZero, 1, &sSize ); // size of '0'
            /* A negative # for tab stop right aligns the tabs... */
            pTabs[0] = -sSize.cx * 3;
            pTabs[1] = -sSize.cx * 5;
            pTabs[2] = -sSize.cx * 8;
        }

        // draw the text
        //
        // Note that the SDK dox for GrayString says that you can detect
        // whether GrayString is needed by saying
        //
        //      if (GetSysColor(COLOR_GRAYTEXT) == 0) {
        //          GrayString(...);
        //      } else {
        //          TextOut(...);
        //      }
        //
        // This is incorrect.  The correct test is the one below, which
        // also catches bad color combinations on color devices.
        //
        if (clrText == clrBack) {
            SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
            GrayString(hDC, GetStockObject(GRAY_BRUSH), diflGrayStringProc ,
                       (LPARAM)(LPVOID)&di, 0,
                        lpdis->rcItem.left + DX_TTBITMAP,
                        lpdis->rcItem.top,
                        lpdis->rcItem.right - lpdis->rcItem.left - DX_TTBITMAP,
                        lpdis->rcItem.bottom - lpdis->rcItem.top);
        } else {
            TabbedTextOut(hDC, lpdis->rcItem.left + DX_TTBITMAP, lpdis->rcItem.top, lpsz, lstrlen(lpsz), cTabsList, pTabs, DX_TTBITMAP);
        }

        // and the bitmap if needed
        if (!IsSpecialBpfdi((BPFDI)lpdis->itemData))
        {
            if (((BPFDI)(lpdis->itemData))->bTT) {
                hdcMem = CreateCompatibleDC(hDC);
                if (hdcMem) {
                    hOld = SelectObject(hdcMem, hbmFont);

                    dy = ((lpdis->rcItem.bottom - lpdis->rcItem.top) - DY_TTBITMAP) / 2;

                    BitBlt(hDC, lpdis->rcItem.left, lpdis->rcItem.top + dy,
                        DX_TTBITMAP, DY_TTBITMAP, hdcMem, 0,
                        lpdis->itemState & ODS_SELECTED ? 0 : DY_TTBITMAP, SRCCOPY);

                    if (hOld)
                        SelectObject(hdcMem, hOld);
                    DeleteDC(hdcMem);
                }
            }
        }

        SetTextColor(hDC, rgbText);
        SetBkColor(hDC, rgbBack);

        if (lpdis->itemState & ODS_FOCUS) {
            DrawFocusRect(hDC, &lpdis->rcItem);
        }
    }
}
#undef lpsz
#undef pTabs


/** MeasureItemFontList
 *
 *  Answer the WM_MEASUREITEM message sent from the font list box or
 *  font combo box. shared between the toolbar combo box code and
 *  the font preview property sheet
 *
 *  Entry:
 *      lpmi -> LPMEASUREITEMSTRUCT describing object to be measured
 *
 *  Returns:
 *      TRUE.
 *
 *      lpmi->itemHeight filled with actual item height.
 */

LONG WINAPI MeasureItemFontList(LPMEASUREITEMSTRUCT lpmi)
{
    FunctionName(MeasureItemFontList);
    lpmi->itemHeight = dyItem;
    return TRUE;
}


/** GetItemFontInfo
 *
 *  Returns font information for the current selection in the given
 *  listbox/combobox.
 *
 *  Entry:
 *      hwndList == handle to listbox or combo box to fill
 *                  if NULL, then AUTO is assumed
 *      fListBox == TRUE if hwndList is a listbox, FALSE if a combo box
 *      hProps   == property handle
 *      lpFnt    -> PROPFNT structure (filled in upon return)
 *
 *  Returns:
 *      LB_ERR/CB_ERR on error, index of current selection otherwise
 */

int WINAPI GetItemFontInfo(HWND hwndList, BOOL fListBox, HANDLE hProps, LPPROPFNT lpFnt)
{
    DWORD_PTR dw;
    int index;
    FunctionName(GetItemFontInfo);

    /*
     * Get font defaults;  the nice thing about this call is that
     * it also takes care of calling ChooseBestFont if AUTOSIZE is set,
     * which means we can tell GetFont() to not bother.
     */
    PifMgr_GetProperties(hProps, MAKELP(0,GROUP_FNT),
                  lpFnt, SIZEOF(PROPFNT), GETPROPS_NONE);

    dw = GetFont(hwndList, fListBox, NULL);
    if (IsSpecialBpfdi((BPFDI)dw))
    {
        return 0;
    }

    index = ((BPFDI)dw)->Index;
    if (index == 0)
        lpFnt->flFnt |= FNT_AUTOSIZE;
    else if (index > 0)
        lpFnt->flFnt &= ~FNT_AUTOSIZE;

    /*
     * Fill the caller's PROPFNT structure with all the font goodies (if any)
     *
     * Note that this does nothing if we ended up picking the AUTO font.
     */
    SetFont(lpFnt, (BPFDI)dw);

    return index;
}


/** MatchCurrentFont
 *
 *  Locates the current font in the indicated list box and
 *  makes him the current selection.
 *
 *  If we are in auto-mode, then "Auto" is selected.
 *
 *  Entry:
 *      hwndList == handle to listbox or combo box
 *      fListBox == TRUE if hwndList is a listbox, FALSE if a combo box
 *      lpFnt    -> PROPFNT structure
 *
 *  Returns:
 *      TRUE if the current font was found and selected.
 */
BOOL WINAPI MatchCurrentFont(HWND hwndList, BOOL fListBox, LPPROPFNT lpFnt)
{
    BPFDI bpfdi;
    DWORD dwCount, dwIndex;
    BOOL  fCurFontIsTt = !!(lpFnt->flFnt & FNT_TT);
    FunctionName(MatchCurrentFont);

    if (lpFnt->flFnt & FNT_AUTOSIZE) {
        /*
         * SORTING-SENSITIVE!  This assumes that "Auto" is at the top
         * of the list.
         */
        lcbSetCurSel(hwndList, fListBox, 0);
        return TRUE;
    }
    dwCount = lcbGetCount(hwndList, fListBox);
    for (dwIndex = 0; dwIndex < dwCount; dwIndex++) {

        bpfdi = lcbGetBpfdi(hwndList, fListBox, dwIndex);

        if (!IsSpecialBpfdi(bpfdi)) {
            // bpfdi = (BPFDI)((DWORD)bpfdi + (DWORD)lpCache);
            if (bpfdi->fdiWidthActual  == lpFnt->cxFontActual &&
                bpfdi->fdiHeightActual == lpFnt->cyFontActual &&
                fCurFontIsTt == (bpfdi->fdiHeightReq != 0)) {

                    lcbSetCurSel(hwndList, fListBox, dwIndex);
                    return TRUE;
            }
        }
    }
    return FALSE;
}

/** AddRasterFontsToFontList
 *
 *  Enumerate the available dimensions for our OEM raster font
 *  and add them to the list or combo box.
 *
 *  Entry:
 *      hwndList        =  List box or combo box to fill with info
 *      fListBox        =  TRUE if hwndList is a listbox, FALSE if a combo box
 *      lpszRasterFaceName
 *
 *  Returns:
 *      TRUE if fonts were enumerated to completion.
 *      FALSE if enumeration failed.  (Out of memory.)
 *
 */
BOOL AddRasterFontsToFontListA(HWND hwndList, BOOL fListBox,
                                       LPCSTR lpszRasterFaceName, INT CodePage)
{
    HDC     hDC;
    BOOL    fSuccess;
    FNTENUMINFO FntEnum;
    FunctionName(AddRasterFontsToFontList);

    hDC = GetDC(hwndList);
    if (!hDC) return FALSE;

    FntEnum.hwndList = hwndList;
    FntEnum.fListBox = fListBox;
    FntEnum.CodePage = CodePage;
    fSuccess = EnumFontFamiliesA(hDC,
                                lpszRasterFaceName,
                                (FONTENUMPROCA)RasterFontEnum,
                                (LPARAM)&FntEnum);
    ReleaseDC(hwndList, hDC);
    return TRUE;
}


/** RasterFontEnum
 *
 *  FONTENUMPROC for enumerating all available dimensions of the OEM
 *  raster font.
 *
 *  This routine is used to load the logical and physical font
 *  dimensions cache with information about raster fonts.
 *
 *  Entry:
 *      lpelf           \
 *      lpntm            > from EnumFonts (see SDK)
 *      nFontType       /
 *      hwndList        =  List box or combo box to fill with info
 *      fListBox        =  TRUE if hwndList is a listbox, FALSE if a combo box
 *
 *  Returns:
 *      TRUE to continue enumeration.
 *      FALSE to stop enumeration.  (Out of memory.)
 */

int CALLBACK RasterFontEnum(ENUMLOGFONTA *lpelf, NEWTEXTMETRICA *lpntm, int nFontType, LPARAM lParam)
{
#define fListBox  (((LPFNTENUMINFO)lParam)->fListBox)
#define hwndList  (((LPFNTENUMINFO)lParam)->hwndList)
#define CodePage (((LPFNTENUMINFO)lParam)->CodePage)
#define lpLogFont (&(lpelf->elfLogFont))
    FunctionName(RasterFontEnum);

    /*
     * We only care about OEM fixed-pitch fonts.
     */
    if (lpLogFont->lfCharSet != OEMCharsetFromCP(CodePage)
        || (lpLogFont->lfPitchAndFamily & (TMPF_TRUETYPE | TMPF_FIXED_PITCH))
            != TMPF_FIXED_PITCH)
        return TRUE;

    return AddToFontListCache(hwndList,
                              fListBox,
                              0, 0,
                              lpLogFont->lfHeight,
                              lpLogFont->lfWidth,
                              CodePage) != BPFDI_CANCEL;
#undef lpLogFont
#undef fListBox
#undef hwndList
#undef CodePage
}

/** AddToFontListCache
 *
 *  Adds an entry to the font dimension information cache,
 *  growing the cache if necessary.
 *
 *  It also adds the entry to the indicated list box, provided
 *  the entry is not a duplicate.
 *
 *  Returns:
 *      BPFDI of the recently-added font, or BPFDI_CANCEL if out of memory.
 *
 *  Overview:
 *      (1) Grow the cache if necessary.
 *      (2) Add the information to the list/combo box.
 *      (3) Add the information to the cache.
 */
BPFDI AddToFontListCache(HWND hwndList,
                         BOOL fListBox,
                         UINT uHeightReq,
                         UINT uWidthReq,
                         UINT uHeightActual,
                         UINT uWidthActual,
                         UINT uCodePage)
{
    LPVOID  hCache;
    LONG_PTR lCacheSave;
    DWORD   dwIndex, ifdi;
    BPFDI   bpfdi;
    TCHAR   szBuf[MAXDIMENSTRING];
    int     idx;
    FunctionName(AddToFontListCache);
    ASSERT( !((uHeightReq==0) && (uWidthReq==0) && (uHeightActual==0) && (uWidthActual==0)) );
    /* Reject too-large fonts out of hand. */
    if (uHeightActual > MAX_FONT_HEIGHT) {
        return BPFDI_IGNORE;
    }

    /*
     * FIRST, determine whether this font entry has already been cached
     */

    // we maintain two set of cache entries in case we have two code page
    // to support
    // 
    idx = IsBilingualCP(uCodePage) ? 1 : 0; 
    
    for (ifdi = 0, bpfdi = (BPFDI)((DWORD_PTR)lpCache + bpfdiStart[idx]); ifdi < cfdiCache[idx]; ++ifdi, ++bpfdi)
    {
        if (bpfdi->fdiWidthReq == uWidthReq &&
            bpfdi->fdiHeightReq == uHeightReq &&
            bpfdi->fdiWidthActual == uWidthActual &&
            bpfdi->fdiHeightActual == uHeightActual)
                goto UpdateListCombo;
    }

    /*
     * Grow the cache if necessary.
     */
    if (cfdiCache[idx] >= cfdiCacheActual[idx]) {

        /*
         * save offset from beginning of cache
         */
        bpfdi = (BPFDI)((DWORD_PTR)bpfdi - (DWORD_PTR)lpCache);

        /*
         * save current lpCache value so can adjust entries in listbox
         * when we're done...
         */
        lCacheSave = (LONG_PTR)lpCache;
        hCache = LocalReAlloc( lpCache,
        (cfdiCacheActual[0] + cfdiCacheActual[1] + FDI_TABLE_INC) *
        SIZEOF(FONTDIMENINFO), LMEM_ZEROINIT|LMEM_MOVEABLE );
        if (!hCache)
            return BPFDI_CANCEL;
        lpCache = hCache;
        
        if (!idx && IsBilingualCP(g_uCodePage)) {
            /*
             * We need to shift 2nd cache before using expanded 1st chache
             */
            BPFDI bpfdi2;
            for (ifdi = cfdiCache[1],
                              bpfdi2 = (BPFDI)((DWORD_PTR)lpCache + bpfdiStart[1]) + ifdi - 1 + FDI_TABLE_INC ;
                                                  ifdi ; ifdi--, bpfdi2--) {
                *bpfdi2 = *(bpfdi2 - FDI_TABLE_INC);
            }
            bpfdiStart[1] += FDI_TABLE_INC;
        }
        /* restore bpfdi from saved offset */
        bpfdi = (BPFDI)((DWORD_PTR)lpCache + (DWORD_PTR)bpfdi);
        cfdiCacheActual[idx] += FDI_TABLE_INC;

        /*
         * Convert lCacheSave to an offset...
         */
        lCacheSave = (LONG_PTR)lpCache - lCacheSave;

        if (lCacheSave)
        {
            /*
             * Now, adjust each entry in the listbox to account for the new
             * relocated cache position..
             */

            dwIndex = lcbGetCount( hwndList, fListBox );
            for( ifdi = 0; ifdi < dwIndex; ifdi++ )
            {
                LONG_PTR lBpfdi;

                lBpfdi = (LONG_PTR)lcbGetItemDataPair( hwndList, fListBox, ifdi );
                if (!IsSpecialBpfdi((BPFDI)lBpfdi))
                {
                    lBpfdi += lCacheSave;
                    lcbSetItemDataPair( hwndList, fListBox, ifdi, lBpfdi, ((BPFDI)lBpfdi)->bTT );
                }
            }
        }
    }

    /*
     * Now add the information to the cache.  All the casting on bpfdiCache
     * is just to inhibit a bogus compiler complaint.
     */
    bpfdi->fdiWidthReq  = uWidthReq;
    bpfdi->fdiHeightReq = uHeightReq;

    bpfdi->fdiWidthActual  = uWidthActual;
    bpfdi->fdiHeightActual = uHeightActual;

    cfdiCache[idx]++;

  UpdateListCombo:

    if (hwndList) {
        /*
         * Add the string to the list/combo box if it isn't there already.
         */
        wsprintf(szBuf, szDimension, uWidthActual, uHeightActual);

        dwIndex = lcbFindStringExact(hwndList, fListBox, szBuf);

        if (IsDlgError(dwIndex)) {
            /*
             * Not already on the list.  Add it.
             */
            dwIndex = lcbAddString(hwndList, fListBox, szBuf);

            if (IsDlgError(dwIndex)) {
                return BPFDI_CANCEL;
            }
            lcbSetItemDataPair(hwndList, fListBox, dwIndex,
                               bpfdi, uHeightReq);
        }
    }
    return bpfdi;
}


/** AddTrueTypeFontsToFontListA
 *
 *  To avoid rasterizing all the fonts unnecessarily, we load the
 *  information from the szTTCacheSection font cache.
 *
 *  Note that the cache information is not validated!  We just
 *  assume that if the value is in the cache, it is valid.
 *
 *  Entry:
 *      hwndList        =  List box or combo box to fill with info
 *      fListBox        =  TRUE if hwndList is a listbox, FALSE if a combo box
 *      lpszTTFaceName
 *
 *  Returns:
 *      TRUE if fonts were enumerated to completion.
 *      FALSE if enumeration failed.  (Out of memory.)
 *
 *  Caveats:
 *      The ParseIniWords call assumes that the values were written
 *      by AddOneNewTrueTypeFontToFontList, who wrote them out so
 *      that a single call to ParseIniWords will read the height and
 *      width directly into a dwHeightWidth.
 *
 *      Similarly, the second ParseIniWords reads the item directly into
 *      a dwHeightWidth.
 */

BOOL AddTrueTypeFontsToFontListA(HWND hwndList, BOOL fListBox,
                                        LPSTR lpszTTFaceName, INT CodePage)
{
    LPTSTR  pszBuf;
    LPTSTR  pszBufNew;
    LPTSTR  psz;
    LPTSTR  lpszNext;
    DWORD   dwHWReq;
    DWORD   dwHWActual;
    BOOL    fSuccess;
    DWORD   cchBuf;
    DWORD   cchActual;
    int     i;
    int     idx = IsBilingualCP(CodePage) ? 1 : 0;
    
    FunctionName(AddTrueTypeFontsToFontListA);

    /*
     * See if we can load everything out of the szTTCacheSection.
     *
     * There is no API to get the size of a profile string, so we
     * have to fake it by reading, reallocing, and reading again
     * until it all fits.
     *
     * The initial value of 1024 characters means that we can handle
     * up to 128 font sizes.  A comfortable number, we hope.
     */

    cchBuf = 1024;
    cchActual = 0;
    pszBufNew = (LPTSTR)LocalAlloc( LPTR, cchBuf*SIZEOF(TCHAR) );

    while (pszBufNew) {
        pszBuf = pszBufNew;
        cchActual = GetPrivateProfileString(szTTCacheSection[idx], NULL,
                                         c_szNULL, pszBuf, cchBuf, szSystemINI);
        if (cchActual < cchBuf - 5) goto Okay;

        cchBuf += 512;
        pszBufNew = (LPTSTR)LocalReAlloc( pszBuf, cchBuf*SIZEOF(TCHAR), LMEM_MOVEABLE|LMEM_ZEROINIT );
    }

    /* Bleargh.  Too much stuff in the cache.  Punt it and start anew. */
    goto FreshStart;

Okay:

    fSuccess = FALSE;

    /*
     *  In the time between flushing the cache and reloading it here,
     *  a handful of fonts may have gotten added to the cache due to
     *  WinOldAp trying to realize the font it got back.  So consider the
     *  font cache decent if there are at least ten fonts in it.
     */
    if (cchActual >= 4 * 10) {

        /*
         * We found cache information.  Party away.
         */

        psz = pszBuf;
        while (*psz) {

            if (ParseIniWords(psz, (PWORD)&dwHWReq, 2, &lpszNext) != 2 ||
                GetIniWords(szTTCacheSection[idx], psz,
                            (PWORD)&dwHWActual, 2, szSystemINI) != 2) {
                /* Font cache looks bogus.  Start with a new one. */
                goto FreshStart;
            }

            if (AddToFontListCache(hwndList, fListBox,
                                   (UINT)HIWORD(dwHWReq),
                                   (UINT)LOWORD(dwHWReq),
                                   (UINT)HIWORD(dwHWActual),
                                   (UINT)LOWORD(dwHWActual),
                                   CodePage) == BPFDI_CANCEL)
                goto E0;
                
            psz = (LPTSTR)(lpszNext + 1);       /* Skip past the NUL */
        }

    } else {
FreshStart:
        /* Blast the old cache, just make sure we have a clean slate */
        WritePrivateProfileString(szTTCacheSection[idx], NULL, NULL, szSystemINI);

        /* No cache available.  Need to build one. */
        for (i = 0 ; i < NUMINITIALTTHEIGHTS; i++) {
            if (rgwInitialTtHeights[i]) {
                AddOneNewTrueTypeFontToFontListA(hwndList, fListBox,
                                                0, (UINT)rgwInitialTtHeights[i],
                                                lpszTTFaceName, CodePage);
            }
        }
    }

    fSuccess = TRUE;
E0:
    EVAL(LocalFree(pszBuf) == NULL);
    return fSuccess;
}


/** AddOneNewTrueTypeFontToFontListA
 *
 *  Given height and width, synthesize a TrueType font with those
 *  dimensions and record the actual font height and width in
 *  the persistent font cache, as well as a FDI.
 *
 *  Entry:
 *      hwndList        =  List box or combo box to fill with info
 *      fListBox        =  TRUE if hwndList is a listbox, FALSE if a combo box
 *      wHeight         =  Desired font height
 *      wWidth          =  Desired font width (can be zero for "default")
 *      lpszTTFaceName
 *
 *  Returns:
 *      BPFDI of font dimension info, or BPFDI_CANCEL on failure.
 *
 *  Caveats:
 *      The wsprintf assumes that the fdiWidthReq and
 *      fdiHeightReq fields appear in the indicated order,
 *      because the values will be read into a dwHeightWidth later.
 *
 *      Similarly for the WriteIniWords.
 */

BPFDI AddOneNewTrueTypeFontToFontListA(HWND hwndList,
                                      BOOL fListBox,
                                      UINT uWidth, UINT uHeight,
                                      LPSTR lpszTTFaceName, INT CodePage)
{
    BPFDI   bpfdi;
    HDC     hDC;
    HFONT   hFont;
    SIZE    sSize;
    HFONT   hFontPrev;
    DWORD   dwHeightWidth;
    TCHAR   szBuf[MAXDIMENSTRING];
    static const TCHAR szPercentDSpacePercentD[] = TEXT("%d %d");
    int     idx;
    BYTE    bCharset;
    DWORD   fdwClipPrecision;
    FunctionName(AddOneNewTrueTypeFontToFontListA);

    bpfdi = BPFDI_CANCEL;

    hDC = GetDC(NULL);          /* Get a screen DC */
    if (!hDC) goto E0;
    
    // choose charset, clip precision based on codepage
    // 0xFE is a hack for japanese platform
    //
    bCharset = (CodePage == CP_JPN? 0xFE: OEMCharsetFromCP(CodePage));
    
    if (CodePage == CP_US)
        fdwClipPrecision = CLIP_DEFAULT_PRECIS|(g_uCodePage == CP_WANSUNG? CLIP_DFA_OVERRIDE: 0);
    else
        fdwClipPrecision = CLIP_DEFAULT_PRECIS;

    hFont = CreateFontA((INT)uHeight, (INT)uWidth, 0, 0, 0, 0, 0, 0,
               bCharset, OUT_TT_PRECIS,
               fdwClipPrecision, 0, FIXED_PITCH | FF_DONTCARE, lpszTTFaceName);
               
    if (!hFont) goto E1;

    hFontPrev = SelectObject(hDC, hFont);
    if (!hFontPrev) goto E2;

    if (GetTextExtentPoint32( hDC, szZero, 1, &sSize ))
    {
        dwHeightWidth = (sSize.cy << 16) | (sSize.cx & 0x00FF);
    }
    else
    {
        dwHeightWidth = 0;
    }

    if (!dwHeightWidth) goto E3;

    if ( IsBilingualCP(CodePage) && (HIWORD(dwHeightWidth)%2) )
        goto E3;

    wsprintf(szBuf, szPercentDSpacePercentD, uWidth, uHeight);

    idx = IsBilingualCP(CodePage) ? 0 : 1;
    
    WriteIniWords(szTTCacheSection[idx], szBuf, (PWORD)&dwHeightWidth, 2, szSystemINI);

    bpfdi = AddToFontListCache(hwndList, fListBox, uHeight, uWidth,
                               (UINT)sSize.cy, (UINT)sSize.cx,
                               CodePage);

E3: SelectObject(hDC, hFontPrev);
E2: DeleteObject(hFont);
E1: ReleaseDC(0, hDC);
E0: return bpfdi;

}


/** GetFont
 *
 *  Returns the BPFDI corresponding to the currently selected font in
 *  the indicated list or combo box, or BPFDI_CANCEL on error.
 *
 *  Entry:
 *      hwndList == handle to listbox or combo box to fill
 *                  if NULL, then AUTO font calculation is assumed
 *      fListBox == TRUE if hwndList is a listbox, FALSE if a combo box
 *      pfi      -> FNTINFO structure
 *                  if pfi is NULL, then AUTO font calculation is ignored
 *  Returns:
 *      BPFDI of the current selection, or BPFDI_CANCEL on error.
 */
DWORD_PTR GetFont(HWND hwndList, BOOL fListBox, PFNTINFO pfi)
{
    DWORD dwIndex = 0;
    BPFDI bpfdi = BPFDI_CANCEL;
    FunctionName(GetFont);

    if (!hwndList) {            // just do AUTO calculations
        if (!pfi)
            goto Exit;          // whoops, can't even do those
        goto ChooseBest;
    }
    dwIndex = lcbGetCurSel(hwndList, fListBox);
    if (!IsDlgError(dwIndex)) {

        if (pfi)
            pfi->fntProposed.flFnt &= ~FNT_AUTOSIZE;

        bpfdi = lcbGetBpfdi(hwndList, fListBox, dwIndex);

        if (bpfdi == BPFDI_AUTO && pfi) {
            pfi->fntProposed.flFnt |= FNT_AUTOSIZE;

ChooseBest:
            bpfdi = ChooseBestFont((UINT)pfi->winOriginal.cxCells,
                                   (UINT)pfi->winOriginal.cyCells,
                                   (UINT)pfi->winOriginal.cxClient,
                                   (UINT)pfi->winOriginal.cyClient,
                                   (UINT)pfi->fntProposed.flFnt,
                                    (INT)pfi->fntProposed.wCurrentCP);
        }
        // Set the index of the current selection (HIWORD
        // of the return code) to LB_ERR if there's an error

        if (bpfdi == BPFDI_CANCEL)
            dwIndex = (DWORD)LB_ERR;
    }
  Exit:
    if (!IsSpecialBpfdi(bpfdi))
    {
        bpfdi->Index = dwIndex;
    }

    return (DWORD_PTR)bpfdi;
}


/** SetFont
 *
 *  Copies data from the given BPFDI to the given PROPFNT structure.
 *
 *  Entry:
 *      lpFnt = pointer to PROPFNT structure
 *      bpfdi = based pointer to a FONTDIMENINFO structure;
 *              if a special BPFDI_* constant, no font info is changed
 *  Returns:
 *      Nothing
 */
void SetFont(LPPROPFNT lpFnt, BPFDI bpfdi)
{
    FunctionName(SetFont);

    if (!IsSpecialBpfdi(bpfdi)) {

        lpFnt->flFnt &= ~(FNT_RASTER | FNT_TT);

        if (bpfdi->fdiHeightReq == 0) {
            /* Raster font */
            lpFnt->flFnt |= FNT_RASTER;
            lpFnt->cxFont = lpFnt->cxFontActual = (WORD) bpfdi->fdiWidthActual;
            lpFnt->cyFont = lpFnt->cyFontActual = (WORD) bpfdi->fdiHeightActual;
        } else {
            /* TrueType font */
            lpFnt->flFnt |= FNT_TT;
            lpFnt->cxFont = (WORD) bpfdi->fdiWidthReq;
            lpFnt->cyFont = (WORD) bpfdi->fdiHeightReq;
            lpFnt->cxFontActual = (WORD) bpfdi->fdiWidthActual;
            lpFnt->cyFontActual = (WORD) bpfdi->fdiHeightActual;
        }
    }
}


/*  AspectScale
 *
 *  Performs the following calculation in LONG arithmetic to avoid
 *  overflow:
 *      return = n1 * m / n2
 *  This can be used to make an aspect ration calculation where n1/n2
 *  is the aspect ratio and m is a known value.  The return value will
 *  be the value that corresponds to m with the correct apsect ratio.
 */

//
// <This is defined as a macro for Win32 >
//

/*  AspectPoint
 *
 *  Scales a point to be preview-sized instead of screen-sized.
 *  Depends on the global vars cxScreen and cyScreen established at init.
 */

void AspectPoint(LPRECT lprcPreview, LPPOINT lppt)
{
    FunctionName(AspectPoint);
    lppt->x = AspectScale(lprcPreview->right, cxScreen, lppt->x);
    lppt->y = AspectScale(lprcPreview->bottom, cyScreen, lppt->y);
}

/*  AspectRect
 *
 *  Scales a rectangle to be preview-sized instead of screen-sized.
 *  Depends on the global vars cxScreen and cyScreen established at init.
 */

void AspectRect(LPRECT lprcPreview, LPRECT lprc)
{
    FunctionName(AspectRect);
    AspectPoint(lprcPreview, &((LPPOINT)lprc)[0]); /* Upper left corner */
    AspectPoint(lprcPreview, &((LPPOINT)lprc)[1]); /* Lower right corner */
}

/** CreateFontFromBpfdi
 *
 *  Given a BPFDI, create a font that corresponds to it.
 *
 *  Entry:
 *      bpfdi       -> FDI describing the font we want to create
 *      pfi         -> proposed font info structure
 *
 *  Returns:
 *      HFONT that was created.
 */
HFONT CreateFontFromBpfdi(BPFDI bpfdi, PFNTINFO pfi)
{
    HFONT hf;
    int   fdwClipPrecision;
    BYTE  bT2Charset;

    FunctionName(CreateFontFromBpfdi);

    // a hack for japanese charset
    bT2Charset = (pfi->fntProposed.wCurrentCP == CP_JPN? 
                  0xFE: OEMCharsetFromCP(pfi->fntProposed.wCurrentCP));
    
    if (pfi->fntProposed.wCurrentCP == CP_US)
        fdwClipPrecision = CLIP_DEFAULT_PRECIS|(g_uCodePage == CP_WANSUNG? CLIP_DFA_OVERRIDE: 0);
    else
        fdwClipPrecision = CLIP_DEFAULT_PRECIS;
        
    if (bpfdi->fdiHeightReq == 0) {
        /* Raster font */
        hf = CreateFontA(bpfdi->fdiHeightActual, bpfdi->fdiWidthActual,
            0, 0, 0, 0, 0, 0, (BYTE)OEMCharsetFromCP(pfi->fntProposed.wCurrentCP), 
            OUT_RASTER_PRECIS, fdwClipPrecision,
            0, FIXED_PITCH | FF_DONTCARE, pfi->fntProposed.achRasterFaceName);
    } else {
        /* a TrueType font */
        hf = CreateFontA(bpfdi->fdiHeightReq, bpfdi->fdiWidthReq,
            0, 0, 0, 0, 0, 0, (BYTE)bT2Charset, OUT_TT_PRECIS, fdwClipPrecision,
            0, FIXED_PITCH | FF_DONTCARE, pfi->fntProposed.achTTFaceName);
    }

#ifdef DEBUG
    {
    /*
     * In DEBUG, we double-check that the font really has the dimensions
     * it's supposed to have.
     *
     */
    HDC     hDC;
    HFONT   hFontPrev;
    SIZE    sSize;

    hDC = GetDC(NULL);          /* Get a screen DC */
    if (!hDC) goto E0;

    hFontPrev = SelectObject(hDC, hf);
    if (!hFontPrev) goto E1;

    if (!GetTextExtentPoint32( hDC, szZero, 1, &sSize ))
        goto E3;

    ASSERTTRUE((sSize.cy == (LONG)bpfdi->fdiHeightActual)
               &&
               (sSize.cx == (LONG)bpfdi->fdiWidthActual));

E3: SelectObject(hDC, hFontPrev);
E1: ReleaseDC(0, hDC);
E0: ;
    }
#endif

    return hf;
}


/** FontSelInit
 *
 *  Obtain the various font selection penalties from SYSTEM.INI
 *  and force the values into range.
 *
 *  Entry:
 *      rgwInitialTtHeights contains default values for sizes.
#ifdef  CUSTOMIZABLE_HEURISTICS
 *      rgpnlPenalties contains default values for penalties.
 *      rgwAspectLimits contains default values for aspect ratio limits.
#endif
 *
 *  Exit:
 *      rgwInitialTtHeights contains actual values for sizes.
#ifdef  CUSTOMIZABLE_HEURISTICS
 *      rgpnlPenalties contains actual values for penalties, forced into range.
 *      rgwAspectLimits contains actual values for aspect ratio limits.
#endif
 */

void FontSelInit(void)
{
    FunctionName(FontSelInit);

    GetIniWords(szNonWinSection, szTTInitialSizes,
                rgwInitialTtHeights, SIZEOF(rgwInitialTtHeights)/SIZEOF(WORD), szSystemINI);

//
// BUGBUG: since rgpnlPenlaties is no longer an array of WORDS, this
// won't work and will need to be changed if we turn back on "CUSTOMIZABLE_HEURISTICS"
//

#ifdef  CUSTOMIZABLE_HEURISTICS
    GetIniWords(szNonWinSection, szTTHeuristics,
                rgpnlPenalties, SIZEOF(rgpnlPenalties)/SIZEOF(WORD), szSystemINI);

  {
    PINT pi;
    for (pi = &rgpnlPenalties[0]; pi < &rgpnlPenalties[NUMPENALTIES]; ++pi) {
        *pi = min(MAXPENALTY, max(MINPENALTY, *pi));
    }
  }

    GetIniWords(szNonWinSection, szTTNonAspectMin, (PWORD)&ptNonAspectMin,
        SIZEOF(ptNonAspectMin)/SIZEOF(WORD), szSystemINI);
#endif
}


/** GetTrueTypeFontTrueDimensions
 *
 *  Convert logical dimensions for a TrueType font into physical
 *  dimensions.  If possible, we get this information from the
 *  font dimension cache, but in the case where this is not possible,
 *  we synthesize the font and measure him directly.
 *
 *  Entry:
 *      dxWidth  = logical font width
 *      dyHeight = logical font height
 *
 *  Returns:
 *      BPFDI pointing to dimension information, or BPFDI_CANCEL on failure.
 */

BPFDI GetTrueTypeFontTrueDimensions(UINT dxWidth, UINT dyHeight, INT CodePage)
{
    IFDI    ifdi;
    BPFDI   bpfdi;
    int     idx;

    FunctionName(GetTrueTypeFontTrueDimensions);

    idx = IsBilingualCP( CodePage )? 1 : 0;
    for ( ifdi = 0, bpfdi = (BPFDI)((DWORD_PTR)lpCache + bpfdiStart[idx]);  
                    ifdi < cfdiCache[idx];  ifdi++, bpfdi++ )
    {
        if (bpfdi->fdiWidthReq  == dxWidth &&
            bpfdi->fdiHeightReq == dyHeight) {
            return bpfdi;
        }
    }

    /*
     * The font dimensions have not been cached.  We have to create it.
     */
    return (BPFDI)AddOneNewTrueTypeFontToFontListA(0, 0, dxWidth, dyHeight,
                                                 szTTFaceName[idx], CodePage);
}


/** FindFontMatch
 *
 *  Look for a font that matches the indicated dimensions, creating
 *  one if necessary.
 *
 *  But we never create a font which is too narrow or too short.
 *  The limits are controlled by the ptNonAspectMin variable.
 *
 *  Entry:
 *      dxWidth  = desired font width
 *      dyHeight = desired font height
 *      fPerfect = see below
 *
 *          If fPerfect is TRUE, then a perfect match is requested
 *          from the font cache (we should not try to synthesize a font).
 *          In which case, the sign of dyHeight determines whether a
 *          raster font (positive) or TrueType font (negative) is
 *          desired.  If a perfect match cannot be found, then we
 *          return BPFDI_CANCEL.
 *
 *  Returns:
 *      BPFDI of of the font that matches the best.
 *      BPFDI_CANCEL if no font could be found.
 */
BPFDI FindFontMatch(UINT dxWidth, UINT dyHeight, LPINT lpfl, INT CodePage)
{
    int     fl;
    IFDI    ifdi;
    BPFDI   bpfdi;
    BPFDI   bpfdiBest = BPFDI_CANCEL;
    PENALTY pnlBest = SENTINELPENALTY;
    int     idx;

    FunctionName(FindFontMatch);

    fl = *lpfl;
    /*
     * First, see if a perfect match already exists.
     */
    idx = IsBilingualCP( CodePage ) ? 1 : 0;
    for ( ifdi = 0, bpfdi = (BPFDI)((DWORD_PTR)lpCache+bpfdiStart[idx]);  
                    ifdi < cfdiCache[idx];  ifdi++, bpfdi++ )
    {

        if (fl & FFM_RESTRICTED) {
            /* Deal with the restrictions.
             * Reject the font if it is raster but we want TTONLY, or v.v.
             *
             * The condition below reads as
             *
             *      If (is a raster font != want a raster font)
             */
            if (!bpfdi->fdiHeightReq != (fl == FFM_RASTERFONTS)) {
                continue;
            }
        }
        if (bpfdi->fdiHeightActual == dyHeight && bpfdi->fdiWidthActual == dxWidth) {
            *lpfl = FFM_PERFECT;
            return bpfdi;
    }   }

    if (fl != FFM_TTFONTS)
        return BPFDI_CANCEL;
    /*
     * We got here if we couldn't find a perfect match.
     *
     * Adjust the requested height and width for aspect ratio
     * constraints.  If adjustments are necessary, trust the height.
     *
     * Comparisons are as WORDs (unsigned) so that a setting of "-1 -1"
     * lets the user forbid all non-aspect ratio fonts.
     */
    if (dyHeight < (UINT)ptNonAspectMin.y || dxWidth < (UINT)ptNonAspectMin.x) {
        dxWidth = 0;
    }
    return GetTrueTypeFontTrueDimensions(dxWidth, dyHeight, CodePage);
}

#ifdef  FONT_INCDEC
/** IncrementFontSize
 *
 *  The user wants to increment or decrement the font size.
 *
 *  Inc and Dec only produce fonts at the standard aspect ratio.
 *
 *  The font sizes are sorted lexicographically (height, then
 *  width); in other words, when searching for the next larger
 *  font, we first try to find a larger font of the same height
 *  but of greater width.  Failing that, we choose the narrowest
 *  font available at the next larger height.  (Reverse everything
 *  if you are decrementing, or if you are left-handed.)
 *
 *  If TrueType fonts are allowed, then we will manually create
 *  a font of appropriate height before beginning the search.
 *  (But we will never create a font of height greater than
 *  MAX_FONT_HEIGHT.)
 *
 *  Fonts which are fatter than they are tall are clearly
 *  mutants, and will be skipped by the search.
 *
 *  Entry:
 *      dySearch == 1 (to increment) or -1 (to decrement)
 *
 *  Exit:
 *      Returns BPFDI pointing to next larger or smaller font,
 *      or BPFDI_CANCEL if no such can be found.
 *
 *  Notes:
 *
 *      The Badness macro computes a value describing how well
 *      the font suits our needs.  It produces a value greater
 *      than 0x7FFFFFFF if the font is completely unsuitable,
 *      and a value less than 0x7FFFFFFF if the font is okay.
 *      Smaller badness values are better.
 *
 *      Badness takes advantage of two's-complement arithmetic.
 *      The extra -1 is so that a perfect match is rejected.
 *
 *      cxFont and cyFont are passed in the order they are so
 *      that they can be converted into BaseSize by a single load.
 *
 */

#define BaseSize()      (MAKELONG(cxFont, cyFont))
#define NewSize(bpfdi)  (MAKELONG((bpfdi)->fdiWidthActual, \
                                  (bpfdi)->fdiHeightActual))
#define Badness(bpfdi)  ((DWORD)((NewSize(bpfdi) - BaseSize()) * dySearch - 1))
#define IsTT(bpfdi)     ((bpfdi)->fdiHeightReq)
#define IsRaster(bpfdi) (!IsTT(bpfdi)))

BPFDI IncrementFontSize(UINT cyFont, UINT cxFont, INT flFnt, INT CodePage)
{
    BPFDI   bpfdi;
    DWORD   dwBadness;
    BPFDI   bpfdiBest;
    DWORD   dwBadnessBest;
    long    dySearch = (flFnt & FNT_GROWTTFONT) ? 1 : -1;
    WORD    dyHeight;
    WORD    ifdi;
    int     idx;

    FunctionName(IncrementFontSize);

    /*
     * If TrueType fonts are allowed, pre-compute some likely candidates.
     *
     * We keep creating bigger and bigger (or smaller and smaller) fonts
     * until we find one that is bigger (smaller) than the one we started
     * at, or until we run into one of the absolute limits on font size.
     */
    if (flFnt & FNT_TTFONTS) {
        for (dyHeight = cyFont; dyHeight > 0 && dyHeight <= MAX_FONT_HEIGHT;
                dyHeight += (int)dySearch) {

            bpfdi = GetTrueTypeFontTrueDimensions(0, dyHeight, CodePage);
            if (IsSpecialBpfdi(bpfdi)) {
                break;                  /* Oof, stop here. */
            }
            if ((NewSize(bpfdi) - BaseSize()) * dySearch > 0) {
                break;                  /* Found a good one */
            }
        }
    }

    /*
     * Okay, we've preloaded the TT fonts (if necessary).  Now go
     * find the lexicographically adjacent font.
     */
    bpfdiBest = BPFDI_CANCEL;           /* No winner found yet */
    dwBadnessBest = (DWORD)0x7FFFFFFF;  /* Bad but not awful */

    idx = IsBilingualCP( CodePage ) ? 1 : 0;
    for ( ifdi = 0, bpfdi = lpCache+bpfdiStart[idx];  ifdi < cfdiCache[idx];  ifdi++, bpfdi++ )
    {

        /*
         * Make sure to reject fonts of the inappropriate type before
         * considering them as new candidates.
         */

        if (IsTT(bpfdi)) {
            if (!(flFnt & FNT_TTFONTS)) continue;
        } else {
            if (!(flFnt & FNT_RASTERFONTS)) continue;
        }

        if (bpfdi->fdiWidthActual > bpfdi->fdiHeightActual) /* Mutant */
            continue;

        dwBadness = Badness(bpfdi);
        if (dwBadness < dwBadnessBest) {
            bpfdiBest = bpfdi;          /* A new winner */
            dwBadnessBest = dwBadness;
        }
    }

    return bpfdiBest;
}
#endif


/** ComputePenaltyFromPair
 *
 *  We have decided whether the desired size is larger or smaller.
 *  Compute the penalty corresponding to the Initial and Scale.
 *
 *  Entry:
 *      ppnlp   -> PENALTYPAIR to apply
 *      dSmaller = the smaller dimension
 *      dLarger  = the larger dimension
 *
 *  Exit:
 *      Returns penalty to apply to the difference in dimensions.
 */
PENALTY ComputePenaltyFromPair(PPENALTYPAIR ppnlp,
                               UINT dSmaller, UINT dLarger)
{
    FunctionName(ComputePenaltyFromPair);
    return (ppnlp->pnlInitial +
            ppnlp->pnlScale - MulDiv(ppnlp->pnlScale, dSmaller, dLarger));
}


/** ComputePenaltyFromList
 *
 *  Compute the penalty depending on whether the desired size
 *  is smaller, equal to, or larger than the actual size.
 *
 *  Entry:
 *      ppnll   -> PENALTYLIST to apply
 *      dActual  = the actual dimension
 *      dDesired = the desired dimension
 *
 *  Exit:
 *      Returns penalty to apply to the difference in dimensions,
 *      choosing between the Overshoot and Shortfall PENALTYPAIRS,
 *      accordingly.
 */
PENALTY ComputePenaltyFromList(PPENALTYLIST ppnll,
                               UINT dActual, UINT dDesired)
{
    FunctionName(ConputePenaltyFromList);
    if (dActual == dDesired)
        return 0;

    if (dActual < dDesired)
        return ComputePenaltyFromPair(&ppnll->pnlpOvershoot, dActual, dDesired);

    return ComputePenaltyFromPair(&ppnll->pnlpShortfall, dDesired, dActual);
}


/** ComputePenalty
 *
 *  Compute the total penalty associated to a window size.
 *
 *  Entry:
 *      dxCells  = width of window in cells
 *      dyCells  = height of window in cells
 *      dxClient = actual horizontal size of window
 *      dyClient = actual vertical   size of window
 *      dxFont   = width of one character in the font
 *      dyFont   = height of one character in the font
 *
 *  Exit:
 *      Returns total penalty associated to a window of the indicated
 *      size with a font of the indicated dimensions.
 */
PENALTY ComputePenalty(UINT cxCells,  UINT cyCells,
                       UINT dxClient, UINT dyClient,
                       UINT dxFont,   UINT dyFont)
{
    FunctionName(ComputePenalty);
    return
        (ComputePenaltyFromList(&pnllX, dxClient, dxFont * cxCells) +
         ComputePenaltyFromList(&pnllY, dyClient, dyFont * cyCells));
}


/** ChooseBestFont
 *
 *  Determine which font looks best for the specified window size
 *  by picking the one which has the smallest penalty.
 *
 *  Entry:
 *      dxCells = width of window in cells
 *      dyCells = height of window in cells
 *      dxClient= width of window we want to fit into
 *      dyClient= height of window we want to fit into
 *      fl      = font pool flags
 *
 *  Returns:
 *      Word offset from lpFontTable of the font we've decided to use.
 *      BPFDI_CANCEL if no font could be found.  (Should never happen.)
 *
 *  NOTE!
 *      We do *not* FontEnum through all the fonts because that would be
 *      too slow.  Instead, we inspect the cache of available font
 *      dimensions, and only after we've chosen the best font do we
 *      load all his other info.
 *
 *      This means that if the user installs new fonts, we won't see
 *      them until the cache is updated on receipt of a WM_FONTCHANGEff
 *      message, or the user either (1) pulls down the font list box,
 *      or (2) calls up the font selection dialog box.
 */

BPFDI ChooseBestFont(UINT cxCells, UINT cyCells, UINT dxClient, UINT dyClient,
                                                         INT fl, INT CodePage)
{
    int     flTemp;
    DWORD    ifdi;
    BPFDI   bpfdi;
    PENALTY pnl;
    UINT    dxWidth, dyHeight;
    BPFDI   bpfdiBest = BPFDI_CANCEL;
    PENALTY pnlBest = SENTINELPENALTY;
    int     idx;
    static int prev_CodePage;  // Only Japan is interested in prev_CodePage.

    FunctionName(ChooseBestFont);

    /*
     * First, synthesize the theoretical best match.
     */
    if (!cxCells)
        cxCells = 80;           // if we get called with no real data,
    if (!cyCells)               // at least try to do something reasonable
        cyCells = 25;

    //
    // In the case where the values passed in don't make sense,
    // we default to raster 8x12.
    //
    dxWidth = (dxClient >= cxCells)? dxClient / cxCells : 8;
    dyHeight = (dyClient >= cyCells)? dyClient / cyCells : 12;

    //
    // Now, if we bad values, make some sense out of bad values for
    // dxClient & dyClient
    //

    if ((dxClient==0) || (dyClient==0))
    {
        dxClient = dxWidth * 80;
        dyClient = dyHeight * 25;
    }

    flTemp = 0;
    if ((fl & FNT_BOTHFONTS) != FNT_BOTHFONTS) {
        flTemp = FFM_RASTERFONTS;
        if (fl & FNT_TTFONTS)
            flTemp = FFM_TTFONTS;
    }
    bpfdi = FindFontMatch(dxWidth, dyHeight, &flTemp, CodePage);
    if (flTemp == FFM_PERFECT)
    {
        prev_CodePage = CodePage;
        return bpfdi;
    }

    idx = IsBilingualCP( CodePage )? 1 : 0;
    for ( ifdi = 0, bpfdi = (BPFDI)((DWORD_PTR)lpCache+bpfdiStart[idx]);  
                    ifdi < cfdiCache[idx];  ifdi++, bpfdi++ )
    {
        // If the font pool is restricted, then only look at like fonts

        if (flTemp)
            if (!bpfdi->fdiHeightReq != (flTemp == FFM_RASTERFONTS))
                continue;

// was ifdef JAPAN (hack)
// to prevent DOS_BOX shrinking which occurs toggling CP437 & CP932,
// just select one size bigger font when change CP437 to CP932
        if (CodePage == 932 && prev_CodePage == 437) {
           if (dxWidth < bpfdi->fdiWidthActual) {
              if (bpfdiBest->fdiWidthActual > bpfdi->fdiWidthActual)
                 bpfdiBest = bpfdi;
              else if (bpfdiBest->fdiWidthActual == bpfdi->fdiWidthActual &&
                       bpfdiBest->fdiHeightActual > bpfdi->fdiHeightActual)
                 bpfdiBest = bpfdi;
           }
           else {
              if (dxWidth == bpfdi->fdiWidthActual) {
                 if (bpfdi->fdiHeightActual > dyHeight &&
                     bpfdiBest->fdiHeightActual > bpfdi->fdiHeightActual)
                    bpfdiBest = bpfdi;
              }
           }
        }
        else 
// was the end of ifdef JAPAN
        {
        pnl = 0;
        if (bpfdi->fdiHeightReq)
            pnl = pnlTrueType;

        pnl += ComputePenalty(cxCells, cyCells,
                              dxClient, dyClient,
                              bpfdi->fdiWidthActual,
                              bpfdi->fdiHeightActual);

        if (pnl <= pnlBest) {
            pnlBest = pnl;
            bpfdiBest = bpfdi;
        }
        }
    }
// was ifdef JAPAN
    prev_CodePage = CodePage;
// was end of ifdef JAPAN
    return bpfdiBest;
}
