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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winver.h"
#include "bcrypt.h"
#include "dbghelp.h"
#include "ole2.h"
#include "fusion.h"
#include "corhdr.h"

#include "fusionpriv.h"
#include "wine/debug.h"

#define TableFromToken(tk) (TypeFromToken(tk) >> 24)
#define TokenFromTable(idx) (idx << 24)

#define MAX_CLR_TABLES  64

#define MD_STRINGS_BIT 0x1
#define MD_GUIDS_BIT   0x2
#define MD_BLOBS_BIT   0x4

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

    METADATATABLESHDR *tableshdr;
    DWORD numtables;
    DWORD *numrows;
    CLRTABLE tables[MAX_CLR_TABLES];

    DWORD stringsz;
    DWORD guidsz;
    DWORD blobsz;

    BYTE *strings;
    BYTE *blobs;
};

static DWORD rva_to_offset(IMAGE_NT_HEADERS *nthdrs, DWORD rva)
{
    DWORD offset = rva, limit;
    IMAGE_SECTION_HEADER *img;
    WORD i;

    img = IMAGE_FIRST_SECTION(nthdrs);

    if (rva < img->PointerToRawData)
        return rva;

    for (i = 0; i < nthdrs->FileHeader.NumberOfSections; i++)
    {
        if (img[i].SizeOfRawData)
            limit = img[i].SizeOfRawData;
        else
            limit = img[i].Misc.VirtualSize;

        if (rva >= img[i].VirtualAddress &&
            rva < (img[i].VirtualAddress + limit))
        {
            if (img[i].PointerToRawData != 0)
            {
                offset -= img[i].VirtualAddress;
                offset += img[i].PointerToRawData;
            }

            return offset;
        }
    }

    return 0;
}

static BYTE *GetData(BYTE *pData, ULONG *pLength)
{
    if ((*pData & 0x80) == 0x00)
    {
        *pLength = (*pData & 0x7f);
        return pData + 1;
    }

    if ((*pData & 0xC0) == 0x80)
    {
        *pLength = ((*pData & 0x3f) << 8 | *(pData + 1));
        return pData + 2;
    }

    if ((*pData & 0xE0) == 0xC0)
    {
        *pLength = ((*pData & 0x1f) << 24 | *(pData + 1) << 16 |
                    *(pData + 2) << 8 | *(pData + 3));
        return pData + 4;
    }

    *pLength = (ULONG)-1;
    return 0;
}

static VOID *assembly_data_offset(ASSEMBLY *assembly, ULONG offset)
{
    return &assembly->data[offset];
}

#define MAX_TABLES_WORD 0xFFFF
#define MAX_TABLES_1BIT_ENCODE 32767
#define MAX_TABLES_2BIT_ENCODE 16383
#define MAX_TABLES_3BIT_ENCODE 8191
#define MAX_TABLES_5BIT_ENCODE 2047

static inline ULONG get_table_size(const ASSEMBLY *assembly, DWORD index)
{
    DWORD size;
    INT tables;

    switch (TokenFromTable(index))
    {
        case mdtModule:
        {
            size = sizeof(MODULETABLE) + (assembly->stringsz - sizeof(WORD)) +
                   2 * (assembly->guidsz - sizeof(WORD));
            break;
        }
        case mdtTypeRef:
        {
            size = sizeof(TYPEREFTABLE) + 2 * (assembly->stringsz - sizeof(WORD));

            /* ResolutionScope:ResolutionScope */
            tables = max(assembly->tables[TableFromToken(mdtModule)].rows,
                         assembly->tables[TableFromToken(mdtModuleRef)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtAssemblyRef)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtTypeRef)].rows);
            size += (tables > MAX_TABLES_2BIT_ENCODE) ? sizeof(WORD) : 0;
            break;
        }
        case mdtTypeDef:
        {
            size = sizeof(TYPEDEFTABLE) + 2 * (assembly->stringsz - sizeof(WORD));

            /* Extends:TypeDefOrRef */
            tables = max(assembly->tables[TableFromToken(mdtTypeDef)].rows,
                         assembly->tables[TableFromToken(mdtTypeRef)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtTypeSpec)].rows);
            size += (tables > MAX_TABLES_2BIT_ENCODE) ? sizeof(WORD) : 0;

            size += (assembly->tables[TableFromToken(mdtFieldDef)].rows >
                     MAX_TABLES_WORD) ? sizeof(WORD) : 0;
            size += (assembly->tables[TableFromToken(mdtMethodDef)].rows >
                     MAX_TABLES_WORD) ? sizeof(WORD) : 0;
            break;
        }
        case mdtFieldDef:
        {
            size = sizeof(FIELDTABLE) + (assembly->stringsz - sizeof(WORD)) +
                   (assembly->blobsz - sizeof(WORD));
            break;
        }
        case mdtMethodDef:
        {
            size = sizeof(METHODDEFTABLE) + (assembly->stringsz - sizeof(WORD)) +
                   (assembly->blobsz - sizeof(WORD));

            size += (assembly->tables[TableFromToken(mdtParamDef)].rows >
                     MAX_TABLES_WORD) ? sizeof(WORD) : 0;
            break;
        }
        case mdtParamDef:
        {
            size = sizeof(PARAMTABLE) + (assembly->stringsz - sizeof(WORD));
            break;
        }
        case mdtInterfaceImpl:
        {
            size = sizeof(INTERFACEIMPLTABLE);

            /* Interface:TypeDefOrRef */
            tables = max(assembly->tables[TableFromToken(mdtTypeDef)].rows,
                         assembly->tables[TableFromToken(mdtTypeRef)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtTypeSpec)].rows);
            size += (tables > MAX_TABLES_2BIT_ENCODE) ? sizeof(WORD) : 0;
            break;
        }
        case mdtMemberRef:
        {
            size = sizeof(MEMBERREFTABLE) + (assembly->stringsz - sizeof(WORD)) +
                   (assembly->blobsz - sizeof(WORD));

            /* Class:MemberRefParent */
            tables = max(assembly->tables[TableFromToken(mdtTypeRef)].rows,
                         assembly->tables[TableFromToken(mdtModuleRef)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtMethodDef)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtTypeSpec)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtTypeDef)].rows);
            size += (tables > MAX_TABLES_3BIT_ENCODE) ? sizeof(WORD) : 0;
            break;
        }
        case 0x0B000000: /* FIXME */
        {
            size = sizeof(CONSTANTTABLE) + (assembly->blobsz - sizeof(WORD));

            /* Parent:HasConstant */
            tables = max(assembly->tables[TableFromToken(mdtParamDef)].rows,
                         assembly->tables[TableFromToken(mdtFieldDef)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtProperty)].rows);
            size += (tables > MAX_TABLES_2BIT_ENCODE) ? sizeof(WORD) : 0;
            break;
        }
        case mdtCustomAttribute:
        {
            size = sizeof(CUSTOMATTRIBUTETABLE) + (assembly->blobsz - sizeof(WORD));

            /* Parent:HasCustomAttribute */
            tables = max(assembly->tables[TableFromToken(mdtMethodDef)].rows,
                         assembly->tables[TableFromToken(mdtFieldDef)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtTypeRef)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtTypeDef)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtParamDef)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtInterfaceImpl)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtMemberRef)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtPermission)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtProperty)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtEvent)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtSignature)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtModuleRef)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtTypeSpec)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtAssembly)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtFile)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtExportedType)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtManifestResource)].rows);
            size += (tables > MAX_TABLES_5BIT_ENCODE) ? sizeof(WORD) : 0;

            /* Type:CustomAttributeType */
            tables = max(assembly->tables[TableFromToken(mdtMethodDef)].rows,
                         assembly->tables[TableFromToken(mdtMemberRef)].rows);
            size += (tables > MAX_TABLES_3BIT_ENCODE) ? sizeof(WORD) : 0;
            break;
        }
        case 0x0D000000: /* FIXME */
        {
            size = sizeof(FIELDMARSHALTABLE) + (assembly->blobsz - sizeof(WORD));

            /* Parent:HasFieldMarshal */
            tables = max(assembly->tables[TableFromToken(mdtFieldDef)].rows,
                         assembly->tables[TableFromToken(mdtParamDef)].rows);
            size += (tables > MAX_TABLES_1BIT_ENCODE) ? sizeof(WORD) : 0;
            break;
        }
        case mdtPermission:
        {
            size = sizeof(DECLSECURITYTABLE) + (assembly->blobsz - sizeof(WORD));

            /* Parent:HasDeclSecurity */
            tables = max(assembly->tables[TableFromToken(mdtTypeDef)].rows,
                         assembly->tables[TableFromToken(mdtMethodDef)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtAssembly)].rows);
            size += (tables > MAX_TABLES_2BIT_ENCODE) ? sizeof(WORD) : 0;
            break;
        }
        case 0x0F000000: /* FIXME */
        {
            size = sizeof(CLASSLAYOUTTABLE);
            size += (assembly->tables[TableFromToken(mdtTypeDef)].rows >
                     MAX_TABLES_WORD) ? sizeof(WORD) : 0;
            break;
        }
        case 0x10000000: /* FIXME */
        {
            size = sizeof(FIELDLAYOUTTABLE);
            size += (assembly->tables[TableFromToken(mdtFieldDef)].rows >
                     MAX_TABLES_WORD) ? sizeof(WORD) : 0;
            break;
        }
        case mdtSignature:
        {
            size = sizeof(STANDALONESIGTABLE) + (assembly->blobsz - sizeof(WORD));
            break;
        }
        case 0x12000000: /* FIXME */
        {
            size = sizeof(EVENTMAPTABLE);
            size += (assembly->tables[TableFromToken(mdtTypeDef)].rows >
                     MAX_TABLES_WORD) ? sizeof(WORD) : 0;
            size += (assembly->tables[TableFromToken(mdtEvent)].rows >
                     MAX_TABLES_WORD) ? sizeof(WORD) : 0;
            break;
        }
        case mdtEvent:
        {
            size = sizeof(EVENTTABLE) + (assembly->stringsz - sizeof(WORD));

            /* EventType:TypeDefOrRef */
            tables = max(assembly->tables[TableFromToken(mdtTypeDef)].rows,
                         assembly->tables[TableFromToken(mdtTypeRef)].rows);
            tables = max(tables, assembly->tables[TableFromToken(mdtTypeSpec)].rows);
            size += (tables > MAX_TABLES_2BIT_ENCODE) ? sizeof(WORD) : 0;
            break;
        }
        case 0x15000000:/* FIXME */
        {
            size = sizeof(PROPERTYMAPTABLE);
            size += (assembly->tables[TableFromToken(mdtTypeDef)].rows >
                     MAX_TABLES_WORD) ? sizeof(WORD) : 0;
            size += (assembly->tables[TableFromToken(mdtProperty)].rows >
                     MAX_TABLES_WORD) ? sizeof(WORD) : 0;
            break;
        }
        case mdtProperty:
        {
            size = sizeof(PROPERTYTABLE) + (assembly->stringsz - sizeof(WORD)) +
                   (assembly->blobsz - sizeof(WORD));
            break;
        }
        case 0x18000000: /* FIXME */
        {
            size = sizeof(METHODSEMANTICSTABLE);

            /* Association:HasSemantics */
            tables = max(assembly->tables[TableFromToken(mdtEvent)].rows,
                         assembly->tables[TableFromToken(mdtProperty)].rows);
            size += (tables > MAX_TABLES_1BIT_ENCODE) ? sizeof(WORD) : 0;

            size += (assembly->tables[TableFromToken(mdtMethodDef)].rows >
                     MAX_TABLES_WORD) ? sizeof(WORD) : 0;
            break;
        }
        case 0x19000000: /* FIXME */
        {
            size = sizeof(METHODIMPLTABLE);

            /* MethodBody:MethodDefOrRef, MethodDeclaration:MethodDefOrRef */
            tables = max(assembly->tables[TableFromToken(mdtMethodDef)].rows,
                         assembly->tables[TableFromToken(mdtMemberRef)].rows);
            size += (tables > MAX_TABLES_1BIT_ENCODE) ? 2 * sizeof(WORD) : 0;

            size += (assembly->tables[TableFromToken(mdtTypeDef)].rows >
                     MAX_TABLES_WORD) ? sizeof(WORD) : 0;
            break;
        }
        case mdtModuleRef:
        {
            size = sizeof(MODULEREFTABLE) + (assembly->stringsz - sizeof(WORD));
            break;
        }
        case mdtTypeSpec:
        {
            size = sizeof(TYPESPECTABLE) + (assembly->blobsz - sizeof(WORD));
            break;
        }
        case 0x1C000000: /* FIXME */
        {
            size = sizeof(IMPLMAPTABLE) + (assembly->stringsz - sizeof(WORD));

            /* MemberForwarded:MemberForwarded */
            tables = max(assembly->tables[TableFromToken(mdtFieldDef)].rows,
                         assembly->tables[TableFromToken(mdtMethodDef)].rows);
            size += (tables > MAX_TABLES_1BIT_ENCODE) ? sizeof(WORD) : 0;

            size += (assembly->tables[TableFromToken(mdtModuleRef)].rows >
                     MAX_TABLES_WORD) ? sizeof(WORD) : 0;
            break;
        }
        case 0x1D000000: /* FIXME */
        {
            size = sizeof(FIELDRVATABLE);
            size += (assembly->tables[TableFromToken(mdtFieldDef)].rows >
                     MAX_TABLES_WORD) ? sizeof(WORD) : 0;
            break;
        }
        case mdtAssembly:
        {
            size = sizeof(ASSEMBLYTABLE) + 2 * (assembly->stringsz - sizeof(WORD)) +
                   (assembly->blobsz - sizeof(WORD));
            break;
        }
        case 0x20000001: /* FIXME */
        {
            size = sizeof(ASSEMBLYPROCESSORTABLE);
            break;
        }
        case 0x22000000: /* FIXME */
        {
            size = sizeof(ASSEMBLYOSTABLE);
            break;
        }
        case mdtAssemblyRef:
        {
            size = sizeof(ASSEMBLYREFTABLE) + 2 * (assembly->stringsz - sizeof(WORD)) +
                   2 * (assembly->blobsz - sizeof(WORD));
            break;
        }
        case 0x24000000: /* FIXME */
        {
            size = sizeof(ASSEMBLYREFPROCESSORTABLE);
            size += (assembly->tables[TableFromToken(mdtAssemblyRef)].rows >
                     MAX_TABLES_WORD) ? sizeof(WORD) : 0;
            break;
        }
        case 0x25000000: /* FIXME */
        {
            size = sizeof(ASSEMBLYREFOSTABLE);
            size += (assembly->tables[TableFromToken(mdtAssemblyRef)].rows >
                     MAX_TABLES_WORD) ? sizeof(WORD) : 0;
            break;
        }
        case mdtFile:
        {
            size = sizeof(FILETABLE) + (assembly->stringsz - sizeof(WORD)) +
                   (assembly->blobsz - sizeof(WORD));
            break;
        }
        case mdtExportedType:
        {
            size = sizeof(EXPORTEDTYPETABLE) + 2 * (assembly->stringsz - sizeof(WORD));

            /* Implementation:Implementation */
            tables = max(assembly->tables[TableFromToken(mdtFile)].rows,
                         assembly->tables[TableFromToken(mdtMethodDef)].rows);
            size += (tables > MAX_TABLES_2BIT_ENCODE) ? sizeof(WORD) : 0;
            break;
        }
        case mdtManifestResource:
        {
            size = sizeof(MANIFESTRESTABLE) + (assembly->stringsz - sizeof(WORD));

            /* Implementation:Implementation */
            tables = max(assembly->tables[TableFromToken(mdtFile)].rows,
                         assembly->tables[TableFromToken(mdtAssemblyRef)].rows);
            size += (tables > MAX_TABLES_2BIT_ENCODE) ? sizeof(WORD) : 0;
            break;
        }
        case 0x29000000: /* FIXME */
        {
            size = sizeof(NESTEDCLASSTABLE);
            size += (assembly->tables[TableFromToken(mdtTypeDef)].rows >
                     MAX_TABLES_WORD) ? 2 * sizeof(WORD) : 0;
            break;
        }
        default:
            return 0;
    }

    return size;
}

static HRESULT parse_clr_tables(ASSEMBLY *assembly, ULONG offset)
{
    DWORD i, count;
    ULONG currofs;
    ULONGLONG mask;

    currofs = offset;
    assembly->tableshdr = assembly_data_offset(assembly, currofs);
    if (!assembly->tableshdr)
        return E_FAIL;

    assembly->stringsz = (assembly->tableshdr->HeapOffsetSizes & MD_STRINGS_BIT) ?
                         sizeof(DWORD) : sizeof(WORD);
    assembly->guidsz = (assembly->tableshdr->HeapOffsetSizes & MD_GUIDS_BIT) ?
                       sizeof(DWORD) : sizeof(WORD);
    assembly->blobsz = (assembly->tableshdr->HeapOffsetSizes & MD_BLOBS_BIT) ?
                       sizeof(DWORD) : sizeof(WORD);

    currofs += sizeof(METADATATABLESHDR);
    assembly->numrows = assembly_data_offset(assembly, currofs);
    if (!assembly->numrows)
        return E_FAIL;

    memset(assembly->tables, -1, MAX_CLR_TABLES * sizeof(CLRTABLE));

    for (i = count = 0, mask = 1; i < MAX_CLR_TABLES; i++, mask <<= 1)
    {
        if (assembly->tableshdr->MaskValid.QuadPart & mask)
            assembly->tables[i].rows = assembly->numrows[count++];
    }
    assembly->numtables = count;
    currofs += assembly->numtables * sizeof(DWORD);

    for (i = 0, mask = 1; i < MAX_CLR_TABLES; i++, mask <<= 1)
    {
        if (assembly->tableshdr->MaskValid.QuadPart & mask)
        {
            assembly->tables[i].offset = currofs;
            currofs += get_table_size(assembly, i) * assembly->tables[i].rows;
        }
    }

    return S_OK;
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

    if (!(assembly->metadatahdr = malloc(sizeof(*assembly->metadatahdr)))) return E_OUTOFMEMORY;

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
    METADATASTREAMHDR *streamhdr;
    ULONG rva, i, ofs;
    LPSTR stream;
    HRESULT hr;
    DWORD hdrsz;
    BYTE *ptr;

    hr = parse_metadata_header(assembly, &hdrsz);
    if (FAILED(hr))
        return hr;

    rva = assembly->corhdr->MetaData.VirtualAddress;
    ptr = ImageRvaToVa(assembly->nthdr, assembly->data, rva + hdrsz, NULL);
    if (!ptr)
        return E_FAIL;

    for (i = 0; i < assembly->metadatahdr->Streams; i++)
    {
        streamhdr = (METADATASTREAMHDR *)ptr;
        ofs = rva_to_offset(assembly->nthdr, rva + streamhdr->Offset);

        ptr += sizeof(METADATASTREAMHDR);
        stream = (LPSTR)ptr;

        if (!lstrcmpA(stream, "#~"))
        {
            hr = parse_clr_tables(assembly, ofs);
            if (FAILED(hr))
                return hr;
        }
        else if (!lstrcmpA(stream, "#Strings") || !lstrcmpA(stream, "Strings"))
            assembly->strings = assembly_data_offset(assembly, ofs);
        else if (!lstrcmpA(stream, "#Blob") || !lstrcmpA(stream, "Blob"))
            assembly->blobs = assembly_data_offset(assembly, ofs);

        ptr += ((lstrlenA(stream) + 1) + 3) & ~3; /* align on DWORD boundary */
    }

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

    if (!(assembly = calloc(1, sizeof(*assembly)))) return E_OUTOFMEMORY;

    assembly->path = wcsdup(file);
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

    free(assembly->metadatahdr);
    free(assembly->path);
    UnmapViewOfFile(assembly->data);
    CloseHandle(assembly->hmap);
    CloseHandle(assembly->hfile);
    free(assembly);

    return S_OK;
}

static LPWSTR assembly_dup_str(const ASSEMBLY *assembly, DWORD index)
{
    int len;
    LPWSTR cpy;
    LPCSTR str = (LPCSTR)&assembly->strings[index];

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);

    if ((cpy = malloc(len * sizeof(WCHAR))))
        MultiByteToWideChar(CP_ACP, 0, str, -1, cpy, len);

    return cpy;
}

HRESULT assembly_get_name(ASSEMBLY *assembly, LPWSTR *name)
{
    BYTE *ptr;
    LONG offset;
    DWORD stridx;

    offset = assembly->tables[TableFromToken(mdtAssembly)].offset;
    if (offset == -1)
        return E_FAIL;

    ptr = assembly_data_offset(assembly, offset);
    if (!ptr)
        return E_FAIL;

    ptr += FIELD_OFFSET(ASSEMBLYTABLE, PublicKey) + assembly->blobsz;
    if (assembly->stringsz == sizeof(DWORD))
        stridx = *(DWORD *)ptr;
    else
        stridx = *(WORD *)ptr;

    *name = assembly_dup_str(assembly, stridx);
    if (!*name)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT assembly_get_path(const ASSEMBLY *assembly, LPWSTR *path)
{
    WCHAR *cpy = malloc((lstrlenW(assembly->path) + 1) * sizeof(WCHAR));
    *path = cpy;
    if (cpy)
        lstrcpyW(cpy, assembly->path);
    else
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT assembly_get_version(ASSEMBLY *assembly, LPWSTR *version)
{
    static const WCHAR format[] = {'%','u','.','%','u','.','%','u','.','%','u',0};

    ASSEMBLYTABLE *asmtbl;
    LONG offset;

    *version = NULL;

    offset = assembly->tables[TableFromToken(mdtAssembly)].offset;
    if (offset == -1)
        return E_FAIL;

    asmtbl = assembly_data_offset(assembly, offset);
    if (!asmtbl)
        return E_FAIL;

    if (!(*version = malloc(24 * sizeof(WCHAR))))
        return E_OUTOFMEMORY;

    swprintf(*version, 24, format, asmtbl->MajorVersion, asmtbl->MinorVersion,
             asmtbl->BuildNumber, asmtbl->RevisionNumber);

    return S_OK;
}

PEKIND assembly_get_architecture(ASSEMBLY *assembly)
{
    if ((assembly->corhdr->MajorRuntimeVersion == 2) && (assembly->corhdr->MinorRuntimeVersion == 0))
        return peNone; /* .NET 1.x assembly */

    if (assembly->nthdr->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return peAMD64; /* AMD64/IA64 assembly */

    if ((assembly->corhdr->Flags & COMIMAGE_FLAGS_ILONLY) && !(assembly->corhdr->Flags & COMIMAGE_FLAGS_32BITREQUIRED))
        return peMSIL; /* MSIL assembly */

    return peI386; /* x86 assembly */
}

static BYTE *assembly_get_blob(ASSEMBLY *assembly, DWORD index, ULONG *size)
{
    return GetData(&assembly->blobs[index], size);
}

HRESULT assembly_get_pubkey_token(ASSEMBLY *assembly, LPWSTR *token)
{
    ULONG i, size;
    LONG offset;
    BYTE hashdata[20], *pubkey, *ptr;
    BYTE tokbytes[BYTES_PER_TOKEN];
    WCHAR *tok;
    DWORD idx;

    *token = NULL;

    offset = assembly->tables[TableFromToken(mdtAssembly)].offset;
    if (offset == -1)
        return E_FAIL;

    ptr = assembly_data_offset(assembly, offset);
    if (!ptr)
        return E_FAIL;

    ptr += FIELD_OFFSET(ASSEMBLYTABLE, PublicKey);
    if (assembly->blobsz == sizeof(DWORD))
        idx = *(DWORD *)ptr;
    else
        idx = *(WORD *)ptr;

    pubkey = assembly_get_blob(assembly, idx, &size);

    if (BCryptHash(BCRYPT_SHA1_ALG_HANDLE, NULL, 0, pubkey, size, hashdata, sizeof(hashdata)) != STATUS_SUCCESS)
        return E_FAIL;

    size = sizeof(hashdata);
    for (i = size - 1; i >= size - 8; i--)
        tokbytes[size - i - 1] = hashdata[i];

    if (!(tok = malloc((TOKEN_LENGTH + 1) * sizeof(WCHAR))))
        return E_OUTOFMEMORY;

    token_to_str(tokbytes, tok);

    *token = tok;
    return S_OK;
}

HRESULT assembly_get_runtime_version(ASSEMBLY *assembly, LPSTR *version)
{
    *version = assembly->metadatahdr->Version;
    return S_OK;
}

HRESULT assembly_get_external_files(ASSEMBLY *assembly, LPWSTR **files, DWORD *count)
{
    LONG offset;
    INT i, num_rows;
    WCHAR **ret;
    BYTE *ptr;
    DWORD idx;

    *count = 0;

    offset = assembly->tables[TableFromToken(mdtFile)].offset;
    if (offset == -1)
        return S_OK;

    ptr = assembly_data_offset(assembly, offset);
    if (!ptr)
        return S_OK;

    num_rows = assembly->tables[TableFromToken(mdtFile)].rows;
    if (num_rows <= 0)
        return S_OK;

    if (!(ret = malloc(num_rows * sizeof(WCHAR *)))) return E_OUTOFMEMORY;

    for (i = 0; i < num_rows; i++)
    {
        ptr += sizeof(DWORD); /* skip Flags field */
        if (assembly->stringsz == sizeof(DWORD))
            idx = *(DWORD *)ptr;
        else
            idx = *(WORD *)ptr;

        ret[i] = assembly_dup_str(assembly, idx);
        if (!ret[i])
        {
            for (; i >= 0; i--) free(ret[i]);
            free(ret);
            return E_OUTOFMEMORY;
        }
        ptr += assembly->stringsz; /* skip Name field */
        ptr += assembly->blobsz; /* skip Hash field */
    }
    *count = num_rows;
    *files = ret;
    return S_OK;
}
