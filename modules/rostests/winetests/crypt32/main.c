/*
 * Miscellaneous crypt32 tests
 *
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

#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <wincrypt.h>
#include <winreg.h>

#include "wine/test.h"

static HMODULE hCrypt;

static void test_findAttribute(void)
{
    PCRYPT_ATTRIBUTE ret;
    BYTE blobbin[] = {0x02,0x01,0x01};
    static CHAR oid[] = "1.2.3";
    CRYPT_ATTR_BLOB blobs[] = { { sizeof blobbin, blobbin }, };
    CRYPT_ATTRIBUTE attr = { oid, ARRAY_SIZE(blobs), blobs };

    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindAttribute(NULL, 0, NULL);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08x\n",
     GetLastError());
    if (0)
    {
        /* crashes */
        CertFindAttribute(NULL, 1, NULL);
        /* returns NULL, last error is ERROR_INVALID_PARAMETER
         * crashes on Vista
         */
        SetLastError(0xdeadbeef);
        ret = CertFindAttribute(NULL, 1, &attr);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d (%08x)\n", GetLastError(),
         GetLastError());
    }
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindAttribute("bogus", 1, &attr);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08x\n",
     GetLastError());
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindAttribute("1.2.4", 1, &attr);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08x\n",
     GetLastError());
    /* succeeds, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindAttribute("1.2.3", 1, &attr);
    ok(ret != NULL, "CertFindAttribute failed: %08x\n", GetLastError());
}

static void test_findExtension(void)
{
    PCERT_EXTENSION ret;
    static CHAR oid[] = "1.2.3";
    BYTE blobbin[] = {0x02,0x01,0x01};
    CERT_EXTENSION ext = { oid, TRUE, { sizeof blobbin, blobbin } };

    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindExtension(NULL, 0, NULL);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08x\n",
     GetLastError());
    if (0)
    {
        /* crashes */
        SetLastError(0xdeadbeef);
        CertFindExtension(NULL, 1, NULL);
        /* returns NULL, last error is ERROR_INVALID_PARAMETER
         * crashes on Vista
         */
        SetLastError(0xdeadbeef);
        ret = CertFindExtension(NULL, 1, &ext);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d (%08x)\n", GetLastError(),
         GetLastError());
    }
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindExtension("bogus", 1, &ext);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08x\n",
     GetLastError());
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindExtension("1.2.4", 1, &ext);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08x\n",
     GetLastError());
    /* succeeds, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindExtension("1.2.3", 1, &ext);
    ok(ret != NULL, "CertFindExtension failed: %08x\n", GetLastError());
}

static void test_findRDNAttr(void)
{
    PCERT_RDN_ATTR ret;
    static CHAR oid[] = "1.2.3";
    BYTE bin[] = { 0x16,0x09,'J','u','a','n',' ','L','a','n','g' };
    CERT_RDN_ATTR attrs[] = {
     { oid, CERT_RDN_IA5_STRING, { sizeof bin, bin } },
    };
    CERT_RDN rdns[] = { { ARRAY_SIZE(attrs), attrs } };
    CERT_NAME_INFO nameInfo = { ARRAY_SIZE(rdns), rdns };

    if (0)
    {
        /* crashes */
        SetLastError(0xdeadbeef);
        CertFindRDNAttr(NULL, NULL);
        /* returns NULL, last error is ERROR_INVALID_PARAMETER
         * crashes on Vista
         */
        SetLastError(0xdeadbeef);
        ret = CertFindRDNAttr(NULL, &nameInfo);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d (%08x)\n", GetLastError(),
         GetLastError());
    }
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindRDNAttr("bogus", &nameInfo);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08x\n",
     GetLastError());
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindRDNAttr("1.2.4", &nameInfo);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08x\n",
     GetLastError());
    /* succeeds, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindRDNAttr("1.2.3", &nameInfo);
    ok(ret != NULL, "CertFindRDNAttr failed: %08x\n", GetLastError());
}

static void test_verifyTimeValidity(void)
{
    SYSTEMTIME sysTime;
    FILETIME fileTime;
    CERT_INFO info = { 0 };
    LONG ret;

    GetSystemTime(&sysTime);
    SystemTimeToFileTime(&sysTime, &fileTime);
    /* crashes
    ret = CertVerifyTimeValidity(NULL, NULL);
    ret = CertVerifyTimeValidity(&fileTime, NULL);
     */
    /* Check with 0 NotBefore and NotAfter */
    ret = CertVerifyTimeValidity(&fileTime, &info);
    ok(ret == 1, "Expected 1, got %d\n", ret);
    info.NotAfter = fileTime;
    /* Check with NotAfter equal to comparison time */
    ret = CertVerifyTimeValidity(&fileTime, &info);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    /* Check with NotBefore after comparison time */
    info.NotBefore = fileTime;
    info.NotBefore.dwLowDateTime += 5000;
    ret = CertVerifyTimeValidity(&fileTime, &info);
    ok(ret == -1, "Expected -1, got %d\n", ret);
}

static void test_cryptAllocate(void)
{
    LPVOID buf;

    buf = CryptMemAlloc(0);
    ok(buf != NULL, "CryptMemAlloc failed: %08x\n", GetLastError());
    CryptMemFree(buf);
    /* CryptMemRealloc(NULL, 0) fails pre-Vista */
    buf = CryptMemAlloc(0);
    buf = CryptMemRealloc(buf, 1);
    ok(buf != NULL, "CryptMemRealloc failed: %08x\n", GetLastError());
    CryptMemFree(buf);
}


static void test_cryptTls(void)
{
    DWORD  (WINAPI *pI_CryptAllocTls)(void);
    LPVOID (WINAPI *pI_CryptDetachTls)(DWORD dwTlsIndex);
    LPVOID (WINAPI *pI_CryptGetTls)(DWORD dwTlsIndex);
    BOOL   (WINAPI *pI_CryptSetTls)(DWORD dwTlsIndex, LPVOID lpTlsValue);
    BOOL   (WINAPI *pI_CryptFreeTls)(DWORD dwTlsIndex, DWORD unknown);
    DWORD index;
    BOOL ret;

    pI_CryptAllocTls = (void *)GetProcAddress(hCrypt, "I_CryptAllocTls");
    pI_CryptDetachTls = (void *)GetProcAddress(hCrypt, "I_CryptDetachTls");
    pI_CryptGetTls = (void *)GetProcAddress(hCrypt, "I_CryptGetTls");
    pI_CryptSetTls = (void *)GetProcAddress(hCrypt, "I_CryptSetTls");
    pI_CryptFreeTls = (void *)GetProcAddress(hCrypt, "I_CryptFreeTls");

    /* One normal pass */
    index = pI_CryptAllocTls();
    ok(index, "I_CryptAllocTls failed: %08x\n", GetLastError());
    if (index)
    {
        LPVOID ptr;

        ptr = pI_CryptGetTls(index);
        ok(!ptr, "Expected NULL\n");
        ret = pI_CryptSetTls(index, (LPVOID)0xdeadbeef);
        ok(ret, "I_CryptSetTls failed: %08x\n", GetLastError());
        ptr = pI_CryptGetTls(index);
        ok(ptr == (LPVOID)0xdeadbeef, "Expected 0xdeadbeef, got %p\n", ptr);
        /* This crashes
        ret = pI_CryptFreeTls(index, 1);
         */
        ret = pI_CryptFreeTls(index, 0);
        ok(ret, "I_CryptFreeTls failed: %08x\n", GetLastError());
        ret = pI_CryptFreeTls(index, 0);
        ok(!ret, "I_CryptFreeTls succeeded\n");
        ok(GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
    }
    /* Similar pass, check I_CryptDetachTls */
    index = pI_CryptAllocTls();
    ok(index, "I_CryptAllocTls failed: %08x\n", GetLastError());
    if (index)
    {
        LPVOID ptr;

        ptr = pI_CryptGetTls(index);
        ok(!ptr, "Expected NULL\n");
        ret = pI_CryptSetTls(index, (LPVOID)0xdeadbeef);
        ok(ret, "I_CryptSetTls failed: %08x\n", GetLastError());
        ptr = pI_CryptGetTls(index);
        ok(ptr == (LPVOID)0xdeadbeef, "Expected 0xdeadbeef, got %p\n", ptr);
        ptr = pI_CryptDetachTls(index);
        ok(ptr == (LPVOID)0xdeadbeef, "Expected 0xdeadbeef, got %p\n", ptr);
        ptr = pI_CryptGetTls(index);
        ok(!ptr, "Expected NULL\n");
    }
}

static void test_readTrustedPublisherDWORD(void)
{

    BOOL (WINAPI *pReadDWORD)(LPCWSTR, DWORD *);

    pReadDWORD = (void *)GetProcAddress(hCrypt, "I_CryptReadTrustedPublisherDWORDValueFromRegistry");
    if (pReadDWORD)
    {
        static const WCHAR safer[] = { 
         'S','o','f','t','w','a','r','e','\\',
         'P','o','l','i','c','i','e','s','\\',
         'M','i','c','r','o','s','o','f','t','\\','S','y','s','t','e','m',
         'C','e','r','t','i','f','i','c','a','t','e','s','\\',
         'T','r','u','s','t','e','d','P','u','b','l','i','s','h','e','r',
         '\\','S','a','f','e','r',0 };
        static const WCHAR authenticodeFlags[] = { 'A','u','t','h','e','n',
         't','i','c','o','d','e','F','l','a','g','s',0 };
        BOOL ret, exists = FALSE;
        DWORD size, readFlags = 0, returnedFlags;
        HKEY key;
        LONG rc;

        rc = RegOpenKeyW(HKEY_LOCAL_MACHINE, safer, &key);
        if (rc == ERROR_SUCCESS)
        {
            size = sizeof(readFlags);
            rc = RegQueryValueExW(key, authenticodeFlags, NULL, NULL,
             (LPBYTE)&readFlags, &size);
            if (rc == ERROR_SUCCESS)
                exists = TRUE;
        }
        returnedFlags = 0xdeadbeef;
        ret = pReadDWORD(authenticodeFlags, &returnedFlags);
        ok(ret == exists, "Unexpected return value\n");
        ok(readFlags == returnedFlags,
         "Expected flags %08x, got %08x\n", readFlags, returnedFlags);
    }
}

static void test_getDefaultCryptProv(void)
{
#define ALG(id) id, #id
    static const struct
    {
        ALG_ID algid;
        const char *name;
        BOOL optional;
    } test_prov[] =
    {
        { ALG(CALG_MD2), TRUE },
        { ALG(CALG_MD4), TRUE },
        { ALG(CALG_MD5), TRUE },
        { ALG(CALG_SHA), TRUE },
        { ALG(CALG_RSA_SIGN) },
        { ALG(CALG_DSS_SIGN) },
        { ALG(CALG_NO_SIGN) },
        { ALG(CALG_ECDSA), TRUE },
        { ALG(CALG_ECDH), TRUE },
        { ALG(CALG_RSA_KEYX) },
        { ALG(CALG_RSA_KEYX) },
    };
#undef ALG
    HCRYPTPROV (WINAPI *pI_CryptGetDefaultCryptProv)(DWORD w);
    HCRYPTPROV prov;
    BOOL ret;
    DWORD size, i;
    LPSTR name;

    pI_CryptGetDefaultCryptProv = (void *)GetProcAddress(hCrypt, "I_CryptGetDefaultCryptProv");
    if (!pI_CryptGetDefaultCryptProv) return;

    prov = pI_CryptGetDefaultCryptProv(0xdeadbeef);
    ok(prov == 0 && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    prov = pI_CryptGetDefaultCryptProv(PROV_RSA_FULL);
    ok(prov == 0 && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    prov = pI_CryptGetDefaultCryptProv(1);
    ok(prov == 0 && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    prov = pI_CryptGetDefaultCryptProv(0);
    ok(prov != 0, "I_CryptGetDefaultCryptProv failed: %08x\n", GetLastError());
    CryptReleaseContext(prov, 0);

    for (i = 0; i < ARRAY_SIZE(test_prov); i++)
    {
        if (winetest_debug > 1)
            trace("%u: algid %#x (%s): class %u, type %u, sid %u\n", i, test_prov[i].algid, test_prov[i].name,
                  GET_ALG_CLASS(test_prov[i].algid) >> 13, GET_ALG_TYPE(test_prov[i].algid) >> 9, GET_ALG_SID(test_prov[i].algid));

        prov = pI_CryptGetDefaultCryptProv(test_prov[i].algid);
        if (!prov)
        {
todo_wine_if(test_prov[i].algid == CALG_DSS_SIGN || test_prov[i].algid == CALG_NO_SIGN)
            ok(test_prov[i].optional, "%u: I_CryptGetDefaultCryptProv(%#x) failed\n", i, test_prov[i].algid);
            continue;
        }

        ret = CryptGetProvParam(prov, PP_NAME, NULL, &size, 0);
        if (ret) /* some provders don't support PP_NAME */
        {
            name = CryptMemAlloc(size);
            ret = CryptGetProvParam(prov, PP_NAME, (BYTE *)name, &size, 0);
            ok(ret, "%u: CryptGetProvParam failed %#x\n", i, GetLastError());
            if (winetest_debug > 1)
                trace("%u: algid %#x, name %s\n", i, test_prov[i].algid, name);
            CryptMemFree(name);
        }

        CryptReleaseContext(prov, 0);
    }
}

static void test_CryptInstallOssGlobal(void)
{
    int (WINAPI *pI_CryptInstallOssGlobal)(DWORD,DWORD,DWORD);
    int ret,i;

    pI_CryptInstallOssGlobal = (void *)GetProcAddress(hCrypt,"I_CryptInstallOssGlobal");
    /* passing in some random values to I_CryptInstallOssGlobal, it always returns 9 the first time, then 10, 11 etc.*/
    for(i=0;i<30;i++)
    {
      ret =  pI_CryptInstallOssGlobal(rand(),rand(),rand());
      ok((9+i) == ret ||
         ret == 0, /* Vista */
         "Expected %d or 0, got %d\n",(9+i),ret);
    }
}

static const BYTE encodedInt[] = { 0x02,0x01,0x01 };
static const WCHAR encodedIntStr[] = { '0','2',' ','0','1',' ','0','1',0 };
static const BYTE encodedBigInt[] = { 0x02,0x1f,0x01,0x02,0x03,0x04,0x05,0x06,
 0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,
 0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f };
static const WCHAR encodedBigIntStr[] = { '0','2',' ','1','f',' ','0','1',' ',
 '0','2',' ','0','3',' ','0','4',' ','0','5',' ','0','6',' ','0','7',' ','0',
 '8',' ','0','9',' ','0','a',' ','0','b',' ','0','c',' ','0','d',' ','0','e',
 ' ','0','f',' ','1','0',' ','1','1',' ','1','2',' ','1','3',' ','1','4',' ',
 '1','5',' ','1','6',' ','1','7',' ','1','8',' ','1','9',' ','1','a',' ','1',
 'b',' ','1','c',' ','1','d',' ','1','e',' ','1','f',0 };

static void test_format_object(void)
{
    BOOL (WINAPI *pCryptFormatObject)(DWORD dwEncoding, DWORD dwFormatType,
        DWORD dwFormatStrType, void *pFormatStruct, LPCSTR lpszStructType,
        const BYTE *pbEncoded, DWORD dwEncoded, void *pbFormat,
        DWORD *pcbFormat);
    BOOL ret;
    DWORD size;
    LPWSTR str;

    pCryptFormatObject = (void *)GetProcAddress(hCrypt, "CryptFormatObject");
    if (!pCryptFormatObject)
    {
        skip("No CryptFormatObject\n");
        return;
    }
    /* Crash */
    if (0)
    {
        pCryptFormatObject(0, 0, 0, NULL, NULL, NULL, 0, NULL, NULL);
    }
    /* When called with any but the default encoding, it fails to find a
     * formatting function.
     */
    SetLastError(0xdeadbeef);
    ret = pCryptFormatObject(0, 0, 0, NULL, NULL, NULL, 0, NULL, &size);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    /* When called with the default encoding type for any undefined struct type
     * (including none), it succeeds:  the default encoding is a hex string
     * encoding.
     */
    SetLastError(0xdeadbeef);
    ret = pCryptFormatObject(X509_ASN_ENCODING, 0, 0, NULL, NULL, NULL, 0,
     NULL, &size);
    ok(ret, "CryptFormatObject failed: %d\n", GetLastError());
    if (ret)
    {
        if (size == 0 && GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            win_skip("CryptFormatObject has no default implementation\n");
            return;
        }
        ok(size == sizeof(WCHAR), "unexpected size %d\n", size);
        str = HeapAlloc(GetProcessHeap(), 0, size);
        SetLastError(0xdeadbeef);
        size = 0;
        ret = pCryptFormatObject(X509_ASN_ENCODING, 0, 0, NULL, NULL, NULL, 0,
         str, &size);
        ok(!ret && GetLastError() == ERROR_MORE_DATA,
         "expected ERROR_MORE_DATA, got %d\n", GetLastError());
        size = sizeof(WCHAR);
        ret = pCryptFormatObject(X509_ASN_ENCODING, 0, 0, NULL, NULL, NULL, 0,
         str, &size);
        ok(ret, "CryptFormatObject failed: %d\n", GetLastError());
        ok(!str[0], "expected empty string\n");
        HeapFree(GetProcessHeap(), 0, str);
    }
    ret = pCryptFormatObject(X509_ASN_ENCODING, 0, 0, NULL, NULL, encodedInt,
     sizeof(encodedInt), NULL, &size);
    ok(ret, "CryptFormatObject failed: %d\n", GetLastError());
    if (ret)
    {
        str = HeapAlloc(GetProcessHeap(), 0, size);
        ret = pCryptFormatObject(X509_ASN_ENCODING, 0, 0, NULL, NULL,
         encodedInt, sizeof(encodedInt), str, &size);
        ok(ret, "CryptFormatObject failed: %d\n", GetLastError());
        ok(!lstrcmpW(str, encodedIntStr), "unexpected format string\n");
        HeapFree(GetProcessHeap(), 0, str);
    }
    ret = pCryptFormatObject(X509_ASN_ENCODING, 0, 0, NULL, NULL,
     encodedBigInt, sizeof(encodedBigInt), NULL, &size);
    ok(ret, "CryptFormatObject failed: %d\n", GetLastError());
    if (ret)
    {
        str = HeapAlloc(GetProcessHeap(), 0, size);
        ret = pCryptFormatObject(X509_ASN_ENCODING, 0, 0, NULL, NULL,
         encodedBigInt, sizeof(encodedBigInt), str, &size);
        ok(ret, "CryptFormatObject failed: %d\n", GetLastError());
        ok(!lstrcmpiW(str, encodedBigIntStr), "unexpected format string\n");
        HeapFree(GetProcessHeap(), 0, str);
    }
    /* When called with the default encoding type for any undefined struct
     * type but CRYPT_FORMAT_STR_NO_HEX specified, it fails to find a
     * formatting function.
     */
    SetLastError(0xdeadbeef);
    ret = pCryptFormatObject(X509_ASN_ENCODING, 0, CRYPT_FORMAT_STR_NO_HEX,
     NULL, NULL, NULL, 0, NULL, &size);
    ok(!ret, "CryptFormatObject succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND ||
     GetLastError() == 0xdeadbeef, /* Vista, W2K8 */
     "expected ERROR_FILE_NOT_FOUND or no change, got %d\n", GetLastError());
    /* When called to format an AUTHORITY_KEY_ID2_INFO, it fails when no
     * data are given.
     */
    SetLastError(0xdeadbeef);
    ret = pCryptFormatObject(X509_ASN_ENCODING, 0, 0, NULL,
     szOID_AUTHORITY_KEY_IDENTIFIER2, NULL, 0, NULL, &size);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %d\n", GetLastError());
}

START_TEST(main)
{
    hCrypt = GetModuleHandleA("crypt32.dll");

    test_findAttribute();
    test_findExtension();
    test_findRDNAttr();
    test_verifyTimeValidity();
    test_cryptAllocate();
    test_cryptTls();
    test_readTrustedPublisherDWORD();
    test_getDefaultCryptProv();
    test_CryptInstallOssGlobal();
    test_format_object();
}
