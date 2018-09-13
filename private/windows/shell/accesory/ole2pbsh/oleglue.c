//
// FILE:    oleglue.c
//
// All OLE-related outbound references from PBrush
//

#include <windows.h>
#include <windowsx.h>

#include "oleglue.h"
#include "pbrush.h"


//
// GLOBALS
//
BOOL gfInDialog = FALSE;
BOOL gfTerminating = FALSE;
BOOL gfClosing = FALSE;

DWORD dwOleBuildVersion = 0;    // OLE library version number
BOOL gfOleInitialized = FALSE;  // did OleInitialize suceed?
HINSTANCE ghInst = 0;

BOOL gfUserClose = FALSE;

BOOL gfStandalone = TRUE;
BOOL gfEmbedded = FALSE;
BOOL gfLinked = FALSE;
BOOL gfInPlace = FALSE;

BOOL gfInvisible = FALSE;       //
BOOL gfLoading = FALSE;         // Are we loading a file?
int iExitWithSaving = IDNO;     //

BOOL gfWholeHog = FALSE;
BOOL gfTransfer = FALSE;

HICON ghiconApp = NULL;

OLECHAR gachLinkFilename[_MAX_PATH];

//
// for Access to global hwnds
//
HWND *gpahwndApp = NULL;
LPRECT gprcApp = NULL;


//
// Pick state save/restore globals
//
    RECT grcOldPick;
    int giOldPickWidth;
    int giOldPickHeight;

//
// Transfer Globals
//
//
// Transfer Globals
//
BOOL        gfXBagOnClipboard = FALSE;
HBITMAP     ghBitmapSnapshot = NULL;
HPALETTE    ghPaletteSnapshot = NULL;
HBITMAP     ghbmSnapTemp = NULL;
HPALETTE    ghpalTemp = NULL;
RECT        grcSnapshot;
HGLOBAL     ghGlobalToPaste = NULL;
CLIPFORMAT  gcfToPaste = 0;

void
BuildUniqueLinkName(void)
{
    //
    //Ensure a unique filename in gachLinkFilename so we can create valid
    //FileMonikers...
    //
    if(gachLinkFilename[0] == L'\0')
        GetTempFileName(L".", L"Tmp", 0, gachLinkFilename);
}

void
SetTransferFlag(BOOL fTransfer)
{
    gfTransfer = fTransfer;
}

//
// Pick state save/restore utilities
// =================================
//
void
SavePickState(void)
{
    grcOldPick = pickRect;
    giOldPickWidth = pickWid;
    giOldPickHeight = pickHgt;
}

void
SelectWholePicture(void)
{
    imageRect.left = imageRect.top = 0;
    imageRect.right = imageWid;
    imageRect.bottom = imageHgt;
    pickRect = imageRect;
    pickWid = imageWid - 1;
    pickHgt = imageHgt - 1;
}

void GetPickRect(LPRECT prc)
{
    *prc = grcSnapshot;
}

void
RestorePickState(void)
{
    pickRect = grcOldPick;
    pickWid = giOldPickWidth;
    pickHgt = giOldPickHeight;
}

void
FreeGlobalToPaste(void)
{
    DOUT(L"PBrush:FreeGlobalToPaste\n\r");
    if(ghGlobalToPaste != NULL)
    {
        switch(gcfToPaste)
        {
        case CF_BITMAP:
            DeleteObject(ghGlobalToPaste);
            break;
        case CF_METAFILEPICT:
        {
            LPMETAFILEPICT pMF = GlobalLock(ghGlobalToPaste);
            DeleteMetaFile(pMF->hMF);
            GlobalUnlock(ghGlobalToPaste);
            GlobalFree(ghGlobalToPaste);
            break;
        }
        default:
            GlobalFree(ghGlobalToPaste);
            break;
        }
    }
    ghGlobalToPaste = NULL;
}

void
SetupForDrop(CLIPFORMAT cf, POINTL ptl)
{
    LONG lparm;
    POINTS *pmpt = (POINTS *)&lparm;
    if(cf == CF_TEXT || cf == CF_UNICODETEXT)
    {
        pmpt->x = (SHORT)ptl.x;
        pmpt->y = (SHORT)ptl.y;
        ToolWP(pbrushWnd[TOOLid], WM_SELECTTOOL, TEXTtool, 0L);
        Text2DP(pbrushWnd[PAINTid], WM_LBUTTONDOWN, 0, lparm);
    }
}

void
PasteTypedHGlobal(CLIPFORMAT cf, HGLOBAL hGlobal)
{
    DOUT(L"PBrush:PasteTypedHGlobal\n\r");
    FreeGlobalToPaste();
    gcfToPaste = cf;
    ghGlobalToPaste = hGlobal;
    switch(cf)
    {
    case CF_BITMAP:
    case CF_DIB:
    case CF_METAFILEPICT:
        Terminatewnd();
        if(theTool != SCISSORStool)
            SendMessage(pbrushWnd[TOOLid], WM_SELECTTOOL, PICKtool, 0L);
        gfDirty = TRUE;
        break;
    case CF_TEXT:
        if(theTool != TEXTtool)
            SendMessage(pbrushWnd[TOOLid], WM_SELECTTOOL, TEXTtool, 0L);
        break;
    }
    SendMessage(bZoomedOut ? zoomOutWnd : pbrushWnd[PAINTid], WM_PASTE, 0, 0L);
    gfDirty = TRUE;
    hGlobal = NULL;
}

//
// GetTransferBitmap is called by our data-transfer (CXBag) object
// when it's asked for CF_BITMAP in the course of a paste operation
//
HBITMAP GetTransferBitmap(void)
{
    if(ghBitmapSnapshot == NULL)
        return NULL;
    return CopyBitmap(ghBitmapSnapshot);
}
//
// GetTransferPalette is called by our data-transfer (CXBag) object
// when it's asked for CF_PALETTE in the course of a paste operation
//
HPALETTE GetTransferPalette(void)
{
    if(ghPaletteSnapshot == NULL)
        return NULL;
    return CopyPalette(ghPaletteSnapshot);
}

//
// RenderPicture is called by the view object Draw routine
//
void
RenderPicture(HDC hdc, LPCRECTL lprectl)
{
    long lx, ly;
    int cx, cy;
    HBITMAP hbm;
    HDC hdcSrc = hdcWork;

    if(gfTransfer)
    {
        hbm = SelectObject(hdcSrc, ghBitmapSnapshot);
        cx = grcSnapshot.right - grcSnapshot.left;
        cy = grcSnapshot.bottom - grcSnapshot.top;
    }
    else
    {
        cx = imageWid;
        cy = imageHgt;
    }

    if (ghPaletteSnapshot)
    {
       SelectPalette(hdc, ghPaletteSnapshot, FALSE);
       RealizePalette(hdc);
    }

    lx = lprectl->left;
    ly = lprectl->top;
    StretchBlt(hdc, lx, ly, 1 + lprectl->right  - lx, 1 + lprectl->bottom - ly,
                hdcSrc, 0, 0, cx, cy, SRCCOPY);

    if(gfTransfer)
        SelectObject(hdcSrc, hbm);
}

//
// Code ported from pbserver.c (OLE1) for serialization...
//=========================================================
//
void PUBLIC NewImage(int);               /* MenuCmd.C */
#define DIBID           0x4D42
#define nPelsPerLogInch GetDeviceCaps(hDC, bHoriz ? LOGPIXELSX : LOGPIXELSY)

int PelsPerLogMeter(HDC hDC,BOOL bHoriz)
{
    #define nMMPerMeterTimes10 10000
    #define nMMPerInchTimes10  254

    return MulDiv(nPelsPerLogInch, nMMPerMeterTimes10, nMMPerInchTimes10);
}

HANDLE
GetNativeData(void)
{
    //
    // The native format is the entire DIB file format
    //
    int i;
    int width, height;
    BOOL error = TRUE;
    LPBYTE lpBits;
    LPBYTE hpBits;
    HDC parentDC = NULL;
    int ScanLineSize, InfoSize;
    DWORD dwImgSize;
    HANDLE hLink = NULL;
    LPBYTE lpLink = NULL;
    HCURSOR hOldCursor;
    HBITMAP hOldBitmap = NULL;
    HPALETTE hOldPalette = NULL;
    LPBITMAPFILEHEADER  lpHdr;
    LPBITMAPINFO lpInfo;
    LPBITMAPINFO lpbmInfo = NULL;           /* deal with alignment -- FGS */
    HANDLE hbmInfo = NULL;
    int ht;
    HBITMAP hbmOld;
    HDC hdcSrc = hdcWork;
    BOOL fSnapshot = !IsRectEmpty(&grcSnapshot) && gfTransfer;


    hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    /* Compute the rectangle to be selected */
    if(fSnapshot)
    {
        width = (grcSnapshot.right - grcSnapshot.left);
        height = (grcSnapshot.bottom - grcSnapshot.top);
        hbmOld = SelectObject(hdcSrc, ghBitmapSnapshot);
    }
    else
    {
        width = (imageView.right - imageView.left);
        height = (imageView.bottom - imageView.top);
    }

    if (!(fileDC = CreateCompatibleDC(NULL)))
    {
        PbrushOkError(IDSNotEnufMem, MB_ICONHAND);
        goto error1;
    }

    if (hPalette)
    {
       hOldPalette = SelectPalette(fileDC, hPalette, 0);
       RealizePalette(fileDC);
    }

    switch (wFileType)
    {
        case BITMAPFILE24:
            i = 24;
            break;

        case BITMAPFILE:
        case MSPFILE:
            i = 1;
            break;

        case BITMAPFILE4:
            i = 4;
            break;

        case BITMAPFILE8:
            i = 8;
            break;
        case PCXFILE:
            i = imagePlanes * imagePixels;
            break;
    }
    /* BITMAPINFOSIZE = size of header + color table. */
    /* no color table for 24 bpp bitmaps. */
    InfoSize = sizeof(BITMAPINFOHEADER) +
                ((i == 24)? 0: (sizeof(RGBQUAD) << i));
    /* Scan Line should be DWORD aligned and the length is in bytes */
    ScanLineSize = ((((WORD)width * (WORD)i) + 31)/32) * 4;

    /* Allocate memory */
    dwImgSize = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)InfoSize +
                (DWORD)ScanLineSize * (DWORD)height;

    hLink = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, dwImgSize);
    if (!hLink || (!(lpLink = (LPBYTE)GlobalLock(hLink))))
    {
        SimpleMessage(IDSNoMemAvail, fileName, MB_OK | MB_ICONEXCLAMATION);
        goto error2;
    }
    hbmInfo = GlobalAlloc(GHND, InfoSize);
    if (!hbmInfo || (!(lpbmInfo = (LPBITMAPINFO)GlobalLock(hbmInfo))))
    {
        SimpleMessage(IDSNoMemAvail, fileName, MB_OK | MB_ICONEXCLAMATION);
        goto error2;
    }
    lpHdr  = (LPBITMAPFILEHEADER)lpLink;
    lpInfo = (LPBITMAPINFO)(lpLink + sizeof(BITMAPFILEHEADER));
    lpBits = (LPBYTE)(lpLink + sizeof(BITMAPFILEHEADER) + InfoSize);

    /* fill in image header */
    lpbmInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    lpbmInfo->bmiHeader.biWidth         = width;
    lpbmInfo->bmiHeader.biHeight        = height;
    lpbmInfo->bmiHeader.biPlanes        = 1;
    lpbmInfo->bmiHeader.biBitCount      = (WORD)i;
    lpbmInfo->bmiHeader.biCompression   =
    lpbmInfo->bmiHeader.biClrUsed       =
    lpbmInfo->bmiHeader.biClrImportant  = 0;
    lpbmInfo->bmiHeader.biSizeImage     = 0;
    lpHdr->bfOffBits = sizeof(BITMAPFILEHEADER) + InfoSize;
    lpbmInfo->bmiHeader.biXPelsPerMeter = PelsPerLogMeter(fileDC,TRUE);
    lpbmInfo->bmiHeader.biYPelsPerMeter = PelsPerLogMeter(fileDC,FALSE);


    for (ht = height, fileBitmap = NULL; ht && !fileBitmap; )
    {
        if (!(fileBitmap = CreateBitmap(width, ht, (BYTE)imagePlanes,
                                        (BYTE)imagePixels, NULL)))
        {
            ht = ht >> 1;
        }
    }

    if (!ht)
    {
        PbrushOkError(IDSNotEnufMem, MB_ICONHAND);
        goto error2;
    }

    lpbmInfo->bmiHeader.biHeight = ht;
    for (i = height; i; i -= ht)
    {
        if (ht > i)
            lpbmInfo->bmiHeader.biHeight = ht = i;

        if(!(hOldBitmap = SelectObject(fileDC, fileBitmap)))
        {
           SimpleMessage(IDSUnableSave, fileName, MB_OK | MB_ICONEXCLAMATION);
           goto error3;
        }

        BitBlt(fileDC, 0, 0, width, ht,
                hdcSrc,
                fSnapshot ? 0 : imageView.left,
                (fSnapshot ? 0 : imageView.top) + (i - ht), SRCCOPY);
                //fSnapshot ? grcSnapshot.left : imageView.left,
                //(fSnapshot ? grcSnapshot.top : imageView.top) + (i - ht), SRCCOPY);

        if(!SelectObject(fileDC, hOldBitmap))
        {
           SimpleMessage(IDSUnableSave, fileName, MB_OK | MB_ICONEXCLAMATION);
           goto error3;
        }

        GetDIBits(fileDC, fileBitmap, 0, ht, lpBits, lpbmInfo, DIB_RGB_COLORS);
        hpBits = lpBits;
        hpBits += (DWORD)ht * (DWORD)ScanLineSize;
        lpBits = hpBits;
    }
    lpbmInfo->bmiHeader.biHeight = height;

    lpHdr->bfSize = dwImgSize;
    lpHdr->bfType = DIBID;
    lpHdr->bfReserved1 = lpHdr->bfReserved2 = 0;
    lpbmInfo->bmiHeader.biSizeImage = (DWORD)height * (DWORD)ScanLineSize;

    RepeatMove(lpInfo, lpbmInfo, InfoSize);
    error = FALSE;

error3:
    if (fileBitmap)
        DeleteObject(fileBitmap);

error2:
    if (lpLink)
        GlobalUnlock(hLink);

    if (lpbmInfo)
        GlobalUnlock(hbmInfo);

    if (hbmInfo)
    {
        GlobalFree(hbmInfo);
        hbmInfo = NULL;
    }

    if (error && hLink)
    {
        GlobalFree(hLink);
        hLink = NULL;
    }

    if (fileDC)
    {
        if (hPalette && hOldPalette)
            SelectPalette(fileDC, hOldPalette, 0);
        DeleteDC(fileDC);
    }

error1:
    if(fSnapshot)
        SelectObject(hdcSrc, hbmOld);
    SetCursor(hOldCursor);
    return hLink;
}

BOOL
PutNativeData(LPBYTE lpbData, HWND hWnd)
{
    LPBITMAPFILEHEADER  lphdr;
    BITMAPINFO  UNALIGNED * lphdrInfo;
    LPBITMAPINFO lpDIBinfo = NULL;               /* needed for alignment */
    BOOL error = TRUE;
    int wplanes, wbitpx, i;
    HANDLE hDIBinfo = NULL;
    HDC hdc = NULL;
    HDC parentdc = NULL;
    HBITMAP hbitmap = NULL;
    HBITMAP htempbit = NULL;
    HCURSOR oldCsr;
    UINT errmsg, wSize;
    UINT wUsage;
    HPALETTE hNewPal = NULL;
    LPBYTE lpTemp;
    int rc;
    LPBYTE hpTemp;
    int ht;
    DWORD dwSize, dwNumColors;

    lphdr = (LPBITMAPFILEHEADER)lpbData;
    lphdrInfo = (BITMAPINFO UNALIGNED *)(lpbData + sizeof(BITMAPFILEHEADER));

    if (lphdrInfo->bmiHeader.biPlanes != 1 || lphdr->bfType != DIBID)
    {
        errmsg = IDSUnableHdr;
        goto error1;
    }

    if (lphdrInfo->bmiHeader.biCompression)
    {
        errmsg = IDSUnknownFmt;
        goto error1;
    }

    if (!(dwNumColors = lphdrInfo->bmiHeader.biClrUsed))
    {
       if (lphdrInfo->bmiHeader.biBitCount != 24)
          dwNumColors = (1L << lphdrInfo->bmiHeader.biBitCount);
    }

    if (!(parentdc = GetDisplayDC(hWnd)))
    {
        errmsg = IDSCantAlloc;
        goto error1;
    }

    if (lphdrInfo->bmiHeader.biBitCount != 1)
    {
        wplanes = GetDeviceCaps(parentdc, PLANES);
        wbitpx = GetDeviceCaps(parentdc, BITSPIXEL);
    }
    else
    {
        wplanes = 1;
        wbitpx = 1;
    }

    oldCsr = SetCursor(LoadCursor(NULL, IDC_WAIT));

    /* Create a new image with the new sizes, etc. */
    nNewImageWidth  = LOWORD(lphdrInfo->bmiHeader.biWidth);
    nNewImageHeight = LOWORD(lphdrInfo->bmiHeader.biHeight);
    nNewImagePlanes = wplanes;
    nNewImagePixels = wbitpx;

    rc = AllocImg(nNewImageWidth,
                nNewImageHeight,
                nNewImagePlanes,
                nNewImagePixels,
                FALSE);

    dwSize = sizeof(BITMAPINFOHEADER)
           + (dwNumColors ? dwNumColors : 1) * sizeof(RGBQUAD);

    if (!(hDIBinfo = GlobalAlloc(GMEM_MOVEABLE, dwSize)))
    {
        errmsg = IDSCantAlloc;
        goto error2;
    }

    if (!(lpDIBinfo = (LPBITMAPINFO) GlobalLock(hDIBinfo)))
    {
        errmsg = IDSCantAlloc;
        goto error3;
    }

    /* copy header into allocated memory */
    RepeatMove((LPBYTE)lpDIBinfo, (LPBYTE)lphdrInfo, (WORD)dwSize);
    hNewPal = MakeImagePalette(hPalette, hDIBinfo, &wUsage);
    if (hNewPal && hNewPal != hPalette)
    {
        SelectPalette(hdcWork, hNewPal, 0);
        RealizePalette(hdcWork);
        SelectPalette(hdcImage, hNewPal, 0);
        RealizePalette(hdcImage);
#ifdef WANT_RIPS
        DeleteObject(hPalette);
#endif
        hPalette = hNewPal;
    }

    hdc = CreateCompatibleDC(parentdc);
    ReleaseDC(hWnd, parentdc);
    parentdc = NULL;

    if (!hdc)
    {
        errmsg = IDSNoMemAvail;
        goto error4;
    }

    for (ht = nNewImageHeight, hbitmap = NULL; ht && !hbitmap; )
    {
        if (!(hbitmap = CreateBitmap(nNewImageWidth,
                                    ht,
                                    (BYTE)imagePlanes,
                                    (BYTE)imagePixels,
                                    NULL)))
        {
            ht = ht >> 1;
        }
    }

    if (!ht)
    {
        errmsg = IDSNoMemAvail;
        goto error5;
    }

    if (hPalette)
    {
       SelectPalette(hdc, hPalette, FALSE);
       RealizePalette(hdc);
    }

    wSize = ((nNewImageWidth
                * lphdrInfo->bmiHeader.biBitCount + 31) & (-32)) >> 3;

    lpTemp = lpbData + sizeof(BITMAPFILEHEADER) + dwSize;

    error = FALSE;

    if (!(htempbit = SelectObject(hdc, hbitmap)))
    {
        errmsg = IDSNoMemAvail;
        goto error6;
    }

    lpDIBinfo->bmiHeader.biHeight = ht;
    for (i = nNewImageHeight; i; i -= ht)
    {
        if (i < ht)
             lpDIBinfo->bmiHeader.biHeight = ht = i;

        if(!SelectObject(hdc, htempbit))
        {
           errmsg = IDSNoMemAvail;
           goto error6;
        }

        if(!SetDIBits(hdc, hbitmap, 0, ht, lpTemp, lpDIBinfo, wUsage))
        {
            errmsg = IDSNoMemAvail;
            error = TRUE;
            break;
        }

        if(!SelectObject(hdc, hbitmap))
        {
           errmsg = IDSNoMemAvail;
           goto error6;
        }

        BitBlt(hdcWork, 0, i - ht, nNewImageWidth, ht,
                hdc, 0, 0, SRCCOPY);

        hpTemp = lpTemp;
        hpTemp += (DWORD)ht * (DWORD)wSize;
        lpTemp = hpTemp;
    }

    /* Copy the work bitmap into the undo bitmap */
    BitBlt(hdcImage, 0, 0, nNewImageWidth, nNewImageHeight,
            hdcWork, 0, 0, SRCCOPY);

    wFileType = GetImageFileType(lphdrInfo->bmiHeader.biBitCount);

    if (error)
        ClearImg();

    NewImage(0);

    if (!error)
        GetCurrentDirectory(PATHlen, filePath);

error6:
    if (htempbit)
        SelectObject(hdc, htempbit);

    DeleteObject(hbitmap);

error5:
    DeleteDC(hdc);

error4:
    GlobalUnlock(hDIBinfo);

error3:
    GlobalFree(hDIBinfo);

error2:
    SetCursor(oldCsr);
    if (parentdc)
        ReleaseDC(hWnd, parentdc);

error1:
    return error;
}

//
// UTILITIES
//

BOOL ParseEmbedding(LPTSTR lpString, LPTSTR lpPattern)
{
    LPTSTR lpTmp;
    LPTSTR lpTmp2;

    while (TRUE)
    {
        while (*lpString && *lpString != *lpPattern)
            lpString++;

        if (!(*lpString))
            return FALSE;

        lpTmp = lpPattern;
        lpTmp2 = lpString++;
        while (*lpTmp && *lpTmp2 && *lpTmp == *lpTmp2)
        {
            lpTmp++; lpTmp2++;
        }

        if (!(*lpTmp))
        {
            //
            //Parse possible linking filename and set up globals:
            //         gfEmbedded, gfLinked, gachAnsiFilename
            //
            while(*lpTmp2 && *lpTmp2 == L' ')
                ++lpTmp2;
            // Wipe out the "/embedding" string
            lstrcpy(--lpString, lpTmp2);
            if(*lpTmp)
            {
                lstrcpy(gachLinkFilename, lpTmp);
                gfLinked = TRUE;
            }
            else
                gfEmbedded = TRUE;
            return TRUE;
        }

        if (!(*lpTmp2))
            return FALSE;
    }
}

#define CBMENUITEMMAX   80

void
ModifyMenusForEmbedding(void)
{
    TCHAR szTemp[CBMENUITEMMAX];
#ifdef MODIFY_EXIT
    TCHAR szBuffer[KEYNAMESIZE + 100];
    TCHAR szMenuStrFormat[CBMENUITEMMAX];
    LoadString(hInst, IDS_EXITANDRETURN, szMenuStrFormat, CharSizeOf(szMenuStrFormat));
    wsprintf(szBuffer, szMenuStrFormat, GetClientObjName());
    ModifyMenu(ghMenuFrame, FILEexit, MF_BYCOMMAND | MF_STRING, FILEexit, szBuffer);
#endif //MODIFY_EXIT
#ifdef OBSOLETE
    LoadString(hInst, IDS_UPDATE, szTemp, CharSizeOf(szTemp));
    ModifyMenu(ghMenuFrame, FILEsave, MF_BYCOMMAND | MF_STRING, FILEupdate, szTemp);
#endif //OBSOLETE
    DeleteMenu(ghMenuFrame, FILEsave, MF_BYCOMMAND);
}



BOOL InitializePBS(HINSTANCE hInst, LPTSTR lpCmdLine)
{
    static TCHAR szEmbedding[] = TEXT("-Embedding");
    static TCHAR szEmbedding2[] = TEXT("/Embedding");
    BOOL fOLE, fServer;
#if DBG
    OLECHAR achBuffer[256];
#endif

    SetRectEmpty(&grcSnapshot);
    ghInst = hInst;
    gpahwndApp = pbrushWnd;
    gprcApp = pbrushRct;
    gfUserClose = FALSE;
    gachLinkFilename[0] = 0;
    fServer = ParseEmbedding(lpCmdLine, szEmbedding)
              || ParseEmbedding(lpCmdLine, szEmbedding2);
    gfStandalone = !fServer;

#if DBG
    wsprintf(achBuffer,
            L"PBrush: gfStandalone = %s\r\n",
            gfStandalone ? L"TRUE\r\n" : L"FALSE");
    DOUT(achBuffer);
#endif


    if(gfOleInitialized)
        fOLE = CreatePBClassFactory(hInst, gfEmbedded);
    else
        fOLE = FALSE;   //BUGBUG signal a serious problem!

    if (fOLE && fServer)
        gfInvisible = TRUE;

    ghiconApp = LoadIcon(hInst, pgmName);

    if(!gfStandalone)
        ModifyMenusForEmbedding();

    return fOLE;
}


//
// The original InitDoc routine allocated memory for a pbrush document
// structure, initialized a ptr to document function vtbl, registered
// the document with OLE and registered an atom based on the document
// title.
//
//void InitDoc(LPTSTR lptitle)

    extern DWORD gdwRegROT;

void TerminateServer(void)
{
    DOUT(L"PBrush: TerminateServer\r\n");

    gfTerminating = TRUE;
    if(gfStandalone)
        RevokeOurDropTarget();

    FlushOleClipboard();
    FreeGlobalToPaste();
    if(ghBitmapSnapshot)
    {
        DeleteObject(ghBitmapSnapshot);
        ghBitmapSnapshot = NULL;
    }
    if (ghPaletteSnapshot) {
        DeleteObject(ghPaletteSnapshot);
        ghPaletteSnapshot = NULL;
    }
    if(!gfClosing)
        DoOleClose(FALSE);
    if(!gfStandalone)
        ReleasePBClassFactory();
    DestroyWindow(gpahwndApp[iFrame]);
}

