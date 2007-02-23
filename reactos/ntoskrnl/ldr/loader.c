/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/loader.c
 * PURPOSE:         Loaders for PE executables
 *
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 *                  Jason Filby (jasonfilby@yahoo.com)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */


/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

static LONG
LdrpCompareModuleNames (
                        IN PUNICODE_STRING String1,
                        IN PUNICODE_STRING String2 )
{
    ULONG len1, len2, i;
    PWCHAR s1, s2, p;
    WCHAR  c1, c2;

    if (String1 && String2)
    {
        /* Search String1 for last path component */
        len1 = String1->Length / sizeof(WCHAR);
        s1 = String1->Buffer;
        for (i = 0, p = String1->Buffer; i < String1->Length; i = i + sizeof(WCHAR), p++)
        {
            if (*p == L'\\')
            {
                if (i == String1->Length - sizeof(WCHAR))
                {
                    s1 = NULL;
                    len1 = 0;
                }
                else
                {
                    s1 = p + 1;
                    len1 = (String1->Length - i) / sizeof(WCHAR);
                }
            }
        }

        /* Search String2 for last path component */
        len2 = String2->Length / sizeof(WCHAR);
        s2 = String2->Buffer;
        for (i = 0, p = String2->Buffer; i < String2->Length; i = i + sizeof(WCHAR), p++)
        {
            if (*p == L'\\')
            {
                if (i == String2->Length - sizeof(WCHAR))
                {
                    s2 = NULL;
                    len2 = 0;
                }
                else
                {
                    s2 = p + 1;
                    len2 = (String2->Length - i) / sizeof(WCHAR);
                }
            }
        }

        /* Compare last path components */
        if (s1 && s2)
        {
            while (1)
            {
                c1 = len1-- ? RtlUpcaseUnicodeChar (*s1++) : 0;
                c2 = len2-- ? RtlUpcaseUnicodeChar (*s2++) : 0;
                if ((c1 == 0 && c2 == L'.') || (c1 == L'.' && c2 == 0))
                    return(0);
                if (!c1 || !c2 || c1 != c2)
                    return(c1 - c2);
            }
        }
    }

    return(0);
}

extern KSPIN_LOCK PsLoadedModuleSpinLock;

//
// Used for checking if a module is already in the module list.
// Used during loading/unloading drivers.
//
PLDR_DATA_TABLE_ENTRY
NTAPI
LdrGetModuleObject ( PUNICODE_STRING ModuleName )
{
    PLDR_DATA_TABLE_ENTRY Module;
    PLIST_ENTRY Entry;
    KIRQL Irql;

    DPRINT("LdrGetModuleObject(%wZ) called\n", ModuleName);

    KeAcquireSpinLock(&PsLoadedModuleSpinLock,&Irql);

    Entry = PsLoadedModuleList.Flink;
    while (Entry != &PsLoadedModuleList)
    {
        Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        DPRINT("Comparing %wZ and %wZ\n",
            &Module->BaseDllName,
            ModuleName);

        if (!LdrpCompareModuleNames(&Module->BaseDllName, ModuleName))
        {
            DPRINT("Module %wZ\n", &Module->BaseDllName);
            KeReleaseSpinLock(&PsLoadedModuleSpinLock, Irql);
            return(Module);
        }

        Entry = Entry->Flink;
    }

    KeReleaseSpinLock(&PsLoadedModuleSpinLock, Irql);

    DPRINT("Could not find module '%wZ'\n", ModuleName);

    return(NULL);
}

//
// Used when unloading drivers
//
NTSTATUS
NTAPI
LdrUnloadModule ( PLDR_DATA_TABLE_ENTRY ModuleObject )
{
    KIRQL Irql;

    /* Remove the module from the module list */
    KeAcquireSpinLock(&PsLoadedModuleSpinLock,&Irql);
    RemoveEntryList(&ModuleObject->InLoadOrderLinks);
    KeReleaseSpinLock(&PsLoadedModuleSpinLock, Irql);

    /* Hook for KDB on unloading a driver. */
    KDB_UNLOADDRIVER_HOOK(ModuleObject);

    /* Free module section */
    //  MmFreeSection(ModuleObject->DllBase);

    ExFreePool(ModuleObject->FullDllName.Buffer);
    ExFreePool(ModuleObject);

    return(STATUS_SUCCESS);
}

/* EOF */
