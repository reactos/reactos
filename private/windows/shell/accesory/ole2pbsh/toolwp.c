/****************************Module*Header******************************\
* Module Name: toolwp.c                                                 *
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
#include <port1632.h>

#include "oleglue.h"
#include "pbrush.h"


extern HWND pbrushWnd[];
extern HBITMAP hToolbox;
extern RECT pbrushRct[];
extern int theTool, cursTool;
extern BOOL inMagnify, bZoomedOut;
extern LPTSTR DrawCursor;
extern DPPROC DrawProc;
extern DPPROC dpArray[];
extern struct csstat CursorStat;
extern HWND zoomOutWnd;

/* this inverts the appropriate button:
** tool: 0...MAXtools-1
** hdc : such as ps->hdc
*/
void InvertButton(HDC hdc, int tool)
{
   RECT r;
   RECT clients;
   int wid;

   tool = min(MAXtools - 1, max(0, tool));

   GetClientRect(pbrushWnd[TOOLid], (LPRECT) &clients);
   wid = (clients.right - clients.left) / 2;

   r.top = (clients.top + (tool / 2) * (clients.bottom - clients.top)
         / (MAXtools / 2)) + 1;
   r.bottom = (clients.top + (tool / 2 + 1) * (clients.bottom - clients.top)
            / (MAXtools / 2));
   r.bottom = min(r.bottom, clients.bottom - 1);

   r.left  = clients.left + ((tool & 1) ? 1 + wid : 0);
   r.right = r.left + wid - 1;

   InvertRect(hdc, (LPRECT) &r);
}

/* display the requisite bitmap in the toolbox area */
void xToolPaint(LPPAINTSTRUCT ps)
{
   HDC     hDC, hDCsrc;
   BITMAP  hbits;
   HBRUSH  hTempBrush;
   RECT    clients;
   int     i, Y;
   SHORTPARM oldBltMode;

   if (!hToolbox)
       return;

   GetClientRect(pbrushWnd[TOOLid], &clients);
   hDC = ps->hdc;

   SetROP2(hDC, R2_COPYPEN);

   oldBltMode = SetStretchBltMode(hDC, (((UINT)GetDeviceCaps(hDC, NUMCOLORS)) >
                                       2) ? COLORONCOLOR : BLACKONWHITE);

   GetObject(hToolbox, sizeof(hbits), (LPVOID) &hbits);

   hDCsrc = CreateCompatibleDC(hDC);
   if (hDCsrc) {
       SelectObject(hDCsrc, hToolbox);

       StretchBlt(hDC, 0, 0, clients.right - clients.left,
                  clients.bottom - clients.top,
                  hDCsrc, 0, 0,
                  hbits.bmWidth, hbits.bmHeight,
                  SRCCOPY);
   }

   for (i = 0; i < (MAXtools / 2); ++i) {
      Y = (clients.top + i * (clients.bottom - clients.top) / (MAXtools / 2));
      MMoveTo(hDC, clients.left, Y);
      LineTo(hDC, clients.right, Y);
   }
   MMoveTo(hDC, (clients.left + clients.right) / 2, clients.top);
   LineTo(hDC, (clients.left + clients.right) / 2, clients.bottom);


   hTempBrush = SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));
   Rectangle(hDC, clients.top, clients.left, clients.right, clients.bottom);
   if (hTempBrush)
       SelectObject(hDC, hTempBrush);

   SetStretchBltMode(hDC, oldBltMode);
   if (hDCsrc)
       DeleteDC(hDCsrc);
}

LONG PASCAL ToolWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
   static int oldTool;

   PAINTSTRUCT ps;
   int         wid, hgt, newTool;
   HDC         toolDC;
   POINT       pt;

   /* calculate dimensions of tool rectangles */
   wid = 1 + ((pbrushRct[TOOLid].right - pbrushRct[TOOLid].left) >> 1);
   hgt = pbrushRct[TOOLid].bottom - pbrushRct[TOOLid].top;

   switch (message) {
      case WM_CREATE:
         toolDC = CreateIC(TEXT("display"), NULL, NULL, NULL);
         if(((UINT)GetDeviceCaps(toolDC, NUMCOLORS)) > 2)
            hToolbox = LoadBitmap(hInst, TEXT("pToolbox"));
         else
            hToolbox = LoadBitmap(hInst, TEXT("pBWToolbox"));
         DeleteDC(toolDC);
         break;

      case WM_DESTROY:
         DeleteObject(hToolbox);
         break;

       case WM_MOUSEMOVE:
           if (SetCursorOn())
               SendMessage(pbrushWnd[PAINTid], WM_HIDECURSOR, 0, 0L);
           break;

       /* paint tool window */
       case WM_PAINT:
           BeginPaint(hWnd, &ps);
           if (!gfInvisible)
           {
               xToolPaint(&ps);
               InvertButton(ps.hdc, theTool);
           }
           EndPaint(hWnd, &ps);
           break;

      case WM_SELECTTOOL:
           message = WM_LBUTTONDOWN;
           if (!wParam)
               wParam = theTool;
           pt.x = ((wid - 1) * 2 * ((wParam % 2) * 2 + 1)) / 4;
           pt.y = (hgt * ((wParam / 2) * 2 + 1)) / MAXtools;
           lParam = MAKELONG(pt.x, pt.y);

           //
           // fall through here
           //

       /* select new tool */
       case WM_LBUTTONDOWN:
       case WM_RBUTTONDOWN:
           oldTool = theTool;
           LONG2POINT(lParam,pt);
           newTool = 2 * ((pt.y * TOOLdiv) / hgt) + pt.x / wid;
           newTool = min(max(0, newTool), MAXtools - 1);

           DOUTR(L"toolwp BUTTONDOWN: sending WM_TERMINATE");
           SendMessage(pbrushWnd[PAINTid], WM_TERMINATE, 0, 0L);

           if ((!inMagnify && !bZoomedOut) ||
               (inMagnify && ((newTool == BRUSHtool) ||
               (newTool == ROLLERtool))) ||
               (bZoomedOut && (newTool == PICKtool))) {
               if ((newTool != theTool) && (message != WM_RBUTTONDOWN)) {
                   toolDC = GetDisplayDC(hWnd);
                   if (toolDC) {
                       InvertButton(toolDC, theTool);
                       theTool = newTool;
                       InvertButton(toolDC, theTool);
                       ReleaseDC(hWnd, toolDC);
                   }
                   if (inMagnify && (theTool == BRUSHtool))
                   {
                       /* FIX BUG 3489(d) - EDH - 20 SEP 91 */
                       PbSetCursor(DrawCursor = szPbCursor(cursTool = HANDtool));
                      /* END FIX - (Was ZOOMINtool) */
                   }
                   else
                       PbSetCursor(DrawCursor = szPbCursor(cursTool = newTool));
               }
               /* set pointer to selected drawing proc */
               DOUT(L"\ntoolwp: setting tool pointer\n" );
               DrawProc = dpArray[theTool];
               DrawCursor = szPbCursor(theTool);
               if (CursorStat.noted) {
                   CursorStat.noted = FALSE;
               }
               if (!inMagnify)
                   UpdatImg();
           } else {
               MessageBeep(0);
           }

           if (bZoomedOut && theTool == PICKtool) {
               SendMessage(zoomOutWnd, WM_COMMAND, ZOOMaccept, 0l);
               SendMessage(zoomOutWnd, WM_COMMAND, EDITundo, 0l);
           }
           break;

       case WM_LBUTTONDBLCLK:
           LONG2POINT(lParam,pt);
           newTool = 2 * ((pt.y * TOOLdiv) / hgt) + pt.x / wid;
           newTool = min(max(0, newTool), MAXtools - 1);
           if (!inMagnify && !(bZoomedOut && (PICKtool != newTool))) {
               if (newTool == ERASERtool) {
                   SendMessage(pbrushWnd[PARENTid], WM_COMMAND, FILEnew, 0L);
               } else if (newTool == COLORERASERtool) {
                   SendMessage(pbrushWnd[PAINTid], WM_WHOLE, 0, 0L);
               } else if (newTool == BRUSHtool)
                   SendMessage(pbrushWnd[PARENTid], WM_COMMAND, MISCbrush, 0L);
               else if (newTool == PICKtool)
                   SendMessage(pbrushWnd[PARENTid], WM_COMMAND, FILEshow, 0L);
               else
                   MessageBeep(0);
           } else
               MessageBeep(0);
           break;

       default:
           return DefWindowProc(hWnd, message, wParam, lParam);
           break;
   }

   return 0L;
}
