/****************************************************************************
 *
 *  MODULE  : PREVIEW.C
 *
 ****************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>
#include "preview.h"
#include "mciwnd.h"

typedef struct {
    HWND        hwnd;               // common dialog handle.
    LPOPENFILENAME pofn;

    LPARAM      lCustData;          // hold old value
    DWORD       Flags;
    UINT        (CALLBACK *lpfnHook)(HWND, UINT, WPARAM, LPARAM);

    RECT        rcPreview;
    RECT        rcImage;
    RECT        rcText;
    HWND        hwndMci;
    HFONT       hfont;
    HPALETTE    hpal;
    HANDLE      hdib;
    char        Title[128];

}   PreviewStuff, FAR *PPreviewStuff;

#define PREVIEW_PROP    "PreviewStuff"

#ifdef WIN32
    #define SetPreviewStuff(hwnd, p) SetProp(hwnd,PREVIEW_PROP,(LPVOID)(p))
    #define GetPreviewStuff(hwnd) (PPreviewStuff)(LPVOID)GetProp(hwnd, PREVIEW_PROP)
    #define RemovePreviewStuff(hwnd) RemoveProp(hwnd,PREVIEW_PROP)
#else
    #define SetPreviewStuff(hwnd, p) SetProp(hwnd,PREVIEW_PROP,HIWORD(p))
    #define GetPreviewStuff(hwnd) (PPreviewStuff)MAKELONG(0, GetProp(hwnd, PREVIEW_PROP))
    #define RemovePreviewStuff(hwnd) RemoveProp(hwnd,PREVIEW_PROP)
#endif

/***************************************************************************
 *
 ****************************************************************************/

static BOOL   PreviewOpen (HWND hwnd, LPOPENFILENAME pofn);
static BOOL   PreviewFile (PPreviewStuff p, LPSTR szFile);
static BOOL   PreviewPaint(PPreviewStuff p);
static BOOL   PreviewSize (PPreviewStuff p);
static BOOL   PreviewClose(PPreviewStuff p);

static HANDLE GetRiffDisp(LPSTR lpszFile, LPSTR szText, int iLen);

/***************************************************************************
 *
 ****************************************************************************/

static BOOL PreviewOpen(HWND hwnd, LPOPENFILENAME pofn)
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
#ifdef WIN32
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

static BOOL PreviewClose(PPreviewStuff p)
{
    if (p == NULL)
        return FALSE;

    PreviewFile(p, NULL);

    RemovePreviewStuff(p->hwnd);

    if (p->hfont)
        DeleteObject(p->hfont);

    if (p->hwndMci)
        MCIWndDestroy(p->hwndMci);
}

/***************************************************************************
 *
 ****************************************************************************/

#define SLASH(c)     ((c) == '/' || (c) == '\\')

static LPSTR NiceName(LPSTR szPath)
{
    LPSTR   sz;
    LPSTR   lpsztmp;

    for (sz=szPath; *sz; sz++)
        ;
    for (; sz>szPath && !SLASH(*sz) && *sz!=':'; sz =AnsiPrev(szPath, sz))
        ;
    if(sz>szPath) sz = AnsiNext(sz) ;

    for(lpsztmp = sz; *lpsztmp	&& *lpsztmp != '.'; lpsztmp = AnsiNext(lpsztmp))
	;
    *lpsztmp = '\0';

    AnsiLower(sz);
    AnsiUpperBuff(sz, 1);

    return sz;
}

/***************************************************************************
 *
 ****************************************************************************/

static BOOL PreviewFile(PPreviewStuff p, LPSTR szFile)
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

                wsprintf(p->Title + lstrlen(p->Title), " (%02d:%02d)", min, sec);
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

static BOOL PreviewSize(PPreviewStuff p)
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

static BOOL PreviewPaint(PPreviewStuff p)
{
    HDC     hdc;
    HBRUSH  hbr;
    HWND    hwnd = p->hwnd;

    if (p == NULL)
        return TRUE;

    hdc = GetDC(hwnd);
    hbr = (HBRUSH)DefWindowProc(hwnd, WM_CTLCOLOR, (WPARAM)hdc, MAKELONG(hwnd, CTLCOLOR_DLG));
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

WORD FAR PASCAL _loadds GetFileNamePreviewHook(HWND hwnd, unsigned msg, WORD wParam, LONG lParam)
{
    int i;
    char ach[80];

    PPreviewStuff p;

    p = GetPreviewStuff(hwnd);

    switch (msg) {
        case WM_COMMAND:
            switch (wParam)
            {
                case lst1:
                    if (HIWORD(lParam) == LBN_SELCHANGE)
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
                    if (HIWORD(lParam) == LBN_SELCHANGE)
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
                SendDlgItemMessage(hwnd, lst1, LB_GETTEXT, i, (LONG)(LPSTR)ach);
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
        return p->lpfnHook(hwnd, msg, wParam, lParam);
    else
        return FALSE;
}

/***************************************************************************
 *
 ****************************************************************************/

static BOOL GetFileNamePreview(LPOPENFILENAME lpofn, BOOL fSave)
{
    BOOL f;
    PPreviewStuff p;

//////// Link to COMMDLG
    HINSTANCE h;
    BOOL (WINAPI *GetFileNameProc)(OPENFILENAME FAR*) = NULL;

    if ((h = LoadLibrary("COMMDLG.DLL")) >= HINSTANCE_ERROR)
        (FARPROC)GetFileNameProc = GetProcAddress(h,
		fSave ? "GetSaveFileName" : "GetOpenFileName");

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
        p->lpfnHook  = lpofn->lpfnHook;
        p->Flags     = lpofn->Flags;
        p->lCustData = lpofn->lCustData;

        lpofn->lpfnHook = GetFileNamePreviewHook;
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
BOOL FAR PASCAL _loadds GetOpenFileNamePreview(LPOPENFILENAME lpofn)
{
    return GetFileNamePreview(lpofn, FALSE);
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
BOOL FAR PASCAL _loadds GetSaveFileNamePreview(LPOPENFILENAME lpofn)
{
    return GetFileNamePreview(lpofn, TRUE);
}

#if 0   ///////////////////// DONT NEED THIS

/****************************************************************************
 *
 ****************************************************************************/

//#define FOURCC_RIFF mmioFOURCC('R','I','F','F')
#define FOURCC_INFO mmioFOURCC('I','N','F','O')
#define FOURCC_DISP mmioFOURCC('D','I','S','P')
#define FOURCC_INAM mmioFOURCC('I','N','A','M')
#define FOURCC_ISBJ mmioFOURCC('I','S','B','J')

#define DibSizeImage(lpbi) (\
    (DWORD)(UINT)((((int)lpbi->biBitCount*(int)lpbi->biWidth+31)&~31)>>3) * \
    (DWORD)(UINT)lpbi->biHeight)

#define DibSize(lpbi) \
    (lpbi->biSize + ((int)lpbi->biClrUsed * sizeof(RGBQUAD)) + lpbi->biSizeImage)

#define DibNumColors(lpbi) \
    (lpbi->biBitCount <= 8 ? (1 << (int)lpbi->biBitCount) : 0)

/****************************************************************************
 *
 *  get both the DISP(CF_DIB) and the DISP(CF_TEXT) info in one pass, this is
 *  much faster than doing multiple passes over the file.
 *
 ****************************************************************************/

static HANDLE GetRiffDisp(LPSTR lpszFile, LPSTR szText, int iLen)
{
    HMMIO       hmmio;
    MMCKINFO    ck;
    MMCKINFO    ckINFO;
    MMCKINFO    ckRIFF;
    HANDLE	h = NULL;
    LONG        lSize;
    DWORD       dw;
    HCURSOR     hcur = NULL;

    if (szText)
        szText[0] = 0;

    /* Open the file */
    hmmio = mmioOpen(lpszFile, NULL, MMIO_ALLOCBUF | MMIO_READ);

    if (hmmio == NULL)
        return NULL;

    mmioSeek(hmmio, 0, SEEK_SET);

    /* descend the input file into the RIFF chunk */
    if (mmioDescend(hmmio, &ckRIFF, NULL, 0) != 0)
        goto error;

    if (ckRIFF.ckid != FOURCC_RIFF)
        goto error;

    while (!mmioDescend(hmmio, &ck, &ckRIFF, 0))
    {
        if (ck.ckid == FOURCC_DISP)
        {
            if (hcur == NULL)
                hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

            /* Read dword into dw, break if read unsuccessful */
            if (mmioRead(hmmio, (LPVOID)&dw, sizeof(dw)) != sizeof(dw))
                goto error;

            /* Find out how much memory to allocate */
            lSize = ck.cksize - sizeof(dw);

            if ((int)dw == CF_DIB && h == NULL)
            {
                /* get a handle to memory to hold the description and lock it down */
                if ((h = GlobalAlloc(GHND, lSize+4)) == NULL)
                    goto error;

                if (mmioRead(hmmio, GlobalLock(h), lSize) != lSize)
                    goto error;
            }
            else if ((int)dw == CF_TEXT && szText[0] == 0)
            {
                if (lSize > iLen-1)
                    lSize = iLen-1;

                szText[lSize] = 0;

                if (mmioRead(hmmio, szText, lSize) != lSize)
                    goto error;
            }
        }
        else if (ck.ckid    == FOURCC_LIST &&
                 ck.fccType == FOURCC_INFO &&
                 szText[0]  == 0)
        {
            while (!mmioDescend(hmmio, &ckINFO, &ck, 0))
            {
                switch (ckINFO.ckid)
                {
                    case FOURCC_INAM:
//                  case FOURCC_ISBJ:

                        lSize = ck.cksize;

                        if (lSize > iLen-1)
                            lSize = iLen-1;

                        szText[lSize] = 0;

                        if (mmioRead(hmmio, szText, lSize) != lSize)
                            goto error;

                        break;
                }

                if (mmioAscend(hmmio, &ckINFO, 0))
                    break;
            }
        }

        //
        // if we have both a picture and a title, then exit.
        //
        if (h != NULL && szText[0] != 0)
            break;

        /* Ascend so that we can descend into next chunk
         */
        if (mmioAscend(hmmio, &ck, 0))
            break;
    }

    goto exit;

error:
    if (h)
        GlobalFree(h);

    h = NULL;
    ckRIFF.fccType = 0;

exit:
    mmioClose(hmmio, 0);

    //
    // verify and correct the DIB
    //
    if (h)
    {
        LPBITMAPINFOHEADER lpbi;

        lpbi = (LPVOID)GlobalLock(h);

        if (lpbi->biSize < sizeof(BITMAPINFOHEADER))
            goto error;

        if (lpbi->biClrUsed == 0)
            lpbi->biClrUsed = DibNumColors(lpbi);

        if (lpbi->biSizeImage == 0)
            lpbi->biSizeImage = DibSizeImage(lpbi);

        if (DibSize(lpbi) > GlobalSize(h))
            goto error;
    }

    if (hcur)
        SetCursor(hcur);

    return h;
}

#endif ///////////////////// DONT NEED THIS
