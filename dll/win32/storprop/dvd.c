/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Storage device properties
 * COPYRIGHT:   2021 Eric Kohl (eric.kohl@reactos.org)
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>


/*
 * @unimplemented
 */
LONG
WINAPI
CdromDisableDigitalPlayback(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData)
{
    DPRINT1("CdromDisableDigitalPlayback(%p %p)\n",
           DeviceInfoSet, DeviceInfoData);
    return ERROR_SUCCESS;
}


/*
 * @unimplemented
 */
LONG
WINAPI
CdromEnableDigitalPlayback(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ BOOLEAN ForceUnknown)
{
    DPRINT1("CdromEnableDigitalPlayback(%p %p %u)\n",
           DeviceInfoSet, DeviceInfoData, ForceUnknown);
    return ERROR_SUCCESS;
}


/*
 * @unimplemented
 */
LONG
WINAPI
CdromIsDigitalPlaybackEnabled(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _Out_ PBOOLEAN Enabled)
{
    DPRINT1("CdromIsDigitalPlaybackEnabled(%p %p %p)\n",
           DeviceInfoSet, DeviceInfoData, Enabled);
    return ERROR_SUCCESS;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
CdromKnownGoodDigitalPlayback(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData)
{
    DPRINT1("CdromKnownGoodDigitalPlayback(%p %p)\n",
           DeviceInfoSet, DeviceInfoData);
    return TRUE;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
DvdClassInstaller(
    _In_ DI_FUNCTION InstallFunction,
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    DPRINT1("DvdClassInstaller(%u %p %p)\n",
           InstallFunction, DeviceInfoSet, DeviceInfoData);

    return ERROR_DI_DO_DEFAULT;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
DvdPropPageProvider(
    _In_ PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    _In_ LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    _In_ LPARAM lParam)
{
    DPRINT1("DvdPropPageProvider(%p %p %lx)\n",
           lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
DvdLauncher(
    _In_ HWND HWnd,
    _In_ CHAR DriveLetter)
{
    DPRINT1("DvdLauncher(%p %c)\n", HWnd, DriveLetter);
    return FALSE;
}

/* EOF */
