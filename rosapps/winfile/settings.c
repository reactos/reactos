/*
 *  ReactOS winfile
 *
 *  settings.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef _MSC_VER
#include "stdafx.h"
#else
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
    
#include "main.h"
#include "settings.h"


DWORD Confirmation;
DWORD ViewType;
TCHAR ViewTypeMaskStr[MAX_TYPE_MASK_LEN];
//LPCTSTR lpViewTypeMaskStr;


void LoadSettings(void)
{
    HKEY    hKey;
    TCHAR   szSubKey[] = _T("Software\\ReactWare\\FileManager");
/*
    int     i;
    DWORD   dwSize;

    // Window size & position settings
    TaskManagerSettings.Maximized = FALSE;
    TaskManagerSettings.Left = 0;
    TaskManagerSettings.Top = 0;
    TaskManagerSettings.Right = 0;
    TaskManagerSettings.Bottom = 0;

    // Tab settings
    TaskManagerSettings.ActiveTabPage = 0;

    // Options menu settings
    TaskManagerSettings.AlwaysOnTop = FALSE;
    TaskManagerSettings.MinimizeOnUse = TRUE;
    TaskManagerSettings.HideWhenMinimized = TRUE;
    TaskManagerSettings.Show16BitTasks = TRUE;

    // Update speed settings
    TaskManagerSettings.UpdateSpeed = 2;

    // Applications page settings
    TaskManagerSettings.View_LargeIcons = FALSE;
    TaskManagerSettings.View_SmallIcons = FALSE;
    TaskManagerSettings.View_Details = TRUE;

    // Processes page settings
    TaskManagerSettings.ShowProcessesFromAllUsers = FALSE; // Server-only?
    TaskManagerSettings.Column_ImageName = TRUE;
    TaskManagerSettings.Column_PID = TRUE;
    TaskManagerSettings.Column_CPUUsage = TRUE;
    TaskManagerSettings.Column_CPUTime = TRUE;
    TaskManagerSettings.Column_MemoryUsage = TRUE;
    TaskManagerSettings.Column_MemoryUsageDelta = FALSE;
    TaskManagerSettings.Column_PeakMemoryUsage = FALSE;
    TaskManagerSettings.Column_PageFaults = FALSE;
    TaskManagerSettings.Column_USERObjects = FALSE;
    TaskManagerSettings.Column_IOReads = FALSE;
    TaskManagerSettings.Column_IOReadBytes = FALSE;
    TaskManagerSettings.Column_SessionID = FALSE; // Server-only?
    TaskManagerSettings.Column_UserName = FALSE; // Server-only?
    TaskManagerSettings.Column_PageFaultsDelta = FALSE;
    TaskManagerSettings.Column_VirtualMemorySize = FALSE;
    TaskManagerSettings.Column_PagedPool = FALSE;
    TaskManagerSettings.Column_NonPagedPool = FALSE;
    TaskManagerSettings.Column_BasePriority = FALSE;
    TaskManagerSettings.Column_HandleCount = FALSE;
    TaskManagerSettings.Column_ThreadCount = FALSE;
    TaskManagerSettings.Column_GDIObjects = FALSE;
    TaskManagerSettings.Column_IOWrites = FALSE;
    TaskManagerSettings.Column_IOWriteBytes = FALSE;
    TaskManagerSettings.Column_IOOther = FALSE;
    TaskManagerSettings.Column_IOOtherBytes = FALSE;

    for (i = 0; i < 25; i++) {
        TaskManagerSettings.ColumnOrderArray[i] = i;
    }
    TaskManagerSettings.ColumnSizeArray[0] = 105;
    TaskManagerSettings.ColumnSizeArray[1] = 50;
    TaskManagerSettings.ColumnSizeArray[2] = 107;
    TaskManagerSettings.ColumnSizeArray[3] = 70;
    TaskManagerSettings.ColumnSizeArray[4] = 35;
    TaskManagerSettings.ColumnSizeArray[5] = 70;
    TaskManagerSettings.ColumnSizeArray[6] = 70;
    TaskManagerSettings.ColumnSizeArray[7] = 100;
    TaskManagerSettings.ColumnSizeArray[8] = 70;
    TaskManagerSettings.ColumnSizeArray[9] = 70;
    TaskManagerSettings.ColumnSizeArray[10] = 70;
    TaskManagerSettings.ColumnSizeArray[11] = 70;
    TaskManagerSettings.ColumnSizeArray[12] = 70;
    TaskManagerSettings.ColumnSizeArray[13] = 70;
    TaskManagerSettings.ColumnSizeArray[14] = 60;
    TaskManagerSettings.ColumnSizeArray[15] = 60;
    TaskManagerSettings.ColumnSizeArray[16] = 60;
    TaskManagerSettings.ColumnSizeArray[17] = 60;
    TaskManagerSettings.ColumnSizeArray[18] = 60;
    TaskManagerSettings.ColumnSizeArray[19] = 70;
    TaskManagerSettings.ColumnSizeArray[20] = 70;
    TaskManagerSettings.ColumnSizeArray[21] = 70;
    TaskManagerSettings.ColumnSizeArray[22] = 70;
    TaskManagerSettings.ColumnSizeArray[23] = 70;
    TaskManagerSettings.ColumnSizeArray[24] = 70;

    TaskManagerSettings.SortColumn = 1;
    TaskManagerSettings.SortAscending = TRUE;

    // Performance page settings
    TaskManagerSettings.CPUHistory_OneGraphPerCPU = TRUE;
    TaskManagerSettings.ShowKernelTimes = FALSE;
 */
    // Open the key
    if (RegOpenKeyEx(HKEY_CURRENT_USER, szSubKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return;
    // Read the settings
//    dwSize = sizeof(TASKMANAGER_SETTINGS);
//    RegQueryValueEx(hKey, "Preferences", NULL, NULL, (LPBYTE)&TaskManagerSettings, &dwSize);

    // Close the key
    RegCloseKey(hKey);
}

void SaveSettings(void)
{
    HKEY hKey;
    TCHAR szSubKey1[] = _T("Software");
    TCHAR szSubKey2[] = _T("Software\\ReactWare");
    TCHAR szSubKey3[] = _T("Software\\ReactWare\\FileManager");

    // Open (or create) the key
    hKey = NULL;
    RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey1, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    RegCloseKey(hKey);
    hKey = NULL;
    RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey2, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    RegCloseKey(hKey);
    hKey = NULL;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey3, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return;
    // Save the settings
//    RegSetValueEx(hKey, "Preferences", 0, REG_BINARY, (LPBYTE)&TaskManagerSettings, sizeof(TASKMANAGER_SETTINGS));
    // Close the key
    RegCloseKey(hKey);
}

