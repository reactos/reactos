/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/eventlog/eventsource.c
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2011 Eric Kohl
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


BOOL
LoadEventSources(HKEY hKey,
                 PLOGFILE pLogFile)
{
    PEVENTSOURCE lpEventSource;
    DWORD dwMaxSubKeyLength;
    DWORD dwEventSourceNameLength;
    DWORD dwIndex;
    WCHAR *Buf = NULL;
    LONG Result;

    DPRINT("LoadEventSources\n");

    Result = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, NULL, &dwMaxSubKeyLength, NULL,
                              NULL, NULL, NULL, NULL, NULL);
    if (Result != ERROR_SUCCESS)
    {
        DPRINT1("RegQueryInfoKey failed: %lu\n", Result);
        return FALSE;
    }

    DPRINT("dwMaxSubKeyLength: %lu\n", dwMaxSubKeyLength);

    dwMaxSubKeyLength++;

    Buf = HeapAlloc(MyHeap, 0, dwMaxSubKeyLength * sizeof(WCHAR));
    if (!Buf)
    {
        DPRINT1("Error: can't allocate heap!\n");
        return FALSE;
    }

    dwEventSourceNameLength = dwMaxSubKeyLength;

    dwIndex = 0;
    while (RegEnumKeyExW(hKey,
                         dwIndex,
                         Buf,
                         &dwEventSourceNameLength,
                         NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        DPRINT("Event Source: %S\n", Buf);

        lpEventSource = HeapAlloc(MyHeap, 0, sizeof(EVENTSOURCE) + wcslen(Buf) * sizeof(WCHAR));
        if (lpEventSource != NULL)
        {
            wcscpy(lpEventSource->szName, Buf);
            lpEventSource->LogFile = pLogFile;

            DPRINT("Insert event source: %S\n", lpEventSource->szName);


            EnterCriticalSection(&EventSourceListCs);
            InsertTailList(&EventSourceListHead,
                           &lpEventSource->EventSourceListEntry);
            LeaveCriticalSection(&EventSourceListCs);
        }

        dwEventSourceNameLength = dwMaxSubKeyLength;
        dwIndex++;
    }

    HeapFree(MyHeap, 0, Buf);

    DumpEventSourceList();

    return TRUE;
}


PEVENTSOURCE
GetEventSourceByName(LPCWSTR Name)
{
    PLIST_ENTRY CurrentEntry;
    PEVENTSOURCE Result = NULL;

    DPRINT("GetEventSourceByName(%S)\n", Name);
    EnterCriticalSection(&EventSourceListCs);

    CurrentEntry = EventSourceListHead.Flink;
    while (CurrentEntry != &EventSourceListHead)
    {
        PEVENTSOURCE Item = CONTAINING_RECORD(CurrentEntry,
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
