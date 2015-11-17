/*
 * Unit tests for rsaenh functions
 *
 * Copyright (c) 2004 Michael Jung
 * Copyright (c) 2006 Juan Lang
 * Copyright (c) 2007 Vijay Kiran Kamuju
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

#include <string.h>
#include <stdio.h>
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wincrypt.h"
#include "winreg.h"

static HCRYPTPROV hProv;
static const char *szProviders[] = {MS_ENHANCED_PROV_A, MS_DEF_PROV_A, MS_STRONG_PROV_A};
static int iProv;
static const char szContainer[] = "winetest";
static const char *szProvider;

#define ENHANCED_PROV (iProv == 0)
#define BASE_PROV (iProv == 1)
#define STRONG_PROV (iProv == 2)

typedef struct _ctdatatype {
       unsigned char origstr[32];
       unsigned char decstr[32];
       int strlen;
       int enclen;
       int buflen;
} cryptdata;

static const cryptdata cTestData[4] = {
       {"abcdefghijkl",
       {'a','b','c','d','e','f','g','h',0x2,0x2,'k','l',0},
       12,8,16},
       {"abcdefghij",
       {'a','b','c','d','e','f','g','h',0x2,0x2,0},
       10,8,16},
       {"abcdefgh",
       {'a','b','c','d','e','f','g','h',0},
       8,8,16},
       {"abcdefghijkl",
       {'a','b','c','d','e','f','g','h','i','j','k','l',0},
       12,12,16}
};

static int win2k, nt4;

/*
 * 1. Take the MD5 Hash of the container name (with an extra null byte)
 * 2. Turn the hash into a 4 DWORD hex value
 * 3. Append a '_'
 * 4. Add the MachineGuid
 *
 */
static void uniquecontainer(char *unique)
{
    /* MD5 hash of "winetest\0" in 4 DWORD hex */
    static const char szContainer_md5[] = "9d20fd8d05ed2b8455d125d0bf6d6a70";
    static const char szCryptography[] = "Software\\Microsoft\\Cryptography";
    static const char szMachineGuid[] = "MachineGuid";
    HKEY hkey;
    char guid[MAX_PATH];
    DWORD size = MAX_PATH;
    HRESULT ret;

    /* Get the MachineGUID */
    ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, szCryptography, 0, KEY_READ | KEY_WOW64_64KEY, &hkey);
    if (ret == ERROR_ACCESS_DENIED)
    {
        /* Windows 2000 can't handle KEY_WOW64_64KEY */
        RegOpenKeyA(HKEY_LOCAL_MACHINE, szCryptography, &hkey);
        win2k++;
    }
    RegQueryValueExA(hkey, szMachineGuid, NULL, NULL, (LPBYTE)guid, &size);
    RegCloseKey(hkey);

    if (!unique) return;
    lstrcpyA(unique, szContainer_md5);
    lstrcatA(unique, "_");
    lstrcatA(unique, guid);
}

static void printBytes(const char *heading, const BYTE *pb, size_t cb)
{
    size_t i;
    printf("%s: ",heading);
    for(i=0;i<cb;i++)
        printf("0x%02x,",pb[i]);
    putchar('\n');
}

static BOOL (WINAPI *pCryptDuplicateHash) (HCRYPTHASH, DWORD*, DWORD, HCRYPTHASH*);

/*
static void trace_hex(BYTE *pbData, DWORD dwLen) {
    char szTemp[256];
    DWORD i, j;

    for (i = 0; i < dwLen-7; i+=8) {
        sprintf(szTemp, "0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x,\n", 
            pbData[i], pbData[i+1], pbData[i+2], pbData[i+3], pbData[i+4], pbData[i+5], 
            pbData[i+6], pbData[i+7]);
        trace(szTemp);
    }
    for (j=0; i<dwLen; j++,i++) {
        sprintf(szTemp+6*j, "0x%02x,\n", pbData[i]);
    }
    trace(szTemp);
}
*/

static BOOL init_base_environment(const char *provider, DWORD dwKeyFlags)
{
    HCRYPTKEY hKey;
    BOOL result;

    if (provider) szProvider = provider;
        
    pCryptDuplicateHash = (void *)GetProcAddress(GetModuleHandleA("advapi32.dll"), "CryptDuplicateHash");
        
    hProv = (HCRYPTPROV)INVALID_HANDLE_VALUE;

    result = CryptAcquireContextA(&hProv, szContainer, szProvider, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    ok(!result && (GetLastError()==NTE_BAD_FLAGS ||
       broken(GetLastError() == NTE_KEYSET_NOT_DEF /* Win9x/NT4 */)),
       "%d, %08x\n", result, GetLastError());
    
    if (!CryptAcquireContextA(&hProv, szContainer, szProvider, PROV_RSA_FULL, 0))
    {
        ok(GetLastError()==NTE_BAD_KEYSET ||
           broken(GetLastError() == NTE_TEMPORARY_PROFILE /* some Win7 setups */) ||
           broken(GetLastError() == NTE_KEYSET_NOT_DEF /* Win9x/NT4 */),
           "%08x\n", GetLastError());
        if (GetLastError()!=NTE_BAD_KEYSET)
        {
            win_skip("RSA full provider not available\n");
            return FALSE;
        }
        result = CryptAcquireContextA(&hProv, szContainer, szProvider, PROV_RSA_FULL,
                                     CRYPT_NEWKEYSET);
        ok(result, "%08x\n", GetLastError());
        if (!result)
        {
            win_skip("Couldn't create crypto provider\n");
            return FALSE;
        }
        result = CryptGenKey(hProv, AT_KEYEXCHANGE, dwKeyFlags, &hKey);
        ok(result, "%08x\n", GetLastError());
        if (result) CryptDestroyKey(hKey);
        result = CryptGenKey(hProv, AT_SIGNATURE, dwKeyFlags, &hKey);
        ok(result, "%08x\n", GetLastError());
        if (result) CryptDestroyKey(hKey);
    }
    return TRUE;
}

static void clean_up_base_environment(void)
{
    BOOL result;

    SetLastError(0xdeadbeef);
    result = CryptReleaseContext(hProv, 1);
    ok(!result || broken(result) /* Win98 */, "Expected failure\n");
    ok(GetLastError()==NTE_BAD_FLAGS, "Expected NTE_BAD_FLAGS, got %08x\n", GetLastError());
        
    /* Just to prove that Win98 also released the CSP */
    SetLastError(0xdeadbeef);
    result = CryptReleaseContext(hProv, 0);
    ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "%08x\n", GetLastError());

    CryptAcquireContextA(&hProv, szContainer, szProvider, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
}

static BOOL init_aes_environment(void)
{
    HCRYPTKEY hKey;
    BOOL result;

    pCryptDuplicateHash = (void *)GetProcAddress(GetModuleHandleA("advapi32.dll"), "CryptDuplicateHash");

    hProv = (HCRYPTPROV)INVALID_HANDLE_VALUE;

    /* we are using NULL as provider name for RSA_AES provider as the provider
     * names are different in Windows XP and Vista. This differs from what
     * is defined in the SDK on Windows XP.
     * This provider is available on Windows XP, Windows 2003 and Vista.      */

    result = CryptAcquireContextA(&hProv, szContainer, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
    if (!result && GetLastError() == NTE_PROV_TYPE_NOT_DEF)
    {
        win_skip("RSA_AES provider not supported\n");
        return FALSE;
    }
    ok(!result && GetLastError()==NTE_BAD_FLAGS, "%d, %08x\n", result, GetLastError());

    if (!CryptAcquireContextA(&hProv, szContainer, NULL, PROV_RSA_AES, 0))
    {
        ok(GetLastError()==NTE_BAD_KEYSET, "%08x\n", GetLastError());
        if (GetLastError()!=NTE_BAD_KEYSET) return FALSE;
        result = CryptAcquireContextA(&hProv, szContainer, NULL, PROV_RSA_AES,
                                     CRYPT_NEWKEYSET);
        ok(result, "%08x\n", GetLastError());
        if (!result) return FALSE;
        result = CryptGenKey(hProv, AT_KEYEXCHANGE, 0, &hKey);
        ok(result, "%08x\n", GetLastError());
        if (result) CryptDestroyKey(hKey);
        result = CryptGenKey(hProv, AT_SIGNATURE, 0, &hKey);
        ok(result, "%08x\n", GetLastError());
        if (result) CryptDestroyKey(hKey);
    }
    return TRUE;
}

static void clean_up_aes_environment(void)
{
    BOOL result;

    result = CryptReleaseContext(hProv, 1);
    ok(!result && GetLastError()==NTE_BAD_FLAGS, "%08x\n", GetLastError());

    CryptAcquireContextA(&hProv, szContainer, NULL, PROV_RSA_AES, CRYPT_DELETEKEYSET);
}

static void test_prov(void) 
{
    BOOL result;
    DWORD dwLen, dwInc;
    
    dwLen = (DWORD)sizeof(DWORD);
    SetLastError(0xdeadbeef);
    result = CryptGetProvParam(hProv, PP_SIG_KEYSIZE_INC, (BYTE*)&dwInc, &dwLen, 0);
    if (!result && GetLastError() == NTE_BAD_TYPE)
    {
        skip("PP_SIG_KEYSIZE_INC is not supported (win9x or NT)\n");
        nt4++;
    }
    else
        ok(result && dwInc==8, "%08x, %d\n", GetLastError(), dwInc);
    
    dwLen = (DWORD)sizeof(DWORD);
    SetLastError(0xdeadbeef);
    result = CryptGetProvParam(hProv, PP_KEYX_KEYSIZE_INC, (BYTE*)&dwInc, &dwLen, 0);
    if (!result && GetLastError() == NTE_BAD_TYPE)
        skip("PP_KEYX_KEYSIZE_INC is not supported (win9x or NT)\n");
    else
        ok(result && dwInc==8, "%08x, %d\n", GetLastError(), dwInc);
}

static void test_gen_random(void)
{
    BOOL result;
    BYTE rnd1[16], rnd2[16];

    memset(rnd1, 0, sizeof(rnd1));
    memset(rnd2, 0, sizeof(rnd2));

    result = CryptGenRandom(hProv, sizeof(rnd1), rnd1);
    if (!result && GetLastError() == NTE_FAIL) {
        /* rsaenh compiled without OpenSSL */
        return;
    }
    
    ok(result, "%08x\n", GetLastError());

    result = CryptGenRandom(hProv, sizeof(rnd2), rnd2);
    ok(result, "%08x\n", GetLastError());

    ok(memcmp(rnd1, rnd2, sizeof(rnd1)), "CryptGenRandom generates non random data\n");
}

static BOOL derive_key(ALG_ID aiAlgid, HCRYPTKEY *phKey, DWORD len) 
{
    HCRYPTHASH hHash;
    BOOL result;
    unsigned char pbData[2000];
    int i;

    *phKey = 0;
    for (i=0; i<2000; i++) pbData[i] = (unsigned char)i;
    result = CryptCreateHash(hProv, CALG_MD2, 0, 0, &hHash);
    if (!result) {
        /* rsaenh compiled without OpenSSL */
        ok(GetLastError()==NTE_BAD_ALGID, "%08x\n", GetLastError());
        return FALSE;
    } 
    ok(result, "%08x\n", GetLastError());
    result = CryptHashData(hHash, pbData, sizeof(pbData), 0);
    ok(result, "%08x\n", GetLastError());
    if (!result) return FALSE;
    result = CryptDeriveKey(hProv, aiAlgid, hHash, (len << 16) | CRYPT_EXPORTABLE, phKey);
    ok(result, "%08x\n", GetLastError());
    if (!result) return FALSE;
    len = 2000;
    result = CryptGetHashParam(hHash, HP_HASHVAL, pbData, &len, 0);
    ok(result, "%08x\n", GetLastError());
    CryptDestroyHash(hHash);
    return TRUE;
}

static BYTE abPlainPrivateKey[596] = {
    0x07, 0x02, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00,
    0x52, 0x53, 0x41, 0x32, 0x00, 0x04, 0x00, 0x00,
    0x01, 0x00, 0x01, 0x00, 0x9b, 0x64, 0xef, 0xce,
    0x31, 0x7c, 0xad, 0x56, 0xe2, 0x1e, 0x9b, 0x96,
    0xb3, 0xf0, 0x29, 0x88, 0x6e, 0xa8, 0xc2, 0x11,
    0x33, 0xd6, 0xcc, 0x8c, 0x69, 0xb2, 0x1a, 0xfd,
    0xfc, 0x23, 0x21, 0x30, 0x4d, 0x29, 0x45, 0xb6,
    0x3a, 0x67, 0x11, 0x80, 0x1a, 0x91, 0xf2, 0x9f,
    0x01, 0xac, 0xc0, 0x11, 0x50, 0x5f, 0xcd, 0xb9,
    0xad, 0x76, 0x9f, 0x6e, 0x91, 0x55, 0x71, 0xda,
    0x97, 0x96, 0x96, 0x22, 0x75, 0xb4, 0x83, 0x44,
    0x89, 0x9e, 0xf8, 0x44, 0x40, 0x7c, 0xd6, 0xcd,
    0x9d, 0x88, 0xd6, 0x88, 0xbc, 0x56, 0xb7, 0x64,
    0xe9, 0x2c, 0x24, 0x2f, 0x0d, 0x78, 0x55, 0x1c,
    0xb2, 0x67, 0xb1, 0x5e, 0xbc, 0x0c, 0xcf, 0x1c,
    0xe9, 0xd3, 0x9e, 0xa2, 0x15, 0x24, 0x73, 0xd6,
    0xdb, 0x6f, 0x83, 0xb2, 0xf8, 0xbc, 0xe7, 0x47,
    0x3b, 0x01, 0xef, 0x49, 0x08, 0x98, 0xd6, 0xa3,
    0xf9, 0x25, 0x57, 0xe9, 0x39, 0x3c, 0x53, 0x30,
    0x1b, 0xf2, 0xc9, 0x62, 0x31, 0x43, 0x5d, 0x84,
    0x24, 0x30, 0x21, 0x9a, 0xad, 0xdb, 0x62, 0x91,
    0xc8, 0x07, 0xd9, 0x2f, 0xd6, 0xb5, 0x37, 0x6f,
    0xfe, 0x7a, 0x12, 0xbc, 0xd9, 0xd2, 0x2b, 0xbf,
    0xd7, 0xb1, 0xfa, 0x7d, 0xc0, 0x48, 0xdd, 0x74,
    0xdd, 0x55, 0x04, 0xa1, 0x8b, 0xc1, 0x0a, 0xc4,
    0xa5, 0x57, 0x62, 0xee, 0x08, 0x8b, 0xf9, 0x19,
    0x6c, 0x52, 0x06, 0xf8, 0x73, 0x0f, 0x24, 0xc9,
    0x71, 0x9f, 0xc5, 0x45, 0x17, 0x3e, 0xae, 0x06,
    0x81, 0xa2, 0x96, 0x40, 0x06, 0xbf, 0xeb, 0x9e,
    0x80, 0x2b, 0x27, 0x20, 0x8f, 0x38, 0xcf, 0xeb,
    0xff, 0x3b, 0x38, 0x41, 0x35, 0x69, 0x66, 0x13,
    0x1d, 0x3c, 0x01, 0x3b, 0xf6, 0x37, 0xca, 0x9c,
    0x61, 0x74, 0x98, 0xcf, 0xc9, 0x6e, 0xe8, 0x90,
    0xc7, 0xb7, 0x33, 0xc0, 0x07, 0x3c, 0xf8, 0xc8,
    0xf6, 0xf2, 0xd7, 0xf0, 0x21, 0x62, 0x58, 0x8a,
    0x55, 0xbf, 0xa1, 0x2d, 0x3d, 0xa6, 0x69, 0xc5,
    0x02, 0x19, 0x31, 0xf0, 0x94, 0x0f, 0x45, 0x5c,
    0x95, 0x1b, 0x53, 0xbc, 0xf5, 0xb0, 0x1a, 0x8f,
    0xbf, 0x40, 0xe0, 0xc7, 0x73, 0xe7, 0x72, 0x6e,
    0xeb, 0xb1, 0x0f, 0x38, 0xc5, 0xf8, 0xee, 0x04,
    0xed, 0x34, 0x1a, 0x10, 0xf9, 0x53, 0x34, 0xf3,
    0x3e, 0xe6, 0x5c, 0xd1, 0x47, 0x65, 0xcd, 0xbd,
    0xf1, 0x06, 0xcb, 0xb4, 0xb1, 0x26, 0x39, 0x9f,
    0x71, 0xfe, 0x3d, 0xf8, 0x62, 0xab, 0x22, 0x8b,
    0x0e, 0xdc, 0xb9, 0xe8, 0x74, 0x06, 0xfc, 0x8c,
    0x25, 0xa1, 0xa9, 0xcf, 0x07, 0xf9, 0xac, 0x21,
    0x01, 0x7b, 0x1c, 0xdc, 0x94, 0xbd, 0x47, 0xe1,
    0xa0, 0x86, 0x59, 0x35, 0x6a, 0x6f, 0xb9, 0x70,
    0x26, 0x7c, 0x3c, 0xfd, 0xbd, 0x81, 0x39, 0x36,
    0x42, 0xc2, 0xbd, 0xbe, 0x84, 0x27, 0x9a, 0x69,
    0x81, 0xda, 0x99, 0x27, 0xc2, 0x4f, 0x62, 0x33,
    0xf4, 0x79, 0x30, 0xc5, 0x63, 0x54, 0x71, 0xf1,
    0x47, 0x22, 0x25, 0x9b, 0x6c, 0x00, 0x2f, 0x1c,
    0xf4, 0x1f, 0x85, 0xbc, 0xf6, 0x67, 0x6a, 0xe3,
    0xf6, 0x55, 0x8a, 0xef, 0xd0, 0x0b, 0xd3, 0xa2,
    0xc5, 0x51, 0x70, 0x15, 0x0a, 0xf0, 0x98, 0x4c,
    0xb7, 0x19, 0x62, 0x0e, 0x2d, 0x2a, 0x4a, 0x7d,
    0x7a, 0x0a, 0xc4, 0x17, 0xe3, 0x5d, 0x20, 0x52,
    0xa9, 0x98, 0xc3, 0xaa, 0x11, 0xf6, 0xbf, 0x4c,
    0x94, 0x99, 0x81, 0x89, 0xf0, 0x7f, 0x66, 0xaa,
    0xc8, 0x88, 0xd7, 0x31, 0x84, 0x71, 0xb6, 0x64,
    0x09, 0x76, 0x0b, 0x7f, 0x1a, 0x1f, 0x2e, 0xfe,
    0xcd, 0x59, 0x2a, 0x54, 0x11, 0x84, 0xd4, 0x6a,
    0x61, 0xdf, 0xaa, 0x76, 0x66, 0x9d, 0x82, 0x11,
    0x56, 0x3d, 0xd2, 0x52, 0xe6, 0x42, 0x5a, 0x77,
    0x92, 0x98, 0x34, 0xf3, 0x56, 0x6c, 0x96, 0x10,
    0x40, 0x59, 0x16, 0xcb, 0x77, 0x61, 0xe3, 0xbf,
    0x4b, 0xd4, 0x39, 0xfb, 0xb1, 0x4e, 0xc1, 0x74,
    0xec, 0x7a, 0xea, 0x3d, 0x68, 0xbb, 0x0b, 0xe6,
    0xc6, 0x06, 0xbf, 0xdd, 0x7f, 0x94, 0x42, 0xc0,
    0x0f, 0xe4, 0x92, 0x33, 0x6c, 0x6e, 0x1b, 0xba,
    0x73, 0xf9, 0x79, 0x84, 0xdf, 0x45, 0x00, 0xe4,
    0x94, 0x88, 0x9d, 0x08, 0x89, 0xcf, 0xf2, 0xa4,
    0xc5, 0x47, 0x45, 0x85, 0x86, 0xa5, 0xcc, 0xa8,
    0xf2, 0x5d, 0x58, 0x07
};

static void test_hashes(void)
{
    static const unsigned char md2hash[16] = {
        0x12, 0xcb, 0x1b, 0x08, 0xc8, 0x48, 0xa4, 0xa9, 
        0xaa, 0xf3, 0xf1, 0x9f, 0xfc, 0x29, 0x28, 0x68 };
    static const unsigned char md4hash[16] = {
        0x8e, 0x2a, 0x58, 0xbf, 0xf2, 0xf5, 0x26, 0x23, 
        0x79, 0xd2, 0x92, 0x36, 0x1b, 0x23, 0xe3, 0x81 };
    static const unsigned char empty_md5hash[16] = {
        0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
        0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e };
    static const unsigned char md5hash[16] = { 
        0x15, 0x76, 0xa9, 0x4d, 0x6c, 0xb3, 0x34, 0xdd, 
        0x12, 0x6c, 0xb1, 0xc2, 0x7f, 0x19, 0xe0, 0xf2 };    
    static const unsigned char sha1hash[20] = { 
        0xf1, 0x0c, 0xcf, 0xde, 0x60, 0xc1, 0x7d, 0xb2, 0x6e, 0x7d, 
        0x85, 0xd3, 0x56, 0x65, 0xc7, 0x66, 0x1d, 0xbb, 0xeb, 0x2c };
    static const unsigned char signed_ssl3_shamd5_hash[] = {
        0x4f,0xcc,0x2f,0x33,0x44,0x60,0x76,0x16,0x13,0xc8,0xff,0xd4,0x59,0x19,
        0xde,0x85,0x44,0x72,0x47,0x98,0x01,0xfb,0x67,0x5c,0x5b,0x35,0x15,0x0f,
        0x91,0xda,0xc7,0x7c,0xfb,0xe2,0x18,0xef,0xac,0x31,0x40,0x7b,0xa9,0x83,
        0xdb,0x30,0xcd,0x94,0x4b,0x8e,0x3b,0x6c,0x7a,0x86,0x59,0xf0,0xd1,0xd2,
        0x5e,0xce,0xd4,0x1b,0x7f,0xed,0x24,0xee,0x53,0x5c,0x15,0x97,0x21,0x7c,
        0x5c,0xea,0xab,0xf5,0xd6,0x4b,0xb3,0xbb,0x14,0xf5,0x59,0x9e,0x21,0x90,
        0x21,0x99,0x19,0xad,0xa2,0xa6,0xea,0x61,0xc1,0x41,0xe2,0x70,0x77,0xf7,
        0x15,0x68,0x96,0x1e,0x5c,0x84,0x97,0xe3,0x5c,0xd2,0xd9,0xfb,0x87,0x6f,
        0x11,0x21,0x82,0x43,0x76,0x32,0xa4,0x38,0x7b,0x85,0x22,0x30,0x1e,0x55,
        0x79,0x93 };
    unsigned char pbData[2048];
    BOOL result;
    HCRYPTHASH hHash, hHashClone;
    HCRYPTPROV prov;
    BYTE pbHashValue[36];
    BYTE pbSigValue[128];
    HCRYPTKEY hKeyExchangeKey;
    DWORD hashlen, len, error, cryptflags;
    int i;

    for (i=0; i<2048; i++) pbData[i] = (unsigned char)i;

    /* MD2 Hashing */
    result = CryptCreateHash(hProv, CALG_MD2, 0, 0, &hHash);
    if (!result) {
        /* rsaenh compiled without OpenSSL */
        ok(GetLastError() == NTE_BAD_ALGID, "%08x\n", GetLastError());
    } else {
        result = CryptHashData(hHash, pbData, sizeof(pbData), 0);
        ok(result, "%08x\n", GetLastError());

        len = sizeof(DWORD);
        result = CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashlen, &len, 0);
           ok(result && (hashlen == 16), "%08x, hashlen: %d\n", GetLastError(), hashlen);

        len = 16;
        result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &len, 0);
        ok(result, "%08x\n", GetLastError());

        ok(!memcmp(pbHashValue, md2hash, 16), "Wrong MD2 hash!\n");

        result = CryptDestroyHash(hHash);
        ok(result, "%08x\n", GetLastError());
    } 

    /* MD4 Hashing */
    result = CryptCreateHash(hProv, CALG_MD4, 0, 0, &hHash);
    ok(result, "%08x\n", GetLastError());

    result = CryptHashData(hHash, pbData, sizeof(pbData), ~0);
    ok(!result && GetLastError() == NTE_BAD_FLAGS, "%08x\n", GetLastError());

    cryptflags = CRYPT_USERDATA;
    result = CryptHashData(hHash, pbData, sizeof(pbData), cryptflags);
    if (!result && GetLastError() == NTE_BAD_FLAGS) /* <= NT4 */
    {
        cryptflags &= ~CRYPT_USERDATA;
        ok(broken(1), "Failed to support CRYPT_USERDATA flag\n");
        result = CryptHashData(hHash, pbData, sizeof(pbData), 0);
    }
    ok(result, "%08x\n", GetLastError());

    len = sizeof(DWORD);
    result = CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashlen, &len, 0);
    ok(result && (hashlen == 16), "%08x, hashlen: %d\n", GetLastError(), hashlen);

    len = 16;
    result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &len, 0);
    ok(result, "%08x\n", GetLastError());

    ok(!memcmp(pbHashValue, md4hash, 16), "Wrong MD4 hash!\n");

    result = CryptDestroyHash(hHash);
    ok(result, "%08x\n", GetLastError());

    /* MD5 Hashing */
    result = CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
    ok(result, "%08x\n", GetLastError());

    len = sizeof(DWORD);
    result = CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashlen, &len, 0);
    ok(result && (hashlen == 16), "%08x, hashlen: %d\n", GetLastError(), hashlen);

    result = CryptHashData(hHash, pbData, sizeof(pbData), ~0);
    ok(!result && GetLastError() == NTE_BAD_FLAGS, "%08x\n", GetLastError());

    result = CryptHashData(hHash, pbData, sizeof(pbData), cryptflags);
    ok(result, "%08x\n", GetLastError());

    len = 16;
    result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &len, 0);
    ok(result, "%08x\n", GetLastError());

    ok(!memcmp(pbHashValue, md5hash, 16), "Wrong MD5 hash!\n");

    result = CryptDestroyHash(hHash);
    ok(result, "%08x\n", GetLastError());

    result = CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
    ok(result, "%08x\n", GetLastError());

    /* The hash is available even if CryptHashData hasn't been called */
    len = 16;
    result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &len, 0);
    ok(result, "%08x\n", GetLastError());

    ok(!memcmp(pbHashValue, empty_md5hash, 16), "Wrong MD5 hash!\n");

    /* It's also stable:  getting it twice results in the same value */
    result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &len, 0);
    ok(result, "%08x\n", GetLastError());

    ok(!memcmp(pbHashValue, empty_md5hash, 16), "Wrong MD5 hash!\n");

    /* Can't add data after the hash been retrieved */
    SetLastError(0xdeadbeef);
    result = CryptHashData(hHash, pbData, sizeof(pbData), 0);
    ok(!result, "Expected failure\n");
    ok(GetLastError() == NTE_BAD_HASH_STATE ||
       GetLastError() == NTE_BAD_ALGID, /* Win9x, WinMe, NT4 */
       "Expected NTE_BAD_HASH_STATE or NTE_BAD_ALGID, got %08x\n", GetLastError());

    /* You can still retrieve the hash, its value just hasn't changed */
    result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &len, 0);
    ok(result, "%08x\n", GetLastError());

    ok(!memcmp(pbHashValue, empty_md5hash, 16), "Wrong MD5 hash!\n");

    result = CryptDestroyHash(hHash);
    ok(result, "%08x\n", GetLastError());

    /* SHA1 Hashing */
    result = CryptCreateHash(hProv, CALG_SHA, 0, 0, &hHash);
    ok(result, "%08x\n", GetLastError());

    result = CryptHashData(hHash, pbData, 5, cryptflags);
    ok(result, "%08x\n", GetLastError());

    if(pCryptDuplicateHash) {
        result = pCryptDuplicateHash(hHash, 0, 0, &hHashClone);
        ok(result, "%08x\n", GetLastError());

        result = CryptHashData(hHashClone, (BYTE*)pbData+5, sizeof(pbData)-5, 0);
        ok(result, "%08x\n", GetLastError());

        len = sizeof(DWORD);
        result = CryptGetHashParam(hHashClone, HP_HASHSIZE, (BYTE*)&hashlen, &len, 0);
        ok(result && (hashlen == 20), "%08x, hashlen: %d\n", GetLastError(), hashlen);

        len = 20;
        result = CryptGetHashParam(hHashClone, HP_HASHVAL, pbHashValue, &len, 0);
        ok(result, "%08x\n", GetLastError());

        ok(!memcmp(pbHashValue, sha1hash, 20), "Wrong SHA1 hash!\n");

        result = CryptDestroyHash(hHashClone);
        ok(result, "%08x\n", GetLastError());
    }

    result = CryptDestroyHash(hHash);
    ok(result, "%08x\n", GetLastError());

    /* The SHA-2 variants aren't supported in the RSA full provider */
    result = CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash);
    ok(!result && GetLastError() == NTE_BAD_ALGID,
       "expected NTE_BAD_ALGID, got %08x\n", GetLastError());
    result = CryptCreateHash(hProv, CALG_SHA_384, 0, 0, &hHash);
    ok(!result && GetLastError() == NTE_BAD_ALGID,
       "expected NTE_BAD_ALGID, got %08x\n", GetLastError());
    result = CryptCreateHash(hProv, CALG_SHA_512, 0, 0, &hHash);
    ok(!result && GetLastError() == NTE_BAD_ALGID,
       "expected NTE_BAD_ALGID, got %08x\n", GetLastError());

    result = CryptAcquireContextA(&prov, NULL, szProvider, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    ok(result, "CryptAcquireContextA failed 0x%08x\n", GetLastError());

    result = CryptCreateHash(prov, CALG_SHA1, 0, 0, &hHash);
    ok(result, "CryptCreateHash failed 0x%08x\n", GetLastError());

    /* release provider before using the hash */
    result = CryptReleaseContext(prov, 0);
    ok(result, "CryptReleaseContext failed 0x%08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    result = CryptHashData(hHash, (const BYTE *)"data", sizeof("data"), 0);
    error = GetLastError();
    ok(!result, "CryptHashData succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", error);

    SetLastError(0xdeadbeef);
    result = CryptDestroyHash(hHash);
    error = GetLastError();
    ok(!result, "CryptDestroyHash succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", error);

    if (!pCryptDuplicateHash)
    {
        win_skip("CryptDuplicateHash is not available\n");
        return;
    }

    result = CryptAcquireContextA(&prov, NULL, szProvider, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    ok(result, "CryptAcquireContextA failed 0x%08x\n", GetLastError());

    result = CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);
    ok(result, "CryptCreateHash failed 0x%08x\n", GetLastError());

    result = CryptHashData(hHash, (const BYTE *)"data", sizeof("data"), 0);
    ok(result, "CryptHashData failed 0x%08x\n", GetLastError());

    result = pCryptDuplicateHash(hHash, NULL, 0, &hHashClone);
    ok(result, "CryptDuplicateHash failed 0x%08x\n", GetLastError());

    len = 20;
    result = CryptGetHashParam(hHashClone, HP_HASHVAL, pbHashValue, &len, 0);
    ok(result, "CryptGetHashParam failed 0x%08x\n", GetLastError());

    /* add data after duplicating the hash */
    result = CryptHashData(hHash, (const BYTE *)"more data", sizeof("more data"), 0);
    ok(result, "CryptHashData failed 0x%08x\n", GetLastError());

    result = CryptDestroyHash(hHash);
    ok(result, "CryptDestroyHash failed 0x%08x\n", GetLastError());

    result = CryptDestroyHash(hHashClone);
    ok(result, "CryptDestroyHash failed 0x%08x\n", GetLastError());

    result = CryptReleaseContext(prov, 0);
    ok(result, "CryptReleaseContext failed 0x%08x\n", GetLastError());

    /* Test CALG_SSL3_SHAMD5 */
    result = CryptAcquireContextA(&prov, NULL, szProvider, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    ok(result, "CryptAcquireContextA failed 0x%08x\n", GetLastError());

    /* Step 1: create an MD5 hash of the data */
    result = CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
    ok(result, "CryptCreateHash failed 0x%08x\n", GetLastError());
    result = CryptHashData(hHash, pbData, sizeof(pbData), 0);
    ok(result, "%08x\n", GetLastError());
    len = 16;
    result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &len, 0);
    ok(result, "CryptGetHashParam failed 0x%08x\n", GetLastError());
    result = CryptDestroyHash(hHash);
    ok(result, "CryptDestroyHash failed 0x%08x\n", GetLastError());
    /* Step 2: create a SHA1 hash of the data */
    result = CryptCreateHash(hProv, CALG_SHA, 0, 0, &hHash);
    ok(result, "CryptCreateHash failed 0x%08x\n", GetLastError());
    result = CryptHashData(hHash, pbData, sizeof(pbData), 0);
    ok(result, "%08x\n", GetLastError());
    len = 20;
    result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue + 16, &len, 0);
    ok(result, "CryptGetHashParam failed 0x%08x\n", GetLastError());
    result = CryptDestroyHash(hHash);
    ok(result, "CryptDestroyHash failed 0x%08x\n", GetLastError());
    /* Step 3: create a CALG_SSL3_SHAMD5 hash handle */
    result = CryptCreateHash(hProv, CALG_SSL3_SHAMD5, 0, 0, &hHash);
    ok(result, "CryptCreateHash failed 0x%08x\n", GetLastError());
    /* Test that CryptHashData fails on this hash */
    result = CryptHashData(hHash, pbData, sizeof(pbData), 0);
    ok(!result && (GetLastError() == NTE_BAD_ALGID || broken(GetLastError() == ERROR_INVALID_HANDLE)) /* Win 8 */,
       "%08x\n", GetLastError());
    result = CryptSetHashParam(hHash, HP_HASHVAL, pbHashValue, 0);
    ok(result, "%08x\n", GetLastError());
    len = (DWORD)sizeof(abPlainPrivateKey);
    result = CryptImportKey(hProv, abPlainPrivateKey, len, 0, 0, &hKeyExchangeKey);
    ok(result, "%08x\n", GetLastError());
    len = 0;
    result = CryptSignHashA(hHash, AT_KEYEXCHANGE, NULL, 0, NULL, &len);
    ok(result, "%08x\n", GetLastError());
    ok(len == 128, "expected len 128, got %d\n", len);
    result = CryptSignHashA(hHash, AT_KEYEXCHANGE, NULL, 0, pbSigValue, &len);
    ok(result, "%08x\n", GetLastError());
    ok(!memcmp(pbSigValue, signed_ssl3_shamd5_hash, len), "unexpected value\n");
    if (len != 128 || memcmp(pbSigValue, signed_ssl3_shamd5_hash, len))
    {
        printBytes("expected", signed_ssl3_shamd5_hash,
                   sizeof(signed_ssl3_shamd5_hash));
        printBytes("got", pbSigValue, len);
    }
    result = CryptDestroyKey(hKeyExchangeKey);
    ok(result, "CryptDestroyKey failed 0x%08x\n", GetLastError());
    result = CryptDestroyHash(hHash);
    ok(result, "CryptDestroyHash failed 0x%08x\n", GetLastError());
    result = CryptReleaseContext(prov, 0);
    ok(result, "CryptReleaseContext failed 0x%08x\n", GetLastError());
}

static void test_block_cipher_modes(void)
{
    static const BYTE plain[23] = { 
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 
        0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16 };
    static const BYTE ecb[24] = {   
        0xc0, 0x9a, 0xe4, 0x2f, 0x0a, 0x47, 0x67, 0x11, 0xf2, 0xb2, 0x5d, 0x5f, 
        0x08, 0xff, 0x49, 0xa4, 0x45, 0x3a, 0x68, 0x14, 0xca, 0x18, 0xe5, 0xf4 };
    static const BYTE cbc[24] = {   
        0xc0, 0x9a, 0xe4, 0x2f, 0x0a, 0x47, 0x67, 0x11, 0x10, 0xf5, 0xda, 0x61,
        0x4e, 0x3d, 0xab, 0xc0, 0x97, 0x85, 0x01, 0x12, 0x97, 0xa4, 0xf7, 0xd3 };
    static const BYTE cfb[24] = {   
        0x29, 0xb5, 0x67, 0x85, 0x0b, 0x1b, 0xec, 0x07, 0x67, 0x2d, 0xa1, 0xa4,
        0x1a, 0x47, 0x24, 0x6a, 0x54, 0xe1, 0xe0, 0x92, 0xf9, 0x0e, 0xf6, 0xeb };
    HCRYPTKEY hKey;
    BOOL result;
    BYTE abData[24];
    DWORD dwMode, dwLen;

    result = derive_key(CALG_RC2, &hKey, 40);
    if (!result) return;

    memcpy(abData, plain, sizeof(plain));

    /* test default chaining mode */
    dwMode = 0xdeadbeef;
    dwLen = sizeof(dwMode);
    result = CryptGetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, &dwLen, 0);
    ok(result, "%08x\n", GetLastError());
    ok(dwMode == CRYPT_MODE_CBC, "Wrong default chaining mode\n");

    dwMode = CRYPT_MODE_ECB;
    result = CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
    ok(result, "%08x\n", GetLastError());

    result = CryptGetKeyParam(hKey, KP_SALT, NULL, &dwLen, 0);
    ok(result, "%08x\n", GetLastError());
    ok(dwLen == 11 || broken(dwLen == 0 /* Win9x/NT4 */), "unexpected salt length %d\n", dwLen);

    dwLen = 23;
    result = CryptEncrypt(hKey, 0, TRUE, 0, NULL, &dwLen, 24);
    ok(result, "CryptEncrypt failed: %08x\n", GetLastError());
    ok(dwLen == 24, "Unexpected length %d\n", dwLen);

    SetLastError(ERROR_SUCCESS);
    dwLen = 23;
    result = CryptEncrypt(hKey, 0, TRUE, 0, abData, &dwLen, 24);
    ok(result && dwLen == 24 && !memcmp(ecb, abData, sizeof(ecb)), 
       "%08x, dwLen: %d\n", GetLastError(), dwLen);

    result = CryptDecrypt(hKey, 0, TRUE, 0, abData, &dwLen);
    ok(result && dwLen == 23 && !memcmp(plain, abData, sizeof(plain)), 
       "%08x, dwLen: %d\n", GetLastError(), dwLen);

    dwMode = CRYPT_MODE_CBC;
    result = CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
    ok(result, "%08x\n", GetLastError());
    
    dwLen = 23;
    result = CryptEncrypt(hKey, 0, TRUE, 0, NULL, &dwLen, 24);
    ok(result, "CryptEncrypt failed: %08x\n", GetLastError());
    ok(dwLen == 24, "Unexpected length %d\n", dwLen);

    dwLen = 23;
    result = CryptEncrypt(hKey, 0, TRUE, 0, abData, &dwLen, 24);
    ok(result && dwLen == 24 && !memcmp(cbc, abData, sizeof(cbc)), 
       "%08x, dwLen: %d\n", GetLastError(), dwLen);

    result = CryptDecrypt(hKey, 0, TRUE, 0, abData, &dwLen);
    ok(result && dwLen == 23 && !memcmp(plain, abData, sizeof(plain)), 
       "%08x, dwLen: %d\n", GetLastError(), dwLen);

    dwMode = CRYPT_MODE_CFB;
    result = CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
    ok(result, "%08x\n", GetLastError());
    
    dwLen = 16;
    result = CryptEncrypt(hKey, 0, FALSE, 0, abData, &dwLen, 24);
    ok(result && dwLen == 16, "%08x, dwLen: %d\n", GetLastError(), dwLen);

    dwLen = 7;
    result = CryptEncrypt(hKey, 0, TRUE, 0, abData+16, &dwLen, 8);
    ok(result && dwLen == 8 && !memcmp(cfb, abData, sizeof(cfb)), 
       "%08x, dwLen: %d\n", GetLastError(), dwLen);
    
    dwLen = 8;
    result = CryptDecrypt(hKey, 0, FALSE, 0, abData, &dwLen);
    ok(result && dwLen == 8, "%08x, dwLen: %d\n", GetLastError(), dwLen);

    dwLen = 16;
    result = CryptDecrypt(hKey, 0, TRUE, 0, abData+8, &dwLen);
    ok(result && dwLen == 15 && !memcmp(plain, abData, sizeof(plain)), 
       "%08x, dwLen: %d\n", GetLastError(), dwLen);

    dwMode = CRYPT_MODE_OFB;
    result = CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
    if(!result && GetLastError() == ERROR_INTERNAL_ERROR)
    {
        ok(broken(1), "OFB mode not supported\n"); /* Windows 8 */
    }
    else
    {
        ok(result, "%08x\n", GetLastError());

        dwLen = 23;
        result = CryptEncrypt(hKey, 0, TRUE, 0, abData, &dwLen, 24);
        ok(!result && GetLastError() == NTE_BAD_ALGID, "%08x\n", GetLastError());
    }

    CryptDestroyKey(hKey);
}

static void test_3des112(void)
{
    HCRYPTKEY hKey;
    BOOL result;
    DWORD dwLen;
    unsigned char pbData[16], enc_data[16], bad_data[16];
    static const BYTE des112[16] = {
        0x8e, 0x0c, 0x3c, 0xa3, 0x05, 0x88, 0x5f, 0x7a,
        0x32, 0xa1, 0x06, 0x52, 0x64, 0xd2, 0x44, 0x1c };
    int i;

    result = derive_key(CALG_3DES_112, &hKey, 0);
    if (!result) {
        /* rsaenh compiled without OpenSSL */
        ok(GetLastError() == NTE_BAD_ALGID, "%08x\n", GetLastError());
        return;
    }

    for (i=0; i<sizeof(pbData); i++) pbData[i] = (unsigned char)i;
    
    dwLen = 13;
    result = CryptEncrypt(hKey, 0, TRUE, 0, pbData, &dwLen, 16);
    ok(result, "%08x\n", GetLastError());
    
    ok(!memcmp(pbData, des112, sizeof(des112)), "3DES_112 encryption failed!\n");

    result = CryptDecrypt(hKey, 0, TRUE, 0, pbData, &dwLen);
    ok(result, "%08x\n", GetLastError());

    for (i=0; i<4; i++)
    {
      memcpy(pbData,cTestData[i].origstr,cTestData[i].strlen);

      dwLen = cTestData[i].enclen;
      result = CryptEncrypt(hKey, 0, TRUE, 0, pbData, &dwLen, cTestData[i].buflen);
      ok(result, "%08x\n", GetLastError());
      ok(dwLen==cTestData[i].buflen,"length incorrect, got %d, expected %d\n",dwLen,cTestData[i].buflen);
      memcpy(enc_data, pbData, cTestData[i].buflen);

      result = CryptDecrypt(hKey, 0, TRUE, 0, pbData, &dwLen);
      ok(result, "%08x\n", GetLastError());
      ok(dwLen==cTestData[i].enclen,"length incorrect, got %d, expected %d\n",dwLen,cTestData[i].enclen);
      ok(memcmp(pbData,cTestData[i].decstr,cTestData[1].enclen)==0,"decryption incorrect %d\n",i);
      if((dwLen != cTestData[i].enclen) ||
         memcmp(pbData,cTestData[i].decstr,cTestData[i].enclen))
      {
          printBytes("expected",cTestData[i].decstr,cTestData[i].strlen);
          printBytes("got",pbData,dwLen);
      }

      /* Test bad data:
         Decrypting a block of bad data with Final = TRUE should restore the
         initial state of the key as well as decrypting a block of good data.
       */

      /* Changing key state by setting Final = FALSE */
      dwLen = cTestData[i].buflen;
      memcpy(pbData, enc_data, cTestData[i].buflen);
      result = CryptDecrypt(hKey, 0, FALSE, 0, pbData, &dwLen);
      ok(result, "%08x\n", GetLastError());

      /* Restoring key state by decrypting bad_data with Final = TRUE */
      memcpy(bad_data, enc_data, cTestData[i].buflen);
      bad_data[cTestData[i].buflen - 1] = ~bad_data[cTestData[i].buflen - 1];
      SetLastError(0xdeadbeef);
      result = CryptDecrypt(hKey, 0, TRUE, 0, bad_data, &dwLen);
      ok(!result, "CryptDecrypt should failed!\n");
      ok(GetLastError() == NTE_BAD_DATA, "%08x\n", GetLastError());
      ok(dwLen==cTestData[i].buflen,"length incorrect, got %d, expected %d\n",dwLen,cTestData[i].buflen);
      ok(memcmp(pbData,cTestData[i].decstr,cTestData[1].enclen)==0,"decryption incorrect %d\n",i);

      /* Checking key state */
      dwLen = cTestData[i].buflen;
      memcpy(pbData, enc_data, cTestData[i].buflen);
      result = CryptDecrypt(hKey, 0, TRUE, 0, pbData, &dwLen);
      ok(result, "%08x\n", GetLastError());
      ok(dwLen==cTestData[i].enclen,"length incorrect, got %d, expected %d\n",dwLen,cTestData[i].enclen);
      ok(memcmp(pbData,cTestData[i].decstr,cTestData[1].enclen)==0,"decryption incorrect %d\n",i);
      if((dwLen != cTestData[i].enclen) ||
         memcmp(pbData,cTestData[i].decstr,cTestData[i].enclen))
      {
          printBytes("expected",cTestData[i].decstr,cTestData[i].strlen);
          printBytes("got",pbData,dwLen);
      }
    }
    result = CryptDestroyKey(hKey);
    ok(result, "%08x\n", GetLastError());
}

static void test_des(void) 
{
    HCRYPTKEY hKey;
    BOOL result;
    DWORD dwLen, dwMode;
    unsigned char pbData[16], enc_data[16], bad_data[16];
    static const BYTE des[16] = {
        0x58, 0x86, 0x42, 0x46, 0x65, 0x4b, 0x92, 0x62,
        0xcf, 0x0f, 0x65, 0x37, 0x43, 0x7a, 0x82, 0xb9 };
    static const BYTE des_old_behavior[16] = {
        0xb0, 0xfd, 0x11, 0x69, 0x76, 0xb1, 0xa1, 0x03,
        0xf7, 0xbc, 0x23, 0xaa, 0xd4, 0xc1, 0xc9, 0x55 };
    static const BYTE des_old_strong[16] = {
        0x9b, 0xc1, 0x2a, 0xec, 0x4a, 0xf9, 0x0f, 0x14,
        0x0a, 0xed, 0xf6, 0xd3, 0xdc, 0xad, 0xf7, 0x0c };
    int i;

    result = derive_key(CALG_DES, &hKey, 0);
    if (!result) {
        /* rsaenh compiled without OpenSSL */
        ok(GetLastError()==NTE_BAD_ALGID, "%08x\n", GetLastError());
        return;
    }

    dwMode = CRYPT_MODE_ECB;
    result = CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
    ok(result, "%08x\n", GetLastError());
    
    dwLen = sizeof(DWORD);
    result = CryptGetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, &dwLen, 0);
    ok(result, "%08x\n", GetLastError());
    ok(dwMode == CRYPT_MODE_ECB, "Expected CRYPT_MODE_ECB, got %d\n", dwMode);
    
    for (i=0; i<sizeof(pbData); i++) pbData[i] = (unsigned char)i;
    
    dwLen = 13;
    result = CryptEncrypt(hKey, 0, TRUE, 0, pbData, &dwLen, 16);
    ok(result, "%08x\n", GetLastError());
    
    ok(!memcmp(pbData, des, sizeof(des)), "DES encryption failed!\n");

    result = CryptDecrypt(hKey, 0, TRUE, 0, pbData, &dwLen);
    ok(result, "%08x\n", GetLastError());

    for (i=0; i<4; i++)
    {
      memcpy(pbData,cTestData[i].origstr,cTestData[i].strlen);

      dwLen = cTestData[i].enclen;
      result = CryptEncrypt(hKey, 0, TRUE, 0, pbData, &dwLen, cTestData[i].buflen);
      ok(result, "%08x\n", GetLastError());
      ok(dwLen==cTestData[i].buflen,"length incorrect, got %d, expected %d\n",dwLen,cTestData[i].buflen);
      memcpy(enc_data, pbData, cTestData[i].buflen);

      result = CryptDecrypt(hKey, 0, TRUE, 0, pbData, &dwLen);
      ok(result, "%08x\n", GetLastError());
      ok(dwLen==cTestData[i].enclen,"length incorrect, got %d, expected %d\n",dwLen,cTestData[i].enclen);
      ok(memcmp(pbData,cTestData[i].decstr,cTestData[i].enclen)==0,"decryption incorrect %d\n",i);
      if((dwLen != cTestData[i].enclen) ||
         memcmp(pbData,cTestData[i].decstr,cTestData[i].enclen))
      {
          printBytes("expected",cTestData[i].decstr,cTestData[i].strlen);
          printBytes("got",pbData,dwLen);
      }

      /* Test bad data:
         Decrypting a block of bad data with Final = TRUE should restore the
         initial state of the key as well as decrypting a block of good data.
       */

      /* Changing key state by setting Final = FALSE */
      dwLen = cTestData[i].buflen;
      memcpy(pbData, enc_data, cTestData[i].buflen);
      result = CryptDecrypt(hKey, 0, FALSE, 0, pbData, &dwLen);
      ok(result, "%08x\n", GetLastError());

      /* Restoring key state by decrypting bad_data with Final = TRUE */
      memcpy(bad_data, enc_data, cTestData[i].buflen);
      bad_data[cTestData[i].buflen - 1] = ~bad_data[cTestData[i].buflen - 1];
      SetLastError(0xdeadbeef);
      result = CryptDecrypt(hKey, 0, TRUE, 0, bad_data, &dwLen);
      ok(!result, "CryptDecrypt should failed!\n");
      ok(GetLastError() == NTE_BAD_DATA, "%08x\n", GetLastError());
      ok(dwLen==cTestData[i].buflen,"length incorrect, got %d, expected %d\n",dwLen,cTestData[i].buflen);
      ok(memcmp(pbData,cTestData[i].decstr,cTestData[1].enclen)==0,"decryption incorrect %d\n",i);

      /* Checking key state */
      dwLen = cTestData[i].buflen;
      memcpy(pbData, enc_data, cTestData[i].buflen);
      result = CryptDecrypt(hKey, 0, TRUE, 0, pbData, &dwLen);
      ok(result, "%08x\n", GetLastError());
      ok(dwLen==cTestData[i].enclen,"length incorrect, got %d, expected %d\n",dwLen,cTestData[i].enclen);
      ok(memcmp(pbData,cTestData[i].decstr,cTestData[1].enclen)==0,"decryption incorrect %d\n",i);
      if((dwLen != cTestData[i].enclen) ||
         memcmp(pbData,cTestData[i].decstr,cTestData[i].enclen))
      {
          printBytes("expected",cTestData[i].decstr,cTestData[i].strlen);
          printBytes("got",pbData,dwLen);
      }
    }

    result = CryptDestroyKey(hKey);
    ok(result, "%08x\n", GetLastError());

    /* Windows >= XP changed the way DES keys are derived, this test ensures we don't break that */
    derive_key(CALG_DES, &hKey, 56);

    dwMode = CRYPT_MODE_ECB;
    result = CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
    ok(result, "%08x\n", GetLastError());

    for (i=0; i<sizeof(pbData); i++) pbData[i] = (unsigned char)i;

    dwLen = 13;
    result = CryptEncrypt(hKey, 0, TRUE, 0, pbData, &dwLen, 16);
    ok(result, "%08x\n", GetLastError());
    ok(!memcmp(pbData, des, sizeof(des)) || broken(
    !memcmp(pbData, des_old_behavior, sizeof(des)) ||
    (STRONG_PROV && !memcmp(pbData, des_old_strong, sizeof(des)))) /* <= 2000 */,
       "DES encryption failed!\n");

    result = CryptDestroyKey(hKey);
    ok(result, "%08x\n", GetLastError());
}

static void test_3des(void)
{
    HCRYPTKEY hKey;
    BOOL result;
    DWORD dwLen;
    unsigned char pbData[16], enc_data[16], bad_data[16];
    static const BYTE des3[16] = { 
        0x7b, 0xba, 0xdd, 0xa2, 0x39, 0xd3, 0x7b, 0xb3, 
        0xc7, 0x51, 0x81, 0x41, 0x53, 0xe8, 0xcf, 0xeb };
    int i;

    result = derive_key(CALG_3DES, &hKey, 0);
    if (!result) return;

    for (i=0; i<sizeof(pbData); i++) pbData[i] = (unsigned char)i;
    
    dwLen = 13;
    result = CryptEncrypt(hKey, 0, TRUE, 0, pbData, &dwLen, 16);
    ok(result, "%08x\n", GetLastError());

    ok(!memcmp(pbData, des3, sizeof(des3)), "3DES encryption failed!\n");

    result = CryptDecrypt(hKey, 0, TRUE, 0, pbData, &dwLen);
    ok(result, "%08x\n", GetLastError());

    for (i=0; i<4; i++)
    {
      memcpy(pbData,cTestData[i].origstr,cTestData[i].strlen);

      dwLen = cTestData[i].enclen;
      result = CryptEncrypt(hKey, 0, TRUE, 0, pbData, &dwLen, cTestData[i].buflen);
      ok(result, "%08x\n", GetLastError());
      ok(dwLen==cTestData[i].buflen,"length incorrect, got %d, expected %d\n",dwLen,cTestData[i].buflen);
      memcpy(enc_data, pbData, cTestData[i].buflen);

      result = CryptDecrypt(hKey, 0, TRUE, 0, pbData, &dwLen);
      ok(result, "%08x\n", GetLastError());
      ok(dwLen==cTestData[i].enclen,"length incorrect, got %d, expected %d\n",dwLen,cTestData[i].enclen);
      ok(memcmp(pbData,cTestData[i].decstr,cTestData[i].enclen)==0,"decryption incorrect %d\n",i);
      if((dwLen != cTestData[i].enclen) ||
         memcmp(pbData,cTestData[i].decstr,cTestData[i].enclen))
      {
          printBytes("expected",cTestData[i].decstr,cTestData[i].strlen);
          printBytes("got",pbData,dwLen);
      }

      /* Test bad data:
         Decrypting a block of bad data with Final = TRUE should restore the
         initial state of the key as well as decrypting a block of good data.
       */

      /* Changing key state by setting Final = FALSE */
      dwLen = cTestData[i].buflen;
      memcpy(pbData, enc_data, cTestData[i].buflen);
      result = CryptDecrypt(hKey, 0, FALSE, 0, pbData, &dwLen);
      ok(result, "%08x\n", GetLastError());

      /* Restoring key state by decrypting bad_data with Final = TRUE */
      memcpy(bad_data, enc_data, cTestData[i].buflen);
      bad_data[cTestData[i].buflen - 1] = ~bad_data[cTestData[i].buflen - 1];
      SetLastError(0xdeadbeef);
      result = CryptDecrypt(hKey, 0, TRUE, 0, bad_data, &dwLen);
      ok(!result, "CryptDecrypt should failed!\n");
      ok(GetLastError() == NTE_BAD_DATA, "%08x\n", GetLastError());
      ok(dwLen==cTestData[i].buflen,"length incorrect, got %d, expected %d\n",dwLen,cTestData[i].buflen);
      ok(memcmp(pbData,cTestData[i].decstr,cTestData[1].enclen)==0,"decryption incorrect %d\n",i);

      /* Checking key state */
      dwLen = cTestData[i].buflen;
      memcpy(pbData, enc_data, cTestData[i].buflen);
      result = CryptDecrypt(hKey, 0, TRUE, 0, pbData, &dwLen);
      ok(result, "%08x\n", GetLastError());
      ok(dwLen==cTestData[i].enclen,"length incorrect, got %d, expected %d\n",dwLen,cTestData[i].enclen);
      ok(memcmp(pbData,cTestData[i].decstr,cTestData[1].enclen)==0,"decryption incorrect %d\n",i);
      if((dwLen != cTestData[i].enclen) ||
         memcmp(pbData,cTestData[i].decstr,cTestData[i].enclen))
      {
          printBytes("expected",cTestData[i].decstr,cTestData[i].strlen);
          printBytes("got",pbData,dwLen);
      }
    }
    result = CryptDestroyKey(hKey);
    ok(result, "%08x\n", GetLastError());
}

static void test_aes(int keylen)
{
    HCRYPTKEY hKey;
    BOOL result;
    DWORD dwLen, dwMode;
    unsigned char pbData[48], enc_data[16], bad_data[16];
    int i;
    static const BYTE aes_plain[32] = {
        "AES Test With 2 Blocks Of Data." };
    static const BYTE aes_cbc_enc[3][48] = {
    /* 128 bit key encrypted text */
    { 0xfe, 0x85, 0x3b, 0xe1, 0xf5, 0xe1, 0x58, 0x75, 0xd5, 0xa9, 0x74, 0xe3, 0x09, 0xea, 0xa5, 0x04,
      0x23, 0x35, 0xa2, 0x3b, 0x5c, 0xf1, 0x6c, 0x6f, 0xb9, 0xcd, 0x64, 0x06, 0x3e, 0x41, 0x83, 0xef,
      0x2a, 0xfe, 0xea, 0xb5, 0x6c, 0x17, 0x20, 0x79, 0x8c, 0x51, 0x3e, 0x56, 0xed, 0xe1, 0x47, 0x68 },
    /* 192 bit key encrypted text */
    { 0x6b, 0xf0, 0xfd, 0x32, 0xee, 0xc6, 0x06, 0x13, 0xa8, 0xe6, 0x3c, 0x81, 0x85, 0xb8, 0x2e, 0xa1,
      0xd4, 0x3b, 0xe8, 0x22, 0xa5, 0x74, 0x4a, 0xbe, 0x9d, 0xcf, 0xcc, 0x37, 0x26, 0x19, 0x5a, 0xd1,
      0x7f, 0x76, 0xbf, 0x94, 0x28, 0xce, 0x27, 0x21, 0x61, 0x87, 0xeb, 0xb9, 0x8b, 0xa8, 0xb4, 0x57 },
    /* 256 bit key encrypted text */
    { 0x20, 0x57, 0x17, 0x0b, 0x17, 0x76, 0xd8, 0x3b, 0x26, 0x90, 0x8b, 0x4c, 0xf2, 0x00, 0x79, 0x33,
      0x29, 0x2b, 0x13, 0x9c, 0xe2, 0x95, 0x09, 0xc1, 0xcd, 0x20, 0x87, 0x22, 0x32, 0x70, 0x9d, 0x75,
      0x9a, 0x94, 0xf5, 0x76, 0x5c, 0xb1, 0x62, 0x2c, 0xe1, 0x76, 0x7c, 0x86, 0x73, 0xe6, 0x7a, 0x23 }
    };
    switch (keylen)
    {
        case 256:
            result = derive_key(CALG_AES_256, &hKey, 0);
            i = 2;
            break;
        case 192:
            result = derive_key(CALG_AES_192, &hKey, 0);
            i = 1;
            break;
        default:
        case 128:
            result = derive_key(CALG_AES_128, &hKey, 0);
            i = 0;
            break;
    }
    if (!result) return;

    dwLen = sizeof(aes_plain);
    memcpy(pbData, aes_plain, dwLen);
    result = CryptEncrypt(hKey, 0, TRUE, 0, pbData, &dwLen, sizeof(pbData));
    ok(result, "Expected OK, got last error %d\n", GetLastError());
    ok(dwLen == 48, "Expected dwLen 48, got %d\n", dwLen);
    ok(!memcmp(aes_cbc_enc[i], pbData, dwLen), "Expected equal data sequences\n");

    result = CryptDecrypt(hKey, 0, TRUE, 0, pbData, &dwLen);
    ok(result && dwLen == 32 && !memcmp(aes_plain, pbData, dwLen),
       "%08x, dwLen: %d\n", GetLastError(), dwLen);

    for (i=0; i<sizeof(pbData); i++) pbData[i] = (unsigned char)i;

    /* Does AES provider support salt? */
    result = CryptGetKeyParam(hKey, KP_SALT, NULL, &dwLen, 0);
    todo_wine ok(result || broken(GetLastError() == NTE_BAD_KEY), /* Vista or older */
       "Expected OK, got last error %d\n", GetLastError());
    if (result)
        ok(!dwLen, "unexpected salt length %d\n", dwLen);

    /* test default chaining mode */
    dwMode = 0xdeadbeef;
    dwLen = sizeof(dwMode);
    result = CryptGetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, &dwLen, 0);
    ok(result, "%08x\n", GetLastError());
    ok(dwMode == CRYPT_MODE_CBC, "Wrong default chaining\n");

    dwLen = 13;
    result = CryptEncrypt(hKey, 0, TRUE, 0, pbData, &dwLen, 16);
    ok(result, "%08x\n", GetLastError());

    result = CryptDecrypt(hKey, 0, TRUE, 0, pbData, &dwLen);
    ok(result, "%08x\n", GetLastError());

    for (i=0; i<4; i++)
    {
      memcpy(pbData,cTestData[i].origstr,cTestData[i].strlen);

      dwLen = cTestData[i].enclen;
      result = CryptEncrypt(hKey, 0, TRUE, 0, pbData, &dwLen, cTestData[i].buflen);
      ok(result, "%08x\n", GetLastError());
      ok(dwLen==cTestData[i].buflen,"length incorrect, got %d, expected %d\n",dwLen,cTestData[i].buflen);
      memcpy(enc_data, pbData, cTestData[i].buflen);

      result = CryptDecrypt(hKey, 0, TRUE, 0, pbData, &dwLen);
      ok(result, "%08x\n", GetLastError());
      ok(dwLen==cTestData[i].enclen,"length incorrect, got %d, expected %d\n",dwLen,cTestData[i].enclen);
      ok(memcmp(pbData,cTestData[i].decstr,cTestData[1].enclen)==0,"decryption incorrect %d\n",i);
      if((dwLen != cTestData[i].enclen) ||
         memcmp(pbData,cTestData[i].decstr,cTestData[i].enclen))
      {
          printBytes("expected",cTestData[i].decstr,cTestData[i].strlen);
          printBytes("got",pbData,dwLen);
      }

      /* Test bad data:
         Decrypting a block of bad data with Final = TRUE should restore the
         initial state of the key as well as decrypting a block of good data.
       */

      /* Changing key state by setting Final = FALSE */
      dwLen = cTestData[i].buflen;
      memcpy(pbData, enc_data, cTestData[i].buflen);
      result = CryptDecrypt(hKey, 0, FALSE, 0, pbData, &dwLen);
      ok(result, "%08x\n", GetLastError());

      /* Restoring key state by decrypting bad_data with Final = TRUE */
      memcpy(bad_data, enc_data, cTestData[i].buflen);
      bad_data[cTestData[i].buflen - 1] = ~bad_data[cTestData[i].buflen - 1];
      SetLastError(0xdeadbeef);
      result = CryptDecrypt(hKey, 0, TRUE, 0, bad_data, &dwLen);
      ok(!result, "CryptDecrypt should failed!\n");
      ok(GetLastError() == NTE_BAD_DATA, "%08x\n", GetLastError());
      ok(dwLen==cTestData[i].buflen,"length incorrect, got %d, expected %d\n",dwLen,cTestData[i].buflen);
      ok(memcmp(pbData,cTestData[i].decstr,cTestData[1].enclen)==0,"decryption incorrect %d\n",i);

      /* Checking key state */
      dwLen = cTestData[i].buflen;
      memcpy(pbData, enc_data, cTestData[i].buflen);
      result = CryptDecrypt(hKey, 0, TRUE, 0, pbData, &dwLen);
      ok(result, "%08x\n", GetLastError());
      ok(dwLen==cTestData[i].enclen,"length incorrect, got %d, expected %d\n",dwLen,cTestData[i].enclen);
      ok(memcmp(pbData,cTestData[i].decstr,cTestData[1].enclen)==0,"decryption incorrect %d\n",i);
      if((dwLen != cTestData[i].enclen) ||
         memcmp(pbData,cTestData[i].decstr,cTestData[i].enclen))
      {
          printBytes("expected",cTestData[i].decstr,cTestData[i].strlen);
          printBytes("got",pbData,dwLen);
      }
    }
    result = CryptDestroyKey(hKey);
    ok(result, "%08x\n", GetLastError());
}

static void test_sha2(void)
{
    static const unsigned char sha256hash[32] = {
        0x10, 0xfc, 0x3c, 0x51, 0xa1, 0x52, 0xe9, 0x0e, 0x5b, 0x90,
        0x31, 0x9b, 0x60, 0x1d, 0x92, 0xcc, 0xf3, 0x72, 0x90, 0xef,
        0x53, 0xc3, 0x5f, 0xf9, 0x25, 0x07, 0x68, 0x7d, 0x8a, 0x91,
        0x1a, 0x08
    };
    static const unsigned char sha384hash[48] = {
        0x98, 0xd3, 0x3f, 0x89, 0x0b, 0x23, 0x33, 0x44, 0x61, 0x32,
        0x5a, 0x7c, 0xa3, 0x03, 0x89, 0xb5, 0x11, 0xd7, 0x41, 0xc8,
        0x54, 0x6b, 0x12, 0x0c, 0x40, 0x15, 0xb6, 0x2a, 0x03, 0x43,
        0xe5, 0x64, 0x7f, 0x10, 0x1e, 0xae, 0x47, 0xa9, 0x39, 0x05,
        0x6f, 0x40, 0x60, 0x94, 0xd6, 0xad, 0x80, 0x55
    };
    static const unsigned char sha512hash[64] = {
        0x37, 0x86, 0x0e, 0x7d, 0x25, 0xd9, 0xf9, 0x84, 0x3e, 0x3d,
        0xc7, 0x13, 0x95, 0x73, 0x42, 0x04, 0xfd, 0x13, 0xad, 0x23,
        0x39, 0x16, 0x32, 0x5f, 0x99, 0x3e, 0x3c, 0xee, 0x3f, 0x11,
        0x36, 0xf9, 0xc9, 0x66, 0x08, 0x70, 0xcc, 0x49, 0xd8, 0xe0,
        0x7d, 0xa1, 0x57, 0x62, 0x71, 0xa6, 0xc9, 0xa4, 0x24, 0x60,
        0xfc, 0xde, 0x9d, 0xb2, 0xf1, 0xd2, 0xc2, 0xfb, 0x2d, 0xbf,
        0xb7, 0xf4, 0x81, 0xd4
    };
    unsigned char pbData[2048];
    BOOL result;
    HCRYPTHASH hHash;
    BYTE pbHashValue[64];
    DWORD hashlen, len;
    int i;

    for (i=0; i<2048; i++) pbData[i] = (unsigned char)i;

    /* SHA-256 hash */
    SetLastError(0xdeadbeef);
    result = CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash);
    if (!result && GetLastError() == NTE_BAD_ALGID) {
        win_skip("SHA-256/384/512 hashes are not supported before Windows XP SP3\n");
        return;
    }
    ok(result, "%08x\n", GetLastError());
    if (result) {
        len = sizeof(DWORD);
        result = CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashlen, &len, 0);
        ok(result && (hashlen == 32), "%08x, hashlen: %d\n", GetLastError(), hashlen);

        result = CryptHashData(hHash, pbData, sizeof(pbData), 0);
        ok(result, "%08x\n", GetLastError());

        len = 32;
        result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &len, 0);
        ok(result, "%08x\n", GetLastError());

        ok(!memcmp(pbHashValue, sha256hash, 32), "Wrong SHA-256 hash!\n");

        result = CryptDestroyHash(hHash);
        ok(result, "%08x\n", GetLastError());
    }

    /* SHA-384 hash */
    result = CryptCreateHash(hProv, CALG_SHA_384, 0, 0, &hHash);
    ok(result, "%08x\n", GetLastError());
    if (result) {
        len = sizeof(DWORD);
        result = CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashlen, &len, 0);
        ok(result && (hashlen == 48), "%08x, hashlen: %d\n", GetLastError(), hashlen);

        result = CryptHashData(hHash, pbData, sizeof(pbData), 0);
        ok(result, "%08x\n", GetLastError());

        len = 48;
        result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &len, 0);
        ok(result, "%08x\n", GetLastError());

        ok(!memcmp(pbHashValue, sha384hash, 48), "Wrong SHA-384 hash!\n");

        result = CryptDestroyHash(hHash);
        ok(result, "%08x\n", GetLastError());
    }

    /* SHA-512 hash */
    result = CryptCreateHash(hProv, CALG_SHA_512, 0, 0, &hHash);
    ok(result, "%08x\n", GetLastError());
    if (result) {
        len = sizeof(DWORD);
        result = CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashlen, &len, 0);
        ok(result && (hashlen == 64), "%08x, hashlen: %d\n", GetLastError(), hashlen);

        result = CryptHashData(hHash, pbData, sizeof(pbData), 0);
        ok(result, "%08x\n", GetLastError());

        len = 64;
        result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &len, 0);
        ok(result, "%08x\n", GetLastError());

        ok(!memcmp(pbHashValue, sha512hash, 64), "Wrong SHA-512 hash!\n");

        result = CryptDestroyHash(hHash);
        ok(result, "%08x\n", GetLastError());
    }
}

static void test_rc2(void)
{
    static const BYTE rc2_40_encrypted[16] = {
        0xc0, 0x9a, 0xe4, 0x2f, 0x0a, 0x47, 0x67, 0x11,
        0xfb, 0x18, 0x87, 0xce, 0x0c, 0x75, 0x07, 0xb1 };
    static const BYTE rc2_128_encrypted[] = {
        0x82,0x81,0xf7,0xff,0xdd,0xd7,0x88,0x8c,
        0x2a,0x2a,0xc0,0xce,0x4c,0x89,0xb6,0x66 };
    static const BYTE rc2_40def_encrypted[] = {
        0x23,0xc8,0x70,0x13,0x42,0x2e,0xa8,0x98,
        0x5c,0xdf,0x7a,0x9b,0xea,0xdb,0x96,0x1b };
    static const BYTE rc2_40_salt_enh[24] = {
        0xA3, 0xD7, 0x41, 0x87, 0x7A, 0xD0, 0x18, 0xDB,
        0xD4, 0x6A, 0x4F, 0xEE, 0xF3, 0xCA, 0xCD, 0x34,
        0xB3, 0x15, 0x9A, 0x2A, 0x88, 0x5F, 0x43, 0xA5 };
    static const BYTE rc2_40_salt_base[24] = {
        0x8C, 0x4E, 0xA6, 0x00, 0x9B, 0x15, 0xEF, 0x9E,
        0x88, 0x81, 0xD0, 0x65, 0xD6, 0x53, 0x57, 0x08,
        0x0A, 0x77, 0x80, 0xFA, 0x7E, 0x89, 0x14, 0x55 };
    static const BYTE rc2_40_salt_strong[24] = {
        0xB9, 0x33, 0xB6, 0x7A, 0x35, 0xC3, 0x06, 0x88,
        0xBF, 0xD5, 0xCC, 0xAF, 0x14, 0xAE, 0xE2, 0x31,
        0xC6, 0x9A, 0xAA, 0x3F, 0x05, 0x2F, 0x22, 0xDA };
    HCRYPTHASH hHash;
    HCRYPTKEY hKey;
    BOOL result;
    DWORD dwLen, dwKeyLen, dwDataLen, dwMode, dwModeBits, error;
    unsigned char pbData[2000], pbHashValue[16], pszBuffer[256];
    int i;
    
    for (i=0; i<2000; i++) pbData[i] = (unsigned char)i;

    /* MD2 Hashing */
    result = CryptCreateHash(hProv, CALG_MD2, 0, 0, &hHash);
    if (!result) {
        ok(GetLastError()==NTE_BAD_ALGID, "%08x\n", GetLastError());
    } else {
        CRYPT_INTEGER_BLOB salt;

        result = CryptHashData(hHash, pbData, sizeof(pbData), 0);
        ok(result, "%08x\n", GetLastError());

        dwLen = 16;
        result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());

        result = CryptDeriveKey(hProv, CALG_RC2, hHash, 40 << 16, &hKey);
        ok(result, "%08x\n", GetLastError());

        dwLen = sizeof(DWORD);
        result = CryptGetKeyParam(hKey, KP_KEYLEN, (BYTE*)&dwKeyLen, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());

        /* test default chaining mode */
        dwMode = 0xdeadbeef;
        dwLen = sizeof(dwMode);
        result = CryptGetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        ok(dwMode == CRYPT_MODE_CBC, "Wrong default chaining mode\n");

        dwMode = CRYPT_MODE_CBC;
        result = CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
        ok(result, "%08x\n", GetLastError());

        dwLen = sizeof(DWORD);
        result = CryptGetKeyParam(hKey, KP_MODE_BITS, (BYTE*)&dwModeBits, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());

        dwModeBits = 0xdeadbeef;
        dwLen = sizeof(DWORD);
        result = CryptGetKeyParam(hKey, KP_PERMISSIONS, (BYTE*)&dwModeBits, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        ok(dwModeBits ==
            (CRYPT_MAC|CRYPT_WRITE|CRYPT_READ|CRYPT_DECRYPT|CRYPT_ENCRYPT) ||
            broken(dwModeBits == 0xffffffff), /* Win9x/NT4 */
            "expected CRYPT_MAC|CRYPT_WRITE|CRYPT_READ|CRYPT_DECRYPT|CRYPT_ENCRYPT,"
            " got %08x\n", dwModeBits);

        dwLen = sizeof(DWORD);
        result = CryptGetKeyParam(hKey, KP_PERMISSIONS, (BYTE*)&dwModeBits, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());

        dwLen = sizeof(DWORD);
        result = CryptGetKeyParam(hKey, KP_BLOCKLEN, (BYTE*)&dwModeBits, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        ok(dwLen == 4, "Expected 4, got %d\n", dwLen);

        dwLen = 0;
        result = CryptGetKeyParam(hKey, KP_IV, NULL, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        result = CryptGetKeyParam(hKey, KP_IV, pszBuffer, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        ok(dwLen == 8, "Expected 8, got %d\n", dwLen);

        dwLen = 0;
        result = CryptGetKeyParam(hKey, KP_SALT, NULL, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        /* The default salt length is always 11... */
        ok(dwLen == 11, "unexpected salt length %d\n", dwLen);
        /* and the default salt is always empty. */
        result = CryptGetKeyParam(hKey, KP_SALT, pszBuffer, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        for (i=0; i<dwLen; i++)
            ok(!pszBuffer[i], "unexpected salt value %02x @ %d\n", pszBuffer[i], i);

        dwLen = sizeof(DWORD);
        result = CryptGetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        ok(dwMode == CRYPT_MODE_CBC, "Expected CRYPT_MODE_CBC, got %d\n", dwMode);

        result = CryptDestroyHash(hHash);
        ok(result, "%08x\n", GetLastError());

        dwDataLen = 13;
        result = CryptEncrypt(hKey, 0, TRUE, 0, pbData, &dwDataLen, 24);
        ok(result, "%08x\n", GetLastError());

        ok(!memcmp(pbData, rc2_40_encrypted, 16), "RC2 encryption failed!\n");

        dwLen = 0;
        result = CryptGetKeyParam(hKey, KP_IV, NULL, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        result = CryptGetKeyParam(hKey, KP_IV, pszBuffer, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());

        result = CryptDecrypt(hKey, 0, TRUE, 0, pbData, &dwDataLen);
        ok(result, "%08x\n", GetLastError());

        /* Setting the salt value will not reset the salt length in base or strong providers */
        result = CryptSetKeyParam(hKey, KP_SALT, pbData, 0);
        ok(result, "setting salt failed: %08x\n", GetLastError());
        dwLen = 0;
        result = CryptGetKeyParam(hKey, KP_SALT, NULL, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        if (BASE_PROV || STRONG_PROV)
            ok(dwLen == 11, "expected salt length 11, got %d\n", dwLen);
        else
            ok(dwLen == 0 || broken(nt4 && dwLen == 11), "expected salt length 0, got %d\n", dwLen);
        /* What sizes salt can I set? */
        salt.pbData = pbData;
        for (i=0; i<24; i++)
        {
            salt.cbData = i;
            result = CryptSetKeyParam(hKey, KP_SALT_EX, (BYTE *)&salt, 0);
            ok(result, "setting salt failed for size %d: %08x\n", i, GetLastError());
            /* The returned salt length is the same as the set salt length */
            result = CryptGetKeyParam(hKey, KP_SALT, NULL, &dwLen, 0);
            ok(result, "%08x\n", GetLastError());
            ok(dwLen == i, "size %d: unexpected salt length %d\n", i, dwLen);
        }
        salt.cbData = 25;
        SetLastError(0xdeadbeef);
        result = CryptSetKeyParam(hKey, KP_SALT_EX, (BYTE *)&salt, 0);
        ok(!result ||
           broken(result), /* Win9x, WinMe, NT4, W2K */
           "%08x\n", GetLastError());

        result = CryptDestroyKey(hKey);
        ok(result, "%08x\n", GetLastError());
    }

    /* Again, but test setting the effective key len */
    for (i=0; i<2000; i++) pbData[i] = (unsigned char)i;

    result = CryptCreateHash(hProv, CALG_MD2, 0, 0, &hHash);
    if (!result) {
        ok(GetLastError()==NTE_BAD_ALGID, "%08x\n", GetLastError());
    } else {
        result = CryptHashData(hHash, pbData, sizeof(pbData), 0);
        ok(result, "%08x\n", GetLastError());

        dwLen = 16;
        result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());

        result = CryptDeriveKey(hProv, CALG_RC2, hHash, 56 << 16, &hKey);
        ok(result, "%08x\n", GetLastError());

        SetLastError(0xdeadbeef);
        result = CryptSetKeyParam(hKey, KP_EFFECTIVE_KEYLEN, NULL, 0);
        ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "%08x\n", GetLastError());
        dwKeyLen = 0;
        SetLastError(0xdeadbeef);
        result = CryptSetKeyParam(hKey, KP_EFFECTIVE_KEYLEN, (LPBYTE)&dwKeyLen, 0);
        ok(!result && GetLastError()==NTE_BAD_DATA, "%08x\n", GetLastError());
        dwKeyLen = 1025;
        SetLastError(0xdeadbeef);
        result = CryptSetKeyParam(hKey, KP_EFFECTIVE_KEYLEN, (LPBYTE)&dwKeyLen, 0);
        ok(!result, "CryptSetKeyParam failed: %08x\n", GetLastError());

        dwLen = sizeof(dwKeyLen);
        result = CryptGetKeyParam(hKey, KP_KEYLEN, (BYTE *)&dwKeyLen, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        ok(dwKeyLen == 56, "%d (%08x)\n", dwKeyLen, GetLastError());
        result = CryptGetKeyParam(hKey, KP_EFFECTIVE_KEYLEN, (BYTE *)&dwKeyLen, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        ok(dwKeyLen == 56 || broken(dwKeyLen == 40), "%d (%08x)\n", dwKeyLen, GetLastError());

        dwKeyLen = 128;
        SetLastError(0xdeadbeef);
        result = CryptSetKeyParam(hKey, KP_EFFECTIVE_KEYLEN, (LPBYTE)&dwKeyLen, 0);
        if (!BASE_PROV)
        {
            dwKeyLen = 12345;
            ok(result, "expected success, got error 0x%08X\n", GetLastError());
            result = CryptGetKeyParam(hKey, KP_EFFECTIVE_KEYLEN, (BYTE *)&dwKeyLen, &dwLen, 0);
            ok(result, "%08x\n", GetLastError());
            ok(dwKeyLen == 128, "Expected 128, got %d\n", dwKeyLen);
        }
        else
        {
            ok(!result, "expected error\n");
            ok(GetLastError() == NTE_BAD_DATA, "Expected 0x80009005, got 0x%08X\n", GetLastError());
            result = CryptGetKeyParam(hKey, KP_EFFECTIVE_KEYLEN, (BYTE *)&dwKeyLen, &dwLen, 0);
            ok(result, "%08x\n", GetLastError());
            ok(dwKeyLen == 40, "Expected 40, got %d\n", dwKeyLen);
        }

        dwLen = sizeof(dwKeyLen);
        result = CryptGetKeyParam(hKey, KP_KEYLEN, (BYTE *)&dwKeyLen, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        ok(dwKeyLen == 56, "%d (%08x)\n", dwKeyLen, GetLastError());
        result = CryptGetKeyParam(hKey, KP_EFFECTIVE_KEYLEN, (BYTE *)&dwKeyLen, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        ok((!BASE_PROV && dwKeyLen == 128) || (BASE_PROV && dwKeyLen == 40),
           "%d (%08x)\n", dwKeyLen, GetLastError());

        result = CryptDestroyHash(hHash);
        ok(result, "%08x\n", GetLastError());

        dwDataLen = 13;
        result = CryptEncrypt(hKey, 0, TRUE, 0, pbData, &dwDataLen, 24);
        ok(result, "%08x\n", GetLastError());
        ok(!memcmp(pbData, !BASE_PROV ? rc2_128_encrypted : rc2_40def_encrypted,
           sizeof(rc2_128_encrypted)), "RC2 encryption failed!\n");

        /* Oddly enough this succeeds, though it should have no effect */
        dwKeyLen = 40;
        result = CryptSetKeyParam(hKey, KP_EFFECTIVE_KEYLEN, (LPBYTE)&dwKeyLen, 0);
        ok(result, "%d\n", GetLastError());

        result = CryptDestroyKey(hKey);
        ok(result, "%08x\n", GetLastError());

        /* Test a 40 bit key with salt */
        result = CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
        ok(result, "%08x\n", GetLastError());

        result = CryptHashData(hHash, pbData, sizeof(pbData), 0);
        ok(result, "%08x\n", GetLastError());

        result = CryptDeriveKey(hProv, CALG_RC2, hHash, (40<<16)|CRYPT_CREATE_SALT, &hKey);
        ok(result, "%08x\n", GetLastError());

        dwDataLen = 16;
        memset(pbData, 0xAF, dwDataLen);
        SetLastError(0xdeadbeef);
        result = CryptEncrypt(hKey, 0, TRUE, 0, pbData, &dwDataLen, 24);
        if(result)
        {
            ok((ENHANCED_PROV && !memcmp(pbData, rc2_40_salt_enh, dwDataLen)) ||
               (STRONG_PROV && !memcmp(pbData, rc2_40_salt_strong, dwDataLen)) ||
               (BASE_PROV && !memcmp(pbData, rc2_40_salt_base, dwDataLen)),
               "RC2 encryption failed!\n");
        }
        else /* <= XP */
        {
            error = GetLastError();
            ok(error == NTE_BAD_DATA || broken(error == NTE_DOUBLE_ENCRYPT),
               "Expected 0x80009005, got 0x%08X\n", error);
        }
        dwLen = sizeof(DWORD);
        dwKeyLen = 12345;
        result = CryptGetKeyParam(hKey, KP_KEYLEN, (BYTE*)&dwKeyLen, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        ok(dwKeyLen == 40, "Expected 40, got %d\n", dwKeyLen);

        dwLen = sizeof(pszBuffer);
        memset(pszBuffer, 0xAF, dwLen);
        result = CryptGetKeyParam(hKey, KP_SALT, pszBuffer, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        if (!ENHANCED_PROV)
            ok(dwLen == 11, "Expected 11, got %d\n", dwLen);
        else
            ok(dwLen == 0, "Expected 0, got %d\n", dwLen);

        result = CryptDestroyKey(hKey);
        ok(result, "%08x\n", GetLastError());

        result = CryptDestroyHash(hHash);
        ok(result, "%08x\n", GetLastError());
    }
}

static void test_rc4(void)
{
    static const BYTE rc4[16] = { 
        0x17, 0x0c, 0x44, 0x8e, 0xae, 0x90, 0xcd, 0xb0, 
        0x7f, 0x87, 0xf5, 0x7a, 0xec, 0xb2, 0x2e, 0x35 };    
    static const BYTE rc4_40_salt[16] = {
        0x41, 0xE6, 0x33, 0xC9, 0x50, 0xA1, 0xBF, 0x88,
        0x12, 0x4D, 0xD3, 0xE3, 0x47, 0x88, 0x6D, 0xA5 };
    static const BYTE rc4_40_salt_base[16] = {
        0x2F, 0xAC, 0xEA, 0xEA, 0xFF, 0x68, 0x7E, 0x77,
        0xF4, 0xB9, 0x48, 0x7C, 0x4E, 0x79, 0xA6, 0xB5 };
    BOOL result;
    HCRYPTHASH hHash;
    HCRYPTKEY hKey;
    DWORD dwDataLen = 5, dwKeyLen, dwLen = sizeof(DWORD), dwMode;
    unsigned char pbData[2000];
    unsigned char pszBuffer[256];
    int i;

    for (i=0; i<2000; i++) pbData[i] = (unsigned char)i;

    /* MD2 Hashing */
    result = CryptCreateHash(hProv, CALG_MD2, 0, 0, &hHash);
    if (!result) {
        /* rsaenh compiled without OpenSSL */
        ok(GetLastError() == NTE_BAD_ALGID, "%08x\n", GetLastError());
    } else {
        CRYPT_INTEGER_BLOB salt;

        result = CryptHashData(hHash, pbData, sizeof(pbData), 0);
           ok(result, "%08x\n", GetLastError());

        dwLen = 16;
        result = CryptGetHashParam(hHash, HP_HASHVAL, pszBuffer, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());

        result = CryptDeriveKey(hProv, CALG_RC4, hHash, 56 << 16, &hKey);
        ok(result, "%08x\n", GetLastError());

        dwLen = sizeof(DWORD);
        result = CryptGetKeyParam(hKey, KP_KEYLEN, (BYTE*)&dwKeyLen, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        ok(dwKeyLen == 56, "Expected 56, got %d\n", dwKeyLen);

        dwLen = sizeof(DWORD);
        result = CryptGetKeyParam(hKey, KP_BLOCKLEN, (BYTE*)&dwKeyLen, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        ok(dwKeyLen == 0, "Expected 0, got %d\n", dwKeyLen);

        dwLen = 0;
        result = CryptGetKeyParam(hKey, KP_IV, NULL, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        result = CryptGetKeyParam(hKey, KP_IV, pszBuffer, &dwLen, 0);

        dwLen = 0;
        result = CryptGetKeyParam(hKey, KP_SALT, NULL, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        result = CryptGetKeyParam(hKey, KP_SALT, pszBuffer, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());

        dwLen = sizeof(DWORD);
        result = CryptGetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        ok(dwMode == 0 || broken(dwMode == CRYPT_MODE_CBC) /* <= 2000 */,
           "Expected 0, got %d\n", dwMode);

        result = CryptDestroyHash(hHash);
        ok(result, "%08x\n", GetLastError());

        dwDataLen = 16;
        result = CryptEncrypt(hKey, 0, TRUE, 0, NULL, &dwDataLen, 24);
        ok(result, "%08x\n", GetLastError());
        dwDataLen = 16;
        result = CryptEncrypt(hKey, 0, TRUE, 0, pbData, &dwDataLen, 24);
        ok(result, "%08x\n", GetLastError());

        ok(!memcmp(pbData, rc4, dwDataLen), "RC4 encryption failed!\n");

        result = CryptDecrypt(hKey, 0, TRUE, 0, pbData, &dwDataLen);
        ok(result, "%08x\n", GetLastError());

        /* Setting the salt value will not reset the salt length in base or strong providers */
        result = CryptSetKeyParam(hKey, KP_SALT, pbData, 0);
        ok(result, "setting salt failed: %08x\n", GetLastError());
        dwLen = 0;
        result = CryptGetKeyParam(hKey, KP_SALT, NULL, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        if (BASE_PROV || STRONG_PROV)
            ok(dwLen == 11, "expected salt length 11, got %d\n", dwLen);
        else
            ok(dwLen == 0 || broken(nt4 && dwLen == 11), "expected salt length 0, got %d\n", dwLen);
        /* What sizes salt can I set? */
        salt.pbData = pbData;
        for (i=0; i<24; i++)
        {
            salt.cbData = i;
            result = CryptSetKeyParam(hKey, KP_SALT_EX, (BYTE *)&salt, 0);
            ok(result, "setting salt failed for size %d: %08x\n", i, GetLastError());
            /* The returned salt length is the same as the set salt length */
            result = CryptGetKeyParam(hKey, KP_SALT, NULL, &dwLen, 0);
            ok(result, "%08x\n", GetLastError());
            ok(dwLen == i, "size %d: unexpected salt length %d\n", i, dwLen);
        }
        salt.cbData = 25;
        SetLastError(0xdeadbeef);
        result = CryptSetKeyParam(hKey, KP_SALT_EX, (BYTE *)&salt, 0);
        ok(!result ||
           broken(result), /* Win9x, WinMe, NT4, W2K */
           "%08x\n", GetLastError());

        result = CryptDestroyKey(hKey);
        ok(result, "%08x\n", GetLastError());

        /* Test a 40 bit key with salt */
        result = CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
        ok(result, "%08x\n", GetLastError());

        result = CryptHashData(hHash, pbData, sizeof(pbData), 0);
        ok(result, "%08x\n", GetLastError());

        result = CryptDeriveKey(hProv, CALG_RC4, hHash, (40<<16)|CRYPT_CREATE_SALT, &hKey);
        ok(result, "%08x\n", GetLastError());
        dwDataLen = 16;
        memset(pbData, 0xAF, dwDataLen);
        SetLastError(0xdeadbeef);
        result = CryptEncrypt(hKey, 0, TRUE, 0, pbData, &dwDataLen, 24);
        ok(result, "%08x\n", GetLastError());
        ok((ENHANCED_PROV && !memcmp(pbData, rc4_40_salt, dwDataLen)) ||
           (!ENHANCED_PROV && !memcmp(pbData, rc4_40_salt_base, dwDataLen)),
           "RC4 encryption failed!\n");

        dwLen = sizeof(DWORD);
        dwKeyLen = 12345;
        result = CryptGetKeyParam(hKey, KP_KEYLEN, (BYTE*)&dwKeyLen, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        ok(dwKeyLen == 40, "Expected 40, got %d\n", dwKeyLen);

        dwLen = sizeof(pszBuffer);
        memset(pszBuffer, 0xAF, dwLen);
        result = CryptGetKeyParam(hKey, KP_SALT, pszBuffer, &dwLen, 0);
        ok(result, "%08x\n", GetLastError());
        if (!ENHANCED_PROV)
            ok(dwLen == 11, "Expected 11, got %d\n", dwLen);
        else
            ok(dwLen == 0, "Expected 0, got %d\n", dwLen);

        result = CryptDestroyKey(hKey);
        ok(result, "%08x\n", GetLastError());

        result = CryptDestroyHash(hHash);
        ok(result, "%08x\n", GetLastError());
    }
}

static void test_hmac(void) {
    HCRYPTKEY hKey;
    HCRYPTHASH hHash;
    BOOL result;
    /* Using CALG_MD2 here fails on Windows 2003, why ? */
    HMAC_INFO hmacInfo = { CALG_MD5, NULL, 0, NULL, 0 };
    DWORD dwLen;
    BYTE abData[256];
    static const BYTE hmac[16] = { 
        0x1a, 0x7d, 0x49, 0xc5, 0x9b, 0x2d, 0x0b, 0x9c, 
        0xcf, 0x10, 0x6b, 0xb6, 0x7d, 0x0f, 0x13, 0x32 };
    int i;

    for (i=0; i<sizeof(abData)/sizeof(BYTE); i++) abData[i] = (BYTE)i;

    if (!derive_key(CALG_RC2, &hKey, 56)) return;

    result = CryptCreateHash(hProv, CALG_HMAC, hKey, 0, &hHash);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptSetHashParam(hHash, HP_HMAC_INFO, (BYTE*)&hmacInfo, 0);
    ok(result, "%08x\n", GetLastError());

    result = CryptHashData(hHash, abData, sizeof(abData), 0);
    ok(result, "%08x\n", GetLastError());

    dwLen = sizeof(abData)/sizeof(BYTE);
    result = CryptGetHashParam(hHash, HP_HASHVAL, abData, &dwLen, 0);
    ok(result, "%08x\n", GetLastError());

    ok(!memcmp(abData, hmac, sizeof(hmac)), "HMAC failed!\n");
    
    result = CryptDestroyHash(hHash);
    ok(result, "%08x\n", GetLastError());
    
    result = CryptDestroyKey(hKey);
    ok(result, "%08x\n", GetLastError());

    /* Provoke errors */
    result = CryptCreateHash(hProv, CALG_HMAC, 0, 0, &hHash);
    ok(!result && GetLastError() == NTE_BAD_KEY, "%08x\n", GetLastError());
}

static void test_mac(void) {
    HCRYPTKEY hKey;
    HCRYPTHASH hHash;
    BOOL result;
    DWORD dwLen;
    BYTE abData[256], abEnc[264];
    static const BYTE mac_40[8] = { 0xb7, 0xa2, 0x46, 0xe9, 0x11, 0x31, 0xe0, 0xad};
    int i;

    for (i=0; i<sizeof(abData)/sizeof(BYTE); i++) abData[i] = (BYTE)i;
    for (i=0; i<sizeof(abData)/sizeof(BYTE); i++) abEnc[i] = (BYTE)i;

    if (!derive_key(CALG_RC2, &hKey, 40)) return;

    dwLen = 256;
    result = CryptEncrypt(hKey, 0, TRUE, 0, abEnc, &dwLen, 264);
    ok (result && dwLen == 264, "%08x, dwLen: %d\n", GetLastError(), dwLen);
    
    result = CryptCreateHash(hProv, CALG_MAC, hKey, 0, &hHash);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptHashData(hHash, abData, sizeof(abData), 0);
    ok(result, "%08x\n", GetLastError());

    dwLen = sizeof(abData)/sizeof(BYTE);
    result = CryptGetHashParam(hHash, HP_HASHVAL, abData, &dwLen, 0);
    ok(result && dwLen == 8, "%08x, dwLen: %d\n", GetLastError(), dwLen);

    ok(!memcmp(abData, mac_40, sizeof(mac_40)), "MAC failed!\n");
    
    result = CryptDestroyHash(hHash);
    ok(result, "%08x\n", GetLastError());
    
    result = CryptDestroyKey(hKey);
    ok(result, "%08x\n", GetLastError());
    
    /* Provoke errors */
    if (!derive_key(CALG_RC4, &hKey, 56)) return;

    SetLastError(0xdeadbeef);
    result = CryptCreateHash(hProv, CALG_MAC, hKey, 0, &hHash);
    ok((!result && GetLastError() == NTE_BAD_KEY) ||
            broken(result), /* Win9x, WinMe, NT4, W2K */
            "%08x\n", GetLastError());

    result = CryptDestroyKey(hKey);
    ok(result, "%08x\n", GetLastError());
}

static void test_import_private(void) 
{
    DWORD dwLen, dwVal;
    HCRYPTKEY hKeyExchangeKey, hSessionKey;
    BOOL result;
    static BYTE abSessionKey[148] = {
        0x01, 0x02, 0x00, 0x00, 0x01, 0x68, 0x00, 0x00,
        0x00, 0xa4, 0x00, 0x00, 0xb8, 0xa4, 0xdf, 0x5e,
        0x9e, 0xb1, 0xbf, 0x85, 0x3d, 0x24, 0x2d, 0x1e,
        0x69, 0xb7, 0x67, 0x13, 0x8e, 0x78, 0xf2, 0xdf,
        0xc6, 0x69, 0xce, 0x46, 0x7e, 0xf2, 0xf2, 0x33,
        0x20, 0x6f, 0xa1, 0xa5, 0x59, 0x83, 0x25, 0xcb,
        0x3a, 0xb1, 0x8a, 0x12, 0x63, 0x02, 0x3c, 0xfb,
        0x4a, 0xfa, 0xef, 0x8e, 0xf7, 0x29, 0x57, 0xb1,
        0x9e, 0xa7, 0xf3, 0x02, 0xfd, 0xca, 0xdf, 0x5a,
        0x1f, 0x71, 0xb6, 0x26, 0x09, 0x24, 0x39, 0xda,
        0xc0, 0xde, 0x2a, 0x0e, 0xcd, 0x1f, 0xe5, 0xb6,
        0x4f, 0x82, 0xa0, 0xa9, 0x90, 0xd3, 0xd9, 0x6a,
        0x43, 0x14, 0x2a, 0xf7, 0xac, 0xd5, 0xa0, 0x54,
        0x93, 0xc4, 0xb9, 0xe7, 0x24, 0x84, 0x4d, 0x69,
        0x5e, 0xcc, 0x2a, 0x32, 0xb5, 0xfb, 0xe4, 0xb4,
        0x08, 0xd5, 0x36, 0x58, 0x59, 0x40, 0xfb, 0x29,
        0x7f, 0xa7, 0x17, 0x25, 0xc4, 0x0d, 0x78, 0x37,
        0x04, 0x8c, 0x49, 0x92
    };
    static BYTE abEncryptedMessage[12] = {
        0x40, 0x64, 0x28, 0xe8, 0x8a, 0xe7, 0xa4, 0xd4,
        0x1c, 0xfd, 0xde, 0x71
    };
    BLOBHEADER *blobHeader = (BLOBHEADER *)abPlainPrivateKey;
    RSAPUBKEY *rsaPubKey = (RSAPUBKEY *)(blobHeader+1);

    dwLen = (DWORD)sizeof(abPlainPrivateKey);
    result = CryptImportKey(hProv, abPlainPrivateKey, dwLen, 0, 0, &hKeyExchangeKey);
    if (!result) {
        /* rsaenh compiled without OpenSSL */
        ok(GetLastError() == NTE_FAIL, "%08x\n", GetLastError());
        return;
    }

    dwLen = (DWORD)sizeof(abSessionKey);
    result = CryptImportKey(hProv, abSessionKey, dwLen, hKeyExchangeKey, 0, &hSessionKey);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    dwVal = 0xdeadbeef;
    dwLen = sizeof(DWORD);
    result = CryptGetKeyParam(hSessionKey, KP_PERMISSIONS, (BYTE*)&dwVal, &dwLen, 0);
    ok(result, "%08x\n", GetLastError());
    ok(dwVal ==
        (CRYPT_MAC|CRYPT_WRITE|CRYPT_READ|CRYPT_DECRYPT|CRYPT_ENCRYPT) ||
        broken(dwVal == 0xffffffff), /* Win9x/NT4 */
        "expected CRYPT_MAC|CRYPT_WRITE|CRYPT_READ|CRYPT_DECRYPT|CRYPT_ENCRYPT,"
        " got %08x\n", dwVal);

    dwLen = (DWORD)sizeof(abEncryptedMessage);
    result = CryptDecrypt(hSessionKey, 0, TRUE, 0, abEncryptedMessage, &dwLen);
    ok(result, "%08x\n", GetLastError());
    ok(dwLen == 12, "expected 12, got %d\n", dwLen);
    ok(!memcmp(abEncryptedMessage, "Wine rocks!", 12), "decrypt failed\n");
    CryptDestroyKey(hSessionKey);
    
    if (!derive_key(CALG_RC4, &hSessionKey, 56)) return;

    dwLen = (DWORD)sizeof(abSessionKey);
    result = CryptExportKey(hSessionKey, hKeyExchangeKey, SIMPLEBLOB, 0, abSessionKey, &dwLen);
    ok(result, "%08x\n", GetLastError());
    CryptDestroyKey(hSessionKey);
    if (!result) return;

    dwLen = (DWORD)sizeof(abSessionKey);
    result = CryptImportKey(hProv, abSessionKey, dwLen, hKeyExchangeKey, 0, &hSessionKey);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    CryptDestroyKey(hSessionKey);
    CryptDestroyKey(hKeyExchangeKey);

    /* Test importing a private key with a buffer that's smaller than the
     * actual buffer.  The private exponent can be omitted, its length is
     * inferred from the passed-in length parameter.
     */
    dwLen = sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) + rsaPubKey->bitlen / 2;
    for (; dwLen < sizeof(abPlainPrivateKey); dwLen++)
    {
        result = CryptImportKey(hProv, abPlainPrivateKey, dwLen, 0, 0, &hKeyExchangeKey);
        ok(result, "CryptImportKey failed at size %d: %d (%08x)\n", dwLen,
           GetLastError(), GetLastError());
        if (result)
            CryptDestroyKey(hKeyExchangeKey);
    }
}

static void test_verify_signature(void) {
    HCRYPTHASH hHash;
    HCRYPTKEY hPubSignKey;
    BYTE abData[] = "Wine rocks!";
    BOOL result;
    BYTE abPubKey[148] = {
        0x06, 0x02, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 
        0x52, 0x53, 0x41, 0x31, 0x00, 0x04, 0x00, 0x00, 
        0x01, 0x00, 0x01, 0x00, 0x71, 0x64, 0x9f, 0x19, 
        0x89, 0x1c, 0x21, 0xcc, 0x36, 0xa3, 0xc9, 0x27, 
        0x08, 0x8a, 0x09, 0xc6, 0xbe, 0xeb, 0xd3, 0xf8, 
        0x19, 0xa9, 0x92, 0x57, 0xe4, 0xb9, 0x5d, 0xda, 
        0x88, 0x93, 0xe4, 0x6b, 0x38, 0x77, 0x14, 0x8a, 
        0x96, 0xc0, 0xb6, 0x4e, 0x42, 0xf5, 0x01, 0xdc, 
        0xf0, 0xeb, 0x3c, 0xc7, 0x7b, 0xc4, 0xfd, 0x7c, 
        0xde, 0x93, 0x34, 0x0a, 0x92, 0xe5, 0x97, 0x9c, 
        0x3e, 0x65, 0xb8, 0x91, 0x2f, 0xe3, 0xf3, 0x89, 
        0xcd, 0x6c, 0x26, 0xa4, 0x6c, 0xc7, 0x6d, 0x0b, 
        0x2c, 0xa2, 0x0b, 0x29, 0xe2, 0xfc, 0x30, 0xfa, 
        0x20, 0xdb, 0x4c, 0xb8, 0x91, 0xb8, 0x69, 0x63, 
        0x96, 0x41, 0xc2, 0xb4, 0x60, 0xeb, 0xcd, 0xff, 
        0x3a, 0x1f, 0x94, 0xb1, 0x23, 0xcf, 0x0f, 0x49, 
        0xad, 0xd5, 0x33, 0x85, 0x71, 0xaf, 0x12, 0x87, 
        0x84, 0xef, 0xa0, 0xea, 0xe1, 0xc1, 0xd4, 0xc7, 
        0xe1, 0x21, 0x50, 0xac
    };
    /* md2 with hash oid */
    BYTE abSignatureMD2[128] = {
        0x4a, 0x4e, 0xb7, 0x5e, 0x32, 0xda, 0xdb, 0x67, 
        0x9f, 0x77, 0x84, 0x32, 0x00, 0xba, 0x5f, 0x6b, 
        0x0d, 0xcf, 0xd9, 0x99, 0xbd, 0x96, 0x31, 0xda, 
        0x23, 0x4c, 0xd9, 0x4a, 0x90, 0x84, 0x20, 0x59, 
        0x51, 0xdc, 0xd4, 0x93, 0x3a, 0xae, 0x0a, 0x0a, 
        0xa1, 0x76, 0xfa, 0xb5, 0x68, 0xee, 0xc7, 0x34, 
        0x41, 0xd3, 0xe7, 0x5a, 0x0e, 0x22, 0x61, 0x40, 
        0xea, 0x24, 0x56, 0xf1, 0x91, 0x5a, 0xf7, 0xa7, 
        0x5b, 0xf4, 0x98, 0x6b, 0xc3, 0xef, 0xad, 0xc0, 
        0x5e, 0x6b, 0x87, 0x76, 0xcb, 0x1f, 0x62, 0x06, 
        0x7c, 0xf6, 0x48, 0x97, 0x81, 0x8d, 0xef, 0x51, 
        0x51, 0xdc, 0x21, 0x91, 0x57, 0x1e, 0x79, 0x6f, 
        0x49, 0xb5, 0xde, 0x31, 0x07, 0x45, 0x99, 0x46, 
        0xc3, 0x4f, 0xca, 0x2d, 0x0e, 0x4c, 0x10, 0x25, 
        0xcb, 0x1a, 0x98, 0x63, 0x41, 0x93, 0x47, 0xc0, 
        0xb2, 0xbc, 0x10, 0x3c, 0xe7, 0xd4, 0x3c, 0x1e
    };
    /* md2 without hash oid */
    BYTE abSignatureMD2NoOID[128] = {
        0x0c, 0x21, 0x3e, 0x60, 0xf9, 0xd0, 0x36, 0x2d, 
        0xe1, 0x10, 0x45, 0x45, 0x85, 0x03, 0x29, 0x19, 
        0xef, 0x19, 0xd9, 0xa6, 0x7e, 0x9c, 0x0d, 0xbd, 
        0x03, 0x0e, 0xb9, 0x51, 0x9e, 0x74, 0x79, 0xc4, 
        0xde, 0x25, 0xf2, 0x35, 0x74, 0x55, 0xbc, 0x65, 
        0x7e, 0x33, 0x28, 0xa8, 0x1e, 0x72, 0xaa, 0x99, 
        0xdd, 0xf5, 0x26, 0x20, 0x29, 0xf8, 0xa6, 0xdf, 
        0x28, 0x4b, 0x1c, 0xdb, 0xa1, 0x41, 0x56, 0xbc, 
        0xf9, 0x9c, 0x66, 0xc0, 0x37, 0x41, 0x55, 0xa0, 
        0xe2, 0xec, 0xbf, 0x71, 0xf0, 0x5d, 0x25, 0x01, 
        0x75, 0x91, 0xe2, 0x81, 0xb2, 0x9f, 0x57, 0xa7, 
        0x5c, 0xd2, 0xfa, 0x66, 0xdb, 0x71, 0x2b, 0x1f, 
        0xad, 0x30, 0xde, 0xea, 0x49, 0x73, 0x30, 0x6a, 
        0x22, 0x54, 0x49, 0x4e, 0xae, 0xf6, 0x88, 0xc9, 
        0xff, 0x71, 0xba, 0xbf, 0x27, 0xc5, 0xfa, 0x06, 
        0xe2, 0x91, 0x71, 0x8a, 0x7e, 0x0c, 0xc2, 0x07
    };
    /* md4 with hash oid */
    BYTE abSignatureMD4[128] = {
        0x1c, 0x78, 0xaa, 0xea, 0x74, 0xf4, 0x83, 0x51, 
        0xae, 0x66, 0xe3, 0xa9, 0x1c, 0x03, 0x39, 0x1b, 
        0xac, 0x7e, 0x4e, 0x85, 0x7e, 0x1c, 0x38, 0xd2, 
        0x82, 0x43, 0xb3, 0x6f, 0x6f, 0x46, 0x45, 0x8e, 
        0x17, 0x74, 0x58, 0x29, 0xca, 0xe1, 0x03, 0x13, 
        0x45, 0x79, 0x34, 0xdf, 0x5c, 0xd6, 0xc3, 0xf9, 
        0x7a, 0x1c, 0x9d, 0xff, 0x6f, 0x03, 0x7d, 0x0f, 
        0x59, 0x1a, 0x2d, 0x0e, 0x94, 0xb4, 0x75, 0x96, 
        0xd1, 0x48, 0x63, 0x6e, 0xb2, 0xc4, 0x5c, 0xd9, 
        0xab, 0x49, 0xb4, 0x90, 0xd9, 0x57, 0x04, 0x6e, 
        0x4c, 0xb6, 0xea, 0x00, 0x94, 0x4a, 0x34, 0xa0, 
        0xd9, 0x63, 0xef, 0x2c, 0xde, 0x5b, 0xb9, 0xbe, 
        0x35, 0xc8, 0xc1, 0x31, 0xb5, 0x31, 0x15, 0x18, 
        0x90, 0x39, 0xf5, 0x2a, 0x34, 0x6d, 0xb4, 0xab, 
        0x09, 0x34, 0x69, 0x54, 0x4d, 0x11, 0x2f, 0xf3, 
        0xa2, 0x36, 0x0e, 0xa8, 0x45, 0xe7, 0x36, 0xac
    };
    /* md4 without hash oid */
    BYTE abSignatureMD4NoOID[128] = {
        0xd3, 0x60, 0xb2, 0xb0, 0x22, 0x0a, 0x99, 0xda, 
        0x04, 0x85, 0x64, 0xc6, 0xc6, 0xdb, 0x11, 0x24, 
        0xe9, 0x68, 0x2d, 0xf7, 0x09, 0xef, 0xb6, 0xa0, 
        0xa2, 0xfe, 0x45, 0xee, 0x85, 0x49, 0xcd, 0x36, 
        0xf7, 0xc7, 0x9d, 0x2b, 0x4c, 0x68, 0xda, 0x85, 
        0x8c, 0x50, 0xcc, 0x4f, 0x4b, 0xe1, 0x82, 0xc3, 
        0xbe, 0xa3, 0xf1, 0x78, 0x6b, 0x60, 0x42, 0x3f, 
        0x67, 0x22, 0x14, 0xe4, 0xe1, 0xa4, 0x6e, 0xa9, 
        0x4e, 0xf1, 0xd4, 0xb0, 0xce, 0x82, 0xac, 0x06, 
        0xba, 0x2c, 0xbc, 0xf7, 0xcb, 0xf6, 0x0c, 0x3f, 
        0xf6, 0x79, 0xfe, 0xb3, 0xd8, 0x5a, 0xbc, 0xdb, 
        0x05, 0x41, 0xa4, 0x07, 0x57, 0x9e, 0xa2, 0x96, 
        0xfc, 0x60, 0x4b, 0xf7, 0x6f, 0x86, 0x26, 0x1f, 
        0xc2, 0x2c, 0x67, 0x08, 0xcd, 0x7f, 0x91, 0xe9, 
        0x16, 0xb5, 0x0e, 0xd9, 0xc4, 0xc4, 0x97, 0xeb, 
        0x91, 0x3f, 0x20, 0x6c, 0xf0, 0x68, 0x86, 0x7f
    }; 
    /* md5 with hash oid */
    BYTE abSignatureMD5[128] = {
        0x4f, 0xe0, 0x8c, 0x9b, 0x43, 0xdd, 0x02, 0xe5, 
        0xf4, 0xa1, 0xdd, 0x88, 0x4c, 0x9c, 0x40, 0x0f, 
        0x6c, 0x43, 0x86, 0x64, 0x00, 0xe6, 0xac, 0xf7, 
        0xd0, 0x92, 0xaa, 0xc4, 0x62, 0x9a, 0x48, 0x98, 
        0x1a, 0x56, 0x6d, 0x75, 0xec, 0x04, 0x89, 0xec, 
        0x69, 0x93, 0xd6, 0x61, 0x37, 0xb2, 0x36, 0xb5, 
        0xb2, 0xba, 0xf2, 0xf5, 0x21, 0x0c, 0xf1, 0x04, 
        0xc8, 0x2d, 0xf5, 0xa0, 0x8d, 0x6d, 0x10, 0x0b, 
        0x68, 0x63, 0xf2, 0x08, 0x68, 0xdc, 0xbd, 0x95, 
        0x25, 0x7d, 0xee, 0x63, 0x5c, 0x3b, 0x98, 0x4c, 
        0xea, 0x41, 0xdc, 0x6a, 0x8b, 0x6c, 0xbb, 0x29, 
        0x2b, 0x1c, 0x5c, 0x8b, 0x7d, 0x94, 0x24, 0xa9, 
        0x7a, 0x62, 0x94, 0xf3, 0x3a, 0x6a, 0xb2, 0x4c, 
        0x33, 0x59, 0x00, 0xcd, 0x7d, 0x37, 0x79, 0x90, 
        0x31, 0xd1, 0xd9, 0x84, 0x12, 0xe5, 0x08, 0x5e, 
        0xb3, 0x60, 0x61, 0x27, 0x78, 0x37, 0x63, 0x01
    };
    /* md5 without hash oid */
    BYTE abSignatureMD5NoOID[128] = {
        0xc6, 0xad, 0x5c, 0x2b, 0x9b, 0xe0, 0x99, 0x2f, 
        0x5e, 0x55, 0x04, 0x32, 0x65, 0xe0, 0xb5, 0x75, 
        0x01, 0x9a, 0x11, 0x4d, 0x0e, 0x9a, 0xe1, 0x9f, 
        0xc7, 0xbf, 0x77, 0x6d, 0xa9, 0xfd, 0xcc, 0x9d, 
        0x8b, 0xd1, 0x31, 0xed, 0x5a, 0xd2, 0xe5, 0x5f, 
        0x42, 0x3b, 0xb5, 0x3c, 0x32, 0x30, 0x88, 0x49, 
        0xcb, 0x67, 0xb8, 0x2e, 0xc9, 0xf5, 0x2b, 0xc8, 
        0x35, 0x71, 0xb5, 0x1b, 0x32, 0x3f, 0x44, 0x4c, 
        0x66, 0x93, 0xcb, 0xe8, 0x48, 0x7c, 0x14, 0x23, 
        0xfb, 0x12, 0xa5, 0xb7, 0x86, 0x94, 0x6b, 0x19, 
        0x17, 0x20, 0xc6, 0xb8, 0x09, 0xe8, 0xbb, 0xdb, 
        0x00, 0x2b, 0x96, 0x4a, 0x93, 0x00, 0x26, 0xd3, 
        0x07, 0xa0, 0x06, 0xce, 0x5a, 0x13, 0x69, 0x6b, 
        0x62, 0x5a, 0x56, 0x61, 0x6a, 0xd8, 0x11, 0x3b, 
        0xd5, 0x67, 0xc7, 0x4d, 0xf6, 0x66, 0x63, 0xc5, 
        0xe3, 0x8f, 0x7c, 0x7c, 0xb1, 0x3e, 0x55, 0x43
    };
    /* sha with hash oid */
    BYTE abSignatureSHA[128] = {
        0x5a, 0x4c, 0x66, 0xc9, 0x30, 0x67, 0xcb, 0x91, 
        0x3c, 0x4d, 0xd5, 0x8d, 0xea, 0x4e, 0x85, 0xcd, 
        0xd9, 0x68, 0x3a, 0xf3, 0x24, 0x3c, 0x99, 0x24, 
        0x25, 0x32, 0x93, 0x3d, 0xd6, 0x2f, 0x86, 0x94, 
        0x23, 0x09, 0xee, 0x02, 0xd4, 0x15, 0xdc, 0x5f, 
        0x0e, 0x44, 0x45, 0x13, 0x5f, 0x18, 0x5d, 0x1a, 
        0xd7, 0x0b, 0xd1, 0x23, 0xd6, 0x35, 0x98, 0x52, 
        0x57, 0x45, 0x74, 0x92, 0xe3, 0x50, 0xb4, 0x20, 
        0x28, 0x2a, 0x11, 0xbf, 0x49, 0xb4, 0x2c, 0xc5, 
        0xd4, 0x1a, 0x27, 0x4e, 0xdf, 0xa0, 0xb5, 0x7a, 
        0xc8, 0x14, 0xdd, 0x9b, 0xb6, 0xca, 0xd6, 0xff, 
        0xb2, 0x6b, 0xd8, 0x98, 0x67, 0x80, 0xab, 0x53, 
        0x52, 0xbb, 0xe1, 0x2a, 0xce, 0x79, 0x2f, 0x00, 
        0x53, 0x26, 0xd8, 0xa7, 0x43, 0xca, 0x72, 0x0e, 
        0x68, 0x97, 0x37, 0x71, 0x87, 0xc2, 0x6a, 0x98, 
        0xbb, 0x6c, 0xa0, 0x01, 0xff, 0x04, 0x9d, 0xa6
    };
    /* sha without hash oid */
    BYTE abSignatureSHANoOID[128] = {
        0x86, 0xa6, 0x2b, 0x9a, 0x04, 0xda, 0x47, 0xc6, 
        0x4f, 0x97, 0x8a, 0x8a, 0xf4, 0xfa, 0x63, 0x1a, 
        0x32, 0x89, 0x56, 0x41, 0x37, 0x91, 0x15, 0x2f, 
        0x2d, 0x1c, 0x8f, 0xdc, 0x88, 0x40, 0xbb, 0x37, 
        0x3e, 0x06, 0x33, 0x1b, 0xde, 0xda, 0x7c, 0x65, 
        0x91, 0x35, 0xca, 0x45, 0x17, 0x0e, 0x24, 0xbe, 
        0x9e, 0xf6, 0x4e, 0x8a, 0xa4, 0x3e, 0xca, 0xe6, 
        0x11, 0x36, 0xb8, 0x3a, 0xf0, 0xde, 0x71, 0xfe, 
        0xdd, 0xb3, 0xcb, 0x6c, 0x39, 0xe0, 0x5f, 0x0c, 
        0x9e, 0xa8, 0x40, 0x26, 0x9c, 0x81, 0xe9, 0xc4, 
        0x15, 0x90, 0xbf, 0x4f, 0xd2, 0xc1, 0xa1, 0x80, 
        0x52, 0xfd, 0xf6, 0x3d, 0x99, 0x1b, 0x9c, 0x8a, 
        0x27, 0x1b, 0x0c, 0x9a, 0xf3, 0xf9, 0xa2, 0x00, 
        0x3e, 0x5b, 0xdf, 0xc2, 0xb4, 0x71, 0xa5, 0xbd, 
        0xf8, 0xae, 0x63, 0xbb, 0x4a, 0xc9, 0xdd, 0x67, 
        0xc1, 0x3e, 0x93, 0xee, 0xf1, 0x1f, 0x24, 0x5b
    }; 
    
    result = CryptImportKey(hProv, abPubKey, 148, 0, 0, &hPubSignKey);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptCreateHash(hProv, CALG_MD2, 0, 0, &hHash);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptHashData(hHash, abData, (DWORD)sizeof(abData), 0);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    /*check that a NULL pointer signature is correctly handled*/
    result = CryptVerifySignatureA(hHash, NULL, 128, hPubSignKey, NULL, 0);
    ok(!result && ERROR_INVALID_PARAMETER == GetLastError(),
     "Expected ERROR_INVALID_PARAMETER error, got %08x\n", GetLastError());
    if (result) return;

    /* check that we get a bad signature error when the signature is too short*/
    SetLastError(0xdeadbeef);
    result = CryptVerifySignatureA(hHash, abSignatureMD2, 64, hPubSignKey, NULL, 0);
    ok((!result && NTE_BAD_SIGNATURE == GetLastError()) ||
     broken(result), /* Win9x, WinMe, NT4 */
     "Expected NTE_BAD_SIGNATURE, got %08x\n",  GetLastError());

    result = CryptVerifySignatureA(hHash, abSignatureMD2, 128, hPubSignKey, NULL, 0);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    /* It seems that CPVerifySignature doesn't care about the OID at all. */
    result = CryptVerifySignatureA(hHash, abSignatureMD2NoOID, 128, hPubSignKey, NULL, 0);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptVerifySignatureA(hHash, abSignatureMD2NoOID, 128, hPubSignKey, NULL, CRYPT_NOHASHOID);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    CryptDestroyHash(hHash);

    result = CryptCreateHash(hProv, CALG_MD4, 0, 0, &hHash);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptHashData(hHash, abData, (DWORD)sizeof(abData), 0);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptVerifySignatureA(hHash, abSignatureMD4, 128, hPubSignKey, NULL, 0);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptVerifySignatureA(hHash, abSignatureMD4NoOID, 128, hPubSignKey, NULL, 0);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptVerifySignatureA(hHash, abSignatureMD4NoOID, 128, hPubSignKey, NULL, CRYPT_NOHASHOID);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    CryptDestroyHash(hHash);

    result = CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptHashData(hHash, abData, (DWORD)sizeof(abData), 0);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptVerifySignatureA(hHash, abSignatureMD5, 128, hPubSignKey, NULL, 0);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptVerifySignatureA(hHash, abSignatureMD5NoOID, 128, hPubSignKey, NULL, 0);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptVerifySignatureA(hHash, abSignatureMD5NoOID, 128, hPubSignKey, NULL, CRYPT_NOHASHOID);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    CryptDestroyHash(hHash);

    result = CryptCreateHash(hProv, CALG_SHA, 0, 0, &hHash);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptHashData(hHash, abData, (DWORD)sizeof(abData), 0);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptVerifySignatureA(hHash, abSignatureSHA, 128, hPubSignKey, NULL, 0);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptVerifySignatureA(hHash, abSignatureSHANoOID, 128, hPubSignKey, NULL, 0);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptVerifySignatureA(hHash, abSignatureSHANoOID, 128, hPubSignKey, NULL, CRYPT_NOHASHOID);
    ok(result, "%08x\n", GetLastError());
    if (!result) return;

    CryptDestroyHash(hHash);
    CryptDestroyKey(hPubSignKey);
}

static void test_rsa_encrypt(void)
{
    HCRYPTKEY hRSAKey;
    BYTE abData[2048] = "Wine rocks!";
    BOOL result;
    DWORD dwVal, dwLen;

    /* It is allowed to use the key exchange key for encryption/decryption */
    result = CryptGetUserKey(hProv, AT_KEYEXCHANGE, &hRSAKey);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    dwLen = 12;
    result = CryptEncrypt(hRSAKey, 0, TRUE, 0, NULL, &dwLen, (DWORD)sizeof(abData));
    if(!ENHANCED_PROV && !result && GetLastError() == NTE_BAD_KEY)
    {
        CryptDestroyKey(hRSAKey);
        return;
    }
    ok(result, "CryptEncrypt failed: %08x\n", GetLastError());
    ok(dwLen == 128, "Unexpected length %d\n", dwLen);
    dwLen = 12;
    result = CryptEncrypt(hRSAKey, 0, TRUE, 0, abData, &dwLen, (DWORD)sizeof(abData));
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptDecrypt(hRSAKey, 0, TRUE, 0, abData, &dwLen);
    ok (result && dwLen == 12 && !memcmp(abData, "Wine rocks!", 12), "%08x\n", GetLastError());
    
    dwVal = 0xdeadbeef;
    dwLen = sizeof(DWORD);
    result = CryptGetKeyParam(hRSAKey, KP_PERMISSIONS, (BYTE*)&dwVal, &dwLen, 0);
    ok(result, "%08x\n", GetLastError());
    ok(dwVal ==
        (CRYPT_MAC|CRYPT_WRITE|CRYPT_READ|CRYPT_DECRYPT|CRYPT_ENCRYPT) ||
        broken(dwVal == 0xffffffff), /* Win9x/NT4 */
        "expected CRYPT_MAC|CRYPT_WRITE|CRYPT_READ|CRYPT_DECRYPT|CRYPT_ENCRYPT,"
        " got %08x\n", dwVal);

    /* An RSA key doesn't support salt */
    result = CryptGetKeyParam(hRSAKey, KP_SALT, NULL, &dwLen, 0);
    ok(!result && (GetLastError() == NTE_BAD_KEY || GetLastError() == NTE_NOT_FOUND /* Win7 */),
       "expected NTE_BAD_KEY or NTE_NOT_FOUND, got %08x\n", GetLastError());

    /* The key exchange key's public key may be exported.. */
    result = CryptExportKey(hRSAKey, 0, PUBLICKEYBLOB, 0, NULL, &dwLen);
    ok(result, "%08x\n", GetLastError());
    /* but its private key may not be. */
    SetLastError(0xdeadbeef);
    result = CryptExportKey(hRSAKey, 0, PRIVATEKEYBLOB, 0, NULL, &dwLen);
    ok((!result && GetLastError() == NTE_BAD_KEY_STATE) ||
        broken(result), /* Win9x/NT4 */
        "expected NTE_BAD_KEY_STATE, got %08x\n", GetLastError());
    /* Setting the permissions of the key exchange key isn't allowed, either. */
    dwVal |= CRYPT_EXPORT;
    SetLastError(0xdeadbeef);
    result = CryptSetKeyParam(hRSAKey, KP_PERMISSIONS, (BYTE *)&dwVal, 0);
    ok(!result &&
        (GetLastError() == NTE_BAD_DATA || GetLastError() == NTE_BAD_FLAGS),
        "expected NTE_BAD_DATA or NTE_BAD_FLAGS, got %08x\n", GetLastError());

    CryptDestroyKey(hRSAKey);

    /* It is not allowed to use the signature key for encryption/decryption */
    result = CryptGetUserKey(hProv, AT_SIGNATURE, &hRSAKey);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    dwVal = 0xdeadbeef;
    dwLen = sizeof(DWORD);
    result = CryptGetKeyParam(hRSAKey, KP_PERMISSIONS, (BYTE*)&dwVal, &dwLen, 0);
    ok(result, "%08x\n", GetLastError());
    ok(dwVal ==
        (CRYPT_MAC|CRYPT_WRITE|CRYPT_READ|CRYPT_DECRYPT|CRYPT_ENCRYPT) ||
        broken(dwVal == 0xffffffff), /* Win9x/NT4 */
        "expected CRYPT_MAC|CRYPT_WRITE|CRYPT_READ|CRYPT_DECRYPT|CRYPT_ENCRYPT,"
        " got %08x\n", dwVal);

    /* The signature key's public key may also be exported.. */
    result = CryptExportKey(hRSAKey, 0, PUBLICKEYBLOB, 0, NULL, &dwLen);
    ok(result, "%08x\n", GetLastError());
    /* but its private key may not be. */
    SetLastError(0xdeadbeef);
    result = CryptExportKey(hRSAKey, 0, PRIVATEKEYBLOB, 0, NULL, &dwLen);
    ok((!result && GetLastError() == NTE_BAD_KEY_STATE) ||
        broken(result), /* Win9x/NT4 */
        "expected NTE_BAD_KEY_STATE, got %08x\n", GetLastError());
    /* Setting the permissions of the signature key isn't allowed, either. */
    dwVal |= CRYPT_EXPORT;
    SetLastError(0xdeadbeef);
    result = CryptSetKeyParam(hRSAKey, KP_PERMISSIONS, (BYTE *)&dwVal, 0);
    ok(!result &&
        (GetLastError() == NTE_BAD_DATA || GetLastError() == NTE_BAD_FLAGS),
        "expected NTE_BAD_DATA or NTE_BAD_FLAGS, got %08x\n", GetLastError());

    dwLen = 12;
    result = CryptEncrypt(hRSAKey, 0, TRUE, 0, abData, &dwLen, (DWORD)sizeof(abData));
    ok (!result && GetLastError() == NTE_BAD_KEY, "%08x\n", GetLastError());

    CryptDestroyKey(hRSAKey);
}

static void test_import_export(void)
{
    DWORD dwLen, dwDataLen, dwVal;
    HCRYPTKEY hPublicKey, hPrivKey;
    BOOL result;
    ALG_ID algID;
    BYTE emptyKey[2048], *exported_key;
    static BYTE abPlainPublicKey[84] = {
        0x06, 0x02, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00,
        0x52, 0x53, 0x41, 0x31, 0x00, 0x02, 0x00, 0x00,
        0x01, 0x00, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11
    };
    static BYTE priv_key_with_high_bit[] = {
        0x07, 0x02, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00,
        0x52, 0x53, 0x41, 0x32, 0x00, 0x04, 0x00, 0x00,
        0x01, 0x00, 0x01, 0x00, 0xd5, 0xa2, 0x0d, 0x66,
        0xfe, 0x65, 0xb5, 0xf1, 0xc9, 0x6b, 0xf5, 0x58,
        0x04, 0x38, 0x2d, 0xf4, 0xa3, 0x5a, 0xda, 0x9e,
        0x95, 0x81, 0x85, 0x3d, 0x01, 0x07, 0xb2, 0x03,
        0x77, 0x70, 0x79, 0x6e, 0x6c, 0x26, 0x42, 0xa4,
        0x12, 0xfd, 0xaa, 0x29, 0x83, 0x04, 0xce, 0x91,
        0x90, 0x39, 0x5e, 0x49, 0x56, 0xfd, 0x0a, 0xe5,
        0xb1, 0xea, 0x3b, 0xb2, 0x70, 0xb0, 0x20, 0xc1,
        0x1f, 0x22, 0x07, 0x3e, 0x4d, 0xc0, 0x73, 0xfd,
        0x92, 0x8f, 0x87, 0xd8, 0xd1, 0xd1, 0x28, 0xd8,
        0x19, 0xd1, 0x93, 0x83, 0xe0, 0xb8, 0x9f, 0x53,
        0xf4, 0x6a, 0x7c, 0xcb, 0x10, 0x53, 0xd0, 0x37,
        0x02, 0xb4, 0xa5, 0xf7, 0xa2, 0x28, 0x6e, 0x26,
        0xef, 0x5c, 0x14, 0x01, 0x40, 0x1e, 0xa3, 0xe1,
        0xda, 0x76, 0xd0, 0x12, 0x84, 0xb7, 0x48, 0x7d,
        0xc8, 0x67, 0x5c, 0xb2, 0xd5, 0x2e, 0xaf, 0x8e,
        0x7d, 0x32, 0x59, 0x92, 0x01, 0xd6, 0x5b, 0x68,
        0x28, 0x9b, 0xb1, 0x6c, 0x69, 0xeb, 0x61, 0x5b,
        0x4b, 0x13, 0xe2, 0xbd, 0x7d, 0xbe, 0xce, 0xe8,
        0x41, 0x54, 0xca, 0xa8, 0xdd, 0xc7, 0xfe, 0x8b,
        0xdf, 0xf6, 0x55, 0x6c, 0x50, 0x11, 0xc8, 0x15,
        0x13, 0x42, 0x59, 0x9f, 0xbb, 0xea, 0x73, 0x78,
        0x7b, 0x22, 0x8d, 0x96, 0x62, 0xe5, 0xda, 0xa2,
        0x85, 0x5c, 0x20, 0x74, 0x9f, 0x1c, 0x12, 0xf2,
        0x48, 0x06, 0x1a, 0xc6, 0xd5, 0x94, 0xec, 0x31,
        0x6b, 0xb6, 0x7b, 0x54, 0x61, 0x77, 0xec, 0x7c,
        0x6f, 0xb7, 0x55, 0x3d, 0x6b, 0x98, 0x05, 0xd7,
        0x8a, 0x73, 0x25, 0xf2, 0x8f, 0xe4, 0xb8, 0x8d,
        0x27, 0x18, 0x0d, 0x05, 0xba, 0x23, 0x54, 0x37,
        0x10, 0xf0, 0x1c, 0x41, 0xa6, 0xae, 0x4c, 0x2a,
        0x6a, 0x2f, 0x7f, 0x68, 0x43, 0x86, 0xe7, 0x9c,
        0xfd, 0x9e, 0xf1, 0xfe, 0x84, 0xe3, 0xb6, 0x99,
        0x51, 0xfe, 0x1e, 0xbd, 0x01, 0xc6, 0x10, 0xef,
        0x88, 0xa4, 0xd8, 0x53, 0x14, 0x88, 0x15, 0xc9,
        0xe5, 0x86, 0xe2, 0x8d, 0x85, 0x2e, 0x0d, 0xec,
        0x15, 0xa7, 0x48, 0xfa, 0x18, 0xfb, 0x01, 0x8d,
        0x2b, 0x90, 0x70, 0x7f, 0x78, 0xb1, 0x33, 0x7e,
        0xfe, 0x82, 0x40, 0x5f, 0x4a, 0x97, 0xc2, 0x42,
        0x22, 0xd5, 0x5f, 0xbc, 0xbd, 0xab, 0x26, 0x98,
        0xcd, 0xb5, 0xdf, 0x7e, 0xa0, 0x68, 0xa7, 0x12,
        0x9e, 0xa5, 0xa2, 0x90, 0x85, 0xc5, 0xca, 0x73,
        0x4a, 0x59, 0x8a, 0xec, 0xcf, 0xdd, 0x65, 0x5d,
        0xc1, 0xaa, 0x86, 0x53, 0xd5, 0xde, 0xbb, 0x23,
        0x24, 0xb8, 0x9b, 0x74, 0x03, 0x20, 0xb4, 0xf0,
        0xe4, 0xdd, 0xd2, 0x03, 0xfd, 0x67, 0x55, 0x19,
        0x28, 0x1d, 0xc1, 0xb8, 0xa5, 0x89, 0x0e, 0xc0,
        0x80, 0x9d, 0xdd, 0xda, 0x9d, 0x30, 0x5c, 0xc8,
        0xbb, 0xfe, 0x8f, 0xce, 0xd5, 0xf6, 0xdf, 0xfa,
        0x14, 0xaf, 0xe4, 0xba, 0xb0, 0x84, 0x45, 0xd8,
        0x67, 0xa7, 0xd0, 0xce, 0x89, 0x2a, 0x30, 0x8c,
        0xfa, 0xe9, 0x65, 0xa4, 0x21, 0x2d, 0x6b, 0xa2,
        0x9b, 0x8f, 0x92, 0xbd, 0x3a, 0x10, 0x71, 0x12,
        0xc2, 0x02, 0x3d, 0xd5, 0x83, 0x1d, 0xfa, 0x42,
        0xb7, 0x48, 0x1b, 0x31, 0xe3, 0x82, 0x90, 0x2d,
        0x91, 0x59, 0xf9, 0x38, 0x52, 0xe5, 0xdb, 0xc1,
        0x4d, 0x3a, 0xe6, 0x9b, 0x6a, 0xbb, 0xea, 0xa4,
        0x8d, 0x5e, 0xc4, 0x00, 0x01, 0xb8, 0xec, 0x91,
        0xc1, 0xdb, 0x63, 0xbd, 0x57, 0xb6, 0x26, 0x15,
        0xb6, 0x3e, 0xa2, 0xdf, 0x62, 0x8d, 0xa8, 0xbe,
        0xe1, 0xf1, 0x39, 0xbd, 0x18, 0xd2, 0x6f, 0xd7,
        0xda, 0xdc, 0x71, 0x30, 0xf1, 0x21, 0x71, 0xa4,
        0x08, 0x43, 0x46, 0xdf, 0x50, 0xbd, 0x3c, 0x60,
        0x5b, 0x63, 0x35, 0xe3, 0x37, 0x5b, 0x25, 0x17,
        0x54, 0x5e, 0x68, 0x60, 0xb6, 0x49, 0xef, 0x6e,
        0x09, 0xef, 0xda, 0x90, 0x3e, 0xd4, 0x09, 0x33,
        0x36, 0x57, 0x9a, 0x14, 0xbd, 0xf7, 0xb1, 0x98,
        0x30, 0x42, 0x03, 0x84, 0x61, 0xeb, 0x8e, 0x50,
        0xdc, 0x6a, 0x93, 0x1b, 0x32, 0x51, 0xf9, 0xc6,
        0xc2, 0x19, 0xb3, 0x5d, 0xe2, 0xf8, 0xc5, 0x8f,
        0x68, 0xaa, 0x1d, 0xdb, 0xd3, 0x7f, 0x8d, 0x98,
        0x9c, 0x16, 0x8c, 0xc3, 0xcd, 0xd9, 0xdb, 0x08,
        0xe6, 0x36, 0x60, 0xb6, 0x36, 0xdc, 0x1d, 0x59,
        0xb6, 0x5f, 0x01, 0x5e
    };
    static const BYTE expected_exported_priv_key[] = {
        0x07, 0x02, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00,
        0x52, 0x53, 0x41, 0x32, 0x00, 0x04, 0x00, 0x00,
        0x01, 0x00, 0x01, 0x00, 0xd5, 0xa2, 0x0d, 0x66,
        0xfe, 0x65, 0xb5, 0xf1, 0xc9, 0x6b, 0xf5, 0x58,
        0x04, 0x38, 0x2d, 0xf4, 0xa3, 0x5a, 0xda, 0x9e,
        0x95, 0x81, 0x85, 0x3d, 0x01, 0x07, 0xb2, 0x03,
        0x77, 0x70, 0x79, 0x6e, 0x6c, 0x26, 0x42, 0xa4,
        0x12, 0xfd, 0xaa, 0x29, 0x83, 0x04, 0xce, 0x91,
        0x90, 0x39, 0x5e, 0x49, 0x56, 0xfd, 0x0a, 0xe5,
        0xb1, 0xea, 0x3b, 0xb2, 0x70, 0xb0, 0x20, 0xc1,
        0x1f, 0x22, 0x07, 0x3e, 0x4d, 0xc0, 0x73, 0xfd,
        0x92, 0x8f, 0x87, 0xd8, 0xd1, 0xd1, 0x28, 0xd8,
        0x19, 0xd1, 0x93, 0x83, 0xe0, 0xb8, 0x9f, 0x53,
        0xf4, 0x6a, 0x7c, 0xcb, 0x10, 0x53, 0xd0, 0x37,
        0x02, 0xb4, 0xa5, 0xf7, 0xa2, 0x28, 0x6e, 0x26,
        0xef, 0x5c, 0x14, 0x01, 0x40, 0x1e, 0xa3, 0xe1,
        0xda, 0x76, 0xd0, 0x12, 0x84, 0xb7, 0x48, 0x7d,
        0xc8, 0x67, 0x5c, 0xb2, 0xd5, 0x2e, 0xaf, 0x8e,
        0x7d, 0x32, 0x59, 0x92, 0x01, 0xd6, 0x5b, 0x68,
        0x28, 0x9b, 0xb1, 0x6c, 0x69, 0xeb, 0x61, 0x5b,
        0x4b, 0x13, 0xe2, 0xbd, 0x7d, 0xbe, 0xce, 0xe8,
        0x41, 0x54, 0xca, 0xa8, 0xdd, 0xc7, 0xfe, 0x8b,
        0xdf, 0xf6, 0x55, 0x6c, 0x50, 0x11, 0xc8, 0x15,
        0x13, 0x42, 0x59, 0x9f, 0xbb, 0xea, 0x73, 0x78,
        0x7b, 0x22, 0x8d, 0x96, 0x62, 0xe5, 0xda, 0xa2,
        0x85, 0x5c, 0x20, 0x74, 0x9f, 0x1c, 0x12, 0xf2,
        0x48, 0x06, 0x1a, 0xc6, 0xd5, 0x94, 0xec, 0x31,
        0x6b, 0xb6, 0x7b, 0x54, 0x61, 0x77, 0xec, 0x7c,
        0x6f, 0xb7, 0x55, 0x3d, 0x6b, 0x98, 0x05, 0xd7,
        0x8a, 0x73, 0x25, 0xf2, 0x8f, 0xe4, 0xb8, 0x8d,
        0x27, 0x18, 0x0d, 0x05, 0xba, 0x23, 0x54, 0x37,
        0x10, 0xf0, 0x1c, 0x41, 0xa6, 0xae, 0x4c, 0x2a,
        0x6a, 0x2f, 0x7f, 0x68, 0x43, 0x86, 0xe7, 0x9c,
        0xfd, 0x9e, 0xf1, 0xfe, 0x84, 0xe3, 0xb6, 0x99,
        0x51, 0xfe, 0x1e, 0xbd, 0x01, 0xc6, 0x10, 0xef,
        0x88, 0xa4, 0xd8, 0x53, 0x14, 0x88, 0x15, 0xc9,
        0xe5, 0x86, 0xe2, 0x8d, 0x85, 0x2e, 0x0d, 0xec,
        0x15, 0xa7, 0x48, 0xfa, 0x18, 0xfb, 0x01, 0x8d,
        0x2b, 0x90, 0x70, 0x7f, 0x78, 0xb1, 0x33, 0x7e,
        0xfe, 0x82, 0x40, 0x5f, 0x4a, 0x97, 0xc2, 0x42,
        0x22, 0xd5, 0x5f, 0xbc, 0xbd, 0xab, 0x26, 0x98,
        0xcd, 0xb5, 0xdf, 0x7e, 0xa0, 0x68, 0xa7, 0x12,
        0x9e, 0xa5, 0xa2, 0x90, 0x85, 0xc5, 0xca, 0x73,
        0x4a, 0x59, 0x8a, 0xec, 0xcf, 0xdd, 0x65, 0x5d,
        0xc1, 0xaa, 0x86, 0x53, 0xd5, 0xde, 0xbb, 0x23,
        0x24, 0xb8, 0x9b, 0x74, 0x03, 0x20, 0xb4, 0xf0,
        0xe4, 0xdd, 0xd2, 0x03, 0xfd, 0x67, 0x55, 0x19,
        0x28, 0x1d, 0xc1, 0xb8, 0xa5, 0x89, 0x0e, 0xc0,
        0x80, 0x9d, 0xdd, 0xda, 0x9d, 0x30, 0x5c, 0xc8,
        0xbb, 0xfe, 0x8f, 0xce, 0xd5, 0xf6, 0xdf, 0xfa,
        0x14, 0xaf, 0xe4, 0xba, 0xb0, 0x84, 0x45, 0xd8,
        0x67, 0xa7, 0xd0, 0xce, 0x89, 0x2a, 0x30, 0x8c,
        0xfa, 0xe9, 0x65, 0xa4, 0x21, 0x2d, 0x6b, 0xa2,
        0x9b, 0x8f, 0x92, 0xbd, 0x3a, 0x10, 0x71, 0x12,
        0xc2, 0x02, 0x3d, 0xd5, 0x83, 0x1d, 0xfa, 0x42,
        0xb7, 0x48, 0x1b, 0x31, 0xe3, 0x82, 0x90, 0x2d,
        0x91, 0x59, 0xf9, 0x38, 0x52, 0xe5, 0xdb, 0xc1,
        0x4d, 0x3a, 0xe6, 0x9b, 0x6a, 0xbb, 0xea, 0xa4,
        0x8d, 0x5e, 0xc4, 0x00, 0x01, 0xb8, 0xec, 0x91,
        0xc1, 0xdb, 0x63, 0xbd, 0x57, 0xb6, 0x26, 0x15,
        0xb6, 0x3e, 0xa2, 0xdf, 0x62, 0x8d, 0xa8, 0xbe,
        0xe1, 0xf1, 0x39, 0xbd, 0x18, 0xd2, 0x6f, 0xd7,
        0xda, 0xdc, 0x71, 0x30, 0xf1, 0x21, 0x71, 0xa4,
        0x08, 0x43, 0x46, 0xdf, 0x50, 0xbd, 0x3c, 0x60,
        0x5b, 0x63, 0x35, 0xe3, 0x37, 0x5b, 0x25, 0x17,
        0x54, 0x5e, 0x68, 0x60, 0xb6, 0x49, 0xef, 0x6e,
        0x09, 0xef, 0xda, 0x90, 0x3e, 0xd4, 0x09, 0x33,
        0x36, 0x57, 0x9a, 0x14, 0xbd, 0xf7, 0xb1, 0x98,
        0x30, 0x42, 0x03, 0x84, 0x61, 0xeb, 0x8e, 0x50,
        0xdc, 0x6a, 0x93, 0x1b, 0x32, 0x51, 0xf9, 0xc6,
        0xc2, 0x19, 0xb3, 0x5d, 0xe2, 0xf8, 0xc5, 0x8f,
        0x68, 0xaa, 0x1d, 0xdb, 0xd3, 0x7f, 0x8d, 0x98,
        0x9c, 0x16, 0x8c, 0xc3, 0xcd, 0xd9, 0xdb, 0x08,
        0xe6, 0x36, 0x60, 0xb6, 0x36, 0xdc, 0x1d, 0x59,
        0xb6, 0x5f, 0x01, 0x5e
    };

    dwLen=84;
    result = CryptImportKey(hProv, abPlainPublicKey, dwLen, 0, 0, &hPublicKey);
    ok(result, "failed to import the public key\n");

    dwDataLen=sizeof(algID);
    result = CryptGetKeyParam(hPublicKey, KP_ALGID, (LPBYTE)&algID, &dwDataLen, 0);
    ok(result, "failed to get the KP_ALGID from the imported public key\n");
    ok(algID == CALG_RSA_KEYX, "Expected CALG_RSA_KEYX, got %x\n", algID);
        
    dwVal = 0xdeadbeef;
    dwDataLen = sizeof(DWORD);
    result = CryptGetKeyParam(hPublicKey, KP_PERMISSIONS, (BYTE*)&dwVal, &dwDataLen, 0);
    ok(result, "%08x\n", GetLastError());
    ok(dwVal ==
        (CRYPT_MAC|CRYPT_WRITE|CRYPT_READ|CRYPT_DECRYPT|CRYPT_ENCRYPT) ||
        broken(dwVal == 0xffffffff), /* Win9x/NT4 */
        "expected CRYPT_MAC|CRYPT_WRITE|CRYPT_READ|CRYPT_DECRYPT|CRYPT_ENCRYPT,"
        " got %08x\n", dwVal);
    result = CryptExportKey(hPublicKey, 0, PUBLICKEYBLOB, 0, emptyKey, &dwLen);
    ok(result, "failed to export the fresh imported public key\n");
    ok(dwLen == 84, "Expected exported key to be 84 bytes long but got %d bytes.\n",dwLen);
    ok(!memcmp(emptyKey, abPlainPublicKey, dwLen), "exported key is different from the imported key\n");

    CryptDestroyKey(hPublicKey);

    result = CryptImportKey(hProv, priv_key_with_high_bit,
        sizeof(priv_key_with_high_bit), 0, CRYPT_EXPORTABLE, &hPrivKey);
    ok(result, "CryptImportKey failed: %08x\n", GetLastError());

    result = CryptExportKey(hPrivKey, 0, PRIVATEKEYBLOB, 0, NULL, &dwDataLen);
    ok(result, "CryptExportKey failed: %08x\n", GetLastError());
    exported_key = HeapAlloc(GetProcessHeap(), 0, dwDataLen);
    result = CryptExportKey(hPrivKey, 0, PRIVATEKEYBLOB, 0, exported_key,
        &dwDataLen);
    ok(result, "CryptExportKey failed: %08x\n", GetLastError());

    ok(dwDataLen == sizeof(expected_exported_priv_key), "unexpected size %d\n",
        dwDataLen);
    ok(!memcmp(exported_key, expected_exported_priv_key, dwDataLen),
        "unexpected value\n");

    HeapFree(GetProcessHeap(), 0, exported_key);

    CryptDestroyKey(hPrivKey);
}

static void test_import_hmac(void)
{
    /* Test cases from RFC 2202, section 3 */
    static const struct rfc2202_test_case {
        const char *key;
        DWORD key_len;
        const char *data;
        const DWORD data_len;
        const char *digest;
    } cases[] = {
        { "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b"
          "\x0b\x0b\x0b\x0b", 20,
          "Hi There", 8,
          "\xb6\x17\x31\x86\x55\x05\x72\x64\xe2\x8b\xc0\xb6\xfb\x37\x8c\x8e"
          "\xf1\x46\xbe\x00" },
        { "Jefe", 4,
          "what do ya want for nothing?", 28,
          "\xef\xfc\xdf\x6a\xe5\xeb\x2f\xa2\xd2\x74\x16\xd5\xf1\x84\xdf\x9c"
          "\x25\x9a\x7c\x79" },
        { "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
          "\xaa\xaa\xaa\xaa", 20,
          "\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
          "\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
          "\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
          "\xdd\xdd", 50,
          "\x12\x5d\x73\x42\xb9\xac\x11\xcd\x91\xa3\x9a\xf4\x8a\xa1\x7b\x4f"
          "\x63\xf1\x75\xd3" },
        { "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10"
          "\x11\x12\x13\x14\x15\x16\x17\x18\x19", 25,
          "\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
          "\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
          "\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
          "\xcd\xcd", 50,
          "\x4c\x90\x07\xF4\x02\x62\x50\xc6\xbc\x84\x14\xf9\xbf\x50\xc8\x6c"
          "\x2d\x72\x35\xda" },
        { "\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c"
          "\x0c\x0c\x0c\x0c", 20,
          "Test With Truncation", 20,
          "\x4c\x1a\x03\x42\x4b\x55\xe0\x7f\xe7\xf2\x7b\xe1\xd5\x8b\xb9\x32"
          "\x4a\x9a\x5a\x04" },
        { "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
          "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
          "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
          "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
          "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa",
          80,
          "Test Using Larger Than Block-Size Key - Hash Key First", 54,
          "\xaa\x4a\xe5\xe1\x52\x72\xd0\x0e\x95\x70\x56\x37\xce\x8a\x3b\x55"
          "\xed\x40\x21\x12" },
        { "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
          "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
          "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
          "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
          "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa",
          80,
          "Test Using Larger Than Block-Size Key and Larger "
          "Than One Block-Size Data", 73,
          "\xe8\xe9\x9D\x0f\x45\x23\x7d\x78\x6d\x6b\xba\xa7\x96\x5c\x78\x08"
          "\xbb\xff\x1a\x91" }
    };
    DWORD i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        const struct rfc2202_test_case *test_case = &cases[i];
        DWORD size = sizeof(BLOBHEADER) + sizeof(DWORD) + test_case->key_len;
        BYTE *blob = HeapAlloc(GetProcessHeap(), 0, size);

        if (blob)
        {
            BLOBHEADER *header = (BLOBHEADER *)blob;
            DWORD *key_len = (DWORD *)(header + 1);
            BYTE *key_bytes = (BYTE *)(key_len + 1);
            BOOL result;
            HCRYPTKEY key;

            header->bType = PLAINTEXTKEYBLOB;
            header->bVersion = CUR_BLOB_VERSION;
            header->reserved = 0;
            header->aiKeyAlg = CALG_RC2;
            *key_len = test_case->key_len;
            memcpy(key_bytes, test_case->key, *key_len);
            result = CryptImportKey(hProv, blob, size, 0, CRYPT_IPSEC_HMAC_KEY, &key);
            ok(result || broken(GetLastError() == NTE_BAD_FLAGS /* Win2k */), "CryptImportKey failed on test case %d: %08x\n", i, GetLastError());
            if (result)
            {
                HCRYPTHASH hash;
                HMAC_INFO hmac_info = { CALG_SHA1, 0 };
                BYTE digest[20];
                DWORD digest_size;

                result = CryptCreateHash(hProv, CALG_HMAC, key, 0, &hash);
                ok(result, "CryptCreateHash failed on test case %d: %08x\n", i, GetLastError());
                result = CryptSetHashParam(hash, HP_HMAC_INFO, (BYTE *)&hmac_info, 0);
                ok(result, "CryptSetHashParam failed on test case %d: %08x\n", i, GetLastError());
                result = CryptHashData(hash, (const BYTE *)test_case->data, test_case->data_len, 0);
                ok(result, "CryptHashData failed on test case %d: %08x\n", i, GetLastError());
                digest_size = sizeof(digest);
                result = CryptGetHashParam(hash, HP_HASHVAL, digest, &digest_size, 0);
                ok(result, "CryptGetHashParam failed on test case %d: %08x\n", i, GetLastError());
                ok(!memcmp(digest, test_case->digest, sizeof(digest)), "Unexpected value on test case %d\n", i);
                CryptDestroyHash(hash);
                CryptDestroyKey(key);
            }
            HeapFree(GetProcessHeap(), 0, blob);
        }
    }
}
        
static void test_schannel_provider(void)
{
    HCRYPTPROV hProv;
    HCRYPTKEY hRSAKey, hMasterSecret, hServerWriteKey, hServerWriteMACKey;
    HCRYPTHASH hMasterHash, hTLS1PRF, hHMAC;
    BOOL result;
    DWORD dwLen;
    SCHANNEL_ALG saSChannelAlg;
    CRYPT_DATA_BLOB data_blob;
    HMAC_INFO hmacInfo = { CALG_MD5, NULL, 0, NULL, 0 };
    BYTE abTLS1Master[140] = {
        0x01, 0x02, 0x00, 0x00, 0x06, 0x4c, 0x00, 0x00, 
        0x00, 0xa4, 0x00, 0x00, 0x5b, 0x13, 0xc7, 0x68, 
        0xd8, 0x55, 0x23, 0x5d, 0xbc, 0xa6, 0x9d, 0x97, 
        0x0e, 0xcd, 0x6b, 0xcf, 0xc0, 0xdc, 0xc5, 0x53, 
        0x28, 0xa0, 0xca, 0xc1, 0x63, 0x4e, 0x3a, 0x24, 
        0x22, 0xe5, 0x4d, 0x15, 0xbb, 0xa5, 0x06, 0xc3, 
        0x98, 0x25, 0xdc, 0x35, 0xd3, 0xdb, 0xab, 0xb8, 
        0x44, 0x1b, 0xfe, 0x63, 0x88, 0x7c, 0x2e, 0x6d, 
        0x34, 0xd9, 0x0f, 0x7e, 0x2f, 0xc2, 0xb2, 0x6e, 
        0x56, 0xfa, 0xab, 0xb2, 0x88, 0xf6, 0x15, 0x6e, 
        0xa8, 0xcd, 0x70, 0x16, 0x94, 0x61, 0x07, 0x40, 
        0x9e, 0x25, 0x22, 0xf8, 0x64, 0x9f, 0xcc, 0x0b, 
        0xf1, 0x92, 0x4d, 0xfe, 0xc3, 0x5d, 0x52, 0xdb, 
        0x0f, 0xff, 0x12, 0x0f, 0x49, 0x43, 0x7d, 0xc6, 
        0x52, 0x61, 0xb0, 0x06, 0xc8, 0x1b, 0x90, 0xac, 
        0x09, 0x7e, 0x4b, 0x95, 0x69, 0x3b, 0x0d, 0x41, 
        0x1b, 0x4c, 0x65, 0x75, 0x4d, 0x85, 0x16, 0xc4, 
        0xd3, 0x1e, 0x82, 0xb3
    };
    BYTE abServerSecret[33] = "Super Secret Server Secret 12345";
    BYTE abClientSecret[33] = "Super Secret Client Secret 12345";
    BYTE abHashedHandshakes[37] = "123456789012345678901234567890123456";
    BYTE abClientFinished[16] = "client finished";
    BYTE abData[16] = "Wine rocks!";
    BYTE abMD5Hash[16];
    static const BYTE abEncryptedData[16] = {
        0x13, 0xd2, 0xdd, 0xeb, 0x6c, 0x3f, 0xbe, 0xb2,
        0x04, 0x86, 0xb5, 0xe5, 0x08, 0xe5, 0xf3, 0x0d    
    };
    static const BYTE abPRF[16] = {
        0xa8, 0xb2, 0xa6, 0xef, 0x83, 0x4e, 0x74, 0xb1,
        0xf3, 0xb1, 0x51, 0x5a, 0x1a, 0x2b, 0x11, 0x31
    };
    static const BYTE abMD5[16] = {
        0xe1, 0x65, 0x3f, 0xdb, 0xbb, 0x3d, 0x99, 0x3c,
        0x3d, 0xca, 0x6a, 0x6f, 0xfa, 0x15, 0x4e, 0xaa
    };
    
    result = CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_SCHANNEL, CRYPT_VERIFYCONTEXT|CRYPT_NEWKEYSET);
    if (!result)
    {
        win_skip("no PROV_RSA_SCHANNEL support\n");
        return;
    }
    ok (result, "%08x\n", GetLastError());
    if (result)
        CryptReleaseContext(hProv, 0);

    result = CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_SCHANNEL, CRYPT_VERIFYCONTEXT);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;
    
    /* To get deterministic results, we import the TLS1 master secret (which
     * is typically generated from a random generator). Therefore, we need
     * an RSA key. */
    dwLen = (DWORD)sizeof(abPlainPrivateKey);
    result = CryptImportKey(hProv, abPlainPrivateKey, dwLen, 0, 0, &hRSAKey);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    dwLen = (DWORD)sizeof(abTLS1Master);
    result = CryptImportKey(hProv, abTLS1Master, dwLen, hRSAKey, 0, &hMasterSecret);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;    

    /* Deriving a hash from the master secret. This is due to the CryptoAPI architecture.
     * (Keys can only be derived from hashes, not from other keys.)
     * The hash can't be created yet because the key doesn't have the client
     * random or server random set.
     */
    result = CryptCreateHash(hProv, CALG_SCHANNEL_MASTER_HASH, hMasterSecret, 0, &hMasterHash);
    ok (!result && GetLastError() == ERROR_INVALID_PARAMETER,
        "expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());

    /* Setting the TLS1 client and server random parameters, as well as the 
     * MAC and encryption algorithm parameters. */
    data_blob.cbData = 33;
    data_blob.pbData = abClientSecret;
    result = CryptSetKeyParam(hMasterSecret, KP_CLIENT_RANDOM, (BYTE*)&data_blob, 0);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    data_blob.cbData = 33;
    data_blob.pbData = abServerSecret;
    result = CryptSetKeyParam(hMasterSecret, KP_SERVER_RANDOM, (BYTE*)&data_blob, 0);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptCreateHash(hProv, CALG_SCHANNEL_MASTER_HASH, hMasterSecret, 0, &hMasterHash);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    /* Deriving the server write encryption key from the master hash can't
     * succeed before the encryption key algorithm is set.
     */
    result = CryptDeriveKey(hProv, CALG_SCHANNEL_ENC_KEY, hMasterHash, CRYPT_SERVER, &hServerWriteKey);
    ok (!result && GetLastError() == NTE_BAD_FLAGS,
        "expected NTE_BAD_FLAGS, got %08x\n", GetLastError());

    CryptDestroyHash(hMasterHash);

    saSChannelAlg.dwUse = SCHANNEL_ENC_KEY;
    saSChannelAlg.Algid = CALG_DES;
    saSChannelAlg.cBits = 64;
    saSChannelAlg.dwFlags = 0;
    saSChannelAlg.dwReserved = 0;
    result = CryptSetKeyParam(hMasterSecret, KP_SCHANNEL_ALG, (PBYTE)&saSChannelAlg, 0);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    saSChannelAlg.dwUse = SCHANNEL_MAC_KEY;
    saSChannelAlg.Algid = CALG_MD5;
    saSChannelAlg.cBits = 128;
    saSChannelAlg.dwFlags = 0;
    saSChannelAlg.dwReserved = 0;
    result = CryptSetKeyParam(hMasterSecret, KP_SCHANNEL_ALG, (PBYTE)&saSChannelAlg, 0);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptCreateHash(hProv, CALG_SCHANNEL_MASTER_HASH, hMasterSecret, 0, &hMasterHash);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    /* Deriving the server write encryption key from the master hash */
    result = CryptDeriveKey(hProv, CALG_SCHANNEL_ENC_KEY, hMasterHash, CRYPT_SERVER, &hServerWriteKey);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    /* Encrypting some data with the server write encryption key and checking the result. */
    dwLen = 12;
    result = CryptEncrypt(hServerWriteKey, 0, TRUE, 0, abData, &dwLen, 16);
    ok (result && (dwLen == 16) && !memcmp(abData, abEncryptedData, 16), "%08x\n", GetLastError());

    /* Second test case: Test the TLS1 pseudo random number function. */
    result = CryptCreateHash(hProv, CALG_TLS1PRF, hMasterSecret, 0, &hTLS1PRF);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    /* Set the label and seed parameters for the random number function */
    data_blob.cbData = 36;
    data_blob.pbData = abHashedHandshakes;
    result = CryptSetHashParam(hTLS1PRF, HP_TLS1PRF_SEED, (BYTE*)&data_blob, 0);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    data_blob.cbData = 15;
    data_blob.pbData = abClientFinished;
    result = CryptSetHashParam(hTLS1PRF, HP_TLS1PRF_LABEL, (BYTE*)&data_blob, 0);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    /* Generate some pseudo random bytes and check if they are correct. */
    dwLen = (DWORD)sizeof(abData);
    result = CryptGetHashParam(hTLS1PRF, HP_HASHVAL, abData, &dwLen, 0);
    ok (result && (dwLen==(DWORD)sizeof(abData)) && !memcmp(abData, abPRF, sizeof(abData)), 
        "%08x\n", GetLastError());

    /* Third test case. Derive the server write mac key. Derive an HMAC object from this one.
     * Hash some data with the HMAC. Compare results. */
    result = CryptDeriveKey(hProv, CALG_SCHANNEL_MAC_KEY, hMasterHash, CRYPT_SERVER, &hServerWriteMACKey);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;
    
    result = CryptCreateHash(hProv, CALG_HMAC, hServerWriteMACKey, 0, &hHMAC);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptSetHashParam(hHMAC, HP_HMAC_INFO, (PBYTE)&hmacInfo, 0);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    result = CryptHashData(hHMAC, abData, (DWORD)sizeof(abData), 0);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    dwLen = (DWORD)sizeof(abMD5Hash);
    result = CryptGetHashParam(hHMAC, HP_HASHVAL, abMD5Hash, &dwLen, 0);
    ok (result && (dwLen == 16) && !memcmp(abMD5Hash, abMD5, 16), "%08x\n", GetLastError());

    CryptDestroyHash(hHMAC);
    CryptDestroyHash(hTLS1PRF);
    CryptDestroyHash(hMasterHash);
    CryptDestroyKey(hServerWriteMACKey);
    CryptDestroyKey(hServerWriteKey);
    CryptDestroyKey(hRSAKey);
    CryptDestroyKey(hMasterSecret);
    CryptReleaseContext(hProv, 0);
    CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_SCHANNEL, CRYPT_DELETEKEYSET);
}

/* Test that a key can be used to encrypt data and exported, and that, when
 * the exported key is imported again, can be used to decrypt the original
 * data again.
 */
static void test_rsa_round_trip(void)
{
    static const char test_string[] = "Well this is a fine how-do-you-do.";
    HCRYPTPROV prov;
    HCRYPTKEY signKey, keyExchangeKey;
    BOOL result;
    BYTE data[256], *exportedKey;
    DWORD dataLen, keyLen;

    CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);

    /* Generate a new key... */
    result = CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_NEWKEYSET);
    ok(result, "CryptAcquireContextA failed: %08x\n", GetLastError());
    result = CryptGenKey(prov, CALG_RSA_KEYX, CRYPT_EXPORTABLE, &signKey);
    ok(result, "CryptGenKey with CALG_RSA_KEYX failed with error %08x\n", GetLastError());
    result = CryptGetUserKey(prov, AT_KEYEXCHANGE, &keyExchangeKey);
    ok(result, "CryptGetUserKey failed: %08x\n", GetLastError());
    /* encrypt some data with it... */
    memcpy(data, test_string, strlen(test_string) + 1);
    dataLen = strlen(test_string) + 1;
    result = CryptEncrypt(keyExchangeKey, 0, TRUE, 0, data, &dataLen,
                          sizeof(data));
    ok(result || broken(GetLastError() == NTE_BAD_KEY /* Win9x/2000 */) ||
       broken(GetLastError() == NTE_PERM /* NT4 */),
       "CryptEncrypt failed: %08x\n", GetLastError());
    /* export the key... */
    result = CryptExportKey(keyExchangeKey, 0, PRIVATEKEYBLOB, 0, NULL,
                            &keyLen);
    ok(result, "CryptExportKey failed: %08x\n", GetLastError());
    exportedKey = HeapAlloc(GetProcessHeap(), 0, keyLen);
    result = CryptExportKey(keyExchangeKey, 0, PRIVATEKEYBLOB, 0, exportedKey,
                            &keyLen);
    ok(result, "CryptExportKey failed: %08x\n", GetLastError());
    /* destroy the key... */
    CryptDestroyKey(keyExchangeKey);
    CryptDestroyKey(signKey);
    /* import the key again... */
    result = CryptImportKey(prov, exportedKey, keyLen, 0, 0, &keyExchangeKey);
    ok(result, "CryptImportKey failed: %08x\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, exportedKey);
    /* and decrypt the data encrypted with the original key with the imported
     * key.
     */
    result = CryptDecrypt(keyExchangeKey, 0, TRUE, 0, data, &dataLen);
    ok(result || broken(GetLastError() == NTE_BAD_KEY /* Win9x/2000 */) ||
       broken(GetLastError() == NTE_PERM /* NT4 */),
       "CryptDecrypt failed: %08x\n", GetLastError());
    if (result)
    {
        ok(dataLen == sizeof(test_string), "unexpected size %d\n", dataLen);
        ok(!memcmp(data, test_string, sizeof(test_string)), "unexpected value\n");
    }
    CryptDestroyKey(keyExchangeKey);
    CryptReleaseContext(prov, 0);

    CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);
}

static void test_enum_container(void)
{
    BYTE abContainerName[MAX_PATH + 2]; /* Larger than maximum name len */
    DWORD dwBufferLen;
    BOOL result, fFound = FALSE;

    /* If PP_ENUMCONTAINERS is queried with CRYPT_FIRST and abData == NULL, it returns
     * the maximum legal length of container names (which is MAX_PATH + 1 == 261) */
    SetLastError(0xdeadbeef);
    result = CryptGetProvParam(hProv, PP_ENUMCONTAINERS, NULL, &dwBufferLen, CRYPT_FIRST);
    ok (result, "%08x\n", GetLastError());
    ok (dwBufferLen == MAX_PATH + 1 ||
        broken(dwBufferLen != MAX_PATH + 1), /* Win9x, WinMe, NT4 */
        "Expected dwBufferLen to be (MAX_PATH + 1), it was : %d\n", dwBufferLen);

    /* If the result fits into abContainerName dwBufferLen is left untouched */
    dwBufferLen = (DWORD)sizeof(abContainerName);
    result = CryptGetProvParam(hProv, PP_ENUMCONTAINERS, abContainerName, &dwBufferLen, CRYPT_FIRST);
    ok (result && dwBufferLen == (DWORD)sizeof(abContainerName), "%08x\n", GetLastError());
    
    /* We only check, if the currently open 'winetest' container is among the enumerated. */
    do {
        if (!strcmp((const char*)abContainerName, "winetest")) fFound = TRUE;
        dwBufferLen = (DWORD)sizeof(abContainerName);
    } while (CryptGetProvParam(hProv, PP_ENUMCONTAINERS, abContainerName, &dwBufferLen, 0));
        
    ok (fFound && GetLastError() == ERROR_NO_MORE_ITEMS, "%d, %08x\n", fFound, GetLastError());
}

static BYTE signBlob[] = {
0x07,0x02,0x00,0x00,0x00,0x24,0x00,0x00,0x52,0x53,0x41,0x32,0x00,0x02,0x00,0x00,
0x01,0x00,0x01,0x00,0xf1,0x82,0x9e,0x84,0xb5,0x79,0x9a,0xbe,0x4d,0x06,0x20,0x21,
0xb1,0x89,0x0c,0xca,0xb0,0x35,0x72,0x18,0xc6,0x92,0xa8,0xe2,0xb1,0xe1,0xf6,0x56,
0x53,0x99,0x47,0x10,0x6e,0x1c,0x81,0xaf,0xb8,0xf9,0x5f,0xfe,0x76,0x7f,0x2c,0x93,
0xec,0x54,0x7e,0x5e,0xc2,0x25,0x3c,0x50,0x56,0x10,0x20,0x72,0x4a,0x93,0x03,0x12,
0x29,0x98,0xcc,0xc9,0x47,0xbf,0xbf,0x93,0xcc,0xb0,0xe5,0x53,0x14,0xc8,0x7e,0x1f,
0xa4,0x03,0x2d,0x8e,0x84,0x7a,0xd2,0xeb,0xf7,0x92,0x5e,0xa2,0xc7,0x6b,0x35,0x7d,
0xcb,0x60,0xae,0xfb,0x07,0x78,0x11,0x73,0xb5,0x79,0xe5,0x7e,0x96,0xe3,0x50,0x95,
0x80,0x0e,0x1c,0xf6,0x56,0xc6,0xe9,0x0a,0xaf,0x03,0xc6,0xdc,0x9a,0x81,0xcf,0x7a,
0x63,0x16,0x43,0xcd,0xab,0x74,0xa1,0x7d,0xe7,0xe0,0x75,0x6d,0xbd,0x19,0xae,0x0b,
0xa3,0x7f,0x6a,0x7b,0x05,0x4e,0xbc,0xec,0x18,0xfc,0x19,0xc2,0x00,0xf0,0x6a,0x2e,
0xc4,0x31,0x73,0xba,0x07,0xcc,0x9d,0x57,0xeb,0xc7,0x7c,0x00,0x7d,0x5d,0x11,0x16,
0x42,0x4b,0xe5,0x3a,0xf5,0xc7,0xf8,0xee,0xc3,0x2c,0x0d,0x86,0x03,0xe2,0xaf,0xb2,
0xd2,0x91,0xdb,0x71,0xcd,0xdf,0x81,0x5f,0x06,0xfc,0x48,0x0d,0xb6,0x88,0x9f,0xc1,
0x5e,0x24,0xa2,0x05,0x4f,0x30,0x2e,0x8f,0x8b,0x0d,0x76,0xa1,0x84,0xda,0x7b,0x44,
0x70,0x85,0xf1,0x50,0xb1,0x21,0x3d,0xe2,0x57,0x3d,0xd0,0x01,0x93,0x49,0x8e,0xc5,
0x0b,0x8b,0x0d,0x7b,0x08,0xe9,0x14,0xec,0x20,0x0d,0xea,0x02,0x00,0x63,0xe8,0x0a,
0x52,0xe8,0xfb,0x21,0xbd,0x37,0xde,0x4c,0x4d,0xc2,0xf6,0xb9,0x0d,0x2a,0xc3,0xe2,
0xc9,0xdf,0x48,0x3e,0x55,0x3d,0xe3,0xc0,0x22,0x37,0xf9,0x52,0xc0,0xd7,0x61,0x22,
0xb6,0x85,0x86,0x07 };

static void test_null_provider(void)
{
    HCRYPTPROV prov;
    HCRYPTKEY key;
    BOOL result;
    DWORD keySpec, dataLen,dwParam;
    char szName[MAX_PATH];

    result = CryptAcquireContextA(NULL, szContainer, NULL, 0, 0);
    ok(!result && GetLastError() == NTE_BAD_PROV_TYPE,
     "Expected NTE_BAD_PROV_TYPE, got %08x\n", GetLastError());
    result = CryptAcquireContextA(NULL, szContainer, NULL, PROV_RSA_FULL, 0);
    ok(!result && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == NTE_BAD_KEYSET),
     "Expected ERROR_INVALID_PARAMETER or NTE_BAD_KEYSET, got %08x\n", GetLastError());
    result = CryptAcquireContextA(NULL, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);
    ok(!result && ( GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == NTE_BAD_KEYSET),
     "Expected ERROR_INVALID_PARAMETER or NTE_BAD_KEYSET, got %08x\n", GetLastError());
    result = CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);
    ok(!result && GetLastError() == NTE_BAD_KEYSET,
     "Expected NTE_BAD_KEYSET, got %08x\n", GetLastError());
    result = CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL, 0);
    ok(!result && GetLastError() == NTE_BAD_KEYSET,
     "Expected NTE_BAD_KEYSET, got %08x\n", GetLastError());

    /* Delete the default container. */
    CryptAcquireContextA(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
    /* Once you've deleted the default container you can't open it as if it
     * already exists.
     */
    result = CryptAcquireContextA(&prov, NULL, NULL, PROV_RSA_FULL, 0);
    ok(!result && GetLastError() == NTE_BAD_KEYSET,
     "Expected NTE_BAD_KEYSET, got %08x\n", GetLastError());
    /* But you can always open the default container for CRYPT_VERIFYCONTEXT. */
    result = CryptAcquireContextA(&prov, NULL, NULL, PROV_RSA_FULL,
     CRYPT_VERIFYCONTEXT);
    ok(result, "CryptAcquireContextA failed: %08x\n", GetLastError());
    if (!result) return;
    dataLen = sizeof(keySpec);
    result = CryptGetProvParam(prov, PP_KEYSPEC, (LPBYTE)&keySpec, &dataLen, 0);
    if (result)
        ok(keySpec == (AT_KEYEXCHANGE | AT_SIGNATURE),
         "Expected AT_KEYEXCHANGE | AT_SIGNATURE, got %08x\n", keySpec);
    /* Even though PP_KEYSPEC says both AT_KEYEXCHANGE and AT_SIGNATURE are
     * supported, you can't get the keys from this container.
     */
    result = CryptGetUserKey(prov, AT_KEYEXCHANGE, &key);
    ok(!result && GetLastError() == NTE_NO_KEY,
     "Expected NTE_NO_KEY, got %08x\n", GetLastError());
    result = CryptGetUserKey(prov, AT_SIGNATURE, &key);
    ok(!result && GetLastError() == NTE_NO_KEY,
     "Expected NTE_NO_KEY, got %08x\n", GetLastError());
    result = CryptReleaseContext(prov, 0);
    ok(result, "CryptReleaseContext failed: %08x\n", GetLastError());
    /* You can create a new default container. */
    result = CryptAcquireContextA(&prov, NULL, NULL, PROV_RSA_FULL,
     CRYPT_NEWKEYSET);
    ok(result, "CryptAcquireContextA failed: %08x\n", GetLastError());
    /* But you still can't get the keys (until one's been generated.) */
    result = CryptGetUserKey(prov, AT_KEYEXCHANGE, &key);
    ok(!result && GetLastError() == NTE_NO_KEY,
     "Expected NTE_NO_KEY, got %08x\n", GetLastError());
    result = CryptGetUserKey(prov, AT_SIGNATURE, &key);
    ok(!result && GetLastError() == NTE_NO_KEY,
     "Expected NTE_NO_KEY, got %08x\n", GetLastError());
    CryptReleaseContext(prov, 0);
    CryptAcquireContextA(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_DELETEKEYSET);

    CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);
    result = CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL, 0);
    ok(!result && GetLastError() == NTE_BAD_KEYSET,
     "Expected NTE_BAD_KEYSET, got %08x\n", GetLastError());
    result = CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_VERIFYCONTEXT);
    ok(!result && GetLastError() == NTE_BAD_FLAGS,
     "Expected NTE_BAD_FLAGS, got %08x\n", GetLastError());
    result = CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_NEWKEYSET);
    ok(result, "CryptAcquireContextA failed: %08x\n", GetLastError());
    if (!result) return;
    /* Test provider parameters getter */
    dataLen = sizeof(dwParam);
    result = CryptGetProvParam(prov, PP_PROVTYPE, (LPBYTE)&dwParam, &dataLen, 0);
    ok(result && dataLen == sizeof(dwParam) && dwParam == PROV_RSA_FULL,
        "Expected PROV_RSA_FULL, got 0x%08X\n",dwParam);
    dataLen = sizeof(dwParam);
    result = CryptGetProvParam(prov, PP_KEYSET_TYPE, (LPBYTE)&dwParam, &dataLen, 0);
    ok(result && dataLen == sizeof(dwParam) && dwParam == 0,
        "Expected 0, got 0x%08X\n",dwParam);
    dataLen = sizeof(dwParam);
    result = CryptGetProvParam(prov, PP_KEYSTORAGE, (LPBYTE)&dwParam, &dataLen, 0);
    ok(result && dataLen == sizeof(dwParam) && (dwParam & CRYPT_SEC_DESCR),
        "Expected CRYPT_SEC_DESCR to be set, got 0x%08X\n",dwParam);
    dataLen = sizeof(keySpec);
    SetLastError(0xdeadbeef);
    result = CryptGetProvParam(prov, PP_KEYSPEC, (LPBYTE)&keySpec, &dataLen, 0);
    if (!result && GetLastError() == NTE_BAD_TYPE)
        skip("PP_KEYSPEC is not supported (win9x or NT)\n");
    else
        ok(result && keySpec == (AT_KEYEXCHANGE | AT_SIGNATURE),
            "Expected AT_KEYEXCHANGE | AT_SIGNATURE, got %08x\n", keySpec);
    /* PP_CONTAINER parameter */
    dataLen = sizeof(szName);
    result = CryptGetProvParam(prov, PP_CONTAINER, (LPBYTE)szName, &dataLen, 0);
    ok(result && dataLen == strlen(szContainer)+1 && strcmp(szContainer,szName) == 0,
        "failed getting PP_CONTAINER. result = %s. Error 0x%08X. returned length = %d\n",
        (result)? "TRUE":"FALSE",GetLastError(),dataLen);
    /* PP_UNIQUE_CONTAINER parameter */
    dataLen = sizeof(szName);
    SetLastError(0xdeadbeef);
    result = CryptGetProvParam(prov, PP_UNIQUE_CONTAINER, (LPBYTE)szName, &dataLen, 0);
    if (!result && GetLastError() == NTE_BAD_TYPE)
    {
        skip("PP_UNIQUE_CONTAINER is not supported (win9x or NT)\n");
    }
    else
    {
        char container[MAX_PATH];

        ok(result, "failed getting PP_UNIQUE_CONTAINER : 0x%08X\n", GetLastError());
        uniquecontainer(container);
        todo_wine
        {
            ok(dataLen == strlen(container)+1 ||
               broken(dataLen == strlen(szContainer)+1) /* WinME */,
               "Expected a param length of 70, got %d\n", dataLen);
            ok(!strcmp(container, szName) ||
               broken(!strcmp(szName, szContainer)) /* WinME */,
               "Wrong container name : %s\n", szName);
        }
    }
    result = CryptGetUserKey(prov, AT_KEYEXCHANGE, &key);
    ok(!result && GetLastError() == NTE_NO_KEY,
     "Expected NTE_NO_KEY, got %08x\n", GetLastError());
    result = CryptGetUserKey(prov, AT_SIGNATURE, &key);
    ok(!result && GetLastError() == NTE_NO_KEY,
     "Expected NTE_NO_KEY, got %08x\n", GetLastError());

    /* Importing a key exchange blob.. */
    result = CryptImportKey(prov, abPlainPrivateKey, sizeof(abPlainPrivateKey),
     0, 0, &key);
    ok(result, "CryptImportKey failed: %08x\n", GetLastError());
    CryptDestroyKey(key);
    /* allows access to the key exchange key.. */
    result = CryptGetUserKey(prov, AT_KEYEXCHANGE, &key);
    ok(result, "CryptGetUserKey failed: %08x\n", GetLastError());
    CryptDestroyKey(key);
    /* but not to the private key. */
    result = CryptGetUserKey(prov, AT_SIGNATURE, &key);
    ok(!result && GetLastError() == NTE_NO_KEY,
     "Expected NTE_NO_KEY, got %08x\n", GetLastError());
    CryptReleaseContext(prov, 0);
    CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);

    /* Whereas importing a sign blob.. */
    result = CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_NEWKEYSET);
    ok(result, "CryptAcquireContextA failed: %08x\n", GetLastError());
    if (!result) return;
    result = CryptImportKey(prov, signBlob, sizeof(signBlob), 0, 0, &key);
    ok(result, "CryptImportKey failed: %08x\n", GetLastError());
    CryptDestroyKey(key);
    /* doesn't allow access to the key exchange key.. */
    result = CryptGetUserKey(prov, AT_KEYEXCHANGE, &key);
    ok(!result && GetLastError() == NTE_NO_KEY,
     "Expected NTE_NO_KEY, got %08x\n", GetLastError());
    /* but does to the private key. */
    result = CryptGetUserKey(prov, AT_SIGNATURE, &key);
    ok(result, "CryptGetUserKey failed: %08x\n", GetLastError());
    CryptDestroyKey(key);
    CryptReleaseContext(prov, 0);

    CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);

    /* Test for being able to get a key generated with CALG_RSA_SIGN. */
    result = CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_NEWKEYSET);
    ok(result, "CryptAcquireContextA failed: %08x\n", GetLastError());
    result = CryptGenKey(prov, CALG_RSA_SIGN, 0, &key);
    ok(result, "CryptGenKey with CALG_RSA_SIGN failed with error %08x\n", GetLastError());
    CryptDestroyKey(key);
    result = CryptGetUserKey(prov, AT_KEYEXCHANGE, &key);
    ok(!result, "expected CryptGetUserKey to fail\n");
    result = CryptGetUserKey(prov, AT_SIGNATURE, &key);
    ok(result, "CryptGetUserKey with AT_SIGNATURE failed: %08x\n", GetLastError());
    CryptDestroyKey(key);
    CryptReleaseContext(prov, 0);

    CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);

    /* Test for being able to get a key generated with CALG_RSA_KEYX. */
    result = CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_NEWKEYSET);
    ok(result, "CryptAcquireContextA failed: %08x\n", GetLastError());
    result = CryptGenKey(prov, CALG_RSA_KEYX, 0, &key);
    ok(result, "CryptGenKey with CALG_RSA_KEYX failed with error %08x\n", GetLastError());
    CryptDestroyKey(key);
    result = CryptGetUserKey(prov, AT_KEYEXCHANGE, &key);
    ok(result, "CryptGetUserKey with AT_KEYEXCHANGE failed: %08x\n", GetLastError());
    CryptDestroyKey(key);
    result = CryptGetUserKey(prov, AT_SIGNATURE, &key);
    ok(!result, "expected CryptGetUserKey to fail\n");
    CryptReleaseContext(prov, 0);

    CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);

    /* test for the bug in accessing the user key in a container
     */
    result = CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_NEWKEYSET);
    ok(result, "CryptAcquireContextA failed: %08x\n", GetLastError());
    result = CryptGenKey(prov, AT_KEYEXCHANGE, 0, &key);
    ok(result, "CryptGenKey with AT_KEYEXCHANGE failed with error %08x\n", GetLastError());
    CryptDestroyKey(key);
    CryptReleaseContext(prov,0);
    result = CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,0);
    ok(result, "CryptAcquireContextA failed: 0x%08x\n", GetLastError());
    result = CryptGetUserKey(prov, AT_KEYEXCHANGE, &key);
    ok (result, "CryptGetUserKey failed with error %08x\n", GetLastError());
    CryptDestroyKey(key);
    CryptReleaseContext(prov, 0);

    CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);

    /* test the machine key set */
    CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET|CRYPT_MACHINE_KEYSET);
    result = CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_NEWKEYSET|CRYPT_MACHINE_KEYSET);
    ok(result, "CryptAcquireContextA with CRYPT_MACHINE_KEYSET failed: %08x\n", GetLastError());
    CryptReleaseContext(prov, 0);
    result = CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_MACHINE_KEYSET);
    ok(result, "CryptAcquireContextA with CRYPT_MACHINE_KEYSET failed: %08x\n", GetLastError());
    CryptReleaseContext(prov,0);
    result = CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
       CRYPT_DELETEKEYSET|CRYPT_MACHINE_KEYSET);
    ok(result, "CryptAcquireContextA with CRYPT_DELETEKEYSET|CRYPT_MACHINE_KEYSET failed: %08x\n",
		GetLastError());
    result = CryptAcquireContextA(&prov, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_MACHINE_KEYSET);
    ok(!result && GetLastError() == NTE_BAD_KEYSET ,
	"Expected NTE_BAD_KEYSET, got %08x\n", GetLastError());

}

static void test_key_permissions(void)
{
    HCRYPTKEY hKey1, hKey2;
    DWORD dwVal, dwLen;
    BOOL result;

    /* Create keys that are exportable */
    if (!init_base_environment(NULL, CRYPT_EXPORTABLE))
        return;

    result = CryptGetUserKey(hProv, AT_KEYEXCHANGE, &hKey1);
    ok (result, "%08x\n", GetLastError());
    if (!result) return;

    dwVal = 0xdeadbeef;
    dwLen = sizeof(DWORD);
    result = CryptGetKeyParam(hKey1, KP_PERMISSIONS, (BYTE*)&dwVal, &dwLen, 0);
    ok(result, "%08x\n", GetLastError());
    ok(dwVal ==
        (CRYPT_MAC|CRYPT_WRITE|CRYPT_READ|CRYPT_EXPORT|CRYPT_DECRYPT|CRYPT_ENCRYPT) ||
        broken(dwVal == 0xffffffff), /* Win9x/NT4 */
        "expected CRYPT_MAC|CRYPT_WRITE|CRYPT_READ|CRYPT_EXPORT|CRYPT_DECRYPT|CRYPT_ENCRYPT,"
        " got %08x\n", dwVal);

    /* The key exchange key's public key may be exported.. */
    result = CryptExportKey(hKey1, 0, PUBLICKEYBLOB, 0, NULL, &dwLen);
    ok(result, "%08x\n", GetLastError());
    /* and its private key may be too. */
    result = CryptExportKey(hKey1, 0, PRIVATEKEYBLOB, 0, NULL, &dwLen);
    ok(result, "%08x\n", GetLastError());
    /* Turning off the key's export permissions is "allowed".. */
    dwVal &= ~CRYPT_EXPORT;
    result = CryptSetKeyParam(hKey1, KP_PERMISSIONS, (BYTE *)&dwVal, 0);
    ok(result ||
        broken(!result && GetLastError() == NTE_BAD_DATA) || /* W2K */
        broken(!result && GetLastError() == NTE_BAD_FLAGS), /* Win9x/WinME/NT4 */
        "%08x\n", GetLastError());
    /* but it has no effect. */
    dwVal = 0xdeadbeef;
    dwLen = sizeof(DWORD);
    result = CryptGetKeyParam(hKey1, KP_PERMISSIONS, (BYTE*)&dwVal, &dwLen, 0);
    ok(result, "%08x\n", GetLastError());
    ok(dwVal ==
        (CRYPT_MAC|CRYPT_WRITE|CRYPT_READ|CRYPT_EXPORT|CRYPT_DECRYPT|CRYPT_ENCRYPT) ||
        broken(dwVal == 0xffffffff), /* Win9x/NT4 */
        "expected CRYPT_MAC|CRYPT_WRITE|CRYPT_READ|CRYPT_EXPORT|CRYPT_DECRYPT|CRYPT_ENCRYPT,"
        " got %08x\n", dwVal);
    /* Thus, changing the export flag of the key doesn't affect whether the key
     * may be exported.
     */
    result = CryptExportKey(hKey1, 0, PRIVATEKEYBLOB, 0, NULL, &dwLen);
    ok(result, "%08x\n", GetLastError());

    result = CryptGetUserKey(hProv, AT_KEYEXCHANGE, &hKey2);
    ok (result, "%08x\n", GetLastError());

    /* A subsequent get of the same key, into a different handle, also doesn't
     * show that the permissions have been changed.
     */
    dwVal = 0xdeadbeef;
    dwLen = sizeof(DWORD);
    result = CryptGetKeyParam(hKey2, KP_PERMISSIONS, (BYTE*)&dwVal, &dwLen, 0);
    ok(result, "%08x\n", GetLastError());
    ok(dwVal ==
        (CRYPT_MAC|CRYPT_WRITE|CRYPT_READ|CRYPT_EXPORT|CRYPT_DECRYPT|CRYPT_ENCRYPT) ||
        broken(dwVal == 0xffffffff), /* Win9x/NT4 */
        "expected CRYPT_MAC|CRYPT_WRITE|CRYPT_READ|CRYPT_EXPORT|CRYPT_DECRYPT|CRYPT_ENCRYPT,"
        " got %08x\n", dwVal);

    CryptDestroyKey(hKey2);
    CryptDestroyKey(hKey1);

    clean_up_base_environment();
}

static void test_key_initialization(void)
{
    DWORD dwLen;
    HCRYPTPROV prov1, prov2;
    HCRYPTKEY hKeyExchangeKey, hSessionKey, hKey;
    BOOL result;
    static BYTE abSessionKey[148] = {
        0x01, 0x02, 0x00, 0x00, 0x01, 0x68, 0x00, 0x00,
        0x00, 0xa4, 0x00, 0x00, 0xb8, 0xa4, 0xdf, 0x5e,
        0x9e, 0xb1, 0xbf, 0x85, 0x3d, 0x24, 0x2d, 0x1e,
        0x69, 0xb7, 0x67, 0x13, 0x8e, 0x78, 0xf2, 0xdf,
        0xc6, 0x69, 0xce, 0x46, 0x7e, 0xf2, 0xf2, 0x33,
        0x20, 0x6f, 0xa1, 0xa5, 0x59, 0x83, 0x25, 0xcb,
        0x3a, 0xb1, 0x8a, 0x12, 0x63, 0x02, 0x3c, 0xfb,
        0x4a, 0xfa, 0xef, 0x8e, 0xf7, 0x29, 0x57, 0xb1,
        0x9e, 0xa7, 0xf3, 0x02, 0xfd, 0xca, 0xdf, 0x5a,
        0x1f, 0x71, 0xb6, 0x26, 0x09, 0x24, 0x39, 0xda,
        0xc0, 0xde, 0x2a, 0x0e, 0xcd, 0x1f, 0xe5, 0xb6,
        0x4f, 0x82, 0xa0, 0xa9, 0x90, 0xd3, 0xd9, 0x6a,
        0x43, 0x14, 0x2a, 0xf7, 0xac, 0xd5, 0xa0, 0x54,
        0x93, 0xc4, 0xb9, 0xe7, 0x24, 0x84, 0x4d, 0x69,
        0x5e, 0xcc, 0x2a, 0x32, 0xb5, 0xfb, 0xe4, 0xb4,
        0x08, 0xd5, 0x36, 0x58, 0x59, 0x40, 0xfb, 0x29,
        0x7f, 0xa7, 0x17, 0x25, 0xc4, 0x0d, 0x78, 0x37,
        0x04, 0x8c, 0x49, 0x92
    };

    /* Like init_base_environment, but doesn't generate new keys, as they'll
     * be imported instead.
     */
    if (!CryptAcquireContextA(&prov1, szContainer, szProvider, PROV_RSA_FULL, 0))
    {
        result = CryptAcquireContextA(&prov1, szContainer, szProvider, PROV_RSA_FULL,
                                     CRYPT_NEWKEYSET);
        ok(result, "CryptAcquireContextA failed: %08x\n", GetLastError());
    }
    dwLen = (DWORD)sizeof(abPlainPrivateKey);
    result = CryptImportKey(prov1, abPlainPrivateKey, dwLen, 0, 0, &hKeyExchangeKey);
    ok(result, "CryptImportKey failed: %08x\n", GetLastError());

    dwLen = (DWORD)sizeof(abSessionKey);
    result = CryptImportKey(prov1, abSessionKey, dwLen, hKeyExchangeKey, 0, &hSessionKey);
    ok(result, "CryptImportKey failed: %08x\n", GetLastError());

    /* Once the key has been imported, subsequently acquiring a context with
     * the same name will allow retrieving the key.
     */
    result = CryptAcquireContextA(&prov2, szContainer, szProvider, PROV_RSA_FULL, 0);
    ok(result, "CryptAcquireContextA failed: %08x\n", GetLastError());
    result = CryptGetUserKey(prov2, AT_KEYEXCHANGE, &hKey);
    ok(result, "CryptGetUserKey failed: %08x\n", GetLastError());
    if (result) CryptDestroyKey(hKey);
    CryptReleaseContext(prov2, 0);

    CryptDestroyKey(hSessionKey);
    CryptDestroyKey(hKeyExchangeKey);
    CryptReleaseContext(prov1, 0);
    CryptAcquireContextA(&prov1, szContainer, NULL, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);
}

static void test_key_derivation(const char *prov)
{
    HCRYPTKEY hKey;
    HCRYPTHASH hHash;
    BOOL result;
    unsigned char pbData[128], dvData[512];
    DWORD i, j, len, mode;
    struct _test
    {
        ALG_ID crypt_algo, hash_algo;
        int blocklen, hashlen, chain_mode;
        DWORD errorkey;
        const char *expected_hash, *expected_enc;
    } tests[] = {
        /* ================================================================== */
        { CALG_DES, CALG_MD2, 8, 16, CRYPT_MODE_CBC, 0,
          "\xBA\xBF\x93\xAE\xBC\x77\x45\xAA\x7E\x45\x69\xE5\x90\xE6\x04\x7F",
          "\x5D\xDA\x25\xA6\xB5\xC4\x43\xFB",
          /* 0 */
        },
        { CALG_3DES_112, CALG_MD2, 8, 16, CRYPT_MODE_CBC, 0,
          "\xDA\x4A\x9F\x5D\x2E\x7A\x3A\x4B\xBF\xDE\x47\x5B\x06\x84\x48\xA7",
          "\x6B\x18\x3B\xA1\x89\x27\xBF\xD4",
          /* 1 */
        },
        { CALG_3DES, CALG_MD2, 8, 16, CRYPT_MODE_CBC, 0,
          "\x38\xE5\x2E\x95\xA4\xA3\x73\x88\xF8\x1F\x87\xB7\x74\xB1\xA1\x56",
          "\x91\xAB\x17\xE5\xDA\x27\x11\x7D",
          /* 2 */
        },
        { CALG_RC2, CALG_MD2, 8, 16, CRYPT_MODE_CBC, 0,
          "\x7D\xA4\xB1\x10\x43\x26\x76\xB1\x0D\xB6\xE6\x9C\xA5\x8B\xCB\xE6",
          "\x7D\x45\x3D\x56\x00\xD7\xD1\x54",
          /* 3 */
        },
        { CALG_RC4, CALG_MD2, 4, 16, 0, 0,
          "\xFF\x32\xF1\x69\x62\xDE\xEB\x53\x8C\xFF\xA6\x92\x58\xA8\x22\xEA",
          "\xA9\x83\x73\xA9",
          /* 4 */
        },
        { CALG_RC5, CALG_MD2, 0, 16, 0, NTE_BAD_ALGID,
          "\x8A\xF2\xA3\xDA\xA5\x9A\x8B\x42\x4C\xE0\x2E\x00\xE5\x1E\x98\xE4",
          NULL,
          /* 5 */
        },
        { CALG_RSA_SIGN, CALG_MD2, 0, 16, 0, NTE_BAD_ALGID,
          "\xAE\xFE\xD6\xA5\x3E\x4B\xAC\xFA\x0E\x92\xC4\xC0\x06\xC9\x2B\xFD",
          NULL,
          /* 6 */
        },
        { CALG_RSA_KEYX, CALG_MD2, 0, 16, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\x30\xF4\xBC\x33\x93\xF3\x58\x19\xD1\x2B\x73\x4A\x92\xC7\xFC\xD7",
          NULL,
          /* 7 */
        },
        { CALG_AES, CALG_MD2, 0, 16, 0, NTE_BAD_ALGID,
          "\x07\x3B\x12\xE9\x96\x93\x85\xD7\xEC\xF4\xB1\xAC\x89\x2D\xC6\x9A",
          NULL,
          /* 8 */
        },
        { CALG_AES_128, CALG_MD2, 16, 16, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\xD2\x37\xE2\x49\xEB\x99\x23\xDA\x3E\x88\x55\x7E\x04\x5E\x15\x5D",
          "\xA1\x64\x3F\xFE\x99\x7F\x24\x13\x0C\xA9\x03\xEF\x9B\xC8\x1F\x2A",
          /* 9 */
        },
        { CALG_AES_192, CALG_MD2, 16, 16, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\x3E\x74\xED\xBF\x23\xAB\x03\x09\xBB\xD3\xE3\xAB\xCA\x12\x72\x7F",
          "\x5D\xEC\xF8\x72\xB2\xA6\x4D\x5C\xEA\x38\x9E\xF0\x86\xB6\x79\x34",
          /* 10 */
        },
        { CALG_AES_256, CALG_MD2, 16, 16, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\xBE\x9A\xE8\xF6\xCE\x79\x86\x5C\x1B\x01\x96\x4E\x5A\x8D\x09\x33",
          "\xD9\x4B\xC2\xE3\xCA\x89\x8B\x94\x0D\x87\xBB\xA2\xE8\x3D\x5C\x62",
          /* 11 */
        },
        /* ================================================================== */
        { CALG_DES, CALG_MD4, 8, 16, CRYPT_MODE_CBC, 0,
          "\xE8\x2F\x96\xC4\x6C\xC1\x91\xB4\x78\x40\x56\xD8\xA0\x25\xF5\x71",
          "\x21\x5A\xBD\x26\xB4\x3E\x86\x04",
          /* 12 */
        },
        { CALG_3DES_112, CALG_MD4, 8, 16, CRYPT_MODE_CBC, 0,
          "\x23\xBB\x6F\xE4\xB0\xF6\x35\xB6\x89\x2F\xEC\xDC\x06\xA9\xDF\x35",
          "\x9B\xE5\xD1\xEB\x8F\x13\x0B\xB3",
          /* 13 */
        },
        { CALG_3DES, CALG_MD4, 8, 16, CRYPT_MODE_CBC, 0,
          "\xE4\x72\x48\xC6\x6E\x38\x2F\x00\xC9\x2D\x01\x12\xB7\x8B\x64\x09",
          "\x7D\x5E\xAA\xEA\x10\xA4\xA4\x44",
          /* 14 */
        },
        { CALG_RC2, CALG_MD4, 8, 16, CRYPT_MODE_CBC, 0,
          "\xBF\x54\xDA\x3A\x56\x72\x0D\x9F\x30\x7D\x2F\x54\x13\xB2\xD7\xC6",
          "\x77\x42\x0E\xD2\x60\x29\x6F\x68",
          /* 15 */
        },
        { CALG_RC4, CALG_MD4, 4, 16, 0, 0,
          "\x9B\x74\x6D\x22\x11\x16\x05\x50\xA3\x75\x6B\xB2\x38\x8C\x2B\xC6",
          "\x5C\x7E\x99\x84",
          /* 16 */
        },
        { CALG_RC5, CALG_MD4, 0, 16, 0, NTE_BAD_ALGID,
          "\x51\xA8\x29\x8D\xE0\x36\xC1\xD3\x5E\x6A\x51\x4F\xE1\x65\xEE\xF1",
          NULL,
          /* 17 */
        },
        { CALG_RSA_SIGN, CALG_MD4, 0, 16, 0, NTE_BAD_ALGID,
          "\xA6\x83\x13\x4C\xB1\xAA\x06\x16\xE6\x4E\x7F\x0B\x8D\x19\xF5\x45",
          NULL,
          /* 18 */
        },
        { CALG_RSA_KEYX, CALG_MD4, 0, 16, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\x04\x24\xC8\x64\x98\x84\xE3\x3A\x7B\x9C\x50\x3E\xE7\xC4\x89\x82",
          NULL,
          /* 19 */
        },
        { CALG_AES, CALG_MD4, 0, 16, 0, NTE_BAD_ALGID,
          "\xF6\xEF\x81\xF8\xF2\xA3\xF6\x11\xFE\xA4\x7D\xC1\xD2\xF7\x7C\xDC",
          NULL,
          /* 20 */
        },
        { CALG_AES_128, CALG_MD4, 16, 16, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\xFF\xE9\x69\xFF\xC1\xDB\x08\xD4\x5B\xC8\x51\x71\x38\xEF\x8A\x5B",
          "\x8A\x24\xD0\x7A\x03\xE7\xA7\x02\xF2\x17\x4C\x01\xD5\x0E\x7F\x12",
          /* 21 */
        },
        { CALG_AES_192, CALG_MD4, 16, 16, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\x12\x01\xDD\x25\xBA\x8F\x1B\xCB\x7B\xAD\x3F\xDF\xB2\x68\x4F\x6A",
          "\xA9\x56\xBC\xA7\x97\x4E\x28\xAA\x4B\xE1\xA0\x6C\xE2\x43\x2C\x61",
          /* 22 */
        },
        { CALG_AES_256, CALG_MD4, 16, 16, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\x69\x08\x9F\x76\xD7\x9A\x93\x6F\xC7\x51\xA4\x00\xCF\x5A\xBB\x3D",
          "\x04\x07\xEA\xD9\x89\x0A\xD2\x65\x12\x13\x68\x9A\xD0\x86\x15\xED",
          /* 23 */
        },
        /* ================================================================== */
        { CALG_DES, CALG_MD5, 8, 16, CRYPT_MODE_CBC, 0,
          "\xEA\x01\x47\xA0\x7F\x96\x44\x6B\x0D\x95\x2C\x97\x4B\x28\x1C\x86",
          "\xF3\x75\xCC\x7C\x6C\x0B\xCF\x93",
          /* 24 */
        },
        { CALG_3DES_112, CALG_MD5, 8, 16, CRYPT_MODE_CBC, 0,
          "\xD2\xA2\xD7\x87\x32\x29\xF9\xE0\x45\x0D\xEC\x8D\xB5\xBC\x8A\xD9",
          "\x51\x70\xE0\xB7\x00\x0D\x3E\x21",
          /* 25 */
        },
        { CALG_3DES, CALG_MD5, 8, 16, CRYPT_MODE_CBC, 0,
          "\x2B\x36\xA2\x85\x85\xC0\xEC\xBE\x04\x56\x1D\x97\x8E\x82\xDB\xD8",
          "\x58\x23\x75\x25\x3F\x88\x25\xEB",
          /* 26 */
        },
        { CALG_RC2, CALG_MD5, 8, 16, CRYPT_MODE_CBC, 0,
          "\x3B\x89\x72\x3B\x8A\xD1\x2E\x13\x44\xD6\xD0\x97\xE6\xB8\x46\xCD",
          "\x90\x1C\x77\x45\x87\xDD\x1C\x2E",
          /* 27 */
        },
        { CALG_RC4, CALG_MD5, 4, 16, 0, 0,
          "\x00\x6D\xEF\xB1\xC8\xC6\x25\x5E\x45\x4F\x4E\x3D\xAF\x9C\x53\xD2",
          "\xC4\x4C\xD2\xF1",
          /* 28 */
        },
        { CALG_RC5, CALG_MD5, 0, 16, 0, NTE_BAD_ALGID,
          "\x56\x49\xDC\xBA\x32\xC6\x0D\x84\xE9\x2D\x42\x8C\xD6\x7C\x4A\x7A",
          NULL,
          /* 29 */
        },
        { CALG_RSA_SIGN, CALG_MD5, 0, 16, 0, NTE_BAD_ALGID,
          "\xDF\xD6\x3A\xE6\x3E\x8D\xB4\x17\x9F\x29\xF0\xFD\x6D\x98\x98\xAD",
          NULL,
          /* 30 */
        },
        { CALG_RSA_KEYX, CALG_MD5, 0, 16, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\xD4\x4D\x60\x9A\x39\x27\x88\xB7\xD7\xB4\x34\x2F\x92\x61\x3C\xA8",
          NULL,
          /* 31 */
        },
        { CALG_AES, CALG_MD5, 0, 16, 0, NTE_BAD_ALGID,
          "\xF4\x83\x2E\x02\xDE\xAE\x46\x1F\xE1\x31\x65\x03\x08\x58\xE0\x7D",
          NULL,
          /* 32 */
        },
        { CALG_AES_128, CALG_MD5, 16, 16, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\x0E\xA0\x40\x72\x55\xE5\x4C\xEB\x79\xCB\x48\xC3\xD1\xB1\xD0\xF4",
          "\x97\x66\x92\x02\x6D\xEC\x33\xF8\x4E\x82\x11\x20\xC7\xE2\xE6\xE8",
          /* 33 */
        },
        { CALG_AES_192, CALG_MD5, 16, 16, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\x3F\x91\x5E\x09\x19\x11\x14\x27\xCA\x6A\x20\x24\x3E\xF0\x02\x3E",
          "\x9B\xDA\x73\xF4\xF3\x06\x93\x07\xC9\x32\xF1\xD8\xD4\x96\xD1\x7D",
          /* 34 */
        },
        { CALG_AES_256, CALG_MD5, 16, 16, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\x27\x51\xD8\xB3\xC7\x14\x66\xE1\x99\xC3\x5C\x9C\x90\xF5\xE5\x94",
          "\x2A\x0F\xE9\xA9\x6F\x53\x7C\x9E\x07\xE6\xC3\xC9\x15\x99\x7C\xA8",
          /* 35 */
        },
        /* ================================================================== */
        { CALG_DES, CALG_SHA1, 8, 20, CRYPT_MODE_CBC, 0,
          "\xC1\x91\xF6\x5A\x81\x87\xAC\x6D\x48\x7C\x78\xF7\xEC\x37\xE2\x0C\xEC\xF7\xC0\xB8",
          "\xD4\xD8\xAA\x44\xAC\x5E\x0B\x8D",
          /* 36 */
        },
        { CALG_3DES_112, CALG_SHA1, 8, 20, CRYPT_MODE_CBC, 0,
          "\x5D\x9B\xC3\x99\xC4\x73\x90\x78\xCB\x51\x6B\x61\x8A\xBE\x1A\xF3\x7A\x90\xF3\x34",
          "\xD8\x1C\xBC\x6C\x92\xD3\x09\xBF",
          /* 37 */
        },
        { CALG_3DES, CALG_SHA1, 8, 20, CRYPT_MODE_CBC, 0,
          "\x90\xB8\x01\x89\xEC\x9A\x6C\xAD\x1E\xAC\xB3\x17\x0A\x44\xA2\x4D\x80\xA5\x25\x97",
          "\xBD\x58\x5A\x88\x98\xF8\x69\x9A",
          /* 38 */
        },
        { CALG_RC2, CALG_SHA1, 8, 20, CRYPT_MODE_CBC, 0,
          "\x42\xBD\xB8\xF2\xB5\xC2\x28\x64\x85\x98\x8E\x49\xE6\xDC\x92\x80\xCD\xC1\x63\x00",
          "\xCC\xFB\x1A\x4D\x29\xAD\x3E\x65",
          /* 39 */
        },
        { CALG_RC4, CALG_SHA1, 4, 20, 0, 0,
          "\x67\x36\xE9\x57\x5E\xCD\x56\x5E\x8B\x25\x35\x23\x74\xBA\x20\x46\xD0\x21\xDE\x0A",
          "\x7A\x34\x3D\x3C",
          /* 40 */
        },
        { CALG_RC5, CALG_SHA1, 0, 20, 0, NTE_BAD_ALGID,
          "\x5F\x29\xA5\xA4\x10\x08\x56\x15\x92\xF9\x55\x3B\x4B\xF5\xAB\xBD\xE7\x4D\x47\x28",
          NULL,
          /* 41 */
        },
        { CALG_RSA_SIGN, CALG_SHA1, 0, 20, 0, NTE_BAD_ALGID,
          "\xD3\xB7\xF8\xB9\xBE\x67\xD1\xFE\x10\x51\x23\x3B\x7D\xB7\x61\xF5\xA7\x1A\x02\x5E",
          NULL,
          /* 42 */
        },
        { CALG_RSA_KEYX, CALG_SHA1, 0, 20, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\x09\x68\x97\x23\x11\x2B\x6A\x71\xBA\x33\x60\x43\xEE\xC9\x9B\xB7\x8F\x8A\x2E\x33",
          NULL,
          /* 43 */
        },
        { CALG_AES, CALG_SHA1, 0, 20, 0, NTE_BAD_ALGID,
          "\xCF\x28\x23\x83\x62\x87\x43\xF6\x50\x57\xED\x54\xEC\x93\x5E\xEC\x0E\xD3\x23\x9A",
          NULL,
          /* 44 */
        },
        { CALG_AES_128, CALG_SHA1, 16, 20, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\x81\xC1\x7E\x42\xC3\x07\x1F\x5E\xF8\x75\xA3\x5A\xFC\x0B\x61\xBA\x0B\xD8\x53\x0D",
          "\x39\xCB\xAF\xD7\x8B\x75\x4A\x3B\xD2\x0E\x0D\xB1\x64\x57\x88\x58",
          /* 45 */
        },
        { CALG_AES_192, CALG_SHA1, 16, 20, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\x93\xA7\xE8\x9E\x96\xB5\x97\x23\xD0\x58\x44\x8C\x4D\xBB\xAB\xB6\x3E\x1F\x2C\x1D",
          "\xA9\x13\x83\xCA\x21\xA2\xF0\xBE\x13\xBC\x55\x04\x38\x08\xA9\xC4",
          /* 46 */
        },
        { CALG_AES_256, CALG_SHA1, 16, 20, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\x15\x6A\xB2\xDF\x32\x57\x14\x69\x09\x07\xAD\x24\x83\xA1\x74\x47\x41\x72\x69\xBC",
          "\xE1\x6C\xA8\x54\x0E\x24\x67\x6D\xCA\xA2\xFE\x84\xF0\x9B\x78\x66",
          /* 47 */
        },
        /* ================================================================== */
        { CALG_DES, CALG_SHA_256, 8, 32, CRYPT_MODE_CBC, 0,
          "\x20\x34\xf7\xbb\x7a\x3a\x79\xf0\xb9\x65\x18\x11\xaa\xfd\x26\x6b"
          "\x60\x5c\x6d\x4c\x81\x7c\x3f\xc4\xce\x94\xe3\x67\xdf\xf2\x16\xd8",
          "\x86\x0d\x8c\xf4\xc0\x22\x4a\xdd",
          /* 48 */
        },
        { CALG_3DES_112, CALG_SHA_256, 8, 32, CRYPT_MODE_CBC, 0,
          "\x09\x6e\x7f\xd5\xf2\x72\x4e\x18\x70\x09\xc1\x35\xf4\xd1\x3a\xe8"
          "\xe6\x1f\x91\xae\x2f\xfd\xa8\x8c\xce\x47\x0f\x7a\xf5\xef\xfd\xbe",
          "\x2d\xe7\x63\xf6\x58\x4d\x9a\xa6",
          /* 49 */
        },
        { CALG_3DES, CALG_SHA_256, 8, 32, CRYPT_MODE_CBC, 0,
          "\x54\x7f\x84\x7f\xfe\x83\xc6\x50\xbc\xd9\x92\x78\x32\x67\x50\x7d"
          "\xdf\x44\x55\x7d\x87\x74\xd2\x56\xff\xd9\x74\x44\xd5\x07\x9e\xdc",
          "\x20\xaa\x66\xd0\xac\x83\x9d\x99",
          /* 50 */
        },
        { CALG_RC2, CALG_SHA_256, 8, 32, CRYPT_MODE_CBC, 0,
          "\xc6\x22\x46\x15\xa1\x27\x38\x23\x91\xf2\x29\xda\x15\xc9\x5d\x92"
          "\x7c\x34\x4a\x1f\xb0\x8a\x81\xd6\x17\x09\xda\x52\x1f\xb9\x64\x60",
          "\x8c\x01\x19\x47\x7e\xd2\x10\x2c",
          /* 51 */
        },
        { CALG_RC4, CALG_SHA_256, 4, 32, 0, 0,
          "\xcd\x53\x95\xa6\xb6\x6e\x25\x92\x78\xac\xe6\x7e\xfc\xd3\x8d\xaa"
          "\xc3\x15\x83\xb5\xe6\xaf\xf9\x32\x4c\x17\xb8\x82\xdf\xc0\x45\x9e",
          "\xfa\x54\x13\x9c",
          /* 52 */
        },
        { CALG_RC5, CALG_SHA_256, 0, 32, 0, NTE_BAD_ALGID,
          "\x2a\x3b\x08\xe1\xec\xa7\x04\xf9\xc9\x42\x74\x9a\x82\xad\x99\xd2"
          "\x10\x51\xe3\x51\x6c\x67\xa4\xf2\xca\x99\x21\x43\xdf\xa0\xfc\xa1",
          NULL,
          /* 53 */
        },
        { CALG_RSA_SIGN, CALG_SHA_256, 0, 32, 0, NTE_BAD_ALGID,
          "\x10\x1d\x36\xc7\x38\x73\xc3\x80\xf0\x7a\x4e\x25\x52\x8a\x5c\x3f"
          "\xfc\x41\xa7\xe5\x20\xed\xd5\x1d\x00\x6e\x77\xf4\xa7\x71\x81\x6b",
          NULL,
          /* 54 */
        },
        { CALG_RSA_KEYX, CALG_SHA_256, 0, 32, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\x0a\x74\xde\x4f\x07\xce\x73\xd6\xd9\xa3\xba\xbb\x7c\x98\xe1\x94"
          "\x13\x93\xb1\xfd\x26\x31\x4b\xfc\x61\x27\xef\x4d\xd0\x48\x76\x67",
          NULL,
          /* 55 */
        },
        { CALG_AES, CALG_SHA_256, 0, 32, 0, NTE_BAD_ALGID,
          "\xf0\x13\xbc\x25\x2a\x2f\xba\xf1\x39\xe5\x7d\xb8\x5f\xaa\xd0\x19"
          "\xbd\x1c\xd8\x7b\x39\x5a\xb3\x85\x84\x80\xbd\xe0\x4a\x65\x03\xdd",
          NULL,
          /* 56 */
        },
        { CALG_AES_128, CALG_SHA_256, 16, 32, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\xc8\xc2\x6f\xe2\xbe\xa7\x38\x87\x04\xc7\x39\xcb\x9f\x57\xfc\xde"
          "\x14\x81\x46\xa4\xbb\xa7\x0f\x01\x1d\xc2\x6d\x7a\x43\x5f\x38\xc3",
          "\xf8\x75\xc6\x71\x8b\xb6\x54\xd3\xdc\xff\x0e\x84\x8a\x3f\x19\x46",
          /* 57 */
        },
        { CALG_AES_192, CALG_SHA_256, 16, 32, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\xb7\x3a\x43\x0f\xea\x90\x4f\x0f\xb9\x82\xf6\x1e\x07\xc4\x25\x4e"
          "\xdb\xe7\xf7\x1d\x7c\xd0\xe5\x51\xd8\x1b\x97\xc8\xc2\x46\xb9\xfe",
          "\x35\xf2\x20\xc7\x6c\xb2\x8e\x51\x3e\xc7\x6b\x3e\x64\xa5\x05\xdf",
          /* 58 */
        },
        { CALG_AES_256, CALG_SHA_256, 16, 32, CRYPT_MODE_CBC, NTE_BAD_ALGID,
          "\xbd\xcc\x0c\x59\x99\x29\xa7\x24\xf3\xdc\x20\x40\x4e\xe8\xe5\x48"
          "\xdd\x27\x0e\xdf\x7e\x50\x65\x17\x34\x50\x47\x78\x9a\x23\x1b\x40",
          "\x8c\xeb\x1f\xd3\x78\x77\xf5\xbf\x7a\xde\x8d\x2c\xa5\x16\xcc\xe9",
          /* 59 */
        },
    };
    /* Due to differences between encryption from <= 2000 and >= XP some tests need to be skipped */
    int old_broken[sizeof(tests)/sizeof(tests[0])];
    memset(old_broken, 0, sizeof(old_broken));
    old_broken[3] = old_broken[4] = old_broken[15] = old_broken[16] = 1;
    old_broken[27] = old_broken[28] = old_broken[39] = old_broken[40] = 1;
    uniquecontainer(NULL);

    for (i=0; i<sizeof(tests)/sizeof(tests[0]); i++)
    {
        if (win2k && old_broken[i]) continue;

        for (j=0; j<sizeof(dvData); j++) dvData[j] = (unsigned char)j+i;
        SetLastError(0xdeadbeef);
        result = CryptCreateHash(hProv, tests[i].hash_algo, 0, 0, &hHash);
        if (!result)
        {
            /* rsaenh compiled without OpenSSL or not supported by provider */
            ok(GetLastError() == NTE_BAD_ALGID, "Test [%s %d]: Expected NTE_BAD_ALGID, got 0x%08x\n",
               prov, i, GetLastError());
            continue;
        }
        ok(result, "Test [%s %d]: CryptCreateHash failed with error 0x%08x\n", prov, i, GetLastError());
        result = CryptHashData(hHash, dvData, sizeof(dvData), 0);
        ok(result, "Test [%s %d]: CryptHashData failed with error 0x%08x\n", prov, i, GetLastError());

        len = sizeof(pbData);
        result = CryptGetHashParam(hHash, HP_HASHVAL, pbData, &len, 0);
        ok(result, "Test [%s %d]: CryptGetHashParam failed with error 0x%08x\n", prov, i, GetLastError());
        ok(len == tests[i].hashlen, "Test [%s %d]: Expected hash len %d, got %d\n",
           prov, i, tests[i].hashlen, len);
        ok(!tests[i].hashlen || !memcmp(pbData, tests[i].expected_hash, tests[i].hashlen),
           "Test [%s %d]: Hash comparison failed\n", prov, i);

        SetLastError(0xdeadbeef);
        result = CryptDeriveKey(hProv, tests[i].crypt_algo, hHash, 0, &hKey);
        /* the provider may not support the algorithm */
        if(!result && (GetLastError() == tests[i].errorkey
           || GetLastError() == ERROR_INVALID_PARAMETER /* <= NT4*/))
            goto err;
        ok(result, "Test [%s %d]: CryptDeriveKey failed with error 0x%08x\n", prov, i, GetLastError());

        len = sizeof(mode);
        mode = 0xdeadbeef;
        result = CryptGetKeyParam(hKey, KP_MODE, (BYTE*)&mode, &len, 0);
        ok(result, "Test [%s %d]: CryptGetKeyParam failed with error %08x\n", prov, i, GetLastError());
        ok(mode == tests[i].chain_mode, "Test [%s %d]: Expected chaining mode %d, got %d\n",
           prov, i, tests[i].chain_mode, mode);

        SetLastError(0xdeadbeef);
        len = 4;
        result = CryptEncrypt(hKey, 0, TRUE, 0, dvData, &len, sizeof(dvData));
        ok(result, "Test [%s %d]: CryptEncrypt failed with error 0x%08x\n", prov, i, GetLastError());
        ok(len == tests[i].blocklen, "Test [%s %d]: Expected block len %d, got %d\n",
           prov, i, tests[i].blocklen, len);
        ok(!memcmp(dvData, tests[i].expected_enc, tests[i].blocklen),
           "Test [%s %d]: Encrypted data comparison failed\n", prov, i);

        CryptDestroyKey(hKey);
err:
        CryptDestroyHash(hHash);
    }
}

START_TEST(rsaenh)
{
    for (iProv = 0; iProv < sizeof(szProviders) / sizeof(szProviders[0]); iProv++)
    {
        if (!init_base_environment(szProviders[iProv], 0))
            continue;
        trace("Testing '%s'\n", szProviders[iProv]);
        test_prov();
        test_gen_random();
        test_hashes();
        test_rc4();
        test_rc2();
        test_des();
        if(!BASE_PROV)
        {
            test_3des112();
            test_3des();
        }
        if(ENHANCED_PROV)
        {
            test_import_private();
        }
        test_hmac();
        test_mac();
        test_block_cipher_modes();
        test_verify_signature();
        test_rsa_encrypt();
        test_import_export();
        test_import_hmac();
        test_enum_container();
        if(!BASE_PROV) test_key_derivation(STRONG_PROV ? "STRONG" : "ENH");
        clean_up_base_environment();
    }
    if (!init_base_environment(MS_ENHANCED_PROV_A, 0))
    test_key_permissions();
    test_key_initialization();
    test_schannel_provider();
    test_null_provider();
    test_rsa_round_trip();
    if (!init_aes_environment())
        return;
    test_aes(128);
    test_aes(192);
    test_aes(256);
    test_sha2();
    test_key_derivation("AES");
    clean_up_aes_environment();
}
