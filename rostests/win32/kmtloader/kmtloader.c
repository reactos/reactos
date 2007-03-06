/*
 * Kernel-Mode Tests Loader (based on PnP Test Driver Loader by Filip Navara)
 *
 * Copyright 2004 Filip Navara <xnavara@volny.cz>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* INCLUDES *******************************************************************/

#include <windows.h>
#include <stdio.h>

/* PUBLIC FUNCTIONS ***********************************************************/

int main()
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    PWCHAR DriverName = L"KMTEST";
    WCHAR ServiceExe[MAX_PATH];

    printf("Kernel Mode Tests loader\n\n");
    GetCurrentDirectoryW(MAX_PATH, ServiceExe);
    wcscat(ServiceExe, L"\\kmtest.sys");

    printf("Opening SC Manager...\n");
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (schSCManager == NULL)
    {
        DWORD Err = GetLastError();
        printf("OpenSCManager failed with error 0x%lx\n", Err);
        return 0;
    }

    printf("Creating service...\n");
    schService = CreateServiceW(
        schSCManager,
        DriverName,
        DriverName,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        ServiceExe,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);

    if (schService == NULL)
    {
        printf("Opening service...\n");
        schService = OpenServiceW(schSCManager, DriverName, SERVICE_ALL_ACCESS);
    }

    if (schService == NULL)
    {
        DWORD Err = GetLastError();
        printf("Create/OpenService failed with error 0x%lx\n", Err);
        CloseServiceHandle(schSCManager);
        return 0;
    }

    //for (;;) ;

    printf("Starting service...\n");
    StartService(schService, 0, NULL);

    printf("Cleaning up and exiting\n");
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

    return 0;
}
