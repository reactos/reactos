/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/eventlog/eventlog.c
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2002 Eric Kohl
 *                   Copyright 2005 Saveliy Tretiakov
 */

/* INCLUDES *****************************************************************/

#include "eventlog.h"

#include <stdio.h>
#include <netevent.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

static VOID CALLBACK ServiceMain(DWORD, LPWSTR *);
static WCHAR ServiceName[] = L"EventLog";
static SERVICE_TABLE_ENTRYW ServiceTable[2] =
{
    { ServiceName, ServiceMain },
    { NULL, NULL }
};

SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE ServiceStatusHandle;

BOOL onLiveCD = FALSE;  // On livecd events will go to debug output only
HANDLE MyHeap = NULL;

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
                           (LPTHREAD_START_ROUTINE)
                            PortThreadRoutine,
                           NULL,
                           0,
                           NULL);

    if (!hThread)
    {
        DPRINT("Can't create PortThread\n");
        return GetLastError();
    }
    else
        CloseHandle(hThread);

    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)
                            RpcThreadRoutine,
                           NULL,
                           0,
                           NULL);

    if (!hThread)
    {
        DPRINT("Can't create RpcThread\n");
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
    DWORD dwLength;
    HKEY hKey;
    DWORD dwValueLength;
    DWORD dwType;
    LONG lResult = ERROR_SUCCESS;

    ZeroMemory(&versionInfo, sizeof(OSVERSIONINFO));
    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    /* Get version information */
    if (!GetVersionExW(&versionInfo))
        return;

    ZeroMemory(szBuffer, 512 * sizeof(WCHAR));

    /* Write version into the buffer */
    dwLength = swprintf(szBuffer,
                        L"%lu.%lu",
                        versionInfo.dwMajorVersion,
                        versionInfo.dwMinorVersion) + 1;

    /* Write build number into the buffer */
    dwLength += swprintf(&szBuffer[dwLength],
                         L"%lu",
                         versionInfo.dwBuildNumber) + 1;

    /* Write service pack info into the buffer */
    wcscpy(&szBuffer[dwLength], versionInfo.szCSDVersion);
    dwLength += wcslen(versionInfo.szCSDVersion) + 1;

    /* Read 'CurrentType' from the registry and write it into the buffer */
    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                           0,
                           KEY_QUERY_VALUE,
                           &hKey);
    if (lResult == ERROR_SUCCESS)
    {
        dwValueLength = 512 - dwLength;
        lResult = RegQueryValueEx(hKey,
                                  L"CurrentType",
                                  NULL,
                                  &dwType,
                                  (LPBYTE)&szBuffer[dwLength],
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
            LPWSTR *argv)
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


PLOGFILE LoadLogFile(HKEY hKey, WCHAR * LogName)
{
    DWORD MaxValueLen, ValueLen, Type, ExpandedLen;
    WCHAR *Buf = NULL, *Expanded = NULL;
    LONG Result;
    PLOGFILE pLogf = NULL;
    UNICODE_STRING FileName;
    ULONG ulMaxSize, ulRetention;
    NTSTATUS Status;

    DPRINT("LoadLogFile: %S\n", LogName);

    Result = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, &MaxValueLen, NULL, NULL);
    if (Result != ERROR_SUCCESS)
    {
        DPRINT1("RegQueryInfoKey failed: %lu\n", Result);
        return NULL;
    }

    Buf = HeapAlloc(MyHeap, 0, MaxValueLen);
    if (!Buf)
    {
        DPRINT1("Can't allocate heap!\n");
        return NULL;
    }

    ValueLen = MaxValueLen;

    Result = RegQueryValueEx(hKey,
                             L"File",
                             NULL,
                             &Type,
                             (LPBYTE) Buf,
                             &ValueLen);
    if (Result != ERROR_SUCCESS)
    {
        DPRINT1("RegQueryValueEx failed: %lu\n", Result);
        HeapFree(MyHeap, 0, Buf);
        return NULL;
    }

    if (Type != REG_EXPAND_SZ && Type != REG_SZ)
    {
        DPRINT1("%S\\File - value of wrong type %x.\n", LogName, Type);
        HeapFree(MyHeap, 0, Buf);
        return NULL;
    }

    ExpandedLen = ExpandEnvironmentStrings(Buf, NULL, 0);
    Expanded = HeapAlloc(MyHeap, 0, ExpandedLen * sizeof(WCHAR));
    if (!Expanded)
    {
        DPRINT1("Can't allocate heap!\n");
        HeapFree(MyHeap, 0, Buf);
        return NULL;
    }

    ExpandEnvironmentStrings(Buf, Expanded, ExpandedLen);

    if (!RtlDosPathNameToNtPathName_U(Expanded, &FileName,
                                      NULL, NULL))
    {
        DPRINT1("Can't convert path!\n");
        HeapFree(MyHeap, 0, Expanded);
        HeapFree(MyHeap, 0, Buf);
        return NULL;
    }

    DPRINT("%S -> %S\n", Buf, Expanded);

    ValueLen = sizeof(ULONG);
    Result = RegQueryValueEx(hKey,
                             L"MaxSize",
                             NULL,
                             &Type,
                             (LPBYTE)&ulMaxSize,
                             &ValueLen);
    if (Result != ERROR_SUCCESS)
        ulMaxSize = 512 * 1024; /* 512 kBytes */

    ValueLen = sizeof(ULONG);
    Result = RegQueryValueEx(hKey,
                             L"Retention",
                             NULL,
                             &Type,
                             (LPBYTE)&ulRetention,
                             &ValueLen);
    if (Result != ERROR_SUCCESS)
        ulRetention = 0;

    Status = LogfCreate(&pLogf, LogName, &FileName, ulMaxSize, ulRetention, TRUE, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create %S! (Status %08lx)\n", Expanded, Status);
    }

    HeapFree(MyHeap, 0, Buf);
    HeapFree(MyHeap, 0, Expanded);
    return pLogf;
}

BOOL LoadLogFiles(HKEY eventlogKey)
{
    LONG Result;
    DWORD MaxLognameLen, LognameLen;
    WCHAR *Buf = NULL;
    INT i;
    PLOGFILE pLogFile;

    Result = RegQueryInfoKey(eventlogKey,
                             NULL, NULL, NULL, NULL,
                             &MaxLognameLen,
                             NULL, NULL, NULL, NULL, NULL, NULL);
    if (Result != ERROR_SUCCESS)
    {
        DPRINT1("RegQueryInfoKey failed: %lu\n", Result);
        return FALSE;
    }

    MaxLognameLen++;

    Buf = HeapAlloc(MyHeap, 0, MaxLognameLen * sizeof(WCHAR));

    if (!Buf)
    {
        DPRINT1("Error: can't allocate heap!\n");
        return FALSE;
    }

    i = 0;
    LognameLen = MaxLognameLen;

    while (RegEnumKeyEx(eventlogKey,
                        i,
                        Buf,
                        &LognameLen,
                        NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        HKEY SubKey;

        DPRINT("%S\n", Buf);

        Result = RegOpenKeyEx(eventlogKey, Buf, 0, KEY_ALL_ACCESS, &SubKey);
        if (Result != ERROR_SUCCESS)
        {
            DPRINT1("Failed to open %S key.\n", Buf);
            HeapFree(MyHeap, 0, Buf);
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
        i++;
    }

    HeapFree(MyHeap, 0, Buf);
    return TRUE;
}

INT wmain()
{
    WCHAR LogPath[MAX_PATH];
    INT RetCode = 0;
    LONG Result;
    HKEY elogKey;

    LogfListInitialize();
    InitEventSourceList();

    MyHeap = HeapCreate(0, 1024 * 256, 0);

    if (!MyHeap)
    {
        DPRINT1("FATAL ERROR, can't create heap.\n");
        RetCode = 1;
        goto bye_bye;
    }

    GetWindowsDirectory(LogPath, MAX_PATH);

    if (GetDriveType(LogPath) == DRIVE_CDROM)
    {
        DPRINT("LiveCD detected\n");
        onLiveCD = TRUE;
    }
    else
    {
        Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              L"SYSTEM\\CurrentControlSet\\Services\\EventLog",
                              0,
                              KEY_ALL_ACCESS,
                              &elogKey);

        if (Result != ERROR_SUCCESS)
        {
            DPRINT1("Fatal error: can't open eventlog registry key.\n");
            RetCode = 1;
            goto bye_bye;
        }

        LoadLogFiles(elogKey);
    }

    StartServiceCtrlDispatcher(ServiceTable);

  bye_bye:
    LogfCloseAll();

    if (MyHeap)
        HeapDestroy(MyHeap);

    return RetCode;
}

VOID EventTimeToSystemTime(DWORD EventTime, SYSTEMTIME * pSystemTime)
{
    SYSTEMTIME st1970 = { 1970, 1, 0, 1, 0, 0, 0, 0 };
    FILETIME ftLocal;
    union
    {
        FILETIME ft;
        ULONGLONG ll;
    } u1970, uUCT;

    uUCT.ft.dwHighDateTime = 0;
    uUCT.ft.dwLowDateTime = EventTime;
    SystemTimeToFileTime(&st1970, &u1970.ft);
    uUCT.ll = uUCT.ll * 10000000 + u1970.ll;
    FileTimeToLocalFileTime(&uUCT.ft, &ftLocal);
    FileTimeToSystemTime(&ftLocal, pSystemTime);
}

VOID SystemTimeToEventTime(SYSTEMTIME * pSystemTime, DWORD * pEventTime)
{
    SYSTEMTIME st1970 = { 1970, 1, 0, 1, 0, 0, 0, 0 };
    union
    {
        FILETIME ft;
        ULONGLONG ll;
    } Time, u1970;

    SystemTimeToFileTime(pSystemTime, &Time.ft);
    SystemTimeToFileTime(&st1970, &u1970.ft);
    *pEventTime = (DWORD)((Time.ll - u1970.ll) / 10000000ull);
}

VOID PRINT_HEADER(PEVENTLOGHEADER header)
{
    DPRINT("HeaderSize = %lu\n", header->HeaderSize);
    DPRINT("Signature = 0x%x\n", header->Signature);
    DPRINT("MajorVersion = %lu\n", header->MajorVersion);
    DPRINT("MinorVersion = %lu\n", header->MinorVersion);
    DPRINT("StartOffset = %lu\n", header->StartOffset);
    DPRINT("EndOffset = 0x%x\n", header->EndOffset);
    DPRINT("CurrentRecordNumber = %lu\n", header->CurrentRecordNumber);
    DPRINT("OldestRecordNumber = %lu\n", header->OldestRecordNumber);
    DPRINT("MaxSize = 0x%x\n", header->MaxSize);
    DPRINT("Retention = 0x%x\n", header->Retention);
    DPRINT("EndHeaderSize = %lu\n", header->EndHeaderSize);
    DPRINT("Flags: ");
    if (header->Flags & ELF_LOGFILE_HEADER_DIRTY)  DPRINT("ELF_LOGFILE_HEADER_DIRTY");
    if (header->Flags & ELF_LOGFILE_HEADER_WRAP)  DPRINT("| ELF_LOGFILE_HEADER_WRAP ");
    if (header->Flags & ELF_LOGFILE_LOGFULL_WRITTEN)  DPRINT("| ELF_LOGFILE_LOGFULL_WRITTEN ");
    if (header->Flags & ELF_LOGFILE_ARCHIVE_SET)  DPRINT("| ELF_LOGFILE_ARCHIVE_SET ");
    DPRINT("\n");
}

VOID PRINT_RECORD(PEVENTLOGRECORD pRec)
{
    UINT i;
    WCHAR *str;
    SYSTEMTIME time;

    DPRINT("Length = %lu\n", pRec->Length);
    DPRINT("Reserved = 0x%x\n", pRec->Reserved);
    DPRINT("RecordNumber = %lu\n", pRec->RecordNumber);

    EventTimeToSystemTime(pRec->TimeGenerated, &time);
    DPRINT("TimeGenerated = %hu.%hu.%hu %hu:%hu:%hu\n",
           time.wDay, time.wMonth, time.wYear,
           time.wHour, time.wMinute, time.wSecond);

    EventTimeToSystemTime(pRec->TimeWritten, &time);
    DPRINT("TimeWritten = %hu.%hu.%hu %hu:%hu:%hu\n",
           time.wDay, time.wMonth, time.wYear,
           time.wHour, time.wMinute, time.wSecond);

    DPRINT("EventID = %lu\n", pRec->EventID);

    switch (pRec->EventType)
    {
        case EVENTLOG_ERROR_TYPE:
            DPRINT("EventType = EVENTLOG_ERROR_TYPE\n");
            break;
        case EVENTLOG_WARNING_TYPE:
            DPRINT("EventType = EVENTLOG_WARNING_TYPE\n");
            break;
        case EVENTLOG_INFORMATION_TYPE:
            DPRINT("EventType = EVENTLOG_INFORMATION_TYPE\n");
            break;
        case EVENTLOG_AUDIT_SUCCESS:
            DPRINT("EventType = EVENTLOG_AUDIT_SUCCESS\n");
            break;
        case EVENTLOG_AUDIT_FAILURE:
            DPRINT("EventType = EVENTLOG_AUDIT_FAILURE\n");
            break;
        default:
            DPRINT("EventType = %hu\n", pRec->EventType);
    }

    DPRINT("NumStrings = %hu\n", pRec->NumStrings);
    DPRINT("EventCategory = %hu\n", pRec->EventCategory);
    DPRINT("ReservedFlags = 0x%x\n", pRec->ReservedFlags);
    DPRINT("ClosingRecordNumber = %lu\n", pRec->ClosingRecordNumber);
    DPRINT("StringOffset = %lu\n", pRec->StringOffset);
    DPRINT("UserSidLength = %lu\n", pRec->UserSidLength);
    DPRINT("UserSidOffset = %lu\n", pRec->UserSidOffset);
    DPRINT("DataLength = %lu\n", pRec->DataLength);
    DPRINT("DataOffset = %lu\n", pRec->DataOffset);

    DPRINT("SourceName: %S\n", (WCHAR *) (((PBYTE) pRec) + sizeof(EVENTLOGRECORD)));

    i = (lstrlenW((WCHAR *) (((PBYTE) pRec) + sizeof(EVENTLOGRECORD))) + 1) *
        sizeof(WCHAR);

    DPRINT("ComputerName: %S\n", (WCHAR *) (((PBYTE) pRec) + sizeof(EVENTLOGRECORD) + i));

    if (pRec->StringOffset < pRec->Length && pRec->NumStrings)
    {
        DPRINT("Strings:\n");
        str = (WCHAR *) (((PBYTE) pRec) + pRec->StringOffset);
        for (i = 0; i < pRec->NumStrings; i++)
        {
            DPRINT("[%u] %S\n", i, str);
            str = str + lstrlenW(str) + 1;
        }
    }

    DPRINT("Length2 = %lu\n", *(PDWORD) (((PBYTE) pRec) + pRec->Length - 4));
}
