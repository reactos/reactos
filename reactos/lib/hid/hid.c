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
/* $Id: hid.c,v 1.2 2004/07/12 16:04:37 weiden Exp $
 *
 * PROJECT:         ReactOS Hid User Library
 * FILE:            lib/hid/hid.c
 * PURPOSE:         ReactOS Hid User Library
 *
 * UPDATE HISTORY:
 *      07/12/2004  Created
 */
#include <windows.h>
#include <ddk/hidpi.h>
#include "internal.h"

HINSTANCE hDllInstance;

/* device interface GUID for HIDClass devices */
const GUID HidClassGuid = {0x4D1E55B2, 0xF16F, 0x11CF, {0x88,0xCB,0x00,0x11,0x11,0x00,0x00,0x30}};

BOOL STDCALL
DllMain(HINSTANCE hinstDLL,
        DWORD dwReason,
        LPVOID lpvReserved)
{
  switch(dwReason)
  {
    case DLL_PROCESS_ATTACH:
      hDllInstance = hinstDLL;
      break;
    
    case DLL_THREAD_ATTACH:
      break;
    
    case DLL_THREAD_DETACH:
      break;
    
    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}


/*
 * HidP_GetButtonCaps							EXPORTED
 *
 * @implemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_GetButtonCaps(IN HIDP_REPORT_TYPE ReportType,
                   OUT PHIDP_BUTTON_CAPS ButtonCaps,
                   IN OUT PULONG ButtonCapsLength,
                   IN PHIDP_PREPARSED_DATA PreparsedData)
{
  return HidP_GetSpecificButtonCaps(ReportType, 0, 0, 0, ButtonCaps, ButtonCapsLength, PreparsedData);
}


/*
 * HidD_GetHidGuid							EXPORTED
 *
 * @implemented
 */
HIDAPI
VOID DDKAPI
HidD_GetHidGuid(OUT LPGUID HidGuid)
{
  *HidGuid = HidClassGuid;
}


/*
 * HidP_GetValueCaps							EXPORTED
 *
 * @implemented
 */
HIDAPI
NTSTATUS DDKAPI
HidP_GetValueCaps(IN HIDP_REPORT_TYPE ReportType,
                  OUT PHIDP_VALUE_CAPS ValueCaps,
                  IN OUT PULONG ValueCapsLength,
                  IN PHIDP_PREPARSED_DATA PreparsedData)
{
  return HidP_GetSpecificValueCaps (ReportType, 0, 0, 0, ValueCaps, ValueCapsLength, PreparsedData);
}


/*
 * HidD_Hello								EXPORTED
 *
 * Undocumented easter egg function. It fills the buffer with "Hello\nI hate Jello\n"
 * and returns number of bytes filled in (lstrlen(Buffer) + 1 == 20)
 *
 * Bugs: - doesn't check Buffer for NULL
 *       - always returns 20 even if BufferLength < 20 but doesn't produce a buffer overflow
 *
 * @implemented
 */
HIDAPI
ULONG DDKAPI
HidD_Hello(OUT PCHAR Buffer,
           IN ULONG BufferLength)
{
  const PCHAR const HelloString = "Hello\nI hate Jello\n";
  ULONG StrSize = lstrlenA(HelloString) + sizeof(CHAR);
  
  memcpy(Buffer, HelloString, min(StrSize, BufferLength));
  return StrSize;
}

/* EOF */
