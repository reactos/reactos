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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "setupapi.h"

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
 *		SetupDiGetDeviceInfoListDetailW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInfoListDetailW(HDEVINFO devinfo, PSP_DEVINFO_LIST_DETAIL_DATA_W devinfo_data )
{
  FIXME("\n");
  return FALSE;
}

/***********************************************************************
 *		  (SETUPAPI.@)
 *
 * NO WINAPI in description given
 */
HDEVINFO WINAPI SetupDiGetClassDevsExA(const GUID *class, PCSTR filter, HWND parent, DWORD flags, HDEVINFO deviceset, PCSTR machine, PVOID reserved)
{
  FIXME("filter %s machine %s\n",debugstr_a(filter),debugstr_a(machine));
  return FALSE;
}

/***********************************************************************
 *		  (SETUPAPI.@)
 *
 * NO WINAPI in description given
 */
HDEVINFO WINAPI SetupDiGetClassDevsExW(const GUID *class, PCWSTR filter, HWND parent, DWORD flags, HDEVINFO deviceset, PCWSTR machine, PVOID reserved)
{
  FIXME("\n");
  return FALSE;
}


/***********************************************************************
 *		SetupCopyOEMInfA  (SETUPAPI.@)
 */
BOOL WINAPI SetupCopyOEMInfA(PCSTR sourceinffile, PCSTR sourcemedialoc,
			    DWORD mediatype, DWORD copystyle, PSTR destinfname,
			    DWORD destnamesize, PDWORD required,
			    PSTR *destinfnamecomponent)
{
  FIXME("stub: source %s location %s ...\n", debugstr_a(sourceinffile),
        debugstr_a(sourcemedialoc));
  return FALSE;
}

/***********************************************************************
 *      SetupCopyOEMInfW  (SETUPAPI.@)
 */
BOOL WINAPI SetupCopyOEMInfW(PCWSTR sourceinffile, PCWSTR sourcemedialoc,
                DWORD mediatype, DWORD copystyle, PWSTR destinfname,
                DWORD destnamesize, PDWORD required,
                PWSTR *destinfnamecomponent)
{
  FIXME("stub: source %s location %s ...\n", debugstr_w(sourceinffile),
        debugstr_w(sourcemedialoc));
  return FALSE;
}

/***********************************************************************
 *		SetupGetInfInformationA    (SETUPAPI.@)
 */
BOOL WINAPI SetupGetInfInformationA( LPCVOID InfSpec, DWORD SearchControl,
                                     PSP_INF_INFORMATION ReturnBuffer,
                                     DWORD ReturnBufferSize, PDWORD RequiredSize)
{
    FIXME("(%p, %ld, %p, %ld, %p) Stub!\n",
          InfSpec, SearchControl, ReturnBuffer, ReturnBufferSize, RequiredSize );
    return TRUE;
}

/***********************************************************************
 *		SetupInitializeFileLogW(SETUPAPI.@)
 */
HANDLE WINAPI SetupInitializeFileLogW(LPWSTR LogFileName, DWORD Flags)
{
    FIXME("Stub %s, 0x%lx\n",debugstr_w(LogFileName),Flags);
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *		SetupInitializeFileLogA(SETUPAPI.@)
 */
HANDLE WINAPI SetupInitializeFileLogA(LPSTR LogFileName, DWORD Flags)
{
    FIXME("Stub %s, 0x%lx\n",debugstr_a(LogFileName),Flags);
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *		SetupTerminateFileLog(SETUPAPI.@)
 */
BOOL WINAPI SetupTerminateFileLog(HANDLE FileLogHandle)
{
    FIXME ("Stub %p\n",FileLogHandle);
    return TRUE;
}
