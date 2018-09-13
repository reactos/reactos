/****************************Module*Header******************************\
* Module Name: brushdlg.c                                               *
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

extern int theBrush;

BOOL FAR PASCAL BrushDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam)
{
   static int selected, tempbrush;
   static BOOL firstHilite;

   HDC dlgDC;
   int x,y;
   RECT butrect;
   BOOL selection;
   LPDRAWITEMSTRUCT lpDrawItem;
   WORD wCtl, wAction;
   HBRUSH hBrush,hOldBrush;
   HPEN hPen, hOldPen;
   int	ROPold;

   switch (message) {
       case WM_COMMAND:
           selection = FALSE;
	   switch (GET_WM_COMMAND_ID(wParam,lParam)) {
               case IDOK:
                   selection = TRUE;
                   theBrush = tempbrush;
                   break;

               case IDCANCEL:
                   selection = TRUE;
                   break;
   
               default:
		   if (GET_WM_COMMAND_CMD(wParam,lParam) == BN_DOUBLECLICKED) {
		       theBrush = GET_WM_COMMAND_ID(wParam,lParam) - IDRECTBRUSH;
                       selection = TRUE;
                   }
                   break;
           }
           if (selection)
               EndDialog(hDlg, TRUE);
           break;

       case WM_INITDIALOG:
           CenterWindow(hDlg);
           selected = -1;
           firstHilite = TRUE;
           SetFocus(GetDlgItem(hDlg, theBrush+IDRECTBRUSH));
           return(FALSE);
           break;

      case WM_DRAWITEM:
           lpDrawItem = (LPDRAWITEMSTRUCT) lParam;
           wAction = lpDrawItem->itemAction;
           wCtl = lpDrawItem->CtlID;
           dlgDC = lpDrawItem->hDC;

           if (wAction & ODA_DRAWENTIRE) {
		hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT));
		hOldBrush = SelectObject(dlgDC, GetStockObject(BLACK_BRUSH));
		hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWTEXT));
		hOldPen = SelectObject(dlgDC, hPen);
		ROPold = SetROP2(dlgDC, R2_COPYPEN);

		butrect = lpDrawItem->rcItem;
		x = (butrect.right - butrect.left) / 3;
		y = (butrect.bottom - butrect.top) / 3;

	       if ((int)wCtl - IDRECTBRUSH == tempbrush) {
                   SelectObject(dlgDC, GetStockObject(HOLLOW_BRUSH));
                   Rectangle(dlgDC, butrect.left, butrect.top,
                             butrect.right, butrect.bottom);
		   SelectObject(dlgDC, hBrush);
               }

               switch (wCtl) {
                   case IDRECTBRUSH:
                       Rectangle(dlgDC, x, y, x << 1, y << 1);
                       break;

                   case IDOVALBRUSH:
                       Ellipse(dlgDC, x, y, x << 1, y << 1);
                       break;

                   case IDHORZBRUSH:
		       MMoveTo(dlgDC, x, (3 * y) >> 1);
                       LineTo(dlgDC, x << 1, (3 * y) >> 1);
                       break;

                   case IDVERTBRUSH:
		       MMoveTo(dlgDC, (3 * x) >> 1, y);
                       LineTo(dlgDC, (3 * x) >> 1, y << 1);
                       break;

                   case IDSLANTLBRUSH:
		       MMoveTo(dlgDC, x, y << 1);
                       LineTo(dlgDC, x << 1, y);
                       break;

                   case IDSLANTRBRUSH:
		       MMoveTo(dlgDC, x, y);
                       LineTo(dlgDC, x << 1, y << 1);
                       break;
               }

		SetROP2(dlgDC, ROPold);
		SelectObject(dlgDC, hOldPen);
		DeleteObject(hPen);
		SelectObject(dlgDC, hOldBrush);
		DeleteObject(hBrush);
           }

	   if ((wAction & ODA_SELECT) || (wAction & ODA_FOCUS)) {
	       if (IDRECTBRUSH <= wCtl && wCtl <= IDSLANTRBRUSH) {
                   InvalidateRect(GetDlgItem(hDlg, tempbrush + IDRECTBRUSH),
                                  NULL, TRUE);
                   tempbrush = wCtl - IDRECTBRUSH;
                   InvalidateRect(lpDrawItem->hwndItem, NULL, TRUE);
	       }
           }
           break;

       default:
           return FALSE;
   }

   return TRUE;
}
