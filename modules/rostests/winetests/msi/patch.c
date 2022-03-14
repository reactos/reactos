/*
 * Copyright 2010 Hans Leidekker for CodeWeavers
 *
 * A test program for patching MSI products.
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

#define _WIN32_MSI 300
#define COBJMACROS

#include <stdio.h>

#include <windows.h>
#include <msiquery.h>
#include <msidefs.h>
#include <msi.h>
#include <wtypes.h>

#include "wine/test.h"

static UINT (WINAPI *pMsiApplyPatchA)( LPCSTR, LPCSTR, INSTALLTYPE, LPCSTR );
static UINT (WINAPI *pMsiGetPatchInfoExA)( LPCSTR, LPCSTR, LPCSTR, MSIINSTALLCONTEXT,
                                           LPCSTR, LPSTR, DWORD * );
static UINT (WINAPI *pMsiEnumPatchesExA)( LPCSTR, LPCSTR, DWORD, DWORD, DWORD, LPSTR,
                                          LPSTR, MSIINSTALLCONTEXT *, LPSTR, LPDWORD );

static const char *msifile = "winetest-patch.msi";
static const char *mspfile = "winetest-patch.msp";
static const WCHAR msifileW[] = L"winetest-patch.msi";
static const WCHAR mspfileW[] = L"winetest-patch.msp";

static char CURR_DIR[MAX_PATH];
static char PROG_FILES_DIR[MAX_PATH];
static char COMMON_FILES_DIR[MAX_PATH];

/* msi database data */

static const char property_dat[] =
    "Property\tValue\n"
    "s72\tl0\n"
    "Property\tProperty\n"
    "Manufacturer\tWineHQ\n"
    "ProductCode\t{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}\n"
    "UpgradeCode\t{A2E3D643-4E2C-477F-A309-F76F552D5F43}\n"
    "ProductLanguage\t1033\n"
    "ProductName\tmsitest\n"
    "ProductVersion\t1.1.1\n"
    "PATCHNEWSUMMARYSUBJECT\tInstaller Database\n"
    "MSIFASTINSTALL\t1\n";

static const char media_dat[] =
    "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
    "i2\ti4\tL64\tS255\tS32\tS72\n"
    "Media\tDiskId\n"
    "1\t1\t\t\tDISK1\t\n";

static const char file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "patch.txt\tpatch\tpatch.txt\t1000\t\t\t0\t1\n"
    "disable.txt\tdisable\tdisable.txt\t1000\t\t\t0\t1\n";

static const char directory_dat[] =
    "Directory\tDirectory_Parent\tDefaultDir\n"
    "s72\tS72\tl255\n"
    "Directory\tDirectory\n"
    "MSITESTDIR\tProgramFilesFolder\tmsitest\n"
    "ProgramFilesFolder\tTARGETDIR\t.\n"
    "TARGETDIR\t\tSourceDir";

static const char component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "patch\t{4B79D87E-6D28-4FD3-92D6-CD9B26AF64F1}\tMSITESTDIR\t0\t\tpatch.txt\n"
    "disable\t{BDDBA0EE-0031-4591-ADC0-33308175AC19}\tMSITESTDIR\t0\t\tdisable.txt\n";

static const char feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "patch\t\t\tpatch feature\t1\t1\tMSITESTDIR\t0\n"
    "disable\t\t\tdisabled feature\t1\t1\tMSITESTDIR\t0\n";

static const char feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "patch\tpatch\n"
    "disable\tdisable\n";

static const char install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "LaunchConditions\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "RemoveFiles\t\t1700\n"
    "InstallFiles\t\t2000\n"
    "RegisterUser\t\t3000\n"
    "RegisterProduct\t\t3100\n"
    "PublishFeatures\t\t5100\n"
    "PublishProduct\t\t5200\n"
    "UnpublishFeatures\t\t5300\n"
    "InstallFinalize\t\t6000\n";

static const char condition_dat[] =
    "Feature_\tLevel\tCondition\n"
    "s38\ti2\tS255\n"
    "Condition\tFeature_\tLevel\n"
    "disable\t0\tDISABLE_FEATURE\n";

struct msi_table
{
    const char *filename;
    const char *data;
    int size;
};

#define ADD_TABLE( x ) { #x".idt", x##_dat, sizeof(x##_dat) }

static const struct msi_table tables[] =
{
    ADD_TABLE( directory ),
    ADD_TABLE( file ),
    ADD_TABLE( component ),
    ADD_TABLE( feature ),
    ADD_TABLE( feature_comp ),
    ADD_TABLE( property ),
    ADD_TABLE( install_exec_seq ),
    ADD_TABLE( media ),
    ADD_TABLE( condition )
};

static void init_function_pointers( void )
{
    HMODULE hmsi = GetModuleHandleA( "msi.dll" );

#define GET_PROC( mod, func ) \
    p ## func = (void *)GetProcAddress( mod, #func ); \
    if (!p ## func) \
        trace( "GetProcAddress(%s) failed\n", #func );

    GET_PROC( hmsi, MsiApplyPatchA );
    GET_PROC( hmsi, MsiGetPatchInfoExA );
    GET_PROC( hmsi, MsiEnumPatchesExA );

#undef GET_PROC
}

static BOOL is_process_limited(void)
{
    HANDLE token;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        BOOL ret;
        TOKEN_ELEVATION_TYPE type = TokenElevationTypeDefault;
        DWORD size;

        ret = GetTokenInformation(token, TokenElevationType, &type, sizeof(type), &size);
        CloseHandle(token);
        return (ret && type == TokenElevationTypeLimited);
    }
    return FALSE;
}

static BOOL get_program_files_dir( char *buf, char *buf2 )
{
    HKEY hkey;
    DWORD type, size;

    if (RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion", &hkey ))
        return FALSE;

    size = MAX_PATH;
    if (RegQueryValueExA( hkey, "ProgramFilesDir (x86)", 0, &type, (LPBYTE)buf, &size ) &&
        RegQueryValueExA( hkey, "ProgramFilesDir", 0, &type, (LPBYTE)buf, &size ))
    {
        RegCloseKey( hkey );
        return FALSE;
    }
    size = MAX_PATH;
    if (RegQueryValueExA( hkey, "CommonFilesDir", 0, &type, (LPBYTE)buf2, &size ))
    {
        RegCloseKey( hkey );
        return FALSE;
    }
    RegCloseKey( hkey );
    return TRUE;
}

static void create_file_data( const char *filename, const char *data, DWORD size )
{
    HANDLE file;
    DWORD written;

    file = CreateFileA( filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
    if (file == INVALID_HANDLE_VALUE)
        return;

    WriteFile( file, data, strlen( data ), &written, NULL );
    if (size)
    {
        SetFilePointer( file, size, NULL, FILE_BEGIN );
        SetEndOfFile( file );
    }
    CloseHandle( file );
}

#define create_file( name, size ) create_file_data( name, name, size )

static BOOL delete_pf( const char *rel_path, BOOL is_file )
{
    char path[MAX_PATH];

    strcpy( path, PROG_FILES_DIR );
    strcat( path, "\\" );
    strcat( path, rel_path );

    if (is_file)
        return DeleteFileA( path );
    else
        return RemoveDirectoryA( path );
}

static DWORD get_pf_file_size( const char *filename )
{
    char path[MAX_PATH];
    HANDLE file;
    DWORD size;

    strcpy( path, PROG_FILES_DIR );
    strcat( path, "\\");
    strcat( path, filename );

    file = CreateFileA( path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if (file == INVALID_HANDLE_VALUE)
        return INVALID_FILE_SIZE;

    size = GetFileSize( file, NULL );
    CloseHandle( file );
    return size;
}

static void write_file( const char *filename, const char *data, DWORD data_size )
{
    DWORD size;
    HANDLE file = CreateFileA( filename, GENERIC_WRITE, 0, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    WriteFile( file, data, data_size, &size, NULL );
    CloseHandle( file );
}

static void set_suminfo( const WCHAR *filename )
{
    UINT r;
    MSIHANDLE hsi, hdb;

    r = MsiOpenDatabaseW( filename, MSIDBOPEN_DIRECT, &hdb );
    ok( r == ERROR_SUCCESS, "failed to open database %u\n", r );

    r = MsiGetSummaryInformationA( hdb, NULL, 7, &hsi );
    ok( r == ERROR_SUCCESS, "failed to open summaryinfo %u\n", r );

    r = MsiSummaryInfoSetPropertyA( hsi, 2, VT_LPSTR, 0, NULL, "Installation Database" );
    ok( r == ERROR_SUCCESS, "failed to set summary info %u\n", r );

    r = MsiSummaryInfoSetPropertyA( hsi, 3, VT_LPSTR, 0, NULL, "Installation Database" );
    ok( r == ERROR_SUCCESS, "failed to set summary info %u\n", r );

    r = MsiSummaryInfoSetPropertyA( hsi, 4, VT_LPSTR, 0, NULL, "WineHQ" );
    ok( r == ERROR_SUCCESS, "failed to set summary info %u\n", r );

    r = MsiSummaryInfoSetPropertyA( hsi, 7, VT_LPSTR, 0, NULL, ";1033" );
    ok( r == ERROR_SUCCESS, "failed to set summary info %u\n", r );

    r = MsiSummaryInfoSetPropertyA( hsi, 9, VT_LPSTR, 0, NULL, "{E528DDD6-4801-4BEC-BBB6-C5EE0FD097E9}" );
    ok( r == ERROR_SUCCESS, "failed to set summary info %u\n", r );

    r = MsiSummaryInfoSetPropertyA( hsi, 14, VT_I4, 100, NULL, NULL );
    ok( r == ERROR_SUCCESS, "failed to set summary info %u\n", r );

    r = MsiSummaryInfoSetPropertyA( hsi, 15, VT_I4, 0, NULL, NULL );
    ok( r == ERROR_SUCCESS, "failed to set summary info %u\n", r );

    r = MsiSummaryInfoPersist( hsi );
    ok( r == ERROR_SUCCESS, "failed to persist suminfo %u\n", r );

    r = MsiCloseHandle( hsi );
    ok( r == ERROR_SUCCESS, "failed to close suminfo %u\n", r );

    r = MsiCloseHandle( hdb );
    ok( r == ERROR_SUCCESS, "failed to close database %u\n", r );
}

static void create_database( const char *filename, const struct msi_table *tables, UINT num_tables )
{
    MSIHANDLE hdb;
    UINT r, i;
    WCHAR *filenameW;
    int len;

    len = MultiByteToWideChar( CP_ACP, 0, filename, -1, NULL, 0 );
    if (!(filenameW = malloc( len * sizeof(WCHAR) ))) return;
    MultiByteToWideChar( CP_ACP, 0, filename, -1, filenameW, len );

    r = MsiOpenDatabaseW( filenameW, MSIDBOPEN_CREATE, &hdb );
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

    /* import the tables into the database */
    for (i = 0; i < num_tables; i++)
    {
        const struct msi_table *table = &tables[i];

        write_file( table->filename, table->data, (table->size - 1) * sizeof(char) );

        r = MsiDatabaseImportA( hdb, CURR_DIR, table->filename );
        ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

        DeleteFileA( table->filename );
    }

    r = MsiDatabaseCommit( hdb );
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle( hdb );
    set_suminfo( filenameW );
    free( filenameW );
}

/* data for generating a patch */

/* table names - encoded as in an msi database file */
static const WCHAR p_name0[] = { 0x4840, 0x3b3f, 0x43f2, 0x4438, 0x45b1, 0 }; /* _Columns */
static const WCHAR p_name1[] = { 0x4840, 0x3f7f, 0x4164, 0x422f, 0x4836, 0 }; /* _Tables */
static const WCHAR p_name2[] = { 0x4840, 0x3f3f, 0x4577, 0x446c, 0x3b6a, 0x45e4, 0x4824, 0 }; /* _StringData */
static const WCHAR p_name3[] = { 0x4840, 0x3f3f, 0x4577, 0x446c, 0x3e6a, 0x44b2, 0x482f, 0 }; /* _StringPool */
static const WCHAR p_name4[] = { 0x3a8c, 0x47cb, 0x45b0, 0x45ec, 0x45a8, 0x4837, 0}; /* CAB_msitest */
static const WCHAR p_name5[] = { 0x4840, 0x4596, 0x3e6c, 0x45e4, 0x42e6, 0x421c, 0x4634, 0x4468, 0x4226, 0 }; /* MsiPatchSequence */
static const WCHAR p_name6[] = { 0x0005, 0x0053, 0x0075, 0x006d, 0x006d, 0x0061, 0x0072,
                                 0x0079, 0x0049, 0x006e, 0x0066, 0x006f, 0x0072, 0x006d,
                                 0x0061, 0x0074, 0x0069, 0x006f, 0x006e, 0 }; /* SummaryInformation */
/* substorage names */
static const WCHAR p_name7[] = { 0x0074, 0x0061, 0x0072, 0x0067, 0x0065, 0x0074, 0x0054, 0x006f, 0x0075, 0x0070,
                                 0x0067, 0x0072, 0x0061, 0x0064, 0x0065, 0x0064, 0 }; /* targetToupgraded */
static const WCHAR p_name8[] = { 0x0023, 0x0074, 0x0061, 0x0072, 0x0067, 0x0065, 0x0074, 0x0054, 0x006f, 0x0075,
                                 0x0070, 0x0067, 0x0072, 0x0061, 0x0064, 0x0065, 0x0064, 0 }; /* #targetToupgraded */

/* data in each table */
static const WCHAR p_data0[] = { /* _Columns */
    0x0001, 0x0001, 0x0001, 0x0001, 0x8001, 0x8002, 0x8003, 0x8004,
    0x0002, 0x0003, 0x0004, 0x0005, 0xad00, 0xbd26, 0x8d00, 0x9502
};
static const WCHAR p_data1[] = { /* _Tables */
    0x0001
};
static const char p_data2[] = { /* _StringData */
    "MsiPatchSequencePatchFamilyProductCodeSequenceAttributes1.1.19388.37230913B8D18FBB64CACA239C74C11E3FA74"
};
static const WCHAR p_data3[] = { /* _StringPool */
/* len, refs */
     0,  0,     /* string 0 '' */
    16,  5,     /* string 1 'MsiPatchSequence' */
    11,  1,     /* string 2 'PatchFamily' */
    11,  1,     /* string 3 'ProductCode' */
     8,  1,     /* string 4 'Sequence' */
    10,  1,     /* string 5 'Attributes' */
    15,  1,     /* string 6 '1.1.19388.37230' */
    32,  1,     /* string 7 '913B8D18FBB64CACA239C74C11E3FA74' */
};
static const char p_data4[] = { /* CAB_msitest */
    0x4d, 0x53, 0x43, 0x46, 0x00, 0x00, 0x00, 0x00, 0x94, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x03, 0x01, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x5f, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
    0xea, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87,
    0x3c, 0xd4, 0xb8, 0x20, 0x00, 0x70, 0x61, 0x74, 0x63, 0x68, 0x2e,
    0x74, 0x78, 0x74, 0x00, 0xe8, 0x03, 0x00, 0x00, 0xea, 0x03, 0x00,
    0x00, 0x00, 0x00, 0xcb, 0x50, 0x17, 0x7e, 0x20, 0x00, 0x66, 0x69,
    0x6c, 0x65, 0x2e, 0x74, 0x78, 0x74, 0x00, 0xb0, 0xb2, 0xb2, 0x25,
    0x2d, 0x00, 0xd2, 0x07, 0x43, 0x4b, 0xcb, 0x2d, 0xce, 0x2c, 0x49,
    0x2d, 0x2e, 0x89, 0x29, 0x48, 0x2c, 0x49, 0xce, 0x48, 0x4d, 0xd1,
    0x2b, 0xa9, 0x28, 0x51, 0x18, 0x05, 0xa3, 0x60, 0x14, 0x0c, 0x37,
    0x90, 0x8b, 0x9c, 0xd3, 0x41, 0xf9, 0x9c, 0x61, 0x14, 0x8c, 0x82,
    0x51, 0x30, 0xdc, 0x00, 0x00
};
static const WCHAR p_data5[] = { /* MsiPatchSequence */
    0x0007, 0x0000, 0x0006, 0x8000
};
static const char p_data6[] = { /* SummaryInformation */
    0xfe, 0xff, 0x00, 0x00, 0x05, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xe0, 0x85, 0x9f, 0xf2, 0xf9,
    0x4f, 0x68, 0x10, 0xab, 0x91, 0x08, 0x00, 0x2b, 0x27, 0xb3, 0xd9,
    0x30, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00,
    0x00, 0x09, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x07, 0x00,
    0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x90,
    0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00,
    0x0f, 0x00, 0x00, 0x00, 0xd8, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00,
    0x00, 0x27, 0x00, 0x00, 0x00, 0x7b, 0x30, 0x46, 0x39, 0x36, 0x43,
    0x44, 0x43, 0x30, 0x2d, 0x34, 0x43, 0x44, 0x46, 0x2d, 0x34, 0x33,
    0x30, 0x34, 0x2d, 0x42, 0x32, 0x38, 0x33, 0x2d, 0x37, 0x42, 0x39,
    0x32, 0x36, 0x34, 0x38, 0x38, 0x39, 0x45, 0x46, 0x37, 0x7d, 0x00,
    0x00, 0x1e, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x7b, 0x39,
    0x31, 0x33, 0x42, 0x38, 0x44, 0x31, 0x38, 0x2d, 0x46, 0x42, 0x42,
    0x36, 0x2d, 0x34, 0x43, 0x41, 0x43, 0x2d, 0x41, 0x32, 0x33, 0x39,
    0x2d, 0x43, 0x37, 0x34, 0x43, 0x31, 0x31, 0x45, 0x33, 0x46, 0x41,
    0x37, 0x34, 0x7d, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x25, 0x00,
    0x00, 0x00, 0x3a, 0x74, 0x61, 0x72, 0x67, 0x65, 0x74, 0x54, 0x6f,
    0x75, 0x70, 0x67, 0x72, 0x61, 0x64, 0x65, 0x64, 0x3b, 0x3a, 0x23,
    0x74, 0x61, 0x72, 0x67, 0x65, 0x74, 0x54, 0x6f, 0x75, 0x70, 0x67,
    0x72, 0x61, 0x64, 0x65, 0x64, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00,
    0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x50, 0x61, 0x74, 0x63, 0x68,
    0x53, 0x6f, 0x75, 0x72, 0x63, 0x65, 0x4c, 0x69, 0x73, 0x74, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
};

struct table_data {
    LPCWSTR name;
    const void *data;
    DWORD size;
};

static const struct table_data table_patch_data[] = {
    { p_name0, p_data0, sizeof p_data0 },
    { p_name1, p_data1, sizeof p_data1 },
    { p_name2, p_data2, sizeof p_data2 - 1 },
    { p_name3, p_data3, sizeof p_data3 },
    { p_name4, p_data4, sizeof p_data4 },
    { p_name5, p_data5, sizeof p_data5 },
    { p_name6, p_data6, sizeof p_data6 }
};

static const WCHAR t1_name0[] = { 0x4840, 0x430f, 0x422f, 0 }; /* File */
static const WCHAR t1_name1[] = { 0x4840, 0x3f3f, 0x4577, 0x446c, 0x3b6a, 0x45e4, 0x4824, 0 }; /* _StringData */
static const WCHAR t1_name2[] = { 0x4840, 0x3f3f, 0x4577, 0x446c, 0x3e6a, 0x44b2, 0x482f, 0 }; /* _StringPool */
static const WCHAR t1_name3[] = { 0x0005, 0x0053, 0x0075, 0x006d, 0x006d, 0x0061, 0x0072,
                                  0x0079, 0x0049, 0x006e, 0x0066, 0x006f, 0x0072, 0x006d,
                                  0x0061, 0x0074, 0x0069, 0x006f, 0x006e, 0 }; /* SummaryInformation */

static const WCHAR t1_data0[] = { /* File */
    0x0008, 0x0001, 0x03ea, 0x8000
};
static const char t1_data1[] = { /* _StringData */
    "patch.txt"
};
static const WCHAR t1_data2[] = { /* _StringPool */
/* len, refs */
     0,  0,     /* string 0 '' */
     9,  1,     /* string 1 'patch.txt' */
};
static const char t1_data3[] = { /* SummaryInformation */
    0xfe, 0xff, 0x00, 0x00, 0x05, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xe0, 0x85, 0x9f, 0xf2, 0xf9,
    0x4f, 0x68, 0x10, 0xab, 0x91, 0x08, 0x00, 0x2b, 0x27, 0xb3, 0xd9,
    0x30, 0x00, 0x00, 0x00, 0x9f, 0x01, 0x00, 0x00, 0x0c, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x03, 0x00,
    0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xa8,
    0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0xc4, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00,
    0x00, 0xd0, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0xdc, 0x00,
    0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0xe4, 0x00, 0x00, 0x00, 0x08,
    0x00, 0x00, 0x00, 0xf7, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
    0x07, 0x01, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x8f, 0x01, 0x00,
    0x00, 0x10, 0x00, 0x00, 0x00, 0x97, 0x01, 0x00, 0x00, 0x1e, 0x00,
    0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x49, 0x6e, 0x73, 0x74, 0x61,
    0x6c, 0x6c, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x44, 0x61, 0x74,
    0x61, 0x62, 0x61, 0x73, 0x65, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00,
    0x00, 0x16, 0x00, 0x00, 0x00, 0x49, 0x6e, 0x73, 0x74, 0x61, 0x6c,
    0x6c, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x44, 0x61, 0x74, 0x61,
    0x62, 0x61, 0x73, 0x65, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x57, 0x69, 0x6e, 0x65, 0x48, 0x51, 0x00,
    0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x1e, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x78, 0x38,
    0x36, 0x3b, 0x31, 0x30, 0x33, 0x33, 0x00, 0x00, 0x00, 0x1e, 0x00,
    0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x3b, 0x31, 0x30, 0x33, 0x33,
    0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00,
    0x7b, 0x39, 0x31, 0x33, 0x42, 0x38, 0x44, 0x31, 0x38, 0x2d, 0x46,
    0x42, 0x42, 0x36, 0x2d, 0x34, 0x43, 0x41, 0x43, 0x2d, 0x41, 0x32,
    0x33, 0x39, 0x2d, 0x43, 0x37, 0x34, 0x43, 0x31, 0x31, 0x45, 0x33,
    0x46, 0x41, 0x37, 0x34, 0x7d, 0x31, 0x2e, 0x31, 0x2e, 0x31, 0x3b,
    0x7b, 0x39, 0x31, 0x33, 0x42, 0x38, 0x44, 0x31, 0x38, 0x2d, 0x46,
    0x42, 0x42, 0x36, 0x2d, 0x34, 0x43, 0x41, 0x43, 0x2d, 0x41, 0x32,
    0x33, 0x39, 0x2d, 0x43, 0x37, 0x34, 0x43, 0x31, 0x31, 0x45, 0x33,
    0x46, 0x41, 0x37, 0x34, 0x7d, 0x31, 0x2e, 0x31, 0x2e, 0x31, 0x3b,
    0x7b, 0x41, 0x32, 0x45, 0x33, 0x44, 0x36, 0x34, 0x33, 0x2d, 0x34,
    0x45, 0x32, 0x43, 0x2d, 0x34, 0x37, 0x37, 0x46, 0x2d, 0x41, 0x33,
    0x30, 0x39, 0x2d, 0x46, 0x37, 0x36, 0x46, 0x35, 0x35, 0x32, 0x44,
    0x35, 0x46, 0x34, 0x33, 0x7d, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x64, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x17, 0x00, 0x22,
    0x09
};

static const struct table_data table_transform1_data[] = {
    { t1_name0, t1_data0, sizeof t1_data0 },
    { t1_name1, t1_data1, sizeof t1_data1 - 1 },
    { t1_name2, t1_data2, sizeof t1_data2 },
    { t1_name3, t1_data3, sizeof t1_data3 }
};

static const WCHAR t2_name0[] = { 0x4840, 0x430f, 0x422f, 0 }; /* File */
static const WCHAR t2_name1[] = { 0x4840, 0x4216, 0x4327, 0x4824, 0 }; /* Media */
static const WCHAR t2_name2[] = { 0x4840, 0x3b3f, 0x43f2, 0x4438, 0x45b1, 0 }; /* _Columns */
static const WCHAR t2_name3[] = { 0x4840, 0x3f7f, 0x4164, 0x422f, 0x4836, 0 }; /* _Tables */
static const WCHAR t2_name4[] = { 0x4840, 0x4559, 0x44f2, 0x4568, 0x4737, 0 }; /* Property */
static const WCHAR t2_name5[] = { 0x4840, 0x4119, 0x41b7, 0x3e6b, 0x41a4, 0x412e, 0x422a, 0 }; /* PatchPackage */
static const WCHAR t2_name6[] = { 0x4840, 0x4452, 0x45f6, 0x43e4, 0x3baf, 0x423b, 0x4626,
                                  0x4237, 0x421c, 0x4634, 0x4468, 0x4226, 0 }; /* InstallExecuteSequence */
static const WCHAR t2_name7[] = { 0x4840, 0x3f3f, 0x4577, 0x446c, 0x3b6a, 0x45e4, 0x4824, 0 }; /* _StringData */
static const WCHAR t2_name8[] = { 0x4840, 0x3f3f, 0x4577, 0x446c, 0x3e6a, 0x44b2, 0x482f, 0 }; /* _StringPool */
static const WCHAR t2_name9[] = { 0x0005, 0x0053, 0x0075, 0x006d, 0x006d, 0x0061, 0x0072,
                                  0x0079, 0x0049, 0x006e, 0x0066, 0x006f, 0x0072, 0x006d,
                                  0x0061, 0x0074, 0x0069, 0x006f, 0x006e, 0 }; /* SummaryInformation */
static const WCHAR t2_name10[] = { 0x4840, 0x420f, 0x45e4, 0x4578, 0x3b28, 0x4432, 0x44b3,
                                   0x4231, 0x45f1, 0x4836, 0 }; /* FeatureComponents */
static const WCHAR t2_name11[] = { 0x4840, 0x448c, 0x44f0, 0x4472, 0x4468, 0x4837, 0 }; /* Component */
static const WCHAR t2_name12[] = { 0x4840, 0x420f, 0x45e4, 0x4578, 0x4828, 0 }; /* Feature */

static const WCHAR t2_data0[] = { /* File */
    0x00c0, 0x0001, 0x9000, 0x83e8, 0x0801, 0x0002, 0x0003, 0x0002,
    0x03e8, 0x8000, 0x0000, 0x0000, 0x9000, 0x83e9
};
static const WCHAR t2_data1[] = { /* Media */
    0x0601, 0x8002, 0x03e9, 0x8000, 0x0000, 0x000d, 0x0000, 0x000e
};
static const WCHAR t2_data2[] = { /* _Columns */
    0x0401, 0x000f, 0x0000, 0x0010, 0xad48, 0x0401, 0x000f, 0x0000, /* 0x0401 = add row (1), 4 shorts */
    0x0011, 0xa502, 0x0401, 0x000f, 0x0000, 0x0012, 0x8104, 0x0401,
    0x000f, 0x0000, 0x0013, 0x8502, 0x0401, 0x000f, 0x0000, 0x0014,
    0x9900, 0x0401, 0x000f, 0x0000, 0x0015, 0x9d48, 0x0401, 0x0016,
    0x0000, 0x0017, 0xad26, 0x0401, 0x0016, 0x0000, 0x0018, 0x8502,
    0x0401, 0x001a, 0x0000, 0x001b, 0xad26, 0x0401, 0x001a, 0x0000,
    0x0014, 0x8900
};
static const WCHAR t2_data3[] = { /* _Tables */
    0x0101, 0x000f, 0x0101, 0x0016, 0x0101, 0x001a
};
static const WCHAR t2_data4[] = { /* Property */
    0x0002, 0x0008, 0x0009, 0x0201, 0x000a, 0x000b
};
static const WCHAR t2_data5[] = { /* PatchPackage */
    0x0201, 0x0019, 0x8002
};
static const WCHAR t2_data6[] = { /* InstallExecuteSequence */
    0x0301, 0x000c, 0x0000, 0x87d1
};
static const char t2_data7[] = { /* _StringData */
    "patch.txtfile.txtfile{327d9640-674f-4b9f-8b8a-547a0f6f8518}MSITESTDIRnewnew featurePATCHNEWSUMMARYSUBJECT"
    "Installation DatabasePATCHNEWPACKAGECODE{42A14A82-12F8-4E6D-970E-1B4EE7BE28B0}PatchFiles#CAB_msitestprop"
    "PatchFile_SequencePatchSizeAttributesHeaderStreamRef_PatchPackagePatchIdMedia_"
    "{0F96CDC0-4CDF-4304-B283-7B9264889EF7}MsiPatchHeadersStreamRef"
};
static const WCHAR t2_data8[] = { /* _StringPool */
/* len, refs */
     0,  0,     /* string 0 '' */
     9,  1,     /* string 1 'patch.txt' */
     8,  3,     /* string 2 'file.txt' */
     4,  3,     /* string 3 'file' */
    38,  1,     /* string 4 '{327d9640-674f-4b9f-8b8a-547a0f6f8518}' */
    10,  2,     /* string 5 'MSITESTDIR' */
     3,  2,     /* string 6 'new' */
    11,  1,     /* string 7 'new feature' */
    22,  1,     /* string 8 'PATCHNEWSUMMARYSUBJECT' */
    21,  1,     /* string 9 'Installation Database' */
    19,  1,     /* string 10 'PATCHNEWPACKAGECODE' */
    38,  1,     /* string 11 '{42A14A82-12F8-4E6D-970E-1B4EE7BE28B0}' */
    10,  1,     /* string 12 'PatchFiles' */
    12,  1,     /* string 13 '#CAB_msitest' */
     4,  1,     /* string 14 'prop' */
     5,  7,     /* string 15 'Patch' */
     5,  1,     /* string 16 'File_' */
     8,  1,     /* string 17 'Sequence' */
     9,  1,     /* string 18 'PatchSize' */
    10,  1,     /* string 19 'Attributes' */
     6,  2,     /* string 20 'Header' */
    10,  1,     /* string 21 'StreamRef_' */
    12,  3,     /* string 22 'PatchPackage' */
     7,  1,     /* string 23 'PatchId' */
     6,  1,     /* string 24 'Media_' */
    38,  1,     /* string 25 '{0F96CDC0-4CDF-4304-B283-7B9264889EF7}' */
    15,  3,     /* string 26 'MsiPatchHeaders' */
     9,  1      /* string 27 'StreamRef' */
};
static const char t2_data9[] = { /* SummaryInformation */
    0xfe, 0xff, 0x00, 0x00, 0x05, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xe0, 0x85, 0x9f, 0xf2, 0xf9,
    0x4f, 0x68, 0x10, 0xab, 0x91, 0x08, 0x00, 0x2b, 0x27, 0xb3, 0xd9,
    0x30, 0x00, 0x00, 0x00, 0x5c, 0x01, 0x00, 0x00, 0x0b, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x03, 0x00,
    0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x78,
    0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x84, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00,
    0x00, 0x9c, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0xa8, 0x00,
    0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0x09,
    0x00, 0x00, 0x00, 0xc4, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
    0x4c, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x54, 0x01, 0x00,
    0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x3b, 0x31, 0x30, 0x33, 0x33, 0x00, 0x00,
    0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x7b,
    0x39, 0x31, 0x33, 0x42, 0x38, 0x44, 0x31, 0x38, 0x2d, 0x46, 0x42,
    0x42, 0x36, 0x2d, 0x34, 0x43, 0x41, 0x43, 0x2d, 0x41, 0x32, 0x33,
    0x39, 0x2d, 0x43, 0x37, 0x34, 0x43, 0x31, 0x31, 0x45, 0x33, 0x46,
    0x41, 0x37, 0x34, 0x7d, 0x31, 0x2e, 0x31, 0x2e, 0x31, 0x3b, 0x7b,
    0x39, 0x31, 0x33, 0x42, 0x38, 0x44, 0x31, 0x38, 0x2d, 0x46, 0x42,
    0x42, 0x36, 0x2d, 0x34, 0x43, 0x41, 0x43, 0x2d, 0x41, 0x32, 0x33,
    0x39, 0x2d, 0x43, 0x37, 0x34, 0x43, 0x31, 0x31, 0x45, 0x33, 0x46,
    0x41, 0x37, 0x34, 0x7d, 0x31, 0x2e, 0x31, 0x2e, 0x31, 0x3b, 0x7b,
    0x41, 0x32, 0x45, 0x33, 0x44, 0x36, 0x34, 0x33, 0x2d, 0x34, 0x45,
    0x32, 0x43, 0x2d, 0x34, 0x37, 0x37, 0x46, 0x2d, 0x41, 0x33, 0x30,
    0x39, 0x2d, 0x46, 0x37, 0x36, 0x46, 0x35, 0x35, 0x32, 0x44, 0x35,
    0x46, 0x34, 0x33, 0x7d, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x2d,
    0x01, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x17, 0x00, 0x27, 0x09
};
static const WCHAR t2_data10[] = { /* FeatureComponents */
    0x0201, 0x0006, 0x0003
};
static const WCHAR t2_data11[] = { /* Component */
    0x0601, 0x0003, 0x0004, 0x0005, 0x8000, 0x0000, 0x0002
};
static const WCHAR t2_data12[] = { /* Feature */
    0x0801, 0x0006, 0x0000, 0x0000, 0x0007, 0x8001, 0x8001, 0x0005,
    0x8000
};

static const struct table_data table_transform2_data[] = {
    { t2_name0, t2_data0, sizeof t2_data0 },
    { t2_name1, t2_data1, sizeof t2_data1 },
    { t2_name2, t2_data2, sizeof t2_data2 },
    { t2_name3, t2_data3, sizeof t2_data3 },
    { t2_name4, t2_data4, sizeof t2_data4 },
    { t2_name5, t2_data5, sizeof t2_data5 },
    { t2_name6, t2_data6, sizeof t2_data6 },
    { t2_name7, t2_data7, sizeof t2_data7 - 1 },
    { t2_name8, t2_data8, sizeof t2_data8 },
    { t2_name9, t2_data9, sizeof t2_data9 },
    { t2_name10, t2_data10, sizeof t2_data10 },
    { t2_name11, t2_data11, sizeof t2_data11 },
    { t2_name12, t2_data12, sizeof t2_data12 },
};

static void write_tables( IStorage *stg, const struct table_data *tables, UINT num_tables )
{
    IStream *stm;
    DWORD i, count;
    HRESULT r;

    for (i = 0; i < num_tables; i++)
    {
        r = IStorage_CreateStream( stg, tables[i].name, STGM_WRITE|STGM_SHARE_EXCLUSIVE, 0, 0, &stm );
        if (FAILED( r ))
        {
            ok( 0, "failed to create stream %#lx\n", r );
            continue;
        }

        r = IStream_Write( stm, tables[i].data, tables[i].size, &count );
        if (FAILED( r ) || count != tables[i].size)
            ok( 0, "failed to write stream\n" );
        IStream_Release( stm );
    }
}

static void create_patch( const char *filename )
{
    IStorage *stg = NULL, *stg1 = NULL, *stg2 = NULL;
    WCHAR *filenameW;
    HRESULT r;
    int len;
    const DWORD mode = STGM_CREATE|STGM_READWRITE|STGM_DIRECT|STGM_SHARE_EXCLUSIVE;

    const CLSID CLSID_MsiPatch = {0xc1086, 0, 0, {0xc0, 0, 0, 0, 0, 0, 0, 0x46}};
    const CLSID CLSID_MsiTransform = {0xc1082, 0, 0, {0xc0, 0, 0, 0, 0, 0, 0, 0x46}};

    len = MultiByteToWideChar( CP_ACP, 0, filename, -1, NULL, 0 );
    filenameW = malloc( len * sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, filename, -1, filenameW, len );

    r = StgCreateDocfile( filenameW, mode, 0, &stg );
    free( filenameW );
    ok( r == S_OK, "failed to create storage %#lx\n", r );
    if (!stg)
        return;

    r = IStorage_SetClass( stg, &CLSID_MsiPatch );
    ok( r == S_OK, "failed to set storage type %#lx\n", r );

    write_tables( stg, table_patch_data, ARRAY_SIZE( table_patch_data ));

    r = IStorage_CreateStorage( stg, p_name7, mode, 0, 0, &stg1 );
    ok( r == S_OK, "failed to create substorage %#lx\n", r );

    r = IStorage_SetClass( stg1, &CLSID_MsiTransform );
    ok( r == S_OK, "failed to set storage type %#lx\n", r );

    write_tables( stg1, table_transform1_data, ARRAY_SIZE( table_transform1_data ));
    IStorage_Release( stg1 );

    r = IStorage_CreateStorage( stg, p_name8, mode, 0, 0, &stg2 );
    ok( r == S_OK, "failed to create substorage %#lx\n", r );

    r = IStorage_SetClass( stg2, &CLSID_MsiTransform );
    ok( r == S_OK, "failed to set storage type %#lx\n", r );

    write_tables( stg2, table_transform2_data, ARRAY_SIZE( table_transform2_data ));
    IStorage_Release( stg2 );
    IStorage_Release( stg );
}

static void test_simple_patch( void )
{
    UINT r;
    DWORD size;
    char path[MAX_PATH], install_source[MAX_PATH], buffer[32];
    const char *query;
    WCHAR pathW[MAX_PATH];
    MSIHANDLE hpackage, hdb, hview, hrec;

    if (!pMsiApplyPatchA)
    {
        win_skip("MsiApplyPatchA is not available\n");
        return;
    }
    if (is_process_limited())
    {
        skip("process is limited\n");
        return;
    }

    CreateDirectoryA( "msitest", NULL );
    create_file( "msitest\\patch.txt", 1000 );

    create_database( msifile, tables, ARRAY_SIZE(tables) );
    create_patch( mspfile );

    MsiSetInternalUI( INSTALLUILEVEL_NONE, NULL );

    r = MsiInstallProductA( msifile, "DISABLE_FEATURE=1" );
    if (r != ERROR_SUCCESS)
    {
        skip("Product installation failed with error code %u\n", r);
        goto cleanup;
    }

    size = get_pf_file_size( "msitest\\patch.txt" );
    ok( size == 1000, "expected 1000, got %lu\n", size );

    size = sizeof(install_source);
    r = MsiGetProductInfoA( "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}",
                            "InstallSource", install_source, &size );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    strcpy( path, CURR_DIR );
    strcat( path, "\\" );
    strcat( path, msifile );

    r = MsiOpenPackageA( path, &hpackage );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    hdb = MsiGetActiveDatabase( hpackage );
    ok( hdb, "failed to get database handle\n" );

    query = "SELECT * FROM `Property` where `Property` = 'PATCHNEWPACKAGECODE'";
    r = MsiDatabaseOpenViewA( hdb, query, &hview );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewExecute( hview, 0 );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewFetch( hview, &hrec );
    ok( r == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %u\n", r );

    MsiCloseHandle( hrec );
    MsiViewClose( hview );
    MsiCloseHandle( hview );

    query = "SELECT * FROM `Property` WHERE `Property` = 'PATCHNEWSUMMARYSUBJECT' "
            "AND `Value` = 'Installer Database'";
    r = MsiDatabaseOpenViewA( hdb, query, &hview );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewExecute( hview, 0 );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewFetch( hview, &hrec );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    MsiCloseHandle( hrec );
    MsiViewClose( hview );
    MsiCloseHandle( hview );

    buffer[0] = 0;
    size = sizeof(buffer);
    r = MsiGetPropertyA( hpackage, "PATCHNEWSUMMARYSUBJECT", buffer, &size );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );
    ok( !strcmp( buffer, "Installer Database" ), "expected \'Installer Database\', got \'%s\'\n", buffer );

    MsiCloseHandle( hdb );
    MsiCloseHandle( hpackage );

    r = MsiApplyPatchA( mspfile, NULL, INSTALLTYPE_DEFAULT, NULL );
    ok( r == ERROR_SUCCESS || broken( r == ERROR_PATCH_PACKAGE_INVALID ), /* version 2.0 */
        "expected ERROR_SUCCESS, got %u\n", r );

    if (r == ERROR_PATCH_PACKAGE_INVALID)
    {
        win_skip("Windows Installer < 3.0 detected\n");
        goto uninstall;
    }

    size = get_pf_file_size( "msitest\\patch.txt" );
    ok( size == 1002, "expected 1002, got %lu\n", size );
    size = get_pf_file_size( "msitest\\file.txt" );
    ok( size == 1000, "expected 1000, got %lu\n", size );

    /* show that MsiOpenPackage applies registered patches */
    r = MsiOpenPackageA( path, &hpackage );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    hdb = MsiGetActiveDatabase( hpackage );
    ok( hdb, "failed to get database handle\n" );

    query = "SELECT * FROM `Property` where `Property` = 'PATCHNEWPACKAGECODE'";
    r = MsiDatabaseOpenViewA( hdb, query, &hview );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewExecute( hview, 0 );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewFetch( hview, &hrec );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    MsiCloseHandle( hrec );
    MsiViewClose( hview );
    MsiCloseHandle( hview );

    query = "SELECT * FROM `Property` WHERE `Property` = 'PATCHNEWSUMMARYSUBJECT' "
            "AND `Value` = 'Installation Database'";
    r = MsiDatabaseOpenViewA( hdb, query, &hview );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewExecute( hview, 0 );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewFetch( hview, &hrec );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    MsiCloseHandle( hrec );
    MsiViewClose( hview );
    MsiCloseHandle( hview );

    buffer[0] = 0;
    size = sizeof(buffer);
    r = MsiGetPropertyA( hpackage, "PATCHNEWSUMMARYSUBJECT", buffer, &size );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );
    ok( !strcmp( buffer, "Installation Database" ), "expected \'Installation Database\', got \'%s\'\n", buffer );

    MsiCloseHandle( hdb );
    MsiCloseHandle( hpackage );

    /* show that patches are not committed to the local package database */
    size = sizeof(path);
    r = MsiGetProductInfoA( "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}",
                            "LocalPackage", path, &size );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    MultiByteToWideChar( CP_ACP, 0, path, -1, pathW, MAX_PATH );
    r = MsiOpenDatabaseW( pathW, MSIDBOPEN_READONLY, &hdb );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiDatabaseOpenViewA( hdb, query, &hview );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewExecute( hview, 0 );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewFetch( hview, &hrec );
    ok( r == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %u\n", r );

    MsiCloseHandle( hrec );
    MsiViewClose( hview );
    MsiCloseHandle( hview );
    MsiCloseHandle( hdb );

uninstall:
    size = sizeof(path);
    r = MsiGetProductInfoA( "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}",
                            "InstallSource", path, &size );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );
    ok( !strcasecmp( path, install_source ), "wrong path %s\n", path );

    r = MsiInstallProductA( msifile, "REMOVE=ALL" );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    ok( !delete_pf( "msitest\\patch.txt", TRUE ), "file not removed\n" );
    ok( !delete_pf( "msitest\\file.txt", TRUE ), "file not removed\n" );
    ok( !delete_pf( "msitest", FALSE ), "directory not removed\n" );

cleanup:
    DeleteFileA( msifile );
    DeleteFileA( mspfile );
    DeleteFileA( "msitest\\patch.txt" );
    RemoveDirectoryA( "msitest" );
}

static void test_MsiOpenDatabase( void )
{
    UINT r;
    MSIHANDLE hdb;

    r = MsiOpenDatabaseW( mspfileW, MSIDBOPEN_CREATE, &hdb );
    ok(r == ERROR_SUCCESS, "failed to open database %u\n", r);

    r = MsiDatabaseCommit( hdb );
    ok(r == ERROR_SUCCESS, "failed to commit database %u\n", r);
    MsiCloseHandle( hdb );

    r = MsiOpenDatabaseW( mspfileW, MSIDBOPEN_READONLY + MSIDBOPEN_PATCHFILE, &hdb );
    ok(r == ERROR_OPEN_FAILED, "expected ERROR_OPEN_FAILED, got %u\n", r);
    DeleteFileA( mspfile );

    r = MsiOpenDatabaseW( mspfileW, MSIDBOPEN_CREATE + MSIDBOPEN_PATCHFILE, &hdb );
    ok(r == ERROR_SUCCESS , "failed to open database %u\n", r);

    r = MsiDatabaseCommit( hdb );
    ok(r == ERROR_SUCCESS, "failed to commit database %u\n", r);
    MsiCloseHandle( hdb );

    r = MsiOpenDatabaseW( mspfileW, MSIDBOPEN_READONLY + MSIDBOPEN_PATCHFILE, &hdb );
    ok(r == ERROR_SUCCESS, "failed to open database %u\n", r);
    MsiCloseHandle( hdb );
    DeleteFileA( mspfile );

    create_database( msifile, tables, ARRAY_SIZE(tables) );
    create_patch( mspfile );

    r = MsiOpenDatabaseW( msifileW, MSIDBOPEN_READONLY + MSIDBOPEN_PATCHFILE, &hdb );
    ok(r == ERROR_OPEN_FAILED, "failed to open database %u\n", r );

    r = MsiOpenDatabaseW( mspfileW, MSIDBOPEN_READONLY + MSIDBOPEN_PATCHFILE, &hdb );
    ok(r == ERROR_SUCCESS, "failed to open database %u\n", r );
    MsiCloseHandle( hdb );

    DeleteFileA( msifile );
    DeleteFileA( mspfile );
}

static UINT find_entry( MSIHANDLE hdb, const char *table, const char *entry )
{
    static const char fmt[] = "SELECT * FROM `%s` WHERE `Name` = '%s'";
    char query[0x100];
    UINT r;
    MSIHANDLE hview, hrec;

    sprintf( query, fmt, table, entry );
    r = MsiDatabaseOpenViewA( hdb, query, &hview );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewExecute( hview, 0 );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewFetch( hview, &hrec );
    MsiViewClose( hview );
    MsiCloseHandle( hview );
    MsiCloseHandle( hrec );
    return r;
}

static UINT find_entryW( MSIHANDLE hdb, const WCHAR *table, const WCHAR *entry )
{
    WCHAR query[0x100];
    MSIHANDLE hview, hrec;
    UINT r;

    wsprintfW( query, L"SELECT * FROM `%s` WHERE `Name` = '%s'", table, entry );
    r = MsiDatabaseOpenViewW( hdb, query, &hview );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewExecute( hview, 0 );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewFetch( hview, &hrec );
    MsiViewClose( hview );
    MsiCloseHandle( hview );
    MsiCloseHandle( hrec );
    return r;
}

static INT get_integer( MSIHANDLE hdb, UINT field, const char *query)
{
    UINT r;
    INT ret = -1;
    MSIHANDLE hview, hrec;

    r = MsiDatabaseOpenViewA( hdb, query, &hview );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewExecute( hview, 0 );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewFetch( hview, &hrec );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );
    if (r == ERROR_SUCCESS)
    {
        UINT r_tmp;
        ret = MsiRecordGetInteger( hrec, field );
        MsiCloseHandle( hrec );

        r_tmp = MsiViewFetch( hview, &hrec );
        ok( r_tmp == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %u\n", r);
    }

    MsiViewClose( hview );
    MsiCloseHandle( hview );
    return ret;
}

static char *get_string( MSIHANDLE hdb, UINT field, const char *query)
{
    UINT r;
    static char ret[MAX_PATH];
    MSIHANDLE hview, hrec;

    ret[0] = '\0';

    r = MsiDatabaseOpenViewA( hdb, query, &hview );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewExecute( hview, 0 );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewFetch( hview, &hrec );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );
    if (r == ERROR_SUCCESS)
    {
        DWORD size = MAX_PATH;
        r = MsiRecordGetStringA( hrec, field, ret, &size );
        ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);
        MsiCloseHandle( hrec );

        r = MsiViewFetch( hview, &hrec );
        ok( r == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %u\n", r);
    }

    MsiViewClose( hview );
    MsiCloseHandle( hview );
    return ret;
}

static void test_system_tables( void )
{
    static const char patchsource[] = "MSPSRC0F96CDC04CDF4304B2837B9264889EF7";
    UINT r;
    char *cr;
    const char *query;
    MSIHANDLE hproduct, hdb, hview, hrec;

    if (!pMsiApplyPatchA)
    {
        win_skip("MsiApplyPatchA is not available\n");
        return;
    }
    if (is_process_limited())
    {
        skip("process is limited\n");
        return;
    }

    CreateDirectoryA( "msitest", NULL );
    create_file( "msitest\\patch.txt", 1000 );

    create_database( msifile, tables, ARRAY_SIZE(tables) );
    create_patch( mspfile );

    MsiSetInternalUI( INSTALLUILEVEL_NONE, NULL );

    r = MsiInstallProductA( msifile, "DISABLE_FEATURE=1" );
    if (r != ERROR_SUCCESS)
    {
        skip("Product installation failed with error code %d\n", r);
        goto cleanup;
    }

    r = MsiOpenProductA( "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}", &hproduct );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    hdb = MsiGetActiveDatabase( hproduct );
    ok( hdb, "failed to get database handle\n" );

    r = find_entry( hdb, "_Streams", "\5SummaryInformation" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    query = "SELECT * FROM `_Storages`";
    r = MsiDatabaseOpenViewA( hdb, query, &hview );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewExecute( hview, 0 );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewFetch( hview, &hrec );
    ok( r == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %u\n", r );

    r = find_entry( hdb, "_Tables", "Directory" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "File" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "Component" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "Feature" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "FeatureComponents" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "Property" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "InstallExecuteSequence" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "Media" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = get_integer( hdb, 1, "SELECT * FROM `Media` WHERE `VolumeLabel`=\'DISK1\'");
    ok( r == 1, "Got %u\n", r );

    r = find_entry( hdb, "_Tables", "_Property" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    MsiCloseHandle( hrec );
    MsiViewClose( hview );
    MsiCloseHandle( hview );
    MsiCloseHandle( hdb );
    MsiCloseHandle( hproduct );

    r = MsiApplyPatchA( mspfile, NULL, INSTALLTYPE_DEFAULT, NULL );
    ok( r == ERROR_SUCCESS || broken( r == ERROR_PATCH_PACKAGE_INVALID ), /* version 2.0 */
        "expected ERROR_SUCCESS, got %u\n", r );

    if (r == ERROR_PATCH_PACKAGE_INVALID)
    {
        win_skip("Windows Installer < 3.0 detected\n");
        goto uninstall;
    }

    r = MsiOpenProductA( "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}", &hproduct );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    hdb = MsiGetActiveDatabase( hproduct );
    ok( hdb, "failed to get database handle\n" );

    r = find_entry( hdb, "_Streams", "\5SummaryInformation" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entryW( hdb, L"_Streams", L"\x3a8c\x47cb\x45b0\x45ec\x45a8\x4837" );
    ok( r == ERROR_NO_MORE_ITEMS, "failed to find entry %u\n", r );

    query = "SELECT * FROM `_Storages`";
    r = MsiDatabaseOpenViewA( hdb, query, &hview );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewExecute( hview, 0 );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    r = MsiViewFetch( hview, &hrec );
    ok( r == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %u\n", r );

    r = find_entry( hdb, "_Tables", "Directory" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "File" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "Component" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "Feature" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "FeatureComponents" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "Property" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "InstallExecuteSequence" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "Media" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "_Property" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "MsiPatchHeaders" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "Patch" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    r = find_entry( hdb, "_Tables", "PatchPackage" );
    ok( r == ERROR_SUCCESS, "failed to find entry %u\n", r );

    cr = get_string( hdb, 6, "SELECT * FROM `Media` WHERE `Source` IS NOT NULL");
    todo_wine ok( !strcmp(cr, patchsource), "Expected \"%s\", got \"%s\"\n", patchsource, cr );

    r = get_integer( hdb, 1, "SELECT * FROM `Media` WHERE `Source` IS NOT NULL");
    todo_wine ok( r == 100, "Got %u\n", r );

    r = get_integer( hdb, 2, "SELECT * FROM `Media` WHERE `Source` IS NOT NULL");
    todo_wine ok( r == 10001, "Got %u\n", r );

    r = get_integer( hdb, 1, "SELECT * FROM `Media` WHERE `VolumeLabel`=\'DISK1\'");
    ok( r == 1, "Got %u\n", r );

    cr = get_string( hdb, 4, "SELECT * FROM `Media` WHERE `Source` IS NOT NULL");
    ok( !strcmp(cr, "#CAB_msitest"), "Expected \"#CAB_msitest\", got \"%s\"\n", cr );

    r = get_integer( hdb, 8, "SELECT * FROM `File` WHERE `File` = 'patch.txt'");
    ok( r == 10000, "Got %u\n", r );

    MsiCloseHandle( hrec );
    MsiViewClose( hview );
    MsiCloseHandle( hview );
    MsiCloseHandle( hdb );
    MsiCloseHandle( hproduct );

uninstall:
    r = MsiInstallProductA( msifile, "REMOVE=ALL" );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

cleanup:
    DeleteFileA( msifile );
    DeleteFileA( mspfile );
    DeleteFileA( "msitest\\patch.txt" );
    RemoveDirectoryA( "msitest" );
}

static void test_patch_registration( void )
{
    UINT r;
    DWORD size;
    char buffer[MAX_PATH], patch_code[39];

    if (!pMsiApplyPatchA || !pMsiGetPatchInfoExA || !pMsiEnumPatchesExA)
    {
        win_skip("required functions not available\n");
        return;
    }
    if (is_process_limited())
    {
        skip("process is limited\n");
        return;
    }

    CreateDirectoryA( "msitest", NULL );
    create_file( "msitest\\patch.txt", 1000 );

    create_database( msifile, tables, ARRAY_SIZE(tables) );
    create_patch( mspfile );

    MsiSetInternalUI( INSTALLUILEVEL_NONE, NULL );

    r = MsiInstallProductA( msifile, "DISABLE_FEATURE=1" );
    if (r != ERROR_SUCCESS)
    {
        skip("Product installation failed with error code %d\n", r);
        goto cleanup;
    }

    r = MsiApplyPatchA( mspfile, NULL, INSTALLTYPE_DEFAULT, NULL );
    ok( r == ERROR_SUCCESS || broken( r == ERROR_PATCH_PACKAGE_INVALID ), /* version 2.0 */
        "expected ERROR_SUCCESS, got %u\n", r );

    if (r == ERROR_PATCH_PACKAGE_INVALID)
    {
        win_skip("Windows Installer < 3.0 detected\n");
        goto uninstall;
    }

    buffer[0] = 0;
    size = sizeof(buffer);
    r = pMsiGetPatchInfoExA( "{0F96CDC0-4CDF-4304-B283-7B9264889EF7}",
                             "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}",
                              NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_LOCALPACKAGEA, buffer, &size );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );
    ok( buffer[0], "buffer empty\n" );

    buffer[0] = 0;
    size = sizeof(buffer);
    r = pMsiGetPatchInfoExA( "{0F96CDC0-4CDF-4304-B283-7B9264889EF7}",
                             "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}",
                             NULL, MSIINSTALLCONTEXT_MACHINE,
                             INSTALLPROPERTY_LOCALPACKAGEA, buffer, &size );
    ok( r == ERROR_UNKNOWN_PRODUCT, "expected ERROR_UNKNOWN_PRODUCT, got %u\n", r );

    buffer[0] = 0;
    size = sizeof(buffer);
    r = pMsiGetPatchInfoExA( "{0F96CDC0-4CDF-4304-B283-7B9264889EF7}",
                             "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}",
                             NULL, MSIINSTALLCONTEXT_USERMANAGED,
                             INSTALLPROPERTY_LOCALPACKAGEA, buffer, &size );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );
    ok( !buffer[0], "got %s\n", buffer );

    r = pMsiEnumPatchesExA( "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}",
                           NULL, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_APPLIED,
                           0, patch_code, NULL, NULL, NULL, NULL );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );
    ok( !strcmp( patch_code, "{0F96CDC0-4CDF-4304-B283-7B9264889EF7}" ), "wrong patch code\n" );

    r = pMsiEnumPatchesExA( "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}",
                           NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_APPLIED,
                           0, patch_code, NULL, NULL, NULL, NULL );
    ok( r == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %u\n", r );

    r = pMsiEnumPatchesExA( "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}",
                           NULL, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_APPLIED,
                           0, patch_code, NULL, NULL, NULL, NULL );
    ok( r == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %u\n", r );

uninstall:
    r = MsiInstallProductA( msifile, "REMOVE=ALL" );
    ok( r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r );

    buffer[0] = 0;
    size = sizeof(buffer);
    r = pMsiGetPatchInfoExA( "{0F96CDC0-4CDF-4304-B283-7B9264889EF7}",
                             "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}",
                              NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_LOCALPACKAGEA, buffer, &size );
    ok( r == ERROR_UNKNOWN_PRODUCT, "expected ERROR_UNKNOWN_PRODUCT, got %u\n", r );

cleanup:
    DeleteFileA( msifile );
    DeleteFileA( mspfile );
    DeleteFileA( "msitest\\patch.txt" );
    RemoveDirectoryA( "msitest" );
}

START_TEST(patch)
{
    DWORD len;
    char temp_path[MAX_PATH], prev_path[MAX_PATH];

    init_function_pointers();

    GetCurrentDirectoryA( MAX_PATH, prev_path );
    GetTempPathA( MAX_PATH, temp_path );
    SetCurrentDirectoryA( temp_path );

    strcpy( CURR_DIR, temp_path );
    len = strlen( CURR_DIR );

    if (len && (CURR_DIR[len - 1] == '\\'))
        CURR_DIR[len - 1] = 0;

    get_program_files_dir( PROG_FILES_DIR, COMMON_FILES_DIR );

    test_simple_patch();
    test_MsiOpenDatabase();
    test_system_tables();
    test_patch_registration();

    SetCurrentDirectoryA( prev_path );
}
