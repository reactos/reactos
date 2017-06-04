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

#include <wine/test.h>
#include <winnls.h>
#include <winreg.h>

#define lok ok_(__FILE__,line)
#define KEY_BASE "Software\\Wine\\reg_test"
#define REG_EXIT_SUCCESS 0
#define REG_EXIT_FAILURE 1
#define TODO_REG_TYPE    (0x0001u)
#define TODO_REG_SIZE    (0x0002u)
#define TODO_REG_DATA    (0x0004u)

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
    todo_wine_if (todo & TODO_REG_DATA)
        lok(memcmp(data, exp_data, size) == 0, "got wrong data\n");
}

#define verify_reg_nonexist(k,v) verify_reg_nonexist_(__LINE__,k,v)
static void verify_reg_nonexist_(unsigned line, HKEY hkey, const char *value)
{
    LONG err;

    err = RegQueryValueExA(hkey, value, NULL, NULL, NULL, NULL);
    lok(err == ERROR_FILE_NOT_FOUND, "registry value '%s' shouldn't exist; got %d, expected 2\n",
        value, err);
}

static void test_add(void)
{
    HKEY hkey, subkey;
    LONG err;
    DWORD r, dword, type, size;
    char buffer[22];

    run_reg_exe("reg add", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg add /?", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(err == ERROR_SUCCESS || err == ERROR_FILE_NOT_FOUND, "got %d\n", err);

    err = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    ok(err == ERROR_FILE_NOT_FOUND, "got %d\n", err);

    run_reg_exe("reg add HKCU\\" KEY_BASE " /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    err = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    ok(err == ERROR_SUCCESS, "key creation failed, got %d\n", err);

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
    err = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE "\\keytest3", 0, KEY_READ, &subkey);
    ok(err == ERROR_SUCCESS, "key creation failed, got %d\n", err);
    RegCloseKey(subkey);
    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE "\\keytest3");
    ok(err == ERROR_SUCCESS, "got exit code %d\n", err);

    run_reg_exe("reg add HKCU\\" KEY_BASE "\\keytest4 /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %u\n", r);
    err = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE "\\keytest4", 0, KEY_READ, &subkey);
    ok(err == ERROR_SUCCESS, "key creation failed, got %d\n", err);
    RegCloseKey(subkey);
    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE "\\keytest4");
    ok(err == ERROR_SUCCESS, "got exit code %d\n", err);

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

    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(err == ERROR_SUCCESS, "got %d\n", err);
}

static void test_delete(void)
{
    HKEY hkey, hsubkey;
    LONG err;
    DWORD r;
    const DWORD deadbeef = 0xdeadbeef;

    run_reg_exe("reg delete", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg delete /?", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    err = RegCreateKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL);
    ok(err == ERROR_SUCCESS, "got %d\n", err);

    err = RegSetValueExA(hkey, "foo", 0, REG_DWORD, (LPBYTE)&deadbeef, sizeof(deadbeef));
    ok(err == ERROR_SUCCESS, "got %d\n" ,err);

    err = RegSetValueExA(hkey, "bar", 0, REG_DWORD, (LPBYTE)&deadbeef, sizeof(deadbeef));
    ok(err == ERROR_SUCCESS, "got %d\n" ,err);

    err = RegSetValueExA(hkey, "", 0, REG_DWORD, (LPBYTE)&deadbeef, sizeof(deadbeef));
    ok(err == ERROR_SUCCESS, "got %d\n" ,err);

    err = RegCreateKeyExA(hkey, "subkey", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hsubkey, NULL);
    ok(err == ERROR_SUCCESS, "got %d\n" ,err);
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
    err = RegOpenKeyExA(hkey, "subkey", 0, KEY_READ, &hsubkey);
    ok(err == ERROR_SUCCESS, "got %d\n", err);
    RegCloseKey(hsubkey);
    RegCloseKey(hkey);

    run_reg_exe("reg delete HKCU\\" KEY_BASE " /f", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    ok(err == ERROR_FILE_NOT_FOUND, "got %d\n", err);

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

    run_reg_exe("reg query", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg query /?", &r);
    ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    /* Create a test key */
    err = RegCreateKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL);
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    run_reg_exe("reg query HKCU\\" KEY_BASE " /ve", &r);
    ok(r == REG_EXIT_SUCCESS || broken(r == REG_EXIT_FAILURE /* WinXP */),
       "got exit code %d, expected 0\n", r);

    err = RegSetValueExA(key, "Test", 0, REG_SZ, (BYTE *)hello, sizeof(hello));
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    err = RegSetValueExA(key, "Wine", 0, REG_DWORD, (BYTE *)&dword1, sizeof(dword1));
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    err = RegSetValueExA(key, NULL, 0, REG_SZ, (BYTE *)empty1, sizeof(empty1));
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

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
    err = RegCreateKeyExA(key, "Subkey", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &subkey, NULL);
    ok(err == ERROR_SUCCESS, "got %d\n", err);

    err = RegSetValueExA(subkey, "Test", 0, REG_SZ, (BYTE *)world, sizeof(world));
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    err = RegSetValueExA(subkey, "Wine", 0, REG_DWORD, (BYTE *)&dword2, sizeof(dword2));
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    err = RegSetValueExA(subkey, NULL, 0, REG_SZ, (BYTE *)empty2, sizeof(empty2));
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

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
    err = RegDeleteKeyA(key, "subkey");
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    err = RegCloseKey(key);
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    run_reg_exe("reg query HKCU\\" KEY_BASE "\\subkey", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

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

static BOOL write_reg_file(const char *value, char *tmp_name)
{
    static const char regedit4[] = "REGEDIT4";
    static const char key[] = "[HKEY_CURRENT_USER\\" KEY_BASE "]";
    char file_data[MAX_PATH], tmp_path[MAX_PATH];
    HANDLE hfile;
    DWORD written;
    BOOL ret;

    sprintf(file_data, "%s\n\n%s\n%s\n", regedit4, key, value);

    GetTempPathA(MAX_PATH, tmp_path);
    GetTempFileNameA(tmp_path, "reg", 0, tmp_name);

    hfile = CreateFileA(tmp_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hfile == INVALID_HANDLE_VALUE)
        return FALSE;

    ret = WriteFile(hfile, file_data, strlen(file_data), &written, NULL);
    CloseHandle(hfile);
    return ret;
}

#define test_import_str(c,r) test_import_str_(__LINE__,c,r)
static BOOL test_import_str_(unsigned line, const char *file_contents, DWORD *rc)
{
    HANDLE regfile;
    DWORD written;
    BOOL ret;

    regfile = CreateFileA("test.reg", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL, NULL);
    lok(regfile != INVALID_HANDLE_VALUE, "Failed to create test.reg file\n");
    if(regfile == INVALID_HANDLE_VALUE)
        return FALSE;

    ret = WriteFile(regfile, file_contents, strlen(file_contents), &written, NULL);
    lok(ret, "WriteFile failed: %u\n", GetLastError());
    CloseHandle(regfile);

    run_reg_exe("reg import test.reg", rc);

    ret = DeleteFileA("test.reg");
    lok(ret, "DeleteFile failed: %u\n", GetLastError());

    return ret;
}

#define test_import_wstr(c,r) test_import_wstr_(__LINE__,c,r)
static BOOL test_import_wstr_(unsigned line, const char *file_contents, DWORD *rc)
{
    int lenA, len, memsize;
    WCHAR *wstr;
    HANDLE regfile;
    DWORD written;
    BOOL ret;

    lenA = strlen(file_contents);

    len = MultiByteToWideChar(CP_UTF8, 0, file_contents, lenA, NULL, 0);
    memsize = len * sizeof(WCHAR);
    wstr = HeapAlloc(GetProcessHeap(), 0, memsize);
    if (!wstr) return FALSE;
    MultiByteToWideChar(CP_UTF8, 0, file_contents, lenA, wstr, memsize);

    regfile = CreateFileA("test.reg", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL, NULL);
    lok(regfile != INVALID_HANDLE_VALUE, "Failed to create test.reg file\n");
    if(regfile == INVALID_HANDLE_VALUE)
        return FALSE;

    ret = WriteFile(regfile, wstr, memsize, &written, NULL);
    lok(ret, "WriteFile failed: %u\n", GetLastError());
    CloseHandle(regfile);

    HeapFree(GetProcessHeap(), 0, wstr);

    run_reg_exe("reg import test.reg", rc);

    ret = DeleteFileA("test.reg");
    lok(ret, "DeleteFile failed: %u\n", GetLastError());

    return ret;
}

static void test_import(void)
{
    DWORD r, dword = 0x123, type, size;
    char test1_reg[MAX_PATH], test2_reg[MAX_PATH], cmdline[MAX_PATH];
    char test_string[] = "Test string", buffer[8];
    HKEY hkey, subkey;
    LONG err;
    BYTE hex[8];

    run_reg_exe("reg import", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    run_reg_exe("reg import /?", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    run_reg_exe("reg import missing.reg", &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    /* Create test files */
    ok(write_reg_file("\"Wine\"=dword:00000123", test1_reg), "Failed to write registry file\n");
    ok(write_reg_file("@=\"Test string\"", test2_reg), "Failed to write registry file\n");

    sprintf(cmdline, "reg import %s", test1_reg);
    run_reg_exe(cmdline, &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    sprintf(cmdline, "reg import %s", test2_reg);
    run_reg_exe(cmdline, &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    err = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    todo_wine verify_reg(hkey, "Wine", REG_DWORD, &dword, sizeof(dword), 0);
    todo_wine verify_reg(hkey, "", REG_SZ, test_string, sizeof(test_string), 0);

    err = RegCloseKey(hkey);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    sprintf(cmdline, "reg import %s %s", test1_reg, test2_reg);
    run_reg_exe(cmdline, &r);
    ok(r == REG_EXIT_FAILURE, "got exit code %d, expected 1\n", r);

    DeleteFileA(test1_reg);
    DeleteFileA(test2_reg);

    /* Test file contents */
    test_import_str("regedit\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_str("regedit4\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_str("REGEDIT", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("REGEDIT\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("REGEDIT4\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str(" REGEDIT4\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("\tREGEDIT4\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

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
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("REGEDIT5\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("REGEDIT9\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("REGEDIT 4\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("REGEDIT4 FOO\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("REGEDIT4\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    err = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    test_import_str("REGEDIT3\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test1\"=\"Value\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test1");

    test_import_str("regedit4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test2\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test2");

    test_import_str("Regedit4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test3\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test3");

    test_import_str("REGEDIT 4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test4\"=\"Value\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test4");

    test_import_str("REGEDIT4FOO\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test5\"=\"Value\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test5");

    test_import_str("REGEDIT4 FOO\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test6\"=\"Value\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test6");

    test_import_str("REGEDIT5\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test7\"=\"Value\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test7");

    test_import_str("REGEDIT9\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test8\"=\"Value\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test8");

    test_import_str("Windows Registry Editor Version 4.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test9\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test9");

    test_import_str("Windows Registry Editor Version 5\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test10\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test10");

    test_import_str("WINDOWS REGISTRY EDITOR VERSION 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test11\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test11");

    test_import_str("Windows Registry Editor version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test12\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test12");

    test_import_str("REGEDIT4\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test1\"=\"Value1\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Test1", REG_SZ, "Value1", 7, 0);

    test_import_str("REGEDIT4\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test2\"=\"Value2\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Test2", REG_SZ, "Value2", 7, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test3\"=\"Value3\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Test3", REG_SZ, "Value3", 7, 0);

    test_import_str("Windows Registry Editor Version 4.00\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_str("Windows Registry Editor Version 5.00\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_str("Windows Registry Editor Version 5.00\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test4\"=\"Value4\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Test4", REG_SZ, "Value4", 7, 0);

    test_import_str("Windows Registry Editor Version 5.00\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test5\"=\"Value5\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Test5", REG_SZ, "Value5", 7, 0);

    test_import_str("Windows Registry Editor Version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test6\"=\"Value6\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Test6", REG_SZ, "Value6", 7, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Line1\"=\"Value1\"\n\n"
                    "\"Line2\"=\"Value2\"\n\n\n"
                    "\"Line3\"=\"Value3\"\n\n\n\n"
                    "\"Line4\"=\"Value4\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Line1", REG_SZ, "Value1", 7, TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);
    todo_wine verify_reg(hkey, "Line2", REG_SZ, "Value2", 7, TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);
    todo_wine verify_reg(hkey, "Line3", REG_SZ, "Value3", 7, TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);
    todo_wine verify_reg(hkey, "Line4", REG_SZ, "Value4", 7, TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);

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
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x782;
    todo_wine verify_reg(hkey, "Wine1", REG_DWORD, &dword, sizeof(dword),
                         TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);
    todo_wine verify_reg(hkey, "Wine2", REG_SZ, "Test Value", 11,
                         TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);
    todo_wine verify_reg(hkey, "Wine3", REG_MULTI_SZ, "Line concatenation\0", 20,
                         TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);
    todo_wine verify_reg(hkey, "", REG_SZ, "Test", 5,
                         TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);
    dword = 0x12345678;
    todo_wine verify_reg(hkey, "Wine4", REG_DWORD, &dword, sizeof(dword),
                         TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine5\"=\"No newline\"", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegQueryValueExA(hkey, "Wine5", NULL, NULL, NULL, NULL);
    todo_wine ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND /* WinXP */),
       "got %d, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine5", REG_SZ, "No newline", 11, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine6\"=dword:00000050\n\n"
                    "\"Wine7\"=\"No newline\"", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x50;
    todo_wine verify_reg(hkey, "Wine6", REG_DWORD, &dword, sizeof(dword),
                         TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);
    err = RegQueryValueExA(hkey, "Wine7", NULL, NULL, NULL, NULL);
    todo_wine ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND /* WinXP */),
       "got %d, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine7", REG_SZ, "No newline", 11, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "#comment\\\n"
                    "\"Wine8\"=\"Line 1\"\n"
                    ";comment\\\n"
                    "\"Wine9\"=\"Line 2\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine8", REG_SZ, "Line 1", 7, 0);
    todo_wine verify_reg(hkey, "Wine9", REG_SZ, "Line 2", 7, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine10\"=\"Value 1\"#comment\n"
                    "\"Wine11\"=\"Value 2\";comment\n"
                    "\"Wine12\"=dword:01020304 #comment\n"
                    "\"Wine13\"=dword:02040608 ;comment\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine10");
    todo_wine verify_reg(hkey, "Wine11", REG_SZ, "Value 2", 8, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine12");
    dword = 0x2040608;
    todo_wine verify_reg(hkey, "Wine13", REG_DWORD, &dword, sizeof(dword), 0);

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
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine14");
    todo_wine verify_reg(hkey, "Wine15", REG_SZ, "A valid line", 13, 0);
    todo_wine verify_reg(hkey, "Wine16", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    todo_wine verify_reg(hkey, "Wine17", REG_SZ, "Another valid line", 19, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "#\"Comment1\"=\"Value 1\"\n"
                    ";\"Comment2\"=\"Value 2\"\n"
                    "    #\"Comment3\"=\"Value 3\"\n"
                    "    ;\"Comment4\"=\"Value 4\"\n"
                    "\"Wine18\"=\"Value 6\"#\"Comment5\"=\"Value 5\"\n"
                    "\"Wine19\"=\"Value 7\";\"Comment6\"=\"Value 6\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Comment1");
    todo_wine verify_reg_nonexist(hkey, "Comment2");
    todo_wine verify_reg_nonexist(hkey, "Comment3");
    todo_wine verify_reg_nonexist(hkey, "Comment4");
    todo_wine verify_reg_nonexist(hkey, "Wine18");
    todo_wine verify_reg_nonexist(hkey, "Comment5");
    todo_wine verify_reg(hkey, "Wine19", REG_SZ, "Value 7", 8, TODO_REG_SIZE|TODO_REG_DATA);
    todo_wine verify_reg_nonexist(hkey, "Comment6");

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
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine20");
    todo_wine verify_reg_nonexist(hkey, "Wine21");
    todo_wine verify_reg(hkey, "Wine22", REG_SZ, "#comment1", 10, 0);
    todo_wine verify_reg(hkey, "Wine23", REG_SZ, ";comment2", 10, 0);
    todo_wine verify_reg(hkey, "Wine24", REG_SZ, "Value#comment3", 15, 0);
    todo_wine verify_reg(hkey, "Wine25", REG_SZ, "Value;comment4", 15, 0);
    todo_wine verify_reg(hkey, "Wine26", REG_SZ, "Value #comment5", 16, 0);
    todo_wine verify_reg(hkey, "Wine27", REG_SZ, "Value ;comment6", 16, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine28");
    todo_wine verify_reg_nonexist(hkey, "Wine29");
    todo_wine verify_reg_nonexist(hkey, "Wine30");
    dword = 0x00000004;
    todo_wine verify_reg(hkey, "Wine31", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line1\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,\\;comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Multi-Line1", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line2\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,;comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Multi-Line2", REG_MULTI_SZ, "Line concat", 12, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line3\"=hex(7):4c,69,6e,65,20\\\n"
                    ",63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Multi-Line3");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line4\"=hex(7):4c,69,6e,65,20\\\n"
                    "  ,63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Multi-Line4");

    test_import_str("Windows Registry Editor Version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line5\"=hex(7):4c,69,6e,65,20\\\n"
                    ",63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Multi-Line5");

    test_import_str("Windows Registry Editor Version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line6\"=hex(7):4c,69,6e,65,20\\\n"
                    "  ,63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Multi-Line6");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line7\"=hex(7):4c,69,6e,\\;comment\n"
                    "  65,20,\\;comment\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,\\;comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Multi-Line7", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line8\"=hex(7):4c,69,6e,\\;#comment\n"
                    "  65,20,\\;#comment\n"
                    "  63,6f,6e,\\;#comment\n"
                    "  63,61,74,\\;#comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Multi-Line8", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line9\"=hex(7):4c,69,6e,\\;comment\n"
                    "  65,20,\\;comment\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,\\#comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Multi-Line9");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line10\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,\\\n\n"
                    "  65,6e,\\;comment\n\n"
                    "  61,74,69,6f,6e,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Multi-Line10", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine32a\"=dword:1\n"
                    "\"Wine32b\"=dword:4444\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x1;
    todo_wine verify_reg(hkey, "Wine32a", REG_DWORD, &dword, sizeof(dword), 0);
    dword = 0x4444;
    todo_wine verify_reg(hkey, "Wine32b", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine33a\"=dword:\n"
                    "\"Wine33b\"=dword:hello\n"
                    "\"Wine33c\"=dword:123456789\n"
                    "\"Wine33d\"=dword:012345678\n"
                    "\"Wine33e\"=dword:000000001\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine33a");
    todo_wine verify_reg_nonexist(hkey, "Wine33b");
    todo_wine verify_reg_nonexist(hkey, "Wine33c");
    todo_wine verify_reg_nonexist(hkey, "Wine33d");
    todo_wine verify_reg_nonexist(hkey, "Wine33e");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine34a\"=dword:12345678abc\n"
                    "\"Wine34b\"=dword:12345678 abc\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine34a");
    todo_wine verify_reg_nonexist(hkey, "Wine34b");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine35a\"=dword:0x123\n"
                    "\"Wine35b\"=dword:123 456\n"
                    "\"Wine35c\"=dword:1234 5678\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine35a");
    todo_wine verify_reg_nonexist(hkey, "Wine35b");
    todo_wine verify_reg_nonexist(hkey, "Wine35c");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine36a\"=dword:1234;5678\n"
                    "\"Wine36b\"=dword:1234 ;5678\n"
                    "\"Wine36c\"=dword:1234#5678\n"
                    "\"Wine36d\"=dword:1234 #5678\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x1234;
    todo_wine verify_reg(hkey, "Wine36a", REG_DWORD, &dword, sizeof(dword), 0);
    todo_wine verify_reg(hkey, "Wine36b", REG_DWORD, &dword, sizeof(dword), 0);
    todo_wine verify_reg_nonexist(hkey, "Wine36c");
    todo_wine verify_reg_nonexist(hkey, "Wine36d");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine37a\"=\"foo\"bar\"\n"
                    "\"Wine37b\"=\"foo\"\"bar\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine37a");
    todo_wine verify_reg_nonexist(hkey, "Wine37b");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Empty string\"=\"\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Empty string", REG_SZ, "", 1, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Test38a\"=\n"
                    "\"Test38b\"=\\\"\n"
                    "\"Test38c\"=\\\"Value\\\"\n"
                    "\"Test38d\"=\\\"Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test38a");
    todo_wine verify_reg_nonexist(hkey, "Test38b");
    todo_wine verify_reg_nonexist(hkey, "Test38c");
    todo_wine verify_reg_nonexist(hkey, "Test38d");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine39a\"=\"Value1\"  ;comment\n"
                    "\"Wine39b\"=\"Value2\"\t\t;comment\n"
                    "\"Wine39c\"=\"Value3\"  #comment\n"
                    "\"Wine39d\"=\"Value4\"\t\t#comment\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine39a", REG_SZ, "Value1", 7, 0);
    todo_wine verify_reg(hkey, "Wine39b", REG_SZ, "Value2", 7, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine39c");
    todo_wine verify_reg_nonexist(hkey, "Wine39d");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"TestNoBeginQuote\"=Asdffdsa\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "TestNoBeginQuote");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"TestNoEndQuote\"=\"Asdffdsa\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "TestNoEndQuote");

    test_import_str("REGEDIT4\n\n"
                   "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                   "\"TestNoQuotes\"=Asdffdsa\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "TestNoQuotes");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "NameNoBeginQuote\"=\"Asdffdsa\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "NameNoBeginQuote");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"NameNoEndQuote=\"Asdffdsa\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "NameNoEndQuote");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "NameNoQuotes=\"Asdffdsa\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "NameNoQuotes");

    test_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"MixedQuotes=Asdffdsa\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "MixedQuotes");
    todo_wine verify_reg_nonexist(hkey, "MixedQuotes=Asdffdsa");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine40a\"=hex(2):4c,69,6e,65,00\n"
                    "\"Wine40b\"=\"Value 1\"\n"
                    "\"Wine40c\"=hex(2):4c,69,6e,65\\\n"
                    "\"Wine40d\"=\"Value 2\"\n"
                    "\"Wine40e\"=hex(2):4c,69,6e,65,\\\n"
                    "\"Wine40f\"=\"Value 3\"\n"
                    "\"Wine40g\"=\"Value 4\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine40a", REG_EXPAND_SZ, "Line", 5, 0);
    todo_wine verify_reg(hkey, "Wine40b", REG_SZ, "Value 1", 8, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine40c");
    todo_wine verify_reg(hkey, "Wine40d", REG_SZ, "Value 2", 8, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine40e");
    todo_wine verify_reg_nonexist(hkey, "Wine40f");
    todo_wine verify_reg(hkey, "Wine40g", REG_SZ, "Value 4", 8, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine41a\"=dword:1234\\\n"
                    "5678\n"
                    "\"Wine41b\"=\"Test \\\n"
                    "Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine41a");
    todo_wine verify_reg_nonexist(hkey, "Wine41b");

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"double\\\"quote\"=\"valid \\\"or\\\" not\"\n"
                    "\"single'quote\"=dword:00000008\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "double\"quote", REG_SZ, "valid \"or\" not", 15, 0);
    dword = 0x00000008;
    todo_wine verify_reg(hkey, "single'quote", REG_DWORD, &dword, sizeof(dword), 0);

    /* Test key name and value name concatenation */
    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\\n"
                    "Subkey1]\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegOpenKeyExA(hkey, "Subkey1", 0, KEY_READ, &subkey);
    todo_wine ok(err == ERROR_FILE_NOT_FOUND, "got %d, expected 2\n", err);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\n"
                    "\\Subkey2]\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegOpenKeyExA(hkey, "Subkey2", 0, KEY_READ, &subkey);
    todo_wine ok(err == ERROR_FILE_NOT_FOUND, "got %d, expected 2\n", err);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine\\\n"
                    "42a\"=\"Value 1\"\n"
                    "\"Wine42b\"=\"Value 2\"\n"
                    "\"Wine\n"
                    "\\42c\"=\"Value 3\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine42a");
    todo_wine verify_reg(hkey, "Wine42b", REG_SZ, "Value 2", 8, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine42c");

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
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine43a", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    todo_wine verify_reg(hkey, "Wine43b", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine43c");
    todo_wine verify_reg_nonexist(hkey, "Wine43d");
    todo_wine verify_reg_nonexist(hkey, "Wine43e");
    todo_wine verify_reg_nonexist(hkey, "Wine43f");
    todo_wine verify_reg_nonexist(hkey, "Wine43g");
    todo_wine verify_reg(hkey, "Wine43h", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    todo_wine verify_reg(hkey, "Wine43i", REG_NONE, "V\0a\0l\0u", 8, 0);
    todo_wine verify_reg(hkey, "Wine43j", REG_NONE, "V\0a\0l\0u", 8, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine43k");

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
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine44a", REG_EXPAND_SZ, "%PATH%", 7, 0);
    todo_wine verify_reg(hkey, "Wine44b", REG_EXPAND_SZ, "%PATH%", 7, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine44c");
    todo_wine verify_reg_nonexist(hkey, "Wine44d");
    todo_wine verify_reg_nonexist(hkey, "Wine44e");
    todo_wine verify_reg_nonexist(hkey, "Wine44f");
    todo_wine verify_reg_nonexist(hkey, "Wine44g");
    todo_wine verify_reg(hkey, "Wine44h", REG_EXPAND_SZ, "%PATH%", 7, 0);
    /* Wine44i */
    size = sizeof(buffer);
    err = RegQueryValueExA(hkey, "Wine44i", NULL, &type, (BYTE *)&buffer, &size);
    todo_wine ok(err == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", err);
    todo_wine ok(type == REG_EXPAND_SZ, "got wrong type %u, expected %u\n", type, REG_EXPAND_SZ);
    todo_wine ok(size == 6 || broken(size == 5) /* WinXP */, "got wrong size %u, expected 6\n", size);
    todo_wine ok(memcmp(buffer, "%PATH", size) == 0, "got wrong data\n");
    /* Wine44j */
    size = sizeof(buffer);
    memset(buffer, '-', size);
    err = RegQueryValueExA(hkey, "Wine44j", NULL, &type, (BYTE *)&buffer, &size);
    todo_wine ok(err == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", err);
    todo_wine ok(type == REG_EXPAND_SZ, "got wrong type %u, expected %u\n", type, REG_EXPAND_SZ);
    todo_wine ok(size == 6 || broken(size == 5) /* WinXP */, "got wrong size %u, expected 6\n", size);
    todo_wine ok(memcmp(buffer, "%PATH", size) == 0, "got wrong data\n");
    /* Wine44k */
    todo_wine verify_reg_nonexist(hkey, "Wine44k");

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
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    hex[0] = 0x11; hex[1] = 0x22; hex[2] = 0x33; hex[3] = 0x44;
    hex[4] = 0x55; hex[5] = 0x66; hex[6] = 0x77; hex[7] = 0x88;
    todo_wine verify_reg(hkey, "Wine45a", REG_BINARY, hex, sizeof(hex), 0);
    todo_wine verify_reg(hkey, "Wine45b", REG_BINARY, hex, sizeof(hex), 0);
    todo_wine verify_reg_nonexist(hkey, "Wine45c");
    todo_wine verify_reg_nonexist(hkey, "Wine45d");
    todo_wine verify_reg_nonexist(hkey, "Wine45e");
    todo_wine verify_reg_nonexist(hkey, "Wine45f");
    todo_wine verify_reg_nonexist(hkey, "Wine45g");
    todo_wine verify_reg(hkey, "Wine45h", REG_BINARY, hex, sizeof(hex), 0);
    todo_wine verify_reg(hkey, "Wine45i", REG_BINARY, hex, 6, 0);
    todo_wine verify_reg(hkey, "Wine45j", REG_BINARY, hex, 6, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine45k");

    /* Test import with subkeys */
    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey\"1]\n"
                    "\"Wine\\\\31\"=\"Test value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegOpenKeyExA(hkey, "Subkey\"1", 0, KEY_READ, &subkey);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);
    todo_wine verify_reg(subkey, "Wine\\31", REG_SZ, "Test value", 11, 0);
    err = RegCloseKey(subkey);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);
    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE "\\Subkey\"1");
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey/2]\n"
                    "\"123/\\\"4;'5\"=\"Random value name\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegOpenKeyExA(hkey, "Subkey/2", 0, KEY_READ, &subkey);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);
    todo_wine verify_reg(subkey, "123/\"4;'5", REG_SZ, "Random value name", 18, 0);
    err = RegCloseKey(subkey);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);
    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE "\\Subkey/2");
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    err = RegCloseKey(hkey);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    /* Test file contents - Unicode */
    test_import_wstr("REGEDIT\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_wstr("REGEDIT4\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_wstr("\xef\xbb\xbfREGEDIT", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbfREGEDIT\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbfREGEDIT4", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbf REGEDIT4\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbf\tREGEDIT4\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbf\nREGEDIT4\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    err = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    test_import_wstr("\xef\xbb\xbfREGEDIT3\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test1\"=\"Value\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test1");

    test_import_wstr("\xef\xbb\xbfregedit4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test2\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test2");

    test_import_wstr("\xef\xbb\xbfRegedit4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test3\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test3");

    test_import_wstr("\xef\xbb\xbfREGEDIT 4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test4\"=\"Value\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test4");

    test_import_wstr("\xef\xbb\xbfREGEDIT4FOO\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test5\"=\"Value\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test5");

    test_import_wstr("\xef\xbb\xbfREGEDIT4 FOO\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test6\"=\"Value\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test6");

    test_import_wstr("\xef\xbb\xbfREGEDIT5\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test7\"=\"Value\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test7");

    test_import_wstr("\xef\xbb\xbfREGEDIT9\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test8\"=\"Value\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test8");

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Unicode1\"=\"Value1\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Unicode1", REG_SZ, "Value1", 7, 0);

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Unicode2\"=\"Value2\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Unicode2", REG_SZ, "Value2", 7, 0);

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Unicode3\"=\"Value3\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Unicode3", REG_SZ, "Value3", 7, 0);

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
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbfWINDOWS Registry Editor Version 5.00\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_wstr("\xef\xbb\xbf Windows Registry Editor Version 5.00\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbf\tWindows Registry Editor Version 5.00\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);

    test_import_wstr("\xef\xbb\xbf\nWindows Registry Editor Version 5.00\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 4.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test9\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test9");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test10\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test10");

    test_import_wstr("\xef\xbb\xbfWINDOWS REGISTRY EDITOR VERSION 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test11\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test11");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test12\"=\"Value\"\n", &r);
    ok(r == REG_EXIT_FAILURE || broken(r == REG_EXIT_SUCCESS) /* WinXP */,
       "got exit code %d, expected 1\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test12");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Unicode4\"=\"Value4\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Unicode4", REG_SZ, "Value4", 7, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Unicode5\"=\"Value5\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Unicode5", REG_SZ, "Value5", 7, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Unicode6\"=\"Value6\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Unicode6", REG_SZ, "Value6", 7, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Line1\"=\"Value1\"\n\n"
                     "\"Line2\"=\"Value2\"\n\n\n"
                     "\"Line3\"=\"Value3\"\n\n\n\n"
                     "\"Line4\"=\"Value4\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Line1", REG_SZ, "Value1", 7, TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);
    todo_wine verify_reg(hkey, "Line2", REG_SZ, "Value2", 7, TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);
    todo_wine verify_reg(hkey, "Line3", REG_SZ, "Value3", 7, TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);
    todo_wine verify_reg(hkey, "Line4", REG_SZ, "Value4", 7, TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);

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
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x782;
    todo_wine verify_reg(hkey, "Wine1", REG_DWORD, &dword, sizeof(dword),
                         TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);
    todo_wine verify_reg(hkey, "Wine2", REG_SZ, "Test Value", 11,
                         TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);
    todo_wine verify_reg(hkey, "Wine3", REG_MULTI_SZ, "Line concatenation\0", 20,
                         TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);
    todo_wine verify_reg(hkey, "", REG_SZ, "Test", 5,
                         TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);
    dword = 0x12345678;
    todo_wine verify_reg(hkey, "Wine4", REG_DWORD, &dword, sizeof(dword),
                         TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine5\"=\"No newline\"", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegQueryValueExA(hkey, "Wine5", NULL, NULL, NULL, NULL);
    todo_wine ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND /* WinXP */),
       "got %d, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine5", REG_SZ, "No newline", 11, 0);

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine6\"=dword:00000050\n\n"
                     "\"Wine7\"=\"No newline\"", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x50;
    todo_wine verify_reg(hkey, "Wine6", REG_DWORD, &dword, sizeof(dword),
                         TODO_REG_TYPE|TODO_REG_SIZE|TODO_REG_DATA);
    err = RegQueryValueExA(hkey, "Wine7", NULL, NULL, NULL, NULL);
    todo_wine ok(err == ERROR_SUCCESS || broken(err == ERROR_FILE_NOT_FOUND /* WinXP */),
       "got %d, expected 0\n", err);
    if (err == ERROR_SUCCESS)
        verify_reg(hkey, "Wine7", REG_SZ, "No newline", 11, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "#comment\\\n"
                     "\"Wine8\"=\"Line 1\"\n"
                     ";comment\\\n"
                     "\"Wine9\"=\"Line 2\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine8", REG_SZ, "Line 1", 7, 0);
    todo_wine verify_reg(hkey, "Wine9", REG_SZ, "Line 2", 7, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine10\"=\"Value 1\"#comment\n"
                     "\"Wine11\"=\"Value 2\";comment\n"
                     "\"Wine12\"=dword:01020304 #comment\n"
                     "\"Wine13\"=dword:02040608 ;comment\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine10");
    todo_wine verify_reg(hkey, "Wine11", REG_SZ, "Value 2", 8, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine12");
    dword = 0x2040608;
    todo_wine verify_reg(hkey, "Wine13", REG_DWORD, &dword, sizeof(dword), 0);

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
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine14");
    todo_wine verify_reg(hkey, "Wine15", REG_SZ, "A valid line", 13, 0);
    todo_wine verify_reg(hkey, "Wine16", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    todo_wine verify_reg(hkey, "Wine17", REG_SZ, "Another valid line", 19, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "#\"Comment1\"=\"Value 1\"\n"
                     ";\"Comment2\"=\"Value 2\"\n"
                     "    #\"Comment3\"=\"Value 3\"\n"
                     "    ;\"Comment4\"=\"Value 4\"\n"
                     "\"Wine18\"=\"Value 6\"#\"Comment5\"=\"Value 5\"\n"
                     "\"Wine19\"=\"Value 7\";\"Comment6\"=\"Value 6\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Comment1");
    todo_wine verify_reg_nonexist(hkey, "Comment2");
    todo_wine verify_reg_nonexist(hkey, "Comment3");
    todo_wine verify_reg_nonexist(hkey, "Comment4");
    todo_wine verify_reg_nonexist(hkey, "Wine18");
    todo_wine verify_reg_nonexist(hkey, "Comment5");
    todo_wine verify_reg(hkey, "Wine19", REG_SZ, "Value 7", 8, TODO_REG_SIZE|TODO_REG_DATA);
    todo_wine verify_reg_nonexist(hkey, "Comment6");

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
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine20");
    todo_wine verify_reg_nonexist(hkey, "Wine21");
    todo_wine verify_reg(hkey, "Wine22", REG_SZ, "#comment1", 10, 0);
    todo_wine verify_reg(hkey, "Wine23", REG_SZ, ";comment2", 10, 0);
    todo_wine verify_reg(hkey, "Wine24", REG_SZ, "Value#comment3", 15, 0);
    todo_wine verify_reg(hkey, "Wine25", REG_SZ, "Value;comment4", 15, 0);
    todo_wine verify_reg(hkey, "Wine26", REG_SZ, "Value #comment5", 16, 0);
    todo_wine verify_reg(hkey, "Wine27", REG_SZ, "Value ;comment6", 16, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine28");
    todo_wine verify_reg_nonexist(hkey, "Wine29");
    todo_wine verify_reg_nonexist(hkey, "Wine30");
    dword = 0x00000004;
    todo_wine verify_reg(hkey, "Wine31", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine32a\"=dword:1\n"
                     "\"Wine32b\"=dword:4444\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x1;
    todo_wine verify_reg(hkey, "Wine32a", REG_DWORD, &dword, sizeof(dword), 0);
    dword = 0x4444;
    todo_wine verify_reg(hkey, "Wine32b", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine33a\"=dword:\n"
                     "\"Wine33b\"=dword:hello\n"
                     "\"Wine33c\"=dword:123456789\n"
                     "\"Wine33d\"=dword:012345678\n"
                     "\"Wine33e\"=dword:000000001\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine33a");
    todo_wine verify_reg_nonexist(hkey, "Wine33b");
    todo_wine verify_reg_nonexist(hkey, "Wine33c");
    todo_wine verify_reg_nonexist(hkey, "Wine33d");
    todo_wine verify_reg_nonexist(hkey, "Wine33e");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine34a\"=dword:12345678abc\n"
                     "\"Wine34b\"=dword:12345678 abc\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine34a");
    todo_wine verify_reg_nonexist(hkey, "Wine34b");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine35a\"=dword:0x123\n"
                     "\"Wine35b\"=dword:123 456\n"
                     "\"Wine35c\"=dword:1234 5678\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine35a");
    todo_wine verify_reg_nonexist(hkey, "Wine35b");
    todo_wine verify_reg_nonexist(hkey, "Wine35c");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine36a\"=dword:1234;5678\n"
                     "\"Wine36b\"=dword:1234 ;5678\n"
                     "\"Wine36c\"=dword:1234#5678\n"
                     "\"Wine36d\"=dword:1234 #5678\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x1234;
    todo_wine verify_reg(hkey, "Wine36a", REG_DWORD, &dword, sizeof(dword), 0);
    todo_wine verify_reg(hkey, "Wine36b", REG_DWORD, &dword, sizeof(dword), 0);
    todo_wine verify_reg_nonexist(hkey, "Wine36c");
    todo_wine verify_reg_nonexist(hkey, "Wine36d");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine37a\"=\"foo\"bar\"\n"
                     "\"Wine37b\"=\"foo\"\"bar\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine37a");
    todo_wine verify_reg_nonexist(hkey, "Wine37b");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Empty string\"=\"\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Empty string", REG_SZ, "", 1, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Test38a\"=\n"
                     "\"Test38b\"=\\\"\n"
                     "\"Test38c\"=\\\"Value\\\"\n"
                     "\"Test38d\"=\\\"Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Test38a");
    todo_wine verify_reg_nonexist(hkey, "Test38b");
    todo_wine verify_reg_nonexist(hkey, "Test38c");
    todo_wine verify_reg_nonexist(hkey, "Test38d");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine39a\"=\"Value1\"  ;comment\n"
                     "\"Wine39b\"=\"Value2\"\t\t;comment\n"
                     "\"Wine39c\"=\"Value3\"  #comment\n"
                     "\"Wine39d\"=\"Value4\"\t\t#comment\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine39a", REG_SZ, "Value1", 7, 0);
    todo_wine verify_reg(hkey, "Wine39b", REG_SZ, "Value2", 7, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine39c");
    todo_wine verify_reg_nonexist(hkey, "Wine39d");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"TestNoBeginQuote\"=Asdffdsa\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "TestNoBeginQuote");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"TestNoEndQuote\"=\"Asdffdsa\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "TestNoEndQuote");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"TestNoQuotes\"=Asdffdsa\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "TestNoQuotes");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "NameNoBeginQuote\"=\"Asdffdsa\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "NameNoBeginQuote");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"NameNoEndQuote=\"Asdffdsa\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "NameNoEndQuote");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "NameNoQuotes=\"Asdffdsa\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "NameNoQuotes");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                 "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                 "\"MixedQuotes=Asdffdsa\"\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "MixedQuotes");
    todo_wine verify_reg_nonexist(hkey, "MixedQuotes=Asdffdsa");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine40a\"=hex(2):4c,00,69,00,6e,00,65,00,00,00\n"
                     "\"Wine40b\"=\"Value 1\"\n"
                     "\"Wine40c\"=hex(2):4c,00,69,00,6e,00,65,00\\\n"
                     "\"Wine40d\"=\"Value 2\"\n"
                     "\"Wine40e\"=hex(2):4c,00,69,00,6e,00,65,00,\\\n"
                     "\"Wine40f\"=\"Value 3\"\n"
                     "\"Wine40g\"=\"Value 4\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine40a", REG_EXPAND_SZ, "Line", 5, 0);
    todo_wine verify_reg(hkey, "Wine40b", REG_SZ, "Value 1", 8, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine40c");
    todo_wine verify_reg(hkey, "Wine40d", REG_SZ, "Value 2", 8, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine40e");
    todo_wine verify_reg_nonexist(hkey, "Wine40f");
    todo_wine verify_reg(hkey, "Wine40g", REG_SZ, "Value 4", 8, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line1\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,\\;comment\n"
                     "  63,00,61,00,74,00,\\;comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Multi-Line1", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line2\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,\\;comment\n"
                     "  63,00,61,00,74,00,;comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Multi-Line2", REG_MULTI_SZ, "Line concat", 12, 0);

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line3\"=hex(7):4c,69,6e,65,20\\\n"
                     ",63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Multi-Line3");

    test_import_wstr("\xef\xbb\xbfREGEDIT4\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line4\"=hex(7):4c,69,6e,65,20\\\n"
                     "  ,63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Multi-Line4");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line5\"=hex(7):4c,69,6e,65,20\\\n"
                     ",63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Multi-Line5");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line6\"=hex(7):4c,69,6e,65,20\\\n"
                     "  ,63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Multi-Line6");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line7\"=hex(7):4c,00,69,00,6e,00,\\;comment\n"
                     "  65,00,20,00,\\;comment\n"
                     "  63,00,6f,00,6e,00,\\;comment\n"
                     "  63,00,61,00,74,00,\\;comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Multi-Line7", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line8\"=hex(7):4c,00,69,00,6e,00,\\;#comment\n"
                     "  65,00,20,00,\\;#comment\n"
                     "  63,00,6f,00,6e,00,\\;#comment\n"
                     "  63,00,61,00,74,00,\\;#comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Multi-Line8", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line9\"=hex(7):4c,00,69,00,6e,00,\\;comment\n"
                     "  65,00,20,00,\\;comment\n"
                     "  63,00,6f,00,6e,00,\\;comment\n"
                     "  63,00,61,00,74,00,\\#comment\n"
                     "  65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Multi-Line9");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Multi-Line10\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "  63,00,6f,00,6e,00,\\;comment\n"
                     "  63,00,61,00,74,00,\\\n\n"
                     "  65,00,6e,00,\\;comment\n\n"
                     "  61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Multi-Line10", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine41a\"=dword:1234\\\n"
                     "5678\n"
                     "\"Wine41b\"=\"Test \\\n"
                     "Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine41a");
    todo_wine verify_reg_nonexist(hkey, "Wine41b");

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"double\\\"quote\"=\"valid \\\"or\\\" not\"\n"
                     "\"single'quote\"=dword:00000008\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "double\"quote", REG_SZ, "valid \"or\" not", 15, 0);
    dword = 0x00000008;
    todo_wine verify_reg(hkey, "single'quote", REG_DWORD, &dword, sizeof(dword), 0);

    /* Test key name and value name concatenation */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\\n"
                     "Subkey1]\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegOpenKeyExA(hkey, "Subkey1", 0, KEY_READ, &subkey);
    todo_wine ok(err == ERROR_FILE_NOT_FOUND, "got %d, expected 2\n", err);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\n"
                     "\\Subkey2]\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegOpenKeyExA(hkey, "Subkey2", 0, KEY_READ, &subkey);
    todo_wine ok(err == ERROR_FILE_NOT_FOUND, "got %d, expected 2\n", err);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine\\\n"
                     "42a\"=\"Value 1\"\n"
                     "\"Wine42b\"=\"Value 2\"\n"
                     "\"Wine\n"
                     "\\42c\"=\"Value 3\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg_nonexist(hkey, "Wine42a");
    todo_wine verify_reg(hkey, "Wine42b", REG_SZ, "Value 2", 8, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine42c");

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
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine43a", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    todo_wine verify_reg(hkey, "Wine43b", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine43c");
    todo_wine verify_reg_nonexist(hkey, "Wine43d");
    todo_wine verify_reg_nonexist(hkey, "Wine43e");
    todo_wine verify_reg_nonexist(hkey, "Wine43f");
    todo_wine verify_reg_nonexist(hkey, "Wine43g");
    todo_wine verify_reg(hkey, "Wine43h", REG_NONE, "V\0a\0l\0u\0e\0\0", 12, 0);
    todo_wine verify_reg(hkey, "Wine43i", REG_NONE, "V\0a\0l\0u", 8, 0);
    todo_wine verify_reg(hkey, "Wine43j", REG_NONE, "V\0a\0l\0u", 8, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine43k");

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
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine44a", REG_EXPAND_SZ, "%PATH%", 7, 0);
    todo_wine verify_reg(hkey, "Wine44b", REG_EXPAND_SZ, "%PATH%", 7, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine44c");
    todo_wine verify_reg_nonexist(hkey, "Wine44d");
    todo_wine verify_reg_nonexist(hkey, "Wine44e");
    todo_wine verify_reg_nonexist(hkey, "Wine44f");
    todo_wine verify_reg_nonexist(hkey, "Wine44g");
    todo_wine verify_reg(hkey, "Wine44h", REG_EXPAND_SZ, "%PATH%", 7, 0);
    /* Wine44i */
    size = sizeof(buffer);
    err = RegQueryValueExA(hkey, "Wine44i", NULL, &type, (BYTE *)&buffer, &size);
    todo_wine ok(err == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", err);
    todo_wine ok(type == REG_EXPAND_SZ, "got wrong type %u, expected %u\n", type, REG_EXPAND_SZ);
    todo_wine ok(size == 6 || broken(size == 5) /* WinXP */, "got wrong size %u, expected 6\n", size);
    todo_wine ok(memcmp(buffer, "%PATH", size) == 0, "got wrong data\n");
    /* Wine44j */
    size = sizeof(buffer);
    memset(buffer, '-', size);
    err = RegQueryValueExA(hkey, "Wine44j", NULL, &type, (BYTE *)&buffer, &size);
    todo_wine ok(err == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", err);
    todo_wine ok(type == REG_EXPAND_SZ, "got wrong type %u, expected %u\n", type, REG_EXPAND_SZ);
    todo_wine ok(size == 6 || broken(size == 5) /* WinXP */, "got wrong size %u, expected 6\n", size);
    todo_wine ok(memcmp(buffer, "%PATH", size) == 0, "got wrong data\n");
    /* Wine44k */
    todo_wine verify_reg_nonexist(hkey, "Wine44k");

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
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    hex[0] = 0x11; hex[1] = 0x22; hex[2] = 0x33; hex[3] = 0x44;
    hex[4] = 0x55; hex[5] = 0x66; hex[6] = 0x77; hex[7] = 0x88;
    todo_wine verify_reg(hkey, "Wine45a", REG_BINARY, hex, sizeof(hex), 0);
    todo_wine verify_reg(hkey, "Wine45b", REG_BINARY, hex, sizeof(hex), 0);
    todo_wine verify_reg_nonexist(hkey, "Wine45c");
    todo_wine verify_reg_nonexist(hkey, "Wine45d");
    todo_wine verify_reg_nonexist(hkey, "Wine45e");
    todo_wine verify_reg_nonexist(hkey, "Wine45f");
    todo_wine verify_reg_nonexist(hkey, "Wine45g");
    todo_wine verify_reg(hkey, "Wine45h", REG_BINARY, hex, sizeof(hex), 0);
    todo_wine verify_reg(hkey, "Wine45i", REG_BINARY, hex, 6, 0);
    todo_wine verify_reg(hkey, "Wine45j", REG_BINARY, hex, 6, 0);
    todo_wine verify_reg_nonexist(hkey, "Wine45k");

    /* Test import with subkeys */
    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey\"1]\n"
                     "\"Wine\\\\31\"=\"Test value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegOpenKeyExA(hkey, "Subkey\"1", 0, KEY_READ, &subkey);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);
    todo_wine verify_reg(subkey, "Wine\\31", REG_SZ, "Test value", 11, 0);
    err = RegCloseKey(subkey);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);
    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE "\\Subkey\"1");
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "\\Subkey/2]\n"
                     "\"123/\\\"4;'5\"=\"Random value name\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    err = RegOpenKeyExA(hkey, "Subkey/2", 0, KEY_READ, &subkey);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);
    todo_wine verify_reg(subkey, "123/\"4;'5", REG_SZ, "Random value name", 18, 0);
    err = RegCloseKey(subkey);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);
    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE "\\Subkey/2");
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    err = RegCloseKey(hkey);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);

    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    todo_wine ok(err == ERROR_SUCCESS, "got %d, expected 0\n", err);
}

static void test_import_with_whitespace(void)
{
    HKEY hkey;
    DWORD r, dword;
    LONG err;

    test_import_str("  REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);

    err = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    todo_wine ok(err == ERROR_SUCCESS, "RegOpenKeyExA failed: got %d, expected 0\n", err);

    test_import_str("  REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine1a\"=\"Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine1a", REG_SZ, "Value", 6, 0);

    test_import_str("\tREGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine1b\"=\"Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine1b", REG_SZ, "Value", 6, 0);

    test_import_str(" \t REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine1c\"=\"Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine1c", REG_SZ, "Value", 6, 0);

    test_import_str("REGEDIT4\n\n"
                    "  [HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine2a\"=\"Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine2a", REG_SZ, "Value", 6, 0);

    test_import_str("REGEDIT4\n\n"
                    "\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine2b\"=\"Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine2b", REG_SZ, "Value", 6, 0);

    test_import_str("REGEDIT4\n\n"
                    " \t [HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine2c\"=\"Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine2c", REG_SZ, "Value", 6, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "  \"Wine3a\"=\"Two leading spaces\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine3a", REG_SZ, "Two leading spaces", 19, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\t\"Wine3b\"=\"One leading tab\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine3b", REG_SZ, "One leading tab", 16, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    " \t \"Wine3c\"=\"Space, tab, space\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine3c", REG_SZ, "Space, tab, space", 18, 0);

    test_import_str("                    REGEDIT4\n\n"
                    "\t\t\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\t    \"Wine4a\"=\"Tab and four spaces\"\n"
                    "    \"Wine4b\"=dword:00112233\n"
                    "\t  \t  \t  \t  \t  \t  \"Wine4c\"=hex(7):4c,69,6e,65,20,\\\n"
                    "        63,6f,6e,\\;comment\n"
                    "\t\t\t\t63,61,74,\\;comment\n"
                    "  \t65,6e,61,74,69,6f,6e,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine4a", REG_SZ, "Tab and four spaces", 20, 0);
    dword = 0x112233;
    todo_wine verify_reg(hkey, "Wine4b", REG_DWORD, &dword, sizeof(dword), 0);
    todo_wine verify_reg(hkey, "Wine4c", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_str("    REGEDIT4\n\n"
                    "\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "  \"Wine5a\"=\"Leading spaces\"\n"
                    "\t\t\"Wine5b\"\t\t=\"Leading tabs\"\n"
                    "\t  \"Wine5c\"=\t  \"Tabs and spaces\"\n"
                    "    \"Wine5d\"    \t    =    \t    \"More whitespace\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine5a", REG_SZ, "Leading spaces", 15, 0);
    todo_wine verify_reg(hkey, "Wine5b", REG_SZ, "Leading tabs", 13, 0);
    todo_wine verify_reg(hkey, "Wine5c", REG_SZ, "Tabs and spaces", 16, 0);
    todo_wine verify_reg(hkey, "Wine5d", REG_SZ, "More whitespace", 16, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"  Wine6a\"=\"Leading spaces\"\n"
                    "\"\t\tWine6b\"=\"Leading tabs\"\n"
                    "  \"  Wine6c  \"  =  \"  Spaces everywhere  \"  \n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "  Wine6a", REG_SZ, "Leading spaces", 15, 0);
    todo_wine verify_reg(hkey, "\t\tWine6b", REG_SZ, "Leading tabs", 13, 0);
    todo_wine verify_reg(hkey, "  Wine6c  ", REG_SZ, "  Spaces everywhere  ", 22, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine7a\"=\"    Four spaces in the data\"\n"
                    "\"Wine7b\"=\"\t\tTwo tabs in the data\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine7a", REG_SZ, "    Four spaces in the data", 28, 0);
    todo_wine verify_reg(hkey, "Wine7b", REG_SZ, "\t\tTwo tabs in the data", 23, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine8a\"=\"Trailing spaces\"    \n"
                    "\"Wine8b\"=\"Trailing tabs and spaces\"\t  \t\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine8a", REG_SZ, "Trailing spaces", 16, 0);
    todo_wine verify_reg(hkey, "Wine8b", REG_SZ, "Trailing tabs and spaces", 25, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine9a\"=dword:  00000008\n"
                    "\"Wine9b\"=dword:\t\t00000008\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x00000008;
    todo_wine verify_reg(hkey, "Wine9a", REG_DWORD, &dword, sizeof(dword), 0);
    todo_wine verify_reg(hkey, "Wine9b", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "@  =  \"Test Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "", REG_SZ, "Test Value", 11, 0);

    test_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\t@\t=\tdword:\t00000008\t\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "", REG_DWORD, &dword, sizeof(DWORD), 0);

    err = RegCloseKey(hkey);
    todo_wine ok(err == ERROR_SUCCESS, "RegCloseKey failed: got %d, expected 0\n", err);

    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    todo_wine ok(err == ERROR_SUCCESS, "RegDeleteKeyA failed: got %d, expected 0\n", err);

    test_import_wstr("\xef\xbb\xbf  Windows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);

    err = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    todo_wine ok(err == ERROR_SUCCESS, "RegOpenKeyExA failed: got %d, expected 0\n", err);

    test_import_wstr("\xef\xbb\xbf  Windows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine1a\"=\"Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine1a", REG_SZ, "Value", 6, 0);

    test_import_wstr("\xef\xbb\xbf\tWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine1b\"=\"Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine1b", REG_SZ, "Value", 6, 0);

    test_import_wstr("\xef\xbb\xbf \t Windows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine1c\"=\"Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine1c", REG_SZ, "Value", 6, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "  [HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine2a\"=\"Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine2a", REG_SZ, "Value", 6, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine2b\"=\"Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine2b", REG_SZ, "Value", 6, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                    " \t [HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine2c\"=\"Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine2c", REG_SZ, "Value", 6, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "  \"Wine3a\"=\"Two leading spaces\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine3a", REG_SZ, "Two leading spaces", 19, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\t\"Wine3b\"=\"One leading tab\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine3b", REG_SZ, "One leading tab", 16, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     " \t \"Wine3c\"=\"Space, tab, space\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %u, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine3c", REG_SZ, "Space, tab, space", 18, 0);

    test_import_wstr("\xef\xbb\xbf                    Windows Registry Editor Version 5.00\n\n"
                     "\t\t\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\t    \"Wine4a\"=\"Tab and four spaces\"\n"
                     "    \"Wine4b\"=dword:00112233\n"
                     "\t  \t  \t  \t  \t  \t  \"Wine4c\"=hex(7):4c,00,69,00,6e,00,65,00,20,00,\\\n"
                     "        63,00,6f,00,6e,00,\\;comment\n"
                     "\t\t\t\t63,00,61,00,74,00,\\;comment\n"
                     "  \t65,00,6e,00,61,00,74,00,69,00,6f,00,6e,00,00,00,00,00\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine4a", REG_SZ, "Tab and four spaces", 20, 0);
    dword = 0x112233;
    todo_wine verify_reg(hkey, "Wine4b", REG_DWORD, &dword, sizeof(dword), 0);
    todo_wine verify_reg(hkey, "Wine4c", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    test_import_wstr("\xef\xbb\xbf    Windows Registry Editor Version 5.00\n\n"
                     "\t[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "  \"Wine5a\"=\"Leading spaces\"\n"
                     "\t\t\"Wine5b\"\t\t=\"Leading tabs\"\n"
                     "\t  \"Wine5c\"=\t  \"Tabs and spaces\"\n"
                     "    \"Wine5d\"    \t    =    \t    \"More whitespace\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine5a", REG_SZ, "Leading spaces", 15, 0);
    todo_wine verify_reg(hkey, "Wine5b", REG_SZ, "Leading tabs", 13, 0);
    todo_wine verify_reg(hkey, "Wine5c", REG_SZ, "Tabs and spaces", 16, 0);
    todo_wine verify_reg(hkey, "Wine5d", REG_SZ, "More whitespace", 16, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"  Wine6a\"=\"Leading spaces\"\n"
                     "\"\t\tWine6b\"=\"Leading tabs\"\n"
                     "  \"  Wine6c  \"  =  \"  Spaces everywhere  \"  \n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "  Wine6a", REG_SZ, "Leading spaces", 15, 0);
    todo_wine verify_reg(hkey, "\t\tWine6b", REG_SZ, "Leading tabs", 13, 0);
    todo_wine verify_reg(hkey, "  Wine6c  ", REG_SZ, "  Spaces everywhere  ", 22, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine7a\"=\"    Four spaces in the data\"\n"
                     "\"Wine7b\"=\"\t\tTwo tabs in the data\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine7a", REG_SZ, "    Four spaces in the data", 28, 0);
    todo_wine verify_reg(hkey, "Wine7b", REG_SZ, "\t\tTwo tabs in the data", 23, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine8a\"=\"Trailing spaces\"    \n"
                     "\"Wine8b\"=\"Trailing tabs and spaces\"\t  \t\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "Wine8a", REG_SZ, "Trailing spaces", 16, 0);
    todo_wine verify_reg(hkey, "Wine8b", REG_SZ, "Trailing tabs and spaces", 25, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\"Wine9a\"=dword:  00000008\n"
                     "\"Wine9b\"=dword:\t\t00000008\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    dword = 0x00000008;
    todo_wine verify_reg(hkey, "Wine9a", REG_DWORD, &dword, sizeof(dword), 0);
    todo_wine verify_reg(hkey, "Wine9b", REG_DWORD, &dword, sizeof(dword), 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "@  =  \"Test Value\"\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "", REG_SZ, "Test Value", 11, 0);

    test_import_wstr("\xef\xbb\xbfWindows Registry Editor Version 5.00\n\n"
                     "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                     "\t@\t=\tdword:\t00000008\t\n\n", &r);
    todo_wine ok(r == REG_EXIT_SUCCESS, "got exit code %d, expected 0\n", r);
    todo_wine verify_reg(hkey, "", REG_DWORD, &dword, sizeof(DWORD), 0);

    err = RegCloseKey(hkey);
    todo_wine ok(err == ERROR_SUCCESS, "RegCloseKey failed: got %d, expected 0\n", err);

    err = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    todo_wine ok(err == ERROR_SUCCESS, "RegDeleteKeyA failed: got %d, expected 0\n", err);
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
    test_import_with_whitespace();
}
