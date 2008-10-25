#ifndef __DEVMGR_H
#define __DEVMGR_H

#include <windows.h>
#include <regstr.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <commctrl.h>
#include <dll/devmgr/devmgr.h>
#include "resource.h"

extern HINSTANCE hDllInstance;

ULONG DbgPrint(PCCH Format,...);

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
