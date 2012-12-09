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

static BOOL supports_wchar;

#define lok ok_(__FILE__,line)

#define exec_import_str(c) r_exec_import_str(__LINE__, c)
static BOOL r_exec_import_str(unsigned line, const char *file_contents)
{
    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION pi;
    HANDLE regfile;
    DWORD written, dr;
    BOOL br;
    char cmd[] = "regedit /s test.reg";

    regfile = CreateFileA("test.reg", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    lok(regfile != INVALID_HANDLE_VALUE, "Failed to create test.reg file\n");
    if(regfile == INVALID_HANDLE_VALUE)
        return FALSE;

    br = WriteFile(regfile, file_contents, strlen(file_contents), &written,
            NULL);
    lok(br == TRUE, "WriteFile failed: %d\n", GetLastError());

    CloseHandle(regfile);

    if(!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        return FALSE;

    dr = WaitForSingleObject(pi.hProcess, 10000);
    if(dr == WAIT_TIMEOUT)
        TerminateProcess(pi.hProcess, 1);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    br = DeleteFileA("test.reg");
    lok(br == TRUE, "DeleteFileA failed: %d\n", GetLastError());

    return (dr != WAIT_TIMEOUT);
}

#define exec_import_wstr(c) r_exec_import_wstr(__LINE__, c)
static BOOL r_exec_import_wstr(unsigned line, const WCHAR *file_contents)
{
    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION pi;
    HANDLE regfile;
    DWORD written, dr;
    BOOL br;
    char cmd[] = "regedit /s test.reg";

    regfile = CreateFileA("test.reg", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    lok(regfile != INVALID_HANDLE_VALUE, "Failed to create test.reg file\n");
    if(regfile == INVALID_HANDLE_VALUE)
        return FALSE;

    br = WriteFile(regfile, file_contents,
            lstrlenW(file_contents) * sizeof(WCHAR), &written, NULL);
    lok(br == TRUE, "WriteFile failed: %d\n", GetLastError());

    CloseHandle(regfile);

    if(!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        return FALSE;

    dr = WaitForSingleObject(pi.hProcess, 10000);
    if(dr == WAIT_TIMEOUT)
        TerminateProcess(pi.hProcess, 1);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    br = DeleteFileA("test.reg");
    lok(br == TRUE, "DeleteFileA failed: %d\n", GetLastError());

    return (dr != WAIT_TIMEOUT);
}

#define verify_reg_sz(k,s,n,e) r_verify_reg_sz(__LINE__,k,s,n,e)
static void r_verify_reg_sz(unsigned line, HKEY key, const char *subkey,
        const char *value_name, const char *exp_value)
{
    LONG lr;
    DWORD fnd_type, fnd_len;
    char fnd_value[1024];
    HKEY fnd_key;

    lr = RegOpenKeyExA(key, subkey, 0, KEY_READ, &fnd_key);
    lok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: %d\n", lr);
    if(lr != ERROR_SUCCESS)
        return;

    fnd_len = sizeof(fnd_value);
    lr = RegQueryValueExA(fnd_key, value_name, NULL, &fnd_type,
            (BYTE*)fnd_value, &fnd_len);
    RegCloseKey(fnd_key);
    lok(lr == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", lr);
    if(lr != ERROR_SUCCESS)
        return;

    lok(fnd_type == REG_SZ, "Got wrong type: %d\n", fnd_type);
    if(fnd_type != REG_SZ)
        return;
    lok(!strcmp(exp_value, fnd_value),
            "Strings differ: expected %s, got %s\n", exp_value, fnd_value);
}

#define verify_reg_wsz(k,s,n,e) r_verify_reg_wsz(__LINE__,k,s,n,e)
static void r_verify_reg_wsz(unsigned line, HKEY key, const char *subkey,
        const char *value_name, const WCHAR *exp_value)
{
    LONG lr;
    DWORD fnd_type, fnd_len;
    WCHAR fnd_value[1024], value_nameW[1024];
    HKEY fnd_key;

    lr = RegOpenKeyExA(key, subkey, 0, KEY_READ, &fnd_key);
    lok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: %d\n", lr);
    if(lr != ERROR_SUCCESS)
        return;

    MultiByteToWideChar(CP_ACP, 0, value_name, -1, value_nameW,
            sizeof(value_nameW)/sizeof(value_nameW[0]));

    fnd_len = sizeof(fnd_value);
    lr = RegQueryValueExW(fnd_key, value_nameW, NULL, &fnd_type,
            (BYTE*)fnd_value, &fnd_len);
    RegCloseKey(fnd_key);
    lok(lr == ERROR_SUCCESS, "RegQueryValueExW failed: %d\n", lr);
    if(lr != ERROR_SUCCESS)
        return;

    lok(fnd_type == REG_SZ, "Got wrong type: %d\n", fnd_type);
    if(fnd_type != REG_SZ)
        return;
    lok(!lstrcmpW(exp_value, fnd_value),
            "Strings differ: expected %s, got %s\n",
            wine_dbgstr_w(exp_value), wine_dbgstr_w(fnd_value));
}

#define verify_reg_dword(k,s,n,e) r_verify_reg_dword(__LINE__,k,s,n,e)
static void r_verify_reg_dword(unsigned line, HKEY key, const char *subkey,
        const char *value_name, DWORD exp_value)
{
    LONG lr;
    DWORD fnd_type, fnd_len, fnd_value;
    HKEY fnd_key;

    lr = RegOpenKeyExA(key, subkey, 0, KEY_READ, &fnd_key);
    lok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: %d\n", lr);
    if(lr != ERROR_SUCCESS)
        return;

    fnd_len = sizeof(fnd_value);
    lr = RegQueryValueExA(fnd_key, value_name, NULL, &fnd_type,
            (BYTE *)&fnd_value, &fnd_len);
    RegCloseKey(fnd_key);
    lok(lr == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", lr);
    if(lr != ERROR_SUCCESS)
        return;

    lok(fnd_type == REG_DWORD, "Got wrong type: %d\n", fnd_type);
    if(fnd_type != REG_DWORD)
        return;
    lok(fnd_value == exp_value, "Values differ: expected: 0x%x, got: 0x%x\n",
            exp_value, fnd_value);
}

#define verify_reg_binary(k,s,n,e,z) r_verify_reg_binary(__LINE__,k,s,n,e,z)
static void r_verify_reg_binary(unsigned line, HKEY key, const char *subkey,
        const char *value_name, const char *exp_value, int exp_len)
{
    LONG lr;
    DWORD fnd_type, fnd_len;
    char fnd_value[1024];
    HKEY fnd_key;

    lr = RegOpenKeyExA(key, subkey, 0, KEY_READ, &fnd_key);
    lok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: %d\n", lr);
    if(lr != ERROR_SUCCESS)
        return;

    fnd_len = sizeof(fnd_value);
    lr = RegQueryValueExA(fnd_key, value_name, NULL, &fnd_type,
            (BYTE*)fnd_value, &fnd_len);
    RegCloseKey(fnd_key);
    lok(lr == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", lr);
    if(lr != ERROR_SUCCESS)
        return;

    lok(fnd_type == REG_BINARY, "Got wrong type: %d\n", fnd_type);
    if(fnd_type != REG_BINARY)
        return;
    lok(fnd_len == exp_len,
            "Lengths differ: expected %d, got %d\n", exp_len, fnd_len);
    lok(!memcmp(exp_value, fnd_value, exp_len),
            "Data differs\n");
}

#define verify_reg_nonexist(k,s,n) r_verify_reg_nonexist(__LINE__,k,s,n)
static void r_verify_reg_nonexist(unsigned line, HKEY key, const char *subkey,
        const char *value_name)
{
    LONG lr;
    DWORD fnd_type, fnd_len;
    char fnd_value[32];
    HKEY fnd_key;

    lr = RegOpenKeyExA(key, subkey, 0, KEY_READ, &fnd_key);
    lok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: %d\n", lr);
    if(lr != ERROR_SUCCESS)
        return;

    fnd_len = sizeof(fnd_value);
    lr = RegQueryValueExA(fnd_key, value_name, NULL, &fnd_type,
            (BYTE*)fnd_value, &fnd_len);
    RegCloseKey(fnd_key);
    lok(lr == ERROR_FILE_NOT_FOUND, "Reg value shouldn't exist: %s\n",
            value_name);
}

static void test_basic_import(void)
{
    char exp_binary[] = {0xAA,0xBB,0xCC,0x11};
    WCHAR wide_test[] = {0xFEFF,'W','i','n','d','o','w','s',' ','R','e','g',
        'i','s','t','r','y',' ','E','d','i','t','o','r',' ','V','e','r','s',
        'i','o','n',' ','5','.','0','0','\n','\n',
        '[','H','K','E','Y','_','C','U','R','R','E','N','T','_','U','S','E',
        'R','\\','S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
        'r','e','g','e','d','i','t','_','t','e','s','t',']','\n',
        '"','T','e','s','t','V','a','l','u','e','3','"','=','"',0x3041,'V','a',
        'l','u','e','"','\n',0};
    WCHAR wide_test_r[] = {0xFEFF,'W','i','n','d','o','w','s',' ','R','e','g',
        'i','s','t','r','y',' ','E','d','i','t','o','r',' ','V','e','r','s',
        'i','o','n',' ','5','.','0','0','\r','\r',
        '[','H','K','E','Y','_','C','U','R','R','E','N','T','_','U','S','E',
        'R','\\','S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
        'r','e','g','e','d','i','t','_','t','e','s','t',']','\r',
        '"','T','e','s','t','V','a','l','u','e','5','"','=','"',0x3041,'V','a',
        'l','u','e','"','\r',0};
    WCHAR wide_exp[] = {0x3041,'V','a','l','u','e',0};
    LONG lr;

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test");
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND,
            "RegDeleteKeyA failed: %d\n", lr);

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\Software\\Wine\\regedit_test]\n"
                "\"TestValue\"=\"AValue\"\n");
    verify_reg_sz(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test",
            "TestValue", "AValue");

    exec_import_str("REGEDIT4\r\n\r\n"
                "[HKEY_CURRENT_USER\\Software\\Wine\\regedit_test]\r\n"
                "\"TestValue2\"=\"BValue\"\r\n");
    verify_reg_sz(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test",
            "TestValue2", "BValue");

    if(supports_wchar){
        exec_import_wstr(wide_test);
        verify_reg_wsz(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test",
                "TestValue3", wide_exp);

        exec_import_wstr(wide_test_r);
        verify_reg_wsz(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test",
                "TestValue5", wide_exp);
    }else
        win_skip("Some WCHAR tests skipped\n");

    exec_import_str("REGEDIT4\r\r"
                "[HKEY_CURRENT_USER\\Software\\Wine\\regedit_test]\r"
                "\"TestValue4\"=\"DValue\"\r");
    verify_reg_sz(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test",
            "TestValue4", "DValue");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\Software\\Wine\\regedit_test]\n"
                "\"TestDword\"=dword:00000017\n");
    verify_reg_dword(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test",
            "TestDword", 0x17);

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\Software\\Wine\\regedit_test]\n"
                "\"TestBinary\"=hex:aa,bb,cc,11\n");
    verify_reg_binary(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test",
            "TestBinary", exp_binary, sizeof(exp_binary));

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\Software\\Wine\\regedit_test]\n"
                "\"With=Equals\"=\"asdf\"\n");
    verify_reg_sz(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test",
            "With=Equals", "asdf");

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test");
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: %d\n", lr);
}

static void test_invalid_import(void)
{
    LONG lr;

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test");
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND,
            "RegDeleteKeyA failed: %d\n", lr);

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\Software\\Wine\\regedit_test]\n"
                "\"TestNoEndQuote\"=\"Asdffdsa\n");
    verify_reg_nonexist(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test",
            "TestNoEndQuote");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\Software\\Wine\\regedit_test]\n"
                "\"TestNoBeginQuote\"=Asdffdsa\"\n");
    verify_reg_nonexist(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test",
            "TestNoBeginQuote");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\Software\\Wine\\regedit_test]\n"
                "\"TestNoQuotes\"=Asdffdsa\n");
    verify_reg_nonexist(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test",
            "TestNoQuotes");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\Software\\Wine\\regedit_test]\n"
                "\"NameNoEndQuote=\"Asdffdsa\"\n");
    verify_reg_nonexist(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test",
            "NameNoEndQuote");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\Software\\Wine\\regedit_test]\n"
                "NameNoBeginQuote\"=\"Asdffdsa\"\n");
    verify_reg_nonexist(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test",
            "NameNoBeginQuote");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\Software\\Wine\\regedit_test]\n"
                "NameNoQuotes=\"Asdffdsa\"\n");
    verify_reg_nonexist(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test",
            "NameNoQuotes");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\Software\\Wine\\regedit_test]\n"
                "\"MixedQuotes=Asdffdsa\"\n");
    verify_reg_nonexist(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test",
            "MixedQuotes");
    verify_reg_nonexist(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test",
            "MixedQuotes=Asdffdsa");

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Wine\\regedit_test");
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: %d\n", lr);
}

START_TEST(regedit)
{
    WCHAR wchar_test[] = {0xFEFF,'W','i','n','d','o','w','s',' ','R','e','g',
        'i','s','t','r','y',' ','E','d','i','t','o','r',' ','V','e','r','s',
        'i','o','n',' ','5','.','0','0','\n','\n',0};

    if(!exec_import_str("REGEDIT4\r\n\r\n")){
        win_skip("regedit not available, skipping regedit tests\n");
        return;
    }

    supports_wchar = exec_import_wstr(wchar_test);

    test_basic_import();
    test_invalid_import();
}
