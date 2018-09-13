/****************************Module*Header******************************\
* Module Name: savebit.c                                                *
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

#undef  NOATOM
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
#include <stdio.h>
#include "port1632.h"
#include "pbrush.h"
#include "pbserver.h"

#define DIBID      0x4D42

extern HBITMAP fileBitmap;
extern int imagePlanes, imagePixels;
extern HWND pbrushWnd[];
extern HPALETTE hPalette;
extern TCHAR fileName[];
extern WORD wFileType;
extern BOOL bZoomedOut, imageFlag;

BOOL SaveBitmapFile(HWND hWnd, int xoff, int yoff, int width, int height,
                    HDC srcDC)
{
   int                 i;
   BOOL                error = TRUE;
   BITMAPFILEHEADER    hdr;
   BITMAPINFO FAR     *lpinfo = NULL;
   LPBYTE              lpbits = NULL;
   HANDLE              hinfo = NULL, hbits = NULL;
   HCURSOR             oldCsr;
   HDC                 parentDC = NULL;
   HANDLE              filenum;
   int                 infosize;
   DWORD               imgsize;
   HBITMAP             htempbit = NULL;
   HPALETTE            hOldPalette = NULL;
   int                 ht;
   DWORD               tmpSize, fError;
   TCHAR               fileUNCPath[MAX_PATH];

   oldCsr = SetCursor(LoadCursor(NULL,IDC_WAIT));

   if (!(parentDC = GetDisplayDC(hWnd))) {
       PbrushOkError(IDSNotEnufMem, MB_ICONHAND);
       ReleaseDC(hWnd, parentDC);
       goto error1;
   }

   if (!(fileDC = CreateCompatibleDC(parentDC))) {
       PbrushOkError(IDSNotEnufMem, MB_ICONHAND);
       ReleaseDC(hWnd, parentDC);
       goto error1;
   }
   ReleaseDC(hWnd, parentDC);

   SaveDC(fileDC);

   if (hPalette) {
      hOldPalette = SelectPalette(fileDC, hPalette, 0);
      RealizePalette(fileDC);
      if (srcDC) {
         SelectPalette(srcDC, hPalette, 0);
         RealizePalette(srcDC);
      }
   }

   /*
    * The Win32 OpenFile API will search other directories, including
    *   the Windows system directory for the file if only a file name
    *   is specified.  So, in order to fix NT product 1 bug #14602,
    *   we'd better build a full path.
    */
   GetCurrentDirectory (CharSizeOf(fileUNCPath), fileUNCPath);
   if (fileUNCPath[lstrlen(fileUNCPath) - 1] != TEXT('\\'))
       lstrcat (fileUNCPath, TEXT("\\"));
   lstrcat (fileUNCPath, fileName);
   if ((filenum = MyOpenFile(fileUNCPath, NULL, OF_CREATE | OF_WRITE))
       == INVALID_HANDLE_VALUE)
   {
       fError = GetLastError ();
       if (fError == 0x04) /* out of handles */
          SimpleMessage(IDSCantCreate, fileName, MB_OK | MB_ICONEXCLAMATION);
/* FIX BUG #11299 - EDH - 17 SEP 91 */
       else if (fError == 0x05 || fError == 0x13) /* file, disk RO */
       {
          SimpleMessage(IDSReadOnly, fileName, MB_OK | MB_ICONEXCLAMATION);
          MenuCmd(hWnd, FILEsaveas);
       }
/* END FIX - EDH - SEE ALSO SAVEIMG.C */
       else
          SimpleMessage(IDSCantOpen, fileName, MB_OK | MB_ICONEXCLAMATION);
       goto error1;
   }

   if (wFileType == BITMAPFILE || wFileType == MSPFILE) {
       if (!(hinfo = GlobalAlloc(GMEM_MOVEABLE,
               (DWORD) (infosize = (sizeof(BITMAPINFOHEADER)
                                    + 2L * sizeof(RGBQUAD)))))) {
           SimpleMessage(IDSNoMemAvail, fileName, MB_OK | MB_ICONEXCLAMATION);
           goto error3;
       }

       imgsize = (DWORD)(((width + 31) & (-32)) >> 3);

   } else if (wFileType == BITMAPFILE4) {
       if (!(hinfo = GlobalAlloc(GMEM_MOVEABLE,
               (DWORD) (infosize = sizeof(BITMAPINFOHEADER)
                                 + 16L * sizeof(RGBQUAD))))) {
           SimpleMessage(IDSNoMemAvail, fileName, MB_OK | MB_ICONEXCLAMATION);
           goto error3;
       }

       imgsize = (DWORD)((((width * 4 + 7) >> 3) + 3) & ~3);
   } else if (wFileType == BITMAPFILE8) {
       if (!(hinfo = GlobalAlloc(GMEM_MOVEABLE,
               (DWORD) (infosize = sizeof(BITMAPINFOHEADER)
                                 + 256L * sizeof(RGBQUAD))))) {
           SimpleMessage(IDSNoMemAvail, fileName, MB_OK | MB_ICONEXCLAMATION);
           goto error3;
       }

       imgsize = (DWORD) (((width * 8 + 31) & (-32))  >> 3);
   } else if (wFileType == BITMAPFILE24) {
       if (!(hinfo = GlobalAlloc(GMEM_MOVEABLE,
               (DWORD) (infosize = sizeof(BITMAPINFOHEADER))))) {
           SimpleMessage(IDSNoMemAvail, fileName, MB_OK | MB_ICONEXCLAMATION);
           goto error3;
       }

       imgsize = (DWORD)((width * sizeof(RGBTRIPLE) + 3) & (-4));
   }

   for (ht = height, fileBitmap = NULL; ht && !fileBitmap; ) {

      tmpSize = imgsize * (DWORD)height;

      if (!((hbits = GlobalAlloc(GMEM_MOVEABLE, tmpSize))
       && (fileBitmap = CreateBitmap(width, ht, (BYTE) imagePlanes,
                                     (BYTE)imagePixels, NULL)))) {
        if (hbits)
            GlobalFree(hbits);
        ht = ht >> 1;
      }
   }
   if (!ht) {
       PbrushOkError(IDSNotEnufMem, MB_ICONHAND);
       goto error4;
   }

   if (!(lpinfo = (BITMAPINFO FAR *) GlobalLock(hinfo))) {
       SimpleMessage(IDSNoMemAvail, fileName, MB_OK | MB_ICONEXCLAMATION);
       goto error5;
   }

   if (!(lpbits = GlobalLock(hbits))) {
       SimpleMessage(IDSNoMemAvail, fileName, MB_OK | MB_ICONEXCLAMATION);
       goto error6;
   }

   /* fill in image header */
   lpinfo->bmiHeader.biSize   = sizeof(BITMAPINFOHEADER);
   lpinfo->bmiHeader.biWidth  = width;
   lpinfo->bmiHeader.biHeight = 1;
   lpinfo->bmiHeader.biPlanes = 1;

   switch (wFileType) {
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

       case BITMAPFILE24:
           i = 24;
           break;
   }

   lpinfo->bmiHeader.biBitCount      = (WORD)i;
   lpinfo->bmiHeader.biCompression   =
   lpinfo->bmiHeader.biClrUsed       =
   lpinfo->bmiHeader.biClrImportant  = 0;
   lpinfo->bmiHeader.biXPelsPerMeter = 0;
   lpinfo->bmiHeader.biYPelsPerMeter = 0;

   MyFileSeek(filenum, (LONG)(sizeof(BITMAPFILEHEADER) + infosize), 0);
   hdr.bfOffBits = MyFileSeek(filenum, 0L, 1);

   lpinfo->bmiHeader.biHeight = ht;
   for (i = height; i; i -= ht) {
      if (ht > i)
          lpinfo->bmiHeader.biHeight = ht = i;

      if(!(htempbit = SelectObject(fileDC, fileBitmap))) {
         SimpleMessage(IDSUnableSave, fileName, MB_OK | MB_ICONEXCLAMATION);
         goto error7;
      }
      if (srcDC)
          BitBlt(fileDC, 0, 0, width, ht,
                 srcDC, xoff, yoff + (i - ht), SRCCOPY);
      else
          BitBlt(fileDC, 0, 0, width, ht,
                  hdcWork, xoff, yoff + (i - ht), SRCCOPY);

      if (!SelectObject(fileDC, htempbit) ||
          !GetDIBits(fileDC, fileBitmap, 0, ht, lpbits, lpinfo, 0) ||
          !MyByteWriteFile(filenum, lpbits, imgsize * (DWORD)ht))
      {
         SimpleMessage(IDSUnableSave, fileName, MB_OK | MB_ICONEXCLAMATION);
         goto error7;
      }
   }

   hdr.bfSize = MyFileSeek(filenum, 0L, 2);
   hdr.bfType = DIBID;
   hdr.bfReserved1 = hdr.bfReserved2 = 0;

   MyFileSeek(filenum, 0L, 0);
   if (!MyByteWriteFile(filenum, &hdr, sizeof(BITMAPFILEHEADER))) {
       SimpleMessage(IDSUnableSave, fileName, MB_OK | MB_ICONEXCLAMATION);
       goto error7;
   }

   lpinfo->bmiHeader.biSizeImage = 0;
   lpinfo->bmiHeader.biHeight = height;
   if (!MyByteWriteFile(filenum, lpinfo, infosize)) {
       SimpleMessage(IDSUnableSave, fileName, MB_OK | MB_ICONEXCLAMATION);
       goto error7;
   }

   error = FALSE;

error7:
    if (htempbit)
        SelectObject(fileDC, htempbit);

    if (lpbits)
        GlobalUnlock(hbits);

error6:
    if (lpinfo)
        GlobalUnlock(hinfo);

error5:
    if (hbits)
        GlobalFree(hbits);

error4:
    if (hinfo)
        GlobalFree(hinfo);

error3:
    MyCloseFile(filenum);


    if (error)
        DeleteFile(fileName);

error1:
    FreeTemp();                     /* Deletes fileBitmap and fileDC */
    SetCursor(oldCsr);

    if (!error && !bZoomedOut && !srcDC) {
        imageFlag = FALSE;
        SetTitle(fileName);
        return TRUE;
    } else
        return !error;
}
