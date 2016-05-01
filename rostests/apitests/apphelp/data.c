/*
 * Copyright 2015 Mark Jansen
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

#include <exdisp.h>
#include <winbase.h>
#include "wine/test.h"


static IMAGE_DOS_HEADER dos_header =
{
    IMAGE_DOS_SIGNATURE, /* e_magic */
    144, /* e_cblp */
    3, /* e_cp */
    0, /* e_crlc */
    4, /* e_cparhdr */
    0, /* e_minalloc */
    65535, /* e_maxalloc */
    0, /* e_ss */
    184, /* e_sp */
    0, /* e_csum */
    0, /* e_ip */
    0, /* e_cs */
    64, /* e_lfarlc */
    0, /* e_ovno */
    { 0 }, /* e_res[4] */
    0, /* e_oemid */
    0, /* e_oeminfo */
    { 0 }, /* e_res2[10] */
    0x80 /* e_lfanew */
};

static IMAGE_NT_HEADERS32 nt_header =
{
    IMAGE_NT_SIGNATURE, /* Signature */
    {
        IMAGE_FILE_MACHINE_I386, /* Machine */
        2, /* NumberOfSections */
        0x12345, /* TimeDateStamp */
        0, /* PointerToSymbolTable */
        0, /* NumberOfSymbols */
        sizeof(IMAGE_OPTIONAL_HEADER), /* SizeOfOptionalHeader */
        IMAGE_FILE_RELOCS_STRIPPED | IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LINE_NUMS_STRIPPED | IMAGE_FILE_32BIT_MACHINE /* Characteristics */
    },
    {
        IMAGE_NT_OPTIONAL_HDR_MAGIC, /* Magic */
        6, /* MajorLinkerVersion */
        3, /* MinorLinkerVersion */
        0, /* SizeOfCode */
        0, /* SizeOfInitializedData */
        0, /* SizeOfUninitializedData */
        0x1000, /* AddressOfEntryPoint */
        0x1000, /* BaseOfCode */
#ifndef _WIN64
        0, /* BaseOfData */
#endif
        0x400000, /* ImageBase */
        0x1000, /* SectionAlignment */
        0x200, /* FileAlignment */
        4, /* MajorOperatingSystemVersion */
        1, /* MinorOperatingSystemVersion */
        4, /* MajorImageVersion */
        2, /* MinorImageVersion */
        4, /* MajorSubsystemVersion */
        3, /* MinorSubsystemVersion */
        0, /* Win32VersionValue */
        0x3000, /* SizeOfImage */
        0x200, /* SizeOfHeaders */
        0xBAAD, /* CheckSum: This checksum is not the correct checksum, intentionally! */
        IMAGE_SUBSYSTEM_WINDOWS_CUI, /* Subsystem */
        0, /* DllCharacteristics */
        0x100000, /* SizeOfStackReserve */
        0x1000, /* SizeOfStackCommit */
        0x100000, /* SizeOfHeapReserve */
        0x1000, /* SizeOfHeapCommit */
        0, /* LoaderFlags */
        0x10, /* NumberOfRvaAndSizes */
        {
            /* IMAGE_DIRECTORY_ENTRY_EXPORT */
            {
                0x2370, /* VirtualAddress */
                76, /* Size */
            },
            { 0 },
            /* IMAGE_DIRECTORY_ENTRY_RESOURCE */
            {
                0x2000, /* VirtualAddress */
                868, /* Size */
            },
        } /* DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES] */
    }
};

static IMAGE_SECTION_HEADER section_headers[] =
{
    {
        { '.','t','e','x','t',0 }, /* Name */
        { 24 }, /* VirtualSize */
        0x1000, /* VirtualAddress */
        0x200, /* SizeOfRawData */
        0x200, /* PointerToRawData */
        0, /* PointerToRelocations */
        0, /* PointerToLinenumbers */
        0, /* NumberOfRelocations */
        0, /* NumberOfLinenumbers */
        IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ /* Characteristics */
    },
    {
        { '.','r','s','r','c',0 }, /* Name */
        { 880 }, /* VirtualSize */
        0x2000, /* VirtualAddress */
        0x400, /* SizeOfRawData */
        0x400, /* PointerToRawData */
        0, /* PointerToRelocations */
        0, /* PointerToLinenumbers */
        0, /* NumberOfRelocations */
        0, /* NumberOfLinenumbers */
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ /* Characteristics */
    }
};

static const unsigned char text_section[] =
{
    0x33, 0xc0,     /* xor eax, eax */
    0xc3            /* ret */
};


/* taken from fusionpriv.h */
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

typedef struct rsrc_section_t
{
    IMAGE_RESOURCE_DIRECTORY header;
    IMAGE_RESOURCE_DIRECTORY_ENTRY file_info_id;
    IMAGE_RESOURCE_DIRECTORY file_info_header;
    IMAGE_RESOURCE_DIRECTORY_ENTRY version_info_id;
    IMAGE_RESOURCE_DIRECTORY version_info_header;
    IMAGE_RESOURCE_DIRECTORY_ENTRY version_lang_id;
    IMAGE_RESOURCE_DATA_ENTRY version_data_entry;
    
    VS_VERSIONINFO version_info;
    STRINGFILEINFO string_file_info;
    STRINGTABLE string_table;

    STRINGHDR FileVersion_hdr;
    WCHAR FileVersion_key[13];
    WCHAR FileVersion_val[8];

    STRINGHDR ProductVersion_hdr;
    WCHAR ProductVersion_key[15];
    WCHAR ProductVersion_val[8];

    STRINGHDR CompanyName_hdr;
    WCHAR CompanyName_key[13];
    WCHAR CompanyName_val[12];

    STRINGHDR FileDescription_hdr;
    WCHAR FileDescription_key[17];
    WCHAR FileDescription_val[16];

    STRINGHDR InternalName_hdr;
    WCHAR InternalName_key[13];
    WCHAR InternalName_val[14];

    STRINGHDR LegalCopyright_hdr;
    WCHAR LegalCopyright_key[15];
    WCHAR LegalCopyright_val[16];

    STRINGHDR LegalTrademarks_hdr;
    WCHAR LegalTrademarks_key[17];
    WCHAR LegalTrademarks_val[16];

    STRINGHDR OriginalFilename_hdr;
    WCHAR OriginalFilename_key[17];
    WCHAR OriginalFilename_val[18];

    STRINGHDR Productname_hdr;
    WCHAR Productname_key[13];
    WCHAR Productname_val[12];

    VARFILEINFO file_info;
    VAR translation;
} rsrc_section_t;

static const rsrc_section_t rsrc_section =
{
    /* header */
    {
        0, /* Characteristics */
        0x55FE8E21, /* TimeDateStamp, 20/09/2015 10:44:49 */
        0, /* MajorVersion */
        0, /* MinorVersion */
        0, /* NumberOfNamedEntries */
        1, /* NumberOfIdEntries */
    },
    /* file_info_id */
    {
        {{
            (DWORD)VS_FILE_INFO, /* NameOffset:31 */
            0 /* NameIsString:1 */
        }},
        {
            0x80000018 /* OffsetToData */
        }
    },
    /* file_info_header */
    {
        0, /* Characteristics */
        0x55FE8E21, /* TimeDateStamp, 20/09/2015 10:44:49 */
        0, /* MajorVersion */
        0, /* MinorVersion */
        0, /* NumberOfNamedEntries */
        1, /* NumberOfIdEntries */
    },
    /* version_info_id */
    {
        {{
            VS_VERSION_INFO, /* NameOffset:31 */
            0 /* NameIsString:1 */
        }},
        {
            0x80000030 /* OffsetToData */
        },
    },
    /* version_info_header */
    {
        0, /* Characteristics */
        0x55FE8E21, /* TimeDateStamp, 20/09/2015 10:44:49 */
        0, /* MajorVersion */
        0, /* MinorVersion */
        0, /* NumberOfNamedEntries */
        1, /* NumberOfIdEntries */
    },
    /* version_lang_id */
    {
        {{
            1033, /* NameOffset:31 */
            0 /* NameIsString:1 */
        }},
        {
            0x48 /* OffsetToDirectory */
        }
    },
    /* version_data_entry */
    {
        0x2058, /* OffsetToData */
        0x30C, /* Size */
        0, /* CodePage */
        0, /* Reserved */
    },

    /* version_info */
    {
    0x30C, /* wLength */
    0x34, /* wValueLength */
    0, /* wType: Binary */
    { 'V','S','_','V','E','R','S','I','O','N','_','I','N','F','O','\0','\0' }, /* szKey[17] */
        /* Value */
        {
            0xFEEF04BD, /* dwSignature */
            0x10000, /* dwStrucVersion */
            0x10000, /* dwFileVersionMS */
            0, /* dwFileVersionLS */
            0x10000, /* dwProductVersionMS */
            1, /* dwProductVersionLS */
            0, /* dwFileFlagsMask */
            0, /* dwFileFlags */
            VOS__WINDOWS32, /* dwFileOS */
            VFT_APP, /* dwFileType */
            0, /* dwFileSubtype */
            0x01d1a019, /* dwFileDateMS */
            0xac754c50 /* dwFileDateLS */
        },
    },

    /* string_file_info */
    {
        0x26C, /* wLength */
        0, /* wValueLength */
        1, /* wType: Text */
        { 'S','t','r','i','n','g','F','i','l','e','I','n','f','o','\0' } /* szKey[15] */
    },
    /* string_table */
    {
        0x248, /* wLength */
        0, /* wValueLength */
        1, /* wType: Text */
        { 'F','F','F','F','0','0','0','0','\0' } /* szKey[9] */
    },

    /* FileVersion */
    {
        48, /* wLength */
        8, /* wValueLength */
        1, /* wType: Text */
    },
    { 'F','i','l','e','V','e','r','s','i','o','n','\0' },
    { '1','.','0','.','0','.','0','\0' },

    /* ProductVersion */
    {
        52, /* wLength */
        8, /* wValueLength */
        1, /* wType: Text */
    },
    { 'P','r','o','d','u','c','t','V','e','r','s','i','o','n','\0' },
    { '1','.','0','.','0','.','1','\0' },

    /* CompanyName */
    {
        56, /* wLength */
        12, /* wValueLength */
        1, /* wType: Text */
    },
    { 'C','o','m','p','a','n','y','N','a','m','e','\0' },
    { 'C','o','m','p','a','n','y','N','a','m','e','\0' },

    /* FileDescription */
    {
        72, /* wLength */
        16, /* wValueLength */
        1, /* wType: Text */
    },
    { 'F','i','l','e','D','e','s','c','r','i','p','t','i','o','n','\0' },
    { 'F','i','l','e','D','e','s','c','r','i','p','t','i','o','n','\0' },

    /* InternalName */
    {
        58, /* wLength */
        13, /* wValueLength */
        1, /* wType: Text */
    },
    { 'I','n','t','e','r','n','a','l','N','a','m','e','\0' },
    { 'I','n','t','e','r','n','a','l','N','a','m','e','\0' },

    /* LegalCopyright */
    {
        66, /* wLength */
        15, /* wValueLength */
        1, /* wType: Text */
    },
    { 'L','e','g','a','l','C','o','p','y','r','i','g','h','t','\0' },
    { 'L','e','g','a','l','C','o','p','y','r','i','g','h','t','\0' },

    /* LegalTrademarks */
    {
        72, /* wLength */
        16, /* wValueLength */
        1, /* wType: Text */
    },
    { 'L','e','g','a','l','T','r','a','d','e','m','a','r','k','s','\0' },
    { 'L','e','g','a','l','T','r','a','d','e','m','a','r','k','s','\0' },

    /* OriginalFilename */
    {
        74, /* wLength */
        17, /* wValueLength */
        1, /* wType: Text */
    },
    { 'O','r','i','g','i','n','a','l','F','i','l','e','n','a','m','e','\0' },
    { 'O','r','i','g','i','n','a','l','F','i','l','e','n','a','m','e','\0' },

    /* ProductName */
    {
        56, /* wLength */
        12, /* wValueLength */
        1, /* wType: Text */
    },
    { 'P','r','o','d','u','c','t','N','a','m','e','\0' },
    { 'P','r','o','d','u','c','t','N','a','m','e','\0' },


    /* file_info */
    {
        0x44, /* wLength */
        0, /* wValueLength */
        1, /* wType: Text */
        { 'V','a','r','F','i','l','e','I','n','f','o','\0' } /* szKey[13] */
    },

    /* translation */
    {
        0x24, /* wLength */
        4, /* wValueLength */
        0, /* wType: Binary */
        { 'T','r','a','n','s','l','a','t','i','o','n','\0' }, /* szKey[13] */
        0xffff /* Value */
    }
};

typedef struct export_section_t
{
    IMAGE_EXPORT_DIRECTORY desc;
    char binary_name[10];
} export_section_t;

/* This export section is not complete, but the Name RVA is only taken into account */
static export_section_t export_dir = 
{
    {
        0, /* Characteristics */
        0, /* TimeDateStamp */
        0, /* MajorVersion */
        0, /* MinorVersion */
        0x2398, /* Name (RVA) */
        1, /* Base */
        0, /* NumberOfFunctions */
        0, /* NumberOfNames */
        0, /* AddressOfFunctions (RVA)  */
        0, /* AddressOfNames (RVA) */
        0, /* AddressOfNameOrdinals (RVA) */
    },
    { 'T','e','S','t','2','.','e','x','e',0 }, /* binary_name */
};


void test_create_exe_imp(const char* name, int skip_rsrc_exports)
{
    HANDLE file;
    char *buf, *cur;
    DWORD size = 0x800;
    buf = malloc(size);

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    winetest_ok(file != INVALID_HANDLE_VALUE, "can't create file\n");
    if(file == INVALID_HANDLE_VALUE)
        return;

    memset(buf, 0, size);
    cur = buf;
    cur = memcpy(buf, &dos_header, sizeof(dos_header));
    cur += dos_header.e_lfanew;

    memcpy(cur, &nt_header, sizeof(nt_header));
    if (skip_rsrc_exports)
    {
        ((IMAGE_NT_HEADERS32*)cur)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = 0;
        ((IMAGE_NT_HEADERS32*)cur)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = 0;
    }
    cur += sizeof(nt_header);
    memcpy(cur, section_headers, sizeof(section_headers));

    /* write code section: */
    cur = buf + section_headers[0].PointerToRawData;
    memcpy(cur, text_section, sizeof(text_section));

    if (!skip_rsrc_exports)
    {
        /* write resource section: */
        cur = buf + section_headers[1].PointerToRawData;
        memcpy(cur, &rsrc_section, sizeof(rsrc_section));

        /* write minimal export directory: */
        cur += 0x370;
        memcpy(cur, &export_dir, sizeof(export_dir));
    }

    WriteFile(file, buf, size, &size, NULL);
    free(buf);
    CloseHandle(file);
}


/* Almost everything in this filetype is ignored, only e_lfanew, ne_restab and ne_nrestab are relevant */
void test_create_ne_imp(const char* name, int skip_names)
{
    HANDLE file;
    DWORD size;
    IMAGE_DOS_HEADER MZ_hdr = { IMAGE_DOS_SIGNATURE, 0 };
    IMAGE_OS2_HEADER NE_hdr = { IMAGE_OS2_SIGNATURE, 0 };
    static const BYTE NE_names[] =
    {
        /* Show that the length is used, not the nullterm*/
        11,'T','E','S','T','M','O','D','.','h','X','x','x',0,0,0,
        20,'M','O','D',' ','D','E','S','C','R','I','P','T','I','O','N',' ','H','E','R','E',0,0,0
    };

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    winetest_ok(file != INVALID_HANDLE_VALUE, "can't create file\n");
    if(file == INVALID_HANDLE_VALUE)
        return;

    MZ_hdr.e_lfanew = sizeof(MZ_hdr);
    if (!skip_names)
    {
        NE_hdr.ne_restab = sizeof(NE_hdr);  /* First entry (pascal string + ordinal) = module name */
        NE_hdr.ne_nrestab = sizeof(MZ_hdr) + sizeof(NE_hdr) + 16;  /* First entry (pascal string + ordinal) = module description */
    }

    WriteFile(file, &MZ_hdr, sizeof(MZ_hdr), &size, NULL);
    WriteFile(file, &NE_hdr, sizeof(NE_hdr), &size, NULL);
    WriteFile(file, NE_names, sizeof(NE_names), &size, NULL);

    CloseHandle(file);
}

void test_create_file_imp(const char* name, const char* contents, size_t len)
{
    HANDLE file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    winetest_ok(file != INVALID_HANDLE_VALUE, "can't create file\n");
    if (file != INVALID_HANDLE_VALUE)
    {
        if (contents && len)
        {
            DWORD size;
            WriteFile(file, contents, len, &size, NULL);
        }
        CloseHandle(file);
    }
}

