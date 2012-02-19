/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for desktop objects
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>
#include "helper.h"
#include <undocuser.h>

#define STATUS_DLL_INIT_FAILED                  (0xC0000142)
#define DESKTOP_ALL_ACCESS 0x01ff

struct test_info {
    WCHAR* ExpectedWinsta;
    WCHAR* ExpectedDesktp;
};

struct test_info TestResults[] = {{L"WinSta0",L"Default"},
                                  {L"WinSta0",L"Default"},
                                  {L"WinSta0",L"Default"},
                                  {NULL, NULL},
                                  {NULL, NULL},
                                  {NULL, NULL},
                                  {NULL, NULL},
                                  {L"WinSta0",L"Default"},
                                  {L"TestWinsta", L"TestDesktop"}};

void do_InitialDesktop_child(int i)
{
	HDESK hdesktop;
	HWINSTA hwinsta;
    WCHAR buffer[100];
    DWORD size;
    BOOL ret;

    if(TestResults[i].ExpectedWinsta == NULL)
    {
        trace("Process should have failed to initialize\n");
        return;
    }

    IsGUIThread(TRUE);

    hdesktop = GetThreadDesktop(GetCurrentThreadId());
	hwinsta = GetProcessWindowStation();

    ret = GetUserObjectInformationW( hwinsta, UOI_NAME, buffer, sizeof(buffer), &size );
    ok(wcscmp(buffer, TestResults[i].ExpectedWinsta) == 0, "Wrong winsta %S insted of %S\n", buffer, TestResults[i].ExpectedWinsta);

    ret = GetUserObjectInformationW( hdesktop, UOI_NAME, buffer, sizeof(buffer), &size );
    ok(wcscmp(buffer, TestResults[i].ExpectedDesktp) == 0, "Wrong desktop %S insted of %S\n", buffer, TestResults[i].ExpectedDesktp);
}

void test_CreateProcessWithDesktop(int i, char *argv0, char* Desktop, DWORD expectedExitCode)
{
    STARTUPINFOA startup;
    char path[MAX_PATH];
    BOOL ret;
    DWORD ExitCode;
    PROCESS_INFORMATION pi;

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpDesktop = Desktop;

    sprintf( path, "%s desktop %u", argv0, i );
    ret = CreateProcessA( NULL, path, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &pi );
    ok( ret, "%d: CreateProcess '%s' failed err %d.\n", i, path, (int)GetLastError() );
    WaitForSingleObject (pi.hProcess, INFINITE);
    ret = GetExitCodeProcess(pi.hProcess, &ExitCode);
    ok(ret > 0 , "%d: GetExitCodeProcess failed\n", i);

    /* the exit code varies from version to version */
    /* xp returns error 128 and 7 returns STATUS_DLL_INIT_FAILED */
    if(ExitCode == 128) ExitCode = STATUS_DLL_INIT_FAILED;

    ok(ExitCode == expectedExitCode, "%d: expected error 0x%x in child process got 0x%x\n", i, (int)expectedExitCode, (int)ExitCode);

    CloseHandle(pi.hProcess);
}

HWINSTA CreateInheritableWinsta(WCHAR* name, ACCESS_MASK dwDesiredAccess, BOOL inheritable)
{
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = inheritable;
	return CreateWindowStationW(name, 0, dwDesiredAccess, &sa );
}

HDESK CreateInheritableDesktop(WCHAR* name, ACCESS_MASK dwDesiredAccess, BOOL inheritable)
{
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = inheritable;
    return CreateDesktopW(name, NULL, NULL, 0, dwDesiredAccess, &sa );
}

void Test_InitialDesktop(char *argv0)
{
    HWINSTA hwinsta = NULL, hwinstaInitial;
	HDESK hdesktop = NULL, hdeskInitial;
    BOOL ret;

    hwinstaInitial = GetProcessWindowStation();
    hdeskInitial = GetThreadDesktop(GetCurrentThreadId());
    
    test_CreateProcessWithDesktop(0, argv0, NULL, 0);
    test_CreateProcessWithDesktop(1, argv0, "Default", 0);
    test_CreateProcessWithDesktop(2, argv0, "WinSta0\\Default", 0);
    test_CreateProcessWithDesktop(3, argv0, "Winlogon", STATUS_DLL_INIT_FAILED);
    test_CreateProcessWithDesktop(4, argv0, "WinSta0/Default", STATUS_DLL_INIT_FAILED);
    test_CreateProcessWithDesktop(5, argv0, "NonExistantDesktop", STATUS_DLL_INIT_FAILED);
    test_CreateProcessWithDesktop(6, argv0, "NonExistantWinsta\\NonExistantDesktop", STATUS_DLL_INIT_FAILED);

    hwinsta = CreateInheritableWinsta(L"TestWinsta", WINSTA_ALL_ACCESS, TRUE);
    ok(hwinsta!=NULL, "CreateWindowStation failed\n");
    ret = SetProcessWindowStation(hwinsta);
    ok(ret != 0, "SetProcessWindowStation failed\n");
    hdesktop = CreateInheritableDesktop(L"TestDesktop", DESKTOP_ALL_ACCESS, TRUE);
    ok(hdesktop!=NULL, "CreateDesktop failed\n");

    test_CreateProcessWithDesktop(7, argv0, NULL, 0);
    test_CreateProcessWithDesktop(8, argv0, "TestWinsta\\TestDesktop", 0);
    test_CreateProcessWithDesktop(8, argv0, "NonExistantWinsta\\NonExistantDesktop", 0);
}

START_TEST(desktop)
{
    char **test_argv;
    int argc = winetest_get_mainargs( &test_argv );

    /* this program tests some cases that a child application fails to initialize */
    /* to test this behaviour properly we have to disable error messages */
    //SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );

    if (argc >= 3)
    {
        unsigned int arg;
        /* Child process. */
        sscanf (test_argv[2], "%d", (unsigned int *) &arg);
        do_InitialDesktop_child( arg );
        return;
    }
	
    Test_InitialDesktop(test_argv[0]);
}
