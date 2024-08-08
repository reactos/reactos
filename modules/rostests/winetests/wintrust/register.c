/* Unit test suite for wintrust API functions
 *
 * Copyright 2006 Paul Vriens
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
 *
 */

#include <stdarg.h>
#include <stdio.h>

#include "windows.h"
#include "softpub.h"
#include "wintrust.h"
#include "winreg.h"

#include "wine/test.h"

static BOOL (WINAPI * pWintrustAddActionID)(GUID*, DWORD, CRYPT_REGISTER_ACTIONID*);
static BOOL (WINAPI * pWintrustAddDefaultForUsage)(const CHAR*,CRYPT_PROVIDER_REGDEFUSAGE*);
static void (WINAPI * pWintrustGetRegPolicyFlags)(DWORD *);
static BOOL (WINAPI * pWintrustLoadFunctionPointers)(GUID *, CRYPT_PROVIDER_FUNCTIONS *);
static BOOL (WINAPI * pWintrustRemoveActionID)(GUID*);
static BOOL (WINAPI * pWintrustSetRegPolicyFlags)(DWORD);

static void InitFunctionPtrs(void)
{
    HMODULE hWintrust = GetModuleHandleA("wintrust.dll");

#define WINTRUST_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hWintrust, #func); \
    if(!p ## func) \
      trace("GetProcAddress(%s) failed\n", #func);

    WINTRUST_GET_PROC(WintrustAddActionID)
    WINTRUST_GET_PROC(WintrustAddDefaultForUsage)
    WINTRUST_GET_PROC(WintrustGetRegPolicyFlags)
    WINTRUST_GET_PROC(WintrustLoadFunctionPointers)
    WINTRUST_GET_PROC(WintrustRemoveActionID)
    WINTRUST_GET_PROC(WintrustSetRegPolicyFlags)

#undef WINTRUST_GET_PROC
}

static void test_AddRem_ActionID(void)
{
    static WCHAR DummyDllW[]      = {'d','e','a','d','b','e','e','f','.','d','l','l',0 };
    static WCHAR DummyFunctionW[] = {'d','u','m','m','y','f','u','n','c','t','i','o','n',0 };
    GUID ActionID = { 0xdeadbeef, 0xdead, 0xbeef, { 0xde,0xad,0xbe,0xef,0xde,0xad,0xbe,0xef }};
    CRYPT_REGISTER_ACTIONID ActionIDFunctions;
    CRYPT_TRUST_REG_ENTRY EmptyProvider = { 0, NULL, NULL };
    CRYPT_TRUST_REG_ENTRY DummyProvider = { sizeof(CRYPT_TRUST_REG_ENTRY), DummyDllW, DummyFunctionW };
    BOOL ret;

    if (!pWintrustAddActionID || !pWintrustRemoveActionID)
    {
        win_skip("WintrustAddActionID and/or WintrustRemoveActionID are not available\n");
        return;
    }

    /* All NULL */
    SetLastError(0xdeadbeef);
    ret = pWintrustAddActionID(NULL, 0, NULL);
    ok (!ret, "Expected WintrustAddActionID to fail.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER /* XP/W2K3 */ ||
        GetLastError() == 0xdeadbeef              /* Win98/NT4/W2K */,
        "Expected ERROR_INVALID_PARAMETER(W2K3) or 0xdeadbeef(Win98/NT4/W2K), got %lu.\n", GetLastError());

    /* NULL functions */
    SetLastError(0xdeadbeef);
    ret = pWintrustAddActionID(&ActionID, 0, NULL);
    ok (!ret, "Expected WintrustAddActionID to fail.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER /* XP/W2K3 */ ||
        GetLastError() == 0xdeadbeef              /* Win98/NT4/W2K */,
        "Expected ERROR_INVALID_PARAMETER(W2K3) or 0xdeadbeef(Win98/NT4/W2K), got %lu.\n", GetLastError());

    /* All OK (although no functions defined), except cbStruct is not set in ActionIDFunctions */
    SetLastError(0xdeadbeef);
    memset(&ActionIDFunctions, 0, sizeof(CRYPT_REGISTER_ACTIONID));
    ret = pWintrustAddActionID(&ActionID, 0, &ActionIDFunctions);
    ok (!ret, "Expected WintrustAddActionID to fail.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER /* XP/W2K3 */ ||
        GetLastError() == 0xdeadbeef              /* Win98/NT4/W2K */,
        "Expected ERROR_INVALID_PARAMETER(W2K3) or 0xdeadbeef(Win98/NT4/W2K), got %lu.\n", GetLastError());

    /* All OK (although no functions defined) and cbStruct is set now */
    SetLastError(0xdeadbeef);
    memset(&ActionIDFunctions, 0, sizeof(CRYPT_REGISTER_ACTIONID));
    ActionIDFunctions.cbStruct = sizeof(CRYPT_REGISTER_ACTIONID);
    ret = pWintrustAddActionID(&ActionID, 0, &ActionIDFunctions);
    ok (ret, "Expected WintrustAddActionID to succeed.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %lu.\n", GetLastError());

    /* All OK and all (but 1) functions are correctly defined. The DLL and entrypoints
     * are not present.
     */
    memset(&ActionIDFunctions, 0, sizeof(CRYPT_REGISTER_ACTIONID));
    ActionIDFunctions.cbStruct = sizeof(CRYPT_REGISTER_ACTIONID);
    ActionIDFunctions.sInitProvider = DummyProvider;
    ActionIDFunctions.sObjectProvider = DummyProvider;
    ActionIDFunctions.sSignatureProvider = EmptyProvider;
    ActionIDFunctions.sCertificateProvider = DummyProvider;
    ActionIDFunctions.sCertificatePolicyProvider = DummyProvider;
    ActionIDFunctions.sFinalPolicyProvider = DummyProvider;
    ActionIDFunctions.sTestPolicyProvider = DummyProvider;
    ActionIDFunctions.sCleanupProvider = DummyProvider;
    SetLastError(0xdeadbeef);
    ret = pWintrustAddActionID(&ActionID, 0, &ActionIDFunctions);
    ok (ret, "Expected WintrustAddActionID to succeed.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED,
        "Expected ERROR_INVALID_PARAMETER or ERROR_ACCESS_DENIED, got %lu.\n",
        GetLastError());

    /* All OK and all functions are correctly defined. The DLL and entrypoints
     * are not present.
     */
    memset(&ActionIDFunctions, 0, sizeof(CRYPT_REGISTER_ACTIONID));
    ActionIDFunctions.cbStruct = sizeof(CRYPT_REGISTER_ACTIONID);
    ActionIDFunctions.sInitProvider = DummyProvider;
    ActionIDFunctions.sObjectProvider = DummyProvider;
    ActionIDFunctions.sSignatureProvider = DummyProvider;
    ActionIDFunctions.sCertificateProvider = DummyProvider;
    ActionIDFunctions.sCertificatePolicyProvider = DummyProvider;
    ActionIDFunctions.sFinalPolicyProvider = DummyProvider;
    ActionIDFunctions.sTestPolicyProvider = DummyProvider;
    ActionIDFunctions.sCleanupProvider = DummyProvider;
    SetLastError(0xdeadbeef);
    ret = pWintrustAddActionID(&ActionID, 0, &ActionIDFunctions);
    ok (ret, "Expected WintrustAddActionID to succeed.\n");
    ok (GetLastError() == 0xdeadbeef || GetLastError() == ERROR_ACCESS_DENIED,
        "Expected 0xdeadbeef or ERROR_ACCESS_DENIED, got %lu.\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    ret = pWintrustRemoveActionID(&ActionID);
    ok ( ret, "WintrustRemoveActionID failed : %ld\n", GetLastError());
    ok ( GetLastError() == 0xdeadbeef, "Last error should not have been changed: %lu\n", GetLastError());

    /* NULL input */
    SetLastError(0xdeadbeef);
    ret = pWintrustRemoveActionID(NULL);
    ok (ret, "Expected WintrustRemoveActionID to succeed.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %lu.\n", GetLastError());

    /* The passed GUID is removed by a previous call, so it's basically a test with a nonexistent Trust provider */ 
    SetLastError(0xdeadbeef);
    ret = pWintrustRemoveActionID(&ActionID);
    ok (ret, "Expected WintrustRemoveActionID to succeed.\n");
    ok (GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got %lu.\n", GetLastError());
}

static void test_AddDefaultForUsage(void)
{
    BOOL ret;
    static GUID ActionID        = { 0xdeadbeef, 0xdead, 0xbeef, { 0xde,0xad,0xbe,0xef,0xde,0xad,0xbe,0xef }};
    static WCHAR DummyDllW[]    = {'d','e','a','d','b','e','e','f','.','d','l','l',0 };
    static CHAR DummyFunction[] = "dummyfunction";
    static const CHAR oid[]     = "1.2.3.4.5.6.7.8.9.10";
    static CRYPT_PROVIDER_REGDEFUSAGE DefUsage;

    if (!pWintrustAddDefaultForUsage)
    {
        win_skip("WintrustAddDefaultForUsage is not available\n");
        return;
    }

    /* All NULL */
    SetLastError(0xdeadbeef);
    ret = pWintrustAddDefaultForUsage(NULL, NULL);
    ok (!ret, "Expected WintrustAddDefaultForUsage to fail.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %lu.\n", GetLastError());

    /* NULL defusage */
    SetLastError(0xdeadbeef);
    ret = pWintrustAddDefaultForUsage(oid, NULL);
    ok (!ret, "Expected WintrustAddDefaultForUsage to fail.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %lu.\n", GetLastError());

    /* NULL oid and proper defusage */
    memset(&DefUsage, 0 , sizeof(CRYPT_PROVIDER_REGDEFUSAGE));
    DefUsage.cbStruct = sizeof(CRYPT_PROVIDER_REGDEFUSAGE);
    DefUsage.pgActionID = &ActionID;
    DefUsage.pwszDllName = DummyDllW;
    DefUsage.pwszLoadCallbackDataFunctionName = DummyFunction;
    DefUsage.pwszFreeCallbackDataFunctionName = DummyFunction;
    SetLastError(0xdeadbeef);
    ret = pWintrustAddDefaultForUsage(NULL, &DefUsage);
    ok (!ret, "Expected WintrustAddDefaultForUsage to fail.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %lu.\n", GetLastError());

    /* cbStruct set to 0 */
    memset(&DefUsage, 0 , sizeof(CRYPT_PROVIDER_REGDEFUSAGE));
    DefUsage.cbStruct = 0;
    DefUsage.pgActionID = &ActionID;
    DefUsage.pwszDllName = DummyDllW;
    DefUsage.pwszLoadCallbackDataFunctionName = DummyFunction;
    DefUsage.pwszFreeCallbackDataFunctionName = DummyFunction;
    SetLastError(0xdeadbeef);
    ret = pWintrustAddDefaultForUsage(oid, &DefUsage);
    ok (!ret, "Expected WintrustAddDefaultForUsage to fail.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %lu.\n", GetLastError());
}

static void test_LoadFunctionPointers(void)
{
    BOOL ret;
    CRYPT_PROVIDER_FUNCTIONS funcs = {0};
    GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    if (!pWintrustLoadFunctionPointers)
    {
        win_skip("WintrustLoadFunctionPointers is not available\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = pWintrustLoadFunctionPointers(NULL, NULL);
    ok(!ret && GetLastError() == 0xdeadbeef, "Expected failure\n");
    SetLastError(0xdeadbeef);
    ret = pWintrustLoadFunctionPointers(&action, NULL);
    ok(!ret && GetLastError() == 0xdeadbeef, "Expected failure\n");

    SetLastError(0xdeadbeef);
    ret = pWintrustLoadFunctionPointers(NULL, &funcs);
    ok(!ret, "WintrustLoadFunctionPointers succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == 0xdeadbeef /* W2K and XP-SP1 */,
        "Expected ERROR_INVALID_PARAMETER or 0xdeadbeef, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    funcs.cbStruct = 0;
    ret = pWintrustLoadFunctionPointers(&action, &funcs);
    ok(!ret && GetLastError() == 0xdeadbeef, "Expected failure\n");
    SetLastError(0xdeadbeef);
    funcs.cbStruct = sizeof(funcs);
    ret = pWintrustLoadFunctionPointers(&action, &funcs);
    ok(ret, "WintrustLoadFunctionPointers failed: %ld\n", GetLastError());
    ok(funcs.pfnAlloc != NULL, "Expected a pointer\n");
    ok(funcs.pfnFree != NULL, "Expected a pointer\n");
}

static void test_RegPolicyFlags(void)
{
    /* Default state value 0x00023c00, which is
     *  WTPF_IGNOREREVOCATIONONTS |
     *  WTPF_OFFLINEOKNBU_COM |
     *  WTPF_OFFLINEOKNBU_IND |
     *  WTPF_OFFLINEOK_COM |
     *  WTPF_OFFLINEOK_IND
     */
    static const CHAR Software_Publishing[] =
     "Software\\Microsoft\\Windows\\CurrentVersion\\Wintrust\\"
     "Trust Providers\\Software Publishing";
    static const CHAR State[] = "State";
    HKEY key;
    LONG r;
    DWORD flags1, flags2, flags3, size;
    BOOL ret;

    if (!pWintrustGetRegPolicyFlags || !pWintrustSetRegPolicyFlags)
    {
        win_skip("Policy flags functions not present\n");
        return;
    }

    pWintrustGetRegPolicyFlags(&flags2);

    r = RegOpenKeyExA(HKEY_CURRENT_USER, Software_Publishing, 0, KEY_ALL_ACCESS,
     &key);
    ok(!r, "RegOpenKeyEx failed: %ld\n", r);

    size = sizeof(flags1);
    r = RegQueryValueExA(key, State, NULL, NULL, (LPBYTE)&flags1, &size);
    ok(!r || r == ERROR_FILE_NOT_FOUND, "RegQueryValueEx failed: %ld\n", r);
    if (!r)
        ok(flags1 == flags2, "Got %08lx flags instead of %08lx\n", flags1, flags2);

    flags3 = flags2 | 1;
    ret = pWintrustSetRegPolicyFlags(flags3);
    ok(ret, "WintrustSetRegPolicyFlags failed: %ld\n", GetLastError());
    size = sizeof(flags1);
    r = RegQueryValueExA(key, State, NULL, NULL, (LPBYTE)&flags1, &size);
    ok(!r, "RegQueryValueEx failed: %ld\n", r);
    ok(flags1 == flags3, "Got %08lx flags instead of %08lx\n", flags1, flags3);

    pWintrustSetRegPolicyFlags(flags2);

    RegCloseKey(key);
}

START_TEST(register)
{
    InitFunctionPtrs();

    test_AddRem_ActionID();
    test_AddDefaultForUsage();
    test_LoadFunctionPointers();
    test_RegPolicyFlags();
}
