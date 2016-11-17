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

static NTSTATUS (WINAPI *pBCryptHash)( BCRYPT_ALG_HANDLE algorithm, UCHAR *secret, ULONG secretlen,
                                     UCHAR *input, ULONG inputlen, UCHAR *output, ULONG outputlen );

static void test_BCryptGenRandom(void)
{
    NTSTATUS ret;
    UCHAR buffer[256];

    ret = BCryptGenRandom(NULL, NULL, 0, 0);
    ok(ret == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got 0x%x\n", ret);
    ret = BCryptGenRandom(NULL, buffer, 0, 0);
    ok(ret == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got 0x%x\n", ret);
    ret = BCryptGenRandom(NULL, buffer, sizeof(buffer), 0);
    ok(ret == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got 0x%x\n", ret);
    ret = BCryptGenRandom(NULL, buffer, sizeof(buffer), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ok(ret == STATUS_SUCCESS, "Expected success, got 0x%x\n", ret);
    ret = BCryptGenRandom(NULL, buffer, sizeof(buffer),
          BCRYPT_USE_SYSTEM_PREFERRED_RNG|BCRYPT_RNG_USE_ENTROPY_IN_BUFFER);
    ok(ret == STATUS_SUCCESS, "Expected success, got 0x%x\n", ret);
    ret = BCryptGenRandom(NULL, NULL, sizeof(buffer), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got 0x%x\n", ret);

    /* Zero sized buffer should work too */
    ret = BCryptGenRandom(NULL, buffer, 0, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ok(ret == STATUS_SUCCESS, "Expected success, got 0x%x\n", ret);

    /* Test random number generation - It's impossible for a sane RNG to return 8 zeros */
    memset(buffer, 0, 16);
    ret = BCryptGenRandom(NULL, buffer, 8, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
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
    BOOLEAN expected;
    BOOLEAN enabled;
    DWORD value, count[2] = {sizeof(value), sizeof(value)};
    NTSTATUS ret;

    if (!RegGetValueW(HKEY_LOCAL_MACHINE, policyKeyVistaW, policyValueVistaW,
        RRF_RT_REG_DWORD, NULL, &value, &count[0]))
    {
        expected = !!value;
    }
    else if (!RegGetValueW(HKEY_LOCAL_MACHINE, policyKeyXPW, policyValueXPW,
             RRF_RT_REG_DWORD, NULL, &value, &count[1]))
    {
        expected = !!value;
    }
    else
    {
        expected = FALSE;
todo_wine
        ok(0, "Neither XP or Vista key is present\n");
    }

    ret = BCryptGetFipsAlgorithmMode(&enabled);
    ok(ret == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%x\n", ret);
    ok(enabled == expected, "expected result %d, got %d\n", expected, enabled);

    ret = BCryptGetFipsAlgorithmMode(NULL);
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

#define test_hash_length(a,b) _test_hash_length(__LINE__,a,b)
static void _test_hash_length(unsigned line, void *handle, ULONG exlen)
{
    ULONG len = 0xdeadbeef, size = 0xdeadbeef;
    NTSTATUS status;

    status = BCryptGetProperty(handle, BCRYPT_HASH_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
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

    status = BCryptGetProperty(handle, BCRYPT_ALGORITHM_NAME, buf, sizeof(buf), &size, 0);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "BCryptGetProperty failed: %08x\n", status);
    ok_(__FILE__,line)(size == (strlen(exname)+1)*sizeof(WCHAR), "got %u\n", size);
    ok_(__FILE__,line)(!strcmp_wa(name, exname), "alg name = %s, expected %s\n", wine_dbgstr_w(name), exname);
}

static void test_sha1(void)
{
    static const char expected[] = "961fa64958818f767707072755d7018dcd278e94";
    static const char expected_hmac[] = "2472cf65d0e090618d769d3e46f0d9446cf212da";
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_HASH_HANDLE hash;
    UCHAR buf[512], buf_hmac[1024], sha1[20], sha1_hmac[20];
    ULONG size, len;
    char str[41];
    NTSTATUS ret;

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA1_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(NULL, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, NULL, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, NULL, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == sizeof(len), "got %u\n", size);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, 0, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(len == 0xdeadbeef, "got %u\n", len);
    ok(size == sizeof(len), "got %u\n", size);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len , sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(len != 0xdeadbeef, "len not set\n");
    ok(size == sizeof(len), "got %u\n", size);

    test_hash_length(alg, 20);
    test_alg_name(alg, "SHA1");

    hash = NULL;
    len = sizeof(buf);
    ret = BCryptCreateHash(alg, &hash, buf, len, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(hash != NULL, "hash not set\n");

    ret = BCryptHashData(hash, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = BCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    test_hash_length(hash, 20);
    test_alg_name(hash, "SHA1");

    memset(sha1, 0, sizeof(sha1));
    ret = BCryptFinishHash(hash, sha1, sizeof(sha1), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    format_hash( sha1, sizeof(sha1), str );
    ok(!strcmp(str, expected), "got %s\n", str);

    ret = BCryptDestroyHash(hash);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA1_ALGORITHM, MS_PRIMITIVE_PROVIDER, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    hash = NULL;
    len = sizeof(buf_hmac);
    ret = BCryptCreateHash(alg, &hash, buf_hmac, len, (UCHAR *)"key", sizeof("key"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(hash != NULL, "hash not set\n");

    ret = BCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    test_hash_length(hash, 20);
    test_alg_name(hash, "SHA1");

    memset(sha1_hmac, 0, sizeof(sha1_hmac));
    ret = BCryptFinishHash(hash, sha1_hmac, sizeof(sha1_hmac), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    format_hash( sha1_hmac, sizeof(sha1_hmac), str );
    ok(!strcmp(str, expected_hmac), "got %s\n", str);

    ret = BCryptDestroyHash(hash);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
}

static void test_sha256(void)
{
    static const char expected[] =
        "ceb73749c899693706ede1e30c9929b3fd5dd926163831c2fb8bd41e6efb1126";
    static const char expected_hmac[] =
        "34c1aa473a4468a91d06e7cdbc75bc4f93b830ccfc2a47ffd74e8e6ed29e4c72";
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_HASH_HANDLE hash;
    UCHAR buf[512], buf_hmac[1024], sha256[32], sha256_hmac[32];
    ULONG size, len;
    char str[65];
    NTSTATUS ret;

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(NULL, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, NULL, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, NULL, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == sizeof(len), "got %u\n", size);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, 0, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(len == 0xdeadbeef, "got %u\n", len);
    ok(size == sizeof(len), "got %u\n", size);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len , sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(len != 0xdeadbeef, "len not set\n");
    ok(size == sizeof(len), "got %u\n", size);

    test_hash_length(alg, 32);
    test_alg_name(alg, "SHA256");

    hash = NULL;
    len = sizeof(buf);
    ret = BCryptCreateHash(alg, &hash, buf, len, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(hash != NULL, "hash not set\n");

    ret = BCryptHashData(hash, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = BCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    test_hash_length(hash, 32);
    test_alg_name(hash, "SHA256");

    memset(sha256, 0, sizeof(sha256));
    ret = BCryptFinishHash(hash, sha256, sizeof(sha256), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    format_hash( sha256, sizeof(sha256), str );
    ok(!strcmp(str, expected), "got %s\n", str);

    ret = BCryptDestroyHash(hash);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, MS_PRIMITIVE_PROVIDER, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    hash = NULL;
    len = sizeof(buf_hmac);
    ret = BCryptCreateHash(alg, &hash, buf_hmac, len, (UCHAR *)"key", sizeof("key"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(hash != NULL, "hash not set\n");

    ret = BCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    test_hash_length(hash, 32);
    test_alg_name(hash, "SHA256");

    memset(sha256_hmac, 0, sizeof(sha256_hmac));
    ret = BCryptFinishHash(hash, sha256_hmac, sizeof(sha256_hmac), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    format_hash( sha256_hmac, sizeof(sha256_hmac), str );
    ok(!strcmp(str, expected_hmac), "got %s\n", str);

    ret = BCryptDestroyHash(hash);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
}

static void test_sha384(void)
{
    static const char expected[] =
        "62b21e90c9022b101671ba1f808f8631a8149f0f12904055839a35c1ca78ae5363eed1e743a692d70e0504b0cfd12ef9";
    static const char expected_hmac[] =
        "4b3e6d6ff2da121790ab7e7b9247583e3a7eed2db5bd4dabc680303b1608f37dfdc836d96a704c03283bc05b4f6c5eb8";
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_HASH_HANDLE hash;
    UCHAR buf[512], buf_hmac[1024], sha384[48], sha384_hmac[48];
    ULONG size, len;
    char str[97];
    NTSTATUS ret;

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA384_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(NULL, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, NULL, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, NULL, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == sizeof(len), "got %u\n", size);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, 0, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(len == 0xdeadbeef, "got %u\n", len);
    ok(size == sizeof(len), "got %u\n", size);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len , sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(len != 0xdeadbeef, "len not set\n");
    ok(size == sizeof(len), "got %u\n", size);

    test_hash_length(alg, 48);
    test_alg_name(alg, "SHA384");

    hash = NULL;
    len = sizeof(buf);
    ret = BCryptCreateHash(alg, &hash, buf, len, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(hash != NULL, "hash not set\n");

    ret = BCryptHashData(hash, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = BCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    test_hash_length(hash, 48);
    test_alg_name(hash, "SHA384");

    memset(sha384, 0, sizeof(sha384));
    ret = BCryptFinishHash(hash, sha384, sizeof(sha384), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    format_hash( sha384, sizeof(sha384), str );
    ok(!strcmp(str, expected), "got %s\n", str);

    ret = BCryptDestroyHash(hash);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA384_ALGORITHM, MS_PRIMITIVE_PROVIDER, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    hash = NULL;
    len = sizeof(buf_hmac);
    ret = BCryptCreateHash(alg, &hash, buf_hmac, len, (UCHAR *)"key", sizeof("key"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(hash != NULL, "hash not set\n");

    ret = BCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    test_hash_length(hash, 48);
    test_alg_name(hash, "SHA384");

    memset(sha384_hmac, 0, sizeof(sha384_hmac));
    ret = BCryptFinishHash(hash, sha384_hmac, sizeof(sha384_hmac), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    format_hash( sha384_hmac, sizeof(sha384_hmac), str );
    ok(!strcmp(str, expected_hmac), "got %s\n", str);

    ret = BCryptDestroyHash(hash);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
}

static void test_sha512(void)
{
    static const char expected[] =
        "d55ced17163bf5386f2cd9ff21d6fd7fe576a915065c24744d09cfae4ec84ee1e"
        "f6ef11bfbc5acce3639bab725b50a1fe2c204f8c820d6d7db0df0ecbc49c5ca";
    static const char expected_hmac[] =
        "415fb6b10018ca03b38a1b1399c42ac0be5e8aceddb9a73103f5e543bf2d888f2"
        "eecf91373941f9315dd730a77937fa92444450fbece86f409d9cb5ec48c6513";
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_HASH_HANDLE hash;
    UCHAR buf[512], buf_hmac[1024], sha512[64], sha512_hmac[64];
    ULONG size, len;
    char str[129];
    NTSTATUS ret;

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA512_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(NULL, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, NULL, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, NULL, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == sizeof(len), "got %u\n", size);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, 0, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(len == 0xdeadbeef, "got %u\n", len);
    ok(size == sizeof(len), "got %u\n", size);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len , sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(len != 0xdeadbeef, "len not set\n");
    ok(size == sizeof(len), "got %u\n", size);

    test_hash_length(alg, 64);
    test_alg_name(alg, "SHA512");

    hash = NULL;
    len = sizeof(buf);
    ret = BCryptCreateHash(alg, &hash, buf, len, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(hash != NULL, "hash not set\n");

    ret = BCryptHashData(hash, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = BCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    test_hash_length(hash, 64);
    test_alg_name(hash, "SHA512");

    memset(sha512, 0, sizeof(sha512));
    ret = BCryptFinishHash(hash, sha512, sizeof(sha512), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    format_hash( sha512, sizeof(sha512), str );
    ok(!strcmp(str, expected), "got %s\n", str);

    ret = BCryptDestroyHash(hash);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA512_ALGORITHM, MS_PRIMITIVE_PROVIDER, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    hash = NULL;
    len = sizeof(buf_hmac);
    ret = BCryptCreateHash(alg, &hash, buf_hmac, len, (UCHAR *)"key", sizeof("key"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(hash != NULL, "hash not set\n");

    ret = BCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    test_hash_length(hash, 64);
    test_alg_name(hash, "SHA512");

    memset(sha512_hmac, 0, sizeof(sha512_hmac));
    ret = BCryptFinishHash(hash, sha512_hmac, sizeof(sha512_hmac), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    format_hash( sha512_hmac, sizeof(sha512_hmac), str );
    ok(!strcmp(str, expected_hmac), "got %s\n", str);

    ret = BCryptDestroyHash(hash);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

}

static void test_md5(void)
{
    static const char expected[] =
        "e2a3e68d23ce348b8f68b3079de3d4c9";
    static const char expected_hmac[] =
        "7bda029b93fa8d817fcc9e13d6bdf092";
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_HASH_HANDLE hash;
    UCHAR buf[512], buf_hmac[1024], md5[16], md5_hmac[16];
    ULONG size, len;
    char str[65];
    NTSTATUS ret;

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_MD5_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(NULL, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, NULL, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %08x\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, NULL, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(size == sizeof(len), "got %u\n", size);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, 0, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %08x\n", ret);
    ok(len == 0xdeadbeef, "got %u\n", len);
    ok(size == sizeof(len), "got %u\n", size);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len , sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(len != 0xdeadbeef, "len not set\n");
    ok(size == sizeof(len), "got %u\n", size);

    test_hash_length(alg, 16);
    test_alg_name(alg, "MD5");

    hash = NULL;
    len = sizeof(buf);
    ret = BCryptCreateHash(alg, &hash, buf, len, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(hash != NULL, "hash not set\n");

    ret = BCryptHashData(hash, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = BCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    test_hash_length(hash, 16);
    test_alg_name(hash, "MD5");

    memset(md5, 0, sizeof(md5));
    ret = BCryptFinishHash(hash, md5, sizeof(md5), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    format_hash( md5, sizeof(md5), str );
    ok(!strcmp(str, expected), "got %s\n", str);

    ret = BCryptDestroyHash(hash);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_MD5_ALGORITHM, MS_PRIMITIVE_PROVIDER, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    hash = NULL;
    len = sizeof(buf_hmac);
    ret = BCryptCreateHash(alg, &hash, buf_hmac, len, (UCHAR *)"key", sizeof("key"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(hash != NULL, "hash not set\n");

    ret = BCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    test_hash_length(hash, 16);
    test_alg_name(hash, "MD5");

    memset(md5_hmac, 0, sizeof(md5_hmac));
    ret = BCryptFinishHash(hash, md5_hmac, sizeof(md5_hmac), 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    format_hash( md5_hmac, sizeof(md5_hmac), str );
    ok(!strcmp(str, expected_hmac), "got %s\n", str);

    ret = BCryptDestroyHash(hash);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
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
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_MD5_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    test_hash_length(alg, 16);
    test_alg_name(alg, "MD5");

    memset(md5, 0, sizeof(md5));
    ret = pBCryptHash(alg, NULL, 0, (UCHAR *)"test", sizeof("test"), md5, sizeof(md5));
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    format_hash( md5, sizeof(md5), str );
    ok(!strcmp(str, expected), "got %s\n", str);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);

    alg = NULL;
    memset(md5_hmac, 0, sizeof(md5_hmac));
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_MD5_ALGORITHM, MS_PRIMITIVE_PROVIDER, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    ret = pBCryptHash(alg, (UCHAR *)"key", sizeof("key"), (UCHAR *)"test", sizeof("test"), md5_hmac, sizeof(md5_hmac));
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    format_hash( md5_hmac, sizeof(md5_hmac), str );
    ok(!strcmp(str, expected_hmac), "got %s\n", str);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
}

static void test_rng(void)
{
    BCRYPT_ALG_HANDLE alg;
    ULONG size, len;
    UCHAR buf[16];
    NTSTATUS ret;

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_RNG_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(alg != NULL, "alg not set\n");

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got %08x\n", ret);

    test_alg_name(alg, "RNG");

    memset(buf, 0, 16);
    ret = BCryptGenRandom(alg, buf, 8, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
    ok(memcmp(buf, buf + 8, 8), "got zeroes\n");

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %08x\n", ret);
}


START_TEST(bcrypt)
{
    HMODULE module;

    module = GetModuleHandleA( "bcrypt.dll" );

    test_BCryptGenRandom();
    test_BCryptGetFipsAlgorithmMode();
    test_sha1();
    test_sha256();
    test_sha384();
    test_sha512();
    test_md5();
    test_rng();

    pBCryptHash = (void *)GetProcAddress( module, "BCryptHash" );

    if (pBCryptHash)
        test_BcryptHash();
    else
        win_skip("BCryptHash is not available\n");
}
