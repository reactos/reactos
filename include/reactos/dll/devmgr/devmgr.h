#ifndef __DEVMGR__H
#define __DEVMGR__H

#ifdef __cplusplus
extern "C" {
#endif

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
                  LPCSTR lpMachineName,
                  LPCSTR lpDeviceID,
                  BOOL bShowDevMgr);

int
WINAPI
DevicePropertiesW(HWND hWndParent,
                  LPCWSTR lpMachineName,
                  LPCWSTR lpDeviceID,
                  BOOL bShowDevMgr);

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

#define DPF_EXTENDED    (0x1)
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

#ifdef __cplusplus
}
#endif

#endif /* __DEVMGR__H */
