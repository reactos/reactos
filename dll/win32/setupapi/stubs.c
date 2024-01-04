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


#define _PSETUP(func)   pSetup ## func

#define RegistryDelnode                 _PSETUP(RegistryDelnode)


/***********************************************************************
 *		RegistryDelnode(SETUPAPI.@)
 */
BOOL WINAPI RegistryDelnode(DWORD x, DWORD y)
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
 *      SetupAddToSourceListA (SETUPAPI.@)
 */
BOOL WINAPI SetupAddToSourceListA(DWORD flags, PCSTR source)
{
    FIXME("0x%08x %s: stub\n", flags, debugstr_a(source));
    return TRUE;
}

/***********************************************************************
 *      SetupAddToSourceListW (SETUPAPI.@)
 */
BOOL WINAPI SetupAddToSourceListW(DWORD flags, PCWSTR source)
{
    FIXME("0x%08x %s: stub\n", flags, debugstr_w(source));
    return TRUE;
}

/***********************************************************************
 *      SetupQueryDrivesInDiskSpaceListA (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryDrivesInDiskSpaceListA(HDSKSPC disk_space, PSTR return_buffer, DWORD return_buffer_size, PDWORD required_size)
{
    FIXME("%p, %p, %d, %p\n", disk_space, return_buffer, return_buffer_size, required_size);
    return FALSE;
}

/***********************************************************************
 *      SetupQueryDrivesInDiskSpaceListW (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryDrivesInDiskSpaceListW(HDSKSPC disk_space, PWSTR return_buffer, DWORD return_buffer_size, PDWORD required_size)
{
    FIXME("%p, %p, %d, %p\n", disk_space, return_buffer, return_buffer_size, required_size);
    return FALSE;
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
 *              SetupLogFileW  (SETUPAPI.@)
 */
BOOL WINAPI SetupLogFileW(
    HSPFILELOG FileLogHandle,
    PCWSTR LogSectionName,
    PCWSTR SourceFileName,
    PCWSTR TargetFileName,
    DWORD Checksum,
    PCWSTR DiskTagfile,
    PCWSTR DiskDescription,
    PCWSTR OtherInfo,
    DWORD Flags )
{
    FIXME("(%p, %s, '%s', '%s', %d, %s, %s, %s, %d): stub\n", FileLogHandle,
        debugstr_w(LogSectionName), debugstr_w(SourceFileName),
        debugstr_w(TargetFileName), Checksum, debugstr_w(DiskTagfile),
        debugstr_w(DiskDescription), debugstr_w(OtherInfo), Flags);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *              SetupLogFileA  (SETUPAPI.@)
 */
BOOL WINAPI SetupLogFileA(
    HSPFILELOG FileLogHandle,
    PCSTR LogSectionName,
    PCSTR SourceFileName,
    PCSTR TargetFileName,
    DWORD Checksum,
    PCSTR DiskTagfile,
    PCSTR DiskDescription,
    PCSTR OtherInfo,
    DWORD Flags )
{
    FIXME("(%p, %p, '%s', '%s', %d, %p, %p, %p, %d): stub\n", FileLogHandle,
        LogSectionName, SourceFileName, TargetFileName, Checksum, DiskTagfile,
        DiskDescription, OtherInfo, Flags);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *              SetupDiDrawMiniIcon  (SETUPAPI.@)
 */
INT WINAPI SetupDiDrawMiniIcon(HDC hdc, RECT rc, INT MiniIconIndex, DWORD Flags)
{
    FIXME("(%p, %s, %d, %x) stub\n", hdc, wine_dbgstr_rect(&rc), MiniIconIndex, Flags);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/***********************************************************************
 *              SetupDiGetClassBitmapIndex  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassBitmapIndex(const GUID *ClassGuid, PINT MiniIconIndex)
{
    FIXME("(%s, %p) stub\n", debugstr_guid(ClassGuid), MiniIconIndex);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *              SetupDiRemoveDeviceInterface (SETUPAPI.@)
 */
BOOL WINAPI SetupDiRemoveDeviceInterface(HDEVINFO info, PSP_DEVICE_INTERFACE_DATA data)
{
    FIXME("(%p, %p): stub\n", info, data);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINSETUPAPI BOOL WINAPI SetupDiGetDeviceInterfaceAlias(IN HDEVINFO  DeviceInfoSet, IN PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData, IN CONST GUID *AliasInterfaceClassGuid, OUT PSP_DEVICE_INTERFACE_DATA  AliasDeviceInterfaceData)
{
    FIXME("%p %p %p %p %p stub\n", DeviceInfoSet, DeviceInterfaceData, AliasInterfaceClassGuid, AliasDeviceInterfaceData);
    SetLastError(ERROR_INVALID_PARAMETER);
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

BOOL WINAPI
SetupDiSetDriverInstallParamsA(
    _In_ HDEVINFO DeviceInfoSet,
    _In_opt_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ PSP_DRVINFO_DATA_A DriverInfoData,
    _In_ PSP_DRVINSTALL_PARAMS DriverInstallParams)
{
    FIXME("Stub %p %p %p %p\n", DeviceInfoSet, DeviceInfoData, DriverInfoData, DriverInstallParams);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI
SetupDiSetDriverInstallParamsW(
    _In_ HDEVINFO DeviceInfoSet,
    _In_opt_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ PSP_DRVINFO_DATA_W DriverInfoData,
    _In_ PSP_DRVINSTALL_PARAMS DriverInstallParams)
{
    FIXME("Stub %p %p %p %p\n", DeviceInfoSet, DeviceInfoData, DriverInfoData, DriverInstallParams);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
