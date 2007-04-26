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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winreg.h"
#include "winnls.h"
#include "mssip.h"
#include "winuser.h"
#include "advpub.h"
#include "crypt32_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

static HCRYPTPROV hDefProv;

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstance);
            crypt_oid_init(hInstance);
            break;
        case DLL_PROCESS_DETACH:
            crypt_oid_free();
            if (hDefProv) CryptReleaseContext(hDefProv, 0);
            break;
    }
    return TRUE;
}

HCRYPTPROV CRYPT_GetDefaultProvider(void)
{
    if (!hDefProv)
        CryptAcquireContextW(&hDefProv, NULL, MS_ENHANCED_PROV_W,
         PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    return hDefProv;
}

typedef void * HLRUCACHE;

/* this function is called by Internet Explorer when it is about to verify a
 * downloaded component.  The first parameter appears to be a pointer to an
 * unknown type, native fails unless it points to a buffer of at least 20 bytes.
 * The second parameter appears to be an out parameter, whatever it's set to is
 * passed (by cryptnet.dll) to I_CryptFlushLruCache.
 */
BOOL WINAPI I_CryptCreateLruCache(void *unknown, HLRUCACHE *out)
{
    FIXME("(%p, %p): stub!\n", unknown, out);
    *out = (void *)0xbaadf00d;
    return TRUE;
}

BOOL WINAPI I_CryptFindLruEntryData(DWORD unk0, DWORD unk1, DWORD unk2)
{
    FIXME("(%08x, %08x, %08x): stub!\n", unk0, unk1, unk2);
    return FALSE;
}

DWORD WINAPI I_CryptFlushLruCache(HLRUCACHE h, DWORD unk0, DWORD unk1)
{
    FIXME("(%p, %08x, %08x): stub!\n", h, unk0, unk1);
    return 0;
}

HLRUCACHE WINAPI I_CryptFreeLruCache(HLRUCACHE h, DWORD unk0, DWORD unk1)
{
    FIXME("(%p, %08x, %08x): stub!\n", h, unk0, unk1);
    return h;
}

LPVOID WINAPI CryptMemAlloc(ULONG cbSize)
{
    return HeapAlloc(GetProcessHeap(), 0, cbSize);
}

LPVOID WINAPI CryptMemRealloc(LPVOID pv, ULONG cbSize)
{
    return HeapReAlloc(GetProcessHeap(), 0, pv, cbSize);
}

VOID WINAPI CryptMemFree(LPVOID pv)
{
    HeapFree(GetProcessHeap(), 0, pv);
}

DWORD WINAPI I_CryptAllocTls(void)
{
    return TlsAlloc();
}

LPVOID WINAPI I_CryptDetachTls(DWORD dwTlsIndex)
{
    LPVOID ret;

    ret = TlsGetValue(dwTlsIndex);
    TlsSetValue(dwTlsIndex, NULL);
    return ret;
}

LPVOID WINAPI I_CryptGetTls(DWORD dwTlsIndex)
{
    return TlsGetValue(dwTlsIndex);
}

BOOL WINAPI I_CryptSetTls(DWORD dwTlsIndex, LPVOID lpTlsValue)
{
    return TlsSetValue(dwTlsIndex, lpTlsValue);
}

BOOL WINAPI I_CryptFreeTls(DWORD dwTlsIndex, DWORD unknown)
{
    TRACE("(%d, %d)\n", dwTlsIndex, unknown);
    return TlsFree(dwTlsIndex);
}

BOOL WINAPI I_CryptGetOssGlobal(DWORD x)
{
    FIXME("%08x\n", x);
    return FALSE;
}

HCRYPTPROV WINAPI I_CryptGetDefaultCryptProv(DWORD reserved)
{
    HCRYPTPROV ret;

    TRACE("(%08x)\n", reserved);

    if (reserved)
    {
        SetLastError(E_INVALIDARG);
        return (HCRYPTPROV)0;
    }
    ret = CRYPT_GetDefaultProvider();
    CryptContextAddRef(ret, NULL, 0);
    return ret;
}

BOOL WINAPI I_CryptReadTrustedPublisherDWORDValueFromRegistry(LPCWSTR name,
 DWORD *value)
{
    static const WCHAR safer[] = { 
     'S','o','f','t','w','a','r','e','\\','P','o','l','i','c','i','e','s','\\',
     'M','i','c','r','o','s','o','f','t','\\','S','y','s','t','e','m',
     'C','e','r','t','i','f','i','c','a','t','e','s','\\',
     'T','r','u','s','t','e','d','P','u','b','l','i','s','h','e','r','\\',
     'S','a','f','e','r',0 };
    HKEY key;
    LONG rc;
    BOOL ret = FALSE;

    TRACE("(%s, %p)\n", debugstr_w(name), value);

    *value = 0;
    rc = RegCreateKeyW(HKEY_LOCAL_MACHINE, safer, &key);
    if (rc == ERROR_SUCCESS)
    {
        DWORD size = sizeof(DWORD);

        if (!RegQueryValueExW(key, name, NULL, NULL, (LPBYTE)value, &size))
            ret = TRUE;
        RegCloseKey(key);
    }
    return ret;
}

int WINAPI I_CryptInstallOssGlobal(DWORD x, DWORD y, DWORD z)
{
    static int ret = 8;
    ret++;
    FIXME("%08x %08x %08x, return value %d\n", x, y, z,ret);
    return ret;
}

BOOL WINAPI I_CryptInstallAsn1Module(void *x, DWORD y, DWORD z)
{
    FIXME("%p %08x %08x\n", x, y, z);
    return TRUE;
}

BOOL WINAPI I_CryptUninstallAsn1Module(void *x)
{
    FIXME("%p\n", x);
    return TRUE;
}

BOOL WINAPI CryptFormatObject(DWORD dwCertEncodingType, DWORD dwFormatType,
 DWORD dwFormatStrType, void *pFormatStruct, LPCSTR lpszStructType,
 const BYTE *pbEncoded, DWORD cbEncoded, void *pbFormat, DWORD *pcbFormat)
{
    FIXME("(%08x, %d, %d, %p, %s, %p, %d, %p, %p): stub\n",
     dwCertEncodingType, dwFormatType, dwFormatStrType, pFormatStruct,
     debugstr_a(lpszStructType), pbEncoded, cbEncoded, pbFormat, pcbFormat);
    return FALSE;
}

BOOL WINAPI CryptQueryObject(DWORD dwObjectType, const void* pvObject,
    DWORD dwExpectedContentTypeFlags, DWORD dwExpectedFormatTypeFlags,
    DWORD dwFlags, DWORD* pdwMsgAndCertEncodingType, DWORD* pdwContentType,
    DWORD* pdwFormatType, HCERTSTORE* phCertStore, HCRYPTMSG* phMsg,
    const void** ppvContext)
{
    FIXME( "%08x %p %08x %08x %08x %p %p %p %p %p %p\n", dwObjectType,
           pvObject, dwExpectedContentTypeFlags, dwExpectedFormatTypeFlags,
           dwFlags, pdwMsgAndCertEncodingType, pdwContentType, pdwFormatType,
           phCertStore, phMsg, ppvContext);
    return FALSE;
}

BOOL WINAPI CryptVerifyMessageSignature(PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
          DWORD dwSignerIndex, const BYTE* pbSignedBlob, DWORD cbSignedBlob,
          BYTE* pbDecoded, DWORD* pcbDecoded, PCCERT_CONTEXT* ppSignerCert)
{
    FIXME("stub: %p, %d, %p, %d, %p, %p, %p\n",
        pVerifyPara, dwSignerIndex, pbSignedBlob, cbSignedBlob,
        pbDecoded, pcbDecoded, ppSignerCert);
    if (ppSignerCert)
        *ppSignerCert = NULL;
    return FALSE;
}
