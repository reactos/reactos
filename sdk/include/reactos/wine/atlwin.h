/*
 * Active Template Library Window Functions
 *
 * Copyright 2006 Robert Shearman for CodeWeavers
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

#ifndef __WINE_ATLWIN_H__
#define __WINE_ATLWIN_H__

typedef struct _ATL_WNDCLASSINFOA_TAG
{
    WNDCLASSEXA m_wc;
    LPCSTR m_lpszOrigName;
    WNDPROC pWndProc;
    LPCSTR m_lpszCursorID;
    BOOL m_bSystemCursor;
    ATOM m_atom;
    CHAR m_szAutoName[sizeof("ATL:") + sizeof(void *) * 2];
} _ATL_WNDCLASSINFOA;

typedef struct _ATL_WNDCLASSINFOW_TAG
{
    WNDCLASSEXW m_wc;
    LPCWSTR m_lpszOrigName;
    WNDPROC pWndProc;
    LPCWSTR m_lpszCursorID;
    BOOL m_bSystemCursor;
    ATOM m_atom;
    WCHAR m_szAutoName[sizeof("ATL:") + sizeof(void *) * 2];
} _ATL_WNDCLASSINFOW;

ATOM WINAPI AtlModuleRegisterWndClassInfoA(_ATL_MODULEA *pm, _ATL_WNDCLASSINFOA *wci, WNDPROC *pProc);
ATOM WINAPI AtlModuleRegisterWndClassInfoW(_ATL_MODULEW *pm, _ATL_WNDCLASSINFOW *wci, WNDPROC *pProc);

HDC WINAPI AtlCreateTargetDC(HDC hdc, DVTARGETDEVICE *ptd);
void WINAPI AtlHiMetricToPixel(const SIZEL *lpSizeInHiMetric, LPSIZEL lpSizeInPix);
void WINAPI AtlPixelToHiMetric(const SIZEL *lpSizeInPix, LPSIZEL lpSizeInHiMetric);

#endif /* __WINE_ATLWIN_H__ */
