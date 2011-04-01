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
/* Copyright (C) 2011, Rafal Harabien <rafalh1992@o2.pl> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <tchar.h>
#include <time.h>

#include "resource.h"

#define BUFFER_SIZE 32767

/* Load from resource and convert to OEM */
static
BOOL
GetOemStrings(UINT rcID, LPTSTR OutMsg)
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
RegGetSZ(HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpValueName, LPTSTR Buf)
{
    DWORD dwBytes = BUFFER_SIZE*sizeof(TCHAR), dwType;
    BOOL bRet = TRUE;
    
    /* If SubKy is specified open it */
    if (lpSubKey && RegOpenKeyEx(hKey,
                                 lpSubKey,
                                 0,
                                 KEY_QUERY_VALUE,
                                 &hKey) != ERROR_SUCCESS)
    {
        //_tprintf("Warning! Cannot open %s. Last error: %lu.\n", lpSubKey, GetLastError());
        return FALSE;
    }

    if (RegQueryValueEx(hKey,
                        lpValueName,
                        NULL,
                        &dwType,
                        (LPBYTE)Buf,
                        &dwBytes) != ERROR_SUCCESS || (dwType != REG_SZ && dwType != REG_MULTI_SZ))
    {
        //_tprintf("Warning! Cannot query %s. Last error: %lu, type: %lu.\n", lpValueName, GetLastError(), dwType);
        dwBytes = 0;
        bRet = FALSE;
    }
    
    /* Close key if we opened it */
    if (lpSubKey)
        RegCloseKey(hKey);
    
    /* NULL-terminate string */
    Buf[min(BUFFER_SIZE-1, dwBytes/sizeof(TCHAR))] = TEXT('\0');
    
    return bRet;
}

static
BOOL
RegGetDWORD(HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpValueName, LPDWORD lpData)
{
    DWORD dwBytes = sizeof(*lpData), dwType;
    BOOL bRet = TRUE;

    /* If SubKy is specified open it */
    if (lpSubKey && RegOpenKeyEx(hKey,
                                 lpSubKey,
                                 0,
                                 KEY_QUERY_VALUE,
                                 &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    if (RegQueryValueEx(hKey,
                        lpValueName,
                        NULL,
                        &dwType,
                        (LPBYTE)lpData,
                        &dwBytes) != ERROR_SUCCESS || dwType != REG_DWORD)
    {
        //_tprintf("Warning! Cannot query %s. Last err: %lu, type: %lu\n", lpValueName, GetLastError(), dwType);
        *lpData = 0;
        bRet = FALSE;
    }
    
    /* Close key if we opened it */
    if (lpSubKey)
        RegCloseKey(hKey);
    
    return bRet;
}

static
void
FormatBytes(LPTSTR Buf, unsigned cBytes)
{
    TCHAR szMB[32], Tmp[BUFFER_SIZE];
    NUMBERFMT fmt;
    
    itoa(cBytes / (1024*1024), szMB, 10);
    
    fmt.NumDigits = 0;
    fmt.LeadingZero = 0;
    fmt.Grouping = 3;
    fmt.lpDecimalSep = TEXT("");
    fmt.lpThousandSep = TEXT(" ");
    fmt.NegativeOrder = 0;
    if(!GetNumberFormat(LOCALE_SYSTEM_DEFAULT, 0, szMB, &fmt, Tmp, BUFFER_SIZE))
        printf("Error! GetNumberFormat failed.\n");
    
    CharToOem(Tmp, Buf);
}

static
void
FormatDateTime(time_t Time, LPTSTR lpBuf)
{
    unsigned cchBuf = BUFFER_SIZE, i;
    SYSTEMTIME SysTime;
    const struct tm *lpTm;
    
    lpTm = localtime(&Time);
    SysTime.wYear = (WORD)(1900 + lpTm->tm_year);
    SysTime.wMonth = (WORD)(1 + lpTm->tm_mon);
    SysTime.wDayOfWeek = (WORD)lpTm->tm_wday;
    SysTime.wDay = (WORD)lpTm->tm_mday;
    SysTime.wHour = (WORD)lpTm->tm_hour;
    SysTime.wMinute = (WORD)lpTm->tm_min;
    SysTime.wSecond = (WORD)lpTm->tm_sec;
    SysTime.wMilliseconds = 0;
    
    i = GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &SysTime, NULL, lpBuf, cchBuf);
    if (i)
        --i; /* don't count NULL character */
    i += _stprintf(lpBuf + i, TEXT(", "));
    GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &SysTime, NULL, lpBuf + i, cchBuf - i);
}

/* Show usage */
static
VOID
Usage(VOID)
{
    TCHAR Buf[BUFFER_SIZE];
    
    if(GetOemStrings(IDS_USAGE, Buf))
        _tprintf("%s", Buf);
}

/* Print all system information */
VOID
AllSysInfo(VOID)
{
    DWORD dwCharCount = BUFFER_SIZE, dwTimestamp;
    OSVERSIONINFO VersionInfo;
    SYSTEM_INFO SysInfo;
    TCHAR Buf[BUFFER_SIZE], Tmp[BUFFER_SIZE], Msg[BUFFER_SIZE], szSystemDir[MAX_PATH];
    const TCHAR *lpcszSysType;
    MEMORYSTATUS MemoryStatus;
    unsigned int cSeconds;
    TIME_ZONE_INFORMATION TimeZoneInfo;
    HKEY hKey;

    if (!GetSystemDirectory(szSystemDir, sizeof(szSystemDir)/sizeof(szSystemDir[0])))
    {
        _tprintf("Error! GetSystemDirectory failed.\n");
        return;
    }

    GetSystemInfo(&SysInfo);

    // getting computer name
    dwCharCount = BUFFER_SIZE;
    if (!GetComputerName(Buf, &dwCharCount))
        _tprintf("Error! GetComputerName failed.\n");
    else if (GetOemStrings(IDS_HOST_NAME, Msg))
        _tprintf(Msg, Buf);

    // open CurrentVersion key
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
                    0,
                    KEY_QUERY_VALUE,
                    &hKey) != ERROR_SUCCESS)
    {
        _tprintf("Error! RegOpenKeyEx failed.\n");
        return;
    }

    //getting OS Name
    RegGetSZ(hKey, NULL, TEXT("ProductName"), Buf);
    if (GetOemStrings(IDS_OS_NAME, Msg))
        _tprintf(Msg, Buf);

    //getting OS Version
    ZeroMemory(&VersionInfo, sizeof(VersionInfo));
    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
    GetVersionEx(&VersionInfo);

    if (GetOemStrings(IDS_OS_VERSION, Msg))
        _tprintf(Msg,
               (unsigned)VersionInfo.dwMajorVersion,
               (unsigned)VersionInfo.dwMinorVersion,
               (unsigned)VersionInfo.dwBuildNumber,
               VersionInfo.szCSDVersion,
               (unsigned)VersionInfo.dwBuildNumber);

    //getting OS Manufacturer

    //getting OS Configuration

    //getting OS Build Type
    RegGetSZ(hKey, NULL, TEXT("CurrentType"), Buf);
    if (GetOemStrings(IDS_OS_BUILD_TYPE, Msg))
        _tprintf(Msg, Buf);

    //getting Registered Owner
    RegGetSZ(hKey, NULL, TEXT("RegisteredOwner"), Buf);
    if (GetOemStrings(IDS_REG_OWNER, Msg))
        _tprintf(Msg, Buf);

    //getting Registered Organization
    RegGetSZ(hKey, NULL, TEXT("RegisteredOrganization"), Buf);
    if (GetOemStrings(IDS_REG_ORG, Msg))
        _tprintf(Msg, Buf);

    //getting Product ID
    RegGetSZ(hKey, NULL, TEXT("ProductId"), Buf);
    if (GetOemStrings(IDS_PRODUCT_ID, Msg))
        _tprintf(Msg, Buf);

    //getting Install Date
    RegGetDWORD(hKey, NULL, TEXT("InstallDate"), &dwTimestamp);
    FormatDateTime((time_t)dwTimestamp, Buf);
    if (GetOemStrings(IDS_INST_DATE, Msg))
        _tprintf(Msg, Buf);

    // close Current Version key now
    RegCloseKey(hKey);

    //getting System Up Time
    cSeconds = GetTickCount() / 1000;
    if (GetOemStrings(IDS_UP_TIME, Msg))
        _tprintf(Msg, cSeconds / (60*60*24), (cSeconds / (60*60)) % 24, (cSeconds / 60) % 60, cSeconds % 60);

    //getting System Manufacturer; HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\OEMInformation\Manufacturer for Win >= 6.0
    sprintf(Tmp, "%s\\oeminfo.ini", szSystemDir);
    GetPrivateProfileString(TEXT("General"),
                            TEXT("Manufacturer"),
                            TEXT("To Be Filled By O.E.M."),
                            Buf,
                            sizeof(Buf)/sizeof(Buf[0]),
                            Tmp);
    if (GetOemStrings(IDS_SYS_MANUFACTURER, Msg))
        _tprintf(Msg, Buf);

    //getting System Model; HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\OEMInformation\Model for Win >= 6.0
    GetPrivateProfileString(TEXT("General"),
                            TEXT("Model"),
                            TEXT("To Be Filled By O.E.M."),
                            Buf,
                            sizeof(Buf)/sizeof(Buf[0]),
                            Tmp);
    if (GetOemStrings(IDS_SYS_MODEL, Msg))
        _tprintf(Msg, Buf);

    //getting System type
    switch (SysInfo.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_INTEL:
            lpcszSysType = TEXT("X86-based PC");
            break;
        case PROCESSOR_ARCHITECTURE_IA64:
            lpcszSysType = TEXT("IA64-based PC");
            break;
        case PROCESSOR_ARCHITECTURE_AMD64:
            lpcszSysType = TEXT("AMD64-based PC");
            break;
        default:
            lpcszSysType = TEXT("Unknown");
            break;
    }
    if (GetOemStrings(IDS_SYS_TYPE, Msg))
        _tprintf(Msg, lpcszSysType);

    //getting Processor(s)
    if (GetOemStrings(IDS_PROCESSORS, Msg))
    {
        unsigned int i;
        _tprintf(Msg, (unsigned int)SysInfo.dwNumberOfProcessors);
        for(i = 0; i < (unsigned int)SysInfo.dwNumberOfProcessors; i++)
        {
            sprintf(Tmp, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%u",i);

            RegGetSZ(HKEY_LOCAL_MACHINE, Tmp, TEXT("Identifier"), Buf);
            _tprintf("                       [%02u]: %s", i+1, Buf);

            RegGetSZ(HKEY_LOCAL_MACHINE, Tmp, TEXT("VendorIdentifier"), Buf);
            _tprintf(" %s\n", Buf);
        }
    }

    //getting BIOS Version
    RegGetSZ(HKEY_LOCAL_MACHINE,
             TEXT("HARDWARE\\DESCRIPTION\\System"),
             TEXT("SystemBiosVersion"),
             Buf);
    if (GetOemStrings(IDS_BIOS_VERSION, Msg))
        _tprintf(Msg, Buf);

    //gettings BIOS date
    RegGetSZ(HKEY_LOCAL_MACHINE,
             TEXT("HARDWARE\\DESCRIPTION\\System"),
             TEXT("SystemBiosDate"),
             Buf);
    if (GetOemStrings(IDS_BIOS_DATE, Msg))
        _tprintf(Msg, Buf);

    //getting ReactOS Directory
    if (!GetWindowsDirectory(Buf, BUFFER_SIZE))
        _tprintf("Error! GetWindowsDirectory failed.");
    else if (GetOemStrings(IDS_ROS_DIR, Msg))
        _tprintf(Msg, Buf);

    //getting System Directory
    if (GetOemStrings(IDS_SYS_DIR, Msg))
        _tprintf(Msg, szSystemDir);

    //getting Boot Device
    RegGetSZ(HKEY_LOCAL_MACHINE,
             TEXT("SYSTEM\\Setup"),
             TEXT("SystemPartition"),
             Buf);
    if (GetOemStrings(IDS_BOOT_DEV, Msg))
        _tprintf(Msg, Buf);

    //getting System Locale
    if (RegGetSZ(HKEY_CURRENT_USER,
                 TEXT("Control Panel\\International"),
                 TEXT("Locale"),
                 Tmp))
        if (RegGetSZ(HKEY_CLASSES_ROOT,
                     TEXT("MIME\\Database\\Rfc1766"),
                     Tmp,
                     Buf))
            if (GetOemStrings(IDS_SYS_LOCALE, Msg))
                _tprintf(Msg, Buf);

    //getting Input Locale
    if (RegGetSZ(HKEY_CURRENT_USER,
                 TEXT("Keyboard Layout\\Preload"),
                 TEXT("1"),
                 Buf))
    {
        int i, j;

        for(j = 0, i = 4; i <= 8; j++, i++)
            Tmp[j] = Buf[i];

        if (RegGetSZ(HKEY_CLASSES_ROOT,
                     TEXT("MIME\\Database\\Rfc1766"),
                     Tmp,
                     Buf))
            if (GetOemStrings(IDS_INPUT_LOCALE, Msg))
                _tprintf(Msg, Buf);
    }

    //getting Time Zone
    GetTimeZoneInformation(&TimeZoneInfo);
    
    /* Open Time Zones key */
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones"),
                    0,
                    KEY_ENUMERATE_SUB_KEYS|KEY_READ,
                    &hKey) == ERROR_SUCCESS)
    {
        unsigned i;
        
        /* Find current timezone */
        dwCharCount = 256; // Windows seems to have a bug - it doesnt accept BUFFER_SIZE here
        for(i = 0; RegEnumKeyEx(hKey, i, Tmp, &dwCharCount, NULL, NULL, NULL, NULL) == ERROR_SUCCESS; ++i, dwCharCount = 255)
        {
#ifdef _UNICODE
            RegGetSZ(hKey, Tmp, TEXT("Std"), Buf);
            
            if(!wcscmp(wBuf, TimeZoneInfo.StandardName))
#else
            wchar_t wBuf[BUFFER_SIZE];

            RegGetSZ(hKey, Tmp, TEXT("Std"), Buf);
            mbstowcs(wBuf, Buf, BUFFER_SIZE);
            
            if(!wcscmp(wBuf, TimeZoneInfo.StandardName))
#endif
            {
                RegGetSZ(hKey, Tmp, TEXT("Display"), Buf);

                if (GetOemStrings(IDS_TIME_ZONE, Msg))
                    _tprintf(Msg, Buf);

                break;
            }
        }
        RegCloseKey(hKey);
    }
    
    
    //getting Total Physical Memory
    GlobalMemoryStatus(&MemoryStatus);
    FormatBytes(Buf, MemoryStatus.dwTotalPhys);
    if (GetOemStrings(IDS_TOTAL_PHYS_MEM, Msg))
        _tprintf(Msg, Buf);

    //getting Available Physical Memory
    FormatBytes(Buf, MemoryStatus.dwAvailPhys);
    if (GetOemStrings(IDS_AVAIL_PHISICAL_MEM,Msg))
        _tprintf(Msg, Buf);

    //getting Virtual Memory: Max Size
    FormatBytes(Buf, MemoryStatus.dwTotalVirtual);
    if (GetOemStrings(IDS_VIRT_MEM_MAX, Msg))
        _tprintf(Msg, Buf);

    //getting Virtual Memory: Available
    FormatBytes(Buf, MemoryStatus.dwAvailVirtual);
    if (GetOemStrings(IDS_VIRT_MEM_AVAIL, Msg))
        _tprintf(Msg, Buf);

    //getting Virtual Memory: In Use
    FormatBytes(Buf, MemoryStatus.dwTotalVirtual-MemoryStatus.dwAvailVirtual);
    if (GetOemStrings(IDS_VIRT_MEM_INUSE, Msg))
        _tprintf(Msg, Buf);

    //getting Page File Location(s)
    if (RegGetSZ(HKEY_LOCAL_MACHINE,
                 TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
                 TEXT("PagingFiles"),
                 Buf))
    {
        int i;

        for(i = 0; i < strlen((char*)Buf); i++)
        {
            if (Buf[i] == TEXT(' '))
            {
                Buf[i] = TEXT('\0');
                break;
            }
        }
        
        if(GetOemStrings(IDS_PAGEFILE_LOC, Msg))
            _tprintf(Msg, Buf);
    }

    //getting Domain
    if (RegGetSZ(HKEY_LOCAL_MACHINE,
                 TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                 TEXT("CachePrimaryDomain"),
                 Buf))
        if(GetOemStrings(IDS_DOMINE, Msg))
            _tprintf(Msg, Buf);

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
