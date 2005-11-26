#ifndef __DEVMGR_H
#define __DEVMGR_H

#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <commctrl.h>
#include "resource.h"

extern HINSTANCE hDllInstance;

ULONG DbgPrint(PCH Format,...);

WINBOOL
WINAPI
DeviceManager_ExecuteA(HWND hWndParent,
                       HINSTANCE hInst,
                       LPCSTR lpMachineName,
                       int nCmdShow);

WINBOOL
WINAPI
DeviceManager_ExecuteW(HWND hWndParent,
                       HINSTANCE hInst,
                       LPCWSTR lpMachineName,
                       int nCmdShow);

VOID
WINAPI
DeviceProperties_RunDLLA(HWND hWndParent,
                         HINSTANCE hInst,
                         LPCWSTR lpDeviceCmd,
                         int nCmdShow);

VOID
WINAPI
DeviceProperties_RunDLLW(HWND hWndParent,
                         HINSTANCE hInst,
                         LPCSTR lpDeviceCmd,
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
DeviceProblemTextA(PVOID Unknown1,
                   PVOID Unknown2,
                   UINT uProblemId,
                   LPSTR lpString,
                   UINT uMaxString);

UINT
WINAPI
DeviceProblemTextW(PVOID Unknown1,
                   PVOID Unknown2,
                   UINT uProblemId,
                   LPWSTR lpString,
                   UINT uMaxString);

WINBOOL
WINAPI
DeviceProblemWizardA(HWND hWndParent,
                     LPCSTR lpMachineName,
                     LPCSTR lpDeviceID);


WINBOOL
WINAPI
DeviceProblemWizardW(HWND hWndParent,
                     LPCWSTR lpMachineName,
                     LPCWSTR lpDeviceID);

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

WINBOOL
WINAPI
DeviceManagerPrintA(LPCSTR lpMachineName,
                    LPCSTR lpPrinter,
                    int nPrintMode,
                    UINT uNumberOfGuids,
                    LPGUID lpGuids);

WINBOOL
WINAPI
DeviceManagerPrintW(LPCWSTR lpMachineName,
                    LPCWSTR lpPrinter,
                    int nPrintMode,
                    UINT uNumberOfGuids,
                    LPGUID lpGuids);

INT_PTR
WINAPI
DeviceAdvancedPropertiesA(HWND hWndParent,
                          LPCSTR lpMachineName,
                          LPCSTR lpDeviceID);

INT_PTR
WINAPI
DeviceAdvancedPropertiesW(HWND hWndParent,
                          LPCWSTR lpMachineName,
                          LPCWSTR lpDeviceID);

HWND
WINAPI
DeviceCreateHardwarePage(HWND hWndParent,
                         LPGUID lpGuid);

HWND
WINAPI
DeviceCreateHardwarePageEx(HWND hWndParent,
                           LPGUID lpGuids,
                           UINT uNumberOfGuids,
                           UINT Unknown);

int
WINAPI
DevicePropertiesExA(HWND hWndParent,
                    LPCSTR lpMachineName,
                    LPCSTR lpDeviceID,
                    HINSTANCE hInst,
                    DWORD Unknown);

int
WINAPI
DevicePropertiesExW(HWND hWndParent,
                    LPCWSTR lpMachineName,
                    LPCWSTR lpDeviceID,
                    HINSTANCE hInst,
                    DWORD Unknown);

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
                                IN HDEVINFO DeviceInfoSet,
                                IN PSP_DEVINFO_DATA DeviceInfoData,
                                IN HINSTANCE hComCtl32);

/* MISC.C */

DWORD
LoadAndFormatString(IN HINSTANCE hInstance,
                    IN UINT uID,
                    OUT LPWSTR *lpTarget,
                    ...);

LPARAM
ListViewGetSelectedItemData(IN HWND hwnd);

LPWSTR
ConvertMultiByteToUnicode(IN LPCSTR lpMultiByteStr,
                          IN UINT uCodePage);

HINSTANCE
LoadAndInitComctl32(VOID);

#endif /* __DEVMGR_H */

/* EOF */
