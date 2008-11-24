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
#include "wincrypt.h"
#include "dbghelp.h"
#include "ole2.h"
#include "fusion.h"
#include "corhdr.h"

#include "fusionpriv.h"
#include "wine/debug.h"
#include "wine/unicode.h"

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
    LPSTR path;

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

static LPSTR strdupWtoA(LPCWSTR str)
{
    LPSTR ret = NULL;
    DWORD len;

    if (!str)
        return ret;

    len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    ret = HeapAlloc(GetProcessHeap(), 0, len);
    if (ret)
        WideCharToMultiByte(CP_ACP, 0, str, -1, ret, len, NULL, NULL);

    return ret;
}

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
    return (VOID *)&assembly->data[offset];
}

#define MAX_TABLES_WORD 0xFFFF
#define MAX_TABLES_1BIT_ENCODE 32767
#define MAX_TABLES_2BIT_ENCODE 16383
#define MAX_TABLES_3BIT_ENCODE 8191
#define MAX_TABLES_5BIT_ENCODE 2047

static inline ULONG get_table_size(ASSEMBLY *assembly, DWORD index)
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
    DWORD i, previ, offidx;
    ULONG currofs;

    currofs = offset;
    assembly->tableshdr = (METADATATABLESHDR *)assembly_data_offset(assembly, currofs);
    if (!assembly->tableshdr)
        return E_FAIL;

    assembly->stringsz = (assembly->tableshdr->HeapOffsetSizes & MD_STRINGS_BIT) ?
                         sizeof(DWORD) : sizeof(WORD);
    assembly->guidsz = (assembly->tableshdr->HeapOffsetSizes & MD_GUIDS_BIT) ?
                       sizeof(DWORD) : sizeof(WORD);
    assembly->blobsz = (assembly->tableshdr->HeapOffsetSizes & MD_BLOBS_BIT) ?
                       sizeof(DWORD) : sizeof(WORD);

    currofs += sizeof(METADATATABLESHDR);
    assembly->numrows = (DWORD *)assembly_data_offset(assembly, currofs);
    if (!assembly->numrows)
        return E_FAIL;

    assembly->numtables = 0;
    for (i = 0; i < MAX_CLR_TABLES; i++)
    {
        if ((i < 32 && (assembly->tableshdr->MaskValid.u.LowPart >> i) & 1) ||
            (i >= 32 && (assembly->tableshdr->MaskValid.u.HighPart >> i) & 1))
        {
            assembly->numtables++;
        }
    }

    currofs += assembly->numtables * sizeof(DWORD);
    memset(assembly->tables, -1, MAX_CLR_TABLES * sizeof(CLRTABLE));

    if (assembly->tableshdr->MaskValid.u.LowPart & 1)
        assembly->tables[0].offset = currofs;

    offidx = 0;
    for (i = 0; i < MAX_CLR_TABLES; i++)
    {
        if ((i < 32 && (assembly->tableshdr->MaskValid.u.LowPart >> i) & 1) ||
            (i >= 32 && (assembly->tableshdr->MaskValid.u.HighPart >> i) & 1))
        {
            assembly->tables[i].rows = assembly->numrows[offidx];
            offidx++;
        }
    }

    previ = 0;
    offidx = 1;
    for (i = 1; i < MAX_CLR_TABLES; i++)
    {
        if ((i < 32 && (assembly->tableshdr->MaskValid.u.LowPart >> i) & 1) ||
            (i >= 32 && (assembly->tableshdr->MaskValid.u.HighPart >> i) & 1))
        {
            currofs += get_table_size(assembly, previ) * assembly->numrows[offidx - 1];
            assembly->tables[i].offset = currofs;
            offidx++;
            previ = i;
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

    assembly->metadatahdr = HeapAlloc(GetProcessHeap(), 0, sizeof(METADATAHDR));
    if (!assembly->metadatahdr)
        return E_OUTOFMEMORY;

    size = FIELD_OFFSET(METADATAHDR, Version);
    memcpy(assembly->metadatahdr, metadatahdr, size);

    /* we don't care about the version string */

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
            assembly->strings = (BYTE *)assembly_data_offset(assembly, ofs);
        else if (!lstrcmpA(stream, "#Blob") || !lstrcmpA(stream, "Blob"))
            assembly->blobs = (BYTE *)assembly_data_offset(assembly, ofs);

        ptr += lstrlenA(stream) + 1;
        ptr = (BYTE *)(((UINT_PTR)ptr + 3) & ~3); /* align on DWORD boundary */
    }

    return S_OK;
}

static HRESULT parse_pe_header(ASSEMBLY *assembly)
{
    IMAGE_DATA_DIRECTORY *datadirs;

    assembly->nthdr = ImageNtHeader(assembly->data);
    if (!assembly->nthdr)
        return E_FAIL;

    if (assembly->nthdr->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
    {
        IMAGE_OPTIONAL_HEADER64 *opthdr =
                (IMAGE_OPTIONAL_HEADER64 *)&assembly->nthdr->OptionalHeader;
        datadirs = opthdr->DataDirectory;
    }
    else
        datadirs = assembly->nthdr->OptionalHeader.DataDirectory;

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

    assembly->path = strdupWtoA(file);
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

static LPSTR assembly_dup_str(ASSEMBLY *assembly, DWORD index)
{
    LPSTR str = (LPSTR)&assembly->strings[index];
    LPSTR cpy = HeapAlloc(GetProcessHeap(), 0, strlen(str)+1);
    if (cpy)
       strcpy(cpy, str);
    return cpy;
}

HRESULT assembly_get_name(ASSEMBLY *assembly, LPSTR *name)
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
        stridx = *((DWORD *)ptr);
    else
        stridx = *((WORD *)ptr);

    *name = assembly_dup_str(assembly, stridx);
    if (!*name)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT assembly_get_path(ASSEMBLY *assembly, LPSTR *path)
{
    LPSTR cpy = HeapAlloc(GetProcessHeap(), 0, strlen(assembly->path)+1);
    *path = cpy;
    if (cpy)
        strcpy(cpy, assembly->path);
    else
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT assembly_get_version(ASSEMBLY *assembly, LPSTR *version)
{
    LPSTR verdata;
    VS_FIXEDFILEINFO *ffi;
    HRESULT hr = S_OK;
    DWORD size;

    size = GetFileVersionInfoSizeA(assembly->path, NULL);
    if (!size)
        return HRESULT_FROM_WIN32(GetLastError());

    verdata = HeapAlloc(GetProcessHeap(), 0, size);
    if (!verdata)
        return E_OUTOFMEMORY;

    if (!GetFileVersionInfoA(assembly->path, 0, size, verdata))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    if (!VerQueryValueA(verdata, "\\", (LPVOID *)&ffi, (PUINT)&size))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    *version = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
    if (!*version)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    sprintf(*version, "%d.%d.%d.%d", HIWORD(ffi->dwFileVersionMS),
            LOWORD(ffi->dwFileVersionMS), HIWORD(ffi->dwFileVersionLS),
            LOWORD(ffi->dwFileVersionLS));

done:
    HeapFree(GetProcessHeap(), 0, verdata);
    return hr;
}

HRESULT assembly_get_architecture(ASSEMBLY *assembly, DWORD fixme)
{
    /* FIXME */
    return S_OK;
}

static BYTE *assembly_get_blob(ASSEMBLY *assembly, WORD index, ULONG *size)
{
    return GetData(&assembly->blobs[index], size);
}

static void bytes_to_str(BYTE *bytes, DWORD len, LPSTR str)
{
    DWORD i;

    static const char hexval[16] = {
        '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
    };

    for(i = 0; i < len; i++)
    {
        str[i * 2] = hexval[((bytes[i] >> 4) & 0xF)];
        str[i * 2 + 1] = hexval[(bytes[i]) & 0x0F];
    }
}

#define BYTES_PER_TOKEN 8
#define CHARS_PER_BYTE 2
#define TOKEN_LENGTH (BYTES_PER_TOKEN * CHARS_PER_BYTE + 1)

HRESULT assembly_get_pubkey_token(ASSEMBLY *assembly, LPSTR *token)
{
    ASSEMBLYTABLE *asmtbl;
    ULONG i, size;
    LONG offset;
    BYTE *hashdata;
    HCRYPTPROV crypt;
    HCRYPTHASH hash;
    BYTE *pubkey;
    BYTE tokbytes[BYTES_PER_TOKEN];
    HRESULT hr = E_FAIL;
    LPSTR tok;

    *token = NULL;

    offset = assembly->tables[TableFromToken(mdtAssembly)].offset;
    if (offset == -1)
        return E_FAIL;

    asmtbl = (ASSEMBLYTABLE *)assembly_data_offset(assembly, offset);
    if (!asmtbl)
        return E_FAIL;

    pubkey = assembly_get_blob(assembly, asmtbl->PublicKey, &size);

    if (!CryptAcquireContextA(&crypt, NULL, NULL, PROV_RSA_FULL,
                              CRYPT_VERIFYCONTEXT))
        return E_FAIL;

    if (!CryptCreateHash(crypt, CALG_SHA1, 0, 0, &hash))
        return E_FAIL;

    if (!CryptHashData(hash, pubkey, size, 0))
        return E_FAIL;

    size = 0;
    if (!CryptGetHashParam(hash, HP_HASHVAL, NULL, &size, 0))
        return E_FAIL;

    hashdata = HeapAlloc(GetProcessHeap(), 0, size);
    if (!hashdata)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if (!CryptGetHashParam(hash, HP_HASHVAL, hashdata, &size, 0))
        goto done;

    for (i = size - 1; i >= size - 8; i--)
        tokbytes[size - i - 1] = hashdata[i];

    tok = HeapAlloc(GetProcessHeap(), 0, TOKEN_LENGTH);
    if (!tok)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    bytes_to_str(tokbytes, BYTES_PER_TOKEN, tok);
    tok[TOKEN_LENGTH - 1] = '\0';

    *token = tok;
    hr = S_OK;

done:
    HeapFree(GetProcessHeap(), 0, hashdata);
    CryptDestroyHash(hash);
    CryptReleaseContext(crypt, 0);

    return hr;
}
