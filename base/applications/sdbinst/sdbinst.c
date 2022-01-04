/*
 * PROJECT:     ReactOS sdbinst
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Application compatibility database installer
 * COPYRIGHT:   Copyright 2020 Max Korostil (mrmks04@yandex.ru)
 */

#include <tchar.h>

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <strsafe.h>
#include <objbase.h>
#include <apphelp.h>
#include <shlwapi.h>

#define APPCOMPAT_CUSTOM_REG_PATH L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Custom"
#define APPCOMPAT_LAYERS_REG_PATH L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Custom\\Layers"
#define APPCOMPAT_INSTALLEDSDB_REG_PATH L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\InstalledSDB"
#define UNINSTALL_REG_PATH L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
#define DBPATH_KEY_NAME L"DatabasePath"
#define SDB_EXT L".sdb"
#define GUID_STR_LENGTH 38
#define GUID_SBD_NAME_LENGTH (GUID_STR_LENGTH + ARRAYSIZE(SDB_EXT))

BOOL SdbUninstallByGuid(_In_ LPWSTR guidSdbStr);

HRESULT
RegisterSdbEntry(
    _In_ PWCHAR sdbEntryName,
    _In_ LPCWSTR dbGuid,
    _In_ ULONGLONG time,
    _In_ BOOL isInstall,
    _In_ BOOL isExe)
{
    WCHAR regName[MAX_PATH];
    HKEY hKey = NULL;
    LSTATUS status;
    HRESULT hres;

    hres = StringCchPrintfW(regName, MAX_PATH, L"%ls\\%ls",
                            isExe ? APPCOMPAT_CUSTOM_REG_PATH : APPCOMPAT_LAYERS_REG_PATH,
                            sdbEntryName);
    if (FAILED(hres))
    {
        wprintf(L"StringCchPrintfW error: 0x%08X\n", hres);
        goto end;
    }

    // Remove key
    if (!isInstall)
    {
        status = RegDeleteKeyW(HKEY_LOCAL_MACHINE, regName);
        if (status == ERROR_FILE_NOT_FOUND)
        {
            status = ERROR_SUCCESS;
        }

        return HRESULT_FROM_WIN32(status);
    }

    // Create main key
    status = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                            regName,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            NULL);
    if (status != ERROR_SUCCESS)
    {
        wprintf(L"RegKeyCreateEx error: 0x%08X", status);
        hres = HRESULT_FROM_WIN32(status);
        goto end;
    }

    // Set installed time
    status = RegSetValueExW(hKey,
                           dbGuid,
                           0,
                           REG_QWORD,
                           (PBYTE)&time,
                           sizeof(time));
    if (status != ERROR_SUCCESS)
    {
        wprintf(L"RegSetValueExW error: 0x%08X", status);
        hres = HRESULT_FROM_WIN32(status);
        goto end;
    }

end:
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return hres;
}

HRESULT
AddUninstallKey(
    _In_ LPCWSTR dbName,
    _In_ LPCWSTR sdbInstalledPath,
    _In_ LPCWSTR guidDbStr)
{
    WCHAR sdbinstPath[MAX_PATH];
    WCHAR regName[MAX_PATH];
    WCHAR uninstString[MAX_PATH];
    HKEY hKey = NULL;
    HRESULT hres;

    UINT count = GetSystemWindowsDirectory(sdbinstPath, MAX_PATH);
    if (sdbinstPath[count - 1] != L'\\')
    {
        hres = StringCchCatW(sdbinstPath, MAX_PATH, L"\\");
        if (FAILED(hres))
        {
            wprintf(L"StringCchCatW error: 0x%08X", hres);
            goto end;
        }
    }

    // Full path to sdbinst.exe
    hres = StringCchCatW(sdbinstPath, MAX_PATH, L"System32\\sdbinst.exe");
    if (FAILED(hres))
    {
        wprintf(L"StringCchCatW error: 0x%08X", hres);
        goto end;
    }

    // Sdb GUID registry key
    hres = StringCchPrintfW(regName, MAX_PATH, L"%ls\\%ls", UNINSTALL_REG_PATH, guidDbStr);
    if (FAILED(hres))
    {
        wprintf(L"StringCchPrintfW error: 0x%08X", hres);
        goto end;
    }

    // Create main key
    LSTATUS status = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                    regName,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hKey,
                                    NULL);

    if (status != ERROR_SUCCESS)
    {
        wprintf(L"RegKeyCreateEx error: 0x%08X", status);
        hres = HRESULT_FROM_WIN32(status);
        goto end;
    }

    // Set Display name
    DWORD length = wcslen(dbName) * sizeof(WCHAR);
    status = RegSetValueExW(hKey,
                           L"DisplayName",
                           0,
                           REG_SZ,
                           (PBYTE)dbName,
                           length + sizeof(WCHAR));
    if (status != ERROR_SUCCESS)
    {
        wprintf(L"RegSetValueExW error: 0x%08X", status);
        hres = HRESULT_FROM_WIN32(status);
        goto end;
    }

    // Uninstall full string
    hres = StringCchPrintfW(uninstString, MAX_PATH, L"%ls -u \"%ls\"", sdbinstPath, sdbInstalledPath);
    if (FAILED(hres))
    {
        wprintf(L"StringCchPrintfW error: 0x%08X", hres);
        goto end;
    }

    // Set uninstall string
    length = wcslen(uninstString) * sizeof(WCHAR);
    status = RegSetValueExW(hKey,
                           L"UninstallString",
                           0,
                           REG_SZ,
                           (PBYTE)uninstString,
                           length + sizeof(WCHAR));
    if (status != ERROR_SUCCESS)
    {
        wprintf(L"RegSetValueExW error: 0x%08X", status);
        hres = HRESULT_FROM_WIN32(status);
        goto end;
    }

end:
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return hres;
}

//
// Get database GUID id
//
BOOL
GetSdbGuid(
    _In_ PDB pdb,
    _In_ TAGID tagDb,
    _Out_ GUID* guid)
{
    TAGID tagDbId;

    tagDbId = SdbFindFirstTag(pdb, tagDb, TAG_DATABASE_ID);
    if (!tagDbId)
    {
        wprintf(L"Can't find database id tag");
        return FALSE;
    }

    if (!SdbReadBinaryTag(pdb, tagDbId, (PBYTE)guid, sizeof(GUID)))
    {
        wprintf(L"Can't read database id");
        return FALSE;
    }

    return TRUE;
}

HRESULT
ProcessLayers(
    _In_ PDB pdb,
    _In_ TAGID tagDb,
    _In_opt_ LPCWSTR guidDbStr,
    _In_opt_ ULONGLONG time,
    _In_ BOOL isInstall)
{
    HRESULT res = ERROR_SUCCESS;
    TAGID tagLayerName;
    TAGID prevTagLayer = 0;

    TAGID tagLayer = SdbFindFirstTag(pdb, tagDb, TAG_LAYER);

    // Add all layers to registry (AppCompatFlags)
    while (tagLayer && (tagLayer != prevTagLayer))
    {
        tagLayerName = SdbFindFirstTag(pdb, tagLayer, TAG_NAME);
        if (!tagLayerName)
        {
            res = ERROR_NOT_FOUND;
            break;
        }

        LPWSTR name = SdbGetStringTagPtr(pdb, tagLayerName);

        res = RegisterSdbEntry(name, guidDbStr, time, isInstall, FALSE);
        if (FAILED(res))
        {
            wprintf(L"Can't register layer\n");
            break;
        }

        prevTagLayer = tagLayer;
        tagLayer = SdbFindNextTag(pdb, tagDb, tagLayer);
    }

    return res;
}

HRESULT
ProcessExe(
    _In_ PDB pdb,
    _In_ TAGID tagDb,
    _In_opt_ LPCWSTR guidDbStr,
    _In_opt_ ULONGLONG time,
    _In_ BOOL isInstall)
{
    HRESULT res = ERROR_SUCCESS;
    TAGID tagExeName;
    TAGID tagExePrev = 0;

    TAGID tagExe = SdbFindFirstTag(pdb, tagDb, TAG_EXE);

    // Add all exe to registry (AppCompatFlags)
    while (tagExe != 0 && (tagExe != tagExePrev))
    {
        tagExeName = SdbFindFirstTag(pdb, tagExe, TAG_NAME);
        if (!tagExeName)
        {
            wprintf(L"Can't find exe tag\n");
            res = ERROR_NOT_FOUND;
            break;
        }

        LPWSTR name = SdbGetStringTagPtr(pdb, tagExeName);

        res = RegisterSdbEntry(name, guidDbStr, time, isInstall, TRUE);
        if (FAILED(res))
        {
            wprintf(L"Can't register exe 0x%08X\n", res);
            break;
        }

        tagExePrev = tagExe;
        tagExe = SdbFindNextTag(pdb, tagDb, tagExe);
    }

    return res;
}

HRESULT
CopySdbToAppPatch(
    _In_ LPCWSTR sourceSdbPath,
    _In_ LPCWSTR destSdbPath)
{
    DWORD error = ERROR_SUCCESS;
    PWCHAR pTmpSysdir = NULL;
    SIZE_T destLen = wcslen(destSdbPath);
    PWCHAR sysdirPath = (PWCHAR)HeapAlloc(GetProcessHeap(), 0, destLen * sizeof(WCHAR));

    if (sysdirPath == NULL)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto end;
    }

    // Get parent folder fo sdb file
    CopyMemory(sysdirPath, destSdbPath, destLen * sizeof(WCHAR));
    pTmpSysdir = StrRChrW(sysdirPath, sysdirPath + destLen, L'\\');
    if (pTmpSysdir == NULL)
    {
        wprintf(L"Can't find directory separator\n");
        goto end;
    }
    else
    {
        *pTmpSysdir = UNICODE_NULL;
    }

    // Create directory if need
    if (!CreateDirectory(sysdirPath, NULL))
    {
        error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS)
        {
            wprintf(L"Can't create folder %ls\n Error: 0x%08X\n", sysdirPath, error);
            goto end;
        }
        error = ERROR_SUCCESS;
    }

    // Copy file
    if (!CopyFile(sourceSdbPath, destSdbPath, TRUE))
    {
        error = GetLastError();
        wprintf(L"Can't copy sdb file");
    }

end:
    if (sysdirPath)
    {
        HeapFree(GetProcessHeap(), 0, sysdirPath);
    }

    return HRESULT_FROM_WIN32(error);
}

HRESULT
BuildPathToSdb(
    _In_ PWCHAR buffer,
    _In_ SIZE_T bufLen,
    _In_ LPCWSTR guidDbStr)
{
    ZeroMemory(buffer, bufLen * sizeof(WCHAR));

    // Can't use here SdbGetAppPatchDir, because Windows XP haven't this function
    UINT count = GetSystemWindowsDirectory(buffer, bufLen);
    if (buffer[count - 1] != L'\\')
    {
        buffer[count] = L'\\';
    }

    HRESULT res = StringCchCatW(buffer, bufLen, L"AppPatch\\Custom\\");
    if (FAILED(res))
    {
        goto end;
    }

    res = StringCchCatW(buffer, bufLen, guidDbStr);

end:
    return res;
}

BOOL
SdbInstall(
    _In_ LPCWSTR sdbPath)
{
    BOOL res = FALSE;
    PDB pdb = NULL;
    TAGID tagDb;
    TAGID tagDbName;
    GUID dbGuid = {0};
    FILETIME systemTime = {0};
    ULARGE_INTEGER currentTime = {0};
    WCHAR sysdirPatchPath[MAX_PATH];
    WCHAR guidDbStr[GUID_SBD_NAME_LENGTH];

    GetSystemTimeAsFileTime(&systemTime);
    currentTime.LowPart  = systemTime.dwLowDateTime;
    currentTime.HighPart = systemTime.dwHighDateTime;

    // Open database
    pdb = SdbOpenDatabase(sdbPath, DOS_PATH);
    if (pdb == NULL)
    {
        wprintf(L"Can't open database %ls\n", sdbPath);
        goto end;
    }

    tagDb = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    if (!tagDb)
    {
        wprintf(L"Can't find database tag\n");
        goto end;
    }

    // Get database GUID
    if (!GetSdbGuid(pdb, tagDb, &dbGuid))
    {
        wprintf(L"GetSdbGuid error\n");
        goto end;
    }

    StringFromGUID2(&dbGuid, guidDbStr, GUID_SBD_NAME_LENGTH);
    HRESULT hres = StringCchCatW(guidDbStr, GUID_SBD_NAME_LENGTH, SDB_EXT);
    if (FAILED(hres))
    {
        wprintf(L"StringCchCatW error 0x%08X\n", hres);
        goto end;
    }

    wprintf(L"Database guid %ls\n", guidDbStr);

    tagDbName = SdbFindFirstTag(pdb, tagDb, TAG_NAME);
    if (!tagDbName)
    {
        wprintf(L"Can't get tag name\n");
        goto end;
    }

    LPWSTR dbName = SdbGetStringTagPtr(pdb, tagDbName);
    wprintf(L"Database name %ls\n", dbName);

    // Process exe tags
    hres = ProcessExe(pdb, tagDb, guidDbStr, currentTime.QuadPart, TRUE);
    if (FAILED(hres))
    {
        wprintf(L"Process exe failed. Status: 0x%08X", res);
        goto end;
    }

    // Proess layer tags
    hres = ProcessLayers(pdb, tagDb, guidDbStr, currentTime.QuadPart, TRUE);
    if (FAILED(hres))
    {
        wprintf(L"Process layers failed. Status: 0x%08X", res);
        goto end;
    }

    // Create full path to sdb in system folder
    hres = BuildPathToSdb(sysdirPatchPath, ARRAYSIZE(sysdirPatchPath), guidDbStr);
    if (FAILED(hres))
    {
        wprintf(L"Build path error\n");
        goto end;
    }

    wprintf(L"file path %ls\n", sysdirPatchPath);

    res = CopySdbToAppPatch(sdbPath, sysdirPatchPath);
    if (FAILED(res))
    {
        wprintf(L"Copy sdb error. Status: 0x%08X\n", res);
        goto end;
    }

    AddUninstallKey(dbName, sysdirPatchPath, guidDbStr);

    // Registration
    if (!SdbRegisterDatabaseEx(sysdirPatchPath, SDB_DATABASE_SHIM, &currentTime.QuadPart))
    {
        wprintf(L"SdbRegisterDatabaseEx failed");
        goto end;
    }

    res = TRUE;

end:
    if (pdb)
    {
        SdbCloseDatabase(pdb);
    }

    return res;
}

HRESULT
DeleteUninstallKey(
    _In_ LPCWSTR keyName)
{
    HKEY hKey = NULL;
    HRESULT hres = HRESULT_FROM_WIN32(ERROR_SUCCESS);

    LSTATUS status = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                    UNINSTALL_REG_PATH,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hKey,
                                    NULL);

    if (status != ERROR_SUCCESS)
    {
        hres = HRESULT_FROM_WIN32(status);
        goto end;
    }

    status = RegDeleteKeyW(hKey, keyName);
    if (status != ERROR_SUCCESS)
    {
        hres = HRESULT_FROM_WIN32(status);
    }

end:
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return hres;
}

BOOL
SdbUninstall(
    _In_ LPWSTR sdbPath)
{
    BOOL res = FALSE;
    PWCHAR sdbName = NULL;
    PDB pdb;
    TAGID tagDb;
    GUID dbGuid = {0};
    WCHAR guidDbStr[GUID_SBD_NAME_LENGTH];

    SIZE_T sdbPathLen = wcslen(sdbPath);
    sdbName = sdbPath + sdbPathLen;

    wprintf(L"uninstall path %ls\n", sdbPath);
    sdbName = StrRChrW(sdbPath, sdbPath + sdbPathLen, L'\\');
    if (sdbName == NULL)
    {
        sdbName = sdbPath;
    }
    else
    {
        sdbName++;
    }

    wprintf(L"uninstall name %ls\n", sdbName);

    // open sdb
    pdb = SdbOpenDatabase(sdbPath, DOS_PATH);
    if (pdb == NULL)
    {
        wprintf(L"Can't open database %ls\n", sdbPath);
        return FALSE;
    }

    tagDb = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    if (!tagDb)
    {
        wprintf(L"Can't find database tag\n");
        goto end;
    }

    if (!GetSdbGuid(pdb, tagDb, &dbGuid))
    {
        wprintf(L"GetSdbGuid error\n");
        goto end;
    }

    // Database name must be GUID string
    if (wcslen(sdbName) + 1 != GUID_SBD_NAME_LENGTH)
    {
        StringFromGUID2(&dbGuid, guidDbStr, GUID_SBD_NAME_LENGTH);
        SdbCloseDatabase(pdb);
        return SdbUninstallByGuid(guidDbStr);
    }

    //remove regkey in appatch/custom
    HRESULT hres = ProcessExe(pdb, tagDb, NULL, 0, FALSE);
    if (FAILED(hres))
    {
        wprintf(L"Process exe fail\n");
        goto end;
    }

    hres = ProcessLayers(pdb, tagDb, NULL, 0, FALSE);
    if (FAILED(hres))
    {
        wprintf(L"Process layers fail\n");
        goto end;
    }

    SdbCloseDatabase(pdb);
    pdb = NULL;

    hres = DeleteUninstallKey(sdbName);
    if (FAILED(hres))
    {
        wprintf(L"Remove uninstall key fail\n");
    }

    if (!SdbUnregisterDatabase(&dbGuid))
    {
        wprintf(L"SdbUnregisterDatabase\n");
        return FALSE;
    }

    SetFileAttributesW(sdbPath, FILE_ATTRIBUTE_NORMAL);
    if (!DeleteFileW(sdbPath))
    {
        wprintf(L"Remove file fail 0x%08X\n", GetLastError());
        return FALSE;
    }

    res = TRUE;

end:
    if (pdb)
        SdbCloseDatabase(pdb);
    return res;
}

BOOL
ValidateGuidString(
    _In_ PWCHAR guidStr)
{
    ULONG length = wcslen(guidStr);

    if (length == GUID_STR_LENGTH &&
        guidStr[0] == L'{' &&
        guidStr[GUID_STR_LENGTH - 1] == L'}' &&
        guidStr[9] == L'-' &&
        guidStr[14] == L'-' &&
        guidStr[19] == L'-' &&
        guidStr[24] == L'-')
    {
        return TRUE;
    }

    return FALSE;
}

BOOL
SdbUninstallByGuid(
    _In_ LPWSTR guidSdbStr)
{
    BOOL res = FALSE;
    HKEY hKey = NULL;
    HKEY guidKey = NULL;
    LSTATUS status;
    WCHAR dbPath[MAX_PATH];
    DWORD keyValSize = sizeof(dbPath);

    if (!ValidateGuidString(guidSdbStr))
    {
        wprintf(L"Invalid GUID: %ls\n", guidSdbStr);
        return res;
    }

    status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, APPCOMPAT_INSTALLEDSDB_REG_PATH, 0, KEY_READ | KEY_QUERY_VALUE, &hKey);

    if (status != ERROR_SUCCESS)
    {
        wprintf(L"RegOpenKeyW error: 0x%08X", status);
        goto end;
    }

    status = RegOpenKeyExW(hKey, guidSdbStr, 0, KEY_READ | KEY_QUERY_VALUE, &guidKey);

    if (status != ERROR_SUCCESS)
    {
        wprintf(L"Cant open key: 0x%08X %ls\n", status, guidSdbStr);
        goto end;
    }

    status = RegQueryValueExW(guidKey, DBPATH_KEY_NAME, NULL, NULL, (LPBYTE)dbPath, &keyValSize);
    if (status != ERROR_SUCCESS)
    {
        wprintf(L"RegQueryValueExW: 0x%08X\n", status);
        goto end;
    }

    res = SdbUninstall(dbPath);

end:
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    if (guidKey)
    {
        RegCloseKey(guidKey);
    }

    return res;
}

BOOL
SdbUninstallByName(
    _In_ LPWSTR nameSdbStr)
{
    BOOL res = FALSE;
    LSTATUS status;
    HKEY hKey = NULL;
    HKEY subKey = NULL;
    DWORD index = 0;
    WCHAR keyName[MAX_PATH];
    DWORD keyNameLen = ARRAYSIZE(keyName);
    DWORD keyValSize;
    WCHAR dbDescript[MAX_PATH];
    WCHAR dbPath[MAX_PATH];

    status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, APPCOMPAT_INSTALLEDSDB_REG_PATH, 0, KEY_READ | KEY_QUERY_VALUE, &hKey);

    if (status != ERROR_SUCCESS)
    {
        wprintf(L"RegOpenKeyW error: 0x%08X", status);
        goto end;
    }

    status = RegEnumKeyEx(hKey, index, keyName, &keyNameLen, NULL, NULL, NULL, NULL);
    wprintf(L"0x%08X %d %ls \n", status, keyNameLen, keyName);

    // Search database GUID by name
    while (status == ERROR_SUCCESS)
    {
        status = RegOpenKeyExW(hKey, keyName, 0, KEY_READ | KEY_QUERY_VALUE, &subKey);
        if (status != ERROR_SUCCESS)
        {
            break;
        }

        keyValSize = sizeof(dbDescript);
        status = RegQueryValueExW(subKey, L"DatabaseDescription", NULL, NULL, (LPBYTE)dbDescript, &keyValSize);
        if (status != ERROR_SUCCESS)
        {
            break;
        }

        wprintf(L"dbdescript: %ls \n", dbDescript);

        if (_wcsnicmp(dbDescript, nameSdbStr, keyNameLen) == 0)
        {
            // Take db full path
            keyValSize = sizeof(dbPath);
            status = RegQueryValueExW(subKey, DBPATH_KEY_NAME, NULL, NULL, (LPBYTE)dbPath, &keyValSize);
            if (status != ERROR_SUCCESS)
            {
                dbPath[0] = UNICODE_NULL;
                break;
            }

            wprintf(L"dbpath: 0x%08X %ls \n", status, dbPath);
            RegCloseKey(subKey);
            break;
        }

        RegCloseKey(subKey);

        keyName[0] = UNICODE_NULL;

        ++index;
        keyNameLen = ARRAYSIZE(keyName);
        status = RegEnumKeyExW(hKey, index, keyName, &keyNameLen, NULL, NULL, NULL, NULL);
    }

    RegCloseKey(hKey);

    if (dbPath[0] != UNICODE_NULL)
    {
        res = SdbUninstall(dbPath);
    }

end:
    return res;
}


void
ShowHelp()
{
    /* FIXME: to be localized */
    wprintf(L"Using: sdbinst [-?][-q][-u][-g][-n] foo.sdb | {guid} | \"name\" \n"
            L"-? - show help\n"
            L"-u - uninstall\n"
            L"-g - {guid} file GUID (only uninstall)\n"
            L"-n - \"name\" - file name (only uninstall)\n");
}

int _tmain(int argc, LPWSTR argv[])
{
    LPWSTR sdbPath = NULL;
    BOOL isInstall = TRUE;
    BOOL isUninstByGuid = FALSE;
    BOOL isUninstByName = FALSE;
    BOOL success = FALSE;
    LPWSTR guidSdbStr = NULL;
    LPWSTR nameSdbStr = NULL;

    if (argc < 2)
    {
        ShowHelp();
    }

    for (int i = 1; i < argc; ++i)
    {
        if (argv[i][0] != L'-')
        {
            sdbPath = argv[i];
            break;
        }

        switch (argv[i][1])
        {
            case L'?':
                ShowHelp();
                return ERROR_SUCCESS;
            break;

            case L'u':
                isInstall = FALSE;
            break;

            case L'g':
                ++i;

                if (i >= argc)
                {
                    return ERROR_INVALID_PARAMETER;
                }

                guidSdbStr = argv[i];
                wprintf(L"guidSdbStr %ls\n", guidSdbStr);

                isUninstByGuid = TRUE;
                isInstall = FALSE;
            break;

            case L'n':
                ++i;

                if (i >= argc)
                {
                    return ERROR_INVALID_PARAMETER;
                }

                nameSdbStr = argv[i];
                wprintf(L"nameSdbStr %ls\n", nameSdbStr);

                isUninstByName = TRUE;
                isInstall = FALSE;
            break;
        }
    }

    if (isInstall)
    {
        wprintf(L"install\n");
        success = SdbInstall(sdbPath);
    }
    else if (isUninstByGuid)
    {
        wprintf(L"uninstall by GUID\n");
        success = SdbUninstallByGuid(guidSdbStr);
    }
    else if (isUninstByName)
    {
        wprintf(L"uninstall by name\n");
        success = SdbUninstallByName(nameSdbStr);
    }
    else
    {
        wprintf(L"uninstall\n");
        success = SdbUninstall(sdbPath);
    }

    if (!success)
    {
        wprintf(isInstall ? L"Sdb install failed\n" : L"Sdb uninstall failed\n");
        return -1;
    }

    return ERROR_SUCCESS;
}
