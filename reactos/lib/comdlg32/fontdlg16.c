/*
 * COMMDLG - Font Dialog
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

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "commdlg.h"
#include "dlgs.h"
#include "wine/debug.h"
#include "cderr.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"
#include "cdlg16.h"

static void FONT_LogFont16To32A( const LPLOGFONT16 font16, LPLOGFONTA font32 )
{
    font32->lfHeight = font16->lfHeight;
    font32->lfWidth = font16->lfWidth;
    font32->lfEscapement = font16->lfEscapement;
    font32->lfOrientation = font16->lfOrientation;
    font32->lfWeight = font16->lfWeight;
    font32->lfItalic = font16->lfItalic;
    font32->lfUnderline = font16->lfUnderline;
    font32->lfStrikeOut = font16->lfStrikeOut;
    font32->lfCharSet = font16->lfCharSet;
    font32->lfOutPrecision = font16->lfOutPrecision;
    font32->lfClipPrecision = font16->lfClipPrecision;
    font32->lfQuality = font16->lfQuality;
    font32->lfPitchAndFamily = font16->lfPitchAndFamily;
    lstrcpynA( font32->lfFaceName, font16->lfFaceName, LF_FACESIZE );
};

static void FONT_Metrics16To32A( const TEXTMETRIC16 *pm16,
                                 NEWTEXTMETRICEXA *pnm32a)
{
    ZeroMemory( pnm32a, sizeof(NEWTEXTMETRICEXA));
    /* NOTE: only the fields used by AddFontStyle() are filled in */
    pnm32a->ntmTm.tmHeight = pm16->tmHeight;
    pnm32a->ntmTm.tmExternalLeading = pm16->tmExternalLeading;
};

static void CFn_CHOOSEFONT16to32A(LPCHOOSEFONT16 chf16, LPCHOOSEFONTA chf32a)
{
  chf32a->lStructSize=sizeof(CHOOSEFONTA);
  chf32a->hwndOwner=HWND_32(chf16->hwndOwner);
  chf32a->hDC=HDC_32(chf16->hDC);
  chf32a->iPointSize=chf16->iPointSize;
  chf32a->Flags=chf16->Flags;
  chf32a->rgbColors=chf16->rgbColors;
  chf32a->lCustData=chf16->lCustData;
  chf32a->lpfnHook=NULL;
  chf32a->lpTemplateName=MapSL(chf16->lpTemplateName);
  chf32a->hInstance=HINSTANCE_32(chf16->hInstance);
  chf32a->lpszStyle=MapSL(chf16->lpszStyle);
  chf32a->nFontType=chf16->nFontType;
  chf32a->nSizeMax=chf16->nSizeMax;
  chf32a->nSizeMin=chf16->nSizeMin;
  FONT_LogFont16To32A(MapSL(chf16->lpLogFont), chf32a->lpLogFont);
};

/***********************************************************************
 *                          CFn_HookCallChk                 [internal]
 */
static BOOL CFn_HookCallChk(LPCHOOSEFONT16 lpcf)
{
 if (lpcf)
  if(lpcf->Flags & CF_ENABLEHOOK)
   if (lpcf->lpfnHook)
    return TRUE;
 return FALSE;
}

/***********************************************************************
 *                FontFamilyEnumProc                     (COMMDLG.19)
 */
INT16 WINAPI FontFamilyEnumProc16( SEGPTR logfont, SEGPTR metrics,
                                   UINT16 nFontType, LPARAM lParam )
{
  HWND hwnd=HWND_32(LOWORD(lParam));
  HWND hDlg=GetParent(hwnd);
  LPCHOOSEFONT16 lpcf=(LPCHOOSEFONT16)GetWindowLongA(hDlg, DWL_USER);
  LOGFONT16 *lplf = MapSL( logfont );
  TEXTMETRIC16 *lpmtrx16 = MapSL(metrics);
  ENUMLOGFONTEXA elf32a;
  NEWTEXTMETRICEXA nmtrx32a;
  FONT_LogFont16To32A(lplf, &(elf32a.elfLogFont));
  FONT_Metrics16To32A(lpmtrx16, &nmtrx32a);
  return AddFontFamily(&elf32a, &nmtrx32a, nFontType,
          (LPCHOOSEFONTA)lpcf->lpTemplateName, hwnd,NULL);
}

/***********************************************************************
 *                 FontStyleEnumProc                     (COMMDLG.18)
 */
INT16 WINAPI FontStyleEnumProc16( SEGPTR logfont, SEGPTR metrics,
                                  UINT16 nFontType, LPARAM lParam )
{
  HWND hcmb2=HWND_32(LOWORD(lParam));
  HWND hcmb3=HWND_32(HIWORD(lParam));
  HWND hDlg=GetParent(hcmb3);
  LPCHOOSEFONT16 lpcf=(LPCHOOSEFONT16)GetWindowLongA(hDlg, DWL_USER);
  LOGFONT16 *lplf = MapSL(logfont);
  TEXTMETRIC16 *lpmtrx16 = MapSL(metrics);
  ENUMLOGFONTEXA elf32a;
  NEWTEXTMETRICEXA nmtrx32a;
  FONT_LogFont16To32A(lplf, &(elf32a.elfLogFont));
  FONT_Metrics16To32A(lpmtrx16, &nmtrx32a);
  return AddFontStyle(&elf32a, &nmtrx32a, nFontType,
          (LPCHOOSEFONTA)lpcf->lpTemplateName, hcmb2, hcmb3, hDlg, TRUE);
}

/***********************************************************************
 *                        ChooseFont   (COMMDLG.15)
 */
BOOL16 WINAPI ChooseFont16(LPCHOOSEFONT16 lpChFont)
{
    HINSTANCE16 hInst;
    HANDLE16 hDlgTmpl16 = 0, hResource16 = 0;
    HGLOBAL16 hGlobal16 = 0;
    BOOL16 bRet = FALSE;
    LPCVOID template;
    FARPROC16 ptr;
    CHOOSEFONTA cf32a;
    LOGFONTA lf32a;
    LOGFONT16 *font16;
    SEGPTR lpTemplateName;

    cf32a.lpLogFont=&lf32a;
    CFn_CHOOSEFONT16to32A(lpChFont, &cf32a);

    TRACE("ChooseFont\n");
    if (!lpChFont) return FALSE;

    if (TRACE_ON(commdlg))
	_dump_cf_flags(lpChFont->Flags);

    if (lpChFont->Flags & CF_ENABLETEMPLATEHANDLE)
    {
        if (!(template = LockResource16( lpChFont->hInstance )))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
    }
    else if (lpChFont->Flags & CF_ENABLETEMPLATE)
    {
        HANDLE16 hResInfo;
        if (!(hResInfo = FindResource16( lpChFont->hInstance,
                                         MapSL(lpChFont->lpTemplateName),
                                         (LPSTR)RT_DIALOG)))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
            return FALSE;
        }
        if (!(hDlgTmpl16 = LoadResource16( lpChFont->hInstance, hResInfo )) ||
            !(template = LockResource16( hDlgTmpl16 )))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
    }
    else
    {
	HRSRC hResInfo;
	HGLOBAL hDlgTmpl32;
        LPCVOID template32;
        DWORD size;
        if (!(hResInfo = FindResourceA(COMDLG32_hInstance, "CHOOSE_FONT", (LPSTR)RT_DIALOG)))
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
            ERR("global lock failure for %x handle\n", hGlobal16);
            GlobalFree16(hGlobal16);
            return FALSE;
        }
        ConvertDialog32To16((LPVOID)template32, size, (LPVOID)template);
        hDlgTmpl16 = hGlobal16;

    }

    /* lpTemplateName is not used in the dialog */
    lpTemplateName=lpChFont->lpTemplateName;
    lpChFont->lpTemplateName=(SEGPTR)&cf32a;

    ptr = GetProcAddress16(GetModuleHandle16("COMMDLG"), (LPCSTR) 16);
    hInst = GetWindowLongA(HWND_32(lpChFont->hwndOwner), GWL_HINSTANCE);
    bRet = DialogBoxIndirectParam16(hInst, hDlgTmpl16, lpChFont->hwndOwner,
                     (DLGPROC16) ptr, (DWORD)lpChFont);
    if (hResource16) FreeResource16(hDlgTmpl16);
    if (hGlobal16)
    {
        GlobalUnlock16(hGlobal16);
        GlobalFree16(hGlobal16);
    }
    lpChFont->lpTemplateName=lpTemplateName;

    lpChFont->iPointSize = cf32a.iPointSize;
    lpChFont->Flags = cf32a.Flags;
    lpChFont->rgbColors = cf32a.rgbColors;
    lpChFont->lCustData = cf32a.lCustData;
    lpChFont->nFontType = cf32a.nFontType;

    font16 = MapSL(lpChFont->lpLogFont);
    font16->lfHeight = cf32a.lpLogFont->lfHeight;
    font16->lfWidth = cf32a.lpLogFont->lfWidth;
    font16->lfEscapement = cf32a.lpLogFont->lfEscapement;
    font16->lfOrientation = cf32a.lpLogFont->lfOrientation;
    font16->lfWeight = cf32a.lpLogFont->lfWeight;
    font16->lfItalic = cf32a.lpLogFont->lfItalic;
    font16->lfUnderline = cf32a.lpLogFont->lfUnderline;
    font16->lfStrikeOut = cf32a.lpLogFont->lfStrikeOut;
    font16->lfCharSet = cf32a.lpLogFont->lfCharSet;
    font16->lfOutPrecision = cf32a.lpLogFont->lfOutPrecision;
    font16->lfClipPrecision = cf32a.lpLogFont->lfClipPrecision;
    font16->lfQuality = cf32a.lpLogFont->lfQuality;
    font16->lfPitchAndFamily = cf32a.lpLogFont->lfPitchAndFamily;
    lstrcpynA( font16->lfFaceName, cf32a.lpLogFont->lfFaceName, LF_FACESIZE );
    return bRet;
}

/***********************************************************************
 *           FormatCharDlgProc   (COMMDLG.16)
             FIXME: 1. some strings are "hardcoded", but it's better load from sysres
                    2. some CF_.. flags are not supported
                    3. some TType extensions
 */
BOOL16 CALLBACK FormatCharDlgProc16(HWND16 hDlg16, UINT16 message,
				   WPARAM16 wParam, LPARAM lParam)
{
  HWND hDlg = HWND_32(hDlg16);
  LPCHOOSEFONT16 lpcf;
  BOOL16 res=0;
  if (message!=WM_INITDIALOG)
  {
   lpcf=(LPCHOOSEFONT16)GetWindowLongA(hDlg, DWL_USER);
   if (!lpcf && message != WM_MEASUREITEM)
      return FALSE;
   if (CFn_HookCallChk(lpcf))
     res=CallWindowProc16((WNDPROC16)lpcf->lpfnHook,hDlg16,message,wParam,lParam);
   if (res)
    return res;
  }
  else
  {
    lpcf=(LPCHOOSEFONT16)lParam;
    if (!CFn_WMInitDialog(hDlg, wParam, lParam, (LPCHOOSEFONTA)lpcf->lpTemplateName))
    {
      TRACE("CFn_WMInitDialog returned FALSE\n");
      return FALSE;
    }
    if (CFn_HookCallChk(lpcf))
      return CallWindowProc16((WNDPROC16)lpcf->lpfnHook,hDlg16,WM_INITDIALOG,wParam,lParam);
  }
  switch (message)
    {
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT16* mis16 = MapSL(lParam);
            MEASUREITEMSTRUCT mis;
            mis.CtlType    = mis16->CtlType;
            mis.CtlID      = mis16->CtlID;
            mis.itemID     = mis16->itemID;
            mis.itemWidth  = mis16->itemWidth;
            mis.itemHeight = mis16->itemHeight;
            mis.itemData   = mis16->itemData;
            res = CFn_WMMeasureItem(hDlg, wParam, (LPARAM)&mis);
            mis16->itemWidth  = (UINT16)mis.itemWidth;
            mis16->itemHeight = (UINT16)mis.itemHeight;
        }
        break;
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT16* dis16 = MapSL(lParam);
            DRAWITEMSTRUCT dis;
            dis.CtlType    = dis16->CtlType;
            dis.CtlID      = dis16->CtlID;
            dis.itemID     = dis16->itemID;
            dis.itemAction = dis16->itemAction;
            dis.itemState  = dis16->itemState;
            dis.hwndItem   = HWND_32(dis16->hwndItem);
            dis.hDC        = HDC_32(dis16->hDC);
            dis.itemData   = dis16->itemData;
            CONV_RECT16TO32( &dis16->rcItem, &dis.rcItem );
            res = CFn_WMDrawItem(hDlg, wParam, (LPARAM)&dis);
        }
        break;
    case WM_COMMAND:
        res=CFn_WMCommand(hDlg, MAKEWPARAM( wParam, HIWORD(lParam) ), LOWORD(lParam),
                          (LPCHOOSEFONTA)lpcf->lpTemplateName);
        break;
    case WM_DESTROY:
        res=CFn_WMDestroy(hDlg, wParam, lParam);
        break;
    case WM_CHOOSEFONT_GETLOGFONT:
        TRACE("WM_CHOOSEFONT_GETLOGFONT lParam=%08lX\n", lParam);
        FIXME("current logfont back to caller\n");
        break;
    case WM_PAINT:
        res= CFn_WMPaint(hDlg, wParam, lParam, (LPCHOOSEFONTA)lpcf->lpTemplateName);
        break;
    }
  return res;
}
