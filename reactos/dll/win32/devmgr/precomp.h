#ifndef __DEVMGR_H
#define __DEVMGR_H

#include <stdarg.h>
#include <stdlib.h> 

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winnls.h>
#include <winuser.h>
#include <wchar.h>
#include <regstr.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <dll/newdevp.h>
#include <dll/devmgr/devmgr.h>
#include <wine/debug.h>

#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(devmgr);

extern HINSTANCE hDllInstance;

typedef INT_PTR (WINAPI *PPROPERTYSHEETW)(LPCPROPSHEETHEADERW);
typedef HPROPSHEETPAGE (WINAPI *PCREATEPROPERTYSHEETPAGEW)(LPCPROPSHEETPAGEW);
typedef BOOL (WINAPI *PDESTROYPROPERTYSHEETPAGE)(HPROPSHEETPAGE);

typedef struct _DEVADVPROP_INFO
{
    HWND hWndGeneralPage;
    HWND hWndParent;
    WNDPROC ParentOldWndProc;
    HICON hDevIcon;

    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    HDEVINFO CurrentDeviceInfoSet;
    SP_DEVINFO_DATA CurrentDeviceInfoData;
    DEVINST ParentDevInst;
    HMACHINE hMachine;
    LPCWSTR lpMachineName;

    HINSTANCE hComCtl32;
    PCREATEPROPERTYSHEETPAGEW pCreatePropertySheetPageW;
    PDESTROYPROPERTYSHEETPAGE pDestroyPropertySheetPage;

    DWORD PropertySheetType;
    DWORD nDevPropSheets;
    HPROPSHEETPAGE *DevPropSheets;

    union
    {
        UINT Flags;
        struct
        {
            UINT Extended : 1;
            UINT FreeDevPropSheets : 1;
            UINT CanDisable : 1;
            UINT DeviceStarted : 1;
            UINT DeviceUsageChanged : 1;
            UINT CloseDevInst : 1;
            UINT IsAdmin : 1;
            UINT DoDefaultDevAction : 1;
            UINT PageInitialized : 1;
            UINT ShowRemotePages : 1;
            UINT HasDriverPage : 1;
            UINT HasResourcePage : 1;
            UINT HasPowerPage : 1;
        };
    };

    WCHAR szDevName[255];
    WCHAR szTemp[255];
    WCHAR szDeviceID[1];
    /* struct may be dynamically expanded here! */
} DEVADVPROP_INFO, *PDEVADVPROP_INFO;


typedef struct _ENUMDRIVERFILES_CONTEXT
{
    HWND hDriversListView;
    UINT nCount;
} ENUMDRIVERFILES_CONTEXT, *PENUMDRIVERFILES_CONTEXT;

#define PM_INITIALIZE (WM_APP + 0x101)



/* HWRESOURCE.C */

INT_PTR
CALLBACK
ResourcesProcDriverDlgProc(IN HWND hwndDlg,
                     IN UINT uMsg,
                     IN WPARAM wParam,
                     IN LPARAM lParam);

/* ADVPROP.C */

INT_PTR
DisplayDeviceAdvancedProperties(IN HWND hWndParent,
                                IN LPCWSTR lpDeviceID  OPTIONAL,
                                IN HDEVINFO DeviceInfoSet,
                                IN PSP_DEVINFO_DATA DeviceInfoData,
                                IN HINSTANCE hComCtl32,
                                IN LPCWSTR lpMachineName,
                                IN DWORD dwFlags);

/* DEVPRBLM.C */

BOOL
ShowDeviceProblemWizard(IN HWND hWndParent  OPTIONAL,
                        IN HDEVINFO hDevInfo,
                        IN PSP_DEVINFO_DATA DevInfoData,
                        IN HMACHINE hMachine  OPTIONAL);

/* MISC.C */


INT
LengthOfStrResource(IN HINSTANCE hInst,
                    IN UINT uID);

DWORD
LoadAndFormatString(IN HINSTANCE hInstance,
                    IN UINT uID,
                    OUT LPWSTR *lpTarget,
                    ...);

DWORD
LoadAndFormatStringsCat(IN HINSTANCE hInstance,
                        IN UINT *uID,
                        IN UINT nIDs,
                        OUT LPWSTR *lpTarget,
                        ...);

LPARAM
ListViewGetSelectedItemData(IN HWND hwnd);

LPWSTR
ConvertMultiByteToUnicode(IN LPCSTR lpMultiByteStr,
                          IN UINT uCodePage);

HINSTANCE
LoadAndInitComctl32(VOID);

BOOL
GetDeviceManufacturerString(IN HDEVINFO DeviceInfoSet,
                            IN PSP_DEVINFO_DATA DeviceInfoData,
                            OUT LPWSTR szBuffer,
                            IN DWORD BufferSize);

BOOL
GetDeviceLocationString(IN HDEVINFO DeviceInfoSet,
                        IN PSP_DEVINFO_DATA DeviceInfoData,
                        IN DEVINST dnParentDevInst  OPTIONAL,
                        OUT LPWSTR szBuffer,
                        IN DWORD BufferSize);

BOOL
GetDeviceStatusString(IN DEVINST DevInst,
                      IN HMACHINE hMachine,
                      OUT LPWSTR szBuffer,
                      IN DWORD BufferSize);

BOOL
GetDriverProviderString(IN HDEVINFO DeviceInfoSet,
                        IN PSP_DEVINFO_DATA DeviceInfoData,
                        OUT LPWSTR szBuffer,
                        IN DWORD BufferSize);

BOOL
GetDriverVersionString(IN HDEVINFO DeviceInfoSet,
                       IN PSP_DEVINFO_DATA DeviceInfoData,
                       OUT LPWSTR szBuffer,
                       IN DWORD BufferSize);

BOOL
GetDriverDateString(IN HDEVINFO DeviceInfoSet,
                    IN PSP_DEVINFO_DATA DeviceInfoData,
                    OUT LPWSTR szBuffer,
                    IN DWORD BufferSize);

BOOL
IsDeviceHidden(IN DEVINST DevInst,
               IN HMACHINE hMachine,
               OUT BOOL *IsHidden);

BOOL
IsDriverInstalled(IN DEVINST DevInst,
                  IN HMACHINE hMachine,
                  OUT BOOL *Installed);

BOOL
CanDisableDevice(IN DEVINST DevInst,
                 IN HMACHINE hMachine,
                 OUT BOOL *CanDisable);

BOOL
IsDeviceStarted(IN DEVINST DevInst,
                IN HMACHINE hMachine,
                OUT BOOL *IsStarted);

BOOL
EnableDevice(IN HDEVINFO DeviceInfoSet,
             IN PSP_DEVINFO_DATA DevInfoData  OPTIONAL,
             IN BOOL bEnable,
             IN DWORD HardwareProfile  OPTIONAL,
             OUT BOOL *bNeedReboot  OPTIONAL);

BOOL
GetDeviceTypeString(IN PSP_DEVINFO_DATA DeviceInfoData,
                    OUT LPWSTR szBuffer,
                    IN DWORD BufferSize);

BOOL
GetDeviceDescriptionString(IN HDEVINFO DeviceInfoSet,
                           IN PSP_DEVINFO_DATA DeviceInfoData,
                           OUT LPWSTR szBuffer,
                           IN DWORD BufferSize);

BOOL
FindCurrentDriver(IN HDEVINFO DeviceInfoSet,
                  IN PSP_DEVINFO_DATA DeviceInfoData,
                  OUT PSP_DRVINFO_DATA DriverInfoData);

#endif /* __DEVMGR_H */
