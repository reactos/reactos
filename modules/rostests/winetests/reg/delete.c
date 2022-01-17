/*
 * Copyright 2014 Akihiro Sagawa
 * Copyright 2016-2018, 2021 Hugh McMaster
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

#include "reg_test.h"

static void test_command_syntax(void)
{
    DWORD r;

    delete_tree(HKEY_CURRENT_USER, KEY_BASE, 0);

    run_reg_exe("reg delete", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete /?", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg delete /h", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg delete -H", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    /* Multiple /v* switches */
    run_reg_exe("reg delete HKCU\\" KEY_BASE " /v Wine /ve", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /v Wine /va", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /ve /ve", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /ve /va", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /va /va", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /v Test /ve /va", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /v Wine /v Test /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    /* No /v argument */
    run_reg_exe("reg delete HKCU\\" KEY_BASE " /v", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /f /v", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    /* Test registry view */
    run_reg_exe("reg delete HKCU\\" KEY_BASE " /f /reg:32 /reg:32", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /f /reg:32 /reg:64", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /f /reg:64 /reg:64", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);
}

static void test_delete(void)
{
    HKEY hkey;
    DWORD r;
    const DWORD deadbeef = 0xdeadbeef;

    delete_tree(HKEY_CURRENT_USER, KEY_BASE, 0);

    /* Create a test key */
    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);
    add_value(hkey, "foo", REG_DWORD, &deadbeef, sizeof(deadbeef));
    add_value(hkey, "bar", REG_DWORD, &deadbeef, sizeof(deadbeef));
    add_value(hkey, NULL, REG_DWORD, &deadbeef, sizeof(deadbeef));

    add_key(hkey, "subkey", 0, NULL);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /v bar /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "bar");

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /ve /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "");

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /va /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "foo");
    verify_key(hkey, "subkey", 0);

    /* Test forward and back slashes */
    add_key(hkey, "https://winehq.org", 0, NULL);
    add_value(hkey, "count/up", REG_SZ, "one/two/three", 14);
    add_value(hkey, "\\foo\\bar", REG_SZ, "", 1);

    run_reg_exe("reg delete HKCU\\" KEY_BASE "\\https://winehq.org /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "https://winehq.org", 0);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /v count/up /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "count/up");

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /v \\foo\\bar /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "\\foo\\bar");

    add_value(hkey, "string\\01", REG_SZ, "Value", 6);
    add_value(hkey, "string2", REG_SZ, "foo\\0bar", 9);
    add_value(hkey, "\\0", REG_SZ, "Value", 6);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /v string\\01 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "string\\01");

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /v string2 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "string2");

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /v \\0 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "\\0");

    close_key(hkey);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE, 0);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);
}

static void create_test_key(REGSAM sam)
{
    HKEY hkey, subkey;
    DWORD dword;

    add_key(HKEY_LOCAL_MACHINE, KEY_BASE, sam, &hkey);

    dword = 0x100;
    add_value(hkey, "DWORD", REG_DWORD, &dword, sizeof(dword));
    add_value(hkey, "String", REG_SZ, "Your text here...", 18);
    add_value(hkey, NULL, REG_SZ, "Default value name", 19);
    add_value(hkey, "Multiple Strings", REG_MULTI_SZ, "Line1\0Line2\0Line3\0", 19);

    add_key(hkey, "Subkey", sam, &subkey);
    add_value(subkey, "Binary", REG_BINARY, "\x11\x22\x33\x44", 4);
    add_value(subkey, "Undefined hex", 0x100, "%PATH%", 7);
    close_key(subkey);

    close_key(hkey);
}

static void test_registry_view_win32(void)
{
    HKEY hkey;
    DWORD r;
    BOOL is_wow64, is_win32;

    IsWow64Process(GetCurrentProcess(), &is_wow64);
    is_win32 = !is_wow64 && (sizeof(void *) == sizeof(int));

    if (!is_win32) return;

    delete_tree(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);

    /* Test deletion from the 32-bit registry view (32-bit Windows) */
    create_test_key(KEY_WOW64_32KEY);

    open_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY, &hkey);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /v DWORD /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "DWORD");

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /ve /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, NULL);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /va /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "String");
    verify_reg_nonexist(hkey, "Multiple Strings");
    verify_key(hkey, "Subkey", KEY_WOW64_32KEY);

    run_reg_exe("reg delete HKLM\\" KEY_BASE "\\Subkey /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey", KEY_WOW64_32KEY);

    close_key(hkey);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);

    /* Test deletion from the 64-bit registry view, which doesn't exist on 32-bit Windows */
    create_test_key(KEY_WOW64_64KEY);

    open_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY, &hkey);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /v DWORD /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "DWORD");

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /ve /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, NULL);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /va /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "String");
    verify_reg_nonexist(hkey, "Multiple Strings");
    verify_key(hkey, "Subkey", KEY_WOW64_64KEY);

    run_reg_exe("reg delete HKLM\\" KEY_BASE "\\Subkey /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey", KEY_WOW64_64KEY);

    close_key(hkey);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY);
}

static void test_registry_view_win64(void)
{
    HKEY hkey;
    DWORD r;
    BOOL is_wow64, is_win64;

    IsWow64Process(GetCurrentProcess(), &is_wow64);
    is_win64 = !is_wow64 && (sizeof(void *) > sizeof(int));

    if (!is_win64) return;

    delete_tree(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);
    delete_tree(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY);

    /* Test deletion from the 32-bit registry view (64-bit Windows) */
    create_test_key(KEY_WOW64_32KEY);

    verify_key_nonexist(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY);

    open_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY, &hkey);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /v DWORD /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "DWORD");

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /ve /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, NULL);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /va /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "String");
    verify_reg_nonexist(hkey, "Multiple Strings");
    verify_key(hkey, "Subkey", KEY_WOW64_32KEY);

    run_reg_exe("reg delete HKLM\\" KEY_BASE "\\Subkey /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey", KEY_WOW64_32KEY);

    close_key(hkey);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);

    /* Test deletion from the 64-bit registry view (64-bit Windows) */
    create_test_key(KEY_WOW64_64KEY);

    verify_key_nonexist(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);

    open_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY, &hkey);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /v DWORD /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "DWORD");

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /ve /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, NULL);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /va /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "String");
    verify_reg_nonexist(hkey, "Multiple Strings");
    verify_key(hkey, "Subkey", KEY_WOW64_64KEY);

    run_reg_exe("reg delete HKLM\\" KEY_BASE "\\Subkey /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey", KEY_WOW64_64KEY);

    close_key(hkey);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY);
}

static void test_registry_view_wow64(void)
{
    HKEY hkey;
    DWORD r;
    BOOL is_wow64;

    IsWow64Process(GetCurrentProcess(), &is_wow64);

    if (!is_wow64) return;

    delete_tree(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);
    delete_tree(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY);

    /* Test deletion from the 32-bit registry view (WOW64) */
    create_test_key(KEY_WOW64_32KEY);

    verify_key_nonexist(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY);

    open_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY, &hkey);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /v DWORD /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "DWORD");

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /ve /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, NULL);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /va /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "String");
    verify_reg_nonexist(hkey, "Multiple Strings");
    verify_key(hkey, "Subkey", KEY_WOW64_32KEY);

    run_reg_exe("reg delete HKLM\\" KEY_BASE "\\Subkey /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey", KEY_WOW64_32KEY);

    close_key(hkey);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);

    /* Test deletion from the 64-bit registry view (WOW64) */
    create_test_key(KEY_WOW64_64KEY);

    verify_key_nonexist(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);

    open_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY, &hkey);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /v DWORD /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "DWORD");

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /ve /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, NULL);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /va /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "String");
    verify_reg_nonexist(hkey, "Multiple Strings");
    verify_key(hkey, "Subkey", KEY_WOW64_64KEY);

    run_reg_exe("reg delete HKLM\\" KEY_BASE "\\Subkey /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey", KEY_WOW64_64KEY);

    close_key(hkey);

    run_reg_exe("reg delete HKLM\\" KEY_BASE " /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY);
}

START_TEST(delete)
{
    DWORD r;

    if (!run_reg_exe("reg.exe /?", &r)) {
        win_skip("reg.exe not available, skipping 'delete' tests\n");
        return;
    }

    test_command_syntax();
    test_delete();

    /* Check if reg.exe is running with elevated privileges */
    if (!is_elevated_process())
    {
        win_skip("reg.exe is not running with elevated privileges; "
                 "skipping registry view tests\n");
        return;
    }

    test_registry_view_win32();
    test_registry_view_win64();
    test_registry_view_wow64();
}
