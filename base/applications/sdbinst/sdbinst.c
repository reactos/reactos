#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <tchar.h>
#include <winreg.h>
#include <strsafe.h>
#include <objbase.h>
#include <apphelp.h>



#define APPCOMPAT_REG_PATH L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Custom\\"
#define UNINSTALL_REG_PATH L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"

HRESULT
RegisterSdbEntry(
    _In_ PWCHAR sdbEntryName,
    _In_ LPCWSTR dbGuid,
    _In_ ULONGLONG time)
{
    WCHAR regName[MAX_PATH] = {0};
    HKEY  hKey;

    HRESULT hres = StringCchPrintf(regName, sizeof(regName) / sizeof(WCHAR), L"%ls\\%ls", APPCOMPAT_REG_PATH, sdbEntryName);
    if (FAILED(hres))
    {
        wprintf(L"StringCchPrintfW error: 0x%08X", hres);
        goto end;
    }

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
    
    hres = StringCchCat(sdbinstPath, sizeof(sdbinstPath) / sizeof(WCHAR), L"System32\\sdbinst.exe");
    if (FAILED(hres))
    {
        wprintf(L"StringCchCat error: 0x%08X", hres);
    }

    hres = StringCchPrintf(regName, sizeof(regName) / sizeof(WCHAR), L"%ls\\%ls", UNINSTALL_REG_PATH, guidDbStr);
    if (FAILED(hres))
    {
        wprintf(L"StringCchPrintfW error: 0x%08X", hres);
        goto end;
    }

    wprintf(L"%ls\n", sdbinstPath);
    wprintf(L"%ls\n", regName);

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

    // Uninst string 
    hres = StringCchPrintf(uninstString, sizeof(uninstString) / sizeof(WCHAR), L"%ls -u \"%ls\"", sdbinstPath, sdbInstalledPath);
    if (FAILED(hres))
    {
        wprintf(L"StringCchPrintfW error: 0x%08X", hres);
        goto end;
    }

    //wprintf(L"%ls\n", uninstString);

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
    _In_ LPCWSTR guidDbStr,
    _In_ ULONGLONG time)
{
    HRESULT res = ERROR_SUCCESS;
    TAGID tagLayerName;

    TAGID tagLayer = SdbFindFirstTag(pdb, tagDb, TAG_LAYER);
    
    while (tagLayer)
    {
        tagLayerName = SdbFindFirstTag(pdb, tagLayer, TAG_NAME);
        if (!tagLayerName)
        {
            res = ERROR_NOT_FOUND;
            break;
        }

        LPWSTR name = SdbGetStringTagPtr(pdb, tagLayerName);
        wprintf(L"Layer name %ls", name);

        res = RegisterSdbEntry(name, guidDbStr, time);
        if (FAILED(res))
        {
            wprintf(L"Can't register layer\n");
            break;
        }

        tagLayer = SdbFindNextTag(pdb, tagDb, TAG_LAYER);
    }

    return res;
}

HRESULT
ProcessExe(
    _In_ PDB pdb,
    _In_ TAGID tagDb,
    _In_ LPCWSTR guidDbStr,
    _In_ ULONGLONG time
)
{
    HRESULT res = ERROR_SUCCESS;
    TAGID tagExeName;

    TAGID tagExe = SdbFindFirstTag(pdb, tagDb, TAG_EXE);

    while (tagExe)
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

        res = RegisterSdbEntry(name, guidDbStr, time);
        if (FAILED(res))
        {
            wprintf(L"Can't register exe\n");
            break;
        }

        tagExe = SdbFindNextTag(pdb, tagDb, TAG_EXE);
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

    CopyMemory(sysdirPath, destSdbPath, destLen * sizeof(WCHAR));
    pTmpSysdir = sysdirPath + destLen;

    while (*pTmpSysdir != L'\\')
    {
        *pTmpSysdir = L'\0';
        --pTmpSysdir;
    }

    wprintf(L"%ls\n", sysdirPath);

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

    hres = ProcessExe(pdb, tagDb, guidDbStr, currentTime.QuadPart);
    if (FAILED(hres))
    {
        wprintf(L"Process exe failed. Status: 0x%08X", res);
        goto end;
    }

    hres = ProcessLayers(pdb, tagDb, guidDbStr, currentTime.QuadPart);
    if (FAILED(hres))
    {
        wprintf(L"Process layers failed. Status: 0x%08X", res);
        goto end;
    }

    SIZE_T bufLen = MAX_PATH * 2;
    sysdirPatchPath = (PWCHAR)HeapAlloc(GetProcessHeap(), 0, bufLen * sizeof(WCHAR));

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

int _tmain(int argc, LPCWSTR argv[])
{
    LPCWSTR sdbPath = NULL;

    if (argc < 2)
    {
        ShowHelp();
    }

    if (argv[1][0] == L'-' && argv[1][1] == L'?')
    {
        ShowHelp();
        return ERROR_SUCCESS;
    }

    sdbPath = argv[1];

    SdbInstall(sdbPath);

    return ERROR_SUCCESS;
}