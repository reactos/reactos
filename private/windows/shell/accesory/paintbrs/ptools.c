/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   ptools.c                                    *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  general tools for paintbrush                *
*   date:   12/19/89 @ 10:20                            *
********************************************************/

#include "onlypbr.h"

#undef NOMB
#undef NOWINMESSAGES
#undef NORASTEROPS
#undef NOCLIPBOARD
#undef NOKEYSTATES
#undef NOKERNEL
#undef NOCTLMGR
#undef NOWINOFFSETS
#undef NOLFILEIO
#undef NOOPENFILE
#undef NOLSTRING
#undef NOMEMMGR
#undef NOMENUS

#undef NOATOM                           /* OLE */
#include <windows.h>
#include "port1632.h"
//#define NOEXTERN
#include "pbrush.h"

#include "pbserver.h"                   /* OLE */
extern  RECT pickRect;                  /* OLE: Rectangle to copy to clip */
extern  RECT imageView;                 /* OLE: Scroll bar displacements  */

#define    MAXPALETTE  256
#define    PALVERSION  0x300

#define    SWAP(a,b)   ((a)^=(b)^=(a)^=(b))

/* define raster ops needed for MaskBlt */
#define    DSx 0x00660046L
#define    DSa 0x008800C6L
#define    DSna 0x00220326L

extern HWND pbrushWnd[];
extern int horzDotsMM, vertDotsMM;
extern int imagePlanes, imagePixels;
extern DWORD *rgbColor;
extern int theBackg;
extern HDC monoDC;
extern HPALETTE hPalette;

static BYTE byColorTable[] = { 0, 0x48, 0x98, 0xb8, 0xd0, 0xe0, 0xf0, 0xff };
static int bluetable[] = { 0, 2, 4, 7 };

/*----------------------------------------------------------------------------
  CropBitmap (hbm,lprect) - Returns a bitmap croped to new size
  ----------------------------------------------------------------------------*/
HBITMAP PUBLIC CropBitmap (HBITMAP hbm, PRECT prc, BOOL fScale)
{
    HDC     hMemDCsrc;
    HDC     hMemDCdst;
    HDC     hdc;
    HBITMAP hNewBm = NULL;
    BITMAP  bm;
    int     dx,dy;       /* size of new bitmap */

    if (!hbm)
       goto error1;

    GetObject(hbm, sizeof(BITMAP), (LPVOID) &bm);
    dx = prc->right  - prc->left;
    dy = prc->bottom - prc->top;

    hdc = GetDC(NULL);
    if (!(hMemDCsrc = CreateCompatibleDC(hdc)))
       goto error2;
    if(!SelectObject(hMemDCsrc,hbm))
       goto error3;

    if (!(hMemDCdst = CreateCompatibleDC(hdc)))
       goto error3;
    if (!(hNewBm = CreateBitmap(dx,dy,bm.bmPlanes,bm.bmBitsPixel,NULL)))
       goto error4;
    if(!SelectObject(hMemDCdst,hNewBm)) {
       DeleteObject(hNewBm);
       hNewBm = NULL;
       goto error4;
    }

    if (fScale) {
        SetStretchBltMode(hMemDCdst,
            bm.bmPlanes * bm.bmBitsPixel == 1 ? BLACKONWHITE : COLORONCOLOR);

        StretchBlt(hMemDCdst,0,0,dx,dy,
                hMemDCsrc,0,0,bm.bmWidth,bm.bmHeight,
                SRCCOPY
               );
    }
    else {
        BitBlt(hMemDCdst,0,0,dx,dy,
               hMemDCsrc,prc->left,prc->top,
               SRCCOPY
              );
    }

error4:
    DeleteDC(hMemDCdst);

error3:
    DeleteDC(hMemDCsrc);

error2:
    ReleaseDC(NULL,hdc);

error1:
    return hNewBm;
}

/*----------------------------------------------------------------------------
  CopyBitmap (hbm) - Returns a copy of a given bitmap
  ----------------------------------------------------------------------------*/
HBITMAP PUBLIC CopyBitmap (HBITMAP hbm)
{
    BITMAP  bm;
    RECT    rc;

    if (!hbm)
         return NULL;

    GetObject(hbm,sizeof(BITMAP),(LPVOID)&bm);
    rc.left   = 0;
    rc.top    = 0;
    rc.right  = bm.bmWidth;
    rc.bottom = bm.bmHeight;

    return CropBitmap(hbm,&rc,FALSE);
}

HPALETTE PUBLIC CopyPalette(HPALETTE hPal)
{
   WORD            wPalSize;
   HPALETTE        hNewPal = NULL;
   LOGPALETTE     *pPal;

   if (!hPal)
       goto cleanup;

   GetObject(hPal, sizeof(wPalSize), (LPVOID) &wPalSize);

   pPal = (LOGPALETTE *) LocalAlloc(LPTR, sizeof(LOGPALETTE) +
                                          wPalSize * sizeof(PALETTEENTRY));

   if (!pPal)
       goto cleanup;

   if (GetPaletteEntries(hPal, 0, wPalSize, pPal->palPalEntry) == wPalSize) {
       pPal->palNumEntries = wPalSize;
       pPal->palVersion    = PALVERSION;

       hNewPal = CreatePalette(pPal);
   }
   LocalFree((HANDLE) pPal);

cleanup:
   return hNewPal;
}

BOOL PUBLIC DumpBitmapToClipboard(HDC hDC, UINT msg, RECT Rect)
{
   HBITMAP hTempBM, hBitmap, hCopy;
   HPALETTE hPal;
   WORD error = IDSNoMemAvail;
   HDC hMemDC;
   BITMAP myBM;
   HCURSOR  oldCur;

   oldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

   if (!(hTempBM = CreateBitmap(1, 1, 1, 1, 0L)))
       goto Error1;
   if (!(hBitmap = SelectObject(hDC, hTempBM)))
       goto Error2;
   if (!(hCopy = CopyBitmap(hBitmap)))
       goto Error3;
   if (hPalette && !(hPal = CopyPalette(hPalette)))
       goto Error4;

   if (!(hMemDC = CreateCompatibleDC(NULL)))
       goto Error5;
   GetObject(hCopy, sizeof(BITMAP), (LPVOID)&myBM);
   MSetBitmapDimension(hCopy,
          (int)((long)myBM.bmWidth *254/GetDeviceCaps(hMemDC, LOGPIXELSX)),
          (int)((long)myBM.bmHeight*254/GetDeviceCaps(hMemDC, LOGPIXELSY)));

   error = IDSNoClipboard;
   if (!OpenClipboard(pbrushWnd[PAINTid]))
       goto Error6;
   if (!EmptyClipboard())
       goto Error7;

   SetClipboardData(CF_BITMAP, hCopy);
   if (hPalette)
       SetClipboardData(CF_PALETTE, hPal);

   if (fOLE) {         /* OLE:  Copy Native, Links to clipboard */
       RECT rc;

       rc = Rect;
       OffsetRect(&rc, imageView.left, imageView.top);
       CutCopyObjectFormats(hDC, hBitmap, rc, msg);
   }

   error = FALSE;

Error7:
   CloseClipboard();
Error6:
   DeleteDC(hMemDC);
Error5:
   if (hPalette && error)
       DeleteObject(hPal);
Error4:
   if (error)
       DeleteObject(hCopy);
Error3:
   SelectObject(hDC, hBitmap);
Error2:
   DeleteObject(hTempBM);
Error1:
   if (error)
       PbrushOkError(error, MB_ICONEXCLAMATION);

    SetCursor(oldCur);

#ifdef DEBUG
   {
       HWND hWnd;

       /* Force Clipboard to update its window */
       hWnd = FindWindow(TEXT("Clipboard"), NULL);
       if (hWnd) {
           InvalidateRect(hWnd, NULL, FALSE);
           UpdateWindow(hWnd);
       }
   }
#endif

   return(!error);
}

HBITMAP PUBLIC CreatePatternBM(HDC hDC, DWORD color)
{
   HDC patDC;
   HBRUSH brush, hOldBrush;
   HBITMAP patBM = NULL, hOldBM;

   if(!(patDC = CreateCompatibleDC(hDC)))
      goto Error1;
   if(hPalette) {
      SelectPalette(patDC, hPalette, 0);
      RealizePalette(patDC);
   }
   if(!(brush = CreateSolidBrush(color)))
      goto Error2;
   if(!(hOldBrush = SelectObject(patDC, brush)))
      goto Error2a;
   if(!(patBM = CreateBitmap(8, 8, (BYTE)imagePlanes, (BYTE)imagePixels, NULL)))
      goto Error3;
   if(!(hOldBM = SelectObject(patDC, patBM))) {
      DeleteObject(patBM);
      patBM = NULL;
      goto Error3;
   }

   PatBlt(patDC, 0, 0, 8, 8, PATCOPY);

   SelectObject(patDC, hOldBM);
Error3:
   SelectObject(patDC, hOldBrush);
Error2a:
   DeleteObject(brush);
Error2:
   DeleteDC(patDC);
Error1:

   return(patBM);
}

void PUBLIC ConstrainBrush(LPRECT lprBounds, WPARAM wParam, int *dir)
{
   int dx, dy;

   if(wParam & MK_SHIFT) {
      if(!(*dir)) {
         dx = abs(lprBounds->right - lprBounds->left);
         dy = abs(lprBounds->bottom - lprBounds->top);

         if(dx > dy)
            *dir = HORIZdir;
         else if(dy > dx)
            *dir = VERTdir;
      }

      if(*dir == HORIZdir)
         lprBounds->bottom = lprBounds->top;
      else if(*dir == VERTdir)
         lprBounds->right = lprBounds->left;
   } else
      *dir = 0;
}

void PUBLIC ConstrainRect(LPRECT lprBounds, LPRECT lprConst, WPARAM wParam)
{
   int dx, dy;

   if(wParam & MK_SHIFT) {
      dx = abs(lprBounds->right - lprBounds->left);
      dy = abs(lprBounds->bottom - lprBounds->top);

      if((long)horzDotsMM * dy < (long)vertDotsMM * dx) {
         dy = (int)((long)(lprBounds->bottom<lprBounds->top ? -dx : dx)
               *vertDotsMM/horzDotsMM);
         dx = lprBounds->right - lprBounds->left;
      } else {
         dx = (int)((long)(lprBounds->right<lprBounds->left ? -dy : dy)
               *horzDotsMM/vertDotsMM);
         dy = lprBounds->bottom - lprBounds->top;
      }
   } else {
      dx = lprBounds->right - lprBounds->left;
      dy = lprBounds->bottom - lprBounds->top;
   }

   lprBounds->right = lprBounds->left + dx;
   lprBounds->bottom = lprBounds->top + dy;


   NormalizeRect(lprBounds);

   if(lprConst)
      IntersectRect(lprBounds, lprBounds, lprConst);
}

int PUBLIC DoDialog(int id, HWND hwnd, WNDPROC theProc)
{
   int f = -1;

#ifdef BLOCKONMSG
   EnterMsgMode();
#endif

#ifndef WIN32
   if(!(theProc = MakeProcInstance (theProc, hInst)))
      goto Error1;
   f = DialogBox (hInst, (LPTSTR) MAKEINTRESOURCE(id), hwnd, (WNDPROC)theProc);
   FreeProcInstance (theProc);
#else
   f = DialogBox (hInst, (LPTSTR) MAKEINTRESOURCE(id), hwnd, theProc);
#endif

//Error1:
   return(f);
}

void PUBLIC CenterWindow(HWND hWnd)
{
   RECT deskRect, winRect;
   POINT pos;

   GetWindowRect(GetDesktopWindow(), &deskRect);
   GetWindowRect(hWnd, &winRect);

   pos.x = (deskRect.right - deskRect.left - winRect.right + winRect.left) >> 1;
   pos.y = (deskRect.bottom - deskRect.top - winRect.bottom + winRect.top) >> 1;
   SetWindowPos(hWnd, NULL, pos.x, pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

#ifdef WIN16
int PUBLIC changeDiskDir(LPTSTR newDir)
{
   TCHAR c;
   TCHAR szDir[4];
   int nRet;

   if (newDir[1] == TEXT(':'))      /* select new drive   */
   {
      c = newDir[0];
      if (_istlower(c))
         c = _totupper(c);
      SetCurrentDrive (c - TEXT('A'));
   }

   /* To avoid overwriting stack on newDir larger than szDir. */
   if (lstrlen(newDir) == 2) {
        lstrcpy(szDir, newDir);
        lstrcat(szDir, TEXT("\\"));
        return _chdir(szDir);  /* change to correct directory */
   } else {
        AnsiToOem (newDir, newDir);
        nRet =  _chdir(newDir);
        OemToAnsi(newDir, newDir);
        return (nRet);
   }
}
#endif

BOOL PUBLIC bFileExists(LPTSTR lpFilename)
{
   HANDLE  fh;
   WIN32_FIND_DATA  FileInfo;

   fh = FindFirstFile(lpFilename, &FileInfo);
   FindClose (fh);

   return (fh != INVALID_HANDLE_VALUE);
}
#if 0 // 15-Sep-1993 JonPa - this functions is never called
BOOL PUBLIC bValidFilename(LPTSTR lpFilename)
{
   HANDLE  fh;
   WIN32_FIND_DATA  FileInfo;

   fh = FindFirstFile(lpFilename, &FileInfo);
   FindClose (fh);

   return (fh != INVALID_HANDLE_VALUE);
}
#endif

/* ** Enable ok button in a dialog box if and only if edit item
      contains text.  Edit item must have id of idEditSave */
void PUBLIC DlgCheckOkEnable(HWND hwnd, int idEdit, UINT message)
{
   if (message == EN_CHANGE)
       EnableWindow(GetDlgItem(hwnd, IDOK),
                    (BOOL) (SendMessage(GetDlgItem(hwnd, idEdit),
                                        WM_GETTEXTLENGTH, 0, 0L)));
}

void PUBLIC MakeValidFilename(LPTSTR s, LPTSTR lpszDefExtension)
{
    LPTSTR pszSrc;
    LPTSTR pszDst;
    LPTSTR pszDot;
    LPTSTR pszFileName;
    LPTSTR pszLastNonWhite;

    /*
     * Strip out '"'s and find the last
     * '.' and '\'.
     */
    pszLastNonWhite = pszFileName = pszDot = pszDst = s;
    for( pszSrc = s; *pszSrc; pszSrc++ ) {
        if (*pszSrc != TEXT('"') ) {
            *pszDst = *pszSrc;

            if (IsPathSep(*pszDst))
                pszFileName = pszDst;

            if (*pszDst == TEXT('.'))
                pszDot = pszDst;

            if (*pszDst != TEXT(' ') && *pszDst != TEXT('\t'))
                pszLastNonWhite = pszDst;

            pszDst++;
        }
    }

    pszDst = pszLastNonWhite+1;
    *pszDst = TEXT('\0');

    if (IsPathSep(*pszFileName))
        pszFileName++;

    /*
     * There was not a '.' in the filename, append
     * the default extention to it.
     */
    if (*pszDot != TEXT('.') || pszDot < pszFileName) {
        lstrcpy( pszDst, lpszDefExtension);
    }
}

BOOL PUBLIC IsReadOnly(LPTSTR lpFilename)
{
   HANDLE  fh;

   fh = MyOpenFile(lpFilename, NULL, 1);
   if (fh != INVALID_HANDLE_VALUE)
   {
      MyCloseFile(fh);
      return FALSE;
   }

   fh = MyOpenFile(lpFilename, NULL, 0);
   if (fh != INVALID_HANDLE_VALUE)
       MyCloseFile(fh);

   return (fh != INVALID_HANDLE_VALUE);
}

/*  How big is the palette? if bits per pel not 24
**  no of bytes to read is 6 for 1 bit, 48 for 4 bits
**  256*3 for 8 bits and 0 for 24 bits
*/
WORD PUBLIC PaletteSize(VOID FAR * pv)
{
   #define lpbi ((LPBITMAPINFOHEADER) pv)

   WORD    NumColors;

   NumColors = DibNumColors(lpbi);

   if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
       return (WORD)(NumColors * sizeof(RGBTRIPLE));
   else
       return (WORD)(NumColors * sizeof(RGBQUAD));

   #undef lpbi
   #undef lpbc
}

/*
**  CreateDibPalette()
**
**  Given a Global HANDLE to a BITMAPINFO Struct
**  will create a GDI palette object from the color table.
**
**  NOTES only works with "new" format DIB's
**
*/
HPALETTE PUBLIC CreateDibPalette(HANDLE hbi)
{
   HPALETTE hpal;

   if (!hbi)
       return NULL;

   hpal = CreateBIPalette((LPBITMAPINFOHEADER) GlobalLock(hbi));
   GlobalUnlock(hbi);
   return hpal;
}

/*
**  CreateBIPalette()
**
**  Given a Pointer to a BITMAPINFO struct will create a
**  a GDI palette object from the color table.
**
**  NOTES only works with "new" format DIB's
**
*/
HPALETTE PUBLIC CreateBIPalette(LPBITMAPINFOHEADER lpbi)
{
   LOGPALETTE          *pPal;
   HPALETTE            hpal = NULL;
   WORD                nNumColors;
   BYTE                red;
   BYTE                green;
   BYTE                blue;
   WORD                i;
   RGBQUAD        FAR *pRgb;
   RGBQUAD        FAR *pColor;

   if (!lpbi)
       return NULL;

   if (lpbi->biSize != sizeof(BITMAPINFOHEADER))
       goto exit;

   pRgb = (RGBQUAD FAR *) ((LPBYTE) lpbi + (WORD) lpbi->biSize);
   pColor = (RGBQUAD FAR *) rgbColor;
   nNumColors = DibNumColors(lpbi);

   if (nNumColors) {
        // BUG 8035: make sure user can't write invalid colors into the DIB
        // by adding all of the rgbColor[] entries (the available brush
        // colors) to the logical palette we "receive" from the DIB.
        //

       pPal = (LOGPALETTE *) LocalAlloc(LPTR, sizeof(LOGPALETTE) +
                (nNumColors+MAXcolors) * sizeof(PALETTEENTRY));

       if (!pPal)
           goto exit;

       pPal->palNumEntries = nNumColors + MAXcolors;
       pPal->palVersion    = PALVERSION;

       for (i = 0; i < nNumColors; i++) {
           pPal->palPalEntry[i].peRed   = pRgb[i].rgbRed;
           pPal->palPalEntry[i].peGreen = pRgb[i].rgbGreen;
           pPal->palPalEntry[i].peBlue  = pRgb[i].rgbBlue;
           pPal->palPalEntry[i].peFlags = (BYTE) 0;
       }

       for (i = 0; i < MAXcolors; i++) {
           pPal->palPalEntry[i+nNumColors].peRed   = pColor[i].rgbRed;
           pPal->palPalEntry[i+nNumColors].peGreen = pColor[i].rgbGreen;
           pPal->palPalEntry[i+nNumColors].peBlue  = pColor[i].rgbBlue;
           pPal->palPalEntry[i+nNumColors].peFlags = (BYTE) 0;
       }

       hpal = CreatePalette(pPal);
       LocalFree((HANDLE)pPal);
   } else if (lpbi->biBitCount == 24) {
       nNumColors = MAXPALETTE;
       pPal = (LOGPALETTE *) LocalAlloc(LPTR, sizeof(LOGPALETTE) +
                                        nNumColors * sizeof(PALETTEENTRY));

       if (!pPal)
           goto exit;

       pPal->palNumEntries = nNumColors;
       pPal->palVersion    = PALVERSION;

       i = 0;
       for (red = 0; red < 8; ++red)
           for (green = 0; green < 8; ++green)
               for (blue = 0; blue < 4; ++blue) {
           pPal->palPalEntry[i].peRed   = byColorTable[red];
           pPal->palPalEntry[i].peGreen = byColorTable[green];
           pPal->palPalEntry[i].peBlue  = byColorTable[bluetable[blue]];
           pPal->palPalEntry[i].peFlags = (BYTE) 0;
           ++i;
       }

       hpal = CreatePalette(pPal);
       LocalFree((HANDLE)pPal);
   }

exit:
   return hpal;
}

/*  How Many colors does this DIB have?
**  this will work on both PM and Windows bitmap info structures.
*/
WORD PUBLIC DibNumColors(VOID FAR *pv)
{
   #define lpbi ((LPBITMAPINFOHEADER) pv)
   #define lpbc ((LPBITMAPCOREHEADER) pv)

   int bits;

   /*
   **  with the new format headers, the size of the palette is in biClrUsed
   **  else is dependent on bits per pixel
   */
   if (lpbi->biSize != sizeof(BITMAPCOREHEADER)) {
       if (lpbi->biClrUsed != 0)
           return (WORD) lpbi->biClrUsed;

       bits = lpbi->biBitCount;
   } else
       bits = lpbc->bcBitCount;

   switch (bits) {
       case 1:
           return 2;

       case 4:
           return 16;

       case 8:
           return 256;

       default:
           return 0;
   }

   #undef lpbi
   #undef lpbc
}

void PUBLIC TripleToQuad(LPBITMAPINFO lpDIBinfo, BOOL bSwap)
{
   BYTE FAR *lpNewRGB, FAR *lpOldRGB, FAR *lpSentinel;
   DWORD dwNumColors;
   BYTE t;

   if (!(dwNumColors = lpDIBinfo->bmiHeader.biClrUsed))
      dwNumColors = (1L << lpDIBinfo->bmiHeader.biBitCount);

   /* need to convert 24-bit palette to 32-bit palette */
   lpOldRGB = lpNewRGB = lpSentinel = (BYTE FAR *) lpDIBinfo->bmiColors;
   lpOldRGB += 3 * dwNumColors - 1;
   lpNewRGB += sizeof(RGBQUAD) * dwNumColors - 1;

   while (lpOldRGB > lpSentinel) {
       /* swap assumes order is r g b and needs to be b g r */
       if (bSwap) {
           t = lpOldRGB[0];
           lpOldRGB[0] = lpOldRGB[-2];
           lpOldRGB[-2] = t;
       }

       *lpNewRGB-- = 0;
       *lpNewRGB-- = *lpOldRGB--;
       *lpNewRGB-- = *lpOldRGB--;
       *lpNewRGB-- = *lpOldRGB--;
   }
}

HPALETTE PUBLIC MakeImagePalette(HPALETTE hPal, HANDLE hDIBinfo, UINT FAR *wUsage)
{
   HPALETTE        hNewPal;
   LPBITMAPINFO    lpDIBinfo;
   DWORD           dwNumColors;
   BYTE            FAR *bp;
   WORD            FAR *wp;
   DWORD           FAR *dp;
   WORD             i;

   *wUsage = DIB_RGB_COLORS;

   lpDIBinfo = (LPBITMAPINFO) GlobalLock(hDIBinfo);

   if (!(dwNumColors = lpDIBinfo->bmiHeader.biClrUsed))
      dwNumColors = (1L << lpDIBinfo->bmiHeader.biBitCount);

   /* 24 bit image w/o palette so use RGB color translation */
   if (dwNumColors > 256L) {
       hNewPal = hPal ? hPal : CreateDibPalette(hDIBinfo);
       goto cleanup1;
   }

   if (!hPal) {
       /* no palette so create it ... */
       hNewPal = CreateDibPalette(hDIBinfo);
       if (hNewPal) {
           for (i = 0, wp = (WORD FAR *) lpDIBinfo->bmiColors;
               i < (WORD) dwNumColors;)
               *wp++ = i++;
       } else
           goto cleanup1;
   } else {
       /* get closest match for each color /w respect to current palette */
       for (i = 0, wp = (WORD FAR *) lpDIBinfo->bmiColors,
                   dp = (DWORD FAR *) lpDIBinfo->bmiColors;
           i < (WORD) dwNumColors; ++i) {
           bp = (BYTE FAR *) dp;
           SWAP(bp[0],bp[2]);
           *wp++ = (WORD)GetNearestPaletteIndex(hPal, *dp++);
       }
       hNewPal = hPal;
   }

   *wUsage = DIB_PAL_COLORS;

cleanup1:
   GlobalUnlock(hDIBinfo);
   return hNewPal;
}

void PUBLIC NormalizeRect(LPRECT lpRect)
{
   int t;

   if (lpRect->left > lpRect->right) {
       t = lpRect->left;
       lpRect->left = lpRect->right;
       lpRect->right = t;
   }

   if (lpRect->top > lpRect->bottom) {
       t = lpRect->top;
       lpRect->top = lpRect->bottom;
       lpRect->bottom = t;
   }
}

/* copy hbm to hDC, using hbmMask as a mask.
** if mask bit = 0 then leave destination unchanged
** if mask bit = 1 then copy from source
** if hbmMask is NULL, this becomes a straight BitBlt (all bits 1)
*/
BOOL PUBLIC MaskStretchBlt(HDC hDCD, int x, int y, int dx, int dy,
                           HDC hDCS, HDC hDCSMask, int x0, int y0,
                           int dx0, int dy0)
{
   DWORD rgbBk, rgbFg;

   if (!hDCSMask)
       return StretchBlt(hDCD, x, y, dx, dy, hDCS, x0, y0, dx0, dy0, SRCCOPY);

   rgbBk = SetBkColor(hDCD, RGB(255,255,255));
   rgbFg = SetTextColor(hDCD, RGB(0,0,0));

   StretchBlt(hDCD, x, y, dx, dy, hDCS, x0, y0, dx0, dy0, DSx);
   StretchBlt(hDCD, x, y, dx, dy, hDCSMask, x0, y0, dx0, dy0, DSna);
   StretchBlt(hDCD, x, y, dx, dy, hDCS, x0, y0, dx0, dy0, DSx);

   SetBkColor(hDCD, rgbBk);
   SetTextColor(hDCD, rgbFg);

   return TRUE;
}

void PUBLIC EnableMenuItems(HMENU hMenu, UINT MenuItems[], UINT wFlags)
{
   while(*MenuItems != -1) {
      EnableMenuItem(hMenu, *MenuItems++, wFlags);
   }
}
