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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

static HMODULE hadvapi32;
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
    hadvapi32 = GetModuleHandleA("advapi32.dll");

    if(hadvapi32)
    {
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

	/* Provoke all kinds of error conditions (which are easy to provoke).
	 * The order of the error tests seems to match Windows XP's rsaenh.dll CSP,
	 * but since this is likely to change between CSP versions, we don't check
	 * this. Please don't change the order of tests. */
	result = pCryptAcquireContextA(&hProv, NULL, NULL, 0, 0);
	ok(!result && GetLastError()==NTE_BAD_PROV_TYPE, "%ld\n", GetLastError());

	result = pCryptAcquireContextA(&hProv, NULL, NULL, 1000, 0);
	ok(!result && GetLastError()==NTE_BAD_PROV_TYPE, "%ld\n", GetLastError());

	result = pCryptAcquireContextA(&hProv, NULL, NULL, NON_DEF_PROV_TYPE, 0);
	ok(!result && GetLastError()==NTE_PROV_TYPE_NOT_DEF, "%ld\n", GetLastError());

	result = pCryptAcquireContextA(&hProv, szKeySet, szNonExistentProv, PROV_RSA_FULL, 0);
	ok(!result && GetLastError()==NTE_KEYSET_NOT_DEF, "%ld\n", GetLastError());

	result = pCryptAcquireContextA(&hProv, szKeySet, szRsaBaseProv, NON_DEF_PROV_TYPE, 0);
	ok(!result && GetLastError()==NTE_PROV_TYPE_NO_MATCH, "%ld\n", GetLastError());

	/* This test fails under Win2k SP4:
	   result = TRUE, GetLastError() == ERROR_INVALID_PARAMETER
	SetLastError(0xdeadbeef);
	result = pCryptAcquireContextA(NULL, szKeySet, szRsaBaseProv, PROV_RSA_FULL, 0);
	ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "%d/%ld\n", result, GetLastError());
	*/

	/* Last not least, try to really acquire a context. */
	hProv = 0;
	SetLastError(0xdeadbeef);
	result = pCryptAcquireContextA(&hProv, szKeySet, szRsaBaseProv, PROV_RSA_FULL, 0);
	ok(result && (GetLastError() == ERROR_ENVVAR_NOT_FOUND || GetLastError() == ERROR_SUCCESS || GetLastError() == ERROR_RING2_STACK_IN_USE || GetLastError() == NTE_FAIL), "%d/%ld\n", result, GetLastError());

	if (hProv)
		pCryptReleaseContext(hProv, 0);

	/* Try again, witch an empty ("\0") szProvider parameter */
	hProv = 0;
	SetLastError(0xdeadbeef);
	result = pCryptAcquireContextA(&hProv, szKeySet, "", PROV_RSA_FULL, 0);
	ok(result && (GetLastError() == ERROR_ENVVAR_NOT_FOUND || GetLastError() == ERROR_SUCCESS  || GetLastError() == ERROR_RING2_STACK_IN_USE || GetLastError() == NTE_FAIL), "%d/%ld\n", result, GetLastError());

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
    ok (result, "%08lx\n", GetLastError());
    if (!result) return;

    result = pCryptCreateHash(hProv, CALG_SHA, 0, 0, &hHash);
    ok (result, "%ld\n", GetLastError());
    if (!result) return;

    result = pCryptGenKey(hProv, CALG_RC4, 0, &hKey);
    ok (result, "%ld\n", GetLastError());
    if (!result) return;

    result = pCryptGenKey(hProv, CALG_RC4, 0, &hKey2);
    ok (result, "%ld\n", GetLastError());
    if (!result) return;

    result = pCryptDestroyKey(hKey2);
    ok (result, "%ld\n", GetLastError());

    dwTemp = CRYPT_MODE_ECB;
    result = pCryptSetKeyParam(hKey2, KP_MODE, (BYTE*)&dwTemp, sizeof(DWORD));
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptAcquireContextA(&hProv2, szBadKeySet, NULL, PROV_RSA_FULL,
                                   CRYPT_DELETEKEYSET);
    ok (result, "%ld\n", GetLastError());
    if (!result) return;

    result = pCryptReleaseContext(hProv, 0);
    ok (result, "%ld\n", GetLastError());
    if (!result) return;

    result = pCryptReleaseContext(hProv, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptGenRandom(hProv, 1, &temp);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

#ifdef CRASHES_ON_NT40
    result = pCryptContextAddRef(hProv, NULL, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());
#endif

    result = pCryptCreateHash(hProv, CALG_SHA, 0, 0, &hHash2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    dwLen = 1;
    result = pCryptDecrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, &temp, &dwLen);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    dwLen = 1;
    result = pCryptEncrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, &temp, &dwLen, 1);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptDeriveKey(hProv, CALG_RC4, hHash, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

#ifdef CRASHES_ON_NT40
    result = pCryptDuplicateHash(hHash, NULL, 0, &hHash2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptDuplicateKey(hKey, NULL, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());
#endif

    dwLen = 1;
    result = pCryptExportKey(hKey, (HCRYPTPROV)NULL, 0, 0, &temp, &dwLen);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptGenKey(hProv, CALG_RC4, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    dwLen = 1;
    result = pCryptGetHashParam(hHash, 0, &temp, &dwLen, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    dwLen = 1;
    result = pCryptGetKeyParam(hKey, 0, &temp, &dwLen, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    dwLen = 1;
    result = pCryptGetProvParam(hProv, 0, &temp, &dwLen, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptGetUserKey(hProv, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptHashData(hHash, &temp, 1, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptHashSessionKey(hHash, hKey, 0);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptImportKey(hProv, &temp, 1, (HCRYPTKEY)NULL, 0, &hKey2);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    dwLen = 1;
    result = pCryptSignHashW(hHash, 0, NULL, 0, &temp, &dwLen);
    ok (!result && (GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_CALL_NOT_IMPLEMENTED), "%ld\n", GetLastError());

    result = pCryptSetKeyParam(hKey, 0, &temp, 1);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptSetHashParam(hHash, 0, &temp, 1);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptSetProvParam(hProv, 0, &temp, 1);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptVerifySignatureW(hHash, &temp, 1, hKey, NULL, 0);
    ok (!result && (GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_CALL_NOT_IMPLEMENTED), "%ld\n", GetLastError());

    result = pCryptDestroyHash(hHash);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

    result = pCryptDestroyKey(hKey);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());
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

	if (!(*pszProvName = ((LPSTR)LocalAlloc(LMEM_ZEROINIT, *pcbProvName))))
		return FALSE;

	RegEnumKeyEx(hKey, dwIndex, *pszProvName, pcbProvName, NULL, NULL, NULL, NULL);
	(*pcbProvName)++;

	RegOpenKey(hKey, *pszProvName, &subkey);
	RegQueryValueEx(subkey, "Type", NULL, NULL, (BYTE*)pdwProvType, &size);

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
	BOOL result;
	DWORD notNull = 5;
	DWORD notZeroFlags = 5;

	if(!pCryptEnumProvidersA)
	{
	    trace("skipping CryptEnumProviders tests\n");
	    return;
	}

	if (!FindProvRegVals(dwIndex, &dwType, &pszProvName, &cbName, &provCount))
		return;

	/* check pdwReserved flag for NULL */
	result = pCryptEnumProvidersA(dwIndex, &notNull, 0, &type, NULL, &providerLen);
	ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "%ld\n", GetLastError());

	/* check dwFlags == 0 */
	result = pCryptEnumProvidersA(dwIndex, NULL, notZeroFlags, &type, NULL, &providerLen);
	ok(!result && GetLastError()==NTE_BAD_FLAGS, "%ld\n", GetLastError());

	/* alloc provider to half the size required
	 * cbName holds the size required */
	providerLen = cbName / 2;
	if (!(provider = ((LPSTR)LocalAlloc(LMEM_ZEROINIT, providerLen))))
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
	if (!(provider = ((LPSTR)LocalAlloc(LMEM_ZEROINIT, providerLen))))
		return;

	result = pCryptEnumProvidersA(dwIndex, NULL, 0, &type, provider, &providerLen);
	ok(result && type==dwType, "expected %ld, got %ld\n",
		dwType, type);
	ok(result && !strcmp(pszProvName, provider), "expected %s, got %s\n", pszProvName, provider);
	ok(result && cbName==providerLen, "expected %ld, got %ld\n",
		cbName, providerLen);

	LocalFree(provider);
}

static BOOL FindProvTypesRegVals(DWORD dwIndex, DWORD *pdwProvType, LPSTR *pszTypeName,
				 DWORD *pcbTypeName, DWORD *pdwTypeCount)
{
	HKEY hKey;
	HKEY hSubKey;
	PSTR ch;

	if (RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Cryptography\\Defaults\\Provider Types", &hKey))
		return FALSE;

	if (RegQueryInfoKey(hKey, NULL, NULL, NULL, pdwTypeCount, pcbTypeName, NULL,
			NULL, NULL, NULL, NULL, NULL))
	    return FALSE;
	(*pcbTypeName)++;

	if (!(*pszTypeName = ((LPSTR)LocalAlloc(LMEM_ZEROINIT, *pcbTypeName))))
		return FALSE;

	if (RegEnumKeyEx(hKey, dwIndex, *pszTypeName, pcbTypeName, NULL, NULL, NULL, NULL))
	    return FALSE;
	(*pcbTypeName)++;
	ch = *pszTypeName + strlen(*pszTypeName);
	/* Convert "Type 000" to 0, etc/ */
	*pdwProvType = *(--ch) - '0';
	*pdwProvType += (*(--ch) - '0') * 10;
	*pdwProvType += (*(--ch) - '0') * 100;

	if (RegOpenKey(hKey, *pszTypeName, &hSubKey))
	    return FALSE;

	if (RegQueryValueEx(hSubKey, "TypeName", NULL, NULL, NULL, pcbTypeName))
            return FALSE;

	if (!(*pszTypeName = ((LPSTR)LocalAlloc(LMEM_ZEROINIT, *pcbTypeName))))
		return FALSE;

	if (RegQueryValueEx(hSubKey, "TypeName", NULL, NULL, *pszTypeName, pcbTypeName))
	    return FALSE;

	RegCloseKey(hSubKey);
	RegCloseKey(hKey);

	return TRUE;
}

static void test_enum_provider_types()
{
	/* expected values */
	DWORD dwProvType;
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
	    trace("skipping CryptEnumProviderTypes tests\n");
	    return;
	}

	if (!FindProvTypesRegVals(index, &dwProvType, &pszTypeName, &cbTypeName, &dwTypeCount))
	{
	    trace("could not find provider types in registry, skipping the test\n");
	    return;
	}

	/* check pdwReserved for NULL */
	result = pCryptEnumProviderTypesA(index, &notNull, 0, &provType, typeName, &typeNameSize);
	ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "expected %i, got %ld\n",
		ERROR_INVALID_PARAMETER, GetLastError());

	/* check dwFlags == zero */
	result = pCryptEnumProviderTypesA(index, NULL, notZeroFlags, &provType, typeName, &typeNameSize);
	ok(!result && GetLastError()==NTE_BAD_FLAGS, "expected %i, got %ld\n",
		ERROR_INVALID_PARAMETER, GetLastError());

	/* alloc provider type to half the size required
	 * cbTypeName holds the size required */
	typeNameSize = cbTypeName / 2;
	if (!(typeName = ((LPSTR)LocalAlloc(LMEM_ZEROINIT, typeNameSize))))
		return;

	/* This test fails under Win2k SP4:
	   result = TRUE, GetLastError() == 0xdeadbeef
	SetLastError(0xdeadbeef);
	result = pCryptEnumProviderTypesA(index, NULL, 0, &provType, typeName, &typeNameSize);
	ok(!result && GetLastError()==ERROR_MORE_DATA, "expected 0/ERROR_MORE_DATA, got %d/%08lx\n",
		result, GetLastError());
	*/

	LocalFree(typeName);

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
	ok(!result && GetLastError()==ERROR_NO_MORE_ITEMS, "expected %i, got %ld\n",
			ERROR_NO_MORE_ITEMS, GetLastError());


	/* check expected versus actual values returned */
	result = pCryptEnumProviderTypesA(index, NULL, 0, &provType, NULL, &typeNameSize);
	ok(result && typeNameSize==cbTypeName, "expected %ld, got %ld\n", cbTypeName, typeNameSize);
	if (!(typeName = ((LPSTR)LocalAlloc(LMEM_ZEROINIT, typeNameSize))))
		return;

	typeNameSize = 0xdeadbeef;
	result = pCryptEnumProviderTypesA(index, NULL, 0, &provType, typeName, &typeNameSize);
	ok(result, "expected TRUE, got %ld\n", result);
	ok(provType==dwProvType, "expected %ld, got %ld\n", dwProvType, provType);
	if (pszTypeName)
	    ok(!strcmp(pszTypeName, typeName), "expected %s, got %s\n", pszTypeName, typeName);
	ok(typeNameSize==cbTypeName, "expected %ld, got %ld\n", cbTypeName, typeNameSize);

	LocalFree(typeName);
}

static BOOL FindDfltProvRegVals(DWORD dwProvType, DWORD dwFlags, LPSTR *pszProvName, DWORD *pcbProvName)
{
	HKEY hKey;
	PSTR keyname;
	PSTR ptr;
	DWORD user = dwFlags & CRYPT_USER_DEFAULT;

	LPSTR MACHINESTR = "Software\\Microsoft\\Cryptography\\Defaults\\Provider Types\\Type XXX";
	LPSTR USERSTR = "Software\\Microsoft\\Cryptography\\Provider Type XXX";

	keyname = LocalAlloc(LMEM_ZEROINIT, (user ? strlen(USERSTR) : strlen(MACHINESTR)) + 1);
	if (keyname)
	{
		user ? strcpy(keyname, USERSTR) : strcpy(keyname, MACHINESTR);
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

	if (RegQueryValueEx(hKey, "Name", NULL, NULL, *pszProvName, pcbProvName))
	{
		if (GetLastError() != ERROR_MORE_DATA)
			SetLastError(NTE_PROV_TYPE_ENTRY_BAD);
		return FALSE;
	}

	if (!(*pszProvName = LocalAlloc(LMEM_ZEROINIT, *pcbProvName)))
		return FALSE;

	if (RegQueryValueEx(hKey, "Name", NULL, NULL, *pszProvName, pcbProvName))
	{
		if (GetLastError() != ERROR_MORE_DATA)
			SetLastError(NTE_PROV_TYPE_ENTRY_BAD);
		return FALSE;
	}

	RegCloseKey(hKey);

	return TRUE;
}

static void test_get_default_provider()
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
	    trace("skipping CryptGetDefaultProvider tests\n");
	    return;
	}

	FindDfltProvRegVals(dwProvType, dwFlags, &pszProvName, &cbProvName);

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

	result = pCryptGetDefaultProviderA(provType, NULL, flags, provName, &provNameSize);
	ok(result && !strcmp(pszProvName, provName), "expected %s, got %s\n", pszProvName, provName);
	ok(result && provNameSize==cbProvName, "expected %ld, got %ld\n", cbProvName, provNameSize);

	LocalFree(provName);
}

static void test_set_provider_ex()
{
	DWORD result;
	DWORD notNull = 5;

	/* results */
	LPSTR pszProvName = NULL;
	DWORD cbProvName;

	if(!pCryptGetDefaultProviderA || !pCryptSetProviderExA)
	{
	    trace("skipping CryptSetProviderEx tests\n");
	    return;
	}

	/* check pdwReserved for NULL */
	result = pCryptSetProviderExA(MS_DEF_PROV, PROV_RSA_FULL, &notNull, CRYPT_MACHINE_DEFAULT);
	ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "expected %i, got %ld\n",
		ERROR_INVALID_PARAMETER, GetLastError());

	/* remove the default provider and then set it to MS_DEF_PROV/PROV_RSA_FULL */
	result = pCryptSetProviderExA(MS_DEF_PROV, PROV_RSA_FULL, NULL, CRYPT_MACHINE_DEFAULT | CRYPT_DELETE_DEFAULT);
	ok(result, "%ld\n", GetLastError());

	result = pCryptSetProviderExA(MS_DEF_PROV, PROV_RSA_FULL, NULL, CRYPT_MACHINE_DEFAULT);
	ok(result, "%ld\n", GetLastError());

	/* call CryptGetDefaultProvider to see if they match */
	result = pCryptGetDefaultProviderA(PROV_RSA_FULL, NULL, CRYPT_MACHINE_DEFAULT, NULL, &cbProvName);
	if (!(pszProvName = LocalAlloc(LMEM_ZEROINIT, cbProvName)))
		return;

	result = pCryptGetDefaultProviderA(PROV_RSA_FULL, NULL, CRYPT_MACHINE_DEFAULT, pszProvName, &cbProvName);
	ok(result && !strcmp(MS_DEF_PROV, pszProvName), "expected %s, got %s\n", MS_DEF_PROV, pszProvName);
	ok(result && cbProvName==(strlen(MS_DEF_PROV) + 1), "expected %i, got %ld\n", (strlen(MS_DEF_PROV) + 1), cbProvName);

	LocalFree(pszProvName);
}

START_TEST(crypt)
{
	init_function_pointers();
	if(pCryptAcquireContextA && pCryptReleaseContext) {
	init_environment();
	test_acquire_context();
	test_incorrect_api_usage();
	clean_up_environment();
	}

	test_enum_providers();
	test_enum_provider_types();
	test_get_default_provider();
	test_set_provider_ex();
}
