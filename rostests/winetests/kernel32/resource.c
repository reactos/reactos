/*
 * Unit test suite for environment functions.
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
static DWORD GLE;

static int build_exe( void )
{
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sec;
    IMAGE_OPTIONAL_HEADER *opt;
    HANDLE file;
    DWORD written;
    BYTE page[0x1000];
    const int page_size = 0x1000;

    memset( page, 0, sizeof page );

    dos = (void*) page;
    dos->e_magic = IMAGE_DOS_SIGNATURE;
    dos->e_lfanew = sizeof *dos;

    nt = (void*) &dos[1];

    nt->Signature = IMAGE_NT_SIGNATURE;
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
    nt->FileHeader.NumberOfSections = 2;
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
    opt->SizeOfImage = page_size*3;
    opt->Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;

    /* if SectionAlignment and File alignment are not specified */
    /* UpdateResource fails trying to create a huge temporary file */
    opt->SectionAlignment = page_size;
    opt->FileAlignment = page_size;

    sec = (void*) &nt[1];

    memcpy( sec[0].Name, ".rodata", 8 );
    sec[0].Misc.VirtualSize = page_size;
    sec[0].PointerToRawData = page_size;
    sec[0].SizeOfRawData = page_size;
    sec[0].VirtualAddress = page_size;
    sec[0].Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;

    memcpy( sec[1].Name, ".rsrc", 6 );
    sec[1].Misc.VirtualSize = page_size;
    sec[1].SizeOfRawData = page_size;
    sec[1].PointerToRawData = page_size*2;
    sec[1].VirtualAddress = page_size*2;
    sec[1].Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;

    file = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok (file != INVALID_HANDLE_VALUE, "failed to create file\n");

    /* write out the header */
    WriteFile( file, page, sizeof page, &written, NULL );

    /* write out an empty page for rodata */
    memset( page, 0, sizeof page );
    WriteFile( file, page, sizeof page, &written, NULL );

    /* write out an empty page for the resources */
    memset( page, 0, sizeof page );
    WriteFile( file, page, sizeof page, &written, NULL );

    CloseHandle( file );

    return 0;
}

static void update_missing_exe( void )
{
    HANDLE res;

    SetLastError(0xdeadbeef);
    res = BeginUpdateResource( filename, TRUE );
    GLE = GetLastError();
    ok( res == NULL, "BeginUpdateResource should fail\n");
}

static void update_empty_exe( void )
{
    HANDLE file, res, test;
    BOOL r;

    file = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok (file != INVALID_HANDLE_VALUE, "failed to create file\n");

    CloseHandle( file );

    res = BeginUpdateResource( filename, TRUE );
    ok( res != NULL, "BeginUpdateResource failed\n");

    /* check if it's possible to open the file now */
    test = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok (test != INVALID_HANDLE_VALUE, "failed to create file\n");

    CloseHandle( test );

    r = EndUpdateResource( res, FALSE );
    ok( r == FALSE, "EndUpdateResource failed\n");

    res = BeginUpdateResource( filename, FALSE );
    ok( res == NULL, "BeginUpdateResource failed\n");
}

static void update_resources_none( void )
{
    HMODULE res;
    BOOL r;

    res = BeginUpdateResource( filename, FALSE );
    ok( res != NULL, "BeginUpdateResource failed\n");

    r = EndUpdateResource( res, FALSE );
    ok( r, "EndUpdateResource failed\n");
}

static void update_resources_delete( void )
{
    HMODULE res;
    BOOL r;

    res = BeginUpdateResource( filename, TRUE );
    ok( res != NULL, "BeginUpdateResource failed\n");

    r = EndUpdateResource( res, FALSE );
    ok( r, "EndUpdateResource failed\n");
}

static void update_resources_version(void)
{
    HANDLE res = NULL;
    BOOL r;
    char foo[] = "red and white";

    res = BeginUpdateResource( filename, TRUE );
    ok( res != NULL, "BeginUpdateResource failed\n");

    if (0)  /* this causes subsequent tests to fail on Vista */
    {
        r = UpdateResource( res,
                            MAKEINTRESOURCE(0x1230),
                            MAKEINTRESOURCE(0x4567),
                            0xabcd,
                            NULL, 0 );
        ok( r == FALSE, "UpdateResource failed\n");
    }

    r = UpdateResource( res,
                        MAKEINTRESOURCE(0x1230),
                        MAKEINTRESOURCE(0x4567),
                        0xabcd,
                        foo, sizeof foo );
    ok( r == TRUE, "UpdateResource failed: %d\n", GetLastError());

    r = EndUpdateResource( res, FALSE );
    ok( r, "EndUpdateResource failed: %d\n", GetLastError());
}


typedef void (*res_check_func)( IMAGE_RESOURCE_DIRECTORY* );

static void check_empty( IMAGE_RESOURCE_DIRECTORY *dir )
{
    char *pad;

    ok( dir->NumberOfNamedEntries == 0, "NumberOfNamedEntries should be 0 instead of %d\n", dir->NumberOfNamedEntries);
    ok( dir->NumberOfIdEntries == 0, "NumberOfIdEntries should be 0 instead of %d\n", dir->NumberOfIdEntries);

    pad = (char*) &dir[1];

    ok( !memcmp( pad, "PADDINGXXPADDING", 16), "padding wrong\n");
}

static void check_not_empty( IMAGE_RESOURCE_DIRECTORY *dir )
{
    ok( dir->NumberOfNamedEntries == 0, "NumberOfNamedEntries should be 0 instead of %d\n", dir->NumberOfNamedEntries);
    ok( dir->NumberOfIdEntries == 1, "NumberOfIdEntries should be 1 instead of %d\n", dir->NumberOfIdEntries);
}

static void check_exe( res_check_func fn )
{
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sec;
    IMAGE_RESOURCE_DIRECTORY *dir;
    HANDLE file, mapping;
    DWORD length;

    file = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok (file != INVALID_HANDLE_VALUE, "failed to create file (%d)\n", GetLastError());

    length = GetFileSize( file, NULL );
    ok( length == 0x3000, "file size wrong\n");

    mapping = CreateFileMapping( file, NULL, PAGE_READONLY, 0, 0, NULL );
    ok (mapping != NULL, "failed to create file\n");

    dos = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, length );
    ok( dos != NULL, "failed to map file\n");

    if (!dos)
        goto end;

    nt = (void*) ((BYTE*) dos + dos->e_lfanew);
    ok( nt->FileHeader.NumberOfSections == 2, "number of sections wrong\n" );

    if (nt->FileHeader.NumberOfSections < 2)
        goto end;

    sec = (void*) &nt[1];

    ok( !memcmp(sec[1].Name, ".rsrc", 6), "resource section name wrong\n");

    dir = (void*) ((BYTE*) dos + sec[1].VirtualAddress);

    ok( dir->Characteristics == 0, "Characteristics wrong\n");
    ok( dir->TimeDateStamp == 0 || abs( dir->TimeDateStamp - GetTickCount() ) < 1000 /* nt4 */,
        "TimeDateStamp wrong %u\n", dir->TimeDateStamp);
    ok( dir->MajorVersion == 4, "MajorVersion wrong\n");
    ok( dir->MinorVersion == 0, "MinorVersion wrong\n");

    fn( dir );

end:
    UnmapViewOfFile( dos );

    CloseHandle( mapping );

    CloseHandle( file );
}

static void test_find_resource(void)
{
    HRSRC rsrc;

    rsrc = FindResourceW( GetModuleHandle(0), (LPCWSTR)MAKEINTRESOURCE(1), (LPCWSTR)RT_MENU );
    ok( rsrc != 0, "resource not found\n" );
    rsrc = FindResourceExW( GetModuleHandle(0), (LPCWSTR)RT_MENU, (LPCWSTR)MAKEINTRESOURCE(1),
                            MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ));
    ok( rsrc != 0, "resource not found\n" );
    rsrc = FindResourceExW( GetModuleHandle(0), (LPCWSTR)RT_MENU, (LPCWSTR)MAKEINTRESOURCE(1),
                            MAKELANGID( LANG_GERMAN, SUBLANG_DEFAULT ));
    ok( rsrc != 0, "resource not found\n" );

    SetLastError( 0xdeadbeef );
    rsrc = FindResourceW( GetModuleHandle(0), (LPCWSTR)MAKEINTRESOURCE(1), (LPCWSTR)RT_DIALOG );
    ok( !rsrc, "resource found\n" );
    ok( GetLastError() == ERROR_RESOURCE_TYPE_NOT_FOUND, "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    rsrc = FindResourceW( GetModuleHandle(0), (LPCWSTR)MAKEINTRESOURCE(2), (LPCWSTR)RT_MENU );
    ok( !rsrc, "resource found\n" );
    ok( GetLastError() == ERROR_RESOURCE_NAME_NOT_FOUND, "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    rsrc = FindResourceExW( GetModuleHandle(0), (LPCWSTR)RT_MENU, (LPCWSTR)MAKEINTRESOURCE(1),
                            MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ) );
    ok( !rsrc, "resource found\n" );
    ok( GetLastError() == ERROR_RESOURCE_LANG_NOT_FOUND, "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    rsrc = FindResourceExW( GetModuleHandle(0), (LPCWSTR)RT_MENU, (LPCWSTR)MAKEINTRESOURCE(1),
                            MAKELANGID( LANG_FRENCH, SUBLANG_DEFAULT ) );
    ok( !rsrc, "resource found\n" );
    ok( GetLastError() == ERROR_RESOURCE_LANG_NOT_FOUND, "wrong error %u\n", GetLastError() );
}

START_TEST(resource)
{
    DeleteFile( filename );
    update_missing_exe();

    if (GLE == ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("Resource calls are not implemented\n");
        return;
    }

    update_empty_exe();
    build_exe();
    update_resources_none();
    check_exe( check_empty );
    update_resources_delete();
    check_exe( check_empty );
    update_resources_version();
    check_exe( check_not_empty );
    DeleteFile( filename );
    test_find_resource();
}
