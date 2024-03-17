/*
 * PROJECT:     ReactOS Hid User Library
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     ReactOS Hid User Library
 * COPYRIGHT:   Copyright 2004 Thomas Weidenmueller <w3seek@reactos.com>
 */

#include "precomp.h"
#include <hidpmem.h>

#include <winbase.h>

HINSTANCE hDllInstance;

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
