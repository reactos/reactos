/*
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

#define COBJMACROS
#define INITGUID

#include <stdio.h>

#include <windows.h>
#include <mscoree.h>
#include <fusion.h>
#include <corerror.h>

#include "wine/test.h"

typedef struct _tagASSEMBLY ASSEMBLY;

typedef struct
{
    ULONG Signature;
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG Reserved;
    ULONG VersionLength;
    BYTE Version[12];
    BYTE Flags;
    WORD Streams;
} METADATAHDR;

#include <pshpack1.h>

typedef struct
{
    DWORD Offset;
    DWORD Size;
} METADATASTREAMHDR;

typedef struct
{
    DWORD Reserved1;
    BYTE MajorVersion;
    BYTE MinorVersion;
    BYTE HeapOffsetSizes;
    BYTE Reserved2;
    LARGE_INTEGER MaskValid;
    LARGE_INTEGER MaskSorted;
} METADATATABLESHDR;

typedef struct
{
    WORD Generation;
    WORD Name;
    WORD Mvid;
    WORD EncId;
    WORD EncBaseId;
} MODULETABLE;

typedef struct
{
    DWORD Flags;
    WORD Name;
    WORD Namespace;
    WORD Extends;
    WORD FieldList;
    WORD MethodList;
} TYPEDEFTABLE;

typedef struct
{
    DWORD HashAlgId;
    WORD MajorVersion;
    WORD MinorVersion;
    WORD BuildNumber;
    WORD RevisionNumber;
    DWORD Flags;
    WORD PublicKey;
    WORD Name;
    WORD Culture;
} ASSEMBLYTABLE;

typedef struct
{
    DWORD Offset;
    DWORD Flags;
    WORD Name;
    WORD Implementation;
} MANIFESTRESTABLE;

typedef struct
{
    DWORD ImportLookupTable;
    DWORD DateTimeStamp;
    DWORD ForwarderChain;
    DWORD Name;
    DWORD ImportAddressTable;
    BYTE pad[20];
} IMPORTTABLE;

typedef struct
{
    DWORD HintNameTableRVA;
    BYTE pad[8];
} IMPORTLOOKUPTABLE;

typedef struct
{
    WORD Hint;
    BYTE Name[12];
    BYTE Module[12];
    DWORD Reserved;
    WORD EntryPoint;
    DWORD RVA;
} HINTNAMETABLE;

typedef struct
{
    DWORD PageRVA;
    DWORD Size;
    DWORD Relocation;
} RELOCATION;

typedef struct
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[17];
    VS_FIXEDFILEINFO Value;
} VS_VERSIONINFO;

typedef struct
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[13];
} VARFILEINFO;

typedef struct
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[13];
    DWORD Value;
} VAR;

typedef struct
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[15];
} STRINGFILEINFO;

typedef struct
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[9];
} STRINGTABLE;

typedef struct
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
} STRINGHDR;

typedef struct
{
    DWORD Size;
    DWORD Signature;
    DWORD HeaderVersion;
    DWORD SkipBytes;
    BYTE Data[168];
} RESOURCE;

#include <poppack.h>

static struct _tagASSEMBLY
{
    IMAGE_DOS_HEADER doshdr;
    WORD unknown[32];
    IMAGE_NT_HEADERS32 nthdrs;
    IMAGE_SECTION_HEADER text;
    IMAGE_SECTION_HEADER rsrc;
    IMAGE_SECTION_HEADER reloc;
    BYTE pad[16];
    IMAGE_IMPORT_BY_NAME iat;
    BYTE pad2[3];
    IMAGE_COR20_HEADER clrhdr;
    WORD strongname[64];
    RESOURCE resource;
    METADATAHDR metadatahdr;
    METADATASTREAMHDR roothdr;
    BYTE rootname[4];
    METADATASTREAMHDR stringshdr;
    BYTE stringsname[12];
    METADATASTREAMHDR ushdr;
    BYTE usname[4];
    METADATASTREAMHDR guidhdr;
    BYTE guidname[8];
    METADATASTREAMHDR blobhdr;
    BYTE blobname[8];
    METADATATABLESHDR tableshdr;
    DWORD numrows[4];
    MODULETABLE modtable;
    TYPEDEFTABLE tdtable;
    ASSEMBLYTABLE asmtable;
    MANIFESTRESTABLE manifestrestable;
    WORD pad3;
    BYTE stringheap[40];
    WORD usheap[4];
    WORD guidheap[8];
    WORD blobheap[82];
    IMAGE_IMPORT_DESCRIPTOR importdesc;
    BYTE pad4[20];
    IMPORTLOOKUPTABLE importlookup;
    HINTNAMETABLE hintnametable;
    BYTE pad5[108];
    IMAGE_RESOURCE_DIRECTORY topresdir;
    IMAGE_RESOURCE_DIRECTORY_ENTRY labelres;
    IMAGE_RESOURCE_DIRECTORY res11dir;
    IMAGE_RESOURCE_DIRECTORY_ENTRY label11res;
    IMAGE_RESOURCE_DIRECTORY res10dir;
    IMAGE_RESOURCE_DIRECTORY_ENTRY label10res;
    IMAGE_RESOURCE_DATA_ENTRY resdata;
    VS_VERSIONINFO verinfo;
    VARFILEINFO varfileinfo;
    VAR translation;
    STRINGFILEINFO strfileinfo;
    STRINGTABLE strtable;
    STRINGHDR filedeschdr;
    WCHAR filedesckey[17];
    WCHAR filedescval[2];
    STRINGHDR fileverhdr;
    WCHAR fileverkey[13];
    WCHAR fileverval[8];
    STRINGHDR intnamehdr;
    WCHAR intnamekey[13];
    WCHAR intnameval[10];
    STRINGHDR copyrighthdr;
    WCHAR copyrightkey[15];
    WCHAR copyrightval[2];
    STRINGHDR orignamehdr;
    WCHAR orignamekey[17];
    WCHAR orignameval[10];
    STRINGHDR prodverhdr;
    WCHAR prodverkey[15];
    WCHAR prodverval[8];
    STRINGHDR asmverhdr;
    WCHAR asmverkey[17];
    WCHAR asmverval[8];
    WORD pad6[182];
    RELOCATION relocation;
    WORD pad7[250];
} assembly =
{
    /* IMAGE_DOS_HEADER */
    {
        IMAGE_DOS_SIGNATURE, 144, 3, 0, 4, 0, 0xFFFF, 0, 0xB8, 0, 0, 0, 0x40,
        0, { 0  }, 0, 0, { 0 }, 0x80
    },
    /* binary to print "This program cannot be run in DOS mode." */
    {
        0x1F0E, 0x0EBA, 0xB400, 0xCD09, 0xB821, 0x4C01, 0x21CD, 0x6854, 0x7369,
        0x7020, 0x6F72, 0x7267, 0x6D61, 0x6320, 0x6E61, 0x6F6E, 0x2074, 0x6562,
        0x7220, 0x6E75, 0x6920, 0x206E, 0x4F44, 0x2053, 0x6F6D, 0x6564, 0x0D2E,
        0x0A0D, 0x0024, 0x0000, 0x0000, 0x0000
    },
    /* IMAGE_NT_HEADERS32 */
    {
        IMAGE_NT_SIGNATURE, /* Signature */
        /* IMAGE_FILE_HEADER */
        {
            IMAGE_FILE_MACHINE_I386, /* Machine */
            3, /* NumberOfSections */
            0x47EFDF09, /* TimeDateStamp */
            0, /* PointerToSymbolTable */
            0, /* NumberOfSymbols */
            0xE0, /* SizeOfOptionalHeader */
            IMAGE_FILE_32BIT_MACHINE | IMAGE_FILE_LOCAL_SYMS_STRIPPED |
            IMAGE_FILE_LINE_NUMS_STRIPPED | IMAGE_FILE_EXECUTABLE_IMAGE |
            IMAGE_FILE_DLL, /* Characteristics */
        },
        /* IMAGE_OPTIONAL_HEADER32 */
        {
            IMAGE_NT_OPTIONAL_HDR32_MAGIC, /* Magic */
            8, /* MajorLinkerVersion */
            0, /* MinorLinkerVersion */
            0x400, /* SizeOfCode */
            0x600, /* SizeOfInitializedData */
            0, /* SizeOfUninitializedData */
            0x238E, /* AddressOfEntryPoint */
            0x2000, /* BaseOfCode */
            0x4000, /* BaseOfData */
            0x400000, /* ImageBase */
            0x2000, /* SectionAlignment */
            0x200, /* FileAlignment */
            4, /* MajorOperatingSystemVersion */
            0, /* MinorOperatingSystemVersion */
            0, /* MajorImageVersion */
            0, /* MinorImageVersion */
            4, /* MajorSubsystemVersion */
            0, /* MinorSubsystemVersion */
            0, /* Win32VersionValue */
            0x8000, /* SizeOfImage */
            0x200, /* SizeOfHeaders */
            0xB576, /* CheckSum */
            IMAGE_SUBSYSTEM_WINDOWS_CUI, /* Subsystem */
            IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE |
            IMAGE_DLLCHARACTERISTICS_NO_SEH |
            IMAGE_DLLCHARACTERISTICS_NX_COMPAT, /* DllCharacteristics */
            0x100000, /* SizeOfStackReserve */
            0x1000, /* SizeOfStackCommit */
            0x100000, /* SizeOfHeapReserve */
            0x1000, /* SizeOfHeapCommit */
            0, /* LoaderFlags */
            0x10, /* NumberOfRvaAndSizes */
            /* IMAGE_DATA_DIRECTORY */
            {
                { 0 }, /* Export Table */
                { 0x233C, 0x4F }, /* Import Table */
                { 0x4000, 0x298 }, /* Resource Table */
                { 0 }, /* Exception Table */
                { 0 }, /* Certificate Table */
                { 0x6000, 0xC }, /* Base Relocation Table */
                { 0 }, /* Debug */
                { 0 }, /* Copyright */
                { 0 }, /* Global Ptr */
                { 0 }, /* TLS Table */
                { 0 }, /* Load Config Table */
                { 0 }, /* Bound Import */
                { 0x2000, 8 }, /* IAT */
                { 0 }, /* Delay Import Descriptor */
                { 0x2008, 0x48 }, /* CLI Header */
                { 0 } /* Reserved */
            }
        }
    },
    /* IMAGE_SECTION_HEADER */
    {
        ".text", /* Name */
        { 0x394 }, /* Misc.VirtualSize */
        0x2000, /* VirtualAddress */
        0x400, /* SizeOfRawData */
        0x200, /* PointerToRawData */
        0, /* PointerToRelocations */
        0, /* PointerToLinenumbers */
        0, /* NumberOfRelocations */
        0, /* NumberOfLinenumbers */
        IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE |
        IMAGE_SCN_CNT_CODE, /* Characteristics */
    },
    /* IMAGE_SECTION_HEADER */
    {
        ".rsrc", /* Name */
        { 0x298 }, /* Misc.VirtualSize */
        0x4000, /* VirtualAddress */
        0x400, /* SizeOfRawData */
        0x600, /* PointerToRawData */
        0, /* PointerToRelocations */
        0, /* PointerToLinenumbers */
        0, /* NumberOfRelocations */
        0, /* NumberOfLinenumbers */
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ, /* Characteristics */
    },
    /* IMAGE_SECTION_HEADER */
    {
        ".reloc", /* Name */
        { 0xC }, /* Misc.VirtualSize */
        0x6000, /* VirtualAddress */
        0x200, /* SizeOfRawData */
        0xA00, /* PointerToRawData */
        0, /* PointerToRelocations */
        0, /* PointerToLinenumbers */
        0, /* NumberOfRelocations */
        0, /* NumberOfLinenumbers */
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ |
        IMAGE_SCN_MEM_DISCARDABLE, /* Characteristics */
    },
    /* fill */
    { 0 },
    /* IMAGE_IMPORT_BY_NAME */
    {
        0x2370, /* Hint */
        { 0 } /* Name */
    },
    /* fill */
    { 0 },
    /* IMAGE_COR20_HEADER */
    {
        0x48, /* Cb */
        2, /* MajorRuntimeVersion */
        5, /* MinorRuntimeVersion */
        { 0x2188, 0x1B4 }, /* MetaData */
        COMIMAGE_FLAGS_ILONLY | COMIMAGE_FLAGS_STRONGNAMESIGNED, /* Flags */
        { 0 }, /* EntryPointToken */
        { 0x20D0, 0xB8 }, /* Resources */
        { 0x2050, 0x80 }, /* StrongNameSignature */
        { 0 }, /* CodeManagerTable */
        { 0 }, /* VTableFixups */
        { 0 }, /* ExportAddressTableJumps */
        { 0 } /* ManagedNativeHeader */
    },
    { 0xE496, 0x9A6E, 0xD95E, 0xA2A1, 0x5D72, 0x9CEF, 0x41E3, 0xD483,
      0xCB5C, 0x329A, 0x887C, 0xE18E, 0xE664, 0x2E1C, 0x0E61, 0xB361,
      0x8B88, 0xC8D0, 0x47A5, 0x9260, 0x6CC5, 0xE60F, 0x1F61, 0x1E3E,
      0xAFEE, 0x925A, 0xA084, 0x6B44, 0x2DC6, 0x8126, 0xEBC9, 0xD812,
      0xF3E9, 0xA3F3, 0xD0D5, 0x2C7F, 0x4592, 0xA0AF, 0x8B15, 0xD91E,
      0x693E, 0x7A4F, 0x5567, 0xC466, 0xC410, 0x3D29, 0xB25F, 0xCD6C,
      0x53EF, 0x0D29, 0x085A, 0xEC39, 0xE3BD, 0x58E0, 0x78F5, 0x0587,
      0xF8D8, 0x14E4, 0x77CE, 0xCCC9, 0x4DCF, 0x8A18, 0x90E8, 0x1A52
    },
    /* RESOURCE */
    {
        0xB4, /* Size */
        0xBEEFCACE, /* Signature */
        1, /* HeaderVersion */
        0x91, /* SkipBytes */
        { 'l','S','y','s','t','e','m','.','R','e','s','o','u','r','c','e','s','.',
          'R','e','s','o','u','r','c','e','R','e','a','d','e','r',',',' ',
          'm','s','c','o','r','l','i','b',',',' ','V','e','r','s','i','o','n','=',
          '2','.','0','.','0','.','0',',',' ','C','u','l','t','u','r','e','=',
          'n','e','u','t','r','a','l',',',' ','P','u','b','l','i','c','K','e','y','T','o','k','e','n','=',
          'b','7','7','a','5','c','5','6','1','9','3','4','e','0','8','9',
          '#','S','y','s','t','e','m','.','R','e','s','o','u','r','c','e','s','.',
          'R','u','n','t','i','m','e','R','e','s','o','u','r','c','e','S','e','t',
          2,0,0,0,0,0,0,0,0,0,0,0,'P','A','D','P','A','D','P',180,0,0,0
        }
    },
    /* METADATAHDR */
    {
        0x424A5342, /* Signature */
        1, /* MajorVersion */
        1, /* MinorVersion */
        0, /* Reserved */
        0xC, /* VersionLength */
        "v2.0.50727", /* Version */
        0, /* Flags */
        5 /* Streams */
    },
    /* METADATASTREAMHDR */
    {
        0x6C, /* Offset */
        0x64, /* Size */
    },
    "#~\0\0",
    /* METADATASTREAMHDR */
    {
        0xD0, /* Offset */
        0x28, /* Size */
    },
    "#Strings\0\0\0\0",
    /* METADATASTREAMHDR */
    {
        0xF8, /* Offset */
        0x8, /* Size */
    },
    "#US\0",
    /* METADATASTREAMHDR */
    {
        0x100, /* Offset */
        0x10, /* Size */
    },
    "#GUID\0\0\0",
    /* METADATASTREAMHDR */
    {
        0x110, /* Offset */
        0xA4, /* Size */
    },
    "#Blob\0\0\0",
    /* METADATATABLESHDR */
    {
        0, /* Reserved1 */
        2, /* MajorVersion */
        0, /* MinorVersion */
        0, /* HeapOffsetSizes */
        1, /* Reserved2 */
        { { 0 } }, /* MaskValid */
        { { 0 } } /* MaskSorted */
    },
    /* numrows */
    { 1, 1, 1, 1 },
    /* MODULETABLE */
    {
        0, /* Generation */
        0xA, /* Name */
        1, /* Mvid */
        0, /* EncId */
        0 /* EncBaseId */
    },
    /* TYPEDEFTABLE */
    {
        0, /* Flags */
        0x1, /* Name */
        0, /* Namespace */
        0, /* Extends */
        1, /* FieldList */
        1 /* MethodList */
    },
    /* ASSEMBLYTABLE */
    {
        0x8004, /* HashAlgId */
        1, /* MajorVersion */
        0, /* MinorVersion */
        0, /* BuildNumber */
        0, /* RevisionNumber */
        1, /* Flags */
        1, /* PublicKey */
        0x13, /* Name */
        0 /* Culture */
    },
    /* MANIFESTRESTABLE */
    {
        0, /* Offset */
        0x2, /* Flags */
        0x18, /* Name */
        0 /* Implementation */
    },
    /* pad */
    0,
    /* String heap */
    "\0<Module>\0wine.dll\0wine\0wine.resources\0\0",
    /* US heap */
    { 0x0300, 0x0020 },
    /* GUID heap */
    { 0x86EF, 0x5B5A, 0x2C5E, 0x4F6D, 0xC2AB, 0x0A94, 0xD658, 0x31DA },
    /* BLOB heap */
    { 0x8000, 0x00A0, 0x0024, 0x0400, 0x0080, 0x9400, 0x0000, 0x0600,
      0x0002, 0x0000, 0x0024, 0x5200, 0x4153, 0x0031, 0x0004, 0x0100,
      0x0100, 0x2F00, 0x60E0, 0x4D76, 0x5E5C, 0x430A, 0x6FF3, 0x77D6,
      0x04CA, 0xF6AD, 0xF54D, 0x0AD2, 0x9FB6, 0x39C2, 0x2E66, 0xD30F,
      0x916F, 0x1826, 0xFB52, 0x78A0, 0x8262, 0x6902, 0xBD47, 0xAF30,
      0xBAB1, 0x29DA, 0xAA6D, 0xF189, 0x296A, 0x0F13, 0x4982, 0x531D,
      0x8283, 0x1343, 0x5A33, 0x5D36, 0xEB3F, 0x0863, 0xA771, 0x0679,
      0x4DFF, 0xD30A, 0xBEAD, 0x2A9F, 0x12A8, 0x4319, 0x5706, 0x333D,
      0x0CAC, 0xE80A, 0xFD99, 0xC82D, 0x3D3B, 0xBFFE, 0xF256, 0x25E3,
      0x1A12, 0xC116, 0x8936, 0xF237, 0x5F26, 0xC68A, 0x1E42, 0xCE41,
      0xC17C, 0x00C4
    },
    /* IMAGE_IMPORT_DESCRIPTOR */
    {
        { 0x2364 }, /* OriginalFirstThunk */
        0, /* TimeDateStamp */
        0, /* ForwarderChain */
        0x237E, /* Name */
        0x2000, /* FirstThunk */
    },
    /* pad */
    { 0 },
    /* IMPORTLOOKUPTABLE */
    {
        0x2370, /* HintNameTableRVA */
        { 0 }, /* pad */
    },
    /* HINTNAMETABLE */
    {
        0, /* Hint */
        "_CorDllMain", /* Name */
        "mscoree.dll", /* Module */
        0, /* Reserved */
        0x25FF, /* EntryPoint */
        0x402000 /* RVA */
    },
    /* pad to 0x600 */
    { 0 },
    /* IMAGE_RESOURCE_DIRECTORY */
    {
        0, /* Characteristics */
        0, /* TimeDateStamp */
        0, /* MajorVersion */
        0, /* MinorVersion */
        0, /* NumberOfNamedEntries */
        1, /* NumberOfIdEntries */
    },
    /* IMAGE_RESOURCE_DIRECTORY_ENTRY */
    { { { 0 } }, { 0 } }, /* nameless unions initialized later */
    /* IMAGE_RESOURCE_DIRECTORY */
    {
        0, /* Characteristics */
        0, /* TimeDateStamp */
        0, /* MajorVersion */
        0, /* MinorVersion */
        0, /* NumberOfNamedEntries */
        1, /* NumberOfIdEntries */
    },
    /* IMAGE_RESOURCE_DIRECTORY_ENTRY */
    { { { 0 } }, { 0 } }, /* nameless unions initialized later */
    /* IMAGE_RESOURCE_DIRECTORY */
    {
        0, /* Characteristics */
        0, /* TimeDateStamp */
        0, /* MajorVersion */
        0, /* MinorVersion */
        0, /* NumberOfNamedEntries */
        1, /* NumberOfIdEntries */
    },
    /* IMAGE_RESOURCE_DIRECTORY_ENTRY */
    { { { 0 } }, { 0 } }, /* nameless unions initialized later */
    /* IMAGE_RESOURCE_DATA_ENTRY */
    {
        0x4058, /* OffsetToData */
        0x23C, /* Size */
        0, /* CodePage */
        0, /* Reserved */
    },
    /* VS_VERSIONINFO */
    {
        0x23C, /* wLength */
        0x34, /* wValueLength */
        0, /* wType */
        { L"VS_VERSION_INFO" }, /* szKey */
        /* VS_FIXEDFILEINFO */
        {
            VS_FFI_SIGNATURE, /* dwSignature */
            VS_FFI_STRUCVERSION, /* dwStrucVersion */
            0x10000, /* dwFileVersionMS */
            0x00000, /* dwFileVersionLS */
            0x10000, /* dwProductVersionMS */
            0x00000, /* dwProductVersionLS */
            VS_FFI_FILEFLAGSMASK, /* dwFileFlagsMask */
            0x0, /* dwFileFlags */
            VOS__WINDOWS32, /* dwFileOS */
            VFT_DLL, /* dwFileType */
            VFT2_UNKNOWN, /* dwFileSubtype */
            0, /* dwFileDateMS */
            0, /* dwFileDateLS */
        },
    },
    /* VARFILEINFO */
    {
        0x44, /* wLength */
        0, /* wValueLength */
        1, /* wType */
        { L"VarFileInfo" } /* szKey */
    },
    /* VAR */
    {
        0x24, /* wLength */
        0x4, /* wValueLength */
        0, /* wType */
        { L"Translation" }, /* szKey */
        0x4B00000, /* Value */
    },
    /* STRINGFILEINFO */
    {
        0x19C, /* wLength */
        0, /* wValueLength */
        1, /* wType */
        { L"StringFileInfo" }, /* szKey */
    },
    /* STRINGTABLE */
    {
        0x178, /* wLength */
        0, /* wValueLength */
        1, /* wType */
        { L"000004b0" }, /* szKey */
    },
    /* STRINGHDR */
    {
        0x2C, /* wLength */
        2, /* wValueLength */
        1, /* wType */
    },
    { L"FileDescription" }, /* szKey */
    { L" " }, /* szValue */
    /* STRINGHDR */
    {
        0x30, /* wLength */
        8, /* wValueLength */
        1, /* wType */
    },
    { L"FileVersion" }, /* szKey */
    { L"1.0.0.0" }, /* szValue */
    /* STRINGHDR */
    {
        0x34, /* wLength */
        9, /* wValueLength */
        1, /* wType */
    },
    { L"InternalName" }, /* szKey */
    { L"wine.dll" }, /* szValue */
    /* STRINGHDR */
    {
        0x28, /* wLength */
        2, /* wValueLength */
        1, /* wType */
    },
    { L"LegalCopyright" }, /* szKey */
    { L" " }, /* szValue */
    /* STRINGHDR */
    {
        0x3C, /* wLength */
        9, /* wValueLength */
        1, /* wType */
    },
    { L"OriginalFilename" }, /* szKey */
    { L"wine.dll" }, /* szValue */
    /* STRINGHDR */
    {
        0x34, /* wLength */
        8, /* wValueLength */
        1, /* wType */
    },
    { L"ProductVersion" }, /* szKey */
    { L"1.0.0.0" }, /* szValue */
    /* STRINGHDR */
    {
        0x38, /* wLength */
        8, /* wValueLength */
        1, /* wType */
    },
    { L"Assembly Version" }, /* szKey */
    { L"1.0.0.0" }, /* szValue */
    { 0 }, /* pad */
    /* RELOCATION */
    {
        0x2000, /* PageRVA */
        0xC, /* Size */
        0x3390, /* Relocation */
    },
    { 0 }
};


static HRESULT (WINAPI *pCreateAssemblyCache)(IAssemblyCache **ppAsmCache,
                                              DWORD dwReserved);
static HRESULT (WINAPI *pGetCachePath)(ASM_CACHE_FLAGS dwCacheFlags,
                                       LPWSTR pwzCachePath, PDWORD pcchPath);
static HRESULT (WINAPI *pLoadLibraryShim)(LPCWSTR szDllName, LPCWSTR szVersion,
                                          LPVOID pvReserved, HMODULE *phModDll);

static BOOL init_functionpointers(void)
{
    HRESULT hr;
    HMODULE hfusion;
    HMODULE hmscoree;

    hmscoree = LoadLibraryA("mscoree.dll");
    if (!hmscoree)
    {
        win_skip("mscoree.dll not available\n");
        return FALSE;
    }

    pLoadLibraryShim = (void *)GetProcAddress(hmscoree, "LoadLibraryShim");
    if (!pLoadLibraryShim)
    {
        win_skip("LoadLibraryShim not available\n");
        FreeLibrary(hmscoree);
        return FALSE;
    }

    hr = pLoadLibraryShim(L"fusion.dll", NULL, NULL, &hfusion);
    if (FAILED(hr))
    {
        win_skip("fusion.dll not available\n");
        FreeLibrary(hmscoree);
        return FALSE;
    }

    pCreateAssemblyCache = (void *)GetProcAddress(hfusion, "CreateAssemblyCache");
    pGetCachePath = (void *)GetProcAddress(hfusion, "GetCachePath");

    if (!pCreateAssemblyCache || !pGetCachePath)
    {
        win_skip("fusion.dll not implemented\n");
        return FALSE;
    }

    FreeLibrary(hmscoree);
    return TRUE;
}

static void create_file_data(LPCSTR name, LPCSTR data, DWORD size)
{
    HANDLE file;
    DWORD written;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failure to open file %s\n", name);
    WriteFile(file, data, strlen(data), &written, NULL);

    if (size)
    {
        SetFilePointer(file, size, NULL, FILE_BEGIN);
        SetEndOfFile(file);
    }

    CloseHandle(file);
}

#define create_file(name, size) create_file_data(name, name, size)

static void create_assembly(LPCSTR file)
{
    HANDLE hfile;
    DWORD written;

    /* nameless unions initialized here */
    assembly.tableshdr.MaskValid.u.HighPart = 0x101;
    assembly.tableshdr.MaskValid.u.LowPart = 0x00000005;
    assembly.tableshdr.MaskSorted.u.HighPart = 0x1600;
    assembly.tableshdr.MaskSorted.u.LowPart = 0x3301FA00;
    assembly.labelres.Name = 0x10;
    assembly.labelres.OffsetToData = 0x80000018;
    assembly.label11res.Name = 0x1;
    assembly.label11res.OffsetToData = 0x80000030;
    assembly.label10res.Name = 0x0;
    assembly.label10res.OffsetToData = 0x48;

    hfile = CreateFileA(file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);

    WriteFile(hfile, &assembly, sizeof(ASSEMBLY), &written, NULL);
    CloseHandle(hfile);
}

static BOOL check_dotnet20(void)
{
    IAssemblyCache *cache;
    HRESULT hr;
    BOOL ret = FALSE;
    ULONG disp;

    create_assembly("wine.dll");

    hr = pCreateAssemblyCache(&cache, 0);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    hr = IAssemblyCache_InstallAssembly(cache, 0, L"wine.dll", NULL);
    if (hr == S_OK)
        ret = TRUE;
    else if (hr == CLDB_E_FILE_OLDVER)
        win_skip("Tests can't be run on older .NET version (.NET 1.1)\n");
    else if (hr == E_ACCESSDENIED)
        skip("Not enough rights to install an assembly\n");
    else
        ok(0, "Expected S_OK, got %08lx\n", hr);

    DeleteFileA("wine.dll");
    IAssemblyCache_UninstallAssembly(cache, 0, L"wine.dll", NULL, &disp);
    IAssemblyCache_Release(cache);
    return ret;
}

static void test_CreateAssemblyCache(void)
{
    IAssemblyCache *cache;
    HRESULT hr;

    /* NULL ppAsmCache */
    hr = pCreateAssemblyCache(NULL, 0);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);

    /* dwReserved is non-zero */
    hr = pCreateAssemblyCache(&cache, 42);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    IAssemblyCache_Release(cache);
}

static void test_CreateAssemblyCacheItem(void)
{
    IAssemblyCache *cache;
    IAssemblyCacheItem *item;
    HRESULT hr;

    hr = pCreateAssemblyCache(&cache, 0);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    hr = IAssemblyCache_CreateAssemblyCacheItem(cache, 0, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);

    hr = IAssemblyCache_CreateAssemblyCacheItem(cache, 0, NULL, &item, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    IAssemblyCacheItem_Release(item);

    hr = IAssemblyCache_CreateAssemblyCacheItem(cache, 0, NULL, &item, L"wine");
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    IAssemblyCacheItem_Release(item);

    hr = IAssemblyCache_CreateAssemblyCacheItem(cache, 1, (void *)0xdeadbeef, &item, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    IAssemblyCacheItem_Release(item);

    hr = IAssemblyCache_CreateAssemblyCacheItem(cache, 1, NULL, &item, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    IAssemblyCacheItem_Release(item);

    hr = IAssemblyCache_CreateAssemblyCacheItem(cache, 0, (void *)0xdeadbeef, &item, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    IAssemblyCacheItem_Release(item);

    IAssemblyCache_Release(cache);
}

static void test_InstallAssembly(void)
{
    IAssemblyCache *cache;
    HRESULT hr;
    ULONG disp;
    DWORD attr;
    char dllpath[MAX_PATH];

    create_file("test.dll", 100);
    create_assembly("wine.dll");

    hr = pCreateAssemblyCache(&cache, 0);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    /* NULL pszManifestFilePath */
    hr = IAssemblyCache_InstallAssembly(cache, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);

    /* empty pszManifestFilePath */
    hr = IAssemblyCache_InstallAssembly(cache, 0, L"", NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);

    /* pszManifestFilePath has no extension */
    hr = IAssemblyCache_InstallAssembly(cache, 0, L"file", NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME),
       "Expected HRESULT_FROM_WIN32(ERROR_INVALID_NAME), got %08lx\n", hr);

    /* pszManifestFilePath has bad extension */
    hr = IAssemblyCache_InstallAssembly(cache, 0, L"file.bad", NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME),
       "Expected HRESULT_FROM_WIN32(ERROR_INVALID_NAME), got %08lx\n", hr);

    /* pszManifestFilePath has dll extension */
    hr = IAssemblyCache_InstallAssembly(cache, 0, L"file.dll", NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got %08lx\n", hr);

    /* pszManifestFilePath has exe extension */
    hr = IAssemblyCache_InstallAssembly(cache, 0, L"file.exe", NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got %08lx\n", hr);

    /* empty file */
    hr = IAssemblyCache_InstallAssembly(cache, 0, L"test.dll", NULL);
    ok(hr == COR_E_ASSEMBLYEXPECTED,
       "Expected COR_E_ASSEMBLYEXPECTED, got %08lx\n", hr);

    /* wine assembly */
    hr = IAssemblyCache_InstallAssembly(cache, 0, L"wine.dll", NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    GetWindowsDirectoryA(dllpath, MAX_PATH);
    strcat(dllpath, "\\assembly\\GAC_MSIL\\wine\\\\1.0.0.0__2d03617b1c31e2f5\\wine.dll");

    attr = GetFileAttributesA(dllpath);
    ok(attr != INVALID_FILE_ATTRIBUTES, "Expected assembly to exist\n");

    /* uninstall the assembly from the GAC */
    disp = 0xf00dbad;
    hr = IAssemblyCache_UninstallAssembly(cache, 0, L"wine", NULL, &disp);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(disp == IASSEMBLYCACHE_UNINSTALL_DISPOSITION_UNINSTALLED,
       "Expected IASSEMBLYCACHE_UNINSTALL_DISPOSITION_UNINSTALLED, got %ld\n", disp);

    attr = GetFileAttributesA(dllpath);
    ok(attr == INVALID_FILE_ATTRIBUTES, "Expected assembly not to exist\n");

    disp = 0xf00dbad;
    hr = IAssemblyCache_UninstallAssembly(cache, 0, L"wine", NULL, &disp);
    ok(hr == S_FALSE, "Expected S_FALSE, got %08lx\n", hr);
    ok(disp == IASSEMBLYCACHE_UNINSTALL_DISPOSITION_ALREADY_UNINSTALLED,
       "Expected IASSEMBLYCACHE_UNINSTALL_DISPOSITION_ALREADY_UNINSTALLED, got %ld\n", disp);

    DeleteFileA("test.dll");
    DeleteFileA("wine.dll");
    IAssemblyCache_Release(cache);
}

#define INIT_ASM_INFO() \
    ZeroMemory(&info, sizeof(ASSEMBLY_INFO)); \
    info.cbAssemblyInfo = sizeof(ASSEMBLY_INFO); \
    info.pszCurrentAssemblyPathBuf = path; \
    info.cchBuf = MAX_PATH; \
    path[0] = '\0';

static void test_QueryAssemblyInfo(void)
{
    IAssemblyCache *cache;
    ASSEMBLY_INFO info;
    WCHAR path[MAX_PATH];
    WCHAR asmpath[MAX_PATH];
    WCHAR name[MAX_PATH];
    HRESULT hr;
    DWORD size;
    ULONG disp;

    size = MAX_PATH;
    hr = pGetCachePath(ASM_CACHE_GAC, asmpath, &size);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    lstrcatW(asmpath, L"_MSIL\\wine\\1.0.0.0__2d03617b1c31e2f5\\wine.dll");

    create_assembly("wine.dll");

    hr = pCreateAssemblyCache(&cache, 0);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    /* assembly not installed yet */
    INIT_ASM_INFO();
    hr = IAssemblyCache_QueryAssemblyInfo(cache, 0, L"wine", &info);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == 0, "Expected 0, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    ok(info.uliAssemblySizeInKB.u.LowPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, L""),
       "Assembly path was changed\n");
    ok(info.cchBuf == MAX_PATH, "Expected MAX_PATH, got %ld\n", info.cchBuf);

    hr = IAssemblyCache_InstallAssembly(cache, 0, L"wine.dll", NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    /* NULL pszAssemblyName */
    INIT_ASM_INFO();
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_VALIDATE,
                                          NULL, &info);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == 0, "Expected 0, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    ok(info.uliAssemblySizeInKB.u.LowPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, L""),
       "Assembly path was changed\n");
    ok(info.cchBuf == MAX_PATH, "Expected MAX_PATH, got %ld\n", info.cchBuf);

    /* empty pszAssemblyName */
    INIT_ASM_INFO();
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_VALIDATE, L"", &info);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == 0, "Expected 0, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    ok(info.uliAssemblySizeInKB.u.LowPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, L""),
       "Assembly path was changed\n");
    ok(info.cchBuf == MAX_PATH, "Expected MAX_PATH, got %ld\n", info.cchBuf);

    /* no dwFlags */
    INIT_ASM_INFO();
    hr = IAssemblyCache_QueryAssemblyInfo(cache, 0, L"wine", &info);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED,
       "Expected ASSEMBLYINFO_FLAG_INSTALLED, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    ok(info.uliAssemblySizeInKB.u.LowPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, asmpath),
       "Wrong assembly path returned\n");
    ok(info.cchBuf == lstrlenW(asmpath) + 1,
       "Expected %d, got %ld\n", lstrlenW(asmpath) + 1, info.cchBuf);

    /* pwzCachePath is full filename */
    INIT_ASM_INFO();
    hr = IAssemblyCache_QueryAssemblyInfo(cache, 0, L"wine.dll", &info);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == 0, "Expected 0, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    ok(info.uliAssemblySizeInKB.u.LowPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, L""),
       "Assembly path was changed\n");
    ok(info.cchBuf == MAX_PATH, "Expected MAX_PATH, got %ld\n", info.cchBuf);

    /* NULL pAsmInfo, QUERYASMINFO_FLAG_VALIDATE */
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_VALIDATE, L"wine", NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    /* NULL pAsmInfo, QUERYASMINFO_FLAG_GETSIZE */
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE, L"wine", NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    /* info.cbAssemblyInfo is 0 */
    INIT_ASM_INFO();
    info.cbAssemblyInfo = 0;
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_VALIDATE, L"wine", &info);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED,
       "Expected ASSEMBLYINFO_FLAG_INSTALLED, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    ok(info.uliAssemblySizeInKB.u.LowPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, asmpath),
       "Wrong assembly path returned\n");
    ok(info.cchBuf == lstrlenW(asmpath) + 1,
       "Expected %d, got %ld\n", lstrlenW(asmpath) + 1, info.cchBuf);

    /* info.cbAssemblyInfo is 1 */
    INIT_ASM_INFO();
    info.cbAssemblyInfo = 1;
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_VALIDATE, L"wine", &info);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);
    ok(info.cbAssemblyInfo == 1, "Expected 1, got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == 0, "Expected 0, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    ok(info.uliAssemblySizeInKB.u.LowPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, L""),
       "Assembly path was changed\n");
    ok(info.cchBuf == MAX_PATH, "Expected MAX_PATH, got %ld\n", info.cchBuf);

    /* info.cbAssemblyInfo is > sizeof(ASSEMBLY_INFO) */
    INIT_ASM_INFO();
    info.cbAssemblyInfo = sizeof(ASSEMBLY_INFO) * 2;
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE, L"wine", &info);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO) * 2,
       "Expected sizeof(ASSEMBLY_INFO) * 2, got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == 0, "Expected 0, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    ok(info.uliAssemblySizeInKB.u.LowPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, L""),
       "Assembly path was changed\n");
    ok(info.cchBuf == MAX_PATH, "Expected MAX_PATH, got %ld\n", info.cchBuf);

    /* QUERYASMINFO_FLAG_GETSIZE */
    INIT_ASM_INFO();
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE, L"wine", &info);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED,
       "Expected ASSEMBLYINFO_FLAG_INSTALLED, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    todo_wine
    {
        ok((info.uliAssemblySizeInKB.u.LowPart == 4),
           "Expected 4, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    }
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, asmpath),
       "Wrong assembly path returned\n");
    ok(info.cchBuf == lstrlenW(asmpath) + 1,
       "Expected %d, got %ld\n", lstrlenW(asmpath) + 1, info.cchBuf);

    /* QUERYASMINFO_FLAG_GETSIZE and QUERYASMINFO_FLAG_VALIDATE */
    INIT_ASM_INFO();
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE |
                                          QUERYASMINFO_FLAG_VALIDATE, L"wine", &info);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED,
       "Expected ASSEMBLYINFO_FLAG_INSTALLED, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    todo_wine
    {
        ok((info.uliAssemblySizeInKB.u.LowPart == 4),
           "Expected 4, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    }
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, asmpath),
       "Wrong assembly path returned\n");
    ok(info.cchBuf == lstrlenW(asmpath) + 1,
       "Expected %d, got %ld\n", lstrlenW(asmpath) + 1, info.cchBuf);

    /* info.pszCurrentAssemblyPathBuf is NULL */
    INIT_ASM_INFO();
    info.pszCurrentAssemblyPathBuf = NULL;
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE, L"wine", &info);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED,
       "Expected ASSEMBLYINFO_FLAG_INSTALLED, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Expected E_NOT_SUFFICIENT_BUFFER, got %08lx\n", hr);
    todo_wine
    {
        ok((info.uliAssemblySizeInKB.u.LowPart == 4),
           "Expected 4, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    }
    ok(info.cchBuf == lstrlenW(asmpath) + 1,
       "Expected %d, got %ld\n", lstrlenW(asmpath) + 1, info.cchBuf);

    /* info.cchBuf is exactly size of asmpath */
    INIT_ASM_INFO();
    info.cchBuf = lstrlenW(asmpath);
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE, L"wine", &info);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED,
       "Expected ASSEMBLYINFO_FLAG_INSTALLED, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, L""),
       "Assembly path was changed\n");
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Expected E_NOT_SUFFICIENT_BUFFER, got %08lx\n", hr);
    todo_wine
    {
        ok((info.uliAssemblySizeInKB.u.LowPart == 4),
           "Expected 4, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    }
    ok(info.cchBuf == lstrlenW(asmpath) + 1,
       "Expected %d, got %ld\n", lstrlenW(asmpath) + 1, info.cchBuf);

    /* info.cchBuf has room for NULL-terminator */
    INIT_ASM_INFO();
    info.cchBuf = lstrlenW(asmpath) + 1;
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE, L"wine", &info);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED,
       "Expected ASSEMBLYINFO_FLAG_INSTALLED, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    ok(info.cchBuf == lstrlenW(asmpath) + 1,
       "Expected %d, got %ld\n", lstrlenW(asmpath) + 1, info.cchBuf);
    todo_wine
    {
        ok((info.uliAssemblySizeInKB.u.LowPart == 4),
           "Expected 4, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    }
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, asmpath),
       "Wrong assembly path returned\n");

    /* display name is "wine, Version=1.0.0.0" */
    INIT_ASM_INFO();
    lstrcpyW(name, L"wine, Version=1.0.0.0");
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE,
                                          name, &info);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED,
       "Expected ASSEMBLYINFO_FLAG_INSTALLED, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    todo_wine
    {
        ok((info.uliAssemblySizeInKB.u.LowPart == 4),
           "Expected 4, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    }
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, asmpath),
       "Wrong assembly path returned\n");
    ok(info.cchBuf == lstrlenW(asmpath) + 1,
       "Expected %d, got %ld\n", lstrlenW(asmpath) + 1, info.cchBuf);

    /* short buffer, QUERYASMINFO_FLAG_VALIDATE */
    memset(&info, 0, sizeof(info));
    lstrcpyW(name, L"wine, Version=1.0.0.00000");
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_VALIDATE,
                                          name, &info);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got %08lx\n", hr);
    ok(info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED, "got %08lx\n", info.dwAssemblyFlags);

    /* short buffer, QUERYASMINFO_FLAG_GETSIZE */
    memset(&info, 0, sizeof(info));
    lstrcpyW(name, L"wine, Version=1.0.0.00000");
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE,
                                          name, &info);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got %08lx\n", hr);
    ok(info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED, "got %08lx\n", info.dwAssemblyFlags);

    /* display name is "wine, Version=1.0.0.00000" */
    INIT_ASM_INFO();
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE,
                                          name, &info);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED,
       "Expected ASSEMBLYINFO_FLAG_INSTALLED, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    todo_wine
    {
        ok((info.uliAssemblySizeInKB.u.LowPart == 4),
           "Expected 4, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    }
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, asmpath),
       "Wrong assembly path returned\n");
    ok(info.cchBuf == lstrlenW(asmpath) + 1,
       "Expected %d, got %ld\n", lstrlenW(asmpath) + 1, info.cchBuf);

    /* display name is "wine, Version=1.0.0.1", versions don't match */
    INIT_ASM_INFO();
    lstrcpyW(name, L"wine, Version=1.0.0.1");
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE,
                                          name, &info);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == 0, "Expected 0, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    ok(info.uliAssemblySizeInKB.u.LowPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, L""),
       "Assembly path was changed\n");
    ok(info.cchBuf == MAX_PATH, "Expected MAX_PATH, got %ld\n", info.cchBuf);

    /* display name is "wine,version=1.0.0.1,publicKeyToken=2d03617b1c31e2f5,culture=neutral" */
    INIT_ASM_INFO();
    lstrcpyW(name, L"wine,version=1.0.0.1,publicKeyToken=2d03617b1c31e2f5,culture=neutral");
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE,
                                          name, &info);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == 0, "Expected 0, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    ok(info.uliAssemblySizeInKB.u.LowPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, L""),
       "Assembly path was changed\n");
    ok(info.cchBuf == MAX_PATH, "Expected MAX_PATH, got %ld\n", info.cchBuf);

    /* display name is "mscorlib.dll,version=0.0.0.0,culture=neutral,publicKeyToken=null" */
    INIT_ASM_INFO();
    lstrcpyW(name, L"mscorlib.dll,version=0.0.0.0,culture=neutral,publicKeyToken=null");
    hr = IAssemblyCache_QueryAssemblyInfo(cache, 0, name, &info);
    ok(hr == FUSION_E_PRIVATE_ASM_DISALLOWED, "got %08lx\n", hr);

    /* display name is "mscorlib.dll,publicKeyToken=nuLl" */
    INIT_ASM_INFO();
    lstrcpyW(name, L"mscorlib.dll,publicKeyToken=nuLl");
    hr = IAssemblyCache_QueryAssemblyInfo(cache, 0, name, &info);
    ok(hr == FUSION_E_PRIVATE_ASM_DISALLOWED, "got %08lx\n", hr);

    /* display name is "wine, Culture=neutral" */
    INIT_ASM_INFO();
    lstrcpyW(name, L"wine, Culture=neutral");
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE,
                                          name, &info);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED,
       "Expected ASSEMBLYINFO_FLAG_INSTALLED, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    todo_wine
    {
        ok((info.uliAssemblySizeInKB.u.LowPart == 4),
           "Expected 4, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    }
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, asmpath),
       "Wrong assembly path returned\n");
    ok(info.cchBuf == lstrlenW(asmpath) + 1,
       "Expected %d, got %ld\n", lstrlenW(asmpath) + 1, info.cchBuf);

    /* display name is "wine, Culture=en", cultures don't match */
    INIT_ASM_INFO();
    lstrcpyW(name, L"wine, Culture=en");
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE,
                                          name, &info);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got %08lx\n", hr);
    ok(info.dwAssemblyFlags == 0, "Expected 0, got %08lx\n", info.dwAssemblyFlags);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    ok(info.uliAssemblySizeInKB.u.LowPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, L""),
       "Assembly path was changed\n");
    ok(info.cchBuf == MAX_PATH, "Expected MAX_PATH, got %ld\n", info.cchBuf);

    /* display name is "wine, PublicKeyTokens=2d03617b1c31e2f5" */
    INIT_ASM_INFO();
    lstrcpyW(name, L"wine, PublicKeyToken=2d03617b1c31e2f5");
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE,
                                          name, &info);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED,
       "Expected ASSEMBLYINFO_FLAG_INSTALLED, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    todo_wine
    {
        ok((info.uliAssemblySizeInKB.u.LowPart == 4),
           "Expected 4, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    }
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, asmpath),
       "Wrong assembly path returned\n");
    ok(info.cchBuf == lstrlenW(asmpath) + 1,
       "Expected %d, got %ld\n", lstrlenW(asmpath) + 1, info.cchBuf);

    /* display name is "wine, PublicKeyToken=aaaaaaaaaaaaaaaa", pubkeys don't match */
    INIT_ASM_INFO();
    lstrcpyW(name, L"wine, PublicKeyToken=aaaaaaaaaaaaaaaa");
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE,
                                          name, &info);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == 0, "Expected 0, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    ok(info.uliAssemblySizeInKB.u.LowPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, L""),
       "Assembly path was changed\n");
    ok(info.cchBuf == MAX_PATH, "Expected MAX_PATH, got %ld\n", info.cchBuf);

    /* display name is "wine, BadProp=buh", bad property */
    INIT_ASM_INFO();
    lstrcpyW(name, L"wine, BadProp=buh");
    hr = IAssemblyCache_QueryAssemblyInfo(cache, QUERYASMINFO_FLAG_GETSIZE,
                                          name, &info);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED,
       "Expected ASSEMBLYINFO_FLAG_INSTALLED, got %08lx\n", info.dwAssemblyFlags);
    ok(info.uliAssemblySizeInKB.u.HighPart == 0,
       "Expected 0, got %ld\n", info.uliAssemblySizeInKB.u.HighPart);
    todo_wine
    {
        ok((info.uliAssemblySizeInKB.u.LowPart == 4),
           "Expected 4, got %ld\n", info.uliAssemblySizeInKB.u.LowPart);
    }
    ok(!lstrcmpW(info.pszCurrentAssemblyPathBuf, asmpath),
       "Wrong assembly path returned\n");
    ok(info.cchBuf == lstrlenW(asmpath) + 1,
       "Expected %d, got %ld\n", lstrlenW(asmpath) + 1, info.cchBuf);

    /* no flags, display name is "wine, Version=1.0.0.0" */
    INIT_ASM_INFO();
    info.pszCurrentAssemblyPathBuf = NULL;
    info.cchBuf = 0;
    lstrcpyW(name, L"wine, Version=1.0.0.0");
    hr = IAssemblyCache_QueryAssemblyInfo(cache, 0, name, &info);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Expected E_NOT_SUFFICIENT_BUFFER, got %08lx\n", hr);
    ok(info.cbAssemblyInfo == sizeof(ASSEMBLY_INFO),
       "Expected sizeof(ASSEMBLY_INFO), got %ld\n", info.cbAssemblyInfo);
    ok(info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED,
       "Expected ASSEMBLYINFO_FLAG_INSTALLED, got %08lx\n", info.dwAssemblyFlags);

    /* uninstall the assembly from the GAC */
    disp = 0xf00dbad;
    hr = IAssemblyCache_UninstallAssembly(cache, 0, L"wine", NULL, &disp);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(disp == IASSEMBLYCACHE_UNINSTALL_DISPOSITION_UNINSTALLED,
       "Expected IASSEMBLYCACHE_UNINSTALL_DISPOSITION_UNINSTALLED, got %ld\n", disp);

    DeleteFileA("test.dll");
    DeleteFileA("wine.dll");
    IAssemblyCache_Release(cache);
}

START_TEST(asmcache)
{
    if (!init_functionpointers())
        return;

    if (!check_dotnet20())
        return;

    test_CreateAssemblyCache();
    test_CreateAssemblyCacheItem();
    test_InstallAssembly();
    test_QueryAssemblyInfo();
}
