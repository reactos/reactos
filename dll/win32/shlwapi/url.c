/*
 * Url functions
 *
 * Copyright 2000 Huw D M Davies for CodeWeavers.
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

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "wininet.h"
#include "winreg.h"
#include "winternl.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "intshcut.h"
#include "wine/debug.h"

HMODULE WINAPI MLLoadLibraryW(LPCWSTR,HMODULE,DWORD);
BOOL    WINAPI MLFreeLibrary(HMODULE);
HRESULT WINAPI MLBuildResURLW(LPCWSTR,HMODULE,DWORD,LPCWSTR,LPWSTR,DWORD);

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#ifndef __REACTOS__
/*************************************************************************
 *      SHAutoComplete  	[SHLWAPI.@]
 *
 * Enable auto-completion for an edit control.
 *
 * PARAMS
 *  hwndEdit [I] Handle of control to enable auto-completion for
 *  dwFlags  [I] SHACF_ flags from "shlwapi.h"
 *
 * RETURNS
 *  Success: S_OK. Auto-completion is enabled for the control.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI SHAutoComplete(HWND hwndEdit, DWORD dwFlags)
{
  FIXME("stub\n");
  return S_FALSE;
}
#endif

/*************************************************************************
 *  MLBuildResURLA	[SHLWAPI.405]
 *
 * Create a Url pointing to a resource in a module.
 *
 * PARAMS
 *  lpszLibName [I] Name of the module containing the resource
 *  hMod        [I] Callers module handle
 *  dwFlags     [I] Undocumented flags for loading the module
 *  lpszRes     [I] Resource name
 *  lpszDest    [O] Destination for resulting Url
 *  dwDestLen   [I] Length of lpszDest
 *
 * RETURNS
 *  Success: S_OK. lpszDest contains the resource Url.
 *  Failure: E_INVALIDARG, if any argument is invalid, or
 *           E_FAIL if dwDestLen is too small.
 */
HRESULT WINAPI MLBuildResURLA(LPCSTR lpszLibName, HMODULE hMod, DWORD dwFlags,
                              LPCSTR lpszRes, LPSTR lpszDest, DWORD dwDestLen)
{
  WCHAR szLibName[MAX_PATH], szRes[MAX_PATH], szDest[MAX_PATH];
  HRESULT hRet;

  if (lpszLibName)
    MultiByteToWideChar(CP_ACP, 0, lpszLibName, -1, szLibName, ARRAY_SIZE(szLibName));

  if (lpszRes)
    MultiByteToWideChar(CP_ACP, 0, lpszRes, -1, szRes, ARRAY_SIZE(szRes));

  if (dwDestLen > ARRAY_SIZE(szLibName))
    dwDestLen = ARRAY_SIZE(szLibName);

  hRet = MLBuildResURLW(lpszLibName ? szLibName : NULL, hMod, dwFlags,
                        lpszRes ? szRes : NULL, lpszDest ? szDest : NULL, dwDestLen);
  if (SUCCEEDED(hRet) && lpszDest)
    WideCharToMultiByte(CP_ACP, 0, szDest, -1, lpszDest, dwDestLen, NULL, NULL);

  return hRet;
}

/*************************************************************************
 *  MLBuildResURLA	[SHLWAPI.406]
 *
 * See MLBuildResURLA.
 */
HRESULT WINAPI MLBuildResURLW(LPCWSTR lpszLibName, HMODULE hMod, DWORD dwFlags,
                              LPCWSTR lpszRes, LPWSTR lpszDest, DWORD dwDestLen)
{
  static const WCHAR szRes[] = { 'r','e','s',':','/','/','\0' };
  static const unsigned int szResLen = ARRAY_SIZE(szRes) - 1;
  HRESULT hRet = E_FAIL;

  TRACE("(%s,%p,0x%08lx,%s,%p,%ld)\n", debugstr_w(lpszLibName), hMod, dwFlags,
        debugstr_w(lpszRes), lpszDest, dwDestLen);

  if (!lpszLibName || !hMod || hMod == INVALID_HANDLE_VALUE || !lpszRes ||
      !lpszDest || (dwFlags && dwFlags != 2))
    return E_INVALIDARG;

  if (dwDestLen >= szResLen + 1)
  {
    dwDestLen -= (szResLen + 1);
    memcpy(lpszDest, szRes, sizeof(szRes));

    hMod = MLLoadLibraryW(lpszLibName, hMod, dwFlags);

    if (hMod)
    {
      WCHAR szBuff[MAX_PATH];
      DWORD len;

      len = GetModuleFileNameW(hMod, szBuff, ARRAY_SIZE(szBuff));
      if (len && len < ARRAY_SIZE(szBuff))
      {
        DWORD dwPathLen = lstrlenW(szBuff) + 1;

        if (dwDestLen >= dwPathLen)
        {
          DWORD dwResLen;

          dwDestLen -= dwPathLen;
          memcpy(lpszDest + szResLen, szBuff, dwPathLen * sizeof(WCHAR));

          dwResLen = lstrlenW(lpszRes) + 1;
          if (dwDestLen >= dwResLen + 1)
          {
            lpszDest[szResLen + dwPathLen-1] = '/';
            memcpy(lpszDest + szResLen + dwPathLen, lpszRes, dwResLen * sizeof(WCHAR));
            hRet = S_OK;
          }
        }
      }
      MLFreeLibrary(hMod);
    }
  }
  return hRet;
}
