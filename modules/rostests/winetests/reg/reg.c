/*
 * Copyright 2014 Akihiro Sagawa
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

#include <stdio.h>
#include <windows.h>
#include "wine/test.h"

#define lok ok_(__FILE__,line)
#define KEY_WINE "Software\\Wine"
#define KEY_BASE KEY_WINE "\\reg_test"
#define REG_EXIT_SUCCESS 0
#define REG_EXIT_FAILURE 1
#define TODO_REG_TYPE    (0x0001u)
#define TODO_REG_SIZE    (0x0002u)
#define TODO_REG_DATA    (0x0004u)
#define TODO_REG_COMPARE (0x0008u)

#define run_reg_exe(c,r) run_reg_exe_(__LINE__,c,r)
static BOOL run_reg_exe_(unsigned line, const char *cmd, DWORD *rc)
{
    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION pi;
    BOOL bret;
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

    bret = GetExitCodeProcess(pi.hProcess, rc);
    lok(bret, "GetExitCodeProcess failed: %d\n", GetLastError());

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return bret;
}

#define verify_reg(k,v,t,d,s,todo) verify_reg_(__LINE__,k,v,t,d,s,todo)
static void verify_reg_(unsigned line, HKEY hkey, const char* value,
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
    if (exp_data)
    {
        todo_wine_if (todo & TODO_REG_DATA)
            lok(memcmp(data, exp_data, size) == 0, "got wrong data\n");
    }
}

#define verify_reg_nonexist(k,v) verify_reg_nonexist_(__LINE__,k,v)
static void verify_reg_nonexist_(unsigned line, HKEY hkey, const char *value)
{
    LONG err;

    err = RegQueryValueExA(hkey, value, NULL, NULL, NULL, NULL);
    lok(err == ERROR_FILE_NOT_FOUND, "registry value '%s' shouldn't exist; got %d, expected 2\n",
        (value && *value) ? value : "(Default)", err);
}

#define open_key(b,p,s,k) open_key_(__LINE__,b,p,s,k)
static void open_key_(unsigned line, const HKEY base, const char *path, const DWORD sam, HKEY *hkey)
{
    LONG err;

    err = RegOpenKeyExA(base, path, 0, KEY_READ|sam, hkey);
    lok(err == ERROR_SUCCESS, "RegOpenKeyExA failed: %d\n", err);
}

#define verify_key(k,s) verify_key_(__LINE__,k,s)
static void verify_key_(unsigned line, HKEY key_base, const char *subkey)
{
    HKEY hkey;
    LONG err;

    err = RegOpenKeyExA(key_base, subkey, 0, KEY_READ, &hkey);
    lok(err == ERROR_SUCCESS, "RegOpenKeyExA failed: got %d\n", err);

    if (hkey)
        RegCloseKey(hkey);
}

#define verify_key_nonexist(k,s) verify_key_nonexist_(__LINE__,k,s)
static void verify_key_nonexist_(unsigned line, HKEY key_base, const char *subkey)
{
    HKEY hkey;
    LONG err;

    err = RegOpenKeyExA(key_base, subkey, 0, KEY_READ, &hkey);
    lok(err == ERROR_FILE_NOT_FOUND, "registry key '%s' shouldn't exist; got %d, expected 2\n",
        subkey, err);

    if (hkey)
        RegCloseKey(hkey);
}

#define add_key(k,p,s) add_key_(__LINE__,k,p,s)
static void add_key_(unsigned line, const HKEY hkey, const char *path, HKEY *subkey)
{
    LONG err;

    err = RegCreateKeyExA(hkey, path, 0, NULL, REG_OPTION_NON_VOLATILE,
                          KEY_READ|KEY_WRITE, NULL, subkey, NULL);
    lok(err == ERROR_SUCCESS, "RegCreateKeyExA failed: %d\n", err);
}

#define delete_key(k,p) delete_key_(__LINE__,k,p)
static void delete_key_(unsigned line, const HKEY hkey, const char *path)
{
    if (path && *path)
    {
        LONG err;

        err = RegDeleteKeyA(hkey, path);
        lok(err == ERROR_SUCCESS, "RegDeleteKeyA failed: %d\n", err);
    }
}

static LONG delete_tree(const HKEY key, const char *subkey)
{
    HKEY hkey;
    LONG ret;
    char *subkey_name = NULL;
    DWORD max_subkey_len, subkey_len;
    static const char empty[1];

    ret = RegOpenKeyExA(key, subkey, 0, KEY_READ, &hkey);
    if (ret) return ret;

    ret = RegQueryInfoKeyA(hkey, NULL, NULL, NULL, NULL, &max_subkey_len,
                           NULL, NULL, NULL, NULL, NULL, NULL);
    if (ret) goto cleanup;

    max_subkey_len++;

    subkey_name = HeapAlloc(GetProcessHeap(), 0, max_subkey_len);
    if (!subkey_name)
    {
        ret = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    for (;;)
    {
        subkey_len = max_subkey_len;
        ret = RegEnumKeyExA(hkey, 0, subkey_name, &subkey_len, NULL, NULL, NULL, NULL);
        if (ret == ERROR_NO_MORE_ITEMS) break;
        if (ret) goto cleanup;
        ret = delete_tree(hkey, subkey_name);
        if (ret) goto cleanup;
    }

    ret = RegDeleteKeyA(hkey, empty);

cleanup:
    HeapFree(GetProcessHeap(), 0, subkey_name);
    RegCloseKey(hkey);
    return ret;
}

#define add_value(k,n,t,d,s) add_value_(__LINE__,k,n,t,d,s)
static void add_value_(unsigned line, HKEY hkey, const char *name, DWORD type,
                       const void *data, size_t size)
{
    LONG err;

    err = RegSetValueExA(hkey, name, 0, type, (const BYTE *)data, size);
    lok(err == ERROR_SUCCESS, "RegSetValueExA failed: %d\n", err);
}

#define delete_value(k,n) delete_value_(__LINE__,k,n)
static void delete_value_(unsigned line, const HKEY hkey, const char *name)
{
    LONG err;

    err = RegDeleteValueA(hkey, name);
    lok(err == ERROR_SUCCESS, "RegDeleteValueA failed: %d\n", err);
}

static void test_add(void)
{
    HKEY hkey;
    LONG err;
    DWORD r, dword, type, size;
    char buffer[22];

    run_reg_exe("reg add", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add /?", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg add /h", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg add -H", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(err == ERROR_SUCCESS || err == ERROR_FILE_NOT_FOUND, "got %d\n", err);

    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    open_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);

    /* Test empty type */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v emptyType /t \"\" /d WineTest /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u\n", r);

    /* Test input key formats */
    run_reg_exe("reg add \\HKCU\\" KEY_BASE "\\keytest0 /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u\n", r);
    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE "\\keytest0");
    ok(err == ERROR_FILE_NOT_FOUND, "got exit code %d\n", err);

    run_reg_exe("reg add \\\\HKCU\\" KEY_BASE "\\keytest1 /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u\n", r);
    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE "\\keytest1");
    ok(err == ERROR_FILE_NOT_FOUND, "got exit code %d\n", err);

    run_reg_exe("reg add HKCU\\" KEY_BASE "\\keytest2\\\\ /f", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */),
        "got exit code %u\n", r);
    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE "\\keytest2");
    ok(err == ERROR_FILE_NOT_FOUND || broken(err == ERROR_SUCCESS /* WinXP */),
        "got exit code %d\n", err);

    run_reg_exe("reg add HKCU\\" KEY_BASE "\\keytest3\\ /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u\n", r);
    verify_key(hkey, "keytest3");
    delete_key(hkey, "keytest3");

    run_reg_exe("reg add HKCU\\" KEY_BASE "\\keytest4 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u\n", r);
    verify_key(hkey, "keytest4");
    delete_key(hkey, "keytest4");

    /* REG_NONE */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v none0 /d deadbeef /t REG_NONE /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d\n", r);
    verify_reg(hkey, "none0", REG_NONE, "d\0e\0a\0d\0b\0e\0e\0f\0\0", 18, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v none1 /t REG_NONE /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "none1", REG_NONE, "\0", 2, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /t REG_NONE /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, NULL, REG_NONE, "\0", 2, 0);

    /* REG_SZ */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /d WineTest /f", &r);
    ok(r == REG_EXIT_SUCCESS || broken(r == REG_EXIT_FAILURE /* WinXP */),
       "got exit code %d, expected 0\n", r);
    if (r == REG_EXIT_SUCCESS)
        verify_reg(hkey, "", REG_SZ, "WineTest", 9, 0);
    else
        win_skip("broken reg.exe detected\n");

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v test /d deadbeef /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "test", REG_SZ, "deadbeef", 9, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v test /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "test", REG_SZ, "", 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v test1 /t REG_SZ /f /d", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKEY_CURRENT_USER\\" KEY_BASE " /ve /d WineTEST /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "", REG_SZ, "WineTEST", 9, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_SZ /v test2 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "test2", REG_SZ, "", 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_SZ /v test3 /f /d \"\"", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "test3", REG_SZ, "", 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, NULL, REG_SZ, "", 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /t REG_SZ /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, NULL, REG_SZ, "", 1, 0);

    /* REG_EXPAND_SZ */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v expand0 /t REG_EXpand_sz /d \"dead%PATH%beef\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u\n", r);
    verify_reg(hkey, "expand0", REG_EXPAND_SZ, "dead%PATH%beef", 15, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v expand1 /t REG_EXpand_sz /d \"dead^%PATH^%beef\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u\n", r);
    verify_reg(hkey, "expand1", REG_EXPAND_SZ, "dead^%PATH^%beef", 17, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_EXPAND_SZ /v expand2 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u\n", r);
    verify_reg(hkey, "expand2", REG_EXPAND_SZ, "", 1, 0);

    run_reg_exe("reg add HKEY_CURRENT_USER\\" KEY_BASE " /ve /t REG_EXPAND_SZ /d WineTEST /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u\n", r);
    verify_reg(hkey, "", REG_EXPAND_SZ, "WineTEST", 9, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_EXPAND_SZ /v expand3 /f /d \"\"", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u\n", r);
    verify_reg(hkey, "expand3", REG_EXPAND_SZ, "", 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /t REG_EXPAND_SZ /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, NULL, REG_EXPAND_SZ, "", 1, 0);

    /* REG_BINARY */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_BINARY /v bin0 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u\n", r);
    verify_reg(hkey, "bin0", REG_BINARY, buffer, 0, 0);

    run_reg_exe("reg add HKEY_CURRENT_USER\\" KEY_BASE " /ve /t REG_BINARY /d deadbeef /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u\n", r);
    dword = 0xefbeadde;
    verify_reg(hkey, "", REG_BINARY, &dword, sizeof(DWORD), 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_BINARY /v bin1 /f /d 0xDeAdBeEf", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u\n", r);
    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_BINARY /v bin2 /f /d x01", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u\n", r);
    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_BINARY /v bin3 /f /d 01x", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_BINARY /v bin4 /f /d DeAdBeEf0DD", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u\n", r);
    /* Remaining nibble prefixed */
    buffer[0] = 0x0d; buffer[1] = 0xea; buffer[2] = 0xdb;
    buffer[3] = 0xee; buffer[4] = 0xf0; buffer[5] = 0xdd;
    /* Remaining nibble suffixed on winXP */
    buffer[6] = 0xde; buffer[7] = 0xad; buffer[8] = 0xbe;
    buffer[9] = 0xef; buffer[10] = 0x0d; buffer[11] = 0xd0;
    size = 6;
    err = RegQueryValueExA(hkey, "bin4", NULL, &type, (void *) (buffer+12), &size);
    ok(err == ERROR_SUCCESS, "RegQueryValueEx failed: got %d\n", err);
    ok(type == REG_BINARY, "got wrong type %u\n", type);
    ok(size == 6, "got wrong size %u\n", size);
    ok(memcmp(buffer, buffer+12, 6) == 0 ||
        broken(memcmp(buffer+6, buffer+12, 6) == 0 /* WinXP */), "got wrong data\n");

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_BINARY /v bin5 /d \"\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u\n", r);
    verify_reg(hkey, "bin5", REG_BINARY, buffer, 0, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v bin6 /t REG_BINARY /f /d", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /t REG_BINARY /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, NULL, REG_BINARY, buffer, 0, 0);

    /* REG_DWORD */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_DWORD /f /d 12345678", &r);
    ok(r == REG_EXIT_SUCCESS || broken(r == REG_EXIT_FAILURE /* WinXP */),
       "got exit code %d, expected 0\n", r);
    dword = 12345678;
    if (r == REG_EXIT_SUCCESS)
        verify_reg(hkey, "", REG_DWORD, &dword, sizeof(dword), 0);
    else
        win_skip("broken reg.exe detected\n");

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword0 /t REG_DWORD /f /d", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword1 /t REG_DWORD /f", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */),
       "got exit code %d, expected 1\n", r);
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword2 /t REG_DWORD /d zzz /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword3 /t REG_DWORD /d deadbeef /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword4 /t REG_DWORD /d 123xyz /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword5 /t reg_dword /d 12345678 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 12345678;
    verify_reg(hkey, "dword5", REG_DWORD, &dword, sizeof(dword), 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword6 /t REG_DWORD /D 0123 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    size = sizeof(dword);
    err = RegQueryValueExA(hkey, "dword6", NULL, &type, (LPBYTE)&dword, &size);
    ok(err == ERROR_SUCCESS, "RegQueryValueEx failed: got %d\n", err);
    ok(type == REG_DWORD, "got wrong type %d, expected %d\n", type, REG_DWORD);
    ok(size == sizeof(DWORD), "got wrong size %d, expected %d\n", size, (int)sizeof(DWORD));
    ok(dword == 123 || broken(dword == 0123 /* WinXP */), "got wrong data %d, expected 123\n", dword);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword7 /t reg_dword /d 0xabcdefg /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword8 /t REG_dword /d 0xdeadbeef /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0xdeadbeef;
    verify_reg(hkey, "dword8", REG_DWORD, &dword, sizeof(dword), 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_DWORD /v dword9 /f /d -1", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */), "got exit code %u\n", r);
    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_DWORD /v dword10 /f /d -0x1", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */), "got exit code %u\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword8 /t REG_dword /d 0x01ffffffff /f", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */), "got exit code %d\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword12 /t REG_DWORD /d 0xffffffff /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = ~0u;
    verify_reg(hkey, "dword12", REG_DWORD, &dword, sizeof(dword), 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword13 /t REG_DWORD /d 00x123 /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword14 /t REG_DWORD /d 0X123 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x123;
    verify_reg(hkey, "dword14", REG_DWORD, &dword, sizeof(dword), 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword15 /t REG_DWORD /d 4294967296 /f", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */), "got exit code %u\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /t REG_DWORD /f", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */), "got exit code %u\n", r);

    /* REG_DWORD_LITTLE_ENDIAN */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v DWORD_LE /t REG_DWORD_LITTLE_ENDIAN /d 456 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 456;
    verify_reg(hkey, "DWORD_LE", REG_DWORD_LITTLE_ENDIAN, &dword, sizeof(dword), 0);

    /* REG_DWORD_BIG_ENDIAN */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v DWORD_BE /t REG_DWORD_BIG_ENDIAN /d 456 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u\n", r);
    dword = 456;
    verify_reg(hkey, "DWORD_BE", REG_DWORD_BIG_ENDIAN, &dword, sizeof(dword), 0);
    /* REG_DWORD_BIG_ENDIAN is broken in every version of windows. It behaves like
     * an ordinary REG_DWORD - that is little endian. GG */

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v DWORD_BE2 /t REG_DWORD_BIG_ENDIAN /f /d", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v DWORD_BE3 /t REG_DWORD_BIG_ENDIAN /f", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */), "got exit code %u\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /t REG_DWORD_BIG_ENDIAN /f", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */), "got exit code %u\n", r);

    /* REG_MULTI_SZ */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi0 /t REG_MULTI_SZ /d \"three\\0little\\0strings\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    memcpy(buffer, "three\0little\0strings\0", 22);
    verify_reg(hkey, "multi0", REG_MULTI_SZ, buffer, 22, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi1 /s \"#\" /d \"three#little#strings\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi1", REG_MULTI_SZ, buffer, 22, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi2 /d \"\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi2", REG_MULTI_SZ, &buffer[21], 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi3 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u\n", r);
    verify_reg(hkey, "multi3", REG_MULTI_SZ, &buffer[21], 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi4 /s \"#\" /d \"threelittlestrings\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi4", REG_MULTI_SZ, "threelittlestrings\0", 20, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi5 /s \"#randomgibberish\" /d \"three#little#strings\" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u\n", r);
    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi6 /s \"\\0\" /d \"three\\0little\\0strings\" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u\n", r);
    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi7 /s \"\" /d \"three#little#strings\" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u\n", r);
    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi8 /s \"#\" /d \"##\" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u\n", r);
    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi9 /s \"#\" /d \"two##strings\" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u\n", r);
    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi10 /s \"#\" /d \"#a\" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi11 /s \"#\" /d \"a#\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    buffer[0]='a'; buffer[1]=0; buffer[2]=0;
    verify_reg(hkey, "multi11", REG_MULTI_SZ, buffer, 3, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi12 /t REG_MULTI_SZ /f /d", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi13 /t REG_MULTI_SZ /f /s", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi14 /t REG_MULTI_SZ /d \"\\0a\" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi15 /t REG_MULTI_SZ /d \"a\\0\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi15", REG_MULTI_SZ, buffer, 3, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi16 /d \"two\\0\\0strings\" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi17 /t REG_MULTI_SZ /s \"#\" /d \"#\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    buffer[0] = 0; buffer[1] = 0;
    verify_reg(hkey, "multi17", REG_MULTI_SZ, buffer, 2, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi18 /t REG_MULTI_SZ /d \"\\0\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi18", REG_MULTI_SZ, buffer, 2, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi19 /t REG_MULTI_SZ /s \"#\" /d \"two\\0#strings\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi19", REG_MULTI_SZ, "two\\0\0strings\0", 15, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi20 /t REG_MULTI_SZ /s \"#\" /d \"two#\\0strings\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi20", REG_MULTI_SZ, "two\0\\0strings\0", 15, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi21 /t REG_MULTI_SZ /s \"#\" /d \"two\\0\\0strings\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi21", REG_MULTI_SZ, "two\\0\\0strings\0", 16, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /t REG_MULTI_SZ /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, NULL, REG_MULTI_SZ, buffer, 1, 0);

    RegCloseKey(hkey);

    /* Test duplicate switches */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dup1 /t REG_DWORD /d 123 /f /t REG_SZ", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */),
       "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dup2 /t REG_DWORD /d 123 /f /d 456", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    /* Test invalid switches */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v invalid1 /a", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v invalid2 /ae", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v invalid3 /", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v invalid4 -", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    delete_key(HKEY_CURRENT_USER, KEY_BASE);
}

static void test_delete(void)
{
    HKEY hkey, hsubkey;
    LONG err;
    DWORD r;
    const DWORD deadbeef = 0xdeadbeef;

    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(err == ERROR_SUCCESS || err == ERROR_FILE_NOT_FOUND, "got %d\n", err);

    run_reg_exe("reg delete", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete /?", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg delete /h", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg delete -H", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    add_key(HKEY_CURRENT_USER, KEY_BASE, &hkey);
    add_value(hkey, "foo", REG_DWORD, &deadbeef, sizeof(deadbeef));
    add_value(hkey, "bar", REG_DWORD, &deadbeef, sizeof(deadbeef));
    add_value(hkey, NULL, REG_DWORD, &deadbeef, sizeof(deadbeef));

    add_key(hkey, "subkey", &hsubkey);
    RegCloseKey(hsubkey);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /v bar /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "bar");

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /ve /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "");

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /va /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "foo");
    verify_key(hkey, "subkey");

    RegCloseKey(hkey);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u\n", r);
}

static void test_query(void)
{
    DWORD r;
    HKEY key, subkey;
    LONG err;
    const char hello[] = "Hello";
    const char world[] = "World";
    const char empty1[] = "Empty1";
    const char empty2[] = "Empty2";
    const DWORD dword1 = 0x123;
    const DWORD dword2 = 0xabc;

    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(err == ERROR_SUCCESS || err == ERROR_FILE_NOT_FOUND, "got %d\n", err);

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

    /* Create a test subkey */
    add_key(key, "Subkey", &subkey);
    add_value(subkey, "Test", REG_SZ, world, sizeof(world));
    add_value(subkey, "Wine", REG_DWORD, &dword2, sizeof(dword2));
    add_value(subkey, NULL, REG_SZ, empty2, sizeof(empty2));

    err = RegCloseKey(subkey);
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

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

    /* Clean-up, then query */
    delete_key(key, "subkey");

    err = RegCloseKey(key);
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    run_reg_exe("reg query HKCU\\" KEY_BASE "\\subkey", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    delete_key(HKEY_CURRENT_USER, KEY_BASE);

    run_reg_exe("reg query HKCU\\" KEY_BASE, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);
}

static void test_v_flags(void)
{
    DWORD r;

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v Wine /ve", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /v Wine /ve", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /v Wine /va", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /ve /va", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    /* No /v argument */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /d Test /f /v", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /v", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /f /v", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    /* Multiple /v switches */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v Wine /t REG_DWORD /d 0x1 /v Test /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /v Wine /v Test /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);
}

static BOOL write_file(const void *str, DWORD size)
{
    HANDLE file;
    BOOL ret;
    DWORD written;

    file = CreateFileA("test.reg", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %u\n", GetLastError());
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;

    ret = WriteFile(file, str, size, &written, NULL);
    ok(ret, "WriteFile failed: %u\n", GetLastError());
    CloseHandle(file);

    return ret;
}

#define test_import_str(c,r) import_reg(__LINE__,c,FALSE,r)
#define test_import_wstr(c,r) import_reg(__LINE__,c,TRUE,r)

static BOOL import_reg(unsigned line, const char *contents, BOOL unicode, DWORD *rc)
{
    int lenA;
    BOOL ret;

    lenA = strlen(contents);

    if (unicode)
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, contents, lenA, NULL, 0);
        int size = len * sizeof(WCHAR);
        WCHAR *wstr = HeapAlloc(GetProcessHeap(), 0, size);
        if (!wstr) return FALSE;
        MultiByteToWideChar(CP_UTF8, 0, contents, lenA, wstr, len);

        ret = write_file(wstr, size);
        HeapFree(GetProcessHeap(), 0, wstr);
    }
    else
        ret = write_file(contents, lenA);

    if (!ret) return FALSE;

    run_reg_exe("reg import test.reg", rc);

    ret = DeleteFileA("test.reg");
    lok(ret, "DeleteFile failed: %u\n", GetLastError());

    return ret;
}

static void test_import(void)
{
    DWORD r, dword = 0x123, type, size;
    char buffer[24];
    HKEY hkey, subkey = NULL;
    LONG err;
    BYTE hex[8];

    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(err == ERROR_SUCCESS || err == ERROR_FILE_NOT_FOUND, "got %d\n", err);

    run_reg_exe("reg import", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg import /?", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg import /h", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg import -H", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg import missing.reg", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg import a.reg b.reg", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    /* Test file contents */
    test_import_str("regedit\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_str("regedit4\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_str("REGEDIT", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("REGEDIT\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("REGEDIT4\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str(" REGEDIT4\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("\tREGEDIT4\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("\nREGEDIT4\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_str("AREGEDIT4\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_str("1REGEDIT4\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_str("REGEDIT3\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("REGEDIT5\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("REGEDIT9\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("REGEDIT 4\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("REGEDIT4 FOO\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("REGEDIT4\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    open_key(HKEY_CURRENT_USER, KEY_BASE, KEY_SET_VALUE, &hkey);

    test_import_str("REGEDIT4\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine\"=dword:00000123\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_str("REGEDIT4\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "@=\"Test string\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, NULL, REG_SZ, "Test string", 12, 0);

    test_import_str("REGEDIT3\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test1\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Test1");

    test_import_str("regedit4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test2\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    verify_reg_nonexist(hkey, "Test2");

    test_import_str("Regedit4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test3\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    verify_reg_nonexist(hkey, "Test3");

    test_import_str("REGEDIT 4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test4\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Test4");

    test_import_str("REGEDIT4FOO\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test5\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Test5");

    test_import_str("REGEDIT4 FOO\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test6\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Test6");

    test_import_str("REGEDIT5\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test7\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Test7");

    test_import_str("REGEDIT9\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test8\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Test8");

    test_import_str("Windows Registry Editor Version 4.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test9\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    verify_reg_nonexist(hkey, "Test9");

    test_import_str("Windows Registry Editor Version 5\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test10\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    verify_reg_nonexist(hkey, "Test10");

    test_import_str("WINDOWS REGISTRY EDITOR VERSION 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test11\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    verify_reg_nonexist(hkey, "Test11");

    test_import_str("Windows Registry Editor version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test12\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    verify_reg_nonexist(hkey, "Test12");

    test_import_str("REGEDIT4\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test1\"=\"Value1\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Test1", REG_SZ, "Value1", 7, 0);

    test_import_str("REGEDIT4\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test2\"=\"Value2\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Test2", REG_SZ, "Value2", 7, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test3\"=\"Value3\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Test3", REG_SZ, "Value3", 7, 0);

    test_import_str("Windows Registry Editor Version 4.00\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_str("Windows Registry Editor Version 5.00\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("Windows Registry Editor Version 5.00\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test4\"=\"Value4\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Test4", REG_SZ, "Value4", 7, 0);

    test_import_str("Windows Registry Editor Version 5.00\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test5\"=\"Value5\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Test5", REG_SZ, "Value5", 7, 0);

    test_import_str("Windows Registry Editor Version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test6\"=\"Value6\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Test6", REG_SZ, "Value6", 7, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Line1\"=\"Value1\"\n\n"
                    "\"Line2\"=\"Value2\"\n\n\n"
                    "\"Line3\"=\"Value3\"\n\n\n\n"
                    "\"Line4\"=\"Value4\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Line1", REG_SZ, "Value1", 7, 0);
    verify_reg(hkey, "Line2", REG_SZ, "Value2", 7, 0);
    verify_reg(hkey, "Line3", REG_SZ, "Value3", 7, 0);
    verify_reg(hkey, "Line4", REG_SZ, "Value4", 7, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine1\"=dword:00000782\n\n"
                    "\"Wine2\"=\"Test Value\"\n"
                    "\"Wine3\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "#comment\n"
                    "@=\"Test\"\n"
                    ";comment\n\n"
                    "\"Wine4\"=dword:12345678\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x782;
    verify_reg(hkey, "Wine1", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine2", REG_SZ, "Test Value", 11, 0);
    verify_reg(hkey, "Wine3", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "", REG_SZ, "Test", 5, 0);
    dword = 0x12345678;
    verify_reg(hkey, "Wine4", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine5\"=\"No newline\"", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegQueryValueExA(hkey, "Wine5", NULL, NULL, NULL, NULL);
    ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND /* WinXP */),
       "got %d, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine5", REG_SZ, "No newline", 11, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine6\"=dword:00000050\n\n"
                    "\"Wine7\"=\"No newline\"", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x50;
    verify_reg(hkey, "Wine6", REG_DWORD, &dword, sizeof(dword), 0);
    err = RegQueryValueExA(hkey, "Wine7", NULL, NULL, NULL, NULL);
    ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND /* WinXP */),
       "got %d, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine7", REG_SZ, "No newline", 11, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "#comment\\\n"
                    "\"Wine8\"=\"Line 1\"\n"
                    ";comment\\\n"
                    "\"Wine9\"=\"Line 2\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine8", REG_SZ, "Line 1", 7, 0);
    verify_reg(hkey, "Wine9", REG_SZ, "Line 2", 7, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine10\"=\"Value 1\"#comment\n"
                    "\"Wine11\"=\"Value 2\";comment\n"
                    "\"Wine12\"=dword:01020304 #comment\n"
                    "\"Wine13\"=dword:02040608 ;comment\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine10");
    verify_reg(hkey, "Wine11", REG_SZ, "Value 2", 8, 0);
    verify_reg_nonexist(hkey, "Wine12");
    dword = 0x2040608;
    verify_reg(hkey, "Wine13", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine14\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  #comment\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine15\"=\"A valid line\"\n"
                    "\"Wine16\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  ;comment\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine17\"=\"Another valid line\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine14");
    verify_reg(hkey, "Wine15", REG_SZ, "A valid line", 13, 0);
    verify_reg(hkey, "Wine16", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine17", REG_SZ, "Another valid line", 19, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "#\"Comment1\"=\"Value 1\"\n"
                    ";\"Comment2\"=\"Value 2\"\n"
                    "    #\"Comment3\"=\"Value 3\"\n"
                    "    ;\"Comment4\"=\"Value 4\"\n"
                    "\"Wine18\"=\"Value 6\"#\"Comment5\"=\"Value 5\"\n"
                    "\"Wine19\"=\"Value 7\";\"Comment6\"=\"Value 6\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Comment1");
    verify_reg_nonexist(hkey, "Comment2");
    verify_reg_nonexist(hkey, "Comment3");
    verify_reg_nonexist(hkey, "Comment4");
    verify_reg_nonexist(hkey, "Wine18");
    verify_reg_nonexist(hkey, "Comment5");
    verify_reg(hkey, "Wine19", REG_SZ, "Value 7", 8, 0);
    verify_reg_nonexist(hkey, "Comment6");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine20\"=#\"Value 8\"\n"
                    "\"Wine21\"=;\"Value 9\"\n"
                    "\"Wine22\"=\"#comment1\"\n"
                    "\"Wine23\"=\";comment2\"\n"
                    "\"Wine24\"=\"Value#comment3\"\n"
                    "\"Wine25\"=\"Value;comment4\"\n"
                    "\"Wine26\"=\"Value #comment5\"\n"
                    "\"Wine27\"=\"Value ;comment6\"\n"
                    "\"Wine28\"=#dword:00000001\n"
                    "\"Wine29\"=;dword:00000002\n"
                    "\"Wine30\"=dword:00000003#comment\n"
                    "\"Wine31\"=dword:00000004;comment\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine20");
    verify_reg_nonexist(hkey, "Wine21");
    verify_reg(hkey, "Wine22", REG_SZ, "#comment1", 10, 0);
    verify_reg(hkey, "Wine23", REG_SZ, ";comment2", 10, 0);
    verify_reg(hkey, "Wine24", REG_SZ, "Value#comment3", 15, 0);
    verify_reg(hkey, "Wine25", REG_SZ, "Value;comment4", 15, 0);
    verify_reg(hkey, "Wine26", REG_SZ, "Value #comment5", 16, 0);
    verify_reg(hkey, "Wine27", REG_SZ, "Value ;comment6", 16, 0);
    verify_reg_nonexist(hkey, "Wine28");
    verify_reg_nonexist(hkey, "Wine29");
    verify_reg_nonexist(hkey, "Wine30");
    dword = 0x00000004;
    verify_reg(hkey, "Wine31", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line1\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,\\;comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Multi-Line1", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line2\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,;comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Multi-Line2", REG_MULTI_SZ, "Line concat", 12, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line3\"=hex(7):4c,69,6e,65,20\\\n"
                    ",63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Multi-Line3");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line4\"=hex(7):4c,69,6e,65,20\\\n"
                    "  ,63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Multi-Line4");

    test_import_str("Windows Registry Editor Version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line5\"=hex(7):4c,69,6e,65,20\\\n"
                    ",63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Multi-Line5");

    test_import_str("Windows Registry Editor Version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line6\"=hex(7):4c,69,6e,65,20\\\n"
                    "  ,63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Multi-Line6");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line7\"=hex(7):4c,69,6e,\\;comment\n"
                    "  65,20,\\;comment\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,\\;comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Multi-Line7", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line8\"=hex(7):4c,69,6e,\\;#comment\n"
                    "  65,20,\\;#comment\n"
                    "  63,6f,6e,\\;#comment\n"
                    "  63,61,74,\\;#comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Multi-Line8", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line9\"=hex(7):4c,69,6e,\\;comment\n"
                    "  65,20,\\;comment\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,\\#comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Multi-Line9");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line10\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,\\\n\n"
                    "  65,6e,\\;comment\n\n"
                    "  61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Multi-Line10", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine32a\"=dword:1\n"
                    "\"Wine32b\"=dword:4444\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x1;
    verify_reg(hkey, "Wine32a", REG_DWORD, &dword, sizeof(dword), 0);
    dword = 0x4444;
    verify_reg(hkey, "Wine32b", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine33a\"=dword:\n"
                    "\"Wine33b\"=dword:hello\n"
                    "\"Wine33c\"=dword:123456789\n"
                    "\"Wine33d\"=dword:012345678\n"
                    "\"Wine33e\"=dword:000000001\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine33a");
    verify_reg_nonexist(hkey, "Wine33b");
    verify_reg_nonexist(hkey, "Wine33c");
    verify_reg_nonexist(hkey, "Wine33d");
    verify_reg_nonexist(hkey, "Wine33e");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine34a\"=dword:12345678abc\n"
                    "\"Wine34b\"=dword:12345678 abc\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine34a");
    verify_reg_nonexist(hkey, "Wine34b");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine35a\"=dword:0x123\n"
                    "\"Wine35b\"=dword:123 456\n"
                    "\"Wine35c\"=dword:1234 5678\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine35a");
    verify_reg_nonexist(hkey, "Wine35b");
    verify_reg_nonexist(hkey, "Wine35c");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine36a\"=dword:1234;5678\n"
                    "\"Wine36b\"=dword:1234 ;5678\n"
                    "\"Wine36c\"=dword:1234#5678\n"
                    "\"Wine36d\"=dword:1234 #5678\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x1234;
    verify_reg(hkey, "Wine36a", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine36b", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg_nonexist(hkey, "Wine36c");
    verify_reg_nonexist(hkey, "Wine36d");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine37a\"=\"foo\"bar\"\n"
                    "\"Wine37b\"=\"foo\"\"bar\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine37a");
    verify_reg_nonexist(hkey, "Wine37b");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Empty string\"=\"\"\n"
                    "\"\"=\"Default Value Name\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Empty string", REG_SZ, "", 1, 0);
    verify_reg(hkey, NULL, REG_SZ, "Default Value Name", 19, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test38a\"=\n"
                    "\"Test38b\"=\\\"\n"
                    "\"Test38c\"=\\\"Value\\\"\n"
                    "\"Test38d\"=\\\"Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Test38a");
    verify_reg_nonexist(hkey, "Test38b");
    verify_reg_nonexist(hkey, "Test38c");
    verify_reg_nonexist(hkey, "Test38d");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine39a\"=\"Value1\"  ;comment\n"
                    "\"Wine39b\"=\"Value2\"\t\t;comment\n"
                    "\"Wine39c\"=\"Value3\"  #comment\n"
                    "\"Wine39d\"=\"Value4\"\t\t#comment\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine39a", REG_SZ, "Value1", 7, 0);
    verify_reg(hkey, "Wine39b", REG_SZ, "Value2", 7, 0);
    verify_reg_nonexist(hkey, "Wine39c");
    verify_reg_nonexist(hkey, "Wine39d");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"TestNoBeginQuote\"=Asdffdsa\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "TestNoBeginQuote");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"TestNoEndQuote\"=\"Asdffdsa\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "TestNoEndQuote");

    test_import_str("REGEDIT4\n\n"
                   "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                   "\"TestNoQuotes\"=Asdffdsa\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "TestNoQuotes");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "NameNoBeginQuote\"=\"Asdffdsa\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "NameNoBeginQuote");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"NameNoEndQuote=\"Asdffdsa\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "NameNoEndQuote");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "NameNoQuotes=\"Asdffdsa\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "NameNoQuotes");

    test_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"MixedQuotes=Asdffdsa\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "MixedQuotes");
    verify_reg_nonexist(hkey, "MixedQuotes=Asdffdsa");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine40a\"=hex(2):4c,69,6e,65,00\n"
                    "\"Wine40b\"=\"Value 1\"\n"
                    "\"Wine40c\"=hex(2):4c,69,6e,65\\\n"
                    "\"Wine40d\"=\"Value 2\"\n"
                    "\"Wine40e\"=hex(2):4c,69,6e,65,\\\n"
                    "\"Wine40f\"=\"Value 3\"\n"
                    "\"Wine40g\"=\"Value 4\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine40a", REG_EXPAND_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine40b", REG_SZ, "Value 1", 8, 0);
    verify_reg_nonexist(hkey, "Wine40c");
    verify_reg(hkey, "Wine40d", REG_SZ, "Value 2", 8, 0);
    verify_reg_nonexist(hkey, "Wine40e");
    verify_reg_nonexist(hkey, "Wine40f");
    verify_reg(hkey, "Wine40g", REG_SZ, "Value 4", 8, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine41a\"=dword:1234\\\n"
                    "5678\n"
                    "\"Wine41b\"=\"Test \\\n"
                    "Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine41a");
    verify_reg_nonexist(hkey, "Wine41b");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"double\\\"quote\"=\"valid \\\"or\\\" not\"\n"
                    "\"single'quote\"=dword:00000008\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "double\"quote", REG_SZ, "valid \"or\" not", 15, 0);
    dword = 0x00000008;
    verify_reg(hkey, "single'quote", REG_DWORD, &dword, sizeof(dword), 0);

    /* Test key name and value name concatenation */
    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\\n"
                    "Subkey1]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey1");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\n"
                    "\\Subkey2]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey2");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine\\\n"
                    "42a\"=\"Value 1\"\n"
                    "\"Wine42b\"=\"Value 2\"\n"
                    "\"Wine\n"
                    "\\42c\"=\"Value 3\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine42a");
    verify_reg(hkey, "Wine42b", REG_SZ, "Value 2", 8, 0);
    verify_reg_nonexist(hkey, "Wine42c");

    /* Test hex data concatenation for REG_NONE, REG_EXPAND_SZ and REG_BINARY */
    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine43a\"=hex(0):56,00,61,00,6c,00,75,00,65,00,00,00\n"
                    "\"Wine43b\"=hex(0):56,00,61,00,6c,00,\\\n"
                    "  75,00,65,00,00,00\n"
                    "\"Wine43c\"=hex(0):56,00,61,00,6c,00\\\n"
                    ",75,00,65,00,00,00\n"
                    "\"Wine43d\"=hex(0):56,00,61,00,6c,00\\\n"
                    "  ,75,00,65,00,00,00\n"
                    "\"Wine43e\"=hex(0):56,00,61,00,6c,00\\\n"
                    "  75,00,65,00,00,00\n"
                    "\"Wine43f\"=hex(0):56,00,61,00,6c,00,7\\\n"
                    "5,00,65,00,00,00\n"
                    "\"Wine43g\"=hex(0):56,00,61,00,6c,00,7\\\n"
                    "  5,00,65,00,00,00\n"
                    "\"Wine43h\"=hex(0):56,00,61,00,\\;comment\n"
                    "  6c,00,75,00,\\\n"
                    "  65,00,00,00\n"
                    "\"Wine43i\"=hex(0):56,00,61,00,\\;comment\n"
                    "  6c,00,75,00,\n"
                    "  65,00,00,00\n"
                    "\"Wine43j\"=hex(0):56,00,61,00,\\;comment\n"
                    "  6c,00,75,00,;comment\n"
                    "  65,00,00,00\n"
                    "\"Wine43k\"=hex(0):56,00,61,00,\\;comment\n"
                    "  6c,00,75,00,\\#comment\n"
                    "  65,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine43a", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    verify_reg(hkey, "Wine43b", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    verify_reg_nonexist(hkey, "Wine43c");
    verify_reg_nonexist(hkey, "Wine43d");
    verify_reg_nonexist(hkey, "Wine43e");
    verify_reg_nonexist(hkey, "Wine43f");
    verify_reg_nonexist(hkey, "Wine43g");
    verify_reg(hkey, "Wine43h", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    verify_reg(hkey, "Wine43i", REG_NONE, "V\0a\0l\0u", 8, 0);
    verify_reg(hkey, "Wine43j", REG_NONE, "V\0a\0l\0u", 8, 0);
    verify_reg_nonexist(hkey, "Wine43k");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine44a\"=hex(2):25,50,41,54,48,25,00\n"
                    "\"Wine44b\"=hex(2):25,50,41,\\\n"
                    "  54,48,25,00\n"
                    "\"Wine44c\"=hex(2):25,50,41\\\n"
                    ",54,48,25,00\n"
                    "\"Wine44d\"=hex(2):25,50,41\\\n"
                    "  ,54,48,25,00\n"
                    "\"Wine44e\"=hex(2):25,50,41\\\n"
                    "  54,48,25,00\n"
                    "\"Wine44f\"=hex(2):25,50,4\\\n"
                    "1,54,48,25,00\n"
                    "\"Wine44g\"=hex(2):25,50,4\\\n"
                    "  1,54,48,25,00\n"
                    "\"Wine44h\"=hex(2):25,50,41,\\;comment\n"
                    "  54,48,\\\n"
                    "  25,00\n"
                    "\"Wine44i\"=hex(2):25,50,41,\\;comment\n"
                    "  54,48,\n"
                    "  25,00\n"
                    "\"Wine44j\"=hex(2):25,50,41,\\;comment\n"
                    "  54,48,;comment\n"
                    "  25,00\n"
                    "\"Wine44k\"=hex(2):25,50,41,\\;comment\n"
                    "  54,48,\\#comment\n"
                    "  25,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine44a", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine44b", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg_nonexist(hkey, "Wine44c");
    verify_reg_nonexist(hkey, "Wine44d");
    verify_reg_nonexist(hkey, "Wine44e");
    verify_reg_nonexist(hkey, "Wine44f");
    verify_reg_nonexist(hkey, "Wine44g");
    verify_reg(hkey, "Wine44h", REG_EXPAND_SZ, "%PATH%", 7, 0);
    /* Wine44i */
    size = sizeof(buffer);
    err = RegQueryValueExA(hkey, "Wine44i", NULL, &type, (BYTE *)&buffer, &size);
    ok(err == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", err);
    ok(type == REG_EXPAND_SZ, "got wrong type %u, expected %u\n", type, REG_EXPAND_SZ);
    ok(size == 6 || broken(size == 5) /* WinXP */, "got wrong size %u, expected 6\n", size);
    ok(memcmp(buffer, "%PATH", size) == 0, "got wrong data\n");
    /* Wine44j */
    size = sizeof(buffer);
    memset(buffer, '-', size);
    err = RegQueryValueExA(hkey, "Wine44j", NULL, &type, (BYTE *)&buffer, &size);
    ok(err == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", err);
    ok(type == REG_EXPAND_SZ, "got wrong type %u, expected %u\n", type, REG_EXPAND_SZ);
    ok(size == 6 || broken(size == 5) /* WinXP */, "got wrong size %u, expected 6\n", size);
    ok(memcmp(buffer, "%PATH", size) == 0, "got wrong data\n");
    /* Wine44k */
    verify_reg_nonexist(hkey, "Wine44k");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine45a\"=hex:11,22,33,44,55,66,77,88\n"
                    "\"Wine45b\"=hex:11,22,33,44,\\\n"
                    "  55,66,77,88\n"
                    "\"Wine45c\"=hex:11,22,33,44\\\n"
                    ",55,66,77,88\n"
                    "\"Wine45d\"=hex:11,22,33,44\\\n"
                    "  ,55,66,77,88\n"
                    "\"Wine45e\"=hex:11,22,33,44\\\n"
                    "  55,66,77,88\n"
                    "\"Wine45f\"=hex:11,22,33,4\\\n"
                    "4,55,66,77,88\n"
                    "\"Wine45g\"=hex:11,22,33,4\\\n"
                    "  4,55,66,77,88\n"
                    "\"Wine45h\"=hex:11,22,33,44,\\;comment\n"
                    "  55,66,\\\n"
                    "  77,88\n"
                    "\"Wine45i\"=hex:11,22,33,44,\\;comment\n"
                    "  55,66,\n"
                    "  77,88\n"
                    "\"Wine45j\"=hex:11,22,33,44,\\;comment\n"
                    "  55,66,;comment\n"
                    "  77,88\n"
                    "\"Wine45k\"=hex:11,22,33,\\;comment\n"
                    "  44,55,66,\\#comment\n"
                    "  77,88\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    hex[0] = 0x11; hex[1] = 0x22; hex[2] = 0x33; hex[3] = 0x44;
    hex[4] = 0x55; hex[5] = 0x66; hex[6] = 0x77; hex[7] = 0x88;
    verify_reg(hkey, "Wine45a", REG_BINARY, hex, sizeof(hex), 0);
    verify_reg(hkey, "Wine45b", REG_BINARY, hex, sizeof(hex), 0);
    verify_reg_nonexist(hkey, "Wine45c");
    verify_reg_nonexist(hkey, "Wine45d");
    verify_reg_nonexist(hkey, "Wine45e");
    verify_reg_nonexist(hkey, "Wine45f");
    verify_reg_nonexist(hkey, "Wine45g");
    verify_reg(hkey, "Wine45h", REG_BINARY, hex, sizeof(hex), 0);
    verify_reg(hkey, "Wine45i", REG_BINARY, hex, 6, 0);
    verify_reg(hkey, "Wine45j", REG_BINARY, hex, 6, 0);
    verify_reg_nonexist(hkey, "Wine45k");

    /* Test import with subkeys */
    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey\"1]\n"
                    "\"Wine\\\\31\"=\"Test value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    open_key(hkey, "Subkey\"1", 0, &subkey);
    verify_reg(subkey, "Wine\\31", REG_SZ, "Test value", 11, 0);
    err = RegCloseKey(subkey);
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);
    delete_key(HKEY_CURRENT_USER, KEY_BASE "\\Subkey\"1");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey/2]\n"
                    "\"123/\\\"4;'5\"=\"Random value name\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    open_key(hkey, "Subkey/2", 0, &subkey);
    verify_reg(subkey, "123/\"4;'5", REG_SZ, "Random value name", 18, 0);
    err = RegCloseKey(subkey);
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);
    delete_key(HKEY_CURRENT_USER, KEY_BASE "\\Subkey/2");

    /* Test key creation */
    test_import_str("REGEDIT4\n\n"
                    "HKEY_CURRENT_USER\\" KEY_BASE "\\No_Opening_Bracket]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "No_Opening_Bracket");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\No_Closing_Bracket\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "No_Closing_Bracket");

    test_import_str("REGEDIT4\n\n"
                    "[ HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1a]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey1a");

    test_import_str("REGEDIT4\n\n"
                    "[\tHKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1b]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey1b");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1c ]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey1c ");
    delete_key(hkey, "Subkey1c ");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1d\t]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey1d\t");
    delete_key(hkey, "Subkey1d\t");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1e\\]\n"
                    "\"Wine\"=\"Test value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey1e\\");
    verify_key(hkey, "Subkey1e");
    open_key(hkey, "Subkey1e", 0, &subkey);
    verify_reg(subkey, "Wine", REG_SZ, "Test value", 11, 0);
    RegCloseKey(subkey);
    delete_key(hkey, "Subkey1e");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1f\\\\]\n"
                    "\"Wine\"=\"Test value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey1f\\\\");
    verify_key(hkey, "Subkey1f\\");
    verify_key(hkey, "Subkey1f");
    open_key(hkey, "Subkey1f\\\\", 0, &subkey);
    verify_reg(subkey, "Wine", REG_SZ, "Test value", 11, 0);
    RegCloseKey(subkey);
    delete_key(hkey, "Subkey1f\\\\");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1g\\\\\\\\]\n"
                    "\"Wine\"=\"Test value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey1g\\\\\\\\");
    verify_key(hkey, "Subkey1g\\\\");
    verify_key(hkey, "Subkey1g\\");
    verify_key(hkey, "Subkey1g");
    open_key(hkey, "Subkey1g\\\\", 0, &subkey);
    verify_reg(subkey, "Wine", REG_SZ, "Test value", 11, 0);
    RegCloseKey(subkey);
    delete_key(hkey, "Subkey1g\\\\");

    /* Test key deletion. We start by creating some registry keys. */
    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2b]\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey2a");
    verify_key(hkey, "Subkey2b");

    test_import_str("REGEDIT4\n\n"
                    "[ -HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey2a");

    test_import_str("REGEDIT4\n\n"
                    "[\t-HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2b]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey2b");

    test_import_str("REGEDIT4\n\n"
                    "[- HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey2a");

    test_import_str("REGEDIT4\n\n"
                    "[-\tHKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2b]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey2b");

    test_import_str("REGEDIT4\n\n"
                    "[-HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\n\n"
                    "[-HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2b]\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey2a");
    verify_key_nonexist(hkey, "Subkey2b");

    /* Test case sensitivity when creating and deleting registry keys. */
    test_import_str("REGEDIT4\n\n"
                    "[hkey_CURRENT_user\\" KEY_BASE "\\Subkey3a]\n\n"
                    "[HkEy_CuRrEnT_uSeR\\" KEY_BASE "\\SuBkEy3b]\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey3a");
    verify_key(hkey, "Subkey3b");

    test_import_str("REGEDIT4\n\n"
                    "[-HKEY_current_USER\\" KEY_BASE "\\sUBKEY3A]\n\n"
                    "[-hKeY_cUrReNt_UsEr\\" KEY_BASE "\\sUbKeY3B]\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey3a");
    verify_key_nonexist(hkey, "Subkey3b");

    /* Test mixed key creation and deletion. We start by creating a subkey. */
    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey4a]\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey4a");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n"
                    "[-HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey4a]\n"
                    "\"Wine46a\"=dword:12345678\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey4a");
    verify_reg_nonexist(hkey, "Wine46a");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n"
                    "[HKEY_CURRENT_USERS\\" KEY_BASE "\\Subkey4b]\n"
                    "\"Wine46b\"=dword:12345678\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey4b");
    verify_reg_nonexist(hkey, "Wine46b");

    /* Test value deletion. We start by creating some registry values. */
    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine46a\"=\"Test Value\"\n"
                    "\"Wine46b\"=dword:00000008\n"
                    "\"Wine46c\"=hex:11,22,33,44\n"
                    "\"Wine46d\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine46e\"=hex(2):25,50,41,54,48,25,00\n"
                    "\"Wine46f\"=hex(0):56,00,61,00,6c,00,75,00,65,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine46a", REG_SZ, "Test Value", 11, 0);
    verify_reg(hkey, "Wine46b", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine46c", REG_BINARY, hex, 4, 0);
    verify_reg(hkey, "Wine46d", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine46e", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine46f", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine46a\"=-\n"
                    "\"Wine46b\"=  -\n"
                    "\"Wine46c\"=  \t-\t  \n"
                    "\"Wine46d\"=-\"Test\"\n"
                    "\"Wine46e\"=- ;comment\n"
                    "\"Wine46f\"=- #comment\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine46a");
    verify_reg_nonexist(hkey, "Wine46b");
    verify_reg_nonexist(hkey, "Wine46c");
    verify_reg(hkey, "Wine46d", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg_nonexist(hkey, "Wine46e");
    verify_reg(hkey, "Wine46f", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);

    /* Test the accepted range of the hex-based data types */
    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine47a\"=hex(0):56,61,6c,75,65,00\n"
                    "\"Wine47b\"=hex(10):56,61,6c,75,65,00\n"
                    "\"Wine47c\"=hex(100):56,61,6c,75,65,00\n"
                    "\"Wine47d\"=hex(1000):56,61,6c,75,65,00\n"
                    "\"Wine47e\"=hex(7fff):56,61,6c,75,65,00\n"
                    "\"Wine47f\"=hex(ffff):56,61,6c,75,65,00\n"
                    "\"Wine47g\"=hex(7fffffff):56,61,6c,75,65,00\n"
                    "\"Wine47h\"=hex(ffffffff):56,61,6c,75,65,00\n"
                    "\"Wine47i\"=hex(100000000):56,61,6c,75,65,00\n"
                    "\"Wine47j\"=hex(0x2):56,61,6c,75,65,00\n"
                    "\"Wine47k\"=hex(0X2):56,61,6c,75,65,00\n"
                    "\"Wine47l\"=hex(x2):56,61,6c,75,65,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine47a", REG_NONE, "Value", 6, 0);
    verify_reg(hkey, "Wine47b", 0x10, "Value", 6, 0);
    verify_reg(hkey, "Wine47c", 0x100, "Value", 6, 0);
    verify_reg(hkey, "Wine47d", 0x1000, "Value", 6, 0);
    verify_reg(hkey, "Wine47e", 0x7fff, "Value", 6, 0);
    verify_reg(hkey, "Wine47f", 0xffff, "Value", 6, 0);
    verify_reg(hkey, "Wine47g", 0x7fffffff, "Value", 6, 0);
    verify_reg(hkey, "Wine47h", 0xffffffff, "Value", 6, 0);
    verify_reg_nonexist(hkey, "Wine47i");
    verify_reg_nonexist(hkey, "Wine47j");
    verify_reg_nonexist(hkey, "Wine47k");
    verify_reg_nonexist(hkey, "Wine47l");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine48a\"=hex(7):4c,69,6e,65,20,  \\\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine48b\"=hex(7):4c,69,6e,65,20,\t\\\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine48a", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine48b", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine49\"=hex(2):25,50,41,54,48,25,00,\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine49", REG_EXPAND_SZ, "%PATH%", 7, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine50a\"=hex(2):25,50,41,54,48,25,00  ;comment\n"
                    "\"Wine50b\"=hex(2):25,50,41,54,48,25,00\t;comment\n"
                    "\"Wine50c\"=hex(2):25,50,41,54,48,25,00  #comment\n"
                    "\"Wine50d\"=hex(2):25,50,41,54,48,25,00\t#comment\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine50a", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine50b", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg_nonexist(hkey, "Wine50c");
    verify_reg_nonexist(hkey, "Wine50d");

    /* Test support for characters greater than 0xff */
    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine51a\"=hex(0):25,50,100,54,48,25,00\n"
                    "\"Wine51b\"=hex(0):25,1a4,100,164,124,25,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine51a");
    verify_reg_nonexist(hkey, "Wine51b");

    /* Test the effect of backslashes in hex data */
    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine52a\"=hex(2):25,48\\,4f,4d,45,25,00\n"
                    "\"Wine52b\"=hex(2):25,48,\\4f,4d,45,25,00\n"
                    "\"Wine52c\"=hex(2):25,48\\ ,4f,4d,45,25,00\n"
                    "\"Wine52d\"=hex(2):25,48,\\ 4f,4d,45,25,00\n"
                    "\"Wine52e\"=hex(2):\\25,48,4f,4d,45,25,00\n"
                    "\"Wine52f\"=hex(2):\\ 25,48,4f,4d,45,25,00\n"
                    "\"Wine52g\"=hex(2):25,48,4\\f,4d,45,25,00\n"
                    "\"Wine52h\"=hex(2):25,48,4\\\n"
                    "  f,4d,45,25,00\n"
                    "\"Wine52i\"=hex(2):25,50,\\,41,54,48,25,00\n"
                    "\"Wine52j\"=hex(2):25,48,4f,4d,45,25,5c,\\\\\n"
                    "  25,50,41,54,48,25,00\n"
                    "\"Wine52k\"=hex(2):,\\\n"
                    "  25,48,4f,4d,45,25,00\n"
                    "\"Wine52l\"=hex(2):\\\n"
                    "  25,48,4f,4d,45,25,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine52a");
    verify_reg_nonexist(hkey, "Wine52b");
    verify_reg_nonexist(hkey, "Wine52c");
    verify_reg_nonexist(hkey, "Wine52d");
    verify_reg_nonexist(hkey, "Wine52e");
    verify_reg_nonexist(hkey, "Wine52f");
    verify_reg_nonexist(hkey, "Wine52g");
    verify_reg_nonexist(hkey, "Wine52h");
    verify_reg_nonexist(hkey, "Wine52i");
    verify_reg_nonexist(hkey, "Wine52j");
    verify_reg_nonexist(hkey, "Wine52k");
    verify_reg(hkey, "Wine52l", REG_EXPAND_SZ, "%HOME%", 7, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine53a\"=hex(2):25,48,4f,4d,45,25,5c,\\\n"
                    "  25,50,41,54,48,25,00\n"
                    "\"Wine53b\"=hex(2):25,48,4f,4d,45,25,5c\\\n"
                    "  25,50,41,54,48,25,00\n"
                    "\"Wine53c\"=hex(2):25,48,4f,4d,45,25,5c,  \\  ;comment\n"
                    "  25,50,41,54,48,25,00\n"
                    "\"Wine53d\"=hex(2):25,48,4f,4d,45,25,5c  \\  ;comment\n"
                    "  25,50,41,54,48,25,00\n"
                    "\"Wine53e\"=hex(2):25,48,4f,4d,45,25,5c,\\\t  ;comment\n"
                    "  25,50,41,54,48,25,00\n"
                    "\"Wine53f\"=hex(2):25,48,4f,4d,45,25,5c\\\t  ;comment\n"
                    "  25,50,41,54,48,25,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine53a", REG_EXPAND_SZ, "%HOME%\\%PATH%", 14, 0);
    verify_reg_nonexist(hkey, "Wine53b");
    verify_reg(hkey, "Wine53c", REG_EXPAND_SZ, "%HOME%\\%PATH%", 14, 0);
    verify_reg_nonexist(hkey, "Wine53d");
    verify_reg(hkey, "Wine53e", REG_EXPAND_SZ, "%HOME%\\%PATH%", 14, 0);
    verify_reg_nonexist(hkey, "Wine53f");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine54a\"=hex(2):4c,69,6e,65,20,\\\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine54a");
    verify_key_nonexist(hkey, "Subkey1");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine54b\"=hex(2):4c,69,6e,65,20\\\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine54b");
    verify_key(hkey, "Subkey2");

    delete_key(hkey, "Subkey2");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine55a\"=hex(2):4c,69,6e,65,20,\\\n"
                    "\"Wine55b\"=\"Test value\"\n"

                    "\"Wine55c\"=hex(2):4c,69,6e,65,20,\\\n"
                    ";comment\n"
                    "\"Wine55d\"=\"Test value\"\n"

                    "\"Wine55e\"=hex(2):4c,69,6e,65,20,\\\n"
                    "#comment\n"
                    "\"Wine55f\"=\"Test value\"\n"

                    "\"Wine55g\"=hex(2):4c,69,6e,65,20,\\\n\n"
                    "\"Wine55h\"=\"Test value\"\n"

                    "\"Wine55i\"=hex(2):4c,69,6e,65,20\\\n"
                    "\"Wine55j\"=\"Test value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine55a");
    verify_reg_nonexist(hkey, "Wine55b");
    verify_reg_nonexist(hkey, "Wine55c");
    verify_reg_nonexist(hkey, "Wine55d");
    verify_reg_nonexist(hkey, "Wine55e");
    verify_reg(hkey, "Wine55f", REG_SZ, "Test value", 11, 0);
    verify_reg_nonexist(hkey, "Wine55g");
    verify_reg_nonexist(hkey, "Wine55h");
    verify_reg_nonexist(hkey, "Wine55i");
    verify_reg(hkey, "Wine55j", REG_SZ, "Test value", 11, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine56a\"=hex(2):4c,69,6e,65,20,\\\n"
                    "\"Wine56b\"=dword:00000008\n"

                    "\"Wine56c\"=hex(2):4c,69,6e,65,20,\\\n"
                    ";comment\n"
                    "\"Wine56d\"=dword:00000008\n"

                    "\"Wine56e\"=hex(2):4c,69,6e,65,20,\\\n"
                    "#comment\n"
                    "\"Wine56f\"=dword:00000008\n"

                    "\"Wine56g\"=hex(2):4c,69,6e,65,20,\\\n\n"
                    "\"Wine56h\"=dword:00000008\n"

                    "\"Wine56i\"=hex(2):4c,69,6e,65,20\\\n"
                    "\"Wine56j\"=dword:00000008\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine56a");
    verify_reg_nonexist(hkey, "Wine56b");
    verify_reg_nonexist(hkey, "Wine56c");
    verify_reg_nonexist(hkey, "Wine56d");
    verify_reg_nonexist(hkey, "Wine56e");
    verify_reg(hkey, "Wine56f", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg_nonexist(hkey, "Wine56g");
    verify_reg_nonexist(hkey, "Wine56h");
    verify_reg_nonexist(hkey, "Wine56i");
    verify_reg(hkey, "Wine56j", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine57a\"=hex(2):25,48,4f,4d,45,25,5c,\\\n"
                    "\"Wine57b\"=hex(2):25,50,41,54,48,25,00\n"

                    "\"Wine57c\"=hex(2):25,48,4f,4d,45,25,5c,\\\n"
                    ";comment\n"
                    "\"Wine57d\"=hex(2):25,50,41,54,48,25,00\n"

                    "\"Wine57e\"=hex(2):25,48,4f,4d,45,25,5c,\\\n"
                    "#comment\n"
                    "\"Wine57f\"=hex(2):25,50,41,54,48,25,00\n"

                    "\"Wine57g\"=hex(2):25,48,4f,4d,45,25,5c,\\\n\n"
                    "\"Wine57h\"=hex(2):25,50,41,54,48,25,00\n"

                    "\"Wine57i\"=hex(2):25,48,4f,4d,45,25,5c\\\n"
                    "\"Wine57j\"=hex(2):25,50,41,54,48,25,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine57a");
    verify_reg_nonexist(hkey, "Wine57b");
    verify_reg_nonexist(hkey, "Wine57c");
    verify_reg_nonexist(hkey, "Wine57d");
    verify_reg_nonexist(hkey, "Wine57e");
    verify_reg(hkey, "Wine57f", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg_nonexist(hkey, "Wine57g");
    verify_reg_nonexist(hkey, "Wine57h");
    verify_reg_nonexist(hkey, "Wine57i");
    verify_reg(hkey, "Wine57j", REG_EXPAND_SZ, "%PATH%", 7, 0);

    delete_value(hkey, NULL);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine58a\"=hex(2):4c,69,6e,65,20,\\\n"
                    "@=\"Default value 1\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine58a");
    verify_reg_nonexist(hkey, NULL);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine58b\"=hex(2):4c,69,6e,65,20,\\\n"
                    ";comment\n"
                    "@=\"Default value 2\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine58b");
    verify_reg_nonexist(hkey, NULL);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine58c\"=hex(2):4c,69,6e,65,20,\\\n"
                    "#comment\n"
                    "@=\"Default value 3\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine58c");
    verify_reg(hkey, NULL, REG_SZ, "Default value 3", 16, 0);

    delete_value(hkey, NULL);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine58d\"=hex(2):4c,69,6e,65,20,\\\n\n"
                    "@=\"Default value 4\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine58d");
    verify_reg_nonexist(hkey, NULL);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine58e\"=hex(2):4c,69,6e,65,20\\\n"
                    "@=\"Default value 5\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine58e");
    verify_reg(hkey, NULL, REG_SZ, "Default value 5", 16, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine59a\"=hex:11,22,33,\\\n"
                    "\\\n"
                    "  44,55,66\n"
                    "\"Wine59b\"=hex:11,22,33,\\\n"
                    "  \\\n"
                    "  44,55,66\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine59a");
    verify_reg_nonexist(hkey, "Wine59b");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine60a\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,\\\n"
                    ";comment\n"
                    "  65,6e,\\;comment\n"
                    "  61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine60a", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine60b\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,\\\n"
                    "  ;comment\n"
                    "  65,6e,\\;comment\n"
                    "  61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine60b", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine60c\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,\\\n"
                    "#comment\n"
                    "  65,6e,\\;comment\n"
                    "  61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine60c");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine60d\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,\\\n"
                    "  #comment\n"
                    "  65,6e,\\;comment\n"
                    "  61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine60d");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine60e\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,\\\n\n"
                    "  63,61,74,\\\n\n\n"
                    "  65,6e,\\\n\n\n\n"
                    "  61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine60e", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine60f\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,\\\n \n"
                    "  63,61,74,\\\n\t\n\t\n"
                    "  65,6e,\\\n\t \t\n\t \t\n\t \t\n"
                    "  61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine60f", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine61a\"=hex(0):25,48,4f,4d,45,25,5c,/\n"
                    "  25,50,41,54,48,25,00\n"
                    "\"Wine61b\"=hex(0):25,48,4f,4d,45,25,5c/\n"
                    "  25,50,41,54,48,25,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine61a");
    verify_reg_nonexist(hkey, "Wine61b");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine62a\"=hex(0):56,61,6c,75,65,\\", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegQueryValueExA(hkey, "Wine62a", NULL, NULL, NULL, NULL);
    ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND) /* WinXP */,
       "got %u, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine62a", REG_NONE, "Value", 5, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine62b\"=hex(2):25,50,41,54,48,25,\\", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegQueryValueExA(hkey, "Wine62b", NULL, NULL, NULL, NULL);
    ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND) /* WinXP */,
       "got %u, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine62b", REG_EXPAND_SZ, "%PATH%", 7, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine62c\"=hex:11,22,33,44,55,\\", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegQueryValueExA(hkey, "Wine62c", NULL, NULL, NULL, NULL);
    ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND) /* WinXP */,
       "got %u, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine62c", REG_BINARY, hex, 5, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine62d\"=hex(7):4c,69,6e,65,\\", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegQueryValueExA(hkey, "Wine62d", NULL, NULL, NULL, NULL);
    ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND) /* WinXP */,
       "got %u, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine62d", REG_MULTI_SZ, "Line", 5, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine62e\"=hex(100):56,61,6c,75,65,\\", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegQueryValueExA(hkey, "Wine62e", NULL, NULL, NULL, NULL);
    ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND) /* WinXP */,
       "got %u, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine62e", 0x100, "Value", 5, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine62f\"=hex(7):4c,69,6e,65,20\\", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine62f");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine63a\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  ,63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine63b\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine63a");
    verify_reg_nonexist(hkey, "Wine63b");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine64a\"=hex(7):4c,69,6e,65,00,00\n"
                    "\"Wine64b\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine64c\"=hex(7):4c,69,6e,65,20,\\;comment\n"
                    "  63,6f,6e,63,61,74,\\\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine64d\"=hex(7):4c,69,6e,65,20,\\;comment\n"
                    "  63,6f,6e,63,61,74,\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine64e\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,;comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine64a", REG_MULTI_SZ, "Line\0", 6, 0);
    verify_reg(hkey, "Wine64b", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine64c", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    /* Wine64d */
    size = sizeof(buffer);
    err = RegQueryValueExA(hkey, "Wine64d", NULL, &type, (BYTE *)&buffer, &size);
    ok(err == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", err);
    ok(type == REG_MULTI_SZ, "got wrong type %u, expected %u\n", type, REG_MULTI_SZ);
    ok(size == 12 || broken(size == 11) /* WinXP */, "got wrong size %u, expected 12\n", size);
    ok(memcmp(buffer, "Line concat", size) == 0, "got wrong data\n");
    /* Wine64e */
    size = sizeof(buffer);
    err = RegQueryValueExA(hkey, "Wine64e", NULL, &type, (BYTE *)&buffer, &size);
    ok(err == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", err);
    ok(type == REG_MULTI_SZ, "got wrong type %u, expected %u\n", type, REG_MULTI_SZ);
    ok(size == 12 || broken(size == 11) /* WinXP */, "got wrong size %u, expected 12\n", size);
    ok(memcmp(buffer, "Line concat", size) == 0, "got wrong data\n");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine65a\"=hex(100):25,50,41,54,48,25,00\n"
                    "\"Wine65b\"=hex(100):25,50,41,\\\n"
                    "  54,48,25,00\n"
                    "\"Wine65c\"=hex(100):25,50,41,\\;comment\n"
                    "  54,48,\\\n"
                    "  25,00\n"
                    "\"Wine65d\"=hex(100):25,50,41,\\;comment\n"
                    "  54,48,\n"
                    "  25,00\n"
                    "\"Wine65e\"=hex(100):25,50,41,\\;comment\n"
                    "  54,48,;comment\n"
                    "  25,00\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine65a", 0x100, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine65b", 0x100, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine65c", 0x100, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine65d", 0x100, "%PATH", 5, 0);
    verify_reg(hkey, "Wine65e", 0x100, "%PATH", 5, 0);

    /* Test null-termination of REG_EXPAND_SZ and REG_MULTI_SZ data*/
    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine66a\"=hex(7):4c,69,6e,65\n"
                    "\"Wine66b\"=hex(7):4c,69,6e,65,\n"
                    "\"Wine66c\"=hex(7):4c,69,6e,65,00\n"
                    "\"Wine66d\"=hex(7):4c,69,6e,65,00,\n"
                    "\"Wine66e\"=hex(7):4c,69,6e,65,00,00\n"
                    "\"Wine66f\"=hex(7):4c,69,6e,65,00,00,\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine66a", REG_MULTI_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine66b", REG_MULTI_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine66c", REG_MULTI_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine66d", REG_MULTI_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine66e", REG_MULTI_SZ, "Line\0", 6, 0);
    verify_reg(hkey, "Wine66f", REG_MULTI_SZ, "Line\0", 6, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine67a\"=hex(2):25,50,41,54,48,25\n"
                    "\"Wine67b\"=hex(2):25,50,41,54,48,25,\n"
                    "\"Wine67c\"=hex(2):25,50,41,54,48,25,00\n"
                    "\"Wine67d\"=hex(2):25,50,41,54,48,25,00,\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine67a", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine67b", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine67c", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine67d", REG_EXPAND_SZ, "%PATH%", 7, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine68a\"=hex(1):\n"
                    "\"Wine68b\"=hex(2):\n"
                    "\"Wine68c\"=hex(3):\n"
                    "\"Wine68d\"=hex(4):\n"
                    "\"Wine68e\"=hex(7):\n"
                    "\"Wine68f\"=hex(100):\n"
                    "\"Wine68g\"=hex(abcd):\n"
                    "\"Wine68h\"=hex:\n"
                    "\"Wine68i\"=hex(0):\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine68a", REG_SZ, NULL, 0, 0);
    verify_reg(hkey, "Wine68b", REG_EXPAND_SZ, NULL, 0, 0);
    verify_reg(hkey, "Wine68c", REG_BINARY, NULL, 0, 0);
    verify_reg(hkey, "Wine68d", REG_DWORD, NULL, 0, 0);
    verify_reg(hkey, "Wine68e", REG_MULTI_SZ, NULL, 0, 0);
    verify_reg(hkey, "Wine68f", 0x100, NULL, 0, 0);
    verify_reg(hkey, "Wine68g", 0xabcd, NULL, 0, 0);
    verify_reg(hkey, "Wine68h", REG_BINARY, NULL, 0, 0);
    verify_reg(hkey, "Wine68i", REG_NONE, NULL, 0, 0);

    /* Test with embedded null characters */
    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine69a\"=\"\\0\n"
                    "\"Wine69b\"=\"\\0\\0\n"
                    "\"Wine69c\"=\"Value1\\0\n"
                    "\"Wine69d\"=\"Value2\\0\\0\\0\\0\n"
                    "\"Wine69e\"=\"Value3\\0Value4\n"
                    "\"Wine69f\"=\"\\0Value4\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine69a");
    verify_reg_nonexist(hkey, "Wine69b");
    verify_reg_nonexist(hkey, "Wine69c");
    verify_reg_nonexist(hkey, "Wine69d");
    verify_reg_nonexist(hkey, "Wine69e");
    verify_reg_nonexist(hkey, "Wine69f");

    err = RegCloseKey(hkey);
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    delete_key(HKEY_CURRENT_USER, KEY_BASE);
}

static void test_unicode_import(void)
{
    DWORD r, dword = 0x123, type, size;
    HKEY hkey, subkey;
    LONG err;
    char buffer[24];
    BYTE hex[8];

    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(err == ERROR_SUCCESS || err == ERROR_FILE_NOT_FOUND, "got %d\n", err);

    test_import_wstr("REGEDIT\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_wstr("REGEDIT4\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_wstr("\xef\xbb\xbfREGEDIT", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbfREGEDIT\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbfREGEDIT4", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbf REGEDIT4\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbf\tREGEDIT4\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbf\nREGEDIT4\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    open_key(HKEY_CURRENT_USER, KEY_BASE, KEY_SET_VALUE, &hkey);

    test_import_wstr("\xef\xbb\xbfREGEDIT3\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test1\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Test1");

    test_import_wstr("\xef\xbb\xbfregedit4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test2\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    verify_reg_nonexist(hkey, "Test2");

    test_import_wstr("\xef\xbb\xbfRegedit4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test3\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    verify_reg_nonexist(hkey, "Test3");

    test_import_wstr("\xef\xbb\xbfREGEDIT 4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test4\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Test4");

    test_import_wstr("\xef\xbb\xbfREGEDIT4FOO\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test5\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Test5");

    test_import_wstr("\xef\xbb\xbfREGEDIT4 FOO\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test6\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Test6");

    test_import_wstr("\xef\xbb\xbfREGEDIT5\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test7\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Test7");

    test_import_wstr("\xef\xbb\xbfREGEDIT9\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test8\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Test8");

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Unicode1\"=\"Value1\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Unicode1", REG_SZ, "Value1", 7, 0);

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Unicode2\"=\"Value2\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Unicode2", REG_SZ, "Value2", 7, 0);

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Unicode3\"=\"Value3\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Unicode3", REG_SZ, "Value3", 7, 0);

    test_import_wstr("Windows Registry Editor Version 4.00\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_wstr("Windows Registry Editor Version 5.00\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbfWINDOWS Registry Editor Version 5.00\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_wstr("\xef\xbb\xbf Windows Registry Editor Version 5.00\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbf\tWindows Registry Editor Version 5.00\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbf\nWindows Registry Editor Version 5.00\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 4.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test9\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    verify_reg_nonexist(hkey, "Test9");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test10\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    verify_reg_nonexist(hkey, "Test10");

    test_import_wstr("\xef\xbb\xbfWINDOWS REGISTRY EDITOR VERSION 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test11\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    verify_reg_nonexist(hkey, "Test11");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test12\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    verify_reg_nonexist(hkey, "Test12");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine\"=dword:00000123\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "@=\"Test string\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, NULL, REG_SZ, "Test string", 12, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Unicode4\"=\"Value4\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Unicode4", REG_SZ, "Value4", 7, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Unicode5\"=\"Value5\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Unicode5", REG_SZ, "Value5", 7, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Unicode6\"=\"Value6\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Unicode6", REG_SZ, "Value6", 7, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Line1\"=\"Value1\"\n\n"
                     "\"Line2\"=\"Value2\"\n\n\n"
                     "\"Line3\"=\"Value3\"\n\n\n\n"
                     "\"Line4\"=\"Value4\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Line1", REG_SZ, "Value1", 7, 0);
    verify_reg(hkey, "Line2", REG_SZ, "Value2", 7, 0);
    verify_reg(hkey, "Line3", REG_SZ, "Value3", 7, 0);
    verify_reg(hkey, "Line4", REG_SZ, "Value4", 7, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine1\"=dword:00000782\n\n"
                     "\"Wine2\"=\"Test Value\"\n"
                     "\"Wine3\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,63,00,6f,00,6e,00,63,00,\\\n"
                     "  61,00,74,00,65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "#comment\n"
                     "@=\"Test\"\n"
                     ";comment\n\n"
                     "\"Wine4\"=dword:12345678\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x782;
    verify_reg(hkey, "Wine1", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine2", REG_SZ, "Test Value", 11, 0);
    verify_reg(hkey, "Wine3", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "", REG_SZ, "Test", 5, 0);
    dword = 0x12345678;
    verify_reg(hkey, "Wine4", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine5\"=\"No newline\"", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegQueryValueExA(hkey, "Wine5", NULL, NULL, NULL, NULL);
    ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND /* WinXP */),
       "got %d, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine5", REG_SZ, "No newline", 11, 0);

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine6\"=dword:00000050\n\n"
                     "\"Wine7\"=\"No newline\"", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x50;
    verify_reg(hkey, "Wine6", REG_DWORD, &dword, sizeof(dword), 0);
    err = RegQueryValueExA(hkey, "Wine7", NULL, NULL, NULL, NULL);
    ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND /* WinXP */),
       "got %d, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine7", REG_SZ, "No newline", 11, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "#comment\\\n"
                     "\"Wine8\"=\"Line 1\"\n"
                     ";comment\\\n"
                     "\"Wine9\"=\"Line 2\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine8", REG_SZ, "Line 1", 7, 0);
    verify_reg(hkey, "Wine9", REG_SZ, "Line 2", 7, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine10\"=\"Value 1\"#comment\n"
                     "\"Wine11\"=\"Value 2\";comment\n"
                     "\"Wine12\"=dword:01020304 #comment\n"
                     "\"Wine13\"=dword:02040608 ;comment\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine10");
    verify_reg(hkey, "Wine11", REG_SZ, "Value 2", 8, 0);
    verify_reg_nonexist(hkey, "Wine12");
    dword = 0x2040608;
    verify_reg(hkey, "Wine13", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine14\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,63,00,6f,00,6e,00,63,00,\\\n"
                     "  #comment\n"
                     "  61,00,74,00,65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "\"Wine15\"=\"A valid line\"\n"
                     "\"Wine16\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,63,00,6f,00,6e,00,63,00,\\\n"
                     "  ;comment\n"
                     "  61,00,74,00,65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "\"Wine17\"=\"Another valid line\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine14");
    verify_reg(hkey, "Wine15", REG_SZ, "A valid line", 13, 0);
    verify_reg(hkey, "Wine16", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine17", REG_SZ, "Another valid line", 19, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "#\"Comment1\"=\"Value 1\"\n"
                     ";\"Comment2\"=\"Value 2\"\n"
                     "    #\"Comment3\"=\"Value 3\"\n"
                     "    ;\"Comment4\"=\"Value 4\"\n"
                     "\"Wine18\"=\"Value 6\"#\"Comment5\"=\"Value 5\"\n"
                     "\"Wine19\"=\"Value 7\";\"Comment6\"=\"Value 6\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Comment1");
    verify_reg_nonexist(hkey, "Comment2");
    verify_reg_nonexist(hkey, "Comment3");
    verify_reg_nonexist(hkey, "Comment4");
    verify_reg_nonexist(hkey, "Wine18");
    verify_reg_nonexist(hkey, "Comment5");
    verify_reg(hkey, "Wine19", REG_SZ, "Value 7", 8, 0);
    verify_reg_nonexist(hkey, "Comment6");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine20\"=#\"Value 8\"\n"
                     "\"Wine21\"=;\"Value 9\"\n"
                     "\"Wine22\"=\"#comment1\"\n"
                     "\"Wine23\"=\";comment2\"\n"
                     "\"Wine24\"=\"Value#comment3\"\n"
                     "\"Wine25\"=\"Value;comment4\"\n"
                     "\"Wine26\"=\"Value #comment5\"\n"
                     "\"Wine27\"=\"Value ;comment6\"\n"
                     "\"Wine28\"=#dword:00000001\n"
                     "\"Wine29\"=;dword:00000002\n"
                     "\"Wine30\"=dword:00000003#comment\n"
                     "\"Wine31\"=dword:00000004;comment\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine20");
    verify_reg_nonexist(hkey, "Wine21");
    verify_reg(hkey, "Wine22", REG_SZ, "#comment1", 10, 0);
    verify_reg(hkey, "Wine23", REG_SZ, ";comment2", 10, 0);
    verify_reg(hkey, "Wine24", REG_SZ, "Value#comment3", 15, 0);
    verify_reg(hkey, "Wine25", REG_SZ, "Value;comment4", 15, 0);
    verify_reg(hkey, "Wine26", REG_SZ, "Value #comment5", 16, 0);
    verify_reg(hkey, "Wine27", REG_SZ, "Value ;comment6", 16, 0);
    verify_reg_nonexist(hkey, "Wine28");
    verify_reg_nonexist(hkey, "Wine29");
    verify_reg_nonexist(hkey, "Wine30");
    dword = 0x00000004;
    verify_reg(hkey, "Wine31", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine32a\"=dword:1\n"
                     "\"Wine32b\"=dword:4444\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x1;
    verify_reg(hkey, "Wine32a", REG_DWORD, &dword, sizeof(dword), 0);
    dword = 0x4444;
    verify_reg(hkey, "Wine32b", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine33a\"=dword:\n"
                     "\"Wine33b\"=dword:hello\n"
                     "\"Wine33c\"=dword:123456789\n"
                     "\"Wine33d\"=dword:012345678\n"
                     "\"Wine33e\"=dword:000000001\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine33a");
    verify_reg_nonexist(hkey, "Wine33b");
    verify_reg_nonexist(hkey, "Wine33c");
    verify_reg_nonexist(hkey, "Wine33d");
    verify_reg_nonexist(hkey, "Wine33e");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine34a\"=dword:12345678abc\n"
                     "\"Wine34b\"=dword:12345678 abc\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine34a");
    verify_reg_nonexist(hkey, "Wine34b");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine35a\"=dword:0x123\n"
                     "\"Wine35b\"=dword:123 456\n"
                     "\"Wine35c\"=dword:1234 5678\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine35a");
    verify_reg_nonexist(hkey, "Wine35b");
    verify_reg_nonexist(hkey, "Wine35c");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine36a\"=dword:1234;5678\n"
                     "\"Wine36b\"=dword:1234 ;5678\n"
                     "\"Wine36c\"=dword:1234#5678\n"
                     "\"Wine36d\"=dword:1234 #5678\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x1234;
    verify_reg(hkey, "Wine36a", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine36b", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg_nonexist(hkey, "Wine36c");
    verify_reg_nonexist(hkey, "Wine36d");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine37a\"=\"foo\"bar\"\n"
                     "\"Wine37b\"=\"foo\"\"bar\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine37a");
    verify_reg_nonexist(hkey, "Wine37b");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Empty string\"=\"\"\n"
                     "\"\"=\"Default registry value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Empty string", REG_SZ, "", 1, 0);
    verify_reg(hkey, NULL, REG_SZ, "Default registry value", 23, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test38a\"=\n"
                     "\"Test38b\"=\\\"\n"
                     "\"Test38c\"=\\\"Value\\\"\n"
                     "\"Test38d\"=\\\"Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Test38a");
    verify_reg_nonexist(hkey, "Test38b");
    verify_reg_nonexist(hkey, "Test38c");
    verify_reg_nonexist(hkey, "Test38d");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine39a\"=\"Value1\"  ;comment\n"
                     "\"Wine39b\"=\"Value2\"\t\t;comment\n"
                     "\"Wine39c\"=\"Value3\"  #comment\n"
                     "\"Wine39d\"=\"Value4\"\t\t#comment\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine39a", REG_SZ, "Value1", 7, 0);
    verify_reg(hkey, "Wine39b", REG_SZ, "Value2", 7, 0);
    verify_reg_nonexist(hkey, "Wine39c");
    verify_reg_nonexist(hkey, "Wine39d");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"TestNoBeginQuote\"=Asdffdsa\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "TestNoBeginQuote");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"TestNoEndQuote\"=\"Asdffdsa\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "TestNoEndQuote");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"TestNoQuotes\"=Asdffdsa\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "TestNoQuotes");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "NameNoBeginQuote\"=\"Asdffdsa\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "NameNoBeginQuote");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"NameNoEndQuote=\"Asdffdsa\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "NameNoEndQuote");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "NameNoQuotes=\"Asdffdsa\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "NameNoQuotes");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                 "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                 "\"MixedQuotes=Asdffdsa\"\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "MixedQuotes");
    verify_reg_nonexist(hkey, "MixedQuotes=Asdffdsa");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine40a\"=hex(2):4c,00,69,00,6e,00,65,00,00,00\n"
                     "\"Wine40b\"=\"Value 1\"\n"
                     "\"Wine40c\"=hex(2):4c,00,69,00,6e,00,65,00\\\n"
                     "\"Wine40d\"=\"Value 2\"\n"
                     "\"Wine40e\"=hex(2):4c,00,69,00,6e,00,65,00,\\\n"
                     "\"Wine40f\"=\"Value 3\"\n"
                     "\"Wine40g\"=\"Value 4\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine40a", REG_EXPAND_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine40b", REG_SZ, "Value 1", 8, 0);
    verify_reg_nonexist(hkey, "Wine40c");
    verify_reg(hkey, "Wine40d", REG_SZ, "Value 2", 8, 0);
    verify_reg_nonexist(hkey, "Wine40e");
    verify_reg_nonexist(hkey, "Wine40f");
    verify_reg(hkey, "Wine40g", REG_SZ, "Value 4", 8, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line1\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,\\;comment\n"
                     "  63,00,61,00,74,00,\\;comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Multi-Line1", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line2\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,\\;comment\n"
                     "  63,00,61,00,74,00,;comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Multi-Line2", REG_MULTI_SZ, "Line concat", 12, 0);

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line3\"=hex(7):4c,69,6e,65,20\\\n"
                     ",63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Multi-Line3");

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line4\"=hex(7):4c,69,6e,65,20\\\n"
                     "  ,63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Multi-Line4");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line5\"=hex(7):4c,69,6e,65,20\\\n"
                     ",63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Multi-Line5");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line6\"=hex(7):4c,69,6e,65,20\\\n"
                     "  ,63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Multi-Line6");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line7\"=hex(7):4c,00,69,00,6e,00,\\;comment\n"
                     "  65,00,20,00,\\;comment\n"
                     "  63,00,6f,00,6e,00,\\;comment\n"
                     "  63,00,61,00,74,00,\\;comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Multi-Line7", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line8\"=hex(7):4c,00,69,00,6e,00,\\;#comment\n"
                     "  65,00,20,00,\\;#comment\n"
                     "  63,00,6f,00,6e,00,\\;#comment\n"
                     "  63,00,61,00,74,00,\\;#comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Multi-Line8", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line9\"=hex(7):4c,00,69,00,6e,00,\\;comment\n"
                     "  65,00,20,00,\\;comment\n"
                     "  63,00,6f,00,6e,00,\\;comment\n"
                     "  63,00,61,00,74,00,\\#comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Multi-Line9");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line10\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,\\;comment\n"
                     "  63,00,61,00,74,00,\\\n\n"
                     "  65,00,6e,00,\\;comment\n\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Multi-Line10", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine41a\"=dword:1234\\\n"
                     "5678\n"
                     "\"Wine41b\"=\"Test \\\n"
                     "Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine41a");
    verify_reg_nonexist(hkey, "Wine41b");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"double\\\"quote\"=\"valid \\\"or\\\" not\"\n"
                     "\"single'quote\"=dword:00000008\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "double\"quote", REG_SZ, "valid \"or\" not", 15, 0);
    dword = 0x00000008;
    verify_reg(hkey, "single'quote", REG_DWORD, &dword, sizeof(dword), 0);

    /* Test key name and value name concatenation */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\\n"
                     "Subkey1]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey1");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\n"
                     "\\Subkey2]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey2");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine\\\n"
                     "42a\"=\"Value 1\"\n"
                     "\"Wine42b\"=\"Value 2\"\n"
                     "\"Wine\n"
                     "\\42c\"=\"Value 3\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine42a");
    verify_reg(hkey, "Wine42b", REG_SZ, "Value 2", 8, 0);
    verify_reg_nonexist(hkey, "Wine42c");

    /* Test hex data concatenation for REG_NONE, REG_EXPAND_SZ and REG_BINARY */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine43a\"=hex(0):56,00,61,00,6c,00,75,00,65,00,00,00\n"
                     "\"Wine43b\"=hex(0):56,00,61,00,6c,00,\\\n"
                     "  75,00,65,00,00,00\n"
                     "\"Wine43c\"=hex(0):56,00,61,00,6c,00\\\n"
                     ",75,00,65,00,00,00\n"
                     "\"Wine43d\"=hex(0):56,00,61,00,6c,00\\\n"
                     "  ,75,00,65,00,00,00\n"
                     "\"Wine43e\"=hex(0):56,00,61,00,6c,00\\\n"
                     "  75,00,65,00,00,00\n"
                     "\"Wine43f\"=hex(0):56,00,61,00,6c,00,7\\\n"
                     "5,00,65,00,00,00\n"
                     "\"Wine43g\"=hex(0):56,00,61,00,6c,00,7\\\n"
                     "  5,00,65,00,00,00\n"
                     "\"Wine43h\"=hex(0):56,00,61,00,\\;comment\n"
                     "  6c,00,75,00,\\\n"
                     "  65,00,00,00\n"
                     "\"Wine43i\"=hex(0):56,00,61,00,\\;comment\n"
                     "  6c,00,75,00,\n"
                     "  65,00,00,00\n"
                     "\"Wine43j\"=hex(0):56,00,61,00,\\;comment\n"
                     "  6c,00,75,00,;comment\n"
                     "  65,00,00,00\n"
                     "\"Wine43k\"=hex(0):56,00,61,00,\\;comment\n"
                     "  6c,00,75,00,\\#comment\n"
                     "  65,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine43a", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    verify_reg(hkey, "Wine43b", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    verify_reg_nonexist(hkey, "Wine43c");
    verify_reg_nonexist(hkey, "Wine43d");
    verify_reg_nonexist(hkey, "Wine43e");
    verify_reg_nonexist(hkey, "Wine43f");
    verify_reg_nonexist(hkey, "Wine43g");
    verify_reg(hkey, "Wine43h", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    verify_reg(hkey, "Wine43i", REG_NONE, "V\0a\0l\0u", 8, 0);
    verify_reg(hkey, "Wine43j", REG_NONE, "V\0a\0l\0u", 8, 0);
    verify_reg_nonexist(hkey, "Wine43k");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine44a\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine44b\"=hex(2):25,00,50,00,41,00,\\\n"
                     "  54,00,48,00,25,00,00,00\n"
                     "\"Wine44c\"=hex(2):25,00,50,00,41,00\\\n"
                     ",54,00,48,00,25,00,00,00\n"
                     "\"Wine44d\"=hex(2):25,00,50,00,41,00\\\n"
                     "  ,54,00,48,00,25,00,00,00\n"
                     "\"Wine44e\"=hex(2):25,00,50,00,41,00\\\n"
                     "  54,00,48,00,25,00,00,00\n"
                     "\"Wine44f\"=hex(2):25,00,50,00,4\\\n"
                     "1,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine44g\"=hex(2):25,00,50,00,4\\\n"
                     "  1,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine44h\"=hex(2):25,00,50,00,41,00,\\;comment\n"
                     "  54,00,48,00,\\\n"
                     "  25,00,00,00\n"
                     "\"Wine44i\"=hex(2):25,00,50,00,41,00,\\;comment\n"
                     "  54,00,48,00\n"
                     "  25,00,00,00\n"
                     "\"Wine44j\"=hex(2):25,00,50,00,41,00,\\;comment\n"
                     "  54,00,48,00;comment\n"
                     "  25,00,00,00\n"
                     "\"Wine44k\"=hex(2):25,00,50,00,41,00,\\;comment\n"
                     "  54,00,48,00,\\#comment\n"
                     "  25,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine44a", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine44b", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg_nonexist(hkey, "Wine44c");
    verify_reg_nonexist(hkey, "Wine44d");
    verify_reg_nonexist(hkey, "Wine44e");
    verify_reg_nonexist(hkey, "Wine44f");
    verify_reg_nonexist(hkey, "Wine44g");
    verify_reg(hkey, "Wine44h", REG_EXPAND_SZ, "%PATH%", 7, 0);
    /* Wine44i */
    size = sizeof(buffer);
    err = RegQueryValueExA(hkey, "Wine44i", NULL, &type, (BYTE *)&buffer, &size);
    ok(err == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", err);
    ok(type == REG_EXPAND_SZ, "got wrong type %u, expected %u\n", type, REG_EXPAND_SZ);
    ok(size == 6 || broken(size == 5) /* WinXP */, "got wrong size %u, expected 6\n", size);
    ok(memcmp(buffer, "%PATH", size) == 0, "got wrong data\n");
    /* Wine44j */
    size = sizeof(buffer);
    memset(buffer, '-', size);
    err = RegQueryValueExA(hkey, "Wine44j", NULL, &type, (BYTE *)&buffer, &size);
    ok(err == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", err);
    ok(type == REG_EXPAND_SZ, "got wrong type %u, expected %u\n", type, REG_EXPAND_SZ);
    ok(size == 6 || broken(size == 5) /* WinXP */, "got wrong size %u, expected 6\n", size);
    ok(memcmp(buffer, "%PATH", size) == 0, "got wrong data\n");
    /* Wine44k */
    verify_reg_nonexist(hkey, "Wine44k");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine45a\"=hex:11,22,33,44,55,66,77,88\n"
                     "\"Wine45b\"=hex:11,22,33,44,\\\n"
                     "  55,66,77,88\n"
                     "\"Wine45c\"=hex:11,22,33,44\\\n"
                     ",55,66,77,88\n"
                     "\"Wine45d\"=hex:11,22,33,44\\\n"
                     "  ,55,66,77,88\n"
                     "\"Wine45e\"=hex:11,22,33,44\\\n"
                     "  55,66,77,88\n"
                     "\"Wine45f\"=hex:11,22,33,4\\\n"
                     "4,55,66,77,88\n"
                     "\"Wine45g\"=hex:11,22,33,4\\\n"
                     "  4,55,66,77,88\n"
                     "\"Wine45h\"=hex:11,22,33,44,\\;comment\n"
                     "  55,66,\\\n"
                     "  77,88\n"
                     "\"Wine45i\"=hex:11,22,33,44,\\;comment\n"
                     "  55,66,\n"
                     "  77,88\n"
                     "\"Wine45j\"=hex:11,22,33,44,\\;comment\n"
                     "  55,66,;comment\n"
                     "  77,88\n"
                     "\"Wine45k\"=hex:11,22,33,\\;comment\n"
                     "  44,55,66,\\#comment\n"
                     "  77,88\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    hex[0] = 0x11; hex[1] = 0x22; hex[2] = 0x33; hex[3] = 0x44;
    hex[4] = 0x55; hex[5] = 0x66; hex[6] = 0x77; hex[7] = 0x88;
    verify_reg(hkey, "Wine45a", REG_BINARY, hex, sizeof(hex), 0);
    verify_reg(hkey, "Wine45b", REG_BINARY, hex, sizeof(hex), 0);
    verify_reg_nonexist(hkey, "Wine45c");
    verify_reg_nonexist(hkey, "Wine45d");
    verify_reg_nonexist(hkey, "Wine45e");
    verify_reg_nonexist(hkey, "Wine45f");
    verify_reg_nonexist(hkey, "Wine45g");
    verify_reg(hkey, "Wine45h", REG_BINARY, hex, sizeof(hex), 0);
    verify_reg(hkey, "Wine45i", REG_BINARY, hex, 6, 0);
    verify_reg(hkey, "Wine45j", REG_BINARY, hex, 6, 0);
    verify_reg_nonexist(hkey, "Wine45k");

    /* Test import with subkeys */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey\"1]\n"
                     "\"Wine\\\\31\"=\"Test value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    open_key(hkey, "Subkey\"1", 0, &subkey);
    verify_reg(subkey, "Wine\\31", REG_SZ, "Test value", 11, 0);
    err = RegCloseKey(subkey);
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);
    delete_key(HKEY_CURRENT_USER, KEY_BASE "\\Subkey\"1");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey/2]\n"
                     "\"123/\\\"4;'5\"=\"Random value name\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    open_key(hkey, "Subkey/2", 0, &subkey);
    verify_reg(subkey, "123/\"4;'5", REG_SZ, "Random value name", 18, 0);
    err = RegCloseKey(subkey);
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);
    delete_key(HKEY_CURRENT_USER, KEY_BASE "\\Subkey/2");

    /* Test key creation */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "HKEY_CURRENT_USER\\" KEY_BASE "\\No_Opening_Bracket]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "No_Opening_Bracket");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\No_Closing_Bracket\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "No_Closing_Bracket");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[ HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1a]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey1a");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[\tHKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1b]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey1b");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1c ]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey1c ");
    delete_key(hkey, "Subkey1c ");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1d\t]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey1d\t");
    delete_key(hkey, "Subkey1d\t");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1e\\]\n"
                     "\"Wine\"=\"Test value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey1e\\");
    verify_key(hkey, "Subkey1e");
    open_key(hkey, "Subkey1e", 0, &subkey);
    verify_reg(subkey, "Wine", REG_SZ, "Test value", 11, 0);
    RegCloseKey(subkey);
    delete_key(hkey, "Subkey1e");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1f\\\\]\n"
                     "\"Wine\"=\"Test value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey1f\\\\");
    verify_key(hkey, "Subkey1f\\");
    verify_key(hkey, "Subkey1f");
    open_key(hkey, "Subkey1f\\\\", 0, &subkey);
    verify_reg(subkey, "Wine", REG_SZ, "Test value", 11, 0);
    RegCloseKey(subkey);
    delete_key(hkey, "Subkey1f\\\\");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1g\\\\\\\\]\n"
                     "\"Wine\"=\"Test value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey1g\\\\\\\\");
    verify_key(hkey, "Subkey1g\\\\");
    verify_key(hkey, "Subkey1g\\");
    verify_key(hkey, "Subkey1g");
    open_key(hkey, "Subkey1g\\\\", 0, &subkey);
    verify_reg(subkey, "Wine", REG_SZ, "Test value", 11, 0);
    RegCloseKey(subkey);
    delete_key(hkey, "Subkey1g\\\\");

    /* Test key deletion. We start by creating some registry keys. */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2b]\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey2a");
    verify_key(hkey, "Subkey2b");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[ -HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey2a");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[\t-HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2b]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey2b");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[- HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey2a");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[-\tHKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2b]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey2b");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[-HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2a]\n\n"
                     "[-HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2b]\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey2a");
    verify_key_nonexist(hkey, "Subkey2b");

    /* Test case sensitivity when creating and deleting registry keys. */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[hkey_CURRENT_user\\" KEY_BASE "\\Subkey3a]\n\n"
                     "[HkEy_CuRrEnT_uSeR\\" KEY_BASE "\\SuBkEy3b]\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey3a");
    verify_key(hkey, "Subkey3b");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[-HKEY_current_USER\\" KEY_BASE "\\sUBKEY3A]\n\n"
                     "[-hKeY_cUrReNt_UsEr\\" KEY_BASE "\\sUbKeY3B]\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey3a");
    verify_key_nonexist(hkey, "Subkey3b");

    /* Test mixed key creation and deletion. We start by creating a subkey. */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey4a]\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "Subkey4a");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n"
                     "[-HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey4a]\n"
                     "\"Wine46a\"=dword:12345678\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey4a");
    verify_reg_nonexist(hkey, "Wine46a");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n"
                     "[HKEY_CURRENT_USERS\\" KEY_BASE "\\Subkey4b]\n"
                     "\"Wine46b\"=dword:12345678\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key_nonexist(hkey, "Subkey4b");
    verify_reg_nonexist(hkey, "Wine46b");

    /* Test value deletion. We start by creating some registry values. */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine46a\"=\"Test Value\"\n"
                     "\"Wine46b\"=dword:00000008\n"
                     "\"Wine46c\"=hex:11,22,33,44\n"
                     "\"Wine46d\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "\"Wine46e\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine46f\"=hex(0):56,00,61,00,6c,00,75,00,65,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine46a", REG_SZ, "Test Value", 11, 0);
    verify_reg(hkey, "Wine46b", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine46c", REG_BINARY, hex, 4, 0);
    verify_reg(hkey, "Wine46d", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine46e", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine46f", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine46a\"=-\n"
                     "\"Wine46b\"=  -\n"
                     "\"Wine46c\"=  \t-\t  \n"
                     "\"Wine46d\"=-\"Test\"\n"
                     "\"Wine46e\"=- ;comment\n"
                     "\"Wine46f\"=- #comment\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine46a");
    verify_reg_nonexist(hkey, "Wine46b");
    verify_reg_nonexist(hkey, "Wine46c");
    verify_reg(hkey, "Wine46d", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg_nonexist(hkey, "Wine46e");
    verify_reg(hkey, "Wine46f", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);

    /* Test the accepted range of the hex-based data types */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine47a\"=hex(0):56,61,6c,75,65,00\n"
                     "\"Wine47b\"=hex(10):56,61,6c,75,65,00\n"
                     "\"Wine47c\"=hex(100):56,61,6c,75,65,00\n"
                     "\"Wine47d\"=hex(1000):56,61,6c,75,65,00\n"
                     "\"Wine47e\"=hex(7fff):56,61,6c,75,65,00\n"
                     "\"Wine47f\"=hex(ffff):56,61,6c,75,65,00\n"
                     "\"Wine47g\"=hex(7fffffff):56,61,6c,75,65,00\n"
                     "\"Wine47h\"=hex(ffffffff):56,61,6c,75,65,00\n"
                     "\"Wine47i\"=hex(100000000):56,61,6c,75,65,00\n"
                     "\"Wine47j\"=hex(0x2):56,00,61,00,6c,00,75,00,65,00,00,00\n"
                     "\"Wine47k\"=hex(0X2):56,00,61,00,6c,00,75,00,65,00,00,00\n"
                     "\"Wine47l\"=hex(x2):56,00,61,00,6c,00,75,00,65,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine47a", REG_NONE, "Value", 6, 0);
    verify_reg(hkey, "Wine47b", 0x10, "Value", 6, 0);
    verify_reg(hkey, "Wine47c", 0x100, "Value", 6, 0);
    verify_reg(hkey, "Wine47d", 0x1000, "Value", 6, 0);
    verify_reg(hkey, "Wine47e", 0x7fff, "Value", 6, 0);
    verify_reg(hkey, "Wine47f", 0xffff, "Value", 6, 0);
    verify_reg(hkey, "Wine47g", 0x7fffffff, "Value", 6, 0);
    verify_reg(hkey, "Wine47h", 0xffffffff, "Value", 6, 0);
    verify_reg_nonexist(hkey, "Wine47i");
    verify_reg_nonexist(hkey, "Wine47j");
    verify_reg_nonexist(hkey, "Wine47k");
    verify_reg_nonexist(hkey, "Wine47l");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine48a\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,  \\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,    \\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "\"Wine48b\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\t\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\t  \t  \\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine48a", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine48b", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine49\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00,\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine49", REG_EXPAND_SZ, "%PATH%", 7, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine50a\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00  ;comment\n"
                     "\"Wine50b\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\t;comment\n"
                     "\"Wine50c\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00  #comment\n"
                     "\"Wine50d\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\t#comment\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine50a", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine50b", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg_nonexist(hkey, "Wine50c");
    verify_reg_nonexist(hkey, "Wine50d");

    /* Test support for characters greater than 0xff */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine51a\"=hex(0):25,50,100,54,48,25,00\n"
                     "\"Wine51b\"=hex(0):25,1a4,100,164,124,25,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine51a");
    verify_reg_nonexist(hkey, "Wine51b");

    /* Test the effect of backslashes in hex data */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine52a\"=hex(2):25,00,48\\,00,4f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine52b\"=hex(2):25,00,48,00,\\4f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine52c\"=hex(2):25,00,48\\ ,00,4f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine52d\"=hex(2):25,00,48,00,\\ 4f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine52e\"=hex(2):\\25,00,48,00,4f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine52f\"=hex(2):\\ 25,00,48,00,4f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine52g\"=hex(2):25,00,48,00,4\\f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine52h\"=hex(2):25,00,48,00,4\\\n"
                     "  f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine52i\"=hex(2):25,00,50,00,\\,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine52j\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,\\\\\n"
                     "  25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine52k\"=hex(2):,\\\n"
                     "  25,00,48,00,4f,00,4d,00,45,00,25,00,00,00\n"
                     "\"Wine52l\"=hex(2):\\\n"
                     "  25,00,48,00,4f,00,4d,00,45,00,25,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine52a");
    verify_reg_nonexist(hkey, "Wine52b");
    verify_reg_nonexist(hkey, "Wine52c");
    verify_reg_nonexist(hkey, "Wine52d");
    verify_reg_nonexist(hkey, "Wine52e");
    verify_reg_nonexist(hkey, "Wine52f");
    verify_reg_nonexist(hkey, "Wine52g");
    verify_reg_nonexist(hkey, "Wine52h");
    verify_reg_nonexist(hkey, "Wine52i");
    verify_reg_nonexist(hkey, "Wine52j");
    verify_reg_nonexist(hkey, "Wine52k");
    verify_reg(hkey, "Wine52l", REG_EXPAND_SZ, "%HOME%", 7, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine53a\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,\\\n"
                     "  25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine53b\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00\\\n"
                     "  25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine53c\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,  \\  ;comment\n"
                     "  25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine53d\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00  \\  ;comment\n"
                     "  25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine53e\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,\\\t  ;comment\n"
                     "  25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine53f\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00\\\t  ;comment\n"
                     "  25,00,50,00,41,00,54,00,48,00,25,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine53a", REG_EXPAND_SZ, "%HOME%\\%PATH%", 14, 0);
    verify_reg_nonexist(hkey, "Wine53b");
    verify_reg(hkey, "Wine53c", REG_EXPAND_SZ, "%HOME%\\%PATH%", 14, 0);
    verify_reg_nonexist(hkey, "Wine53d");
    verify_reg(hkey, "Wine53e", REG_EXPAND_SZ, "%HOME%\\%PATH%", 14, 0);
    verify_reg_nonexist(hkey, "Wine53f");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine54a\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey1]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine54a");
    verify_key_nonexist(hkey, "Subkey1");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine54b\"=hex(2):4c,00,69,00,6e,00,65,00,20,00\\\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey2]\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine54b");
    verify_key(hkey, "Subkey2");

    delete_key(hkey, "Subkey2");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine55a\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "\"Wine55b\"=\"Test value\"\n"

                     "\"Wine55c\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     ";comment\n"
                     "\"Wine55d\"=\"Test value\"\n"

                     "\"Wine55e\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "#comment\n"
                     "\"Wine55f\"=\"Test value\"\n"

                     "\"Wine55g\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n\n"
                     "\"Wine55h\"=\"Test value\"\n"

                     "\"Wine55i\"=hex(2):4c,00,69,00,6e,00,65,00,20,00\\\n"
                     "\"Wine55j\"=\"Test value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine55a");
    verify_reg_nonexist(hkey, "Wine55b");
    verify_reg_nonexist(hkey, "Wine55c");
    verify_reg_nonexist(hkey, "Wine55d");
    verify_reg_nonexist(hkey, "Wine55e");
    verify_reg(hkey, "Wine55f", REG_SZ, "Test value", 11, 0);
    verify_reg_nonexist(hkey, "Wine55g");
    verify_reg_nonexist(hkey, "Wine55h");
    verify_reg_nonexist(hkey, "Wine55i");
    verify_reg(hkey, "Wine55j", REG_SZ, "Test value", 11, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine56a\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "\"Wine56b\"=dword:00000008\n"

                     "\"Wine56c\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     ";comment\n"
                     "\"Wine56d\"=dword:00000008\n"

                     "\"Wine56e\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "#comment\n"
                     "\"Wine56f\"=dword:00000008\n"

                     "\"Wine56g\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n\n"
                     "\"Wine56h\"=dword:00000008\n"

                     "\"Wine56i\"=hex(2):4c,00,69,00,6e,00,65,00,20,00\\\n"
                     "\"Wine56j\"=dword:00000008\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine56a");
    verify_reg_nonexist(hkey, "Wine56b");
    verify_reg_nonexist(hkey, "Wine56c");
    verify_reg_nonexist(hkey, "Wine56d");
    verify_reg_nonexist(hkey, "Wine56e");
    verify_reg(hkey, "Wine56f", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg_nonexist(hkey, "Wine56g");
    verify_reg_nonexist(hkey, "Wine56h");
    verify_reg_nonexist(hkey, "Wine56i");
    verify_reg(hkey, "Wine56j", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine57a\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,\\\n"
                     "\"Wine57b\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"

                     "\"Wine57c\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,\\\n"
                     ";comment\n"
                     "\"Wine57d\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"

                     "\"Wine57e\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,\\\n"
                     "#comment\n"
                     "\"Wine57f\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"

                     "\"Wine57g\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00,\\\n\n"
                     "\"Wine57h\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"

                     "\"Wine57i\"=hex(2):25,00,48,00,4f,00,4d,00,45,00,25,00,5c,00\\\n"
                     "\"Wine57j\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine57a");
    verify_reg_nonexist(hkey, "Wine57b");
    verify_reg_nonexist(hkey, "Wine57c");
    verify_reg_nonexist(hkey, "Wine57d");
    verify_reg_nonexist(hkey, "Wine57e");
    verify_reg(hkey, "Wine57f", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg_nonexist(hkey, "Wine57g");
    verify_reg_nonexist(hkey, "Wine57h");
    verify_reg_nonexist(hkey, "Wine57i");
    verify_reg(hkey, "Wine57j", REG_EXPAND_SZ, "%PATH%", 7, 0);

    delete_value(hkey, NULL);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine58a\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "@=\"Default value 1\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine58a");
    verify_reg_nonexist(hkey, NULL);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine58b\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     ";comment\n"
                     "@=\"Default value 2\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine58b");
    verify_reg_nonexist(hkey, NULL);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine58c\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "#comment\n"
                     "@=\"Default value 3\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine58c");
    verify_reg(hkey, NULL, REG_SZ, "Default value 3", 16, 0);

    delete_value(hkey, NULL);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine58d\"=hex(2):4c,00,69,00,6e,00,65,00,20,00,\\\n\n"
                     "@=\"Default value 4\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine58d");
    verify_reg_nonexist(hkey, NULL);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine58e\"=hex(2):4c,00,69,00,6e,00,65,00,20,00\\\n"
                     "@=\"Default value 5\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine58e");
    verify_reg(hkey, NULL, REG_SZ, "Default value 5", 16, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine59a\"=hex:11,22,33,\\\n"
                     "\\\n"
                     "  44,55,66\n"
                     "\"Wine59b\"=hex:11,22,33,\\\n"
                     "  \\\n"
                     "  44,55,66\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine59a");
    verify_reg_nonexist(hkey, "Wine59b");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine60a\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     ";comment\n"
                     "  65,00,6e,00,\\;comment\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine60a", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine60b\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  ;comment\n"
                     "  65,00,6e,00,\\;comment\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine60b", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine60c\"=hex(7):4c,69,6e,65,20,\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "#comment\n"
                     "  65,00,6e,00,\\;comment\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine60c");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine60d\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  #comment\n"
                     "  65,00,6e,00,\\;comment\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine60d");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine60e\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,\\\n\n"
                     "  63,00,61,00,74,00,\\\n\n\n"
                     "  65,00,6e,00,\\\n\n\n\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine60e", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine60f\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,\\\n \n"
                     "  63,00,61,00,74,00,\\\n\t\n\t\n"
                     "  65,00,6e,00,\\\n\t \t\n\t \t\n\t \t\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine60f", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine61a\"=hex(0):25,48,4f,4d,45,25,5c,/\n"
                     "  25,50,41,54,48,25,00\n"
                     "\"Wine61b\"=hex(0):25,48,4f,4d,45,25,5c/\n"
                     "  25,50,41,54,48,25,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine61a");
    verify_reg_nonexist(hkey, "Wine61b");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine62a\"=hex(0):56,61,6c,75,65,\\", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegQueryValueExA(hkey, "Wine62a", NULL, NULL, NULL, NULL);
    ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND) /* WinXP */,
       "got %u, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine62a", REG_NONE, "Value", 5, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine62b\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,\\", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegQueryValueExA(hkey, "Wine62b", NULL, NULL, NULL, NULL);
    ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND) /* WinXP */,
       "got %u, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine62b", REG_EXPAND_SZ, "%PATH%", 7, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine62c\"=hex:11,22,33,44,55,\\", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegQueryValueExA(hkey, "Wine62c", NULL, NULL, NULL, NULL);
    ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND) /* WinXP */,
       "got %u, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine62c", REG_BINARY, hex, 5, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine62d\"=hex(7):4c,00,69,00,6e,00,65,00,\\", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegQueryValueExA(hkey, "Wine62d", NULL, NULL, NULL, NULL);
    ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND) /* WinXP */,
       "got %u, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine62d", REG_MULTI_SZ, "Line", 5, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine62e\"=hex(100):56,61,6c,75,65,\\", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegQueryValueExA(hkey, "Wine62e", NULL, NULL, NULL, NULL);
    ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND) /* WinXP */,
       "got %u, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine62e", 0x100, "Value", 5, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine62f\"=hex(7):4c,00,69,00,6e,00,65,00,20,00\\", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine62f");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine63a\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  ,63,00,6f,00,6e,00,\\\n"
                     "  63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,\\\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "\"Wine63b\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,,00,6f,00,6e,00,\\\n"
                     "  63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,\\\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine63a");
    verify_reg_nonexist(hkey, "Wine63b");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine64a\"=hex(7):4c,00,69,00,6e,00,65,00,00,00,00,00\n"
                     "\"Wine64b\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "\"Wine64c\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\;comment\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\\\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "\"Wine64d\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\;comment\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n"
                     "\"Wine64e\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,63,00,61,00,74,00,;comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine64a", REG_MULTI_SZ, "Line\0", 6, 0);
    verify_reg(hkey, "Wine64b", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine64c", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    /* Wine64d */
    size = sizeof(buffer);
    err = RegQueryValueExA(hkey, "Wine64d", NULL, &type, (BYTE *)&buffer, &size);
    ok(err == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", err);
    ok(type == REG_MULTI_SZ, "got wrong type %u, expected %u\n", type, REG_MULTI_SZ);
    ok(size == 12 || broken(size == 11) /* WinXP */, "got wrong size %u, expected 12\n", size);
    ok(memcmp(buffer, "Line concat", size) == 0, "got wrong data\n");
    /* Wine64e */
    size = sizeof(buffer);
    err = RegQueryValueExA(hkey, "Wine64e", NULL, &type, (BYTE *)&buffer, &size);
    ok(err == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", err);
    ok(type == REG_MULTI_SZ, "got wrong type %u, expected %u\n", type, REG_MULTI_SZ);
    ok(size == 12 || broken(size == 11) /* WinXP */, "got wrong size %u, expected 12\n", size);
    ok(memcmp(buffer, "Line concat", size) == 0, "got wrong data\n");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine65a\"=hex(100):25,50,41,54,48,25,00\n"
                     "\"Wine65b\"=hex(100):25,50,41,\\\n"
                     "  54,48,25,00\n"
                     "\"Wine65c\"=hex(100):25,50,41,\\;comment\n"
                     "  54,48,\\\n"
                     "  25,00\n"
                     "\"Wine65d\"=hex(100):25,50,41,\\;comment\n"
                     "  54,48,\n"
                     "  25,00\n"
                     "\"Wine65e\"=hex(100):25,50,41,\\;comment\n"
                     "  54,48,;comment\n"
                     "  25,00\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine65a", 0x100, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine65b", 0x100, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine65c", 0x100, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine65d", 0x100, "%PATH", 5, 0);
    verify_reg(hkey, "Wine65e", 0x100, "%PATH", 5, 0);

    /* Test null-termination of REG_EXPAND_SZ and REG_MULTI_SZ data*/
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine66a\"=hex(7):4c,00,69,00,6e,00,65,00\n"
                     "\"Wine66b\"=hex(7):4c,00,69,00,6e,00,65,00,\n"
                     "\"Wine66c\"=hex(7):4c,00,69,00,6e,00,65,00,00,00\n"
                     "\"Wine66d\"=hex(7):4c,00,69,00,6e,00,65,00,00,00,\n"
                     "\"Wine66e\"=hex(7):4c,00,69,00,6e,00,65,00,00,00,00,00\n"
                     "\"Wine66f\"=hex(7):4c,00,69,00,6e,00,65,00,00,00,00,00,\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine66a", REG_MULTI_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine66b", REG_MULTI_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine66c", REG_MULTI_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine66d", REG_MULTI_SZ, "Line", 5, 0);
    verify_reg(hkey, "Wine66e", REG_MULTI_SZ, "Line\0", 6, 0);
    verify_reg(hkey, "Wine66f", REG_MULTI_SZ, "Line\0", 6, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine67a\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00\n"
                     "\"Wine67b\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,\n"
                     "\"Wine67c\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00\n"
                     "\"Wine67d\"=hex(2):25,00,50,00,41,00,54,00,48,00,25,00,00,00,\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine67a", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine67b", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine67c", REG_EXPAND_SZ, "%PATH%", 7, 0);
    verify_reg(hkey, "Wine67d", REG_EXPAND_SZ, "%PATH%", 7, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine68a\"=hex(1):\n"
                     "\"Wine68b\"=hex(2):\n"
                     "\"Wine68c\"=hex(3):\n"
                     "\"Wine68d\"=hex(4):\n"
                     "\"Wine68e\"=hex(7):\n"
                     "\"Wine68f\"=hex(100):\n"
                     "\"Wine68g\"=hex(abcd):\n"
                     "\"Wine68h\"=hex:\n"
                     "\"Wine68i\"=hex(0):\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine68a", REG_SZ, NULL, 0, 0);
    verify_reg(hkey, "Wine68b", REG_EXPAND_SZ, NULL, 0, 0);
    verify_reg(hkey, "Wine68c", REG_BINARY, NULL, 0, 0);
    verify_reg(hkey, "Wine68d", REG_DWORD, NULL, 0, 0);
    verify_reg(hkey, "Wine68e", REG_MULTI_SZ, NULL, 0, 0);
    verify_reg(hkey, "Wine68f", 0x100, NULL, 0, 0);
    verify_reg(hkey, "Wine68g", 0xabcd, NULL, 0, 0);
    verify_reg(hkey, "Wine68h", REG_BINARY, NULL, 0, 0);
    verify_reg(hkey, "Wine68i", REG_NONE, NULL, 0, 0);

    /* Test with embedded null characters */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine69a\"=\"\\0\n"
                     "\"Wine69b\"=\"\\0\\0\n"
                     "\"Wine69c\"=\"Value1\\0\n"
                     "\"Wine69d\"=\"Value2\\0\\0\\0\\0\n"
                     "\"Wine69e\"=\"Value3\\0Value4\n"
                     "\"Wine69f\"=\"\\0Value4\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "Wine69a");
    verify_reg_nonexist(hkey, "Wine69b");
    verify_reg_nonexist(hkey, "Wine69c");
    verify_reg_nonexist(hkey, "Wine69d");
    verify_reg_nonexist(hkey, "Wine69e");
    verify_reg_nonexist(hkey, "Wine69f");

    err = RegCloseKey(hkey);
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    delete_key(HKEY_CURRENT_USER, KEY_BASE);
}

static void test_import_with_whitespace(void)
{
    HKEY hkey;
    DWORD r, dword;
    LONG err;

    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(err == ERROR_SUCCESS || err == ERROR_FILE_NOT_FOUND, "got %d\n", err);

    test_import_str("  REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);

    open_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);

    test_import_str("  REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine1a\"=\"Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine1a", REG_SZ, "Value", 6, 0);

    test_import_str("\tREGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine1b\"=\"Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine1b", REG_SZ, "Value", 6, 0);

    test_import_str(" \t REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine1c\"=\"Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine1c", REG_SZ, "Value", 6, 0);

    test_import_str("REGEDIT4\n\n"
                    "  [HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine2a\"=\"Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine2a", REG_SZ, "Value", 6, 0);

    test_import_str("REGEDIT4\n\n"
                    "\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine2b\"=\"Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine2b", REG_SZ, "Value", 6, 0);

    test_import_str("REGEDIT4\n\n"
                    " \t [HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine2c\"=\"Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine2c", REG_SZ, "Value", 6, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "  \"Wine3a\"=\"Two leading spaces\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine3a", REG_SZ, "Two leading spaces", 19, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\t\"Wine3b\"=\"One leading tab\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine3b", REG_SZ, "One leading tab", 16, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    " \t \"Wine3c\"=\"Space, tab, space\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine3c", REG_SZ, "Space, tab, space", 18, 0);

    test_import_str("                    REGEDIT4\n\n"
                    "\t\t\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\t    \"Wine4a\"=\"Tab and four spaces\"\n"
                    "    \"Wine4b\"=dword:00112233\n"
                    "\t  \t  \t  \t  \t  \t  \"Wine4c\"=hex(7):4c,69,6e,65,20,\\\n"
                    "        63,6f,6e,\\;comment\n"
                    "\t\t\t\t63,61,74,\\;comment\n"
                    "  \t65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine4a", REG_SZ, "Tab and four spaces", 20, 0);
    dword = 0x112233;
    verify_reg(hkey, "Wine4b", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine4c", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_str("    REGEDIT4\n\n"
                    "\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "  \"Wine5a\"=\"Leading spaces\"\n"
                    "\t\t\"Wine5b\"\t\t=\"Leading tabs\"\n"
                    "\t  \"Wine5c\"=\t  \"Tabs and spaces\"\n"
                    "    \"Wine5d\"    \t    =    \t    \"More whitespace\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine5a", REG_SZ, "Leading spaces", 15, 0);
    verify_reg(hkey, "Wine5b", REG_SZ, "Leading tabs", 13, 0);
    verify_reg(hkey, "Wine5c", REG_SZ, "Tabs and spaces", 16, 0);
    verify_reg(hkey, "Wine5d", REG_SZ, "More whitespace", 16, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"  Wine6a\"=\"Leading spaces\"\n"
                    "\"\t\tWine6b\"=\"Leading tabs\"\n"
                    "  \"  Wine6c  \"  =  \"  Spaces everywhere  \"  \n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "  Wine6a", REG_SZ, "Leading spaces", 15, 0);
    verify_reg(hkey, "\t\tWine6b", REG_SZ, "Leading tabs", 13, 0);
    verify_reg(hkey, "  Wine6c  ", REG_SZ, "  Spaces everywhere  ", 22, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine7a\"=\"    Four spaces in the data\"\n"
                    "\"Wine7b\"=\"\t\tTwo tabs in the data\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine7a", REG_SZ, "    Four spaces in the data", 28, 0);
    verify_reg(hkey, "Wine7b", REG_SZ, "\t\tTwo tabs in the data", 23, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine8a\"=\"Trailing spaces\"    \n"
                    "\"Wine8b\"=\"Trailing tabs and spaces\"\t  \t\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine8a", REG_SZ, "Trailing spaces", 16, 0);
    verify_reg(hkey, "Wine8b", REG_SZ, "Trailing tabs and spaces", 25, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine9a\"=dword:  00000008\n"
                    "\"Wine9b\"=dword:\t\t00000008\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x00000008;
    verify_reg(hkey, "Wine9a", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine9b", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "@  =  \"Test Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "", REG_SZ, "Test Value", 11, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\t@\t=\tdword:\t00000008\t\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "", REG_DWORD, &dword, sizeof(DWORD), 0);

    err = RegCloseKey(hkey);
    ok(err == ERROR_SUCCESS, "RegCloseKey failed: got %d, expected 0\n", err);

    delete_key(HKEY_CURRENT_USER, KEY_BASE);
}

static void test_unicode_import_with_whitespace(void)
{
    HKEY hkey;
    DWORD r, dword;
    LONG err;

    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(err == ERROR_SUCCESS || err == ERROR_FILE_NOT_FOUND, "got %d\n", err);

    test_import_wstr("\xef\xbb\xbf  Windows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);

    open_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);

    test_import_wstr("\xef\xbb\xbf  Windows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine1a\"=\"Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine1a", REG_SZ, "Value", 6, 0);

    test_import_wstr("\xef\xbb\xbf\tWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine1b\"=\"Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine1b", REG_SZ, "Value", 6, 0);

    test_import_wstr("\xef\xbb\xbf \t Windows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine1c\"=\"Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine1c", REG_SZ, "Value", 6, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "  [HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine2a\"=\"Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine2a", REG_SZ, "Value", 6, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine2b\"=\"Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine2b", REG_SZ, "Value", 6, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                    " \t [HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine2c\"=\"Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine2c", REG_SZ, "Value", 6, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "  \"Wine3a\"=\"Two leading spaces\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine3a", REG_SZ, "Two leading spaces", 19, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\t\"Wine3b\"=\"One leading tab\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine3b", REG_SZ, "One leading tab", 16, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     " \t \"Wine3c\"=\"Space, tab, space\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "Wine3c", REG_SZ, "Space, tab, space", 18, 0);

    test_import_wstr("\xef\xbb\xbf                    Windows Registry Editor Version 5.00\n\n"
                     "\t\t\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\t    \"Wine4a\"=\"Tab and four spaces\"\n"
                     "    \"Wine4b\"=dword:00112233\n"
                     "\t  \t  \t  \t  \t  \t  \"Wine4c\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "        63,00,6f,00,6e,00,\\;comment\n"
                     "\t\t\t\t63,00,61,00,74,00,\\;comment\n"
                     "  \t65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine4a", REG_SZ, "Tab and four spaces", 20, 0);
    dword = 0x112233;
    verify_reg(hkey, "Wine4b", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine4c", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_wstr("\xef\xbb\xbf    Windows Registry Editor Version 5.00\n\n"
                     "\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "  \"Wine5a\"=\"Leading spaces\"\n"
                     "\t\t\"Wine5b\"\t\t=\"Leading tabs\"\n"
                     "\t  \"Wine5c\"=\t  \"Tabs and spaces\"\n"
                     "    \"Wine5d\"    \t    =    \t    \"More whitespace\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine5a", REG_SZ, "Leading spaces", 15, 0);
    verify_reg(hkey, "Wine5b", REG_SZ, "Leading tabs", 13, 0);
    verify_reg(hkey, "Wine5c", REG_SZ, "Tabs and spaces", 16, 0);
    verify_reg(hkey, "Wine5d", REG_SZ, "More whitespace", 16, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"  Wine6a\"=\"Leading spaces\"\n"
                     "\"\t\tWine6b\"=\"Leading tabs\"\n"
                     "  \"  Wine6c  \"  =  \"  Spaces everywhere  \"  \n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "  Wine6a", REG_SZ, "Leading spaces", 15, 0);
    verify_reg(hkey, "\t\tWine6b", REG_SZ, "Leading tabs", 13, 0);
    verify_reg(hkey, "  Wine6c  ", REG_SZ, "  Spaces everywhere  ", 22, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine7a\"=\"    Four spaces in the data\"\n"
                     "\"Wine7b\"=\"\t\tTwo tabs in the data\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine7a", REG_SZ, "    Four spaces in the data", 28, 0);
    verify_reg(hkey, "Wine7b", REG_SZ, "\t\tTwo tabs in the data", 23, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine8a\"=\"Trailing spaces\"    \n"
                     "\"Wine8b\"=\"Trailing tabs and spaces\"\t  \t\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "Wine8a", REG_SZ, "Trailing spaces", 16, 0);
    verify_reg(hkey, "Wine8b", REG_SZ, "Trailing tabs and spaces", 25, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine9a\"=dword:  00000008\n"
                     "\"Wine9b\"=dword:\t\t00000008\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x00000008;
    verify_reg(hkey, "Wine9a", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine9b", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "@  =  \"Test Value\"\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "", REG_SZ, "Test Value", 11, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\t@\t=\tdword:\t00000008\t\n\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "", REG_DWORD, &dword, sizeof(DWORD), 0);

    err = RegCloseKey(hkey);
    ok(err == ERROR_SUCCESS, "RegCloseKey failed: got %d, expected 0\n", err);

    delete_key(HKEY_CURRENT_USER, KEY_BASE);
}

static void test_import_31(void)
{
    LONG err;
    HKEY hkey;
    DWORD r;

    err = RegDeleteKeyA(HKEY_CLASSES_ROOT, KEY_BASE);
    ok(err == ERROR_SUCCESS || err == ERROR_FILE_NOT_FOUND, "RegDeleteKeyA failed: %d\n", err);

    /* Check if reg.exe is running with elevated privileges */
    err = RegCreateKeyExA(HKEY_CLASSES_ROOT, KEY_BASE, 0, NULL, REG_OPTION_NON_VOLATILE,
                          KEY_READ|KEY_SET_VALUE, NULL, &hkey, NULL);
    if (err == ERROR_ACCESS_DENIED)
    {
        win_skip("reg.exe is not running with elevated privileges; "
                 "skipping Windows 3.1 import tests\n");
        return;
    }

    /* Test simple value */
    test_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE " = Value0\r\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "", REG_SZ, "Value0", 7, 0);

    /* Test proper handling of spaces and equals signs */
    test_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE " =Value1\r\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "", REG_SZ, "Value1", 7, 0);

    test_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE " =  Value2\r\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "", REG_SZ, " Value2", 8, 0);

    test_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE " = Value3 \r\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "", REG_SZ, "Value3 ", 8, 0);

    test_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE " Value4\r\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "", REG_SZ, "Value4", 7, 0);

    test_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE "  Value5\r\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "", REG_SZ, "Value5", 7, 0);

    test_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE "\r\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "", REG_SZ, "", 1, 0);

    test_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE "  \r\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "", REG_SZ, "", 1, 0);

    test_import_str("REGEDIT\r\n"
                    "HKEY_CLASSES_ROOT\\" KEY_BASE " = No newline", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "", REG_SZ, "No newline", 11, 0);

    delete_value(hkey, NULL);

    /* Test character validity at the start of the line */
    test_import_str("REGEDIT\r\n"
                    " HKEY_CLASSES_ROOT\\" KEY_BASE " = Value1a\r\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "");

    test_import_str("REGEDIT\r\n"
                    "  HKEY_CLASSES_ROOT\\" KEY_BASE " = Value1b\r\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "");

    test_import_str("REGEDIT\r\n"
                    "\tHKEY_CLASSES_ROOT\\" KEY_BASE " = Value1c\r\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "");

    test_import_str("REGEDIT\r\n"
                    ";HKEY_CLASSES_ROOT\\" KEY_BASE " = Value2a\r\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "");

    test_import_str("REGEDIT\r\n"
                    "#HKEY_CLASSES_ROOT\\" KEY_BASE " = Value2b\r\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "");

    /* Test case sensitivity */
    test_import_str("REGEDIT\r\n"
                    "hkey_classes_root\\" KEY_BASE " = Value3a\r\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "");

    test_import_str("REGEDIT\r\n"
                    "hKEY_CLASSES_ROOT\\" KEY_BASE " = Value3b\r\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "");

    test_import_str("REGEDIT\r\n"
                    "Hkey_Classes_Root\\" KEY_BASE " = Value3c\r\n", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg_nonexist(hkey, "");

    RegCloseKey(hkey);

    delete_key(HKEY_CLASSES_ROOT, KEY_BASE);
}

#define compare_export(f,e,todo) compare_export_(__LINE__,f,e,todo)
static BOOL compare_export_(unsigned line, const char *filename, const char *expected, DWORD todo)
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

static void test_export(void)
{
    LONG err;
    DWORD r, dword, type, size;
    HKEY hkey, subkey;
    BYTE hex[4], buffer[8];

    const char *empty_key_test =
        "\xef\xbb\xbfWindows Registry Editor Version 5.00\r\n\r\n"
        "[HKEY_CURRENT_USER\\" KEY_BASE "]\r\n\r\n";

    const char *simple_test =
        "\xef\xbb\xbfWindows Registry Editor Version 5.00\r\n\r\n"
        "[HKEY_CURRENT_USER\\" KEY_BASE "]\r\n"
        "\"DWORD\"=dword:00000100\r\n"
        "\"String\"=\"Your text here...\"\r\n\r\n";

    const char *complex_test =
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

    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(err == ERROR_SUCCESS || err == ERROR_FILE_NOT_FOUND, "got %d\n", err);

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
    add_key(HKEY_CURRENT_USER, KEY_BASE, &hkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

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

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", simple_test, 0), "compare_export() failed\n");

    /* Test registry export with a complex data structure */
    add_key(hkey, "Subkey1", &subkey);
    add_value(subkey, "Binary", REG_BINARY, "\x11\x22\x33\x44", 4);
    add_value(subkey, "Undefined hex", 0x100, "%PATH%", 7);
    RegCloseKey(subkey);

    add_key(hkey, "Subkey2a", &subkey);
    add_value(subkey, "double\"quote", REG_SZ, "\"Hello, World!\"", 16);
    dword = 0x8;
    add_value(subkey, "single'quote", REG_DWORD, &dword, sizeof(dword));
    RegCloseKey(subkey);

    add_key(hkey, "Subkey2a\\Subkey2b", &subkey);
    add_value(subkey, NULL, REG_SZ, "Default value name", 19);
    add_value(subkey, "Multiple strings", REG_MULTI_SZ, "Line1\0Line2\0Line3\0", 19);
    RegCloseKey(subkey);

    add_key(hkey, "Subkey3a", &subkey);
    add_value(subkey, "Backslash", REG_SZ, "Use \\\\ to escape a backslash", 29);
    RegCloseKey(subkey);

    add_key(hkey, "Subkey3a\\Subkey3b\\Subkey3c", &subkey);
    add_value(subkey, "String expansion", REG_EXPAND_SZ, "%HOME%\\%PATH%", 14);
    add_value(subkey, "Zero data type", REG_NONE, "Value", 6);
    RegCloseKey(subkey);

    add_key(hkey, "Subkey4", &subkey);
    dword = 0x12345678;
    add_value(subkey, NULL, REG_DWORD, &dword, sizeof(dword));
    add_value(subkey, "43981", 0xabcd, "Value", 6);
    RegCloseKey(subkey);

    RegCloseKey(hkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", complex_test, 0), "compare_export() failed\n");

    err = delete_tree(HKEY_CURRENT_USER, KEY_BASE);
    ok(err == ERROR_SUCCESS, "delete_tree() failed: %d\n", err);

    /* Test the export order of registry keys */
    add_key(HKEY_CURRENT_USER, KEY_BASE, &hkey);
    add_key(hkey, "Subkey2", &subkey);
    RegCloseKey(subkey);
    add_key(hkey, "Subkey1", &subkey);
    RegCloseKey(subkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", key_order_test, 0), "compare_export() failed\n");

    delete_key(hkey, "Subkey1");
    delete_key(hkey, "Subkey2");

    /* Test the export order of registry values. Windows exports registry values
     * in order of creation; Wine uses alphabetical order.
     */
    add_value(hkey, "Value 2", REG_SZ, "I was added first!", 19);
    add_value(hkey, "Value 1", REG_SZ, "I was added second!", 20);

    RegCloseKey(hkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", value_order_test, TODO_REG_COMPARE), "compare_export() failed\n");

    delete_key(HKEY_CURRENT_USER, KEY_BASE);

    /* Test registry export with empty hex data */
    add_key(HKEY_CURRENT_USER, KEY_BASE, &hkey);
    add_value(hkey, "Wine1a", REG_NONE, NULL, 0);
    add_value(hkey, "Wine1b", REG_SZ, NULL, 0);
    add_value(hkey, "Wine1c", REG_EXPAND_SZ, NULL, 0);
    add_value(hkey, "Wine1d", REG_BINARY, NULL, 0);
    add_value(hkey, "Wine1e", REG_DWORD, NULL, 0);
    add_value(hkey, "Wine1f", REG_MULTI_SZ, NULL, 0);
    add_value(hkey, "Wine1g", 0x100, NULL, 0);
    add_value(hkey, "Wine1h", 0xabcd, NULL, 0);
    RegCloseKey(hkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", empty_hex_test, 0), "compare_export() failed\n");

    delete_key(HKEY_CURRENT_USER, KEY_BASE);

    /* Test registry export after importing alternative registry data types */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine2a\"=hex(1):\n"
                     "\"Wine2b\"=hex(3):\n"
                     "\"Wine2c\"=hex(4):\n\n", &r);
    open_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);
    verify_reg(hkey, "Wine2a", REG_SZ, NULL, 0, 0);
    verify_reg(hkey, "Wine2b", REG_BINARY, NULL, 0, 0);
    verify_reg(hkey, "Wine2c", REG_DWORD, NULL, 0, 0);
    RegCloseKey(hkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", empty_hex_test2, 0), "compare_export() failed\n");

    delete_key(HKEY_CURRENT_USER, KEY_BASE);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine3a\"=hex(1):56,00,61,00,6c,00,75,00,65,00,00,00\n"
                     "\"Wine3b\"=hex(3):12,34,56,78\n"
                     "\"Wine3c\"=hex(4):40,30,20,10\n\n", &r);
    open_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);
    verify_reg(hkey, "Wine3a", REG_SZ, "Value", 6, 0);
    memcpy(hex, "\x12\x34\x56\x78", 4);
    verify_reg(hkey, "Wine3b", REG_BINARY, hex, 4, 0);
    dword = 0x10203040;
    verify_reg(hkey, "Wine3c", REG_DWORD, &dword, sizeof(dword), 0);
    RegCloseKey(hkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", hex_types_test, 0), "compare_export() failed\n");

    delete_key(HKEY_CURRENT_USER, KEY_BASE);

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
    RegCloseKey(hkey);

    run_reg_exe("reg export HKEY_CURRENT_USER\\" KEY_BASE " file.reg", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    ok(compare_export("file.reg", embedded_null_test, 0), "compare_export() failed\n");

    delete_key(HKEY_CURRENT_USER, KEY_BASE);
}

#define COPY_DEST  KEY_WINE "\\reg_copy"

static void test_copy(void)
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

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" KEY_BASE " /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" KEY_BASE " foo /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" KEY_BASE " HKEY_CURRENT_USER\\" KEY_BASE" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy /f HKEY_CURRENT_USER\\" KEY_BASE " HKEY_CURRENT_USER\\" KEY_BASE, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy /f HKEY_CURRENT_USER\\" KEY_BASE " HKEY_CURRENT_USER\\" COPY_DEST, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" KEY_BASE " /f HKEY_CURRENT_USER\\" COPY_DEST, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" KEY_BASE " /s HKEY_CURRENT_USER\\" COPY_DEST, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" COPY_DEST " HKEY_CURRENT_USER\\" KEY_BASE, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" COPY_DEST " HKEY_CURRENT_USER\\" KEY_BASE, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" KEY_BASE " HKEY_CURRENT_USER\\" COPY_DEST "/a", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg copy HKEY_CURRENT_USER\\" KEY_BASE " HKEY_CURRENT_USER\\" COPY_DEST "/f /a", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);
}

START_TEST(reg)
{
    DWORD r;
    if (!run_reg_exe("reg.exe /?", &r)) {
        win_skip("reg.exe not available, skipping reg.exe tests\n");
        return;
    }

    test_add();
    test_delete();
    test_query();
    test_v_flags();
    test_import();
    test_unicode_import();
    test_import_with_whitespace();
    test_unicode_import_with_whitespace();
    test_import_31();
    test_export();
    test_copy();
}
