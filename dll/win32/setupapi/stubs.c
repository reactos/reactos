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

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/***********************************************************************
 *		TPWriteProfileString (SETUPX.62)
 */
BOOL WINAPI TPWriteProfileString16( LPCSTR section, LPCSTR entry, LPCSTR string )
{
    FIXME( "%s %s %s: stub\n", debugstr_a(section), debugstr_a(entry), debugstr_a(string) );
    return TRUE;
}


/***********************************************************************
 *		suErrorToIds  (SETUPX.61)
 */
DWORD WINAPI suErrorToIds16( WORD w1, WORD w2 )
{
    FIXME( "%x %x: stub\n", w1, w2 );
    return 0;
}

/***********************************************************************
 *		SetupDiGetDeviceInfoListDetailA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInfoListDetailA(HDEVINFO devinfo, PSP_DEVINFO_LIST_DETAIL_DATA_A devinfo_data )
{
  FIXME("\n");
  return FALSE;
}

/***********************************************************************
 *		SetupInitializeFileLogW(SETUPAPI.@)
 */
HANDLE WINAPI SetupInitializeFileLogW(LPCWSTR LogFileName, DWORD Flags)
{
    FIXME("Stub %s, 0x%lx\n",debugstr_w(LogFileName),Flags);
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *		SetupInitializeFileLogA(SETUPAPI.@)
 */
HANDLE WINAPI SetupInitializeFileLogA(LPCSTR LogFileName, DWORD Flags)
{
    FIXME("Stub %s, 0x%lx\n",debugstr_a(LogFileName),Flags);
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *		SetupPromptReboot(SETUPAPI.@)
 */
INT WINAPI SetupPromptReboot(HSPFILEQ FileQueue, HWND Owner, BOOL ScanOnly)
{
#if 0
    int ret;
    TCHAR RebootText[RC_STRING_MAX_SIZE];
    TCHAR RebootCaption[RC_STRING_MAX_SIZE];
    INT rc = 0;

    TRACE("%p %p %d\n", FileQueue, Owner, ScanOnly);

    if (ScanOnly && !FileQueue)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    if (FileQueue)
    {
        FIXME("Case 'FileQueue != NULL' not implemented\n");
        /* In some cases, do 'rc |= SPFILEQ_FILE_IN_USE' */
    }

    if (ScanOnly)
        return rc;

    /* We need to ask the question to the user. */
    rc |= SPFILEQ_REBOOT_RECOMMENDED;
    if (0 == LoadString(hInstance, IDS_QUERY_REBOOT_TEXT, RebootText, RC_STRING_MAX_SIZE))
        return -1;
    if (0 == LoadString(hInstance, IDS_QUERY_REBOOT_CAPTION, RebootCaption, RC_STRING_MAX_SIZE))
        return -1;
    ret = MessageBox(Owner, RebootText, RebootCaption, MB_YESNO | MB_DEFBUTTON1);
    if (IDNO == ret)
        return rc;
    else
    {
        if (ExitWindowsEx(EWX_REBOOT, 0))
            return rc | SPFILEQ_REBOOT_IN_PROGRESS;
        else
            return -1;
    }
#endif
    FIXME("Stub %p %p %d\n", FileQueue, Owner, ScanOnly);
    SetLastError(ERROR_GEN_FAILURE);
    return -1;
}


/***********************************************************************
 *		SetupTerminateFileLog(SETUPAPI.@)
 */
BOOL WINAPI SetupTerminateFileLog(HANDLE FileLogHandle)
{
    FIXME ("Stub %p\n",FileLogHandle);
    return TRUE;
}


/***********************************************************************
 *      SetupCloseLog(SETUPAPI.@)
 */
void WINAPI SetupCloseLog()
{
    FIXME("() stub\n");
}

/***********************************************************************
 *      SetupLogErrorW(SETUPAPI.@)
 */
BOOL WINAPI SetupLogErrorW(PCWSTR MessageString, LogSeverity Severity)
{
    FIXME("(%s, %ld) stub\n", debugstr_w(MessageString), Severity);
    return TRUE;
}

/***********************************************************************
 *      SetupOpenLog(SETUPAPI.@)
 */
BOOL WINAPI SetupOpenLog(BOOL Reserved)
{
    FIXME("(%d) stub\n", Reserved);
    return TRUE;
}

/***********************************************************************
 *		SetupDiRegisterDeviceInfo(SETUPAPI.@)
 */
BOOL WINAPI
SetupDiRegisterDeviceInfo(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD Flags,
    IN PSP_DETSIG_CMPPROC CompareProc OPTIONAL,
    IN PVOID CompareContext OPTIONAL,
    OUT PSP_DEVINFO_DATA DupDeviceInfoData OPTIONAL)
{
    FIXME ("Stub %p %p 0x%lx %p %p %p\n", DeviceInfoSet, DeviceInfoData,
        Flags, CompareProc, CompareContext, DupDeviceInfoData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *		SetupDiRemoveDevice(SETUPAPI.@)
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
 *		SetupDiUnremoveDevice(SETUPAPI.@)
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
