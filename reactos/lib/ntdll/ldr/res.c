/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/ldr/res.c
 * PURPOSE:         Resource access for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 *                  Robert Dickenson (robd@mok.lvcm.com)
 * NOTES:           Parts based on Wine code
 *                  Copyright 1995 Thomas Sandford
 *                  Copyright 1996 Martin von Loewis
 *                  Copyright 2003 Alexandre Julliard
 *                  Copyright 1993 Robert J. Amstadt
 *                  Copyright 1995 Alexandre Julliard
 *                  Copyright 1997 Marcus Meissner
 */

/*
 * TODO:
 *  - any comments ??
 */

/* INCLUDES *****************************************************************/

#include <reactos/config.h>
#include <ddk/ntddk.h>
#include <ntos.h>
#include <ntos/ldrtypes.h>
#include <windows.h>
#include <string.h>
#include <wchar.h>
#include <ntdll/ldr.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* PROTOTYPES ****************************************************************/



/* FUNCTIONS *****************************************************************/

static PIMAGE_RESOURCE_DIRECTORY_ENTRY FASTCALL
FindEntryById(PIMAGE_RESOURCE_DIRECTORY ResDir,
              ULONG Id)
{
    PIMAGE_RESOURCE_DIRECTORY_ENTRY ResEntry;
    LONG low, high, mid, result;

    /* We use ID number instead of string */
    ResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResDir + 1) + ResDir->NumberOfNamedEntries;
    DPRINT("ResEntry %x - Resource ID number instead of string\n", (ULONG)ResEntry);
    DPRINT("EntryCount %d\n", (ULONG)ResDir->NumberOfIdEntries);

    low = 0;
    high = ResDir->NumberOfIdEntries - 1;
    mid = high/2;
    while( low <= high ) {
        result = Id - ResEntry[mid].Name;
        if(result == 0)
            return ResEntry + mid;
        if(result < 0)
            high = mid - 1;
        else
            low = mid + 1;

        mid = (low + high)/2;
    }

    return NULL;
}

static int FASTCALL
PushLanguage(WORD *list, int pos, WORD lang)
{
    int i;

    for (i = 0; i < pos; i++) {
        if (list[i] == lang) {
            return pos;
        }
    }

    list[pos++] = lang;

    return pos;
}

/*
	Status = LdrFindResource_U (hModule,
				    &ResourceInfo,
				    RESOURCE_DATA_LEVEL,
				    &ResourceDataEntry);
 */
NTSTATUS STDCALL
LdrFindResource_U(PVOID BaseAddress,
                  PLDR_RESOURCE_INFO ResourceInfo,
                  ULONG Level,
                  PIMAGE_RESOURCE_DATA_ENTRY* ResourceDataEntry)
{
    PIMAGE_RESOURCE_DIRECTORY ResDir;
    PIMAGE_RESOURCE_DIRECTORY ResBase;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY ResEntry;
    NTSTATUS Status = STATUS_SUCCESS;
    PWCHAR ws;
    ULONG i;
    ULONG Id;
    LONG low, high, mid, result;
    WORD list[9];  /* list of languages to try */
    int j, pos = 0;
    LCID UserLCID, SystemLCID;
    LANGID UserLangID, SystemLangID;
    BOOLEAN MappedAsDataFile;

    MappedAsDataFile = LdrMappedAsDataFile(&BaseAddress);
    DPRINT("LdrFindResource_U(%08x, %08x, %d, %08x)\n", BaseAddress, ResourceInfo, Level, ResourceDataEntry);

    /* Get the pointer to the resource directory */
    ResDir = (PIMAGE_RESOURCE_DIRECTORY)RtlImageDirectoryEntryToData(BaseAddress,
                      ! MappedAsDataFile, IMAGE_DIRECTORY_ENTRY_RESOURCE, &i);
    if (ResDir == NULL) {
        return STATUS_RESOURCE_DATA_NOT_FOUND;
    }

    DPRINT("ResourceDirectory: %x  Size: %d\n", (ULONG)ResDir, (int)i);

    ResBase = ResDir;

    /* Let's go into resource tree */
    for (i = 0; i < (2 < Level ? 2 : Level); i++) {
        DPRINT("ResDir: %x  Level: %d\n", (ULONG)ResDir, i);

        Id = ((PULONG)ResourceInfo)[i];

        if (Id & 0xFFFF0000) {
            /* Resource name is a unicode string */
            ResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResDir + 1);
            DPRINT("ResEntry %x - Resource name is a unicode string\n", (ULONG)ResEntry);
            DPRINT("EntryCount %d\n", (ULONG)ResDir->NumberOfNamedEntries);

            low = 0;
            high = ResDir->NumberOfNamedEntries - 1;
            mid = high/2;
            while( low <= high ) {
               /* Does we need check if it's named entry, think not */
               ws = (PWCHAR)((ULONG)ResBase + (ResEntry[mid].Name & 0x7FFFFFFF));
               result = _wcsnicmp((PWCHAR)Id, ws + 1, *ws);
               /* Need double check for lexical & length */
               if(result == 0) {
                  result = (wcslen((PWCHAR)Id) - (int)*ws);
                  if(result == 0) {
                      ResEntry += mid;
                      goto found;
                  }
               }
               if(result < 0)
                  high = mid - 1;
               else
                  low = mid + 1;

               mid = (low + high)/2;
            }
        } else {
            /* We use ID number instead of string */
            ResEntry = FindEntryById(ResDir, Id);
            if (NULL != ResEntry) goto found;
        }

        switch (i) {
        case 0:
            DPRINT("Error %lu - STATUS_RESOURCE_TYPE_NOT_FOUND\n", i);
            return STATUS_RESOURCE_TYPE_NOT_FOUND;
        case 1:
            DPRINT("Error %lu - STATUS_RESOURCE_NAME_NOT_FOUND\n", i);
            return STATUS_RESOURCE_NAME_NOT_FOUND;
        case 2:
            if (ResDir->NumberOfNamedEntries || ResDir->NumberOfIdEntries) {
                /* Use the first available language */
                ResEntry = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)(ResDir + 1);
                break;
            }
            DPRINT("Error %lu - STATUS_RESOURCE_LANG_NOT_FOUND\n", i);
            return STATUS_RESOURCE_LANG_NOT_FOUND;
         case 3:
            DPRINT("Error %lu - STATUS_RESOURCE_DATA_NOT_FOUND\n", i);
            return STATUS_RESOURCE_DATA_NOT_FOUND;
         default:
            DPRINT("Error %lu - STATUS_INVALID_PARAMETER\n", i);
            return STATUS_INVALID_PARAMETER;
        }
found:;
        ResDir = (PIMAGE_RESOURCE_DIRECTORY)((ULONG)ResBase +
                     (ResEntry->OffsetToData & 0x7FFFFFFF));
    }

    if (3 <= Level) {
        /* 1. specified language */
        pos = PushLanguage(list, pos, ResourceInfo->Language );

        /* 2. specified language with neutral sublanguage */
        pos = PushLanguage(list, pos, MAKELANGID(PRIMARYLANGID(ResourceInfo->Language), SUBLANG_NEUTRAL));

        /* 3. neutral language with neutral sublanguage */
        pos = PushLanguage(list, pos, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));

        /* if no explicitly specified language, try some defaults */
        if (LANG_NEUTRAL == PRIMARYLANGID(ResourceInfo->Language)) {
            /* user defaults, unless SYS_DEFAULT sublanguage specified  */
            if (SUBLANG_SYS_DEFAULT != SUBLANGID(ResourceInfo->Language)) {
                NtQueryDefaultLocale(TRUE, &UserLCID);
                UserLangID = LANGIDFROMLCID(UserLCID);

                /* 4. current thread locale language */
                pos = PushLanguage(list, pos, LANGIDFROMLCID(NtCurrentTeb()->CurrentLocale));

                /* 5. user locale language */
                pos = PushLanguage(list, pos, UserLangID);

                /* 6. user locale language with neutral sublanguage  */
                pos = PushLanguage(list, pos, MAKELANGID(PRIMARYLANGID(UserLangID),
                                                         SUBLANG_NEUTRAL));
            }

            /* now system defaults */
            NtQueryDefaultLocale(FALSE, &SystemLCID);
            SystemLangID = LANGIDFROMLCID(SystemLCID);

            /* 7. system locale language */
            pos = PushLanguage(list, pos, SystemLangID);

            /* 8. system locale language with neutral sublanguage */
            pos = PushLanguage(list, pos, MAKELANGID(PRIMARYLANGID(SystemLangID),
                                                     SUBLANG_NEUTRAL));

            /* 9. English */
            pos = PushLanguage(list, pos, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT));
        }

        ResEntry = NULL;
        for (j = 0; NULL == ResEntry && j < pos; j++)
            ResEntry = FindEntryById(ResDir, list[j]);
        if (NULL == ResEntry) {
            if (ResDir->NumberOfNamedEntries || ResDir->NumberOfIdEntries) {
                /* Use the first available language */
                ResEntry = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)(ResDir + 1);
            } else {
                DPRINT("Error - STATUS_RESOURCE_LANG_NOT_FOUND\n", i);
                return STATUS_RESOURCE_LANG_NOT_FOUND;
            }
        }
        ResDir = (PIMAGE_RESOURCE_DIRECTORY)((ULONG)ResBase +
                     (ResEntry->OffsetToData & 0x7FFFFFFF));
        if (3 < Level) {
            DPRINT("Error - STATUS_INVALID_PARAMETER\n", i);
            return STATUS_INVALID_PARAMETER;
        }
    }
    DPRINT("ResourceDataEntry: %x\n", (ULONG)ResDir);

    if (ResourceDataEntry) {
        *ResourceDataEntry = (PVOID)ResDir;
    }
    return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
LdrAccessResource(IN  PVOID BaseAddress,
                  IN  PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
                  OUT PVOID* Resource OPTIONAL,
                  OUT PULONG Size OPTIONAL)
{
    PIMAGE_SECTION_HEADER Section;
    PIMAGE_NT_HEADERS NtHeader;
    ULONG SectionRva;
    ULONG SectionVa;
    ULONG DataSize;
    ULONG Offset = 0;
    ULONG Data;
    BOOLEAN MappedAsDataFile;

    if(!ResourceDataEntry)
        return STATUS_RESOURCE_DATA_NOT_FOUND;

    MappedAsDataFile = LdrMappedAsDataFile(&BaseAddress);
    Data = (ULONG)RtlImageDirectoryEntryToData(BaseAddress,
                           TRUE, IMAGE_DIRECTORY_ENTRY_RESOURCE, &DataSize);
    if (Data == 0) {
        return STATUS_RESOURCE_DATA_NOT_FOUND;
    }
    if (MappedAsDataFile) {
        /* loaded as ordinary file */
        NtHeader = RtlImageNtHeader(BaseAddress);
        Offset = (ULONG)BaseAddress - Data + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
        Section = RtlImageRvaToSection(NtHeader, BaseAddress, NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress);
        if (Section == NULL) {
            return STATUS_RESOURCE_DATA_NOT_FOUND;
        }
        if (Section->Misc.VirtualSize < ResourceDataEntry->OffsetToData) {
            SectionRva = RtlImageRvaToSection (NtHeader, BaseAddress, ResourceDataEntry->OffsetToData)->VirtualAddress;
            SectionVa = RtlImageRvaToVa(NtHeader, BaseAddress, SectionRva, NULL);
            Offset = SectionRva - SectionVa + Data - Section->VirtualAddress;
        }
    }
    if (Resource) {
        *Resource = (PVOID)(ResourceDataEntry->OffsetToData - Offset + (ULONG)BaseAddress);
    }
    if (Size) {
        *Size = ResourceDataEntry->Size;
    }
    return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
LdrFindResourceDirectory_U(IN PVOID BaseAddress,
                           IN PLDR_RESOURCE_INFO info,
                           IN ULONG level,
                          OUT PIMAGE_RESOURCE_DIRECTORY* addr)
{
    PIMAGE_RESOURCE_DIRECTORY ResDir;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY ResEntry;
    ULONG EntryCount;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR* ws;
    PWCHAR* name = (PWCHAR*) info;

    /* Get the pointer to the resource directory */
    ResDir = (PIMAGE_RESOURCE_DIRECTORY)
    RtlImageDirectoryEntryToData(BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_RESOURCE, &i);
    if (ResDir == NULL) {
        return STATUS_RESOURCE_DATA_NOT_FOUND;
    }

    /* Let's go into resource tree */
    for (i = 0; i < level; i++, name++) {
        EntryCount = ResDir->NumberOfNamedEntries;
        ResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResDir + 1);
        if ((ULONG)(*name) & 0xFFFF0000) {
            /* Resource name is a unicode string */
            for (; EntryCount--; ResEntry++) {
                /* Scan entries for equal name */
                if (ResEntry->Name & 0x80000000) {
                    ws = (WCHAR*)((ULONG)ResDir + (ResEntry->Name & 0x7FFFFFFF));
                    if (!wcsncmp(*name, ws + 1, *ws) && wcslen(*name) == (int)*ws) {
                        goto found;
                    }
                }
            }
        } else {
            /* We use ID number instead of string */
            ResEntry += EntryCount;
            EntryCount = ResDir->NumberOfIdEntries;
            for (; EntryCount--; ResEntry++) {
                /* Scan entries for equal name */
                if (ResEntry->Name == (ULONG)(*name))
                    goto found;
            }
        }
        switch (i) {
        case 0:
            return STATUS_RESOURCE_TYPE_NOT_FOUND;
        case 1:
            return STATUS_RESOURCE_NAME_NOT_FOUND;
        case 2:
            Status = STATUS_RESOURCE_LANG_NOT_FOUND;
            /* Just use first language entry */
            if (ResDir->NumberOfNamedEntries || ResDir->NumberOfIdEntries) {
                ResEntry = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)(ResDir + 1);
                break;
            }
            return Status;
        case 3:
            return STATUS_RESOURCE_DATA_NOT_FOUND;
        default:
            return STATUS_INVALID_PARAMETER;
        }
found:;
        ResDir = (PIMAGE_RESOURCE_DIRECTORY)((ULONG)ResDir + ResEntry->OffsetToData);
    }
    if (addr) {
        *addr = (PVOID)ResDir;
    }
    return Status;
}

/* EOF */
