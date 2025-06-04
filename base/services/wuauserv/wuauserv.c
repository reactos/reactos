/*
* PROJECT:          ReactOS Update Service
* LICENSE:          GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
* PURPOSE:          Automatic Updates service stub.
*                   Stub is required for some application at the installation phase.
* COPYRIGHT:        Copyright 2018 Denis Malikov (filedem@gmail.com)
*/

#include "wuauserv.h"
#include <wininet.h>

// Define a placeholder URL for the manifest file
// In a real implementation, this would point to an actual manifest server.
// Using ReactOS README.md as a readily available text file for testing download.
static const WCHAR DUMMY_MANIFEST_URL[] = L"https://raw.githubusercontent.com/reactos/reactos/master/README.md";

// Define the local path for storing the downloaded manifest
// TODO: Determine the correct system temporary path dynamically in a real implementation.
static const WCHAR LOCAL_MANIFEST_PATH[] = L"C:\\Temp\\wuauserv_manifest.xml";

/* GLOBALS ******************************************************************/

static WCHAR ServiceName[] = L"wuauserv";

static SERVICE_STATUS_HANDLE ServiceStatusHandle;
static SERVICE_STATUS ServiceStatus;

static HANDLE hStopEvent = NULL;
static HANDLE hTimer = NULL;

/* FUNCTIONS *****************************************************************/

static BOOL DownloadManifestFile(LPCWSTR szUrl, LPCWSTR szLocalPath)
{
    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    BOOL bResult = FALSE;
    BYTE pBuffer[4096];
    DWORD dwBytesRead = 0;
    DWORD dwBytesWritten = 0;

    DPRINT1("WU Downloading manifest from %S to %S\n", szUrl, szLocalPath);

    hInternet = InternetOpenW(L"ReactOS WUAUSERV/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet == NULL)
    {
        DPRINT1("WU InternetOpenW failed (Error %lu)\n", GetLastError());
        goto cleanup;
    }

    hConnect = InternetOpenUrlW(hInternet, szUrl, NULL, 0,
                                INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
    if (hConnect == NULL)
    {
        DPRINT1("WU InternetOpenUrlW failed for %S (Error %lu)\n", szUrl, GetLastError());
        goto cleanup;
    }

    hFile = CreateFileW(szLocalPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DPRINT1("WU CreateFileW failed for %S (Error %lu)\n", szLocalPath, GetLastError());
        goto cleanup;
    }

    while (TRUE)
    {
        if (!InternetReadFile(hConnect, pBuffer, sizeof(pBuffer), &dwBytesRead))
        {
            DPRINT1("WU InternetReadFile failed (Error %lu)\n", GetLastError());
            goto cleanup_and_delete_file; // Error during read
        }

        if (dwBytesRead == 0)
        {
            bResult = TRUE; // End of file successfully reached
            DPRINT1("WU Finished downloading manifest file.\n");
            break;
        }

        if (!WriteFile(hFile, pBuffer, dwBytesRead, &dwBytesWritten, NULL) || dwBytesRead != dwBytesWritten)
        {
            DPRINT1("WU WriteFile failed for %S (Error %lu)\n", szLocalPath, GetLastError());
            goto cleanup_and_delete_file; // Error during write
        }
    }

cleanup_and_delete_file:
    if (!bResult && hFile != INVALID_HANDLE_VALUE)
    {
        // Close it first before trying to delete
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE; // Mark as closed
        DPRINT1("WU Download failed, deleting partially downloaded file %S\n", szLocalPath);
        DeleteFileW(szLocalPath);
    }

cleanup:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (hConnect)
        InternetCloseHandle(hConnect);
    if (hInternet)
        InternetCloseHandle(hInternet);

    DPRINT1("WU DownloadManifestFile returning %s\n", bResult ? "TRUE" : "FALSE");
    return bResult;
}

static VOID
UpdateServiceStatus(DWORD dwState)
{
    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwCurrentState = dwState;
    ServiceStatus.dwControlsAccepted = 0;
    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;

    if (dwState == SERVICE_START_PENDING ||
        dwState == SERVICE_STOP_PENDING ||
        dwState == SERVICE_PAUSE_PENDING ||
        dwState == SERVICE_CONTINUE_PENDING)
        ServiceStatus.dwWaitHint = 1000;
    else
        ServiceStatus.dwWaitHint = 0;

    if (dwState == SERVICE_RUNNING)
        ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

    SetServiceStatus(ServiceStatusHandle, &ServiceStatus);
    DPRINT1("WU UpdateServiceStatus(%lu) called\n", dwState);
}

static DWORD WINAPI
ServiceControlHandler(DWORD dwControl,
                      DWORD dwEventType,
                      LPVOID lpEventData,
                      LPVOID lpContext)
{
    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
            DPRINT1("WU ServiceControlHandler(SERVICE_CONTROL_STOP) received\n");
            SetEvent(hStopEvent);
            UpdateServiceStatus(SERVICE_STOP_PENDING);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_PAUSE:
            DPRINT1("WU ServiceControlHandler(SERVICE_CONTROL_PAUSE) received\n");
            UpdateServiceStatus(SERVICE_PAUSED);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_CONTINUE:
            DPRINT1("WU ServiceControlHandler(SERVICE_CONTROL_CONTINUE) received\n");
            UpdateServiceStatus(SERVICE_RUNNING);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_INTERROGATE:
            DPRINT1("WU ServiceControlHandler(SERVICE_CONTROL_INTERROGATE) received\n");
            SetServiceStatus(ServiceStatusHandle, &ServiceStatus);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_SHUTDOWN:
            DPRINT1("WU ServiceControlHandler(SERVICE_CONTROL_SHUTDOWN) received\n");
            SetEvent(hStopEvent);
            UpdateServiceStatus(SERVICE_STOP_PENDING);
            return ERROR_SUCCESS;

        default :
            DPRINT1("WU ServiceControlHandler(Control %lu) received\n", dwControl);
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

VOID WINAPI
ServiceMain(DWORD argc, LPTSTR *argv)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    DPRINT("WU ServiceMain() called\n");

    ServiceStatusHandle = RegisterServiceCtrlHandlerExW(ServiceName,
                                                        ServiceControlHandler,
                                                        NULL);
    if (!ServiceStatusHandle)
    {
        DPRINT1("RegisterServiceCtrlHandlerExW() failed (Error %lu)\n", GetLastError());
        return;
    }

    hStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (hStopEvent == NULL)
    {
        DPRINT1("CreateEvent() failed (Error %lu)\n", GetLastError());
        goto done;
    }

    UpdateServiceStatus(SERVICE_RUNNING);

    hTimer = CreateWaitableTimerW(NULL, FALSE, L"WuauservTimer"); // Auto-reset timer
    if (hTimer == NULL)
    {
        DPRINT1("CreateWaitableTimerW() failed (Error %lu)\n", GetLastError());
        CloseHandle(hStopEvent);
        hStopEvent = NULL;
        goto done;
    }

    LARGE_INTEGER liDueTime;
    LONG lPeriod = 1 * 60 * 1000; // 1 minute in milliseconds
    liDueTime.QuadPart = -10LL * 1000000LL; // Fire 10 seconds after service start (relative)

    if (!SetWaitableTimer(hTimer, &liDueTime, lPeriod, NULL, NULL, 0))
    {
        DPRINT1("SetWaitableTimer() failed (Error %lu)\n", GetLastError());
        CloseHandle(hTimer);
        hTimer = NULL;
        CloseHandle(hStopEvent);
        hStopEvent = NULL;
        goto done;
    }

    DPRINT1("WU Service waiting for stop or timer event.\n");

    HANDLE hEvents[2];
    hEvents[0] = hStopEvent;
    hEvents[1] = hTimer;

    DWORD dwWaitResult;
    BOOL bRunning = TRUE;
    while (bRunning)
    {
        dwWaitResult = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
        switch (dwWaitResult)
        {
            case WAIT_OBJECT_0 + 0: // hStopEvent
                DPRINT1("WU Stop event signaled.\n");
                bRunning = FALSE;
                break;

            case WAIT_OBJECT_0 + 1: // hTimer
                DPRINT1("WU Timer event signaled. Attempting to download manifest...\n");
                if (DownloadManifestFile(DUMMY_MANIFEST_URL, LOCAL_MANIFEST_PATH))
                {
                    DPRINT1("WU Manifest downloaded successfully to %S\n", LOCAL_MANIFEST_PATH);

                    HANDLE hManifestFile = CreateFileW(LOCAL_MANIFEST_PATH, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hManifestFile != INVALID_HANDLE_VALUE)
                    {
                        LARGE_INTEGER liFileSize;
                        if (GetFileSizeEx(hManifestFile, &liFileSize))
                        {
                            DPRINT1("WU Downloaded manifest file size: %I64d bytes.\n", liFileSize.QuadPart);
                        }
                        else
                        {
                            DPRINT1("WU Could not get size of downloaded manifest file (Error %lu).\n", GetLastError());
                        }
                        CloseHandle(hManifestFile);
                    }
                    else
                    {
                        DPRINT1("WU Could not open downloaded manifest file for reading size (Error %lu).\n", GetLastError());
                    }
                    // Further processing will be added in the next step.
                }
                else
                {
                    DPRINT1("WU Failed to download manifest from %S\n", DUMMY_MANIFEST_URL);
                }
                break;

            default: // Error or abandon
                DPRINT1("WU WaitForMultipleObjects failed or unexpected result (Error %lu)\n", GetLastError());
                bRunning = FALSE;
                break;
        }
    }

done:
    if (hTimer)
    {
        CancelWaitableTimer(hTimer);
        CloseHandle(hTimer);
        hTimer = NULL;
    }

    if (hStopEvent)
    {
        CloseHandle(hStopEvent);
        hStopEvent = NULL;
    }

    UpdateServiceStatus(SERVICE_STOPPED);
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD fdwReason,
        LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
