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

static BOOL (WINAPI *pCryptAcquireContextA)(HCRYPTPROV*,LPCSTR,LPCSTR,DWORD,DWORD);
static BOOL (WINAPI *pCryptEnumProviderTypesA)(DWORD, DWORD*, DWORD, DWORD*, LPSTR, DWORD*);
static BOOL (WINAPI *pCryptEnumProvidersA)(DWORD, DWORD*, DWORD, DWORD*, LPSTR, DWORD*);
static BOOL (WINAPI *pCryptGetDefaultProviderA)(DWORD, DWORD*, DWORD, LPSTR, DWORD*);
static BOOL (WINAPI *pCryptReleaseContext)(HCRYPTPROV, DWORD);
static BOOL (WINAPI *pCryptSetProviderExA)(LPCSTR, DWORD, DWORD*, DWORD);
static BOOL (WINAPI *pCryptCreateHash)(HCRYPTPROV, ALG_ID, HCRYPTKEY, DWORD, HCRYPTHASH*);
static BOOL (WINAPI *pCryptDestroyHash)(HCRYPTHASH);
static BOOL (WINAPI *pCryptGenRandom)(HCRYPTPROV, DWORD, BYTE*);
static BOOL (WINAPI *pCryptContextAddRef)(HCRYPTPROV, DWORD*, DWORD dwFlags);
static BOOL (WINAPI *pCryptGenKey)(HCRYPTPROV, ALG_ID, DWORD, HCRYPTKEY*);
static BOOL (WINAPI *pCryptDestroyKey)(HCRYPTKEY);
static BOOL (WINAPI *pCryptDecrypt)(HCRYPTKEY, HCRYPTHASH, BOOL, DWORD, BYTE*, DWORD*);
static BOOL (WINAPI *pCryptDeriveKey)(HCRYPTPROV, ALG_ID, HCRYPTHASH, DWORD, HCRYPTKEY*);
static BOOL (WINAPI *pCryptDuplicateHash)(HCRYPTHASH, DWORD*, DWORD, HCRYPTHASH*);
static BOOL (WINAPI *pCryptDuplicateKey)(HCRYPTKEY, DWORD*, DWORD, HCRYPTKEY*);
static BOOL (WINAPI *pCryptEncrypt)(HCRYPTKEY, HCRYPTHASH, BOOL, DWORD, BYTE*, DWORD*, DWORD);
static BOOL (WINAPI *pCryptExportKey)(HCRYPTKEY, HCRYPTKEY, DWORD, DWORD, BYTE*, DWORD*);
static BOOL (WINAPI *pCryptGetHashParam)(HCRYPTHASH, DWORD, BYTE*, DWORD*, DWORD);
static BOOL (WINAPI *pCryptGetKeyParam)(HCRYPTKEY, DWORD, BYTE*, DWORD*, DWORD);
static BOOL (WINAPI *pCryptGetProvParam)(HCRYPTPROV, DWORD, BYTE*, DWORD*, DWORD);
static BOOL (WINAPI *pCryptGetUserKey)(HCRYPTPROV, DWORD, HCRYPTKEY*);
static BOOL (WINAPI *pCryptHashData)(HCRYPTHASH, BYTE*, DWORD, DWORD);
static BOOL (WINAPI *pCryptHashSessionKey)(HCRYPTHASH, HCRYPTKEY, DWORD);
static BOOL (WINAPI *pCryptImportKey)(HCRYPTPROV, BYTE*, DWORD, HCRYPTKEY, DWORD, HCRYPTKEY*);
static BOOL (WINAPI *pCryptSignHashW)(HCRYPTHASH, DWORD, LPCWSTR, DWORD, BYTE*, DWORD*);
static BOOL (WINAPI *pCryptSetHashParam)(HCRYPTKEY, DWORD, BYTE*, DWORD);
static BOOL (WINAPI *pCryptSetKeyParam)(HCRYPTKEY, DWORD, BYTE*, DWORD);
static BOOL (WINAPI *pCryptSetProvParam)(HCRYPTPROV, DWORD, BYTE*, DWORD);
static BOOL (WINAPI *pCryptVerifySignatureW)(HCRYPTHASH, BYTE*, DWORD, HCRYPTKEY, LPCWSTR, DWORD);

static void init_function_pointers(void)
{
    HMODULE hadvapi32 = GetModuleHandleA("advapi32.dll");

    pCryptAcquireContextA = (void*)GetProcAddress(hadvapi32, "CryptAcquireContextA");
    pCryptEnumProviderTypesA = (void*)GetProcAddress(hadvapi32, "CryptEnumProviderTypesA");
    pCryptEnumProvidersA = (void*)GetProcAddress(hadvapi32, "CryptEnumProvidersA");
    pCryptGetDefaultProviderA = (void*)GetProcAddress(hadvapi32, "CryptGetDefaultProviderA");
    pCryptReleaseContext = (void*)GetProcAddress(hadvapi32, "CryptReleaseContext");
    pCryptSetProviderExA = (void*)GetProcAddress(hadvapi32, "CryptSetProviderExA");
    pCryptCreateHash = (void*)GetProcAddress(hadvapi32, "CryptCreateHash");
    pCryptDestroyHash = (void*)GetProcAddress(hadvapi32, "CryptDestroyHash");
    pCryptGenRandom = (void*)GetProcAddress(hadvapi32, "CryptGenRandom");
    pCryptContextAddRef = (void*)GetProcAddress(hadvapi32, "CryptContextAddRef");
    pCryptGenKey = (void*)GetProcAddress(hadvapi32, "CryptGenKey");
    pCryptDestroyKey = (void*)GetProcAddress(hadvapi32, "CryptDestroyKey");
    pCryptDecrypt = (void*)GetProcAddress(hadvapi32, "CryptDecrypt");
    pCryptDeriveKey = (void*)GetProcAddress(hadvapi32, "CryptDeriveKey");
    pCryptDuplicateHash = (void*)GetProcAddress(hadvapi32, "CryptDuplicateHash");
    pCryptDuplicateKey = (void*)GetProcAddress(hadvapi32, "CryptDuplicateKey");
    pCryptEncrypt = (void*)GetProcAddress(hadvapi32, "CryptEncrypt");
    pCryptExportKey = (void*)GetProcAddress(hadvapi32, "CryptExportKey");
    pCryptGetHashParam = (void*)GetProcAddress(hadvapi32, "CryptGetHashParam");
    pCryptGetKeyParam = (void*)GetProcAddress(hadvapi32, "CryptGetKeyParam");
    pCryptGetProvParam = (void*)GetProcAddress(hadvapi32, "CryptGetProvParam");
    pCryptGetUserKey = (void*)GetProcAddress(hadvapi32, "CryptGetUserKey");
    pCryptHashData = (void*)GetProcAddress(hadvapi32, "CryptHashData");
    pCryptHashSessionKey = (void*)GetProcAddress(hadvapi32, "CryptHashSessionKey");
    pCryptImportKey = (void*)GetProcAddress(hadvapi32, "CryptImportKey");
    pCryptSignHashW = (void*)GetProcAddress(hadvapi32, "CryptSignHashW");
    pCryptSetHashParam = (void*)GetProcAddress(hadvapi32, "CryptSetHashParam");
    pCryptSetKeyParam = (void*)GetProcAddress(hadvapi32, "CryptSetKeyParam");
    pCryptSetProvParam = (void*)GetProcAddress(hadvapi32, "CryptSetProvParam");
    pCryptVerifySignatureW = (void*)GetProcAddress(hadvapi32, "CryptVerifySignatureW");
}

static void init_environment(void)
{
	HCRYPTPROV hProv;
	
	/* Ensure that container "wine_test_keyset" does exist */
	if (!pCryptAcquireContextA(&hProv, szKeySet, szRsaBaseProv, PROV_RSA_FULL, 0))
	{
		pCryptAcquireContextA(&hProv, szKeySet, szRsaBaseProv, PROV_RSA_FULL, CRYPT_NEWKEYSET);
	}
	pCryptReleaseContext(hProv, 0);

	/* Ensure that container "wine_test_keyset" does exist in default PROV_RSA_FULL type provider */
	if (!pCryptAcquireContextA(&hProv, szKeySet, NULL, PROV_RSA_FULL, 0))
	{
		pCryptAcquireContextA(&hProv, szKeySet, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET);
	}
	pCryptReleaseContext(hProv, 0);

	/* Ensure that container "wine_test_bad_keyset" does not exist. */
	if (pCryptAcquireContextA(&hProv, szBadKeySet, szRsaBaseProv, PROV_RSA_FULL, 0))
	{
		pCryptReleaseContext(hProv, 0);
		pCryptAcquireContextA(&hProv, szBadKeySet, szRsaBaseProv, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
	}
}

static void clean_up_environment(void)
{
	HCRYPTPROV hProv;

	/* Remove container "wine_test_keyset" */
	if (pCryptAcquireContextA(&hProv, szKeySet, szRsaBaseProv, PROV_RSA_FULL, 0))
	{
		pCryptReleaseContext(hProv, 0);
		pCryptAcquireContextA(&hProv, szKeySet, szRsaBaseProv, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
	}

	/* Remove container "wine_test_keyset" from default PROV_RSA_FULL type provider */
	if (pCryptAcquireContextA(&hProv, szKeySet, NULL, PROV_RSA_FULL, 0))
	{
		pCryptReleaseContext(hProv, 0);
		pCryptAcquireContextA(&hProv, szKeySet, NULL, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
	}
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
	result = pCryptAcquireContextA(&hProv, NULL, NULL, 0, 0);
	ok(!result && GetLastError()==NTE_BAD_PROV_TYPE, "%d\n", GetLastError());
	
	result = pCryptAcquireContextA(&hProv, NULL, NULL, 1000, 0);
	ok(!result && GetLastError()==NTE_BAD_PROV_TYPE, "%d\n", GetLastError());

	result = pCryptAcquireContextA(&hProv, NULL, NULL, NON_DEF_PROV_TYPE, 0);
	ok(!result && GetLastError()==NTE_PROV_TYPE_NOT_DEF, "%d\n", GetLastError());
	
	result = pCryptAcquireContextA(&hProv, szKeySet, szNonExistentProv, PROV_RSA_FULL, 0);
	ok(!result && GetLastError()==NTE_KEYSET_NOT_DEF, "%d\n", GetLastError());

	result = pCryptAcquireContextA(&hProv, szKeySet, szRsaBaseProv, NON_DEF_PROV_TYPE, 0);
	ok(!result && GetLastError()==NTE_PROV_TYPE_NO_MATCH, "%d\n", GetLastError());
	
	/* This test fails under Win2k SP4:
	   result = TRUE, GetLastError() == ERROR_INVALID_PARAMETER
	SetLastError(0xdeadbeef);
	result = pCryptAcquireContextA(NULL, szKeySet, szRsaBaseProv, PROV_RSA_FULL, 0);
	ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "%d/%d\n", result, GetLastError());
	*/
	
	/* Last not least, try to really acquire a context. */
	hProv = 0;
	SetLastError(0xdeadbeef);
	result = pCryptAcquireContextA(&hProv, szKeySet, szRsaBaseProv, PROV_RSA_FULL, 0);
	GLE = GetLastError();
	ok(result && (GLE == ERROR_ENVVAR_NOT_FOUND   || 
		      GLE == ERROR_SUCCESS            || 
		      GLE == ERROR_RING2_STACK_IN_USE || 
		      GLE == NTE_FAIL                 ||
		      GLE == ERROR_NOT_LOGGED_ON), "%d/%d\n", result, GLE);

	if (hProv) 
		pCryptReleaseContext(hProv, 0);

	/* Try again, witch an empty ("\0") szProvider parameter */
	hProv = 0;
	SetLastError(0xdeadbeef);
	result = pCryptAcquireContextA(&hProv, szKeySet, "", PROV_RSA_FULL, 0);
	GLE = GetLastError();
	ok(result && (GLE == ERROR_ENVVAR_NOT_FOUND   || 
		      GLE == ERROR_SUCCESS            || 
		      GLE == ERROR_RING2_STACK_IN_USE || 
		      GLE == NTE_FAIL                 ||
		      GLE == ERROR_NOT_LOGGED_ON), "%d/%d\n", result, GetLastError());

	if (hProv) 
		pCryptReleaseContext(hProv, 0);
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
    
    result = pCryptAcquireContextA(&hProv, szBadKeySet, szRsaBaseProv, 
                                   PROV_RSA_FULL, CRYPT_NEWKEYSET);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    result = pCryptCreateHash(hProv, CALG_SHA, 0, 0, &hHash);
    ok (result, "%d\n", GetLastError());
    if (!result) return;

    result = pCryptGenKey(hProv, CALG_RC4, 0, &hKey);
    ok (result, "%d\n", GetLastError());
    if (!result) return;

    result = pCryptGenKey(hProv, CALG_RC4, 0, &hKey2);
    ok (result, "%d\n", GetLastError());
    if (!result) return;

    result = pCryptDestroyKey(hKey2);
    ok (result, "%d\n", GetLastError());

    dwTemp = CRYPT_MODE_ECB;    
    result = pCryptSetKeyParam(hKey2, KP_MODE, (BYTE*)&dwTemp, sizeof(DWORD));
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());
    
    result = pCryptAcquireContextA(&hProv2, szBadKeySet, NULL, PROV_RSA_FULL, 
                                   CRYPT_DELETEKEYSET);
    ok (result, "%d\n", GetLastError());
    if (!result) return;
    
    result = pCryptReleaseContext(hProv, 0);
    ok (result, "%d\n", GetLastError());
    if (!result) return;

    result = pCryptReleaseContext(hProv, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

    result = pCryptGenRandom(hProv, 1, &temp);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

#ifdef CRASHES_ON_NT40
    result = pCryptContextAddRef(hProv, NULL, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());
#endif

    result = pCryptCreateHash(hProv, CALG_SHA, 0, 0, &hHash2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

    dwLen = 1;
    result = pCryptDecrypt(hKey, 0, TRUE, 0, &temp, &dwLen);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

    dwLen = 1;
    result = pCryptEncrypt(hKey, 0, TRUE, 0, &temp, &dwLen, 1);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

    result = pCryptDeriveKey(hProv, CALG_RC4, hHash, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

#ifdef CRASHES_ON_NT40
    result = pCryptDuplicateHash(hHash, NULL, 0, &hHash2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

    result = pCryptDuplicateKey(hKey, NULL, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());
#endif

    dwLen = 1;
    result = pCryptExportKey(hKey, 0, 0, 0, &temp, &dwLen);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

    result = pCryptGenKey(hProv, CALG_RC4, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

    dwLen = 1;
    result = pCryptGetHashParam(hHash, 0, &temp, &dwLen, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

    dwLen = 1;
    result = pCryptGetKeyParam(hKey, 0, &temp, &dwLen, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

    dwLen = 1;
    result = pCryptGetProvParam(hProv, 0, &temp, &dwLen, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());
    
    result = pCryptGetUserKey(hProv, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

    result = pCryptHashData(hHash, &temp, 1, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

    result = pCryptHashSessionKey(hHash, hKey, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

    result = pCryptImportKey(hProv, &temp, 1, 0, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

    if (pCryptSignHashW)
    {
        dwLen = 1;
        result = pCryptSignHashW(hHash, 0, NULL, 0, &temp, &dwLen);
        ok (!result && (GetLastError() == ERROR_INVALID_PARAMETER ||
            GetLastError() == ERROR_CALL_NOT_IMPLEMENTED), "%d\n", GetLastError());
    }
    else
        win_skip("CryptSignHashW is not available\n");

    result = pCryptSetKeyParam(hKey, 0, &temp, 1);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

    result = pCryptSetHashParam(hHash, 0, &temp, 1);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

    result = pCryptSetProvParam(hProv, 0, &temp, 1);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());

    if (pCryptVerifySignatureW)
    {
        result = pCryptVerifySignatureW(hHash, &temp, 1, hKey, NULL, 0);
        ok (!result && (GetLastError() == ERROR_INVALID_PARAMETER ||
            GetLastError() == ERROR_CALL_NOT_IMPLEMENTED), "%d\n", GetLastError());
    }
    else
        win_skip("CryptVerifySignatureW is not available\n");

    result = pCryptDestroyHash(hHash);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());
    
    result = pCryptDestroyKey(hKey);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%d\n", GetLastError());
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
	 "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
	ret = pCryptAcquireContextA(&prov, szKeySet, NULL, PROV_RSA_FULL,
	 CRYPT_NEWKEYSET);
	if (!ret && GetLastError() == NTE_EXISTS)
		ret = pCryptAcquireContextA(&prov, szKeySet, NULL, PROV_RSA_FULL, 0);
	ret = pCryptImportKey(prov, (LPBYTE)privKey, sizeof(privKey), 0, 0, &key);
	ok(ret, "CryptImportKey failed: %08x\n", GetLastError());
	ret = pCryptCreateHash(prov, CALG_MD5, 0, 0, &hash);
	ok(ret, "CryptCreateHash failed: %08x\n", GetLastError());
	SetLastError(0xdeadbeef);
	ret = pCryptVerifySignatureW(hash, NULL, 0, 0, NULL, 0);
	ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
	 "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
	SetLastError(0xdeadbeef);
	ret = pCryptVerifySignatureW(0, NULL, 0, key, NULL, 0);
	ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
	 "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
	SetLastError(0xdeadbeef);
	ret = pCryptVerifySignatureW(hash, NULL, 0, key, NULL, 0);
	ok(!ret && (GetLastError() == NTE_BAD_SIGNATURE ||
	 GetLastError() == ERROR_INVALID_PARAMETER),
	 "Expected NTE_BAD_SIGNATURE or ERROR_INVALID_PARAMETER, got %08x\n",
	 GetLastError());
	SetLastError(0xdeadbeef);
	ret = pCryptVerifySignatureW(hash, NULL, sizeof(bogus), key, NULL, 0);
	ok(!ret && (GetLastError() == NTE_BAD_SIGNATURE ||
	 GetLastError() == ERROR_INVALID_PARAMETER),
	 "Expected NTE_BAD_SIGNATURE or ERROR_INVALID_PARAMETER, got %08x\n",
	 GetLastError());
	SetLastError(0xdeadbeef);
	ret = pCryptVerifySignatureW(hash, bogus, 0, key, NULL, 0);
	ok(!ret && GetLastError() == NTE_BAD_SIGNATURE,
	 "Expected NTE_BAD_SIGNATURE, got %08x\n", GetLastError());
	SetLastError(0xdeadbeef);
	ret = pCryptVerifySignatureW(hash, bogus, sizeof(bogus), key, NULL, 0);
	ok(!ret && GetLastError() == NTE_BAD_SIGNATURE,
	 "Expected NTE_BAD_SIGNATURE, got %08x\n", GetLastError());
	pCryptDestroyKey(key);
	pCryptDestroyHash(hash);
	pCryptReleaseContext(prov, 0);
}

static BOOL FindProvRegVals(DWORD dwIndex, DWORD *pdwProvType, LPSTR *pszProvName, 
			    DWORD *pcbProvName, DWORD *pdwProvCount)
{
	HKEY hKey;
	HKEY subkey;
	DWORD size = sizeof(DWORD);
	
	if (RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Cryptography\\Defaults\\Provider", &hKey))
		return FALSE;
	
	RegQueryInfoKey(hKey, NULL, NULL, NULL, pdwProvCount, pcbProvName, 
				 NULL, NULL, NULL, NULL, NULL, NULL);
	(*pcbProvName)++;

	if (!(*pszProvName = LocalAlloc(LMEM_ZEROINIT, *pcbProvName)))
		return FALSE;
	
	RegEnumKeyEx(hKey, dwIndex, *pszProvName, pcbProvName, NULL, NULL, NULL, NULL);
	(*pcbProvName)++;

	RegOpenKey(hKey, *pszProvName, &subkey);
	RegQueryValueEx(subkey, "Type", NULL, NULL, (LPBYTE)pdwProvType, &size);
	
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
	ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "%d\n", GetLastError());
	
	/* check dwFlags == 0 */
	result = pCryptEnumProvidersA(dwIndex, NULL, notZeroFlags, &type, NULL, &providerLen);
	ok(!result && GetLastError()==NTE_BAD_FLAGS, "%d\n", GetLastError());
	
	/* alloc provider to half the size required
	 * cbName holds the size required */
	providerLen = cbName / 2;
	if (!(provider = LocalAlloc(LMEM_ZEROINIT, providerLen)))
		return;

	result = pCryptEnumProvidersA(dwIndex, NULL, 0, &type, provider, &providerLen);
	ok(!result && GetLastError()==ERROR_MORE_DATA, "expected %i, got %d\n",
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
	ok(!result && GetLastError()==ERROR_NO_MORE_ITEMS, "expected %i, got %d\n", 
			ERROR_NO_MORE_ITEMS, GetLastError());
	
	/* check expected versus actual values returned */
	result = pCryptEnumProvidersA(dwIndex, NULL, 0, &type, NULL, &providerLen);
	ok(result && providerLen==cbName, "expected %i, got %i\n", (int)cbName, (int)providerLen);
	if (!(provider = LocalAlloc(LMEM_ZEROINIT, providerLen)))
		return;
		
	providerLen = 0xdeadbeef;
	result = pCryptEnumProvidersA(dwIndex, NULL, 0, &type, provider, &providerLen);
	ok(result, "expected TRUE, got %d\n", result);
	ok(type==dwType, "expected %d, got %d\n", dwType, type);
	if (pszProvName)
	    ok(!strcmp(pszProvName, provider), "expected %s, got %s\n", pszProvName, provider);
	ok(cbName==providerLen, "expected %d, got %d\n", cbName, providerLen);

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

	if (RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Cryptography\\Defaults\\Provider Types", &hKey))
		return FALSE;

	if (RegQueryInfoKey(hKey, NULL, NULL, NULL, pdwTypeCount, &cbName, NULL,
			NULL, NULL, NULL, NULL, NULL))
		goto cleanup;
	cbName++;

	if (!(szName = LocalAlloc(LMEM_ZEROINIT, cbName)))
		goto cleanup;

	while (!RegEnumKeyEx(hKey, *pdwIndex, szName, &cbName, NULL, NULL, NULL, NULL))
	{
		cbName++;
		ch = szName + strlen(szName);
		/* Convert "Type 000" to 0, etc/ */
		*pdwProvType = *(--ch) - '0';
		*pdwProvType += (*(--ch) - '0') * 10;
		*pdwProvType += (*(--ch) - '0') * 100;

		if (RegOpenKey(hKey, szName, &hSubKey))
			break;

		if (!RegQueryValueEx(hSubKey, "TypeName", NULL, NULL, NULL, pcbTypeName))
		{
			if (!(*pszTypeName = LocalAlloc(LMEM_ZEROINIT, *pcbTypeName)))
				break;

			if (!RegQueryValueEx(hSubKey, "TypeName", NULL, NULL, (LPBYTE)*pszTypeName, pcbTypeName))
			{
				ret = TRUE;
				break;
			}

			LocalFree(*pszTypeName);
		}

		RegCloseKey(hSubKey);

		(*pdwIndex)++;
	}

	if (!ret)
		LocalFree(*pszTypeName);
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
	ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n",
		GetLastError());

	/* check dwFlags == zero */
	result = pCryptEnumProviderTypesA(index, NULL, notZeroFlags, &provType, typeName, &typeNameSize);
	ok(!result && GetLastError()==NTE_BAD_FLAGS, "expected ERROR_INVALID_PARAMETER, got %d\n",
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
		ok(!result && GetLastError()==ERROR_MORE_DATA, "expected 0/ERROR_MORE_DATA, got %d/%d\n",
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
	ok(typeCount==dwTypeCount, "expected %d, got %d\n", dwTypeCount, typeCount);

	/* loop past the actual number of provider types to get the error
	 * ERROR_NO_MORE_ITEMS */
	for (typeCount = 0; typeCount < dwTypeCount + 1; typeCount++)
		result = pCryptEnumProviderTypesA(typeCount, NULL, 0, &provType, NULL, &typeNameSize);
	ok(!result && GetLastError()==ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %d\n",
		GetLastError());

	/* check expected versus actual values returned */
	result = pCryptEnumProviderTypesA(index, NULL, 0, &provType, NULL, &typeNameSize);
	ok(result && typeNameSize==cbTypeName, "expected %d, got %d\n", cbTypeName, typeNameSize);
	if (!(typeName = LocalAlloc(LMEM_ZEROINIT, typeNameSize)))
		goto cleanup;

	typeNameSize = 0xdeadbeef;
	result = pCryptEnumProviderTypesA(index, NULL, 0, &provType, typeName, &typeNameSize);
	ok(result, "expected TRUE, got %d\n", result);
	ok(provType==dwProvType, "expected %d, got %d\n", dwProvType, provType);
	if (pszTypeName)
		ok(!strcmp(pszTypeName, typeName), "expected %s, got %s\n", pszTypeName, typeName);
	ok(typeNameSize==cbTypeName, "expected %d, got %d\n", cbTypeName, typeNameSize);

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
	
	if (RegOpenKey((dwFlags & CRYPT_USER_DEFAULT) ?  HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE ,keyname, &hKey))
	{
		LocalFree(keyname);
		return FALSE;
	}
	LocalFree(keyname);
	
	if (RegQueryValueEx(hKey, "Name", NULL, NULL, (LPBYTE)*pszProvName, pcbProvName))
	{
		if (GetLastError() != ERROR_MORE_DATA)
			SetLastError(NTE_PROV_TYPE_ENTRY_BAD);
		return FALSE;
	}
	
	if (!(*pszProvName = LocalAlloc(LMEM_ZEROINIT, *pcbProvName)))
		return FALSE;
	
	if (RegQueryValueEx(hKey, "Name", NULL, NULL, (LPBYTE)*pszProvName, pcbProvName))
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
	ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "expected %i, got %d\n",
		ERROR_INVALID_PARAMETER, GetLastError());
	
	/* check for invalid flag */
	flags = 0xdeadbeef;
	result = pCryptGetDefaultProviderA(provType, NULL, flags, provName, &provNameSize);
	ok(!result && GetLastError()==NTE_BAD_FLAGS, "expected %d, got %d\n",
		NTE_BAD_FLAGS, GetLastError());
	flags = CRYPT_MACHINE_DEFAULT;
	
	/* check for invalid prov type */
	provType = 0xdeadbeef;
	result = pCryptGetDefaultProviderA(provType, NULL, flags, provName, &provNameSize);
	ok(!result && (GetLastError() == NTE_BAD_PROV_TYPE ||
	               GetLastError() == ERROR_INVALID_PARAMETER),
		"expected NTE_BAD_PROV_TYPE or ERROR_INVALID_PARAMETER, got %d/%d\n",
		result, GetLastError());
	provType = PROV_RSA_FULL;
	
	SetLastError(0);
	
	/* alloc provName to half the size required
	 * cbProvName holds the size required */
	provNameSize = cbProvName / 2;
	if (!(provName = LocalAlloc(LMEM_ZEROINIT, provNameSize)))
		return;
	
	result = pCryptGetDefaultProviderA(provType, NULL, flags, provName, &provNameSize);
	ok(!result && GetLastError()==ERROR_MORE_DATA, "expected %i, got %d\n",
		ERROR_MORE_DATA, GetLastError());
		
	LocalFree(provName);
	
	/* check expected versus actual values returned */
	result = pCryptGetDefaultProviderA(provType, NULL, flags, NULL, &provNameSize);
	ok(result && provNameSize==cbProvName, "expected %d, got %d\n", cbProvName, provNameSize);
	provNameSize = cbProvName;
	
	if (!(provName = LocalAlloc(LMEM_ZEROINIT, provNameSize)))
		return;
	
	provNameSize = 0xdeadbeef;
	result = pCryptGetDefaultProviderA(provType, NULL, flags, provName, &provNameSize);
	ok(result, "expected TRUE, got %d\n", result);
	if(pszProvName)
	    ok(!strcmp(pszProvName, provName), "expected %s, got %s\n", pszProvName, provName);
	ok(provNameSize==cbProvName, "expected %d, got %d\n", cbProvName, provNameSize);

	LocalFree(provName);
}

static void test_set_provider_ex(void)
{
	DWORD result;
	DWORD notNull = 5;
	
	/* results */
	LPSTR pszProvName = NULL;
	DWORD cbProvName;
	
	if(!pCryptGetDefaultProviderA || !pCryptSetProviderExA)
	{
	    win_skip("CryptGetDefaultProviderA and/or CryptSetProviderExA are not available\n");
	    return;
	}

	/* check pdwReserved for NULL */
	result = pCryptSetProviderExA(MS_DEF_PROV, PROV_RSA_FULL, &notNull, CRYPT_MACHINE_DEFAULT);
	ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "expected %i, got %d\n",
		ERROR_INVALID_PARAMETER, GetLastError());

	/* remove the default provider and then set it to MS_DEF_PROV/PROV_RSA_FULL */
        SetLastError(0xdeadbeef);
	result = pCryptSetProviderExA(MS_DEF_PROV, PROV_RSA_FULL, NULL, CRYPT_MACHINE_DEFAULT | CRYPT_DELETE_DEFAULT);
	if (!result)
	{
                ok( GetLastError() == ERROR_ACCESS_DENIED || broken(GetLastError() == ERROR_INVALID_PARAMETER),
                    "wrong error %u\n", GetLastError() );
		skip("Not enough rights to remove the default provider\n");
		return;
	}

	result = pCryptSetProviderExA(MS_DEF_PROV, PROV_RSA_FULL, NULL, CRYPT_MACHINE_DEFAULT);
	ok(result, "%d\n", GetLastError());
	
	/* call CryptGetDefaultProvider to see if they match */
	result = pCryptGetDefaultProviderA(PROV_RSA_FULL, NULL, CRYPT_MACHINE_DEFAULT, NULL, &cbProvName);
	if (!(pszProvName = LocalAlloc(LMEM_ZEROINIT, cbProvName)))
		return;

	result = pCryptGetDefaultProviderA(PROV_RSA_FULL, NULL, CRYPT_MACHINE_DEFAULT, pszProvName, &cbProvName);
	ok(result && !strcmp(MS_DEF_PROV, pszProvName), "expected %s, got %s\n", MS_DEF_PROV, pszProvName);
	ok(result && cbProvName==(strlen(MS_DEF_PROV) + 1), "expected %i, got %d\n", (lstrlenA(MS_DEF_PROV) + 1), cbProvName);

	LocalFree(pszProvName);
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
       ok(!r, "RegDeleteValueA failed: %d\n", r);
   }
   else
       ok(r == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %d\n",
          r);
   /* Create and release a provider */
   ret = pCryptAcquireContextA(&hCryptProv, szKeySet, NULL, PROV_RSA_FULL, 0);
   ok(ret, "CryptAcquireContextA failed: %08x\n", GetLastError());
   pCryptReleaseContext(hCryptProv, 0);

   if (restoreGuid)
       RegSetValueExA(key, "MachineGuid", 0, REG_SZ, (const BYTE *)originalGuid,
                      strlen(originalGuid)+1);
   RegCloseKey(key);
}

START_TEST(crypt)
{
	init_function_pointers();
	if(pCryptAcquireContextA && pCryptReleaseContext) {
	init_environment();
	test_acquire_context();
	test_incorrect_api_usage();
	test_verify_sig();
	test_machine_guid();
	clean_up_environment();
	}
	
	test_enum_providers();
	test_enum_provider_types();
	test_get_default_provider();
	test_set_provider_ex();
}
