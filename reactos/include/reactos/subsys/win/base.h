/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Base API Server DLL
 * FILE:            include/reactos/subsys/win/base.h
 * PURPOSE:         Public definitions for Base API Clients
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _BASE_H
#define _BASE_H

#pragma once

typedef
BOOL
(CALLBACK *BASE_PROCESS_CREATE_NOTIFY_ROUTINE)(
    HANDLE NewProcessId,
    HANDLE SourceThreadId,
    DWORD dwUnknown,
    ULONG CreateFlags);

NTSTATUS WINAPI BaseSetProcessCreateNotify(BASE_PROCESS_CREATE_NOTIFY_ROUTINE);

typedef struct _NLS_USER_INFO
{
    WCHAR sLanguage[80];
    WCHAR iCountry[80];
    WCHAR sCountry[80];
    WCHAR sList[80];
    WCHAR iMeasure[80];
    WCHAR iPaperSize[80];
    WCHAR sDecimal[80];
    WCHAR sThousand[80];
    WCHAR sGrouping[80];
    WCHAR iDigits[80];
    WCHAR iLZero[80];
    WCHAR iNegNumber[80];
    WCHAR sNativeDigits[80];
    WCHAR NumShape[80];
    WCHAR sCurrency[80];
    WCHAR sMonDecSep[80];
    WCHAR sMonThouSep[80];
    WCHAR sMonGrouping[80];
    WCHAR iCurrDigits[80];
    WCHAR iCurrency[80];
    WCHAR iNegCurr[80];
    WCHAR sPositiveSign[80];
    WCHAR sNegativeSign[80];
    WCHAR sTimeFormat[80];
    WCHAR sTime[80];
    WCHAR iTime[80];
    WCHAR iTLZero[80];
    WCHAR iTimePrefix[80];
    WCHAR s1159[80];
    WCHAR s2359[80];
    WCHAR sShortDate[80];
    WCHAR sDate[80];
    WCHAR iDate[80];
    WCHAR sYearMonth[80];
    WCHAR sLongDate[80];
    WCHAR iCalType[80];
    WCHAR iFirstDayOfWeek[80];
    WCHAR iFirstWeekOfYear[80];
    WCHAR Locale[80];
    LCID UserLocaleId;
    LUID InteractiveUserLuid;
    ULONG ulCacheUpdateCount;
} NLS_USER_INFO, *PNLS_USER_INFO;
C_ASSERT(sizeof(NLS_USER_INFO) == 0x1870);

typedef struct _INIFILE_MAPPING_TARGET
{
    struct _INIFILE_MAPPING_TARGET *Next;
    UNICODE_STRING RegistryPath;
} INIFILE_MAPPING_TARGET, *PINIFILE_MAPPING_TARGET;

typedef struct _INIFILE_MAPPING_VARNAME
{
    struct _INIFILE_MAPPING_VARNAME *Next;
    UNICODE_STRING Name;
    ULONG MappingFlags;
    PINIFILE_MAPPING_TARGET MappingTarget;
} INIFILE_MAPPING_VARNAME, *PINIFILE_MAPPING_VARNAME;

typedef struct _INIFILE_MAPPING_APPNAME
{
    struct _INIFILE_MAPPING_APPNAME *Next;
    UNICODE_STRING Name;
    PINIFILE_MAPPING_VARNAME VariableNames;
    PINIFILE_MAPPING_VARNAME DefaultVarNameMapping;
} INIFILE_MAPPING_APPNAME, *PINIFILE_MAPPING_APPNAME;

typedef struct _INIFILE_MAPPING_FILENAME
{
    struct _INIFILE_MAPPING_FILENAME *Next;
    UNICODE_STRING Name;
    PINIFILE_MAPPING_APPNAME ApplicationNames;
    PINIFILE_MAPPING_APPNAME DefaultAppNameMapping;
} INIFILE_MAPPING_FILENAME, *PINIFILE_MAPPING_FILENAME;

typedef struct _INIFILE_MAPPING
{
    PINIFILE_MAPPING_FILENAME FileNames;
    PINIFILE_MAPPING_FILENAME DefaultFileNameMapping;
    PINIFILE_MAPPING_FILENAME WinIniFileMapping;
    ULONG Reserved;
} INIFILE_MAPPING, *PINIFILE_MAPPING;

typedef struct _BASE_STATIC_SERVER_DATA
{
    UNICODE_STRING WindowsDirectory;
    UNICODE_STRING WindowsSystemDirectory;
    UNICODE_STRING NamedObjectDirectory;
    USHORT WindowsMajorVersion;
    USHORT WindowsMinorVersion;
    USHORT BuildNumber;
    USHORT CSDNumber;
    USHORT RCNumber;
    WCHAR CSDVersion[128];
    SYSTEM_BASIC_INFORMATION SysInfo;
    SYSTEM_TIMEOFDAY_INFORMATION TimeOfDay;
    PVOID IniFileMapping;
    NLS_USER_INFO NlsUserInfo;
    BOOLEAN DefaultSeparateVDM;
    BOOLEAN IsWowTaskReady;
    UNICODE_STRING WindowsSys32x86Directory;
    BOOLEAN fTermsrvAppInstallMode;
    TIME_ZONE_INFORMATION tziTermsrvClientTimeZone;
    KSYSTEM_TIME ktTermsrvClientBias;
    ULONG TermsrvClientTimeZoneId;
    BOOLEAN LUIDDeviceMapsEnabled;
    ULONG TermsrvClientTimeZoneChangeNum;
} BASE_STATIC_SERVER_DATA, *PBASE_STATIC_SERVER_DATA;
#ifndef _WIN64
C_ASSERT(sizeof(BASE_STATIC_SERVER_DATA) == 0x1AC8);
#endif

#endif // _BASE_H

/* EOF */
