/****************************Module*Header******************************\
* Module Name: scrolmag.c                                               *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation			*
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

extern int paintWid, paintHgt, imageWid, imageHgt;
extern RECT imageView, zoomView;
extern int zoomWid, zoomHgt;

void ScrolMag(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
   int maxPos, ix, iy;

   SendMessage(hWnd, WM_SCROLLINIT, 0, 0L);

   ix = imageWid;
   iy = imageHgt;

   if ((ix - imageView.left) < imageWid)
       ix = min(ix + GetSystemMetrics(SM_CXVSCROLL), 
                imageWid - imageView.left);
   if ((iy - imageView.top) < imageHgt)
       iy = min(iy + GetSystemMetrics(SM_CYHSCROLL),
                imageHgt - imageView.top);

   if (message == WM_VSCROLL) {
       maxPos = iy - zoomHgt;

       switch (GET_WM_VSCROLL_CODE(wParam,lParam)) {
           case SB_LINEUP:
               if ((zoomView.top -= 10) < 0)
                   zoomView.top = 0;
               break;

           case SB_LINEDOWN:
               if ((zoomView.top += 10) > maxPos)
                   zoomView.top = maxPos;
               break;

           case SB_PAGEUP:
               if ((zoomView.top -= zoomHgt) < 0)
                   zoomView.top = 0;
               break;

           case SB_PAGEDOWN:
               if ((zoomView.top += zoomHgt) > maxPos)
                   zoomView.top = maxPos;
               break;

           case SB_THUMBPOSITION:
           case SB_THUMBTRACK:
	       zoomView.top = GET_WM_VSCROLL_POS(wParam,lParam);
               break;

           case SB_TOP:
               zoomView.top = 0;
               break;

           case SB_BOTTOM:
               zoomView.top = maxPos;
               break;
       }

       SetScrollPos(hWnd, SB_VERT, zoomView.top, TRUE);
       zoomView.bottom = zoomView.top + zoomHgt;
   } else {
       maxPos = ix - zoomWid;

       switch (GET_WM_HSCROLL_CODE(wParam,lParam)) {
           case SB_LINEUP:
               if ((zoomView.left -= 10) < 0)
                   zoomView.left = 0;
               break;

           case SB_LINEDOWN:
               if ((zoomView.left += 10) > maxPos)
                   zoomView.left = maxPos;
               break;

           case SB_PAGEUP:
               if ((zoomView.left -= zoomWid) < 0)
                   zoomView.left = 0;
               break;

           case SB_PAGEDOWN:
               if ((zoomView.left += zoomWid) > maxPos)
                   zoomView.left = maxPos;
               break;

           case SB_THUMBPOSITION:
           case SB_THUMBTRACK:
	       zoomView.left = GET_WM_HSCROLL_POS(wParam,lParam);
               break;

           case SB_TOP:
               zoomView.left = 0;
               break;

           case SB_BOTTOM:
               zoomView.left = maxPos;
               break;
       }

       SetScrollPos(hWnd, SB_HORZ, zoomView.left, TRUE);
       zoomView.right = zoomView.left + zoomWid;
   }

   SendMessage(hWnd, WM_SCROLLVIEW, 0, 0L);
}
