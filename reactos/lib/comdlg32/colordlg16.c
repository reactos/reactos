/*
 * COMMDLG - Color Dialog
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* BUGS : still seems to not refresh correctly
   sometimes, especially when 2 instances of the
   dialog are loaded at the same time */

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "winuser.h"
#include "commdlg.h"
#include "dlgs.h"
#include "wine/debug.h"
#include "cderr.h"
#include "cdlg.h"
#include "cdlg16.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

/* Chose Color PRIVATE Structure:
 *
 * This is a duplicate of the 32bit code with
 * a extra member
 */
typedef struct CCPRIVATE
{
    LPCHOOSECOLORW lpcc; /* points to public known data structure */
    LPCHOOSECOLOR16 lpcc16; /* save the 16 bits pointer */
    int nextuserdef;     /* next free place in user defined color array */
    HDC hdcMem;          /* color graph used for BitBlt() */
    HBITMAP hbmMem;      /* color graph bitmap */
    RECT fullsize;       /* original dialog window size */
    UINT msetrgb;        /* # of SETRGBSTRING message (today not used)  */
    RECT old3angle;      /* last position of l-marker */
    RECT oldcross;       /* last position of color/satuation marker */
    BOOL updating;       /* to prevent recursive WM_COMMAND/EN_UPDATE processing */
    int h;
    int s;
    int l;               /* for temporary storing of hue,sat,lum */
    int capturedGraph;   /* control mouse captured */
    RECT focusRect;      /* rectangle last focused item */
    HWND hwndFocus;      /* handle last focused item */
} *LCCPRIV;

/***********************************************************************
 *                              CC_WMInitDialog16                  [internal]
 */
LONG CC_WMInitDialog16( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   int i, res;
   int r, g, b;
   HWND hwnd;
   RECT rect;
   POINT point;
   LCCPRIV lpp;
   CHOOSECOLORW *ch32;
   CHOOSECOLOR16 *ch16 = (CHOOSECOLOR16 *) lParam;

   TRACE("WM_INITDIALOG lParam=%08lX\n", lParam);
   lpp = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct CCPRIVATE) );

   if (ch16->lStructSize != sizeof(CHOOSECOLOR16) )
   {
       HeapFree(GetProcessHeap(), 0, lpp);
       EndDialog (hDlg, 0) ;
       return FALSE;
   }
   ch32 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CHOOSECOLORW) );
   lpp->lpcc = ch32;
   lpp->lpcc16 = ch16;
   ch32->lStructSize = sizeof(CHOOSECOLORW);
   ch32->hwndOwner = HWND_32(ch16->hwndOwner);
   /* Should be an HINSTANCE but MS made a typo */
   ch32->hInstance = HWND_32(ch16->hInstance);
   ch32->lpCustColors = MapSL(ch16->lpCustColors);
   ch32->lpfnHook = (LPCCHOOKPROC) ch16->lpfnHook; /* only used as flag */
   ch32->Flags = ch16->Flags;

   SetWindowLongA(hDlg, DWL_USER, (LONG)lpp);

   if (!(lpp->lpcc->Flags & CC_SHOWHELP))
      ShowWindow( GetDlgItem(hDlg,0x40e), SW_HIDE);
   lpp->msetrgb = RegisterWindowMessageA(SETRGBSTRINGA);

#if 0
   cpos = MAKELONG(5,7); /* init */
   if (lpp->lpcc->Flags & CC_RGBINIT)
   {
     for (i = 0; i < 6; i++)
       for (j = 0; j < 8; j++)
        if (predefcolors[i][j] == lpp->lpcc->rgbResult)
        {
          cpos = MAKELONG(i,j);
          goto found;
        }
   }
   found:
   /* FIXME: Draw_a_focus_rect & set_init_values */
#endif

   GetWindowRect(hDlg, &lpp->fullsize);
   if (lpp->lpcc->Flags & CC_FULLOPEN || lpp->lpcc->Flags & CC_PREVENTFULLOPEN)
   {
      hwnd = GetDlgItem(hDlg, 0x2cf);
      EnableWindow(hwnd, FALSE);
   }
   if (!(lpp->lpcc->Flags & CC_FULLOPEN ) || lpp->lpcc->Flags & CC_PREVENTFULLOPEN)
   {
      rect = lpp->fullsize;
      res = rect.bottom - rect.top;
      hwnd = GetDlgItem(hDlg, 0x2c6); /* cut at left border */
      point.x = point.y = 0;
      ClientToScreen(hwnd, &point);
      ScreenToClient(hDlg,&point);
      GetClientRect(hDlg, &rect);
      point.x += GetSystemMetrics(SM_CXDLGFRAME);
      SetWindowPos(hDlg, 0, 0, 0, point.x, res, SWP_NOMOVE|SWP_NOZORDER);

      for (i = 0x2bf; i < 0x2c5; i++)
         ShowWindow( GetDlgItem(hDlg, i), SW_HIDE);
      for (i = 0x2d3; i < 0x2d9; i++)
         ShowWindow( GetDlgItem(hDlg, i), SW_HIDE);
      ShowWindow( GetDlgItem(hDlg, 0x2c9), SW_HIDE);
      ShowWindow( GetDlgItem(hDlg, 0x2c8), SW_HIDE);
      ShowWindow( GetDlgItem(hDlg, 0x2c6), SW_HIDE);
      ShowWindow( GetDlgItem(hDlg, 0x2c5), SW_HIDE);
      ShowWindow( GetDlgItem(hDlg, 1090 ), SW_HIDE);
   }
   else
      CC_SwitchToFullSize(hDlg, lpp->lpcc->rgbResult, NULL);
   res = TRUE;
   for (i = 0x2bf; i < 0x2c5; i++)
     SendMessageA( GetDlgItem(hDlg, i), EM_LIMITTEXT, 3, 0);  /* max 3 digits:  xyz  */
   if (CC_HookCallChk(lpp->lpcc))
   {
          res = CallWindowProc16( (WNDPROC16)lpp->lpcc16->lpfnHook,
				  HWND_16(hDlg), WM_INITDIALOG, wParam, lParam);
   }

   /* Set the initial values of the color chooser dialog */
   r = GetRValue(lpp->lpcc->rgbResult);
   g = GetGValue(lpp->lpcc->rgbResult);
   b = GetBValue(lpp->lpcc->rgbResult);

   CC_PaintSelectedColor(hDlg, lpp->lpcc->rgbResult);
   lpp->h = CC_RGBtoHSL('H', r, g, b);
   lpp->s = CC_RGBtoHSL('S', r, g, b);
   lpp->l = CC_RGBtoHSL('L', r, g, b);

   /* Doing it the long way because CC_EditSetRGB/HSL doesn't seem to work */
   SetDlgItemInt(hDlg, 703, lpp->h, TRUE);
   SetDlgItemInt(hDlg, 704, lpp->s, TRUE);
   SetDlgItemInt(hDlg, 705, lpp->l, TRUE);
   SetDlgItemInt(hDlg, 706, r, TRUE);
   SetDlgItemInt(hDlg, 707, g, TRUE);
   SetDlgItemInt(hDlg, 708, b, TRUE);

   CC_PaintCross(hDlg, lpp->h, lpp->s);
   CC_PaintTriangle(hDlg, lpp->l);

   return res;
}

/***********************************************************************
 *                              CC_WMCommand16                  [internal]
 */
LRESULT CC_WMCommand16( HWND hDlg, WPARAM wParam, LPARAM lParam, WORD notifyCode, HWND hwndCtl )
{
    int  r, g, b, i, xx;
    UINT cokmsg;
    HDC hdc;
    COLORREF *cr;
    LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER);
    TRACE("CC_WMCommand wParam=%x lParam=%lx\n", wParam, lParam);
    switch (wParam)
    {
          case 0x2c2:  /* edit notify RGB */
	  case 0x2c3:
	  case 0x2c4:
	       if (notifyCode == EN_UPDATE && !lpp->updating)
			 {
			   i = CC_CheckDigitsInEdit(hwndCtl, 255);
			   r = GetRValue(lpp->lpcc->rgbResult);
			   g = GetGValue(lpp->lpcc->rgbResult);
			   b= GetBValue(lpp->lpcc->rgbResult);
			   xx = 0;
			   switch (wParam)
			   {
			    case 0x2c2: if ((xx = (i != r))) r = i; break;
			    case 0x2c3: if ((xx = (i != g))) g = i; break;
			    case 0x2c4: if ((xx = (i != b))) b = i; break;
			   }
			   if (xx) /* something has changed */
			   {
			    lpp->lpcc->rgbResult = RGB(r, g, b);
			    CC_PaintSelectedColor(hDlg, lpp->lpcc->rgbResult);
			    lpp->h = CC_RGBtoHSL('H', r, g, b);
			    lpp->s = CC_RGBtoHSL('S', r, g, b);
			    lpp->l = CC_RGBtoHSL('L', r, g, b);
			    CC_EditSetHSL(hDlg, lpp->h, lpp->s, lpp->l);
			    CC_PaintCross(hDlg, lpp->h, lpp->s);
			    CC_PaintTriangle(hDlg, lpp->l);
			   }
			 }
		 break;

	  case 0x2bf:  /* edit notify HSL */
	  case 0x2c0:
	  case 0x2c1:
	       if (notifyCode == EN_UPDATE && !lpp->updating)
			 {
			   i = CC_CheckDigitsInEdit(hwndCtl , wParam == 0x2bf ? 239:240);
			   xx = 0;
			   switch (wParam)
			   {
			    case 0x2bf: if ((xx = ( i != lpp->h))) lpp->h = i; break;
			    case 0x2c0: if ((xx = ( i != lpp->s))) lpp->s = i; break;
			    case 0x2c1: if ((xx = ( i != lpp->l))) lpp->l = i; break;
			   }
			   if (xx) /* something has changed */
			   {
			    r = CC_HSLtoRGB('R', lpp->h, lpp->s, lpp->l);
			    g = CC_HSLtoRGB('G', lpp->h, lpp->s, lpp->l);
			    b = CC_HSLtoRGB('B', lpp->h, lpp->s, lpp->l);
			    lpp->lpcc->rgbResult = RGB(r, g, b);
			    CC_PaintSelectedColor(hDlg, lpp->lpcc->rgbResult);
			    CC_EditSetRGB(hDlg, lpp->lpcc->rgbResult);
			    CC_PaintCross(hDlg, lpp->h, lpp->s);
			    CC_PaintTriangle(hDlg, lpp->l);
			   }
			 }
	       break;

          case 0x2cf:
               CC_SwitchToFullSize(hDlg, lpp->lpcc->rgbResult, &lpp->fullsize);
	       SetFocus( GetDlgItem(hDlg, 0x2bf));
	       break;

          case 0x2c8:    /* add colors ... column by column */
               cr = lpp->lpcc->lpCustColors;
               cr[(lpp->nextuserdef % 2) * 8 + lpp->nextuserdef / 2] = lpp->lpcc->rgbResult;
               if (++lpp->nextuserdef == 16)
		   lpp->nextuserdef = 0;
	       CC_PaintUserColorArray(hDlg, 2, 8, lpp->lpcc->lpCustColors);
	       break;

          case 0x2c9:              /* resulting color */
	       hdc = GetDC(hDlg);
	       lpp->lpcc->rgbResult = GetNearestColor(hdc, lpp->lpcc->rgbResult);
	       ReleaseDC(hDlg, hdc);
	       CC_EditSetRGB(hDlg, lpp->lpcc->rgbResult);
	       CC_PaintSelectedColor(hDlg, lpp->lpcc->rgbResult);
	       r = GetRValue(lpp->lpcc->rgbResult);
	       g = GetGValue(lpp->lpcc->rgbResult);
	       b = GetBValue(lpp->lpcc->rgbResult);
	       lpp->h = CC_RGBtoHSL('H', r, g, b);
	       lpp->s = CC_RGBtoHSL('S', r, g, b);
	       lpp->l = CC_RGBtoHSL('L', r, g, b);
	       CC_EditSetHSL(hDlg, lpp->h, lpp->s, lpp->l);
	       CC_PaintCross(hDlg, lpp->h, lpp->s);
	       CC_PaintTriangle(hDlg, lpp->l);
	       break;

	  case 0x40e:           /* Help! */ /* The Beatles, 1965  ;-) */
	       i = RegisterWindowMessageA(HELPMSGSTRINGA);
               if (lpp->lpcc16)
               {
                   if (lpp->lpcc->hwndOwner)
		       SendMessageA(lpp->lpcc->hwndOwner, i, 0, (LPARAM)lpp->lpcc16);
                   if ( CC_HookCallChk(lpp->lpcc))
		       CallWindowProc16( (WNDPROC16) lpp->lpcc16->lpfnHook,
					 HWND_16(hDlg), WM_COMMAND, psh15,
					 (LPARAM)lpp->lpcc16);
               }
	       break;

          case IDOK :
		cokmsg = RegisterWindowMessageA(COLOROKSTRINGA);
                if (lpp->lpcc16)
                {
		    if (lpp->lpcc->hwndOwner)
			if (SendMessageA(lpp->lpcc->hwndOwner, cokmsg, 0, (LPARAM)lpp->lpcc16))
			   break;    /* do NOT close */
                }
                if (lpp->lpcc16)
                {
                    BYTE *ptr = MapSL(lpp->lpcc16->lpCustColors);
                    memcpy(ptr, lpp->lpcc->lpCustColors, sizeof(COLORREF)*16);
                    lpp->lpcc16->rgbResult = lpp->lpcc->rgbResult;
                }
		EndDialog(hDlg, 1) ;
		return TRUE ;

	  case IDCANCEL :
		EndDialog(hDlg, 0) ;
		return TRUE ;

       }
       return FALSE;
}

/***********************************************************************
 *           ColorDlgProc   (COMMDLG.8)
 */
BOOL16 CALLBACK ColorDlgProc16( HWND16 hDlg16, UINT16 message,
                            WPARAM16 wParam, LONG lParam )
{
    BOOL16 res;
    HWND hDlg = HWND_32(hDlg16);

    LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER);
    if (message != WM_INITDIALOG)
    {
        if (!lpp)
            return FALSE;
        res=0;
        if (CC_HookCallChk(lpp->lpcc))
            res = CallWindowProc16( (WNDPROC16)lpp->lpcc16->lpfnHook, hDlg16, message, wParam, lParam);
        if (res)
            return res;
    }

    /* FIXME: SetRGB message
       if (message && message == msetrgb)
       return HandleSetRGB(hDlg, lParam);
    */

    switch (message)
    {
	  case WM_INITDIALOG:
	                return CC_WMInitDialog16(hDlg, wParam, lParam);
	  case WM_NCDESTROY:
	                DeleteDC(lpp->hdcMem);
	                DeleteObject(lpp->hbmMem);
                        HeapFree(GetProcessHeap(), 0, lpp->lpcc);
                        HeapFree(GetProcessHeap(), 0, lpp);
	                SetWindowLongA(hDlg, DWL_USER, 0L); /* we don't need it anymore */
	                break;
	  case WM_COMMAND:
	                if (CC_WMCommand16(hDlg, wParam, lParam,
					 HIWORD(lParam), HWND_32(LOWORD(lParam))))
	                   return TRUE;
	                break;
	  case WM_PAINT:
	                if (CC_WMPaint(hDlg, wParam, lParam))
	                   return TRUE;
	                break;
	  case WM_LBUTTONDBLCLK:
	                if (CC_MouseCheckResultWindow(hDlg,lParam))
			  return TRUE;
			break;
	  case WM_MOUSEMOVE:
	                if (CC_WMMouseMove(hDlg, lParam))
			  return TRUE;
			break;
	  case WM_LBUTTONUP:  /* FIXME: ClipCursor off (if in color graph)*/
                        if (CC_WMLButtonUp(hDlg, wParam, lParam))
                           return TRUE;
			break;
	  case WM_LBUTTONDOWN:/* FIXME: ClipCursor on  (if in color graph)*/
	                if (CC_WMLButtonDown(hDlg, wParam, lParam))
	                   return TRUE;
	                break;
    }
    return FALSE ;
}

/***********************************************************************
 *            ChooseColor  (COMMDLG.5)
 */
BOOL16 WINAPI ChooseColor16( LPCHOOSECOLOR16 lpChCol )
{
    HINSTANCE16 hInst;
    HANDLE16 hDlgTmpl16 = 0, hResource16 = 0;
    HGLOBAL16 hGlobal16 = 0;
    BOOL16 bRet = FALSE;
    LPCVOID template;
    FARPROC16 ptr;

    TRACE("ChooseColor\n");
    if (!lpChCol) return FALSE;

    if (lpChCol->Flags & CC_ENABLETEMPLATEHANDLE)
        hDlgTmpl16 = lpChCol->hInstance;
    else if (lpChCol->Flags & CC_ENABLETEMPLATE)
    {
        HANDLE16 hResInfo;
        if (!(hResInfo = FindResource16(lpChCol->hInstance,
                                        MapSL(lpChCol->lpTemplateName),
                                        (LPSTR)RT_DIALOG)))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
            return FALSE;
        }
        if (!(hDlgTmpl16 = LoadResource16(lpChCol->hInstance, hResInfo)))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
        hResource16 = hDlgTmpl16;
    }
    else
    {
	HRSRC hResInfo;
	HGLOBAL hDlgTmpl32;
        LPCVOID template32;
        DWORD size;
	if (!(hResInfo = FindResourceA(COMDLG32_hInstance, "CHOOSE_COLOR", (LPSTR)RT_DIALOG)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	    return FALSE;
	}
	if (!(hDlgTmpl32 = LoadResource(COMDLG32_hInstance, hResInfo)) ||
	    !(template32 = LockResource(hDlgTmpl32)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
        size = SizeofResource(GetModuleHandleA("COMDLG32"), hResInfo);
        hGlobal16 = GlobalAlloc16(0, size);
        if (!hGlobal16)
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMALLOCFAILURE);
            ERR("alloc failure for %ld bytes\n", size);
            return FALSE;
        }
        template = GlobalLock16(hGlobal16);
        if (!template)
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMLOCKFAILURE);
            ERR("global lock failure for %x handle\n", hDlgTmpl16);
            GlobalFree16(hGlobal16);
            return FALSE;
        }
        ConvertDialog32To16((LPVOID)template32, size, (LPVOID)template);
        hDlgTmpl16 = hGlobal16;
    }

    ptr = GetProcAddress16(GetModuleHandle16("COMMDLG"), (LPCSTR) 8);
    hInst = GetWindowLongA(HWND_32(lpChCol->hwndOwner), GWL_HINSTANCE);
    bRet = DialogBoxIndirectParam16(hInst, hDlgTmpl16, lpChCol->hwndOwner,
                     (DLGPROC16) ptr, (DWORD)lpChCol);
    if (hResource16) FreeResource16(hDlgTmpl16);
    if (hGlobal16)
    {
        GlobalUnlock16(hGlobal16);
        GlobalFree16(hGlobal16);
    }
    return bRet;
}
