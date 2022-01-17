/*
 * Copyright 2017-2018, 2021 Hugh McMaster
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

BOOL compare_export_(const char *file, unsigned line, const char *filename,
                     const char *expected, DWORD todo)
{
    FILE *fp;
    long file_size;
    WCHAR *fbuf = NULL, *wstr = NULL;
    size_t len;
    BOOL ret = FALSE;

    fp = fopen(filename, "rb");
    if (!fp) return FALSE;

    if (fseek(fp, 0, SEEK_END)) goto error;
    file_size = ftell(fp);
    if (file_size == -1) goto error;
    rewind(fp);

    fbuf = HeapAlloc(GetProcessHeap(), 0, file_size + sizeof(WCHAR));
    if (!fbuf) goto error;

    fread(fbuf, file_size, 1, fp);
    fbuf[file_size/sizeof(WCHAR)] = 0;
    fclose(fp);

    len = MultiByteToWideChar(CP_UTF8, 0, expected, -1, NULL, 0);
    wstr = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!wstr) goto exit;
    MultiByteToWideChar(CP_UTF8, 0, expected, -1, wstr, len);

    todo_wine_if (todo & TODO_REG_COMPARE)
        lok(!lstrcmpW(fbuf, wstr), "export data does not match expected data\n");

    ret = DeleteFileA(filename);
    lok(ret, "DeleteFile failed: %u\n", GetLastError());

exit:
    HeapFree(GetProcessHeap(), 0, fbuf);
    HeapFree(GetProcessHeap(), 0, wstr);
    return ret;

error:
    fclose(fp);
    return FALSE;
}

const char *empty_key_test =
    "\xef\xbb\xbfWindows Registry Editor Version 5.00\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "]\r\n\r\n";

const char *simple_data_test =
    "\xef\xbb\xbfWindows Registry Editor Version 5.00\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "]\r\n"
    "\"DWORD\"=dword:00000100\r\n"
    "\"String\"=\"Your text here...\"\r\n\r\n";

const char *complex_data_test =
    "\xef\xbb\xbfWindows Registry Editor Version 5.00\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "]\r\n"
    "\"DWORD\"=dword:00000100\r\n"
    "\"String\"=\"Your text here...\"\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1]\r\n"
    "\"Binary\"=hex:11,22,33,44\r\n"
    "\"Undefined hex\"=hex(100):25,50,41,54,48,25,00\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\r\n"
    "\"double\\\"quote\"=\"\\\"Hello, World!\\\"\"\r\n"
    "\"single'quote\"=dword:00000008\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a\\Subkey2b]\r\n"
    "@=\"Default value name\"\r\n"
    "\"Multiple strings\"=hex(7):4c,00,69,00,6e,00,65,00,31,00,00,00,4c,00,69,00,6e,\\\r\n"
    "  00,65,00,32,00,00,00,4c,00,69,00,6e,00,65,00,33,00,00,00,00,00\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey3a]\r\n"
    "\"Backslash\"=\"Use \\\\\\\\ to escape a backslash\"\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey3a\\Subkey3b]\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey3a\\Subkey3b\\Subkey3c]\r\n"
    "\"String expansion\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,25,00,50,\\\r\n"
    "  00,41,00,54,00,48,00,25,00,00,00\r\n"
    "\"Zero data type\"=hex(0):56,61,6c,75,65,00\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey4]\r\n"
    "@=dword:12345678\r\n"
    "\"43981\"=hex(abcd):56,61,6c,75,65,00\r\n\r\n";

const char *key_order_test =
    "\xef\xbb\xbfWindows Registry Editor Version 5.00\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "]\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1]\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2]\r\n\r\n";

const char *value_order_test =
    "\xef\xbb\xbfWindows Registry Editor Version 5.00\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "]\r\n"
    "\"Value 2\"=\"I was added first!\"\r\n"
    "\"Value 1\"=\"I was added second!\"\r\n\r\n";

const char *empty_hex_test =
    "\xef\xbb\xbfWindows Registry Editor Version 5.00\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "]\r\n"
    "\"Wine1a\"=hex(0):\r\n"
    "\"Wine1b\"=\"\"\r\n"
    "\"Wine1c\"=hex(2):\r\n"
    "\"Wine1d\"=hex:\r\n"
    "\"Wine1e\"=hex(4):\r\n"
    "\"Wine1f\"=hex(7):\r\n"
    "\"Wine1g\"=hex(100):\r\n"
    "\"Wine1h\"=hex(abcd):\r\n\r\n";

const char *empty_hex_test2 =
    "\xef\xbb\xbfWindows Registry Editor Version 5.00\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "]\r\n"
    "\"Wine2a\"=\"\"\r\n"
    "\"Wine2b\"=hex:\r\n"
    "\"Wine2c\"=hex(4):\r\n\r\n";

const char *hex_types_test =
    "\xef\xbb\xbfWindows Registry Editor Version 5.00\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "]\r\n"
    "\"Wine3a\"=\"Value\"\r\n"
    "\"Wine3b\"=hex:12,34,56,78\r\n"
    "\"Wine3c\"=dword:10203040\r\n\r\n";

const char *embedded_null_test =
    "\xef\xbb\xbfWindows Registry Editor Version 5.00\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "]\r\n"
    "\"Wine4a\"=dword:00000005\r\n"
    "\"Wine4b\"=\"\"\r\n"
    "\"Wine4c\"=\"Value\"\r\n"
    "\"Wine4d\"=\"\"\r\n"
    "\"Wine4e\"=dword:00000100\r\n"
    "\"Wine4f\"=\"\"\r\n"
    "\"Wine4g\"=\"Value2\"\r\n"
    "\"Wine4h\"=\"abc\"\r\n\r\n";

const char *slashes_test =
    "\xef\xbb\xbfWindows Registry Editor Version 5.00\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "]\r\n"
    "\"count/up\"=\"one/two/three\"\r\n"
    "\"\\\\foo\\\\bar\"=\"\"\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "\\https://winehq.org]\r\n\r\n";

const char *escaped_null_test =
    "\xef\xbb\xbfWindows Registry Editor Version 5.00\r\n\r\n"
    "[HKEY_CURRENT_USER\\" KEY_BASE "]\r\n"
    "\"Wine5a\"=\"\\\\0\"\r\n"
    "\"Wine5b\"=\"\\\\0\\\\0\"\r\n"
    "\"Wine5c\"=\"Value1\\\\0\"\r\n"
    "\"Wine5d\"=\"Value2\\\\0\\\\0\\\\0\\\\0\"\r\n"
    "\"Wine5e\"=\"Value3\\\\0Value4\"\r\n"
    "\"Wine5f\"=\"\\\\0Value5\"\r\n\r\n";


/* Unit tests */

static void test_export(void)
{
    LONG err;
    DWORD r, dword, type, size;
    HKEY hkey, subkey;
    BYTE hex[4], buffer[8];

    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE, 0);

    run_reg_exe("reg export", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg export /?", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg export /h", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg export -H", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg export \\\\remote-pc\\HKLM\\Wine file.reg", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg export HKEY_DYN_DATA file.reg", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg export HKDD file.reg", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg export file.reg", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg export file.reg HKEY_CURRENT_USER\\" KEY_BASE, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg file2.reg", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    /* Test registry export with an empty key */
    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", empty_key_test, 0), "compare_export() failed\n");

    run_reg_exe("reg export /y HKEY_CURRENT_USER\\" KEY_BASE " file.reg", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " /y file.reg", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg /y", &r);
    ok(r == REG_EXIT_SUCCESS || broken(r == REG_EXIT_FAILURE), /* winxp */
       "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", empty_key_test, 0), "compare_export() failed\n");

    /* Test registry export with a simple data structure */
    dword = 0x100;
    add_value(hkey, "DWORD", REG_DWORD, &dword, sizeof(dword));
    add_value(hkey, "String", REG_SZ, "Your text here...", 18);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg /y", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", simple_data_test, 0), "compare_export() failed\n");

    /* Test whether a .reg file extension is required when exporting */
    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " foo /y", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("foo", simple_data_test, 0), "compare_export() failed\n");

    /* Test registry export with a complex data structure */
    add_key(hkey, "Subkey1", 0, &subkey);
    add_value(subkey, "Binary", REG_BINARY, "\x11\x22\x33\x44", 4);
    add_value(subkey, "Undefined hex", 0x100, "%PATH%", 7);
    close_key(subkey);

    add_key(hkey, "Subkey2a", 0, &subkey);
    add_value(subkey, "double\"quote", REG_SZ, "\"Hello, World!\"", 16);
    dword = 0x8;
    add_value(subkey, "single'quote", REG_DWORD, &dword, sizeof(dword));
    close_key(subkey);

    add_key(hkey, "Subkey2a\\Subkey2b", 0, &subkey);
    add_value(subkey, NULL, REG_SZ, "Default value name", 19);
    add_value(subkey, "Multiple strings", REG_MULTI_SZ, "Line1\0Line2\0Line3\0", 19);
    close_key(subkey);

    add_key(hkey, "Subkey3a", 0, &subkey);
    add_value(subkey, "Backslash", REG_SZ, "Use \\\\ to escape a backslash", 29);
    close_key(subkey);

    add_key(hkey, "Subkey3a\\Subkey3b\\Subkey3c", 0, &subkey);
    add_value(subkey, "String expansion", REG_EXPAND_SZ, "%HOME%\\%PATH%", 14);
    add_value(subkey, "Zero data type", REG_NONE, "Value", 6);
    close_key(subkey);

    add_key(hkey, "Subkey4", 0, &subkey);
    dword = 0x12345678;
    add_value(subkey, NULL, REG_DWORD, &dword, sizeof(dword));
    add_value(subkey, "43981", 0xabcd, "Value", 6);
    close_key(subkey);

    close_key(hkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg /y", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", complex_data_test, 0), "compare_export() failed\n");
    delete_tree(HKEY_CURRENT_USER, KEY_BASE);

    /* Test the export order of registry keys */
    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);
    add_key(hkey, "Subkey2", 0, NULL);
    add_key(hkey, "Subkey1", 0, NULL);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg /y", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", key_order_test, 0), "compare_export() failed\n");
    delete_key(hkey, "Subkey1", 0);
    delete_key(hkey, "Subkey2", 0);

    /* Test the export order of registry values. Windows exports registry values
     * in order of creation; Wine uses alphabetical order.
     */
    add_value(hkey, "Value 2", REG_SZ, "I was added first!", 19);
    add_value(hkey, "Value 1", REG_SZ, "I was added second!", 20);
    close_key(hkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg /y", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", value_order_test, TODO_REG_COMPARE), "compare_export() failed\n");
    delete_key(HKEY_CURRENT_USER, KEY_BASE, 0);

    /* Test registry export with empty hex data */
    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);
    add_value(hkey, "Wine1a", REG_NONE, NULL, 0);
    add_value(hkey, "Wine1b", REG_SZ, NULL, 0);
    add_value(hkey, "Wine1c", REG_EXPAND_SZ, NULL, 0);
    add_value(hkey, "Wine1d", REG_BINARY, NULL, 0);
    add_value(hkey, "Wine1e", REG_DWORD, NULL, 0);
    add_value(hkey, "Wine1f", REG_MULTI_SZ, NULL, 0);
    add_value(hkey, "Wine1g", 0x100, NULL, 0);
    add_value(hkey, "Wine1h", 0xabcd, NULL, 0);
    close_key(hkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg /y", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", empty_hex_test, 0), "compare_export() failed\n");
    delete_key(HKEY_CURRENT_USER, KEY_BASE, 0);

    /* Test registry export after importing alternative registry data types */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine2a\"=hex(1):\n"
                     "\"Wine2b\"=hex(3):\n"
                     "\"Wine2c\"=hex(4):\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    open_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);
    verify_reg(hkey, "Wine2a", REG_SZ, NULL, 0, 0);
    verify_reg(hkey, "Wine2b", REG_BINARY, NULL, 0, 0);
    verify_reg(hkey, "Wine2c", REG_DWORD, NULL, 0, 0);
    close_key(hkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg /y", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", empty_hex_test2, 0), "compare_export() failed\n");
    delete_key(HKEY_CURRENT_USER, KEY_BASE, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine3a\"=hex(1):56,00,61,00,6c,00,75,00,65,00,00,00\n"
                     "\"Wine3b\"=hex(3):12,34,56,78\n"
                     "\"Wine3c\"=hex(4):40,30,20,10\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    open_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);
    verify_reg(hkey, "Wine3a", REG_SZ, "Value", 6, 0);
    memcpy(hex, "\x12\x34\x56\x78", 4);
    verify_reg(hkey, "Wine3b", REG_BINARY, hex, 4, 0);
    dword = 0x10203040;
    verify_reg(hkey, "Wine3c", REG_DWORD, &dword, sizeof(dword), 0);
    close_key(hkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg /y", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", hex_types_test, 0), "compare_export() failed\n");
    delete_key(HKEY_CURRENT_USER, KEY_BASE, 0);

    /* Test registry export with embedded null characters */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
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
    open_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);
    dword = 0x5;
    verify_reg(hkey, "Wine4a", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine4b", REG_SZ, "\0\0\0\0\0\0\0", 4, 0);
    verify_reg(hkey, "Wine4c", REG_SZ, "Value", 6, 0);
    /* Wine4d */
    size = sizeof(buffer);
    err = RegQueryValueExA(hkey, "Wine4d", NULL, &type, (BYTE *)&buffer, &size);
    ok(err == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", err);
    ok(type == REG_SZ, "got wrong type %u, expected %u\n", type, REG_SZ);
    ok(size == 5 || broken(size == 4) /* WinXP */, "got wrong size %u, expected 5\n", size);
    ok(memcmp(buffer, "\0abc", size) == 0, "got wrong data\n");
    dword = 0x100;
    verify_reg(hkey, "Wine4e", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine4f", REG_SZ, "\0Value", 7, 0);
    verify_reg(hkey, "Wine4g", REG_SZ, "Value2", 7, 0);
    verify_reg(hkey, "Wine4h", REG_SZ, "abc\0def", 8, 0);
    close_key(hkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg /y", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", embedded_null_test, 0), "compare_export() failed\n");
    delete_key(HKEY_CURRENT_USER, KEY_BASE, 0);

    /* Test registry export with forward and back slashes */
    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);
    add_key(hkey, "https://winehq.org", 0, NULL);
    add_value(hkey, "count/up", REG_SZ, "one/two/three", 14);
    add_value(hkey, "\\foo\\bar", REG_SZ, "", 1);
    close_key(hkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg /y", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", slashes_test, TODO_REG_COMPARE), "compare_export() failed\n");
    delete_tree(HKEY_CURRENT_USER, KEY_BASE);

    /* Test escaped null characters */
    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);
    add_value(hkey, "Wine5a", REG_SZ, "\\0", 3);
    add_value(hkey, "Wine5b", REG_SZ, "\\0\\0", 5);
    add_value(hkey, "Wine5c", REG_SZ, "Value1\\0", 9);
    add_value(hkey, "Wine5d", REG_SZ, "Value2\\0\\0\\0\\0", 15);
    add_value(hkey, "Wine5e", REG_SZ, "Value3\\0Value4", 15);
    add_value(hkey, "Wine5f", REG_SZ, "\\0Value5", 9);
    close_key(hkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg /y", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", escaped_null_test, 0), "compare_export() failed\n");
    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
}

START_TEST(export)
{
    DWORD r;

    if (!run_reg_exe("reg.exe /?", &r)) {
        win_skip("reg.exe not available, skipping 'export' tests\n");
        return;
    }

    test_export();
}
