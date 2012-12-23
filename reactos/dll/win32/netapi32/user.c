/*
 * Copyright 2002 Andriy Palamarchuk
 *
 * netapi32 user functions
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
#include "lmuse.h"
#include "ntsecapi.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"

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


/* NOTE: So far, this is implemented to support tests that require user logins,
 *       but not designed to handle real user databases. Those should probably
 *       be synced with either the host's user database or with Samba.
 *
 * FIXME: The user database should hold all the information the USER_INFO_4 struct
 * needs, but for the first try, I will just implement the USER_INFO_1 fields.
 */

struct sam_user
{
    struct list entry;
    WCHAR user_name[LM20_UNLEN+1];
    WCHAR user_password[PWLEN + 1];
    DWORD sec_since_passwd_change;
    DWORD user_priv;
    LPWSTR home_dir;
    LPWSTR user_comment;
    DWORD user_flags;
    LPWSTR user_logon_script_path;
};

static struct list user_list = LIST_INIT( user_list );

BOOL NETAPI_IsLocalComputer(LPCWSTR ServerName);

/************************************************************
 *                NETAPI_ValidateServername
 *
 * Validates server name
 */
static NET_API_STATUS NETAPI_ValidateServername(LPCWSTR ServerName)
{
    if (ServerName)
    {
        if (ServerName[0] == 0)
            return ERROR_BAD_NETPATH;
        else if (
            ((ServerName[0] == '\\') &&
             (ServerName[1] != '\\'))
            ||
            ((ServerName[0] == '\\') &&
             (ServerName[1] == '\\') &&
             (ServerName[2] == 0))
            )
            return ERROR_INVALID_NAME;
    }
    return NERR_Success;
}

/************************************************************
 *                NETAPI_FindUser
 *
 * Looks for a user in the user database.
 * Returns a pointer to the entry in the user list when the user
 * is found, NULL otherwise.
 */
static struct sam_user* NETAPI_FindUser(LPCWSTR UserName)
{
    struct sam_user *user;

    LIST_FOR_EACH_ENTRY(user, &user_list, struct sam_user, entry)
    {
        if(lstrcmpW(user->user_name, UserName) == 0)
            return user;
    }
    return NULL;
}

static BOOL NETAPI_IsCurrentUser(LPCWSTR username)
{
    LPWSTR curr_user = NULL;
    DWORD dwSize;
    BOOL ret = FALSE;

    dwSize = LM20_UNLEN+1;
    curr_user = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(WCHAR));
    if(!curr_user)
    {
        ERR("Failed to allocate memory for user name.\n");
        goto end;
    }
    if(!GetUserNameW(curr_user, &dwSize))
    {
        ERR("Failed to get current user's user name.\n");
        goto end;
    }
    if (!lstrcmpW(curr_user, username))
    {
        ret = TRUE;
    }

end:
    HeapFree(GetProcessHeap(), 0, curr_user);
    return ret;
}

/************************************************************
 * NetUserAdd (NETAPI32.@)
 */
NET_API_STATUS
WINAPI
NetUserAdd(LPCWSTR servername,
           DWORD level,
           LPBYTE bufptr,
           LPDWORD parm_err)
{
    NET_API_STATUS status;
    struct sam_user * su = NULL;

    FIXME("(%s, %d, %p, %p) stub!\n", debugstr_w(servername), level, bufptr, parm_err);

    if((status = NETAPI_ValidateServername(servername)) != NERR_Success)
        return status;

    switch(level)
    {
    /* Level 3 and 4 are identical for the purposes of NetUserAdd */
    case 4:
    case 3:
        FIXME("Level 3 and 4 not implemented.\n");
        /* Fall through */
    case 2:
        FIXME("Level 2 not implemented.\n");
        /* Fall through */
    case 1:
    {
        PUSER_INFO_1 ui = (PUSER_INFO_1) bufptr;
        su = HeapAlloc(GetProcessHeap(), 0, sizeof(struct sam_user));
        if(!su)
        {
            status = NERR_InternalError;
            break;
        }

        if(lstrlenW(ui->usri1_name) > LM20_UNLEN)
        {
            status = NERR_BadUsername;
            break;
        }

        /*FIXME: do other checks for a valid username */
        lstrcpyW(su->user_name, ui->usri1_name);

        if(lstrlenW(ui->usri1_password) > PWLEN)
        {
            /* Always return PasswordTooShort on invalid passwords. */
            status = NERR_PasswordTooShort;
            break;
        }
        lstrcpyW(su->user_password, ui->usri1_password);

        su->sec_since_passwd_change = ui->usri1_password_age;
        su->user_priv = ui->usri1_priv;
        su->user_flags = ui->usri1_flags;

        /*FIXME: set the other LPWSTRs to NULL for now */
        su->home_dir = NULL;
        su->user_comment = NULL;
        su->user_logon_script_path = NULL;

        list_add_head(&user_list, &su->entry);
        return NERR_Success;
    }
    default:
        TRACE("Invalid level %d specified.\n", level);
        status = ERROR_INVALID_LEVEL;
        break;
    }

    HeapFree(GetProcessHeap(), 0, su);

    return status;
}


/******************************************************************************
 *                NetUserChangePassword  (NETAPI32.@)
 * PARAMS
 *  domainname  [I] Optional. Domain on which the user resides or the logon
 *                  domain of the current user if NULL.
 *  username    [I] Optional. Username to change the password for or the name
 *                  of the current user if NULL.
 *  oldpassword [I] The user's current password.
 *  newpassword [I] The password that the user will be changed to using.
 *
 * RETURNS
 *  Success: NERR_Success.
 *  Failure: NERR_* failure code or win error code.
 *
 */
NET_API_STATUS
WINAPI
NetUserChangePassword(LPCWSTR domainname,
                      LPCWSTR username,
                      LPCWSTR oldpassword,
                      LPCWSTR newpassword)
{
    struct sam_user *user;

    TRACE("(%s, %s, ..., ...)\n", debugstr_w(domainname), debugstr_w(username));

    if(domainname)
        FIXME("Ignoring domainname %s.\n", debugstr_w(domainname));

    if((user = NETAPI_FindUser(username)) == NULL)
        return NERR_UserNotFound;

    if(lstrcmpW(user->user_password, oldpassword) != 0)
        return ERROR_INVALID_PASSWORD;

    if(lstrlenW(newpassword) > PWLEN)
        return ERROR_PASSWORD_RESTRICTION;

    lstrcpyW(user->user_password, newpassword);

    return NERR_Success;
}


/************************************************************
 * NetUserDel  (NETAPI32.@)
 */
NET_API_STATUS
WINAPI
NetUserDel(LPCWSTR servername,
           LPCWSTR username)
{
    NET_API_STATUS status;
    struct sam_user *user;

    TRACE("(%s, %s)\n", debugstr_w(servername), debugstr_w(username));

    if((status = NETAPI_ValidateServername(servername))!= NERR_Success)
        return status;

    if ((user = NETAPI_FindUser(username)) == NULL)
        return NERR_UserNotFound;

    list_remove(&user->entry);

    HeapFree(GetProcessHeap(), 0, user->home_dir);
    HeapFree(GetProcessHeap(), 0, user->user_comment);
    HeapFree(GetProcessHeap(), 0, user->user_logon_script_path);
    HeapFree(GetProcessHeap(), 0, user);

    return NERR_Success;
}


/************************************************************
 * NetUserEnum  (NETAPI32.@)
 */
NET_API_STATUS
WINAPI
NetUserEnum(LPCWSTR servername,
            DWORD level,
            DWORD filter,
            LPBYTE* bufptr,
            DWORD prefmaxlen,
            LPDWORD entriesread,
            LPDWORD totalentries,
            LPDWORD resume_handle)
{
    PSAM_RID_ENUMERATION CurrentUser;
    PENUM_CONTEXT EnumContext = NULL;
    LPVOID Buffer = NULL;
    PSID DomainSid = NULL;
    PUSER_INFO_0 UserInfo0;
    PUSER_INFO_1 UserInfo1;
    PUSER_INFO_20 UserInfo20;

    LPWSTR Ptr;
    ULONG i;
    ULONG Size;

    SAM_HANDLE UserHandle = NULL;
    PUSER_ACCOUNT_INFORMATION UserInfo = NULL;

    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    FIXME("(%s %d 0x%d %p %d %p %p %p) stub!\n", debugstr_w(servername), level,
          filter, bufptr, prefmaxlen, entriesread, totalentries, resume_handle);

    *entriesread = 0;
    *totalentries = 0;
    *bufptr = NULL;

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
        EnumContext->BuiltinDone = FALSE;

        Status = SamConnect(NULL,
                            &EnumContext->ServerHandle,
                            SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                            NULL);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamConnect failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        Status = GetAccountDomainSid(&DomainSid);
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

            TRACE("Calling SamEnumerateUsersInDomain\n");
            Status = SamEnumerateUsersInDomain(EnumContext->AccountDomainHandle, //BuiltinDomainHandle,
                                               &EnumContext->EnumerationContext,
                                               0,
                                               (PVOID *)&EnumContext->Buffer,
                                               prefmaxlen,
                                               &EnumContext->Returned);

            TRACE("SamEnumerateUsersInDomain returned (Status %08lx)\n", Status);
            if (!NT_SUCCESS(Status))
            {
                ERR("SamEnumerateUsersInDomain failed (Status %08lx)\n", Status);
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

        /* Get a pointer to the current user */
        CurrentUser = &EnumContext->Buffer[EnumContext->Index];

        TRACE("RID: %lu\n", CurrentUser->RelativeId);

        Status = SamOpenUser(EnumContext->AccountDomainHandle, //BuiltinDomainHandle,
                             USER_READ_GENERAL | USER_READ_PREFERENCES | USER_READ_LOGON | USER_READ_ACCOUNT,
                             CurrentUser->RelativeId,
                             &UserHandle);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamOpenUser failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        Status = SamQueryInformationUser(UserHandle,
                                         UserAccountInformation,
                                         (PVOID *)&UserInfo);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamQueryInformationUser failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        SamCloseHandle(UserHandle);
        UserHandle = NULL;

        switch (level)
        {
            case 0:
                Size = sizeof(USER_INFO_0) +
                       UserInfo->UserName.Length + sizeof(WCHAR);
                break;

            case 1:
                Size = sizeof(USER_INFO_1) +
                       UserInfo->UserName.Length + sizeof(WCHAR);

                if (UserInfo->HomeDirectory.Length > 0)
                    Size += UserInfo->HomeDirectory.Length + sizeof(WCHAR);

                if (UserInfo->AdminComment.Length > 0)
                    Size += UserInfo->AdminComment.Length + sizeof(WCHAR);

                if (UserInfo->ScriptPath.Length > 0)
                    Size = UserInfo->ScriptPath.Length + sizeof(WCHAR);
                break;

//            case 2:
//            case 3:
//            case 10:
//            case 11:

            case 20:
                Size = sizeof(USER_INFO_20) +
                       UserInfo->UserName.Length + sizeof(WCHAR);

                if (UserInfo->FullName.Length > 0)
                    Size += UserInfo->FullName.Length + sizeof(WCHAR);

                if (UserInfo->AdminComment.Length > 0)
                    Size += UserInfo->AdminComment.Length + sizeof(WCHAR);
                break;

//            case 23:

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
                UserInfo0 = (PUSER_INFO_0)Buffer;

                Ptr = (LPWSTR)((ULONG_PTR)UserInfo0 + sizeof(USER_INFO_0));
                UserInfo0->usri0_name = Ptr;

                memcpy(UserInfo0->usri0_name,
                       UserInfo->UserName.Buffer,
                       UserInfo->UserName.Length);
                UserInfo0->usri0_name[UserInfo->UserName.Length / sizeof(WCHAR)] = UNICODE_NULL;
                break;

            case 1:
                UserInfo1 = (PUSER_INFO_1)Buffer;

                Ptr = (LPWSTR)((ULONG_PTR)UserInfo1 + sizeof(USER_INFO_1));

                UserInfo1->usri1_name = Ptr;

                memcpy(UserInfo1->usri1_name,
                       UserInfo->UserName.Buffer,
                       UserInfo->UserName.Length);
                UserInfo1->usri1_name[UserInfo->UserName.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->UserName.Length + sizeof(WCHAR));

                UserInfo1->usri1_password = NULL;

                UserInfo1->usri1_password_age = 0; /* FIXME */

                UserInfo1->usri1_priv = 0; /* FIXME */

                if (UserInfo->HomeDirectory.Length > 0)
                {
                    UserInfo1->usri1_home_dir = Ptr;

                    memcpy(UserInfo1->usri1_home_dir,
                           UserInfo->HomeDirectory.Buffer,
                           UserInfo->HomeDirectory.Length);
                    UserInfo1->usri1_home_dir[UserInfo->HomeDirectory.Length / sizeof(WCHAR)] = UNICODE_NULL;

                    Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->HomeDirectory.Length + sizeof(WCHAR));
                }

                if (UserInfo->AdminComment.Length > 0)
                {
                    UserInfo1->usri1_comment = Ptr;

                    memcpy(UserInfo1->usri1_comment,
                           UserInfo->AdminComment.Buffer,
                           UserInfo->AdminComment.Length);
                    UserInfo1->usri1_comment[UserInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;

                    Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->AdminComment.Length + sizeof(WCHAR));
                }

                UserInfo1->usri1_flags = UserInfo->UserAccountControl;

                if (UserInfo->ScriptPath.Length > 0)
                {
                    UserInfo1->usri1_script_path = Ptr;

                    memcpy(UserInfo1->usri1_script_path,
                           UserInfo->ScriptPath.Buffer,
                           UserInfo->ScriptPath.Length);
                    UserInfo1->usri1_script_path[UserInfo->ScriptPath.Length / sizeof(WCHAR)] = UNICODE_NULL;
                }
                break;

//            case 2:
//            case 3:
//            case 10:
//            case 11:

            case 20:
                UserInfo20 = (PUSER_INFO_20)Buffer;

                Ptr = (LPWSTR)((ULONG_PTR)UserInfo20 + sizeof(USER_INFO_20));

                UserInfo20->usri20_name = Ptr;

                memcpy(UserInfo20->usri20_name,
                       UserInfo->UserName.Buffer,
                       UserInfo->UserName.Length);
                UserInfo20->usri20_name[UserInfo->UserName.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->UserName.Length + sizeof(WCHAR));

                if (UserInfo->FullName.Length > 0)
                {
                    UserInfo20->usri20_full_name = Ptr;

                    memcpy(UserInfo20->usri20_full_name,
                           UserInfo->FullName.Buffer,
                           UserInfo->FullName.Length);
                    UserInfo20->usri20_full_name[UserInfo->FullName.Length / sizeof(WCHAR)] = UNICODE_NULL;

                    Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->FullName.Length + sizeof(WCHAR));
                }

                if (UserInfo->AdminComment.Length > 0)
                {
                    UserInfo20->usri20_comment = Ptr;

                    memcpy(UserInfo20->usri20_comment,
                           UserInfo->AdminComment.Buffer,
                           UserInfo->AdminComment.Length);
                    UserInfo20->usri20_comment[UserInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;

                    Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->AdminComment.Length + sizeof(WCHAR));
                }

                UserInfo20->usri20_flags = UserInfo->UserAccountControl;
                UserInfo20->usri20_user_id = CurrentUser->RelativeId;
                break;

//            case 23:
        }

        if (UserInfo != NULL)
        {
            if (UserInfo->UserName.Buffer != NULL)
                SamFreeMemory(UserInfo->UserName.Buffer);

            if (UserInfo->FullName.Buffer != NULL)
                SamFreeMemory(UserInfo->FullName.Buffer);

            if (UserInfo->HomeDirectory.Buffer != NULL)
                SamFreeMemory(UserInfo->HomeDirectory.Buffer);

            if (UserInfo->HomeDirectoryDrive.Buffer != NULL)
                SamFreeMemory(UserInfo->HomeDirectoryDrive.Buffer);

            if (UserInfo->ScriptPath.Buffer != NULL)
                SamFreeMemory(UserInfo->ScriptPath.Buffer);

            if (UserInfo->ProfilePath.Buffer != NULL)
                SamFreeMemory(UserInfo->ProfilePath.Buffer);

            if (UserInfo->AdminComment.Buffer != NULL)
                SamFreeMemory(UserInfo->AdminComment.Buffer);

            if (UserInfo->WorkStations.Buffer != NULL)
                SamFreeMemory(UserInfo->WorkStations.Buffer);

            if (UserInfo->LogonHours.LogonHours != NULL)
                SamFreeMemory(UserInfo->LogonHours.LogonHours);

            SamFreeMemory(UserInfo);
            UserInfo = NULL;
        }

        EnumContext->Index++;

        (*entriesread)++;
//    }

done:
    if (ApiStatus == NERR_Success && EnumContext->Index < EnumContext->Returned)
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

    if (UserHandle != NULL)
        SamCloseHandle(UserHandle);

    if (UserInfo != NULL)
    {
        if (UserInfo->UserName.Buffer != NULL)
            SamFreeMemory(UserInfo->UserName.Buffer);

        if (UserInfo->FullName.Buffer != NULL)
            SamFreeMemory(UserInfo->FullName.Buffer);

        if (UserInfo->HomeDirectory.Buffer != NULL)
            SamFreeMemory(UserInfo->HomeDirectory.Buffer);

        if (UserInfo->HomeDirectoryDrive.Buffer != NULL)
            SamFreeMemory(UserInfo->HomeDirectoryDrive.Buffer);

        if (UserInfo->ScriptPath.Buffer != NULL)
            SamFreeMemory(UserInfo->ScriptPath.Buffer);

        if (UserInfo->ProfilePath.Buffer != NULL)
            SamFreeMemory(UserInfo->ProfilePath.Buffer);

        if (UserInfo->AdminComment.Buffer != NULL)
            SamFreeMemory(UserInfo->AdminComment.Buffer);

        if (UserInfo->WorkStations.Buffer != NULL)
            SamFreeMemory(UserInfo->WorkStations.Buffer);

        if (UserInfo->LogonHours.LogonHours != NULL)
            SamFreeMemory(UserInfo->LogonHours.LogonHours);

        SamFreeMemory(UserInfo);
    }

    if (resume_handle != NULL)
        *resume_handle = (DWORD_PTR)EnumContext;

    *bufptr = (LPBYTE)Buffer;

    TRACE("return %lu\n", ApiStatus);

    return ApiStatus;
}


/************************************************************
 * NetUserGetGroups (NETAPI32.@)
 */
NET_API_STATUS
WINAPI
NetUserGetGroups(LPCWSTR servername,
                 LPCWSTR username,
                 DWORD level,
                 LPBYTE *bufptr,
                 DWORD prefixmaxlen,
                 LPDWORD entriesread,
                 LPDWORD totalentries)
{
    FIXME("%s %s %d %p %d %p %p stub\n", debugstr_w(servername),
          debugstr_w(username), level, bufptr, prefixmaxlen, entriesread,
          totalentries);

    *bufptr = NULL;
    *entriesread = 0;
    *totalentries = 0;

    return ERROR_INVALID_LEVEL;
}


/************************************************************
 * NetUserGetInfo  (NETAPI32.@)
 */
NET_API_STATUS
WINAPI
NetUserGetInfo(LPCWSTR servername,
               LPCWSTR username,
               DWORD level,
               LPBYTE* bufptr)
{
    NET_API_STATUS status;
    TRACE("(%s, %s, %d, %p)\n", debugstr_w(servername), debugstr_w(username),
          level, bufptr);
    status = NETAPI_ValidateServername(servername);
    if (status != NERR_Success)
        return status;

    if(!NETAPI_IsLocalComputer(servername))
    {
        FIXME("Only implemented for local computer, but remote server"
              "%s was requested.\n", debugstr_w(servername));
        return NERR_InvalidComputer;
    }

    if(!NETAPI_FindUser(username) && !NETAPI_IsCurrentUser(username))
    {
        TRACE("User %s is unknown.\n", debugstr_w(username));
        return NERR_UserNotFound;
    }

    switch (level)
    {
    case 0:
    {
        PUSER_INFO_0 ui;
        int name_sz;

        name_sz = lstrlenW(username) + 1;

        /* set up buffer */
        NetApiBufferAllocate(sizeof(USER_INFO_0) + name_sz * sizeof(WCHAR),
                             (LPVOID *) bufptr);

        ui = (PUSER_INFO_0) *bufptr;
        ui->usri0_name = (LPWSTR) (*bufptr + sizeof(USER_INFO_0));

        /* get data */
        lstrcpyW(ui->usri0_name, username);
        break;
    }

    case 10:
    {
        PUSER_INFO_10 ui;
        PUSER_INFO_0 ui0;
        NET_API_STATUS status;
        /* sizes of the field buffers in WCHARS */
        int name_sz, comment_sz, usr_comment_sz, full_name_sz;

        comment_sz = 1;
        usr_comment_sz = 1;
        full_name_sz = 1;

        /* get data */
        status = NetUserGetInfo(servername, username, 0, (LPBYTE *) &ui0);
        if (status != NERR_Success)
        {
            NetApiBufferFree(ui0);
            return status;
        }
        name_sz = lstrlenW(ui0->usri0_name) + 1;

        /* set up buffer */
        NetApiBufferAllocate(sizeof(USER_INFO_10) +
                             (name_sz + comment_sz + usr_comment_sz +
                              full_name_sz) * sizeof(WCHAR),
                             (LPVOID *) bufptr);
        ui = (PUSER_INFO_10) *bufptr;
        ui->usri10_name = (LPWSTR) (*bufptr + sizeof(USER_INFO_10));
        ui->usri10_comment = (LPWSTR) (
            ((PBYTE) ui->usri10_name) + name_sz * sizeof(WCHAR));
        ui->usri10_usr_comment = (LPWSTR) (
            ((PBYTE) ui->usri10_comment) + comment_sz * sizeof(WCHAR));
        ui->usri10_full_name = (LPWSTR) (
            ((PBYTE) ui->usri10_usr_comment) + usr_comment_sz * sizeof(WCHAR));

        /* set data */
        lstrcpyW(ui->usri10_name, ui0->usri0_name);
        NetApiBufferFree(ui0);
        ui->usri10_comment[0] = 0;
        ui->usri10_usr_comment[0] = 0;
        ui->usri10_full_name[0] = 0;
        break;
    }

    case 1:
      {
        static const WCHAR homedirW[] = {'H','O','M','E',0};
        PUSER_INFO_1 ui;
        PUSER_INFO_0 ui0;
        NET_API_STATUS status;
        /* sizes of the field buffers in WCHARS */
        int name_sz, password_sz, home_dir_sz, comment_sz, script_path_sz;

        password_sz = 1; /* not filled out for security reasons for NetUserGetInfo*/
        comment_sz = 1;
        script_path_sz = 1;

       /* get data */
        status = NetUserGetInfo(servername, username, 0, (LPBYTE *) &ui0);
        if (status != NERR_Success)
        {
            NetApiBufferFree(ui0);
            return status;
        }
        name_sz = lstrlenW(ui0->usri0_name) + 1;
        home_dir_sz = GetEnvironmentVariableW(homedirW, NULL,0);
        /* set up buffer */
        NetApiBufferAllocate(sizeof(USER_INFO_1) +
                             (name_sz + password_sz + home_dir_sz +
                              comment_sz + script_path_sz) * sizeof(WCHAR),
                             (LPVOID *) bufptr);

        ui = (PUSER_INFO_1) *bufptr;
        ui->usri1_name = (LPWSTR) (ui + 1);
        ui->usri1_password = ui->usri1_name + name_sz;
        ui->usri1_home_dir = ui->usri1_password + password_sz;
        ui->usri1_comment = ui->usri1_home_dir + home_dir_sz;
        ui->usri1_script_path = ui->usri1_comment + comment_sz;
        /* set data */
        lstrcpyW(ui->usri1_name, ui0->usri0_name);
        NetApiBufferFree(ui0);
        ui->usri1_password[0] = 0;
        ui->usri1_password_age = 0;
        ui->usri1_priv = 0;
        GetEnvironmentVariableW(homedirW, ui->usri1_home_dir,home_dir_sz);
        ui->usri1_comment[0] = 0;
        ui->usri1_flags = 0;
        ui->usri1_script_path[0] = 0;
        break;
      }
    case 2:
    case 3:
    case 4:
    case 11:
    case 20:
    case 23:
    case 1003:
    case 1005:
    case 1006:
    case 1007:
    case 1008:
    case 1009:
    case 1010:
    case 1011:
    case 1012:
    case 1013:
    case 1014:
    case 1017:
    case 1018:
    case 1020:
    case 1023:
    case 1024:
    case 1025:
    case 1051:
    case 1052:
    case 1053:
    {
        FIXME("Level %d is not implemented\n", level);
        return NERR_InternalError;
    }
    default:
        TRACE("Invalid level %d is specified\n", level);
        return ERROR_INVALID_LEVEL;
    }
    return NERR_Success;
}


/************************************************************
 * NetUserGetLocalGroups  (NETAPI32.@)
 */
NET_API_STATUS
WINAPI
NetUserGetLocalGroups(LPCWSTR servername,
                      LPCWSTR username,
                      DWORD level,
                      DWORD flags,
                      LPBYTE* bufptr,
                      DWORD prefmaxlen,
                      LPDWORD entriesread,
                      LPDWORD totalentries)
{
    NET_API_STATUS status;
    const WCHAR admins[] = {'A','d','m','i','n','i','s','t','r','a','t','o','r','s',0};
    LPWSTR currentuser;
    LOCALGROUP_USERS_INFO_0* info;
    DWORD size;

    FIXME("(%s, %s, %d, %08x, %p %d, %p, %p) stub!\n",
          debugstr_w(servername), debugstr_w(username), level, flags, bufptr,
          prefmaxlen, entriesread, totalentries);

    status = NETAPI_ValidateServername(servername);
    if (status != NERR_Success)
        return status;

    size = UNLEN + 1;
    NetApiBufferAllocate(size * sizeof(WCHAR), (LPVOID*)&currentuser);
    GetUserNameW(currentuser, &size);

    if (lstrcmpiW(username, currentuser) && NETAPI_FindUser(username))
    {
        NetApiBufferFree(currentuser);
        return NERR_UserNotFound;
    }

    NetApiBufferFree(currentuser);
    *totalentries = 1;
    size = sizeof(*info) + sizeof(admins);

    if(prefmaxlen < size)
        status = ERROR_MORE_DATA;
    else
        status = NetApiBufferAllocate(size, (LPVOID*)&info);

    if(status != NERR_Success)
    {
        *bufptr = NULL;
        *entriesread = 0;
        return status;
    }

    info->lgrui0_name = (LPWSTR)((LPBYTE)info + sizeof(*info));
    lstrcpyW(info->lgrui0_name, admins);

    *bufptr = (LPBYTE)info;
    *entriesread = 1;

    return NERR_Success;
}


/******************************************************************************
 * NetUserSetGroups  (NETAPI32.@)
 */
NET_API_STATUS
WINAPI
NetUserSetGroups(LPCWSTR servername,
                 LPCWSTR username,
                 DWORD level,
                 LPBYTE buf,
                 DWORD num_entries)
{
    FIXME("(%s %s %lu %p %lu)\n",
          debugstr_w(servername), debugstr_w(username), level, buf, num_entries);
    return ERROR_ACCESS_DENIED;
}


/******************************************************************************
 * NetUserSetInfo  (NETAPI32.@)
 */
NET_API_STATUS
WINAPI
NetUserSetInfo(LPCWSTR servername,
               LPCWSTR username,
               DWORD level,
               LPBYTE buf,
               LPDWORD parm_err)
{
    FIXME("(%s %s %lu %p %p)\n",
          debugstr_w(servername), debugstr_w(username), level, buf, parm_err);
    return ERROR_ACCESS_DENIED;
}

/* EOF */
