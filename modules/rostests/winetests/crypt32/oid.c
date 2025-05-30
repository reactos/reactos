/*
 * Unit test suite for crypt32.dll's OID support functions.
 *
 * Copyright 2005 Juan Lang
 * Copyright 2018 Dmitry Timoshkov
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
#define CRYPT_OID_INFO_HAS_EXTRA_FIELDS
#include <wincrypt.h>
#include <winreg.h>

#include "wine/test.h"

struct OIDToAlgID
{
    LPCSTR oid;
    DWORD algID;
    DWORD altAlgID;
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
 { szOID_NIST_sha256, CALG_SHA_256, -1 },
 { szOID_NIST_sha384, CALG_SHA_384, -1 },
 { szOID_NIST_sha512, CALG_SHA_512, -1 },
 { szOID_ECC_PUBLIC_KEY, CALG_OID_INFO_PARAMETERS },
};

static const struct OIDToAlgID algIDToOID[] = {
 { szOID_RSA_RSA, CALG_RSA_KEYX },
 { szOID_RSA_SMIMEalgESDH, CALG_DH_EPHEM },
 { szOID_RSA_MD2, CALG_MD2 },
 { szOID_RSA_MD4, CALG_MD4 },
 { szOID_RSA_MD5, CALG_MD5 },
 { szOID_RSA_RC2CBC, CALG_RC2 },
 { szOID_RSA_RC4, CALG_RC4 },
 { szOID_RSA_DES_EDE3_CBC, CALG_3DES },
 { szOID_ANSI_X942_DH, CALG_DH_SF },
 { szOID_X957_DSA, CALG_DSS_SIGN },
 { szOID_OIWSEC_desCBC, CALG_DES },
 { szOID_OIWSEC_sha1, CALG_SHA },
};

static void test_OIDToAlgID(void)
{
    int i;
    DWORD alg;

    /* Test with a bogus one */
    alg = CertOIDToAlgId("1.2.3");
    ok(!alg, "Expected failure, got %ld\n", alg);

    for (i = 0; i < ARRAY_SIZE(oidToAlgID); i++)
    {
        alg = CertOIDToAlgId(oidToAlgID[i].oid);
        ok(alg == oidToAlgID[i].algID || (oidToAlgID[i].altAlgID && alg == oidToAlgID[i].altAlgID),
         "Expected %ld, got %ld\n", oidToAlgID[i].algID, alg);
    }
}

static void test_AlgIDToOID(void)
{
    int i;
    LPCSTR oid;

    /* Test with a bogus one */
    SetLastError(0xdeadbeef);
    oid = CertAlgIdToOID(ALG_CLASS_SIGNATURE | ALG_TYPE_ANY | 80);
    ok(!oid && GetLastError() == 0xdeadbeef,
     "Didn't expect last error (%08lx) to be set\n", GetLastError());
    for (i = 0; i < ARRAY_SIZE(algIDToOID); i++)
    {
        oid = CertAlgIdToOID(algIDToOID[i].algID);
        /* Allow failure, not every version of Windows supports every algo */
        ok(oid != NULL, "CertAlgIdToOID failed, expected %s\n", algIDToOID[i].oid);
        if (oid)
            ok(!strcmp(oid, algIDToOID[i].oid), "Expected %s, got %s\n", algIDToOID[i].oid, oid);
    }
}

static void test_oidFunctionSet(void)
{
    HCRYPTOIDFUNCSET set1, set2;
    BOOL ret;
    LPWSTR buf = NULL;
    DWORD size;

    /* This crashes
    set = CryptInitOIDFunctionSet(NULL, 0);
     */

    /* The name doesn't mean much */
    set1 = CryptInitOIDFunctionSet("funky", 0);
    ok(set1 != 0, "CryptInitOIDFunctionSet failed: %08lx\n", GetLastError());
    if (set1)
    {
        /* These crash
        ret = CryptGetDefaultOIDDllList(NULL, 0, NULL, NULL);
        ret = CryptGetDefaultOIDDllList(NULL, 0, NULL, &size);
         */
        size = 0;
        ret = CryptGetDefaultOIDDllList(set1, 0, NULL, &size);
        ok(ret, "CryptGetDefaultOIDDllList failed: %08lx\n", GetLastError());
        if (ret)
        {
            buf = malloc(size * sizeof(WCHAR));
            if (buf)
            {
                ret = CryptGetDefaultOIDDllList(set1, 0, buf, &size);
                ok(ret, "CryptGetDefaultOIDDllList failed: %08lx\n",
                 GetLastError());
                ok(!*buf, "Expected empty DLL list\n");
                free(buf);
            }
        }
    }

    /* MSDN says flags must be 0, but it's not checked */
    set1 = CryptInitOIDFunctionSet("", 1);
    ok(set1 != 0, "CryptInitOIDFunctionSet failed: %08lx\n", GetLastError());
    set2 = CryptInitOIDFunctionSet("", 0);
    ok(set2 != 0, "CryptInitOIDFunctionSet failed: %08lx\n", GetLastError());
    /* There isn't a free function, so there must be only one set per name to
     * limit leaks.  (I guess the sets are freed when crypt32 is unloaded.)
     */
    ok(set1 == set2, "Expected identical sets\n");
    if (set1)
    {
        /* The empty name function set used here seems to correspond to
         * DEFAULT.
         */
    }

    /* There's no installed function for a built-in encoding. */
    set1 = CryptInitOIDFunctionSet("CryptDllEncodeObject", 0);
    ok(set1 != 0, "CryptInitOIDFunctionSet failed: %08lx\n", GetLastError());
    if (set1)
    {
        void *funcAddr;
        HCRYPTOIDFUNCADDR hFuncAddr;

        ret = CryptGetOIDFunctionAddress(set1, X509_ASN_ENCODING, X509_CERT, 0,
         &funcAddr, &hFuncAddr);
        ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
         "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    }
}

typedef int (*funcY)(int);

static int funky(int x)
{
    return x;
}

static void test_installOIDFunctionAddress(void)
{
    BOOL ret;
    CRYPT_OID_FUNC_ENTRY entry = { CRYPT_DEFAULT_OID, funky };
    HCRYPTOIDFUNCSET set;

    /* This crashes
    ret = CryptInstallOIDFunctionAddress(NULL, 0, NULL, 0, NULL, 0);
     */

    /* Installing zero functions should work */
    SetLastError(0xdeadbeef);
    ret = CryptInstallOIDFunctionAddress(NULL, 0, "CryptDllEncodeObject", 0,
     NULL, 0);
    ok(ret && GetLastError() == 0xdeadbeef, "Expected success, got %08lx\n",
     GetLastError());

    /* The function name doesn't much matter */
    SetLastError(0xdeadbeef);
    ret = CryptInstallOIDFunctionAddress(NULL, 0, "OhSoFunky", 0, NULL, 0);
    ok(ret && GetLastError() == 0xdeadbeef, "Expected success, got %08lx\n",
     GetLastError());
    SetLastError(0xdeadbeef);
    entry.pszOID = X509_CERT;
    ret = CryptInstallOIDFunctionAddress(NULL, 0, "OhSoFunky", 1, &entry, 0);
    ok(ret && GetLastError() == 0xdeadbeef, "Expected success, got %08lx\n",
     GetLastError());
    set = CryptInitOIDFunctionSet("OhSoFunky", 0);
    ok(set != 0, "CryptInitOIDFunctionSet failed: %08lx\n", GetLastError());
    if (set)
    {
        funcY funcAddr = NULL;
        HCRYPTOIDFUNCADDR hFuncAddr = NULL;

        /* This crashes
        ret = CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING, 0, 0, NULL,
         NULL);
         */
        ret = CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING, 0, 0,
         (void **)&funcAddr, &hFuncAddr);
        ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
         "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());
        ret = CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING, X509_CERT, 0,
         (void **)&funcAddr, &hFuncAddr);
        ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
         "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());
        ret = CryptGetOIDFunctionAddress(set, 0, X509_CERT, 0,
         (void **)&funcAddr, &hFuncAddr);
        ok(ret, "CryptGetOIDFunctionAddress failed: %ld\n", GetLastError());
        if (funcAddr)
        {
            int y = funcAddr(0xabadc0da);

            ok(y == 0xabadc0da, "Unexpected return (%d) from function\n", y);
            CryptFreeOIDFunctionAddress(hFuncAddr, 0);
        }
    }
}

static void test_registerOIDFunction(void)
{
    BOOL ret;

    /* oddly, this succeeds under WinXP; the function name key is merely
     * omitted.  This may be a side effect of the registry code, I don't know.
     * I don't check it because I doubt anyone would depend on it.
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, NULL,
     "1.2.3.4.5.6.7.8.9.10", L"bogus.dll", NULL);
     */
    /* On windows XP, GetLastError is incorrectly being set with an HRESULT,
     * E_INVALIDARG
     */
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, "foo", NULL, L"bogus.dll",
     NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG: %ld\n", GetLastError());
    /* This has no effect, but "succeeds" on XP */
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, "foo",
     "1.2.3.4.5.6.7.8.9.10", NULL, NULL);
    ok(ret, "Expected pseudo-success, got %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10", L"bogus.dll", NULL);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Need admin rights\n");
        return;
    }
    ok(ret, "CryptRegisterOIDFunction failed: %ld\n", GetLastError());
    ret = CryptUnregisterOIDFunction(X509_ASN_ENCODING, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10");
    ok(ret, "CryptUnregisterOIDFunction failed: %ld\n", GetLastError());
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, "bogus",
     "1.2.3.4.5.6.7.8.9.10", L"bogus.dll", NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %ld\n", GetLastError());
    ret = CryptUnregisterOIDFunction(X509_ASN_ENCODING, "bogus",
     "1.2.3.4.5.6.7.8.9.10");
    ok(ret, "CryptUnregisterOIDFunction failed: %ld\n", GetLastError());
    /* Unwanted Cryptography\OID\EncodingType 1\bogus\ will still be there */
    ok(!RegDeleteKeyA(HKEY_LOCAL_MACHINE,
     "SOFTWARE\\Microsoft\\Cryptography\\OID\\EncodingType 1\\bogus"),
     "Could not delete bogus key\n");
    /* Shouldn't have effect but registry keys are created */
    ret = CryptRegisterOIDFunction(PKCS_7_ASN_ENCODING, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10", L"bogus.dll", NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %ld\n", GetLastError());
    ret = CryptUnregisterOIDFunction(PKCS_7_ASN_ENCODING, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10");
    ok(ret, "CryptUnregisterOIDFunction failed: %ld\n", GetLastError());
    /* Check with bogus encoding type. Registry keys are still created */
    ret = CryptRegisterOIDFunction(0, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10", L"bogus.dll", NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %ld\n", GetLastError());
    ret = CryptUnregisterOIDFunction(0, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10");
    ok(ret, "CryptUnregisterOIDFunction failed: %ld\n", GetLastError());
    /* Unwanted Cryptography\OID\EncodingType 0\CryptDllEncodeObject\
     * will still be there
     */
    ok(!RegDeleteKeyA(HKEY_LOCAL_MACHINE,
     "SOFTWARE\\Microsoft\\Cryptography\\OID\\EncodingType 0\\CryptDllEncodeObject"),
     "Could not delete CryptDllEncodeObject key\n");
    /* This is written with value 3 verbatim.  Thus, the encoding type isn't
     * (for now) treated as a mask. Registry keys are created.
     */
    ret = CryptRegisterOIDFunction(3, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10", L"bogus.dll", NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %ld\n", GetLastError());
    ret = CryptUnregisterOIDFunction(3, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10");
    ok(ret, "CryptUnregisterOIDFunction failed: %ld\n", GetLastError());
    /* Unwanted Cryptography\OID\EncodingType 3\CryptDllEncodeObject
     * will still be there.
     */
    ok(!RegDeleteKeyA(HKEY_LOCAL_MACHINE,
     "SOFTWARE\\Microsoft\\Cryptography\\OID\\EncodingType 3\\CryptDllEncodeObject"),
     "Could not delete CryptDllEncodeObject key\n");
    ok(!RegDeleteKeyA(HKEY_LOCAL_MACHINE,
     "SOFTWARE\\Microsoft\\Cryptography\\OID\\EncodingType 3"),
     "Could not delete 'EncodingType 3' key\n");
}

static void test_registerDefaultOIDFunction(void)
{
    static const char fmt[] =
     "Software\\Microsoft\\Cryptography\\OID\\EncodingType %d\\%s\\DEFAULT";
    static const char func[] = "CertDllOpenStoreProv";
    char buf[MAX_PATH];
    BOOL ret;
    LSTATUS rc;
    HKEY key;

    ret = CryptRegisterDefaultOIDFunction(0, NULL, 0, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* This succeeds on WinXP, although the bogus entry is unusable.
    ret = CryptRegisterDefaultOIDFunction(0, NULL, 0, L"bogus.dll");
     */
    /* Register one at index 0 */
    SetLastError(0xdeadbeef);
    ret = CryptRegisterDefaultOIDFunction(0, "CertDllOpenStoreProv", 0,
     L"bogus.dll");
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Need admin rights\n");
        return;
    }
    ok(ret, "CryptRegisterDefaultOIDFunction failed: %08lx\n", GetLastError());
    /* Reregistering should fail */
    ret = CryptRegisterDefaultOIDFunction(0, "CertDllOpenStoreProv", 0,
     L"bogus.dll");
    ok(!ret && GetLastError() == ERROR_FILE_EXISTS,
     "Expected ERROR_FILE_EXISTS, got %08lx\n", GetLastError());
    /* Registering the same one at index 1 should also fail */
    ret = CryptRegisterDefaultOIDFunction(0, "CertDllOpenStoreProv", 1,
     L"bogus.dll");
    ok(!ret && GetLastError() == ERROR_FILE_EXISTS,
     "Expected ERROR_FILE_EXISTS, got %08lx\n", GetLastError());
    /* Registering a different one at index 1 succeeds */
    ret = CryptRegisterDefaultOIDFunction(0, "CertDllOpenStoreProv", 1,
     L"bogus2.dll");
    ok(ret, "CryptRegisterDefaultOIDFunction failed: %08lx\n", GetLastError());
    sprintf(buf, fmt, 0, func);
    rc = RegOpenKeyA(HKEY_LOCAL_MACHINE, buf, &key);
    ok(rc == 0, "Expected key to exist, RegOpenKeyA failed: %ld\n", rc);
    if (rc == 0)
    {
        CHAR dllBuf[MAX_PATH];
        DWORD type, size;
        LPSTR ptr;

        size = ARRAY_SIZE(dllBuf);
        rc = RegQueryValueExA(key, "Dll", NULL, &type, (LPBYTE)dllBuf, &size);
        ok(rc == 0,
         "Expected Dll value to exist, RegQueryValueExA failed: %ld\n", rc);
        ok(type == REG_MULTI_SZ, "Expected type REG_MULTI_SZ, got %ld\n", type);
        /* bogus.dll was registered first, so that should be first */
        ptr = dllBuf;
        ok(!lstrcmpiA(ptr, "bogus.dll"), "Unexpected dll\n");
        ptr += lstrlenA(ptr) + 1;
        ok(!lstrcmpiA(ptr, "bogus2.dll"), "Unexpected dll\n");
        RegCloseKey(key);
    }
    /* Unregister both of them */
    ret = CryptUnregisterDefaultOIDFunction(0, "CertDllOpenStoreProv",
     L"bogus.dll");
    ok(ret, "CryptUnregisterDefaultOIDFunction failed: %08lx\n",
     GetLastError());
    ret = CryptUnregisterDefaultOIDFunction(0, "CertDllOpenStoreProv",
     L"bogus2.dll");
    ok(ret, "CryptUnregisterDefaultOIDFunction failed: %08lx\n",
     GetLastError());
    /* Now that they're both unregistered, unregistering should fail */
    ret = CryptUnregisterDefaultOIDFunction(0, "CertDllOpenStoreProv",
     L"bogus.dll");
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());

    /* Repeat a few tests on the normal encoding type */
    ret = CryptRegisterDefaultOIDFunction(X509_ASN_ENCODING,
     "CertDllOpenStoreProv", 0, L"bogus.dll");
    ok(ret, "CryptRegisterDefaultOIDFunction failed\n");
    ret = CryptUnregisterDefaultOIDFunction(X509_ASN_ENCODING,
     "CertDllOpenStoreProv", L"bogus.dll");
    ok(ret, "CryptUnregisterDefaultOIDFunction failed\n");
    ret = CryptUnregisterDefaultOIDFunction(X509_ASN_ENCODING,
     "CertDllOpenStoreProv", L"bogus.dll");
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
}

static void test_getDefaultOIDFunctionAddress(void)
{
    BOOL ret;
    HCRYPTOIDFUNCSET set;
    void *funcAddr;
    HCRYPTOIDFUNCADDR hFuncAddr;

    /* Crash
    ret = CryptGetDefaultOIDFunctionAddress(0, 0, NULL, 0, NULL, NULL);
    ret = CryptGetDefaultOIDFunctionAddress(0, 0, NULL, 0, &funcAddr, NULL);
    ret = CryptGetDefaultOIDFunctionAddress(0, 0, NULL, 0, NULL, &hFuncAddr);
    ret = CryptGetDefaultOIDFunctionAddress(0, 0, NULL, 0, &funcAddr,
     &hFuncAddr);
     */
    set = CryptInitOIDFunctionSet("CertDllOpenStoreProv", 0);
    ok(set != 0, "CryptInitOIDFunctionSet failed: %ld\n", GetLastError());
    /* This crashes if hFuncAddr is not 0 to begin with */
    hFuncAddr = 0;
    ret = CryptGetDefaultOIDFunctionAddress(set, 0, NULL, 0, &funcAddr,
     &hFuncAddr);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());
    /* This fails with the normal encoding too, so built-in functions aren't
     * returned.
     */
    ret = CryptGetDefaultOIDFunctionAddress(set, X509_ASN_ENCODING, NULL, 0,
     &funcAddr, &hFuncAddr);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());

    /* Even with a registered dll, this fails (since the dll doesn't exist) */
    SetLastError(0xdeadbeef);
    ret = CryptRegisterDefaultOIDFunction(0, "CertDllOpenStoreProv", 0,
     L"bogus.dll");
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
        skip("Need admin rights\n");
    else
        ok(ret, "CryptRegisterDefaultOIDFunction failed: %08lx\n", GetLastError());
    ret = CryptGetDefaultOIDFunctionAddress(set, 0, NULL, 0, &funcAddr,
     &hFuncAddr);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());
    CryptUnregisterDefaultOIDFunction(0, "CertDllOpenStoreProv", L"bogus.dll");
}

static BOOL WINAPI countOidInfo(PCCRYPT_OID_INFO pInfo, void *pvArg)
{
    (*(DWORD *)pvArg)++;
    return TRUE;
}

static BOOL WINAPI noOidInfo(PCCRYPT_OID_INFO pInfo, void *pvArg)
{
    return FALSE;
}

static void test_enumOIDInfo(void)
{
    BOOL ret;
    DWORD count = 0;

    /* This crashes
    ret = CryptEnumOIDInfo(7, 0, NULL, NULL);
     */

    /* Silly tests, check that more than one thing is enumerated */
    ret = CryptEnumOIDInfo(0, 0, &count, countOidInfo);
    ok(ret && count > 0, "Expected more than item enumerated\n");
    ret = CryptEnumOIDInfo(0, 0, NULL, noOidInfo);
    ok(!ret, "Expected FALSE\n");
}

static void test_findOIDInfo(void)
{
    static CHAR oid_rsa_md5[] = szOID_RSA_MD5, oid_sha256[] = szOID_NIST_sha256;
    static CHAR oid_ecdsa_sha256[] = szOID_ECDSA_SHA256;
    static CHAR oid_ecc_public_key[] = szOID_ECC_PUBLIC_KEY;
    ALG_ID alg = CALG_SHA1;
    ALG_ID algs[2] = { CALG_MD5, CALG_RSA_SIGN };
    const struct oid_info
    {
        DWORD key_type;
        void *key;
        const char *oid;
        ALG_ID algid;
        ALG_ID broken_algid;
    } oid_test_info [] =
    {
        { CRYPT_OID_INFO_OID_KEY, oid_rsa_md5, szOID_RSA_MD5, CALG_MD5 },
        { CRYPT_OID_INFO_NAME_KEY, (void *)L"sha1", szOID_OIWSEC_sha1, CALG_SHA1 },
        { CRYPT_OID_INFO_ALGID_KEY, &alg, szOID_OIWSEC_sha1, CALG_SHA1 },
        { CRYPT_OID_INFO_SIGN_KEY, algs, szOID_RSA_MD5RSA, CALG_MD5 },
        { CRYPT_OID_INFO_OID_KEY, oid_sha256, szOID_NIST_sha256, CALG_SHA_256, -1 },
    };
    PCCRYPT_OID_INFO info;
    int i;

    info = CryptFindOIDInfo(0, NULL, 0);
    ok(info == NULL, "Expected NULL\n");

    for (i = 0; i < ARRAY_SIZE(oid_test_info); i++)
    {
        const struct oid_info *test = &oid_test_info[i];

        info = CryptFindOIDInfo(test->key_type, test->key, 0);
        ok(info != NULL, "Failed to find %s.\n", test->oid);
        if (info)
        {
            ok(!strcmp(info->pszOID, test->oid), "Unexpected OID %s, expected %s\n", info->pszOID, test->oid);
            ok(info->Algid == test->algid || broken(info->Algid == test->broken_algid),
                "Unexpected Algid %d, expected %d\n", info->Algid, test->algid);
        }
    }

    info = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, oid_ecdsa_sha256, 0);
    if (info)
    {
        DWORD *data;

        ok(info->cbSize == sizeof(*info), "Unexpected structure size %ld.\n", info->cbSize);
        ok(!strcmp(info->pszOID, oid_ecdsa_sha256), "Expected %s, got %s\n", oid_ecdsa_sha256, info->pszOID);
        ok(!lstrcmpW(info->pwszName, L"sha256ECDSA"), "Expected %s, got %s\n",
            wine_dbgstr_w(L"sha256ECDSA"), wine_dbgstr_w(info->pwszName));
        ok(info->dwGroupId == CRYPT_SIGN_ALG_OID_GROUP_ID,
           "Expected CRYPT_SIGN_ALG_OID_GROUP_ID, got %lu\n", info->dwGroupId);
        ok(info->Algid == CALG_OID_INFO_CNG_ONLY,
           "Expected CALG_OID_INFO_CNG_ONLY, got %d\n", info->Algid);

        data = (DWORD *)info->ExtraInfo.pbData;
        ok(info->ExtraInfo.cbData == 8, "Expected 8, got %ld\n", info->ExtraInfo.cbData);
        ok(data[0] == CALG_OID_INFO_PARAMETERS, "Expected CALG_OID_INFO_PARAMETERS, got %lx\n", data[0]);
        ok(data[1] == CRYPT_OID_NO_NULL_ALGORITHM_PARA_FLAG,
            "Expected CRYPT_OID_NO_NULL_ALGORITHM_PARA_FLAG, got %lx\n", data[1]);

        ok(!lstrcmpW(info->pwszCNGAlgid, BCRYPT_SHA256_ALGORITHM), "Expected %s, got %s\n",
           wine_dbgstr_w(BCRYPT_SHA256_ALGORITHM), wine_dbgstr_w(info->pwszCNGAlgid));
        ok(!lstrcmpW(info->pwszCNGExtraAlgid, CRYPT_OID_INFO_ECC_PARAMETERS_ALGORITHM), "Expected %s, got %s\n",
           wine_dbgstr_w(CRYPT_OID_INFO_ECC_PARAMETERS_ALGORITHM), wine_dbgstr_w(info->pwszCNGExtraAlgid));
    }
    else
        win_skip("Host does not support ECDSA_SHA256, skipping test\n");

    info = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, oid_ecc_public_key, 0);
    ok(!!info, "got error %#lx.\n", GetLastError());
    ok(!strcmp(info->pszOID, oid_ecc_public_key), "got %s.\n", info->pszOID);
    ok(!wcscmp(info->pwszName, L"ECC"), "got %s.\n", wine_dbgstr_w(info->pwszName));
    ok(info->dwGroupId == CRYPT_PUBKEY_ALG_OID_GROUP_ID, "got %lu.\n", info->dwGroupId);
    ok(info->Algid == CALG_OID_INFO_PARAMETERS, "got %d.\n", info->Algid);
    ok(!info->ExtraInfo.cbData, "got %ld.\n", info->ExtraInfo.cbData);
    ok(!wcscmp(info->pwszCNGAlgid, CRYPT_OID_INFO_ECC_PARAMETERS_ALGORITHM), "got %s.\n", wine_dbgstr_w(info->pwszCNGAlgid));
    ok(info->pwszCNGExtraAlgid && !wcscmp(info->pwszCNGExtraAlgid, L""), "got %s.\n",
        wine_dbgstr_w(info->pwszCNGExtraAlgid));
}

static void test_registerOIDInfo(void)
{
    static char test_oid[] = "1.2.3.4.5.6.7.8.9.10";
    CRYPT_OID_INFO info1;
    const CRYPT_OID_INFO *info2;
    HKEY key;
    DWORD ret, size, type, value;
    char buf[256];

    SetLastError(0xdeadbeef);
    ret = CryptUnregisterOIDInfo(NULL);
    ok(!ret, "should fail\n");
    ok(GetLastError() == E_INVALIDARG, "got %#lx\n", GetLastError());

    memset(&info1, 0, sizeof(info1));
    SetLastError(0xdeadbeef);
    ret = CryptUnregisterOIDInfo(&info1);
    ok(!ret, "should fail\n");
    ok(GetLastError() == E_INVALIDARG, "got %#lx\n", GetLastError());

    info1.cbSize = sizeof(info1);
    SetLastError(0xdeadbeef);
    ret = CryptUnregisterOIDInfo(&info1);
    ok(!ret, "should fail\n");
    ok(GetLastError() == E_INVALIDARG, "got %#lx\n", GetLastError());

    info1.pszOID = test_oid;
    SetLastError(0xdeadbeef);
    ret = CryptUnregisterOIDInfo(&info1);
    ok(!ret, "should fail\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND ||
       GetLastError() == ERROR_ACCESS_DENIED, "got %lu\n", GetLastError());

    info2 = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, (void *)test_oid, 0);
    ok(!info2, "should fail\n");

    SetLastError(0xdeadbeef);
    /* While it succeeds, the next call does not write anything to the
     * registry on Windows because dwGroupId == 0.
     */
    ret = CryptRegisterOIDInfo(&info1, 0);
    ok(ret, "got %lu\n", GetLastError());

    ret = RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Cryptography\\OID\\EncodingType 0\\CryptDllFindOIDInfo\\1.2.3.4.5.6.7.8.9.10!1", &key);
    ok(ret == ERROR_FILE_NOT_FOUND, "got %lu\n", ret);

    info2 = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, (void *)test_oid, 0);
    ok(!info2, "should fail\n");

    info1.pwszName = L"winetest";
    info1.dwGroupId = CRYPT_HASH_ALG_OID_GROUP_ID;
    SetLastError(0xdeadbeef);
    ret = CryptRegisterOIDInfo(&info1, CRYPT_INSTALL_OID_INFO_BEFORE_FLAG);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Need admin rights\n");
        return;
    }
    ok(ret, "got %lu\n", GetLastError());

    /* It looks like crypt32 reads the OID info from registry only on load,
     * and CryptFindOIDInfo will find the registered OID on next run
     */
    info2 = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, (void *)test_oid, 0);
    ok(!info2, "should fail\n");

    ret = RegCreateKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Cryptography\\OID\\EncodingType 0\\CryptDllFindOIDInfo\\1.2.3.4.5.6.7.8.9.10!1", &key);
    ok(!ret, "got %lu\n", ret);

    memset(buf, 0, sizeof(buf));
    size = sizeof(buf);
    ret = RegQueryValueExA(key, "Name", NULL, &type, (BYTE *)buf, &size);
    ok(!ret, "got %lu\n", ret);
    ok(type == REG_SZ, "got %lu\n", type);
    ok(!strcmp(buf, "winetest"), "got %s\n", buf);

    value = 0xdeadbeef;
    size = sizeof(value);
    ret = RegQueryValueExA(key, "Flags", NULL, &type, (BYTE *)&value, &size);
    ok(!ret, "got %lu\n", ret);
    ok(type == REG_DWORD, "got %lu\n", type);
    ok(value == 1, "got %lu\n", value);

    RegCloseKey(key);

    CryptUnregisterOIDInfo(&info1);

    ret = RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Cryptography\\OID\\EncodingType 0\\CryptDllFindOIDInfo\\1.2.3.4.5.6.7.8.9.10!1", &key);
    ok(ret == ERROR_FILE_NOT_FOUND, "got %lu\n", ret);
}

START_TEST(oid)
{
    test_OIDToAlgID();
    test_AlgIDToOID();
    test_enumOIDInfo();
    test_findOIDInfo();
    test_registerOIDInfo();
    test_oidFunctionSet();
    test_installOIDFunctionAddress();
    test_registerOIDFunction();
    test_registerDefaultOIDFunction();
    test_getDefaultOIDFunctionAddress();
}
