/*
 * Copyright 2002 Andriy Palamarchuk
 *
 * Conformance test of the workstation functions.
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

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winresrc.h" /* Ensure we use Unicode defns with native headers */
#include "nb30.h"
#include "lmcons.h"
#include "lmerr.h"
#include "lmwksta.h"
#include "lmapibuf.h"
#include "lmjoin.h"

static NET_API_STATUS (WINAPI *pNetApiBufferFree)(LPVOID)=NULL;
static NET_API_STATUS (WINAPI *pNetApiBufferSize)(LPVOID,LPDWORD)=NULL;
static NET_API_STATUS (WINAPI *pNetpGetComputerName)(LPWSTR*)=NULL;
static NET_API_STATUS (WINAPI *pNetWkstaUserGetInfo)(LPWSTR,DWORD,PBYTE*)=NULL;
static NET_API_STATUS (WINAPI *pNetWkstaTransportEnum)(LPWSTR,DWORD,LPBYTE*,
 DWORD,LPDWORD,LPDWORD,LPDWORD)=NULL;
static NET_API_STATUS (WINAPI *pNetGetJoinInformation)(LPCWSTR,LPWSTR*,PNETSETUP_JOIN_STATUS);

static WCHAR user_name[UNLEN + 1];
static WCHAR computer_name[MAX_COMPUTERNAME_LENGTH + 1];

static BOOL init_wksta_tests(void)
{
    DWORD dwSize;
    BOOL rc;

    user_name[0] = 0;
    dwSize = sizeof(user_name)/sizeof(user_name[0]);
    rc=GetUserNameW(user_name, &dwSize);
    if (rc==FALSE && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED) {
        win_skip("GetUserNameW is not implemented\n");
        return FALSE;
    }
    ok(rc, "User Name Retrieved\n");

    computer_name[0] = 0;
    dwSize = sizeof(computer_name)/sizeof(computer_name[0]);
    ok(GetComputerNameW(computer_name, &dwSize), "Computer Name Retrieved\n");
    return TRUE;
}

static void run_get_comp_name_tests(void)
{
    LPWSTR ws = NULL;

    ok(pNetpGetComputerName(&ws) == NERR_Success, "Computer name is retrieved\n");
    ok(!lstrcmpW(computer_name, ws), "This is really computer name\n");
    pNetApiBufferFree(ws);
}

static void run_wkstausergetinfo_tests(void)
{
    LPWKSTA_USER_INFO_0 ui0 = NULL;
    LPWKSTA_USER_INFO_1 ui1 = NULL;
    LPWKSTA_USER_INFO_1101 ui1101 = NULL;
    DWORD dwSize;
    NET_API_STATUS rc;

    /* Level 0 */
    rc = pNetWkstaUserGetInfo(NULL, 0, (LPBYTE *)&ui0);
    if (rc == NERR_WkstaNotStarted)
    {
        skip("Workstation service not running\n");
        return;
    }
    ok(!rc && ui0, "got %d and %p (expected NERR_Success and != NULL\n", rc, ui0);

    /* This failure occurred when I ran sshd as service and didn't authenticate
     * Since the test dereferences ui0, the rest of this test is worthless
     */
    if (!ui0)
    {
        return;
    }

    ok(!lstrcmpW(user_name, ui0->wkui0_username), "This is really user name\n");
    pNetApiBufferSize(ui0, &dwSize);
    ok(dwSize >= (sizeof(WKSTA_USER_INFO_0) +
                 lstrlenW(ui0->wkui0_username) * sizeof(WCHAR)),
       "Is allocated with NetApiBufferAllocate\n");

    /* Level 1 */
    ok(pNetWkstaUserGetInfo(NULL, 1, (LPBYTE *)&ui1) == NERR_Success,
       "NetWkstaUserGetInfo is successful\n");
    ok(lstrcmpW(ui1->wkui1_username, ui0->wkui0_username) == 0,
       "the same name as returned for level 0\n");
    pNetApiBufferSize(ui1, &dwSize);
    ok(dwSize >= (sizeof(WKSTA_USER_INFO_1) +
                  (lstrlenW(ui1->wkui1_username) +
                   lstrlenW(ui1->wkui1_logon_domain) +
                   lstrlenW(ui1->wkui1_oth_domains) +
                   lstrlenW(ui1->wkui1_logon_server)) * sizeof(WCHAR)),
       "Is allocated with NetApiBufferAllocate\n");

    /* Level 1101 */
    ok(pNetWkstaUserGetInfo(NULL, 1101, (LPBYTE *)&ui1101) == NERR_Success,
       "NetWkstaUserGetInfo is successful\n");
    ok(lstrcmpW(ui1101->wkui1101_oth_domains, ui1->wkui1_oth_domains) == 0,
       "the same oth_domains as returned for level 1\n");
    pNetApiBufferSize(ui1101, &dwSize);
    ok(dwSize >= (sizeof(WKSTA_USER_INFO_1101) +
                 lstrlenW(ui1101->wkui1101_oth_domains) * sizeof(WCHAR)),
       "Is allocated with NetApiBufferAllocate\n");

    pNetApiBufferFree(ui0);
    pNetApiBufferFree(ui1);
    pNetApiBufferFree(ui1101);

    /* errors handling */
    ok(pNetWkstaUserGetInfo(NULL, 10000, (LPBYTE *)&ui0) == ERROR_INVALID_LEVEL,
       "Invalid level\n");
}

static void run_wkstatransportenum_tests(void)
{
    LPBYTE bufPtr;
    NET_API_STATUS apiReturn;
    DWORD entriesRead, totalEntries;

    /* 1st check: is param 2 (level) correct? (only if param 5 passed?) */
    apiReturn = pNetWkstaTransportEnum(NULL, 1, NULL, MAX_PREFERRED_LENGTH,
        NULL, &totalEntries, NULL);
    ok(apiReturn == ERROR_INVALID_LEVEL || apiReturn == ERROR_INVALID_PARAMETER,
       "NetWkstaTransportEnum returned %d\n", apiReturn);

    /* 2nd check: is param 5 passed? (only if level passes?) */
    apiReturn = pNetWkstaTransportEnum(NULL, 0, NULL, MAX_PREFERRED_LENGTH,
        NULL, &totalEntries, NULL);

    /* if no network adapter present, bail, the rest of the test will fail */
    if (apiReturn == ERROR_NETWORK_UNREACHABLE)
        return;

    ok(apiReturn == STATUS_ACCESS_VIOLATION || apiReturn == ERROR_INVALID_PARAMETER,
       "NetWkstaTransportEnum returned %d\n", apiReturn);

    /* 3rd check: is param 3 passed? */
    apiReturn = pNetWkstaTransportEnum(NULL, 0, NULL, MAX_PREFERRED_LENGTH,
        NULL, NULL, NULL);
    ok(apiReturn == STATUS_ACCESS_VIOLATION || apiReturn == RPC_X_NULL_REF_POINTER || apiReturn == ERROR_INVALID_PARAMETER,
       "NetWkstaTransportEnum returned %d\n", apiReturn);

    /* 4th check: is param 6 passed? */
    apiReturn = pNetWkstaTransportEnum(NULL, 0, &bufPtr, MAX_PREFERRED_LENGTH,
        &entriesRead, NULL, NULL);
    ok(apiReturn == RPC_X_NULL_REF_POINTER, "null pointer\n");

    /* final check: valid return, actually get data back */
    apiReturn = pNetWkstaTransportEnum(NULL, 0, &bufPtr, MAX_PREFERRED_LENGTH,
        &entriesRead, &totalEntries, NULL);
    ok(apiReturn == NERR_Success || apiReturn == ERROR_NETWORK_UNREACHABLE || apiReturn == NERR_WkstaNotStarted,
       "NetWkstaTransportEnum returned %d\n", apiReturn);
    if (apiReturn == NERR_Success) {
        /* WKSTA_TRANSPORT_INFO_0 *transports = (WKSTA_TRANSPORT_INFO_0 *)bufPtr; */

        ok(bufPtr != NULL, "got data back\n");
        ok(entriesRead > 0, "read at least one transport\n");
        ok(totalEntries > 0 || broken(totalEntries == 0) /* Win7 */,
           "at least one transport\n");
        pNetApiBufferFree(bufPtr);
    }
}

static void run_wkstajoininfo_tests(void)
{
    NET_API_STATUS ret;
    LPWSTR buffer = NULL;
    NETSETUP_JOIN_STATUS buffertype = 0xdada;
    /* NT4 doesn't have this function */
    if (!pNetGetJoinInformation) {
        win_skip("NetGetJoinInformation not available\n");
        return;
    }

    ret = pNetGetJoinInformation(NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "NetJoinGetInformation returned unexpected 0x%08x\n", ret);
    ok(buffertype == 0xdada, "buffertype set to unexpected value %d\n", buffertype);

    ret = pNetGetJoinInformation(NULL, &buffer, &buffertype);
    ok(ret == NERR_Success, "NetJoinGetInformation returned unexpected 0x%08x\n", ret);
    ok(buffertype != 0xdada && buffertype != NetSetupUnknownStatus, "buffertype set to unexpected value %d\n", buffertype);
    trace("workstation joined to %s with status %d\n", wine_dbgstr_w(buffer), buffertype);
    pNetApiBufferFree(buffer);
}

START_TEST(wksta)
{
    HMODULE hnetapi32=LoadLibraryA("netapi32.dll");

    pNetApiBufferFree=(void*)GetProcAddress(hnetapi32,"NetApiBufferFree");
    pNetApiBufferSize=(void*)GetProcAddress(hnetapi32,"NetApiBufferSize");
    pNetpGetComputerName=(void*)GetProcAddress(hnetapi32,"NetpGetComputerName");
    pNetWkstaUserGetInfo=(void*)GetProcAddress(hnetapi32,"NetWkstaUserGetInfo");
    pNetWkstaTransportEnum=(void*)GetProcAddress(hnetapi32,"NetWkstaTransportEnum");
    pNetGetJoinInformation=(void*)GetProcAddress(hnetapi32,"NetGetJoinInformation");

    /* These functions were introduced with NT. It's safe to assume that
     * if one is not available, none are.
     */
    if (!pNetApiBufferFree) {
        win_skip("Needed functions are not available\n");
        FreeLibrary(hnetapi32);
        return;
    }

    if (init_wksta_tests()) {
        if (pNetpGetComputerName)
            run_get_comp_name_tests();
        else
            win_skip("Function NetpGetComputerName not available\n");
        run_wkstausergetinfo_tests();
        run_wkstatransportenum_tests();
        run_wkstajoininfo_tests();
    }

    FreeLibrary(hnetapi32);
}
