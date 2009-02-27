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

static BOOLEAN (WINAPI * pGetComputerObjectNameA)(EXTENDED_NAME_FORMAT NameFormat, LPSTR lpNameBuffer, PULONG lpnSize);
static BOOLEAN (WINAPI * pGetComputerObjectNameW)(EXTENDED_NAME_FORMAT NameFormat, LPWSTR lpNameBuffer, PULONG lpnSize);
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
    int i;

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
    int i;

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

static void test_InitSecurityInterface(void)
{
    PSecurityFunctionTableA sftA;
    PSecurityFunctionTableW sftW;

    sftA = pInitSecurityInterfaceA();
    ok(sftA != NULL, "pInitSecurityInterfaceA failed\n");
    ok(sftA->dwVersion == SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION, "wrong dwVersion %d in security function table\n", sftA->dwVersion);
    ok(!sftA->Reserved2 || broken(sftA->Reserved2 != NULL) /* WinME */,
       "Reserved2 should be NULL instead of %p in security function table\n",
       sftA->Reserved2);
    ok(sftA->Reserved3 == sftA->EncryptMessage ||
       broken(sftA->Reserved3 != sftA->EncryptMessage) /* Win9x */,
       "Reserved3 should be equal to EncryptMessage in the security function table\n");
    ok(sftA->Reserved4 == sftA->DecryptMessage ||
       broken(sftA->Reserved4 != sftA->DecryptMessage) /* Win9x */,
       "Reserved4 should be equal to DecryptMessage in the security function table\n");

    if (!pInitSecurityInterfaceW)
    {
        skip("InitSecurityInterfaceW not exported by secur32.dll\n");
        return;
    }

    sftW = pInitSecurityInterfaceW();
    ok(sftW != NULL, "pInitSecurityInterfaceW failed\n");
    ok(sftW->dwVersion == SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION, "wrong dwVersion %d in security function table\n", sftW->dwVersion);
    ok(!sftW->Reserved2, "Reserved2 should be NULL instead of %p in security function table\n", sftW->Reserved2);
    ok(sftW->Reserved3 == sftW->EncryptMessage, "Reserved3 should be equal to EncryptMessage in the security function table\n");
    ok(sftW->Reserved4 == sftW->DecryptMessage, "Reserved4 should be equal to DecryptMessage in the security function table\n");
}

START_TEST(secur32)
{
    secdll = LoadLibraryA("secur32.dll");

    if (!secdll)
        secdll = LoadLibraryA("security.dll");

    if (secdll)
    {
        pGetComputerObjectNameA = (PVOID)GetProcAddress(secdll, "GetComputerObjectNameA");
        pGetComputerObjectNameW = (PVOID)GetProcAddress(secdll, "GetComputerObjectNameW");
        pInitSecurityInterfaceA = (PVOID)GetProcAddress(secdll, "InitSecurityInterfaceA");
        pInitSecurityInterfaceW = (PVOID)GetProcAddress(secdll, "InitSecurityInterfaceW");
 
        if (pGetComputerObjectNameA)
            testGetComputerObjectNameA();

        if (pGetComputerObjectNameW)
            testGetComputerObjectNameW();

        test_InitSecurityInterface();

        FreeLibrary(secdll);
    }
}
