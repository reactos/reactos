/* $Id: font.h,v 1.1 2004/03/23 00:18:54 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            include/win32k/font.h
 * PURPOSE:         GDI32/Win32k font interface
 * PROGRAMMER:
 *
 */

#ifndef WIN32K_FONT_H_INCLUDED
#define WIN32K_FONT_H_INCLUDED

#include <windows.h>

typedef struct tagFONTFAMILYINFO
{
  ENUMLOGFONTEXW EnumLogFontEx;
  NEWTEXTMETRICEXW NewTextMetricEx;
  DWORD FontType;
} FONTFAMILYINFO, *PFONTFAMILYINFO;

int STDCALL NtGdiGetFontFamilyInfo(HDC Dc, LPLOGFONTW LogFont, PFONTFAMILYINFO Info, DWORD Size);
BOOL STDCALL NtGdiTranslateCharsetInfo(PDWORD Src, LPCHARSETINFO CSI, DWORD Flags);

#endif /* WIN32K_FONT_H_INCLUDED */
