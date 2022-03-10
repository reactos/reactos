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


/**
 * @brief
 * Sets a primary token to the newly created process.
 * The primary token that gets assigned to is a token
 * whose security context is associated with the logged
 * in user. For futher documentation information, see
 * Remarks.
 *
 * @param[in] ImpersonateAsSelf
 * If set to TRUE, the function will act on behalf of
 * the calling process by impersonating its security context.
 * Generally the caller will disable impersonation and attempt
 * to act on behalf of the said main process as a first tentative
 * to acquire the needed privilege in order to assign a token
 * to the process. If set to FALSE, the function won't act on behalf
 * of the calling process.
 *
 * @param[in] ProcessHandle
 * A handle to the newly created process. The function will use it
 * as a mean to assign the primary token to this process.
 *
 * @param[in] ThreadHandle
 * A handle to the newly and primary created thread associated with
 * the process.
 *
 * @param[in] DuplicatedTokenHandle
 * A handle to a duplicated access token. This token represents as a primary
 * one, initially duplicated in form as a primary type from an impersonation
 * type.
 *
 * @return
 * STATUS_SUCCESS is returned if token assignment to process succeeded, otherwise
 * a failure NTSTATUS code is returned. A potential failure status code is
 * STATUS_ACCESS_DENIED which means the caller doesn't have enough rights
 * to grant access for primary token assignment to process.
 *
 * @remarks
 * This function acts like an internal helper for CreateProcessAsUserCommon (and as
 * such for CreateProcessAsUserW/A as well) as once a process is created, the
 * function is tasked to assign the security context of the logged in user to
 * that process. However, the rate of success of inserting the token into the
 * process ultimately depends on the caller.
 *
 * The caller will either succeed or fail at acquiring SE_ASSIGNPRIMARYTOKEN_PRIVILEGE
 * privilege depending on the security context of the user. If it's allowed, the caller
 * would generally acquire such privilege immediately but if not, the caller will attempt
 * to do a second try.
 */
static
NTSTATUS
InsertTokenToProcessCommon(
    _In_ BOOL ImpersonateAsSelf,
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE DuplicatedTokenHandle)
{
    NTSTATUS Status;
    PROCESS_ACCESS_TOKEN AccessToken;
    BOOLEAN PrivilegeSet;
    BOOLEAN HavePrivilege;

    /*
     * Assume the SE_ASSIGNPRIMARYTOKEN_PRIVILEGE
     * privilege hasn't been set.
     */
    PrivilegeSet = FALSE;

    /*
     * The caller asked that we must impersonate as
     * ourselves, that is, we'll be going to impersonate
     * the security context of the calling process. If
     * self impersonation fails then the caller has
     * to do a "rinse and repeat" approach.
     */
    if (ImpersonateAsSelf)
    {
        Status = RtlImpersonateSelf(SecurityImpersonation);
        if (!NT_SUCCESS(Status))
        {
            ERR("RtlImpersonateSelf(SecurityImpersonation) failed, Status 0x%08x\n", Status);
            return Status;
        }
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

    /*
     * Assign the duplicated token and thread
     * handle to the structure so that we'll
     * use it to assign the primary token
     * to process.
     */
    AccessToken.Token = DuplicatedTokenHandle;
    AccessToken.Thread = ThreadHandle;

    /* Set the new process token */
    Status = NtSetInformationProcess(ProcessHandle,
                                     ProcessAccessToken,
                                     (PVOID)&AccessToken,
                                     sizeof(AccessToken));

    /* Restore the privilege */
    if (HavePrivilege)
    {
        RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE,
                           PrivilegeSet, TRUE, &PrivilegeSet);
    }

    /*
     * Check again if the caller wanted to impersonate
     * as self. If that is the case we must revert this
     * impersonation back.
     */
    if (ImpersonateAsSelf)
    {
        RevertToSelf();
    }

    /*
     * Finally, check if we actually succeeded on assigning
     * a primary token to the process. If we failed, oh well,
     * asta la vista baby e arrivederci. The caller has to do
     * a rinse and repeat approach.
     */
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to assign primary token to the process (Status 0x%08lx)\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Internal function that serves as a helper for
 * CreateProcessAsUserW/A routines on creating
 * a process within the context of the logged in
 * user.
 *
 * @param[in] hToken
 * A handle to an access token that is associated
 * with the logged in user. If the caller does not
 * submit a token, the helper will immediately quit
 * and return success, and the newly created process
 * will be created upon using the default security
 * context.
 *
 * @param[in] dwCreationFlags
 * Bit masks containing the creation process flags.
 * The function uses this parameter to determine
 * if the process wasn't created in a suspended way
 * and if not the function will resume the main thread.
 *
 * @param[in,out] lpProcessInformation
 * A pointer to a structure that contains process creation
 * information data. Such pointer contains the process
 * and thread handles and whatnot.
 *
 * @return
 * Returns TRUE if the helper has successfully assigned
 * the newly created process the user's security context
 * to that process, otherwise FALSE is returned.
 *
 * @remarks
 * In order for the helper function to assign the primary
 * token to the process, it has to do a "rinse and repeat"
 * approach. That is, the helper will stop the impersonation
 * and attempt to assign the token to process by acting
 * on behalf of the main process' security context. If that
 * fails, the function will do a second attempt by doing this
 * but with impersonation enabled instead.
 */
static
BOOL
CreateProcessAsUserCommon(
    _In_opt_ HANDLE hToken,
    _In_ DWORD dwCreationFlags,
    _Inout_ LPPROCESS_INFORMATION lpProcessInformation)
{
    NTSTATUS Status = STATUS_SUCCESS, StatusOnExit;
    BOOL Success;
    TOKEN_TYPE Type;
    ULONG ReturnLength;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hTokenDup = NULL;
    HANDLE OriginalImpersonationToken = NULL;
    HANDLE NullToken = NULL;

    if (hToken != NULL)
    {
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
            Success = FALSE;
            goto Quit;
        }

        if (Type != TokenPrimary)
        {
            ERR("Wrong token type for token 0x%p, expected TokenPrimary, got %ld\n", hToken, Type);
            Status = STATUS_BAD_TOKEN_TYPE;
            Success = FALSE;
            goto Quit;
        }

        /*
         * Open the original token of the calling thread
         * and halt the impersonation for the moment
         * being. The opened thread token will be cached
         * so that we will restore it back when we're done.
         */
        Status = NtOpenThreadToken(NtCurrentThread(),
                                   TOKEN_QUERY | TOKEN_IMPERSONATE,
                                   TRUE,
                                   &OriginalImpersonationToken);
        if (!NT_SUCCESS(Status))
        {
            /* We failed? Does this thread have a token at least? */
            OriginalImpersonationToken = NULL;
            if (Status != STATUS_NO_TOKEN)
            {
                /*
                 * OK so this thread has a token but we
                 * could not open it for whatever reason.
                 * Bail out then.
                 */
                ERR("Failed to open thread token with 0x%08lx\n", Status);
                Success = FALSE;
                goto Quit;
            }
        }
        else
        {
            /* We succeeded, stop the impersonation for now */
            Status = NtSetInformationThread(NtCurrentThread(),
                                            ThreadImpersonationToken,
                                            &NullToken,
                                            sizeof(NullToken));
            if (!NT_SUCCESS(Status))
            {
                ERR("Failed to stop impersonation with 0x%08lx\n", Status);
                Success = FALSE;
                goto Quit;
            }
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
            Success = FALSE;
            goto Quit;
        }

        /*
         * Now it's time to set the primary token into
         * the process. On the first try, do it by
         * impersonating the security context of the
         * calling process (impersonate as self).
         */
        Status = InsertTokenToProcessCommon(TRUE,
                                            lpProcessInformation->hProcess,
                                            lpProcessInformation->hThread,
                                            hTokenDup);
        if (!NT_SUCCESS(Status))
        {
            /*
             * OK, we failed. Our second (and last try) is to not
             * impersonate as self but instead we will try by setting
             * the original impersonation (thread) token and set the
             * primary token to the process through this way. This is
             * what we call -- the "rinse and repeat" approach.
             */
            Status = NtSetInformationThread(NtCurrentThread(),
                                            ThreadImpersonationToken,
                                            &OriginalImpersonationToken,
                                            sizeof(OriginalImpersonationToken));
            if (!NT_SUCCESS(Status))
            {
                ERR("Failed to restore impersonation token for setting process token, Status 0x%08lx\n", Status);
                NtClose(hTokenDup);
                Success = FALSE;
                goto Quit;
            }

            /* Retry again */
            Status = InsertTokenToProcessCommon(FALSE,
                                                lpProcessInformation->hProcess,
                                                lpProcessInformation->hThread,
                                                hTokenDup);
            if (!NT_SUCCESS(Status))
            {
                /* Even the second try failed, bail out... */
                ERR("Failed to insert the primary token into process, Status 0x%08lx\n", Status);
                NtClose(hTokenDup);
                Success = FALSE;
                goto Quit;
            }

            /* All good, now stop impersonation */
            Status = NtSetInformationThread(NtCurrentThread(),
                                            ThreadImpersonationToken,
                                            &NullToken,
                                            sizeof(NullToken));
            if (!NT_SUCCESS(Status))
            {
                ERR("Failed to unset impersonationg token after setting process token, Status 0x%08lx\n", Status);
                NtClose(hTokenDup);
                Success = FALSE;
                goto Quit;
            }
        }

        /*
         * FIXME: As we have successfully set up a primary token to
         * the newly created process, we must set up as well a definite
         * limit of quota charges for this process on the context of
         * this user.
         */

        /* Close the duplicated token */
        NtClose(hTokenDup);
        Success = TRUE;
    }

    /*
     * If the caller did not supply a token then just declare
     * ourselves as job done. The newly created process will use
     * the default security context at this point anyway.
     */
    TRACE("No token supplied, the process will use default security context!\n");
    Success = TRUE;

Quit:
    /*
     * If we successfully opened the thread token before
     * and stopped the impersonation then we have to assign
     * its original token back and close that token we have
     * referenced it.
     */
    if (OriginalImpersonationToken != NULL)
    {
        StatusOnExit = NtSetInformationThread(NtCurrentThread(),
                                              ThreadImpersonationToken,
                                              &OriginalImpersonationToken,
                                              sizeof(OriginalImpersonationToken));

        /*
         * We really must assert ourselves that we successfully
         * set the original token back, otherwise if we fail
         * then something is seriously going wrong....
         * The status code is cached in a separate status
         * variable because we would not want to tamper
         * with the original status code that could have been
         * returned by someone else above in this function code.
         */
        ASSERT(NT_SUCCESS(StatusOnExit));

        /* De-reference it */
        NtClose(OriginalImpersonationToken);
    }

    /* Terminate the process and set the last error status */
    if (!NT_SUCCESS(Status))
    {
        TerminateProcess(lpProcessInformation->hProcess, Status);
        SetLastError(RtlNtStatusToDosError(Status));
    }

    /* Resume the main thread */
    if (!(dwCreationFlags & CREATE_SUSPENDED))
    {
        ResumeThread(lpProcessInformation->hThread);
    }

    return Success;
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
