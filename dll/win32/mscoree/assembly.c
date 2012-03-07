/*
 * assembly parser
 *
 * Copyright 2008 James Hawkins
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winver.h"
#include "dbghelp.h"
#include "ole2.h"
#include "mscoree.h"
#include "corhdr.h"
#include "metahost.h"
#include "cordebug.h"
#include "wine/list.h"
#include "mscoree_private.h"

#include "wine/debug.h"
#include "wine/unicode.h"

typedef struct
{
    ULONG Signature;
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG Reserved;
    ULONG VersionLength;
    LPSTR Version;
    BYTE Flags;
    WORD Streams;
} METADATAHDR;

typedef struct
{
    DWORD Offset;
    DWORD Size;
} METADATASTREAMHDR;

typedef struct tagCLRTABLE
{
    INT rows;
    DWORD offset;
} CLRTABLE;

struct tagASSEMBLY
{
    LPWSTR path;

    HANDLE hfile;
    HANDLE hmap;
    BYTE *data;

    IMAGE_NT_HEADERS *nthdr;
    IMAGE_COR20_HEADER *corhdr;

    METADATAHDR *metadatahdr;
};

static inline LPWSTR strdupW(LPCWSTR src)
{
    LPWSTR dest;

    if (!src)
        return NULL;

    dest = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(src) + 1) * sizeof(WCHAR));
    if (dest)
        lstrcpyW(dest, src);

    return dest;
}

static HRESULT parse_metadata_header(ASSEMBLY *assembly, DWORD *hdrsz)
{
    METADATAHDR *metadatahdr;
    BYTE *ptr, *dest;
    DWORD size, ofs;
    ULONG rva;

    rva = assembly->corhdr->MetaData.VirtualAddress;
    ptr = ImageRvaToVa(assembly->nthdr, assembly->data, rva, NULL);
    if (!ptr)
        return E_FAIL;

    metadatahdr = (METADATAHDR *)ptr;

    assembly->metadatahdr = HeapAlloc(GetProcessHeap(), 0, sizeof(METADATAHDR));
    if (!assembly->metadatahdr)
        return E_OUTOFMEMORY;

    size = FIELD_OFFSET(METADATAHDR, Version);
    memcpy(assembly->metadatahdr, metadatahdr, size);

    assembly->metadatahdr->Version = (LPSTR)&metadatahdr->Version;

    ofs = FIELD_OFFSET(METADATAHDR, Flags);
    ptr += FIELD_OFFSET(METADATAHDR, Version) + metadatahdr->VersionLength + 1;
    dest = (BYTE *)assembly->metadatahdr + ofs;
    memcpy(dest, ptr, sizeof(METADATAHDR) - ofs);

    *hdrsz = sizeof(METADATAHDR) - sizeof(LPSTR) + metadatahdr->VersionLength + 1;

    return S_OK;
}

static HRESULT parse_clr_metadata(ASSEMBLY *assembly)
{
    HRESULT hr;
    DWORD hdrsz;

    hr = parse_metadata_header(assembly, &hdrsz);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

static HRESULT parse_pe_header(ASSEMBLY *assembly)
{
    IMAGE_DATA_DIRECTORY *datadirs;

    assembly->nthdr = ImageNtHeader(assembly->data);
    if (!assembly->nthdr)
        return E_FAIL;

    if (assembly->nthdr->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        IMAGE_OPTIONAL_HEADER64 *opthdr =
                (IMAGE_OPTIONAL_HEADER64 *)&assembly->nthdr->OptionalHeader;
        datadirs = opthdr->DataDirectory;
    }
    else
    {
        IMAGE_OPTIONAL_HEADER32 *opthdr =
                (IMAGE_OPTIONAL_HEADER32 *)&assembly->nthdr->OptionalHeader;
        datadirs = opthdr->DataDirectory;
    }

    if (!datadirs)
        return E_FAIL;

    if (!datadirs[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress ||
        !datadirs[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size)
    {
        return E_FAIL;
    }

    assembly->corhdr = ImageRvaToVa(assembly->nthdr, assembly->data,
        datadirs[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress, NULL);
    if (!assembly->corhdr)
        return E_FAIL;

    return S_OK;
}

HRESULT assembly_create(ASSEMBLY **out, LPCWSTR file)
{
    ASSEMBLY *assembly;
    HRESULT hr;

    *out = NULL;

    assembly = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ASSEMBLY));
    if (!assembly)
        return E_OUTOFMEMORY;

    assembly->path = strdupW(file);
    if (!assembly->path)
    {
        hr = E_OUTOFMEMORY;
        goto failed;
    }

    assembly->hfile = CreateFileW(file, GENERIC_READ, FILE_SHARE_READ,
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (assembly->hfile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }

    assembly->hmap = CreateFileMappingW(assembly->hfile, NULL, PAGE_READONLY,
                                        0, 0, NULL);
    if (!assembly->hmap)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }

    assembly->data = MapViewOfFile(assembly->hmap, FILE_MAP_READ, 0, 0, 0);
    if (!assembly->data)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }

    hr = parse_pe_header(assembly);
    if (FAILED(hr)) goto failed;

    hr = parse_clr_metadata(assembly);
    if (FAILED(hr)) goto failed;

    *out = assembly;
    return S_OK;

failed:
    assembly_release(assembly);
    return hr;
}

HRESULT assembly_release(ASSEMBLY *assembly)
{
    if (!assembly)
        return S_OK;

    HeapFree(GetProcessHeap(), 0, assembly->metadatahdr);
    HeapFree(GetProcessHeap(), 0, assembly->path);
    UnmapViewOfFile(assembly->data);
    CloseHandle(assembly->hmap);
    CloseHandle(assembly->hfile);
    HeapFree(GetProcessHeap(), 0, assembly);

    return S_OK;
}

HRESULT assembly_get_runtime_version(ASSEMBLY *assembly, LPSTR *version)
{
    *version = assembly->metadatahdr->Version;

    return S_OK;
}
