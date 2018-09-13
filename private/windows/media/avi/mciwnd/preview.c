/****************************************************************************
 *
 *  MODULE  : PREVIEW.C
 *
 ****************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <win32.h>

#include <vfw.h>

#define SQUAWKNUMZ(num) #num
#define SQUAWKNUM(num) SQUAWKNUMZ(num)
#define SQUAWK __FILE__ "(" SQUAWKNUM(__LINE__) ") :squawk: "

typedef struct {
    BOOL        bUnicode;
    HWND        hwnd;               // common dialog handle.
    LPOPENFILENAME pofn;

    LPARAM      lCustData;          // hold old value
    DWORD       Flags;
    LPOFNHOOKPROC lpfnHook;

    RECT        rcPreview;
    RECT        rcImage;
    RECT        rcText;
    HWND        hwndMci;
    HFONT       hfont;
    HPALETTE    hpal;
    HANDLE      hdib;
    TCHAR       Title[128];

}   PreviewStuff, FAR *PPreviewStuff;

#define PREVIEW_PROP    TEXT("PreviewStuff")

#ifdef _WIN32
    #define SetPreviewStuff(hwnd, p) SetProp(hwnd,PREVIEW_PROP,(LPVOID)(p))
    #define GetPreviewStuff(hwnd) (PPreviewStuff)(LPVOID)GetProp(hwnd, PREVIEW_PROP)
    #define RemovePreviewStuff(hwnd) RemoveProp(hwnd,PREVIEW_PROP)
#else
    #define SetPreviewStuff(hwnd, p) SetProp(hwnd,PREVIEW_PROP,HIWORD(p))
    #define GetPreviewStuff(hwnd) (PPreviewStuff)MAKELONG(0, GetProp(hwnd, PREVIEW_PROP))
    #define RemovePreviewStuff(hwnd) RemoveProp(hwnd,PREVIEW_PROP)
    #define CharNext AnsiNext
    #define CharPrev AnsiPrev
    #define CharUpperBuff AnsiUpperBuff
    #define CharLower AnsiLower
#endif

/***************************************************************************
 *
 ****************************************************************************/

STATICFN BOOL   PreviewOpen (HWND hwnd, LPOPENFILENAME pofn);
STATICFN BOOL   PreviewFile (PPreviewStuff p, LPTSTR szFile);
STATICFN BOOL   PreviewPaint(PPreviewStuff p);
STATICFN BOOL   PreviewSize (PPreviewStuff p);
STATICFN BOOL   PreviewClose(PPreviewStuff p);

STATICFN HANDLE GetRiffDisp(LPTSTR lpszFile, LPTSTR szText, int iLen);

/***************************************************************************
 *
 ****************************************************************************/

STATICFN BOOL PreviewOpen(HWND hwnd, LPOPENFILENAME pofn)
{
    LOGFONT lf;
    PPreviewStuff p;
    RECT rc;

    p = (LPVOID)pofn->lCustData;
    pofn->lCustData = p->lCustData;

    SetPreviewStuff(hwnd, p);

    p->hwnd = hwnd;
    p->pofn = pofn;

    //
    // create a MCI window for preview.
    //
    p->hwndMci = MCIWndCreate(p->hwnd, NULL,
//          MCIWNDF_NOAUTOSIZEWINDOW    |
//          MCIWNDF_NOPLAYBAR           |
//          MCIWNDF_NOAUTOSIZEMOVIE     |
            MCIWNDF_NOMENU              |
//          MCIWNDF_SHOWNAME            |
//          MCIWNDF_SHOWPOS             |
//          MCIWNDF_SHOWMODE            |
//          MCIWNDF_RECORD              |
            MCIWNDF_NOERRORDLG          |
            WS_CHILD | WS_BORDER,
            NULL);

    //
    // locate the preview in the lower corner of the dialog (below the
    // cancel button)
    //
    GetClientRect(hwnd, &p->rcPreview);
    GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &rc);
    ScreenToClient(hwnd, (LPPOINT)&rc);
    ScreenToClient(hwnd, (LPPOINT)&rc+1);

// The open space we're allowed to use in the dialog is different in NT and on
// Win31.  Under NT there is a network button at the bottom of the dialog on
// the right hand side, so we use the area from just under the CANCEL button to
// a little more than 1 button height from the bottom of the dialog.
// Under Win31, the network button is under CANCEL, so we use the area a little
// over one button height under CANCEL, to just about the bottom of the dialog.
#ifdef _WIN32
    if (1)
#else
    if (GetWinFlags() & WF_WINNT)
#endif
    {
	p->rcPreview.top   = rc.bottom + 4;
	p->rcPreview.left  = rc.left;
	p->rcPreview.right = rc.right;
	p->rcPreview.bottom -= (rc.bottom - rc.top) + 12;
    } else {
	p->rcPreview.top   = rc.bottom + (rc.bottom - rc.top) + 12;
	p->rcPreview.left  = rc.left;
	p->rcPreview.right = rc.right;
	p->rcPreview.bottom -= 4;          // leave a little room at the bottom
    }

    //
    // create a font to use.
    //
    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), (LPVOID)&lf, 0);
    p->hfont = CreateFontIndirect(&lf);

    return TRUE;
}

/***************************************************************************
 *
 ****************************************************************************/

STATICFN BOOL PreviewClose(PPreviewStuff p)
{
    if (p == NULL)
        return FALSE;

    PreviewFile(p, NULL);

    RemovePreviewStuff(p->hwnd);

    if (p->hfont)
        DeleteObject(p->hfont);

    if (p->hwndMci)
        MCIWndDestroy(p->hwndMci);

    return TRUE;
}

/***************************************************************************
 *
 ****************************************************************************/

#define SLASH(c)     ((c) == TEXT('/') || (c) == TEXT('\\'))

STATICFN LPTSTR NiceName(LPTSTR szPath)
{
    LPTSTR   sz;
    LPTSTR   lpsztmp;

    for (sz=szPath; *sz; sz++)
        ;
    for (; sz>szPath && !SLASH(*sz) && *sz!=TEXT(':'); sz =CharPrev(szPath, sz))
        ;
    if(sz>szPath) sz = CharNext(sz) ;

    for(lpsztmp = sz; *lpsztmp  && *lpsztmp != TEXT('.'); lpsztmp = CharNext(lpsztmp))
	;
    *lpsztmp = TEXT('\0');

    CharLower(sz);
    CharUpperBuff(sz, 1);

    return sz;
}

/***************************************************************************
 *
 ****************************************************************************/

STATICFN BOOL PreviewFile(PPreviewStuff p, LPTSTR szFile)
{
    if (p == NULL || !p->hwndMci)
        return FALSE;

    p->Title[0] = 0;

    ShowWindow(p->hwndMci, SW_HIDE);
    MCIWndClose(p->hwndMci);

    if (p->hdib)
        GlobalFree(p->hdib);

    if (p->hpal)
        DeleteObject(p->hpal);

    p->hdib = NULL;
    p->hpal = NULL;

    PreviewPaint(p);

    if (szFile == NULL)
        return TRUE;

    if (MCIWndOpen(p->hwndMci, szFile, 0) == 0)
    {
        lstrcpy(p->Title, NiceName(szFile));

        if (MCIWndUseTime(p->hwndMci) == 0)
        {
            LONG len;
            UINT min,sec;

            len = MCIWndGetLength(p->hwndMci);

            if (len > 0)
            {
                #define ONE_HOUR    (60ul*60ul*1000ul)
                #define ONE_MINUTE  (60ul*1000ul)
                #define ONE_SECOND  (1000ul)

                min  = (UINT)(len / ONE_MINUTE) % 60;
                sec  = (UINT)(len / ONE_SECOND) % 60;

                wsprintf(p->Title + lstrlen(p->Title), TEXT(" (%02d:%02d)"), min, sec);
            }
        }
    }

    PreviewSize(p);
    PreviewPaint(p);
    return TRUE;
}

/***************************************************************************
 *
 ****************************************************************************/

STATICFN BOOL PreviewSize(PPreviewStuff p)
{
    RECT    rc;
    RECT    rcImage;
    RECT    rcText;
    RECT    rcPreview;
    HDC     hdc;
    int     dx;
    int     dy;
    int     dyPlayBar;

    SetRectEmpty(&p->rcText);
    SetRectEmpty(&p->rcImage);

    //
    // if nothing to do clear it.
    //
    if (p->Title[0] == 0 && p->hdib == NULL)
        return FALSE;

    rcPreview = p->rcPreview;

    //
    // compute the text rect, using DrawText
    //
    hdc = GetDC(p->hwnd);
    SelectObject(hdc, p->hfont);

    rcText = rcPreview;
    rcText.bottom = rcText.top;

    DrawText(hdc, p->Title, -1, &rcText, DT_CALCRECT|DT_LEFT|DT_WORDBREAK);
    ReleaseDC(p->hwnd, hdc);

    //
    // compute the image size
    //
    MCIWndChangeStyles(p->hwndMci, MCIWNDF_NOPLAYBAR, MCIWNDF_NOPLAYBAR);
    GetWindowRect(p->hwndMci, &rc);
    dx = rc.right - rc.left;
    dy = rc.bottom - rc.top;
    MCIWndChangeStyles(p->hwndMci, MCIWNDF_NOPLAYBAR, 0);
    GetWindowRect(p->hwndMci, &rc);
    dyPlayBar = rc.bottom - rc.top - dy;

    rcImage = rcPreview;
    rcImage.bottom -= dyPlayBar;

    //
    //  if wider than preview area scale to fit
    //
    if (dx > rcImage.right - rcImage.left)
    {
        rcImage.bottom = rcImage.top + MulDiv(dy,rcImage.right-rcImage.left,dx);
    }
    //
    //  if x2 will fit then use it
    //
    else if (dx * 2 < rcImage.right - rcImage.left)
    {
        rcImage.right  = rcImage.left + dx*2;
        rcImage.bottom = rcImage.top + dy*2;
    }
    //
    //  else center the image in the preview area
    //
    else
    {
        rcImage.right  = rcImage.left + dx;
        rcImage.bottom = rcImage.top + dy;
    }

    if (rcImage.bottom > rcPreview.bottom - (rcText.bottom - rcText.top) - dyPlayBar)
    {
        rcImage.bottom = rcPreview.bottom - (rcText.bottom - rcText.top) - dyPlayBar;
        rcImage.right  = rcPreview.left + MulDiv(dx,rcImage.bottom-rcImage.top,dy);
        rcImage.left   = rcPreview.left;
    }

    rcImage.bottom += dyPlayBar;

    //
    //  now center
    //
    dx = ((rcPreview.right - rcPreview.left) - (rcText.right - rcText.left))/2;
    OffsetRect(&rcText, dx, 0);

    dx = ((rcPreview.right - rcPreview.left) - (rcImage.right - rcImage.left))/2;
    OffsetRect(&rcImage, dx, 0);

    dy  = rcPreview.bottom - rcPreview.top;
    dy -= rcImage.bottom - rcImage.top;
    dy -= rcText.bottom - rcText.top;

    if (dy < 0)
        dy = 0;
    else
        dy = dy / 2;

    OffsetRect(&rcImage, 0, dy);
    OffsetRect(&rcText, 0, dy + rcImage.bottom - rcImage.top + 2);

    //
    // store RECTs
    //
    p->rcImage = rcImage;
    p->rcText = rcText;

    //
    // position window.
    //
    SetWindowPos(p->hwndMci, NULL, rcImage.left, rcImage.top,
        rcImage.right - rcImage.left, rcImage.bottom - rcImage.top,
        SWP_NOZORDER | SWP_NOACTIVATE);

    ShowWindow(p->hwndMci, SW_SHOW);

    return TRUE;
}


/***************************************************************************
 *
 ****************************************************************************/

STATICFN BOOL PreviewPaint(PPreviewStuff p)
{
    HDC     hdc;
    HBRUSH  hbr;
    HWND    hwnd = p->hwnd;

    if (p == NULL)
        return TRUE;

    hdc = GetDC(hwnd);

  #ifdef _WIN32
    hbr = (HBRUSH)DefWindowProc(hwnd, WM_CTLCOLORDLG, (WPARAM)hdc, (LPARAM)hwnd);
  #else
    hbr = (HBRUSH)DefWindowProc(hwnd, WM_CTLCOLOR, (WPARAM)hdc, MAKELONG(hwnd, CTLCOLOR_DLG));
  #endif

////FillRect(hdc, &p->rcPreview, hbr);
    FillRect(hdc, &p->rcText, hbr);

    SelectObject(hdc, p->hfont);
    DrawText(hdc, p->Title, -1, &p->rcText, DT_LEFT|DT_WORDBREAK);

    ReleaseDC(hwnd, hdc);
    return TRUE;
}

/***************************************************************************
 *
 ****************************************************************************/

#pragma message (SQUAWK "should use the correct header for cmb1, etc")

    /* Combo boxes */
#define cmb1        0x0470
#define cmb2        0x0471
    /* Listboxes */
#define lst1        0x0460
#define lst2        0x0461
    /* Edit controls */
#define edt1        0x0480

#define ID_TIMER    1234
#define PREVIEWWAIT 1000

UINT_PTR FAR PASCAL _loadds GetFileNamePreviewHook(HWND hwnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
    int i;
    TCHAR ach[80];

    PPreviewStuff p;

    p = GetPreviewStuff(hwnd);

    switch (msg) {
        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case lst1:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SELCHANGE)
                    {
                        KillTimer(hwnd, ID_TIMER);
                        SetTimer(hwnd, ID_TIMER, PREVIEWWAIT, NULL);
                    }
                    break;

                case IDOK:
                case IDCANCEL:
                    KillTimer(hwnd, ID_TIMER);
                    PreviewFile(p, NULL);
                    break;

                case cmb1:
                case cmb2:
                case lst2:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SELCHANGE)
                    {
                        KillTimer(hwnd, ID_TIMER);
                        PreviewFile(p, NULL);
                    }
                    break;
            }
            break;

        case WM_TIMER:
            if (wParam == ID_TIMER)
            {
                KillTimer(hwnd, ID_TIMER);

                ach[0] = 0;
                i = (int)SendDlgItemMessage(hwnd, lst1, LB_GETCURSEL, 0, 0L);
                SendDlgItemMessage(hwnd, lst1, LB_GETTEXT, i, (LPARAM)(LPTSTR)ach);
                PreviewFile(p, ach);
                return TRUE;
            }
            break;

        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
            if (p && p->hwndMci)
                SendMessage(p->hwndMci, msg, wParam, lParam);
	    break;

        case WM_PAINT:
            PreviewPaint(p);
            break;

        case WM_INITDIALOG:
            PreviewOpen(hwnd, (LPOPENFILENAME)lParam);

            p = GetPreviewStuff(hwnd);

            if (!(p->Flags & OFN_ENABLEHOOK))
                return TRUE;

            break;

        case WM_DESTROY:
            PreviewClose(p);
            break;
    }

    if (p && (p->Flags & OFN_ENABLEHOOK))
        return (int)p->lpfnHook(hwnd, msg, wParam, lParam);
    else
        return FALSE;
}

/***************************************************************************
 *
 ****************************************************************************/

STATICFN BOOL GetFileNamePreview(LPOPENFILENAME lpofn, BOOL fSave, BOOL bUnicode)
{
    BOOL f;
    PPreviewStuff p;

//////// Link to COMMDLG
    HINSTANCE h;
    BOOL (WINAPI *GetFileNameProc)(OPENFILENAME FAR*) = NULL;
    char procname[60];

    if (fSave) {
	lstrcpyA(procname, "GetSaveFileName");
    } else {
	lstrcpyA(procname, "GetOpenFileName");
    }
#ifdef _WIN32
    if (bUnicode) {
	lstrcatA(procname, "W");
    } else {
    	lstrcatA(procname, "A");
    }
#endif

#ifdef _WIN32
    if ((h = LoadLibrary(TEXT("COMDLG32.DLL"))) != NULL) {
        (FARPROC)GetFileNameProc = GetProcAddress(h, procname);
#else
    if ((h = LoadLibrary(TEXT("COMMDLG.DLL"))) >= (HINSTANCE)HINSTANCE_ERROR) {
        (FARPROC)GetFileNameProc = GetProcAddress(h, procname);
#endif
    }

    if (GetFileNameProc == NULL)
        return FALSE;      //!!! what's the right error here?
////////////////

#ifndef OFN_NONETWORKBUTTON
#define OFN_NONETWORKBUTTON 0x00020000
#endif

    // If we have a READ ONLY checkbox, or both HELP and NETWORK, then it's in
    // our way, so get rid of it. (keep NETWORK, lose HELP)

    if (!(lpofn->Flags & OFN_HIDEREADONLY))
	lpofn->Flags |= OFN_HIDEREADONLY;
    if ((lpofn->Flags & OFN_SHOWHELP) && !(lpofn->Flags & OFN_NONETWORKBUTTON))
	lpofn->Flags &= ~OFN_SHOWHELP;

    p = (LPVOID)GlobalAllocPtr(GHND, sizeof(PreviewStuff));

    if (p == NULL)
    {
        f = GetFileNameProc(lpofn);
    }
    else
    {
	p->bUnicode  = bUnicode;
        p->lpfnHook  = lpofn->lpfnHook;
        p->Flags     = lpofn->Flags;
        p->lCustData = lpofn->lCustData;

        lpofn->lpfnHook = (LPVOID)GetFileNamePreviewHook;
        lpofn->Flags |= OFN_ENABLEHOOK;
        lpofn->lCustData = (LPARAM)p;

        f = GetFileNameProc(lpofn);

        lpofn->lpfnHook  = p->lpfnHook;
        lpofn->Flags     = p->Flags;

        GlobalFreePtr(p);
    }

    FreeLibrary(h);     //!!! should we free DLL?
    return f;
}

#ifdef _WIN32

/**************************************************************************
* @doc EXTERNAL
*
* @api BOOL | GetOpenFileNamePreview | This is just like <f GetOpenFileName>
*   in COMMDLG, but with a preview window so people can see what movie
*   they're opening.
*
* @parm LPOPENFILENAME | lpofn | See the documentation for <f GetOpenFileName>.
*
* @rdesc Returns true if a file was opened.
*
* @xref GetOpenFileName
*
*************************************************************************/
BOOL FAR PASCAL _loadds GetOpenFileNamePreviewW(LPOPENFILENAMEW lpofn)
{
    return GetFileNamePreview((LPOPENFILENAME)lpofn, FALSE, TRUE);
}

/**************************************************************************
* @doc EXTERNAL
*
* @api BOOL | GetSaveFileNamePreview | This is just like <f GetSaveFileName>
*   in COMMDLG, but with a preview window so people can see what movie
*   they're saving over.
*
* @parm LPOPENFILENAME | lpofn | See the documentation for <f GetSaveFileName>.
*
* @rdesc Returns true if a file was opened.
*
* @xref GetSaveFileName
*
*************************************************************************/
BOOL FAR PASCAL _loadds GetSaveFileNamePreviewW(LPOPENFILENAMEW lpofn)
{
    return GetFileNamePreview((LPOPENFILENAME)lpofn, TRUE, TRUE);
}

// ansi thunks for above two functions
BOOL FAR PASCAL _loadds GetOpenFileNamePreviewA(LPOPENFILENAMEA lpofn)
{
    return GetFileNamePreview((LPOPENFILENAME)lpofn, FALSE, FALSE);
}

BOOL FAR PASCAL _loadds GetSaveFileNamePreviewA(LPOPENFILENAMEA lpofn)
{
    return GetFileNamePreview((LPOPENFILENAME)lpofn, TRUE, FALSE);
}

#else
BOOL FAR PASCAL _loadds GetOpenFileNamePreview(LPOPENFILENAME lpofn)
{
    return GetFileNamePreview(lpofn, FALSE, FALSE);
}

BOOL FAR PASCAL _loadds GetSaveFileNamePreview(LPOPENFILENAME lpofn)
{
    return GetFileNamePreview(lpofn, TRUE, FALSE);
}
#endif
