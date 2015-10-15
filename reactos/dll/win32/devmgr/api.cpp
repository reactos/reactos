/*
*
* COPYRIGHT:       See COPYING in the top level directory
* PROJECT:         ReactOS devmgr.dll
* FILE:            dll/win32/devmgr/api.cpp
* PURPOSE:         devmgr.dll stubs
* PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
*                  Ged Murphy (gedmurphy@reactos.org)
* NOTES:           
*                  Some helpful resources:
*                    http://support.microsoft.com/default.aspx?scid=kb;%5BLN%5D;815320
*                    http://www.jsiinc.com/SUBO/tip7400/rh7482.htm
*                    http://www.jsiinc.com/SUBM/tip6400/rh6490.htm
*
* UPDATE HISTORY:
*      04-04-2004  Created
*/

#include "precomp.h"
#include <devmgr/devmgr.h>
#include "devmgmt/MainWindow.h"

HINSTANCE hDllInstance = NULL;

WINE_DEFAULT_DEBUG_CHANNEL(devmgr);


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
* @unimplemented
*/
BOOL
WINAPI
DeviceManager_ExecuteA(HWND hWndParent,
                       HINSTANCE hInst,
                       LPCSTR lpMachineName,
                       int nCmdShow)
{
    UNIMPLEMENTED;
    return FALSE;
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
* @unimplemented
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


BOOL
WINAPI
DllMain(IN HINSTANCE hinstDLL,
IN DWORD dwReason,
IN LPVOID lpvReserved)
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