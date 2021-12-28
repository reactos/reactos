/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Security Account Manager (SAM) related functions
 * COPYRIGHT:   Copyright 2013 Eric Kohl <eric.kohl@reactos.org>
 */

#include "precomp.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msv1_0_sam);


static
NTSTATUS
GetAccountDomainSid(
    _In_ PRPC_SID *Sid)
{
    LSAPR_HANDLE PolicyHandle = NULL;
    PLSAPR_POLICY_INFORMATION PolicyInfo = NULL;
    ULONG Length = 0;
    NTSTATUS Status;

    Status = LsaIOpenPolicyTrusted(&PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LsaIOpenPolicyTrusted() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    Status = LsarQueryInformationPolicy(PolicyHandle,
                                        PolicyAccountDomainInformation,
                                        &PolicyInfo);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LsarQueryInformationPolicy() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    Length = RtlLengthSid(PolicyInfo->PolicyAccountDomainInfo.Sid);

    *Sid = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
    if (*Sid == NULL)
    {
        ERR("Failed to allocate SID\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    memcpy(*Sid, PolicyInfo->PolicyAccountDomainInfo.Sid, Length);

done:
    if (PolicyInfo != NULL)
        LsaIFree_LSAPR_POLICY_INFORMATION(PolicyAccountDomainInformation,
                                          PolicyInfo);

    if (PolicyHandle != NULL)
        LsarClose(&PolicyHandle);

    return Status;
}


static
NTSTATUS
MsvpCheckPassword(
    _In_ PLSA_SAM_PWD_DATA UserPwdData,
    _In_ PSAMPR_USER_INFO_BUFFER UserInfo)
{
    ENCRYPTED_NT_OWF_PASSWORD UserNtPassword;
    ENCRYPTED_LM_OWF_PASSWORD UserLmPassword;
    BOOLEAN UserLmPasswordPresent = FALSE;
    BOOLEAN UserNtPasswordPresent = FALSE;
    OEM_STRING LmPwdString;
    CHAR LmPwdBuffer[15];
    NTSTATUS Status;

    TRACE("(%p %p)\n", UserPwdData, UserInfo);

    /* Calculate the LM password and hash for the users password */
    LmPwdString.Length = 15;
    LmPwdString.MaximumLength = 15;
    LmPwdString.Buffer = LmPwdBuffer;
    ZeroMemory(LmPwdString.Buffer, LmPwdString.MaximumLength);

    Status = RtlUpcaseUnicodeStringToOemString(&LmPwdString,
                                               UserPwdData->PlainPwd,
                                               FALSE);
    if (NT_SUCCESS(Status))
    {
        /* Calculate the LM hash value of the users password */
        Status = SystemFunction006(LmPwdString.Buffer,
                                   (LPSTR)&UserLmPassword);
        if (NT_SUCCESS(Status))
        {
            UserLmPasswordPresent = TRUE;
        }
    }

    /* Calculate the NT hash of the users password */
    Status = SystemFunction007(UserPwdData->PlainPwd,
                               (LPBYTE)&UserNtPassword);
    if (NT_SUCCESS(Status))
    {
        UserNtPasswordPresent = TRUE;
    }

    Status = STATUS_WRONG_PASSWORD;

    /* Succeed, if no password has been set */
    if (UserInfo->All.NtPasswordPresent == FALSE &&
        UserInfo->All.LmPasswordPresent == FALSE)
    {
        TRACE("No password check!\n");
        Status = STATUS_SUCCESS;
        goto done;
    }

    /* Succeed, if NT password matches */
    if (UserNtPasswordPresent && UserInfo->All.NtPasswordPresent)
    {
        TRACE("Check NT password hashes:\n");
        if (RtlEqualMemory(&UserNtPassword,
                           UserInfo->All.NtOwfPassword.Buffer,
                           sizeof(ENCRYPTED_NT_OWF_PASSWORD)))
        {
            TRACE("  success!\n");
            Status = STATUS_SUCCESS;
            goto done;
        }

        TRACE("  failed!\n");
    }

    /* Succeed, if LM password matches */
    if (UserLmPasswordPresent && UserInfo->All.LmPasswordPresent)
    {
        TRACE("Check LM password hashes:\n");
        if (RtlEqualMemory(&UserLmPassword,
                           UserInfo->All.LmOwfPassword.Buffer,
                           sizeof(ENCRYPTED_LM_OWF_PASSWORD)))
        {
            TRACE("  success!\n");
            Status = STATUS_SUCCESS;
            goto done;
        }
        TRACE("  failed!\n");
    }

done:
    return Status;
}


static
bool
MsvpCheckLogonHours(
    _In_ PSAMPR_LOGON_HOURS LogonHours,
    _In_ LARGE_INTEGER LogonTime)
{
#if 0
    LARGE_INTEGER LocalLogonTime;
    TIME_FIELDS TimeFields;
    USHORT MinutesPerUnit, Offset;
    bool bFound;

    FIXME("MsvpCheckLogonHours(%p %llx)\n", LogonHours, LogonTime);

    if (LogonHours->UnitsPerWeek == 0 || LogonHours->LogonHours == NULL)
    {
        FIXME("No logon hours!\n");
        return true;
    }

    RtlSystemTimeToLocalTime(&LogonTime, &LocalLogonTime);
    RtlTimeToTimeFields(&LocalLogonTime, &TimeFields);

    FIXME("UnitsPerWeek: %u\n", LogonHours->UnitsPerWeek);
    MinutesPerUnit = 10080 / LogonHours->UnitsPerWeek;

    Offset = ((TimeFields.Weekday * 24 + TimeFields.Hour) * 60 + TimeFields.Minute) / MinutesPerUnit;
    FIXME("Offset: %us\n", Offset);

    bFound = (bool)(LogonHours->LogonHours[Offset / 8] & (1 << (Offset % 8)));
    FIXME("Logon permitted: %s\n", bFound ? "Yes" : "No");

    return bFound;
#endif
    return true;
}


static
bool
MsvpCheckWorkstations(
    _In_ PRPC_UNICODE_STRING WorkStations,
    _In_ PWSTR ComputerName)
{
    PWSTR pStart, pEnd;
    bool bFound = false;

    TRACE("MsvpCheckWorkstations(%p %S)\n", WorkStations, ComputerName);

    if (WorkStations->Length == 0 || WorkStations->Buffer == NULL)
    {
        TRACE("No workstations!\n");
        return true;
    }

    TRACE("Workstations: %wZ\n", WorkStations);

    pStart = WorkStations->Buffer;
    for (;;)
    {
        pEnd = wcschr(pStart, L',');
        if (pEnd != NULL)
            *pEnd = UNICODE_NULL;

        TRACE("Comparing '%S' and '%S'\n", ComputerName, pStart);
        if (_wcsicmp(ComputerName, pStart) == 0)
        {
            bFound = true;
            if (pEnd != NULL)
                *pEnd = L',';
            break;
        }

        if (pEnd == NULL)
            break;

        *pEnd = L',';
        pStart = pEnd + 1;
    }

    TRACE("Found allowed workstation: %s\n", (bFound) ? "Yes" : "No");

    return bFound;
}


static
NTSTATUS
SamValidateNormalUser(
    _In_ PUNICODE_STRING UserName,
    _In_ PLSA_SAM_PWD_DATA PwdData,
    _In_ PUNICODE_STRING ComputerName,
    _Out_ PRPC_SID* AccountDomainSidPtr,
    _Out_ SAMPR_HANDLE* UserHandlePtr,
    _Out_ PSAMPR_USER_INFO_BUFFER* UserInfoPtr,
    _Out_ PNTSTATUS SubStatus)
{
    NTSTATUS Status;
    SAMPR_HANDLE ServerHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;
    PRPC_SID AccountDomainSid;
    RPC_UNICODE_STRING Names[1];
    SAMPR_HANDLE UserHandle = NULL;
    SAMPR_ULONG_ARRAY RelativeIds = {0, NULL};
    SAMPR_ULONG_ARRAY Use = {0, NULL};
    PSAMPR_USER_INFO_BUFFER UserInfo = NULL;
    LARGE_INTEGER LogonTime;

    /* Get the logon time */
    NtQuerySystemTime(&LogonTime);

    /* Get the account domain SID */
    Status = GetAccountDomainSid(&AccountDomainSid);
    if (!NT_SUCCESS(Status))
    {
        ERR("GetAccountDomainSid() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Connect to the SAM server */
    Status = SamIConnect(NULL, &ServerHandle, SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN, TRUE);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamIConnect() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Open the account domain */
    Status = SamrOpenDomain(ServerHandle, DOMAIN_LOOKUP, AccountDomainSid, &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamrOpenDomain failed (Status %08lx)\n", Status);
        goto done;
    }

    Names[0].Length = UserName->Length;
    Names[0].MaximumLength = UserName->MaximumLength;
    Names[0].Buffer = UserName->Buffer;

    /* Try to get the RID for the user name */
    Status = SamrLookupNamesInDomain(DomainHandle, 1, Names, &RelativeIds, &Use);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamrLookupNamesInDomain failed (Status %08lx)\n", Status);
        Status = STATUS_NO_SUCH_USER;
        // FIXME: Try without domain?
        goto done;
    }

    /* Fail, if it is not a user account */
    if (Use.Element[0] != SidTypeUser)
    {
        ERR("Account is not a user account!\n");
        Status = STATUS_NO_SUCH_USER;
        goto done;
    }

    /* Open the user object */
    Status = SamrOpenUser(DomainHandle,
                          USER_READ_GENERAL | USER_READ_LOGON |
                          USER_READ_ACCOUNT | USER_READ_PREFERENCES, /* FIXME */
                          RelativeIds.Element[0],
                          &UserHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamrOpenUser failed (Status %08lx)\n", Status);
        goto done;
    }

    Status = SamrQueryInformationUser(UserHandle, UserAllInformation, &UserInfo);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamrQueryInformationUser failed (Status %08lx)\n", Status);
        goto done;
    }

    TRACE("UserName: %wZ\n", &UserInfo->All.UserName);

    /* Check the password */
    if ((UserInfo->All.UserAccountControl & USER_PASSWORD_NOT_REQUIRED) == 0)
    {
        Status = MsvpCheckPassword(PwdData, UserInfo);
        if (!NT_SUCCESS(Status))
        {
            ERR("MsvpCheckPassword failed (Status %08lx)\n", Status);
            goto done;
        }
    }

    /* Check account restrictions for non-administrator accounts */
    if (RelativeIds.Element[0] != DOMAIN_USER_RID_ADMIN)
    {
        /* Check if the account has been disabled */
        if (UserInfo->All.UserAccountControl & USER_ACCOUNT_DISABLED)
        {
            ERR("Account disabled!\n");
            *SubStatus = STATUS_ACCOUNT_DISABLED;
            Status = STATUS_ACCOUNT_RESTRICTION;
            goto done;
        }

        /* Check if the account has been locked */
        if (UserInfo->All.UserAccountControl & USER_ACCOUNT_AUTO_LOCKED)
        {
            ERR("Account locked!\n");
            *SubStatus = STATUS_ACCOUNT_LOCKED_OUT;
            Status = STATUS_ACCOUNT_RESTRICTION;
            goto done;
        }

        /* Check if the account expired */
        if (LogonTime.QuadPart >= *(UINT64*)&UserInfo->All.AccountExpires)
        {
            ERR("Account expired!\n");
            *SubStatus = STATUS_ACCOUNT_EXPIRED;
            Status = STATUS_ACCOUNT_RESTRICTION;
            goto done;
        }

        /* Check if the password expired */
        if (LogonTime.QuadPart >= *(UINT64*)&UserInfo->All.PasswordMustChange)
        {
            ERR("Password expired!\n");
            if (*(UINT64*)&UserInfo->All.PasswordLastSet == 0)
                *SubStatus = STATUS_PASSWORD_MUST_CHANGE;
            else
                *SubStatus = STATUS_PASSWORD_EXPIRED;

            Status = STATUS_ACCOUNT_RESTRICTION;
            goto done;
        }

        /* Check logon hours */
        if (!MsvpCheckLogonHours(&UserInfo->All.LogonHours, LogonTime))
        {
            ERR("Invalid logon hours!\n");
            *SubStatus = STATUS_INVALID_LOGON_HOURS;
            Status = STATUS_ACCOUNT_RESTRICTION;
            goto done;
        }

        /* Check workstations */
        if (!MsvpCheckWorkstations(&UserInfo->All.WorkStations, ComputerName->Buffer))
        {
            ERR("Invalid workstation!\n");
            *SubStatus = STATUS_INVALID_WORKSTATION;
            Status = STATUS_ACCOUNT_RESTRICTION;
            goto done;
        }
    }
done:
    if (NT_SUCCESS(Status))
    {
        *UserHandlePtr = UserHandle;
        *AccountDomainSidPtr = AccountDomainSid;
        *UserInfoPtr = UserInfo;
    }
    else
    {
        if (AccountDomainSid != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, AccountDomainSid);

        if (UserHandle != NULL)
            SamrCloseHandle(&UserHandle);

        SamIFree_SAMPR_USER_INFO_BUFFER(UserInfo,
                                        UserAllInformation);
    }

    SamIFree_SAMPR_ULONG_ARRAY(&RelativeIds);
    SamIFree_SAMPR_ULONG_ARRAY(&Use);

    if (DomainHandle != NULL)
        SamrCloseHandle(&DomainHandle);

    if (ServerHandle != NULL)
        SamrCloseHandle(&ServerHandle);

    return Status;
}


static
NTSTATUS
GetNtAuthorityDomainSid(
    _In_ PRPC_SID *Sid)
{
    SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
    ULONG Length = 0;

    Length = RtlLengthRequiredSid(0);
    *Sid = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
    if (*Sid == NULL)
    {
        ERR("Failed to allocate SID\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitializeSid(*Sid,&NtAuthority, 0);

    return STATUS_SUCCESS;
}


NTSTATUS
SamValidateUser(
    _In_ SECURITY_LOGON_TYPE LogonType,
    _In_ PUNICODE_STRING LogonUserName,
    _In_ PUNICODE_STRING LogonDomain,
    _In_ PLSA_SAM_PWD_DATA LogonPwdData,
    _In_ PUNICODE_STRING ComputerName,
    _Out_ PBOOL SpecialAccount,
    _Out_ PRPC_SID* AccountDomainSidPtr,
    _Out_ SAMPR_HANDLE* UserHandlePtr,
    _Out_ PSAMPR_USER_INFO_BUFFER* UserInfoPtr,
    _Out_ PNTSTATUS SubStatus)
{
    static const UNICODE_STRING NtAuthorityU = RTL_CONSTANT_STRING(L"NT AUTHORITY");
    static const UNICODE_STRING LocalServiceU = RTL_CONSTANT_STRING(L"LocalService");
    static const UNICODE_STRING NetworkServiceU = RTL_CONSTANT_STRING(L"NetworkService");

    NTSTATUS Status = STATUS_SUCCESS;

    *SpecialAccount = FALSE;
    *UserInfoPtr = NULL;
    *SubStatus = STATUS_SUCCESS;

    /* Check for special accounts */
    // FIXME: Windows does not do this that way!! (msv1_0 does not contain these hardcoded values)
    if (RtlEqualUnicodeString(LogonDomain, &NtAuthorityU, TRUE))
    {
        *SpecialAccount = TRUE;

        /* Get the authority domain SID */
        Status = GetNtAuthorityDomainSid(AccountDomainSidPtr);
        if (!NT_SUCCESS(Status))
        {
            ERR("GetNtAuthorityDomainSid() failed (Status 0x%08lx)\n", Status);
            return Status;
        }

        if (RtlEqualUnicodeString(LogonUserName, &LocalServiceU, TRUE))
        {
            TRACE("SpecialAccount: LocalService\n");

            if (LogonType != Service)
                return STATUS_LOGON_FAILURE;

            *UserInfoPtr = RtlAllocateHeap(RtlGetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           sizeof(SAMPR_USER_ALL_INFORMATION));
            if (*UserInfoPtr == NULL)
                return STATUS_INSUFFICIENT_RESOURCES;

            (*UserInfoPtr)->All.UserId = SECURITY_LOCAL_SERVICE_RID;
            (*UserInfoPtr)->All.PrimaryGroupId = SECURITY_LOCAL_SERVICE_RID;
        }
        else if (RtlEqualUnicodeString(LogonUserName, &NetworkServiceU, TRUE))
        {
            TRACE("SpecialAccount: NetworkService\n");

            if (LogonType != Service)
                return STATUS_LOGON_FAILURE;

            *UserInfoPtr = RtlAllocateHeap(RtlGetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           sizeof(SAMPR_USER_ALL_INFORMATION));
            if (*UserInfoPtr == NULL)
                return STATUS_INSUFFICIENT_RESOURCES;

            (*UserInfoPtr)->All.UserId = SECURITY_NETWORK_SERVICE_RID;
            (*UserInfoPtr)->All.PrimaryGroupId = SECURITY_NETWORK_SERVICE_RID;
        }
        else
        {
            return STATUS_NO_SUCH_USER;
        }
    }
    else
    {
        TRACE("NormalAccount\n");
        Status = SamValidateNormalUser(LogonUserName,
                                       LogonPwdData,
                                       ComputerName,
                                       AccountDomainSidPtr,
                                       UserHandlePtr,
                                       UserInfoPtr,
                                       SubStatus);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamValidateNormalUser() failed (Status 0x%08lx)\n", Status);
            return Status;
        }
    }

    return Status;
}
