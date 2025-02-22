/*
 * PROJECT:         ReactOS msgina.dll
 * FILE:            dll/win32/msgina/gui.c
 * PURPOSE:         ReactOS Logon GINA DLL
 * PROGRAMMER:      Eric Kohl
 */

#include "msgina.h"

NTSTATUS
ConnectToLsa(
    PGINA_CONTEXT pgContext)
{
    LSA_STRING LogonProcessName;
    LSA_STRING PackageName;
    LSA_OPERATIONAL_MODE SecurityMode = 0;
    NTSTATUS Status;

    /* We are already connected to the LSA */
    if (pgContext->LsaHandle != NULL)
        return STATUS_SUCCESS;

    /* Connect to the LSA server */
    RtlInitAnsiString((PANSI_STRING)&LogonProcessName,
                      "MSGINA");

    Status = LsaRegisterLogonProcess(&LogonProcessName,
                                     &pgContext->LsaHandle,
                                     &SecurityMode);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaRegisterLogonProcess failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Get the authentication package */
    RtlInitAnsiString((PANSI_STRING)&PackageName,
                      MSV1_0_PACKAGE_NAME);

    Status = LsaLookupAuthenticationPackage(pgContext->LsaHandle,
                                            &PackageName,
                                            &pgContext->AuthenticationPackage);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaLookupAuthenticationPackage failed (Status 0x%08lx)\n", Status);
    }

    return Status;
}

static const CHAR User32TokenSourceName[] = "User32  ";
C_ASSERT(sizeof(User32TokenSourceName) == RTL_FIELD_SIZE(TOKEN_SOURCE, SourceName) + 1);

NTSTATUS
MyLogonUser(
    HANDLE LsaHandle,
    ULONG AuthenticationPackage,
    LPWSTR lpszUsername,
    LPWSTR lpszDomain,
    LPWSTR lpszPassword,
    PHANDLE phToken,
    PNTSTATUS SubStatus)
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
    NTSTATUS Status;

    *phToken = NULL;

    RtlInitAnsiString((PANSI_STRING)&OriginName,
                      "MSGINA Logon");

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
    RtlCopyMemory(TokenSource.SourceName, User32TokenSourceName, sizeof(TokenSource.SourceName));
    AllocateLocallyUniqueId(&TokenSource.SourceIdentifier);

    Status = LsaLogonUser(LsaHandle,
                          &OriginName,
                          Interactive,
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
                          SubStatus);
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

    return Status;
}

/* EOF */
