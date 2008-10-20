/*
 * PROJECT:     ReactOS Service Control Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/services/groupdb.c
 * PURPOSE:     Service group control interface
 * COPYRIGHT:   Copyright 2005 Eric Kohl
 *
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS *******************************************************************/

LIST_ENTRY GroupListHead;
LIST_ENTRY UnknownGroupListHead;


/* FUNCTIONS *****************************************************************/

DWORD
ScmSetServiceGroup(PSERVICE lpService,
                   LPWSTR lpGroupName)
{
    PLIST_ENTRY GroupEntry;
    PSERVICE_GROUP lpGroup;

    GroupEntry = GroupListHead.Flink;
    while (GroupEntry != &GroupListHead)
    {
        lpGroup = CONTAINING_RECORD(GroupEntry, SERVICE_GROUP, GroupListEntry);

        if (!_wcsicmp(lpGroup->lpGroupName, lpGroupName))
        {
            lpService->lpGroup = lpGroup;
            return ERROR_SUCCESS;
        }

        GroupEntry = GroupEntry->Flink;
    }

    GroupEntry = UnknownGroupListHead.Flink;
    while (GroupEntry != &UnknownGroupListHead)
    {
        lpGroup = CONTAINING_RECORD(GroupEntry, SERVICE_GROUP, GroupListEntry);

        if (!_wcsicmp(lpGroup->lpGroupName, lpGroupName))
        {
            lpGroup->dwRefCount++;
            lpService->lpGroup = lpGroup;
            return ERROR_SUCCESS;
        }

        GroupEntry = GroupEntry->Flink;
    }

    lpGroup = (PSERVICE_GROUP)HeapAlloc(GetProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        sizeof(SERVICE_GROUP) + ((wcslen(lpGroupName) + 1)* sizeof(WCHAR)));
    if (lpGroup == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    wcscpy(lpGroup->szGroupName, lpGroupName);
    lpGroup->lpGroupName = lpGroup->szGroupName;
    lpGroup->dwRefCount = 1;
    lpService->lpGroup = lpGroup;

    InsertTailList(&UnknownGroupListHead,
                   &lpGroup->GroupListEntry);

    return ERROR_SUCCESS;
}


static NTSTATUS STDCALL
CreateGroupOrderListRoutine(PWSTR ValueName,
                            ULONG ValueType,
                            PVOID ValueData,
                            ULONG ValueLength,
                            PVOID Context,
                            PVOID EntryContext)
{
    PSERVICE_GROUP Group;

    DPRINT("CreateGroupOrderListRoutine(%S, %x, %x, %x, %x, %x)\n",
           ValueName, ValueType, ValueData, ValueLength, Context, EntryContext);

    if (ValueType == REG_BINARY &&
        ValueData != NULL &&
        ValueLength >= sizeof(DWORD) &&
        ValueLength >= (*(PULONG)ValueData + 1) * sizeof(DWORD))
    {
        Group = (PSERVICE_GROUP)Context;
        Group->TagCount = ((PULONG)ValueData)[0];
        if (Group->TagCount > 0)
        {
            if (ValueLength >= (Group->TagCount + 1) * sizeof(DWORD))
            {
                Group->TagArray = (PULONG)HeapAlloc(GetProcessHeap(),
                                                    HEAP_ZERO_MEMORY,
                                                    Group->TagCount * sizeof(DWORD));
                if (Group->TagArray == NULL)
                {
                    Group->TagCount = 0;
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(Group->TagArray,
                              (PULONG)ValueData + 1,
                              Group->TagCount * sizeof(DWORD));
            }
            else
            {
                Group->TagCount = 0;
                return STATUS_UNSUCCESSFUL;
            }
        }
    }

    return STATUS_SUCCESS;
}


static NTSTATUS STDCALL
CreateGroupListRoutine(PWSTR ValueName,
                       ULONG ValueType,
                       PVOID ValueData,
                       ULONG ValueLength,
                       PVOID Context,
                       PVOID EntryContext)
{
    PSERVICE_GROUP Group;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    NTSTATUS Status;

    if (ValueType == REG_SZ)
    {
        DPRINT("Data: '%S'\n", (PWCHAR)ValueData);

        Group = (PSERVICE_GROUP)HeapAlloc(GetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          sizeof(SERVICE_GROUP) + ((wcslen((const wchar_t*) ValueData) + 1) * sizeof(WCHAR)));
        if (Group == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        wcscpy(Group->szGroupName, (const wchar_t*) ValueData);
        Group->lpGroupName = Group->szGroupName;
        Group->dwRefCount = (DWORD)-1;

        RtlZeroMemory(&QueryTable, sizeof(QueryTable));
        QueryTable[0].Name = (PWSTR)ValueData;
        QueryTable[0].QueryRoutine = CreateGroupOrderListRoutine;

        Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                                        L"GroupOrderList",
                                        QueryTable,
                                        (PVOID)Group,
                                        NULL);
        DPRINT("%x %d %S\n", Status, Group->TagCount, (PWSTR)ValueData);

        InsertTailList(&GroupListHead,
                       &Group->GroupListEntry);
    }

    return STATUS_SUCCESS;
}


DWORD
ScmCreateGroupList(VOID)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    NTSTATUS Status;

    InitializeListHead(&GroupListHead);
    InitializeListHead(&UnknownGroupListHead);

    /* Build group order list */
    RtlZeroMemory(&QueryTable,
                  sizeof(QueryTable));

    QueryTable[0].Name = L"List";
    QueryTable[0].QueryRoutine = CreateGroupListRoutine;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                                    L"ServiceGroupOrder",
                                    QueryTable,
                                    NULL,
                                    NULL);

    return RtlNtStatusToDosError(Status);
}

/* EOF */
