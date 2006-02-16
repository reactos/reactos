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

#include "config.h"
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winreg.h"
#include "winnls.h"
#include "mssip.h"
#include "crypt32_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

static HCRYPTPROV hDefProv;

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            CRYPT_InitFunctionSets();
            break;
        case DLL_PROCESS_DETACH:
            CRYPT_FreeFunctionSets();
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
    FIXME("(%08lx, %08lx, %08lx): stub!\n", unk0, unk1, unk2);
    return FALSE;
}

DWORD WINAPI I_CryptFlushLruCache(HLRUCACHE h, DWORD unk0, DWORD unk1)
{
    FIXME("(%p, %08lx, %08lx): stub!\n", h, unk0, unk1);
    return 0;
}

HLRUCACHE WINAPI I_CryptFreeLruCache(HLRUCACHE h, DWORD unk0, DWORD unk1)
{
    FIXME("(%p, %08lx, %08lx): stub!\n", h, unk0, unk1);
    return h;
}

BOOL WINAPI CryptSIPRemoveProvider(GUID *pgProv)
{
    FIXME("stub!\n");
    return FALSE;
}

/* convert a guid to a wide character string */
static void CRYPT_guid2wstr( LPGUID guid, LPWSTR wstr )
{
    char str[40];

    sprintf(str, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
           guid->Data1, guid->Data2, guid->Data3,
           guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
           guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
    MultiByteToWideChar( CP_ACP, 0, str, -1, wstr, 40 );
}

/*
 * Helper for CryptSIPAddProvider
 *
 * Add a registry key containing a dll name and function under
 *  "Software\\Microsoft\\Cryptography\\OID\\EncodingType 0\\<func>\\<guid>"
 */
static LONG CRYPT_SIPWriteFunction( LPGUID guid, LPCWSTR szKey, 
              LPCWSTR szDll, LPCWSTR szFunction )
{
    static const WCHAR szOID[] = {
        'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'C','r','y','p','t','o','g','r','a','p','h','y','\\',
        'O','I','D','\\',
        'E','n','c','o','d','i','n','g','T','y','p','e',' ','0','\\',
        'C','r','y','p','t','S','I','P','D','l','l', 0 };
    static const WCHAR szBackSlash[] = { '\\', 0 };
    static const WCHAR szDllName[] = { 'D','l','l',0 };
    static const WCHAR szFuncName[] = { 'F','u','n','c','N','a','m','e',0 };
    WCHAR szFullKey[ 0x100 ];
    LONG r;
    HKEY hKey;

    if( !szFunction )
         return ERROR_SUCCESS;

    /* max length of szFullKey depends on our code only, so we won't overrun */
    lstrcpyW( szFullKey, szOID );
    lstrcatW( szFullKey, szKey );
    lstrcatW( szFullKey, szBackSlash );
    CRYPT_guid2wstr( guid, &szFullKey[ lstrlenW( szFullKey ) ] );
    lstrcatW( szFullKey, szBackSlash );

    TRACE("key is %s\n", debugstr_w( szFullKey ) );

    r = RegCreateKeyW( HKEY_LOCAL_MACHINE, szFullKey, &hKey );
    if( r != ERROR_SUCCESS )
        return r;

    /* write the values */
    RegSetValueExW( hKey, szFuncName, 0, REG_SZ, (const BYTE*) szFunction,
                    ( lstrlenW( szFunction ) + 1 ) * sizeof (WCHAR) );
    RegSetValueExW( hKey, szDllName, 0, REG_SZ, (const BYTE*) szDll,
                    ( lstrlenW( szDll ) + 1) * sizeof (WCHAR) );

    RegCloseKey( hKey );

    return ERROR_SUCCESS;
}

BOOL WINAPI CryptSIPAddProvider(SIP_ADD_NEWPROVIDER *psNewProv)
{
    static const WCHAR szCreate[] = {
       'C','r','e','a','t','e',
       'I','n','d','i','r','e','c','t','D','a','t','a',0};
    static const WCHAR szGetSigned[] = {
       'G','e','t','S','i','g','n','e','d','D','a','t','a','M','s','g',0};
    static const WCHAR szIsMyFile[] = {
       'I','s','M','y','F','i','l','e','T','y','p','e', 0 };
    static const WCHAR szPutSigned[] = {
       'P','u','t','S','i','g','n','e','d','D','a','t','a','M','s','g',0};
    static const WCHAR szRemoveSigned[] = {
       'R','e','m','o','v','e',
       'S','i','g','n','e','d','D','a','t','a','M','s','g',0};
    static const WCHAR szVerify[] = {
       'V','e','r','i','f','y',
       'I','n','d','i','r','e','c','t','D','a','t','a',0};

    TRACE("%p\n", psNewProv);

    if( !psNewProv )
        return FALSE;

    TRACE("%s %s %s %s\n",
          debugstr_guid( psNewProv->pgSubject ),
          debugstr_w( psNewProv->pwszDLLFileName ),
          debugstr_w( psNewProv->pwszMagicNumber ),
          debugstr_w( psNewProv->pwszIsFunctionName ) );

#define CRYPT_SIPADDPROV( key, field ) \
    CRYPT_SIPWriteFunction( psNewProv->pgSubject, key, \
           psNewProv->pwszDLLFileName, psNewProv->field)

    CRYPT_SIPADDPROV( szGetSigned, pwszGetFuncName );
    CRYPT_SIPADDPROV( szPutSigned, pwszPutFuncName );
    CRYPT_SIPADDPROV( szCreate, pwszCreateFuncName );
    CRYPT_SIPADDPROV( szVerify, pwszVerifyFuncName );
    CRYPT_SIPADDPROV( szRemoveSigned, pwszRemoveFuncName );
    CRYPT_SIPADDPROV( szIsMyFile, pwszIsFunctionNameFmt2 );

#undef CRYPT_SIPADDPROV

    return TRUE;
}

BOOL WINAPI CryptSIPRetrieveSubjectGuid
      (LPCWSTR FileName, HANDLE hFileIn, GUID *pgSubject)
{
    FIXME("stub!\n");
    return FALSE;
}

BOOL WINAPI CryptSIPLoad
       (const GUID *pgSubject, DWORD dwFlags, SIP_DISPATCH_INFO *pSipDispatch)
{
    FIXME("stub!\n");
    return FALSE;
}

struct OIDToAlgID
{
    LPCSTR oid;
    DWORD algID;
};

static const struct OIDToAlgID oidToAlgID[] = {
 { szOID_RSA_RSA, CALG_RSA_KEYX },
 { szOID_RSA_MD2RSA, CALG_MD2 },
 { szOID_RSA_MD4RSA, CALG_MD4 },
 { szOID_RSA_MD5RSA, CALG_MD5 },
 { szOID_RSA_SHA1RSA, CALG_SHA },
 { szOID_RSA_DH, CALG_DH_SF },
 { szOID_RSA_SMIMEalgESDH, CALG_DH_EPHEM },
 { szOID_RSA_SMIMEalgCMS3DESwrap, CALG_3DES },
 { szOID_RSA_SMIMEalgCMSRC2wrap, CALG_RC2 },
 { szOID_RSA_MD2, CALG_MD2 },
 { szOID_RSA_MD4, CALG_MD4 },
 { szOID_RSA_MD5, CALG_MD5 },
 { szOID_RSA_RC2CBC, CALG_RC2 },
 { szOID_RSA_RC4, CALG_RC4 },
 { szOID_RSA_DES_EDE3_CBC, CALG_3DES },
 { szOID_ANSI_X942_DH, CALG_DH_SF },
 { szOID_X957_DSA, CALG_DSS_SIGN },
 { szOID_X957_SHA1DSA, CALG_SHA },
 { szOID_OIWSEC_md4RSA, CALG_MD4 },
 { szOID_OIWSEC_md5RSA, CALG_MD5 },
 { szOID_OIWSEC_md4RSA2, CALG_MD4 },
 { szOID_OIWSEC_desCBC, CALG_DES },
 { szOID_OIWSEC_dsa, CALG_DSS_SIGN },
 { szOID_OIWSEC_shaDSA, CALG_SHA },
 { szOID_OIWSEC_shaRSA, CALG_SHA },
 { szOID_OIWSEC_sha, CALG_SHA },
 { szOID_OIWSEC_rsaXchg, CALG_RSA_KEYX },
 { szOID_OIWSEC_sha1, CALG_SHA },
 { szOID_OIWSEC_dsaSHA1, CALG_SHA },
 { szOID_OIWSEC_sha1RSASign, CALG_SHA },
 { szOID_OIWDIR_md2RSA, CALG_MD2 },
 { szOID_INFOSEC_mosaicUpdatedSig, CALG_SHA },
 { szOID_INFOSEC_mosaicKMandUpdSig, CALG_DSS_SIGN },
};

LPCSTR WINAPI CertAlgIdToOID(DWORD dwAlgId)
{
    switch (dwAlgId)
    {
    case CALG_RSA_KEYX:
        return szOID_RSA_RSA;
    case CALG_DH_EPHEM:
        return szOID_RSA_SMIMEalgESDH;
    case CALG_MD2:
        return szOID_RSA_MD2;
    case CALG_MD4:
        return szOID_RSA_MD4;
    case CALG_MD5:
        return szOID_RSA_MD5;
    case CALG_RC2:
        return szOID_RSA_RC2CBC;
    case CALG_RC4:
        return szOID_RSA_RC4;
    case CALG_3DES:
        return szOID_RSA_DES_EDE3_CBC;
    case CALG_DH_SF:
        return szOID_ANSI_X942_DH;
    case CALG_DSS_SIGN:
        return szOID_X957_DSA;
    case CALG_DES:
        return szOID_OIWSEC_desCBC;
    case CALG_SHA:
        return szOID_OIWSEC_sha1;
    default:
        return NULL;
    }
}

DWORD WINAPI CertOIDToAlgId(LPCSTR pszObjId)
{
    int i;

    if (pszObjId)
    {
        for (i = 0; i < sizeof(oidToAlgID) / sizeof(oidToAlgID[0]); i++)
        {
            if (!strcmp(pszObjId, oidToAlgID[i].oid))
                return oidToAlgID[i].algID;
        }
    }
    return 0;
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
    TRACE("(%ld, %ld)\n", dwTlsIndex, unknown);
    return TlsFree(dwTlsIndex);
}
