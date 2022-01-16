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
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg copy /h", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg copy -H", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

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

static void test_copy_complex_data(void)
{
    HKEY hkey, subkey;
    DWORD r, dword;

    delete_tree(HKEY_CURRENT_USER, COPY_SRC);
    verify_key_nonexist(HKEY_CURRENT_USER, COPY_SRC);

    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE);

    add_key(HKEY_CURRENT_USER, COPY_SRC, &hkey);

    dword = 0x100;
    add_value(hkey, "DWORD", REG_DWORD, &dword, sizeof(dword));
    add_value(hkey, "String", REG_SZ, "Your text here...", 18);

    add_key(hkey, "Subkey1", &subkey);
    add_value(subkey, "Binary", REG_BINARY, "\x11\x22\x33\x44", 4);
    add_value(subkey, "Undefined hex", 0x100, "%PATH%", 7);
    close_key(subkey);

    add_key(hkey, "Subkey2a", &subkey);
    add_value(subkey, "double\"quote", REG_SZ, "\"Hello, World!\"", 16);
    dword = 0x8;
    add_value(subkey, "single'quote", REG_DWORD, &dword, sizeof(dword));
    close_key(subkey);

    add_key(hkey, "Subkey2a\\Subkey2b", &subkey);
    add_value(subkey, NULL, REG_SZ, "Default value name", 19);
    add_value(subkey, "Multiple strings", REG_MULTI_SZ, "Line1\0Line2\0Line3\0", 19);
    close_key(subkey);

    add_key(hkey, "Subkey3a", &subkey);
    add_value(subkey, "Backslash", REG_SZ, "Use \\\\ to escape a backslash", 29);
    close_key(subkey);

    add_key(hkey, "Subkey3a\\Subkey3b\\Subkey3c", &subkey);
    add_value(subkey, "String expansion", REG_EXPAND_SZ, "%HOME%\\%PATH%", 14);
    add_value(subkey, "Zero data type", REG_NONE, "Value", 6);
    close_key(subkey);

    add_key(hkey, "Subkey4", &subkey);
    dword = 0x12345678;
    add_value(subkey, NULL, REG_DWORD, &dword, sizeof(dword));
    add_value(subkey, "43981", 0xabcd, "Value", 6);
    close_key(subkey);

    close_key(hkey);

    /* Copy values only */
    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE " /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg /y", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", simple_data_test, 0), "compare_export() failed\n");

    /* Copy subkeys and values */
    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE " /s /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg /y", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", complex_data_test, 0), "compare_export() failed\n");
}

static void test_copy_key_order(void)
{
    HKEY hkey;
    DWORD r;

    delete_tree(HKEY_CURRENT_USER, COPY_SRC);
    verify_key_nonexist(HKEY_CURRENT_USER, COPY_SRC);

    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE);

    add_key(HKEY_CURRENT_USER, COPY_SRC, &hkey);
    add_key(hkey, "Subkey2", NULL);
    add_key(hkey, "Subkey1", NULL);
    close_key(hkey);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE " /s /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKCU\\" KEY_BASE " file.reg /y", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", key_order_test, 0), "compare_export() failed\n");
}

static void test_copy_value_order(void)
{
    HKEY hkey;
    DWORD r;

    delete_tree(HKEY_CURRENT_USER, COPY_SRC);
    verify_key_nonexist(HKEY_CURRENT_USER, COPY_SRC);

    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE);

    add_key(HKEY_CURRENT_USER, COPY_SRC, &hkey);
    add_value(hkey, "Value 2", REG_SZ, "I was added first!", 19);
    add_value(hkey, "Value 1", REG_SZ, "I was added second!", 20);
    close_key(hkey);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE " /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKCU\\" KEY_BASE " file.reg /y", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", value_order_test, 0), "compare_export() failed\n");
}

static void test_copy_hex_data(void)
{
    HKEY hkey;
    DWORD r;

    delete_tree(HKEY_CURRENT_USER, COPY_SRC);
    verify_key_nonexist(HKEY_CURRENT_USER, COPY_SRC);

    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE);

    /* Try copying empty hex values */
    add_key(HKEY_CURRENT_USER, COPY_SRC, &hkey);
    add_value(hkey, "Wine1a", REG_NONE, NULL, 0);
    add_value(hkey, "Wine1b", REG_SZ, NULL, 0);
    add_value(hkey, "Wine1c", REG_EXPAND_SZ, NULL, 0);
    add_value(hkey, "Wine1d", REG_BINARY, NULL, 0);
    add_value(hkey, "Wine1e", REG_DWORD, NULL, 0);
    add_value(hkey, "Wine1f", REG_MULTI_SZ, NULL, 0);
    add_value(hkey, "Wine1g", 0x100, NULL, 0);
    add_value(hkey, "Wine1h", 0xabcd, NULL, 0);
    close_key(hkey);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE " /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKCU\\" KEY_BASE " file.reg /y", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", empty_hex_test, 0), "compare_export() failed\n");

    delete_key(HKEY_CURRENT_USER, COPY_SRC);
    todo_wine delete_key(HKEY_CURRENT_USER, KEY_BASE);

    /* Try copying after importing alternative registry data types */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" COPY_SRC "]\n"
                     "\"Wine2a\"=hex(1):\n"
                     "\"Wine2b\"=hex(3):\n"
                     "\"Wine2c\"=hex(4):\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE " /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKCU\\" KEY_BASE " file.reg /y", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", empty_hex_test2, 0), "compare_export() failed\n");

    delete_key(HKEY_CURRENT_USER, COPY_SRC);
    todo_wine delete_key(HKEY_CURRENT_USER, KEY_BASE);

    /* Try copying more complex hex data */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" COPY_SRC "]\n"
                     "\"Wine3a\"=hex(1):56,00,61,00,6c,00,75,00,65,00,00,00\n"
                     "\"Wine3b\"=hex(3):12,34,56,78\n"
                     "\"Wine3c\"=hex(4):40,30,20,10\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE " /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg /y", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", hex_types_test, 0), "compare_export() failed\n");
}

static void test_copy_embedded_null_values(void)
{
    DWORD r;

    delete_tree(HKEY_CURRENT_USER, COPY_SRC);
    verify_key_nonexist(HKEY_CURRENT_USER, COPY_SRC);

    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" COPY_SRC "]\n"
                     "\"Wine4a\"=dword:00000005\n"
                     "\"Wine4b\"=hex(1):00,00,00,00,00,00,00,00\n"
                     "\"Wine4c\"=\"Value\"\n"
                     "\"Wine4d\"=hex(1):00,00,61,00,62,00,63,00\n"
                     "\"Wine4e\"=dword:00000100\n"
                     "\"Wine4f\"=hex(1):00,00,56,00,61,00,6c,00,75,00,65,00,00,00\n"
                     "\"Wine4g\"=\"Value2\"\n"
                     "\"Wine4h\"=hex(1):61,00,62,00,63,00,00,00, \\\n"
                     "  64,00,65,00,66,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE " /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg /y", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", embedded_null_test, 0), "compare_export() failed\n");
}

static void test_copy_slashes(void)
{
    HKEY hkey;
    DWORD r;

    delete_tree(HKEY_CURRENT_USER, COPY_SRC);
    verify_key_nonexist(HKEY_CURRENT_USER, COPY_SRC);

    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE);

    add_key(HKEY_CURRENT_USER, COPY_SRC, &hkey);
    add_key(hkey, "https://winehq.org", NULL);
    add_value(hkey, "count/up", REG_SZ, "one/two/three", 14);
    add_value(hkey, "\\foo\\bar", REG_SZ, "", 1);
    close_key(hkey);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE " /s /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKCU\\" KEY_BASE " file.reg /y", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", slashes_test, 0), "compare_export() failed\n");
}

static void test_copy_escaped_null_values(void)
{
    HKEY hkey;
    DWORD r;

    delete_tree(HKEY_CURRENT_USER, COPY_SRC);
    verify_key_nonexist(HKEY_CURRENT_USER, COPY_SRC);

    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE);

    add_key(HKEY_CURRENT_USER, COPY_SRC, &hkey);
    add_value(hkey, "Wine5a", REG_SZ, "\\0", 3);
    add_value(hkey, "Wine5b", REG_SZ, "\\0\\0", 5);
    add_value(hkey, "Wine5c", REG_SZ, "Value1\\0", 9);
    add_value(hkey, "Wine5d", REG_SZ, "Value2\\0\\0\\0\\0", 15);
    add_value(hkey, "Wine5e", REG_SZ, "Value3\\0Value4", 15);
    add_value(hkey, "Wine5f", REG_SZ, "\\0Value5", 9);
    close_key(hkey);

    run_reg_exe("reg copy HKCU\\" COPY_SRC " HKCU\\" KEY_BASE " /f", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg export HKCU\\" KEY_BASE " file.reg /y", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine ok(compare_export("file.reg", escaped_null_test, 0), "compare_export() failed\n");

    delete_key(HKEY_CURRENT_USER, COPY_SRC);
    todo_wine delete_key(HKEY_CURRENT_USER, KEY_BASE);
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
    test_copy_complex_data();
    test_copy_key_order();
    test_copy_value_order();
    test_copy_hex_data();
    test_copy_embedded_null_values();
    test_copy_slashes();
    test_copy_escaped_null_values();
}
