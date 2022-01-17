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

#include "reg_test.h"

BOOL run_reg_exe_(const char *file, unsigned line, const char *cmd, DWORD *rc)
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

void verify_reg_(const char *file, unsigned line, HKEY hkey, const char *value,
                 DWORD exp_type, const void *exp_data, DWORD exp_size, DWORD todo)
{
    DWORD type, size;
    BYTE data[256];
    LONG err;

    size = sizeof(data);
    memset(data, 0xdd, size);
    err = RegQueryValueExA(hkey, value, NULL, &type, data, &size);
    lok(err == ERROR_SUCCESS, "RegQueryValueEx failed: got error %d\n", err);
    if (err != ERROR_SUCCESS)
        return;

    todo_wine_if (todo & TODO_REG_TYPE)
        lok(type == exp_type, "got registry type %d, expected %d\n", type, exp_type);
    todo_wine_if (todo & TODO_REG_SIZE)
        lok(size == exp_size, "got data size %d, expected %d\n", size, exp_size);
    if (exp_data)
    {
        todo_wine_if (todo & TODO_REG_DATA)
            lok(memcmp(data, exp_data, size) == 0, "registry data does not match\n");
    }
}

void verify_reg_nonexist_(const char *file, unsigned line, HKEY hkey, const char *value)
{
    LONG err;

    err = RegQueryValueExA(hkey, value, NULL, NULL, NULL, NULL);
    lok(err == ERROR_FILE_NOT_FOUND, "registry value '%s' shouldn't exist; got %d, expected 2\n",
        (value && *value) ? value : "(Default)", err);
}

void open_key_(const char *file, unsigned line, HKEY root, const char *path, REGSAM sam, HKEY *hkey)
{
    LONG err;

    err = RegOpenKeyExA(root, path, 0, KEY_READ|sam, hkey);
    lok(err == ERROR_SUCCESS, "RegOpenKeyExA failed: got error %d\n", err);
}

void close_key_(const char *file, unsigned line, HKEY hkey)
{
    LONG err;

    err = RegCloseKey(hkey);
    lok(err == ERROR_SUCCESS, "RegCloseKey failed: got error %d\n", err);
}

void verify_key_(const char *file, unsigned line, HKEY root, const char *path, REGSAM sam)
{
    HKEY hkey;
    LONG err;

    err = RegOpenKeyExA(root, path, 0, KEY_READ|sam, &hkey);
    lok(err == ERROR_SUCCESS, "RegOpenKeyExA failed: got error %d\n", err);

    if (hkey)
        RegCloseKey(hkey);
}

void verify_key_nonexist_(const char *file, unsigned line, HKEY root, const char *path, REGSAM sam)
{
    HKEY hkey;
    LONG err;

    err = RegOpenKeyExA(root, path, 0, KEY_READ|sam, &hkey);
    lok(err == ERROR_FILE_NOT_FOUND, "registry key '%s' shouldn't exist; got %d, expected 2\n", path, err);

    if (hkey)
        RegCloseKey(hkey);
}

void add_key_(const char *file, unsigned line, const HKEY root, const char *path, REGSAM sam, HKEY *hkey)
{
    LONG err;
    HKEY new_key;

    err = RegCreateKeyExA(root, path, 0, NULL, REG_OPTION_NON_VOLATILE,
                          KEY_READ|KEY_WRITE|sam, NULL, &new_key, NULL);
    lok(err == ERROR_SUCCESS, "RegCreateKeyExA failed: got error %d\n", err);

    if (hkey)
        *hkey = new_key;
    else
        RegCloseKey(new_key);
}

void delete_key_(const char *file, unsigned line, HKEY root, const char *path, REGSAM sam)
{
    LONG err;

    if (!path) return;

    if (!sam)
    {
        err = RegDeleteKeyA(root, path);
        lok(err == ERROR_SUCCESS, "RegDeleteKeyA failed: got error %d\n", err);
    }
    else
    {
        err = RegDeleteKeyExA(root, path, sam, 0);
        lok(err == ERROR_SUCCESS, "RegDeleteKeyExA failed: got error %d\n", err);
    }
}

LONG delete_tree(const HKEY key, const char *subkey)
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

void add_value_(const char *file, unsigned line, HKEY hkey, const char *name,
                DWORD type, const void *data, size_t size)
{
    LONG err;

    err = RegSetValueExA(hkey, name, 0, type, (const BYTE *)data, size);
    lok(err == ERROR_SUCCESS, "RegSetValueExA failed: got error %d\n", err);
}

void delete_value_(const char *file, unsigned line, HKEY hkey, const char *name)
{
    LONG err;

    err = RegDeleteValueA(hkey, name);
    lok(err == ERROR_SUCCESS, "RegDeleteValueA failed: got error %d\n", err);
}

/* Unit tests */

static void test_command_syntax(void)
{
    DWORD r;

    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
    verify_key_nonexist(HKEY_CURRENT_USER, KEY_BASE, 0);

    run_reg_exe("reg add", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add /?", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg add /h", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg add -H", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    /* Duplicate switches */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v Wine /t REG_DWORD /d 0x1 /v Test /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dup1 /t REG_DWORD /d 123 /f /t REG_SZ", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */),
       "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dup2 /t REG_DWORD /d 123 /f /d 456", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /f /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v Wine /ve", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    /* No /v argument */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /d Test /f /v", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    /* Test invalid switches */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v invalid1 /a", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v invalid2 /ae", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v invalid3 /", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v invalid4 -", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    /* Test empty type */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v emptyType /t \"\" /d WineTest /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    /* Test registry view */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v abc /d 123 /f /reg:32 /reg:32", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v abc /d 123 /f /reg:32 /reg:64", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v abc /d 123 /f /reg:64 /reg:64", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);
}

static void test_key_formats(void)
{
    HKEY hkey;
    DWORD r;
    LONG err;

    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);

    run_reg_exe("reg add \\HKCU\\" KEY_BASE "\\keytest0 /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);
    verify_key_nonexist(hkey, "keytest0", 0);

    run_reg_exe("reg add \\\\HKCU\\" KEY_BASE "\\keytest1 /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);
    verify_key_nonexist(hkey, "keytest1", 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE "\\keytest2\\\\ /f", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */),
        "got exit code %u, expected 1\n", r);
    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE "\\keytest2");
    ok(err == ERROR_FILE_NOT_FOUND || broken(err == ERROR_SUCCESS /* WinXP */),
        "got exit code %d, expected 2\n", err);

    run_reg_exe("reg add HKCU\\" KEY_BASE "\\keytest3\\ /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_key(hkey, "keytest3", 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE "\\keytest4 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_key(hkey, "keytest4", 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE "\\https://winehq.org /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_key(hkey, "https://winehq.org", 0);

    close_key(hkey);
    delete_tree(HKEY_CURRENT_USER, KEY_BASE);

    /* Test validity of trailing backslash after system key */
    run_reg_exe("reg add HKCU\\ /v Value1 /t REG_SZ /d foo /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);
    verify_reg_nonexist(HKEY_CURRENT_USER, "Value1");

    run_reg_exe("reg add HKEY_CURRENT_USER\\ /v Value2 /t REG_SZ /d bar /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);
    verify_reg_nonexist(HKEY_CURRENT_USER, "Value2");
}

static void test_add(void)
{
    HKEY hkey;
    DWORD r, dword;

    run_reg_exe("reg add HKCU\\" KEY_BASE " /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    open_key(HKEY_CURRENT_USER, KEY_BASE, KEY_WRITE, &hkey);

    /* The Default value is initialized if no parameters are specified */
    verify_reg(hkey, NULL, REG_SZ, "", 1, 0);
    delete_value(hkey, NULL);

    /* This also occurs when specifying a registry type and passing data */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_DWORD /d 0x5 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x5;
    verify_reg(hkey, NULL, REG_DWORD, &dword, sizeof(dword), 0);

    /* The Default value can also be overwritten as an empty string */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, NULL, REG_SZ, "", 1, 0);

    close_key(hkey);
    delete_key(HKEY_CURRENT_USER, KEY_BASE, 0);

    /* Specifying a value name doesn't initialize the Default value in a new key */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v Test /t REG_SZ /d \"Just me here\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    open_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);

    verify_reg(hkey, "Test", REG_SZ, "Just me here", 13, 0);
    verify_reg_nonexist(hkey, NULL);

    close_key(hkey);
    delete_key(HKEY_CURRENT_USER, KEY_BASE, 0);

    /* Adding a registry key via WinAPI doesn't initialize the Default value... */
    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);
    verify_reg_nonexist(hkey, NULL);

    /* ... but we can add it without passing [/f] to reg.exe */
    run_reg_exe("reg add HKCU\\" KEY_BASE, &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, NULL, REG_SZ, "", 1, 0);
    delete_value(hkey, NULL);

    /* Test whether overwriting a registry key modifies existing keys and values */
    add_key(hkey, "Subkey", 0, NULL);
    add_value(hkey, "Test1", REG_SZ, "Value1", 7);
    dword = 0x123;
    add_value(hkey, "Test2", REG_DWORD, &dword, sizeof(dword));

    run_reg_exe("reg add HKCU\\" KEY_BASE " /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    verify_key(HKEY_CURRENT_USER, KEY_BASE, 0);
    verify_key(hkey, "Subkey", 0);
    verify_reg(hkey, "Test1", REG_SZ, "Value1", 7, 0);
    verify_reg(hkey, "Test2", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, NULL, REG_SZ, "", 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_NONE /d Test /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    verify_key(HKEY_CURRENT_USER, KEY_BASE, 0);
    verify_key(hkey, "Subkey", 0);
    verify_reg(hkey, "Test1", REG_SZ, "Value1", 7, 0);
    verify_reg(hkey, "Test2", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, NULL, REG_NONE, "T\0e\0s\0t\0\0", 10, 0);

    close_key(hkey);
    delete_tree(HKEY_CURRENT_USER, KEY_BASE);
}

static void test_reg_none(void)
{
    HKEY hkey;
    DWORD r;

    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_NONE /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, NULL, REG_NONE, "\0", 2, 0);

    delete_value(hkey, NULL);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /t REG_NONE /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, NULL, REG_NONE, "\0", 2, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v none0 /d deadbeef /t REG_NONE /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "none0", REG_NONE, "d\0e\0a\0d\0b\0e\0e\0f\0\0", 18, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v none1 /t REG_NONE /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "none1", REG_NONE, "\0", 2, 0);

    close_key(hkey);
    delete_key(HKEY_CURRENT_USER, KEY_BASE, 0);
}

static void test_reg_sz(void)
{
    HKEY hkey;
    DWORD r;

    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_SZ /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, NULL, REG_SZ, "", 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /d WineTest /f", &r);
    ok(r == REG_EXIT_SUCCESS || broken(r == REG_EXIT_FAILURE /* WinXP */),
       "got exit code %d, expected 0\n", r);
    if (r == REG_EXIT_SUCCESS)
        verify_reg(hkey, "", REG_SZ, "WineTest", 9, 0);
    else
        win_skip("broken reg.exe detected\n");

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, NULL, REG_SZ, "", 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /f /ve", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKEY_CURRENT_USER\\" KEY_BASE " /ve /d WineTEST /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "", REG_SZ, "WineTEST", 9, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /t REG_SZ /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, NULL, REG_SZ, "", 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v test0 /d deadbeef /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "test0", REG_SZ, "deadbeef", 9, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v test0 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "test0", REG_SZ, "", 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v test1 /t REG_SZ /f /d", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_SZ /v test2 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "test2", REG_SZ, "", 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_SZ /v test3 /f /d \"\"", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "test3", REG_SZ, "", 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v string\\04 /t REG_SZ /d \"Value\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "string\\04", REG_SZ, "Value", 6, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v string5 /t REG_SZ /d \"foo\\0bar\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "string5", REG_SZ, "foo\\0bar", 9, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v \\0 /t REG_SZ /d \"Value\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "\\0", REG_SZ, "Value", 6, 0);

    /* Test support for forward and back slashes in value names */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v count/up /d one/two/three /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "count/up", REG_SZ, "one/two/three", 14, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v \\foo\\bar /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    verify_reg(hkey, "\\foo\\bar", REG_SZ, "", 1, 0);

    close_key(hkey);
    delete_key(HKEY_CURRENT_USER, KEY_BASE, 0);
}

static void test_reg_expand_sz(void)
{
    HKEY hkey;
    DWORD r;

    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_EXPAND_SZ /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, NULL, REG_EXPAND_SZ, "", 1, 0);

    delete_value(hkey, NULL);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /t REG_EXPAND_SZ /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, NULL, REG_EXPAND_SZ, "", 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v expand0 /t REG_EXpand_sz /d \"dead%PATH%beef\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "expand0", REG_EXPAND_SZ, "dead%PATH%beef", 15, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v expand1 /t REG_EXpand_sz /d \"dead^%PATH^%beef\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "expand1", REG_EXPAND_SZ, "dead^%PATH^%beef", 17, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_EXPAND_SZ /v expand2 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "expand2", REG_EXPAND_SZ, "", 1, 0);

    run_reg_exe("reg add HKEY_CURRENT_USER\\" KEY_BASE " /ve /t REG_EXPAND_SZ /d WineTEST /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "", REG_EXPAND_SZ, "WineTEST", 9, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_EXPAND_SZ /v expand3 /f /d \"\"", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "expand3", REG_EXPAND_SZ, "", 1, 0);

    close_key(hkey);
    delete_key(HKEY_CURRENT_USER, KEY_BASE, 0);
}

static void test_reg_binary(void)
{
    HKEY hkey;
    DWORD r, dword, type, size;
    char buffer[22];
    LONG err;

    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_BINARY /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, NULL, REG_BINARY, buffer, 0, 0);

    delete_value(hkey, NULL);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /t REG_BINARY /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, NULL, REG_BINARY, buffer, 0, 0);

    run_reg_exe("reg add HKEY_CURRENT_USER\\" KEY_BASE " /ve /t REG_BINARY /d deadbeef /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    dword = 0xefbeadde;
    verify_reg(hkey, "", REG_BINARY, &dword, sizeof(DWORD), 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_BINARY /v bin0 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "bin0", REG_BINARY, buffer, 0, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_BINARY /v bin1 /f /d 0xDeAdBeEf", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_BINARY /v bin2 /f /d x01", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_BINARY /v bin3 /f /d 01x", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_BINARY /v bin4 /f /d DeAdBeEf0DD", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    /* Remaining nibble prefixed */
    buffer[0] = 0x0d; buffer[1] = 0xea; buffer[2] = 0xdb;
    buffer[3] = 0xee; buffer[4] = 0xf0; buffer[5] = 0xdd;
    /* Remaining nibble suffixed on WinXP */
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
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "bin5", REG_BINARY, buffer, 0, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v bin6 /t REG_BINARY /f /d", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    close_key(hkey);
    delete_key(HKEY_CURRENT_USER, KEY_BASE, 0);
}

static void test_reg_dword(void)
{
    HKEY hkey;
    DWORD r, dword, type, size;
    LONG err;

    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_DWORD /f /d 12345678", &r);
    ok(r == REG_EXIT_SUCCESS || broken(r == REG_EXIT_FAILURE /* WinXP */),
       "got exit code %d, expected 0\n", r);
    dword = 12345678;
    if (r == REG_EXIT_SUCCESS)
        verify_reg(hkey, "", REG_DWORD, &dword, sizeof(dword), 0);
    else
        win_skip("broken reg.exe detected\n");

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /t REG_DWORD /f", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */), "got exit code %u, expected 1\n", r);

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
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */), "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_DWORD /v dword10 /f /d -0x1", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */), "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword11 /t REG_dword /d 0x01ffffffff /f", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */), "got exit code %d, expected 1\n", r);

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
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */), "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword16 /t REG_DWORD /d 456 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 456;
    verify_reg(hkey, "dword16", REG_DWORD, &dword, sizeof(dword), 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v dword17 /t REG_DWORD /d 0x456 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x456;
    verify_reg(hkey, "dword17", REG_DWORD, &dword, sizeof(dword), 0);

    /* REG_DWORD_LITTLE_ENDIAN */
    run_reg_exe("reg add HKCU\\" KEY_BASE " /v DWORD_LE /t REG_DWORD_LITTLE_ENDIAN /d 456 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 456;
    verify_reg(hkey, "DWORD_LE", REG_DWORD_LITTLE_ENDIAN, &dword, sizeof(dword), 0);

    close_key(hkey);
    delete_key(HKEY_CURRENT_USER, KEY_BASE, 0);
}

/* REG_DWORD_BIG_ENDIAN is broken in every version of Windows. It behaves
 * like an ordinary REG_DWORD, which is little endian.
 */
static void test_reg_dword_big_endian(void)
{
    HKEY hkey;
    DWORD r, dword;

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /t REG_DWORD_BIG_ENDIAN /f", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */), "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v Test1 /t REG_DWORD_BIG_ENDIAN /f /d", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v Test2 /t REG_DWORD_BIG_ENDIAN /f", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS /* WinXP */), "got exit code %d, expected 1\n", r);

    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v Test3 /t REG_DWORD_BIG_ENDIAN /d 456 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 456;
    verify_reg(hkey, "Test3", REG_DWORD_BIG_ENDIAN, &dword, sizeof(dword), 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v Test4 /t REG_DWORD_BIG_ENDIAN /d 0x456 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x456;
    verify_reg(hkey, "Test4", REG_DWORD_BIG_ENDIAN, &dword, sizeof(dword), 0);

    close_key(hkey);
    delete_key(HKEY_CURRENT_USER, KEY_BASE, 0);
}

static void test_reg_multi_sz(void)
{
    HKEY hkey;
    DWORD r;

    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, NULL, REG_MULTI_SZ, "", 1, 0);

    delete_value(hkey, NULL);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /ve /t REG_MULTI_SZ /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, NULL, REG_MULTI_SZ, "", 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi0 /t REG_MULTI_SZ /d \"three\\0little\\0strings\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi0", REG_MULTI_SZ, "three\0little\0strings\0", 22, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi1 /s \"#\" /d \"three#little#strings\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi1", REG_MULTI_SZ, "three\0little\0strings\0", 22, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi2 /d \"\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi2", REG_MULTI_SZ, "", 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi3 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi3", REG_MULTI_SZ, "", 1, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi4 /s \"#\" /d \"threelittlestrings\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi4", REG_MULTI_SZ, "threelittlestrings\0", 20, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi5 /s \"#randomgibberish\" /d \"three#little#strings\" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi6 /s \"\\0\" /d \"three\\0little\\0strings\" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi7 /s \"\" /d \"three#little#strings\" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi8 /s \"#\" /d \"##\" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi9 /s \"#\" /d \"two##strings\" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi10 /s \"#\" /d \"#a\" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi11 /s \"#\" /d \"a#\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi11", REG_MULTI_SZ, "a\0", 3, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi12 /t REG_MULTI_SZ /f /d", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi13 /t REG_MULTI_SZ /f /s", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi14 /t REG_MULTI_SZ /d \"\\0a\" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi15 /t REG_MULTI_SZ /d \"a\\0\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi15", REG_MULTI_SZ, "a\0", 3, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /t REG_MULTI_SZ /v multi16 /d \"two\\0\\0strings\" /f", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %u, expected 1\n", r);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi17 /t REG_MULTI_SZ /s \"#\" /d \"#\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi17", REG_MULTI_SZ, "\0", 2, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi18 /t REG_MULTI_SZ /d \"\\0\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi18", REG_MULTI_SZ, "\0", 2, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi19 /t REG_MULTI_SZ /s \"#\" /d \"two\\0#strings\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi19", REG_MULTI_SZ, "two\\0\0strings\0", 15, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi20 /t REG_MULTI_SZ /s \"#\" /d \"two#\\0strings\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi20", REG_MULTI_SZ, "two\0\\0strings\0", 15, 0);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /v multi21 /t REG_MULTI_SZ /s \"#\" /d \"two\\0\\0strings\" /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    verify_reg(hkey, "multi21", REG_MULTI_SZ, "two\\0\\0strings\0", 16, 0);

    close_key(hkey);
    delete_key(HKEY_CURRENT_USER, KEY_BASE, 0);
}

static void test_registry_view_win32(void)
{
    HKEY hkey;
    DWORD r;
    BOOL is_wow64, is_win32;

    IsWow64Process(GetCurrentProcess(), &is_wow64);
    is_win32 = !is_wow64 && (sizeof(void *) == sizeof(int));

    if (!is_win32) return;

    /* Try adding to the 32-bit registry view (32-bit Windows) */
    run_reg_exe("reg add HKLM\\" KEY_BASE " /v Wine32 /d Test /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    open_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY, &hkey);
    verify_reg(hkey, "Wine32", REG_SZ, "Test", 5, 0);
    close_key(hkey);

    open_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY, &hkey);
    verify_reg(hkey, "Wine32", REG_SZ, "Test", 5, 0);
    close_key(hkey);

    delete_key(HKEY_LOCAL_MACHINE, KEY_BASE, 0);

    /* Try adding to the 64-bit registry view, which doesn't exist on 32-bit Windows */
    run_reg_exe("reg add HKLM\\" KEY_BASE " /v Wine64 /d Test /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    open_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY, &hkey);
    verify_reg(hkey, "Wine64", REG_SZ, "Test", 5, 0);
    close_key(hkey);

    open_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY, &hkey);
    verify_reg(hkey, "Wine64", REG_SZ, "Test", 5, 0);
    close_key(hkey);

    delete_key(HKEY_LOCAL_MACHINE, KEY_BASE, 0);
}

static void test_registry_view_win64(void)
{
    HKEY hkey;
    DWORD r;
    BOOL is_wow64, is_win64;

    IsWow64Process(GetCurrentProcess(), &is_wow64);
    is_win64 = !is_wow64 && (sizeof(void *) > sizeof(int));

    if (!is_win64) return;

    /* Try adding to the 32-bit registry view (64-bit Windows) */
    run_reg_exe("reg add HKLM\\" KEY_BASE " /v Wine32 /d Test /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    open_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY, &hkey);
    verify_reg(hkey, "Wine32", REG_SZ, "Test", 5, 0);
    close_key(hkey);
    delete_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);

    verify_key_nonexist(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY);

    /* Try adding to the 64-bit registry view (64-bit Windows) */
    run_reg_exe("reg add HKLM\\" KEY_BASE " /v Wine64 /d Test /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    open_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY, &hkey);
    verify_reg(hkey, "Wine64", REG_SZ, "Test", 5, 0);
    close_key(hkey);
    delete_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY);

    verify_key_nonexist(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);
}

static void test_registry_view_wow64(void)
{
    HKEY hkey;
    DWORD r;
    BOOL is_wow64;

    IsWow64Process(GetCurrentProcess(), &is_wow64);

    if (!is_wow64) return;

    /* Try adding to the 32-bit registry view (WOW64) */
    run_reg_exe("reg add HKLM\\" KEY_BASE " /v Wine32 /d Test /f /reg:32", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    open_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY, &hkey);
    verify_reg(hkey, "Wine32", REG_SZ, "Test", 5, 0);
    close_key(hkey);
    delete_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);

    verify_key_nonexist(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY);

    /* Try adding to the 64-bit registry view (WOW64) */
    run_reg_exe("reg add HKLM\\" KEY_BASE " /v Wine64 /d Test /f /reg:64", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    open_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY, &hkey);
    verify_reg(hkey, "Wine64", REG_SZ, "Test", 5, 0);
    close_key(hkey);

    open_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY, &hkey);
    verify_reg(hkey, "Wine64", REG_SZ, "Test", 5, 0);
    close_key(hkey);
    delete_key(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY);

    verify_key_nonexist(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);
}

START_TEST(add)
{
    DWORD r;

    if (!run_reg_exe("reg.exe /?", &r)) {
        win_skip("reg.exe not available, skipping 'add' tests\n");
        return;
    }

    test_command_syntax();
    test_key_formats();
    test_add();
    test_reg_none();
    test_reg_sz();
    test_reg_expand_sz();
    test_reg_binary();
    test_reg_dword();
    test_reg_dword_big_endian();
    test_reg_multi_sz();

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
