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
/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/misc.c
 * PURPOSE:         User profile code
 * PROGRAMMER:      Eric Kohl
 */

#include "precomp.h"


/* FUNCTIONS ***************************************************************/

LPWSTR
AppendBackslash (LPWSTR String)
{
  ULONG Length;

  Length = lstrlenW (String);
  if (String[Length - 1] != L'\\')
    {
      String[Length] = L'\\';
      Length++;
      String[Length] = (WCHAR)0;
    }

  return &String[Length];
}


BOOL
GetUserSidFromToken (HANDLE hToken,
		     PUNICODE_STRING SidString)
{
  PSID_AND_ATTRIBUTES SidBuffer;
  ULONG Length;
  NTSTATUS Status;

  Length = 256;
  SidBuffer = LocalAlloc (0,
			  Length);
  if (SidBuffer == NULL)
    return FALSE;

  Status = NtQueryInformationToken (hToken,
				    TokenUser,
				    (PVOID)SidBuffer,
				    Length,
				    &Length);
  if (Status == STATUS_BUFFER_TOO_SMALL)
    {
      SidBuffer = LocalReAlloc (SidBuffer,
				Length,
				0);
      if (SidBuffer == NULL)
	return FALSE;

      Status = NtQueryInformationToken (hToken,
					TokenUser,
					(PVOID)SidBuffer,
					Length,
					&Length);
    }

  if (!NT_SUCCESS (Status))
    {
      LocalFree (SidBuffer);
      return FALSE;
    }

  DPRINT ("SidLength: %lu\n", RtlLengthSid (SidBuffer[0].Sid));

  Status = RtlConvertSidToUnicodeString (SidString,
					 SidBuffer[0].Sid,
					 TRUE);

  LocalFree (SidBuffer);

  if (!NT_SUCCESS (Status))
    return FALSE;

  DPRINT ("SidString.Length: %lu\n", SidString->Length);
  DPRINT ("SidString.MaximumLength: %lu\n", SidString->MaximumLength);
  DPRINT ("SidString: '%wZ'\n", SidString);

  return TRUE;
}

/* Dynamic DLL loading interface **********************************************/

/* OLE32.DLL import table */
DYN_MODULE DynOle32 =
{
  L"ole32.dll",
  {
    "CoInitialize",
    "CoCreateInstance",
    "CoUninitialize",
    NULL
  }
};


/*
 * Use this function to load functions from other modules. We cannot statically
 * link to e.g. ole32.dll because those dlls would get loaded on startup with
 * winlogon and they may try to register classes etc when not even a window station
 * has been created!
 */
BOOL
LoadDynamicImports(PDYN_MODULE Module, PDYN_FUNCS DynFuncs)
{
  LPSTR *fname;
  PVOID *fn;

  ZeroMemory(DynFuncs, sizeof(DYN_FUNCS));

  DynFuncs->hModule = LoadLibraryW(Module->Library);
  if (!DynFuncs->hModule)
    {
      return FALSE;
    }

  fn = &DynFuncs->fn.foo;

  /* load the imports */
  for (fname = Module->Functions; *fname != NULL; fname++)
    {
      *fn = GetProcAddress(DynFuncs->hModule, *fname);
      if (*fn == NULL)
        {
          FreeLibrary(DynFuncs->hModule);
          DynFuncs->hModule = (HMODULE)0;

          return FALSE;
        }

      fn++;
    }

  return TRUE;
}


VOID
UnloadDynamicImports(PDYN_FUNCS DynFuncs)
{
  if (DynFuncs->hModule)
    {
      FreeLibrary(DynFuncs->hModule);
      DynFuncs->hModule = (HMODULE)0;
    }
}

/* EOF */
