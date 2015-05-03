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
#include "setupapi.h"

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

static BOOL (WINAPI *pSetupGetFileCompressionInfoExA)(PCSTR, PSTR, DWORD, PDWORD, PDWORD, PDWORD, PUINT);
static BOOL (WINAPI *pSetupCopyOEMInfA)(PCSTR, PCSTR, DWORD, DWORD, PSTR, DWORD, PDWORD, PSTR *);
static BOOL (WINAPI *pSetupQueryInfOriginalFileInformationA)(PSP_INF_INFORMATION, UINT, PSP_ALTPLATFORM_INFO, PSP_ORIGINAL_FILE_INFO_A);
static BOOL (WINAPI *pSetupUninstallOEMInfA)(PCSTR, DWORD, PVOID);

static void create_inf_file(LPCSTR filename)
{
    DWORD dwNumberOfBytesWritten;
    HANDLE hf = CreateFileA(filename, GENERIC_WRITE, 0, NULL,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    static const char data[] =
        "[Version]\n"
        "Signature=\"$Chicago$\"\n"
        "AdvancedINF=2.5\n"
        "[DefaultInstall]\n"
        "RegisterOCXs=RegisterOCXsSection\n"
        "[RegisterOCXsSection]\n"
        "%%11%%\\ole32.dll\n";

    WriteFile(hf, data, sizeof(data) - 1, &dwNumberOfBytesWritten, NULL);
    CloseHandle(hf);
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

static BOOL check_format(LPSTR path, LPSTR inf)
{
    CHAR check[MAX_PATH];
    BOOL res;

    static const CHAR format[] = "\\INF\\oem";

    GetWindowsDirectoryA(check, MAX_PATH);
    strcat(check, format);
    res = CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, check, -1, path, strlen(check)) == CSTR_EQUAL &&
          path[strlen(check)] != '\\';

    return (!inf) ? res : res && (inf == path + strlen(check) - 3);
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
    ok(hinf != NULL, "SetupOpenInfFileA failed with error %d\n", GetLastError());

    res = SetupGetInfInformationA(hinf, INFINFO_INF_SPEC_IS_HINF, NULL, 0, &size);
    ok(res, "SetupGetInfInformation failed with error %d\n", GetLastError());

    pspii = HeapAlloc(GetProcessHeap(), 0, size);

    res = SetupGetInfInformationA(hinf, INFINFO_INF_SPEC_IS_HINF, pspii, size, NULL);
    ok(res, "SetupGetInfInformation failed with error %d\n", GetLastError());

    spofi.cbSize = 0;
    res = pSetupQueryInfOriginalFileInformationA(pspii, 0, NULL, &spofi);
    ok(!res && GetLastError() == ERROR_INVALID_USER_BUFFER,
        "SetupQueryInfOriginalFileInformationA should have failed with ERROR_INVALID_USER_BUFFER instead of %d\n", GetLastError());

    spofi.cbSize = sizeof(spofi);
    res = pSetupQueryInfOriginalFileInformationA(pspii, 0, NULL, &spofi);
    ok(res, "SetupQueryInfOriginalFileInformationA failed with error %d\n", GetLastError());
    ok(!spofi.OriginalCatalogName[0], "spofi.OriginalCatalogName should have been \"\" instead of \"%s\"\n", spofi.OriginalCatalogName);
    todo_wine
    ok(!strcmp(original, spofi.OriginalInfName), "spofi.OriginalInfName of %s didn't match real original name %s\n", spofi.OriginalInfName, original);

    HeapFree(GetProcessHeap(), 0, pspii);

    SetupCloseInfFile(hinf);
}

static void test_SetupCopyOEMInf(void)
{
    CHAR toolong[MAX_PATH * 2];
    CHAR path[MAX_PATH], dest[MAX_PATH];
    CHAR tmpfile[MAX_PATH], dest_save[MAX_PATH];
    LPSTR inf = NULL;
    DWORD size;
    BOOL res;

    /* try NULL SourceInfFileName */
    SetLastError(0xdeadbeef);
    res = pSetupCopyOEMInfA(NULL, NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* try empty SourceInfFileName */
    SetLastError(0xdeadbeef);
    res = pSetupCopyOEMInfA("", NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND ||
       GetLastError() == ERROR_BAD_PATHNAME || /* Win98 */
       GetLastError() == ERROR_INVALID_PARAMETER, /* Vista, W2K8 */
       "Unexpected error : %d\n", GetLastError());

    /* try a relative nonexistent SourceInfFileName */
    SetLastError(0xdeadbeef);
    res = pSetupCopyOEMInfA("nonexistent", NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());

    /* try an absolute nonexistent SourceInfFileName */
    strcpy(path, CURR_DIR);
    strcat(path, "\\nonexistent");
    SetLastError(0xdeadbeef);
    res = pSetupCopyOEMInfA(path, NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());

    /* try a long SourceInfFileName */
    memset(toolong, 'a', MAX_PATH * 2);
    toolong[MAX_PATH * 2 - 1] = '\0';
    SetLastError(0xdeadbeef);
    res = pSetupCopyOEMInfA(toolong, NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND ||
       GetLastError() == ERROR_FILENAME_EXCED_RANGE, /* Win98 */
       "Expected ERROR_FILE_NOT_FOUND or ERROR_FILENAME_EXCED_RANGE, got %d\n", GetLastError());

    get_temp_filename(tmpfile);
    create_inf_file(tmpfile);

    /* try a relative SourceInfFileName */
    SetLastError(0xdeadbeef);
    res = pSetupCopyOEMInfA(tmpfile, NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE ||
       broken(res == TRUE), /* Win98 */
       "Expected FALSE, got %d\n", res);
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

    ok(GetLastError() == ERROR_FILE_NOT_FOUND ||
       broken(GetLastError() == ERROR_SUCCESS), /* Win98 */
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    ok(file_exists(tmpfile), "Expected tmpfile to exist\n");

    /* try SP_COPY_REPLACEONLY, dest does not exist */
    SetLastError(0xdeadbeef);
    res = pSetupCopyOEMInfA(path, NULL, SPOST_NONE, SP_COPY_REPLACEONLY, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    ok(file_exists(tmpfile), "Expected source inf to exist\n");

    /* try an absolute SourceInfFileName, without DestinationInfFileName */
    strcpy(path, CURR_DIR);
    strcat(path, "\\");
    strcat(path, tmpfile);
    SetLastError(0xdeadbeef);
    res = pSetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, NULL, 0, NULL, NULL);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    ok(file_exists(path), "Expected source inf to exist\n");

    /* try SP_COPY_REPLACEONLY, dest exists */
    SetLastError(0xdeadbeef);
    res = pSetupCopyOEMInfA(path, NULL, SPOST_NONE, SP_COPY_REPLACEONLY, NULL, 0, NULL, NULL);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    ok(file_exists(path), "Expected source inf to exist\n");

    /* try SP_COPY_NOOVERWRITE */
    SetLastError(0xdeadbeef);
    res = pSetupCopyOEMInfA(path, NULL, SPOST_NONE, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    ok(GetLastError() == ERROR_FILE_EXISTS,
       "Expected ERROR_FILE_EXISTS, got %d\n", GetLastError());

    /* get the DestinationInfFileName */
    SetLastError(0xdeadbeef);
    res = pSetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, MAX_PATH, NULL, NULL);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    ok(strlen(dest) != 0, "Expected a non-zero length string\n");
    ok(file_exists(dest), "Expected destination inf to exist\n");
    ok(check_format(dest, NULL), "Expected %%windir%%\\inf\\OEMx.inf, got %s\n", dest);
    ok(file_exists(path), "Expected source inf to exist\n");

    strcpy(dest_save, dest);
    DeleteFileA(dest_save);

    /* get the DestinationInfFileName, DestinationInfFileNameSize is too small
     *   - inf is still copied
     */
    strcpy(dest, "aaa");
    size = 0;
    SetLastError(0xdeadbeef);
    res = pSetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, 5, &size, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(file_exists(path), "Expected source inf to exist\n");
    ok(file_exists(dest_save), "Expected dest inf to exist\n");
    ok(!strcmp(dest, "aaa"), "Expected dest to be unchanged\n");
    ok(size == strlen(dest_save) + 1, "Expected size to be lstrlen(dest_save) + 1\n");

    /* get the DestinationInfFileName and DestinationInfFileNameSize */
    SetLastError(0xdeadbeef);
    res = pSetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, MAX_PATH, &size, NULL);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    ok(lstrlenA(dest) + 1 == size, "Expected sizes to match, got (%d, %d)\n", lstrlenA(dest), size);
    ok(file_exists(dest), "Expected destination inf to exist\n");
    ok(check_format(dest, NULL), "Expected %%windir%%\\inf\\OEMx.inf, got %s\n", dest);
    ok(file_exists(path), "Expected source inf to exist\n");
    ok(size == lstrlenA(dest_save) + 1, "Expected size to be lstrlen(dest_save) + 1\n");

    test_original_file_name(strrchr(path, '\\') + 1, dest);

    /* get the DestinationInfFileName, DestinationInfFileNameSize, and DestinationInfFileNameComponent */
    SetLastError(0xdeadbeef);
    res = pSetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, MAX_PATH, &size, &inf);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    ok(lstrlenA(dest) + 1 == size, "Expected sizes to match, got (%d, %d)\n", lstrlenA(dest), size);
    ok(file_exists(dest), "Expected destination inf to exist\n");
    ok((inf && inf[0] != 0) ||
       broken(!inf), /* Win98 */
       "Expected inf to point to the filename\n");
    ok(check_format(dest, inf), "Expected %%windir%%\\inf\\OEMx.inf, got %s\n", dest);
    ok(file_exists(path), "Expected source inf to exist\n");
    ok(size == lstrlenA(dest_save) + 1, "Expected size to be lstrlen(dest_save) + 1\n");

    /* try SP_COPY_DELETESOURCE */
    SetLastError(0xdeadbeef);
    res = pSetupCopyOEMInfA(path, NULL, SPOST_NONE, SP_COPY_DELETESOURCE, NULL, 0, NULL, NULL);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    ok(!file_exists(path), "Expected source inf to not exist\n");

    if (pSetupUninstallOEMInfA)
    {
        char pnf[MAX_PATH];
        char *pnffile;
        char *destfile = strrchr(dest, '\\') + 1;

        strcpy(pnf, dest);
        *(strrchr(pnf, '.') + 1) = 'p';
        pnffile = strrchr(pnf, '\\') + 1;

        SetLastError(0xdeadbeef);
        res = pSetupUninstallOEMInfA(destfile, 0, NULL);
        if(!res)
            res = pSetupUninstallOEMInfA(pnffile, 0, NULL);
        ok(res, "Failed to uninstall '%s'/'%s' : %d\n", destfile,
           pnffile, GetLastError());
        todo_wine ok(!file_exists(dest), "Expected inf '%s' to not exist\n", dest);
        if(file_exists(dest))
        {
            SetLastError(0xdeadbeef);
            res = DeleteFileA(dest);
            ok(res, "Failed to delete file '%s' : %d\n", dest, GetLastError());
        }
        ok(!file_exists(pnf), "Expected pnf '%s' to not exist\n", pnf);
        if(file_exists(pnf))
        {
            SetLastError(0xdeadbeef);
            res = DeleteFileA(pnf);
            ok(res, "Failed to delete file '%s' : %d\n", pnf, GetLastError());
        }
    }
    else
    {
        /* Win9x/WinMe */
        SetLastError(0xdeadbeef);
        res = DeleteFileA(dest);
        ok(res, "Failed to delete file '%s' : %d\n", dest, GetLastError());

        /* On WinMe we also need to remove the .pnf file */
        *(strrchr(dest, '.') + 1) = 'p';
        DeleteFileA(dest);
    }
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
    ok(source_size == sizeof(uncompressed), "got %d\n", source_size);
    ok(target_size == sizeof(uncompressed), "got %d\n", target_size);
    ok(type == FILE_COMPRESSION_NONE, "got %d, expected FILE_COMPRESSION_NONE\n", type);

    MyFree(name);
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
    ok(required_len == lstrlenA(source) + 1, "got %d, expected %d\n", required_len, lstrlenA(source) + 1);

    create_source_file(source, comp_lzx, sizeof(comp_lzx));

    ret = pSetupGetFileCompressionInfoExA(source, name, sizeof(name), &required_len, &source_size, &target_size, &type);
    ok(ret, "SetupGetFileCompressionInfoEx failed unexpectedly: %d\n", ret);
    ok(!lstrcmpA(name, source), "got %s, expected %s\n", name, source);
    ok(required_len == lstrlenA(source) + 1, "got %d, expected %d\n", required_len, lstrlenA(source) + 1);
    ok(source_size == sizeof(comp_lzx), "got %d\n", source_size);
    ok(target_size == sizeof(uncompressed), "got %d\n", target_size);
    ok(type == FILE_COMPRESSION_WINLZA, "got %d, expected FILE_COMPRESSION_WINLZA\n", type);
    DeleteFileA(source);

    create_source_file(source, comp_zip, sizeof(comp_zip));

    ret = pSetupGetFileCompressionInfoExA(source, name, sizeof(name), &required_len, &source_size, &target_size, &type);
    ok(ret, "SetupGetFileCompressionInfoEx failed unexpectedly: %d\n", ret);
    ok(!lstrcmpA(name, source), "got %s, expected %s\n", name, source);
    ok(required_len == lstrlenA(source) + 1, "got %d, expected %d\n", required_len, lstrlenA(source) + 1);
    ok(source_size == sizeof(comp_zip), "got %d\n", source_size);
    ok(target_size == sizeof(comp_zip), "got %d\n", target_size);
    ok(type == FILE_COMPRESSION_NONE, "got %d, expected FILE_COMPRESSION_NONE\n", type);
    DeleteFileA(source);

    create_source_file(source, comp_cab_lzx, sizeof(comp_cab_lzx));

    ret = pSetupGetFileCompressionInfoExA(source, name, sizeof(name), &required_len, &source_size, &target_size, &type);
    ok(ret, "SetupGetFileCompressionInfoEx failed unexpectedly: %d\n", ret);
    ok(!lstrcmpA(name, source), "got %s, expected %s\n", name, source);
    ok(required_len == lstrlenA(source) + 1, "got %d, expected %d\n", required_len, lstrlenA(source) + 1);
    ok(source_size == sizeof(comp_cab_lzx), "got %d\n", source_size);
    ok(target_size == sizeof(uncompressed), "got %d\n", target_size);
    ok(type == FILE_COMPRESSION_MSZIP, "got %d, expected FILE_COMPRESSION_MSZIP\n", type);
    DeleteFileA(source);

    create_source_file(source, comp_cab_zip, sizeof(comp_cab_zip));

    ret = pSetupGetFileCompressionInfoExA(source, name, sizeof(name), &required_len, &source_size, &target_size, &type);
    ok(ret, "SetupGetFileCompressionInfoEx failed unexpectedly: %d\n", ret);
    ok(!lstrcmpA(name, source), "got %s, expected %s\n", name, source);
    ok(required_len == lstrlenA(source) + 1, "got %d, expected %d\n", required_len, lstrlenA(source) + 1);
    ok(source_size == sizeof(comp_cab_zip), "got %d\n", source_size);
    ok(target_size == sizeof(uncompressed), "got %d\n", target_size);
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

    for (i = 0; i < sizeof(invalid_parameters)/sizeof(invalid_parameters[0]); i++)
    {
        type = FILE_COMPRESSION_NONE;
        ret = SetupDecompressOrCopyFileA(invalid_parameters[i].source,
                                         invalid_parameters[i].target,
                                         invalid_parameters[i].type);
        ok(ret == ERROR_INVALID_PARAMETER,
           "[%d] Expected SetupDecompressOrCopyFileA to return ERROR_INVALID_PARAMETER, got %u\n",
           i, ret);

        /* try an invalid compression type */
        type = 5;
        ret = SetupDecompressOrCopyFileA(invalid_parameters[i].source,
                                         invalid_parameters[i].target,
                                         invalid_parameters[i].type);
        ok(ret == ERROR_INVALID_PARAMETER,
           "[%d] Expected SetupDecompressOrCopyFileA to return ERROR_INVALID_PARAMETER, got %u\n",
           i, ret);
    }

    type = 5; /* try an invalid compression type */
    ret = SetupDecompressOrCopyFileA(source, target, &type);
    ok(ret == ERROR_INVALID_PARAMETER, "SetupDecompressOrCopyFile failed unexpectedly\n");

    DeleteFileA(target);

    /* no compression tests */

    ret = SetupDecompressOrCopyFileA(source, target, NULL);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %d\n", ret);
    ok(compare_file_data(target, uncompressed, sizeof(uncompressed)), "incorrect target file\n");

    /* try overwriting existing file */
    ret = SetupDecompressOrCopyFileA(source, target, NULL);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %d\n", ret);
    DeleteFileA(target);

    type = FILE_COMPRESSION_NONE;
    ret = SetupDecompressOrCopyFileA(source, target, &type);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %d\n", ret);
    ok(compare_file_data(target, uncompressed, sizeof(uncompressed)), "incorrect target file\n");
    DeleteFileA(target);

    type = FILE_COMPRESSION_WINLZA;
    ret = SetupDecompressOrCopyFileA(source, target, &type);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %d\n", ret);
    ok(compare_file_data(target, uncompressed, sizeof(uncompressed)), "incorrect target file\n");
    DeleteFileA(target);

    /* lz compression tests */

    create_source_file(source, comp_lzx, sizeof(comp_lzx));

    ret = SetupDecompressOrCopyFileA(source, target, NULL);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %d\n", ret);
    DeleteFileA(target);

    /* zip compression tests */

    create_source_file(source, comp_zip, sizeof(comp_zip));

    ret = SetupDecompressOrCopyFileA(source, target, NULL);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %d\n", ret);
    ok(compare_file_data(target, comp_zip, sizeof(comp_zip)), "incorrect target file\n");
    DeleteFileA(target);

    /* cabinet compression tests */

    create_source_file(source, comp_cab_zip, sizeof(comp_cab_zip));

    p = strrchr(target, '\\');
    lstrcpyA(p + 1, "wine");

    ret = SetupDecompressOrCopyFileA(source, target, NULL);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %d\n", ret);
    ok(compare_file_data(target, uncompressed, sizeof(uncompressed)), "incorrect target file\n");

    /* try overwriting existing file */
    ret = SetupDecompressOrCopyFileA(source, target, NULL);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %d\n", ret);

    /* try zip compression */
    type = FILE_COMPRESSION_MSZIP;
    ret = SetupDecompressOrCopyFileA(source, target, &type);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %d\n", ret);
    ok(compare_file_data(target, uncompressed, sizeof(uncompressed)), "incorrect target file\n");

    /* try no compression */
    type = FILE_COMPRESSION_NONE;
    ret = SetupDecompressOrCopyFileA(source, target, &type);
    ok(!ret, "SetupDecompressOrCopyFile failed unexpectedly: %d\n", ret);
    ok(compare_file_data(target, comp_cab_zip, sizeof(comp_cab_zip)), "incorrect target file\n");

    /* Show that SetupDecompressOrCopyFileA simply extracts the first file it
     * finds within the compressed cabinet. Contents are:
     * tristram -> "laurence\r\n"
     * wine     -> "uncompressed\r\n"
     * shandy   -> "sterne\r\n" */

    create_source_file(source, comp_cab_zip_multi, sizeof(comp_cab_zip_multi));

    p = strrchr(target, '\\');

    for (i = 0; i < sizeof(zip_multi_tests)/sizeof(zip_multi_tests[0]); i++)
    {
        lstrcpyA(p + 1, zip_multi_tests[i].filename);

        ret = SetupDecompressOrCopyFileA(source, target, NULL);
        ok(!ret, "[%d] SetupDecompressOrCopyFile failed unexpectedly: %d\n", i, ret);
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
    ret = pSetupUninstallOEMInfA(NULL, 0, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pSetupUninstallOEMInfA("", 0, NULL);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    ret = pSetupUninstallOEMInfA("nonexistent.inf", 0, NULL);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
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

    ok(ctxt->magic == magic || broken(ctxt->magic != magic) /* win2000 */, "got magic 0x%08x\n", ctxt->magic);
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
    ok(ctxt->magic == magic, "got magic 0x%08x\n", ctxt->magic);
    ok(ctxt->owner == owner, "got %p, expected %p\n", ctxt->owner, owner);
    ok(ctxt->progress == NULL, "got %p, expected %p\n", ctxt->progress, progress);
    ok(ctxt->message == 0, "got %d\n", ctxt->message);
    SetupTermDefaultQueueCallback(ctxt);
}

START_TEST(misc)
{
    HMODULE hsetupapi = GetModuleHandleA("setupapi.dll");

    pSetupGetFileCompressionInfoExA = (void*)GetProcAddress(hsetupapi, "SetupGetFileCompressionInfoExA");
    pSetupCopyOEMInfA = (void*)GetProcAddress(hsetupapi, "SetupCopyOEMInfA");
    pSetupQueryInfOriginalFileInformationA = (void*)GetProcAddress(hsetupapi, "SetupQueryInfOriginalFileInformationA");
    pSetupUninstallOEMInfA = (void*)GetProcAddress(hsetupapi, "SetupUninstallOEMInfA");

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);

    if (pSetupCopyOEMInfA)
        test_SetupCopyOEMInf();
    else
        win_skip("SetupCopyOEMInfA is not available\n");

    test_SetupGetFileCompressionInfo();

    if (pSetupGetFileCompressionInfoExA)
        test_SetupGetFileCompressionInfoEx();
    else
        win_skip("SetupGetFileCompressionInfoExA is not available\n");

    test_SetupDecompressOrCopyFile();

    if (pSetupUninstallOEMInfA)
        test_SetupUninstallOEMInf();
    else
        win_skip("SetupUninstallOEMInfA is not available\n");

    test_defaultcallback();
}
