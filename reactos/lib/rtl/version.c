/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
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
/* $Id: version.c,v 1.4 2004/11/07 18:45:52 hyperion Exp $
 *
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Runtime code
 * FILE:              lib/rtl/version.c
 * PROGRAMER:         Filip Navara
 */

/* INCLUDES *****************************************************************/

#define __USE_W32API

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

extern ULONG NtGlobalFlag;

/* FUNCTIONS ****************************************************************/

/*
* @implemented
*/
NTSTATUS STDCALL
RtlGetVersion(RTL_OSVERSIONINFOW *Info)
{
   WCHAR CSDString[] = L"Service Pack 6";

   if (Info->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOW) ||
         Info->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
   {
      Info->dwMajorVersion = 4;
      Info->dwMinorVersion = 0;
      Info->dwBuildNumber = 1381;
      Info->dwPlatformId = VER_PLATFORM_WIN32_NT;
      RtlCopyMemory(Info->szCSDVersion, CSDString, sizeof(CSDString));
      if (Info->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
      {
         RTL_OSVERSIONINFOEXW *InfoEx = (RTL_OSVERSIONINFOEXW *)Info;
         InfoEx->wServicePackMajor = 6;
         InfoEx->wServicePackMinor = 0;
         InfoEx->wSuiteMask = 0;
         InfoEx->wProductType = VER_NT_WORKSTATION;
      }

      return STATUS_SUCCESS;
   }

   return STATUS_INVALID_PARAMETER;
}

/*
* @implemented
*/
ULONG
STDCALL
RtlGetNtGlobalFlags(VOID)
{
	return(NtGlobalFlag);
}

/*
* @unimplemented
*/
/*
NTSTATUS
STDCALL
RtlVerifyVersionInfo(
	IN PRTL_OSVERSIONINFOEXW VersionInfo,
	IN ULONG TypeMask,
	IN ULONGLONG  ConditionMask
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
*/

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
ULONGLONG NTAPI VerSetConditionMask
(
 IN ULONGLONG dwlConditionMask,
 IN DWORD dwTypeBitMask,
 IN BYTE dwConditionMask
)
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
