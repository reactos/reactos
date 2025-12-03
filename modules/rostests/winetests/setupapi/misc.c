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
#include "ntddvdeo.h"
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
    buffer = malloc(size);
    if (buffer)
    {
        ReadFile(handle, buffer, size, &read, NULL);
        if (read == size && !memcmp(data, buffer, size)) ret = TRUE;
        free(buffer);
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
    ok(!ret, "Expected failure\n");
    todo_wine ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupUninstallOEMInfA("nonexistent.inf", 0, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
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

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);

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
