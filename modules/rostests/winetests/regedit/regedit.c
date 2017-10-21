/*
 * Copyright 2010 Andrew Eikum for CodeWeavers
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

#include <windows.h>

#include "wine/test.h"

#define lok ok_(__FILE__,line)

#define run_regedit_exe(c) run_regedit_exe_(__LINE__,c)
static BOOL run_regedit_exe_(unsigned line, const char *cmd)
{
    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION pi;
    DWORD ret;
    char cmdline[256];

    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput  = INVALID_HANDLE_VALUE;
    si.hStdOutput = INVALID_HANDLE_VALUE;
    si.hStdError  = INVALID_HANDLE_VALUE;

    strcpy(cmdline, cmd);
    if (!CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        return FALSE;

    ret = WaitForSingleObject(pi.hProcess, 10000);
    if (ret == WAIT_TIMEOUT)
        TerminateProcess(pi.hProcess, 1);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return (ret != WAIT_TIMEOUT);
}

#define exec_import_str(c) r_exec_import_str(__LINE__, c)
static BOOL r_exec_import_str(unsigned line, const char *file_contents)
{
    HANDLE regfile;
    DWORD written;
    BOOL br;

    regfile = CreateFileA("test.reg", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    lok(regfile != INVALID_HANDLE_VALUE, "Failed to create test.reg file\n");
    if(regfile == INVALID_HANDLE_VALUE)
        return FALSE;

    br = WriteFile(regfile, file_contents, strlen(file_contents), &written, NULL);
    lok(br, "WriteFile failed: %u\n", GetLastError());
    CloseHandle(regfile);

    run_regedit_exe("regedit.exe /s test.reg");

    br = DeleteFileA("test.reg");
    lok(br, "DeleteFile failed: %u\n", GetLastError());

    return br;
}

#define exec_import_wstr(c) r_exec_import_wstr(__LINE__, c)
static BOOL r_exec_import_wstr(unsigned line, const char *file_contents)
{
    int lenA, len, memsize;
    WCHAR *wstr;
    HANDLE regfile;
    DWORD written;
    BOOL br;

    lenA = strlen(file_contents);

    len = MultiByteToWideChar(CP_UTF8, 0, file_contents, lenA, NULL, 0);
    memsize = len * sizeof(WCHAR);
    wstr = HeapAlloc(GetProcessHeap(), 0, memsize);
    if (!wstr) return FALSE;
    MultiByteToWideChar(CP_UTF8, 0, file_contents, lenA, wstr, len);

    regfile = CreateFileA("test.reg", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    lok(regfile != INVALID_HANDLE_VALUE, "Failed to create test.reg file\n");
    if (regfile == INVALID_HANDLE_VALUE)
    {
        HeapFree(GetProcessHeap(), 0, wstr);
        return FALSE;
    }

    br = WriteFile(regfile, wstr, memsize, &written, NULL);
    lok(br, "WriteFile failed: %u\n", GetLastError());
    CloseHandle(regfile);

    HeapFree(GetProcessHeap(), 0, wstr);

    run_regedit_exe("regedit.exe /s test.reg");

    br = DeleteFileA("test.reg");
    lok(br, "DeleteFile failed: %u\n", GetLastError());

    return br;
}

#define TODO_REG_TYPE    (0x0001u)
#define TODO_REG_SIZE    (0x0002u)
#define TODO_REG_DATA    (0x0004u)

/* verify_reg() adapted from programs/reg/tests/reg.c */
#define verify_reg(k,v,t,d,s,todo) verify_reg_(__LINE__,k,v,t,d,s,todo)
static void verify_reg_(unsigned line, HKEY hkey, const char *value,
                        DWORD exp_type, const void *exp_data, DWORD exp_size, DWORD todo)
{
    DWORD type, size;
    BYTE data[256];
    LONG err;

    size = sizeof(data);
    memset(data, 0xdd, size);
    err = RegQueryValueExA(hkey, value, NULL, &type, data, &size);
    lok(err == ERROR_SUCCESS, "RegQueryValueEx failed: got %d\n", err);
    if (err != ERROR_SUCCESS)
        return;

    todo_wine_if (todo & TODO_REG_TYPE)
        lok(type == exp_type, "got wrong type %d, expected %d\n", type, exp_type);
    todo_wine_if (todo & TODO_REG_SIZE)
        lok(size == exp_size, "got wrong size %d, expected %d\n", size, exp_size);
    todo_wine_if (todo & TODO_REG_DATA)
        lok(memcmp(data, exp_data, size) == 0, "got wrong data\n");
}

#define verify_reg_nonexist(k,n) r_verify_reg_nonexist(__LINE__,k,n)
static void r_verify_reg_nonexist(unsigned line, HKEY key, const char *value_name)
{
    LONG lr;

    lr = RegQueryValueExA(key, value_name, NULL, NULL, NULL, NULL);
    lok(lr == ERROR_FILE_NOT_FOUND, "registry value '%s' shouldn't exist; got %d, expected 2\n",
       (value_name && *value_name) ? value_name : "(Default)", lr);
}

#define verify_key(k,s) verify_key_(__LINE__,k,s)
static void verify_key_(unsigned line, HKEY key_base, const char *subkey)
{
    HKEY hkey;
    LONG lr;

    lr = RegOpenKeyExA(key_base, subkey, 0, KEY_READ, &hkey);
    lok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: got %d\n", lr);

    if (hkey)
        RegCloseKey(hkey);
}

#define verify_key_nonexist(k,s) verify_key_nonexist_(__LINE__,k,s)
static void verify_key_nonexist_(unsigned line, HKEY key_base, const char *subkey)
{
    HKEY hkey;
    LONG lr;

    lr = RegOpenKeyExA(key_base, subkey, 0, KEY_READ, &hkey);
    lok(lr == ERROR_FILE_NOT_FOUND, "registry key '%s' shouldn't exist; got %d, expected 2\n",
        subkey, lr);

    if (hkey)
        RegCloseKey(hkey);
}

#define KEY_BASE "Software\\Wine\\regedit_test"

static void test_basic_import(void)
{
    HKEY hkey, subkey;
    DWORD dword = 0x17, type, size;
    char exp_binary[] = {0xAA,0xBB,0xCC,0x11};
    LONG lr;
    char buffer[256];
    BYTE hex[8];

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND,
            "RegDeleteKeyA failed: %d\n", lr);

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"TestValue\"=\"AValue\"\n");
    lr = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: %d\n", lr);
    verify_reg(hkey, "TestValue", REG_SZ, "AValue", 7, 0);

    exec_import_str("REGEDIT4\r\n\r\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\r\n"
                "\"TestValue2\"=\"BValue\"\r\n");
    verify_reg(hkey, "TestValue2", REG_SZ, "BValue", 7, 0);

    exec_import_str("Windows Registry Editor Version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"TestValue3\"=\"Value\"\n");
    verify_reg(hkey, "TestValue3", REG_SZ, "Value", 6, 0);

    exec_import_str("REGEDIT4\r\r"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\r"
                "\"TestValue4\"=\"DValue\"\r");
    verify_reg(hkey, "TestValue4", REG_SZ, "DValue", 7, 0);

    exec_import_str("Windows Registry Editor Version 5.00\r\r"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\r"
                    "\"TestValue5\"=\"Value\"\r");
    verify_reg(hkey, "TestValue5", REG_SZ, "Value", 6, 0);

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"TestDword\"=dword:00000017\n");
    verify_reg(hkey, "TestDword", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"TestBinary\"=hex:aa,bb,cc,11\n");
    verify_reg(hkey, "TestBinary", REG_BINARY, exp_binary, sizeof(exp_binary), 0);

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"With=Equals\"=\"asdf\"\n");
    verify_reg(hkey, "With=Equals", REG_SZ, "asdf", 5, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Empty string\"=\"\"\n"
                    "\"\"=\"Default registry value\"\n\n");
    verify_reg(hkey, "Empty string", REG_SZ, "", 1, 0);
    verify_reg(hkey, NULL, REG_SZ, "Default registry value", 23, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Line1\"=\"Value1\"\n\n"
                    "\"Line2\"=\"Value2\"\n\n\n"
                    "\"Line3\"=\"Value3\"\n\n\n\n"
                    "\"Line4\"=\"Value4\"\n\n");
    verify_reg(hkey, "Line1", REG_SZ, "Value1", 7, 0);
    verify_reg(hkey, "Line2", REG_SZ, "Value2", 7, 0);
    verify_reg(hkey, "Line3", REG_SZ, "Value3", 7, 0);
    verify_reg(hkey, "Line4", REG_SZ, "Value4", 7, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine1\"=dword:00000782\n\n"
                    "\"Wine2\"=\"Test Value\"\n"
                    "\"Wine3\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "#comment\n"
                    "@=\"Test\"\n"
                    ";comment\n\n"
                    "\"Wine4\"=dword:12345678\n\n");
    dword = 0x782;
    verify_reg(hkey, "Wine1", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine2", REG_SZ, "Test Value", 11, 0);
    verify_reg(hkey, "Wine3", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "", REG_SZ, "Test", 5, 0);
    dword = 0x12345678;
    verify_reg(hkey, "Wine4", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine5\"=\"No newline\"");
    verify_reg(hkey, "Wine5", REG_SZ, "No newline", 11, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine6\"=dword:00000050\n\n"
                    "\"Wine7\"=\"No newline\"");
    dword = 0x50;
    verify_reg(hkey, "Wine6", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine7", REG_SZ, "No newline", 11, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine8a\"=dword:1\n"
                    "\"Wine8b\"=dword:4444\n\n");
    dword = 0x1;
    verify_reg(hkey, "Wine8a", REG_DWORD, &dword, sizeof(dword), 0);
    dword = 0x4444;
    verify_reg(hkey, "Wine8b", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine9a\"=hex(2):4c,69,6e,65,00\n"
                    "\"Wine9b\"=\"Value 1\"\n"
                    "\"Wine9c\"=hex(2):4c,69,6e,65\\\n"
                    "\"Wine9d\"=\"Value 2\"\n"
                    "\"Wine9e\"=hex(2):4c,69,6e,65,\\\n"
                    "\"Wine9f\"=\"Value 3\"\n"
                    "\"Wine9g\"=\"Value 4\"\n\n");
    verify_reg(hkey, "Wine9a", REG_EXPAND_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine9b", REG_SZ, "Value 1", 8, 0);
    verify_reg_nonexist(hkey, "Wine9c");
    verify_reg(hkey, "Wine9d", REG_SZ, "Value 2", 8, 0);
    verify_reg_nonexist(hkey, "Wine9e");
    verify_reg_nonexist(hkey, "Wine9f");
    verify_reg(hkey, "Wine9g", REG_SZ, "Value 4", 8, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"double\\\"quote\"=\"valid \\\"or\\\" not\"\n"
                    "\"single'quote\"=dword:00000008\n\n");
    verify_reg(hkey, "double\"quote", REG_SZ, "valid \"or\" not", 15, 0);
    dword = 0x00000008;
    verify_reg(hkey, "single'quote", REG_DWORD, &dword, sizeof(dword), 0);

    /* Test hex data concatenation for REG_NONE, REG_EXPAND_SZ and REG_BINARY */
    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine10a\"=hex(0):56,00,61,00,6c,00,75,00,65,00,00,00\n"
                    "\"Wine10b\"=hex(0):56,00,61,00,6c,00,\\\n"
                    "  75,00,65,00,00,00\n"
                    "\"Wine10c\"=hex(0):56,00,61,00,\\;comment\n"
                    "  6c,00,75,00,\\\n"
                    "  65,00,00,00\n"
                    "\"Wine10d\"=hex(0):56,00,61,00,\\;comment\n"
                    "  6c,00,75,00,\n"
                    "  65,00,00,00\n"
                    "\"Wine10e\"=hex(0):56,00,61,00,\\;comment\n"
                    "  6c,00,75,00,;comment\n"
                    "  65,00,00,00\n");
    verify_reg(hkey, "Wine10a", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    verify_reg(hkey, "Wine10b", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    verify_reg(hkey, "Wine10c", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    verify_reg(hkey, "Wine10d", REG_NONE, "V\0a\0l\0u", 8, 0);
    verify_reg(hkey, "Wine10e", REG_NONE, "V\0a\0l\0u", 8, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine11a\"=hex(2):25,50,41,54,48,25,00\n"
                    "\"Wine11b\"=hex(2):25,50,41,\\\n"
                    "  54,48,25,00\n"
                    "\"Wine11c\"=hex(2):25,50,41,\\;comment\n"
                    "  54,48,\\\n"
                    "  25,00\n"
                    "\"Wine11d\"=hex(2):25,50,41,\\;comment\n"
                    "  54,48,\n"
                    "  25,00\n"
                    "\"Wine11e\"=hex(2):25,50,41,\\;comment\n"
                    "  54,48,;comment\n"
                    "  25,00\n");
    verify_reg(hkey, "Wine11a", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine11b", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine11c", REG_EXPAND_SZ, "%PATH%", 7, 0);
    /* Wine11d */
    size = sizeof(buffer);
    lr = RegQueryValueExA(hkey, "Wine11d", NULL, &type, (BYTE *)&buffer, &size);
    ok(lr == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", lr);
    ok(type == REG_EXPAND_SZ, "got wrong type %u, expected %u\n", type, REG_EXPAND_SZ);
    ok(size == 6 || broken(size == 5) /* Win2k */, "got wrong size %u, expected 6\n", size);
    ok(memcmp(buffer, "%PATH", size) == 0, "got wrong data\n");
    /* Wine11e */
    size = sizeof(buffer);
    memset(buffer, '-', size);
    lr = RegQueryValueExA(hkey, "Wine11e", NULL, &type, (BYTE *)&buffer, &size);
    ok(lr == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", lr);
    ok(type == REG_EXPAND_SZ, "got wrong type %u, expected %u\n", type, REG_EXPAND_SZ);
    ok(size == 6 || broken(size == 5) /* Win2k */, "got wrong size %u, expected 6\n", size);
    ok(memcmp(buffer, "%PATH", size) == 0, "got wrong data\n");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine12a\"=hex:11,22,33,44,55,66,77,88\n"
                    "\"Wine12b\"=hex:11,22,33,44,\\\n"
                    "  55,66,77,88\n"
                    "\"Wine12c\"=hex:11,22,33,44,\\;comment\n"
                    "  55,66,\\\n"
                    "  77,88\n"
                    "\"Wine12d\"=hex:11,22,33,44,\\;comment\n"
                    "  55,66,\n"
                    "  77,88\n"
                    "\"Wine12e\"=hex:11,22,33,44,\\;comment\n"
                    "  55,66,;comment\n"
                    "  77,88\n");
    hex[0] = 0x11; hex[1] = 0x22; hex[2] = 0x33; hex[3] = 0x44;
    hex[4] = 0x55; hex[5] = 0x66; hex[6] = 0x77; hex[7] = 0x88;
    verify_reg(hkey, "Wine12a", REG_BINARY, hex, sizeof(hex), 0);
    verify_reg(hkey, "Wine12b", REG_BINARY, hex, sizeof(hex), 0);
    verify_reg(hkey, "Wine12c", REG_BINARY, hex, sizeof(hex), 0);
    verify_reg(hkey, "Wine12d", REG_BINARY, hex, 6, 0);
    verify_reg(hkey, "Wine12e", REG_BINARY, hex, 6, 0);

    /* Test import with subkeys */
    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey\"1]\n"
                    "\"Wine\\\\31\"=\"Test value\"\n\n");
    lr = RegOpenKeyExA(hkey, "Subkey\"1", 0, KEY_READ, &subkey);
    ok(lr == ERROR_SUCCESS, "got %d, expected 0\n", lr);
    verify_reg(subkey, "Wine\\31", REG_SZ, "Test value", 11, 0);
    lr = RegCloseKey(subkey);
    ok(lr == ERROR_SUCCESS, "got %d, expected 0\n", lr);
    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE "\\Subkey\"1");
    ok(lr == ERROR_SUCCESS, "got %d, expected 0\n", lr);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey/2]\n"
                    "\"123/\\\"4;'5\"=\"Random value name\"\n\n");
    lr = RegOpenKeyExA(hkey, "Subkey/2", 0, KEY_READ, &subkey);
    ok(lr == ERROR_SUCCESS, "got %d, expected 0\n", lr);
    verify_reg(subkey, "123/\"4;'5", REG_SZ, "Random value name", 18, 0);
    lr = RegCloseKey(subkey);
    ok(lr == ERROR_SUCCESS, "got %d, expected 0\n", lr);
    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE "\\Subkey/2");
    ok(lr == ERROR_SUCCESS, "got %d, expected 0\n", lr);

    /* Test the accepted range of the hex-based data types */
    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine13a\"=hex(0):56,61,6c,75,65,00\n"
                    "\"Wine13b\"=hex(10):56,61,6c,75,65,00\n"
                    "\"Wine13c\"=hex(100):56,61,6c,75,65,00\n"
                    "\"Wine13d\"=hex(1000):56,61,6c,75,65,00\n"
                    "\"Wine13e\"=hex(7fff):56,61,6c,75,65,00\n"
                    "\"Wine13f\"=hex(ffff):56,61,6c,75,65,00\n"
                    "\"Wine13g\"=hex(7fffffff):56,61,6c,75,65,00\n"
                    "\"Wine13h\"=hex(ffffffff):56,61,6c,75,65,00\n"
                    "\"Wine13i\"=hex(100000000):56,61,6c,75,65,00\n"
                    "\"Wine13j\"=hex(0x2):56,61,6c,75,65,00\n"
                    "\"Wine13k\"=hex(0X2):56,61,6c,75,65,00\n"
                    "\"Wine13l\"=hex(x2):56,61,6c,75,65,00\n\n");
    verify_reg(hkey, "Wine13a", REG_NONE, "Value", 6, 0);
    verify_reg(hkey, "Wine13b", 0x10, "Value", 6, 0);
    verify_reg(hkey, "Wine13c", 0x100, "Value", 6, 0);
    verify_reg(hkey, "Wine13d", 0x1000, "Value", 6, 0);
    verify_reg(hkey, "Wine13e", 0x7fff, "Value", 6, 0);
    verify_reg(hkey, "Wine13f", 0xffff, "Value", 6, 0);
    verify_reg(hkey, "Wine13g", 0x7fffffff, "Value", 6, 0);
    verify_reg(hkey, "Wine13h", 0xffffffff, "Value", 6, 0);
    verify_reg_nonexist(hkey, "Wine13i");
    verify_reg_nonexist(hkey, "Wine13j");
    verify_reg_nonexist(hkey, "Wine13k");
    verify_reg_nonexist(hkey, "Wine13l");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine14a\"=hex(7):4c,69,6e,65,20,  \\\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine14b\"=hex(7):4c,69,6e,65,20,\t\\\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n");
    verify_reg(hkey, "Wine14a", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine14b", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine15\"=hex(2):25,50,41,54,48,25,00,\n\n");
    verify_reg(hkey, "Wine15", REG_EXPAND_SZ, "%PATH%", 7, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine16\"=hex(2):\\\n"
                    "  25,48,4f,4d,45,25,00\n\n");
    verify_reg(hkey, "Wine16", REG_EXPAND_SZ, "%HOME%", 7, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine17a\"=hex(0):56,61,6c,75,65,\\");
    verify_reg(hkey, "Wine17a", REG_NONE, "Value", 5, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine17b\"=hex(2):25,50,41,54,48,25,\\");
    verify_reg(hkey, "Wine17b", REG_EXPAND_SZ, "%PATH%", 7, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine17c\"=hex:11,22,33,44,55,\\");
    verify_reg(hkey, "Wine17c", REG_BINARY, hex, 5, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine17d\"=hex(7):4c,69,6e,65,\\");
    verify_reg(hkey, "Wine17d", REG_MULTI_SZ, "Line", 5, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine17e\"=hex(100):56,61,6c,75,65,\\");
    verify_reg(hkey, "Wine17e", 0x100, "Value", 5, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine18a\"=hex(7):4c,69,6e,65,00,00\n"
                    "\"Wine18b\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine18c\"=hex(7):4c,69,6e,65,20,\\;comment\n"
                    "  63,6f,6e,63,61,74,\\\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine18d\"=hex(7):4c,69,6e,65,20,\\;comment\n"
                    "  63,6f,6e,63,61,74,\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine18e\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,;comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n");
    verify_reg(hkey, "Wine18a", REG_MULTI_SZ, "Line\0", 6, 0);
    verify_reg(hkey, "Wine18b", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine18c", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    /* Wine18d */
    size = sizeof(buffer);
    memset(buffer, '-', size);
    lr = RegQueryValueExA(hkey, "Wine18d", NULL, &type, (BYTE *)&buffer, &size);
    ok(lr == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", lr);
    ok(type == REG_MULTI_SZ, "got wrong type %u, expected %u\n", type, REG_MULTI_SZ);
    ok(size == 12 || broken(size == 11) /* Win2k */, "got wrong size %u, expected 12\n", size);
    ok(memcmp(buffer, "Line concat", size) == 0, "got wrong data\n");
    /* Wine18e */
    size = sizeof(buffer);
    memset(buffer, '-', size);
    lr = RegQueryValueExA(hkey, "Wine18e", NULL, &type, (BYTE *)&buffer, &size);
    ok(lr == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", lr);
    ok(type == REG_MULTI_SZ, "got wrong type %u, expected %u\n", type, REG_MULTI_SZ);
    ok(size == 12 || broken(size == 11) /* Win2k */, "got wrong size %u, expected 12\n", size);
    ok(memcmp(buffer, "Line concat", size) == 0, "got wrong data\n");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine19a\"=hex(100):25,50,41,54,48,25,00\n"
                    "\"Wine19b\"=hex(100):25,50,41,\\\n"
                    "  54,48,25,00\n"
                    "\"Wine19c\"=hex(100):25,50,41,\\;comment\n"
                    "  54,48,\\\n"
                    "  25,00\n"
                    "\"Wine19d\"=hex(100):25,50,41,\\;comment\n"
                    "  54,48,\n"
                    "  25,00\n"
                    "\"Wine19e\"=hex(100):25,50,41,\\;comment\n"
                    "  54,48,;comment\n"
                    "  25,00\n");
    verify_reg(hkey, "Wine19a", 0x100, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine19b", 0x100, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine19c", 0x100, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine19d", 0x100, "%PATH", 5, 0);
    verify_reg(hkey, "Wine19e", 0x100, "%PATH", 5, 0);

    /* Test null-termination of REG_EXPAND_SZ and REG_MULTI_SZ data*/
    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine20a\"=hex(7):4c,69,6e,65\n"
                    "\"Wine20b\"=hex(7):4c,69,6e,65,\n"
                    "\"Wine20c\"=hex(7):4c,69,6e,65,00\n"
                    "\"Wine20d\"=hex(7):4c,69,6e,65,00,\n"
                    "\"Wine20e\"=hex(7):4c,69,6e,65,00,00\n"
                    "\"Wine20f\"=hex(7):4c,69,6e,65,00,00,\n\n");
    verify_reg(hkey, "Wine20a", REG_MULTI_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine20b", REG_MULTI_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine20c", REG_MULTI_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine20d", REG_MULTI_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine20e", REG_MULTI_SZ, "Line\0", 6, 0);
    verify_reg(hkey, "Wine20f", REG_MULTI_SZ, "Line\0", 6, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine21a\"=hex(2):25,50,41,54,48,25\n"
                    "\"Wine21b\"=hex(2):25,50,41,54,48,25,\n"
                    "\"Wine21c\"=hex(2):25,50,41,54,48,25,00\n"
                    "\"Wine21d\"=hex(2):25,50,41,54,48,25,00,\n\n");
    verify_reg(hkey, "Wine21a", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine21b", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine21c", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine21d", REG_EXPAND_SZ, "%PATH%", 7, 0);

    RegCloseKey(hkey);

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: %d\n", lr);
}

static void test_basic_import_unicode(void)
{
    HKEY hkey, subkey;
    DWORD dword = 0x17, type, size;
    char exp_binary[] = {0xAA,0xBB,0xCC,0x11};
    LONG lr;
    char buffer[256];
    BYTE hex[8];

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND, "RegDeleteKeyA failed: %d\n", lr);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"TestValue\"=\"AValue\"\n");
    lr = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: %d\n", lr);
    verify_reg(hkey, "TestValue", REG_SZ, "AValue", 7, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\r\n\r\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\r\n"
                     "\"TestValue2\"=\"BValue\"\r\n");
    verify_reg(hkey, "TestValue2", REG_SZ, "BValue", 7, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"TestValue3\"=\"Value\"\n");
    verify_reg(hkey, "TestValue3", REG_SZ, "Value", 6, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\r\r"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\r"
                     "\"TestValue4\"=\"DValue\"\r");
    verify_reg(hkey, "TestValue4", REG_SZ, "DValue", 7, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\r\r"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\r"
                     "\"TestValue5\"=\"Value\"\r");
    verify_reg(hkey, "TestValue5", REG_SZ, "Value", 6, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"TestDword\"=dword:00000017\n");
    verify_reg(hkey, "TestDword", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"TestBinary\"=hex:aa,bb,cc,11\n");
    verify_reg(hkey, "TestBinary", REG_BINARY, exp_binary, sizeof(exp_binary), 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"With=Equals\"=\"asdf\"\n");
    verify_reg(hkey, "With=Equals", REG_SZ, "asdf", 5, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Empty string\"=\"\"\n"
                     "\"\"=\"Default registry value\"\n\n");
    verify_reg(hkey, "Empty string", REG_SZ, "", 1, 0);
    verify_reg(hkey, NULL, REG_SZ, "Default registry value", 23, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Line1\"=\"Value1\"\n\n"
                     "\"Line2\"=\"Value2\"\n\n\n"
                     "\"Line3\"=\"Value3\"\n\n\n\n"
                     "\"Line4\"=\"Value4\"\n\n");
    verify_reg(hkey, "Line1", REG_SZ, "Value1", 7, 0);
    verify_reg(hkey, "Line2", REG_SZ, "Value2", 7, 0);
    verify_reg(hkey, "Line3", REG_SZ, "Value3", 7, 0);
    verify_reg(hkey, "Line4", REG_SZ, "Value4", 7, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine1\"=dword:00000782\n\n"
                     "\"Wine2\"=\"Test Value\"\n"
                     "\"Wine3\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "#comment\n"
                     "@=\"Test\"\n"
                     ";comment\n\n"
                     "\"Wine4\"=dword:12345678\n\n");
    dword = 0x782;
    verify_reg(hkey, "Wine1", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine2", REG_SZ, "Test Value", 11, 0);
    verify_reg(hkey, "Wine3", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "", REG_SZ, "Test", 5, 0);
    dword = 0x12345678;
    verify_reg(hkey, "Wine4", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine5\"=\"No newline\"");
    verify_reg(hkey, "Wine5", REG_SZ, "No newline", 11, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine6\"=dword:00000050\n\n"
                     "\"Wine7\"=\"No newline\"");
    dword = 0x50;
    verify_reg(hkey, "Wine6", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine7", REG_SZ, "No newline", 11, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine8a\"=dword:1\n"
                     "\"Wine8b\"=dword:4444\n\n");
    dword = 0x1;
    verify_reg(hkey, "Wine8a", REG_DWORD, &dword, sizeof(dword), 0);
    dword = 0x4444;
    verify_reg(hkey, "Wine8b", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine9a\"=hex(2):4c,00,69,00,6e,00,65,00,00,00\n"
                     "\"Wine9b\"=\"Value 1\"\n"
                     "\"Wine9c\"=hex(2):4c,00,69,00,6e,00,65,00\\\n"
                     "\"Wine9d\"=\"Value 2\"\n"
                     "\"Wine9e\"=hex(2):4c,00,69,00,6e,00,65,00,\\\n"
                     "\"Wine9f\"=\"Value 3\"\n"
                     "\"Wine9g\"=\"Value 4\"\n\n");
    verify_reg(hkey, "Wine9a", REG_EXPAND_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine9b", REG_SZ, "Value 1", 8, 0);
    verify_reg_nonexist(hkey, "Wine9c");
    verify_reg(hkey, "Wine9d", REG_SZ, "Value 2", 8, 0);
    verify_reg_nonexist(hkey, "Wine9e");
    verify_reg_nonexist(hkey, "Wine9f");
    verify_reg(hkey, "Wine9g", REG_SZ, "Value 4", 8, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"double\\\"quote\"=\"valid \\\"or\\\" not\"\n"
                     "\"single'quote\"=dword:00000008\n\n");
    verify_reg(hkey, "double\"quote", REG_SZ, "valid \"or\" not", 15, 0);
    dword = 0x00000008;
    verify_reg(hkey, "single'quote", REG_DWORD, &dword, sizeof(dword), 0);

    /* Test hex data concatenation for REG_NONE, REG_EXPAND_SZ and REG_BINARY */
    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine10a\"=hex(0):56,00,61,00,6c,00,75,00,65,00,00,00\n"
                     "\"Wine10b\"=hex(0):56,00,61,00,6c,00,\\\n"
                     "  75,00,65,00,00,00\n"
                     "\"Wine10c\"=hex(0):56,00,61,00,\\;comment\n"
                     "  6c,00,75,00,\\\n"
                     "  65,00,00,00\n"
                     "\"Wine10d\"=hex(0):56,00,61,00,\\;comment\n"
                     "  6c,00,75,00,\n"
                     "  65,00,00,00\n"
                     "\"Wine10e\"=hex(0):56,00,61,00,\\;comment\n"
                     "  6c,00,75,00,;comment\n"
                     "  65,00,00,00\n");
    verify_reg(hkey, "Wine10a", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    verify_reg(hkey, "Wine10b", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    verify_reg(hkey, "Wine10c", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    verify_reg(hkey, "Wine10d", REG_NONE, "V\0a\0l\0u", 8, 0);
    verify_reg(hkey, "Wine10e", REG_NONE, "V\0a\0l\0u", 8, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine11a\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine11b\"=hex(2):25,00,50,00,41,00,\\\n"
                     "  54,00,48,00,25,00,00,00\n"
                     "\"Wine11c\"=hex(2):25,00,50,00,41,00,\\;comment\n"
                     "  54,00,48,00,\\\n"
                     "  25,00,00,00\n"
                     "\"Wine11d\"=hex(2):25,00,50,00,41,00,\\;comment\n"
                     "  54,00,48,00,\n"
                     "  25,00,00,00\n"
                     "\"Wine11e\"=hex(2):25,00,50,00,41,00,\\;comment\n"
                     "  54,00,48,00,;comment\n"
                     "  25,00,00,00\n");
    verify_reg(hkey, "Wine11a", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine11b", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine11c", REG_EXPAND_SZ, "%PATH%", 7, 0);
    /* Wine11d */
    size = sizeof(buffer);
    lr = RegQueryValueExA(hkey, "Wine11d", NULL, &type, (BYTE *)&buffer, &size);
    ok(lr == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", lr);
    ok(type == REG_EXPAND_SZ, "got wrong type %u, expected %u\n", type, REG_EXPAND_SZ);
    ok(size == 6 || broken(size == 5) /* Win2k */, "got wrong size %u, expected 6\n", size);
    ok(memcmp(buffer, "%PATH", size) == 0, "got wrong data\n");
    /* Wine11e */
    size = sizeof(buffer);
    memset(buffer, '-', size);
    lr = RegQueryValueExA(hkey, "Wine11e", NULL, &type, (BYTE *)&buffer, &size);
    ok(lr == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", lr);
    ok(type == REG_EXPAND_SZ, "got wrong type %u, expected %u\n", type, REG_EXPAND_SZ);
    ok(size == 6 || broken(size == 5) /* Win2k */, "got wrong size %u, expected 6\n", size);
    ok(memcmp(buffer, "%PATH", size) == 0, "got wrong data\n");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine12a\"=hex:11,22,33,44,55,66,77,88\n"
                     "\"Wine12b\"=hex:11,22,33,44,\\\n"
                     "  55,66,77,88\n"
                     "\"Wine12c\"=hex:11,22,33,44,\\;comment\n"
                     "  55,66,\\\n"
                     "  77,88\n"
                     "\"Wine12d\"=hex:11,22,33,44,\\;comment\n"
                     "  55,66,\n"
                     "  77,88\n"
                     "\"Wine12e\"=hex:11,22,33,44,\\;comment\n"
                     "  55,66,;comment\n"
                     "  77,88\n");
    hex[0] = 0x11; hex[1] = 0x22; hex[2] = 0x33; hex[3] = 0x44;
    hex[4] = 0x55; hex[5] = 0x66; hex[6] = 0x77; hex[7] = 0x88;
    verify_reg(hkey, "Wine12a", REG_BINARY, hex, sizeof(hex), 0);
    verify_reg(hkey, "Wine12b", REG_BINARY, hex, sizeof(hex), 0);
    verify_reg(hkey, "Wine12c", REG_BINARY, hex, sizeof(hex), 0);
    verify_reg(hkey, "Wine12d", REG_BINARY, hex, 6, 0);
    verify_reg(hkey, "Wine12e", REG_BINARY, hex, 6, 0);

    /* Test import with subkeys */
    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey\"1]\n"
                     "\"Wine\\\\31\"=\"Test value\"\n\n");
    lr = RegOpenKeyExA(hkey, "Subkey\"1", 0, KEY_READ, &subkey);
    ok(lr == ERROR_SUCCESS, "got %d, expected 0\n", lr);
    verify_reg(subkey, "Wine\\31", REG_SZ, "Test value", 11, 0);
    lr = RegCloseKey(subkey);
    ok(lr == ERROR_SUCCESS, "got %d, expected 0\n", lr);
    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE "\\Subkey\"1");
    ok(lr == ERROR_SUCCESS, "got %d, expected 0\n", lr);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey/2]\n"
                     "\"123/\\\"4;'5\"=\"Random value name\"\n\n");
    lr = RegOpenKeyExA(hkey, "Subkey/2", 0, KEY_READ, &subkey);
    ok(lr == ERROR_SUCCESS, "got %d, expected 0\n", lr);
    verify_reg(subkey, "123/\"4;'5", REG_SZ, "Random value name", 18, 0);
    lr = RegCloseKey(subkey);
    ok(lr == ERROR_SUCCESS, "got %d, expected 0\n", lr);
    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE "\\Subkey/2");
    ok(lr == ERROR_SUCCESS, "got %d, expected 0\n", lr);

    /* Test the accepted range of the hex-based data types */
    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine13a\"=hex(0):56,61,6c,75,65,00\n"
                     "\"Wine13b\"=hex(10):56,61,6c,75,65,00\n"
                     "\"Wine13c\"=hex(100):56,61,6c,75,65,00\n"
                     "\"Wine13d\"=hex(1000):56,61,6c,75,65,00\n"
                     "\"Wine13e\"=hex(7fff):56,61,6c,75,65,00\n"
                     "\"Wine13f\"=hex(ffff):56,61,6c,75,65,00\n"
                     "\"Wine13g\"=hex(7fffffff):56,61,6c,75,65,00\n"
                     "\"Wine13h\"=hex(ffffffff):56,61,6c,75,65,00\n"
                     "\"Wine13i\"=hex(100000000):56,61,6c,75,65,00\n"
                     "\"Wine13j\"=hex(0x2):56,61,6c,75,65,00\n"
                     "\"Wine13k\"=hex(0X2):56,61,6c,75,65,00\n"
                     "\"Wine13l\"=hex(x2):56,61,6c,75,65,00\n\n");
    verify_reg(hkey, "Wine13a", REG_NONE, "Value", 6, 0);
    verify_reg(hkey, "Wine13b", 0x10, "Value", 6, 0);
    verify_reg(hkey, "Wine13c", 0x100, "Value", 6, 0);
    verify_reg(hkey, "Wine13d", 0x1000, "Value", 6, 0);
    verify_reg(hkey, "Wine13e", 0x7fff, "Value", 6, 0);
    verify_reg(hkey, "Wine13f", 0xffff, "Value", 6, 0);
    verify_reg(hkey, "Wine13g", 0x7fffffff, "Value", 6, 0);
    verify_reg(hkey, "Wine13h", 0xffffffff, "Value", 6, 0);
    verify_reg_nonexist(hkey, "Wine13i");
    verify_reg_nonexist(hkey, "Wine13j");
    verify_reg_nonexist(hkey, "Wine13k");
    verify_reg_nonexist(hkey, "Wine13l");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine14a\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,  \\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "\"Wine14b\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\t\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg(hkey, "Wine14a", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine14b", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine15\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00,\n\n");
    verify_reg(hkey, "Wine15", REG_EXPAND_SZ, "%PATH%", 7, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine16\"=hex(2):\\\n"
                     "  25,00,48,00,4f,00,4d,00,45,00,25,00,00,00\n\n");
    verify_reg(hkey, "Wine16", REG_EXPAND_SZ, "%HOME%", 7, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine17a\"=hex(0):56,61,6c,75,65,\\");
    verify_reg(hkey, "Wine17a", REG_NONE, "Value", 5, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine17b\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,\\");
    verify_reg(hkey, "Wine17b", REG_EXPAND_SZ, "%PATH%", 7, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine17c\"=hex:11,22,33,44,55,\\");
    verify_reg(hkey, "Wine17c", REG_BINARY, hex, 5, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine17d\"=hex(7):4c,00,69,00,6e,00,65,00,\\");
    verify_reg(hkey, "Wine17d", REG_MULTI_SZ, "Line", 5, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine17e\"=hex(100):56,61,6c,75,65,\\");
    verify_reg(hkey, "Wine17e", 0x100, "Value", 5, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine18a\"=hex(7):4c,00,69,00,6e,00,65,00,00,00,00,00\n"
                     "\"Wine18b\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "\"Wine18c\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\;comment\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "\"Wine18d\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\;comment\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "\"Wine18e\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,;comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg(hkey, "Wine18a", REG_MULTI_SZ, "Line\0", 6, 0);
    verify_reg(hkey, "Wine18b", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine18c", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    /* Wine18d */
    size = sizeof(buffer);
    memset(buffer, '-', size);
    lr = RegQueryValueExA(hkey, "Wine18d", NULL, &type, (BYTE *)&buffer, &size);
    ok(lr == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", lr);
    ok(type == REG_MULTI_SZ, "got wrong type %u, expected %u\n", type, REG_MULTI_SZ);
    ok(size == 12 || broken(size == 11) /* Win2k */, "got wrong size %u, expected 12\n", size);
    ok(memcmp(buffer, "Line concat", size) == 0, "got wrong data\n");
    /* Wine18e */
    size = sizeof(buffer);
    memset(buffer, '-', size);
    lr = RegQueryValueExA(hkey, "Wine18e", NULL, &type, (BYTE *)&buffer, &size);
    ok(lr == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", lr);
    ok(type == REG_MULTI_SZ, "got wrong type %u, expected %u\n", type, REG_MULTI_SZ);
    ok(size == 12 || broken(size == 11) /* Win2k */, "got wrong size %u, expected 12\n", size);
    ok(memcmp(buffer, "Line concat", size) == 0, "got wrong data\n");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine19a\"=hex(100):25,50,41,54,48,25,00\n"
                     "\"Wine19b\"=hex(100):25,50,41,\\\n"
                     "  54,48,25,00\n"
                     "\"Wine19c\"=hex(100):25,50,41,\\;comment\n"
                     "  54,48,\\\n"
                     "  25,00\n"
                     "\"Wine19d\"=hex(100):25,50,41,\\;comment\n"
                     "  54,48,\n"
                     "  25,00\n"
                     "\"Wine19e\"=hex(100):25,50,41,\\;comment\n"
                     "  54,48,;comment\n"
                     "  25,00\n");
    verify_reg(hkey, "Wine19a", 0x100, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine19b", 0x100, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine19c", 0x100, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine19d", 0x100, "%PATH", 5, 0);
    verify_reg(hkey, "Wine19e", 0x100, "%PATH", 5, 0);

    /* Test null-termination of REG_EXPAND_SZ and REG_MULTI_SZ data*/
    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine20a\"=hex(7):4c,00,69,00,6e,00,65,00\n"
                     "\"Wine20b\"=hex(7):4c,00,69,00,6e,00,65,00,\n"
                     "\"Wine20c\"=hex(7):4c,00,69,00,6e,00,65,00,00,00\n"
                     "\"Wine20d\"=hex(7):4c,00,69,00,6e,00,65,00,00,00,\n"
                     "\"Wine20e\"=hex(7):4c,00,69,00,6e,00,65,00,00,00,00,00\n"
                     "\"Wine20f\"=hex(7):4c,00,69,00,6e,00,65,00,00,00,00,00,\n\n");
    verify_reg(hkey, "Wine20a", REG_MULTI_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine20b", REG_MULTI_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine20c", REG_MULTI_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine20d", REG_MULTI_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine20e", REG_MULTI_SZ, "Line\0", 6, 0);
    verify_reg(hkey, "Wine20f", REG_MULTI_SZ, "Line\0", 6, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine21a\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00\n"
                     "\"Wine21b\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,\n"
                     "\"Wine21c\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine21d\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00,\n\n");
    verify_reg(hkey, "Wine21a", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine21b", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine21c", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine21d", REG_EXPAND_SZ, "%PATH%", 7, 0);

    RegCloseKey(hkey);

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: %d\n", lr);
}

static void test_basic_import_31(void)
{
    HKEY hkey;
    LONG lr;

    lr = RegDeleteKeyA(HKEY_CLASSES_ROOT, KEY_BASE);
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND,
            "RegDeleteKeyA failed: %d\n", lr);

    /* Check if regedit.exe is running with elevated privileges */
    lr = RegCreateKeyExA(HKEY_CLASSES_ROOT, KEY_BASE, 0, NULL, REG_OPTION_NON_VOLATILE,
                         KEY_READ, NULL, &hkey, NULL);
    if (lr == ERROR_ACCESS_DENIED)
    {
        win_skip("regedit.exe is not running with elevated privileges; "
                 "skipping Windows 3.1 import tests\n");
        return;
    }

    /* Test simple value */
    exec_import_str("REGEDIT\r\n"
		    "HKEY_CLASSES_ROOT\\" KEY_BASE " = Value0\r\n");
    verify_reg(hkey, "", REG_SZ, "Value0", 7, 0);

    /* Test proper handling of spaces and equals signs */
    exec_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE " =Value1\r\n");
    verify_reg(hkey, "", REG_SZ, "Value1", 7, 0);

    exec_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE " =  Value2\r\n");
    verify_reg(hkey, "", REG_SZ, " Value2", 8, 0);

    exec_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE " = Value3 \r\n");
    verify_reg(hkey, "", REG_SZ, "Value3 ", 8, 0);

    exec_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE " Value4\r\n");
    verify_reg(hkey, "", REG_SZ, "Value4", 7, 0);

    exec_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE "  Value5\r\n");
    verify_reg(hkey, "", REG_SZ, "Value5", 7, 0);

    exec_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE "\r\n");
    verify_reg(hkey, "", REG_SZ, "", 1, 0);

    exec_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE "  \r\n");
    verify_reg(hkey, "", REG_SZ, "", 1, 0);

    exec_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE " = No newline");
    verify_reg(hkey, "", REG_SZ, "No newline", 11, 0);

    RegCloseKey(hkey);

    lr = RegDeleteKeyA(HKEY_CLASSES_ROOT, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: %d\n", lr);
}

static void test_invalid_import(void)
{
    LONG lr;
    HKEY hkey;
    DWORD dword = 0x8;

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND, "RegDeleteKeyA failed: %d\n", lr);

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"TestNoEndQuote\"=\"Asdffdsa\n");
    lr = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ|KEY_SET_VALUE, &hkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: %d\n", lr);
    verify_reg_nonexist(hkey, "TestNoEndQuote");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"TestNoBeginQuote\"=Asdffdsa\"\n");
    verify_reg_nonexist(hkey, "TestNoBeginQuote");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"TestNoQuotes\"=Asdffdsa\n");
    verify_reg_nonexist(hkey, "TestNoQuotes");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"NameNoEndQuote=\"Asdffdsa\"\n");
    verify_reg_nonexist(hkey, "NameNoEndQuote");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "NameNoBeginQuote\"=\"Asdffdsa\"\n");
    verify_reg_nonexist(hkey, "NameNoBeginQuote");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "NameNoQuotes=\"Asdffdsa\"\n");
    verify_reg_nonexist(hkey, "NameNoQuotes");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"MixedQuotes=Asdffdsa\"\n");
    verify_reg_nonexist(hkey, "MixedQuotes");
    verify_reg_nonexist(hkey, "MixedQuotes=Asdffdsa");

    /* Test import with non-standard registry file headers */
    exec_import_str("REGEDIT3\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test1\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test1");

    exec_import_str("regedit4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test2\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test2");

    exec_import_str("Regedit4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test3\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test3");

    exec_import_str("REGEDIT 4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test4\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test4");

    exec_import_str("REGEDIT4FOO\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test5\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test5");

    exec_import_str("REGEDIT4 FOO\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test6\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test6");

    exec_import_str("REGEDIT5\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test7\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test7");

    exec_import_str("Windows Registry Editor Version 4.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test8\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test8");

    exec_import_str("Windows Registry Editor Version 5\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test9\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test9");

    exec_import_str("WINDOWS REGISTRY EDITOR VERSION 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test10\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test10");

    exec_import_str("Windows Registry Editor version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test11\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test11");

    /* Test multi-line import with incorrect comma placement */
    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line1\"=hex(7):4c,69,6e,65,20\\\n"
                    ",63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n");
    verify_reg_nonexist(hkey, "Multi-Line1");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line2\"=hex(7):4c,69,6e,65,20\\\n"
                    "  ,63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n");
    verify_reg_nonexist(hkey, "Multi-Line2");

    exec_import_str("Windows Registry Editor Version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line3\"=hex(7):4c,69,6e,65,20\\\n"
                    ",63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n");
    verify_reg_nonexist(hkey, "Multi-Line3");

    exec_import_str("Windows Registry Editor Version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line4\"=hex(7):4c,69,6e,65,20\\\n"
                    "  ,63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n");
    verify_reg_nonexist(hkey, "Multi-Line4");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test12a\"=dword:\n"
                    "\"Test12b\"=dword:hello\n"
                    "\"Test12c\"=dword:123456789\n"
                    "\"Test12d\"=dword:012345678\n"
                    "\"Test12e\"=dword:000000001\n\n");
    verify_reg_nonexist(hkey, "Test12a");
    verify_reg_nonexist(hkey, "Test12b");
    verify_reg_nonexist(hkey, "Test12c");
    verify_reg_nonexist(hkey, "Test12d");
    verify_reg_nonexist(hkey, "Test12e");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test13a\"=dword:12345678abc\n"
                    "\"Test13b\"=dword:12345678 abc\n\n");
    verify_reg_nonexist(hkey, "Test13a");
    verify_reg_nonexist(hkey, "Test13b");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test14a\"=dword:0x123\n"
                    "\"Test14b\"=dword:123 456\n"
                    "\"Test14c\"=dword:1234 5678\n\n");
    verify_reg_nonexist(hkey, "Test14a");
    verify_reg_nonexist(hkey, "Test14b");
    verify_reg_nonexist(hkey, "Test14c");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test15a\"=\"foo\"bar\"\n"
                    "\"Test15b\"=\"foo\"\"bar\"\n\n");
    verify_reg_nonexist(hkey, "Test15a");
    verify_reg_nonexist(hkey, "Test15b");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test16a\"=\n"
                    "\"Test16b\"=\\\"\n"
                    "\"Test16c\"=\\\"Value\\\"\n"
                    "\"Test16d\"=\\\"Value\"\n\n");
    verify_reg_nonexist(hkey, "Test16a");
    verify_reg_nonexist(hkey, "Test16b");
    verify_reg_nonexist(hkey, "Test16c");
    verify_reg_nonexist(hkey, "Test16d");

    /* Test key name and value name concatenation */
    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\\n"
                    "Subkey1]\n");
    verify_key_nonexist(hkey, "Subkey1");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\n"
                    "\\Subkey2]\n");
    verify_key_nonexist(hkey, "Subkey2");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test\\\n"
                    "17a\"=\"Value 1\"\n"
                    "\"Test17b\"=\"Value 2\"\n"
                    "\"Test\n"
                    "\\17c\"=\"Value 3\"\n\n");
    verify_reg_nonexist(hkey, "Test17a");
    verify_reg(hkey, "Test17b", REG_SZ, "Value 2", 8, 0);
    verify_reg_nonexist(hkey, "Test17c");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test18a\"=dword:1234\\\n"
                    "5678\n"
                    "\"Test18b\"=\"Test \\\n"
                    "Value\"\n\n");
    verify_reg_nonexist(hkey, "Test18a");
    verify_reg_nonexist(hkey, "Test18b");

    /* Test hex data concatenation for REG_NONE, REG_EXPAND_SZ and REG_BINARY */
    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test19a\"=hex(0):56,00,61,00,6c,00\\\n"
                    ",75,00,65,00,00,00\n"
                    "\"Test19b\"=hex(0):56,00,61,00,6c,00\\\n"
                    "  ,75,00,65,00,00,00\n"
                    "\"Test19c\"=hex(0):56,00,61,00,6c,00\\\n"
                    "  75,00,65,00,00,00\n"
                    "\"Test19d\"=hex(0):56,00,61,00,6c,00,7\\\n"
                    "5,00,65,00,00,00\n"
                    "\"Test19e\"=hex(0):56,00,61,00,6c,00,7\\\n"
                    "  5,00,65,00,00,00\n"
                    "\"Test19f\"=hex(0):56,00,61,00,\\;comment\n"
                    "  6c,00,75,00,\\#comment\n"
                    "  65,00,00,00\n\n");
    verify_reg_nonexist(hkey, "Test19a");
    verify_reg_nonexist(hkey, "Test19b");
    verify_reg_nonexist(hkey, "Test19c");
    verify_reg_nonexist(hkey, "Test19d");
    verify_reg_nonexist(hkey, "Test19e");
    verify_reg_nonexist(hkey, "Test19f");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test20a\"=hex(2):25,50,41\\\n"
                    ",54,48,25,00\n"
                    "\"Test20b\"=hex(2):25,50,41\\\n"
                    "  ,54,48,25,00\n"
                    "\"Test20c\"=hex(2):25,50,41\\\n"
                    "  54,48,25,00\n"
                    "\"Test20d\"=hex(2):25,50,4\\\n"
                    "1,54,48,25,00\n"
                    "\"Test20e\"=hex(2):25,50,4\\\n"
                    "  1,54,48,25,00\n"
                    "\"Test20f\"=hex(2):25,50,41,\\;comment\n"
                    "  54,48,\\#comment\n"
                    "  25,00\n\n");
    verify_reg_nonexist(hkey, "Test20a");
    verify_reg_nonexist(hkey, "Test20b");
    verify_reg_nonexist(hkey, "Test20c");
    verify_reg_nonexist(hkey, "Test20d");
    verify_reg_nonexist(hkey, "Test20e");
    verify_reg_nonexist(hkey, "Test20f");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test21a\"=hex:11,22,33,44\\\n"
                    ",55,66,77,88\n"
                    "\"Test21b\"=hex:11,22,33,44\\\n"
                    "  ,55,66,77,88\n"
                    "\"Test21c\"=hex:11,22,33,44\\\n"
                    "  55,66,77,88\n"
                    "\"Test21d\"=hex:11,22,33,4\\\n"
                    "4,55,66,77,88\n"
                    "\"Test21e\"=hex:11,22,33,4\\\n"
                    "  4,55,66,77,88\n"
                    "\"Test21f\"=hex:11,22,33,\\;comment\n"
                    "  44,55,66,\\#comment\n"
                    "  77,88\n\n");
    verify_reg_nonexist(hkey, "Test21a");
    verify_reg_nonexist(hkey, "Test21b");
    verify_reg_nonexist(hkey, "Test21c");
    verify_reg_nonexist(hkey, "Test21d");
    verify_reg_nonexist(hkey, "Test21e");
    verify_reg_nonexist(hkey, "Test21f");

    /* Test support for characters greater than 0xff */
    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine22a\"=hex(0):25,50,100,54,48,25,00\n"
                    "\"Wine22b\"=hex(0):25,1a4,100,164,124,25,00\n\n");
    verify_reg_nonexist(hkey, "Wine22a");
    verify_reg_nonexist(hkey, "Wine22b");

    /* Test the effect of backslashes in hex data */
    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine23a\"=hex(2):25,48\\,4f,4d,45,25,00\n"
                    "\"Wine23b\"=hex(2):25,48,\\4f,4d,45,25,00\n"
                    "\"Wine23c\"=hex(2):25,48\\ ,4f,4d,45,25,00\n"
                    "\"Wine23d\"=hex(2):25,48,\\ 4f,4d,45,25,00\n"
                    "\"Wine23e\"=hex(2):\\25,48,4f,4d,45,25,00\n"
                    "\"Wine23f\"=hex(2):\\ 25,48,4f,4d,45,25,00\n"
                    "\"Wine23g\"=hex(2):25,48,4\\f,4d,45,25,00\n"
                    "\"Wine23h\"=hex(2):25,48,4\\\n"
                    "  f,4d,45,25,00\n"
                    "\"Wine23i\"=hex(2):25,50,\\,41,54,48,25,00\n"
                    "\"Wine23j\"=hex(2):25,48,4f,4d,45,25,5c,\\\\\n"
                    "  25,50,41,54,48,25,00\n"
                    "\"Wine23k\"=hex(2):,\\\n"
                    "  25,48,4f,4d,45,25,00\n\n");
    verify_reg_nonexist(hkey, "Wine23a");
    verify_reg_nonexist(hkey, "Wine23b");
    verify_reg_nonexist(hkey, "Wine23c");
    verify_reg_nonexist(hkey, "Wine23d");
    verify_reg_nonexist(hkey, "Wine23e");
    verify_reg_nonexist(hkey, "Wine23f");
    verify_reg_nonexist(hkey, "Wine23g");
    verify_reg_nonexist(hkey, "Wine23h");
    verify_reg_nonexist(hkey, "Wine23i");
    verify_reg_nonexist(hkey, "Wine23j");
    verify_reg_nonexist(hkey, "Wine23k");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine24a\"=hex(2):4c,69,6e,65,20,\\\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1]\n");
    verify_reg_nonexist(hkey, "Wine24a");
    verify_key_nonexist(hkey, "Subkey1");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine24b\"=hex(2):4c,69,6e,65,20\\\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2]\n");
    verify_reg_nonexist(hkey, "Wine24b");
    verify_key(hkey, "Subkey2");

    lr = RegDeleteKeyA(hkey, "Subkey2");
    ok(lr == ERROR_SUCCESS, "RegDeleteKey failed: %u\n", lr);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine25a\"=hex(2):4c,69,6e,65,20,\\\n"
                    "\"Wine25b\"=\"Test value\"\n"

                    "\"Wine25c\"=hex(2):4c,69,6e,65,20,\\\n"
                    ";comment\n"
                    "\"Wine25d\"=\"Test value\"\n"

                    "\"Wine25e\"=hex(2):4c,69,6e,65,20,\\\n"
                    "#comment\n"
                    "\"Wine25f\"=\"Test value\"\n"

                    "\"Wine25g\"=hex(2):4c,69,6e,65,20,\\\n\n"
                    "\"Wine25h\"=\"Test value\"\n"

                    "\"Wine25i\"=hex(2):4c,69,6e,65,20\\\n"
                    "\"Wine25j\"=\"Test value\"\n\n");
    verify_reg_nonexist(hkey, "Wine25a");
    verify_reg_nonexist(hkey, "Wine25b");
    verify_reg_nonexist(hkey, "Wine25c");
    verify_reg_nonexist(hkey, "Wine25d");
    verify_reg_nonexist(hkey, "Wine25e");
    verify_reg(hkey, "Wine25f", REG_SZ, "Test value", 11, 0);
    verify_reg_nonexist(hkey, "Wine25g");
    verify_reg_nonexist(hkey, "Wine25h");
    verify_reg_nonexist(hkey, "Wine25i");
    verify_reg(hkey, "Wine25j", REG_SZ, "Test value", 11, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine26a\"=hex(2):4c,69,6e,65,20,\\\n"
                    "\"Wine26b\"=dword:00000008\n"

                    "\"Wine26c\"=hex(2):4c,69,6e,65,20,\\\n"
                    ";comment\n"
                    "\"Wine26d\"=dword:00000008\n"

                    "\"Wine26e\"=hex(2):4c,69,6e,65,20,\\\n"
                    "#comment\n"
                    "\"Wine26f\"=dword:00000008\n"

                    "\"Wine26g\"=hex(2):4c,69,6e,65,20,\\\n\n"
                    "\"Wine26h\"=dword:00000008\n"

                    "\"Wine26i\"=hex(2):4c,69,6e,65,20\\\n"
                    "\"Wine26j\"=dword:00000008\n\n");
    verify_reg_nonexist(hkey, "Wine26a");
    verify_reg_nonexist(hkey, "Wine26b");
    verify_reg_nonexist(hkey, "Wine26c");
    verify_reg_nonexist(hkey, "Wine26d");
    verify_reg_nonexist(hkey, "Wine26e");
    verify_reg(hkey, "Wine26f", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg_nonexist(hkey, "Wine26g");
    verify_reg_nonexist(hkey, "Wine26h");
    verify_reg_nonexist(hkey, "Wine26i");
    verify_reg(hkey, "Wine26j", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine27a\"=hex(2):25,48,4f,4d,45,25,5c,\\\n"
                    "\"Wine27b\"=hex(2):25,50,41,54,48,25,00\n"

                    "\"Wine27c\"=hex(2):25,48,4f,4d,45,25,5c,\\\n"
                    ";comment\n"
                    "\"Wine27d\"=hex(2):25,50,41,54,48,25,00\n"

                    "\"Wine27e\"=hex(2):25,48,4f,4d,45,25,5c,\\\n"
                    "#comment\n"
                    "\"Wine27f\"=hex(2):25,50,41,54,48,25,00\n"

                    "\"Wine27g\"=hex(2):25,48,4f,4d,45,25,5c,\\\n\n"
                    "\"Wine27h\"=hex(2):25,50,41,54,48,25,00\n"

                    "\"Wine27i\"=hex(2):25,48,4f,4d,45,25,5c\\\n"
                    "\"Wine27j\"=hex(2):25,50,41,54,48,25,00\n\n");
    verify_reg_nonexist(hkey, "Wine27a");
    verify_reg_nonexist(hkey, "Wine27b");
    verify_reg_nonexist(hkey, "Wine27c");
    verify_reg_nonexist(hkey, "Wine27d");
    verify_reg_nonexist(hkey, "Wine27e");
    verify_reg(hkey, "Wine27f", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg_nonexist(hkey, "Wine27g");
    verify_reg_nonexist(hkey, "Wine27h");
    verify_reg_nonexist(hkey, "Wine27i");
    verify_reg(hkey, "Wine27j", REG_EXPAND_SZ, "%PATH%", 7, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine28a\"=hex(2):4c,69,6e,65,20,\\\n"
                    "@=\"Default value 1\"\n\n");
    verify_reg_nonexist(hkey, "Wine28a");
    verify_reg_nonexist(hkey, NULL);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine28b\"=hex(2):4c,69,6e,65,20,\\\n"
                    ";comment\n"
                    "@=\"Default value 2\"\n\n");
    verify_reg_nonexist(hkey, "Wine28b");
    verify_reg_nonexist(hkey, NULL);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine28c\"=hex(2):4c,69,6e,65,20,\\\n"
                    "#comment\n"
                    "@=\"Default value 3\"\n\n");
    verify_reg_nonexist(hkey, "Wine28c");
    verify_reg(hkey, NULL, REG_SZ, "Default value 3", 16, 0);

    lr = RegDeleteValueW(hkey, NULL);
    ok(lr == ERROR_SUCCESS, "RegDeleteValue failed: %u\n", lr);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine28d\"=hex(2):4c,69,6e,65,20,\\\n\n"
                    "@=\"Default value 4\"\n\n");
    verify_reg_nonexist(hkey, "Wine28d");
    verify_reg_nonexist(hkey, NULL);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine28e\"=hex(2):4c,69,6e,65,20\\\n"
                    "@=\"Default value 5\"\n\n");
    verify_reg_nonexist(hkey, "Wine28e");
    verify_reg(hkey, NULL, REG_SZ, "Default value 5", 16, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine29a\"=hex:11,22,33,\\\n"
                    "\\\n"
                    "  44,55,66\n"
                    "\"Wine29b\"=hex:11,22,33,\\\n"
                    "  \\\n"
                    "  44,55,66\n\n");
    verify_reg_nonexist(hkey, "Wine29a");
    verify_reg_nonexist(hkey, "Wine29b");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine30a\"=hex(0):25,48,4f,4d,45,25,5c,/\n"
                    "  25,50,41,54,48,25,00\n"
                    "\"Wine30b\"=hex(0):25,48,4f,4d,45,25,5c/\n"
                    "  25,50,41,54,48,25,00\n\n");
    verify_reg_nonexist(hkey, "Wine30a");
    verify_reg_nonexist(hkey, "Wine30b");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine31\"=hex(7):4c,69,6e,65,20\\");
    verify_reg_nonexist(hkey, "Wine31");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine32a\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  ,63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine32b\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n");
    verify_reg_nonexist(hkey, "Wine32a");
    verify_reg_nonexist(hkey, "Wine32b");

    RegCloseKey(hkey);

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: %d\n", lr);
}

static void test_invalid_import_unicode(void)
{
    LONG lr;
    HKEY hkey;
    DWORD dword = 0x8, os_version, major_version, minor_version;

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND, "RegDeleteKeyA failed: %d\n", lr);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"TestNoEndQuote\"=\"Asdffdsa\n");
    lr = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ|KEY_SET_VALUE, &hkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: %d\n", lr);
    verify_reg_nonexist(hkey, "TestNoEndQuote");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"TestNoBeginQuote\"=Asdffdsa\"\n");
    verify_reg_nonexist(hkey, "TestNoBeginQuote");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"TestNoQuotes\"=Asdffdsa\n");
    verify_reg_nonexist(hkey, "TestNoQuotes");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"NameNoEndQuote=\"Asdffdsa\"\n");
    verify_reg_nonexist(hkey, "NameNoEndQuote");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "NameNoBeginQuote\"=\"Asdffdsa\"\n");
    verify_reg_nonexist(hkey, "NameNoBeginQuote");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "NameNoQuotes=\"Asdffdsa\"\n");
    verify_reg_nonexist(hkey, "NameNoQuotes");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"MixedQuotes=Asdffdsa\"\n");
    verify_reg_nonexist(hkey, "MixedQuotes");
    verify_reg_nonexist(hkey, "MixedQuotes=Asdffdsa");

    /* Test import with non-standard registry file headers */
    exec_import_wstr("\xef\xbb\xbfREGEDIT3\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test1\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test1");

    exec_import_wstr("\xef\xbb\xbfregedit4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test2\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test2");

    exec_import_wstr("\xef\xbb\xbfRegedit4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test3\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test3");

    exec_import_wstr("\xef\xbb\xbfREGEDIT 4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test4\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test4");

    exec_import_wstr("\xef\xbb\xbfREGEDIT4FOO\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test5\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test5");

    exec_import_wstr("\xef\xbb\xbfREGEDIT4 FOO\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test6\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test6");

    exec_import_wstr("\xef\xbb\xbfREGEDIT5\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test7\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test7");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 4.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test8\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test8");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test9\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test9");

    exec_import_wstr("\xef\xbb\xbfWINDOWS REGISTRY EDITOR VERSION 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test10\"=\"Value\"\n");
    verify_reg_nonexist(hkey, "Test10");

    os_version = GetVersion();
    major_version = LOBYTE(LOWORD(os_version));
    minor_version = HIBYTE(LOWORD(os_version));

    if (major_version > 5 || (major_version == 5 && minor_version > 0))
    {
        exec_import_wstr("\xef\xbb\xbfWindows Registry Editor version 5.00\n\n"
                         "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                         "\"Test11\"=\"Value\"\n");
        verify_reg_nonexist(hkey, "Test11");
    }
    else /* Windows 2000 */
        win_skip("Skipping a non-standard header test\n");

    /* Test multi-line import with incorrect comma placement */
    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line1\"=hex(7):4c,00,69,00,6e,00,65,00,20,00\\\n"
                     ",63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg_nonexist(hkey, "Multi-Line1");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line2\"=hex(7):4c,00,69,00,6e,00,65,00,20,00\\\n"
                     "  ,63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg_nonexist(hkey, "Multi-Line2");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line3\"=hex(7):4c,00,69,00,6e,00,65,00,20,00\\\n"
                     ",63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg_nonexist(hkey, "Multi-Line3");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line4\"=hex(7):4c,00,69,00,6e,00,65,00,20,00\\\n"
                     ",  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg_nonexist(hkey, "Multi-Line4");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test12a\"=dword:\n"
                     "\"Test12b\"=dword:hello\n"
                     "\"Test12c\"=dword:123456789\n"
                     "\"Test12d\"=dword:012345678\n"
                     "\"Test12e\"=dword:000000001\n\n");
    verify_reg_nonexist(hkey, "Test12a");
    verify_reg_nonexist(hkey, "Test12b");
    verify_reg_nonexist(hkey, "Test12c");
    verify_reg_nonexist(hkey, "Test12d");
    verify_reg_nonexist(hkey, "Test12e");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test13a\"=dword:12345678abc\n"
                     "\"Test13b\"=dword:12345678 abc\n\n");
    verify_reg_nonexist(hkey, "Test13a");
    verify_reg_nonexist(hkey, "Test13b");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test14a\"=dword:0x123\n"
                     "\"Test14b\"=dword:123 456\n"
                     "\"Test14c\"=dword:1234 5678\n\n");
    verify_reg_nonexist(hkey, "Test14a");
    verify_reg_nonexist(hkey, "Test14b");
    verify_reg_nonexist(hkey, "Test14c");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test15a\"=\"foo\"bar\"\n"
                     "\"Test15b\"=\"foo\"\"bar\"\n\n");
    verify_reg_nonexist(hkey, "Test15a");
    verify_reg_nonexist(hkey, "Test15b");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test16a\"=\n"
                     "\"Test16b\"=\\\"\n"
                     "\"Test16c\"=\\\"Value\\\"\n"
                     "\"Test16d\"=\\\"Value\"\n\n");
    verify_reg_nonexist(hkey, "Test16a");
    verify_reg_nonexist(hkey, "Test16b");
    verify_reg_nonexist(hkey, "Test16c");
    verify_reg_nonexist(hkey, "Test16d");

    /* Test key name and value name concatenation */
    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\\n"
                     "Subkey1]\n");
    verify_key_nonexist(hkey, "Subkey1");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\n"
                     "\\Subkey2]\n");
    verify_key_nonexist(hkey, "Subkey2");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test\\\n"
                     "17a\"=\"Value 1\"\n"
                     "\"Test17b\"=\"Value 2\"\n"
                     "\"Test\n"
                     "\\17c\"=\"Value 3\"\n\n");
    verify_reg_nonexist(hkey, "Test17a");
    verify_reg(hkey, "Test17b", REG_SZ, "Value 2", 8, 0);
    verify_reg_nonexist(hkey, "Test17c");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test18a\"=dword:1234\\\n"
                     "5678\n"
                     "\"Test18b\"=\"Test \\\n"
                     "Value\"\n\n");
    verify_reg_nonexist(hkey, "Test18a");
    verify_reg_nonexist(hkey, "Test18b");

    /* Test hex data concatenation for REG_NONE, REG_EXPAND_SZ and REG_BINARY */
    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test19a\"=hex(0):56,00,61,00,6c,00\\\n"
                     ",75,00,65,00,00,00\n"
                     "\"Test19b\"=hex(0):56,00,61,00,6c,00\\\n"
                     "  ,75,00,65,00,00,00\n"
                     "\"Test19c\"=hex(0):56,00,61,00,6c,00\\\n"
                     "  75,00,65,00,00,00\n"
                     "\"Test19d\"=hex(0):56,00,61,00,6c,00,7\\\n"
                     "5,00,65,00,00,00\n"
                     "\"Test19e\"=hex(0):56,00,61,00,6c,00,7\\\n"
                     "  5,00,65,00,00,00\n"
                     "\"Test19f\"=hex(0):56,00,61,00,\\;comment\n"
                     "  6c,00,75,00,\\#comment\n"
                     "  65,00,00,00\n\n");
    verify_reg_nonexist(hkey, "Test19a");
    verify_reg_nonexist(hkey, "Test19b");
    verify_reg_nonexist(hkey, "Test19c");
    verify_reg_nonexist(hkey, "Test19d");
    verify_reg_nonexist(hkey, "Test19e");
    verify_reg_nonexist(hkey, "Test19f");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test20a\"=hex(2):25,00,50,00,41,00\\\n"
                     ",54,00,48,00,25,00,00,00\n"
                     "\"Test20b\"=hex(2):25,00,50,00,41,00\\\n"
                     "  ,54,00,48,00,25,00,00,00\n"
                     "\"Test20c\"=hex(2):25,00,50,00,41,00\\\n"
                     "  54,00,48,00,25,00,00,00\n"
                     "\"Test20d\"=hex(2):25,00,50,00,4\\\n"
                     "1,00,54,00,48,00,25,00,00,00\n"
                     "\"Test20e\"=hex(2):25,00,50,00,4\\\n"
                     "  1,00,54,00,48,00,25,00,00\n"
                     "\"Test20f\"=hex(2):25,00,50,00,41,00,\\;comment\n"
                     "  54,00,48,00,\\#comment\n"
                     "  25,00,00,00\n\n");
    verify_reg_nonexist(hkey, "Test20a");
    verify_reg_nonexist(hkey, "Test20b");
    verify_reg_nonexist(hkey, "Test20c");
    verify_reg_nonexist(hkey, "Test20d");
    verify_reg_nonexist(hkey, "Test20e");
    verify_reg_nonexist(hkey, "Test20f");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test21a\"=hex:11,22,33,44\\\n"
                     ",55,66,77,88\n"
                     "\"Test21b\"=hex:11,22,33,44\\\n"
                     "  ,55,66,77,88\n"
                     "\"Test21c\"=hex:11,22,33,44\\\n"
                     "  55,66,77,88\n"
                     "\"Test21d\"=hex:11,22,33,4\\\n"
                     "4,55,66,77,88\n"
                     "\"Test21e\"=hex:11,22,33,4\\\n"
                     "  4,55,66,77,88\n"
                     "\"Test21f\"=hex:11,22,33,\\;comment\n"
                     "  44,55,66,\\#comment\n"
                     "  77,88\n\n");
    verify_reg_nonexist(hkey, "Test21a");
    verify_reg_nonexist(hkey, "Test21b");
    verify_reg_nonexist(hkey, "Test21c");
    verify_reg_nonexist(hkey, "Test21d");
    verify_reg_nonexist(hkey, "Test21e");
    verify_reg_nonexist(hkey, "Test21f");

    /* Test support for characters greater than 0xff */
    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine22a\"=hex(0):25,50,100,54,48,25,00\n"
                     "\"Wine22b\"=hex(0):25,1a4,100,164,124,25,00\n\n");
    verify_reg_nonexist(hkey, "Wine22a");
    verify_reg_nonexist(hkey, "Wine22b");

    /* Test the effect of backslashes in hex data */
    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine23a\"=hex(2):25,00,48\\,00,4f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine23b\"=hex(2):25,00,48,00,\\4f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine23c\"=hex(2):25,00,48\\ ,00,4f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine23d\"=hex(2):25,00,48,00,\\ 4f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine23e\"=hex(2):\\25,00,48,00,4f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine23f\"=hex(2):\\ 25,00,48,00,4f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine23g\"=hex(2):25,00,48,00,4\\f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine23h\"=hex(2):25,00,48,00,4\\\n"
                     "  f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine23i\"=hex(2):25,00,50,00,\\,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine23j\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,\\\\\n"
                     "  25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine23k\"=hex(2):,\\\n"
                     "  25,00,48,00,4f,00,4d,00,45,00,25,00,00,00\n\n");
    verify_reg_nonexist(hkey, "Wine23a");
    verify_reg_nonexist(hkey, "Wine23b");
    verify_reg_nonexist(hkey, "Wine23c");
    verify_reg_nonexist(hkey, "Wine23d");
    verify_reg_nonexist(hkey, "Wine23e");
    verify_reg_nonexist(hkey, "Wine23f");
    verify_reg_nonexist(hkey, "Wine23g");
    verify_reg_nonexist(hkey, "Wine23h");
    verify_reg_nonexist(hkey, "Wine23i");
    verify_reg_nonexist(hkey, "Wine23j");
    verify_reg_nonexist(hkey, "Wine23k");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine24a\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1]\n");
    verify_reg_nonexist(hkey, "Wine24a");
    verify_key_nonexist(hkey, "Subkey1");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine24b\"=hex(2):4c,00,69,00,6e,00,65,00,20,00\\\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2]\n");
    verify_reg_nonexist(hkey, "Wine24b");
    verify_key(hkey, "Subkey2");

    lr = RegDeleteKeyA(hkey, "Subkey2");
    ok(lr == ERROR_SUCCESS, "RegDeleteKey failed: %u\n", lr);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine25a\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "\"Wine25b\"=\"Test value\"\n"

                     "\"Wine25c\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     ";comment\n"
                     "\"Wine25d\"=\"Test value\"\n"

                     "\"Wine25e\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "#comment\n"
                     "\"Wine25f\"=\"Test value\"\n"

                     "\"Wine25g\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n\n"
                     "\"Wine25h\"=\"Test value\"\n"

                     "\"Wine25i\"=hex(2):4c,00,69,00,6e,00,65,00,20,00\\\n"
                     "\"Wine25j\"=\"Test value\"\n\n");
    verify_reg_nonexist(hkey, "Wine25a");
    verify_reg_nonexist(hkey, "Wine25b");
    verify_reg_nonexist(hkey, "Wine25c");
    verify_reg_nonexist(hkey, "Wine25d");
    verify_reg_nonexist(hkey, "Wine25e");
    verify_reg(hkey, "Wine25f", REG_SZ, "Test value", 11, 0);
    verify_reg_nonexist(hkey, "Wine25g");
    verify_reg_nonexist(hkey, "Wine25h");
    verify_reg_nonexist(hkey, "Wine25i");
    verify_reg(hkey, "Wine25j", REG_SZ, "Test value", 11, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine26a\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "\"Wine26b\"=dword:00000008\n"

                     "\"Wine26c\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     ";comment\n"
                     "\"Wine26d\"=dword:00000008\n"

                     "\"Wine26e\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "#comment\n"
                     "\"Wine26f\"=dword:00000008\n"

                     "\"Wine26g\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n\n"
                     "\"Wine26h\"=dword:00000008\n"

                     "\"Wine26i\"=hex(2):4c,00,69,00,6e,00,65,00,20,00\\\n"
                     "\"Wine26j\"=dword:00000008\n\n");
    verify_reg_nonexist(hkey, "Wine26a");
    verify_reg_nonexist(hkey, "Wine26b");
    verify_reg_nonexist(hkey, "Wine26c");
    verify_reg_nonexist(hkey, "Wine26d");
    verify_reg_nonexist(hkey, "Wine26e");
    verify_reg(hkey, "Wine26f", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg_nonexist(hkey, "Wine26g");
    verify_reg_nonexist(hkey, "Wine26h");
    verify_reg_nonexist(hkey, "Wine26i");
    verify_reg(hkey, "Wine26j", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine27a\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,\\\n"
                     "\"Wine27b\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"

                     "\"Wine27c\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,\\\n"
                     ";comment\n"
                     "\"Wine27d\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"

                     "\"Wine27e\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,\\\n"
                     "#comment\n"
                     "\"Wine27f\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"

                     "\"Wine27g\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,\\\n\n"
                     "\"Wine27h\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"

                     "\"Wine27i\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00\\\n"
                     "\"Wine27j\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\n\n");
    verify_reg_nonexist(hkey, "Wine27a");
    verify_reg_nonexist(hkey, "Wine27b");
    verify_reg_nonexist(hkey, "Wine27c");
    verify_reg_nonexist(hkey, "Wine27d");
    verify_reg_nonexist(hkey, "Wine27e");
    verify_reg(hkey, "Wine27f", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg_nonexist(hkey, "Wine27g");
    verify_reg_nonexist(hkey, "Wine27h");
    verify_reg_nonexist(hkey, "Wine27i");
    verify_reg(hkey, "Wine27j", REG_EXPAND_SZ, "%PATH%", 7, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine28a\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "@=\"Default value 1\"\n\n");
    verify_reg_nonexist(hkey, "Wine28a");
    verify_reg_nonexist(hkey, NULL);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine28b\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     ";comment\n"
                     "@=\"Default value 2\"\n\n");
    verify_reg_nonexist(hkey, "Wine28b");
    verify_reg_nonexist(hkey, NULL);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine28c\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "#comment\n"
                     "@=\"Default value 3\"\n\n");
    verify_reg_nonexist(hkey, "Wine28c");
    verify_reg(hkey, NULL, REG_SZ, "Default value 3", 16, 0);

    lr = RegDeleteValueW(hkey, NULL);
    ok(lr == ERROR_SUCCESS, "RegDeleteValue failed: %u\n", lr);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine28d\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n\n"
                     "@=\"Default value 4\"\n\n");
    verify_reg_nonexist(hkey, "Wine28d");
    verify_reg_nonexist(hkey, NULL);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine28e\"=hex(2):4c,00,69,00,6e,00,65,00,20,00\\\n"
                     "@=\"Default value 5\"\n\n");
    verify_reg_nonexist(hkey, "Wine28e");
    verify_reg(hkey, NULL, REG_SZ, "Default value 5", 16, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine29a\"=hex:11,22,33,\\\n"
                     "\\\n"
                     "  44,55,66\n"
                     "\"Wine29b\"=hex:11,22,33,\\\n"
                     "  \\\n"
                     "  44,55,66\n\n");
    verify_reg_nonexist(hkey, "Wine29a");
    verify_reg_nonexist(hkey, "Wine29b");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine30a\"=hex(0):25,48,4f,4d,45,25,5c,/\n"
                     "  25,50,41,54,48,25,00\n"
                     "\"Wine30b\"=hex(0):25,48,4f,4d,45,25,5c/\n"
                     "  25,50,41,54,48,25,00\n\n");
    verify_reg_nonexist(hkey, "Wine30a");
    verify_reg_nonexist(hkey, "Wine30b");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine31\"=hex(7):4c,00,69,00,6e,00,65,00,20,00\\");
    verify_reg_nonexist(hkey, "Wine31");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine32a\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  ,63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "\"Wine32b\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg_nonexist(hkey, "Wine32a");
    verify_reg_nonexist(hkey, "Wine32b");

    RegCloseKey(hkey);

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: %d\n", lr);
}

static void test_invalid_import_31(void)
{
    HKEY hkey;
    LONG lr;

    /* Check if regedit.exe is running with elevated privileges */
    lr = RegCreateKeyExA(HKEY_CLASSES_ROOT, KEY_BASE, 0, NULL, REG_OPTION_NON_VOLATILE,
                         KEY_READ, NULL, &hkey, NULL);
    if (lr == ERROR_ACCESS_DENIED)
    {
        win_skip("regedit.exe is not running with elevated privileges; "
            "skipping Windows 3.1 invalid import tests\n");
        return;
    }

    /* Test character validity at the start of the line */
    exec_import_str("REGEDIT\r\n"
                    " HKEY_CLASSES_ROOT\\" KEY_BASE " = Value1a\r\n");
    verify_reg_nonexist(hkey, "");

    exec_import_str("REGEDIT\r\n"
                    "  HKEY_CLASSES_ROOT\\" KEY_BASE " = Value1b\r\n");
    verify_reg_nonexist(hkey, "");

    exec_import_str("REGEDIT\r\n"
                    "\tHKEY_CLASSES_ROOT\\" KEY_BASE " = Value1c\r\n");
    verify_reg_nonexist(hkey, "");

    exec_import_str("REGEDIT\r\n"
                    ";HKEY_CLASSES_ROOT\\" KEY_BASE " = Value2a\r\n");
    verify_reg_nonexist(hkey, "");

    exec_import_str("REGEDIT\r\n"
                    "#HKEY_CLASSES_ROOT\\" KEY_BASE " = Value2b\r\n");
    verify_reg_nonexist(hkey, "");

    /* Test case sensitivity */
    exec_import_str("REGEDIT\r\n"
                    "hkey_classes_root\\" KEY_BASE " = Value3a\r\n");
    verify_reg_nonexist(hkey, "");

    exec_import_str("REGEDIT\r\n"
                    "hKEY_CLASSES_ROOT\\" KEY_BASE " = Value3b\r\n");
    verify_reg_nonexist(hkey, "");

    exec_import_str("REGEDIT\r\n"
                    "Hkey_Classes_Root\\" KEY_BASE " = Value3c\r\n");
    verify_reg_nonexist(hkey, "");

    RegCloseKey(hkey);

    lr = RegDeleteKeyA(HKEY_CLASSES_ROOT, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: %d\n", lr);
}

static void test_comments(void)
{
    LONG lr;
    HKEY hkey;
    DWORD dword;

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND, "RegDeleteKeyA failed: %d\n", lr);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "#comment\\\n"
                    "\"Wine1\"=\"Line 1\"\n"
                    ";comment\\\n"
                    "\"Wine2\"=\"Line 2\"\n\n");
    lr = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: %d\n", lr);
    verify_reg(hkey, "Wine1", REG_SZ, "Line 1", 7, 0);
    verify_reg(hkey, "Wine2", REG_SZ, "Line 2", 7, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine3\"=\"Value 1\"#comment\n"
                    "\"Wine4\"=\"Value 2\";comment\n"
                    "\"Wine5\"=dword:01020304 #comment\n"
                    "\"Wine6\"=dword:02040608 ;comment\n\n");
    verify_reg_nonexist(hkey, "Wine3");
    verify_reg(hkey, "Wine4", REG_SZ, "Value 2", 8, 0);
    verify_reg_nonexist(hkey, "Wine5");
    dword = 0x2040608;
    verify_reg(hkey, "Wine6", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine7\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  #comment\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine8\"=\"A valid line\"\n"
                    "\"Wine9\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  ;comment\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine10\"=\"Another valid line\"\n\n");
    verify_reg_nonexist(hkey, "Wine7");
    verify_reg(hkey, "Wine8", REG_SZ, "A valid line", 13, 0);
    verify_reg(hkey, "Wine9", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine10", REG_SZ, "Another valid line", 19, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "#\"Comment1\"=\"Value 1\"\n"
                    ";\"Comment2\"=\"Value 2\"\n"
                    "    #\"Comment3\"=\"Value 3\"\n"
                    "    ;\"Comment4\"=\"Value 4\"\n"
                    "\"Wine11\"=\"Value 6\"#\"Comment5\"=\"Value 5\"\n"
                    "\"Wine12\"=\"Value 7\";\"Comment6\"=\"Value 6\"\n\n");
    verify_reg_nonexist(hkey, "Comment1");
    verify_reg_nonexist(hkey, "Comment2");
    verify_reg_nonexist(hkey, "Comment3");
    verify_reg_nonexist(hkey, "Comment4");
    verify_reg_nonexist(hkey, "Wine11");
    verify_reg_nonexist(hkey, "Comment5");
    verify_reg(hkey, "Wine12", REG_SZ, "Value 7", 8, 0);
    verify_reg_nonexist(hkey, "Comment6");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine13\"=#\"Value 8\"\n"
                    "\"Wine14\"=;\"Value 9\"\n"
                    "\"Wine15\"=\"#comment1\"\n"
                    "\"Wine16\"=\";comment2\"\n"
                    "\"Wine17\"=\"Value#comment3\"\n"
                    "\"Wine18\"=\"Value;comment4\"\n"
                    "\"Wine19\"=\"Value #comment5\"\n"
                    "\"Wine20\"=\"Value ;comment6\"\n"
                    "\"Wine21\"=#dword:00000001\n"
                    "\"Wine22\"=;dword:00000002\n"
                    "\"Wine23\"=dword:00000003#comment\n"
                    "\"Wine24\"=dword:00000004;comment\n\n");
    verify_reg_nonexist(hkey, "Wine13");
    verify_reg_nonexist(hkey, "Wine14");
    verify_reg(hkey, "Wine15", REG_SZ, "#comment1", 10, 0);
    verify_reg(hkey, "Wine16", REG_SZ, ";comment2", 10, 0);
    verify_reg(hkey, "Wine17", REG_SZ, "Value#comment3", 15, 0);
    verify_reg(hkey, "Wine18", REG_SZ, "Value;comment4", 15, 0);
    verify_reg(hkey, "Wine19", REG_SZ, "Value #comment5", 16, 0);
    verify_reg(hkey, "Wine20", REG_SZ, "Value ;comment6", 16, 0);
    verify_reg_nonexist(hkey, "Wine21");
    verify_reg_nonexist(hkey, "Wine22");
    verify_reg_nonexist(hkey, "Wine23");
    dword = 0x00000004;
    verify_reg(hkey, "Wine24", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine25a\"=dword:1234;5678\n"
                    "\"Wine25b\"=dword:1234 ;5678\n"
                    "\"Wine25c\"=dword:1234#5678\n"
                    "\"Wine25d\"=dword:1234 #5678\n\n");
    dword = 0x1234;
    verify_reg(hkey, "Wine25a", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine25b", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg_nonexist(hkey, "Wine25c");
    verify_reg_nonexist(hkey, "Wine25d");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine26a\"=\"Value1\"  ;comment\n"
                    "\"Wine26b\"=\"Value2\"\t\t;comment\n"
                    "\"Wine26c\"=\"Value3\"  #comment\n"
                    "\"Wine26d\"=\"Value4\"\t\t#comment\n\n");
    verify_reg(hkey, "Wine26a", REG_SZ, "Value1", 7, 0);
    verify_reg(hkey, "Wine26b", REG_SZ, "Value2", 7, 0);
    verify_reg_nonexist(hkey, "Wine26c");
    verify_reg_nonexist(hkey, "Wine26d");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line1\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,\\;comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n");
    verify_reg(hkey, "Multi-Line1", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line2\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,;comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n");
    verify_reg(hkey, "Multi-Line2", REG_MULTI_SZ, "Line concat", 12, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line3\"=hex(7):4c,69,6e,\\;comment\n"
                    "  65,20,\\;comment\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,\\;comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n");
    verify_reg(hkey, "Multi-Line3", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line4\"=hex(7):4c,69,6e,\\;#comment\n"
                    "  65,20,\\;#comment\n"
                    "  63,6f,6e,\\;#comment\n"
                    "  63,61,74,\\;#comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n");
    verify_reg(hkey, "Multi-Line4", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line5\"=hex(7):4c,69,6e,\\;comment\n"
                    "  65,20,\\;comment\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,\\#comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n");
    verify_reg_nonexist(hkey, "Multi-Line5");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line6\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,\\\n\n"
                    "  65,6e,\\;comment\n\n"
                    "  61,74,69,6f,6e,00,00\n\n");
    verify_reg(hkey, "Multi-Line6", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine27a\"=hex(2):25,50,41,54,48,25,00  ;comment\n"
                    "\"Wine27b\"=hex(2):25,50,41,54,48,25,00\t;comment\n"
                    "\"Wine27c\"=hex(2):25,50,41,54,48,25,00  #comment\n"
                    "\"Wine27d\"=hex(2):25,50,41,54,48,25,00\t#comment\n\n");
    verify_reg(hkey, "Wine27a", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine27b", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg_nonexist(hkey, "Wine27c");
    verify_reg_nonexist(hkey, "Wine27d");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine28a\"=hex(2):25,48,4f,4d,45,25,5c,\\\n"
                    "  25,50,41,54,48,25,00\n"
                    "\"Wine28b\"=hex(2):25,48,4f,4d,45,25,5c\\\n"
                    "  25,50,41,54,48,25,00\n"
                    "\"Wine28c\"=hex(2):25,48,4f,4d,45,25,5c,  \\  ;comment\n"
                    "  25,50,41,54,48,25,00\n"
                    "\"Wine28d\"=hex(2):25,48,4f,4d,45,25,5c  \\  ;comment\n"
                    "  25,50,41,54,48,25,00\n"
                    "\"Wine28e\"=hex(2):25,48,4f,4d,45,25,5c,\\\t  ;comment\n"
                    "  25,50,41,54,48,25,00\n"
                    "\"Wine28f\"=hex(2):25,48,4f,4d,45,25,5c\\\t  ;comment\n"
                    "  25,50,41,54,48,25,00\n\n");
    verify_reg(hkey, "Wine28a", REG_EXPAND_SZ, "%HOME%\\%PATH%", 14, 0);
    verify_reg_nonexist(hkey, "Wine28b");
    verify_reg(hkey, "Wine28c", REG_EXPAND_SZ, "%HOME%\\%PATH%", 14, 0);
    verify_reg_nonexist(hkey, "Wine28d");
    verify_reg(hkey, "Wine28e", REG_EXPAND_SZ, "%HOME%\\%PATH%", 14, 0);
    verify_reg_nonexist(hkey, "Wine28f");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine29a\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,\\\n"
                    ";comment\n"
                    "  65,6e,\\;comment\n"
                    "  61,74,69,6f,6e,00,00\n\n");
    verify_reg(hkey, "Wine29a", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine29b\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,\\\n"
                    "  ;comment\n"
                    "  65,6e,\\;comment\n"
                    "  61,74,69,6f,6e,00,00\n\n");
    verify_reg(hkey, "Wine29b", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine29c\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,\\\n"
                    "#comment\n"
                    "  65,6e,\\;comment\n"
                    "  61,74,69,6f,6e,00,00\n\n");
    verify_reg_nonexist(hkey, "Wine29c");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine29d\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,\\\n"
                    "  #comment\n"
                    "  65,6e,\\;comment\n"
                    "  61,74,69,6f,6e,00,00\n\n");
    verify_reg_nonexist(hkey, "Wine29d");

    RegCloseKey(hkey);

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: %d\n", lr);
}

static void test_comments_unicode(void)
{
    LONG lr;
    HKEY hkey;
    DWORD dword;

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND, "RegDeleteKeyA failed: %d\n", lr);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "#comment\\\n"
                     "\"Wine1\"=\"Line 1\"\n"
                     ";comment\\\n"
                     "\"Wine2\"=\"Line 2\"\n\n");
    lr = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: %d\n", lr);
    verify_reg(hkey, "Wine1", REG_SZ, "Line 1", 7, 0);
    verify_reg(hkey, "Wine2", REG_SZ, "Line 2", 7, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine3\"=\"Value 1\"#comment\n"
                     "\"Wine4\"=\"Value 2\";comment\n"
                     "\"Wine5\"=dword:01020304 #comment\n"
                     "\"Wine6\"=dword:02040608 ;comment\n\n");
    verify_reg_nonexist(hkey, "Wine3");
    verify_reg(hkey, "Wine4", REG_SZ, "Value 2", 8, 0);
    verify_reg_nonexist(hkey, "Wine5");
    dword = 0x2040608;
    verify_reg(hkey, "Wine6", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine7\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  #comment\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "\"Wine8\"=\"A valid line\"\n"
                     "\"Wine9\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  ;comment\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "\"Wine10\"=\"Another valid line\"\n\n");
    verify_reg_nonexist(hkey, "Wine7");
    verify_reg(hkey, "Wine8", REG_SZ, "A valid line", 13, 0);
    verify_reg(hkey, "Wine9", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine10", REG_SZ, "Another valid line", 19, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "#\"Comment1\"=\"Value 1\"\n"
                     ";\"Comment2\"=\"Value 2\"\n"
                     "    #\"Comment3\"=\"Value 3\"\n"
                     "    ;\"Comment4\"=\"Value 4\"\n"
                     "\"Wine11\"=\"Value 6\"#\"Comment5\"=\"Value 5\"\n"
                     "\"Wine12\"=\"Value 7\";\"Comment6\"=\"Value 6\"\n\n");
    verify_reg_nonexist(hkey, "Comment1");
    verify_reg_nonexist(hkey, "Comment2");
    verify_reg_nonexist(hkey, "Comment3");
    verify_reg_nonexist(hkey, "Comment4");
    verify_reg_nonexist(hkey, "Wine11");
    verify_reg_nonexist(hkey, "Comment5");
    verify_reg(hkey, "Wine12", REG_SZ, "Value 7", 8, 0);
    verify_reg_nonexist(hkey, "Comment6");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine13\"=#\"Value 8\"\n"
                     "\"Wine14\"=;\"Value 9\"\n"
                     "\"Wine15\"=\"#comment1\"\n"
                     "\"Wine16\"=\";comment2\"\n"
                     "\"Wine17\"=\"Value#comment3\"\n"
                     "\"Wine18\"=\"Value;comment4\"\n"
                     "\"Wine19\"=\"Value #comment5\"\n"
                     "\"Wine20\"=\"Value ;comment6\"\n"
                     "\"Wine21\"=#dword:00000001\n"
                     "\"Wine22\"=;dword:00000002\n"
                     "\"Wine23\"=dword:00000003#comment\n"
                     "\"Wine24\"=dword:00000004;comment\n\n");
    verify_reg_nonexist(hkey, "Wine13");
    verify_reg_nonexist(hkey, "Wine14");
    verify_reg(hkey, "Wine15", REG_SZ, "#comment1", 10, 0);
    verify_reg(hkey, "Wine16", REG_SZ, ";comment2", 10, 0);
    verify_reg(hkey, "Wine17", REG_SZ, "Value#comment3", 15, 0);
    verify_reg(hkey, "Wine18", REG_SZ, "Value;comment4", 15, 0);
    verify_reg(hkey, "Wine19", REG_SZ, "Value #comment5", 16, 0);
    verify_reg(hkey, "Wine20", REG_SZ, "Value ;comment6", 16, 0);
    verify_reg_nonexist(hkey, "Wine21");
    verify_reg_nonexist(hkey, "Wine22");
    verify_reg_nonexist(hkey, "Wine23");
    dword = 0x00000004;
    verify_reg(hkey, "Wine24", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine25a\"=dword:1234;5678\n"
                     "\"Wine25b\"=dword:1234 ;5678\n"
                     "\"Wine25c\"=dword:1234#5678\n"
                     "\"Wine25d\"=dword:1234 #5678\n\n");
    dword = 0x1234;
    verify_reg(hkey, "Wine25a", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine25b", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg_nonexist(hkey, "Wine25c");
    verify_reg_nonexist(hkey, "Wine25d");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine26a\"=\"Value1\"  ;comment\n"
                     "\"Wine26b\"=\"Value2\"\t\t;comment\n"
                     "\"Wine26c\"=\"Value3\"  #comment\n"
                     "\"Wine26d\"=\"Value4\"\t\t#comment\n\n");
    verify_reg(hkey, "Wine26a", REG_SZ, "Value1", 7, 0);
    verify_reg(hkey, "Wine26b", REG_SZ, "Value2", 7, 0);
    verify_reg_nonexist(hkey, "Wine26c");
    verify_reg_nonexist(hkey, "Wine26d");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line1\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,\\;comment\n"
                     "  63,00,61,00,74,00,\\;comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg(hkey, "Multi-Line1", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line2\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,\\;comment\n"
                     "  63,00,61,00,74,00,;comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg(hkey, "Multi-Line2", REG_MULTI_SZ, "Line concat", 12, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line3\"=hex(7):4c,00,69,00,6e,00,\\;comment\n"
                     "  65,00,20,00,\\;comment\n"
                     "  63,00,6f,00,6e,00,\\;comment\n"
                     "  63,00,61,00,74,00,\\;comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg(hkey, "Multi-Line3", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line4\"=hex(7):4c,00,69,00,6e,00,\\;#comment\n"
                     "  65,00,20,00,\\;#comment\n"
                     "  63,00,6f,00,6e,00,\\;#comment\n"
                     "  63,00,61,00,74,00,\\;#comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg(hkey, "Multi-Line4", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line5\"=hex(7):4c,00,69,00,6e,00,\\;comment\n"
                     "  65,00,20,00,\\;comment\n"
                     "  63,00,6f,00,6e,00,\\;comment\n"
                     "  63,00,61,00,74,00,\\#comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg_nonexist(hkey, "Multi-Line5");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line6\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,\\;comment\n"
                     "  63,00,61,00,74,00,\\\n\n"
                     "  65,00,6e,00,\\;comment\n\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg(hkey, "Multi-Line6", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine27a\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00  ;comment\n"
                     "\"Wine27b\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\t;comment\n"
                     "\"Wine27c\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00  #comment\n"
                     "\"Wine27d\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\t#comment\n\n");
    verify_reg(hkey, "Wine27a", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine27b", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg_nonexist(hkey, "Wine27c");
    verify_reg_nonexist(hkey, "Wine27d");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine28a\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,\\\n"
                     "  25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine28b\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00\\\n"
                     "  25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine28c\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,  \\  ;comment\n"
                     "  25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine28d\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00  \\  ;comment\n"
                     "  25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine28e\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,\\\t  ;comment\n"
                     "  25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine28f\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00\\\t  ;comment\n"
                     "  25,00,50,00,41,00,54,00,48,00,25,00,00,00\n\n");
    verify_reg(hkey, "Wine28a", REG_EXPAND_SZ, "%HOME%\\%PATH%", 14, 0);
    verify_reg_nonexist(hkey, "Wine28b");
    verify_reg(hkey, "Wine28c", REG_EXPAND_SZ, "%HOME%\\%PATH%", 14, 0);
    verify_reg_nonexist(hkey, "Wine28d");
    verify_reg(hkey, "Wine28e", REG_EXPAND_SZ, "%HOME%\\%PATH%", 14, 0);
    verify_reg_nonexist(hkey, "Wine28f");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine29a\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     ";comment\n"
                     "  65,00,6e,00,\\;comment\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg(hkey, "Wine29a", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine29b\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  ;comment\n"
                     "  65,00,6e,00,\\;comment\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg(hkey, "Wine29b", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine29c\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "#comment\n"
                     "  65,00,6e,00,\\;comment\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg_nonexist(hkey, "Wine29c");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine29d\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  #comment\n"
                     "  65,00,6e,00,\\;comment\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg_nonexist(hkey, "Wine29d");

    RegCloseKey(hkey);

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: %d\n", lr);
}

static void test_import_with_whitespace(void)
{
    HKEY hkey;
    LONG lr;
    DWORD dword;

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND, "RegDeleteKeyA failed: %d\n", lr);

    exec_import_str("  REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n");

    lr = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: got %d, expected 0\n", lr);

    exec_import_str("  REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine1a\"=\"Value\"\n\n");
    verify_reg(hkey, "Wine1a", REG_SZ, "Value", 6, 0);

    exec_import_str("\tREGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine1b\"=\"Value\"\n\n");
    verify_reg(hkey, "Wine1b", REG_SZ, "Value", 6, 0);

    exec_import_str(" \t REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine1c\"=\"Value\"\n\n");
    verify_reg(hkey, "Wine1c", REG_SZ, "Value", 6, 0);

    exec_import_str("REGEDIT4\n\n"
                    "  [HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine2a\"=\"Value\"\n\n");
    verify_reg(hkey, "Wine2a", REG_SZ, "Value", 6, 0);

    exec_import_str("REGEDIT4\n\n"
                    "\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine2b\"=\"Value\"\n\n");
    verify_reg(hkey, "Wine2b", REG_SZ, "Value", 6, 0);

    exec_import_str("REGEDIT4\n\n"
                    " \t [HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine2c\"=\"Value\"\n\n");
    verify_reg(hkey, "Wine2c", REG_SZ, "Value", 6, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "  \"Wine3a\"=\"Two leading spaces\"\n\n");
    verify_reg(hkey, "Wine3a", REG_SZ, "Two leading spaces", 19, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\t\"Wine3b\"=\"One leading tab\"\n\n");
    verify_reg(hkey, "Wine3b", REG_SZ, "One leading tab", 16, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    " \t \"Wine3c\"=\"Space, tab, space\"\n\n");
    verify_reg(hkey, "Wine3c", REG_SZ, "Space, tab, space", 18, 0);

    exec_import_str("                    REGEDIT4\n\n"
                    "\t\t\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\t    \"Wine4a\"=\"Tab and four spaces\"\n"
                    "    \"Wine4b\"=dword:00112233\n"
                    "\t  \t  \t  \t  \t  \t  \"Wine4c\"=hex(7):4c,69,6e,65,20,\\\n"
                    "        63,6f,6e,\\;comment\n"
                    "\t\t\t\t63,61,74,\\;comment\n"
                    "  \t65,6e,61,74,69,6f,6e,00,00\n\n");
    verify_reg(hkey, "Wine4a", REG_SZ, "Tab and four spaces", 20, 0);
    dword = 0x112233;
    verify_reg(hkey, "Wine4b", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine4c", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_str("    REGEDIT4\n\n"
                    "\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "  \"Wine5a\"=\"Leading spaces\"\n"
                    "\t\t\"Wine5b\"\t\t=\"Leading tabs\"\n"
                    "\t  \"Wine5c\"=\t  \"Tabs and spaces\"\n"
                    "    \"Wine5d\"    \t    =    \t    \"More whitespace\"\n\n");
    verify_reg(hkey, "Wine5a", REG_SZ, "Leading spaces", 15, 0);
    verify_reg(hkey, "Wine5b", REG_SZ, "Leading tabs", 13, 0);
    verify_reg(hkey, "Wine5c", REG_SZ, "Tabs and spaces", 16, 0);
    verify_reg(hkey, "Wine5d", REG_SZ, "More whitespace", 16, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"  Wine6a\"=\"Leading spaces\"\n"
                    "\"\t\tWine6b\"=\"Leading tabs\"\n"
                    "  \"  Wine6c  \"  =  \"  Spaces everywhere  \"  \n\n");
    verify_reg(hkey, "  Wine6a", REG_SZ, "Leading spaces", 15, 0);
    verify_reg(hkey, "\t\tWine6b", REG_SZ, "Leading tabs", 13, 0);
    verify_reg(hkey, "  Wine6c  ", REG_SZ, "  Spaces everywhere  ", 22, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine7a\"=\"    Four spaces in the data\"\n"
                    "\"Wine7b\"=\"\t\tTwo tabs in the data\"\n\n");
    verify_reg(hkey, "Wine7a", REG_SZ, "    Four spaces in the data", 28, 0);
    verify_reg(hkey, "Wine7b", REG_SZ, "\t\tTwo tabs in the data", 23, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine8a\"=\"Trailing spaces\"    \n"
                    "\"Wine8b\"=\"Trailing tabs and spaces\"\t  \t\n\n");
    verify_reg(hkey, "Wine8a", REG_SZ, "Trailing spaces", 16, 0);
    verify_reg(hkey, "Wine8b", REG_SZ, "Trailing tabs and spaces", 25, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine9a\"=dword:  00000008\n"
                    "\"Wine9b\"=dword:\t\t00000008\n\n");
    dword = 0x00000008;
    verify_reg(hkey, "Wine9a", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine9b", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "@  =  \"Test Value\"\n\n");
    verify_reg(hkey, "", REG_SZ, "Test Value", 11, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\t@\t=\tdword:\t00000008\t\n\n");
    verify_reg(hkey, "", REG_DWORD, &dword, sizeof(DWORD), 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine10a\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,\\\n\n"
                    "  63,61,74,\\\n\n\n"
                    "  65,6e,\\\n\n\n\n"
                    "  61,74,69,6f,6e,00,00\n\n");
    verify_reg(hkey, "Wine10a", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine10b\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,\\\n \n"
                    "  63,61,74,\\\n\t\n\t\n"
                    "  65,6e,\\\n\t \t\n\t \t\n\t \t\n"
                    "  61,74,69,6f,6e,00,00\n\n");
    verify_reg(hkey, "Wine10b", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    lr = RegCloseKey(hkey);
    ok(lr == ERROR_SUCCESS, "RegCloseKey failed: got %d, expected 0\n", lr);

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: got %d, expected 0\n", lr);
}

static void test_import_with_whitespace_unicode(void)
{
    HKEY hkey;
    LONG lr;
    DWORD dword;

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND, "RegDeleteKeyA failed: %d\n", lr);

    exec_import_wstr("\xef\xbb\xbf  Windows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n");

    lr = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: got %d, expected 0\n", lr);

    exec_import_wstr("\xef\xbb\xbf  Windows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine1a\"=\"Value\"\n\n");
    verify_reg(hkey, "Wine1a", REG_SZ, "Value", 6, 0);

    exec_import_wstr("\xef\xbb\xbf\tWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine1b\"=\"Value\"\n\n");
    verify_reg(hkey, "Wine1b", REG_SZ, "Value", 6, 0);

    exec_import_wstr("\xef\xbb\xbf \t Windows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine1c\"=\"Value\"\n\n");
    verify_reg(hkey, "Wine1c", REG_SZ, "Value", 6, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "  [HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine2a\"=\"Value\"\n\n");
    verify_reg(hkey, "Wine2a", REG_SZ, "Value", 6, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine2b\"=\"Value\"\n\n");
    verify_reg(hkey, "Wine2b", REG_SZ, "Value", 6, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     " \t [HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine2c\"=\"Value\"\n\n");
    verify_reg(hkey, "Wine2c", REG_SZ, "Value", 6, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "  \"Wine3a\"=\"Two leading spaces\"\n\n");
    verify_reg(hkey, "Wine3a", REG_SZ, "Two leading spaces", 19, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\t\"Wine3b\"=\"One leading tab\"\n\n");
    verify_reg(hkey, "Wine3b", REG_SZ, "One leading tab", 16, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     " \t \"Wine3c\"=\"Space, tab, space\"\n\n");
    verify_reg(hkey, "Wine3c", REG_SZ, "Space, tab, space", 18, 0);

    exec_import_wstr("\xef\xbb\xbf                    Windows Registry Editor Version 5.00\n\n"
                     "\t\t\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\t    \"Wine4a\"=\"Tab and four spaces\"\n"
                     "    \"Wine4b\"=dword:00112233\n"
                     "\t  \t  \t  \t  \t  \t  \"Wine4c\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "        63,00,6f,00,6e,00,\\;comment\n"
                     "\t\t\t\t63,00,61,00,74,00,\\;comment\n"
                     "  \t65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg(hkey, "Wine4a", REG_SZ, "Tab and four spaces", 20, 0);
    dword = 0x112233;
    verify_reg(hkey, "Wine4b", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine4c", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_wstr("\xef\xbb\xbf    Windows Registry Editor Version 5.00\n\n"
                     "\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "  \"Wine5a\"=\"Leading spaces\"\n"
                     "\t\t\"Wine5b\"\t\t=\"Leading tabs\"\n"
                     "\t  \"Wine5c\"=\t  \"Tabs and spaces\"\n"
                     "    \"Wine5d\"    \t    =    \t    \"More whitespace\"\n\n");
    verify_reg(hkey, "Wine5a", REG_SZ, "Leading spaces", 15, 0);
    verify_reg(hkey, "Wine5b", REG_SZ, "Leading tabs", 13, 0);
    verify_reg(hkey, "Wine5c", REG_SZ, "Tabs and spaces", 16, 0);
    verify_reg(hkey, "Wine5d", REG_SZ, "More whitespace", 16, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"  Wine6a\"=\"Leading spaces\"\n"
                     "\"\t\tWine6b\"=\"Leading tabs\"\n"
                     "  \"  Wine6c  \"  =  \"  Spaces everywhere  \"  \n\n");
    verify_reg(hkey, "  Wine6a", REG_SZ, "Leading spaces", 15, 0);
    verify_reg(hkey, "\t\tWine6b", REG_SZ, "Leading tabs", 13, 0);
    verify_reg(hkey, "  Wine6c  ", REG_SZ, "  Spaces everywhere  ", 22, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine7a\"=\"    Four spaces in the data\"\n"
                     "\"Wine7b\"=\"\t\tTwo tabs in the data\"\n\n");
    verify_reg(hkey, "Wine7a", REG_SZ, "    Four spaces in the data", 28, 0);
    verify_reg(hkey, "Wine7b", REG_SZ, "\t\tTwo tabs in the data", 23, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine8a\"=\"Trailing spaces\"    \n"
                     "\"Wine8b\"=\"Trailing tabs and spaces\"\t  \t\n\n");
    verify_reg(hkey, "Wine8a", REG_SZ, "Trailing spaces", 16, 0);
    verify_reg(hkey, "Wine8b", REG_SZ, "Trailing tabs and spaces", 25, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine9a\"=dword:  00000008\n"
                     "\"Wine9b\"=dword:\t\t00000008\n\n");
    dword = 0x00000008;
    verify_reg(hkey, "Wine9a", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine9b", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "@  =  \"Test Value\"\n\n");
    verify_reg(hkey, "", REG_SZ, "Test Value", 11, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\t@\t=\tdword:\t00000008\t\n\n");
    verify_reg(hkey, "", REG_DWORD, &dword, sizeof(DWORD), 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine10a\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,\\\n\n"
                     "  63,00,61,00,74,00,\\\n\n\n"
                     "  65,00,6e,00,\\\n\n\n\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg(hkey, "Wine10a", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine10b\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,\\\n \n"
                     "  63,00,61,00,74,00,\\\n\t\n\t\n"
                     "  65,00,6e,00,\\\n\t \t\n\t \t\n\t \t\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n");
    verify_reg(hkey, "Wine10b", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    lr = RegCloseKey(hkey);
    ok(lr == ERROR_SUCCESS, "RegCloseKey failed: got %d, expected 0\n", lr);

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: got %d, expected 0\n", lr);
}

static void test_key_creation_and_deletion(void)
{
    HKEY hkey, subkey;
    LONG lr;

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND, "RegDeleteKeyA failed: %d\n", lr);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n");

    lr = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: got %d, expected 0\n", lr);

    /* Test key creation */
    exec_import_str("REGEDIT4\n\n"
                    "HKEY_CURRENT_USER\\" KEY_BASE "\\No_Opening_Bracket]\n");
    verify_key_nonexist(hkey, "No_Opening_Bracket");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\No_Closing_Bracket\n");
    verify_key_nonexist(hkey, "No_Closing_Bracket");

    exec_import_str("REGEDIT4\n\n"
                    "[ HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1a]\n");
    verify_key_nonexist(hkey, "Subkey1a");

    exec_import_str("REGEDIT4\n\n"
                    "[\tHKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1b]\n");
    verify_key_nonexist(hkey, "Subkey1b");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1c ]\n");
    verify_key(hkey, "Subkey1c ");
    lr = RegDeleteKeyA(hkey, "Subkey1c ");
    ok(lr == ERROR_SUCCESS, "got %d, expected 0\n", lr);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1d\t]\n");
    verify_key(hkey, "Subkey1d\t");
    lr = RegDeleteKeyA(hkey, "Subkey1d\t");
    ok(lr == ERROR_SUCCESS, "got %d, expected 0\n", lr);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1e\\]\n"
                    "\"Wine\"=\"Test value\"\n\n");
    verify_key(hkey, "Subkey1e\\");
    verify_key(hkey, "Subkey1e");
    lr = RegOpenKeyExA(hkey, "Subkey1e", 0, KEY_READ, &subkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: got %u, expected 0\n", lr);
    verify_reg(subkey, "Wine", REG_SZ, "Test value", 11, 0);
    RegCloseKey(subkey);
    lr = RegDeleteKeyA(hkey, "Subkey1e");
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: got %u, expected 0\n", lr);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1f\\\\]\n"
                    "\"Wine\"=\"Test value\"\n\n");
    verify_key(hkey, "Subkey1f\\\\");
    verify_key(hkey, "Subkey1f\\");
    verify_key(hkey, "Subkey1f");
    lr = RegOpenKeyExA(hkey, "Subkey1f\\\\", 0, KEY_READ, &subkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: got %u, expected 0\n", lr);
    verify_reg(subkey, "Wine", REG_SZ, "Test value", 11, 0);
    RegCloseKey(subkey);
    lr = RegDeleteKeyA(hkey, "Subkey1f\\\\");
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: got %u, expected 0\n", lr);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1g\\\\\\\\]\n"
                    "\"Wine\"=\"Test value\"\n\n");
    verify_key(hkey, "Subkey1g\\\\\\\\");
    verify_key(hkey, "Subkey1g\\\\");
    verify_key(hkey, "Subkey1g\\");
    verify_key(hkey, "Subkey1g");
    lr = RegOpenKeyExA(hkey, "Subkey1g\\\\", 0, KEY_READ, &subkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: got %u, expected 0\n", lr);
    verify_reg(subkey, "Wine", REG_SZ, "Test value", 11, 0);
    RegCloseKey(subkey);
    lr = RegDeleteKeyA(hkey, "Subkey1g\\\\");
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: got %u, expected 0\n", lr);

    /* Test key deletion. We start by creating some registry keys. */
    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2b]\n\n");
    verify_key(hkey, "Subkey2a");
    verify_key(hkey, "Subkey2b");

    exec_import_str("REGEDIT4\n\n"
                    "[ -HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\n");
    verify_key(hkey, "Subkey2a");

    exec_import_str("REGEDIT4\n\n"
                    "[\t-HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2b]\n");
    verify_key(hkey, "Subkey2b");

    exec_import_str("REGEDIT4\n\n"
                    "[- HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\n");
    verify_key(hkey, "Subkey2a");

    exec_import_str("REGEDIT4\n\n"
                    "[-\tHKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2b]\n");
    verify_key(hkey, "Subkey2b");

    exec_import_str("REGEDIT4\n\n"
                    "[-HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\n\n"
                    "[-HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2b]\n\n");
    verify_key_nonexist(hkey, "Subkey2a");
    verify_key_nonexist(hkey, "Subkey2b");

    /* Test case sensitivity when creating and deleting registry keys. */
    exec_import_str("REGEDIT4\n\n"
                    "[hkey_CURRENT_user\\" KEY_BASE "\\Subkey3a]\n\n"
                    "[HkEy_CuRrEnT_uSeR\\" KEY_BASE "\\SuBkEy3b]\n\n");
    verify_key(hkey, "Subkey3a");
    verify_key(hkey, "Subkey3b");

    exec_import_str("REGEDIT4\n\n"
                    "[-HKEY_current_USER\\" KEY_BASE "\\sUBKEY3A]\n\n"
                    "[-hKeY_cUrReNt_UsEr\\" KEY_BASE "\\sUbKeY3B]\n\n");
    verify_key_nonexist(hkey, "Subkey3a");
    verify_key_nonexist(hkey, "Subkey3b");

    /* Test mixed key creation and deletion. We start by creating a subkey. */
    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey4a]\n\n");
    verify_key(hkey, "Subkey4a");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n"
                    "[-HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey4a]\n"
                    "\"Wine1a\"=dword:12345678\n\n");
    verify_key_nonexist(hkey, "Subkey4a");
    verify_reg_nonexist(hkey, "Wine1a");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n"
                    "[HKEY_CURRENT_USERS\\" KEY_BASE "\\Subkey4b]\n"
                    "\"Wine1b\"=dword:12345678\n\n");
    verify_key_nonexist(hkey, "Subkey4b");
    verify_reg_nonexist(hkey, "Wine1b");

    lr = RegCloseKey(hkey);
    ok(lr == ERROR_SUCCESS, "RegCloseKey failed: got %d, expected 0\n", lr);

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: got %d, expected 0\n", lr);
}

static void test_key_creation_and_deletion_unicode(void)
{
    HKEY hkey, subkey;
    LONG lr;

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND, "RegDeleteKeyA failed: %d\n", lr);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n");

    lr = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: got %d, expected 0\n", lr);

    /* Test key creation */
    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "HKEY_CURRENT_USER\\" KEY_BASE "\\No_Opening_Bracket]\n");
    verify_key_nonexist(hkey, "No_Opening_Bracket");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\No_Closing_Bracket\n");
    verify_key_nonexist(hkey, "No_Closing_Bracket");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[ HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1a]\n");
    verify_key_nonexist(hkey, "Subkey1a");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[\tHKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1b]\n");
    verify_key_nonexist(hkey, "Subkey1b");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1c ]\n");
    verify_key(hkey, "Subkey1c ");
    lr = RegDeleteKeyA(hkey, "Subkey1c ");
    ok(lr == ERROR_SUCCESS, "got %d, expected 0\n", lr);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1d\t]\n");
    verify_key(hkey, "Subkey1d\t");
    lr = RegDeleteKeyA(hkey, "Subkey1d\t");
    ok(lr == ERROR_SUCCESS, "got %d, expected 0\n", lr);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1e\\]\n"
                     "\"Wine\"=\"Test value\"\n\n");
    verify_key(hkey, "Subkey1e\\");
    verify_key(hkey, "Subkey1e");
    lr = RegOpenKeyExA(hkey, "Subkey1e", 0, KEY_READ, &subkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: got %u, expected 0\n", lr);
    verify_reg(subkey, "Wine", REG_SZ, "Test value", 11, 0);
    RegCloseKey(subkey);
    lr = RegDeleteKeyA(hkey, "Subkey1e");
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: got %u, expected 0\n", lr);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1f\\\\]\n"
                     "\"Wine\"=\"Test value\"\n\n");
    verify_key(hkey, "Subkey1f\\\\");
    verify_key(hkey, "Subkey1f\\");
    verify_key(hkey, "Subkey1f");
    lr = RegOpenKeyExA(hkey, "Subkey1f\\\\", 0, KEY_READ, &subkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: got %u, expected 0\n", lr);
    verify_reg(subkey, "Wine", REG_SZ, "Test value", 11, 0);
    RegCloseKey(subkey);
    lr = RegDeleteKeyA(hkey, "Subkey1f\\\\");
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: got %u, expected 0\n", lr);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1g\\\\\\\\]\n"
                     "\"Wine\"=\"Test value\"\n\n");
    verify_key(hkey, "Subkey1g\\\\\\\\");
    verify_key(hkey, "Subkey1g\\\\");
    verify_key(hkey, "Subkey1g\\");
    verify_key(hkey, "Subkey1g");
    lr = RegOpenKeyExA(hkey, "Subkey1g\\\\", 0, KEY_READ, &subkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: got %u, expected 0\n", lr);
    verify_reg(subkey, "Wine", REG_SZ, "Test value", 11, 0);
    RegCloseKey(subkey);
    lr = RegDeleteKeyA(hkey, "Subkey1g\\\\");
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: got %u, expected 0\n", lr);

    /* Test key deletion. We start by creating some registry keys. */
    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2b]\n\n");
    verify_key(hkey, "Subkey2a");
    verify_key(hkey, "Subkey2b");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[ -HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\n");
    verify_key(hkey, "Subkey2a");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[\t-HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2b]\n");
    verify_key(hkey, "Subkey2b");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[- HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\n");
    verify_key(hkey, "Subkey2a");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[-\tHKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2b]\n");
    verify_key(hkey, "Subkey2b");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[-HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\n\n"
                     "[-HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2b]\n\n");
    verify_key_nonexist(hkey, "Subkey2a");
    verify_key_nonexist(hkey, "Subkey2b");

    /* Test case sensitivity when creating and deleting registry keys. */
    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[hkey_CURRENT_user\\" KEY_BASE "\\Subkey3a]\n\n"
                     "[HkEy_CuRrEnT_uSeR\\" KEY_BASE "\\SuBkEy3b]\n\n");
    verify_key(hkey, "Subkey3a");
    verify_key(hkey, "Subkey3b");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[-HKEY_current_USER\\" KEY_BASE "\\sUBKEY3A]\n\n"
                     "[-hKeY_cUrReNt_UsEr\\" KEY_BASE "\\sUbKeY3B]\n\n");
    verify_key_nonexist(hkey, "Subkey3a");
    verify_key_nonexist(hkey, "Subkey3b");

    /* Test mixed key creation and deletion. We start by creating a subkey. */
    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey4a]\n\n");
    verify_key(hkey, "Subkey4a");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n"
                     "[-HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey4a]\n"
                     "\"Wine1a\"=dword:12345678\n\n");
    verify_key_nonexist(hkey, "Subkey4a");
    verify_reg_nonexist(hkey, "Wine1a");

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n"
                     "[HKEY_CURRENT_USERS\\" KEY_BASE "\\Subkey4b]\n"
                     "\"Wine1b\"=dword:12345678\n\n");
    verify_key_nonexist(hkey, "Subkey4b");
    verify_reg_nonexist(hkey, "Wine1b");

    lr = RegCloseKey(hkey);
    ok(lr == ERROR_SUCCESS, "RegCloseKey failed: got %d, expected 0\n", lr);

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: got %d, expected 0\n", lr);
}

static void test_value_deletion(void)
{
    HKEY hkey;
    LONG lr;
    DWORD dword = 0x8;
    BYTE hex[4] = {0x11, 0x22, 0x33, 0x44};

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND, "RegDeleteKeyA failed: %d\n", lr);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n");

    lr = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: got %d, expected 0\n", lr);

    /* Test value deletion. We start by creating some registry values. */
    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine46a\"=\"Test Value\"\n"
                    "\"Wine46b\"=dword:00000008\n"
                    "\"Wine46c\"=hex:11,22,33,44\n"
                    "\"Wine46d\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine46e\"=hex(2):25,50,41,54,48,25,00\n"
                    "\"Wine46f\"=hex(0):56,00,61,00,6c,00,75,00,65,00,00,00\n\n");
    verify_reg(hkey, "Wine46a", REG_SZ, "Test Value", 11, 0);
    verify_reg(hkey, "Wine46b", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine46c", REG_BINARY, hex, 4, 0);
    verify_reg(hkey, "Wine46d", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine46e", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine46f", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine46a\"=-\n"
                    "\"Wine46b\"=  -\n"
                    "\"Wine46c\"=  \t-\t  \n"
                    "\"Wine46d\"=-\"Test\"\n"
                    "\"Wine46e\"=- ;comment\n"
                    "\"Wine46f\"=- #comment\n\n");
    verify_reg_nonexist(hkey, "Wine46a");
    verify_reg_nonexist(hkey, "Wine46b");
    verify_reg_nonexist(hkey, "Wine46c");
    verify_reg(hkey, "Wine46d", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg_nonexist(hkey, "Wine46e");
    verify_reg(hkey, "Wine46f", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);

    lr = RegCloseKey(hkey);
    ok(lr == ERROR_SUCCESS, "RegCloseKey failed: got %d, expected 0\n", lr);

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: got %d, expected 0\n", lr);
}


static void test_value_deletion_unicode(void)
{
    HKEY hkey;
    LONG lr;
    DWORD dword = 0x8;
    BYTE hex[4] = {0x11, 0x22, 0x33, 0x44};

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND, "RegDeleteKeyA failed: %d\n", lr);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n");

    lr = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: got %d, expected 0\n", lr);

    /* Test value deletion. We start by creating some registry values. */
    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine46a\"=\"Test Value\"\n"
                     "\"Wine46b\"=dword:00000008\n"
                     "\"Wine46c\"=hex:11,22,33,44\n"
                     "\"Wine46d\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "\"Wine46e\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine46f\"=hex(0):56,00,61,00,6c,00,75,00,65,00,00,00\n\n");
    verify_reg(hkey, "Wine46a", REG_SZ, "Test Value", 11, 0);
    verify_reg(hkey, "Wine46b", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine46c", REG_BINARY, hex, 4, 0);
    verify_reg(hkey, "Wine46d", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine46e", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine46f", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);

    exec_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine46a\"=-\n"
                     "\"Wine46b\"=  -\n"
                     "\"Wine46c\"=  \t-\t  \n"
                     "\"Wine46d\"=-\"Test\"\n"
                     "\"Wine46e\"=- ;comment\n"
                     "\"Wine46f\"=- #comment\n\n");
    verify_reg_nonexist(hkey, "Wine46a");
    verify_reg_nonexist(hkey, "Wine46b");
    verify_reg_nonexist(hkey, "Wine46c");
    verify_reg(hkey, "Wine46d", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg_nonexist(hkey, "Wine46e");
    verify_reg(hkey, "Wine46f", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);

    lr = RegCloseKey(hkey);
    ok(lr == ERROR_SUCCESS, "RegCloseKey failed: got %d, expected 0\n", lr);

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: got %d, expected 0\n", lr);
}

START_TEST(regedit)
{
    if(!exec_import_str("REGEDIT4\r\n\r\n")){
        win_skip("regedit not available, skipping regedit tests\n");
        return;
    }

    test_basic_import();
    test_basic_import_unicode();
    test_basic_import_31();
    test_invalid_import();
    test_invalid_import_unicode();
    test_invalid_import_31();
    test_comments();
    test_comments_unicode();
    test_import_with_whitespace();
    test_import_with_whitespace_unicode();
    test_key_creation_and_deletion();
    test_key_creation_and_deletion_unicode();
    test_value_deletion();
    test_value_deletion_unicode();
}
