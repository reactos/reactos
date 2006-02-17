/*
 * Copyright 2002 Mike McCormack for CodeWeavers
 * Copyright 2005 Juan Lang
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
#include <stdio.h>
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winreg.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

static const WCHAR DllW[] = { 'D','l','l',0 };
CRITICAL_SECTION funcSetCS;
struct list funcSets;

struct OIDFunctionSet
{
    LPSTR name;
    CRITICAL_SECTION cs; /* protects functions */
    struct list functions;
    struct list next;
};

struct OIDFunction
{
    DWORD encoding;
    CRYPT_OID_FUNC_ENTRY entry;
    struct list next;
};

void CRYPT_InitFunctionSets(void)
{
    InitializeCriticalSection(&funcSetCS);
    list_init(&funcSets);
}

void CRYPT_FreeFunctionSets(void)
{
    struct OIDFunctionSet *setCursor, *setNext;

    LIST_FOR_EACH_ENTRY_SAFE(setCursor, setNext, &funcSets,
     struct OIDFunctionSet, next)
    {
        struct OIDFunction *functionCursor, *funcNext;

        list_remove(&setCursor->next);
        CryptMemFree(setCursor->name);
        CryptMemFree(setCursor);
        LIST_FOR_EACH_ENTRY_SAFE(functionCursor, funcNext,
         &setCursor->functions, struct OIDFunction, next)
        {
            list_remove(&functionCursor->next);
            CryptMemFree(functionCursor);
        }
        DeleteCriticalSection(&setCursor->cs);
    }
    DeleteCriticalSection(&funcSetCS);
}

/* There is no free function associated with this; therefore, the sets are
 * freed when crypt32.dll is unloaded.
 */
HCRYPTOIDFUNCSET WINAPI CryptInitOIDFunctionSet(LPCSTR pszFuncName,
 DWORD dwFlags)
{
    struct OIDFunctionSet *cursor, *ret = NULL;

    TRACE("(%s, %lx)\n", debugstr_a(pszFuncName), dwFlags);

    EnterCriticalSection(&funcSetCS);
    LIST_FOR_EACH_ENTRY(cursor, &funcSets, struct OIDFunctionSet, next)
    {
        if (!strcasecmp(pszFuncName, cursor->name))
        {
            ret = (HCRYPTOIDFUNCSET)cursor;
            break;
        }
    }
    if (!ret)
    {
        ret = CryptMemAlloc(sizeof(struct OIDFunctionSet));
        if (ret)
        {
            memset(ret, 0, sizeof(*ret));
            ret->name = CryptMemAlloc(strlen(pszFuncName) + 1);
            if (ret->name)
            {
                InitializeCriticalSection(&ret->cs);
                list_init(&ret->functions);
                strcpy(ret->name, pszFuncName);
                list_add_tail(&funcSets, &ret->next);
            }
            else
            {
                CryptMemFree(ret);
                ret = NULL;
            }
        }
    }
    LeaveCriticalSection(&funcSetCS);

    return (HCRYPTOIDFUNCSET)ret;
}

static char *CRYPT_GetKeyName(DWORD dwEncodingType, LPCSTR pszFuncName,
 LPCSTR pszOID)
{
    static const char szEncodingTypeFmt[] =
     "Software\\Microsoft\\Cryptography\\OID\\EncodingType %ld\\%s\\%s";
    UINT len;
    char numericOID[7]; /* enough for "#65535" */
    const char *oid;
    LPSTR szKey;

    /* MSDN says the encoding type is a mask, but it isn't treated that way.
     * (E.g., if dwEncodingType were 3, the key names "EncodingType 1" and
     * "EncodingType 2" would be expected if it were a mask.  Instead native
     * stores values in "EncodingType 3".
     */
    if (!HIWORD(pszOID))
    {
        snprintf(numericOID, sizeof(numericOID), "#%d", LOWORD(pszOID));
        oid = numericOID;
    }
    else
        oid = pszOID;

    /* This is enough: the lengths of the two string parameters are explicitly
     * counted, and we need up to five additional characters for the encoding
     * type.  These are covered by the "%d", "%s", and "%s" characters in the
     * format specifier that are removed by sprintf.
     */
    len = sizeof(szEncodingTypeFmt) + lstrlenA(pszFuncName) + lstrlenA(oid);
    szKey = CryptMemAlloc(len);
    if (szKey)
        sprintf(szKey, szEncodingTypeFmt, dwEncodingType, pszFuncName, oid);
    return szKey;
}

BOOL WINAPI CryptGetDefaultOIDDllList(HCRYPTOIDFUNCSET hFuncSet,
 DWORD dwEncodingType, LPWSTR pwszDllList, DWORD *pcchDllList)
{
    BOOL ret = TRUE;
    struct OIDFunctionSet *set = (struct OIDFunctionSet *)hFuncSet;
    char *keyName;
    HKEY key;
    long rc;

    TRACE("(%p, %ld, %p, %p)\n", hFuncSet, dwEncodingType, pwszDllList,
     pcchDllList);

    keyName = CRYPT_GetKeyName(dwEncodingType, set->name, "DEFAULT");
    rc = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keyName, 0, NULL, 0,
     KEY_READ, NULL, &key, NULL);
    if (!rc)
    {
        DWORD size = *pcchDllList * sizeof(WCHAR);

        rc = RegQueryValueExW(key, DllW, NULL, NULL, (LPBYTE)pwszDllList,
         &size);
        if (!rc)
            *pcchDllList = size / sizeof(WCHAR);
        else
        {
            /* No value, return an empty list */
            if (*pcchDllList)
                *pwszDllList = '\0';
            *pcchDllList = 1;
        }
        RegCloseKey(key);
    }
    else
    {
        SetLastError(rc);
        ret = FALSE;
    }
    CryptMemFree(keyName);

    return ret;
}

BOOL WINAPI CryptInstallOIDFunctionAddress(HMODULE hModule,
 DWORD dwEncodingType, LPCSTR pszFuncName, DWORD cFuncEntry,
 const CRYPT_OID_FUNC_ENTRY rgFuncEntry[], DWORD dwFlags)
{
    BOOL ret = TRUE;
    struct OIDFunctionSet *set;

    TRACE("(%p, %ld, %s, %ld, %p, %08lx)\n", hModule, dwEncodingType,
     debugstr_a(pszFuncName), cFuncEntry, rgFuncEntry, dwFlags);

    set = (struct OIDFunctionSet *)CryptInitOIDFunctionSet(pszFuncName, 0);
    if (set)
    {
        DWORD i;

        EnterCriticalSection(&set->cs);
        for (i = 0; ret && i < cFuncEntry; i++)
        {
            struct OIDFunction *func;

            if (HIWORD(rgFuncEntry[i].pszOID))
                func = CryptMemAlloc(sizeof(struct OIDFunction)
                 + strlen(rgFuncEntry[i].pszOID) + 1);
            else
                func = CryptMemAlloc(sizeof(struct OIDFunction));
            if (func)
            {
                func->encoding = dwEncodingType;
                if (HIWORD(rgFuncEntry[i].pszOID))
                {
                    func->entry.pszOID = (LPSTR)((LPBYTE)func + sizeof(*func));
                    strcpy((LPSTR)func->entry.pszOID, rgFuncEntry[i].pszOID);
                }
                else
                    func->entry.pszOID = rgFuncEntry[i].pszOID;
                func->entry.pvFuncAddr = rgFuncEntry[i].pvFuncAddr;
                list_add_tail(&set->functions, &func->next);
            }
            else
                ret = FALSE;
        }
        LeaveCriticalSection(&set->cs);
    }
    else
        ret = FALSE;
    return ret;
}

static BOOL CRYPT_GetFuncFromReg(DWORD dwEncodingType, LPCSTR pszOID,
 LPCSTR szFuncName, LPVOID *ppvFuncAddr, HCRYPTOIDFUNCADDR *phFuncAddr)
{
    BOOL ret = FALSE;
    char *keyName;
    const char *funcName;
    HKEY key;
    long rc;

    keyName = CRYPT_GetKeyName(dwEncodingType, szFuncName, pszOID);
    rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyName, 0, KEY_READ, &key);
    if (!rc)
    {
        DWORD type, size = 0;

        rc = RegQueryValueExA(key, "FuncName", NULL, &type, NULL, &size);
        if (rc == ERROR_MORE_DATA && type == REG_SZ)
        {
            funcName = CryptMemAlloc(size);
            rc = RegQueryValueExA(key, "FuncName", NULL, &type,
             (LPBYTE)funcName, &size);
        }
        else
            funcName = szFuncName;
        rc = RegQueryValueExW(key, DllW, NULL, &type, NULL, &size);
        if (rc == ERROR_MORE_DATA && type == REG_SZ)
        {
            LPWSTR dllName = CryptMemAlloc(size);

            if (dllName)
            {
                rc = RegQueryValueExW(key, DllW, NULL, NULL,
                 (LPBYTE)dllName, &size);
                if (!rc)
                {
                    HMODULE lib;

                    /* This is a bit of a hack; MSDN describes a more
                     * complicated unload routine than this will allow.
                     * Still, this seems to suffice for now.
                     */
                    lib = LoadLibraryW(dllName);
                    if (lib)
                    {
                        *ppvFuncAddr = GetProcAddress(lib, szFuncName);
                        if (*ppvFuncAddr)
                        {
                            *phFuncAddr = (HCRYPTOIDFUNCADDR)lib;
                            ret = TRUE;
                        }
                        else
                        {
                            /* Unload the library, the caller doesn't want
                             * to unload it when the return value is NULL.
                             */
                            FreeLibrary(lib);
                        }
                    }
                }
                else
                    SetLastError(rc);
                CryptMemFree(dllName);
            }
        }
        else
            SetLastError(rc);
        if (funcName != szFuncName)
            CryptMemFree((char *)funcName);
        RegCloseKey(key);
    }
    else
        SetLastError(rc);
    CryptMemFree(keyName);
    return ret;
}

BOOL WINAPI CryptGetOIDFunctionAddress(HCRYPTOIDFUNCSET hFuncSet,
 DWORD dwEncodingType, LPCSTR pszOID, DWORD dwFlags, void **ppvFuncAddr,
 HCRYPTOIDFUNCADDR *phFuncAddr)
{
    BOOL ret = FALSE;
    struct OIDFunctionSet *set = (struct OIDFunctionSet *)hFuncSet;

    TRACE("(%p, %ld, %s, %08lx, %p, %p)\n", hFuncSet, dwEncodingType,
     debugstr_a(pszOID), dwFlags, ppvFuncAddr, phFuncAddr);

    *ppvFuncAddr = NULL;
    if (!(dwFlags & CRYPT_GET_INSTALLED_OID_FUNC_FLAG))
    {
        struct OIDFunction *function;

        EnterCriticalSection(&set->cs);
        LIST_FOR_EACH_ENTRY(function, &set->functions, struct OIDFunction, next)
        {
            if (function->encoding == dwEncodingType)
            {
                if (HIWORD(pszOID))
                {
                    if (HIWORD(function->entry.pszOID &&
                     !strcasecmp(function->entry.pszOID, pszOID)))
                    {
                        *ppvFuncAddr = function->entry.pvFuncAddr;
                        *phFuncAddr = NULL; /* FIXME: what should it be? */
                        ret = TRUE;
                        break;
                    }
                }
                else if (function->entry.pszOID == pszOID)
                {
                    *ppvFuncAddr = function->entry.pvFuncAddr;
                    *phFuncAddr = NULL; /* FIXME: what should it be? */
                    ret = TRUE;
                    break;
                }
            }
        }
        LeaveCriticalSection(&set->cs);
    }
    if (!*ppvFuncAddr)
        ret = CRYPT_GetFuncFromReg(dwEncodingType, pszOID, set->name,
         ppvFuncAddr, phFuncAddr);
    return ret;
}

BOOL WINAPI CryptFreeOIDFunctionAddress(HCRYPTOIDFUNCADDR hFuncAddr,
 DWORD dwFlags)
{
    TRACE("(%p, %08lx)\n", hFuncAddr, dwFlags);

    /* FIXME: as MSDN states, need to check for DllCanUnloadNow in the DLL,
     * and only unload it if it can be unloaded.  Also need to implement ref
     * counting on the functions.
     */
    FreeLibrary((HMODULE)hFuncAddr);
    return TRUE;
}

BOOL WINAPI CryptRegisterDefaultOIDFunction(DWORD dwEncodingType,
 LPCSTR pszFuncName, DWORD dwIndex, LPCWSTR pwszDll)
{
    FIXME("(%lx,%s,%lx,%s) stub!\n", dwEncodingType, pszFuncName, dwIndex,
          debugstr_w(pwszDll));
    return FALSE;
}

BOOL WINAPI CryptUnregisterDefaultOIDFunction(DWORD dwEncodingType,
 LPCSTR pszFuncName, LPCWSTR pwszDll)
{
    FIXME("(%lx %s %s): stub\n", dwEncodingType, debugstr_a(pszFuncName),
     debugstr_w(pwszDll));
    return FALSE;
}

BOOL WINAPI CryptGetDefaultOIDFunctionAddress(HCRYPTOIDFUNCSET hFuncSet,
 DWORD dwEncodingType, LPCWSTR pwszDll, DWORD dwFlags, void *ppvFuncAddr,
 HCRYPTOIDFUNCADDR *phFuncAddr)
{
    FIXME("(%p, %ld, %s, %08lx, %p, %p): stub\n", hFuncSet, dwEncodingType,
     debugstr_w(pwszDll), dwFlags, ppvFuncAddr, phFuncAddr);
    return FALSE;
}

BOOL WINAPI CryptRegisterOIDFunction(DWORD dwEncodingType, LPCSTR pszFuncName,
                  LPCSTR pszOID, LPCWSTR pwszDll, LPCSTR pszOverrideFuncName)
{
    LONG r;
    HKEY hKey;
    LPSTR szKey;

    TRACE("(%lx, %s, %s, %s, %s)\n", dwEncodingType, pszFuncName, pszOID,
     debugstr_w(pwszDll), pszOverrideFuncName);

    /* This only registers functions for encoding certs, not messages */
    if (!GET_CERT_ENCODING_TYPE(dwEncodingType))
        return TRUE;

    /* Native does nothing pwszDll is NULL */
    if (!pwszDll)
        return TRUE;

    /* I'm not matching MS bug for bug here, because I doubt any app depends on
     * it:  native "succeeds" if pszFuncName is NULL, but the nonsensical entry
     * it creates would never be used.
     */
    if (!pszFuncName || !pszOID)
    {
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        return FALSE;
    }

    szKey = CRYPT_GetKeyName(dwEncodingType, pszFuncName, pszOID);
    TRACE("Key name is %s\n", debugstr_a(szKey));

    if (!szKey)
        return FALSE;

    r = RegCreateKeyA(HKEY_LOCAL_MACHINE, szKey, &hKey);
    CryptMemFree(szKey);
    if(r != ERROR_SUCCESS)
        return FALSE;

    /* write the values */
    if (pszOverrideFuncName)
        RegSetValueExA(hKey, "FuncName", 0, REG_SZ,
         (const BYTE*)pszOverrideFuncName, lstrlenA(pszOverrideFuncName) + 1);
    RegSetValueExW(hKey, DllW, 0, REG_SZ, (const BYTE*) pwszDll,
     (lstrlenW(pwszDll) + 1) * sizeof (WCHAR));

    RegCloseKey(hKey);
    return TRUE;
}

BOOL WINAPI CryptUnregisterOIDFunction(DWORD dwEncodingType, LPCSTR pszFuncName,
 LPCSTR pszOID)
{
    LPSTR szKey;
    LONG rc;

    TRACE("%lx %s %s\n", dwEncodingType, pszFuncName, pszOID);

    if (!GET_CERT_ENCODING_TYPE(dwEncodingType))
        return TRUE;

    if (!pszFuncName || !pszOID)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    szKey = CRYPT_GetKeyName(dwEncodingType, pszFuncName, pszOID);
    rc = RegDeleteKeyA(HKEY_LOCAL_MACHINE, szKey);
    CryptMemFree(szKey);
    if (rc)
        SetLastError(rc);
    return rc ? FALSE : TRUE;
}

BOOL WINAPI CryptGetOIDFunctionValue(DWORD dwEncodingType, LPCSTR pszFuncName,
 LPCSTR pszOID, LPCWSTR pwszValueName, DWORD *pdwValueType, BYTE *pbValueData,
 DWORD *pcbValueData)
{
    LPSTR szKey;
    LONG rc;
    HKEY hKey;

    TRACE("%lx %s %s %s %p %p %p\n", dwEncodingType, debugstr_a(pszFuncName),
     debugstr_a(pszOID), debugstr_w(pwszValueName), pdwValueType, pbValueData,
     pcbValueData);

    if (!GET_CERT_ENCODING_TYPE(dwEncodingType))
        return TRUE;

    if (!pszFuncName || !pszOID || !pwszValueName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    szKey = CRYPT_GetKeyName(dwEncodingType, pszFuncName, pszOID);
    rc = RegOpenKeyA(HKEY_LOCAL_MACHINE, szKey, &hKey);
    CryptMemFree(szKey);
    if (rc)
        SetLastError(rc);
    else
    {
        rc = RegQueryValueExW(hKey, pwszValueName, NULL, pdwValueType,
         pbValueData, pcbValueData);
        if (rc)
            SetLastError(rc);
        RegCloseKey(hKey);
    }
    return rc ? FALSE : TRUE;
}

BOOL WINAPI CryptSetOIDFunctionValue(DWORD dwEncodingType, LPCSTR pszFuncName,
 LPCSTR pszOID, LPCWSTR pwszValueName, DWORD dwValueType,
 const BYTE *pbValueData, DWORD cbValueData)
{
    LPSTR szKey;
    LONG rc;
    HKEY hKey;

    TRACE("%lx %s %s %s %ld %p %ld\n", dwEncodingType, debugstr_a(pszFuncName),
     debugstr_a(pszOID), debugstr_w(pwszValueName), dwValueType, pbValueData,
     cbValueData);

    if (!GET_CERT_ENCODING_TYPE(dwEncodingType))
        return TRUE;

    if (!pszFuncName || !pszOID || !pwszValueName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    szKey = CRYPT_GetKeyName(dwEncodingType, pszFuncName, pszOID);
    rc = RegOpenKeyA(HKEY_LOCAL_MACHINE, szKey, &hKey);
    CryptMemFree(szKey);
    if (rc)
        SetLastError(rc);
    else
    {
        rc = RegSetValueExW(hKey, pwszValueName, 0, dwValueType, pbValueData,
         cbValueData);
        if (rc)
            SetLastError(rc);
        RegCloseKey(hKey);
    }
    return rc ? FALSE : TRUE;
}
