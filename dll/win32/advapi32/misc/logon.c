/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/advapi32/misc/logon.c
 * PURPOSE:     Logon functions
 * PROGRAMMER:  Eric Kohl
 */

#include <advapi32.h>
WINE_DEFAULT_DEBUG_CHANNEL(advapi);

/* GLOBALS *****************************************************************/

static const CHAR AdvapiTokenSourceName[] = "Advapi  ";
C_ASSERT(sizeof(AdvapiTokenSourceName) == RTL_FIELD_SIZE(TOKEN_SOURCE, SourceName) + 1);

HANDLE LsaHandle = NULL;
ULONG AuthenticationPackage = 0;

/* FUNCTIONS ***************************************************************/

static
NTSTATUS
OpenLogonLsaHandle(VOID)
{
    LSA_STRING LogonProcessName;
    LSA_STRING PackageName;
    LSA_OPERATIONAL_MODE SecurityMode = 0;
    NTSTATUS Status;

    RtlInitAnsiString((PANSI_STRING)&LogonProcessName,
                      "User32LogonProcess");

    Status = LsaRegisterLogonProcess(&LogonProcessName,
                                     &LsaHandle,
                                     &SecurityMode);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LsaRegisterLogonProcess failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    RtlInitAnsiString((PANSI_STRING)&PackageName,
                      MSV1_0_PACKAGE_NAME);

    Status = LsaLookupAuthenticationPackage(LsaHandle,
                                            &PackageName,
                                            &AuthenticationPackage);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LsaLookupAuthenticationPackage failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    TRACE("AuthenticationPackage: 0x%08lx\n", AuthenticationPackage);

done:
    if (!NT_SUCCESS(Status))
    {
        if (LsaHandle != NULL)
        {
            Status = LsaDeregisterLogonProcess(LsaHandle);
            if (!NT_SUCCESS(Status))
            {
                TRACE("LsaDeregisterLogonProcess failed (Status 0x%08lx)\n", Status);
            }
        }
    }

    return Status;
}


NTSTATUS
CloseLogonLsaHandle(VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (LsaHandle != NULL)
    {
        Status = LsaDeregisterLogonProcess(LsaHandle);
        if (!NT_SUCCESS(Status))
        {
            TRACE("LsaDeregisterLogonProcess failed (Status 0x%08lx)\n", Status);
        }
    }

    return Status;
}


static
BOOL
CreateProcessAsUserCommon(
    _In_opt_ HANDLE hToken,
    _In_ DWORD dwCreationFlags,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation)
{
    NTSTATUS Status;
    PROCESS_ACCESS_TOKEN AccessToken;

    if (hToken != NULL)
    {
        TOKEN_TYPE Type;
        ULONG ReturnLength;
        OBJECT_ATTRIBUTES ObjectAttributes;
        HANDLE hTokenDup;
        BOOLEAN PrivilegeSet = FALSE, HavePrivilege;

        /* Check whether the user-provided token is a primary token */
        // GetTokenInformation();
        Status = NtQueryInformationToken(hToken,
                                         TokenType,
                                         &Type,
                                         sizeof(Type),
                                         &ReturnLength);
        if (!NT_SUCCESS(Status))
        {
            ERR("NtQueryInformationToken() failed, Status 0x%08x\n", Status);
            goto Quit;
        }
        if (Type != TokenPrimary)
        {
            ERR("Wrong token type for token 0x%p, expected TokenPrimary, got %ld\n", hToken, Type);
            Status = STATUS_BAD_TOKEN_TYPE;
            goto Quit;
        }

        /* Duplicate the token for this new process */
        InitializeObjectAttributes(&ObjectAttributes,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL); // FIXME: Use a valid SecurityDescriptor!
        Status = NtDuplicateToken(hToken,
                                  0,
                                  &ObjectAttributes,
                                  FALSE,
                                  TokenPrimary,
                                  &hTokenDup);
        if (!NT_SUCCESS(Status))
        {
            ERR("NtDuplicateToken() failed, Status 0x%08x\n", Status);
            goto Quit;
        }

        // FIXME: Do we always need SecurityImpersonation?
        Status = RtlImpersonateSelf(SecurityImpersonation);
        if (!NT_SUCCESS(Status))
        {
            ERR("RtlImpersonateSelf(SecurityImpersonation) failed, Status 0x%08x\n", Status);
            NtClose(hTokenDup);
            goto Quit;
        }

        /*
         * Attempt to acquire the process primary token assignment privilege
         * in case we actually need it.
         * The call will either succeed or fail when the caller has (or has not)
         * enough rights.
         * The last situation may not be dramatic for us. Indeed it may happen
         * that the user-provided token is a restricted version of the caller's
         * primary token (aka. a "child" token), or both tokens inherit (i.e. are
         * children, and are together "siblings") from a common parent token.
         * In this case the NT kernel allows us to assign the token to the child
         * process without the need for the assignment privilege, which is fine.
         * On the contrary, if the user-provided token is completely arbitrary,
         * then the NT kernel will enforce the presence of the assignment privilege:
         * because we failed (by assumption) to assign the privilege, the process
         * token assignment will fail as required. It is then the job of the
         * caller to manually acquire the necessary privileges.
         */
        Status = RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE,
                                    TRUE, TRUE, &PrivilegeSet);
        HavePrivilege = NT_SUCCESS(Status);
        if (!HavePrivilege)
        {
            ERR("RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE) failed, Status 0x%08lx, "
                "attempting to continue without it...\n", Status);
        }

        AccessToken.Token  = hTokenDup;
        AccessToken.Thread = lpProcessInformation->hThread;

        /* Set the new process token */
        Status = NtSetInformationProcess(lpProcessInformation->hProcess,
                                         ProcessAccessToken,
                                         (PVOID)&AccessToken,
                                         sizeof(AccessToken));

        /* Restore the privilege */
        if (HavePrivilege)
        {
            RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE,
                               PrivilegeSet, TRUE, &PrivilegeSet);
        }

        RevertToSelf();

        /* Close the duplicated token */
        NtClose(hTokenDup);

        /* Check whether NtSetInformationProcess() failed */
        if (!NT_SUCCESS(Status))
        {
            ERR("NtSetInformationProcess() failed, Status 0x%08x\n", Status);
            goto Quit;
        }

        if (!NT_SUCCESS(Status))
        {
Quit:
            TerminateProcess(lpProcessInformation->hProcess, Status);
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }
    }

    /* Resume the main thread */
    if (!(dwCreationFlags & CREATE_SUSPENDED))
    {
        ResumeThread(lpProcessInformation->hThread);
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
CreateProcessAsUserA(
    _In_opt_ HANDLE hToken,
    _In_opt_ LPCSTR lpApplicationName,
    _Inout_opt_ LPSTR lpCommandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ BOOL bInheritHandles,
    _In_ DWORD dwCreationFlags,
    _In_opt_ LPVOID lpEnvironment,
    _In_opt_ LPCSTR lpCurrentDirectory,
    _In_ LPSTARTUPINFOA lpStartupInfo,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation)
{
    TRACE("%p %s %s %p %p %d 0x%08x %p %s %p %p\n", hToken, debugstr_a(lpApplicationName),
        debugstr_a(lpCommandLine), lpProcessAttributes, lpThreadAttributes, bInheritHandles,
        dwCreationFlags, lpEnvironment, debugstr_a(lpCurrentDirectory), lpStartupInfo, lpProcessInformation);

    /* Create the process with a suspended main thread */
    if (!CreateProcessA(lpApplicationName,
                        lpCommandLine,
                        lpProcessAttributes,
                        lpThreadAttributes,
                        bInheritHandles,
                        dwCreationFlags | CREATE_SUSPENDED,
                        lpEnvironment,
                        lpCurrentDirectory,
                        lpStartupInfo,
                        lpProcessInformation))
    {
        ERR("CreateProcessA failed, last error: %d\n", GetLastError());
        return FALSE;
    }

    /* Call the helper function */
    return CreateProcessAsUserCommon(hToken,
                                     dwCreationFlags,
                                     lpProcessInformation);
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
CreateProcessAsUserW(
    _In_opt_ HANDLE hToken,
    _In_opt_ LPCWSTR lpApplicationName,
    _Inout_opt_ LPWSTR lpCommandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ BOOL bInheritHandles,
    _In_ DWORD dwCreationFlags,
    _In_opt_ LPVOID lpEnvironment,
    _In_opt_ LPCWSTR lpCurrentDirectory,
    _In_ LPSTARTUPINFOW lpStartupInfo,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation)
{
    TRACE("%p %s %s %p %p %d 0x%08x %p %s %p %p\n", hToken, debugstr_w(lpApplicationName),
        debugstr_w(lpCommandLine), lpProcessAttributes, lpThreadAttributes, bInheritHandles,
        dwCreationFlags, lpEnvironment, debugstr_w(lpCurrentDirectory), lpStartupInfo, lpProcessInformation);

    /* Create the process with a suspended main thread */
    if (!CreateProcessW(lpApplicationName,
                        lpCommandLine,
                        lpProcessAttributes,
                        lpThreadAttributes,
                        bInheritHandles,
                        dwCreationFlags | CREATE_SUSPENDED,
                        lpEnvironment,
                        lpCurrentDirectory,
                        lpStartupInfo,
                        lpProcessInformation))
    {
        ERR("CreateProcessW failed, last error: %d\n", GetLastError());
        return FALSE;
    }

    /* Call the helper function */
    return CreateProcessAsUserCommon(hToken,
                                     dwCreationFlags,
                                     lpProcessInformation);
}


/*
 * @implemented
 */
BOOL
WINAPI
LogonUserA(
    _In_ LPSTR lpszUsername,
    _In_opt_ LPSTR lpszDomain,
    _In_opt_ LPSTR lpszPassword,
    _In_ DWORD dwLogonType,
    _In_ DWORD dwLogonProvider,
    _Out_opt_ PHANDLE phToken)
{
    return LogonUserExA(lpszUsername,
                        lpszDomain,
                        lpszPassword,
                        dwLogonType,
                        dwLogonProvider,
                        phToken,
                        NULL,
                        NULL,
                        NULL,
                        NULL);
}


/*
 * @implemented
 */
BOOL
WINAPI
LogonUserExA(
    _In_ LPSTR lpszUsername,
    _In_opt_ LPSTR lpszDomain,
    _In_opt_ LPSTR lpszPassword,
    _In_ DWORD dwLogonType,
    _In_ DWORD dwLogonProvider,
    _Out_opt_ PHANDLE phToken,
    _Out_opt_ PSID *ppLogonSid,
    _Out_opt_ PVOID *ppProfileBuffer,
    _Out_opt_ LPDWORD pdwProfileLength,
    _Out_opt_ PQUOTA_LIMITS pQuotaLimits)
{
    UNICODE_STRING UserName;
    UNICODE_STRING Domain;
    UNICODE_STRING Password;
    BOOL ret = FALSE;

    UserName.Buffer = NULL;
    Domain.Buffer = NULL;
    Password.Buffer = NULL;

    if (!RtlCreateUnicodeStringFromAsciiz(&UserName, lpszUsername))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto UsernameDone;
    }

    if (!RtlCreateUnicodeStringFromAsciiz(&Domain, lpszDomain))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto DomainDone;
    }

    if (!RtlCreateUnicodeStringFromAsciiz(&Password, lpszPassword))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto PasswordDone;
    }

    ret = LogonUserExW(UserName.Buffer,
                       Domain.Buffer,
                       Password.Buffer,
                       dwLogonType,
                       dwLogonProvider,
                       phToken,
                       ppLogonSid,
                       ppProfileBuffer,
                       pdwProfileLength,
                       pQuotaLimits);

    if (Password.Buffer != NULL)
        RtlFreeUnicodeString(&Password);

PasswordDone:
    if (Domain.Buffer != NULL)
        RtlFreeUnicodeString(&Domain);

DomainDone:
    if (UserName.Buffer != NULL)
        RtlFreeUnicodeString(&UserName);

UsernameDone:
    return ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
LogonUserW(
    _In_ LPWSTR lpszUsername,
    _In_opt_ LPWSTR lpszDomain,
    _In_opt_ LPWSTR lpszPassword,
    _In_ DWORD dwLogonType,
    _In_ DWORD dwLogonProvider,
    _Out_opt_ PHANDLE phToken)
{
    return LogonUserExW(lpszUsername,
                        lpszDomain,
                        lpszPassword,
                        dwLogonType,
                        dwLogonProvider,
                        phToken,
                        NULL,
                        NULL,
                        NULL,
                        NULL);
}


/*
 * @implemented
 */
BOOL
WINAPI
LogonUserExW(
    _In_ LPWSTR lpszUsername,
    _In_opt_ LPWSTR lpszDomain,
    _In_opt_ LPWSTR lpszPassword,
    _In_ DWORD dwLogonType,
    _In_ DWORD dwLogonProvider,
    _Out_opt_ PHANDLE phToken,
    _Out_opt_ PSID *ppLogonSid,
    _Out_opt_ PVOID *ppProfileBuffer,
    _Out_opt_ LPDWORD pdwProfileLength,
    _Out_opt_ PQUOTA_LIMITS pQuotaLimits)
{
    SID_IDENTIFIER_AUTHORITY LocalAuthority = {SECURITY_LOCAL_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};
    PSID LogonSid = NULL;
    PSID LocalSid = NULL;
    LSA_STRING OriginName;
    UNICODE_STRING DomainName;
    UNICODE_STRING UserName;
    UNICODE_STRING Password;
    PMSV1_0_INTERACTIVE_LOGON AuthInfo = NULL;
    ULONG AuthInfoLength;
    ULONG_PTR Ptr;
    TOKEN_SOURCE TokenSource;
    PTOKEN_GROUPS TokenGroups = NULL;
    PMSV1_0_INTERACTIVE_PROFILE ProfileBuffer = NULL;
    ULONG ProfileBufferLength = 0;
    LUID Luid = {0, 0};
    LUID LogonId = {0, 0};
    HANDLE TokenHandle = NULL;
    QUOTA_LIMITS QuotaLimits;
    SECURITY_LOGON_TYPE LogonType;
    NTSTATUS SubStatus = STATUS_SUCCESS;
    NTSTATUS Status;

    if ((ppProfileBuffer != NULL && pdwProfileLength == NULL) ||
        (ppProfileBuffer == NULL && pdwProfileLength != NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (ppProfileBuffer != NULL && pdwProfileLength != NULL)
    {
        *ppProfileBuffer = NULL;
        *pdwProfileLength = 0;
    }

    if (phToken != NULL)
        *phToken = NULL;

    switch (dwLogonType)
    {
        case LOGON32_LOGON_INTERACTIVE:
            LogonType = Interactive;
            break;

        case LOGON32_LOGON_NETWORK:
            LogonType = Network;
            break;

        case LOGON32_LOGON_BATCH:
            LogonType = Batch;
            break;

        case LOGON32_LOGON_SERVICE:
            LogonType = Service;
            break;

       default:
            ERR("Invalid logon type: %ul\n", dwLogonType);
            Status = STATUS_INVALID_PARAMETER;
            goto done;
    }

    if (LsaHandle == NULL)
    {
        Status = OpenLogonLsaHandle();
        if (!NT_SUCCESS(Status))
            goto done;
    }

    RtlInitAnsiString((PANSI_STRING)&OriginName,
                      "Advapi32 Logon");

    RtlInitUnicodeString(&DomainName,
                         lpszDomain);

    RtlInitUnicodeString(&UserName,
                         lpszUsername);

    RtlInitUnicodeString(&Password,
                         lpszPassword);

    AuthInfoLength = sizeof(MSV1_0_INTERACTIVE_LOGON)+
                     DomainName.MaximumLength +
                     UserName.MaximumLength +
                     Password.MaximumLength;

    AuthInfo = RtlAllocateHeap(RtlGetProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               AuthInfoLength);
    if (AuthInfo == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    AuthInfo->MessageType = MsV1_0InteractiveLogon;

    Ptr = (ULONG_PTR)AuthInfo + sizeof(MSV1_0_INTERACTIVE_LOGON);

    AuthInfo->LogonDomainName.Length = DomainName.Length;
    AuthInfo->LogonDomainName.MaximumLength = DomainName.MaximumLength;
    AuthInfo->LogonDomainName.Buffer = (DomainName.Buffer == NULL) ? NULL : (PWCHAR)Ptr;
    if (DomainName.MaximumLength > 0)
    {
        RtlCopyMemory(AuthInfo->LogonDomainName.Buffer,
                      DomainName.Buffer,
                      DomainName.MaximumLength);

        Ptr += DomainName.MaximumLength;
    }

    AuthInfo->UserName.Length = UserName.Length;
    AuthInfo->UserName.MaximumLength = UserName.MaximumLength;
    AuthInfo->UserName.Buffer = (PWCHAR)Ptr;
    if (UserName.MaximumLength > 0)
        RtlCopyMemory(AuthInfo->UserName.Buffer,
                      UserName.Buffer,
                      UserName.MaximumLength);

    Ptr += UserName.MaximumLength;

    AuthInfo->Password.Length = Password.Length;
    AuthInfo->Password.MaximumLength = Password.MaximumLength;
    AuthInfo->Password.Buffer = (PWCHAR)Ptr;
    if (Password.MaximumLength > 0)
        RtlCopyMemory(AuthInfo->Password.Buffer,
                      Password.Buffer,
                      Password.MaximumLength);

    /* Create the Logon SID */
    AllocateLocallyUniqueId(&LogonId);
    Status = RtlAllocateAndInitializeSid(&SystemAuthority,
                                         SECURITY_LOGON_IDS_RID_COUNT,
                                         SECURITY_LOGON_IDS_RID,
                                         LogonId.HighPart,
                                         LogonId.LowPart,
                                         SECURITY_NULL_RID,
                                         SECURITY_NULL_RID,
                                         SECURITY_NULL_RID,
                                         SECURITY_NULL_RID,
                                         SECURITY_NULL_RID,
                                         &LogonSid);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Local SID */
    Status = RtlAllocateAndInitializeSid(&LocalAuthority,
                                         1,
                                         SECURITY_LOCAL_RID,
                                         SECURITY_NULL_RID,
                                         SECURITY_NULL_RID,
                                         SECURITY_NULL_RID,
                                         SECURITY_NULL_RID,
                                         SECURITY_NULL_RID,
                                         SECURITY_NULL_RID,
                                         SECURITY_NULL_RID,
                                         &LocalSid);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Allocate and set the token groups */
    TokenGroups = RtlAllocateHeap(RtlGetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  sizeof(TOKEN_GROUPS) + ((2 - ANYSIZE_ARRAY) * sizeof(SID_AND_ATTRIBUTES)));
    if (TokenGroups == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    TokenGroups->GroupCount = 2;
    TokenGroups->Groups[0].Sid = LogonSid;
    TokenGroups->Groups[0].Attributes = SE_GROUP_MANDATORY | SE_GROUP_ENABLED |
                                        SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_LOGON_ID;
    TokenGroups->Groups[1].Sid = LocalSid;
    TokenGroups->Groups[1].Attributes = SE_GROUP_MANDATORY | SE_GROUP_ENABLED |
                                        SE_GROUP_ENABLED_BY_DEFAULT;

    /* Set the token source */
    RtlCopyMemory(TokenSource.SourceName,
                  AdvapiTokenSourceName,
                  sizeof(TokenSource.SourceName));
    AllocateLocallyUniqueId(&TokenSource.SourceIdentifier);

    Status = LsaLogonUser(LsaHandle,
                          &OriginName,
                          LogonType,
                          AuthenticationPackage,
                          (PVOID)AuthInfo,
                          AuthInfoLength,
                          TokenGroups,
                          &TokenSource,
                          (PVOID*)&ProfileBuffer,
                          &ProfileBufferLength,
                          &Luid,
                          &TokenHandle,
                          &QuotaLimits,
                          &SubStatus);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaLogonUser failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    if (ProfileBuffer != NULL)
    {
        TRACE("ProfileBuffer: %p\n", ProfileBuffer);
        TRACE("MessageType: %u\n", ProfileBuffer->MessageType);

        TRACE("FullName: %p\n", ProfileBuffer->FullName.Buffer);
        TRACE("FullName: %S\n", ProfileBuffer->FullName.Buffer);

        TRACE("LogonServer: %p\n", ProfileBuffer->LogonServer.Buffer);
        TRACE("LogonServer: %S\n", ProfileBuffer->LogonServer.Buffer);
    }

    TRACE("Luid: 0x%08lx%08lx\n", Luid.HighPart, Luid.LowPart);

    if (TokenHandle != NULL)
    {
        TRACE("TokenHandle: %p\n", TokenHandle);
    }

    if (phToken != NULL)
        *phToken = TokenHandle;

    /* FIXME: return ppLogonSid and pQuotaLimits */

done:
    if (ProfileBuffer != NULL)
        LsaFreeReturnBuffer(ProfileBuffer);

    if (!NT_SUCCESS(Status))
    {
        if (TokenHandle != NULL)
            CloseHandle(TokenHandle);
    }

    if (TokenGroups != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, TokenGroups);

    if (LocalSid != NULL)
        RtlFreeSid(LocalSid);

    if (LogonSid != NULL)
        RtlFreeSid(LogonSid);

    if (AuthInfo != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AuthInfo);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/* EOF */
