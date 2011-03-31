/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
/* Copyright (C) 2007, Dmitry Chapyshev <lentind@yandex.ru> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <io.h>
#include <tchar.h>

#include "resource.h"

#define BUFFER_SIZE 32767
#define B_TO_MB(bytes) ((bytes)/(1024*1024))
#define B_TO_KB(bytes) ((bytes)/1024)

/* Load from resource and convert to OEM */
static
BOOL
GetOemStrings(UINT rcID, LPSTR OutMsg)
{
    TCHAR Msg[BUFFER_SIZE];
    if (LoadString(GetModuleHandle(NULL), rcID, (LPTSTR)Msg, BUFFER_SIZE))
    {
        CharToOem(Msg, OutMsg);
        return TRUE;
    }
    return FALSE;
}

/* Load data from registry */
static
BOOL
GetRegistryValue(HKEY hKeyName, LPCTSTR SubKey, LPCTSTR ValueName, LPCTSTR Buf)
{
    DWORD CharCount = BUFFER_SIZE;
    HKEY hKey;
    LONG lRet;

    lRet = RegOpenKeyEx(hKeyName,
                        SubKey,
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);
    if (lRet != ERROR_SUCCESS) return FALSE;
    lRet = RegQueryValueEx(hKey,
                           ValueName,
                           NULL,
                           NULL,
                           (LPBYTE)Buf,
                           &CharCount);
    if (lRet != ERROR_SUCCESS) return FALSE;
    RegCloseKey(hKey);
    return TRUE;
}

/* Show usage */
static
VOID
Usage(VOID)
{
    TCHAR Buf[BUFFER_SIZE];
    if(GetOemStrings(IDS_USAGE, Buf)) printf("%s",Buf);
}

/* Print all system information */
VOID
AllSysInfo(VOID)
{
    DWORD dwCharCount = BUFFER_SIZE;
    OSVERSIONINFO VersionInfo;
    SYSTEM_INFO SysInfo;
    TCHAR Buf[BUFFER_SIZE], Tmp[BUFFER_SIZE], Msg[BUFFER_SIZE], szSystemDir[MAX_PATH];
    MEMORYSTATUS MemoryStatus;
    unsigned int cSeconds;
    TIME_ZONE_INFORMATION TimeZoneInfo;

    if (!GetSystemDirectory(szSystemDir, sizeof(szSystemDir)/sizeof(szSystemDir[0])))
        printf("Error getting: GetSystemDirectory\n");
    GetSystemInfo(&SysInfo);

    // getting computer name
    dwCharCount = BUFFER_SIZE;
    if (!GetComputerName(Buf, &dwCharCount))
        printf("Error getting: GetComputerName");
    if (GetOemStrings(IDS_HOST_NAME, Msg))
        printf(Msg, Buf);

    //getting OS Name
    GetRegistryValue(HKEY_LOCAL_MACHINE,
                     TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
                     TEXT("ProductName"),
                     Buf);
    if (GetOemStrings(IDS_OS_NAME, Msg)) printf(Msg, Buf);

    //getting OS Version
    ZeroMemory(&VersionInfo, sizeof(VersionInfo));
    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
    GetVersionEx(&VersionInfo);
    
    if (GetOemStrings(IDS_OS_VERSION, Msg))
        printf(Msg,
               (unsigned)VersionInfo.dwMajorVersion,
               (unsigned)VersionInfo.dwMinorVersion,
               (unsigned)VersionInfo.dwBuildNumber,
               VersionInfo.szCSDVersion,
               (unsigned)VersionInfo.dwBuildNumber);

    //getting OS Manufacturer

    //getting OS Configuration

    //getting OS Build Type
    if (GetRegistryValue(HKEY_LOCAL_MACHINE,
                         TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
                         TEXT("CurrentType"),
                         Buf))
        if (GetOemStrings(IDS_OS_BUILD_TYPE, Msg)) printf(Msg, Buf);

    //getting Registered Owner
    if (GetRegistryValue(HKEY_LOCAL_MACHINE,
                         TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
                         TEXT("RegisteredOwner"),
                         Buf))
        if (GetOemStrings(IDS_REG_OWNER, Msg)) printf(Msg, Buf);

    //getting Registered Organization
    if (GetRegistryValue(HKEY_LOCAL_MACHINE,
                         TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
                         TEXT("RegisteredOrganization"),
                         Buf))
        if (GetOemStrings(IDS_REG_ORG, Msg)) printf(Msg, Buf);

    //getting Product ID // use SOFTWARE\Microsoft\Windows NT\CurrentVersion\InstallDate
    if (GetRegistryValue(HKEY_LOCAL_MACHINE,
                         TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
                         TEXT("ProductId"),
                         Buf))
        if (GetOemStrings(IDS_PRODUCT_ID, Msg)) printf(Msg, Buf);

    //getting Install Date
    if (GetRegistryValue(HKEY_LOCAL_MACHINE,
                         TEXT("SOFTWARE\\Microsoft\\WBEM\\CIMOM"),
                         TEXT("SetupDate"),
                         Buf))
        if (GetOemStrings(IDS_INST_DATE, Msg)) printf(Msg, Buf);

    //getting Install Time
    if (GetRegistryValue(HKEY_LOCAL_MACHINE,
                         TEXT("SOFTWARE\\Microsoft\\WBEM\\CIMOM"),
                         TEXT("SetupTime"),
                         Buf))
        if (GetOemStrings(IDS_INST_TIME, Msg)) printf(Msg, Buf);

    //getting System Up Time
    cSeconds = GetTickCount() / 1000;
    if (GetOemStrings(IDS_UP_TIME, Msg))
        printf(Msg, cSeconds / (60*60*24), (cSeconds / (60*60)) % 24, (cSeconds / 60) % 60, cSeconds % 60);

    //getting System Manufacturer
    sprintf(Tmp, "%s\\oeminfo.ini", szSystemDir);
    GetPrivateProfileString(TEXT("General"),
                            TEXT("Manufacturer"),
                            TEXT("To Be Filled By O.E.M."),
                            Buf,
                            sizeof(Buf)/sizeof(Buf[0]),
                            Tmp);
    printf("System Manufacturer:\t\t%s\n", Buf);

    //getting System Model
    GetPrivateProfileString(TEXT("General"),
                            TEXT("Model"),
                            TEXT("To Be Filled By O.E.M."),
                            Buf,
                            sizeof(Buf)/sizeof(Buf[0]),
                            Tmp);
    printf("System Model:\t\t\t%s\n", Buf);

    //getting System type
    switch (SysInfo.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_UNKNOWN:
            if (GetOemStrings(IDS_SYS_TYPE_UNKNOWN, Msg)) printf("%s", Msg);
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            if (GetOemStrings(IDS_SYS_TYPE_X86, Msg)) printf("%s", Msg);
            break;
        case PROCESSOR_ARCHITECTURE_IA64:
            if (GetOemStrings(IDS_SYS_TYPE_IA64, Msg)) printf("%s", Msg);
            break;
        case PROCESSOR_ARCHITECTURE_AMD64:
            if (GetOemStrings(IDS_SYS_TYPE_AMD64, Msg)) printf("%s", Msg);
            break;
    }

    //getting Processor(s)
    if (GetOemStrings(IDS_PROCESSORS, Msg))
    {
        unsigned int i;
        printf(Msg, (unsigned int)SysInfo.dwNumberOfProcessors);
        for(i = 0; i < (unsigned int)SysInfo.dwNumberOfProcessors; i++)
        {
            sprintf(Tmp,"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%u",i);
            if (GetRegistryValue(HKEY_LOCAL_MACHINE,
                                 (LPCTSTR)Tmp,
                                 TEXT("Identifier"),
                                 Buf))
                printf("				[0%u]: %s", i+1, Buf);
            if (GetRegistryValue(HKEY_LOCAL_MACHINE,
                                 (LPCTSTR)Tmp,
                                 TEXT("VendorIdentifier"),
                                 Buf))
                printf(" %s\n", Buf);
        }
    }

    //getting BIOS Version
    if (GetRegistryValue(HKEY_LOCAL_MACHINE,
                         TEXT("HARDWARE\\DESCRIPTION\\System"),
                         TEXT("SystemBiosVersion"),
                         Buf))
        if (GetOemStrings(IDS_BIOS_VERSION, Msg)) printf(Msg, Buf);

    //gettings BIOS date
    if (GetRegistryValue(HKEY_LOCAL_MACHINE,
                         TEXT("HARDWARE\\DESCRIPTION\\System"),
                         TEXT("SystemBiosDate"),
                         Buf))
        if (GetOemStrings(IDS_BIOS_DATE, Msg)) printf(Msg, Buf);

    //getting ReactOS Directory
    if (!GetWindowsDirectory(Buf, BUFFER_SIZE)) printf("Error getting: GetWindowsDirectory");
    if (GetOemStrings(IDS_ROS_DIR, Msg)) printf(Msg, Buf);

    //getting System Directory
    if (GetOemStrings(IDS_SYS_DIR, Msg)) printf(Msg, szSystemDir);

    //getting Boot Device
    if (GetRegistryValue(HKEY_LOCAL_MACHINE,
                         TEXT("SYSTEM\\Setup"),
                         TEXT("SystemPartition"),
                         Buf))
        if (GetOemStrings(IDS_BOOT_DEV, Msg)) printf(Msg, Buf);

    //getting System Locale
    if (GetRegistryValue(HKEY_CURRENT_USER,
                         TEXT("Control Panel\\International"),
                         TEXT("Locale"),
                         Tmp))
        if (GetRegistryValue(HKEY_CLASSES_ROOT,
                             TEXT("MIME\\Database\\Rfc1766"),
                             (LPTSTR)Tmp,
                             Buf))
            if (GetOemStrings(IDS_SYS_LOCALE, Msg)) printf(Msg, Buf);

    //getting Input Locale
    if (GetRegistryValue(HKEY_CURRENT_USER,
                         TEXT("Keyboard Layout\\Preload"),
                         TEXT("1"),
                         Buf))
    {
        int i,j;
        for(j = 0, i = 4; i <= 8; j++, i++) Tmp[j] = Buf[i];
        if (GetRegistryValue(HKEY_CLASSES_ROOT,
                             TEXT("MIME\\Database\\Rfc1766"),
                             (LPTSTR)Tmp,
                             Buf))
            if (GetOemStrings(IDS_INPUT_LOCALE, Msg)) printf(Msg, Buf);
    }

    //getting Time Zone
    GetTimeZoneInformation(&TimeZoneInfo);
    sprintf(Tmp,"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\%S",TimeZoneInfo.StandardName);
    if (GetRegistryValue(HKEY_LOCAL_MACHINE,
                         (LPCTSTR)Tmp,
                         TEXT("Display"),
                         Buf))
        if (GetOemStrings(IDS_TIME_ZONE, Msg)) printf(Msg, Buf);

    //getting Total Physical Memory
    GlobalMemoryStatus(&MemoryStatus);
    if(GetOemStrings(IDS_TOTAL_PHYS_MEM, Msg))
        printf(Msg,
               B_TO_MB(MemoryStatus.dwTotalPhys),
               B_TO_KB(MemoryStatus.dwTotalPhys));

    //getting Available Physical Memory
    if(GetOemStrings(IDS_AVAIL_PHISICAL_MEM,Msg))
        printf(Msg,
               B_TO_MB(MemoryStatus.dwAvailPhys),
               B_TO_KB(MemoryStatus.dwAvailPhys));

    //getting Virtual Memory: Max Size
    if(GetOemStrings(IDS_VIRT_MEM_MAX, Msg))
        printf(Msg,
               B_TO_MB(MemoryStatus.dwTotalVirtual),
               B_TO_KB(MemoryStatus.dwTotalVirtual));

    //getting Virtual Memory: Available
    if(GetOemStrings(IDS_VIRT_MEM_AVAIL, Msg))
        printf(Msg,
               B_TO_MB(MemoryStatus.dwAvailVirtual),
               B_TO_KB(MemoryStatus.dwAvailVirtual));

    //getting Virtual Memory: In Use
    if(GetOemStrings(IDS_VIRT_MEM_INUSE, Msg))
        printf(Msg,
               B_TO_MB(MemoryStatus.dwTotalVirtual-MemoryStatus.dwAvailVirtual),
               B_TO_KB(MemoryStatus.dwTotalVirtual-MemoryStatus.dwAvailVirtual));

    //getting Page File Location(s)
    if (GetRegistryValue(HKEY_LOCAL_MACHINE,
                         TEXT("SYSTEM\\ControlSet001\\Control\\Session Manager\\Memory Management"),
                         TEXT("PagingFiles"),
                         Buf))
    {
        int i;
        for(i = 0; i < strlen((char*)Buf); i++)
        {
            if (Buf[i] == ' ')
            {
                Buf[i] = '\0';
                break;
            }
        }
        if(GetOemStrings(IDS_PAGEFILE_LOC, Msg)) printf(Msg, Buf);
    }

    //getting Domain
    if (GetRegistryValue(HKEY_LOCAL_MACHINE,
                         TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                         TEXT("CachePrimaryDomain"),
                         Buf))
        if(GetOemStrings(IDS_DOMINE, Msg)) printf(Msg, Buf);

    //getting Logon Server

    //getting NetWork Card(s)
    if(GetOemStrings(IDS_NETWORK_CARDS, Msg))
    {

    }
}

/* Main program */
int
main(int argc, char *argv[])
{
    if (argc > 1 && (!strcmp(argv[1], "/?") || !strcmp(argv[1], "-?")))
    {
        Usage();
        return 0;
    }
    AllSysInfo();
    return 0;
}
