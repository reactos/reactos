/****************************Module*Header******************************\
* Module Name: allocimg.c                                               *
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

#define BiToByAl(bi,al) (((bi)+(al)*8-1)/((al)*8)*(al))

BOOL AllocTemp(int wid, int hgt, int planes, int pixelBits, BOOL f24PCX)
{
   BITMAP bm;
   HDC hDC;
   int fplanes, fpixelBits;

   /* set parameter default values if necessary */
   hDC = GetDisplayDC(pbrushWnd[PARENTid]);
   fplanes = abs(planes);
   fpixelBits = abs(pixelBits);

   if(planes <= 0)
       planes = GetDeviceCaps(hDC, PLANES);

   if (pixelBits <= 0)
       pixelBits = GetDeviceCaps(hDC, BITSPIXEL);

   ReleaseDC(pbrushWnd[PARENTid], hDC);

   /* Special case 24 bpp PCX file saves */
   if (!f24PCX) {
       /* attempt to create the global bitmap */
       if (!(fileBitmap = CreateBitmap(wid, hgt, (BYTE) planes,
           (BYTE) pixelBits, (LPVOID) NULL)))
           goto error1;

       if (!(fileDC = CreateCompatibleDC(NULL)))
           goto error2;

       /* attempt to allocate file buffer */
       if (!SelectObject(fileDC, fileBitmap))
           goto error3;

       bm.bmWidthBytes = BiToByAl(fplanes * fpixelBits * wid, 4);
       hfileBuff = LocalAlloc(LHND, hgt *
                                    BiToByAl(fplanes * fpixelBits * wid, 4));
       if (!hfileBuff)
           goto error3;

       if (!(fileBuff = LocalLock(hfileBuff)))
           goto error4;

       /* set file globals */
       filePlanes = fplanes;
       fileWid = wid;
       fileByteWid = bm.bmWidthBytes;
   } else {
       /* attempt to create the global bitmap */
       fpixelBits = 8;
       fplanes = 3;

       if (!(fileBitmap = CreateBitmap(wid, hgt, (BYTE) 1,
           (BYTE) pixelBits, (LPVOID) NULL)))
           goto error1;

       if (!(fileDC = CreateCompatibleDC(NULL)))
           goto error2;

       /* attempt to allocate file buffer */
       if (!SelectObject(fileDC, fileBitmap))
           goto error3;

       //bmWidthBytes == width of one PLANE
       bm.bmWidthBytes = BiToByAl(fpixelBits * wid, 4);

       hfileBuff = LocalAlloc(LHND, fplanes * bm.bmWidthBytes);

       if (!hfileBuff)
           goto error3;

       if (!(fileBuff = LocalLock(hfileBuff)))
           goto error4;

       /* set file globals */
       filePlanes = fplanes;
       fileWid = wid;
       fileByteWid = bm.bmWidthBytes;
   }
   return TRUE;

error4:
   LocalFree(hfileBuff);
   hfileBuff = NULL;

error3:
   DeleteDC(fileDC);
   fileDC = NULL;

error2:
   DeleteObject(fileBitmap);
   fileBitmap = NULL;

error1:
   return FALSE;
}
