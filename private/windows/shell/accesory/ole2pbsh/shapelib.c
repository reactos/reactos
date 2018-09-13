/****************************Module*Header******************************\
* Module Name: filename.c                                               *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
*                                                                       *
* Drawing primitives that generate color for color images and           *
* black/white for monochrome images, regardless of the display type.    *
*                                                                       *
* Additional information such as restrictions, limitations, or special  *
* algorithms used if they are externally visible or effect proper use   *
* of the module.                                                        *
\***********************************************************************/

#include   <windows.h>
#include "port1632.h"

#include   "pbrush.h"

static BOOL    bColor = FALSE;

extern HPALETTE hPalette;
extern int imagePlanes, imagePixels;
extern HWND pbrushWnd[];
extern int theSize;

void CompensateForPen(HDC hDC, LPRECT lpRect)
{
   POINT pt, p1, p2;

   /* convert pen width/height to device coordinates */
   pt.x = pt.y = theSize;


   SetWindowOrgEx(hDC, 0, 0, &p1);
   LPtoDP(hDC, &pt, 1);
   SetWindowOrgEx(hDC, p1.x, p1.y, &p2);
   pt.x = (pt.x + 1) >> 1;
   pt.y = (pt.y + 1) >> 1;

   /* inflate bounding rectangle to surround the object */
   InflateRect(lpRect, pt.x, pt.y);
}

void InitShapeLibrary(void)
{
   if (imagePixels != 1 || imagePlanes != 1)
       bColor = TRUE;
   else {
//       hDC = GetDC(NULL);
//       bColor = (GetDeviceCaps(hDC, NUMCOLORS) == 2);
//       ReleaseDC(NULL, hDC);
         bColor = FALSE;
   }

}

DWORD PBGetNearestColor(HDC hDC, DWORD rgbColor)
{
   /* if we are editing a color image, let GDI handle it */
   if (bColor)
       return GetNearestColor(hDC, rgbColor);

   /* B/W image so threshold the gray scale: 0-127 = black, 128-255 = white */
   return (GetGValue(rgbColor) & 128) ? RGB(255,255,255) : RGB(0,0,0);
}
