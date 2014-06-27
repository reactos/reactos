/*
 *    file type mapping
 *    (HKEY_CLASSES_ROOT - Stuff)
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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define MAX_EXTENSION_LENGTH 20

BOOL HCR_MapTypeToValueW(LPCWSTR szExtension, LPWSTR szFileType, LONG len, BOOL bPrependDot)
{
    HKEY hkey;
    WCHAR szTemp[MAX_EXTENSION_LENGTH + 2];

    TRACE("%s %p\n", debugstr_w(szExtension), debugstr_w(szFileType));

    /* added because we do not want to have double dots */
    if (szExtension[0] == '.')
        bPrependDot = FALSE;

    if (bPrependDot)
        szTemp[0] = '.';

    lstrcpynW(szTemp + (bPrependDot ? 1 : 0), szExtension, MAX_EXTENSION_LENGTH);

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szTemp, 0, KEY_READ, &hkey) != ERROR_SUCCESS)
        return FALSE;

    if (RegQueryValueW(hkey, NULL, szFileType, &len) != ERROR_SUCCESS)
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
    HKEY hkey;
    char szTemp[MAX_EXTENSION_LENGTH + 2];

    TRACE("%s %p\n", szExtension, szFileType);

    /* added because we do not want to have double dots */
    if (szExtension[0] == '.')
        bPrependDot = FALSE;

    if (bPrependDot)
        szTemp[0] = '.';

    lstrcpynA(szTemp + (bPrependDot ? 1 : 0), szExtension, MAX_EXTENSION_LENGTH);

    if (RegOpenKeyExA(HKEY_CLASSES_ROOT, szTemp, 0, KEY_READ, &hkey))
    {
        return FALSE;
    }

    if (RegLoadMUIStringA(hkey, "FriendlyTypeName", szFileType, len, NULL, 0, NULL) == ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        return TRUE;
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

static const WCHAR swShell[] = L"shell\\";
static const WCHAR swOpen[] = L"open";
static const WCHAR swCommand[] = L"\\command";

BOOL HCR_GetDefaultVerbW(HKEY hkeyClass, LPCWSTR szVerb, LPWSTR szDest, DWORD len)
{
    WCHAR sTemp[MAX_PATH];
    LONG size;
    HKEY hkey;

    TRACE("%p %s %p\n", hkeyClass, debugstr_w(szVerb), szDest);

    if (szVerb && *szVerb)
    {
        lstrcpynW(szDest, szVerb, len);
        return TRUE;
    }

    size = len;
    *szDest = '\0';
    if (RegQueryValueW(hkeyClass, L"shell", szDest, &size) == ERROR_SUCCESS && *szDest)
    {
        /* The MSDN says to first try the default verb */
        wcscpy(sTemp, swShell);
        wcscat(sTemp, szDest);
        wcscat(sTemp, swCommand);
        if (RegOpenKeyExW(hkeyClass, sTemp, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
        {
            RegCloseKey(hkey);
            TRACE("default verb=%s\n", debugstr_w(szDest));
            return TRUE;
        }
    }

    /* then fallback to 'open' */
    wcscpy(sTemp, swShell);
    wcscat(sTemp, swOpen);
    wcscat(sTemp, swCommand);
    if (RegOpenKeyExW(hkeyClass, sTemp, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        lstrcpynW(szDest, swOpen, len);
        TRACE("default verb=open\n");
        return TRUE;
    }

    /* and then just use the first verb on Windows >= 2000 */
    if (RegOpenKeyExW(hkeyClass, L"shell", 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        if (RegEnumKeyW(hkey, 0, szDest, len) == ERROR_SUCCESS && *szDest)
        {
            TRACE("default verb=first verb=%s\n", debugstr_w(szDest));
            RegCloseKey(hkey);
            return TRUE;
        }
        RegCloseKey(hkey);
    }


    TRACE("no default verb!\n");
    return FALSE;
}

BOOL HCR_GetExecuteCommandW(HKEY hkeyClass, LPCWSTR szClass, LPCWSTR szVerb, LPWSTR szDest, DWORD len)
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
        wcscpy(sTemp, swShell);
        wcscat(sTemp, sTempVerb);
        wcscat(sTemp, swCommand);
        ret = (ERROR_SUCCESS == SHGetValueW(hkeyClass, sTemp, NULL, NULL, szDest, &len));
    }
    if (szClass)
        RegCloseKey(hkeyClass);

    TRACE("-- %s\n", debugstr_w(szDest));
    return ret;
}

static BOOL HCR_RegOpenClassIDKey(REFIID riid, HKEY *hkey)
{
    WCHAR xriid[50];
    swprintf(xriid, L"CLSID\\{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             riid.Data1, riid.Data2, riid.Data3,
             riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3],
             riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7] );

    TRACE("%S\n", xriid);

    return (RegOpenKeyExW(HKEY_CLASSES_ROOT, xriid, 0, KEY_READ, hkey) == ERROR_SUCCESS);
}

/***************************************************************************************
*    HCR_GetIcon    [internal]
*
* Gets the icon for a filetype, szName can be NULL, in which case the default icon is loaded
*/
static BOOL HCR_RegGetIconW(HKEY hkey, LPWSTR szDest, LPCWSTR szName, DWORD len, int* picon_idx)
{
    DWORD dwType;
    WCHAR sTemp[MAX_PATH];
    WCHAR sNum[7];

    if (!RegQueryValueExW(hkey, szName, 0, &dwType, (LPBYTE)szDest, &len))
    {
        if (dwType == REG_EXPAND_SZ)
        {
            ExpandEnvironmentStringsW(szDest, sTemp, MAX_PATH);
            lstrcpynW(szDest, sTemp, len);
        }
        if (ParseFieldW (szDest, 2, sNum, _countof(sNum)))
            *picon_idx = atoiW(sNum);
        else
            *picon_idx = 0; /* sometimes the icon number is missing */
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
            *picon_idx = atoi(sNum);
        else
            *picon_idx = 0; /* sometimes the icon number is missing */
        ParseFieldA (szDest, 1, szDest, len);
        PathUnquoteSpacesA(szDest);
        return TRUE;
    }
    return FALSE;
}

BOOL HCR_GetIconW(LPCWSTR szClass, LPWSTR szDest, LPCWSTR szName, DWORD len, int* picon_idx)
{
    static const WCHAR swDefaultIcon[] = L"\\DefaultIcon";
    HKEY hKey;
    WCHAR sTemp[MAX_PATH];
    BOOL ret = FALSE;

    TRACE("%s\n", debugstr_w(szClass) );

    lstrcpynW(sTemp, szClass, MAX_PATH);
    wcscat(sTemp, swDefaultIcon);

    if (!RegOpenKeyExW(HKEY_CLASSES_ROOT, sTemp, 0, KEY_READ, &hKey))
    {
        ret = HCR_RegGetIconW(hKey, szDest, szName, len, picon_idx);
        RegCloseKey(hKey);
    }

    if(ret)
        TRACE("-- %s %i\n", debugstr_w(szDest), *picon_idx);
    else
        TRACE("-- not found\n");

    return ret;
}

BOOL HCR_GetIconA(LPCSTR szClass, LPSTR szDest, LPCSTR szName, DWORD len, int* picon_idx)
{
    HKEY hKey;
    char sTemp[MAX_PATH];
    BOOL ret = FALSE;

    TRACE("%s\n", szClass );

    sprintf(sTemp, "%s\\DefaultIcon", szClass);

    if (!RegOpenKeyExA(HKEY_CLASSES_ROOT, sTemp, 0, KEY_READ, &hKey))
    {
        ret = HCR_RegGetIconA(hKey, szDest, szName, len, picon_idx);
        RegCloseKey(hKey);
    }
    TRACE("-- %s %i\n", szDest, *picon_idx);
    return ret;
}

BOOL HCR_GetIconFromGUIDW(REFIID riid, LPWSTR szDest, LPWSTR szName, DWORD len, int* picon_idx)
{
    HKEY hKey;
    BOOL ret = FALSE;

    if (HCR_RegOpenClassIDKey(riid, &hKey))
    {
        ret = HCR_RegGetIconW(hKey, szDest, szName, len, picon_idx);
        RegCloseKey(hKey);
    }
    TRACE("-- %s %i\n", debugstr_w(szDest), *picon_idx);
    return ret;
}

/***************************************************************************************
*    HCR_GetClassName    [internal]
*
* Gets the name of a registered class
*/
static const WCHAR swEmpty[] = {0};

BOOL HCR_GetClassNameW(REFIID riid, LPWSTR szDest, DWORD len)
{
    HKEY hKey;
    BOOL ret = FALSE;
    DWORD buflen = len;
    WCHAR szName[100];
    LPOLESTR pStr;

    szDest[0] = 0;

    if (StringFromCLSID(riid, &pStr) == S_OK)
    {
        DWORD dwLen = buflen * sizeof(WCHAR);
        swprintf(szName, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\%s", pStr);
        if (RegGetValueW(HKEY_CURRENT_USER, szName, NULL, RRF_RT_REG_SZ, NULL, (PVOID)szDest, &dwLen) == ERROR_SUCCESS)
        {
            ret = TRUE;
        }
        CoTaskMemFree(pStr);
    }
    if (!ret && HCR_RegOpenClassIDKey(riid, &hKey))
    {
        static const WCHAR wszLocalizedString[] = L"LocalizedString";

        if (RegLoadMUIStringW(hKey, wszLocalizedString, szDest, len, NULL, 0, NULL) == ERROR_SUCCESS ||
            RegQueryValueExW(hKey, swEmpty, 0, NULL, (LPBYTE)szDest, &len) == ERROR_SUCCESS)
        {
            ret = TRUE;
        }
        RegCloseKey(hKey);
    }

    if (!ret || !szDest[0])
    {
        if(IsEqualIID(riid, CLSID_ShellDesktop))
        {
            if (LoadStringW(shell32_hInstance, IDS_DESKTOP, szDest, buflen))
                ret = TRUE;
        }
        else if (IsEqualIID(riid, CLSID_MyComputer))
        {
            if(LoadStringW(shell32_hInstance, IDS_MYCOMPUTER, szDest, buflen))
                ret = TRUE;
        }
        else if (IsEqualIID(riid, CLSID_MyDocuments))
        {
            if(LoadStringW(shell32_hInstance, IDS_PERSONAL, szDest, buflen))
                ret = TRUE;
        }
        else if (IsEqualIID(riid, CLSID_RecycleBin))
        {
            if(LoadStringW(shell32_hInstance, IDS_RECYCLEBIN_FOLDER_NAME, szDest, buflen))
                ret = TRUE;
        }
        else if (IsEqualIID(riid, CLSID_ControlPanel))
        {
            if(LoadStringW(shell32_hInstance, IDS_CONTROLPANEL, szDest, buflen))
                ret = TRUE;
        }
        else if (IsEqualIID(riid, CLSID_AdminFolderShortcut))
        {
            if(LoadStringW(shell32_hInstance, IDS_ADMINISTRATIVETOOLS, szDest, buflen))
                ret = TRUE;
        }
    }

    TRACE("-- %s\n", debugstr_w(szDest));
    return ret;
}

BOOL HCR_GetClassNameA(REFIID riid, LPSTR szDest, DWORD len)
{   HKEY hKey;
    BOOL ret = FALSE;
    DWORD buflen = len;

    szDest[0] = 0;
    if (HCR_RegOpenClassIDKey(riid, &hKey))
    {
        if (!RegLoadMUIStringA(hKey, "LocalizedString", szDest, len, NULL, 0, NULL) ||
            !RegQueryValueExA(hKey, "", 0, NULL, (LPBYTE)szDest, &len))
        {
            ret = TRUE;
        }
        RegCloseKey(hKey);
    }

    if (!ret || !szDest[0])
    {
        if(IsEqualIID(riid, CLSID_ShellDesktop))
        {
            if (LoadStringA(shell32_hInstance, IDS_DESKTOP, szDest, buflen))
                ret = TRUE;
        }
        else if (IsEqualIID(riid, CLSID_MyComputer))
        {
            if(LoadStringA(shell32_hInstance, IDS_MYCOMPUTER, szDest, buflen))
                ret = TRUE;
        }
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
    static const WCHAR wszAttributes[] = L"Attributes";
    static const WCHAR wszCallForAttributes[] = L"CallForAttributes";
    WCHAR wszShellFolderKey[] = L"CLSID\\{00021400-0000-0000-C000-000000000046}\\ShellFolder";

    TRACE("(pidlFolder=%p, pdwAttributes=%p)\n", pidlFolder, pdwAttributes);

    if (!_ILIsPidlSimple(pidlFolder))
    {
        ERR("should be called for simple PIDL's only!\n");
        return FALSE;
    }

    if (!_ILIsDesktop(pidlFolder))
    {
        if (FAILED(StringFromCLSID(*_ILGetGUIDPointer(pidlFolder), &pwszCLSID)))
            return FALSE;
        memcpy(&wszShellFolderKey[6], pwszCLSID, 38 * sizeof(WCHAR));
        CoTaskMemFree(pwszCLSID);
    }

    lResult = RegOpenKeyExW(HKEY_CLASSES_ROOT, wszShellFolderKey, 0, KEY_READ, &hSFKey);
    if (lResult != ERROR_SUCCESS)
    {
        ERR("Cannot open key: %ls\n", wszShellFolderKey);
        return FALSE;
    }

    dwLen = sizeof(DWORD);
    lResult = RegQueryValueExW(hSFKey, wszCallForAttributes, 0, NULL, (LPBYTE)&dwTemp, &dwLen);
    if ((lResult == ERROR_SUCCESS) && (dwTemp & *pdwAttributes))
    {
        CComPtr<IShellFolder> psfDesktop;
        CComPtr<IShellFolder> psfFolder;
        HRESULT hr;

        RegCloseKey(hSFKey);
        hr = SHGetDesktopFolder(&psfDesktop);
        if (SUCCEEDED(hr))
        {
            hr = psfDesktop->BindToObject(pidlFolder, NULL, IID_PPV_ARG(IShellFolder,&psfFolder));
            if (SUCCEEDED(hr))
                hr = psfFolder->GetAttributesOf(0, NULL, pdwAttributes);
        }
        if (FAILED(hr))
            return FALSE;
    }
    else
    {
        lResult = RegQueryValueExW(hSFKey, wszAttributes, 0, NULL, (LPBYTE)&dwTemp, &dwLen);
        RegCloseKey(hSFKey);
        if (lResult == ERROR_SUCCESS)
            *pdwAttributes &= dwTemp;
        else
            return FALSE;
    }

    TRACE("-- *pdwAttributes == 0x%08x\n", *pdwAttributes);

    return TRUE;
}
