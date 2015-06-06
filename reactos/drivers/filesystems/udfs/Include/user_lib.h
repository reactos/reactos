////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////

/*************************************************************************
*
* File: user_lib.h
*
* Module: User-mode library header
*
* Description: common useful user-mode functions
*
* Author: Ivan
*
*************************************************************************/


#ifndef __USER_LIB_H__
#define __USER_LIB_H__

#if defined DBG || defined PRINT_ALWAYS
#define ODS(sz) OutputDebugString(sz)
#else
#define ODS(sz) {}
#endif 

#define arraylen(a)     (sizeof(a)/sizeof(a[0]))

/// CD/DVD-RW device types
typedef enum _JS_DEVICE_TYPE {
    OTHER = 0,
    CDR,
    CDRW,
    DVDROM,
    DVDRAM,
    DVDR,
    DVDRW,
    DVDPR,
    DVDPRW,
    DDCDROM,
    DDCDR,
    DDCDRW,
    BDROM,
    BDRE,
    BUSY
} JS_DEVICE_TYPE;

extern TCHAR* MediaTypeStrings[];

/// Service state constants
typedef enum _JS_SERVICE_STATE {
    JS_SERVICE_NOT_PRESENT, ///< Service not installed
    JS_SERVICE_RUNNING,     ///< Service is running
    JS_SERVICE_NOT_RUNNING, ///< Service installed, but not running
    JS_ERROR_STATUS         ///< Errror while taking service status
} JS_SERVICE_STATE;

void * __cdecl mymemchr (
        const void * buf,
        int chr,
        size_t cnt
        );

char * __cdecl mystrrchr (
        const char * string,
        int ch
        );

char * __cdecl  mystrchr (
        const char * string,
        int ch
        );

int __cdecl Exist (
        PCHAR path
        );

ULONG MyMessageBox(
    HINSTANCE hInst,
    HWND hWnd,
    LPCSTR pszFormat,
    LPCSTR pszTitle,
    UINT fuStyle,
    ...
    );

// simple win32 registry api wrappers
BOOL RegisterString (LPSTR pszKey, LPSTR pszValue, LPSTR pszData,BOOLEAN MultiSz,DWORD size);
BOOL
GetRegString (
  LPSTR pszKey,
  LPSTR pszValue,
  LPSTR pszData,
  DWORD dwBufSize
  );
BOOL RegDelString   (LPSTR pszKey, LPSTR pszValue);
BOOL RegisterDword  (LPSTR pszKey, LPSTR pszValue, DWORD dwData);
BOOL
GetRegUlong (
  LPSTR pszKey,
  LPSTR pszValue,
  LPDWORD pszData
  );


JS_SERVICE_STATE
ServiceInfo(
    LPCTSTR ServiceName
    );

BOOL
CheckCdrwFilter(
    BOOL ReInstall
    );

BOOL
Privilege(
    LPTSTR pszPrivilege,
    BOOL bEnable
    );

BOOL IsWow64(VOID);

#define         DW_GLOBAL_QUIT_EVENT_NAME   L"Global\\DwQuitEvent"
#define         DW_QUIT_EVENT_NAME   L"DwQuitEvent"

HANDLE
CreatePublicEvent(
    PWCHAR EventName
    );

ULONG
UDFPhSendIOCTL(
    IN ULONG IoControlCode,
    IN HANDLE DeviceObject,
    IN PVOID InputBuffer ,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer ,
    IN ULONG OutputBufferLength,
    IN BOOLEAN OverrideVerify,
    IN PVOID Dummy
    );

PCHAR
UDFGetDeviceName(
    PCHAR szDeviceName
    );

BOOL
GetOptUlong(
    PCHAR Path,
    PCHAR OptName,
    PULONG OptVal
    );

BOOL
SetOptUlong(
    PCHAR Path,
    PCHAR OptName,
    PULONG OptVal
    );

#define UDF_OPTION_GLOBAL     1
#define UDF_OPTION_MEDIASPEC  10
#define UDF_OPTION_DEVSPEC    2
#define UDF_OPTION_DISKSPEC   3
#define UDF_OPTION_MAX_DEPTH  0xffffffff

ULONG
UDFGetOptUlongInherited(
    PCHAR Drive,
    PCHAR OptName,
    PULONG OptVal,
    ULONG CheckDepth
    );

HANDLE
OpenOurVolume(
    PCHAR szDeviceName
    );

ULONG
drv_letter_to_index(
    WCHAR a
    );

DWORD
WINAPI
LauncherRoutine2(
    LPVOID lpParameter
    );

#endif // __USER_LIB_H__
