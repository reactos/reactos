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
/* $Id: version.c,v 1.1 2004/05/13 21:01:14 navaraf Exp $
 *
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Runtime code
 * FILE:              ntoskrnl/rtl/version.c
 * PROGRAMER:         Filip Navara
 */

/* INCLUDES *****************************************************************/

#define __USE_W32API
#include <ddk/ntddk.h>

/* FUNCTIONS ****************************************************************/

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

/* EOF */
