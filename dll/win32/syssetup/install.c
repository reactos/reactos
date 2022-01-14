/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           System setup
 * FILE:              dll/win32/syssetup/install.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#define COBJMACROS

#include <io.h>
#include <wincon.h>
#include <winnls.h>
#include <winsvc.h>
#include <userenv.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <rpcproxy.h>
#include <ndk/cmfuncs.h>

#define NDEBUG
#include <debug.h>

//DWORD WINAPI
//CMP_WaitNoPendingInstallEvents(DWORD dwTimeout);

DWORD WINAPI
SetupStartService(LPCWSTR lpServiceName, BOOL bWait);

/* GLOBALS ******************************************************************/

HINF hSysSetupInf = INVALID_HANDLE_VALUE;
ADMIN_INFO AdminInfo;

typedef struct _DLG_DATA
{
    HBITMAP hLogoBitmap;
    HBITMAP hBarBitmap;
    HWND hWndBarCtrl;
    DWORD BarCounter;
    DWORD BarWidth;
    DWORD BarHeight;
} DLG_DATA, *PDLG_DATA;

/* FUNCTIONS ****************************************************************/

static VOID
FatalError(char *pszFmt,...)
{
    char szBuffer[512];
    va_list ap;

    va_start(ap, pszFmt);
    vsprintf(szBuffer, pszFmt, ap);
    va_end(ap);

    LogItem(NULL, L"Failed");

    strcat(szBuffer, "\nRebooting now!");
    MessageBoxA(NULL,
                szBuffer,
                "ReactOS Setup",
                MB_OK);
}

static HRESULT
CreateShellLink(
    LPCWSTR pszLinkPath,
    LPCWSTR pszCmd,
    LPCWSTR pszArg,
    LPCWSTR pszDir,
    LPCWSTR pszIconPath,
    INT iIconNr,
    LPCWSTR pszComment)
{
    IShellLinkW *psl;
    IPersistFile *ppf;

    HRESULT hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLink, (LPVOID*)&psl);

    if (SUCCEEDED(hr))
    {
        hr = IShellLinkW_SetPath(psl, pszCmd);

        if (pszArg)
            hr = IShellLinkW_SetArguments(psl, pszArg);

        if (pszDir)
            hr = IShellLinkW_SetWorkingDirectory(psl, pszDir);

        if (pszIconPath)
            hr = IShellLinkW_SetIconLocation(psl, pszIconPath, iIconNr);

        if (pszComment)
            hr = IShellLinkW_SetDescription(psl, pszComment);

        hr = IShellLinkW_QueryInterface(psl, &IID_IPersistFile, (LPVOID*)&ppf);

        if (SUCCEEDED(hr))
        {
            hr = IPersistFile_Save(ppf, pszLinkPath, TRUE);
            IPersistFile_Release(ppf);
        }

        IShellLinkW_Release(psl);
    }

    return hr;
}


static BOOL
CreateShortcut(
    LPCWSTR pszFolder,
    LPCWSTR pszName,
    LPCWSTR pszCommand,
    LPCWSTR pszDescription,
    INT iIconNr,
    LPCWSTR pszWorkingDir)
{
    DWORD dwLen;
    LPWSTR Ptr;
    LPWSTR lpFilePart;
    WCHAR szPath[MAX_PATH];
    WCHAR szWorkingDirBuf[MAX_PATH];

    /* If no working directory is provided, try to compute a default one */
    if (pszWorkingDir == NULL || pszWorkingDir[0] == L'\0')
    {
        if (ExpandEnvironmentStringsW(pszCommand, szPath, ARRAYSIZE(szPath)) == 0)
            wcscpy(szPath, pszCommand);

        dwLen = GetFullPathNameW(szPath,
                                 ARRAYSIZE(szWorkingDirBuf),
                                 szWorkingDirBuf,
                                 &lpFilePart);
        if (dwLen != 0 && dwLen <= ARRAYSIZE(szWorkingDirBuf))
        {
            /* Since those should only be called with (.exe) files,
               lpFilePart has not to be NULL */
            ASSERT(lpFilePart != NULL);

            /* We're only interested in the path. Cut the file name off.
               Also remove the trailing backslash unless the working directory
               is only going to be a drive, i.e. C:\ */
            *(lpFilePart--) = L'\0';
            if (!(lpFilePart - szWorkingDirBuf == 2 &&
                  szWorkingDirBuf[1] == L':' && szWorkingDirBuf[2] == L'\\'))
            {
                *lpFilePart = L'\0';
            }
            pszWorkingDir = szWorkingDirBuf;
        }
    }

    /* If we failed to compute a working directory, just do not use one */
    if (pszWorkingDir && pszWorkingDir[0] == L'\0')
        pszWorkingDir = NULL;

    /* Build the shortcut file name */
    wcscpy(szPath, pszFolder);
    Ptr = PathAddBackslash(szPath);
    wcscpy(Ptr, pszName);

    /* Create the shortcut */
    return SUCCEEDED(CreateShellLink(szPath,
                                     pszCommand,
                                     L"",
                                     pszWorkingDir,
                                     /* Special value to indicate no icon */
                                     (iIconNr != -1 ? pszCommand : NULL),
                                     iIconNr,
                                     pszDescription));
}


static BOOL CreateShortcutsFromSection(HINF hinf, LPWSTR pszSection, LPCWSTR pszFolder)
{
    INFCONTEXT Context;
    DWORD dwFieldCount;
    INT iIconNr;
    WCHAR szCommand[MAX_PATH];
    WCHAR szName[MAX_PATH];
    WCHAR szDescription[MAX_PATH];
    WCHAR szDirectory[MAX_PATH];

    if (!SetupFindFirstLine(hinf, pszSection, NULL, &Context))
        return FALSE;

    do
    {
        dwFieldCount = SetupGetFieldCount(&Context);
        if (dwFieldCount < 3)
            continue;

        if (!SetupGetStringFieldW(&Context, 1, szCommand, ARRAYSIZE(szCommand), NULL))
            continue;

        if (!SetupGetStringFieldW(&Context, 2, szName, ARRAYSIZE(szName), NULL))
            continue;

        if (!SetupGetStringFieldW(&Context, 3, szDescription, ARRAYSIZE(szDescription), NULL))
            continue;

        if (dwFieldCount < 4 || !SetupGetIntField(&Context, 4, &iIconNr))
            iIconNr = -1; /* Special value to indicate no icon */

        if (dwFieldCount < 5 || !SetupGetStringFieldW(&Context, 5, szDirectory, ARRAYSIZE(szDirectory), NULL))
            szDirectory[0] = L'\0';

        wcscat(szName, L".lnk");

        CreateShortcut(pszFolder, szName, szCommand, szDescription, iIconNr, szDirectory);

    } while (SetupFindNextLine(&Context, &Context));

    return TRUE;
}

static BOOL CreateShortcuts(HINF hinf, LPCWSTR szSection)
{
    INFCONTEXT Context;
    WCHAR szPath[MAX_PATH];
    WCHAR szFolder[MAX_PATH];
    WCHAR szFolderSection[MAX_PATH];
    INT csidl;

    CoInitialize(NULL);

    if (!SetupFindFirstLine(hinf, szSection, NULL, &Context))
        return FALSE;

    do
    {
        if (SetupGetFieldCount(&Context) < 2)
            continue;

        if (!SetupGetStringFieldW(&Context, 0, szFolderSection, ARRAYSIZE(szFolderSection), NULL))
            continue;

        if (!SetupGetIntField(&Context, 1, &csidl))
            continue;

        if (!SetupGetStringFieldW(&Context, 2, szFolder, ARRAYSIZE(szFolder), NULL))
            continue;

        if (FAILED(SHGetFolderPathAndSubDirW(NULL, csidl|CSIDL_FLAG_CREATE, (HANDLE)-1, SHGFP_TYPE_DEFAULT, szFolder, szPath)))
            continue;

        CreateShortcutsFromSection(hinf, szFolderSection, szPath);

    } while (SetupFindNextLine(&Context, &Context));

    CoUninitialize();

    return TRUE;
}

static VOID
CreateTempDir(
    IN LPCWSTR VarName)
{
    WCHAR szTempDir[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];
    DWORD dwLength;
    HKEY hKey;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
                      0,
                      KEY_QUERY_VALUE,
                      &hKey) != ERROR_SUCCESS)
    {
        FatalError("Error: %lu\n", GetLastError());
        return;
    }

    /* Get temp dir */
    dwLength = sizeof(szBuffer);
    if (RegQueryValueExW(hKey,
                         VarName,
                         NULL,
                         NULL,
                         (LPBYTE)szBuffer,
                         &dwLength) != ERROR_SUCCESS)
    {
        FatalError("Error: %lu\n", GetLastError());
        goto cleanup;
    }

    /* Expand it */
    if (!ExpandEnvironmentStringsW(szBuffer, szTempDir, ARRAYSIZE(szTempDir)))
    {
        FatalError("Error: %lu\n", GetLastError());
        goto cleanup;
    }

    /* Create profiles directory */
    if (!CreateDirectoryW(szTempDir, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            FatalError("Error: %lu\n", GetLastError());
            goto cleanup;
        }
    }

cleanup:
    RegCloseKey(hKey);
}

static BOOL
InstallSysSetupInfDevices(VOID)
{
    INFCONTEXT InfContext;
    WCHAR szLineBuffer[256];
    DWORD dwLineLength;

    if (!SetupFindFirstLineW(hSysSetupInf,
                            L"DeviceInfsToInstall",
                            NULL,
                            &InfContext))
    {
        return FALSE;
    }

    do
    {
        if (!SetupGetStringFieldW(&InfContext,
                                  0,
                                  szLineBuffer,
                                  ARRAYSIZE(szLineBuffer),
                                  &dwLineLength))
        {
            return FALSE;
        }

        if (!SetupDiInstallClassW(NULL, szLineBuffer, DI_QUIETINSTALL, NULL))
        {
            return FALSE;
        }
    }
    while (SetupFindNextLine(&InfContext, &InfContext));

    return TRUE;
}

static BOOL
InstallSysSetupInfComponents(VOID)
{
    INFCONTEXT InfContext;
    WCHAR szNameBuffer[256];
    WCHAR szSectionBuffer[256];
    HINF hComponentInf = INVALID_HANDLE_VALUE;

    if (!SetupFindFirstLineW(hSysSetupInf,
                             L"Infs.Always",
                             NULL,
                             &InfContext))
    {
        DPRINT("No Inf.Always section found\n");
    }
    else
    {
        do
        {
            if (!SetupGetStringFieldW(&InfContext,
                                      1, // Get the component name
                                      szNameBuffer,
                                      ARRAYSIZE(szNameBuffer),
                                      NULL))
            {
                FatalError("Error while trying to get component name\n");
                return FALSE;
            }

            if (!SetupGetStringFieldW(&InfContext,
                                      2, // Get the component install section
                                      szSectionBuffer,
                                      ARRAYSIZE(szSectionBuffer),
                                      NULL))
            {
                FatalError("Error while trying to get component install section\n");
                return FALSE;
            }

            DPRINT("Trying to execute install section '%S' from '%S'\n", szSectionBuffer, szNameBuffer);

            hComponentInf = SetupOpenInfFileW(szNameBuffer,
                                              NULL,
                                              INF_STYLE_WIN4,
                                              NULL);

            if (hComponentInf == INVALID_HANDLE_VALUE)
            {
                FatalError("SetupOpenInfFileW() failed to open '%S' (Error: %lu)\n", szNameBuffer, GetLastError());
                return FALSE;
            }

            if (!SetupInstallFromInfSectionW(NULL,
                                             hComponentInf,
                                             szSectionBuffer,
                                             SPINST_ALL,
                                             NULL,
                                             NULL,
                                             SP_COPY_NEWER,
                                             SetupDefaultQueueCallbackW,
                                             NULL,
                                             NULL,
                                             NULL))
           {
                FatalError("Error while trying to install : %S (Error: %lu)\n", szNameBuffer, GetLastError());
                SetupCloseInfFile(hComponentInf);
                return FALSE;
           }

           SetupCloseInfFile(hComponentInf);
        }
        while (SetupFindNextLine(&InfContext, &InfContext));
    }

    return TRUE;
}



BOOL
RegisterTypeLibraries(HINF hinf, LPCWSTR szSection)
{
    INFCONTEXT InfContext;
    BOOL res;
    WCHAR szName[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    INT csidl;
    LPWSTR p;
    HMODULE hmod;
    HRESULT hret;

    /* Begin iterating the entries in the inf section */
    res = SetupFindFirstLine(hinf, szSection, NULL, &InfContext);
    if (!res) return FALSE;

    do
    {
        /* Get the name of the current type library */
        if (!SetupGetStringFieldW(&InfContext, 1, szName, ARRAYSIZE(szName), NULL))
        {
            FatalError("SetupGetStringFieldW failed\n");
            continue;
        }

        if (!SetupGetIntField(&InfContext, 2, &csidl))
            csidl = CSIDL_SYSTEM;

        hret = SHGetFolderPathW(NULL, csidl, NULL, 0, szPath);
        if (FAILED(hret))
        {
            FatalError("SHGetFolderPathW failed hret=0x%lx\n", hret);
            continue;
        }

        p = PathAddBackslash(szPath);
        wcscpy(p, szName);

        hmod = LoadLibraryW(szPath);
        if (hmod == NULL)
        {
            FatalError("LoadLibraryW failed\n");
            continue;
        }

        __wine_register_resources(hmod);

    } while (SetupFindNextLine(&InfContext, &InfContext));

    return TRUE;
}

static BOOL
EnableUserModePnpManager(VOID)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS_PROCESS ServiceStatus;
    BOOL bRet = FALSE;
    DWORD BytesNeeded, WaitTime;

    hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (hSCManager == NULL)
    {
        DPRINT1("Unable to open the service control manager.\n");
        DPRINT1("Last Error %d\n", GetLastError());
        goto cleanup;
    }

    hService = OpenServiceW(hSCManager,
                            L"PlugPlay",
                            SERVICE_CHANGE_CONFIG | SERVICE_START | SERVICE_QUERY_STATUS);
    if (hService == NULL)
    {
        DPRINT1("Unable to open PlugPlay service\n");
        goto cleanup;
    }

    bRet = ChangeServiceConfigW(hService,
                                SERVICE_NO_CHANGE,
                                SERVICE_AUTO_START,
                                SERVICE_NO_CHANGE,
                                NULL, NULL, NULL,
                                NULL, NULL, NULL, NULL);
    if (!bRet)
    {
        DPRINT1("Unable to change the service configuration\n");
        goto cleanup;
    }

    bRet = StartServiceW(hService, 0, NULL);
    if (!bRet && (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING))
    {
        DPRINT1("Unable to start service\n");
        goto cleanup;
    }

    while (TRUE)
    {
        bRet = QueryServiceStatusEx(hService,
                                    SC_STATUS_PROCESS_INFO,
                                    (LPBYTE)&ServiceStatus,
                                    sizeof(ServiceStatus),
                                    &BytesNeeded);
        if (!bRet)
        {
            DPRINT1("QueryServiceStatusEx() failed for PlugPlay service (error 0x%x)\n", GetLastError());
            goto cleanup;
        }

        if (ServiceStatus.dwCurrentState != SERVICE_START_PENDING)
            break;

        WaitTime = ServiceStatus.dwWaitHint / 10;
        if (WaitTime < 1000) WaitTime = 1000;
        else if (WaitTime > 10000) WaitTime = 10000;
        Sleep(WaitTime);
    };

    if (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
    {
        bRet = FALSE;
        DPRINT1("Failed to start PlugPlay service\n");
        goto cleanup;
    }

    bRet = TRUE;

cleanup:
    if (hService != NULL)
        CloseServiceHandle(hService);
    if (hSCManager != NULL)
        CloseServiceHandle(hSCManager);
    return bRet;
}

static VOID
AdjustStatusMessageWindow(HWND hwndDlg, PDLG_DATA pDlgData)
{
    INT xOld, yOld, cxOld, cyOld;
    INT xNew, yNew, cxNew, cyNew;
    INT cxLabel, cyLabel, dyLabel;
    RECT rc, rcBar, rcLabel, rcWnd;
    BITMAP bmLogo, bmBar;
    DWORD style, exstyle;
    HWND hwndLogo = GetDlgItem(hwndDlg, IDC_ROSLOGO);
    HWND hwndBar = GetDlgItem(hwndDlg, IDC_BAR);
    HWND hwndLabel = GetDlgItem(hwndDlg, IDC_STATUSLABEL);

    /* This adjustment is for CJK only */
    switch (PRIMARYLANGID(GetUserDefaultLangID()))
    {
        case LANG_CHINESE:
        case LANG_JAPANESE:
        case LANG_KOREAN:
            break;

        default:
            return;
    }

    if (!GetObjectW(pDlgData->hLogoBitmap, sizeof(BITMAP), &bmLogo) ||
        !GetObjectW(pDlgData->hBarBitmap, sizeof(BITMAP), &bmBar))
    {
        return;
    }

    GetWindowRect(hwndBar, &rcBar);
    MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rcBar, 2);
    dyLabel = bmLogo.bmHeight - rcBar.top;

    GetWindowRect(hwndLabel, &rcLabel);
    MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rcLabel, 2);
    cxLabel = rcLabel.right - rcLabel.left;
    cyLabel = rcLabel.bottom - rcLabel.top;

    MoveWindow(hwndLogo, 0, 0, bmLogo.bmWidth, bmLogo.bmHeight, TRUE);
    MoveWindow(hwndBar, 0, bmLogo.bmHeight, bmLogo.bmWidth, bmBar.bmHeight, TRUE);
    MoveWindow(hwndLabel, rcLabel.left, rcLabel.top + dyLabel, cxLabel, cyLabel, TRUE);

    GetWindowRect(hwndDlg, &rcWnd);
    xOld = rcWnd.left;
    yOld = rcWnd.top;
    cxOld = rcWnd.right - rcWnd.left;
    cyOld = rcWnd.bottom - rcWnd.top;

    GetClientRect(hwndDlg, &rc);
    SetRect(&rc, 0, 0, bmLogo.bmWidth, rc.bottom - rc.top); /* new client size */

    style = (DWORD)GetWindowLongPtrW(hwndDlg, GWL_STYLE);
    exstyle = (DWORD)GetWindowLongPtrW(hwndDlg, GWL_EXSTYLE);
    AdjustWindowRectEx(&rc, style, FALSE, exstyle);

    cxNew = rc.right - rc.left;
    cyNew = (rc.bottom - rc.top) + dyLabel;
    xNew = xOld - (cxNew - cxOld) / 2;
    yNew = yOld - (cyNew - cyOld) / 2;
    MoveWindow(hwndDlg, xNew, yNew, cxNew, cyNew, TRUE);
}

static INT_PTR CALLBACK
StatusMessageWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PDLG_DATA pDlgData;
    UNREFERENCED_PARAMETER(wParam);

    pDlgData = (PDLG_DATA)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    /* pDlgData is required for each case except WM_INITDIALOG */
    if (uMsg != WM_INITDIALOG && pDlgData == NULL) return FALSE;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            BITMAP bm;
            WCHAR szMsg[256];

            /* Allocate pDlgData */
            pDlgData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pDlgData));
            if (pDlgData)
            {
                /* Set pDlgData to GWLP_USERDATA, so we can get it for new messages */
                SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)pDlgData);

                /* Load bitmaps */
                pDlgData->hLogoBitmap = LoadImageW(hDllInstance,
                                                    MAKEINTRESOURCEW(IDB_REACTOS), IMAGE_BITMAP,
                                                    0, 0, LR_DEFAULTCOLOR);

                pDlgData->hBarBitmap = LoadImageW(hDllInstance, MAKEINTRESOURCEW(IDB_LINE),
                                                IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
                GetObject(pDlgData->hBarBitmap, sizeof(bm), &bm);
                pDlgData->BarWidth = bm.bmWidth;
                pDlgData->BarHeight = bm.bmHeight;

                if (pDlgData->hLogoBitmap && pDlgData->hBarBitmap)
                {
                    if (SetTimer(hwndDlg, IDT_BAR, 20, NULL) == 0)
                    {
                        DPRINT1("SetTimer(IDT_BAR) failed: %lu\n", GetLastError());
                    }

                    /* Get the animation bar control */
                    pDlgData->hWndBarCtrl = GetDlgItem(hwndDlg, IDC_BAR);
                }
            }

            /* Get and set status text */
            if (!LoadStringW(hDllInstance, IDS_STATUS_INSTALL_DEV, szMsg, ARRAYSIZE(szMsg)))
                return FALSE;
            SetDlgItemTextW(hwndDlg, IDC_STATUSLABEL, szMsg);

            AdjustStatusMessageWindow(hwndDlg, pDlgData);
            return TRUE;
        }

        case WM_TIMER:
        {
            if (pDlgData->hBarBitmap)
            {
                /*
                 * Default rotation bar image width is 413 (same as logo)
                 * We can divide 413 by 7 without remainder
                 */
                pDlgData->BarCounter = (pDlgData->BarCounter + 7) % pDlgData->BarWidth;
                InvalidateRect(pDlgData->hWndBarCtrl, NULL, FALSE);
                UpdateWindow(pDlgData->hWndBarCtrl);
            }
            return TRUE;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDis = (LPDRAWITEMSTRUCT)lParam;

            if (lpDis->CtlID != IDC_BAR)
            {
                return FALSE;
            }

            if (pDlgData->hBarBitmap)
            {
                HDC hdcMem;
                HGDIOBJ hOld;
                DWORD off = pDlgData->BarCounter;
                DWORD iw = pDlgData->BarWidth;
                DWORD ih = pDlgData->BarHeight;

                hdcMem = CreateCompatibleDC(lpDis->hDC);
                hOld = SelectObject(hdcMem, pDlgData->hBarBitmap);
                BitBlt(lpDis->hDC, off, 0, iw - off, ih, hdcMem, 0, 0, SRCCOPY);
                BitBlt(lpDis->hDC, 0, 0, off, ih, hdcMem, iw - off, 0, SRCCOPY);
                SelectObject(hdcMem, hOld);
                DeleteDC(hdcMem);
                return TRUE;
            }
            return FALSE;
        }

        case WM_DESTROY:
        {
            if (pDlgData->hBarBitmap)
            {
                KillTimer(hwndDlg, IDT_BAR);
            }

            DeleteObject(pDlgData->hLogoBitmap);
            DeleteObject(pDlgData->hBarBitmap);
            HeapFree(GetProcessHeap(), 0, pDlgData);
            return TRUE;
        }
    }
    return FALSE;
}

static DWORD WINAPI
ShowStatusMessageThread(
    IN LPVOID lpParameter)
{
    HWND hWnd;
    MSG Msg;
    UNREFERENCED_PARAMETER(lpParameter);

    hWnd = CreateDialogParam(hDllInstance,
                             MAKEINTRESOURCE(IDD_STATUSWINDOW_DLG),
                             GetDesktopWindow(),
                             StatusMessageWindowProc,
                             (LPARAM)NULL);
    if (!hWnd)
        return 0;

    ShowWindow(hWnd, SW_SHOW);

    /* Message loop for the Status window */
    while (GetMessage(&Msg, NULL, 0, 0))
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    EndDialog(hWnd, 0);

    return 0;
}

static LONG
ReadRegSzKey(
    IN HKEY hKey,
    IN LPCWSTR pszKey,
    OUT LPWSTR* pValue)
{
    LONG rc;
    DWORD dwType;
    DWORD cbData = 0;
    LPWSTR pwszValue;

    if (!pValue)
        return ERROR_INVALID_PARAMETER;

    *pValue = NULL;
    rc = RegQueryValueExW(hKey, pszKey, NULL, &dwType, NULL, &cbData);
    if (rc != ERROR_SUCCESS)
        return rc;
    if (dwType != REG_SZ)
        return ERROR_FILE_NOT_FOUND;
    pwszValue = HeapAlloc(GetProcessHeap(), 0, cbData + sizeof(WCHAR));
    if (!pwszValue)
        return ERROR_NOT_ENOUGH_MEMORY;
    rc = RegQueryValueExW(hKey, pszKey, NULL, NULL, (LPBYTE)pwszValue, &cbData);
    if (rc != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, pwszValue);
        return rc;
    }
    /* NULL-terminate the string */
    pwszValue[cbData / sizeof(WCHAR)] = '\0';

    *pValue = pwszValue;
    return ERROR_SUCCESS;
}

static BOOL
IsConsoleBoot(VOID)
{
    HKEY hControlKey = NULL;
    LPWSTR pwszSystemStartOptions = NULL;
    LPWSTR pwszCurrentOption, pwszNextOption; /* Pointers into SystemStartOptions */
    BOOL bConsoleBoot = FALSE;
    LONG rc;

    rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                       L"SYSTEM\\CurrentControlSet\\Control",
                       0,
                       KEY_QUERY_VALUE,
                       &hControlKey);
    if (rc != ERROR_SUCCESS)
        goto cleanup;

    rc = ReadRegSzKey(hControlKey, L"SystemStartOptions", &pwszSystemStartOptions);
    if (rc != ERROR_SUCCESS)
        goto cleanup;

    /* Check for CONSOLE switch in SystemStartOptions */
    pwszCurrentOption = pwszSystemStartOptions;
    while (pwszCurrentOption)
    {
        pwszNextOption = wcschr(pwszCurrentOption, L' ');
        if (pwszNextOption)
            *pwszNextOption = L'\0';
        if (wcsicmp(pwszCurrentOption, L"CONSOLE") == 0)
        {
            DPRINT("Found %S. Switching to console boot\n", pwszCurrentOption);
            bConsoleBoot = TRUE;
            goto cleanup;
        }
        pwszCurrentOption = pwszNextOption ? pwszNextOption + 1 : NULL;
    }

cleanup:
    if (hControlKey != NULL)
        RegCloseKey(hControlKey);
    if (pwszSystemStartOptions)
        HeapFree(GetProcessHeap(), 0, pwszSystemStartOptions);
    return bConsoleBoot;
}

static BOOL
CommonInstall(VOID)
{
    HANDLE hThread = NULL;
    BOOL bResult = FALSE;

    hSysSetupInf = SetupOpenInfFileW(L"syssetup.inf",
                                     NULL,
                                     INF_STYLE_WIN4,
                                     NULL);
    if (hSysSetupInf == INVALID_HANDLE_VALUE)
    {
        FatalError("SetupOpenInfFileW() failed to open 'syssetup.inf' (Error: %lu)\n", GetLastError());
        return FALSE;
    }

    if (!InstallSysSetupInfDevices())
    {
        FatalError("InstallSysSetupInfDevices() failed!\n");
        goto Exit;
    }

    if(!InstallSysSetupInfComponents())
    {
        FatalError("InstallSysSetupInfComponents() failed!\n");
        goto Exit;
    }

    if (!IsConsoleBoot())
    {
        hThread = CreateThread(NULL,
                               0,
                               ShowStatusMessageThread,
                               NULL,
                               0,
                               NULL);
    }

    if (!EnableUserModePnpManager())
    {
        FatalError("EnableUserModePnpManager() failed!\n");
        goto Exit;
    }

    if (CMP_WaitNoPendingInstallEvents(INFINITE) != WAIT_OBJECT_0)
    {
        FatalError("CMP_WaitNoPendingInstallEvents() failed!\n");
        goto Exit;
    }

    bResult = TRUE;

Exit:

    if (bResult == FALSE)
    {
        SetupCloseInfFile(hSysSetupInf);
    }

    if (hThread != NULL)
    {
        PostThreadMessage(GetThreadId(hThread), WM_QUIT, 0, 0);
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    return bResult;
}

static
DWORD
InstallLiveCD(VOID)
{
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL bRes;

    if (!CommonInstall())
        goto error;

    /* Install the TCP/IP protocol driver */
    bRes = InstallNetworkComponent(L"MS_TCPIP");
    if (!bRes && GetLastError() != ERROR_FILE_NOT_FOUND)
    {
        DPRINT("InstallNetworkComponent() failed with error 0x%lx\n", GetLastError());
    }
    else
    {
        /* Start the TCP/IP protocol driver */
        SetupStartService(L"Tcpip", FALSE);
        SetupStartService(L"Dhcp", FALSE);
        SetupStartService(L"Dnscache", FALSE);
    }

    /* Register components */
    _SEH2_TRY
    {
        if (!SetupInstallFromInfSectionW(NULL,
                                         hSysSetupInf, L"RegistrationPhase2",
                                         SPINST_ALL,
                                         0, NULL, 0, NULL, NULL, NULL, NULL))
        {
            DPRINT1("SetupInstallFromInfSectionW failed!\n");
        }

        RegisterTypeLibraries(hSysSetupInf, L"TypeLibraries");
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("Catching exception\n");
    }
    _SEH2_END;

    SetupCloseInfFile(hSysSetupInf);

    /* Run the shell */
    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    bRes = CreateProcessW(L"userinit.exe",
                          NULL,
                          NULL,
                          NULL,
                          FALSE,
                          0,
                          NULL,
                          NULL,
                          &StartupInfo,
                          &ProcessInformation);
    if (!bRes)
        goto error;

    CloseHandle(ProcessInformation.hThread);
    CloseHandle(ProcessInformation.hProcess);

    return 0;

error:
    MessageBoxW(
        NULL,
        L"Failed to load LiveCD! You can shutdown your computer, or press ENTER to reboot.",
        L"ReactOS LiveCD",
        MB_OK);
    return 0;
}


static BOOL
SetSetupType(DWORD dwSetupType)
{
    DWORD dwError;
    HKEY hKey;

    dwError = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\Setup",
        0,
        KEY_SET_VALUE,
        &hKey);
    if (dwError != ERROR_SUCCESS)
        return FALSE;

    dwError = RegSetValueExW(
        hKey,
        L"SetupType",
        0,
        REG_DWORD,
        (LPBYTE)&dwSetupType,
        sizeof(DWORD));
    RegCloseKey(hKey);
    if (dwError != ERROR_SUCCESS)
        return FALSE;

    return TRUE;
}

static DWORD CALLBACK
HotkeyThread(LPVOID Parameter)
{
    ATOM hotkey;
    MSG msg;

    DPRINT("HotkeyThread start\n");

    hotkey = GlobalAddAtomW(L"Setup Shift+F10 Hotkey");

    if (!RegisterHotKey(NULL, hotkey, MOD_SHIFT, VK_F10))
        DPRINT1("RegisterHotKey failed with %lu\n", GetLastError());

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.hwnd == NULL && msg.message == WM_HOTKEY && msg.wParam == hotkey)
        {
            STARTUPINFOW si = { sizeof(si) };
            PROCESS_INFORMATION pi;

            if (CreateProcessW(L"cmd.exe",
                               NULL,
                               NULL,
                               NULL,
                               FALSE,
                               CREATE_NEW_CONSOLE,
                               NULL,
                               NULL,
                               &si,
                               &pi))
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            else
            {
                DPRINT1("Failed to launch command prompt: %lu\n", GetLastError());
            }
        }
    }

    UnregisterHotKey(NULL, hotkey);
    GlobalDeleteAtom(hotkey);

    DPRINT("HotkeyThread terminate\n");
    return 0;
}


static
BOOL
InitializeProgramFilesDir(VOID)
{
    LONG Error;
    HKEY hKey;
    DWORD dwLength;
    WCHAR szProgramFilesDirPath[MAX_PATH];
    WCHAR szCommonFilesDirPath[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];

    /* Load 'Program Files' location */
    if (!LoadStringW(hDllInstance,
                     IDS_PROGRAMFILES,
                     szBuffer,
                     ARRAYSIZE(szBuffer)))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        return FALSE;
    }

    if (!LoadStringW(hDllInstance,
                     IDS_COMMONFILES,
                     szCommonFilesDirPath,
                     ARRAYSIZE(szCommonFilesDirPath)))
    {
        DPRINT1("Warning: %lu\n", GetLastError());
    }

    /* Expand it */
    if (!ExpandEnvironmentStringsW(szBuffer,
                                   szProgramFilesDirPath,
                                   ARRAYSIZE(szProgramFilesDirPath)))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        return FALSE;
    }

    wcscpy(szBuffer, szProgramFilesDirPath);
    wcscat(szBuffer, L"\\");
    wcscat(szBuffer, szCommonFilesDirPath);

    if (!ExpandEnvironmentStringsW(szBuffer,
                                   szCommonFilesDirPath,
                                   ARRAYSIZE(szCommonFilesDirPath)))
    {
        DPRINT1("Warning: %lu\n", GetLastError());
    }

    /* Store it */
    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion",
                          0,
                          KEY_SET_VALUE,
                          &hKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        return FALSE;
    }

    dwLength = (wcslen(szProgramFilesDirPath) + 1) * sizeof(WCHAR);
    Error = RegSetValueExW(hKey,
                           L"ProgramFilesDir",
                           0,
                           REG_SZ,
                           (LPBYTE)szProgramFilesDirPath,
                           dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        return FALSE;
    }

    dwLength = (wcslen(szCommonFilesDirPath) + 1) * sizeof(WCHAR);
    Error = RegSetValueExW(hKey,
                           L"CommonFilesDir",
                           0,
                           REG_SZ,
                           (LPBYTE)szCommonFilesDirPath,
                           dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Warning: %lu\n", Error);
    }

    RegCloseKey(hKey);

    /* Create directory */
    // FIXME: Security!
    if (!CreateDirectoryW(szProgramFilesDirPath, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            DPRINT1("Error: %lu\n", GetLastError());
            return FALSE;
        }
    }

    /* Create directory */
    // FIXME: Security!
    if (!CreateDirectoryW(szCommonFilesDirPath, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            DPRINT1("Warning: %lu\n", GetLastError());
            // return FALSE;
        }
    }

    return TRUE;
}


static
VOID
InitializeDefaultUserLocale(VOID)
{
    WCHAR szBuffer[80];
    PWSTR ptr;
    HKEY hLocaleKey;
    DWORD ret;
    DWORD dwSize;
    LCID lcid;
    INT i;

    struct {LCTYPE LCType; PWSTR pValue;} LocaleData[] = {
        /* Number */
        {LOCALE_SDECIMAL, L"sDecimal"},
        {LOCALE_STHOUSAND, L"sThousand"},
        {LOCALE_SNEGATIVESIGN, L"sNegativeSign"},
        {LOCALE_SPOSITIVESIGN, L"sPositiveSign"},
        {LOCALE_SGROUPING, L"sGrouping"},
        {LOCALE_SLIST, L"sList"},
        {LOCALE_SNATIVEDIGITS, L"sNativeDigits"},
        {LOCALE_INEGNUMBER, L"iNegNumber"},
        {LOCALE_IDIGITS, L"iDigits"},
        {LOCALE_ILZERO, L"iLZero"},
        {LOCALE_IMEASURE, L"iMeasure"},
        {LOCALE_IDIGITSUBSTITUTION, L"NumShape"},

        /* Currency */
        {LOCALE_SCURRENCY, L"sCurrency"},
        {LOCALE_SMONDECIMALSEP, L"sMonDecimalSep"},
        {LOCALE_SMONTHOUSANDSEP, L"sMonThousandSep"},
        {LOCALE_SMONGROUPING, L"sMonGrouping"},
        {LOCALE_ICURRENCY, L"iCurrency"},
        {LOCALE_INEGCURR, L"iNegCurr"},
        {LOCALE_ICURRDIGITS, L"iCurrDigits"},

        /* Time */
        {LOCALE_STIMEFORMAT, L"sTimeFormat"},
        {LOCALE_STIME, L"sTime"},
        {LOCALE_S1159, L"s1159"},
        {LOCALE_S2359, L"s2359"},
        {LOCALE_ITIME, L"iTime"},
        {LOCALE_ITIMEMARKPOSN, L"iTimePrefix"},
        {LOCALE_ITLZERO, L"iTLZero"},

        /* Date */
        {LOCALE_SLONGDATE, L"sLongDate"},
        {LOCALE_SSHORTDATE, L"sShortDate"},
        {LOCALE_SDATE, L"sDate"},
        {LOCALE_IFIRSTDAYOFWEEK, L"iFirstDayOfWeek"},
        {LOCALE_IFIRSTWEEKOFYEAR, L"iFirstWeekOfYear"},
        {LOCALE_IDATE, L"iDate"},
        {LOCALE_ICALENDARTYPE, L"iCalendarType"},

        /* Misc */
        {LOCALE_SCOUNTRY, L"sCountry"},
        {LOCALE_SABBREVLANGNAME, L"sLanguage"},
        {LOCALE_ICOUNTRY, L"iCountry"},
        {0, NULL}};

    ret = RegOpenKeyExW(HKEY_USERS,
                        L".DEFAULT\\Control Panel\\International",
                        0,
                        KEY_READ | KEY_WRITE,
                        &hLocaleKey);
    if (ret != ERROR_SUCCESS)
    {
        return;
    }

    dwSize = 9 * sizeof(WCHAR);
    ret = RegQueryValueExW(hLocaleKey,
                           L"Locale",
                           NULL,
                           NULL,
                           (PBYTE)szBuffer,
                           &dwSize);
    if (ret != ERROR_SUCCESS)
        goto done;

    lcid = (LCID)wcstoul(szBuffer, &ptr, 16);
    if (lcid == 0)
        goto done;

    i = 0;
    while (LocaleData[i].pValue != NULL)
    {
        if (GetLocaleInfoW(lcid,
                           LocaleData[i].LCType | LOCALE_NOUSEROVERRIDE,
                           szBuffer,
                           ARRAYSIZE(szBuffer)))
        {
            RegSetValueExW(hLocaleKey,
                           LocaleData[i].pValue,
                           0,
                           REG_SZ,
                           (PBYTE)szBuffer,
                           (wcslen(szBuffer) + 1) * sizeof(WCHAR));
        }

        i++;
    }

done:
    RegCloseKey(hLocaleKey);
}


static
DWORD
SaveDefaultUserHive(VOID)
{
    WCHAR szDefaultUserHive[MAX_PATH];
    HKEY hUserKey = NULL;
    DWORD cchSize;
    DWORD dwError;

    DPRINT("SaveDefaultUserHive()\n");

    cchSize = ARRAYSIZE(szDefaultUserHive);
    GetDefaultUserProfileDirectoryW(szDefaultUserHive, &cchSize);

    wcscat(szDefaultUserHive, L"\\ntuser.dat");

    dwError = RegOpenKeyExW(HKEY_USERS,
                            L".DEFAULT",
                            0,
                            KEY_READ,
                            &hUserKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("RegOpenKeyExW() failed (Error %lu)\n", dwError);
        return dwError;
    }

    pSetupEnablePrivilege(L"SeBackupPrivilege", TRUE);

    /* Save the Default hive */
    dwError = RegSaveKeyExW(hUserKey,
                            szDefaultUserHive,
                            NULL,
                            REG_STANDARD_FORMAT);
    if (dwError == ERROR_ALREADY_EXISTS)
    {
        WCHAR szBackupHive[MAX_PATH];

        /* Build the backup hive file name by replacing the extension */
        wcscpy(szBackupHive, szDefaultUserHive);
        wcscpy(&szBackupHive[wcslen(szBackupHive) - 4], L".bak");

        /* Back up the existing default user hive by renaming it, replacing any possible existing old backup */
        if (!MoveFileExW(szDefaultUserHive,
                         szBackupHive,
                         MOVEFILE_REPLACE_EXISTING))
        {
            dwError = GetLastError();
            DPRINT1("Failed to create a default-user hive backup '%S', MoveFileExW failed (Error %lu)\n",
                    szBackupHive, dwError);
        }
        else
        {
            /* The backup has been done, retry saving the Default hive */
            dwError = RegSaveKeyExW(hUserKey,
                                    szDefaultUserHive,
                                    NULL,
                                    REG_STANDARD_FORMAT);
        }
    }
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("RegSaveKeyExW() failed (Error %lu)\n", dwError);
    }

    pSetupEnablePrivilege(L"SeBackupPrivilege", FALSE);

    RegCloseKey(hUserKey);

    return dwError;
}


static
DWORD
InstallReactOS(VOID)
{
    WCHAR szBuffer[MAX_PATH];
    HANDLE token;
    TOKEN_PRIVILEGES privs;
    HKEY hKey;
    HINF hShortcutsInf;
    HANDLE hHotkeyThread;
    BOOL ret;

    InitializeSetupActionLog(FALSE);
    LogItem(NULL, L"Installing ReactOS");

    CreateTempDir(L"TEMP");
    CreateTempDir(L"TMP");

    if (!InitializeProgramFilesDir())
    {
        FatalError("InitializeProgramFilesDir() failed");
        return 0;
    }

    if (!InitializeProfiles())
    {
        FatalError("InitializeProfiles() failed");
        return 0;
    }

    InitializeDefaultUserLocale();

    if (GetWindowsDirectoryW(szBuffer, ARRAYSIZE(szBuffer)))
    {
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                          0,
                          KEY_WRITE,
                          &hKey) == ERROR_SUCCESS)
        {
            RegSetValueExW(hKey,
                           L"PathName",
                           0,
                           REG_SZ,
                           (LPBYTE)szBuffer,
                           (wcslen(szBuffer) + 1) * sizeof(WCHAR));

            RegSetValueExW(hKey,
                           L"SystemRoot",
                           0,
                           REG_SZ,
                           (LPBYTE)szBuffer,
                           (wcslen(szBuffer) + 1) * sizeof(WCHAR));

            RegCloseKey(hKey);
        }

        PathAddBackslash(szBuffer);
        wcscat(szBuffer, L"system");
        CreateDirectory(szBuffer, NULL);
    }

    if (SaveDefaultUserHive() != ERROR_SUCCESS)
    {
        FatalError("SaveDefaultUserHive() failed");
        return 0;
    }

    if (!CopySystemProfile(0))
    {
        FatalError("CopySystemProfile() failed");
        return 0;
    }

    hHotkeyThread = CreateThread(NULL, 0, HotkeyThread, NULL, 0, NULL);

    if (!CommonInstall())
        return 0;

    /* Install the TCP/IP protocol driver */
    ret = InstallNetworkComponent(L"MS_TCPIP");
    if (!ret && GetLastError() != ERROR_FILE_NOT_FOUND)
    {
        DPRINT("InstallNetworkComponent() failed with error 0x%lx\n", GetLastError());
    }
    else
    {
        /* Start the TCP/IP protocol driver */
        SetupStartService(L"Tcpip", FALSE);
        SetupStartService(L"Dhcp", FALSE);
        SetupStartService(L"Dnscache", FALSE);
    }

    InstallWizard();

    InstallSecurity();

    SetAutoAdminLogon();

    hShortcutsInf = SetupOpenInfFileW(L"shortcuts.inf",
                                      NULL,
                                      INF_STYLE_WIN4,
                                      NULL);
    if (hShortcutsInf == INVALID_HANDLE_VALUE)
    {
        FatalError("Failed to open shortcuts.inf");
        return 0;
    }

    if (!CreateShortcuts(hShortcutsInf, L"ShortcutFolders"))
    {
        FatalError("CreateShortcuts() failed");
        return 0;
    }

    SetupCloseInfFile(hShortcutsInf);

    hShortcutsInf = SetupOpenInfFileW(L"rosapps_shortcuts.inf",
                                       NULL,
                                       INF_STYLE_WIN4,
                                       NULL);
    if (hShortcutsInf != INVALID_HANDLE_VALUE)
    {
        if (!CreateShortcuts(hShortcutsInf, L"ShortcutFolders"))
        {
            FatalError("CreateShortcuts(rosapps) failed");
            return 0;
        }
        SetupCloseInfFile(hShortcutsInf);
    }

    SetupCloseInfFile(hSysSetupInf);
    SetSetupType(0);

    if (hHotkeyThread)
    {
        PostThreadMessage(GetThreadId(hHotkeyThread), WM_QUIT, 0, 0);
        CloseHandle(hHotkeyThread);
    }

    LogItem(NULL, L"Installing ReactOS done");
    TerminateSetupActionLog();

    if (AdminInfo.Name != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AdminInfo.Name);

    if (AdminInfo.Domain != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AdminInfo.Domain);

    if (AdminInfo.Password != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AdminInfo.Password);

    /* Get shutdown privilege */
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token))
    {
        FatalError("OpenProcessToken() failed!");
        return 0;
    }
    if (!LookupPrivilegeValue(NULL,
                              SE_SHUTDOWN_NAME,
                              &privs.Privileges[0].Luid))
    {
        FatalError("LookupPrivilegeValue() failed!");
        return 0;
    }
    privs.PrivilegeCount = 1;
    privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (AdjustTokenPrivileges(token,
                              FALSE,
                              &privs,
                              0,
                              (PTOKEN_PRIVILEGES)NULL,
                              NULL) == 0)
    {
        FatalError("AdjustTokenPrivileges() failed!");
        return 0;
    }

    ExitWindowsEx(EWX_REBOOT, 0);
    return 0;
}


/*
 * Standard Windows-compatible export, which dispatches
 * to either 'InstallReactOS' or 'InstallLiveCD'.
 */
INT
WINAPI
InstallWindowsNt(INT argc, WCHAR** argv)
{
    INT i;
    PWSTR p;

    for (i = 0; i < argc; ++i)
    {
        p = argv[i];
        if (*p == L'-')
        {
            p++;

            // NOTE: On Windows, "mini" means "minimal UI", and can be used
            // in addition to "newsetup"; these options are not exclusive.
            if (_wcsicmp(p, L"newsetup") == 0)
                return (INT)InstallReactOS();
            else if (_wcsicmp(p, L"mini") == 0)
                return (INT)InstallLiveCD();

            /* Add support for other switches */
        }
    }

    return 0;
}


/*
 * @unimplemented
 */
DWORD WINAPI
SetupChangeFontSize(
    IN HANDLE hWnd,
    IN LPCWSTR lpszFontSize)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
DWORD WINAPI
SetupChangeLocaleEx(HWND hWnd,
                    LCID Lcid,
                    LPCWSTR lpSrcRootPath,
                    char Unknown,
                    DWORD dwUnused1,
                    DWORD dwUnused2)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @implemented
 */
DWORD WINAPI
SetupChangeLocale(HWND hWnd, LCID Lcid)
{
    return SetupChangeLocaleEx(hWnd, Lcid, NULL, 0, 0, 0);
}


DWORD
WINAPI
SetupStartService(
    LPCWSTR lpServiceName,
    BOOL bWait)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    DWORD dwError = ERROR_SUCCESS;

    hManager = OpenSCManagerW(NULL,
                              NULL,
                              SC_MANAGER_ALL_ACCESS);
    if (hManager == NULL)
    {
        dwError = GetLastError();
        goto done;
    }

    hService = OpenServiceW(hManager,
                            lpServiceName,
                            SERVICE_START);
    if (hService == NULL)
    {
        dwError = GetLastError();
        goto done;
    }

    if (!StartService(hService, 0, NULL))
    {
        dwError = GetLastError();
        goto done;
    }

done:
    if (hService != NULL)
        CloseServiceHandle(hService);

    if (hManager != NULL)
        CloseServiceHandle(hManager);

    return dwError;
}
