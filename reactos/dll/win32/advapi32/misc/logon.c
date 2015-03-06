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


/*
 * @implemented
 */
BOOL WINAPI
CreateProcessAsUserA(HANDLE hToken,
                     LPCSTR lpApplicationName,
                     LPSTR lpCommandLine,
                     LPSECURITY_ATTRIBUTES lpProcessAttributes,
                     LPSECURITY_ATTRIBUTES lpThreadAttributes,
                     BOOL bInheritHandles,
                     DWORD dwCreationFlags,
                     LPVOID lpEnvironment,
                     LPCSTR lpCurrentDirectory,
                     LPSTARTUPINFOA lpStartupInfo,
                     LPPROCESS_INFORMATION lpProcessInformation)
{
    PROCESS_ACCESS_TOKEN AccessToken;
    NTSTATUS Status;

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
        ERR("CreateProcessA failed! GLE: %d\n", GetLastError());
        return FALSE;
    }

    AccessToken.Token = hToken;
    AccessToken.Thread = NULL;

    /* Set the new process token */
    Status = NtSetInformationProcess(lpProcessInformation->hProcess,
                                     ProcessAccessToken,
                                     (PVOID)&AccessToken,
                                     sizeof(AccessToken));
    if (!NT_SUCCESS (Status))
    {
        ERR("NtSetInformationProcess failed: 0x%08x\n", Status);
        TerminateProcess(lpProcessInformation->hProcess, Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
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
BOOL WINAPI
CreateProcessAsUserW(HANDLE hToken,
                     LPCWSTR lpApplicationName,
                     LPWSTR lpCommandLine,
                     LPSECURITY_ATTRIBUTES lpProcessAttributes,
                     LPSECURITY_ATTRIBUTES lpThreadAttributes,
                     BOOL bInheritHandles,
                     DWORD dwCreationFlags,
                     LPVOID lpEnvironment,
                     LPCWSTR lpCurrentDirectory,
                     LPSTARTUPINFOW lpStartupInfo,
                     LPPROCESS_INFORMATION lpProcessInformation)
{
    PROCESS_ACCESS_TOKEN AccessToken;
    NTSTATUS Status;

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
        ERR("CreateProcessW failed! GLE: %d\n", GetLastError());
        return FALSE;
    }

    AccessToken.Token = hToken;
    AccessToken.Thread = NULL;

    /* Set the new process token */
    Status = NtSetInformationProcess(lpProcessInformation->hProcess,
                                     ProcessAccessToken,
                                     (PVOID)&AccessToken,
                                     sizeof(AccessToken));
    if (!NT_SUCCESS (Status))
    {
        ERR("NtSetInformationProcess failed: 0x%08x\n", Status);
        TerminateProcess(lpProcessInformation->hProcess, Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
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
BOOL WINAPI
LogonUserA(LPSTR lpszUsername,
           LPSTR lpszDomain,
           LPSTR lpszPassword,
           DWORD dwLogonType,
           DWORD dwLogonProvider,
           PHANDLE phToken)
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

    ret = LogonUserW(UserName.Buffer,
                     Domain.Buffer,
                     Password.Buffer,
                     dwLogonType,
                     dwLogonProvider,
                     phToken);

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
BOOL WINAPI
LogonUserW(LPWSTR lpszUsername,
           LPWSTR lpszDomain,
           LPWSTR lpszPassword,
           DWORD dwLogonType,
           DWORD dwLogonProvider,
           PHANDLE phToken)
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

    /* Create the Logon SID*/
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

    /* Create the Local SID*/
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
    strncpy(TokenSource.SourceName, "Advapi  ", sizeof(TokenSource.SourceName));
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

    *phToken = TokenHandle;

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
