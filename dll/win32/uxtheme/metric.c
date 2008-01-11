/*
 * Win32 5.1 Theme metrics
 *
 * Copyright (C) 2003 Kevin Koltzau
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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "vfwmsgs.h"
#include "uxtheme.h"
#include "tmschema.h"

#include "msstyles.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);

/***********************************************************************
 *      GetThemeSysBool                                     (UXTHEME.@)
 */
BOOL WINAPI GetThemeSysBool(HTHEME hTheme, int iBoolID)
{
    HRESULT hr;
    PTHEME_PROPERTY tp;
    BOOL ret;

    TRACE("(%p, %d)\n", hTheme, iBoolID);
    SetLastError(0);
    if(hTheme) {
        if((tp = MSSTYLES_FindMetric(TMT_BOOL, iBoolID))) {
            hr = MSSTYLES_GetPropertyBool(tp, &ret);
            if(SUCCEEDED(hr))
                return ret;
            else
                SetLastError(hr);
       }
    }
    if(iBoolID == TMT_FLATMENUS) {
        if(SystemParametersInfoW(SPI_GETFLATMENU, 0, &ret, 0))
            return ret;
    }
    else {
        FIXME("Unknown bool id: %d\n", iBoolID);
        SetLastError(STG_E_INVALIDPARAMETER);
    }
    return FALSE;
}

/***********************************************************************
 *      GetThemeSysColor                                    (UXTHEME.@)
 */
COLORREF WINAPI GetThemeSysColor(HTHEME hTheme, int iColorID)
{
    HRESULT hr;
    PTHEME_PROPERTY tp;

    TRACE("(%p, %d)\n", hTheme, iColorID);
    SetLastError(0);
    if(hTheme) {
        if((tp = MSSTYLES_FindMetric(TMT_COLOR, iColorID))) {
            COLORREF color;
            hr = MSSTYLES_GetPropertyColor(tp, &color);
            if(SUCCEEDED(hr))
                return color;
            else
                SetLastError(hr);
       }
    }
    return GetSysColor(iColorID - TMT_FIRSTCOLOR);
}

/***********************************************************************
 *      GetThemeSysColorBrush                               (UXTHEME.@)
 */
HBRUSH WINAPI GetThemeSysColorBrush(HTHEME hTheme, int iColorID)
{
    TRACE("(%p, %d)\n", hTheme, iColorID);
    return CreateSolidBrush(GetThemeSysColor(hTheme, iColorID));
}

/***********************************************************************
 *      GetThemeSysFont                                     (UXTHEME.@)
 */
HRESULT WINAPI GetThemeSysFont(HTHEME hTheme, int iFontID, LOGFONTW *plf)
{
    HRESULT hr = S_OK;
    PTHEME_PROPERTY tp;

    TRACE("(%p, %d)\n", hTheme, iFontID);
    if(hTheme) {
        if((tp = MSSTYLES_FindMetric(TMT_FONT, iFontID))) {
            HDC hdc = GetDC(NULL);
            hr = MSSTYLES_GetPropertyFont(tp, hdc, plf);
            ReleaseDC(NULL, hdc);
            if(SUCCEEDED(hr))
                return S_OK;
       }
    }
    if(iFontID == TMT_ICONTITLEFONT) {
        if(!SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(LOGFONTW), &plf, 0))
            return HRESULT_FROM_WIN32(GetLastError());
    }
    else {
        NONCLIENTMETRICSW ncm;
        LOGFONTW *font = NULL;
        ncm.cbSize = sizeof(NONCLIENTMETRICSW);
        if(!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0))
            return HRESULT_FROM_WIN32(GetLastError());
        switch(iFontID) {
            case TMT_CAPTIONFONT: font = &ncm.lfCaptionFont; break;
            case TMT_SMALLCAPTIONFONT: font = &ncm.lfSmCaptionFont; break;
            case TMT_MENUFONT: font = &ncm.lfMenuFont; break;
            case TMT_STATUSFONT: font = &ncm.lfStatusFont; break;
            case TMT_MSGBOXFONT: font = &ncm.lfMessageFont; break;
            default: FIXME("Unknown FontID: %d\n", iFontID); break;
        }
        if(font) CopyMemory(plf, font, sizeof(LOGFONTW));
        else     hr = STG_E_INVALIDPARAMETER;
    }
    return hr;
}

/***********************************************************************
 *      GetThemeSysInt                                      (UXTHEME.@)
 */
HRESULT WINAPI GetThemeSysInt(HTHEME hTheme, int iIntID, int *piValue)
{
    PTHEME_PROPERTY tp;

    TRACE("(%p, %d)\n", hTheme, iIntID);
    if(!hTheme)
        return E_HANDLE;
    if(iIntID < TMT_FIRSTINT || iIntID > TMT_LASTINT) {
        WARN("Unknown IntID: %d\n", iIntID);
        return STG_E_INVALIDPARAMETER;
    }
    if((tp = MSSTYLES_FindMetric(TMT_INT, iIntID)))
        return MSSTYLES_GetPropertyInt(tp, piValue);
    return E_PROP_ID_UNSUPPORTED;
}

/***********************************************************************
 *      GetThemeSysSize                                     (UXTHEME.@)
 */
int WINAPI GetThemeSysSize(HTHEME hTheme, int iSizeID)
{
    PTHEME_PROPERTY tp;
    int i, id = -1;
    int metricMap[] = {
        SM_CXVSCROLL, TMT_SCROLLBARWIDTH,
        SM_CYHSCROLL, TMT_SCROLLBARHEIGHT,
        SM_CXSIZE, TMT_CAPTIONBARWIDTH,
        SM_CYSIZE, TMT_CAPTIONBARHEIGHT,
        SM_CXFRAME, TMT_SIZINGBORDERWIDTH,
        SM_CYFRAME, TMT_SIZINGBORDERWIDTH, /* There is no TMT_SIZINGBORDERHEIGHT, but this works in windows.. */
        SM_CXSMSIZE, TMT_SMCAPTIONBARWIDTH,
        SM_CYSMSIZE, TMT_SMCAPTIONBARHEIGHT,
        SM_CXMENUSIZE, TMT_MENUBARWIDTH,
        SM_CYMENUSIZE, TMT_MENUBARHEIGHT
    };

    if(hTheme) {
        for(i=0; i<sizeof(metricMap)/sizeof(metricMap[0]); i+=2) {
            if(metricMap[i] == iSizeID) {
                id = metricMap[i+1];
                break;
            }
        }
        SetLastError(0);
        if(id != -1) {
            if((tp = MSSTYLES_FindMetric(TMT_SIZE, id))) {
                if(SUCCEEDED(MSSTYLES_GetPropertyInt(tp, &i))) {
                    return i;
                }
            }
            TRACE("Size %d not found in theme, using system metric\n", iSizeID);
        }
        else {
            SetLastError(STG_E_INVALIDPARAMETER);
            return 0;
        }
    }
    return GetSystemMetrics(iSizeID);
}

/***********************************************************************
 *      GetThemeSysString                                   (UXTHEME.@)
 */
HRESULT WINAPI GetThemeSysString(HTHEME hTheme, int iStringID,
                                 LPWSTR pszStringBuff, int cchMaxStringChars)
{
    PTHEME_PROPERTY tp;

    TRACE("(%p, %d)\n", hTheme, iStringID);
    if(!hTheme)
        return E_HANDLE;
    if(iStringID < TMT_FIRSTSTRING || iStringID > TMT_LASTSTRING) {
        WARN("Unknown StringID: %d\n", iStringID);
        return STG_E_INVALIDPARAMETER;
    }
    if((tp = MSSTYLES_FindMetric(TMT_STRING, iStringID)))
        return MSSTYLES_GetPropertyString(tp, pszStringBuff, cchMaxStringChars);
    return E_PROP_ID_UNSUPPORTED;
}
