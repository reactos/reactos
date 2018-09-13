/****************************Module*Header******************************\
* Module Name: loadcolr.c                                               *
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
#define NOWH       /* SetWindowsHook and WH_* */
#define NOSCROLL
#define NOICONS

#include <windows.h>
#include "port1632.h"
#include "pbrush.h"
#include "colr.h"

BOOL LoadColr(HWND hWnd, LPTSTR fileName)
{
   int         i;
   HANDLE      fh;
   COLORhdr    hdr;
   DWORD       tempColor[MAXcolors];
   long        bwvalue;
   WORD        errmsg, wSize;

   if ((fh = MyOpenFile(fileName, NULL, OF_READ | OF_SHARE_DENY_WRITE)) == INVALID_HANDLE_VALUE)
   {
       errmsg = IDSCantOpen;
       goto error1;
   }

   wSize = sizeof(hdr);
   if (!MyByteReadFile(fh, &hdr, wSize))
   {
       errmsg = IDSUnableHdr;
       goto error2;
   }

   if ('C' != hdr.tag || MAXcolors != hdr.colors) {
       errmsg = IDSBadHeader;
       goto error2;
   }

   wSize = sizeof(tempColor);
   if (!MyByteReadFile (fh, tempColor, wSize))
   {
       errmsg = IDSBadData;
       goto error2;
   }

   MyCloseFile(fh);

   for (i = 0; i < MAXcolors; ++i) {
       if (bwColor == rgbColor) {
           bwvalue = (30 * GetRValue(tempColor[i]) +
                      59 * GetGValue(tempColor[i]) +
                      11 * GetBValue(tempColor[i])) / 100;
           rgbColor[i] = RGB(bwvalue, bwvalue, bwvalue);
       } else
           rgbColor[i] = tempColor[i];
   }

   InvalidateRect(pbrushWnd[COLORid], NULL, FALSE);
   UpdateWindow(pbrushWnd[COLORid]);

   GetCurrentDirectory(PATHlen, colorPath);
   return TRUE;

error2:
   MyCloseFile(fh);

error1:
   SimpleMessage(errmsg, fileName, MB_OK | MB_ICONEXCLAMATION);
   return FALSE;
}
