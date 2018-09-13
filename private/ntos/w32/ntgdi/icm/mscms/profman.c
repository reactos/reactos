/****************************Module*Header******************************\
* Module Name: PROFMAN.C
*
* Module Descripton: Profile management functions.
*
* Warnings:
*
* Issues:
*
* Public Routines:
*
* Created: 5 Nov 1996
*
* Author:   Srinivasan Chandrasekar    [srinivac]
*
* Copyright (c) 1996, 1997  Microsoft Corporation
\***********************************************************************/

#include "mscms.h"
#include "objbase.h"
#include "initguid.h"
#include "devguid.h"
#include "sti.h"

#define TAG_DEVICESETTINGS      'devs'
#define TAG_MS01                'MS01'
#define TAG_MS02                'MS02'
#define TAG_MS03                'MS03'

#define ID_MSFT_REVERSED        'tfsm'
#define ID_MEDIATYPE_REVERSED   'aidm'
#define ID_DITHER_REVERSED      'ntfh'
#define ID_RESLN_REVERSED       'nlsr'

#define DEVICE_PROFILE_DATA      1
#define DEVICE_PROFILE_ENUMMODE  2

//
// Local types
//

typedef enum {
    NOMATCH     = 0,
    MATCH       = 1,
    EXACT_MATCH = 2,
} MATCHTYPE;

typedef struct tagREGDATA {
    DWORD dwRefCount;
    DWORD dwManuID;
    DWORD dwModelID;
} REGDATA, *PREGDATA;

typedef struct tagSCANNERDATA {
    PWSTR     pDeviceName;
    HINSTANCE hModule;
    PSTI      pSti;
} SCANNERDATA, *PSCANNERDATA;

typedef BOOL  (WINAPI *PFNOPENDEVICE)(PTSTR, LPHANDLE, PTSTR);
typedef BOOL  (WINAPI *PFNCLOSEDEVICE)(HANDLE);
typedef DWORD (WINAPI *PFNGETDEVICEDATA)(HANDLE, PTSTR, PTSTR, PDWORD, PBYTE, DWORD, PDWORD);
typedef DWORD (WINAPI *PFNSETDEVICEDATA)(HANDLE, PTSTR, PTSTR, DWORD, PBYTE, DWORD);
typedef HRESULT (__stdcall *PFNSTICREATEINSTANCE)(HINSTANCE, DWORD, PSTI*, LPDWORD);

//
// Local functions
//

BOOL  InternalGetColorDirectory(LPCTSTR, PTSTR, DWORD*);
BOOL  InternalInstallColorProfile(LPCTSTR, LPCTSTR);
BOOL  InternalUninstallColorProfile(LPCTSTR, LPCTSTR, BOOL);
BOOL  InternalAssociateColorProfileWithDevice(LPCTSTR, LPCTSTR, LPCTSTR);
BOOL  InternalDisassociateColorProfileFromDevice(LPCTSTR, LPCTSTR, LPCTSTR);
BOOL  InternalEnumColorProfiles(LPCTSTR, PENUMTYPE, PBYTE, PDWORD, PDWORD);
BOOL  InternalSetSCSProfile(LPCTSTR, DWORD, LPCTSTR);
BOOL  InternalGetSCSProfile(LPCTSTR, DWORD, PTSTR, PDWORD);
VOID  ConvertDwordToString(DWORD, PTSTR);
PTSTR ConvertClassIdToClassString(DWORD);
BOOL  GetProfileClassString(LPCTSTR, PTSTR, PPROFILEHEADER);
BOOL  GetDeviceData(LPCTSTR, DWORD, DWORD, PVOID*, PDWORD, BOOL);
BOOL  SetDeviceData(LPCTSTR, DWORD, DWORD, PVOID, DWORD);
BOOL  IGetDeviceData(LPCTSTR, DWORD, DWORD, PVOID*, PDWORD, BOOL);
BOOL  ISetDeviceData(LPCTSTR, DWORD, DWORD, PVOID, DWORD);
BOOL  IsStringInMultiSz(PTSTR, PTSTR);
DWORD RemoveStringFromMultiSz(PTSTR, PTSTR, DWORD);
VOID  InsertInBuffer(PBYTE, PBYTE, PTSTR);
MATCHTYPE DoesProfileMatchEnumRecord(PTSTR, PENUMTYPE);
MATCHTYPE CheckResMedHftnMatch(HPROFILE, PENUMTYPE);
BOOL  DwordMatches(PSETTINGS, DWORD);
BOOL  QwordMatches(PSETTINGS, PDWORD);
BOOL  WINAPI OpenPrtr(PTSTR, LPHANDLE, PTSTR);
BOOL  WINAPI ClosePrtr(HANDLE);
DWORD WINAPI GetPrtrData(HANDLE, PTSTR, PTSTR, PDWORD, PBYTE, DWORD, PDWORD);
DWORD WINAPI SetPrtrData(HANDLE, PTSTR, PTSTR, DWORD, PBYTE, DWORD);
BOOL  WINAPI OpenMonitor(PTSTR, LPHANDLE, PTSTR);
BOOL  WINAPI CloseMonitor(HANDLE);
DWORD WINAPI GetMonitorData(HANDLE, PTSTR, PTSTR, PDWORD, PBYTE, DWORD, PDWORD);
DWORD WINAPI SetMonitorData(HANDLE, PTSTR, PTSTR, DWORD, PBYTE, DWORD);
BOOL  WINAPI OpenScanner(PTSTR, LPHANDLE, PTSTR);
BOOL  WINAPI CloseScanner(HANDLE);
DWORD WINAPI GetScannerData(HANDLE, PTSTR, PTSTR, PDWORD, PBYTE, DWORD, PDWORD);
DWORD WINAPI SetScannerData(HANDLE, PTSTR, PTSTR, DWORD, PBYTE, DWORD);
#ifdef _WIN95_
BOOL  LoadSetupAPIDll(VOID);
#else
VOID  ChangeICMSetting(LPCTSTR, LPCTSTR, DWORD);
#endif // _WIN95_

//
// SetupAPI function pointers
//
typedef WINSETUPAPI HKEY
(WINAPI *FP_SetupDiOpenDevRegKey)(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD            Scope,
    IN DWORD            HwProfile,
    IN DWORD            KeyType,
    IN REGSAM           samDesired
);

typedef WINSETUPAPI BOOL
(WINAPI *FP_SetupDiDestroyDeviceInfoList)(
    IN HDEVINFO DeviceInfoSet
);

typedef WINSETUPAPI BOOL
(WINAPI *FP_SetupDiEnumDeviceInfo)(
    IN  HDEVINFO         DeviceInfoSet,
    IN  DWORD            MemberIndex,
    OUT PSP_DEVINFO_DATA DeviceInfoData
);

#if !defined(_WIN95_)
typedef WINSETUPAPI BOOL
(WINAPI *FP_SetupDiGetDeviceInstanceId)(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    OUT PWSTR            DeviceInstanceId,
    IN  DWORD            DeviceInstanceIdSize,
    OUT PDWORD           RequiredSize          OPTIONAL
);

typedef WINSETUPAPI HDEVINFO
(WINAPI *FP_SetupDiGetClassDevs)(
    IN LPGUID ClassGuid,  OPTIONAL
    IN PCWSTR Enumerator, OPTIONAL
    IN HWND   hwndParent, OPTIONAL
    IN DWORD  Flags
);
#else
typedef WINSETUPAPI BOOL
(WINAPI *FP_SetupDiGetDeviceInstanceId)(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    OUT PSTR             DeviceInstanceId,
    IN  DWORD            DeviceInstanceIdSize,
    OUT PDWORD           RequiredSize          OPTIONAL
);

typedef WINSETUPAPI HDEVINFO
(WINAPI *FP_SetupDiGetClassDevs)(
    IN LPGUID ClassGuid,  OPTIONAL
    IN PCSTR  Enumerator, OPTIONAL
    IN HWND   hwndParent, OPTIONAL
    IN DWORD  Flags
);
#endif

HMODULE ghModSetupAPIDll = NULL;

FP_SetupDiOpenDevRegKey         fpSetupDiOpenDevRegKey         = NULL;
FP_SetupDiDestroyDeviceInfoList fpSetupDiDestroyDeviceInfoList = NULL;
FP_SetupDiEnumDeviceInfo        fpSetupDiEnumDeviceInfo        = NULL;
FP_SetupDiGetDeviceInstanceId   fpSetupDiGetDeviceInstanceId   = NULL;
FP_SetupDiGetClassDevs          fpSetupDiGetClassDevs          = NULL;

//
// Predefined profiles in order - INF file has 1-based index into this list
//

TCHAR  *gszDispProfiles[] = {
    __TEXT("mnB22G15.icm"),                         // 1
    __TEXT("mnB22G18.icm"),                         // 2
    __TEXT("mnB22G21.icm"),                         // 3
    __TEXT("mnEBUG15.icm"),                         // 4
    __TEXT("mnEBUG18.icm"),                         // 5
    __TEXT("mnEBUG21.icm"),                         // 6
    __TEXT("mnP22G15.icm"),                         // 7
    __TEXT("mnP22G18.icm"),                         // 8
    __TEXT("mnP22G21.icm"),                         // 9
    __TEXT("Diamond Compatible 9300K G2.2.icm"),    // 10
    __TEXT("Hitachi Compatible 9300K G2.2.icm"),    // 11
    __TEXT("NEC Compatible 9300K G2.2.icm"),        // 12
    __TEXT("Trinitron Compatible 9300K G2.2.icm"),  // 13
    };

TCHAR  *gpszClasses[] = {  // different profile classes
    __TEXT("mntr"),                                 // 0
    __TEXT("prtr"),                                 // 1
    __TEXT("scnr"),                                 // 2
    __TEXT("link"),                                 // 3
    __TEXT("abst"),                                 // 4
    __TEXT("spac"),                                 // 5
    __TEXT("nmcl")                                  // 6
    };

#define INDEX_CLASS_MONITOR     0
#define INDEX_CLASS_PRINTER     1
#define INDEX_CLASS_SCANNER     2
#define INDEX_CLASS_LINK        3
#define INDEX_CLASS_ABSTRACT    4
#define INDEX_CLASS_COLORSPACE  5
#define INDEX_CLASS_NAMED       6

/******************************************************************************
 *
 *                            GetColorDirectory
 *
 *  Function:
 *       These are the ANSI & Unicode wrappers for InternalGetColorDirectory.
 *       Please see InternalGetColorDirectory for more details on this
 *       function.
 *
 *  Arguments:
 *       pMachineName    - name identifying machine on which the path
 *                         to the color directory is requested
 *       pBuffer         - pointer to buffer to receive pathname
 *       pdwSize         - pointer to size of buffer. On return it has size of
 *                         buffer needed if failure, and used on success
 *
 *  Returns:
 *       TRUE if successful, NULL otherwise
 *
 ******************************************************************************/

#ifdef UNICODE          // Windows NT versions

BOOL WINAPI
GetColorDirectoryA(
    PCSTR   pMachineName,
    PSTR    pBuffer,
    PDWORD  pdwSize
    )
{
    PWSTR pwszMachineName = NULL;   // Unicode machine name
    PWSTR pwBuffer = NULL;          // Unicode color directory path
    DWORD dwSize;                   // size of Unicode buffer
    DWORD dwErr = 0;                // error code
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("GetColorDirectoryA\n")));

    //
    // Validate parameters before we touch them
    //

    if (!pdwSize ||
        IsBadWritePtr(pdwSize, sizeof(DWORD)) ||
        (pBuffer && IsBadWritePtr(pBuffer, *pdwSize)))
    {
        WARNING((__TEXT("Invalid parameter to GetColorDirectory\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Convert machine name to Unicode
    //

    if (pMachineName)
    {
        rc = ConvertToUnicode(pMachineName, &pwszMachineName, TRUE);
    }
    else
        pwszMachineName = NULL;

    dwSize = *pdwSize * sizeof(WCHAR);

    //
    // Create a buffer to get Unicode directory from system
    //

    if (pBuffer && dwSize)
    {
        pwBuffer = (PWSTR)MemAlloc(dwSize);
        if (! pwBuffer)
        {
            WARNING((__TEXT("Error allocating memory for Unicode string\n")));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            rc = FALSE;
            goto EndGetColorDirectoryA;
        }
    }

    rc = rc && InternalGetColorDirectory(pwszMachineName, pwBuffer, &dwSize);

    *pdwSize = dwSize / sizeof(WCHAR);

    //
    // Convert Unicode path to Ansi
    //

    if (pwBuffer)
    {
        rc = rc && ConvertToAnsi(pwBuffer, &pBuffer, FALSE);
    }

EndGetColorDirectoryA:
    if (pwszMachineName)
    {
        MemFree(pwszMachineName);
    }

    if (pwBuffer)
    {
        MemFree(pwBuffer);
    }

    return rc;
}

BOOL WINAPI
GetColorDirectoryW(
    PCWSTR   pMachineName,
    PWSTR    pBuffer,
    PDWORD   pdwSize
    )
{
    TRACEAPI((__TEXT("GetColorDirectoryW\n")));

    //
    // Internal function is ANSI in Windows 95, call it directly.
    //

    return InternalGetColorDirectory(pMachineName, pBuffer, pdwSize);
}

#else                           // Windows 95 versions

BOOL WINAPI
GetColorDirectoryA(
    PCSTR   pMachineName,
    PSTR    pBuffer,
    PDWORD  pdwSize
    )
{
    TRACEAPI((__TEXT("GetColorDirectoryA\n")));

    //
    // Internal function is ANSI in Windows 95, call it directly.
    //

    return InternalGetColorDirectory(pMachineName, pBuffer, pdwSize);
}

BOOL WINAPI
GetColorDirectoryW(
    PCWSTR   pMachineName,
    PWSTR    pBuffer,
    PDWORD   pdwSize
    )
{
    PSTR pszMachineName = NULL;     // Ansi machine name
    PSTR pszBuffer = NULL;          // Ansi color directory path
    DWORD dwSize;                   // size of Ansi buffer
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("GetColorDirectoryW\n")));

    //
    // Validate parameters before we touch them
    //

    if (!pdwSize ||
        IsBadWritePtr(pdwSize, sizeof(DWORD)) ||
        (pBuffer && IsBadWritePtr(pBuffer, *pdwSize)))
    {
        WARNING((__TEXT("Invalid parameter to GetColorDirectory\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Convert machine name to Ansi
    //

    if (pMachineName)
    {
        rc = ConvertToAnsi(pMachineName, &pszMachineName, TRUE);
    }
    else
        pszMachineName = NULL;

    //
    // Create a buffer to get Ansi directory from system
    //

    dwSize = *pdwSize / sizeof(WCHAR);

    if (pBuffer && dwSize)
    {
        pszBuffer = (PSTR)MemAlloc(dwSize);
        if (! pszBuffer)
        {
            WARNING((__TEXT("Error allocating memory for Ansi string\n")));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            rc = FALSE;
            goto EndGetColorDirectoryW;
        }
    }

    rc = rc && InternalGetColorDirectory(pszMachineName, pszBuffer, &dwSize);

    *pdwSize = dwSize * sizeof(WCHAR);

    //
    // Convert Ansi path to Unicode
    //

    if (pszBuffer)
    {
        rc = rc && ConvertToUnicode(pszBuffer, &pBuffer, FALSE);
    }

EndGetColorDirectoryW:
    if (pszMachineName)
    {
        MemFree(pszMachineName);
    }

    if (pszBuffer)
    {
        MemFree(pszBuffer);
    }

    return rc;
}

#endif                          // ! UNICODE


/******************************************************************************
 *
 *                            InstallColorProfile
 *
 *  Function:
 *       These are the ANSI & Unicode wrappers for InternalInstallColorProfile.
 *       Please see InternalInstallColorProfile for more details on this
 *       function.
 *
 *  Arguments:
 *       pMachineName    - name identifying machine on which the profile
 *                         should be installed. NULL implies local
 *       pProfileName    - pointer to filename of profile to install
 *
 *  Returns:
 *       TRUE if successful, NULL otherwise
 *
 ******************************************************************************/

#ifdef UNICODE          // Windows NT versions

BOOL  WINAPI
InstallColorProfileA(
    PCSTR   pMachineName,
    PCSTR   pProfileName
    )

{
    PWSTR pwszMachineName = NULL;   // Unicode machine name
    PWSTR pwszProfileName = NULL;   // Unicode profile name
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("InstallColorProfileA\n")));

    //
    // Validate parameters before we touch them
    //

    if (!pProfileName)
    {
        WARNING((__TEXT("Invalid parameter to InstallColorProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Convert machine name to Unicode
    //

    if (pMachineName)
    {
        rc = ConvertToUnicode(pMachineName, &pwszMachineName, TRUE);
    }
    else
        pwszMachineName = NULL;

    //
    // Convert profile name to Unicode
    //

    rc = rc && ConvertToUnicode(pProfileName, &pwszProfileName, TRUE);

    //
    // Call the internal Unicode function
    //

    rc = rc && InternalInstallColorProfile(pwszMachineName, pwszProfileName);

    //
    // Free memory before leaving
    //

    if (pwszProfileName)
    {
        MemFree(pwszProfileName);
    }

    if (pwszMachineName)
    {
        MemFree(pwszMachineName);
    }

    return rc;
}


BOOL  WINAPI
InstallColorProfileW(
    PCWSTR   pMachineName,
    PCWSTR   pProfileName
    )
{
    TRACEAPI((__TEXT("InstallColorProfileW\n")));

    //
    // Internal function is Unicode in Windows NT, call it directly.
    //

    return InternalInstallColorProfile(pMachineName, pProfileName);
}


#else                           // Windows 95 versions

BOOL  WINAPI
InstallColorProfileA(
    PCSTR   pMachineName,
    PCSTR   pProfileName
    )
{
    TRACEAPI((__TEXT("InstallColorProfileA\n")));

    //
    // Internal function is ANSI in Windows 95, call it directly.
    //

    return InternalInstallColorProfile(pMachineName, pProfileName);
}


BOOL  WINAPI
InstallColorProfileW(
    PCWSTR   pMachineName,
    PCWSTR   pProfileName
    )
{
    PSTR  pszMachineName = NULL;    // Ansi machine name
    PSTR  pszProfileName = NULL;    // Ansi profile name
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("InstallColorProfileW\n")));

    //
    // Validate parameters before we touch them
    //

    if (!pProfileName)
    {
        WARNING((__TEXT("Invalid parameter to InstallColorProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Convert machine name to Ansi
    //

    if (pMachineName)
    {
        rc = ConvertToAnsi(pMachineName, &pszMachineName, TRUE);
    }
    else
        pszMachineName = NULL;

    //
    // Convert profile name to Ansi
    //

    rc = rc && ConvertToAnsi(pProfileName, &pszProfileName, TRUE);

    //
    // Call the internal Ansi function
    //

    rc = rc && InternalInstallColorProfile(pszMachineName, pszProfileName);

    //
    // Free memory before leaving
    //

    if (pszProfileName)
    {
        MemFree(pszProfileName);
    }

    if (pszMachineName)
    {
        MemFree(pszMachineName);
    }

    return rc;
}


#endif                          // ! UNICODE

/******************************************************************************
 *
 *                            UninstallColorProfile
 *
 *  Function:
 *       These are the ANSI & Unicode wrappers for InternalUninstallColorProfile.
 *       Please see InternalUninstallColorProfile for more details on this
 *       function.
 *
 *  Arguments:
 *       pMachineName    - name identifying machine on which the profile
 *                         should be uninstalled. NULL implies local
 *       pProfileName    - pointer to filename of profile to uninstall
 *       bDelete         - TRUE if profile should be deleted in disk
 *
 *  Returns:
 *       TRUE if successful, NULL otherwise
 *
 ******************************************************************************/

#ifdef UNICODE          // Windows NT versions

BOOL  WINAPI
UninstallColorProfileA(
    PCSTR   pMachineName,
    PCSTR   pProfileName,
    BOOL    bDelete
    )
{
    PWSTR pwszMachineName = NULL;   // Unicode machine name
    PWSTR pwszProfileName = NULL;   // Unicode profile name
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("UninstallColorProfileA\n")));

    //
    // Validate parameters before we touch them
    //

    if (!pProfileName)
    {
        WARNING((__TEXT("Invalid parameter to UninstallColorProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Convert machine name to Unicode
    //

    if (pMachineName)
    {
        rc = ConvertToUnicode(pMachineName, &pwszMachineName, TRUE);
    }
    else
        pwszMachineName = NULL;

    //
    // Convert profile name to Unicode
    //

    rc = rc && ConvertToUnicode(pProfileName, &pwszProfileName, TRUE);

    //
    // Call the internal Unicode function
    //

    rc = rc && InternalUninstallColorProfile(pwszMachineName, pwszProfileName,
                    bDelete);

    //
    // Free memory before leaving
    //

    if (pwszProfileName)
    {
        MemFree(pwszProfileName);
    }

    if (pwszMachineName)
    {
        MemFree(pwszMachineName);
    }

    return rc;
}

BOOL  WINAPI
UninstallColorProfileW(
    PCWSTR   pMachineName,
    PCWSTR   pProfileName,
    BOOL     bDelete
    )
{
    TRACEAPI((__TEXT("UninstallColorProfileW\n")));

    //
    // Internal function is Unicode in Windows NT, call it directly.
    //

    return InternalUninstallColorProfile(pMachineName, pProfileName, bDelete);
}


#else                           // Windows 95 versions

BOOL  WINAPI
UninstallColorProfileA(
    PCSTR   pMachineName,
    PCSTR   pProfileName,
    BOOL    bDelete
    )
{
    TRACEAPI((__TEXT("UninstallColorProfileA\n")));

    //
    // Internal function is ANSI in Windows 95, call it directly.
    //

    return InternalUninstallColorProfile(pMachineName, pProfileName, bDelete);
}


BOOL  WINAPI
UninstallColorProfileW(
    PCWSTR   pMachineName,
    PCWSTR   pProfileName,
    BOOL     bDelete
    )
{
    PSTR  pszMachineName = NULL;    // Ansi machine name
    PSTR  pszProfileName = NULL;    // Ansi profile name
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("UninstallColorProfileW\n")));

    //
    // Validate parameters before we touch them
    //

    if (!pProfileName)
    {
        WARNING((__TEXT("Invalid parameter to UninstallColorProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Convert machine name to Ansi
    //

    if (pMachineName)
    {
        rc = ConvertToAnsi(pMachineName, &pszMachineName, TRUE);
    }
    else
        pszMachineName = NULL;

    //
    // Convert profile name to Ansi
    //

    rc = rc && ConvertToAnsi(pProfileName, &pszProfileName, TRUE);

    //
    // Call the internal Ansi function
    //

    rc = rc && InternalUninstallColorProfile(pszMachineName, pszProfileName,
                    bDelete);

    //
    // Free memory before leaving
    //

    if (pszProfileName)
    {
        MemFree(pszProfileName);
    }

    if (pszMachineName)
    {
        MemFree(pszMachineName);
    }

    return rc;
}


#endif                          // ! UNICODE


/******************************************************************************
 *
 *                        AssociateColorProfileWithDevice
 *
 *  Function:
 *       These are the ANSI & Unicode wrappers for
 *       InternalAssociateColorProfileWithDevice. Please see
 *       InternalAssociateColorProfileWithDevice for more details
 *       on this function.
 *
 *  Arguments:
 *       pMachineName    - name identifying machine. NULL implies local
 *       pProfileName    - pointer to profile to associate
 *       pDeviceName     - pointer to device name
 *
 *  Returns:
 *       TRUE if successful, NULL otherwise
 *
 ******************************************************************************/

#ifdef UNICODE          // Windows NT versions

BOOL WINAPI
AssociateColorProfileWithDeviceA(
    PCSTR pMachineName,
    PCSTR pProfileName,
    PCSTR pDeviceName
    )
{
    PWSTR pwszMachineName = NULL;   // Unicode machine name
    PWSTR pwszProfileName = NULL;   // Unicode profile name
    PWSTR pwszDeviceName = NULL;    // Unicode device name
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("AssociateColorProfileWithDeviceA\n")));

    //
    // Validate parameters before we touch  them
    //

    if (! pProfileName ||
        ! pDeviceName)
    {
        WARNING((__TEXT("Invalid parameter to AssociateColorProfileWithDevice\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Convert machine name to Unicode
    //

    if (pMachineName)
    {
        rc = ConvertToUnicode(pMachineName, &pwszMachineName, TRUE);
    }
    else
        pwszMachineName = NULL;

    //
    // Convert profile name to Unicode
    //

    rc = rc && ConvertToUnicode(pProfileName, &pwszProfileName, TRUE);

    //
    // Convert device name to Unicode
    //

    rc = rc && ConvertToUnicode(pDeviceName, &pwszDeviceName, TRUE);

    //
    // Call the internal Unicode function
    //

    rc = rc && InternalAssociateColorProfileWithDevice(pwszMachineName,
                pwszProfileName, pwszDeviceName);

    //
    // Free memory before leaving
    //

    if (pwszProfileName)
    {
        MemFree(pwszProfileName);
    }

    if (pwszMachineName)
    {
        MemFree(pwszMachineName);
    }

    if (pwszDeviceName)
    {
        MemFree(pwszDeviceName);
    }

    return rc;
}


BOOL WINAPI
AssociateColorProfileWithDeviceW(
    PCWSTR pMachineName,
    PCWSTR pProfileName,
    PCWSTR pDeviceName
    )
{
    TRACEAPI((__TEXT("AssociateColorProfileWithDeviceW\n")));

    //
    // Internal function is Unicode in Windows NT, call it directly.
    //

    return InternalAssociateColorProfileWithDevice(pMachineName,
                pProfileName, pDeviceName);
}


#else                           // Windows 95 versions

BOOL WINAPI
AssociateColorProfileWithDeviceA(
    PCSTR pMachineName,
    PCSTR pProfileName,
    PCSTR pDeviceName
    )
{
    TRACEAPI((__TEXT("AssociateColorProfileWithDeviceA\n")));

    //
    // Internal function is ANSI in Windows 95, call it directly.
    //

    return InternalAssociateColorProfileWithDevice(pMachineName,
                pProfileName, pDeviceName);
}


BOOL WINAPI
AssociateColorProfileWithDeviceW(
    PCWSTR pMachineName,
    PCWSTR pProfileName,
    PCWSTR pDeviceName
    )
{
    PSTR  pszMachineName = NULL;    // Ansi machine name
    PSTR  pszProfileName = NULL;    // Ansi profile name
    PSTR  pszDeviceName = NULL;     // Ansi device name
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("AssociateColorProfileWithDeviceW\n")));

    //
    // Validate parameters before we touch them
    //

    if (! pProfileName ||
        ! pDeviceName)
    {
        WARNING((__TEXT("Invalid parameter to AssociateColorProfileWithDevice\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Convert machine name to Ansi
    //

    if (pMachineName)
    {
        rc = ConvertToAnsi(pMachineName, &pszMachineName, TRUE);
    }
    else
        pszMachineName = NULL;

    //
    // Convert profile name to Ansi
    //

    rc = rc && ConvertToAnsi(pProfileName, &pszProfileName, TRUE);

    //
    // Convert device name to Ansi
    //

    rc = rc && ConvertToAnsi(pDeviceName, &pszDeviceName, TRUE);

    //
    // Call the internal Ansi function
    //

    rc = rc && InternalAssociateColorProfileWithDevice(pszMachineName,
                pszProfileName, pszDeviceName);

    //
    // Free memory before leaving
    //

    if (pszProfileName)
    {
        MemFree(pszProfileName);
    }

    if (pszMachineName)
    {
        MemFree(pszMachineName);
    }

    if (pszDeviceName)
    {
        MemFree(pszDeviceName);
    }

    return rc;
}

#endif                          // ! UNICODE


/******************************************************************************
 *
 *                     DisassociateColorProfileFromDevice
 *
 *  Function:
 *       These are the ANSI & Unicode wrappers for
 *       InternalDisassociateColorProfileFromDevice. Please see
 *       InternalDisassociateColorProfileFromDevice for more details
 *       on this function.
 *
 *  Arguments:
 *       pMachineName    - name identifying machine. NULL implies local
 *       pProfileName    - pointer to profile to disassiciate
 *       pDeviceName     - pointer to device name
 *
 *  Returns:
 *       TRUE if successful, NULL otherwise
 *
 ******************************************************************************/

#ifdef UNICODE          // Windows NT versions

BOOL WINAPI
DisassociateColorProfileFromDeviceA(
    PCSTR pMachineName,
    PCSTR pProfileName,
    PCSTR pDeviceName
    )
{
    PWSTR pwszMachineName = NULL;   // Unicode machine name
    PWSTR pwszProfileName = NULL;   // Unicode profile name
    PWSTR pwszDeviceName = NULL;    // Unicode device name
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("DisassociateColorProfileWithDeviceA\n")));

    //
    // Validate parameters before we touch  them
    //

    if (! pProfileName ||
        ! pDeviceName)
    {
        WARNING((__TEXT("Invalid parameter to DisassociateColorProfileFromDevice\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Convert machine name to Unicode
    //

    if (pMachineName)
    {
        rc = ConvertToUnicode(pMachineName, &pwszMachineName, TRUE);
    }
    else
        pwszMachineName = NULL;

    //
    // Convert profile name to Unicode
    //

    rc = rc && ConvertToUnicode(pProfileName, &pwszProfileName, TRUE);

    //
    // Convert device name to Unicode
    //

    rc = rc && ConvertToUnicode(pDeviceName, &pwszDeviceName, TRUE);

    //
    // Call the internal Unicode function
    //

    rc = rc && InternalDisassociateColorProfileFromDevice(pwszMachineName,
                pwszProfileName, pwszDeviceName);

    //
    // Free memory before leaving
    //

    if (pwszProfileName)
    {
        MemFree(pwszProfileName);
    }

    if (pwszMachineName)
    {
        MemFree(pwszMachineName);
    }

    if (pwszDeviceName)
    {
        MemFree(pwszDeviceName);
    }

    return rc;
}


BOOL WINAPI
DisassociateColorProfileFromDeviceW(
    PCWSTR pMachineName,
    PCWSTR pProfileName,
    PCWSTR pDeviceName
    )
{
    TRACEAPI((__TEXT("DisassociateColorProfileWithDeviceW\n")));

    //
    // Internal function is Unicode in Windows NT, call it directly.
    //

    return InternalDisassociateColorProfileFromDevice(pMachineName,
                pProfileName, pDeviceName);
}


#else                           // Windows 95 versions

BOOL WINAPI
DisassociateColorProfileFromDeviceA(
    PCSTR pMachineName,
    PCSTR pProfileName,
    PCSTR pDeviceName
    )
{
    TRACEAPI((__TEXT("DisassociateColorProfileWithDeviceA\n")));

    //
    // Internal function is ANSI in Windows 95, call it directly.
    //

    return InternalDisassociateColorProfileFromDevice(pMachineName,
                pProfileName, pDeviceName);
}


BOOL WINAPI
DisassociateColorProfileFromDeviceW(
    PCWSTR pMachineName,
    PCWSTR pProfileName,
    PCWSTR pDeviceName
    )
{
    PSTR  pszMachineName = NULL;    // Ansi machine name
    PSTR  pszProfileName = NULL;    // Ansi profile name
    PSTR  pszDeviceName = NULL;     // Ansi device name
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("DisassociateColorProfileWithDeviceW\n")));

    //
    // Validate parameters before we touch them
    //

    if (! pProfileName ||
        ! pDeviceName)
    {
        WARNING((__TEXT("Invalid parameter to AssociateColorProfileWithDevice\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Convert machine name to Ansi
    //

    if (pMachineName)
    {
        rc = ConvertToAnsi(pMachineName, &pszMachineName, TRUE);
    }
    else
        pszMachineName = NULL;

    //
    // Convert profile name to Ansi
    //

    rc = rc && ConvertToAnsi(pProfileName, &pszProfileName, TRUE);

    //
    // Convert device name to Ansi
    //

    rc = rc && ConvertToAnsi(pDeviceName, &pszDeviceName, TRUE);

    //
    // Call the internal Ansi function
    //

    rc = rc && InternalDisassociateColorProfileFromDevice(pszMachineName,
                pszProfileName, pszDeviceName);

    //
    // Free memory before leaving
    //

    if (pszProfileName)
    {
        MemFree(pszProfileName);
    }

    if (pszMachineName)
    {
        MemFree(pszMachineName);
    }

    if (pszDeviceName)
    {
        MemFree(pszDeviceName);
    }

    return rc;
}

#endif                          // ! UNICODE



/******************************************************************************
 *
 *                               EnumColorProfiles
 *
 *  Function:
 *       These are the ANSI & Unicode wrappers for InternalEnumColorProfile.
 *       Please see InternalEnumColorProfile for more details on this
 *       function.
 *
 *  Arguments:
 *       pMachineName    - name identifying machine on which the enumeration
 *                         needs to be done
 *       pEnumRecord     - pointer to enumeration criteria
 *       pBuffer         - pointer to buffer to receive result, can be NULL
 *       pdwSize         - pointer to buffer size. On return it is actual number
 *                         of bytes copied/needed.
 *       pnProfiles      - pointer to DWORD. On return, it is number of profiles
 *                         copied to pBuffer.
 *
 *  Returns:
 *       TRUE if successful, NULL otherwise
 *
 ******************************************************************************/

#ifdef UNICODE          // Windows NT versions

BOOL WINAPI
EnumColorProfilesA(
    PCSTR      pMachineName,
    PENUMTYPEA pEnumRecord,
    PBYTE      pBuffer,
    PDWORD     pdwSize,
    PDWORD     pnProfiles
    )
{
    PWSTR pwszMachineName = NULL;   // Unicode machine name
    PWSTR pwszDeviceName = NULL;    // Unicode device name
    PSTR  pAnsiDeviceName = NULL;   // incoming Ansi device name
    PWSTR pwBuffer = NULL;          // buffer to receive data
    PWSTR pwTempBuffer = NULL;      // temporary pointer to buffer
    DWORD dwSize;                   // size of buffer
    DWORD dwSizeOfStruct;           // size of ENUMTYPE structure
    DWORD dwVersion;                // ENUMTYPE structure version
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("EnumColorProfilesA\n")));

    //
    // Validate parameters before we touch them
    //

    if (! pdwSize ||
        IsBadWritePtr(pdwSize, sizeof(DWORD)) ||
        (pBuffer && IsBadWritePtr(pBuffer, *pdwSize)) ||
        ! pEnumRecord ||
        IsBadReadPtr(pEnumRecord, sizeof(DWORD)*3))   // probe until ENUMTYPE.dwFields
    {
ParameterError_EnumColorProfilesA:
        WARNING((__TEXT("Invalid parameter to EnumColorProfiles\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Check structure size based on its version.
    //

    dwSizeOfStruct = pEnumRecord->dwSize;
    dwVersion      = pEnumRecord->dwVersion;

    if (dwVersion >= ENUM_TYPE_VERSION)
    {
        if (dwSizeOfStruct < sizeof(ENUMTYPE))
            goto ParameterError_EnumColorProfilesA;
    }
    else if (dwVersion == 0x0200)
    {
        if (dwSizeOfStruct < sizeof(ENUMTYPE)-sizeof(DWORD))
            goto ParameterError_EnumColorProfilesA;

        //
        // Version 2 should not have ET_DEVICECLASS bit
        //

        if (pEnumRecord->dwFields & ET_DEVICECLASS)
            goto ParameterError_EnumColorProfilesA;

        WARNING((__TEXT("Old version ENUMTYPE to EnumColorProfiles\n")));
    }
    else
    {
        goto ParameterError_EnumColorProfilesA;
    }

    if (IsBadReadPtr(pEnumRecord, dwSizeOfStruct))
    {
        goto ParameterError_EnumColorProfilesA;
    }

    //
    // Convert machine name to Unicode
    //

    if (pMachineName)
    {
        rc = ConvertToUnicode(pMachineName, &pwszMachineName, TRUE);
    }
    else
        pwszMachineName = NULL;

    //
    // If device name is specified, convert it to Unicode
    //

    if (pEnumRecord->dwFields & ET_DEVICENAME)
    {
        if (! pEnumRecord->pDeviceName)
        {
            WARNING((__TEXT("Invalid parameter to EnumColorProfiles\n")));
            SetLastError(ERROR_INVALID_PARAMETER);
            rc = FALSE;
            goto EndEnumColorProfilesA;
        }

        //
        // Convert device name to Unicode
        //

        pAnsiDeviceName = (PSTR)pEnumRecord->pDeviceName;
        rc = rc && ConvertToUnicode(pAnsiDeviceName, &pwszDeviceName, TRUE);
        pEnumRecord->pDeviceName = (PSTR) pwszDeviceName;
    }

    dwSize = *pdwSize * sizeof(WCHAR);

    //
    // Allocate buffer of suitable size
    //

    if (pBuffer && dwSize)
    {
        pwBuffer = MemAlloc(dwSize);
        if (! pwBuffer)
        {
            WARNING((__TEXT("Error allocating memory for Unicode buffer\n")));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            rc = FALSE;
            goto EndEnumColorProfilesA;
        }
    }

    //
    // Call the internal Unicode function
    //

    rc = rc && InternalEnumColorProfiles(pwszMachineName,
                (PENUMTYPEW)pEnumRecord, (PBYTE)pwBuffer, &dwSize, pnProfiles);

    if (pwBuffer && rc)
    {
        pwTempBuffer = pwBuffer;
        while (*pwTempBuffer)
        {
            rc = rc && ConvertToAnsi(pwTempBuffer, (PSTR *)&pBuffer, FALSE);
            pwTempBuffer += lstrlenW(pwTempBuffer) + 1;
            pBuffer  += (lstrlenA((PSTR)pBuffer) + 1) * sizeof(char);
        }

        *((PSTR)pBuffer) = '\0';
    }

    *pdwSize = dwSize / sizeof(WCHAR);

EndEnumColorProfilesA:
    if (pwszMachineName)
    {
        MemFree(pwszMachineName);
    }
    if (pAnsiDeviceName)
    {
        ASSERT(pEnumRecord->pDeviceName != NULL);

        MemFree((PBYTE)pEnumRecord->pDeviceName);
        pEnumRecord->pDeviceName = (PCSTR)pAnsiDeviceName;
    }
    if (pwBuffer)
    {
        MemFree(pwBuffer);
    }

    return rc;
}


BOOL WINAPI
EnumColorProfilesW(
    PCWSTR     pMachineName,
    PENUMTYPEW pEnumRecord,
    PBYTE      pBuffer,
    PDWORD     pdwSize,
    PDWORD     pnProfiles
    )
{
    TRACEAPI((__TEXT("EnumColorProfilesW\n")));

    //
    // Internal function is Unicode in Windows NT, call it directly.
    //

    return InternalEnumColorProfiles(pMachineName, pEnumRecord,
        pBuffer, pdwSize, pnProfiles);
}

#else                           // Windows 95 versions

BOOL WINAPI
EnumColorProfilesA(
    PCSTR      pMachineName,
    PENUMTYPEA pEnumRecord,
    PBYTE      pBuffer,
    PDWORD     pdwSize,
    PDWORD     pnProfiles
    )
{
    TRACEAPI((__TEXT("EnumColorProfilesA\n")));

    //
    // Internal function is ANSI in Windows 95, call it directly.
    //

    return InternalEnumColorProfiles(pMachineName, pEnumRecord,
        pBuffer, pdwSize, pnProfiles);
}


BOOL WINAPI
EnumColorProfilesW(
    PCWSTR     pMachineName,
    PENUMTYPEW pEnumRecord,
    PBYTE      pBuffer,
    PDWORD     pdwSize,
    PDWORD     pnProfiles
    )
{
    PSTR  pszMachineName = NULL;    // Ansi machine name
    PSTR  pszDeviceName = NULL;     // Ansi device name
    PWSTR pUnicodeDeviceName = NULL;// incoming Unicode device name
    PSTR  pszBuffer = NULL;         // buffer to receive data
    PSTR  pszTempBuffer = NULL;     // temporary pointer to buffer
    DWORD dwSize;                   // size of buffer
    DWORD dwSizeOfStruct;           // size of ENUMTYPE structure
    DWORD dwVersion;                // ENUMTYPE structure version
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("EnumColorProfilesW\n")));

    //
    // Validate parameters before we touch them
    //

    if (! pdwSize ||
        IsBadWritePtr(pdwSize, sizeof(DWORD)) ||
        (pBuffer && IsBadWritePtr(pBuffer, *pdwSize)) ||
        ! pEnumRecord ||
        IsBadReadPtr(pEnumRecord, sizeof(DWORD)*3))   // probe until ENUMTYPE.dwFields
    {
ParameterError_EnumColorProfilesW:
        WARNING((__TEXT("Invalid parameter to EnumColorProfiles\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Check structure size based on its version.
    //

    dwSizeOfStruct = pEnumRecord->dwSize;
    dwVersion      = pEnumRecord->dwVersion;

    if (dwVersion >= ENUM_TYPE_VERSION)
    {
        if (dwSizeOfStruct < sizeof(ENUMTYPE))
            goto ParameterError_EnumColorProfilesW;
    }
    else if (dwVersion == 0x0200)
    {
        if (dwSizeOfStruct < sizeof(ENUMTYPE)-sizeof(DWORD))
            goto ParameterError_EnumColorProfilesW;

        //
        // Version 2 should not have ET_DEVICECLASS bit
        //

        if (pEnumRecord->dwFields & ET_DEVICECLASS)
            goto ParameterError_EnumColorProfilesW;

        WARNING((__TEXT("Old version ENUMTYPE to EnumColorProfiles\n")));
    }
    else
    {
        goto ParameterError_EnumColorProfilesW;
    }

    if (IsBadReadPtr(pEnumRecord, dwSizeOfStruct))
    {
        goto ParameterError_EnumColorProfilesW;
    }

    //
    // Convert machine name to Ansi
    //

    if (pMachineName)
    {
        rc = ConvertToAnsi(pMachineName, &pszMachineName, TRUE);
    }

    //
    // If device name is specified, convert it to Unicode
    //

    if (pEnumRecord->dwFields & ET_DEVICENAME)
    {
        if (! pEnumRecord->pDeviceName)
        {
            WARNING((__TEXT("Invalid parameter to EnumColorProfiles\n")));
            SetLastError(ERROR_INVALID_PARAMETER);
            goto EndEnumColorProfilesW;
        }

        //
        // Convert device name to Ansi
        //

        pUnicodeDeviceName = (PWSTR)pEnumRecord->pDeviceName;
        rc = rc && ConvertToAnsi(pUnicodeDeviceName, &pszDeviceName, TRUE);
        pEnumRecord->pDeviceName = (PCWSTR) pszDeviceName;
    }

    dwSize = *pdwSize / sizeof(WCHAR);

    //
    // Allocate buffer of suitable size
    //

    if (pBuffer && dwSize)
    {
        pszBuffer = MemAlloc(dwSize);
        if (! pszBuffer)
        {
            WARNING((__TEXT("Error allocating memory for Ansi buffer\n")));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            rc = FALSE;
            goto EndEnumColorProfilesW;
        }
    }

    //
    // Call the internal Ansi function
    //

    rc = rc && InternalEnumColorProfiles(pszMachineName,
                (PENUMTYPEA)pEnumRecord, (PBYTE)pszBuffer, &dwSize, pnProfiles);

    if (pszBuffer && rc)
    {
        pszTempBuffer = pszBuffer;
        while (*pszTempBuffer)
        {
            rc = rc && ConvertToUnicode(pszTempBuffer, (PWSTR *)&pBuffer, FALSE);
            pszTempBuffer += lstrlenA(pszTempBuffer) + 1;
            pBuffer   += (lstrlenW((PWSTR)pBuffer) + 1) * sizeof(WCHAR);
        }

        *((PWSTR)pBuffer) = '\0';
    }
    *pdwSize = dwSize * sizeof(WCHAR);

EndEnumColorProfilesW:
    if (pszMachineName)
    {
        MemFree(pszMachineName);
    }
    if (pUnicodeDeviceName)
    {
        ASSERT(pEnumRecord->pDeviceName != NULL);

        MemFree((PSTR)pEnumRecord->pDeviceName);
        pEnumRecord->pDeviceName = (PCWSTR)pUnicodeDeviceName;
    }
    if (pszBuffer)
    {
        MemFree(pszBuffer);
    }

    return rc;
}

#endif                          // ! UNICODE


/******************************************************************************
 *
 *                          SetStandardColorSpaceProfile
 *
 *  Function:
 *       These are the ANSI & Unicode wrappers for InternalSetSCSProfile.
 *       Please see InternalSetSCSProfile for more details on this
 *       function.
 *
 *  Arguments:
 *       pMachineName    - name identifying machine on which the standard color
 *                         space profile should be registered
 *       dwSCS           - ID for the standard color space
 *       pProfileName    - pointer to profile filename
 *
 *  Returns:
 *       TRUE if successful, NULL otherwise
 *
 ******************************************************************************/

#ifdef UNICODE          // Windows NT versions

BOOL  WINAPI
SetStandardColorSpaceProfileA(
    PCSTR   pMachineName,
    DWORD   dwSCS,
    PCSTR   pProfileName
    )
{
    PWSTR pwszMachineName = NULL;   // Unicode machine name
    PWSTR pwszProfileName = NULL;   // Unicode profile name
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("SetStandardColorSpaceProfileA\n")));

    //
    // Validate parameters before we touch them
    //

    if (! pProfileName)
    {
        WARNING((__TEXT("Invalid parameter to SetStandardColorSpaceProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Convert machine name to Unicode
    //

    if (pMachineName)
    {
        rc = ConvertToUnicode(pMachineName, &pwszMachineName, TRUE);
    }
    else
        pwszMachineName = NULL;

    //
    // Convert profile name to Unicode
    //

    rc = rc && ConvertToUnicode(pProfileName, &pwszProfileName, TRUE);

    //
    // Call the internal Unicode function
    //

    rc = rc && InternalSetSCSProfile(pwszMachineName, dwSCS, pwszProfileName);

    //
    // Free memory before leaving
    //

    if (pwszProfileName)
    {
        MemFree(pwszProfileName);
    }

    if (pwszMachineName)
    {
        MemFree(pwszMachineName);
    }

    return rc;
}

BOOL  WINAPI
SetStandardColorSpaceProfileW(
    PCWSTR   pMachineName,
    DWORD    dwSCS,
    PCWSTR   pProfileName
    )
{
    TRACEAPI((__TEXT("SetStandardColorSpaceProfileW\n")));

    //
    // Internal function is Unicode in Windows NT, call it directly.
    //

    return InternalSetSCSProfile(pMachineName, dwSCS, pProfileName);
}

#else                           // Windows 95 versions

BOOL  WINAPI
SetStandardColorSpaceProfileA(
    PCSTR   pMachineName,
    DWORD   dwSCS,
    PCSTR   pProfileName
    )
{
    TRACEAPI((__TEXT("SetStandardColorSpaceProfileA\n")));

    //
    // Internal function is ANSI in Windows 95, call it directly.
    //

    return InternalSetSCSProfile(pMachineName, dwSCS, pProfileName);
}

BOOL  WINAPI
SetStandardColorSpaceProfileW(
    PCWSTR   pMachineName,
    DWORD    dwSCS,
    PCWSTR   pProfileName
    )
{
    PSTR  pszMachineName = NULL;    // Ansi machine name
    PSTR  pszProfileName = NULL;    // Ansi profile name
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("SetStandardColorSpaceProfileW\n")));

    //
    // Validate parameters before we touch them
    //

    if (! pProfileName)
    {
        WARNING((__TEXT("Invalid parameter to SetStandardColorSpaceProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Convert machine name to Ansi
    //

    if (pMachineName)
    {
        rc = ConvertToAnsi(pMachineName, &pszMachineName, TRUE);
    }
    else
        pszMachineName = NULL;

    //
    // Convert profile name to Ansi
    //

    rc = rc && ConvertToAnsi(pProfileName, &pszProfileName, TRUE);

    //
    // Call the internal Ansi function
    //

    rc = rc && InternalSetSCSProfile(pszMachineName, dwSCS, pszProfileName);

    //
    // Free memory before leaving
    //

    if (pszProfileName)
    {
        MemFree(pszProfileName);
    }

    if (pszMachineName)
    {
        MemFree(pszMachineName);
    }

    return rc;
}

#endif                          // ! UNICODE


/******************************************************************************
 *
 *                          GetStandardColorSpaceProfile
 *
 *  Function:
 *       These are the ANSI & Unicode wrappers for InternalGetSCSProfile.
 *       Please see InternalGetSCSProfile for more details on this
 *       function.
 *
 *  Arguments:
 *       pMachineName    - name identifying machine on which the standard color
 *                         space profile should be queried
 *       dwSCS           - ID for the standard color space
 *       pBuffer         - pointer to buffer to receive profile filename
 *       pdwSize         - pointer to DWORD specifying size of buffer. On return
 *                         it has size needed/used
 *
 *  Returns:
 *       TRUE if successful, NULL otherwise
 *
 ******************************************************************************/

#ifdef UNICODE          // Windows NT versions

BOOL  WINAPI
GetStandardColorSpaceProfileA(
    PCSTR   pMachineName,
    DWORD   dwSCS,
    PSTR    pBuffer,
    PDWORD  pdwSize
    )
{
    PWSTR pwszMachineName = NULL;   // Unicode machine name
    PWSTR pwBuffer = NULL;          // Unicode color directory path
    DWORD dwSize;                   // size of Unicode buffer
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("GetStandardColorSpaceProfileA\n")));

    //
    // Validate parameters before we touch them
    //

    if (! pdwSize ||
        IsBadWritePtr(pdwSize, sizeof(DWORD)) ||
        (pBuffer && IsBadWritePtr(pBuffer, *pdwSize)))
    {
        WARNING((__TEXT("Invalid parameter to GetStandardColorSpaceProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Convert machine name to Unicode
    //

    if (pMachineName)
    {
        rc = ConvertToUnicode(pMachineName, &pwszMachineName, TRUE);
    }
    else
        pwszMachineName = NULL;

    dwSize = *pdwSize * sizeof(WCHAR);

    //
    // Create a buffer to get Unicode filename from system
    //

    if (pBuffer && dwSize)
    {
        pwBuffer = (PWSTR)MemAlloc(dwSize);
        if (! pwBuffer)
        {
            WARNING((__TEXT("Error allocating memory for Unicode string\n")));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            rc = FALSE;
            goto EndGetSCSProfileA;
        }
    }

    rc = rc && InternalGetSCSProfile(pwszMachineName, dwSCS, pwBuffer, &dwSize);

    *pdwSize = dwSize / sizeof(WCHAR);

    //
    // Convert Unicode path to Ansi
    //

    if (pwBuffer)
    {
        rc = rc && ConvertToAnsi(pwBuffer, &pBuffer, FALSE);
    }

EndGetSCSProfileA:
    if (pwszMachineName)
    {
        MemFree(pwszMachineName);
    }

    if (pwBuffer)
    {
        MemFree(pwBuffer);
    }

    return rc;
}

BOOL  WINAPI
GetStandardColorSpaceProfileW(
    PCWSTR   pMachineName,
    DWORD    dwSCS,
    PWSTR    pBuffer,
    PDWORD   pdwSize
    )
{
    TRACEAPI((__TEXT("GetStandardColorSpaceProfileW\n")));

    //
    // Internal function is Unicode in Windows NT, call it directly.
    //

    return InternalGetSCSProfile(pMachineName, dwSCS, pBuffer, pdwSize);
}

#else                           // Windows 95 versions

BOOL  WINAPI
GetStandardColorSpaceProfileA(
    PCSTR   pMachineName,
    DWORD   dwSCS,
    PSTR    pBuffer,
    PDWORD  pdwSize
    )
{
    TRACEAPI((__TEXT("GetStandardColorSpaceProfileA\n")));

    //
    // Internal function is ANSI in Windows 95, call it directly.
    //

    return InternalGetSCSProfile(pMachineName, dwSCS, pBuffer, pdwSize);
}

BOOL  WINAPI
GetStandardColorSpaceProfileW(
    PCWSTR   pMachineName,
    DWORD    dwSCS,
    PWSTR    pBuffer,
    PDWORD   pdwSize
    )
{
    PSTR pszMachineName = NULL;     // Ansi machine name
    PSTR pszBuffer = NULL;          // Ansi color directory path
    DWORD dwSize;                   // size of Ansi buffer
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("GetStandardColorSpaceProfileW\n")));

    //
    // Validate parameters before we touch them
    //

    if (! pdwSize ||
        IsBadWritePtr(pdwSize, sizeof(DWORD)) ||
        (pBuffer && IsBadWritePtr(pBuffer, *pdwSize)))
    {
        WARNING((__TEXT("Invalid parameter to GetStandardColorSpaceProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Convert machine name to Ansi
    //

    if (pMachineName)
    {
        rc = ConvertToAnsi(pMachineName, &pszMachineName, TRUE);
    }
    else
        pszMachineName = NULL;

    dwSize = *pdwSize / sizeof(WCHAR);

    //
    // Create a buffer to get Ansi profilename from system
    //

    if (pBuffer && dwSize)
    {
        pszBuffer = (PSTR)MemAlloc(dwSize);
        if (! pBuffer)
        {
            WARNING((__TEXT("Error allocating memory for Ansi string\n")));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            rc = FALSE;
            goto EndGetSCSProfileW;
        }
    }

    rc = rc && InternalGetSCSProfile(pszMachineName, dwSCS, pszBuffer, &dwSize);

    *pdwSize = dwSize * sizeof(WCHAR);

    //
    // Convert Ansi path to Unicode
    //

    if (pszBuffer)
    {
        rc = rc && ConvertToUnicode(pszBuffer, &pBuffer, FALSE);
    }

EndGetSCSProfileW:
    if (pszMachineName)
    {
        MemFree(pszMachineName);
    }

    if (pszBuffer)
    {
        MemFree(pszBuffer);
    }

    return rc;
}

#endif                          // ! UNICODE


/******************************************************************************
 *
 *                          GenerateCopyFilePaths
 *
 *  Function:
 *       This function is called by the Windows NT spooler to find the
 *       directories from which color profiles should be picked up and copied
 *       to. This is useful if the locations are version or  processor
 *       architecture dependent. As color profiles depend on neither, we don't
 *       have to do anything, but have to export this function.
 *
 *  Arguments:
 *       don't care
 *
 *  Returns:
 *       ERROR_SUCCESS if successful, error code otherwise
 *
 ******************************************************************************/

DWORD WINAPI
GenerateCopyFilePaths(
    LPCWSTR     pszPrinterName,
    LPCWSTR     pszDirectory,
    LPBYTE      pSplClientInfo,
    DWORD       dwLevel,
    LPWSTR      pszSourceDir,
    LPDWORD     pcchSourceDirSize,
    LPWSTR      pszTargetDir,
    LPDWORD     pcchTargetDirSize,
    DWORD       dwFlags
    )
{
    TRACEAPI((__TEXT("GenerateCopyFilePaths\n")));
    return ERROR_SUCCESS;
}


/******************************************************************************
 *
 *                          SpoolerCopyFileEvent
 *
 *  Function:
 *       This function is called by the Windows NT spooler when one of the
 *       following events happens:
 *          1. When someone does a SetPrinterDataEx of the CopyFiles section
 *          2. When a printer connection is made
 *          3. When files for a printer connection get updated
 *          4. When a printer is deleted
 *
 *  Arguments:
 *       pszPrinterName  - friendly name of printer
 *       pszKey          - "CopyFiles\ICM" for us
 *       dwCopyFileEvent - reason for calling
 *
 *  Returns:
 *       TRUE if successful, NULL otherwise
 *
 ******************************************************************************/

BOOL WINAPI
SpoolerCopyFileEvent(
    LPWSTR  pszPrinterName,
    LPWSTR  pszKey,
    DWORD   dwCopyFileEvent
    )
{
    PTSTR pProfileList, pTemp, pBuffer;
    DWORD dwSize;
    BOOL  bRc = FALSE;
    TCHAR szPath[MAX_PATH];

    TRACEAPI((__TEXT("SpoolerCopyFileEvent\n")));

    switch (dwCopyFileEvent)
    {
    case COPYFILE_EVENT_SET_PRINTER_DATAEX:

        //
        // When associating profiles with printer connections, we copy
        // the files to the remote machine, and then do a SePrinterDataEx.
        // This causes this event to be generated on the remote machine. We
        // use this to install the profile. This is not needed after we make
        // our APIs remotable
        //
        // Fall thru'
        //

        TERSE((__TEXT("SetPrinterDataEx event\n")));

    case COPYFILE_EVENT_ADD_PRINTER_CONNECTION:
    case COPYFILE_EVENT_FILES_CHANGED:

        //
        // This event is generated when a printer connection is added or
        // associated profiles have changed. Install all the profiles in
        // the client machine now.
        //

        #ifdef DBG
        if (dwCopyFileEvent == COPYFILE_EVENT_ADD_PRINTER_CONNECTION)
        {
            WARNING((__TEXT("AddPrinterConnection Event\n")));
        }
        else  if (dwCopyFileEvent == COPYFILE_EVENT_FILES_CHANGED)
        {
            WARNING((__TEXT("FilesChanged Event\n")));
        }
        #endif

        dwSize = 0;
        if (GetDeviceData((PTSTR)pszPrinterName, CLASS_PRINTER, DEVICE_PROFILE_DATA,
                          (PVOID *)&pProfileList, &dwSize, TRUE))
        {
            dwSize = sizeof(szPath);
            if (InternalGetColorDirectory(NULL, szPath, &dwSize))
            {
                lstrcat(szPath, gszBackslash);
                pBuffer = szPath + lstrlen(szPath);
                pTemp = pProfileList;
                while (*pTemp)
                {
                    lstrcpy(pBuffer, pTemp);
                    InstallColorProfile(NULL, szPath);
                    pTemp += lstrlen(pTemp) + 1;
                }
            }

            MemFree(pProfileList);
        }
        break;

    case COPYFILE_EVENT_DELETE_PRINTER:

        //
        // This event is generated when a printer is about to be deleted.
        // Get all profiles associated with the printer and disassociate
        // them now.
        //

        TERSE((__TEXT("DeletePrinterDataEx Event\n")));

        dwSize = 0;
        if (GetDeviceData((PTSTR)pszPrinterName, CLASS_PRINTER, DEVICE_PROFILE_DATA,
                          (PVOID *)&pProfileList, &dwSize, TRUE))
        {
            pTemp = pProfileList;
            while (*pTemp)
            {
                DisassociateColorProfileFromDevice(NULL, pTemp, (PTSTR)pszPrinterName);
                pTemp += lstrlen(pTemp) + 1;
            }

            MemFree(pProfileList);
        }
        break;
    }

    bRc = TRUE;

    return bRc;
}


/*****************************************************************************/
/***************************** Internal Functions ****************************/
/*****************************************************************************/

/******************************************************************************
 *
 *                         InternalGetColorDirectory
 *
 *  Function:
 *       This function returns the path to the color directory on the machine
 *       specified.
 *       associations should be removed before calling this function. It also
 *       optionally deletes the file if the profile was successfully
 *       uninstalled.
 *
 *  Arguments:
 *       pMachineName    - name identifying machine on which the path
 *                         to the color directory is requested
 *       pBuffer         - pointer to buffer to receive pathname
 *       pdwSize         - pointer to size of buffer. On return it has size of
 *                         buffer needed if failure, and used on success
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
InternalGetColorDirectory(
    LPCTSTR  pMachineName,
    PTSTR    pBuffer,
    DWORD   *pdwSize
    )
{
    DWORD dwBufLen = *pdwSize;      // size supplied
    BOOL  rc = FALSE;               // return value

    //
    // Get the printer driver directory
    //

#if !defined(_WIN95_)

    DWORD dwNeeded;                 // size needed

    if (!pBuffer && pdwSize && !IsBadWritePtr(pdwSize, sizeof(DWORD)))
    {
        *pdwSize = 0;
    }

    if (GetPrinterDriverDirectory((PTSTR)pMachineName, NULL, 1, (PBYTE)pBuffer,
        *pdwSize, pdwSize))
    {
        //
        // This API returns the print$ path appended with the environment
        // directory. e.g. c:\winnt\system32\spool\drivers\w32x86. So we need
        // to go back one step and then append the color directory.
        //

        PWSTR pDriverDir;

        pDriverDir = GetFilenameFromPath(pBuffer);

        ASSERT (pDriverDir != NULL);

        *pdwSize -= lstrlen(pDriverDir) * sizeof(WCHAR);

        *pDriverDir = '\0';

        //
        // Calculate size of buffer needed to append color directory
        //

        dwNeeded = *pdwSize + lstrlen(gszColorDir) * sizeof(WCHAR);
        if (pBuffer[lstrlen(pBuffer) - 1] != '\\')
        {
            dwNeeded += sizeof(WCHAR);
        }

        //
        // Update size needed
        //

        *pdwSize = dwNeeded;

        //
        // If supplied buffer is big enough, append our stuff
        //

        if (dwNeeded <= dwBufLen)
        {
            if (pBuffer[lstrlen(pBuffer) - 1] != '\\')
            {
                lstrcat(pBuffer, gszBackslash);
            }

            lstrcat(pBuffer, gszColorDir);

            rc = TRUE;
        }
        else
        {
            WARNING((__TEXT("Input buffer to GetColorDirectory not big enough\n")));
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    }
    else if (GetLastError() == ERROR_INVALID_USER_BUFFER)
    {
        //
        // Spooler sets this error if buffer is NULL. Map it to our error
        //

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }
    else if (GetLastError() == RPC_S_SERVER_UNAVAILABLE)
    {
        TCHAR achTempPath[MAX_PATH * 2]; // Make sure enough path space.

        //
        // Spooler service is not running. Use hardcoded path
        //

        if (GetSystemDirectory(achTempPath,MAX_PATH) != 0)
        {
            _tcscat(achTempPath,TEXT("\\spool\\drivers\\color"));

            *pdwSize = wcslen(achTempPath) + 1;

            if (pBuffer && (*pdwSize <= dwBufLen))
            {
                _tcscpy(pBuffer,achTempPath);

                rc = TRUE;
            }
            else
            {
                WARNING((__TEXT("Input buffer to GetColorDirectory not big enough\n")));
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
            }
        }
    }

#else

    HKEY  hkSetup;                  // registry key
    DWORD dwErr;                    // error code

    //
    // Only local color directory query is allowed in Memphis
    //

    if (pMachineName)
    {
        WARNING((__TEXT("Remote color directory query, failing...\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // On Memphis, get this information from the setup section in the registry.
    // The reason we don't call GetPrinterDriverDirectory is that when we call
    // this function from GDI 16, it tries to go back into 16-bit mode and
    // deadlocks on the Win16 lock.
    //

    if ((dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, gszSetupPath, &hkSetup)) == ERROR_SUCCESS)
    {
        if ((dwErr = RegQueryValueEx(hkSetup, gszICMDir, 0, NULL, (PBYTE)pBuffer,
                pdwSize)) == ERROR_SUCCESS)
        {
            rc = TRUE;
        }
        RegCloseKey(hkSetup);
    }

    if (!rc)
    {
        //
        // Make error codes consistent
        //

        if (dwErr == ERROR_MORE_DATA)
        {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
        }

        WARNING((__TEXT("Error getting color directory: %d\n"), dwErr));
        SetLastError(dwErr);
    }

    //
    // RegQueryValueEx returns TRUE even if the calling buffer is NULL. Our API
    // is supposed to return FALSE. Check for this case.
    //

    if (pBuffer == NULL && rc)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        rc = FALSE;
    }

#endif // !defined(_WIN95_)

    return rc;
}


/******************************************************************************
 *
 *                         InternalInstallColorProfile
 *
 *  Function:
 *       This function installs a given color profile on a a given machine.
 *
 *  Arguments:
 *       pMachineName    - name identifying machine on which the profile
 *                         should be uninstalled. NULL implies local
 *       pProfileName    - Fully qualified pathname of profile to uninstall
 *       bDelete         - TRUE if profile should be deleted in disk
 *
 *  Returns:
 *       TRUE if successful, NULL otherwise
 *
 *  Warning:
 *       Currently only local install is supported, so pMachineName should
 *       be NULL.
 *
 ******************************************************************************/

BOOL
InternalInstallColorProfile(
    LPCTSTR   pMachineName,
    LPCTSTR   pProfileName
    )
{
    PROFILEHEADER header;           // profile header
    REGDATA  regData;               // for storing registry data about profile
    HKEY     hkICM = NULL;          // key to ICM branch in registry
    HKEY     hkDevice = NULL;       // key to ICM device branch in registry
    DWORD    dwSize;                // size of registry data for profile
    DWORD    dwErr;                 // error code
    BOOL     rc = FALSE;            // return code
    PTSTR    pFilename;             // profile name without path
    TCHAR    szDest[MAX_PATH];      // destination path for profile
    TCHAR    szClass[5];            // profile class
    BOOL     FileExist;             // profile already in directory?
    BOOL     RegExist;              // profile in registry?
    
    //
    // Validate parameters
    //
    
    if (!pProfileName)
    {
        WARNING((__TEXT("Invalid parameter to InstallColorProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Only local installs are allowed now
    //

    if (pMachineName)
    {
        WARNING((__TEXT("Remote install attempted, failing...\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Get rid of the directory path and get a pointer to the filename
    //

    pFilename = GetFilenameFromPath((PTSTR)pProfileName);
    if (! pFilename)
    {
        WARNING((__TEXT("Could not parse file name from profile path %s\n"), pProfileName));
        SetLastError(ERROR_INVALID_PARAMETER);
        goto EndInstallColorProfile;
    }

    //
    // Get the profile class in the form of a string
    //

    if (! GetProfileClassString(pProfileName, szClass, &header))
    {
        WARNING((__TEXT("Installing invalid profile %s\n"), pProfileName));
        SetLastError(ERROR_INVALID_PROFILE);
        goto EndInstallColorProfile;
    }

    //
    // Open the registry path where profiles are kept
    //

    if (((dwErr = RegCreateKey(HKEY_LOCAL_MACHINE, gszICMRegPath, &hkICM)) != ERROR_SUCCESS) ||
        ((dwErr = RegCreateKey(hkICM, szClass, &hkDevice)) != ERROR_SUCCESS))
    {
        WARNING((__TEXT("Cannot open ICM\\device branch of registry: %d\n"), dwErr));
        SetLastError(dwErr);
        goto EndInstallColorProfile;
    }


    //
    // If registry data exists && the profile is in the directory,  then the profile is already installed,
    // in which case, return success. Otherwise, copy profile to color
    // directory (if it's not already there) and add it to the registry (if it's not already there).
    //

    dwSize = sizeof(szDest);
        
    //
    // Copy the file to the color directory
    //

    if (! InternalGetColorDirectory(NULL, szDest, &dwSize))
    {
        WARNING((__TEXT("Could not get color directory\n")));
        goto EndInstallColorProfile;
    }

    //
    // This creates the directory if it doesn't exist, doesn't do anything
    // if it already exists
    //

    CreateDirectory(szDest, NULL);

    if (szDest[lstrlen(szDest) - 1] != '\\')
    {
        lstrcat(szDest, gszBackslash);
    }
    lstrcat(szDest, pFilename);

    //
    // If the profile is already in the color directory, do not attempt
    // to copy it again; it will fail.
    //
        
    dwSize = sizeof(REGDATA);
    
    FileExist = GetFileAttributes(szDest) != (DWORD)-1;
    RegExist = RegQueryValueEx(hkDevice, pFilename, 0, NULL, (PBYTE)&regData, &dwSize) == ERROR_SUCCESS;

    //
    // If the file does exist, short circuit the CopyFile 
    // and go on to add it into the registry.
    //

    if (!FileExist && !CopyFile(pProfileName, szDest, FALSE))
    {
        WARNING((__TEXT("Could not copy profile %s to color directory\n"), pProfileName));
        goto EndInstallColorProfile;
    }

    //
    // Add profile to the registry
    //

    if(!RegExist) 
    {
        regData.dwRefCount = 0;
        regData.dwManuID = FIX_ENDIAN(header.phManufacturer);
        regData.dwModelID = FIX_ENDIAN(header.phModel);
        if ((dwErr = RegSetValueEx(hkDevice, pFilename, 0, REG_BINARY,
                  (PBYTE)&regData, sizeof(REGDATA))) != ERROR_SUCCESS)
        {
            WARNING((__TEXT("Could not set registry data to install profile %s: %d\n"), pFilename, dwErr));
            SetLastError(dwErr);
            goto EndInstallColorProfile;
        }
    }
    
    rc = TRUE;              // Everything went well!

EndInstallColorProfile:    
    if (hkICM)
    {
        RegCloseKey(hkICM);
    }
    if (hkDevice)
    {
        RegCloseKey(hkDevice);
    }

    return rc;
}


/******************************************************************************
 *
 *                         InternalUninstallColorProfile
 *
 *  Function:
 *       This function uninstalls a given color profile on a a given machine.
 *       It fails if the color profile is associated with any device, so all
 *       associations should be removed before calling this function. It also
 *       optionally deletes the file if the profile was successfully
 *       uninstalled.
 *
 *  Arguments:
 *       pMachineName    - name identifying machine on which the profile
 *                         should be uninstalled. NULL implies local
 *       pProfileName    - pointer to profile to uninstall
 *       bDelete         - TRUE if profile should be deleted in disk
 *
 *  Returns:
 *       TRUE if successful, NULL otherwise
 *
 *  Warning:
 *       Currently only local uninstall is supported, so pMachineName should
 *       be NULL.
 *
 ******************************************************************************/

BOOL
InternalUninstallColorProfile(
    LPCTSTR pMachineName,
    LPCTSTR pProfileName,
    BOOL    bDelete
    )
{
    REGDATA  regData;               // for storing registry data about profile
    HKEY     hkICM = NULL;          // key to ICM branch in registry
    HKEY     hkDevice = NULL;       // key to ICM device branch in registry
    DWORD    dwSize;                // size of registry data for profile
    DWORD    dwErr;                 // error code
    BOOL     rc = FALSE;            // return code
    PTSTR    pFilename;             // profile name without path
    TCHAR    szColorPath[MAX_PATH]; // full path name of profile
    TCHAR    szClass[5];            // profile class

    //
    // Validate parameters
    //

    if (!pProfileName)
    {
        WARNING((__TEXT("Invalid parameter to UninstallColorProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Only local installs are allowed now
    //

    if (pMachineName != NULL)
    {
        WARNING((__TEXT("Remote uninstall attempted, failing...\n")));
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    //
    // Get rid of the directory path and get a pointer to the filename
    //

    pFilename = GetFilenameFromPath((PTSTR)pProfileName);
    if (! pFilename)
    {
        WARNING((__TEXT("Could not parse file name from profile path\n"), pProfileName));
        SetLastError(ERROR_INVALID_PARAMETER);
        goto EndUninstallColorProfile;
    }

    //
    // Create a fully qualified path name
    //

    dwSize = sizeof(szColorPath);
    if (! InternalGetColorDirectory(NULL, szColorPath, &dwSize))
    {
        WARNING((__TEXT("Could not get color directory\n")));
        goto EndUninstallColorProfile;
    }

    if (szColorPath[lstrlen(szColorPath) - 1] != '\\')
    {
        lstrcat(szColorPath, gszBackslash);
    }
    lstrcat(szColorPath, pFilename);

    //
    // Get the profile class in the form of a string
    //

    if (! GetProfileClassString(szColorPath, szClass, NULL))
    {
        WARNING((__TEXT("Installing invalid profile\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        goto EndUninstallColorProfile;
    }

    //
    // Open the registry path where profiles are kept
    //

    if (((dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, gszICMRegPath, &hkICM)) != ERROR_SUCCESS) ||
        ((dwErr = RegOpenKey(hkICM, szClass, &hkDevice)) != ERROR_SUCCESS))
    {
        WARNING((__TEXT("Cannot open ICM\\device branch of registry: %d\n"), dwErr));
        SetLastError(dwErr);
        goto EndUninstallColorProfile;
    }

    //
    // Check if reference count is zero and remove value from registry
    //

    dwSize = sizeof(REGDATA);
    if ((dwErr = RegQueryValueEx(hkDevice, pFilename, 0, NULL, (PBYTE)&regData,
            &dwSize)) != ERROR_SUCCESS)
    {
        WARNING((__TEXT("Trying to uninstall a profile that is not installed %s: %d\n"), pFilename, dwErr));
        SetLastError(dwErr);
        goto EndUninstallColorProfile;
    }

    if (regData.dwRefCount != 0)
    {
        WARNING((__TEXT("Trying to uninstall profile %s whose refcount is %d\n"),
            pFilename, regData.dwRefCount));
        goto EndUninstallColorProfile;
    }

    if ((dwErr = RegDeleteValue(hkDevice, pFilename)) != ERROR_SUCCESS)
    {
        WARNING((__TEXT("Error deleting profile %s from registry: %d\n"), pFilename, dwErr));
        SetLastError(dwErr);
        goto EndUninstallColorProfile;
    }

    //
    // Remove profile from the registry
    //

    if (bDelete)
    {
        //
        // Delete profile from the color directory
        //

        if (! DeleteFile(szColorPath))
        {
            WARNING((__TEXT("Error deleting profile %s: %d\n"), szColorPath, GetLastError()));
            goto EndUninstallColorProfile;
        }
    }

    rc = TRUE;              // Everything went well!

EndUninstallColorProfile:
    if (hkICM)
    {
        RegCloseKey(hkICM);
    }
    if (hkDevice)
    {
        RegCloseKey(hkDevice);
    }

    return rc;
}


/******************************************************************************
 *
 *                    InternalAssociateColorProfileWithDevice
 *
 *  Function:
 *       This function associates a color profile on a a given machine with a
 *       particular device. It fails if the color profile is not installed on
 *       the machine. It increases the usage reference count of the profile.
 *
 *  Arguments:
 *       pMachineName    - name identifying machine. NULL implies local
 *       pProfileName    - pointer to profile to associate
 *       pDeviceName     - pointer to device name
 *
 *  Returns:
 *       TRUE if successful, NULL otherwise
 *
 *  Warning:
 *       Currently only local association is supported, so pMachineName should
 *       be NULL.
 *
 ******************************************************************************/

BOOL
InternalAssociateColorProfileWithDevice(
    LPCTSTR pMachineName,
    LPCTSTR pProfileName,
    LPCTSTR pDeviceName
    )
{
    PROFILEHEADER header;           // profile header
    REGDATA  regData;               // for storing registry data about profile
    HKEY     hkICM = NULL;          // key to ICM branch in registry
    HKEY     hkDevice = NULL;       // key to ICM device branch in registry
    DWORD    dwSize;                // size of registry data
    DWORD    dwNewSize;             // new size of device registry data
    DWORD    dwErr;                 // error code
    BOOL     rc = FALSE;            // return code
    PTSTR    pFilename;             // profile name without path
    PTSTR    pProfileList = NULL;   // list of associated profiles
    TCHAR    szColorPath[MAX_PATH]; // full path name of profile
    TCHAR    szClass[5];            // profile class
    BOOL     bFirstProfile = FALSE; // First profile to be associated for device
    DWORD    dwDeviceClass;         // device class

    //
    // Validate parameters
    //

    if (! pProfileName ||
        ! pDeviceName)
    {
        WARNING((__TEXT("Invalid parameter to AssociateColorProfileWithDevice\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Only local associations are allowed now
    //

    if (pMachineName != NULL)
    {
        WARNING((__TEXT("Remote profile association attempted, failing...\n")));
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    //
    // Get rid of the directory path and get a pointer to the filename
    //

    pFilename = GetFilenameFromPath((PTSTR)pProfileName);
    if (! pFilename)
    {
        WARNING((__TEXT("Could not parse file name from profile path %s\n"), pProfileName));
        SetLastError(ERROR_INVALID_PARAMETER);
        goto EndAssociateProfileWithDevice;
    }

    //
    // Create a fully qualified path name
    //

    dwSize = sizeof(szColorPath);
    if (! InternalGetColorDirectory(NULL, szColorPath, &dwSize))
    {
        WARNING((__TEXT("Could not get color directory\n")));
        goto EndAssociateProfileWithDevice;
    }

    if (szColorPath[lstrlen(szColorPath) - 1] != '\\')
    {
        lstrcat(szColorPath, gszBackslash);
    }
    lstrcat(szColorPath, pFilename);

    //
    // Get the profile class in the form of a string
    //

    if (! GetProfileClassString(szColorPath, szClass, &header))
    {
        WARNING((__TEXT("Installing invalid profile %s\n"), szColorPath));
        SetLastError(ERROR_INVALID_PROFILE);
        goto EndAssociateProfileWithDevice;
    }

    //
    // Open the registry path where profiles are kept
    //

    if (((dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, gszICMRegPath, &hkICM)) != ERROR_SUCCESS) ||
        ((dwErr = RegOpenKey(hkICM, szClass, &hkDevice)) != ERROR_SUCCESS))
    {
        WARNING((__TEXT("Cannot open ICM\\device branch of registry: %d\n"), dwErr));
        SetLastError(dwErr);
        goto EndAssociateProfileWithDevice;
    }

    //
    // Check if profile is installed
    //

    dwSize = sizeof(REGDATA);
    if ((dwErr = RegQueryValueEx(hkDevice, pFilename, 0, NULL, (PBYTE)&regData,
            &dwSize)) != ERROR_SUCCESS)
    {
        WARNING((__TEXT("Trying to associate a profile that is not installed %s: %d\n"), pFilename, dwErr));
        SetLastError(dwErr);
        goto EndAssociateProfileWithDevice;
    }

    //
    // Treat CLASS_MONITOR as CLASS_COLORSPACE.
    //
    if ((dwDeviceClass = header.phClass) == CLASS_MONITOR)
    {
        //
        // since CLASS_MONTOR profile can be associated any device class.
        //
        dwDeviceClass = CLASS_COLORSPACE;
    }

    //
    // Read in the list of profiles associated with the device
    //

    dwSize = 0;
    if (! GetDeviceData(pDeviceName, dwDeviceClass, DEVICE_PROFILE_DATA,
                        (PVOID *)&pProfileList, &dwSize, TRUE))
    {
        pProfileList = NULL;        // no data found
    }

    //
    // If the profile is already associated with the device, return success.
    // Do not check if we didn't get a profile list
    //

    if (pProfileList &&
        IsStringInMultiSz(pProfileList, pFilename) == TRUE)
    {
        rc = TRUE;
        goto EndAssociateProfileWithDevice;
    }

    if (dwSize <= sizeof(TCHAR))
    {
        bFirstProfile = TRUE;
    }

    //
    // Add new profile to the list of profiles, and double NULL
    // terminate the MULTI_SZ string.
    //

    if (dwSize > 0)
    {
        //
        // Use a temporary pointer, so that if MemReAlloc fails, we do
        // not have a memory leak - the original pointer needs to be freed
        //

        PTSTR pTemp;

        dwNewSize = dwSize + (lstrlen(pFilename) + 1) * sizeof(TCHAR);

        pTemp = MemReAlloc(pProfileList, dwNewSize);
        if (! pTemp)
        {
            WARNING((__TEXT("Error reallocating pProfileList\n")));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto EndAssociateProfileWithDevice;
        }
        else
            pProfileList = pTemp;
    }
    else
    {
        //
        // Allocate extra character for double NULL termination. Setting
        // dwSize to 1 accomplishes this and lets the lstrcpy below
        // to work correctly.
        //

        dwSize = sizeof(TCHAR);     // extra char for double NULL termination

        dwNewSize = dwSize + (lstrlen(pFilename) + 1) * sizeof(TCHAR);
        pProfileList = MemAlloc(dwNewSize);
        if (! pProfileList)
        {
            WARNING((__TEXT("Error allocating pProfileList\n")));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto EndAssociateProfileWithDevice;
        }
    }
    {
        PTSTR pPtr;                 // temporary pointer

        pPtr = (PTSTR)((PBYTE)pProfileList + dwSize - sizeof(TCHAR));
        lstrcpy(pPtr, pFilename);
        pPtr = (PTSTR)((PBYTE)pProfileList + dwNewSize - sizeof(TCHAR));
        *pPtr = '\0';               // double NULL terminate
    }

    //
    // Set the device data
    //

    if (! SetDeviceData(pDeviceName, dwDeviceClass, DEVICE_PROFILE_DATA,
                        pProfileList, dwNewSize))
    {
        WARNING((__TEXT("Error setting device profile data for %s\n"), pDeviceName));
        goto EndAssociateProfileWithDevice;
    }

    //
    // Increment usage count and set it
    //

    regData.dwRefCount++;
    if ((dwErr = RegSetValueEx(hkDevice, pFilename, 0, REG_BINARY,
            (PBYTE)&regData, sizeof(REGDATA))) != ERROR_SUCCESS)
    {
        ERR((__TEXT("Could not set registry data for profile %s: %d\n"), pFilename, dwErr));
        SetLastError(dwErr);
        goto EndAssociateProfileWithDevice;
    }

    #if !defined(_WIN95_)

    if (bFirstProfile)
    {
        ChangeICMSetting(pMachineName, pDeviceName, ICM_ON);
    }

    #endif

    rc = TRUE;              // Everything went well!

EndAssociateProfileWithDevice:
    if (hkICM)
    {
        RegCloseKey(hkICM);
    }
    if (hkDevice)
    {
        RegCloseKey(hkDevice);
    }
    if (pProfileList)
    {
        MemFree(pProfileList);
    }

    return rc;
}


/******************************************************************************
 *
 *                    InternalDisassociateColorProfileFromDevice
 *
 *  Function:
 *       This function disassociates a color profile on a a given machine from
 *       a particular device. It fails if the color profile is not installed
 *       on the machine and associated with the device. It decreases the usage
 *       reference count of the profile.
 *
 *  Arguments:
 *       pMachineName    - name identifying machine. NULL implies local
 *       pProfileName    - pointer to profile to disassociate
 *       pDeviceName     - pointer to device name
 *
 *  Returns:
 *       TRUE if successful, NULL otherwise
 *
 *  Warning:
 *       Currently only local disassociation is supported, so pMachineName
 *       should be NULL.
 *
 ******************************************************************************/

BOOL
InternalDisassociateColorProfileFromDevice(
    LPCTSTR pMachineName,
    LPCTSTR pProfileName,
    LPCTSTR pDeviceName
    )
{
    PROFILEHEADER header;           // profile header
    REGDATA  regData;               // for storing registry data about profile
    HKEY     hkICM = NULL;          // key to ICM branch in registry
    HKEY     hkDevice = NULL;       // key to ICM device branch in registry
    DWORD    dwSize;                // size of registry data
    DWORD    dwNewSize;             // new size of device registry data
    DWORD    dwErr;                 // error code
    BOOL     rc = FALSE;            // return code
    PTSTR    pFilename;             // profile name without path
    PTSTR    pProfileList = NULL;   // list of associated profiles
    TCHAR    szColorPath[MAX_PATH]; // full path name of profile
    TCHAR    szClass[5];            // profile class
    BOOL     bLastProfile = FALSE;  // whether last profile is being removed
    DWORD    dwDeviceClass;         // device class

    //
    // Validate parameters
    //

    if (! pProfileName ||
        ! pDeviceName)
    {
        WARNING((__TEXT("Invalid parameter to DisassociateColorProfileFromDevice\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Only local associations are allowed now
    //

    if (pMachineName != NULL)
    {
        WARNING((__TEXT("Remote profile disassociation attempted, failing...\n")));
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    //
    // Get rid of the directory path and get a pointer to the filename
    //

    pFilename = GetFilenameFromPath((PTSTR)pProfileName);
    if (! pFilename)
    {
        WARNING((__TEXT("Could not parse file name from profile path %s\n"), pProfileName));
        SetLastError(ERROR_INVALID_PARAMETER);
        goto EndDisassociateProfileWithDevice;
    }

    //
    // Create a fully qualified path name
    //

    dwSize = sizeof(szColorPath);
    if (! InternalGetColorDirectory(NULL, szColorPath, &dwSize))
    {
        WARNING((__TEXT("Could not get color directory\n")));
        goto EndDisassociateProfileWithDevice;
    }

    if (szColorPath[lstrlen(szColorPath) - 1] != '\\')
    {
        lstrcat(szColorPath, gszBackslash);
    }
    lstrcat(szColorPath, pFilename);

    //
    // Get the profile class in the form of a string
    //

    if (! GetProfileClassString(szColorPath, szClass, &header))
    {
        WARNING((__TEXT("Installing invalid profile %s\n"), szColorPath));
        SetLastError(ERROR_INVALID_PROFILE);
        goto EndDisassociateProfileWithDevice;
    }

    //
    // Open the registry path where profiles are kept
    //

    if (((dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, gszICMRegPath, &hkICM)) != ERROR_SUCCESS) ||
        ((dwErr = RegOpenKey(hkICM, szClass, &hkDevice)) != ERROR_SUCCESS))
    {
        WARNING((__TEXT("Cannot open ICM\\device branch of registry: %d\n"), dwErr));
        SetLastError(dwErr);
        goto EndDisassociateProfileWithDevice;
    }

    //
    // Check if profile is installed
    //

    dwSize = sizeof(REGDATA);
    if ((dwErr = RegQueryValueEx(hkDevice, pFilename, 0, NULL, (PBYTE)&regData,
            &dwSize)) != ERROR_SUCCESS)
    {
        WARNING((__TEXT("Trying to disassociate a profile that is not installed %s: %d\n"), pFilename, dwErr));
        SetLastError(dwErr);
        goto EndDisassociateProfileWithDevice;
    }

    //
    // Treat CLASS_MONITOR as CLASS_COLORSPACE.
    //
    if ((dwDeviceClass = header.phClass) == CLASS_MONITOR)
    {
        //
        // since CLASS_MONTOR profile can be associated any device class.
        //
        dwDeviceClass = CLASS_COLORSPACE;
    }

    //
    // Read in the list of profiles associated with the device
    //

    dwSize = 0;
    if (! GetDeviceData(pDeviceName, dwDeviceClass, DEVICE_PROFILE_DATA,
                        (PVOID *)&pProfileList, &dwSize, TRUE))
    {
        pProfileList = NULL;        // no data found
    }

    //
    // If the profile is not associated with the device, return failure
    //

    if (! pProfileList ||
        ! IsStringInMultiSz(pProfileList, pFilename))
    {
        WARNING((__TEXT("Trying to disassociate a profile that is not associated %s\n"), pFilename));
        SetLastError(ERROR_PROFILE_NOT_ASSOCIATED_WITH_DEVICE);
        goto EndDisassociateProfileWithDevice;
    }

    //
    // Remove profile from the list of profiles, and double NULL
    // terminate the MULTI_SZ string.
    //

    dwNewSize = RemoveStringFromMultiSz(pProfileList, pFilename, dwSize);

    //
    // Set the device data
    //

    if (! SetDeviceData(pDeviceName, dwDeviceClass, DEVICE_PROFILE_DATA,
                        pProfileList, dwNewSize))
    {
        WARNING((__TEXT("Error setting device profile data for %s\n"), pDeviceName));
        goto EndDisassociateProfileWithDevice;
    }

    if (dwNewSize <= sizeof(TCHAR))
    {
        bLastProfile = TRUE;
    }

    //
    // Decrement usage count and set it
    //

    regData.dwRefCount--;
    if ((dwErr = RegSetValueEx(hkDevice, pFilename, 0, REG_BINARY,
            (PBYTE)&regData, sizeof(REGDATA))) != ERROR_SUCCESS)
    {
        ERR((__TEXT("Could not set registry data for profile %s: %d\n"), pFilename, dwErr));
        SetLastError(dwErr);
        goto EndDisassociateProfileWithDevice;
    }

    #if !defined(_WIN95_)

    if (bLastProfile)
    {
        ChangeICMSetting(pMachineName, pDeviceName, ICM_OFF);
    }

    #endif

    rc = TRUE;              // Everything went well!

EndDisassociateProfileWithDevice:
    if (hkICM)
    {
        RegCloseKey(hkICM);
    }
    if (hkDevice)
    {
        RegCloseKey(hkDevice);
    }
    if (pProfileList)
    {
        MemFree(pProfileList);
    }

    return rc;
}


/******************************************************************************
 *
 *                          InternalEnumColorProfiles
 *
 *  Function:
 *       These functions enumerates color profiles satisfying the given
 *       enumeration criteria. It searches among all installed profiles, and
 *       on return fills out a buffer with a series of NULL terminated profile
 *       filenames double NULL terminated at the end.
 *
 *  Arguments:
 *       pMachineName    - name identifying machine on which the enumeration
 *                         needs to be done
 *       pEnumRecord     - pointer to enumeration criteria
 *       pBuffer         - pointer to buffer to receive result, can be NULL
 *       pdwSize         - pointer to buffer size. On return it is actual number
 *                         of bytes copied/needed.
 *       pnProfiles      - pointer to DWORD. On return, it is number of profiles
 *                         copied to pBuffer.
 *
 *  Returns:
 *       TRUE if successful, NULL otherwise
 *
 *  Warning:
 *       Currently only local enumeration is supported, so pMachineName should
 *       be NULL.
 *
 ******************************************************************************/

BOOL
InternalEnumColorProfiles(
    LPCTSTR    pMachineName,
    PENUMTYPE  pEnumRecord,
    PBYTE      pBuffer,
    PDWORD     pdwSize,
    PDWORD     pnProfiles
    )
{
    REGDATA  regData;               // for storing registry data about profile
    HKEY     hkICM = NULL;          // key to ICM branch in registry
    HKEY     hkDevice = NULL;       // key to ICM device branch in registry
    PTSTR    pProfileList = NULL;   // list of associated profiles
    PTSTR    pTempProfileList;      // temporary copy of profile list
    DWORD    dwSize;                // size of profile value
    DWORD    dwDataSize;            // size of profile data
    DWORD    dwInputSize;           // incoming size of buffer
    DWORD    i, j;                  // counter variables
    DWORD    dwErr;                 // error code
    BOOL     rc = FALSE;            // return code
    TCHAR    szFullPath[MAX_PATH];  // full path of profile to open
    PTSTR    pProfile;              // pointer to profile name in full path
    DWORD    dwLen;                 // length of color directory string
    DWORD    bSkipMatch;            // true for skipping exact profile matching
    MATCHTYPE match;                // Type of profile match
    PBYTE     pBufferStart;         // Start of user given buffer
    DWORD    dwSizeOfStruct;        // size of ENUMTYPE structure
    DWORD    dwVersion;             // ENUMTYPE structure version

    //
    // Validate parameters
    //

    if (! pdwSize ||
        IsBadWritePtr(pdwSize, sizeof(DWORD)) ||
        (pBuffer && IsBadWritePtr(pBuffer, *pdwSize)) ||
        (pnProfiles && IsBadWritePtr(pnProfiles, sizeof(DWORD))) ||
        ! pEnumRecord ||
        IsBadReadPtr(pEnumRecord, sizeof(DWORD)*3))   // probe until ENUMTYPE.dwFields
    {
ParameterError_InternalEnumColorProfiles:
        WARNING((__TEXT("Invalid parameter to EnumColorProfiles\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Check structure size based on its version.
    //

    dwSizeOfStruct = pEnumRecord->dwSize;
    dwVersion      = pEnumRecord->dwVersion;

    if (dwVersion >= ENUM_TYPE_VERSION)
    {
        if (dwSizeOfStruct < sizeof(ENUMTYPE))
            goto ParameterError_InternalEnumColorProfiles;
    }
    else if (dwVersion == 0x0200)
    {
        if (dwSizeOfStruct < sizeof(ENUMTYPE)-sizeof(DWORD))
            goto ParameterError_InternalEnumColorProfiles;

        //
        // Version 2 should not have ET_DEVICECLASS bit
        //

        if (pEnumRecord->dwFields & ET_DEVICECLASS)
            goto ParameterError_InternalEnumColorProfiles;

        WARNING((__TEXT("Old version ENUMTYPE to InternalEnumColorProfiles\n")));
    }
    else
    {
        goto ParameterError_InternalEnumColorProfiles;
    }

    if (IsBadReadPtr(pEnumRecord, dwSizeOfStruct))
    {
        goto ParameterError_InternalEnumColorProfiles;
    }

    //
    // Only local enumerations are allowed now
    //

    if (pMachineName != NULL)
    {
        WARNING((__TEXT("Remote profile enumeration attempted, failing...\n")));
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    dwInputSize = *pdwSize;

    //
    // Get color directory
    //

    dwLen = sizeof(szFullPath);
    if (! InternalGetColorDirectory(NULL, szFullPath, &dwLen))
    {
        WARNING((__TEXT("Error getting color directory\n")));
        return FALSE;
    }

    if (szFullPath[lstrlen(szFullPath) - 1] != '\\')
    {
        lstrcat(szFullPath, gszBackslash);
    }
    pProfile = &szFullPath[lstrlen(szFullPath)];
    dwLen = lstrlen(szFullPath) * sizeof(TCHAR);

    //
    // Initialize return parameters
    //

    *pdwSize = 0;
    if (pnProfiles)
        *pnProfiles = 0;

    if (pBuffer && dwInputSize >= sizeof(TCHAR))
    {
        *((PTSTR)pBuffer) = '\0';
    }

    //
    // Check if we are looking for the profiles of a particular device or
    // not because we enumerate them from different places.
    //

    if (pEnumRecord->dwFields & ET_DEVICENAME)
    {
        DWORD *pbSkipMatch = &bSkipMatch;
        DWORD  dwDeviceClass;

        if (! pEnumRecord->pDeviceName)
        {
            WARNING((__TEXT("Invalid parameter to EnumColorProfiles\n")));
            SetLastError(ERROR_INVALID_PARAMETER);
            goto EndEnumerateColorProfiles;
        }

        //
        // Get list of profiles associated with the device. If we don't
        // know what device it is, specify ColorSpace, which tries all three
        //

        if (pEnumRecord->dwFields & ET_DEVICECLASS)
        {
            dwDeviceClass = pEnumRecord->dwDeviceClass;
        }
        else
        {
            dwDeviceClass = CLASS_COLORSPACE;
        }

        //
        // Get the device configuration whether we do exact matching or not.
        //

        dwSize = sizeof(DWORD);
        if (! GetDeviceData(pEnumRecord->pDeviceName, dwDeviceClass, DEVICE_PROFILE_ENUMMODE,
                            (PVOID *)&pbSkipMatch, &dwSize, FALSE))
        {
            bSkipMatch = FALSE;
        }

        //
        // Get the profile data.
        //

        dwSize = 0;
        if (! GetDeviceData(pEnumRecord->pDeviceName, dwDeviceClass, DEVICE_PROFILE_DATA,
                            (PVOID *)&pProfileList, &dwSize, TRUE))
        {
            pProfileList = NULL;    // no data found
        }

        if(! pProfileList)
        {
            //
            // No profiles associated with this device
            //

            rc = TRUE;
            if (pBuffer && dwInputSize >= sizeof(TCHAR)*2)
            {
                *((PTSTR)pBuffer + 1) = '\0';
            }
            goto EndEnumerateColorProfiles;
        }

        //
        // Run through the list of profiles and check each one to see if it
        // matches the enumeration criteria. If it does, and the buffer is
        // large enough, copy it to the buffer, and increment the byte count
        // and number of profiles enumerated.
        //

        pBufferStart = pBuffer;
        pTempProfileList = pProfileList;

        while (*pTempProfileList)
        {
            lstrcpy(pProfile, pTempProfileList);

            if (bSkipMatch)
            {
                match = EXACT_MATCH;
            }
            else
            {
                match = DoesProfileMatchEnumRecord(szFullPath, pEnumRecord);
            }

            if (match != NOMATCH)
            {
                *pdwSize += (lstrlen(pTempProfileList) + 1) * sizeof(TCHAR);

                //
                // Check strictly less than because you need one more for
                // the final NULL termination
                //

                if (pBuffer && (*pdwSize < dwInputSize))
                {
                    if (match == MATCH)
                    {
                        lstrcpy((PTSTR)pBuffer, pTempProfileList);
                    }
                    else
                    {
                        //
                        // Exact match, add to beginning of buffer
                        //

                        InsertInBuffer(pBufferStart, pBuffer, pTempProfileList);
                    }

                    pBuffer += (lstrlen(pTempProfileList) + 1) * sizeof(TCHAR);
                }

                if (pnProfiles)
                    (*pnProfiles)++;
            }

            pTempProfileList += lstrlen(pTempProfileList) + 1;
        }
    }
    else
    {
        DWORD  dwNumClasses;
        PTSTR *ppszEnumClasses;
        PTSTR  pszEnumClassArray[2];

        //
        // We are not looking at a particular device, so enumerate
        // profiles from the registry
        //

        if (pEnumRecord->dwFields & ET_DEVICECLASS)
        {
            //
            // If device class is specified, enumrate the specified device class and color
            // space class which can be associated to any device.
            //

            pszEnumClassArray[0] = ConvertClassIdToClassString(pEnumRecord->dwDeviceClass);
            pszEnumClassArray[1] = ConvertClassIdToClassString(CLASS_COLORSPACE);

            if (!pszEnumClassArray[0] || !pszEnumClassArray[1])
            {
                WARNING((__TEXT("Invalid DeviceClass to EnumColorProfiles\n")));
                SetLastError(ERROR_INVALID_PARAMETER);
                goto EndEnumerateColorProfiles;
            }

            ppszEnumClasses = pszEnumClassArray;
            dwNumClasses    = 2;
        }
        else
        {
            ppszEnumClasses = gpszClasses;
            dwNumClasses    = sizeof(gpszClasses)/sizeof(PTSTR);
        }

        //
        // Open the registry path where profiles are kept (and create it if not exist)
        //

        if ((dwErr = RegCreateKey(HKEY_LOCAL_MACHINE, gszICMRegPath, &hkICM)) != ERROR_SUCCESS)
        {
            WARNING((__TEXT("Cannot open ICM branch of registry: %d\n"), dwErr));
            SetLastError(dwErr);
            goto EndEnumerateColorProfiles;
        }

        pBufferStart = pBuffer;

        for (i=0; i<dwNumClasses; i++,ppszEnumClasses++)
        {
            DWORD   nValues;        // number of name-values in key

            if (RegOpenKey(hkICM, *ppszEnumClasses, &hkDevice) != ERROR_SUCCESS)
            {
                continue;           // go to next key
            }

            if ((dwErr = RegQueryInfoKey(hkDevice, NULL, NULL, 0, NULL, NULL, NULL,
                &nValues, NULL, NULL, NULL, NULL)) != ERROR_SUCCESS)
            {
                WARNING((__TEXT("Cannot count values in device branch of registry: %d\n"), dwErr));
                RegCloseKey(hkDevice);
                SetLastError(dwErr);
                goto EndEnumerateColorProfiles;
            }

            //
            // Go through the list of profiles and return everything that
            // satisfies the enumeration criteria
            //

            for (j=0; j<nValues; j++)
            {
                dwSize = sizeof(szFullPath) - dwLen;
                dwDataSize = sizeof(REGDATA);
                if (RegEnumValue(hkDevice, j, pProfile, &dwSize, 0,
                    NULL, (PBYTE)&regData, &dwDataSize) == ERROR_SUCCESS)
                {
                    match = DoesProfileMatchEnumRecord(szFullPath, pEnumRecord);

                    if (match != NOMATCH)
                    {
                        *pdwSize += (lstrlen(pProfile) + 1) * sizeof(TCHAR);
                        if (pBuffer && (*pdwSize < dwInputSize))
                        {
                            if (match == MATCH)
                            {
                                lstrcpy((PTSTR)pBuffer, pProfile);
                            }
                            else
                            {
                                //
                                // Exact match, add to beginning of buffer
                                //

                                InsertInBuffer(pBufferStart, pBuffer, pProfile);
                            }

                            pBuffer += (lstrlen(pProfile) + 1) * sizeof(TCHAR);
                        }

                        if (pnProfiles)
                            (*pnProfiles)++;
                    }
                }
            }

            RegCloseKey(hkDevice);
        }
    }

    *pdwSize += sizeof(TCHAR);      // extra NULL termination

    if (pBuffer && *pdwSize <= dwInputSize)
    {
        *((PTSTR)pBuffer) = '\0';
        rc = TRUE;
    }
    else
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

EndEnumerateColorProfiles:

    if (hkICM)
    {
        RegCloseKey(hkICM);
    }
    if (pProfileList)
    {
        MemFree(pProfileList);
    }

    return rc;
}

VOID
InsertInBuffer(
    PBYTE  pStart,
    PBYTE  pEnd,
    PTSTR  pString
    )
{
    DWORD cnt = (lstrlen(pString) + 1) * sizeof(TCHAR);

    MyCopyMemory(pStart+cnt, pStart, (DWORD)(pEnd - pStart));

    lstrcpy((PTSTR)pStart, pString);

    return;
}

/******************************************************************************
 *
 *                          InternalSetSCSProfile
 *
 *  Function:
 *       These functions regsiters the given profile for the standard color
 *       space specified. This will register it in the OS and can be queried
 *       using GetStandardColorSpaceProfile.
 *
 *  Arguments:
 *       pMachineName    - name identifying machine on which the standard color
 *                         space profile should be registered
 *       dwSCS           - ID for the standard color space
 *       pProfileName    - pointer to profile filename
 *
 *  Returns:
 *       TRUE if successful, NULL otherwise
 *
 *  Warning:
 *       Currently only local registration is supported, so pMachineName should
 *       be NULL.
 *
 ******************************************************************************/

BOOL
InternalSetSCSProfile(
    LPCTSTR   pMachineName,
    DWORD     dwSCS,
    LPCTSTR   pProfileName
    )
{
    HKEY   hkICM = NULL;            // key to ICM branch in registry
    HKEY   hkRegProf = NULL;        // key to registered color spaces branch
    DWORD  dwSize;                  // size of registry data
    DWORD  dwErr;                   // error code
    BOOL   rc = FALSE;              // return code
    TCHAR  szProfileID[5];          // profile class

    //
    // Validate parameters
    //

    if (!pProfileName)
    {
        WARNING((__TEXT("Invalid parameter to SetStandardColorSpaceProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Only local registration is allowed now
    //

    if (pMachineName != NULL)
    {
        WARNING((__TEXT("Remote SCS profile registration attempted, failing...\n")));
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    dwSize = (lstrlen(pProfileName) + 1) * sizeof(TCHAR);

    //
    // Open the registry location where this is kept
    //

    if (((dwErr = RegCreateKey(HKEY_LOCAL_MACHINE, gszICMRegPath, &hkICM)) == ERROR_SUCCESS) &&
        ((dwErr = RegCreateKey(hkICM, gszRegisteredProfiles, &hkRegProf))== ERROR_SUCCESS))
    {
        ConvertDwordToString(dwSCS, szProfileID);

        if ((dwErr = RegSetValueEx(hkRegProf, szProfileID, 0, REG_SZ,
            (PBYTE)pProfileName, dwSize)) == ERROR_SUCCESS)
        {
            rc = TRUE;
        }
    }

    if (hkICM)
    {
        RegCloseKey(hkICM);
    }

    if (hkRegProf)
    {
        RegCloseKey(hkRegProf);
    }

    if (!rc)
    {
        WARNING((__TEXT("InternalSetSCSProfile failed: %d\n"), dwErr));
        SetLastError(dwErr);
    }

    return rc;
}


/******************************************************************************
 *
 *                          InternalGetSCSProfile
 *
 *  Function:
 *       These functions retrieves the profile regsitered for the standard color
 *       space specified.
 *
 *  Arguments:
 *       pMachineName    - name identifying machine on which the standard color
 *                         space profile should be queried
 *       dwSCS           - ID for the standard color space
 *       pBuffer         - pointer to buffer to receive profile filename
 *       pdwSize         - pointer to DWORD specifying size of buffer. On return
 *                         it has size needed/used
 *
 *  Returns:
 *       TRUE if successful, NULL otherwise
 *
 *  Warning:
 *       Currently only local query is supported, so pMachineName should
 *       be NULL.
 *
 ******************************************************************************/

BOOL
InternalGetSCSProfile(
    LPCTSTR   pMachineName,
    DWORD     dwSCS,
    PTSTR     pBuffer,
    PDWORD    pdwSize
    )
{
    HKEY   hkICM = NULL;            // key to ICM branch in registry
    HKEY   hkRegProf = NULL;        // key to registered color spaces branch
    DWORD  dwErr;                   // error code
    DWORD  dwSize;
    BOOL   rc = FALSE;              // return code
    TCHAR  szProfileID[5];          // profile class

    //
    // Validate parameters
    //

    if (! pdwSize ||
        IsBadWritePtr(pdwSize, sizeof(DWORD)) ||
        (pBuffer && IsBadWritePtr(pBuffer, *pdwSize)))
    {
        WARNING((__TEXT("Invalid parameter to GetStandardColorSpaceProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Only local query is allowed now
    //

    if (pMachineName != NULL)
    {
        WARNING((__TEXT("Remote SCS profile query attempted, failing...\n")));
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    dwSize = *pdwSize;

    //
    // Look in the registry for a profile registered for this color space ID
    //

    if (((dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, gszICMRegPath, &hkICM)) == ERROR_SUCCESS) &&
        ((dwErr = RegOpenKey(hkICM, gszRegisteredProfiles, &hkRegProf)) == ERROR_SUCCESS))
    {
        ConvertDwordToString(dwSCS, szProfileID);
        if ((dwErr = RegQueryValueEx(hkRegProf, szProfileID, NULL, NULL,
                (PBYTE)pBuffer, pdwSize)) == ERROR_SUCCESS)
        {
            rc = TRUE;
        }
    }

    if (hkICM)
    {
        RegCloseKey(hkICM);
    }

    if (hkRegProf)
    {
        RegCloseKey(hkRegProf);
    }

    if (!rc && (dwSCS == LCS_sRGB || dwSCS == LCS_WINDOWS_COLOR_SPACE))
    {
        *pdwSize = dwSize;
        rc = GetColorDirectory(NULL, pBuffer, pdwSize);
        if (!rc && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            return FALSE;
        }

        *pdwSize += (lstrlen(gszBackslash) + lstrlen(gszsRGBProfile)) * sizeof(TCHAR);

        if (*pdwSize <= dwSize && pBuffer)
        {
            lstrcat(pBuffer, gszBackslash);
            lstrcat(pBuffer, gszsRGBProfile);
            rc = TRUE;
        }
        else
        {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
        }
    }

    if (!rc)
    {
        WARNING((__TEXT("InternalGetSCSProfile failed: %d\n"), dwErr));
        SetLastError(dwErr);
    }

    //
    // If pBuffer is NULL, RegQueryValueEx return TRUE. Our API should return FALSE
    // in this case. Handle this.
    //

    if (pBuffer == NULL && rc)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        rc = FALSE;
    }

    return rc;
}


/******************************************************************************
 *
 *                           ConvertDwordToString
 *
 *  Function:
 *       This function converts a DWORD into a string. The string passed in
 *       is large enough. It converts to Unicode or Ansi depending on how
 *       this is compiled.
 *
 *  Arguments:
 *       dword           - DWORD to convert
 *       pString         - pointer to buffer to hold the result
 *
 *  Returns:
 *       Nothing
 *
 ******************************************************************************/

VOID
ConvertDwordToString(
    DWORD  dword,
    PTSTR  pString
    )
{
    int i;                          // counter

    for (i=0; i<4; i++)
    {
        pString[i]  = (TCHAR)(((char*)&dword)[3-i]);
    }

    pString[4] = '\0';

    return;
}

/******************************************************************************
 *
 *                           ConvertClassIdToClassString
 *
 *  Function:
 *       This function converts a DWORD Device Class Id into a its device string
 *
 *  Arguments:
 *       dwClassId      - Device class id.
 *
 *  Returns:
 *       pointer to a string
 *
 ******************************************************************************/

PTSTR
ConvertClassIdToClassString(
    DWORD  dwClassId
    )
{
    switch (dwClassId)
    {
    case CLASS_MONITOR:
        return (gpszClasses[INDEX_CLASS_MONITOR]);
    case CLASS_PRINTER:
        return (gpszClasses[INDEX_CLASS_PRINTER]);
    case CLASS_SCANNER:
        return (gpszClasses[INDEX_CLASS_SCANNER]);
    case CLASS_COLORSPACE:
        return (gpszClasses[INDEX_CLASS_COLORSPACE]);
    default:
        return NULL;
    }
}

/******************************************************************************
 *
 *                          GetProfileClassString
 *
 *  Function:
 *       This function returns the profile class from the header as a string.
 *       It also validates the profile.
 *
 *  Arguments:
 *       pProfileName    - name of profile
 *       pClass          - pointer to buffer to hold the profile class string
 *       pHeader         - if this is non NULL, it returns the header here
 *
 *  Returns:
 *       TRUE on success, FALSE otherwise
 *
 ******************************************************************************/

BOOL
GetProfileClassString(
    LPCTSTR        pProfileName,
    PTSTR          pClass,
    PPROFILEHEADER pHeader
    )
{
    PROFILEHEADER header;                // color profile header
    PROFILE       prof;                  // profile object for opening profile
    HPROFILE      hProfile = NULL;       // handle to opened profile
    BOOL          bValidProfile = FALSE; // validation of the profile
    BOOL          rc = FALSE;            // return code

    //
    // Open a handle to the profile
    //

    prof.dwType = PROFILE_FILENAME;
    prof.pProfileData = (PVOID)pProfileName;
    prof.cbDataSize = (lstrlen(pProfileName) + 1) * sizeof(TCHAR);

    hProfile = OpenColorProfile(&prof, PROFILE_READ, FILE_SHARE_READ,
                    OPEN_EXISTING);
    if (! hProfile)
    {
        WARNING((__TEXT("Error opening profile %s\n"), pProfileName));
        goto EndGetProfileClassString;
    }

    //
    // Check the validation of the profile.
    //
    if (! IsColorProfileValid(hProfile,&bValidProfile) || ! bValidProfile)
    {
        WARNING((__TEXT("Error invalid profile %s\n"), pProfileName));
        goto EndGetProfileClassString;
    }

    //
    // Get the profile class
    //

    if (! pHeader)
    {
        pHeader = &header;
    }

    if (! GetColorProfileHeader(hProfile, pHeader))
    {
        ERR((__TEXT("Error getting color profile header for %s\n"), pProfileName));
        goto EndGetProfileClassString;
    }
    ConvertDwordToString(pHeader->phClass, pClass);

    rc= TRUE;

EndGetProfileClassString:
    if (hProfile)
    {
        CloseColorProfile(hProfile);
    }

    return rc;
}


/******************************************************************************
 *
 *                           GetFilenameFromPath
 *
 *  Function:
 *       This function takes a fully qualified pathname and returns a pointer
 *       to the filename part alone
 *
 *  Arguments:
 *       pPathName       - pointer to pathname
 *
 *  Returns:
 *       Pointer to filename on success, NULL otherwise
 *
 ******************************************************************************/

PTSTR
GetFilenameFromPath(
    PTSTR pPathName
    )
{
    DWORD dwLen;                      // length of pathname
    PTSTR pPathNameStart = pPathName;

    dwLen = lstrlen(pPathName);

    if (dwLen == 0)
    {
        return NULL;
    }

    //
    // Go to the end of the pathname, and start going backwards till
    // you reach the beginning or a backslash
    //

    pPathName += dwLen;

    //
    // Currently 'pPathName' points null-terminate character, so move
    // the pointer to last character.
    //

    do
    {
        pPathName = CharPrev(pPathNameStart,pPathName);

        if (*pPathName == TEXT('\\'))
        {
            pPathName = CharNext(pPathName);
            break;
        }

        //
        // Loop until fist
        //

    } while (pPathNameStart < pPathName);

    //
    // if *pPathName is zero, then we had a string that ends in a backslash
    //

    return *pPathName ? pPathName : NULL;
}


/******************************************************************************
 *
 *                              GetDeviceData
 *
 *  Function:
 *       This function is a wrapper for IGetDeviceData. For devices like monitor,
 *       printer & scanner it calls the internal function. If we are asked
 *       to get the device data for a "colorspace device", we try monitor, printer
 *       and scanner till one succeeds or they all fail. This is done so that we
 *       we can associate sRGB like profiles with any device.
 *
 *  Arguments:
 *       pDeviceName     - pointer to name of the device
 *       dwClass         - device type like monitor, printer etc.
 *       ppDeviceData    - pointer to pointer to buffer to receive data
 *       pdwSize         - pointer to size of buffer. On return it is size of
 *                         data returned/size needed.
 *       bAllocate       - If TRUE, allocate memory for data
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
GetDeviceData(
    LPCTSTR pDeviceName,
    DWORD   dwClass,
    DWORD   dwDataType,
    PVOID  *ppDeviceData,
    PDWORD  pdwSize,
    BOOL    bAllocate
    )
{
    BOOL rc = FALSE;

    if (dwClass == CLASS_MONITOR ||
        dwClass == CLASS_PRINTER ||
        dwClass == CLASS_SCANNER)
    {
        rc = IGetDeviceData(pDeviceName, dwClass, dwDataType, ppDeviceData, pdwSize, bAllocate);
    }
    else if (dwClass == CLASS_COLORSPACE)
    {
        rc = IGetDeviceData(pDeviceName, CLASS_MONITOR, dwDataType, ppDeviceData, pdwSize, bAllocate) ||
             IGetDeviceData(pDeviceName, CLASS_PRINTER, dwDataType, ppDeviceData, pdwSize, bAllocate) ||
             IGetDeviceData(pDeviceName, CLASS_SCANNER, dwDataType, ppDeviceData, pdwSize, bAllocate);
    }

    return rc;
}


/******************************************************************************
 *
 *                              IGetDeviceData
 *
 *  Function:
 *       This function retrieves ICM data stored with the different devices.
 *
 *  Arguments:
 *       pDeviceName     - pointer to name of the device
 *       dwClass         - device type like monitor, printer etc.
 *       ppDeviceData    - pointer to pointer to buffer to receive data
 *       pdwSize         - pointer to size of buffer. On return it is size of
 *                         data returned/size needed.
 *       bAllocate       - If TRUE, allocate memory for data
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
IGetDeviceData(
    LPCTSTR pDeviceName,
    DWORD   dwClass,
    DWORD   dwDataType,
    PVOID  *ppDeviceData,
    PDWORD  pdwSize,
    BOOL    bAllocate
    )
{
    PFNOPENDEVICE    fnOpenDevice;
    PFNCLOSEDEVICE   fnCloseDevice;
    PFNGETDEVICEDATA fnGetData;
    HANDLE           hDevice;
    DWORD            dwSize;
    LPTSTR           pDataKey;
    LPTSTR           pDataValue;
    BOOL             rc = FALSE;

    //
    // Set up function pointers so we can write common code
    //

    switch (dwClass)
    {
    case CLASS_PRINTER:
        fnOpenDevice  = (PFNOPENDEVICE)OpenPrtr;
        fnCloseDevice = (PFNCLOSEDEVICE)ClosePrtr;
        fnGetData     = (PFNGETDEVICEDATA)GetPrtrData;
        break;

    case CLASS_MONITOR:
        fnOpenDevice  = (PFNOPENDEVICE)OpenMonitor;
        fnCloseDevice = (PFNCLOSEDEVICE)CloseMonitor;
        fnGetData     = (PFNGETDEVICEDATA)GetMonitorData;
        break;

    case CLASS_SCANNER:
        fnOpenDevice  = (PFNOPENDEVICE)OpenScanner;
        fnCloseDevice = (PFNCLOSEDEVICE)CloseScanner;
        fnGetData     = (PFNGETDEVICEDATA)GetScannerData;
        break;

    default:
        return FALSE;
    }

    //
    // Set up registry keywords.
    //

    switch (dwDataType)
    {
    case DEVICE_PROFILE_DATA:

        pDataKey      = gszICMProfileListKey;

        //
        // The way to store printer profile is different than others... trim it.
        //

        if (dwClass == CLASS_PRINTER)
        {
            pDataValue = gszFiles;
        }
        else
        {
            pDataValue = gszICMProfileListValue;
        }

        break;

    case DEVICE_PROFILE_ENUMMODE:

        pDataKey      = gszICMDeviceDataKey;
        pDataValue    = gszICMProfileEnumMode;
        break;

    default:
        return FALSE;
    }

    //
    // Open the device and get a handle to it
    //

    if (! (*fnOpenDevice)((PTSTR)pDeviceName, &hDevice, NULL))
    {
        return FALSE;
    }

    if (bAllocate || (ppDeviceData == NULL))
    {
        DWORD retcode;

        //
        // We need to allocate memory. Find out how much we need, and
        // allocate it.
        //

        dwSize = 0;
        retcode = (*fnGetData)(hDevice, pDataKey, pDataValue, NULL, NULL, 0, &dwSize);

        if ((retcode != ERROR_SUCCESS)    &&  // Win 95 returns this
            (retcode != ERROR_MORE_DATA))     // NT returns this
        {
            VERBOSE((__TEXT("GetDeviceData failed for %s\n"), pDeviceName));
            goto EndGetDeviceData;
        }

        *pdwSize = dwSize;

        if (ppDeviceData == NULL)
        {
            //
            // Caller wants to know the data size.
            //

            rc = TRUE;
            goto EndGetDeviceData;
        }
        else
        {
            //
            // Allocate buffer.
            //

            *ppDeviceData = MemAlloc(dwSize);
            if (! *ppDeviceData)
            {
                WARNING((__TEXT("Error allocating memory\n")));
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto EndGetDeviceData;
            }
        }
    }

    //
    // Get the data
    //

    if ((*fnGetData)(hDevice, pDataKey, pDataValue, NULL, (PBYTE)*ppDeviceData,
        *pdwSize, pdwSize) == ERROR_SUCCESS)
    {
        rc = TRUE;
    }

EndGetDeviceData:
    (*fnCloseDevice)(hDevice);

    return rc;
}


/******************************************************************************
 *
 *                              SetDeviceData
 *
 *  Function:
 *       This function is a wrapper for ISetDeviceData. For devices like monitor,
 *       printer & scanner it calls the internal function. If we are asked
 *       to set the device data for a "colorspace device", we try monitor, printer
 *       and scanner till one succeeds or they all fail. This is done so that we
 *       we can associate sRGB like profiles with any device.
 *
 *  Arguments:
 *       pDeviceName     - pointer to name of the device
 *       dwClass         - device type like monitor, printer etc.
 *       pDeviceData     - pointer buffer containing data
 *       dwSize          - size of data
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
SetDeviceData(
    LPCTSTR pDeviceName,
    DWORD   dwClass,
    DWORD   dwDataType,
    PVOID   pDeviceData,
    DWORD   dwSize
    )
{
    BOOL rc = FALSE;

    if (dwClass == CLASS_MONITOR ||
        dwClass == CLASS_PRINTER ||
        dwClass == CLASS_SCANNER)
    {
        rc = ISetDeviceData(pDeviceName, dwClass, dwDataType, pDeviceData, dwSize);
    }
    else if (dwClass == CLASS_COLORSPACE)
    {
        rc = ISetDeviceData(pDeviceName, CLASS_MONITOR, dwDataType, pDeviceData, dwSize) ||
             ISetDeviceData(pDeviceName, CLASS_PRINTER, dwDataType, pDeviceData, dwSize) ||
             ISetDeviceData(pDeviceName, CLASS_SCANNER, dwDataType, pDeviceData, dwSize);
    }

    return rc;
}


/******************************************************************************
 *
 *                              ISetDeviceData
 *
 *  Function:
 *       This function sets ICM data stored with the different devices.
 *
 *  Arguments:
 *       pDeviceName     - pointer to name of the device
 *       dwClass         - device type like monitor, printer etc.
 *       pDeviceData     - pointer buffer containing data
 *       dwSize          - size of data
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
ISetDeviceData(
    LPCTSTR pDeviceName,
    DWORD   dwClass,
    DWORD   dwDataType,
    PVOID   pDeviceData,
    DWORD   dwSize
    )
{
    PRINTER_DEFAULTS    pd;
    PFNOPENDEVICE       fnOpenDevice;
    PFNCLOSEDEVICE      fnCloseDevice;
    PFNSETDEVICEDATA    fnSetData;
    HANDLE              hDevice;
    LPTSTR              pDataKey;
    LPTSTR              pDataValue;
    DWORD               dwRegType = REG_BINARY;
    BOOL                rc = FALSE;

    //
    // Set up function pointers so we can write common code
    //

    switch (dwClass)
    {
    case CLASS_PRINTER:
        fnOpenDevice  = (PFNOPENDEVICE)OpenPrtr;
        fnCloseDevice = (PFNCLOSEDEVICE)ClosePrtr;
        fnSetData     = (PFNSETDEVICEDATA)SetPrtrData;
        pd.pDatatype  = __TEXT("RAW");
        pd.pDevMode   = NULL;
        pd.DesiredAccess = PRINTER_ACCESS_ADMINISTER;
        break;

    case CLASS_MONITOR:
        fnOpenDevice  = (PFNOPENDEVICE)OpenMonitor;
        fnCloseDevice = (PFNCLOSEDEVICE)CloseMonitor;
        fnSetData     = (PFNSETDEVICEDATA)SetMonitorData;
        break;

    case CLASS_SCANNER:
        fnOpenDevice  = (PFNOPENDEVICE)OpenScanner;
        fnCloseDevice = (PFNCLOSEDEVICE)CloseScanner;
        fnSetData     = (PFNSETDEVICEDATA)SetScannerData;
        break;

    default:
        return FALSE;
    }

    //
    // Set up registry keywords.
    //

    switch (dwDataType)
    {
    case DEVICE_PROFILE_DATA:

        pDataKey      = gszICMProfileListKey;

        //
        // The way to store printer profile is different than others... trim it.
        //

        if (dwClass == CLASS_PRINTER)
        {
            pDataValue = gszFiles;
            dwRegType  = REG_MULTI_SZ;
        }
        else
        {
            pDataValue = gszICMProfileListValue;
        }

        break;

    case DEVICE_PROFILE_ENUMMODE:

        pDataKey      = gszICMDeviceDataKey;
        pDataValue    = gszICMProfileEnumMode;
        break;

    default:
        return FALSE;
    }

    //
    // Open the device and get a handle to it
    //

    if (! (*fnOpenDevice)((PTSTR)pDeviceName, &hDevice, (PTSTR)&pd))
    {
        WARNING((__TEXT("Error opening device %s\n"), pDeviceName));
        return FALSE;
    }

    //
    // Set the data
    //

    if ((*fnSetData)(hDevice, pDataKey, pDataValue, dwRegType, (PBYTE)pDeviceData,
                     dwSize) == ERROR_SUCCESS)
    {
        rc = TRUE;
    }

#if !defined(_WIN95_)

    //
    // If this is printer class, need some more data for profile list.
    //

    if ((rc == TRUE) && (dwClass == CLASS_PRINTER) && (dwDataType == DEVICE_PROFILE_DATA))
    {
        if (((*fnSetData)(hDevice, pDataKey, gszDirectory, REG_SZ, (PBYTE)gszColorDir,
                          (lstrlen(gszColorDir) + 1)*sizeof(TCHAR)) != ERROR_SUCCESS) ||
            ((*fnSetData)(hDevice, pDataKey, gszModule, REG_SZ, (PBYTE)gszMSCMS,
                          (lstrlen(gszMSCMS) + 1)*sizeof(TCHAR)) != ERROR_SUCCESS))
        {
            rc = FALSE;
        }
    }

#endif

    (*fnCloseDevice)(hDevice);

    return rc;
}


/******************************************************************************
 *
 *                              IsStringInMultiSz
 *
 *  Function:
 *       This functions checks if a given multi-sz string has the given string
 *       as one of the strings, and returns TRUE if it does.
 *
 *  Arguments:
 *       pMultiSzString - multi sz string to look in
 *       pString        - string to find
 *
 *  Returns:
 *       TRUE
 *
 ******************************************************************************/

BOOL
IsStringInMultiSz(
    PTSTR pMultiSzString,
    PTSTR pString
    )
{
    BOOL rc = FALSE;                // return code

    while (*pMultiSzString)
    {
        if (! lstrcmpi(pMultiSzString, pString))
        {
            rc = TRUE;
            break;
        }

        pMultiSzString += lstrlen(pMultiSzString) + 1;
    }

    return rc;
}


/******************************************************************************
 *
 *                            RemoveStringFromMultiSz
 *
 *  Function:
 *       This functions removes a given string from a multi-sz string.
 *
 *  Arguments:
 *       pMultiSzString - multi sz string to look in
 *       pString        - string to remove
 *       dwSize         - size in bytes of multi-sz string
 *
 *  Returns:
 *       TRUE
 *
 ******************************************************************************/

DWORD
RemoveStringFromMultiSz(
    PTSTR pMultiSzString,
    PTSTR pString,
    DWORD dwSize
    )
{
    DWORD dwCount = dwSize;         // count of bytes remaining

    while (*pMultiSzString)
    {
        dwCount -= (lstrlen(pMultiSzString) + 1) * sizeof(TCHAR);

        if (! lstrcmpi(pMultiSzString, pString))
        {
            break;
        }

        pMultiSzString += lstrlen(pMultiSzString) + 1;
    }

    MyCopyMemory((PBYTE)pMultiSzString, (PBYTE)(pMultiSzString + lstrlen(pString) + 1), dwCount);

    return dwSize - sizeof(TCHAR) * (lstrlen(pString) + 1);
}


/******************************************************************************
 *
 *                           DoesProfileMatchEnumRecord
 *
 *  Function:
 *       This functions checks if a profile matches the criteria given in
 *       the enumeration record. Note that it does not check if the profile
 *       belongs to the device specified by pDeviceName. So that check must
 *       have happened before itself.
 *
 *  Arguments:
 *       pProfileName   - profile to look at
 *       pEnumRecord    - pointer to criteria to check against
 *
 *  Returns:
 *       MATCH or EXACT_MATCH if the profile matches the criteria, NOMATCH otherwise
 *
 ******************************************************************************/

#define SET(pEnumRecord, bit)        ((pEnumRecord)->dwFields & (bit))

MATCHTYPE
DoesProfileMatchEnumRecord(
    PTSTR     pProfileName,
    PENUMTYPE pEnumRecord
    )
{
    PROFILEHEADER header;           // color profile header
    PROFILE       prof;             // profile object for opening profile
    HPROFILE      hProfile = NULL;  // handle to opened profile
    MATCHTYPE     rc = NOMATCH;     // return code

    //
    // Open a handle to the profile
    //

    prof.dwType = PROFILE_FILENAME;
    prof.pProfileData = (PVOID)pProfileName;
    prof.cbDataSize = (lstrlen(pProfileName) + 1) * sizeof(TCHAR);

    hProfile = OpenColorProfile(&prof, PROFILE_READ, FILE_SHARE_READ,
                    OPEN_EXISTING);
    if (! hProfile)
    {
        WARNING((__TEXT("Error opening profile %s\n"), pProfileName));
        goto EndDoesProfileMatchEnumRecord;
    }

    //
    // Get the profile header
    //

    if (! GetColorProfileHeader(hProfile, &header))
    {
        ERR((__TEXT("Error getting color profile header for %s\n"), pProfileName));
        goto EndDoesProfileMatchEnumRecord;
    }

    if ((!SET(pEnumRecord, ET_CMMTYPE)         || (pEnumRecord->dwCMMType         == header.phCMMType))         &&
        (!SET(pEnumRecord, ET_CLASS)           || (pEnumRecord->dwClass           == header.phClass))           &&
        (!SET(pEnumRecord, ET_DATACOLORSPACE)  || (pEnumRecord->dwDataColorSpace  == header.phDataColorSpace))  &&
        (!SET(pEnumRecord, ET_CONNECTIONSPACE) || (pEnumRecord->dwConnectionSpace == header.phConnectionSpace)) &&
        (!SET(pEnumRecord, ET_SIGNATURE)       || (pEnumRecord->dwSignature       == header.phSignature))       &&
        (!SET(pEnumRecord, ET_PLATFORM)        || (pEnumRecord->dwPlatform        == header.phPlatform))        &&
        (!SET(pEnumRecord, ET_PROFILEFLAGS)    || (pEnumRecord->dwProfileFlags    == header.phProfileFlags))    &&
        (!SET(pEnumRecord, ET_MANUFACTURER)    || (pEnumRecord->dwManufacturer    == header.phManufacturer))    &&
        (!SET(pEnumRecord, ET_MODEL)           || (pEnumRecord->dwModel           == header.phModel))           &&
        (!SET(pEnumRecord, ET_ATTRIBUTES)      || (pEnumRecord->dwAttributes[0]   == header.phAttributes[0] &&
                                                   pEnumRecord->dwAttributes[1]   == header.phAttributes[1]))   &&
        (!SET(pEnumRecord, ET_RENDERINGINTENT) || (pEnumRecord->dwRenderingIntent == header.phRenderingIntent)) &&
        (!SET(pEnumRecord, ET_CREATOR)         || (pEnumRecord->dwCreator         == header.phCreator)))
    {
        rc = EXACT_MATCH;
    }

    //
    // Check for resolution, media type and halftoning match
    //

    if (rc != NOMATCH && SET(pEnumRecord, ET_RESOLUTION|ET_MEDIATYPE|ET_DITHERMODE))
    {
        rc = CheckResMedHftnMatch(hProfile, pEnumRecord);
    }

EndDoesProfileMatchEnumRecord:
    if (hProfile)
    {
        CloseColorProfile(hProfile);
    }

    return rc;
}


/******************************************************************************
 *
 *                           CheckResMedHftnMatch
 *
 *  Function:
 *       This functions checks if a profile matches the resolution,
 *       media type and halftoning criteria specified by the enumeration record.
 *       It allows an exact match, as well as an ambiguous match. If the
 *       profile doesn't specify the criteria, it is considered to ambiguously
 *       match the specification.
 *       is desired.
 *
 *  Arguments:
 *       hProfile       - handle identifying profile
 *       pEnumRecord    - pointer to criteria to check against
 *
 *  Returns:
 *       MATCH or EXACT_MATCH if the profile matches the criteria, NOMATCH otherwise
 *
 ******************************************************************************/

MATCHTYPE
CheckResMedHftnMatch(
    HPROFILE   hProfile,
    PENUMTYPE  pEnumRecord
    )
{
    PDEVICESETTINGS   pDevSettings = NULL;
    PPLATFORMENTRY    pPlatform;
    PSETTINGCOMBOS    pCombo;
    PSETTINGS         pSetting;
    DWORD             dwMSData[4];
    DWORD             dwSize, i, iMax, j, jMax;
    MATCHTYPE         rc = MATCH;       // Assume ambiguous match
    BOOL              bReference;

    //
    // Check if the profile has the new device settings tag
    //

    dwSize = 0;
    GetColorProfileElement(hProfile, TAG_DEVICESETTINGS, 0, &dwSize, NULL, &bReference);

    if (dwSize > 0)
    {
        if (!(pDevSettings = (PDEVICESETTINGS)GlobalAllocPtr(GHND, dwSize)))
        {
            WARNING((__TEXT("Error allocating memory\n")));
            return NOMATCH;
        }

        if (GetColorProfileElement(hProfile, TAG_DEVICESETTINGS, 0, &dwSize, (PBYTE)pDevSettings, &bReference))
        {
            pPlatform = &pDevSettings->PlatformEntry[0];

            //
            // Navigate to the place where Microsoft specific settings are kept
            //

            i = 0;
            iMax = FIX_ENDIAN(pDevSettings->nPlatforms);
            while ((i < iMax) && (pPlatform->PlatformID != ID_MSFT_REVERSED))

            {
                i++;
                pPlatform = (PPLATFORMENTRY)((PBYTE)pPlatform + FIX_ENDIAN(pPlatform->dwSize));
            }

            if (i >= iMax)
            {
                //
                // There are no MS specific settings, assume this profile is valid
                // for all settings (ambigous match)
                //

                goto EndCheckResMedHftnMatch;
            }

            //
            // Found MS specific data. Now go through each combination of settings
            //

            pCombo = &pPlatform->SettingCombos[0];
            iMax = FIX_ENDIAN(pPlatform->nSettingCombos);
            for (i=0; i<iMax; i++)
            {
                //
                // Go through each setting in the combination
                //

                pSetting = &pCombo->Settings[0];
                jMax = FIX_ENDIAN(pCombo->nSettings);
                for (j=0; j<jMax; j++)
                {
                    if (pSetting->dwSettingType == ID_MEDIATYPE_REVERSED)
                    {
                        if (SET(pEnumRecord, ET_MEDIATYPE) &&
                            !DwordMatches(pSetting, pEnumRecord->dwMediaType))
                        {
                            goto NextCombo;
                        }
                    }
                    else if (pSetting->dwSettingType == ID_DITHER_REVERSED)
                    {
                        if (SET(pEnumRecord, ET_DITHERMODE) &&
                            !DwordMatches(pSetting, pEnumRecord->dwDitheringMode))
                        {
                            goto NextCombo;
                        }
                    }
                    else if (pSetting->dwSettingType == ID_RESLN_REVERSED)
                    {
                        if (SET(pEnumRecord, ET_RESOLUTION) &&
                            !QwordMatches(pSetting, &pEnumRecord->dwResolution[0]))
                        {
                            goto NextCombo;
                        }
                    }

                    pSetting = (PSETTINGS)((PBYTE)pSetting + sizeof(SETTINGS) - sizeof(DWORD) +
                                        FIX_ENDIAN(pSetting->dwSizePerValue) * FIX_ENDIAN(pSetting->nValues));
                }

                //
                // This combination worked!
                //

                rc = EXACT_MATCH;
                goto EndCheckResMedHftnMatch;

            NextCombo:
                pCombo = (PSETTINGCOMBOS)((PBYTE)pCombo + FIX_ENDIAN(pCombo->dwSize));
            }

            rc = NOMATCH;
            goto EndCheckResMedHftnMatch;

        }
        else
        {
            rc = NOMATCH;
            goto EndCheckResMedHftnMatch;
        }
    }
    else
    {
        //
        // Check if the old MSxx tags are present
        //

        dwSize = sizeof(dwMSData);
        if (SET(pEnumRecord, ET_MEDIATYPE))
        {
            if (GetColorProfileElement(hProfile, TAG_MS01, 0, &dwSize, dwMSData, &bReference))
            {
                rc = EXACT_MATCH;       // Assume exact match

                if (pEnumRecord->dwMediaType != FIX_ENDIAN(dwMSData[2]))
                {
                    return NOMATCH;
                }
            }
        }

        dwSize = sizeof(dwMSData);
        if (SET(pEnumRecord, ET_DITHERMODE))
        {
            if (GetColorProfileElement(hProfile, TAG_MS02, 0, &dwSize, dwMSData, &bReference))
            {
                rc = EXACT_MATCH;       // Assume exact match

                if (pEnumRecord->dwDitheringMode != FIX_ENDIAN(dwMSData[2]))
                {
                    return NOMATCH;
                }
            }
        }

        dwSize = sizeof(dwMSData);
        if (SET(pEnumRecord, ET_RESOLUTION))
        {
            if (GetColorProfileElement(hProfile, TAG_MS03, 0, &dwSize, dwMSData, &bReference))
            {
                rc = EXACT_MATCH;       // Assume exact match

                if (pEnumRecord->dwResolution[0] != FIX_ENDIAN(dwMSData[2]) ||
                    pEnumRecord->dwResolution[1] != FIX_ENDIAN(dwMSData[3]))
                {
                    return NOMATCH;
                }
            }
        }
    }

EndCheckResMedHftnMatch:

    if (pDevSettings)
    {
        GlobalFreePtr(pDevSettings);
    }

    return rc;
}


BOOL
DwordMatches(
    PSETTINGS    pSetting,
    DWORD        dwValue
    )
{
    DWORD  i, iMax;
    PDWORD pValue;

    dwValue = FIX_ENDIAN(dwValue);  // so we don't have to do this in the loop

    //
    // Go through all the values. If any of them match, return TRUE.
    //

    pValue = &pSetting->Value[0];
    iMax = FIX_ENDIAN(pSetting->nValues);
    for (i=0; i<iMax; i++)
    {
        if (dwValue == *pValue)
        {
            return TRUE;
        }

        pValue++;                   // We know that it is a DWORD
    }

    return FALSE;
}


BOOL
QwordMatches(
    PSETTINGS    pSetting,
    PDWORD       pdwValue
    )
{
    DWORD  i, iMax, dwValue1, dwValue2;
    PDWORD pValue;

    dwValue1 = FIX_ENDIAN(*pdwValue);  // so we don't have to do this in the loop
    dwValue2 = FIX_ENDIAN(*(pdwValue+1));

    //
    // Go through all the values. If any of them match, return TRUE.
    //

    pValue = &pSetting->Value[0];
    iMax = FIX_ENDIAN(pSetting->nValues);
    for (i=0; i<iMax; i++)
    {
        if ((dwValue1 == *pValue) && (dwValue2 == *(pValue + 1)))
        {
            return TRUE;
        }

        pValue += 2;                   // We know that it is a QWORD
    }

    return FALSE;
}


/******************************************************************************
 *
 *                              OpenPrtr
 *
 *  Function:
 *       On Memphis, we cannot call OpenPrinter() because it calls into 16-bit
 *       code, so if we call this function from GDI-16, we deadlock. So we
 *       look into the registry directly.
 *
 *  Arguments:
 *       pDeviceName     - pointer to name of the device
 *       phDevice        - pointer that receives the handle.
 *       pDummy          - dummy parameter
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI
OpenPrtr(
    PTSTR    pDeviceName,
    LPHANDLE phDevice,
    PTSTR    pDummy
    )
{
#if !defined(_WIN95_)
    return OpenPrinter(pDeviceName, phDevice, (LPPRINTER_DEFAULTS)pDummy);
#else
    HKEY    hkDevice = NULL;        // printers branch of registry
    HKEY    hkPrtr   = NULL;        // Friendly name branch of registry
    DWORD   dwErr;                  // error code
    BOOL    rc = FALSE;             // return code

    *phDevice = NULL;

    if (((dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, gszRegPrinter, &hkDevice)) != ERROR_SUCCESS) ||
        ((dwErr = RegOpenKey(hkDevice, pDeviceName, &hkPrtr)) != ERROR_SUCCESS) ||
        ((dwErr = RegOpenKey(hkPrtr, gszPrinterData, (HKEY *)phDevice)) != ERROR_SUCCESS))
    {
        WARNING((__TEXT("Cannot open printer data branch of registry for %s: %d\n"), pDeviceName, dwErr));
        SetLastError(dwErr);
        goto EndOpenPrtr;
    }

    rc = TRUE;

EndOpenPrtr:
    if (hkDevice)
    {
        RegCloseKey(hkDevice);
    }
    if (hkPrtr)
    {
        RegCloseKey(hkPrtr);
    }

    return rc;
#endif
}


/******************************************************************************
 *
 *                              ClosePrtr
 *
 *  Function:
 *       This function closes the printer handle opened by OpenPrtr.
 *
 *  Arguments:
 *       hDevice         - open handle
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/


BOOL WINAPI
ClosePrtr(
    HANDLE hDevice
    )
{
#if !defined(_WIN95_)
    return ClosePrinter(hDevice);
#else
    DWORD dwErr;

    dwErr = RegCloseKey((HKEY)hDevice);
    SetLastError(dwErr);
    return dwErr == ERROR_SUCCESS;
#endif
}


/******************************************************************************
 *
 *                              GetPrtrData
 *
 *  Function:
 *       This functions returns ICM data stored with the printer instance
 *
 *  Arguments:
 *       hDevice         - open printer handle
 *       pKey            - registry key for compatibility with GetPrinterDataEx
 *       pName           - name of registry value
 *       pdwType         - pointer to dword that receives type  of value
 *       pData           - pointer to  buffer to receive data
 *       dwSize          - size of buffer
 *       pdwNeeded       - on return, this has size of buffer filled/needed
 *
 *  Returns:
 *       ERROR_SUCCESS if successful, error code otherwise
 *
 ******************************************************************************/

DWORD WINAPI
GetPrtrData(
    HANDLE hDevice,
    PTSTR  pKey,
    PTSTR  pName,
    PDWORD pdwType,
    PBYTE  pData,
    DWORD  dwSize,
    PDWORD pdwNeeded
    )
{
#if !defined(_WIN95_)
    return GetPrinterDataEx(hDevice, pKey, pName, pdwType, pData, dwSize, pdwNeeded);
#else
    *pdwNeeded = dwSize;

    return RegQueryValueEx((HKEY)hDevice, pName, 0, NULL, pData, pdwNeeded);
#endif
}


/******************************************************************************
 *
 *                              SetPrtrData
 *
 *  Function:
 *       This functions stores ICM data with the printer instance
 *
 *  Arguments:
 *       hDevice         - open printer handle
 *       pKey            - registry key for compatibility with SetPrinterDataEx
 *       pName           - name of registry value
 *       dwType          - type  of value
 *       pData           - pointer to  data buffer
 *       dwSize          - size of buffer
 *
 *  Returns:
 *       ERROR_SUCCESS if successful, error code otherwise
 *
 ******************************************************************************/

DWORD WINAPI
SetPrtrData(
    HANDLE hDevice,
    PTSTR  pKey,
    PTSTR  pName,
    DWORD  dwType,
    PBYTE  pData,
    DWORD  dwSize
    )
{
#if !defined(_WIN95_)
    return SetPrinterDataEx(hDevice, pKey, pName, dwType, pData, dwSize);
#else
    return RegSetValueEx((HKEY)hDevice, pName, 0, dwType, pData, dwSize);
#endif
}


/******************************************************************************
 *
 *                              OpenMonitor
 *
 *  Function:
 *       This function returns a handle to the monitor
 *
 *  Arguments:
 *       pDeviceName     - pointer to name of the device
 *       phDevice        - pointer that receives the handle.
 *       pDummy          - dummy parameter
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI
OpenMonitor(
    PTSTR    pDeviceName,
    LPHANDLE phDevice,
    PTSTR    pDummy
    )
{
#ifdef _WIN95_

    //
    // For Windows 9x platform.
    //

    HDEVINFO        hDevInfo = INVALID_HANDLE_VALUE;
    HKEY            hkICM = NULL;
    HKEY            hkDriver = NULL;        // software branch of registry
    DWORD           dwSize;                 // size of buffer
    TCHAR           szName[MAX_PATH];       // buffer
    BOOL            rc = FALSE;             // return value
    SP_DEVINFO_DATA spdid;
    int             i;                      // instance counter

    if (!LoadSetupAPIDll())
    {
        WARNING((__TEXT("Error loading setupapi.dll: %d\n"), GetLastError()));
        return FALSE;
    }

    hDevInfo = (*fpSetupDiGetClassDevs)((LPGUID)&GUID_DEVCLASS_MONITOR, NULL,  NULL, DIGCF_PRESENT);

    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        WARNING((__TEXT("Error getting hDevInfo: %d\n"), GetLastError()));
        goto EndOpenMonitor;
    }

    i = 0;
    while (! rc)
    {
        ZeroMemory(&spdid, sizeof(SP_DEVINFO_DATA));
        spdid.cbSize = sizeof(SP_DEVINFO_DATA);
        if (! (*fpSetupDiEnumDeviceInfo)(hDevInfo, i, &spdid))
        {
            if (i == 0 && !lstrcmpi(pDeviceName, gszDisplay))
            {
                //
                // PnP support not in - open ICM key in registry
                //

                TCHAR szICMMonitorData[] = __TEXT("ICMMonitorData");

                WARNING((__TEXT("PnP support absent - Using DISPLAY\n")));

                //
                // Open the registry path where monitor data is kept
                //

                if ((RegOpenKey(HKEY_LOCAL_MACHINE, gszICMRegPath, &hkICM) != ERROR_SUCCESS) ||
                    (RegCreateKey(hkICM, szICMMonitorData, &hkDriver) != ERROR_SUCCESS))
                {
                    WARNING((__TEXT("Cannot open ICMMonitorData branch of registry\n")));
                    goto EndOpenMonitor;
                }
                rc = TRUE;
            }
            break;
        }

        //
        // Get PnP ID. Check and see if the monitor name matches it.
        //

        dwSize = sizeof(szName);
        if ((*fpSetupDiGetDeviceInstanceId)(hDevInfo, &spdid, szName, dwSize, NULL) &&
            ! lstrcmp(szName, pDeviceName))
        {
            hkDriver = (*fpSetupDiOpenDevRegKey)(hDevInfo, &spdid, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_ALL_ACCESS);
            if (hkDriver == INVALID_HANDLE_VALUE)
            {
                WARNING((__TEXT("Could not open monitor s/w key for all access\n")));
                hkDriver = (*fpSetupDiOpenDevRegKey)(hDevInfo, &spdid, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
                if (hkDriver == INVALID_HANDLE_VALUE)
                {
                    WARNING((__TEXT("Error opening s/w registry key for read access: %x\n"), GetLastError()));
                    goto EndOpenMonitor;
                }
            }

            rc = TRUE;
        }

        i++;
    }

EndOpenMonitor:

    if (hkICM)
    {
        RegCloseKey(hkICM);
    }

    if (hDevInfo != INVALID_HANDLE_VALUE)
    {
        (*fpSetupDiDestroyDeviceInfoList)(hDevInfo);
    }

    *phDevice = (HANDLE)hkDriver;

    return rc;

#else

    //
    // For Windows NT (later than 5.0) platform
    //

    TCHAR  szRegPath[MAX_PATH];
    HKEY   hkDriver = NULL;

    //
    // Copy device class root key.
    //

    lstrcpy(szRegPath,gszDeviceClass);
    lstrcat(szRegPath,gszMonitorGUID);

    if (!lstrcmpi(pDeviceName, gszDisplay))
    {
        WARNING((__TEXT("PnP support absent - Using DISPLAY\n")));

        //
        // PnP support not in -  just open "0000" device.
        //

        lstrcat(szRegPath,TEXT("\\0000"));
    }
    else if (_tcsstr(pDeviceName,gszMonitorGUID))
    {
        //
        // Extract monitor number from DeviceName
        //

        TCHAR *pDeviceNumber = _tcsrchr(pDeviceName,TEXT('\\'));

        if (pDeviceNumber)
        {
            lstrcat(szRegPath,pDeviceNumber);
        }
        else
        {
            lstrcat(szRegPath,TEXT("\\0000"));
        }
    }
    else
    {
        //
        // This is not valid monitor name.
        //
        goto EndOpenMonitor;
    }

    //
    // Open the registry path where monitor data is kept
    //

    if (RegOpenKey(HKEY_LOCAL_MACHINE, szRegPath, &hkDriver) != ERROR_SUCCESS)
    {
        WARNING((__TEXT("Cannot open %s key\n"),szRegPath));
        hkDriver = NULL;
    }

EndOpenMonitor:

    *phDevice = (HANDLE) hkDriver;

    return (hkDriver != NULL);

#endif // _WIN95_
}


/******************************************************************************
 *
 *                              CloseMonitor
 *
 *  Function:
 *       This function closes the monitor handle opened by OpenMonitor
 *
 *  Arguments:
 *       hDevice         - open handle
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI
CloseMonitor(
    HANDLE hDevice
    )
{
    DWORD dwErr;

    dwErr = RegCloseKey((HKEY)hDevice);
    SetLastError(dwErr);
    return (dwErr == ERROR_SUCCESS);
}


/******************************************************************************
 *
 *                              GetMonitorData
 *
 *  Function:
 *       This functions returns ICM data stored with the monitor instance
 *
 *  Arguments:
 *       hDevice         - open monitor handle
 *       pKey            - registry key for compatibility with GetPrinterDataEx
 *       pName           - name of registry value
 *       pdwType         - pointer to dword that receives type  of value
 *       pData           - pointer to  buffer to receive data
 *       dwSize          - size of buffer
 *       pdwNeeded       - on return, this has size of buffer filled/needed
 *
 *  Returns:
 *       ERROR_SUCCESS if successful, error code otherwise
 *
 ******************************************************************************/

DWORD WINAPI
GetMonitorData(
    HANDLE hDevice,
    PTSTR  pKey,
    PTSTR  pName,
    PDWORD pdwType,
    PBYTE  pData,
    DWORD  dwSize,
    PDWORD pdwNeeded
    )
{
    DWORD    dwType, dwTemp;
    DWORD    rc;

    *pdwNeeded = dwSize;

    rc = RegQueryValueEx((HKEY)hDevice, pName, 0, &dwType, pData, pdwNeeded);
    if (rc == ERROR_SUCCESS || rc == ERROR_MORE_DATA)
    {
        if (dwType == REG_SZ)
        {
            PTSTR pFilename;

            //
            // Old style value, convert to double null terminated binary
            //

            if (pData)
            {
                pFilename = GetFilenameFromPath((PTSTR)pData);
                if (pFilename != (PTSTR)pData)
                {
                    lstrcpy((PTSTR)pData, pFilename);
                }
                *pdwNeeded = lstrlen((PTSTR)pData) * sizeof(TCHAR);
            }

            *pdwNeeded += sizeof(TCHAR);    // for double NULL termination

            if ((dwSize >= *pdwNeeded) && pData)
            {
                *((PTSTR)pData + lstrlen((PTSTR)pData) + 1) = '\0';

                //
                // Set the profile name in new format
                //

                RegSetValueEx((HKEY)hDevice, pName, 0, REG_BINARY, pData, (lstrlen((PTSTR)pData)+2)*sizeof(TCHAR));
            }
        }
        else if (*pdwNeeded == 1)
        {
            //
            // If we have picked up the data and it is a 1 byte non-zero
            // value, it is an 1 based index in a list of
            // predefined profiles. Deal with this case.
            //
            // If pData is NULL, then we don't know if it is non-zero or
            // not, so we assume it is and ask for a large enough buffer.
            //

            if (!pData || *pData != 0)
            {
                //
                // Old style 1-based index value
                //

                if ((dwSize >= MAX_PATH) && pData)
                {
                    HKEY     hkICM = NULL;
                    HKEY     hkDevice = NULL;
                    REGDATA  regData;

                    //
                    // Make sure buggy inf doesn't crash us
                    //

                    if (pData[0] > sizeof(gszDispProfiles)/sizeof(gszDispProfiles[0]))
                    {
                        WARNING((__TEXT("Predefined profile index too large: %d\n"), pData[0]));
                        goto EndCompatMode;
                    }

                    lstrcpy((PTSTR)pData, gszDispProfiles[pData[0] - 1]);
                    *((PTSTR)pData + lstrlen((PTSTR)pData) + 1) = '\0';

                    //
                    // We need to update reference count as it wasn't set up
                    // using the new API
                    //
                    // Open the registry path where profiles are kept
                    //

                    if ((RegCreateKey(HKEY_LOCAL_MACHINE, gszICMRegPath, &hkICM) != ERROR_SUCCESS) ||
                        (RegCreateKey(hkICM, __TEXT("mntr"), &hkDevice) != ERROR_SUCCESS))
                    {
                        WARNING((__TEXT("Cannot open ICM\\device branch of registry\n")));
                        goto EndCompatMode;
                    }

                    //
                    // If registry data exists, then the profile is already installed,
                    // in which case, increment use count, otherwise add entry
                    //

                    dwTemp = sizeof(REGDATA);
                    if (RegQueryValueEx(hkDevice, (PTSTR)pData, 0, NULL, (PBYTE)&regData,
                            &dwTemp) == ERROR_SUCCESS)
                    {
                        regData.dwRefCount++;
                    }
                    else
                    {
                        regData.dwRefCount = 1;
                        regData.dwManuID = 'enon';  // it is our profile
                        regData.dwModelID = 'enon';
                    }

                    if (RegSetValueEx(hkDevice, (PTSTR)pData, 0, REG_BINARY,
                            (PBYTE)&regData, sizeof(REGDATA)) != ERROR_SUCCESS)
                    {
                        WARNING((__TEXT("Error setting registry value\n")));
                        goto EndCompatMode;
                    }

                    //
                    // Set the profile name in new format
                    //

                    RegSetValueEx((HKEY)hDevice, pName, 0, REG_BINARY, pData,
                                  (lstrlen((PTSTR)pData) + 2)*sizeof(TCHAR));

                EndCompatMode:
                    if (hkICM)
                    {
                        RegCloseKey(hkICM);
                    }
                    if (hkDevice)
                    {
                        RegCloseKey(hkDevice);
                    }
                }
                *pdwNeeded = MAX_PATH;
            }
        }
    }

    if ((rc == ERROR_SUCCESS) && (*pdwNeeded > dwSize))
    {
        rc = ERROR_MORE_DATA;
    }

    return rc;
}


/******************************************************************************
 *
 *                              SetMonitorData
 *
 *  Function:
 *       This functions stores ICM data with the monitor instance
 *
 *  Arguments:
 *       hDevice         - open monitor handle
 *       pKey            - registry key for compatibility with SetPrinterDataEx
 *       pName           - name of registry value
 *       dwType          - type  of value
 *       pData           - pointer to  data buffer
 *       dwSize          - size of buffer
 *
 *  Returns:
 *       ERROR_SUCCESS if successful, error code otherwise
 *
 ******************************************************************************/

DWORD WINAPI
SetMonitorData(
    HANDLE hDevice,
    PTSTR  pKey,
    PTSTR  pName,
    DWORD  dwType,
    PBYTE  pData,
    DWORD  dwSize
    )
{
    return RegSetValueEx((HKEY)hDevice, pName, 0, dwType, pData, dwSize);
}


/******************************************************************************
 *
 *                              OpenScanner
 *
 *  Function:
 *       This function returns a handle to the scanner
 *
 *  Arguments:
 *       pDeviceName     - pointer to name of the device
 *       phDevice        - pointer that receives the handle.
 *       pDummy          - dummy parameter
 *
 *  Returns:
 *       TRUE
 *
 ******************************************************************************/

BOOL WINAPI
OpenScanner(
    PTSTR    pDeviceName,
    LPHANDLE phDevice,
    PTSTR    pDummy
    )
{
    PFNSTICREATEINSTANCE pStiCreateInstance;
    PSCANNERDATA         psd = NULL;
    HRESULT              hres;
    BOOL                 bRc = FALSE;

    if (!(psd = (PSCANNERDATA)MemAlloc(sizeof(SCANNERDATA))))
    {
        WARNING((__TEXT("Error allocating memory for scanner data\n")));
        return FALSE;
    }

    if (!(psd->pDeviceName = MemAlloc((lstrlen(pDeviceName) + 1) * sizeof(WCHAR))))
    {
        WARNING((__TEXT("Error allocating memory for scanner name\n")));
        goto EndOpenScanner;
    }

    #ifdef UNICODE
    lstrcpy(psd->pDeviceName, pDeviceName);
    #else
    if (! ConvertToUnicode(pDeviceName, &psd->pDeviceName, FALSE))
    {
        WARNING((__TEXT("Error converting scanner name to Unicode\n")));
        goto EndOpenScanner;
    }
    #endif

    if (!(psd->hModule = LoadLibrary(gszStiDll)))
    {
        WARNING((__TEXT("Error loading sti.dll: %d\n"), GetLastError()));
        goto EndOpenScanner;
    }

    if (!(pStiCreateInstance = (PFNSTICREATEINSTANCE)GetProcAddress(psd->hModule, gszStiCreateInstance)))
    {
        WARNING((__TEXT("Error getting proc StiCreateInstance\n")));
        goto EndOpenScanner;
    }

    hres = (*pStiCreateInstance)(GetModuleHandle(NULL), STI_VERSION, &psd->pSti, NULL);

    if (FAILED(hres))
    {
        WARNING((__TEXT("Error creating sti instance: %d\n"), hres));
        goto EndOpenScanner;
    }

    *phDevice = (HANDLE)psd;

    bRc = TRUE;

EndOpenScanner:

    if (!bRc && psd)
    {
        CloseScanner((HANDLE)psd);
    }

    return bRc;
}


/******************************************************************************
 *
 *                              CloseScanner
 *
 *  Function:
 *       This function closes the monitor handle opened by OpenMonitor
 *
 *  Arguments:
 *       hDevice        - handle of device
 *
 *  Returns:
 *       TRUE
 *
 ******************************************************************************/

BOOL WINAPI
CloseScanner(
    HANDLE hDevice
    )
{
    PSCANNERDATA psd = (PSCANNERDATA)hDevice;

    if (psd)
    {
        if (psd->pSti)
        {
            psd->pSti->lpVtbl->Release(psd->pSti);
        }

        if (psd->pDeviceName)
        {
            MemFree(psd->pDeviceName);
        }

        if (psd->hModule)
        {
            FreeLibrary(psd->hModule);
        }

        MemFree(psd);
    }

    return TRUE;
}


/******************************************************************************
 *
 *                              GetScannerData
 *
 *  Function:
 *       This functions returns ICM data stored with the scannerr instance
 *
 *  Arguments:
 *       hDevice         - open scanner handle
 *       pKey            - registry key for compatibility with GetPrinterDataEx
 *       pName           - name of registry value
 *       pdwType         - pointer to dword that receives type  of value
 *       pData           - pointer to  buffer to receive data
 *       dwSize          - size of buffer
 *       pdwNeeded       - on return, this has size of buffer filled/needed
 *
 *  Returns:
 *       ERROR_SUCCESS if successful, error code otherwise
 *
 ******************************************************************************/

DWORD WINAPI
GetScannerData(
    HANDLE hDevice,
    PTSTR  pKey,
    PTSTR  pName,
    PDWORD pdwType,
    PBYTE  pData,
    DWORD  dwSize,
    PDWORD pdwNeeded
    )
{
    PSCANNERDATA psd = (PSCANNERDATA)hDevice;
    HRESULT      hres;
#ifndef UNICODE
    PWSTR        pwszName;

    //
    // STI interface "ALWAYS" expects Unicode.
    //
    hres = ConvertToUnicode(pName, &pwszName, TRUE);

    if (!hres)
    {
        return (ERROR_INVALID_PARAMETER);
    }

    pName = (PSTR)pwszName;

#endif

    *pdwNeeded = dwSize;

    hres = psd->pSti->lpVtbl->GetDeviceValue(psd->pSti, psd->pDeviceName, (PWSTR)pName, pdwType, pData, pdwNeeded);

#ifndef UNICODE

    MemFree(pwszName);

#endif

    return hres;
}


/******************************************************************************
 *
 *                              SetScannerData
 *
 *  Function:
 *       This functions stores ICM data with the scanner instance
 *
 *  Arguments:
 *       hDevice         - open scanner handle
 *       pKey            - registry key for compatibility with SetPrinterDataEx
 *       pName           - name of registry value
 *       dwType          - type  of value
 *       pData           - pointer to  data buffer
 *       dwSize          - size of buffer
 *
 *  Returns:
 *       ERROR_SUCCESS if successful, error code otherwise
 *
 ******************************************************************************/

DWORD WINAPI
SetScannerData(
    HANDLE hDevice,
    PTSTR  pKey,
    PTSTR  pName,
    DWORD  dwType,
    PBYTE  pData,
    DWORD  dwSize
    )
{
    PSCANNERDATA psd = (PSCANNERDATA)hDevice;
    HRESULT      hres;
#ifndef UNICODE
    PWSTR        pwszName;

    //
    // STI interface "ALWAYS" expects Unicode.
    //
    hres = ConvertToUnicode(pName, &pwszName, TRUE);

    if (!hres)
    {
        return (ERROR_INVALID_PARAMETER);
    }

    pName = (PSTR)pwszName;

#endif

    hres = psd->pSti->lpVtbl->SetDeviceValue(psd->pSti, psd->pDeviceName, (PWSTR)pName, dwType, pData, dwSize);

#ifndef UNICODE

    MemFree(pwszName);

#endif

    return hres;
}

//
// Internal functions
//

BOOL WINAPI
InternalGetDeviceConfig(
    LPCTSTR pDeviceName,
    DWORD   dwDeviceClass,
    DWORD   dwConfigType,
    PVOID   pConfigData,
    PDWORD  pdwSize
    )
{
    DWORD   dwDataType;
    DWORD   dwSizeRequired = 0;
    BOOL    rc = FALSE;

    switch (dwConfigType)
    {
    case MSCMS_PROFILE_ENUM_MODE:

        dwDataType = DEVICE_PROFILE_ENUMMODE;
        break;

    default:

        WARNING((__TEXT("Invalid parameter to InternalGetDeviceConfig\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Query the size of the data.
    //

    if (GetDeviceData(pDeviceName,dwDeviceClass,dwDataType,NULL,&dwSizeRequired,FALSE))
    {
        if ((dwSizeRequired <= *pdwSize) && (pConfigData != NULL))
        {
            //
            // If buffer is enough, get the data.
            //

            if (GetDeviceData(pDeviceName,dwDeviceClass,dwDataType,
                              (PVOID *)&pConfigData,pdwSize,FALSE))
            {
                rc = TRUE;
            }
            else
            {
                WARNING((__TEXT("Failed on GetDeviceData to query data\n")));
                SetLastError(ERROR_INVALID_PARAMETER);
            }
        }
        else
        {
            //
            // Return nessesary buffer size to caller.
            //

            *pdwSize = dwSizeRequired;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    }
    else
    {
        WARNING((__TEXT("Failed on GetDeviceData to query data size\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return rc;
}

BOOL WINAPI
InternalSetDeviceConfig(
    LPCTSTR pDeviceName,
    DWORD   dwDeviceClass,
    DWORD   dwConfigType,
    PVOID   pConfigData,
    DWORD   dwSize
    )
{
    DWORD   dwDataType;

    switch (dwConfigType)
    {
    case MSCMS_PROFILE_ENUM_MODE:

        dwDataType = DEVICE_PROFILE_ENUMMODE;
        break;

    default:

        WARNING((__TEXT("Invalid parameter to InternalGetDeviceConfig\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Save the data.
    //

    return (SetDeviceData(pDeviceName,dwDeviceClass,dwDataType,pConfigData,dwSize));
}

#ifdef _WIN95_

//
// Win9x specific functions are here.
//

BOOL
LoadSetupAPIDll(
    VOID
    )
{
    EnterCriticalSection(&critsec);

    if (ghModSetupAPIDll == NULL)
    {
        ghModSetupAPIDll = LoadLibrary(TEXT("setupapi.dll"));

        if (ghModSetupAPIDll)
        {
            fpSetupDiOpenDevRegKey = (FP_SetupDiOpenDevRegKey)
                GetProcAddress(ghModSetupAPIDll,"SetupDiOpenDevRegKey");
            fpSetupDiDestroyDeviceInfoList = (FP_SetupDiDestroyDeviceInfoList)
                GetProcAddress(ghModSetupAPIDll,"SetupDiDestroyDeviceInfoList");
            fpSetupDiEnumDeviceInfo = (FP_SetupDiEnumDeviceInfo)
                GetProcAddress(ghModSetupAPIDll,"SetupDiEnumDeviceInfo");
            fpSetupDiGetDeviceInstanceId = (FP_SetupDiGetDeviceInstanceId)
                GetProcAddress(ghModSetupAPIDll,"SetupDiGetDeviceInstanceIdA");
            fpSetupDiGetClassDevs = (FP_SetupDiGetClassDevs)
                GetProcAddress(ghModSetupAPIDll,"SetupDiGetClassDevsA");

            if ((fpSetupDiOpenDevRegKey == NULL) ||
                (fpSetupDiDestroyDeviceInfoList == NULL) ||
                (fpSetupDiEnumDeviceInfo == NULL) ||
                (fpSetupDiGetDeviceInstanceId == NULL) ||
                (fpSetupDiGetClassDevs == NULL))
            {
                WARNING((__TEXT("Could not find Export function in setupapi.dll\n")));

                FreeLibrary(ghModSetupAPIDll);
                ghModSetupAPIDll = NULL;
            }
        }
    }

    LeaveCriticalSection(&critsec);

    return (!!ghModSetupAPIDll);
}

#else

//
// Win NT specific functions are here.
//

VOID
ChangeICMSetting(
    LPCTSTR pMachineName,
    LPCTSTR pDeviceName,
    DWORD   dwICMMode
    )
{
    PRINTER_INFO_8   *ppi8;
    PRINTER_INFO_9   *ppi9;
    PRINTER_DEFAULTS pd;
    HANDLE           hPrinter;
    DWORD            dwSize;
    BYTE             temp[2*1024];    // sufficient for devmode

    pd.pDatatype = NULL;
    pd.pDevMode = NULL;
    pd.DesiredAccess = PRINTER_ALL_ACCESS;

    if (!OpenPrinter((PTSTR)pDeviceName, &hPrinter, &pd))
        return;

    //
    // Get and update system devmode
    //

    ppi8 = (PRINTER_INFO_8 *)&temp;
    if (GetPrinter(hPrinter, 8, (PBYTE)ppi8, sizeof(temp), &dwSize) &&
        ppi8->pDevMode)
    {
        switch (dwICMMode)
        {
        case ICM_ON:
        case ICM_OFF:
            ppi8->pDevMode->dmFields |= DM_ICMMETHOD;
            if (dwICMMode == ICM_ON)
                ppi8->pDevMode->dmICMMethod = DMICMMETHOD_SYSTEM;
            else
                ppi8->pDevMode->dmICMMethod = DMICMMETHOD_NONE;
            SetPrinter(hPrinter, 8, (PBYTE)ppi8, 0);
            break;
        }
    }

    //
    // If the user has a per-user devmode, update this as well
    //

    ppi9 = (PRINTER_INFO_9 *)&temp;
    if (GetPrinter(hPrinter, 9, (PBYTE)ppi9, sizeof(temp), &dwSize) &&
        ppi9->pDevMode)
    {
        switch (dwICMMode)
        {
        case ICM_ON:
        case ICM_OFF:
            ppi9->pDevMode->dmFields |= DM_ICMMETHOD;
            if (dwICMMode == ICM_ON)
                ppi9->pDevMode->dmICMMethod = DMICMMETHOD_SYSTEM;
            else
                ppi9->pDevMode->dmICMMethod = DMICMMETHOD_NONE;
            SetPrinter(hPrinter, 9, (PBYTE)ppi9, 0);
            break;
        }
    }

    ClosePrinter(hPrinter);

    return;
}

#endif // _WIN95_


