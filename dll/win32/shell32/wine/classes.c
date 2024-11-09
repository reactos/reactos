/*
 *	file type mapping
 *	(HKEY_CLASSES_ROOT - Stuff)
 *
 * Copyright 1998, 1999, 2000 Juergen Schmied
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

#include <wine/config.h>

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <shlguid_undoc.h>
#include <shlwapi.h>
#include <wine/debug.h>
#include <wine/unicode.h>
#ifdef __REACTOS__
#include <strsafe.h>
#endif

#include "pidl.h"
#include "shell32_main.h"
#include "shresdef.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define MAX_EXTENSION_LENGTH 20 // FIXME: The limit is 254?

static LONG GetRegString(HKEY hKey, PCWSTR SubKey, PCWSTR Name, PWSTR Buffer, UINT cchBuf)
{
    DWORD cb = sizeof(*Buffer) * cchBuf;
    return RegGetValueW(hKey, SubKey, Name, RRF_RT_REG_SZ, NULL, Buffer, &cb);
}

HRESULT HCR_GetProgIdKeyOfExtension(PCWSTR szExtension, PHKEY phKey, BOOL AllowFallback)
{
    LONG err;
    WCHAR ext[max(1 + MAX_EXTENSION_LENGTH + 1, MAX_PATH)];
    WCHAR progid[MAX_PATH];
    if (szExtension[0] != '.')
    {
        ext[0] = '.';
        lstrcpynW(ext + 1, szExtension, _countof(ext) - 1);
        szExtension = ext;
    }
    err = GetRegString(HKEY_CLASSES_ROOT, szExtension, NULL, progid, _countof(progid));
    if (!err && progid[0] != UNICODE_NULL)
    {
        err = RegOpenKeyExW(HKEY_CLASSES_ROOT, progid, 0, KEY_READ, phKey);
        if (!err)
            return err; /* A real ProgId key, return S_OK */
    }
    if (AllowFallback)
    {
        err = RegOpenKeyExW(HKEY_CLASSES_ROOT, szExtension, 0, KEY_READ, phKey);
        if (!err)
            return S_FALSE;
    }
    return HRESULT_FROM_WIN32(err);
}

BOOL HCR_MapTypeToValueW(LPCWSTR szExtension, LPWSTR szFileType, LONG len, BOOL bPrependDot)
{	
	HKEY	hkey;
	WCHAR	szTemp[MAX_EXTENSION_LENGTH + 2];

	TRACE("%s %p\n", debugstr_w(szExtension), szFileType);

        /* added because we do not want to have double dots */
        if (szExtension[0] == '.')
          bPrependDot = FALSE;

	if (bPrependDot)
	  szTemp[0] = '.';

	lstrcpynW(szTemp + (bPrependDot?1:0), szExtension, MAX_EXTENSION_LENGTH);

	if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szTemp, 0, KEY_READ, &hkey))
	{ 
	  return FALSE;
	}

	if (RegQueryValueW(hkey, NULL, szFileType, &len))
	{ 
	  RegCloseKey(hkey);
	  return FALSE;
	}

	RegCloseKey(hkey);

	TRACE("--UE;\n} %s\n", debugstr_w(szFileType));

	return TRUE;
}

BOOL HCR_MapTypeToValueA(LPCSTR szExtension, LPSTR szFileType, LONG len, BOOL bPrependDot)
{
	HKEY	hkey;
	char	szTemp[MAX_EXTENSION_LENGTH + 2];

	TRACE("%s %p\n", szExtension, szFileType);

        /* added because we do not want to have double dots */
        if (szExtension[0] == '.')
          bPrependDot = FALSE;

	if (bPrependDot)
	  szTemp[0] = '.';

	lstrcpynA(szTemp + (bPrependDot?1:0), szExtension, MAX_EXTENSION_LENGTH);

	if (RegOpenKeyExA(HKEY_CLASSES_ROOT, szTemp, 0, KEY_READ, &hkey))
	{ 
	  return FALSE;
	}

	if (RegQueryValueA(hkey, NULL, szFileType, &len))
	{ 
	  RegCloseKey(hkey);
	  return FALSE;
	}

	RegCloseKey(hkey);

	TRACE("--UE;\n} %s\n", szFileType);

	return TRUE;
}

EXTERN_C HRESULT SHELL32_EnumDefaultVerbList(LPCWSTR List, UINT Index, LPWSTR Verb, SIZE_T cchMax);

BOOL HCR_GetDefaultVerbW( HKEY hkeyClass, LPCWSTR szVerb, LPWSTR szDest, DWORD len )
{
        WCHAR sTemp[MAX_PATH], verbs[MAX_PATH];
        LONG size;
        HKEY hkey;

	TRACE("%p %s %p\n", hkeyClass, debugstr_w(szVerb), szDest);

        if (szVerb && *szVerb)
        {
            lstrcpynW(szDest, szVerb, len);
            return TRUE;
        }

        /* MSDN says to first try the default verb */
        size = _countof(verbs);
        if (!RegQueryValueW(hkeyClass, L"shell", verbs, &size) && *verbs)
        {
            for (UINT i = 0;; ++i)
            {
                if (FAILED(SHELL32_EnumDefaultVerbList(verbs, i, szDest, len)))
                    break;
                if (FAILED(StringCchPrintfW(sTemp, _countof(sTemp), L"shell\\%s\\command", szDest)))
                    break;
                if (!RegOpenKeyExW(hkeyClass, sTemp, 0, KEY_READ, &hkey))
                {
                    RegCloseKey(hkey);
                    TRACE("default verb=%s\n", debugstr_w(szDest));
                    return TRUE;
                }
            }
        }
        *szDest = UNICODE_NULL;

        /* then fallback to 'open' */
        lstrcpyW(sTemp, L"shell\\open\\command");
        if (!RegOpenKeyExW(hkeyClass, sTemp, 0, KEY_READ, &hkey))
        {
            RegCloseKey(hkey);
            lstrcpynW(szDest, L"open", len);
            TRACE("default verb=open\n");
            return TRUE;
        }

        /* and then just use the first verb on Windows >= 2000 */
#ifdef __REACTOS__
        if (!RegOpenKeyExW(hkeyClass, L"shell", 0, KEY_READ, &hkey))
        {
            if (!RegEnumKeyW(hkey, 0, szDest, len) && *szDest)
            {
                TRACE("default verb=first verb=%s\n", debugstr_w(szDest));
                RegCloseKey(hkey);
                return TRUE;
            }
            RegCloseKey(hkey);
        }
#else
        if (!RegEnumKeyW(hkeyClass, 0, szDest, len) && *szDest)
        {
            TRACE("default verb=first verb=%s\n", debugstr_w(szDest));
            return TRUE;
        }
#endif

        TRACE("no default verb!\n");
	return FALSE;
}

BOOL HCR_GetExecuteCommandW( HKEY hkeyClass, LPCWSTR szClass, LPCWSTR szVerb, LPWSTR szDest, DWORD len )
{
	WCHAR sTempVerb[MAX_PATH];
	BOOL ret;

	TRACE("%p %s %s %p\n", hkeyClass, debugstr_w(szClass), debugstr_w(szVerb), szDest);

	if (szClass)
            RegOpenKeyExW(HKEY_CLASSES_ROOT, szClass, 0, KEY_READ, &hkeyClass);
        if (!hkeyClass)
            return FALSE;
        ret = FALSE;

        if (HCR_GetDefaultVerbW(hkeyClass, szVerb, sTempVerb, sizeof(sTempVerb)/sizeof(sTempVerb[0])))
        {
            WCHAR sTemp[MAX_PATH];
            lstrcpyW(sTemp, L"shell\\");
            lstrcatW(sTemp, sTempVerb);
            lstrcatW(sTemp, L"\\command");
            ret = (ERROR_SUCCESS == SHGetValueW(hkeyClass, sTemp, NULL, NULL, szDest, &len));
        }
        if (szClass)
            RegCloseKey(hkeyClass);

	TRACE("-- %s\n", debugstr_w(szDest) );
	return ret;
}

/***************************************************************************************
*	HCR_GetDefaultIcon	[internal]
*
* Gets the icon for a filetype
*/
BOOL HCR_RegOpenClassIDKey(REFIID riid, HKEY *hkey)
{
	char	xriid[50];
    sprintf( xriid, "CLSID\\{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 riid->Data1, riid->Data2, riid->Data3,
                 riid->Data4[0], riid->Data4[1], riid->Data4[2], riid->Data4[3],
                 riid->Data4[4], riid->Data4[5], riid->Data4[6], riid->Data4[7] );

 	TRACE("%s\n",xriid );

	return !RegOpenKeyExA(HKEY_CLASSES_ROOT, xriid, 0, KEY_READ, hkey);
}

static BOOL HCR_RegGetIconW(HKEY hkey, LPWSTR szDest, LPCWSTR szName, DWORD len, int* picon_idx)
{
    DWORD dwType, size = len * sizeof(WCHAR);
    WCHAR sTemp[MAX_PATH];
    WCHAR sNum[5];

    if (!RegQueryValueExW(hkey, szName, 0, &dwType, (LPBYTE)szDest, &size))
    {
      if (dwType == REG_EXPAND_SZ)
      {
        ExpandEnvironmentStringsW(szDest, sTemp, MAX_PATH);
        lstrcpynW(szDest, sTemp, len);
      }
        if (ParseFieldW (szDest, 2, sNum, _countof(sNum)))
             *picon_idx = atoiW(sNum);
          else
             *picon_idx=0; /* sometimes the icon number is missing */
      ParseFieldW (szDest, 1, szDest, len);
          PathUnquoteSpacesW(szDest);
      return TRUE;
    }
    return FALSE;
}

static BOOL HCR_RegGetIconA(HKEY hkey, LPSTR szDest, LPCSTR szName, DWORD len, int* picon_idx)
{
	DWORD dwType;
	char sTemp[MAX_PATH];
	char  sNum[5];

	if (!RegQueryValueExA(hkey, szName, 0, &dwType, (LPBYTE)szDest, &len))
	{
          if (dwType == REG_EXPAND_SZ)
	  {
	    ExpandEnvironmentStringsA(szDest, sTemp, MAX_PATH);
	    lstrcpynA(szDest, sTemp, len);
	  }
	  if (ParseFieldA (szDest, 2, sNum, 5))
             *picon_idx=atoi(sNum);
          else
             *picon_idx=0; /* sometimes the icon number is missing */
	  ParseFieldA (szDest, 1, szDest, len);
          PathUnquoteSpacesA(szDest);
	  return TRUE;
	}
	return FALSE;
}

BOOL HCR_GetIconW(LPCWSTR szClass, LPWSTR szDest, LPCWSTR szName, DWORD len, int* picon_idx)
{
	HKEY	hkey;
	WCHAR	sTemp[MAX_PATH];
	BOOL	ret = FALSE;

	TRACE("%s\n",debugstr_w(szClass) );

	lstrcpynW(sTemp, szClass, MAX_PATH);
	lstrcatW(sTemp, L"\\DefaultIcon");

	if (!RegOpenKeyExW(HKEY_CLASSES_ROOT, sTemp, 0, KEY_READ, &hkey))
	{
	  ret = HCR_RegGetIconW(hkey, szDest, szName, len, picon_idx);
	  RegCloseKey(hkey);
	}

        if(ret)
            TRACE("-- %s %i\n", debugstr_w(szDest), *picon_idx);
        else
            TRACE("-- not found\n");

	return ret;
}

BOOL HCR_GetIconA(LPCSTR szClass, LPSTR szDest, LPCSTR szName, DWORD len, int* picon_idx)
{
	HKEY	hkey;
	char	sTemp[MAX_PATH];
	BOOL	ret = FALSE;

	TRACE("%s\n",szClass );

	sprintf(sTemp, "%s\\DefaultIcon",szClass);

	if (!RegOpenKeyExA(HKEY_CLASSES_ROOT, sTemp, 0, KEY_READ, &hkey))
	{
	  ret = HCR_RegGetIconA(hkey, szDest, szName, len, picon_idx);
	  RegCloseKey(hkey);
	}

    if (ret)
        TRACE("-- %s %i\n", szDest, *picon_idx);
    else
        TRACE("-- not found\n");

	return ret;
}

#ifdef __REACTOS__
BOOL HCU_GetIconW(LPCWSTR szClass, LPWSTR szDest, LPCWSTR szName, DWORD len, int* picon_idx)
{
    HKEY hkey;
    WCHAR sTemp[MAX_PATH];
    BOOL ret = FALSE;

    TRACE("%s\n", debugstr_w(szClass));

    StringCchPrintfW(sTemp, _countof(sTemp), L"%s\\DefaultIcon", szClass);

    if (!RegOpenKeyExW(HKEY_CURRENT_USER, sTemp, 0, KEY_READ, &hkey))
    {
        ret = HCR_RegGetIconW(hkey, szDest, szName, len, picon_idx);
        RegCloseKey(hkey);
    }

    if (ret)
        TRACE("-- %s %i\n", debugstr_w(szDest), *picon_idx);
    else
        TRACE("-- not found\n");

    return ret;
}

BOOL HLM_GetIconW(int reg_idx, LPWSTR szDest, DWORD len, int* picon_idx)
{
    HKEY hkey;
    WCHAR sTemp[5];
    BOOL ret = FALSE;

    TRACE("%d\n", reg_idx);

    StringCchPrintfW(sTemp, _countof(sTemp), L"%d", reg_idx);

    if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                       L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Icons",
                       0,
                       KEY_READ,
                       &hkey))
    {
        ret = HCR_RegGetIconW(hkey, szDest, sTemp, len, picon_idx);
        RegCloseKey(hkey);
    }

    if (ret)
        TRACE("-- %s %i\n", debugstr_w(szDest), *picon_idx);
    else
        TRACE("-- not found\n");

    return ret;
}
#endif

/***************************************************************************************
*	HCR_GetClassName	[internal]
*
* Gets the name of a registered class
*/
BOOL HCR_GetClassNameW(REFIID riid, LPWSTR szDest, DWORD len)
{	
	HKEY	hkey;
	BOOL ret = FALSE;
	DWORD buflen = len;
#ifdef __REACTOS__
        WCHAR szName[100];
        LPOLESTR pStr;
#endif

 	szDest[0] = 0;

#ifdef __REACTOS__
        if (StringFromCLSID(riid, &pStr) == S_OK)
        {
            DWORD dwLen = buflen * sizeof(WCHAR);
            swprintf(szName, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\%s", pStr);
            if (!RegGetValueW(HKEY_CURRENT_USER, szName, NULL, RRF_RT_REG_SZ, NULL, (PVOID)szDest, &dwLen))
            {
                ret = TRUE;
            }
            CoTaskMemFree(pStr);
        }
        if (!ret && HCR_RegOpenClassIDKey(riid, &hkey))
#else
	if (HCR_RegOpenClassIDKey(riid, &hkey))
#endif
	{
          if (!RegLoadMUIStringW(hkey, L"LocalizedString", szDest, len, NULL, 0, NULL) ||
              !RegQueryValueExW(hkey, L"", 0, NULL, (LPBYTE)szDest, &len))
          {
	    ret = TRUE;
	  }
	  RegCloseKey(hkey);
	}

	if (!ret || !szDest[0])
	{
	  if(IsEqualIID(riid, &CLSID_ShellDesktop))
	  {
	    if (LoadStringW(shell32_hInstance, IDS_DESKTOP, szDest, buflen))
	      ret = TRUE;
	  }
	  else if (IsEqualIID(riid, &CLSID_MyComputer))
	  {
	    if(LoadStringW(shell32_hInstance, IDS_MYCOMPUTER, szDest, buflen))
	      ret = TRUE;
	  }
#ifdef __REACTOS__
          else if (IsEqualIID(riid, &CLSID_MyDocuments))
          {
              if(LoadStringW(shell32_hInstance, IDS_PERSONAL, szDest, buflen))
                  ret = TRUE;
          }
          else if (IsEqualIID(riid, &CLSID_RecycleBin))
          {
              if(LoadStringW(shell32_hInstance, IDS_RECYCLEBIN_FOLDER_NAME, szDest, buflen))
                  ret = TRUE;
          }
          else if (IsEqualIID(riid, &CLSID_ControlPanel))
          {
              if(LoadStringW(shell32_hInstance, IDS_CONTROLPANEL, szDest, buflen))
                  ret = TRUE;
          }
          else if (IsEqualIID(riid, &CLSID_AdminFolderShortcut))
          {
              if(LoadStringW(shell32_hInstance, IDS_ADMINISTRATIVETOOLS, szDest, buflen))
                  ret = TRUE;
          }
#endif
	}
	TRACE("-- %s\n", debugstr_w(szDest));
	return ret;
}

BOOL HCR_GetClassNameA(REFIID riid, LPSTR szDest, DWORD len)
{	HKEY	hkey;
	BOOL ret = FALSE;
	DWORD buflen = len;
#ifdef __REACTOS__
        CHAR szName[100];
        LPOLESTR pStr;
#endif

	szDest[0] = 0;

#ifdef __REACTOS__
        if (StringFromCLSID(riid, &pStr) == S_OK)
        {
            DWORD dwLen = buflen * sizeof(CHAR);
            sprintf(szName, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\%S", pStr);
            if (!RegGetValueA(HKEY_CURRENT_USER, szName, NULL, RRF_RT_REG_SZ, NULL, (PVOID)szDest, &dwLen))
            {
                ret = TRUE;
            }
            CoTaskMemFree(pStr);
        }
        if (!ret && HCR_RegOpenClassIDKey(riid, &hkey))
#else
	if (HCR_RegOpenClassIDKey(riid, &hkey))
#endif
	{
          if (!RegLoadMUIStringA(hkey,"LocalizedString",szDest,len,NULL,0,NULL) ||
              !RegQueryValueExA(hkey,"",0,NULL,(LPBYTE)szDest,&len))
          {
	    ret = TRUE;
	  }
	  RegCloseKey(hkey);
	}

	if (!ret || !szDest[0])
	{
	  if(IsEqualIID(riid, &CLSID_ShellDesktop))
	  {
	    if (LoadStringA(shell32_hInstance, IDS_DESKTOP, szDest, buflen))
	      ret = TRUE;
	  }
	  else if (IsEqualIID(riid, &CLSID_MyComputer))
	  {
	    if(LoadStringA(shell32_hInstance, IDS_MYCOMPUTER, szDest, buflen))
	      ret = TRUE;
	  }
#ifdef __REACTOS__
          else if (IsEqualIID(riid, &CLSID_MyDocuments))
          {
              if(LoadStringA(shell32_hInstance, IDS_PERSONAL, szDest, buflen))
                  ret = TRUE;
          }
          else if (IsEqualIID(riid, &CLSID_RecycleBin))
          {
              if(LoadStringA(shell32_hInstance, IDS_RECYCLEBIN_FOLDER_NAME, szDest, buflen))
                  ret = TRUE;
          }
          else if (IsEqualIID(riid, &CLSID_ControlPanel))
          {
              if(LoadStringA(shell32_hInstance, IDS_CONTROLPANEL, szDest, buflen))
                  ret = TRUE;
          }
          else if (IsEqualIID(riid, &CLSID_AdminFolderShortcut))
          {
              if(LoadStringA(shell32_hInstance, IDS_ADMINISTRATIVETOOLS, szDest, buflen))
                  ret = TRUE;
          }
#endif
	}

	TRACE("-- (%s)\n", szDest);

	return ret;
}

/******************************************************************************
 * HCR_GetFolderAttributes [Internal]
 *
 * Query the registry for a shell folders' attributes
 *
 * PARAMS
 *  pidlFolder    [I]  A simple pidl of type PT_GUID. 
 *  pdwAttributes [IO] In: Attributes to be queried, OUT: Resulting attributes.
 *
 * RETURNS
 *  TRUE:  Found information for the attributes in the registry
 *  FALSE: No attribute information found
 *
 * NOTES
 *  If queried for an attribute, which is set in the CallForAttributes registry
 *  value, the function binds to the shellfolder objects and queries it.
 */
BOOL HCR_GetFolderAttributes(LPCITEMIDLIST pidlFolder, LPDWORD pdwAttributes)
{
    HKEY hSFKey;
    LPOLESTR pwszCLSID;
    LONG lResult;
    DWORD dwTemp, dwLen;
    WCHAR wszShellFolderKey[] = L"CLSID\\{00021400-0000-0000-C000-000000000046}\\ShellFolder";

    TRACE("(pidlFolder=%p, pdwAttributes=%p)\n", pidlFolder, pdwAttributes);
       
    if (!_ILIsPidlSimple(pidlFolder)) { 
        static BOOL firstHit = TRUE;
        if (firstHit) {
            ERR("should be called for simple PIDL's only!\n");
            firstHit = FALSE;
        }
        return FALSE;
    }
    
    if (!_ILIsDesktop(pidlFolder)) {
        if (FAILED(StringFromCLSID(_ILGetGUIDPointer(pidlFolder), &pwszCLSID))) return FALSE;
        memcpy(&wszShellFolderKey[6], pwszCLSID, 38 * sizeof(WCHAR));
        CoTaskMemFree(pwszCLSID);
    }
    
    lResult = RegOpenKeyExW(HKEY_CLASSES_ROOT, wszShellFolderKey, 0, KEY_READ, &hSFKey);
#ifdef __REACTOS__
    if (lResult != ERROR_SUCCESS)
    {
        ERR("Cannot open key: %ls\n", wszShellFolderKey);
        return FALSE;
    }
#else
    if (lResult != ERROR_SUCCESS) return FALSE;
#endif
    
    dwLen = sizeof(DWORD);
    lResult = RegQueryValueExW(hSFKey, L"CallForAttributes", 0, NULL, (LPBYTE)&dwTemp, &dwLen);
    if ((lResult == ERROR_SUCCESS) && (dwTemp & *pdwAttributes)) {
        LPSHELLFOLDER psfDesktop, psfFolder;
        HRESULT hr;

        RegCloseKey(hSFKey);
        hr = SHGetDesktopFolder(&psfDesktop);
        if (SUCCEEDED(hr)) {
            hr = IShellFolder_BindToObject(psfDesktop, pidlFolder, NULL, &IID_IShellFolder, 
                                           (LPVOID*)&psfFolder);
            if (SUCCEEDED(hr)) { 
                hr = IShellFolder_GetAttributesOf(psfFolder, 0, NULL, pdwAttributes);
                IShellFolder_Release(psfFolder);
            }
            IShellFolder_Release(psfDesktop);
        }
        if (FAILED(hr)) return FALSE;
    } else {
        lResult = RegQueryValueExW(hSFKey, L"Attributes", 0, NULL, (LPBYTE)&dwTemp, &dwLen);
        RegCloseKey(hSFKey);
        if (lResult == ERROR_SUCCESS) {
            *pdwAttributes &= dwTemp;
        } else {
            return FALSE;
        }
    }

    TRACE("-- *pdwAttributes == 0x%08x\n", *pdwAttributes);

    return TRUE;
}
