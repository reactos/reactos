/*
 * PROJECT:         ReactOS EventLog Service
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/services/eventlog/eventlog.c
 * PURPOSE:         Event logging service
 * COPYRIGHT:       Copyright 2002 Eric Kohl
 *                  Copyright 2005 Saveliy Tretiakov
 *                  Hermes Belusca-Maito
 */

/* INCLUDES *****************************************************************/

#include "eventlog.h"
#include <stdio.h>
#include <netevent.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

static VOID CALLBACK ServiceMain(DWORD, LPWSTR*);
static WCHAR ServiceName[] = L"EventLog";
static SERVICE_TABLE_ENTRYW ServiceTable[2] =
{
    { ServiceName, ServiceMain },
    { NULL, NULL }
};

SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE ServiceStatusHandle;

BOOL onLiveCD = FALSE;  // On LiveCD events will go to debug output only

PEVENTSOURCE EventLogSource = NULL;

/* FUNCTIONS ****************************************************************/

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
        ServiceStatus.dwWaitHint = 10000;
    else
        ServiceStatus.dwWaitHint = 0;

    SetServiceStatus(ServiceStatusHandle,
                     &ServiceStatus);
}

static DWORD WINAPI
ServiceControlHandler(DWORD dwControl,
                      DWORD dwEventType,
                      LPVOID lpEventData,
                      LPVOID lpContext)
{
    DPRINT("ServiceControlHandler() called\n");

    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
            DPRINT("  SERVICE_CONTROL_STOP received\n");

            LogfReportEvent(EVENTLOG_INFORMATION_TYPE,
                            0,
                            EVENT_EventlogStopped, 0, NULL, 0, NULL);

            /* Stop listening to incoming RPC messages */
            RpcMgmtStopServerListening(NULL);
            UpdateServiceStatus(SERVICE_STOPPED);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_PAUSE:
            DPRINT("  SERVICE_CONTROL_PAUSE received\n");
            UpdateServiceStatus(SERVICE_PAUSED);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_CONTINUE:
            DPRINT("  SERVICE_CONTROL_CONTINUE received\n");
            UpdateServiceStatus(SERVICE_RUNNING);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_INTERROGATE:
            DPRINT("  SERVICE_CONTROL_INTERROGATE received\n");
            SetServiceStatus(ServiceStatusHandle,
                             &ServiceStatus);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_SHUTDOWN:
            DPRINT("  SERVICE_CONTROL_SHUTDOWN received\n");

            LogfReportEvent(EVENTLOG_INFORMATION_TYPE,
                            0,
                            EVENT_EventlogStopped, 0, NULL, 0, NULL);

            UpdateServiceStatus(SERVICE_STOPPED);
            return ERROR_SUCCESS;

        default:
            DPRINT1("  Control %lu received\n", dwControl);
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}


static DWORD
ServiceInit(VOID)
{
    HANDLE hThread;

    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)PortThreadRoutine,
                           NULL,
                           0,
                           NULL);
    if (!hThread)
    {
        DPRINT("Cannot create PortThread\n");
        return GetLastError();
    }
    else
        CloseHandle(hThread);

    hThread = CreateThread(NULL,
                           0,
                           RpcThreadRoutine,
                           NULL,
                           0,
                           NULL);

    if (!hThread)
    {
        DPRINT("Cannot create RpcThread\n");
        return GetLastError();
    }
    else
        CloseHandle(hThread);

    return ERROR_SUCCESS;
}


static VOID
ReportProductInfoEvent(VOID)
{
    OSVERSIONINFOW versionInfo;
    WCHAR szBuffer[512];
    PWSTR str;
    size_t cchRemain;
    HKEY hKey;
    DWORD dwValueLength;
    DWORD dwType;
    LONG lResult = ERROR_SUCCESS;

    ZeroMemory(&versionInfo, sizeof(versionInfo));
    versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);

    /* Get version information */
    if (!GetVersionExW(&versionInfo))
        return;

    ZeroMemory(szBuffer, sizeof(szBuffer));
    str = szBuffer;
    cchRemain = ARRAYSIZE(szBuffer);

    /* Write the version number into the buffer */
    StringCchPrintfExW(str, cchRemain,
                       &str, &cchRemain, 0,
                       L"%lu.%lu",
                       versionInfo.dwMajorVersion,
                       versionInfo.dwMinorVersion);
    str++;
    cchRemain++;

    /* Write the build number into the buffer */
    StringCchPrintfExW(str, cchRemain,
                       &str, &cchRemain, 0,
                       L"%lu",
                       versionInfo.dwBuildNumber);
    str++;
    cchRemain++;

    /* Write the service pack info into the buffer */
    StringCchCopyExW(str, cchRemain,
                     versionInfo.szCSDVersion,
                     &str, &cchRemain, 0);
    str++;
    cchRemain++;

    /* Read 'CurrentType' from the registry and write it into the buffer */
    lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                            0,
                            KEY_QUERY_VALUE,
                            &hKey);
    if (lResult == ERROR_SUCCESS)
    {
        dwValueLength = cchRemain;
        lResult = RegQueryValueExW(hKey,
                                   L"CurrentType",
                                   NULL,
                                   &dwType,
                                   (LPBYTE)str,
                                   &dwValueLength);

        RegCloseKey(hKey);
    }

    /* Log the product information */
    LogfReportEvent(EVENTLOG_INFORMATION_TYPE,
                    0,
                    EVENT_EventLogProductInfo,
                    4,
                    szBuffer,
                    0,
                    NULL);
}


static VOID CALLBACK
ServiceMain(DWORD argc,
            LPWSTR* argv)
{
    DWORD dwError;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    DPRINT("ServiceMain() called\n");

    ServiceStatusHandle = RegisterServiceCtrlHandlerExW(ServiceName,
                                                        ServiceControlHandler,
                                                        NULL);
    if (!ServiceStatusHandle)
    {
        dwError = GetLastError();
        DPRINT1("RegisterServiceCtrlHandlerW() failed! (Error %lu)\n", dwError);
        return;
    }

    UpdateServiceStatus(SERVICE_START_PENDING);

    dwError = ServiceInit();
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("Service stopped (dwError: %lu\n", dwError);
        UpdateServiceStatus(SERVICE_START_PENDING);
    }
    else
    {
        DPRINT("Service started\n");
        UpdateServiceStatus(SERVICE_RUNNING);

        ReportProductInfoEvent();

        LogfReportEvent(EVENTLOG_INFORMATION_TYPE,
                        0,
                        EVENT_EventlogStarted,
                        0,
                        NULL,
                        0,
                        NULL);
    }

    DPRINT("ServiceMain() done\n");
}


static PLOGFILE
LoadLogFile(HKEY hKey, PWSTR LogName)
{
    DWORD MaxValueLen, ValueLen, Type, ExpandedLen;
    PWSTR Buf = NULL, Expanded = NULL;
    LONG Result;
    PLOGFILE pLogf = NULL;
    UNICODE_STRING FileName;
    ULONG ulMaxSize, ulRetention;
    NTSTATUS Status;

    DPRINT("LoadLogFile: `%S'\n", LogName);

    Result = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
                              NULL, NULL, &MaxValueLen, NULL, NULL);
    if (Result != ERROR_SUCCESS)
    {
        DPRINT1("RegQueryInfoKeyW failed: %lu\n", Result);
        return NULL;
    }

    MaxValueLen = ROUND_DOWN(MaxValueLen, sizeof(WCHAR));
    Buf = HeapAlloc(GetProcessHeap(), 0, MaxValueLen);
    if (!Buf)
    {
        DPRINT1("Cannot allocate heap!\n");
        return NULL;
    }

    ValueLen = MaxValueLen;
    Result = RegQueryValueExW(hKey,
                              L"File",
                              NULL,
                              &Type,
                              (LPBYTE)Buf,
                              &ValueLen);
    /*
     * If we failed, because the registry value was inexistent
     * or the value type was incorrect, create a new "File" value
     * that holds the default event log path.
     */
    if ((Result != ERROR_SUCCESS) || (Type != REG_EXPAND_SZ && Type != REG_SZ))
    {
        MaxValueLen = (wcslen(L"%SystemRoot%\\System32\\Config\\") +
                       wcslen(LogName) + wcslen(L".evt") + 1) * sizeof(WCHAR);

        Expanded = HeapReAlloc(GetProcessHeap(), 0, Buf, MaxValueLen);
        if (!Expanded)
        {
            DPRINT1("Cannot reallocate heap!\n");
            HeapFree(GetProcessHeap(), 0, Buf);
            return NULL;
        }
        Buf = Expanded;

        StringCbCopyW(Buf, MaxValueLen, L"%SystemRoot%\\System32\\Config\\");
        StringCbCatW(Buf, MaxValueLen, LogName);
        StringCbCatW(Buf, MaxValueLen, L".evt");

        ValueLen = MaxValueLen;
        Result = RegSetValueExW(hKey,
                                L"File",
                                0,
                                REG_EXPAND_SZ,
                                (LPBYTE)Buf,
                                ValueLen);
        if (Result != ERROR_SUCCESS)
        {
            DPRINT1("RegSetValueExW failed: %lu\n", Result);
            HeapFree(GetProcessHeap(), 0, Buf);
            return NULL;
        }
    }

    ExpandedLen = ExpandEnvironmentStringsW(Buf, NULL, 0);
    Expanded = HeapAlloc(GetProcessHeap(), 0, ExpandedLen * sizeof(WCHAR));
    if (!Expanded)
    {
        DPRINT1("Cannot allocate heap!\n");
        HeapFree(GetProcessHeap(), 0, Buf);
        return NULL;
    }

    ExpandEnvironmentStringsW(Buf, Expanded, ExpandedLen);

    if (!RtlDosPathNameToNtPathName_U(Expanded, &FileName, NULL, NULL))
    {
        DPRINT1("Cannot convert path!\n");
        HeapFree(GetProcessHeap(), 0, Expanded);
        HeapFree(GetProcessHeap(), 0, Buf);
        return NULL;
    }

    DPRINT("%S -> %S\n", Buf, Expanded);

    ValueLen = sizeof(ulMaxSize);
    Result = RegQueryValueExW(hKey,
                              L"MaxSize",
                              NULL,
                              &Type,
                              (LPBYTE)&ulMaxSize,
                              &ValueLen);
    if ((Result != ERROR_SUCCESS) || (Type != REG_DWORD))
    {
        ulMaxSize = 512 * 1024; /* 512 kBytes */

        Result = RegSetValueExW(hKey,
                                L"MaxSize",
                                0,
                                REG_DWORD,
                                (LPBYTE)&ulMaxSize,
                                sizeof(ulMaxSize));
    }

    ValueLen = sizeof(ulRetention);
    Result = RegQueryValueExW(hKey,
                              L"Retention",
                              NULL,
                              &Type,
                              (LPBYTE)&ulRetention,
                              &ValueLen);
    if ((Result != ERROR_SUCCESS) || (Type != REG_DWORD))
    {
        /* On Windows 2003 it is 604800 (secs) == 7 days */
        ulRetention = 0;

        Result = RegSetValueExW(hKey,
                                L"Retention",
                                0,
                                REG_DWORD,
                                (LPBYTE)&ulRetention,
                                sizeof(ulRetention));
    }

    // TODO: Add, or use, default values for "AutoBackupLogFiles" (REG_DWORD)
    // and "CustomSD" (REG_SZ).

    Status = LogfCreate(&pLogf, LogName, &FileName, ulMaxSize, ulRetention, TRUE, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create %S! (Status %08lx)\n", Expanded, Status);
    }

    HeapFree(GetProcessHeap(), 0, Expanded);
    HeapFree(GetProcessHeap(), 0, Buf);
    return pLogf;
}

static BOOL
LoadLogFiles(HKEY eventlogKey)
{
    LONG Result;
    DWORD MaxLognameLen, LognameLen;
    DWORD dwIndex;
    PWSTR Buf = NULL;
    PLOGFILE pLogFile;

    Result = RegQueryInfoKeyW(eventlogKey, NULL, NULL, NULL, NULL, &MaxLognameLen,
                              NULL, NULL, NULL, NULL, NULL, NULL);
    if (Result != ERROR_SUCCESS)
    {
        DPRINT1("RegQueryInfoKeyW failed: %lu\n", Result);
        return FALSE;
    }

    MaxLognameLen++;

    Buf = HeapAlloc(GetProcessHeap(), 0, MaxLognameLen * sizeof(WCHAR));
    if (!Buf)
    {
        DPRINT1("Error: cannot allocate heap!\n");
        return FALSE;
    }

    LognameLen = MaxLognameLen;
    dwIndex = 0;
    while (RegEnumKeyExW(eventlogKey,
                         dwIndex,
                         Buf,
                         &LognameLen,
                         NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        HKEY SubKey;

        DPRINT("%S\n", Buf);

        Result = RegOpenKeyExW(eventlogKey, Buf, 0, KEY_ALL_ACCESS, &SubKey);
        if (Result != ERROR_SUCCESS)
        {
            DPRINT1("Failed to open %S key.\n", Buf);
            HeapFree(GetProcessHeap(), 0, Buf);
            return FALSE;
        }

        pLogFile = LoadLogFile(SubKey, Buf);
        if (pLogFile != NULL)
        {
            DPRINT("Loaded %S\n", Buf);
            LoadEventSources(SubKey, pLogFile);
        }
        else
        {
            DPRINT1("Failed to load %S\n", Buf);
        }

        RegCloseKey(SubKey);

        LognameLen = MaxLognameLen;
        dwIndex++;
    }

    HeapFree(GetProcessHeap(), 0, Buf);
    return TRUE;
}


int wmain(int argc, WCHAR* argv[])
{
    INT RetCode = 0;
    LONG Result;
    HKEY elogKey;
    WCHAR LogPath[MAX_PATH];

    LogfListInitialize();
    InitEventSourceList();

__debugbreak();
    GetSystemWindowsDirectoryW(LogPath, ARRAYSIZE(LogPath));
    if (GetDriveTypeW(LogPath) == DRIVE_CDROM)
    {
        DPRINT("LiveCD detected\n");
        onLiveCD = TRUE;
    }
    else
    {
        Result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                               L"SYSTEM\\CurrentControlSet\\Services\\EventLog",
                               0,
                               KEY_ALL_ACCESS,
                               &elogKey);
        if (Result != ERROR_SUCCESS)
        {
            DPRINT1("Fatal error: cannot open eventlog registry key.\n");
            RetCode = 1;
            goto bye_bye;
        }

        LoadLogFiles(elogKey);
    }

    EventLogSource = GetEventSourceByName(L"EventLog");
    if (!EventLogSource)
    {
        DPRINT1("The 'EventLog' source is unavailable. The EventLog service will not be able to log its own events.\n");
    }

    StartServiceCtrlDispatcher(ServiceTable);

bye_bye:
    LogfCloseAll();

    return RetCode;
}

VOID PRINT_RECORD(PEVENTLOGRECORD pRec)
{
    UINT i;
    PWSTR str;
    LARGE_INTEGER SystemTime;
    TIME_FIELDS Time;

    DPRINT1("PRINT_RECORD(0x%p)\n", pRec);

    DbgPrint("Length = %lu\n", pRec->Length);
    DbgPrint("Reserved = 0x%x\n", pRec->Reserved);
    DbgPrint("RecordNumber = %lu\n", pRec->RecordNumber);

    RtlSecondsSince1970ToTime(pRec->TimeGenerated, &SystemTime);
    RtlTimeToTimeFields(&SystemTime, &Time);
    DbgPrint("TimeGenerated = %hu.%hu.%hu %hu:%hu:%hu\n",
             Time.Day, Time.Month, Time.Year,
             Time.Hour, Time.Minute, Time.Second);

    RtlSecondsSince1970ToTime(pRec->TimeWritten, &SystemTime);
    RtlTimeToTimeFields(&SystemTime, &Time);
    DbgPrint("TimeWritten = %hu.%hu.%hu %hu:%hu:%hu\n",
             Time.Day, Time.Month, Time.Year,
             Time.Hour, Time.Minute, Time.Second);

    DbgPrint("EventID = %lu\n", pRec->EventID);

    switch (pRec->EventType)
    {
        case EVENTLOG_ERROR_TYPE:
            DbgPrint("EventType = EVENTLOG_ERROR_TYPE\n");
            break;
        case EVENTLOG_WARNING_TYPE:
            DbgPrint("EventType = EVENTLOG_WARNING_TYPE\n");
            break;
        case EVENTLOG_INFORMATION_TYPE:
            DbgPrint("EventType = EVENTLOG_INFORMATION_TYPE\n");
            break;
        case EVENTLOG_AUDIT_SUCCESS:
            DbgPrint("EventType = EVENTLOG_AUDIT_SUCCESS\n");
            break;
        case EVENTLOG_AUDIT_FAILURE:
            DbgPrint("EventType = EVENTLOG_AUDIT_FAILURE\n");
            break;
        default:
            DbgPrint("EventType = %hu\n", pRec->EventType);
    }

    DbgPrint("NumStrings = %hu\n", pRec->NumStrings);
    DbgPrint("EventCategory = %hu\n", pRec->EventCategory);
    DbgPrint("ReservedFlags = 0x%x\n", pRec->ReservedFlags);
    DbgPrint("ClosingRecordNumber = %lu\n", pRec->ClosingRecordNumber);
    DbgPrint("StringOffset = %lu\n", pRec->StringOffset);
    DbgPrint("UserSidLength = %lu\n", pRec->UserSidLength);
    DbgPrint("UserSidOffset = %lu\n", pRec->UserSidOffset);
    DbgPrint("DataLength = %lu\n", pRec->DataLength);
    DbgPrint("DataOffset = %lu\n", pRec->DataOffset);

    i = sizeof(EVENTLOGRECORD);
    DbgPrint("SourceName: %S\n", (PWSTR)((ULONG_PTR)pRec + i));

    i += (wcslen((PWSTR)((ULONG_PTR)pRec + i)) + 1) * sizeof(WCHAR);
    DbgPrint("ComputerName: %S\n", (PWSTR)((ULONG_PTR)pRec + i));

    if (pRec->StringOffset < pRec->Length && pRec->NumStrings)
    {
        DbgPrint("Strings:\n");
        str = (PWSTR)((ULONG_PTR)pRec + pRec->StringOffset);
        for (i = 0; i < pRec->NumStrings; i++)
        {
            DbgPrint("[%u] %S\n", i, str);
            str += wcslen(str) + 1;
        }
    }

    DbgPrint("Length2 = %lu\n", *(PULONG)((ULONG_PTR)pRec + pRec->Length - 4));
}
