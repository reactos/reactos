/*
 * Miscellaneous secur32 tests
 *
 * Copyright 2005, 2006 Kai Blin
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

#define SECURITY_WIN32

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <windef.h>
#include <winbase.h>
#include <sspi.h>

#include "wine/test.h"

static HMODULE secdll;
static PSecurityFunctionTableA (SEC_ENTRY * pInitSecurityInterfaceA)(void);
static SECURITY_STATUS (SEC_ENTRY * pEnumerateSecurityPackagesA)(PULONG, PSecPkgInfoA*);
static SECURITY_STATUS (SEC_ENTRY * pFreeContextBuffer)(PVOID pv);
static SECURITY_STATUS (SEC_ENTRY * pQuerySecurityPackageInfoA)(SEC_CHAR*, PSecPkgInfoA*);
static SECURITY_STATUS (SEC_ENTRY * pAcquireCredentialsHandleA)(SEC_CHAR*, SEC_CHAR*,
                            ULONG, PLUID, PVOID, SEC_GET_KEY_FN, PVOID, PCredHandle, PTimeStamp);
static SECURITY_STATUS (SEC_ENTRY * pInitializeSecurityContextA)(PCredHandle, PCtxtHandle,
                            SEC_CHAR*, ULONG, ULONG, ULONG, PSecBufferDesc, ULONG, 
                            PCtxtHandle, PSecBufferDesc, PULONG, PTimeStamp);
static SECURITY_STATUS (SEC_ENTRY * pCompleteAuthToken)(PCtxtHandle, PSecBufferDesc);
static SECURITY_STATUS (SEC_ENTRY * pAcceptSecurityContext)(PCredHandle, PCtxtHandle,
                            PSecBufferDesc, ULONG, ULONG, PCtxtHandle, PSecBufferDesc,
                            PULONG, PTimeStamp);
static SECURITY_STATUS (SEC_ENTRY * pFreeCredentialsHandle)(PCredHandle);
static SECURITY_STATUS (SEC_ENTRY * pDeleteSecurityContext)(PCtxtHandle);
static SECURITY_STATUS (SEC_ENTRY * pQueryContextAttributesA)(PCtxtHandle, ULONG, PVOID);

static void InitFunctionPtrs(void)
{
    secdll = LoadLibraryA("secur32.dll");
    if(!secdll)
        secdll = LoadLibraryA("security.dll");
    if(secdll)
    {
        pInitSecurityInterfaceA = (PVOID)GetProcAddress(secdll, "InitSecurityInterfaceA");
        pEnumerateSecurityPackagesA = (PVOID)GetProcAddress(secdll, "EnumerateSecurityPackagesA");
        pFreeContextBuffer = (PVOID)GetProcAddress(secdll, "FreeContextBuffer");
        pQuerySecurityPackageInfoA = (PVOID)GetProcAddress(secdll, "QuerySecurityPackageInfoA");
        pAcquireCredentialsHandleA = (PVOID)GetProcAddress(secdll, "AcquireCredentialsHandleA");
        pInitializeSecurityContextA = (PVOID)GetProcAddress(secdll, "InitializeSecurityContextA");
        pCompleteAuthToken = (PVOID)GetProcAddress(secdll, "CompleteAuthToken");
        pAcceptSecurityContext = (PVOID)GetProcAddress(secdll, "AcceptSecurityContext");
        pFreeCredentialsHandle = (PVOID)GetProcAddress(secdll, "FreeCredentialsHandle");
        pDeleteSecurityContext = (PVOID)GetProcAddress(secdll, "DeleteSecurityContext");
        pQueryContextAttributesA = (PVOID)GetProcAddress(secdll, "QueryContextAttributesA");
    }
}

/*---------------------------------------------------------*/
/* General helper functions */

static const char* getSecError(SECURITY_STATUS status)
{
    static char buf[20];

#define _SEC_ERR(x) case (x): return #x;
    switch(status)
    {
        _SEC_ERR(SEC_E_OK);
        _SEC_ERR(SEC_E_INSUFFICIENT_MEMORY);
        _SEC_ERR(SEC_E_INVALID_HANDLE);
        _SEC_ERR(SEC_E_UNSUPPORTED_FUNCTION);
        _SEC_ERR(SEC_E_TARGET_UNKNOWN);
        _SEC_ERR(SEC_E_INTERNAL_ERROR);
        _SEC_ERR(SEC_E_SECPKG_NOT_FOUND);
        _SEC_ERR(SEC_E_NOT_OWNER);
        _SEC_ERR(SEC_E_CANNOT_INSTALL);
        _SEC_ERR(SEC_E_INVALID_TOKEN);
        _SEC_ERR(SEC_E_CANNOT_PACK);
        _SEC_ERR(SEC_E_QOP_NOT_SUPPORTED);
        _SEC_ERR(SEC_E_NO_IMPERSONATION);
        _SEC_ERR(SEC_I_CONTINUE_NEEDED);
        _SEC_ERR(SEC_E_BUFFER_TOO_SMALL);
        _SEC_ERR(SEC_E_ILLEGAL_MESSAGE);
        _SEC_ERR(SEC_E_LOGON_DENIED);
        _SEC_ERR(SEC_E_NO_CREDENTIALS);
        _SEC_ERR(SEC_E_OUT_OF_SEQUENCE);
        default:
            sprintf(buf, "%08x\n", status);
            return buf;
    }
#undef _SEC_ERR
}

/*---------------------------------------------------------*/
/* Helper for testQuerySecurityPagageInfo */

static SECURITY_STATUS setupPackageA(SEC_CHAR *p_package_name, 
        PSecPkgInfo *p_pkg_info)
{
    SECURITY_STATUS ret;
    
    ret = pQuerySecurityPackageInfoA( p_package_name, p_pkg_info);
    return ret;
}

/*--------------------------------------------------------- */
/* The test functions */

static void testInitSecurityInterface(void)
{
    PSecurityFunctionTable sec_fun_table = NULL;

    sec_fun_table = pInitSecurityInterfaceA();
    ok(sec_fun_table != NULL, "InitSecurityInterface() returned NULL.\n");

}

static void testEnumerateSecurityPackages(void)
{

    SECURITY_STATUS sec_status;
    ULONG           num_packages, i;
    PSecPkgInfo     pkg_info = NULL;

    trace("Running testEnumerateSecurityPackages\n");
    
    sec_status = pEnumerateSecurityPackagesA(&num_packages, &pkg_info);

    ok(sec_status == SEC_E_OK, 
            "EnumerateSecurityPackages() should return %d, not %08x\n",
            SEC_E_OK, sec_status);

    if (num_packages == 0)
    {
        todo_wine
        ok(num_packages > 0, "Number of sec packages should be > 0 ,but is %d\n",
                num_packages);
        skip("no sec packages to check\n");
        return;
    }
    else
        ok(num_packages > 0, "Number of sec packages should be > 0 ,but is %d\n",
                num_packages);

    ok(pkg_info != NULL, 
            "pkg_info should not be NULL after EnumerateSecurityPackages\n");
    
    trace("Number of packages: %d\n", num_packages);
    for(i = 0; i < num_packages; ++i){
        trace("%d: Package \"%s\"\n", i, pkg_info[i].Name);
        trace("Supported flags:\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_INTEGRITY)
            trace("\tSECPKG_FLAG_INTEGRITY\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_PRIVACY)
            trace("\tSECPKG_FLAG_PRIVACY\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_TOKEN_ONLY)
            trace("\tSECPKG_FLAG_TOKEN_ONLY\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_DATAGRAM)
            trace("\tSECPKG_FLAG_DATAGRAM\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_CONNECTION)
            trace("\tSECPKG_FLAG_CONNECTION\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_MULTI_REQUIRED)
            trace("\tSECPKG_FLAG_MULTI_REQUIRED\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_CLIENT_ONLY)
            trace("\tSECPKG_FLAG_CLIENT_ONLY\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_EXTENDED_ERROR)
            trace("\tSECPKG_FLAG_EXTENDED_ERROR\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_IMPERSONATION)
            trace("\tSECPKG_FLAG_IMPERSONATION\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_ACCEPT_WIN32_NAME)
            trace("\tSECPKG_FLAG_ACCEPT_WIN32_NAME\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_STREAM)
            trace("\tSECPKG_FLAG_STREAM\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_READONLY_WITH_CHECKSUM)
            trace("\tSECPKG_FLAG_READONLY_WITH_CHECKSUM\n");
        trace("Comment: %s\n", pkg_info[i].Comment);
        trace("\n");
    }

    pFreeContextBuffer(pkg_info);
}


static void testQuerySecurityPackageInfo(void)
{
    SECURITY_STATUS     sec_status;
    PSecPkgInfo         pkg_info;
    static SEC_CHAR     ntlm[]     = "NTLM",
                        winetest[] = "Winetest";

    trace("Running testQuerySecurityPackageInfo\n");

    /* Test with an existing package. Test should pass */

    pkg_info = (void *)0xdeadbeef;
    sec_status = setupPackageA(ntlm, &pkg_info);

    ok((sec_status == SEC_E_OK) || (sec_status == SEC_E_SECPKG_NOT_FOUND) ||
       broken(sec_status == SEC_E_UNSUPPORTED_FUNCTION), /* win95 */
       "Return value of QuerySecurityPackageInfo() shouldn't be %s\n",
       getSecError(sec_status) );

    if (sec_status == SEC_E_OK)
    {
        ok(pkg_info != (void *)0xdeadbeef, "wrong pkg_info address %p\n", pkg_info);
        ok(pkg_info->wVersion == 1, "wVersion always should be 1, but is %d\n", pkg_info->wVersion);
        /* there is no point in testing pkg_info->cbMaxToken since it varies
         * between implementations.
         */

        sec_status = pFreeContextBuffer(pkg_info);
        ok( sec_status == SEC_E_OK,
            "Return value of FreeContextBuffer() shouldn't be %s\n",
            getSecError(sec_status) );
    }

    /* Test with a nonexistent package, test should fail */

    pkg_info = (void *)0xdeadbeef;
    sec_status = pQuerySecurityPackageInfoA(winetest, &pkg_info);

    ok( sec_status == SEC_E_SECPKG_NOT_FOUND,
        "Return value of QuerySecurityPackageInfo() should be %s for a nonexistent package\n",
        getSecError(SEC_E_SECPKG_NOT_FOUND));

    ok(pkg_info == (void *)0xdeadbeef, "wrong pkg_info address %p\n", pkg_info);
}

START_TEST(main)
{
    InitFunctionPtrs();
    if(pInitSecurityInterfaceA)
        testInitSecurityInterface();
    if(pFreeContextBuffer)
    {
        if(pEnumerateSecurityPackagesA)
            testEnumerateSecurityPackages();
        if(pQuerySecurityPackageInfoA)
        {
            testQuerySecurityPackageInfo();
        }
    }
    if(secdll)
        FreeLibrary(secdll);
}
