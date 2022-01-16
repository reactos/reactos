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

#define COPY_DEST  KEY_WINE "\\reg_copy"

static void test_copy(void)
{
    DWORD r;

    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE);

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

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" KEY_BASE " /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" KEY_BASE " foo /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy /f HKEY_CURRENT_USER\\" KEY_BASE " HKEY_CURRENT_USER\\" KEY_BASE, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy /f HKEY_CURRENT_USER\\" KEY_BASE " HKEY_CURRENT_USER\\" COPY_DEST, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" KEY_BASE " /f HKEY_CURRENT_USER\\" COPY_DEST, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" KEY_BASE " /s HKEY_CURRENT_USER\\" COPY_DEST, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" KEY_BASE " HKEY_CURRENT_USER\\" COPY_DEST " /a", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" KEY_BASE " HKEY_CURRENT_USER\\" COPY_DEST " /f /a", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" KEY_BASE " HKEY_CURRENT_USER\\" KEY_BASE, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" KEY_BASE " HKEY_CURRENT_USER\\" KEY_BASE " /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" KEY_BASE " HKEY_CURRENT_USER\\" KEY_BASE " /s /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);
}

START_TEST(copy)
{
    DWORD r;

    if (!run_reg_exe("reg.exe /?", &r)) {
        win_skip("reg.exe not available, skipping 'copy' tests\n");
        return;
    }

    test_copy();
}
