/*
 * PROJECT:         ReactOS Tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            rostests/win32/advapi32/eventlog/EvtLogTest.c
 * PURPOSE:         Interactively test some EventLog service APIs and its behaviour.
 * PROGRAMMER:      Hermes Belusca-Maito
 */

#include <stdio.h>
#include <conio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "MyEventProvider.h"

#define PROVIDER_NAME L"MyEventProvider"

BOOL CreateEventLog(LPCWSTR EventLogName,
                    UINT MaxSize,
                    UINT SourcesCount,
                    LPCWSTR EventLogSources[])
{
    BOOL Success = FALSE;
    LONG lRet;
    HKEY hKey = NULL, hEventKey = NULL, hSrcKey = NULL;
    UINT i;
    // WCHAR evtFile[] = L"D:\\myfile.evtx";

    wprintf(L"Creating log %s of MaxSize 0x%x with %d sources...", EventLogName, MaxSize, SourcesCount);

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SYSTEM\\CurrentControlSet\\Services\\Eventlog",
                         0, KEY_CREATE_SUB_KEY,
                         &hKey);
    if (lRet != ERROR_SUCCESS)
        goto Quit;

    /*
     * At this precise moment, EventLog detects we create a new log registry key
     * and therefore creates for us the log file in System32\config (<= Win2k3)
     * or in System32\winevt\Logs (>= Vista).
     */
    lRet = RegCreateKeyExW(hKey,
                           EventLogName,
                           0, NULL, REG_OPTION_NON_VOLATILE,
                           KEY_CREATE_SUB_KEY | KEY_SET_VALUE, NULL,
                           &hEventKey, NULL);
    if (lRet != ERROR_SUCCESS)
        goto Quit;

    RegSetValueExW(hEventKey, L"MaxSize", 0, REG_DWORD,
                   (LPBYTE)&MaxSize, sizeof(MaxSize));

    i = 0;
    RegSetValueExW(hEventKey, L"Retention", 0, REG_DWORD,
                   (LPBYTE)&i, sizeof(i));

#if 0
    /*
     * Set the flag that will allow EventLog to use an alternative log file name.
     * If this flag is not set, EventLog will not care about the "File" value.
     * When the flag is set, EventLog monitors for the "File" value, which can be
     * changed at run-time, in which case a new event log file is created.
     */
    i = 1;
    RegSetValueExW(hEventKey, L"Flags", 0, REG_DWORD,
                   (LPBYTE)&i, sizeof(i));

    RegSetValueExW(hEventKey, L"File", 0, REG_EXPAND_SZ,
                   (LPBYTE)evtFile, sizeof(evtFile));
#endif

    for (i = 0; i < SourcesCount; i++)
    {
        lRet = RegCreateKeyExW(hEventKey,
                               EventLogSources[i],
                               0, NULL, REG_OPTION_NON_VOLATILE,
                               KEY_QUERY_VALUE, NULL,
                               &hSrcKey, NULL);
        RegFlushKey(hSrcKey);
        RegCloseKey(hSrcKey);
    }

    RegFlushKey(hEventKey);

    Success = TRUE;

Quit:
    if (Success)
        wprintf(L"Success\n");
    else
        wprintf(L"Failure\n");

    if (hEventKey)
        RegCloseKey(hEventKey);

    if (hKey)
        RegCloseKey(hKey);

    return Success;
}

BOOL RemoveEventLog(LPCWSTR EventLogName)
{
    BOOL Success = FALSE;
    LONG lRet;
    HKEY hKey, hEventKey;
    DWORD MaxKeyNameLen, KeyNameLen;
    LPWSTR Buf = NULL;

    wprintf(L"Deleting log %s...", EventLogName);

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SYSTEM\\CurrentControlSet\\Services\\Eventlog",
                         0, KEY_CREATE_SUB_KEY,
                         &hKey);
    if (lRet != ERROR_SUCCESS)
        goto Quit;

    lRet = RegOpenKeyExW(hKey,
                         EventLogName,
                         0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                         &hEventKey);
    if (lRet != ERROR_SUCCESS)
        goto Quit;

    lRet = RegQueryInfoKeyW(hEventKey,
                            NULL, NULL, NULL, NULL,
                            &MaxKeyNameLen,
                            NULL, NULL, NULL, NULL, NULL, NULL);
    if (lRet != ERROR_SUCCESS)
        goto Quit;

    MaxKeyNameLen++;

    Buf = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, MaxKeyNameLen * sizeof(WCHAR));
    if (!Buf)
        goto Quit;

    KeyNameLen = MaxKeyNameLen;
    while (RegEnumKeyExW(hEventKey,
                         0,
                         Buf,
                         &KeyNameLen,
                         NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        RegDeleteKeyW(hEventKey, Buf);
        KeyNameLen = MaxKeyNameLen;
    }

    RegFlushKey(hEventKey);

    HeapFree(GetProcessHeap(), 0, Buf);

    Success = TRUE;

Quit:
    if (Success)
    {
        RegCloseKey(hEventKey);
        RegDeleteKeyW(hKey, EventLogName);

        wprintf(L"Success\n");
    }
    else
    {
        if (hEventKey)
            RegCloseKey(hEventKey);

        wprintf(L"Failure\n");
    }

    if (hKey)
        RegCloseKey(hKey);

    return Success;
}

VOID TestEventsGeneration(VOID)
{
    BOOL Success = FALSE;
    LPCWSTR EvtLog = L"MyLog";
    LPCWSTR Sources[] = {L"Source1", L"Source2"};
    HANDLE hEventLog;
    ULONG MaxSize = max(0x30 + 0x28 + 0x200, 0x010000);

    /* Create the test event log */
    if (!CreateEventLog(EvtLog, MaxSize, ARRAYSIZE(Sources), Sources))
        return;

    wprintf(L"Press any key to continue...\n");
    _getch();

    /* To report events we can either use a handle got from OpenEventLog or from RegisterEventSource! */
    hEventLog = OpenEventLogW(NULL, EvtLog);
    wprintf(L"OpenEventLogW(NULL, EvtLog = %s) ", EvtLog);
    if (hEventLog)
    {
        LPCWSTR String = L"Event from OpenEventLog handle with EvtLog";

        wprintf(L"succeeded\n");
        Success = ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, 1, 1, NULL, 1, 0, &String, NULL);
        if (!Success)
            wprintf(L"    Failed to report event\n");
    }
    else
    {
         wprintf(L"failed\n");
    }
    CloseEventLog(hEventLog);

    /* This call should fail (where we use a source name for OpenEventLog) */
    hEventLog = OpenEventLogW(NULL, L"Source1");
    wprintf(L"OpenEventLogW(NULL, Source = %s) ", L"Source1");
    if (hEventLog)
    {
        LPCWSTR String = L"Event from OpenEventLog handle with Source";

        wprintf(L"succeeded\n");
        Success = ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, 1, 1, NULL, 1, 0, &String, NULL);
        if (!Success)
            wprintf(L"    Failed to report event\n");
    }
    else
    {
        wprintf(L"failed\n");
    }
    CloseEventLog(hEventLog);

    /* Now use RegisterEventSource */
    hEventLog = RegisterEventSourceW(NULL, EvtLog);
    wprintf(L"RegisterEventSourceW(NULL, EvtLog = %s) ", EvtLog);
    if (hEventLog)
    {
        LPCWSTR String = L"Event from RegisterEventSource handle with EvtLog";

        wprintf(L"succeeded\n");
        Success = ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, 1, 1, NULL, 1, 0, &String, NULL);
        if (!Success)
            wprintf(L"    Failed to report event\n");
    }
    else
    {
        wprintf(L"failed\n");
    }
    DeregisterEventSource(hEventLog);

    /* Now use RegisterEventSource */
    hEventLog = RegisterEventSourceW(NULL, L"Source1");
    wprintf(L"RegisterEventSourceW(NULL, Source = %s) ", L"Source1");
    if (hEventLog)
    {
        LPCWSTR String = L"Event from RegisterEventSource handle with Source";

        wprintf(L"succeeded\n");
        Success = ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, 1, 1, NULL, 1, 0, &String, NULL);
        if (!Success)
            wprintf(L"    Failed to report event\n");
    }
    else
    {
        wprintf(L"failed\n");
    }
    DeregisterEventSource(hEventLog);

    /* Now fill the log with one big event */
    hEventLog = OpenEventLogW(NULL, EvtLog);
    wprintf(L"OpenEventLogW(NULL, EvtLog = %s) ", EvtLog);
    if (hEventLog)
    {
        // LPWSTR String = L"Big event";
        PVOID Data;

        wprintf(L"succeeded\n");

        // MaxSize -= (0x30 + 0x28 + 0x300 + 0x40);
        MaxSize = 0xFC80 - sizeof(EVENTLOGRECORD); // With a StartOffset of 0x14, that should allow seeing the effect of splitting the EOF record in half...
        Data = HeapAlloc(GetProcessHeap(), 0, MaxSize);
        if (Data)
        {
            RtlFillMemory(Data, MaxSize, 0xCA);
            Success = ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, 1, 1, NULL, 0 /* 1 */, MaxSize, NULL /* &String */, Data);
            if (!Success)
                wprintf(L"    Failed to report event\n");

            HeapFree(GetProcessHeap(), 0, Data);
        }
    }
    else
    {
         wprintf(L"failed\n");
    }
    CloseEventLog(hEventLog);

    wprintf(L"Press any key to continue...\n");
    _getch();

    /* Delete the test event log */
    RemoveEventLog(EvtLog);
}

/*
 * This code was adapted from the MSDN article "Reporting Events" at:
 * https://learn.microsoft.com/en-us/windows/win32/eventlog/reporting-an-event
 */
VOID TestMyEventProvider(VOID)
{
    LONG lRet;
    HKEY hKey = NULL, hSourceKey = NULL;
    DWORD dwData;
    // WCHAR DllPath[] = L"C:\\Users\\ReactOS\\Desktop\\EvtLogTest\\Debug\\" PROVIDER_NAME L".dll";
    WCHAR DllPath[] = L"C:\\" PROVIDER_NAME L".dll";

    wprintf(L"Testing \"" PROVIDER_NAME L"\" in 'Application' log...");

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application",
                         0, KEY_CREATE_SUB_KEY,
                         &hKey);
    if (lRet != ERROR_SUCCESS)
        goto Quit;

    lRet = RegCreateKeyExW(hKey,
                           PROVIDER_NAME,
                           0, NULL, REG_OPTION_NON_VOLATILE,
                           KEY_CREATE_SUB_KEY | KEY_SET_VALUE, NULL,
                           &hSourceKey, NULL);
    if (lRet != ERROR_SUCCESS)
        goto Quit;

    dwData = 3;
    RegSetValueExW(hSourceKey, L"CategoryCount", 0, REG_DWORD,
                  (LPBYTE)&dwData, sizeof(dwData));

    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
    RegSetValueExW(hSourceKey, L"TypesSupported", 0, REG_DWORD,
                   (LPBYTE)&dwData, sizeof(dwData));

    RegSetValueExW(hSourceKey, L"CategoryMessageFile", 0, REG_SZ,
                   (LPBYTE)DllPath, sizeof(DllPath));

    RegSetValueExW(hSourceKey, L"EventMessageFile", 0, REG_SZ,
                   (LPBYTE)DllPath, sizeof(DllPath));

    RegSetValueExW(hSourceKey, L"ParameterMessageFile", 0, REG_SZ,
                   (LPBYTE)DllPath, sizeof(DllPath));

    RegFlushKey(hSourceKey);

    {
    CONST LPWSTR pBadCommand = L"The command that was not valid";
    CONST LPWSTR pFilename = L"c:\\folder\\file.ext";
    CONST LPWSTR pNumberOfRetries = L"3";
    CONST LPWSTR pSuccessfulRetries = L"0";
    CONST LPWSTR pQuarts = L"8";
    CONST LPWSTR pGallons = L"2";

    HANDLE hEventLog = NULL;
    LPWSTR pInsertStrings[2] = {NULL, NULL};
    DWORD dwEventDataSize = 0;

    // The source name (provider) must exist as a subkey of Application.
    hEventLog = RegisterEventSourceW(NULL, PROVIDER_NAME);
    if (NULL == hEventLog)
    {
        wprintf(L"RegisterEventSource failed with 0x%x.\n", GetLastError());
        goto cleanup;
    }

    // This event includes user-defined data as part of the event.
    // The event message does not use insert strings.
    dwEventDataSize = ((DWORD)wcslen(pBadCommand) + 1) * sizeof(WCHAR);
    if (!ReportEventW(hEventLog, EVENTLOG_ERROR_TYPE, UI_CATEGORY, MSG_INVALID_COMMAND, NULL, 0, dwEventDataSize, NULL, pBadCommand))
    {
        wprintf(L"ReportEvent failed with 0x%x for event 0x%x.\n", GetLastError(), MSG_INVALID_COMMAND);
        goto cleanup;
    }

    // This event uses insert strings.
    pInsertStrings[0] = pFilename;
    if (!ReportEventW(hEventLog, EVENTLOG_ERROR_TYPE, DATABASE_CATEGORY, MSG_BAD_FILE_CONTENTS, NULL, 1, 0, (LPCWSTR*)pInsertStrings, NULL))
    {
        wprintf(L"ReportEvent failed with 0x%x for event 0x%x.\n", GetLastError(), MSG_BAD_FILE_CONTENTS);
        goto cleanup;
    }

    // This event uses insert strings.
    pInsertStrings[0] = pNumberOfRetries;
    pInsertStrings[1] = pSuccessfulRetries;
    if (!ReportEventW(hEventLog, EVENTLOG_WARNING_TYPE, NETWORK_CATEGORY, MSG_RETRIES, NULL, 2, 0, (LPCWSTR*)pInsertStrings, NULL))
    {
        wprintf(L"ReportEvent failed with 0x%x for event 0x%x.\n", GetLastError(), MSG_RETRIES);
        goto cleanup;
    }

    // This event uses insert strings.
    pInsertStrings[0] = pQuarts;
    pInsertStrings[1] = pGallons;
    if (!ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, UI_CATEGORY, MSG_COMPUTE_CONVERSION, NULL, 2, 0, (LPCWSTR*)pInsertStrings, NULL))
    {
        wprintf(L"ReportEvent failed with 0x%x for event 0x%x.\n", GetLastError(), MSG_COMPUTE_CONVERSION);
        goto cleanup;
    }

    wprintf(L"All events successfully reported.\n");

cleanup:

    if (hEventLog)
        DeregisterEventSource(hEventLog);

    }

Quit:
    // RegDeleteKeyW(hKey, PROVIDER_NAME);

    if (hKey)
        RegCloseKey(hKey);
}

int wmain(int argc, WCHAR* argv[])
{
    UINT Choice = 0;

    wprintf(L"\n"
            L"EventLog API interactive tester for ReactOS\n"
            L"===========================================\n"
            L"\n");

ChoiceMenu:
    do
    {
        wprintf(L"What do you want to do:\n"
                L"1) Test events generation.\n"
                L"2) Test customized event provider.\n"
                L"\n"
                L"0) Quit the program.\n"
                L"(Enter the right number, or 0 to quit): ");
        wscanf(L"%lu", &Choice);
        wprintf(L"\n\n");
    } while ((Choice != 0) && (Choice != 1) && (Choice != 2));

    switch (Choice)
    {
        case 0:
            goto Quit;
            break;

        case 1:
            TestEventsGeneration();
            break;

        case 2:
            TestMyEventProvider();
            break;

        default:
            break;
    }
    wprintf(L"\n\n\n\n");
    goto ChoiceMenu;

Quit:
    wprintf(L"Press any key to quit...\n");
    _getch();
    return 0;
}
