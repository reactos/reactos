/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Helper function for shutting down the system
 * COPYRIGHT:   Copyright 2008-2009 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

/**
 * Shuts down the system.
 *
 * @return
 * true if everything went well, false if there was a problem while trying to shut down the system.
 */
bool ShutdownSystem()
{
    HANDLE hToken;
    TOKEN_PRIVILEGES Privileges;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        StringOut("OpenProcessToken failed\n");
        return false;
    }

    /* Get the LUID for the Shutdown privilege */
    if (!LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &Privileges.Privileges[0].Luid))
    {
        StringOut("LookupPrivilegeValue failed\n");
        return false;
    }

    /* Assign the Shutdown privilege to our process */
    Privileges.PrivilegeCount = 1;
    Privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &Privileges, 0, NULL, NULL))
    {
        StringOut("AdjustTokenPrivileges failed\n");
        return false;
    }

    /* Finally shut down the system */
    if(!ExitWindowsEx(EWX_POWEROFF, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED))
    {
        StringOut("ExitWindowsEx failed\n");
        return false;
    }

    return true;
}
