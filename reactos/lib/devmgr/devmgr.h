#ifndef __DEVMGR_H
#define __DEVMGR_H

HINSTANCE hDllInstance;

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

int
WINAPI
DeviceAdvancedPropertiesA(HWND hWndParent,
                          LPCSTR lpMachineName,
                          LPCSTR lpDeviceID);

int
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
                    HINSTANCE hInst,
                    LPCSTR lpMachineName,
                    LPCSTR lpDeviceID,
                    DWORD Unknown);

int
WINAPI
DevicePropertiesExW(HWND hWndParent,
                    HINSTANCE hInst,
                    LPCWSTR lpMachineName,
                    LPCWSTR lpDeviceID,
                    DWORD Unknown);

#endif /* __DEVMGR_H */

/* EOF */
