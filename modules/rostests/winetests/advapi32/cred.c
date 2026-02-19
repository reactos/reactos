/*
 * Credential Function Tests
 *
 * Copyright 2007 Robert Shearman
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wincred.h"

#include "wine/test.h"

static BOOL (WINAPI *pCredDeleteA)(LPCSTR,DWORD,DWORD);
static BOOL (WINAPI *pCredEnumerateA)(LPCSTR,DWORD,DWORD *,PCREDENTIALA **);
static VOID (WINAPI *pCredFree)(PVOID);
static BOOL (WINAPI *pCredGetSessionTypes)(DWORD,LPDWORD);
static BOOL (WINAPI *pCredReadA)(LPCSTR,DWORD,DWORD,PCREDENTIALA *);
static BOOL (WINAPI *pCredRenameA)(LPCSTR,LPCSTR,DWORD,DWORD);
static BOOL (WINAPI *pCredWriteA)(PCREDENTIALA,DWORD);
static BOOL (WINAPI *pCredReadDomainCredentialsA)(PCREDENTIAL_TARGET_INFORMATIONA,DWORD,DWORD*,PCREDENTIALA**);
static BOOL (WINAPI *pCredMarshalCredentialA)(CRED_MARSHAL_TYPE,PVOID,LPSTR *);
static BOOL (WINAPI *pCredUnmarshalCredentialA)(LPCSTR,PCRED_MARSHAL_TYPE,PVOID);
static BOOL (WINAPI *pCredIsMarshaledCredentialA)(LPCSTR);

#define TEST_TARGET_NAME  "credtest.winehq.org"
#define TEST_TARGET_NAME2 "credtest2.winehq.org"
static const WCHAR TEST_PASSWORD[] = {'p','4','$','$','w','0','r','d','!',0};

static void test_CredReadA(void)
{
    BOOL ret;
    PCREDENTIALA cred;

    SetLastError(0xdeadbeef);
    ret = pCredReadA(TEST_TARGET_NAME, -1, 0, &cred);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "CredReadA should have failed with ERROR_INVALID_PARAMETER instead of %ld\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    ret = pCredReadA(TEST_TARGET_NAME, CRED_TYPE_GENERIC, 0xdeadbeef, &cred);
    ok(!ret && ( GetLastError() == ERROR_INVALID_FLAGS || GetLastError() == ERROR_INVALID_PARAMETER ),
        "CredReadA should have failed with ERROR_INVALID_FLAGS or ERROR_INVALID_PARAMETER instead of %ld\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    ret = pCredReadA(NULL, CRED_TYPE_GENERIC, 0, &cred);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "CredReadA should have failed with ERROR_INVALID_PARAMETER instead of %ld\n",
        GetLastError());
}

static void test_CredWriteA(void)
{
    CREDENTIALA new_cred;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = pCredWriteA(NULL, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "CredWriteA should have failed with ERROR_INVALID_PARAMETER instead of %ld\n",
        GetLastError());

    new_cred.Flags = 0;
    new_cred.Type = CRED_TYPE_GENERIC;
    new_cred.TargetName = NULL;
    new_cred.Comment = (char *)"Comment";
    new_cred.CredentialBlobSize = 0;
    new_cred.CredentialBlob = NULL;
    new_cred.Persist = CRED_PERSIST_ENTERPRISE;
    new_cred.AttributeCount = 0;
    new_cred.Attributes = NULL;
    new_cred.TargetAlias = NULL;
    new_cred.UserName = (char *)"winetest";

    SetLastError(0xdeadbeef);
    ret = pCredWriteA(&new_cred, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "CredWriteA should have failed with ERROR_INVALID_PARAMETER instead of %ld\n",
        GetLastError());

    new_cred.TargetName = (char *)TEST_TARGET_NAME;
    new_cred.Type = CRED_TYPE_DOMAIN_PASSWORD;

    SetLastError(0xdeadbeef);
    ret = pCredWriteA(&new_cred, 0);
    if (ret)
    {
        ok(GetLastError() == ERROR_SUCCESS ||
           GetLastError() == ERROR_IO_PENDING, /* Vista */
           "Expected ERROR_IO_PENDING, got %ld\n", GetLastError());
    }
    else
    {
        ok(GetLastError() == ERROR_BAD_USERNAME ||
           GetLastError() == ERROR_NO_SUCH_LOGON_SESSION, /* Vista */
           "CredWrite with username without domain should return ERROR_BAD_USERNAME"
           "or ERROR_NO_SUCH_LOGON_SESSION not %ld\n", GetLastError());
    }

    new_cred.UserName = NULL;
    SetLastError(0xdeadbeef);
    ret = pCredWriteA(&new_cred, 0);
    ok(!ret && GetLastError() == ERROR_BAD_USERNAME,
        "CredWriteA with NULL username should have failed with ERROR_BAD_USERNAME instead of %ld\n",
        GetLastError());

    new_cred.UserName = (char *)"winetest";
    new_cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    SetLastError(0xdeadbeef);
    ret = pCredWriteA(&new_cred, 0);
    ok(ret || broken(!ret), "CredWriteA failed with error %lu\n", GetLastError());
    if (ret)
    {
        ret = pCredDeleteA(TEST_TARGET_NAME, CRED_TYPE_DOMAIN_PASSWORD, 0);
        ok(ret, "CredDeleteA failed with error %lu\n", GetLastError());
    }
    new_cred.Type = CRED_TYPE_GENERIC;
    SetLastError(0xdeadbeef);
    ret = pCredWriteA(&new_cred, 0);
    ok(ret || broken(!ret), "CredWriteA failed with error %lu\n", GetLastError());
    if  (ret)
    {
        ret = pCredDeleteA(TEST_TARGET_NAME, CRED_TYPE_GENERIC, 0);
        ok(ret, "CredDeleteA failed with error %lu\n", GetLastError());
    }
    new_cred.Persist = CRED_PERSIST_SESSION;
    ret = pCredWriteA(&new_cred, 0);
    ok(ret, "CredWriteA failed with error %lu\n", GetLastError());

    ret = pCredDeleteA(TEST_TARGET_NAME, CRED_TYPE_GENERIC, 0);
    ok(ret, "CredDeleteA failed with error %lu\n", GetLastError());

    new_cred.Type = CRED_TYPE_DOMAIN_PASSWORD;
    SetLastError(0xdeadbeef);
    ret = pCredWriteA(&new_cred, 0);
    ok(ret || broken(!ret), "CredWriteA failed with error %lu\n", GetLastError());
    if (ret)
    {
        ret = pCredDeleteA(TEST_TARGET_NAME, CRED_TYPE_DOMAIN_PASSWORD, 0);
        ok(ret, "CredDeleteA failed with error %lu\n", GetLastError());
    }
    new_cred.UserName = NULL;
    SetLastError(0xdeadbeef);
    ret = pCredWriteA(&new_cred, 0);
    ok(!ret, "CredWriteA succeeded\n");
    ok(GetLastError() == ERROR_BAD_USERNAME, "got %lu\n", GetLastError());
}

static void test_CredDeleteA(void)
{
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = pCredDeleteA(TEST_TARGET_NAME, -1, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "CredDeleteA should have failed with ERROR_INVALID_PARAMETER instead of %ld\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    ret = pCredDeleteA(TEST_TARGET_NAME, CRED_TYPE_GENERIC, 0xdeadbeef);
    ok(!ret && ( GetLastError() == ERROR_INVALID_FLAGS || GetLastError() == ERROR_INVALID_PARAMETER /* Vista */ ),
        "CredDeleteA should have failed with ERROR_INVALID_FLAGS or ERROR_INVALID_PARAMETER instead of %ld\n",
        GetLastError());
}

static void test_CredReadDomainCredentialsA(void)
{
    BOOL ret;
    char target_name[] = "no_such_target";
    CREDENTIAL_TARGET_INFORMATIONA info = {target_name, NULL, target_name, NULL, NULL, NULL, NULL, 0, 0, NULL};
    DWORD count;
    PCREDENTIALA* creds;

    if (!pCredReadDomainCredentialsA)
    {
        win_skip("CredReadDomainCredentialsA() is not implemented\n");
        return;
    }

    /* these two tests would crash on both native and Wine. Implementations
     * does not check for NULL output pointers and try to zero them out early */
if(0)
{
    ret = pCredReadDomainCredentialsA(&info, 0, NULL, &creds);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "!\n");
    ret = pCredReadDomainCredentialsA(&info, 0, &count, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "!\n");
}

    SetLastError(0xdeadbeef);
    ret = pCredReadDomainCredentialsA(NULL, 0, &count, &creds);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "CredReadDomainCredentialsA should have failed with ERROR_INVALID_PARAMETER instead of %ld\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    creds = (void*)0x12345;
    count = 2;
    ret = pCredReadDomainCredentialsA(&info, 0, &count, &creds);
    ok(!ret && GetLastError() == ERROR_NOT_FOUND,
        "CredReadDomainCredentialsA should have failed with ERROR_NOT_FOUND instead of %ld\n",
        GetLastError());
    ok(count ==0 && creds == NULL, "CredReadDomainCredentialsA must not return any result\n");

    info.TargetName = NULL;

    SetLastError(0xdeadbeef);
    ret = pCredReadDomainCredentialsA(&info, 0, &count, &creds);
    ok(!ret, "CredReadDomainCredentialsA should have failed\n");
    ok(GetLastError() == ERROR_NOT_FOUND ||
        GetLastError() == ERROR_INVALID_PARAMETER, /* Vista, W2K8 */
        "Expected ERROR_NOT_FOUND or ERROR_INVALID_PARAMETER instead of %ld\n",
        GetLastError());

    info.DnsServerName = NULL;

    SetLastError(0xdeadbeef);
    ret = pCredReadDomainCredentialsA(&info, 0, &count, &creds);
    ok(!ret, "CredReadDomainCredentialsA should have failed\n");
    ok(GetLastError() == ERROR_NOT_FOUND ||
        GetLastError() == ERROR_INVALID_PARAMETER, /* Vista, W2K8 */
        "Expected ERROR_NOT_FOUND or ERROR_INVALID_PARAMETER instead of %ld\n",
        GetLastError());
}

static void check_blob(int line, DWORD cred_type, PCREDENTIALA cred)
{
    if (cred_type == CRED_TYPE_DOMAIN_PASSWORD)
    {
        todo_wine
        ok_(__FILE__, line)(cred->CredentialBlobSize == 0, "expected CredentialBlobSize of 0 but got %ld\n", cred->CredentialBlobSize);
        todo_wine
        ok_(__FILE__, line)(!cred->CredentialBlob, "expected NULL credentials but got %p\n", cred->CredentialBlob);
    }
    else
    {
        DWORD size=sizeof(TEST_PASSWORD);
        ok_(__FILE__, line)(cred->CredentialBlobSize == size, "expected CredentialBlobSize of %lu but got %lu\n", size, cred->CredentialBlobSize);
        ok_(__FILE__, line)(cred->CredentialBlob != NULL, "CredentialBlob should be present\n");
        if (cred->CredentialBlob)
            ok_(__FILE__, line)(!memcmp(cred->CredentialBlob, TEST_PASSWORD, size), "wrong CredentialBlob\n");
    }
}

static void test_generic(void)
{
    BOOL ret;
    DWORD count, i;
    PCREDENTIALA *creds;
    CREDENTIALA new_cred;
    PCREDENTIALA cred;
    BOOL found = FALSE;

    new_cred.Flags = 0;
    new_cred.Type = CRED_TYPE_GENERIC;
    new_cred.TargetName = (char *)TEST_TARGET_NAME;
    new_cred.Comment = (char *)"Comment";
    new_cred.CredentialBlobSize = sizeof(TEST_PASSWORD);
    new_cred.CredentialBlob = (LPBYTE)TEST_PASSWORD;
    new_cred.Persist = CRED_PERSIST_ENTERPRISE;
    new_cred.AttributeCount = 0;
    new_cred.Attributes = NULL;
    new_cred.TargetAlias = NULL;
    new_cred.UserName = (char *)"winetest";

    ret = pCredWriteA(&new_cred, 0);
    ok(ret || broken(GetLastError() == ERROR_NO_SUCH_LOGON_SESSION),
       "CredWriteA failed with error %ld\n", GetLastError());
    if (!ret)
    {
        skip("couldn't write generic credentials, skipping tests\n");
        return;
    }

    ret = pCredEnumerateA(NULL, 0, &count, &creds);
    ok(ret, "CredEnumerateA failed with error %ld\n", GetLastError());

    for (i = 0; i < count; i++)
    {
        if (creds[i]->TargetName && !strcmp(creds[i]->TargetName, TEST_TARGET_NAME))
        {
            ok(creds[i]->Type == CRED_TYPE_GENERIC ||
               creds[i]->Type == CRED_TYPE_DOMAIN_PASSWORD, /* Vista */
               "expected creds[%ld]->Type CRED_TYPE_GENERIC or CRED_TYPE_DOMAIN_PASSWORD but got %ld\n", i, creds[i]->Type);
            ok(!creds[i]->Flags, "expected creds[%ld]->Flags 0 but got 0x%lx\n", i, creds[i]->Flags);
            ok(!strcmp(creds[i]->Comment, "Comment"), "expected creds[%ld]->Comment \"Comment\" but got \"%s\"\n", i, creds[i]->Comment);
            check_blob(__LINE__, creds[i]->Type, creds[i]);
            ok(creds[i]->Persist, "expected creds[%ld]->Persist CRED_PERSIST_ENTERPRISE but got %ld\n", i, creds[i]->Persist);
            ok(!strcmp(creds[i]->UserName, "winetest"), "expected creds[%ld]->UserName \"winetest\" but got \"%s\"\n", i, creds[i]->UserName);
            found = TRUE;
        }
    }
    pCredFree(creds);
    ok(found, "credentials not found\n");

    ret = pCredReadA(TEST_TARGET_NAME, CRED_TYPE_GENERIC, 0, &cred);
    ok(ret, "CredReadA failed with error %ld\n", GetLastError());
    pCredFree(cred);

    ret = pCredDeleteA(TEST_TARGET_NAME, CRED_TYPE_GENERIC, 0);
    ok(ret, "CredDeleteA failed with error %ld\n", GetLastError());
}

static void test_domain_password(DWORD cred_type)
{
    BOOL ret;
    DWORD count, i;
    PCREDENTIALA *creds;
    CREDENTIALA new_cred;
    PCREDENTIALA cred;
    BOOL found = FALSE;

    new_cred.Flags = 0;
    new_cred.Type = cred_type;
    new_cred.TargetName = (char *)TEST_TARGET_NAME;
    new_cred.Comment = (char *)"Comment";
    new_cred.CredentialBlobSize = sizeof(TEST_PASSWORD);
    new_cred.CredentialBlob = (LPBYTE)TEST_PASSWORD;
    new_cred.Persist = CRED_PERSIST_ENTERPRISE;
    new_cred.AttributeCount = 0;
    new_cred.Attributes = NULL;
    new_cred.TargetAlias = NULL;
    new_cred.UserName = (char *)"test\\winetest";
    ret = pCredWriteA(&new_cred, 0);
    if (!ret && GetLastError() == ERROR_NO_SUCH_LOGON_SESSION)
    {
        skip("CRED_TYPE_DOMAIN_PASSWORD credentials are not supported "
             "or are disabled. Skipping\n");
        return;
    }
    ok(ret, "CredWriteA failed with error %ld\n", GetLastError());

    ret = pCredEnumerateA(NULL, 0, &count, &creds);
    ok(ret, "CredEnumerateA failed with error %ld\n", GetLastError());

    for (i = 0; i < count; i++)
    {
        if (creds[i]->TargetName && !strcmp(creds[i]->TargetName, TEST_TARGET_NAME))
        {
            ok(creds[i]->Type == cred_type, "expected creds[%ld]->Type CRED_TYPE_DOMAIN_PASSWORD but got %ld\n", i, creds[i]->Type);
            ok(!creds[i]->Flags, "expected creds[%ld]->Flags 0 but got 0x%lx\n", i, creds[i]->Flags);
            ok(!strcmp(creds[i]->Comment, "Comment"), "expected creds[%ld]->Comment \"Comment\" but got \"%s\"\n", i, creds[i]->Comment);
            check_blob(__LINE__, cred_type, creds[i]);
            ok(creds[i]->Persist, "expected creds[%ld]->Persist CRED_PERSIST_ENTERPRISE but got %ld\n", i, creds[i]->Persist);
            ok(!strcmp(creds[i]->UserName, "test\\winetest"), "expected creds[%ld]->UserName \"winetest\" but got \"%s\"\n", i, creds[i]->UserName);
            found = TRUE;
        }
    }
    pCredFree(creds);
    ok(found, "credentials not found\n");

    ret = pCredReadA(TEST_TARGET_NAME, cred_type, 0, &cred);
    ok(ret, "CredReadA failed with error %ld\n", GetLastError());
    if (ret)  /* don't check the values of cred, if CredReadA failed. */
    {
        check_blob(__LINE__, cred_type, cred);
        pCredFree(cred);
    }

    ret = pCredDeleteA(TEST_TARGET_NAME, cred_type, 0);
    ok(ret, "CredDeleteA failed with error %ld\n", GetLastError());
}

static void test_CredMarshalCredentialA(void)
{
    static WCHAR emptyW[] = {0};
    static WCHAR tW[] = {'t',0};
    static WCHAR teW[] = {'t','e',0};
    static WCHAR tesW[] = {'t','e','s',0};
    static WCHAR testW[] = {'t','e','s','t',0};
    static WCHAR test1W[] = {'t','e','s','t','1',0};
    CERT_CREDENTIAL_INFO cert;
    USERNAME_TARGET_CREDENTIAL_INFO username;
    DWORD error;
    char *str;
    BOOL ret;

    SetLastError( 0xdeadbeef );
    ret = pCredMarshalCredentialA( 0, NULL, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %lu\n", error );

    memset( cert.rgbHashOfCert, 0, sizeof(cert.rgbHashOfCert) );
    cert.cbSize = sizeof(cert);
    SetLastError( 0xdeadbeef );
    ret = pCredMarshalCredentialA( 0, &cert, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %lu\n", error );

    str = (char *)0xdeadbeef;
    SetLastError( 0xdeadbeef );
    ret = pCredMarshalCredentialA( 0, &cert, &str );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %lu\n", error );
    ok( str == (char *)0xdeadbeef, "got %p\n", str );

    SetLastError( 0xdeadbeef );
    ret = pCredMarshalCredentialA( CertCredential, NULL, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %lu\n", error );

    if (0) { /* crash */
    SetLastError( 0xdeadbeef );
    ret = pCredMarshalCredentialA( CertCredential, &cert, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %lu\n", error );
    }

    cert.cbSize = 0;
    str = (char *)0xdeadbeef;
    SetLastError( 0xdeadbeef );
    ret = pCredMarshalCredentialA( CertCredential, &cert, &str );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %lu\n", error );
    ok( str == (char *)0xdeadbeef, "got %p\n", str );

    cert.cbSize = sizeof(cert) + 4;
    str = NULL;
    ret = pCredMarshalCredentialA( CertCredential, &cert, &str );
    ok( ret, "unexpected failure %lu\n", GetLastError() );
    ok( str != NULL, "str not set\n" );
    ok( !lstrcmpA( str, "@@BAAAAAAAAAAAAAAAAAAAAAAAAAAA" ), "got %s\n", str );
    pCredFree( str );

    cert.cbSize = sizeof(cert);
    cert.rgbHashOfCert[0] = 2;
    str = NULL;
    ret = pCredMarshalCredentialA( CertCredential, &cert, &str );
    ok( ret, "unexpected failure %lu\n", GetLastError() );
    ok( str != NULL, "str not set\n" );
    ok( !lstrcmpA( str, "@@BCAAAAAAAAAAAAAAAAAAAAAAAAAA" ), "got %s\n", str );
    pCredFree( str );

    cert.rgbHashOfCert[0] = 255;
    str = NULL;
    ret = pCredMarshalCredentialA( CertCredential, &cert, &str );
    ok( ret, "unexpected failure %lu\n", GetLastError() );
    ok( str != NULL, "str not set\n" );
    ok( !lstrcmpA( str, "@@B-DAAAAAAAAAAAAAAAAAAAAAAAAA" ), "got %s\n", str );
    pCredFree( str );

    cert.rgbHashOfCert[0] = 1;
    cert.rgbHashOfCert[1] = 1;
    str = NULL;
    ret = pCredMarshalCredentialA( CertCredential, &cert, &str );
    ok( ret, "unexpected failure %lu\n", GetLastError() );
    ok( str != NULL, "str not set\n" );
    ok( !lstrcmpA( str, "@@BBEAAAAAAAAAAAAAAAAAAAAAAAAA" ), "got %s\n", str );
    pCredFree( str );

    cert.rgbHashOfCert[0] = 1;
    cert.rgbHashOfCert[1] = 1;
    cert.rgbHashOfCert[2] = 1;
    str = NULL;
    ret = pCredMarshalCredentialA( CertCredential, &cert, &str );
    ok( ret, "unexpected failure %lu\n", GetLastError() );
    ok( str != NULL, "str not set\n" );
    ok( !lstrcmpA( str, "@@BBEQAAAAAAAAAAAAAAAAAAAAAAAA" ), "got %s\n", str );
    pCredFree( str );

    memset( cert.rgbHashOfCert, 0, sizeof(cert.rgbHashOfCert) );
    cert.rgbHashOfCert[0] = 'W';
    cert.rgbHashOfCert[1] = 'i';
    cert.rgbHashOfCert[2] = 'n';
    cert.rgbHashOfCert[3] = 'e';
    str = NULL;
    ret = pCredMarshalCredentialA( CertCredential, &cert, &str );
    ok( ret, "unexpected failure %lu\n", GetLastError() );
    ok( str != NULL, "str not set\n" );
    ok( !lstrcmpA( str, "@@BXlmblBAAAAAAAAAAAAAAAAAAAAA" ), "got %s\n", str );
    pCredFree( str );

    memset( cert.rgbHashOfCert, 0xff, sizeof(cert.rgbHashOfCert) );
    str = NULL;
    ret = pCredMarshalCredentialA( CertCredential, &cert, &str );
    ok( ret, "unexpected failure %lu\n", GetLastError() );
    ok( str != NULL, "str not set\n" );
    ok( !lstrcmpA( str, "@@B--------------------------P" ), "got %s\n", str );
    pCredFree( str );

    username.UserName = NULL;
    str = (char *)0xdeadbeef;
    SetLastError( 0xdeadbeef );
    ret = pCredMarshalCredentialA( UsernameTargetCredential, &username, &str );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %lu\n", error );
    ok( str == (char *)0xdeadbeef, "got %p\n", str );

    username.UserName = emptyW;
    str = (char *)0xdeadbeef;
    SetLastError( 0xdeadbeef );
    ret = pCredMarshalCredentialA( UsernameTargetCredential, &username, &str );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %lu\n", error );
    ok( str == (char *)0xdeadbeef, "got %p\n", str );

    username.UserName = tW;
    str = NULL;
    ret = pCredMarshalCredentialA( UsernameTargetCredential, &username, &str );
    ok( ret, "unexpected failure %lu\n", GetLastError() );
    ok( str != NULL, "str not set\n" );
    ok( !lstrcmpA( str, "@@CCAAAAA0BA" ), "got %s\n", str );
    pCredFree( str );

    username.UserName = teW;
    str = NULL;
    ret = pCredMarshalCredentialA( UsernameTargetCredential, &username, &str );
    ok( ret, "unexpected failure %lu\n", GetLastError() );
    ok( str != NULL, "str not set\n" );
    ok( !lstrcmpA( str, "@@CEAAAAA0BQZAA" ), "got %s\n", str );
    pCredFree( str );

    username.UserName = tesW;
    str = NULL;
    ret = pCredMarshalCredentialA( UsernameTargetCredential, &username, &str );
    ok( ret, "unexpected failure %lu\n", GetLastError() );
    ok( str != NULL, "str not set\n" );
    ok( !lstrcmpA( str, "@@CGAAAAA0BQZAMHA" ), "got %s\n", str );
    pCredFree( str );

    username.UserName = testW;
    str = NULL;
    ret = pCredMarshalCredentialA( UsernameTargetCredential, &username, &str );
    ok( ret, "unexpected failure %lu\n", GetLastError() );
    ok( str != NULL, "str not set\n" );
    ok( !lstrcmpA( str, "@@CIAAAAA0BQZAMHA0BA" ), "got %s\n", str );
    pCredFree( str );

    username.UserName = test1W;
    str = NULL;
    ret = pCredMarshalCredentialA( UsernameTargetCredential, &username, &str );
    ok( ret, "unexpected failure %lu\n", GetLastError() );
    ok( str != NULL, "str not set\n" );
    ok( !lstrcmpA( str, "@@CKAAAAA0BQZAMHA0BQMAA" ), "got %s\n", str );
    pCredFree( str );
}

static void test_CredUnmarshalCredentialA(void)
{
    static const UCHAR cert_empty[CERT_HASH_LENGTH] = {0};
    static const UCHAR cert_wine[CERT_HASH_LENGTH] = {'W','i','n','e',0};
    static const WCHAR tW[] = {'t',0};
    static const WCHAR teW[] = {'t','e',0};
    static const WCHAR tesW[] = {'t','e','s',0};
    static const WCHAR testW[] = {'t','e','s','t',0};
    void *p;
    CERT_CREDENTIAL_INFO *cert;
    const UCHAR *hash;
    USERNAME_TARGET_CREDENTIAL_INFO *username;
    CRED_MARSHAL_TYPE type;
    unsigned int i, j;
    DWORD error;
    BOOL ret;
    const struct {
        const char *cred;
        CRED_MARSHAL_TYPE type;
        const void *unmarshaled;
    } tests[] = {
        { "", 0, NULL },
        { "@", 0, NULL },
        { "@@", 0, NULL },
        { "@@@", 0, NULL },
        { "@@A", 0, NULL },
        { "@@E", 4, NULL },
        { "@@Z", 25, NULL },
        { "@@a", 26, NULL },
        { "@@0", 52, NULL },
        { "@@#", 62, NULL },
        { "@@-", 63, NULL },
        { "@@B", CertCredential, NULL },
        { "@@BA", CertCredential, NULL },
        { "@@BAAAAAAAAAAAAAAAAAAAAAAAAAA", CertCredential, NULL },
        { "@@BAAAAAAAAAAAAAAAAAAAAAAAAAAAA", CertCredential, NULL },
        { "@@BAAAAAAAAAAAAAAAAAAAAAAAAAAA", CertCredential, cert_empty },
        { "@@BXlmblBAAAAAAAAAAAAAAAAAAAAA", CertCredential, cert_wine },
        { "@@C", UsernameTargetCredential, NULL },
        { "@@CA", UsernameTargetCredential, NULL },
        { "@@CAAAAAA", UsernameTargetCredential, NULL },
        { "@@CAAAAAA0B", UsernameTargetCredential, NULL },
        { "@@CAAAAAA0BA", UsernameTargetCredential, NULL },
        { "@@CCAAAAA0BA", UsernameTargetCredential, tW },
        { "@@CEAAAAA0BA", UsernameTargetCredential, NULL },
        { "@@CEAAAAA0BAd", UsernameTargetCredential, NULL },
        { "@@CEAAAAA0BAdA", UsernameTargetCredential, NULL },
        { "@@CEAAAAA0BQZAA", UsernameTargetCredential, teW },
        { "@@CEAAAAA0BQZAQ", UsernameTargetCredential, teW },
        { "@@CEAAAAA0BQZAg", UsernameTargetCredential, teW },
        { "@@CEAAAAA0BQZAw", UsernameTargetCredential, teW },
        { "@@CEAAAAA0BQZAAA", UsernameTargetCredential, NULL },
        { "@@CGAAAAA0BQZAMH", UsernameTargetCredential, NULL },
        { "@@CGAAAAA0BQZAMHA", UsernameTargetCredential, tesW },
        { "@@CGAAAAA0BQZAMHAA", UsernameTargetCredential, NULL },
        { "@@CCAAAAA0BAA", UsernameTargetCredential, NULL },
        { "@@CBAAAAA0BAA", UsernameTargetCredential, NULL },
        { "@@CAgAAAA0BAA", UsernameTargetCredential, NULL },
        { "@@CIAAAAA0BQZAMHA0BA", UsernameTargetCredential, testW },
        { "@@CA-----0BQZAMHA0BA", UsernameTargetCredential, NULL },
    };

    SetLastError( 0xdeadbeef );
    ret = pCredUnmarshalCredentialA( NULL, NULL, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %lu\n", error );

    cert = NULL;
    SetLastError( 0xdeadbeef );
    ret = pCredUnmarshalCredentialA( NULL, NULL, (void **)&cert );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %lu\n", error );

    type = 0;
    cert = NULL;
    SetLastError( 0xdeadbeef );
    ret = pCredUnmarshalCredentialA( NULL, &type, (void **)&cert );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %lu\n", error );

    if (0) { /* crash */
    SetLastError( 0xdeadbeef );
    ret = pCredUnmarshalCredentialA( "@@BAAAAAAAAAAAAAAAAAAAAAAAAAAA", &type, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %lu\n", error );

    SetLastError( 0xdeadbeef );
    ret = pCredUnmarshalCredentialA( "@@BAAAAAAAAAAAAAAAAAAAAAAAAAAA", NULL, (void **)&cert );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %lu\n", error );
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        SetLastError(0xdeadbeef);
        type = 0;
        p = NULL;
        ret = pCredUnmarshalCredentialA(tests[i].cred, &type, &p);
        error = GetLastError();
        if (tests[i].unmarshaled)
        {
            ok(ret, "[%u] unexpected failure %lu\n", i, error);
            ok(type == tests[i].type, "[%u] got %u\n", i, type);
            ok(p != NULL, "[%u] returned pointer is NULL\n", i);
            if (tests[i].type == CertCredential)
            {
                cert = p;
                hash = tests[i].unmarshaled;
                ok(cert->cbSize == sizeof(*cert),
                   "[%u] wrong size %lu\n", i, cert->cbSize);
                for (j = 0; j < sizeof(cert->rgbHashOfCert); j++)
                    ok(cert->rgbHashOfCert[j] == hash[j], "[%u] wrong data\n", i);
            }
            else if (tests[i].type == UsernameTargetCredential)
            {
                username = p;
                ok(username->UserName != NULL, "[%u] UserName is NULL\n", i);
                ok(!lstrcmpW(username->UserName, tests[i].unmarshaled),
                   "[%u] got %s\n", i, wine_dbgstr_w(username->UserName));
            }
        }
        else
        {
            ok(!ret, "[%u] unexpected success\n", i);
            ok(error == ERROR_INVALID_PARAMETER, "[%u] got %lu\n", i, error);
            ok(type == tests[i].type, "[%u] got %u\n", i, type);
            ok(p == NULL, "[%u] returned pointer is not NULL\n", i);
        }

        if (ret)
            pCredFree(p);
    }
}

static void test_CredIsMarshaledCredentialA(void)
{
    int i;
    BOOL res;
    BOOL expected = TRUE;

    const char * ptr[] = {
        /* CertCredential */
        "@@BXlmblBAAAAAAAAAAAAAAAAAAAAA",   /* hash for 'W','i','n','e' */
        "@@BAAAAAAAAAAAAAAAAAAAAAAAAAAA",   /* hash for all 0 */

        /* UsernameTargetCredential */
        "@@CCAAAAA0BA",                     /* "t" */
        "@@CIAAAAA0BQZAMHA0BA",             /* "test" */

        /* todo: BinaryBlobCredential */

        /* not marshaled names return always FALSE */
        "winetest",
        "",
        "@@",
        "@@A",
        "@@AA",
        "@@AAA",
        "@@B",
        "@@BB",
        "@@BBB",

        /* CertCredential */
        "@@BAAAAAAAAAAAAAAAAAAAAAAAAAAAA",  /* to long */
        "@@BAAAAAAAAAAAAAAAAAAAAAAAAAA",    /* to short */
        "@@BAAAAAAAAAAAAAAAAAAAAAAAAAA+",   /* bad char */
        "@@BAAAAAAAAAAAAAAAAAAAAAAAAAA:",
        "@@BAAAAAAAAAAAAAAAAAAAAAAAAAA>",
        "@@BAAAAAAAAAAAAAAAAAAAAAAAAAA<",

        "@@C",
        "@@CC",
        "@@CCC",
        "@@D",
        "@@DD",
        "@@DDD",
        NULL};

    for (i = 0; ptr[i]; i++)
    {
        if (*ptr[i] != '@')
            expected = FALSE;

        SetLastError(0xdeadbeef);
        res = pCredIsMarshaledCredentialA(ptr[i]);
        if (expected)
            ok(res != FALSE, "%d: got %d and %lu for %s (expected TRUE)\n", i, res, GetLastError(), ptr[i]);
        else
        {
            /* Windows returns ERROR_INVALID_PARAMETER here, but that's not documented */
            ok(!res, "%d: got %d and %lu for %s (expected FALSE)\n", i, res, GetLastError(), ptr[i]);
        }
    }
}

START_TEST(cred)
{
    DWORD persists[CRED_TYPE_MAXIMUM];
    HMODULE mod = GetModuleHandleA("advapi32.dll");

    pCredEnumerateA = (void *)GetProcAddress(mod, "CredEnumerateA");
    pCredFree = (void *)GetProcAddress(mod, "CredFree");
    pCredGetSessionTypes = (void *)GetProcAddress(mod, "CredGetSessionTypes");
    pCredWriteA = (void *)GetProcAddress(mod, "CredWriteA");
    pCredDeleteA = (void *)GetProcAddress(mod, "CredDeleteA");
    pCredReadA = (void *)GetProcAddress(mod, "CredReadA");
    pCredRenameA = (void *)GetProcAddress(mod, "CredRenameA");
    pCredReadDomainCredentialsA = (void *)GetProcAddress(mod, "CredReadDomainCredentialsA");
    pCredMarshalCredentialA = (void *)GetProcAddress(mod, "CredMarshalCredentialA");
    pCredUnmarshalCredentialA = (void *)GetProcAddress(mod, "CredUnmarshalCredentialA");
    pCredIsMarshaledCredentialA = (void *)GetProcAddress(mod, "CredIsMarshaledCredentialA");

    if (!pCredEnumerateA || !pCredFree || !pCredWriteA || !pCredDeleteA || !pCredReadA)
    {
        win_skip("credentials functions not present in advapi32.dll\n");
        return;
    }

    if (pCredGetSessionTypes)
    {
        BOOL ret;
        DWORD i;
        ret = pCredGetSessionTypes(CRED_TYPE_MAXIMUM, persists);
        ok(ret, "CredGetSessionTypes failed with error %ld\n", GetLastError());
        ok(persists[0] == CRED_PERSIST_NONE, "persists[0] = %lu instead of CRED_PERSIST_NONE\n", persists[0]);
        for (i=0; i < CRED_TYPE_MAXIMUM; i++)
            ok(persists[i] <= CRED_PERSIST_ENTERPRISE, "bad value for persists[%lu]: %lu\n", i, persists[i]);
    }
    else
        memset(persists, CRED_PERSIST_ENTERPRISE, sizeof(persists));

    test_CredReadA();
    test_CredWriteA();
    test_CredDeleteA();

    test_CredReadDomainCredentialsA();

    trace("generic:\n");
    if (persists[CRED_TYPE_GENERIC] == CRED_PERSIST_NONE)
        skip("CRED_TYPE_GENERIC credentials are not supported or are disabled. Skipping\n");
    else
        test_generic();

    trace("domain password:\n");
    if (persists[CRED_TYPE_DOMAIN_PASSWORD] == CRED_PERSIST_NONE)
        skip("CRED_TYPE_DOMAIN_PASSWORD credentials are not supported or are disabled. Skipping\n");
    else
        test_domain_password(CRED_TYPE_DOMAIN_PASSWORD);

    trace("domain visible password:\n");
    if (persists[CRED_TYPE_DOMAIN_VISIBLE_PASSWORD] == CRED_PERSIST_NONE)
        skip("CRED_TYPE_DOMAIN_VISIBLE_PASSWORD credentials are not supported or are disabled. Skipping\n");
    else
        test_domain_password(CRED_TYPE_DOMAIN_VISIBLE_PASSWORD);

    test_CredMarshalCredentialA();
    test_CredUnmarshalCredentialA();
    test_CredIsMarshaledCredentialA();
}
