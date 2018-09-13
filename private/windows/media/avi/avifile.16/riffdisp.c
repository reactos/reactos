/****************************************************************************
 *
 *  MODULE  : RIFFDISP.C
 *
 ****************************************************************************/

#include <windows.h>
#include <win32.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <drawdib.h>
#include "riffdisp.h"
#include "avifile.h"

static  HWND        hwndPreview;
static  HANDLE      hdibPreview;
static  char        achPreview[80];
static  HFONT       hfontPreview;

static  HDRAWDIB    hdd;

#define GetHInstance()  (HINSTANCE)(SELECTOROF((LPVOID)&hwndPreview))

#define DibSizeImage(lpbi) (\
    (DWORD)(UINT)((((int)lpbi->biBitCount*(int)lpbi->biWidth+31)&~31)>>3) * \
    (DWORD)(UINT)lpbi->biHeight)

#define DibSize(lpbi) \
    (lpbi->biSize + ((int)lpbi->biClrUsed * sizeof(RGBQUAD)) + lpbi->biSizeImage)

#define DibNumColors(lpbi) \
    (lpbi->biBitCount <= 8 ? (1 << (int)lpbi->biBitCount) : 0)

/***************************************************************************
 *
 ****************************************************************************/

//#define FOURCC_RIFF mmioFOURCC('R','I','F','F')
#define FOURCC_AVI  mmioFOURCC('A','V','I',' ')
#define FOURCC_INFO mmioFOURCC('I','N','F','O')
#define FOURCC_DISP mmioFOURCC('D','I','S','P')
#define FOURCC_INAM mmioFOURCC('I','N','A','M')
#define FOURCC_ISBJ mmioFOURCC('I','S','B','J')

BOOL   PreviewOpen(HWND hwnd);
BOOL   PreviewFile(HWND hwnd, LPSTR szFile);
BOOL   PreviewPaint(HWND hwnd);
BOOL   PreviewClose(HWND hwnd);

HANDLE ReadDisp(LPSTR lpszFile, int cf, LPSTR pv, int iLen);
HANDLE ReadInfo(LPSTR lpszFile, FOURCC fcc, LPSTR pv, int iLen);
HANDLE GetRiffDisp(LPSTR lpszFile, LPSTR szText, int iLen);

/***************************************************************************
 *
 ****************************************************************************/

BOOL PreviewOpen(HWND hwnd)
{
    LOGFONT lf;

    if (hwndPreview)
        return FALSE;

    hwndPreview = hwnd;

    hdd = DrawDibOpen();

    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), (LPVOID)&lf, 0);
    hfontPreview = CreateFontIndirect(&lf);
}

/***************************************************************************
 *
 ****************************************************************************/

BOOL PreviewClose(HWND hwnd)
{
    if (hwndPreview != hwnd)
        return FALSE;

    if (hdibPreview)
        GlobalFree(hdibPreview);

    if (hfontPreview)
        DeleteObject(hfontPreview);

    if (hdd)
        DrawDibClose(hdd);

    achPreview[0] = 0;
    hdd           = NULL;
    hwndPreview   = NULL;
    hdibPreview   = NULL;
    hfontPreview  = NULL;
}

/***************************************************************************
 *
 ****************************************************************************/

BOOL PreviewFile(HWND hwnd, LPSTR szFile)
{
    if (hwndPreview != hwnd)
        return FALSE;

    achPreview[0] = 0;

    if (hdibPreview)
        GlobalFree(hdibPreview);

    hdibPreview = NULL;

    if (szFile)
    {
        hdibPreview = GetRiffDisp(szFile, achPreview, sizeof(achPreview));
    }

    PreviewPaint(hwnd);
    return TRUE;
}

/***************************************************************************
 *
 ****************************************************************************/

BOOL PreviewPaint(HWND hwnd)
{
    RECT    rc;
    RECT    rcPreview;
    RECT    rcImage;
    RECT    rcText;
    HDC     hdc;
    HBRUSH  hbr;
    int     dx;
    int     dy;
    LPBITMAPINFOHEADER lpbi;

    if (hwndPreview != hwnd)
        return FALSE;

    //
    // locate the preview in the lower corner of the dialog (below the
    // cancel button)
    //
//////!!! find a better way to do this.
    GetClientRect(hwnd, &rcPreview);
    GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &rc);
    ScreenToClient(hwnd, (LPPOINT)&rc);
    ScreenToClient(hwnd, (LPPOINT)&rc+1);

    rcPreview.top   = rc.bottom + (rc.bottom - rc.top) + 12;
    rcPreview.left  = rc.left;
    rcPreview.right = rc.right;
    rcPreview.bottom -= 4;          // leave a little room at the bottom
//////

    hdc = GetDC(hwnd);
    hbr = (HBRUSH)DefWindowProc(hwnd, WM_CTLCOLOR, (WPARAM)hdc, MAKELONG(hwnd, CTLCOLOR_DLG));
    SelectObject(hdc, hfontPreview);
    SetStretchBltMode(hdc, COLORONCOLOR);

    InflateRect(&rcPreview, 4, 1);
    FillRect(hdc, &rcPreview, hbr);
    IntersectClipRect(hdc, rcPreview.left, rcPreview.top, rcPreview.right, rcPreview.bottom);
    InflateRect(&rcPreview, -4, -1);

    //
    // compute the text rect, using DrawText
    //
    rcText = rcPreview;
    rcText.bottom = rcText.top;

    DrawText(hdc, achPreview, -1, &rcText, DT_CALCRECT|DT_LEFT|DT_WORDBREAK);

    //
    // compute the image size
    //
    if (hdibPreview && hdd)
    {
        lpbi = (LPVOID)GlobalLock(hdibPreview);

#if 0
        //
        // DISP(CF_DIB) chunks are screwed up they contain a DIB file! not
        // a CF_DIB, skip over the header if it is there.
        //
        if (lpbi->biSize != sizeof(BITMAPINFOHEADER))
            (LPSTR)lpbi += sizeof(BITMAPFILEHEADER);
#endif

        rcImage = rcPreview;

        //
        //  if wider than preview area scale to fit
        //
        if ((int)lpbi->biWidth > rcImage.right - rcImage.left)
        {
            rcImage.bottom = rcImage.top + MulDiv((int)lpbi->biHeight,rcImage.right-rcImage.left,(int)lpbi->biWidth);
        }
        //
        //  if x2 will fit then use it
        //
        else if ((int)lpbi->biWidth * 2 < rcImage.right - rcImage.left)
        {
            rcImage.right  = rcImage.left + (int)lpbi->biWidth*2;
            rcImage.bottom = rcImage.top + (int)lpbi->biHeight*2;
        }
        //
        //  else center the image in the preview area
        //
        else
        {
            rcImage.right  = rcImage.left + (int)lpbi->biWidth;
            rcImage.bottom = rcImage.top + (int)lpbi->biHeight;
        }

	if (rcImage.bottom > rcPreview.bottom - (rcText.bottom - rcText.top))
	{
	    rcImage.bottom = rcPreview.bottom - (rcText.bottom - rcText.top);

	    rcImage.right = rcPreview.left + MulDiv((int)lpbi->biWidth,rcImage.bottom-rcImage.top,(int)lpbi->biHeight);
	    rcImage.left = rcPreview.left;
	}
    }
    else
    {
        SetRectEmpty(&rcImage);
    }

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
    //  now draw
    //
    DrawText(hdc, achPreview, -1, &rcText, DT_LEFT|DT_WORDBREAK);

    if (hdibPreview && hdd)
    {
        DrawDibDraw(hdd,
                    hdc,
                    rcImage.left,
                    rcImage.top,
                    rcImage.right  - rcImage.left,
                    rcImage.bottom - rcImage.top,
                    lpbi,
                    NULL,
                    0,
                    0,
                    -1,
                    -1,
                    0);

        InflateRect(&rcImage, 1, 1);
        FrameRect(hdc, &rcImage, GetStockObject(BLACK_BRUSH));
    }

    ReleaseDC(hwnd, hdc);
    return TRUE;
}

/***************************************************************************
 *
 ****************************************************************************/

static UINT    (CALLBACK *lpfnOldHook)(HWND, UINT, WPARAM, LPARAM);

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

WORD FAR PASCAL _export GetFileNamePreviewHook(HWND hwnd, unsigned msg, WORD wParam, LONG lParam)
{
    int i;
    char ach[80];

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
                    PreviewFile(hwnd, NULL);
                    break;

                case cmb1:
                case cmb2:
                case lst2:
                    if (HIWORD(lParam) == LBN_SELCHANGE)
                    {
                        KillTimer(hwnd, ID_TIMER);
                        PreviewFile(hwnd, NULL);
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
                PreviewFile(hwnd, ach);
                return TRUE;
            }
            break;

        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
        case WM_PAINT:
            PreviewPaint(hwnd);
            break;

        case WM_INITDIALOG:
            PreviewOpen(hwnd);

            if (!lpfnOldHook)
                return TRUE;

            break;

        case WM_DESTROY:
            PreviewClose(hwnd);
            break;
    }

    if (lpfnOldHook)
        return (*lpfnOldHook)(hwnd, msg, wParam, lParam);
    else
        return FALSE;
}

/**************************************************************************
* @doc EXTERNAL AVIGetStream
*
* @api BOOL | GetOpenFileNamePreview | This function is similar 
*      to <f GetOpenFileName> defined in COMMDLG.DLL except that 
*      it has a preview window to display the movie about to be opened.
*
* @parm LPOPENFILENAME | lpofn | Points to an <t OPENFILENAME> structure 
*       used to initialize the dialog box. On return, the structure 
*       contains information about the user's file selection.
*
* @rdesc Returns true if a file was opened.
*
* @comm For more information on this function, see the description for 
*       <f GetOpenFileName>.
*
* @xref <f GetOpenFileName>
*
*************************************************************************/
BOOL FAR PASCAL GetOpenFileNamePreview(LPOPENFILENAME lpofn)
{
    BOOL fHook;
    BOOL f;

    if (hwndPreview)
        return GetOpenFileName(lpofn);

    fHook = (BOOL)(lpofn->Flags & OFN_ENABLEHOOK);

    if (fHook)
        lpfnOldHook = lpofn->lpfnHook;

    (FARPROC)lpofn->lpfnHook = MakeProcInstance((FARPROC)GetFileNamePreviewHook, GetHInstance());
    lpofn->Flags |= OFN_ENABLEHOOK;

    f = GetOpenFileName(lpofn);

    FreeProcInstance((FARPROC)lpofn->lpfnHook);

    if (fHook)
        lpofn->lpfnHook = lpfnOldHook;
    else
        lpofn->Flags &= ~OFN_ENABLEHOOK;

    return f;
}

HANDLE AVIFirstFrame(LPSTR szFile)
{
    HANDLE h = NULL;
    LPBITMAPINFOHEADER lpbi;
    DWORD dwSize;
    PAVISTREAM pavi;
    PGETFRAME pg;

    if (AVIStreamOpenFromFile(&pavi, szFile, streamtypeVIDEO, 0, OF_READ, NULL) == AVIERR_OK)
    {
	pg = AVIStreamGetFrameOpen(pavi, NULL);
	if (pg) {
	    lpbi = AVIStreamGetFrame(pg, 0);

	    if (lpbi)
	    {
		dwSize = lpbi->biSize + lpbi->biSizeImage + lpbi->biClrUsed * sizeof(RGBQUAD);
		h = GlobalAlloc(GHND, dwSize);

		if (h)
		    hmemcpy(GlobalLock(h), lpbi, dwSize);
	    }

	    AVIStreamGetFrameClose(pg);
	}
        AVIStreamClose(pavi);
    }

    return h;
}

/****************************************************************************
 *
 *  get both the DISP(CF_DIB) and the DISP(CF_TEXT) info in one pass, this is
 *  much faster than doing multiple passes over the file.
 *
 ****************************************************************************/

HANDLE GetRiffDisp(LPSTR lpszFile, LPSTR szText, int iLen)
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
    // !!!we need a way to preview other file types!
    // !!!what about text.
    //
    if (h == NULL && ckRIFF.fccType == FOURCC_AVI)
    {
        if (hcur == NULL)
            hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

        h = AVIFirstFrame(lpszFile);
    }

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
