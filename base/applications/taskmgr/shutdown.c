/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     System Shutdown.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2011 Mário Kacmár /Mario Kacmar/ aka Kario <kario@szm.sk>
 *              Copyright 2014 Robert Naumann <gonzomdx@gmail.com>
 */

#include "precomp.h"
#include <ndk/exfuncs.h>
#include <ndk/pofuncs.h>
#include <ndk/rtlfuncs.h>

// Uncomment when NtInitiatePowerAction() is implemented
// #define NT_INITIATE_POWERACTION_IMPLEMENTED

static BOOL
EnablePrivilege(LPCWSTR lpszPrivilegeName, BOOL bEnablePrivilege)
{
    BOOL   Success;
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;

    Success = OpenProcessToken(GetCurrentProcess(),
                               TOKEN_ADJUST_PRIVILEGES,
                               &hToken);
    if (!Success) return Success;

    Success = LookupPrivilegeValueW(NULL,
                                    lpszPrivilegeName,
                                    &tp.Privileges[0].Luid);
    if (!Success) goto Quit;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = (bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0);

    Success = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);

Quit:
    CloseHandle(hToken);
    return Success;
}

VOID
ShutDown_StandBy(VOID)
{
    NTSTATUS Status;

    if (!EnablePrivilege(SE_SHUTDOWN_NAME, TRUE))
    {
        ShowWin32Error(GetLastError());
        return;
    }

#ifdef NT_INITIATE_POWERACTION_IMPLEMENTED
    Status = NtInitiatePowerAction(PowerActionSleep,
                                   PowerSystemSleeping1,
                                   0, FALSE);
#else
    Status = NtSetSystemPowerState(PowerActionSleep,
                                   PowerSystemSleeping1,
                                   0);
#endif

    if (!NT_SUCCESS(Status))
        ShowWin32Error(RtlNtStatusToDosError(Status));

    EnablePrivilege(SE_SHUTDOWN_NAME, FALSE);
}

VOID
ShutDown_Hibernate(VOID)
{
    NTSTATUS Status;

    if (!EnablePrivilege(SE_SHUTDOWN_NAME, TRUE))
    {
        ShowWin32Error(GetLastError());
        return;
    }

#ifdef NT_INITIATE_POWERACTION_IMPLEMENTED
    Status = NtInitiatePowerAction(PowerActionHibernate,
                                   PowerSystemHibernate,
                                   0, FALSE);
#else
    Status = NtSetSystemPowerState(PowerActionHibernate,
                                   PowerSystemHibernate,
                                   0);
#endif

    if (!NT_SUCCESS(Status))
        ShowWin32Error(RtlNtStatusToDosError(Status));

    EnablePrivilege(SE_SHUTDOWN_NAME, FALSE);
}

VOID
ShutDown_PowerOff(VOID)
{
    /* Trick: on Windows, pressing the CTRL key forces shutdown via NT API */
    BOOL ForceShutdown = !!(GetKeyState(VK_CONTROL) & 0x8000);

    if (!EnablePrivilege(SE_SHUTDOWN_NAME, TRUE))
    {
        ShowWin32Error(GetLastError());
        return;
    }

    if (ForceShutdown)
    {
        NTSTATUS Status = NtShutdownSystem(ShutdownPowerOff);
        if (!NT_SUCCESS(Status))
            ShowWin32Error(RtlNtStatusToDosError(Status));
    }
    else
    {
        // The choice of EWX_SHUTDOWN or EWX_POWEROFF may be done with NtPowerInformation
        if (!ExitWindowsEx(EWX_POWEROFF /* EWX_SHUTDOWN */, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER))
            ShowWin32Error(GetLastError());
    }

    EnablePrivilege(SE_SHUTDOWN_NAME, FALSE);
}

VOID
ShutDown_Reboot(VOID)
{
    /* Trick: on Windows, pressing the CTRL key forces reboot via NT API */
    BOOL ForceReboot = !!(GetKeyState(VK_CONTROL) & 0x8000);

    if (!EnablePrivilege(SE_SHUTDOWN_NAME, TRUE))
    {
        ShowWin32Error(GetLastError());
        return;
    }

    if (ForceReboot)
    {
        NTSTATUS Status = NtShutdownSystem(ShutdownReboot);
        if (!NT_SUCCESS(Status))
            ShowWin32Error(RtlNtStatusToDosError(Status));
    }
    else
    {
        if (!ExitWindowsEx(EWX_REBOOT, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER))
            ShowWin32Error(GetLastError());
    }

    EnablePrivilege(SE_SHUTDOWN_NAME, FALSE);
}

VOID
ShutDown_LogOffUser(VOID)
{
    if (!ExitWindowsEx(EWX_LOGOFF, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER))
        ShowWin32Error(GetLastError());
}

VOID
ShutDown_SwitchUser(VOID)
{
}

VOID
ShutDown_LockComputer(VOID)
{
    if (!LockWorkStation())
        ShowWin32Error(GetLastError());
}

VOID
ShutDown_Disconnect(VOID)
{
}

VOID
ShutDown_EjectComputer(VOID)
{
}
