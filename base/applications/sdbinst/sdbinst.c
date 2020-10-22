#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <tchar.h>
#include <winreg.h>
#include <strsafe.h>
#include <objbase.h>
#include <apphelp.h>



#define APPCOMPAT_CUSTOM_REG_PATH L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Custom"
#define APPCOMPAT_LAYERS_REG_PATH L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"
#define UNINSTALL_REG_PATH L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"

HRESULT
RegisterSdbEntry(
    _In_ PWCHAR sdbEntryName,
    _In_ LPCWSTR dbGuid,
    _In_ ULONGLONG time,
    _In_ BOOL isInstall,
    _In_ BOOL isExe)
{
    WCHAR regName[MAX_PATH] = {0};
    HKEY  hKey;
    LSTATUS status;

    HRESULT hres = StringCchPrintf(regName, sizeof(regName) / sizeof(WCHAR), L"%ls\\%ls",
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
    return hres;
}

HRESULT
AddUninstallKey(
    _In_ LPCWSTR dbName,
    _In_ LPCWSTR sdbInstalledPath,
    _In_ LPCWSTR guidDbStr)
{
    WCHAR sdbinstPath[MAX_PATH] = {0};
    WCHAR regName[MAX_PATH] = {0};
    WCHAR uninstString[MAX_PATH * 2] = {0};
    HKEY hKey;
    HRESULT hres;

    UINT count = GetSystemWindowsDirectory(sdbinstPath, sizeof(sdbinstPath) / sizeof(WCHAR));
    if (sdbinstPath[count - 1] != L'\\')
    {
        sdbinstPath[count] = L'\\';
    }
    
    // Full path to sdbinst.exe
    hres = StringCchCat(sdbinstPath, sizeof(sdbinstPath) / sizeof(WCHAR), L"System32\\sdbinst.exe");
    if (FAILED(hres))
    {
        wprintf(L"StringCchCat error: 0x%08X", hres);
    }

    // Sdb guid reg key
    hres = StringCchPrintf(regName, sizeof(regName) / sizeof(WCHAR), L"%ls\\%ls", UNINSTALL_REG_PATH, guidDbStr);
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
    hres = StringCchPrintf(uninstString, sizeof(uninstString) / sizeof(WCHAR), L"%ls -u \"%ls\"", sdbinstPath, sdbInstalledPath);
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
    WCHAR guidDbStr[MAX_PATH] = {0};

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
    HRESULT hres = StringCchCatW(guidDbStr, sizeof(guidDbStr) / sizeof(WCHAR), L".sdb");
    if (FAILED(hres))
    {
        wprintf(L"StringCchCatW error 0x%08X\n", hres);
        goto end;
    }

    wprintf(L"Database guid %wZ\n", &guidDbStr);

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

    return res;
}

HRESULT
DeleteUninstallKey(
    _In_ LPCWSTR keyName)
{
    HKEY hKey;
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
    GUID dbGuid;

    SIZE_T sdbPathLen = wcslen(sdbPath);
    sdbName = sdbPath + sdbPathLen;

    while (*sdbName != L'\\')
    {
        --sdbName;
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
        goto end;
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

end:
    return res;
}



void
ShowHelp()
{
    wprintf(L"Using: sdbinst [-?][-q][-u][-g][-n] foo.sdb | {guid} | \"name\" \n"
            L"-? - show help\n"
            L"-q - silence mode\n"
            L"-u - uninstall\n"
            L"-g - {guid} file guid (only uninstall)\n"
            L"-n - \"name\" - file name (only uninstall)\n");
}

int _tmain(int argc, LPWSTR argv[])
{
    LPWSTR sdbPath = NULL;
    BOOL isInstall = TRUE;
    //LPWSTR guidSdbStr = NULL; 

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

                if (i < argc)
                {
                    return ERROR_INVALID_PARAMETER;
                }

                //guidSdbStr = argv[i];
            break;
        }
    }

    if (isInstall)
    {
        SdbInstall(sdbPath);
    }
    else
    {
        // call uninstall
        SdbUninstall(sdbPath);
    }

    return ERROR_SUCCESS;
}
