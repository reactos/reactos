/*
 * Copyright 2021 Hugh McMaster
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

#define COPY_SRC  KEY_WINE "\\reg_copy"

static void test_command_syntax(void)
{
    DWORD r;

    run_reg_exe("reg copy", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy /?", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg copy /h", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg copy -H", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg copy /? /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy /h /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy /? /s", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy /h /s", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy /s", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy /s /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy /f /s", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " foo /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy /f HKCU\\" COPY_SRC " HKCU\\" KEY_BASE, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " /f HKCU\\" KEY_BASE, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " /s HKCU\\" KEY_BASE, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE " /a", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE " /f /a", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    /* Source and destination keys are the same */
    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" COPY_SRC, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC "\\ HKCU\\" COPY_SRC, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" COPY_SRC "\\", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC "\\ HKCU\\" COPY_SRC "\\", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" COPY_SRC " /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC "\\ HKCU\\" COPY_SRC " /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" COPY_SRC "\\ /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC "\\ HKCU\\" COPY_SRC "\\ /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" COPY_SRC " /s /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC "\\ HKCU\\" COPY_SRC " /s /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" COPY_SRC "\\ /s /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC "\\ HKCU\\" COPY_SRC "\\ /s /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);
}

static void test_copy_empty_key(void)
{
    DWORD r;

    delete_tree(HKEY_CURRENT_USER, COPY_SRC);
    verify_key_nonexist(HKEY_CURRENT_USER, COPY_SRC);

    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE);

    add_key(HKEY_CURRENT_USER, COPY_SRC, NULL);


    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE " /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKCU\\" KEY_BASE " file.reg", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", empty_key_test, 0), "compare_export() failed\n");

    todo_wine delete_key(HKEY_CURRENT_USER, KEY_BASE);


    run_reg_exe("reg copy HKCU\\" COPY_SRC "\\ HKCU\\" KEY_BASE " /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKCU\\" KEY_BASE " file.reg", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", empty_key_test, 0), "compare_export() failed\n");

    todo_wine delete_key(HKEY_CURRENT_USER, KEY_BASE);


    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE "\\ /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKCU\\" KEY_BASE " file.reg", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", empty_key_test, 0), "compare_export() failed\n");

    todo_wine delete_key(HKEY_CURRENT_USER, KEY_BASE);


    run_reg_exe("reg copy HKCU\\" COPY_SRC "\\ HKCU\\" KEY_BASE "\\ /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKCU\\" KEY_BASE " file.reg", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", empty_key_test, 0), "compare_export() failed\n");


    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE " /s /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKCU\\" KEY_BASE " file.reg", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", empty_key_test, 0), "compare_export() failed\n");
}

static void test_copy_simple_data(void)
{
    HKEY hkey;
    DWORD r, dword;

    delete_tree(HKEY_CURRENT_USER, COPY_SRC);
    verify_key_nonexist(HKEY_CURRENT_USER, COPY_SRC);

    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE);

    add_key(HKEY_CURRENT_USER, COPY_SRC, &hkey);

    dword = 0x100;
    add_value(hkey, "DWORD", REG_DWORD, &dword, sizeof(dword));
    add_value(hkey, "String", REG_SZ, "Your text here...", 18);
    close_key(hkey);


    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE " /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKCU\\" KEY_BASE " file.reg /y", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", simple_data_test, 0), "compare_export() failed\n");

    todo_wine delete_key(HKEY_CURRENT_USER, KEY_BASE);


    run_reg_exe("reg copy HKCU\\" COPY_SRC "\\ HKCU\\" KEY_BASE " /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKCU\\" KEY_BASE " file.reg /y", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", simple_data_test, 0), "compare_export() failed\n");

    todo_wine delete_key(HKEY_CURRENT_USER, KEY_BASE);


    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE "\\ /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKCU\\" KEY_BASE " file.reg /y", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", simple_data_test, 0), "compare_export() failed\n");

    todo_wine delete_key(HKEY_CURRENT_USER, KEY_BASE);


    run_reg_exe("reg copy HKCU\\" COPY_SRC "\\ HKCU\\" KEY_BASE "\\ /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKCU\\" KEY_BASE " file.reg /y", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", simple_data_test, 0), "compare_export() failed\n");


    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE " /s /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKCU\\" KEY_BASE " file.reg /y", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", simple_data_test, 0), "compare_export() failed\n");
}

START_TEST(copy)
{
    DWORD r;

    if (!run_reg_exe("reg.exe /?", &r)) {
        win_skip("reg.exe not available, skipping 'copy' tests\n");
        return;
    }

    test_command_syntax();
    test_copy_empty_key();
    test_copy_simple_data();
}
