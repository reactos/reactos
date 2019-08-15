/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     wininet Redirection testcase
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include <apitest.h>
#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <wininet.h>

#define FILENAME "redirection-testdata.txt"
#define TESTDATA "This is a test data.\r\n"

static void DoDownload1(const char *url, const char *filename)
{
    HANDLE hFile;
    HINTERNET hInternet, hConnect;
    static const char s_header[] = "Accept: */" "*\r\n\r\n";
    BYTE buffer[256];
    DWORD cbRead;
    BOOL ret;

    hFile = CreateFileA(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "hFile was INVALID_HANDLE_VALUE.\n");

    hInternet = InternetOpenA(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    ok(hInternet != NULL, "hInternet was NULL.\n");

    hConnect = InternetOpenUrlA(hInternet, url, s_header, lstrlenA(s_header),
                                INTERNET_FLAG_DONT_CACHE, 0);
    ok(hConnect != NULL, "hConnect was NULL.\n");

    for (;;)
    {
        Sleep(100);

        ret = InternetReadFile(hConnect, buffer, ARRAYSIZE(buffer), &cbRead);
        if (!ret || !cbRead)
            break;

        if (!WriteFile(hFile, buffer, cbRead, &cbRead, NULL))
        {
            ok(0, "WriteFile returns FALSE.\n");
            break;
        }
    }

    ok_int(InternetCloseHandle(hConnect), TRUE);
    ok_int(InternetCloseHandle(hInternet), TRUE);
    CloseHandle(hFile);
}

static void DoDownload2(const char *url, const char *filename)
{
    FILE *fp;
    char buf[256];
    DoDownload1(url, filename);
    ok_int(GetFileAttributesA(FILENAME) != INVALID_FILE_ATTRIBUTES, TRUE);
    fp = fopen(FILENAME, "rb");
    ok(fp != NULL, "fp was NULL.\n");
    ok(fgets(buf, ARRAYSIZE(buf), fp) != NULL, "fgets failed.\n");
    ok_str(buf, TESTDATA);
    fclose(fp);
    DeleteFileA(FILENAME);
}

START_TEST(Redirection)
{
    // https://tinyurl.com/y3euesr5
    // -->
    // https://raw.githubusercontent.com/katahiromz/downloads/master/redirection-testdata.txt
    DoDownload2("https://tinyurl.com/y3euesr5", FILENAME);

    DoDownload2(
        "https://raw.githubusercontent.com/katahiromz/downloads/master/redirection-testdata.txt",
        FILENAME);
}
