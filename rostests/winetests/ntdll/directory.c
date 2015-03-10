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
#include "winternl.h"

static NTSTATUS (WINAPI *pNtClose)( PHANDLE );
static NTSTATUS (WINAPI *pNtOpenFile)    ( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG );
static NTSTATUS (WINAPI *pNtQueryDirectoryFile)(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,
                                                PVOID,ULONG,FILE_INFORMATION_CLASS,BOOLEAN,PUNICODE_STRING,BOOLEAN);
static BOOLEAN  (WINAPI *pRtlCreateUnicodeStringFromAsciiz)(PUNICODE_STRING,LPCSTR);
static BOOL     (WINAPI *pRtlDosPathNameToNtPathName_U)( LPCWSTR, PUNICODE_STRING, PWSTR*, CURDIR* );
static VOID     (WINAPI *pRtlInitUnicodeString)( PUNICODE_STRING, LPCWSTR );
static VOID     (WINAPI *pRtlFreeUnicodeString)( PUNICODE_STRING );
static NTSTATUS (WINAPI *pRtlMultiByteToUnicodeN)( LPWSTR dst, DWORD dstlen, LPDWORD reslen,
                                                   LPCSTR src, DWORD srclen );
static NTSTATUS (WINAPI *pRtlWow64EnableFsRedirection)( BOOLEAN enable );
static NTSTATUS (WINAPI *pRtlWow64EnableFsRedirectionEx)( ULONG disable, ULONG *old_value );

/* The attribute sets to test */
static struct testfile_s {
    BOOL attr_done;           /* set if attributes were tested for this file already */
    const DWORD attr;         /* desired attribute */
    const char *name;         /* filename to use */
    const char *target;       /* what to point to (only for reparse pts) */
    const char *description;  /* for error messages */
    int nfound;               /* How many were found (expect 1) */
    WCHAR nameW[20];          /* unicode version of name (filled in later) */
} testfiles[] = {
    { 0, FILE_ATTRIBUTE_NORMAL,    "n.tmp", NULL, "normal" },
    { 0, FILE_ATTRIBUTE_HIDDEN,    "h.tmp", NULL, "hidden" },
    { 0, FILE_ATTRIBUTE_SYSTEM,    "s.tmp", NULL, "system" },
    { 0, FILE_ATTRIBUTE_DIRECTORY, "d.tmp", NULL, "directory" },
    { 0, FILE_ATTRIBUTE_DIRECTORY, ".",     NULL, ". directory" },
    { 0, FILE_ATTRIBUTE_DIRECTORY, "..",    NULL, ".. directory" },
    { 0, 0, NULL }
};
static const int max_test_dir_size = 20;  /* size of above plus some for .. etc */

/* Create a test directory full of attribute test files, clear counts */
static void set_up_attribute_test(const char *testdirA)
{
    int i;
    BOOL ret;

    ret = CreateDirectoryA(testdirA, NULL);
    ok(ret, "couldn't create dir '%s', error %d\n", testdirA, GetLastError());

    for (i=0; testfiles[i].name; i++) {
        char buf[MAX_PATH];
        pRtlMultiByteToUnicodeN(testfiles[i].nameW, sizeof(testfiles[i].nameW), NULL, testfiles[i].name, strlen(testfiles[i].name)+1);

        if (strcmp(testfiles[i].name, ".") == 0 || strcmp(testfiles[i].name, "..") == 0)
            continue;
        sprintf(buf, "%s\\%s", testdirA, testfiles[i].name);
        if (testfiles[i].attr & FILE_ATTRIBUTE_DIRECTORY) {
            ret = CreateDirectoryA(buf, NULL);
            ok(ret, "couldn't create dir '%s', error %d\n", buf, GetLastError());
        } else {
            HANDLE h = CreateFileA(buf,
                                   GENERIC_READ|GENERIC_WRITE,
                                   0, NULL, CREATE_ALWAYS,
                                   testfiles[i].attr, 0);
            ok( h != INVALID_HANDLE_VALUE, "failed to create temp file '%s'\n", buf );
            CloseHandle(h);
        }
    }
}

static void reset_found_files(void)
{
    int i;

    for (i = 0; testfiles[i].name; i++)
        testfiles[i].nfound = 0;
}

/* Remove the given test directory and the attribute test files, if any */
static void tear_down_attribute_test(const char *testdirA)
{
    int i;

    for (i=0; testfiles[i].name; i++) {
        int ret;
        char buf[MAX_PATH];
        if (strcmp(testfiles[i].name, ".") == 0 || strcmp(testfiles[i].name, "..") == 0)
            continue;
        sprintf(buf, "%s\\%s", testdirA, testfiles[i].name);
        if (testfiles[i].attr & FILE_ATTRIBUTE_DIRECTORY) {
            ret = RemoveDirectoryA(buf);
            ok(ret || (GetLastError() == ERROR_PATH_NOT_FOUND),
               "Failed to rmdir %s, error %d\n", buf, GetLastError());
        } else {
            ret = DeleteFileA(buf);
            ok(ret || (GetLastError() == ERROR_PATH_NOT_FOUND),
               "Failed to rm %s, error %d\n", buf, GetLastError());
        }
    }
    RemoveDirectoryA(testdirA);
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

    for (i=0; testfiles[i].name; i++) {
        int len = strlen(testfiles[i].name);
        if (namelen != len || memcmp(nameW, testfiles[i].nameW, len*sizeof(WCHAR)))
            continue;
        if (!testfiles[i].attr_done) {
            ok (attrib == (testfiles[i].attr & attribmask), "file %s: expected %s (%x), got %x (is your linux new enough?)\n", testfiles[i].name, testfiles[i].description, testfiles[i].attr, attrib);
            testfiles[i].attr_done = TRUE;
        }
        testfiles[i].nfound++;
        break;
    }
    ok(testfiles[i].name != NULL, "unexpected file found\n");
}

static void test_flags_NtQueryDirectoryFile(OBJECT_ATTRIBUTES *attr, const char *testdirA,
                                            UNICODE_STRING *mask,
                                            BOOLEAN single_entry, BOOLEAN restart_flag)
{
    HANDLE dirh;
    IO_STATUS_BLOCK io;
    UINT data_pos, data_size;
    UINT data_len;    /* length of dir data */
    BYTE data[8192];  /* directory data */
    FILE_BOTH_DIRECTORY_INFORMATION *dir_info;
    DWORD status;
    int numfiles;
    int i;

    reset_found_files();

    data_size = mask ? offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[256] ) : sizeof(data);

    /* Read the directory and note which files are found */
    status = pNtOpenFile( &dirh, SYNCHRONIZE | FILE_LIST_DIRECTORY, attr, &io, FILE_OPEN,
                         FILE_SYNCHRONOUS_IO_NONALERT|FILE_OPEN_FOR_BACKUP_INTENT|FILE_DIRECTORY_FILE);
    ok (status == STATUS_SUCCESS, "failed to open dir '%s', ret 0x%x, error %d\n", testdirA, status, GetLastError());
    if (status != STATUS_SUCCESS) {
       skip("can't test if we can't open the directory\n");
       return;
    }

    pNtQueryDirectoryFile( dirh, NULL, NULL, NULL, &io, data, data_size,
                       FileBothDirectoryInformation, single_entry, mask, restart_flag );
    ok (U(io).Status == STATUS_SUCCESS, "failed to query directory; status %x\n", U(io).Status);
    data_len = io.Information;
    ok (data_len >= sizeof(FILE_BOTH_DIRECTORY_INFORMATION), "not enough data in directory\n");

    data_pos = 0;
    numfiles = 0;
    while ((data_pos < data_len) && (numfiles < max_test_dir_size)) {
        dir_info = (FILE_BOTH_DIRECTORY_INFORMATION *)(data + data_pos);

        tally_test_file(dir_info);

        if (dir_info->NextEntryOffset == 0) {
            pNtQueryDirectoryFile( dirh, 0, NULL, NULL, &io, data, data_size,
                               FileBothDirectoryInformation, single_entry, mask, FALSE );
            if (U(io).Status == STATUS_NO_MORE_FILES)
                break;
            ok (U(io).Status == STATUS_SUCCESS, "failed to query directory; status %x\n", U(io).Status);
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

    if (mask)
        for (i=0; testfiles[i].name; i++)
            ok(testfiles[i].nfound == (testfiles[i].nameW == mask->Buffer),
               "Wrong number %d of %s files found (single_entry=%d,mask=%s)\n",
               testfiles[i].nfound, testfiles[i].description, single_entry,
               wine_dbgstr_wn(mask->Buffer, mask->Length/sizeof(WCHAR) ));
    else
        for (i=0; testfiles[i].name; i++)
            ok(testfiles[i].nfound == 1, "Wrong number %d of %s files found (single_entry=%d,restart=%d)\n",
               testfiles[i].nfound, testfiles[i].description, single_entry, restart_flag);
    pNtClose(dirh);
}

static void test_NtQueryDirectoryFile(void)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING ntdirname;
    char testdirA[MAX_PATH];
    WCHAR testdirW[MAX_PATH];
    int i;

    /* Clean up from prior aborted run, if any, then set up test files */
    ok(GetTempPathA(MAX_PATH, testdirA), "couldn't get temp dir\n");
    strcat(testdirA, "NtQueryDirectoryFile.tmp");
    tear_down_attribute_test(testdirA);
    set_up_attribute_test(testdirA);

    pRtlMultiByteToUnicodeN(testdirW, sizeof(testdirW), NULL, testdirA, strlen(testdirA)+1);
    if (!pRtlDosPathNameToNtPathName_U(testdirW, &ntdirname, NULL, NULL))
    {
        ok(0, "RtlDosPathNametoNtPathName_U failed\n");
        goto done;
    }
    InitializeObjectAttributes(&attr, &ntdirname, OBJ_CASE_INSENSITIVE, 0, NULL);

    test_flags_NtQueryDirectoryFile(&attr, testdirA, NULL, FALSE, TRUE);
    test_flags_NtQueryDirectoryFile(&attr, testdirA, NULL, FALSE, FALSE);
    test_flags_NtQueryDirectoryFile(&attr, testdirA, NULL, TRUE, TRUE);
    test_flags_NtQueryDirectoryFile(&attr, testdirA, NULL, TRUE, FALSE);

    for (i = 0; testfiles[i].name; i++)
    {
        UNICODE_STRING mask;

        if (testfiles[i].nameW[0] == '.') continue;  /* . and .. as masks are broken on Windows */
        mask.Buffer = testfiles[i].nameW;
        mask.Length = mask.MaximumLength = lstrlenW(testfiles[i].nameW) * sizeof(WCHAR);
        test_flags_NtQueryDirectoryFile(&attr, testdirA, &mask, FALSE, TRUE);
        test_flags_NtQueryDirectoryFile(&attr, testdirA, &mask, FALSE, FALSE);
        test_flags_NtQueryDirectoryFile(&attr, testdirA, &mask, TRUE, TRUE);
        test_flags_NtQueryDirectoryFile(&attr, testdirA, &mask, TRUE, FALSE);
    }

done:
    tear_down_attribute_test(testdirA);
    pRtlFreeUnicodeString(&ntdirname);
}

static void test_redirection(void)
{
    ULONG old, cur;
    NTSTATUS status;

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
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %x\n", status );

    status = pRtlWow64EnableFsRedirectionEx( FALSE, &cur );
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %x\n", status );
    ok( !cur, "RtlWow64EnableFsRedirectionEx got %u\n", cur );

    status = pRtlWow64EnableFsRedirectionEx( TRUE, &cur );
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %x\n", status );
    status = pRtlWow64EnableFsRedirectionEx( TRUE, &cur );
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %x\n", status );
    ok( cur == 1, "RtlWow64EnableFsRedirectionEx got %u\n", cur );

    status = pRtlWow64EnableFsRedirection( TRUE );
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %x\n", status );
    status = pRtlWow64EnableFsRedirectionEx( TRUE, &cur );
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %x\n", status );
    ok( !cur, "RtlWow64EnableFsRedirectionEx got %u\n", cur );

    status = pRtlWow64EnableFsRedirectionEx( TRUE, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "RtlWow64EnableFsRedirectionEx failed with status %x\n", status );
    status = pRtlWow64EnableFsRedirectionEx( TRUE, (void*)1 );
    ok( status == STATUS_ACCESS_VIOLATION, "RtlWow64EnableFsRedirectionEx failed with status %x\n", status );

    status = pRtlWow64EnableFsRedirection( FALSE );
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %x\n", status );
    status = pRtlWow64EnableFsRedirectionEx( FALSE, &cur );
    ok( !status, "RtlWow64EnableFsRedirectionEx failed status %x\n", status );
    ok( cur == 1, "RtlWow64EnableFsRedirectionEx got %u\n", cur );

    pRtlWow64EnableFsRedirectionEx( old, &cur );
}

START_TEST(directory)
{
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    if (!hntdll)
    {
        skip("not running on NT, skipping test\n");
        return;
    }

    pNtClose                = (void *)GetProcAddress(hntdll, "NtClose");
    pNtOpenFile             = (void *)GetProcAddress(hntdll, "NtOpenFile");
    pNtQueryDirectoryFile   = (void *)GetProcAddress(hntdll, "NtQueryDirectoryFile");
    pRtlCreateUnicodeStringFromAsciiz = (void *)GetProcAddress(hntdll, "RtlCreateUnicodeStringFromAsciiz");
    pRtlDosPathNameToNtPathName_U = (void *)GetProcAddress(hntdll, "RtlDosPathNameToNtPathName_U");
    pRtlInitUnicodeString   = (void *)GetProcAddress(hntdll, "RtlInitUnicodeString");
    pRtlFreeUnicodeString   = (void *)GetProcAddress(hntdll, "RtlFreeUnicodeString");
    pRtlMultiByteToUnicodeN = (void *)GetProcAddress(hntdll,"RtlMultiByteToUnicodeN");
    pRtlWow64EnableFsRedirection = (void *)GetProcAddress(hntdll,"RtlWow64EnableFsRedirection");
    pRtlWow64EnableFsRedirectionEx = (void *)GetProcAddress(hntdll,"RtlWow64EnableFsRedirectionEx");

    test_NtQueryDirectoryFile();
    test_redirection();
}
