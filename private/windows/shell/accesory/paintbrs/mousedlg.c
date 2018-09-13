/****************************Module*Header******************************\
* Module Name: mousedlg.c                                               *
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

extern HWND pbrushWnd[];
extern HWND mouseWnd;
extern BOOL mouseFlag;
extern RECT imageView;
extern TCHAR szIntl[];
TCHAR sList[] = TEXT("sList");

BOOL FAR PASCAL MouseDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam)
{
   static RECT fromRect;
   static BOOL moveReq = FALSE;
   static BOOL firstCreate = TRUE;

   int     command;
   POINT   csrPt;
   RECT    r, paintRect, toRect;
   TCHAR    szSep[2];

   switch (message) {
       case WM_INITDIALOG:
           GetProfileString(szIntl, sList, TEXT(","), szSep, CharSizeOf(szSep));
           SetDlgItemText(hDlg, IDMOUSESEP, szSep);
           GetCursorPos(&csrPt);
           ScreenToClient(hDlg, &csrPt);
           SendMessage(hDlg, WM_MOUSEMOVE, 0, MAKELONG(csrPt.x,csrPt.y));
           GetWindowRect(hDlg, &r);
           GetWindowRect(pbrushWnd[PARENTid], &toRect);
           MoveWindow(hDlg,
               toRect.right - 2 * GetSystemMetrics(SM_CXSIZE)
                            - GetSystemMetrics(SM_CXFRAME)
                            - (r.right - r.left) - 8,
               toRect.top + GetSystemMetrics(SM_CYFRAME)
                          - GetSystemMetrics(SM_CYBORDER),
               r.right - r.left,
               r.bottom - r.top,
               TRUE);

           break;

       case WM_SYSCOMMAND:
           if ((command = wParam & 0xFFF0) == SC_CLOSE) {
               CheckMenuItem(GetMenu(pbrushWnd[PARENTid]), MISCmousePos,
                             MF_UNCHECKED);
               DestroyWindow(mouseWnd);
               mouseFlag = FALSE;
               break;
           }

           if (command == SC_MOVE) {
               moveReq = TRUE;
               GetWindowRect(mouseWnd, (LPRECT) &fromRect);
           }

           return FALSE;
           break;

       case WM_MOUSESYS:
           if ((command = wParam & 0xFFF0) == SC_CLOSE) {
               CheckMenuItem(GetMenu(pbrushWnd[PARENTid]), MISCmousePos,
                             MF_UNCHECKED);
               DestroyWindow(mouseWnd);
               mouseFlag = FALSE;
               break;
           }

           if (command == SC_MOVE) {
               moveReq = TRUE;
               GetWindowRect(mouseWnd, (LPRECT) &fromRect);
           }
           break;

       case WM_MOVE:
           if (moveReq) {
               moveReq = FALSE;
               GetWindowRect(mouseWnd, &toRect);
               GetWindowRect(pbrushWnd[PAINTid], &paintRect);
               if (IntersectRect(&r, &toRect,
                                 &paintRect))
                   MoveWindow(mouseWnd, fromRect.left, fromRect.top,
                              fromRect.right - fromRect.left,
                              fromRect.bottom - fromRect.top,
                              TRUE);
           }
           break;

       case WM_MOUSEPOS:
           SetDlgItemInt(hDlg, IDMOUSEX, imageView.left + LOWORD(lParam),
                         TRUE);
           SetDlgItemInt(hDlg, IDMOUSEY, imageView.top + HIWORD(lParam),
                         TRUE);
           break;

#ifdef JAPAN    //KKBUGFIX     // added by Hiraisi 11 Nov. 1992 (BUG#457/WIN31 in Japan)
       // The mouse window sometimes overlaps on the paint window.
       case WM_MOUSEWINDOW:
       {
           RECT    trect, trect1, r;

           GetWindowRect(mouseWnd, &trect);
           GetWindowRect(pbrushWnd[PAINTid], &trect1);
           if (IntersectRect(&r, &trect, &trect1)){
               GetWindowRect(pbrushWnd[PARENTid], &trect1);
               MoveWindow(mouseWnd,
                          trect1.right - 2 * GetSystemMetrics(SM_CXSIZE)
                                       - GetSystemMetrics(SM_CXFRAME)
                                       - (trect.right - trect.left) - 8,
                          trect1.top + GetSystemMetrics(SM_CYFRAME)
                                     - GetSystemMetrics(SM_CYBORDER),
                          trect.right - trect.left,
                          trect.bottom - trect.top,
                          TRUE);
           }
           break;
       }
#endif

       default:
           return FALSE;
           break;
   }

   return TRUE;
}

