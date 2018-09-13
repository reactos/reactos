/***************************************************************************\
*
*  DLGBEGIN.C -
*
*      Dialog Initialization Routines
*
* ??-???-???? mikeke    Ported from Win 3.0 sources
* 12-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

LPVOID __cdecl MakeAResizeDlg(int id,HANDLE h);

BOOL ValidateCallback(HANDLE h);

CONST WCHAR szEDITCLASS[] = TEXT("Edit");
#ifdef USE_MIRRORING
#define MIRRORED_HWND(hwnd)   (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL)
#endif
/***************************************************************************\
* DefShortToInt
*
* Avoid sign extending 16 bit CW2_USEDEFAULT. We need this because the
*  dialog resource template uses SHORT fields to store the coordinates
*  but CreateWindow wants INT values.
*
* History:
* 12/04/96 GerardoB Created
\***************************************************************************/
__inline int DefShortToInt (short s)
{
    if (s == (short)CW2_USEDEFAULT) {
        return (int)(DWORD)(WORD)CW2_USEDEFAULT;
    } else {
        return (int)s;
    }
}
/***************************************************************************\
* BYTE FAR *SkipSz(lpsz)
*
* History:
\***************************************************************************/

PBYTE SkipSz(
    UTCHAR *lpsz)
{
    if (*lpsz == 0xFF)
        return (PBYTE)lpsz + 4;

    while (*lpsz++ != 0) ;

    return (PBYTE)lpsz;
}

PBYTE WordSkipSz(
    UTCHAR *lpsz)
{
    PBYTE pb = SkipSz(lpsz);
    return NextWordBoundary(pb);
}

PBYTE DWordSkipSz(
    UTCHAR *lpsz)
{
    PBYTE pb = SkipSz(lpsz);
    return NextDWordBoundary(pb);
}


/***************************************************************************\
*
* IsFontTooSmall()
*
* If this is a low res device, we need to check if the
* font we're creating is smaller than the system font.
*
\***************************************************************************/
__inline BOOLEAN IsFontTooSmall(LPWSTR szTempBuffer, LPCWSTR lpStrSubst, TEXTMETRIC* ptm)
{
    //
    // For FarEast version, we will allow the font smaller than system font.
    //
    return _wcsicmp(szTempBuffer, lpStrSubst) ||
                (!IS_ANY_DBCS_CHARSET(ptm->tmCharSet) &&
                    (SYSMET(CXICON) < 32 || SYSMET(CYICON) < 32) &&
                    ptm->tmHeight < gpsi->cySysFontChar);
}

// --------------------------------------------------------------------------
//  GetCharsetEnumProc()
//
//  This gets the best asian font for a dialog box.
//
//  1996-Sep-11 hideyukn     Port from Win95-FE
// --------------------------------------------------------------------------
int CALLBACK GetCharsetEnumProc(
    LPLOGFONT     lpLogFont,
    LPTEXTMETRIC  lptm,
    DWORD         nType,
    LPARAM        lpData)
{
    UNREFERENCED_PARAMETER(lptm);
    UNREFERENCED_PARAMETER(nType);

    //
    // Use other than FIXED pitch sysfont when the face name isn't specified.
    //
    if ((lpLogFont->lfPitchAndFamily & 3) == FIXED_PITCH)
    {
        if (!lstrcmpi(lpLogFont->lfFaceName,L"System") ||
            !lstrcmpi(lpLogFont->lfFaceName,L"@System"))
            return TRUE; // try to get another system font metric
    }

    ((LPLOGFONT)lpData)->lfCharSet = lpLogFont->lfCharSet;
    ((LPLOGFONT)lpData)->lfPitchAndFamily = lpLogFont->lfPitchAndFamily;
    return FALSE;
}

UINT GetACPCharSet()
{
    CHARSETINFO csInfo;

    if (!TranslateCharsetInfo((DWORD*)GetACP(), &csInfo, TCI_SRCCODEPAGE)) {
        return DEFAULT_CHARSET;
    }
    return csInfo.ciCharset;
}


/***************************************************************************\
*
* CreateDlgFont()
*
* Create the dialog font described at the given location in a resource
*
\***************************************************************************/

HFONT CreateDlgFont(HDC hdcDlg, LPWORD FAR *lplpstr, LPDLGTEMPLATE2 lpdt, DWORD dwExpWinVer)
{
    LOGFONT     LogFont;
    int         fontheight, fheight;
    HFONT       hOldFont, hFont;
    WCHAR       szTempBuffer[20];
    LPCWSTR     lpStrSubst;
    TEXTMETRIC  tm;
    BOOL        fDeleteFont = FALSE;
    // FE
    BOOL        bWillTryDefaultCharset = FALSE;

    fheight = fontheight = (SHORT)(*((WORD *) *lplpstr)++);

    if (fontheight == 0x7FFF) {
        // a 0x7FFF height is our special code meaning use the message box font
//        return(gpsi->hMsgFont);
        GetObject(gpsi->hMsgFont, sizeof(LOGFONT), &LogFont);
        return(CreateFontIndirect(&LogFont));
    }

    //
    // The dialog template contains a font description! Use it.
    //
    // Fill the LogFont with default values
    RtlZeroMemory(&LogFont, sizeof(LOGFONT));

    fontheight = -MultDiv(fontheight, gpsi->dmLogPixels, 72);
    LogFont.lfHeight = fontheight;

    if (lpdt->wDlgVer)
    {
        LogFont.lfWeight  = *((WORD FAR *) *lplpstr)++;
        LogFont.lfItalic  = *((BYTE FAR *) *lplpstr)++;
        LogFont.lfCharSet = *((BYTE FAR *) *lplpstr)++;
        // FE_SB
        if (IS_DBCS_ENABLED() && LogFont.lfCharSet == ANSI_CHARSET) {
            //
            // When resource compiler generates dialog resource data
            // from DIALOGEX template, it can specify 'charset'. but
            // optional, if it is not specified, it will be filled
            // with 0 (ANSI charset). But, on localized version,
            // User might guess the default will be localized-charset
            //
            // [Dialog Resource File]
            //
            // DIALOGEX ...
            // ...
            // FONT pointsize, typeface, [weight], [italic], [charset]
            //
            if (Is500Compat(dwExpWinVer)) {
                bWillTryDefaultCharset = TRUE;
            } else {
                // #100182
                LogFont.lfCharSet = DEFAULT_CHARSET;
                RIPMSG0(RIP_WARNING, "No CharSet information in DIALOGEX");
            }
        }
        // end FE_SB
    }
    else
    {
        LogFont.lfWeight  = FW_BOLD;
        LogFont.lfCharSet = DEFAULT_CHARSET;
    }


    lpStrSubst = *lplpstr;

    wcsncpycch(LogFont.lfFaceName, lpStrSubst, sizeof(LogFont.lfFaceName) / sizeof(WCHAR));

    //
    // "MS Shell Dlg" should have native character set ---
    // Yes I hate this kind of hard code literals, but there's no way to get
    // font link information.
    // That's the reason why you see this hard coded **** here
    //
    if (IS_DBCS_ENABLED() && _wcsicmp(LogFont.lfFaceName, L"MS Shell Dlg") == 0) {
        LogFont.lfCharSet = GetACPCharSet();
    }

    *lplpstr = (WORD *)DWordSkipSz(*lplpstr);

    if (lpdt->style & DS_3DLOOK)
        LogFont.lfWeight = FW_NORMAL;

TryDefaultCharset:
    if (LogFont.lfCharSet == DEFAULT_CHARSET) {
        if (IS_DBCS_ENABLED()) {
            {
                //
                // Get character set for given facename.
                //
                EnumFonts(hdcDlg, LogFont.lfFaceName,
                          (FONTENUMPROC)GetCharsetEnumProc, (LPARAM)(&LogFont));
                //
                // We already tried default charset.
                //
                bWillTryDefaultCharset = FALSE;
            }

            //
            // [Windows 3.1 FarEast version did this...]
            //
            // Use FW_NORMAL as default for DIALOG template. For DIALOGEX
            // template, we need to respect the value in the template.
            //
            if ((!(lpdt->wDlgVer)) && // not DIALOGEX template ?
                (IS_ANY_DBCS_CHARSET(LogFont.lfCharSet)) && // any FarEast font ?
                (LogFont.lfWeight != FW_NORMAL)) { // already FW_NORMAL ?

                //
                // Set weight to FW_NORMAL.
                //
                LogFont.lfWeight = FW_NORMAL;
            }
        } else {
            LogFont.lfCharSet = GetTextCharset(hdcDlg); // Assume shell charset.
        }
    }

    if (!(hFont = CreateFontIndirect((LPLOGFONT) &LogFont)))
        return(NULL);

    if (!(hOldFont = SelectFont(hdcDlg, hFont)))
        goto deleteFont;

    if (!GetTextMetrics(hdcDlg, &tm)) {
        RIPMSG0(RIP_WARNING, "CreateDlgFont: GetTextMetrics failed");
        goto deleteFont;
    }


    GetTextFace(hdcDlg, sizeof(szTempBuffer)/sizeof(WCHAR), szTempBuffer);

    //
    // If this is a low res device, we need to check if the
    // font we're creating is smaller than the system font.
    // If so, just use the system font.
    //
    if (IsFontTooSmall(szTempBuffer, lpStrSubst, &tm)) {
        //
        // Couldn't find a font with the height or facename
        // the app wanted so use the system font instead. Note
        // that we need to make sure the app knows it is
        // getting the system font via the WM_SETFONT message
        // so we still need to act as if a new font is being
        // sent to the dialog box.
        //
deleteFont:
        fDeleteFont = TRUE;
    }


    if (hOldFont != NULL) {
        SelectFont(hdcDlg, hOldFont);
    }

    if (fDeleteFont) {
        DeleteFont(hFont);
        // FE_SB
        //
        // Font is deleted, Prepare for reTry...
        //
        fDeleteFont = FALSE;
        // end FE_SB
        hFont = NULL;
    }

    // FE_SB
    //
    // 1. We fail to create font.
    // 2. We did *NOT* try default charset, yet.
    // 3. We want to try default charset.
    //
    // if all of answer is 'Yes', we will try...
    //
    if (IS_DBCS_ENABLED() && (hFont == NULL) && bWillTryDefaultCharset) {
        //
        // Try DEFAULT_CHARSET.
        //
        LogFont.lfCharSet = DEFAULT_CHARSET;
        goto TryDefaultCharset;
    }
    // end FE_SB

    return(hFont);
}

#define CD_VISIBLE          0x01
#define CD_GLOBALEDIT       0x02
#define CD_USERFONT         0x04
#define CD_SETFOREGROUND    0x08
#define CD_USEDEFAULTX      0x10
#define CD_USEDEFAULTCX     0x20


/***************************************************************************\
* GetDialogMonitor
*
* Gets the monitor a dialog should be created on.
*
* Params:
*     hwndOwner - the owner of the dialog. May be NULL.
*
* History:
* 10-Oct-1996 adams     Created.
\***************************************************************************/

PMONITOR
GetDialogMonitor(HWND hwndOwner, DWORD dwFlags)
{
    PMONITOR    pMonitor;
    PWND        pwnd;
    HWND        hwndForeground;
    DWORD       pid;

    UserAssert(dwFlags == MONITOR_DEFAULTTONULL ||
               dwFlags == MONITOR_DEFAULTTOPRIMARY);

    pMonitor = NULL;
    if (hwndOwner) {
        pwnd = ValidateHwnd(hwndOwner);
        if (    pwnd &&
                GETFNID(pwnd) != FNID_DESKTOP  &&
                GETFNID(REBASEPWND(pwnd, spwndParent)) == FNID_DESKTOP) {

            pMonitor = _MonitorFromWindow(pwnd, MONITOR_DEFAULTTOPRIMARY);
        }
    } else {
        /*
         * HACK!  They passed in no owner and are creating a top level
         * dialog window.  Does this process own the foreground window?
         * If so, pin to that window's monitor.  That way 16-bit apps
         * will work mostly as expected, and old multithreaded dudes just
         * might too.  Especially the shell, for whom many system UI pieces
         * pop up random dialogs inside of API calls.
         */

        hwndForeground = NtUserGetForegroundWindow();
        if (hwndForeground) {
            GetWindowThreadProcessId(hwndForeground, &pid);
            if (pid == HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) {
                pwnd = ValidateHwnd(hwndForeground);
                if (pwnd) {
                    pMonitor = _MonitorFromWindow(pwnd, MONITOR_DEFAULTTOPRIMARY);
                }
            }
        }
    }

    if (!pMonitor && dwFlags == MONITOR_DEFAULTTOPRIMARY) {
        pMonitor = GetPrimaryMonitor();
    }

    return pMonitor;
}



/***************************************************************************\
* InternalCreateDialog
*
* Creates a dialog from a template. Uses passed in menu if there is one,
* destroys menu if creation failed. Server portion of
* CreateDialogIndirectParam.
*
* History:
* 04-10-91 ScottLu
* 04-17-91 Mikehar Win31 Merge
\***************************************************************************/

HWND InternalCreateDialog(
    HANDLE hmod,
    LPDLGTEMPLATE lpdt,
    DWORD cb,
    HWND hwndOwner,
    DLGPROC lpfnDialog,
    LPARAM lParam,
    UINT fSCDLGFlags)
{
    HWND                hwnd;
    HWND                hwnd2;
    PWND                pwnd;
    HWND                hwndNewFocus;
    HWND                hwndEditFirst = NULL;
    RECT                rc;
    WORD                w;
    UTCHAR              *lpszMenu,
                        *lpszClass,
                        *lpszText,
                        *lpCreateParams,
                        *lpStr;
    int                 cxChar,
                        cyChar;
    BOOL                f40Compat;
    HFONT               hNewFont = NULL;
    HFONT               hOldFont;
    LPDLGITEMTEMPLATE   lpdit;
    HMENU               hMenu;
    BOOL                fSuccess;
    BOOL                fWowWindow;
    HANDLE              hmodCreate;
    LPBYTE              lpCreateParamsData;
    DLGTEMPLATE2        dt;
    DLGITEMTEMPLATE2    dit;
    DWORD               dwExpWinVer;
    DWORD               dsStyleOld;
    DWORD               bFlags = 0;
    HDC                 hdcDlg;
    LARGE_STRING        strClassName;
    PLARGE_STRING       pstrClassName;
    LARGE_STRING        strWindowName;
    PMONITOR            pMonitor;
	// FA
	BOOL bResizeMe=FALSE;

    UNREFERENCED_PARAMETER(cb);

    ConnectIfNecessary();

    UserAssert(!(fSCDLGFlags & ~(SCDLG_CLIENT|SCDLG_ANSI|SCDLG_NOREVALIDATE|SCDLG_16BIT)));    // These are the only valid flags

    /*
     * Is this a Win4 extended dialog?
     */
    if (((LPDLGTEMPLATE2)lpdt)->wSignature == 0xffff) {

        UserAssert(((LPDLGTEMPLATE2)lpdt)->wDlgVer == 1);
        RtlCopyMemory(&dt, lpdt, sizeof dt);
    } else {
        dt.wDlgVer = 0;
        dt.wSignature = 0;
        dt.dwHelpID = 0;
        dt.dwExStyle = lpdt->dwExtendedStyle;
        dt.style = lpdt->style;
        dt.cDlgItems = lpdt->cdit;
        dt.x = lpdt->x;
        dt.y = lpdt->y;
        dt.cx = lpdt->cx;
        dt.cy = lpdt->cy;
    }

    /*
     * If this is called from wow code, then the loword of hmod != 0.
     * In this case, allow any DS_ style bits that were passed in win3.1
     * to be legal in win32. Case in point: 16 bit quark xpress passes the
     * same bit as the win32 style DS_SETFOREGROUND. Also, VC++ sample
     * "scribble" does the same thing.
     *
     * For win32 apps test the DS_SETFOREGROUND bit; wow apps are not set
     * foreground (this is the new NT semantics)
     * We have to let no "valid" bits through because apps depend on them
     * bug 5232.
     */
    dsStyleOld = LOWORD(dt.style);

    /*
     * If the app is Win4 or greater, require correct dialog style bits.
     * Prevents conflicts with new bits introduced in Chicago
     */
    dwExpWinVer = GETEXPWINVER(hmod);

    if ( f40Compat = Is400Compat(dwExpWinVer) ) {
        dt.style &= (DS_VALID40 | 0xffff0000);

        //
        // For old applications:
        //      If DS_COMMONDIALOG isn't set, don't touch DS_3DLOOK style
        // bit.  If it's there, it stays there.  If not, not.  That way old
        // apps which pass in their own templates, not commdlg's, don't get
        // forced 3D.
        //      If DS_COMMONDIALOG is there, remove DS_3DLOOK.
        //
        // For new applications:
        //      Force 3D always.
        //
        if (GETAPPVER() < VER40) {
            if (dt.style & DS_COMMONDIALOG) {
                dt.style &= ~DS_3DLOOK;
                dsStyleOld &= ~DS_3DLOOK;
            }
        } else {
            dt.style |= DS_3DLOOK;
            dsStyleOld |= DS_3DLOOK;
        }
    } else {
#if DBG
        if (dt.style != (dt.style & (DS_VALID31 | DS_3DLOOK | 0xffff0000))) {
            RIPMSG1(RIP_WARNING, "CreateDialog: stripping invalid bits %lX", dt.style);
        }
#endif // DBG


        /*
         * Don't strip off bits for old apps, they depend on this.  Especially 16 bit MFC apps!
         *
         * dt.dwStyle &= (DS_VALID31 | 0xffff0000);
         */
    }

    if (LOWORD((ULONG_PTR)hmod) == 0) {
        if (dt.style & DS_SETFOREGROUND)
            bFlags |= CD_SETFOREGROUND;
    }

    if (dsStyleOld != LOWORD(dt.style))
    {

        RIPMSG1(f40Compat ? RIP_ERROR : RIP_WARNING,
                "Bad dialog style bits (%x) - please remove.",
                LOWORD(dt.style));
        // Fail new apps that pass in bogus bits!

        if (f40Compat) {
            return NULL;
        }
    }

    if ( dt.style & DS_MODALFRAME) {
        dt.dwExStyle |= WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE;
    }

    if (( dt.style & DS_CONTEXTHELP) && f40Compat) {
        dt.dwExStyle |= WS_EX_CONTEXTHELP;
    }

    if (dt.style & DS_CONTROL) {
        // Captions and system menus aren't allowed on "control" dialogs.
        // And strip DS_SYSMODAL.
        dt.style &= ~(WS_CAPTION | WS_SYSMENU | DS_SYSMODAL);
    } else if (dt.style & WS_DLGFRAME) {
        // Add on window edge same way that CreateWindowEx() will
        dt.dwExStyle |= WS_EX_WINDOWEDGE;
    }

    if (dt.style & DS_SYSMODAL) {
        dt.dwExStyle |= WS_EX_TOPMOST;
    }

    if (!(dt.style & WS_CHILD) || (dt.style & DS_CONTROL)) {
        // only a control parent if it's not a child dialog or if it's
        // explicitly marked as a recursive dialog
        dt.dwExStyle |= WS_EX_CONTROLPARENT;
    }

    if (dt.x == (short)CW2_USEDEFAULT) {
        bFlags |= CD_USEDEFAULTX;
        dt.x = 0;
    }

    if (dt.cx == (short)CW2_USEDEFAULT) {
        bFlags |= CD_USEDEFAULTCX;
        dt.cx = 0;
    } else if (dt.cx < 0) {
        dt.cx = 0;
    }

    if (dt.cy < 0) {
        dt.cy = 0;
    }


    // If there's a menu name string, load it.
    lpszMenu = (LPWSTR)(((PBYTE)(lpdt)) + (dt.wDlgVer ? sizeof(DLGTEMPLATE2):sizeof(DLGTEMPLATE)));

    /*
     * If the menu id is expressed as an ordinal and not a string,
     * skip all 4 bytes to get to the class string.
     */
    w = *(LPWORD)lpszMenu;

    /*
     * If there's a menu name string, load it.
     */
    if (w != 0) {
        if ((hMenu = LoadMenu(hmod, (w == 0xFFFF) ?
                MAKEINTRESOURCE(*(WORD *)((PBYTE)lpszMenu + 2)) : lpszMenu)) == NULL) {
            RIPMSG0(RIP_WARNING, "ServerCreateDialog() failed: couldn't load menu");
            goto DeleteFontAndMenuAndFail;
        }
    } else {
        hMenu = NULL;
    }

    if (w == 0xFFFF) {
        lpszClass = (LPWSTR)((LPBYTE)lpszMenu + 4);
    } else {
        lpszClass = (UTCHAR *)WordSkipSz(lpszMenu);
    }

    lpszText = (UTCHAR *)WordSkipSz(lpszClass);

    lpStr = (UTCHAR *)WordSkipSz(lpszText);

    hdcDlg = CreateCompatibleDC(NULL);
    if (hdcDlg == NULL)
        goto DeleteFontAndMenuAndFail;

    if (dt.style & DS_SETFONT) {
        hNewFont = CreateDlgFont(hdcDlg, &lpStr, &dt, dwExpWinVer);
        bFlags |= CD_USERFONT;
        lpdit = (LPDLGITEMTEMPLATE) NextDWordBoundary(lpStr);
    } else if (Is400Compat(dwExpWinVer) && (dt.style & DS_FIXEDSYS)) {

        //
        // B#2078 -- WISH for fixed width system font in dialog.  We need
        // to tell the dialog that it's using a font different from the
        // standard system font, so set CD_USERFONT bit.
        //
        // We need the 400 compat. check for CorelDraw, since they use
        // this style bit for their own purposes.
        //
        hNewFont = GetStockObject(SYSTEM_FIXED_FONT);
        bFlags |= CD_USERFONT;
        lpdit = (LPDLGITEMTEMPLATE)NextDWordBoundary(lpStr);
    } else {
        lpdit = (LPDLGITEMTEMPLATE)NextDWordBoundary(lpStr);
    }

    /*
     * If the application requested a particular font and for some
     * reason we couldn't find it, we just use the system font.  BUT we
     * need to make sure we tell him he gets the system font.  Dialogs
     * which never request a particular font get the system font and we
     * don't bother telling them this (via the WM_SETFONT message).
     */

    // Is it anything other than the default system font?  If we can't get
    // enough memory to select in the new font specified, just use the system
    // font.
    if (hNewFont && (hOldFont = SelectFont(hdcDlg, hNewFont))) {
        // Get the ave character width and height to be used
        cxChar = GdiGetCharDimensions(hdcDlg, NULL, &cyChar);

        SelectFont(hdcDlg, hOldFont);
        if (cxChar == 0) {
            RIPMSG0(RIP_WARNING, "InternalCreateDialog: GdiGetCharDimensions failed");
            goto UseSysFontMetrics;
        }
    }
    else
    {
        if (hNewFont || (bFlags & CD_USERFONT))
            hNewFont = ghFontSys;

UseSysFontMetrics:
        cxChar = gpsi->cxSysFontChar;
        cyChar = gpsi->cySysFontChar;
    }
    DeleteDC(hdcDlg);

    if (dt.style & WS_VISIBLE) {
        bFlags |= CD_VISIBLE;
        dt.style &= ~WS_VISIBLE;
    }

    if (!(dt.style & DS_LOCALEDIT)) {
        bFlags |= CD_GLOBALEDIT;
    }

    /* Figure out dimensions of real window
     */
    rc.left = rc.top = 0;
    rc.right = XPixFromXDU(dt.cx, cxChar);
    rc.bottom = YPixFromYDU(dt.cy, cyChar);

    _AdjustWindowRectEx(&rc, dt.style, w, dt.dwExStyle);

    dt.cx = (SHORT)(rc.right - rc.left);
    dt.cy = (SHORT)(rc.bottom - rc.top);

    if ((dt.style & DS_CENTERMOUSE) && SYSMET(MOUSEPRESENT) && f40Compat) {
        pMonitor = _MonitorFromPoint(gpsi->ptCursor, MONITOR_DEFAULTTONULL);
        UserAssert(pMonitor);
        *((LPPOINT)&rc.left) = gpsi->ptCursor;
        rc.left -= (dt.cx / 2);
        rc.top  -= (dt.cy / 2);
    } else {
        BOOL fNoDialogMonitor;

        pMonitor = GetDialogMonitor(hwndOwner, MONITOR_DEFAULTTONULL);
        fNoDialogMonitor = pMonitor ? FALSE : TRUE;
        if (!pMonitor) {
            pMonitor = GetPrimaryMonitor();
        }

        if ((dt.style & (DS_CENTER | DS_CENTERMOUSE)) && f40Compat) {
            /*
             * Center to the work area of the owner monitor.
             */
            rc.left = (pMonitor->rcWork.left + pMonitor->rcWork.right - dt.cx) / 2;
            rc.top  = (pMonitor->rcWork.top + pMonitor->rcWork.bottom - dt.cy) / 2;
        } else {
            rc.left = XPixFromXDU(dt.x, cxChar);
            rc.top = YPixFromYDU(dt.y, cyChar);

            if (!(dt.style & DS_ABSALIGN) && hwndOwner) {
                /*
                 * Offset relative coordinates to the owner window. If it is
                 * a child window, there is nothing to do.
                 */
                if ((HIWORD(dt.style) & MaskWF(WFTYPEMASK)) != MaskWF(WFCHILD)) {
                    //This is will considre rc.left form the right hand side of the owner window if it a mirrored one.
                    ClientToScreen(hwndOwner, (LPPOINT)&rc.left);

#ifdef USE_MIRRORING
                    //It is not chiled then do Visual ClientToScreen
                    //i.e. rc.left it is form the left hand side of the owner window
                    if (MIRRORED_HWND(hwndOwner)) {
                        rc.left -= dt.cx;
                    }
#endif

                }
            } else {
                /*
                 * Position the dialog in screen coordinates. If the dialog's
                 * owner is on a different monitor than specified in the
                 * template, move the dialog to the owner window. If the owner
                 * doesn't exist, then use the monitor from the dialog's
                 * template.
                 */

                PMONITOR    pMonitorTemplate;
                RECT        rcTemplate;

                rcTemplate.left  = rc.left;
                rcTemplate.top   = rc.top;
                rcTemplate.right  = rc.left + dt.cx;
                rcTemplate.bottom = rc.top + dt.cy;

                pMonitorTemplate = _MonitorFromRect(&rcTemplate, MONITOR_DEFAULTTOPRIMARY);
                if (fNoDialogMonitor) {
                    pMonitor = pMonitorTemplate;
                } else if (pMonitorTemplate != pMonitor) {
                    rc.left += pMonitor->rcMonitor.left - pMonitorTemplate->rcMonitor.left;
                    rc.top  += pMonitor->rcMonitor.top  - pMonitorTemplate->rcMonitor.top;
                }
            }
        }
    }

    rc.right  = rc.left + dt.cx;
    rc.bottom = rc.top  + dt.cy;

    // If the right or bottom coordinate has overflowed, then pin it back to
    // a valid rectangle.  Likely to happen if a minimized window is the owner of
    // the dialog.
    if (rc.left > rc.right || rc.top > rc.bottom) {
        OffsetRect(&rc, -dt.cx, -dt.cy);
    }

   //
    // Need to do this for ALL dialogs, not just top-level, since we used
    // to in 3.1.
    //

    // Clip top level dialogs within working area
    // Start child dialogs at least at (0, 0)
    RepositionRect(pMonitor, &rc, dt.style, dt.dwExStyle);

    dt.x  = (SHORT)((bFlags & CD_USEDEFAULTX) ? CW2_USEDEFAULT : rc.left);
    dt.y  = (SHORT)(rc.top);
    dt.cx = (SHORT)((bFlags & CD_USEDEFAULTCX) ? CW2_USEDEFAULT : rc.right - rc.left);
    dt.cy = (SHORT)(rc.bottom - rc.top);

    if (*lpszClass != 0) {
        if (IS_PTR(lpszClass)) {
            RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&strClassName,
                    lpszClass, (UINT)-1);
            pstrClassName = &strClassName;
        } else {
            pstrClassName = (PLARGE_STRING)lpszClass;
        }
    } else {
        pstrClassName = (PLARGE_STRING)DIALOGCLASS;
    }

    RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&strWindowName,
            lpszText, (UINT)-1);

	//
	// FA
	// instantiate the resize dlg class if NOT WS_DLGFRAME and WS_THICKFRAME
	// as those dialogs already support the resize code themselves.

	bResizeMe=FALSE;
	if(!( (dt.style & WS_DLGFRAME) && ( dt.style & WS_THICKFRAME) ))
	{
		if ( (dt.style & WS_CHILD) == FALSE )
		{
			// PDLG(pwnd)->pDlgResize=MakeAResizeDlg(0,hmod);
			dt.style |= ( WS_DLGFRAME | WS_THICKFRAME );
			bResizeMe=TRUE;
		}
	}

    UserAssert((dt.dwExStyle & WS_EX_MDICHILD) == 0);
    hwnd = NtUserCreateWindowEx(
            dt.dwExStyle | ((fSCDLGFlags & SCDLG_ANSI) ? WS_EX_ANSICREATOR : 0),
            pstrClassName,
            &strWindowName,
            dt.style,
            DefShortToInt(dt.x),
            dt.y,
            DefShortToInt(dt.cx),
            dt.cy,
            hwndOwner,
            hMenu,
            hmod,
            (LPVOID)NULL,
            dwExpWinVer);

    if (hwnd == NULL) {
        RIPMSG0(RIP_WARNING, "CreateDialog() failed: couldn't create window");
DeleteFontAndMenuAndFail:
        if (hMenu != NULL)
            NtUserDestroyMenu(hMenu);
        /*
         * Only delete the font if we didn't grab it
         * from the dialog font cache.
         */
        if ((hNewFont != NULL)) {
            DeleteObject(hNewFont);
        }
        return NULL;
    }

    pwnd = ValidateHwnd(hwnd);

    // tell WOW the hDlg of the Window just created BEFORE they get any messages
    // at WOW32!w32win16wndprocex
    if(fSCDLGFlags & SCDLG_16BIT) {
        TellWOWThehDlg(hwnd);
    }

    /*
     * Before anything happens with this window, we need to mark it as a
     * dialog window!!!! So do that.
     */
    if (pwnd == NULL || !ValidateDialogPwnd(pwnd))
        goto DeleteFontAndMenuAndFail;

    if (dt.dwHelpID) {
        NtUserSetWindowContextHelpId(hwnd, dt.dwHelpID);
    }

    /*
     * Set up the system menu on this dialog box if it has one.
     */
    if (TestWF(pwnd, WFSYSMENU)) {

        /*
         * For a modal dialog box with a frame and caption, we want to
         * delete the unselectable items from the system menu.
         */
        UserAssert(HIBYTE(WFSIZEBOX) == HIBYTE(WFMINBOX));
        UserAssert(HIBYTE(WFMINBOX) == HIBYTE(WFMAXBOX));
        if (!TestWF(pwnd, WFSIZEBOX | WFMINBOX | WFMAXBOX)) {

            NtUserCallHwndLock(hwnd, SFI_XXXSETDIALOGSYSTEMMENU);
        } else {

            /*
             * We have to give this dialog its own copy of the system menu
             * in case it modifies the menu.
             */
            NtUserGetSystemMenu(hwnd, FALSE);
        }
    }

    /*
     * Set fDisabled to FALSE so EndDialog will Enable if dialog is ended
     * before returning to DialogBox (or if modeless).
     */
    PDLG(pwnd)->fDisabled = FALSE;

    PDLG(pwnd)->cxChar = cxChar;
    PDLG(pwnd)->cyChar = cyChar;
    PDLG(pwnd)->lpfnDlg = lpfnDialog;
    PDLG(pwnd)->fEnd = FALSE;
    PDLG(pwnd)->result = IDOK;
	PDLG(pwnd)->pDlgResize = NULL;

	//
	// FA - instantiate the resize dlg class if WS_DLGFRAME and WS_THICKFRAME
	//
	if(bResizeMe)
		PDLG(pwnd)->pDlgResize=MakeAResizeDlg(0,hmod);

    /*
     * Need to remember Unicode status.
     */
    if (fSCDLGFlags & SCDLG_ANSI) {
        PDLG(pwnd)->flags |= DLGF_ANSI;
    }

    /*
     * Have to do a callback here for WOW apps.  WOW needs what's in lParam
     * before the dialog gets any messages.
     */

    /*
     * If the app is a Wow app then the Lo Word of the hInstance is the
     * 16-bit hInstance.  Set the lParam, which no-one should look at
     * but the app, to the 16 bit value
     */
    if (LOWORD((ULONG_PTR)hmod) != 0) {
        fWowWindow = TRUE;
    } else {
        fWowWindow = FALSE;
    }

    /*
     * If a user defined font is used, save the handle so that we can delete
     * it when the dialog is destroyed.
     */
    if (bFlags & CD_USERFONT) {

        PDLG(pwnd)->hUserFont = hNewFont;

        if (lpfnDialog != NULL) {
            /*
             * Tell the dialog that it will be using this font...
             */
            SendMessageWorker(pwnd, WM_SETFONT, (WPARAM)hNewFont, 0L, FALSE);
        }
    }

    if (!dt.wDlgVer) {
        dit.dwHelpID = 0;
    }

    /*
     * Loop through the dialog controls, doing a CreateWindowEx() for each of
     * them.
     */
    while (dt.cDlgItems-- != 0) {
        DWORD dwExpWinVer2;

        if (dt.wDlgVer) {
            RtlCopyMemory(&dit, lpdit, sizeof dit);
        } else {
            dit.dwHelpID = 0;
            dit.dwExStyle = lpdit->dwExtendedStyle;
            dit.style = lpdit->style;
            dit.x = lpdit->x;
            dit.y = lpdit->y;
            dit.cx = lpdit->cx;
            dit.cy = lpdit->cy;
            dit.dwID = lpdit->id;
        }

        dit.x = XPixFromXDU(dit.x, cxChar);
        dit.y = YPixFromYDU(dit.y, cyChar);
        dit.cx = XPixFromXDU(dit.cx, cxChar);
        dit.cy = YPixFromYDU(dit.cy, cyChar);

        lpszClass = (LPWSTR)(((PBYTE)(lpdit)) + (dt.wDlgVer ? sizeof(DLGITEMTEMPLATE2):sizeof(DLGITEMTEMPLATE)));

        /*
         * If the first WORD is 0xFFFF the second word is the encoded class name index.
         * Use it to look up the class name string.
         */
        if (*(LPWORD)lpszClass == 0xFFFF) {
            lpszText = lpszClass + 2;
            lpszClass = (LPWSTR)(gpsi->atomSysClass[*(((LPWORD)lpszClass)+1) & ~CODEBIT]);
        } else {
            lpszText = (UTCHAR *)SkipSz(lpszClass);
        }
        lpszText = (UTCHAR *)NextWordBoundary(lpszText); // UINT align lpszText

        dit.dwExStyle |= WS_EX_NOPARENTNOTIFY;

        //
        // Replace flat borders with 3D ones for DS_3DLOOK dialogs
        // We test the WINDOW style, not the template style now.  This is so
        // that 4.0 apps--who get 3D stuff automatically--can turn it off on
        // create if they want.
        //

        //
        // HACK!
        // Treat DS_3DLOOK combos like they have a WS_EX_CLIENTEDGE.  Why
        // should we have to draw the borders of a combobox ourselves?
        // We can't do the same thing for WS_BORDER though becaues of
        // PC Fools--they use the presence of WS_BORDER to distinguish
        // between lists and combos.
        //

        if (TestWF(pwnd, DF3DLOOK)) {
            if (    (dit.style & WS_BORDER) ||
                    (lpszClass == MAKEINTRESOURCE(gpsi->atomSysClass[ICLS_COMBOBOX]))) {

                dit.style &= ~WS_BORDER;
                dit.dwExStyle |= WS_EX_CLIENTEDGE;
            }
        }

        /*
         * Get pointer to additional data.  lpszText can point to an encoded
         * ordinal number for some controls (e.g.  static icon control) so
         * we check for that here.
         */
        if (*(LPWORD)lpszText == 0xFFFF) {
            lpCreateParams = (LPWSTR)((PBYTE)lpszText + 4);
            strWindowName.Buffer = lpszText;
            strWindowName.Length = 4;
            strWindowName.MaximumLength = 4;
            strWindowName.bAnsi = FALSE;
        } else {
            lpCreateParams = (LPWSTR)((PBYTE)WordSkipSz(lpszText));
            RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&strWindowName,
                    lpszText, (UINT)-1);
        }

        /*
         * If control is edit control and caller wants global storage
         * of edit text, allocate object in WOW and pass instance
         * handle to CreateWindowEx().
         */
        if (fWowWindow && (bFlags & CD_GLOBALEDIT) &&
               ((!IS_PTR(lpszClass) &&
                    PTR_TO_ID(lpszClass) == (ATOM)(gpsi->atomSysClass[ICLS_EDIT])) ||
               (IS_PTR(lpszClass) &&
                    (wcscmp(lpszClass, szEDITCLASS) == 0)))) {

            /*
             * Allocate only one global object (first time we see editctl.)
             */
            if (!(PDLG(pwnd)->hData)) {
                PDLG(pwnd)->hData = GetEditDS();
                if (!(PDLG(pwnd)->hData))
                    goto NoCreate;
            }

            hmodCreate = PDLG(pwnd)->hData;
            dwExpWinVer2 = GETEXPWINVER(hmodCreate);
        } else {
            hmodCreate = hmod;
            dwExpWinVer2 = dwExpWinVer;
        }

        UserAssert((dit.dwExStyle & WS_EX_ANSICREATOR) == 0);

        /*
         * Get pointer to additional data.
         *
         * For WOW, instead of pointing lpCreateParams at the CreateParams
         * data, set lpCreateParams to whatever DWORD is stored in the 32-bit
         * DLGTEMPLATE's CreateParams.  WOW has already made sure that that
         * 32-bit value is indeed a 16:16 pointer to the CreateParams in the
         * 16-bit DLGTEMPLATE.
         */

        if (*lpCreateParams) {
            lpCreateParamsData = (LPBYTE)lpCreateParams;
            if (fWowWindow || fSCDLGFlags & SCDLG_16BIT) {
                lpCreateParamsData =
                    (LPBYTE)*(UNALIGNED DWORD *)
                    (lpCreateParamsData + sizeof(WORD));
            }
        } else {
            lpCreateParamsData = NULL;
        }

        /*
         * If the dialog template specifies a menu ID then TestwndChild(pwnd)
         * must be TRUE or CreateWindowEx will think the ID is an hMenu rather
         * than an ID (in a dialog template you'll never have an hMenu).
         * However for compatibility reasons we let it go if the ID = 0.
         */
        if (dit.dwID) {
            /*
             * This makes TestwndChild(pwnd) on this window return TRUE.
             */
            dit.style |= WS_CHILD;
            dit.style &= ~WS_POPUP;
        }

        if (IS_PTR(lpszClass)) {
            RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&strClassName,
                    lpszClass, (UINT)-1);
            pstrClassName = &strClassName;
        } else {
            pstrClassName = (PLARGE_STRING)lpszClass;
        }
        UserAssert((dit.dwExStyle & WS_EX_MDICHILD) == 0);

        hwnd2 = NtUserCreateWindowEx(
                dit.dwExStyle | ((fSCDLGFlags & SCDLG_ANSI) ? WS_EX_ANSICREATOR : 0),
                pstrClassName,
                &strWindowName,
                dit.style,
                DefShortToInt(dit.x),
                dit.y,
                DefShortToInt(dit.cx),
                dit.cy,
                hwnd,
                (HMENU)dit.dwID,
                hmodCreate,
                lpCreateParamsData,
                dwExpWinVer2);

        if (hwnd2 == NULL) {
NoCreate:
            /*
             * Couldn't create the window -- return NULL.
             */
            if (!TestWF(pwnd, DFNOFAILCREATE)) {
                RIPMSG0(RIP_WARNING, "CreateDialog() failed: couldn't create control");
                NtUserDestroyWindow(hwnd);
                return NULL;
            }
        } else {

            if (dit.dwHelpID) {
                NtUserSetWindowContextHelpId(hwnd2, dit.dwHelpID);
            }

        /*
         * If it is a not a default system font, set the font for all the
         * child windows of the dialogbox.
         */
            if (hNewFont != NULL) {
                SendMessage(hwnd2, WM_SETFONT, (WPARAM)hNewFont, 0L);
            }

        /*
         * Result gets ID of last (hopefully only) defpushbutton.
         */
            if (SendMessage(hwnd2, WM_GETDLGCODE, 0, 0L) & DLGC_DEFPUSHBUTTON) {
                PDLG(pwnd)->result = dit.dwID;
            }
        }
        /*
         * Point at next item template
         */
        lpdit = (LPDLGITEMTEMPLATE)NextDWordBoundary(
                (LPBYTE)(lpCreateParams + 1) + *lpCreateParams);
    }

    if (!TestWF(pwnd, DFCONTROL)) {
        PWND pwndT = _GetNextDlgTabItem(pwnd, NULL, FALSE);
        hwndEditFirst = HW(pwndT);
    }

    if (lpfnDialog != NULL) {
        fSuccess = SendMessageWorker(pwnd, WM_INITDIALOG,
                               (WPARAM)hwndEditFirst, lParam, FALSE);

        //
        // Make sure the window didn't get nuked during WM_INITDIALOG
        //
        if (!RevalidateHwnd(hwnd)) {
            goto CreateDialogReturn;
        }

        if (fSuccess && !PDLG(pwnd)->fEnd) {

            //
            // To remove the two-default-push-buttons problem, we must make
            // sure CheckDefPushButton() will remove default from other push
            // buttons.  This happens only if hwndEditFirst != hwndNewFocus;
            // So, we make it NULL here. This breaks Designer's install
            // program(which can't take a DM_GETDEFID.  So, we do a version
            // check here.
            //
            if (!TestWF(pwnd, DFCONTROL)) {
                PWND pwndT;
                if (!IsWindow(hwndEditFirst) || TestWF(pwnd, WFWIN40COMPAT))
                    hwndEditFirst = NULL;

                //
                // They could have disabled hwndEditFirst during WM_INITDIALOG.
                // So, let use obtain the First Tab again.
                //
                pwndT = _GetNextDlgTabItem(pwnd, NULL, FALSE);
                if (hwndNewFocus = HW(pwndT)) {
                    DlgSetFocus(hwndNewFocus);
                }

                xxxCheckDefPushButton(pwnd, hwndEditFirst, hwndNewFocus);
            }
        }
    }

    if (!IsWindow(hwnd))
    {
        // Omnis7 relies on a nonzero return even though they nuked this
        // dialog during processing of the WM_INITDIALOG message
        // -- jeffbog -- 2/24/95 -- Win95B B#12368
        if (GETAPPVER() < VER40) {
            return(hwnd);
        }

        return(NULL);
    }

    /*
     * Bring this dialog into the foreground
     * if DS_SETFOREGROUND is set.
     */
    if (bFlags & CD_SETFOREGROUND) {
        NtUserSetForegroundWindow(hwnd);
        if (!IsWindow(hwnd)) {
            hwnd = NULL;
            goto CreateDialogReturn;
        }
    }

    if ((bFlags & CD_VISIBLE) && !PDLG(pwnd)->fEnd && (!TestWF(pwnd, WFVISIBLE))) {
        NtUserShowWindow(hwnd, SHOW_OPENWINDOW);
        UpdateWindow(hwnd);
    }

CreateDialogReturn:

    /*
     * 17609 Gupta's SQLWin deletes the window before CreateDialog returns
     * but still expects non-zero return value from CreateDialog so we will
     * do like win 3.1 and not revalidate for 16 bit apps
     */
    if (!(fSCDLGFlags & SCDLG_NOREVALIDATE) && !RevalidateHwnd(hwnd)) {
        hwnd = NULL;
    }

    return hwnd;
}
