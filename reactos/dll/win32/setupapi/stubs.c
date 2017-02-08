/*
 * SetupAPI stubs
 *
 * Copyright 2000 James Hatheway
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "setupapi_private.h"

/***********************************************************************
 *		pSetupRegistryDelnode(SETUPAPI.@)
 */
BOOL WINAPI pSetupRegistryDelnode(DWORD x, DWORD y)
{
    FIXME("%08x %08x: stub\n", x, y);
    return FALSE;
}

/***********************************************************************
 *      SetupPromptReboot(SETUPAPI.@)
 */
INT WINAPI SetupPromptReboot( HSPFILEQ file_queue, HWND owner, BOOL scan_only )
{
    FIXME("%p, %p, %d\n", file_queue, owner, scan_only);
    return 0;
}

/***********************************************************************
 *      SetupSetSourceListA (SETUPAPI.@)
 */
BOOL WINAPI SetupSetSourceListA(DWORD flags, PCSTR *list, UINT count)
{
    FIXME("0x%08x %p %d\n", flags, list, count);
    return FALSE;
}

/***********************************************************************
 *      SetupSetSourceListW (SETUPAPI.@)
 */
BOOL WINAPI SetupSetSourceListW(DWORD flags, PCWSTR *list, UINT count)
{
    FIXME("0x%08x %p %d\n", flags, list, count);
    return FALSE;
}

/***********************************************************************
 *      SetupPromptForDiskA (SETUPAPI.@)
 */
UINT WINAPI SetupPromptForDiskA(HWND hwndParent, PCSTR DialogTitle, PCSTR DiskName,
        PCSTR PathToSource, PCSTR FileSought, PCSTR TagFile, DWORD DiskPromptStyle,
        PSTR PathBuffer, DWORD PathBufferSize, PDWORD PathRequiredSize)
{
    FIXME("%p %s %s %s %s %s %d %p %d %p: stub\n", hwndParent, debugstr_a(DialogTitle),
          debugstr_a(DiskName), debugstr_a(PathToSource), debugstr_a(FileSought),
          debugstr_a(TagFile), DiskPromptStyle, PathBuffer, PathBufferSize,
          PathRequiredSize);
    return 0;
}

/***********************************************************************
 *      SetupPromptForDiskW (SETUPAPI.@)
 */
UINT WINAPI SetupPromptForDiskW(HWND hwndParent, PCWSTR DialogTitle, PCWSTR DiskName,
        PCWSTR PathToSource, PCWSTR FileSought, PCWSTR TagFile, DWORD DiskPromptStyle,
        PWSTR PathBuffer, DWORD PathBufferSize, PDWORD PathRequiredSize)
{
    FIXME("%p %s %s %s %s %s %d %p %d %p: stub\n", hwndParent, debugstr_w(DialogTitle),
          debugstr_w(DiskName), debugstr_w(PathToSource), debugstr_w(FileSought),
          debugstr_w(TagFile), DiskPromptStyle, PathBuffer, PathBufferSize,
          PathRequiredSize);
    return 0;
}

/***********************************************************************
 *      SetupDiRemoveDevice(SETUPAPI.@)
 */
BOOL WINAPI
SetupDiRemoveDevice(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData)
{
    FIXME ("Stub %p %p\n", DeviceInfoSet, DeviceInfoData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *      SetupDiUnremoveDevice(SETUPAPI.@)
 */
BOOL WINAPI
SetupDiUnremoveDevice(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData)
{
    FIXME ("Stub %p %p\n", DeviceInfoSet, DeviceInfoData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINSETUPAPI BOOL WINAPI SetupDiGetDeviceInterfaceAlias(IN HDEVINFO  DeviceInfoSet, IN PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData, IN CONST GUID *AliasInterfaceClassGuid, OUT PSP_DEVICE_INTERFACE_DATA  AliasDeviceInterfaceData)
{
    FIXME("%p %p %p %p %p stub\n", DeviceInfoSet, DeviceInterfaceData, AliasInterfaceClassGuid, AliasDeviceInterfaceData);
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

WINSETUPAPI BOOL WINAPI SetupSetNonInteractiveMode(BOOL NonInteractiveFlag)
{
    FIXME("(%d) stub\n", NonInteractiveFlag);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *      SetupVerifyInfFileA(SETUPAPI.@)
 */
BOOL WINAPI
SetupVerifyInfFileA(
    IN PCSTR InfName,
    IN PSP_ALTPLATFORM_INFO AltPlatformInfo,
    OUT PSP_INF_SIGNER_INFO_A InfFileName)
{
    FIXME ("Stub %s %p %p\n", debugstr_a(InfName), AltPlatformInfo, InfFileName);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *      SetupVerifyInfFileW(SETUPAPI.@)
 */
BOOL WINAPI
SetupVerifyInfFileW(
    IN PCWSTR InfName,
    IN PSP_ALTPLATFORM_INFO AltPlatformInfo,
    OUT PSP_INF_SIGNER_INFO_W InfFileName)
{
    FIXME ("Stub %s %p %p\n", debugstr_w(InfName), AltPlatformInfo, InfFileName);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

