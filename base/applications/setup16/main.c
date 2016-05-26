#define WIN32_NO_STATUS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <setupapi.h>

#define NT_PARAMS        L"NT3.51 Intel Params"
#define MSSETUP_PATH     L"~MSSETUP.T\\"

static UINT WINAPI ExtCabCallback(PVOID Context, UINT Notification, UINT_PTR Param1, UINT_PTR Param2)
{
    FILE_IN_CABINET_INFO_W *pInfo;
    FILEPATHS_W *pFilePaths;

    DBG_UNREFERENCED_LOCAL_VARIABLE(pFilePaths);

    switch(Notification)
    {
        case SPFILENOTIFY_FILEINCABINET:
            pInfo = (FILE_IN_CABINET_INFO_W*)Param1;
            wcscpy(pInfo->FullTargetName, (LPCWSTR)Context);
            wcscat(pInfo->FullTargetName, pInfo->NameInCabinet);
            return FILEOP_DOIT;
        case SPFILENOTIFY_FILEEXTRACTED:
            pFilePaths = (FILEPATHS_W*)Param1;
            return NO_ERROR;
    }
    return NO_ERROR;
}

BOOL DeleteDirectory(LPWSTR lpszDir)
{
    SHFILEOPSTRUCT fileop;
    DWORD len = wcslen(lpszDir);
    WCHAR *pszFrom = HeapAlloc(GetProcessHeap(), 0, (len + 2) * sizeof(WCHAR));
    int ret;

    wcscpy(pszFrom, lpszDir);
    pszFrom[len] = 0;
    pszFrom[len+1] = 0;

    fileop.hwnd   = NULL;
    fileop.wFunc  = FO_DELETE;
    fileop.pFrom  = pszFrom;
    fileop.pTo    = NULL;
    fileop.fFlags = FOF_NOCONFIRMATION|FOF_SILENT;
    fileop.fAnyOperationsAborted = FALSE;
    fileop.lpszProgressTitle     = NULL;
    fileop.hNameMappings         = NULL;

    ret = SHFileOperation(&fileop);
    HeapFree(GetProcessHeap(), 0, pszFrom);
    return (ret == 0);
}

VOID GetSystemDrive(LPWSTR lpszDrive)
{
    WCHAR szWindir[MAX_PATH];
    GetWindowsDirectoryW(szWindir, MAX_PATH);
    _wsplitpath(szWindir, lpszDrive, NULL, NULL, NULL);
    wcscat(lpszDrive, L"\\");
}

int APIENTRY wWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPWSTR    lpCmdLine,
                       int       nCmdShow)
{
    WCHAR szSetupPath[MAX_PATH];
    WCHAR szFileName[MAX_PATH];
    WCHAR szCabFileName[MAX_PATH];
    WCHAR szCabFilePath[MAX_PATH];
    WCHAR szTempDirName[50];
    WCHAR szCmdLine[MAX_PATH];
    WCHAR szTempCmdLine[MAX_PATH];
    WCHAR szTempPath[MAX_PATH];
    WCHAR szFullTempPath[MAX_PATH];
    WCHAR szDrive[4];
    DWORD dwAttrib;
    PROCESS_INFORMATION processInfo;
    STARTUPINFO startupInfo;
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    GetCurrentDirectory(MAX_PATH, szSetupPath);
    wcscat(szSetupPath, L"\\");
    wcscpy(szFileName, szSetupPath);
    wcscat(szFileName, L"setup.lst");

    if (GetFileAttributes(szFileName) == INVALID_FILE_ATTRIBUTES)
    {
        MessageBoxW(0, L"Cannot find Setup.lst file", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    /* read information from setup.lst */
    GetPrivateProfileStringW(NT_PARAMS, L"CabinetFile", NULL, szCabFileName, MAX_PATH, szFileName);
    GetPrivateProfileStringW(NT_PARAMS, L"TmpDirName", NULL, szTempDirName, 50, szFileName);
    GetPrivateProfileStringW(NT_PARAMS, L"CmdLine", NULL, szCmdLine, MAX_PATH, szFileName);

    wcscpy(szCabFilePath, szSetupPath);
    wcscat(szCabFilePath, szCabFileName);

    /* ceate temp directory */
    GetSystemDrive(szDrive);
    wcscpy(szTempPath, szDrive);
    wcscat(szTempPath, MSSETUP_PATH);
    wcscpy(szFullTempPath, szTempPath);
    wcscat(szFullTempPath, szTempDirName);
    wcscat(szFullTempPath, L"\\");

    if (SHCreateDirectoryEx(0, szFullTempPath, NULL) != ERROR_SUCCESS)
    {
        MessageBoxW(0, L"Could not create Temp Directory.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    dwAttrib = GetFileAttributes(szTempPath);
    SetFileAttributes(szTempPath, dwAttrib | FILE_ATTRIBUTE_HIDDEN);

    /* extract files */
    if (!SetupIterateCabinetW(szCabFilePath, 0, ExtCabCallback, szFullTempPath))
    {
        MessageBoxW(0, L"Could not extract cab file", L"Error", MB_OK | MB_ICONERROR);
        DeleteDirectory(szTempPath);
        return 1;
    }

    /* prepare command line */
    wsprintf(szTempCmdLine, szCmdLine, szFullTempPath, lpCmdLine);
    wcscpy(szCmdLine, szFullTempPath);
    wcscat(szCmdLine, szTempCmdLine);

    /* execute the 32-Bit installer */
    ZeroMemory(&processInfo, sizeof(processInfo));
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    if (CreateProcessW(NULL, szCmdLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, szFullTempPath, &startupInfo, &processInfo))
    {
        WaitForSingleObject(processInfo.hProcess, INFINITE);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
    }
    else
    {
        WCHAR szMsg[MAX_PATH];
        wsprintf(szMsg, L"Failed to load the installer. Error %lu", GetLastError());
        MessageBoxW(0, szMsg, L"Error", MB_OK | MB_ICONERROR);
        DeleteDirectory(szTempPath);
        return 1;
    }

    /* cleanup */
    DeleteDirectory(szTempPath);

    return 0;
}

/* EOF */
