/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/eventlog/eventlog.c
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2002 Eric Kohl
 *                   Copyright 2005 Saveliy Tretiakov
 */

/* INCLUDES *****************************************************************/

#include "eventlog.h"

/* GLOBALS ******************************************************************/

VOID CALLBACK ServiceMain(DWORD argc, LPTSTR * argv);

SERVICE_TABLE_ENTRY ServiceTable[2] =
{
    { L"EventLog", (LPSERVICE_MAIN_FUNCTION) ServiceMain },
    { NULL, NULL }
};

BOOL onLiveCD = FALSE;  // On livecd events will go to debug output only
HANDLE MyHeap = NULL;

/* FUNCTIONS ****************************************************************/

VOID CALLBACK ServiceMain(DWORD argc, LPTSTR * argv)
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
        DPRINT("Can't create PortThread\n");
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
        DPRINT("Can't create RpcThread\n");
    else
        CloseHandle(hThread);
}

BOOL LoadLogFile(HKEY hKey, WCHAR * LogName)
{
    DWORD MaxValueLen, ValueLen, Type, ExpandedLen;
    WCHAR *Buf = NULL, *Expanded = NULL;
    LONG Result;
    BOOL ret = TRUE;
    PLOGFILE pLogf;

    DPRINT("LoadLogFile: %S\n", LogName);

    RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
                    NULL, NULL, &MaxValueLen, NULL, NULL);

    Buf = HeapAlloc(MyHeap, 0, MaxValueLen);

    if (!Buf)
    {
        DPRINT1("Can't allocate heap!\n");
        return FALSE;
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
        DPRINT1("RegQueryValueEx failed: %d\n", GetLastError());
        HeapFree(MyHeap, 0, Buf);
        return FALSE;
    }

    if (Type != REG_EXPAND_SZ && Type != REG_SZ)
    {
        DPRINT1("%S\\File - value of wrong type %x.\n", LogName, Type);
        HeapFree(MyHeap, 0, Buf);
        return FALSE;
    }

    ExpandedLen = ExpandEnvironmentStrings(Buf, NULL, 0);
    Expanded = HeapAlloc(MyHeap, 0, ExpandedLen * sizeof(WCHAR));

    if (!Expanded)
    {
        DPRINT1("Can't allocate heap!\n");
        HeapFree(MyHeap, 0, Buf);
        return FALSE;
    }

    ExpandEnvironmentStrings(Buf, Expanded, ExpandedLen);

    DPRINT("%S -> %S\n", Buf, Expanded);

    pLogf = LogfCreate(LogName, Expanded);

    if (pLogf == NULL)
    {
        DPRINT1("Failed to create %S!\n", Expanded);
        ret = FALSE;
    }

    HeapFree(MyHeap, 0, Buf);
    HeapFree(MyHeap, 0, Expanded);
    return ret;
}

BOOL LoadLogFiles(HKEY eventlogKey)
{
    LONG result;
    DWORD MaxLognameLen, LognameLen;
    WCHAR *Buf = NULL;
    INT i;

    RegQueryInfoKey(eventlogKey,
                    NULL, NULL, NULL, NULL,
                    &MaxLognameLen,
                    NULL, NULL, NULL, NULL, NULL, NULL);

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

        result = RegOpenKeyEx(eventlogKey, Buf, 0, KEY_ALL_ACCESS, &SubKey);
        if (result != ERROR_SUCCESS)
        {
            DPRINT1("Failed to open %S key.\n", Buf);
            HeapFree(MyHeap, 0, Buf);
            return FALSE;
        }

        if (!LoadLogFile(SubKey, Buf))
            DPRINT1("Failed to load %S\n", Buf);
        else
            DPRINT("Loaded %S\n", Buf);

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
    LONG result;
    HKEY elogKey;

    LogfListInitialize();

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
        result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              L"SYSTEM\\CurrentControlSet\\Services\\EventLog",
                              0,
                              KEY_ALL_ACCESS,
                              &elogKey);

        if (result != ERROR_SUCCESS)
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
    *pEventTime = (Time.ll - u1970.ll) / 10000000;
}

VOID PRINT_HEADER(PFILE_HEADER header)
{
    DPRINT("SizeOfHeader = %d\n", header->SizeOfHeader);
    DPRINT("Signature = 0x%x\n", header->Signature);
    DPRINT("MajorVersion = %d\n", header->MajorVersion);
    DPRINT("MinorVersion = %d\n", header->MinorVersion);
    DPRINT("FirstRecordOffset = %d\n", header->FirstRecordOffset);
    DPRINT("EofOffset = 0x%x\n", header->EofOffset);
    DPRINT("NextRecord = %d\n", header->NextRecord);
    DPRINT("OldestRecord = %d\n", header->OldestRecord);
    DPRINT("unknown1 = 0x%x\n", header->unknown1);
    DPRINT("unknown2 = 0x%x\n", header->unknown2);
    DPRINT("SizeOfHeader2 = %d\n", header->SizeOfHeader2);
    DPRINT("Flags: ");
    if (header->Flags & LOGFILE_FLAG1)  DPRINT("LOGFILE_FLAG1 ");
    if (header->Flags & LOGFILE_FLAG2)  DPRINT("| LOGFILE_FLAG2 ");
    if (header->Flags & LOGFILE_FLAG3)  DPRINT("| LOGFILE_FLAG3 ");
    if (header->Flags & LOGFILE_FLAG4)  DPRINT("| LOGFILE_FLAG4");
    DPRINT("\n");
}

VOID PRINT_RECORD(PEVENTLOGRECORD pRec)
{
    UINT i;
    WCHAR *str;
    SYSTEMTIME time;

    DPRINT("Length = %d\n", pRec->Length);
    DPRINT("Reserved = 0x%x\n", pRec->Reserved);
    DPRINT("RecordNumber = %d\n", pRec->RecordNumber);

    EventTimeToSystemTime(pRec->TimeGenerated, &time);
    DPRINT("TimeGenerated = %d.%d.%d %d:%d:%d\n",
           time.wDay, time.wMonth, time.wYear,
           time.wHour, time.wMinute, time.wSecond);

    EventTimeToSystemTime(pRec->TimeWritten, &time);
    DPRINT("TimeWritten = %d.%d.%d %d:%d:%d\n",
           time.wDay, time.wMonth, time.wYear,
           time.wHour, time.wMinute, time.wSecond);

    DPRINT("EventID = %d\n", pRec->EventID);

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
            DPRINT("EventType = %d\n", pRec->EventType);
    }

    DPRINT("NumStrings = %d\n", pRec->NumStrings);
    DPRINT("EventCategory = %d\n", pRec->EventCategory);
    DPRINT("ReservedFlags = 0x%x\n", pRec->ReservedFlags);
    DPRINT("ClosingRecordNumber = %d\n", pRec->ClosingRecordNumber);
    DPRINT("StringOffset = %d\n", pRec->StringOffset);
    DPRINT("UserSidLength = %d\n", pRec->UserSidLength);
    DPRINT("UserSidOffset = %d\n", pRec->UserSidOffset);
    DPRINT("DataLength = %d\n", pRec->DataLength);
    DPRINT("DataOffset = %d\n", pRec->DataOffset);

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
            DPRINT("[%d] %S\n", i, str);
            str = str + lstrlenW(str) + 1;
        }
    }

    DPRINT("Length2 = %d\n", *(PDWORD) (((PBYTE) pRec) + pRec->Length - 4));
}
