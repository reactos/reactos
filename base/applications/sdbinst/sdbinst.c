/*
 * PROJECT:     ReactOS sdbinst
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Application compatibility database installer
 * COPYRIGHT:   Copyright 2020 Max Korostil (mrmks04@yandex.ru)
 */


#include <windef.h>
#include <winbase.h>
#include <tchar.h>
#include <winreg.h>
#include <strsafe.h>
#include <objbase.h>
#include <apphelp.h>



#define APPCOMPAT_CUSTOM_REG_PATH L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Custom"
#define APPCOMPAT_LAYERS_REG_PATH L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"
#define APPCOMPAT_INSTALLEDSDB_REG_PATH L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\InstalledSDB"
#define UNINSTALL_REG_PATH L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
#define DBPATH_KEY_NAME L"DatabasePath"

HRESULT
RegisterSdbEntry(
    _In_ PWCHAR sdbEntryName,
    _In_ LPCWSTR dbGuid,
    _In_ ULONGLONG time,
    _In_ BOOL isInstall,
    _In_ BOOL isExe)
{
    PWCHAR regName;
    HKEY  hKey = NULL;
    LSTATUS status;
    HRESULT hres;

    regName = (PWCHAR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    if (!regName)
    {
        hres = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto end;
    }
    ZeroMemory(regName, MAX_PATH * sizeof(WCHAR));

    hres = StringCchPrintf(regName, MAX_PATH, L"%ls\\%ls",
                     isExe ? APPCOMPAT_CUSTOM_REG_PATH : APPCOMPAT_LAYERS_REG_PATH, sdbEntryName);
    if (FAILED(hres))
    {
        wprintf(L"StringCchPrintfW error: 0x%08X\n", hres);
        goto end;
    }

    // Remove key
    if (!isInstall)
    {
        status = RegDeleteKey(HKEY_LOCAL_MACHINE, regName);
        return HRESULT_FROM_WIN32(status);
    }

    // Create main key
    status = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
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

    // Set instlled time
    status = RegSetValueEx(hKey,
                           dbGuid,
                           0,
                           REG_QWORD,
                           (PBYTE)&time,
                           sizeof(time));
    if (status != ERROR_SUCCESS)
    {
        wprintf(L"RegSetValueEx error: 0x%08X", status);
        hres = HRESULT_FROM_WIN32(status);
        goto end;
    }

end:
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    if (regName)
    {
        HeapFree(GetProcessHeap(), 0, regName);
    }

    return hres;
}

HRESULT
AddUninstallKey(
    _In_ LPCWSTR dbName,
    _In_ LPCWSTR sdbInstalledPath,
    _In_ LPCWSTR guidDbStr)
{
    PWCHAR sdbinstPath;
    PWCHAR regName;
    PWCHAR uninstString;
    HKEY hKey = NULL;
    HRESULT hres;

    sdbinstPath = (PWCHAR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    regName = (PWCHAR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    uninstString = (PWCHAR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));

    if (!sdbinstPath || !regName || !uninstString)
    {
        hres = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto end;
    }

    ZeroMemory(sdbinstPath, MAX_PATH * sizeof(WCHAR));
    ZeroMemory(regName, MAX_PATH * sizeof(WCHAR));
    ZeroMemory(uninstString, MAX_PATH * sizeof(WCHAR));

    UINT count = GetSystemWindowsDirectory(sdbinstPath, MAX_PATH);
    if (sdbinstPath[count - 1] != L'\\')
    {
        sdbinstPath[count] = L'\\';
    }
    
    // Full path to sdbinst.exe
    hres = StringCchCat(sdbinstPath, MAX_PATH, L"System32\\sdbinst.exe");
    if (FAILED(hres))
    {
        wprintf(L"StringCchCat error: 0x%08X", hres);
    }

    // Sdb guid reg key
    hres = StringCchPrintf(regName, MAX_PATH, L"%ls\\%ls", UNINSTALL_REG_PATH, guidDbStr);
    if (FAILED(hres))
    {
        wprintf(L"StringCchPrintfW error: 0x%08X", hres);
        goto end;
    }

    wprintf(L"%ls\n", sdbinstPath);
    wprintf(L"%ls\n", regName);

    // Create main key
    LSTATUS status = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
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
    status = RegSetValueEx(hKey,
                           L"DisplayName",
                           0,
                           REG_SZ,
                           (PBYTE)dbName,
                           length + sizeof(WCHAR));
    if (status != ERROR_SUCCESS)
    {
        wprintf(L"RegSetValueEx error: 0x%08X", status);
        hres = HRESULT_FROM_WIN32(status);
        goto end;
    }

    // Uninstall full string 
    hres = StringCchPrintf(uninstString, MAX_PATH, L"%ls -u \"%ls\"", sdbinstPath, sdbInstalledPath);
    if (FAILED(hres))
    {
        wprintf(L"StringCchPrintfW error: 0x%08X", hres);
        goto end;
    }

    // Set uninstall string
    length = wcslen(uninstString) * sizeof(WCHAR);
    status = RegSetValueEx(hKey,
                           L"UninstallString",
                           0,
                           REG_SZ,
                           (PBYTE)uninstString,
                           length + sizeof(WCHAR));
    if (status != ERROR_SUCCESS)
    {
        wprintf(L"RegSetValueEx error: 0x%08X", status);
        hres = HRESULT_FROM_WIN32(status);
        goto end;
    }

end:
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    if (sdbinstPath)
    {
        HeapFree(GetProcessHeap(), 0, sdbinstPath);
    }

    if (regName)
    {
        HeapFree(GetProcessHeap(), 0, regName);
    }

    if (uninstString)
    {
        HeapFree(GetProcessHeap(), 0, uninstString);
    }

    return hres;
}

//
// Get database guid id
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
    
    // Add all exe to registry (AppCompatFlags)
    while (tagLayer && (tagLayer != prevTagLayer))
    {
        tagLayerName = SdbFindFirstTag(pdb, tagLayer, TAG_NAME);
        if (!tagLayerName)
        {
            res = ERROR_NOT_FOUND;
            break;
        }

        LPWSTR name = SdbGetStringTagPtr(pdb, tagLayerName);
        wprintf(L"Layer name %ls", name);

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
        wprintf(L"Exe name %ls\n", name);

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
    pTmpSysdir = sysdirPath + destLen;

    while (*pTmpSysdir != L'\\')
    {
        *pTmpSysdir = L'\0';
        --pTmpSysdir;
    }

    wprintf(L"%ls\n", sysdirPath);

    // Create directory if need
    if (!CreateDirectory(sysdirPath, NULL))
    {
        error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS)
        {
            wprintf(L"Can't create folder %ls\n Error: 0x%08\n", sysdirPath, error);
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

    HeapFree(GetProcessHeap(), 0, sysdirPath);

end:
    return HRESULT_FROM_WIN32(error);
}

HRESULT
BuildPathToSdb(
    _In_ PWCHAR* buffer,
    _In_ SIZE_T bufLen,
    _In_ LPCWSTR guidDbStr)
{
    PWCHAR pBuffer = *buffer;
    ZeroMemory(pBuffer, bufLen * sizeof(WCHAR));

    UINT count = GetSystemWindowsDirectory(pBuffer, bufLen);
    if (pBuffer[count - 1] != L'\\')
    {
        pBuffer[count] = L'\\';
    }

    HRESULT res = StringCchCatW(pBuffer, bufLen, L"AppPatch\\Custom\\");
    if (FAILED(res))
    {
        goto end;
    }

    res = StringCchCatW(pBuffer, bufLen, guidDbStr);

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
    PWCHAR sysdirPatchPath = NULL;
    PWCHAR guidDbStr;

    guidDbStr = (PWCHAR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    if (!guidDbStr)
    {
        goto end;
    }
    ZeroMemory(guidDbStr, MAX_PATH * sizeof(WCHAR));

    GetSystemTimeAsFileTime(&systemTime);
    currentTime.LowPart  = systemTime.dwLowDateTime;
    currentTime.HighPart = systemTime.dwHighDateTime;

    // Open db
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

    // Get db guid
    if (!GetSdbGuid(pdb, tagDb, &dbGuid))    
    {
        wprintf(L"GetSdbGuid error\n");
        goto end;
    }

    StringFromGUID2(&dbGuid, guidDbStr, MAX_PATH);
    HRESULT hres = StringCchCatW(guidDbStr, MAX_PATH, L".sdb");
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

    SIZE_T bufLen = MAX_PATH * 2;
    sysdirPatchPath = (PWCHAR)HeapAlloc(GetProcessHeap(), 0, bufLen * sizeof(WCHAR));

    // Create full path tos db in system folder
    hres = BuildPathToSdb(&sysdirPatchPath, bufLen, guidDbStr);
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
        wprintf(L"SdbRegisterDatabaseEx UNSUCCESS");
        goto end;
    }

    res = TRUE;

end:
    if (pdb)
    {
        SdbCloseDatabase(pdb);
    }

    if (sysdirPatchPath)
    {
        HeapFree(GetProcessHeap(), 0, sysdirPatchPath);
    }

    if (guidDbStr)
    {
        HeapFree(GetProcessHeap(), 0, guidDbStr);
    }

    return res;
}

HRESULT
DeleteUninstallKey(
    _In_ LPCWSTR keyName)
{
    HKEY hKey = NULL;
    HRESULT hres = HRESULT_FROM_WIN32(ERROR_SUCCESS);

    LSTATUS status = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
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

    status = RegDeleteKey(hKey, keyName);
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

    SIZE_T sdbPathLen = wcslen(sdbPath);
    sdbName = sdbPath + sdbPathLen;

    wprintf(L"uninstall name %ls\n", sdbPath);
    while (*sdbName != L'\\' && sdbPathLen > 0)
    {
        --sdbName;
        --sdbPathLen;
    }
    sdbName++;

    wprintf(L"uninstall name %ls\n", sdbName);

    // open sdb
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

    //
    if (!GetSdbGuid(pdb, tagDb, &dbGuid))    
    {
        wprintf(L"GetSdbGuid error\n");
        goto end;
    }

    //remove regkey in appatch/custom
    HRESULT hres = ProcessExe(pdb, tagDb, NULL, 0, FALSE);
    if (FAILED(hres))
    {
        wprintf(L"Process exe fail\n");
        goto end;
    }

    SdbCloseDatabase(pdb);

    hres = DeleteUninstallKey(sdbName);
    if (FAILED(hres))
    {
        wprintf(L"Remove key fail\n");
        //goto end;
    }

    if (!SdbUnregisterDatabase(&dbGuid))
    {
        wprintf(L"SdbUnregisterDatabase\n");
        goto end;
    }

    if (!DeleteFile(sdbPath))
    {
        wprintf(L"Remove file fail 0x%08X\n", GetLastError());
        goto end;
    }

    res = TRUE;

end:
    return res;
}

#define BUFFER_SIZE (MAX_PATH * sizeof(WCHAR) + sizeof(WCHAR))
#define STRING_LEN  (MAX_PATH + 1)

BOOL
SdbUninstallByGuid(
    _In_ LPWSTR guidSdbStr)
{
    BOOL res = FALSE;
    HKEY hKey = NULL;
    HKEY guidKey = NULL;
    LSTATUS status;
    DWORD keyValSize = BUFFER_SIZE;
    PWCHAR dbPath = NULL;

    dbPath = (PWCHAR)HeapAlloc(GetProcessHeap(), 0, BUFFER_SIZE);
    if (!dbPath)
    {
        goto end;
    }

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, APPCOMPAT_INSTALLEDSDB_REG_PATH, 0, KEY_READ | KEY_QUERY_VALUE, &hKey);

    if (status != ERROR_SUCCESS)
    {
        wprintf(L"RegOpenKey error: 0x%08X", status);
        goto end;
    }

    status = RegOpenKeyEx(hKey, guidSdbStr, 0, KEY_READ | KEY_QUERY_VALUE, &guidKey);

    if (status != ERROR_SUCCESS)
    {
        wprintf(L"Cant open key: 0x%08X %ls\n", status, guidSdbStr);
        goto end;
    }

    status = RegQueryValueEx(guidKey, DBPATH_KEY_NAME, NULL, NULL, (LPBYTE)dbPath, &keyValSize);
    if (status != ERROR_SUCCESS)
    {
        wprintf(L"RegQueryValueEx: 0x%08X\n", status);
        goto end;
    }

    res = SdbUninstall(dbPath);
    
end:
    if (dbPath)
    {
        HeapFree(GetProcessHeap(), 0, dbPath);
    }

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
    DWORD keyNameLen = STRING_LEN;
    PWCHAR keyName = NULL;
    DWORD keyValSize;
    PWCHAR dbDescript = NULL;
    PWCHAR dbPath = NULL;

    keyName =    (PWCHAR)HeapAlloc(GetProcessHeap(), 0, BUFFER_SIZE);
    dbDescript = (PWCHAR)HeapAlloc(GetProcessHeap(), 0, BUFFER_SIZE);
    dbPath =     (PWCHAR)HeapAlloc(GetProcessHeap(), 0, BUFFER_SIZE);

    if (!keyName || !dbDescript || !dbPath)
    {
        goto end;
    }

    ZeroMemory(keyName, BUFFER_SIZE);
    ZeroMemory(dbDescript, BUFFER_SIZE);
    ZeroMemory(dbPath, BUFFER_SIZE);

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, APPCOMPAT_INSTALLEDSDB_REG_PATH, 0, KEY_READ | KEY_QUERY_VALUE, &hKey);

    if (status != ERROR_SUCCESS)
    {
        wprintf(L"RegOpenKey error: 0x%08X", status);
        goto end;
    }

    status = RegEnumKeyEx(hKey, index, keyName, &keyNameLen, NULL, NULL, NULL, NULL);
    wprintf(L"0x%08X %d %ls \n", status, keyNameLen, keyName);

    // Search db guid by name
    while (status == ERROR_SUCCESS)
    {
        status = RegOpenKeyEx(hKey, keyName, 0, KEY_READ | KEY_QUERY_VALUE, &subKey);
        if (status != ERROR_SUCCESS)
        {
            break;
        }

        keyValSize = BUFFER_SIZE;
        status = RegQueryValueEx(subKey, L"DatabaseDescription", NULL, NULL, (LPBYTE)dbDescript, &keyValSize);
        if (status != ERROR_SUCCESS)
        {
            break;
        }

        wprintf(L"dbdescript: %ls \n", dbDescript);

        if (_wcsnicmp(dbDescript, nameSdbStr, keyNameLen) == 0)
        {
            // Take db full path
            keyValSize = BUFFER_SIZE;
            status = RegQueryValueEx(subKey, DBPATH_KEY_NAME, NULL, NULL, (LPBYTE)dbPath, &keyValSize);
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
        keyNameLen = STRING_LEN;
        status = RegEnumKeyEx(hKey, index, keyName, &keyNameLen, NULL, NULL, NULL, NULL);        
    }

    RegCloseKey(hKey);

    if (dbPath[0] != UNICODE_NULL)
    {
        res = SdbUninstall(dbPath);
    }

end:
    if (dbPath)
    {
        HeapFree(GetProcessHeap(), 0, dbPath);
    }

    if (dbDescript)
    {
        HeapFree(GetProcessHeap(), 0, dbDescript);
    }

    if (keyName)
    {
        HeapFree(GetProcessHeap(), 0, keyName);
    }

    return res;
}


void
ShowHelp()
{
    wprintf(L"Using: sdbinst [-?][-q][-u][-g][-n] foo.sdb | {guid} | \"name\" \n"
            L"-? - show help\n"
            //L"-q - silence mode\n"
            L"-u - uninstall\n"
            L"-g - {guid} file guid (only uninstall)\n"
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
                wprintf(L"guidSdbStr %ls\n", nameSdbStr);

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
        wprintf(L"uninstall by guid\n");
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
        wprintf(L"Sdb install failed\n");
        return -1;
    }

    return ERROR_SUCCESS;
}
