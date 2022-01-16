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

static void test_query(void)
{
    DWORD r;
    HKEY key, subkey;
    const char hello[] = "Hello";
    const char world[] = "World";
    const char empty1[] = "Empty1";
    const char empty2[] = "Empty2";
    const DWORD dword1 = 0x123;
    const DWORD dword2 = 0xabc;

    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg query", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg query /?", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query /h", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query -H", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    /* Create a test key */
    add_key(HKEY_CURRENT_USER, KEY_BASE, &key);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /ve", &r);
    ok(r == REG_EXIT_SUCCESS || broken(r == REG_EXIT_FAILURE /* WinXP */),
       "got exit code %d, expected 0\n", r);

    add_value(key, "Test", REG_SZ, hello, sizeof(hello));
    add_value(key, "Wine", REG_DWORD, &dword1, sizeof(dword1));
    add_value(key, NULL, REG_SZ, empty1, sizeof(empty1));

    run_reg_exe("reg query HKCU\\" KEY_BASE, &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /v", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /v Missing", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /v Test", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /v Wine", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /ve", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /v Test /v Wine", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /v Test /ve", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    /* Create a test subkey */
    add_key(key, "Subkey", &subkey);
    add_value(subkey, "Test", REG_SZ, world, sizeof(world));
    add_value(subkey, "Wine", REG_DWORD, &dword2, sizeof(dword2));
    add_value(subkey, NULL, REG_SZ, empty2, sizeof(empty2));

    close_key(subkey);

    run_reg_exe("reg query HKCU\\" KEY_BASE "\\subkey", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE "\\subkey /v Test", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE "\\subkey /v Wine", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE "\\subkey /ve", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    /* Test recursion */
    run_reg_exe("reg query HKCU\\" KEY_BASE " /s", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /v Test /s", &r);
    ok(r == REG_EXIT_SUCCESS || r == REG_EXIT_FAILURE /* WinXP */,
       "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /v Wine /s", &r);
    ok(r == REG_EXIT_SUCCESS || r == REG_EXIT_FAILURE /* WinXP */,
       "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /ve /s", &r);
    ok(r == REG_EXIT_SUCCESS || r == REG_EXIT_FAILURE /* WinXP */,
       "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /s /s", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    /* Clean-up, then query */
    delete_key(key, "subkey");
    close_key(key);

    run_reg_exe("reg query HKCU\\" KEY_BASE "\\subkey", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    delete_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg query HKCU\\" KEY_BASE, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);
}

START_TEST(query)
{
    DWORD r;

    if (!run_reg_exe("reg.exe /?", &r)) {
        win_skip("reg.exe not available, skipping 'query' tests\n");
        return;
    }

    test_query();
}
