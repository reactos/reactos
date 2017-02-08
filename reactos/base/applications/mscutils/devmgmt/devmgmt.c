/*
 * PROJECT:     ReactOS Device Managment
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/devmgmt/devmgmt.c
 * PURPOSE:     Bootstrap for the device manager
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>

#include <cfgmgr32.h>
#include <devmgr.h>

int WINAPI
wWinMain(HINSTANCE hThisInstance,
         HINSTANCE hPrevInstance,
         LPWSTR lpCmdLine,
         int nCmdShow)
{
    if (!DeviceManager_ExecuteW(NULL,
                                hThisInstance,
                                NULL,
                                nCmdShow))
    {
        return GetLastError();
    }

    return 0;
}
