/*
 * Wininet - ftp tests
 *
 * Copyright 2007 Paul Vriens
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

/*
 * FIXME:
 *     Use InternetGetLastResponseInfo when the last error is set to ERROR_INTERNET_EXTENDED_ERROR.
 * TODO:
 *     Add W-function tests.
 *     Add missing function tests:
 *         FtpFindFirstFile
 *         FtpGetCurrentDirectory
 *         FtpGetFileSize
 *         FtpSetCurrentDirectory
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winsock.h"

#include "wine/test.h"

static void test_getfile_no_open(void)
{
    BOOL      bRet;

    /* Invalid internet handle, the others are valid parameters */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(NULL, "welcome.msg", "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_NOT_INITIALIZED ||
         GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INTERNET_NOT_INITIALIZED or ERROR_INVALID_HANDLE (win98), got %d\n", GetLastError());
}

static void test_connect(HINTERNET hInternet)
{
    HINTERNET hFtp;

    /* Try a few username/password combinations:
     * anonymous : NULL
     * NULL      : IEUser@
     * NULL      : NULL
     */

    SetLastError(0xdeadbeef);
    hFtp = InternetConnect(hInternet, "ftp.winehq.org", INTERNET_DEFAULT_FTP_PORT, "anonymous", NULL, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
    if (hFtp)  /* some servers accept an empty password */
    {
        ok ( GetLastError() == ERROR_SUCCESS, "ERROR_SUCCESS, got %d\n", GetLastError());
        InternetCloseHandle(hFtp);
    }
    else
        ok ( GetLastError() == ERROR_INTERNET_LOGIN_FAILURE,
             "Expected ERROR_INTERNET_LOGIN_FAILURE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    hFtp = InternetConnect(hInternet, "ftp.winehq.org", INTERNET_DEFAULT_FTP_PORT, NULL, "IEUser@", INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
    ok ( hFtp == NULL, "Expected InternetConnect to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Using a NULL username and password will be interpreted as anonymous ftp. The username will be 'anonymous' the password
     * is created via some simple heuristics (see dlls/wininet/ftp.c).
     * On Wine this registry key is not set by default so (NULL, NULL) will result in anonymous ftp with an (most likely) not
     * accepted password (the username).
     * If the first call fails because we get an ERROR_INTERNET_LOGIN_FAILURE, we try again with a (more) correct password.
     */

    SetLastError(0xdeadbeef);
    hFtp = InternetConnect(hInternet, "ftp.winehq.org", INTERNET_DEFAULT_FTP_PORT, NULL, NULL, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
    if (!hFtp && (GetLastError() == ERROR_INTERNET_LOGIN_FAILURE))
    {
        /* We are most likely running on a clean Wine install or a Windows install where the registry key is removed */
        SetLastError(0xdeadbeef);
        hFtp = InternetConnect(hInternet, "ftp.winehq.org", INTERNET_DEFAULT_FTP_PORT, "anonymous", "IEUser@", INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
    }
    ok ( hFtp != NULL, "InternetConnect failed : %d\n", GetLastError());
    ok ( GetLastError() == ERROR_SUCCESS,
        "ERROR_SUCCESS, got %d\n", GetLastError());
}

static void test_createdir(HINTERNET hFtp, HINTERNET hConnect)
{
    BOOL      bRet;

    /* Invalid internet handle, the other is a valid parameter */
    SetLastError(0xdeadbeef);
    bRet = FtpCreateDirectoryA(NULL, "new_directory_deadbeef");
    ok ( bRet == FALSE, "Expected FtpCreateDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    /* No directory-name */
    SetLastError(0xdeadbeef);
    bRet = FtpCreateDirectoryA(hFtp, NULL);
    ok ( bRet == FALSE, "Expected FtpCreateDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Parameters are OK, but we shouldn't be allowed to create the directory */
    SetLastError(0xdeadbeef);
    bRet = FtpCreateDirectoryA(hFtp, "new_directory_deadbeef");
    ok ( bRet == FALSE, "Expected FtpCreateDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR,
        "Expected ERROR_INTERNET_EXTENDED_ERROR, got %d\n", GetLastError());

    /* One small test to show that handle type is checked before parameters */
    SetLastError(0xdeadbeef);
    bRet = FtpCreateDirectoryA(hConnect, NULL);
    ok ( bRet == FALSE, "Expected FtpCreateDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    bRet = FtpCreateDirectoryA(hConnect, "new_directory_deadbeef");
    ok ( bRet == FALSE, "Expected FtpCreateDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());
}

static void test_deletefile(HINTERNET hFtp, HINTERNET hConnect)
{
    BOOL      bRet;

    /* Invalid internet handle, the other is a valid parameter */
    SetLastError(0xdeadbeef);
    bRet = FtpDeleteFileA(NULL, "non_existent_file_deadbeef");
    ok ( bRet == FALSE, "Expected FtpDeleteFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    /* No filename */
    SetLastError(0xdeadbeef);
    bRet = FtpDeleteFileA(hFtp, NULL);
    ok ( bRet == FALSE, "Expected FtpDeleteFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Parameters are OK but remote file should not be there */
    SetLastError(0xdeadbeef);
    bRet = FtpDeleteFileA(hFtp, "non_existent_file_deadbeef");
    ok ( bRet == FALSE, "Expected FtpDeleteFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR,
        "Expected ERROR_INTERNET_EXTENDED_ERROR, got %d\n", GetLastError());

    /* One small test to show that handle type is checked before parameters */
    SetLastError(0xdeadbeef);
    bRet = FtpDeleteFileA(hConnect, NULL);
    ok ( bRet == FALSE, "Expected FtpDeleteFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    bRet = FtpDeleteFileA(hConnect, "non_existent_file_deadbeef");
    ok ( bRet == FALSE, "Expected FtpCreateDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());
}

static void test_getfile(HINTERNET hFtp, HINTERNET hConnect)
{
    BOOL      bRet;
    HANDLE    hFile;

    /* The order of checking is:
     *
     *   All parameters except 'session handle' and 'condition flags'
     *   Session handle
     *   Session handle type
     *   Condition flags
     */

    /* Test to show the parameter checking order depends on the Windows version */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(NULL, NULL, "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_HANDLE ||
         GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_HANDLE (win98) or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Test to show session handle is checked before 'condition flags' */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(NULL, "welcome.msg", "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, 5, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    /* Make sure we start clean */

    DeleteFileA("should_be_non_existing_deadbeef");
    DeleteFileA("should_also_be_non_existing_deadbeef");

    /* No remote file */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hFtp, NULL, "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    ok (GetFileAttributesA("should_be_non_existing_deadbeef") == INVALID_FILE_ATTRIBUTES,
        "Local file should not have been created\n");
    DeleteFileA("should_be_non_existing_deadbeef");

    /* No local file */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hFtp, "welcome.msg", NULL, FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Zero attributes */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hFtp, "welcome.msg", "should_be_existing_non_deadbeef", FALSE, 0, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == TRUE, "Expected FtpGetFileA to succeed\n");
    ok (GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    ok (GetFileAttributesA("should_be_existing_non_deadbeef") != INVALID_FILE_ATTRIBUTES,
        "Local file should have been created\n");
    DeleteFileA("should_be_existing_non_deadbeef");

    /* Illegal condition flags */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hFtp, "welcome.msg", "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, 0xffffffff, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR || GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INTERNET_EXTENDED_ERROR or ERROR_INVALID_PARAMETER (win98), got %d\n", GetLastError());
    ok (GetFileAttributesA("should_be_non_existing_deadbeef") == INVALID_FILE_ATTRIBUTES,
        "Local file should not have been created\n");
    DeleteFileA("should_be_non_existing_deadbeef");

    /* Remote file doesn't exist (and local doesn't exist as well) */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hFtp, "should_be_non_existing_deadbeef", "should_also_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR,
        "Expected ERROR_INTERNET_EXTENDED_ERROR, got %d\n", GetLastError());
    /* Currently Wine always creates the local file (even on failure) which is not correct, hence the test */
    todo_wine
    ok (GetFileAttributesA("should_also_be_non_existing_deadbeef") == INVALID_FILE_ATTRIBUTES,
        "Local file should not have been created\n");

    DeleteFileA("should_also_be_non_existing_deadbeef");

    /* Same call as the previous but now the local file does exists. Windows just removes the file if the call fails
     * even if the local existed before!
     */

    /* Create a temporary local file */
    SetLastError(0xdeadbeef);
    hFile = CreateFileA("should_also_be_non_existing_deadbeef", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok ( hFile != NULL, "Error creating a local file : %d\n", GetLastError());
    CloseHandle(hFile);
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hFtp, "should_be_non_existing_deadbeef", "should_also_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR,
        "Expected ERROR_INTERNET_EXTENDED_ERROR, got %d\n", GetLastError());
    /* Currently Wine always creates the local file (even on failure) which is not correct, hence the test */
    todo_wine
    ok (GetFileAttributesA("should_also_be_non_existing_deadbeef") == INVALID_FILE_ATTRIBUTES,
        "Local file should not have been created\n");

    DeleteFileA("should_also_be_non_existing_deadbeef");

    /* This one should succeed */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hFtp, "welcome.msg", "should_be_existing_non_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == TRUE, "Expected FtpGetFileA to fail\n");
    ok ( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", GetLastError());

    if (GetFileAttributesA("should_be_existing_non_deadbeef") != INVALID_FILE_ATTRIBUTES)
    {
        /* Should succeed as fFailIfExists is set to FALSE (meaning don't fail if local file exists) */
        SetLastError(0xdeadbeef);
        bRet = FtpGetFileA(hFtp, "welcome.msg", "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
        ok ( bRet == TRUE, "Expected FtpGetFileA to succeed\n");
        ok ( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got %d\n", GetLastError());

        /* Should fail as fFailIfExists is set to TRUE */
        SetLastError(0xdeadbeef);
        bRet = FtpGetFileA(hFtp, "welcome.msg", "should_be_non_existing_deadbeef", TRUE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
        ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
        ok ( GetLastError() == ERROR_FILE_EXISTS,
            "Expected ERROR_FILE_EXISTS, got %d\n", GetLastError());

        /* Prove that the existence of the local file is checked first (or at least reported last) */
        SetLastError(0xdeadbeef);
        bRet = FtpGetFileA(hFtp, "should_be_non_existing_deadbeef", "should_be_non_existing_deadbeef", TRUE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
        ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
        ok ( GetLastError() == ERROR_FILE_EXISTS,
            "Expected ERROR_FILE_EXISTS, got %d\n", GetLastError());

        DeleteFileA("should_be_existing_non_deadbeef");
    }

    /* Test to show the parameter checking order depends on the Windows version */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hConnect, NULL, "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE ||
         GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE (win98) or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Test to show that 'session handle type' is checked before 'condition flags' */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hConnect, "welcome.msg", "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, 5, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hConnect, "should_be_non_existing_deadbeef", "should_be_non_existing_deadbeef", TRUE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());
}

static void test_openfile(HINTERNET hFtp, HINTERNET hConnect)
{
    HINTERNET hOpenFile;

    /* Invalid internet handle, the rest are valid parameters */
    SetLastError(0xdeadbeef);
    hOpenFile = FtpOpenFileA(NULL, "welcome.msg", GENERIC_READ, FTP_TRANSFER_TYPE_ASCII, 0);
    ok ( !hOpenFile, "Expected FtpOpenFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    InternetCloseHandle(hOpenFile); /* Just in case */

    /* No filename */
    SetLastError(0xdeadbeef);
    hOpenFile = FtpOpenFileA(hFtp, NULL, GENERIC_READ, FTP_TRANSFER_TYPE_ASCII, 0);
    ok ( !hOpenFile, "Expected FtpOpenFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    InternetCloseHandle(hOpenFile); /* Just in case */

    /* Illegal access flags */
    SetLastError(0xdeadbeef);
    hOpenFile = FtpOpenFileA(hFtp, "welcome.msg", 0, FTP_TRANSFER_TYPE_ASCII, 0);
    ok ( !hOpenFile, "Expected FtpOpenFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    InternetCloseHandle(hOpenFile); /* Just in case */

    /* Illegal combination of access flags */
    SetLastError(0xdeadbeef);
    hOpenFile = FtpOpenFileA(hFtp, "welcome.msg", GENERIC_READ|GENERIC_WRITE, FTP_TRANSFER_TYPE_ASCII, 0);
    ok ( !hOpenFile, "Expected FtpOpenFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    InternetCloseHandle(hOpenFile); /* Just in case */

    /* Illegal condition flags */
    SetLastError(0xdeadbeef);
    hOpenFile = FtpOpenFileA(hFtp, "welcome.msg", GENERIC_READ, 0xffffffff, 0);
    ok ( !hOpenFile, "Expected FtpOpenFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR || GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INTERNET_EXTENDED_ERROR or ERROR_INVALID_PARAMETER (win98), got %d\n", GetLastError());
    InternetCloseHandle(hOpenFile); /* Just in case */

    SetLastError(0xdeadbeef);
    hOpenFile = FtpOpenFileA(hFtp, "welcome.msg", GENERIC_READ, FTP_TRANSFER_TYPE_ASCII, 0);
    ok ( hOpenFile != NULL, "Expected FtpOpenFileA to succeed\n");
    ok ( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", GetLastError());

    if (hOpenFile)
    {
        BOOL bRet;
        HINTERNET hOpenFile2;
        HANDLE    hFile;

        /* We have a handle so all ftp calls should fail (TODO: Put all ftp-calls in here) */
        SetLastError(0xdeadbeef);
        bRet = FtpCreateDirectoryA(hFtp, "new_directory_deadbeef");
        ok ( bRet == FALSE, "Expected FtpCreateDirectoryA to fail\n");
        ok ( GetLastError() == ERROR_FTP_TRANSFER_IN_PROGRESS,
            "Expected ERROR_FTP_TRANSFER_IN_PROGRESS, got %d\n", GetLastError());

        SetLastError(0xdeadbeef);
        bRet = FtpDeleteFileA(hFtp, "non_existent_file_deadbeef");
        ok ( bRet == FALSE, "Expected FtpDeleteFileA to fail\n");
        ok ( GetLastError() == ERROR_FTP_TRANSFER_IN_PROGRESS,
            "Expected ERROR_FTP_TRANSFER_IN_PROGRESS, got %d\n", GetLastError());

        SetLastError(0xdeadbeef);
        bRet = FtpGetFileA(hFtp, "welcome.msg", "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
        ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
        ok ( GetLastError() == ERROR_FTP_TRANSFER_IN_PROGRESS,
            "Expected ERROR_FTP_TRANSFER_IN_PROGRESS, got %d\n", GetLastError());
        DeleteFileA("should_be_non_existing_deadbeef"); /* Just in case */

        SetLastError(0xdeadbeef);
        hOpenFile2 = FtpOpenFileA(hFtp, "welcome.msg", GENERIC_READ, FTP_TRANSFER_TYPE_ASCII, 0);
        ok ( bRet == FALSE, "Expected FtpOpenFileA to fail\n");
        ok ( GetLastError() == ERROR_FTP_TRANSFER_IN_PROGRESS,
            "Expected ERROR_FTP_TRANSFER_IN_PROGRESS, got %d\n", GetLastError());
        InternetCloseHandle(hOpenFile2); /* Just in case */

        /* Create a temporary local file */
        SetLastError(0xdeadbeef);
        hFile = CreateFileA("now_existing_local", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        ok ( hFile != NULL, "Error creating a local file : %d\n", GetLastError());
        CloseHandle(hFile);
        SetLastError(0xdeadbeef);
        bRet = FtpPutFileA(hFtp, "now_existing_local", "non_existing_remote", FTP_TRANSFER_TYPE_UNKNOWN, 0);
        ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
        ok ( GetLastError() == ERROR_FTP_TRANSFER_IN_PROGRESS,
            "Expected ERROR_FTP_TRANSFER_IN_PROGRESS, got %d\n", GetLastError());
        DeleteFileA("now_existing_local");

        SetLastError(0xdeadbeef);
        bRet = FtpRemoveDirectoryA(hFtp, "should_be_non_existing_deadbeef_dir");
        ok ( bRet == FALSE, "Expected FtpRemoveDirectoryA to fail\n");
        ok ( GetLastError() == ERROR_FTP_TRANSFER_IN_PROGRESS,
            "Expected ERROR_FTP_TRANSFER_IN_PROGRESS, got %d\n", GetLastError());

        SetLastError(0xdeadbeef);
        bRet = FtpRenameFileA(hFtp , "should_be_non_existing_deadbeef", "new");
        ok ( bRet == FALSE, "Expected FtpRenameFileA to fail\n");
        ok ( GetLastError() == ERROR_FTP_TRANSFER_IN_PROGRESS,
            "Expected ERROR_FTP_TRANSFER_IN_PROGRESS, got %d\n", GetLastError());
    }

    InternetCloseHandle(hOpenFile);

    /* One small test to show that handle type is checked before parameters */
    SetLastError(0xdeadbeef);
    hOpenFile = FtpOpenFileA(hConnect, "welcome.msg", GENERIC_READ, 5, 0);
    ok ( !hOpenFile, "Expected FtpOpenFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());
    InternetCloseHandle(hOpenFile); /* Just in case */

    SetLastError(0xdeadbeef);
    hOpenFile = FtpOpenFileA(hConnect, "welcome.msg", GENERIC_READ, FTP_TRANSFER_TYPE_ASCII, 0);
    ok ( hOpenFile == NULL, "Expected FtpOpenFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    InternetCloseHandle(hOpenFile); /* Just in case */
}

static void test_putfile(HINTERNET hFtp, HINTERNET hConnect)
{
    BOOL      bRet;
    HANDLE    hFile;

    /* The order of checking is:
     *
     *   All parameters except 'session handle' and 'condition flags'
     *   Session handle
     *   Session handle type
     *   Condition flags
     */

    /* Test to show the parameter checking order depends on the Windows version */
    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(NULL, NULL, "non_existing_remote", FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_HANDLE ||
         GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_HANDLE (win98) or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Test to show session handle is checked before 'condition flags' */
    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(NULL, "non_existing_local", "non_existing_remote", 5, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    /* Start clean */
    DeleteFileA("non_existing_local");

    /* No local file given */
    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(hFtp, NULL, "non_existing_remote", FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* No remote file given */
    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(hFtp, "non_existing_local", NULL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Illegal condition flags */
    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(hFtp, "non_existing_local", "non_existing_remote", 5, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    ok ( GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_FILE_NOT_FOUND or ERROR_INVALID_PARAMETER (win98), got %d\n", GetLastError());

    /* Parameters are OK but local file doesn't exist */
    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(hFtp, "non_existing_local", "non_existing_remote", FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    ok ( GetLastError() == ERROR_FILE_NOT_FOUND,
        "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());

    /* Create a temporary local file */
    SetLastError(0xdeadbeef);
    hFile = CreateFileA("now_existing_local", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok ( hFile != NULL, "Error creating a local file : %d\n", GetLastError());
    CloseHandle(hFile);

    /* Local file exists but we shouldn't be allowed to 'put' the file */
    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(hFtp, "now_existing_local", "non_existing_remote", FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR,
        "Expected ERROR_INTERNET_EXTENDED_ERROR, got %d\n", GetLastError());

    DeleteFileA("now_existing_local");

    /* Test to show the parameter checking order depends on the Windows version */
    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(hConnect, NULL, "non_existing_remote", FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE ||
         GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE (win98) or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Test to show that 'session handle type' is checked before 'condition flags' */
    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(hConnect, "non_existing_local", "non_existing_remote", 5, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(hConnect, "non_existing_local", "non_existing_remote", FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());
}

static void test_removedir(HINTERNET hFtp, HINTERNET hConnect)
{
    BOOL      bRet;

    /* Invalid internet handle, the other is a valid parameter */
    SetLastError(0xdeadbeef);
    bRet = FtpRemoveDirectoryA(NULL, "should_be_non_existing_deadbeef_dir");
    ok ( bRet == FALSE, "Expected FtpRemoveDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    /* No remote directory given */
    SetLastError(0xdeadbeef);
    bRet = FtpRemoveDirectoryA(hFtp, NULL);
    ok ( bRet == FALSE, "Expected FtpRemoveDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Remote directory doesn't exist */
    SetLastError(0xdeadbeef);
    bRet = FtpRemoveDirectoryA(hFtp, "should_be_non_existing_deadbeef_dir");
    ok ( bRet == FALSE, "Expected FtpRemoveDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR,
        "Expected ERROR_INTERNET_EXTENDED_ERROR, got %d\n", GetLastError());

    /* We shouldn't be allowed to remove that directory */
    SetLastError(0xdeadbeef);
    bRet = FtpRemoveDirectoryA(hFtp, "pub");
    ok ( bRet == FALSE, "Expected FtpRemoveDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR,
        "Expected ERROR_INTERNET_EXTENDED_ERROR, got %d\n", GetLastError());

    /* One small test to show that handle type is checked before parameters */
    SetLastError(0xdeadbeef);
    bRet = FtpRemoveDirectoryA(hConnect, NULL);
    ok ( bRet == FALSE, "Expected FtpRemoveDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    bRet = FtpRemoveDirectoryA(hConnect, "should_be_non_existing_deadbeef_dir");
    ok ( bRet == FALSE, "Expected FtpRemoveDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());
}

static void test_renamefile(HINTERNET hFtp, HINTERNET hConnect)
{
    BOOL      bRet;

    /* Invalid internet handle, the rest are valid parameters */
    SetLastError(0xdeadbeef);
    bRet = FtpRenameFileA(NULL , "should_be_non_existing_deadbeef", "new");
    ok ( bRet == FALSE, "Expected FtpRenameFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    /* No 'existing' file */
    SetLastError(0xdeadbeef);
    bRet = FtpRenameFileA(hFtp , NULL, "new");
    ok ( bRet == FALSE, "Expected FtpRenameFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* No new file */
    SetLastError(0xdeadbeef);
    bRet = FtpRenameFileA(hFtp , "should_be_non_existing_deadbeef", NULL);
    ok ( bRet == FALSE, "Expected FtpRenameFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Existing file shouldn't be there */
    SetLastError(0xdeadbeef);
    bRet = FtpRenameFileA(hFtp , "should_be_non_existing_deadbeef", "new");
    ok ( bRet == FALSE, "Expected FtpRenameFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR,
        "Expected ERROR_INTERNET_EXTENDED_ERROR, got %d\n", GetLastError());

    /* One small test to show that handle type is checked before parameters */
    SetLastError(0xdeadbeef);
    bRet = FtpRenameFileA(hConnect , "should_be_non_existing_deadbeef", NULL);
    ok ( bRet == FALSE, "Expected FtpRenameFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    bRet = FtpRenameFileA(hConnect , "should_be_non_existing_deadbeef", "new");
    ok ( bRet == FALSE, "Expected FtpRenameFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());
}

static void test_command(HINTERNET hFtp, HINTERNET hConnect)
{
    BOOL ret;
    DWORD error;
    unsigned int i;
    static const struct
    {
        BOOL  ret;
        DWORD error;
        const char *cmd;
    }
    command_test[] =
    {
        { FALSE, ERROR_INVALID_PARAMETER,       NULL },
        { FALSE, ERROR_INVALID_PARAMETER,       "" },
        { FALSE, ERROR_INTERNET_EXTENDED_ERROR, "HELO" },
        { FALSE, ERROR_INTERNET_EXTENDED_ERROR, "SIZE " },
        { FALSE, ERROR_INTERNET_EXTENDED_ERROR, " SIZE" },
        { FALSE, ERROR_INTERNET_EXTENDED_ERROR, "SIZE " },
        { FALSE, ERROR_INTERNET_EXTENDED_ERROR, "SIZE /welcome.msg /welcome.msg" },
        { FALSE, ERROR_INTERNET_EXTENDED_ERROR, "SIZE  /welcome.msg" },
        { FALSE, ERROR_INTERNET_EXTENDED_ERROR, "SIZE /welcome.msg " },
        { TRUE,  ERROR_SUCCESS,                 "SIZE\t/welcome.msg" },
        { TRUE,  ERROR_SUCCESS,                 "SIZE /welcome.msg" },
        { FALSE, ERROR_INTERNET_EXTENDED_ERROR, "PWD /welcome.msg" },
        { TRUE,  ERROR_SUCCESS,                 "PWD" },
        { TRUE,  ERROR_SUCCESS,                 "PWD\r\n" }
    };

    for (i = 0; i < sizeof(command_test) / sizeof(command_test[0]); i++)
    {
        SetLastError(0xdeadbeef);
        ret = FtpCommandA(hFtp, FALSE, FTP_TRANSFER_TYPE_ASCII, command_test[i].cmd, 0, NULL);
        error = GetLastError();

        ok(ret == command_test[i].ret, "%d: expected FtpCommandA to %s\n", i, command_test[i].ret ? "succeed" : "fail");
        ok(error == command_test[i].error, "%d: expected error %u, got %u\n", i, command_test[i].error, error);
    }
}

START_TEST(ftp)
{
    HANDLE hInternet, hFtp, hHttp;

    SetLastError(0xdeadbeef);
    hInternet = InternetOpen("winetest", 0, NULL, NULL, 0);
    ok(hInternet != NULL, "InternetOpen failed: %u\n", GetLastError());

    hFtp = InternetConnect(hInternet, "ftp.winehq.org", INTERNET_DEFAULT_FTP_PORT, "anonymous", NULL, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
    if (!hFtp)
    {
        InternetCloseHandle(hInternet);
        skip("No ftp connection could be made to ftp.winehq.org\n");
        return;
    }
    hHttp = InternetConnect(hInternet, "www.winehq.org", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hHttp)
    {
        InternetCloseHandle(hFtp);
        InternetCloseHandle(hInternet);
        skip("No http connection could be made to www.winehq.org\n");
        return;
    }

    /* The first call should always be a proper InternetOpen, if not
     * several calls will return ERROR_INTERNET_NOT_INITIALIZED when
     * all parameters are correct but no session handle is given. Whereas
     * the same call will return ERROR_INVALID_HANDLE if an InternetOpen
     * is done before.
     * The following test will show that behaviour, where the tests inside
     * the other sub-tests will show the other situation.
     */
    test_getfile_no_open();
    test_connect(hInternet);
    test_createdir(hFtp, hHttp);
    test_deletefile(hFtp, hHttp);
    test_getfile(hFtp, hHttp);
    test_openfile(hFtp, hHttp);
    test_putfile(hFtp, hHttp);
    test_removedir(hFtp, hHttp);
    test_renamefile(hFtp, hHttp);
    test_command(hFtp, hHttp);

    InternetCloseHandle(hHttp);
    InternetCloseHandle(hFtp);
    InternetCloseHandle(hInternet);
}
