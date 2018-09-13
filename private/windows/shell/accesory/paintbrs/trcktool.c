/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*       file:   TrckTool.c                              *
*       system: PC Paintbrush for MS-Windows            *
*       descr:  track mouse movements for a tool        *
*       date:   11/07/89                                *
********************************************************/

#include "onlypbr.h"
#undef NOWINMESSAGES
#undef NOMSG
#undef NOVIRTUALKEYCODES
#undef NOKEYSTATES
#undef NORASTEROPS
#undef NOMEMMGR
#undef NOKERNEL
#undef NOSYSMETRICS

#include <windows.h>
#include "port1632.h"
//#define NOEXTERN
#include "pbrush.h"

extern RECT theBounds;
extern BOOL drawing;
extern HWND pbrushWnd[];

WORD TrackTool(HWND hWnd, TRACKPROC lpfnDrawTool,
      LPRECT lprReturn, WPARAM *wParam, HDC paintDC)
{
   HWND  oldWnd;
   HDC   dstDC;
   MSG   msg;
   RECT  rcClient, rcClip;
   int   nSavedDC;
   POINT pt;

   drawing = TRUE;
   SuspendCopy();

   theBounds = *lprReturn;

   if(paintDC)
      dstDC = paintDC;
   else {
      if(!(dstDC = GetDisplayDC(hWnd)))
         return(WM_TERMINATE);

      nSavedDC = SaveDC(dstDC);
      SelectObject(dstDC, GetStockObject(WHITE_PEN));
      SelectObject(dstDC, GetStockObject(NULL_BRUSH));
      SetROP2(dstDC, R2_XORPEN);
   }

#ifndef WIN32
   LockSegment(-1);
#endif

   oldWnd = SetCapture(hWnd);

//   if (oldWnd == hWnd) oldWnd=NULL;
   DB_OUTF((acDbgBfr,TEXT("SetCapture, %lx had the capture\n"),oldWnd));

   GetClientRect(hWnd, &rcClient);
   ClientToScreen(hWnd, (LPPOINT) &rcClient);
   ClientToScreen(hWnd, ((LPPOINT) &rcClient) + 1);
   /* Client area can be outside the screen. Clip the client area to the
    * visible portion on the screen */
   rcClip.top=rcClip.left=0;
   rcClip.right=GetSystemMetrics(SM_CXSCREEN)-1;
   rcClip.bottom=GetSystemMetrics(SM_CYSCREEN)-1;
   IntersectRect((LPRECT)&rcClip,(LPRECT)&rcClip,(LPRECT)&rcClient);
   ClipCursor(&rcClip);

   (*lpfnDrawTool)(dstDC, lprReturn, *wParam);

   do {
      while(!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE | PM_NOYIELD))
         /* do nothing */ ;

      DB_OUTF((acDbgBfr,TEXT("TT, msg = %x\n"),msg.message));

      switch(msg.message) {
      case WM_KEYDOWN:
      case WM_KEYUP:
      case WM_CHAR:
         if((msg.wParam==VK_SHIFT || msg.wParam==VK_CONTROL)
               && ((msg.message==WM_KEYDOWN && !(msg.lParam&(1L<<30)))
               || msg.message==WM_KEYUP)) {
            msg.message = WM_MOUSEMOVE;
            msg.wParam = (GetKeyState(VK_CONTROL) & 0x8000 ? MK_CONTROL : 0)
                       | (GetKeyState(VK_LBUTTON) & 0x8000 ? MK_LBUTTON : 0)
                       | (GetKeyState(VK_MBUTTON) & 0x8000 ? MK_MBUTTON : 0)
                       | (GetKeyState(VK_RBUTTON) & 0x8000 ? MK_RBUTTON : 0)
                       | (GetKeyState(VK_SHIFT  ) & 0x8000 ? MK_SHIFT   : 0);
	    GetCursorPos(&pt);
	    ScreenToClient(hWnd, &pt);
	    msg.lParam=MAKELONG(pt.x,pt.y);
            PostMessage(hWnd, msg.message, msg.wParam, msg.lParam);
         }
         SendMessage(hWnd, msg.message, msg.wParam, msg.lParam);
         break;

      case WM_MOUSEMOVE:
         SendMessage(hWnd, msg.message, msg.wParam, msg.lParam);

         (*lpfnDrawTool)(dstDC, lprReturn, *wParam);

         lprReturn->right  = LOWORD(msg.lParam);
         lprReturn->bottom = HIWORD(msg.lParam);
         *wParam = msg.wParam;

         (*lpfnDrawTool)(dstDC, lprReturn, *wParam);

	 LONG2POINT(msg.lParam,pt);
	 UnionWithPt(&theBounds,pt);
         break;

      case WM_LBUTTONDOWN:
      case WM_MBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_LBUTTONUP:
      case WM_MBUTTONUP:
      case WM_RBUTTONUP:
      case WM_LBUTTONDBLCLK:
      case WM_MBUTTONDBLCLK:
      case WM_RBUTTONDBLCLK:
         break;

      case WM_ACTIVATEAPP:
        DB_OUT("TT, ActivateApp\n");

      default:
         TranslateMessage(&msg);
         DispatchMessage(&msg);
         break;
      }
   } while(msg.message != WM_LBUTTONUP
         && msg.message != WM_RBUTTONUP
         && msg.message != WM_LBUTTONDBLCLK
         && msg.message != WM_RBUTTONDBLCLK
         && msg.message != WM_LBUTTONDOWN
         && msg.message != WM_RBUTTONDOWN) ;

   (*lpfnDrawTool)(dstDC, lprReturn, *wParam);

   ClipCursor(NULL);
   if(oldWnd) {
        DB_OUT("Set\n");
	SetCapture(oldWnd);
   }
   else {
      DB_OUT("Release\n");
      ReleaseCapture();
   }

#ifndef WIN32
   UnlockSegment(-1);
#endif

   if(!(paintDC)) {
      RestoreDC(dstDC, nSavedDC);
      ReleaseDC(hWnd, dstDC);
   }

   ++theBounds.right;
   ++theBounds.bottom;

   ResumeCopy();
   drawing = FALSE;

   return(msg.message);
}
