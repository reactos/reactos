/*
 * ReactOS Hid User Library
 * Copyright (C) 2004-2005 ReactOS Team
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
 /*
  * PROJECT:         ReactOS Hid User Library
  * FILE:            lib/hid/hid.c
  * PURPOSE:         ReactOS Hid User Library
  * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
  *
  * UPDATE HISTORY:
  *      07/12/2004  Created
  */

#include "precomp.h"
#include <hidpmem.h>

#include <winbase.h>

HINSTANCE hDllInstance;

/* device interface GUID for HIDClass devices */
const GUID HidClassGuid = {0x4D1E55B2, 0xF16F, 0x11CF, {0x88,0xCB,0x00,0x11,0x11,0x00,0x00,0x30}};

PVOID
NTAPI
AllocFunction(
    IN ULONG ItemSize)
{
    return LocalAlloc(LPTR, ItemSize);
}

VOID
NTAPI
FreeFunction(
    IN PVOID Item)
{
    LocalFree((HLOCAL)Item);
}

VOID
NTAPI
ZeroFunction(
    IN PVOID Item,
    IN ULONG ItemSize)
{
    memset(Item, 0, ItemSize);
}

VOID
NTAPI
CopyFunction(
    IN PVOID Target,
    IN PVOID Source,
    IN ULONG Length)
{
    memcpy(Target, Source, Length);
}

BOOL WINAPI
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
