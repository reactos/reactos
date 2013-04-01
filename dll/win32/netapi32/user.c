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

/*
 *  TODO:
 *    Implement NetUserChangePassword
 *    Implement NetUserDel
 *    Implement NetUserGetGroups
 *    Implement NetUserSetGroups
 *    Implement NetUserSetInfo
 *    NetUserGetLocalGroups does not support LG_INCLUDE_INDIRECT yet.
 *    Add missing information levels.
 *    ...
 */

#include "netapi32.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);


typedef struct _ENUM_CONTEXT
{
    SAM_HANDLE ServerHandle;
    SAM_HANDLE BuiltinDomainHandle;
    SAM_HANDLE AccountDomainHandle;

    SAM_ENUMERATE_HANDLE EnumerationContext;
    PSAM_RID_ENUMERATION Buffer;
    ULONG Count;
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


static PSID
CreateSidFromSidAndRid(PSID SrcSid,
                       ULONG RelativeId)
{
    UCHAR RidCount;
    PSID DstSid;
    ULONG i;
    ULONG DstSidSize;
    PULONG p, q;

    RidCount = *RtlSubAuthorityCountSid(SrcSid);
    if (RidCount >= 8)
        return NULL;

    DstSidSize = RtlLengthRequiredSid(RidCount + 1);

    DstSid = RtlAllocateHeap(RtlGetProcessHeap(),
                             0,
                             DstSidSize);
    if (DstSid == NULL)
        return NULL;

    RtlInitializeSid(DstSid,
                     RtlIdentifierAuthoritySid(SrcSid),
                     RidCount + 1);

    for (i = 0; i < (ULONG)RidCount; i++)
    {
        p = RtlSubAuthoritySid(SrcSid, i);
        q = RtlSubAuthoritySid(DstSid, i);
        *q = *p;
    }

    q = RtlSubAuthoritySid(DstSid, (ULONG)RidCount);
    *q = RelativeId;

    return DstSid;
}


static
ULONG
GetAccountFlags(ULONG AccountControl)
{
    ULONG Flags = UF_SCRIPT;

    if (AccountControl & USER_ACCOUNT_DISABLED)
        Flags |= UF_ACCOUNTDISABLE;

    if (AccountControl & USER_HOME_DIRECTORY_REQUIRED)
        Flags |= UF_HOMEDIR_REQUIRED;

    if (AccountControl & USER_PASSWORD_NOT_REQUIRED)
        Flags |= UF_PASSWD_NOTREQD;

//    UF_PASSWD_CANT_CHANGE

    if (AccountControl & USER_ACCOUNT_AUTO_LOCKED)
        Flags |= UF_LOCKOUT;

    if (AccountControl & USER_DONT_EXPIRE_PASSWORD)
        Flags |= UF_DONT_EXPIRE_PASSWD;

/*
    if (AccountControl & USER_ENCRYPTED_TEXT_PASSWORD_ALLOWED)
        Flags |= UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED;

    if (AccountControl & USER_SMARTCARD_REQUIRED)
        Flags |= UF_SMARTCARD_REQUIRED;

    if (AccountControl & USER_TRUSTED_FOR_DELEGATION)
        Flags |= UF_TRUSTED_FOR_DELEGATION;

    if (AccountControl & USER_NOT_DELEGATED)
        Flags |= UF_NOT_DELEGATED;

    if (AccountControl & USER_USE_DES_KEY_ONLY)
        Flags |= UF_USE_DES_KEY_ONLY;

    if (AccountControl & USER_DONT_REQUIRE_PREAUTH)
        Flags |= UF_DONT_REQUIRE_PREAUTH;

    if (AccountControl & USER_PASSWORD_EXPIRED)
        Flags |= UF_PASSWORD_EXPIRED;
*/

    /* Set account type flags */
    if (AccountControl & USER_TEMP_DUPLICATE_ACCOUNT)
        Flags |= UF_TEMP_DUPLICATE_ACCOUNT;
    else if (AccountControl & USER_NORMAL_ACCOUNT)
        Flags |= UF_NORMAL_ACCOUNT;
    else if (AccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT)
        Flags |= UF_INTERDOMAIN_TRUST_ACCOUNT;
    else if (AccountControl & USER_WORKSTATION_TRUST_ACCOUNT)
        Flags |= UF_WORKSTATION_TRUST_ACCOUNT;
    else if (AccountControl & USER_SERVER_TRUST_ACCOUNT)
        Flags |= UF_SERVER_TRUST_ACCOUNT;

    return Flags;
}


static
NET_API_STATUS
BuildUserInfoBuffer(PUSER_ACCOUNT_INFORMATION UserInfo,
                    DWORD level,
                    ULONG RelativeId,
                    LPVOID *Buffer)
{
    LPVOID LocalBuffer = NULL;
    PUSER_INFO_0 UserInfo0;
    PUSER_INFO_1 UserInfo1;
    PUSER_INFO_2 UserInfo2;
    PUSER_INFO_3 UserInfo3;
    PUSER_INFO_10 UserInfo10;
    PUSER_INFO_20 UserInfo20;
    PUSER_INFO_23 UserInfo23;
    LPWSTR Ptr;
    ULONG Size = 0;
    NET_API_STATUS ApiStatus = NERR_Success;

    *Buffer = NULL;

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
                Size += UserInfo->ScriptPath.Length + sizeof(WCHAR);
            break;

        case 2:
            Size = sizeof(USER_INFO_2) +
                   UserInfo->UserName.Length + sizeof(WCHAR);

            if (UserInfo->HomeDirectory.Length > 0)
                Size += UserInfo->HomeDirectory.Length + sizeof(WCHAR);

            if (UserInfo->AdminComment.Length > 0)
                Size += UserInfo->AdminComment.Length + sizeof(WCHAR);

            if (UserInfo->ScriptPath.Length > 0)
                Size += UserInfo->ScriptPath.Length + sizeof(WCHAR);

            if (UserInfo->FullName.Length > 0)
                Size += UserInfo->FullName.Length + sizeof(WCHAR);

            /* FIXME: usri2_usr_comment */
            /* FIXME: usri2_parms */

            if (UserInfo->WorkStations.Length > 0)
                Size += UserInfo->WorkStations.Length + sizeof(WCHAR);

            /* FIXME: usri2_logon_hours */
            /* FIXME: usri2_logon_server */
            break;

        case 3:
            Size = sizeof(USER_INFO_3) +
                   UserInfo->UserName.Length + sizeof(WCHAR);

            if (UserInfo->HomeDirectory.Length > 0)
                Size += UserInfo->HomeDirectory.Length + sizeof(WCHAR);

            if (UserInfo->AdminComment.Length > 0)
                Size += UserInfo->AdminComment.Length + sizeof(WCHAR);

            if (UserInfo->ScriptPath.Length > 0)
                Size += UserInfo->ScriptPath.Length + sizeof(WCHAR);

            if (UserInfo->FullName.Length > 0)
                Size += UserInfo->FullName.Length + sizeof(WCHAR);

            /* FIXME: usri3_usr_comment */
            /* FIXME: usri3_parms */

            if (UserInfo->WorkStations.Length > 0)
                Size += UserInfo->WorkStations.Length + sizeof(WCHAR);

            /* FIXME: usri3_logon_hours */
            /* FIXME: usri3_logon_server */

            if (UserInfo->ProfilePath.Length > 0)
                Size += UserInfo->ProfilePath.Length + sizeof(WCHAR);

            if (UserInfo->HomeDirectoryDrive.Length > 0)
                Size += UserInfo->HomeDirectoryDrive.Length + sizeof(WCHAR);
            break;

//        case 4:

        case 10:
            Size = sizeof(USER_INFO_10) +
                   UserInfo->UserName.Length + sizeof(WCHAR);

            if (UserInfo->AdminComment.Length > 0)
                Size += UserInfo->AdminComment.Length + sizeof(WCHAR);

            /* FIXME: usri10_usr_comment */

            if (UserInfo->FullName.Length > 0)
                Size += UserInfo->FullName.Length + sizeof(WCHAR);
            break;

//        case 11:

        case 20:
            Size = sizeof(USER_INFO_20) +
                   UserInfo->UserName.Length + sizeof(WCHAR);

            if (UserInfo->FullName.Length > 0)
                Size += UserInfo->FullName.Length + sizeof(WCHAR);

            if (UserInfo->AdminComment.Length > 0)
                Size += UserInfo->AdminComment.Length + sizeof(WCHAR);
            break;

        case 23:
            Size = sizeof(USER_INFO_23) +
                   UserInfo->UserName.Length + sizeof(WCHAR);

            if (UserInfo->FullName.Length > 0)
                Size += UserInfo->FullName.Length + sizeof(WCHAR);

            if (UserInfo->AdminComment.Length > 0)
                Size += UserInfo->AdminComment.Length + sizeof(WCHAR);

            /* FIXME: usri23_user_sid */
            break;

        default:
            ApiStatus = ERROR_INVALID_LEVEL;
            goto done;
    }

    ApiStatus = NetApiBufferAllocate(Size, &LocalBuffer);
    if (ApiStatus != NERR_Success)
        goto done;

    ZeroMemory(LocalBuffer, Size);

    switch (level)
    {
        case 0:
            UserInfo0 = (PUSER_INFO_0)LocalBuffer;

            Ptr = (LPWSTR)((ULONG_PTR)UserInfo0 + sizeof(USER_INFO_0));
            UserInfo0->usri0_name = Ptr;

            memcpy(UserInfo0->usri0_name,
                   UserInfo->UserName.Buffer,
                   UserInfo->UserName.Length);
            UserInfo0->usri0_name[UserInfo->UserName.Length / sizeof(WCHAR)] = UNICODE_NULL;
            break;

        case 1:
            UserInfo1 = (PUSER_INFO_1)LocalBuffer;

            Ptr = (LPWSTR)((ULONG_PTR)UserInfo1 + sizeof(USER_INFO_1));

            UserInfo1->usri1_name = Ptr;

            memcpy(UserInfo1->usri1_name,
                   UserInfo->UserName.Buffer,
                   UserInfo->UserName.Length);
            UserInfo1->usri1_name[UserInfo->UserName.Length / sizeof(WCHAR)] = UNICODE_NULL;

            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->UserName.Length + sizeof(WCHAR));

            UserInfo1->usri1_password = NULL;

            /* FIXME: UserInfo1->usri1_password_age */
            /* FIXME: UserInfo1->usri1_priv */

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

            UserInfo1->usri1_flags = GetAccountFlags(UserInfo->UserAccountControl);

            if (UserInfo->ScriptPath.Length > 0)
            {
                UserInfo1->usri1_script_path = Ptr;

                memcpy(UserInfo1->usri1_script_path,
                       UserInfo->ScriptPath.Buffer,
                       UserInfo->ScriptPath.Length);
                UserInfo1->usri1_script_path[UserInfo->ScriptPath.Length / sizeof(WCHAR)] = UNICODE_NULL;
            }
            break;

        case 2:
            UserInfo2 = (PUSER_INFO_2)LocalBuffer;

            Ptr = (LPWSTR)((ULONG_PTR)UserInfo2 + sizeof(USER_INFO_2));

            UserInfo2->usri2_name = Ptr;

            memcpy(UserInfo2->usri2_name,
                   UserInfo->UserName.Buffer,
                   UserInfo->UserName.Length);
            UserInfo2->usri2_name[UserInfo->UserName.Length / sizeof(WCHAR)] = UNICODE_NULL;

            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->UserName.Length + sizeof(WCHAR));

            /* FIXME: usri2_password_age */
            /* FIXME: usri2_priv */

            if (UserInfo->HomeDirectory.Length > 0)
            {
                UserInfo2->usri2_home_dir = Ptr;

                memcpy(UserInfo2->usri2_home_dir,
                       UserInfo->HomeDirectory.Buffer,
                       UserInfo->HomeDirectory.Length);
                UserInfo2->usri2_home_dir[UserInfo->HomeDirectory.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->HomeDirectory.Length + sizeof(WCHAR));
            }

            if (UserInfo->AdminComment.Length > 0)
            {
                UserInfo2->usri2_comment = Ptr;

                memcpy(UserInfo2->usri2_comment,
                       UserInfo->AdminComment.Buffer,
                       UserInfo->AdminComment.Length);
                UserInfo2->usri2_comment[UserInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->AdminComment.Length + sizeof(WCHAR));
            }

            UserInfo2->usri2_flags = GetAccountFlags(UserInfo->UserAccountControl);

            if (UserInfo->ScriptPath.Length > 0)
            {
                UserInfo2->usri2_script_path = Ptr;

                memcpy(UserInfo2->usri2_script_path,
                       UserInfo->ScriptPath.Buffer,
                       UserInfo->ScriptPath.Length);
                UserInfo2->usri2_script_path[UserInfo->ScriptPath.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->ScriptPath.Length + sizeof(WCHAR));
            }

            /* FIXME: usri2_auth_flags */

            if (UserInfo->FullName.Length > 0)
            {
                UserInfo2->usri2_full_name = Ptr;

                memcpy(UserInfo2->usri2_full_name,
                       UserInfo->FullName.Buffer,
                       UserInfo->FullName.Length);
                UserInfo2->usri2_full_name[UserInfo->FullName.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->FullName.Length + sizeof(WCHAR));
            }

            /* FIXME: usri2_usr_comment */
            /* FIXME: usri2_parms */

            if (UserInfo->WorkStations.Length > 0)
            {
                UserInfo2->usri2_workstations = Ptr;

                memcpy(UserInfo2->usri2_workstations,
                       UserInfo->WorkStations.Buffer,
                       UserInfo->WorkStations.Length);
                UserInfo2->usri2_workstations[UserInfo->WorkStations.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->WorkStations.Length + sizeof(WCHAR));
            }

            RtlTimeToSecondsSince1970(&UserInfo->LastLogon,
                                      &UserInfo2->usri2_last_logon);

            RtlTimeToSecondsSince1970(&UserInfo->LastLogoff,
                                      &UserInfo2->usri2_last_logoff);

            RtlTimeToSecondsSince1970(&UserInfo->AccountExpires,
                                      &UserInfo2->usri2_acct_expires);

            UserInfo2->usri2_max_storage = USER_MAXSTORAGE_UNLIMITED;

            /* FIXME: usri2_units_per_week */
            /* FIXME: usri2_logon_hours */

            UserInfo2->usri2_bad_pw_count = UserInfo->BadPasswordCount;
            UserInfo2->usri2_num_logons = UserInfo->LogonCount;

            /* FIXME: usri2_logon_server */
            /* FIXME: usri2_country_code */
            /* FIXME: usri2_code_page */

            break;

        case 3:
            UserInfo3 = (PUSER_INFO_3)LocalBuffer;

            Ptr = (LPWSTR)((ULONG_PTR)UserInfo3 + sizeof(USER_INFO_3));

            UserInfo3->usri3_name = Ptr;

            memcpy(UserInfo3->usri3_name,
                   UserInfo->UserName.Buffer,
                   UserInfo->UserName.Length);
            UserInfo3->usri3_name[UserInfo->UserName.Length / sizeof(WCHAR)] = UNICODE_NULL;

            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->UserName.Length + sizeof(WCHAR));

            /* FIXME: usri3_password_age */
            /* FIXME: usri3_priv */

            if (UserInfo->HomeDirectory.Length > 0)
            {
                UserInfo3->usri3_home_dir = Ptr;

                memcpy(UserInfo3->usri3_home_dir,
                       UserInfo->HomeDirectory.Buffer,
                       UserInfo->HomeDirectory.Length);
                UserInfo3->usri3_home_dir[UserInfo->HomeDirectory.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->HomeDirectory.Length + sizeof(WCHAR));
            }

            if (UserInfo->AdminComment.Length > 0)
            {
                UserInfo3->usri3_comment = Ptr;

                memcpy(UserInfo3->usri3_comment,
                       UserInfo->AdminComment.Buffer,
                       UserInfo->AdminComment.Length);
                UserInfo3->usri3_comment[UserInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->AdminComment.Length + sizeof(WCHAR));
            }

            UserInfo3->usri3_flags = GetAccountFlags(UserInfo->UserAccountControl);

            if (UserInfo->ScriptPath.Length > 0)
            {
                UserInfo3->usri3_script_path = Ptr;

                memcpy(UserInfo3->usri3_script_path,
                       UserInfo->ScriptPath.Buffer,
                       UserInfo->ScriptPath.Length);
                UserInfo3->usri3_script_path[UserInfo->ScriptPath.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->ScriptPath.Length + sizeof(WCHAR));
            }

            /* FIXME: usri3_auth_flags */

            if (UserInfo->FullName.Length > 0)
            {
                UserInfo3->usri3_full_name = Ptr;

                memcpy(UserInfo3->usri3_full_name,
                       UserInfo->FullName.Buffer,
                       UserInfo->FullName.Length);
                UserInfo3->usri3_full_name[UserInfo->FullName.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->FullName.Length + sizeof(WCHAR));
            }

            /* FIXME: usri3_usr_comment */
            /* FIXME: usri3_parms */

            if (UserInfo->WorkStations.Length > 0)
            {
                UserInfo3->usri3_workstations = Ptr;

                memcpy(UserInfo3->usri3_workstations,
                       UserInfo->WorkStations.Buffer,
                       UserInfo->WorkStations.Length);
                UserInfo3->usri3_workstations[UserInfo->WorkStations.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->WorkStations.Length + sizeof(WCHAR));
            }

            RtlTimeToSecondsSince1970(&UserInfo->LastLogon,
                                      &UserInfo3->usri3_last_logon);

            RtlTimeToSecondsSince1970(&UserInfo->LastLogoff,
                                      &UserInfo3->usri3_last_logoff);

            RtlTimeToSecondsSince1970(&UserInfo->AccountExpires,
                                      &UserInfo3->usri3_acct_expires);

            UserInfo3->usri3_max_storage = USER_MAXSTORAGE_UNLIMITED;

            /* FIXME: usri3_units_per_week */
            /* FIXME: usri3_logon_hours */

            UserInfo3->usri3_bad_pw_count = UserInfo->BadPasswordCount;
            UserInfo3->usri3_num_logons = UserInfo->LogonCount;

            /* FIXME: usri3_logon_server */
            /* FIXME: usri3_country_code */
            /* FIXME: usri3_code_page */

            UserInfo3->usri3_user_id = RelativeId;
            UserInfo3->usri3_primary_group_id = UserInfo->PrimaryGroupId;

            if (UserInfo->ProfilePath.Length > 0)
            {
                UserInfo3->usri3_profile = Ptr;

                memcpy(UserInfo3->usri3_profile,
                       UserInfo->ProfilePath.Buffer,
                       UserInfo->ProfilePath.Length);
                UserInfo3->usri3_profile[UserInfo->ProfilePath.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->ProfilePath.Length + sizeof(WCHAR));
            }

            if (UserInfo->HomeDirectoryDrive.Length > 0)
            {
                UserInfo3->usri3_home_dir_drive = Ptr;

                memcpy(UserInfo3->usri3_home_dir_drive,
                       UserInfo->HomeDirectoryDrive.Buffer,
                       UserInfo->HomeDirectoryDrive.Length);
                UserInfo3->usri3_home_dir_drive[UserInfo->HomeDirectoryDrive.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->HomeDirectoryDrive.Length + sizeof(WCHAR));
            }

            UserInfo3->usri3_password_expired = (UserInfo->UserAccountControl & USER_PASSWORD_EXPIRED);
            break;

//        case 4:

        case 10:
            UserInfo10 = (PUSER_INFO_10)LocalBuffer;

            Ptr = (LPWSTR)((ULONG_PTR)UserInfo10 + sizeof(USER_INFO_10));

            UserInfo10->usri10_name = Ptr;

            memcpy(UserInfo10->usri10_name,
                   UserInfo->UserName.Buffer,
                   UserInfo->UserName.Length);
            UserInfo10->usri10_name[UserInfo->UserName.Length / sizeof(WCHAR)] = UNICODE_NULL;

            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->UserName.Length + sizeof(WCHAR));

            if (UserInfo->AdminComment.Length > 0)
            {
                UserInfo10->usri10_comment = Ptr;

                memcpy(UserInfo10->usri10_comment,
                       UserInfo->AdminComment.Buffer,
                       UserInfo->AdminComment.Length);
                UserInfo10->usri10_comment[UserInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->AdminComment.Length + sizeof(WCHAR));
            }

            /* FIXME: usri10_usr_comment */

            if (UserInfo->FullName.Length > 0)
            {
                UserInfo10->usri10_full_name = Ptr;

                memcpy(UserInfo10->usri10_full_name,
                       UserInfo->FullName.Buffer,
                       UserInfo->FullName.Length);
                UserInfo10->usri10_full_name[UserInfo->FullName.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->FullName.Length + sizeof(WCHAR));
            }
            break;

//        case 11:

        case 20:
            UserInfo20 = (PUSER_INFO_20)LocalBuffer;

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

            UserInfo20->usri20_flags = GetAccountFlags(UserInfo->UserAccountControl);

            UserInfo20->usri20_user_id = RelativeId;
            break;

        case 23:
            UserInfo23 = (PUSER_INFO_23)LocalBuffer;

            Ptr = (LPWSTR)((ULONG_PTR)UserInfo23 + sizeof(USER_INFO_23));

            UserInfo23->usri23_name = Ptr;

            memcpy(UserInfo23->usri23_name,
                   UserInfo->UserName.Buffer,
                   UserInfo->UserName.Length);
            UserInfo23->usri23_name[UserInfo->UserName.Length / sizeof(WCHAR)] = UNICODE_NULL;

            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->UserName.Length + sizeof(WCHAR));

            if (UserInfo->FullName.Length > 0)
            {
                UserInfo23->usri23_full_name = Ptr;

                memcpy(UserInfo23->usri23_full_name,
                       UserInfo->FullName.Buffer,
                       UserInfo->FullName.Length);
                UserInfo23->usri23_full_name[UserInfo->FullName.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->FullName.Length + sizeof(WCHAR));
            }

            if (UserInfo->AdminComment.Length > 0)
            {
                UserInfo23->usri23_comment = Ptr;

                memcpy(UserInfo23->usri23_comment,
                       UserInfo->AdminComment.Buffer,
                       UserInfo->AdminComment.Length);
                UserInfo23->usri23_comment[UserInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->AdminComment.Length + sizeof(WCHAR));
            }

            UserInfo23->usri23_flags = GetAccountFlags(UserInfo->UserAccountControl);

            /* FIXME: usri23_user_sid */
           break;
    }

done:
    if (ApiStatus == NERR_Success)
    {
        *Buffer = LocalBuffer;
    }
    else
    {
        if (LocalBuffer != NULL)
            NetApiBufferFree(LocalBuffer);
    }

    return ApiStatus;
}


static
VOID
FreeUserInfo(PUSER_ACCOUNT_INFORMATION UserInfo)
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


static
NET_API_STATUS
SetUserInfo(SAM_HANDLE UserHandle,
            LPBYTE UserInfo,
            DWORD Level)
{
    USER_ALL_INFORMATION UserAllInfo;
    PUSER_INFO_1 UserInfo1;
    PUSER_INFO_3 UserInfo3;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    ZeroMemory(&UserAllInfo, sizeof(USER_ALL_INFORMATION));

    switch (Level)
    {
        case 1:
            UserInfo1 = (PUSER_INFO_1)UserInfo;
//            RtlInitUnicodeString(&UserAllInfo.UserName,
//                                 UserInfo1->usri1_name);

            RtlInitUnicodeString(&UserAllInfo.AdminComment,
                                 UserInfo1->usri1_comment);

            RtlInitUnicodeString(&UserAllInfo.HomeDirectory,
                                 UserInfo1->usri1_home_dir);

            RtlInitUnicodeString(&UserAllInfo.ScriptPath,
                                 UserInfo1->usri1_script_path);

            RtlInitUnicodeString(&UserAllInfo.NtPassword,
                                 UserInfo1->usri1_password);
            UserAllInfo.NtPasswordPresent = TRUE;

//          UserInfo1->usri1_flags
//          UserInfo1->usri1_priv

            UserAllInfo.WhichFields = 
                USER_ALL_ADMINCOMMENT |
                USER_ALL_HOMEDIRECTORY |
                USER_ALL_SCRIPTPATH |
                USER_ALL_NTPASSWORDPRESENT
//                USER_ALL_USERACCOUNTCONTROL
                ;
            break;


        case 3:
            UserInfo3 = (PUSER_INFO_3)UserInfo;

//  LPWSTR usri3_name;

            RtlInitUnicodeString(&UserAllInfo.NtPassword,
                                 UserInfo3->usri3_password);
            UserAllInfo.NtPasswordPresent = TRUE;

//  DWORD  usri3_password_age; // ignored
//  DWORD  usri3_priv;

            RtlInitUnicodeString(&UserAllInfo.HomeDirectory,
                                 UserInfo3->usri3_home_dir);

            RtlInitUnicodeString(&UserAllInfo.AdminComment,
                                 UserInfo3->usri3_comment);

//  DWORD  usri3_flags;

            RtlInitUnicodeString(&UserAllInfo.ScriptPath,
                                 UserInfo3->usri3_script_path);

//  DWORD  usri3_auth_flags;

            RtlInitUnicodeString(&UserAllInfo.FullName,
                                 UserInfo3->usri3_full_name);

//  LPWSTR usri3_usr_comment;
//  LPWSTR usri3_parms;
//  LPWSTR usri3_workstations;
//  DWORD  usri3_last_logon;
//  DWORD  usri3_last_logoff;
//  DWORD  usri3_acct_expires;
//  DWORD  usri3_max_storage;
//  DWORD  usri3_units_per_week;
//  PBYTE  usri3_logon_hours;
//  DWORD  usri3_bad_pw_count;
//  DWORD  usri3_num_logons;
//  LPWSTR usri3_logon_server;
//  DWORD  usri3_country_code;
//  DWORD  usri3_code_page;
//  DWORD  usri3_user_id;  // ignored
//  DWORD  usri3_primary_group_id;
//  LPWSTR usri3_profile;
//  LPWSTR usri3_home_dir_drive;
//  DWORD  usri3_password_expired;

            UserAllInfo.WhichFields = 
                USER_ALL_NTPASSWORDPRESENT |
                USER_ALL_HOMEDIRECTORY |
                USER_ALL_ADMINCOMMENT |
                USER_ALL_SCRIPTPATH |
                USER_ALL_FULLNAME
//                USER_ALL_USERACCOUNTCONTROL
                ;
            break;

        default:
            ERR("Unsupported level %lu!\n", Level);
            return ERROR_INVALID_PARAMETER;
    }

    Status = SamSetInformationUser(UserHandle,
                                   UserAllInformation,
                                   &UserAllInfo);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamSetInformationUser failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

done:
    return ApiStatus;
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
    UNICODE_STRING ServerName;
    UNICODE_STRING UserName;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE UserHandle = NULL;
    ULONG GrantedAccess;
    ULONG RelativeId;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("(%s, %d, %p, %p)\n", debugstr_w(servername), level, bufptr, parm_err);

    /* Check the info level */
    if (level < 1 || level > 4)
        return ERROR_INVALID_LEVEL;

    if (servername != NULL)
        RtlInitUnicodeString(&ServerName, servername);

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

    /* Open the Account Domain */
    Status = OpenAccountDomain(ServerHandle,
                               (servername != NULL) ? &ServerName : NULL,
                               DOMAIN_CREATE_USER | DOMAIN_LOOKUP,
                               &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("OpenAccountDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Initialize the user name string */
    RtlInitUnicodeString(&UserName,
                         ((PUSER_INFO_1)bufptr)->usri1_name);

    /* Create the user account */
    Status = SamCreateUser2InDomain(DomainHandle,
                                    &UserName,
                                    USER_NORMAL_ACCOUNT,
                                    USER_ALL_ACCESS | DELETE | WRITE_DAC,
                                    &UserHandle,
                                    &GrantedAccess,
                                    &RelativeId);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamCreateUser2InDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Set user information */
    ApiStatus = SetUserInfo(UserHandle,
                            bufptr,
                            level);
    if (ApiStatus != NERR_Success)
    {
        ERR("SetUserInfo failed (Status %lu)\n", ApiStatus);
        goto done;
    }

done:
    if (UserHandle != NULL)
        SamCloseHandle(UserHandle);

    if (DomainHandle != NULL)
        SamCloseHandle(DomainHandle);

    if (ServerHandle != NULL)
        SamCloseHandle(ServerHandle);

    return ApiStatus;
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
    UNICODE_STRING ServerName;
    PSAM_RID_ENUMERATION CurrentUser;
    PENUM_CONTEXT EnumContext = NULL;
    LPVOID Buffer = NULL;
    ULONG i;
    SAM_HANDLE UserHandle = NULL;
    PUSER_ACCOUNT_INFORMATION UserInfo = NULL;

    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    FIXME("(%s %d 0x%d %p %d %p %p %p) stub!\n", debugstr_w(servername), level,
          filter, bufptr, prefmaxlen, entriesread, totalentries, resume_handle);

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
        EnumContext->Count = 0;
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
    }

//    while (TRUE)
//    {
        TRACE("EnumContext->Index: %lu\n", EnumContext->Index);
        TRACE("EnumContext->Count: %lu\n", EnumContext->Count);

        if (EnumContext->Index >= EnumContext->Count)
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
                                               &EnumContext->Count);

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
        TRACE("EnumContext->Count: %lu\n", EnumContext->Count);
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

        ApiStatus = BuildUserInfoBuffer(UserInfo,
                                        level,
                                        CurrentUser->RelativeId,
                                        &Buffer);
        if (ApiStatus != NERR_Success)
        {
            ERR("BuildUserInfoBuffer failed (ApiStatus %lu)\n", ApiStatus);
            goto done;
        }

        if (UserInfo != NULL)
        {
            FreeUserInfo(UserInfo);
            UserInfo = NULL;
        }

        EnumContext->Index++;

        (*entriesread)++;
//    }

done:
    if (ApiStatus == NERR_Success && EnumContext->Index < EnumContext->Count)
        ApiStatus = ERROR_MORE_DATA;

    if (EnumContext != NULL)
        *totalentries = EnumContext->Count;

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
                for (i = 0; i < EnumContext->Count; i++)
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
        FreeUserInfo(UserInfo);

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
    UNICODE_STRING ServerName;
    UNICODE_STRING UserName;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE AccountDomainHandle = NULL;
    SAM_HANDLE UserHandle = NULL;
    PULONG RelativeIds = NULL;
    PSID_NAME_USE Use = NULL;
    PUSER_ACCOUNT_INFORMATION UserInfo = NULL;
    LPVOID Buffer = NULL;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("(%s, %s, %d, %p)\n", debugstr_w(servername),
          debugstr_w(username), level, bufptr);

    if (servername != NULL)
        RtlInitUnicodeString(&ServerName, servername);

    RtlInitUnicodeString(&UserName, username);

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

    /* Open the Account Domain */
    Status = OpenAccountDomain(ServerHandle,
                               (servername != NULL) ? &ServerName : NULL,
                               DOMAIN_LIST_ACCOUNTS | DOMAIN_LOOKUP,
                               &AccountDomainHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("OpenAccountDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Get the RID for the given user name */
    Status = SamLookupNamesInDomain(AccountDomainHandle,
                                    1,
                                    &UserName,
                                    &RelativeIds,
                                    &Use);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamOpenDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Check if the account is a user account */
    if (Use[0] != SidTypeUser)
    {
        ERR("No user found!\n");
        ApiStatus = NERR_UserNotFound;
        goto done;
    }

    TRACE("RID: %lu\n", RelativeIds[0]);

    /* Open the user object */
    Status = SamOpenUser(AccountDomainHandle,
                         USER_READ_GENERAL | USER_READ_PREFERENCES | USER_READ_LOGON | USER_READ_ACCOUNT,
                         RelativeIds[0],
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

    ApiStatus = BuildUserInfoBuffer(UserInfo,
                                    level,
                                    RelativeIds[0],
                                    &Buffer);
    if (ApiStatus != NERR_Success)
    {
        ERR("BuildUserInfoBuffer failed (ApiStatus %08lu)\n", ApiStatus);
        goto done;
    }

done:
    if (UserInfo != NULL)
        FreeUserInfo(UserInfo);

    if (UserHandle != NULL)
        SamCloseHandle(UserHandle);

    if (RelativeIds != NULL)
        SamFreeMemory(RelativeIds);

    if (Use != NULL)
        SamFreeMemory(Use);

    if (AccountDomainHandle != NULL)
        SamCloseHandle(AccountDomainHandle);

    if (ServerHandle != NULL)
        SamCloseHandle(ServerHandle);

    *bufptr = (LPBYTE)Buffer;

    return ApiStatus;
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
    UNICODE_STRING ServerName;
    UNICODE_STRING UserName;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE BuiltinDomainHandle = NULL;
    SAM_HANDLE AccountDomainHandle = NULL;
    PSID AccountDomainSid = NULL;
    PSID UserSid = NULL;
    PULONG RelativeIds = NULL;
    PSID_NAME_USE Use = NULL;
    ULONG BuiltinMemberCount = 0;
    ULONG AccountMemberCount = 0;
    PULONG BuiltinAliases = NULL;
    PULONG AccountAliases = NULL;
    PUNICODE_STRING BuiltinNames = NULL;
    PUNICODE_STRING AccountNames = NULL;
    PLOCALGROUP_USERS_INFO_0 Buffer = NULL;
    ULONG Size;
    ULONG Count = 0;
    ULONG Index;
    ULONG i;
    LPWSTR StrPtr;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("(%s, %s, %d, %08x, %p %d, %p, %p) stub!\n",
          debugstr_w(servername), debugstr_w(username), level, flags, bufptr,
          prefmaxlen, entriesread, totalentries);

    if (level != 0)
        return ERROR_INVALID_LEVEL;

    if (flags & ~LG_INCLUDE_INDIRECT)
        return ERROR_INVALID_PARAMETER;

    if (flags & LG_INCLUDE_INDIRECT)
    {
        WARN("The flag LG_INCLUDE_INDIRECT is not supported yet!\n");
    }

    if (servername != NULL)
        RtlInitUnicodeString(&ServerName, servername);

    RtlInitUnicodeString(&UserName, username);

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

    /* Open the Builtin Domain */
    Status = OpenBuiltinDomain(ServerHandle,
                               DOMAIN_LOOKUP | DOMAIN_GET_ALIAS_MEMBERSHIP,
                               &BuiltinDomainHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("OpenBuiltinDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Get the Account Domain SID */
    Status = GetAccountDomainSid((servername != NULL) ? &ServerName : NULL,
                                 &AccountDomainSid);
    if (!NT_SUCCESS(Status))
    {
        ERR("GetAccountDomainSid failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Open the Account Domain */
    Status = SamOpenDomain(ServerHandle,
                           DOMAIN_LOOKUP | DOMAIN_GET_ALIAS_MEMBERSHIP,
                           AccountDomainSid,
                           &AccountDomainHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("OpenAccountDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Get the RID for the given user name */
    Status = SamLookupNamesInDomain(AccountDomainHandle,
                                    1,
                                    &UserName,
                                    &RelativeIds,
                                    &Use);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamLookupNamesInDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Fail, if it is not a user account */
    if (Use[0] != SidTypeUser)
    {
        ERR("Account is not a User!\n");
        ApiStatus = NERR_UserNotFound;
        goto done;
    }

    /* Build the User SID from the Account Domain SID and the users RID */
    UserSid = CreateSidFromSidAndRid(AccountDomainSid,
                                     RelativeIds[0]);
    if (UserSid == NULL)
    {
        ERR("CreateSidFromSidAndRid failed!\n");
        ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    /* Get alias memberships in the Builtin Domain */
    Status = SamGetAliasMembership(BuiltinDomainHandle,
                                   1,
                                   &UserSid,
                                   &BuiltinMemberCount,
                                   &BuiltinAliases);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamGetAliasMembership failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    if (BuiltinMemberCount > 0)
    {
        /* Get the Names of the builtin alias members */
        Status = SamLookupIdsInDomain(BuiltinDomainHandle,
                                      BuiltinMemberCount,
                                      BuiltinAliases,
                                      &BuiltinNames,
                                      NULL);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamLookupIdsInDomain failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }
    }

    /* Get alias memberships in the Account Domain */
    Status = SamGetAliasMembership(AccountDomainHandle,
                                   1,
                                   &UserSid,
                                   &AccountMemberCount,
                                   &AccountAliases);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamGetAliasMembership failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    if (AccountMemberCount > 0)
    {
        /* Get the Names of the builtin alias members */
        Status = SamLookupIdsInDomain(AccountDomainHandle,
                                      AccountMemberCount,
                                      AccountAliases,
                                      &AccountNames,
                                      NULL);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamLookupIdsInDomain failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }
    }

    /* Calculate the required buffer size */
    Size = 0;

    for (i = 0; i < BuiltinMemberCount; i++)
    {
        if (BuiltinNames[i].Length > 0)
        {
            Size += (sizeof(LOCALGROUP_USERS_INFO_0) + BuiltinNames[i].Length + sizeof(UNICODE_NULL));
            Count++;
        }
    }

    for (i = 0; i < AccountMemberCount; i++)
    {
        if (BuiltinNames[i].Length > 0)
        {
            Size += (sizeof(LOCALGROUP_USERS_INFO_0) + AccountNames[i].Length + sizeof(UNICODE_NULL));
            Count++;
        }
    }

    if (Size == 0)
    {
        ApiStatus = NERR_Success;
        goto done;
    }

    /* Allocate buffer */
    ApiStatus = NetApiBufferAllocate(Size, (LPVOID*)&Buffer);
    if (ApiStatus != NERR_Success)
        goto done;

    ZeroMemory(Buffer, Size);

    StrPtr = (LPWSTR)((INT_PTR)Buffer + Count * sizeof(LOCALGROUP_USERS_INFO_0));

    /* Copy data to the allocated buffer */
    Index = 0;
    for (i = 0; i < BuiltinMemberCount; i++)
    {
        if (BuiltinNames[i].Length > 0)
        {
            CopyMemory(StrPtr,
                       BuiltinNames[i].Buffer,
                       BuiltinNames[i].Length);
            Buffer[Index].lgrui0_name = StrPtr;

            StrPtr = (LPWSTR)((INT_PTR)StrPtr + BuiltinNames[i].Length + sizeof(UNICODE_NULL));
            Index++;
        }
    }

    for (i = 0; i < AccountMemberCount; i++)
    {
        if (AccountNames[i].Length > 0)
        {
            CopyMemory(StrPtr,
                       AccountNames[i].Buffer,
                       AccountNames[i].Length);
            Buffer[Index].lgrui0_name = StrPtr;

            StrPtr = (LPWSTR)((INT_PTR)StrPtr + AccountNames[i].Length + sizeof(UNICODE_NULL));
            Index++;
        }
    }

done:
    if (AccountNames != NULL)
        SamFreeMemory(AccountNames);

    if (BuiltinNames != NULL)
        SamFreeMemory(BuiltinNames);

    if (AccountAliases != NULL)
        SamFreeMemory(AccountAliases);

    if (BuiltinAliases != NULL)
        SamFreeMemory(BuiltinAliases);

    if (RelativeIds != NULL)
        SamFreeMemory(RelativeIds);

    if (Use != NULL)
        SamFreeMemory(Use);

    if (UserSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, UserSid);

    if (AccountDomainSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AccountDomainSid);

    if (AccountDomainHandle != NULL)
        SamCloseHandle(AccountDomainHandle);

    if (BuiltinDomainHandle != NULL)
        SamCloseHandle(BuiltinDomainHandle);

    if (ServerHandle != NULL)
        SamCloseHandle(ServerHandle);

    if (ApiStatus != NERR_Success && ApiStatus != ERROR_MORE_DATA)
    {
        *entriesread = 0;
        *totalentries = 0;
    }
    else
    {
        *entriesread = Count;
        *totalentries = Count;
    }

    *bufptr = (LPBYTE)Buffer;

    return ApiStatus;
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
