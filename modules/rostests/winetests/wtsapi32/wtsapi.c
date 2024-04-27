/*
 * Copyright 2014 Stefan Leichter
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

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <stdarg.h>
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <wine/winternl.h>
#include <lmcons.h>
#include <wtsapi32.h>
#define PSAPI_VERSION 1
#include <psapi.h>

#include "wine/test.h"

static BOOL (WINAPI *pWTSEnumerateProcessesExW)(HANDLE server, DWORD *level, DWORD session, WCHAR **info, DWORD *count);
static BOOL (WINAPI *pWTSFreeMemoryExW)(WTS_TYPE_CLASS class, void *memory, ULONG count);

static const SYSTEM_PROCESS_INFORMATION *find_nt_process_info(const SYSTEM_PROCESS_INFORMATION *head, DWORD pid)
{
    for (;;)
    {
        if ((DWORD)(DWORD_PTR)head->UniqueProcessId == pid)
            return head;
        if (!head->NextEntryOffset)
            break;
        head = (SYSTEM_PROCESS_INFORMATION *)((char *)head + head->NextEntryOffset);
    }
    return NULL;
}

static void check_wts_process_info(const WTS_PROCESS_INFOW *info, DWORD count)
{
    ULONG nt_length = 1024;
    SYSTEM_PROCESS_INFORMATION *nt_info = malloc(nt_length);
    WCHAR process_name[MAX_PATH], *process_filepart;
    BOOL ret, found = FALSE;
    NTSTATUS status;
    DWORD i;

    GetModuleFileNameW(NULL, process_name, MAX_PATH);
    process_filepart = wcsrchr(process_name, '\\') + 1;

    while ((status = NtQuerySystemInformation(SystemProcessInformation, nt_info,
            nt_length, NULL)) == STATUS_INFO_LENGTH_MISMATCH)
    {
        nt_length *= 2;
        nt_info = realloc(nt_info, nt_length);
    }
#ifdef __REACTOS__
    ok(!status, "got %#x\n", status);
#else
    ok(!status, "got %#lx\n", status);
#endif
    for (i = 0; i < count; i++)
    {
        char sid_buffer[50];
        SID_AND_ATTRIBUTES *sid = (SID_AND_ATTRIBUTES *)sid_buffer;
        const SYSTEM_PROCESS_INFORMATION *nt_process;
        HANDLE process, token;
        DWORD size;

        nt_process = find_nt_process_info(nt_info, info[i].ProcessId);
#ifdef __REACTOS__
        ok(!!nt_process, "failed to find pid %#x\n", info[i].ProcessId);
        winetest_push_context("pid %#x", info[i].ProcessId);

        ok(info[i].SessionId == nt_process->SessionId, "expected session id %#x, got %#x\n",
                nt_process->SessionId, info[i].SessionId);

        ok(!memcmp(info[i].pProcessName, nt_process->ProcessName.Buffer, nt_process->ProcessName.Length),
                "expected process name %s, got %s\n",
                debugstr_w(nt_process->ProcessName.Buffer), debugstr_w(info[i].pProcessName));
#else
        ok(!!nt_process, "failed to find pid %#lx\n", info[i].ProcessId);
        winetest_push_context("pid %#lx", info[i].ProcessId);

        ok(info[i].SessionId == nt_process->SessionId, "expected session id %#lx, got %#lx\n",
                nt_process->SessionId, info[i].SessionId);

        ok(!memcmp(info[i].pProcessName, nt_process->ProcessName.Buffer, nt_process->ProcessName.Length),
                "expected process name %s, got %s\n",
                debugstr_w(nt_process->ProcessName.Buffer), debugstr_w(info[i].pProcessName));
#endif

        if ((process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, info[i].ProcessId)))
        {
            ret = OpenProcessToken(process, TOKEN_QUERY, &token);
#ifdef __REACTOS__
            ok(ret, "failed to open token, error %u\n", GetLastError());
#else
            ok(ret, "failed to open token, error %lu\n", GetLastError());
#endif
            ret = GetTokenInformation(token, TokenUser, sid_buffer, sizeof(sid_buffer), &size);
#ifdef __REACTOS__
            ok(ret, "failed to get token user, error %u\n", GetLastError());
#else
            ok(ret, "failed to get token user, error %lu\n", GetLastError());
#endif
#ifdef __REACTOS__
            if (info[i].pUserSid == NULL) {
                skip("pUserSid is NULL\n");
            } else {
                ok(EqualSid(info[i].pUserSid, sid->Sid), "SID did not match\n");
            }
#else
            ok(EqualSid(info[i].pUserSid, sid->Sid), "SID did not match\n");
#endif
            CloseHandle(token);
            CloseHandle(process);
        }

        winetest_pop_context();

        found = found || !wcscmp(info[i].pProcessName, process_filepart);
    }

    ok(found, "did not find current process\n");

    free(nt_info);
}

static void test_WTSEnumerateProcessesW(void)
{
    PWTS_PROCESS_INFOW info;
    DWORD count, level;
    BOOL ret;

    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 1, 1, &info, &count);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
#ifdef __REACTOS__
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());
#else
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %ld\n", GetLastError());
#endif
    WTSFreeMemory(info);

    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 0, &info, &count);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
#ifdef __REACTOS__
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());
#else
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %ld\n", GetLastError());
#endif
    WTSFreeMemory(info);

    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 2, &info, &count);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
#ifdef __REACTOS__
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());
#else
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %ld\n", GetLastError());
#endif
    WTSFreeMemory(info);

    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, NULL, &count);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
#ifdef __REACTOS__
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());
#else
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %ld\n", GetLastError());
#endif

    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &info, NULL);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
#ifdef __REACTOS__
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());
#else
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %ld\n", GetLastError());
#endif
    WTSFreeMemory(info);

    count = 0;
    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &info, &count);
    ok(ret, "expected success\n");
#ifdef __REACTOS__
    ok(!GetLastError(), "got error %u\n", GetLastError());
#else
    ok(!GetLastError(), "got error %lu\n", GetLastError());
#endif
    check_wts_process_info(info, count);
    WTSFreeMemory(info);

    if (!pWTSEnumerateProcessesExW)
    {
        skip("WTSEnumerateProcessesEx is not available\n");
        return;
    }

    level = 0;

    SetLastError(0xdeadbeef);
    count = 0xdeadbeef;
    ret = pWTSEnumerateProcessesExW(WTS_CURRENT_SERVER_HANDLE, &level, WTS_ANY_SESSION, NULL, &count);
    ok(!ret, "expected failure\n");
#ifdef __REACTOS__
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %u\n", GetLastError());
    ok(count == 0xdeadbeef, "got count %u\n", count);
#else
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError());
    ok(count == 0xdeadbeef, "got count %lu\n", count);
#endif

    info = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pWTSEnumerateProcessesExW(WTS_CURRENT_SERVER_HANDLE, &level, WTS_ANY_SESSION, (WCHAR **)&info, NULL);
    ok(!ret, "expected failure\n");
#ifdef __REACTOS__
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %u\n", GetLastError());
#else
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError());
#endif
    ok(info == (void *)0xdeadbeef, "got info %p\n", info);

    info = NULL;
    count = 0;
    SetLastError(0xdeadbeef);
    ret = pWTSEnumerateProcessesExW(WTS_CURRENT_SERVER_HANDLE, &level, WTS_ANY_SESSION, (WCHAR **)&info, &count);
    ok(ret, "expected success\n");
#ifdef __REACTOS__
    ok(!GetLastError(), "got error %u\n", GetLastError());
#else
    ok(!GetLastError(), "got error %lu\n", GetLastError());
#endif
    check_wts_process_info(info, count);
    pWTSFreeMemoryExW(WTSTypeProcessInfoLevel0, info, count);
}

static void test_WTSQuerySessionInformation(void)
{
    WCHAR *buf1, usernameW[UNLEN + 1], computernameW[MAX_COMPUTERNAME_LENGTH + 1];
    char *buf2, username[UNLEN + 1], computername[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD count, tempsize;
    USHORT *protocol;
    BOOL ret;

    SetLastError(0xdeadbeef);
    count = 0;
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, NULL, &count);
#ifdef __REACTOS__
    ok(!ret, "got %u\n", GetLastError());
    ok(count == 0, "got %u\n", count);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %u\n", GetLastError());
#else
    ok(!ret, "got %lu\n", GetLastError());
    ok(count == 0, "got %lu\n", count);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %lu\n", GetLastError());
#endif

    SetLastError(0xdeadbeef);
    count = 1;
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, NULL, &count);
#ifdef __REACTOS__
    ok(!ret, "got %u\n", GetLastError());
    ok(count == 1, "got %u\n", count);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %u\n", GetLastError());
#else
    ok(!ret, "got %lu\n", GetLastError());
    ok(count == 1, "got %lu\n", count);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %lu\n", GetLastError());
#endif

    SetLastError(0xdeadbeef);
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, &buf1, NULL);
#ifdef __REACTOS__
    ok(!ret, "got %u\n", GetLastError());
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %u\n", GetLastError());
#else
    ok(!ret, "got %lu\n", GetLastError());
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %lu\n", GetLastError());
#endif

    count = 0;
    buf1 = NULL;
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, &buf1, &count);
#ifdef __REACTOS__
    ok(ret, "got %u\n", GetLastError());
#else
    ok(ret, "got %lu\n", GetLastError());
#endif
    ok(buf1 != NULL, "buf not set\n");
#ifdef __REACTOS__
    ok(count == (lstrlenW(buf1) + 1) * sizeof(WCHAR), "expected %Iu, got %u\n", (lstrlenW(buf1) + 1) * sizeof(WCHAR), count);
#else
    ok(count == (lstrlenW(buf1) + 1) * sizeof(WCHAR), "expected %Iu, got %lu\n", (lstrlenW(buf1) + 1) * sizeof(WCHAR), count);
#endif
    tempsize = UNLEN + 1;
    GetUserNameW(usernameW, &tempsize);
    /* Windows Vista, 7 and 8 return uppercase username, while the rest return lowercase. */
    ok(!wcsicmp(buf1, usernameW), "expected %s, got %s\n", wine_dbgstr_w(usernameW), wine_dbgstr_w(buf1));
#ifdef __REACTOS__
    ok(count == tempsize * sizeof(WCHAR), "expected %Iu, got %u\n", tempsize * sizeof(WCHAR), count);
#else
    ok(count == tempsize * sizeof(WCHAR), "expected %Iu, got %lu\n", tempsize * sizeof(WCHAR), count);
#endif
    WTSFreeMemory(buf1);

    count = 0;
    buf1 = NULL;
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSDomainName, &buf1, &count);
#ifdef __REACTOS__
    ok(ret, "got %u\n", GetLastError());
#else
    ok(ret, "got %lu\n", GetLastError());
#endif
    ok(buf1 != NULL, "buf not set\n");
#ifdef __REACTOS__
    ok(count == (lstrlenW(buf1) + 1) * sizeof(WCHAR), "expected %Iu, got %u\n", (lstrlenW(buf1) + 1) * sizeof(WCHAR), count);
#else
    ok(count == (lstrlenW(buf1) + 1) * sizeof(WCHAR), "expected %Iu, got %lu\n", (lstrlenW(buf1) + 1) * sizeof(WCHAR), count);
#endif
    tempsize = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameW(computernameW, &tempsize);
    /* Windows Vista, 7 and 8 return uppercase computername, while the rest return lowercase. */
#ifdef __REACTOS__
    if (buf1 == NULL) {
        skip("WTSQuerySessionInformationW buffer is NULL\n");
    } else {
        ok(!wcsicmp(buf1, computernameW), "expected %s, got %s\n", wine_dbgstr_w(computernameW), wine_dbgstr_w(buf1));
    }
#else
    ok(!wcsicmp(buf1, computernameW), "expected %s, got %s\n", wine_dbgstr_w(computernameW), wine_dbgstr_w(buf1));
#endif
#ifdef __REACTOS__
    ok(count == (tempsize + 1) * sizeof(WCHAR), "expected %Iu, got %u\n", (tempsize + 1) * sizeof(WCHAR), count);
#else
    ok(count == (tempsize + 1) * sizeof(WCHAR), "expected %Iu, got %lu\n", (tempsize + 1) * sizeof(WCHAR), count);
#endif
    WTSFreeMemory(buf1);

    SetLastError(0xdeadbeef);
    count = 0;
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, NULL, &count);
#ifdef __REACTOS__
    ok(!ret, "got %u\n", GetLastError());
    ok(count == 0, "got %u\n", count);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %u\n", GetLastError());
#else
    ok(!ret, "got %lu\n", GetLastError());
    ok(count == 0, "got %lu\n", count);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %lu\n", GetLastError());
#endif

    SetLastError(0xdeadbeef);
    count = 1;
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, NULL, &count);
#ifdef __REACTOS__
    ok(!ret, "got %u\n", GetLastError());
    ok(count == 1, "got %u\n", count);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %u\n", GetLastError());
#else
    ok(!ret, "got %lu\n", GetLastError());
    ok(count == 1, "got %lu\n", count);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %lu\n", GetLastError());
#endif

    SetLastError(0xdeadbeef);
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, &buf2, NULL);
#ifdef __REACTOS__
    ok(!ret, "got %u\n", GetLastError());
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %u\n", GetLastError());
#else
    ok(!ret, "got %lu\n", GetLastError());
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %lu\n", GetLastError());
#endif

    count = 0;
    buf2 = NULL;
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, &buf2, &count);
#ifdef __REACTOS__
    ok(ret, "got %u\n", GetLastError());
#else
    ok(ret, "got %lu\n", GetLastError());
#endif
    ok(buf2 != NULL, "buf not set\n");
#ifdef __REACTOS__
    ok(count == lstrlenA(buf2) + 1, "expected %u, got %u\n", lstrlenA(buf2) + 1, count);
#else
    ok(count == lstrlenA(buf2) + 1, "expected %u, got %lu\n", lstrlenA(buf2) + 1, count);
#endif
    tempsize = UNLEN + 1;
    GetUserNameA(username, &tempsize);
    /* Windows Vista, 7 and 8 return uppercase username, while the rest return lowercase. */
#ifdef __REACTOS__
    if (buf2 == NULL) {
        skip("WTSQuerySessionInformationA bufffer is NULL\n");
    } else {
        ok(!stricmp(buf2, username), "expected %s, got %s\n", username, buf2);
    }
#else
    ok(!stricmp(buf2, username), "expected %s, got %s\n", username, buf2);
#endif
#ifdef __REACTOS__
    ok(count == tempsize, "expected %u, got %u\n", tempsize, count);
#else
    ok(count == tempsize, "expected %lu, got %lu\n", tempsize, count);
#endif
    WTSFreeMemory(buf2);

    count = 0;
    buf2 = NULL;
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSDomainName, &buf2, &count);
#ifdef __REACTOS__
    ok(ret, "got %u\n", GetLastError());
#else
    ok(ret, "got %lu\n", GetLastError());
#endif
    ok(buf2 != NULL, "buf not set\n");
#ifdef __REACTOS__
    ok(count == lstrlenA(buf2) + 1, "expected %u, got %u\n", lstrlenA(buf2) + 1, count);
#else
    ok(count == lstrlenA(buf2) + 1, "expected %u, got %lu\n", lstrlenA(buf2) + 1, count);
#endif
    tempsize = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameA(computername, &tempsize);
    /* Windows Vista, 7 and 8 return uppercase computername, while the rest return lowercase. */
    ok(!stricmp(buf2, computername), "expected %s, got %s\n", computername, buf2);
#ifdef __REACTOS__
    ok(count == tempsize + 1, "expected %u, got %u\n", tempsize + 1, count);
#else
    ok(count == tempsize + 1, "expected %lu, got %lu\n", tempsize + 1, count);
#endif
    WTSFreeMemory(buf2);

    count = 0;
    protocol = NULL;
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSClientProtocolType,
                                      (char **)&protocol, &count);
#ifdef __REACTOS__
    ok(ret, "got %u\n", GetLastError());
#else
    ok(ret, "got %lu\n", GetLastError());
#endif
    ok(protocol != NULL, "protocol not set\n");
#ifdef __REACTOS__
    ok(count == sizeof(*protocol), "got %u\n", count);
#else
    ok(count == sizeof(*protocol), "got %lu\n", count);
#endif
    WTSFreeMemory(protocol);
}

static void test_WTSQueryUserToken(void)
{
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = WTSQueryUserToken(WTS_CURRENT_SESSION, NULL);
    ok(!ret, "expected WTSQueryUserToken to fail\n");
#ifdef __REACTOS__
    ok(GetLastError()==ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());
#else
    ok(GetLastError()==ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %ld\n", GetLastError());
#endif
}

START_TEST (wtsapi)
{
    pWTSEnumerateProcessesExW = (void *)GetProcAddress(GetModuleHandleA("wtsapi32"), "WTSEnumerateProcessesExW");
    pWTSFreeMemoryExW = (void *)GetProcAddress(GetModuleHandleA("wtsapi32"), "WTSFreeMemoryExW");

    test_WTSEnumerateProcessesW();
    test_WTSQuerySessionInformation();
    test_WTSQueryUserToken();
}
