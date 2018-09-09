/*
 * PROJECT:     ReactOS NetAPI DLL
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     SAM service group interface code
 * COPYRIGHT:   Copyright 2018 Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

typedef enum _ENUM_PHASE
{
    BuiltinPhase,
    AccountPhase,
    DonePhase
} ENUM_PHASE;

typedef struct _ENUM_CONTEXT
{
    SAM_HANDLE ServerHandle;
    SAM_HANDLE DomainHandle;
    SAM_HANDLE BuiltinDomainHandle;
    SAM_HANDLE AccountDomainHandle;

    SAM_ENUMERATE_HANDLE EnumerationContext;
    PSAM_RID_ENUMERATION Buffer;
    ULONG Returned;
    ULONG Index;
    ENUM_PHASE Phase;
} ENUM_CONTEXT, *PENUM_CONTEXT;


/* FUNCTIONS *****************************************************************/

static
NET_API_STATUS
BuildGroupInfoBuffer(
    _In_ PGROUP_GENERAL_INFORMATION GroupInfo,
    _In_ DWORD Level,
    _In_ DWORD GroupId,
    _Out_ LPVOID *Buffer)
{
    PVOID GroupBuffer = NULL;
    PGROUP_INFO_0 GroupInfo0;
    PGROUP_INFO_1 GroupInfo1;
    PGROUP_INFO_2 GroupInfo2;
    PGROUP_INFO_3 GroupInfo3;
    PWSTR Ptr;
    ULONG Size = 0;
    NET_API_STATUS ApiStatus = NERR_Success;

    *Buffer = NULL;

    switch (Level)
    {
        case 0:
            Size = sizeof(GROUP_INFO_0) +
                   GroupInfo->Name.Length + sizeof(WCHAR);
            break;

        case 1:
            Size = sizeof(GROUP_INFO_1) +
                   GroupInfo->Name.Length + sizeof(WCHAR) +
                   GroupInfo->AdminComment.Length + sizeof(WCHAR);
            break;

        case 2:
            Size = sizeof(GROUP_INFO_2) +
                   GroupInfo->Name.Length + sizeof(WCHAR) +
                   GroupInfo->AdminComment.Length + sizeof(WCHAR);
            break;

        case 3:
            Size = sizeof(GROUP_INFO_3) +
                   GroupInfo->Name.Length + sizeof(WCHAR) +
                   GroupInfo->AdminComment.Length + sizeof(WCHAR);
                   /* FIXME: Sid size */
            break;

        default:
            ApiStatus = ERROR_INVALID_LEVEL;
            goto done;
    }

    ApiStatus = NetApiBufferAllocate(Size, &GroupBuffer);
    if (ApiStatus != NERR_Success)
        goto done;

    ZeroMemory(GroupBuffer, Size);

    switch (Level)
    {
        case 0:
            GroupInfo0 = (PGROUP_INFO_0)GroupBuffer;

            Ptr = (PWSTR)((ULONG_PTR)GroupInfo0 + sizeof(LOCALGROUP_INFO_0));
            GroupInfo0->grpi0_name = Ptr;

            memcpy(GroupInfo0->grpi0_name,
                   GroupInfo->Name.Buffer,
                   GroupInfo->Name.Length);
            GroupInfo0->grpi0_name[GroupInfo->Name.Length / sizeof(WCHAR)] = UNICODE_NULL;
            break;

        case 1:
            GroupInfo1 = (PGROUP_INFO_1)GroupBuffer;

            Ptr = (PWSTR)((ULONG_PTR)GroupInfo1 + sizeof(GROUP_INFO_1));
            GroupInfo1->grpi1_name = Ptr;

            memcpy(GroupInfo1->grpi1_name,
                   GroupInfo->Name.Buffer,
                   GroupInfo->Name.Length);
            GroupInfo1->grpi1_name[GroupInfo->Name.Length / sizeof(WCHAR)] = UNICODE_NULL;

            Ptr = (PWSTR)((ULONG_PTR)Ptr + GroupInfo->Name.Length + sizeof(WCHAR));
            GroupInfo1->grpi1_comment = Ptr;

            memcpy(GroupInfo1->grpi1_comment,
                   GroupInfo->AdminComment.Buffer,
                   GroupInfo->AdminComment.Length);
            GroupInfo1->grpi1_comment[GroupInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;
            break;

        case 2:
            GroupInfo2 = (PGROUP_INFO_2)GroupBuffer;

            Ptr = (PWSTR)((ULONG_PTR)GroupInfo2 + sizeof(GROUP_INFO_2));
            GroupInfo2->grpi2_name = Ptr;

            memcpy(GroupInfo2->grpi2_name,
                   GroupInfo->Name.Buffer,
                   GroupInfo->Name.Length);
            GroupInfo2->grpi2_name[GroupInfo->Name.Length / sizeof(WCHAR)] = UNICODE_NULL;

            Ptr = (PWSTR)((ULONG_PTR)Ptr + GroupInfo->Name.Length + sizeof(WCHAR));
            GroupInfo2->grpi2_comment = Ptr;

            memcpy(GroupInfo2->grpi2_comment,
                   GroupInfo->AdminComment.Buffer,
                   GroupInfo->AdminComment.Length);
            GroupInfo2->grpi2_comment[GroupInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;

            GroupInfo2->grpi2_group_id = GroupId;

            GroupInfo2->grpi2_attributes= GroupInfo->Attributes;
            break;

        case 3:
            GroupInfo3 = (PGROUP_INFO_3)GroupBuffer;

            Ptr = (PWSTR)((ULONG_PTR)GroupInfo3 + sizeof(GROUP_INFO_3));
            GroupInfo3->grpi3_name = Ptr;

            memcpy(GroupInfo3->grpi3_name,
                   GroupInfo->Name.Buffer,
                   GroupInfo->Name.Length);
            GroupInfo3->grpi3_name[GroupInfo->Name.Length / sizeof(WCHAR)] = UNICODE_NULL;

            Ptr = (PWSTR)((ULONG_PTR)Ptr + GroupInfo->Name.Length + sizeof(WCHAR));
            GroupInfo3->grpi3_comment = Ptr;

            memcpy(GroupInfo3->grpi3_comment,
                   GroupInfo->AdminComment.Buffer,
                   GroupInfo->AdminComment.Length);
            GroupInfo3->grpi3_comment[GroupInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;

            GroupInfo3->grpi3_group_sid = NULL; /* FIXME */

            GroupInfo3->grpi3_attributes= GroupInfo->Attributes;
            break;
    }

done:
    if (ApiStatus == NERR_Success)
    {
        *Buffer = GroupBuffer;
    }
    else
    {
        if (GroupBuffer != NULL)
            NetApiBufferFree(GroupBuffer);
    }

    return ApiStatus;
}


static
VOID
FreeGroupInfo(
    _In_ PGROUP_GENERAL_INFORMATION GroupInfo)
{
    if (GroupInfo->Name.Buffer != NULL)
        SamFreeMemory(GroupInfo->Name.Buffer);

    if (GroupInfo->AdminComment.Buffer != NULL)
        SamFreeMemory(GroupInfo->AdminComment.Buffer);

    SamFreeMemory(GroupInfo);
}


static
NET_API_STATUS
OpenGroupByName(
    _In_ SAM_HANDLE DomainHandle,
    _In_ PUNICODE_STRING GroupName,
    _In_ ULONG DesiredAccess,
    _Out_ PSAM_HANDLE GroupHandle,
    _Out_ PULONG RelativeId)
{
    PULONG RelativeIds = NULL;
    PSID_NAME_USE Use = NULL;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Get the RID for the given user name */
    Status = SamLookupNamesInDomain(DomainHandle,
                                    1,
                                    GroupName,
                                    &RelativeIds,
                                    &Use);
    if (!NT_SUCCESS(Status))
    {
        WARN("SamLookupNamesInDomain failed (Status %08lx)\n", Status);
        return NetpNtStatusToApiStatus(Status);
    }

    /* Fail, if it is not an alias account */
    if (Use[0] != SidTypeGroup)
    {
        ERR("Object is not a group!\n");
        ApiStatus = NERR_GroupNotFound;
        goto done;
    }

    /* Open the alias account */
    Status = SamOpenGroup(DomainHandle,
                          DesiredAccess,
                          RelativeIds[0],
                          GroupHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamOpenGroup failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    if (RelativeId != NULL)
        *RelativeId = RelativeIds[0];

done:
    if (RelativeIds != NULL)
        SamFreeMemory(RelativeIds);

    if (Use != NULL)
        SamFreeMemory(Use);

    return ApiStatus;
}


/* PUBLIC FUNCTIONS **********************************************************/

NET_API_STATUS
WINAPI
NetGroupAdd(
    _In_opt_ LPCWSTR servername,
    _In_ DWORD level,
    _In_ LPBYTE buf,
    _Out_opt_ LPDWORD parm_err)
{
    GROUP_ADM_COMMENT_INFORMATION AdminComment;
    GROUP_ATTRIBUTE_INFORMATION AttributeInfo;
    UNICODE_STRING ServerName;
    UNICODE_STRING GroupName;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE GroupHandle = NULL;
    PWSTR Name = NULL;
    PWSTR Comment = NULL;
    ULONG Attributes = 0;
    ULONG RelativeId;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("NetGroupAdd(%s, %d, %p, %p)\n",
          debugstr_w(servername), level, buf, parm_err);

    /* Initialize the Server name*/
    if (servername != NULL)
        RtlInitUnicodeString(&ServerName, servername);

    /* Initialize the Alias name*/
    switch (level)
    {
        case 0:
            Name = ((PGROUP_INFO_0)buf)->grpi0_name;
            Comment = NULL;
            Attributes = 0;
            break;

        case 1:
            Name = ((PGROUP_INFO_1)buf)->grpi1_name;
            Comment = ((PGROUP_INFO_1)buf)->grpi1_comment;
            Attributes = 0;
            break;

        case 2:
            Name = ((PGROUP_INFO_2)buf)->grpi2_name;
            Comment = ((PGROUP_INFO_2)buf)->grpi2_comment;
            Attributes = ((PGROUP_INFO_2)buf)->grpi2_attributes;
            break;

        case 3:
            Name = ((PGROUP_INFO_3)buf)->grpi3_name;
            Comment = ((PGROUP_INFO_3)buf)->grpi3_comment;
            Attributes = ((PGROUP_INFO_3)buf)->grpi3_attributes;
            break;

        default:
            return ERROR_INVALID_LEVEL;
    }

    RtlInitUnicodeString(&GroupName, Name);

    /* Connect to the SAM Server */
    Status = SamConnect((servername != NULL) ? &ServerName : NULL,
                        &ServerHandle,
                        SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamConnect failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Open the account domain */
    Status = OpenAccountDomain(ServerHandle,
                               (servername != NULL) ? &ServerName : NULL,
                               DOMAIN_CREATE_GROUP | DOMAIN_LOOKUP,
                               &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamOpenDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Try to open the group account */
    ApiStatus = OpenGroupByName(DomainHandle,
                                &GroupName,
                                GROUP_READ_INFORMATION,
                                &GroupHandle,
                                NULL);
    if (ApiStatus == NERR_Success)
    {
        ERR("OpenGroupByName: Group %wZ already exists!\n", &GroupName);

        SamCloseHandle(GroupHandle);
        ApiStatus = ERROR_GROUP_EXISTS;
        goto done;
    }

    ApiStatus = NERR_Success;

    /* Create the group */
    Status = SamCreateGroupInDomain(DomainHandle,
                                    &GroupName,
                                    DELETE | GROUP_WRITE_ACCOUNT,
                                    &GroupHandle,
                                    &RelativeId);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamCreateGroupInDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    TRACE("Created group \"%wZ\" (RID: %lu)\n", &GroupName, RelativeId);

    /* Set the admin comment */
    if (level == 1 || level == 2 || level == 3)
    {
        RtlInitUnicodeString(&AdminComment.AdminComment, Comment);

        Status = SamSetInformationGroup(GroupHandle,
                                        GroupAdminCommentInformation,
                                        &AdminComment);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamSetInformationAlias failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);

            /* Delete the Alias if the Comment could not be set */
            SamDeleteGroup(GroupHandle);

            goto done;
        }
    }

    /* Set the attributes */
    if (level == 2 || level == 3)
    {
        AttributeInfo.Attributes = Attributes;

        Status = SamSetInformationGroup(GroupHandle,
                                        GroupAttributeInformation,
                                        &AttributeInfo);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamSetInformationAlias failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);

            /* Delete the Alias if the Attributes could not be set */
            SamDeleteGroup(GroupHandle);

            goto done;
        }
    }

done:
    if (GroupHandle != NULL)
    {
        if (ApiStatus != NERR_Success)
            SamDeleteGroup(GroupHandle);
        else
            SamCloseHandle(GroupHandle);
    }

    if (DomainHandle != NULL)
        SamCloseHandle(DomainHandle);

    if (ServerHandle != NULL)
        SamCloseHandle(ServerHandle);

    return ApiStatus;
}


NET_API_STATUS
WINAPI
NetGroupDel(
    _In_opt_ LPCWSTR servername,
    _In_ IN LPCWSTR groupname)
{
    UNICODE_STRING ServerName;
    UNICODE_STRING GroupName;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE GroupHandle = NULL;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("NetGroupDel(%s, %s)\n",
          debugstr_w(servername), debugstr_w(groupname));

    if (servername != NULL)
        RtlInitUnicodeString(&ServerName, servername);

    RtlInitUnicodeString(&GroupName, groupname);

    /* Connect to the SAM Server */
    Status = SamConnect((servername != NULL) ? &ServerName : NULL,
                        &ServerHandle,
                        SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamConnect failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Open the Acount Domain */
    Status = OpenAccountDomain(ServerHandle,
                               (servername != NULL) ? &ServerName : NULL,
                               DOMAIN_LOOKUP,
                               &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("OpenAccountDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Open the group */
    ApiStatus = OpenGroupByName(DomainHandle,
                                &GroupName,
                                DELETE,
                                &GroupHandle,
                                NULL);
    if (ApiStatus != NERR_Success)
    {
        ERR("OpenGroupByName failed (ApiStatus %lu)\n", ApiStatus);
        if (ApiStatus == ERROR_NONE_MAPPED)
            ApiStatus = NERR_GroupNotFound;
        goto done;
    }

    /* Delete the group */
    Status = SamDeleteGroup(GroupHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamDeleteGroup failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

done:
    if (GroupHandle != NULL)
        SamCloseHandle(GroupHandle);

    if (DomainHandle != NULL)
        SamCloseHandle(DomainHandle);

    if (ServerHandle != NULL)
        SamCloseHandle(ServerHandle);

    return ApiStatus;
}


NET_API_STATUS
WINAPI
NetGroupEnum(
    _In_opt_ LPCWSTR servername,
    _In_ DWORD level,
    _Out_ LPBYTE *bufptr,
    _In_ DWORD prefmaxlen,
    _Out_ LPDWORD entriesread,
    _Out_ LPDWORD totalentries,
    _Inout_opt_ PDWORD_PTR resume_handle)
{
    UNICODE_STRING ServerName;
    PSAM_RID_ENUMERATION CurrentGroup;
    PENUM_CONTEXT EnumContext = NULL;
    ULONG i;
    SAM_HANDLE GroupHandle = NULL;
    PGROUP_GENERAL_INFORMATION GroupInfo = NULL;
    PVOID Buffer = NULL;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("NetGroupEnum(%s, %d, %p, %d, %p, %p, %p)\n", debugstr_w(servername),
          level, bufptr, prefmaxlen, entriesread, totalentries, resume_handle);

    *entriesread = 0;
    *totalentries = 0;
    *bufptr = NULL;

    if (servername != NULL)
        RtlInitUnicodeString(&ServerName, servername);

    if (resume_handle != NULL && *resume_handle != 0)
    {
        EnumContext = (PENUM_CONTEXT)*resume_handle;
    }
    else
    {
        ApiStatus = NetApiBufferAllocate(sizeof(ENUM_CONTEXT), (PVOID*)&EnumContext);
        if (ApiStatus != NERR_Success)
            goto done;

        EnumContext->EnumerationContext = 0;
        EnumContext->Buffer = NULL;
        EnumContext->Returned = 0;
        EnumContext->Index = 0;

        Status = SamConnect((servername != NULL) ? &ServerName : NULL,
                            &EnumContext->ServerHandle,
                            SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                            NULL);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamConnect failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        Status = OpenAccountDomain(EnumContext->ServerHandle,
                                   (servername != NULL) ? &ServerName : NULL,
                                   DOMAIN_LIST_ACCOUNTS | DOMAIN_LOOKUP,
                                   &EnumContext->AccountDomainHandle);
        if (!NT_SUCCESS(Status))
        {
            ERR("OpenAccountDomain failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        Status = OpenBuiltinDomain(EnumContext->ServerHandle,
                                   DOMAIN_LIST_ACCOUNTS | DOMAIN_LOOKUP,
                                   &EnumContext->BuiltinDomainHandle);
        if (!NT_SUCCESS(Status))
        {
            ERR("OpenBuiltinDomain failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        EnumContext->Phase = AccountPhase; //BuiltinPhase;
        EnumContext->DomainHandle = EnumContext->AccountDomainHandle; //BuiltinDomainHandle;
    }


//    while (TRUE)
//    {
        TRACE("EnumContext->Index: %lu\n", EnumContext->Index);
        TRACE("EnumContext->Returned: %lu\n", EnumContext->Returned);

        if (EnumContext->Index >= EnumContext->Returned)
        {
            TRACE("Calling SamEnumerateGroupsInDomain\n");

            Status = SamEnumerateGroupsInDomain(EnumContext->DomainHandle,
                                                &EnumContext->EnumerationContext,
                                                (PVOID *)&EnumContext->Buffer,
                                                prefmaxlen,
                                                &EnumContext->Returned);

            TRACE("SamEnumerateGroupsInDomain returned (Status %08lx)\n", Status);
            if (!NT_SUCCESS(Status))
            {
                ERR("SamEnumerateAliasesInDomain failed (Status %08lx)\n", Status);
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }

            if (Status == STATUS_MORE_ENTRIES)
            {
                ApiStatus = NERR_BufTooSmall;
                goto done;
            }
        }

        TRACE("EnumContext: %lu\n", EnumContext);
        TRACE("EnumContext->Returned: %lu\n", EnumContext->Returned);
        TRACE("EnumContext->Buffer: %p\n", EnumContext->Buffer);

        /* Get a pointer to the current group */
        CurrentGroup = &EnumContext->Buffer[EnumContext->Index];

        TRACE("RID: %lu\n", CurrentGroup->RelativeId);

        Status = SamOpenGroup(EnumContext->DomainHandle,
                              GROUP_READ_INFORMATION,
                              CurrentGroup->RelativeId,
                              &GroupHandle);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamOpenGroup failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        Status = SamQueryInformationGroup(GroupHandle,
                                          GroupGeneralInformation,
                                          (PVOID *)&GroupInfo);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamQueryInformationGroup failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        SamCloseHandle(GroupHandle);
        GroupHandle = NULL;

        TRACE("Name: %S\n", GroupInfo->Name.Buffer);
        TRACE("Comment: %S\n", GroupInfo->AdminComment.Buffer);

        ApiStatus = BuildGroupInfoBuffer(GroupInfo,
                                         level,
                                         CurrentGroup->RelativeId,
                                         &Buffer);
        if (ApiStatus != NERR_Success)
            goto done;

        if (GroupInfo != NULL)
        {
            FreeGroupInfo(GroupInfo);
            GroupInfo = NULL;
        }

        EnumContext->Index++;

        (*entriesread)++;

        if (EnumContext->Index == EnumContext->Returned)
        {
            switch (EnumContext->Phase)
            {
                case BuiltinPhase:
                    EnumContext->Phase = AccountPhase;
                    EnumContext->DomainHandle = EnumContext->AccountDomainHandle;
                    EnumContext->EnumerationContext = 0;
                    EnumContext->Index = 0;
                    EnumContext->Returned = 0;

                    if (EnumContext->Buffer != NULL)
                    {
                        for (i = 0; i < EnumContext->Returned; i++)
                        {
                            SamFreeMemory(EnumContext->Buffer[i].Name.Buffer);
                        }

                        SamFreeMemory(EnumContext->Buffer);
                        EnumContext->Buffer = NULL;
                    }
                    break;

                case AccountPhase:
                case DonePhase:
                    EnumContext->Phase = DonePhase;
                    break;
            }
        }
//    }

done:
    if (ApiStatus == NERR_Success && EnumContext != NULL && EnumContext->Phase != DonePhase)
        ApiStatus = ERROR_MORE_DATA;

    if (EnumContext != NULL)
        *totalentries = EnumContext->Returned;

    if (resume_handle == NULL || ApiStatus != ERROR_MORE_DATA)
    {
        if (EnumContext != NULL)
        {
            if (EnumContext->BuiltinDomainHandle != NULL)
                SamCloseHandle(EnumContext->BuiltinDomainHandle);

            if (EnumContext->AccountDomainHandle != NULL)
                SamCloseHandle(EnumContext->AccountDomainHandle);

            if (EnumContext->ServerHandle != NULL)
                SamCloseHandle(EnumContext->ServerHandle);

            if (EnumContext->Buffer != NULL)
            {
                for (i = 0; i < EnumContext->Returned; i++)
                {
                    SamFreeMemory(EnumContext->Buffer[i].Name.Buffer);
                }

                SamFreeMemory(EnumContext->Buffer);
            }

            NetApiBufferFree(EnumContext);
            EnumContext = NULL;
        }
    }

    if (GroupHandle != NULL)
        SamCloseHandle(GroupHandle);

    if (GroupInfo != NULL)
        FreeGroupInfo(GroupInfo);

    if (resume_handle != NULL)
        *resume_handle = (DWORD_PTR)EnumContext;

    *bufptr = (LPBYTE)Buffer;

    TRACE("return %lu\n", ApiStatus);

    return ApiStatus;
}


NET_API_STATUS
WINAPI
NetGroupGetInfo(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR groupname,
    _In_ DWORD level,
    _Out_ LPBYTE *bufptr)
{
    UNICODE_STRING ServerName;
    UNICODE_STRING GroupName;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE GroupHandle = NULL;
    PGROUP_GENERAL_INFORMATION GroupInfo = NULL;
    PVOID Buffer = NULL;
    ULONG RelativeId;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("NetGroupGetInfo(%s, %s, %d, %p)\n",
          debugstr_w(servername), debugstr_w(groupname), level, bufptr);

    if (servername != NULL)
        RtlInitUnicodeString(&ServerName, servername);

    RtlInitUnicodeString(&GroupName, groupname);

    /* Connect to the SAM Server */
    Status = SamConnect((servername != NULL) ? &ServerName : NULL,
                        &ServerHandle,
                        SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamConnect failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Open the Acount Domain */
    Status = OpenAccountDomain(ServerHandle,
                               (servername != NULL) ? &ServerName : NULL,
                               DOMAIN_LOOKUP,
                               &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("OpenAccountDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Open the group account in the account domain */
    ApiStatus = OpenGroupByName(DomainHandle,
                                &GroupName,
                                GROUP_READ_INFORMATION,
                                &GroupHandle,
                                &RelativeId);
    if (ApiStatus != NERR_Success)
    {
        ERR("OpenGroupByName failed (ApiStatus %lu)\n", ApiStatus);
        if (ApiStatus == ERROR_NONE_MAPPED)
            ApiStatus = NERR_GroupNotFound;
        goto done;
    }

    Status = SamQueryInformationGroup(GroupHandle,
                                      GroupGeneralInformation,
                                      (PVOID *)&GroupInfo);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamQueryInformationGroup failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    ApiStatus = BuildGroupInfoBuffer(GroupInfo,
                                     level,
                                     RelativeId,
                                     &Buffer);
    if (ApiStatus != NERR_Success)
        goto done;

done:
    if (GroupInfo != NULL)
        FreeGroupInfo(GroupInfo);

    if (GroupHandle != NULL)
        SamCloseHandle(GroupHandle);

    if (DomainHandle != NULL)
        SamCloseHandle(DomainHandle);

    if (ServerHandle != NULL)
        SamCloseHandle(ServerHandle);

    *bufptr = (LPBYTE)Buffer;

    return ApiStatus;
}


NET_API_STATUS
WINAPI
NetGroupSetInfo(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR groupname,
    _In_ DWORD level,
    _In_ LPBYTE buf,
    _Out_opt_ LPDWORD parm_err)
{
    UNICODE_STRING ServerName;
    UNICODE_STRING GroupName;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE GroupHandle = NULL;
    GROUP_NAME_INFORMATION GroupNameInfo;
    GROUP_ADM_COMMENT_INFORMATION AdminCommentInfo;
    GROUP_ATTRIBUTE_INFORMATION AttributeInfo;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("NetGroupSetInfo(%s, %s, %d, %p, %p)\n",
          debugstr_w(servername), debugstr_w(groupname), level, buf, parm_err);

    if (parm_err != NULL)
        *parm_err = PARM_ERROR_NONE;

    if (servername != NULL)
        RtlInitUnicodeString(&ServerName, servername);

    RtlInitUnicodeString(&GroupName, groupname);

    /* Connect to the SAM Server */
    Status = SamConnect((servername != NULL) ? &ServerName : NULL,
                        &ServerHandle,
                        SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamConnect failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Open the Acount Domain */
    Status = OpenAccountDomain(ServerHandle,
                               (servername != NULL) ? &ServerName : NULL,
                               DOMAIN_LOOKUP,
                               &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("OpenAccountDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Open the group */
    ApiStatus = OpenGroupByName(DomainHandle,
                                &GroupName,
                                GROUP_WRITE_ACCOUNT,
                                &GroupHandle,
                                NULL);
    if (ApiStatus != NERR_Success)
    {
        WARN("OpenGroupByName failed (ApiStatus %lu)\n", ApiStatus);
        if (ApiStatus == ERROR_NONE_MAPPED)
            ApiStatus = NERR_GroupNotFound;
        goto done;
    }

    switch (level)
    {
        case 0:
            /* Set the group name */
            RtlInitUnicodeString(&GroupNameInfo.Name,
                                 ((PGROUP_INFO_0)buf)->grpi0_name);

            Status = SamSetInformationGroup(GroupHandle,
                                            GroupNameInformation,
                                            &GroupNameInfo);
            if (!NT_SUCCESS(Status))
            {
                ERR("SamSetInformationGroup failed (ApiStatus %lu)\n", ApiStatus);
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }
            break;

        case 1:
            /* Set the group name */
            RtlInitUnicodeString(&GroupNameInfo.Name,
                                 ((PGROUP_INFO_1)buf)->grpi1_name);

            Status = SamSetInformationGroup(GroupHandle,
                                            GroupNameInformation,
                                            &GroupNameInfo);
            if (!NT_SUCCESS(Status))
            {
                ERR("SamSetInformationGroup failed (ApiStatus %lu)\n", ApiStatus);
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }

            /* Set the admin comment */
            RtlInitUnicodeString(&AdminCommentInfo.AdminComment,
                                 ((PGROUP_INFO_1)buf)->grpi1_comment);

            Status = SamSetInformationGroup(GroupHandle,
                                            GroupAdminCommentInformation,
                                            &AdminCommentInfo);
            if (!NT_SUCCESS(Status))
            {
                ERR("SamSetInformationGroup failed (ApiStatus %lu)\n", ApiStatus);
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }
            break;

        case 2:
            /* Set the group name */
            RtlInitUnicodeString(&GroupNameInfo.Name,
                                 ((PGROUP_INFO_2)buf)->grpi2_name);

            Status = SamSetInformationGroup(GroupHandle,
                                            GroupNameInformation,
                                            &GroupNameInfo);
            if (!NT_SUCCESS(Status))
            {
                ERR("SamSetInformationGroup failed (ApiStatus %lu)\n", ApiStatus);
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }

            /* Set the admin comment */
            RtlInitUnicodeString(&AdminCommentInfo.AdminComment,
                                 ((PGROUP_INFO_2)buf)->grpi2_comment);

            Status = SamSetInformationGroup(GroupHandle,
                                            GroupAdminCommentInformation,
                                            &AdminCommentInfo);
            if (!NT_SUCCESS(Status))
            {
                ERR("SamSetInformationGroup failed (ApiStatus %lu)\n", ApiStatus);
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }

            /* Set the attributes */
            AttributeInfo.Attributes = ((PGROUP_INFO_2)buf)->grpi2_attributes;

            Status = SamSetInformationGroup(GroupHandle,
                                            GroupAttributeInformation,
                                            &AttributeInfo);
            if (!NT_SUCCESS(Status))
            {
                ERR("SamSetInformationGroup failed (ApiStatus %lu)\n", ApiStatus);
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }
            break;

        case 3:
            /* Set the group name */
            RtlInitUnicodeString(&GroupNameInfo.Name,
                                 ((PGROUP_INFO_3)buf)->grpi3_name);

            Status = SamSetInformationGroup(GroupHandle,
                                            GroupNameInformation,
                                            &GroupNameInfo);
            if (!NT_SUCCESS(Status))
            {
                ERR("SamSetInformationGroup failed (ApiStatus %lu)\n", ApiStatus);
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }

            /* Set the admin comment */
            RtlInitUnicodeString(&AdminCommentInfo.AdminComment,
                                 ((PGROUP_INFO_3)buf)->grpi3_comment);

            Status = SamSetInformationGroup(GroupHandle,
                                            GroupAdminCommentInformation,
                                            &AdminCommentInfo);
            if (!NT_SUCCESS(Status))
            {
                ERR("SamSetInformationGroup failed (ApiStatus %lu)\n", ApiStatus);
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }

            /* Set the attributes */
            AttributeInfo.Attributes = ((PGROUP_INFO_3)buf)->grpi3_attributes;

            Status = SamSetInformationGroup(GroupHandle,
                                            GroupAttributeInformation,
                                            &AttributeInfo);
            if (!NT_SUCCESS(Status))
            {
                ERR("SamSetInformationGroup failed (ApiStatus %lu)\n", ApiStatus);
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }
            break;

        case 1002:
            /* Set the admin comment */
            RtlInitUnicodeString(&AdminCommentInfo.AdminComment,
                                 ((PGROUP_INFO_1002)buf)->grpi1002_comment);

            Status = SamSetInformationGroup(GroupHandle,
                                            GroupAdminCommentInformation,
                                            &AdminCommentInfo);
            if (!NT_SUCCESS(Status))
            {
                ERR("SamSetInformationGroup failed (ApiStatus %lu)\n", ApiStatus);
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }
            break;

        case 1005:
            /* Set the attributes */
            AttributeInfo.Attributes = ((PGROUP_INFO_1005)buf)->grpi1005_attributes;

            Status = SamSetInformationGroup(GroupHandle,
                                            GroupAttributeInformation,
                                            &AttributeInfo);
            if (!NT_SUCCESS(Status))
            {
                ERR("SamSetInformationGroup failed (ApiStatus %lu)\n", ApiStatus);
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }
            break;

        default:
            ApiStatus = ERROR_INVALID_LEVEL;
            goto done;
    }

done:
    if (GroupHandle != NULL)
        SamCloseHandle(GroupHandle);

    if (DomainHandle != NULL)
        SamCloseHandle(DomainHandle);

    if (ServerHandle != NULL)
        SamCloseHandle(ServerHandle);

    return ApiStatus;
}

/* EOF */
