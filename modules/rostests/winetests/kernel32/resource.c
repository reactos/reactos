/*
 * Unit test suite for resource functions.
 *
 * Copyright 2006 Mike McCormack
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

#include <windows.h>
#include <stdio.h>

#include "wine/test.h"

static const char filename[] = "test_.exe";
static const WCHAR filenameW[] = {'t','e','s','t','_','.','e','x','e',0};
static DWORD GLE;

enum constants {
    page_size = 0x1000,
    rva_rsrc_start = page_size * 3,
    max_sections = 3
};

/* rodata @ [0x1000-0x3000) */
static const IMAGE_SECTION_HEADER sh_rodata_1 =
{
    ".rodata", {2*page_size}, page_size, 2*page_size, page_size, 0, 0, 0, 0,
    IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ
};

/* rodata @ [0x1000-0x2000) */
static const IMAGE_SECTION_HEADER sh_rodata_2 =
{
    ".rodata", {page_size}, page_size, page_size, page_size, 0, 0, 0, 0,
    IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ
};

/* rsrc @ [0x3000-0x4000) */
static const IMAGE_SECTION_HEADER sh_rsrc_1 =
{
    ".rsrc\0\0", {page_size}, rva_rsrc_start, page_size, rva_rsrc_start, 0, 0, 0, 0,
    IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ
};

/* rsrc @ [0x2000-0x4000) */
static const IMAGE_SECTION_HEADER sh_rsrc_2 =
{
    ".rsrc\0\0", {2*page_size}, rva_rsrc_start-page_size, 2*page_size, rva_rsrc_start-page_size, 0, 0, 0, 0,
    IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ
};

/* rsrc @ [0x2000-0x3000) */
static const IMAGE_SECTION_HEADER sh_rsrc_3 =
{
    ".rsrc\0\0", {page_size}, rva_rsrc_start-page_size, page_size, rva_rsrc_start-page_size, 0, 0, 0, 0,
    IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ
};

/* rsrc @ [0x3000-0x6000) */
static const IMAGE_SECTION_HEADER sh_rsrc_4 =
{
    ".rsrc\0\0", {3*page_size}, rva_rsrc_start, 3*page_size, rva_rsrc_start, 0, 0, 0, 0,
    IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ
};

/* rsrc @ [0x2000-0x5000) */
static const IMAGE_SECTION_HEADER sh_rsrc_5 =
{
    ".rsrc\0\0", {3*page_size}, 2*page_size, 3*page_size, 2*page_size, 0, 0, 0, 0,
    IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ
};

/* rsrc @ [0x3000-0x4000), small SizeOfRawData */
static const IMAGE_SECTION_HEADER sh_rsrc_6 =
{
    ".rsrc\0\0", {page_size}, rva_rsrc_start, 8, rva_rsrc_start, 0, 0, 0, 0,
    IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ
};

/* reloc @ [0x4000-0x5000) */
static const IMAGE_SECTION_HEADER sh_junk =
{
    ".reloc\0", {page_size}, 4*page_size, page_size, 4*page_size, 0, 0, 0, 0,
    IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ
};

/* reloc @ [0x6000-0x7000) */
static const IMAGE_SECTION_HEADER sh_junk_2 =
{
    ".reloc\0", {page_size}, 6*page_size, page_size, 6*page_size, 0, 0, 0, 0,
    IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ
};

typedef struct _sec_build
{
    const IMAGE_SECTION_HEADER *sect_in[max_sections];
} sec_build;

typedef struct _sec_verify
{
    const IMAGE_SECTION_HEADER *sect_out[max_sections];
    DWORD length;
    int rsrc_section;
    DWORD NumberOfNamedEntries, NumberOfIdEntries;
} sec_verify;

static const struct _sec_variants
{
    sec_build build;
    sec_verify chk_none, chk_delete, chk_version, chk_bigdata;
} sec_variants[] =
{
    /* .rsrc is the last section, data directory entry points to whole section */
    {
        {{&sh_rodata_1, &sh_rsrc_1, NULL}},
        {{&sh_rodata_1, &sh_rsrc_1, NULL}, 4*page_size, 1, 0, 0},
        {{&sh_rodata_1, &sh_rsrc_1, NULL}, 4*page_size, 1, 0, 0},
        {{&sh_rodata_1, &sh_rsrc_1, NULL}, 4*page_size, 1, 0, 1},
        {{&sh_rodata_1, &sh_rsrc_4, NULL}, 6*page_size, 1, 0, 1}
    },
    /* .rsrc is the last section, data directory entry points to section end */
    /* Vista+ - resources are moved to section start (trashing data that could be there), and section is trimmed */
    /* NT4/2000/2003 - resources are moved to section start (trashing data that could be there); section isn't trimmed */
    {
        {{&sh_rodata_2, &sh_rsrc_2, NULL}},
        {{&sh_rodata_2, &sh_rsrc_3, NULL}, 3*page_size, 1, 0, 0},
        {{&sh_rodata_2, &sh_rsrc_3, NULL}, 3*page_size, 1, 0, 0},
        {{&sh_rodata_2, &sh_rsrc_3, NULL}, 3*page_size, 1, 0, 1},
        {{&sh_rodata_2, &sh_rsrc_5, NULL}, 5*page_size, 1, 0, 1}
    },
    /* .rsrc is not the last section */
    /* section is reused; sections after .rsrc are shifted to give space to rsrc (in-image offset and RVA!) */
    {
        {{&sh_rodata_1, &sh_rsrc_1, &sh_junk}},
        {{&sh_rodata_1, &sh_rsrc_1, &sh_junk}, 5*page_size, 1, 0, 0},
        {{&sh_rodata_1, &sh_rsrc_1, &sh_junk}, 5*page_size, 1, 0, 0},
        {{&sh_rodata_1, &sh_rsrc_1, &sh_junk}, 5*page_size, 1, 0, 1},
        {{&sh_rodata_1, &sh_rsrc_4, &sh_junk_2}, 7*page_size, 1, 0, 1}
    },
    /* .rsrc is the last section, data directory entry points to whole section, file size is not aligned on FileAlign */
    {
        {{&sh_rodata_1, &sh_rsrc_6, NULL}},
        {{&sh_rodata_1, &sh_rsrc_1, NULL}, 4*page_size, 1, 0, 0},
        {{&sh_rodata_1, &sh_rsrc_1, NULL}, 4*page_size, 1, 0, 0},
        {{&sh_rodata_1, &sh_rsrc_1, NULL}, 4*page_size, 1, 0, 1},
        {{&sh_rodata_1, &sh_rsrc_4, NULL}, 6*page_size, 1, 0, 1}
    }
};

static int build_exe( const sec_build* sec_descr )
{
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sec;
    IMAGE_OPTIONAL_HEADER *opt;
    HANDLE file;
    DWORD written, i, file_size;
    BYTE page[page_size];

    memset( page, 0, sizeof page );

    dos = (void*) page;
    dos->e_magic = IMAGE_DOS_SIGNATURE;
    dos->e_lfanew = sizeof *dos;

    nt = (void*) &dos[1];

    nt->Signature = IMAGE_NT_SIGNATURE;
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
    nt->FileHeader.NumberOfSections = 0;
    nt->FileHeader.SizeOfOptionalHeader = sizeof nt->OptionalHeader;
    nt->FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL;

    opt = &nt->OptionalHeader;

    opt->Magic = IMAGE_NT_OPTIONAL_HDR_MAGIC;
    opt->MajorLinkerVersion = 1;
    opt->BaseOfCode = 0x10;
    opt->ImageBase = 0x10000000;
    opt->MajorOperatingSystemVersion = 4;
    opt->MajorImageVersion = 1;
    opt->MajorSubsystemVersion = 4;
    opt->SizeOfHeaders = sizeof *dos + sizeof *nt + sizeof *sec * 2;
    opt->SizeOfImage = page_size;
    opt->Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;

    /* if SectionAlignment and File alignment are not specified */
    /* UpdateResource fails trying to create a huge temporary file */
    opt->SectionAlignment = page_size;
    opt->FileAlignment = page_size;

    opt->NumberOfRvaAndSizes = IMAGE_FILE_RESOURCE_DIRECTORY + 1;
    opt->DataDirectory[IMAGE_FILE_RESOURCE_DIRECTORY].VirtualAddress = rva_rsrc_start;
    opt->DataDirectory[IMAGE_FILE_RESOURCE_DIRECTORY].Size = page_size;

    sec = (void*) &nt[1];

    file_size = 0;
    for ( i = 0; i < max_sections; i++ )
        if ( sec_descr->sect_in[i] )
        {
            DWORD virt_end_of_section = sec_descr->sect_in[i]->Misc.VirtualSize +
                sec_descr->sect_in[i]->VirtualAddress;
            DWORD phys_end_of_section = sec_descr->sect_in[i]->SizeOfRawData +
                sec_descr->sect_in[i]->PointerToRawData;
            memcpy( sec + nt->FileHeader.NumberOfSections, sec_descr->sect_in[i],
                    sizeof(sec[0]) );
            nt->FileHeader.NumberOfSections++;
            if ( opt->SizeOfImage < virt_end_of_section )
                opt->SizeOfImage = virt_end_of_section;
            if ( file_size < phys_end_of_section )
                file_size = phys_end_of_section;
        }

    file = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok (file != INVALID_HANDLE_VALUE, "failed to create file\n");

    /* write out the header */
    WriteFile( file, page, sizeof page, &written, NULL );

    /* write out zeroed pages for sections */
    memset( page, 0, sizeof page );
    for ( i = page_size; i < file_size; i += page_size )
    {
	DWORD size = min(page_size, file_size - i);
        WriteFile( file, page, size, &written, NULL );
    }

    CloseHandle( file );

    return 0;
}

static void update_missing_exe( void )
{
    HANDLE res;

    SetLastError(0xdeadbeef);
    res = BeginUpdateResourceA( filename, TRUE );
    GLE = GetLastError();
    ok( res == NULL, "BeginUpdateResource should fail\n");
}

static void update_empty_exe( void )
{
    HANDLE file, res, test;
    BOOL r;

    file = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok (file != INVALID_HANDLE_VALUE, "failed to create file\n");

    CloseHandle( file );

    res = BeginUpdateResourceA( filename, TRUE );
    if ( res != NULL || GetLastError() != ERROR_FILE_INVALID )
    {
        ok( res != NULL, "BeginUpdateResource failed\n");

        /* check if it's possible to open the file now */
        test = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
        ok (test != INVALID_HANDLE_VALUE, "failed to create file\n");

        CloseHandle( test );

        r = EndUpdateResourceA( res, FALSE );
        ok( r == FALSE, "EndUpdateResource failed\n");
    }
    else
        skip( "Can't update resource in empty file\n" );

    res = BeginUpdateResourceA( filename, FALSE );
    ok( res == NULL, "BeginUpdateResource failed\n");
}

static void update_resources_none( void )
{
    HMODULE res;
    BOOL r;

    res = BeginUpdateResourceA( filename, FALSE );
    ok( res != NULL, "BeginUpdateResource failed\n");

    r = EndUpdateResourceA( res, FALSE );
    ok( r, "EndUpdateResource failed\n");
}

static void update_resources_delete( void )
{
    HMODULE res;
    BOOL r;

    res = BeginUpdateResourceA( filename, TRUE );
    ok( res != NULL, "BeginUpdateResource failed\n");

    r = EndUpdateResourceA( res, FALSE );
    ok( r, "EndUpdateResource failed\n");
}

static void update_resources_version( void )
{
    HANDLE res = NULL;
    BOOL r;
    char foo[] = "red and white";

    res = BeginUpdateResourceA( filename, TRUE );
    ok( res != NULL, "BeginUpdateResource failed\n");

    if (0)  /* this causes subsequent tests to fail on Vista */
    {
        r = UpdateResourceA( res,
                            MAKEINTRESOURCEA(0x1230),
                            MAKEINTRESOURCEA(0x4567),
                            0xabcd,
                            NULL, 0 );
        ok( r == FALSE, "UpdateResource failed\n");
    }

    r = UpdateResourceA( res,
                        MAKEINTRESOURCEA(0x1230),
                        MAKEINTRESOURCEA(0x4567),
                        0xabcd,
                        foo, sizeof foo );
    ok( r == TRUE, "UpdateResource failed: %ld\n", GetLastError());

    r = EndUpdateResourceA( res, FALSE );
    ok( r, "EndUpdateResource failed: %ld\n", GetLastError());
}

static void update_resources_bigdata( void )
{
    HANDLE res = NULL;
    BOOL r;
    char foo[2*page_size] = "foobar";

    res = BeginUpdateResourceA( filename, TRUE );
    ok( res != NULL, "BeginUpdateResource succeeded\n");

    r = UpdateResourceA( res,
                        MAKEINTRESOURCEA(0x3012),
                        MAKEINTRESOURCEA(0x5647),
                        0xcdba,
                        foo, sizeof foo );
    ok( r == TRUE, "UpdateResource failed: %ld\n", GetLastError());

    r = EndUpdateResourceA( res, FALSE );
    ok( r, "EndUpdateResource failed\n");
}

static void update_resources_name( void )
{
    char foo[] = "resource data", res_name[] = "name", res_type[] = "type";
    HANDLE res = NULL;
    HMODULE module;
    HRSRC rsrc;
    BOOL ret;

    res = BeginUpdateResourceA( filename, TRUE );
    ok( res != NULL, "BeginUpdateResource failed: %lu\n", GetLastError() );
    if ( !res ) return;

    ret = UpdateResourceA( res, res_type, res_name, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), foo, sizeof(foo) );
    ok( ret == TRUE, "UpdateResource failed: %lu\n", GetLastError() );

    ret = EndUpdateResourceA( res, FALSE );
    ok( ret, "EndUpdateResource failed: %lu\n", GetLastError() );
    if ( !ret ) return;

    module = LoadLibraryExA( filename, NULL, LOAD_LIBRARY_AS_DATAFILE );
    ok( module != NULL, "LoadLibraryEx failed: %lu\n", GetLastError() );
    if ( !module ) return;

    rsrc = FindResourceA( module, res_name, res_type );
    ok( rsrc != NULL ||
        broken( GetLastError() == ERROR_RESOURCE_TYPE_NOT_FOUND ) /* win2008 */,
        "FindResource failed: %lu\n", GetLastError() );

    ret = FreeLibrary(module);
    ok( ret, "FreeLibrary failed: %lu\n", GetLastError() );
}

static void check_exe( const sec_verify *verify )
{
    int i;
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_OPTIONAL_HEADER *opt;
    IMAGE_SECTION_HEADER *sec;
    IMAGE_RESOURCE_DIRECTORY *dir;
    HANDLE file, mapping;
    DWORD length, sec_count = 0;

    file = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok (file != INVALID_HANDLE_VALUE, "failed to create file (%ld)\n", GetLastError());

    length = GetFileSize( file, NULL );
    ok( length >= verify->length, "file size wrong\n");

    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 0, NULL );
    ok (mapping != NULL, "failed to create file\n");

    dos = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, length );
    ok( dos != NULL, "failed to map file\n");

    if (!dos)
        goto end;

    nt = (void*) ((BYTE*) dos + dos->e_lfanew);
    opt = &nt->OptionalHeader;
    sec = (void*) &nt[1];

    for(i = 0; i < max_sections; i++)
        if (verify->sect_out[i])
        {
            ok( !memcmp(&verify->sect_out[i]->Name, &sec[sec_count].Name, 8), "section %ld name wrong\n", sec_count);
            ok( verify->sect_out[i]->VirtualAddress == sec[sec_count].VirtualAddress, "section %ld vaddr wrong\n", sec_count);
            ok( verify->sect_out[i]->SizeOfRawData <= sec[sec_count].SizeOfRawData, "section %ld SizeOfRawData wrong (%ld vs %ld)\n", sec_count, verify->sect_out[i]->SizeOfRawData ,sec[sec_count].SizeOfRawData);
            ok( verify->sect_out[i]->PointerToRawData == sec[sec_count].PointerToRawData, "section %ld PointerToRawData wrong\n", sec_count);
            ok( verify->sect_out[i]->Characteristics == sec[sec_count].Characteristics , "section %ld characteristics wrong\n", sec_count);
            sec_count++;
        }

    ok( nt->FileHeader.NumberOfSections == sec_count, "number of sections wrong\n" );

    if (verify->rsrc_section >= 0 && verify->rsrc_section < nt->FileHeader.NumberOfSections)
    {
        dir = (void*) ((BYTE*) dos + sec[verify->rsrc_section].VirtualAddress);

        ok( dir->Characteristics == 0, "Characteristics wrong\n");
        ok( dir->TimeDateStamp == 0, "TimeDateStamp wrong %lu\n", dir->TimeDateStamp);
        ok( dir->MajorVersion == 4, "MajorVersion wrong\n");
        ok( dir->MinorVersion == 0, "MinorVersion wrong\n");

        ok( dir->NumberOfNamedEntries == verify->NumberOfNamedEntries, "NumberOfNamedEntries should be %ld instead of %d\n",
                verify->NumberOfNamedEntries, dir->NumberOfNamedEntries);
        ok( dir->NumberOfIdEntries == verify->NumberOfIdEntries, "NumberOfIdEntries should be %ld instead of %d\n",
                verify->NumberOfIdEntries, dir->NumberOfIdEntries);

        ok(opt->DataDirectory[IMAGE_FILE_RESOURCE_DIRECTORY].VirtualAddress == sec[verify->rsrc_section].VirtualAddress,
                "VirtualAddress in optional header should be %ld instead of %ld\n",
                sec[verify->rsrc_section].VirtualAddress, opt->DataDirectory[IMAGE_FILE_RESOURCE_DIRECTORY].VirtualAddress);
    }

end:
    UnmapViewOfFile( dos );

    CloseHandle( mapping );

    CloseHandle( file );
}

static void test_find_resource(void)
{
    HRSRC rsrc;

    rsrc = FindResourceW( GetModuleHandleW(NULL), MAKEINTRESOURCEW(1), (LPCWSTR)RT_MENU );
    ok( rsrc != 0, "resource not found\n" );
    rsrc = FindResourceExW( GetModuleHandleW(NULL), (LPCWSTR)RT_MENU, MAKEINTRESOURCEW(1),
                            MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ));
    ok( rsrc != 0, "resource not found\n" );
    rsrc = FindResourceExW( GetModuleHandleW(NULL), (LPCWSTR)RT_MENU, MAKEINTRESOURCEW(1),
                            MAKELANGID( LANG_GERMAN, SUBLANG_DEFAULT ));
    ok( rsrc != 0, "resource not found\n" );

    SetLastError( 0xdeadbeef );
    rsrc = FindResourceW( GetModuleHandleW(NULL), MAKEINTRESOURCEW(1), (LPCWSTR)RT_DIALOG );
    ok( !rsrc, "resource found\n" );
    ok( GetLastError() == ERROR_RESOURCE_TYPE_NOT_FOUND, "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    rsrc = FindResourceW( GetModuleHandleW(NULL), MAKEINTRESOURCEW(2), (LPCWSTR)RT_MENU );
    ok( !rsrc, "resource found\n" );
    ok( GetLastError() == ERROR_RESOURCE_NAME_NOT_FOUND, "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    rsrc = FindResourceExW( GetModuleHandleW(NULL), (LPCWSTR)RT_MENU, MAKEINTRESOURCEW(1),
                            MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ) );
    ok( !rsrc, "resource found\n" );
    ok( GetLastError() == ERROR_RESOURCE_LANG_NOT_FOUND, "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    rsrc = FindResourceExW( GetModuleHandleW(NULL), (LPCWSTR)RT_MENU, MAKEINTRESOURCEW(1),
                            MAKELANGID( LANG_FRENCH, SUBLANG_DEFAULT ) );
    ok( !rsrc, "resource found\n" );
    ok( GetLastError() == ERROR_RESOURCE_LANG_NOT_FOUND, "wrong error %lu\n", GetLastError() );
}

typedef struct
{
    void *unknown[6];
    HGLOBAL pFileName;
} QUEUEDUPDATES;

static void test_internal_structure(void)
{
    HANDLE res;
    QUEUEDUPDATES *res_data;
    WCHAR *res_filenameW;

    res = BeginUpdateResourceW( filenameW, FALSE );
    ok( res != NULL, "BeginUpdateResourceW failed\n" );
    res_data = GlobalLock(res);
    ok( res_data != NULL, "GlobalLock failed\n" );
    res_filenameW = GlobalLock( res_data->pFileName );
    ok( res_filenameW != NULL, "GlobalLock for res_filenameW failed\n" );
    ok( !lstrcmpW( res_filenameW, filenameW ), "Filename fields do not match\n" );
    ok( GlobalUnlock( res_filenameW ), "GlobalUnlock res_filenamed failed\n" );
    ok( GlobalUnlock( res_data ), "GlobalUnlock res_data failed\n" );
    ok( EndUpdateResourceW( res, TRUE ), "EndUpdateResourceW failed\n");
}

static const struct
{
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    IMAGE_SECTION_HEADER section;
} dll_image =
{
    { IMAGE_DOS_SIGNATURE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, 0, 0, { 0 },
      sizeof(IMAGE_DOS_HEADER) },
    {
        IMAGE_NT_SIGNATURE, /* Signature */
        {
#if defined __i386__
            IMAGE_FILE_MACHINE_I386, /* Machine */
#elif defined __x86_64__
            IMAGE_FILE_MACHINE_AMD64, /* Machine */
#elif defined __arm__
            IMAGE_FILE_MACHINE_ARMNT, /* Machine */
#elif defined __aarch64__
            IMAGE_FILE_MACHINE_ARM64, /* Machine */
#else
# error You must specify the machine type
#endif
            1, /* NumberOfSections */
            0, /* TimeDateStamp */
            0, /* PointerToSymbolTable */
            0, /* NumberOfSymbols */
            sizeof(IMAGE_OPTIONAL_HEADER), /* SizeOfOptionalHeader */
            IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL /* Characteristics */
        },
        { IMAGE_NT_OPTIONAL_HDR_MAGIC, /* Magic */
          1, /* MajorLinkerVersion */
          0, /* MinorLinkerVersion */
          0, /* SizeOfCode */
          0, /* SizeOfInitializedData */
          0, /* SizeOfUninitializedData */
          0, /* AddressOfEntryPoint */
          0x1000, /* BaseOfCode */
#ifndef _WIN64
          0, /* BaseOfData */
#endif
          0x10000000, /* ImageBase */
          0x1000, /* SectionAlignment */
          0x1000, /* FileAlignment */
          4, /* MajorOperatingSystemVersion */
          0, /* MinorOperatingSystemVersion */
          1, /* MajorImageVersion */
          0, /* MinorImageVersion */
          4, /* MajorSubsystemVersion */
          0, /* MinorSubsystemVersion */
          0, /* Win32VersionValue */
          0x3000, /* SizeOfImage */
          sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS),
          0, /* CheckSum */
          IMAGE_SUBSYSTEM_WINDOWS_CUI, /* Subsystem */
          IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE | IMAGE_DLLCHARACTERISTICS_NX_COMPAT, /* DllCharacteristics */
          0, /* SizeOfStackReserve */
          0, /* SizeOfStackCommit */
          0, /* SizeOfHeapReserve */
          0, /* SizeOfHeapCommit */
          0, /* LoaderFlags */
          IMAGE_FILE_RESOURCE_DIRECTORY + 1, /* NumberOfRvaAndSizes */
          { { 0 }, { 0 }, { 0x1000, 0x1000 } } /* DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES] */
        }
    },
    { ".rsrc\0\0", { 0 }, 0x1000, 0x1000, 0, 0, 0, 0, 0,
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ }
};

#ifndef __REACTOS__
static void create_test_dll( const WCHAR *name )
{
    DWORD dummy;
    HANDLE handle = CreateFileW( name, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0 );

    ok( handle != INVALID_HANDLE_VALUE, "failed to create file err %lu\n", GetLastError() );
    WriteFile( handle, &dll_image, sizeof(dll_image), &dummy, NULL );
    SetFilePointer( handle, dll_image.nt.OptionalHeader.SizeOfImage, NULL, FILE_BEGIN );
    SetEndOfFile( handle );
    CloseHandle( handle );
}

static struct mui_res
{
    DWORD signature;
    DWORD size;
    DWORD version;
    DWORD path_type;
    DWORD file_type;
    DWORD system_attributes;
    DWORD fallback_location;
    BYTE service_checksum[16];
    BYTE checksum[16];
    DWORD unk1[2];
    DWORD mui_path_off;
    DWORD mui_path_size;
    DWORD unk2[2];
    DWORD ln_type_name_off;
    DWORD ln_type_name_size;
    DWORD ln_type_id_off;
    DWORD ln_type_id_size;
    DWORD mui_type_name_off;
    DWORD mui_type_name_size;
    DWORD mui_type_id_off;
    DWORD mui_type_id_size;
    DWORD lang_off;
    DWORD lang_size;
    DWORD fallback_lang_off;
    DWORD fallback_lang_size;
    WCHAR ln_type_names[8];
    DWORD ln_type_ids[1];
    WCHAR mui_type_names[8];
    DWORD mui_type_ids[1];
    WCHAR lang[8];
    WCHAR fallback_lang[8];
} ln_mui_res = {
    0xfecdfecd, sizeof(ln_mui_res), 0x10000, 0,
    MUI_FILETYPE_LANGUAGE_NEUTRAL_MAIN >> 1,
    0, 0, {'s','c'}, {'c'}, {0}, 0, 0, {0},
    offsetof(struct mui_res, ln_type_names), sizeof(L"MUI\0"),
    offsetof(struct mui_res, ln_type_ids), sizeof(ln_mui_res.ln_type_ids),
    offsetof(struct mui_res, mui_type_names), sizeof(L"MUI\0"),
    offsetof(struct mui_res, mui_type_ids), sizeof(ln_mui_res.mui_type_ids), 0, 0,
    offsetof(struct mui_res, fallback_lang), sizeof(L"en-US"),
    {'M','U','I',0,0}, {RT_CURSOR}, {'M','U','I',0,0}, {RT_STRING}, {0}, {'e','n','-','U','S',0},
}, en_mui_res = {
    0xfecdfecd, sizeof(ln_mui_res), 0x10000, 0,
    MUI_FILETYPE_LANGUAGE_NEUTRAL_MUI >> 1,
    0, 0, {'s','c'}, {'c'}, {0}, 0, 0, {0},
    offsetof(struct mui_res, ln_type_names), sizeof(L"MUI\0"),
    offsetof(struct mui_res, ln_type_ids), sizeof(ln_mui_res.ln_type_ids),
    0, 0, 0, 0, offsetof(struct mui_res, lang), sizeof(L"en-US"), 0, 0,
    {'M','U','I',0,0}, {RT_STRING}, {0}, {0}, {'e','n','-','U','S',0}
};

static void test_mui(void)
{
    static const WCHAR ln_dll[] = L"test_mui.dll";
    static const WCHAR en_dll[] = L"en-US\\test_mui.dll.mui";
    static const BYTE zeros[16] = { 0 };
    BYTE buf[1024];
    FILEMUIINFO *info = (FILEMUIINFO *)buf;
    const WCHAR *str;
    DWORD size, *id;
    HANDLE res;
    BOOL r;

    size = 0;
    r = GetFileMUIInfo( 0, ln_dll, NULL, &size);
    ok( !r, "GetFileMUIInfo succeeded\n" );
    ok( GetLastError() == ERROR_FILE_NOT_FOUND, "GetLastError() = %ld\n", GetLastError() );

    create_test_dll( ln_dll );
    CreateDirectoryW( L"en-US", NULL );
    create_test_dll( en_dll );

    size = 0;
    r = GetFileMUIInfo( 0, ln_dll, NULL, &size );
    ok( r, "GetFileMUIInfo failed: %ld\n", GetLastError() );
    ok( size == sizeof(*info), "unexpected size: %ld\n", size );

    memset( buf, 0xfe, sizeof(buf) );
    size = sizeof(buf);
    info->dwSize = sizeof(buf);
    info->dwVersion = 0;
    r = GetFileMUIInfo( 0, ln_dll, info, &size );
    ok( !r, "GetFileMUIInfo succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError() = %ld\n", GetLastError() );
    ok( !size, "size = %ld\n", size );

    size = sizeof(buf);
    info->dwVersion = MUI_FILEINFO_VERSION;
    r = GetFileMUIInfo( 0, ln_dll, info, &size );
    ok( r, "GetFileMUIInfo failed: %ld\n", GetLastError() );
    ok( info->dwSize == sizeof(buf), "dwSize = %ld\n", info->dwSize );
    ok( info->dwVersion == MUI_FILEINFO_VERSION, "dwVersion = %ld\n", info->dwVersion );
    ok( info->dwFileType == MUI_FILETYPE_NOT_LANGUAGE_NEUTRAL, "dwFileType = %ld\n", info->dwFileType );
    ok( !memcmp(info->pChecksum, zeros, sizeof(info->pChecksum)), "pChecksum = %s\n",
            wine_dbgstr_an((char *)info->pChecksum, sizeof(info->pChecksum)) );
    ok( !memcmp(info->pServiceChecksum, zeros, sizeof(info->pServiceChecksum)), "pServiceChecksum = %s\n",
            wine_dbgstr_an((char *)info->pServiceChecksum, sizeof(info->pServiceChecksum)) );
    ok( !info->dwLanguageNameOffset, "dwLanguageNameOffset = %ld\n", info->dwLanguageNameOffset );
    ok( !info->dwTypeIDMainSize, "dwTypeIDMainSize = %ld\n", info->dwTypeIDMainSize );
    ok( !info->dwTypeIDMainOffset, "dwTypeIDMainOffset = %ld\n", info->dwTypeIDMainOffset );
    ok( !info->dwTypeNameMainOffset, "dwTypeNameMainOffset = %ld\n", info->dwTypeNameMainOffset );
    ok( !info->dwTypeIDMUISize, "dwTypeIDMUISize = %ld\n", info->dwTypeIDMUISize );
    ok( !info->dwTypeIDMUIOffset, "dwTypeIDMUIOffset = %ld\n", info->dwTypeIDMUIOffset );
    ok( !info->dwTypeNameMUIOffset, "dwTypeNameMUIOffset = %ld\n", info->dwTypeNameMUIOffset );
    ok( !memcmp(info->abBuffer, zeros, sizeof(info->abBuffer)), "abBuffer = %s\n",
            wine_dbgstr_an((char *)info->abBuffer, sizeof(info->abBuffer)) );

    res = BeginUpdateResourceW( ln_dll, TRUE );
    ok( res != NULL, "BeginUpdateResourceW failed: %ld\n", GetLastError() );
    r = UpdateResourceW( res, L"MUI", MAKEINTRESOURCEW(1), 0, &ln_mui_res, 4 );
    ok( r, "UpdateResource failed: %ld\n", GetLastError() );
    r = EndUpdateResourceW( res, FALSE );
    ok( r, "EndUpdateResourceW failed: %ld\n", GetLastError() );

    size = 0;
    r = GetFileMUIInfo( MUI_QUERY_TYPE | MUI_QUERY_CHECKSUM | MUI_QUERY_LANGUAGE_NAME
            | MUI_QUERY_RESOURCE_TYPES, ln_dll, NULL, &size );
    ok( !r, "GetFileMUIInfo succeeded\n" );
    ok( GetLastError() == ERROR_BAD_EXE_FORMAT, "GetLastError() = %ld\n", GetLastError() );

    res = BeginUpdateResourceW( ln_dll, TRUE );
    ok( res != NULL, "BeginUpdateResourceW failed: %ld\n", GetLastError() );
    r = UpdateResourceW( res, L"MUI", MAKEINTRESOURCEW(1), 0, &ln_mui_res, sizeof(ln_mui_res) );
    ok( r, "UpdateResource failed: %ld\n", GetLastError() );
    r = EndUpdateResourceW( res, FALSE );
    ok( r, "EndUpdateResourceW failed: %ld\n", GetLastError() );

    size = 0;
    r = GetFileMUIInfo( MUI_QUERY_TYPE | MUI_QUERY_CHECKSUM | MUI_QUERY_LANGUAGE_NAME
            | MUI_QUERY_RESOURCE_TYPES, ln_dll, NULL, &size );
    ok( !r, "GetFileMUIInfo succeeded\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetLastError() = %ld\n", GetLastError() );
    ok( size, "size was not set\n" );

    memset( buf, 0xfe, sizeof(buf) );
    size = sizeof(buf);
    info->dwSize = sizeof(buf);
    info->dwVersion = MUI_FILEINFO_VERSION;
    r = GetFileMUIInfo( MUI_QUERY_TYPE | MUI_QUERY_CHECKSUM | MUI_QUERY_LANGUAGE_NAME
            | MUI_QUERY_RESOURCE_TYPES, ln_dll, info, &size );
    ok( r, "GetFileMUIInfo failed: %ld\n", GetLastError() );
    ok( info->dwSize == sizeof(buf), "dwSize = %ld\n", info->dwSize );
    ok( info->dwVersion == MUI_FILEINFO_VERSION, "dwVersion = %ld\n", info->dwVersion );
    ok( info->dwFileType == MUI_FILETYPE_LANGUAGE_NEUTRAL_MAIN, "dwFileType = %ld\n", info->dwFileType );
    ok( info->pChecksum[0] == 'c', "pChecksum = %s\n",
            wine_dbgstr_an((char *)info->pChecksum, sizeof(info->pChecksum)) );
    ok( info->pServiceChecksum[0] == 's', "pServiceChecksum = %s\n",
            wine_dbgstr_an((char *)info->pServiceChecksum, sizeof(info->pServiceChecksum)) );
    ok( info->dwLanguageNameOffset == 72, "dwLanguageNameOffset = %ld\n", info->dwLanguageNameOffset );
    str = (WCHAR *)(buf + info->dwLanguageNameOffset);
    ok( !wcscmp(str, L"en-US"), "language name = %s\n", wine_dbgstr_w(str) );
    ok( info->dwTypeIDMainSize == 1, "dwTypeIDMainSize = %ld\n", info->dwTypeIDMainSize );
    ok( info->dwTypeIDMainOffset == 84, "dwTypeIDMainOffset = %ld\n", info->dwTypeIDMainOffset );
    id = (DWORD *)(buf + info->dwTypeIDMainOffset);
    ok( id[0] == RT_CURSOR, "type ID main[0] = %ld\n", id[0] );
    ok( info->dwTypeNameMainOffset == 88, "dwTypeNameMainOffset = %ld\n", info->dwTypeNameMainOffset );
    str = (WCHAR *)(buf + info->dwTypeNameMainOffset);
    ok( !wcscmp(str, L"MUI"), "type name main[0] = %s\n", wine_dbgstr_w(str) );
    str += wcslen(str) + 1;
    ok( !str[0], "string list is not NULL terminated: %s\n", wine_dbgstr_w(str) );
    ok( info->dwTypeIDMUISize == 1, "dwTypeIDMUISize = %ld\n", info->dwTypeIDMUISize );
    ok( info->dwTypeIDMUIOffset == 98, "dwTypeIDMUIOffset = %ld\n", info->dwTypeIDMUIOffset );
    id = (DWORD *)(buf + info->dwTypeIDMUIOffset);
    ok( id[0] == RT_STRING, "type ID MUI[0] = %ld\n", id[0] );
    ok( info->dwTypeNameMUIOffset == 102, "dwTypeNameMUIOffset = %ld\n", info->dwTypeNameMUIOffset );
    str = (WCHAR *)(buf + info->dwTypeNameMUIOffset);
    ok( !wcscmp(str, L"MUI"), "type name MUI[0] = %s\n", wine_dbgstr_w(str) );
    str += wcslen(str) + 1;
    ok( !str[0], "string list is not NULL terminated: %s\n", wine_dbgstr_w(str) );

    res = BeginUpdateResourceW( en_dll, TRUE );
    ok( res != NULL, "BeginUpdateResourceW failed: %ld\n", GetLastError() );
    r = UpdateResourceW( res, L"MUI", MAKEINTRESOURCEW(1), 0, &en_mui_res, sizeof(en_mui_res) );
    ok( r, "UpdateResource failed: %ld\n", GetLastError() );
    r = EndUpdateResourceW( res, FALSE );
    ok( r, "EndUpdateResourceW failed: %ld\n", GetLastError() );

    memset( buf, 0xfe, sizeof(buf) );
    size = sizeof(buf);
    info->dwSize = sizeof(buf);
    info->dwVersion = MUI_FILEINFO_VERSION;
    r = GetFileMUIInfo( MUI_QUERY_TYPE | MUI_QUERY_CHECKSUM | MUI_QUERY_LANGUAGE_NAME
            | MUI_QUERY_RESOURCE_TYPES, en_dll, info, &size );
    ok( r, "GetFileMUIInfo failed: %ld\n", GetLastError() );
    ok( info->dwSize == sizeof(buf), "dwSize = %ld\n", info->dwSize );
    ok( info->dwVersion == MUI_FILEINFO_VERSION, "dwVersion = %ld\n", info->dwVersion );
    ok( info->dwFileType == MUI_FILETYPE_LANGUAGE_NEUTRAL_MUI, "dwFileType = %ld\n", info->dwFileType );
    ok( info->pChecksum[0] == 'c', "pChecksum = %s\n",
            wine_dbgstr_an((char *)info->pChecksum, sizeof(info->pChecksum)) );
    ok( info->pServiceChecksum[0] == 's', "pServiceChecksum = %s\n",
            wine_dbgstr_an((char *)info->pServiceChecksum, sizeof(info->pServiceChecksum)) );
    ok( info->dwLanguageNameOffset == 72, "dwLanguageNameOffset = %ld\n", info->dwLanguageNameOffset );
    str = (WCHAR *)(buf + info->dwLanguageNameOffset);
    ok( !wcscmp(str, L"en-US"), "language name = %s\n", wine_dbgstr_w(str) );
    ok( !info->dwTypeIDMainSize, "dwTypeIDMainSize = %ld\n", info->dwTypeIDMainSize );
    ok( !info->dwTypeIDMainOffset, "dwTypeIDMainOffset = %ld\n", info->dwTypeIDMainOffset );
    ok( !info->dwTypeNameMainOffset, "dwTypeNameMainOffset = %ld\n", info->dwTypeNameMainOffset );
    ok( info->dwTypeIDMUISize == 1, "dwTypeIDMUISize = %ld\n", info->dwTypeIDMUISize );
    ok( info->dwTypeIDMUIOffset == 84, "dwTypeIDMUIOffset = %ld\n", info->dwTypeIDMUIOffset );
    id = (DWORD *)(buf + info->dwTypeIDMUIOffset);
    ok( id[0] == RT_STRING, "type ID MUI[0] = %ld\n", id[0] );
    ok( info->dwTypeNameMUIOffset == 88, "dwTypeNameMUIOffset = %ld\n", info->dwTypeNameMUIOffset );
    str = (WCHAR *)(buf + info->dwTypeNameMUIOffset);
    ok( !wcscmp(str, L"MUI"), "type name MUI[0] = %s\n", wine_dbgstr_w(str) );
    str += wcslen(str) + 1;
    ok( !str[0], "string list is not NULL terminated: %s\n", wine_dbgstr_w(str) );

    DeleteFileW( ln_dll );
    DeleteFileW( en_dll );
    RemoveDirectoryW( L"en-US" );
}
#endif

START_TEST(resource)
{
    DWORD i;

    DeleteFileA( filename );
    update_missing_exe();

    if (GLE == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("Resource calls are not implemented\n");
        return;
    }

    update_empty_exe();

    for(i=0; i < ARRAY_SIZE(sec_variants); i++)
    {
        const struct _sec_variants *sec = &sec_variants[i];
        build_exe( &sec->build );
        test_internal_structure();
        update_resources_none();
        check_exe( &sec->chk_none );
        update_resources_delete();
        check_exe( &sec->chk_delete );
        update_resources_version();
        check_exe( &sec->chk_version );
        update_resources_bigdata();
        check_exe( &sec->chk_bigdata );
        update_resources_name();
        DeleteFileA( filename );
    }
    test_find_resource();
#ifndef __REACTOS__
    test_mui();
#endif
}
