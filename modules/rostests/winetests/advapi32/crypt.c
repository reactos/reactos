/*
 * Unit tests for crypt functions
 *
 * Copyright (c) 2004 Michael Jung
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

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winerror.h"
#include "winreg.h"

#include "wine/test.h"

static const char szRsaBaseProv[] = MS_DEF_PROV_A;
static const char szNonExistentProv[] = "Wine Nonexistent Cryptographic Provider v11.2";
static const char szKeySet[] = "wine_test_keyset";
static const char szBadKeySet[] = "wine_test_bad_keyset";
#define NON_DEF_PROV_TYPE 999

static BOOL (WINAPI *pCryptEnumProviderTypesA)(DWORD, DWORD*, DWORD, DWORD*, LPSTR, DWORD*);
static BOOL (WINAPI *pCryptEnumProvidersA)(DWORD, DWORD*, DWORD, DWORD*, LPSTR, DWORD*);
static BOOL (WINAPI *pCryptGetDefaultProviderA)(DWORD, DWORD*, DWORD, LPSTR, DWORD*);
static BOOL (WINAPI *pCryptSetProviderExA)(LPCSTR, DWORD, DWORD*, DWORD);
static BOOL (WINAPI *pCryptGenRandom)(HCRYPTPROV, DWORD, BYTE*);
static BOOL (WINAPI *pCryptDuplicateHash)(HCRYPTHASH, DWORD*, DWORD, HCRYPTHASH*);
static BOOL (WINAPI *pCryptHashSessionKey)(HCRYPTHASH, HCRYPTKEY, DWORD);
static BOOL (WINAPI *pCryptSignHashW)(HCRYPTHASH, DWORD, LPCWSTR, DWORD, BYTE*, DWORD*);
static BOOL (WINAPI *pCryptVerifySignatureW)(HCRYPTHASH, BYTE*, DWORD, HCRYPTKEY, LPCWSTR, DWORD);
static BOOLEAN (WINAPI *pSystemFunction036)(PVOID, ULONG);

static void init_function_pointers(void)
{
    HMODULE hadvapi32 = GetModuleHandleA("advapi32.dll");

    pCryptEnumProviderTypesA = (void*)GetProcAddress(hadvapi32, "CryptEnumProviderTypesA");
    pCryptEnumProvidersA = (void*)GetProcAddress(hadvapi32, "CryptEnumProvidersA");
    pCryptGetDefaultProviderA = (void*)GetProcAddress(hadvapi32, "CryptGetDefaultProviderA");
    pCryptSetProviderExA = (void*)GetProcAddress(hadvapi32, "CryptSetProviderExA");
    pCryptGenRandom = (void*)GetProcAddress(hadvapi32, "CryptGenRandom");
    pCryptDuplicateHash = (void*)GetProcAddress(hadvapi32, "CryptDuplicateHash");
    pCryptHashSessionKey = (void*)GetProcAddress(hadvapi32, "CryptHashSessionKey");
    pCryptSignHashW = (void*)GetProcAddress(hadvapi32, "CryptSignHashW");
    pCryptVerifySignatureW = (void*)GetProcAddress(hadvapi32, "CryptVerifySignatureW");
    pSystemFunction036 = (void*)GetProcAddress(hadvapi32, "SystemFunction036");
}

static void init_environment(void)
{
    HCRYPTPROV hProv;
    BOOL ret;

    /* Ensure that container "wine_test_keyset" does exist */
    if (!CryptAcquireContextA(&hProv, szKeySet, szRsaBaseProv, PROV_RSA_FULL, 0))
    {
        CryptAcquireContextA(&hProv, szKeySet, szRsaBaseProv, PROV_RSA_FULL, CRYPT_NEWKEYSET);
    }
    ret = CryptReleaseContext(hProv, 0);
    ok(ret, "got %lu\n", GetLastError());

    /* Ensure that container "wine_test_keyset" does exist in default PROV_RSA_FULL type provider */
    if (!CryptAcquireContextA(&hProv, szKeySet, NULL, PROV_RSA_FULL, 0))
    {
        CryptAcquireContextA(&hProv, szKeySet, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET);
    }
    ret = CryptReleaseContext(hProv, 0);
    ok(ret, "got %lu\n", GetLastError());

    /* Ensure that container "wine_test_bad_keyset" does not exist. */
    if (CryptAcquireContextA(&hProv, szBadKeySet, szRsaBaseProv, PROV_RSA_FULL, 0))
    {
        ret = CryptReleaseContext(hProv, 0);
        ok(ret, "got %lu\n", GetLastError());

        CryptAcquireContextA(&hProv, szBadKeySet, szRsaBaseProv, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
    }
}

static void clean_up_environment(void)
{
    HCRYPTPROV hProv;
    BOOL ret;

    /* Remove container "wine_test_keyset" */
    if (CryptAcquireContextA(&hProv, szKeySet, szRsaBaseProv, PROV_RSA_FULL, 0))
    {
        ret = CryptReleaseContext(hProv, 0);
        ok(ret, "got %lu\n", GetLastError());

        CryptAcquireContextA(&hProv, szKeySet, szRsaBaseProv, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
    }

    /* Remove container "wine_test_keyset" from default PROV_RSA_FULL type provider */
    if (CryptAcquireContextA(&hProv, szKeySet, NULL, PROV_RSA_FULL, 0))
    {
        ret = CryptReleaseContext(hProv, 0);
        ok(ret, "got %lu\n", GetLastError());

        CryptAcquireContextA(&hProv, szKeySet, NULL, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
    }

    /* Remove container "wine_test_bad_keyset" */
    if (CryptAcquireContextA(&hProv, szBadKeySet, szRsaBaseProv, PROV_RSA_FULL, 0))
    {
        ret = CryptReleaseContext(hProv, 0);
        ok(ret, "got %lu\n", GetLastError());

        CryptAcquireContextA(&hProv, szBadKeySet, szRsaBaseProv, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
    }
}

static void test_CryptReleaseContext(void)
{
    BOOL ret;
    HCRYPTPROV prov;

    /* TODO: Add cases for ERROR_BUSY, ERROR_INVALID_HANDLE and NTE_BAD_UID */

    /* NULL provider */

    SetLastError(0xdeadbeef);
    ret = CryptReleaseContext(0, 0);
    ok(!ret, "CryptReleaseContext succeeded unexpectedly\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = CryptReleaseContext(0, ~0);
    ok(!ret, "CryptReleaseContext succeeded unexpectedly\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu\n", GetLastError());

    /* Additional refcount */

    ret = CryptAcquireContextA(&prov, szKeySet, szRsaBaseProv, PROV_RSA_FULL, 0);
    ok(ret, "got %lu\n", GetLastError());

    ret = CryptContextAddRef(prov, NULL, 0);
    ok(ret, "got %lu\n", GetLastError());

    ret = CryptContextAddRef(0, NULL, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %lu\n", GetLastError());
    ret = CryptContextAddRef(0xdeadbeef, NULL, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %lu\n", GetLastError());

    ret = CryptReleaseContext(prov, 0);
    ok(ret, "got %lu\n", GetLastError());

    /* Nonzero flags, which allow release nonetheless */

    SetLastError(0xdeadbeef);
    ret = CryptReleaseContext(prov, ~0);
    ok(!ret, "CryptReleaseContext succeeded unexpectedly\n");
    ok(GetLastError() == NTE_BAD_FLAGS, "got %lu\n", GetLastError());

    /* Obsolete provider */

    SetLastError(0xdeadbeef);
    ret = CryptReleaseContext(prov, 0);
    ok(!ret, "CryptReleaseContext succeeded unexpectedly\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = CryptReleaseContext(prov, ~0);
    ok(!ret, "CryptReleaseContext succeeded unexpectedly\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu\n", GetLastError());
}

static void test_acquire_context(void)
{
	BOOL result;
	HCRYPTPROV hProv;
	DWORD GLE;

	/* Provoke all kinds of error conditions (which are easy to provoke). 
	 * The order of the error tests seems to match Windows XP's rsaenh.dll CSP,
	 * but since this is likely to change between CSP versions, we don't check
	 * this. Please don't change the order of tests. */
	result = CryptAcquireContextA(&hProv, NULL, NULL, 0, 0);
	ok(!result && GetLastError()==NTE_BAD_PROV_TYPE, "%ld\n", GetLastError());
	
	result = CryptAcquireContextA(&hProv, NULL, NULL, 1000, 0);
	ok(!result && GetLastError()==NTE_BAD_PROV_TYPE, "%ld\n", GetLastError());

	result = CryptAcquireContextA(&hProv, NULL, NULL, NON_DEF_PROV_TYPE, 0);
	ok(!result && GetLastError()==NTE_PROV_TYPE_NOT_DEF, "%ld\n", GetLastError());
	
	result = CryptAcquireContextA(&hProv, szKeySet, szNonExistentProv, PROV_RSA_FULL, 0);
	ok(!result && GetLastError()==NTE_KEYSET_NOT_DEF, "%ld\n", GetLastError());

	result = CryptAcquireContextA(&hProv, szKeySet, szRsaBaseProv, NON_DEF_PROV_TYPE, 0);
	ok(!result && GetLastError()==NTE_PROV_TYPE_NO_MATCH, "%ld\n", GetLastError());
	

if (0)
{
	/* This test fails under Win2k SP4:
	   result = TRUE, GetLastError() == ERROR_INVALID_PARAMETER */
	SetLastError(0xdeadbeef);
	result = CryptAcquireContextA(NULL, szKeySet, szRsaBaseProv, PROV_RSA_FULL, 0);
	ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "%d/%ld\n", result, GetLastError());
}
	
	/* Last not least, try to really acquire a context. */
	hProv = 0;
	SetLastError(0xdeadbeef);
	result = CryptAcquireContextA(&hProv, szKeySet, szRsaBaseProv, PROV_RSA_FULL, 0);
	GLE = GetLastError();
	ok(result && (GLE == ERROR_ENVVAR_NOT_FOUND   || 
		      GLE == ERROR_SUCCESS            || 
		      GLE == ERROR_RING2_STACK_IN_USE || 
		      GLE == NTE_FAIL                 ||
		      GLE == ERROR_NOT_LOGGED_ON), "%d/%ld\n", result, GLE);

	if (hProv)
	{
	    result = CryptReleaseContext(hProv, 0);
	    ok(result, "got %lu\n", GetLastError());
	}

	/* Try again, witch an empty ("\0") szProvider parameter */
	hProv = 0;
	SetLastError(0xdeadbeef);
	result = CryptAcquireContextA(&hProv, szKeySet, "", PROV_RSA_FULL, 0);
	GLE = GetLastError();
	ok(result && (GLE == ERROR_ENVVAR_NOT_FOUND   || 
		      GLE == ERROR_SUCCESS            || 
		      GLE == ERROR_RING2_STACK_IN_USE || 
		      GLE == NTE_FAIL                 ||
		      GLE == ERROR_NOT_LOGGED_ON), "%d/%ld\n", result, GetLastError());

	if (hProv)
	{
	    result = CryptReleaseContext(hProv, 0);
	    ok(result, "got %lu\n", GetLastError());
	}
}

static void test_incorrect_api_usage(void)
{
    BOOL result;
    HCRYPTPROV hProv, hProv2;
    HCRYPTHASH hHash, hHash2;
    HCRYPTKEY hKey, hKey2;
    BYTE temp;
    DWORD dwLen, dwTemp;

    /* This is to document incorrect api usage in the 
     * "Uru - Ages beyond Myst Demo" installer as reported by Paul Vriens.
     *
     * The installer destroys a hash object after having released the context 
     * with which the hash was created. This is not allowed according to MSDN, 
     * since CryptReleaseContext destroys all hash and key objects belonging to 
     * the respective context. However, while wine used to crash, Windows is more 
     * robust here and returns an ERROR_INVALID_PARAMETER code.
     */
    
    result = CryptAcquireContextA(&hProv, szBadKeySet, szRsaBaseProv,
                                   PROV_RSA_FULL, CRYPT_NEWKEYSET);
    ok (result, "%08lx\n", GetLastError());
    if (!result) return;

    /* Looks like native handles are just pointers. */
    ok(!!*(void **)hProv, "Got zero *(void **)hProv.\n");

    result = CryptCreateHash(hProv, CALG_SHA, 0, 0, &hHash);
    ok (result, "%ld\n", GetLastError());
    if (!result) return;

    result = CryptDeriveKey(0, CALG_RC4, hHash, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptDeriveKey(hProv, CALG_RC4, 0, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptHashData(0, &temp, 1, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptGenKey(hProv, CALG_RC4, 0, &hKey);
    ok (result, "%ld\n", GetLastError());
    if (!result) return;

    result = pCryptHashSessionKey(hHash, 0, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptHashSessionKey(0, hKey, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptDestroyHash(hHash);
    ok (result, "%08lx\n", GetLastError());

    result = CryptDestroyHash(0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptCreateHash(0xdeadbeef, CALG_SHA, 0, 0, &hHash);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptCreateHash(0, CALG_SHA, 0, 0, &hHash);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptGenKey(0, CALG_RC4, 0, &hKey);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    dwLen = 0;
    SetLastError(0xdeadbeef);
    result = CryptDecrypt(hKey, 0, FALSE, 0, &temp, &dwLen);
    ok (result, "%lx\n", GetLastError());
    dwLen = 0;
    SetLastError(0xdeadbeef);
    result = CryptDecrypt(hKey, 0, TRUE, 0, &temp, &dwLen);
    ok (!result && GetLastError() == NTE_BAD_LEN, "%lx\n", GetLastError());
    dwLen = 1;
    result = CryptDecrypt(hKey, 0, TRUE, 0, &temp, &dwLen);
    ok (result, "%ld\n", GetLastError());
    result = CryptDecrypt(hKey, 0xdeadbeef, TRUE, 0, &temp, &dwLen);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());
    result = CryptDecrypt(0, 0, TRUE, 0, &temp, &dwLen);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());
    result = CryptDecrypt(0xdeadbeef, 0, TRUE, 0, &temp, &dwLen);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptEncrypt(hKey, 0, TRUE, 0, &temp, &dwLen, sizeof(temp));
    ok (result, "%ld\n", GetLastError());
    result = CryptEncrypt(hKey, 0xdeadbeef, TRUE, 0, &temp, &dwLen, sizeof(temp));
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());
    result = CryptEncrypt(0, 0, TRUE, 0, &temp, &dwLen, sizeof(temp));
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());
    result = CryptEncrypt(0xdeadbeef, 0, TRUE, 0, &temp, &dwLen, sizeof(temp));
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    dwLen = 1;
    result = CryptExportKey(hKey, 0xdeadbeef, 0, 0, &temp, &dwLen);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptDestroyKey(hKey);
    ok (result, "%ld\n", GetLastError());

    result = CryptGenKey(hProv, CALG_RC4, 0, &hKey2);
    ok (result, "%ld\n", GetLastError());
    if (!result) return;

    result = CryptDestroyKey(hKey2);
    ok (result, "%ld\n", GetLastError());

    result = CryptDestroyKey(0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    dwTemp = CRYPT_MODE_ECB;    
    result = CryptSetKeyParam(hKey2, KP_MODE, (BYTE*)&dwTemp, sizeof(DWORD));
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    hProv2 = 0xdeadbeef;
    result = CryptAcquireContextA(&hProv2, szBadKeySet, NULL, PROV_RSA_FULL,
                                   CRYPT_DELETEKEYSET);
    ok (result, "%ld\n", GetLastError());
    ok (hProv2 == 0, "%Id\n", hProv2);
    if (!result) return;

    result = CryptReleaseContext(hProv, 0);
    ok(result, "got %lu\n", GetLastError());
    if (!result) return;

    result = pCryptGenRandom(0, 1, &temp);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptGenRandom(hProv, 1, &temp);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptContextAddRef(hProv, NULL, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptCreateHash(hProv, CALG_SHA, 0, 0, &hHash2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    dwLen = 1;
    result = CryptDecrypt(hKey, 0, TRUE, 0, &temp, &dwLen);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    dwLen = 1;
    result = CryptEncrypt(hKey, 0, TRUE, 0, &temp, &dwLen, 1);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptDeriveKey(hProv, CALG_RC4, hHash, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptDuplicateHash(hHash, NULL, 0, &hHash2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptDuplicateKey(hKey, NULL, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    dwLen = 1;
    result = CryptExportKey(hKey, 0, 0, 0, &temp, &dwLen);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptGenKey(hProv, CALG_RC4, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    dwLen = 1;
    result = CryptGetHashParam(hHash, 0, &temp, &dwLen, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    dwLen = 1;
    result = CryptGetKeyParam(hKey, 0, &temp, &dwLen, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    dwLen = 1;
    result = CryptGetProvParam(hProv, 0, &temp, &dwLen, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptGetUserKey(0, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());
    
    result = CryptGetUserKey(hProv, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptHashData(hHash, &temp, 1, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptHashSessionKey(hHash, hKey, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptImportKey(hProv, &temp, 1, 0, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    if (pCryptSignHashW)
    {
        dwLen = 1;
        result = pCryptSignHashW(hHash, 0, NULL, 0, &temp, &dwLen);
        ok (!result && (GetLastError() == ERROR_INVALID_PARAMETER ||
            GetLastError() == ERROR_CALL_NOT_IMPLEMENTED), "%ld\n", GetLastError());
        result = pCryptSignHashW(hHash, 0, NULL, 0, &temp, &dwLen);
        ok (!result && (GetLastError() == ERROR_INVALID_PARAMETER ||
            GetLastError() == ERROR_CALL_NOT_IMPLEMENTED), "%ld\n", GetLastError());
    }
    else
        win_skip("CryptSignHashW is not available\n");

    result = CryptSetKeyParam(hKey, 0, &temp, 1);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptSetHashParam(hHash, 0, &temp, 1);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptSetProvParam(0, 0, &temp, 1);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = CryptSetProvParam(hProv, 0, &temp, 1);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    if (pCryptVerifySignatureW)
    {
        result = pCryptVerifySignatureW(hHash, &temp, 1, hKey, NULL, 0);
        ok (!result && (GetLastError() == ERROR_INVALID_PARAMETER ||
            GetLastError() == ERROR_CALL_NOT_IMPLEMENTED), "%ld\n", GetLastError());
    }
    else
        win_skip("CryptVerifySignatureW is not available\n");

    result = CryptDestroyHash(hHash);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());
    
    result = CryptDestroyKey(hKey);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());
}

static const BYTE privKey[] = {
 0x07, 0x02, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x52, 0x53, 0x41, 0x32, 0x00,
 0x02, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x79, 0x10, 0x1c, 0xd0, 0x6b, 0x10,
 0x18, 0x30, 0x94, 0x61, 0xdc, 0x0e, 0xcb, 0x96, 0x4e, 0x21, 0x3f, 0x79, 0xcd,
 0xa9, 0x17, 0x62, 0xbc, 0xbb, 0x61, 0x4c, 0xe0, 0x75, 0x38, 0x6c, 0xf3, 0xde,
 0x60, 0x86, 0x03, 0x97, 0x65, 0xeb, 0x1e, 0x6b, 0xdb, 0x53, 0x85, 0xad, 0x68,
 0x21, 0xf1, 0x5d, 0xe7, 0x1f, 0xe6, 0x53, 0xb4, 0xbb, 0x59, 0x3e, 0x14, 0x27,
 0xb1, 0x83, 0xa7, 0x3a, 0x54, 0xe2, 0x8f, 0x65, 0x8e, 0x6a, 0x4a, 0xcf, 0x3b,
 0x1f, 0x65, 0xff, 0xfe, 0xf1, 0x31, 0x3a, 0x37, 0x7a, 0x8b, 0xcb, 0xc6, 0xd4,
 0x98, 0x50, 0x36, 0x67, 0xe4, 0xa1, 0xe8, 0x7e, 0x8a, 0xc5, 0x23, 0xf2, 0x77,
 0xf5, 0x37, 0x61, 0x49, 0x72, 0x59, 0xe8, 0x3d, 0xf7, 0x60, 0xb2, 0x77, 0xca,
 0x78, 0x54, 0x6d, 0x65, 0x9e, 0x03, 0x97, 0x1b, 0x61, 0xbd, 0x0c, 0xd8, 0x06,
 0x63, 0xe2, 0xc5, 0x48, 0xef, 0xb3, 0xe2, 0x6e, 0x98, 0x7d, 0xbd, 0x4e, 0x72,
 0x91, 0xdb, 0x31, 0x57, 0xe3, 0x65, 0x3a, 0x49, 0xca, 0xec, 0xd2, 0x02, 0x4e,
 0x22, 0x7e, 0x72, 0x8e, 0xf9, 0x79, 0x84, 0x82, 0xdf, 0x7b, 0x92, 0x2d, 0xaf,
 0xc9, 0xe4, 0x33, 0xef, 0x89, 0x5c, 0x66, 0x99, 0xd8, 0x80, 0x81, 0x47, 0x2b,
 0xb1, 0x66, 0x02, 0x84, 0x59, 0x7b, 0xc3, 0xbe, 0x98, 0x45, 0x4a, 0x3d, 0xdd,
 0xea, 0x2b, 0xdf, 0x4e, 0xb4, 0x24, 0x6b, 0xec, 0xe7, 0xd9, 0x0c, 0x45, 0xb8,
 0xbe, 0xca, 0x69, 0x37, 0x92, 0x4c, 0x38, 0x6b, 0x96, 0x6d, 0xcd, 0x86, 0x67,
 0x5c, 0xea, 0x54, 0x94, 0xa4, 0xca, 0xa4, 0x02, 0xa5, 0x21, 0x4d, 0xae, 0x40,
 0x8f, 0x9d, 0x51, 0x83, 0xf2, 0x3f, 0x33, 0xc1, 0x72, 0xb4, 0x1d, 0x94, 0x6e,
 0x7d, 0xe4, 0x27, 0x3f, 0xea, 0xff, 0xe5, 0x9b, 0xa7, 0x5e, 0x55, 0x8e, 0x0d,
 0x69, 0x1c, 0x7a, 0xff, 0x81, 0x9d, 0x53, 0x52, 0x97, 0x9a, 0x76, 0x79, 0xda,
 0x93, 0x32, 0x16, 0xec, 0x69, 0x51, 0x1a, 0x4e, 0xc3, 0xf1, 0x72, 0x80, 0x78,
 0x5e, 0x66, 0x4a, 0x8d, 0x85, 0x2f, 0x3f, 0xb2, 0xa7 };

static void test_verify_sig(void)
{
	BOOL ret;
	HCRYPTPROV prov;
	HCRYPTKEY key;
	HCRYPTHASH hash;
	BYTE bogus[] = { 0 };

	if (!pCryptVerifySignatureW)
	{
		win_skip("CryptVerifySignatureW is not available\n");
		return;
	}

	SetLastError(0xdeadbeef);
	ret = pCryptVerifySignatureW(0, NULL, 0, 0, NULL, 0);
	if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
	{
		win_skip("CryptVerifySignatureW is not implemented\n");
		return;
	}
	ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
	 "Expected ERROR_INVALID_PARAMETER, got %08lx\n", GetLastError());
	ret = CryptAcquireContextA(&prov, szKeySet, NULL, PROV_RSA_FULL,
	 CRYPT_NEWKEYSET);
	if (!ret && GetLastError() == NTE_EXISTS)
		ret = CryptAcquireContextA(&prov, szKeySet, NULL, PROV_RSA_FULL, 0);
	ok(ret, "CryptAcquireContextA failed: %08lx\n", GetLastError());
	ret = CryptImportKey(prov, (LPBYTE)privKey, sizeof(privKey), 0, 0, &key);
	ok(ret, "CryptImportKey failed: %08lx\n", GetLastError());
	ret = CryptCreateHash(prov, CALG_MD5, 0, 0, &hash);
	ok(ret, "CryptCreateHash failed: %08lx\n", GetLastError());
	SetLastError(0xdeadbeef);
	ret = pCryptVerifySignatureW(hash, NULL, 0, 0, NULL, 0);
	ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
	 "Expected ERROR_INVALID_PARAMETER, got %08lx\n", GetLastError());
	SetLastError(0xdeadbeef);
	ret = pCryptVerifySignatureW(0, NULL, 0, key, NULL, 0);
	ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
	 "Expected ERROR_INVALID_PARAMETER, got %08lx\n", GetLastError());
	SetLastError(0xdeadbeef);
	ret = pCryptVerifySignatureW(hash, NULL, 0, key, NULL, 0);
	ok(!ret && (GetLastError() == NTE_BAD_SIGNATURE ||
	 GetLastError() == ERROR_INVALID_PARAMETER),
	 "Expected NTE_BAD_SIGNATURE or ERROR_INVALID_PARAMETER, got %08lx\n",
	 GetLastError());
	SetLastError(0xdeadbeef);
	ret = pCryptVerifySignatureW(hash, NULL, sizeof(bogus), key, NULL, 0);
	ok(!ret && (GetLastError() == NTE_BAD_SIGNATURE ||
	 GetLastError() == ERROR_INVALID_PARAMETER),
	 "Expected NTE_BAD_SIGNATURE or ERROR_INVALID_PARAMETER, got %08lx\n",
	 GetLastError());
	SetLastError(0xdeadbeef);
	ret = pCryptVerifySignatureW(hash, bogus, 0, key, NULL, 0);
	ok(!ret && GetLastError() == NTE_BAD_SIGNATURE,
	 "Expected NTE_BAD_SIGNATURE, got %08lx\n", GetLastError());
	SetLastError(0xdeadbeef);
	ret = pCryptVerifySignatureW(hash, bogus, sizeof(bogus), key, NULL, 0);
	ok(!ret &&
         (GetLastError() == NTE_BAD_SIGNATURE ||
         broken(GetLastError() == NTE_BAD_HASH_STATE /* older NT4 */)),
	 "Expected NTE_BAD_SIGNATURE, got %08lx\n", GetLastError());
	CryptDestroyKey(key);
	CryptDestroyHash(hash);

	ret = CryptReleaseContext(prov, 0);
	ok(ret, "got %lu\n", GetLastError());
}

static BOOL FindProvRegVals(DWORD dwIndex, DWORD *pdwProvType, LPSTR *pszProvName, 
			    DWORD *pcbProvName, DWORD *pdwProvCount)
{
	HKEY hKey;
	HKEY subkey;
	DWORD size = sizeof(DWORD);
	
	if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Cryptography\\Defaults\\Provider", &hKey))
		return FALSE;
	
	RegQueryInfoKeyA(hKey, NULL, NULL, NULL, pdwProvCount, pcbProvName,
				 NULL, NULL, NULL, NULL, NULL, NULL);
	(*pcbProvName)++;

	if (!(*pszProvName = LocalAlloc(LMEM_ZEROINIT, *pcbProvName)))
		return FALSE;
	
	RegEnumKeyExA(hKey, dwIndex, *pszProvName, pcbProvName, NULL, NULL, NULL, NULL);
	(*pcbProvName)++;

	RegOpenKeyA(hKey, *pszProvName, &subkey);
	RegQueryValueExA(subkey, "Type", NULL, NULL, (LPBYTE)pdwProvType, &size);
	
	RegCloseKey(subkey);
	RegCloseKey(hKey);
	
	return TRUE;
}

static void test_enum_providers(void)
{
	/* expected results */
	CHAR *pszProvName = NULL;
	DWORD cbName;
	DWORD dwType;
	DWORD provCount;
	DWORD dwIndex = 0;
	
	/* actual results */
	CHAR *provider = NULL;
	DWORD providerLen;
	DWORD type;
	DWORD count;
	DWORD result;
	DWORD notNull = 5;
	DWORD notZeroFlags = 5;
	
	if(!pCryptEnumProvidersA)
	{
	    win_skip("CryptEnumProvidersA is not available\n");
	    return;
	}
	
	if (!FindProvRegVals(dwIndex, &dwType, &pszProvName, &cbName, &provCount))
	{
	    win_skip("Could not find providers in registry\n");
	    return;
	}
	
	/* check pdwReserved flag for NULL */
	result = pCryptEnumProvidersA(dwIndex, &notNull, 0, &type, NULL, &providerLen);
	ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());
	
	/* check dwFlags == 0 */
	result = pCryptEnumProvidersA(dwIndex, NULL, notZeroFlags, &type, NULL, &providerLen);
	ok(!result && GetLastError()==NTE_BAD_FLAGS, "%ld\n", GetLastError());
	
	/* alloc provider to half the size required
	 * cbName holds the size required */
	providerLen = cbName / 2;
	if (!(provider = LocalAlloc(LMEM_ZEROINIT, providerLen)))
		return;

	result = pCryptEnumProvidersA(dwIndex, NULL, 0, &type, provider, &providerLen);
	ok(!result && GetLastError()==ERROR_MORE_DATA, "expected %i, got %ld\n",
		ERROR_MORE_DATA, GetLastError());

	LocalFree(provider);

	/* loop through the providers to get the number of providers 
	 * after loop ends, count should be provCount + 1 so subtract 1
	 * to get actual number of providers */
	count = 0;
	while(pCryptEnumProvidersA(count++, NULL, 0, &type, NULL, &providerLen))
		;
	count--;
	ok(count==provCount, "expected %i, got %i\n", (int)provCount, (int)count);
	
	/* loop past the actual number of providers to get the error
	 * ERROR_NO_MORE_ITEMS */
	for (count = 0; count < provCount + 1; count++)
		result = pCryptEnumProvidersA(count, NULL, 0, &type, NULL, &providerLen);
	ok(!result && GetLastError()==ERROR_NO_MORE_ITEMS, "expected %i, got %ld\n",
			ERROR_NO_MORE_ITEMS, GetLastError());
	
	/* check expected versus actual values returned */
	result = pCryptEnumProvidersA(dwIndex, NULL, 0, &type, NULL, &providerLen);
	ok(result && providerLen==cbName, "expected %i, got %i\n", (int)cbName, (int)providerLen);
	if (!(provider = LocalAlloc(LMEM_ZEROINIT, providerLen)))
		return;
		
	providerLen = -1;
	result = pCryptEnumProvidersA(dwIndex, NULL, 0, &type, provider, &providerLen);
	ok(result, "expected TRUE, got %ld\n", result);
	ok(type==dwType, "expected %ld, got %ld\n", dwType, type);
	if (pszProvName)
	    ok(!strcmp(pszProvName, provider), "expected %s, got %s\n", pszProvName, provider);
	ok(cbName==providerLen, "expected %ld, got %ld\n", cbName, providerLen);

	providerLen = -1000;
	provider[0] = 0;
	result = pCryptEnumProvidersA(dwIndex, NULL, 0, &type, provider, &providerLen);
	ok(result, "expected TRUE, got %ld\n", result);
	ok(type==dwType, "expected %ld, got %ld\n", dwType, type);
	if (pszProvName)
	    ok(!strcmp(pszProvName, provider), "expected %s, got %s\n", pszProvName, provider);
	ok(cbName==providerLen, "expected %ld, got %ld\n", cbName, providerLen);

	LocalFree(pszProvName);
	LocalFree(provider);
}

static BOOL FindProvTypesRegVals(DWORD *pdwIndex, DWORD *pdwProvType, LPSTR *pszTypeName,
				 DWORD *pcbTypeName, DWORD *pdwTypeCount)
{
	HKEY hKey;
	HKEY hSubKey;
	PSTR ch;
	LPSTR szName;
	DWORD cbName;
	BOOL ret = FALSE;

	if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Cryptography\\Defaults\\Provider Types", &hKey))
		return FALSE;

	if (RegQueryInfoKeyA(hKey, NULL, NULL, NULL, pdwTypeCount, &cbName, NULL,
			NULL, NULL, NULL, NULL, NULL))
		goto cleanup;
	cbName++;

	if (!(szName = LocalAlloc(LMEM_ZEROINIT, cbName)))
		goto cleanup;

	while (!RegEnumKeyExA(hKey, *pdwIndex, szName, &cbName, NULL, NULL, NULL, NULL))
	{
		cbName++;
		ch = szName + strlen(szName);
		/* Convert "Type 000" to 0, etc/ */
		*pdwProvType = *(--ch) - '0';
		*pdwProvType += (*(--ch) - '0') * 10;
		*pdwProvType += (*(--ch) - '0') * 100;

		if (RegOpenKeyA(hKey, szName, &hSubKey))
			break;

		if (!RegQueryValueExA(hSubKey, "TypeName", NULL, NULL, NULL, pcbTypeName))
		{
			if (!(*pszTypeName = LocalAlloc(LMEM_ZEROINIT, *pcbTypeName)))
				break;

			if (!RegQueryValueExA(hSubKey, "TypeName", NULL, NULL, (LPBYTE)*pszTypeName, pcbTypeName))
			{
				ret = TRUE;
				break;
			}

			LocalFree(*pszTypeName);
		}

		RegCloseKey(hSubKey);

		(*pdwIndex)++;
	}
	RegCloseKey(hSubKey);
	LocalFree(szName);

cleanup:
	RegCloseKey(hKey);

	return ret;
}

static void test_enum_provider_types(void)
{
	/* expected values */
	DWORD dwProvType = 0;
	LPSTR pszTypeName = NULL;
	DWORD cbTypeName;
	DWORD dwTypeCount;

	/* actual values */
	DWORD index = 0;
	DWORD provType;
	LPSTR typeName = NULL;
	DWORD typeNameSize;
	DWORD typeCount;
	DWORD result;
	DWORD notNull = 5;
	DWORD notZeroFlags = 5;

	if(!pCryptEnumProviderTypesA)
	{
		win_skip("CryptEnumProviderTypesA is not available\n");
		return;
	}

	if (!FindProvTypesRegVals(&index, &dwProvType, &pszTypeName, &cbTypeName, &dwTypeCount))
	{
		skip("Could not find provider types in registry\n");
		return;
	}

	/* check pdwReserved for NULL */
	result = pCryptEnumProviderTypesA(index, &notNull, 0, &provType, typeName, &typeNameSize);
	ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n",
		GetLastError());

	/* check dwFlags == zero */
	result = pCryptEnumProviderTypesA(index, NULL, notZeroFlags, &provType, typeName, &typeNameSize);
	ok(!result && GetLastError()==NTE_BAD_FLAGS, "expected ERROR_INVALID_PARAMETER, got %ld\n",
		GetLastError());

	/* This test fails under Win2k SP4:
	 * result = TRUE, GetLastError() == 0xdeadbeef */
	if (0)
	{
		/* alloc provider type to half the size required
		 * cbTypeName holds the size required */
		typeNameSize = cbTypeName / 2;
		if (!(typeName = LocalAlloc(LMEM_ZEROINIT, typeNameSize)))
			goto cleanup;

		SetLastError(0xdeadbeef);
		result = pCryptEnumProviderTypesA(index, NULL, 0, &provType, typeName, &typeNameSize);
		ok(!result && GetLastError()==ERROR_MORE_DATA, "expected 0/ERROR_MORE_DATA, got %ld/%ld\n",
			result, GetLastError());

		LocalFree(typeName);
	}

	/* loop through the provider types to get the number of provider types 
	 * after loop ends, count should be dwTypeCount + 1 so subtract 1
	 * to get actual number of provider types */
	typeCount = 0;
	while(pCryptEnumProviderTypesA(typeCount++, NULL, 0, &provType, NULL, &typeNameSize))
		;
	typeCount--;
	ok(typeCount==dwTypeCount, "expected %ld, got %ld\n", dwTypeCount, typeCount);

	/* loop past the actual number of provider types to get the error
	 * ERROR_NO_MORE_ITEMS */
	for (typeCount = 0; typeCount < dwTypeCount + 1; typeCount++)
		result = pCryptEnumProviderTypesA(typeCount, NULL, 0, &provType, NULL, &typeNameSize);
	ok(!result && GetLastError()==ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %ld\n",
		GetLastError());

	/* check expected versus actual values returned */
	result = pCryptEnumProviderTypesA(index, NULL, 0, &provType, NULL, &typeNameSize);
	ok(result && typeNameSize==cbTypeName, "expected %ld, got %ld\n", cbTypeName, typeNameSize);
	if (!(typeName = LocalAlloc(LMEM_ZEROINIT, typeNameSize)))
		goto cleanup;

	typeNameSize = 0xdeadbeef;
	result = pCryptEnumProviderTypesA(index, NULL, 0, &provType, typeName, &typeNameSize);
	ok(result, "expected TRUE, got %ld\n", result);
	ok(provType==dwProvType, "expected %ld, got %ld\n", dwProvType, provType);
	if (pszTypeName)
		ok(!strcmp(pszTypeName, typeName), "expected %s, got %s\n", pszTypeName, typeName);
	ok(typeNameSize==cbTypeName, "expected %ld, got %ld\n", cbTypeName, typeNameSize);

	LocalFree(typeName);
cleanup:
	LocalFree(pszTypeName);
}

static BOOL FindDfltProvRegVals(DWORD dwProvType, DWORD dwFlags, LPSTR *pszProvName, DWORD *pcbProvName)
{
	HKEY hKey;
	PSTR keyname;
	PSTR ptr;
	DWORD user = dwFlags & CRYPT_USER_DEFAULT;
	
	LPCSTR machinestr = "Software\\Microsoft\\Cryptography\\Defaults\\Provider Types\\Type XXX";
	LPCSTR userstr = "Software\\Microsoft\\Cryptography\\Provider Type XXX";
	
	keyname = LocalAlloc(LMEM_ZEROINIT, (user ? strlen(userstr) : strlen(machinestr)) + 1);
	if (keyname)
	{
		user ? strcpy(keyname, userstr) : strcpy(keyname, machinestr);
		ptr = keyname + strlen(keyname);
		*(--ptr) = (dwProvType % 10) + '0';
		*(--ptr) = ((dwProvType / 10) % 10) + '0';
		*(--ptr) = (dwProvType / 100) + '0';
	} else
		return FALSE;
	
	if (RegOpenKeyA((dwFlags & CRYPT_USER_DEFAULT) ?  HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE ,keyname, &hKey))
	{
		LocalFree(keyname);
		return FALSE;
	}
	LocalFree(keyname);
	
	if (RegQueryValueExA(hKey, "Name", NULL, NULL, (LPBYTE)*pszProvName, pcbProvName))
	{
		if (GetLastError() != ERROR_MORE_DATA)
			SetLastError(NTE_PROV_TYPE_ENTRY_BAD);
		return FALSE;
	}
	
	if (!(*pszProvName = LocalAlloc(LMEM_ZEROINIT, *pcbProvName)))
		return FALSE;
	
	if (RegQueryValueExA(hKey, "Name", NULL, NULL, (LPBYTE)*pszProvName, pcbProvName))
	{
		if (GetLastError() != ERROR_MORE_DATA)
			SetLastError(NTE_PROV_TYPE_ENTRY_BAD);
		return FALSE;
	}
	
	RegCloseKey(hKey);
	
	return TRUE;
}

static void test_get_default_provider(void)
{
	/* expected results */
	DWORD dwProvType = PROV_RSA_FULL;
	DWORD dwFlags = CRYPT_MACHINE_DEFAULT;
	LPSTR pszProvName = NULL;
	DWORD cbProvName;
	
	/* actual results */
	DWORD provType = PROV_RSA_FULL;
	DWORD flags = CRYPT_MACHINE_DEFAULT;
	LPSTR provName = NULL;
	DWORD provNameSize;
	DWORD result;
	DWORD notNull = 5;
	
	if(!pCryptGetDefaultProviderA)
	{
	    win_skip("CryptGetDefaultProviderA is not available\n");
	    return;
	}
	
	if(!FindDfltProvRegVals(dwProvType, dwFlags, &pszProvName, &cbProvName))
	{
	    skip("Could not find default provider in registry\n");
	    return;
	}
	
	/* check pdwReserved for NULL */
	result = pCryptGetDefaultProviderA(provType, &notNull, flags, provName, &provNameSize);
	ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "expected %i, got %ld\n",
		ERROR_INVALID_PARAMETER, GetLastError());
	
	/* check for invalid flag */
	flags = 0xdeadbeef;
	result = pCryptGetDefaultProviderA(provType, NULL, flags, provName, &provNameSize);
	ok(!result && GetLastError()==NTE_BAD_FLAGS, "expected %ld, got %ld\n",
		NTE_BAD_FLAGS, GetLastError());
	flags = CRYPT_MACHINE_DEFAULT;
	
	/* check for invalid prov type */
	provType = 0xdeadbeef;
	result = pCryptGetDefaultProviderA(provType, NULL, flags, provName, &provNameSize);
	ok(!result && (GetLastError() == NTE_BAD_PROV_TYPE ||
	               GetLastError() == ERROR_INVALID_PARAMETER),
		"expected NTE_BAD_PROV_TYPE or ERROR_INVALID_PARAMETER, got %ld/%ld\n",
		result, GetLastError());
	provType = PROV_RSA_FULL;
	
	SetLastError(0);
	
	/* alloc provName to half the size required
	 * cbProvName holds the size required */
	provNameSize = cbProvName / 2;
	if (!(provName = LocalAlloc(LMEM_ZEROINIT, provNameSize)))
		return;
	
	result = pCryptGetDefaultProviderA(provType, NULL, flags, provName, &provNameSize);
	ok(!result && GetLastError()==ERROR_MORE_DATA, "expected %i, got %ld\n",
		ERROR_MORE_DATA, GetLastError());
		
	LocalFree(provName);
	
	/* check expected versus actual values returned */
	result = pCryptGetDefaultProviderA(provType, NULL, flags, NULL, &provNameSize);
	ok(result && provNameSize==cbProvName, "expected %ld, got %ld\n", cbProvName, provNameSize);
	provNameSize = cbProvName;
	
	if (!(provName = LocalAlloc(LMEM_ZEROINIT, provNameSize)))
		return;
	
	provNameSize = 0xdeadbeef;
	result = pCryptGetDefaultProviderA(provType, NULL, flags, provName, &provNameSize);
	ok(result, "expected TRUE, got %ld\n", result);
	if(pszProvName)
	    ok(!strcmp(pszProvName, provName), "expected %s, got %s\n", pszProvName, provName);
	ok(provNameSize==cbProvName, "expected %ld, got %ld\n", cbProvName, provNameSize);

	LocalFree(pszProvName);
	LocalFree(provName);
}

static void test_set_provider_ex(void)
{
	DWORD result;
	DWORD notNull = 5;
        LPSTR curProvName = NULL;
        DWORD curlen;
	
	/* results */
	LPSTR pszProvName = NULL;
	DWORD cbProvName;
	
	if(!pCryptGetDefaultProviderA || !pCryptSetProviderExA)
	{
	    win_skip("CryptGetDefaultProviderA and/or CryptSetProviderExA are not available\n");
	    return;
	}

        /* store the current one */
        pCryptGetDefaultProviderA(PROV_RSA_FULL, NULL, CRYPT_MACHINE_DEFAULT, NULL, &curlen);
        if (!(curProvName = LocalAlloc(LMEM_ZEROINIT, curlen)))
            return;
        result = pCryptGetDefaultProviderA(PROV_RSA_FULL, NULL, CRYPT_MACHINE_DEFAULT, curProvName, &curlen);
        ok(result, "%ld\n", GetLastError());

	/* check pdwReserved for NULL */
	result = pCryptSetProviderExA(MS_DEF_PROV_A, PROV_RSA_FULL, &notNull, CRYPT_MACHINE_DEFAULT);
	ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "expected %i, got %ld\n",
		ERROR_INVALID_PARAMETER, GetLastError());

	/* remove the default provider and then set it to MS_DEF_PROV/PROV_RSA_FULL */
        SetLastError(0xdeadbeef);
	result = pCryptSetProviderExA(MS_DEF_PROV_A, PROV_RSA_FULL, NULL, CRYPT_MACHINE_DEFAULT | CRYPT_DELETE_DEFAULT);
	if (!result)
	{
                ok( GetLastError() == ERROR_ACCESS_DENIED || broken(GetLastError() == ERROR_INVALID_PARAMETER),
                    "wrong error %lu\n", GetLastError() );
		skip("Not enough rights to remove the default provider\n");
                LocalFree(curProvName);
		return;
	}

	result = pCryptSetProviderExA(MS_DEF_PROV_A, PROV_RSA_FULL, NULL, CRYPT_MACHINE_DEFAULT);
	ok(result, "%ld\n", GetLastError());
	
	/* call CryptGetDefaultProvider to see if they match */
	result = pCryptGetDefaultProviderA(PROV_RSA_FULL, NULL, CRYPT_MACHINE_DEFAULT, NULL, &cbProvName);
	ok(result, "%ld\n", GetLastError());
	if (!(pszProvName = LocalAlloc(LMEM_ZEROINIT, cbProvName)))
		goto reset;

	result = pCryptGetDefaultProviderA(PROV_RSA_FULL, NULL, CRYPT_MACHINE_DEFAULT, pszProvName, &cbProvName);
	ok(result && !strcmp(MS_DEF_PROV_A, pszProvName), "expected %s, got %s\n", MS_DEF_PROV_A, pszProvName);
	ok(result && cbProvName==(strlen(MS_DEF_PROV_A) + 1), "expected %i, got %ld\n", (lstrlenA(MS_DEF_PROV_A) + 1), cbProvName);

	LocalFree(pszProvName);

reset:
        /* Set the provider back to its original */
        result = pCryptSetProviderExA(curProvName, PROV_RSA_FULL, NULL, CRYPT_MACHINE_DEFAULT);
        ok(result, "%ld\n", GetLastError());
        LocalFree(curProvName);
}

static void test_machine_guid(void)
{
   char originalGuid[40];
   LONG r;
   HKEY key;
   DWORD size;
   HCRYPTPROV hCryptProv;
   BOOL restoreGuid = FALSE, ret;

   r = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Cryptography",
                     0, KEY_ALL_ACCESS, &key);
   if (r != ERROR_SUCCESS)
   {
       skip("couldn't open HKLM\\Software\\Microsoft\\Cryptography\n");
       return;
   }
   /* Cache existing MachineGuid, and delete it */
   size = sizeof(originalGuid);
   r = RegQueryValueExA(key, "MachineGuid", NULL, NULL, (BYTE *)originalGuid,
                        &size);
   if (r == ERROR_SUCCESS)
   {
       restoreGuid = TRUE;
       r = RegDeleteValueA(key, "MachineGuid");
       ok(!r || broken(r == ERROR_ACCESS_DENIED) /*win8*/, "RegDeleteValueA failed: %ld\n", r);
       if (r == ERROR_ACCESS_DENIED)
       {
           skip("broken virtualization on HKLM\\Software\\Microsoft\\Cryptography\n");
           RegCloseKey(key);
           return;
       }
   }
   else
       ok(r == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %ld\n",
          r);
   /* Create and release a provider */
   ret = CryptAcquireContextA(&hCryptProv, szKeySet, NULL, PROV_RSA_FULL, 0);
   ok(ret || broken(!ret && GetLastError() == NTE_KEYSET_ENTRY_BAD /* NT4 */),
      "CryptAcquireContextA failed: %08lx\n", GetLastError());
   ret = CryptReleaseContext(hCryptProv, 0);
   ok(ret, "got %lu\n", GetLastError());

   if (restoreGuid)
       RegSetValueExA(key, "MachineGuid", 0, REG_SZ, (const BYTE *)originalGuid,
                      strlen(originalGuid)+1);
   RegCloseKey(key);
}

#define key_length 16

static const unsigned char key[key_length] =
    { 0xbf, 0xf6, 0x83, 0x4b, 0x3e, 0xa3, 0x23, 0xdd,
      0x96, 0x78, 0x70, 0x8e, 0xa1, 0x9d, 0x3b, 0x40 };

static void test_rc2_keylen(void)
{
    struct KeyBlob
    {
        BLOBHEADER header;
        DWORD key_size;
        BYTE key_data[2048];
    } key_blob;

    HCRYPTPROV provider;
    HCRYPTKEY hkey = 0;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = CryptAcquireContextA(&provider, NULL, NULL,
                                PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    ok(ret, "CryptAcquireContext error %lu\n", GetLastError());
    if (ret)
    {
        key_blob.header.bType = PLAINTEXTKEYBLOB;
        key_blob.header.bVersion = CUR_BLOB_VERSION;
        key_blob.header.reserved = 0;
        key_blob.header.aiKeyAlg = CALG_RC2;
        key_blob.key_size = sizeof(key);
        memcpy(key_blob.key_data, key, key_length);

        /* Importing a 16-byte key works with the default provider. */
        SetLastError(0xdeadbeef);
        ret = CryptImportKey(provider, (BYTE *)&key_blob, sizeof(BLOBHEADER) + sizeof(DWORD) + key_blob.key_size,
                0, CRYPT_IPSEC_HMAC_KEY, &hkey);
        /* CRYPT_IPSEC_HMAC_KEY is not supported on W2K and lower */
        ok(ret ||
           broken(!ret && GetLastError() == NTE_BAD_FLAGS),
           "CryptImportKey error %08lx\n", GetLastError());
        if (ret)
            CryptDestroyKey(hkey);

        ret = CryptReleaseContext(provider, 0);
        ok(ret, "got %lu\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    ret = CryptAcquireContextA(&provider, NULL, MS_DEF_PROV_A,
                                PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    ok(ret, "CryptAcquireContext error %08lx\n", GetLastError());

    if (ret)
    {
        /* Importing a 16-byte key doesn't work with the base provider.. */
        SetLastError(0xdeadbeef);
        ret = CryptImportKey(provider, (BYTE *)&key_blob, sizeof(BLOBHEADER) + sizeof(DWORD) + key_blob.key_size,
                0, 0, &hkey);
        ok(!ret && (GetLastError() == NTE_BAD_DATA ||
                    GetLastError() == NTE_BAD_LEN || /* Win7 */
                    GetLastError() == NTE_BAD_TYPE || /* W2K */
                    GetLastError() == NTE_PERM), /* Win9x, WinMe and NT4 */
           "unexpected error %08lx\n", GetLastError());
        /* but importing an 56-bit (7-byte) key does.. */
        key_blob.key_size = 7;
        SetLastError(0xdeadbeef);
        ret = CryptImportKey(provider, (BYTE *)&key_blob, sizeof(BLOBHEADER) + sizeof(DWORD) + key_blob.key_size,
                0, 0, &hkey);
        ok(ret ||
           broken(!ret && GetLastError() == NTE_BAD_TYPE) || /* W2K */
           broken(!ret && GetLastError() == NTE_PERM), /* Win9x, WinMe and NT4 */
           "CryptAcquireContext error %08lx\n", GetLastError());
        if (ret)
            CryptDestroyKey(hkey);
        /* as does importing a 16-byte key with the base provider when
         * CRYPT_IPSEC_HMAC_KEY is specified.
         */
        key_blob.key_size = sizeof(key);
        SetLastError(0xdeadbeef);
        ret = CryptImportKey(provider, (BYTE *)&key_blob, sizeof(BLOBHEADER) + sizeof(DWORD) + key_blob.key_size,
                0, CRYPT_IPSEC_HMAC_KEY, &hkey);
        /* CRYPT_IPSEC_HMAC_KEY is not supported on W2K and lower */
        ok(ret ||
           broken(!ret && GetLastError() == NTE_BAD_FLAGS),
           "CryptImportKey error %08lx\n", GetLastError());
        if (ret)
            CryptDestroyKey(hkey);

        ret = CryptReleaseContext(provider, 0);
        ok(ret, "got %lu\n", GetLastError());
    }

    key_blob.key_size = sizeof(key);
    SetLastError(0xdeadbeef);
    ret = CryptAcquireContextA(&provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    ok(ret, "CryptAcquireContext error %08lx\n", GetLastError());

    if (ret)
    {
        /* Importing a 16-byte key also works with the default provider when
         * CRYPT_IPSEC_HMAC_KEY is specified.
         */
        SetLastError(0xdeadbeef);
        ret = CryptImportKey(provider, (BYTE *)&key_blob, sizeof(BLOBHEADER) + sizeof(DWORD) + key_blob.key_size,
                0, CRYPT_IPSEC_HMAC_KEY, &hkey);
        ok(ret ||
           broken(!ret && GetLastError() == NTE_BAD_FLAGS),
           "CryptImportKey error %08lx\n", GetLastError());
        if (ret)
            CryptDestroyKey(hkey);

        /* There is no apparent limit to the size of the input key when
         * CRYPT_IPSEC_HMAC_KEY is specified.
         */
        key_blob.key_size = sizeof(key_blob.key_data);
        SetLastError(0xdeadbeef);
        ret = CryptImportKey(provider, (BYTE *)&key_blob, sizeof(BLOBHEADER) + sizeof(DWORD) + key_blob.key_size,
                0, CRYPT_IPSEC_HMAC_KEY, &hkey);
        ok(ret ||
           broken(!ret && GetLastError() == NTE_BAD_FLAGS),
           "CryptImportKey error %08lx\n", GetLastError());
        if (ret)
            CryptDestroyKey(hkey);

        ret = CryptReleaseContext(provider, 0);
        ok(ret, "got %lu\n", GetLastError());
    }
}

static void test_SystemFunction036(void)
{
    BOOL ret;
    int test;

    if (!pSystemFunction036)
    {
        win_skip("SystemFunction036 is not available\n");
        return;
    }

    ret = pSystemFunction036(NULL, 0);
    ok(ret == TRUE, "Expected SystemFunction036 to return TRUE, got %d\n", ret);

    /* Test crashes on Windows. */
    if (0)
    {
        SetLastError(0xdeadbeef);
        ret = pSystemFunction036(NULL, 5);
        trace("ret = %d, GetLastError() = %ld\n", ret, GetLastError());
    }

    ret = pSystemFunction036(&test, 0);
    ok(ret == TRUE, "Expected SystemFunction036 to return TRUE, got %d\n", ret);

    ret = pSystemFunction036(&test, sizeof(int));
    ok(ret == TRUE, "Expected SystemFunction036 to return TRUE, got %d\n", ret);
}

static void test_container_sd(void)
{
    HCRYPTPROV prov;
    SECURITY_DESCRIPTOR *sd;
    DWORD len, err;
    BOOL ret;

    ret = CryptAcquireContextA(&prov, "winetest", "Microsoft Enhanced Cryptographic Provider v1.0",
                               PROV_RSA_FULL, CRYPT_MACHINE_KEYSET|CRYPT_NEWKEYSET);
    ok(ret, "got %lu\n", GetLastError());

    len = 0;
    SetLastError(0xdeadbeef);
    ret = CryptGetProvParam(prov, PP_KEYSET_SEC_DESCR, NULL, &len, OWNER_SECURITY_INFORMATION);
    err = GetLastError();
    ok(ret, "got %lu\n", err);
    ok(err == ERROR_INSUFFICIENT_BUFFER || broken(err == ERROR_INVALID_PARAMETER), "got %lu\n", err);
    ok(len, "expected len > 0\n");

    sd = malloc(len);
    ret = CryptGetProvParam(prov, PP_KEYSET_SEC_DESCR, (BYTE *)sd, &len, OWNER_SECURITY_INFORMATION);
    ok(ret, "got %lu\n", GetLastError());
    free(sd);

    ret = CryptReleaseContext(prov, 0);
    ok(ret, "got %lu\n", GetLastError());

    prov = 0xdeadbeef;
    ret = CryptAcquireContextA(&prov, "winetest", "Microsoft Enhanced Cryptographic Provider v1.0",
                               PROV_RSA_FULL, CRYPT_MACHINE_KEYSET|CRYPT_DELETEKEYSET);
    ok(ret, "got %lu\n", GetLastError());
    ok(prov == 0, "got %Id\n", prov);
}

START_TEST(crypt)
{
    init_function_pointers();

    test_rc2_keylen();

    init_environment();
    test_CryptReleaseContext();
    test_acquire_context();
    test_incorrect_api_usage();
    test_verify_sig();
    test_machine_guid();
    test_container_sd();
    clean_up_environment();

    test_enum_providers();
    test_enum_provider_types();
    test_get_default_provider();
    test_set_provider_ex();
    test_SystemFunction036();
}
