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

static void read_from_pipe(HANDLE child_proc_stdout, BYTE *buf, DWORD buf_size)
{
    DWORD read, len = 0;
    BOOL ret;

    while (1)
    {
        ret = ReadFile(child_proc_stdout, buf + len, buf_size - len, &read, NULL);
        if (!ret || !read) break;

        len += read;
    }

    buf[len] = 0;
}

#define read_reg_output(c,b,s,r) read_reg_output_(__FILE__,__LINE__,c,b,s,r)
static BOOL read_reg_output_(const char *file, unsigned line, const char *cmd,
                             BYTE *buf, DWORD buf_size, DWORD *rc)
{
    SECURITY_ATTRIBUTES sa;
    HANDLE pipe_stdout_rd, pipe_stdout_wr;
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi;
    char cmdline[256];
    BOOL bret;
    DWORD ret;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&pipe_stdout_rd, &pipe_stdout_wr, &sa, 0))
        return FALSE;

    if (!SetHandleInformation(pipe_stdout_rd, HANDLE_FLAG_INHERIT, 0))
        return FALSE;

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = INVALID_HANDLE_VALUE;
    si.hStdOutput = pipe_stdout_wr;
    si.hStdError = INVALID_HANDLE_VALUE;

    strcpy(cmdline, cmd);
    if (!CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        return FALSE;

    CloseHandle(pipe_stdout_wr);

    read_from_pipe(pipe_stdout_rd, buf, buf_size);

    ret = WaitForSingleObject(pi.hProcess, 10000);
    if (ret == WAIT_TIMEOUT)
        TerminateProcess(pi.hProcess, 1);

    bret = GetExitCodeProcess(pi.hProcess, rc);
    lok(bret, "GetExitCodeProcess failed: %d\n", GetLastError());

    CloseHandle(pipe_stdout_rd);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return bret;
}

#define compare_query(b,e,c,todo) compare_query_(__FILE__,__LINE__,b,e,c,todo)
static void compare_query_(const char *file, unsigned line, const BYTE *buf,
                           const char *expected, BOOL cmp_len, DWORD todo)
{
    const char *str = (const char *)buf;
    const char *err = "query output does not match expected output";

    if (!cmp_len)
    {
        todo_wine_if (todo & TODO_REG_COMPARE)
            lok(!strcmp(str, expected), "%s\n", err);
    }
    else
    {
        todo_wine_if (todo & TODO_REG_COMPARE)
            lok(!strncmp(str, expected, strlen(expected)), "%s\n", err);
    }
}

/* Unit tests */

static void test_command_syntax(void)
{
    DWORD r;

    run_reg_exe("reg query", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg query /?", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query /h", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query -H", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /v", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /v Test1 /v Test2", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /v Test1 /ve", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /s /s", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    /* Test registry view */
    run_reg_exe("reg query HKCU\\" KEY_BASE " /reg:32 /reg:32", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /reg:32 /reg:64", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /reg:64 /reg:64", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);
}

static void test_query(void)
{
    const char *test1 = "\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\r\n"
        "    Test1    REG_SZ    Hello, World\r\n"
        "    Test2    REG_DWORD    0x123\r\n\r\n";

    const char *test2 = "\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\r\n"
        "    Test1    REG_SZ    Hello, World\r\n\r\n";

    const char *test3 = "\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\r\n"
        "    Test1    REG_SZ    Hello, World\r\n"
        "    Test2    REG_DWORD    0x123\r\n"
        "    Wine    REG_SZ    First instance\r\n\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\\subkey\r\n";

    const char *test4 = "\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\\subkey\r\n"
        "    Test3    REG_SZ    Some string data\r\n"
        "    Test4    REG_DWORD    0xabc\r\n\r\n";

    const char *test5 = "\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\\subkey\r\n"
        "    Test4    REG_DWORD    0xabc\r\n\r\n";

    const char *test6 = "\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\r\n"
        "    Test1    REG_SZ    Hello, World\r\n"
        "    Test2    REG_DWORD    0x123\r\n"
        "    Wine    REG_SZ    First instance\r\n\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\\subkey\r\n"
        "    Test3    REG_SZ    Some string data\r\n"
        "    Test4    REG_DWORD    0xabc\r\n"
        "    Wine    REG_SZ    Second instance\r\n\r\n";

    const char *test7 = "\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\r\n"
        "    Wine    REG_SZ    First instance\r\n\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\\subkey\r\n"
        "    Wine    REG_SZ    Second instance\r\n\r\n";

    const char *test8a = "\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\\subkey1\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\\subkey2\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\\subkey3\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\\subkey4\r\n";

    const char *test8b = "\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\\subkey1\r\n\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\\subkey2\r\n\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\\subkey3\r\n\r\n"
        "HKEY_CURRENT_USER\\" KEY_BASE "\\subkey4\r\n\r\n";

    DWORD r, dword = 0x123;
    HKEY hkey, subkey;
    BYTE buf[512];

    delete_tree(HKEY_CURRENT_USER, KEY_BASE, 0);

    /* Key not present */
    run_reg_exe("reg query HKCU\\" KEY_BASE, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    /* Create a test key */
    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);
    add_value(hkey, "Test1", REG_SZ, "Hello, World", 13);
    add_value(hkey, "Test2", REG_DWORD, &dword, sizeof(dword));

    run_reg_exe("reg query HKCU\\" KEY_BASE " /v Missing", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    read_reg_output("reg query HKCU\\" KEY_BASE, buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test1, FALSE, 0);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /ve", &r);
    ok(r == REG_EXIT_SUCCESS || broken(r == REG_EXIT_FAILURE /* WinXP */),
       "got exit code %d, expected 0\n", r);

    read_reg_output("reg query HKCU\\" KEY_BASE " /v Test1", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test2, FALSE, 0);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /v Test2", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    add_value(hkey, "Wine", REG_SZ, "First instance", 15);

    /* Create a test subkey */
    add_key(hkey, "subkey", 0, &subkey);

    read_reg_output("reg query HKCU\\" KEY_BASE, buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test3, FALSE, 0);

    add_value(subkey, "Test3", REG_SZ, "Some string data", 16);
    dword = 0xabc;
    add_value(subkey, "Test4", REG_DWORD, &dword, sizeof(dword));

    read_reg_output("reg query HKCU\\" KEY_BASE "\\subkey", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test4, FALSE, 0);

    run_reg_exe("reg query HKCU\\" KEY_BASE "\\subkey /v Test3", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    read_reg_output("reg query HKCU\\" KEY_BASE "\\subkey /v Test4", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test5, FALSE, 0);

    add_value(subkey, "Wine", REG_SZ, "Second instance", 16);

    /* Test recursion */
    read_reg_output("reg query HKCU\\" KEY_BASE " /s", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test6, FALSE, 0);

    read_reg_output("reg query HKCU\\" KEY_BASE "\\ /s", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test6, FALSE, 0);

    read_reg_output("reg query HKCU\\" KEY_BASE " /v Wine /s", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS || r == REG_EXIT_FAILURE /* WinXP */,
       "got exit code %d, expected 0\n", r);
    compare_query(buf, test7, TRUE, 0);

    add_value(hkey, NULL, REG_SZ, "Empty", 6);
    add_value(subkey, NULL, REG_SZ, "Empty", 6);
    close_key(subkey);
    close_key(hkey);

    run_reg_exe("reg query HKCU\\" KEY_BASE "\\subkey /ve", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /ve /s", &r);
    ok(r == REG_EXIT_SUCCESS || r == REG_EXIT_FAILURE /* WinXP */,
       "got exit code %d, expected 0\n", r);

    delete_tree(HKEY_CURRENT_USER, KEY_BASE, 0);

    /* Subkeys only */
    add_key(HKEY_CURRENT_USER, KEY_BASE, 0, &hkey);
    add_key(hkey, "subkey1", 0, NULL);
    add_key(hkey, "subkey2", 0, NULL);
    add_key(hkey, "subkey3", 0, NULL);
    add_key(hkey, "subkey4", 0, NULL);
    close_key(hkey);

    read_reg_output("reg query HKCU\\" KEY_BASE, buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test8a, FALSE, 0);

    read_reg_output("reg query HKCU\\" KEY_BASE " /s", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test8b, FALSE, 0);

    delete_tree(HKEY_CURRENT_USER, KEY_BASE, 0);
}

#if 0
static const char *test9a = "\r\n"
    "HKEY_LOCAL_MACHINE\\" KEY_BASE "\r\n"
    "    Test1    REG_SZ    Hello, World\r\n"
    "    Test2    REG_DWORD    0x123\r\n"
    "    Wine    REG_SZ    First instance\r\n\r\n"
    "HKEY_LOCAL_MACHINE\\" KEY_BASE "\\subkey\r\n";

static const char *test9b = "\r\n"
    "HKEY_LOCAL_MACHINE\\" KEY_BASE "\r\n"
    "    Test1    REG_SZ    Hello, World\r\n"
    "    Test2    REG_DWORD    0x123\r\n"
    "    Wine    REG_SZ    First instance\r\n\r\n"
    "HKEY_LOCAL_MACHINE\\" KEY_BASE "\\subkey\r\n"
    "    Test3    REG_SZ    Some string data\r\n"
    "    Test4    REG_DWORD    0xabc\r\n"
    "    Wine    REG_SZ    Second instance\r\n\r\n";

static const char *test9c = "\r\n"
    "HKEY_LOCAL_MACHINE\\" KEY_BASE "\r\n"
    "    Wine    REG_SZ    First instance\r\n\r\n"
    "HKEY_LOCAL_MACHINE\\" KEY_BASE "\\subkey\r\n"
    "    Wine    REG_SZ    Second instance\r\n\r\n";

static void create_test_key(REGSAM sam)
{
    HKEY hkey, subkey;
    DWORD dword;

    add_key(HKEY_LOCAL_MACHINE, KEY_BASE, sam, &hkey);
    add_value(hkey, "Test1", REG_SZ, "Hello, World", 13);
    dword = 0x123;
    add_value(hkey, "Test2", REG_DWORD, &dword, sizeof(dword));
    add_value(hkey, "Wine", REG_SZ, "First instance", 15);

    add_key(hkey, "subkey", sam, &subkey);
    add_value(subkey, "Test3", REG_SZ, "Some string data", 16);
    dword = 0xabc;
    add_value(subkey, "Test4", REG_DWORD, &dword, sizeof(dword));
    add_value(subkey, "Wine", REG_SZ, "Second instance", 16);

    close_key(subkey);
    close_key(hkey);
}

static void test_registry_view_win32(void)
{
    BOOL is_wow64, is_win32;
    DWORD r;
    BYTE buf[512];

    IsWow64Process(GetCurrentProcess(), &is_wow64);
    is_win32 = !is_wow64 && (sizeof(void *) == sizeof(int));

    if (!is_win32) return;

    delete_tree(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);

    /* Try querying the 32-bit registry view (32-bit Windows) */
    create_test_key(KEY_WOW64_32KEY);

    read_reg_output("reg query HKLM\\" KEY_BASE " /reg:32", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9a, FALSE, 0);

    read_reg_output("reg query HKLM\\" KEY_BASE " /s /reg:32", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9b, FALSE, 0);

    read_reg_output("reg query HKLM\\" KEY_BASE " /v Wine /s /reg:32", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9c, TRUE, 0);

    delete_tree(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);

    /* Try querying the 64-bit registry view, which doesn't exist on 32-bit Windows */
    create_test_key(KEY_WOW64_64KEY);

    read_reg_output("reg query HKLM\\" KEY_BASE " /reg:64", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9a, FALSE, 0);

    read_reg_output("reg query HKLM\\" KEY_BASE " /s /reg:64", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9b, FALSE, 0);

    read_reg_output("reg query HKLM\\" KEY_BASE " /v Wine /s /reg:64", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9c, TRUE, 0);

    delete_tree(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY);
}

static void test_registry_view_win64(void)
{
    BOOL is_wow64, is_win64;
    DWORD r;
    BYTE buf[512];

    IsWow64Process(GetCurrentProcess(), &is_wow64);
    is_win64 = !is_wow64 && (sizeof(void *) > sizeof(int));

    if (!is_win64) return;

    delete_tree(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);

    /* Try querying the 32-bit registry view (64-bit Windows) */
    create_test_key(KEY_WOW64_32KEY);

    read_reg_output("reg query HKLM\\" KEY_BASE " /reg:32", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9a, FALSE, 0);

    read_reg_output("reg query HKLM\\" KEY_BASE " /s /reg:32", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9b, FALSE, 0);

    read_reg_output("reg query HKLM\\" KEY_BASE " /v Wine /s /reg:32", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9c, TRUE, 0);

    delete_tree(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);
    delete_tree(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY);

    /* Try querying the 64-bit registry view (64-bit Windows) */
    create_test_key(KEY_WOW64_64KEY);

    read_reg_output("reg query HKLM\\" KEY_BASE " /reg:64", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9a, FALSE, 0);

    read_reg_output("reg query HKLM\\" KEY_BASE " /s /reg:64", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9b, FALSE, 0);

    read_reg_output("reg query HKLM\\" KEY_BASE " /v Wine /s /reg:64", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9c, TRUE, 0);

    delete_tree(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY);
}

static void test_registry_view_wow64(void)
{
    BOOL is_wow64;
    DWORD r;
    BYTE buf[512];

    IsWow64Process(GetCurrentProcess(), &is_wow64);

    if (!is_wow64) return;

    delete_tree(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);

    /* Try querying the 32-bit registry view (WOW64) */
    create_test_key(KEY_WOW64_32KEY);

    read_reg_output("reg query HKLM\\" KEY_BASE " /reg:32", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9a, FALSE, 0);

    read_reg_output("reg query HKLM\\" KEY_BASE " /s /reg:32", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9b, FALSE, 0);

    read_reg_output("reg query HKLM\\" KEY_BASE " /v Wine /s /reg:32", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9c, TRUE, 0);

    delete_tree(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_32KEY);
    delete_tree(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY);

    /* Try querying the 64-bit registry view (WOW64) */
    create_test_key(KEY_WOW64_64KEY);

    read_reg_output("reg query HKLM\\" KEY_BASE " /reg:64", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9a, FALSE, 0);

    read_reg_output("reg query HKLM\\" KEY_BASE " /s /reg:64", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9b, FALSE, 0);

    read_reg_output("reg query HKLM\\" KEY_BASE " /v Wine /s /reg:64", buf, sizeof(buf), &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    compare_query(buf, test9c, TRUE, 0);

    delete_tree(HKEY_LOCAL_MACHINE, KEY_BASE, KEY_WOW64_64KEY);
}
#endif

START_TEST(query)
{
    DWORD r;

    if (!run_reg_exe("reg.exe /?", &r)) {
        win_skip("reg.exe not available, skipping 'query' tests\n");
        return;
    }

    test_command_syntax();
    test_query();

    /* Check if reg.exe is running with elevated privileges */
    if (!is_elevated_process())
    {
        win_skip("reg.exe is not running with elevated privileges; "
                 "skipping registry view tests\n");
        return;
    }

#if 0
    test_registry_view_win32();
    test_registry_view_win64();
    test_registry_view_wow64();
#endif

}
