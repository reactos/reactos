/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
 *  Copyright 1997 Marcus Meissner
 *  Copyright 1998 Patrik Stridvall
 *  Copyright 1998, 2003 Andreas Mohr
 *  Copyright 1997, 2003 Alexandre Julliard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Runtime code
 * FILE:              lib/rtl/version.c
 * PROGRAMER:         Filip Navara
 */

/* INCLUDES *****************************************************************/

#include "rtl.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

NTSTATUS
STDCALL
RtlGetVersion(
    OUT PRTL_OSVERSIONINFOW lpVersionInformation
    );

/* FUNCTIONS ****************************************************************/

/*
* @implemented
*/
NTSTATUS
STDCALL
RtlVerifyVersionInfo(
	IN PRTL_OSVERSIONINFOEXW VersionInfo,
	IN ULONG TypeMask,
	IN ULONGLONG ConditionMask
	)
{
    RTL_OSVERSIONINFOEXW ver;
    NTSTATUS status;

    /* FIXME:
        - Check the following special case on Windows (various versions):
          o lp->wSuiteMask == 0 and ver.wSuiteMask != 0 and VER_AND/VER_OR
          o lp->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXW)
        - MSDN talks about some tests being impossible. Check what really happens.
     */

    ver.dwOSVersionInfoSize = sizeof(ver);
    if ((status = RtlGetVersion( (PRTL_OSVERSIONINFOW)&ver )) != STATUS_SUCCESS) return status;

    if(!(TypeMask && ConditionMask)) return STATUS_INVALID_PARAMETER;

    if(TypeMask & VER_PRODUCT_TYPE)
        switch(ConditionMask >> 7*3 & 0x07) {
            case VER_EQUAL:
                if(ver.wProductType != VersionInfo->wProductType) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER:
                if(ver.wProductType <= VersionInfo->wProductType) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER_EQUAL:
                if(ver.wProductType < VersionInfo->wProductType) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS:
                if(ver.wProductType >= VersionInfo->wProductType) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS_EQUAL:
                if(ver.wProductType > VersionInfo->wProductType) return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }
    if(TypeMask & VER_SUITENAME)
        switch(ConditionMask >> 6*3 & 0x07)
        {
            case VER_AND:
                if((VersionInfo->wSuiteMask & ver.wSuiteMask) != VersionInfo->wSuiteMask)
                    return STATUS_REVISION_MISMATCH;
                break;
            case VER_OR:
                if(!(VersionInfo->wSuiteMask & ver.wSuiteMask) && VersionInfo->wSuiteMask)
                    return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }
    if(TypeMask & VER_PLATFORMID)
        switch(ConditionMask >> 3*3 & 0x07)
        {
            case VER_EQUAL:
                if(ver.dwPlatformId != VersionInfo->dwPlatformId) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER:
                if(ver.dwPlatformId <= VersionInfo->dwPlatformId) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER_EQUAL:
                if(ver.dwPlatformId < VersionInfo->dwPlatformId) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS:
                if(ver.dwPlatformId >= VersionInfo->dwPlatformId) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS_EQUAL:
                if(ver.dwPlatformId > VersionInfo->dwPlatformId) return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }
    if(TypeMask & VER_BUILDNUMBER)
        switch(ConditionMask >> 2*3 & 0x07)
        {
            case VER_EQUAL:
                if(ver.dwBuildNumber != VersionInfo->dwBuildNumber) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER:
                if(ver.dwBuildNumber <= VersionInfo->dwBuildNumber) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER_EQUAL:
                if(ver.dwBuildNumber < VersionInfo->dwBuildNumber) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS:
                if(ver.dwBuildNumber >= VersionInfo->dwBuildNumber) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS_EQUAL:
                if(ver.dwBuildNumber > VersionInfo->dwBuildNumber) return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }
    if(TypeMask & VER_MAJORVERSION)
        switch(ConditionMask >> 1*3 & 0x07)
        {
            case VER_EQUAL:
                if(ver.dwMajorVersion != VersionInfo->dwMajorVersion) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER:
                if(ver.dwMajorVersion <= VersionInfo->dwMajorVersion) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER_EQUAL:
                if(ver.dwMajorVersion < VersionInfo->dwMajorVersion) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS:
                if(ver.dwMajorVersion >= VersionInfo->dwMajorVersion) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS_EQUAL:
                if(ver.dwMajorVersion > VersionInfo->dwMajorVersion) return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }
    if(TypeMask & VER_MINORVERSION)
        switch(ConditionMask >> 0*3 & 0x07)
        {
            case VER_EQUAL:
                if(ver.dwMinorVersion != VersionInfo->dwMinorVersion) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER:
                if(ver.dwMinorVersion <= VersionInfo->dwMinorVersion) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER_EQUAL:
                if(ver.dwMinorVersion < VersionInfo->dwMinorVersion) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS:
                if(ver.dwMinorVersion >= VersionInfo->dwMinorVersion) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS_EQUAL:
                if(ver.dwMinorVersion > VersionInfo->dwMinorVersion) return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }
    if(TypeMask & VER_SERVICEPACKMAJOR)
        switch(ConditionMask >> 5*3 & 0x07)
        {
            case VER_EQUAL:
                if(ver.wServicePackMajor != VersionInfo->wServicePackMajor) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER:
                if(ver.wServicePackMajor <= VersionInfo->wServicePackMajor) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER_EQUAL:
                if(ver.wServicePackMajor < VersionInfo->wServicePackMajor) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS:
                if(ver.wServicePackMajor >= VersionInfo->wServicePackMajor) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS_EQUAL:
                if(ver.wServicePackMajor > VersionInfo->wServicePackMajor) return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }
    if(TypeMask & VER_SERVICEPACKMINOR)
        switch(ConditionMask >> 4*3 & 0x07)
        {
            case VER_EQUAL:
                if(ver.wServicePackMinor != VersionInfo->wServicePackMinor) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER:
                if(ver.wServicePackMinor <= VersionInfo->wServicePackMinor) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER_EQUAL:
                if(ver.wServicePackMinor < VersionInfo->wServicePackMinor) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS:
                if(ver.wServicePackMinor >= VersionInfo->wServicePackMinor) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS_EQUAL:
                if(ver.wServicePackMinor > VersionInfo->wServicePackMinor) return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }

    return STATUS_SUCCESS;
}


/*
 Header hell made me do it, don't blame me. Please move these somewhere more
 sensible
*/
#define VER_EQUAL         1
#define VER_GREATER       2
#define VER_GREATER_EQUAL 3
#define VER_LESS          4
#define VER_LESS_EQUAL    5
#define VER_AND           6
#define VER_OR            7

#define VER_CONDITION_MASK              7
#define VER_NUM_BITS_PER_CONDITION_MASK 3

#define VER_MINORVERSION     0x0000001
#define VER_MAJORVERSION     0x0000002
#define VER_BUILDNUMBER      0x0000004
#define VER_PLATFORMID       0x0000008
#define VER_SERVICEPACKMINOR 0x0000010
#define VER_SERVICEPACKMAJOR 0x0000020
#define VER_SUITENAME        0x0000040
#define VER_PRODUCT_TYPE     0x0000080

/*
 * @implemented
 */
ULONGLONG NTAPI
VerSetConditionMask(IN ULONGLONG dwlConditionMask,
                    IN DWORD dwTypeBitMask,
                    IN BYTE dwConditionMask)
{
  if(dwTypeBitMask == 0)
    return dwlConditionMask;

  dwConditionMask &= VER_CONDITION_MASK;

  if(dwConditionMask == 0)
    return dwlConditionMask;

  if(dwTypeBitMask & VER_PRODUCT_TYPE)
    dwlConditionMask |= dwConditionMask << 7 * VER_NUM_BITS_PER_CONDITION_MASK;
  else if(dwTypeBitMask & VER_SUITENAME)
    dwlConditionMask |= dwConditionMask << 6 * VER_NUM_BITS_PER_CONDITION_MASK;
  else if(dwTypeBitMask & VER_SERVICEPACKMAJOR)
    dwlConditionMask |= dwConditionMask << 5 * VER_NUM_BITS_PER_CONDITION_MASK;
  else if(dwTypeBitMask & VER_SERVICEPACKMINOR)
    dwlConditionMask |= dwConditionMask << 4 * VER_NUM_BITS_PER_CONDITION_MASK;
  else if(dwTypeBitMask & VER_PLATFORMID)
    dwlConditionMask |= dwConditionMask << 3 * VER_NUM_BITS_PER_CONDITION_MASK;
  else if(dwTypeBitMask & VER_BUILDNUMBER)
    dwlConditionMask |= dwConditionMask << 2 * VER_NUM_BITS_PER_CONDITION_MASK;
  else if(dwTypeBitMask & VER_MAJORVERSION)
    dwlConditionMask |= dwConditionMask << 1 * VER_NUM_BITS_PER_CONDITION_MASK;
  else if(dwTypeBitMask & VER_MINORVERSION)
    dwlConditionMask |= dwConditionMask << 0 * VER_NUM_BITS_PER_CONDITION_MASK;

  return dwlConditionMask;
}

/* EOF */
