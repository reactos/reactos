/****************************Module*Header******************************\
* Module Name: saveimg.c                                                *
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

#include "onlypbr.h"

#undef  NOCOLOR
#undef  NOGDICAPMASKS
#undef  NOKERNEL
#undef  NOMEMMGR
#undef  NOLSTRING
#undef  NOOPENFILE
#undef  NOMB
#undef  NORASTEROPS
#undef  NOLFILEIO


#include <windows.h>
#include <port1632.h>
#include <memory.h>

//#define NOEXTERN
#include "pbrush.h"

#define BiToByAl(bi,al) (((bi)+(al)*8-1)/((al)*8)*(al))

extern int imagePlanes, imagePixels;
extern HPALETTE hPalette;
extern HDC fileDC;
extern TCHAR fileName[];
extern HBITMAP fileBitmap;
extern HWND pbrushWnd[];
extern int fileByteWid;
extern LPBYTE fileBuff;
extern BOOL bZoomedOut, gfDirty;

void ChunkyToPlanar(LPBYTE lpDIBits, LPBYTE lpRedBuf, LPBYTE lpGreenBuf,
                    LPBYTE lpBlueBuf, LPBYTE lpIntBuf, WORD wScanSize)
{
   WORD i, j;
   BYTE m, red, green, blue, intensity, chunky;

   for (i = 0; i < wScanSize; i += 4) {
       red = green = blue = intensity = 0;
       m = 0x80;

       /* decode 8 pixels worth */
       for (j = 0; j < 4; ++j) {
           chunky = *lpDIBits++;

           /* add in appropriate bits from upper nibble */
           if (chunky & 0x80) intensity |= m;
           if (chunky & 0x40) blue |= m;
           if (chunky & 0x20) green |= m;
           if (chunky & 0x10) red |= m;
           m >>= 1;

           /* add in appropriate bits from lower nibble */
           if (chunky & 0x08) intensity |= m;
           if (chunky & 0x04) blue |= m;
           if (chunky & 0x02) green |= m;
           if (chunky & 0x01) red |= m;
           m >>= 1;
       }

       /* copy results to planar buffers */
       *lpRedBuf++ = red;
       *lpGreenBuf++ = green;
       *lpBlueBuf++ = blue;
       *lpIntBuf++ = intensity;
   }
}


BOOL SaveImg(HWND hWnd, int xoff, int yoff, int width, int height,
             int bytewid, HDC srcDC)
{
   int             i, bpp;
   BOOL            error = TRUE;
   WORD            errmsg;
   DHDR            hdr;
   HCURSOR         oldCsr;
   HBITMAP         hTempBit, hOldBM;
   HDC             parentDC;
   HANDLE          t;
   LPBITMAPINFO    lpDIBinfo;
   LPBYTE          lpDIBits;
   BYTE FAR       *lpDst, FAR *lpSrc;
   HANDLE          hDIBinfo, hDIBits;
   WORD            wScanSize;
   DWORD           fError;
   TCHAR           fileUNCPath[MAX_PATH];

   oldCsr = SetCursor(LoadCursor(NULL, IDC_WAIT));

   /* compute the bits per pel */
   bpp = imagePlanes * imagePixels;

   if (bpp > 8) {
        /* convert anything more than 8bpp to 24 bits */
        bpp = 24;
   } else if (bpp == 3) {
        /* convert CGA to VGA */
       bpp = 4;
   }

   if (!AllocTemp(width, FILEBUFFrows, imagePlanes, imagePixels, (bpp == 24))){
       errmsg = IDSNotEnufMem;
       goto error1;
   }

   /* realize palette if there is one */
   if (hPalette) {
       SelectPalette(fileDC, hPalette, 0);
       RealizePalette(fileDC);
   }

   if (imagePlanes == 4)
       bytewid *= 4;

   GetCurrentDirectory (CharSizeOf(fileUNCPath), fileUNCPath);
   if (fileUNCPath[lstrlen (fileUNCPath) - 1] != TEXT('\\'))
       lstrcat (fileUNCPath, TEXT("\\"));
   lstrcat (fileUNCPath, fileName);
   if ((t = MyOpenFile(fileUNCPath, NULL, OF_CREATE | OF_WRITE)) == INVALID_HANDLE_VALUE)
   {
       fError = GetLastError ();
       if (fError == 0x04) /* out of handles */
          errmsg = IDSCantCreate;
/* FIX BUG #11299 - EDH - 17 SEP 91 */
       else if (fError == 0x05 || fError == 0x13) /* file, disk RO */
       {
          errmsg = IDSReadOnly;
          MenuCmd(hWnd, FILEsaveas);
       }
/* END FIX - EDH - SEE ALSO SAVEBIT.C */
       else
          errmsg = IDSCantOpen;
       goto error2;
   }

   hTempBit = CreateBitmap(1, 1, 1, 1, NULL);
   if (!hTempBit) {
       errmsg = IDSNotEnufMem;
       goto error2;
   }

   /* allocate DIB header */
   hDIBinfo = GlobalAlloc(GHND, sizeof(BITMAPINFOHEADER)
                    + ((bpp <= 8) ? ((1L << bpp) * sizeof(RGBQUAD)) : 0));
   if (!hDIBinfo) {
       errmsg = IDSNotEnufMem;
       goto error2a;
   }
   lpDIBinfo = (LPBITMAPINFO) GlobalLock(hDIBinfo);

   /* allocate memory for DIBits buffer (note: this is REALLY only needed
   ** for 4 plane images (so they can be converted to planar format) but
   ** it is easier to code if the buffer is always used.  Next rev: do it
   ** right.
   */
   wScanSize = (WORD)BiToByAl(width * bpp, 4);

   hDIBits = GlobalAlloc(GHND, (DWORD) wScanSize);
   if (!hDIBits) {
       errmsg = IDSNotEnufMem;
       goto error2b;
   }
   lpDIBits = GlobalLock(hDIBits);

   /* make DIB header */
   lpDIBinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   lpDIBinfo->bmiHeader.biWidth = width;
   lpDIBinfo->bmiHeader.biHeight = 1;
   lpDIBinfo->bmiHeader.biPlanes = 1;
   lpDIBinfo->bmiHeader.biBitCount = (WORD)bpp;
   lpDIBinfo->bmiHeader.biSizeImage = 0;
   lpDIBinfo->bmiHeader.biCompression = 0;
   lpDIBinfo->bmiHeader.biXPelsPerMeter = 0;
   lpDIBinfo->bmiHeader.biYPelsPerMeter = 0;
   lpDIBinfo->bmiHeader.biClrUsed = 0;
   lpDIBinfo->bmiHeader.biClrImportant = 0;

   /* Get color table */
   hOldBM = SelectObject(fileDC, hTempBit);
   GetDIBits(fileDC, fileBitmap, 0, 1, NULL, lpDIBinfo, DIB_RGB_COLORS);
   if (hOldBM)
       SelectObject(fileDC, hOldBM);

   /* fill in image header */
   hdr.manuf = 10;
   hdr.hard = (BYTE)((bpp == 24 || hPalette ||
         (GetDeviceCaps(fileDC, RASTERCAPS) & RC_PALETTE)) ? 5 : 3);
   hdr.encod = 1;
   parentDC = GetDisplayDC(pbrushWnd[PARENTid]);
   hdr.bitpx = (BYTE) ((bpp == 4) ? 1 : bpp);
   hdr.x1 = hdr.y1 = 0;
   hdr.x2 = width - 1;
   hdr.y2 = height - 1;
   hdr.hRes = GetDeviceCaps(parentDC, HORZRES);
   hdr.vRes = GetDeviceCaps(parentDC, VERTRES);
   ReleaseDC(pbrushWnd[PARENTid], parentDC);
   hdr.vMode = 0;

   if (bpp == 4) {
       hdr.nPlanes = (BYTE) 4;
       hdr.bplin = fileByteWid / 4;
   } else if (bpp == 24) {
       hdr.nPlanes = 3;
       hdr.bitpx = 8;
       hdr.bplin = fileByteWid;
   } else {
       hdr.nPlanes = (BYTE) 1;
       hdr.bplin = fileByteWid;
   }

   /* If there are at most 16 colors place them in header */
   if (hdr.hard == 5 && hdr.bitpx < 8) {
       lpDst = hdr.clrma;
       lpSrc = (BYTE FAR *) lpDIBinfo->bmiColors;
       for (i = (1 << bpp); i; --i) {
           *lpDst++ = lpSrc[2];    /* swap RED and BLUE components */
           *lpDst++ = lpSrc[1];
           *lpDst++ = lpSrc[0];    /* swap RED and BLUE components */
           lpSrc += 4;
       }
   }

   if (!MyByteWriteFile(t, &hdr, sizeof(DHDR))) {
       errmsg = IDSHdrSave;
       goto error3;
   }

   for (i = yoff; i < (height + yoff); ++i) {
       if (srcDC)
           BitBlt(fileDC, 0, 0, width, 1, srcDC, xoff, i, SRCCOPY);
       else
           BitBlt(fileDC, 0, 0, width, 1, hdcWork, xoff, i, SRCCOPY);

       hOldBM = SelectObject(fileDC, hTempBit);
       GetDIBits(fileDC, fileBitmap, 0, 1, lpDIBits, lpDIBinfo,
                 DIB_RGB_COLORS);
       if (hOldBM)
           SelectObject(fileDC, hOldBM);

       /* if saving 4 plane file convert buffer to planar format first */
       /* if hdr.hard==5 (if there is a palette) then we have palette
          entries, and the RED and BLUE planes should NOT be switched */
       if (hdr.nPlanes == 4)
           ChunkyToPlanar(lpDIBits,
                 fileBuff + (hdr.hard==5 ? 0 : 2) * hdr.bplin,
                 fileBuff + hdr.bplin,
                 fileBuff + (hdr.hard==5 ? 2 : 0) * hdr.bplin,
                 fileBuff + 3 * hdr.bplin,
                 wScanSize);
       else if (bpp == 24) {
           /*
            * In a 24 bpp DIB, the colors are returned as BGR tripplets.
            * We need to reverse the order and split them into planes
            * for .PCX format.
            */
           LPBYTE pSrc, pRed, pBlue, pGreen;
           DWORD cbPlane;

           // Compute the # of bytes needed for one color plane
           cbPlane = bpp * width / 8 / 3;

           pSrc = lpDIBits;
           pRed = fileBuff;
           pGreen = pRed + fileByteWid;
           pBlue = pGreen + fileByteWid;

           for( ; cbPlane > 0; cbPlane-- ) {
                *pBlue++ = *pSrc++;
                *pGreen++ = *pSrc++;
                *pRed++ = *pSrc++;
           }

           //zero fill unused pad area at end of line.
           while (pBlue < (fileBuff + wScanSize)) {
                *pBlue++ = 0;
           }

       } else
           RepeatMove(fileBuff, lpDIBits, wScanSize);


       if (!PackBuff(fileBuff, 0, bytewid, t)) {
           errmsg = IDSUnableSave;
           goto error3;
       }
   }

   /* if we are writing 256 color PCX image write palette */
   if (hdr.hard == 5 && (hdr.bitpx * hdr.nPlanes) == 8) {
       /* convert palette to RGB triple form */
       hOldBM = SelectObject(fileDC, hTempBit);
       GetDIBits(fileDC, fileBitmap, 0, 1, NULL, lpDIBinfo, DIB_RGB_COLORS);
       if (hOldBM)
           SelectObject(fileDC, hOldBM);
       lpSrc = (BYTE FAR *) (lpDIBinfo->bmiColors + 255);
       lpDst = lpSrc + 3;
       while (lpSrc >= (BYTE FAR *) lpDIBinfo->bmiColors) {
           lpDst[0] = lpSrc[0]; /* blue component */
           i = lpSrc[1];        /* save green component */
           lpDst[-2] = lpSrc[2];   /* red component */
           lpDst[-1] = (BYTE) i;   /* green component */
           lpSrc -= 4;
           lpDst -= 3;
       }

       /* hack: place ID byte (12) in first byte before palette */
       *lpDst = 12;

       /* write out ID byte and palette */
       if (!MyByteWriteFile(t, lpDst, 769))
       {
           errmsg = IDSUnableSave;
           goto error3;
       }
   }

   error = FALSE;

error3:
   GlobalUnlock(hDIBits);
   GlobalFree(hDIBits);

   MyCloseFile(t);
   if (error)
       DeleteFile(fileName);

error2b:
   GlobalUnlock(hDIBinfo);
   GlobalFree(hDIBinfo);

error2a:
   DeleteObject(hTempBit);

error2:
   FreeTemp();

error1:
   SetCursor(oldCsr);
   if (error)
       SimpleMessage(errmsg, fileName, MB_OK | MB_ICONEXCLAMATION);
   if (!error && !bZoomedOut && !srcDC) {
       gfDirty = FALSE;
       SetTitle(fileName);
       return TRUE;
   } else
       return !error;
}
