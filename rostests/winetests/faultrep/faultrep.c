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

static char regpath_root[] = "Software\\Microsoft\\PCHealth\\ErrorReporting";
static char regpath_exclude[] = "ExclusionList";

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
    ok(res, "got %d and 0x%x (expected TRUE)\n", res, GetLastError());

    /* add, when already present */
    SetLastError(0xdeadbeef);
    res = AddERExcludedApplicationA("winetest_faultrep.exe");
    ok(res, "got %d and 0x%x (expected TRUE)\n", res, GetLastError());

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
