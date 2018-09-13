/****************************Module*Header******************************\
* Module Name: saveclip.c                                               *
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

#define NODRAWFRAME
#define NOKEYSTATES
#define NOATOM
#define NOGDICAPMASKS
#define NOSOUND
#define NOCOMM
#define NOKANJI
#define NOPROFILER
#define NOMINMAX
#include <windows.h>
#include "port1632.h"

#include "pbrush.h"

#define BiToByAl(bi,al) (((bi)+(al)*8-1)/((al)*8)*(al))

extern int imagePlanes, imagePixels;
extern TCHAR clipName[], fileName[];
extern WORD wFileType;
extern HWND pbrushWnd[];

BOOL SaveClip(HDC srcDC, HBITMAP srcBM, int wid, int hgt)
{
   BOOL    error;
   HCURSOR oldCsr;
   BITMAP  bm;
   TCHAR   s[MAX_PATH];

   if (!AllocTemp(wid, FILEBUFFrows, imagePlanes, imagePixels, FALSE)) {
       PbrushOkError(IDSNotEnufMem, MB_ICONHAND);
       return FALSE;
   }

   oldCsr = SetCursor(LoadCursor(NULL,IDC_WAIT));

   if (!GetObject(srcBM, sizeof(BITMAP), (LPVOID) &bm)) {
       SimpleMessage(IDSCantCreate, clipName, MB_OK | MB_ICONEXCLAMATION);
       FreeTemp();
       SetCursor(oldCsr);
       return(FALSE);
   }

   lstrcpy(s, fileName);
   lstrcpy(fileName, clipName);
   switch (wFileType) {
       case BITMAPFILE:
       case BITMAPFILE4:
       case BITMAPFILE8:
       case BITMAPFILE24:
           error = !SaveBitmapFile(pbrushWnd[PARENTid], 0, 0, wid, hgt, srcDC);
           break;

       case PCXFILE:
           error = SaveImg(pbrushWnd[PARENTid], 0, 0, wid, hgt,
                           BiToByAl(wid * imagePlanes * imagePixels, 4),
                           pickDC);
           break;
   }
   lstrcpy(fileName, s);


   SetCursor(oldCsr);

   return !error;
}
