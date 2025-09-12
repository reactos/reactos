/*
 * PROJECT:         ReactOS EventLog Service
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/services/eventlog/eventsource.c
 * PURPOSE:         Event log sources support
 * COPYRIGHT:       Copyright 2011 Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "eventlog.h"

#define NDEBUG
#include <debug.h>

static LIST_ENTRY EventSourceListHead;
static CRITICAL_SECTION EventSourceListCs;

/* FUNCTIONS ****************************************************************/

VOID
InitEventSourceList(VOID)
{
    InitializeCriticalSection(&EventSourceListCs);
    InitializeListHead(&EventSourceListHead);
}


static VOID
DumpEventSourceList(VOID)
{
    PLIST_ENTRY CurrentEntry;
    PEVENTSOURCE EventSource;

    DPRINT("DumpEventSourceList()\n");
    EnterCriticalSection(&EventSourceListCs);

    CurrentEntry = EventSourceListHead.Flink;
    while (CurrentEntry != &EventSourceListHead)
    {
        EventSource = CONTAINING_RECORD(CurrentEntry,
                                        EVENTSOURCE,
                                        EventSourceListEntry);

        DPRINT("EventSource->szName: %S\n", EventSource->szName);

        CurrentEntry = CurrentEntry->Flink;
    }

    LeaveCriticalSection(&EventSourceListCs);

    DPRINT("Done\n");
}


static BOOL
AddNewEventSource(PLOGFILE pLogFile,
                  PWSTR lpSourceName)
{
    PEVENTSOURCE lpEventSource;

    lpEventSource = HeapAlloc(GetProcessHeap(), 0,
                              FIELD_OFFSET(EVENTSOURCE, szName[wcslen(lpSourceName) + 1]));
    if (lpEventSource != NULL)
    {
        wcscpy(lpEventSource->szName, lpSourceName);
        lpEventSource->LogFile = pLogFile;

        DPRINT("Insert event source: %S\n", lpEventSource->szName);

        EnterCriticalSection(&EventSourceListCs);
        InsertTailList(&EventSourceListHead,
                       &lpEventSource->EventSourceListEntry);
        LeaveCriticalSection(&EventSourceListCs);
    }

    return (lpEventSource != NULL);
}


BOOL
LoadEventSources(HKEY hKey,
                 PLOGFILE pLogFile)
{
    BOOL Success;
    DWORD dwNumSubKeys, dwMaxSubKeyLength;
    DWORD dwEventSourceNameLength, MaxValueLen;
    DWORD dwIndex;
    PWSTR Buf = NULL, SourceList = NULL, Source = NULL;
    size_t cchRemaining = 0;
    LONG Result;

    DPRINT("LoadEventSources\n");

    Result = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, &dwNumSubKeys, &dwMaxSubKeyLength,
                              NULL, NULL, NULL, NULL, NULL, NULL);
    if (Result != ERROR_SUCCESS)
    {
        DPRINT1("RegQueryInfoKeyW failed: %lu\n", Result);
        return FALSE;
    }

    dwMaxSubKeyLength++;

    Buf = HeapAlloc(GetProcessHeap(), 0, dwMaxSubKeyLength * sizeof(WCHAR));
    if (!Buf)
    {
        DPRINT1("Error: cannot allocate heap!\n");
        return FALSE;
    }

    /*
     * Allocate a buffer for storing the names of the sources as a REG_MULTI_SZ
     * in the registry. Also add the event log as its own source.
     * Add a final NULL-terminator.
     */
    MaxValueLen = dwNumSubKeys * dwMaxSubKeyLength + wcslen(pLogFile->LogName) + 2;
    SourceList = HeapAlloc(GetProcessHeap(), 0, MaxValueLen * sizeof(WCHAR));
    if (!SourceList)
    {
        DPRINT1("Error: cannot allocate heap!\n");
        /* It is not dramatic if we cannot create it */
    }
    else
    {
        cchRemaining = MaxValueLen;
        Source = SourceList;
    }

    /*
     * Enumerate all the subkeys of the event log key, that constitute
     * all the possible event sources for this event log. At this point,
     * skip the possible existing source having the same name as the
     * event log, it will be added later on.
     */
    dwEventSourceNameLength = dwMaxSubKeyLength;
    dwIndex = 0;
    while (RegEnumKeyExW(hKey,
                         dwIndex,
                         Buf,
                         &dwEventSourceNameLength,
                         NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        if (_wcsicmp(pLogFile->LogName, Buf) != 0)
        {
            DPRINT("Event Source: %S\n", Buf);
            Success = AddNewEventSource(pLogFile, Buf);
            if (Success && (Source != NULL))
            {
                /* Append the event source name and an extra NULL-terminator */
                StringCchCopyExW(Source, cchRemaining, Buf, &Source, &cchRemaining, 0);
                if (cchRemaining > 0)
                {
                    *++Source = L'\0';
                    cchRemaining--;
                }
            }
        }

        dwEventSourceNameLength = dwMaxSubKeyLength;
        dwIndex++;
    }

    /* Finally, allow the event log itself to be its own source */
    DPRINT("Event Source: %S\n", pLogFile->LogName);
    Success = AddNewEventSource(pLogFile, pLogFile->LogName);
    if (Success && (Source != NULL))
    {
        /* Append the event source name and an extra NULL-terminator */
        StringCchCopyExW(Source, cchRemaining, pLogFile->LogName, &Source, &cchRemaining, 0);
        if (cchRemaining > 0)
        {
            *++Source = L'\0';
            cchRemaining--;
        }
    }

    /* Save the list of sources in the registry */
    Result = RegSetValueExW(hKey,
                            L"Sources",
                            0,
                            REG_MULTI_SZ,
                            (LPBYTE)SourceList,
                            (MaxValueLen - cchRemaining + 1) * sizeof(WCHAR));
    if (Result != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueExW failed: %lu\n", Result);
    }

    if (SourceList)
        HeapFree(GetProcessHeap(), 0, SourceList);

    HeapFree(GetProcessHeap(), 0, Buf);

    DumpEventSourceList();

    return TRUE;
}


PEVENTSOURCE
GetEventSourceByName(LPCWSTR Name)
{
    PLIST_ENTRY CurrentEntry;
    PEVENTSOURCE Item, Result = NULL;

    DPRINT("GetEventSourceByName(%S)\n", Name);
    EnterCriticalSection(&EventSourceListCs);

    CurrentEntry = EventSourceListHead.Flink;
    while (CurrentEntry != &EventSourceListHead)
    {
        Item = CONTAINING_RECORD(CurrentEntry,
                                 EVENTSOURCE,
                                 EventSourceListEntry);

        DPRINT("Item->szName: %S\n", Item->szName);
//        if ((*(Item->szName) != 0) && !_wcsicmp(Item->szName, Name))
        if (_wcsicmp(Item->szName, Name) == 0)
        {
            DPRINT("Found it\n");
            Result = Item;
            break;
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    LeaveCriticalSection(&EventSourceListCs);

    DPRINT("Done (Result: %p)\n", Result);

    return Result;
}
