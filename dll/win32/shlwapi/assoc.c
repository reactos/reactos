/*
 * IQueryAssociations helper functions
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "objbase.h"
#include "shlguid.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* Default IQueryAssociations::Init() flags */
#define SHLWAPI_DEF_ASSOCF (ASSOCF_INIT_BYEXENAME|ASSOCF_INIT_DEFAULTTOSTAR| \
                            ASSOCF_INIT_DEFAULTTOFOLDER)

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
 *  clsid  must be equal to CLSID_QueryAssociations and
 *  refiid must be equal to IID_IQueryAssociations, IID_IUnknown or this function will fail
 */
HRESULT WINAPI AssocCreate(CLSID clsid, REFIID refiid, void **lpInterface)
{
  TRACE("(%s,%s,%p)\n", debugstr_guid(&clsid), debugstr_guid(refiid),
        lpInterface);

  if (!lpInterface)
    return E_INVALIDARG;

  *(DWORD*)lpInterface = 0;

  if (!IsEqualGUID(&clsid,  &CLSID_QueryAssociations))
    return CLASS_E_CLASSNOTAVAILABLE;

  return SHCoCreateInstance( NULL, &clsid, NULL, refiid, lpInterface );
}


struct AssocPerceivedInfo
{
    PCWSTR Type;
    PERCEIVED Perceived;
    INT FlagHardcoded;
    INT FlagSoftcoded;
    PCWSTR Extensions;
};

static const WCHAR unspecified_exts[] = {
    '.','l','n','k',0,
    '.','s','e','a','r','c','h','-','m','s',0,
    0
};

static const WCHAR image_exts[] = {
    '.','b','m','p',0,
    '.','d','i','b',0,
    '.','e','m','f',0,
    '.','g','i','f',0,
    '.','i','c','o',0,
    '.','j','f','i','f',0,
    '.','j','p','e',0,
    '.','j','p','e','g',0,
    '.','j','p','g',0,
    '.','p','n','g',0,
    '.','r','l','e',0,
    '.','t','i','f',0,
    '.','t','i','f','f',0,
    '.','w','m','f',0,
    0
};

static const WCHAR audio_exts[] = {
    '.','a','i','f',0,
    '.','a','i','f','c',0,
    '.','a','i','f','f',0,
    '.','a','u',0,
    '.','m','3','u',0,
    '.','m','i','d',0,
    '.','m','i','d','i',0,
#if _WIN32_WINNT > 0x602
    '.','m','p','2',0,
#endif
    '.','m','p','3',0,
    '.','r','m','i',0,
    '.','s','n','d',0,
    '.','w','a','v',0,
    '.','w','a','x',0,
    '.','w','m','a',0,
    0
};

static const WCHAR video_exts[] = {
    '.','a','s','f',0,
    '.','a','s','x',0,
    '.','a','v','i',0,
    '.','d','v','r','-','m','s',0,
    '.','I','V','F',0,
    '.','m','1','v',0,
#if _WIN32_WINNT <= 0x602
    '.','m','p','2',0,
#endif
    '.','m','p','2','v',0,
    '.','m','p','a',0,
    '.','m','p','e',0,
    '.','m','p','e','g',0,
    '.','m','p','g',0,
    '.','m','p','v','2',0,
    '.','w','m',0,
    '.','w','m','v',0,
    '.','w','m','x',0,
    '.','w','v','x',0,
    0
};

static const WCHAR compressed_exts[] = {
    '.','z','i','p',0,
    0
};

static const WCHAR document_exts[] = {
#if _WIN32_WINNT >= 0x600
    '.','h','t','m',0,
    '.','h','t','m','l',0,
#endif
    '.','m','h','t',0,
    0
};

static const WCHAR system_exts[] = {
    '.','c','p','l',0,
    0
};

static const WCHAR application_exts[] = {
    '.','b','a','s',0,
    '.','b','a','t',0,
    '.','c','m','d',0,
    '.','c','o','m',0,
    '.','e','x','e',0,
    '.','h','t','a',0,
    '.','m','s','i',0,
    '.','p','i','f',0,
    '.','r','e','g',0,
    '.','s','c','r',0,
    '.','v','b',0,
    0
};

const WCHAR type_text[] = {'t','e','x','t',0};
const WCHAR type_image[] = {'i','m','a','g','e',0};
const WCHAR type_audio[] = {'a','u','d','i','o',0};
const WCHAR type_video[] = {'v','i','d','e','o',0};
const WCHAR type_compressed[] = {'c','o','m','p','r','e','s','s','e','d',0};
const WCHAR type_document[] = {'d','o','c','u','m','e','n','t',0};
const WCHAR type_system[] = {'s','y','s','t','e','m',0};
const WCHAR type_application[] = {'a','p','p','l','i','c','a','t','i','o','n',0};

#define HARDCODED_NATIVE_WMSDK      (PERCEIVEDFLAG_HARDCODED | PERCEIVEDFLAG_NATIVESUPPORT | PERCEIVEDFLAG_WMSDK)
#define HARDCODED_NATIVE_GDIPLUS    (PERCEIVEDFLAG_HARDCODED | PERCEIVEDFLAG_NATIVESUPPORT | PERCEIVEDFLAG_GDIPLUS)
#define HARDCODED_NATIVE_ZIPFLDR    (PERCEIVEDFLAG_HARDCODED | PERCEIVEDFLAG_NATIVESUPPORT | PERCEIVEDFLAG_ZIPFOLDER)
#define SOFTCODED_NATIVESUPPORT     (PERCEIVEDFLAG_SOFTCODED | PERCEIVEDFLAG_NATIVESUPPORT)

static const struct AssocPerceivedInfo known_types[] = {
    { NULL,             PERCEIVED_TYPE_UNSPECIFIED, PERCEIVEDFLAG_HARDCODED,  PERCEIVEDFLAG_SOFTCODED, unspecified_exts },
    { type_text,        PERCEIVED_TYPE_TEXT,        PERCEIVEDFLAG_HARDCODED,  SOFTCODED_NATIVESUPPORT, NULL },
    { type_image,       PERCEIVED_TYPE_IMAGE,       HARDCODED_NATIVE_GDIPLUS, PERCEIVEDFLAG_SOFTCODED, image_exts },
    { type_audio,       PERCEIVED_TYPE_AUDIO,       HARDCODED_NATIVE_WMSDK,   PERCEIVEDFLAG_SOFTCODED, audio_exts },
    { type_video,       PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,   PERCEIVEDFLAG_SOFTCODED, video_exts },
    { type_compressed,  PERCEIVED_TYPE_COMPRESSED,  HARDCODED_NATIVE_ZIPFLDR, PERCEIVEDFLAG_SOFTCODED, compressed_exts },
    { type_document,    PERCEIVED_TYPE_DOCUMENT,    PERCEIVEDFLAG_HARDCODED,  PERCEIVEDFLAG_SOFTCODED, document_exts },
    { type_system,      PERCEIVED_TYPE_SYSTEM,      PERCEIVEDFLAG_HARDCODED,  PERCEIVEDFLAG_SOFTCODED, system_exts },
    { type_application, PERCEIVED_TYPE_APPLICATION, PERCEIVEDFLAG_HARDCODED,  PERCEIVEDFLAG_SOFTCODED, application_exts },
};

static const struct AssocPerceivedInfo* AssocFindByBuiltinExtension(LPCWSTR pszExt)
{
    UINT n;
    for (n = 0; n < sizeof(known_types) / sizeof(known_types[0]); ++n)
    {
        PCWSTR Ext = known_types[n].Extensions;
        while (Ext && *Ext)
        {
            if (!StrCmpIW(Ext, pszExt))
                return &known_types[n];
            Ext += (strlenW(Ext) + 1);
        }
    }
    return NULL;
}

static const struct AssocPerceivedInfo* AssocFindByType(LPCWSTR pszType)
{
    UINT n;
    for (n = 0; n < sizeof(known_types) / sizeof(known_types[0]); ++n)
    {
        if (known_types[n].Type)
        {
            if (!StrCmpIW(known_types[n].Type, pszType))
                return &known_types[n];
        }
    }
    return NULL;
}


/*************************************************************************
 * AssocGetPerceivedType  [SHLWAPI.@]
 *
 * Detect the type of a file by inspecting its extension
 *
 * PARAMS
 *  lpszExt     [I] File extension to evaluate.
 *  lpType      [O] Pointer to perceived type
 *  lpFlag      [O] Pointer to perceived type flag
 *  lppszType   [O] Address to pointer for perceived type text
 *
 * RETURNS
 *  Success: S_OK. lpType and lpFlag contain the perceived type and
 *           its information. If lppszType is not NULL, it will point
 *           to a string with perceived type text.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  lppszType is optional and it can be NULL.
 *  if lpType or lpFlag are NULL, the function will crash.
 *  if lpszExt is NULL, an error is returned.
 */
HRESULT WINAPI AssocGetPerceivedType(LPCWSTR lpszExt, PERCEIVED *lpType,
                                     INT *lpFlag, LPWSTR *lppszType)
{
    static const WCHAR PerceivedTypeKey[] = {'P','e','r','c','e','i','v','e','d','T','y','p','e',0};
    static const WCHAR SystemFileAssociationsKey[] = {'S','y','s','t','e','m','F','i','l','e',
        'A','s','s','o','c','i','a','t','i','o','n','s','\\','%','s',0};
    const struct AssocPerceivedInfo *Info;

    TRACE("(%s,%p,%p,%p)\n", debugstr_w(lpszExt), lpType, lpFlag, lppszType);

    Info = AssocFindByBuiltinExtension(lpszExt);
    if (Info)
    {
        *lpType = Info->Perceived;
        *lpFlag = Info->FlagHardcoded;
    }
    else
    {
        WCHAR Buffer[100] = { 0 };
        DWORD Size = sizeof(Buffer);
        if (RegGetValueW(HKEY_CLASSES_ROOT, lpszExt, PerceivedTypeKey,
                         RRF_RT_REG_SZ, NULL, Buffer, &Size) == ERROR_SUCCESS)
        {
            Info = AssocFindByType(Buffer);
        }
        if (!Info)
        {
            WCHAR KeyName[MAX_PATH] = { 0 };
            snprintfW(KeyName, MAX_PATH, SystemFileAssociationsKey, lpszExt);
            Size = sizeof(Buffer);
            if (RegGetValueW(HKEY_CLASSES_ROOT, KeyName, PerceivedTypeKey,
                             RRF_RT_REG_SZ, NULL, Buffer, &Size) == ERROR_SUCCESS)
            {
                Info = AssocFindByType(Buffer);
            }
        }
        if (Info)
        {
            *lpType = Info->Perceived;
            *lpFlag = Info->FlagSoftcoded;
        }
    }

    if (Info)
    {
        if (lppszType && Info->Type)
        {
            return SHStrDupW(Info->Type, lppszType);
        }
        return Info->Type ? S_OK : E_FAIL;
    }
    else
    {
        *lpType = PERCEIVED_TYPE_UNSPECIFIED;
        *lpFlag = 0;
    }
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
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

  TRACE("(0x%x,%d,%s,%s,%p)\n", cfFlags, assockey, debugstr_w(pszAssoc),
        debugstr_w(pszExtra), phkeyOut);

  hRet = AssocCreate( CLSID_QueryAssociations, &IID_IQueryAssociations, (void **)&lpAssoc );
  if (FAILED(hRet)) return hRet;

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

  TRACE("(0x%x,%d,%s,%s,%p)\n", cfFlags, assockey, debugstr_a(pszAssoc),
        debugstr_a(pszExtra), phkeyOut);

  if (SHLWAPI_ParamAToW(pszAssoc, szAssocW, MAX_PATH, &lpszAssocW) &&
      SHLWAPI_ParamAToW(pszExtra, szExtraW, MAX_PATH, &lpszExtraW))
  {
    hRet = AssocQueryKeyW(cfFlags, assockey, lpszAssocW, lpszExtraW, phkeyOut);
  }

  if (lpszAssocW != szAssocW)
    HeapFree(GetProcessHeap(), 0, lpszAssocW);

  if (lpszExtraW != szExtraW)
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

  TRACE("(0x%x,%d,%s,%s,%p,%p)\n", cfFlags, str, debugstr_w(pszAssoc),
        debugstr_w(pszExtra), pszOut, pcchOut);

  if (!pcchOut)
    return E_UNEXPECTED;

  hRet = AssocCreate( CLSID_QueryAssociations, &IID_IQueryAssociations, (void **)&lpAssoc );
  if (FAILED(hRet)) return hRet;

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

  TRACE("(0x%x,0x%d,%s,%s,%p,%p)\n", cfFlags, str, debugstr_a(pszAssoc),
        debugstr_a(pszExtra), pszOut, pcchOut);

  if (!pcchOut)
    hRet = E_UNEXPECTED;
  else if (SHLWAPI_ParamAToW(pszAssoc, szAssocW, MAX_PATH, &lpszAssocW) &&
           SHLWAPI_ParamAToW(pszExtra, szExtraW, MAX_PATH, &lpszExtraW))
  {
    WCHAR szReturnW[MAX_PATH], *lpszReturnW = szReturnW;
    DWORD dwLenOut = *pcchOut;

    if (dwLenOut >= MAX_PATH)
      lpszReturnW = HeapAlloc(GetProcessHeap(), 0,
                                      (dwLenOut + 1) * sizeof(WCHAR));
    else
      dwLenOut = sizeof(szReturnW) / sizeof(szReturnW[0]);

    if (!lpszReturnW)
      hRet = E_OUTOFMEMORY;
    else
    {
      hRet = AssocQueryStringW(cfFlags, str, lpszAssocW, lpszExtraW,
                               lpszReturnW, &dwLenOut);

      if (SUCCEEDED(hRet))
        dwLenOut = WideCharToMultiByte(CP_ACP, 0, lpszReturnW, -1,
                                       pszOut, *pcchOut, NULL, NULL);

      *pcchOut = dwLenOut;
      if (lpszReturnW != szReturnW)
        HeapFree(GetProcessHeap(), 0, lpszReturnW);
    }
  }

  if (lpszAssocW != szAssocW)
    HeapFree(GetProcessHeap(), 0, lpszAssocW);
  if (lpszExtraW != szExtraW)
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

  TRACE("(0x%x,0x%d,%p,%s,%p,%p)\n", cfFlags, str, hkAssoc,
        debugstr_w(pszExtra), pszOut, pcchOut);

  hRet = AssocCreate( CLSID_QueryAssociations, &IID_IQueryAssociations, (void **)&lpAssoc );
  if (FAILED(hRet)) return hRet;

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
  WCHAR szExtraW[MAX_PATH], *lpszExtraW = szExtraW;
  WCHAR szReturnW[MAX_PATH], *lpszReturnW = szReturnW;
  HRESULT hRet = E_OUTOFMEMORY;

  TRACE("(0x%x,0x%d,%p,%s,%p,%p)\n", cfFlags, str, hkAssoc,
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

  if (lpszExtraW != szExtraW)
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
