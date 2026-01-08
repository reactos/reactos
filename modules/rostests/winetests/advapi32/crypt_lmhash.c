/*
 * Unit tests for SystemFunctionXXX (LMHash?)
 *
 * Copyright 2004 Hans Leidekker
 * Copyright 2006 Mike McCormack
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"

struct ustring {
    DWORD Length;
    DWORD MaximumLength;
    unsigned char *Buffer;
};

typedef int (WINAPI *descrypt)(unsigned char *, unsigned char *, unsigned char *);
static NTSTATUS (WINAPI *pSystemFunction001)(const BYTE *, const BYTE *, LPBYTE);
static NTSTATUS (WINAPI *pSystemFunction002)(const BYTE *, const BYTE *, LPBYTE);
static NTSTATUS (WINAPI *pSystemFunction003)(const BYTE *, LPBYTE);
static NTSTATUS (WINAPI *pSystemFunction004)(const struct ustring *, const struct ustring *, struct ustring *);
static NTSTATUS (WINAPI *pSystemFunction005)(const struct ustring *, const struct ustring *, struct ustring *);
static VOID (WINAPI *pSystemFunction006)( PCSTR passwd, PSTR lmhash );
static NTSTATUS (WINAPI *pSystemFunction008)(const BYTE *, const BYTE *, LPBYTE);
static NTSTATUS (WINAPI *pSystemFunction009)(const BYTE *, const BYTE *, LPBYTE);
static NTSTATUS (WINAPI *pSystemFunction032)(struct ustring *, const struct ustring *);

/* encrypt two blocks */
static descrypt pSystemFunction012;
static descrypt pSystemFunction014;
static descrypt pSystemFunction016;
static descrypt pSystemFunction018;
static descrypt pSystemFunction020;
static descrypt pSystemFunction022;

/* decrypt two blocks */
static descrypt pSystemFunction013;
static descrypt pSystemFunction015;
static descrypt pSystemFunction017;
static descrypt pSystemFunction019;
static descrypt pSystemFunction021;
static descrypt pSystemFunction023;

/* encrypt two blocks with a 32bit key */
static descrypt pSystemFunction024;
static descrypt pSystemFunction025;

/* decrypt two blocks with a 32bit key */
static descrypt pSystemFunction026;
static descrypt pSystemFunction027;

typedef int (WINAPI *memcmpfunc)(unsigned char *, unsigned char *);
static memcmpfunc pSystemFunction030;
static memcmpfunc pSystemFunction031;

static void test_SystemFunction006(void)
{
    char lmhash[16 + 1];

    char passwd[] = { 's','e','c','r','e','t', 0, 0, 0, 0, 0, 0, 0, 0 };
    unsigned char expect[] = 
        { 0x85, 0xf5, 0x28, 0x9f, 0x09, 0xdc, 0xa7, 0xeb,
          0xaa, 0xd3, 0xb4, 0x35, 0xb5, 0x14, 0x04, 0xee };

    pSystemFunction006( passwd, lmhash );

    ok( !memcmp( lmhash, expect, sizeof(expect) ),
        "lmhash: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
        lmhash[0], lmhash[1], lmhash[2], lmhash[3], lmhash[4], lmhash[5],
        lmhash[6], lmhash[7], lmhash[8], lmhash[9], lmhash[10], lmhash[11],
        lmhash[12], lmhash[13], lmhash[14], lmhash[15] );
}

static void test_SystemFunction008(void)
{
    /* example data from http://davenport.sourceforge.net/ntlm.html */
    unsigned char hash[0x40] = {
        0xff, 0x37, 0x50, 0xbc, 0xc2, 0xb2, 0x24, 0x12,
        0xc2, 0x26, 0x5b, 0x23, 0x73, 0x4e, 0x0d, 0xac };
    unsigned char challenge[0x40] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };
    unsigned char expected[0x18] = {
        0xc3, 0x37, 0xcd, 0x5c, 0xbd, 0x44, 0xfc, 0x97,
        0x82, 0xa6, 0x67, 0xaf, 0x6d, 0x42, 0x7c, 0x6d,
        0xe6, 0x7c, 0x20, 0xc2, 0xd3, 0xe7, 0x7c, 0x56 };
    unsigned char output[0x18];
    NTSTATUS r;

    r = pSystemFunction008(0,0,0);
    ok( r == STATUS_UNSUCCESSFUL, "wrong error code\n");

    r = pSystemFunction008(challenge,0,0);
    ok( r == STATUS_UNSUCCESSFUL, "wrong error code\n");

    r = pSystemFunction008(challenge, hash, 0);
    ok( r == STATUS_UNSUCCESSFUL, "wrong error code\n");

    /* crashes */
    if (0)
    {
        r = pSystemFunction008(challenge, 0, output);
        ok( r == STATUS_UNSUCCESSFUL, "wrong error code\n");
    }

    r = pSystemFunction008(0, 0, output);
    ok( r == STATUS_UNSUCCESSFUL, "wrong error code\n");

    memset(output, 0, sizeof output);
    r = pSystemFunction008(challenge, hash, output);
    ok( r == STATUS_SUCCESS, "wrong error code\n");

    ok( !memcmp(output, expected, sizeof expected), "response wrong\n");
}

static void test_SystemFunction001(void)
{
    unsigned char key[8] = { 0xff, 0x37, 0x50, 0xbc, 0xc2, 0xb2, 0x24, 0 };
    unsigned char data[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };
    unsigned char expected[8] = { 0xc3, 0x37, 0xcd, 0x5c, 0xbd, 0x44, 0xfc, 0x97 };
    unsigned char output[16];
    NTSTATUS r;

    r = pSystemFunction001(0,0,0);
    ok( r == STATUS_UNSUCCESSFUL, "wrong error code\n");

    memset(output, 0, sizeof output);

    r = pSystemFunction001(data,key,output);
    ok( r == STATUS_SUCCESS, "wrong error code\n");

    ok(!memcmp(output, expected, sizeof expected), "response wrong\n");
}

static void test_SystemFunction002(void)
{
    /* reverse of SystemFunction001 */
    unsigned char key[8] = { 0xff, 0x37, 0x50, 0xbc, 0xc2, 0xb2, 0x24, 0 };
    unsigned char expected[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };
    unsigned char data[8] = { 0xc3, 0x37, 0xcd, 0x5c, 0xbd, 0x44, 0xfc, 0x97 };
    unsigned char output[8];
    int r;

    memset(output, 0, sizeof output);
    r = pSystemFunction002(data, key, output);
    ok(r == STATUS_SUCCESS, "function failed\n");
    ok(!memcmp(output, expected, sizeof expected), "response wrong\n");
}

static void test_SystemFunction032(void)
{
    struct ustring key, data;
    unsigned char szKey[] = { 'f','o','o',0 };
    unsigned char szData[8] = { 'b','a','r',0 };
    unsigned char expected[] = {0x28, 0xb9, 0xf8, 0xe1};
    int r;

    /* crashes:    pSystemFunction032(NULL,NULL); */

    key.Buffer = szKey;
    key.Length = sizeof szKey;
    key.MaximumLength = key.Length;

    data.Buffer = szData;
    data.Length = 4;
    data.MaximumLength = 8;

    r = pSystemFunction032(&data, &key);
    ok(r == STATUS_SUCCESS, "function failed\n");

    ok(!memcmp(expected, data.Buffer, data.Length), "wrong result\n");
}

static void test_SystemFunction003(void)
{
    unsigned char output[8], data[8];
    unsigned char key[7] = { 0xff, 0x37, 0x50, 0xbc, 0xc2, 0xb2, 0x24 };
    unsigned char exp1[8] = { 0x9d, 0x21, 0xc8, 0x86, 0x6c, 0x21, 0xcf, 0x43 };
    char exp2[] = "KGS!@#$%";
    int r;

    r = pSystemFunction003(NULL, NULL);
    ok(r == STATUS_UNSUCCESSFUL, "function failed\n");

    r = pSystemFunction003(key, NULL);
    ok(r == STATUS_UNSUCCESSFUL, "function failed\n");

    memset(data, 0, sizeof data);
    r = pSystemFunction003(key, data);
    ok(r == STATUS_SUCCESS, "function failed\n");
    ok( !memcmp(exp1, data, sizeof data), "decrypted message wrong\n");

    memset(output, 0, sizeof output);
    r = pSystemFunction002(data, key, output);
    ok(r == STATUS_SUCCESS, "function failed\n");

    ok( !memcmp(exp2, output, sizeof output), "decrypted message wrong\n");
}

static void test_SystemFunction004(void)
{
    unsigned char inbuf[0x100], keybuf[0x100], resbuf[0x100];
    unsigned char output[8];
    int r;
    struct ustring in, key, out;

    /* crash 
    r = pSystemFunction004(NULL, NULL, NULL);
    ok(r == STATUS_UNSUCCESSFUL, "function failed\n");
    */

    memset(inbuf, 0, sizeof inbuf);
    memset(keybuf, 0, sizeof keybuf);
    memset(resbuf, 0, sizeof resbuf);

    in.Buffer = NULL;
    in.Length = in.MaximumLength = 0;

    key.Buffer = NULL;
    key.Length = key.MaximumLength = 0;

    out.Buffer = NULL;
    out.Length = out.MaximumLength = 0;

    r = pSystemFunction004(&in, &key, &out);
    ok(r == STATUS_INVALID_PARAMETER_2, "function failed\n");

    key.Buffer = keybuf;
    key.Length = 0x100;
    key.MaximumLength = 0x100;

    r = pSystemFunction004(&in, &key, &out);
    ok(r == STATUS_BUFFER_TOO_SMALL, "function failed\n");

    in.Buffer = inbuf;
    in.Length = 0x0c;
    in.MaximumLength = 0;

    /* add two identical blocks... */
    inbuf[0] = 1;
    inbuf[1] = 2;
    inbuf[2] = 3;
    inbuf[3] = 4;

    inbuf[8] = 1;
    inbuf[9] = 2;
    inbuf[10] = 3;
    inbuf[11] = 4;

    /* check that the Length field is really obeyed */
    keybuf[6] = 1;

    key.Buffer = keybuf;
    key.Length = 6;
    key.MaximumLength = 0;

    keybuf[1] = 0x33;

    out.Buffer = resbuf;
    out.Length = 0;
    out.MaximumLength = 0x40;
    r = pSystemFunction004(&in, &key, &out);
    ok(r == STATUS_SUCCESS, "function failed\n");

    keybuf[6] = 0;

    memset(output, 0, sizeof output);
    r = pSystemFunction002(out.Buffer, key.Buffer, output);
    ok(r == STATUS_SUCCESS, "function failed\n");

    ok(((unsigned int*)output)[0] == in.Length, "encrypted length wrong\n");
    ok(((unsigned int*)output)[1] == 1, "encrypted value wrong\n");

    memset(output, 0, sizeof output);
    r = pSystemFunction002(out.Buffer+8, key.Buffer, output);
    ok(r == STATUS_SUCCESS, "function failed\n");
    ok(!memcmp(output, inbuf, sizeof output), "encrypted data wrong\n");

    memset(output, 0, sizeof output);
    r = pSystemFunction002(out.Buffer+16, key.Buffer, output);
    ok(r == STATUS_SUCCESS, "function failed\n");
    ok(!memcmp(output, inbuf, sizeof output), "encrypted data wrong\n");
}

static void test_SystemFunction005(void)
{
    char output[0x40], result[0x40];
    int r;
    struct ustring in, key, out, res;
    static char datastr[] = "twinkle twinkle little star";
    static char keystr[]  = "byolnim";

    in.Buffer = (unsigned char *)datastr;
    in.Length = strlen(datastr);
    in.MaximumLength = 0;

    key.Buffer = (unsigned char *)keystr;
    key.Length = strlen(keystr);
    key.MaximumLength = 0;

    out.Buffer = (unsigned char *)output;
    out.Length = out.MaximumLength = sizeof output;

    r = pSystemFunction004(&in, &key, &out);
    ok(r == STATUS_SUCCESS, "function failed\n");

    memset(result, 0, sizeof result);
    res.Buffer = (unsigned char *)result;
    res.Length = 0;
    res.MaximumLength = sizeof result;

    r = pSystemFunction005(&out, &key, &res);
    ok(r == STATUS_SUCCESS, "function failed\n");

    r = pSystemFunction005(&out, &key, &res);
    ok(r == STATUS_SUCCESS, "function failed\n");

    ok(res.Length == in.Length, "Length wrong\n");
    ok(!memcmp(res.Buffer, in.Buffer, in.Length), "data wrong\n");

    out.Length = 0;
    out.MaximumLength = 0;
    r = pSystemFunction005(&out, &key, &res);
    ok(r == STATUS_SUCCESS ||
       r == STATUS_INVALID_PARAMETER_1, /* Vista */
       "Expected STATUS_SUCCESS or STATUS_INVALID_PARAMETER_1, got %08x\n", r);

    ok(res.Length == in.Length, "Length wrong\n");
    ok(!memcmp(res.Buffer, in.Buffer, in.Length), "data wrong\n");

    res.MaximumLength = 0;
    r = pSystemFunction005(&out, &key, &res);
    ok(r == STATUS_BUFFER_TOO_SMALL ||
       r == STATUS_INVALID_PARAMETER_1, /* Vista */
       "Expected STATUS_BUFFER_TOO_SMALL or STATUS_INVALID_PARAMETER_1, got %08x\n", r);

    key.Length = 1;
    r = pSystemFunction005(&out, &key, &res);
    ok(r == STATUS_UNKNOWN_REVISION ||
       r == STATUS_INVALID_PARAMETER_1, /* Vista */
       "Expected STATUS_UNKNOWN_REVISION or STATUS_INVALID_PARAMETER_1, got %08x\n", r);

    key.Length = 0;
    r = pSystemFunction005(&out, &key, &res);
    ok(r == STATUS_INVALID_PARAMETER_2, "function failed\n");
}

static void test_SystemFunction009(void)
{
    unsigned char hash[0x10] = {
        0xff, 0x37, 0x50, 0xbc, 0xc2, 0xb2, 0x24, 0x12,
        0xc2, 0x26, 0x5b, 0x23, 0x73, 0x4e, 0x0d, 0xac };
    unsigned char challenge[8] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };
    unsigned char expected[0x18] = {
        0xc3, 0x37, 0xcd, 0x5c, 0xbd, 0x44, 0xfc, 0x97,
        0x82, 0xa6, 0x67, 0xaf, 0x6d, 0x42, 0x7c, 0x6d,
        0xe6, 0x7c, 0x20, 0xc2, 0xd3, 0xe7, 0x7c, 0x56 };
    unsigned char output[0x18];
    int r;

    memset(output, 0, sizeof output);
    r = pSystemFunction009(challenge, hash, output);
    ok( r == STATUS_SUCCESS, "wrong error code\n");
    ok(!memcmp(output, expected, sizeof expected), "response wrong\n");
}

static unsigned char des_key[] = { 
    0xff, 0x37, 0x50, 0xbc, 0xc2, 0xb2, 0x24, 
    0xff, 0x37, 0x50, 0xbc, 0xc2, 0xb2, 0x24, 
};
static unsigned char des_plaintext[] = { 
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0 
};
static unsigned char des_ciphertext[] = { 
    0xc3, 0x37, 0xcd, 0x5c, 0xbd, 0x44, 0xfc, 0x97,
    0xc3, 0x37, 0xcd, 0x5c, 0xbd, 0x44, 0xfc, 0x97, 0
};

/* test functions that encrypt two DES blocks */
static void test_SystemFunction_encrypt(descrypt func, int num)
{
    unsigned char output[0x11];
    int r;

    if (!func)
    {
        win_skip("SystemFunction%03d is not available\n", num);
        return;
    }

    r = func(NULL, NULL, NULL);
    ok( r == STATUS_UNSUCCESSFUL, "wrong error code\n");

    memset(output, 0, sizeof output);
    r = func(des_plaintext, des_key, output);
    ok( r == STATUS_SUCCESS, "wrong error code\n");
    ok( !memcmp(des_ciphertext, output, sizeof des_ciphertext), "ciphertext wrong (%d)\n", num);
}

/* test functions that decrypt two DES blocks */
static void test_SystemFunction_decrypt(descrypt func, int num)
{
    unsigned char output[0x11];
    int r;

    if (!func)
    {
        win_skip("SystemFunction%03d is not available\n", num);
        return;
    }

    r = func(NULL, NULL, NULL);
    ok( r == STATUS_UNSUCCESSFUL, "wrong error code\n");

    memset(output, 0, sizeof output);

    r = func(des_ciphertext, des_key, output);
    ok( r == STATUS_SUCCESS, "wrong error code\n");
    ok( !memcmp(des_plaintext, output, sizeof des_plaintext), "plaintext wrong (%d)\n", num);
}

static unsigned char des_ciphertext32[] = { 
    0x69, 0x51, 0x35, 0x69, 0x0d, 0x29, 0x24, 0xad,
    0x23, 0x6d, 0xfd, 0x43, 0x0d, 0xd3, 0x25, 0x81, 0
};

static void test_SystemFunction_enc32(descrypt func, int num)
{
    unsigned char key[4], output[0x11];
    int r;

    if (!func)
    {
        win_skip("SystemFunction%03d is not available\n", num);
        return;
    }

    memset(output, 0, sizeof output);

    /* two keys are generated using 4 bytes, repeated 4 times ... */
    memcpy(key, "foo", 4);

    r = func(des_plaintext, key, output);
    ok( r == STATUS_SUCCESS, "wrong error code (%d)\n", num);

    ok( !memcmp( output, des_ciphertext32, sizeof des_ciphertext32), "ciphertext wrong (%d)\n", num);
}

static void test_SystemFunction_dec32(descrypt func, int num)
{
    unsigned char key[4], output[0x11];
    int r;

    if (!func)
    {
        win_skip("SystemFunction%03d is not available\n", num);
        return;
    }

    memset(output, 0, sizeof output);

    /* two keys are generated using 4 bytes, repeated 4 times ... */
    memcpy(key, "foo", 4);

    r = func(des_ciphertext32, key, output);
    ok( r == STATUS_SUCCESS, "wrong error code (%d)\n", num);

    ok( !memcmp( output, des_plaintext, sizeof des_plaintext), "plaintext wrong (%d)\n", num);
}

static void test_memcmpfunc(memcmpfunc fn)
{
    unsigned char arg1[0x20], arg2[0x20];
    int r;

    if (!fn)
    {
        win_skip("function is not available\n");
        return;
    }

    memset(arg1, 0, sizeof arg1);
    memset(arg2, 0, sizeof arg2);
    arg1[0x10] = 1;

    r = fn(arg1, arg2);
    ok( r == 1, "wrong error code\n");

    memset(arg1, 1, sizeof arg1);
    memset(arg2, 1, sizeof arg2);
    arg1[0x10] = 0;

    r = fn(arg1, arg2);
    ok( r == 1, "wrong error code\n");

    memset(arg1, 0, sizeof arg1);
    memset(arg2, 1, sizeof arg2);

    r = fn(arg1, arg2);
    ok( r == 0, "wrong error code\n");

    memset(arg1, 1, sizeof arg1);
    memset(arg2, 0, sizeof arg2);

    r = fn(arg1, arg2);
    ok( r == 0, "wrong error code\n");
}

START_TEST(crypt_lmhash)
{
    HMODULE module = GetModuleHandleA("advapi32.dll");

    pSystemFunction001 = (void *)GetProcAddress( module, "SystemFunction001" );
    if (pSystemFunction001)
        test_SystemFunction001();
    else
        win_skip("SystemFunction001 is not available\n");

    pSystemFunction002 = (void *)GetProcAddress( module, "SystemFunction002" );
    if (pSystemFunction002)
        test_SystemFunction002();
    else
        win_skip("SystemFunction002 is not available\n");

    pSystemFunction003 = (void *)GetProcAddress( module, "SystemFunction003" );
    if (pSystemFunction003)
        test_SystemFunction003();
    else
        win_skip("SystemFunction002 is not available\n");

    pSystemFunction004 = (void *)GetProcAddress( module, "SystemFunction004" );
    if (pSystemFunction004)
        test_SystemFunction004();
    else
        win_skip("SystemFunction004 is not available\n");

    pSystemFunction005 = (void *)GetProcAddress( module, "SystemFunction005" );
    if (pSystemFunction005)
        test_SystemFunction005();
    else
        win_skip("SystemFunction005 is not available\n");

    pSystemFunction006 = (void *)GetProcAddress( module, "SystemFunction006" );
    if (pSystemFunction006) 
        test_SystemFunction006();
    else
        win_skip("SystemFunction006 is not available\n");

    pSystemFunction008 = (void *)GetProcAddress( module, "SystemFunction008" );
    if (pSystemFunction008)
        test_SystemFunction008();
    else
        win_skip("SystemFunction008 is not available\n");

    pSystemFunction009 = (void *)GetProcAddress( module, "SystemFunction009" );
    if (pSystemFunction009)
        test_SystemFunction009();
    else
        win_skip("SystemFunction009 is not available\n");

    pSystemFunction012 = (descrypt) GetProcAddress( module, "SystemFunction012");
    pSystemFunction013 = (descrypt) GetProcAddress( module, "SystemFunction013");
    pSystemFunction014 = (descrypt) GetProcAddress( module, "SystemFunction014");
    pSystemFunction015 = (descrypt) GetProcAddress( module, "SystemFunction015");
    pSystemFunction016 = (descrypt) GetProcAddress( module, "SystemFunction016");
    pSystemFunction017 = (descrypt) GetProcAddress( module, "SystemFunction017");
    pSystemFunction018 = (descrypt) GetProcAddress( module, "SystemFunction018");
    pSystemFunction019 = (descrypt) GetProcAddress( module, "SystemFunction019");
    pSystemFunction020 = (descrypt) GetProcAddress( module, "SystemFunction020");
    pSystemFunction021 = (descrypt) GetProcAddress( module, "SystemFunction021");
    pSystemFunction022 = (descrypt) GetProcAddress( module, "SystemFunction022");
    pSystemFunction023 = (descrypt) GetProcAddress( module, "SystemFunction023");

    /* these all encrypt two DES blocks */
    test_SystemFunction_encrypt(pSystemFunction012, 12);
    test_SystemFunction_encrypt(pSystemFunction014, 14);
    test_SystemFunction_encrypt(pSystemFunction016, 16);
    test_SystemFunction_encrypt(pSystemFunction018, 18);
    test_SystemFunction_encrypt(pSystemFunction020, 20);
    test_SystemFunction_encrypt(pSystemFunction022, 22);

    /* these all decrypt two DES blocks */
    test_SystemFunction_decrypt(pSystemFunction013, 13);
    test_SystemFunction_decrypt(pSystemFunction015, 15);
    test_SystemFunction_decrypt(pSystemFunction017, 17);
    test_SystemFunction_decrypt(pSystemFunction019, 19);
    test_SystemFunction_decrypt(pSystemFunction021, 21);
    test_SystemFunction_decrypt(pSystemFunction023, 23);

    pSystemFunction024 = (descrypt) GetProcAddress( module, "SystemFunction024");
    pSystemFunction025 = (descrypt) GetProcAddress( module, "SystemFunction025");
    pSystemFunction026 = (descrypt) GetProcAddress( module, "SystemFunction026");
    pSystemFunction027 = (descrypt) GetProcAddress( module, "SystemFunction027");

    /* these encrypt two DES blocks with a short key */
    test_SystemFunction_enc32(pSystemFunction024, 24);
    test_SystemFunction_enc32(pSystemFunction026, 26);

    /* these descrypt two DES blocks with a short key */
    test_SystemFunction_dec32(pSystemFunction025, 25);
    test_SystemFunction_dec32(pSystemFunction027, 27);

    pSystemFunction030 = (memcmpfunc) GetProcAddress( module, "SystemFunction030" );
    pSystemFunction031 = (memcmpfunc) GetProcAddress( module, "SystemFunction031" );

    test_memcmpfunc(pSystemFunction030);
    test_memcmpfunc(pSystemFunction031);

    pSystemFunction032 = (void *)GetProcAddress( module, "SystemFunction032" );
    if (pSystemFunction032)
        test_SystemFunction032();
    else
        win_skip("SystemFunction032 is not available\n");
}
