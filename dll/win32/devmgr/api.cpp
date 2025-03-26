/*
*
* COPYRIGHT:       See COPYING in the top level directory
* PROJECT:         ReactOS devmgr.dll
* FILE:            dll/win32/devmgr/api.cpp
* PURPOSE:         devmgr.dll interface
* PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
*                  Ged Murphy (gedmurphy@reactos.org)
* NOTES:
*                  Some helpful resources:
*                    http://support.microsoft.com/default.aspx?scid=kb;%5BLN%5D;815320
*                    https://web.archive.org/web/20050321020634/http://www.jsifaq.com/SUBO/tip7400/rh7482.htm
*                    http://www.jsiinc.com/SUBM/tip6400/rh6490.htm
*
* UPDATE HISTORY:
*      04-04-2004  Created
*/

#include "precomp.h"
#include "devmgmt/MainWindow.h"
#include "properties/properties.h"

HINSTANCE hDllInstance = NULL;



/***************************************************************************
* NAME                                                         EXPORTED
*      DeviceAdvancedPropertiesW
*
* DESCRIPTION
*   Invokes the device properties dialog, this version may add some property pages
*   for some devices
*
* ARGUMENTS
*   hWndParent:    Handle to the parent window
*   lpMachineName: Machine Name, NULL is the local machine
*   lpDeviceID:    Specifies the device whose properties are to be shown
*
* RETURN VALUE
*   Always returns -1, a call to GetLastError returns 0 if successful
*
* @implemented
*/
INT_PTR
WINAPI
DeviceAdvancedPropertiesW(IN HWND hWndParent  OPTIONAL,
                          IN LPCWSTR lpMachineName  OPTIONAL,
                          IN LPCWSTR lpDeviceID)
{
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DevInfoData;
    HINSTANCE hComCtl32;
    INT_PTR Ret = -1;

    if (lpDeviceID == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* dynamically load comctl32 */
    hComCtl32 = LoadAndInitComctl32();
    if (hComCtl32 != NULL)
    {
        hDevInfo = SetupDiCreateDeviceInfoListEx(NULL,
                                                 hWndParent,
                                                 lpMachineName,
                                                 NULL);
        if (hDevInfo != INVALID_HANDLE_VALUE)
        {
            DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            if (SetupDiOpenDeviceInfo(hDevInfo,
                                      lpDeviceID,
                                      hWndParent,
                                      0,
                                      &DevInfoData))
            {
                Ret = DisplayDeviceAdvancedProperties(hWndParent,
                                                      lpDeviceID,
                                                      hDevInfo,
                                                      &DevInfoData,
                                                      hComCtl32,
                                                      lpMachineName,
                                                      0);
            }

            SetupDiDestroyDeviceInfoList(hDevInfo);
        }

        FreeLibrary(hComCtl32);
    }

    return Ret;
}


/***************************************************************************
* NAME                                                         EXPORTED
*      DeviceAdvancedPropertiesA
*
* DESCRIPTION
*   Invokes the device properties dialog, this version may add some property pages
*   for some devices
*
* ARGUMENTS
*   hWndParent:    Handle to the parent window
*   lpMachineName: Machine Name, NULL is the local machine
*   lpDeviceID:    Specifies the device whose properties are to be shown
*
* RETURN VALUE
*   Always returns -1, a call to GetLastError returns 0 if successful
*
* @implemented
*/
INT_PTR
WINAPI
DeviceAdvancedPropertiesA(IN HWND hWndParent  OPTIONAL,
                          IN LPCSTR lpMachineName  OPTIONAL,
                          IN LPCSTR lpDeviceID)
{
    LPWSTR lpMachineNameW = NULL;
    LPWSTR lpDeviceIDW = NULL;
    INT_PTR Ret = -1;

    if (lpMachineName != NULL)
    {
        if (!(lpMachineNameW = ConvertMultiByteToUnicode(lpMachineName,
                                                         CP_ACP)))
        {
            goto Cleanup;
        }
    }
    if (lpDeviceID != NULL)
    {
        if (!(lpDeviceIDW = ConvertMultiByteToUnicode(lpDeviceID,
                                                      CP_ACP)))
        {
            goto Cleanup;
        }
    }

    Ret = DeviceAdvancedPropertiesW(hWndParent,
                                    lpMachineNameW,
                                    lpDeviceIDW);

Cleanup:
    if (lpMachineNameW != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpMachineNameW);
    }
    if (lpDeviceIDW != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpDeviceIDW);
    }

    return Ret;
}


/***************************************************************************
* NAME                                                         EXPORTED
*      DevicePropertiesExA
*
* DESCRIPTION
*   Invokes the extended device properties dialog
*
* ARGUMENTS
*   hWndParent:    Handle to the parent window
*   lpMachineName: Machine Name, NULL is the local machine
*   lpDeviceID:    Specifies the device whose properties are to be shown, optional if
*                  bShowDevMgr is nonzero
*   dwFlags:       This parameter can be a combination of the following flags:
*                  * DPF_DEVICE_STATUS_ACTION: Only valid if bShowDevMgr, causes
*                                              the default device status action button
*                                              to be clicked (Troubleshoot, Enable
*                                              Device, etc)
*   bShowDevMgr:   If non-zero it displays the device manager instead of
*                  the advanced device property dialog
*
* RETURN VALUE
*   1:  if bShowDevMgr is non-zero and no error occured
*   -1: a call to GetLastError returns 0 if successful
*
* @implemented
*/
INT_PTR
WINAPI
DevicePropertiesExA(IN HWND hWndParent  OPTIONAL,
                    IN LPCSTR lpMachineName  OPTIONAL,
                    IN LPCSTR lpDeviceID  OPTIONAL,
                    IN DWORD dwFlags  OPTIONAL,
                    IN BOOL bShowDevMgr)
{
    LPWSTR lpMachineNameW = NULL;
    LPWSTR lpDeviceIDW = NULL;
    INT_PTR Ret = -1;

    if (lpMachineName != NULL)
    {
        if (!(lpMachineNameW = ConvertMultiByteToUnicode(lpMachineName,
                                                         CP_ACP)))
        {
            goto Cleanup;
        }
    }
    if (lpDeviceID != NULL)
    {
        if (!(lpDeviceIDW = ConvertMultiByteToUnicode(lpDeviceID,
                                                      CP_ACP)))
        {
            goto Cleanup;
        }
    }

    Ret = DevicePropertiesExW(hWndParent,
                              lpMachineNameW,
                              lpDeviceIDW,
                              dwFlags,
                              bShowDevMgr);

Cleanup:
    if (lpMachineNameW != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpMachineNameW);
    }
    if (lpDeviceIDW != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpDeviceIDW);
    }

    return Ret;
}


/***************************************************************************
* NAME                                                         EXPORTED
*      DevicePropertiesExW
*
* DESCRIPTION
*   Invokes the extended device properties dialog
*
* ARGUMENTS
*   hWndParent:    Handle to the parent window
*   lpMachineName: Machine Name, NULL is the local machine
*   lpDeviceID:    Specifies the device whose properties are to be shown, optional if
*                  bShowDevMgr is nonzero
*   dwFlags:       This parameter can be a combination of the following flags:
*                  * DPF_DEVICE_STATUS_ACTION: Only valid if bShowDevMgr, causes
*                                              the default device status action button
*                                              to be clicked (Troubleshoot, Enable
*                                              Device, etc)
*   bShowDevMgr:   If non-zero it displays the device manager instead of
*                  the advanced device property dialog
*
* RETURN VALUE
*   1:  if bShowDevMgr is non-zero and no error occured
*   -1: a call to GetLastError returns 0 if successful
*
* @implemented
*/
INT_PTR
WINAPI
DevicePropertiesExW(IN HWND hWndParent  OPTIONAL,
                    IN LPCWSTR lpMachineName  OPTIONAL,
                    IN LPCWSTR lpDeviceID  OPTIONAL,
                    IN DWORD dwFlags  OPTIONAL,
                    IN BOOL bShowDevMgr)
{
    INT_PTR Ret = -1;

    if (dwFlags & ~(DPF_EXTENDED | DPF_DEVICE_STATUS_ACTION))
    {
        FIXME("DevPropertiesExW: Invalid flags: 0x%x\n",
              dwFlags & ~(DPF_EXTENDED | DPF_DEVICE_STATUS_ACTION));
        SetLastError(ERROR_INVALID_FLAGS);
        return -1;
    }

    if (bShowDevMgr)
    {
        FIXME("DevPropertiesExW doesn't support bShowDevMgr!\n");
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    }
    else
    {
        HDEVINFO hDevInfo;
        SP_DEVINFO_DATA DevInfoData;
        HINSTANCE hComCtl32;

        if (lpDeviceID == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return -1;
        }

        /* dynamically load comctl32 */
        hComCtl32 = LoadAndInitComctl32();
        if (hComCtl32 != NULL)
        {
            hDevInfo = SetupDiCreateDeviceInfoListEx(NULL,
                                                     hWndParent,
                                                     lpMachineName,
                                                     NULL);
            if (hDevInfo != INVALID_HANDLE_VALUE)
            {
                DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
                if (SetupDiOpenDeviceInfo(hDevInfo,
                                          lpDeviceID,
                                          hWndParent,
                                          0,
                                          &DevInfoData))
                {
                    Ret = DisplayDeviceAdvancedProperties(hWndParent,
                                                          lpDeviceID,
                                                          hDevInfo,
                                                          &DevInfoData,
                                                          hComCtl32,
                                                          lpMachineName,
                                                          dwFlags);
                }

                SetupDiDestroyDeviceInfoList(hDevInfo);
            }

            FreeLibrary(hComCtl32);
        }
    }

    return Ret;
}


/***************************************************************************
* NAME                                                         EXPORTED
*      DevicePropertiesA
*
* DESCRIPTION
*   Invokes the device properties dialog directly
*
* ARGUMENTS
*   hWndParent:    Handle to the parent window
*   lpMachineName: Machine Name, NULL is the local machine
*   lpDeviceID:    Specifies the device whose properties are to be shown
*   bShowDevMgr:   If non-zero it displays the device manager instead of
*                  the device property dialog
*
* RETURN VALUE
*   >=0: if no errors occured
*   -1:  if errors occured
*
* REVISIONS
*
* @implemented
*/
int
WINAPI
DevicePropertiesA(HWND hWndParent,
                  LPCSTR lpMachineName,
                  LPCSTR lpDeviceID,
                  BOOL bShowDevMgr)
{
    return DevicePropertiesExA(hWndParent,
                               lpMachineName,
                               lpDeviceID,
                               DPF_EXTENDED,
                               bShowDevMgr);
}


/***************************************************************************
* NAME                                                         EXPORTED
*      DevicePropertiesW
*
* DESCRIPTION
*   Invokes the device properties dialog directly
*
* ARGUMENTS
*   hWndParent:    Handle to the parent window
*   lpMachineName: Machine Name, NULL is the local machine
*   lpDeviceID:    Specifies the device whose properties are to be shown
*   bShowDevMgr:   If non-zero it displays the device manager instead of
*                  the device property dialog
*
* RETURN VALUE
*   >=0: if no errors occured
*   -1:  if errors occured
*
* REVISIONS
*
* @implemented
*/
int
WINAPI
DevicePropertiesW(HWND hWndParent,
                  LPCWSTR lpMachineName,
                  LPCWSTR lpDeviceID,
                  BOOL bShowDevMgr)
{
    return DevicePropertiesExW(hWndParent,
                               lpMachineName,
                               lpDeviceID,
                               DPF_EXTENDED,
                               bShowDevMgr);
}


/***************************************************************************
* NAME                                                         EXPORTED
*      DeviceProperties_RunDLLA
*
* DESCRIPTION
*   Invokes the device properties dialog
*
* ARGUMENTS
*   hWndParent:  Handle to the parent window
*   hInst:       Handle to the application instance
*   lpDeviceCmd: A command that includes the DeviceID of the properties to be shown,
*                also see NOTEs
*   nCmdShow:    Specifies how the window should be shown
*
* RETURN VALUE
*
* REVISIONS
*
* NOTE
*   - lpDeviceCmd is a string in the form of "/MachineName MACHINE /DeviceID DEVICEPATH"
*     (/MachineName is optional). This function only parses this string and eventually
*     calls DeviceProperties().
*
* @implemented
*/
VOID
WINAPI
DeviceProperties_RunDLLA(HWND hWndParent,
                         HINSTANCE hInst,
                         LPCSTR lpDeviceCmd,
                         int nCmdShow)
{
    LPWSTR lpDeviceCmdW = NULL;

    if (lpDeviceCmd != NULL)
    {
        if ((lpDeviceCmdW = ConvertMultiByteToUnicode(lpDeviceCmd,
                                                      CP_ACP)))
        {
            DeviceProperties_RunDLLW(hWndParent,
                                     hInst,
                                     lpDeviceCmdW,
                                     nCmdShow);
        }
    }

    if (lpDeviceCmdW != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpDeviceCmdW);
    }
}


/***************************************************************************
* NAME                                                         EXPORTED
*      DeviceProperties_RunDLLW
*
* DESCRIPTION
*   Invokes the device properties dialog
*
* ARGUMENTS
*   hWndParent:  Handle to the parent window
*   hInst:       Handle to the application instance
*   lpDeviceCmd: A command that includes the DeviceID of the properties to be shown,
*                also see NOTEs
*   nCmdShow:    Specifies how the window should be shown
*
* RETURN VALUE
*
* REVISIONS
*
* NOTE
*   - lpDeviceCmd is a string in the form of "/MachineName MACHINE /DeviceID DEVICEPATH"
*     (/MachineName is optional). This function only parses this string and eventually
*     calls DeviceProperties().
*
* @implemented
*/
VOID
WINAPI
DeviceProperties_RunDLLW(HWND hWndParent,
                         HINSTANCE hInst,
                         LPCWSTR lpDeviceCmd,
                         int nCmdShow)
{
    WCHAR szDeviceID[MAX_DEVICE_ID_LEN + 1];
    WCHAR szMachineName[MAX_COMPUTERNAME_LENGTH + 1];
    LPWSTR lpString = (LPWSTR)lpDeviceCmd;

    if (!GetDeviceAndComputerName(lpString,
                                  szDeviceID,
                                  szMachineName))
    {
        ERR("DeviceProperties_RunDLLW DeviceID: %S, MachineName: %S\n", szDeviceID, szMachineName);
        return;
    }

    DevicePropertiesW(hWndParent,
                      szMachineName,
                      szDeviceID,
                      FALSE);
}



/***************************************************************************
* NAME                                                         EXPORTED
*      DeviceManager_ExecuteA
*
* DESCRIPTION
*   Starts the Device Manager
*
* ARGUMENTS
*   hWndParent:   Handle to the parent window
*   hInst:        Handle to the application instance
*   lpMachineName: Machine Name, NULL is the local machine
*   nCmdShow:      Specifies how the window should be shown
*
* RETURN VALUE
*   TRUE:  if no errors occured
*   FALSE: if the device manager could not be executed
*
* REVISIONS
*
* NOTE
*   - Win runs the device manager in a separate process, so hWndParent is somehow
*     obsolete.
*
* @implemented
*/
BOOL
WINAPI
DeviceManager_ExecuteA(HWND hWndParent,
                       HINSTANCE hInst,
                       LPCSTR lpMachineName,
                       int nCmdShow)
{
    LPWSTR lpMachineNameW = NULL;
    BOOL Ret;

    if (lpMachineName != NULL)
    {
        if (!(lpMachineNameW = ConvertMultiByteToUnicode(lpMachineName,
                                                         CP_ACP)))
        {
            return FALSE;
        }
    }

    Ret = DeviceManager_ExecuteW(hWndParent,
                                 hInst,
                                 lpMachineNameW,
                                 nCmdShow);

    if (lpMachineNameW != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpMachineNameW);
    }


    return Ret;
}


/***************************************************************************
* NAME                                                         EXPORTED
*      DeviceManager_ExecuteW
*
* DESCRIPTION
*   Starts the Device Manager
*
* ARGUMENTS
*   hWndParent:   Handle to the parent window
*   hInst:        Handle to the application instance
*   lpMachineName: Machine Name, NULL is the local machine
*   nCmdShow:      Specifies how the window should be shown
*
* RETURN VALUE
*   TRUE:  if no errors occured
*   FALSE: if the device manager could not be executed
*
* REVISIONS
*
* NOTE
*   - Win runs the device manager in a separate process, so hWndParent is somehow
*     obsolete.
*
* @implemented
*/
BOOL
WINAPI
DeviceManager_ExecuteW(HWND hWndParent,
                       HINSTANCE hInst,
                       LPCWSTR lpMachineName,
                       int nCmdShow)
{
    // FIXME: Call mmc with devmgmt.msc

    CDeviceManager DevMgr;
    return DevMgr.Create(hWndParent, hInst, lpMachineName, nCmdShow);
}


/***************************************************************************
* NAME                                                         EXPORTED
*      DeviceProblemWizard_RunDLLA
*
* DESCRIPTION
*   Calls the device problem wizard
*
* ARGUMENTS
*   hWndParent:  Handle to the parent window
*   hInst:       Handle to the application instance
*   lpDeviceCmd: A command that includes the DeviceID of the properties to be shown,
*                also see NOTEs
*   nCmdShow:    Specifies how the window should be shown
*
* RETURN VALUE
*
* REVISIONS
*
* NOTE
*   - Win XP exports this function as DeviceProblenWizard_RunDLLA, apparently it's
*     a typo so we additionally export an alias function
*   - lpDeviceCmd is a string in the form of "/MachineName MACHINE /DeviceID DEVICEPATH"
*     (/MachineName is optional). This function only parses this string and eventually
*     calls DeviceProperties().
*
* @unimplemented
*/
VOID
WINAPI
DeviceProblemWizard_RunDLLA(HWND hWndParent,
                            HINSTANCE hInst,
                            LPCSTR lpDeviceCmd,
                            int nCmdShow)
{
    UNIMPLEMENTED;
}


/***************************************************************************
* NAME                                                         EXPORTED
*      DeviceProblemWizard_RunDLLW
*
* DESCRIPTION
*   Calls the device problem wizard
*
* ARGUMENTS
*   hWndParent:  Handle to the parent window
*   hInst:       Handle to the application instance
*   lpDeviceCmd: A command that includes the DeviceID of the properties to be shown,
*                also see NOTEs
*   nCmdShow:    Specifies how the window should be shown
*
* RETURN VALUE
*
* REVISIONS
*
* NOTE
*   - Win XP exports this function as DeviceProblenWizard_RunDLLA, apparently it's
*     a typo so we additionally export an alias function
*   - lpDeviceCmd is a string in the form of "/MachineName MACHINE /DeviceID DEVICEPATH"
*     (/MachineName is optional). This function only parses this string and eventually
*     calls DeviceProperties().
*
* @unimplemented
*/
VOID
WINAPI
DeviceProblemWizard_RunDLLW(HWND hWndParent,
                            HINSTANCE hInst,
                            LPCWSTR lpDeviceCmd,
                            int nCmdShow)
{
    UNIMPLEMENTED;
}


/***************************************************************************
* NAME                                                         EXPORTED
*      DeviceManagerPrintA
*
* DESCRIPTION
*   Calls the device problem wizard
*
* ARGUMENTS
*   lpMachineName:  Machine Name, NULL is the local machine
*   lpPrinter:      Filename of the printer where it should be printed on
*   nPrintMode:     Specifies what kind of information is to be printed
*                     DEV_PRINT_ABSTRACT: Prints an abstract of system information, the parameters
*                                         uNumberOfGuids, Guids are ignored
*                     DEV_PRINT_SELECTED: Prints information about the devices listed in Guids
*                     DEV_PRINT_ALL:      Prints an abstract of system information and all
*                                         system devices
*   uNumberOfGuids: Numbers of guids in the Guids array, this parameter is ignored unless
*                   nPrintMode is DEV_PRINT_SELECTED
*   lpGuids:        Array of device guids, this parameter is ignored unless
*                   nPrintMode is DEV_PRINT_SELECTED
*
* RETURN VALUE
*   TRUE:  if no errors occured
*   FALSE: if errors occured
*
* REVISIONS
*
* NOTE
*
* @unimplemented
*/
BOOL
WINAPI
DeviceManagerPrintA(LPCSTR lpMachineName,
                    LPCSTR lpPrinter,
                    int nPrintMode,
                    UINT uNumberOfGuids,
                    LPGUID lpGuids)
{
    UNIMPLEMENTED;
    return FALSE;
}


/***************************************************************************
* NAME                                                         EXPORTED
*      DeviceManagerPrintW
*
* DESCRIPTION
*   Calls the device problem wizard
*
* ARGUMENTS
*   lpMachineName:  Machine Name, NULL is the local machine
*   lpPrinter:      Filename of the printer where it should be printed on
*   nPrintMode:     Specifies what kind of information is to be printed
*                     DEV_PRINT_ABSTRACT: Prints an abstract of system information, the parameters
*                                         uNumberOfGuids, Guids are ignored
*                     DEV_PRINT_SELECTED: Prints information about the devices listed in Guids
*                     DEV_PRINT_ALL:      Prints an abstract of system information and all
*                                         system devices
*   uNumberOfGuids: Numbers of guids in the Guids array, this parameter is ignored unless
*                   nPrintMode is DEV_PRINT_SELECTED
*   lpGuids:        Array of device guids, this parameter is ignored unless
*                   nPrintMode is DEV_PRINT_SELECTED
*
* RETURN VALUE
*   TRUE:  if no errors occured
*   FALSE: if errors occured
*
* REVISIONS
*
* NOTE
*
* @unimplemented
*/
BOOL
WINAPI
DeviceManagerPrintW(LPCWSTR lpMachineName,
                    LPCWSTR lpPrinter,
                    int nPrintMode,
                    UINT uNumberOfGuids,
                    LPGUID lpGuids)
{
    UNIMPLEMENTED;
    return FALSE;
}

class CDevMgrUIModule : public CComModule
{
public:
};

CDevMgrUIModule gModule;

STDAPI DllCanUnloadNow()
{
    return gModule.DllCanUnloadNow();
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return gModule.DllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer()
{
    return gModule.DllRegisterServer(FALSE);
}

STDAPI DllUnregisterServer()
{
    return gModule.DllUnregisterServer(FALSE);
}

extern "C" {

BOOL
WINAPI
DllMain(_In_ HINSTANCE hinstDLL,
        _In_ DWORD dwReason,
        _In_ LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            hDllInstance = hinstDLL;
            break;
    }

    return TRUE;
}
}
