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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

NTSTATUS find_entry( PVOID BaseAddress, LDR_RESOURCE_INFO *info,
                     ULONG level, void **ret, int want_dir );

/* FUNCTIONS ****************************************************************/

_SEH_FILTER(page_fault)
{
    if (_SEH_GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ||
        _SEH_GetExceptionCode() == EXCEPTION_PRIV_INSTRUCTION)
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
    int i;
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
    int min, max, res, pos, namelen;

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

    _SEH_TRY
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
    _SEH_EXCEPT(page_fault)
    {
        status = _SEH_GetExceptionCode();
    }
    _SEH_END;
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

    _SEH_TRY
    {
	if (ResourceInfo) 
        {
            DPRINT( "module %p type %ws name %ws lang %04lx level %ld\n",
                     BaseAddress, (LPCWSTR)ResourceInfo->Type,
                     Level > 1 ? (LPCWSTR)ResourceInfo->Name : L"",
                     Level > 2 ? ResourceInfo->Language : 0, Level );
        }

        status = find_entry( BaseAddress, ResourceInfo, Level, &res, FALSE );
        if (status == STATUS_SUCCESS) *ResourceDataEntry = res;
    }
    _SEH_EXCEPT(page_fault)
    {
        status = _SEH_GetExceptionCode();
    }
    _SEH_END;
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

    _SEH_TRY
    {
	if (info)
        {
            DPRINT( "module %p type %ws name %ws lang %04lx level %ld\n",
                     BaseAddress, (LPCWSTR)info->Type,
                     level > 1 ? (LPCWSTR)info->Name : L"",
                     level > 2 ? info->Language : 0, level );
        }

        status = find_entry( BaseAddress, info, level, &res, TRUE );
        if (status == STATUS_SUCCESS) *addr = res;
    }
    _SEH_EXCEPT(page_fault)
    {
        status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    return status;
}


/*
 * @unimplemented
 */
NTSTATUS NTAPI
LdrEnumResources(IN PVOID BaseAddress,
                 IN PLDR_RESOURCE_INFO ResourceInfo,
                 IN ULONG Level,
                 IN OUT PULONG ResourceCount,
                 OUT PVOID Resources  OPTIONAL)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
