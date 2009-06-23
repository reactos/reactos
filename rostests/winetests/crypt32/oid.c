/*
 * Unit test suite for crypt32.dll's OID support functions.
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


static BOOL (WINAPI *pCryptEnumOIDInfo)(DWORD,DWORD,void*,PFN_CRYPT_ENUM_OID_INFO);


struct OIDToAlgID
{
    LPCSTR oid;
    LPCSTR altOid;
    DWORD algID;
};

static const struct OIDToAlgID oidToAlgID[] = {
 { szOID_RSA_RSA, NULL, CALG_RSA_KEYX },
 { szOID_RSA_MD2RSA, NULL, CALG_MD2 },
 { szOID_RSA_MD4RSA, NULL, CALG_MD4 },
 { szOID_RSA_MD5RSA, NULL, CALG_MD5 },
 { szOID_RSA_SHA1RSA, NULL, CALG_SHA },
 { szOID_RSA_DH, NULL, CALG_DH_SF },
 { szOID_RSA_SMIMEalgESDH, NULL, CALG_DH_EPHEM },
 { szOID_RSA_SMIMEalgCMS3DESwrap, NULL, CALG_3DES },
 { szOID_RSA_SMIMEalgCMSRC2wrap, NULL, CALG_RC2 },
 { szOID_RSA_MD2, NULL, CALG_MD2 },
 { szOID_RSA_MD4, NULL, CALG_MD4 },
 { szOID_RSA_MD5, NULL, CALG_MD5 },
 { szOID_RSA_RC2CBC, NULL, CALG_RC2 },
 { szOID_RSA_RC4, NULL, CALG_RC4 },
 { szOID_RSA_DES_EDE3_CBC, NULL, CALG_3DES },
 { szOID_ANSI_X942_DH, NULL, CALG_DH_SF },
 { szOID_X957_DSA, NULL, CALG_DSS_SIGN },
 { szOID_X957_SHA1DSA, NULL, CALG_SHA },
 { szOID_OIWSEC_md4RSA, NULL, CALG_MD4 },
 { szOID_OIWSEC_md5RSA, NULL, CALG_MD5 },
 { szOID_OIWSEC_md4RSA2, NULL, CALG_MD4 },
 { szOID_OIWSEC_desCBC, NULL, CALG_DES },
 { szOID_OIWSEC_dsa, NULL, CALG_DSS_SIGN },
 { szOID_OIWSEC_shaDSA, NULL, CALG_SHA },
 { szOID_OIWSEC_shaRSA, NULL, CALG_SHA },
 { szOID_OIWSEC_sha, NULL, CALG_SHA },
 { szOID_OIWSEC_rsaXchg, NULL, CALG_RSA_KEYX },
 { szOID_OIWSEC_sha1, NULL, CALG_SHA },
 { szOID_OIWSEC_dsaSHA1, NULL, CALG_SHA },
 { szOID_OIWSEC_sha1RSASign, NULL, CALG_SHA },
 { szOID_OIWDIR_md2RSA, NULL, CALG_MD2 },
 { szOID_INFOSEC_mosaicUpdatedSig, NULL, CALG_SHA },
 { szOID_INFOSEC_mosaicKMandUpdSig, NULL, CALG_DSS_SIGN },
};

static const struct OIDToAlgID algIDToOID[] = {
 { szOID_RSA_RSA, NULL, CALG_RSA_KEYX },
 { szOID_RSA_SMIMEalgESDH, NULL, CALG_DH_EPHEM },
 { szOID_RSA_MD2, NULL, CALG_MD2 },
 { szOID_RSA_MD4, NULL, CALG_MD4 },
 { szOID_RSA_MD5, NULL, CALG_MD5 },
 { szOID_RSA_RC2CBC, NULL, CALG_RC2 },
 { szOID_RSA_RC4, NULL, CALG_RC4 },
 { szOID_RSA_DES_EDE3_CBC, NULL, CALG_3DES },
 { szOID_ANSI_X942_DH, NULL, CALG_DH_SF },
 { szOID_X957_DSA, szOID_OIWSEC_dsa /* some Win98 */, CALG_DSS_SIGN },
 { szOID_OIWSEC_desCBC, NULL, CALG_DES },
 { szOID_OIWSEC_sha1, NULL, CALG_SHA },
};

static const WCHAR bogusDll[] = { 'b','o','g','u','s','.','d','l','l',0 };
static const WCHAR bogus2Dll[] = { 'b','o','g','u','s','2','.','d','l','l',0 };

static void testOIDToAlgID(void)
{
    int i;
    DWORD alg;

    /* Test with a bogus one */
    SetLastError(0xdeadbeef);
    alg = CertOIDToAlgId("1.2.3");
    ok(!alg, "Expected failure, got %d\n", alg);
    ok(GetLastError() == 0xdeadbeef ||
       GetLastError() == ERROR_RESOURCE_NAME_NOT_FOUND ||
       GetLastError() == ERROR_INVALID_PARAMETER || /* Vista */
       GetLastError() == ERROR_SUCCESS || /* win2k */
       GetLastError() == ERROR_FILE_INVALID, /* another Vista */
       "Expected ERROR_RESOURCE_NAME_NOT_FOUND, ERROR_INVALID_PARAMETER, "
       "ERROR_SUCCESS or no error set, got %08x\n", GetLastError());

    for (i = 0; i < sizeof(oidToAlgID) / sizeof(oidToAlgID[0]); i++)
    {
        alg = CertOIDToAlgId(oidToAlgID[i].oid);
        /* Not all Windows installations support all these, so make sure it's
         * at least not the wrong one.
         */
        ok(alg == 0 || alg == oidToAlgID[i].algID,
         "Expected %d, got %d\n", oidToAlgID[i].algID, alg);
    }
}

static void testAlgIDToOID(void)
{
    int i;
    LPCSTR oid;

    /* Test with a bogus one */
    SetLastError(0xdeadbeef);
    oid = CertAlgIdToOID(ALG_CLASS_SIGNATURE | ALG_TYPE_ANY | 80);
    ok(!oid && GetLastError() == 0xdeadbeef,
     "Didn't expect last error (%08x) to be set\n", GetLastError());
    for (i = 0; i < sizeof(algIDToOID) / sizeof(algIDToOID[0]); i++)
    {
        oid = CertAlgIdToOID(algIDToOID[i].algID);
        /* Allow failure, not every version of Windows supports every algo */
        if (oid)
        {
            if (strcmp(oid, algIDToOID[i].oid))
            {
                if (algIDToOID[i].altOid)
                    ok(!strcmp(oid, algIDToOID[i].altOid),
                     "Expected %s or %s, got %s\n", algIDToOID[i].oid,
                     algIDToOID[i].altOid, oid);
                else
                {
                    /* No need to rerun the test, we already know it failed. */
                    ok(0, "Expected %s, got %s\n", algIDToOID[i].oid, oid);
                }
            }
            else
            {
                /* No need to rerun the test, we already know it succeeded. */
                ok(1, "Expected %s, got %s\n", algIDToOID[i].oid, oid);
            }
        }
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
    ok(set1 != 0, "CryptInitOIDFunctionSet failed: %08x\n", GetLastError());
    if (set1)
    {
        /* These crash
        ret = CryptGetDefaultOIDDllList(NULL, 0, NULL, NULL);
        ret = CryptGetDefaultOIDDllList(NULL, 0, NULL, &size);
         */
        size = 0;
        ret = CryptGetDefaultOIDDllList(set1, 0, NULL, &size);
        ok(ret, "CryptGetDefaultOIDDllList failed: %08x\n", GetLastError());
        if (ret)
        {
            buf = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
            if (buf)
            {
                ret = CryptGetDefaultOIDDllList(set1, 0, buf, &size);
                ok(ret, "CryptGetDefaultOIDDllList failed: %08x\n",
                 GetLastError());
                ok(!*buf, "Expected empty DLL list\n");
                HeapFree(GetProcessHeap(), 0, buf);
            }
        }
    }

    /* MSDN says flags must be 0, but it's not checked */
    set1 = CryptInitOIDFunctionSet("", 1);
    ok(set1 != 0, "CryptInitOIDFunctionSet failed: %08x\n", GetLastError());
    set2 = CryptInitOIDFunctionSet("", 0);
    ok(set2 != 0, "CryptInitOIDFunctionSet failed: %08x\n", GetLastError());
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
    ok(set1 != 0, "CryptInitOIDFunctionSet failed: %08x\n", GetLastError());
    if (set1)
    {
        void *funcAddr;
        HCRYPTOIDFUNCADDR hFuncAddr;

        ret = CryptGetOIDFunctionAddress(set1, X509_ASN_ENCODING, X509_CERT, 0,
         &funcAddr, &hFuncAddr);
        ok((!ret && GetLastError() == ERROR_FILE_NOT_FOUND) ||
         broken(ret) /* some Win98 */,
         "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
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
    ok(ret && GetLastError() == 0xdeadbeef, "Expected success, got %08x\n",
     GetLastError());

    /* The function name doesn't much matter */
    SetLastError(0xdeadbeef);
    ret = CryptInstallOIDFunctionAddress(NULL, 0, "OhSoFunky", 0, NULL, 0);
    ok(ret && GetLastError() == 0xdeadbeef, "Expected success, got %08x\n",
     GetLastError());
    SetLastError(0xdeadbeef);
    entry.pszOID = X509_CERT;
    ret = CryptInstallOIDFunctionAddress(NULL, 0, "OhSoFunky", 1, &entry, 0);
    ok(ret && GetLastError() == 0xdeadbeef, "Expected success, got %08x\n",
     GetLastError());
    set = CryptInitOIDFunctionSet("OhSoFunky", 0);
    ok(set != 0, "CryptInitOIDFunctionSet failed: %08x\n", GetLastError());
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
        ok(!ret && (GetLastError() == ERROR_FILE_NOT_FOUND ||
         GetLastError() == E_INVALIDARG /* some Win98 */),
         "Expected ERROR_FILE_NOT_FOUND or E_INVALIDARG, got %d\n",
         GetLastError());
        ret = CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING, X509_CERT, 0,
         (void **)&funcAddr, &hFuncAddr);
        ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
         "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
        ret = CryptGetOIDFunctionAddress(set, 0, X509_CERT, 0,
         (void **)&funcAddr, &hFuncAddr);
        ok(ret, "CryptGetOIDFunctionAddress failed: %d\n", GetLastError());
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
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
     */
    /* On windows XP, GetLastError is incorrectly being set with an HRESULT,
     * E_INVALIDARG
     */
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, "foo", NULL, bogusDll,
     NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG: %d\n", GetLastError());
    /* This has no effect, but "succeeds" on XP */
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, "foo",
     "1.2.3.4.5.6.7.8.9.10", NULL, NULL);
    ok(ret, "Expected pseudo-success, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Need admin rights\n");
        return;
    }
    ok(ret, "CryptRegisterOIDFunction failed: %d\n", GetLastError());
    ret = CryptUnregisterOIDFunction(X509_ASN_ENCODING, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10");
    ok(ret, "CryptUnregisterOIDFunction failed: %d\n", GetLastError());
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, "bogus",
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %d\n", GetLastError());
    ret = CryptUnregisterOIDFunction(X509_ASN_ENCODING, "bogus",
     "1.2.3.4.5.6.7.8.9.10");
    ok(ret, "CryptUnregisterOIDFunction failed: %d\n", GetLastError());
    /* Unwanted Cryptography\OID\EncodingType 1\bogus\ will still be there */
    ok(!RegDeleteKeyA(HKEY_LOCAL_MACHINE,
     "SOFTWARE\\Microsoft\\Cryptography\\OID\\EncodingType 1\\bogus"),
     "Could not delete bogus key\n");
    /* Shouldn't have effect but registry keys are created */
    ret = CryptRegisterOIDFunction(PKCS_7_ASN_ENCODING, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %d\n", GetLastError());
    ret = CryptUnregisterOIDFunction(PKCS_7_ASN_ENCODING, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10");
    ok(ret, "CryptUnregisterOIDFunction failed: %d\n", GetLastError());
    /* Check with bogus encoding type. Registry keys are still created */
    ret = CryptRegisterOIDFunction(0, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %d\n", GetLastError());
    ret = CryptUnregisterOIDFunction(0, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10");
    ok(ret, "CryptUnregisterOIDFunction failed: %d\n", GetLastError());
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
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %d\n", GetLastError());
    ret = CryptUnregisterOIDFunction(3, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10");
    ok(ret, "CryptUnregisterOIDFunction failed: %d\n", GetLastError());
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
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* This succeeds on WinXP, although the bogus entry is unusable.
    ret = CryptRegisterDefaultOIDFunction(0, NULL, 0, bogusDll);
     */
    /* Register one at index 0 */
    SetLastError(0xdeadbeef);
    ret = CryptRegisterDefaultOIDFunction(0, "CertDllOpenStoreProv", 0,
     bogusDll);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Need admin rights\n");
        return;
    }
    ok(ret, "CryptRegisterDefaultOIDFunction failed: %08x\n", GetLastError());
    /* Reregistering should fail */
    ret = CryptRegisterDefaultOIDFunction(0, "CertDllOpenStoreProv", 0,
     bogusDll);
    ok(!ret && GetLastError() == ERROR_FILE_EXISTS,
     "Expected ERROR_FILE_EXISTS, got %08x\n", GetLastError());
    /* Registering the same one at index 1 should also fail */
    ret = CryptRegisterDefaultOIDFunction(0, "CertDllOpenStoreProv", 1,
     bogusDll);
    ok(!ret && GetLastError() == ERROR_FILE_EXISTS,
     "Expected ERROR_FILE_EXISTS, got %08x\n", GetLastError());
    /* Registering a different one at index 1 succeeds */
    ret = CryptRegisterDefaultOIDFunction(0, "CertDllOpenStoreProv", 1,
     bogus2Dll);
    ok(ret, "CryptRegisterDefaultOIDFunction failed: %08x\n", GetLastError());
    sprintf(buf, fmt, 0, func);
    rc = RegOpenKeyA(HKEY_LOCAL_MACHINE, buf, &key);
    ok(rc == 0, "Expected key to exist, RegOpenKeyA failed: %d\n", rc);
    if (rc == 0)
    {
        static const CHAR dllA[] = "Dll";
        static const CHAR bogusDll_A[] = "bogus.dll";
        static const CHAR bogus2Dll_A[] = "bogus2.dll";
        CHAR dllBuf[MAX_PATH];
        DWORD type, size;
        LPSTR ptr;

        size = sizeof(dllBuf) / sizeof(dllBuf[0]);
        rc = RegQueryValueExA(key, dllA, NULL, &type, (LPBYTE)dllBuf, &size);
        ok(rc == 0,
         "Expected Dll value to exist, RegQueryValueExA failed: %d\n", rc);
        ok(type == REG_MULTI_SZ, "Expected type REG_MULTI_SZ, got %d\n", type);
        /* bogusDll was registered first, so that should be first */
        ptr = dllBuf;
        ok(!lstrcmpiA(ptr, bogusDll_A), "Unexpected dll\n");
        ptr += lstrlenA(ptr) + 1;
        ok(!lstrcmpiA(ptr, bogus2Dll_A), "Unexpected dll\n");
        RegCloseKey(key);
    }
    /* Unregister both of them */
    ret = CryptUnregisterDefaultOIDFunction(0, "CertDllOpenStoreProv",
     bogusDll);
    ok(ret, "CryptUnregisterDefaultOIDFunction failed: %08x\n",
     GetLastError());
    ret = CryptUnregisterDefaultOIDFunction(0, "CertDllOpenStoreProv",
     bogus2Dll);
    ok(ret, "CryptUnregisterDefaultOIDFunction failed: %08x\n",
     GetLastError());
    /* Now that they're both unregistered, unregistering should fail */
    ret = CryptUnregisterDefaultOIDFunction(0, "CertDllOpenStoreProv",
     bogusDll);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());

    /* Repeat a few tests on the normal encoding type */
    ret = CryptRegisterDefaultOIDFunction(X509_ASN_ENCODING,
     "CertDllOpenStoreProv", 0, bogusDll);
    ret = CryptUnregisterDefaultOIDFunction(X509_ASN_ENCODING,
     "CertDllOpenStoreProv", bogusDll);
    ok(ret, "CryptUnregisterDefaultOIDFunction failed\n");
    ret = CryptUnregisterDefaultOIDFunction(X509_ASN_ENCODING,
     "CertDllOpenStoreProv", bogusDll);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
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
    ok(set != 0, "CryptInitOIDFunctionSet failed: %d\n", GetLastError());
    /* This crashes if hFuncAddr is not 0 to begin with */
    hFuncAddr = 0;
    ret = CryptGetDefaultOIDFunctionAddress(set, 0, NULL, 0, &funcAddr,
     &hFuncAddr);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    /* This fails with the normal encoding too, so built-in functions aren't
     * returned.
     */
    ret = CryptGetDefaultOIDFunctionAddress(set, X509_ASN_ENCODING, NULL, 0,
     &funcAddr, &hFuncAddr);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());

    /* Even with a registered dll, this fails (since the dll doesn't exist) */
    SetLastError(0xdeadbeef);
    ret = CryptRegisterDefaultOIDFunction(0, "CertDllOpenStoreProv", 0,
     bogusDll);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
        skip("Need admin rights\n");
    else
        ok(ret, "CryptRegisterDefaultOIDFunction failed: %08x\n", GetLastError());
    ret = CryptGetDefaultOIDFunctionAddress(set, 0, NULL, 0, &funcAddr,
     &hFuncAddr);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    CryptUnregisterDefaultOIDFunction(0, "CertDllOpenStoreProv", bogusDll);
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

    if (!pCryptEnumOIDInfo)
    {
        win_skip("CryptEnumOIDInfo() is not available\n");
        return;
    }

    /* This crashes
    ret = pCryptEnumOIDInfo(7, 0, NULL, NULL);
     */

    /* Silly tests, check that more than one thing is enumerated */
    ret = pCryptEnumOIDInfo(0, 0, &count, countOidInfo);
    ok(ret && count > 0, "Expected more than item enumerated\n");
    ret = pCryptEnumOIDInfo(0, 0, NULL, noOidInfo);
    ok(!ret, "Expected FALSE\n");
}

static void test_findOIDInfo(void)
{
    static WCHAR sha1[] = { 's','h','a','1',0 };
    static CHAR oid_rsa_md5[] = szOID_RSA_MD5;
    ALG_ID alg = CALG_SHA1;
    ALG_ID algs[2] = { CALG_MD5, CALG_RSA_SIGN };
    PCCRYPT_OID_INFO info;

    info = CryptFindOIDInfo(0, NULL, 0);
    ok(info == NULL, "Expected NULL\n");
    info = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, oid_rsa_md5, 0);
    ok(info != NULL, "Expected to find szOID_RSA_MD5\n");
    if (info)
    {
        ok(!strcmp(info->pszOID, szOID_RSA_MD5), "Expected %s, got %s\n",
         szOID_RSA_MD5, info->pszOID);
        ok(U(*info).Algid == CALG_MD5, "Expected CALG_MD5, got %d\n",
           U(*info).Algid);
    }
    info = CryptFindOIDInfo(CRYPT_OID_INFO_NAME_KEY, sha1, 0);
    ok(info != NULL, "Expected to find sha1\n");
    if (info)
    {
        ok(!strcmp(info->pszOID, szOID_OIWSEC_sha1), "Expected %s, got %s\n",
         szOID_OIWSEC_sha1, info->pszOID);
        ok(U(*info).Algid == CALG_SHA1, "Expected CALG_SHA1, got %d\n",
           U(*info).Algid);
    }
    info = CryptFindOIDInfo(CRYPT_OID_INFO_ALGID_KEY, &alg, 0);
    ok(info != NULL, "Expected to find sha1\n");
    if (info)
    {
        ok(!strcmp(info->pszOID, szOID_OIWSEC_sha1), "Expected %s, got %s\n",
         szOID_OIWSEC_sha1, info->pszOID);
        ok(U(*info).Algid == CALG_SHA1, "Expected CALG_SHA1, got %d\n",
           U(*info).Algid);
    }
    info = CryptFindOIDInfo(CRYPT_OID_INFO_SIGN_KEY, algs, 0);
    ok(info != NULL, "Expected to find md5RSA\n");
    if (info)
    {
        ok(!strcmp(info->pszOID, szOID_RSA_MD5RSA), "Expected %s, got %s\n",
         szOID_RSA_MD5RSA, info->pszOID);
        ok(U(*info).Algid == CALG_MD5, "Expected CALG_MD5, got %d\n",
           U(*info).Algid);
    }
}

START_TEST(oid)
{
    HMODULE hCrypt32 = GetModuleHandleA("crypt32.dll");
    pCryptEnumOIDInfo = (void*)GetProcAddress(hCrypt32, "CryptEnumOIDInfo");

    testOIDToAlgID();
    testAlgIDToOID();
    test_enumOIDInfo();
    test_findOIDInfo();
    test_oidFunctionSet();
    test_installOIDFunctionAddress();
    test_registerOIDFunction();
    test_registerDefaultOIDFunction();
    test_getDefaultOIDFunctionAddress();
}
