/* $Id: misc.c,v 1.3 2004/07/11 22:35:07 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/misc.c
 * PURPOSE:         User profile code
 * PROGRAMMER:      Eric Kohl
 */

#include <ntos.h>
#include <windows.h>
#include <string.h>

#include "internal.h"


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
LPSTR Ole32Imports[] =
{
  "CoInitialize",
  "CoCreateInstance",
  "CoUninitialize",
};

DYN_MODULE DynOle32 = 
{
  L"ole32.dll",
  sizeof(Ole32Imports) / sizeof(LPSTR),
  Ole32Imports
};

/*
   Use this function to load functions from other modules. We cannot statically
   link to e.g. ole32.dll because those dlls would get loaded on startup with
   winlogon and they may try to register classes etc when not even a window station
   has been created!
*/

BOOL
LoadDynamicImports(PDYN_MODULE Module, PDYN_FUNCS DynFuncs)
{
  int i;
  PVOID *fn;
  
  ZeroMemory(DynFuncs, sizeof(DYN_FUNCS));
  
  DynFuncs->hModule = LoadLibraryW(Module->Library);
  if(!DynFuncs->hModule)
  {
    return FALSE;
  }
  
  /* begin with the first function */
  fn = &DynFuncs->fn.foo; /* warning: assignment from incompatible pointer type */
  
  /* load the imports */
  for(i = 0; i < Module->nFunctions; i++)
  {
    *fn = GetProcAddress(DynFuncs->hModule, Module->Functions[i]);
    if(*fn == NULL)
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
  if(DynFuncs->hModule)
  {
    FreeLibrary(DynFuncs->hModule);
    DynFuncs->hModule = (HMODULE)0;
  }
}

/* EOF */
