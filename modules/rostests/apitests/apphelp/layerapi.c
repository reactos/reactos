/*
 * PROJECT:     apphelp_apitest
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for (registry)layer manipulation api's
 * COPYRIGHT:   Copyright 2015-2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <shlwapi.h>
#include <winnt.h>
#ifdef __REACTOS__
#include <ntndk.h>
#else
#include <winternl.h>
#endif
#include <winerror.h>
#include <stdio.h>
#include <strsafe.h>

#include "wine/test.h"
#include "apitest_iathook.h"
#include "apphelp_apitest.h"

#define GPLK_USER 1
#define GPLK_MACHINE 2
#define MAX_LAYER_LENGTH 256
#define LAYER_APPLY_TO_SYSTEM_EXES 1


static HMODULE hdll;
static BOOL(WINAPI *pAllowPermLayer)(PCWSTR path);
static BOOL(WINAPI *pSdbSetPermLayerKeys)(PCWSTR wszPath, PCWSTR wszLayers, BOOL bMachine);
static BOOL(WINAPI *pSdbGetPermLayerKeys)(PCWSTR wszPath, PWSTR pwszLayers, PDWORD pdwBytes, DWORD dwFlags);
static BOOL(WINAPI *pSetPermLayerState)(PCWSTR wszPath, PCWSTR wszLayer, DWORD dwFlags, BOOL bMachine, BOOL bEnable);


/* Helper function to disable Wow64 redirection on an os that reports it being enabled. */
static DWORD g_QueryFlag = 0xffffffff;
static DWORD QueryFlag(void)
{
    if (g_QueryFlag == 0xffffffff)
    {
        ULONG_PTR wow64_ptr = 0;
        NTSTATUS status = NtQueryInformationProcess(NtCurrentProcess(), ProcessWow64Information, &wow64_ptr, sizeof(wow64_ptr), NULL);
        g_QueryFlag = (NT_SUCCESS(status) && wow64_ptr != 0) ? KEY_WOW64_64KEY : 0;
    }
    return g_QueryFlag;
}

/* Helper function to prepare the registry key with a value. */
static BOOL setLayerValue(BOOL bMachine, const char* valueName, const char* value)
{
    HKEY key = NULL;
    LSTATUS lstatus = RegCreateKeyExA(bMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers", 0, NULL, 0, QueryFlag() | KEY_SET_VALUE, NULL, &key, NULL);
    if (lstatus == ERROR_SUCCESS)
    {
        if (value)
            lstatus = RegSetValueExA(key, valueName, 0, REG_SZ, (const BYTE*)value, (DWORD)strlen(value)+1);
        else
        {
            lstatus = RegDeleteValueA(key, valueName);
            lstatus = (lstatus == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : lstatus);
        }
        RegCloseKey(key);
    }
    return lstatus == ERROR_SUCCESS;
}


static void expect_LayerValue_imp(BOOL bMachine, const char* valueName, const char* value)
{
    HKEY key = NULL;
    LSTATUS lstatus = RegCreateKeyExA(bMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers", 0, NULL, 0, QueryFlag() | KEY_QUERY_VALUE, NULL, &key, NULL);
    winetest_ok(lstatus == ERROR_SUCCESS, "Expected to be able to open a registry key\n");
    if (lstatus == ERROR_SUCCESS)
    {
        char data[512] = { 0 };
        DWORD dwType = 0;
        DWORD dwDataLen = sizeof(data);
        lstatus = RegQueryValueExA(key, valueName, NULL, &dwType, (LPBYTE)data, &dwDataLen);
        if (value)
        {
            winetest_ok(lstatus == ERROR_SUCCESS, "Expected to get a valid value, err: %u\n", lstatus);
            if (lstatus == ERROR_SUCCESS)
            {
                winetest_ok(dwType == REG_SZ, "Expected the type to be REG_SZ, was: %u\n", dwType);
                winetest_ok(!strcmp(data, value), "Expected the data to be: '%s', was: '%s'\n", value, data);
            }
        }
        else
        {
            winetest_ok(lstatus == ERROR_FILE_NOT_FOUND, "Expected not to find the value %s\n", valueName);
        }
        RegCloseKey(key);
    }
}

static void expect_LayerValue_imp2(BOOL bMachine, const char* valueName, const char* value, int use_alt, const char* alt_value)
{
    expect_LayerValue_imp(bMachine, valueName, use_alt ? alt_value : value);
}


void expect_Sdb_imp(PCSTR path, DWORD type, BOOL result, DWORD lenResult, PCSTR stringResult)
{
    WCHAR pathW[MAX_PATH], buffer[MAX_LAYER_LENGTH] = { 0 };
    char resultBuffer[MAX_LAYER_LENGTH] = { 0 };
    DWORD dwBufSize = sizeof(buffer);

    /* In case of a failure, the buffer size is sometimes set to 0, and sometimes not touched,
        depending on the version. Either case is fine, since the function returns FALSE anyway. */

    MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, MAX_PATH);

    winetest_ok(pSdbGetPermLayerKeys(pathW, buffer, &dwBufSize, type) == result, "Expected pSdbGetPermLayerKeys to %s\n", (result ? "succeed" : "fail"));
    if (!result && lenResult == 0xffffffff)
        winetest_ok(dwBufSize == 0 || dwBufSize == sizeof(buffer), "Expected dwBufSize to be 0 or %u, was %u\n", sizeof(buffer), dwBufSize);
    else
        winetest_ok(dwBufSize == lenResult ||
            /* W2k3 is off by 2 when concatenating user / machine */
            broken(g_WinVersion < WINVER_VISTA && type == (GPLK_MACHINE|GPLK_USER) && (lenResult + 2) == dwBufSize),
                "Expected dwBufSize to be %u, was %u\n", lenResult, dwBufSize);
    if (result)
    {
        winetest_ok(lstrlenW(buffer) * sizeof(WCHAR) + sizeof(WCHAR) == lenResult, "Expected lstrlenW(buffer)*2+2 to be %u, was %u\n",
            lenResult, lstrlenW(buffer) * sizeof(WCHAR) + sizeof(WCHAR));
    }
    WideCharToMultiByte(CP_ACP, 0, buffer, -1, resultBuffer, sizeof(resultBuffer), NULL, NULL);
    winetest_ok(!strcmp(stringResult, resultBuffer), "Expected the result to be '%s', was '%s'\n", stringResult, resultBuffer);

    if (result)
    {
        UNICODE_STRING pathNT;

        if (RtlDosPathNameToNtPathName_U(pathW, &pathNT, NULL, NULL))
        {
            memset(buffer, 0, sizeof(buffer));
            dwBufSize = sizeof(buffer);
            winetest_ok(pSdbGetPermLayerKeys(pathNT.Buffer, buffer, &dwBufSize, type) == FALSE, "Expected pSdbGetPermLayerKeys to fail for NT path\n");

            RtlFreeUnicodeString(&pathNT);
        }
    }

}


/* In case of a failure, let the location be from where the function was invoked, not inside the function itself. */
#define expect_Sdb  (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_Sdb_imp
#define expect_LayerValue  (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_LayerValue_imp
#define expect_LayerValue2  (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_LayerValue_imp2


BOOL wrapAllowPermLayer(const char* str)
{
    WCHAR buf[100];
    MultiByteToWideChar(CP_ACP, 0, str, -1, buf, 100);
    return pAllowPermLayer(buf);
}

/* Brute forcing all ascii chars in the first 2 places seems to indicate that all it cares for is:
    - Second char has to be a ':'
        if it's not a ':', display a diagnostic message (and a different one for '\\').
    - First char does not really matter, as long as it's not on a DRIVE_REMOTE (but, according to the logging this is meant to check for a CDROM drive...)
*/
static void test_AllowPermLayer(void)
{
    char buf[20];
    char drive_letter;
    UINT drivetype = 0;
    ok(pAllowPermLayer(NULL) == FALSE, "Expected AllowPermLayer to fail for NULL\n");
    if (g_WinVersion < WINVER_WIN8)
    {
        ok(wrapAllowPermLayer("-:"), "Expected AllowPermLayer to succeed\n");
        ok(wrapAllowPermLayer("@:"), "Expected AllowPermLayer to succeed\n");
        ok(wrapAllowPermLayer("4:"), "Expected AllowPermLayer to succeed\n");
        ok(wrapAllowPermLayer("*:"), "Expected AllowPermLayer to succeed\n");
    }
    ok(wrapAllowPermLayer("*a") == FALSE, "Expected AllowPermLayer to fail\n");
    ok(wrapAllowPermLayer("*\\") == FALSE, "Expected AllowPermLayer to fail\n");
    for (drive_letter = 'a'; drive_letter <= 'z'; ++drive_letter)
    {
        sprintf(buf, "%c:\\", drive_letter);
        drivetype = GetDriveTypeA(buf);
        ok(wrapAllowPermLayer(buf) == (drivetype != DRIVE_REMOTE), "Expected AllowPermLayer to be %d for %c:\\\n", (drivetype != DRIVE_REMOTE), drive_letter);
    }
}

static BOOL wrapSdbSetPermLayerKeys(PCWSTR wszPath, PCSTR szLayers, BOOL bMachine)
{
    WCHAR wszLayers[MAX_LAYER_LENGTH];
    MultiByteToWideChar(CP_ACP, 0, szLayers, -1, wszLayers, MAX_LAYER_LENGTH);
    return pSdbSetPermLayerKeys(wszPath, wszLayers, bMachine);
}

static void test_SdbSetPermLayerKeysLevel(BOOL bMachine, const char* file)
{
    WCHAR fileW[MAX_PATH+20];
    WCHAR emptyString[1] = { 0 };

    MultiByteToWideChar(CP_ACP, 0, file, -1, fileW, MAX_PATH+20);

    /* Test some parameter validation. */
    ok(pSdbSetPermLayerKeys(NULL, NULL, bMachine) == FALSE, "Expected SdbSetPermLayerKeys to fail\n");
    ok(pSdbSetPermLayerKeys(NULL, emptyString, bMachine) == FALSE, "Expected SdbSetPermLayerKeys to fail\n");
    ok(pSdbSetPermLayerKeys(emptyString, emptyString, bMachine) == FALSE, "Expected SdbSetPermLayerKeys to fail\n");
    ok(pSdbSetPermLayerKeys(fileW, NULL, bMachine) == TRUE, "Expected SdbSetPermLayerKeys to succeed\n");
    ok(pSdbSetPermLayerKeys(fileW, emptyString, bMachine) == TRUE, "Expected SdbSetPermLayerKeys to fail\n");

    /* Basic tests */
    ok(wrapSdbSetPermLayerKeys(fileW, "TEST1", bMachine), "Expected SdbSetPermLayerKeys to succeed\n");
    expect_LayerValue(bMachine, file, "TEST1");

    ok(wrapSdbSetPermLayerKeys(fileW, "TEST1 TEST2", bMachine), "Expected SdbSetPermLayerKeys to succeed\n");
    expect_LayerValue(bMachine, file, "TEST1 TEST2");

    /* SdbSetPermLayerKeys does not do any validation of the value passed in. */
    ok(wrapSdbSetPermLayerKeys(fileW, "!#$% TEST1 TEST2", bMachine), "Expected SdbSetPermLayerKeys to succeed\n");
    expect_LayerValue(bMachine, file, "!#$% TEST1 TEST2");

    ok(wrapSdbSetPermLayerKeys(fileW, "!#$% TEST1     TEST2", bMachine), "Expected SdbSetPermLayerKeys to succeed\n");
    expect_LayerValue(bMachine, file, "!#$% TEST1     TEST2");

    ok(pSdbSetPermLayerKeys(fileW, NULL, bMachine) == TRUE, "Expected SdbSetPermLayerKeys to succeed\n");
    expect_LayerValue(bMachine, file, NULL);

    ok(wrapSdbSetPermLayerKeys(fileW, " ", bMachine), "Expected SdbSetPermLayerKeys to succeed\n");
    expect_LayerValue(bMachine, file, " ");

    ok(pSdbSetPermLayerKeys(fileW, NULL, bMachine) == TRUE, "Expected SdbSetPermLayerKeys to fail\n");
    expect_LayerValue(bMachine, file, NULL);
}

static void test_SdbGetPermLayerKeys(void)
{
    WCHAR pathW[MAX_PATH], buffer[MAX_LAYER_LENGTH] = { 0 };
    char file[MAX_PATH + 20], tmp[MAX_PATH + 20];
    BOOL bUser, bMachine;
    HANDLE hfile;
    DWORD dwBufSize = sizeof(buffer);

    GetTempPathA(MAX_PATH, tmp);
    GetLongPathNameA(tmp, file, sizeof(file));
    PathCombineA(tmp, file, "notexist.exe");
    PathAppendA(file, "test_file.exe");

    /* Check that we can access the keys */
    bUser = setLayerValue(FALSE, file, "RUNASADMIN WINXPSP3");
    expect_LayerValue(FALSE, file, "RUNASADMIN WINXPSP3");
    ok(bUser, "Expected to be able to set atleast the flags for the user\n");
    if (!bUser)
    {
        skip("Cannot do any tests if I cannot set some values\n");
        return;
    }
    bMachine = setLayerValue(TRUE, file, "WINXPSP3 WINXPSP2");
    if (bMachine)
    {
        expect_LayerValue(TRUE, file, "WINXPSP3 WINXPSP2");
    }


    hfile = CreateFileA(file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile failed on '%s'..\n", file);
    if (hfile == INVALID_HANDLE_VALUE)
    {
        skip("Running these tests is useless without a file present\n");
        return;
    }
    CloseHandle(hfile);

    MultiByteToWideChar(CP_ACP, 0, file, -1, pathW, MAX_PATH);

    /* Parameter validation */
    ok(pSdbGetPermLayerKeys(NULL, NULL, NULL, 0) == FALSE, "Expected pSdbGetPermLayerKeys to fail\n");
    ok(pSdbGetPermLayerKeys(pathW, NULL, NULL, 0) == FALSE, "Expected pSdbGetPermLayerKeys to fail\n");
    ok(pSdbGetPermLayerKeys(pathW, buffer, NULL, 0) == FALSE, "Expected pSdbGetPermLayerKeys to fail\n");
    ok(pSdbGetPermLayerKeys(pathW, buffer, &dwBufSize, 0) == FALSE, "Expected pSdbGetPermLayerKeys to fail\n");
    ok(dwBufSize == 0, "Expected dwBufSize to be %u, was %u\n", 0, dwBufSize);

    /* It fails on a nonexisting file */
    expect_Sdb(tmp, GPLK_USER | GPLK_MACHINE, FALSE, 0xffffffff, "");
    expect_Sdb(file, GPLK_USER, TRUE, 40, "RUNASADMIN WINXPSP3");
    GetShortPathNameA(file, tmp, sizeof(tmp));
    expect_Sdb(tmp, GPLK_USER, TRUE, 40, "RUNASADMIN WINXPSP3");

    if (bMachine)
    {
        /* Query from HKLM */
        expect_Sdb(file, GPLK_MACHINE, TRUE, 36, "WINXPSP3 WINXPSP2");
        /* Query from both, showing that duplicates are not removed */
        expect_Sdb(file, GPLK_USER | GPLK_MACHINE, TRUE, 76, "WINXPSP3 WINXPSP2 RUNASADMIN WINXPSP3");

        /* Showing that no validation is done on the value read. */
        ok(setLayerValue(TRUE, file, "!#!# WINXPSP3 WINXPSP3  !#  WINXPSP2    "), "Expected setLayerValue not to fail\n");
        expect_Sdb(file, GPLK_MACHINE, TRUE, 82, "!#!# WINXPSP3 WINXPSP3  !#  WINXPSP2    ");
        /* Showing that a space is inserted, even if the last char was already a space. */
        expect_Sdb(file, GPLK_USER | GPLK_MACHINE, TRUE, 122, "!#!# WINXPSP3 WINXPSP3  !#  WINXPSP2     RUNASADMIN WINXPSP3");
        /* Now clear the user key */
        setLayerValue(FALSE, file, NULL);
        /* Request both, to show that the last space (from the key) is not cut off. */
        expect_Sdb(file, GPLK_USER | GPLK_MACHINE, TRUE, 82, "!#!# WINXPSP3 WINXPSP3  !#  WINXPSP2    ");
        setLayerValue(FALSE, file, "RUNASADMIN WINXPSP3");
    }
    else
    {
        skip("Skipping tests for HKLM, cannot alter the registry\n");
    }
    /* Fail from these paths */
    StringCbPrintfA(tmp, sizeof(tmp), "\\?\\%s", file);
    expect_Sdb(tmp, GPLK_USER, FALSE, 0xffffffff, "");
    StringCbPrintfA(tmp, sizeof(tmp), "\\??\\%s", file);
    expect_Sdb(tmp, GPLK_USER, FALSE, 0xffffffff, "");

    ok(setLayerValue(FALSE, file, "!#!# RUNASADMIN RUNASADMIN  !#  WINXPSP3    "), "Expected setLayerValue not to fail\n");
    /* There is no validation on information read back. */
    expect_Sdb(file, GPLK_USER, TRUE, 90, "!#!# RUNASADMIN RUNASADMIN  !#  WINXPSP3    ");


    /* Cleanup */
    ok(DeleteFileA(file), "DeleteFile failed....\n");
    setLayerValue(FALSE, file, NULL);
    setLayerValue(TRUE, file, NULL);
}


static BOOL wrapSetPermLayerState(PCWSTR wszPath, PCSTR szLayer, DWORD dwFlags, BOOL bMachine, BOOL bEnable)
{
    WCHAR wszLayer[MAX_LAYER_LENGTH];
    MultiByteToWideChar(CP_ACP, 0, szLayer, -1, wszLayer, MAX_LAYER_LENGTH);
    return pSetPermLayerState(wszPath, wszLayer, dwFlags, bMachine, bEnable);
}

static void test_SetPermLayerStateLevel(BOOL bMachine, const char* file)
{
    WCHAR fileW[MAX_PATH+20];
    WCHAR emptyString[1] = { 0 };
    DWORD dwFlag;

    MultiByteToWideChar(CP_ACP, 0, file, -1, fileW, MAX_PATH+20);

    /* Test some parameter validation. */
    ok(pSetPermLayerState(fileW, NULL, 0, bMachine, 0) == FALSE, "Expected SetPermLayerState to fail\n");
    expect_LayerValue(bMachine, file, NULL);

    ok(pSetPermLayerState(fileW, NULL, 0, bMachine, 1) == FALSE, "Expected SetPermLayerState to fail\n");
    expect_LayerValue(bMachine, file, NULL);

    ok(wrapSetPermLayerState(fileW, "", 0, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, NULL);

    ok(wrapSetPermLayerState(fileW, "", 0, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, NULL);

    ok(wrapSetPermLayerState(NULL, NULL, 0, bMachine, 0) == FALSE, "Expected SetPermLayerState to fail\n");
    expect_LayerValue(bMachine, NULL, NULL);

    ok(wrapSetPermLayerState(NULL, NULL, 0, bMachine, 1) == FALSE, "Expected SetPermLayerState to fail\n");
    expect_LayerValue(bMachine, NULL, NULL);

    ok(wrapSetPermLayerState(emptyString, "", 0, bMachine, 0) == FALSE, "Expected SetPermLayerState to fail\n");
    expect_LayerValue(bMachine, NULL, NULL);

    ok(wrapSetPermLayerState(emptyString, "", 0, bMachine, 1) == FALSE, "Expected SetPermLayerState to fail\n");
    expect_LayerValue(bMachine, NULL, NULL);

    ok(wrapSetPermLayerState(emptyString, "TEST", 0, bMachine, 0) == FALSE, "Expected SetPermLayerState to fail\n");
    expect_LayerValue(bMachine, NULL, NULL);

    if (g_WinVersion <= WINVER_WIN8)
    {
        ok(wrapSetPermLayerState(emptyString, "TEST", 0, bMachine, 1) == FALSE, "Expected SetPermLayerState to fail\n");
        expect_LayerValue(bMachine, NULL, NULL);
    }


    /* Now, on to the actual tests. */
    expect_LayerValue(bMachine, file, NULL);
    ok(wrapSetPermLayerState(fileW, "TEST", 0, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, NULL);

    ok(wrapSetPermLayerState(fileW, "TEST", 0, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, "TEST");

    ok(wrapSetPermLayerState(fileW, "", 0, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, "TEST");

    ok(wrapSetPermLayerState(fileW, "test", 0, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, "test");

    ok(wrapSetPermLayerState(fileW, "TEST", 0, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, NULL);

    ok(wrapSetPermLayerState(fileW, "TEST", 0, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, "TEST");

    ok(wrapSetPermLayerState(fileW, "TEST1", 0, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
        expect_LayerValue2(bMachine, file, "TEST TEST1", g_WinVersion >= WINVER_WIN8, "TEST1 TEST");

    ok(wrapSetPermLayerState(fileW, "TEST2", 0, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue2(bMachine, file, "TEST TEST1 TEST2", g_WinVersion >= WINVER_WIN8, "TEST2 TEST1 TEST");

    ok(wrapSetPermLayerState(fileW, "TEST1", 0, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue2(bMachine, file, "TEST TEST2", g_WinVersion >= WINVER_WIN8, "TEST2 TEST");

    ok(wrapSetPermLayerState(fileW, "TEST", 0, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, "TEST2");

    ok(wrapSetPermLayerState(fileW, "TEST2", 0, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, NULL);

    /* Valid flags until win8: !# */
    /* Key is empty, now play around with the flags. */
    for (dwFlag = ((g_WinVersion >= WINVER_WIN8) ? 6 : 2); dwFlag < 32; ++dwFlag)
    {
        ok(wrapSetPermLayerState(fileW, "TEST", (1<<dwFlag), bMachine, 1) == FALSE, "Expected SetPermLayerState to fail on 0x%x\n", (1<<dwFlag));
    }
    expect_LayerValue(bMachine, file, NULL);

    /* Add layer flags */
    ok(wrapSetPermLayerState(fileW, "TEST", LAYER_APPLY_TO_SYSTEM_EXES, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, "# TEST");

    ok(wrapSetPermLayerState(fileW, "TEST2", 2, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue2(bMachine, file, "!# TEST TEST2", g_WinVersion >= WINVER_WIN8, "!# TEST2 TEST");

    ok(wrapSetPermLayerState(fileW, "TEST", 0, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue2(bMachine, file, "!# TEST2 TEST", g_WinVersion >= WINVER_WIN8, "!# TEST TEST2");

    ok(wrapSetPermLayerState(fileW, "TEST3", 0, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue2(bMachine, file, "!# TEST2 TEST TEST3", g_WinVersion >= WINVER_WIN8, "!# TEST3 TEST TEST2");

    /* Remove on a flag removes that flag from the start. */
    ok(wrapSetPermLayerState(fileW, "TEST2", 2, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue2(bMachine, file, "# TEST TEST3", g_WinVersion >= WINVER_WIN8, "# TEST3 TEST");

    ok(wrapSetPermLayerState(fileW, "", LAYER_APPLY_TO_SYSTEM_EXES, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue2(bMachine, file, "TEST TEST3", g_WinVersion >= WINVER_WIN8, "TEST3 TEST");

    ok(wrapSetPermLayerState(fileW, "", LAYER_APPLY_TO_SYSTEM_EXES | 2, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue2(bMachine, file, "!# TEST TEST3", g_WinVersion >= WINVER_WIN8, "!# TEST3 TEST");

    ok(wrapSetPermLayerState(fileW, "TEST3", LAYER_APPLY_TO_SYSTEM_EXES, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, "! TEST");

    ok(wrapSetPermLayerState(fileW, "TEST", 2, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, NULL);

    /* Try adding multiple layers: */
    ok(wrapSetPermLayerState(fileW, "TEST TEST2", LAYER_APPLY_TO_SYSTEM_EXES | 2, bMachine, 1) == FALSE, "Expected SetPermLayerState to fail\n");
    expect_LayerValue(bMachine, file, NULL);

    ok(wrapSetPermLayerState(fileW, "TEST2", 0, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, "TEST2");

    /* Try adding flags in via layer string */
    ok(wrapSetPermLayerState(fileW, "#", 0, bMachine, 1) == FALSE, "Expected SetPermLayerState to fail\n");
    expect_LayerValue(bMachine, file, "TEST2");

    ok(wrapSetPermLayerState(fileW, "!", 0, bMachine, 1) == FALSE, "Expected SetPermLayerState to fail\n");
    expect_LayerValue(bMachine, file, "TEST2");

    /* Now we prepare the registry with some crap to see how data is validated. */
    setLayerValue(bMachine, file, "!#!# TEST2 TEST2  !#  TEST    ");

    ok(wrapSetPermLayerState(fileW, "TEST1", 0, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue2(bMachine, file, "!# TEST2 TEST2 !# TEST TEST1", g_WinVersion >= WINVER_WIN8, "!# TEST1 TEST2 TEST2 !# TEST");

    /* Removing a duplicate entry will remove all instances of it */
    ok(wrapSetPermLayerState(fileW, "TEST2", 0, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue2(bMachine, file, "!# !# TEST TEST1", g_WinVersion >= WINVER_WIN8, "!# TEST1 !# TEST");

    /* Adding a flag cleans other flags (from the start) */
    ok(wrapSetPermLayerState(fileW, "", LAYER_APPLY_TO_SYSTEM_EXES, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue2(bMachine, file, "!# TEST TEST1", g_WinVersion >= WINVER_WIN8, "!# TEST1 !# TEST");

    if(g_WinVersion < WINVER_WIN8)
    {
        ok(wrapSetPermLayerState(fileW, "$%$%^^", 0, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
        expect_LayerValue(bMachine, file, "!# TEST TEST1 $%$%^^");
    }

    setLayerValue(bMachine, file, "!#!# TEST2  !#  TEST    ");
    ok(wrapSetPermLayerState(fileW, "", LAYER_APPLY_TO_SYSTEM_EXES, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, "! TEST2 !# TEST");

    /* Tabs are treated as spaces */
    setLayerValue(bMachine, file, "!#!# TEST2 \t  TEST2 !#  \t TEST    ");
    ok(wrapSetPermLayerState(fileW, "TEST2", 0, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue2(bMachine, file, "!# !# TEST TEST2", g_WinVersion >= WINVER_WIN8, "!# TEST2 !# TEST");

    /* Newlines are left as-is */
    setLayerValue(bMachine, file, "!#!# TEST2 \n  TEST2 !#  \r\n TEST    ");
    ok(wrapSetPermLayerState(fileW, "TEST2", 0, bMachine, 1) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue2(bMachine, file, "!# \n !# \r\n TEST TEST2", g_WinVersion >= WINVER_WIN8, "!# TEST2 \n !# \r\n TEST");

    /* Whitespace and duplicate flags are eaten from the start */
    setLayerValue(bMachine, file, "     !#!# TEST2 \t  TEST2 !#  \t TEST    ");
    ok(wrapSetPermLayerState(fileW, "", LAYER_APPLY_TO_SYSTEM_EXES, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, "! TEST2 TEST2 !# TEST");

    setLayerValue(bMachine, file, "!# !# TEST2  !#  TEST    ");
    ok(wrapSetPermLayerState(fileW, "", LAYER_APPLY_TO_SYSTEM_EXES, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, "! TEST2 !# TEST");

    ok(wrapSetPermLayerState(fileW, "", LAYER_APPLY_TO_SYSTEM_EXES, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, "! TEST2 !# TEST");

    ok(wrapSetPermLayerState(fileW, "", 2, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, "TEST2 !# TEST");

    /* First flags are cleaned, then a layer is removed. */
    ok(wrapSetPermLayerState(fileW, "TEST2", 2, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, "!# TEST");

    /* Nothing is changed, still it succeeds. */
    ok(wrapSetPermLayerState(fileW, "TEST2", 2, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, "# TEST");

    /* And remove the last bits. */
    ok(wrapSetPermLayerState(fileW, "TEST", LAYER_APPLY_TO_SYSTEM_EXES, bMachine, 0) == TRUE, "Expected SetPermLayerState to succeed\n");
    expect_LayerValue(bMachine, file, NULL);
}

static void test_SetPermLayer(void)
{
    char file[MAX_PATH + 20], tmp[MAX_PATH + 20];
    HANDLE hfile;

    GetTempPathA(MAX_PATH, tmp);
    GetLongPathNameA(tmp, file, sizeof(file));
    PathAppendA(file, "test_file.exe");

    hfile = CreateFileA(file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile failed for '%s'\n", file);
    if (hfile == INVALID_HANDLE_VALUE)
    {
        skip("Running these tests is useless without a file present\n");
        return;
    }
    CloseHandle(hfile);

    if (setLayerValue(FALSE, file, NULL))
    {
        test_SdbSetPermLayerKeysLevel(FALSE, file);
        test_SetPermLayerStateLevel(FALSE, file);
    }
    else
    {
        skip("Skipping SetPermLayerStateLevel tests for User, because I cannot prepare the environment\n");
    }
    if (setLayerValue(TRUE, file, NULL))
    {
        test_SdbSetPermLayerKeysLevel(TRUE, file);
        test_SetPermLayerStateLevel(TRUE, file);
    }
    else
    {
        skip("Skipping SetPermLayerStateLevel tests for Machine (HKLM), because I cannot prepare the environment\n");
    }
    ok(DeleteFileA(file), "DeleteFile failed....\n");
}

static BOOL create_file(LPCSTR dir, LPCSTR name, int filler, DWORD size)
{
    char target[MAX_PATH], *tmp;
    HANDLE file;
    PathCombineA(target, dir, name);

    tmp = malloc(size);
    memset(tmp, filler, size);

    file = CreateFileA(target, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(file == INVALID_HANDLE_VALUE)
        return FALSE;

    WriteFile(file, tmp, size, &size, NULL);
    CloseHandle(file);
    free(tmp);
    return TRUE;
}

static BOOL delete_file(LPCSTR dir, LPCSTR name)
{
    char target[MAX_PATH];
    PathCombineA(target, dir, name);
    return DeleteFileA(target);
}

static char g_FakeDrive = 0;

UINT (WINAPI *pGetDriveTypeW)(LPCWSTR target) = NULL;
UINT WINAPI mGetDriveTypeW(LPCWSTR target)
{
    UINT uRet = pGetDriveTypeW(target);
    if(g_FakeDrive && target && (char)*target == g_FakeDrive)
        return DRIVE_CDROM;
    return uRet;
}

static BOOL wrapSdbSetPermLayerKeys2(LPCSTR dir, LPCSTR name, PCSTR szLayers, BOOL bMachine)
{
    char szPath[MAX_PATH];
    WCHAR wszPath[MAX_PATH], wszLayers[MAX_LAYER_LENGTH];
    PathCombineA(szPath, dir, name);
    MultiByteToWideChar(CP_ACP, 0, szLayers, -1, wszLayers, MAX_LAYER_LENGTH);
    MultiByteToWideChar(CP_ACP, 0, szPath, -1, wszPath, MAX_PATH);
    return pSdbSetPermLayerKeys(wszPath, wszLayers, bMachine);
}


BOOL expect_files(const char* dir, int num, ...)
{
    char finddir[MAX_PATH + 20];
    va_list args;
    WIN32_FIND_DATAA find = { 0 };
    HANDLE hFind;
    int cmp = 0;

    va_start(args, num);

    PathCombineA(finddir, dir, "*");
    hFind = FindFirstFileA(finddir, &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        const char* file;
        do
        {
            if (!(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                if (--num < 0)
                    break;
                file = va_arg(args, const char*);
                cmp = strcmp(file, find.cFileName);
            }
        } while (cmp == 0 && FindNextFileA(hFind, &find));
        FindClose(hFind);
    }
    va_end(args);
    return cmp == 0 && num == 0;
}


static void test_Sign_Media(void)
{
    char workdir[MAX_PATH], subdir[MAX_PATH], drive[5] = "Z:";
    BOOL ret;

    DWORD logical_drives = GetLogicalDrives();
    g_FakeDrive = 0;
    for (drive[0] = 'D'; drive[0] <= 'Z'; drive[0]++)
    {
        DWORD idx = 1 << (drive[0] - 'D' + 3);
        if (!(logical_drives & idx))
        {
            g_FakeDrive = drive[0];
            break;
        }
    }
    if (!g_FakeDrive)
    {
        skip("Unable to find a free drive\n");
        return;
    }

    ret = GetTempPathA(MAX_PATH, workdir);
    ok(ret, "GetTempPathA error: %d\n", GetLastError());
    PathAppendA(workdir, "apphelp_test");

    ret = CreateDirectoryA(workdir, NULL);
    ok(ret, "CreateDirectoryA error: %d\n", GetLastError());

    PathCombineA(subdir, workdir, "sub");
    ret = CreateDirectoryA(subdir, NULL);
    ok(ret, "CreateDirectoryA error: %d\n", GetLastError());

    ret = DefineDosDeviceA(DDD_NO_BROADCAST_SYSTEM, drive, workdir);
    ok(ret, "DefineDosDeviceA error: %d\n", GetLastError());
    if(ret)
    {
        ret = RedirectIat(GetModuleHandleA("apphelp.dll"), "kernel32.dll", "GetDriveTypeW",
                          (ULONG_PTR)mGetDriveTypeW, (ULONG_PTR*)&pGetDriveTypeW);
        if (g_WinVersion < WINVER_WIN8)
            ok(ret, "Expected redirect_iat to succeed\n");
        if(ret)
        {
            ok(create_file(workdir, "test.exe", 'a', 4), "create_file error: %d\n", GetLastError());

            ok(wrapSdbSetPermLayerKeys2(drive, "test.exe", "TEST", 0), "Expected wrapSdbSetPermLayerKeys2 to succeed\n");
            /* 4 */
            /* test.exe */
            expect_LayerValue(0, "SIGN.MEDIA=4 test.exe", "TEST");
            ok(wrapSdbSetPermLayerKeys2(drive, "test.exe", "", 0), "Expected wrapSdbSetPermLayerKeys2 to succeed\n");

            ok(create_file(workdir, "test.txt", 'a', 1), "create_file error: %d\n", GetLastError());

            if (!expect_files(workdir, 2, "test.exe", "test.txt"))
            {
                skip("Skipping test, files are not returned in the expected order by the FS\n");
            }
            else
            {
                ok(wrapSdbSetPermLayerKeys2(drive, "test.exe", "TEST", 0), "Expected wrapSdbSetPermLayerKeys2 to succeed\n");
                /* (4 << 1) ^ 1 */
                /* test.exe   test.txt */
                expect_LayerValue(0, "SIGN.MEDIA=9 test.exe", "TEST");
                ok(wrapSdbSetPermLayerKeys2(drive, "test.exe", "", 0), "Expected wrapSdbSetPermLayerKeys2 to succeed\n");
            }

            ok(create_file(workdir, "test.zz", 'a', 0x1000), "create_file error: %d\n", GetLastError());

            if (!expect_files(workdir, 3, "test.exe", "test.txt", "test.zz"))
            {
                skip("Skipping test, files are not returned in the expected order by the FS\n");
            }
            else
            {
                ok(wrapSdbSetPermLayerKeys2(drive, "test.exe", "TEST", 0), "Expected wrapSdbSetPermLayerKeys2 to succeed\n");
                /* (((4 << 1) ^ 1) << 1) ^ 0x1000 */
                /* test.exe   test.txt     test.zz */
                expect_LayerValue(0, "SIGN.MEDIA=1012 test.exe", "TEST");
                ok(wrapSdbSetPermLayerKeys2(drive, "test.exe", "", 0), "Expected wrapSdbSetPermLayerKeys2 to succeed\n");
            }

            ok(create_file(subdir, "test.exe", 'a', 0x10203), "create_file error: %d\n", GetLastError());

            if (!expect_files(subdir, 1, "test.exe"))
            {
                skip("Skipping test, files are not returned in the expected order by the FS\n");
            }
            else
            {
                ok(wrapSdbSetPermLayerKeys2(drive, "sub\\test.exe", "TEST", 0), "Expected wrapSdbSetPermLayerKeys2 to succeed\n");
                /* 0x10203 */
                /* test.exe */
                expect_LayerValue(0, "SIGN.MEDIA=10203 sub\\test.exe", "TEST");
                ok(wrapSdbSetPermLayerKeys2(drive, "sub\\test.exe", "", 0), "Expected wrapSdbSetPermLayerKeys2 to succeed\n");
            }

            ok(create_file(subdir, "test.bbb", 'a', 0), "create_file error: %d\n", GetLastError());

            if (!expect_files(subdir, 2, "test.bbb", "test.exe"))
            {
                skip("Skipping test, files are not returned in the expected order by the FS\n");
            }
            else
            {
                ok(wrapSdbSetPermLayerKeys2(drive, "sub\\test.exe", "TEST", 0), "Expected wrapSdbSetPermLayerKeys2 to succeed\n");
                /* 0x10203 */
                /* test.exe */
                expect_LayerValue(0, "SIGN.MEDIA=10203 sub\\test.exe", "TEST");
                ok(wrapSdbSetPermLayerKeys2(drive, "sub\\test.exe", "", 0), "Expected wrapSdbSetPermLayerKeys2 to succeed\n");
            }

            ok(create_file(subdir, "TEST.txt", 'a', 0x30201), "create_file error: %d\n", GetLastError());

            if (!expect_files(subdir, 3, "test.bbb", "test.exe", "TEST.txt"))
            {
                skip("Skipping test, files are not returned in the expected order by the FS\n");
            }
            else
            {
                ok(wrapSdbSetPermLayerKeys2(drive, "sub\\test.exe", "TEST", 0), "Expected wrapSdbSetPermLayerKeys2 to succeed\n");
                /* (0x10203 << 1) ^ 0x30201 */
                /*  test.exe        TEST.txt */
                expect_LayerValue(0, "SIGN.MEDIA=10607 sub\\test.exe", "TEST");
                ok(wrapSdbSetPermLayerKeys2(drive, "sub\\test.exe", "", 0), "Expected wrapSdbSetPermLayerKeys2 to succeed\n");
            }

            ok(create_file(subdir, "TEST.aaa", 'a', 0x3a2a1), "create_file error: %d\n", GetLastError());

            if (!expect_files(subdir, 4, "TEST.aaa", "test.bbb", "test.exe", "TEST.txt"))
            {
                skip("Skipping test, files are not returned in the expected order by the FS\n");
            }
            else
            {
                ok(wrapSdbSetPermLayerKeys2(drive, "sub\\test.exe", "TEST", 0), "Expected wrapSdbSetPermLayerKeys2 to succeed\n");
                /* (((0x3a2a1 << 1) ^ 0x10203) << 1) ^ 0x30201 */
                /*    TEST.aaa        test.exe         TEST.txt */
                expect_LayerValue(0, "SIGN.MEDIA=F8C83 sub\\test.exe", "TEST");
                ok(wrapSdbSetPermLayerKeys2(drive, "sub\\test.exe", "", 0), "Expected wrapSdbSetPermLayerKeys2 to succeed\n");
            }

            ret = RestoreIat(GetModuleHandleA("apphelp.dll"), "kernel32.dll", "GetDriveTypeW", (ULONG_PTR)pGetDriveTypeW);
            ok(ret, "Expected restore_iat to succeed\n");

            ok(delete_file(subdir, "test.bbb"), "delete_file error: %d\n", GetLastError());
            ok(delete_file(subdir, "TEST.aaa"), "delete_file error: %d\n", GetLastError());
            ok(delete_file(subdir, "TEST.txt"), "delete_file error: %d\n", GetLastError());
            ok(delete_file(subdir, "test.exe"), "delete_file error: %d\n", GetLastError());
            ok(delete_file(workdir, "test.zz"), "delete_file error: %d\n", GetLastError());
            ok(delete_file(workdir, "test.txt"), "delete_file error: %d\n", GetLastError());
            ok(delete_file(workdir, "test.exe"), "delete_file error: %d\n", GetLastError());
        }
        ret = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_NO_BROADCAST_SYSTEM, drive, NULL);
        ok(ret, "DefineDosDeviceA error: %d\n", GetLastError());
    }
    ret = RemoveDirectoryA(subdir);
    ok(ret, "RemoveDirectoryA error: %d\n", GetLastError());
    ret = RemoveDirectoryA(workdir);
    ok(ret, "RemoveDirectoryA error: %d\n", GetLastError());
}


START_TEST(layerapi)
{
    silence_debug_output();
    /*SetEnvironmentVariable("SHIM_DEBUG_LEVEL", "4");*/
    hdll = LoadLibraryA("apphelp.dll");
    pAllowPermLayer = (void *)GetProcAddress(hdll, "AllowPermLayer");
    pSdbSetPermLayerKeys = (void *)GetProcAddress(hdll, "SdbSetPermLayerKeys");
    pSdbGetPermLayerKeys = (void *)GetProcAddress(hdll, "SdbGetPermLayerKeys");
    pSetPermLayerState = (void *)GetProcAddress(hdll, "SetPermLayerState");
    g_WinVersion = get_host_winver();

    if (!pAllowPermLayer)
    {
        skip("Skipping tests with AllowPermLayer, function not found\n");
    }
    else
    {
        test_AllowPermLayer();
    }

    if (!pSdbSetPermLayerKeys)
    {
        skip("Skipping tests with SdbSetPermLayerKeys, function not found\n");
    }
    else
    {
        if (!pSdbGetPermLayerKeys)
        {
            skip("Skipping tests with SdbGetPermLayerKeys, function not found\n");
        }
        else
        {
            test_SdbGetPermLayerKeys();
        }

        if (!pSetPermLayerState)
        {
            skip("Skipping tests with SetPermLayerState, function not found\n");
        }
        else
        {
            test_SetPermLayer();
            test_Sign_Media();
        }
    }
}
