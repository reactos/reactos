/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for desktop objects
 * COPYRIGHT:   Copyright 2012-2017 Giannis Adamopoulos <gadamopoulos@reactos.org>
 *              Copyright 2016 Thomas Faber <thomas.faber@reactos.org>
 *              Copyright 2018-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"

#include <ndk/obfuncs.h>

typedef _Enum_is_bitflag_ enum _TEST_USER_CONTEXT
{
    TestNone = 0,
    TestForLocalUser,
    TestForSYSTEM
} TEST_USER_CONTEXT;
static struct test_info
{
    PCWSTR TestWinstaDesktop;
    PCWSTR ExpectedWinsta;
    PCWSTR ExpectedDesktp;
    NTSTATUS ExpectedStatus;
    union
    {
        UCHAR TestAsUser; // TEST_USER_CONTEXT value -- keep it first!
        struct
        {
            BOOLEAN ForLocalUser : 1;
            BOOLEAN ForSYSTEM : 1;
        };
    };
} TestList[] =
{
    /* Use the default (interactive) window station */
// 0
    {NULL,                                      L"WinSta0", L"Default", 0,          {TestForLocalUser | TestForSYSTEM}},
    // Even if we run on WinSta0, when on non-SYSTEM user the child is started on WinSta0,
    // while on SYSTEM user the child is started on a service-like window station.
    {L"Default",                                L"WinSta0", L"Default", 0,          {TestForLocalUser}},
    {L"Default",                                L"Service-0x0-3e7$", L"Default", 0, {TestForSYSTEM}},
    // The Winlogon desktop is available only for SYSTEM-user running programs.
    {L"WinSta0\\Winlogon",                      NULL, NULL, STATUS_DLL_INIT_FAILED, {TestForLocalUser}},
    {L"WinSta0\\Winlogon",                      L"WinSta0", L"Winlogon", 0,         {TestForSYSTEM}},
    {L"WinSta0\\",                              NULL, NULL, STATUS_DLL_INIT_FAILED, {TestForLocalUser | TestForSYSTEM}},
    {L"\\Default",                              NULL, NULL, STATUS_DLL_INIT_FAILED, {TestForLocalUser | TestForSYSTEM}},
    {L"WinSta0\\Default",                       L"WinSta0", L"Default", 0,          {TestForLocalUser | TestForSYSTEM}},
    // This test fails because, even if we run on WinSta0, the child is started on Service-0x0-3e7$ and there isn't any "Winlogon" desktop there.
    {L"Winlogon",                               NULL, NULL, STATUS_DLL_INIT_FAILED, {TestForLocalUser | TestForSYSTEM}},
    {L"WinSta0/Default",                        NULL, NULL, STATUS_DLL_INIT_FAILED, {TestForLocalUser | TestForSYSTEM}},
// 10
    {L"NonExistantDesktop",                     NULL, NULL, STATUS_DLL_INIT_FAILED, {TestForLocalUser | TestForSYSTEM}},
    {L"NonExistantWinsta\\NonExistantDesktop",  NULL, NULL, STATUS_DLL_INIT_FAILED, {TestForLocalUser | TestForSYSTEM}},

    /* Test on an (non-interactive) window station */
// 12
    {NULL,                                      L"WinSta0", L"Default", 0,          {TestForLocalUser | TestForSYSTEM}},
    {L"TestDesktop",                            NULL, NULL, STATUS_DLL_INIT_FAILED, {TestForLocalUser}},
    {L"TestDesktop",                            L"TestWinsta", L"TestDesktop", 0,   {TestForSYSTEM}},
    {L"TestWinsta\\",                           NULL, NULL, STATUS_DLL_INIT_FAILED, {TestForLocalUser | TestForSYSTEM}},
    {L"\\TestDesktop",                          NULL, NULL, STATUS_DLL_INIT_FAILED, {TestForLocalUser | TestForSYSTEM}},
    {L"TestWinsta\\TestDesktop",                L"TestWinsta", L"TestDesktop", 0,   {TestForLocalUser | TestForSYSTEM}},
    {L"NonExistantWinsta\\NonExistantDesktop",  NULL, NULL, STATUS_DLL_INIT_FAILED, {TestForLocalUser | TestForSYSTEM}},
    // This test is executed when the caller runs on WinSta0.
    {L"TestWinsta\\TestDesktop",                L"TestWinsta", L"TestDesktop", 0,   {TestForLocalUser | TestForSYSTEM}},

    /* Test on an non-interactive Service-0xXXXX-YYYY$ window station */
// 20
    {NULL,              L"WinSta0", L"Default", 0,              {TestForLocalUser | TestForSYSTEM}},
    {L"TestDesktop",    NULL, NULL, STATUS_DLL_INIT_FAILED,     {TestForLocalUser}},
    {L"TestDesktop",    L"Service-0x0-3e7$", L"TestDesktop", 0, {TestForSYSTEM}},
};

BOOL g_bRunningAsSYSTEM = FALSE;

// Taken from base/services/rpcss/setup.c
static BOOL
RunningAsSYSTEM(VOID)
{
    /* S-1-5-18 -- Local System */
    static SID SystemSid = { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_LOCAL_SYSTEM_RID } };

    BOOL bRet = FALSE;
    PTOKEN_USER pTokenUser;
    HANDLE hToken;
    DWORD cbTokenBuffer = 0;

    /* Get the process token */
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return FALSE;

    /* Retrieve token's information */
    if (GetTokenInformation(hToken, TokenUser, NULL, 0, &cbTokenBuffer) ||
        GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        goto Quit;
    }

    pTokenUser = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbTokenBuffer);
    if (!pTokenUser)
        goto Quit;

    if (GetTokenInformation(hToken, TokenUser, pTokenUser, cbTokenBuffer, &cbTokenBuffer))
    {
        /* Compare with SYSTEM SID */
        bRet = EqualSid(pTokenUser->User.Sid, &SystemSid);
    }

    HeapFree(GetProcessHeap(), 0, pTokenUser);

Quit:
    CloseHandle(hToken);
    return bRet;
}

void do_InitialDesktop_child(int i, BOOL WaitBeforeQuit)
{
    HDESK hdesktop;
    HWINSTA hwinsta;
    DWORD size;
    BOOL ret;
    WCHAR buffer[100];

    winetest_push_context("%d", i);

    if (TestList[i].ExpectedWinsta == NULL)
        ok(FALSE, "Process should have failed to initialize\n");

    /* Convert the thread to a GUI thread */
    IsGUIThread(TRUE);

    hwinsta = GetProcessWindowStation();
    hdesktop = GetThreadDesktop(GetCurrentThreadId());

    *buffer = UNICODE_NULL;
    ret = GetUserObjectInformationW(hwinsta, UOI_NAME, buffer, sizeof(buffer), &size);
    ok(ret != FALSE, "GetUserObjectInformationW(winsta 0x%p, UOI_NAME) failed, err %lu\n", hwinsta, GetLastError());
    if (TestList[i].ExpectedWinsta)
    {
        ok(wcscmp(buffer, TestList[i].ExpectedWinsta) == 0, "Wrong winsta %S instead of %S\n",
           buffer, TestList[i].ExpectedWinsta);
    }
    trace("We run on winstation %S\n", buffer);

    *buffer = UNICODE_NULL;
    ret = GetUserObjectInformationW(hdesktop, UOI_NAME, buffer, sizeof(buffer), &size);
    ok(ret != FALSE, "GetUserObjectInformationW(desktop 0x%p, UOI_NAME) failed, err %lu\n", hdesktop, GetLastError());
    if (TestList[i].ExpectedDesktp)
    {
        ok(wcscmp(buffer, TestList[i].ExpectedDesktp) == 0, "Wrong desktop %S instead of %S\n",
           buffer, TestList[i].ExpectedDesktp);
    }
    trace("We run on desktop %S\n", buffer);

    if (WaitBeforeQuit)
    {
        /* Open the notification event used for telling the caller what to do */
        HANDLE hEvent;
        _swprintf(buffer, L"desktop_test_%u", i);
        hEvent = OpenEventW(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, buffer);
        if (hEvent)
        {
            /* Set the event to tell the caller it can proceed with testing us */
            SetEvent(hEvent);
            YieldProcessor();

            /* Now wait for the caller to tell us we can terminate */
            WaitForSingleObject(hEvent, INFINITE /*10*1000*/); // Don't wait forever, but just 10 seconds max.
            CloseHandle(hEvent);
        }
    }

    winetest_pop_context();
}

static HWINSTA open_winsta_ex(PCWSTR winstaName, ACCESS_MASK dwDesiredAccess, BOOL bInherit, DWORD *error)
{
    HWINSTA hwinsta;
    SetLastError(0xfeedf00d);
    hwinsta = OpenWindowStationW(winstaName, bInherit, dwDesiredAccess);
    *error = GetLastError();
    return hwinsta;
}

static HWINSTA open_winsta(PCWSTR winstaName, DWORD *error)
{
    return open_winsta_ex(winstaName, WINSTA_ALL_ACCESS, FALSE, error);
}

static HWINSTA create_winsta_ex(PCWSTR winstaName, ACCESS_MASK dwDesiredAccess, BOOL bInherit, DWORD *error)
{
    HWINSTA hwinsta;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = bInherit;
    SetLastError(0xfeedf00d);
    hwinsta = CreateWindowStationW(winstaName, 0, dwDesiredAccess, &sa);
    *error = GetLastError();
    return hwinsta;
}

static HWINSTA create_winsta(PCWSTR winstaName, DWORD *error)
{
    HWINSTA hwinsta;
    SetLastError(0xfeedf00d);
    hwinsta = CreateWindowStationW(winstaName, 0, WINSTA_ALL_ACCESS, NULL);
    *error = GetLastError();
    return hwinsta;
}

static HDESK open_desk_ex(PCWSTR deskName, ACCESS_MASK dwDesiredAccess, BOOL bInherit, DWORD *error)
{
    HDESK hdesk;
    SetLastError(0xfeedf00d);
    hdesk = OpenDesktopW(deskName, 0, bInherit, dwDesiredAccess);
    *error = GetLastError();
    return hdesk;
}

static HDESK open_desk(PCWSTR deskName, DWORD *error)
{
    return open_desk_ex(deskName, DESKTOP_ALL_ACCESS, FALSE, error);
}

static HDESK create_desk_ex(PCWSTR deskName, ACCESS_MASK dwDesiredAccess, BOOL bInherit, DWORD *error)
{
    HDESK hdesk;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = bInherit;
    SetLastError(0xfeedf00d);
    hdesk = CreateDesktopW(deskName, NULL, NULL, 0, dwDesiredAccess, &sa);
    *error = GetLastError();
    return hdesk;
}

static HDESK create_desk(PCWSTR deskName, DWORD *error)
{
    HDESK hdesk;
    SetLastError(0xfeedf00d);
    hdesk = CreateDesktopW(deskName, NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL);
    *error = GetLastError();
    return hdesk;
}

void test_CreateProcessOnDesktop(int i, const char *argv0)
{
    PCWSTR Desktop = TestList[i].TestWinstaDesktop;
    DWORD expectedExitCode = TestList[i].ExpectedStatus; // (TestList[i].ExpectedWinsta ? 0 : STATUS_DLL_INIT_FAILED);
    HANDLE hHandles[2];
    STARTUPINFOW startup;
    PROCESS_INFORMATION pi;
    BOOL ret;
    DWORD ExitCode;
    WCHAR path[MAX_PATH];

    if (( g_bRunningAsSYSTEM && !TestList[i].ForSYSTEM) ||
        (!g_bRunningAsSYSTEM && !TestList[i].ForLocalUser))
    {
        if (TestList[i].ForSYSTEM || TestList[i].ForLocalUser)
        {
            skip("Test %d skipped because we are running as %s, but the test can only be run as %s\n",
                 i, (g_bRunningAsSYSTEM ? "SYSTEM" : "local user"),
                    (TestList[i].ForSYSTEM ? "SYSTEM" : "local user"));
        }
        else
        {
            skip("Test %d skipped because we are running as %s, but the test is disabled\n",
                 i, (g_bRunningAsSYSTEM ? "SYSTEM" : "local user"));
        }
        return;
    }

    winetest_push_context("%d", i);

    /* Create the auto-reset notification event for the child process to wait for us */
    _swprintf(path, L"desktop_test_%u", i);
    hHandles[0] = CreateEventW(NULL, FALSE, FALSE, path);
    ok(hHandles[0] != NULL, "CreateEvent '%S' failed, err %lu\n", path, GetLastError());

    /* Start the child process on the specified desktop */
    ZeroMemory(&startup, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpDesktop = (PWSTR)Desktop;

    _swprintf(path, L"%S desktop %u %u", argv0, i, TRUE);
    ret = CreateProcessW(NULL, path, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &pi);
    ok(ret, "CreateProcess '%S' failed, err %lu\n", path, GetLastError());

    /* Wait for the process' thread to have startup until it becomes
     * a GUI thread, then check what its desktop is. Don't wait forever,
     * but just 10 seconds max. */
    hHandles[1] = pi.hProcess;
    if (WaitForMultipleObjects(_countof(hHandles), hHandles, FALSE, 10*1000) == WAIT_OBJECT_0)
    {
        HWINSTA hCurrentWinSta, hwinstaTry = NULL;
        HDESK hCurrentDesk, hdesk, hdeskTry;
        PCWSTR WinSta = NULL;
        DWORD WinStaLen = 0;
        BOOL bSameWinSta, bExpectSuccess;
        DWORD size, error;
        WCHAR currentDeskName[100];
        WCHAR buffer[100];

        /*
         * Exercise GetThreadDesktop() where the thread does NOT belong to the current process.
         *
         * When GetThreadDesktop() is invoked for a GUI thread that belongs to the current process,
         * the call is always successful because the thread is already attached to a desktop, whose
         * handle exists in the process' handles table with the same handle information (attributes
         * and accesses).
         *
         * When GetThreadDesktop() is invoked for a thread that does not belong to the calling
         * process, but to a different one instead, the previous conditions are not systematically
         * satisfied anymore. If the calling process does not have an opened handle to that desktop,
         * or, the handle(s) it has opened to the desktop do no exactly match the handle information
         * the desktop is opened with in the target process, then GetThreadDesktop() will fail.
         */

        /* Extract the window station name from the Desktop string,
         * then, skip the window station specification, if any, e.g.:
         *   "WinSta0\\Default" --> "Default"
         * since OpenDesktop() only accepts a desktop name without a window station
         * (it is expected the desktop belongs to the current process' window station). */
        if (Desktop)
        {
            PCWSTR ptr = wcsrchr(Desktop, L'\\');
            if (ptr)
            {
                WinSta = Desktop;
                WinStaLen = (ptr - Desktop);
                Desktop = ++ptr;
            }
        }

        /* If we don't have a window station name from the Desktop string,
         * use the expected one */
        if (!WinSta)
        {
            WinSta = TestList[i].ExpectedWinsta;
            WinStaLen = (WinSta ? wcslen(WinSta) : 0);
        }

        /* Retrieve the current process window station handle and name */
        hCurrentWinSta = GetProcessWindowStation();
        ok(hCurrentWinSta != NULL, "GetProcessWindowStation failed, err %lu\n", GetLastError());
        *buffer = UNICODE_NULL;
        ret = GetUserObjectInformationW(hCurrentWinSta, UOI_NAME, buffer, sizeof(buffer), &size);
        ok(ret != FALSE, "GetUserObjectInformationW(desktop 0x%p, UOI_NAME) failed, err %lu\n", hCurrentWinSta, GetLastError());

        /* Check whether the target process uses the same window station as ours */
        size = (size - sizeof(UNICODE_NULL)) / sizeof(WCHAR);
        bSameWinSta = /*!WinSta ||*/ (ret && size == WinStaLen && wcsncmp(buffer, WinSta, size) == 0);

        /* Retrieve the current thread desktop handle and name */
        hCurrentDesk = GetThreadDesktop(GetCurrentThreadId());
        ok(hCurrentDesk != NULL, "GetThreadDesktop failed, err %lu\n", GetLastError());
        *currentDeskName = UNICODE_NULL;
        ret = GetUserObjectInformationW(hCurrentDesk, UOI_NAME, currentDeskName, sizeof(currentDeskName), &size);
        ok(ret != FALSE, "GetUserObjectInformationW(desktop 0x%p, UOI_NAME) failed, err %lu\n", hCurrentDesk, GetLastError());

        /* When invoking GetThreadDesktop() on the target process' thread,
         * we expect success only if this thread desktop is the same as ours
         * *AND* is opened with the very same attributes as ours. Since we
         * cannot retrieve the actual handle attributes of that target desktop
         * (in a simple way, at least), we assume it's opened with the
         * MAXIMUM_ALLOWED access, as it is commonly the case. */
        //
        /* We may expect success if our window station is the same as the
         * target process' one (and it'll be definitively the same if no
         * window station name was explicitly given) */
        // bExpectSuccess = (!bSameWinSta && !Desktop) || ( bSameWinSta && (!Desktop || (ret && wcscmp(currentDeskName, Desktop) == 0)) );
        bExpectSuccess = !Desktop || (bSameWinSta && ret && wcscmp(currentDeskName, Desktop) == 0);

        /* If Desktop == NULL, make it point now to the current desktop name buffer */
        if (!Desktop)
            Desktop = currentDeskName;

        // TODO: If different, expect first try to fail; then try opening the desktop with different info and see if it fails. Then try opening again with same information and see if it succeeds.

        hdesk = GetThreadDesktop(pi.dwThreadId);
        if (bExpectSuccess)
        {
            ok(hdesk != NULL, "GetThreadDesktop(%u) expected success, but failed, err %lu\n", pi.dwThreadId, GetLastError());
            ok(hdesk == hCurrentDesk, "Expected same desk handle (0x%x) but is different (0x%x)\n", hCurrentDesk, hdesk);
        }
        else
        {
            ok(hdesk == NULL, "GetThreadDesktop(%u) expected failure, but succeeded\n", pi.dwThreadId);
        }

        if (hdesk)
        {
            *buffer = UNICODE_NULL;
            ret = GetUserObjectInformationW(hdesk, UOI_NAME, buffer, sizeof(buffer), &size);
            ok(ret != FALSE, "GetUserObjectInformationW(desktop 0x%p, UOI_NAME) failed, err %lu\n", hdesk, GetLastError());
            ok(wcscmp(buffer, Desktop) == 0, "Wrong desktop %S instead of %S\n", buffer, Desktop);
            trace("Child process %u runs on desktop %S\n", pi.dwProcessId, buffer);
        }

        /* If we don't run on the same window station as the target process,
         * open its window station, switch to it, then proceed */
        if (!bSameWinSta)
        {
            CopyMemory(buffer, WinSta, WinStaLen * sizeof(WCHAR));
            buffer[WinStaLen] = UNICODE_NULL;
            /* Use MAXIMUM_ALLOWED to exactly match what the OS uses
             * (e.g. WINSTA_ALL_ACCESS wouldn't work for GetThreadDesktop) */
            hwinstaTry = open_winsta_ex(buffer, MAXIMUM_ALLOWED, FALSE, &error);
            ok(hwinstaTry != NULL, "open_winsta_ex(%S) failed, err 0x%lx\n", buffer, error);

            if (hwinstaTry)
            {
                ret = SetProcessWindowStation(hwinstaTry);
                ok(ret != FALSE, "SetProcessWindowStation failed, err %lu\n", GetLastError());
            }
        }

        /* Use MAXIMUM_ALLOWED to exactly match what the OS uses
         * (e.g. DESKTOP_ALL_ACCESS wouldn't work for GetThreadDesktop) */
        hdeskTry = open_desk_ex(Desktop, MAXIMUM_ALLOWED, FALSE, &error);
        ok(hdeskTry != NULL, "open_desk_ex(%S) failed, err 0x%lx\n", Desktop, error);

        /* The call should now succeed */
        hdesk = GetThreadDesktop(pi.dwThreadId);
        ok(hdesk != NULL, "GetThreadDesktop(%u) failed, err %lu\n", pi.dwThreadId, GetLastError());
        ok(hdesk == hdeskTry ||  /* WinXP/2003 */
           hdesk == hCurrentDesk /* Vista/7+ */,
           "Expected same desk handle (0x%x or 0x%x) but is different (0x%x)\n", hdeskTry, hCurrentDesk, hdesk);

        *buffer = UNICODE_NULL;
        ret = GetUserObjectInformationW(hdesk, UOI_NAME, buffer, sizeof(buffer), &size);
        ok(ret != FALSE, "GetUserObjectInformationW(desktop 0x%p, UOI_NAME) failed, err %lu\n", hdesk, GetLastError());
        ok(wcscmp(buffer, Desktop) == 0, "Wrong desktop %S instead of %S\n", buffer, Desktop);
        trace("Child process %u runs on desktop %S\n", pi.dwProcessId, buffer);

        if (hdeskTry)
            CloseDesktop(hdeskTry);

        /* Now restore our original window station and close the one we opened */
        if (/*!bSameWinSta &&*/ hwinstaTry)
        {
            ret = SetProcessWindowStation(hCurrentWinSta);
            ok(ret != FALSE, "SetProcessWindowStation failed, err %lu\n", GetLastError());
            CloseWindowStation(hwinstaTry);
        }

        /* Reset Desktop pointer if needed */
        if (Desktop == currentDeskName)
            Desktop = NULL;
    }
    /* Set the event again to tell the process' thread to terminate */
    SetEvent(hHandles[0]);

    /* Wait for the process to terminate */
    WaitForSingleObject(pi.hProcess, INFINITE);
    ret = GetExitCodeProcess(pi.hProcess, &ExitCode);
    ok(ret > 0, "GetExitCodeProcess failed\n");

    /* The exit code varies from version to version. XP returns error 128
     * (ERROR_WAIT_NO_CHILDREN) and 7 returns STATUS_DLL_INIT_FAILED. */
    if (ExitCode == 128) ExitCode = STATUS_DLL_INIT_FAILED;

    ok(ExitCode == expectedExitCode, "expected error 0x%lx in child process, got 0x%lx\n", expectedExitCode, ExitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hHandles[0]);

    winetest_pop_context();
}

void Test_InitialDesktop(const char *argv0)
{
    HWINSTA hwinsta = NULL, hwinstaInitial;
    HDESK hdesktop = NULL;
    int i;
    BOOL ret;
    DWORD error;

    trace("** Running as %s **\n\n", (g_bRunningAsSYSTEM ? "SYSTEM" : "local user"));

    hwinstaInitial = GetProcessWindowStation();

    /* Use the default (interactive) window station */
    for (i = 0; i <= 11; ++i)
    {
        test_CreateProcessOnDesktop(i, argv0);
    }

    /* Test on an (non-interactive) window station */
    hwinsta = create_winsta_ex(L"TestWinsta", WINSTA_ALL_ACCESS, TRUE, &error);
    ok(hwinsta != NULL && error == NO_ERROR, "CreateWindowStation failed, got 0x%p, err 0x%lx\n", hwinsta, error);
    ret = SetProcessWindowStation(hwinsta);
    ok(ret != FALSE, "SetProcessWindowStation failed, err %lu\n", GetLastError());
    hdesktop = create_desk_ex(L"TestDesktop", DESKTOP_ALL_ACCESS, TRUE, &error);
    ok(hdesktop != NULL && error == 0xfeedf00d, "CreateDesktop failed, got 0x%p, err 0x%lx\n", hdesktop, error);

    for (i = 12; i <= 18; ++i)
    {
        test_CreateProcessOnDesktop(i, argv0);
    }

    ret = SetProcessWindowStation(hwinstaInitial);
    ok(ret != FALSE, "SetProcessWindowStation failed, err %lu\n", GetLastError());

    test_CreateProcessOnDesktop(19, argv0);

    ret = CloseDesktop(hdesktop);
    ok(ret != FALSE, "CloseDesktop failed, err %lu\n", GetLastError());

    ret = CloseWindowStation(hwinsta);
    ok(ret != FALSE, "CloseWindowStation failed, err %lu\n", GetLastError());

    /* Test on an non-interactive Service-0xXXXX-YYYY$ window station */
    /*
     * NOTE: When running as SYSTEM, the call would fail if we were to use
     * WINSTA_ALL_ACCESS, due to access rights mismatch: the non-interactive
     * "Service-0x0-3e7$" window station already exists and wasn't created
     * by the system with the WINSTA_ALL_ACCESS, but with the MAXIMUM_ALLOWED
     * access rights instead.
     */
    hwinsta = create_winsta_ex(NULL, MAXIMUM_ALLOWED, TRUE, &error);
    ok(hwinsta != NULL && error == NO_ERROR, "CreateWindowStation failed, got 0x%p, err 0x%lx\n", hwinsta, error);
    ret = SetProcessWindowStation(hwinsta);
    ok(ret != FALSE, "SetProcessWindowStation failed, err %lu\n", GetLastError());
    hdesktop = create_desk_ex(L"TestDesktop", DESKTOP_ALL_ACCESS, TRUE, &error);
    ok(hdesktop != NULL && error == 0xfeedf00d, "CreateDesktop failed, got 0x%p, err 0x%lx\n", hdesktop, error);

    for (i = 20; i <= 22; ++i)
    {
        test_CreateProcessOnDesktop(i, argv0);
    }

    ret = SetProcessWindowStation(hwinstaInitial);
    ok(ret != FALSE, "SetProcessWindowStation failed, err %lu\n", GetLastError());

    ret = CloseDesktop(hdesktop);
    ok(ret != FALSE, "CloseDesktop failed, err %lu\n", GetLastError());

    ret = CloseWindowStation(hwinsta);
    ok(ret != FALSE, "CloseWindowStation failed, err %lu\n", GetLastError());
}

void Test_OpenInputDesktop(void)
{
    HDESK hDeskInput ,hDeskInput2;
    HDESK hDeskInitial;
    HWINSTA hwinsta = NULL, hwinstaInitial;
    BOOL ret;
    DWORD err;

    hDeskInput = OpenInputDesktop(0, FALSE, DESKTOP_ALL_ACCESS);
    ok(hDeskInput != NULL, "OpenInputDesktop failed, err %lu\n", GetLastError());
    hDeskInitial = GetThreadDesktop(GetCurrentThreadId());
    ok(hDeskInitial != NULL, "GetThreadDesktop failed, err %lu\n", GetLastError());
    ok(hDeskInput != hDeskInitial, "OpenInputDesktop returned thread desktop\n");

    hDeskInput2 = OpenInputDesktop(0, FALSE, DESKTOP_ALL_ACCESS);
    ok(hDeskInput2 != NULL, "Second call to OpenInputDesktop failed, err %lu\n", GetLastError());
    ok(hDeskInput2 != hDeskInput, "Second call to OpenInputDesktop returned same handle\n");

    ok(CloseDesktop(hDeskInput2) != FALSE, "CloseDesktop failed, err %lu\n", GetLastError());

    ret = SetThreadDesktop(hDeskInput);
    ok(ret != FALSE, "SetThreadDesktop for input desktop failed, err %lu\n", GetLastError());

    ret = SetThreadDesktop(hDeskInitial);
    ok(ret != FALSE, "SetThreadDesktop for initial desktop failed, err %lu\n", GetLastError());

    ok(CloseDesktop(hDeskInput) != FALSE, "CloseDesktop failed, err %lu\n", GetLastError());

    /* Try calling OpenInputDesktop after switching to a new winsta */
    hwinstaInitial = GetProcessWindowStation();
    ok(hwinstaInitial != NULL, "GetProcessWindowStation failed, err %lu\n", GetLastError());

    hwinsta = CreateWindowStationW(L"TestWinsta", 0, WINSTA_ALL_ACCESS, NULL);
    ok(hwinsta != NULL, "CreateWindowStationW failed, err %lu\n", GetLastError());

    ret = SetProcessWindowStation(hwinsta);
    ok(ret != FALSE, "SetProcessWindowStation failed, err %lu\n", GetLastError());

    hDeskInput = OpenInputDesktop(0, FALSE, DESKTOP_ALL_ACCESS);
    ok(hDeskInput == NULL, "OpenInputDesktop should fail\n");
    err = GetLastError();
    ok(err == ERROR_INVALID_FUNCTION, "Got last error: %lu\n", err);

    ret = SetProcessWindowStation(hwinstaInitial);
    ok(ret != FALSE, "SetProcessWindowStation failed, err %lu\n", GetLastError());

    ret = CloseWindowStation(hwinsta);
    ok(ret != FALSE, "CloseWindowStation failed, err %lu\n", GetLastError());
}

static void Test_References(void)
{
    PCWSTR winstaName = L"RefTestWinsta";
    PCWSTR deskName = L"RefTestDesktop";
    HWINSTA hwinsta, hwinsta2;
    HWINSTA hwinstaProcess;
    HDESK hdesk, hdesk1;
    DWORD error;
    NTSTATUS status;
    OBJECT_BASIC_INFORMATION objectInfo = { 0 };
    BOOL ret;
    ULONG baseRefs;

#define check_ref(handle, hdlcnt, ptrcnt) \
    RtlFillMemory(&objectInfo, sizeof(objectInfo), 0xAB); \
    status = NtQueryObject(handle, ObjectBasicInformation, &objectInfo, sizeof(objectInfo), NULL);  \
    ok(status == STATUS_SUCCESS, "status = 0x%lx\n", status);                                       \
    ok(objectInfo.HandleCount == (hdlcnt), "HandleCount = %lu, expected %lu\n", objectInfo.HandleCount, (ULONG)(hdlcnt));  \
    ok(objectInfo.PointerCount == (ptrcnt), "PointerCount = %lu, expected %lu\n", objectInfo.PointerCount, (ULONG)(ptrcnt));

    /* Winsta shouldn't exist */
    hwinsta = open_winsta(winstaName, &error);
    ok(hwinsta == NULL && error == ERROR_FILE_NOT_FOUND, "Got 0x%p, 0x%lx\n", hwinsta, error);

    /* Create it -- we get 1/4 instead of 1/3 because Winstas are kept in a list */
    hwinsta = create_winsta(winstaName, &error);
    ok(hwinsta != NULL && error == NO_ERROR, "Got 0x%p, 0x%lx\n", hwinsta, error);
    check_ref(hwinsta, 1, 4);
    baseRefs = objectInfo.PointerCount;
    ok(baseRefs == 4, "Window station initially has %lu references, expected 4\n", baseRefs);
    check_ref(hwinsta, 1, baseRefs);

    /* Open a second handle */
    hwinsta2 = open_winsta(winstaName, &error);
    ok(hwinsta2 != NULL && error == 0xfeedf00d, "Got 0x%p, err 0x%lx\n", hwinsta, error);
    check_ref(hwinsta2, 2, baseRefs + 1); // hwinsta2 refers to the same object as hwinsta
    check_ref(hwinsta, 2, baseRefs + 1);

    /* Close second handle -- back to 1/4 */
    ret = CloseHandle(hwinsta2);
    ok(ret != FALSE, "CloseHandle failed, err %lu\n", GetLastError());
    check_ref(hwinsta, 1, baseRefs);

    /* Same game but using CloseWindowStation */
    hwinsta2 = open_winsta(winstaName, &error);
    ok(hwinsta2 != NULL && error == 0xfeedf00d, "Got 0x%p, err 0x%lx\n", hwinsta, error);
    check_ref(hwinsta2, 2, baseRefs + 1); // hwinsta2 refers to the same object as hwinsta
    check_ref(hwinsta, 2, baseRefs + 1);
    ret = CloseWindowStation(hwinsta2);
    ok(ret != FALSE, "CloseWindowStation failed, err %lu\n", GetLastError());
    check_ref(hwinsta, 1, baseRefs);

    /* Set it as the process Winsta */
    hwinstaProcess = GetProcessWindowStation();
    SetProcessWindowStation(hwinsta);
    check_ref(hwinsta, 2, baseRefs + 2);

    /* Create a desktop. It takes a reference */
    hdesk = create_desk(deskName, &error);
    ok(hdesk != NULL && error == 0xfeedf00d, "Got 0x%p, err 0x%lx\n", hdesk, error);
    check_ref(hwinsta, 2, baseRefs + 3);
    check_ref(hdesk, 1, 8); // 8 == baseRefs + 4 ??

    /* CloseHandle fails, must use CloseDesktop */
    ret = CloseHandle(hdesk);
    error = GetLastError();
    if (!ret)
        trace("CloseHandle failed, err %lu -- Must use CloseDesktop!\n", error);
    ok(ret == FALSE, "CloseHandle expected failure, but succeeded\n");
    check_ref(hwinsta, 2, baseRefs + 3);
    ret = CloseDesktop(hdesk);
    ok(ret != FALSE, "CloseDesktop failed, err %lu\n", GetLastError());
    /* NOTE: On Windows, desktops aren't immediately destroyed, but are queued
     * and asynchronously destroyed by the desktop thread. Therefore, there
     * always exist a transient where the desktop being closed/destroyed holds
     * extra references to its former window station. These references are
     * removed once the desktop is completely destroyed.
     * In order to make the test more reliable, wait a little bit for the
     * desktop to be fully destroyed and release all references held. */
    Sleep(50);
    check_ref(hwinsta, 2, baseRefs + 2);

    /* Desktop no longer exists */
    hdesk = open_desk(deskName, &error);
    ok(hdesk == NULL && error == ERROR_FILE_NOT_FOUND, "Got 0x%p, err 0x%lx\n", hdesk, error);
    check_ref(hwinsta, 2, baseRefs + 2);

    /* Restore the original process Winsta */
    SetProcessWindowStation(hwinstaProcess);
    check_ref(hwinsta, 1, baseRefs);

    /* Close our last handle */
    ret = CloseHandle(hwinsta);
    ok(ret != FALSE, "CloseHandle failed, err %lu\n", GetLastError());

    /* Winsta no longer exists */
    hwinsta = open_winsta(winstaName, &error);
    ok(hwinsta == NULL && error == ERROR_FILE_NOT_FOUND, "Got 0x%p, err 0x%lx\n", hwinsta, error);

    /* Create the Winsta again, and close it while there's still a desktop */
    hwinsta = create_winsta(winstaName, &error);
    ok(hwinsta != NULL && error == NO_ERROR, "Got 0x%p, err 0x%lx\n", hwinsta, error);
    check_ref(hwinsta, 1, baseRefs);
    hwinstaProcess = GetProcessWindowStation();
    SetProcessWindowStation(hwinsta);
    check_ref(hwinsta, 2, baseRefs + 2);

    hdesk = create_desk(deskName, &error);
    ok(hdesk != NULL && error == 0xfeedf00d, "Got 0x%p, err 0x%lx\n", hdesk, error);
    check_ref(hwinsta, 2, baseRefs + 3);
    check_ref(hdesk, 1, 8); // 8 == baseRefs + 4 ??

    /* The reference from the desktop is still there, hence 1/5 */
    SetProcessWindowStation(hwinstaProcess);
    check_ref(hwinsta, 1, baseRefs + 1);
    ret = CloseHandle(hwinsta);
    ok(ret != FALSE, "CloseHandle failed, err %lu\n", GetLastError());
    hwinsta = open_winsta(winstaName, &error);
    ok(hwinsta == NULL && error == ERROR_FILE_NOT_FOUND, "Got 0x%p, err 0x%lx\n", hwinsta, error);

    /* Test references by SetThreadDesktop */
    hdesk1 = GetThreadDesktop(GetCurrentThreadId());
    ok(hdesk1 != hdesk, "Expected the new desktop not to be the thread desktop\n");

    check_ref(hdesk, 1, 8);
    baseRefs = objectInfo.PointerCount;
    ok(baseRefs == 8, "Desktop initially has %lu references, expected 8\n", baseRefs);
    check_ref(hdesk, 1, baseRefs);

    SetThreadDesktop(hdesk);
    check_ref(hdesk, 1, baseRefs + 1);
    ok(GetThreadDesktop(GetCurrentThreadId()) == hdesk, "Expected GetThreadDesktop to return hdesk\n");

    SetThreadDesktop(hdesk1);
    check_ref(hdesk, 1, baseRefs);
    ok(GetThreadDesktop(GetCurrentThreadId()) == hdesk1, "Expected GetThreadDesktop to return hdesk1\n");
}

START_TEST(desktop)
{
    char **test_argv;
    int argc = winetest_get_mainargs(&test_argv);

    /* This program tests some cases where a child application fails to initialize.
     * To test this behaviour properly we have to disable error messages. */
    //SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

    /* Check whether the test is running under the SYSTEM account */
    g_bRunningAsSYSTEM = RunningAsSYSTEM();

    if (argc >= 3)
    {
        /* Child process */
        int i, wait = 0;
        sscanf(test_argv[2], "%d", (int*)&i);
        if (argc >= 4)
            sscanf(test_argv[3], "%d", (int*)&wait);
        do_InitialDesktop_child(i, !!wait);
        return;
    }

    Test_InitialDesktop(test_argv[0]);
    Test_OpenInputDesktop();
    Test_References();
}
