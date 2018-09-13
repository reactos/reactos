//
//  APITHK.H
//


#ifndef _APITHK_H_
#define _APITHK_H_

#include <appmgmt.h>
#include <aclapi.h>
#include <userenv.h>

STDAPI_(BOOL) NT5_EncryptFileW(LPCWSTR lpFileName);
STDAPI_(BOOL) NT5_DecryptFileW(LPCWSTR lpFileName, DWORD dwReserved);
STDAPI_(BOOL) NT5_GetVolumeNameForVolumeMountPointW(LPCWSTR lpszFileName, LPWSTR lpszVolumePathName, DWORD cchBufferLength);
STDAPI_(BOOL) NT5_GetVolumePathNameW(LPCWSTR lpszFileName, LPWSTR lpszVolumePathName,
                DWORD cchBufferLength);

STDAPI_(BOOL) NT5_UpdateLayeredWindow(HWND hwnd, HDC hdcDest, POINT* pptDest, SIZE* psize, 
                        HDC hdcSrc, POINT* pptSrc, COLORREF crKey, BLENDFUNCTION* pbf, DWORD dwFlags);

STDAPI_(BOOL) NT5_ConvertSidToStringSidW(
    IN   PSID               psid,
    OUT  LPTSTR *           StringSid
);

#define ConvertSidToStringSidW                  NT5_ConvertSidToStringSidW
#ifdef UNICODE
#define ConvertSidToStringSid                   ConvertSidToStringSidW
#else
#define ConvertSidToStringSid                   ConvertSidToStringSidA
#endif

STDAPI_(LANGID) NT5_GetSystemDefaultUILanguage();

STDAPI_(BOOL) NT5_ExpandEnvironmentStringsForUserW(HANDLE hToken, LPCWSTR lpSrc, LPWSTR lpDest, DWORD dwSize);

#define ExpandEnvironmentStringsForUserW        NT5_ExpandEnvironmentStringsForUserW
#ifdef UNICODE
#define ExpandEnvironmentStringsForUser         ExpandEnvironmentStringsForUserW
#else
#define ExpandEnvironmentStringsForUser         ExpandEnvironmentStringsForUserA
#endif


STDAPI_(BOOL) NT5_GetDefaultUserProfileDirectoryW(LPWSTR lpProfileDir, LPDWORD lpcchSize);

#define GetDefaultUserProfileDirectoryW         NT5_GetDefaultUserProfileDirectoryW
#ifdef UNICODE
#define GetDefaultUserProfileDirectory          GetDefaultUserProfileDirectoryW
#else
#define GetDefaultUserProfileDirectory          GetDefaultUserProfileDirectoryA
#endif


#undef MsiGetProductInfo
#define MsiGetProductInfo       MSI_MsiGetProductInfo
STDAPI_(UINT) MSI_MsiGetProductInfo(LPCTSTR szProduct, LPCTSTR szAttribute, LPTSTR lpValueBuf, DWORD* pcchValueBuf);

STDAPI_(BOOL) IsRemoteSession();

STDAPI_(BOOL) NT5_TermsrvAppInstallMode();
#define TermsrvAppInstallMode NT5_TermsrvAppInstallMode


// NOTE: we dont have thunks for the ANSI versions of Encrypt/Decrypt file
#define EncryptFileW                             NT5_EncryptFileW
#define DecryptFileW                             NT5_DecryptFileW

#define GetVolumeNameForVolumeMountPointW        NT5_GetVolumeNameForVolumeMountPointW                
#ifdef UNICODE
#define GetVolumeNameForVolumeMountPoint GetVolumeNameForVolumeMountPointW
#else
// For completeness we have this, but since this is for NT platform only, no code 
// is expected to call the Ansi version.
#define GetVolumeNameForVolumeMountPoint GetVolumeNameForVolumeMountPointA
#endif

#define GetVolumePathNameW                       NT5_GetVolumePathNameW
#ifdef UNICODE
#define GetVolumePathName GetVolumePathNameW
#else
// For completeness we have this, but since this is for NT platform only, no code 
// is expected to call the Ansi version.
#define GetVolumePathName GetVolumePathNameA
#endif

STDAPI_(DWORD) NT5_InstallApplication(PINSTALLDATA pInstallInfo);

#define InstallApplication      NT5_InstallApplication

BOOL IsDarwinAdW(LPCWSTR pszDescriptor);

#ifdef UNICODE
#define IsDarwinAd IsDarwinAdW
#else
#define IsDarwinAd FALSE
#endif

STDAPI_(BOOL) NT5_GlobalMemoryStatusEx(LPMEMORYSTATUSEX pmsex);
STDAPI_(BOOL) NT5_GetLongPathName(LPCTSTR pszShort, LPTSTR pszLong, DWORD cchBuf);

STDAPI_(HCURSOR) LoadHandCursor(DWORD dwRes) ;
STDAPI_(int)     GetCOLOR_HOTLIGHT() ;

// This is a Darwin Error code that may exist on Win9x. We don't wrap like normal.
#ifndef ERROR_SUCCESS_REBOOT_INITIATED
#define ERROR_SUCCESS_REBOOT_INITIATED 1641
#endif

// =========================================================================
// things supported on both Win98 and NT5

#define PrivateDEVICE_NOTIFY_WINDOW_HANDLE  0x00000000
#define PrivateDEVICE_NOTIFY_SERVICE_HANDLE 0x00000001

#if(WINVER < 0x0500)
typedef PVOID HDEVNOTIFY;
#define RegisterDeviceNotification      Win98_RegisterDeviceNotification
#define UnregisterDeviceNotification    Win98_UnregisterDeviceNotification
#define DEVICE_NOTIFY_WINDOW_HANDLE     PrivateDEVICE_NOTIFY_WINDOW_HANDLE
#define DEVICE_NOTIFY_SERVICE_HANDLE    PrivateDEVICE_NOTIFY_SERVICE_HANDLE
#else

#if PrivateDEVICE_NOTIFY_WINDOW_HANDLE  != DEVICE_NOTIFY_WINDOW_HANDLE || \
    PrivateDEVICE_NOTIFY_SERVICE_HANDLE != DEVICE_NOTIFY_SERVICE_HANDLE
#error inconsistant DEVICE_NOTIFY_xxx_HANDLE in winuser.h
#endif

#endif

STDAPI_(HDEVNOTIFY) Win98_RegisterDeviceNotification(
    IN HANDLE hRecipient,
    IN LPVOID NotificationFilter,
    IN DWORD Flags
    );

STDAPI_(BOOL) Win98_UnregisterDeviceNotification(
    IN HDEVNOTIFY Handle
    );

//
//  Compiler will yell at us if we made inconsistent definitions
//
#define DBT_DEVTYP_HANDLE               0x00000006

typedef struct _DEV_BROADCAST_HANDLE {
    DWORD       dbch_size;
    DWORD       dbch_devicetype;
    DWORD       dbch_reserved;
    HANDLE      dbch_handle;     // file handle used in call to RegisterDeviceNotification
    HDEVNOTIFY  dbch_hdevnotify; // returned from RegisterDeviceNotification
    //
    // The following 3 fields are only valid if wParam is DBT_CUSTOMEVENT.
    //
    GUID        dbch_eventguid;
    LONG        dbch_nameoffset; // offset (bytes) of variable-length string buffer (-1 if none)
    BYTE        dbch_data[1];    // variable-sized buffer, potentially containing binary and/or text data
} DEV_BROADCAST_HANDLE, *PDEV_BROADCAST_HANDLE;


#ifdef WINNT
// =========================================================================
// things supported since NT5

#define PrivateVOLUME_UPGRADE_SCHEDULED         (0x00000002)

#define KEYBOARDCUES
#ifdef KEYBOARDCUES
#define PrivateWM_CHANGEUISTATE     0x0127
#define PrivateWM_UPDATEUISTATE     0x0128
#define PrivateWM_QUERYUISTATE      0x0129
#define PrivateUIS_SET              1
#define PrivateUIS_CLEAR            2
#define PrivateUIS_INITIALIZE       3
#define PrivateUISF_HIDEFOCUS       0x1
#define PrivateUISF_HIDEACCEL       0x2
#endif //KEYBOARDCUES

#define PrivateULW_COLORKEY            0x00000001
#define PrivateULW_ALPHA               0x00000002
#define PrivateULW_OPAQUE              0x00000004
#define PrivateWS_EX_LAYERED           0x00080000



#if (_WIN32_WINNT >= 0x0500)

// for files in nt5api dirs, use the definition in sdk include.
// And make sure our private define is in sync with winuser.h.

#if WS_EX_LAYERED != PrivateWS_EX_LAYERED
#error inconsistant WS_EX_LAYERED in winuser.h
#endif

#else   // (_WIN32_WINNT >= 0x0500)

#define WS_EX_LAYERED           PrivateWS_EX_LAYERED
#define UpdateLayeredWindow     NT5_UpdateLayeredWindow 
#define ULW_COLORKEY            PrivateULW_COLORKEY
#define ULW_ALPHA               PrivateULW_ALPHA
#define ULW_OPAQUE              PrivateULW_OPAQUE
#define GetSystemDefaultUILanguage NT5_GetSystemDefaultUILanguage
#define WM_CHANGEUISTATE        PrivateWM_CHANGEUISTATE 
#define WM_UPDATEUISTATE        PrivateWM_UPDATEUISTATE 
#define WM_QUERYUISTATE         PrivateWM_QUERYUISTATE  
#define UIS_SET                 PrivateUIS_SET          
#define UIS_CLEAR               PrivateUIS_CLEAR        
#define UIS_INITIALIZE          PrivateUIS_INITIALIZE   
#define UISF_HIDEFOCUS          PrivateUISF_HIDEFOCUS
#define UISF_HIDEACCEL          PrivateUISF_HIDEACCEL   

#endif  // (_WIN32_WINNT >= 0x0500)


#if (WINVER >= 0x0500)
// And make sure our private define is in sync with winuser.h.
#if VOLUME_UPGRADE_SCHEDULED != PrivateVOLUME_UPGRADE_SCHEDULED
#error inconsistant VOLUME_UPGRADE_SCHEDULED in ntioapi.h
#endif

#else

#define VOLUME_UPGRADE_SCHEDULED    PrivateVOLUME_UPGRADE_SCHEDULED

#endif // (WINVER >= 0x0500)
#endif // WINNT

#ifdef WINNT

STDAPI_(UINT) NT5_GetSystemWindowsDirectoryW(LPWSTR pszBuffer, UINT cchBuff);

// NOTE: we only thunk the W version right now since this is NT5 only
// and we are UNICODE on NT5. For the ansi version, we just #define to error 
// now and hope no one tries to calls it ;-)
#define GetSystemWindowsDirectoryW NT5_GetSystemWindowsDirectoryW
#define GetSystemWindowsDirectoryA error
#else
// on win95 thunk this back to the old api
#ifdef GetSystemWindowsDirectory
#undef GetSystemWindowsDirectory
#define GetSystemWindowsDirectory GetWindowsDirectory
#endif // ifdef GetSystemWindowsDirectory
#endif // WINNT

LPTSTR GetEnvBlock(HANDLE hUserToken);
void FreeEnvBlock(HANDLE hUserToken, LPTSTR pszEnv);

//  IAccessible chit in winuser.h not defined WINVER < 0x0500, so...
STDAPI_(BOOL) IsWM_GETOBJECT( UINT uMsg );

STDAPI_(BOOL) GetAllUsersDirectory(LPTSTR pszPath);

#endif // _APITHK_H_
