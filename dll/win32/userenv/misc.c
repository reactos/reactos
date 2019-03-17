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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/userenv/misc.c
 * PURPOSE:         User profile code
 * PROGRAMMER:      Eric Kohl
 */

#include "precomp.h"

#include <ndk/sefuncs.h>

#define NDEBUG
#include <debug.h>

SID_IDENTIFIER_AUTHORITY LocalSystemAuthority = {SECURITY_NT_AUTHORITY};
SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};

/* FUNCTIONS ***************************************************************/

LPWSTR
AppendBackslash(LPWSTR String)
{
    ULONG Length;

    Length = lstrlenW(String);
    if (String[Length - 1] != L'\\')
    {
        String[Length] = L'\\';
        Length++;
        String[Length] = (WCHAR)0;
    }

    return &String[Length];
}


PSECURITY_DESCRIPTOR
CreateDefaultSecurityDescriptor(VOID)
{
    PSID LocalSystemSid = NULL;
    PSID AdministratorsSid = NULL;
    PSID EveryoneSid = NULL;
    PACL Dacl;
    DWORD DaclSize;
    PSECURITY_DESCRIPTOR pSD = NULL;

    /* create the SYSTEM, Administrators and Everyone SIDs */
    if (!AllocateAndInitializeSid(&LocalSystemAuthority,
                                  1,
                                  SECURITY_LOCAL_SYSTEM_RID,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  &LocalSystemSid) ||
        !AllocateAndInitializeSid(&LocalSystemAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  &AdministratorsSid) ||
        !AllocateAndInitializeSid(&WorldAuthority,
                                  1,
                                  SECURITY_WORLD_RID,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  &EveryoneSid))
    {
        DPRINT1("Failed initializing the SIDs for the default security descriptor (0x%p, 0x%p, 0x%p)\n",
                LocalSystemSid, AdministratorsSid, EveryoneSid);
        goto Cleanup;
    }

    /* allocate the security descriptor and DACL */
    DaclSize = sizeof(ACL) +
               ((GetLengthSid(LocalSystemSid) +
                 GetLengthSid(AdministratorsSid) +
                 GetLengthSid(EveryoneSid)) +
                (3 * FIELD_OFFSET(ACCESS_ALLOWED_ACE,
                                  SidStart)));

    pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED,
                                           (SIZE_T)DaclSize + sizeof(SECURITY_DESCRIPTOR));
    if (pSD == NULL)
    {
        DPRINT1("Failed to allocate the default security descriptor and ACL\n");
        goto Cleanup;
    }

    if (!InitializeSecurityDescriptor(pSD,
                                      SECURITY_DESCRIPTOR_REVISION))
    {
        DPRINT1("Failed to initialize the default security descriptor\n");
        goto Cleanup;
    }

    /* initialize and build the DACL */
    Dacl = (PACL)((ULONG_PTR)pSD + sizeof(SECURITY_DESCRIPTOR));
    if (!InitializeAcl(Dacl,
                       (DWORD)DaclSize,
                       ACL_REVISION))
    {
        DPRINT1("Failed to initialize the DACL of the default security descriptor\n");
        goto Cleanup;
    }

    /* add the SYSTEM Ace */
    if (!AddAccessAllowedAce(Dacl,
                             ACL_REVISION,
                             GENERIC_ALL,
                             LocalSystemSid))
    {
        DPRINT1("Failed to add the SYSTEM ACE\n");
        goto Cleanup;
    }

    /* add the Administrators Ace */
    if (!AddAccessAllowedAce(Dacl,
                             ACL_REVISION,
                             GENERIC_ALL,
                             AdministratorsSid))
    {
        DPRINT1("Failed to add the Administrators ACE\n");
        goto Cleanup;
    }

    /* add the Everyone Ace */
    if (!AddAccessAllowedAce(Dacl,
                             ACL_REVISION,
                             GENERIC_EXECUTE,
                             EveryoneSid))
    {
        DPRINT1("Failed to add the Everyone ACE\n");
        goto Cleanup;
    }

    /* set the DACL */
    if (!SetSecurityDescriptorDacl(pSD,
                                   TRUE,
                                   Dacl,
                                   FALSE))
    {
        DPRINT1("Failed to set the DACL of the default security descriptor\n");

Cleanup:
        if (pSD != NULL)
        {
            LocalFree((HLOCAL)pSD);
            pSD = NULL;
        }
    }

    if (LocalSystemSid != NULL)
    {
        FreeSid(LocalSystemSid);
    }
    if (AdministratorsSid != NULL)
    {
        FreeSid(AdministratorsSid);
    }
    if (EveryoneSid != NULL)
    {
        FreeSid(EveryoneSid);
    }

    return pSD;
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
LoadDynamicImports(PDYN_MODULE Module,
                   PDYN_FUNCS DynFuncs)
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
