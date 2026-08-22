/*
 * Unit tests for advpack.dll
 *
 * Copyright (C) 2005 Robert Reif
 * Copyright (C) 2005 Sami Aario
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

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <advpub.h>
#include <assert.h>
#include "wine/test.h"

/* defines for the TranslateInfString/Ex tests */
#define TEST_STRING1 "\\Application Name"
#define TEST_STRING2 "%49001%\\Application Name"

/* defines for the SetPerUserSecValues tests */
#define GUID_KEY    "SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\guid"
#define REG_VAL_EXISTS(key, value)   !RegQueryValueExA(key, value, NULL, NULL, NULL, NULL)
#define OPEN_GUID_KEY() !RegOpenKeyA(HKEY_LOCAL_MACHINE, GUID_KEY, &guid)

static HMODULE hAdvPack;
static HRESULT (WINAPI *pCloseINFEngine)(HINF);
static HRESULT (WINAPI *pDelNode)(LPCSTR,DWORD);
static HRESULT (WINAPI *pGetVersionFromFile)(LPCSTR,LPDWORD,LPDWORD,BOOL);
static HRESULT (WINAPI *pOpenINFEngine)(PCSTR,PCSTR,DWORD,HINF*,PVOID);
static HRESULT (WINAPI *pSetPerUserSecValues)(PPERUSERSECTIONA pPerUser);
static HRESULT (WINAPI *pTranslateInfString)(LPCSTR,LPCSTR,LPCSTR,LPCSTR,LPSTR,DWORD,LPDWORD,LPVOID);
static HRESULT (WINAPI *pTranslateInfStringEx)(HINF,PCSTR,PCSTR,PCSTR,PSTR,DWORD,PDWORD,PVOID);

static CHAR inf_file[MAX_PATH];
static CHAR PROG_FILES_ROOT[MAX_PATH];
static CHAR PROG_FILES[MAX_PATH];
static CHAR APP_PATH[MAX_PATH];
static DWORD APP_PATH_LEN;

static void get_progfiles_dir(void)
{
    HKEY hkey;
    DWORD size = MAX_PATH;

    RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion", &hkey);
    RegQueryValueExA(hkey, "ProgramFilesDir", NULL, NULL, (LPBYTE)PROG_FILES_ROOT, &size);
    RegCloseKey(hkey);

    lstrcpyA(PROG_FILES, PROG_FILES_ROOT + 3); /* skip C:\ */
    lstrcpyA(APP_PATH, PROG_FILES_ROOT);
    lstrcatA(APP_PATH, TEST_STRING1);
    APP_PATH_LEN = lstrlenA(APP_PATH) + 1;
}

static BOOL init_function_pointers(void)
{
    hAdvPack = LoadLibraryA("advpack.dll");

    if (!hAdvPack)
        return FALSE;

    pCloseINFEngine = (void*)GetProcAddress(hAdvPack, "CloseINFEngine");
    pDelNode = (void *)GetProcAddress(hAdvPack, "DelNode");
    pGetVersionFromFile = (void *)GetProcAddress(hAdvPack, "GetVersionFromFile");
    pOpenINFEngine = (void*)GetProcAddress(hAdvPack, "OpenINFEngine");
    pSetPerUserSecValues = (void*)GetProcAddress(hAdvPack, "SetPerUserSecValues");
    pTranslateInfString = (void *)GetProcAddress(hAdvPack, "TranslateInfString");
    pTranslateInfStringEx = (void*)GetProcAddress(hAdvPack, "TranslateInfStringEx");

    if (!pCloseINFEngine || !pDelNode || !pGetVersionFromFile ||
        !pOpenINFEngine || !pSetPerUserSecValues || !pTranslateInfString)
    {
        win_skip("Needed functions are not available\n");
        FreeLibrary(hAdvPack);
        return FALSE;
    }

    return TRUE;
}

static void version_test(void)
{
    HRESULT hr;
    DWORD major, minor;

    major = minor = 0;
    hr = pGetVersionFromFile("kernel32.dll", &major, &minor, FALSE);
    ok (hr == S_OK, "GetVersionFromFileEx(kernel32.dll) failed, returned "
        "0x%08lx\n", hr);
    trace("kernel32.dll Language ID: 0x%08lx, Codepage ID: 0x%08lx\n",
           major, minor);

    major = minor = 0;
    hr = pGetVersionFromFile("kernel32.dll", &major, &minor, TRUE);
    ok (hr == S_OK, "GetVersionFromFileEx(kernel32.dll) failed, returned "
        "0x%08lx\n", hr);
    trace("kernel32.dll version: %d.%d.%d.%d\n", HIWORD(major), LOWORD(major),
          HIWORD(minor), LOWORD(minor));

    major = minor = 0;
    hr = pGetVersionFromFile("advpack.dll", &major, &minor, FALSE);
    ok (hr == S_OK, "GetVersionFromFileEx(advpack.dll) failed, returned "
        "0x%08lx\n", hr);
    trace("advpack.dll Language ID: 0x%08lx, Codepage ID: 0x%08lx\n",
           major, minor);

    major = minor = 0;
    hr = pGetVersionFromFile("advpack.dll", &major, &minor, TRUE);
    ok (hr == S_OK, "GetVersionFromFileEx(advpack.dll) failed, returned "
        "0x%08lx\n", hr);
    trace("advpack.dll version: %d.%d.%d.%d\n", HIWORD(major), LOWORD(major),
          HIWORD(minor), LOWORD(minor));
}

static void delnode_test(void)
{
    HRESULT hr;
    HANDLE hn;
    CHAR currDir[MAX_PATH];
    UINT currDirLen;

    /* Native DelNode apparently does not support relative paths, so we use
       absolute paths for testing */
    currDirLen = GetCurrentDirectoryA(ARRAY_SIZE(currDir), currDir);
    assert(currDirLen > 0 && currDirLen < ARRAY_SIZE(currDir));

    if(currDir[currDirLen - 1] == '\\')
        currDir[--currDirLen] = 0;

    /* Simple tests; these should fail. */
    hr = pDelNode(NULL, 0);
    ok (hr == E_FAIL, "DelNode called with NULL pathname should return E_FAIL\n");
    hr = pDelNode("", 0);
    ok (hr == E_FAIL, "DelNode called with empty pathname should return E_FAIL\n");

    /* Test deletion of a file. */
    hn = CreateFileA("DelNodeTestFile1", GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(hn != INVALID_HANDLE_VALUE);
    CloseHandle(hn);
    hr = pDelNode(lstrcatA(currDir, "\\DelNodeTestFile1"), 0);
    ok (hr == S_OK, "DelNode failed deleting a single file\n");
    currDir[currDirLen] = '\0';

    /* Test deletion of an empty directory. */
    CreateDirectoryA("DelNodeTestDir", NULL);
    hr = pDelNode(lstrcatA(currDir, "\\DelNodeTestDir"), 0);
    ok (hr == S_OK, "DelNode failed deleting an empty directory\n");
    currDir[currDirLen] = '\0';

    /* Test deletion of a directory containing one file. */
    CreateDirectoryA("DelNodeTestDir", NULL);
    hn = CreateFileA("DelNodeTestDir\\DelNodeTestFile1", GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(hn != INVALID_HANDLE_VALUE);
    CloseHandle(hn);
    hr = pDelNode(lstrcatA(currDir, "\\DelNodeTestDir"), 0);
    ok (hr == S_OK, "DelNode failed deleting a directory containing one file\n");
    currDir[currDirLen] = '\0';

    /* Test deletion of a directory containing multiple files. */
    CreateDirectoryA("DelNodeTestDir", NULL);
    hn = CreateFileA("DelNodeTestDir\\DelNodeTestFile1", GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(hn != INVALID_HANDLE_VALUE);
    CloseHandle(hn);
    hn = CreateFileA("DelNodeTestDir\\DelNodeTestFile2", GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(hn != INVALID_HANDLE_VALUE);
    CloseHandle(hn);
    hn = CreateFileA("DelNodeTestDir\\DelNodeTestFile3", GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(hn != INVALID_HANDLE_VALUE);
    CloseHandle(hn);
    hr = pDelNode(lstrcatA(currDir, "\\DelNodeTestDir"), 0);
    ok (hr == S_OK, "DelNode failed deleting a directory containing multiple files\n");
}

static void WINAPIV append_str(char **str, const char *data, ...)
{
    va_list valist;

    va_start(valist, data);
    vsprintf(*str, data, valist);
    *str += strlen(*str);
    va_end(valist);
}

static void create_inf_file(void)
{
    char data[1024];
    char *ptr = data;
    DWORD dwNumberOfBytesWritten;
    HANDLE hf = CreateFileA(inf_file, GENERIC_WRITE, 0, NULL,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    append_str(&ptr, "[Version]\n");
    append_str(&ptr, "Signature=\"$Chicago$\"\n");
    append_str(&ptr, "[CustInstDestSection]\n");
    append_str(&ptr, "49001=ProgramFilesDir\n");
    append_str(&ptr, "49010=DestA,1\n");
    append_str(&ptr, "49020=DestB\n");
    append_str(&ptr, "49030=DestC\n");
    append_str(&ptr, "[ProgramFilesDir]\n");
    append_str(&ptr, "HKLM,\"Software\\Microsoft\\Windows\\CurrentVersion\",");
    append_str(&ptr, "\"ProgramFilesDir\",,\"%%24%%\\%%LProgramF%%\"\n");
    append_str(&ptr, "[section]\n");
    append_str(&ptr, "NotACustomDestination=Version\n");
    append_str(&ptr, "CustomDestination=CustInstDestSection\n");
    append_str(&ptr, "[Options.NTx86]\n");
    append_str(&ptr, "49001=ProgramFilesDir\n");
    append_str(&ptr, "InstallDir=%%49001%%\\%%DefaultAppPath%%\n");
    append_str(&ptr, "Result1=%%49010%%\n");
    append_str(&ptr, "Result2=%%49020%%\n");
    append_str(&ptr, "Result3=%%49030%%\n");
    append_str(&ptr, "CustomHDestination=CustInstDestSection\n");
    append_str(&ptr, "[Strings]\n");
    append_str(&ptr, "DefaultAppPath=\"Application Name\"\n");
    append_str(&ptr, "LProgramF=\"%s\"\n", PROG_FILES);
    append_str(&ptr, "[DestA]\n");
    append_str(&ptr, "HKLM,\"Software\\Garbage\",\"ProgramFilesDir\",,'%%24%%\\%%LProgramF%%'\n");
    append_str(&ptr, "[DestB]\n");
    append_str(&ptr, "'HKLM','Software\\Microsoft\\Windows\\CurrentVersion',");
    append_str(&ptr, "'ProgramFilesDir',,\"%%24%%\"\n");
    append_str(&ptr, "[DestC]\n");
    append_str(&ptr, "HKLM,\"Software\\Garbage\",\"ProgramFilesDir\",,'%%24%%'\n");

    WriteFile(hf, data, ptr - data, &dwNumberOfBytesWritten, NULL);
    CloseHandle(hf);
}

static void translateinfstring_test(void)
{
    HRESULT hr;
    char buffer[MAX_PATH];
    DWORD dwSize;

    create_inf_file();

    /* pass in a couple invalid parameters */
    hr = pTranslateInfString(NULL, NULL, NULL, NULL, buffer, MAX_PATH, &dwSize, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got 0x%08x\n", (UINT)hr);

    /* try to open an inf file that doesn't exist */
    hr = pTranslateInfString("c:\\a.inf", "Options.NTx86", "Options.NTx86",
                             "InstallDir", buffer, MAX_PATH, &dwSize, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || hr == E_INVALIDARG || 
       hr == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND), 
       "Expected E_INVALIDARG, 0x80070002 or 0x8007007e, got 0x%08x\n", (UINT)hr);

    if(hr == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND))
    {
        win_skip("WinNT 3.51 detected. Skipping tests for TranslateInfString()\n");
        return;
    }

    /* try a nonexistent section */
    buffer[0] = 0;
    hr = pTranslateInfString(inf_file, "idontexist", "Options.NTx86",
                             "InstallDir", buffer, MAX_PATH, &dwSize, NULL);
    if (hr == E_ACCESSDENIED)
    {
        skip("TranslateInfString is broken\n");
        return;
    }
    ok(hr == S_OK, "Expected S_OK, got 0x%08x\n", (UINT)hr);
    ok(!strcmp(buffer, TEST_STRING2), "Expected %s, got %s\n", TEST_STRING2, buffer);
    ok(dwSize == 25, "Expected size 25, got %ld\n", dwSize);

    buffer[0] = 0;
    /* try other nonexistent section */
    hr = pTranslateInfString(inf_file, "Options.NTx86", "idontexist",
                             "InstallDir", buffer, MAX_PATH, &dwSize, NULL);
    ok(hr == SPAPI_E_LINE_NOT_FOUND || hr == E_INVALIDARG, 
       "Expected SPAPI_E_LINE_NOT_FOUND or E_INVALIDARG, got 0x%08x\n", (UINT)hr);

    buffer[0] = 0;
    /* try nonexistent key */
    hr = pTranslateInfString(inf_file, "Options.NTx86", "Options.NTx86",
                             "notvalid", buffer, MAX_PATH, &dwSize, NULL);
    ok(hr == SPAPI_E_LINE_NOT_FOUND || hr == E_INVALIDARG, 
       "Expected SPAPI_E_LINE_NOT_FOUND or E_INVALIDARG, got 0x%08x\n", (UINT)hr);

    buffer[0] = 0;
    /* test the behavior of pszInstallSection */
    hr = pTranslateInfString(inf_file, "section", "Options.NTx86",
                             "InstallDir", buffer, MAX_PATH, &dwSize, NULL);
    ok(hr == ERROR_SUCCESS || hr == E_FAIL, 
       "Expected ERROR_SUCCESS or E_FAIL, got 0x%08x\n", (UINT)hr);

    if(hr == ERROR_SUCCESS)
    {
        ok(!strcmp(buffer, APP_PATH), "Expected '%s', got '%s'\n", APP_PATH, buffer);
        ok(dwSize == APP_PATH_LEN, "Expected size %ld, got %ld\n", APP_PATH_LEN, dwSize);
    }

    buffer[0] = 0;
    /* try without a pszInstallSection */
    hr = pTranslateInfString(inf_file, NULL, "Options.NTx86",
                             "InstallDir", buffer, MAX_PATH, &dwSize, NULL);
    ok(hr == S_OK, "Expected S_OK, got 0x%08x\n", (UINT)hr);
    todo_wine
    {
        ok(!strcmp(buffer, TEST_STRING2), "Expected %s, got %s\n", TEST_STRING2, buffer);
        ok(dwSize == 25, "Expected size 25, got %ld\n", dwSize);
    }

    DeleteFileA("c:\\a.inf");
    DeleteFileA(inf_file);
}

static void translateinfstringex_test(void)
{
    HINF hinf;
    HRESULT hr;
    char buffer[MAX_PATH];
    DWORD size = MAX_PATH;

    hr = pOpenINFEngine(inf_file, NULL, 0, &hinf, NULL);
    if (hr == E_UNEXPECTED)
    {
        win_skip("Skipping tests on win9x because of brokenness\n");
        return;
    }

    create_inf_file();

    /* need to see if there are any flags */

    /* try a NULL filename */
    hr = pOpenINFEngine(NULL, "Options.NTx86", 0, &hinf, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);

    /* try an empty filename */
    hr = pOpenINFEngine("", "Options.NTx86", 0, &hinf, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) /* NT+ */ ||
       hr == HRESULT_FROM_WIN32(E_UNEXPECTED) /* 9x */,
        "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND or E_UNEXPECTED), got %08lx\n", hr);

    /* try a NULL hinf */
    hr = pOpenINFEngine(inf_file, "Options.NTx86", 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);

    /* open the INF without the Install section specified */
    hr = pOpenINFEngine(inf_file, NULL, 0, &hinf, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    /* try a NULL hinf */
    hr = pTranslateInfStringEx(NULL, inf_file, "Options.NTx86", "InstallDir",
                              buffer, size, &size, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);

    /* try a NULL filename */
    hr = pTranslateInfStringEx(hinf, NULL, "Options.NTx86", "InstallDir",
                              buffer, size, &size, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);

    /* try an empty filename */
    memset(buffer, 'a', 25);
    buffer[24] = '\0';
    size = MAX_PATH;
    hr = pTranslateInfStringEx(hinf, "", "Options.NTx86", "InstallDir",
                              buffer, size, &size, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    todo_wine
    {
        ok(!strcmp(buffer, TEST_STRING2), "Expected %s, got %s\n", TEST_STRING2, buffer);
        ok(size == 25, "Expected size 25, got %ld\n", size);
    }

    /* try a NULL translate section */
    hr = pTranslateInfStringEx(hinf, inf_file, NULL, "InstallDir",
                              buffer, size, &size, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);

    /* try an empty translate section */
    hr = pTranslateInfStringEx(hinf, inf_file, "", "InstallDir",
                              buffer, size, &size, NULL);
    ok(hr == SPAPI_E_LINE_NOT_FOUND, "Expected SPAPI_E_LINE_NOT_FOUND, got %08lx\n", hr);

    /* try a NULL translate key */
    hr = pTranslateInfStringEx(hinf, inf_file, "Options.NTx86", NULL,
                              buffer, size, &size, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);

    /* try an empty translate key */
    hr = pTranslateInfStringEx(hinf, inf_file, "Options.NTx86", "",
                              buffer, size, &size, NULL);
    ok(hr == SPAPI_E_LINE_NOT_FOUND, "Expected SPAPI_E_LINE_NOT_FOUND, got %08lx\n", hr);

    /* successfully translate the string */
    memset(buffer, 'a', 25);
    buffer[24] = '\0';
    size = MAX_PATH;
    hr = pTranslateInfStringEx(hinf, inf_file, "Options.NTx86", "InstallDir",
                              buffer, size, &size, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    todo_wine
    {
        ok(!strcmp(buffer, TEST_STRING2), "Expected %s, got %s\n", TEST_STRING2, buffer);
        ok(size == 25, "Expected size 25, got %ld\n", size);
    }

    /* try a NULL hinf */
    hr = pCloseINFEngine(NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);

    /* successfully close the hinf */
    hr = pCloseINFEngine(hinf);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    /* open the inf with the install section */
    hr = pOpenINFEngine(inf_file, "section", 0, &hinf, NULL);
    if (hr == E_FAIL)
    {
        skip("can't open engine with install section (needs admin rights)\n");
        DeleteFileA(inf_file);
        return;
    }
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    /* translate the string with the install section specified */
    memset(buffer, 'a', APP_PATH_LEN);
    buffer[APP_PATH_LEN - 1] = '\0';
    size = MAX_PATH;
    hr = pTranslateInfStringEx(hinf, inf_file, "Options.NTx86", "InstallDir",
                              buffer, size, &size, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(!strcmp(buffer, APP_PATH), "Expected %s, got %s\n", APP_PATH, buffer);
    ok(size == APP_PATH_LEN, "Expected size %ld, got %ld\n", APP_PATH_LEN, size);

    /* Single quote test (Note size includes null on return from call) */
    memset(buffer, 'a', APP_PATH_LEN);
    buffer[APP_PATH_LEN - 1] = '\0';
    size = MAX_PATH;
    hr = pTranslateInfStringEx(hinf, inf_file, "Options.NTx86", "Result1",
                              buffer, size, &size, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(!lstrcmpiA(buffer, PROG_FILES_ROOT),
           "Expected %s, got %s\n", PROG_FILES_ROOT, buffer);
    ok(size == strlen(PROG_FILES_ROOT)+1, "Expected size %d, got %ld\n",
           lstrlenA(PROG_FILES_ROOT)+1, size);

    memset(buffer, 'a', APP_PATH_LEN);
    buffer[APP_PATH_LEN - 1] = '\0';
    size = MAX_PATH;
    hr = pTranslateInfStringEx(hinf, inf_file, "Options.NTx86", "Result2",
                              buffer, size, &size, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(!lstrcmpiA(buffer, PROG_FILES_ROOT),
           "Expected %s, got %s\n", PROG_FILES_ROOT, buffer);
    ok(size == strlen(PROG_FILES_ROOT)+1, "Expected size %d, got %ld\n",
           lstrlenA(PROG_FILES_ROOT)+1, size);

    {
        char drive[MAX_PATH];
        lstrcpyA(drive, PROG_FILES_ROOT);
        drive[3] = 0x00; /* Just keep the system drive plus '\' */

        memset(buffer, 'a', APP_PATH_LEN);
        buffer[APP_PATH_LEN - 1] = '\0';
        size = MAX_PATH;
        hr = pTranslateInfStringEx(hinf, inf_file, "Options.NTx86", "Result3",
                                  buffer, size, &size, NULL);
        ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
        ok(!lstrcmpiA(buffer, drive),
               "Expected %s, got %s\n", drive, buffer);
        ok(size == strlen(drive)+1, "Expected size %d, got %ld\n",
               lstrlenA(drive)+1, size);
    }

    /* close the INF again */
    hr = pCloseINFEngine(hinf);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    DeleteFileA(inf_file);

    /* Create another .inf file which is just here to trigger a wine bug */
    {
        char data[1024];
        char *ptr = data;
        DWORD dwNumberOfBytesWritten;
        HANDLE hf = CreateFileA(inf_file, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        append_str(&ptr, "[Version]\n");
        append_str(&ptr, "Signature=\"$Chicago$\"\n");
        append_str(&ptr, "[section]\n");
        append_str(&ptr, "NotACustomDestination=Version\n");
        append_str(&ptr, "CustomDestination=CustInstDestSection\n");
        append_str(&ptr, "[CustInstDestSection]\n");
        append_str(&ptr, "49010=DestA,1\n");
        append_str(&ptr, "49020=DestB\n");
        append_str(&ptr, "49030=DestC\n");
        append_str(&ptr, "49040=DestD\n");
        append_str(&ptr, "[Options.NTx86]\n");
        append_str(&ptr, "Result2=%%49030%%\n");
        append_str(&ptr, "[DestA]\n");
        append_str(&ptr, "HKLM,\"Software\\Garbage\",\"ProgramFilesDir\",,'%%24%%'\n");
        /* The point of this test is to have HKCU just before the quoted HKLM */
        append_str(&ptr, "[DestB]\n");
        append_str(&ptr, "HKCU,\"Software\\Garbage\",\"ProgramFilesDir\",,'%%24%%'\n");
        append_str(&ptr, "[DestC]\n");
        append_str(&ptr, "'HKLM','Software\\Microsoft\\Windows\\CurrentVersion',");
        append_str(&ptr, "'ProgramFilesDir',,\"%%24%%\"\n");
        append_str(&ptr, "[DestD]\n");
        append_str(&ptr, "HKLM,\"Software\\Garbage\",\"ProgramFilesDir\",,'%%24%%'\n");

        WriteFile(hf, data, ptr - data, &dwNumberOfBytesWritten, NULL);
        CloseHandle(hf);
    }

    /* open the inf with the install section */
    hr = pOpenINFEngine(inf_file, "section", 0, &hinf, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    /* Single quote test (Note size includes null on return from call) */
    memset(buffer, 'a', APP_PATH_LEN);
    buffer[APP_PATH_LEN - 1] = '\0';
    size = MAX_PATH;
    hr = pTranslateInfStringEx(hinf, inf_file, "Options.NTx86", "Result2",
                              buffer, size, &size, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(!lstrcmpiA(buffer, PROG_FILES_ROOT),
           "Expected %s, got %s\n", PROG_FILES_ROOT, buffer);
    ok(size == strlen(PROG_FILES_ROOT)+1, "Expected size %d, got %ld\n",
           lstrlenA(PROG_FILES_ROOT)+1, size);

    /* close the INF again */
    hr = pCloseINFEngine(hinf);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    DeleteFileA(inf_file);
}

static BOOL check_reg_str(HKEY hkey, LPCSTR name, LPCSTR value)
{
    DWORD size = MAX_PATH;
    char check[MAX_PATH];

    if (RegQueryValueExA(hkey, name, NULL, NULL, (LPBYTE)check, &size))
        return FALSE;

    return !lstrcmpA(check, value);
}

static BOOL check_reg_dword(HKEY hkey, LPCSTR name, DWORD value)
{
    DWORD size = sizeof(DWORD);
    DWORD check;

    if (RegQueryValueExA(hkey, name, NULL, NULL, (LPBYTE)&check, &size))
        return FALSE;

    return (check == value);
}

static void setperusersecvalues_test(void)
{
    PERUSERSECTIONA peruser;
    HRESULT hr;
    HKEY guid;

    lstrcpyA(peruser.szDispName, "displayname");
    lstrcpyA(peruser.szLocale, "locale");
    lstrcpyA(peruser.szStub, "stub");
    lstrcpyA(peruser.szVersion, "1,1,1,1");
    lstrcpyA(peruser.szCompID, "compid");
    peruser.dwIsInstalled = 1;
    peruser.bRollback = FALSE;

    /* try a NULL pPerUser */
    if (0)
    {
        /* This crashes on systems with IE7 */
        hr = pSetPerUserSecValues(NULL);
        todo_wine
        ok(hr == S_OK, "Expected S_OK, got %ld\n", hr);
        ok(!OPEN_GUID_KEY(), "Expected guid key to not exist\n");
    }

    /* at the very least, szGUID must be valid */
    peruser.szGUID[0] = '\0';
    hr = pSetPerUserSecValues(&peruser);
    ok(hr == S_OK, "Expected S_OK, got %ld\n", hr);
    ok(!OPEN_GUID_KEY(), "Expected guid key to not exist\n");

    /* set initial values */
    lstrcpyA(peruser.szGUID, "guid");
    hr = pSetPerUserSecValues(&peruser);
    if (hr == E_FAIL)
    {
        skip("SetPerUserSecValues is broken\n");
        return;
    }
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(OPEN_GUID_KEY(), "Expected guid key to exist\n");
    ok(check_reg_str(guid, NULL, "displayname"), "Expected displayname\n");
    ok(check_reg_str(guid, "ComponentID", "compid"), "Expected compid\n");
    ok(check_reg_str(guid, "Locale", "locale"), "Expected locale\n");
    ok(check_reg_str(guid, "StubPath", "stub"), "Expected stub\n");
    ok(check_reg_str(guid, "Version", "1,1,1,1"), "Expected 1,1,1,1\n");
    ok(check_reg_dword(guid, "IsInstalled", 1), "Expected 1\n");
    ok(!REG_VAL_EXISTS(guid, "OldDisplayName"), "Expected OldDisplayName to not exist\n");
    ok(!REG_VAL_EXISTS(guid, "OldLocale"), "Expected OldLocale to not exist\n");
    ok(!REG_VAL_EXISTS(guid, "OldStubPath"), "Expected OldStubPath to not exist\n");
    ok(!REG_VAL_EXISTS(guid, "OldVersion"), "Expected OldVersion to not exist\n");
    ok(!REG_VAL_EXISTS(guid, "RealStubPath"), "Expected RealStubPath to not exist\n");

    /* raise the version, but bRollback is FALSE, so vals not saved */
    lstrcpyA(peruser.szVersion, "2,1,1,1");
    hr = pSetPerUserSecValues(&peruser);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(check_reg_str(guid, NULL, "displayname"), "Expected displayname\n");
    ok(check_reg_str(guid, "ComponentID", "compid"), "Expected compid\n");
    ok(check_reg_str(guid, "Locale", "locale"), "Expected locale\n");
    ok(check_reg_str(guid, "StubPath", "stub"), "Expected stub\n");
    ok(check_reg_str(guid, "Version", "2,1,1,1"), "Expected 2,1,1,1\n");
    ok(check_reg_dword(guid, "IsInstalled", 1), "Expected 1\n");
    ok(!REG_VAL_EXISTS(guid, "OldDisplayName"), "Expected OldDisplayName to not exist\n");
    ok(!REG_VAL_EXISTS(guid, "OldLocale"), "Expected OldLocale to not exist\n");
    ok(!REG_VAL_EXISTS(guid, "OldStubPath"), "Expected OldStubPath to not exist\n");
    ok(!REG_VAL_EXISTS(guid, "OldVersion"), "Expected OldVersion to not exist\n");
    ok(!REG_VAL_EXISTS(guid, "RealStubPath"), "Expected RealStubPath to not exist\n");

    /* raise the version again, bRollback is TRUE so vals are saved */
    peruser.bRollback = TRUE;
    lstrcpyA(peruser.szVersion, "3,1,1,1");
    hr = pSetPerUserSecValues(&peruser);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(check_reg_str(guid, NULL, "displayname"), "Expected displayname\n");
    ok(check_reg_str(guid, "ComponentID", "compid"), "Expected compid\n");
    ok(check_reg_str(guid, "Locale", "locale"), "Expected locale\n");
    ok(check_reg_dword(guid, "IsInstalled", 1), "Expected 1\n");
    ok(check_reg_str(guid, "Version", "3,1,1,1"), "Expected 3,1,1,1\n");
    todo_wine
    {
        ok(check_reg_str(guid, "OldDisplayName", "displayname"), "Expected displayname\n");
        ok(check_reg_str(guid, "OldLocale", "locale"), "Expected locale\n");
        ok(check_reg_str(guid, "RealStubPath", "stub"), "Expected stub\n");
        ok(check_reg_str(guid, "OldStubPath", "stub"), "Expected stub\n");
        ok(check_reg_str(guid, "OldVersion", "2,1,1,1"), "Expected 2,1,1,1\n");
        ok(check_reg_str(guid, "StubPath",
           "rundll32.exe advpack.dll,UserInstStubWrapper guid"),
           "Expected real stub\n");
    }

    RegDeleteKeyA(HKEY_LOCAL_MACHINE, GUID_KEY);
}

START_TEST(advpack)
{
    if (!init_function_pointers())
        return;

    /* Make sure we create the temporary file in a directory
     * where we have adequate rights
     */
    GetTempPathA(MAX_PATH, inf_file);
    lstrcatA(inf_file,"test.inf");

    get_progfiles_dir();

    version_test();
    delnode_test();
    setperusersecvalues_test();
    translateinfstring_test();
    translateinfstringex_test();

    FreeLibrary(hAdvPack);
}
