/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/msv1_0.c
 * PURPOSE:     Functions needed to fill PSECPKG_USER_FUNCTION_TABLE
 *              (SpUserModeInitialize)
 * COPYRIGHT:   Copyright 2019 Andreas Maier <staubim@quantentunnel.de>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(msv1_0);

/* LsaSpInitialize will fill this vars */
ULONG_PTR LsaPackageId = 0;
LUID NtLmGlobalLuidMachineLogon;
DWORD NtLmPackageId = 0;
PSECPKG_PARAMETERS LsaParameters = NULL;

PLSA_SECPKG_FUNCTION_TABLE LsaFunctions = NULL;
SECPKG_FUNCTION_TABLE NtlmLsaFn[1];

#define MAX_PATH_SHORT 104


NTSTATUS NTAPI
LsaApLogonUser(
    LPWSTR p1,
    LPWSTR p2,
    LPWSTR p3,
    LPWSTR p4,
    DWORD p5,
    DWORD p6,
    PHANDLE p7)
{
    fdTRACE("*** UNIMPLEMENTED *** LsaApLogonUser(%S %S %S %S %i %i %p)\n",
          p1, p2, p3, p4, p5, p6, p7);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
LsaApLogonUserEx(
    PLSA_CLIENT_REQUEST p1,
    SECURITY_LOGON_TYPE p2,
    PVOID p3,
    PVOID p4,
    ULONG p5,
    PVOID *p6,
    PULONG p7,
    PLUID p8,
    PNTSTATUS p9,
    PLSA_TOKEN_INFORMATION_TYPE p10,
    PVOID *p11,
    PUNICODE_STRING *p12,
    PUNICODE_STRING *p13,
    PUNICODE_STRING *p14)
{
    fdTRACE("*** UNIMPLEMENTED *** LsaApLogonUserEx(%p %i %p %p %i %p %p %p %p %p %p %p %p %p)\n",
          p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14);

    return ERROR_NOT_SUPPORTED;
}


typedef NTSTATUS (NTAPI *LsaIRegisterNotification)(
    void* p1,
    void* p2,
    void* p3,
    void* p4,
    void* p5,
    void* p6,
    void* p7);

/**
 * @brief Initialize LSA
 * @param PackageId Unique Package Id
 * @param Parameters
 * @param FunctionTable functions that we should call
 */
NTSTATUS
NTAPI
SpInitialize(
    _In_ ULONG_PTR PackageId,
    _In_ PSECPKG_PARAMETERS Parameters,
    _In_ PLSA_SECPKG_FUNCTION_TABLE FunctionTable)
{
    NTSTATUS status;

    fdTRACE("LsaSpInitialize\n");
    fdTRACE("> PackageId %0xlx, Parameters %p, FunctionTable %p\n",
        PackageId, Parameters, FunctionTable);

    //_SEH2_TRY
    {
        NtLmPackageId = PackageId;
        LsaFunctions = FunctionTable;

        NtlmInit(NtlmLsaMode);

        status = NtAllocateLocallyUniqueId(&NtLmGlobalLuidMachineLogon);
        if (!NT_SUCCESS(status))
        {
            ERR("AllocateLocallyUniqueId failed!\n");
            goto done;
        }

        status = LsaFunctions->CreateLogonSession(&NtLmGlobalLuidMachineLogon);
        if (!NT_SUCCESS(status))
        {
            ERR("AllocateLocallyUniqueId failed (0x%x)!\n", status);
            goto done;
        }
        TRACE("Logon session id created %x.%x\n",
              NtLmGlobalLuidMachineLogon.LowPart,
              NtLmGlobalLuidMachineLogon.HighPart);

        TRACE("ok\n");
    }
    //_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
     //   status = STATUS_INTERNAL_ERROR;
    }

    //_SEH2_END;
done:
    return status;
}

NTSTATUS NTAPI
LsaSpShutDown(void)
{
    fdTRACE("LsaSpShutDown()\n");

    //TODO FiniLsaPort + port freigeben / löschen
    NtlmFini();

    LsaFunctions = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
LsaSpAcceptCredentials(
    _In_ SECURITY_LOGON_TYPE LogonType,
    _In_ PUNICODE_STRING AccountName,
    _In_ PSECPKG_PRIMARY_CRED PrimaryCredentials)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (PrimaryCredentials->Flags && SECPKG_CRED_INBOUND)
    {
        // FIXME: Check special Login Id's
        if ((PrimaryCredentials->LogonId.LowPart == 0x3E7) &&
            (PrimaryCredentials->LogonId.HighPart != 0))
        {
            // todo ...
            goto done;
        }
    }
done:
    return status;
}

/* MS doc says name must be SpAcceptCredentials! */
NTSTATUS
NTAPI
SpAcceptCredentials(
    _In_ SECURITY_LOGON_TYPE LogonType,
    _In_ PUNICODE_STRING AccountName,
    _In_ PSECPKG_PRIMARY_CRED PrimaryCredentials,
    _In_ PSECPKG_SUPPLEMENTAL_CRED SupplementalCredentials)
{
    fdTRACE("LsaSpAcceptCredentials(0x%lx %p %p %p)\n",
          LogonType, AccountName, PrimaryCredentials, SupplementalCredentials);

    LsaSpAcceptCredentials(LogonType, AccountName, PrimaryCredentials);

    // Wenn Flags != 0
    // -> (oder was)
    // -> System-Logon
    // FIXE Logon-Ids!
    //SYSTEM 	3e7 	999
    //NETWORK SERVICE 	3e4 	996
    //LOCAL SERVICE 	3e5 	997
    //IUSR 	3e3 	995

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaSpAcquireCredentialsHandle(
    PUNICODE_STRING PrincipalName,
    ULONG CredentialUseFlags,
    PLUID LogonId,
    PVOID AuthorizationData,
    PVOID GetKeyFunciton,
    PVOID GetKeyArgument,
    PLSA_SEC_HANDLE CredentialHandle,
    PTimeStamp ExpirationTime)
{
    NTSTATUS status;
    WCHAR* PrincipalW = NULL;

    fdTRACE("LsaSpAcquireCredentialsHandle(%p %i %p %p %p %p %p %p)\n",
          PrincipalName, CredentialUseFlags, LogonId,
          AuthorizationData, GetKeyFunciton, GetKeyArgument,
          CredentialHandle, ExpirationTime);

    if (PrincipalName)
        PrincipalW = PrincipalName->Buffer;

    status = NtlmAcquireCredentialsHandle(
        PrincipalW,
        NULL,
        CredentialUseFlags,
        LogonId,
        AuthorizationData,
        GetKeyFunciton,
        GetKeyArgument,
        CredentialHandle,
        ExpirationTime);

    fdTRACE("RESULT 0x%x, Handle 0x%x\n", status, *CredentialHandle);
    
    return status;
}

NTSTATUS NTAPI
LsaSpQueryCredentialsAttributes(
    LSA_SEC_HANDLE CredentialHandle,
    ULONG CredentialAttribute,
    PVOID Buffer)
{
    SECURITY_STATUS status;

    fdTRACE("LsaSpQueryCredentialsAttributes(%p %i %p) {\n",
          CredentialHandle, CredentialAttribute, Buffer);

    // FIXME we mix SECURITY_STATUS + NTSTATUS ..
    status = NtlmQueryCredentialsAttributes(
        CredentialHandle, CredentialAttribute, Buffer);

    fdTRACE("RESULT 0x%x }\n", status);

    return status;
}

NTSTATUS NTAPI
LsaSpFreeCredentialsHandle(
    IN LSA_SEC_HANDLE CredentialHandle)
{
    SECURITY_STATUS status;

    fdTRACE("LsaSpFreeCredentialsHandle(%p) {\n", CredentialHandle);
    status = NtlmFreeCredentialsHandle(CredentialHandle);
    fdTRACE("RESULT 0x%x }\n", status);

    return status;
}

NTSTATUS NTAPI
LsaSpSaveCredentials(
    LSA_SEC_HANDLE p1,
    PSecBuffer p2)
{
    fdTRACE("*** UNIMPLEMENTED *** LsaSpSaveCredentials(%p %p)\n", p1, p2);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
LsaSpGetCredentials(
    LSA_SEC_HANDLE p1,
    PSecBuffer p2)
{
    fdTRACE("*** UNIMPLEMENTED *** LsaSpGetCredentials(%p %p)\n", p1, p2);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
LsaSpDeleteCredentials(
    LSA_SEC_HANDLE p1,
    PSecBuffer p2)
{
    fdTRACE("*** UNIMPLEMENTED *** LsaSpDeleteCredentials(%p %p)\n", p1, p2);

    return ERROR_NOT_SUPPORTED;
}

/**
 * @brief return general infos about our security package
 * @param PSecPkgInfoW
 */
NTSTATUS
NTAPI
LsaSpGetInfoW(
    _Out_ PSecPkgInfoW PackageInfo)
{
    fdTRACE("LsaGetInfo(%p)\n", PackageInfo);

    if (PackageInfo == NULL)
        return STATUS_INVALID_PARAMETER_1;

    /* Fill in PackageInfo. These are the Values that
     * Windows XP will report:
     * fCapabilities is 0x42B37, so we need to set (support!)
     * the following flags */
    PackageInfo->fCapabilities = 0x42b37;/*SECPKG_FLAG_INTEGRITY |
                                 SECPKG_FLAG_PRIVACY |
                                 SECPKG_FLAG_TOKEN_ONLY |
                                 SECPKG_FLAG_CONNECTION |
                                 SECPKG_FLAG_MULTI_REQUIRED |
                                 SECPKG_FLAG_IMPERSONATION |
                                 SECPKG_FLAG_ACCEPT_WIN32_NAME |
                                 SECPKG_FLAG_NEGOTIABLE |
                                 / * supports LsaLogonUser * /
                                 SECPKG_FLAG_LOGON |
                                 SECPKG_FLAG_READONLY_WITH_CHECKSUM;*/
    PackageInfo->wVersion = SECPKG_VERSION;
    PackageInfo->wRPCID = SECPKG_ID_NONE;
    PackageInfo->cbMaxToken = SECPKG_MAX_TOKEN_SIZE;
    PackageInfo->Name = L"NTLM";
    PackageInfo->Comment = L"NTLM Security Package (ReactOS)";

    fdTRACE("> PackageInfo\n");
    fdTRACE("> .fCapabilities 0x%x\n", PackageInfo->fCapabilities);
    fdTRACE("> .wVersion %i\n", PackageInfo->wVersion);
    fdTRACE("> .wRPCID %i\n", PackageInfo->wRPCID);
    fdTRACE("> .cbMaxToken %i\n", PackageInfo->cbMaxToken);
    fdTRACE("> .Name %S\n", PackageInfo->Name);
    fdTRACE("> .Comment %S\n", PackageInfo->Comment);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaSpInitLsaModeContext(
    LSA_SEC_HANDLE CredentialHandle,
    LSA_SEC_HANDLE ContextHandle,
    PUNICODE_STRING TargetName,
    ULONG ContextRequirements,
    ULONG TargetDataRep,
    PSecBufferDesc InputBuffers,
    PLSA_SEC_HANDLE NewContextHandle,
    PSecBufferDesc OutputBuffers,
    PULONG ContextAttributes,
    PTimeStamp ExpirationTime,
    PBOOLEAN MappedContext,
    PSecBuffer ContextData)
{
    NTSTATUS status;

    fdTRACE("LsaSpInitLsaModeContext(%p %p %p %p %i %i %p %p %p %p %p %p %p %p %p %p)\n",
          CredentialHandle, ContextHandle, TargetName,
          ContextRequirements, TargetDataRep, InputBuffers,
          NewContextHandle, OutputBuffers, ContextAttributes,
          ExpirationTime, MappedContext, ContextData);

    status = NtlmInitializeSecurityContext(
        CredentialHandle,
        ContextHandle,
        TargetName->Buffer,
        ContextRequirements,
        0,//IN ULONG Reserved1,
        TargetDataRep,
        InputBuffers,
        0,//IN ULONG Reserved2,
        NewContextHandle,
        OutputBuffers,
        ContextAttributes,
        ExpirationTime,
        MappedContext,
        ContextData);

    //NtlmPrintHexDump(OutputBuffers->pBuffers[2].pvBuffer, OutputBuffers->pBuffers[2].cbBuffer);

    fdTRACE("0x%x\n", status);

    return status;
}

NTSTATUS NTAPI
LsaSpAcceptLsaModeContext(
    LSA_SEC_HANDLE CredentialHandle,
    LSA_SEC_HANDLE ContextHandle,
    PSecBufferDesc InputBuffer,
    ULONG ContextRequirements,
    ULONG TargetDataRep,
    PLSA_SEC_HANDLE NewContextHandle,
    PSecBufferDesc OutputBuffer,
    PULONG ContextAttributes,
    PTimeStamp ExpirationTime,
    PBOOLEAN MappedContext,
    PSecBuffer ContextData)
{
    SECURITY_STATUS Status;
    fdTRACE("LsaSpAcceptLsaModeContext(%p %p %p %p %i %i %p %p %p %p %p)\n",
          CredentialHandle, ContextHandle, InputBuffer, ContextRequirements,
          TargetDataRep, NewContextHandle, OutputBuffer,
          ContextAttributes, ExpirationTime, MappedContext, ContextData);

    //FIXME: we mix SECURITY_STATUS / NTSTATUS
    Status = NtlmAcceptSecurityContext(
        CredentialHandle, ContextHandle, InputBuffer,
        ContextRequirements, TargetDataRep, NewContextHandle,
        OutputBuffer, ContextAttributes, ExpirationTime,
        MappedContext, ContextData);

    fdTRACE("0x%x\n", Status);

    return Status;
}

NTSTATUS NTAPI
LsaSpDeleteContext(
    LSA_SEC_HANDLE ContextHandle)
{
    fdTRACE("LsaSpDeleteContext(%p)\n", ContextHandle);

    /* windows doesnt validate the handle too ... should we? */
    NtlmDereferenceContext((PNTLMSSP_CONTEXT_HDR)ContextHandle);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaSpApplyControlToken(
    LSA_SEC_HANDLE p1,
    PSecBufferDesc p2)
{
    fdTRACE("*** UNIMPLEMENTED *** LsaSpApplyControlToken(%p %p)\n", p1, p2);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
LsaSpGetUserInfo(
    PLUID p1,
    ULONG p2,
    PSecurityUserData *p3)
{
    fdTRACE("*** UNIMPLEMENTED *** LsaSpGetUserInfo(%p %i %p)\n", p1, p2, p3);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
LsaSpGetExtendedInformation(
    _In_ SECPKG_EXTENDED_INFORMATION_CLASS Class,
    _Out_ PSECPKG_EXTENDED_INFORMATION *ppInfo)
{
    fdTRACE("LsaSpGetExtendedInformation(Class %p, Info %p)\n",
        Class, ppInfo);

    switch (Class)
    {
        case SecpkgContextThunks:
        {
            PSECPKG_EXTENDED_INFORMATION pInfo;
            pInfo = NtlmAllocate(sizeof(SECPKG_EXTENDED_INFORMATION), FALSE);
            if (pInfo == NULL)
                return STATUS_NO_MEMORY;

            pInfo->Class = SecpkgContextThunks;
            pInfo->Info.ContextThunks.InfoLevelCount = 1;
            pInfo->Info.ContextThunks.Levels[0] = 0x10;
            *ppInfo = pInfo;
            break;
        }
        /*case SecpkgMutualAuthLevel,
        {
            // Need a Test ...
            break;
        }*/
        case SecpkgWowClientDll:
        {
            PBYTE buffer;
            PSECPKG_EXTENDED_INFORMATION pInfo;

            // allocate memory at once: for struct and string
            // (WinXP does it in the same way 30(struct) + 204(wchar) bytes).
            buffer = NtlmAllocate(sizeof(SECPKG_EXTENDED_INFORMATION) +
                                  MAX_PATH_SHORT * sizeof(WCHAR), FALSE);
            if (buffer == NULL)
            {
                ERR("Failed to allocate memmory!\n");
                return STATUS_NO_MEMORY;
            }

            pInfo = (PSECPKG_EXTENDED_INFORMATION)buffer;
            pInfo->Class = SecpkgWowClientDll;
            pInfo->Info.WowClientDll.WowClientDllPath.MaximumLength = MAX_PATH_SHORT * sizeof(WCHAR);
            pInfo->Info.WowClientDll.WowClientDllPath.Buffer = (WCHAR*)(buffer + sizeof(SECPKG_EXTENDED_INFORMATION));

            // Use ExpandEnvironmentStringsW - WinXP does the same
            pInfo->Info.WowClientDll.WowClientDllPath.Length =
                ExpandEnvironmentStringsW(L"%systemroot%\\syswow64\\msv1_0.DLL",
                                          pInfo->Info.WowClientDll.WowClientDllPath.Buffer,
                                          MAX_PATH_SHORT) * sizeof(WCHAR);
            if (pInfo->Info.WowClientDll.WowClientDllPath.Length == 0)
            {
                ERR("ExpandEnvironmentStringsW failed!\n");
                return STATUS_INTERNAL_ERROR;
            }

            fdTRACE(" returning %.*S\n",
                pInfo->Info.WowClientDll.WowClientDllPath.Length / sizeof(WCHAR),
                pInfo->Info.WowClientDll.WowClientDllPath.Buffer);
            *ppInfo = pInfo;
            break;
        }
        default:
        {
            /* XP returns this code */
            fdTRACE("Class not supported\n");
            return SEC_E_UNSUPPORTED_FUNCTION;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaSpQueryContextAttributes(
    LSA_SEC_HANDLE p1,
    ULONG p2,
    PVOID p3)
{
    fdTRACE("*** UNIMPLEMENTED *** LsaSpQueryContextAttributes(%p %i %p)\n", p1, p2, p3);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
LsaSpAddCredentials(
    LSA_SEC_HANDLE p1,
    PUNICODE_STRING p2,
    PUNICODE_STRING p3,
    ULONG p4,
    PVOID p5,
    PVOID p6,
    PVOID p7,
    PTimeStamp p8)
{
    fdTRACE("*** UNIMPLEMENTED *** LsaSpAddCredentials(%p %p %p %i %p %p %p %p)\n",
          p1, p2, p3, p4, p4, p5, p6, p7, p8);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
LsaSpSetExtendedInformation(
    SECPKG_EXTENDED_INFORMATION_CLASS Class,
    PSECPKG_EXTENDED_INFORMATION pInfo)
{
    fdTRACE("LsaSpSetExtendedInformation(Class %p pInfo %p)\n",
        Class, pInfo);

    if (Class == SecpkgMutualAuthLevel)
    {
        /* Is this the Authentication level (LMCompatibilityLevel)
         * ??? */
        FIXME("Store auth level!!\n");
        return STATUS_SUCCESS;
    }

    return ERROR_NOT_SUPPORTED;
}
