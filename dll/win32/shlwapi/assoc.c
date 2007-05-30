/*
 * IQueryAssociations object and helper functions
 *
 * Copyright 2002 Jon Griffiths
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
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "objbase.h"
#include "shlguid.h"
#include "shlwapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/**************************************************************************
 *  IQueryAssociations {SHLWAPI}
 *
 * DESCRIPTION
 *  This object provides a layer of abstraction over the system registry in
 *  order to simplify the process of parsing associations between files.
 *  Associations in this context means the registry entries that link (for
 *  example) the extension of a file with its description, list of
 *  applications to open the file with, and actions that can be performed on it
 *  (the shell displays such information in the context menu of explorer
 *  when you right-click on a file).
 *
 * HELPERS
 * You can use this object transparently by calling the helper functions
 * AssocQueryKeyA(), AssocQueryStringA() and AssocQueryStringByKeyA(). These
 * create an IQueryAssociations object, perform the requested actions
 * and then dispose of the object. Alternatively, you can create an instance
 * of the object using AssocCreate() and call the following methods on it:
 *
 * METHODS
 */

/* Default IQueryAssociations::Init() flags */
#define SHLWAPI_DEF_ASSOCF (ASSOCF_INIT_BYEXENAME|ASSOCF_INIT_DEFAULTTOSTAR| \
                            ASSOCF_INIT_DEFAULTTOFOLDER)

typedef struct
{
  const IQueryAssociationsVtbl *lpVtbl;
  LONG ref;
  HKEY hkeySource;
  HKEY hkeyProgID;
} IQueryAssociationsImpl;

static const IQueryAssociationsVtbl IQueryAssociations_vtbl;

/**************************************************************************
 *  IQueryAssociations_Constructor [internal]
 *
 * Construct a new IQueryAssociations object.
 */
static IQueryAssociations* IQueryAssociations_Constructor(void)
{
  IQueryAssociationsImpl* iface;

  iface = HeapAlloc(GetProcessHeap(),0,sizeof(IQueryAssociationsImpl));
  iface->lpVtbl = &IQueryAssociations_vtbl;
  iface->ref = 1;
  iface->hkeySource = NULL;
  iface->hkeyProgID = NULL;

  TRACE("Returning IQueryAssociations* %p\n", iface);
  return (IQueryAssociations*)iface;
}

/*************************************************************************
 * SHLWAPI_ParamAToW
 *
 * Internal helper function: Convert ASCII parameter to Unicode.
 */
static BOOL SHLWAPI_ParamAToW(LPCSTR lpszParam, LPWSTR lpszBuff, DWORD dwLen,
                              LPWSTR* lpszOut)
{
  if (lpszParam)
  {
    DWORD dwStrLen = MultiByteToWideChar(CP_ACP, 0, lpszParam, -1, NULL, 0);

    if (dwStrLen < dwLen)
    {
      *lpszOut = lpszBuff; /* Use Buffer, it is big enough */
    }
    else
    {
      /* Create a new buffer big enough for the string */
      *lpszOut = HeapAlloc(GetProcessHeap(), 0,
                                   dwStrLen * sizeof(WCHAR));
      if (!*lpszOut)
        return FALSE;
    }
    MultiByteToWideChar(CP_ACP, 0, lpszParam, -1, *lpszOut, dwStrLen);
  }
  else
    *lpszOut = NULL;
  return TRUE;
}

/*************************************************************************
 * AssocCreate  [SHLWAPI.@]
 *
 * Create a new IQueryAssociations object.
 *
 * PARAMS
 *  clsid       [I] CLSID of object
 *  refiid      [I] REFIID of interface
 *  lpInterface [O] Destination for the created IQueryAssociations object
 *
 * RETURNS
 *  Success: S_OK. lpInterface contains the new object.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  refiid must be equal to IID_IQueryAssociations, or this function will fail.
 */
HRESULT WINAPI AssocCreate(CLSID clsid, REFIID refiid, void **lpInterface)
{
  HRESULT hRet;
  IQueryAssociations* lpAssoc;

  TRACE("(%s,%s,%p)\n", debugstr_guid(&clsid), debugstr_guid(refiid),
        lpInterface);

  if (!lpInterface)
    return E_INVALIDARG;

  *(DWORD*)lpInterface = 0;

  if (!IsEqualGUID(&clsid, &IID_IQueryAssociations))
    return E_NOTIMPL;

  lpAssoc = IQueryAssociations_Constructor();

  if (!lpAssoc)
    return E_OUTOFMEMORY;

  hRet = IQueryAssociations_QueryInterface(lpAssoc, refiid, lpInterface);
  IQueryAssociations_Release(lpAssoc);
  return hRet;
}

/*************************************************************************
 * AssocQueryKeyW  [SHLWAPI.@]
 *
 * See AssocQueryKeyA.
 */
HRESULT WINAPI AssocQueryKeyW(ASSOCF cfFlags, ASSOCKEY assockey, LPCWSTR pszAssoc,
                              LPCWSTR pszExtra, HKEY *phkeyOut)
{
  HRESULT hRet;
  IQueryAssociations* lpAssoc;

  TRACE("(0x%8lx,0x%8x,%s,%s,%p)\n", cfFlags, assockey, debugstr_w(pszAssoc),
        debugstr_w(pszExtra), phkeyOut);

  lpAssoc = IQueryAssociations_Constructor();

  if (!lpAssoc)
    return E_OUTOFMEMORY;

  cfFlags &= SHLWAPI_DEF_ASSOCF;
  hRet = IQueryAssociations_Init(lpAssoc, cfFlags, pszAssoc, NULL, NULL);

  if (SUCCEEDED(hRet))
    hRet = IQueryAssociations_GetKey(lpAssoc, cfFlags, assockey, pszExtra, phkeyOut);

  IQueryAssociations_Release(lpAssoc);
  return hRet;
}

/*************************************************************************
 * AssocQueryKeyA  [SHLWAPI.@]
 *
 * Get a file association key from the registry.
 *
 * PARAMS
 *  cfFlags  [I] ASSOCF_ flags from "shlwapi.h"
 *  assockey [I] Type of key to get
 *  pszAssoc [I] Key name to search below
 *  pszExtra [I] Extra information about the key location
 *  phkeyOut [O] Destination for the association key
 *
 * RETURNS
 *  Success: S_OK. phkeyOut contains the key.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI AssocQueryKeyA(ASSOCF cfFlags, ASSOCKEY assockey, LPCSTR pszAssoc,
                              LPCSTR pszExtra, HKEY *phkeyOut)
{
  WCHAR szAssocW[MAX_PATH], *lpszAssocW = NULL;
  WCHAR szExtraW[MAX_PATH], *lpszExtraW = NULL;
  HRESULT hRet = E_OUTOFMEMORY;

  TRACE("(0x%8lx,0x%8x,%s,%s,%p)\n", cfFlags, assockey, debugstr_a(pszAssoc),
        debugstr_a(pszExtra), phkeyOut);

  if (SHLWAPI_ParamAToW(pszAssoc, szAssocW, MAX_PATH, &lpszAssocW) &&
      SHLWAPI_ParamAToW(pszExtra, szExtraW, MAX_PATH, &lpszExtraW))
  {
    hRet = AssocQueryKeyW(cfFlags, assockey, lpszAssocW, lpszExtraW, phkeyOut);
  }

  if (lpszAssocW && lpszAssocW != szAssocW)
    HeapFree(GetProcessHeap(), 0, lpszAssocW);

  if (lpszExtraW && lpszExtraW != szExtraW)
    HeapFree(GetProcessHeap(), 0, lpszExtraW);

  return hRet;
}

/*************************************************************************
 * AssocQueryStringW  [SHLWAPI.@]
 *
 * See AssocQueryStringA.
 */
HRESULT WINAPI AssocQueryStringW(ASSOCF cfFlags, ASSOCSTR str, LPCWSTR pszAssoc,
                                 LPCWSTR pszExtra, LPWSTR pszOut, DWORD *pcchOut)
{
  HRESULT hRet;
  IQueryAssociations* lpAssoc;

  TRACE("(0x%8lx,0x%8x,%s,%s,%p,%p)\n", cfFlags, str, debugstr_w(pszAssoc),
        debugstr_w(pszExtra), pszOut, pcchOut);

  if (!pcchOut)
    return E_INVALIDARG;

  lpAssoc = IQueryAssociations_Constructor();

  if (!lpAssoc)
    return E_OUTOFMEMORY;

  hRet = IQueryAssociations_Init(lpAssoc, cfFlags & SHLWAPI_DEF_ASSOCF,
                                 pszAssoc, NULL, NULL);

  if (SUCCEEDED(hRet))
    hRet = IQueryAssociations_GetString(lpAssoc, cfFlags, str, pszExtra,
                                        pszOut, pcchOut);

  IQueryAssociations_Release(lpAssoc);
  return hRet;
}

/*************************************************************************
 * AssocQueryStringA  [SHLWAPI.@]
 *
 * Get a file association string from the registry.
 *
 * PARAMS
 *  cfFlags  [I] ASSOCF_ flags from "shlwapi.h"
 *  str      [I] Type of string to get (ASSOCSTR enum from "shlwapi.h")
 *  pszAssoc [I] Key name to search below
 *  pszExtra [I] Extra information about the string location
 *  pszOut   [O] Destination for the association string
 *  pcchOut  [O] Length of pszOut
 *
 * RETURNS
 *  Success: S_OK. pszOut contains the string, pcchOut contains its length.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI AssocQueryStringA(ASSOCF cfFlags, ASSOCSTR str, LPCSTR pszAssoc,
                                 LPCSTR pszExtra, LPSTR pszOut, DWORD *pcchOut)
{
  WCHAR szAssocW[MAX_PATH], *lpszAssocW = NULL;
  WCHAR szExtraW[MAX_PATH], *lpszExtraW = NULL;
  HRESULT hRet = E_OUTOFMEMORY;

  TRACE("(0x%8lx,0x%8x,%s,%s,%p,%p)\n", cfFlags, str, debugstr_a(pszAssoc),
        debugstr_a(pszExtra), pszOut, pcchOut);

  if (!pcchOut)
    hRet = E_INVALIDARG;
  else if (SHLWAPI_ParamAToW(pszAssoc, szAssocW, MAX_PATH, &lpszAssocW) &&
           SHLWAPI_ParamAToW(pszExtra, szExtraW, MAX_PATH, &lpszExtraW))
  {
    WCHAR szReturnW[MAX_PATH], *lpszReturnW = szReturnW;
    DWORD dwLenOut = *pcchOut;

    if (dwLenOut >= MAX_PATH)
      lpszReturnW = HeapAlloc(GetProcessHeap(), 0,
                                      (dwLenOut + 1) * sizeof(WCHAR));

    if (!lpszReturnW)
      hRet = E_OUTOFMEMORY;
    else
    {
      hRet = AssocQueryStringW(cfFlags, str, lpszAssocW, lpszExtraW,
                               lpszReturnW, &dwLenOut);

      if (SUCCEEDED(hRet))
        WideCharToMultiByte(CP_ACP,0,szReturnW,-1,pszOut,dwLenOut,0,0);
      *pcchOut = dwLenOut;

      if (lpszReturnW && lpszReturnW != szReturnW)
        HeapFree(GetProcessHeap(), 0, lpszReturnW);
    }
  }

  if (lpszAssocW && lpszAssocW != szAssocW)
    HeapFree(GetProcessHeap(), 0, lpszAssocW);
  if (lpszExtraW && lpszExtraW != szExtraW)
    HeapFree(GetProcessHeap(), 0, lpszExtraW);
  return hRet;
}

/*************************************************************************
 * AssocQueryStringByKeyW  [SHLWAPI.@]
 *
 * See AssocQueryStringByKeyA.
 */
HRESULT WINAPI AssocQueryStringByKeyW(ASSOCF cfFlags, ASSOCSTR str, HKEY hkAssoc,
                                      LPCWSTR pszExtra, LPWSTR pszOut,
                                      DWORD *pcchOut)
{
  HRESULT hRet;
  IQueryAssociations* lpAssoc;

  TRACE("(0x%8lx,0x%8x,%p,%s,%p,%p)\n", cfFlags, str, hkAssoc,
        debugstr_w(pszExtra), pszOut, pcchOut);

  lpAssoc = IQueryAssociations_Constructor();

  if (!lpAssoc)
    return E_OUTOFMEMORY;

  cfFlags &= SHLWAPI_DEF_ASSOCF;
  hRet = IQueryAssociations_Init(lpAssoc, cfFlags, 0, hkAssoc, NULL);

  if (SUCCEEDED(hRet))
    hRet = IQueryAssociations_GetString(lpAssoc, cfFlags, str, pszExtra,
                                        pszOut, pcchOut);

  IQueryAssociations_Release(lpAssoc);
  return hRet;
}

/*************************************************************************
 * AssocQueryStringByKeyA  [SHLWAPI.@]
 *
 * Get a file association string from the registry, given a starting key.
 *
 * PARAMS
 *  cfFlags  [I] ASSOCF_ flags from "shlwapi.h"
 *  str      [I] Type of string to get
 *  hkAssoc  [I] Key to search below
 *  pszExtra [I] Extra information about the string location
 *  pszOut   [O] Destination for the association string
 *  pcchOut  [O] Length of pszOut
 *
 * RETURNS
 *  Success: S_OK. pszOut contains the string, pcchOut contains its length.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI AssocQueryStringByKeyA(ASSOCF cfFlags, ASSOCSTR str, HKEY hkAssoc,
                                      LPCSTR pszExtra, LPSTR pszOut,
                                      DWORD *pcchOut)
{
  WCHAR szExtraW[MAX_PATH], *lpszExtraW;
  WCHAR szReturnW[MAX_PATH], *lpszReturnW = szReturnW;
  HRESULT hRet = E_OUTOFMEMORY;

  TRACE("(0x%8lx,0x%8x,%p,%s,%p,%p)\n", cfFlags, str, hkAssoc,
        debugstr_a(pszExtra), pszOut, pcchOut);

  if (!pcchOut)
    hRet = E_INVALIDARG;
  else if (SHLWAPI_ParamAToW(pszExtra, szExtraW, MAX_PATH, &lpszExtraW))
  {
    DWORD dwLenOut = *pcchOut;
    if (dwLenOut >= MAX_PATH)
      lpszReturnW = HeapAlloc(GetProcessHeap(), 0,
                                      (dwLenOut + 1) * sizeof(WCHAR));

    if (lpszReturnW)
    {
      hRet = AssocQueryStringByKeyW(cfFlags, str, hkAssoc, lpszExtraW,
                                    lpszReturnW, &dwLenOut);

      if (SUCCEEDED(hRet))
        WideCharToMultiByte(CP_ACP,0,szReturnW,-1,pszOut,dwLenOut,0,0);
      *pcchOut = dwLenOut;

      if (lpszReturnW != szReturnW)
        HeapFree(GetProcessHeap(), 0, lpszReturnW);
    }
  }

  if (lpszExtraW && lpszExtraW != szExtraW)
    HeapFree(GetProcessHeap(), 0, lpszExtraW);
  return hRet;
}


/**************************************************************************
 *  AssocIsDangerous  (SHLWAPI.@)
 *  
 * Determine if a file association is dangerous (potentially malware).
 *
 * PARAMS
 *  lpszAssoc [I] Name of file or file extension to check.
 *
 * RETURNS
 *  TRUE, if lpszAssoc may potentially be malware (executable),
 *  FALSE, Otherwise.
 */
BOOL WINAPI AssocIsDangerous(LPCWSTR lpszAssoc)
{
    FIXME("%s\n", debugstr_w(lpszAssoc));
    return FALSE;
}

/**************************************************************************
 *  IQueryAssociations_QueryInterface {SHLWAPI}
 *
 * See IUnknown_QueryInterface.
 */
static HRESULT WINAPI IQueryAssociations_fnQueryInterface(
  IQueryAssociations* iface,
  REFIID riid,
  LPVOID *ppvObj)
{
  IQueryAssociationsImpl *This = (IQueryAssociationsImpl *)iface;

  TRACE("(%p,%s,%p)\n",This, debugstr_guid(riid), ppvObj);

  *ppvObj = NULL;

  if (IsEqualIID(riid, &IID_IUnknown) ||
      IsEqualIID(riid, &IID_IQueryAssociations))
  {
    *ppvObj = (IQueryAssociations*)This;

    IQueryAssociations_AddRef((IQueryAssociations*)*ppvObj);
    TRACE("Returning IQueryAssociations (%p)\n", *ppvObj);
    return S_OK;
  }
  TRACE("Returning E_NOINTERFACE\n");
  return E_NOINTERFACE;
}

/**************************************************************************
 *  IQueryAssociations_AddRef {SHLWAPI}
 *
 * See IUnknown_AddRef.
 */
static ULONG WINAPI IQueryAssociations_fnAddRef(IQueryAssociations *iface)
{
  IQueryAssociationsImpl *This = (IQueryAssociationsImpl *)iface;
  ULONG refCount = InterlockedIncrement(&This->ref);
  
  TRACE("(%p)->(ref before=%lu)\n",This, refCount - 1);

  return refCount;
}

/**************************************************************************
 *  IQueryAssociations_Release {SHLWAPI}
 *
 * See IUnknown_Release.
 */
static ULONG WINAPI IQueryAssociations_fnRelease(IQueryAssociations *iface)
{
  IQueryAssociationsImpl *This = (IQueryAssociationsImpl *)iface;
  ULONG refCount = InterlockedDecrement(&This->ref);

  TRACE("(%p)->(ref before=%lu)\n",This, refCount + 1);

  if (!refCount)
  {
    TRACE("Destroying IQueryAssociations (%p)\n", This);
    HeapFree(GetProcessHeap(), 0, This);
  }
  
  return refCount;
}

/**************************************************************************
 *  IQueryAssociations_Init {SHLWAPI}
 *
 * Initialise an IQueryAssociations object.
 *
 * PARAMS
 *  iface      [I] IQueryAssociations interface to initialise
 *  cfFlags    [I] ASSOCF_ flags from "shlwapi.h"
 *  pszAssoc   [I] String for the root key name, or NULL if hkProgid is given
 *  hkeyProgid [I] Handle for the root key, or NULL if pszAssoc is given
 *  hWnd       [I] Reserved, must be NULL.
 *
 * RETURNS
 *  Success: S_OK. iface is initialised with the parameters given.
 *  Failure: An HRESULT error code indicating the error.
 */
static HRESULT WINAPI IQueryAssociations_fnInit(
  IQueryAssociations *iface,
  ASSOCF cfFlags,
  LPCWSTR pszAssoc,
  HKEY hkeyProgid,
  HWND hWnd)
{
    static const WCHAR szProgID[] = {'P','r','o','g','I','D',0};
    IQueryAssociationsImpl *This = (IQueryAssociationsImpl *)iface;
    HRESULT hr;

    TRACE("(%p)->(%ld,%s,%p,%p)\n", iface,
                                    cfFlags,
                                    debugstr_w(pszAssoc),
                                    hkeyProgid,
                                    hWnd);
    if (hWnd != NULL)
        FIXME("hwnd != NULL not supported\n");
    if (cfFlags != 0)
    	FIXME("unsupported flags: %lx\n", cfFlags);
    if (pszAssoc != NULL)
    {
        hr = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                           pszAssoc,
                           0,
                           KEY_READ,
                           &This->hkeySource);
        if (FAILED(hr))
            return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
        /* if this is not a prog id */
        if ((*pszAssoc == '.') || (*pszAssoc == '{'))
        {
            hr = RegOpenKeyExW(This->hkeySource,
                               szProgID,
                               0,
                               KEY_READ,
                               &This->hkeyProgID);
            if (FAILED(hr))
                FIXME("Don't know what to return\n");
        }
        else
            This->hkeyProgID = This->hkeySource;
        return S_OK;
    }
    else if (hkeyProgid != NULL)
    {
        This->hkeyProgID = hkeyProgid;
        return S_OK;
    }
    else
        return E_FAIL;
}

/**************************************************************************
 *  IQueryAssociations_GetString {SHLWAPI}
 *
 * Get a file association string from the registry.
 *
 * PARAMS
 *  iface    [I]   IQueryAssociations interface to query
 *  cfFlags  [I]   ASSOCF_ flags from "shlwapi.h"
 *  str      [I]   Type of string to get (ASSOCSTR enum from "shlwapi.h")
 *  pszExtra [I]   Extra information about the string location
 *  pszOut   [O]   Destination for the association string
 *  pcchOut  [I/O] Length of pszOut
 *
 * RETURNS
 *  Success: S_OK. pszOut contains the string, pcchOut contains its length.
 *  Failure: An HRESULT error code indicating the error.
 */
static HRESULT WINAPI IQueryAssociations_fnGetString(
  IQueryAssociations *iface,
  ASSOCF cfFlags,
  ASSOCSTR str,
  LPCWSTR pszExtra,
  LPWSTR pszOut,
  DWORD *pcchOut)
{
  IQueryAssociationsImpl *This = (IQueryAssociationsImpl *)iface;

  FIXME("(%p,0x%8lx,0x%8x,%s,%p,%p)-stub!\n", This, cfFlags, str,
        debugstr_w(pszExtra), pszOut, pcchOut);
  return E_NOTIMPL;
}

/**************************************************************************
 *  IQueryAssociations_GetKey {SHLWAPI}
 *
 * Get a file association key from the registry.
 *
 * PARAMS
 *  iface    [I] IQueryAssociations interface to query
 *  cfFlags  [I] ASSOCF_ flags from "shlwapi.h"
 *  assockey [I] Type of key to get (ASSOCKEY enum from "shlwapi.h")
 *  pszExtra [I] Extra information about the key location
 *  phkeyOut [O] Destination for the association key
 *
 * RETURNS
 *  Success: S_OK. phkeyOut contains a handle to the key.
 *  Failure: An HRESULT error code indicating the error.
 */
static HRESULT WINAPI IQueryAssociations_fnGetKey(
  IQueryAssociations *iface,
  ASSOCF cfFlags,
  ASSOCKEY assockey,
  LPCWSTR pszExtra,
  HKEY *phkeyOut)
{
  IQueryAssociationsImpl *This = (IQueryAssociationsImpl *)iface;

  FIXME("(%p,0x%8lx,0x%8x,%s,%p)-stub!\n", This, cfFlags, assockey,
        debugstr_w(pszExtra), phkeyOut);
  return E_NOTIMPL;
}

/**************************************************************************
 *  IQueryAssociations_GetData {SHLWAPI}
 *
 * Get the data for a file association key from the registry.
 *
 * PARAMS
 *  iface     [I]   IQueryAssociations interface to query
 *  cfFlags   [I]   ASSOCF_ flags from "shlwapi.h"
 *  assocdata [I]   Type of data to get (ASSOCDATA enum from "shlwapi.h")
 *  pszExtra  [I]   Extra information about the data location
 *  pvOut     [O]   Destination for the association key
 *  pcbOut    [I/O] Size of pvOut
 *
 * RETURNS
 *  Success: S_OK. pszOut contains the data, pcbOut contains its length.
 *  Failure: An HRESULT error code indicating the error.
 */
static HRESULT WINAPI IQueryAssociations_fnGetData(
  IQueryAssociations *iface,
  ASSOCF cfFlags,
  ASSOCDATA assocdata,
  LPCWSTR pszExtra,
  LPVOID pvOut,
  DWORD *pcbOut)
{
  IQueryAssociationsImpl *This = (IQueryAssociationsImpl *)iface;

  FIXME("(%p,0x%8lx,0x%8x,%s,%p,%p)-stub!\n", This, cfFlags, assocdata,
        debugstr_w(pszExtra), pvOut, pcbOut);
  return E_NOTIMPL;
}

/**************************************************************************
 *  IQueryAssociations_GetEnum {SHLWAPI}
 *
 * Not yet implemented in native Win32.
 *
 * PARAMS
 *  iface     [I] IQueryAssociations interface to query
 *  cfFlags   [I] ASSOCF_ flags from "shlwapi.h"
 *  assocenum [I] Type of enum to get (ASSOCENUM enum from "shlwapi.h")
 *  pszExtra  [I] Extra information about the enum location
 *  riid      [I] REFIID to look for
 *  ppvOut    [O] Destination for the interface.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  Presumably this function returns an enumerator object.
 */
static HRESULT WINAPI IQueryAssociations_fnGetEnum(
  IQueryAssociations *iface,
  ASSOCF cfFlags,
  ASSOCENUM assocenum,
  LPCWSTR pszExtra,
  REFIID riid,
  LPVOID *ppvOut)
{
  IQueryAssociationsImpl *This = (IQueryAssociationsImpl *)iface;

  FIXME("(%p,0x%8lx,0x%8x,%s,%s,%p)-stub!\n", This, cfFlags, assocenum,
        debugstr_w(pszExtra), debugstr_guid(riid), ppvOut);
  return E_NOTIMPL;
}

static const IQueryAssociationsVtbl IQueryAssociations_vtbl =
{
  IQueryAssociations_fnQueryInterface,
  IQueryAssociations_fnAddRef,
  IQueryAssociations_fnRelease,
  IQueryAssociations_fnInit,
  IQueryAssociations_fnGetString,
  IQueryAssociations_fnGetKey,
  IQueryAssociations_fnGetData,
  IQueryAssociations_fnGetEnum
};
