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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
#include "wine/debug.h"
#include "cderr.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"
#include "cdlg16.h"

static const WCHAR strWineFontData16[] =
                               {'_','_','W','I','N','E','_','F','O','N','T','D','L','G','D','A','T','A','1','6',0};

static void FONT_LogFont16To32W( const LOGFONT16 *font16, LPLOGFONTW font32 )
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
    MultiByteToWideChar(CP_ACP, 0, font16->lfFaceName,
                        LF_FACESIZE, font32->lfFaceName, LF_FACESIZE);
}

static void FONT_Metrics16To32W( const TEXTMETRIC16 *pm16,
                                 NEWTEXTMETRICEXW *pnm32w)
{
    ZeroMemory( pnm32w, sizeof(NEWTEXTMETRICEXW));
    /* NOTE: only the fields used by AddFontStyle() are filled in */
    pnm32w->ntmTm.tmHeight = pm16->tmHeight;
    pnm32w->ntmTm.tmExternalLeading = pm16->tmExternalLeading;
}

static void CFn_CHOOSEFONT16to32W(const CHOOSEFONT16 *chf16, LPCHOOSEFONTW chf32w)
{
  int len;
  if (chf16->Flags & CF_ENABLETEMPLATE)
  {
      LPWSTR name32w;

      len = MultiByteToWideChar( CP_ACP, 0, MapSL(chf16->lpTemplateName), -1, NULL, 0);
      name32w = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
      MultiByteToWideChar( CP_ACP, 0, MapSL(chf16->lpTemplateName), -1, name32w, len);
      chf32w->lpTemplateName = name32w;
  }
  if (chf16->Flags & CF_USESTYLE)
  {
      len = MultiByteToWideChar( CP_ACP, 0, MapSL(chf16->lpszStyle), -1, NULL, 0);
      chf32w->lpszStyle = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
      MultiByteToWideChar( CP_ACP, 0, MapSL(chf16->lpszStyle), -1, chf32w->lpszStyle, len);
  }
  chf32w->lStructSize=sizeof(CHOOSEFONTW);
  chf32w->hwndOwner=HWND_32(chf16->hwndOwner);
  chf32w->hDC=HDC_32(chf16->hDC);
  chf32w->iPointSize=chf16->iPointSize;
  chf32w->Flags=chf16->Flags;
  chf32w->rgbColors=chf16->rgbColors;
  chf32w->lCustData=chf16->lCustData;
  chf32w->lpfnHook=NULL;
  chf32w->hInstance=HINSTANCE_32(chf16->hInstance);
  chf32w->nFontType=chf16->nFontType;
  chf32w->nSizeMax=chf16->nSizeMax;
  chf32w->nSizeMin=chf16->nSizeMin;
  FONT_LogFont16To32W(MapSL(chf16->lpLogFont), chf32w->lpLogFont);
}

/***********************************************************************
 *                          CFn_HookCallChk                 [internal]
 */
static BOOL CFn_HookCallChk(const CHOOSEFONT16 *lpcf)
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
  LPCHOOSEFONT16 lpcf;
  LOGFONT16 *lplf = MapSL( logfont );
  TEXTMETRIC16 *lpmtrx16 = MapSL(metrics);
  ENUMLOGFONTEXW elf32w;
  NEWTEXTMETRICEXW nmtrx32w;

  lpcf = (LPCHOOSEFONT16)GetPropW(hDlg, strWineFontData16);
  FONT_LogFont16To32W(lplf, &(elf32w.elfLogFont));
  FONT_Metrics16To32W(lpmtrx16, &nmtrx32w);
  return AddFontFamily(&elf32w, &nmtrx32w, nFontType,
          (LPCHOOSEFONTW)lpcf->lpTemplateName, hwnd,NULL);
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
  LPCHOOSEFONT16 lpcf;
  LOGFONT16 *lplf = MapSL(logfont);
  TEXTMETRIC16 *lpmtrx16 = MapSL(metrics);
  ENUMLOGFONTEXW elf32w;
  NEWTEXTMETRICEXW nmtrx32w;

  lpcf = (LPCHOOSEFONT16)GetPropW(hDlg, strWineFontData16);
  FONT_LogFont16To32W(lplf, &(elf32w.elfLogFont));
  FONT_Metrics16To32W(lpmtrx16, &nmtrx32w);
  return AddFontStyle(&elf32w, &nmtrx32w, nFontType,
          (LPCHOOSEFONTW)lpcf->lpTemplateName, hcmb2, hcmb3, hDlg, TRUE);
}

/***********************************************************************
 *                        ChooseFont   (COMMDLG.15)
 */
BOOL16 WINAPI ChooseFont16(LPCHOOSEFONT16 lpChFont)
{
    HINSTANCE16 hInst;
    HANDLE16 hDlgTmpl16 = 0;
    HGLOBAL16 hGlobal16 = 0;
    BOOL16 bRet = FALSE;
    LPVOID template;
    FARPROC16 ptr;
    CHOOSEFONTW cf32w;
    LOGFONTW lf32w;
    LOGFONT16 *font16;
    SEGPTR lpTemplateName;

    TRACE("ChooseFont\n");

    if (!lpChFont) return FALSE;

    cf32w.lpLogFont=&lf32w;
    CFn_CHOOSEFONT16to32W(lpChFont, &cf32w);

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
        size = SizeofResource(COMDLG32_hInstance, hResInfo);
        hGlobal16 = GlobalAlloc16(0, size);
        if (!hGlobal16)
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMALLOCFAILURE);
            ERR("alloc failure for %d bytes\n", size);
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
        ConvertDialog32To16(template32, size, template);
        hDlgTmpl16 = hGlobal16;

    }

    /* lpTemplateName is not used in the dialog */
    lpTemplateName=lpChFont->lpTemplateName;
    lpChFont->lpTemplateName=(SEGPTR)&cf32w;

    ptr = GetProcAddress16(GetModuleHandle16("COMMDLG"), (LPCSTR) 16);
    hInst = GetWindowLongPtrA(HWND_32(lpChFont->hwndOwner), GWLP_HINSTANCE);
    bRet = DialogBoxIndirectParam16(hInst, hDlgTmpl16, lpChFont->hwndOwner,
                     (DLGPROC16) ptr, (DWORD)lpChFont);
    if (hGlobal16)
    {
        GlobalUnlock16(hGlobal16);
        GlobalFree16(hGlobal16);
    }
    lpChFont->lpTemplateName=lpTemplateName;

    lpChFont->iPointSize = cf32w.iPointSize;
    lpChFont->Flags = cf32w.Flags;
    lpChFont->rgbColors = cf32w.rgbColors;
    lpChFont->lCustData = cf32w.lCustData;
    lpChFont->nFontType = cf32w.nFontType;

    font16 = MapSL(lpChFont->lpLogFont);
    font16->lfHeight = cf32w.lpLogFont->lfHeight;
    font16->lfWidth = cf32w.lpLogFont->lfWidth;
    font16->lfEscapement = cf32w.lpLogFont->lfEscapement;
    font16->lfOrientation = cf32w.lpLogFont->lfOrientation;
    font16->lfWeight = cf32w.lpLogFont->lfWeight;
    font16->lfItalic = cf32w.lpLogFont->lfItalic;
    font16->lfUnderline = cf32w.lpLogFont->lfUnderline;
    font16->lfStrikeOut = cf32w.lpLogFont->lfStrikeOut;
    font16->lfCharSet = cf32w.lpLogFont->lfCharSet;
    font16->lfOutPrecision = cf32w.lpLogFont->lfOutPrecision;
    font16->lfClipPrecision = cf32w.lpLogFont->lfClipPrecision;
    font16->lfQuality = cf32w.lpLogFont->lfQuality;
    font16->lfPitchAndFamily = cf32w.lpLogFont->lfPitchAndFamily;
    WideCharToMultiByte(CP_ACP, 0, cf32w.lpLogFont->lfFaceName,
                          LF_FACESIZE, font16->lfFaceName, LF_FACESIZE, 0, 0);

    HeapFree(GetProcessHeap(), 0, (LPBYTE)cf32w.lpTemplateName);
    HeapFree(GetProcessHeap(), 0, cf32w.lpszStyle);

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
      lpcf = (LPCHOOSEFONT16)GetPropW(hDlg, strWineFontData16);
      if (!lpcf)
          return FALSE;
      if (CFn_HookCallChk(lpcf))
          res=CallWindowProc16((WNDPROC16)lpcf->lpfnHook,hDlg16,message,wParam,lParam);
      if (res)
          return res;
  }
  else
  {
    lpcf=(LPCHOOSEFONT16)lParam;
    if (!CFn_WMInitDialog(hDlg, wParam, lParam, (LPCHOOSEFONTW)lpcf->lpTemplateName))
    {
      TRACE("CFn_WMInitDialog returned FALSE\n");
      return FALSE;
    }
    SetPropW(hDlg, strWineFontData16, (HANDLE)lParam);
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
            dis.CtlType       = dis16->CtlType;
            dis.CtlID         = dis16->CtlID;
            dis.itemID        = dis16->itemID;
            dis.itemAction    = dis16->itemAction;
            dis.itemState     = dis16->itemState;
            dis.hwndItem      = HWND_32(dis16->hwndItem);
            dis.hDC           = HDC_32(dis16->hDC);
            dis.itemData      = dis16->itemData;
            dis.rcItem.left   = dis16->rcItem.left;
            dis.rcItem.top    = dis16->rcItem.top;
            dis.rcItem.right  = dis16->rcItem.right;
            dis.rcItem.bottom = dis16->rcItem.bottom;
            res = CFn_WMDrawItem(hDlg, wParam, (LPARAM)&dis);
        }
        break;
    case WM_COMMAND:
        res=CFn_WMCommand(hDlg, MAKEWPARAM( wParam, HIWORD(lParam) ), LOWORD(lParam),
                          (LPCHOOSEFONTW)lpcf->lpTemplateName);
        break;
    case WM_DESTROY:
        return TRUE;
    case WM_CHOOSEFONT_GETLOGFONT:
        TRACE("WM_CHOOSEFONT_GETLOGFONT lParam=%08lX\n", lParam);
        FIXME("current logfont back to caller\n");
        break;
    case WM_PAINT:
        res= CFn_WMPaint(hDlg, wParam, lParam, (LPCHOOSEFONTW)lpcf->lpTemplateName);
        break;
    }
  return res;
}
