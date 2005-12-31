/*
 * groupdb.c
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS *******************************************************************/

LIST_ENTRY GroupListHead;


/* FUNCTIONS *****************************************************************/

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
                                          sizeof(SERVICE_GROUP) + (wcslen(ValueData) * sizeof(WCHAR)));
        if (Group == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        wcscpy(Group->szGroupName, ValueData);
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
