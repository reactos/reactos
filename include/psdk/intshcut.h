/*
 * Copyright (C) 2007 Francois Gouget
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

#ifndef __WINE_INTSHCUT_H
#define __WINE_INTSHCUT_H

#include <isguids.h>

#define INTSHCUTAPI

#ifdef __cplusplus
extern "C" {
#endif

#define E_FLAGS MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x1000)
#define IS_E_EXEC_FAILED MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x2002)
#define URL_E_INVALID_SYNTAX MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x1001)
#define URL_E_UNREGISTERED_PROTOCOL MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x1002)

typedef enum iurl_seturl_flags {
    IURL_SETURL_FL_GUESS_PROTOCOL=1,
    IURL_SETURL_FL_USE_DEFAULT_PROTOCOL,
    ALL_IURL_SETURL_FLAGS=(IURL_SETURL_FL_GUESS_PROTOCOL|IURL_SETURL_FL_USE_DEFAULT_PROTOCOL)
} IURL_SETURL_FLAGS;

typedef enum iurl_invokecommand_flags {
    IURL_INVOKECOMMAND_FL_ALLOW_UI=1,
    IURL_INVOKECOMMAND_FL_USE_DEFAULT_VERB,
    ALL_IURL_INVOKECOMMAND_FLAGS=(IURL_INVOKECOMMAND_FL_ALLOW_UI|IURL_INVOKECOMMAND_FL_USE_DEFAULT_VERB)
} IURL_INVOKECOMMAND_FLAGS;

typedef struct urlinvokecommandinfoA {
    DWORD dwcbSize;
    DWORD dwFlags;
    HWND hwndParent;
    LPCSTR pcszVerb;
} URLINVOKECOMMANDINFOA, *PURLINVOKECOMMANDINFOA;
typedef const URLINVOKECOMMANDINFOA CURLINVOKECOMMANDINFOA;
typedef const URLINVOKECOMMANDINFOA *PCURLINVOKECOMMANDINFOA;

typedef struct urlinvokecommandinfoW {
    DWORD dwcbSize;
    DWORD dwFlags;
    HWND hwndParent;
    LPCWSTR pcszVerb;
} URLINVOKECOMMANDINFOW, *PURLINVOKECOMMANDINFOW;
typedef const URLINVOKECOMMANDINFOW CURLINVOKECOMMANDINFOW;
typedef const URLINVOKECOMMANDINFOW *PCURLINVOKECOMMANDINFOW;

#define INTERFACE IUniformResourceLocatorA
DECLARE_INTERFACE_(IUniformResourceLocatorA,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, VOID **ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IUniformResourceLocatorA methods ***/
    STDMETHOD(SetURL)(THIS_ LPCSTR pcszURL, DWORD dwInFlags) PURE;
    STDMETHOD(GetURL)(THIS_ LPSTR *ppszURL) PURE;
    STDMETHOD(InvokeCommand)(THIS_ PURLINVOKECOMMANDINFOA pURLCommandInfo) PURE;
};
#undef INTERFACE

#define INTERFACE IUniformResourceLocatorW
DECLARE_INTERFACE_(IUniformResourceLocatorW,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, VOID **ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IUniformResourceLocatorW methods ***/
    STDMETHOD(SetURL)(THIS_ LPCWSTR pcszURL, DWORD dwInFlags) PURE;
    STDMETHOD(GetURL)(THIS_ LPWSTR *ppszURL) PURE;
    STDMETHOD(InvokeCommand)(THIS_ PURLINVOKECOMMANDINFOW pURLCommandInfo) PURE;
};
#undef INTERFACE

DECL_WINELIB_TYPE_AW(URLINVOKECOMMANDINFO)
DECL_WINELIB_TYPE_AW(PURLINVOKECOMMANDINFO)
DECL_WINELIB_TYPE_AW(CURLINVOKECOMMANDINFO)
DECL_WINELIB_TYPE_AW(PCURLINVOKECOMMANDINFO)


typedef enum translateurl_in_flags {
    TRANSLATEURL_FL_GUESS_PROTOCOL=1,
    TRANSLATEURL_FL_USE_DEFAULT_PROTOCOL
} TRANSLATEURL_IN_FLAGS;

HRESULT WINAPI TranslateURLA(LPCSTR, DWORD, LPSTR *);
HRESULT WINAPI TranslateURLW(LPCWSTR, DWORD, LPWSTR *);
#define TranslateURL WINELIB_NAME_AW(TranslateURL)

BOOL    WINAPI InetIsOffline(DWORD);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_INTSHCUT_H */
