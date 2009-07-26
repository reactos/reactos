/*
 * Copyright 2002 Mike McCormack for CodeWeavers
 * Copyright 2005-2006 Juan Lang
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
#include "wine/port.h"

#include <stdio.h>
#include <stdarg.h>
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winreg.h"
#include "winuser.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "crypt32_private.h"
#include "cryptres.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

static const WCHAR DllW[] = { 'D','l','l',0 };

static void init_oid_info(void);
static void free_function_sets(void);
static void free_oid_info(void);

void crypt_oid_init(void)
{
    init_oid_info();
}

void crypt_oid_free(void)
{
    free_function_sets();
    free_oid_info();
}

static CRITICAL_SECTION funcSetCS;
static CRITICAL_SECTION_DEBUG funcSetCSDebug =
{
    0, 0, &funcSetCS,
    { &funcSetCSDebug.ProcessLocksList, &funcSetCSDebug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": funcSetCS") }
};
static CRITICAL_SECTION funcSetCS = { &funcSetCSDebug, -1, 0, 0, 0, 0 };
static struct list funcSets = { &funcSets, &funcSets };

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

static const WCHAR ROOT[] = {'R','O','O','T',0};
static const WCHAR MY[] = {'M','Y',0};
static const WCHAR CA[] = {'C','A',0};
static const WCHAR ADDRESSBOOK[] = {'A','D','D','R','E','S','S','B','O','O','K',0};
static const WCHAR TRUSTEDPUBLISHER[] = {'T','r','u','s','t','e','d','P','u','b','l','i','s','h','e','r',0};
static const LPCWSTR LocalizedKeys[] = {ROOT,MY,CA,ADDRESSBOOK,TRUSTEDPUBLISHER};
static WCHAR LocalizedNames[sizeof(LocalizedKeys)/sizeof(LocalizedKeys[0])][256];

static void free_function_sets(void)
{
    struct OIDFunctionSet *setCursor, *setNext;

    LIST_FOR_EACH_ENTRY_SAFE(setCursor, setNext, &funcSets,
     struct OIDFunctionSet, next)
    {
        struct OIDFunction *functionCursor, *funcNext;

        list_remove(&setCursor->next);
        CryptMemFree(setCursor->name);
        LIST_FOR_EACH_ENTRY_SAFE(functionCursor, funcNext,
         &setCursor->functions, struct OIDFunction, next)
        {
            list_remove(&functionCursor->next);
            CryptMemFree(functionCursor);
        }
        setCursor->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&setCursor->cs);
        CryptMemFree(setCursor);
    }
}

/* There is no free function associated with this; therefore, the sets are
 * freed when crypt32.dll is unloaded.
 */
HCRYPTOIDFUNCSET WINAPI CryptInitOIDFunctionSet(LPCSTR pszFuncName,
 DWORD dwFlags)
{
    struct OIDFunctionSet *cursor, *ret = NULL;

    TRACE("(%s, %x)\n", debugstr_a(pszFuncName), dwFlags);

    EnterCriticalSection(&funcSetCS);
    LIST_FOR_EACH_ENTRY(cursor, &funcSets, struct OIDFunctionSet, next)
    {
        if (!strcasecmp(pszFuncName, cursor->name))
        {
            ret = cursor;
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
                ret->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": OIDFunctionSet.cs");
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

    return ret;
}

static char *CRYPT_GetKeyName(DWORD dwEncodingType, LPCSTR pszFuncName,
 LPCSTR pszOID)
{
    static const char szEncodingTypeFmt[] =
     "Software\\Microsoft\\Cryptography\\OID\\EncodingType %d\\%s\\%s";
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
        sprintf(szKey, szEncodingTypeFmt,
         GET_CERT_ENCODING_TYPE(dwEncodingType), pszFuncName, oid);
    return szKey;
}

BOOL WINAPI CryptGetDefaultOIDDllList(HCRYPTOIDFUNCSET hFuncSet,
 DWORD dwEncodingType, LPWSTR pwszDllList, DWORD *pcchDllList)
{
    BOOL ret = TRUE;
    struct OIDFunctionSet *set = hFuncSet;
    char *keyName;
    HKEY key;
    LSTATUS rc;

    TRACE("(%p, %d, %p, %p)\n", hFuncSet, dwEncodingType, pwszDllList,
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
            if (pwszDllList && *pcchDllList)
                *pwszDllList = '\0';
            *pcchDllList = 1;
        }
        RegCloseKey(key);
    }
    else
    {
        /* No value, return an empty list */
        if (pwszDllList && *pcchDllList)
            *pwszDllList = '\0';
        *pcchDllList = 1;
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

    TRACE("(%p, %d, %s, %d, %p, %08x)\n", hModule, dwEncodingType,
     debugstr_a(pszFuncName), cFuncEntry, rgFuncEntry, dwFlags);

    set = CryptInitOIDFunctionSet(pszFuncName, 0);
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
                func->encoding = GET_CERT_ENCODING_TYPE(dwEncodingType);
                if (HIWORD(rgFuncEntry[i].pszOID))
                {
                    LPSTR oid;

                    oid = (LPSTR)((LPBYTE)func + sizeof(*func));
                    strcpy(oid, rgFuncEntry[i].pszOID);
                    func->entry.pszOID = oid;
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

struct FuncAddr
{
    HMODULE lib;
    LPWSTR  dllList;
    LPWSTR  currentDll;
};

static BOOL CRYPT_GetFuncFromReg(DWORD dwEncodingType, LPCSTR pszOID,
 LPCSTR szFuncName, LPVOID *ppvFuncAddr, HCRYPTOIDFUNCADDR *phFuncAddr)
{
    BOOL ret = FALSE;
    char *keyName;
    const char *funcName;
    HKEY key;
    LSTATUS rc;

    keyName = CRYPT_GetKeyName(dwEncodingType, szFuncName, pszOID);
    rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyName, 0, KEY_READ, &key);
    if (!rc)
    {
        DWORD type, size = 0;

        rc = RegQueryValueExA(key, "FuncName", NULL, &type, NULL, &size);
        if ((!rc || rc == ERROR_MORE_DATA) && type == REG_SZ)
        {
            funcName = CryptMemAlloc(size);
            rc = RegQueryValueExA(key, "FuncName", NULL, &type,
             (LPBYTE)funcName, &size);
        }
        else
            funcName = szFuncName;
        rc = RegQueryValueExW(key, DllW, NULL, &type, NULL, &size);
        if ((!rc || rc == ERROR_MORE_DATA) && type == REG_SZ)
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
                        *ppvFuncAddr = GetProcAddress(lib, funcName);
                        if (*ppvFuncAddr)
                        {
                            struct FuncAddr *addr =
                             CryptMemAlloc(sizeof(struct FuncAddr));

                            if (addr)
                            {
                                addr->lib = lib;
                                addr->dllList = addr->currentDll = NULL;
                                *phFuncAddr = addr;
                                ret = TRUE;
                            }
                            else
                            {
                                *phFuncAddr = NULL;
                                FreeLibrary(lib);
                            }
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
    struct OIDFunctionSet *set = hFuncSet;

    TRACE("(%p, %d, %s, %08x, %p, %p)\n", hFuncSet, dwEncodingType,
     debugstr_a(pszOID), dwFlags, ppvFuncAddr, phFuncAddr);

    *ppvFuncAddr = NULL;
    if (!(dwFlags & CRYPT_GET_INSTALLED_OID_FUNC_FLAG))
    {
        struct OIDFunction *function;

        EnterCriticalSection(&set->cs);
        LIST_FOR_EACH_ENTRY(function, &set->functions, struct OIDFunction, next)
        {
            if (function->encoding == GET_CERT_ENCODING_TYPE(dwEncodingType))
            {
                if (HIWORD(pszOID))
                {
                    if (HIWORD(function->entry.pszOID) &&
                     !strcasecmp(function->entry.pszOID, pszOID))
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
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CryptFreeOIDFunctionAddress(HCRYPTOIDFUNCADDR hFuncAddr,
 DWORD dwFlags)
{
    TRACE("(%p, %08x)\n", hFuncAddr, dwFlags);

    /* FIXME: as MSDN states, need to check for DllCanUnloadNow in the DLL,
     * and only unload it if it can be unloaded.  Also need to implement ref
     * counting on the functions.
     */
    if (hFuncAddr)
    {
        struct FuncAddr *addr = hFuncAddr;

        CryptMemFree(addr->dllList);
        FreeLibrary(addr->lib);
        CryptMemFree(addr);
    }
    return TRUE;
}

static BOOL CRYPT_GetFuncFromDll(LPCWSTR dll, LPCSTR func, HMODULE *lib,
 void **ppvFuncAddr)
{
    BOOL ret = FALSE;

    *lib = LoadLibraryW(dll);
    if (*lib)
    {
        *ppvFuncAddr = GetProcAddress(*lib, func);
        if (*ppvFuncAddr)
            ret = TRUE;
        else
        {
            FreeLibrary(*lib);
            *lib = NULL;
        }
    }
    return ret;
}

BOOL WINAPI CryptGetDefaultOIDFunctionAddress(HCRYPTOIDFUNCSET hFuncSet,
 DWORD dwEncodingType, LPCWSTR pwszDll, DWORD dwFlags, void **ppvFuncAddr,
 HCRYPTOIDFUNCADDR *phFuncAddr)
{
    struct OIDFunctionSet *set = hFuncSet;
    BOOL ret = FALSE;

    TRACE("(%p, %d, %s, %08x, %p, %p)\n", hFuncSet, dwEncodingType,
     debugstr_w(pwszDll), dwFlags, ppvFuncAddr, phFuncAddr);

    if (pwszDll)
    {
        HMODULE lib;

        *phFuncAddr = NULL;
        ret = CRYPT_GetFuncFromDll(pwszDll, set->name, &lib, ppvFuncAddr);
        if (ret)
        {
            struct FuncAddr *addr = CryptMemAlloc(sizeof(struct FuncAddr));

            if (addr)
            {
                addr->lib = lib;
                addr->dllList = addr->currentDll = NULL;
                *phFuncAddr = addr;
            }
            else
            {
                FreeLibrary(lib);
                *ppvFuncAddr = NULL;
                SetLastError(ERROR_OUTOFMEMORY);
                ret = FALSE;
            }
        }
        else
            SetLastError(ERROR_FILE_NOT_FOUND);
    }
    else
    {
        struct FuncAddr *addr = *phFuncAddr;

        if (!addr)
        {
            DWORD size;

            ret = CryptGetDefaultOIDDllList(hFuncSet, dwEncodingType, NULL,
             &size);
            if (ret)
            {
                LPWSTR dllList = CryptMemAlloc(size * sizeof(WCHAR));

                if (dllList)
                {
                    ret = CryptGetDefaultOIDDllList(hFuncSet, dwEncodingType,
                     dllList, &size);
                    if (ret)
                    {
                        addr = CryptMemAlloc(sizeof(struct FuncAddr));
                        if (addr)
                        {
                            addr->dllList = dllList;
                            addr->currentDll = dllList;
                            addr->lib = NULL;
                            *phFuncAddr = addr;
                        }
                        else
                        {
                            CryptMemFree(dllList);
                            SetLastError(ERROR_OUTOFMEMORY);
                            ret = FALSE;
                        }
                    }
                }
                else
                {
                    SetLastError(ERROR_OUTOFMEMORY);
                    ret = FALSE;
                }
            }
        }
        if (addr)
        {
            if (!*addr->currentDll)
            {
                CryptFreeOIDFunctionAddress(*phFuncAddr, 0);
                SetLastError(ERROR_FILE_NOT_FOUND);
                *phFuncAddr = NULL;
                ret = FALSE;
            }
            else
            {
                /* FIXME: as elsewhere, can't free until DllCanUnloadNow says
                 * it's possible, and should defer unloading for some time to
                 * avoid repeated LoadLibrary/FreeLibrary on the same dll.
                 */
                FreeLibrary(addr->lib);
                ret = CRYPT_GetFuncFromDll(addr->currentDll, set->name,
                 &addr->lib, ppvFuncAddr);
                if (ret)
                {
                    /* Move past the current DLL */
                    addr->currentDll += lstrlenW(addr->currentDll) + 1;
                    *phFuncAddr = addr;
                }
                else
                {
                    CryptFreeOIDFunctionAddress(*phFuncAddr, 0);
                    SetLastError(ERROR_FILE_NOT_FOUND);
                    *phFuncAddr = NULL;
                }
            }
        }
    }
    return ret;
}

/***********************************************************************
 *             CryptRegisterOIDFunction (CRYPT32.@)
 *
 * Register the DLL and the functions it uses to cover the combination
 * of encoding type, functionname and OID.
 *
 * PARAMS
 *  dwEncodingType       [I] Encoding type to be used.
 *  pszFuncName          [I] Name of the function to be registered.
 *  pszOID               [I] OID of the function (numeric or string).
 *  pwszDll              [I] The DLL that is to be registered.
 *  pszOverrideFuncName  [I] Name of the function in the DLL.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE. (Look at GetLastError()).
 *
 * NOTES
 *  Registry errors are always reported via SetLastError().
 */
BOOL WINAPI CryptRegisterOIDFunction(DWORD dwEncodingType, LPCSTR pszFuncName,
                  LPCSTR pszOID, LPCWSTR pwszDll, LPCSTR pszOverrideFuncName)
{
    LONG r;
    HKEY hKey;
    LPSTR szKey;

    TRACE("(%x, %s, %s, %s, %s)\n", dwEncodingType, pszFuncName,
     debugstr_a(pszOID), debugstr_w(pwszDll), pszOverrideFuncName);

    /* Native does nothing pwszDll is NULL */
    if (!pwszDll)
        return TRUE;

    /* I'm not matching MS bug for bug here, because I doubt any app depends on
     * it:  native "succeeds" if pszFuncName is NULL, but the nonsensical entry
     * it creates would never be used.
     */
    if (!pszFuncName || !pszOID)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    szKey = CRYPT_GetKeyName(dwEncodingType, pszFuncName, pszOID);
    TRACE("Key name is %s\n", debugstr_a(szKey));

    if (!szKey)
        return FALSE;

    r = RegCreateKeyA(HKEY_LOCAL_MACHINE, szKey, &hKey);
    CryptMemFree(szKey);

    if (r != ERROR_SUCCESS) goto error_close_key;

    /* write the values */
    if (pszOverrideFuncName)
    {
        r = RegSetValueExA(hKey, "FuncName", 0, REG_SZ,
             (const BYTE*)pszOverrideFuncName, lstrlenA(pszOverrideFuncName) + 1);
        if (r != ERROR_SUCCESS) goto error_close_key;
    }
    r = RegSetValueExW(hKey, DllW, 0, REG_SZ, (const BYTE*) pwszDll,
         (lstrlenW(pwszDll) + 1) * sizeof (WCHAR));

error_close_key:

    RegCloseKey(hKey);

    if (r != ERROR_SUCCESS) 
    {
        SetLastError(r);
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *             CryptRegisterOIDInfo (CRYPT32.@)
 */
BOOL WINAPI CryptRegisterOIDInfo(PCCRYPT_OID_INFO pInfo, DWORD dwFlags)
{
    FIXME("(%p, %x): stub\n", pInfo, dwFlags );
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *             CryptUnregisterOIDFunction (CRYPT32.@)
 */
BOOL WINAPI CryptUnregisterOIDFunction(DWORD dwEncodingType, LPCSTR pszFuncName,
 LPCSTR pszOID)
{
    LPSTR szKey;
    LONG rc;

    TRACE("%x %s %s\n", dwEncodingType, debugstr_a(pszFuncName),
     debugstr_a(pszOID));

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

    TRACE("%x %s %s %s %p %p %p\n", dwEncodingType, debugstr_a(pszFuncName),
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

    TRACE("%x %s %s %s %d %p %d\n", dwEncodingType, debugstr_a(pszFuncName),
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

static LPCWSTR CRYPT_FindStringInMultiString(LPCWSTR multi, LPCWSTR toFind)
{
    LPCWSTR ret = NULL, ptr;

    for (ptr = multi; ptr && *ptr && !ret; ptr += lstrlenW(ptr) + 1)
    {
        if (!lstrcmpiW(ptr, toFind))
            ret = ptr;
    }
    return ret;
}

static DWORD CRYPT_GetMultiStringCharacterLen(LPCWSTR multi)
{
    DWORD ret;

    if (multi)
    {
        LPCWSTR ptr;

        /* Count terminating empty string */
        ret = 1;
        for (ptr = multi; *ptr; ptr += lstrlenW(ptr) + 1)
            ret += lstrlenW(ptr) + 1;
    }
    else
        ret = 0;
    return ret;
}

static LPWSTR CRYPT_AddStringToMultiString(LPWSTR multi, LPCWSTR toAdd,
 DWORD index)
{
    LPWSTR ret;

    if (!multi)
    {
        /* FIXME: ignoring index, is that okay? */
        ret = CryptMemAlloc((lstrlenW(toAdd) + 2) * sizeof(WCHAR));
        if (ret)
        {
            /* copy string, including NULL terminator */
            memcpy(ret, toAdd, (lstrlenW(toAdd) + 1) * sizeof(WCHAR));
            /* add terminating empty string */
            *(ret + lstrlenW(toAdd) + 1) = 0;
        }
    }
    else
    {
        DWORD len = CRYPT_GetMultiStringCharacterLen(multi);

        ret = CryptMemRealloc(multi, (len + lstrlenW(toAdd) + 1) *
         sizeof(WCHAR));
        if (ret)
        {
            LPWSTR spotToAdd;

            if (index == CRYPT_REGISTER_LAST_INDEX)
                spotToAdd = ret + len - 1;
            else
            {
                DWORD i;

                /* FIXME: if index is too large for the string, toAdd is
                 * added to the end.  Is that okay?
                 */
                for (i = 0, spotToAdd = ret; i < index && *spotToAdd;
                 spotToAdd += lstrlenW(spotToAdd) + 1)
                    ;
            }
            if (spotToAdd)
            {
                /* Copy existing string "right" */
                memmove(spotToAdd + lstrlenW(toAdd) + 1, spotToAdd,
                 (len - (spotToAdd - ret)) * sizeof(WCHAR));
                /* Copy new string */
                memcpy(spotToAdd, toAdd, (lstrlenW(toAdd) + 1) * sizeof(WCHAR));
            }
            else
            {
                CryptMemFree(ret);
                ret = NULL;
            }
        }
    }
    return ret;
}

static BOOL CRYPT_RemoveStringFromMultiString(LPWSTR multi, LPCWSTR toRemove)
{
    LPWSTR spotToRemove = (LPWSTR)CRYPT_FindStringInMultiString(multi,
     toRemove);
    BOOL ret;

    if (spotToRemove)
    {
        DWORD len = CRYPT_GetMultiStringCharacterLen(multi);

        /* Copy remainder of string "left" */
        memmove(spotToRemove, spotToRemove + lstrlenW(toRemove) + 1,
         (len - (spotToRemove - multi)) * sizeof(WCHAR));
        ret = TRUE;
    }
    else
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        ret = FALSE;
    }
    return ret;
}

static BOOL CRYPT_GetDefaultOIDKey(DWORD dwEncodingType, LPCSTR pszFuncName,
 PHKEY key)
{
    LPSTR keyName;
    LONG r;

    keyName = CRYPT_GetKeyName(dwEncodingType, pszFuncName, "DEFAULT");
    TRACE("Key name is %s\n", debugstr_a(keyName));

    if (!keyName)
        return FALSE;

    r = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keyName, 0, NULL, 0, KEY_ALL_ACCESS,
     NULL, key, NULL);
    CryptMemFree(keyName);
    if (r != ERROR_SUCCESS)
    {
        SetLastError(r);
        return FALSE;
    }
    return TRUE;
}

static LPWSTR CRYPT_GetDefaultOIDDlls(HKEY key)
{
    LONG r;
    DWORD type, size;
    LPWSTR dlls;

    r = RegQueryValueExW(key, DllW, NULL, &type, NULL, &size);
    if (r == ERROR_SUCCESS && type == REG_MULTI_SZ)
    {
        dlls = CryptMemAlloc(size);
        r = RegQueryValueExW(key, DllW, NULL, &type, (LPBYTE)dlls, &size);
        if (r != ERROR_SUCCESS)
        {
            CryptMemFree(dlls);
            dlls = NULL;
        }
    }
    else
        dlls = NULL;
    return dlls;
}

static inline BOOL CRYPT_SetDefaultOIDDlls(HKEY key, LPCWSTR dlls)
{
    DWORD len = CRYPT_GetMultiStringCharacterLen(dlls);
    LONG r;

    if ((r = RegSetValueExW(key, DllW, 0, REG_MULTI_SZ, (const BYTE *)dlls,
     len * sizeof (WCHAR))))
        SetLastError(r);
    return r == ERROR_SUCCESS;
}

/***********************************************************************
 *             CryptRegisterDefaultOIDFunction (CRYPT32.@)
 */
BOOL WINAPI CryptRegisterDefaultOIDFunction(DWORD dwEncodingType,
 LPCSTR pszFuncName, DWORD dwIndex, LPCWSTR pwszDll)
{
    HKEY key;
    LPWSTR dlls;
    BOOL ret = FALSE;

    TRACE("(%x, %s, %d, %s)\n", dwEncodingType, debugstr_a(pszFuncName),
     dwIndex, debugstr_w(pwszDll));

    if (!pwszDll)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    if (!CRYPT_GetDefaultOIDKey(dwEncodingType, pszFuncName, &key))
        return FALSE;

    dlls = CRYPT_GetDefaultOIDDlls(key);
    if (CRYPT_FindStringInMultiString(dlls, pwszDll))
        SetLastError(ERROR_FILE_EXISTS);
    else
    {
        dlls = CRYPT_AddStringToMultiString(dlls, pwszDll, dwIndex);
        if (dlls)
            ret = CRYPT_SetDefaultOIDDlls(key, dlls);
    }
    CryptMemFree(dlls);
    RegCloseKey(key);
    return ret;
}

BOOL WINAPI CryptUnregisterDefaultOIDFunction(DWORD dwEncodingType,
 LPCSTR pszFuncName, LPCWSTR pwszDll)
{
    HKEY key;
    LPWSTR dlls;
    BOOL ret;

    TRACE("(%x, %s, %s)\n", dwEncodingType, debugstr_a(pszFuncName),
     debugstr_w(pwszDll));

    if (!pwszDll)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    if (!CRYPT_GetDefaultOIDKey(dwEncodingType, pszFuncName, &key))
        return FALSE;

    dlls = CRYPT_GetDefaultOIDDlls(key);
    if ((ret = CRYPT_RemoveStringFromMultiString(dlls, pwszDll)))
        ret = CRYPT_SetDefaultOIDDlls(key, dlls);
    CryptMemFree(dlls);
    RegCloseKey(key);
    return ret;
}

static void oid_init_localizednames(void)
{
    unsigned int i;

    for(i = 0; i < sizeof(LocalizedKeys)/sizeof(LPCWSTR); i++)
    {
        LoadStringW(hInstance, IDS_LOCALIZEDNAME_ROOT+i, LocalizedNames[i], 256);
    }
}

/********************************************************************
 *              CryptFindLocalizedName (CRYPT32.@)
 */
LPCWSTR WINAPI CryptFindLocalizedName(LPCWSTR pwszCryptName)
{
    unsigned int i;

    for(i = 0; i < sizeof(LocalizedKeys)/sizeof(LPCWSTR); i++)
    {
        if(!lstrcmpiW(LocalizedKeys[i], pwszCryptName))
        {
            return LocalizedNames[i];
        }
    }

    FIXME("No name for: %s - stub\n",debugstr_w(pwszCryptName));
    return NULL;
}

static CRITICAL_SECTION oidInfoCS;
static CRITICAL_SECTION_DEBUG oidInfoCSDebug =
{
    0, 0, &oidInfoCS,
    { &oidInfoCSDebug.ProcessLocksList, &oidInfoCSDebug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": oidInfoCS") }
};
static CRITICAL_SECTION oidInfoCS = { &oidInfoCSDebug, -1, 0, 0, 0, 0 };
static struct list oidInfo = { &oidInfo, &oidInfo };

static const WCHAR tripledes[] = { '3','d','e','s',0 };
static const WCHAR cms3deswrap[] = { 'C','M','S','3','D','E','S','w','r','a',
 'p',0 };
static const WCHAR cmsrc2wrap[] = { 'C','M','S','R','C','2','w','r','a','p',0 };
static const WCHAR des[] = { 'd','e','s',0 };
static const WCHAR md2[] = { 'm','d','2',0 };
static const WCHAR md4[] = { 'm','d','4',0 };
static const WCHAR md5[] = { 'm','d','5',0 };
static const WCHAR rc2[] = { 'r','c','2',0 };
static const WCHAR rc4[] = { 'r','c','4',0 };
static const WCHAR sha[] = { 's','h','a',0 };
static const WCHAR sha1[] = { 's','h','a','1',0 };
static const WCHAR RSA[] = { 'R','S','A',0 };
static const WCHAR RSA_KEYX[] = { 'R','S','A','_','K','E','Y','X',0 };
static const WCHAR RSA_SIGN[] = { 'R','S','A','_','S','I','G','N',0 };
static const WCHAR DSA[] = { 'D','S','A',0 };
static const WCHAR DSA_SIGN[] = { 'D','S','A','_','S','I','G','N',0 };
static const WCHAR DH[] = { 'D','H',0 };
static const WCHAR DSS[] = { 'D','S','S',0 };
static const WCHAR mosaicKMandUpdSig[] =
 { 'm','o','s','a','i','c','K','M','a','n','d','U','p','d','S','i','g',0 };
static const WCHAR ESDH[] = { 'E','S','D','H',0 };
static const WCHAR NO_SIGN[] = { 'N','O','S','I','G','N',0 };
static const WCHAR dsaSHA1[] = { 'd','s','a','S','H','A','1',0 };
static const WCHAR md2RSA[] = { 'm','d','2','R','S','A',0 };
static const WCHAR md4RSA[] = { 'm','d','4','R','S','A',0 };
static const WCHAR md5RSA[] = { 'm','d','5','R','S','A',0 };
static const WCHAR shaDSA[] = { 's','h','a','D','S','A',0 };
static const WCHAR sha1DSA[] = { 's','h','a','1','D','S','A',0 };
static const WCHAR shaRSA[] = { 's','h','a','R','S','A',0 };
static const WCHAR sha1RSA[] = { 's','h','a','1','R','S','A',0 };
static const WCHAR mosaicUpdatedSig[] =
 { 'm','o','s','a','i','c','U','p','d','a','t','e','d','S','i','g',0 };
static const WCHAR CN[] = { 'C','N',0 };
static const WCHAR L[] = { 'L',0 };
static const WCHAR O[] = { 'O',0 };
static const WCHAR OU[] = { 'O','U',0 };
static const WCHAR E[] = { 'E',0 };
static const WCHAR C[] = { 'C',0 };
static const WCHAR S[] = { 'S',0 };
static const WCHAR ST[] = { 'S','T',0 };
static const WCHAR STREET[] = { 'S','T','R','E','E','T',0 };
static const WCHAR T[] = { 'T',0 };
static const WCHAR Title[] = { 'T','i','t','l','e',0 };
static const WCHAR G[] = { 'G',0 };
static const WCHAR GivenName[] = { 'G','i','v','e','n','N','a','m','e',0 };
static const WCHAR I[] = { 'I',0 };
static const WCHAR Initials[] = { 'I','n','i','t','i','a','l','s',0 };
static const WCHAR SN[] = { 'S','N',0 };
static const WCHAR DC[] = { 'D','C',0 };
static const WCHAR Description[] =
 { 'D','e','s','c','r','i','p','t','i','o','n',0 };
static const WCHAR PostalCode[] = { 'P','o','s','t','a','l','C','o','d','e',0 };
static const WCHAR POBox[] = { 'P','O','B','o','x',0 };
static const WCHAR Phone[] = { 'P','h','o','n','e',0 };
static const WCHAR X21Address[] = { 'X','2','1','A','d','d','r','e','s','s',0 };
static const WCHAR dnQualifier[] =
 { 'd','n','Q','u','a','l','i','f','i','e','r',0 };
static const WCHAR Email[] = { 'E','m','a','i','l',0 };
static const WCHAR GN[] = { 'G','N',0 };

static const DWORD noNullFlag = CRYPT_OID_NO_NULL_ALGORITHM_PARA_FLAG;
static const DWORD mosaicFlags = CRYPT_OID_INHIBIT_SIGNATURE_FORMAT_FLAG |
 CRYPT_OID_NO_NULL_ALGORITHM_PARA_FLAG;
static const CRYPT_DATA_BLOB noNullBlob = { sizeof(noNullFlag),
 (LPBYTE)&noNullFlag };
static const CRYPT_DATA_BLOB mosaicFlagsBlob = { sizeof(mosaicFlags),
 (LPBYTE)&mosaicFlags };

static const DWORD rsaSign = CALG_RSA_SIGN;
static const DWORD dssSign[2] = { CALG_DSS_SIGN,
 CRYPT_OID_NO_NULL_ALGORITHM_PARA_FLAG };
static const DWORD mosaicSign[2] = { CALG_DSS_SIGN,
 CRYPT_OID_INHIBIT_SIGNATURE_FORMAT_FLAG |
 CRYPT_OID_NO_NULL_ALGORITHM_PARA_FLAG };
static const CRYPT_DATA_BLOB rsaSignBlob = { sizeof(rsaSign),
 (LPBYTE)&rsaSign };
static const CRYPT_DATA_BLOB dssSignBlob = { sizeof(dssSign),
 (LPBYTE)dssSign };
static const CRYPT_DATA_BLOB mosaicSignBlob = { sizeof(mosaicSign),
 (LPBYTE)mosaicSign };

static const DWORD ia5String[] = { CERT_RDN_IA5_STRING, 0 };
static const DWORD numericString[] = { CERT_RDN_NUMERIC_STRING, 0 };
static const DWORD printableString[] = { CERT_RDN_PRINTABLE_STRING, 0 };
static const DWORD domainCompTypes[] = { CERT_RDN_IA5_STRING,
 CERT_RDN_UTF8_STRING, 0 };
static const CRYPT_DATA_BLOB ia5StringBlob = { sizeof(ia5String),
 (LPBYTE)ia5String };
static const CRYPT_DATA_BLOB numericStringBlob = { sizeof(numericString),
 (LPBYTE)numericString };
static const CRYPT_DATA_BLOB printableStringBlob = { sizeof(printableString),
 (LPBYTE)printableString };
static const CRYPT_DATA_BLOB domainCompTypesBlob = { sizeof(domainCompTypes),
 (LPBYTE)domainCompTypes };

static const struct OIDInfoConstructor {
    DWORD   dwGroupId;
    LPCSTR  pszOID;
    UINT    Algid;
    LPCWSTR pwszName;
    const CRYPT_DATA_BLOB *blob;
} oidInfoConstructors[] = {
 { 1, szOID_OIWSEC_sha1,               CALG_SHA1,     sha1, NULL },
 { 1, szOID_OIWSEC_sha1,               CALG_SHA1,     sha, NULL },
 { 1, szOID_OIWSEC_sha,                CALG_SHA,      sha, NULL },
 { 1, szOID_RSA_MD5,                   CALG_MD5,      md5, NULL },
 { 1, szOID_RSA_MD4,                   CALG_MD4,      md4, NULL },
 { 1, szOID_RSA_MD2,                   CALG_MD2,      md2, NULL },

 { 2, szOID_OIWSEC_desCBC,             CALG_DES,      des, NULL },
 { 2, szOID_RSA_DES_EDE3_CBC,          CALG_3DES,     tripledes, NULL },
 { 2, szOID_RSA_RC2CBC,                CALG_RC2,      rc2, NULL },
 { 2, szOID_RSA_RC4,                   CALG_RC4,      rc4, NULL },
 { 2, szOID_RSA_SMIMEalgCMS3DESwrap,   CALG_3DES,     cms3deswrap, NULL },
 { 2, szOID_RSA_SMIMEalgCMSRC2wrap,    CALG_RC2,      cmsrc2wrap, NULL },

 { 3, szOID_RSA_RSA,                   CALG_RSA_KEYX, RSA, NULL },
 { 3, szOID_X957_DSA,                  CALG_DSS_SIGN, DSA, &noNullBlob },
 { 3, szOID_ANSI_X942_DH,              CALG_DH_SF,    DH, &noNullBlob },
 { 3, szOID_RSA_RSA,                   CALG_RSA_KEYX, RSA_KEYX, NULL },
 { 3, szOID_RSA_RSA,                   CALG_RSA_SIGN, RSA, NULL },
 { 3, szOID_RSA_RSA,                   CALG_RSA_SIGN, RSA_SIGN, NULL },
 { 3, szOID_OIWSEC_dsa,                CALG_DSS_SIGN, DSA, &noNullBlob },
 { 3, szOID_OIWSEC_dsa,                CALG_DSS_SIGN, DSS, &noNullBlob },
 { 3, szOID_OIWSEC_dsa,                CALG_DSS_SIGN, DSA_SIGN, &noNullBlob },
 { 3, szOID_RSA_DH,                    CALG_DH_SF,    DH, &noNullBlob },
 { 3, szOID_OIWSEC_rsaXchg,            CALG_RSA_KEYX, RSA_KEYX, NULL },
 { 3, szOID_INFOSEC_mosaicKMandUpdSig, CALG_DSS_SIGN, mosaicKMandUpdSig,
   &mosaicFlagsBlob },
 { 3, szOID_RSA_SMIMEalgESDH,          CALG_DH_EPHEM, ESDH, &noNullBlob },
 { 3, szOID_PKIX_NO_SIGNATURE,         CALG_NO_SIGN,  NO_SIGN, NULL },

 { 4, szOID_RSA_SHA1RSA,               CALG_SHA1,     sha1RSA, &rsaSignBlob },
 { 4, szOID_RSA_MD5RSA,                CALG_MD5,      md5RSA, &rsaSignBlob },
 { 4, szOID_X957_SHA1DSA,              CALG_SHA1,     sha1DSA, &dssSignBlob },
 { 4, szOID_OIWSEC_sha1RSASign,        CALG_SHA1,     sha1RSA, &rsaSignBlob },
 { 4, szOID_OIWSEC_sha1RSASign,        CALG_SHA1,     shaRSA, &rsaSignBlob },
 { 4, szOID_OIWSEC_shaRSA,             CALG_SHA1,     shaRSA, &rsaSignBlob },
 { 4, szOID_OIWSEC_md5RSA,             CALG_MD5,      md5RSA, &rsaSignBlob },
 { 4, szOID_RSA_MD2RSA,                CALG_MD2,      md2RSA, &rsaSignBlob },
 { 4, szOID_RSA_MD4RSA,                CALG_MD4,      md4RSA, &rsaSignBlob },
 { 4, szOID_OIWSEC_md4RSA,             CALG_MD4,      md4RSA, &rsaSignBlob },
 { 4, szOID_OIWSEC_md4RSA2,            CALG_MD4,      md4RSA, &rsaSignBlob },
 { 4, szOID_OIWDIR_md2RSA,             CALG_MD2,      md2RSA, &rsaSignBlob },
 { 4, szOID_OIWSEC_shaDSA,             CALG_SHA1,     sha1DSA, &dssSignBlob },
 { 4, szOID_OIWSEC_shaDSA,             CALG_SHA1,     shaDSA, &dssSignBlob },
 { 4, szOID_OIWSEC_dsaSHA1,            CALG_SHA1,     dsaSHA1, &dssSignBlob },
 { 4, szOID_INFOSEC_mosaicUpdatedSig,  CALG_SHA1,     mosaicUpdatedSig,
   &mosaicSignBlob },

 { 5, szOID_COMMON_NAME,              0, CN, NULL },
 { 5, szOID_LOCALITY_NAME,            0, L, NULL },
 { 5, szOID_ORGANIZATION_NAME,        0, O, NULL },
 { 5, szOID_ORGANIZATIONAL_UNIT_NAME, 0, OU, NULL },
 { 5, szOID_RSA_emailAddr,            0, E, &ia5StringBlob },
 { 5, szOID_RSA_emailAddr,            0, Email, &ia5StringBlob },
 { 5, szOID_COUNTRY_NAME,             0, C, &printableStringBlob },
 { 5, szOID_STATE_OR_PROVINCE_NAME,   0, S, NULL },
 { 5, szOID_STATE_OR_PROVINCE_NAME,   0, ST, NULL },
 { 5, szOID_STREET_ADDRESS,           0, STREET, NULL },
 { 5, szOID_TITLE,                    0, T, NULL },
 { 5, szOID_TITLE,                    0, Title, NULL },
 { 5, szOID_GIVEN_NAME,               0, G, NULL },
 { 5, szOID_GIVEN_NAME,               0, GN, NULL },
 { 5, szOID_GIVEN_NAME,               0, GivenName, NULL },
 { 5, szOID_INITIALS,                 0, I, NULL },
 { 5, szOID_INITIALS,                 0, Initials, NULL },
 { 5, szOID_SUR_NAME,                 0, SN, NULL },
 { 5, szOID_DOMAIN_COMPONENT,         0, DC, &domainCompTypesBlob },
 { 5, szOID_DESCRIPTION,              0, Description, NULL },
 { 5, szOID_POSTAL_CODE,              0, PostalCode, NULL },
 { 5, szOID_POST_OFFICE_BOX,          0, POBox, NULL },
 { 5, szOID_TELEPHONE_NUMBER,         0, Phone, &printableStringBlob },
 { 5, szOID_X21_ADDRESS,              0, X21Address, &numericStringBlob },
 { 5, szOID_DN_QUALIFIER,             0, dnQualifier, NULL },

 { 6, szOID_AUTHORITY_KEY_IDENTIFIER2, 0, (LPCWSTR)IDS_AUTHORITY_KEY_ID, NULL },
 { 6, szOID_AUTHORITY_KEY_IDENTIFIER, 0, (LPCWSTR)IDS_AUTHORITY_KEY_ID, NULL },
 { 6, szOID_KEY_ATTRIBUTES, 0, (LPCWSTR)IDS_KEY_ATTRIBUTES, NULL },
 { 6, szOID_KEY_USAGE_RESTRICTION, 0, (LPCWSTR)IDS_KEY_USAGE_RESTRICTION, NULL },
 { 6, szOID_SUBJECT_ALT_NAME2, 0, (LPCWSTR)IDS_SUBJECT_ALT_NAME, NULL },
 { 6, szOID_SUBJECT_ALT_NAME, 0, (LPCWSTR)IDS_SUBJECT_ALT_NAME, NULL },
 { 6, szOID_ISSUER_ALT_NAME2, 0, (LPCWSTR)IDS_ISSUER_ALT_NAME, NULL },
 { 6, szOID_ISSUER_ALT_NAME2, 0, (LPCWSTR)IDS_ISSUER_ALT_NAME, NULL },
 { 6, szOID_BASIC_CONSTRAINTS2, 0, (LPCWSTR)IDS_BASIC_CONSTRAINTS, NULL },
 { 6, szOID_BASIC_CONSTRAINTS, 0, (LPCWSTR)IDS_BASIC_CONSTRAINTS, NULL },
 { 6, szOID_KEY_USAGE, 0, (LPCWSTR)IDS_KEY_USAGE, NULL },
 { 6, szOID_CERT_POLICIES, 0, (LPCWSTR)IDS_CERT_POLICIES, NULL },
 { 6, szOID_SUBJECT_KEY_IDENTIFIER, 0, (LPCWSTR)IDS_SUBJECT_KEY_IDENTIFIER, NULL },
 { 6, szOID_CRL_REASON_CODE, 0, (LPCWSTR)IDS_CRL_REASON_CODE, NULL },
 { 6, szOID_CRL_DIST_POINTS, 0, (LPCWSTR)IDS_CRL_DIST_POINTS, NULL },
 { 6, szOID_ENHANCED_KEY_USAGE, 0, (LPCWSTR)IDS_ENHANCED_KEY_USAGE, NULL },
 { 6, szOID_AUTHORITY_INFO_ACCESS, 0, (LPCWSTR)IDS_AUTHORITY_INFO_ACCESS, NULL },
 { 6, szOID_CERT_EXTENSIONS, 0, (LPCWSTR)IDS_CERT_EXTENSIONS, NULL },
 { 6, szOID_RSA_certExtensions, 0, (LPCWSTR)IDS_CERT_EXTENSIONS, NULL },
 { 6, szOID_NEXT_UPDATE_LOCATION, 0, (LPCWSTR)IDS_NEXT_UPDATE_LOCATION, NULL },
 { 6, szOID_YESNO_TRUST_ATTR, 0, (LPCWSTR)IDS_YES_OR_NO_TRUST, NULL },
 { 6, szOID_RSA_emailAddr, 0, (LPCWSTR)IDS_EMAIL_ADDRESS, NULL },
 { 6, szOID_RSA_unstructName, 0, (LPCWSTR)IDS_UNSTRUCTURED_NAME, NULL },
 { 6, szOID_RSA_contentType, 0, (LPCWSTR)IDS_CONTENT_TYPE, NULL },
 { 6, szOID_RSA_messageDigest, 0, (LPCWSTR)IDS_MESSAGE_DIGEST, NULL },
 { 6, szOID_RSA_signingTime, 0, (LPCWSTR)IDS_SIGNING_TIME, NULL },
 { 6, szOID_RSA_counterSign, 0, (LPCWSTR)IDS_COUNTER_SIGN, NULL },
 { 6, szOID_RSA_challengePwd, 0, (LPCWSTR)IDS_CHALLENGE_PASSWORD, NULL },
 { 6, szOID_RSA_unstructAddr, 0, (LPCWSTR)IDS_UNSTRUCTURED_ADDRESS, NULL },
 { 6, szOID_RSA_SMIMECapabilities, 0, (LPCWSTR)IDS_SMIME_CAPABILITIES, NULL },
 { 6, szOID_RSA_preferSignedData, 0, (LPCWSTR)IDS_PREFER_SIGNED_DATA, NULL },
 { 6, szOID_PKIX_POLICY_QUALIFIER_CPS, 0, (LPCWSTR)IDS_CPS, NULL },
 { 6, szOID_PKIX_POLICY_QUALIFIER_USERNOTICE, 0, (LPCWSTR)IDS_USER_NOTICE, NULL },
 { 6, szOID_PKIX_OCSP, 0, (LPCWSTR)IDS_OCSP, NULL },
 { 6, szOID_PKIX_CA_ISSUERS, 0, (LPCWSTR)IDS_CA_ISSUER, NULL },
 { 6, szOID_ENROLL_CERTTYPE_EXTENSION, 0, (LPCWSTR)IDS_CERT_TEMPLATE_NAME, NULL },
 { 6, szOID_ENROLL_CERTTYPE_EXTENSION, 0, (LPCWSTR)IDS_CERT_TYPE, NULL },
 { 6, szOID_CERT_MANIFOLD, 0, (LPCWSTR)IDS_CERT_MANIFOLD, NULL },
 { 6, szOID_NETSCAPE_CERT_TYPE, 0, (LPCWSTR)IDS_NETSCAPE_CERT_TYPE, NULL },
 { 6, szOID_NETSCAPE_BASE_URL, 0, (LPCWSTR)IDS_NETSCAPE_BASE_URL, NULL },
 { 6, szOID_NETSCAPE_REVOCATION_URL, 0, (LPCWSTR)IDS_NETSCAPE_REVOCATION_URL, NULL },
 { 6, szOID_NETSCAPE_CA_REVOCATION_URL, 0, (LPCWSTR)IDS_NETSCAPE_CA_REVOCATION_URL, NULL },
 { 6, szOID_NETSCAPE_CERT_RENEWAL_URL, 0, (LPCWSTR)IDS_NETSCAPE_CERT_RENEWAL_URL, NULL },
 { 6, szOID_NETSCAPE_CA_POLICY_URL, 0, (LPCWSTR)IDS_NETSCAPE_CA_POLICY_URL, NULL },
 { 6, szOID_NETSCAPE_SSL_SERVER_NAME, 0, (LPCWSTR)IDS_NETSCAPE_SSL_SERVER_NAME, NULL },
 { 6, szOID_NETSCAPE_COMMENT, 0, (LPCWSTR)IDS_NETSCAPE_COMMENT, NULL },
 { 6, "1.3.6.1.4.1.311.2.1.10", 0, (LPCWSTR)IDS_SPC_SP_AGENCY_INFO, NULL },
 { 6, "1.3.6.1.4.1.311.2.1.27", 0, (LPCWSTR)IDS_SPC_FINANCIAL_CRITERIA, NULL },
 { 6, "1.3.6.1.4.1.311.2.1.26", 0, (LPCWSTR)IDS_SPC_MINIMAL_CRITERIA, NULL },
 { 6, szOID_COUNTRY_NAME, 0, (LPCWSTR)IDS_COUNTRY, NULL },
 { 6, szOID_ORGANIZATION_NAME, 0, (LPCWSTR)IDS_ORGANIZATION, NULL },
 { 6, szOID_ORGANIZATIONAL_UNIT_NAME, 0, (LPCWSTR)IDS_ORGANIZATIONAL_UNIT, NULL },
 { 6, szOID_COMMON_NAME, 0, (LPCWSTR)IDS_COMMON_NAME, NULL },
 { 6, szOID_LOCALITY_NAME, 0, (LPCWSTR)IDS_LOCALITY, NULL },
 { 6, szOID_STATE_OR_PROVINCE_NAME, 0, (LPCWSTR)IDS_STATE_OR_PROVINCE, NULL },
 { 6, szOID_TITLE, 0, (LPCWSTR)IDS_TITLE, NULL },
 { 6, szOID_GIVEN_NAME, 0, (LPCWSTR)IDS_GIVEN_NAME, NULL },
 { 6, szOID_INITIALS, 0, (LPCWSTR)IDS_INITIALS, NULL },
 { 6, szOID_SUR_NAME, 0, (LPCWSTR)IDS_SUR_NAME, NULL },
 { 6, szOID_DOMAIN_COMPONENT, 0, (LPCWSTR)IDS_DOMAIN_COMPONENT, NULL },
 { 6, szOID_STREET_ADDRESS, 0, (LPCWSTR)IDS_STREET_ADDRESS, NULL },
 { 6, szOID_DEVICE_SERIAL_NUMBER, 0, (LPCWSTR)IDS_SERIAL_NUMBER, NULL },
 { 6, szOID_CERTSRV_CA_VERSION, 0, (LPCWSTR)IDS_CA_VERSION, NULL },
 { 6, szOID_CERTSRV_CROSSCA_VERSION, 0, (LPCWSTR)IDS_CROSS_CA_VERSION, NULL },
 { 6, szOID_SERIALIZED, 0, (LPCWSTR)IDS_SERIALIZED_SIG_SERIAL_NUMBER, NULL },
 { 6, szOID_NT_PRINCIPAL_NAME, 0, (LPCWSTR)IDS_PRINCIPAL_NAME, NULL },
 { 6, szOID_PRODUCT_UPDATE, 0, (LPCWSTR)IDS_WINDOWS_PRODUCT_UPDATE, NULL },
 { 6, szOID_ENROLLMENT_NAME_VALUE_PAIR, 0, (LPCWSTR)IDS_ENROLLMENT_NAME_VALUE_PAIR, NULL },
 { 6, szOID_OS_VERSION, 0, (LPCWSTR)IDS_OS_VERSION, NULL },
 { 6, szOID_ENROLLMENT_CSP_PROVIDER, 0, (LPCWSTR)IDS_ENROLLMENT_CSP, NULL },
 { 6, szOID_CRL_NUMBER, 0, (LPCWSTR)IDS_CRL_NUMBER, NULL },
 { 6, szOID_DELTA_CRL_INDICATOR, 0, (LPCWSTR)IDS_DELTA_CRL_INDICATOR, NULL },
 { 6, szOID_ISSUING_DIST_POINT, 0, (LPCWSTR)IDS_ISSUING_DIST_POINT, NULL },
 { 6, szOID_FRESHEST_CRL, 0, (LPCWSTR)IDS_FRESHEST_CRL, NULL },
 { 6, szOID_NAME_CONSTRAINTS, 0, (LPCWSTR)IDS_NAME_CONSTRAINTS, NULL },
 { 6, szOID_POLICY_MAPPINGS, 0, (LPCWSTR)IDS_POLICY_MAPPINGS, NULL },
 { 6, szOID_LEGACY_POLICY_MAPPINGS, 0, (LPCWSTR)IDS_POLICY_MAPPINGS, NULL },
 { 6, szOID_POLICY_CONSTRAINTS, 0, (LPCWSTR)IDS_POLICY_CONSTRAINTS, NULL },
 { 6, szOID_CROSS_CERT_DIST_POINTS, 0, (LPCWSTR)IDS_CROSS_CERT_DIST_POINTS, NULL },
 { 6, szOID_APPLICATION_CERT_POLICIES, 0, (LPCWSTR)IDS_APPLICATION_POLICIES, NULL },
 { 6, szOID_APPLICATION_POLICY_MAPPINGS, 0, (LPCWSTR)IDS_APPLICATION_POLICY_MAPPINGS, NULL },
 { 6, szOID_APPLICATION_POLICY_CONSTRAINTS, 0, (LPCWSTR)IDS_APPLICATION_POLICY_CONSTRAINTS, NULL },
 { 6, szOID_CT_PKI_DATA, 0, (LPCWSTR)IDS_CMC_DATA, NULL },
 { 6, szOID_CT_PKI_RESPONSE, 0, (LPCWSTR)IDS_CMC_RESPONSE, NULL },
 { 6, szOID_CMC, 0, (LPCWSTR)IDS_UNSIGNED_CMC_REQUEST, NULL },
 { 6, szOID_CMC_STATUS_INFO, 0, (LPCWSTR)IDS_CMC_STATUS_INFO, NULL },
 { 6, szOID_CMC_ADD_EXTENSIONS, 0, (LPCWSTR)IDS_CMC_EXTENSIONS, NULL },
 { 6, szOID_CTL, 0, (LPCWSTR)IDS_CMC_ATTRIBUTES, NULL },
 { 6, szOID_RSA_data, 0, (LPCWSTR)IDS_PKCS_7_DATA, NULL },
 { 6, szOID_RSA_signedData, 0, (LPCWSTR)IDS_PKCS_7_SIGNED, NULL },
 { 6, szOID_RSA_envelopedData, 0, (LPCWSTR)IDS_PKCS_7_ENVELOPED, NULL },
 { 6, szOID_RSA_signEnvData, 0, (LPCWSTR)IDS_PKCS_7_SIGNED_ENVELOPED, NULL },
 { 6, szOID_RSA_digestedData, 0, (LPCWSTR)IDS_PKCS_7_DIGESTED, NULL },
 { 6, szOID_RSA_encryptedData, 0, (LPCWSTR)IDS_PKCS_7_ENCRYPTED, NULL },
 { 6, szOID_CERTSRV_PREVIOUS_CERT_HASH, 0, (LPCWSTR)IDS_PREVIOUS_CA_CERT_HASH, NULL },
 { 6, szOID_CRL_VIRTUAL_BASE, 0, (LPCWSTR)IDS_CRL_VIRTUAL_BASE, NULL },
 { 6, szOID_CRL_NEXT_PUBLISH, 0, (LPCWSTR)IDS_CRL_NEXT_PUBLISH, NULL },
 { 6, szOID_KP_CA_EXCHANGE, 0, (LPCWSTR)IDS_CA_EXCHANGE, NULL },
 { 6, szOID_KP_KEY_RECOVERY_AGENT, 0, (LPCWSTR)IDS_KEY_RECOVERY_AGENT, NULL },
 { 6, szOID_CERTIFICATE_TEMPLATE, 0, (LPCWSTR)IDS_CERTIFICATE_TEMPLATE, NULL },
 { 6, szOID_ENTERPRISE_OID_ROOT, 0, (LPCWSTR)IDS_ENTERPRISE_ROOT_OID, NULL },
 { 6, szOID_RDN_DUMMY_SIGNER, 0, (LPCWSTR)IDS_RDN_DUMMY_SIGNER, NULL },
 { 6, szOID_ARCHIVED_KEY_ATTR, 0, (LPCWSTR)IDS_ARCHIVED_KEY_ATTR, NULL },
 { 6, szOID_CRL_SELF_CDP, 0, (LPCWSTR)IDS_CRL_SELF_CDP, NULL },
 { 6, szOID_REQUIRE_CERT_CHAIN_POLICY, 0, (LPCWSTR)IDS_REQUIRE_CERT_CHAIN_POLICY, NULL },
 { 6, szOID_CMC_TRANSACTION_ID, 0, (LPCWSTR)IDS_TRANSACTION_ID, NULL },
 { 6, szOID_CMC_SENDER_NONCE, 0, (LPCWSTR)IDS_SENDER_NONCE, NULL },
 { 6, szOID_CMC_RECIPIENT_NONCE, 0, (LPCWSTR)IDS_RECIPIENT_NONCE, NULL },
 { 6, szOID_CMC_REG_INFO, 0, (LPCWSTR)IDS_REG_INFO, NULL },
 { 6, szOID_CMC_GET_CERT, 0, (LPCWSTR)IDS_GET_CERTIFICATE, NULL },
 { 6, szOID_CMC_GET_CRL, 0, (LPCWSTR)IDS_GET_CRL, NULL },
 { 6, szOID_CMC_REVOKE_REQUEST, 0, (LPCWSTR)IDS_REVOKE_REQUEST, NULL },
 { 6, szOID_CMC_QUERY_PENDING, 0, (LPCWSTR)IDS_QUERY_PENDING, NULL },
 { 6, szOID_SORTED_CTL, 0, (LPCWSTR)IDS_SORTED_CTL, NULL },
 { 6, szOID_ARCHIVED_KEY_CERT_HASH, 0, (LPCWSTR)IDS_ARCHIVED_KEY_CERT_HASH, NULL },
 { 6, szOID_PRIVATEKEY_USAGE_PERIOD, 0, (LPCWSTR)IDS_PRIVATE_KEY_USAGE_PERIOD, NULL },
 { 6, szOID_REQUEST_CLIENT_INFO, 0, (LPCWSTR)IDS_CLIENT_INFORMATION, NULL },

 { 7, szOID_PKIX_KP_SERVER_AUTH, 0, (LPCWSTR)IDS_SERVER_AUTHENTICATION, NULL },
 { 7, szOID_PKIX_KP_CLIENT_AUTH, 0, (LPCWSTR)IDS_CLIENT_AUTHENTICATION, NULL },
 { 7, szOID_PKIX_KP_CODE_SIGNING, 0, (LPCWSTR)IDS_CODE_SIGNING, NULL },
 { 7, szOID_PKIX_KP_EMAIL_PROTECTION, 0, (LPCWSTR)IDS_SECURE_EMAIL, NULL },
 { 7, szOID_PKIX_KP_TIMESTAMP_SIGNING, 0, (LPCWSTR)IDS_TIME_STAMPING, NULL },
 { 7, szOID_KP_CTL_USAGE_SIGNING, 0, (LPCWSTR)IDS_MICROSOFT_TRUST_LIST_SIGNING, NULL },
 { 7, szOID_KP_TIME_STAMP_SIGNING, 0, (LPCWSTR)IDS_MICROSOFT_TIME_STAMPING, NULL },
 { 7, szOID_PKIX_KP_IPSEC_END_SYSTEM, 0, (LPCWSTR)IDS_IPSEC_END_SYSTEM, NULL },
 { 7, szOID_PKIX_KP_IPSEC_TUNNEL, 0, (LPCWSTR)IDS_IPSEC_TUNNEL, NULL },
 { 7, szOID_PKIX_KP_IPSEC_USER, 0, (LPCWSTR)IDS_IPSEC_USER, NULL },
 { 7, szOID_KP_EFS, 0, (LPCWSTR)IDS_EFS, NULL },
 { 7, szOID_WHQL_CRYPTO, 0, (LPCWSTR)IDS_WHQL_CRYPTO, NULL },
 { 7, szOID_NT5_CRYPTO, 0, (LPCWSTR)IDS_NT5_CRYPTO, NULL },
 { 7, szOID_OEM_WHQL_CRYPTO, 0, (LPCWSTR)IDS_OEM_WHQL_CRYPTO, NULL },
 { 7, szOID_EMBEDDED_NT_CRYPTO, 0, (LPCWSTR)IDS_EMBEDDED_NT_CRYPTO, NULL },
 { 7, szOID_LICENSES, 0, (LPCWSTR)IDS_KEY_PACK_LICENSES, NULL },
 { 7, szOID_LICENSE_SERVER, 0, (LPCWSTR)IDS_LICENSE_SERVER, NULL },
 { 7, szOID_KP_SMARTCARD_LOGON, 0, (LPCWSTR)IDS_SMART_CARD_LOGON, NULL },
 { 7, szOID_DRM, 0, (LPCWSTR)IDS_DIGITAL_RIGHTS, NULL },
 { 7, szOID_KP_QUALIFIED_SUBORDINATION, 0, (LPCWSTR)IDS_QUALIFIED_SUBORDINATION, NULL },
 { 7, szOID_KP_KEY_RECOVERY, 0, (LPCWSTR)IDS_KEY_RECOVERY, NULL },
 { 7, szOID_KP_DOCUMENT_SIGNING, 0, (LPCWSTR)IDS_DOCUMENT_SIGNING, NULL },
 { 7, szOID_IPSEC_KP_IKE_INTERMEDIATE, 0, (LPCWSTR)IDS_IPSEC_IKE_INTERMEDIATE, NULL },
 { 7, szOID_EFS_RECOVERY, 0, (LPCWSTR)IDS_FILE_RECOVERY, NULL },
 { 7, szOID_ROOT_LIST_SIGNER, 0, (LPCWSTR)IDS_ROOT_LIST_SIGNER, NULL },
 { 7, szOID_ANY_APPLICATION_POLICY, 0, (LPCWSTR)IDS_ANY_APPLICATION_POLICIES, NULL },
 { 7, szOID_DS_EMAIL_REPLICATION, 0, (LPCWSTR)IDS_DS_EMAIL_REPLICATION, NULL },
 { 7, szOID_ENROLLMENT_AGENT, 0, (LPCWSTR)IDS_ENROLLMENT_AGENT, NULL },
 { 7, szOID_KP_KEY_RECOVERY_AGENT, 0, (LPCWSTR)IDS_KEY_RECOVERY_AGENT, NULL },
 { 7, szOID_KP_CA_EXCHANGE, 0, (LPCWSTR)IDS_CA_EXCHANGE, NULL },
 { 7, szOID_KP_LIFETIME_SIGNING, 0, (LPCWSTR)IDS_LIFETIME_SIGNING, NULL },

 { 8, szOID_ANY_CERT_POLICY, 0, (LPCWSTR)IDS_ANY_CERT_POLICY, NULL },
};

struct OIDInfo {
    CRYPT_OID_INFO info;
    struct list entry;
};

static void init_oid_info(void)
{
    DWORD i;

    oid_init_localizednames();
    for (i = 0; i < sizeof(oidInfoConstructors) /
     sizeof(oidInfoConstructors[0]); i++)
    {
        if (HIWORD(oidInfoConstructors[i].pwszName))
        {
            struct OIDInfo *info;

            /* The name is a static string, so just use the same pointer */
            info = CryptMemAlloc(sizeof(struct OIDInfo));
            if (info)
            {
                memset(info, 0, sizeof(*info));
                info->info.cbSize = sizeof(CRYPT_OID_INFO);
                info->info.pszOID = oidInfoConstructors[i].pszOID;
                info->info.pwszName = oidInfoConstructors[i].pwszName;
                info->info.dwGroupId = oidInfoConstructors[i].dwGroupId;
                info->info.u.Algid = oidInfoConstructors[i].Algid;
                if (oidInfoConstructors[i].blob)
                {
                    info->info.ExtraInfo.cbData =
                     oidInfoConstructors[i].blob->cbData;
                    info->info.ExtraInfo.pbData =
                     oidInfoConstructors[i].blob->pbData;
                }
                list_add_tail(&oidInfo, &info->entry);
            }
        }
        else
        {
            LPCWSTR stringresource;
            int len = LoadStringW(hInstance,
             (UINT_PTR)oidInfoConstructors[i].pwszName,
             (LPWSTR)&stringresource, 0);

            if (len)
            {
                struct OIDInfo *info = CryptMemAlloc(sizeof(struct OIDInfo) +
                 (len + 1) * sizeof(WCHAR));

                if (info)
                {
                    memset(info, 0, sizeof(*info));
                    info->info.cbSize = sizeof(CRYPT_OID_INFO);
                    info->info.pszOID = oidInfoConstructors[i].pszOID;
                    info->info.pwszName = (LPWSTR)(info + 1);
                    info->info.dwGroupId = oidInfoConstructors[i].dwGroupId;
                    info->info.u.Algid = oidInfoConstructors[i].Algid;
                    memcpy(info + 1, stringresource, len*sizeof(WCHAR));
                    ((LPWSTR)(info + 1))[len] = 0;
                    if (oidInfoConstructors[i].blob)
                    {
                        info->info.ExtraInfo.cbData =
                         oidInfoConstructors[i].blob->cbData;
                        info->info.ExtraInfo.pbData =
                         oidInfoConstructors[i].blob->pbData;
                    }
                    list_add_tail(&oidInfo, &info->entry);
                }
            }
        }
    }
}

static void free_oid_info(void)
{
    struct OIDInfo *info, *next;

    LIST_FOR_EACH_ENTRY_SAFE(info, next, &oidInfo, struct OIDInfo, entry)
    {
        list_remove(&info->entry);
        CryptMemFree(info);
    }
}

/***********************************************************************
 *             CryptEnumOIDInfo (CRYPT32.@)
 */
BOOL WINAPI CryptEnumOIDInfo(DWORD dwGroupId, DWORD dwFlags, void *pvArg,
 PFN_CRYPT_ENUM_OID_INFO pfnEnumOIDInfo)
{
    BOOL ret = TRUE;
    struct OIDInfo *info;

    TRACE("(%d, %08x, %p, %p)\n", dwGroupId, dwFlags, pvArg,
     pfnEnumOIDInfo);

    EnterCriticalSection(&oidInfoCS);
    LIST_FOR_EACH_ENTRY(info, &oidInfo, struct OIDInfo, entry)
    {
        if (!dwGroupId || dwGroupId == info->info.dwGroupId)
        {
            ret = pfnEnumOIDInfo(&info->info, pvArg);
            if (!ret)
                break;
        }
    }
    LeaveCriticalSection(&oidInfoCS);
    return ret;
}

PCCRYPT_OID_INFO WINAPI CryptFindOIDInfo(DWORD dwKeyType, void *pvKey,
 DWORD dwGroupId)
{
    PCCRYPT_OID_INFO ret = NULL;

    TRACE("(%d, %p, %d)\n", dwKeyType, pvKey, dwGroupId);

    switch(dwKeyType)
    {
    case CRYPT_OID_INFO_ALGID_KEY:
    {
        struct OIDInfo *info;

        TRACE("CRYPT_OID_INFO_ALGID_KEY: %d\n", *(DWORD *)pvKey);
        EnterCriticalSection(&oidInfoCS);
        LIST_FOR_EACH_ENTRY(info, &oidInfo, struct OIDInfo, entry)
        {
            if (info->info.u.Algid == *(DWORD *)pvKey &&
             (!dwGroupId || info->info.dwGroupId == dwGroupId))
            {
                ret = &info->info;
                break;
            }
        }
        LeaveCriticalSection(&oidInfoCS);
        break;
    }
    case CRYPT_OID_INFO_NAME_KEY:
    {
        struct OIDInfo *info;

        TRACE("CRYPT_OID_INFO_NAME_KEY: %s\n", debugstr_w(pvKey));
        EnterCriticalSection(&oidInfoCS);
        LIST_FOR_EACH_ENTRY(info, &oidInfo, struct OIDInfo, entry)
        {
            if (!lstrcmpW(info->info.pwszName, pvKey) &&
             (!dwGroupId || info->info.dwGroupId == dwGroupId))
            {
                ret = &info->info;
                break;
            }
        }
        LeaveCriticalSection(&oidInfoCS);
        break;
    }
    case CRYPT_OID_INFO_OID_KEY:
    {
        struct OIDInfo *info;
        LPSTR oid = pvKey;

        TRACE("CRYPT_OID_INFO_OID_KEY: %s\n", debugstr_a(oid));
        EnterCriticalSection(&oidInfoCS);
        LIST_FOR_EACH_ENTRY(info, &oidInfo, struct OIDInfo, entry)
        {
            if (!lstrcmpA(info->info.pszOID, oid) &&
             (!dwGroupId || info->info.dwGroupId == dwGroupId))
            {
                ret = &info->info;
                break;
            }
        }
        LeaveCriticalSection(&oidInfoCS);
        break;
    }
    case CRYPT_OID_INFO_SIGN_KEY:
    {
        struct OIDInfo *info;

        TRACE("CRYPT_OID_INFO_SIGN_KEY: %d\n", *(DWORD *)pvKey);
        EnterCriticalSection(&oidInfoCS);
        LIST_FOR_EACH_ENTRY(info, &oidInfo, struct OIDInfo, entry)
        {
            if (info->info.u.Algid == *(DWORD *)pvKey &&
             info->info.ExtraInfo.cbData >= sizeof(DWORD) &&
             *(DWORD *)info->info.ExtraInfo.pbData ==
             *(DWORD *)((LPBYTE)pvKey + sizeof(DWORD)) &&
             (!dwGroupId || info->info.dwGroupId == dwGroupId))
            {
                ret = &info->info;
                break;
            }
        }
        LeaveCriticalSection(&oidInfoCS);
        break;
    }
    }
    return ret;
}

LPCSTR WINAPI CertAlgIdToOID(DWORD dwAlgId)
{
    LPCSTR ret;
    PCCRYPT_OID_INFO info = CryptFindOIDInfo(CRYPT_OID_INFO_ALGID_KEY,
     &dwAlgId, 0);

    if (info)
        ret = info->pszOID;
    else
        ret = NULL;
    return ret;
}

DWORD WINAPI CertOIDToAlgId(LPCSTR pszObjId)
{
    DWORD ret;
    PCCRYPT_OID_INFO info = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
     (void *)pszObjId, 0);

    if (info)
        ret = info->u.Algid;
    else
        ret = 0;
    return ret;
}
