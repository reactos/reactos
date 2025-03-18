/*
 * Miscellaneous tests
 *
 * Copyright 2007 James Hawkins
 * Copyright 2007 Hans Leidekker
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
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "winreg.h"
#include "devguid.h"
#include "initguid.h"

#pragma push_macro("NTDDI_VERSION")
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_VISTA
#include "ntddvdeo.h"
#pragma pop_macro("NTDDI_VERSION")

#include "setupapi.h"
#include "cfgmgr32.h"

#include "wine/test.h"

static CHAR CURR_DIR[MAX_PATH];

/* test:
 *  - fails if not administrator
 *  - what if it's not a .inf file?
 *  - copied to %windir%/Inf
 *  - SourceInfFileName should be a full path
 *  - SourceInfFileName should be <= MAX_PATH
 *  - copy styles
 */

static void (WINAPI *pMyFree)(void*);
static BOOL (WINAPI *pSetupGetFileCompressionInfoExA)(PCSTR, PSTR, DWORD, PDWORD, PDWORD, PDWORD, PUINT);
static BOOL (WINAPI *pSetupQueryInfOriginalFileInformationA)(PSP_INF_INFORMATION, UINT, PSP_ALTPLATFORM_INFO, PSP_ORIGINAL_FILE_INFO_A);

static void create_file(const char *name, const char *data)
{
    HANDLE file;
    DWORD size;
    BOOL ret;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create %s, error %lu.\n", name, GetLastError());
    ret = WriteFile(file, data, strlen(data), &size, NULL);
    ok(ret && size == strlen(data), "Failed to write %s, error %lu.\n", name, GetLastError());
    CloseHandle(file);
}

static void get_temp_filename(LPSTR path)
{
    CHAR temp[MAX_PATH];
    LPSTR ptr;

    GetTempFileNameA(CURR_DIR, "set", 0, temp);
    ptr = strrchr(temp, '\\');

    strcpy(path, ptr + 1);
}

static BOOL file_exists(LPSTR path)
{
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

static BOOL is_in_inf_dir(const char *path)
{
    char expect[MAX_PATH];

    GetWindowsDirectoryA(expect, sizeof(expect));
    strcat(expect, "\\inf\\");
    return !strncasecmp(path, expect, strrchr(path, '\\') - path);
}

static void test_original_file_name(LPCSTR original, LPCSTR dest)
{
    HINF hinf;
    PSP_INF_INFORMATION pspii;
    SP_ORIGINAL_FILE_INFO_A spofi;
    BOOL res;
    DWORD size;

    if (!pSetupQueryInfOriginalFileInformationA)
    {
        win_skip("SetupQueryInfOriginalFileInformationA is not available\n");
        return;
    }

    hinf = SetupOpenInfFileA(dest, NULL, INF_STYLE_WIN4, NULL);
    ok(hinf != NULL, "SetupOpenInfFileA failed with error %ld\n", GetLastError());

    res = SetupGetInfInformationA(hinf, INFINFO_INF_SPEC_IS_HINF, NULL, 0, &size);
    ok(res, "SetupGetInfInformation failed with error %ld\n", GetLastError());

    pspii = HeapAlloc(GetProcessHeap(), 0, size);

    res = SetupGetInfInformationA(hinf, INFINFO_INF_SPEC_IS_HINF, pspii, size, NULL);
    ok(res, "SetupGetInfInformation failed with error %ld\n", GetLastError());

    spofi.cbSize = 0;
    res = pSetupQueryInfOriginalFileInformationA(pspii, 0, NULL, &spofi);
    ok(!res && GetLastError() == ERROR_INVALID_USER_BUFFER,
        "SetupQueryInfOriginalFileInformationA should have failed with ERROR_INVALID_USER_BUFFER instead of %ld\n", GetLastError());

    spofi.cbSize = sizeof(spofi);
    res = pSetupQueryInfOriginalFileInformationA(pspii, 0, NULL, &spofi);
    ok(res, "SetupQueryInfOriginalFileInformationA failed with error %ld\n", GetLastError());
    ok(!spofi.OriginalCatalogName[0], "spofi.OriginalCatalogName should have been \"\" instead of \"%s\"\n", spofi.OriginalCatalogName);
    ok(!strcmp(original, spofi.OriginalInfName), "spofi.OriginalInfName of %s didn't match real original name %s\n", spofi.OriginalInfName, original);

    HeapFree(GetProcessHeap(), 0, pspii);

    SetupCloseInfFile(hinf);
}

static void test_SetupCopyOEMInf(void)
{
    char path[MAX_PATH * 2], dest[MAX_PATH], tmpfile[MAX_PATH], orig_dest[MAX_PATH];
    char *filepart, pnf[MAX_PATH];
    DWORD size;
    BOOL res;

    static const char inf_data1[] =
        "[Version]\n"
        "Signature=\"$Chicago$\"\n"
        "; This is a WINE test INF file\n";

    static const char inf_data2[] =
        "[Version]\n"
        "Signature=\"$Chicago$\"\n"
        "; This is another WINE test INF file\n";

    /* try NULL SourceInfFileName */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA(NULL, NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    /* try empty SourceInfFileName */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA("", NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND ||
       GetLastError() == ERROR_INVALID_PARAMETER, /* Vista, W2K8 */
       "Unexpected error : %ld\n", GetLastError());

    /* try a relative nonexistent SourceInfFileName */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA("nonexistent", NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());

    /* try an absolute nonexistent SourceInfFileName */
    strcpy(path, CURR_DIR);
    strcat(path, "\\nonexistent");
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA(path, NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());

    get_temp_filename(tmpfile);
    create_file(tmpfile, inf_data1);

    /* try a relative SourceInfFileName */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA(tmpfile, NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    if (GetLastError() == ERROR_WRONG_INF_TYPE || GetLastError() == ERROR_UNSUPPORTED_TYPE /* Win7 */)
    {
       /* FIXME:
        * Vista needs a [Manufacturer] entry in the inf file. Doing this will give some
        * popups during the installation though as it also needs a catalog file (signed?).
        */
       win_skip("Needs a different inf file on Vista+\n");
       DeleteFileA(tmpfile);
       return;
    }

    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());
    ok(file_exists(tmpfile), "Expected tmpfile to exist\n");

    /* try SP_COPY_REPLACEONLY, dest does not exist */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA(path, NULL, SPOST_NONE, SP_COPY_REPLACEONLY, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());
    ok(file_exists(tmpfile), "Expected source inf to exist\n");

    /* Test a successful call. */
    strcpy(path, CURR_DIR);
    strcat(path, "\\");
    strcat(path, tmpfile);
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, sizeof(dest), NULL, NULL);
    if (!res && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("SetupCopyOEMInfA() failed on insufficient permissions\n");
        DeleteFileA(tmpfile);
        return;
    }
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    ok(file_exists(path), "Expected source inf to exist.\n");
    ok(file_exists(dest), "Expected dest file to exist.\n");
    ok(is_in_inf_dir(dest), "Got unexpected path '%s'.\n", dest);
    strcpy(orig_dest, dest);

    /* Existing INF files are checked for a match. */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, sizeof(dest), NULL, NULL);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    ok(file_exists(path), "Expected source inf to exist.\n");
    ok(file_exists(dest), "Expected dest file to exist.\n");
    ok(!strcmp(orig_dest, dest), "Expected '%s', got '%s'.\n", orig_dest, dest);

    /* try SP_COPY_REPLACEONLY, dest exists */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA(path, NULL, SPOST_NONE, SP_COPY_REPLACEONLY, dest, sizeof(dest), NULL, NULL);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    ok(file_exists(path), "Expected source inf to exist.\n");
    ok(file_exists(dest), "Expected dest file to exist.\n");
    ok(!strcmp(orig_dest, dest), "Expected '%s', got '%s'.\n", orig_dest, dest);

    strcpy(dest, "aaa");
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA(path, NULL, SPOST_NONE, SP_COPY_NOOVERWRITE, dest, sizeof(dest), NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    ok(GetLastError() == ERROR_FILE_EXISTS,
       "Expected ERROR_FILE_EXISTS, got %ld\n", GetLastError());
    ok(!strcmp(orig_dest, dest), "Expected '%s', got '%s'.\n", orig_dest, dest);

    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, NULL, 0, NULL, NULL);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    ok(file_exists(path), "Expected source inf to exist.\n");
    ok(file_exists(orig_dest), "Expected dest file to exist.\n");

    strcpy(dest, "aaa");
    size = 0;
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, 5, &size, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(file_exists(path), "Expected source inf to exist\n");
    ok(file_exists(orig_dest), "Expected dest inf to exist\n");
    ok(!strcmp(dest, "aaa"), "Expected dest to be unchanged\n");
    ok(size == strlen(orig_dest) + 1, "Got %ld.\n", size);

    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, sizeof(dest), &size, NULL);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    ok(!strcmp(orig_dest, dest), "Expected '%s', got '%s'.\n", orig_dest, dest);
    ok(size == strlen(dest) + 1, "Got %ld.\n", size);

    test_original_file_name(strrchr(path, '\\') + 1, dest);

    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, sizeof(dest), NULL, &filepart);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    ok(!strcmp(orig_dest, dest), "Expected '%s', got '%s'.\n", orig_dest, dest);
    ok(filepart == strrchr(dest, '\\') + 1, "Got unexpected file part %s.\n", filepart);

    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA(path, NULL, SPOST_NONE, SP_COPY_DELETESOURCE, NULL, 0, NULL, NULL);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    ok(!file_exists(path), "Expected source inf to not exist\n");

    strcpy(pnf, dest);
    *(strrchr(pnf, '.') + 1) = 'p';

    res = SetupUninstallOEMInfA(strrchr(dest, '\\') + 1, 0, NULL);
    ok(res, "Failed to uninstall '%s', error %lu.\n", dest, GetLastError());
    todo_wine ok(!file_exists(dest), "Expected inf '%s' to not exist\n", dest);
    DeleteFileA(dest);
    ok(!file_exists(pnf), "Expected pnf '%s' to not exist\n", pnf);

    create_file(tmpfile, inf_data1);
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, sizeof(dest), NULL, NULL);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    ok(is_in_inf_dir(dest), "Got unexpected path '%s'.\n", dest);
    strcpy(orig_dest, dest);

    create_file(tmpfile, inf_data2);
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, sizeof(dest), NULL, NULL);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    ok(is_in_inf_dir(dest), "Got unexpected path '%s'.\n", dest);
    ok(strcmp(dest, orig_dest), "Expected INF files to be copied to different paths.\n");

    res = SetupUninstallOEMInfA(strrchr(dest, '\\') + 1, 0, NULL);
    ok(res, "Failed to uninstall '%s', error %lu.\n", dest, GetLastError());
    todo_wine ok(!file_exists(dest), "Expected inf '%s' to not exist\n", dest);
    DeleteFileA(dest);
    strcpy(pnf, dest);
    *(strrchr(pnf, '.') + 1) = 'p';
    todo_wine ok(!file_exists(pnf), "Expected pnf '%s' to not exist\n", pnf);

    res = SetupUninstallOEMInfA(strrchr(orig_dest, '\\') + 1, 0, NULL);
    ok(res, "Failed to uninstall '%s', error %lu.\n", orig_dest, GetLastError());
    todo_wine ok(!file_exists(orig_dest), "Expected inf '%s' to not exist\n", dest);
    DeleteFileA(orig_dest);
    strcpy(pnf, dest);
    *(strrchr(pnf, '.') + 1) = 'p';
    todo_wine ok(!file_exists(pnf), "Expected pnf '%s' to not exist\n", pnf);

    GetWindowsDirectoryA(orig_dest, sizeof(orig_dest));
    strcat(orig_dest, "\\inf\\");
    strcat(orig_dest, tmpfile);
    res = CopyFileA(tmpfile, orig_dest, TRUE);
    ok(res, "Failed to copy file, error %lu.\n", GetLastError());
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, sizeof(dest), NULL, NULL);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    ok(!strcasecmp(dest, orig_dest), "Expected '%s', got '%s'.\n", orig_dest, dest);

    /* Since it wasn't actually installed, SetupUninstallOEMInf would fail here. */
    res = DeleteFileA(dest);
    ok(res, "Failed to delete '%s', error %lu.\n", tmpfile, GetLastError());

    res = DeleteFileA(tmpfile);
    ok(res, "Failed to delete '%s', error %lu.\n", tmpfile, GetLastError());
}

static void create_source_file(LPSTR filename, const BYTE *data, DWORD size)
{
    HANDLE handle;
    DWORD written;

    handle = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(handle, data, size, &written, NULL);
    CloseHandle(handle);
}

static BOOL compare_file_data(LPSTR file, const BYTE *data, DWORD size)
{
    DWORD read;
    HANDLE handle;
    BOOL ret = FALSE;
    LPBYTE buffer;

    handle = CreateFileA(file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    if (buffer)
    {
        ReadFile(handle, buffer, size, &read, NULL);
        if (read == size && !memcmp(data, buffer, size)) ret = TRUE;
        HeapFree(GetProcessHeap(), 0, buffer);
    }
    CloseHandle(handle);
    return ret;
}

static const BYTE uncompressed[] = {
    'u','n','c','o','m','p','r','e','s','s','e','d','\r','\n'
};
static const BYTE laurence[] = {
    'l','a','u','r','e','n','c','e','\r','\n'
};
static const BYTE comp_lzx[] = {
    0x53, 0x5a, 0x44, 0x44, 0x88, 0xf0, 0x27, 0x33, 0x41, 0x00, 0x0e, 0x00, 0x00, 0x00, 0xff, 0x00,
    0x00, 0x75, 0x6e, 0x63, 0x6f, 0x6d, 0x70, 0x3f, 0x72, 0x65, 0x73, 0x73, 0x65, 0x64
};
static const BYTE comp_zip[] = {
    0x50, 0x4b, 0x03, 0x04, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbd, 0xae, 0x81, 0x36, 0x75, 0x11,
    0x2c, 0x1b, 0x0e, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x04, 0x00, 0x15, 0x00, 0x77, 0x69,
    0x6e, 0x65, 0x55, 0x54, 0x09, 0x00, 0x03, 0xd6, 0x0d, 0x10, 0x46, 0xfd, 0x0d, 0x10, 0x46, 0x55,
    0x78, 0x04, 0x00, 0xe8, 0x03, 0xe8, 0x03, 0x00, 0x00, 0x75, 0x6e, 0x63, 0x6f, 0x6d, 0x70, 0x72,
    0x65, 0x73, 0x73, 0x65, 0x64, 0x50, 0x4b, 0x01, 0x02, 0x17, 0x03, 0x0a, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xbd, 0xae, 0x81, 0x36, 0x75, 0x11, 0x2c, 0x1b, 0x0e, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00,
    0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa4, 0x81, 0x00,
    0x00, 0x00, 0x00, 0x77, 0x69, 0x6e, 0x65, 0x55, 0x54, 0x05, 0x00, 0x03, 0xd6, 0x0d, 0x10, 0x46,
    0x55, 0x78, 0x00, 0x00, 0x50, 0x4b, 0x05, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x3f, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const BYTE comp_cab_lzx[] = {
    0x4d, 0x53, 0x43, 0x46, 0x00, 0x00, 0x00, 0x00, 0x6b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x0f, 0x0e, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x36, 0x86, 0x72, 0x20, 0x00, 0x77, 0x69, 0x6e, 0x65,
    0x00, 0x19, 0xd0, 0x1a, 0xe3, 0x22, 0x00, 0x0e, 0x00, 0x5b, 0x80, 0x80, 0x8d, 0x00, 0x30, 0xe0,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x75, 0x6e, 0x63,
    0x6f, 0x6d, 0x70, 0x72, 0x65, 0x73, 0x73, 0x65, 0x64, 0x0d, 0x0a
};
static const BYTE comp_cab_zip[] =  {
    0x4d, 0x53, 0x43, 0x46, 0x00, 0x00, 0x00, 0x00, 0x5b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x0e, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0x36, 0x2f, 0xa5, 0x20, 0x00, 0x77, 0x69, 0x6e, 0x65,
    0x00, 0x7c, 0x80, 0x26, 0x2b, 0x12, 0x00, 0x0e, 0x00, 0x43, 0x4b, 0x2b, 0xcd, 0x4b, 0xce, 0xcf,
    0x2d, 0x28, 0x4a, 0x2d, 0x2e, 0x4e, 0x4d, 0xe1, 0xe5, 0x02, 0x00
};
static const BYTE comp_cab_zip_multi[] = {
    0x4d, 0x53, 0x43, 0x46, 0x00, 0x00, 0x00, 0x00, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x01, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x71, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x0a, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd1, 0x38, 0xf0, 0x48, 0x20, 0x00, 0x74, 0x72, 0x69, 0x73,
    0x74, 0x72, 0x61, 0x6d, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd1,
    0x38, 0xf0, 0x48, 0x20, 0x00, 0x77, 0x69, 0x6e, 0x65, 0x00, 0x08, 0x00, 0x00, 0x00, 0x18, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xd1, 0x38, 0xf0, 0x48, 0x20, 0x00, 0x73, 0x68, 0x61, 0x6e, 0x64, 0x79,
    0x00, 0x67, 0x2c, 0x03, 0x85, 0x23, 0x00, 0x20, 0x00, 0x43, 0x4b, 0xcb, 0x49, 0x2c, 0x2d, 0x4a,
    0xcd, 0x4b, 0x4e, 0xe5, 0xe5, 0x2a, 0xcd, 0x4b, 0xce, 0xcf, 0x2d, 0x28, 0x4a, 0x2d, 0x2e, 0x4e,
    0x4d, 0xe1, 0xe5, 0x2a, 0x2e, 0x49, 0x2d, 0xca, 0x03, 0x8a, 0x02, 0x00
};

static void test_SetupGetFileCompressionInfo(void)
{
    DWORD ret, source_size, target_size;
    char source[MAX_PATH], temp[MAX_PATH], *name;
    UINT type;

    GetTempPathA(sizeof(temp), temp);
    GetTempFileNameA(temp, "fci", 0, source);

    create_source_file(source, uncompressed, sizeof(uncompressed));

    ret = SetupGetFileCompressionInfoA(NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "SetupGetFileCompressionInfo failed unexpectedly\n");

    ret = SetupGetFileCompressionInfoA(source, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "SetupGetFileCompressionInfo failed unexpectedly\n");

    ret = SetupGetFileCompressionInfoA(source, &name, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "SetupGetFileCompressionInfo failed unexpectedly\n");

    ret = SetupGetFileCompressionInfoA(source, &name, &source_size, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "SetupGetFileCompressionInfo failed unexpectedly\n");

    ret = SetupGetFileCompressionInfoA(source, &name, &source_size, &target_size, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "SetupGetFileCompressionInfo failed unexpectedly\n");

    name = NULL;
    source_size = target_size = 0;
    type = 5;

    ret = SetupGetFileCompressionInfoA(source, &name, &source_size, &target_size, &type);
    ok(!ret, "SetupGetFileCompressionInfo failed unexpectedly\n");
    ok(name && !lstrcmpA(name, source), "got %s, expected %s\n", name, source);
    ok(source_size == sizeof(uncompressed), "got %ld\n", source_size);
    ok(target_size == sizeof(uncompressed), "got %ld\n", target_size);
    ok(type == FILE_COMPRESSION_NONE, "got %d, expected FILE_COMPRESSION_NONE\n", type);

    pMyFree(name);
    DeleteFileA(source);
}

static void test_SetupGetFileCompressionInfoEx(void)
{
    BOOL ret;
    DWORD required_len, source_size, target_size;
    char source[MAX_PATH], temp[MAX_PATH], name[MAX_PATH];
    UINT type;

    GetTempPathA(sizeof(temp), temp);
    GetTempFileNameA(temp, "doc", 0, source);

    ret = pSetupGetFileCompressionInfoExA(NULL, NULL, 0, NULL, NULL, NULL, NULL);
    ok(!ret, "SetupGetFileCompressionInfoEx succeeded unexpectedly\n");

    ret = pSetupGetFileCompressionInfoExA(source, NULL, 0, NULL, NULL, NULL, NULL);
    ok(!ret, "SetupGetFileCompressionInfoEx succeeded unexpectedly\n");

    ret = pSetupGetFileCompressionInfoExA(source, NULL, 0, &required_len, NULL, NULL, NULL);
    ok(!ret, "SetupGetFileCompressionInfoEx succeeded unexpectedly\n");
    ok(required_len == lstrlenA(source) + 1, "got %ld, expected %d\n", required_len, lstrlenA(source) + 1);

    create_source_file(source, comp_lzx, sizeof(comp_lzx));

    ret = pSetupGetFileCompressionInfoExA(source, name, sizeof(name), &required_len, &source_size, &target_size, &type);
    ok(ret, "SetupGetFileCompressionInfoEx failed unexpectedly: %d\n", ret);
    ok(!lstrcmpA(name, source), "got %s, expected %s\n", name, source);
    ok(required_len == lstrlenA(source) + 1, "got %ld, expected %d\n", required_len, lstrlenA(source) + 1);
    ok(source_size == sizeof(comp_lzx), "got %ld\n", source_size);
    ok(target_size == sizeof(uncompressed), "got %ld\n", target_size);
    ok(type == FILE_COMPRESSION_WINLZA, "got %d, expected FILE_COMPRESSION_WINLZA\n", type);
    DeleteFileA(source);

    create_source_file(source, comp_zip, sizeof(comp_zip));

    ret = pSetupGetFileCompressionInfoExA(source, name, sizeof(name), &required_len, &source_size, &target_size, &type);
    ok(ret, "SetupGetFileCompressionInfoEx failed unexpectedly: %d\n", ret);
    ok(!lstrcmpA(name, source), "got %s, expected %s\n", name, source);
    ok(required_len == lstrlenA(source) + 1, "got %ld, expected %d\n", required_len, lstrlenA(source) + 1);
    ok(source_size == sizeof(comp_zip), "got %ld\n", source_size);
    ok(target_size == sizeof(comp_zip), "got %ld\n", target_size);
    ok(type == FILE_COMPRESSION_NONE, "got %d, expected FILE_COMPRESSION_NONE\n", type);
    DeleteFileA(source);

    create_source_file(source, comp_cab_lzx, sizeof(comp_cab_lzx));

    ret = pSetupGetFileCompressionInfoExA(source, name, sizeof(name), &required_len, &source_size, &target_size, &type);
    ok(ret, "SetupGetFileCompressionInfoEx failed unexpectedly: %d\n", ret);
    ok(!lstrcmpA(name, source), "got %s, expected %s\n", name, source);
    ok(required_len == lstrlenA(source) + 1, "got %ld, expected %d\n", required_len, lstrlenA(source) + 1);
    ok(source_size == sizeof(comp_cab_lzx), "got %ld\n", source_size);
    ok(target_size == sizeof(uncompressed), "got %ld\n", target_size);
    ok(type == FILE_COMPRESSION_MSZIP, "got %d, expected FILE_COMPRESSION_MSZIP\n", type);
    DeleteFileA(source);

    create_source_file(source, comp_cab_zip, sizeof(comp_cab_zip));

    ret = pSetupGetFileCompressionInfoExA(source, name, sizeof(name), &required_len, &source_size, &target_size, &type);
    ok(ret, "SetupGetFileCompressionInfoEx failed unexpectedly: %d\n", ret);
    ok(!lstrcmpA(name, source), "got %s, expected %s\n", name, source);
    ok(required_len == lstrlenA(source) + 1, "got %ld, expected %d\n", required_len, lstrlenA(source) + 1);
    ok(source_size == sizeof(comp_cab_zip), "got %ld\n", source_size);
    ok(target_size == sizeof(uncompressed), "got %ld\n", target_size);
    ok(type == FILE_COMPRESSION_MSZIP, "got %d, expected FILE_COMPRESSION_MSZIP\n", type);
    DeleteFileA(source);
}

static void test_SetupDecompressOrCopyFile(void)
{
    DWORD ret;
    char source[MAX_PATH], target[MAX_PATH], temp[MAX_PATH], *p;
    UINT type;
    int i;

    const struct
    {
        PCSTR source;
        PCSTR target;
        PUINT type;
    } invalid_parameters[] =
    {
        {NULL,   NULL,   NULL},
        {NULL,   NULL,   &type},
        {NULL,   target, NULL},
        {NULL,   target, &type},
        {source, NULL,   NULL},
        {source, NULL,   &type},
    };

    const struct
    {
        const char *filename;
        const BYTE *expected_buffer;
        const size_t buffer_size;
    } zip_multi_tests[] =
    {
        {"tristram",     laurence, sizeof(laurence)},
        {"tristram.txt", laurence, sizeof(laurence)},
        {"wine",         laurence, sizeof(laurence)},
        {"wine.txt",     laurence, sizeof(laurence)},
        {"shandy",       laurence, sizeof(laurence)},
        {"shandy.txt",   laurence, sizeof(laurence)},
        {"deadbeef",     laurence, sizeof(laurence)},
        {"deadbeef.txt", laurence, sizeof(laurence)},
    };

    GetTempPathA(sizeof(temp), temp);
    GetTempFileNameA(temp, "doc", 0, source);
    GetTempFileNameA(temp, "doc", 0, target);

    /* parameter tests */

    create_source_file(source, uncompressed, sizeof(uncompressed));

    for (i = 0; i < ARRAY_SIZE(invalid_parameters); i++)
    {
        type = FILE_COMPRESSION_NONE;
        ret = SetupDecompressOrCopyFileA(invalid_parameters[i].source,
                                         invalid_parameters[i].target,
                                         invalid_parameters[i].type);
        ok(ret == ERROR_INVALID_PARAMETER,
           "[%d] Expected SetupDecompressOrCopyFileA to return ERROR_INVALID_PARAMETER, got %lu\n",
           i, ret);

        /* try an invalid compression type */
        type = 5;
        ret = SetupDecompressOrCopyFileA(invalid_parameters[i].source,
                                         invalid_parameters[i].target,
                                         invalid_parameters[i].type);
        ok(ret == ERROR_INVALID_PARAMETER,
           "[%d] Expected SetupDecompressOrCopyFileA to return ERROR_INVALID_PARAMETER, got %lu\n",
           i, ret);
    }

    type = 5; /* try an invalid compression type */
    ret = SetupDecompressOrCopyFileA(source, target, &type);
    ok(ret == ERROR_INVALID_PARAMETER, "SetupDecompressOrCopyFile failed unexpectedly\n");

    DeleteFileA(target);

    /* no compression tests */

    ret = SetupDecompressOrCopyFileA(source, target, NULL);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %ld\n", ret);
    ok(compare_file_data(target, uncompressed, sizeof(uncompressed)), "incorrect target file\n");

    /* try overwriting existing file */
    ret = SetupDecompressOrCopyFileA(source, target, NULL);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %ld\n", ret);
    DeleteFileA(target);

    type = FILE_COMPRESSION_NONE;
    ret = SetupDecompressOrCopyFileA(source, target, &type);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %ld\n", ret);
    ok(compare_file_data(target, uncompressed, sizeof(uncompressed)), "incorrect target file\n");
    DeleteFileA(target);

    type = FILE_COMPRESSION_WINLZA;
    ret = SetupDecompressOrCopyFileA(source, target, &type);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %ld\n", ret);
    ok(compare_file_data(target, uncompressed, sizeof(uncompressed)), "incorrect target file\n");
    DeleteFileA(target);

    /* lz compression tests */

    create_source_file(source, comp_lzx, sizeof(comp_lzx));

    ret = SetupDecompressOrCopyFileA(source, target, NULL);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %ld\n", ret);
    DeleteFileA(target);

    /* zip compression tests */

    create_source_file(source, comp_zip, sizeof(comp_zip));

    ret = SetupDecompressOrCopyFileA(source, target, NULL);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %ld\n", ret);
    ok(compare_file_data(target, comp_zip, sizeof(comp_zip)), "incorrect target file\n");
    DeleteFileA(target);

    /* cabinet compression tests */

    create_source_file(source, comp_cab_zip, sizeof(comp_cab_zip));

    p = strrchr(target, '\\');
    lstrcpyA(p + 1, "wine");

    ret = SetupDecompressOrCopyFileA(source, target, NULL);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %ld\n", ret);
    ok(compare_file_data(target, uncompressed, sizeof(uncompressed)), "incorrect target file\n");

    /* try overwriting existing file */
    ret = SetupDecompressOrCopyFileA(source, target, NULL);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %ld\n", ret);

    /* try zip compression */
    type = FILE_COMPRESSION_MSZIP;
    ret = SetupDecompressOrCopyFileA(source, target, &type);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %ld\n", ret);
    ok(compare_file_data(target, uncompressed, sizeof(uncompressed)), "incorrect target file\n");

    /* try no compression */
    type = FILE_COMPRESSION_NONE;
    ret = SetupDecompressOrCopyFileA(source, target, &type);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %ld\n", ret);
    ok(compare_file_data(target, comp_cab_zip, sizeof(comp_cab_zip)), "incorrect target file\n");

    /* Show that SetupDecompressOrCopyFileA simply extracts the first file it
     * finds within the compressed cabinet. Contents are:
     * tristram -> "laurence\r\n"
     * wine     -> "uncompressed\r\n"
     * shandy   -> "sterne\r\n" */

    create_source_file(source, comp_cab_zip_multi, sizeof(comp_cab_zip_multi));

    p = strrchr(target, '\\');

    for (i = 0; i < ARRAY_SIZE(zip_multi_tests); i++)
    {
        lstrcpyA(p + 1, zip_multi_tests[i].filename);

        ret = SetupDecompressOrCopyFileA(source, target, NULL);
        ok(!ret, "[%d] SetupDecompressOrCopyFile failed unexpectedly: %ld\n", i, ret);
        ok(compare_file_data(target, zip_multi_tests[i].expected_buffer, zip_multi_tests[i].buffer_size),
           "[%d] incorrect target file\n", i);
        DeleteFileA(target);
    }

    DeleteFileA(source);
}

static void test_SetupUninstallOEMInf(void)
{
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = SetupUninstallOEMInfA(NULL, 0, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %08lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupUninstallOEMInfA("", 0, NULL);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    ret = SetupUninstallOEMInfA("nonexistent.inf", 0, NULL);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    }
}

struct default_callback_context
{
    DWORD     magic;
    HWND      owner;
    DWORD     unk1[4];
    DWORD_PTR unk2[7];
    HWND      progress;
    UINT      message;
    DWORD_PTR unk3[5];
};

static void test_defaultcallback(void)
{
    struct default_callback_context *ctxt;
    static const DWORD magic = 0x43515053; /* "SPQC" */
    HWND owner, progress;

    owner = (HWND)0x123;
    progress = (HWND)0x456;
    ctxt = SetupInitDefaultQueueCallbackEx(owner, progress, WM_USER, 0, NULL);
    ok(ctxt != NULL, "got %p\n", ctxt);

    ok(ctxt->magic == magic || broken(ctxt->magic != magic) /* win2000 */, "got magic 0x%08lx\n", ctxt->magic);
    if (ctxt->magic == magic)
    {
        ok(ctxt->owner == owner, "got %p, expected %p\n", ctxt->owner, owner);
        ok(ctxt->progress == progress, "got %p, expected %p\n", ctxt->progress, progress);
        ok(ctxt->message == WM_USER, "got %d, expected %d\n", ctxt->message, WM_USER);
        SetupTermDefaultQueueCallback(ctxt);
    }
    else
    {
        win_skip("Skipping tests on old systems.\n");
        SetupTermDefaultQueueCallback(ctxt);
        return;
    }

    ctxt = SetupInitDefaultQueueCallback(owner);
    ok(ctxt->magic == magic, "got magic 0x%08lx\n", ctxt->magic);
    ok(ctxt->owner == owner, "got %p, expected %p\n", ctxt->owner, owner);
    ok(ctxt->progress == NULL, "got %p, expected %p\n", ctxt->progress, progress);
    ok(ctxt->message == 0, "got %d\n", ctxt->message);
    SetupTermDefaultQueueCallback(ctxt);
}

static void test_SetupLogError(void)
{
    BOOL ret;
    DWORD error;

    SetLastError(0xdeadbeef);
    ret = SetupLogErrorA("Test without opening\r\n", LogSevInformation);
    error = GetLastError();
    ok(!ret, "SetupLogError succeeded\n");
    ok(error == ERROR_FILE_INVALID, "got wrong error: %ld\n", error);

    SetLastError(0xdeadbeef);
    ret = SetupOpenLog(FALSE);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("SetupOpenLog() failed on insufficient permissions\n");
        return;
    }
    ok(ret, "SetupOpenLog failed, error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupLogErrorA("Test with wrong log severity\r\n", LogSevMaximum);
    error = GetLastError();
    ok(!ret, "SetupLogError succeeded\n");
    ok(error == 0xdeadbeef, "got wrong error: %ld\n", error);
    ret = SetupLogErrorA("Test without EOL", LogSevInformation);
    ok(ret, "SetupLogError failed\n");

    SetLastError(0xdeadbeef);
    ret = SetupLogErrorA(NULL, LogSevInformation);
    ok(ret || broken(!ret && GetLastError() == ERROR_INVALID_PARAMETER /* Win Vista+ */),
        "SetupLogError failed: %08lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupOpenLog(FALSE);
    ok(ret, "SetupOpenLog failed, error %ld\n", GetLastError());

    SetupCloseLog();
}

static void test_CM_Get_Version(void)
{
    WORD ret;

    ret = CM_Get_Version();
    ok(ret == 0x0400, "got version %#x\n", ret);
}

#define check_device_interface(a, b) _check_device_interface(__LINE__, a, b)
static void _check_device_interface(int line, const char *instance_id, const GUID *guid)
{
    SP_DEVICE_INTERFACE_DATA iface_data = {sizeof(iface_data)};
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    HDEVINFO devinfo;
    BOOL ret;

    devinfo = SetupDiGetClassDevsA(guid, instance_id, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    ret = SetupDiEnumDeviceInfo(devinfo, 0, &device_data);
    ok_(__FILE__, line)(ret || broken(!ret) /* <= Win7 */,
                        "SetupDiEnumDeviceInfo failed, error %lu.\n", GetLastError());
    if (!ret)
    {
        SetupDiDestroyDeviceInfoList(devinfo);
        return;
    }
    ret = SetupDiEnumDeviceInterfaces(devinfo, &device_data, guid, 0, &iface_data);
    ok_(__FILE__, line)(ret, "SetupDiEnumDeviceInterfaces failed, error %lu.\n", GetLastError());
    ok_(__FILE__, line)(IsEqualGUID(&iface_data.InterfaceClassGuid, guid),
                        "Expected guid %s, got %s.\n", wine_dbgstr_guid(guid),
                        wine_dbgstr_guid(&iface_data.InterfaceClassGuid));

    SetupDiDestroyDeviceInfoList(devinfo);
}

static void test_device_interfaces(void)
{
    SP_DEVICE_INTERFACE_DATA iface_data = {sizeof(iface_data)};
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    char instance_id[MAX_DEVICE_ID_LEN];
    DWORD device_idx = 0, size, error;
    HDEVINFO devinfo;
    BOOL ret;

    /* GPUs */
    devinfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, NULL, NULL, DIGCF_PRESENT);
    ok(devinfo != INVALID_HANDLE_VALUE, "SetupDiGetClassDevsW failed, error %lu.\n", GetLastError());
    while ((ret = SetupDiEnumDeviceInfo(devinfo, device_idx, &device_data)))
    {
        ret = SetupDiGetDeviceInstanceIdA(devinfo, &device_data, instance_id,
                                          ARRAY_SIZE(instance_id), &size);
        ok(ret, "SetupDiGetDeviceInstanceIdA failed, error %lu.\n", GetLastError());

        winetest_push_context("GPU %ld", device_idx);

        check_device_interface(instance_id, &GUID_DEVINTERFACE_DISPLAY_ADAPTER);
        check_device_interface(instance_id, &GUID_DISPLAY_DEVICE_ARRIVAL);

        winetest_pop_context();
        ++device_idx;
    }
    error = GetLastError();
    ok(error == ERROR_NO_MORE_ITEMS, "Expected error %u, got %lu.\n", ERROR_NO_MORE_ITEMS, error);
    ok(device_idx > 0, "Expected at least one GPU.\n");
    SetupDiDestroyDeviceInfoList(devinfo);

    /* Monitors */
    device_idx = 0;
    devinfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_MONITOR, L"DISPLAY", NULL, DIGCF_PRESENT);
    ok(devinfo != INVALID_HANDLE_VALUE, "SetupDiGetClassDevsW failed, error %lu.\n", GetLastError());
    while ((ret = SetupDiEnumDeviceInfo(devinfo, device_idx, &device_data)))
    {
        ret = SetupDiGetDeviceInstanceIdA(devinfo, &device_data, instance_id,
                                          ARRAY_SIZE(instance_id), &size);
        ok(ret, "SetupDiGetDeviceInstanceIdA failed, error %lu.\n", GetLastError());

        winetest_push_context("Monitor %ld", device_idx);

        check_device_interface(instance_id, &GUID_DEVINTERFACE_MONITOR);

        winetest_pop_context();
        ++device_idx;
    }
    error = GetLastError();
    ok(error == ERROR_NO_MORE_ITEMS, "Expected error %u, got %lu.\n", ERROR_NO_MORE_ITEMS, error);
    ok(device_idx > 0 || broken(device_idx == 0) /* w7u_2qxl TestBot */,
       "Expected at least one monitor.\n");
    SetupDiDestroyDeviceInfoList(devinfo);
}

START_TEST(misc)
{
    HMODULE hsetupapi = GetModuleHandleA("setupapi.dll");

    pMyFree = (void*)GetProcAddress(hsetupapi, "MyFree");
    pSetupGetFileCompressionInfoExA = (void*)GetProcAddress(hsetupapi, "SetupGetFileCompressionInfoExA");
    pSetupQueryInfOriginalFileInformationA = (void*)GetProcAddress(hsetupapi, "SetupQueryInfOriginalFileInformationA");

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);

    test_SetupCopyOEMInf();
    test_SetupGetFileCompressionInfo();

    if (pSetupGetFileCompressionInfoExA)
        test_SetupGetFileCompressionInfoEx();
    else
        win_skip("SetupGetFileCompressionInfoExA is not available\n");

    test_SetupDecompressOrCopyFile();
    test_SetupUninstallOEMInf();
    test_defaultcallback();
    test_SetupLogError();
    test_CM_Get_Version();
    test_device_interfaces();
}
