/* Unit test suite for Ntdll directory functions
 *
 * Copyright 2007 Jeff Latimer
 * Copyright 2007 Andrey Turkin
 * Copyright 2008 Jeff Zaroyko
 * Copyright 2009 Dan Kegel
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
 *
 * NOTES
 * We use function pointers here as there is no import library for NTDLL on
 * windows.
 */

#include <stdio.h>
#include <stdarg.h>

#include "ntstatus.h"
/* Define WIN32_NO_STATUS so MSVC does not give us duplicate macro
 * definition errors when we get to winnt.h
 */
#define WIN32_NO_STATUS

#include "wine/test.h"
#include "winnls.h"
#include "winternl.h"

static NTSTATUS (WINAPI *pNtClose)( PHANDLE );
static NTSTATUS (WINAPI *pNtOpenFile)    ( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG );
static NTSTATUS (WINAPI *pNtQueryDirectoryFile)(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,
                                                PVOID,ULONG,FILE_INFORMATION_CLASS,BOOLEAN,PUNICODE_STRING,BOOLEAN);
static NTSTATUS (WINAPI *pNtQueryInformationFile)(HANDLE,PIO_STATUS_BLOCK,PVOID,LONG,FILE_INFORMATION_CLASS);
static NTSTATUS (WINAPI *pNtSetInformationFile)(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FILE_INFORMATION_CLASS);
static BOOLEAN  (WINAPI *pRtlCreateUnicodeStringFromAsciiz)(PUNICODE_STRING,LPCSTR);
static BOOL     (WINAPI *pRtlDosPathNameToNtPathName_U)( LPCWSTR, PUNICODE_STRING, PWSTR*, CURDIR* );
static VOID     (WINAPI *pRtlInitUnicodeString)( PUNICODE_STRING, LPCWSTR );
static VOID     (WINAPI *pRtlFreeUnicodeString)( PUNICODE_STRING );
static LONG     (WINAPI *pRtlCompareUnicodeString)( const UNICODE_STRING*, const UNICODE_STRING*,BOOLEAN );
static NTSTATUS (WINAPI *pRtlMultiByteToUnicodeN)( LPWSTR dst, DWORD dstlen, LPDWORD reslen,
                                                   LPCSTR src, DWORD srclen );
static NTSTATUS (WINAPI *pRtlWow64EnableFsRedirection)( BOOLEAN enable );
static NTSTATUS (WINAPI *pRtlWow64EnableFsRedirectionEx)( ULONG disable, ULONG *old_value );

/* The attribute sets to test */
static struct testfile_s {
    BOOL attr_done;           /* set if attributes were tested for this file already */
    const DWORD attr;         /* desired attribute */
    WCHAR name[20];           /* filename to use */
    const char *target;       /* what to point to (only for reparse pts) */
    const char *description;  /* for error messages */
    int nfound;               /* How many were found (expect 1) */
} testfiles[] = {
    { 0, FILE_ATTRIBUTE_NORMAL,    {'l','o','n','g','f','i','l','e','n','a','m','e','.','t','m','p'}, "normal" },
    { 0, FILE_ATTRIBUTE_NORMAL,    {'n','.','t','m','p',}, "normal" },
    { 0, FILE_ATTRIBUTE_HIDDEN,    {'h','.','t','m','p',}, "hidden" },
    { 0, FILE_ATTRIBUTE_SYSTEM,    {'s','.','t','m','p',}, "system" },
    { 0, FILE_ATTRIBUTE_DIRECTORY, {'d','.','t','m','p',}, "directory" },
    { 0, FILE_ATTRIBUTE_NORMAL,    {0xe9,'a','.','t','m','p'}, "normal" },
    { 0, FILE_ATTRIBUTE_NORMAL,    {0xc9,'b','.','t','m','p'}, "normal" },
    { 0, FILE_ATTRIBUTE_NORMAL,    {'e','a','.','t','m','p'},  "normal" },
    { 0, FILE_ATTRIBUTE_NORMAL,    {'e','a'},                  "normal" },
    { 0, FILE_ATTRIBUTE_DIRECTORY, {'.'},                  ". directory" },
    { 0, FILE_ATTRIBUTE_DIRECTORY, {'.','.'},              ".. directory" },
    { 0, FILE_ATTRIBUTE_NORMAL,    {'e','a','.','t','m','p','.','t','m','p'}, "normal" },
    { 0, FILE_ATTRIBUTE_NORMAL,    {'.','a'}, "normal" },
    { 0, FILE_ATTRIBUTE_NORMAL,    {'.','a','.','a'}, "normal" },
    { 0, FILE_ATTRIBUTE_NORMAL,    {'a','.'}, "normal" },
    { 0, FILE_ATTRIBUTE_NORMAL,    {'.','.','a'}, "normal" },
    { 0, FILE_ATTRIBUTE_NORMAL,    {'.','a','a'}, "normal" },
    { 0, FILE_ATTRIBUTE_NORMAL,    {'a','.', '.'}, "normal" },
};
static const int test_dir_count = ARRAY_SIZE(testfiles);
static const int max_test_dir_size = ARRAY_SIZE(testfiles) + 5;  /* size of above plus some for .. etc */

static const WCHAR dummyW[] = {'d','u','m','m','y',0};
static const WCHAR dotW[] = {'.',0};
static const WCHAR dotdotW[] = {'.','.',0};
static const WCHAR backslashW[] = {'\\',0};

/* Create a test directory full of attribute test files, clear counts */
static void set_up_attribute_test(const WCHAR *testdir)
{
    int i;
    BOOL ret;

    ret = CreateDirectoryW(testdir, NULL);
    ok(ret, "couldn't create dir %s, error %ld\n", wine_dbgstr_w(testdir), GetLastError());

    for (i=0; i < test_dir_count; i++) {
        WCHAR buf[MAX_PATH];

        if (lstrcmpW(testfiles[i].name, dotW) == 0 || lstrcmpW(testfiles[i].name, dotdotW) == 0)
            continue;
        lstrcpyW( buf, L"\\\\?\\" );
        lstrcatW( buf, testdir );
        lstrcatW( buf, backslashW );
        lstrcatW( buf, testfiles[i].name );
        if (testfiles[i].attr & FILE_ATTRIBUTE_DIRECTORY) {
            ret = CreateDirectoryW(buf, NULL);
            ok(ret, "couldn't create dir %s, error %ld\n", wine_dbgstr_w(buf), GetLastError());
        } else {
            HANDLE h = CreateFileW(buf,
                                   GENERIC_READ|GENERIC_WRITE,
                                   0, NULL, CREATE_ALWAYS,
                                   testfiles[i].attr, 0);
            ok( h != INVALID_HANDLE_VALUE, "failed to create temp file %s\n", wine_dbgstr_w(buf) );
            CloseHandle(h);
        }
    }
}

static void reset_found_files(void)
{
    int i;

    for (i = 0; i < test_dir_count; i++)
        testfiles[i].nfound = 0;
}

/* Remove the given test directory and the attribute test files, if any */
static void tear_down_attribute_test(const WCHAR *testdir)
{
    int i;

    for (i = 0; i < test_dir_count; i++) {
        int ret;
        WCHAR buf[MAX_PATH];
        if (lstrcmpW(testfiles[i].name, dotW) == 0 || lstrcmpW(testfiles[i].name, dotdotW) == 0)
            continue;
        lstrcpyW( buf, L"\\\\?\\" );
        lstrcatW( buf, testdir );
        lstrcatW( buf, backslashW );
        lstrcatW( buf, testfiles[i].name );
        if (testfiles[i].attr & FILE_ATTRIBUTE_DIRECTORY) {
            ret = RemoveDirectoryW(buf);
            ok(ret || (GetLastError() == ERROR_PATH_NOT_FOUND),
               "Failed to rmdir %s, error %ld\n", wine_dbgstr_w(buf), GetLastError());
        } else {
            ret = DeleteFileW(buf);
            ok(ret || (GetLastError() == ERROR_PATH_NOT_FOUND),
               "Failed to rm %s, error %ld\n", wine_dbgstr_w(buf), GetLastError());
        }
    }
    RemoveDirectoryW(testdir);
}

/* Match one found file against testfiles[], increment count if found */
static void tally_test_file(FILE_BOTH_DIRECTORY_INFORMATION *dir_info)
{
    int i;
    DWORD attribmask =
      (FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT);
    DWORD attrib = dir_info->FileAttributes & attribmask;
    WCHAR *nameW = dir_info->FileName;
    int namelen = dir_info->FileNameLength / sizeof(WCHAR);

    for (i = 0; i < test_dir_count; i++) {
        int len = lstrlenW(testfiles[i].name);
        if (namelen != len || memcmp(nameW, testfiles[i].name, len*sizeof(WCHAR)))
            continue;
        if (!testfiles[i].attr_done) {
            ok (attrib == (testfiles[i].attr & attribmask), "file %s: expected %s (%lx), got %lx\n",
                wine_dbgstr_w(testfiles[i].name), testfiles[i].description, testfiles[i].attr, attrib);
            testfiles[i].attr_done = TRUE;
        }
        testfiles[i].nfound++;
        break;
    }
    ok(i < test_dir_count, "unexpected file found %s\n", wine_dbgstr_wn(dir_info->FileName, namelen));
}

static void test_flags_NtQueryDirectoryFile(OBJECT_ATTRIBUTES *attr, const char *testdirA,
                                            UNICODE_STRING *mask,
                                            BOOLEAN single_entry, BOOLEAN restart_flag, BOOLEAN expect_empty)
{
    UNICODE_STRING dummy_mask;
    HANDLE dirh, new_dirh;
    IO_STATUS_BLOCK io;
    UINT data_pos, data_size;
    UINT data_len;    /* length of dir data */
    BYTE data[8192];  /* directory data */
    FILE_BOTH_DIRECTORY_INFORMATION *dir_info;
    NTSTATUS status;
    int numfiles;
    int i;

    reset_found_files();
    pRtlInitUnicodeString( &dummy_mask, dummyW );

    data_size = mask ? offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[256] ) : sizeof(data);

    /* Read the directory and note which files are found */
    status = pNtOpenFile( &dirh, SYNCHRONIZE | FILE_LIST_DIRECTORY, attr, &io, FILE_SHARE_READ,
                         FILE_SYNCHRONOUS_IO_NONALERT|FILE_OPEN_FOR_BACKUP_INTENT|FILE_DIRECTORY_FILE);
    ok (status == STATUS_SUCCESS, "failed to open dir '%s', ret 0x%lx, error %ld\n", testdirA, status, GetLastError());
    if (status != STATUS_SUCCESS) {
       skip("can't test if we can't open the directory\n");
       return;
    }

    io.Status = 0xdeadbeef;
    status = pNtQueryDirectoryFile( dirh, NULL, NULL, NULL, &io, data, data_size,
                                    FileBothDirectoryInformation, single_entry, mask, restart_flag );
    if (expect_empty)
    {
        ok( status == STATUS_NO_SUCH_FILE, "got %#lx.\n", status );
        pNtClose( dirh );
        return;
    }
    ok (status == STATUS_SUCCESS, "failed to query directory; status %lx\n", status);
    ok (io.Status == STATUS_SUCCESS, "failed to query directory; status %lx\n", io.Status);
    data_len = io.Information;
    ok (data_len >= sizeof(FILE_BOTH_DIRECTORY_INFORMATION), "not enough data in directory\n");

    DuplicateHandle( GetCurrentProcess(), dirh, GetCurrentProcess(), &new_dirh,
                     0, FALSE, DUPLICATE_SAME_ACCESS );
    pNtClose(dirh);

    data_pos = 0;
    numfiles = 0;
    while ((data_pos < data_len) && (numfiles < max_test_dir_size)) {
        dir_info = (FILE_BOTH_DIRECTORY_INFORMATION *)(data + data_pos);

        tally_test_file(dir_info);

        if (dir_info->NextEntryOffset == 0) {
            io.Status = 0xdeadbeef;
            status = pNtQueryDirectoryFile( new_dirh, 0, NULL, NULL, &io, data, data_size,
                                            FileBothDirectoryInformation, single_entry, &dummy_mask, FALSE );
            ok (io.Status == status, "wrong status %lx / %lx\n", status, io.Status);
            if (status == STATUS_NO_MORE_FILES) break;
            ok (status == STATUS_SUCCESS, "failed to query directory; status %lx\n", status);
            data_len = io.Information;
            if (data_len < sizeof(FILE_BOTH_DIRECTORY_INFORMATION))
                break;
            data_pos = 0;
        } else {
            data_pos += dir_info->NextEntryOffset;
        }
        numfiles++;
    }
    ok(numfiles < max_test_dir_size, "too many loops\n");

    if (mask && !wcspbrk( mask->Buffer, L"*?<\">" ))
        for (i = 0; i < test_dir_count; i++)
            ok(testfiles[i].nfound == (testfiles[i].name == mask->Buffer),
               "Wrong number %d of %s files found (single_entry=%d,mask=%s)\n",
               testfiles[i].nfound, testfiles[i].description, single_entry,
               wine_dbgstr_wn(mask->Buffer, mask->Length/sizeof(WCHAR) ));
    else if (!mask)
        for (i = 0; i < test_dir_count; i++)
            ok(testfiles[i].nfound == 1, "Wrong number %d of %s files found (single_entry=%d,restart=%d)\n",
               testfiles[i].nfound, testfiles[i].description, single_entry, restart_flag);
    pNtClose(new_dirh);
}

static void test_directory_sort( const WCHAR *testdir )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING ntdirname;
    IO_STATUS_BLOCK io;
    UINT data_pos, data_len, count;
    BYTE data[8192];
    WCHAR prev[MAX_PATH], name[MAX_PATH];
    UNICODE_STRING prev_str, name_str;
    FILE_BOTH_DIRECTORY_INFORMATION *info;
    NTSTATUS status;
    HANDLE handle;
    int res;

    if (!pRtlDosPathNameToNtPathName_U( testdir, &ntdirname, NULL, NULL ))
    {
        ok(0, "RtlDosPathNametoNtPathName_U failed\n");
        return;
    }
    InitializeObjectAttributes( &attr, &ntdirname, OBJ_CASE_INSENSITIVE, 0, NULL );
    status = pNtOpenFile( &handle, SYNCHRONIZE | FILE_LIST_DIRECTORY, &attr, &io, FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_DIRECTORY_FILE );
    ok(status == STATUS_SUCCESS, "failed to open dir %s\n", wine_dbgstr_w(testdir) );

    io.Status = 0xdeadbeef;
    status = pNtQueryDirectoryFile( handle, NULL, NULL, NULL, &io, data, sizeof(data),
                                    FileBothDirectoryInformation, FALSE, NULL, TRUE );
    ok( status == STATUS_SUCCESS, "failed to query directory; status %lx\n", status );
    ok( io.Status == STATUS_SUCCESS, "failed to query directory; status %lx\n", io.Status );
    data_len = io.Information;
    ok( data_len >= sizeof(FILE_BOTH_DIRECTORY_INFORMATION), "not enough data in directory\n" );
    data_pos = 0;
    count = 0;

    while (data_pos < data_len)
    {
        info = (FILE_BOTH_DIRECTORY_INFORMATION *)(data + data_pos);

        memcpy( name, info->FileName, info->FileNameLength );
        name[info->FileNameLength / sizeof(WCHAR)] = 0;
        switch (count)
        {
        case 0:  /* first entry must be '.' */
            ok( !lstrcmpW( name, dotW ), "wrong name %s\n", wine_dbgstr_w( name ));
            break;
        case 1:  /* second entry must be '..' */
            ok( !lstrcmpW( name, dotdotW ), "wrong name %s\n", wine_dbgstr_w( name ));
            break;
        case 2:  /* nothing to compare against */
            break;
        default:
            pRtlInitUnicodeString( &prev_str, prev );
            pRtlInitUnicodeString( &name_str, name );
            res = pRtlCompareUnicodeString( &prev_str, &name_str, TRUE );
            ok( res < 0, "wrong result %d %s %s\n", res, wine_dbgstr_w( prev ), wine_dbgstr_w( name ));
            break;
        }
        count++;
        lstrcpyW( prev, name );

        if (info->NextEntryOffset == 0)
        {
            io.Status = 0xdeadbeef;
            status = pNtQueryDirectoryFile( handle, 0, NULL, NULL, &io, data, sizeof(data),
                                            FileBothDirectoryInformation, FALSE, NULL, FALSE );
            ok (io.Status == status, "wrong status %lx / %lx\n", status, io.Status);
            if (status == STATUS_NO_MORE_FILES) break;
            ok( status == STATUS_SUCCESS, "failed to query directory; status %lx\n", status );
            data_len = io.Information;
            data_pos = 0;
        }
        else data_pos += info->NextEntryOffset;
    }

    pNtClose( handle );
    pRtlFreeUnicodeString( &ntdirname );
}

static void test_NtQueryDirectoryFile_classes( HANDLE handle, UNICODE_STRING *mask )
{
    IO_STATUS_BLOCK io;
    UINT data_size;
    ULONG data[256];
    NTSTATUS status;
    int class;

    for (class = 0; class < FileMaximumInformation; class++)
    {
        io.Status = 0xdeadbeef;
        io.Information = 0xdeadbeef;
        data_size = 0;
        memset( data, 0x55, sizeof(data) );

        status = pNtQueryDirectoryFile( handle, 0, NULL, NULL, &io, data, data_size,
                                        class, FALSE, mask, TRUE );
        ok( io.Status == 0xdeadbeef, "%u: wrong status %lx\n", class, io.Status );
        ok( io.Information == 0xdeadbeef, "%u: wrong info %Ix\n", class, io.Information );
        ok(data[0] == 0x55555555, "%u: wrong offset %lx\n",  class, data[0] );

        switch (class)
        {
        case FileIdGlobalTxDirectoryInformation:
        case FileIdExtdDirectoryInformation:
        case FileIdExtdBothDirectoryInformation:
            if (status == STATUS_INVALID_INFO_CLASS || status == STATUS_NOT_IMPLEMENTED) continue;
            /* fall through */
        case FileDirectoryInformation:
        case FileFullDirectoryInformation:
        case FileBothDirectoryInformation:
        case FileNamesInformation:
        case FileIdBothDirectoryInformation:
        case FileIdFullDirectoryInformation:
        case FileObjectIdInformation:
        case FileQuotaInformation:
        case FileReparsePointInformation:
            ok( status == STATUS_INFO_LENGTH_MISMATCH, "%u: wrong status %lx\n", class, status );
            break;
        default:
            ok( status == STATUS_INVALID_INFO_CLASS || status == STATUS_NOT_IMPLEMENTED,
                "%u: wrong status %lx\n", class, status );
            continue;
        }

        for (data_size = 1; data_size < sizeof(data); data_size++)
        {
            status = pNtQueryDirectoryFile( handle, 0, NULL, NULL, &io, data, data_size,
                                            class, FALSE, mask, TRUE );
            if (status == STATUS_BUFFER_OVERFLOW)
            {
                ok( io.Status == STATUS_BUFFER_OVERFLOW, "%u: wrong status %lx\n", class, io.Status );
                ok( io.Information == data_size || broken(!io.Information), /* win10 1709 */
                    "%u: wrong info %Ix\n", class, io.Information );
                if (io.Information) ok(data[0] == 0, "%u: wrong offset %lx\n",  class, data[0] );
            }
            else
            {
                ok( io.Status == 0xdeadbeef || io.Status == status, "%u: wrong status %lx\n", class, io.Status );
                ok( io.Information == (io.Status == 0xdeadbeef ? 0xdeadbeef : 0), "%u: wrong info %Ix\n", class, io.Information );
                ok(data[0] == 0x55555555, "%u: wrong offset %lx\n",  class, data[0] );
            }
            if (status != STATUS_INFO_LENGTH_MISMATCH) break;
        }

        switch (class)
        {
        case FileDirectoryInformation:
            ok( status == STATUS_BUFFER_OVERFLOW, "%u: wrong status %lx\n", class, status );
            ok( data_size == ((offsetof( FILE_DIRECTORY_INFORMATION, FileName[1] ) + 7) & ~7),
                "%u: wrong size %u\n", class, data_size );
            break;
        case FileFullDirectoryInformation:
            ok( status == STATUS_BUFFER_OVERFLOW, "%u: wrong status %lx\n", class, status );
            ok( data_size == ((offsetof( FILE_FULL_DIRECTORY_INFORMATION, FileName[1] ) + 7) & ~7),
                "%u: wrong size %u\n", class, data_size );
            break;
        case FileBothDirectoryInformation:
            ok( status == STATUS_BUFFER_OVERFLOW, "%u: wrong status %lx\n", class, status );
            ok( data_size == ((offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[1] ) + 7) & ~7),
                "%u: wrong size %u\n", class, data_size );
            break;
        case FileNamesInformation:
            ok( status == STATUS_BUFFER_OVERFLOW, "%u: wrong status %lx\n", class, status );
            ok( data_size == ((offsetof( FILE_NAMES_INFORMATION, FileName[1] ) + 7) & ~7),
                "%u: wrong size %u\n", class, data_size );
            break;
        case FileIdBothDirectoryInformation:
            ok( status == STATUS_BUFFER_OVERFLOW, "%u: wrong status %lx\n", class, status );
            ok( data_size == ((offsetof( FILE_ID_BOTH_DIRECTORY_INFORMATION, FileName[1] ) + 7) & ~7),
                "%u: wrong size %u\n", class, data_size );
            break;
        case FileIdFullDirectoryInformation:
            ok( status == STATUS_BUFFER_OVERFLOW, "%u: wrong status %lx\n", class, status );
            ok( data_size == ((offsetof( FILE_ID_FULL_DIRECTORY_INFORMATION, FileName[1] ) + 7) & ~7),
                "%u: wrong size %u\n", class, data_size );
            break;
        case FileIdGlobalTxDirectoryInformation:
            ok( status == STATUS_BUFFER_OVERFLOW, "%u: wrong status %lx\n", class, status );
            ok( data_size == ((offsetof( FILE_ID_GLOBAL_TX_DIR_INFORMATION, FileName[1] ) + 7) & ~7),
                "%u: wrong size %u\n", class, data_size );
            break;
        case FileObjectIdInformation:
            ok( status == STATUS_INVALID_INFO_CLASS, "%u: wrong status %lx\n", class, status );
            ok( data_size == sizeof(FILE_OBJECTID_INFORMATION), "%u: wrong size %u\n", class, data_size );
            break;
        case FileQuotaInformation:
            ok( status == STATUS_INVALID_INFO_CLASS, "%u: wrong status %lx\n", class, status );
            ok( data_size == sizeof(FILE_QUOTA_INFORMATION), "%u: wrong size %u\n", class, data_size );
            break;
        case FileReparsePointInformation:
            ok( status == STATUS_INVALID_INFO_CLASS, "%u: wrong status %lx\n", class, status );
            ok( data_size == sizeof(FILE_REPARSE_POINT_INFORMATION), "%u: wrong size %u\n", class, data_size );
            break;
        }
    }
}

static void test_NtQueryDirectoryFile(void)
{
    static const struct
    {
        const WCHAR *mask;
        int found[ARRAY_SIZE(testfiles)];
    }
    mask_tests[] =
    {
        {L"*.",                    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1}},
        {L"*. ",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L"* .",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L" *.",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L"*.*",                   {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
        {L"* *",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L"*.**",                  {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
        {L"*",                     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
        {L"**",                    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
        {L"?",                     {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0}},
        {L"?.",                    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}},
        {L"?..",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}},
        {L"??",                    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0}},
        {L"??.",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}},
        {L"??.???",                {0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L"<",                     {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1}},
        {L"*<a",                   {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0}},
        {L"<.",                    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1}},
        {L"<..",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}},
        {L"<.\"",                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1}},
        {L".<",                    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0}},
        {L"..<",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0}},
        {L"..*",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0}},
        {L"*..*",                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1}},
        {L"*..",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}},
        {L"..?",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0}},
        {L"..\"",                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L"\"",                    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0}},
        {L"\"\"",                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0}},
        {L"\"\"\"",                {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0}},
        {L"a.<",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1}},
        {L"ea.tmp.tmp<",           {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0}},
        {L"<tmp",                  {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0}},
        {L"<.tmp",                 {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0}},
        {L"<name.tmp",             {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L"<nam<tmp",              {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L"<name.<",               {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L"<name<",                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L"<\"",                   {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1}},
        {L"*\"",                   {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
        {L"*\"tmp\"tmp\"",         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0}},
        {L"n\"tmp",                {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L"ea\"",                  {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L"\"a",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0}},
        {L"\"\"a",                 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0}},
        {L"e\"a",                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L"*\"tmp",                {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0}},
        {L"<.<",                   {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
        {L"<\"<",                  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
        {L"<\"<\"",                {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
        {L"<\"<.",                 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}},
        {L"<.<\"",                 {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
        {L"<<",                    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
        {L"<a",                    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0}},
        {L"*a",                    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0}},
        {L"<aa",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0}},
        {L"<.a",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0}},
        {L"<..a",                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0}},
        {L"<.<.<",                 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1}},
        {L".<.<",                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0}},
        {L"<<.<",                  {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
        {L"<.<<",                  {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
        {L"<<<",                   {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
        {L"< ..",                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L"< .",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L"<\"\"",                 {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1}},
        {L">",                     {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0}},
        {L">.",                    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0}},
        {L">..",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}},
        {L">>",                    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0}},
        {L">>.",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0}},
        {L">>>",                   {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0}},
        {L">>.>>>",                {0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1}},
        {L">.>",                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1}},
        {L">>.tmp",                {0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L">>tmp",                 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {L">>>tmp",                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    };

    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING ntdirname, mask;
    char testdirA[MAX_PATH], buffer[MAX_PATH];
    WCHAR testdirW[MAX_PATH];
    int i, j;
    IO_STATUS_BLOCK io;
    WCHAR short_name[12];
    UINT data_size;
    BYTE data[8192];
    FILE_BOTH_DIRECTORY_INFORMATION *next, *fbdi = (FILE_BOTH_DIRECTORY_INFORMATION*)data;
    FILE_POSITION_INFORMATION pos_info;
    FILE_NAMES_INFORMATION *names;
    const WCHAR *filename = fbdi->FileName;
    BOOLEAN expect_empty;
    NTSTATUS status;
    HANDLE dirh, h;

    /* Clean up from prior aborted run, if any, then set up test files */
    ok(GetTempPathA(MAX_PATH, testdirA), "couldn't get temp dir\n");
    strcat(testdirA, "NtQueryDirectoryFile.tmp");
    pRtlMultiByteToUnicodeN(testdirW, sizeof(testdirW), NULL, testdirA, strlen(testdirA)+1);
    tear_down_attribute_test(testdirW);
    set_up_attribute_test(testdirW);

    if (!pRtlDosPathNameToNtPathName_U(testdirW, &ntdirname, NULL, NULL))
    {
        ok(0, "RtlDosPathNametoNtPathName_U failed\n");
        goto done;
    }
    InitializeObjectAttributes(&attr, &ntdirname, OBJ_CASE_INSENSITIVE, 0, NULL);

    test_flags_NtQueryDirectoryFile(&attr, testdirA, NULL, FALSE, TRUE, FALSE);
    test_flags_NtQueryDirectoryFile(&attr, testdirA, NULL, FALSE, FALSE, FALSE);
    test_flags_NtQueryDirectoryFile(&attr, testdirA, NULL, TRUE, TRUE, FALSE);
    test_flags_NtQueryDirectoryFile(&attr, testdirA, NULL, TRUE, FALSE, FALSE);

    for (i = 0; i < test_dir_count; i++)
    {
        if (testfiles[i].name[0] == '.') continue;  /* . and .. as masks are broken on Windows */
        mask.Buffer = testfiles[i].name;
        mask.Length = mask.MaximumLength = lstrlenW(testfiles[i].name) * sizeof(WCHAR);
        test_flags_NtQueryDirectoryFile(&attr, testdirA, &mask, FALSE, TRUE, FALSE);
        test_flags_NtQueryDirectoryFile(&attr, testdirA, &mask, FALSE, FALSE, FALSE);
        test_flags_NtQueryDirectoryFile(&attr, testdirA, &mask, TRUE, TRUE, FALSE);
        test_flags_NtQueryDirectoryFile(&attr, testdirA, &mask, TRUE, FALSE, FALSE);
    }

    for (i = 0; i < ARRAY_SIZE(mask_tests); ++i)
    {
        winetest_push_context("mask %s", debugstr_w(mask_tests[i].mask));
        RtlInitUnicodeString(&mask, mask_tests[i].mask);
        expect_empty = TRUE;
        for (j = 0; j < ARRAY_SIZE(mask_tests[i].found); ++j)
        {
            if (mask_tests[i].found[j])
            {
                expect_empty = FALSE;
                break;
            }
        }
        test_flags_NtQueryDirectoryFile(&attr, testdirA, &mask, FALSE, TRUE, expect_empty);
        for (j = 0; j < test_dir_count; j++)
            ok(testfiles[j].nfound == mask_tests[i].found[j], "%S, got %d.\n", testfiles[j].name, testfiles[j].nfound);
        winetest_pop_context();
    }

    /* short path passed as mask */
    status = pNtOpenFile(&dirh, SYNCHRONIZE | FILE_LIST_DIRECTORY, &attr, &io, FILE_SHARE_READ,
            FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_DIRECTORY_FILE);
    ok(status == STATUS_SUCCESS, "failed to open dir '%s'\n", testdirA);
    if (status != STATUS_SUCCESS) {
        skip("can't test if we can't open the directory\n");
        return;
    }
    status = pNtQueryInformationFile( dirh, &io, &pos_info, sizeof(pos_info), FilePositionInformation );
    ok( status == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", status );
    ok( pos_info.CurrentByteOffset.QuadPart == 0, "wrong pos %s\n",
        wine_dbgstr_longlong(pos_info.CurrentByteOffset.QuadPart));

    pos_info.CurrentByteOffset.QuadPart = 0xbeef;
    status = pNtSetInformationFile( dirh, &io, &pos_info, sizeof(pos_info), FilePositionInformation );
    ok( status == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", status );

    status = pNtQueryInformationFile( dirh, &io, &pos_info, sizeof(pos_info), FilePositionInformation );
    ok( status == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", status );
    ok( pos_info.CurrentByteOffset.QuadPart == 0xbeef, "wrong pos %s\n",
        wine_dbgstr_longlong(pos_info.CurrentByteOffset.QuadPart));

    mask.Buffer = testfiles[0].name;
    mask.Length = mask.MaximumLength = lstrlenW(testfiles[0].name) * sizeof(WCHAR);
    data_size = offsetof(FILE_BOTH_DIRECTORY_INFORMATION, FileName[256]);
    io.Status = 0xdeadbeef;
    status = pNtQueryDirectoryFile(dirh, 0, NULL, NULL, &io, data, data_size,
                                   FileBothDirectoryInformation, TRUE, &mask, FALSE);
    ok(status == STATUS_SUCCESS, "failed to query directory; status %lx\n", status);
    ok(io.Status == STATUS_SUCCESS, "failed to query directory; status %lx\n", io.Status);
    ok(fbdi->ShortName[0], "ShortName is empty\n");

    status = pNtQueryInformationFile( dirh, &io, &pos_info, sizeof(pos_info), FilePositionInformation );
    ok( status == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", status );
    ok( pos_info.CurrentByteOffset.QuadPart == 0xbeef, "wrong pos %s\n",
        wine_dbgstr_longlong(pos_info.CurrentByteOffset.QuadPart) );

    mask.Length = mask.MaximumLength = fbdi->ShortNameLength;
    memcpy(short_name, fbdi->ShortName, mask.Length);
    mask.Buffer = short_name;
    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    status = pNtQueryDirectoryFile(dirh, 0, NULL, NULL, &io, data, data_size,
                                   FileBothDirectoryInformation, TRUE, &mask, TRUE);
    ok(status == STATUS_SUCCESS, "failed to query directory status %lx\n", status);
    ok(io.Status == STATUS_SUCCESS, "failed to query directory status %lx\n", io.Status);
    ok(io.Information == offsetof(FILE_BOTH_DIRECTORY_INFORMATION, FileName[lstrlenW(testfiles[0].name)]),
       "wrong info %Ix\n", io.Information);
    ok(fbdi->FileNameLength == lstrlenW(testfiles[0].name)*sizeof(WCHAR) &&
            !memcmp(fbdi->FileName, testfiles[0].name, fbdi->FileNameLength),
            "incorrect long file name: %s\n", wine_dbgstr_wn(fbdi->FileName,
                fbdi->FileNameLength/sizeof(WCHAR)));

    status = pNtQueryInformationFile( dirh, &io, &pos_info, sizeof(pos_info), FilePositionInformation );
    ok( status == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", status );
    ok( pos_info.CurrentByteOffset.QuadPart == 0xbeef, "wrong pos %s\n",
        wine_dbgstr_longlong(pos_info.CurrentByteOffset.QuadPart) );

    /* tests with short buffer */
    memset( data, 0x55, data_size );
    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    data_size = offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[1] );
    status = pNtQueryDirectoryFile(dirh, 0, NULL, NULL, &io, data, data_size,
                                   FileBothDirectoryInformation, TRUE, &mask, TRUE);
    ok( status == STATUS_BUFFER_OVERFLOW, "wrong status %lx\n", status );
    ok( io.Status == STATUS_BUFFER_OVERFLOW, "wrong status %lx\n", io.Status );
    ok( io.Information == data_size || broken( io.Information == 0),
        "wrong info %Ix\n", io.Information );
    ok( fbdi->NextEntryOffset == 0 || fbdi->NextEntryOffset == 0x55555555, /* win10 >= 1709 */
        "wrong offset %lx\n",  fbdi->NextEntryOffset );
    if (!fbdi->NextEntryOffset)
    {
        ok( fbdi->FileNameLength == lstrlenW(testfiles[0].name) * sizeof(WCHAR),
            "wrong length %lx\n", fbdi->FileNameLength );
        ok( filename[0] == testfiles[0].name[0], "incorrect long file name: %s\n",
            wine_dbgstr_wn(fbdi->FileName, fbdi->FileNameLength/sizeof(WCHAR)));
        ok( filename[1] == 0x5555, "incorrect long file name: %s\n",
            wine_dbgstr_wn(fbdi->FileName, fbdi->FileNameLength/sizeof(WCHAR)));
    }

    test_NtQueryDirectoryFile_classes( dirh, &mask );

    /* mask may or may not be ignored when restarting the search */
    pRtlInitUnicodeString( &mask, dummyW );
    io.Status = 0xdeadbeef;
    data_size = offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[256] );
    status = pNtQueryDirectoryFile(dirh, 0, NULL, NULL, &io, data, data_size,
                                   FileBothDirectoryInformation, TRUE, &mask, TRUE);
    ok( status == STATUS_SUCCESS || status == STATUS_NO_MORE_FILES, "wrong status %lx\n", status );
    ok( io.Status == status, "wrong status %lx / %lx\n", io.Status, status );
    if (!status)
        ok( fbdi->FileNameLength == lstrlenW(testfiles[0].name)*sizeof(WCHAR) &&
            !memcmp(fbdi->FileName, testfiles[0].name, fbdi->FileNameLength),
            "incorrect long file name: %s\n",
            wine_dbgstr_wn(fbdi->FileName, fbdi->FileNameLength/sizeof(WCHAR)));

    pNtClose(dirh);

    status = pNtOpenFile(&dirh, SYNCHRONIZE | FILE_LIST_DIRECTORY, &attr, &io, FILE_SHARE_READ,
            FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_DIRECTORY_FILE);
    ok(status == STATUS_SUCCESS, "failed to open dir '%s'\n", testdirA);

    memset( data, 0x55, data_size );
    data_size = sizeof(data);
    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    status = pNtQueryDirectoryFile(dirh, 0, NULL, NULL, &io, data, data_size,
                                   FileBothDirectoryInformation, FALSE, NULL, TRUE);
    ok(status == STATUS_SUCCESS, "wrong status %lx\n", status);
    ok(io.Status == STATUS_SUCCESS, "wrong status %lx\n", io.Status);
    ok(io.Information > 0 && io.Information < data_size, "wrong info %Ix\n", io.Information);
    ok( fbdi->NextEntryOffset == ((offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[1] ) + 7) & ~7),
        "wrong offset %lx\n",  fbdi->NextEntryOffset );
    ok( fbdi->FileNameLength == sizeof(WCHAR), "wrong length %lx\n", fbdi->FileNameLength );
    ok( fbdi->FileName[0] == '.', "incorrect long file name: %s\n",
        wine_dbgstr_wn(fbdi->FileName, fbdi->FileNameLength/sizeof(WCHAR)));
    next = (FILE_BOTH_DIRECTORY_INFORMATION *)(data + fbdi->NextEntryOffset);
    ok( next->NextEntryOffset == ((offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[2] ) + 7) & ~7),
        "wrong offset %lx\n",  next->NextEntryOffset );
    ok( next->FileNameLength == 2 * sizeof(WCHAR), "wrong length %lx\n", next->FileNameLength );
    filename = next->FileName;
    ok( filename[0] == '.' && filename[1] == '.', "incorrect long file name: %s\n",
        wine_dbgstr_wn(next->FileName, next->FileNameLength/sizeof(WCHAR)));

    data_size = fbdi->NextEntryOffset + offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[1] );
    memset( data, 0x55, data_size );
    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    status = pNtQueryDirectoryFile( dirh, 0, NULL, NULL, &io, data, data_size,
                                    FileBothDirectoryInformation, FALSE, NULL, TRUE );
    ok( status == STATUS_SUCCESS, "wrong status %lx\n", status );
    ok( io.Status == STATUS_SUCCESS, "wrong status %lx\n", io.Status );
    ok( io.Information == offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[1] ),
        "wrong info %Ix\n", io.Information );
    ok( fbdi->NextEntryOffset == 0, "wrong offset %lx\n",  fbdi->NextEntryOffset );
    ok( fbdi->FileNameLength == sizeof(WCHAR), "wrong length %lx\n", fbdi->FileNameLength );
    ok( fbdi->FileName[0] == '.', "incorrect long file name: %s\n",
        wine_dbgstr_wn(fbdi->FileName, fbdi->FileNameLength/sizeof(WCHAR)));
    next = (FILE_BOTH_DIRECTORY_INFORMATION *)&fbdi->FileName[1];
    ok( next->NextEntryOffset == 0x55555555, "wrong offset %lx\n",  next->NextEntryOffset );

    data_size = fbdi->NextEntryOffset + offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[2] );
    memset( data, 0x55, data_size );
    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    status = pNtQueryDirectoryFile( dirh, 0, NULL, NULL, &io, data, data_size,
                                    FileBothDirectoryInformation, FALSE, NULL, TRUE );
    ok( status == STATUS_SUCCESS, "wrong status %lx\n", status );
    ok( io.Status == STATUS_SUCCESS, "wrong status %lx\n", io.Status );
    ok( io.Information == offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[1] ),
        "wrong info %Ix\n", io.Information );
    ok( fbdi->NextEntryOffset == 0, "wrong offset %lx\n",  fbdi->NextEntryOffset );

    data_size = ((offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[1] ) + 7) & ~7) +
                  offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[2] );
    memset( data, 0x55, data_size );
    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    status = pNtQueryDirectoryFile( dirh, 0, NULL, NULL, &io, data, data_size + 32,
                                    FileBothDirectoryInformation, FALSE, NULL, TRUE );
    ok( status == STATUS_SUCCESS, "wrong status %lx\n", status );
    ok( io.Status == STATUS_SUCCESS, "wrong status %lx\n", io.Status );
    ok( io.Information == data_size || io.Information == ((data_size + 7) & ~7),
        "wrong info %Ix / %x\n", io.Information, data_size );
    ok( fbdi->NextEntryOffset == ((offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[1] ) + 7) & ~7),
        "wrong offset %lx\n",  fbdi->NextEntryOffset );
    ok( fbdi->FileNameLength == sizeof(WCHAR), "wrong length %lx\n", fbdi->FileNameLength );
    ok( fbdi->FileName[0] == '.', "incorrect long file name: %s\n",
        wine_dbgstr_wn(fbdi->FileName, fbdi->FileNameLength/sizeof(WCHAR)));
    next = (FILE_BOTH_DIRECTORY_INFORMATION *)(data + fbdi->NextEntryOffset);
    ok( next->NextEntryOffset == 0, "wrong offset %lx\n",  next->NextEntryOffset );
    ok( next->FileNameLength == 2 * sizeof(WCHAR), "wrong length %lx\n", next->FileNameLength );
    filename = next->FileName;
    ok( filename[0] == '.' && filename[1] == '.', "incorrect long file name: %s\n",
        wine_dbgstr_wn(next->FileName, next->FileNameLength/sizeof(WCHAR)));

    data_size = ((offsetof( FILE_NAMES_INFORMATION, FileName[1] ) + 7) & ~7) +
                  offsetof( FILE_NAMES_INFORMATION, FileName[2] );
    memset( data, 0x55, data_size );
    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    status = pNtQueryDirectoryFile( dirh, 0, NULL, NULL, &io, data, data_size,
                                    FileNamesInformation, FALSE, NULL, TRUE );
    ok( status == STATUS_SUCCESS, "wrong status %lx\n", status );
    ok( io.Status == STATUS_SUCCESS, "wrong status %lx\n", io.Status );
    ok( io.Information == data_size, "wrong info %Ix / %x\n", io.Information, data_size );
    names = (FILE_NAMES_INFORMATION *)data;
    ok( names->NextEntryOffset == ((offsetof( FILE_NAMES_INFORMATION, FileName[1] ) + 7) & ~7),
        "wrong offset %lx\n",  names->NextEntryOffset );
    ok( names->FileNameLength == sizeof(WCHAR), "wrong length %lx\n", names->FileNameLength );
    ok( names->FileName[0] == '.', "incorrect long file name: %s\n",
        wine_dbgstr_wn(names->FileName, names->FileNameLength/sizeof(WCHAR)));
    names = (FILE_NAMES_INFORMATION *)(data + names->NextEntryOffset);
    ok( names->NextEntryOffset == 0, "wrong offset %lx\n",  names->NextEntryOffset );
    ok( names->FileNameLength == 2 * sizeof(WCHAR), "wrong length %lx\n", names->FileNameLength );
    filename = names->FileName;
    ok( filename[0] == '.' && filename[1] == '.', "incorrect long file name: %s\n",
        wine_dbgstr_wn(names->FileName, names->FileNameLength/sizeof(WCHAR)));

    pNtClose(dirh);

    /* create new handle to change mask */
    status = pNtOpenFile(&dirh, SYNCHRONIZE | FILE_LIST_DIRECTORY, &attr, &io, FILE_SHARE_READ,
            FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_DIRECTORY_FILE);
    ok(status == STATUS_SUCCESS, "failed to open dir '%s'\n", testdirA);

    pRtlInitUnicodeString( &mask, dummyW );
    io.Status = 0xdeadbeef;
    data_size = sizeof(data);
    status = pNtQueryDirectoryFile(dirh, 0, NULL, NULL, &io, data, data_size,
                                   FileBothDirectoryInformation, TRUE, &mask, TRUE);
    ok(status == STATUS_NO_SUCH_FILE, "wrong status %lx\n", status);
    ok(io.Status == 0xdeadbeef, "wrong status %lx\n", io.Status);

    io.Status = 0xdeadbeef;
    status = pNtQueryDirectoryFile(dirh, 0, NULL, NULL, &io, data, data_size,
                                   FileBothDirectoryInformation, TRUE, NULL, FALSE);
    ok(status == STATUS_NO_MORE_FILES, "wrong status %lx\n", status);
    ok(io.Status == STATUS_NO_MORE_FILES, "wrong status %lx\n", io.Status);

    io.Status = 0xdeadbeef;
    status = pNtQueryDirectoryFile(dirh, 0, NULL, NULL, &io, data, data_size,
                                   FileBothDirectoryInformation, TRUE, NULL, TRUE);
    ok(status == STATUS_NO_MORE_FILES, "wrong status %lx\n", status);
    ok(io.Status == STATUS_NO_MORE_FILES, "wrong status %lx\n", io.Status);

    pNtClose(dirh);

    io.Status = 0xdeadbeef;
    status = pNtQueryDirectoryFile( (HANDLE)0xbeef, 0, NULL, NULL, &io, data, data_size,
                                    FileBothDirectoryInformation, TRUE, NULL, TRUE );
    ok(status == STATUS_INVALID_HANDLE, "wrong status %lx\n", status);
    ok(io.Status == 0xdeadbeef, "wrong status %lx\n", io.Status);

    GetModuleFileNameA( 0, buffer, sizeof(buffer) );
    h = CreateFileA( buffer, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    if (h != INVALID_HANDLE_VALUE)
    {
        io.Status = 0xdeadbeef;
        status = pNtQueryDirectoryFile( h, 0, NULL, NULL, &io, data, data_size,
                                        FileBothDirectoryInformation, TRUE, NULL, TRUE );
        ok(status == STATUS_INVALID_PARAMETER, "wrong status %lx\n", status);
        ok(io.Status == 0xdeadbeef, "wrong status %lx\n", io.Status);
        CloseHandle ( h );
    }

done:
    test_directory_sort( testdirW );
    tear_down_attribute_test( testdirW );
    pRtlFreeUnicodeString(&ntdirname);
}

static void set_up_case_test(const char *testdir)
{
    BOOL ret;
    char buf[MAX_PATH + 5];
    HANDLE h;

    ret = CreateDirectoryA(testdir, NULL);
    ok(ret, "couldn't create dir '%s', error %ld\n", testdir, GetLastError());

    sprintf(buf, "%s\\%s", testdir, "TesT");
    h = CreateFileA(buf, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, 0);
    ok(h != INVALID_HANDLE_VALUE, "failed to create temp file '%s'\n", buf);
    CloseHandle(h);
}

static void tear_down_case_test(const char *testdir)
{
    int ret;
    char buf[MAX_PATH];

    sprintf(buf, "%s\\%s", testdir, "TesT");
    ret = DeleteFileA(buf);
    ok(ret || (GetLastError() == ERROR_PATH_NOT_FOUND),
       "Failed to rm %s, error %ld\n", buf, GetLastError());
    RemoveDirectoryA(testdir);
}

static void test_NtQueryDirectoryFile_case(void)
{
    static const char testfile[] = "TesT";
    static const WCHAR testfile_w[] = {'T','e','s','T'};
    static int testfile_len = sizeof(testfile) - 1;
    static WCHAR testmask[] = {'t','e','s','t'};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING ntdirname;
    char testdir[MAX_PATH];
    WCHAR testdir_w[MAX_PATH];
    HANDLE dirh;
    UNICODE_STRING mask;
    IO_STATUS_BLOCK io;
    UINT data_size, data_len;
    BYTE data[8192];
    FILE_BOTH_DIRECTORY_INFORMATION *dir_info = (FILE_BOTH_DIRECTORY_INFORMATION *)data;
    DWORD status;
    WCHAR *name;
    ULONG name_len;

    /* Clean up from prior aborted run, if any, then set up test files */
    ok(GetTempPathA(MAX_PATH, testdir), "couldn't get temp dir\n");
    strcat(testdir, "case.tmp");
    tear_down_case_test(testdir);
    set_up_case_test(testdir);

    pRtlMultiByteToUnicodeN(testdir_w, sizeof(testdir_w), NULL, testdir, strlen(testdir) + 1);
    if (!pRtlDosPathNameToNtPathName_U(testdir_w, &ntdirname, NULL, NULL))
    {
        ok(0, "RtlDosPathNametoNtPathName_U failed\n");
        goto done;
    }
    InitializeObjectAttributes(&attr, &ntdirname, OBJ_CASE_INSENSITIVE, 0, NULL);

    data_size = offsetof(FILE_BOTH_DIRECTORY_INFORMATION, FileName[256]);

    status = pNtOpenFile(&dirh, SYNCHRONIZE | FILE_LIST_DIRECTORY, &attr, &io, FILE_SHARE_READ,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_DIRECTORY_FILE);
    ok (status == STATUS_SUCCESS, "failed to open dir '%s', ret 0x%lx, error %ld\n", testdir, status, GetLastError());
    if (status != STATUS_SUCCESS)
    {
       skip("can't test if we can't open the directory\n");
       return;
    }

    mask.Buffer = testmask;
    mask.Length = mask.MaximumLength = sizeof(testmask);
    pNtQueryDirectoryFile(dirh, NULL, NULL, NULL, &io, data, data_size,
                          FileBothDirectoryInformation, TRUE, &mask, FALSE);
    ok(io.Status == STATUS_SUCCESS, "failed to query directory; status %lx\n", io.Status);
    data_len = io.Information;
    ok(data_len >= sizeof(FILE_BOTH_DIRECTORY_INFORMATION), "not enough data in directory\n");

    name = dir_info->FileName;
    name_len = dir_info->FileNameLength / sizeof(WCHAR);

    ok(name_len == testfile_len, "unexpected filename length %lu\n", name_len);
    ok(!memcmp(name, testfile_w, testfile_len * sizeof(WCHAR)), "unexpected filename %s\n",
       wine_dbgstr_wn(name, name_len));

    pNtClose(dirh);

done:
    tear_down_case_test(testdir);
    pRtlFreeUnicodeString(&ntdirname);
}

static NTSTATUS get_file_id( FILE_INTERNAL_INFORMATION *info, const WCHAR *root, const WCHAR *name )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;

    InitializeObjectAttributes( &attr, &nameW, OBJ_CASE_INSENSITIVE, 0, NULL );
    if (root)
    {
        RtlInitUnicodeString( &nameW, root );
        status = pNtOpenFile( &attr.RootDirectory, SYNCHRONIZE | FILE_LIST_DIRECTORY, &attr, &io,
                              FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT |
                              FILE_OPEN_FOR_BACKUP_INTENT | FILE_DIRECTORY_FILE );
        if (status) return status;
    }
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        status = pNtOpenFile( &handle, FILE_GENERIC_READ, &attr, &io, FILE_SHARE_READ,
                              FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE );
        if (attr.RootDirectory) NtClose( attr.RootDirectory );
    }
    else handle = attr.RootDirectory;

    if (!status)
    {
        status = pNtQueryInformationFile( handle, &io, info, sizeof(*info), FileInternalInformation );
        NtClose( handle );
    }
    return status;
}

static void test_redirection(void)
{
    ULONG old, cur;
    NTSTATUS status;
    ULONGLONG *tls64 = NULL;

    if (!pRtlWow64EnableFsRedirection || !pRtlWow64EnableFsRedirectionEx)
    {
        skip( "Wow64 redirection not supported\n" );
        return;
    }
    status = pRtlWow64EnableFsRedirectionEx( FALSE, &old );
    if (status == STATUS_NOT_IMPLEMENTED)
    {
        skip( "Wow64 redirection not supported\n" );
        return;
    }
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %lx\n", status );

#ifndef _WIN64
    if (NtCurrentTeb()->GdiBatchCount)
        tls64 = ((TEB64 *)NtCurrentTeb()->GdiBatchCount)->TlsSlots + WOW64_TLS_FILESYSREDIR;
#endif

    status = pRtlWow64EnableFsRedirectionEx( FALSE, &cur );
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %lx\n", status );
    ok( !cur, "RtlWow64EnableFsRedirectionEx got %lu\n", cur );
    if (tls64) ok( *tls64 == FALSE, "wrong tls %s\n", wine_dbgstr_longlong(*tls64) );

    status = pRtlWow64EnableFsRedirectionEx( TRUE, &cur );
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %lx\n", status );
    status = pRtlWow64EnableFsRedirectionEx( TRUE, &cur );
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %lx\n", status );
    ok( cur == 1, "RtlWow64EnableFsRedirectionEx got %lu\n", cur );
    if (tls64) ok( *tls64 == TRUE, "wrong tls %s\n", wine_dbgstr_longlong(*tls64) );

    status = pRtlWow64EnableFsRedirection( TRUE );
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %lx\n", status );
    status = pRtlWow64EnableFsRedirectionEx( TRUE, &cur );
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %lx\n", status );
    ok( !cur, "RtlWow64EnableFsRedirectionEx got %lu\n", cur );
    if (tls64) ok( *tls64 == TRUE, "wrong tls %s\n", wine_dbgstr_longlong(*tls64) );

    status = pRtlWow64EnableFsRedirectionEx( 123, &cur );
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %lx\n", status );
    ok( cur == TRUE, "RtlWow64EnableFsRedirectionEx got %lu\n", cur );
    if (tls64) ok( *tls64 == 123, "wrong tls %s\n", wine_dbgstr_longlong(*tls64) );

    status = pRtlWow64EnableFsRedirectionEx( 0xdeadbeef, &cur );
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %lx\n", status );
    ok( cur == 123, "RtlWow64EnableFsRedirectionEx got %lu\n", cur );
    if (tls64) ok( *tls64 == 0xdeadbeef, "wrong tls %s\n", wine_dbgstr_longlong(*tls64) );

    status = pRtlWow64EnableFsRedirectionEx( TRUE, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "RtlWow64EnableFsRedirectionEx failed with status %lx\n", status );
    status = pRtlWow64EnableFsRedirectionEx( TRUE, (void*)1 );
    ok( status == STATUS_ACCESS_VIOLATION, "RtlWow64EnableFsRedirectionEx failed with status %lx\n", status );
    status = pRtlWow64EnableFsRedirectionEx( TRUE, (void*)0xDEADBEEF );
    ok( status == STATUS_ACCESS_VIOLATION, "RtlWow64EnableFsRedirectionEx failed with status %lx\n", status );

    status = pRtlWow64EnableFsRedirection( FALSE );
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %lx\n", status );
    status = pRtlWow64EnableFsRedirectionEx( FALSE, &cur );
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %lx\n", status );
    ok( cur == 1, "RtlWow64EnableFsRedirectionEx got %lu\n", cur );
    if (tls64) ok( *tls64 == FALSE, "wrong tls %s\n", wine_dbgstr_longlong(*tls64) );

    if (tls64)
    {
        static const struct
        {
            const WCHAR *root, *name;
            NTSTATUS expect;
            BOOL redirected;
            NTSTATUS alt;
        } tests[] =
        {
            { NULL, L"\\??\\C:\\windows\\system32\\kernel32.dll", STATUS_SUCCESS, TRUE },
            { NULL, L"\\??\\C:\\\\windows\\system32\\kernel32.dll", STATUS_SUCCESS, FALSE, STATUS_OBJECT_NAME_INVALID },
            { L"\\??\\C:\\", L"windows\\system32\\kernel32.dll", STATUS_SUCCESS, FALSE },
            { L"\\??\\C:\\windows", L"system32\\kernel32.dll", STATUS_SUCCESS, TRUE },
            { L"\\??\\C:\\\\windows", L"system32\\kernel32.dll", STATUS_SUCCESS, TRUE, STATUS_OBJECT_NAME_INVALID },
            { L"\\??\\C:\\windows\\system32", L"kernel32.dll", STATUS_SUCCESS, TRUE },
            { L"\\??\\C:\\windows\\system32", NULL, STATUS_SUCCESS, TRUE },
            { L"\\??\\C:\\windows\\system32", L"drivers\\ndis.sys", STATUS_OBJECT_NAME_NOT_FOUND, FALSE, STATUS_OBJECT_PATH_NOT_FOUND },
            { L"\\??\\C:\\windows\\system32", L"drivers\\etc\\hosts", STATUS_OBJECT_PATH_NOT_FOUND },
            { L"\\??\\C:\\windows\\system32\\drivers", NULL, STATUS_SUCCESS, TRUE, STATUS_OBJECT_NAME_NOT_FOUND },
            { L"\\??\\C:\\windows\\system32\\drivers\\etc", L"hosts", STATUS_SUCCESS, FALSE },
            { NULL, L"\\DosDevices\\C:\\windows\\system32\\kernel32.dll", STATUS_SUCCESS, FALSE },
            { L"\\DosDevices\\C:\\", L"windows\\system32\\kernel32.dll", STATUS_SUCCESS, FALSE },
            { L"\\DosDevices\\C:\\windows", L"system32\\kernel32.dll", STATUS_SUCCESS, TRUE },
            { L"\\DosDevices\\C:\\windows\\system32", L"kernel32.dll", STATUS_SUCCESS, TRUE },
            { L"\\DosDevices\\C:\\windows\\system32", NULL, STATUS_SUCCESS, FALSE },
            { L"\\DosDevices\\C:\\windows\\system32", L"drivers\\ndis.sys", STATUS_OBJECT_NAME_NOT_FOUND, FALSE, STATUS_OBJECT_PATH_NOT_FOUND },
            { L"\\DosDevices\\C:\\windows\\system32", L"drivers\\etc\\hosts", STATUS_SUCCESS, FALSE },
            { L"\\DosDevices\\C:\\windows\\system32\\drivers", NULL, STATUS_SUCCESS, FALSE },
            { L"\\DosDevices\\C:\\windows\\system32\\drivers\\etc", NULL, STATUS_SUCCESS, FALSE },
            { NULL, L"\\??\\C:\\windows\\sysnative\\kernel32.dll", STATUS_SUCCESS, FALSE },
            { L"\\??\\C:\\", L"windows\\sysnative\\kernel32.dll", STATUS_OBJECT_PATH_NOT_FOUND },
            { L"\\??\\C:\\windows", L"sysnative\\kernel32.dll", STATUS_SUCCESS, FALSE },
            { L"\\??\\C:\\windows\\sysnative", L"kernel32.dll" , STATUS_SUCCESS, TRUE },
            { L"\\??\\C:\\windows\\sysnative", NULL, STATUS_SUCCESS, FALSE },
            { NULL, L"\\DosDevices\\C:\\windows\\sysnative\\kernel32.dll", STATUS_OBJECT_PATH_NOT_FOUND },
            { L"\\DosDevices\\C:\\", L"windows\\sysnative\\kernel32.dll", STATUS_OBJECT_PATH_NOT_FOUND },
            { L"\\DosDevices\\C:\\windows", L"sysnative\\kernel32.dll", STATUS_SUCCESS, FALSE },
            { L"\\DosDevices\\C:\\windows\\sysnative", L"kernel32.dll" , STATUS_OBJECT_NAME_NOT_FOUND },
            { L"\\DosDevices\\C:\\windows\\sysnative", NULL, STATUS_OBJECT_NAME_NOT_FOUND },
        };
        FILE_INTERNAL_INFORMATION info, info_redir;
        unsigned int i;

        for (i = 0; i < ARRAY_SIZE(tests); i++)
        {
            pRtlWow64EnableFsRedirection( FALSE );
            status = get_file_id( &info, tests[i].root, tests[i].name );
            ok( !status || status == tests[i].expect || (tests[i].alt && status == tests[i].alt),
                "%u: got %lx / %lx for %s + %s without redirect\n", i, status, tests[i].expect,
                debugstr_w( tests[i].root ), debugstr_w( tests[i].name ));
            if (status) memset( &info, 0xcc, sizeof(info) );
            pRtlWow64EnableFsRedirection( TRUE );
            status = get_file_id( &info_redir, tests[i].root, tests[i].name );
            ok( status == tests[i].expect || (tests[i].alt && status == tests[i].alt),
                "%u: got %lx / %lx for %s + %s\n", i, status, tests[i].expect,
                debugstr_w( tests[i].root ), debugstr_w( tests[i].name ));
            if (!status)
            {
                BOOL redirected = memcmp( &info_redir, &info, sizeof(info) );
                ok( !redirected == !tests[i].redirected,
                    "%u: was %sredirected for %s + %s\n", i, redirected ? "" : "not ",
                    debugstr_w( tests[i].root ), debugstr_w( tests[i].name ));
            }
        }
    }
    pRtlWow64EnableFsRedirectionEx( old, &cur );
}

START_TEST(directory)
{
    WCHAR sysdir[MAX_PATH];
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");

    pNtClose                = (void *)GetProcAddress(hntdll, "NtClose");
    pNtOpenFile             = (void *)GetProcAddress(hntdll, "NtOpenFile");
    pNtQueryDirectoryFile   = (void *)GetProcAddress(hntdll, "NtQueryDirectoryFile");
    pNtQueryInformationFile = (void *)GetProcAddress(hntdll, "NtQueryInformationFile");
    pNtSetInformationFile   = (void *)GetProcAddress(hntdll, "NtSetInformationFile");
    pRtlCreateUnicodeStringFromAsciiz = (void *)GetProcAddress(hntdll, "RtlCreateUnicodeStringFromAsciiz");
    pRtlDosPathNameToNtPathName_U = (void *)GetProcAddress(hntdll, "RtlDosPathNameToNtPathName_U");
    pRtlInitUnicodeString   = (void *)GetProcAddress(hntdll, "RtlInitUnicodeString");
    pRtlFreeUnicodeString   = (void *)GetProcAddress(hntdll, "RtlFreeUnicodeString");
    pRtlCompareUnicodeString = (void *)GetProcAddress(hntdll, "RtlCompareUnicodeString");
    pRtlMultiByteToUnicodeN = (void *)GetProcAddress(hntdll,"RtlMultiByteToUnicodeN");
    pRtlWow64EnableFsRedirection = (void *)GetProcAddress(hntdll,"RtlWow64EnableFsRedirection");
    pRtlWow64EnableFsRedirectionEx = (void *)GetProcAddress(hntdll,"RtlWow64EnableFsRedirectionEx");

    GetSystemDirectoryW( sysdir, MAX_PATH );
    test_directory_sort( sysdir );
    test_NtQueryDirectoryFile();
    test_NtQueryDirectoryFile_case();
    test_redirection();
}
