/*
 * Unit test suite for fault reporting in XP and above
 *
 * Copyright 2010 Detlef Riekenberg
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "errorrep.h"
#include "wine/test.h"

static const char regpath_root[] = "Software\\Microsoft\\PCHealth\\ErrorReporting";
static const char regpath_exclude[] = "ExclusionList";


static BOOL is_process_limited(void)
{
    static BOOL (WINAPI *pCheckTokenMembership)(HANDLE,PSID,PBOOL) = NULL;
    static BOOL (WINAPI *pOpenProcessToken)(HANDLE, DWORD, PHANDLE) = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
    PSID Group;
    BOOL IsInGroup;
    HANDLE token;

    if (!pOpenProcessToken)
    {
        HMODULE hadvapi32 = GetModuleHandleA("advapi32.dll");
        pOpenProcessToken = (void*)GetProcAddress(hadvapi32, "OpenProcessToken");
        pCheckTokenMembership = (void*)GetProcAddress(hadvapi32, "CheckTokenMembership");
        if (!pCheckTokenMembership || !pOpenProcessToken)
        {
            /* Win9x (power to the masses) or NT4 (no way to know) */
            trace("missing pOpenProcessToken or CheckTokenMembership\n");
            return FALSE;
        }
    }

    if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0, &Group) ||
        !pCheckTokenMembership(NULL, Group, &IsInGroup))
    {
        trace("Could not check if the current user is an administrator\n");
        return FALSE;
    }
    if (!IsInGroup)
    {
        if (!AllocateAndInitializeSid(&NtAuthority, 2,
                                      SECURITY_BUILTIN_DOMAIN_RID,
                                      DOMAIN_ALIAS_RID_POWER_USERS,
                                      0, 0, 0, 0, 0, 0, &Group) ||
            !pCheckTokenMembership(NULL, Group, &IsInGroup))
        {
            trace("Could not check if the current user is a power user\n");
            return FALSE;
        }
        if (!IsInGroup)
        {
            /* Only administrators and power users can be powerful */
            return TRUE;
        }
    }

    if (pOpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        BOOL ret;
        TOKEN_ELEVATION_TYPE type = TokenElevationTypeDefault;
        DWORD size;

        ret = GetTokenInformation(token, TokenElevationType, &type, sizeof(type), &size);
        CloseHandle(token);
        return (ret && type == TokenElevationTypeLimited);
    }
    return FALSE;
}


/* ###### */

static void test_AddERExcludedApplicationA(void)
{
    BOOL res;
    LONG lres;
    HKEY hroot;
    HKEY hexclude = 0;

    /* clean state */
    lres = RegCreateKeyA(HKEY_LOCAL_MACHINE, regpath_root, &hroot);
    if (lres == ERROR_ACCESS_DENIED)
    {
        skip("Not enough access rights\n");
        return;
    }

    if (!lres)
        lres = RegOpenKeyA(hroot, regpath_exclude, &hexclude);

    if (!lres)
        RegDeleteValueA(hexclude, "winetest_faultrep.exe");


    SetLastError(0xdeadbeef);
    res = AddERExcludedApplicationA(NULL);
    ok(!res, "got %d and 0x%x (expected FALSE)\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = AddERExcludedApplicationA("");
    ok(!res, "got %d and 0x%x (expected FALSE)\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    /* existence of the path doesn't matter this function succeeded */
    res = AddERExcludedApplicationA("winetest_faultrep.exe");
    if (is_process_limited())
    {
        /* LastError is not set! */
        ok(!res, "AddERExcludedApplicationA should have failed got %d\n", res);
    }
    else
    {
        ok(res, "AddERExcludedApplicationA failed (le=0x%x)\n", GetLastError());

        /* add, when already present */
        SetLastError(0xdeadbeef);
        res = AddERExcludedApplicationA("winetest_faultrep.exe");
        ok(res, "AddERExcludedApplicationA failed (le=0x%x)\n", GetLastError());
    }

    /* cleanup */
    RegDeleteValueA(hexclude, "winetest_faultrep.exe");

    RegCloseKey(hexclude);
    RegCloseKey(hroot);
}

/* ########################### */

START_TEST(faultrep)
{
    test_AddERExcludedApplicationA();
}
