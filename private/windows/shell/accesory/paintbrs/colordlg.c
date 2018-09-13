/****************************Module*Header******************************\
* Module Name: colordlg.c                                               *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1990  Microsoft Corporation                      *
*                                                                       *
* A general description of how the module is used goes here.            *
*                                                                       *
* Additional information such as restrictions, limitations, or special  *
* algorithms used if they are externally visible or effect proper use   *
* of the module.                                                        *
\***********************************************************************/

#include <windows.h>
#include "port1632.h"
//#define NOEXTERN
#include "pbrush.h"

extern DWORD *rgbColor;
extern DWORD colorColor[], bwColor[], defltColor[], defltBW[];
extern int theTool, theForeg, theBackg;
extern HWND pbrushWnd[];
extern BOOL bZoomedOut;
extern HWND colorWnd, hDlgModeless;


BOOL FAR PASCAL ColorDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam)
{
   static first = TRUE;
   static bRedraw = TRUE;
   static DWORD rgbSave[MAXcolors];

   int color;
   UINT i;
   BYTE red,green,blue;
   HDC dc;
   HBRUSH solidBrush, hOldBrush;
   HMENU hMenu;
   LPDRAWITEMSTRUCT lpDrawItem;
   LPMEASUREITEMSTRUCT lpMeasureItem;

   switch (message) {
       case WM_COMMAND:
	   switch (GET_WM_COMMAND_ID(wParam,lParam)) {
               case IDOK:
                   red = (BYTE) GetScrollPos(GetDlgItem(hDlg,IDREDBAR),SB_CTL);
                   green = (BYTE) GetScrollPos(GetDlgItem(hDlg,IDGREENBAR),SB_CTL);
                   blue = (BYTE) GetScrollPos(GetDlgItem(hDlg,IDBLUEBAR),SB_CTL);
                   rgbColor[theForeg] = RGB(red,green,blue);

                   EnableWindow(pbrushWnd[PAINTid], TRUE);
                   EnableWindow(pbrushWnd[TOOLid], TRUE);
                   EnableWindow(pbrushWnd[SIZEid], TRUE);
                   hMenu = GetMenu(pbrushWnd[PARENTid]);
		   EnableMenuItem(hMenu, 0, (UINT)(bZoomedOut ? MF_GRAYED
                                                        : MF_ENABLED) 
                                            | MF_BYPOSITION);
		   EnableMenuItem(hMenu, 1, (UINT)MF_ENABLED | MF_BYPOSITION);
		   EnableMenuItem(hMenu, 2, (UINT)MF_ENABLED | MF_BYPOSITION);
                   for (i = 3; i < MAXmenus; ++i)
                       EnableMenuItem(hMenu, i, (bZoomedOut ? MF_GRAYED 
                                                            : MF_ENABLED) 
                                                | MF_BYPOSITION);

                   if ((theTool == PICKtool || SCISSORStool == theTool) 
                       && !bZoomedOut)
		       EnableMenuItem(hMenu, MENUPOS_PICK, MF_ENABLED | MF_BYPOSITION);
                   else
		       EnableMenuItem(hMenu, MENUPOS_PICK, MF_GRAYED | MF_BYPOSITION);
                   DrawMenuBar(pbrushWnd[PARENTid]);

                   InvalidateRect(pbrushWnd[COLORid], (LPRECT) NULL, FALSE);
                   UpdateWindow(pbrushWnd[COLORid]);

                   colorWnd = NULL;
                   DestroyWindow(hDlg);
                   hDlgModeless = 0;
                   break;

               case IDDEFAULT:
                   SendMessage(hDlg, WM_COMMAND, IDSETBARS, 
                               (colorColor == rgbColor ? defltColor[theForeg] 
                                                       : defltBW[theForeg]));
                   SendMessage(hDlg, WM_COMMAND, IDSETEDIT, 0L);

                   InvalidateRect(GetDlgItem(hDlg, IDCOLOR), (LPRECT) NULL, 
                                  FALSE);
                   UpdateWindow(GetDlgItem(hDlg, IDCOLOR));
                   break;

               case IDSETEDIT:
                   red = (BYTE) GetScrollPos(GetDlgItem(hDlg, IDREDBAR),
                                             SB_CTL);
                   green = (BYTE) GetScrollPos(GetDlgItem(hDlg, IDGREENBAR),
                                               SB_CTL);
                   blue = (BYTE) GetScrollPos(GetDlgItem(hDlg, IDBLUEBAR),
                                              SB_CTL);
                   SetDlgItemInt(hDlg, IDREDEDIT, red, FALSE);
                   SetDlgItemInt(hDlg, IDGREENEDIT, green, FALSE);
                   SetDlgItemInt(hDlg, IDBLUEEDIT, blue, FALSE);

                   InvalidateRect(GetDlgItem(hDlg, IDCOLOR), (LPRECT) NULL, 
                                  FALSE);
                   UpdateWindow(GetDlgItem(hDlg, IDCOLOR));
                   break;

               case IDSETBARS:
                   SetScrollPos(GetDlgItem(hDlg, IDREDBAR), SB_CTL,
                                GetRValue(lParam), TRUE);
                   SetScrollPos(GetDlgItem(hDlg, IDGREENBAR), SB_CTL,
                                GetGValue(lParam), TRUE);
                   SetScrollPos(GetDlgItem(hDlg, IDBLUEBAR), SB_CTL, 
                                GetBValue(lParam), TRUE);
                   break;

               case IDCANCEL:
                   for (i = 0; i < MAXcolors; ++i)
                       rgbColor[i] = rgbSave[i];
                   SendMessage(hDlg, WM_COMMAND, IDSETBARS, rgbColor[theForeg]);
                   SendMessage(hDlg, WM_COMMAND, IDOK, 0L);
                   break;

               case IDREDEDIT:
		   if (GET_WM_COMMAND_CMD(wParam,lParam) != EN_UPDATE)
                       break;

                   color = GetDlgItemInt(hDlg, IDREDEDIT, NULL, FALSE);
                   if (color > 255 || color < 0) {
                       color = min(max(color, 0), 255);
                       SetDlgItemInt(hDlg, IDREDEDIT, color, FALSE);
                   }
                   SetScrollPos(GetDlgItem(hDlg, IDREDBAR), SB_CTL, color,
                                TRUE);
                   if (rgbColor == bwColor && first) {
                       first = FALSE;
                       SetScrollPos(GetDlgItem(hDlg, IDGREENBAR), SB_CTL, 
                                    color, TRUE);
                       SetScrollPos(GetDlgItem(hDlg, IDBLUEBAR), SB_CTL,
                                    color, TRUE);
                       SetDlgItemInt(hDlg, IDGREENEDIT, color, FALSE);
                       SetDlgItemInt(hDlg, IDBLUEEDIT, color, FALSE);
                       first = TRUE;
                   }

                   if (bRedraw) {
                       InvalidateRect(GetDlgItem(hDlg, IDCOLOR), (LPRECT) NULL, 
                                      FALSE);
                       UpdateWindow(GetDlgItem(hDlg, IDCOLOR));
                   }
                   break;

               case IDGREENEDIT:
		   if (GET_WM_COMMAND_CMD(wParam,lParam) != EN_UPDATE)
                       break;

                   color = GetDlgItemInt(hDlg, IDGREENEDIT, NULL, FALSE);
                   if (color > 255 || color < 0) {
                       color = min(max(color, 0), 255);
                       SetDlgItemInt(hDlg, IDGREENEDIT, color, FALSE);
                   }
                   SetScrollPos(GetDlgItem(hDlg, IDGREENBAR), SB_CTL, color,
                                TRUE);

                   if (rgbColor == bwColor && first) {
                       first = FALSE;
                       SetScrollPos(GetDlgItem(hDlg, IDREDBAR), SB_CTL, color,
                                    TRUE);
                       SetScrollPos(GetDlgItem(hDlg, IDBLUEBAR), SB_CTL, color,
                                    TRUE);
                       SetDlgItemInt(hDlg, IDREDEDIT, color, FALSE);
                       SetDlgItemInt(hDlg, IDBLUEEDIT, color, FALSE);
                       first = TRUE;
                   }

                   if (bRedraw) {
                       InvalidateRect(GetDlgItem(hDlg,IDCOLOR), (LPRECT) NULL, 
                                      FALSE);
                       UpdateWindow(GetDlgItem(hDlg,IDCOLOR));
                   }
                   break;

               case IDBLUEEDIT:
		   if (GET_WM_COMMAND_CMD(wParam,lParam) != EN_UPDATE)
                       break;

                   color = GetDlgItemInt(hDlg, IDBLUEEDIT, NULL, FALSE);
                   if (color > 255 || color < 0) {
                       color = min(max(color, 0), 255);
                       SetDlgItemInt(hDlg, IDBLUEEDIT, color, FALSE);
                   }
                   SetScrollPos(GetDlgItem(hDlg,IDBLUEBAR),SB_CTL,color,
                                TRUE);

                   if (rgbColor == bwColor && first) {
                       first = FALSE;
                       SetScrollPos(GetDlgItem(hDlg, IDGREENBAR), SB_CTL,
                                    color, TRUE);
                       SetScrollPos(GetDlgItem(hDlg, IDREDBAR), SB_CTL,
                                    color, TRUE);
                       SetDlgItemInt(hDlg, IDREDEDIT, color, FALSE);
                       SetDlgItemInt(hDlg, IDGREENEDIT, color, FALSE);
                       first = TRUE;
                   }

                   if (bRedraw) {
                       InvalidateRect(GetDlgItem(hDlg, IDCOLOR), (LPRECT) NULL, 
                                      FALSE);
                       UpdateWindow(GetDlgItem(hDlg,IDCOLOR));
                   }
                   break;


           }
           break;

       case WM_INITDIALOG:
           CenterWindow(hDlg);

           for (i = 0; i < MAXcolors; ++i)
               rgbSave[i] = rgbColor[i];

           SetScrollRange(GetDlgItem(hDlg, IDREDBAR), SB_CTL, 
                          0, 255, FALSE);
           SetScrollRange(GetDlgItem(hDlg, IDGREENBAR), SB_CTL,
                          0, 255, FALSE);
           SetScrollRange(GetDlgItem(hDlg, IDBLUEBAR), SB_CTL,
                          0, 255, FALSE);

           SendMessage(hDlg, WM_COMMAND, IDSETBARS, rgbColor[theForeg]);
           SendMessage(hDlg, WM_COMMAND, IDSETEDIT, 0L);
           break;

       case WM_HSCROLL:
           bRedraw = FALSE;
	   color = GetScrollPos(GET_WM_HSCROLL_HWND(wParam,lParam),SB_CTL);

	   switch (GET_WM_HSCROLL_CODE(wParam,lParam)) {
               case SB_LINEUP:
                   if (--color < 0)
                       color = 0;
                   break;

               case SB_LINEDOWN:
                   if (++color > 255)
                       color = 255;
                   break;

               case SB_PAGEUP:
                   if ((color -= 10) < 0)
                       color = 0;
                   break;

               case SB_PAGEDOWN:
                   if ((color += 10) > 255)
                       color = 255;
                   break;

               case SB_THUMBPOSITION:
               case SB_THUMBTRACK:
		   color = GET_WM_HSCROLL_POS(wParam,lParam);
                   break;

               case SB_TOP:
                   color = 0;
                   break;

               case SB_BOTTOM:
                   color = 255;
                   break;
           }

           if (rgbColor == colorColor) {
	       SetScrollPos(GET_WM_HSCROLL_HWND(wParam,lParam), SB_CTL, color, TRUE);

	       if (GET_WM_HSCROLL_HWND(wParam,lParam) == GetDlgItem(hDlg, IDREDBAR))
                   SetDlgItemInt(hDlg, IDREDEDIT, color, FALSE);
	       else if (GET_WM_HSCROLL_HWND(wParam,lParam) == GetDlgItem(hDlg, IDGREENBAR))
                   SetDlgItemInt(hDlg, IDGREENEDIT, color, FALSE);
               else
                   SetDlgItemInt(hDlg, IDBLUEEDIT, color, FALSE);
           } else {
               SetScrollPos(GetDlgItem(hDlg, IDREDBAR), SB_CTL, color, TRUE);
               SetScrollPos(GetDlgItem(hDlg, IDBLUEBAR), SB_CTL, color, TRUE);
               SetScrollPos(GetDlgItem(hDlg, IDGREENBAR), SB_CTL, color, TRUE);
               SetDlgItemInt(hDlg, IDREDEDIT, color, FALSE);
               SetDlgItemInt(hDlg, IDGREENEDIT, color, FALSE);
               SetDlgItemInt(hDlg, IDBLUEEDIT, color, FALSE);
           }

           InvalidateRect(GetDlgItem(hDlg, IDCOLOR), (LPRECT) NULL, FALSE);
           UpdateWindow(GetDlgItem(hDlg, IDCOLOR));
           bRedraw = TRUE;
           break;

      case WM_MEASUREITEM:
	   lpMeasureItem = (LPMEASUREITEMSTRUCT) lParam;
	   lpMeasureItem->CtlType = ODT_BUTTON;
	   lpMeasureItem->CtlID = (UINT)GetDlgItem(hDlg, IDCOLOR);
	   break;

      case WM_DRAWITEM:
           lpDrawItem = (LPDRAWITEMSTRUCT) lParam;
           red = (BYTE) GetScrollPos(GetDlgItem(hDlg, IDREDBAR), SB_CTL);
           green = (BYTE) GetScrollPos(GetDlgItem(hDlg, IDGREENBAR), SB_CTL);
           blue = (BYTE) GetScrollPos(GetDlgItem(hDlg, IDBLUEBAR), SB_CTL);
           dc = lpDrawItem->hDC;
           solidBrush = CreateSolidBrush(RGB(red,green,blue));
           hOldBrush = SelectObject(dc, solidBrush);
           DrawMonoRect(dc, 0, 0, lpDrawItem->rcItem.right, 
                        lpDrawItem->rcItem.bottom);
           if (hOldBrush)
               SelectObject(dc, hOldBrush);
           DeleteObject(solidBrush);
           break;

      default:
           return FALSE;
   }

   return TRUE;
}
