/*
 * tests
 *
 * Copyright 2006 Robert Reif
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
#include <winnls.h>
#define SECURITY_WIN32
#include <security.h>
#include <schannel.h>

#include "wine/test.h"

static HMODULE secdll;

static SECURITY_STATUS (SEC_ENTRY *pSspiEncodeAuthIdentityAsStrings)
    (PSEC_WINNT_AUTH_IDENTITY_OPAQUE, PCWSTR *, PCWSTR *, PCWSTR *);
static SECURITY_STATUS (SEC_ENTRY *pSspiEncodeStringsAsAuthIdentity)
    (PCWSTR, PCWSTR, PCWSTR, PSEC_WINNT_AUTH_IDENTITY_OPAQUE *);
static void (SEC_ENTRY *pSspiFreeAuthIdentity)
    (PSEC_WINNT_AUTH_IDENTITY_OPAQUE);
static void (SEC_ENTRY *pSspiLocalFree)
    (void *);
static void (SEC_ENTRY *pSspiZeroAuthIdentity)
    (PSEC_WINNT_AUTH_IDENTITY_OPAQUE);

static BOOLEAN (WINAPI * pGetComputerObjectNameA)(EXTENDED_NAME_FORMAT NameFormat, LPSTR lpNameBuffer, PULONG lpnSize);
static BOOLEAN (WINAPI * pGetComputerObjectNameW)(EXTENDED_NAME_FORMAT NameFormat, LPWSTR lpNameBuffer, PULONG lpnSize);
static BOOLEAN (WINAPI * pGetUserNameExA)(EXTENDED_NAME_FORMAT NameFormat, LPSTR lpNameBuffer, PULONG lpnSize);
static BOOLEAN (WINAPI * pGetUserNameExW)(EXTENDED_NAME_FORMAT NameFormat, LPWSTR lpNameBuffer, PULONG lpnSize);
static PSecurityFunctionTableA (SEC_ENTRY * pInitSecurityInterfaceA)(void);
static PSecurityFunctionTableW (SEC_ENTRY * pInitSecurityInterfaceW)(void);

static EXTENDED_NAME_FORMAT formats[] = {
    NameUnknown, NameFullyQualifiedDN, NameSamCompatible, NameDisplay,
    NameUniqueId, NameCanonical, NameUserPrincipal, NameCanonicalEx,
    NameServicePrincipal, NameDnsDomain
};

static void testGetComputerObjectNameA(void)
{
    char name[256];
    ULONG size;
    BOOLEAN rc;
    UINT i;

    for (i = 0; i < (sizeof(formats) / sizeof(formats[0])); i++) {
        size = sizeof(name);
        ZeroMemory(name, sizeof(name));
        rc = pGetComputerObjectNameA(formats[i], name, &size);
        ok(rc || ((formats[i] == NameUnknown) &&
           (GetLastError() == ERROR_INVALID_PARAMETER)) ||
           (GetLastError() == ERROR_CANT_ACCESS_DOMAIN_INFO) ||
           (GetLastError() == ERROR_NO_SUCH_DOMAIN) ||
           (GetLastError() == ERROR_NO_SUCH_USER) ||
           (GetLastError() == ERROR_NONE_MAPPED) ||
           (GetLastError() == ERROR_ACCESS_DENIED),
           "GetComputerObjectNameA(%d) failed: %d\n",
           formats[i], GetLastError());
        if (rc)
            trace("GetComputerObjectNameA() returned %s\n", name);
    }
}

static void testGetComputerObjectNameW(void)
{
    WCHAR nameW[256];
    ULONG size;
    BOOLEAN rc;
    UINT i;

    for (i = 0; i < (sizeof(formats) / sizeof(formats[0])); i++) {
        size = sizeof(nameW)/sizeof(nameW[0]);
        ZeroMemory(nameW, sizeof(nameW));
        rc = pGetComputerObjectNameW(formats[i], nameW, &size);
        ok(rc || ((formats[i] == NameUnknown) &&
           (GetLastError() == ERROR_INVALID_PARAMETER)) ||
           (GetLastError() == ERROR_CANT_ACCESS_DOMAIN_INFO) ||
           (GetLastError() == ERROR_NO_SUCH_DOMAIN) ||
           (GetLastError() == ERROR_NO_SUCH_USER) ||
           (GetLastError() == ERROR_NONE_MAPPED) ||
           (GetLastError() == ERROR_ACCESS_DENIED),
           "GetComputerObjectNameW(%d) failed: %d\n",
           formats[i], GetLastError());
        if (rc) {
            char name[256];
            WideCharToMultiByte( CP_ACP, 0, nameW, -1, name, sizeof(name), NULL, NULL );
            trace("GetComputerObjectNameW() returned %s\n", name);
        }
    }
}

static void testGetUserNameExA(void)
{
    char name[256];
    ULONG size;
    BOOLEAN rc;
    UINT i;

    for (i = 0; i < (sizeof(formats) / sizeof(formats[0])); i++) {
        size = sizeof(name);
        ZeroMemory(name, sizeof(name));
        rc = pGetUserNameExA(formats[i], name, &size);
        ok(rc ||
           (formats[i] == NameUnknown &&
            GetLastError() == ERROR_NO_SUCH_USER) ||
           GetLastError() == ERROR_NONE_MAPPED ||
           broken(formats[i] == NameDnsDomain &&
                  GetLastError() == ERROR_INVALID_PARAMETER),
           "GetUserNameExW(%d) failed: %d\n",
           formats[i], GetLastError());
    }

    if (0) /* Crashes on Windows */
        pGetUserNameExA(NameSamCompatible, NULL, NULL);

    size = 0;
    rc = pGetUserNameExA(NameSamCompatible, NULL, &size);
    ok(! rc && GetLastError() == ERROR_MORE_DATA, "Expected fail with ERROR_MORE_DATA, got %d with %u\n", rc, GetLastError());
    ok(size != 0, "Expected size to be set to required size\n");

    if (0) /* Crashes on Windows with big enough size */
    {
        /* Returned size is already big enough */
        pGetUserNameExA(NameSamCompatible, NULL, &size);
    }

    size = 0;
    rc = pGetUserNameExA(NameSamCompatible, name, &size);
    ok(! rc && GetLastError() == ERROR_MORE_DATA, "Expected fail with ERROR_MORE_DATA, got %d with %u\n", rc, GetLastError());
    ok(size != 0, "Expected size to be set to required size\n");
    size = 1;
    name[0] = 0xff;
    rc = pGetUserNameExA(NameSamCompatible, name, &size);
    ok(! rc && GetLastError() == ERROR_MORE_DATA, "Expected fail with ERROR_MORE_DATA, got %d with %u\n", rc, GetLastError());
    ok(1 < size, "Expected size to be set to required size\n");
    ok(name[0] == (char) 0xff, "Expected unchanged buffer\n");
}

static void testGetUserNameExW(void)
{
    WCHAR nameW[256];
    ULONG size;
    BOOLEAN rc;
    UINT i;

    for (i = 0; i < (sizeof(formats) / sizeof(formats[0])); i++) {
        size = sizeof(nameW);
        ZeroMemory(nameW, sizeof(nameW));
        rc = pGetUserNameExW(formats[i], nameW, &size);
        ok(rc ||
           (formats[i] == NameUnknown &&
            GetLastError() == ERROR_NO_SUCH_USER) ||
           GetLastError() == ERROR_NONE_MAPPED ||
           broken(formats[i] == NameDnsDomain &&
                  GetLastError() == ERROR_INVALID_PARAMETER),
           "GetUserNameExW(%d) failed: %d\n",
           formats[i], GetLastError());
    }

    if (0) /* Crashes on Windows */
        pGetUserNameExW(NameSamCompatible, NULL, NULL);

    size = 0;
    rc = pGetUserNameExW(NameSamCompatible, NULL, &size);
    ok(! rc && GetLastError() == ERROR_MORE_DATA, "Expected fail with ERROR_MORE_DATA, got %d with %u\n", rc, GetLastError());
    ok(size != 0, "Expected size to be set to required size\n");

    if (0) /* Crashes on Windows with big enough size */
    {
        /* Returned size is already big enough */
        pGetUserNameExW(NameSamCompatible, NULL, &size);
    }

    size = 0;
    rc = pGetUserNameExW(NameSamCompatible, nameW, &size);
    ok(! rc && GetLastError() == ERROR_MORE_DATA, "Expected fail with ERROR_MORE_DATA, got %d with %u\n", rc, GetLastError());
    ok(size != 0, "Expected size to be set to required size\n");
    size = 1;
    nameW[0] = 0xff;
    rc = pGetUserNameExW(NameSamCompatible, nameW, &size);
    ok(! rc && GetLastError() == ERROR_MORE_DATA, "Expected fail with ERROR_MORE_DATA, got %d with %u\n", rc, GetLastError());
    ok(1 < size, "Expected size to be set to required size\n");
    ok(nameW[0] == (WCHAR) 0xff, "Expected unchanged buffer\n");
}

static void test_InitSecurityInterface(void)
{
    PSecurityFunctionTableA sftA;
    PSecurityFunctionTableW sftW;

    sftA = pInitSecurityInterfaceA();
    ok(sftA != NULL, "pInitSecurityInterfaceA failed\n");
    ok(sftA->dwVersion == SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION, "wrong dwVersion %d in security function table\n", sftA->dwVersion);
    ok(!sftA->Reserved2,
       "Reserved2 should be NULL instead of %p in security function table\n",
       sftA->Reserved2);
    ok(sftA->Reserved3 == sftA->EncryptMessage,
       "Reserved3 should be equal to EncryptMessage in the security function table\n");
    ok(sftA->Reserved4 == sftA->DecryptMessage,
       "Reserved4 should be equal to DecryptMessage in the security function table\n");

    if (!pInitSecurityInterfaceW)
    {
        win_skip("InitSecurityInterfaceW not exported by secur32.dll\n");
        return;
    }

    sftW = pInitSecurityInterfaceW();
    ok(sftW != NULL, "pInitSecurityInterfaceW failed\n");
    ok(sftW->dwVersion == SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION, "wrong dwVersion %d in security function table\n", sftW->dwVersion);
    ok(!sftW->Reserved2, "Reserved2 should be NULL instead of %p in security function table\n", sftW->Reserved2);
    ok(sftW->Reserved3 == sftW->EncryptMessage, "Reserved3 should be equal to EncryptMessage in the security function table\n");
    ok(sftW->Reserved4 == sftW->DecryptMessage, "Reserved4 should be equal to DecryptMessage in the security function table\n");
}

static void test_SspiEncodeStringsAsAuthIdentity(void)
{
    static const WCHAR username[] = {'u','s','e','r','n','a','m','e',0};
    static const WCHAR domainname[] = {'d','o','m','a','i','n','n','a','m','e',0};
    static const WCHAR password[] = {'p','a','s','s','w','o','r','d',0};
    const WCHAR *username_ptr, *domainname_ptr, *password_ptr;
    PSEC_WINNT_AUTH_IDENTITY_OPAQUE id;
    SECURITY_STATUS status;

    if (!pSspiEncodeStringsAsAuthIdentity)
    {
        win_skip( "SspiEncodeAuthIdentityAsStrings not exported by secur32.dll\n" );
        return;
    }

    status = pSspiEncodeStringsAsAuthIdentity( NULL, NULL, NULL, NULL );
    ok( status == SEC_E_INVALID_TOKEN, "got %08x\n", status );

    id = (PSEC_WINNT_AUTH_IDENTITY_OPAQUE)0xdeadbeef;
    status = pSspiEncodeStringsAsAuthIdentity( NULL, NULL, NULL, &id );
    ok( status == SEC_E_INVALID_TOKEN, "got %08x\n", status );
    ok( id == (PSEC_WINNT_AUTH_IDENTITY_OPAQUE)0xdeadbeef, "id set\n" );

    id = NULL;
    status = pSspiEncodeStringsAsAuthIdentity( NULL, NULL, password, &id );
    ok( status == SEC_E_OK, "got %08x\n", status );
    ok( id != NULL, "id not set\n" );
    pSspiFreeAuthIdentity( id );

    id = NULL;
    status = pSspiEncodeStringsAsAuthIdentity( NULL, domainname, password, &id );
    ok( status == SEC_E_OK, "got %08x\n", status );
    ok( id != NULL, "id not set\n" );
    pSspiFreeAuthIdentity( id );

    id = NULL;
    status = pSspiEncodeStringsAsAuthIdentity( username, NULL, password, &id );
    ok( status == SEC_E_OK, "got %08x\n", status );
    ok( id != NULL, "id not set\n" );
    pSspiFreeAuthIdentity( id );

    id = NULL;
    status = pSspiEncodeStringsAsAuthIdentity( username, NULL, NULL, &id );
    ok( status == SEC_E_OK, "got %08x\n", status );
    ok( id != NULL, "id not set\n" );
    pSspiFreeAuthIdentity( id );

    id = NULL;
    status = pSspiEncodeStringsAsAuthIdentity( username, domainname, password, &id );
    ok( status == SEC_E_OK, "got %08x\n", status );
    ok( id != NULL, "id not set\n" );

    username_ptr = domainname_ptr = password_ptr = NULL;
    status = pSspiEncodeAuthIdentityAsStrings( id, &username_ptr, &domainname_ptr, &password_ptr );
    ok( status == SEC_E_OK, "got %08x\n", status );
    ok( !lstrcmpW( username, username_ptr ), "wrong username\n" );
    ok( !lstrcmpW( domainname, domainname_ptr ), "wrong domainname\n" );
    ok( !lstrcmpW( password, password_ptr ), "wrong password\n" );

    pSspiZeroAuthIdentity( id );

    pSspiLocalFree( (void *)username_ptr );
    pSspiLocalFree( (void *)domainname_ptr );
    pSspiLocalFree( (void *)password_ptr );
    pSspiFreeAuthIdentity( id );

    id = NULL;
    status = pSspiEncodeStringsAsAuthIdentity( username, NULL, password, &id );
    ok( status == SEC_E_OK, "got %08x\n", status );
    ok( id != NULL, "id not set\n" );

    username_ptr = password_ptr = NULL;
    domainname_ptr = (const WCHAR *)0xdeadbeef;
    status = pSspiEncodeAuthIdentityAsStrings( id, &username_ptr, &domainname_ptr, &password_ptr );
    ok( status == SEC_E_OK, "got %08x\n", status );
    ok( !lstrcmpW( username, username_ptr ), "wrong username\n" );
    ok( domainname_ptr == NULL, "domainname_ptr not cleared\n" );
    ok( !lstrcmpW( password, password_ptr ), "wrong password\n" );

    pSspiLocalFree( (void *)username_ptr );
    pSspiLocalFree( (void *)password_ptr );
    pSspiFreeAuthIdentity( id );
}

START_TEST(secur32)
{
    secdll = LoadLibraryA("secur32.dll");

    if (!secdll)
        secdll = LoadLibraryA("security.dll");

    if (secdll)
    {
        pSspiEncodeAuthIdentityAsStrings = (void *)GetProcAddress(secdll, "SspiEncodeAuthIdentityAsStrings");
        pSspiEncodeStringsAsAuthIdentity = (void *)GetProcAddress(secdll, "SspiEncodeStringsAsAuthIdentity");
        pSspiFreeAuthIdentity = (void *)GetProcAddress(secdll, "SspiFreeAuthIdentity");
        pSspiLocalFree = (void *)GetProcAddress(secdll, "SspiLocalFree");
        pSspiZeroAuthIdentity = (void *)GetProcAddress(secdll, "SspiZeroAuthIdentity");
        pGetComputerObjectNameA = (PVOID)GetProcAddress(secdll, "GetComputerObjectNameA");
        pGetComputerObjectNameW = (PVOID)GetProcAddress(secdll, "GetComputerObjectNameW");
        pGetUserNameExA = (PVOID)GetProcAddress(secdll, "GetUserNameExA");
        pGetUserNameExW = (PVOID)GetProcAddress(secdll, "GetUserNameExW");
        pInitSecurityInterfaceA = (PVOID)GetProcAddress(secdll, "InitSecurityInterfaceA");
        pInitSecurityInterfaceW = (PVOID)GetProcAddress(secdll, "InitSecurityInterfaceW");
 
        if (pGetComputerObjectNameA)
            testGetComputerObjectNameA();
        else
            win_skip("GetComputerObjectNameA not exported by secur32.dll\n");

        if (pGetComputerObjectNameW)
            testGetComputerObjectNameW();
        else
            win_skip("GetComputerObjectNameW not exported by secur32.dll\n");

        if (pGetUserNameExA)
            testGetUserNameExA();
        else
            win_skip("GetUserNameExA not exported by secur32.dll\n");

        if (pGetUserNameExW)
            testGetUserNameExW();
        else
            win_skip("GetUserNameExW not exported by secur32.dll\n");

        test_InitSecurityInterface();
        test_SspiEncodeStringsAsAuthIdentity();

        FreeLibrary(secdll);
    }
}
