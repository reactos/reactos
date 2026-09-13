/*
 * Copyright 2002 Mike McCormack for CodeWeavers
 * Copyright 2005-2008 Juan Lang
 * Copyright 2006 Paul Vriens
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winreg.h"
#include "winnls.h"
#include "mssip.h"
#include "winuser.h"
#include "crypt32_private.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

/* convert a guid to a wide character string */
static void CRYPT_guid2wstr( const GUID *guid, LPWSTR wstr )
{
    char str[40];

    sprintf(str, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
           guid->Data1, guid->Data2, guid->Data3,
           guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
           guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
    MultiByteToWideChar( CP_ACP, 0, str, -1, wstr, 40 );
}

/***********************************************************************
 *              CRYPT_SIPDeleteFunction
 *
 * Helper function for CryptSIPRemoveProvider
 */
static LONG CRYPT_SIPDeleteFunction( const GUID *guid, LPCWSTR szKey )
{
    WCHAR szFullKey[ 0x100 ];
    LONG r = ERROR_SUCCESS;

    /* max length of szFullKey depends on our code only, so we won't overrun */
    lstrcpyW( szFullKey, L"Software\\Microsoft\\Cryptography\\OID\\EncodingType 0\\CryptSIPDll" );
    lstrcatW( szFullKey, szKey );
    CRYPT_guid2wstr( guid, &szFullKey[ lstrlenW( szFullKey ) ] );

    r = RegDeleteKeyW(HKEY_LOCAL_MACHINE, szFullKey);

    return r;
}

/***********************************************************************
 *             CryptSIPRemoveProvider (CRYPT32.@)
 *
 * Remove a SIP provider and its functions from the registry.
 *
 * PARAMS
 *  pgProv     [I] Pointer to a GUID for this SIP provider
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE. (Look at GetLastError()).
 *
 * NOTES
 *  Registry errors are always reported via SetLastError(). Every registry
 *  deletion will be tried.
 */
BOOL WINAPI CryptSIPRemoveProvider(GUID *pgProv)
{
    LONG r = ERROR_SUCCESS;
    LONG remove_error = ERROR_SUCCESS;

    TRACE("%s\n", debugstr_guid(pgProv));

    if (!pgProv)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


#define CRYPT_SIPREMOVEPROV( key ) \
    r = CRYPT_SIPDeleteFunction( pgProv, key); \
    if (r != ERROR_SUCCESS) remove_error = r

    CRYPT_SIPREMOVEPROV( L"PutSignedDataMsg\\" );
    CRYPT_SIPREMOVEPROV( L"GetSignedDataMsg\\" );
    CRYPT_SIPREMOVEPROV( L"RemoveSignedDataMsg\\" );
    CRYPT_SIPREMOVEPROV( L"CreateIndirectData\\" );
    CRYPT_SIPREMOVEPROV( L"VerifyIndirectData\\" );
    CRYPT_SIPREMOVEPROV( L"IsMyFileType\\" );
    CRYPT_SIPREMOVEPROV( L"IsMyFileType2\\" );

#undef CRYPT_SIPREMOVEPROV

    if (remove_error != ERROR_SUCCESS)
    {
        SetLastError(remove_error);
        return FALSE;
    }

    return TRUE;
}

/*
 * Helper for CryptSIPAddProvider
 *
 * Add a registry key containing a dll name and function under
 *  "Software\\Microsoft\\Cryptography\\OID\\EncodingType 0\\<func>\\<guid>"
 */
static LONG CRYPT_SIPWriteFunction( const GUID *guid, LPCWSTR szKey,
              LPCWSTR szDll, LPCWSTR szFunction )
{
    WCHAR szFullKey[ 0x100 ];
    LONG r = ERROR_SUCCESS;
    HKEY hKey;

    if( !szFunction )
         return ERROR_SUCCESS;

    /* max length of szFullKey depends on our code only, so we won't overrun */
    lstrcpyW( szFullKey, L"Software\\Microsoft\\Cryptography\\OID\\EncodingType 0\\CryptSIPDll" );
    lstrcatW( szFullKey, szKey );
    CRYPT_guid2wstr( guid, &szFullKey[ lstrlenW( szFullKey ) ] );

    TRACE("key is %s\n", debugstr_w( szFullKey ) );

    r = RegCreateKeyW( HKEY_LOCAL_MACHINE, szFullKey, &hKey );
    if( r != ERROR_SUCCESS ) goto error_close_key;

    /* write the values */
    r = RegSetValueExW( hKey, L"FuncName", 0, REG_SZ, (const BYTE*) szFunction,
                        ( lstrlenW( szFunction ) + 1 ) * sizeof (WCHAR) );
    if( r != ERROR_SUCCESS ) goto error_close_key;
    r = RegSetValueExW( hKey, L"Dll", 0, REG_SZ, (const BYTE*) szDll,
                        ( lstrlenW( szDll ) + 1) * sizeof (WCHAR) );

error_close_key:

    RegCloseKey( hKey );

    return r;
}

/***********************************************************************
 *             CryptSIPAddProvider (CRYPT32.@)
 *
 * Add a SIP provider and its functions to the registry.
 *
 * PARAMS
 *  psNewProv       [I] Pointer to a structure with information about
 *                      the functions this SIP provider can perform.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE. (Look at GetLastError()).
 *
 * NOTES
 *  Registry errors are always reported via SetLastError(). If a
 *  registry error occurs the rest of the registry write operations
 *  will be skipped.
 */
BOOL WINAPI CryptSIPAddProvider(SIP_ADD_NEWPROVIDER *psNewProv)
{
    LONG r = ERROR_SUCCESS;

    TRACE("%p\n", psNewProv);

    if (!psNewProv ||
        psNewProv->cbStruct < FIELD_OFFSET(SIP_ADD_NEWPROVIDER, pwszGetCapFuncName) ||
        !psNewProv->pwszGetFuncName ||
        !psNewProv->pwszPutFuncName ||
        !psNewProv->pwszCreateFuncName ||
        !psNewProv->pwszVerifyFuncName ||
        !psNewProv->pwszRemoveFuncName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    TRACE("%s %s %s %s %s\n",
          debugstr_guid( psNewProv->pgSubject ),
          debugstr_w( psNewProv->pwszDLLFileName ),
          debugstr_w( psNewProv->pwszMagicNumber ),
          debugstr_w( psNewProv->pwszIsFunctionName ),
          debugstr_w( psNewProv->pwszIsFunctionNameFmt2 ) );

#define CRYPT_SIPADDPROV( key, field ) \
    r = CRYPT_SIPWriteFunction( psNewProv->pgSubject, key, \
           psNewProv->pwszDLLFileName, psNewProv->field); \
    if (r != ERROR_SUCCESS) goto end_function

    CRYPT_SIPADDPROV( L"PutSignedDataMsg\\", pwszPutFuncName );
    CRYPT_SIPADDPROV( L"GetSignedDataMsg\\", pwszGetFuncName );
    CRYPT_SIPADDPROV( L"RemoveSignedDataMsg\\", pwszRemoveFuncName );
    CRYPT_SIPADDPROV( L"CreateIndirectData\\", pwszCreateFuncName );
    CRYPT_SIPADDPROV( L"VerifyIndirectData\\", pwszVerifyFuncName );
    CRYPT_SIPADDPROV( L"IsMyFileType\\", pwszIsFunctionName );
    CRYPT_SIPADDPROV( L"IsMyFileType2\\", pwszIsFunctionNameFmt2 );

#undef CRYPT_SIPADDPROV

end_function:

    if (r != ERROR_SUCCESS)
    {
        SetLastError(r);
        return FALSE;
    }

    return TRUE;
}

static void *CRYPT_LoadSIPFuncFromKey(HKEY key, HMODULE *pLib)
{
    LONG r;
    DWORD size;
    WCHAR dllName[MAX_PATH];
    char functionName[MAX_PATH];
    HMODULE lib;
    void *func = NULL;

    /* Read the DLL entry */
    size = sizeof(dllName);
    r = RegQueryValueExW(key, L"Dll", NULL, NULL, (LPBYTE)dllName, &size);
    if (r) goto end;

    /* Read the Function entry */
    size = sizeof(functionName);
    r = RegQueryValueExA(key, "FuncName", NULL, NULL, (LPBYTE)functionName,
     &size);
    if (r) goto end;

    lib = LoadLibraryW(dllName);
    if (!lib)
        goto end;
    func = GetProcAddress(lib, functionName);
    if (func)
        *pLib = lib;
    else
        FreeLibrary(lib);

end:
    return func;
}

/***********************************************************************
 *             CryptSIPRetrieveSubjectGuid (CRYPT32.@)
 *
 * Determine the right SIP GUID for the given file.
 *
 * PARAMS
 *  FileName   [I] Filename.
 *  hFileIn    [I] Optional handle to the file.
 *  pgSubject  [O] The SIP's GUID.
 *
 * RETURNS
 *  Success: TRUE. pgSubject contains the SIP GUID.
 *  Failure: FALSE. (Look at GetLastError()).
 *
 * NOTES
 *  On failure pgSubject will contain a NULL GUID.
 *  The handle is always preferred above the filename.
 */
BOOL WINAPI CryptSIPRetrieveSubjectGuid
      (LPCWSTR FileName, HANDLE hFileIn, GUID *pgSubject)
{
    HANDLE hFile;
    BOOL   bRet = FALSE;
    DWORD  count;
    LARGE_INTEGER zero, oldPos;
    /* FIXME, find out if there is a name for this GUID */
    static const GUID unknown = { 0xC689AAB8, 0x8E78, 0x11D0, { 0x8C,0x47,0x00,0xC0,0x4F,0xC2,0x95,0xEE }};
    static const GUID cabGUID = { 0xc689aaba, 0x8e78, 0x11d0, {0x8c,0x47,0x00,0xc0,0x4f,0xc2,0x95,0xee }};
    static const GUID catGUID = { 0xDE351A43, 0x8E59, 0x11D0, { 0x8C,0x47,0x00,0xC0,0x4F,0xC2,0x95,0xEE }};
    static const WORD dosHdr = IMAGE_DOS_SIGNATURE;
    static const BYTE cabHdr[] = { 'M','S','C','F' };
    BYTE hdr[SIP_MAX_MAGIC_NUMBER];
    LONG r = ERROR_SUCCESS;
    HKEY key;

    TRACE("(%s %p %p)\n", wine_dbgstr_w(FileName), hFileIn, pgSubject);

    if (!pgSubject || (!FileName && !hFileIn))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Set pgSubject to zero's */
    memset(pgSubject, 0 , sizeof(GUID));

    if (hFileIn)
        /* Use the given handle, make sure not to close this one ourselves */
        hFile = hFileIn;
    else
    {
        hFile = CreateFileW(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        /* Last error is set by CreateFile */
        if (hFile == INVALID_HANDLE_VALUE) return FALSE;
    }

    zero.QuadPart = 0;
    SetFilePointerEx(hFile, zero, &oldPos, FILE_CURRENT);
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    if (!ReadFile(hFile, hdr, sizeof(hdr), &count, NULL))
        goto cleanup;

    if (count < SIP_MAX_MAGIC_NUMBER)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    TRACE("file magic = 0x%02x%02x%02x%02x\n", hdr[0], hdr[1], hdr[2], hdr[3]);
    /* As everything is in place now we start looking at the file header */
    if (!memcmp(hdr, &dosHdr, sizeof(dosHdr)))
    {
        *pgSubject = unknown;
        SetLastError(S_OK);
        bRet = TRUE;
        goto cleanup;
    }
    /* Quick-n-dirty check for a cab file. */
    if (!memcmp(hdr, cabHdr, sizeof(cabHdr)))
    {
        *pgSubject = cabGUID;
        SetLastError(S_OK);
        bRet = TRUE;
        goto cleanup;
    }
    /* If it's asn.1-encoded, it's probably a .cat file. */
    if (hdr[0] == 0x30)
    {
        DWORD fileLen = GetFileSize(hFile, NULL);

        TRACE("fileLen = %ld\n", fileLen);
        /* Sanity-check length */
        if (hdr[1] < 0x80 && fileLen == 2 + hdr[1])
        {
            *pgSubject = catGUID;
            SetLastError(S_OK);
            bRet = TRUE;
            goto cleanup;
        }
        else if (hdr[1] == 0x80)
        {
            /* Indefinite length, can't verify with just the header, assume it
             * is.
             */
            *pgSubject = catGUID;
            SetLastError(S_OK);
            bRet = TRUE;
            goto cleanup;
        }
        else
        {
            BYTE lenBytes = hdr[1] & 0x7f;

            if (lenBytes == 1 && fileLen == 2 + lenBytes + hdr[2])
            {
                *pgSubject = catGUID;
                SetLastError(S_OK);
                bRet = TRUE;
                goto cleanup;
            }
            else if (lenBytes == 2 && fileLen == 2 + lenBytes +
             (hdr[2] << 8 | hdr[3]))
            {
                *pgSubject = catGUID;
                SetLastError(S_OK);
                bRet = TRUE;
                goto cleanup;
            }
            else if (fileLen > 0xffff)
            {
                /* The file size must be greater than 2 bytes in length, so
                 * assume it is a .cat file
                 */
                *pgSubject = catGUID;
                SetLastError(S_OK);
                bRet = TRUE;
                goto cleanup;
            }
        }
    }

    /* Check for supported functions using CryptSIPDllIsMyFileType */
    r = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Cryptography\\OID\\EncodingType 0\\CryptSIPDllIsMyFileType\\", 0, KEY_READ, &key);
    if (r == ERROR_SUCCESS)
    {
        DWORD index = 0, size;
        WCHAR subKeyName[MAX_PATH];

        do {
            size = ARRAY_SIZE(subKeyName);
            r = RegEnumKeyExW(key, index++, subKeyName, &size, NULL, NULL,
             NULL, NULL);
            if (r == ERROR_SUCCESS)
            {
                HKEY subKey;

                r = RegOpenKeyExW(key, subKeyName, 0, KEY_READ, &subKey);
                if (r == ERROR_SUCCESS)
                {
                    HMODULE lib;
                    pfnIsFileSupported isMy = CRYPT_LoadSIPFuncFromKey(subKey,
                     &lib);

                    if (isMy)
                    {
                        bRet = isMy(hFile, pgSubject);
                        FreeLibrary(lib);
                    }
                    RegCloseKey(subKey);
                }
            }
        } while (!bRet && r == ERROR_SUCCESS);
        RegCloseKey(key);
    }

    /* Check for supported functions using CryptSIPDllIsMyFileType2 */
    if (!bRet)
    {
        r = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Cryptography\\OID\\EncodingType 0\\CryptSIPDllIsMyFileType2\\", 0, KEY_READ, &key);
        if (r == ERROR_SUCCESS)
        {
            DWORD index = 0, size;
            WCHAR subKeyName[MAX_PATH];

            do {
                size = ARRAY_SIZE(subKeyName);
                r = RegEnumKeyExW(key, index++, subKeyName, &size, NULL, NULL,
                 NULL, NULL);
                if (r == ERROR_SUCCESS)
                {
                    HKEY subKey;

                    r = RegOpenKeyExW(key, subKeyName, 0, KEY_READ, &subKey);
                    if (r == ERROR_SUCCESS)
                    {
                        HMODULE lib;
                        pfnIsFileSupportedName isMy2 =
                         CRYPT_LoadSIPFuncFromKey(subKey, &lib);

                        if (isMy2)
                        {
                            bRet = isMy2((LPWSTR)FileName, pgSubject);
                            FreeLibrary(lib);
                        }
                        RegCloseKey(subKey);
                    }
                }
            } while (!bRet && r == ERROR_SUCCESS);
            RegCloseKey(key);
        }
    }

    if (!bRet)
        SetLastError(TRUST_E_SUBJECT_FORM_UNKNOWN);

cleanup:
    /* If we didn't open this one we shouldn't close it (hFile is a copy),
     * but we should reset the file pointer to its original position.
     */
    if (!hFileIn)
        CloseHandle(hFile);
    else
        SetFilePointerEx(hFile, oldPos, NULL, FILE_BEGIN);

    return bRet;
}

static LONG CRYPT_OpenSIPFunctionKey(const GUID *guid, LPCWSTR function,
 HKEY *key)
{
    WCHAR szFullKey[ 0x100 ];

    lstrcpyW(szFullKey, L"Software\\Microsoft\\Cryptography\\OID\\EncodingType 0\\CryptSIPDll");
    lstrcatW(szFullKey, function);
    CRYPT_guid2wstr(guid, &szFullKey[lstrlenW(szFullKey)]);
    return RegOpenKeyExW(HKEY_LOCAL_MACHINE, szFullKey, 0, KEY_READ, key);
}

/* Loads the function named function for the SIP specified by pgSubject, and
 * returns it if found.  Returns NULL on error.  If the function is loaded,
 * *pLib is set to the library in which it is found.
 */
static void *CRYPT_LoadSIPFunc(const GUID *pgSubject, LPCWSTR function,
 HMODULE *pLib)
{
    LONG r;
    HKEY key;
    void *func = NULL;

    TRACE("(%s, %s)\n", debugstr_guid(pgSubject), debugstr_w(function));

    r = CRYPT_OpenSIPFunctionKey(pgSubject, function, &key);
    if (!r)
    {
        func = CRYPT_LoadSIPFuncFromKey(key, pLib);
        RegCloseKey(key);
    }
    TRACE("returning %p\n", func);
    return func;
}

typedef struct _WINE_SIP_PROVIDER {
    GUID              subject;
    SIP_DISPATCH_INFO info;
    struct list       entry;
} WINE_SIP_PROVIDER;

static struct list providers = { &providers, &providers };
static CRITICAL_SECTION providers_cs;
static CRITICAL_SECTION_DEBUG providers_cs_debug =
{
    0, 0, &providers_cs,
    { &providers_cs_debug.ProcessLocksList,
    &providers_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": providers_cs") }
};
static CRITICAL_SECTION providers_cs = { &providers_cs_debug, -1, 0, 0, 0, 0 };

static void CRYPT_CacheSIP(const GUID *pgSubject, SIP_DISPATCH_INFO *info)
{
    WINE_SIP_PROVIDER *prov = CryptMemAlloc(sizeof(WINE_SIP_PROVIDER));

    if (prov)
    {
        prov->subject = *pgSubject;
        prov->info = *info;
        EnterCriticalSection(&providers_cs);
        list_add_tail(&providers, &prov->entry);
        LeaveCriticalSection(&providers_cs);
    }
}

static WINE_SIP_PROVIDER *CRYPT_GetCachedSIP(const GUID *pgSubject)
{
    WINE_SIP_PROVIDER *provider = NULL, *ret = NULL;

    EnterCriticalSection(&providers_cs);
    LIST_FOR_EACH_ENTRY(provider, &providers, WINE_SIP_PROVIDER, entry)
    {
        if (IsEqualGUID(pgSubject, &provider->subject))
            break;
    }
    if (provider && IsEqualGUID(pgSubject, &provider->subject))
        ret = provider;
    LeaveCriticalSection(&providers_cs);
    return ret;
}

static inline BOOL CRYPT_IsSIPCached(const GUID *pgSubject)
{
    return CRYPT_GetCachedSIP(pgSubject) != NULL;
}

void crypt_sip_free(void)
{
    WINE_SIP_PROVIDER *prov, *next;

    LIST_FOR_EACH_ENTRY_SAFE(prov, next, &providers, WINE_SIP_PROVIDER, entry)
    {
        list_remove(&prov->entry);
        FreeLibrary(prov->info.hSIP);
        CryptMemFree(prov);
    }
    DeleteCriticalSection(&providers_cs);
}

/* Loads the SIP for pgSubject into the global cache.  Returns FALSE if the
 * SIP isn't registered or is invalid.
 */
static BOOL CRYPT_LoadSIP(const GUID *pgSubject)
{
    SIP_DISPATCH_INFO sip = { 0 };
    HMODULE lib = NULL, temp = NULL;

    sip.pfGet = CRYPT_LoadSIPFunc(pgSubject, L"GetSignedDataMsg\\", &lib);
    if (!sip.pfGet)
        goto error;
    sip.pfPut = CRYPT_LoadSIPFunc(pgSubject, L"PutSignedDataMsg\\", &temp);
    if (!sip.pfPut || temp != lib)
        goto error;
    FreeLibrary(temp);
    temp = NULL;
    sip.pfCreate = CRYPT_LoadSIPFunc(pgSubject, L"CreateIndirectData\\", &temp);
    if (!sip.pfCreate || temp != lib)
        goto error;
    FreeLibrary(temp);
    temp = NULL;
    sip.pfVerify = CRYPT_LoadSIPFunc(pgSubject, L"VerifyIndirectData\\", &temp);
    if (!sip.pfVerify || temp != lib)
        goto error;
    FreeLibrary(temp);
    temp = NULL;
    sip.pfRemove = CRYPT_LoadSIPFunc(pgSubject, L"RemoveSignedDataMsg\\", &temp);
    if (!sip.pfRemove || temp != lib)
        goto error;
    FreeLibrary(temp);
    sip.hSIP = lib;
    CRYPT_CacheSIP(pgSubject, &sip);
    return TRUE;

error:
    FreeLibrary(lib);
    FreeLibrary(temp);
    SetLastError(TRUST_E_SUBJECT_FORM_UNKNOWN);
    return FALSE;
}

/***********************************************************************
 *             CryptSIPLoad (CRYPT32.@)
 *
 * Load some internal crypt32 functions into a SIP_DISPATCH_INFO structure.
 *
 * PARAMS
 *  pgSubject    [I] The GUID.
 *  dwFlags      [I] Flags.
 *  pSipDispatch [I] The loaded functions.
 *
 * RETURNS
 *  Success: TRUE. pSipDispatch contains the functions.
 *  Failure: FALSE. (Look at GetLastError()).
 *
 * NOTES
 *  CryptSIPLoad uses caching for the list of GUIDs and whether a SIP is
 *  already loaded.
 *
 *  An application calls CryptSipLoad which will return a structure with the
 *  function addresses of some internal crypt32 functions. The application will
 *  then call these functions which will be forwarded to the appropriate SIP.
 *
 *  CryptSIPLoad will load the needed SIP but doesn't unload this dll. The unloading
 *  is done when crypt32 is unloaded.
 */
BOOL WINAPI CryptSIPLoad
       (const GUID *pgSubject, DWORD dwFlags, SIP_DISPATCH_INFO *pSipDispatch)
{
    TRACE("(%s %ld %p)\n", debugstr_guid(pgSubject), dwFlags, pSipDispatch);

    if (!pgSubject || dwFlags != 0 || !pSipDispatch)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!CRYPT_IsSIPCached(pgSubject) && !CRYPT_LoadSIP(pgSubject))
        return FALSE;

    pSipDispatch->hSIP = NULL;
    pSipDispatch->pfGet = CryptSIPGetSignedDataMsg;
    pSipDispatch->pfPut = CryptSIPPutSignedDataMsg;
    pSipDispatch->pfCreate = CryptSIPCreateIndirectData;
    pSipDispatch->pfVerify = CryptSIPVerifyIndirectData;
    pSipDispatch->pfRemove = CryptSIPRemoveSignedDataMsg;

    return TRUE;
}

/***********************************************************************
 *             CryptSIPCreateIndirectData (CRYPT32.@)
 */
BOOL WINAPI CryptSIPCreateIndirectData(SIP_SUBJECTINFO* pSubjectInfo, DWORD* pcbIndirectData,
                                       SIP_INDIRECT_DATA* pIndirectData)
{
    WINE_SIP_PROVIDER *sip;
    BOOL ret = FALSE;

    TRACE("(%p %p %p)\n", pSubjectInfo, pcbIndirectData, pIndirectData);

    if (!pSubjectInfo || !pSubjectInfo->pgSubjectType || !pcbIndirectData)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if ((sip = CRYPT_GetCachedSIP(pSubjectInfo->pgSubjectType)))
        ret = sip->info.pfCreate(pSubjectInfo, pcbIndirectData, pIndirectData);
    TRACE("returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *             CryptSIPGetSignedDataMsg (CRYPT32.@)
 */
BOOL WINAPI CryptSIPGetSignedDataMsg(SIP_SUBJECTINFO* pSubjectInfo, DWORD* pdwEncodingType,
                                       DWORD dwIndex, DWORD* pcbSignedDataMsg, BYTE* pbSignedDataMsg)
{
    WINE_SIP_PROVIDER *sip;
    BOOL ret = FALSE;

    TRACE("(%p %p %ld %p %p)\n", pSubjectInfo, pdwEncodingType, dwIndex,
          pcbSignedDataMsg, pbSignedDataMsg);

    if ((sip = CRYPT_GetCachedSIP(pSubjectInfo->pgSubjectType)))
        ret = sip->info.pfGet(pSubjectInfo, pdwEncodingType, dwIndex,
         pcbSignedDataMsg, pbSignedDataMsg);
    TRACE("returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *             CryptSIPPutSignedDataMsg (CRYPT32.@)
 */
BOOL WINAPI CryptSIPPutSignedDataMsg(SIP_SUBJECTINFO* pSubjectInfo, DWORD pdwEncodingType,
                                       DWORD* pdwIndex, DWORD cbSignedDataMsg, BYTE* pbSignedDataMsg)
{
    WINE_SIP_PROVIDER *sip;
    BOOL ret = FALSE;

    TRACE("(%p %ld %p %ld %p)\n", pSubjectInfo, pdwEncodingType, pdwIndex,
          cbSignedDataMsg, pbSignedDataMsg);

    if ((sip = CRYPT_GetCachedSIP(pSubjectInfo->pgSubjectType)))
        ret = sip->info.pfPut(pSubjectInfo, pdwEncodingType, pdwIndex,
         cbSignedDataMsg, pbSignedDataMsg);
    TRACE("returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *             CryptSIPRemoveSignedDataMsg (CRYPT32.@)
 */
BOOL WINAPI CryptSIPRemoveSignedDataMsg(SIP_SUBJECTINFO* pSubjectInfo,
                                       DWORD dwIndex)
{
    WINE_SIP_PROVIDER *sip;
    BOOL ret = FALSE;

    TRACE("(%p %ld)\n", pSubjectInfo, dwIndex);

    if ((sip = CRYPT_GetCachedSIP(pSubjectInfo->pgSubjectType)))
        ret = sip->info.pfRemove(pSubjectInfo, dwIndex);
    TRACE("returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *             CryptSIPVerifyIndirectData (CRYPT32.@)
 */
BOOL WINAPI CryptSIPVerifyIndirectData(SIP_SUBJECTINFO* pSubjectInfo,
                                       SIP_INDIRECT_DATA* pIndirectData)
{
    WINE_SIP_PROVIDER *sip;
    BOOL ret = FALSE;

    TRACE("(%p %p)\n", pSubjectInfo, pIndirectData);

    if ((sip = CRYPT_GetCachedSIP(pSubjectInfo->pgSubjectType)))
        ret = sip->info.pfVerify(pSubjectInfo, pIndirectData);
    TRACE("returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *             CryptSIPRetrieveSubjectGuidForCatalogFile (CRYPT32.@)
 */
BOOL WINAPI CryptSIPRetrieveSubjectGuidForCatalogFile(LPCWSTR filename, HANDLE handle, GUID *subject)
{
    FIXME("(%s %p %p)\n", debugstr_w(filename), handle, subject);
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}
