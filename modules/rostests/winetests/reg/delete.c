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

    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE, 0);

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
}

static void test_delete(void)
{
    HKEY hkey;
    DWORD r;
    const DWORD deadbeef = 0xdeadbeef;

    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE, 0);

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

START_TEST(delete)
{
    DWORD r;

    if (!run_reg_exe("reg.exe /?", &r)) {
        win_skip("reg.exe not available, skipping 'delete' tests\n");
        return;
    }

    test_command_syntax();
    test_delete();
}
