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

#include "wine/test.h"

static NTSTATUS (WINAPI *pBCryptOpenAlgorithmProvider)(BCRYPT_ALG_HANDLE *, LPCWSTR, LPCWSTR, ULONG);
static NTSTATUS (WINAPI *pBCryptCloseAlgorithmProvider)(BCRYPT_ALG_HANDLE, ULONG);
static NTSTATUS (WINAPI *pBCryptGetFipsAlgorithmMode)(BOOLEAN *);
static NTSTATUS (WINAPI *pBCryptCreateHash)(BCRYPT_ALG_HANDLE, BCRYPT_HASH_HANDLE *, PUCHAR, ULONG, PUCHAR,
                                            ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptHash)(BCRYPT_ALG_HANDLE, UCHAR *, ULONG, UCHAR *, ULONG, UCHAR *, ULONG);
static NTSTATUS (WINAPI *pBCryptHashData)(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptDuplicateHash)(BCRYPT_HASH_HANDLE, BCRYPT_HASH_HANDLE *, UCHAR *, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptFinishHash)(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptDestroyHash)(BCRYPT_HASH_HANDLE);
static NTSTATUS (WINAPI *pBCryptGenRandom)(BCRYPT_ALG_HANDLE, PUCHAR, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptGetProperty)(BCRYPT_HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG *, ULONG);
static NTSTATUS (WINAPI *pBCryptSetProperty)(BCRYPT_HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptGenerateSymmetricKey)(BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE *, PUCHAR, ULONG,
                                                      PUCHAR, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptEncrypt)(BCRYPT_KEY_HANDLE, PUCHAR, ULONG, VOID *, PUCHAR, ULONG, PUCHAR, ULONG,
                                      ULONG *, ULONG);
static NTSTATUS (WINAPI *pBCryptDecrypt)(BCRYPT_KEY_HANDLE, PUCHAR, ULONG, VOID *, PUCHAR, ULONG, PUCHAR, ULONG,
                                      ULONG *, ULONG);
static NTSTATUS (WINAPI *pBCryptDuplicateKey)(BCRYPT_KEY_HANDLE, BCRYPT_KEY_HANDLE *, UCHAR *, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptDestroyKey)(BCRYPT_KEY_HANDLE);
static NTSTATUS (WINAPI *pBCryptImportKey)(BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE, LPCWSTR, BCRYPT_KEY_HANDLE *,
                                           PUCHAR, ULONG, PUCHAR, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptExportKey)(BCRYPT_KEY_HANDLE, BCRYPT_KEY_HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG *, ULONG);
static NTSTATUS (WINAPI *pBCryptImportKeyPair)(BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE, LPCWSTR, BCRYPT_KEY_HANDLE *, UCHAR *, ULONG, ULONG);
static NTSTATUS (WINAPI *pBCryptVerifySignature)(BCRYPT_KEY_HANDLE, VOID *, UCHAR *, ULONG, UCHAR *, ULONG, ULONG);

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

static int strcmp_wa(const WCHAR *strw, const char *stra)
{
    WCHAR buf[512];
    MultiByteToWideChar(CP_ACP, 0, stra, -1, buf, sizeof(buf)/sizeof(buf[0]));
    return lstrcmpW(strw, buf);
}

#define test_object_length(a) _test_object_length(__LINE__,a)
static void _test_object_length(unsigned line, void *handle)
{
    NTSTATUS status;
    ULONG len, size;

    len = size = 0xdeadbeef;
    status = pBCryptGetProperty(NULL, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok_(__FILE__,line)(status == STATUS_INVALID_HANDLE, "BCryptGetProperty failed: %08x\n", status);

    len = size = 0xdeadbeef;
    status = pBCryptGetProperty(handle, NULL, (UCHAR *)&len, sizeof(len), &size, 0);
    ok_(__FILE__,line)(status == STATUS_INVALID_PARAMETER, "BCryptGetProperty failed: %08x\n", status);

    len = size = 0xdeadbeef;
    status = pBCryptGetProperty(handle, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), NULL, 0);
    ok_(__FILE__,line)(status == STATUS_INVALID_PARAMETER, "BCryptGetProperty failed: %08x\n", status);

    len = size = 0xdeadbeef;
    status = pBCryptGetProperty(handle, BCRYPT_OBJECT_LENGTH, NULL, sizeof(len), &size, 0);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "BCryptGetProperty failed: %08x\n", status);
    ok_(__FILE__,line)(size == sizeof(len), "got %u\n", size);

    len = size = 0xdeadbeef;
    status = pBCryptGetProperty(handle, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, 0, &size, 0);
    ok_(__FILE__,line)(status == STATUS_BUFFER_TOO_SMALL, "BCryptGetProperty failed: %08x\n", status);
    ok_(__FILE__,line)(len == 0xdeadbeef, "got %u\n", len);
    ok_(__FILE__,line)(size == sizeof(len), "got %u\n", size);

    len = size = 0xdeadbeef;
    status = pBCryptGetProperty(handle, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "BCryptGetProperty failed: %08x\n", status);
    ok_(__FILE__,line)(len != 0xdeadbeef, "len not set\n");
    ok_(__FILE__,line)(size == sizeof(len), "got %u\n", size);
}

#define test_hash_length(a,b) _test_hash_length(__LINE__,a,b)
static void _test_hash_length(unsigned line, void *handle, ULONG exlen)
{
    ULONG len = 0xdeadbeef, size = 0xdeadbeef;
    NTSTATUS status;

    status = pBCryptGetProperty(handle, BCRYPT_HASH_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "BCryptGetProperty failed: %08x\n", status);
    ok_(__FILE__,line)(size == sizeof(len), "got %u\n", size);
    ok_(__FILE__,line)(len == exlen, "len = %u, expected %u\n", len, exlen);
}

#define test_alg_name(a,b) _test_alg_name(__LINE__,a,b)
static void _test_alg_name(unsigned line, void *handle, const char *exname)
{
    ULONG size = 0xdeadbeef;
    UCHAR buf[256];
    const WCHAR *name = (const WCHAR*)buf;
    NTSTATUS status;

    status = pBCryptGetProperty(handle, BCRYPT_ALGORITHM_NAME, buf, sizeof(buf), &size, 0);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "BCryptGetProperty failed: %08x\n", status);
    ok_(__FILE__,line)(size == (strlen(exname)+1)*sizeof(WCHAR), "got %u\n", size);
    ok_(__FILE__,line)(!strcmp_wa(name, exname), "alg name = %s, expected %s\n", wine_dbgstr_w(name), exname);
}

struct hash_test
{
    const char *alg;
    unsigned hash_size;
    const char *hash;
    const char *hmac_hash;
};

static void test_hash(const struct hash_test *test)
{
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_HASH_HANDLE hash;
    UCHAR buf[512], buf_hmac[1024], hash_buf[128], hmac_hash[128];
    WCHAR alg_name[64];
    char str[512];
    NTSTATUS ret;
    ULONG len;

    MultiByteToWideChar(CP_ACP, 0, test->alg, -1, alg_name, sizeof(alg_name)/sizeof(WCHAR));

    alg = NULL;
    ret = pBCryptOpenAlgorithmProvider(&alg, alg_name, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    test_object_length(alg);
    test_hash_length(alg, test->hash_size);
    test_alg_name(alg, test->alg);

    hash = NULL;
    len = sizeof(buf);
    ret = pBCryptCreateHash(alg, &hash, buf, len, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(hash != NULL, "hash not set\n");

    ret = pBCryptHashData(hash, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = pBCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    test_hash_length(hash, test->hash_size);
    test_alg_name(hash, test->alg);

    memset(hash_buf, 0, sizeof(hash_buf));
    ret = pBCryptFinishHash(hash, hash_buf, test->hash_size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    format_hash( hash_buf, test->hash_size, str );
    ok(!strcmp(str, test->hash), "got %s\n", str);

    ret = pBCryptDestroyHash(hash);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    alg = NULL;
    ret = pBCryptOpenAlgorithmProvider(&alg, alg_name, MS_PRIMITIVE_PROVIDER, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    hash = NULL;
    len = sizeof(buf_hmac);
    ret = pBCryptCreateHash(alg, &hash, buf_hmac, len, (UCHAR *)"key", sizeof("key"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(hash != NULL, "hash not set\n");

    ret = pBCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    test_hash_length(hash, test->hash_size);
    test_alg_name(hash, test->alg);

    memset(hmac_hash, 0, sizeof(hmac_hash));
    ret = pBCryptFinishHash(hash, hmac_hash, test->hash_size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    format_hash( hmac_hash, test->hash_size, str );
    ok(!strcmp(str, test->hmac_hash), "got %s\n", str);

    ret = pBCryptDestroyHash(hash);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
}

static void test_hashes(void)
{
    static const struct hash_test tests[] = {
        {
            "SHA1",
            20,
            "961fa64958818f767707072755d7018dcd278e94",
            "2472cf65d0e090618d769d3e46f0d9446cf212da"
        },
        {
            "SHA256",
            32,
            "ceb73749c899693706ede1e30c9929b3fd5dd926163831c2fb8bd41e6efb1126",
            "34c1aa473a4468a91d06e7cdbc75bc4f93b830ccfc2a47ffd74e8e6ed29e4c72"
        },
        {
            "SHA384",
            48,
            "62b21e90c9022b101671ba1f808f8631a8149f0f12904055839a35c1ca78ae53"
            "63eed1e743a692d70e0504b0cfd12ef9",
            "4b3e6d6ff2da121790ab7e7b9247583e3a7eed2db5bd4dabc680303b1608f37d"
            "fdc836d96a704c03283bc05b4f6c5eb8"
        },
        {
            "SHA512",
            64,
            "d55ced17163bf5386f2cd9ff21d6fd7fe576a915065c24744d09cfae4ec84ee1"
            "ef6ef11bfbc5acce3639bab725b50a1fe2c204f8c820d6d7db0df0ecbc49c5ca",
            "415fb6b10018ca03b38a1b1399c42ac0be5e8aceddb9a73103f5e543bf2d888f"
            "2eecf91373941f9315dd730a77937fa92444450fbece86f409d9cb5ec48c6513"
        },
        {
            "MD2",
            16,
            "1bb33606ba908912a84221109d29cd7e",
            "7f05b0638d77f4a27f3a9c4d353cd648"
        },
        {
            "MD4",
            16,
            "74b5db93c0b41e36ca7074338fc0b637",
            "bc2e8ac4d8248ed21b8d26227a30ea3a"
        },
        {
            "MD5",
            16,
            "e2a3e68d23ce348b8f68b3079de3d4c9",
            "7bda029b93fa8d817fcc9e13d6bdf092"
        }
    };
    unsigned i;

    for(i = 0; i < sizeof(tests)/sizeof(*tests); i++)
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

    alg = NULL;
    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_MD5_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    test_hash_length(alg, 16);
    test_alg_name(alg, "MD5");

    memset(md5, 0, sizeof(md5));
    ret = pBCryptHash(alg, NULL, 0, (UCHAR *)"test", sizeof("test"), md5, sizeof(md5));
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    format_hash( md5, sizeof(md5), str );
    ok(!strcmp(str, expected), "got %s\n", str);

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    alg = NULL;
    memset(md5_hmac, 0, sizeof(md5_hmac));
    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_MD5_ALGORITHM, MS_PRIMITIVE_PROVIDER, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    ret = pBCryptHash(alg, (UCHAR *)"key", sizeof("key"), (UCHAR *)"test", sizeof("test"), md5_hmac, sizeof(md5_hmac));
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    format_hash( md5_hmac, sizeof(md5_hmac), str );
    ok(!strcmp(str, expected_hmac), "got %s\n", str);

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
}

static void test_rng(void)
{
    BCRYPT_ALG_HANDLE alg;
    ULONG size, len;
    UCHAR buf[16];
    NTSTATUS ret;

    alg = NULL;
    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_RNG_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    len = size = 0xdeadbeef;
    ret = pBCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = pBCryptGetProperty(alg, BCRYPT_HASH_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got %08x\n", ret);

    test_alg_name(alg, "RNG");

    memset(buf, 0, 16);
    ret = pBCryptGenRandom(alg, buf, 8, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(memcmp(buf, buf + 8, 8), "got zeroes\n");

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
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
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    len = size = 0;
    ret = pBCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(len, "expected non-zero len\n");
    ok(size == sizeof(len), "got %u\n", size);

    len = size = 0;
    ret = pBCryptGetProperty(alg, BCRYPT_BLOCK_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(len == 16, "got %u\n", len);
    ok(size == sizeof(len), "got %u\n", size);

    size = 0;
    ret = pBCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, 0, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(size == 64, "got %u\n", size);

    size = 0;
    ret = pBCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, sizeof(mode) - 1, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(size == 64, "got %u\n", size);

    size = 0;
    memset(mode, 0, sizeof(mode));
    ret = pBCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, sizeof(mode), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(!lstrcmpW((const WCHAR *)mode, BCRYPT_CHAIN_MODE_CBC), "got %s\n", mode);
    ok(size == 64, "got %u\n", size);

    size = 0;
    memset(&key_lengths, 0, sizeof(key_lengths));
    ret = BCryptGetProperty(alg, BCRYPT_KEY_LENGTHS, (UCHAR*)&key_lengths, sizeof(key_lengths), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == sizeof(key_lengths), "got %u\n", size);
    ok(key_lengths.dwMinLength == 128, "Expected 128, got %d\n", key_lengths.dwMinLength);
    ok(key_lengths.dwMaxLength == 256, "Expected 256, got %d\n", key_lengths.dwMaxLength);
    ok(key_lengths.dwIncrement == 64, "Expected 64, got %d\n", key_lengths.dwIncrement);

    memcpy(mode, BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM));
    ret = pBCryptSetProperty(alg, BCRYPT_CHAINING_MODE, mode, sizeof(mode), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    size = 0;
    memset(mode, 0, sizeof(mode));
    ret = pBCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, sizeof(mode), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(!lstrcmpW((const WCHAR *)mode, BCRYPT_CHAIN_MODE_GCM), "got %s\n", mode);
    ok(size == 64, "got %u\n", size);

    test_alg_name(alg, "AES");

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
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
    BCRYPT_ALG_HANDLE aes;
    BCRYPT_KEY_HANDLE key, key2;
    UCHAR *buf, ciphertext[16], plaintext[16], ivbuf[16];
    ULONG size, len, i;
    NTSTATUS ret;

    ret = pBCryptOpenAlgorithmProvider(&aes, BCRYPT_AES_ALGORITHM, NULL, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = pBCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    key = NULL;
    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(key != NULL, "key not set\n");

    ret = pBCryptSetProperty(aes, BCRYPT_CHAINING_MODE, (UCHAR *)BCRYPT_CHAIN_MODE_CBC,
                            sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = pBCryptSetProperty(key, BCRYPT_CHAINING_MODE, (UCHAR *)BCRYPT_CHAIN_MODE_CBC,
                             sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    ok(ret == STATUS_SUCCESS || broken(ret == STATUS_NOT_SUPPORTED) /* < Win 8 */, "got %08x\n", ret);

    size = 0xdeadbeef;
    ret = pBCryptEncrypt(key, NULL, 0, NULL, NULL, 0, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(!size, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data, 16, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 16, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(ciphertext, expected, sizeof(expected)), "wrong data\n");
    for (i = 0; i < 16; i++)
        ok(ciphertext[i] == expected[i], "%u: %02x != %02x\n", i, ciphertext[i], expected[i]);

    ret = pBCryptDuplicateKey(NULL, &key2, NULL, 0, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got %08x\n", ret);

    if (0) /* crashes on some Windows versions */
    {
        ret = pBCryptDuplicateKey(key, NULL, NULL, 0, 0);
        ok(ret == STATUS_INVALID_PARAMETER, "got %08x\n", ret);
    }

    key2 = (void *)0xdeadbeef;
    ret = pBCryptDuplicateKey(key, &key2, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS || broken(ret == STATUS_INVALID_PARAMETER), "got %08x\n", ret);

    if (ret == STATUS_SUCCESS)
    {
        size = 0;
        memcpy(ivbuf, iv, sizeof(iv));
        memset(ciphertext, 0, sizeof(ciphertext));
        ret = pBCryptEncrypt(key2, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
        ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
        ok(size == 16, "got %u\n", size);
        ok(!memcmp(ciphertext, expected, sizeof(expected)), "wrong data\n");
        for (i = 0; i < 16; i++)
            ok(ciphertext[i] == expected[i], "%u: %02x != %02x\n", i, ciphertext[i], expected[i]);

        ret = pBCryptDestroyKey(key2);
        ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    }

    size = 0xdeadbeef;
    ret = pBCryptDecrypt(key, NULL, 0, NULL, NULL, 0, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(!size, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext, 16, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 16, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext, 16, NULL, ivbuf, 16, plaintext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(plaintext, data, sizeof(data)), "wrong data\n");

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    ret = pBCryptCloseAlgorithmProvider(aes, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
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
        {0x66,0xb8,0xbd,0xe5,0x90,0x6c,0xec,0xdf,0xfa,0x8a,0xb2,0xfd,0x92,0x84,0xeb,0xf0,
         0x95,0xc4,0xdf,0xa7,0x7a,0x62,0xe4,0xab,0xd4,0x0e,0x94,0x4e,0xd7,0x6e,0xa1,0x47,
         0x29,0x4b,0x37,0xfe,0x28,0x6d,0x5f,0x69,0x46,0x30,0x73,0xc0,0xaa,0x42,0xe4,0x46};
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
    BCRYPT_ALG_HANDLE aes;
    BCRYPT_KEY_HANDLE key;
    ULONG size, len, i;
    NTSTATUS ret;

    ret = pBCryptOpenAlgorithmProvider(&aes, BCRYPT_AES_ALGORITHM, NULL, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    /******************
     * AES - CBC mode *
     ******************/

    len = 0xdeadbeef;
    size = sizeof(len);
    ret = pBCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    /* input size is a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data, 16, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 16, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(ciphertext, expected, sizeof(expected)), "wrong data\n");
    for (i = 0; i < 16; i++)
        ok(ciphertext[i] == expected[i], "%u: %02x != %02x\n", i, ciphertext[i], expected[i]);

    /* input size is not a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data, 17, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got %08x\n", ret);
    ok(size == 17, "got %u\n", size);

    /* input size is not a multiple of block size, block padding set */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data, 17, NULL, ivbuf, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 17, NULL, ivbuf, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ciphertext, expected2, sizeof(expected2)), "wrong data\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected2[i], "%u: %02x != %02x\n", i, ciphertext[i], expected2[i]);

    /* input size is a multiple of block size, block padding set */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 48, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, ciphertext, 48, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 48, "got %u\n", size);
    ok(!memcmp(ciphertext, expected3, sizeof(expected3)), "wrong data\n");
    for (i = 0; i < 48; i++)
        ok(ciphertext[i] == expected3[i], "%u: %02x != %02x\n", i, ciphertext[i], expected3[i]);

    /* output size too small */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 17, NULL, ivbuf, 16, ciphertext, 31, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(size == 48, "got %u\n", size);

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    /* 256 bit key */
    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret256, sizeof(secret256), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 48, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, ciphertext, 48, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 48, "got %u\n", size);
    ok(!memcmp(ciphertext, expected8, sizeof(expected8)), "wrong data\n");
    for (i = 0; i < 48; i++)
        ok(ciphertext[i] == expected8[i], "%u: %02x != %02x\n", i, ciphertext[i], expected8[i]);

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    /******************
     * AES - GCM mode *
     ******************/

    size = 0;
    ret = BCryptGetProperty(aes, BCRYPT_AUTH_TAG_LENGTH, NULL, 0, &size, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got %08x\n", ret);

    ret = BCryptSetProperty(aes, BCRYPT_CHAINING_MODE, (UCHAR*)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    size = 0;
    ret = BCryptGetProperty(aes, BCRYPT_AUTH_TAG_LENGTH, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == sizeof(tag_length), "got %u\n", size);

    size = 0;
    memset(&tag_length, 0, sizeof(tag_length));
    ret = BCryptGetProperty(aes, BCRYPT_AUTH_TAG_LENGTH, (UCHAR*)&tag_length, sizeof(tag_length), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == sizeof(tag_length), "got %u\n", size);
    ok(tag_length.dwMinLength == 12, "Expected 12, got %d\n", tag_length.dwMinLength);
    ok(tag_length.dwMaxLength == 16, "Expected 16, got %d\n", tag_length.dwMaxLength);
    ok(tag_length.dwIncrement == 1, "Expected 1, got %d\n", tag_length.dwIncrement);

    len = 0xdeadbeef;
    size = sizeof(len);
    ret = pBCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

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
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ciphertext, expected4, sizeof(expected4)), "wrong data\n");
    ok(!memcmp(tag, expected_tag, sizeof(expected_tag)), "wrong tag\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected4[i], "%u: %02x != %02x\n", i, ciphertext[i], expected4[i]);
    for (i = 0; i < 16; i++)
        ok(tag[i] == expected_tag[i], "%u: %02x != %02x\n", i, tag[i], expected_tag[i]);

    /* input size is not multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0xff, sizeof(ciphertext));
    memset(tag, 0xff, sizeof(tag));
    ret = pBCryptEncrypt(key, data2, 24, &auth_info, ivbuf, 16, ciphertext, 24, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 24, "got %u\n", size);
    ok(!memcmp(ciphertext, expected4, 24), "wrong data\n");
    ok(!memcmp(tag, expected_tag2, sizeof(expected_tag2)), "wrong tag\n");
    for (i = 0; i < 24; i++)
        ok(ciphertext[i] == expected4[i], "%u: %02x != %02x\n", i, ciphertext[i], expected4[i]);
    for (i = 0; i < 16; i++)
        ok(tag[i] == expected_tag2[i], "%u: %02x != %02x\n", i, tag[i], expected_tag2[i]);

    /* test with auth data */
    auth_info.pbAuthData = auth_data;
    auth_info.cbAuthData = sizeof(auth_data);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0xff, sizeof(ciphertext));
    memset(tag, 0xff, sizeof(tag));
    ret = pBCryptEncrypt(key, data2, 32, &auth_info, ivbuf, 16, ciphertext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ciphertext, expected4, sizeof(expected4)), "wrong data\n");
    ok(!memcmp(tag, expected_tag3, sizeof(expected_tag3)), "wrong tag\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected4[i], "%u: %02x != %02x\n", i, ciphertext[i], expected4[i]);
    for (i = 0; i < 16; i++)
        ok(tag[i] == expected_tag3[i], "%u: %02x != %02x\n", i, tag[i], expected_tag3[i]);

    memset(tag, 0xff, sizeof(tag));
    ret = pBCryptEncrypt(key, data2, 0, &auth_info, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 0, "got %u\n", size);
    for (i = 0; i < 16; i++)
        ok(tag[i] == 0xff, "%u: %02x != %02x\n", i, tag[i], 0xff);

    memset(tag, 0xff, sizeof(tag));
    ret = pBCryptEncrypt(key, NULL, 0, &auth_info, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 0, "got %u\n", size);
    ok(!memcmp(tag, expected_tag4, sizeof(expected_tag4)), "wrong tag\n");
    for (i = 0; i < 16; i++)
        ok(tag[i] == expected_tag4[i], "%u: %02x != %02x\n", i, tag[i], expected_tag4[i]);

    /* test with padding */
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, &auth_info, ivbuf, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    todo_wine ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);

    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, &auth_info, ivbuf, 16, ciphertext, 48, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_INVALID_PARAMETER, "got %08x\n", ret);

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    /******************
     * AES - ECB mode *
     ******************/

    ret = BCryptSetProperty(aes, BCRYPT_CHAINING_MODE, (UCHAR*)BCRYPT_CHAIN_MODE_ECB, sizeof(BCRYPT_CHAIN_MODE_ECB), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    len = 0xdeadbeef;
    size = sizeof(len);
    ret = pBCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    /* initialization vector is not allowed */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptEncrypt(key, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %08x\n", ret);
    ok(size == 16, "got %u\n", size);

    /* input size is a multiple of block size */
    size = 0;
    ret = pBCryptEncrypt(key, data, 16, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 16, "got %u\n", size);

    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 16, NULL, NULL, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 16, "got %u\n", size);
    ok(!memcmp(ciphertext, expected5, sizeof(expected5)), "wrong data\n");
    for (i = 0; i < 16; i++)
        ok(ciphertext[i] == expected5[i], "%u: %02x != %02x\n", i, ciphertext[i], expected5[i]);

    /* input size is not a multiple of block size */
    size = 0;
    ret = pBCryptEncrypt(key, data, 17, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got %08x\n", ret);
    ok(size == 17, "got %u\n", size);

    /* input size is not a multiple of block size, block padding set */
    size = 0;
    ret = pBCryptEncrypt(key, data, 17, NULL, NULL, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 17, NULL, NULL, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(ciphertext, expected6, sizeof(expected6)), "wrong data\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected6[i], "%u: %02x != %02x\n", i, ciphertext[i], expected6[i]);

    /* input size is a multiple of block size, block padding set */
    size = 0;
    ret = pBCryptEncrypt(key, data2, 32, NULL, NULL, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 48, "got %u\n", size);

    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, NULL, NULL, 16, ciphertext, 48, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 48, "got %u\n", size);
    ok(!memcmp(ciphertext, expected7, sizeof(expected7)), "wrong data\n");
    for (i = 0; i < 48; i++)
        ok(ciphertext[i] == expected7[i], "%u: %02x != %02x\n", i, ciphertext[i], expected7[i]);

    /* output size too small */
    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data, 17, NULL, NULL, 16, ciphertext, 31, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = pBCryptEncrypt(key, data2, 32, NULL, NULL, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(size == 48, "got %u\n", size);

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    ret = pBCryptCloseAlgorithmProvider(aes, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
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
    BCRYPT_ALG_HANDLE aes;
    BCRYPT_KEY_HANDLE key;
    UCHAR *buf, plaintext[48], ivbuf[16];
    ULONG size, len;
    NTSTATUS ret;

    ret = pBCryptOpenAlgorithmProvider(&aes, BCRYPT_AES_ALGORITHM, NULL, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    size = 0;
    memset(&key_lengths, 0, sizeof(key_lengths));
    ret = BCryptGetProperty(aes, BCRYPT_KEY_LENGTHS, (UCHAR*)&key_lengths, sizeof(key_lengths), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == sizeof(key_lengths), "got %u\n", size);
    ok(key_lengths.dwMinLength == 128, "Expected 128, got %d\n", key_lengths.dwMinLength);
    ok(key_lengths.dwMaxLength == 256, "Expected 256, got %d\n", key_lengths.dwMaxLength);
    ok(key_lengths.dwIncrement == 64, "Expected 64, got %d\n", key_lengths.dwIncrement);

    /******************
     * AES - CBC mode *
     ******************/

    len = 0xdeadbeef;
    size = sizeof(len);
    ret = pBCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    /* input size is a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext, 32, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext, 32, NULL, ivbuf, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(plaintext, expected, sizeof(expected)), "wrong data\n");

    /* test with padding smaller than block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext2, 32, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext2, 32, NULL, ivbuf, 16, plaintext, 17, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 17, "got %u\n", size);
    ok(!memcmp(plaintext, expected2, sizeof(expected2)), "wrong data\n");

    /* test with padding of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext3, 48, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 48, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext3, 48, NULL, ivbuf, 16, plaintext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(plaintext, expected3, sizeof(expected3)), "wrong data\n");

    /* output size too small */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext, 32, NULL, ivbuf, 16, plaintext, 31, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext2, 32, NULL, ivbuf, 16, plaintext, 15, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext2, 32, NULL, ivbuf, 16, plaintext, 16, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(size == 17, "got %u\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext3, 48, NULL, ivbuf, 16, plaintext, 31, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(size == 48, "got %u\n", size);

    /* input size is not a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext, 17, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got %08x\n", ret);
    ok(size == 17 || broken(size == 0 /* Win < 7 */), "got %u\n", size);

    /* input size is not a multiple of block size, block padding set */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext, 17, NULL, ivbuf, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got %08x\n", ret);
    ok(size == 17 || broken(size == 0 /* Win < 7 */), "got %u\n", size);

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    /******************
     * AES - GCM mode *
     ******************/

    ret = BCryptSetProperty(aes, BCRYPT_CHAINING_MODE, (UCHAR*)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

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
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(plaintext, expected3, sizeof(expected3)), "wrong data\n");

    /* test with auuth data */
    auth_info.pbAuthData = auth_data;
    auth_info.cbAuthData = sizeof(auth_data);
    auth_info.pbTag = tag2;
    auth_info.cbTag = sizeof(tag2);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext4, 32, &auth_info, ivbuf, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(plaintext, expected3, sizeof(expected3)), "wrong data\n");

    /* test with wrong tag */
    memcpy(ivbuf, iv, sizeof(iv));
    auth_info.pbTag = iv; /* wrong tag */
    ret = pBCryptDecrypt(key, ciphertext4, 32, &auth_info, ivbuf, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_AUTH_TAG_MISMATCH, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    /******************
     * AES - ECB mode *
     ******************/

    ret = BCryptSetProperty(aes, BCRYPT_CHAINING_MODE, (UCHAR*)BCRYPT_CHAIN_MODE_ECB, sizeof(BCRYPT_CHAIN_MODE_ECB), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    len = 0xdeadbeef;
    size = sizeof(len);
    ret = pBCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    ret = pBCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    /* initialization vector is not allowed */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = pBCryptDecrypt(key, ciphertext5, 32, NULL, ivbuf, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);

    /* input size is a multiple of block size */
    size = 0;
    ret = pBCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(plaintext, expected, sizeof(expected)), "wrong data\n");

    /* test with padding smaller than block size */
    size = 0;
    ret = pBCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, plaintext, 17, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 17, "got %u\n", size);
    ok(!memcmp(plaintext, expected2, sizeof(expected2)), "wrong data\n");

    /* test with padding of block size */
    size = 0;
    ret = pBCryptDecrypt(key, ciphertext6, 48, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 48, "got %u\n", size);

    size = 0;
    memset(plaintext, 0, sizeof(plaintext));
    ret = pBCryptDecrypt(key, ciphertext6, 48, NULL, NULL, 16, plaintext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);
    ok(!memcmp(plaintext, expected3, sizeof(expected3)), "wrong data\n");

    /* output size too small */
    size = 0;
    ret = pBCryptDecrypt(key, ciphertext4, 32, NULL, NULL, 16, plaintext, 31, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    ret = pBCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, plaintext, 15, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(size == 32, "got %u\n", size);

    size = 0;
    ret = pBCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, plaintext, 16, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(size == 17, "got %u\n", size);

    size = 0;
    ret = pBCryptDecrypt(key, ciphertext6, 48, NULL, NULL, 16, plaintext, 31, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(size == 48, "got %u\n", size);

    /* input size is not a multiple of block size */
    size = 0;
    ret = pBCryptDecrypt(key, ciphertext4, 17, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got %08x\n", ret);
    ok(size == 17 || broken(size == 0 /* Win < 7 */), "got %u\n", size);

    /* input size is not a multiple of block size, block padding set */
    size = 0;
    ret = pBCryptDecrypt(key, ciphertext4, 17, NULL, NULL, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got %08x\n", ret);
    ok(size == 17 || broken(size == 0 /* Win < 7 */), "got %u\n", size);

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_INVALID_HANDLE, "got %08x\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    ret = pBCryptCloseAlgorithmProvider(aes, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
}

static void test_key_import_export(void)
{
    UCHAR buffer1[sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + 16];
    UCHAR buffer2[sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + 16];
    BCRYPT_KEY_DATA_BLOB_HEADER *key_data1 = (void*)buffer1;
    BCRYPT_ALG_HANDLE aes;
    BCRYPT_KEY_HANDLE key;
    NTSTATUS ret;
    ULONG size;

    ret = pBCryptOpenAlgorithmProvider(&aes, BCRYPT_AES_ALGORITHM, NULL, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    key_data1->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
    key_data1->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
    key_data1->cbKeyData = 16;
    memset(&key_data1[1], 0x11, 16);

    ret = pBCryptImportKey(aes, NULL, BCRYPT_KEY_DATA_BLOB, &key, NULL, 0, buffer1, sizeof(buffer1), 0);
    ok(ret == STATUS_SUCCESS || broken(ret == STATUS_INVALID_PARAMETER) /* vista */, "got %08x\n", ret);
    if (ret == STATUS_INVALID_PARAMETER)
    {
        win_skip("broken BCryptImportKey\n");
        return;
    }

    size = 0;
    ret = pBCryptExportKey(key, NULL, BCRYPT_KEY_DATA_BLOB, buffer2, 1, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(size == sizeof(buffer2), "Got %u\n", size);

    size = 0;
    memset(buffer2, 0xff, sizeof(buffer2));
    ret = pBCryptExportKey(key, NULL, BCRYPT_KEY_DATA_BLOB, buffer2, sizeof(buffer2), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == sizeof(buffer2), "Got %u\n", size);
    ok(!memcmp(buffer1, buffer2, sizeof(buffer1)), "Expected exported key to match imported key\n");

    ret = pBCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = pBCryptCloseAlgorithmProvider(aes, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
}

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
    BYTE buffer[sizeof(BCRYPT_ECCKEY_BLOB) + sizeof(eccPubkey)];
    BCRYPT_ECCKEY_BLOB *ecckey = (void *)buffer;
    BCRYPT_ALG_HANDLE alg = NULL;
    BCRYPT_KEY_HANDLE key = NULL;
    NTSTATUS status;

    status = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0);
    if (status)
    {
        win_skip("Failed to open ECDSA provider: %08x, skipping test\n", status);
        return;
    }

    ecckey->dwMagic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
    memcpy(ecckey + 1, eccPubkey, sizeof(eccPubkey));

    ecckey->cbKey = 2;
    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_ECCPUBLIC_BLOB, &key, buffer, sizeof(buffer), 0);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %08x\n", status);

    ecckey->cbKey = sizeof(eccPubkey) / 2;
    status = pBCryptImportKeyPair(alg, NULL, BCRYPT_ECCPUBLIC_BLOB, &key, buffer, sizeof(buffer), 0);
    ok(!status, "BCryptImportKeyPair failed: %08x\n", status);

    status = pBCryptVerifySignature(key, NULL, certHash, sizeof(certHash) - 1, certSignature, sizeof(certSignature), 0);
    ok(status == STATUS_INVALID_SIGNATURE, "Expected STATUS_INVALID_SIGNATURE, got %08x\n", status);

    status = pBCryptVerifySignature(key, NULL, certHash, sizeof(certHash), certSignature, sizeof(certSignature), 0);
    ok(!status, "BCryptVerifySignature failed: %08x\n", status);

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

static void test_RSA(void)
{
    BCRYPT_PKCS1_PADDING_INFO pad;
    BCRYPT_ALG_HANDLE alg = NULL;
    BCRYPT_KEY_HANDLE key = NULL;
    NTSTATUS ret;

    ret = pBCryptOpenAlgorithmProvider(&alg, BCRYPT_RSA_ALGORITHM, NULL, 0);
    if (ret)
    {
        win_skip("Failed to open RSA provider: %08x, skipping test\n", ret);
        return;
    }

    ret = pBCryptImportKeyPair(alg, NULL, BCRYPT_RSAPUBLIC_BLOB, &key, rsaPublicBlob, sizeof(rsaPublicBlob), 0);
    ok(!ret, "pBCryptImportKeyPair failed: %08x\n", ret);

    pad.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    ret = pBCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(!ret, "pBCryptVerifySignature failed: %08x\n", ret);

    ret = pBCryptVerifySignature(key, NULL, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %08x\n", ret);

    pad.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    ret = pBCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), 0);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %08x\n", ret);

    ret = pBCryptVerifySignature(key, NULL, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), 0);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %08x\n", ret);

    pad.pszAlgId = BCRYPT_AES_ALGORITHM;
    ret = pBCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_NOT_SUPPORTED, "Expected STATUS_NOT_SUPPORTED, got %08x\n", ret);

    pad.pszAlgId = NULL;
    ret = pBCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_INVALID_SIGNATURE, "Expected STATUS_INVALID_SIGNATURE, got %08x\n", ret);

    ret = pBCryptDestroyKey(key);
    ok(!ret, "pBCryptDestroyKey failed: %08x\n", ret);

    ret = pBCryptCloseAlgorithmProvider(alg, 0);
    ok(!ret, "pBCryptCloseAlgorithmProvider failed: %08x\n", ret);
}

START_TEST(bcrypt)
{
    HMODULE module;

    __debugbreak();
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
    pBCryptDuplicateHash = (void *)GetProcAddress(module, "BCryptDuplicateHash");
    pBCryptFinishHash = (void *)GetProcAddress(module, "BCryptFinishHash");
    pBCryptDestroyHash = (void *)GetProcAddress(module, "BCryptDestroyHash");
    pBCryptGenRandom = (void *)GetProcAddress(module, "BCryptGenRandom");
    pBCryptGetProperty = (void *)GetProcAddress(module, "BCryptGetProperty");
    pBCryptSetProperty = (void *)GetProcAddress(module, "BCryptSetProperty");
    pBCryptGenerateSymmetricKey = (void *)GetProcAddress(module, "BCryptGenerateSymmetricKey");
    pBCryptEncrypt = (void *)GetProcAddress(module, "BCryptEncrypt");
    pBCryptDecrypt = (void *)GetProcAddress(module, "BCryptDecrypt");
    pBCryptDuplicateKey = (void *)GetProcAddress(module, "BCryptDuplicateKey");
    pBCryptDestroyKey = (void *)GetProcAddress(module, "BCryptDestroyKey");
    pBCryptImportKey = (void *)GetProcAddress(module, "BCryptImportKey");
    pBCryptExportKey = (void *)GetProcAddress(module, "BCryptExportKey");
    pBCryptImportKeyPair = (void *)GetProcAddress(module, "BCryptImportKeyPair");
    pBCryptVerifySignature = (void *)GetProcAddress(module, "BCryptVerifySignature");

    test_BCryptGenRandom();
    test_BCryptGetFipsAlgorithmMode();
    test_hashes();
    test_rng();
    test_aes();
    test_BCryptGenerateSymmetricKey();
    test_BCryptEncrypt();
    test_BCryptDecrypt();
    test_key_import_export();
    test_ECDSA();
    test_RSA();

    if (pBCryptHash) /* >= Win 10 */
        test_BcryptHash();
    else
        win_skip("BCryptHash is not available\n");

    FreeLibrary(module);
}
