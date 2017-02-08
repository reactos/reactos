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

typedef VOID (CALLBACK * BASE_PROCESS_CREATE_NOTIFY_ROUTINE)(PVOID);

NTSTATUS WINAPI BaseSetProcessCreateNotify(BASE_PROCESS_CREATE_NOTIFY_ROUTINE);

typedef struct _NLS_USER_INFO
{
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
    WCHAR iDigitSubstitution[80];
    WCHAR sCurrency[80];
    WCHAR sMonDecSep[80];
    WCHAR sMonThouSep[80];
    WCHAR sMonGrouping[80];
    WCHAR iCurrDigits[80];
    WCHAR iCurrency[80];
    WCHAR iNegCurr[80];
    WCHAR sPosSign[80];
    WCHAR sNegSign[80];
    WCHAR sTimeFormat[80];
    WCHAR s1159[80];
    WCHAR s2359[80];
    WCHAR sShortDate[80];
    WCHAR sYearMonth[80];
    WCHAR sLongDate[80];
    WCHAR iCalType[80];
    WCHAR iFirstDay[80];
    WCHAR iFirstWeek[80];
    WCHAR sLocale[80];
    WCHAR sLocaleName[85];
    LCID UserLocaleId;
    LUID InteractiveUserLuid;
    CHAR InteractiveUserSid[68]; // SECURITY_MAX_SID_SIZE to make ROS happy
    ULONG ulCacheUpdateCount;
} NLS_USER_INFO, *PNLS_USER_INFO;


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

#endif // _BASE_H

/* EOF */
