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
 *    Implement NetUserGetGroups (WIP)
 *    Implement NetUserSetGroups
 *    NetUserGetLocalGroups does not support LG_INCLUDE_INDIRECT yet.
 *    Add missing information levels.
 *    ...
 */

#include "netapi32.h"

#include <ndk/kefuncs.h>
#include <ndk/obfuncs.h>

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


static
ULONG
DeltaTimeToSeconds(LARGE_INTEGER DeltaTime)
{
    LARGE_INTEGER Seconds;

    if (DeltaTime.QuadPart == 0)
        return 0;

    Seconds.QuadPart = -DeltaTime.QuadPart / 10000000;

    if (Seconds.HighPart != 0)
        return TIMEQ_FOREVER;

    return Seconds.LowPart;
}


static
NTSTATUS
GetAllowedWorldAce(IN PACL Acl,
                   OUT PACCESS_ALLOWED_ACE *Ace)
{
    SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
    ULONG WorldSid[sizeof(SID) / sizeof(ULONG) + SID_MAX_SUB_AUTHORITIES];
    ACL_SIZE_INFORMATION AclSize;
    PVOID LocalAce = NULL;
    ULONG i;
    NTSTATUS Status;

    *Ace = NULL;

    RtlInitializeSid((PSID)WorldSid,
                     &WorldAuthority,
                     1);
    *(RtlSubAuthoritySid((PSID)WorldSid, 0)) = SECURITY_WORLD_RID;

    Status = RtlQueryInformationAcl(Acl,
                                    &AclSize,
                                    sizeof(AclSize),
                                    AclSizeInformation);
    if (!NT_SUCCESS(Status))
        return Status;

    for (i = 0; i < AclSize.AceCount; i++)
    {
        Status = RtlGetAce(Acl, i, &LocalAce);
        if (!NT_SUCCESS(Status))
            return Status;

        if (((PACE_HEADER)LocalAce)->AceType != ACCESS_ALLOWED_ACE_TYPE)
            continue;

        if (RtlEqualSid((PSID)WorldSid,
                        (PSID)&((PACCESS_ALLOWED_ACE)LocalAce)->SidStart))
        {
            *Ace = (PACCESS_ALLOWED_ACE)LocalAce;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_SUCCESS;
}


static
ULONG
GetAccountFlags(ULONG AccountControl,
                PACL Dacl)
{
    PACCESS_ALLOWED_ACE Ace = NULL;
    ULONG Flags = UF_SCRIPT;
    NTSTATUS Status;

    if (Dacl != NULL)
    {
        Status = GetAllowedWorldAce(Dacl, &Ace);
        if (NT_SUCCESS(Status))
        {
            if (Ace == NULL)
            {
                Flags |= UF_PASSWD_CANT_CHANGE;
            }
            else if ((Ace->Mask & USER_CHANGE_PASSWORD) == 0)
            {
                Flags |= UF_PASSWD_CANT_CHANGE;
            }
        }
    }

    if (AccountControl & USER_ACCOUNT_DISABLED)
        Flags |= UF_ACCOUNTDISABLE;

    if (AccountControl & USER_HOME_DIRECTORY_REQUIRED)
        Flags |= UF_HOMEDIR_REQUIRED;

    if (AccountControl & USER_PASSWORD_NOT_REQUIRED)
        Flags |= UF_PASSWD_NOTREQD;

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
ULONG
GetAccountControl(ULONG Flags)
{
    ULONG AccountControl = 0;

    if (Flags & UF_ACCOUNTDISABLE)
        AccountControl |= USER_ACCOUNT_DISABLED;

    if (Flags & UF_HOMEDIR_REQUIRED)
        AccountControl |= USER_HOME_DIRECTORY_REQUIRED;

    if (Flags & UF_PASSWD_NOTREQD)
        AccountControl |= USER_PASSWORD_NOT_REQUIRED;

    if (Flags & UF_LOCKOUT)
        AccountControl |= USER_ACCOUNT_AUTO_LOCKED;

    if (Flags & UF_DONT_EXPIRE_PASSWD)
        AccountControl |= USER_DONT_EXPIRE_PASSWORD;

    /* Set account type flags */
    if (Flags & UF_TEMP_DUPLICATE_ACCOUNT)
        AccountControl |= USER_TEMP_DUPLICATE_ACCOUNT;
    else if (Flags & UF_NORMAL_ACCOUNT)
        AccountControl |= USER_NORMAL_ACCOUNT;
    else if (Flags & UF_INTERDOMAIN_TRUST_ACCOUNT)
        AccountControl |= USER_INTERDOMAIN_TRUST_ACCOUNT;
    else if (Flags & UF_WORKSTATION_TRUST_ACCOUNT)
        AccountControl |= USER_WORKSTATION_TRUST_ACCOUNT;
    else if (Flags & UF_SERVER_TRUST_ACCOUNT)
        AccountControl |= USER_SERVER_TRUST_ACCOUNT;

    return AccountControl;
}


static
DWORD
GetPasswordAge(IN PLARGE_INTEGER PasswordLastSet)
{
    LARGE_INTEGER SystemTime;
    ULONG SystemSecondsSince1970;
    ULONG PasswordSecondsSince1970;
    NTSTATUS Status;

    Status = NtQuerySystemTime(&SystemTime);
    if (!NT_SUCCESS(Status))
        return 0;

    RtlTimeToSecondsSince1970(&SystemTime, &SystemSecondsSince1970);
    RtlTimeToSecondsSince1970(PasswordLastSet, &PasswordSecondsSince1970);

    return SystemSecondsSince1970 - PasswordSecondsSince1970;
}


static
VOID
ChangeUserDacl(IN PACL Dacl,
               IN ULONG Flags)
{
    PACCESS_ALLOWED_ACE Ace = NULL;
    NTSTATUS Status;

    if (Dacl == NULL)
        return;

    Status = GetAllowedWorldAce(Dacl, &Ace);
    if (!NT_SUCCESS(Status))
        return;

    if (Flags & UF_PASSWD_CANT_CHANGE)
        Ace->Mask &= ~USER_CHANGE_PASSWORD;
    else
        Ace->Mask |= USER_CHANGE_PASSWORD;
}


static
NET_API_STATUS
GetUserDacl(IN SAM_HANDLE UserHandle,
            OUT PACL *Dacl)
{
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    PACL SamDacl;
    PACL LocalDacl;
    BOOLEAN Defaulted;
    BOOLEAN Present;
    ACL_SIZE_INFORMATION AclSize;
    NET_API_STATUS ApiStatus;
    NTSTATUS Status;

    TRACE("(%p %p)\n", UserHandle, Dacl);

    *Dacl = NULL;

    Status = SamQuerySecurityObject(UserHandle,
                                    DACL_SECURITY_INFORMATION,
                                    &SecurityDescriptor);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamQuerySecurityObject() failed (Status 0x%08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
                                          &Present,
                                          &SamDacl,
                                          &Defaulted);
    if (!NT_SUCCESS(Status))
    {
        TRACE("RtlGetDaclSecurityDescriptor() failed (Status 0x%08lx)\n", Status);
        ApiStatus = NERR_InternalError;
        goto done;
    }

    if (Present == FALSE)
    {
        TRACE("No DACL present\n");
        ApiStatus = NERR_Success;
        goto done;
    }

    Status = RtlQueryInformationAcl(SamDacl,
                                    &AclSize,
                                    sizeof(AclSize),
                                    AclSizeInformation);
    if (!NT_SUCCESS(Status))
    {
        TRACE("RtlQueryInformationAcl() failed (Status 0x%08lx)\n", Status);
        ApiStatus = NERR_InternalError;
        goto done;
    }

    LocalDacl = HeapAlloc(GetProcessHeap(), 0, AclSize.AclBytesInUse);
    if (LocalDacl == NULL)
    {
        TRACE("Memory allocation failed\n");
        ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    RtlCopyMemory(LocalDacl, SamDacl, AclSize.AclBytesInUse);

    *Dacl = LocalDacl;

    ApiStatus = NERR_Success;

done:
    if (SecurityDescriptor != NULL)
        SamFreeMemory(SecurityDescriptor);

    TRACE("done (ApiStatus: 0x%08lx)\n", ApiStatus);

    return ApiStatus;
}


static
VOID
FreeUserInfo(PUSER_ALL_INFORMATION UserInfo)
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

    if (UserInfo->UserComment.Buffer != NULL)
        SamFreeMemory(UserInfo->UserComment.Buffer);

    if (UserInfo->Parameters.Buffer != NULL)
        SamFreeMemory(UserInfo->Parameters.Buffer);

    if (UserInfo->PrivateData.Buffer != NULL)
        SamFreeMemory(UserInfo->PrivateData.Buffer);

    if (UserInfo->LogonHours.LogonHours != NULL)
        SamFreeMemory(UserInfo->LogonHours.LogonHours);

    SamFreeMemory(UserInfo);
}


static
NET_API_STATUS
BuildUserInfoBuffer(SAM_HANDLE UserHandle,
                    DWORD level,
                    ULONG RelativeId,
                    LPVOID *Buffer)
{
    UNICODE_STRING LogonServer = RTL_CONSTANT_STRING(L"\\\\*");
    PUSER_ALL_INFORMATION UserInfo = NULL;
    LPVOID LocalBuffer = NULL;
    PACL Dacl = NULL;
    PUSER_INFO_0 UserInfo0;
    PUSER_INFO_1 UserInfo1;
    PUSER_INFO_2 UserInfo2;
    PUSER_INFO_3 UserInfo3;
    PUSER_INFO_4 UserInfo4;
    PUSER_INFO_10 UserInfo10;
    PUSER_INFO_11 UserInfo11;
    PUSER_INFO_20 UserInfo20;
    PUSER_INFO_23 UserInfo23;
    LPWSTR Ptr;
    ULONG Size = 0;
    NTSTATUS Status;
    NET_API_STATUS ApiStatus = NERR_Success;

    *Buffer = NULL;

    Status = SamQueryInformationUser(UserHandle,
                                     UserAllInformation,
                                     (PVOID *)&UserInfo);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamQueryInformationUser failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    if ((level == 1) || (level == 2) || (level == 3) ||
        (level == 4) || (level == 20) || (level == 23))
    {
        ApiStatus = GetUserDacl(UserHandle, &Dacl);
        if (ApiStatus != NERR_Success)
            goto done;
    }

    switch (level)
    {
        case 0:
            Size = sizeof(USER_INFO_0) +
                   UserInfo->UserName.Length + sizeof(WCHAR);
            break;

        case 1:
            Size = sizeof(USER_INFO_1) +
                   UserInfo->UserName.Length + sizeof(WCHAR) +
                   UserInfo->HomeDirectory.Length + sizeof(WCHAR) +
                   UserInfo->AdminComment.Length + sizeof(WCHAR) +
                   UserInfo->ScriptPath.Length + sizeof(WCHAR);
            break;

        case 2:
            Size = sizeof(USER_INFO_2) +
                   UserInfo->UserName.Length + sizeof(WCHAR) +
                   UserInfo->HomeDirectory.Length + sizeof(WCHAR) +
                   UserInfo->AdminComment.Length + sizeof(WCHAR) +
                   UserInfo->ScriptPath.Length + sizeof(WCHAR) +
                   UserInfo->FullName.Length + sizeof(WCHAR) +
                   UserInfo->UserComment.Length + sizeof(WCHAR) +
                   UserInfo->Parameters.Length + sizeof(WCHAR) +
                   UserInfo->WorkStations.Length + sizeof(WCHAR) +
                   LogonServer.Length + sizeof(WCHAR);

            if (UserInfo->LogonHours.UnitsPerWeek > 0)
                Size += (((ULONG)UserInfo->LogonHours.UnitsPerWeek) + 7) / 8;
            break;

        case 3:
            Size = sizeof(USER_INFO_3) +
                   UserInfo->UserName.Length + sizeof(WCHAR) +
                   UserInfo->HomeDirectory.Length + sizeof(WCHAR) +
                   UserInfo->AdminComment.Length + sizeof(WCHAR) +
                   UserInfo->ScriptPath.Length + sizeof(WCHAR) +
                   UserInfo->FullName.Length + sizeof(WCHAR) +
                   UserInfo->UserComment.Length + sizeof(WCHAR) +
                   UserInfo->Parameters.Length + sizeof(WCHAR) +
                   UserInfo->WorkStations.Length + sizeof(WCHAR) +
                   LogonServer.Length + sizeof(WCHAR) +
                   UserInfo->ProfilePath.Length + sizeof(WCHAR) +
                   UserInfo->HomeDirectoryDrive.Length + sizeof(WCHAR);

            if (UserInfo->LogonHours.UnitsPerWeek > 0)
                Size += (((ULONG)UserInfo->LogonHours.UnitsPerWeek) + 7) / 8;
            break;

        case 4:
            Size = sizeof(USER_INFO_4) +
                   UserInfo->UserName.Length + sizeof(WCHAR) +
                   UserInfo->HomeDirectory.Length + sizeof(WCHAR) +
                   UserInfo->AdminComment.Length + sizeof(WCHAR) +
                   UserInfo->ScriptPath.Length + sizeof(WCHAR) +
                   UserInfo->FullName.Length + sizeof(WCHAR) +
                   UserInfo->UserComment.Length + sizeof(WCHAR) +
                   UserInfo->Parameters.Length + sizeof(WCHAR) +
                   UserInfo->WorkStations.Length + sizeof(WCHAR) +
                   LogonServer.Length + sizeof(WCHAR) +
                   UserInfo->ProfilePath.Length + sizeof(WCHAR) +
                   UserInfo->HomeDirectoryDrive.Length + sizeof(WCHAR);

            if (UserInfo->LogonHours.UnitsPerWeek > 0)
                Size += (((ULONG)UserInfo->LogonHours.UnitsPerWeek) + 7) / 8;

            /* FIXME: usri4_user_sid */
            break;

        case 10:
            Size = sizeof(USER_INFO_10) +
                   UserInfo->UserName.Length + sizeof(WCHAR) +
                   UserInfo->AdminComment.Length + sizeof(WCHAR) +
                   UserInfo->UserComment.Length + sizeof(WCHAR) +
                   UserInfo->FullName.Length + sizeof(WCHAR);
            break;

        case 11:
            Size = sizeof(USER_INFO_11) +
                   UserInfo->UserName.Length + sizeof(WCHAR) +
                   UserInfo->AdminComment.Length + sizeof(WCHAR) +
                   UserInfo->UserComment.Length + sizeof(WCHAR) +
                   UserInfo->FullName.Length + sizeof(WCHAR) +
                   UserInfo->HomeDirectory.Length + sizeof(WCHAR) +
                   UserInfo->Parameters.Length + sizeof(WCHAR) +
                   LogonServer.Length + sizeof(WCHAR) +
                   UserInfo->WorkStations.Length + sizeof(WCHAR);

            if (UserInfo->LogonHours.UnitsPerWeek > 0)
                Size += (((ULONG)UserInfo->LogonHours.UnitsPerWeek) + 7) / 8;
            break;

        case 20:
            Size = sizeof(USER_INFO_20) +
                   UserInfo->UserName.Length + sizeof(WCHAR) +
                   UserInfo->FullName.Length + sizeof(WCHAR) +
                   UserInfo->AdminComment.Length + sizeof(WCHAR);
            break;

        case 23:
            Size = sizeof(USER_INFO_23) +
                   UserInfo->UserName.Length + sizeof(WCHAR) +
                   UserInfo->FullName.Length + sizeof(WCHAR) +
                   UserInfo->AdminComment.Length + sizeof(WCHAR);

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
            UserInfo1->usri1_password_age = GetPasswordAge(&UserInfo->PasswordLastSet);

            /* FIXME: usri1_priv */

            UserInfo1->usri1_home_dir = Ptr;
            memcpy(UserInfo1->usri1_home_dir,
                   UserInfo->HomeDirectory.Buffer,
                   UserInfo->HomeDirectory.Length);
            UserInfo1->usri1_home_dir[UserInfo->HomeDirectory.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->HomeDirectory.Length + sizeof(WCHAR));

            UserInfo1->usri1_comment = Ptr;
            memcpy(UserInfo1->usri1_comment,
                   UserInfo->AdminComment.Buffer,
                   UserInfo->AdminComment.Length);
            UserInfo1->usri1_comment[UserInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->AdminComment.Length + sizeof(WCHAR));

            UserInfo1->usri1_flags = GetAccountFlags(UserInfo->UserAccountControl,
                                                     Dacl);

            UserInfo1->usri1_script_path = Ptr;
            memcpy(UserInfo1->usri1_script_path,
                   UserInfo->ScriptPath.Buffer,
                   UserInfo->ScriptPath.Length);
            UserInfo1->usri1_script_path[UserInfo->ScriptPath.Length / sizeof(WCHAR)] = UNICODE_NULL;
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

            UserInfo2->usri2_password = NULL;
            UserInfo2->usri2_password_age = GetPasswordAge(&UserInfo->PasswordLastSet);

            /* FIXME: usri2_priv */

            UserInfo2->usri2_home_dir = Ptr;
            memcpy(UserInfo2->usri2_home_dir,
                   UserInfo->HomeDirectory.Buffer,
                   UserInfo->HomeDirectory.Length);
            UserInfo2->usri2_home_dir[UserInfo->HomeDirectory.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->HomeDirectory.Length + sizeof(WCHAR));

            UserInfo2->usri2_comment = Ptr;
            memcpy(UserInfo2->usri2_comment,
                   UserInfo->AdminComment.Buffer,
                   UserInfo->AdminComment.Length);
            UserInfo2->usri2_comment[UserInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->AdminComment.Length + sizeof(WCHAR));

            UserInfo2->usri2_flags = GetAccountFlags(UserInfo->UserAccountControl,
                                                     Dacl);

            UserInfo2->usri2_script_path = Ptr;
            memcpy(UserInfo2->usri2_script_path,
                   UserInfo->ScriptPath.Buffer,
                   UserInfo->ScriptPath.Length);
            UserInfo2->usri2_script_path[UserInfo->ScriptPath.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->ScriptPath.Length + sizeof(WCHAR));

            /* FIXME: usri2_auth_flags */

            UserInfo2->usri2_full_name = Ptr;
            memcpy(UserInfo2->usri2_full_name,
                   UserInfo->FullName.Buffer,
                   UserInfo->FullName.Length);
            UserInfo2->usri2_full_name[UserInfo->FullName.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->FullName.Length + sizeof(WCHAR));

            UserInfo2->usri2_usr_comment = Ptr;
            memcpy(UserInfo2->usri2_usr_comment,
                   UserInfo->UserComment.Buffer,
                   UserInfo->UserComment.Length);
            UserInfo2->usri2_usr_comment[UserInfo->UserComment.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->UserComment.Length + sizeof(WCHAR));

            UserInfo2->usri2_parms = Ptr;
            memcpy(UserInfo2->usri2_parms,
                   UserInfo->Parameters.Buffer,
                   UserInfo->Parameters.Length);
            UserInfo2->usri2_parms[UserInfo->Parameters.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->Parameters.Length + sizeof(WCHAR));

            UserInfo2->usri2_workstations = Ptr;
            memcpy(UserInfo2->usri2_workstations,
                   UserInfo->WorkStations.Buffer,
                   UserInfo->WorkStations.Length);
            UserInfo2->usri2_workstations[UserInfo->WorkStations.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->WorkStations.Length + sizeof(WCHAR));

            RtlTimeToSecondsSince1970(&UserInfo->LastLogon,
                                      &UserInfo2->usri2_last_logon);

            RtlTimeToSecondsSince1970(&UserInfo->LastLogoff,
                                      &UserInfo2->usri2_last_logoff);

            RtlTimeToSecondsSince1970(&UserInfo->AccountExpires,
                                      &UserInfo2->usri2_acct_expires);

            UserInfo2->usri2_max_storage = USER_MAXSTORAGE_UNLIMITED;
            UserInfo2->usri2_units_per_week = UserInfo->LogonHours.UnitsPerWeek;

            if (UserInfo->LogonHours.UnitsPerWeek > 0)
            {
                UserInfo2->usri2_logon_hours = (PVOID)Ptr;

                memcpy(UserInfo2->usri2_logon_hours,
                       UserInfo->LogonHours.LogonHours,
                       (((ULONG)UserInfo->LogonHours.UnitsPerWeek) + 7) / 8);

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + (((ULONG)UserInfo->LogonHours.UnitsPerWeek) + 7) / 8);
            }

            UserInfo2->usri2_bad_pw_count = UserInfo->BadPasswordCount;
            UserInfo2->usri2_num_logons = UserInfo->LogonCount;

            UserInfo2->usri2_logon_server = Ptr;
            memcpy(UserInfo2->usri2_logon_server,
                   LogonServer.Buffer,
                   LogonServer.Length);
            UserInfo2->usri2_logon_server[LogonServer.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + LogonServer.Length + sizeof(WCHAR));

            UserInfo2->usri2_country_code = UserInfo->CountryCode;
            UserInfo2->usri2_code_page = UserInfo->CodePage;
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

            UserInfo3->usri3_password = NULL;
            UserInfo3->usri3_password_age = GetPasswordAge(&UserInfo->PasswordLastSet);

            /* FIXME: usri3_priv */

            UserInfo3->usri3_home_dir = Ptr;
            memcpy(UserInfo3->usri3_home_dir,
                   UserInfo->HomeDirectory.Buffer,
                   UserInfo->HomeDirectory.Length);
            UserInfo3->usri3_home_dir[UserInfo->HomeDirectory.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->HomeDirectory.Length + sizeof(WCHAR));

            UserInfo3->usri3_comment = Ptr;
            memcpy(UserInfo3->usri3_comment,
                   UserInfo->AdminComment.Buffer,
                   UserInfo->AdminComment.Length);
            UserInfo3->usri3_comment[UserInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->AdminComment.Length + sizeof(WCHAR));

            UserInfo3->usri3_flags = GetAccountFlags(UserInfo->UserAccountControl,
                                                     Dacl);

            UserInfo3->usri3_script_path = Ptr;
            memcpy(UserInfo3->usri3_script_path,
                   UserInfo->ScriptPath.Buffer,
                   UserInfo->ScriptPath.Length);
            UserInfo3->usri3_script_path[UserInfo->ScriptPath.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->ScriptPath.Length + sizeof(WCHAR));

            /* FIXME: usri3_auth_flags */

            UserInfo3->usri3_full_name = Ptr;
            memcpy(UserInfo3->usri3_full_name,
                   UserInfo->FullName.Buffer,
                   UserInfo->FullName.Length);
            UserInfo3->usri3_full_name[UserInfo->FullName.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->FullName.Length + sizeof(WCHAR));

            UserInfo3->usri3_usr_comment = Ptr;
            memcpy(UserInfo3->usri3_usr_comment,
                   UserInfo->UserComment.Buffer,
                   UserInfo->UserComment.Length);
            UserInfo3->usri3_usr_comment[UserInfo->UserComment.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->UserComment.Length + sizeof(WCHAR));

            UserInfo3->usri3_parms = Ptr;
            memcpy(UserInfo3->usri3_parms,
                   UserInfo->Parameters.Buffer,
                   UserInfo->Parameters.Length);
            UserInfo3->usri3_parms[UserInfo->Parameters.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->Parameters.Length + sizeof(WCHAR));

            UserInfo3->usri3_workstations = Ptr;
            memcpy(UserInfo3->usri3_workstations,
                   UserInfo->WorkStations.Buffer,
                   UserInfo->WorkStations.Length);
            UserInfo3->usri3_workstations[UserInfo->WorkStations.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->WorkStations.Length + sizeof(WCHAR));

            RtlTimeToSecondsSince1970(&UserInfo->LastLogon,
                                      &UserInfo3->usri3_last_logon);

            RtlTimeToSecondsSince1970(&UserInfo->LastLogoff,
                                      &UserInfo3->usri3_last_logoff);

            RtlTimeToSecondsSince1970(&UserInfo->AccountExpires,
                                      &UserInfo3->usri3_acct_expires);

            UserInfo3->usri3_max_storage = USER_MAXSTORAGE_UNLIMITED;
            UserInfo3->usri3_units_per_week = UserInfo->LogonHours.UnitsPerWeek;

            if (UserInfo->LogonHours.UnitsPerWeek > 0)
            {
                UserInfo3->usri3_logon_hours = (PVOID)Ptr;

                memcpy(UserInfo3->usri3_logon_hours,
                       UserInfo->LogonHours.LogonHours,
                       (((ULONG)UserInfo->LogonHours.UnitsPerWeek) + 7) / 8);

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + (((ULONG)UserInfo->LogonHours.UnitsPerWeek) + 7) / 8);
            }

            UserInfo3->usri3_bad_pw_count = UserInfo->BadPasswordCount;
            UserInfo3->usri3_num_logons = UserInfo->LogonCount;

            UserInfo3->usri3_logon_server = Ptr;
            memcpy(UserInfo3->usri3_logon_server,
                   LogonServer.Buffer,
                   LogonServer.Length);
            UserInfo3->usri3_logon_server[LogonServer.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + LogonServer.Length + sizeof(WCHAR));

            UserInfo3->usri3_country_code = UserInfo->CountryCode;
            UserInfo3->usri3_code_page = UserInfo->CodePage;
            UserInfo3->usri3_user_id = RelativeId;
            UserInfo3->usri3_primary_group_id = UserInfo->PrimaryGroupId;

            UserInfo3->usri3_profile = Ptr;
            memcpy(UserInfo3->usri3_profile,
                   UserInfo->ProfilePath.Buffer,
                   UserInfo->ProfilePath.Length);
            UserInfo3->usri3_profile[UserInfo->ProfilePath.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->ProfilePath.Length + sizeof(WCHAR));

            UserInfo3->usri3_home_dir_drive = Ptr;
            memcpy(UserInfo3->usri3_home_dir_drive,
                   UserInfo->HomeDirectoryDrive.Buffer,
                   UserInfo->HomeDirectoryDrive.Length);
            UserInfo3->usri3_home_dir_drive[UserInfo->HomeDirectoryDrive.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->HomeDirectoryDrive.Length + sizeof(WCHAR));

            UserInfo3->usri3_password_expired = (UserInfo->UserAccountControl & USER_PASSWORD_EXPIRED);
            break;

        case 4:
            UserInfo4 = (PUSER_INFO_4)LocalBuffer;

            Ptr = (LPWSTR)((ULONG_PTR)UserInfo4 + sizeof(USER_INFO_4));

            UserInfo4->usri4_name = Ptr;

            memcpy(UserInfo4->usri4_name,
                   UserInfo->UserName.Buffer,
                   UserInfo->UserName.Length);
            UserInfo4->usri4_name[UserInfo->UserName.Length / sizeof(WCHAR)] = UNICODE_NULL;

            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->UserName.Length + sizeof(WCHAR));

            UserInfo4->usri4_password = NULL;
            UserInfo4->usri4_password_age = GetPasswordAge(&UserInfo->PasswordLastSet);

            /* FIXME: usri4_priv */

            UserInfo4->usri4_home_dir = Ptr;
            memcpy(UserInfo4->usri4_home_dir,
                   UserInfo->HomeDirectory.Buffer,
                   UserInfo->HomeDirectory.Length);
            UserInfo4->usri4_home_dir[UserInfo->HomeDirectory.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->HomeDirectory.Length + sizeof(WCHAR));

            UserInfo4->usri4_comment = Ptr;
            memcpy(UserInfo4->usri4_comment,
                   UserInfo->AdminComment.Buffer,
                   UserInfo->AdminComment.Length);
            UserInfo4->usri4_comment[UserInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->AdminComment.Length + sizeof(WCHAR));

            UserInfo4->usri4_flags = GetAccountFlags(UserInfo->UserAccountControl,
                                                     Dacl);

            UserInfo4->usri4_script_path = Ptr;
            memcpy(UserInfo4->usri4_script_path,
                   UserInfo->ScriptPath.Buffer,
                   UserInfo->ScriptPath.Length);
            UserInfo4->usri4_script_path[UserInfo->ScriptPath.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->ScriptPath.Length + sizeof(WCHAR));

            /* FIXME: usri4_auth_flags */

            UserInfo4->usri4_full_name = Ptr;
            memcpy(UserInfo4->usri4_full_name,
                   UserInfo->FullName.Buffer,
                   UserInfo->FullName.Length);
            UserInfo4->usri4_full_name[UserInfo->FullName.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->FullName.Length + sizeof(WCHAR));

            UserInfo4->usri4_usr_comment = Ptr;
            memcpy(UserInfo4->usri4_usr_comment,
                   UserInfo->UserComment.Buffer,
                   UserInfo->UserComment.Length);
            UserInfo4->usri4_usr_comment[UserInfo->UserComment.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->UserComment.Length + sizeof(WCHAR));

            UserInfo4->usri4_parms = Ptr;
            memcpy(UserInfo4->usri4_parms,
                   UserInfo->Parameters.Buffer,
                   UserInfo->Parameters.Length);
            UserInfo4->usri4_parms[UserInfo->Parameters.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->Parameters.Length + sizeof(WCHAR));

            UserInfo4->usri4_workstations = Ptr;
            memcpy(UserInfo4->usri4_workstations,
                   UserInfo->WorkStations.Buffer,
                   UserInfo->WorkStations.Length);
            UserInfo4->usri4_workstations[UserInfo->WorkStations.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->WorkStations.Length + sizeof(WCHAR));

            RtlTimeToSecondsSince1970(&UserInfo->LastLogon,
                                      &UserInfo4->usri4_last_logon);

            RtlTimeToSecondsSince1970(&UserInfo->LastLogoff,
                                      &UserInfo4->usri4_last_logoff);

            RtlTimeToSecondsSince1970(&UserInfo->AccountExpires,
                                      &UserInfo4->usri4_acct_expires);

            UserInfo4->usri4_max_storage = USER_MAXSTORAGE_UNLIMITED;
            UserInfo4->usri4_units_per_week = UserInfo->LogonHours.UnitsPerWeek;

            if (UserInfo->LogonHours.UnitsPerWeek > 0)
            {
                UserInfo4->usri4_logon_hours = (PVOID)Ptr;

                memcpy(UserInfo4->usri4_logon_hours,
                       UserInfo->LogonHours.LogonHours,
                       (((ULONG)UserInfo->LogonHours.UnitsPerWeek) + 7) / 8);

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + (((ULONG)UserInfo->LogonHours.UnitsPerWeek) + 7) / 8);
            }

            UserInfo4->usri4_bad_pw_count = UserInfo->BadPasswordCount;
            UserInfo4->usri4_num_logons = UserInfo->LogonCount;

            UserInfo4->usri4_logon_server = Ptr;
            memcpy(UserInfo4->usri4_logon_server,
                   LogonServer.Buffer,
                   LogonServer.Length);
            UserInfo4->usri4_logon_server[LogonServer.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + LogonServer.Length + sizeof(WCHAR));

            UserInfo4->usri4_country_code = UserInfo->CountryCode;
            UserInfo4->usri4_code_page = UserInfo->CodePage;

            /* FIXME: usri4_user_sid */

            UserInfo4->usri4_primary_group_id = UserInfo->PrimaryGroupId;

            UserInfo4->usri4_profile = Ptr;
            memcpy(UserInfo4->usri4_profile,
                   UserInfo->ProfilePath.Buffer,
                   UserInfo->ProfilePath.Length);
            UserInfo4->usri4_profile[UserInfo->ProfilePath.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->ProfilePath.Length + sizeof(WCHAR));

            UserInfo4->usri4_home_dir_drive = Ptr;
            memcpy(UserInfo4->usri4_home_dir_drive,
                   UserInfo->HomeDirectoryDrive.Buffer,
                   UserInfo->HomeDirectoryDrive.Length);
            UserInfo4->usri4_home_dir_drive[UserInfo->HomeDirectoryDrive.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->HomeDirectoryDrive.Length + sizeof(WCHAR));

            UserInfo4->usri4_password_expired = (UserInfo->UserAccountControl & USER_PASSWORD_EXPIRED);
            break;

        case 10:
            UserInfo10 = (PUSER_INFO_10)LocalBuffer;

            Ptr = (LPWSTR)((ULONG_PTR)UserInfo10 + sizeof(USER_INFO_10));

            UserInfo10->usri10_name = Ptr;

            memcpy(UserInfo10->usri10_name,
                   UserInfo->UserName.Buffer,
                   UserInfo->UserName.Length);
            UserInfo10->usri10_name[UserInfo->UserName.Length / sizeof(WCHAR)] = UNICODE_NULL;

            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->UserName.Length + sizeof(WCHAR));

            UserInfo10->usri10_comment = Ptr;
            memcpy(UserInfo10->usri10_comment,
                   UserInfo->AdminComment.Buffer,
                   UserInfo->AdminComment.Length);
            UserInfo10->usri10_comment[UserInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->AdminComment.Length + sizeof(WCHAR));

            UserInfo10->usri10_usr_comment = Ptr;
            memcpy(UserInfo10->usri10_usr_comment,
                   UserInfo->UserComment.Buffer,
                   UserInfo->UserComment.Length);
            UserInfo10->usri10_usr_comment[UserInfo->UserComment.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->UserComment.Length + sizeof(WCHAR));

            UserInfo10->usri10_full_name = Ptr;
            memcpy(UserInfo10->usri10_full_name,
                   UserInfo->FullName.Buffer,
                   UserInfo->FullName.Length);
            UserInfo10->usri10_full_name[UserInfo->FullName.Length / sizeof(WCHAR)] = UNICODE_NULL;
            break;

        case 11:
            UserInfo11 = (PUSER_INFO_11)LocalBuffer;

            Ptr = (LPWSTR)((ULONG_PTR)UserInfo11 + sizeof(USER_INFO_11));

            UserInfo11->usri11_name = Ptr;

            memcpy(UserInfo11->usri11_name,
                   UserInfo->UserName.Buffer,
                   UserInfo->UserName.Length);
            UserInfo11->usri11_name[UserInfo->UserName.Length / sizeof(WCHAR)] = UNICODE_NULL;

            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->UserName.Length + sizeof(WCHAR));

            UserInfo11->usri11_comment = Ptr;
            memcpy(UserInfo11->usri11_comment,
                   UserInfo->AdminComment.Buffer,
                   UserInfo->AdminComment.Length);
            UserInfo11->usri11_comment[UserInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->AdminComment.Length + sizeof(WCHAR));

            UserInfo11->usri11_usr_comment = Ptr;
            memcpy(UserInfo11->usri11_usr_comment,
                   UserInfo->UserComment.Buffer,
                   UserInfo->UserComment.Length);
            UserInfo11->usri11_usr_comment[UserInfo->UserComment.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->UserComment.Length + sizeof(WCHAR));

            UserInfo11->usri11_full_name = Ptr;
            memcpy(UserInfo11->usri11_full_name,
                   UserInfo->FullName.Buffer,
                   UserInfo->FullName.Length);
            UserInfo11->usri11_full_name[UserInfo->FullName.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->FullName.Length + sizeof(WCHAR));

            /* FIXME: usri11_priv */
            /* FIXME: usri11_auth_flags */

            UserInfo11->usri11_password_age = GetPasswordAge(&UserInfo->PasswordLastSet);

            UserInfo11->usri11_home_dir = Ptr;
            memcpy(UserInfo11->usri11_home_dir,
                   UserInfo->HomeDirectory.Buffer,
                   UserInfo->HomeDirectory.Length);
            UserInfo11->usri11_home_dir[UserInfo->HomeDirectory.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->HomeDirectory.Length + sizeof(WCHAR));

            UserInfo11->usri11_parms = Ptr;
            memcpy(UserInfo11->usri11_parms,
                   UserInfo->Parameters.Buffer,
                   UserInfo->Parameters.Length);
            UserInfo11->usri11_parms[UserInfo->Parameters.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->Parameters.Length + sizeof(WCHAR));

            RtlTimeToSecondsSince1970(&UserInfo->LastLogon,
                                      &UserInfo11->usri11_last_logon);

            RtlTimeToSecondsSince1970(&UserInfo->LastLogoff,
                                      &UserInfo11->usri11_last_logoff);

            UserInfo11->usri11_bad_pw_count = UserInfo->BadPasswordCount;
            UserInfo11->usri11_num_logons = UserInfo->LogonCount;

            UserInfo11->usri11_logon_server = Ptr;
            memcpy(UserInfo11->usri11_logon_server,
                   LogonServer.Buffer,
                   LogonServer.Length);
            UserInfo11->usri11_logon_server[LogonServer.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + LogonServer.Length + sizeof(WCHAR));

            UserInfo11->usri11_country_code = UserInfo->CountryCode;

            UserInfo11->usri11_workstations = Ptr;
            memcpy(UserInfo11->usri11_workstations,
                   UserInfo->WorkStations.Buffer,
                   UserInfo->WorkStations.Length);
            UserInfo11->usri11_workstations[UserInfo->WorkStations.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->WorkStations.Length + sizeof(WCHAR));

            UserInfo11->usri11_max_storage = USER_MAXSTORAGE_UNLIMITED;
            UserInfo11->usri11_units_per_week = UserInfo->LogonHours.UnitsPerWeek;

            if (UserInfo->LogonHours.UnitsPerWeek > 0)
            {
                UserInfo11->usri11_logon_hours = (PVOID)Ptr;

                memcpy(UserInfo11->usri11_logon_hours,
                       UserInfo->LogonHours.LogonHours,
                       (((ULONG)UserInfo->LogonHours.UnitsPerWeek) + 7) / 8);

                Ptr = (LPWSTR)((ULONG_PTR)Ptr + (((ULONG)UserInfo->LogonHours.UnitsPerWeek) + 7) / 8);
            }

            UserInfo11->usri11_code_page = UserInfo->CodePage;
            break;

        case 20:
            UserInfo20 = (PUSER_INFO_20)LocalBuffer;

            Ptr = (LPWSTR)((ULONG_PTR)UserInfo20 + sizeof(USER_INFO_20));

            UserInfo20->usri20_name = Ptr;

            memcpy(UserInfo20->usri20_name,
                   UserInfo->UserName.Buffer,
                   UserInfo->UserName.Length);
            UserInfo20->usri20_name[UserInfo->UserName.Length / sizeof(WCHAR)] = UNICODE_NULL;

            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->UserName.Length + sizeof(WCHAR));

            UserInfo20->usri20_full_name = Ptr;
            memcpy(UserInfo20->usri20_full_name,
                   UserInfo->FullName.Buffer,
                   UserInfo->FullName.Length);
            UserInfo20->usri20_full_name[UserInfo->FullName.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->FullName.Length + sizeof(WCHAR));

            UserInfo20->usri20_comment = Ptr;
            memcpy(UserInfo20->usri20_comment,
                   UserInfo->AdminComment.Buffer,
                   UserInfo->AdminComment.Length);
            UserInfo20->usri20_comment[UserInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->AdminComment.Length + sizeof(WCHAR));

            UserInfo20->usri20_flags = GetAccountFlags(UserInfo->UserAccountControl,
                                                       Dacl);

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

            UserInfo23->usri23_full_name = Ptr;
            memcpy(UserInfo23->usri23_full_name,
                   UserInfo->FullName.Buffer,
                   UserInfo->FullName.Length);
            UserInfo23->usri23_full_name[UserInfo->FullName.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->FullName.Length + sizeof(WCHAR));

            UserInfo23->usri23_comment = Ptr;
            memcpy(UserInfo23->usri23_comment,
                   UserInfo->AdminComment.Buffer,
                   UserInfo->AdminComment.Length);
            UserInfo23->usri23_comment[UserInfo->AdminComment.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Ptr = (LPWSTR)((ULONG_PTR)Ptr + UserInfo->AdminComment.Length + sizeof(WCHAR));

            UserInfo23->usri23_flags = GetAccountFlags(UserInfo->UserAccountControl,
                                                       Dacl);

            /* FIXME: usri23_user_sid */
            break;
    }

done:
    if (UserInfo != NULL)
        FreeUserInfo(UserInfo);

    if (Dacl != NULL)
        HeapFree(GetProcessHeap(), 0, Dacl);

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
NET_API_STATUS
SetUserInfo(SAM_HANDLE UserHandle,
            LPBYTE UserInfo,
            DWORD Level)
{
    USER_ALL_INFORMATION UserAllInfo;
    PUSER_INFO_0 UserInfo0;
    PUSER_INFO_1 UserInfo1;
    PUSER_INFO_2 UserInfo2;
    PUSER_INFO_3 UserInfo3;
    PUSER_INFO_4 UserInfo4;
    PUSER_INFO_22 UserInfo22;
    PUSER_INFO_1003 UserInfo1003;
    PUSER_INFO_1006 UserInfo1006;
    PUSER_INFO_1007 UserInfo1007;
    PUSER_INFO_1008 UserInfo1008;
    PUSER_INFO_1009 UserInfo1009;
    PUSER_INFO_1011 UserInfo1011;
    PUSER_INFO_1012 UserInfo1012;
    PUSER_INFO_1013 UserInfo1013;
    PUSER_INFO_1014 UserInfo1014;
    PUSER_INFO_1017 UserInfo1017;
    PUSER_INFO_1018 UserInfo1018;
    PUSER_INFO_1024 UserInfo1024;
    PUSER_INFO_1025 UserInfo1025;
    PUSER_INFO_1051 UserInfo1051;
    PUSER_INFO_1052 UserInfo1052;
    PUSER_INFO_1053 UserInfo1053;
    PACL Dacl = NULL;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    ZeroMemory(&UserAllInfo, sizeof(USER_ALL_INFORMATION));

    if ((Level == 1) || (Level == 2) || (Level == 3) ||
        (Level == 4) || (Level == 22) || (Level == 1008))
    {
        ApiStatus = GetUserDacl(UserHandle, &Dacl);
        if (ApiStatus != NERR_Success)
            goto done;
    }

    switch (Level)
    {
        case 0:
            UserInfo0 = (PUSER_INFO_0)UserInfo;

            RtlInitUnicodeString(&UserAllInfo.UserName,
                                 UserInfo0->usri0_name);

            UserAllInfo.WhichFields |= USER_ALL_USERNAME;
            break;

        case 1:
            UserInfo1 = (PUSER_INFO_1)UserInfo;

            // usri1_name ignored

            if (UserInfo1->usri1_password != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.NtPassword,
                                     UserInfo1->usri1_password);
                UserAllInfo.NtPasswordPresent = TRUE;
                UserAllInfo.WhichFields |= USER_ALL_NTPASSWORDPRESENT;
            }

            // usri1_password_age ignored

//          UserInfo1->usri1_priv

            if (UserInfo1->usri1_home_dir != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.HomeDirectory,
                                     UserInfo1->usri1_home_dir);
                UserAllInfo.WhichFields |= USER_ALL_HOMEDIRECTORY;
            }

            if (UserInfo1->usri1_comment != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.AdminComment,
                                     UserInfo1->usri1_comment);
                UserAllInfo.WhichFields |= USER_ALL_ADMINCOMMENT;
            }

            ChangeUserDacl(Dacl, UserInfo1->usri1_flags);
            UserAllInfo.UserAccountControl = GetAccountControl(UserInfo1->usri1_flags);
            UserAllInfo.WhichFields |= USER_ALL_USERACCOUNTCONTROL;

            if (UserInfo1->usri1_script_path != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.ScriptPath,
                                     UserInfo1->usri1_script_path);
                UserAllInfo.WhichFields |= USER_ALL_SCRIPTPATH;
            }
            break;

        case 2:
            UserInfo2 = (PUSER_INFO_2)UserInfo;

            // usri2_name ignored

            if (UserInfo2->usri2_password != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.NtPassword,
                                     UserInfo2->usri2_password);
                UserAllInfo.NtPasswordPresent = TRUE;
                UserAllInfo.WhichFields |= USER_ALL_NTPASSWORDPRESENT;
            }

            // usri2_password_age ignored

//          UserInfo2->usri2_priv;

            if (UserInfo2->usri2_home_dir != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.HomeDirectory,
                                     UserInfo2->usri2_home_dir);
                UserAllInfo.WhichFields |= USER_ALL_HOMEDIRECTORY;
            }

            if (UserInfo2->usri2_comment != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.AdminComment,
                                     UserInfo2->usri2_comment);
                UserAllInfo.WhichFields |= USER_ALL_ADMINCOMMENT;
            }

            ChangeUserDacl(Dacl, UserInfo2->usri2_flags);
            UserAllInfo.UserAccountControl = GetAccountControl(UserInfo2->usri2_flags);
            UserAllInfo.WhichFields |= USER_ALL_USERACCOUNTCONTROL;

            if (UserInfo2->usri2_script_path != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.ScriptPath,
                                     UserInfo2->usri2_script_path);
                UserAllInfo.WhichFields |= USER_ALL_SCRIPTPATH;
            }

//          UserInfo2->usri2_auth_flags;

            if (UserInfo2->usri2_full_name != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.FullName,
                                     UserInfo2->usri2_full_name);
                UserAllInfo.WhichFields |= USER_ALL_FULLNAME;
            }

            if (UserInfo2->usri2_usr_comment != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.UserComment,
                                     UserInfo2->usri2_usr_comment);
                UserAllInfo.WhichFields |= USER_ALL_USERCOMMENT;
            }

            if (UserInfo2->usri2_parms != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.Parameters,
                                     UserInfo2->usri2_parms);
                UserAllInfo.WhichFields |= USER_ALL_PARAMETERS;
            }

            if (UserInfo2->usri2_workstations != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.WorkStations,
                                     UserInfo2->usri2_workstations);
                UserAllInfo.WhichFields |= USER_ALL_WORKSTATIONS;
            }

            // usri2_last_logon ignored
            // usri2_last_logoff ignored

            if (UserInfo2->usri2_acct_expires == TIMEQ_FOREVER)
            {
                UserAllInfo.AccountExpires.LowPart = 0;
                UserAllInfo.AccountExpires.HighPart = 0;
            }
            else
            {
                RtlSecondsSince1970ToTime(UserInfo2->usri2_acct_expires,
                                          &UserAllInfo.AccountExpires);
            }
            UserAllInfo.WhichFields |= USER_ALL_ACCOUNTEXPIRES;

            // usri2_max_storage ignored

//          UserInfo2->usri2_units_per_week;
//          UserInfo2->usri2_logon_hours;

            // usri2_bad_pw_count ignored
            // usri2_num_logons ignored
            // usri2_logon_server ignored

            UserAllInfo.CountryCode = UserInfo2->usri2_country_code;
            UserAllInfo.WhichFields |= USER_ALL_COUNTRYCODE;

            UserAllInfo.CodePage = UserInfo2->usri2_code_page;
            UserAllInfo.WhichFields |= USER_ALL_CODEPAGE;
            break;

        case 3:
            UserInfo3 = (PUSER_INFO_3)UserInfo;

            // usri3_name ignored

            if (UserInfo3->usri3_password != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.NtPassword,
                                     UserInfo3->usri3_password);
                UserAllInfo.NtPasswordPresent = TRUE;
                UserAllInfo.WhichFields |= USER_ALL_NTPASSWORDPRESENT;
            }

            // usri3_password_age ignored

//          UserInfo3->usri3_priv;

            if (UserInfo3->usri3_home_dir != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.HomeDirectory,
                                     UserInfo3->usri3_home_dir);
                UserAllInfo.WhichFields |= USER_ALL_HOMEDIRECTORY;
            }

            if (UserInfo3->usri3_comment != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.AdminComment,
                                     UserInfo3->usri3_comment);
                UserAllInfo.WhichFields |= USER_ALL_ADMINCOMMENT;
            }

            ChangeUserDacl(Dacl, UserInfo3->usri3_flags);
            UserAllInfo.UserAccountControl = GetAccountControl(UserInfo3->usri3_flags);
            UserAllInfo.WhichFields |= USER_ALL_USERACCOUNTCONTROL;

            if (UserInfo3->usri3_script_path != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.ScriptPath,
                                     UserInfo3->usri3_script_path);
                UserAllInfo.WhichFields |= USER_ALL_SCRIPTPATH;
            }

//          UserInfo3->usri3_auth_flags;

            if (UserInfo3->usri3_full_name != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.FullName,
                                     UserInfo3->usri3_full_name);
                UserAllInfo.WhichFields |= USER_ALL_FULLNAME;
            }

            if (UserInfo3->usri3_usr_comment != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.UserComment,
                                     UserInfo3->usri3_usr_comment);
                UserAllInfo.WhichFields |= USER_ALL_USERCOMMENT;
            }

            if (UserInfo3->usri3_parms != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.Parameters,
                                     UserInfo3->usri3_parms);
                UserAllInfo.WhichFields |= USER_ALL_PARAMETERS;
            }

            if (UserInfo3->usri3_workstations != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.WorkStations,
                                     UserInfo3->usri3_workstations);
                UserAllInfo.WhichFields |= USER_ALL_WORKSTATIONS;
            }

            // usri3_last_logon ignored
            // usri3_last_logoff ignored

            if (UserInfo3->usri3_acct_expires == TIMEQ_FOREVER)
            {
                UserAllInfo.AccountExpires.LowPart = 0;
                UserAllInfo.AccountExpires.HighPart = 0;
            }
            else
            {
                RtlSecondsSince1970ToTime(UserInfo3->usri3_acct_expires,
                                          &UserAllInfo.AccountExpires);
            }
            UserAllInfo.WhichFields |= USER_ALL_ACCOUNTEXPIRES;

            // usri3_max_storage ignored

//          UserInfo3->usri3_units_per_week;
//          UserInfo3->usri3_logon_hours;

            // usri3_bad_pw_count ignored
            // usri3_num_logons ignored
            // usri3_logon_server ignored

            UserAllInfo.CountryCode = UserInfo3->usri3_country_code;
            UserAllInfo.WhichFields |= USER_ALL_COUNTRYCODE;

            UserAllInfo.CodePage = UserInfo3->usri3_code_page;
            UserAllInfo.WhichFields |= USER_ALL_CODEPAGE;

            // usri3_user_id ignored

            UserAllInfo.PrimaryGroupId = UserInfo3->usri3_primary_group_id;
            UserAllInfo.WhichFields |= USER_ALL_PRIMARYGROUPID;

            if (UserInfo3->usri3_profile != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.ProfilePath,
                                     UserInfo3->usri3_profile);
                UserAllInfo.WhichFields |= USER_ALL_PROFILEPATH;
            }

            if (UserInfo3->usri3_home_dir_drive != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.HomeDirectoryDrive,
                                     UserInfo3->usri3_home_dir_drive);
                UserAllInfo.WhichFields |= USER_ALL_HOMEDIRECTORYDRIVE;
            }

            UserAllInfo.PasswordExpired = (UserInfo3->usri3_password_expired != 0);
            UserAllInfo.WhichFields |= USER_ALL_PASSWORDEXPIRED;
            break;

        case 4:
            UserInfo4 = (PUSER_INFO_4)UserInfo;

            // usri4_name ignored

            if (UserInfo4->usri4_password != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.NtPassword,
                                     UserInfo4->usri4_password);
                UserAllInfo.NtPasswordPresent = TRUE;
                UserAllInfo.WhichFields |= USER_ALL_NTPASSWORDPRESENT;
            }

            // usri4_password_age ignored

//          UserInfo3->usri4_priv;

            if (UserInfo4->usri4_home_dir != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.HomeDirectory,
                                     UserInfo4->usri4_home_dir);
                UserAllInfo.WhichFields |= USER_ALL_HOMEDIRECTORY;
            }

            if (UserInfo4->usri4_comment != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.AdminComment,
                                     UserInfo4->usri4_comment);
                UserAllInfo.WhichFields |= USER_ALL_ADMINCOMMENT;
            }

            ChangeUserDacl(Dacl, UserInfo4->usri4_flags);
            UserAllInfo.UserAccountControl = GetAccountControl(UserInfo4->usri4_flags);
            UserAllInfo.WhichFields |= USER_ALL_USERACCOUNTCONTROL;

            if (UserInfo4->usri4_script_path != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.ScriptPath,
                                     UserInfo4->usri4_script_path);
                UserAllInfo.WhichFields |= USER_ALL_SCRIPTPATH;
            }

//          UserInfo4->usri4_auth_flags;

            if (UserInfo4->usri4_full_name != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.FullName,
                                     UserInfo4->usri4_full_name);
                UserAllInfo.WhichFields |= USER_ALL_FULLNAME;
            }

            if (UserInfo4->usri4_usr_comment != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.UserComment,
                                     UserInfo4->usri4_usr_comment);
                UserAllInfo.WhichFields |= USER_ALL_USERCOMMENT;
            }

            if (UserInfo4->usri4_parms != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.Parameters,
                                     UserInfo4->usri4_parms);
                UserAllInfo.WhichFields |= USER_ALL_PARAMETERS;
            }

            if (UserInfo4->usri4_workstations != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.WorkStations,
                                     UserInfo4->usri4_workstations);
                UserAllInfo.WhichFields |= USER_ALL_WORKSTATIONS;
            }

            // usri4_last_logon ignored
            // usri4_last_logoff ignored

            if (UserInfo4->usri4_acct_expires == TIMEQ_FOREVER)
            {
                UserAllInfo.AccountExpires.LowPart = 0;
                UserAllInfo.AccountExpires.HighPart = 0;
            }
            else
            {
                RtlSecondsSince1970ToTime(UserInfo4->usri4_acct_expires,
                                          &UserAllInfo.AccountExpires);
            }
            UserAllInfo.WhichFields |= USER_ALL_ACCOUNTEXPIRES;

            // usri4_max_storage ignored

//          UserInfo4->usri4_units_per_week;
//          UserInfo4->usri4_logon_hours;

            // usri4_bad_pw_count ignored
            // usri4_num_logons ignored
            // usri4_logon_server ignored

            UserAllInfo.CountryCode = UserInfo4->usri4_country_code;
            UserAllInfo.WhichFields |= USER_ALL_COUNTRYCODE;

            UserAllInfo.CodePage = UserInfo4->usri4_code_page;
            UserAllInfo.WhichFields |= USER_ALL_CODEPAGE;

            // usri4_user_sid ignored

            UserAllInfo.PrimaryGroupId = UserInfo4->usri4_primary_group_id;
            UserAllInfo.WhichFields |= USER_ALL_PRIMARYGROUPID;

            if (UserInfo4->usri4_profile != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.ProfilePath,
                                     UserInfo4->usri4_profile);
                UserAllInfo.WhichFields |= USER_ALL_PROFILEPATH;
            }

            if (UserInfo4->usri4_home_dir_drive != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.HomeDirectoryDrive,
                                     UserInfo4->usri4_home_dir_drive);
                UserAllInfo.WhichFields |= USER_ALL_HOMEDIRECTORYDRIVE;
            }

            UserAllInfo.PasswordExpired = (UserInfo4->usri4_password_expired != 0);
            UserAllInfo.WhichFields |= USER_ALL_PASSWORDEXPIRED;
            break;

//        case 21:
//            break;

        case 22:
            UserInfo22 = (PUSER_INFO_22)UserInfo;

            // usri22_name ignored

//          UserInfo22->usri22_password[ENCRYPTED_PWLEN];

            // usri22_password_age ignored

//          UserInfo3->usri3_priv;

            if (UserInfo22->usri22_home_dir != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.HomeDirectory,
                                     UserInfo22->usri22_home_dir);
                UserAllInfo.WhichFields |= USER_ALL_HOMEDIRECTORY;
            }

            if (UserInfo22->usri22_comment != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.AdminComment,
                                     UserInfo22->usri22_comment);
                UserAllInfo.WhichFields |= USER_ALL_ADMINCOMMENT;
            }

            ChangeUserDacl(Dacl, UserInfo22->usri22_flags);
            UserAllInfo.UserAccountControl = GetAccountControl(UserInfo22->usri22_flags);
            UserAllInfo.WhichFields |= USER_ALL_USERACCOUNTCONTROL;

            if (UserInfo22->usri22_script_path != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.ScriptPath,
                                     UserInfo22->usri22_script_path);
                UserAllInfo.WhichFields |= USER_ALL_SCRIPTPATH;
            }

//          UserInfo22->usri22_auth_flags;

            if (UserInfo22->usri22_full_name != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.FullName,
                                     UserInfo22->usri22_full_name);
                UserAllInfo.WhichFields |= USER_ALL_FULLNAME;
            }

            if (UserInfo22->usri22_usr_comment != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.UserComment,
                                     UserInfo22->usri22_usr_comment);
                UserAllInfo.WhichFields |= USER_ALL_USERCOMMENT;
            }

            if (UserInfo22->usri22_parms != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.Parameters,
                                     UserInfo22->usri22_parms);
                UserAllInfo.WhichFields |= USER_ALL_PARAMETERS;
            }

            if (UserInfo22->usri22_workstations != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.WorkStations,
                                     UserInfo22->usri22_workstations);
                UserAllInfo.WhichFields |= USER_ALL_WORKSTATIONS;
            }

            // usri22_last_logon ignored
            // usri22_last_logoff ignored

            if (UserInfo22->usri22_acct_expires == TIMEQ_FOREVER)
            {
                UserAllInfo.AccountExpires.LowPart = 0;
                UserAllInfo.AccountExpires.HighPart = 0;
            }
            else
            {
                RtlSecondsSince1970ToTime(UserInfo22->usri22_acct_expires,
                                          &UserAllInfo.AccountExpires);
            }
            UserAllInfo.WhichFields |= USER_ALL_ACCOUNTEXPIRES;

            // usri22_max_storage ignored

//          UserInfo22->usri22_units_per_week;
//          UserInfo22->usri22_logon_hours;

            // usri22_bad_pw_count ignored
            // usri22_num_logons ignored
            // usri22_logon_server ignored

            UserAllInfo.CountryCode = UserInfo22->usri22_country_code;
            UserAllInfo.WhichFields |= USER_ALL_COUNTRYCODE;

            UserAllInfo.CodePage = UserInfo22->usri22_code_page;
            UserAllInfo.WhichFields |= USER_ALL_CODEPAGE;
            break;

        case 1003:
            UserInfo1003 = (PUSER_INFO_1003)UserInfo;

            if (UserInfo1003->usri1003_password != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.NtPassword,
                                     UserInfo1003->usri1003_password);
                UserAllInfo.NtPasswordPresent = TRUE;
                UserAllInfo.WhichFields |= USER_ALL_NTPASSWORDPRESENT;
            }
            break;

//        case 1005:
//            break;

        case 1006:
            UserInfo1006 = (PUSER_INFO_1006)UserInfo;

            if (UserInfo1006->usri1006_home_dir != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.HomeDirectory,
                                     UserInfo1006->usri1006_home_dir);
                UserAllInfo.WhichFields |= USER_ALL_HOMEDIRECTORY;
            }
            break;

        case 1007:
            UserInfo1007 = (PUSER_INFO_1007)UserInfo;

            if (UserInfo1007->usri1007_comment != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.AdminComment,
                                     UserInfo1007->usri1007_comment);
                UserAllInfo.WhichFields |= USER_ALL_ADMINCOMMENT;
            }
            break;

        case 1008:
            UserInfo1008 = (PUSER_INFO_1008)UserInfo;
            ChangeUserDacl(Dacl, UserInfo1008->usri1008_flags);
            UserAllInfo.UserAccountControl = GetAccountControl(UserInfo1008->usri1008_flags);
            UserAllInfo.WhichFields |= USER_ALL_USERACCOUNTCONTROL;
            break;

        case 1009:
            UserInfo1009 = (PUSER_INFO_1009)UserInfo;

            if (UserInfo1009->usri1009_script_path != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.ScriptPath,
                                     UserInfo1009->usri1009_script_path);
                UserAllInfo.WhichFields |= USER_ALL_SCRIPTPATH;
            }
            break;

//        case 1010:
//            break;

        case 1011:
            UserInfo1011 = (PUSER_INFO_1011)UserInfo;

            if (UserInfo1011->usri1011_full_name != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.FullName,
                                     UserInfo1011->usri1011_full_name);
                UserAllInfo.WhichFields |= USER_ALL_FULLNAME;
            }
            break;

        case 1012:
            UserInfo1012 = (PUSER_INFO_1012)UserInfo;

            if (UserInfo1012->usri1012_usr_comment != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.UserComment,
                                     UserInfo1012->usri1012_usr_comment);
                UserAllInfo.WhichFields |= USER_ALL_USERCOMMENT;
            }
            break;

        case 1013:
            UserInfo1013 = (PUSER_INFO_1013)UserInfo;

            if (UserInfo1013->usri1013_parms != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.Parameters,
                                     UserInfo1013->usri1013_parms);
                UserAllInfo.WhichFields |= USER_ALL_PARAMETERS;
            }
            break;

        case 1014:
            UserInfo1014 = (PUSER_INFO_1014)UserInfo;

            if (UserInfo1014->usri1014_workstations != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.WorkStations,
                                     UserInfo1014->usri1014_workstations);
                UserAllInfo.WhichFields |= USER_ALL_WORKSTATIONS;
            }
            break;

        case 1017:
            UserInfo1017 = (PUSER_INFO_1017)UserInfo;

            if (UserInfo1017->usri1017_acct_expires == TIMEQ_FOREVER)
            {
                UserAllInfo.AccountExpires.LowPart = 0;
                UserAllInfo.AccountExpires.HighPart = 0;
            }
            else
            {
                RtlSecondsSince1970ToTime(UserInfo1017->usri1017_acct_expires,
                                          &UserAllInfo.AccountExpires);
            }
            UserAllInfo.WhichFields |= USER_ALL_ACCOUNTEXPIRES;
            break;

        case 1018:
            UserInfo1018 = (PUSER_INFO_1018)UserInfo;

            if (UserInfo1018->usri1018_max_storage != USER_MAXSTORAGE_UNLIMITED)
            {
                // FIXME: Report error
                return ERROR_INVALID_PARAMETER;
            }
            break;

//        case 1020:
//            break;

        case 1024:
            UserInfo1024 = (PUSER_INFO_1024)UserInfo;

            UserAllInfo.CountryCode = UserInfo1024->usri1024_country_code;
            UserAllInfo.WhichFields |= USER_ALL_COUNTRYCODE;
            break;

        case 1025:
            UserInfo1025 = (PUSER_INFO_1025)UserInfo;

            UserAllInfo.CodePage = UserInfo1025->usri1025_code_page;
            UserAllInfo.WhichFields |= USER_ALL_CODEPAGE;
            break;

        case 1051:
            UserInfo1051 = (PUSER_INFO_1051)UserInfo;

            UserAllInfo.PrimaryGroupId = UserInfo1051->usri1051_primary_group_id;
            UserAllInfo.WhichFields |= USER_ALL_PRIMARYGROUPID;
            break;

        case 1052:
            UserInfo1052 = (PUSER_INFO_1052)UserInfo;

            if (UserInfo1052->usri1052_profile != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.ProfilePath,
                                     UserInfo1052->usri1052_profile);
                UserAllInfo.WhichFields |= USER_ALL_PROFILEPATH;
            }
            break;

        case 1053:
            UserInfo1053 = (PUSER_INFO_1053)UserInfo;

            if (UserInfo1053->usri1053_home_dir_drive != NULL)
            {
                RtlInitUnicodeString(&UserAllInfo.HomeDirectoryDrive,
                                     UserInfo1053->usri1053_home_dir_drive);
                UserAllInfo.WhichFields |= USER_ALL_HOMEDIRECTORYDRIVE;
            }
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
    if (Dacl != NULL)
        HeapFree(GetProcessHeap(), 0, Dacl);

    return ApiStatus;
}


static
NET_API_STATUS
OpenUserByName(SAM_HANDLE DomainHandle,
               PUNICODE_STRING UserName,
               ULONG DesiredAccess,
               PSAM_HANDLE UserHandle)
{
    PULONG RelativeIds = NULL;
    PSID_NAME_USE Use = NULL;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Get the RID for the given user name */
    Status = SamLookupNamesInDomain(DomainHandle,
                                    1,
                                    UserName,
                                    &RelativeIds,
                                    &Use);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamLookupNamesInDomain failed (Status %08lx)\n", Status);
        return NetpNtStatusToApiStatus(Status);
    }

    /* Fail, if it is not an alias account */
    if (Use[0] != SidTypeUser)
    {
        ERR("Object is not a user!\n");
        ApiStatus = NERR_GroupNotFound;
        goto done;
    }

    /* Open the alias account */
    Status = SamOpenUser(DomainHandle,
                         DesiredAccess,
                         RelativeIds[0],
                         UserHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamOpenUser failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

done:
    if (RelativeIds != NULL)
        SamFreeMemory(RelativeIds);

    if (Use != NULL)
        SamFreeMemory(Use);

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
    switch (level)
    {
        case 1:
        case 2:
        case 3:
        case 4:
            break;

        default:
            return ERROR_INVALID_LEVEL;
    }

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
                               DOMAIN_CREATE_USER | DOMAIN_LOOKUP | DOMAIN_READ_PASSWORD_PARAMETERS,
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
    {
        if (ApiStatus != NERR_Success)
            SamDeleteUser(UserHandle);
        else
            SamCloseHandle(UserHandle);
    }

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
    PMSV1_0_CHANGEPASSWORD_REQUEST RequestBuffer = NULL;
    PMSV1_0_CHANGEPASSWORD_RESPONSE ResponseBuffer = NULL;
    ULONG RequestBufferSize;
    ULONG ResponseBufferSize = 0;
    LPWSTR Ptr;
    ANSI_STRING PackageName;
    ULONG AuthenticationPackage = 0;
    HANDLE LsaHandle = NULL;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS ProtocolStatus;

    TRACE("(%s, %s, ..., ...)\n", debugstr_w(domainname), debugstr_w(username));

    /* FIXME: handle null domain or user name */

    /* Check the parameters */
    if ((oldpassword == NULL) ||
        (newpassword == NULL))
        return ERROR_INVALID_PARAMETER;

    /* Connect to the LSA server */
    Status = LsaConnectUntrusted(&LsaHandle);
    if (!NT_SUCCESS(Status))
        return NetpNtStatusToApiStatus(Status);

    /* Get the authentication package ID */
    RtlInitAnsiString(&PackageName,
                      MSV1_0_PACKAGE_NAME);

    Status = LsaLookupAuthenticationPackage(LsaHandle,
                                            &PackageName,
                                            &AuthenticationPackage);
    if (!NT_SUCCESS(Status))
    {
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Calculate the request buffer size */
    RequestBufferSize = sizeof(MSV1_0_CHANGEPASSWORD_REQUEST) +
                        ((wcslen(domainname) + 1) * sizeof(WCHAR)) +
                        ((wcslen(username) + 1) * sizeof(WCHAR)) +
                        ((wcslen(oldpassword) + 1) * sizeof(WCHAR)) +
                        ((wcslen(newpassword) + 1) * sizeof(WCHAR));

    /* Allocate the request buffer */
    ApiStatus = NetApiBufferAllocate(RequestBufferSize,
                                     (PVOID*)&RequestBuffer);
    if (ApiStatus != NERR_Success)
        goto done;

    /* Initialize the request buffer */
    RequestBuffer->MessageType = MsV1_0ChangePassword;
    RequestBuffer->Impersonating = TRUE;

    Ptr = (LPWSTR)((ULONG_PTR)RequestBuffer + sizeof(MSV1_0_CHANGEPASSWORD_REQUEST));

    /* Pack the domain name */
    RequestBuffer->DomainName.Length = wcslen(domainname) * sizeof(WCHAR);
    RequestBuffer->DomainName.MaximumLength = RequestBuffer->DomainName.Length + sizeof(WCHAR);
    RequestBuffer->DomainName.Buffer = Ptr;

    RtlCopyMemory(RequestBuffer->DomainName.Buffer,
                  domainname,
                  RequestBuffer->DomainName.MaximumLength);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + RequestBuffer->DomainName.MaximumLength);

    /* Pack the user name */
    RequestBuffer->AccountName.Length = wcslen(username) * sizeof(WCHAR);
    RequestBuffer->AccountName.MaximumLength = RequestBuffer->AccountName.Length + sizeof(WCHAR);
    RequestBuffer->AccountName.Buffer = Ptr;

    RtlCopyMemory(RequestBuffer->AccountName.Buffer,
                  username,
                  RequestBuffer->AccountName.MaximumLength);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + RequestBuffer->AccountName.MaximumLength);

    /* Pack the old password */
    RequestBuffer->OldPassword.Length = wcslen(oldpassword) * sizeof(WCHAR);
    RequestBuffer->OldPassword.MaximumLength = RequestBuffer->OldPassword.Length + sizeof(WCHAR);
    RequestBuffer->OldPassword.Buffer = Ptr;

    RtlCopyMemory(RequestBuffer->OldPassword.Buffer,
                  oldpassword,
                  RequestBuffer->OldPassword.MaximumLength);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + RequestBuffer->OldPassword.MaximumLength);

    /* Pack the new password */
    RequestBuffer->NewPassword.Length = wcslen(newpassword) * sizeof(WCHAR);
    RequestBuffer->NewPassword.MaximumLength = RequestBuffer->NewPassword.Length + sizeof(WCHAR);
    RequestBuffer->NewPassword.Buffer = Ptr;

    RtlCopyMemory(RequestBuffer->NewPassword.Buffer,
                  newpassword,
                  RequestBuffer->NewPassword.MaximumLength);

    /* Call the authentication package */
    Status = LsaCallAuthenticationPackage(LsaHandle,
                                          AuthenticationPackage,
                                          RequestBuffer,
                                          RequestBufferSize,
                                          (PVOID*)&ResponseBuffer,
                                          &ResponseBufferSize,
                                          &ProtocolStatus);
    if (!NT_SUCCESS(Status))
    {
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    if (!NT_SUCCESS(ProtocolStatus))
    {
        ApiStatus = NetpNtStatusToApiStatus(ProtocolStatus);
        goto done;
    }

done:
    if (RequestBuffer != NULL)
        NetApiBufferFree(RequestBuffer);

    if (ResponseBuffer != NULL)
        LsaFreeReturnBuffer(ResponseBuffer);

    if (LsaHandle != NULL)
        NtClose(LsaHandle);

    return ApiStatus;
}


/************************************************************
 * NetUserDel  (NETAPI32.@)
 */
NET_API_STATUS
WINAPI
NetUserDel(LPCWSTR servername,
           LPCWSTR username)
{
    UNICODE_STRING ServerName;
    UNICODE_STRING UserName;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE UserHandle = NULL;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("(%s, %s)\n", debugstr_w(servername), debugstr_w(username));

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
                               DOMAIN_LOOKUP,
                               &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("OpenBuiltinDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Open the user account in the builtin domain */
    ApiStatus = OpenUserByName(DomainHandle,
                               &UserName,
                               DELETE,
                               &UserHandle);
    if (ApiStatus != NERR_Success && ApiStatus != ERROR_NONE_MAPPED)
    {
        TRACE("OpenUserByName failed (ApiStatus %lu)\n", ApiStatus);
        goto done;
    }

    if (UserHandle == NULL)
    {
        if (DomainHandle != NULL)
        {
            SamCloseHandle(DomainHandle);
            DomainHandle = NULL;
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

        /* Open the user account in the account domain */
        ApiStatus = OpenUserByName(DomainHandle,
                                   &UserName,
                                   DELETE,
                                   &UserHandle);
        if (ApiStatus != NERR_Success)
        {
            ERR("OpenUserByName failed (ApiStatus %lu)\n", ApiStatus);
            if (ApiStatus == ERROR_NONE_MAPPED)
                ApiStatus = NERR_UserNotFound;
            goto done;
        }
    }

    /* Delete the user */
    Status = SamDeleteUser(UserHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamDeleteUser failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* A successful delete invalidates the handle */
    UserHandle = NULL;

done:
    if (UserHandle != NULL)
        SamCloseHandle(UserHandle);

    if (DomainHandle != NULL)
        SamCloseHandle(DomainHandle);

    if (ServerHandle != NULL)
        SamCloseHandle(ServerHandle);

    return ApiStatus;
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
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("(%s %d 0x%d %p %d %p %p %p)\n", debugstr_w(servername), level,
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
                             READ_CONTROL | USER_READ_GENERAL | USER_READ_PREFERENCES | USER_READ_LOGON | USER_READ_ACCOUNT,
                             CurrentUser->RelativeId,
                             &UserHandle);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamOpenUser failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        ApiStatus = BuildUserInfoBuffer(UserHandle,
                                        level,
                                        CurrentUser->RelativeId,
                                        &Buffer);
        if (ApiStatus != NERR_Success)
        {
            ERR("BuildUserInfoBuffer failed (ApiStatus %lu)\n", ApiStatus);
            goto done;
        }

        SamCloseHandle(UserHandle);
        UserHandle = NULL;

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
    UNICODE_STRING ServerName;
    UNICODE_STRING UserName;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE AccountDomainHandle = NULL;
    SAM_HANDLE UserHandle = NULL;
    PSID AccountDomainSid = NULL;
    PULONG RelativeIds = NULL;
    PSID_NAME_USE Use = NULL;
    PGROUP_MEMBERSHIP GroupMembership = NULL;
    ULONG GroupCount;

    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("%s %s %d %p %d %p %p stub\n", debugstr_w(servername),
          debugstr_w(username), level, bufptr, prefixmaxlen, entriesread,
          totalentries);

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
        if (Status == STATUS_NONE_MAPPED)
            ApiStatus = NERR_UserNotFound;
        else
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

    /* Open the user object */
    Status = SamOpenUser(AccountDomainHandle,
                         USER_LIST_GROUPS,
                         RelativeIds[0],
                         &UserHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamOpenUser failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Get the group memberships of this user */
    Status = SamGetGroupsForUser(UserHandle,
                                 &GroupMembership,
                                 &GroupCount);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamGetGroupsForUser failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* If there is no group membership, we're done */
    if (GroupCount == 0)
    {
        ApiStatus = NERR_Success;
        goto done;
    }


done:

    if (GroupMembership != NULL)
        SamFreeMemory(GroupMembership);

    if (UserHandle != NULL)
        SamCloseHandle(UserHandle);

    if (RelativeIds != NULL)
        SamFreeMemory(RelativeIds);

    if (Use != NULL)
        SamFreeMemory(Use);

    if (AccountDomainSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AccountDomainSid);

    if (AccountDomainHandle != NULL)
        SamCloseHandle(AccountDomainHandle);

    if (ServerHandle != NULL)
        SamCloseHandle(ServerHandle);

    if (ApiStatus != NERR_Success && ApiStatus != ERROR_MORE_DATA)
    {
        *entriesread = 0;
        *totalentries = 0;
    }
    else
    {
//        *entriesread = Count;
//        *totalentries = Count;
    }

//    *bufptr = (LPBYTE)Buffer;

    return ApiStatus;
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
        if (Status == STATUS_NONE_MAPPED)
            ApiStatus = NERR_UserNotFound;
        else
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
                         READ_CONTROL | USER_READ_GENERAL | USER_READ_PREFERENCES | USER_READ_LOGON | USER_READ_ACCOUNT,
                         RelativeIds[0],
                         &UserHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamOpenUser failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    ApiStatus = BuildUserInfoBuffer(UserHandle,
                                    level,
                                    RelativeIds[0],
                                    &Buffer);
    if (ApiStatus != NERR_Success)
    {
        ERR("BuildUserInfoBuffer failed (ApiStatus %08lu)\n", ApiStatus);
        goto done;
    }

done:
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
        if (Status == STATUS_NONE_MAPPED)
            ApiStatus = NERR_UserNotFound;
        else
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
    ApiStatus = BuildSidFromSidAndRid(AccountDomainSid,
                                      RelativeIds[0],
                                      &UserSid);
    if (ApiStatus != NERR_Success)
    {
        ERR("BuildSidFromSidAndRid failed!\n");
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
        if (AccountNames[i].Length > 0)
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
        NetApiBufferFree(UserSid);

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
 * NetUserModalsGet  (NETAPI32.@)
 *
 * Retrieves global information for all users and global groups in the security
 * database.
 *
 * PARAMS
 *  servername [I] Specifies the DNS or the NetBIOS name of the remote server
 *                 on which the function is to execute.
 *  level      [I] Information level of the data.
 *     0   Return global passwords parameters. bufptr points to a
 *         USER_MODALS_INFO_0 struct.
 *     1   Return logon server and domain controller information. bufptr
 *         points to a USER_MODALS_INFO_1 struct.
 *     2   Return domain name and identifier. bufptr points to a 
 *         USER_MODALS_INFO_2 struct.
 *     3   Return lockout information. bufptr points to a USER_MODALS_INFO_3
 *         struct.
 *  bufptr     [O] Buffer that receives the data.
 *
 * RETURNS
 *  Success: NERR_Success.
 *  Failure: 
 *     ERROR_ACCESS_DENIED - the user does not have access to the info.
 *     NERR_InvalidComputer - computer name is invalid.
 */
NET_API_STATUS
WINAPI
NetUserModalsGet(LPCWSTR servername,
                 DWORD level,
                 LPBYTE *bufptr)
{
    UNICODE_STRING ServerName;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    PSID DomainSid = NULL;
    PDOMAIN_PASSWORD_INFORMATION PasswordInfo = NULL;
    PDOMAIN_LOGOFF_INFORMATION LogoffInfo = NULL;
    PDOMAIN_SERVER_ROLE_INFORMATION ServerRoleInfo = NULL;
    PDOMAIN_REPLICATION_INFORMATION ReplicationInfo = NULL;
    PDOMAIN_NAME_INFORMATION NameInfo = NULL;
    PDOMAIN_LOCKOUT_INFORMATION LockoutInfo = NULL;
    ULONG DesiredAccess;
    ULONG BufferSize;
    PUSER_MODALS_INFO_0 umi0;
    PUSER_MODALS_INFO_1 umi1;
    PUSER_MODALS_INFO_2 umi2;
    PUSER_MODALS_INFO_3 umi3;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("(%s %d %p)\n", debugstr_w(servername), level, bufptr);

    *bufptr = NULL;

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

    /* Get the Account Domain SID */
    Status = GetAccountDomainSid((servername != NULL) ? &ServerName : NULL,
                                 &DomainSid);
    if (!NT_SUCCESS(Status))
    {
        ERR("GetAccountDomainSid failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    switch (level)
    {
        case 0:
            DesiredAccess = DOMAIN_READ_OTHER_PARAMETERS | DOMAIN_READ_PASSWORD_PARAMETERS;
            break;

        case 1:
            DesiredAccess = DOMAIN_READ_OTHER_PARAMETERS;
            break;

        case 2:
            DesiredAccess = DOMAIN_READ_OTHER_PARAMETERS;
            break;

        case 3:
            DesiredAccess = DOMAIN_READ_PASSWORD_PARAMETERS;
            break;

        default:
            ApiStatus = ERROR_INVALID_LEVEL;
            goto done;
    }

    /* Open the Account Domain */
    Status = SamOpenDomain(ServerHandle,
                           DesiredAccess,
                           DomainSid,
                           &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("OpenAccountDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    switch (level)
    {
        case 0:
            /* return global passwords parameters */
            Status = SamQueryInformationDomain(DomainHandle,
                                               DomainPasswordInformation,
                                               (PVOID*)&PasswordInfo);
            if (!NT_SUCCESS(Status))
            {
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }

            Status = SamQueryInformationDomain(DomainHandle,
                                               DomainLogoffInformation,
                                               (PVOID*)&LogoffInfo);
            if (!NT_SUCCESS(Status))
            {
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }

            BufferSize = sizeof(USER_MODALS_INFO_0);
            break;

        case 1:
            /* return logon server and domain controller info */
            Status = SamQueryInformationDomain(DomainHandle,
                                               DomainServerRoleInformation,
                                               (PVOID*)&ServerRoleInfo);
            if (!NT_SUCCESS(Status))
            {
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }

            Status = SamQueryInformationDomain(DomainHandle,
                                               DomainReplicationInformation,
                                               (PVOID*)&ReplicationInfo);
            if (!NT_SUCCESS(Status))
            {
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }

            BufferSize = sizeof(USER_MODALS_INFO_1) +
                         ReplicationInfo->ReplicaSourceNodeName.Length + sizeof(WCHAR);
            break;

        case 2:
            /* return domain name and identifier */
            Status = SamQueryInformationDomain(DomainHandle,
                                               DomainNameInformation,
                                               (PVOID*)&NameInfo);
            if (!NT_SUCCESS(Status))
            {
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }

            BufferSize = sizeof( USER_MODALS_INFO_2 ) +
                         NameInfo->DomainName.Length + sizeof(WCHAR) +
                         RtlLengthSid(DomainSid);
            break;

        case 3:
            /* return lockout information */
            Status = SamQueryInformationDomain(DomainHandle,
                                               DomainLockoutInformation,
                                               (PVOID*)&LockoutInfo);
            if (!NT_SUCCESS(Status))
            {
                ApiStatus = NetpNtStatusToApiStatus(Status);
                goto done;
            }

            BufferSize = sizeof(USER_MODALS_INFO_3);
            break;

        default:
            TRACE("Invalid level %d is specified\n", level);
            ApiStatus = ERROR_INVALID_LEVEL;
            goto done;
    }


    ApiStatus = NetApiBufferAllocate(BufferSize,
                                     (LPVOID *)bufptr);
    if (ApiStatus != NERR_Success)
    {
        WARN("NetApiBufferAllocate() failed\n");
        goto done;
    }

    switch (level)
    {
        case 0:
            umi0 = (PUSER_MODALS_INFO_0)*bufptr;

            umi0->usrmod0_min_passwd_len = PasswordInfo->MinPasswordLength;
            umi0->usrmod0_max_passwd_age = (ULONG)(-PasswordInfo->MaxPasswordAge.QuadPart / 10000000);
            umi0->usrmod0_min_passwd_age =
                DeltaTimeToSeconds(PasswordInfo->MinPasswordAge);
            umi0->usrmod0_force_logoff =
                DeltaTimeToSeconds(LogoffInfo->ForceLogoff);
            umi0->usrmod0_password_hist_len = PasswordInfo->PasswordHistoryLength;
            break;

        case 1:
            umi1 = (PUSER_MODALS_INFO_1)*bufptr;

            switch (ServerRoleInfo->DomainServerRole)
            {
                case DomainServerRolePrimary:
                    umi1->usrmod1_role = UAS_ROLE_PRIMARY;
                    break;

                case DomainServerRoleBackup:
                    umi1->usrmod1_role = UAS_ROLE_BACKUP;
                    break;

                default:
                    ApiStatus = NERR_InternalError;
                    goto done;
            }

            umi1->usrmod1_primary = (LPWSTR)(*bufptr + sizeof(USER_MODALS_INFO_1));
            RtlCopyMemory(umi1->usrmod1_primary,
                          ReplicationInfo->ReplicaSourceNodeName.Buffer,
                          ReplicationInfo->ReplicaSourceNodeName.Length);
            umi1->usrmod1_primary[ReplicationInfo->ReplicaSourceNodeName.Length / sizeof(WCHAR)] = UNICODE_NULL;
            break;

        case 2:
            umi2 = (PUSER_MODALS_INFO_2)*bufptr;

            umi2->usrmod2_domain_name = (LPWSTR)(*bufptr + sizeof(USER_MODALS_INFO_2));
            RtlCopyMemory(umi2->usrmod2_domain_name,
                          NameInfo->DomainName.Buffer,
                          NameInfo->DomainName.Length);
            umi2->usrmod2_domain_name[NameInfo->DomainName.Length / sizeof(WCHAR)] = UNICODE_NULL;

            umi2->usrmod2_domain_id = *bufptr +
                                      sizeof(USER_MODALS_INFO_2) +
                                      NameInfo->DomainName.Length + sizeof(WCHAR);
            RtlCopyMemory(umi2->usrmod2_domain_id,
                          DomainSid,
                          RtlLengthSid(DomainSid));
            break;

        case 3:
            umi3 = (PUSER_MODALS_INFO_3)*bufptr;
            umi3->usrmod3_lockout_duration =
                DeltaTimeToSeconds(LockoutInfo->LockoutDuration);
            umi3->usrmod3_lockout_observation_window =
                DeltaTimeToSeconds(LockoutInfo->LockoutObservationWindow );
            umi3->usrmod3_lockout_threshold = LockoutInfo->LockoutThreshold;
            break;
    }

done:
    if (LockoutInfo != NULL)
        SamFreeMemory(LockoutInfo);

    if (NameInfo != NULL)
        SamFreeMemory(NameInfo);

    if (ReplicationInfo != NULL)
        SamFreeMemory(ReplicationInfo);

    if (ServerRoleInfo != NULL)
        SamFreeMemory(ServerRoleInfo);

    if (LogoffInfo != NULL)
        SamFreeMemory(LogoffInfo);

    if (PasswordInfo != NULL)
        SamFreeMemory(PasswordInfo);

    if (DomainSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, DomainSid);

    if (DomainHandle != NULL)
        SamCloseHandle(DomainHandle);

    if (ServerHandle != NULL)
        SamCloseHandle(ServerHandle);

    return ApiStatus;
}


/******************************************************************************
 * NetUserModalsSet  (NETAPI32.@)
 */
NET_API_STATUS
WINAPI
NetUserModalsSet(IN LPCWSTR servername,
                 IN DWORD level,
                 IN LPBYTE buf,
                 OUT LPDWORD parm_err)
{
    FIXME("(%s %d %p %p)\n", debugstr_w(servername), level, buf, parm_err);
    return ERROR_ACCESS_DENIED;
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
    UNICODE_STRING ServerName;
    UNICODE_STRING UserName;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE AccountDomainHandle = NULL;
    SAM_HANDLE UserHandle = NULL;
    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("(%s %s %lu %p %p)\n",
          debugstr_w(servername), debugstr_w(username), level, buf, parm_err);

    if (parm_err != NULL)
        *parm_err = PARM_ERROR_NONE;

    /* Check the info level */
    switch (level)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
//        case 21:
        case 22:
        case 1003:
//        case 1005:
        case 1006:
        case 1007:
        case 1008:
        case 1009:
//        case 1010:
        case 1011:
        case 1012:
        case 1013:
        case 1014:
        case 1017:
        case 1018:
//        case 1020:
        case 1024:
        case 1025:
        case 1051:
        case 1052:
        case 1053:
            break;

        default:
            return ERROR_INVALID_LEVEL;
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

    /* Open the Account Domain */
    Status = OpenAccountDomain(ServerHandle,
                               (servername != NULL) ? &ServerName : NULL,
                               DOMAIN_LIST_ACCOUNTS | DOMAIN_LOOKUP | DOMAIN_READ_PASSWORD_PARAMETERS,
                               &AccountDomainHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("OpenAccountDomain failed (Status %08lx)\n", Status);
        ApiStatus = NetpNtStatusToApiStatus(Status);
        goto done;
    }

    /* Open the User Account */
    ApiStatus = OpenUserByName(AccountDomainHandle,
                               &UserName,
                               USER_ALL_ACCESS,
                               &UserHandle);
    if (ApiStatus != NERR_Success)
    {
        ERR("OpenUserByName failed (ApiStatus %lu)\n", ApiStatus);
        goto done;
    }

    /* Set user information */
    ApiStatus = SetUserInfo(UserHandle,
                            buf,
                            level);
    if (ApiStatus != NERR_Success)
    {
        ERR("SetUserInfo failed (Status %lu)\n", ApiStatus);
    }

done:
    if (UserHandle != NULL)
        SamCloseHandle(UserHandle);

    if (AccountDomainHandle != NULL)
        SamCloseHandle(AccountDomainHandle);

    if (ServerHandle != NULL)
        SamCloseHandle(ServerHandle);

    return ApiStatus;
}

/* EOF */
