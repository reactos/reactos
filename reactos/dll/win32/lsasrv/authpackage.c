/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/lsasrv/authpackage.c
 * PURPOSE:     Authenticaton package management routines
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "lsasrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(lsasrv);

typedef enum _LSA_TOKEN_INFORMATION_TYPE
{
    LsaTokenInformationNull,
    LsaTokenInformationV1
} LSA_TOKEN_INFORMATION_TYPE, *PLSA_TOKEN_INFORMATION_TYPE;

typedef PVOID PLSA_CLIENT_REQUEST;

typedef PVOID (NTAPI *PLSA_ALLOCATE_LSA_HEAP)(ULONG);
typedef VOID (NTAPI *PLSA_FREE_LSA_HEAP)(PVOID);
typedef NTSTATUS (NTAPI *PLSA_ALLOCATE_CLIENT_BUFFER)(PLSA_CLIENT_REQUEST, ULONG, PVOID*);
typedef NTSTATUS (NTAPI *PLSA_FREE_CLIENT_BUFFER)(PLSA_CLIENT_REQUEST, PVOID);

typedef struct LSA_DISPATCH_TABLE
{
    PVOID /*PLSA_CREATE_LOGON_SESSION */ CreateLogonSession;
    PVOID /*PLSA_DELETE_LOGON_SESSION */ DeleteLogonSession;
    PVOID /*PLSA_ADD_CREDENTIAL */ AddCredential;
    PVOID /*PLSA_GET_CREDENTIALS */ GetCredentials;
    PVOID /*PLSA_DELETE_CREDENTIAL */ DeleteCredential;
    PLSA_ALLOCATE_LSA_HEAP AllocateLsaHeap;
    PLSA_FREE_LSA_HEAP FreeLsaHeap;
    PLSA_ALLOCATE_CLIENT_BUFFER AllocateClientBuffer;
    PLSA_FREE_CLIENT_BUFFER FreeClientBuffer;
    PVOID /*PLSA_COPY_TO_CLIENT_BUFFER */ CopyToClientBuffer;
    PVOID /*PLSA_COPY_FROM_CLIENT_BUFFER */ CopyFromClientBuffer;
} LSA_DISPATCH_TABLE, *PLSA_DISPATCH_TABLE;


typedef NTSTATUS (NTAPI *PLSA_AP_INITIALIZE_PACKAGE)(ULONG, PLSA_DISPATCH_TABLE,
 PLSA_STRING, PLSA_STRING, PLSA_STRING *);
typedef NTSTATUS (NTAPI *PLSA_AP_CALL_PACKAGE_INTERNAL)(PLSA_CLIENT_REQUEST, PVOID, PVOID,
 ULONG, PVOID *, PULONG, PNTSTATUS);
typedef NTSTATUS (NTAPI *PLSA_AP_CALL_PACKAGE_PASSTHROUGH)(PLSA_CLIENT_REQUEST,
 PVOID, PVOID, ULONG, PVOID *, PULONG, PNTSTATUS);
typedef NTSTATUS (NTAPI *PLSA_AP_CALL_PACKAGE_UNTRUSTED)(PLSA_CLIENT_REQUEST,
 PVOID, PVOID, ULONG, PVOID *, PULONG, PNTSTATUS);
typedef VOID (NTAPI *PLSA_AP_LOGON_TERMINATED)(PLUID);
typedef NTSTATUS (NTAPI *PLSA_AP_LOGON_USER_EX2)(PLSA_CLIENT_REQUEST,
 SECURITY_LOGON_TYPE, PVOID, PVOID, ULONG, PVOID *, PULONG, PLUID, PNTSTATUS,
 PLSA_TOKEN_INFORMATION_TYPE, PVOID *, PUNICODE_STRING *, PUNICODE_STRING *,
 PUNICODE_STRING *, PVOID /*PSECPKG_PRIMARY_CRED*/, PVOID /*PSECPKG_SUPPLEMENTAL_CRED_ARRAY **/);
typedef NTSTATUS (NTAPI *PLSA_AP_LOGON_USER_EX)(PLSA_CLIENT_REQUEST,
 SECURITY_LOGON_TYPE, PVOID, PVOID, ULONG, PVOID *, PULONG, PLUID, PNTSTATUS,
 PLSA_TOKEN_INFORMATION_TYPE, PVOID *, PUNICODE_STRING *, PUNICODE_STRING *,
 PUNICODE_STRING *);

typedef NTSTATUS (NTAPI *PLSA_AP_LOGON_USER_INTERNAL)(PLSA_CLIENT_REQUEST, SECURITY_LOGON_TYPE,
 PVOID, PVOID, ULONG, PVOID *, PULONG, PLUID, PNTSTATUS, PLSA_TOKEN_INFORMATION_TYPE,
 PVOID *, PUNICODE_STRING *, PUNICODE_STRING *);

typedef struct _AUTH_PACKAGE
{
    LIST_ENTRY Entry;
    PSTRING Name;
    ULONG Id;
    PVOID ModuleHandle;

    PLSA_AP_INITIALIZE_PACKAGE LsaApInitializePackage;
    PLSA_AP_CALL_PACKAGE_INTERNAL LsaApCallPackage;
    PLSA_AP_CALL_PACKAGE_PASSTHROUGH LsaApCallPackagePassthrough;
    PLSA_AP_CALL_PACKAGE_UNTRUSTED LsaApCallPackageUntrusted;
    PLSA_AP_LOGON_TERMINATED LsaApLogonTerminated;
    PLSA_AP_LOGON_USER_EX2 LsaApLogonUserEx2;
    PLSA_AP_LOGON_USER_EX LsaApLogonUserEx;
    PLSA_AP_LOGON_USER_INTERNAL LsaApLogonUser;
} AUTH_PACKAGE, *PAUTH_PACKAGE;


/* GLOBALS *****************************************************************/

static LIST_ENTRY PackageListHead;
static ULONG PackageId;
static LSA_DISPATCH_TABLE DispatchTable;


/* FUNCTIONS ***************************************************************/

static
NTSTATUS
NTAPI
LsapAddAuthPackage(IN PWSTR ValueName,
                   IN ULONG ValueType,
                   IN PVOID ValueData,
                   IN ULONG ValueLength,
                   IN PVOID Context,
                   IN PVOID EntryContext)
{
    PAUTH_PACKAGE Package = NULL;
    UNICODE_STRING PackageName;
    STRING ProcName;
    PULONG Id;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("LsapAddAuthPackage()\n");

    PackageName.Length = (USHORT)ValueLength - sizeof(WCHAR);
    PackageName.MaximumLength = (USHORT)ValueLength;
    PackageName.Buffer = ValueData;

    Id = (PULONG)Context;

    Package = RtlAllocateHeap(RtlGetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  sizeof(AUTH_PACKAGE));
    if (Package == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = LdrLoadDll(NULL,
                        NULL,
                        &PackageName,
                        &Package->ModuleHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LdrLoadDll failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    RtlInitAnsiString(&ProcName, "LsaApInitializePackage");
    Status = LdrGetProcedureAddress(Package->ModuleHandle,
                                    &ProcName,
                                    0,
                                    (PVOID *)&Package->LsaApInitializePackage);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LdrGetProcedureAddress() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    RtlInitAnsiString(&ProcName, "LsaApCallPackage");
    Status = LdrGetProcedureAddress(Package->ModuleHandle,
                                    &ProcName,
                                    0,
                                    (PVOID *)&Package->LsaApCallPackage);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LdrGetProcedureAddress() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    RtlInitAnsiString(&ProcName, "LsaApCallPackagePassthrough");
    Status = LdrGetProcedureAddress(Package->ModuleHandle,
                                    &ProcName,
                                    0,
                                    (PVOID *)&Package->LsaApCallPackagePassthrough);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LdrGetProcedureAddress() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    RtlInitAnsiString(&ProcName, "LsaApCallPackageUntrusted");
    Status = LdrGetProcedureAddress(Package->ModuleHandle,
                                    &ProcName,
                                    0,
                                    (PVOID *)&Package->LsaApCallPackageUntrusted);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LdrGetProcedureAddress() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    RtlInitAnsiString(&ProcName, "LsaApLogonTerminated");
    Status = LdrGetProcedureAddress(Package->ModuleHandle,
                                    &ProcName,
                                    0,
                                    (PVOID *)&Package->LsaApLogonTerminated);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LdrGetProcedureAddress() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    RtlInitAnsiString(&ProcName, "LsaApLogonUserEx2");
    Status = LdrGetProcedureAddress(Package->ModuleHandle,
                                    &ProcName,
                                    0,
                                    (PVOID *)&Package->LsaApLogonUserEx2);
    if (!NT_SUCCESS(Status))
    {
        RtlInitAnsiString(&ProcName, "LsaApLogonUserEx");
        Status = LdrGetProcedureAddress(Package->ModuleHandle,
                                        &ProcName,
                                        0,
                                        (PVOID *)&Package->LsaApLogonUserEx);
        if (!NT_SUCCESS(Status))
        {
            RtlInitAnsiString(&ProcName, "LsaApLogonUser");
            Status = LdrGetProcedureAddress(Package->ModuleHandle,
                                            &ProcName,
                                            0,
                                            (PVOID *)&Package->LsaApLogonUser);
            if (!NT_SUCCESS(Status))
            {
                TRACE("LdrGetProcedureAddress() failed (Status 0x%08lx)\n", Status);
                goto done;
            }
        }
    }

    /* Initialize the current package */
    Status = Package->LsaApInitializePackage(*Id,
                                             &DispatchTable,
                                             NULL,
                                             NULL,
                                             &Package->Name);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Package->LsaApInitializePackage() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    TRACE("Package Name: %s\n", Package->Name->Buffer);

    Package->Id = *Id;
    *Id++;

    InsertTailList(&PackageListHead, &Package->Entry);

done:
    if (!NT_SUCCESS(Status))
    {
        if (Package != NULL)
        {
            if (Package->ModuleHandle != NULL)
                LdrUnloadDll(Package->ModuleHandle);

            if (Package->Name != NULL)
            {
                if (Package->Name->Buffer != NULL)
                    RtlFreeHeap(RtlGetProcessHeap(), 0, Package->Name->Buffer);

                RtlFreeHeap(RtlGetProcessHeap(), 0, Package->Name);
            }

            RtlFreeHeap(RtlGetProcessHeap(), 0, Package);
        }
    }

    return Status;
}


static
PAUTH_PACKAGE
LsapGetAuthenticationPackage(IN ULONG PackageId)
{
    PLIST_ENTRY ListEntry;
    PAUTH_PACKAGE Package;

    ListEntry = PackageListHead.Flink;
    while (ListEntry != &PackageListHead)
    {
        Package = CONTAINING_RECORD(ListEntry, AUTH_PACKAGE, Entry);

        if (Package->Id == PackageId)
        {
            return Package;
        }

        ListEntry = ListEntry->Flink;
    }

    return NULL;
}


static
PVOID
NTAPI
LsapAllocateHeap(IN ULONG Length)
{
    return RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           Length);
}


static
VOID
NTAPI
LsapFreeHeap(IN PVOID Base)
{
    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                Base);
}


static
NTSTATUS
NTAPI
LsapAllocateClientBuffer(IN PLSA_CLIENT_REQUEST ClientRequest,
                         IN ULONG LengthRequired,
                         OUT PVOID *ClientBaseAddress)
{
    FIXME("() stub\n");
    return STATUS_NOT_IMPLEMENTED;
}


static
NTSTATUS
NTAPI
LsapFreeClientBuffer(IN PLSA_CLIENT_REQUEST ClientRequest,
                     IN PVOID ClientBaseAddress)
{
    FIXME("() stub\n");
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
LsapInitAuthPackages(VOID)
{
    RTL_QUERY_REGISTRY_TABLE AuthPackageTable[] = {
    {LsapAddAuthPackage, 0, L"Authentication Packages", NULL, REG_NONE, NULL, 0},
    {NULL, 0, NULL, NULL, REG_NONE, NULL, 0}};

    NTSTATUS Status;

    InitializeListHead(&PackageListHead);
    PackageId = 0;

    /* Initialize the dispatch table */
    DispatchTable.CreateLogonSession = NULL;
    DispatchTable.DeleteLogonSession = NULL;
    DispatchTable.AddCredential = NULL;
    DispatchTable.GetCredentials = NULL;
    DispatchTable.DeleteCredential = NULL;
    DispatchTable.AllocateLsaHeap = &LsapAllocateHeap;
    DispatchTable.FreeLsaHeap = &LsapFreeHeap;
    DispatchTable.AllocateClientBuffer = &LsapAllocateClientBuffer;
    DispatchTable.FreeClientBuffer = &LsapFreeClientBuffer;
    DispatchTable.CopyToClientBuffer = NULL;
    DispatchTable.CopyFromClientBuffer = NULL;

    /* Add registered authentication packages */
    Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                                    L"Lsa",
                                    AuthPackageTable,
                                    &PackageId,
                                    NULL);


    return STATUS_SUCCESS;
}


NTSTATUS
LsapLookupAuthenticationPackage(PLSA_API_MSG RequestMsg,
                                PLSAP_LOGON_CONTEXT LogonContext)
{
    PLIST_ENTRY ListEntry;
    PAUTH_PACKAGE Package;
    ULONG PackageNameLength;
    PCHAR PackageName;

    TRACE("(%p %p)\n", RequestMsg, LogonContext);

    PackageNameLength = RequestMsg->LookupAuthenticationPackage.Request.PackageNameLength;
    PackageName = RequestMsg->LookupAuthenticationPackage.Request.PackageName;

    TRACE("PackageName: %s\n", PackageName);

    ListEntry = PackageListHead.Flink;
    while (ListEntry != &PackageListHead)
    {
        Package = CONTAINING_RECORD(ListEntry, AUTH_PACKAGE, Entry);

        if ((PackageNameLength == Package->Name->Length) &&
            (_strnicmp(PackageName, Package->Name->Buffer, Package->Name->Length) == 0))
        {
            RequestMsg->LookupAuthenticationPackage.Reply.Package = Package->Id;
            return STATUS_SUCCESS;
        }

        ListEntry = ListEntry->Flink;
    }

    return STATUS_NO_SUCH_PACKAGE;
}


NTSTATUS
LsapCallAuthenticationPackage(PLSA_API_MSG RequestMsg,
                              PLSAP_LOGON_CONTEXT LogonContext)
{
    PAUTH_PACKAGE Package;
    ULONG PackageId;

    NTSTATUS Status;

    TRACE("(%p %p)\n", RequestMsg, LogonContext);

    PackageId = RequestMsg->CallAuthenticationPackage.Request.AuthenticationPackage;

    Package = LsapGetAuthenticationPackage(PackageId);
    if (Package == NULL)
    {
        TRACE("LsapGetAuthenticationPackage() failed to find a package\n");
        return STATUS_NO_SUCH_PACKAGE;
    }

    Status = Package->LsaApCallPackage(NULL, /* FIXME: PLSA_CLIENT_REQUEST ClientRequest */
                                       RequestMsg->CallAuthenticationPackage.Request.ProtocolSubmitBuffer,
                                       NULL, /* FIXME: PVOID ClientBufferBase */
                                       RequestMsg->CallAuthenticationPackage.Request.SubmitBufferLength,
                                       &RequestMsg->CallAuthenticationPackage.Reply.ProtocolReturnBuffer,
                                       &RequestMsg->CallAuthenticationPackage.Reply.ReturnBufferLength,
                                       &RequestMsg->CallAuthenticationPackage.Reply.ProtocolStatus);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Package->LsaApCallPackage() failed (Status 0x%08lx)\n", Status);
    }

    return Status;
}


NTSTATUS
LsapLogonUser(PLSA_API_MSG RequestMsg,
              PLSAP_LOGON_CONTEXT LogonContext)
{
    PAUTH_PACKAGE Package;
    ULONG PackageId;
    NTSTATUS Status;

    LSA_TOKEN_INFORMATION_TYPE TokenInformationType;
    PVOID TokenInformation = NULL;
    PUNICODE_STRING AccountName = NULL;
    PUNICODE_STRING AuthenticatingAuthority = NULL;
    PUNICODE_STRING MachineName = NULL;

    TRACE("(%p %p)\n", RequestMsg, LogonContext);

    PackageId = RequestMsg->LogonUser.Request.AuthenticationPackage;

    Package = LsapGetAuthenticationPackage(PackageId);
    if (Package == NULL)
    {
        TRACE("LsapGetAuthenticationPackage() failed to find a package\n");
        return STATUS_NO_SUCH_PACKAGE;
    }

    if (Package->LsaApLogonUserEx2 != NULL)
    {
        Status = Package->LsaApLogonUserEx2(NULL,  /* FIXME: PLSA_CLIENT_REQUEST ClientRequest */
                                            RequestMsg->LogonUser.Request.LogonType,
                                            RequestMsg->LogonUser.Request.AuthenticationInformation,
                                            NULL,  /* FIXME: PVOID ClientBufferBase*/
                                            RequestMsg->LogonUser.Request.AuthenticationInformationLength,
                                            &RequestMsg->LogonUser.Reply.ProfileBuffer,
                                            &RequestMsg->LogonUser.Reply.ProfileBufferLength,
                                            &RequestMsg->LogonUser.Reply.LogonId,
                                            &RequestMsg->LogonUser.Reply.SubStatus,
                                            &TokenInformationType,
                                            &TokenInformation,
                                            &AccountName,
                                            &AuthenticatingAuthority,
                                            &MachineName,
                                            NULL,  /* FIXME: PSECPKG_PRIMARY_CRED PrimaryCredentials */
                                            NULL); /* FIXME: PSECPKG_SUPPLEMENTAL_CRED_ARRAY *SupplementalCredentials */
    }
    else if (Package->LsaApLogonUserEx != NULL)
    {
        Status = Package->LsaApLogonUserEx(NULL,  /* FIXME: PLSA_CLIENT_REQUEST ClientRequest */
                                           RequestMsg->LogonUser.Request.LogonType,
                                           RequestMsg->LogonUser.Request.AuthenticationInformation,
                                           NULL,  /* FIXME: PVOID ClientBufferBase*/
                                           RequestMsg->LogonUser.Request.AuthenticationInformationLength,
                                           &RequestMsg->LogonUser.Reply.ProfileBuffer,
                                           &RequestMsg->LogonUser.Reply.ProfileBufferLength,
                                           &RequestMsg->LogonUser.Reply.LogonId,
                                           &RequestMsg->LogonUser.Reply.SubStatus,
                                           &TokenInformationType,
                                           &TokenInformation,
                                           &AccountName,
                                           &AuthenticatingAuthority,
                                           &MachineName);
    }
    else
    {
        Status = Package->LsaApLogonUser(NULL,  /* FIXME: PLSA_CLIENT_REQUEST ClientRequest */
                                         RequestMsg->LogonUser.Request.LogonType,
                                         RequestMsg->LogonUser.Request.AuthenticationInformation,
                                         NULL,  /* FIXME:  PVOID ClientBufferBase*/
                                         RequestMsg->LogonUser.Request.AuthenticationInformationLength,
                                         &RequestMsg->LogonUser.Reply.ProfileBuffer,
                                         &RequestMsg->LogonUser.Reply.ProfileBufferLength,
                                         &RequestMsg->LogonUser.Reply.LogonId,
                                         &RequestMsg->LogonUser.Reply.SubStatus,
                                         &TokenInformationType,
                                         &TokenInformation,
                                         &AccountName,
                                         &AuthenticatingAuthority);
    }


    if (TokenInformation != NULL)
    {

    }

    if (AuthenticatingAuthority != NULL)
    {

    }

    if (AccountName != NULL)
    {

    }

    if (MachineName != NULL)
    {

    }

    return Status;
}

/* EOF */
