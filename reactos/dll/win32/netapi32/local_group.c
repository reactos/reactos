/*
 * Copyright 2006 Robert Reif
 *
 * netapi32 local group functions
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "lmcons.h"
#include "lmaccess.h"
#include "lmapibuf.h"
#include "lmerr.h"
#include "winreg.h"
#include "ntsecapi.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include "ntsam.h"
#include "netapi32.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);


typedef struct _ENUM_CONTEXT
{
    SAM_HANDLE ServerHandle;
    SAM_HANDLE BuiltinDomainHandle;
    SAM_HANDLE AccountDomainHandle;

    SAM_ENUMERATE_HANDLE EnumerationContext;
    PSAM_RID_ENUMERATION Buffer;
    ULONG Returned;
    ULONG Index;
    BOOLEAN BuiltinDone;

} ENUM_CONTEXT, *PENUM_CONTEXT;


/************************************************************
 *                NetLocalGroupAdd  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupAdd(
    LPCWSTR servername,
    DWORD level,
    LPBYTE buf,
    LPDWORD parm_err)
{
    FIXME("(%s %d %p %p) stub!\n", debugstr_w(servername), level, buf,
          parm_err);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupAddMember  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupAddMember(
    LPCWSTR servername,
    LPCWSTR groupname,
    PSID membersid)
{
    FIXME("(%s %s %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), membersid);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupAddMembers  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupAddMembers(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    DWORD totalentries)
{
    FIXME("(%s %s %d %p %d) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, totalentries);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupDel  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupDel(
    LPCWSTR servername,
    LPCWSTR groupname)
{
    FIXME("(%s %s) stub!\n", debugstr_w(servername), debugstr_w(groupname));
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupDelMember  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupDelMember(
    LPCWSTR servername,
    LPCWSTR groupname,
    PSID membersid)
{
    FIXME("(%s %s %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), membersid);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupDelMembers  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupDelMembers(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    DWORD totalentries)
{
    FIXME("(%s %s %d %p %d) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, totalentries);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupEnum  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupEnum(
    LPCWSTR servername,
    DWORD level,
    LPBYTE* bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    PDWORD_PTR resumehandle)
{
    UNICODE_STRING ServerName;
    PSAM_RID_ENUMERATION CurrentAlias;
    PENUM_CONTEXT EnumContext = NULL;
    PSID DomainSid = NULL;
    PLOCALGROUP_INFO_0 LocalInfo0;
    PLOCALGROUP_INFO_1 LocalInfo1;
    LPWSTR Ptr;
    ULONG i;
    ULONG Size;
    SAM_HANDLE AliasHandle = NULL;
    PALIAS_GENERAL_INFORMATION AliasInfo = NULL;
    LPVOID Buffer = NULL;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    FIXME("(%s %d %p %d %p %p %p) stub!\n", debugstr_w(servername),
          level, bufptr, prefmaxlen, entriesread, totalentries, resumehandle);

    *entriesread = 0;
    *totalentries = 0;
    *bufptr = NULL;

    if (servername != NULL)
        RtlInitUnicodeString(&ServerName, servername);

    if (resumehandle != NULL && *resumehandle != 0)
    {
        EnumContext = (PENUM_CONTEXT)*resumehandle;
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
        EnumContext->BuiltinDone = FALSE;

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

        Status = GetAccountDomainSid((servername != NULL) ? &ServerName : NULL,
                                     &DomainSid);
        if (!NT_SUCCESS(Status))
        {
            ERR("GetAccountDomainSid failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        Status = SamOpenDomain(EnumContext->ServerHandle,
                               DOMAIN_LIST_ACCOUNTS | DOMAIN_LOOKUP,
                               DomainSid,
                               &EnumContext->AccountDomainHandle);

        RtlFreeHeap(RtlGetProcessHeap(), 0, DomainSid);

        if (!NT_SUCCESS(Status))
        {
            ERR("SamOpenDomain failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        Status = GetBuiltinDomainSid(&DomainSid);
        if (!NT_SUCCESS(Status))
        {
            ERR("GetAccountDomainSid failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        Status = SamOpenDomain(EnumContext->ServerHandle,
                               DOMAIN_LIST_ACCOUNTS | DOMAIN_LOOKUP,
                               DomainSid,
                               &EnumContext->BuiltinDomainHandle);

        RtlFreeHeap(RtlGetProcessHeap(), 0, DomainSid);

        if (!NT_SUCCESS(Status))
        {
            ERR("SamOpenDomain failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }
    }


//    while (TRUE)
//    {
        TRACE("EnumContext->Index: %lu\n", EnumContext->Index);
        TRACE("EnumContext->Returned: %lu\n", EnumContext->Returned);

        if (EnumContext->Index >= EnumContext->Returned)
        {
//            if (EnumContext->BuiltinDone == TRUE)
//            {
//                ApiStatus = NERR_Success;
//                goto done;
//            }

            TRACE("Calling SamEnumerateAliasesInDomain\n");

            Status = SamEnumerateAliasesInDomain(EnumContext->BuiltinDomainHandle,
                                                 &EnumContext->EnumerationContext,
                                                 (PVOID *)&EnumContext->Buffer,
                                                 prefmaxlen,
                                                 &EnumContext->Returned);

            TRACE("SamEnumerateAliasesInDomain returned (Status %08lx)\n", Status);
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
            else
            {
                EnumContext->BuiltinDone = TRUE;
            }
        }

        TRACE("EnumContext: %lu\n", EnumContext);
        TRACE("EnumContext->Returned: %lu\n", EnumContext->Returned);
        TRACE("EnumContext->Buffer: %p\n", EnumContext->Buffer);

        /* Get a pointer to the current alias */
        CurrentAlias = &EnumContext->Buffer[EnumContext->Index];

        TRACE("RID: %lu\n", CurrentAlias->RelativeId);

        Status = SamOpenAlias(EnumContext->BuiltinDomainHandle,
                              ALIAS_READ_INFORMATION,
                              CurrentAlias->RelativeId,
                              &AliasHandle);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamOpenAlias failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        Status = SamQueryInformationAlias(AliasHandle,
                                          AliasGeneralInformation,
                                          (PVOID *)&AliasInfo);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamQueryInformationAlias failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        SamCloseHandle(AliasHandle);
        AliasHandle = NULL;

        TRACE("Name: %S\n", AliasInfo->Name.Buffer);
        TRACE("Comment: %S\n", AliasInfo->AdminComment.Buffer);

        switch (level)
        {
            case 0:
                Size = sizeof(LOCALGROUP_INFO_0) +
                       AliasInfo->Name.Length + sizeof(WCHAR);
                break;

            case 1:
                Size = sizeof(LOCALGROUP_INFO_1) +
                       AliasInfo->Name.Length + sizeof(WCHAR) +
                       AliasInfo->AdminComment.Length + sizeof(WCHAR);
                break;

            default:
                ApiStatus = ERROR_INVALID_LEVEL;
                goto done;
        }

        ApiStatus = NetApiBufferAllocate(Size, &Buffer);
        if (ApiStatus != NERR_Success)
            goto done;

        switch (level)
        {
            case 0:
                LocalInfo0 = (PLOCALGROUP_INFO_0)Buffer;

                Ptr = (LPWSTR)LocalInfo0++;
                LocalInfo0->lgrpi0_name = Ptr;

                memcpy(LocalInfo0->lgrpi0_name,
                       AliasInfo->Name.Buffer,
                       AliasInfo->Name.Length);
                LocalInfo0->lgrpi0_name[AliasInfo->Name.Length / sizeof(WCHAR)] = UNICODE_NULL;
                break;

            case 1:
                LocalInfo1 = (PLOCALGROUP_INFO_1)Buffer;

                Ptr = (LPWSTR)((ULONG_PTR)LocalInfo1 + sizeof(LOCALGROUP_INFO_1));
                LocalInfo1->lgrpi1_name = Ptr;

                memcpy(LocalInfo1->lgrpi1_name,
                       AliasInfo->Name.Buffer,
                       AliasInfo->Name.Length);
                LocalInfo1->lgrpi1_name[AliasInfo->Name.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + AliasInfo->Name.Length + sizeof(WCHAR));
                LocalInfo1->lgrpi1_comment = Ptr;

                memcpy(LocalInfo1->lgrpi1_comment,
                       AliasInfo->AdminComment.Buffer,
                       AliasInfo->AdminComment.Length);
                LocalInfo1->lgrpi1_comment[AliasInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;
                break;
        }

        if (AliasInfo != NULL)
        {
            if (AliasInfo->Name.Buffer != NULL)
                SamFreeMemory(AliasInfo->Name.Buffer);

            if (AliasInfo->AdminComment.Buffer != NULL)
                SamFreeMemory(AliasInfo->AdminComment.Buffer);

            SamFreeMemory(AliasInfo);
            AliasInfo = NULL;
        }

        EnumContext->Index++;

        (*entriesread)++;

//    }

done:
    if (ApiStatus == NERR_Success && EnumContext->Index < EnumContext->Returned)
        ApiStatus = ERROR_MORE_DATA;

    if (EnumContext != NULL)
        *totalentries = EnumContext->Returned;

    if (resumehandle == NULL || ApiStatus != ERROR_MORE_DATA)
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

    if (AliasHandle != NULL)
        SamCloseHandle(AliasHandle);

    if (AliasInfo != NULL)
    {
        if (AliasInfo->Name.Buffer != NULL)
            SamFreeMemory(AliasInfo->Name.Buffer);

        if (AliasInfo->AdminComment.Buffer != NULL)
            SamFreeMemory(AliasInfo->AdminComment.Buffer);

        SamFreeMemory(AliasInfo);
    }

    if (resumehandle != NULL)
        *resumehandle = (DWORD_PTR)EnumContext;

    *bufptr = (LPBYTE)Buffer;

    TRACE ("return %lu\n", ApiStatus);

    return ApiStatus;
}

/************************************************************
 *                NetLocalGroupGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupGetInfo(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE* bufptr)
{
    FIXME("(%s %s %d %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, bufptr);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupGetMembers  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupGetMembers(
    LPCWSTR servername,
    LPCWSTR localgroupname,
    DWORD level,
    LPBYTE* bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    PDWORD_PTR resumehandle)
{
    FIXME("(%s %s %d %p %d, %p %p %p) stub!\n", debugstr_w(servername),
          debugstr_w(localgroupname), level, bufptr, prefmaxlen, entriesread,
          totalentries, resumehandle);

    if (level == 3)
    {
        WCHAR userName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD userNameLen;
        DWORD len,needlen;
        PLOCALGROUP_MEMBERS_INFO_3 ptr;

        /* still a stub,  current user is belonging to all groups */

        *totalentries = 1;
        *entriesread = 0;

        userNameLen = MAX_COMPUTERNAME_LENGTH + 1;
        GetUserNameW(userName,&userNameLen);
        needlen = sizeof(LOCALGROUP_MEMBERS_INFO_3) +
             (userNameLen+2) * sizeof(WCHAR);
        if (prefmaxlen != MAX_PREFERRED_LENGTH)
            len = min(prefmaxlen,needlen);
        else
            len = needlen;

        NetApiBufferAllocate(len, (LPVOID *) bufptr);
        if (len < needlen)
            return ERROR_MORE_DATA;

        ptr = (PLOCALGROUP_MEMBERS_INFO_3)*bufptr;
        ptr->lgrmi3_domainandname = (LPWSTR)(*bufptr+sizeof(LOCALGROUP_MEMBERS_INFO_3));
        lstrcpyW(ptr->lgrmi3_domainandname,userName);

        *entriesread = 1;
    }

    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupSetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupSetInfo(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    LPDWORD parm_err)
{
    FIXME("(%s %s %d %p %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, parm_err);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupSetMember (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupSetMembers(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    DWORD totalentries)
{
    FIXME("(%s %s %d %p %d) stub!\n", debugstr_w(servername),
            debugstr_w(groupname), level, buf, totalentries);
    return NERR_Success;
}
