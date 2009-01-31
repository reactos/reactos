/*
 * Unit test suite for crypt32.dll's CryptProtectData/CryptUnprotectData
 *
 * Copyright 2005 Kees Cook <kees@outflux.net>
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

#include "wine/test.h"

static BOOL (WINAPI *pCryptProtectData)(DATA_BLOB*,LPCWSTR,DATA_BLOB*,PVOID,CRYPTPROTECT_PROMPTSTRUCT*,DWORD,DATA_BLOB*);
static BOOL (WINAPI *pCryptUnprotectData)(DATA_BLOB*,LPWSTR*,DATA_BLOB*,PVOID,CRYPTPROTECT_PROMPTSTRUCT*,DWORD,DATA_BLOB*);

static char  secret[]     = "I am a super secret string that no one can see!";
static char  secret2[]    = "I am a super secret string indescribable string";
static char  key[]        = "Wibble wibble wibble";
static const WCHAR desc[] = {'U','l','t','r','a',' ','s','e','c','r','e','t',' ','t','e','s','t',' ','m','e','s','s','a','g','e',0};
static BOOL protected = FALSE; /* if true, the unprotect tests can run */
static DATA_BLOB cipher;
static DATA_BLOB cipher_entropy;
static DATA_BLOB cipher_no_desc;

static void test_cryptprotectdata(void)
{
    LONG r;
    DATA_BLOB plain;
    DATA_BLOB entropy;

    plain.pbData=(void*)secret;
    plain.cbData=strlen(secret)+1;

    entropy.pbData=(void*)key;
    entropy.cbData=strlen(key)+1;

    SetLastError(0xDEADBEEF);
    protected = pCryptProtectData(NULL,desc,NULL,NULL,NULL,0,&cipher);
    ok(!protected, "Encrypting without plain data source.\n");
    r = GetLastError();
    ok(r == ERROR_INVALID_PARAMETER, "Wrong (%u) GetLastError seen\n",r);

    SetLastError(0xDEADBEEF);
    protected = pCryptProtectData(&plain,desc,NULL,NULL,NULL,0,NULL);
    ok(!protected, "Encrypting without cipher destination.\n");
    r = GetLastError();
    ok(r == ERROR_INVALID_PARAMETER, "Wrong (%u) GetLastError seen\n",r);

    cipher.pbData=NULL;
    cipher.cbData=0;

    /* without entropy */
    SetLastError(0xDEADBEEF);
    protected = pCryptProtectData(&plain,desc,NULL,NULL,NULL,0,&cipher);
    ok(protected ||
     broken(!protected), /* Win9x/NT4 */
     "Encrypting without entropy.\n");
    if (protected)
    {
        r = GetLastError();
        ok(r == ERROR_SUCCESS ||
           r == ERROR_IO_PENDING, /* win2k */
           "Expected ERROR_SUCCESS or ERROR_IO_PENDING, got %d\n",r);
    }

    cipher_entropy.pbData=NULL;
    cipher_entropy.cbData=0;

    /* with entropy */
    SetLastError(0xDEADBEEF);
    protected = pCryptProtectData(&plain,desc,&entropy,NULL,NULL,0,&cipher_entropy);
    ok(protected ||
     broken(!protected), /* Win9x/NT4 */
     "Encrypting with entropy.\n");

    cipher_no_desc.pbData=NULL;
    cipher_no_desc.cbData=0;

    /* with entropy but no description */
    plain.pbData=(void*)secret2;
    plain.cbData=strlen(secret2)+1;
    SetLastError(0xDEADBEEF);
    protected = pCryptProtectData(&plain,NULL,&entropy,NULL,NULL,0,&cipher_no_desc);
    if (!protected)
    {
        /* fails in win2k */
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }
}

static void test_cryptunprotectdata(void)
{
    LONG r;
    DATA_BLOB plain;
    DATA_BLOB entropy;
    BOOL okay;
    WCHAR * data_desc;

    entropy.pbData=(void*)key;
    entropy.cbData=strlen(key)+1;

    /* fails in win2k */
    if (!protected)
    {
        skip("CryptProtectData failed to run\n");
        return;
    }

    plain.pbData=NULL;
    plain.cbData=0;

    SetLastError(0xDEADBEEF);
    okay = pCryptUnprotectData(&cipher,NULL,NULL,NULL,NULL,0,NULL);
    ok(!okay,"Decrypting without destination\n");
    r = GetLastError();
    ok(r == ERROR_INVALID_PARAMETER, "Wrong (%u) GetLastError seen\n",r);

    SetLastError(0xDEADBEEF);
    okay = pCryptUnprotectData(NULL,NULL,NULL,NULL,NULL,0,&plain);
    ok(!okay,"Decrypting without source\n");
    r = GetLastError();
    ok(r == ERROR_INVALID_PARAMETER, "Wrong (%u) GetLastError seen\n",r);

    plain.pbData=NULL;
    plain.cbData=0;

    SetLastError(0xDEADBEEF);
    okay = pCryptUnprotectData(&cipher_entropy,NULL,NULL,NULL,NULL,0,&plain);
    ok(!okay,"Decrypting without needed entropy\n");
    r = GetLastError();
    ok(r == ERROR_INVALID_DATA, "Wrong (%u) GetLastError seen\n", r);

    plain.pbData=NULL;
    plain.cbData=0;
    data_desc=NULL;

    /* without entropy */
    SetLastError(0xDEADBEEF);
    okay = pCryptUnprotectData(&cipher,&data_desc,NULL,NULL,NULL,0,&plain);
    ok(okay,"Decrypting without entropy\n");

    ok(plain.pbData!=NULL,"Plain DATA_BLOB missing data\n");
    ok(plain.cbData==strlen(secret)+1,"Plain DATA_BLOB wrong length\n");
    ok(!strcmp((const char*)plain.pbData,secret),"Plain does not match secret\n");
    ok(data_desc!=NULL,"Description not allocated\n");
    ok(!lstrcmpW(data_desc,desc),"Description does not match\n");

    LocalFree(plain.pbData);
    LocalFree(data_desc);

    plain.pbData=NULL;
    plain.cbData=0;
    data_desc=NULL;

    /* with wrong entropy */
    SetLastError(0xDEADBEEF);
    okay = pCryptUnprotectData(&cipher_entropy,&data_desc,&cipher_entropy,NULL,NULL,0,&plain);
    ok(!okay,"Decrypting with wrong entropy\n");
    r = GetLastError();
    ok(r == ERROR_INVALID_DATA, "Wrong (%u) GetLastError seen\n",r);

    /* with entropy */
    SetLastError(0xDEADBEEF);
    okay = pCryptUnprotectData(&cipher_entropy,&data_desc,&entropy,NULL,NULL,0,&plain);
    ok(okay,"Decrypting with entropy\n");

    ok(plain.pbData!=NULL,"Plain DATA_BLOB missing data\n");
    ok(plain.cbData==strlen(secret)+1,"Plain DATA_BLOB wrong length\n");
    ok(!strcmp((const char*)plain.pbData,secret),"Plain does not match secret\n");
    ok(data_desc!=NULL,"Description not allocated\n");
    ok(!lstrcmpW(data_desc,desc),"Description does not match\n");

    LocalFree(plain.pbData);
    LocalFree(data_desc);

    plain.pbData=NULL;
    plain.cbData=0;
    data_desc=NULL;

    /* with entropy but no description */
    SetLastError(0xDEADBEEF);
    okay = pCryptUnprotectData(&cipher_no_desc,&data_desc,&entropy,NULL,NULL,0,&plain);
    ok(okay,"Decrypting with entropy and no description\n");

    ok(plain.pbData!=NULL,"Plain DATA_BLOB missing data\n");
    ok(plain.cbData==strlen(secret2)+1,"Plain DATA_BLOB wrong length\n");
    ok(!strcmp((const char*)plain.pbData,secret2),"Plain does not match secret\n");
    ok(data_desc!=NULL,"Description not allocated\n");
    ok(data_desc[0]=='\0',"Description not empty\n");

    LocalFree(data_desc);
    LocalFree(plain.pbData);

    plain.pbData=NULL;
    plain.cbData=0;
}

START_TEST(protectdata)
{
    HMODULE hCrypt32 = GetModuleHandleA("crypt32.dll");
    hCrypt32 = GetModuleHandleA("crypt32.dll");
    pCryptProtectData = (void*)GetProcAddress(hCrypt32, "CryptProtectData");
    pCryptUnprotectData = (void*)GetProcAddress(hCrypt32, "CryptUnprotectData");
    if (!pCryptProtectData || !pCryptUnprotectData)
    {
        skip("Crypt(Un)ProtectData() is not available\n");
        return;
    }

    protected=FALSE;
    test_cryptprotectdata();
    test_cryptunprotectdata();

    /* deinit globals here */
    if (cipher.pbData) LocalFree(cipher.pbData);
    if (cipher_entropy.pbData) LocalFree(cipher_entropy.pbData);
    if (cipher_no_desc.pbData) LocalFree(cipher_no_desc.pbData);
}
