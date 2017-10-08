/*
 * PROJECT:     NetAPI DLL
 * LICENSE:     GPL-2.0 (https://spdx.org/licenses/GPL-2.0)
 * PURPOSE:     SAM service interface code
 * COPYRIGHT:   Copyright 2017 Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/* FUNCTIONS *****************************************************************/

/* PUBLIC FUNCTIONS **********************************************************/

NET_API_STATUS
WINAPI
NetQueryDisplayInformation(
    _In_ LPCWSTR ServerName,
    _In_ DWORD Level,
    _In_ DWORD Index,
    _In_ DWORD EntriesRequested,
    _In_ DWORD PreferredMaximumLength,
    _Out_ LPDWORD ReturnedEntryCount,
    _Out_ PVOID *SortedBuffer)
{
    UNICODE_STRING ServerNameString;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    DOMAIN_DISPLAY_INFORMATION DisplayInformation;
    DWORD LocalTotalBytesAvailable;
    DWORD LocalTotalBytesReturned;
    DWORD LocalReturnedEntryCount;
    PVOID LocalSortedBuffer;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status;

    TRACE("NetQueryDisplayInformation(%s, %ld, %ld, %ld, %ld, %p, %p)\n",
          debugstr_w(ServerName), Level, Index, EntriesRequested,
          PreferredMaximumLength, ReturnedEntryCount, SortedBuffer);

    *ReturnedEntryCount = 0;
    *SortedBuffer = NULL;

    switch (Level)
    {
        case 1:
            DisplayInformation = DomainDisplayUser;
            break;

        case 2:
            DisplayInformation = DomainDisplayMachine;
            break;

        case 3:
            DisplayInformation = DomainDisplayGroup;
            break;

        default:
            return ERROR_INVALID_LEVEL;
    }

    if (ServerName != NULL)
        RtlInitUnicodeString(&ServerNameString, ServerName);

    /* Connect to the SAM Server */
    Status = SamConnect((ServerName != NULL) ? &ServerNameString : NULL,
                        &ServerHandle,
                        SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamConnect failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Open the Account Domain */
    Status = OpenAccountDomain(ServerHandle,
                               (ServerName != NULL) ? &ServerNameString : NULL,
                               DOMAIN_LIST_ACCOUNTS,
                               &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("OpenAccountDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Query the information */
    Status = SamQueryDisplayInformation(DomainHandle,
                                        DisplayInformation,
                                        Index,
                                        EntriesRequested,
                                        PreferredMaximumLength,
                                        &LocalTotalBytesAvailable,
                                        &LocalTotalBytesReturned,
                                        &LocalReturnedEntryCount,
                                        &LocalSortedBuffer);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamQueryDisplayInformation failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* FIXME */

done:
    if (DomainHandle != NULL)
        SamCloseHandle(DomainHandle);

    if (ServerHandle != NULL)
        SamCloseHandle(ServerHandle);

    return ApiStatus;
}


NET_API_STATUS
WINAPI
NetGetDisplayInformationIndex(
    _In_ LPCWSTR ServerName,
    _In_ DWORD Level,
    _In_ LPCWSTR Prefix,
    _Out_ LPDWORD Index)
{
    UNICODE_STRING ServerNameString, PrefixString;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    DOMAIN_DISPLAY_INFORMATION DisplayInformation;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status;

    TRACE("NetGetDisplayInformationIndex(%s %ld %s %p)\n",
          debugstr_w(ServerName), Level, debugstr_w(Prefix), Index);

    switch (Level)
    {
        case 1:
            DisplayInformation = DomainDisplayUser;
            break;

        case 2:
            DisplayInformation = DomainDisplayMachine;
            break;

        case 3:
            DisplayInformation = DomainDisplayGroup;
            break;

        default:
            return ERROR_INVALID_LEVEL;
    }

    if (ServerName != NULL)
        RtlInitUnicodeString(&ServerNameString, ServerName);

    /* Connect to the SAM Server */
    Status = SamConnect((ServerName != NULL) ? &ServerNameString : NULL,
                        &ServerHandle,
                        SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamConnect failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Open the Account Domain */
    Status = OpenAccountDomain(ServerHandle,
                               (ServerName != NULL) ? &ServerNameString : NULL,
                               DOMAIN_LIST_ACCOUNTS,
                               &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("OpenAccountDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    RtlInitUnicodeString(&PrefixString, Prefix);

    /* Get the index */
    Status = SamGetDisplayEnumerationIndex(DomainHandle,
                                           DisplayInformation,
                                           &PrefixString,
                                           Index);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamGetDisplayEnumerationIndex failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
    }

done:
    if (DomainHandle != NULL)
        SamCloseHandle(DomainHandle);

    if (ServerHandle != NULL)
        SamCloseHandle(ServerHandle);

    return ApiStatus;
}

/* EOF */
