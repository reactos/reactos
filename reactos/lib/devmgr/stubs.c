/* $Id: stubs.c,v 1.1 2004/04/04 21:49:15 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS msgina.dll
 * FILE:            lib/devmgr/stubs.c
 * PURPOSE:         devmgr.dll stubs
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * NOTES:           If you implement a function, remove it from this file
 *
 *                  Some helpful resources:
 *                    http://support.microsoft.com/default.aspx?scid=kb;%5BLN%5D;815320
 *                    http://www.jsiinc.com/SUBO/tip7400/rh7482.htm
 *                    http://www.jsiinc.com/SUBM/tip6400/rh6490.htm
 *
 * UPDATE HISTORY:
 *      04-04-2004  Created
 */
#include <windows.h>
#include "devmgr.h"

#define UNIMPLEMENTED \
  DbgPrint("DEVMGR:  %s at %s:%d is UNIMPLEMENTED!\n",__FUNCTION__,__FILE__,__LINE__)


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
WINBOOL
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
WINBOOL
WINAPI
DeviceManager_ExecuteW(HWND hWndParent,
                       HINSTANCE hInst,
                       LPCWSTR lpMachineName,
                       int nCmdShow)
{
  UNIMPLEMENTED;
  return FALSE;
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
 * @unimplemented
 */
VOID
WINAPI
DeviceProperties_RunDLLA(HWND hWndParent,
                         HINSTANCE hInst,
                         LPCWSTR lpDeviceCmd,
                         int nCmdShow)
{
  UNIMPLEMENTED;
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
 * @unimplemented
 */
VOID
WINAPI
DeviceProperties_RunDLLW(HWND hWndParent,
                         HINSTANCE hInst,
                         LPCSTR lpDeviceCmd,
                         int nCmdShow)
{
  UNIMPLEMENTED;
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
 *   hInst:         Handle to the application instance
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device whose properties are to be shown
 *   Unknown:       Unknown parameter, see NOTEs
 *
 * RETURN VALUE
 *   >=0: if no errors occured
 *   -1:  if errors occured
 *
 * REVISIONS
 *
 * NOTE
 *   Unknown seems to be a BOOL, not sure what it does
 *
 * @unimplemented
 */
int
WINAPI
DevicePropertiesA(HWND hWndParent,
                  HINSTANCE hInst,
                  LPCSTR lpMachineName,
                  LPCSTR lpDeviceID,
                  DWORD Unknown)
{
  UNIMPLEMENTED;
  return -1;
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
 *   hInst:         Handle to the application instance
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device whose properties are to be shown
 *   Unknown:       Unknown parameter, see NOTEs
 *
 * RETURN VALUE
 *   >=0: if no errors occured
 *   -1:  if errors occured
 *
 * REVISIONS
 *
 * NOTE
 *   Unknown seems to be a BOOL, not sure what it does
 *
 * @unimplemented
 */
int
WINAPI
DevicePropertiesW(HWND hWndParent,
                  HINSTANCE hInst,
                  LPCWSTR lpMachineName,
                  LPCWSTR lpDeviceID,
                  DWORD Unknown)
{
  UNIMPLEMENTED;
  return -1;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceProblemTextW
 *
 * DESCRIPTION
 *   Gets the problem text from a problem number displayed in the properties dialog
 *
 * ARGUMENTS
 *   Unknown1:   Unknown parameter
 *   Unknown2:   Unknown parameter
 *   uProblemId: Specifies the problem ID
 *   lpString:   Pointer to a buffer where the string is to be copied to
 *   uMaxString: Size of the buffer
 *
 * RETURN VALUE
 *   The return value is the number of bytes copied into the string buffer.
 *   It returns 0 if an error occured.
 *
 * REVISIONS
 *
 * NOTE
 *
 * @unimplemented
 */
UINT
WINAPI
DeviceProblemTextA(PVOID Unknown1,
                   PVOID Unknown2,
                   UINT uProblemId,
                   LPSTR lpString,
                   UINT uMaxString)
{
  UNIMPLEMENTED;
  return 0;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceProblemTextW
 *
 * DESCRIPTION
 *   Gets the problem text from a problem number displayed in the properties dialog
 *
 * ARGUMENTS
 *   Unknown1:   Unknown parameter
 *   Unknown2:   Unknown parameter
 *   uProblemId: Specifies the problem ID
 *   lpString:   Pointer to a buffer where the string is to be copied to
 *   uMaxString: Size of the buffer
 *
 * RETURN VALUE
 *   The return value is the number of bytes copied into the string buffer.
 *   It returns 0 if an error occured.
 *
 * REVISIONS
 *
 * NOTE
 *
 * @unimplemented
 */
UINT
WINAPI
DeviceProblemTextW(PVOID Unknown1,
                   PVOID Unknown2,
                   UINT uProblemId,
                   LPWSTR lpString,
                   UINT uMaxString)
{
  UNIMPLEMENTED;
  return 0;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceProblemWizardA
 *
 * DESCRIPTION
 *   Calls the device problem wizard
 *
 * ARGUMENTS
 *   hWndParent:    Handle to the parent window
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device, also see NOTEs
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
WINBOOL
WINAPI
DeviceProblemWizardA(HWND hWndParent,
                     LPCSTR lpMachineName,
                     LPCSTR lpDeviceID)
{
  UNIMPLEMENTED;
  return FALSE;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceProblemWizardW
 *
 * DESCRIPTION
 *   Calls the device problem wizard
 *
 * ARGUMENTS
 *   hWndParent:    Handle to the parent window
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device, also see NOTEs
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
WINBOOL
WINAPI
DeviceProblemWizardW(HWND hWndParent,
                     LPCWSTR lpMachineName,
                     LPCWSTR lpDeviceID)
{
  UNIMPLEMENTED;
  return FALSE;
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
WINBOOL
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
WINBOOL
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
 *   -1: if errors occured
 *
 * REVISIONS
 *
 * NOTE
 *
 * @unimplemented
 */
int
WINAPI
DeviceAdvancedPropertiesA(HWND hWndParent,
                          LPCSTR lpMachineName,
                          LPCSTR lpDeviceID)
{
  UNIMPLEMENTED;
  return -1;
}


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
 *   -1: if errors occured
 *
 * REVISIONS
 *
 * NOTE
 *
 * @unimplemented
 */
int
WINAPI
DeviceAdvancedPropertiesW(HWND hWndParent,
                          LPCWSTR lpMachineName,
                          LPCWSTR lpDeviceID)
{
  UNIMPLEMENTED;
  return -1;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceCreateHardwarePage
 *
 * DESCRIPTION
 *   Creates a hardware page
 *
 * ARGUMENTS
 *   hWndParent: Handle to the parent window
 *   lpGuid:     Guid of the device
 *
 * RETURN VALUE
 *   Returns the handle of the hardware page window that has been created or
 *   NULL if it failed.
 *
 * REVISIONS
 *
 * NOTE
 *
 * @unimplemented
 */
HWND
WINAPI
DeviceCreateHardwarePage(HWND hWndParent,
                         LPGUID lpGuid)
{
  UNIMPLEMENTED;
  return NULL;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceCreateHardwarePageEx
 *
 * DESCRIPTION
 *   Creates a hardware page
 *
 * ARGUMENTS
 *   hWndParent:     Handle to the parent window
 *   lpGuids:        An array of guids of devices that are to be listed
 *   uNumberOfGuids: Numbers of guids in the Guids array
 *   Unknown:        Unknown parameter, see NOTEs
 *
 * RETURN VALUE
 *   Returns the handle of the hardware page window that has been created or
 *   NULL if it failed.
 *
 * REVISIONS
 *
 * NOTE
 *   uUnknown seems to be some kind of flag how the entries should be displayed,
 *   in Win it seems to be always 0x00000001
 *
 * @unimplemented
 */
HWND
WINAPI
DeviceCreateHardwarePageEx(HWND hWndParent,
                           LPGUID lpGuids,
                           UINT uNumberOfGuids,
                           UINT Unknown)
{
  UNIMPLEMENTED;
  return NULL;
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
 *   hInst:         Handle to the application instance
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device whose properties are to be shown
 *   Unknown:       Unknown parameter, see NOTEs
 *
 * RETURN VALUE
 *   >=0: if no errors occured
 *   -1:  if errors occured
 *
 * REVISIONS
 *
 * NOTE
 *   Unknown seems to be a BOOL, not sure what it affects
 *
 * @unimplemented
 */
int
WINAPI
DevicePropertiesExA(HWND hWndParent,
                    HINSTANCE hInst,
                    LPCSTR lpMachineName,
                    LPCSTR lpDeviceID,
                    DWORD Unknown)
{
  UNIMPLEMENTED;
  return -1;
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
 *   hInst:         Handle to the application instance
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device whose properties are to be shown
 *   Unknown:       Unknown parameter, see NOTEs
 *
 * RETURN VALUE
 *   >=0: if no errors occured
 *   -1:  if errors occured
 *
 * REVISIONS
 *
 * NOTE
 *   Unknown seems to be a BOOL, not sure what it affects
 *
 * @unimplemented
 */
int
WINAPI
DevicePropertiesExW(HWND hWndParent,
                    HINSTANCE hInst,
                    LPCWSTR lpMachineName,
                    LPCWSTR lpDeviceID,
                    DWORD Unknown)
{
  UNIMPLEMENTED;
  return -1;
}
