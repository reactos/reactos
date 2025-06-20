/*
 * Unit test for bcrypt functions
 *
 * Copyright 2014 Bruno Jesus
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
#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>

#include "wine/test.h"

static NTSTATUS (WINAPI *pBCryptOpenAlgorithmProvider)(BCRYPT_ALG_HANDLE *, LPCWSTR, LPCWSTR, ULONG);
static NTSTATUS (WINAPI *pBCryptCloseAlgorithmProvider)(BCRYPT_ALG_HANDLE, ULONG);
static NTSTATUS (WINAPI *pBCryptGetFipsAlgorithmMode)(BOOLEAN *);
static NTSTATUS (WINAPI *pBCryptCreateHash)(BCRYPT_ALG_HANDLE, BCRYPT_HASH_HANDLE *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptHash)(BCRYPT_ALG_HANDLE, UCHAR *, ULONG, UCHAR *, ULONG, UCHAR *, ULONG);
static NTSTATUS (WINAPI *pBCryptHashData)(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptFinishHash)(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptDestroyHash)(BCRYPT_HASH_HANDLE);
static NTSTATUS (WINAPI *pBCryptGenRandom)(BCRYPT_ALG_HANDLE, PUCHAR, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptGetProperty)(BCRYPT_HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG *, ULONG);
static NTSTATUS (WINAPI *pBCryptKeyDerivation)(BCRYPT_KEY_HANDLE,
        BCryptBufferDesc *, UCHAR *, ULONG, ULONG *, ULONG);
static NTSTATUS (WINAPI *pBCryptDeriveKeyPBKDF2)(BCRYPT_ALG_HANDLE, PUCHAR, ULONG, PUCHAR, ULONG, ULONGLONG, PUCHAR, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptSetProperty)(BCRYPT_HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptGenerateSymmetricKey)(BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptEncrypt)(BCRYPT_KEY_HANDLE, PUCHAR, ULONG, VOID *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG *, ULONG);
static NTSTATUS (WINAPI *pBCryptDuplicateKey)(BCRYPT_KEY_HANDLE, BCRYPT_KEY_HANDLE *, PUCHAR, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptDestroyKey)(BCRYPT_KEY_HANDLE);
static NTSTATUS (WINAPI *pBCryptDecrypt)(BCRYPT_KEY_HANDLE, PUCHAR, ULONG, VOID *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG *, ULONG);
static NTSTATUS (WINAPI *pBCryptImportKey)(BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE, LPCWSTR, BCRYPT_KEY_HANDLE *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptExportKey)(BCRYPT_KEY_HANDLE, BCRYPT_KEY_HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG *, ULONG);
static NTSTATUS (WINAPI *pBCryptImportKeyPair)(BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE, LPCWSTR, BCRYPT_KEY_HANDLE *, UCHAR *, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptVerifySignature)(BCRYPT_KEY_HANDLE, void *, UCHAR *, ULONG, UCHAR *, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptGenerateKeyPair)(BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE *, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptFinalizeKeyPair)(BCRYPT_KEY_HANDLE, ULONG);
static NTSTATUS (WINAPI *pBCryptSignHash)(BCRYPT_KEY_HANDLE, void *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG *, ULONG);
static NTSTATUS (WINAPI *pBCryptSecretAgreement)(BCRYPT_KEY_HANDLE, BCRYPT_KEY_HANDLE, BCRYPT_SECRET_HANDLE *, ULONG);
static NTSTATUS (WINAPI *pBCryptDeriveKey)(BCRYPT_SECRET_HANDLE, LPCWSTR, BCryptBufferDesc*, PUCHAR, ULONG, ULONG *, ULONG);
static NTSTATUS (WINAPI *pBCryptDestroySecret)(BCRYPT_SECRET_HANDLE);
static NTSTATUS (WINAPI *pBCryptEnumAlgorithms)(ULONG, ULONG *, BCRYPT_ALGORITHM_IDENTIFIER **, ULONG);
static void (WINAPI *pBCryptFreeBuffer)(void *);
static NTSTATUS (WINAPI *pBCryptDeriveKeyCapi)(BCRYPT_HASH_HANDLE, BCRYPT_ALG_HANDLE, PUCHAR, ULONG, ULONG);

static void test_BCryptGenRandom(void)
{
    NTSTATUS ret;
    UCHAR buffer[256];

    ret = pBCryptGenRandom(NULL, NULL, 0, 0);
    ok(ret == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got 0x%x\n", ret);
    ret = pBCryptGenRandom(NULL, buffer, 0, 0);
    ok(ret == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got 0x%x\n", ret);
    ret = pBCryptGenRandom(NULL, buffer, sizeof(buffer), 0);
    ok(ret == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got 0x%x\n", ret);
    ret = pBCryptGenRandom(NULL, buffer, sizeof(buffer), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ok(ret == STATUS_SUCCESS, "Expected success, got 0x%x\n", ret);
    ret = pBCryptGenRandom(NULL, buffer, sizeof(buffer),
          BCRYPT_USE_SYSTEM_PREFERRED_RNG|BCRYPT_RNG_USE_ENTROPY_IN_BUFFER);
    ok(ret == STATUS_SUCCESS, "Expected success, got 0x%x\n", ret);
    ret = pBCryptGenRandom(NULL, NULL, sizeof(buffer), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got 0x%x\n", ret);

    /* Zero sized buffer should work too */
    ret = pBCryptGenRandom(NULL, buffer, 0, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ok(ret == STATUS_SUCCESS, "Expected success, got 0x%x\n", ret);

    /* Test random number generation - It's impossible for a sane RNG to return 8 zeros */
    memset(buffer, 0, 16);
    ret = pBCryptGenRandom(NULL, buffer, 8, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ok(ret == STATUS_SUCCESS, "Expected success, got 0x%x\n", ret);
    ok(memcmp(buffer, buffer + 8, 8), "Expected a random number, got 0\n");

    /* Test pseudo handle, which was introduced at the same time as BCryptHash */
    if (pBCryptHash)
    {
        ret = pBCryptGenRandom(BCRYPT_RNG_ALG_HANDLE, buffer, sizeof(buffer), 0);
        ok(ret == STATUS_SUCCESS, "Expected success, got 0x%x\n", ret);

        ret = pBCryptCloseAlgorithmProvider(BCRYPT_RNG_ALG_HANDLE, 0);
        ok(ret == STATUS_INVALID_HANDLE, "got 0x%x\n", ret);
    }
    else win_skip("BCryptGenRandom pseudo handles are not available\n");

}

static void test_BCryptGetFipsAlgorithmMode(void)
{
    static const WCHAR policyKeyVistaW[] = {
        'S','y','s','t','e','m','\\',
        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
        'C','o','n','t','r','o','l','\\',
        'L','s','a','\\',
        'F','I','P','S','A','l','g','o','r','i','t','h','m','P','o','l','i','c','y',0};
    static const WCHAR policyValueVistaW[] = {'E','n','a','b','l','e','d',0};
    static const WCHAR policyKeyXPW[] = {
        'S','y','s','t','e','m','\\',
        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
        'C','o','n','t','r','o','l','\\',
        'L','s','a',0};
    static const WCHAR policyValueXPW[] = {
        'F','I','P','S','A','l','g','o','r','i','t','h','m','P','o','l','i','c','y',0};
    HKEY hkey = NULL;
    BOOLEAN expected;
    BOOLEAN enabled;
    DWORD value, count[2] = {sizeof(value), sizeof(value)};
    NTSTATUS ret;

    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, policyKeyVistaW, &hkey) == ERROR_SUCCESS &&
        RegQueryValueExW(hkey, policyValueVistaW, NULL, NULL, (void *)&value, &count[0]) == ERROR_SUCCESS)
    {
        expected = !!value;
    }
      else if (RegOpenKeyW(HKEY_LOCAL_MACHINE, policyKeyXPW, &hkey) == ERROR_SUCCESS &&
               RegQueryValueExW(hkey, policyValueXPW, NULL, NULL, (void *)&value, &count[0]) == ERROR_SUCCESS)
    {
        expected = !!value;
    }
    else
    {
        expected = FALSE;
        todo_wine
        ok(0, "Neither XP or Vista key is present\n");
    }
    RegCloseKey(hkey);

    ret = pBCryptGetFipsAlgorithmMode(&enabled);
    ok(ret == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%x\n", ret);
    ok(enabled == expected, "expected result %d, got %d\n", expected, enabled);

    ret = pBCryptGetFipsAlgorithmMode(NULL);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got 0x%x\n", ret);
}

static void format_hash(const UCHAR *bytes, ULONG size, char *buf)
{
    ULONG i;
    buf[0] = '\0';
    for (i = 0; i < size; i++)
    {
        sprintf(buf + i * 2, "%02x", bytes[i]);
    }
    return;
}

#define test_object_length(a) _test_object_length(__LINE__,a)
static void _test_object_length(unsigned line, void *handle)
{
    NTSTATUS status;
    ULONG len, size;

    len = size = 0xdeadbeef;
    status = pBCryptGetProperty(NULL, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok_(__FILE__,line)(status == STATUS_INVALID_HANDLE, "BCryptGetProperty failed: 0x%x\n", status);

    len = size = 0xdeadbeef;
    status = pBCryptGetProperty(handle, NULL, (UCHAR *)&len, sizeof(len), &size, 0);
    ok_(__FILE__,line)(status == STATUS_INVALID_PARAMETER, "BCryptGetProperty failed: 0x%x\n", status);

    len = size = 0xdeadbeef;
    status = pBCryptGetProperty(handle, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), NULL, 0);
    ok_(__FILE__,line)(status == STATUS_INVALID_PARAMETER, "BCryptGetProperty failed: 0x%x\n", status);

    len = size = 0xdeadbeef;
    status = pBCryptGetProperty(handle, BCRYPT_OBJECT_LENGTH, NULL, sizeof(len), &size, 0);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "BCryptGetProperty failed: 0x%x\n", status);
    ok_(__FILE__,line)(size == sizeof(len), "got %u\n", size);

    len = size = 0xdeadbeef;
    status = pBCryptGetProperty(handle, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, 0, &size, 0);
    ok_(__FILE__,line)(status == STATUS_BUFFER_TOO_SMALL, "BCryptGetProperty failed: 0x%x\n", status);
    ok_(__FILE__,line)(len == 0xdeadbeef, "got %u\n", len);
    ok_(__FILE__,line)(size == sizeof(len), "got %u\n", size);

    len = size = 0xdeadbeef;
    status = pBCryptGetProperty(handle, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "BCryptGetProperty failed: 0x%x\n", status);
    ok_(__FILE__,line)(len != 0xdeadbeef, "len not set\n");
    ok_(__FILE__,line)(size == sizeof(len), "got %u\n", size);
}

#define test_hash_length(a,b) _test_hash_length(__LINE__,a,b)
static void _test_hash_length(unsigned line, void *handle, ULONG exlen)
{
    ULONG len = 0xdeadbeef, size = 0xdeadbeef;
    NTSTATUS status;

    status = pBCryptGetProperty(handle, BCRYPT_HASH_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "BCryptGetProperty failed: 0x%08x\n", status);
    ok_(__FILE__,line)(size == sizeof(len), "got %u\n", size);
    ok_(__FILE__,line)(len == exlen, "len = %u, expected %u\n", len, exlen);
}

#define test_alg_name(a,b) _test_alg_name(__LINE__,a,b)
static void _test_alg_name(unsigned line, void *handle, const WCHAR *exname)
{
    ULONG size = 0xdeadbeef;
    UCHAR buf[256];
    const WCHAR *name = (const WCHAR*)buf;
    NTSTATUS status;

    status = pBCryptGetProperty(handle, BCRYPT_ALGORITHM_NAME, buf, sizeof(buf), &size, 0);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "BCryptGetProperty failed: 0x%08x\n", status);
    ok_(__FILE__,line)(size == (lstrlenW(exname) + 1) * sizeof(WCHAR), "got %u\n", size);
    ok_(__FILE__,line)(!lstrcmpW(name, exname), "alg name = %s, expected %s\n", wine_dbgstr_w(name),
                       wine_dbgstr_w(exname));
}

struct hash_test
{
    const WCHAR *alg;
    unsigned hash_size;
    const char *hash;
    const char *hash2;
    const char *hmac_hash;
    const char *hmac_hash2;
};

static void test_hash(const struct hash_test *test)
{
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_HASH_HANDLE hash;
    UCHAR buf[512], buf_hmac[1024], hash_buf[128], hmac_hash[128];
    char str[512];
    NTSTATUS ret;
    ULONG len;

    alg = NULL;
    ret = pBCryptOpenAlgorithmProvider(&alg, test->alg, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    test_object_length(alg);
    test_hash_length(alg, test->hash_size);
    test_alg_name(alg, test->alg);

    hash = NULL;
    len = sizeof(buf);
    ret = pBCryptCreateHash(alg, &hash, buf, len, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
    ok(hash != NULL, "hash not set\n");

    ret = pBCryptHashData(hash, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);

    ret = pBCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);

    test_hash_length(hash, test->hash_size);
    test_alg_name(hash, test->alg);

    memset(hash_buf, 0, sizeof(hash_buf));
    ret = pBCryptFinishHash(hash, hash_buf, test->hash_size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
    format_hash( hash_buf, test->hash_size, str );
    ok(!strcmp(str, test->hash), "got %s\n", str);

    ret = pBCryptDestroyHash(hash);
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);

    hash = NULL;
    len = sizeof(buf);
    ret = pBCryptCreateHash(alg, &hash, buf, len, NULL, 0, BCRYPT_HASH_REUSABLE_FLAG);
    ok(ret == STATUS_SUCCESS || broken(ret == STATUS_INVALID_PARAMETER) /* < win8 */, "got 0x%08x\n", ret);
    if (ret == STATUS_SUCCESS)
    {
        ret = pBCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);

        memset(hash_buf, 0, sizeof(hash_buf));
        ret = pBCryptFinishHash(hash, hash_buf, test->hash_size, 0);
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
        format_hash( hash_buf, test->hash_size, str );
        ok(!strcmp(str, test->hash), "got %s\n", str);

        /* reuse it */
        ret = pBCryptHashData(hash, (UCHAR *)"tset", sizeof("tset"), 0);
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);

        memset(hash_buf, 0, sizeof(hash_buf));
        ret = pBCryptFinishHash(hash, hash_buf, test->hash_size, 0);
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
        format_hash( hash_buf, test->hash_size, str );
        ok(!strcmp(str, test->hash2), "got %s\n", str);

        ret = pBCryptDestroyHash(hash);
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
    }

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);

    alg = NULL;
    ret = pBCryptOpenAlgorithmProvider(&alg, test->alg, MS_PRIMITIVE_PROVIDER, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    hash = NULL;
    len = sizeof(buf_hmac);
    ret = pBCryptCreateHash(alg, &hash, buf_hmac, len, (UCHAR *)"key", sizeof("key"), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
    ok(hash != NULL, "hash not set\n");

    ret = pBCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    test_hash_length(hash, test->hash_size);
    test_alg_name(hash, test->alg);

    memset(hmac_hash, 0, sizeof(hmac_hash));
    ret = pBCryptFinishHash(hash, hmac_hash, test->hash_size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
    format_hash( hmac_hash, test->hash_size, str );
    ok(!strcmp(str, test->hmac_hash), "got %s\n", str);

    ret = pBCryptDestroyHash(hash);
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);

    hash = NULL;
    len = sizeof(buf_hmac);
    ret = pBCryptCreateHash(alg, &hash, buf_hmac, len, (UCHAR *)"key", sizeof("key"), BCRYPT_HASH_REUSABLE_FLAG);
    ok(ret == STATUS_SUCCESS || broken(ret == STATUS_INVALID_PARAMETER) /* < win8 */, "got 0x%x\n", ret);
    if (ret == STATUS_SUCCESS)
    {
        ret = pBCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);

        memset(hmac_hash, 0, sizeof(hmac_hash));
        ret = pBCryptFinishHash(hash, hmac_hash, test->hash_size, 0);
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
        format_hash( hmac_hash, test->hash_size, str );
        ok(!strcmp(str, test->hmac_hash), "got %s\n", str);

        /* reuse it */
        ret = pBCryptHashData(hash, (UCHAR *)"tset", sizeof("tset"), 0);
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);

        memset(hmac_hash, 0, sizeof(hmac_hash));
        ret = pBCryptFinishHash(hash, hmac_hash, test->hash_size, 0);
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
        format_hash( hmac_hash, test->hash_size, str );
        ok(!strcmp(str, test->hmac_hash2), "got %s\n", str);

        ret = pBCryptDestroyHash(hash);
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
    }

    ret = pBCryptDestroyHash(hash);
    ok(ret == STATUS_INVALID_PARAMETER, "got 0x%08x\n", ret);

    ret = pBCryptDestroyHash(NULL);
    ok(ret == STATUS_INVALID_PARAMETER, "got 0x%08x\n", ret);

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);

    if (pBCryptHash)
    {
        ret = pBCryptHash(BCRYPT_SHA1_ALG_HANDLE, NULL, 0, NULL, 0, hash_buf, 20);
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
        ret = pBCryptHash(BCRYPT_SHA1_ALG_HANDLE, NULL, 0, (UCHAR *)"test", sizeof("test"), NULL, 20);
        ok(ret == STATUS_INVALID_PARAMETER, "got 0x%08x\n", ret);
        ret = pBCryptHash(BCRYPT_SHA1_ALG_HANDLE, NULL, 0, (UCHAR *)"test", sizeof("test"), hash_buf, 21);
        ok(ret == STATUS_INVALID_PARAMETER, "got 0x%08x\n", ret);
        ret = pBCryptHash(BCRYPT_SHA1_ALG_HANDLE, NULL, 0, (UCHAR *)"test", sizeof("test"), hash_buf, 20);
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);

        ret = pBCryptCreateHash(BCRYPT_SHA1_ALG_HANDLE, &hash, NULL, 0, NULL, 0, 0);
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
        ret = pBCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
        ret = pBCryptFinishHash(hash, hash_buf, 21, 0);
        ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);
        ret = pBCryptFinishHash(hash, hash_buf, 20, 0);
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
        ret = pBCryptDestroyHash(hash);
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
    }
}

static void test_hashes(void)
{
    static const struct hash_test tests[] =
    {
        { L"SHA1", 20,
        "961fa64958818f767707072755d7018dcd278e94",
        "9314f62ff64197143c91fc86de37e9ae776a3fb8",
        "2472cf65d0e090618d769d3e46f0d9446cf212da",
        "b2d2ba8cfd714d474cf0d9622cc5d15e1f53d53f",
        },
        { L"SHA256", 32,
        "ceb73749c899693706ede1e30c9929b3fd5dd926163831c2fb8bd41e6efb1126",
        "ea0938c118a7b15954f41b85195f2b42aec3a9429c63f593cfa65c137ffaa986",
        "34c1aa473a4468a91d06e7cdbc75bc4f93b830ccfc2a47ffd74e8e6ed29e4c72",
        "55feb7052060bd99e33f36eb0982c7f4856eb6a84fbefe19a1afd9faafc3af6f",
        },
        { L"SHA384", 48,
        "62b21e90c9022b101671ba1f808f8631a8149f0f12904055839a35c1ca78ae53"
        "63eed1e743a692d70e0504b0cfd12ef9",
        "724db7c0bbc51ef1ac3fc793083fc54c0e5c423faec9b11378c01c236b19aaaf"
        "a45177ad055feaf003968cc40ece44c7",
        "4b3e6d6ff2da121790ab7e7b9247583e3a7eed2db5bd4dabc680303b1608f37d"
        "fdc836d96a704c03283bc05b4f6c5eb8",
        "03e1818e5c165a0e54619e513acb06c393e1a6cb0ddbb4036b5f29617b334642"
        "e6e0be8b214d8508595b17a8c4b4e7db",
        },
        { L"SHA512", 64,
        "d55ced17163bf5386f2cd9ff21d6fd7fe576a915065c24744d09cfae4ec84ee1"
        "ef6ef11bfbc5acce3639bab725b50a1fe2c204f8c820d6d7db0df0ecbc49c5ca",
        "7752d707b54d2b00e7d1c09120d189475b0fd2e31ebb988cf0a01fc8492ddc0b"
        "3ca9c9ca61d9d7d1fb65ca7665e87f043c1d5bc9f786f8345e951c2d91ac594f",
        "415fb6b10018ca03b38a1b1399c42ac0be5e8aceddb9a73103f5e543bf2d888f"
        "2eecf91373941f9315dd730a77937fa92444450fbece86f409d9cb5ec48c6513",
        "1487bcecba46ae677622fa499e4cb2f0fdf92f6f3427cba76382d537a06e49c3"
        "3e70a2fc1fc730092bf21128c3704cc6387f6dfbf7e2f9f315bbb894505a1205",
        },
        { L"MD2", 16,
        "1bb33606ba908912a84221109d29cd7e",
        "b9a6ad9323b17e2d0cd389dddd6ef78a",
        "7f05b0638d77f4a27f3a9c4d353cd648",
        "05980873e6bfdd05dd7b30078de7e42a",
        },
        { L"MD4", 16,
        "74b5db93c0b41e36ca7074338fc0b637",
        "a14a9ff2059a8c28f47b01e6bc48a1bf",
        "bc2e8ac4d8248ed21b8d26227a30ea3a",
        "b609db0eb4b8669db74f2c20099701e4",
        },
        { L"MD5", 16,
        "e2a3e68d23ce348b8f68b3079de3d4c9",
        "bcdd7ca574342aa9db0e212348eacb16",
        "7bda029b93fa8d817fcc9e13d6bdf092",
        "dd636ab8e9592c5088e57c37d44c5bb3",
        }
    };
    unsigned i;

    for(i = 0; i < ARRAY_SIZE(tests); i++)
        test_hash(tests+i);
}

static void test_BcryptHash(void)
{
    static const char expected[] =
        "e2a3e68d23ce348b8f68b3079de3d4c9";
    static const char expected_hmac[] =
        "7bda029b93fa8d817fcc9e13d6bdf092";
    BCRYPT_ALG_HANDLE alg;
    UCHAR md5[16], md5_hmac[16];
    char str[65];
    NTSTATUS ret;

    if (!pBCryptHash) /* < Win10 */
    {
        win_skip("BCryptHash is not available\n");
        return;
    }

    alg = NULL;
    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_MD5_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    test_hash_length(alg, 16);
    test_alg_name(alg, L"MD5");

    memset(md5, 0, sizeof(md5));
    ret = pBCryptHash(alg, NULL, 0, (UCHAR *)"test", sizeof("test"), md5, sizeof(md5));
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
    format_hash( md5, sizeof(md5), str );
    ok(!strcmp(str, expected), "got %s\n", str);

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);

    alg = NULL;
    memset(md5_hmac, 0, sizeof(md5_hmac));
    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_MD5_ALGORITHM, MS_PRIMITIVE_PROVIDER, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    ret = pBCryptHash(alg, (UCHAR *)"key", sizeof("key"), (UCHAR *)"test", sizeof("test"), md5_hmac, sizeof(md5_hmac));
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
    format_hash( md5_hmac, sizeof(md5_hmac), str );
    ok(!strcmp(str, expected_hmac), "got %s\n", str);

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);

    if (pBCryptHash)
    {
        memset(md5, 0, sizeof(md5));
        ret = pBCryptHash(BCRYPT_MD5_ALG_HANDLE, NULL, 0, (UCHAR *)"test", sizeof("test"), md5, sizeof(md5));
        ok(ret == STATUS_SUCCESS, "got 0x%08x\n", ret);
        format_hash( md5, sizeof(md5), str );
        ok(!strcmp(str, expected), "got %s\n", str);
    }
}

/* test vectors from RFC 6070 */
static UCHAR password[] = "password";
static UCHAR salt[] = "salt";
static UCHAR long_password[] = "passwordPASSWORDpassword";
static UCHAR long_salt[] = "saltSALTsaltSALTsaltSALTsaltSALTsalt";
static UCHAR password_NUL[] = "pass\0word";
static UCHAR salt_NUL[] = "sa\0lt";

static UCHAR dk1[] = "0c60c80f961f0e71f3a9b524af6012062fe037a6";
static UCHAR dk2[] = "ea6c014dc72d6f8ccd1ed92ace1d41f0d8de8957";
static UCHAR dk3[] = "4b007901b765489abead49d926f721d065a429c1";
static UCHAR dk4[] = "364dd6bc200ec7d197f1b85f4a61769010717124";
static UCHAR dk5[] = "3d2eec4fe41c849b80c8d83662c0e44a8b291a964cf2f07038";
static UCHAR dk6[] = "56fa6aa75548099dcc37d7f03425e0c3";
static UCHAR dk7[] = "8754c32c64b0f524fc50c00f788135de";

static const struct
{
    ULONG        pwd_len;
    ULONG        salt_len;
    ULONGLONG    iterations;
    ULONG        dk_len;
    UCHAR       *pwd;
    UCHAR       *salt;
    const UCHAR *dk;
} rfc6070[] =
{
    {  8,  4,        1, 20, password,      salt,      dk1 },
    {  8,  4,        2, 20, password,      salt,      dk2 },
    {  8,  4,     4096, 20, password,      salt,      dk3 },
    {  8,  4,  1000000, 20, password,      salt,      dk4 },
    { 24, 36,     4096, 25, long_password, long_salt, dk5 },
    {  9,  5,     4096, 16, password_NUL,  salt_NUL,  dk6 },
    {  8,  0,        1, 16, password,      NULL,      dk7 }
};

static void test_BcryptDeriveKeyPBKDF2(void)
{
    BCRYPT_ALG_HANDLE alg;
    UCHAR buf[25];
    char str[51];
    NTSTATUS ret;
    ULONG i;

    alg = NULL;
    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA1_ALGORITHM, MS_PRIMITIVE_PROVIDER,
                                       BCRYPT_ALG_HANDLE_HMAC_FLAG);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(alg != NULL, "alg not set\n");

    test_hash_length(alg, 20);
    test_alg_name(alg, L"SHA1");

    ret = pBCryptDeriveKeyPBKDF2(alg, rfc6070[0].pwd, rfc6070[0].pwd_len, rfc6070[0].salt, rfc6070[0].salt_len,
                                 0, buf, rfc6070[0].dk_len, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);

    for (i = 0; i < ARRAY_SIZE(rfc6070); i++)
    {
        memset(buf, 0, sizeof(buf));
        ret = pBCryptDeriveKeyPBKDF2(alg, rfc6070[i].pwd, rfc6070[i].pwd_len, rfc6070[i].salt, rfc6070[i].salt_len,
                                     rfc6070[i].iterations, buf, rfc6070[i].dk_len, 0);
        ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
        format_hash(buf, rfc6070[i].dk_len, str);
        ok(!memcmp(str, rfc6070[i].dk, rfc6070[i].dk_len), "got %s\n", str);
    }

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    if (pBCryptHash)
    {
        ret = pBCryptDeriveKeyPBKDF2(BCRYPT_HMAC_SHA1_ALG_HANDLE, rfc6070[0].pwd, rfc6070[0].pwd_len, rfc6070[0].salt,
                                    rfc6070[0].salt_len, 1, buf, rfc6070[0].dk_len, 0);
        ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    }
}

static void test_rng(void)
{
    BCRYPT_ALG_HANDLE alg;
    ULONG size, len;
    UCHAR buf[16];
    NTSTATUS ret;

    alg = NULL;
    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_RNG_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(alg != NULL, "alg not set\n");

    len = size = 0xdeadbeef;
    ret = pBCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got 0x%x\n", ret);

    len = size = 0xdeadbeef;
    ret = pBCryptGetProperty(alg, BCRYPT_HASH_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got 0x%x\n", ret);

    test_alg_name(alg, L"RNG");

    memset(buf, 0, 16);
    ret = pBCryptGenRandom(alg, buf, 8, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(memcmp(buf, buf + 8, 8), "got zeroes\n");

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
}

static void test_aes(void)
{
    BCRYPT_KEY_LENGTHS_STRUCT key_lengths;
    BCRYPT_ALG_HANDLE alg;
    ULONG size, len;
    UCHAR mode[64];
    NTSTATUS ret;

    alg = NULL;
    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(alg != NULL, "alg not set\n");

    len = size = 0;
    ret = pBCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(len, "expected non-zero len\n");
    ok(size == sizeof(len), "got %u\n", size);

    len = size = 0;
    ret = pBCryptGetProperty(alg, BCRYPT_BLOCK_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(len == 16, "got %u\n", len);
    ok(size == sizeof(len), "got %u\n", size);

    size = 0;
    ret = pBCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, 0, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 64, "got %u\n", size);

    size = 0;
    ret = pBCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, sizeof(mode) - 1, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 64, "got %u\n", size);

    size = 0;
    memset(mode, 0, sizeof(mode));
    ret = pBCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, sizeof(mode), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(!lstrcmpW((const WCHAR *)mode, BCRYPT_CHAIN_MODE_CBC), "got %s\n", wine_dbgstr_w((const WCHAR *)mode));
    ok(size == 64, "got %u\n", size);

    size = 0;
    memset(&key_lengths, 0, sizeof(key_lengths));
    ret = pBCryptGetProperty(alg, BCRYPT_KEY_LENGTHS, (UCHAR*)&key_lengths, sizeof(key_lengths), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == sizeof(key_lengths), "got %u\n", size);
    ok(key_lengths.dwMinLength == 128, "Expected 128, got %u\n", key_lengths.dwMinLength);
    ok(key_lengths.dwMaxLength == 256, "Expected 256, got %u\n", key_lengths.dwMaxLength);
    ok(key_lengths.dwIncrement == 64, "Expected 64, got %u\n", key_lengths.dwIncrement);

    ret = pBCryptSetProperty(alg, BCRYPT_CHAINING_MODE, (UCHAR *)BCRYPT_CHAIN_MODE_GCM, 0, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    size = 0;
    memset(mode, 0, sizeof(mode));
    ret = pBCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, sizeof(mode), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(!lstrcmpW((const WCHAR *)mode, BCRYPT_CHAIN_MODE_GCM), "got %s\n", wine_dbgstr_w((const WCHAR *)mode));
    ok(size == 64, "got %u\n", size);

    ret = pBCryptSetProperty(alg, BCRYPT_CHAINING_MODE, (UCHAR *)BCRYPT_CHAIN_MODE_CFB, 0, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    size = 0;
    memset(mode, 0, sizeof(mode));
    ret = pBCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, sizeof(mode), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(!lstrcmpW((const WCHAR *)mode, BCRYPT_CHAIN_MODE_CFB), "got %s\n", wine_dbgstr_w((const WCHAR *)mode));
    ok(size == 64, "got %u\n", size);

    test_alg_name(alg, L"AES");

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    if (pBCryptHash)
    {
        ret = pBCryptSetProperty(BCRYPT_AES_CBC_ALG_HANDLE, BCRYPT_CHAINING_MODE, (UCHAR *)BCRYPT_CHAIN_MODE_GCM, 0, 0);
        ok(ret == STATUS_ACCESS_DENIED, "got 0x%x\n", ret);

        size = 0;
        memset(mode, 0, sizeof(mode));
        ret = pBCryptGetProperty(BCRYPT_AES_CBC_ALG_HANDLE, BCRYPT_CHAINING_MODE, mode, sizeof(mode), &size, 0);
        ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
        ok(!lstrcmpW((const WCHAR *)mode, BCRYPT_CHAIN_MODE_CBC), "got %s\n", wine_dbgstr_w((const WCHAR *)mode));
        ok(size == 64, "got %u\n", size);
    }
}

static void test_3des(void)
{
    BCRYPT_KEY_LENGTHS_STRUCT key_lengths;
    BCRYPT_ALG_HANDLE alg;
    ULONG size, len;
    UCHAR mode[64];
    NTSTATUS ret;

    alg = NULL;
    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_3DES_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(alg != NULL, "alg not set\n");

    len = size = 0;
    ret = pBCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(len, "expected non-zero len\n");
    ok(size == sizeof(len), "got %u\n", size);

    len = size = 0;
    ret = pBCryptGetProperty(alg, BCRYPT_BLOCK_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(len == 8, "got %u\n", len);
    ok(size == sizeof(len), "got %u\n", size);

    size = 0;
    ret = pBCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, 0, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 64, "got %u\n", size);

    size = 0;
    ret = pBCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, sizeof(mode) - 1, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 64, "got %u\n", size);

    size = 0;
    memset(mode, 0, sizeof(mode));
    ret = pBCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, sizeof(mode), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(!lstrcmpW((const WCHAR *)mode, BCRYPT_CHAIN_MODE_CBC), "got %s\n", wine_dbgstr_w((const WCHAR *)mode));
    ok(size == 64, "got %u\n", size);

    size = 0;
    memset(&key_lengths, 0, sizeof(key_lengths));
    ret = pBCryptGetProperty(alg, BCRYPT_KEY_LENGTHS, (UCHAR*)&key_lengths, sizeof(key_lengths), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == sizeof(key_lengths), "got %u\n", size);
    ok(key_lengths.dwMinLength == 192, "Expected 192, got %u\n", key_lengths.dwMinLength);
    ok(key_lengths.dwMaxLength == 192, "Expected 192, got %u\n", key_lengths.dwMaxLength);
    ok(key_lengths.dwIncrement == 0, "Expected 0, got %u\n", key_lengths.dwIncrement);

    memcpy(mode, BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM));
    ret = pBCryptSetProperty(alg, BCRYPT_CHAINING_MODE, mode, 0, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got 0x%x\n", ret);

    test_alg_name(alg, L"3DES");

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
}

static void test_BCryptGenerateSymmetricKey(void)
{
    static UCHAR secret[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static UCHAR iv[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static UCHAR data[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static UCHAR expected[] =
        {0xc6,0xa1,0x3b,0x37,0x87,0x8f,0x5b,0x82,0x6f,0x4f,0x81,0x62,0xa1,0xc8,0xd8,0x79};
    static UCHAR expected2[] =
        {0xb5,0x8a,0x10,0x64,0xd8,0xac,0xa9,0x9b,0xd9,0xb0,0x40,0x5b,0x85,0x45,0xf5,0xbb};
    static UCHAR expected3[] =
        {0xe3,0x7c,0xd3,0x63,0xdd,0x7c,0x87,0xa0,0x9a,0xff,0x0e,0x3e,0x60,0xe0,0x9c,0x82};
    BCRYPT_ALG_HANDLE aes;
    BCRYPT_KEY_HANDLE key, key2;
    UCHAR *buf, ciphertext[16], plaintext[16], ivbuf[16], mode[64];
    BCRYPT_KEY_LENGTHS_STRUCT key_lengths;
    ULONG size, len, len2, i;
    NTSTATUS ret;
    DWORD keylen;

    ret = pBCryptOpenAlgorithmProvider(&aes, BCRYPT_AES_ALGORITHM, NULL, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    len = size = 0xdeadbeef;
    ret = pBCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    len2 = len;

    key = (void *)0xdeadbeef;
    ret = pBCryptGenerateSymmetricKey(NULL, &key, NULL, 0, secret, sizeof(secret), 0);
    ok(ret == STATUS_INVALID_HANDLE, "got 0x%x\n", ret);
    ok(key == (void *)0xdeadbeef, "got %p\n", key);

    key = NULL;
    buf = calloc(1, len);

    key = (BCRYPT_KEY_HANDLE)0xdeadbeef;
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, 1, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);
    ok(key == (HANDLE)0xdeadbeef, "got unexpected key %p.\n", key);

    key = (BCRYPT_KEY_HANDLE)0xdeadbeef;
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret) + 1, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);
    ok(key == (HANDLE)0xdeadbeef, "got unexpected key %p.\n", key);

    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(key != NULL, "key not set\n");

    keylen = 0;
    ret = pBCryptGetProperty(key, BCRYPT_KEY_STRENGTH, (UCHAR *)&keylen, sizeof(keylen), &size, 0);
    ok(!ret, "got 0x%x\n", ret);
    ok(size == sizeof(keylen), "got %u\n", size);
    ok(keylen == 128, "got %u\n", keylen);

    ret = pBCryptSetProperty(aes, BCRYPT_CHAINING_MODE, (UCHAR *)BCRYPT_CHAIN_MODE_CBC,
                            sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    todo_wine
    {
    keylen = 512;
    ret = pBCryptSetProperty(aes, BCRYPT_KEY_LENGTH, (UCHAR *)&keylen, sizeof(keylen), 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got 0x%x\n", ret);
    }

    size = 0;
    memset(mode, 0, sizeof(mode));
    ret = pBCryptGetProperty(key, BCRYPT_CHAINING_MODE, mode, sizeof(mode), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(!lstrcmpW((const WCHAR *)mode, BCRYPT_CHAIN_MODE_CBC), "got %s\n", wine_dbgstr_w((const WCHAR *)mode));
    ok(size == 64, "got %u\n", size);

    ret = pBCryptSetProperty(key, BCRYPT_CHAINING_MODE, (UCHAR *)BCRYPT_CHAIN_MODE_ECB, 0, 0);
    ok(ret == STATUS_SUCCESS || broken(ret == STATUS_NOT_SUPPORTED) /* < Win 8 */, "got 0x%x\n", ret);
    if (ret == STATUS_SUCCESS)
    {
        size = 0;
        memset(mode, 0, sizeof(mode));
        ret = pBCryptGetProperty(key, BCRYPT_CHAINING_MODE, mode, sizeof(mode), &size, 0);
        ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
        ok(!lstrcmpW((const WCHAR *)mode, BCRYPT_CHAIN_MODE_ECB), "got %s\n", wine_dbgstr_w((const WCHAR *)mode));
        ok(size == 64, "got %u\n", size);
    }

    ret = pBCryptSetProperty(key, BCRYPT_CHAINING_MODE, (UCHAR *)BCRYPT_CHAIN_MODE_CBC,
                             sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    ok(ret == STATUS_SUCCESS || broken(ret == STATUS_NOT_SUPPORTED) /* < Win 8 */, "got 0x%x\n", ret);

    size = 0xdeadbeef;
    ret = pBCryptEncrypt(key, NULL, 0, NULL, NULL, 0, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(!size, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data, 16, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(ciphertext, expected, sizeof(expected)), "wrong data\n");
    for (i = 0; i < 16; i++)
        ok(ciphertext[i] == expected[i], "%u: %02x != %02x\n", i, ciphertext[i], expected[i]);
    ok(!memcmp(ivbuf, ciphertext, sizeof(iv)), "wrong iv data.\n");

    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 16, NULL, NULL, 0, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(ciphertext, expected2, sizeof(expected2)), "wrong data\n");

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ++ivbuf[0];
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(ciphertext, expected3, sizeof(expected3)), "wrong data\n");
    for (i = 0; i < 16; i++)
        ok(ciphertext[i] == expected3[i], "%u: %02x != %02x\n", i, ciphertext[i], expected3[i]);
    ok(!memcmp(ivbuf, ciphertext, sizeof(iv)), "wrong iv data.\n");

    key2 = (void *)0xdeadbeef;
    ret = pBCryptDuplicateKey(NULL, &key2, NULL, 0, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got 0x%x\n", ret);
    ok(key2 == (void *)0xdeadbeef, "got %p\n", key2);

    if (0) /* crashes on some Windows versions */
    {
        ret = pBCryptDuplicateKey(key, NULL, NULL, 0, 0);
        ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);
    }

    key2 = (void *)0xdeadbeef;
    ret = pBCryptDuplicateKey(key, &key2, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS || broken(ret == STATUS_INVALID_PARAMETER), "got 0x%x\n", ret);

    if (ret == STATUS_SUCCESS)
    {
        size = 0;
        memcpy(ivbuf, iv, sizeof(iv));
        memset(ciphertext, 0, sizeof(ciphertext));
        ret = pBCryptEncrypt(key2, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
        ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
        ok(size == 16, "got %u\n", size);
        ok(!memcmp(ciphertext, expected, sizeof(expected)), "wrong data\n");
        for (i = 0; i < 16; i++)
            ok(ciphertext[i] == expected[i], "%u: %02x != %02x\n", i, ciphertext[i], expected[i]);
        ok(!memcmp(ivbuf, ciphertext, sizeof(iv)), "wrong iv data.\n");

        ret = pBCryptDestroyKey(key2);
        ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    }

    size = 0xdeadbeef;
    ret = pBCryptDecrypt(key, NULL, 0, NULL, NULL, 0, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(!size, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext, 16, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext, 16, NULL, ivbuf, 16, plaintext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(plaintext, data, sizeof(data)), "wrong data\n");
    ok(!memcmp(ivbuf, ciphertext, sizeof(iv)), "wrong iv data.\n");

    memset(mode, 0, sizeof(mode));
    ret = pBCryptGetProperty(key, BCRYPT_CHAINING_MODE, mode, sizeof(mode), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(!lstrcmpW((const WCHAR *)mode, BCRYPT_CHAIN_MODE_CBC), "wrong mode\n");

    len = 0;
    size = 0;
    ret = pBCryptGetProperty(key, BCRYPT_BLOCK_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(len == 16, "got %u\n", len);
    ok(size == sizeof(len), "got %u\n", size);

    size = 0;
    memset(&key_lengths, 0, sizeof(key_lengths));
    ret = pBCryptGetProperty(aes, BCRYPT_KEY_LENGTHS, (UCHAR*)&key_lengths, sizeof(key_lengths), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == sizeof(key_lengths), "got %u\n", size);
    ok(key_lengths.dwMinLength == 128, "Expected 128, got %u\n", key_lengths.dwMinLength);
    ok(key_lengths.dwMaxLength == 256, "Expected 256, got %u\n", key_lengths.dwMaxLength);
    ok(key_lengths.dwIncrement == 64, "Expected 64, got %u\n", key_lengths.dwIncrement);

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    if (pBCryptHash)
    {
        ret = pBCryptGenerateSymmetricKey(BCRYPT_AES_CBC_ALG_HANDLE, &key, buf, len2, secret, sizeof(secret), 0);
        ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
        ret = pBCryptDestroyKey(key);
        ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    }

    free(buf);
    ret = pBCryptCloseAlgorithmProvider(aes, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
}

#define RACE_TEST_COUNT 200
static LONG encrypt_race_start_barrier;

static DWORD WINAPI encrypt_race_thread(void *parameter)
{
    static UCHAR nonce[] =
        {0x11,0x20,0x30,0x40,0x50,0x60,0x10,0x20,0x30,0x40,0x50,0x60};
    static UCHAR auth_data[] =
        {0x61,0x50,0x40,0x30,0x20,0x10,0x60,0x50,0x40,0x30,0x20,0x10};
    static UCHAR data2[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
         0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
    static UCHAR expected4[] =
        {0xb2,0x27,0x19,0x09,0xc7,0x89,0xdc,0x52,0x24,0x83,0x3a,0x55,0x34,0x76,0x2c,0xbf,
         0x15,0xa1,0xcb,0x40,0x78,0x11,0xba,0xbc,0xa4,0x76,0x69,0x7c,0x75,0x4f,0x11,0xba};
    static UCHAR expected_tag3[] =
        {0xef,0xee,0x75,0x99,0xb8,0x12,0xe9,0xf0,0xb4,0xcc,0x65,0x11,0x67,0x60,0x2d,0xe6};

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO auth_info;
    BCRYPT_KEY_HANDLE key = parameter;
    UCHAR ciphertext[48], tag[16];
    unsigned int i, test;
    NTSTATUS ret;
    ULONG size;

    memset(&auth_info, 0, sizeof(auth_info));
    auth_info.cbSize = sizeof(auth_info);
    auth_info.dwInfoVersion = 1;
    auth_info.pbNonce = nonce;
    auth_info.cbNonce = sizeof(nonce);
    auth_info.pbTag = tag;
    auth_info.cbTag = sizeof(tag);
    auth_info.pbAuthData = auth_data;
    auth_info.cbAuthData = sizeof(auth_data);

    InterlockedIncrement(&encrypt_race_start_barrier);
    while (InterlockedCompareExchange(&encrypt_race_start_barrier, 3, 2) != 2)
        ;

    for (test = 0; test < RACE_TEST_COUNT; ++test)
    {
        size = 0;
        memset(ciphertext, 0xff, sizeof(ciphertext));
        memset(tag, 0xff, sizeof(tag));
        ret = pBCryptEncrypt(key, data2, 32, &auth_info, NULL, 0, ciphertext, 32, &size, 0);
        ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
        ok(size == 32, "got %u\n", size);
        ok(!memcmp(ciphertext, expected4, sizeof(expected4)), "wrong data\n");
        ok(!memcmp(tag, expected_tag3, sizeof(expected_tag3)), "wrong tag\n");
        for (i = 0; i < 32; i++)
            ok(ciphertext[i] == expected4[i], "%u: %02x != %02x\n", i, ciphertext[i], expected4[i]);
        for (i = 0; i < 16; i++)
            ok(tag[i] == expected_tag3[i], "%u: %02x != %02x\n", i, tag[i], expected_tag3[i]);
    }

    return 0;
}

static void test_BCryptEncrypt(void)
{
    static UCHAR nonce[] =
        {0x10,0x20,0x30,0x40,0x50,0x60,0x10,0x20,0x30,0x40,0x50,0x60};
    static UCHAR auth_data[] =
        {0x60,0x50,0x40,0x30,0x20,0x10,0x60,0x50,0x40,0x30,0x20,0x10};
    static UCHAR secret[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static UCHAR secret256[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
         0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00};
    static UCHAR iv[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static UCHAR data[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10};
    static UCHAR data2[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
         0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
    static UCHAR expected[] =
        {0xc6,0xa1,0x3b,0x37,0x87,0x8f,0x5b,0x82,0x6f,0x4f,0x81,0x62,0xa1,0xc8,0xd8,0x79};
    static UCHAR expected2[] =
        {0xc6,0xa1,0x3b,0x37,0x87,0x8f,0x5b,0x82,0x6f,0x4f,0x81,0x62,0xa1,0xc8,0xd8,0x79,
         0x28,0x73,0x3d,0xef,0x84,0x8f,0xb0,0xa6,0x5d,0x1a,0x51,0xb7,0xec,0x8f,0xea,0xe9};
    static UCHAR expected3[] =
        {0xc6,0xa1,0x3b,0x37,0x87,0x8f,0x5b,0x82,0x6f,0x4f,0x81,0x62,0xa1,0xc8,0xd8,0x79,
         0xb1,0xa2,0x92,0x73,0xbe,0x2c,0x42,0x07,0xa5,0xac,0xe3,0x93,0x39,0x8c,0xb6,0xfb,
         0x87,0x5d,0xea,0xa3,0x7e,0x0f,0xde,0xfa,0xd9,0xec,0x6c,0x4e,0x3c,0x76,0x86,0xe4};
    static UCHAR expected4[] =
        {0xe1,0x82,0xc3,0xc0,0x24,0xfb,0x86,0x85,0xf3,0xf1,0x2b,0x7d,0x09,0xb4,0x73,0x67,
         0x86,0x64,0xc3,0xfe,0xa3,0x07,0x61,0xf8,0x16,0xc9,0x78,0x7f,0xe7,0xb1,0xc4,0x94};
    static UCHAR expected5[] =
        {0x0a,0x94,0x0b,0xb5,0x41,0x6e,0xf0,0x45,0xf1,0xc3,0x94,0x58,0xc6,0x53,0xea,0x5a};
    static UCHAR expected6[] =
        {0x0a,0x94,0x0b,0xb5,0x41,0x6e,0xf0,0x45,0xf1,0xc3,0x94,0x58,0xc6,0x53,0xea,0x5a,
         0x84,0x07,0x66,0xb7,0x49,0xc0,0x9b,0x49,0x74,0x28,0x8c,0x10,0xb9,0xc2,0x09,0x70};
    static UCHAR expected7[] =
        {0x0a,0x94,0x0b,0xb5,0x41,0x6e,0xf0,0x45,0xf1,0xc3,0x94,0x58,0xc6,0x53,0xea,0x5a,
         0x95,0x4f,0x64,0xf2,0xe4,0xe8,0x6e,0x9e,0xee,0x82,0xd2,0x02,0x16,0x68,0x48,0x99,
         0x95,0x4f,0x64,0xf2,0xe4,0xe8,0x6e,0x9e,0xee,0x82,0xd2,0x02,0x16,0x68,0x48,0x99};
    static UCHAR expected8[] =
        {0xb5,0x8a,0x10,0x64,0xd8,0xac,0xa9,0x9b,0xd9,0xb0,0x40,0x5b,0x85,0x45,0xf5,0xbb};
    static UCHAR expected9[] =
        {0x0a,0x94,0x0b,0xb5,0x41,0x6e,0xf0,0x45,0xf1,0xc3,0x94,0x58,0xc6,0x53,0xea,0x5a};
    static UCHAR expected10[] =
        {0x66,0xb8,0xbd,0xe5,0x90,0x6c,0xec,0xdf,0xfa,0x8a,0xb2,0xfd,0x92,0x84,0xeb,0xf0,
         0x95,0xc4,0xdf,0xa7,0x7a,0x62,0xe4,0xab,0xd4,0x0e,0x94,0x4e,0xd7,0x6e,0xa1,0x47,
         0x29,0x4b,0x37,0xfe,0x28,0x6d,0x5f,0x69,0x46,0x30,0x73,0xc0,0xaa,0x42,0xe4,0x46};
    static UCHAR expected11[] =
        {0x0a,0x22,0xf7,0x96,0xe1,0xb9,0x3e,0x90,0x32,0xcf,0xf8,0x04,0x83,0x8a,0xdf,0xc3};
    static UCHAR expected12[] =
        {0xb5,0x9d,0xe7,0x8e,0x0f,0x80,0x16,0xc3,0xf6,0x80,0xad,0xf8,0xac,0xef,0xf9,0x9f};
    static UCHAR expected13[] =
        {0xc6,0x41,0x37,0xd8,0x6f,0x65,0x64,0x1d,0x2b,0xa0,0xad,0xee,0x67,0xba,0x9b,0x5f};
    static UCHAR expected14[] =
        {0x0a,0x22,0xf7,0x96,0xe1,0xb9,0x3e,0x90,0x32,0xcf,0xf8,0x04,0x83,0x8a,0xdf,0xc3,
         0xa5,0xfa,0x54,0xd7,0x62,0x9d,0x8b,0x11,0x3d,0xd8,0xe2,0x95,0xc2,0x23,0x3f,0x29};
    static UCHAR expected15[] =
        {0x0a,0x22,0xf7,0x96,0xe1,0xb9,0x3e,0x90,0x32,0xcf,0xf8,0x04,0x83,0x8a,0xdf,0xc3,
         0xa5,0xe5,0xcd,0x29,0x48,0x37,0x3f,0x92,0x6f,0x09,0x84,0x5a,0xb8,0x88,0xda,0xfd,
         0x65,0x00,0x20,0x9a,0xc6,0xdb,0xfa,0x82,0xdb,0xbc,0xf4,0x70,0x7d,0x88,0x86,0xb5};
    static UCHAR expected16[] =
        {0x64,0xff,0xc6,0x97,0x14,0x5e,0x21,0xfa,0xe8,0x6e,0xba,0xc1,0x0c,0xb1,0x72,0x64,
         0x5d,0xf0,0x22,0x72,0xef,0x39,0xe3,0x2f,0x80,0x3e,0x53,0x00,0xc1,0xcf,0x3f,0xbb,
         0xea,0x30,0x01,0x78,0x3b,0xba,0x4d,0xca,0xa0,0x2e,0x83,0x5e,0x0e,0xe2,0x65,0xcb};
    static UCHAR expected_tag[] =
        {0x89,0xb3,0x92,0x00,0x39,0x20,0x09,0xb4,0x6a,0xd6,0xaf,0xca,0x4b,0x5b,0xfd,0xd0};
    static UCHAR expected_tag2[] =
        {0x9a,0x92,0x32,0x2c,0x61,0x2a,0xae,0xef,0x66,0x2a,0xfb,0x55,0xe9,0x48,0xdf,0xbd};
    static UCHAR expected_tag3[] =
        {0x17,0x9d,0xc0,0x7a,0xf0,0xcf,0xaa,0xd5,0x1c,0x11,0xc4,0x4b,0xd6,0xa3,0x3e,0x77};
    static UCHAR expected_tag4[] =
        {0x4c,0x42,0x83,0x9e,0x8d,0x40,0xf1,0x19,0xd6,0x2b,0x1c,0x66,0x03,0x2b,0x39,0x63};

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO auth_info;
    UCHAR *buf, ciphertext[48], ivbuf[16], tag[16];
    BCRYPT_AUTH_TAG_LENGTHS_STRUCT tag_length;
    ULONG size, len, i, test;
    BCRYPT_ALG_HANDLE aes;
    BCRYPT_KEY_HANDLE key;
    HANDLE hthread;
    NTSTATUS ret;

    ret = pBCryptOpenAlgorithmProvider(&aes, BCRYPT_AES_ALGORITHM, NULL, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    /******************
     * AES - CBC mode *
     ******************/

    len = 0xdeadbeef;
    size = sizeof(len);
    ret = pBCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    key = NULL;
    buf = calloc(1, len);
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(key != NULL, "key not set\n");

    /* input size is a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data, 16, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(ciphertext, expected, sizeof(expected)), "wrong data\n");
    for (i = 0; i < 16; i++)
        ok(ciphertext[i] == expected[i], "%u: %02x != %02x\n", i, ciphertext[i], expected[i]);
    ok(!memcmp(ivbuf, ciphertext, sizeof(iv)), "wrong iv data.\n");

    /* NULL initialization vector */
    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 16, NULL, NULL, 0, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(ciphertext, expected8, sizeof(expected8)), "wrong data\n");

    /* all zero initialization vector */
    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    memset(ivbuf, 0, sizeof(ivbuf));
    ret = pBCryptEncrypt(key, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(ciphertext, expected9, sizeof(expected9)), "wrong data\n");
    for (i = 0; i < 16; i++)
        ok(ciphertext[i] == expected9[i], "%u: %02x != %02x\n", i, ciphertext[i], expected9[i]);
    ok(!memcmp(ivbuf, ciphertext, sizeof(iv)), "wrong iv data.\n");

    /* input size is not a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data, 17, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got 0x%x\n", ret);
    ok(size == 17, "got %u\n", size);

    /* input size is not a multiple of block size, block padding set */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data, 17, NULL, ivbuf, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 17, NULL, ivbuf, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ciphertext, expected2, sizeof(expected2)), "wrong data\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected2[i], "%u: %02x != %02x\n", i, ciphertext[i], expected2[i]);
    ok(!memcmp(ivbuf, ciphertext + 32 - 16, sizeof(iv)), "wrong iv data.\n");

    /* input size is a multiple of block size, block padding set */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, ciphertext, 48, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);
    ok(!memcmp(ciphertext, expected3, sizeof(expected3)), "wrong data\n");
    for (i = 0; i < 48; i++)
        ok(ciphertext[i] == expected3[i], "%u: %02x != %02x\n", i, ciphertext[i], expected3[i]);
    ok(!memcmp(ivbuf, ciphertext + 48 - 16, sizeof(iv)), "wrong iv data.\n");

    /* output size too small */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 17, NULL, ivbuf, 16, ciphertext, 31, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    free(buf);

    /* 256 bit key */
    buf = calloc(1, len);

    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret256, sizeof(secret256), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    /* Key generations succeeds if the key size exceeds maximum and uses maximum key length
     * from secret. */
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret256, sizeof(secret256) + 1, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, ciphertext, 48, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);
    ok(!memcmp(ciphertext, expected10, sizeof(expected10)), "wrong data\n");
    for (i = 0; i < 48; i++)
        ok(ciphertext[i] == expected10[i], "%u: %02x != %02x\n", i, ciphertext[i], expected10[i]);
    ok(!memcmp(ivbuf, ciphertext + 48 - 16, sizeof(iv)), "wrong iv data.\n");

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    free(buf);

    /******************
     * AES - GCM mode *
     ******************/

    size = 0;
    ret = pBCryptGetProperty(aes, BCRYPT_AUTH_TAG_LENGTH, NULL, 0, &size, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got 0x%x\n", ret);

    ret = pBCryptSetProperty(aes, BCRYPT_CHAINING_MODE, (UCHAR*)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    size = 0;
    ret = pBCryptGetProperty(aes, BCRYPT_AUTH_TAG_LENGTH, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == sizeof(tag_length), "got %u\n", size);

    size = 0;
    memset(&tag_length, 0, sizeof(tag_length));
    ret = pBCryptGetProperty(aes, BCRYPT_AUTH_TAG_LENGTH, (UCHAR*)&tag_length, sizeof(tag_length), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == sizeof(tag_length), "got %u\n", size);
    ok(tag_length.dwMinLength == 12, "Expected 12, got %u\n", tag_length.dwMinLength);
    ok(tag_length.dwMaxLength == 16, "Expected 16, got %u\n", tag_length.dwMaxLength);
    ok(tag_length.dwIncrement == 1, "Expected 1, got %u\n", tag_length.dwIncrement);

    len = 0xdeadbeef;
    size = sizeof(len);
    ret = pBCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    key = NULL;
    buf = calloc(1, len);
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(key != NULL, "key not set\n");

    ret = pBCryptGetProperty(key, BCRYPT_AUTH_TAG_LENGTH, (UCHAR*)&tag_length, sizeof(tag_length), &size, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got 0x%x\n", ret);

    memset(&auth_info, 0, sizeof(auth_info));
    auth_info.cbSize = sizeof(auth_info);
    auth_info.dwInfoVersion = 1;
    auth_info.pbNonce = nonce;
    auth_info.cbNonce = sizeof(nonce);
    auth_info.pbTag = tag;
    auth_info.cbTag = sizeof(tag);

    /* input size is a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0xff, sizeof(ciphertext));
    memset(tag, 0xff, sizeof(tag));
    ret = pBCryptEncrypt(key, data2, 32, &auth_info, ivbuf, 16, ciphertext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ciphertext, expected4, sizeof(expected4)), "wrong data\n");
    ok(!memcmp(tag, expected_tag, sizeof(expected_tag)), "wrong tag\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected4[i], "%u: %02x != %02x\n", i, ciphertext[i], expected4[i]);
    for (i = 0; i < 16; i++)
        ok(tag[i] == expected_tag[i], "%u: %02x != %02x\n", i, tag[i], expected_tag[i]);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    /* NULL initialization vector */
    size = 0;
    memset(ciphertext, 0xff, sizeof(ciphertext));
    memset(tag, 0xff, sizeof(tag));
    ret = pBCryptEncrypt(key, data2, 32, &auth_info, NULL, 0, ciphertext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ciphertext, expected4, sizeof(expected4)), "wrong data\n");
    ok(!memcmp(tag, expected_tag, sizeof(expected_tag)), "wrong tag\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected4[i], "%u: %02x != %02x\n", i, ciphertext[i], expected4[i]);
    for (i = 0; i < 16; i++)
        ok(tag[i] == expected_tag[i], "%u: %02x != %02x\n", i, tag[i], expected_tag[i]);

    /* all zero initialization vector */
    size = 0;
    memset(ciphertext, 0xff, sizeof(ciphertext));
    memset(tag, 0xff, sizeof(tag));
    memset(ivbuf, 0, sizeof(ivbuf));
    ret = pBCryptEncrypt(key, data2, 32, &auth_info, ivbuf, 16, ciphertext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ciphertext, expected4, sizeof(expected4)), "wrong data\n");
    ok(!memcmp(tag, expected_tag, sizeof(expected_tag)), "wrong tag\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected4[i], "%u: %02x != %02x\n", i, ciphertext[i], expected4[i]);
    for (i = 0; i < 16; i++)
        ok(tag[i] == expected_tag[i], "%u: %02x != %02x\n", i, tag[i], expected_tag[i]);
    memset(ciphertext, 0, sizeof(iv));
    ok(!memcmp(ivbuf, ciphertext, sizeof(iv)), "wrong iv data.\n");

    /* input size is not multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0xff, sizeof(ciphertext));
    memset(tag, 0xff, sizeof(tag));
    ret = pBCryptEncrypt(key, data2, 24, &auth_info, ivbuf, 16, ciphertext, 24, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 24, "got %u\n", size);
    ok(!memcmp(ciphertext, expected4, 24), "wrong data\n");
    ok(!memcmp(tag, expected_tag2, sizeof(expected_tag2)), "wrong tag\n");
    for (i = 0; i < 24; i++)
        ok(ciphertext[i] == expected4[i], "%u: %02x != %02x\n", i, ciphertext[i], expected4[i]);
    for (i = 0; i < 16; i++)
        ok(tag[i] == expected_tag2[i], "%u: %02x != %02x\n", i, tag[i], expected_tag2[i]);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    /* test with auth data */
    auth_info.pbAuthData = auth_data;
    auth_info.cbAuthData = sizeof(auth_data);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0xff, sizeof(ciphertext));
    memset(tag, 0xff, sizeof(tag));
    ret = pBCryptEncrypt(key, data2, 32, &auth_info, ivbuf, 16, ciphertext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ciphertext, expected4, sizeof(expected4)), "wrong data\n");
    ok(!memcmp(tag, expected_tag3, sizeof(expected_tag3)), "wrong tag\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected4[i], "%u: %02x != %02x\n", i, ciphertext[i], expected4[i]);
    for (i = 0; i < 16; i++)
        ok(tag[i] == expected_tag3[i], "%u: %02x != %02x\n", i, tag[i], expected_tag3[i]);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    memset(tag, 0xff, sizeof(tag));
    ret = pBCryptEncrypt(key, data2, 0, &auth_info, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(!size, "got %u\n", size);
    for (i = 0; i < 16; i++)
        ok(tag[i] == 0xff, "%u: %02x != %02x\n", i, tag[i], 0xff);

    memset(tag, 0xff, sizeof(tag));
    ret = pBCryptEncrypt(key, NULL, 0, &auth_info, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(!size, "got %u\n", size);
    ok(!memcmp(tag, expected_tag4, sizeof(expected_tag4)), "wrong tag\n");
    for (i = 0; i < 16; i++)
        ok(tag[i] == expected_tag4[i], "%u: %02x != %02x\n", i, tag[i], expected_tag4[i]);

    /* test with padding */
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, &auth_info, ivbuf, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    todo_wine ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);

    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, &auth_info, ivbuf, 16, ciphertext, 48, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);

    /* race test */

    encrypt_race_start_barrier = 0;
    hthread = CreateThread(NULL, 0, encrypt_race_thread, key, 0, NULL);

    while (InterlockedCompareExchange(&encrypt_race_start_barrier, 2, 1) != 1)
        ;

    for (test = 0; test < RACE_TEST_COUNT; ++test)
    {
        size = 0;
        memset(ciphertext, 0xff, sizeof(ciphertext));
        memset(tag, 0xff, sizeof(tag));
        ret = pBCryptEncrypt(key, data2, 32, &auth_info, NULL, 0, ciphertext, 32, &size, 0);
        ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
        ok(size == 32, "got %u\n", size);
        ok(!memcmp(ciphertext, expected4, sizeof(expected4)), "wrong data\n");
        ok(!memcmp(tag, expected_tag3, sizeof(expected_tag2)), "wrong tag\n");
        for (i = 0; i < 32; i++)
            ok(ciphertext[i] == expected4[i], "%u: %02x != %02x\n", i, ciphertext[i], expected4[i]);
        for (i = 0; i < 16; i++)
            ok(tag[i] == expected_tag3[i], "%u: %02x != %02x\n", i, tag[i], expected_tag3[i]);
    }

    WaitForSingleObject(hthread, INFINITE);

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    free(buf);

    /******************
     * AES - ECB mode *
     ******************/

    ret = pBCryptSetProperty(aes, BCRYPT_CHAINING_MODE, (UCHAR*)BCRYPT_CHAIN_MODE_ECB, sizeof(BCRYPT_CHAIN_MODE_ECB), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    len = 0xdeadbeef;
    size = sizeof(len);
    ret = pBCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    buf = calloc(1, len);
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    /* initialization vector is not allowed */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);

    /* input size is a multiple of block size */
    size = 0;
    ret = pBCryptEncrypt(key, data, 16, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);

    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 16, NULL, NULL, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(ciphertext, expected5, sizeof(expected5)), "wrong data\n");
    for (i = 0; i < 16; i++)
        ok(ciphertext[i] == expected5[i], "%u: %02x != %02x\n", i, ciphertext[i], expected5[i]);

    /* input size is not a multiple of block size */
    size = 0;
    ret = pBCryptEncrypt(key, data, 17, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got 0x%x\n", ret);
    ok(size == 17, "got %u\n", size);

    /* input size is not a multiple of block size, block padding set */
    size = 0;
    ret = pBCryptEncrypt(key, data, 17, NULL, NULL, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 17, NULL, NULL, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ciphertext, expected6, sizeof(expected6)), "wrong data\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected6[i], "%u: %02x != %02x\n", i, ciphertext[i], expected6[i]);

    /* input size is a multiple of block size, block padding set */
    size = 0;
    ret = pBCryptEncrypt(key, data2, 32, NULL, NULL, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);

    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, NULL, NULL, 16, ciphertext, 48, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);
    ok(!memcmp(ciphertext, expected7, sizeof(expected7)), "wrong data\n");
    for (i = 0; i < 48; i++)
        ok(ciphertext[i] == expected7[i], "%u: %02x != %02x\n", i, ciphertext[i], expected7[i]);

    /* output size too small */
    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 17, NULL, NULL, 16, ciphertext, 31, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, NULL, NULL, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    free(buf);

    /******************
     * AES - CFB mode *
     ******************/

    ret = pBCryptSetProperty(aes, BCRYPT_CHAINING_MODE, (UCHAR *)BCRYPT_CHAIN_MODE_CFB, sizeof(BCRYPT_CHAIN_MODE_CFB), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    key = NULL;
    buf = calloc(1, len);
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(key != NULL, "key not set\n");

    /* input size is a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data, 16, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(ciphertext, expected11, sizeof(expected11)), "wrong data\n");
    for (i = 0; i < 16; i++)
        ok(ciphertext[i] == expected11[i], "%u: %02x != %02x\n", i, ciphertext[i], expected11[i]);
    ok(!memcmp(ivbuf, ciphertext, sizeof(iv)), "wrong iv data.\n");

    /* NULL initialization vector */
    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 16, NULL, NULL, 0, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(ciphertext, expected12, sizeof(expected12)), "wrong data\n");

    /* all zero initialization vector */
    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    memset(ivbuf, 0, sizeof(ivbuf));
    ret = pBCryptEncrypt(key, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(ciphertext, expected13, sizeof(expected13)), "wrong data\n");
    for (i = 0; i < 16; i++)
        ok(ciphertext[i] == expected13[i], "%u: %02x != %02x\n", i, ciphertext[i], expected13[i]);
    ok(!memcmp(ivbuf, ciphertext, sizeof(iv)), "wrong iv data.\n");

    /* input size is not a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data, 17, NULL, ivbuf, 16, NULL, 0, &size, 0);
    todo_wine ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 17, "got %u\n", size);

    /* input size is not a multiple of block size, block padding set */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data, 17, NULL, ivbuf, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 17, NULL, ivbuf, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ciphertext, expected14, sizeof(expected14)), "wrong data\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected14[i], "%u: %02x != %02x\n", i, ciphertext[i], expected14[i]);
    ok(!memcmp(ivbuf, ciphertext + 32 - 16, sizeof(iv)), "wrong iv data.\n");

    /* input size is a multiple of block size, block padding set */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, ciphertext, 48, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);
    ok(!memcmp(ciphertext, expected15, sizeof(expected15)), "wrong data\n");
    for (i = 0; i < 48; i++)
        ok(ciphertext[i] == expected15[i], "%u: %02x != %02x\n", i, ciphertext[i], expected15[i]);
    ok(!memcmp(ivbuf, ciphertext + 48 - 16, sizeof(iv)), "wrong iv data.\n");

    /* output size too small */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 17, NULL, ivbuf, 16, ciphertext, 31, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    free(buf);

    /* 256 bit key */
    buf = calloc(1, len);

    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret256, sizeof(secret256), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    /* Key generations succeeds if the key size exceeds maximum and uses maximum key length
     * from secret. */
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret256, sizeof(secret256) + 1, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, ciphertext, 48, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);
    ok(!memcmp(ciphertext, expected16, sizeof(expected16)), "wrong data\n");
    for (i = 0; i < 48; i++)
        ok(ciphertext[i] == expected16[i], "%u: %02x != %02x\n", i, ciphertext[i], expected16[i]);
    ok(!memcmp(ivbuf, ciphertext + 48 - 16, sizeof(iv)), "wrong iv data.\n");

    ret = pBCryptCloseAlgorithmProvider(aes, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
}

static void test_BCryptDecrypt(void)
{
    static UCHAR nonce[] =
        {0x10,0x20,0x30,0x40,0x50,0x60,0x10,0x20,0x30,0x40,0x50,0x60};
    static UCHAR auth_data[] =
        {0x60,0x50,0x40,0x30,0x20,0x10,0x60,0x50,0x40,0x30,0x20,0x10};
    static UCHAR secret[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static UCHAR iv[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static UCHAR expected[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static UCHAR expected2[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10};
    static UCHAR expected3[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
         0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
    static UCHAR expected4[] =
        {0x28,0x73,0x3d,0xef,0x84,0x8f,0xb0,0xa6,0x5d,0x1a,0x51,0xb7,0xec,0x8f,0xea,0xe9,
         0x10,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f};
    static UCHAR expected5[] =
        {0x29,0x73,0x3d,0xef,0x84,0x8f,0xb0,0xa6,0x5d,0x1a,0x51,0xb7,0xec,0x8f,0xea,0xe9,
         0x10,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f};
    static UCHAR ciphertext[32] =
        {0xc6,0xa1,0x3b,0x37,0x87,0x8f,0x5b,0x82,0x6f,0x4f,0x81,0x62,0xa1,0xc8,0xd8,0x79,
         0x28,0x73,0x3d,0xef,0x84,0x8f,0xb0,0xa6,0x5d,0x1a,0x51,0xb7,0xec,0x8f,0xea,0xe9};
    static UCHAR ciphertext2[] =
        {0xc6,0xa1,0x3b,0x37,0x87,0x8f,0x5b,0x82,0x6f,0x4f,0x81,0x62,0xa1,0xc8,0xd8,0x79,
         0x28,0x73,0x3d,0xef,0x84,0x8f,0xb0,0xa6,0x5d,0x1a,0x51,0xb7,0xec,0x8f,0xea,0xe9};
    static UCHAR ciphertext3[] =
        {0xc6,0xa1,0x3b,0x37,0x87,0x8f,0x5b,0x82,0x6f,0x4f,0x81,0x62,0xa1,0xc8,0xd8,0x79,
         0xb1,0xa2,0x92,0x73,0xbe,0x2c,0x42,0x07,0xa5,0xac,0xe3,0x93,0x39,0x8c,0xb6,0xfb,
         0x87,0x5d,0xea,0xa3,0x7e,0x0f,0xde,0xfa,0xd9,0xec,0x6c,0x4e,0x3c,0x76,0x86,0xe4};
    static UCHAR ciphertext4[] =
        {0xe1,0x82,0xc3,0xc0,0x24,0xfb,0x86,0x85,0xf3,0xf1,0x2b,0x7d,0x09,0xb4,0x73,0x67,
         0x86,0x64,0xc3,0xfe,0xa3,0x07,0x61,0xf8,0x16,0xc9,0x78,0x7f,0xe7,0xb1,0xc4,0x94};
    static UCHAR ciphertext5[] =
        {0x0a,0x94,0x0b,0xb5,0x41,0x6e,0xf0,0x45,0xf1,0xc3,0x94,0x58,0xc6,0x53,0xea,0x5a,
         0x84,0x07,0x66,0xb7,0x49,0xc0,0x9b,0x49,0x74,0x28,0x8c,0x10,0xb9,0xc2,0x09,0x70};
    static UCHAR ciphertext6[] =
        {0x0a,0x94,0x0b,0xb5,0x41,0x6e,0xf0,0x45,0xf1,0xc3,0x94,0x58,0xc6,0x53,0xea,0x5a,
         0x95,0x4f,0x64,0xf2,0xe4,0xe8,0x6e,0x9e,0xee,0x82,0xd2,0x02,0x16,0x68,0x48,0x99,
         0x95,0x4f,0x64,0xf2,0xe4,0xe8,0x6e,0x9e,0xee,0x82,0xd2,0x02,0x16,0x68,0x48,0x99};
    static UCHAR tag[] =
        {0x89,0xb3,0x92,0x00,0x39,0x20,0x09,0xb4,0x6a,0xd6,0xaf,0xca,0x4b,0x5b,0xfd,0xd0};
    static UCHAR tag2[] =
        {0x17,0x9d,0xc0,0x7a,0xf0,0xcf,0xaa,0xd5,0x1c,0x11,0xc4,0x4b,0xd6,0xa3,0x3e,0x77};
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO auth_info;
    BCRYPT_KEY_LENGTHS_STRUCT key_lengths;
    BCRYPT_AUTH_TAG_LENGTHS_STRUCT tag_lengths;
    BCRYPT_ALG_HANDLE aes;
    BCRYPT_KEY_HANDLE key;
    UCHAR *buf, plaintext[48], ivbuf[16];
    ULONG size, len;
    NTSTATUS ret;

    ret = pBCryptOpenAlgorithmProvider(&aes, BCRYPT_AES_ALGORITHM, NULL, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    size = 0;
    memset(&key_lengths, 0, sizeof(key_lengths));
    ret = pBCryptGetProperty(aes, BCRYPT_KEY_LENGTHS, (UCHAR*)&key_lengths, sizeof(key_lengths), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == sizeof(key_lengths), "got %u\n", size);
    ok(key_lengths.dwMinLength == 128, "Expected 128, got %u\n", key_lengths.dwMinLength);
    ok(key_lengths.dwMaxLength == 256, "Expected 256, got %u\n", key_lengths.dwMaxLength);
    ok(key_lengths.dwIncrement == 64, "Expected 64, got %u\n", key_lengths.dwIncrement);

    /******************
     * AES - CBC mode *
     ******************/

    len = 0xdeadbeef;
    size = sizeof(len);
    ret = pBCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    key = NULL;
    buf = calloc(1, len);
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(key != NULL, "key not set\n");

    /* input size is a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext, 32, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext, 32, NULL, ivbuf, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(plaintext, expected, sizeof(expected)), "wrong data\n");
    ok(!memcmp(ivbuf, ciphertext + size - sizeof(iv), sizeof(iv)), "wrong iv data.\n");

    size = 0;
    ++ivbuf[0];
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext, 32, NULL, ivbuf, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(plaintext, expected5, sizeof(expected)), "wrong data\n");
    ok(!memcmp(ivbuf, ciphertext + 32 - 16, sizeof(iv)), "wrong iv data.\n");

    size = 0;
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext, 32, NULL, NULL, 0, plaintext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(plaintext, expected4, sizeof(expected4)), "wrong data\n");

    /* test with padding smaller than block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext2, 32, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext2, 32, NULL, ivbuf, 16, plaintext, 17, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 17, "got %u\n", size);
    ok(!memcmp(plaintext, expected2, sizeof(expected2)), "wrong data\n");
    ok(!memcmp(ivbuf, ciphertext2 + 32 - sizeof(iv), sizeof(iv)), "wrong iv data.\n");

    size = 0;
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext2, 32, NULL, ivbuf, 16, plaintext, 17, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 17, "got %u\n", size);
    ok(!memcmp(plaintext, expected4, size), "wrong data\n");
    ok(!memcmp(ivbuf, ciphertext2 + 32 - 16, sizeof(iv)), "wrong iv data.\n");

    /* test with padding of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext3, 48, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext3, 48, NULL, ivbuf, 16, plaintext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(plaintext, expected3, sizeof(expected3)), "wrong data\n");
    ok(!memcmp(ivbuf, ciphertext3 + 48 - 16, sizeof(iv)), "wrong iv data.\n");

    /* output size too small */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext, 32, NULL, ivbuf, 16, plaintext, 31, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext2, 32, NULL, ivbuf, 16, plaintext, 15, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext2, 32, NULL, ivbuf, 16, plaintext, 16, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 17, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext3, 48, NULL, ivbuf, 16, plaintext, 31, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);

    /* input size is not a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext, 17, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got 0x%x\n", ret);
    ok(size == 17 || broken(size == 0 /* Win < 7 */), "got %u\n", size);

    /* input size is not a multiple of block size, block padding set */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext, 17, NULL, ivbuf, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got 0x%x\n", ret);
    ok(size == 17 || broken(size == 0 /* Win < 7 */), "got %u\n", size);

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    free(buf);

    /******************
     * AES - GCM mode *
     ******************/

    ret = pBCryptSetProperty(aes, BCRYPT_CHAINING_MODE, (UCHAR*)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    key = NULL;
    buf = calloc(1, len);
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(key != NULL, "key not set\n");

    ret = pBCryptGetProperty(key, BCRYPT_AUTH_TAG_LENGTH, (UCHAR*)&tag_lengths, sizeof(tag_lengths), &size, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got 0x%x\n", ret);

    memset(&auth_info, 0, sizeof(auth_info));
    auth_info.cbSize = sizeof(auth_info);
    auth_info.dwInfoVersion = 1;
    auth_info.pbNonce = nonce;
    auth_info.cbNonce = sizeof(nonce);
    auth_info.pbTag = tag;
    auth_info.cbTag = sizeof(tag);

    /* input size is a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext4, 32, &auth_info, ivbuf, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(plaintext, expected3, sizeof(expected3)), "wrong data\n");
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv.\n");

    size = 0;
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext4, 32, &auth_info, NULL, 0, plaintext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(plaintext, expected3, sizeof(expected3)), "wrong data\n");

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ++ivbuf[0];
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext4, 32, &auth_info, ivbuf, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(plaintext, expected3, sizeof(expected3)), "wrong data\n");
    ok(!memcmp(ivbuf + 1, iv + 1, sizeof(iv) - 1), "wrong iv data.\n");
    ok(ivbuf[0] == iv[0] + 1, "wrong iv data.\n");

    /* test with auth data */
    auth_info.pbAuthData = auth_data;
    auth_info.cbAuthData = sizeof(auth_data);
    auth_info.pbTag = tag2;
    auth_info.cbTag = sizeof(tag2);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext4, 32, &auth_info, ivbuf, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(plaintext, expected3, sizeof(expected3)), "wrong data\n");
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv.\n");

    /* test with wrong tag */
    memcpy(ivbuf, iv, sizeof(iv));
    auth_info.pbTag = iv; /* wrong tag */
    ret = pBCryptDecrypt(key, ciphertext4, 32, &auth_info, ivbuf, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_AUTH_TAG_MISMATCH, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ivbuf, iv, sizeof(iv)), "wrong iv data.\n");

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    free(buf);

    /******************
     * AES - ECB mode *
     ******************/

    ret = pBCryptSetProperty(aes, BCRYPT_CHAINING_MODE, (UCHAR*)BCRYPT_CHAIN_MODE_ECB, sizeof(BCRYPT_CHAIN_MODE_ECB), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    len = 0xdeadbeef;
    size = sizeof(len);
    ret = pBCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    buf = calloc(1, len);
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    /* initialization vector is not allowed */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext5, 32, NULL, ivbuf, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);

    /* input size is a multiple of block size */
    size = 0;
    ret = pBCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(plaintext, expected, sizeof(expected)), "wrong data\n");

    /* test with padding smaller than block size */
    size = 0;
    ret = pBCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, plaintext, 17, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 17, "got %u\n", size);
    ok(!memcmp(plaintext, expected2, sizeof(expected2)), "wrong data\n");

    /* test with padding of block size */
    size = 0;
    ret = pBCryptDecrypt(key, ciphertext6, 48, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);

    size = 0;
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext6, 48, NULL, NULL, 16, plaintext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(plaintext, expected3, sizeof(expected3)), "wrong data\n");

    /* output size too small */
    size = 0;
    ret = pBCryptDecrypt(key, ciphertext4, 32, NULL, NULL, 16, plaintext, 31, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    ret = pBCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, plaintext, 15, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    ret = pBCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, plaintext, 16, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 17, "got %u\n", size);

    size = 0;
    ret = pBCryptDecrypt(key, ciphertext6, 48, NULL, NULL, 16, plaintext, 31, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == 48, "got %u\n", size);

    /* input size is not a multiple of block size */
    size = 0;
    ret = pBCryptDecrypt(key, ciphertext4, 17, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got 0x%x\n", ret);
    ok(size == 17 || broken(size == 0 /* Win < 7 */), "got %u\n", size);

    /* input size is not a multiple of block size, block padding set */
    size = 0;
    ret = pBCryptDecrypt(key, ciphertext4, 17, NULL, NULL, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got 0x%x\n", ret);
    ok(size == 17 || broken(size == 0 /* Win < 7 */), "got %u\n", size);

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_INVALID_HANDLE, "got 0x%x\n", ret);
    free(buf);

    ret = pBCryptDestroyKey(NULL);
    ok(ret == STATUS_INVALID_HANDLE, "got 0x%x\n", ret);

    ret = pBCryptCloseAlgorithmProvider(aes, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    ret = pBCryptCloseAlgorithmProvider(aes, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got 0x%x\n", ret);

    ret = pBCryptCloseAlgorithmProvider(NULL, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got 0x%x\n", ret);
}

static void test_key_import_export(void)
{
    static const UCHAR encrypted_blob[40] = {0x33,0x6e,0x51,0x10,
        0x25,0xba,0xdb,0xce,0xcb,0x25,0x00,0x85,0x51,0xc0,0xfa,0x21,
        0x66,0xdd,0x6d,0x67,0x46,0x76,0x0f,0x8a,0x44,0xe5,0x65,0x31,
        0xcb,0x02,0x52,0x9c,0x69,0x59,0x1a,0xec,0x67,0x27,0x11,0xaa};
    UCHAR buffer1[sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + 16];
    UCHAR buffer2[sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + 32], *buf;
    UCHAR buffer3[32 + 8], buffer4[sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + 32];
    BCRYPT_KEY_DATA_BLOB_HEADER *key_data1 = (void*)buffer1;
    BCRYPT_KEY_DATA_BLOB_HEADER *key_data2 = (void*)buffer2;
    BCRYPT_ALG_HANDLE aes;
    BCRYPT_KEY_HANDLE key, key2, key3;
    NTSTATUS ret;
    ULONG size;

    ret = pBCryptOpenAlgorithmProvider(&aes, BCRYPT_AES_ALGORITHM, NULL, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    key_data1->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
    key_data1->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
    key_data1->cbKeyData = 16;
    memset(&key_data1[1], 0x11, 16);

    key = NULL;
    ret = pBCryptImportKey(aes, NULL, BCRYPT_KEY_DATA_BLOB, &key, NULL, 0, buffer1, sizeof(buffer1), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(key != NULL, "key not set\n");

    key_data2->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
    key_data2->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
    key_data2->cbKeyData = 32;
    memset(&key_data2[1], 0x22, 32);
    key2 = NULL;
    ret = pBCryptImportKey(aes, NULL, BCRYPT_KEY_DATA_BLOB, &key2, NULL, 0, buffer2, sizeof(buffer2), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(key2 != NULL, "key not set\n");

    size = 0;
    ret = pBCryptExportKey(key2, key, BCRYPT_AES_WRAP_KEY_BLOB, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == sizeof(buffer3), "got %u\n", size);

    ret = pBCryptExportKey(key2, key, BCRYPT_AES_WRAP_KEY_BLOB, buffer3, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(!memcmp(buffer3, encrypted_blob, sizeof(encrypted_blob)), "blobs didn't match\n");

    key3 = NULL;
    ret = pBCryptImportKey(aes, key, BCRYPT_AES_WRAP_KEY_BLOB, &key3, NULL, 0, buffer3, sizeof(buffer3), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(key3 != NULL, "key not set\n");

    size = 0;
    memset(buffer4, 0xff, sizeof(buffer4));
    ret = pBCryptExportKey(key3, NULL, BCRYPT_KEY_DATA_BLOB, buffer4, sizeof(buffer4), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == sizeof(buffer2), "Got %u\n", size);
    ok(!memcmp(buffer4, buffer2, sizeof(buffer2)), "Expected exported key to match imported key\n");

    pBCryptDestroyKey(key3);
    pBCryptDestroyKey(key2);

    size = 0;
    ret = pBCryptExportKey(key, NULL, BCRYPT_KEY_DATA_BLOB, buffer1, 0, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size == sizeof(buffer1), "got %u\n", size);

    size = 0;
    memset(buffer2, 0xff, sizeof(buffer2));
    ret = pBCryptExportKey(key, NULL, BCRYPT_KEY_DATA_BLOB, buffer2, sizeof(buffer2), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == sizeof(buffer1), "Got %u\n", size);
    ok(!memcmp(buffer1, buffer2, sizeof(buffer1)), "Expected exported key to match imported key\n");

    /* opaque blob */
    size = 0;
    ret = pBCryptExportKey(key, NULL, BCRYPT_OPAQUE_KEY_BLOB, buffer2, 0, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(size > 0, "got zero\n");

    buf = malloc(size);
    ret = pBCryptExportKey(key, NULL, BCRYPT_OPAQUE_KEY_BLOB, buf, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    key = NULL;
    ret = pBCryptImportKey(aes, NULL, BCRYPT_OPAQUE_KEY_BLOB, &key, NULL, 0, buf, size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(key != NULL, "key not set\n");

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    if (pBCryptHash)
    {
        ret = pBCryptImportKey(BCRYPT_AES_CBC_ALG_HANDLE, NULL, BCRYPT_OPAQUE_KEY_BLOB, &key, NULL, 0, buf, size, 0);
        ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
        pBCryptDestroyKey(key);
    }

    free(buf);
    ret = pBCryptCloseAlgorithmProvider(aes, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
}

static BYTE eccPrivkey[] =
{
    /* X */
    0x26, 0xff, 0x0e, 0xf9, 0x71, 0x93, 0xf8, 0xed, 0x59, 0xfa, 0x24, 0xec, 0x18, 0x13, 0xfe, 0xf5,
    0x0b, 0x4a, 0xb1, 0x27, 0xb7, 0xab, 0x3e, 0x4f, 0xc5, 0x5a, 0x91, 0xa3, 0x6e, 0x21, 0x61, 0x65,
    /* Y */
    0x62, 0x7b, 0x8b, 0x30, 0x7a, 0x63, 0x4c, 0x1a, 0xf4, 0x54, 0x54, 0xbb, 0x75, 0x59, 0x68, 0x36,
    0xfe, 0x49, 0x95, 0x75, 0x9e, 0x20, 0x3e, 0x69, 0x58, 0xb9, 0x7a, 0x84, 0x03, 0x45, 0x5c, 0x10,
    /* d */
    0xb9, 0xcd, 0xbe, 0xd4, 0x75, 0x5d, 0x05, 0xe5, 0x83, 0x0c, 0xd3, 0x37, 0x34, 0x15, 0xe3, 0x2c,
    0xe5, 0x85, 0x15, 0xa9, 0xee, 0xba, 0x94, 0x03, 0x03, 0x0b, 0x86, 0xea, 0x85, 0x40, 0xbd, 0x35,
};
static BYTE eccPubkey[] =
{
    /* X */
    0x3b, 0x3c, 0x34, 0xc8, 0x3f, 0x15, 0xea, 0x02, 0x68, 0x46, 0x69, 0xdf, 0x0c, 0xa6, 0xee, 0x7a,
    0xd9, 0x82, 0x08, 0x9b, 0x37, 0x53, 0x42, 0xf3, 0x13, 0x63, 0xda, 0x65, 0x79, 0xe8, 0x04, 0x9e,
    /* Y */
    0x8c, 0x77, 0xc4, 0x33, 0x77, 0xd9, 0x5a, 0x7f, 0x60, 0x7b, 0x98, 0xce, 0xf3, 0x96, 0x56, 0xd6,
    0xb5, 0x8d, 0x87, 0x7a, 0x00, 0x2b, 0xf3, 0x70, 0xb3, 0x90, 0x73, 0xa0, 0x56, 0x06, 0x3b, 0x22,
};
static BYTE certHash[] =
{
    0x28, 0x19, 0x0f, 0x15, 0x6d, 0x75, 0xcc, 0xcf, 0x62, 0xf1, 0x5e, 0xe6, 0x8a, 0xc3, 0xf0, 0x5d,
    0x89, 0x28, 0x2d, 0x48, 0xd8, 0x73, 0x7c, 0x05, 0x05, 0x8e, 0xbc, 0xce, 0x28, 0xb7, 0xba, 0xc9,
};
static BYTE certSignature[] =
{
    /* r */
    0xd7, 0x29, 0xce, 0x5a, 0xef, 0x74, 0x85, 0xd1, 0x18, 0x5f, 0x6e, 0xf1, 0xba, 0x53, 0xd4, 0xcd,
    0xdd, 0xe0, 0x5d, 0xf1, 0x5e, 0x48, 0x51, 0xea, 0x63, 0xc0, 0xe8, 0xe2, 0xf6, 0xfa, 0x4c, 0xaf,
    /* s */
    0xe3, 0x94, 0x15, 0x3b, 0x6c, 0x71, 0x6e, 0x44, 0x22, 0xcb, 0xa0, 0x88, 0xcd, 0x0a, 0x5a, 0x50,
    0x29, 0x7c, 0x5c, 0xd6, 0x6c, 0xd2, 0xe0, 0x7f, 0xcd, 0x02, 0x92, 0x21, 0x4c, 0x2c, 0x92, 0xee,
};

static void test_ECDSA(void)
{
    BYTE buffer[sizeof(BCRYPT_ECCKEY_BLOB) + sizeof(eccPrivkey)];
    BCRYPT_ECCKEY_BLOB *ecckey = (void *)buffer;
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key;
    NTSTATUS status;
    DWORD keylen;
    ULONG size;

    status = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0);
    ok(!status, "got 0x%x\n", status);

    ecckey->dwMagic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
    memcpy(ecckey + 1, eccPubkey, sizeof(eccPubkey));

    ecckey->cbKey = 2;
    size = sizeof(BCRYPT_ECCKEY_BLOB) + sizeof(eccPubkey);
    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_ECCPUBLIC_BLOB, &key, buffer, size, 0);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got 0x%x\n", status);

    ecckey->dwMagic = BCRYPT_ECDH_PUBLIC_P256_MAGIC;
    ecckey->cbKey = 32;
    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_ECCPUBLIC_BLOB, &key, buffer, size, 0);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got 0x%x\n", status);

    ecckey->dwMagic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
    ecckey->cbKey = 32;
    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_PUBLIC_KEY_BLOB, &key, buffer, size, 0);
    ok(!status, "BCryptImportKeyPair failed: 0x%x\n", status);
    pBCryptDestroyKey(key);

    if (pBCryptHash)
    {
        status = pBCryptImportKeyPair(BCRYPT_ECDSA_P256_ALG_HANDLE, NULL, BCRYPT_PUBLIC_KEY_BLOB, &key, buffer,
                                     size, 0);
        ok(!status, "BCryptImportKeyPair failed: 0x%x\n", status);
        pBCryptDestroyKey(key);
    }

    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_ECCPUBLIC_BLOB, &key, buffer, size, 0);
    ok(!status, "BCryptImportKeyPair failed: 0x%x\n", status);

    keylen = 0;
    status = pBCryptGetProperty(key, BCRYPT_KEY_STRENGTH, (UCHAR *)&keylen, sizeof(keylen), &size, 0);
    ok(!status, "got 0x%x\n", status);
    ok(size == sizeof(keylen), "got %u\n", size);
    ok(keylen == 256, "got %u\n", keylen);

    memset(buffer, 0xcc, sizeof(buffer));
    status = pBCryptExportKey(key, NULL, BCRYPT_ECCPUBLIC_BLOB, buffer, sizeof(buffer), &size, 0);
    ok(!status, "Got unexpected status 0x%x\n", status);
    ok(ecckey->dwMagic == BCRYPT_ECDSA_PUBLIC_P256_MAGIC, "Got unexpected magic 0x%x.\n", ecckey->dwMagic);
    ok(ecckey->cbKey == 32, "got %u\n", ecckey->cbKey);
    ok(!memcmp(ecckey + 1, eccPubkey, sizeof(eccPubkey)), "Got unexpected key data.\n");

    status = pBCryptExportKey(key, NULL, BCRYPT_ECCPRIVATE_BLOB, buffer, sizeof(buffer), &size, 0);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected status 0x%x\n", status);

    status = pBCryptVerifySignature(key, NULL, certHash, sizeof(certHash) - 1, certSignature, sizeof(certSignature), 0);
    ok(status == STATUS_INVALID_SIGNATURE, "Expected STATUS_INVALID_SIGNATURE, got 0x%x\n", status);

    status = pBCryptVerifySignature(key, NULL, certHash, sizeof(certHash), certSignature, sizeof(certSignature), 0);
    ok(!status, "BCryptVerifySignature failed: 0x%x\n", status);
    pBCryptDestroyKey(key);

    ecckey->dwMagic = BCRYPT_ECDSA_PRIVATE_P256_MAGIC;
    memcpy(ecckey + 1, eccPrivkey, sizeof(eccPrivkey));

    ecckey->cbKey = 2;
    size = sizeof(BCRYPT_ECCKEY_BLOB) + sizeof(eccPrivkey);
    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_ECCPRIVATE_BLOB, &key, buffer, size, 0);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got 0x%x\n", status);

    ecckey->dwMagic = BCRYPT_ECDH_PRIVATE_P256_MAGIC;
    ecckey->cbKey = 32;
    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_ECCPRIVATE_BLOB, &key, buffer, size, 0);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got 0x%x\n", status);

    ecckey->dwMagic = BCRYPT_ECDSA_PRIVATE_P256_MAGIC;
    ecckey->cbKey = 32;
    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_ECCPRIVATE_BLOB, &key, buffer, size, 0);
    ok(!status, "BCryptImportKeyPair failed: 0x%x\n", status);

    memset( buffer, 0xcc, sizeof(buffer) );
    status = pBCryptExportKey(key, NULL, BCRYPT_ECCPUBLIC_BLOB, buffer, sizeof(buffer), &size, 0);
    ok(!status, "Got unexpected status 0x%x\n", status);
    ok(ecckey->dwMagic == BCRYPT_ECDSA_PUBLIC_P256_MAGIC, "got 0x%x\n", ecckey->dwMagic);
    ok(ecckey->cbKey == 32, "got %u\n", ecckey->cbKey);
    ok(!memcmp(ecckey + 1, eccPrivkey, sizeof(eccPubkey)), "Got unexpected key data.\n");

    size = sizeof(BCRYPT_ECCKEY_BLOB) + sizeof(eccPrivkey);
    memset( buffer, 0, sizeof(buffer) );
    status = pBCryptExportKey(key, NULL, BCRYPT_ECCPRIVATE_BLOB, buffer, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ecckey = (BCRYPT_ECCKEY_BLOB *)buffer;
    ok(ecckey->dwMagic == BCRYPT_ECDSA_PRIVATE_P256_MAGIC, "got 0x%x\n", ecckey->dwMagic);
    ok(ecckey->cbKey == 32, "got %u\n", ecckey->cbKey);
    ok(size == sizeof(*ecckey) + ecckey->cbKey * 3, "got %u\n", size);

    pBCryptDestroyKey(key);
    pBCryptCloseAlgorithmProvider(alg, 0);
}

static UCHAR rsaPublicBlob[] =
{
    0x52, 0x53, 0x41, 0x31, 0x00, 0x08, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0xad, 0x41, 0x09, 0xa2, 0x56,
    0x3a, 0x7b, 0x75, 0x4b, 0x72, 0x9b, 0x28, 0x72, 0x3b, 0xae, 0x9f, 0xd8, 0xa8, 0x25, 0x4a, 0x4c,
    0x19, 0xf5, 0xa6, 0xd0, 0x05, 0x1c, 0x59, 0x8f, 0xe3, 0xf3, 0x2d, 0x29, 0x47, 0xf8, 0x80, 0x25,
    0x25, 0x21, 0x58, 0xc2, 0xac, 0xa1, 0x9e, 0x93, 0x8e, 0x82, 0x6d, 0xd7, 0xf3, 0xe7, 0x8f, 0x0b,
    0xc0, 0x41, 0x85, 0x29, 0x3c, 0xf1, 0x0b, 0x2c, 0x5d, 0x49, 0xed, 0xb4, 0x30, 0x6e, 0x02, 0x15,
    0x4b, 0x9a, 0x08, 0x0d, 0xe1, 0x6f, 0xa8, 0xd3, 0x12, 0xab, 0x66, 0x48, 0x4d, 0xd9, 0x28, 0x03,
    0x6c, 0x9d, 0x44, 0x7a, 0xed, 0xc9, 0x43, 0x4f, 0x9d, 0x4e, 0x3c, 0x7d, 0x0e, 0xff, 0x07, 0x87,
    0xeb, 0xca, 0xca, 0x65, 0x6d, 0xbe, 0xc5, 0x31, 0x8b, 0xcc, 0x7e, 0x0a, 0x71, 0x4a, 0x4d, 0x9d,
    0x3d, 0xfd, 0x7a, 0x56, 0x32, 0x8a, 0x6c, 0x6d, 0x9d, 0x2a, 0xd9, 0x8e, 0x68, 0x89, 0x63, 0xc6,
    0x4f, 0x24, 0xd1, 0x2a, 0x72, 0x69, 0x08, 0x77, 0xa0, 0x7f, 0xfe, 0xc6, 0x33, 0x8d, 0xb4, 0x7d,
    0x73, 0x91, 0x13, 0x9c, 0x47, 0x53, 0x6a, 0x13, 0xdf, 0x19, 0xc7, 0xed, 0x48, 0x81, 0xed, 0xd8,
    0x1f, 0x11, 0x11, 0xbb, 0x41, 0x15, 0x5b, 0xa4, 0xf5, 0xc9, 0x2b, 0x48, 0x5e, 0xd8, 0x4b, 0x52,
    0x1f, 0xf7, 0x87, 0xf2, 0x68, 0x25, 0x28, 0x79, 0xee, 0x39, 0x41, 0xc9, 0x0e, 0xc8, 0xf9, 0xf2,
    0xd8, 0x24, 0x09, 0xb4, 0xd4, 0xb7, 0x90, 0xba, 0x26, 0xe8, 0x1d, 0xb4, 0xd7, 0x09, 0x00, 0xc4,
    0xa0, 0xb6, 0x14, 0xe8, 0x4c, 0x29, 0x60, 0x54, 0x2e, 0x01, 0xde, 0x54, 0x66, 0x40, 0x22, 0x50,
    0x27, 0xf1, 0xe7, 0x62, 0xa9, 0x00, 0x5a, 0x61, 0x2e, 0xfa, 0xfe, 0x16, 0xd8, 0xe0, 0xe7, 0x66,
    0x17, 0xda, 0xb8, 0x0c, 0xa6, 0x04, 0x8d, 0xf8, 0x21, 0x68, 0x39
};

static UCHAR rsaHash[] =
{
    0x96, 0x1f, 0xa6, 0x49, 0x58, 0x81, 0x8f, 0x76, 0x77, 0x07, 0x07, 0x27, 0x55, 0xd7, 0x01, 0x8d,
    0xcd, 0x27, 0x8e, 0x94
};

static UCHAR rsaSignature[] =
{
    0xa8, 0x3a, 0x9d, 0xaf, 0x92, 0x94, 0xa4, 0x4d, 0x34, 0xba, 0x41, 0x0c, 0xc1, 0x23, 0x91, 0xc7,
    0x91, 0xa8, 0xf8, 0xfc, 0x94, 0x87, 0x4d, 0x05, 0x85, 0x63, 0xe8, 0x7d, 0xea, 0x7f, 0x6b, 0x8d,
    0xbb, 0x9a, 0xd4, 0x46, 0xa6, 0xc0, 0xd6, 0xdc, 0x91, 0xba, 0xd3, 0x1a, 0xbf, 0xf4, 0x52, 0xa0,
    0xc7, 0x15, 0x87, 0xe9, 0x1e, 0x60, 0x49, 0x9c, 0xee, 0x5a, 0x9c, 0x6c, 0xbd, 0x7a, 0x3e, 0xc3,
    0x48, 0xb3, 0xee, 0xca, 0x68, 0x40, 0x9b, 0xa1, 0x4c, 0x6e, 0x20, 0xd6, 0xca, 0x6c, 0x72, 0xaf,
    0x2b, 0x6b, 0x62, 0x7c, 0x78, 0x06, 0x94, 0x4c, 0x02, 0xf3, 0x8d, 0x49, 0xe0, 0x11, 0xc4, 0x9b,
    0x62, 0x5b, 0xc2, 0xfd, 0x68, 0xf4, 0x07, 0x15, 0x71, 0x11, 0x4c, 0x35, 0x97, 0x5d, 0xc0, 0xe6,
    0x22, 0xc9, 0x8a, 0x7b, 0x96, 0xc9, 0xc3, 0xe4, 0x2b, 0x1e, 0x88, 0x17, 0x4f, 0x98, 0x9b, 0xf3,
    0x42, 0x23, 0x0c, 0xa0, 0xfa, 0x19, 0x03, 0x2a, 0xf7, 0x13, 0x2d, 0x27, 0xac, 0x9f, 0xaf, 0x2d,
    0xa3, 0xce, 0xf7, 0x63, 0xbb, 0x39, 0x9f, 0x72, 0x80, 0xdd, 0x6c, 0x73, 0x00, 0x85, 0x70, 0xf2,
    0xed, 0x50, 0xed, 0xa0, 0x74, 0x42, 0xd7, 0x22, 0x46, 0x24, 0xee, 0x67, 0xdf, 0xb5, 0x45, 0xe8,
    0x49, 0xf4, 0x9c, 0xe4, 0x00, 0x83, 0xf2, 0x27, 0x8e, 0xa2, 0xb1, 0xc3, 0xc2, 0x01, 0xd7, 0x59,
    0x2e, 0x4d, 0xac, 0x49, 0xa2, 0xc1, 0x8d, 0x88, 0x4b, 0xfe, 0x28, 0xe5, 0xac, 0xa6, 0x85, 0xc4,
    0x1f, 0xf8, 0xc5, 0xc5, 0x14, 0x4e, 0xa3, 0xcb, 0x17, 0xb7, 0x64, 0xb3, 0xc2, 0x12, 0xf8, 0xf8,
    0x36, 0x99, 0x1c, 0x91, 0x9b, 0xbd, 0xed, 0x55, 0x0f, 0xfd, 0x49, 0x85, 0xbb, 0x32, 0xad, 0x78,
    0xc1, 0x74, 0xe6, 0x7c, 0x18, 0x0f, 0x2b, 0x3b, 0xaa, 0xd1, 0x9d, 0x40, 0x71, 0x1d, 0x19, 0x53
};

static UCHAR rsaPrivateBlob[] =
{
    0x52, 0x53, 0x41, 0x32, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0xa6, 0x8b, 0x46, 0x26, 0xb5,
    0xa9, 0x69, 0x83, 0x94, 0x66, 0xa7, 0xf3, 0x33, 0x95, 0x74, 0xe9, 0xeb, 0xc8, 0xcd, 0xd7, 0x81,
    0x9e, 0x45, 0x66, 0xb2, 0x48, 0x8b, 0x1f, 0xfe, 0xb3, 0x62, 0xc4, 0x0d, 0xa2, 0xf9, 0xf3, 0xe2,
    0xa6, 0x86, 0xd1, 0x1e, 0x8a, 0xbb, 0x1d, 0xa5, 0xc5, 0xe8, 0xa7, 0x50, 0x37, 0xfd, 0x69, 0x1f,
    0x6f, 0x99, 0x99, 0xca, 0x39, 0x13, 0xea, 0x5b, 0x6b, 0xe3, 0x91, 0xc0, 0xd2, 0x2c, 0x0b, 0x21,
    0xb1, 0xac, 0xa9, 0xe8, 0xa0, 0x6d, 0xa4, 0x1f, 0x1b, 0x34, 0xcb, 0x88, 0x7f, 0x2e, 0xeb, 0x7d,
    0x91, 0x38, 0x48, 0xce, 0x05, 0x73, 0x05, 0xdd, 0x22, 0x94, 0xc3, 0xdd, 0x1c, 0xfd, 0xc5, 0x41,
    0x2e, 0x94, 0xf9, 0xed, 0xe5, 0x92, 0x5f, 0x3f, 0x06, 0xf8, 0x49, 0x60, 0xb8, 0x92, 0x52, 0x6a,
    0x56, 0x6e, 0xd7, 0x04, 0x1a, 0xb5, 0xb5, 0x1c, 0x31, 0xd1, 0x1b,
};

static UCHAR rsaFullPrivateBlob[] =
{
    0x52, 0x53, 0x41, 0x33, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0xa6, 0x8b, 0x46, 0x26, 0xb5,
    0xa9, 0x69, 0x83, 0x94, 0x66, 0xa7, 0xf3, 0x33, 0x95, 0x74, 0xe9, 0xeb, 0xc8, 0xcd, 0xd7, 0x81,
    0x9e, 0x45, 0x66, 0xb2, 0x48, 0x8b, 0x1f, 0xfe, 0xb3, 0x62, 0xc4, 0x0d, 0xa2, 0xf9, 0xf3, 0xe2,
    0xa6, 0x86, 0xd1, 0x1e, 0x8a, 0xbb, 0x1d, 0xa5, 0xc5, 0xe8, 0xa7, 0x50, 0x37, 0xfd, 0x69, 0x1f,
    0x6f, 0x99, 0x99, 0xca, 0x39, 0x13, 0xea, 0x5b, 0x6b, 0xe3, 0x91, 0xc0, 0xd2, 0x2c, 0x0b, 0x21,
    0xb1, 0xac, 0xa9, 0xe8, 0xa0, 0x6d, 0xa4, 0x1f, 0x1b, 0x34, 0xcb, 0x88, 0x7f, 0x2e, 0xeb, 0x7d,
    0x91, 0x38, 0x48, 0xce, 0x05, 0x73, 0x05, 0xdd, 0x22, 0x94, 0xc3, 0xdd, 0x1c, 0xfd, 0xc5, 0x41,
    0x2e, 0x94, 0xf9, 0xed, 0xe5, 0x92, 0x5f, 0x3f, 0x06, 0xf8, 0x49, 0x60, 0xb8, 0x92, 0x52, 0x6a,
    0x56, 0x6e, 0xd7, 0x04, 0x1a, 0xb5, 0xb5, 0x1c, 0x31, 0xd1, 0x1b, 0xa3, 0xf3, 0xd1, 0x69, 0x61,
    0xab, 0xfe, 0xc1, 0xb6, 0x40, 0x7b, 0x19, 0xbb, 0x2d, 0x59, 0xf5, 0xda, 0x49, 0x32, 0x6f, 0x20,
    0x24, 0xd3, 0xb3, 0xec, 0x21, 0xec, 0x0c, 0xc7, 0x5b, 0xf9, 0x1b, 0xba, 0x6e, 0xe9, 0x61, 0xda,
    0x55, 0xc6, 0x72, 0xfd, 0x2d, 0x66, 0x3f, 0x3c, 0xcb, 0x49, 0xa9, 0xc5, 0x0d, 0x9b, 0x02, 0x36,
    0x7a, 0xee, 0x36, 0x09, 0x55, 0xe4, 0x03, 0xf2, 0xe3, 0xe6, 0x25, 0x14, 0x89, 0x7f, 0x2b, 0xfb,
    0x27, 0x0e, 0x8d, 0x37, 0x84, 0xfd, 0xad, 0x10, 0x79, 0x43, 0x4e, 0x38, 0x4a, 0xd4, 0x5e, 0xfa,
    0xda, 0x9f, 0x88, 0x21, 0x7c, 0xb4, 0x98, 0xb6, 0x6e, 0x1c, 0x24, 0x09, 0xe5, 0xe7, 0x22, 0x6f,
    0xd3, 0x84, 0xc0, 0xdc, 0x36, 0x09, 0xaf, 0x4b, 0x96, 0x8b, 0x5f, 0x47, 0xb3, 0x24, 0x80, 0xb5,
    0x64, 0x69, 0xad, 0x83, 0xd5, 0x09, 0xe7, 0xb9, 0xe4, 0x81, 0x6f, 0x1a, 0xe2, 0x6d, 0xf1, 0x5e,
    0x2b, 0xb3, 0x7a, 0xd0, 0x77, 0xef, 0x82, 0xcd, 0x55, 0x2e, 0xd5, 0xb1, 0xa7, 0x72, 0xec, 0x02,
    0x9d, 0xe2, 0xcc, 0x5a, 0xf1, 0x68, 0x30, 0xe5, 0xbc, 0x8d, 0xad,
};

static struct
{
    PUBLICKEYSTRUC header;
    RSAPUBKEY rsapubkey;
    BYTE modulus[512 / 8];
    BYTE prime1[512 / 16];
    BYTE prime2[512 / 16];
    BYTE exp1[512 / 16];
    BYTE exp2[512 / 16];
    BYTE coefficient[512 / 16];
    BYTE private_exp[512 / 8];

} rsaLegacyPrivateBlob =
{
    { PRIVATEKEYBLOB, CUR_BLOB_VERSION, 0, CALG_RSA_KEYX },
    { BCRYPT_RSAPRIVATE_MAGIC, 512, 0x10001 },
    {
        0x91, 0xe3, 0x6b, 0x5b, 0xea, 0x13, 0x39, 0xca, 0x99, 0x99, 0x6f, 0x1f, 0x69, 0xfd, 0x37, 0x50,
        0xa7, 0xe8, 0xc5, 0xa5, 0x1d, 0xbb, 0x8a, 0x1e, 0xd1, 0x86, 0xa6, 0xe2, 0xf3, 0xf9, 0xa2, 0x0d,
        0xc4, 0x62, 0xb3, 0xfe, 0x1f, 0x8b, 0x48, 0xb2, 0x66, 0x45, 0x9e, 0x81, 0xd7, 0xcd, 0xc8, 0xeb,
        0xe9, 0x74, 0x95, 0x33, 0xf3, 0xa7, 0x66, 0x94, 0x83, 0x69, 0xa9, 0xb5, 0x26, 0x46, 0x8b, 0xa6
    },
    {
        0xc3, 0x94, 0x22, 0xdd, 0x05, 0x73, 0x05, 0xce, 0x48, 0x38, 0x91, 0x7d, 0xeb, 0x2e, 0x7f, 0x88,
        0xcb, 0x34, 0x1b, 0x1f, 0xa4, 0x6d, 0xa0, 0xe8, 0xa9, 0xac, 0xb1, 0x21, 0x0b, 0x2c, 0xd2, 0xc0
    },
    {
        0x1b, 0xd1, 0x31, 0x1c, 0xb5, 0xb5, 0x1a, 0x04, 0xd7, 0x6e, 0x56, 0x6a, 0x52, 0x92, 0xb8, 0x60,
        0x49, 0xf8, 0x06, 0x3f, 0x5f, 0x92, 0xe5, 0xed, 0xf9, 0x94, 0x2e, 0x41, 0xc5, 0xfd, 0x1c, 0xdd
    },
    {
        0x1b, 0xf9, 0x5b, 0xc7, 0x0c, 0xec, 0x21, 0xec, 0xb3, 0xd3, 0x24, 0x20, 0x6f, 0x32, 0x49, 0xda,
        0xf5, 0x59, 0x2d, 0xbb, 0x19, 0x7b, 0x40, 0xb6, 0xc1, 0xfe, 0xab, 0x61, 0x69, 0xd1, 0xf3, 0xa3
    },
    {
        0x25, 0xe6, 0xe3, 0xf2, 0x03, 0xe4, 0x55, 0x09, 0x36, 0xee, 0x7a, 0x36, 0x02, 0x9b, 0x0d, 0xc5,
        0xa9, 0x49, 0xcb, 0x3c, 0x3f, 0x66, 0x2d, 0xfd, 0x72, 0xc6, 0x55, 0xda, 0x61, 0xe9, 0x6e, 0xba
    },
    {
        0x24, 0x1c, 0x6e, 0xb6, 0x98, 0xb4, 0x7c, 0x21, 0x88, 0x9f, 0xda, 0xfa, 0x5e, 0xd4, 0x4a, 0x38,
        0x4e, 0x43, 0x79, 0x10, 0xad, 0xfd, 0x84, 0x37, 0x8d, 0x0e, 0x27, 0xfb, 0x2b, 0x7f, 0x89, 0x14
    },
    {
        0xad, 0x8d, 0xbc, 0xe5, 0x30, 0x68, 0xf1, 0x5a, 0xcc, 0xe2, 0x9d, 0x02, 0xec, 0x72, 0xa7, 0xb1,
        0xd5, 0x2e, 0x55, 0xcd, 0x82, 0xef, 0x77, 0xd0, 0x7a, 0xb3, 0x2b, 0x5e, 0xf1, 0x6d, 0xe2, 0x1a,
        0x6f, 0x81, 0xe4, 0xb9, 0xe7, 0x09, 0xd5, 0x83, 0xad, 0x69, 0x64, 0xb5, 0x80, 0x24, 0xb3, 0x47,
        0x5f, 0x8b, 0x96, 0x4b, 0xaf, 0x09, 0x36, 0xdc, 0xc0, 0x84, 0xd3, 0x6f, 0x22, 0xe7, 0xe5, 0x09
    }
};


static UCHAR rsaPublicBlobWithInvalidPublicExpSize[] =
{
    0x52, 0x53, 0x41, 0x31, 0x00, 0x04, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x01, 0xc7, 0x8f, 0xac, 0x2a, 0xce, 0xbf, 0xc9, 0x6c, 0x7b,
    0x85, 0x74, 0x71, 0xbb, 0xff, 0xbb, 0x9b, 0x20, 0x03, 0x79, 0x17, 0x34,
    0xe7, 0x26, 0x91, 0x5c, 0x1f, 0x1b, 0x03, 0x3d, 0x46, 0xdf, 0xb6, 0xf2,
    0x10, 0x55, 0xf0, 0x39, 0x55, 0x0a, 0xe3, 0x9c, 0x0c, 0x63, 0xc2, 0x14,
    0x03, 0x94, 0x51, 0x0d, 0xb4, 0x22, 0x09, 0xf2, 0x5c, 0xb2, 0xd1, 0xc3,
    0xac, 0x6f, 0xa8, 0xc4, 0xac, 0xb8, 0xbc, 0x59, 0xe7, 0xed, 0x77, 0x6e,
    0xb1, 0x80, 0x58, 0x7d, 0xb2, 0x94, 0x46, 0xe5, 0x00, 0xe2, 0xb7, 0x33,
    0x48, 0x7a, 0xd3, 0x78, 0xe9, 0x26, 0x01, 0xc7, 0x00, 0x7b, 0x41, 0x6d,
    0x94, 0x3a, 0xe1, 0x50, 0x2b, 0x9f, 0x6b, 0x1c, 0x08, 0xa3, 0xfc, 0x0a,
    0x44, 0x81, 0x09, 0x41, 0x80, 0x23, 0x7b, 0xf6, 0x3f, 0xaf, 0x91, 0xa1,
    0x87, 0x75, 0x33, 0x15, 0xb8, 0xde, 0x32, 0x30, 0xb4, 0x5e, 0xfd
};

static const UCHAR rsa_encrypted_no_padding[] =
{
    0x4a, 0xc1, 0xfa, 0x4f, 0xe0, 0x3f, 0x36, 0x9a, 0x64, 0xbf, 0x2e, 0x00, 0xb4, 0xb5, 0x40, 0xbe,
    0x2d, 0x9a, 0x14, 0xf6, 0x8f, 0xa5, 0xc2, 0xe2, 0x20, 0xaf, 0x21, 0x79, 0xc6, 0x32, 0x7e, 0xea,
    0x73, 0x00, 0x01, 0xbb, 0x9a, 0x19, 0x73, 0x41, 0x96, 0xae, 0x88, 0x6e, 0x36, 0x56, 0xe9, 0x9c,
    0xac, 0x04, 0x82, 0xa8, 0x00, 0xdb, 0x4e, 0x29, 0x61, 0x7e, 0xaf, 0x64, 0xdb, 0xa2, 0x70, 0x0f,
};

static void test_rsa_encrypt(void)
{
    UCHAR input[] = "Hello World!", input_no_padding[64] = { 0 }, encrypted[64], decrypted[64];
    BCRYPT_ALG_HANDLE rsa;
    BCRYPT_KEY_HANDLE key, key2;
    NTSTATUS ret;
    DWORD encrypted_size, decrypted_size;
    UCHAR *encrypted_a = NULL, *encrypted_b = NULL;
    BCRYPT_OAEP_PADDING_INFO oaep_pad;

    oaep_pad.pszAlgId = BCRYPT_SHA256_ALGORITHM;
    oaep_pad.pbLabel = (UCHAR *)"test";
    oaep_pad.cbLabel = 5;

    ret = pBCryptOpenAlgorithmProvider(&rsa, BCRYPT_RSA_ALGORITHM, NULL, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ret = pBCryptGenerateKeyPair(rsa, &key, 512, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    /* Not finalized key */
    ret = pBCryptEncrypt(key, input, sizeof(input), NULL, NULL, 0, NULL, 0, &encrypted_size, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got 0x%x\n", ret);
    pBCryptDestroyKey(key);

    /* Import a different public key first to make sure a public key from private key improted next
     * overrides it. */
    ret = pBCryptImportKeyPair(rsa, NULL, BCRYPT_RSAPUBLIC_BLOB, &key, rsaPublicBlob, sizeof(rsaPublicBlob), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    ret = pBCryptImportKeyPair(rsa, NULL, BCRYPT_RSAPRIVATE_BLOB, &key, rsaPrivateBlob, sizeof(rsaPrivateBlob), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    /*   No padding    */
    todo_wine {
    memset(input_no_padding, 0, sizeof(input_no_padding));
    strcpy((char *)input_no_padding, "Hello World");
    encrypted_size = 0;
    ret = pBCryptEncrypt(key, input_no_padding, sizeof(input_no_padding), NULL, NULL, 0, NULL, 0, &encrypted_size, BCRYPT_PAD_NONE);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(encrypted_size == 64, "got size of %d\n", encrypted_size);

    encrypted_a = malloc(encrypted_size);
    memset(encrypted_a, 0, encrypted_size);
    encrypted_b = malloc(encrypted_size);
    memset(encrypted_b, 0xff, encrypted_size);

    ret = pBCryptEncrypt(key, input, sizeof(input), NULL, NULL, 0, encrypted_a, encrypted_size, &encrypted_size, BCRYPT_PAD_NONE);
    ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);

    ret = pBCryptEncrypt(key, input_no_padding, sizeof(input_no_padding), NULL, NULL, 0, encrypted_a, 12, &encrypted_size, BCRYPT_PAD_NONE);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);
    ok(encrypted_size == 64, "got size of %d\n", encrypted_size);

    ret = pBCryptEncrypt(key, input_no_padding, sizeof(input_no_padding), NULL, NULL, 0, encrypted_a, encrypted_size, &encrypted_size, BCRYPT_PAD_NONE);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(encrypted_size == 64, "got size of %d\n", encrypted_size);

    ret = pBCryptEncrypt(key, input_no_padding, sizeof(input_no_padding), NULL, NULL, 0, encrypted_b, encrypted_size, &encrypted_size, BCRYPT_PAD_NONE);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    }
    ok(!memcmp(encrypted_a, encrypted_b, encrypted_size), "Both outputs should be the same\n");
    ok(!memcmp(encrypted_b, rsa_encrypted_no_padding, encrypted_size), "Data mismatch.\n");

    todo_wine {
    decrypted_size = 0;
    ret = pBCryptDecrypt(key, encrypted_a, encrypted_size, NULL, NULL, 0, NULL, 0, &decrypted_size, BCRYPT_PAD_NONE);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(decrypted_size == sizeof(input_no_padding), "got %u\n", decrypted_size);

    ret = pBCryptDecrypt(key, encrypted_a, encrypted_size, NULL, NULL, 0, decrypted, decrypted_size, &decrypted_size, BCRYPT_PAD_NONE);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(decrypted_size == sizeof(input_no_padding), "got %u\n", decrypted_size);
    ok(!memcmp(decrypted, input_no_padding, sizeof(input_no_padding)), "unexpected output\n");
    }

    /*  PKCS1 Padding  */
    encrypted_size = 0;
    ret = pBCryptEncrypt(key, input, sizeof(input), NULL, NULL, 0, NULL, 0, &encrypted_size, BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(encrypted_size == 64, "got size of %d\n", encrypted_size);

    encrypted_a = realloc(encrypted_a, encrypted_size);
    memset(encrypted_a, 0, encrypted_size);
    encrypted_b = realloc(encrypted_b, encrypted_size);
    memset(encrypted_b, 0, encrypted_size);

    ret = pBCryptEncrypt(key, input, sizeof(input), NULL, NULL, 0, encrypted_a, encrypted_size, &encrypted_size, BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(encrypted_size == 64, "got size of %d\n", encrypted_size);

    ret = pBCryptEncrypt(key, input, sizeof(input), NULL, NULL, 0, encrypted_b, encrypted_size, &encrypted_size, BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(encrypted_size == 64, "got size of %d\n", encrypted_size);
    ok(memcmp(encrypted_a, encrypted_b, encrypted_size), "Both outputs are the same\n");

    decrypted_size = 0;
    ret = pBCryptDecrypt(key, encrypted_a, encrypted_size, NULL, NULL, 0, NULL, 0, &decrypted_size, BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(decrypted_size == sizeof(input), "got size of %d\n", decrypted_size);

    ret = pBCryptDecrypt(key, encrypted_a, encrypted_size, NULL, NULL, 0, decrypted, decrypted_size, &decrypted_size, BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(decrypted_size == sizeof(input), "got size of %d\n", decrypted_size);
    ok(!memcmp(decrypted, input, sizeof(input)), "unexpected output\n");

    ret = pBCryptImportKeyPair(rsa, NULL, LEGACY_RSAPRIVATE_BLOB, &key2, (UCHAR *)&rsaLegacyPrivateBlob,
                              sizeof(rsaLegacyPrivateBlob), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    ret = pBCryptEncrypt(key2, input, sizeof(input), NULL, NULL, 0, encrypted, sizeof(encrypted),
                        &encrypted_size, BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(encrypted_size == 64, "got size of %d\n", encrypted_size);

    memset(decrypted, 0, sizeof(decrypted));
    ret = pBCryptDecrypt(key, encrypted, sizeof(encrypted), NULL, NULL, 0, decrypted, sizeof(decrypted),
                        &decrypted_size, BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(decrypted_size == sizeof(input), "got size of %d\n", decrypted_size);
    ok(!memcmp(decrypted, input, sizeof(input)), "unexpected output\n");
    pBCryptDestroyKey(key2);
    pBCryptDestroyKey(key);

    /*  OAEP Padding  */
    ret = pBCryptGenerateKeyPair(rsa, &key, 640 /* minimum size for sha256 hash */, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    ret = pBCryptFinalizeKeyPair(key, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    todo_wine {
    encrypted_size = 0;
    ret = pBCryptEncrypt(key, input, sizeof(input), &oaep_pad, NULL, 0, NULL, 0, &encrypted_size, BCRYPT_PAD_OAEP);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(encrypted_size == 80, "got size of %d\n", encrypted_size);

    encrypted_a = realloc(encrypted_a, encrypted_size);
    memset(encrypted_a, 0, encrypted_size);
    encrypted_b = realloc(encrypted_b, encrypted_size);
    memset(encrypted_b, 0, encrypted_size);

    ret = pBCryptEncrypt(key, input, sizeof(input), &oaep_pad, NULL, 0, encrypted_a, encrypted_size, &encrypted_size, BCRYPT_PAD_OAEP);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(encrypted_size == 80, "got size of %d\n", encrypted_size);

    ret = pBCryptEncrypt(key, input, sizeof(input), &oaep_pad, NULL, 0, encrypted_b, encrypted_size, &encrypted_size, BCRYPT_PAD_OAEP);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(encrypted_size == 80, "got size of %d\n", encrypted_size);
    ok(memcmp(encrypted_a, encrypted_b, encrypted_size), "Both outputs are the same\n");

    decrypted_size = 0;
    memset(decrypted, 0, sizeof(decrypted));
    ret = pBCryptDecrypt(key, encrypted_a, encrypted_size, &oaep_pad, NULL, 0, NULL, 0, &decrypted_size, BCRYPT_PAD_OAEP);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(decrypted_size == sizeof(input), "got %u\n", decrypted_size);

    ret = pBCryptDecrypt(key, encrypted_a, encrypted_size, &oaep_pad, NULL, 0, decrypted, decrypted_size, &decrypted_size, BCRYPT_PAD_OAEP);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(decrypted_size == sizeof(input), "got %u\n", decrypted_size);
    ok(!memcmp(decrypted, input, sizeof(input)), "unexpected output\n");
    }

    free(encrypted_a);
    free(encrypted_b);

    pBCryptDestroyKey(key);

    if (pBCryptHash)
    {
        ret = pBCryptGenerateKeyPair(BCRYPT_RSA_ALG_HANDLE, &key, 512, 0);
        ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
        pBCryptDestroyKey(key);
    }
}

static void test_RSA(void)
{
    static UCHAR hash[] =
        {0x7e,0xe3,0x74,0xe7,0xc5,0x0b,0x6b,0x70,0xdb,0xab,0x32,0x6d,0x1d,0x51,0xd6,0x74,0x79,0x8e,0x5b,0x4b};
    static UCHAR hash48[] =
        {0x62,0xb2,0x1e,0x90,0xc9,0x02,0x2b,0x10,0x16,0x71,0xba,0x1f,0x80,0x8f,0x86,0x31,0xa8,0x14,0x9f,0x0f,
         0x12,0x90,0x40,0x55,0x83,0x9a,0x35,0xc1,0xca,0x78,0xae,0x53,0x1b,0xb3,0x36,0x06,0xba,0x90,0x89,0x12,
         0xa8,0x42,0x21,0x10,0x9d,0x29,0xcd,0x7e};
    BCRYPT_PKCS1_PADDING_INFO pad;
    BCRYPT_PSS_PADDING_INFO pad_pss;
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key;
    BCRYPT_RSAKEY_BLOB *rsablob;
    UCHAR sig[256], sig_pss[256];
    ULONG len, size, size2, schemes;
    NTSTATUS ret;
    BYTE *buf;
    DWORD keylen;

    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_RSA_ALGORITHM, NULL, 0);
    ok(!ret, "got 0x%x\n", ret);

    schemes = size = 0;
    ret = pBCryptGetProperty(alg, L"PaddingSchemes", (UCHAR *)&schemes, sizeof(schemes), &size, 0);
    ok(!ret, "got 0x%x\n", ret);
    ok(schemes, "schemes not set\n");
    ok(size == sizeof(schemes), "got %u\n", size);

    ret = pBCryptImportKeyPair(alg, NULL, BCRYPT_PUBLIC_KEY_BLOB, &key, rsaPublicBlob, sizeof(rsaPublicBlob), 0);
    ok(!ret, "BCryptImportKeyPair failed: 0x%x\n", ret);
    pBCryptDestroyKey(key);

    ret = pBCryptImportKeyPair(alg, NULL, BCRYPT_RSAPUBLIC_BLOB, &key, rsaPublicBlob, sizeof(rsaPublicBlob), 0);
    ok(!ret, "BCryptImportKeyPair failed: 0x%x\n", ret);

    keylen = 0;
    ret = pBCryptGetProperty(key, BCRYPT_KEY_STRENGTH, (UCHAR *)&keylen, sizeof(keylen), &size, 0);
    ok(!ret, "got 0x%x\n", ret);
    ok(size == sizeof(keylen), "got %u\n", size);
    ok(keylen == 2048, "got %u\n", keylen);

    pad.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    ret = pBCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(!ret, "BCryptVerifySignature failed: 0x%x\n", ret);

    ret = pBCryptVerifySignature(key, NULL, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got 0x%x\n", ret);

    pad.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    ret = pBCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), 0);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got 0x%x\n", ret);

    ret = pBCryptVerifySignature(key, NULL, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), 0);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got 0x%x\n", ret);

    pad.pszAlgId = BCRYPT_AES_ALGORITHM;
    ret = pBCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_NOT_SUPPORTED, "Expected STATUS_NOT_SUPPORTED, got 0x%x\n", ret);

    pad.pszAlgId = NULL;
    ret = pBCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_INVALID_SIGNATURE, "Expected STATUS_INVALID_SIGNATURE, got 0x%x\n", ret);

    ret = pBCryptDestroyKey(key);
    ok(!ret, "BCryptDestroyKey failed: 0x%x\n", ret);

    /* sign/verify with export/import round-trip */
    ret = pBCryptGenerateKeyPair(alg, &key, 1024, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    keylen = 2048;
    ret = pBCryptSetProperty(key, BCRYPT_KEY_LENGTH, (UCHAR *)&keylen, 2, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);
    ret = pBCryptSetProperty(key, BCRYPT_KEY_LENGTH, (UCHAR *)&keylen, sizeof(keylen), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    ret = pBCryptFinalizeKeyPair(key, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    keylen = 0;
    ret = pBCryptGetProperty(key, BCRYPT_KEY_STRENGTH, (UCHAR *)&keylen, sizeof(keylen), &size, 0);
    ok(!ret, "got 0x%x\n", ret);
    ok(size == sizeof(keylen), "got %u\n", size);
    ok(keylen == 2048, "got %u\n", keylen);

    ret = pBCryptSetProperty(key, BCRYPT_KEY_LENGTH, (UCHAR *)&keylen, sizeof(keylen), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    pad.pszAlgId = BCRYPT_MD5_ALGORITHM;
    memset(sig, 0, sizeof(sig));
    len = 0;
    ret = pBCryptSignHash(key, &pad, hash, 16, sig, sizeof(sig), &len, BCRYPT_PAD_PKCS1);
    ok(!ret, "got 0x%x\n", ret);
    ok(len == 256, "got %u\n", len);
    pad.pszAlgId = BCRYPT_MD5_ALGORITHM;
    ret = pBCryptVerifySignature(key, &pad, hash, 16, sig, len, BCRYPT_PAD_PKCS1);
    ok(!ret, "BCryptVerifySignature failed: 0x%x\n", ret);

    pad.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    memset(sig, 0, sizeof(sig));
    len = 0;
    ret = pBCryptSignHash(key, &pad, hash, sizeof(hash), sig, sizeof(sig), &len, BCRYPT_PAD_PKCS1);
    ok(!ret, "got 0x%x\n", ret);
    ok(len == 256, "got %u\n", len);
    pad.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    ret = pBCryptVerifySignature(key, &pad, hash, sizeof(hash), sig, len, BCRYPT_PAD_PKCS1);
    ok(!ret, "BCryptVerifySignature failed: 0x%x\n", ret);

    pad.pszAlgId = NULL;
    memset(sig, 0, sizeof(sig));
    len = 0;
    ret = pBCryptSignHash(key, &pad, hash, sizeof(hash), sig, sizeof(sig), &len, BCRYPT_PAD_PKCS1);
    ok(!ret, "got 0x%x\n", ret);
    ok(len == 256, "got %u\n", len);

    pad.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    ret = pBCryptVerifySignature(key, &pad, hash, sizeof(hash), sig, len, BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_INVALID_SIGNATURE, "BCryptVerifySignature failed: 0x%x, len %d\n", ret, len);

    pad.pszAlgId = NULL;
    ret = pBCryptVerifySignature(key, &pad, hash, sizeof(hash), sig, len, BCRYPT_PAD_PKCS1);
    ok(!ret, "BCryptVerifySignature failed: 0x%x, len %d\n", ret, len);

    pad_pss.pszAlgId = BCRYPT_SHA384_ALGORITHM;
    pad_pss.cbSalt = 48;
    memset(sig_pss, 0, sizeof(sig_pss));
    len = 0;
    ret = pBCryptSignHash(key, &pad_pss, hash48, sizeof(hash48), sig_pss, sizeof(sig_pss), &len, BCRYPT_PAD_PSS);
    ok(!ret, "got 0x%x\n", ret);
    ok(len == 256, "got %u\n", len);

    /* export private key */
    size = 0;
    ret = pBCryptExportKey(key, NULL, BCRYPT_RSAPRIVATE_BLOB, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size, "size not set\n");

    buf = malloc(size);
    ret = pBCryptExportKey(key, NULL, BCRYPT_RSAPRIVATE_BLOB, buf, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    rsablob = (BCRYPT_RSAKEY_BLOB *)buf;
    ok(rsablob->Magic == BCRYPT_RSAPRIVATE_MAGIC, "got 0x%x\n", rsablob->Magic);
    ok(rsablob->BitLength == 2048, "got %u\n", rsablob->BitLength);
    ok(rsablob->cbPublicExp == 3, "got %u\n", rsablob->cbPublicExp);
    ok(rsablob->cbModulus == 256, "got %u\n", rsablob->cbModulus);
    ok(rsablob->cbPrime1 == 128, "got %u\n", rsablob->cbPrime1);
    ok(rsablob->cbPrime2 == 128, "got %u\n", rsablob->cbPrime2);
    size2 = sizeof(*rsablob) + rsablob->cbPublicExp + rsablob->cbModulus + rsablob->cbPrime1 + rsablob->cbPrime2;
    ok(size == size2, "got %u expected %u\n", size2, size);
    free(buf);

    size = 0;
    ret = pBCryptExportKey(key, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size, "size not set\n");

    buf = malloc(size);
    ret = pBCryptExportKey(key, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, buf, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    rsablob = (BCRYPT_RSAKEY_BLOB *)buf;
    ok(rsablob->Magic == BCRYPT_RSAFULLPRIVATE_MAGIC, "got 0x%x\n", rsablob->Magic);
    ok(rsablob->BitLength == 2048, "got %u\n", rsablob->BitLength);
    ok(rsablob->cbPublicExp == 3, "got %u\n", rsablob->cbPublicExp);
    ok(rsablob->cbModulus == 256, "got %u\n", rsablob->cbModulus);
    ok(rsablob->cbPrime1 == 128, "got %u\n", rsablob->cbPrime1);
    ok(rsablob->cbPrime2 == 128, "got %u\n", rsablob->cbPrime2);
    size2 = sizeof(*rsablob) + rsablob->cbPublicExp + rsablob->cbModulus * 2 + rsablob->cbPrime1 * 3 + rsablob->cbPrime2 * 2;
    ok(size == size2, "got %u expected %u\n", size2, size);
    free(buf);

    /* import/export public key */
    size = 0;
    ret = pBCryptExportKey(key, NULL, BCRYPT_RSAPUBLIC_BLOB, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size, "size not set\n");

    buf = malloc(size);
    ret = pBCryptExportKey(key, NULL, BCRYPT_RSAPUBLIC_BLOB, buf, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    rsablob = (BCRYPT_RSAKEY_BLOB *)buf;
    ok(rsablob->Magic == BCRYPT_RSAPUBLIC_MAGIC, "got 0x%x\n", rsablob->Magic);
    ok(rsablob->BitLength == 2048, "got %u\n", rsablob->BitLength);
    ok(rsablob->cbPublicExp == 3, "got %u\n", rsablob->cbPublicExp);
    ok(rsablob->cbModulus == 256, "got %u\n", rsablob->cbModulus);
    ok(!rsablob->cbPrime1, "got %u\n", rsablob->cbPrime1);
    ok(!rsablob->cbPrime2, "got %u\n", rsablob->cbPrime2);
    ok(size == sizeof(*rsablob) + rsablob->cbPublicExp + rsablob->cbModulus, "got %u\n", size);
    ret = pBCryptDestroyKey(key);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptImportKeyPair(alg, NULL, BCRYPT_RSAPUBLIC_BLOB, &key, rsaPublicBlobWithInvalidPublicExpSize,
                              sizeof(rsaPublicBlobWithInvalidPublicExpSize), 0);
    ok(ret == NTE_BAD_DATA, "got 0x%x\n", ret);

    ret = pBCryptImportKeyPair(alg, NULL, BCRYPT_RSAPUBLIC_BLOB, &key, buf, size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    free(buf);

    ret = pBCryptVerifySignature(key, &pad, hash, sizeof(hash), sig, sizeof(sig), BCRYPT_PAD_PKCS1);
    ok(!ret, "got 0x%x\n", ret);
    ret = pBCryptVerifySignature(key, &pad_pss, hash48, sizeof(hash48), sig_pss, sizeof(sig_pss), BCRYPT_PAD_PSS);
    ok(!ret, "got 0x%x\n", ret);
    ret = pBCryptDestroyKey(key);
    ok(!ret, "got 0x%x\n", ret);

    /* import/export private key */
    ret = pBCryptImportKeyPair(alg, NULL, BCRYPT_RSAPRIVATE_BLOB, &key, rsaPrivateBlob, sizeof(rsaPrivateBlob), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    ret = pBCryptFinalizeKeyPair(key, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got 0x%x\n", ret);

    size = 0;
    buf = malloc(sizeof(rsaPrivateBlob));
    ret = pBCryptExportKey(key, NULL, BCRYPT_RSAPRIVATE_BLOB, buf, sizeof(rsaPrivateBlob), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == sizeof(rsaPrivateBlob), "got %u\n", size);
    ok(!memcmp(buf, rsaPrivateBlob, size), "wrong data\n");
    free(buf);
    pBCryptDestroyKey(key);

    /* import/export full private key */
    ret = pBCryptImportKeyPair(alg, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, &key, rsaFullPrivateBlob, sizeof(rsaFullPrivateBlob), 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    size = 0;
    buf = malloc(sizeof(rsaFullPrivateBlob));
    ret = pBCryptExportKey(key, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, buf, sizeof(rsaFullPrivateBlob), &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == sizeof(rsaFullPrivateBlob), "got %u\n", size);
    ok(!memcmp(buf, rsaFullPrivateBlob, size), "wrong data\n");
    free(buf);
    pBCryptDestroyKey(key);

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(!ret, "got 0x%x\n", ret);
}

static void test_RSA_SIGN(void)
{
    BCRYPT_PKCS1_PADDING_INFO pad;
    BCRYPT_ALG_HANDLE alg = NULL;
    BCRYPT_KEY_HANDLE key = NULL;
    BCRYPT_RSAKEY_BLOB *rsablob;
    NTSTATUS ret;
    ULONG size, size2;
    BYTE *buf, buf2[sizeof(BCRYPT_RSAKEY_BLOB) + sizeof(rsaPublicBlob)];

    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_RSA_SIGN_ALGORITHM, NULL, 0);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptImportKeyPair(alg, NULL, BCRYPT_RSAPUBLIC_BLOB, &key, rsaPublicBlob, sizeof(rsaPublicBlob), 0);
    ok(!ret, "BCryptImportKeyPair failed: 0x%x\n", ret);

    memset(buf2, 0xcc, sizeof(buf2));
    ret = pBCryptExportKey(key, NULL, BCRYPT_RSAPUBLIC_BLOB, buf2, sizeof(buf2), &size, 0);
    ok(!ret, "got 0x%x\n", ret);
    rsablob = (BCRYPT_RSAKEY_BLOB *)buf2;
    ok(rsablob->Magic == BCRYPT_RSAPUBLIC_MAGIC, "got 0x%x\n", rsablob->Magic);
    ok(rsablob->BitLength == 2048, "got %u\n", rsablob->BitLength);
    ok(rsablob->cbPublicExp == 3, "got %u\n", rsablob->cbPublicExp);
    ok(rsablob->cbModulus == 256, "got %u\n", rsablob->cbModulus);
    ok(rsablob->cbPrime1 == 0, "got %u\n", rsablob->cbPrime1);
    ok(rsablob->cbPrime2 == 0, "got %u\n", rsablob->cbPrime2);
    size2 = sizeof(*rsablob) + rsablob->cbPublicExp + rsablob->cbModulus + rsablob->cbPrime1 + rsablob->cbPrime2;
    ok(size == size2, "got %u expected %u\n", size2, size);

    ret = pBCryptExportKey(key, NULL, BCRYPT_RSAPRIVATE_BLOB, buf2, sizeof(buf2), &size, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);

    pad.pszAlgId = NULL;
    ret = pBCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_INVALID_SIGNATURE, "BCryptVerifySignature failed: 0x%x\n", ret);

    pad.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    ret = pBCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(!ret, "BCryptVerifySignature failed: 0x%x\n", ret);

    ret = pBCryptVerifySignature(key, NULL, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got 0x%x\n", ret);

    pad.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    ret = pBCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), 0);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got 0x%x\n", ret);

    ret = pBCryptVerifySignature(key, NULL, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), 0);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got 0x%x\n", ret);

    pad.pszAlgId = BCRYPT_AES_ALGORITHM;
    ret = pBCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_NOT_SUPPORTED, "Expected STATUS_NOT_SUPPORTED, got 0x%x\n", ret);

    pad.pszAlgId = NULL;
    ret = pBCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_INVALID_SIGNATURE, "Expected STATUS_INVALID_SIGNATURE, got 0x%x\n", ret);

    ret = pBCryptDestroyKey(key);
    ok(!ret, "BCryptDestroyKey failed: 0x%x\n", ret);

    /* export private key */
    ret = pBCryptGenerateKeyPair(alg, &key, 512, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    ret = pBCryptFinalizeKeyPair(key, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    size = 0;
    ret = pBCryptExportKey(key, NULL, BCRYPT_RSAPRIVATE_BLOB, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size, "size not set\n");

    buf = malloc(size);
    ret = pBCryptExportKey(key, NULL, BCRYPT_RSAPRIVATE_BLOB, buf, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    rsablob = (BCRYPT_RSAKEY_BLOB *)buf;
    ok(rsablob->Magic == BCRYPT_RSAPRIVATE_MAGIC, "got 0x%x\n", rsablob->Magic);
    ok(rsablob->BitLength == 512, "got %u\n", rsablob->BitLength);
    ok(rsablob->cbPublicExp == 3, "got %u\n", rsablob->cbPublicExp);
    ok(rsablob->cbModulus == 64, "got %u\n", rsablob->cbModulus);
    ok(rsablob->cbPrime1 == 32, "got %u\n", rsablob->cbPrime1);
    ok(rsablob->cbPrime2 == 32, "got %u\n", rsablob->cbPrime2);
    size2 = sizeof(*rsablob) + rsablob->cbPublicExp + rsablob->cbModulus + rsablob->cbPrime1 + rsablob->cbPrime2;
    ok(size == size2, "got %u expected %u\n", size2, size);
    free(buf);

    size = 0;
    ret = pBCryptExportKey(key, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size, "size not set\n");

    buf = malloc(size);
    ret = pBCryptExportKey(key, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, buf, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    rsablob = (BCRYPT_RSAKEY_BLOB *)buf;
    ok(rsablob->Magic == BCRYPT_RSAFULLPRIVATE_MAGIC, "got 0x%x\n", rsablob->Magic);
    ok(rsablob->BitLength == 512, "got %u\n", rsablob->BitLength);
    ok(rsablob->cbPublicExp == 3, "got %u\n", rsablob->cbPublicExp);
    ok(rsablob->cbModulus == 64, "got %u\n", rsablob->cbModulus);
    ok(rsablob->cbPrime1 == 32, "got %u\n", rsablob->cbPrime1);
    ok(rsablob->cbPrime2 == 32, "got %u\n", rsablob->cbPrime2);
    size2 = sizeof(*rsablob) + rsablob->cbPublicExp + rsablob->cbModulus * 2 + rsablob->cbPrime1 * 3 + rsablob->cbPrime2 * 2;
    ok(size == size2, "got %u expected %u\n", size2, size);
    free(buf);
    pBCryptDestroyKey(key);

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(!ret, "BCryptCloseAlgorithmProvider failed: 0x%x\n", ret);
}

static BYTE eccprivkey[] =
{
    0x45, 0x43, 0x4b, 0x32, 0x20, 0x00, 0x00, 0x00,
    0xfb, 0xbd, 0x3d, 0x20, 0x1b, 0x6d, 0x66, 0xb3, 0x7c, 0x9f, 0x89, 0xf3, 0xe4, 0x41, 0x16, 0xa5,
    0x68, 0x52, 0x77, 0xac, 0xab, 0x55, 0xb2, 0x6c, 0xb0, 0x23, 0x55, 0xcb, 0x96, 0x14, 0xfd, 0x0b,
    0x1c, 0xef, 0xdf, 0x07, 0x6d, 0x31, 0xaf, 0x39, 0xce, 0x8c, 0x8f, 0x9d, 0x75, 0xd0, 0x7b, 0xea,
    0x81, 0xdc, 0x40, 0x21, 0x1f, 0x58, 0x22, 0x5f, 0x72, 0x55, 0xfc, 0x58, 0x8a, 0xeb, 0x88, 0x5d,
    0x02, 0x09, 0x90, 0xd2, 0xe3, 0x36, 0xac, 0xfe, 0x83, 0x13, 0x6c, 0x88, 0x1a, 0xab, 0x9b, 0xdd,
    0xaa, 0x8a, 0xee, 0x69, 0x9a, 0x6a, 0x62, 0x86, 0x6a, 0x13, 0x69, 0x88, 0xb7, 0xd5, 0xa3, 0xcd
};

static BYTE ecdh_pubkey[] =
{
    0x45, 0x43, 0x4b, 0x31, 0x20, 0x00, 0x00, 0x00,
    0x07, 0x61, 0x9d, 0x49, 0x63, 0x6b, 0x96, 0x94, 0xd1, 0x8f, 0xd1, 0x48, 0xcc, 0xcf, 0x72, 0x4d,
    0xff, 0x43, 0xf4, 0x97, 0x0f, 0xa3, 0x8a, 0x72, 0xe9, 0xe0, 0xba, 0x87, 0x6d, 0xc3, 0x62, 0x15,
    0xae, 0x65, 0xdd, 0x31, 0x51, 0xfc, 0x3b, 0xc9, 0x59, 0xa1, 0x0a, 0x92, 0x17, 0x2b, 0x64, 0x55,
    0x03, 0x3e, 0x62, 0x1d, 0xac, 0x3e, 0x37, 0x40, 0x6a, 0x4c, 0xb6, 0x21, 0x3f, 0x73, 0x5c, 0xf5
};

/* little endian */
static BYTE ecdh_secret[] =
{
    0x48, 0xb0, 0x11, 0xdb, 0x69, 0x4e, 0xb4, 0xf4, 0xf5, 0x3e, 0xe1, 0x9b, 0xca, 0x00, 0x04, 0xc8,
    0x9b, 0x69, 0xaf, 0xd1, 0xaf, 0x1f, 0xc2, 0xd7, 0x83, 0x0a, 0xb7, 0xf8, 0x4f, 0x24, 0x32, 0x8e,
};

BCryptBuffer hash_param_buffers[] =
{
{
    sizeof(BCRYPT_SHA1_ALGORITHM),
    KDF_HASH_ALGORITHM,
    (void *)BCRYPT_SHA1_ALGORITHM,
}
};

BCryptBufferDesc hash_params =
{
    BCRYPTBUFFER_VERSION,
    ARRAY_SIZE(hash_param_buffers),
    hash_param_buffers,
};

static BYTE hashed_secret[] =
{
    0x1b, 0xe7, 0xbf, 0x0f, 0x65, 0x1e, 0xd0, 0x07, 0xf9, 0xf4, 0x77, 0x48, 0x48, 0x39, 0xd0, 0xf8,
    0xf3, 0xce, 0xfc, 0x89
};

static void test_ECDH(void)
{
    BYTE *buf;
    BCRYPT_ECCKEY_BLOB *ecckey;
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key, privkey, pubkey;
    BCRYPT_SECRET_HANDLE secret;
    NTSTATUS status;
    ULONG size;

    status = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDH_P256_ALGORITHM, NULL, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    key = NULL;
    status = pBCryptGenerateKeyPair(alg, &key, 256, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(key != NULL, "key not set\n");

    status = pBCryptFinalizeKeyPair(key, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    size = 0;
    status = pBCryptExportKey(key, NULL, BCRYPT_ECCPUBLIC_BLOB, NULL, 0, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(size, "size not set\n");

    buf = malloc(size);
    status = pBCryptExportKey(key, NULL, BCRYPT_ECCPUBLIC_BLOB, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ecckey = (BCRYPT_ECCKEY_BLOB *)buf;
    ok(ecckey->dwMagic == BCRYPT_ECDH_PUBLIC_P256_MAGIC, "got 0x%x\n", ecckey->dwMagic);
    ok(ecckey->cbKey == 32, "got %u\n", ecckey->cbKey);
    ok(size == sizeof(*ecckey) + ecckey->cbKey * 2, "got %u\n", size);

    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_PUBLIC_KEY_BLOB, &pubkey, buf, size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    pBCryptDestroyKey(pubkey);

    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_ECCPUBLIC_BLOB, &pubkey, buf, size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    free(buf);

    size = 0;
    status = pBCryptExportKey(key, NULL, BCRYPT_ECCPRIVATE_BLOB, NULL, 0, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(size, "size not set\n");

    buf = malloc(size);
    status = pBCryptExportKey(key, NULL, BCRYPT_ECCPRIVATE_BLOB, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ecckey = (BCRYPT_ECCKEY_BLOB *)buf;
    ok(ecckey->dwMagic == BCRYPT_ECDH_PRIVATE_P256_MAGIC, "got 0x%x\n", ecckey->dwMagic);
    ok(ecckey->cbKey == 32, "got %u\n", ecckey->cbKey);
    ok(size == sizeof(*ecckey) + ecckey->cbKey * 3, "got %u\n", size);

    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_ECCPRIVATE_BLOB, &privkey, buf, size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    free(buf);
    pBCryptDestroyKey(pubkey);
    pBCryptDestroyKey(privkey);
    pBCryptDestroyKey(key);

    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_ECCPRIVATE_BLOB, &privkey, eccprivkey, sizeof(eccprivkey), 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    size = 0;
    status = pBCryptExportKey(privkey, NULL, BCRYPT_ECCPRIVATE_BLOB, NULL, 0, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(size, "size not set\n");

    buf = malloc(size);
    status = pBCryptExportKey(privkey, NULL, BCRYPT_ECCPRIVATE_BLOB, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(size == sizeof(eccprivkey), "got %u\n", size);
    ok(!memcmp(buf, eccprivkey, size), "wrong data\n");
    free(buf);

    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_ECCPUBLIC_BLOB, &pubkey, ecdh_pubkey, sizeof(ecdh_pubkey), 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    status = pBCryptSecretAgreement(privkey, pubkey, &secret, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    if (status != STATUS_SUCCESS) goto derive_end;

    status = pBCryptDeriveKey(secret, BCRYPT_KDF_RAW_SECRET, NULL, NULL, 0, &size, 0);
    if (status == STATUS_NOT_SUPPORTED) /* < win10 */
    {
        win_skip("BCRYPT_KDF_RAW_SECRET not supported\n");
        goto raw_secret_end;
    }
    todo_wine ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    if (status != STATUS_SUCCESS) goto raw_secret_end;

    ok(size == 32, "size of secret key incorrect, got %u, expected 32\n", size);
    buf = malloc(size);
    status = pBCryptDeriveKey(secret, BCRYPT_KDF_RAW_SECRET, NULL, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(!(memcmp(ecdh_secret, buf, size)), "wrong data\n");
    free(buf);

raw_secret_end:
    status = pBCryptDeriveKey(secret, BCRYPT_KDF_HASH, &hash_params, NULL, 0, &size, 0);
    todo_wine ok (status == STATUS_SUCCESS, "got 0x%x\n", status);
    if (status != STATUS_SUCCESS) goto derive_end;

    ok (size == 20, "got %u\n", size);
    buf = malloc(size);
    status = pBCryptDeriveKey(secret, BCRYPT_KDF_HASH, &hash_params, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(!(memcmp(hashed_secret, buf, size)), "wrong data\n");
    free(buf);

    /* ulVersion is not verified */
    hash_params.ulVersion = 0xdeadbeef;
    status = pBCryptDeriveKey(secret, BCRYPT_KDF_HASH, &hash_params, NULL, 0, &size, 0);
    ok (status == STATUS_SUCCESS, "got 0x%x\n", status);

    hash_params.ulVersion = BCRYPTBUFFER_VERSION;
    hash_param_buffers[0].pvBuffer = (void*) L"INVALID";
    hash_param_buffers[0].cbBuffer = sizeof(L"INVALID");

    status = pBCryptDeriveKey(secret, BCRYPT_KDF_HASH, &hash_params, NULL, 0, &size, 0);
    ok (status == STATUS_NOT_SUPPORTED || broken (status == STATUS_NOT_FOUND) /* < win8 */, "got 0x%x\n", status);

    hash_param_buffers[0].pvBuffer = (void*) BCRYPT_RNG_ALGORITHM;
    hash_param_buffers[0].cbBuffer = sizeof(BCRYPT_RNG_ALGORITHM);
    status = pBCryptDeriveKey(secret, BCRYPT_KDF_HASH, &hash_params, NULL, 0, &size, 0);
    ok (status == STATUS_NOT_SUPPORTED, "got 0x%x\n", status);

derive_end:
    pBCryptDestroySecret(secret);
    pBCryptDestroyKey(pubkey);
    pBCryptDestroyKey(privkey);
    pBCryptCloseAlgorithmProvider(alg, 0);

    status = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDH_P384_ALGORITHM, NULL, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    key = NULL;
    status = pBCryptGenerateKeyPair(alg, &key, 384, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(key != NULL, "key not set\n");

    status = pBCryptFinalizeKeyPair(key, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    size = 0;
    status = pBCryptExportKey(key, NULL, BCRYPT_ECCPUBLIC_BLOB, NULL, 0, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(size, "size not set\n");

    buf = malloc(size);
    status = pBCryptExportKey(key, NULL, BCRYPT_ECCPUBLIC_BLOB, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ecckey = (BCRYPT_ECCKEY_BLOB *)buf;
    ok(ecckey->dwMagic == BCRYPT_ECDH_PUBLIC_P384_MAGIC, "got 0x%x\n", ecckey->dwMagic);
    ok(ecckey->cbKey == 48, "got %u\n", ecckey->cbKey);
    ok(size == sizeof(*ecckey) + ecckey->cbKey * 2, "got %u\n", size);

    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_PUBLIC_KEY_BLOB, &pubkey, buf, size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    pBCryptDestroyKey(pubkey);

    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_ECCPUBLIC_BLOB, &pubkey, buf, size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    free(buf);
    pBCryptDestroyKey(pubkey);
    pBCryptCloseAlgorithmProvider(alg, 0);
}

static BYTE dh_pubkey[] =
{
    /* BCRYPT_DH_KEY_BLOB */
    0x44, 0x48, 0x50, 0x42, 0x40, 0x00, 0x00, 0x00,
    /* p */
    0xcf, 0x18, 0xe9, 0xa9, 0xb3, 0x97, 0x59, 0xae, 0x0d, 0xac, 0xf0, 0x99, 0x39, 0xdc, 0xd2, 0xfe,
    0x1e, 0xf3, 0xfc, 0x2c, 0x49, 0xdf, 0x76, 0x89, 0xff, 0x13, 0x57, 0xc7, 0xe6, 0xbd, 0xed, 0xa7,
    0x42, 0xc0, 0xc3, 0xd7, 0x8e, 0x84, 0xa8, 0xdf, 0xcd, 0x52, 0x50, 0x81, 0x73, 0x8a, 0x33, 0x60,
    0xde, 0x6d, 0x56, 0xeb, 0xd5, 0xec, 0x1f, 0x9f, 0x9f, 0xd6, 0x2c, 0xe4, 0x8f, 0xab, 0x58, 0x0b,
    /* g */
    0x5f, 0xc5, 0x50, 0x9a, 0xde, 0xf6, 0x84, 0x48, 0x39, 0xa9, 0xa7, 0xb1, 0x73, 0x0c, 0x56, 0xd4,
    0x28, 0xbb, 0x12, 0x93, 0x51, 0x44, 0x33, 0xdf, 0xa6, 0xe7, 0x7f, 0x0b, 0x3f, 0xe9, 0x41, 0xef,
    0x32, 0x80, 0xcd, 0x8e, 0x2b, 0x38, 0x85, 0x49, 0x4d, 0x0c, 0xcc, 0x74, 0x02, 0x07, 0x92, 0xd3,
    0xe4, 0x3e, 0x37, 0x84, 0x27, 0x1f, 0xa3, 0xad, 0x94, 0x8c, 0xc1, 0xc2, 0x22, 0x99, 0x36, 0xf0,
    /* y */
    0x22, 0xaf, 0x98, 0xeb, 0xd9, 0xc4, 0xb5, 0xbd, 0xe1, 0xab, 0x19, 0x1b, 0xe3, 0x36, 0x20, 0xca,
    0xff, 0xe8, 0x6c, 0x30, 0x96, 0x3c, 0x90, 0x77, 0x0e, 0xe0, 0x96, 0xae, 0xb1, 0x47, 0xd1, 0x52,
    0x2c, 0xc3, 0x65, 0x5e, 0x9b, 0x41, 0x9a, 0xa6, 0xfe, 0xab, 0x54, 0xa0, 0xf0, 0x71, 0xab, 0x6c,
    0xd0, 0x0e, 0x01, 0x08, 0x5b, 0x66, 0xe5, 0x62, 0xd2, 0xe5, 0x5d, 0xae, 0x9c, 0x60, 0xb2, 0xc6,
};

static BYTE dh_privkey[] =
{
    /* BCRYPT_DH_KEY_BLOB */
    0x44, 0x48, 0x50, 0x56, 0x40, 0x00, 0x00, 0x00,
    /* p */
    0xcf, 0x18, 0xe9, 0xa9, 0xb3, 0x97, 0x59, 0xae, 0x0d, 0xac, 0xf0, 0x99, 0x39, 0xdc, 0xd2, 0xfe,
    0x1e, 0xf3, 0xfc, 0x2c, 0x49, 0xdf, 0x76, 0x89, 0xff, 0x13, 0x57, 0xc7, 0xe6, 0xbd, 0xed, 0xa7,
    0x42, 0xc0, 0xc3, 0xd7, 0x8e, 0x84, 0xa8, 0xdf, 0xcd, 0x52, 0x50, 0x81, 0x73, 0x8a, 0x33, 0x60,
    0xde, 0x6d, 0x56, 0xeb, 0xd5, 0xec, 0x1f, 0x9f, 0x9f, 0xd6, 0x2c, 0xe4, 0x8f, 0xab, 0x58, 0x0b,
    /* g */
    0x5f, 0xc5, 0x50, 0x9a, 0xde, 0xf6, 0x84, 0x48, 0x39, 0xa9, 0xa7, 0xb1, 0x73, 0x0c, 0x56, 0xd4,
    0x28, 0xbb, 0x12, 0x93, 0x51, 0x44, 0x33, 0xdf, 0xa6, 0xe7, 0x7f, 0x0b, 0x3f, 0xe9, 0x41, 0xef,
    0x32, 0x80, 0xcd, 0x8e, 0x2b, 0x38, 0x85, 0x49, 0x4d, 0x0c, 0xcc, 0x74, 0x02, 0x07, 0x92, 0xd3,
    0xe4, 0x3e, 0x37, 0x84, 0x27, 0x1f, 0xa3, 0xad, 0x94, 0x8c, 0xc1, 0xc2, 0x22, 0x99, 0x36, 0xf0,
    /* y */
    0x22, 0xaf, 0x98, 0xeb, 0xd9, 0xc4, 0xb5, 0xbd, 0xe1, 0xab, 0x19, 0x1b, 0xe3, 0x36, 0x20, 0xca,
    0xff, 0xe8, 0x6c, 0x30, 0x96, 0x3c, 0x90, 0x77, 0x0e, 0xe0, 0x96, 0xae, 0xb1, 0x47, 0xd1, 0x52,
    0x2c, 0xc3, 0x65, 0x5e, 0x9b, 0x41, 0x9a, 0xa6, 0xfe, 0xab, 0x54, 0xa0, 0xf0, 0x71, 0xab, 0x6c,
    0xd0, 0x0e, 0x01, 0x08, 0x5b, 0x66, 0xe5, 0x62, 0xd2, 0xe5, 0x5d, 0xae, 0x9c, 0x60, 0xb2, 0xc6,
    /* x */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x97, 0x71, 0x3e, 0x82,
    0x8b, 0xea, 0x11, 0x77, 0xc4, 0xb4, 0x62, 0xc7, 0x4d, 0xff, 0x0f, 0x63, 0xd9, 0xe2, 0xda, 0xab,
};

static BYTE dh_secret[] =
{
    0x73, 0x84, 0x62, 0xc1, 0x9a, 0x9c, 0xc2, 0x91, 0x9f, 0xc1, 0xc2, 0x94, 0x0c, 0xa8, 0x2f, 0x58,
    0xac, 0x50, 0xcd, 0xd5, 0x29, 0x43, 0x41, 0x8e, 0x5d, 0xca, 0x73, 0x55, 0x4d, 0x46, 0x50, 0xe5,
    0xb7, 0x34, 0xa9, 0xcb, 0x3a, 0x18, 0x68, 0x99, 0x30, 0xef, 0x58, 0x26, 0xd3, 0x03, 0x61, 0x02,
    0x17, 0xb7, 0xba, 0x01, 0xbc, 0xae, 0xdf, 0x3f, 0xb5, 0xb5, 0x4a, 0xb0, 0x08, 0xe5, 0xea, 0xc3,
};

static BYTE dh_hashed_secret[] =
{
    0xa7, 0xfc, 0xff, 0x21, 0xb3, 0xd1, 0x46, 0xb8, 0x21, 0x3d, 0xc6, 0xd4, 0xe3, 0x61, 0x97, 0x5e,
    0xb5, 0x0a, 0xfe, 0x8f,
};

BCryptBuffer dh_hash_param_buffers[] =
{
    {
        sizeof(BCRYPT_SHA1_ALGORITHM),
        KDF_HASH_ALGORITHM,
        (void *)BCRYPT_SHA1_ALGORITHM,
    }
};

BCryptBufferDesc dh_hash_params =
{
    BCRYPTBUFFER_VERSION,
    ARRAY_SIZE(dh_hash_param_buffers),
    dh_hash_param_buffers,
};

static void test_DH(void)
{
    UCHAR hash[20];
    BCRYPT_KEY_HANDLE key, pubkey, privkey;
    BCRYPT_SECRET_HANDLE secret;
    BCRYPT_DH_KEY_BLOB *dhkey;
    NTSTATUS status;
    UCHAR *buf;
    ULONG size;

    if (!pBCryptHash) /* < Win10 */
    {
        win_skip("broken DH detected\n");
        return;
    }

    key = NULL;
    status = pBCryptGenerateKeyPair(BCRYPT_DH_ALG_HANDLE, &key, 512, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(key != NULL, "key not set\n");

    status = pBCryptFinalizeKeyPair(key, 0);
    todo_wine ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    if (status != STATUS_SUCCESS)
    {
        pBCryptDestroyKey(key);
        return;
    }

    size = 0;
    status = pBCryptExportKey(key, NULL, BCRYPT_DH_PUBLIC_BLOB, NULL, 0, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(size, "size not set\n");

    buf = malloc(size);
    status = pBCryptExportKey(key, NULL, BCRYPT_DH_PUBLIC_BLOB, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    dhkey = (BCRYPT_DH_KEY_BLOB *)buf;
    ok(dhkey->dwMagic == BCRYPT_DH_PUBLIC_MAGIC, "got 0x%x\n", dhkey->dwMagic);
    ok(dhkey->cbKey == 64, "got %u\n", dhkey->cbKey);
    ok(size == sizeof(*dhkey) + dhkey->cbKey * 3, "got %u\n", size);

    status = pBCryptImportKeyPair(BCRYPT_DH_ALG_HANDLE, NULL, BCRYPT_DH_PUBLIC_BLOB, &pubkey, buf, size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    free(buf);
    pBCryptDestroyKey(pubkey);

    size = 0;
    status = pBCryptExportKey(key, NULL, BCRYPT_DH_PRIVATE_BLOB, NULL, 0, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(size == sizeof(*dhkey) + 64 * 4, "size not set\n");

    buf = calloc(1, size);
    status = pBCryptExportKey(key, NULL, BCRYPT_DH_PRIVATE_BLOB, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    dhkey = (BCRYPT_DH_KEY_BLOB *)buf;
    ok(dhkey->dwMagic == BCRYPT_DH_PRIVATE_MAGIC, "got 0x%x\n", dhkey->dwMagic);
    ok(dhkey->cbKey == 64, "got %u\n", dhkey->cbKey);
    ok(size == sizeof(*dhkey) + dhkey->cbKey * 4, "got %u\n", size);
    pBCryptDestroyKey(key);

    status = pBCryptImportKeyPair(BCRYPT_DH_ALG_HANDLE, NULL, BCRYPT_DH_PRIVATE_BLOB, &privkey, buf, size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    free(buf);
    pBCryptDestroyKey(privkey);

    status = pBCryptImportKeyPair(BCRYPT_DH_ALG_HANDLE, NULL, BCRYPT_DH_PRIVATE_BLOB, &privkey, dh_privkey,
                                 sizeof(dh_privkey), 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    size = 0;
    status = pBCryptExportKey(privkey, NULL, BCRYPT_DH_PRIVATE_BLOB, NULL, 0, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(size, "size not set\n");

    buf = malloc(size);
    status = pBCryptExportKey(privkey, NULL, BCRYPT_DH_PRIVATE_BLOB, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(size == sizeof(dh_privkey), "got %u\n", size);
    ok(!memcmp(buf, dh_privkey, size), "wrong data\n");
    free(buf);

    status = pBCryptImportKeyPair(BCRYPT_DH_ALG_HANDLE, NULL, BCRYPT_DH_PUBLIC_BLOB, &pubkey, dh_pubkey,
                                 sizeof(dh_pubkey), 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    size = 0;
    status = pBCryptExportKey(privkey, NULL, BCRYPT_DH_PUBLIC_BLOB, NULL, 0, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(size, "size not set\n");

    buf = malloc(size);
    status = pBCryptExportKey(privkey, NULL, BCRYPT_DH_PUBLIC_BLOB, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(size == sizeof(dh_pubkey), "got %u\n", size);
    ok(!memcmp(buf, dh_pubkey, size), "wrong data\n");
    free(buf);

    status = pBCryptSignHash(privkey, NULL, hash, sizeof(hash), NULL, 0, &size, 0);
    ok(status == STATUS_NOT_SUPPORTED, "got 0x%x\n", status);

    status = pBCryptEncrypt(privkey, NULL, 0, NULL, NULL, 0, NULL, 0, &size, 0);
    ok(status == STATUS_NOT_SUPPORTED, "got 0x%x\n", status);

    status = pBCryptSecretAgreement(privkey, pubkey, &secret, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    pBCryptDestroyKey(pubkey);
    pBCryptDestroyKey(privkey);

    size = 0;
    status = pBCryptDeriveKey(secret, BCRYPT_KDF_RAW_SECRET, NULL, NULL, 0, &size, 0);
    if (status == STATUS_NOT_SUPPORTED)
    {
        win_skip("BCRYPT_KDF_RAW_SECRET not supported\n"); /* < win10 */
        pBCryptDestroySecret(secret);
        return;
    }
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(size == 64, "got %u\n", size);

    buf = calloc(1, size);
    status = pBCryptDeriveKey(secret, BCRYPT_KDF_RAW_SECRET, NULL, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(!memcmp(dh_secret, buf, size), "wrong data\n");
    free(buf);

    size = 0;
    status = pBCryptDeriveKey(secret, BCRYPT_KDF_HASH, NULL, NULL, 0, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(size == 20, "got %u\n", size);

    size = 0;
    status = pBCryptDeriveKey(secret, BCRYPT_KDF_HASH, &dh_hash_params, NULL, 0, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(size == 20, "got %u\n", size);

    buf = calloc(1, size);
    status = pBCryptDeriveKey(secret, BCRYPT_KDF_HASH, &dh_hash_params, buf, size, NULL, 0);
    ok(status == STATUS_INVALID_PARAMETER, "got 0x%x\n", status);

    status = pBCryptDeriveKey(secret, BCRYPT_KDF_HASH, &dh_hash_params, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(!memcmp(dh_hashed_secret, buf, size), "wrong data\n");
    ok(size == 20, "got %u\n", size);

    memset(buf, 0, 20);
    status = pBCryptDeriveKey(secret, BCRYPT_KDF_HASH, &dh_hash_params, buf, 10, &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(!memcmp(dh_hashed_secret, buf, size), "wrong data\n");
    ok(size == 10, "got %u\n", size);
    free(buf);

    pBCryptDestroySecret(secret);
}

#ifndef __REACTOS__
static void test_BCryptEnumContextFunctions(void)
{
    CRYPT_CONTEXT_FUNCTIONS *buffer;
    NTSTATUS status;
    ULONG buflen;

    buffer = NULL;
    status = pBCryptEnumContextFunctions( CRYPT_LOCAL, L"SSL", NCRYPT_SCHANNEL_INTERFACE, &buflen, &buffer );
    todo_wine ok( status == STATUS_SUCCESS, "got 0x%08x\n", status);
    if (status == STATUS_SUCCESS) BCryptFreeBuffer( buffer );
}
#endif /* __REACTOS__ */

static BYTE rsapublic[] =
{
    0x52, 0x53, 0x41, 0x31, 0x00, 0x08, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0xd5, 0xfe, 0xf6, 0x7a, 0x9a, 0xa1, 0x2d, 0xcf, 0x98,
    0x60, 0xca, 0x38, 0x60, 0x0b, 0x74, 0x4c, 0x7e, 0xa1, 0x42, 0x64, 0xad, 0x05, 0xa5, 0x29, 0x25, 0xcb, 0xd5,
    0x9c, 0xaf, 0x6f, 0x63, 0x85, 0x6d, 0x5b, 0x59, 0xe5, 0x17, 0x8f, 0xf9, 0x18, 0x90, 0xa7, 0x63, 0xae, 0xe0,
    0x3a, 0x62, 0xf7, 0x98, 0x57, 0xe9, 0x91, 0xda, 0xfb, 0xd9, 0x36, 0x45, 0xe4, 0x9e, 0x75, 0xf6, 0x73, 0xc4,
    0x99, 0x23, 0x21, 0x1b, 0x3d, 0xe1, 0xe0, 0xa6, 0xa0, 0x4a, 0x50, 0x2a, 0xcb, 0x2a, 0x50, 0xf0, 0x8b, 0x70,
    0x9c, 0xe4, 0x1a, 0x14, 0x3b, 0xbe, 0x35, 0xa5, 0x5a, 0x91, 0xa3, 0xa1, 0x82, 0xea, 0x84, 0x4d, 0xe8, 0x62,
    0x3b, 0x11, 0xec, 0x61, 0x09, 0x6c, 0xfe, 0xb2, 0xcc, 0x4b, 0xa8, 0xff, 0xaf, 0x73, 0x72, 0x05, 0x4e, 0x7e,
    0xe5, 0x73, 0xdf, 0x24, 0xcf, 0x7f, 0x5d, 0xaf, 0x8a, 0xf0, 0xd8, 0xcb, 0x08, 0x1e, 0xf2, 0x36, 0x70, 0x8d,
    0x1b, 0x9e, 0xc8, 0x98, 0x60, 0x54, 0xeb, 0x45, 0x34, 0x21, 0x43, 0x4d, 0x42, 0x0a, 0x3a, 0x2d, 0x0f, 0x0e,
    0xd6, 0x0d, 0xe4, 0x2e, 0x8c, 0x31, 0x87, 0xa8, 0x09, 0x89, 0x61, 0x16, 0xca, 0x5b, 0xbe, 0x76, 0x69, 0xbb,
    0xfd, 0x91, 0x63, 0xd2, 0x66, 0x57, 0x08, 0xef, 0xe2, 0x40, 0x67, 0xd7, 0x7f, 0x50, 0x15, 0x42, 0x33, 0x97,
    0x54, 0x73, 0x47, 0xe7, 0x9c, 0x14, 0xa8, 0xb0, 0x3d, 0xc9, 0x23, 0xb0, 0x27, 0x3b, 0xe7, 0xdd, 0x5f, 0xd1,
    0x4f, 0x31, 0x10, 0x7d, 0xdd, 0x69, 0x8e, 0xde, 0xa3, 0xe8, 0x92, 0x00, 0xfa, 0xa5, 0xa4, 0x40, 0x51, 0x23,
    0x82, 0x84, 0xc7, 0xce, 0x19, 0x61, 0x26, 0xf1, 0xae, 0xf3, 0x90, 0x93, 0x98, 0x56, 0x23, 0x9a, 0xd1, 0xbd,
    0xf2, 0xdf, 0xfd, 0x13, 0x9c, 0x30, 0x07, 0xf9, 0x5a, 0x2e, 0x00, 0xc6, 0x1f
};

static void test_BCryptSignHash(void)
{
    static UCHAR hash[] =
        {0x7e,0xe3,0x74,0xe7,0xc5,0x0b,0x6b,0x70,0xdb,0xab,0x32,0x6d,0x1d,0x51,0xd6,0x74,0x79,0x8e,0x5b,0x4b};
    static UCHAR hash_sha256[] =
        {0x25,0x2f,0x10,0xc8,0x36,0x10,0xeb,0xca,0x1a,0x05,0x9c,0x0b,0xae,0x82,0x55,0xeb,0xa2,0xf9,0x5b,0xe4,
         0xd1,0xd7,0xbC,0xfA,0x89,0xd7,0x24,0x8a,0x82,0xd9,0xf1,0x11};
    BCRYPT_PKCS1_PADDING_INFO pad;
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key;
    UCHAR sig[256];
    NTSTATUS ret;
    ULONG len;

    /* RSA */
    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_RSA_ALGORITHM, NULL, 0);
    ok(!ret, "got 0x%x\n", ret);

    /* public key */
    ret = pBCryptImportKeyPair(alg, NULL, BCRYPT_RSAPUBLIC_BLOB, &key, rsapublic, sizeof(rsapublic), 0);
    ok(!ret, "got 0x%x\n", ret);

    len = 0;
    pad.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    ret = pBCryptSignHash(key, &pad, NULL, 0, NULL, 0, &len, BCRYPT_PAD_PKCS1);
    ok(!ret, "got 0x%x\n", ret);
    ok(len == 256, "got %u\n", len);

    /* test len return when only output is NULL, as described in BCryptSignHash doc */
    ret = pBCryptSignHash(key, &pad, hash, sizeof(hash), NULL, 0, &len, BCRYPT_PAD_PKCS1);
    ok(!ret, "got 0x%x\n", ret);
    ok(len == 256, "got %u\n", len);

    len = 0;
    ret = pBCryptSignHash(key, &pad, hash, sizeof(hash), sig, sizeof(sig), &len, BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_INVALID_PARAMETER || broken(ret == STATUS_INTERNAL_ERROR) /* < win7 */, "got 0x%x\n", ret);
    ret = pBCryptDestroyKey(key);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptGenerateKeyPair(alg, &key, 512, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    ret = pBCryptFinalizeKeyPair(key, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    ret = pBCryptFinalizeKeyPair(key, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got 0x%x\n", ret);

    len = 0;
    memset(sig, 0, sizeof(sig));

    /* inference of padding info on RSA not supported */
    ret = pBCryptSignHash(key, NULL, hash, sizeof(hash), sig, sizeof(sig), &len, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);

    ret = pBCryptSignHash(key, &pad, hash, sizeof(hash), sig, 0, &len, BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got 0x%x\n", ret);

    ret = pBCryptSignHash(key, &pad, hash, sizeof(hash), sig, sizeof(sig), &len, BCRYPT_PAD_PKCS1);
    ok(!ret, "got 0x%x\n", ret);
    ok(len == 64, "got %u\n", len);

    ret = pBCryptVerifySignature(key, &pad, hash, sizeof(hash), sig, len, BCRYPT_PAD_PKCS1);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptDestroyKey(key);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(!ret, "got 0x%x\n", ret);

    /* ECDSA */
    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptGenerateKeyPair(alg, &key, 256, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    ret = pBCryptFinalizeKeyPair(key, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    memset(sig, 0, sizeof(sig));
    len = 0;

    /* automatically detects padding info */
    ret = pBCryptSignHash(key, NULL, hash, sizeof(hash), sig, sizeof(sig), &len, 0);
    ok (!ret, "got 0x%x\n", ret);
    ok (len == 64, "got %u\n", len);

    ret = pBCryptVerifySignature(key, NULL, hash, sizeof(hash), sig, len, 0);
    ok(!ret, "got 0x%x\n", ret);

    /* mismatch info (SHA-1 != SHA-256) */
    ret  = pBCryptSignHash(key, &pad, hash_sha256, sizeof(hash_sha256), sig, sizeof(sig), &len, BCRYPT_PAD_PKCS1);
    ok (ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);

    ret = pBCryptDestroyKey(key);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(!ret, "got 0x%x\n", ret);
}

static void test_BCryptEnumAlgorithms(void)
{
    BCRYPT_ALGORITHM_IDENTIFIER *list;
    NTSTATUS ret;
    ULONG count, op;

    ret = pBCryptEnumAlgorithms(0, NULL, NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);

    ret = pBCryptEnumAlgorithms(0, &count, NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);

    ret = pBCryptEnumAlgorithms(0, NULL, &list, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);

    ret = pBCryptEnumAlgorithms(~0u, &count, &list, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);

    count = 0;
    list = NULL;
    ret = pBCryptEnumAlgorithms(0, &count, &list, 0);
    ok(!ret, "got 0x%x\n", ret);
    ok(list != NULL, "NULL list\n");
    ok(count, "got %u\n", count);
    pBCryptFreeBuffer( list );

    op = BCRYPT_CIPHER_OPERATION | BCRYPT_ASYMMETRIC_ENCRYPTION_OPERATION | BCRYPT_SIGNATURE_OPERATION |
         BCRYPT_SECRET_AGREEMENT_OPERATION;
    count = 0;
    list = NULL;
    ret = pBCryptEnumAlgorithms(op, &count, &list, 0);
    ok(!ret, "got 0x%x\n", ret);
    ok(list != NULL, "NULL list\n");
    ok(count, "got %u\n", count);
    pBCryptFreeBuffer( list );
}

static void test_aes_vector(void)
{
    static const UCHAR secret[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10};
    static const UCHAR expect[] = {0xb0,0xcb,0xf5,0x80,0xd4,0xe3,0x55,0x23,0x6e,0x19,0x5b,0xdb,0xfe,0xe0,0x6c,0xd3};
    static const UCHAR expect2[] = {0x06,0x0c,0x81,0xab,0xd4,0x28,0x80,0x42,0xce,0x30,0x56,0x17,0x15,0x00,0x9e,0xc1};
    static const UCHAR expect3[] = {0x3e,0x99,0xbf,0x02,0xf5,0xd3,0xb8,0x81,0x91,0x4d,0x93,0xea,0xd4,0x92,0x93,0x46};
    static UCHAR iv[16], input[] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p'};
    UCHAR output[16];
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key;
    UCHAR data[sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + sizeof(secret)];
    BCRYPT_KEY_DATA_BLOB_HEADER *blob = (BCRYPT_KEY_DATA_BLOB_HEADER *)data;
    ULONG size;
    NTSTATUS ret;

    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, NULL, 0);
    ok(!ret, "got 0x%x\n", ret);

    size = sizeof(BCRYPT_CHAIN_MODE_CBC);
    ret = pBCryptSetProperty(alg, BCRYPT_CHAINING_MODE, (UCHAR *)BCRYPT_CHAIN_MODE_CBC, size, 0);
    ok(!ret, "got 0x%x\n", ret);

    blob->dwMagic   = BCRYPT_KEY_DATA_BLOB_MAGIC;
    blob->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
    blob->cbKeyData = sizeof(secret);
    memcpy(data + sizeof(*blob), secret, sizeof(secret));
    size = sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + sizeof(secret);
    ret = pBCryptImportKey(alg, NULL, BCRYPT_KEY_DATA_BLOB, &key, NULL, 0, data, size, 0);
    ok(!ret, "got 0x%x\n", ret);

    /* zero initialization vector */
    size = 0;
    memset(output, 0, sizeof(output));
    ret = pBCryptEncrypt(key, input, sizeof(input), NULL, iv, sizeof(iv), output, sizeof(output), &size, 0);
    ok(!ret, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(output, expect, sizeof(expect)), "wrong cipher text\n");

    /* same initialization vector */
    size = 0;
    memset(output, 0, sizeof(output));
    ret = pBCryptEncrypt(key, input, sizeof(input), NULL, iv, sizeof(iv), output, sizeof(output), &size, 0);
    ok(!ret, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(output, expect2, sizeof(expect2)), "wrong cipher text\n");

    /* different initialization vector */
    iv[0] = 0x1;
    size = 0;
    memset(output, 0, sizeof(output));
    ret = pBCryptEncrypt(key, input, sizeof(input), NULL, iv, sizeof(iv), output, sizeof(output), &size, 0);
    ok(!ret, "got 0x%x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(output, expect3, sizeof(expect3)), "wrong cipher text\n");

    ret = pBCryptDestroyKey(key);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(!ret, "got 0x%x\n", ret);
}

static void test_BcryptDeriveKeyCapi(void)
{
    static const UCHAR expect[] =
        {0xda,0x39,0xa3,0xee,0x5e,0x6b,0x4b,0x0d,0x32,0x55,0xbf,0xef,0x95,0x60,0x18,0x90,0xaf,0xd8,0x07,0x09};
    static const UCHAR expect2[] =
        {0x9b,0x03,0x17,0x41,0xf4,0x75,0x11,0xac,0xff,0x22,0xee,0x40,0xbb,0xe8,0xf9,0x74,0x17,0x26,0xb6,0xf2,
         0xf8,0xc7,0x88,0x02,0x9a,0xdc,0x0d,0xd7,0x83,0x58,0xea,0x65,0x2e,0x8b,0x85,0xc6,0xdb,0xb7,0xed,0x1c};
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_HASH_HANDLE hash;
    UCHAR key[40];
    NTSTATUS ret;

    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA1_ALGORITHM, NULL, 0);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptDeriveKeyCapi(NULL, NULL, NULL, 0, 0);
    ok(ret == STATUS_INVALID_PARAMETER || ret == STATUS_INVALID_HANDLE /* win7 */, "got 0x%x\n", ret);

    ret = pBCryptDeriveKeyCapi(hash, NULL, NULL, 0, 0);
    ok(ret == STATUS_INVALID_PARAMETER || !ret /* win7 */, "got 0x%x\n", ret);

    ret = pBCryptDestroyHash(hash);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptDeriveKeyCapi(hash, NULL, key, 0, 0);
    ok(ret == STATUS_INVALID_PARAMETER || !ret /* win7 */, "got 0x%x\n", ret);

    ret = pBCryptDestroyHash(hash);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0);
    ok(!ret, "got 0x%x\n", ret);

    memset(key, 0, sizeof(key));
    ret = pBCryptDeriveKeyCapi(hash, NULL, key, 41, 0);
    ok(ret == STATUS_INVALID_PARAMETER || !ret /* win7 */, "got 0x%x\n", ret);
    if (!ret)
        ok(!memcmp(key, expect, sizeof(expect) - 1), "wrong key data\n");

    ret = pBCryptDestroyHash(hash);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0);
    ok(!ret, "got 0x%x\n", ret);

    memset(key, 0, sizeof(key));
    ret = pBCryptDeriveKeyCapi(hash, NULL, key, 20, 0);
    ok(!ret, "got 0x%x\n", ret);
    ok(!memcmp(key, expect, sizeof(expect) - 1), "wrong key data\n");

    ret = pBCryptDeriveKeyCapi(hash, NULL, key, 20, 0);
    todo_wine ok(ret == STATUS_INVALID_HANDLE, "got 0x%x\n", ret);

    ret = pBCryptHashData(hash, NULL, 0, 0);
    todo_wine ok(ret == STATUS_INVALID_HANDLE, "got 0x%x\n", ret);

    ret = pBCryptDestroyHash(hash);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptHashData(hash, (UCHAR *)"test", 4, 0);
    ok(!ret, "got 0x%x\n", ret);

    /* padding */
    memset(key, 0, sizeof(key));
    ret = pBCryptDeriveKeyCapi(hash, NULL, key, 40, 0);
    ok(!ret, "got 0x%x\n", ret);
    ok(!memcmp(key, expect2, sizeof(expect2) - 1), "wrong key data\n");

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(!ret, "got 0x%x\n", ret);
}

static UCHAR dsaHash[] =
{
    0x7e,0xe3,0x74,0xe7,0xc5,0x0b,0x6b,0x70,0xdb,0xab,0x32,0x6d,0x1d,0x51,0xd6,0x74,0x79,0x8e,0x5b,0x4b
};

static UCHAR dsaSignature[] =
{
    0x5f,0x95,0x1f,0x08,0x19,0x44,0xa5,0xab,0x28,0x11,0x51,0x68,0x82,0x9b,0xe4,0xc3,0x04,0x1b,0xc9,0xdc,
    0x41,0x2a,0x89,0xd4,0x4a,0x8b,0x86,0xaf,0x98,0x2c,0x59,0x0b,0xd2,0x88,0xf6,0xe8,0x29,0x13,0x84,0x49
};

static UCHAR dsaPublicBlob[] =
{
    0x44,0x53,0x50,0x42,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x8f,0xd2,0x92,0xbb,0x92,0xb9,0x00,0xc5,0xed,
    0x52,0xcc,0x48,0x4a,0x44,0x1d,0xd3,0x74,0xfb,0x75,0xd1,0x7e,0xb6,0x24,0x9b,0x5d,0x57,0x0a,0x8a,0xc4,
    0x5d,0xab,0x9c,0x26,0x86,0xc6,0x25,0x16,0x20,0xf9,0xa9,0x71,0xbc,0x1d,0x30,0xc4,0xef,0x8c,0xc4,0xdf,
    0x1a,0xaf,0x96,0xdf,0x90,0xd8,0x85,0x9d,0xf9,0x2c,0x86,0x8c,0x91,0x39,0x6c,0x6d,0x11,0x4e,0x53,0x63,
    0x2a,0x2b,0x26,0xa7,0xf9,0x76,0x74,0x51,0xbf,0x08,0x87,0x6f,0xe0,0x71,0x91,0x24,0x8a,0xc2,0x84,0x2d,
    0x84,0x9c,0x5f,0x94,0xaa,0x38,0x53,0x77,0x84,0xba,0xbc,0xff,0x49,0x3a,0x08,0x0f,0x38,0xb5,0x91,0x5c,
    0x06,0x15,0xa4,0x27,0xf4,0xa5,0x59,0xaa,0x1c,0x41,0xa3,0xa0,0xbb,0xf7,0x32,0x86,0xfb,0x94,0x41,0xff,
    0xcd,0xed,0x69,0xeb,0xc6,0x5e,0xb6,0xa8,0x15,0x82,0x3b,0x60,0x1e,0x91,0x55,0xd5,0x2c,0xa5,0x74,0x5a,
    0x65,0x8f,0xc6,0x56,0xc4,0x3f,0x4e,0xe3,0x3a,0x71,0xb2,0x63,0x66,0xa4,0x0d,0x0d,0xf9,0xdd,0x1e,0x48,
    0x81,0xe9,0xbf,0x8f,0xbb,0x85,0x47,0x81,0x68,0x11,0xb5,0x91,0x6b,0xc4,0x05,0xef,0xa3,0xc7,0xbf,0x26,
    0x53,0x4f,0xc4,0x10,0xfd,0xfa,0xed,0x61,0x64,0xd6,0x2e,0xad,0x04,0x3e,0x82,0xed,0xb2,0x22,0x76,0xd0,
    0x44,0xad,0xc1,0x4c,0xde,0x33,0xa3,0x61,0x55,0xec,0x24,0xe5,0x79,0x45,0xcf,0x94,0x39,0x92,0x9f,0xd8,
    0x24,0xce,0x85,0xb9
};

static UCHAR dssKey[] =
{
    0x07,0x02,0x00,0x00,0x00,0x22,0x00,0x00,0x44,0x53,0x53,0x32,0x00,0x04,0x00,0x00,0x01,0xd1,0xfc,0x7a,
    0x70,0x53,0xb2,0x48,0x70,0x23,0x19,0x1f,0x3c,0xe1,0x26,0x14,0x7e,0x9f,0x0f,0x7f,0x33,0x5e,0x2b,0xf7,
    0xca,0x01,0x74,0x8c,0xb4,0xfd,0xf6,0x44,0x95,0x35,0x56,0xaa,0x4d,0x62,0x48,0xe2,0xd1,0xa2,0x7e,0x6e,
    0xeb,0xd6,0xcc,0x7c,0xe8,0xfd,0x21,0x9a,0xa2,0xfd,0x7a,0x9d,0x1a,0x38,0x69,0x87,0x39,0x5a,0x91,0xc0,
    0x52,0x2b,0x9f,0x2a,0x54,0x78,0x37,0x82,0x9a,0x70,0x57,0xab,0xec,0x93,0x8e,0xac,0x73,0x04,0xe8,0x53,
    0x72,0x72,0x32,0xc6,0xcb,0xef,0x47,0x98,0x3c,0x56,0x49,0x62,0xcb,0xbb,0xe7,0x34,0x84,0xa6,0x72,0x3a,
    0xbe,0x26,0x46,0x86,0xca,0xcb,0x35,0x62,0x4f,0x19,0x18,0x0b,0xb0,0x78,0xae,0xd5,0x42,0xdf,0x26,0xdb,
    0x85,0x63,0x77,0x85,0x01,0x3b,0x32,0xbe,0x5c,0xf8,0x05,0xc8,0xde,0x17,0x7f,0xb9,0x03,0x82,0xfa,0xf1,
    0x9e,0x32,0x73,0xfa,0x8d,0xea,0xa3,0x30,0x48,0xe2,0xdf,0x5a,0xcb,0x83,0x3d,0xff,0x56,0xe9,0xc0,0x94,
    0xf8,0x6d,0xb3,0xaf,0x4a,0x97,0xb9,0x43,0x0e,0xd4,0x28,0x98,0x57,0x2e,0x3a,0xca,0xde,0x6f,0x45,0x0d,
    0xfb,0x58,0xec,0x78,0x34,0x2e,0x46,0x4d,0xfe,0x98,0x02,0xbb,0xef,0x07,0x1a,0x13,0xb6,0xc2,0x2c,0x06,
    0xd9,0x0c,0xc4,0xb0,0x4c,0x3a,0xfc,0x01,0x63,0xb5,0x5a,0x5d,0x2d,0x9c,0x47,0x04,0x67,0x51,0xf2,0x52,
    0xf5,0x82,0x36,0xeb,0x6e,0x66,0x58,0x4c,0x10,0x2c,0x29,0x72,0x4a,0x6f,0x6b,0x6c,0xe0,0x93,0x31,0x42,
    0xf6,0xda,0xfa,0x5b,0x22,0x43,0x9b,0x1a,0x98,0x71,0xe7,0x41,0x74,0xe9,0x12,0xa4,0x1f,0x27,0x0a,0x63,
    0x94,0x49,0xd7,0xad,0xa5,0xc4,0x5c,0xc3,0xc9,0x70,0xb3,0x7b,0x16,0xb6,0x1d,0xd4,0x09,0xc4,0x9a,0x46,
    0x2d,0x0e,0x75,0x07,0x31,0x7b,0xed,0x45,0xcd,0x99,0x84,0x14,0xf1,0x01,0x00,0x00,0x93,0xd5,0xa3,0xe4,
    0x34,0x05,0xeb,0x98,0x3b,0x5f,0x2f,0x11,0xa4,0xa5,0xc4,0xff,0xfb,0x22,0x7c,0x54
};

static void test_DSA(void)
{
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key;
    BCRYPT_DSA_KEY_BLOB *dsablob;
    UCHAR sig[40], schemes;
    ULONG len, size;
    NTSTATUS ret;
    BYTE *buf, buf2[sizeof(BCRYPT_DSA_KEY_BLOB) + sizeof(dsaPublicBlob)];

    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_DSA_ALGORITHM, NULL, 0);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptGetProperty(alg, L"PaddingSchemes", (UCHAR *)&schemes, sizeof(schemes), &size, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got 0x%x\n", ret);

    ret = pBCryptImportKeyPair(alg, NULL, BCRYPT_PUBLIC_KEY_BLOB, &key, dsaPublicBlob, sizeof(dsaPublicBlob), 0);
    ok(!ret, "got 0x%x\n", ret);
    pBCryptDestroyKey(key);

    ret = pBCryptImportKeyPair(alg, NULL, BCRYPT_DSA_PUBLIC_BLOB, &key, dsaPublicBlob, sizeof(dsaPublicBlob), 0);
    ok(!ret, "got 0x%x\n", ret);

    memset(buf2, 0xcc, sizeof(buf2));
    ret = pBCryptExportKey(key, NULL, BCRYPT_DSA_PUBLIC_BLOB, buf2, sizeof(buf2), &size, 0);
    ok(!ret, "got 0x%x\n", ret);
    dsablob = (BCRYPT_DSA_KEY_BLOB *)buf2;
    ok(dsablob->dwMagic == BCRYPT_DSA_PUBLIC_MAGIC, "got 0x%x\n", dsablob->dwMagic);
    ok(dsablob->cbKey == 64, "got %u\n", dsablob->cbKey);
    ok(size == sizeof(*dsablob) + dsablob->cbKey * 3, "got %u\n", size);

    ret = pBCryptExportKey(key, NULL, BCRYPT_DSA_PRIVATE_BLOB, buf2, sizeof(buf2), &size, 0);
    todo_wine ok(ret == STATUS_INVALID_PARAMETER, "got 0x%x\n", ret);

    ret = pBCryptVerifySignature(key, NULL, dsaHash, sizeof(dsaHash), dsaSignature, sizeof(dsaSignature), 0);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptDestroyKey(key);
    ok(!ret, "got 0x%x\n", ret);

    /* sign/verify with export/import round-trip */
    ret = pBCryptGenerateKeyPair(alg, &key, 512, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    ret = pBCryptFinalizeKeyPair(key, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);

    len = 0;
    memset(sig, 0, sizeof(sig));
    ret = pBCryptSignHash(key, NULL, dsaHash, sizeof(dsaHash), sig, sizeof(sig), &len, 0);
    ok(!ret, "got 0x%x\n", ret);
    ok(len == 40, "got %u\n", len);

    size = 0;
    ret = pBCryptExportKey(key, NULL, BCRYPT_DSA_PUBLIC_BLOB, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size, "size not set\n");

    buf = calloc(1, size);
    ret = pBCryptExportKey(key, NULL, BCRYPT_DSA_PUBLIC_BLOB, buf, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    dsablob = (BCRYPT_DSA_KEY_BLOB *)buf;
    ok(dsablob->dwMagic == BCRYPT_DSA_PUBLIC_MAGIC, "got 0x%x\n", dsablob->dwMagic);
    ok(dsablob->cbKey == 64, "got %u\n", dsablob->cbKey);
    ok(size == sizeof(*dsablob) + dsablob->cbKey * 3, "got %u\n", size);

    ret = pBCryptDestroyKey(key);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptImportKeyPair(alg, NULL, BCRYPT_DSA_PUBLIC_BLOB, &key, buf, size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    free(buf);

    ret = pBCryptVerifySignature(key, NULL, dsaHash, sizeof(dsaHash), sig, len, 0);
    ok(!ret, "got 0x%x\n", ret);
    ret = pBCryptDestroyKey(key);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptImportKeyPair(alg, NULL, LEGACY_DSA_V2_PRIVATE_BLOB, &key, dssKey, sizeof(dssKey), 0);
    ok(!ret, "got 0x%x\n", ret);

    size = 0;
    ret = pBCryptExportKey(key, NULL, LEGACY_DSA_V2_PRIVATE_BLOB, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size, "size not set\n");

    buf = calloc(1, size);
    ret = pBCryptExportKey(key, NULL, LEGACY_DSA_V2_PRIVATE_BLOB, buf, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got 0x%x\n", ret);
    ok(size == sizeof(dssKey), "got %u expected %Iu\n", size, sizeof(dssKey));
    ok(!memcmp(dssKey, buf, size), "wrong data\n");
    free(buf);

    ret = pBCryptDestroyKey(key);
    ok(!ret, "got 0x%x\n", ret);

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(!ret, "got 0x%x\n", ret);
}

static void test_SecretAgreement(void)
{
    static BCryptBuffer hash_param_buffers[] =
    {
        {
            sizeof(BCRYPT_SHA256_ALGORITHM),
            KDF_HASH_ALGORITHM,
            (void *)BCRYPT_SHA256_ALGORITHM,
        }
    };
    static BCryptBufferDesc hash_params =
    {
        BCRYPTBUFFER_VERSION,
        ARRAY_SIZE(hash_param_buffers),
        hash_param_buffers,
    };

    static const ULONG dh_private_key[] =
    {
        0xc4caf69c, 0x57b4db27, 0x36f7135f, 0x5ccba686, 0xc37b8819, 0x1d35c9b2, 0xbb07a1cf, 0x0c5d1c1b,
        0xc79acb10, 0x31dfdabb, 0x702e02b9, 0x1efab345, 0x262a8074, 0x5edf7698, 0x9b9dc630, 0x13c34b93,
        0xacbc928b, 0xb79eed8c, 0x7413dce9, 0xa5521280, 0x88d8e695, 0xa310269f, 0xca7c5719, 0xcd0c775b,
        0x9a6e2cf2, 0x9e235c51, 0xf49db62d, 0x28e72424, 0x4a44da5a, 0x3d98268d, 0x8e4d2be3, 0x254e44e6,

        0x18a67e55, 0x572e13a1, 0x46f81ca8, 0xc331c9b9, 0xf8fe3dd4, 0x8a889e5a, 0x6c0505fd, 0xbd97a121,
        0xed2dbd67, 0xf39efa8e, 0x36f9c287, 0xf6bbfa6c, 0x461e42ad, 0x17dc170e, 0xc002dc2e, 0x4813d9a4,
        0x0b6fabb8, 0x6a9e1860, 0xa8a8cbd9, 0xb7ed6b5d, 0xabb34d23, 0xf2fbe1fd, 0x8670df1e, 0xba7fa4e6,
        0xf7039712, 0x94448f30, 0xe10c812e, 0x3e311976, 0xcfdd72c4, 0xbdbea98f, 0xc9a540d6, 0x89646d57,

        0x7ab63b33, 0x03a1e9b6, 0x947f7a9b, 0x5ae59eeb, 0x1d12eb05, 0x3f425d92, 0xe028c6ba, 0xbf90ddc9,
        0xb554f55a, 0x7aeb88b6, 0x4a443a5f, 0xbab35111, 0x82c78a0c, 0x298dd482, 0x02937cb1, 0xc94cdc2e,
        0x59b010eb, 0x3bbc0a2b, 0xd845fee0, 0x04c1d0db, 0x0c8c9424, 0x1cafd4b2, 0x9aa7aed9, 0x6a478486,
        0xa8841fd7, 0xbfeff40a, 0x8fd7bcc5, 0x3bb28977, 0x2b9a7955, 0xa55cd2e4, 0x1b6ad657, 0x067cdf21,

        0x06f36920, 0x63280e1b, 0xf17d930f, 0xa06e74a8, 0x463b3a6f, 0x2a464507, 0x93f8a982, 0x8f620a7d,
        0xeda32d11, 0x9706a6d4, 0x33dce588, 0x75a1c446, 0x048ab567, 0xd735aafa, 0x806f7c1c, 0xdcb9651a,
        0x26acf3b4, 0x45f91cc9, 0x2a0de6fc, 0xf3c03d0c, 0xf5aee0aa, 0x3eeaaf36, 0x18ccee61, 0x83faa783,
        0x4b2b5250, 0xf4ccea22, 0x5ac0714b, 0x3f0b2bc6, 0x481b13ce, 0x12040ea7, 0x66e0bbed, 0x158e1a67,
    };
    static const ULONG dh_private_key2[] =
    {
        0xffffffff, 0xffffffff, 0xa2da0fc9, 0x34c26821, 0x8b62c6c4, 0xd11cdc80, 0x084e0229, 0x74cc678a,
        0xa6be0b02, 0x229b133b, 0x79084a51, 0xdd04348e, 0xb31995ef, 0x1b433acd, 0x6d0a2b30, 0x37145ff2,
        0x6d35e14f, 0x45c2516d, 0x76b585e4, 0xc67e5e62, 0xe9424cf4, 0x6bed37a6, 0xb65cff0b, 0xedb706f4,
        0xfb6b38ee, 0xa59f895a, 0x11249fae, 0xe61f4b7c, 0x51662849, 0x8153e6ec, 0xffffffff, 0xffffffff,

        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x02000000,

        0xa0c3c734, 0xc130c92d, 0x5265abf8, 0xff409f17, 0xbcdce187, 0xff64dae3, 0x170560aa, 0xb2423ed8,
        0x9ee5a8b9, 0x92548030, 0x02bba1f9, 0x823e39a4, 0x69c438f5, 0xf91016ac, 0x89bfd166, 0x7f996446,
        0x86224203, 0x15bf689c, 0x619354a4, 0x0c1d3a1f, 0x11bcf3d2, 0x58aae029, 0x41c69824, 0x3fafc179,
        0xa742747c, 0x60658c7a, 0xd3b0bde4, 0x78d3f08b, 0x6cefa061, 0x33752536, 0xe84d4901, 0x48cd73f4,

        0x8d449700, 0x1f95120e, 0xceb31745, 0x3663177b, 0xbd9bb2d5, 0x9c23c0d9, 0x814d34f8, 0xbc54edb0,
        0xb874659a, 0x3bac8a30, 0xa1f3dd46, 0x1705c900, 0xbc46fefe, 0x7d13875b, 0x3064351a, 0x4bd89a1c,
        0x9e938761, 0x931949db, 0x34490719, 0x84fb08ca, 0xa9dd355a, 0x5b3f5061, 0x2ac96663, 0xc594429e,
        0xbe58395d, 0x2f7d872a, 0x303d37b3, 0xa3a9b606, 0x735a6732, 0xa095bd95, 0x3d55a7c3, 0x00e54635,
    };
    static const ULONG dh_peer_key[] =
    {
        0xffffffff, 0xffffffff, 0xa2da0fc9, 0x34c26821, 0x8b62c6c4, 0xd11cdc80, 0x084e0229, 0x74cc678a,
        0xa6be0b02, 0x229b133b, 0x79084a51, 0xdd04348e, 0xb31995ef, 0x1b433acd, 0x6d0a2b30, 0x37145ff2,
        0x6d35e14f, 0x45c2516d, 0x76b585e4, 0xc67e5e62, 0xe9424cf4, 0x6bed37a6, 0xb65cff0b, 0xedb706f4,
        0xfb6b38ee, 0xa59f895a, 0x11249fae, 0xe61f4b7c, 0x51662849, 0x8153e6ec, 0xffffffff, 0xffffffff,

        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x02000000,

        0x3bf7404b, 0x6284fffe, 0x97c0d565, 0xd830c658, 0xcc21bf39, 0xcae45bb6, 0x019df7df, 0xbf4cd293,
        0x6bf1989d, 0x78a81f52, 0xa4ed861c, 0x6bacf493, 0xa3e700d1, 0xd06cc206, 0x411b9727, 0x01e9c9ab,
        0x9b7e6efa, 0xf46bb25d, 0xd1027242, 0x6130787c, 0xa7b87d8b, 0xfee41492, 0x50db6213, 0x321199b6,
        0x7dace53a, 0xe8b1ec51, 0x2181b113, 0x3b33e3c0, 0x5b3a2d67, 0xbd34f0c1, 0x7037c542, 0x4a8d5540,
    };
    static const ULONG dh_shared_secret_raw[] =
    {
        0x375d89b5, 0x35a9c270, 0xfbc5ba82, 0x09eb3069, 0xd50965b0, 0xace510f7, 0x981e8731, 0x80a76115,
        0xf386d348, 0xca17b8df, 0x0b0e84ec, 0xf81f756e, 0x5030fa20, 0x03113b71, 0x97b7e879, 0x899b5fae,
        0xe6913299, 0x09270076, 0x39bc813a, 0xde3ef070, 0x65ad5b3a, 0x2b7c4ba4, 0x86c98ef9, 0x3236feaf,
        0x3e0253f7, 0x0489d2dd, 0x97669a3d, 0x50242fca, 0x5d4aecb1, 0xcf2d805f, 0x2258afff, 0x750e92cd,
    };
    static const ULONG dh_shared_secret_raw2[] =
    {
        0x0815f37d, 0x19ee74ab, 0x9f63f123, 0xe1b3f10c, 0xbcc9be83, 0xaddf5b9d, 0x28174e72, 0xf8a33825,
        0xfc74e47d, 0x2c950888, 0xf5b776d9, 0xfc712fef, 0x5b213b32, 0x489a9829, 0xfc0a4d1d, 0x6e641d3b,
        0x3bb2ff57, 0x63500318, 0x081ee54f, 0xf33a2805, 0xb3759e98, 0xa9a64afe, 0x964b8897, 0x04691bbc,
        0x80f4aae1, 0x617405ee, 0xab71724d, 0x6c10c214, 0x6f60b96f, 0xdc777b0b, 0x22f40d4f, 0x8a1c4eb5,
    };
    static const ULONG dh_shared_secret_sha1[] =
    {
        0x0babba9c, 0x0bdeacbd, 0x04e36574, 0xdd504dcd, 0x0cd88db0,
    };
    static const ULONG dh_shared_secret_sha256[] =
    {
        0x3213db5b, 0x8cc8250b, 0xc829eaab, 0x00933709, 0x68160aa9, 0xfb9f1e20, 0xf92368e6, 0x2b8e18eb,
    };
    static const ULONG length = 1024;
    BCRYPT_DH_PARAMETER_HEADER *dh_header;
    BCRYPT_DH_KEY_BLOB *dh_key_blob;
    BCRYPT_SECRET_HANDLE secret, secret2;
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key, key2;
    UCHAR buffer[2048];
    NTSTATUS status;
    ULONG size, i;

    status = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDH_P256_ALGORITHM, NULL, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    key = NULL;
    status = pBCryptGenerateKeyPair(alg, &key, 256, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(key != NULL, "key not set\n");

    status = pBCryptFinalizeKeyPair(key, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    status = pBCryptSecretAgreement(NULL, key, &secret, 0);
    ok(status == STATUS_INVALID_HANDLE, "got 0x%x\n", status);

    status = pBCryptSecretAgreement(key, NULL, &secret, 0);
    ok(status == STATUS_INVALID_HANDLE, "got 0x%x\n", status);

    status = pBCryptSecretAgreement(key, key, NULL, 0);
    ok(status == STATUS_INVALID_PARAMETER, "got 0x%x\n", status);

    status = pBCryptSecretAgreement(key, key, &secret, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    status = pBCryptDeriveKey(NULL, L"HASH", NULL, NULL, 0, &size, 0);
    ok(status == STATUS_INVALID_HANDLE, "got 0x%x\n", status);

    status = pBCryptDeriveKey(key, L"HASH", NULL, NULL, 0, &size, 0);
    ok(status == STATUS_INVALID_HANDLE, "got 0x%x\n", status);

    status = pBCryptDeriveKey(secret, NULL, NULL, NULL, 0, &size, 0);
    ok(status == STATUS_INVALID_PARAMETER, "got 0x%x\n", status);

    status = pBCryptDeriveKey(secret, L"HASH", NULL, NULL, 0, &size, 0);
    todo_wine
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    status = pBCryptDestroyHash(secret);
    ok(status == STATUS_INVALID_PARAMETER, "got 0x%x\n", status);

    status = pBCryptDestroyKey(secret);
    ok(status == STATUS_INVALID_HANDLE, "got 0x%x\n", status);

    status = pBCryptDestroySecret(NULL);
    ok(status == STATUS_INVALID_HANDLE, "got 0x%x\n", status);

    status = pBCryptDestroySecret(alg);
    ok(status == STATUS_INVALID_HANDLE, "got 0x%x\n", status);

    status = pBCryptDestroySecret(secret);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    status = pBCryptDestroyKey(key);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    status = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    /* DH */
    status = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_DH_ALGORITHM, NULL, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    key = NULL;
    status = pBCryptGenerateKeyPair(alg, &key, 1024, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(key != NULL, "key not set\n");

    status = pBCryptFinalizeKeyPair(key, 0);
    if (status == STATUS_INVALID_PARAMETER)
    {
        win_skip("broken DH detected\n");
        pBCryptCloseAlgorithmProvider(alg, 0);
        return;
    }
    todo_wine ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    status = pBCryptSecretAgreement(key, key, &secret, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    status = pBCryptDestroyKey(key);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    status = pBCryptDeriveKey(secret, L"HASH", NULL, NULL, 0, &size, 0);
    todo_wine ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    status = pBCryptDestroySecret(secret);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    key = NULL;
    status = pBCryptGenerateKeyPair(alg, &key, 256, 0);
    ok(status == STATUS_INVALID_PARAMETER, "got %08x\n", status);

    status = pBCryptGenerateKeyPair(alg, &key, length, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    ok(key != NULL, "key not set\n");

    memset(buffer, 0xcc, sizeof(buffer));
    status = pBCryptGetProperty(key, BCRYPT_DH_PARAMETERS, buffer, sizeof(buffer), &size, 0);
    ok(status == STATUS_INVALID_HANDLE, "got %08x\n", status);

    status = pBCryptExportKey(key, NULL, BCRYPT_DH_PUBLIC_BLOB, buffer, sizeof(buffer), &size, 0);
    ok(status == STATUS_INVALID_HANDLE, "got %08x\n", status);

    status = pBCryptFinalizeKeyPair(key, 0);
    if (status != STATUS_SUCCESS)
    {
        pBCryptDestroyKey(key);
        pBCryptCloseAlgorithmProvider(alg, 0);
        return;
    }
    ok(status == STATUS_SUCCESS, "got %08x\n", status);

    status = pBCryptFinalizeKeyPair(key, 0);
    ok(status == STATUS_INVALID_HANDLE, "got %08x\n", status);

    size = 0xdeadbeef;
    status = pBCryptGetProperty(key, BCRYPT_DH_PARAMETERS, NULL, sizeof(buffer), &size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    ok(size == sizeof(BCRYPT_DH_PARAMETER_HEADER) + length / 8 * 2, "Got unexpected size %u.\n", size);

    size = 0xdeadbeef;
    status = pBCryptGetProperty(key, BCRYPT_DH_PARAMETERS, buffer, 28, &size, 0);
    ok(status == STATUS_BUFFER_TOO_SMALL, "got %08x\n", status);
    ok(size == sizeof(BCRYPT_DH_PARAMETER_HEADER) + length / 8 * 2, "Got unexpected size %u.\n", size);

    size = 0xdeadbeef;
    status = pBCryptGetProperty(key, BCRYPT_DH_PARAMETERS, buffer, sizeof(buffer), &size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    ok(size == sizeof(BCRYPT_DH_PARAMETER_HEADER) + length / 8 * 2, "Got unexpected size %u.\n", size);

    dh_header = (BCRYPT_DH_PARAMETER_HEADER *)buffer;
    ok(dh_header->cbLength == sizeof(*dh_header) + length / 8 * 2, "Got unexpected length %u.\n", dh_header->cbLength);
    ok(dh_header->cbKeyLength == length / 8, "Got unexpected length %u.\n", dh_header->cbKeyLength);
    ok(dh_header->dwMagic == BCRYPT_DH_PARAMETERS_MAGIC, "Got unexpected magic 0x%x.\n", dh_header->dwMagic);

    status = pBCryptDestroyKey(key);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);

    dh_key_blob = (BCRYPT_DH_KEY_BLOB *)buffer;
    dh_key_blob->dwMagic = BCRYPT_DH_PRIVATE_MAGIC;
    dh_key_blob->cbKey = length / 8;
    memcpy(dh_key_blob + 1, dh_private_key, sizeof(dh_private_key));
    size = sizeof(buffer);
    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_DH_PRIVATE_BLOB, &key, buffer, size, 0);
    ok(status == STATUS_INVALID_PARAMETER, "got %08x\n", status);
    size = sizeof(*dh_key_blob) + length / 8 * 4;
    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_DH_PRIVATE_BLOB, &key, buffer, size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);

    memset(buffer, 0xcc, sizeof(buffer));
    size = 0xdeadbeef;
    status = pBCryptExportKey(key, NULL, BCRYPT_DH_PUBLIC_BLOB, NULL, 0, &size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    ok(size == sizeof(BCRYPT_DH_KEY_BLOB) + length / 8 * 3, "Got unexpected size %u.\n", size);

    size = 0xdeadbeef;
    status = pBCryptExportKey(key, NULL, BCRYPT_DH_PUBLIC_BLOB, buffer, sizeof(buffer), &size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    ok(size == sizeof(BCRYPT_DH_KEY_BLOB) + length / 8 * 3, "Got unexpected size %u.\n", size);
    dh_key_blob = (BCRYPT_DH_KEY_BLOB *)buffer;
    ok(dh_key_blob->dwMagic == BCRYPT_DH_PUBLIC_MAGIC, "Got unexpected magic 0x%x.\n", dh_key_blob->dwMagic);
    ok(dh_key_blob->cbKey == length / 8, "Got unexpected length %u.\n", dh_key_blob->cbKey);
    ok(!memcmp(dh_key_blob + 1, dh_private_key, length / 8 * 3), "Key data does not match.\n");

    status = pBCryptGenerateKeyPair(alg, &key2, length, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    dh_header = (BCRYPT_DH_PARAMETER_HEADER *)buffer;
    dh_header->dwMagic = BCRYPT_DH_PARAMETERS_MAGIC;
    dh_header->cbLength = sizeof(*dh_header) + length / 8 * 2;
    dh_header->cbKeyLength = length / 8;
    memcpy(dh_header + 1, dh_private_key, length / 8 * 2);
    status = pBCryptSetProperty(key2, BCRYPT_DH_PARAMETERS, buffer, dh_header->cbLength, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    status = pBCryptFinalizeKeyPair(key2, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);

    status = pBCryptExportKey(key2, NULL, BCRYPT_DH_PUBLIC_BLOB, buffer, sizeof(buffer), &size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    ok(size == sizeof(BCRYPT_DH_KEY_BLOB) + length / 8 * 3, "Got unexpected size %u.\n", size);
    ok(dh_key_blob->dwMagic == BCRYPT_DH_PUBLIC_MAGIC, "Got unexpected dwMagic 0x%x.\n", dh_key_blob->dwMagic);
    ok(dh_key_blob->cbKey == length / 8, "Got unexpected length %u.\n", dh_key_blob->cbKey);
    todo_wine ok(!memcmp(dh_key_blob + 1, dh_private_key, length / 8 * 2), "DH parameters do not match.\n");
    ok(memcmp((BYTE *)(dh_key_blob + 1) + length / 8 * 2, (BYTE *)dh_private_key + length / 8 * 2, length / 8),
       "Random public key data matches.\n");

    memset(buffer, 0xcc, sizeof(buffer));
    status = pBCryptExportKey(key, NULL, BCRYPT_DH_PRIVATE_BLOB, buffer, sizeof(buffer), &size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    dh_key_blob = (BCRYPT_DH_KEY_BLOB *)buffer;
    ok(size == sizeof(BCRYPT_DH_KEY_BLOB) + length / 8 * 4, "Got unexpected size %u.\n", size);
    ok(dh_key_blob->dwMagic == BCRYPT_DH_PRIVATE_MAGIC, "Got unexpected dwMagic 0x%x.\n", dh_key_blob->dwMagic);
    ok(dh_key_blob->cbKey == length / 8, "Got unexpected length %u.\n", dh_key_blob->cbKey);
    ok(!memcmp(dh_key_blob + 1, dh_private_key, length / 8 * 4), "Private key data does not match.\n");

    status = pBCryptSecretAgreement(NULL, key, &secret, 0);
    ok(status == STATUS_INVALID_HANDLE, "got %08x\n", status);

    status = pBCryptSecretAgreement(key, NULL, &secret, 0);
    ok(status == STATUS_INVALID_HANDLE, "got %08x\n", status);

    status = pBCryptSecretAgreement(key, key, NULL, 0);
    ok(status == STATUS_INVALID_PARAMETER, "got %08x\n", status);

    status = pBCryptSecretAgreement(key, key, &secret, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);

    status = pBCryptDeriveKey(NULL, L"HASH", NULL, NULL, 0, &size, 0);
    ok(status == STATUS_INVALID_HANDLE, "got %08x\n", status);

    status = pBCryptDeriveKey(key, L"HASH", NULL, NULL, 0, &size, 0);
    ok(status == STATUS_INVALID_HANDLE, "got %08x\n", status);

    status = pBCryptDeriveKey(secret, NULL, NULL, NULL, 0, &size, 0);
    ok(status == STATUS_INVALID_PARAMETER, "got %08x\n", status);

    size = 0xdeadbeef;
    status = pBCryptDeriveKey(secret, L"HASH", NULL, NULL, 0, &size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    ok(size == 20, "Got unexpected size %u.\n", size);

    size = 0xdeadbeef;
    status = pBCryptDeriveKey(secret, BCRYPT_KDF_RAW_SECRET, NULL, NULL, 0, &size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    ok(size == length / 8, "Got unexpected size %u.\n", size);

    status = pBCryptDeriveKey(secret, BCRYPT_KDF_RAW_SECRET, NULL, buffer, 128, &size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    ok(size == length / 8, "Got unexpected size %u.\n", size);
    ok(!memcmp(buffer, dh_shared_secret_raw, size), "Raw shared secret data does not match.\n");

    size = sizeof(buffer);
    memset(buffer, 0xcc, sizeof(buffer));
    status = pBCryptDeriveKey(secret, BCRYPT_KDF_HASH, NULL, buffer, 128, &size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    ok(size == 20, "Got unexpected size %u.\n", size);
    ok(!memcmp(buffer, dh_shared_secret_sha1, sizeof(dh_shared_secret_sha1)),
       "sha1 shared secret data does not match.\n");

    size = sizeof(buffer);
    status = pBCryptDeriveKey(secret, BCRYPT_KDF_HASH, &hash_params, buffer, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    ok(size == 32, "Got unexpected size %u.\n", size);
    ok(!memcmp(buffer, dh_shared_secret_sha256, sizeof(dh_shared_secret_sha256)),
       "sha256 shared secret data does not match.\n");

    for (i = size; i < sizeof(buffer); ++i)
        if (buffer[i] != 0xcc) break;
    ok(i == sizeof(buffer), "Buffer modified at %u, value %#x.\n", i, buffer[i]);

    status = pBCryptDestroySecret(secret);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);

    status = pBCryptSecretAgreement(key, key2, &secret, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    status = pBCryptSecretAgreement(key2, key, &secret2, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);

    status = pBCryptDeriveKey(secret, BCRYPT_KDF_RAW_SECRET, NULL, buffer, 128, &size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    status = pBCryptDeriveKey(secret, BCRYPT_KDF_RAW_SECRET, NULL, buffer + size, 128, &size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    ok(!memcmp(buffer, buffer + size, size), "Shared secrets do not match.\n");

    status = pBCryptDestroySecret(secret);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    status = pBCryptDestroySecret(secret2);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);

    status = pBCryptDestroyKey(key);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    status = pBCryptDestroyKey(key2);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);

    dh_key_blob = (BCRYPT_DH_KEY_BLOB *)buffer;
    dh_key_blob->dwMagic = BCRYPT_DH_PRIVATE_MAGIC;
    dh_key_blob->cbKey = length / 8;
    memcpy(dh_key_blob + 1, dh_private_key2, sizeof(dh_private_key2));

    size = sizeof(*dh_key_blob) + length / 8 * 4;
    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_DH_PRIVATE_BLOB, &key, buffer, size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);

    dh_key_blob = (BCRYPT_DH_KEY_BLOB *)buffer;
    dh_key_blob->dwMagic = BCRYPT_DH_PUBLIC_MAGIC;
    dh_key_blob->cbKey = length / 8;
    memcpy(dh_key_blob + 1, dh_peer_key, sizeof(dh_peer_key));

    size = sizeof(*dh_key_blob) + length / 8 * 3;
    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_DH_PUBLIC_BLOB, &key2, buffer, size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);

    status = pBCryptSecretAgreement(key, key2, &secret, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);

    status = pBCryptDeriveKey(secret, BCRYPT_KDF_RAW_SECRET, NULL, buffer, 128, &size, 0);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    ok(size == length / 8, "Got unexpected size %u.\n", size);
    ok(!memcmp(buffer, dh_shared_secret_raw2, size), "Raw shared secret data does not match.\n");

    status = pBCryptDestroySecret(secret);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    status = pBCryptDestroyKey(key);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);
    status = pBCryptDestroyKey(key2);
    ok(status == STATUS_SUCCESS, "got %08x\n", status);

    status = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
}

static void test_RC4(void)
{
    BCRYPT_ALG_HANDLE alg;
    NTSTATUS status;
    ULONG len, size;

    status = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_RC4_ALGORITHM, NULL, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);

    len = size = 0;
    status = pBCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(len, "expected non-zero len\n");
    ok(size == sizeof(len), "got %u\n", size);

    len = size = 0;
    status = pBCryptGetProperty(alg, BCRYPT_BLOCK_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
    ok(len == 1, "got %u\n", len);
    ok(size == sizeof(len), "got %u\n", size);

    size = sizeof(BCRYPT_CHAIN_MODE_NA);
    status = pBCryptSetProperty(alg, BCRYPT_CHAINING_MODE, (UCHAR *)BCRYPT_CHAIN_MODE_NA, size, 0);
    ok(!status, "got 0x%x\n", status);

    status = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
}

static void test_PBKDF2(void)
{
    static char salt[] = "cCxuHMEHLibcglJOG88dIw==";
    static ULONGLONG iter_count = 25;
    static BCryptBuffer pbkdf2_param_buffers[] =
    {
        {
            sizeof(BCRYPT_SHA1_ALGORITHM),
            KDF_HASH_ALGORITHM,
            (void *)BCRYPT_SHA1_ALGORITHM,
        },
        {
            sizeof(salt) - 1,
            KDF_SALT,
            salt,
        },
        {
            sizeof(iter_count),
            KDF_ITERATION_COUNT,
            (void *)&iter_count,
        }
    };
    static BCryptBufferDesc pbkdf2_params =
    {
        BCRYPTBUFFER_VERSION,
        ARRAY_SIZE(pbkdf2_param_buffers),
        pbkdf2_param_buffers,
    };
    static UCHAR pbkdf2_hash[] =
    {
        0x18, 0xf2, 0x58, 0x0b, 0x92, 0x6e, 0x6d, 0xfe,
        0x55, 0xbb, 0x62, 0x87, 0x5b, 0x1f, 0x61, 0x83,
        0x4d, 0x16, 0xe4, 0x04, 0xcd, 0xab, 0x43, 0x77,
        0x25, 0x3e, 0x1d, 0xb4, 0x95, 0x5f, 0xf7, 0x01,
    };

    BCRYPT_KEY_LENGTHS_STRUCT key_lengths;
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key;
    NTSTATUS status;
    ULONG val, size;
    static BYTE buf[32];

    status = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_PBKDF2_ALGORITHM, NULL, 0);
    if (status == STATUS_NOT_FOUND)
    {
        win_skip("PBKDF2 not available\n");
        return;
    }
    ok(!status, "got 0x%x\n", status);
    ok(pBCryptKeyDerivation != NULL, "BCryptKeyDerivation not available\n");

    val = size = 0;
    status = pBCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&val, sizeof(val), &size, 0);
    ok(!status, "got 0x%x\n", status);
    ok(val, "got %u\n", val);
    ok(size == sizeof(val), "got %u\n", size);

    val = size = 0;
    status = pBCryptGetProperty(alg, BCRYPT_BLOCK_LENGTH, (UCHAR *)&val, sizeof(val), &size, 0);
    ok(status == STATUS_NOT_SUPPORTED, "got 0x%x\n", status);

    memset(&key_lengths, 0xfe, sizeof(key_lengths));
    size = 0;
    status = pBCryptGetProperty(alg, BCRYPT_KEY_LENGTHS, (UCHAR *)&key_lengths, sizeof(key_lengths), &size, 0);
    ok(!status, "got 0x%x\n", status);
    ok(size == sizeof(key_lengths), "got %u\n", size);
    ok(key_lengths.dwMinLength == 0, "got %u\n", key_lengths.dwMinLength);
    ok(key_lengths.dwMaxLength == 16384, "got %u\n", key_lengths.dwMaxLength);
    ok(key_lengths.dwIncrement == 8, "got %u\n", key_lengths.dwIncrement);

    key = 0;
    status = pBCryptGenerateSymmetricKey(alg, &key, NULL, 0, (UCHAR *)"test", 4, 0);
    ok(!status, "got 0x%x\n", status);
    val = size = 0;
    status = pBCryptGetProperty(key, BCRYPT_KEY_STRENGTH, (UCHAR *)&val, sizeof(val), &size, 0);
    ok(!status, "got 0x%x\n", status);
    ok(val == strlen("test") * 8, "got %u\n", val);

    status = pBCryptKeyDerivation(key, &pbkdf2_params, NULL, 0, &size, 0);
    ok(status == STATUS_INVALID_PARAMETER, "got 0x%x\n", status);

    buf[0] = buf[1] = 'x';
    status = pBCryptKeyDerivation(key, &pbkdf2_params, buf, 1, &size, 0);
    ok(!status, "got 0x%x\n", status);
    ok(size == 1, "size = %u\n", size);
    ok(buf[0] == pbkdf2_hash[0], "buf[0] = %x\n", buf[0]);
    ok(buf[1] == 'x', "buf[1] = %x\n", buf[1]);

    memset(buf, 'x', sizeof(buf));
    status = pBCryptKeyDerivation(key, &pbkdf2_params, buf, sizeof(buf), &size, 0);
    ok(!status, "got 0x%x\n", status);
    ok(size == sizeof(buf), "size = %u\n", size);
    ok(!memcmp(buf, pbkdf2_hash, sizeof(pbkdf2_hash)),
            "wrong data (%s)\n", wine_dbgstr_an((char *)buf, size));

    status = pBCryptDestroyKey(key);
    ok(!status, "got 0x%x\n", status);
    status = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(status == STATUS_SUCCESS, "got 0x%x\n", status);
}

START_TEST(bcrypt)
{
    HMODULE module;

    module = LoadLibraryA("bcrypt.dll");
    if (!module)
    {
        win_skip("bcrypt.dll not found\n");
        return;
    }

    pBCryptOpenAlgorithmProvider = (void *)GetProcAddress(module, "BCryptOpenAlgorithmProvider");
    pBCryptCloseAlgorithmProvider = (void *)GetProcAddress(module, "BCryptCloseAlgorithmProvider");
    pBCryptGetFipsAlgorithmMode = (void *)GetProcAddress(module, "BCryptGetFipsAlgorithmMode");
    pBCryptCreateHash = (void *)GetProcAddress(module, "BCryptCreateHash");
    pBCryptHash = (void *)GetProcAddress(module, "BCryptHash");
    pBCryptHashData = (void *)GetProcAddress(module, "BCryptHashData");
    pBCryptFinishHash = (void *)GetProcAddress(module, "BCryptFinishHash");
    pBCryptDestroyHash = (void *)GetProcAddress(module, "BCryptDestroyHash");
    pBCryptGenRandom = (void *)GetProcAddress(module, "BCryptGenRandom");
    pBCryptGetProperty = (void *)GetProcAddress(module, "BCryptGetProperty");
    pBCryptKeyDerivation = (void *)GetProcAddress(module, "BCryptKeyDerivation");
    pBCryptDeriveKeyPBKDF2 = (void *)GetProcAddress(module, "BCryptDeriveKeyPBKDF2");
    pBCryptSetProperty = (void *)GetProcAddress(module, "BCryptSetProperty");
    pBCryptGenerateSymmetricKey = (void *)GetProcAddress(module, "BCryptGenerateSymmetricKey");
    pBCryptEncrypt = (void *)GetProcAddress(module, "BCryptEncrypt");
    pBCryptDuplicateKey = (void *)GetProcAddress(module, "BCryptDuplicateKey");
    pBCryptDestroyKey = (void *)GetProcAddress(module, "BCryptDestroyKey");
    pBCryptDecrypt = (void *)GetProcAddress(module, "BCryptDecrypt");
    pBCryptImportKey = (void *)GetProcAddress(module, "BCryptImportKey");
    pBCryptExportKey = (void *)GetProcAddress(module, "BCryptExportKey");
    pBCryptImportKeyPair = (void *)GetProcAddress(module, "BCryptImportKeyPair");
    pBCryptVerifySignature = (void *)GetProcAddress(module, "BCryptVerifySignature");
    pBCryptGenerateKeyPair = (void *)GetProcAddress(module, "BCryptGenerateKeyPair");
    pBCryptFinalizeKeyPair = (void *)GetProcAddress(module, "BCryptFinalizeKeyPair");
    pBCryptSignHash = (void *)GetProcAddress(module, "BCryptSignHash");
    pBCryptSecretAgreement = (void *)GetProcAddress(module, "BCryptSecretAgreement");
    pBCryptDeriveKey = (void *)GetProcAddress(module, "BCryptDeriveKey");
    pBCryptDestroySecret = (void *)GetProcAddress(module, "BCryptDestroySecret");
    pBCryptEnumAlgorithms = (void *)GetProcAddress(module, "BCryptEnumAlgorithms");
    pBCryptFreeBuffer = (void *)GetProcAddress(module, "BCryptFreeBuffer");
    pBCryptDeriveKeyCapi = (void *)GetProcAddress(module, "BCryptDeriveKeyCapi");

    test_BCryptGenRandom();
    test_BCryptGetFipsAlgorithmMode();
    test_hashes();
    test_BcryptHash();
    test_BcryptDeriveKeyPBKDF2();
    test_rng();
    test_3des();
    test_aes();
    test_BCryptGenerateSymmetricKey();
    test_BCryptEncrypt();
    test_BCryptDecrypt();
    test_key_import_export();
    test_ECDSA();
    test_RSA();
    test_RSA_SIGN();
    test_ECDH();
    test_DH();
#ifndef __REACTOS__
    test_BCryptEnumContextFunctions();
#endif
    test_BCryptSignHash();
    test_BCryptEnumAlgorithms();
    test_aes_vector();
    test_BcryptDeriveKeyCapi();
    test_DSA();
    test_SecretAgreement();
    test_rsa_encrypt();
    test_RC4();
    test_PBKDF2();

    FreeLibrary(module);
}
