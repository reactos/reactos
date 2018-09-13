/****************************Module*Header******************************\
* Module Name: getinfo.c                                                *
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

#include <windows.h>
#include "port1632.h"
#include "pbrush.h"
#include "dlgs.h"

BOOL GetInfo(HWND hWnd)
{
   HANDLE fh;
   TCHAR  tempName[MAX_PATH];
   WORD   errmsg;

   if (!GetDlgItemText(hWnd, edt1, tempName, CharSizeOf(tempName)-1))
       return FALSE;

   fh = MyOpenFile(fileName, NULL, OF_READ | OF_SHARE_DENY_WRITE);
   if (fh == INVALID_HANDLE_VALUE)
   {
       errmsg = IDSCantOpen;
       goto error1;
   }

   if (!MyByteReadFile(fh, &imageHdr, sizeof(DHDR)))
   {
       errmsg = IDSUnableHdr;
       goto error2;
   }

   MyCloseFile(fh);

   return ValidHdr(hWnd, &imageHdr, fileName);

error2:
   MyCloseFile(fh);

error1:
   SimpleMessage(errmsg, fileName, MB_OK | MB_ICONEXCLAMATION);
   return FALSE;
}

BOOL GetBitmapFileInfo(HWND hWnd, LPTSTR npszFileName)
{
   BITMAPINFO HeaderInfo;

   DlgCaptionNo = -1;
   if (LoadBitmapFile(pbrushWnd[PARENTid], npszFileName, &HeaderInfo))
      return FALSE;

   BitmapHeader.wid = (WORD) HeaderInfo.bmiHeader.biWidth;
   BitmapHeader.hgt = (WORD) HeaderInfo.bmiHeader.biHeight;
   BitmapHeader.planes = (BYTE) HeaderInfo.bmiHeader.biPlanes;
   BitmapHeader.bitcount = (BYTE) HeaderInfo.bmiHeader.biBitCount;

   return TRUE;
}

BOOL GetBitmapInfo(HWND hWnd)
{
   LPBITMAPINFO lpHeader;
   HANDLE hHeader;
   TCHAR tempName[MAX_PATH];
   int rc;

   if (!GetDlgItemText(hWnd, edt1, tempName, CharSizeOf(tempName)-1))
      return FALSE;

   DlgCaptionNo = -1;

   if (!(hHeader = GlobalAlloc(GHND, sizeof(BITMAPINFOHEADER) +
                                     256L * sizeof(RGBQUAD))))
       return FALSE;

   lpHeader = (LPBITMAPINFO) GlobalLock(hHeader);
   rc = !LoadBitmapFile(pbrushWnd[PARENTid], tempName, lpHeader);

   if (rc) {
       BitmapHeader.wid = (WORD) lpHeader->bmiHeader.biWidth;
       BitmapHeader.hgt = (WORD) lpHeader->bmiHeader.biHeight;
       BitmapHeader.planes = (BYTE) lpHeader->bmiHeader.biPlanes;
       BitmapHeader.bitcount = (BYTE) lpHeader->bmiHeader.biBitCount;
   } else {
       SimpleMessage(IDSCantOpen, tempName, MB_OK | MB_ICONEXCLAMATION);
   }

   GlobalUnlock(hHeader);
   GlobalFree(hHeader);

   return rc;
}
