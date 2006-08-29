#ifndef __DEVMGR_H
#define __DEVMGR_H

#include <windows.h>
#include <regstr.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <commctrl.h>
#include "resource.h"

extern HINSTANCE hDllInstance;

ULONG DbgPrint(PCCH Format,...);

BOOL
WINAPI
DeviceManager_ExecuteA(HWND hWndParent,
                       HINSTANCE hInst,
                       LPCSTR lpMachineName,
                       int nCmdShow);

BOOL
WINAPI
DeviceManager_ExecuteW(HWND hWndParent,
                       HINSTANCE hInst,
                       LPCWSTR lpMachineName,
                       int nCmdShow);

VOID
WINAPI
DeviceProperties_RunDLLA(HWND hWndParent,
                         HINSTANCE hInst,
                         LPCSTR lpDeviceCmd,
                         int nCmdShow);

VOID
WINAPI
DeviceProperties_RunDLLW(HWND hWndParent,
                         HINSTANCE hInst,
                         LPCWSTR lpDeviceCmd,
                         int nCmdShow);

int
WINAPI
DevicePropertiesA(HWND hWndParent,
                  HINSTANCE hInst,
                  LPCSTR lpMachineName,
                  LPCSTR lpDeviceID,
                  DWORD Unknown);

int
WINAPI
DevicePropertiesW(HWND hWndParent,
                  HINSTANCE hInst,
                  LPCWSTR lpMachineName,
                  LPCWSTR lpDeviceID,
                  DWORD Unknown);

UINT
WINAPI
DeviceProblemTextA(IN HMACHINE hMachine  OPTIONAL,
                   IN DEVINST dnDevInst,
                   IN ULONG uProblemId,
                   OUT LPSTR lpString,
                   IN UINT uMaxString);

UINT
WINAPI
DeviceProblemTextW(IN HMACHINE hMachine  OPTIONAL,
                   IN DEVINST dnDevInst,
                   IN ULONG uProblemId,
                   OUT LPWSTR lpString,
                   IN UINT uMaxString);

BOOL
WINAPI
DeviceProblemWizardA(IN HWND hWndParent  OPTIONAL,
                     IN LPCSTR lpMachineName  OPTIONAL,
                     IN LPCSTR lpDeviceID);


BOOL
WINAPI
DeviceProblemWizardW(IN HWND hWndParent  OPTIONAL,
                     IN LPCWSTR lpMachineName  OPTIONAL,
                     IN LPCWSTR lpDeviceID);

VOID
WINAPI
DeviceProblemWizard_RunDLLA(HWND hWndParent,
                            HINSTANCE hInst,
                            LPCSTR lpDeviceCmd,
                            int nCmdShow);

VOID
WINAPI
DeviceProblemWizard_RunDLLW(HWND hWndParent,
                            HINSTANCE hInst,
                            LPCWSTR lpDeviceCmd,
                            int nCmdShow);

#define DEV_PRINT_ABSTRACT	(0)
#define DEV_PRINT_SELECTED	(1)
#define DEV_PRINT_ALL	(2)

BOOL
WINAPI
DeviceManagerPrintA(LPCSTR lpMachineName,
                    LPCSTR lpPrinter,
                    int nPrintMode,
                    UINT uNumberOfGuids,
                    LPGUID lpGuids);

BOOL
WINAPI
DeviceManagerPrintW(LPCWSTR lpMachineName,
                    LPCWSTR lpPrinter,
                    int nPrintMode,
                    UINT uNumberOfGuids,
                    LPGUID lpGuids);

INT_PTR
WINAPI
DeviceAdvancedPropertiesA(IN HWND hWndParent  OPTIONAL,
                          IN LPCSTR lpMachineName  OPTIONAL,
                          IN LPCSTR lpDeviceID);

INT_PTR
WINAPI
DeviceAdvancedPropertiesW(IN HWND hWndParent  OPTIONAL,
                          IN LPCWSTR lpMachineName  OPTIONAL,
                          IN LPCWSTR lpDeviceID);

typedef enum
{
    HWPD_STANDARDLIST = 0,
    HWPD_LARGELIST,
    HWPD_MAX = HWPD_LARGELIST
} HWPAGE_DISPLAYMODE, *PHWPAGE_DISPLAYMODE;

HWND
WINAPI
DeviceCreateHardwarePage(HWND hWndParent,
                         LPGUID lpGuid);

HWND
WINAPI
DeviceCreateHardwarePageEx(IN HWND hWndParent,
                           IN LPGUID lpGuids,
                           IN UINT uNumberOfGuids,
                           IN HWPAGE_DISPLAYMODE DisplayMode);

#define DPF_UNKNOWN (0x1)
#define DPF_DEVICE_STATUS_ACTION    (0x2)
INT_PTR
WINAPI
DevicePropertiesExA(IN HWND hWndParent  OPTIONAL,
                    IN LPCSTR lpMachineName  OPTIONAL,
                    IN LPCSTR lpDeviceID  OPTIONAL,
                    IN DWORD dwFlags  OPTIONAL,
                    IN BOOL bShowDevMgr);

INT_PTR
WINAPI
DevicePropertiesExW(IN HWND hWndParent  OPTIONAL,
                    IN LPCWSTR lpMachineName  OPTIONAL,
                    IN LPCWSTR lpDeviceID  OPTIONAL,
                    IN DWORD dwFlags  OPTIONAL,
                    IN BOOL bShowDevMgr);

#ifdef UNICODE
#define DeviceManager_Execute DeviceManager_ExecuteW
#define DeviceProperties_RunDLL DeviceProperties_RunDLLW
#define DeviceProperties DevicePropertiesW
#define DeviceProblemText DeviceProblemTextW
#define DeviceProblemWizard DeviceProblemWizardW
#define DeviceProblemWizard_RunDLL DeviceProblemWizard_RunDLLW
#define DeviceManagerPrint DeviceManagerPrintW
#define DeviceAdvancedProperties DeviceAdvancedPropertiesW
#define DevicePropertiesEx DevicePropertiesExW
#else
#define DeviceManager_Execute DeviceManager_ExecuteA
#define DeviceProperties_RunDLL DeviceProperties_RunDLLA
#define DeviceProperties DevicePropertiesA
#define DeviceProblemText DeviceProblemTextA
#define DeviceProblemWizard DeviceProblemWizardA
#define DeviceProblemWizard_RunDLL DeviceProblemWizard_RunDLLA
#define DeviceManagerPrint DeviceManagerPrintA
#define DeviceAdvancedProperties DeviceAdvancedPropertiesA
#define DevicePropertiesEx DevicePropertiesExA
#endif

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
GetDeviceLocationString(IN DEVINST dnDevInst  OPTIONAL,
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

/* EOF */
