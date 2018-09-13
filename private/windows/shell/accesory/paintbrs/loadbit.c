/****************************Module*Header******************************\
* Module Name: loadbit.c                                                *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
*                                                                       *
* A general description of how the module is used goes here.            *
*                                                                       *
* Additional information such as restrictions, limitations, or special  *
* algorithms used if they are externally visible or effect proper use   *
* of the module.                                                        *
\***********************************************************************/

#define    NOVIRTUALKEYCODES
#define    NOKANJI
#define    NOWH
#define    NOCOMM
#define    NOSOUND
#define    NOSCROLL
#define    NOICONS
#include <windows.h>
#include <stdio.h>
#include "port1632.h"
#include "pbrush.h"


#define DIBID             0x4D42
#define SBITMAPFILEHEADER 14

extern int DlgCaptionNo;
extern int pickWid, pickHgt, defaultWid, defaultHgt;
extern HWND pbrushWnd[];
extern HPALETTE hPalette;
extern int imagePlanes, imagePixels;
extern HDC pickDC;
extern WORD wFileType;
extern BOOL imageFlag;
extern TCHAR filePath[];

int LoadBitmapFile(HWND hWnd, LPTSTR lpFilename, LPBITMAPINFO lpInfoHeader)
{
   BITMAPFILEHEADER    hdr;
   BITMAPINFO          hdrInfo;
   O_BITMAPINFO        O_hdrInfo;
   LPBITMAPINFO        lpDIBinfo;
   HANDLE              fh;
   BOOL                error = TRUE;
   int                 wplanes, wbitpx, i;
   HANDLE              hDIBbits = NULL, hDIBinfo = NULL;
   HDC                 hdc = NULL, parentdc = NULL;
   LPBYTE              lpDIBbits = NULL;
   HBITMAP             hbitmap = NULL, htempbit = NULL;
   DWORD               dwSize, dwHdrSize, dwRGBsize, dwNumColors;
   HCURSOR             oldCsr;
   int                 alloc;
   WORD                errmsg;
   UINT                wUsage;
   UINT                wSize;
   HPALETTE            hNewPal = NULL;
   LONG                lCurPos;
   int                 ht;
   int                 height;


   if ((fh = MyOpenFile(lpFilename, NULL, OF_READ | OF_SHARE_DENY_WRITE)) == INVALID_HANDLE_VALUE)
   {
       errmsg = IDSCantOpen;
       goto error1;
   }

   if (!MyByteReadFile(fh, &hdr, SBITMAPFILEHEADER))
   {
       errmsg = IDSUnableHdr;
       goto error2;
   }

   lCurPos = MyFileSeek(fh, 0L, 1);

   /* read in size field to determine if we are reading old or new DIB */
   if (!MyByteReadFile(fh, (LPBYTE) &dwHdrSize, sizeof(DWORD)))
   {
       errmsg = IDSUnableHdr;
       goto error2;
   }

   MyFileSeek(fh, lCurPos, 0);

   /* call the appropriate routine to fill in the header */
   switch (LOWORD(dwHdrSize)) {
       case sizeof(BITMAPINFOHEADER):
           wSize = sizeof(BITMAPINFOHEADER);
           if (!MyByteReadFile(fh, &hdrInfo, wSize))
           {
               errmsg = IDSUnableHdr;
               goto error2;
           }
           dwRGBsize = sizeof(RGBQUAD);
           break;

       case sizeof(O_BITMAPCOREHEADER):
           wSize = sizeof(O_BITMAPCOREHEADER);
           if (!MyByteReadFile(fh, &O_hdrInfo, wSize))
           {
               errmsg = IDSUnableHdr;
               goto error2;
           }

           /* fill in new bitmap header fields by hand... */
           hdrInfo.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
           hdrInfo.bmiHeader.biWidth         = O_hdrInfo.bmciHeader.bcWidth;
           hdrInfo.bmiHeader.biHeight        = O_hdrInfo.bmciHeader.bcHeight;
           hdrInfo.bmiHeader.biPlanes        = O_hdrInfo.bmciHeader.bcPlanes;
           hdrInfo.bmiHeader.biBitCount      = O_hdrInfo.bmciHeader.bcBitCount;
           hdrInfo.bmiHeader.biCompression   = 0;
           hdrInfo.bmiHeader.biSizeImage     = 0;
           hdrInfo.bmiHeader.biXPelsPerMeter = 0;
           hdrInfo.bmiHeader.biYPelsPerMeter = 0;
           hdrInfo.bmiHeader.biClrUsed       = 0;
           hdrInfo.bmiHeader.biClrImportant  = 0;

           dwRGBsize = sizeof(RGBTRIPLE);
           break;

       default:
           errmsg = IDSUnableHdr;
           goto error2;
   }

   if (hdrInfo.bmiHeader.biPlanes != 1 || hdr.bfType != DIBID) {
       errmsg = IDSUnableHdr;
       goto error2;
   }

   if (hdrInfo.bmiHeader.biCompression) {
       errmsg = IDSUnknownFmt;
       goto error2;
   }

   if (DlgCaptionNo == PASTEFROM) {
      pickWid = LOWORD(hdrInfo.bmiHeader.biWidth);
      pickHgt = LOWORD(hdrInfo.bmiHeader.biHeight);
   }

   /* if lpInfoHeader != NULL then we are just supposed to read header */
   if (lpInfoHeader)
   {
      *lpInfoHeader = hdrInfo;
      MyCloseFile(fh);
      return 0;
   }

   if (!(dwNumColors = hdrInfo.bmiHeader.biClrUsed))
      if (hdrInfo.bmiHeader.biBitCount != 24)
         dwNumColors = (1L << hdrInfo.bmiHeader.biBitCount);

   if (!(parentdc = GetDisplayDC(hWnd))) {
       errmsg = IDSCantAlloc;
       goto error2;
   }

   if (hdrInfo.bmiHeader.biBitCount != 1) {
       wplanes = GetDeviceCaps(parentdc, PLANES);
       wbitpx = GetDeviceCaps(parentdc, BITSPIXEL);
   } else {
       wplanes = 1;
       wbitpx = 1;
   }

   oldCsr = SetCursor(LoadCursor(NULL, IDC_WAIT));

   if (DlgCaptionNo == PASTEFROM) {
       if (!(alloc = AlocPick(parentdc))) {
           errmsg = IDSCantAlloc;
           goto error3;
       }
   } else if (errmsg = AllocImg(LOWORD(abs( hdrInfo.bmiHeader.biWidth ) ),
                        LOWORD(abs( hdrInfo.bmiHeader.biHeight ) ),
                        wplanes, wbitpx, FALSE))
       goto error3;

   dwSize = sizeof(BITMAPINFOHEADER) + dwNumColors * sizeof(RGBQUAD);

   if (!(hDIBinfo = GlobalAlloc(GMEM_MOVEABLE, dwSize))) {
       errmsg = IDSCantAlloc;
       goto error3;
   }

   if (!(lpDIBinfo = (LPBITMAPINFO) GlobalLock(hDIBinfo))) {
       errmsg = IDSCantAlloc;
       goto error4;
   }

   /* copy header into allocated memory */
   *lpDIBinfo = hdrInfo;

   /* read in the palette if not 24 bits */
   if (dwNumColors) {
       wSize = LOWORD(dwRGBsize * dwNumColors);
       if (!MyByteReadFile(fh, lpDIBinfo->bmiColors, wSize))
       {
           errmsg = IDSUnablePalette;
           goto error5;
       }

       if (dwRGBsize == sizeof(RGBTRIPLE)) {
           TripleToQuad(lpDIBinfo, FALSE);
       }
   }

   hNewPal = MakeImagePalette(hPalette, hDIBinfo, &wUsage);
   if (hNewPal && hNewPal != hPalette) {
       SelectPalette(hdcWork, hNewPal, 0);
       RealizePalette(hdcWork);
       SelectPalette(hdcImage, hNewPal, 0);
       RealizePalette(hdcImage);

       hPalette = hNewPal;
   }

   hdc = CreateCompatibleDC((DlgCaptionNo == PASTEFROM) ? pickDC : parentdc);
   ReleaseDC(hWnd, parentdc);
   parentdc = NULL;

   if (!hdc) {
       errmsg = IDSNoMemAvail;
       goto error7;
   }
   wSize = (WORD)(((hdrInfo.bmiHeader.biWidth *
                hdrInfo.bmiHeader.biBitCount + 31) & (~31)) / 8);

   height = abs( (int)hdrInfo.bmiHeader.biHeight );
   for (ht = abs( height ), hbitmap = NULL; ht && !hbitmap; ) {
      if (!(hDIBbits = GlobalAlloc(GMEM_MOVEABLE, (DWORD)wSize * (DWORD)ht))
       || !(hbitmap = CreateBitmap(LOWORD(hdrInfo.bmiHeader.biWidth), ht,
                                   (BYTE) imagePlanes, (BYTE) imagePixels,
                                   NULL))) {
        if (hDIBbits)
            GlobalFree(hDIBbits);
        ht = ht >> 1;
      }
   }
   if (!ht) {
       errmsg = IDSNoMemAvail;
       goto error7;
   }

   if (!(lpDIBbits = GlobalLock(hDIBbits))) {
       errmsg = IDSNoMemAvail;
       goto error7;
   }

   if(!(htempbit = SelectObject(hdc, hbitmap))) {
      errmsg = IDSNoMemAvail;
      goto error7;
   }

   if (hPalette) {
      SelectPalette(hdc, hPalette, FALSE);
      RealizePalette(hdc);
   }

   lpDIBinfo->bmiHeader.biHeight = ht;
   if (MyFileSeek(fh, hdr.bfOffBits, 0) == -1) {
       errmsg = IDSBadData;
       goto error7;
   }

   error = FALSE;
   for (i = height; i; i -= ht) {
       if (i < ht)
            lpDIBinfo->bmiHeader.biHeight = ht = i;

       if (!MyByteReadFile(fh, lpDIBbits, (DWORD) wSize*ht)) {
           errmsg = IDSBadData;
           error = TRUE;
           break;
       }

       if(!(hbitmap = SelectObject(hdc, htempbit))) {   /* BUGBUGBUG */
          errmsg = IDSNoMemAvail;
          goto error7;
       }
       if(!SetDIBits(hdc, hbitmap, 0, ht, lpDIBbits, lpDIBinfo, wUsage)) {
           errmsg = IDSNoMemAvail;
           error = TRUE;
           break;
       }

       if(!(htempbit = SelectObject(hdc, hbitmap))) {    /* BUGBUGBUG */
          errmsg = IDSNoMemAvail;
          goto error7;
       }

       if (hdrInfo.bmiHeader.biHeight > 0) {
           if (DlgCaptionNo != PASTEFROM)
               BitBlt(hdcWork, 0, i - ht, (WORD) hdrInfo.bmiHeader.biWidth, ht,
                       hdc, 0, 0, SRCCOPY);
           else
               BitBlt(pickDC, 0, i - ht, (WORD) hdrInfo.bmiHeader.biWidth, ht,
                      hdc, 0, 0, SRCCOPY);
       } else {
           if (DlgCaptionNo != PASTEFROM)
               StretchBlt(hdcWork, 0, i - ht, (WORD) hdrInfo.bmiHeader.biWidth, ht,
                       hdc, 0, ht - 1, (WORD) hdrInfo.bmiHeader.biWidth, -ht, SRCCOPY);
           else
               StretchBlt(pickDC, 0, i - ht, (WORD) hdrInfo.bmiHeader.biWidth, ht,
                      hdc, 0, ht - 1, (WORD) hdrInfo.bmiHeader.biWidth, -ht, SRCCOPY);
       }
   }

   if (DlgCaptionNo != PASTEFROM) {
       BitBlt(hdcImage, 0, 0, LOWORD(hdrInfo.bmiHeader.biWidth),
               LOWORD(hdrInfo.bmiHeader.biHeight),
               hdcWork, 0, 0, SRCCOPY);

       wFileType = GetImageFileType(hdrInfo.bmiHeader.biBitCount);

       if (error)
           ClearImg();
   }
   imageFlag = FALSE;

   if (!error)
       GetCurrentDirectory(PATHlen, filePath);

error7:

    if (hdc)
    {
        if (htempbit)
            SelectObject(hdc, htempbit);

        DeleteDC(hdc);
    }

    if (hbitmap)
        DeleteObject(hbitmap);

    if (lpDIBbits)
        GlobalUnlock(hDIBbits);

    if (hDIBbits)
        GlobalFree(hDIBbits);

error5:
    if (lpDIBinfo)
        GlobalUnlock(hDIBinfo);

error4:
    if (hDIBinfo)
        GlobalFree(hDIBinfo);

error3:
    if (error && DlgCaptionNo == PASTEFROM)
        FreePick();

    if (parentdc)
        ReleaseDC(hWnd, parentdc);

    SetCursor(oldCsr);

error2:
   MyCloseFile(fh);

error1:
    return error ? errmsg : 0;
}
