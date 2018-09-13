/****************************Module*Header******************************\
* Module Name: scrolimg.c                                               *
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

extern BOOL bExchanged;
extern RECT rDirty;

void ScrolImg(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
   int     x, y, maxPos;
   POINT   origin;
   HDC     hdc;
   RECT    rcClient;

   if (wParam == SB_THUMBTRACK)
       return;

   if (DrawProc != ZoomInDP) {
       //DOUTR(L"ScrollImg: sending WM_TERMINATE");
       SendMessage(pbrushWnd[PAINTid], WM_TERMINATE, 0, 0L);
   }

   UpdatImg();
   x = imageView.left;
   y = imageView.top;

   if (message == WM_VSCROLL) {
       maxPos = imageHgt - paintHgt;
       switch (GET_WM_VSCROLL_CODE(wParam,lParam)) {
           case SB_LINEUP:
               if ((imageView.top -= 10) < 0)
                   imageView.top = 0;
               break;

           case SB_LINEDOWN:
               if ((imageView.top += 10) > maxPos)
                   imageView.top = maxPos;
               break;

           case SB_PAGEUP:
               if ((imageView.top -= paintHgt) < 0)
                   imageView.top = 0;
               break;

           case SB_PAGEDOWN:
               if ((imageView.top += paintHgt) > maxPos)
                  imageView.top = maxPos;
               break;

           case SB_THUMBPOSITION:
	       imageView.top = GET_WM_VSCROLL_POS(wParam,lParam);
               break;

           case SB_TOP:
               imageView.top = 0;
               break;

           case SB_BOTTOM:
               imageView.top = maxPos;
               break;
       }
       SetScrollPos(hWnd, SB_VERT, imageView.top, TRUE);
       imageView.bottom = imageView.top + paintHgt;
   } else {
       maxPos = imageWid - paintWid;
       switch (GET_WM_HSCROLL_CODE(wParam,lParam)) {
           case SB_LINEUP:
               if ((imageView.left -= 8) < 0)
                   imageView.left = 0;
               break;

           case SB_LINEDOWN:
               if ((imageView.left += 8) > maxPos)
                   imageView.left = maxPos;
               break;

           case SB_PAGEUP:
               if ((imageView.left -= 4*(paintWid/8)) < 0)
                   imageView.left = 0;
               break;

           case SB_PAGEDOWN:
               if ((imageView.left += 4*(paintWid/8)) > maxPos)
                   imageView.left = maxPos;
               break;

           case SB_THUMBPOSITION:
	       imageView.left = GET_WM_HSCROLL_POS(wParam,lParam);
               break;

           case SB_TOP:
               imageView.left = 0;
               break;

           case SB_BOTTOM:
               imageView.left = maxPos;
               break;
       }

       if (imageView.left && (imageView.left != maxPos)) {
           origin.x = origin.y = 0;
           hdc = GetDisplayDC(hWnd);
           if (hdc) {
               LPtoDP(hdc, &origin, 1);
               ReleaseDC(hWnd, hdc);
           }
           imageView.left = (origin.x % 8) + 8*(imageView.left / 8);
       }
       SetScrollPos(hWnd, SB_HORZ, imageView.left, TRUE);
       imageView.right = imageView.left + paintWid;
   }

   GetClientRect(hWnd, &rcClient);
   ScrollWindow(hWnd,x - imageView.left,y - imageView.top, &rcClient,
		NULL);

   UpdateWindow(hWnd);
}
