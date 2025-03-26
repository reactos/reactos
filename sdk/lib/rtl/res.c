/*
 * PE file resources
 *
 * Copyright 1995 Thomas Sandford
 * Copyright 1996 Martin von Loewis
 * Copyright 2003 Alexandre Julliard
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1997 Marcus Meissner
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

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

NTSTATUS find_entry( PVOID BaseAddress, LDR_RESOURCE_INFO *info,
                     ULONG level, void **ret, int want_dir );

/* FUNCTIONS ****************************************************************/

int page_fault(ULONG ExceptionCode)
{
    if (ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
        ExceptionCode == EXCEPTION_PRIV_INSTRUCTION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

/**********************************************************************
 *  is_data_file_module
 *
 * Check if a module handle is for a LOAD_LIBRARY_AS_DATAFILE module.
 */
static int is_data_file_module( PVOID BaseAddress )
{
    return (ULONG_PTR)BaseAddress & 1;
}


/**********************************************************************
 *  push_language
 *
 * push a language in the list of languages to try
 */
int push_language( USHORT *list, ULONG pos, WORD lang )
{
    ULONG i;
    for (i = 0; i < pos; i++) if (list[i] == lang) return pos;
    list[pos++] = lang;
    return pos;
}


/**********************************************************************
 *  find_first_entry
 *
 * Find the first suitable entry in a resource directory
 */
IMAGE_RESOURCE_DIRECTORY *find_first_entry( IMAGE_RESOURCE_DIRECTORY *dir,
                                            void *root, int want_dir )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    int pos;

    for (pos = 0; pos < dir->NumberOfNamedEntries + dir->NumberOfIdEntries; pos++)
    {
        if (!entry[pos].DataIsDirectory == !want_dir)
            return (IMAGE_RESOURCE_DIRECTORY *)((char *)root + entry[pos].OffsetToDirectory);
    }
    return NULL;
}


/**********************************************************************
 *  find_entry_by_id
 *
 * Find an entry by id in a resource directory
 */
IMAGE_RESOURCE_DIRECTORY *find_entry_by_id( IMAGE_RESOURCE_DIRECTORY *dir,
                                            WORD id, void *root, int want_dir )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    int min, max, pos;

    entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    min = dir->NumberOfNamedEntries;
    max = min + dir->NumberOfIdEntries - 1;
    while (min <= max)
    {
        pos = (min + max) / 2;
        if (entry[pos].Id == id)
        {
            if (!entry[pos].DataIsDirectory == !want_dir)
            {
                DPRINT("root %p dir %p id %04x ret %p\n",
                       root, dir, id, (const char*)root + entry[pos].OffsetToDirectory);
                return (IMAGE_RESOURCE_DIRECTORY *)((char *)root + entry[pos].OffsetToDirectory);
            }
            break;
        }
        if (entry[pos].Id > id) max = pos - 1;
        else min = pos + 1;
    }
    DPRINT("root %p dir %p id %04x not found\n", root, dir, id );
    return NULL;
}


/**********************************************************************
 *  find_entry_by_name
 *
 * Find an entry by name in a resource directory
 */
IMAGE_RESOURCE_DIRECTORY *find_entry_by_name( IMAGE_RESOURCE_DIRECTORY *dir,
                                              LPCWSTR name, void *root,
                                              int want_dir )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    const IMAGE_RESOURCE_DIR_STRING_U *str;
    int min, max, res, pos;
    size_t namelen;

    if (!((ULONG_PTR)name & 0xFFFF0000)) return find_entry_by_id( dir, (ULONG_PTR)name & 0xFFFF, root, want_dir );
    entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    namelen = wcslen(name);
    min = 0;
    max = dir->NumberOfNamedEntries - 1;
    while (min <= max)
    {
        pos = (min + max) / 2;
        str = (const IMAGE_RESOURCE_DIR_STRING_U *)((const char *)root + entry[pos].NameOffset);
        res = _wcsnicmp( name, str->NameString, str->Length );
        if (!res && namelen == str->Length)
        {
            if (!entry[pos].DataIsDirectory == !want_dir)
            {
                DPRINT("root %p dir %p name %ws ret %p\n",
                       root, dir, name, (const char*)root + entry[pos].OffsetToDirectory);
                return (IMAGE_RESOURCE_DIRECTORY *)((char *)root + entry[pos].OffsetToDirectory);
            }
            break;
        }
        if (res < 0) max = pos - 1;
        else min = pos + 1;
    }
    DPRINT("root %p dir %p name %ws not found\n", root, dir, name);
    return NULL;
}

#ifdef __i386__
NTSTATUS NTAPI LdrpAccessResource( PVOID BaseAddress, IMAGE_RESOURCE_DATA_ENTRY *entry,
                                   void **ptr, ULONG *size )
#else
static NTSTATUS LdrpAccessResource( PVOID BaseAddress, IMAGE_RESOURCE_DATA_ENTRY *entry,
                                    void **ptr, ULONG *size )
#endif
{
    NTSTATUS status = STATUS_SUCCESS;

    _SEH2_TRY
    {
        ULONG dirsize;

        if (!RtlImageDirectoryEntryToData( BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_RESOURCE, &dirsize ))
            status = STATUS_RESOURCE_DATA_NOT_FOUND;
        else
        {
            if (ptr)
            {
                if (is_data_file_module(BaseAddress))
                {
                    PVOID mod = (PVOID)((ULONG_PTR)BaseAddress & ~1);
                    *ptr = RtlImageRvaToVa( RtlImageNtHeader(mod), mod, entry->OffsetToData, NULL );
                }
                else *ptr = (char *)BaseAddress + entry->OffsetToData;
            }
            if (size) *size = entry->Size;
        }
    }
    _SEH2_EXCEPT(page_fault(_SEH2_GetExceptionCode()))
    {
        status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    return status;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
LdrFindResource_U(PVOID BaseAddress,
                  PLDR_RESOURCE_INFO ResourceInfo,
                  ULONG Level,
                  PIMAGE_RESOURCE_DATA_ENTRY* ResourceDataEntry)
{
    void *res;
    NTSTATUS status = STATUS_SUCCESS;

    _SEH2_TRY
    {
        if (ResourceInfo)
        {
            DPRINT( "module %p type %lx name %lx lang %04lx level %lu\n",
                     BaseAddress, ResourceInfo->Type,
                     Level > 1 ? ResourceInfo->Name : 0,
                     Level > 2 ? ResourceInfo->Language : 0, Level );
        }

        status = find_entry( BaseAddress, ResourceInfo, Level, &res, FALSE );
        if (NT_SUCCESS(status))
            *ResourceDataEntry = res;
    }
    _SEH2_EXCEPT(page_fault(_SEH2_GetExceptionCode()))
    {
        status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    return status;
}

#ifndef __i386__
/*
 * @implemented
 */
NTSTATUS NTAPI
LdrAccessResource(IN  PVOID BaseAddress,
                  IN  PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
                  OUT PVOID* Resource OPTIONAL,
                  OUT PULONG Size OPTIONAL)
{
    return LdrpAccessResource( BaseAddress, ResourceDataEntry, Resource, Size );
}
#endif

/*
 * @implemented
 */
NTSTATUS NTAPI
LdrFindResourceDirectory_U(IN PVOID BaseAddress,
                           IN PLDR_RESOURCE_INFO info,
                           IN ULONG level,
                           OUT PIMAGE_RESOURCE_DIRECTORY* addr)
{
    void *res;
    NTSTATUS status = STATUS_SUCCESS;

    _SEH2_TRY
    {
        if (info)
        {
            DPRINT( "module %p type %ws name %ws lang %04lx level %lu\n",
                     BaseAddress, (LPCWSTR)info->Type,
                     level > 1 ? (LPCWSTR)info->Name : L"",
                     level > 2 ? info->Language : 0, level );
        }

        status = find_entry( BaseAddress, info, level, &res, TRUE );
        if (NT_SUCCESS(status))
            *addr = res;
    }
    _SEH2_EXCEPT(page_fault(_SEH2_GetExceptionCode()))
    {
        status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    return status;
}


#define NAME_FROM_RESOURCE_ENTRY(RootDirectory, Entry) \
    ((Entry)->NameIsString ? (ULONG_PTR)(RootDirectory) + (Entry)->NameOffset : (Entry)->Id)

static
LONG
LdrpCompareResourceNames_U(
    _In_ PUCHAR ResourceData,
    _In_ PIMAGE_RESOURCE_DIRECTORY_ENTRY Entry,
    _In_ ULONG_PTR CompareName)
{
    PIMAGE_RESOURCE_DIR_STRING_U ResourceString;
    PWSTR String1, String2;
    USHORT ResourceStringLength;
    WCHAR Char1, Char2;

    /* Check if the resource name is an ID */
    if (CompareName <= USHRT_MAX)
    {
        /* Just compare the 2 IDs */
        return (CompareName - Entry->Id);
    }
    else
    {
        /* Get the resource string */
        ResourceString = (PIMAGE_RESOURCE_DIR_STRING_U)(ResourceData +
                                                        Entry->NameOffset);

        /* Get the string length */
        ResourceStringLength = ResourceString->Length;

        String1 = ResourceString->NameString;
        String2 = (PWSTR)CompareName;

        /* Loop all characters of the resource string */
        while (ResourceStringLength--)
        {
            /* Get the next characters */
            Char1 = *String1++;
            Char2 = *String2++;

            /* Check if they don't match, or if the compare string ends */
            if ((Char1 != Char2) || (Char2 == 0))
            {
                /* They don't match, fail */
                return Char2 - Char1;
            }
        }

        /* All characters match, check if the compare string ends here */
        return (*String2 == 0) ? 0 : 1;
    }
}

NTSTATUS
NTAPI
LdrEnumResources(
    _In_ PVOID ImageBase,
    _In_ PLDR_RESOURCE_INFO ResourceInfo,
    _In_ ULONG Level,
    _Inout_ ULONG *ResourceCount,
    _Out_writes_to_(*ResourceCount,*ResourceCount) LDR_ENUM_RESOURCE_INFO *Resources)
{
    PUCHAR ResourceData;
    NTSTATUS Status;
    ULONG i, j, k;
    ULONG NumberOfTypeEntries, NumberOfNameEntries, NumberOfLangEntries;
    ULONG Count, MaxResourceCount;
    PIMAGE_RESOURCE_DIRECTORY TypeDirectory, NameDirectory, LangDirectory;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY TypeEntry, NameEntry, LangEntry;
    PIMAGE_RESOURCE_DATA_ENTRY DataEntry;
    ULONG Size;
    LONG Result;

    /* If the caller wants data, get the maximum count of entries */
    MaxResourceCount = (Resources != NULL) ? *ResourceCount : 0;

    /* Default to 0 */
    *ResourceCount = 0;

    /* Locate the resource directory */
    ResourceData = RtlImageDirectoryEntryToData(ImageBase,
                                                TRUE,
                                                IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                                &Size);
    if (ResourceData == NULL)
        return STATUS_RESOURCE_DATA_NOT_FOUND;

    /* The type directory is at the root, followed by the entries */
    TypeDirectory = (PIMAGE_RESOURCE_DIRECTORY)ResourceData;
    TypeEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(TypeDirectory + 1);

    /* Get the number of entries in the type directory */
    NumberOfTypeEntries = TypeDirectory->NumberOfNamedEntries +
                          TypeDirectory->NumberOfIdEntries;

    /* Start with 0 resources and status success */
    Status = STATUS_SUCCESS;
    Count = 0;

    /* Loop all entries in the type directory */
    for (i = 0; i < NumberOfTypeEntries; ++i, ++TypeEntry)
    {
        /* Check if comparison of types is requested */
        if (Level > RESOURCE_TYPE_LEVEL)
        {
            /* Compare the type with the requested Type */
            Result = LdrpCompareResourceNames_U(ResourceData,
                                                TypeEntry,
                                                ResourceInfo->Type);

            /* Not equal, continue with next entry */
            if (Result != 0) continue;
        }

        /* The entry must point to the name directory */
        if (!TypeEntry->DataIsDirectory)
        {
            return STATUS_INVALID_IMAGE_FORMAT;
        }

        /* Get a pointer to the name subdirectory and it's first entry */
        NameDirectory = (PIMAGE_RESOURCE_DIRECTORY)(ResourceData +
                                                    TypeEntry->OffsetToDirectory);
        NameEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(NameDirectory + 1);

        /* Get the number of entries in the name directory */
        NumberOfNameEntries = NameDirectory->NumberOfNamedEntries +
                              NameDirectory->NumberOfIdEntries;

        /* Loop all entries in the name directory */
        for (j = 0; j < NumberOfNameEntries; ++j, ++NameEntry)
        {
            /* Check if comparison of names is requested */
            if (Level > RESOURCE_NAME_LEVEL)
            {
                /* Compare the name with the requested name */
                Result = LdrpCompareResourceNames_U(ResourceData,
                                                    NameEntry,
                                                    ResourceInfo->Name);

                /* Not equal, continue with next entry */
                if (Result != 0) continue;
            }

            /* The entry must point to the language directory */
            if (!NameEntry->DataIsDirectory)
            {
                return STATUS_INVALID_IMAGE_FORMAT;
            }

            /* Get a pointer to the language subdirectory and it's first entry */
            LangDirectory = (PIMAGE_RESOURCE_DIRECTORY)(ResourceData +
                                                        NameEntry->OffsetToDirectory);
            LangEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(LangDirectory + 1);

            /* Get the number of entries in the language directory */
            NumberOfLangEntries = LangDirectory->NumberOfNamedEntries +
                                  LangDirectory->NumberOfIdEntries;

            /* Loop all entries in the language directory */
            for (k = 0; k < NumberOfLangEntries; ++k, ++LangEntry)
            {
                /* Check if comparison of languages is requested */
                if (Level > RESOURCE_LANGUAGE_LEVEL)
                {
                    /* Compare the language with the requested language */
                    Result = LdrpCompareResourceNames_U(ResourceData,
                                                        LangEntry,
                                                        ResourceInfo->Language);

                    /* Not equal, continue with next entry */
                    if (Result != 0) continue;
                }

                /* This entry must point to data */
                if (LangEntry->DataIsDirectory)
                {
                    return STATUS_INVALID_IMAGE_FORMAT;
                }

                /* Get a pointer to the data entry */
                DataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)(ResourceData +
                                                         LangEntry->OffsetToData);

                /* Check if there is still space to store the data */
                if (Count < MaxResourceCount)
                {
                    /* There is, fill the entry */
                    Resources[Count].Type =
                        NAME_FROM_RESOURCE_ENTRY(ResourceData, TypeEntry);
                    Resources[Count].Name =
                        NAME_FROM_RESOURCE_ENTRY(ResourceData, NameEntry);
                    Resources[Count].Language =
                        NAME_FROM_RESOURCE_ENTRY(ResourceData, LangEntry);
                    Resources[Count].Data = (PUCHAR)ImageBase + DataEntry->OffsetToData;
                    Resources[Count].Reserved = 0;
                    Resources[Count].Size = DataEntry->Size;
                }
                else
                {
                    /* There is not enough space, save error status */
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }

                /* Count this resource */
                ++Count;
            }
        }
    }

    /* Return the number of matching resources */
    *ResourceCount = Count;
    return Status;
}
