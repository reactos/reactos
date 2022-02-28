/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/lsasrv/authpackage.c
 * PURPOSE:     Authentication package management routines
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

#include "lsasrv.h"

#include <ndk/sefuncs.h>
#include <ndk/umfuncs.h>

typedef enum _LSA_TOKEN_INFORMATION_TYPE
{
    LsaTokenInformationNull,
    LsaTokenInformationV1
} LSA_TOKEN_INFORMATION_TYPE, *PLSA_TOKEN_INFORMATION_TYPE;

typedef struct _LSA_TOKEN_INFORMATION_NULL
{
    LARGE_INTEGER ExpirationTime;
    PTOKEN_GROUPS Groups;
} LSA_TOKEN_INFORMATION_NULL, *PLSA_TOKEN_INFORMATION_NULL;

typedef struct _LSA_TOKEN_INFORMATION_V1
{
    LARGE_INTEGER ExpirationTime;
    TOKEN_USER User;
    PTOKEN_GROUPS Groups;
    TOKEN_PRIMARY_GROUP PrimaryGroup;
    PTOKEN_PRIVILEGES Privileges;
    TOKEN_OWNER Owner;
    TOKEN_DEFAULT_DACL DefaultDacl;
} LSA_TOKEN_INFORMATION_V1, *PLSA_TOKEN_INFORMATION_V1;

typedef PVOID PLSA_CLIENT_REQUEST;

typedef NTSTATUS (NTAPI *PLSA_CREATE_LOGON_SESSION)(PLUID);
typedef NTSTATUS (NTAPI *PLSA_DELETE_LOGON_SESSION)(PLUID);
typedef NTSTATUS (NTAPI *PLSA_ADD_CREDENTIAL)(PLUID, ULONG, PLSA_STRING, PLSA_STRING);
typedef NTSTATUS (NTAPI *PLSA_GET_CREDENTIALS)(PLUID, ULONG, PULONG, BOOLEAN, PLSA_STRING, PULONG, PLSA_STRING);
typedef NTSTATUS (NTAPI *PLSA_DELETE_CREDENTIAL)(PLUID, ULONG, PLSA_STRING);
typedef PVOID (NTAPI *PLSA_ALLOCATE_LSA_HEAP)(ULONG);
typedef VOID (NTAPI *PLSA_FREE_LSA_HEAP)(PVOID);
typedef NTSTATUS (NTAPI *PLSA_ALLOCATE_CLIENT_BUFFER)(PLSA_CLIENT_REQUEST, ULONG, PVOID*);
typedef NTSTATUS (NTAPI *PLSA_FREE_CLIENT_BUFFER)(PLSA_CLIENT_REQUEST, PVOID);
typedef NTSTATUS (NTAPI *PLSA_COPY_TO_CLIENT_BUFFER)(PLSA_CLIENT_REQUEST, ULONG,
 PVOID, PVOID);
typedef NTSTATUS (NTAPI *PLSA_COPY_FROM_CLIENT_BUFFER)(PLSA_CLIENT_REQUEST,
 ULONG, PVOID, PVOID);

typedef struct LSA_DISPATCH_TABLE
{
    PLSA_CREATE_LOGON_SESSION CreateLogonSession;
    PLSA_DELETE_LOGON_SESSION DeleteLogonSession;
    PLSA_ADD_CREDENTIAL AddCredential;
    PLSA_GET_CREDENTIALS GetCredentials;
    PLSA_DELETE_CREDENTIAL DeleteCredential;
    PLSA_ALLOCATE_LSA_HEAP AllocateLsaHeap;
    PLSA_FREE_LSA_HEAP FreeLsaHeap;
    PLSA_ALLOCATE_CLIENT_BUFFER AllocateClientBuffer;
    PLSA_FREE_CLIENT_BUFFER FreeClientBuffer;
    PLSA_COPY_TO_CLIENT_BUFFER CopyToClientBuffer;
    PLSA_COPY_FROM_CLIENT_BUFFER CopyFromClientBuffer;
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

VOID
NTAPI
LsaIFree_LSAPR_PRIVILEGE_SET(IN PLSAPR_PRIVILEGE_SET Ptr);

typedef wchar_t *PSAMPR_SERVER_NAME;
typedef void *SAMPR_HANDLE;

typedef struct _SAMPR_SID_INFORMATION
{
    PRPC_SID SidPointer;
} SAMPR_SID_INFORMATION, *PSAMPR_SID_INFORMATION;

typedef struct _SAMPR_PSID_ARRAY
{
    unsigned long Count;
    PSAMPR_SID_INFORMATION Sids;
} SAMPR_PSID_ARRAY, *PSAMPR_PSID_ARRAY;

NTSTATUS
NTAPI
SamIConnect(
    PSAMPR_SERVER_NAME ServerName,
    SAMPR_HANDLE *ServerHandle,
    ACCESS_MASK DesiredAccess,
    BOOLEAN Trusted);

VOID
NTAPI
SamIFree_SAMPR_ULONG_ARRAY(
    PSAMPR_ULONG_ARRAY Ptr);

NTSTATUS
__stdcall
SamrCloseHandle(
    SAMPR_HANDLE *SamHandle);

NTSTATUS
__stdcall
SamrOpenDomain(
    SAMPR_HANDLE ServerHandle,
    ACCESS_MASK DesiredAccess,
    PRPC_SID DomainId,
    SAMPR_HANDLE *DomainHandle);

NTSTATUS
__stdcall
SamrGetAliasMembership(
    SAMPR_HANDLE DomainHandle,
    PSAMPR_PSID_ARRAY SidArray,
    PSAMPR_ULONG_ARRAY Membership);


/* GLOBALS *****************************************************************/

static LIST_ENTRY PackageListHead;
static ULONG PackageId;
static LSA_DISPATCH_TABLE DispatchTable;

#define CONST_LUID(x1, x2) {x1, x2}
static const LUID SeChangeNotifyPrivilege = CONST_LUID(SE_CHANGE_NOTIFY_PRIVILEGE, 0);
static const LUID SeCreateGlobalPrivilege = CONST_LUID(SE_CREATE_GLOBAL_PRIVILEGE, 0);
static const LUID SeImpersonatePrivilege = CONST_LUID(SE_IMPERSONATE_PRIVILEGE, 0);


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
    (*Id)++;

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


PVOID
NTAPI
LsapAllocateHeap(IN ULONG Length)
{
    return RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
}


PVOID
NTAPI
LsapAllocateHeapZero(IN ULONG Length)
{
    return RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, Length);
}


VOID
NTAPI
LsapFreeHeap(IN PVOID Base)
{
    RtlFreeHeap(RtlGetProcessHeap(), 0, Base);
}


static
NTSTATUS
NTAPI
LsapAllocateClientBuffer(IN PLSA_CLIENT_REQUEST ClientRequest,
                         IN ULONG LengthRequired,
                         OUT PVOID *ClientBaseAddress)
{
    PLSAP_LOGON_CONTEXT LogonContext;
    SIZE_T Length;

    *ClientBaseAddress = NULL;

    LogonContext = (PLSAP_LOGON_CONTEXT)ClientRequest;

    Length = LengthRequired;
    return NtAllocateVirtualMemory(LogonContext->ClientProcessHandle,
                                   ClientBaseAddress,
                                   0,
                                   &Length,
                                   MEM_COMMIT,
                                   PAGE_READWRITE);
}


static
NTSTATUS
NTAPI
LsapFreeClientBuffer(IN PLSA_CLIENT_REQUEST ClientRequest,
                     IN PVOID ClientBaseAddress)
{
    PLSAP_LOGON_CONTEXT LogonContext;
    SIZE_T Length;

    if (ClientBaseAddress == NULL)
        return STATUS_SUCCESS;

    LogonContext = (PLSAP_LOGON_CONTEXT)ClientRequest;

    Length = 0;
    return NtFreeVirtualMemory(LogonContext->ClientProcessHandle,
                               &ClientBaseAddress,
                               &Length,
                               MEM_RELEASE);
}


static
NTSTATUS
NTAPI
LsapCopyToClientBuffer(IN PLSA_CLIENT_REQUEST ClientRequest,
                       IN ULONG Length,
                       IN PVOID ClientBaseAddress,
                       IN PVOID BufferToCopy)
{
    PLSAP_LOGON_CONTEXT LogonContext;

    LogonContext = (PLSAP_LOGON_CONTEXT)ClientRequest;

    return NtWriteVirtualMemory(LogonContext->ClientProcessHandle,
                                ClientBaseAddress,
                                BufferToCopy,
                                Length,
                                NULL);
}


static
NTSTATUS
NTAPI
LsapCopyFromClientBuffer(IN PLSA_CLIENT_REQUEST ClientRequest,
                         IN ULONG Length,
                         IN PVOID BufferToCopy,
                         IN PVOID ClientBaseAddress)
{
    PLSAP_LOGON_CONTEXT LogonContext;

    LogonContext = (PLSAP_LOGON_CONTEXT)ClientRequest;

    return NtReadVirtualMemory(LogonContext->ClientProcessHandle,
                               ClientBaseAddress,
                               BufferToCopy,
                               Length,
                               NULL);
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
    DispatchTable.CreateLogonSession = &LsapCreateLogonSession;
    DispatchTable.DeleteLogonSession = &LsapDeleteLogonSession;
    DispatchTable.AddCredential = &LsapAddCredential;
    DispatchTable.GetCredentials = &LsapGetCredentials;
    DispatchTable.DeleteCredential = &LsapDeleteCredential;
    DispatchTable.AllocateLsaHeap = &LsapAllocateHeapZero;
    DispatchTable.FreeLsaHeap = &LsapFreeHeap;
    DispatchTable.AllocateClientBuffer = &LsapAllocateClientBuffer;
    DispatchTable.FreeClientBuffer = &LsapFreeClientBuffer;
    DispatchTable.CopyToClientBuffer = &LsapCopyToClientBuffer;
    DispatchTable.CopyFromClientBuffer = &LsapCopyFromClientBuffer;

    /* Add registered authentication packages */
    Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                                    L"Lsa",
                                    AuthPackageTable,
                                    &PackageId,
                                    NULL);

    return Status;
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


VOID
LsapTerminateLogon(
    _In_ PLUID LogonId)
{
    PLIST_ENTRY ListEntry;
    PAUTH_PACKAGE Package;

    ListEntry = PackageListHead.Flink;
    while (ListEntry != &PackageListHead)
    {
        Package = CONTAINING_RECORD(ListEntry, AUTH_PACKAGE, Entry);

        Package->LsaApLogonTerminated(LogonId);

        ListEntry = ListEntry->Flink;
    }
}


NTSTATUS
LsapCallAuthenticationPackage(PLSA_API_MSG RequestMsg,
                              PLSAP_LOGON_CONTEXT LogonContext)
{
    PAUTH_PACKAGE Package;
    PVOID LocalBuffer = NULL;
    ULONG PackageId;
    NTSTATUS Status;

    TRACE("(%p %p)\n", RequestMsg, LogonContext);

    PackageId = RequestMsg->CallAuthenticationPackage.Request.AuthenticationPackage;

    /* Get the right authentication package */
    Package = LsapGetAuthenticationPackage(PackageId);
    if (Package == NULL)
    {
        TRACE("LsapGetAuthenticationPackage() failed to find a package\n");
        return STATUS_NO_SUCH_PACKAGE;
    }

    if (RequestMsg->CallAuthenticationPackage.Request.SubmitBufferLength > 0)
    {
        LocalBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      RequestMsg->CallAuthenticationPackage.Request.SubmitBufferLength);
        if (LocalBuffer == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Status = NtReadVirtualMemory(LogonContext->ClientProcessHandle,
                                     RequestMsg->CallAuthenticationPackage.Request.ProtocolSubmitBuffer,
                                     LocalBuffer,
                                     RequestMsg->CallAuthenticationPackage.Request.SubmitBufferLength,
                                     NULL);
        if (!NT_SUCCESS(Status))
        {
            TRACE("NtReadVirtualMemory() failed (Status 0x%08lx)\n", Status);
            RtlFreeHeap(RtlGetProcessHeap(), 0, LocalBuffer);
            return Status;
        }
    }

    if (LogonContext->TrustedCaller)
        Status = Package->LsaApCallPackage((PLSA_CLIENT_REQUEST)LogonContext,
                                           LocalBuffer,
                                           RequestMsg->CallAuthenticationPackage.Request.ProtocolSubmitBuffer,
                                           RequestMsg->CallAuthenticationPackage.Request.SubmitBufferLength,
                                           &RequestMsg->CallAuthenticationPackage.Reply.ProtocolReturnBuffer,
                                           &RequestMsg->CallAuthenticationPackage.Reply.ReturnBufferLength,
                                           &RequestMsg->CallAuthenticationPackage.Reply.ProtocolStatus);
    else
        Status = Package->LsaApCallPackageUntrusted((PLSA_CLIENT_REQUEST)LogonContext,
                                                    LocalBuffer,
                                                    RequestMsg->CallAuthenticationPackage.Request.ProtocolSubmitBuffer,
                                                    RequestMsg->CallAuthenticationPackage.Request.SubmitBufferLength,
                                                    &RequestMsg->CallAuthenticationPackage.Reply.ProtocolReturnBuffer,
                                                    &RequestMsg->CallAuthenticationPackage.Reply.ReturnBufferLength,
                                                    &RequestMsg->CallAuthenticationPackage.Reply.ProtocolStatus);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Package->LsaApCallPackage() failed (Status 0x%08lx)\n", Status);
    }

    if (LocalBuffer != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, LocalBuffer);

    return Status;
}


static
NTSTATUS
LsapCopyLocalGroups(
    IN PLSAP_LOGON_CONTEXT LogonContext,
    IN PTOKEN_GROUPS ClientGroups,
    IN ULONG ClientGroupsCount,
    OUT PTOKEN_GROUPS *TokenGroups)
{
    ULONG LocalGroupsLength = 0;
    PTOKEN_GROUPS LocalGroups = NULL;
    ULONG SidHeaderLength = 0;
    PSID SidHeader = NULL;
    PSID SrcSid, DstSid;
    ULONG SidLength;
    ULONG AllocatedSids = 0;
    ULONG i;
    NTSTATUS Status;

    LocalGroupsLength = sizeof(TOKEN_GROUPS) +
                        (ClientGroupsCount - ANYSIZE_ARRAY) * sizeof(SID_AND_ATTRIBUTES);
    LocalGroups = RtlAllocateHeap(RtlGetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  LocalGroupsLength);
    if (LocalGroups == NULL)
    {
        TRACE("RtlAllocateHeap() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = NtReadVirtualMemory(LogonContext->ClientProcessHandle,
                                 ClientGroups,
                                 LocalGroups,
                                 LocalGroupsLength,
                                 NULL);
    if (!NT_SUCCESS(Status))
        goto done;


    SidHeaderLength  = RtlLengthRequiredSid(0);
    SidHeader = RtlAllocateHeap(RtlGetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                SidHeaderLength);
    if (SidHeader == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    for (i = 0; i < ClientGroupsCount; i++)
    {
        SrcSid = LocalGroups->Groups[i].Sid;

        Status = NtReadVirtualMemory(LogonContext->ClientProcessHandle,
                                     SrcSid,
                                     SidHeader,
                                     SidHeaderLength,
                                     NULL);
        if (!NT_SUCCESS(Status))
            goto done;

        SidLength = RtlLengthSid(SidHeader);
        TRACE("Sid %lu: Length %lu\n", i, SidLength);

        DstSid = RtlAllocateHeap(RtlGetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 SidLength);
        if (DstSid == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        Status = NtReadVirtualMemory(LogonContext->ClientProcessHandle,
                                     SrcSid,
                                     DstSid,
                                     SidLength,
                                     NULL);
        if (!NT_SUCCESS(Status))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, DstSid);
            goto done;
        }

        LocalGroups->Groups[i].Sid = DstSid;
        AllocatedSids++;
    }

    *TokenGroups = LocalGroups;

done:
    if (SidHeader != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, SidHeader);

    if (!NT_SUCCESS(Status))
    {
        if (LocalGroups != NULL)
        {
            for (i = 0; i < AllocatedSids; i++)
                RtlFreeHeap(RtlGetProcessHeap(), 0, LocalGroups->Groups[i].Sid);

            RtlFreeHeap(RtlGetProcessHeap(), 0, LocalGroups);
        }
    }

    return Status;
}


static
NTSTATUS
LsapAddLocalGroups(
    IN PVOID TokenInformation,
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN PTOKEN_GROUPS LocalGroups)
{
    PLSA_TOKEN_INFORMATION_V1 TokenInfo1;
    PTOKEN_GROUPS Groups;
    ULONG Length;
    ULONG i;
    ULONG j;

    if (LocalGroups == NULL || LocalGroups->GroupCount == 0)
        return STATUS_SUCCESS;

    if (TokenInformationType == LsaTokenInformationV1)
    {
        TokenInfo1 = (PLSA_TOKEN_INFORMATION_V1)TokenInformation;

        if (TokenInfo1->Groups != NULL)
        {
            Length = sizeof(TOKEN_GROUPS) +
                     (LocalGroups->GroupCount + TokenInfo1->Groups->GroupCount - ANYSIZE_ARRAY) * sizeof(SID_AND_ATTRIBUTES);

            Groups = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, Length);
            if (Groups == NULL)
            {
                ERR("Group buffer allocation failed!\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            Groups->GroupCount = LocalGroups->GroupCount + TokenInfo1->Groups->GroupCount;

            for (i = 0; i < TokenInfo1->Groups->GroupCount; i++)
            {
                Groups->Groups[i].Sid = TokenInfo1->Groups->Groups[i].Sid;
                Groups->Groups[i].Attributes = TokenInfo1->Groups->Groups[i].Attributes;
            }

            for (j = 0; j < LocalGroups->GroupCount; i++, j++)
            {
                Groups->Groups[i].Sid = LocalGroups->Groups[j].Sid;
                Groups->Groups[i].Attributes = LocalGroups->Groups[j].Attributes;
                LocalGroups->Groups[j].Sid = NULL;
            }

            RtlFreeHeap(RtlGetProcessHeap(), 0, TokenInfo1->Groups);

            TokenInfo1->Groups = Groups;
        }
        else
        {
            Length = sizeof(TOKEN_GROUPS) +
                     (LocalGroups->GroupCount - ANYSIZE_ARRAY) * sizeof(SID_AND_ATTRIBUTES);

            Groups = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, Length);
            if (Groups == NULL)
            {
                ERR("Group buffer allocation failed!\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            Groups->GroupCount = LocalGroups->GroupCount;

            for (i = 0; i < LocalGroups->GroupCount; i++)
            {
                Groups->Groups[i].Sid = LocalGroups->Groups[i].Sid;
                Groups->Groups[i].Attributes = LocalGroups->Groups[i].Attributes;
            }

            TokenInfo1->Groups = Groups;
        }
    }
    else
    {
        FIXME("TokenInformationType %d is not supported!\n", TokenInformationType);
        return STATUS_NOT_IMPLEMENTED;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
LsapAddDefaultGroups(
    IN PVOID TokenInformation,
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN SECURITY_LOGON_TYPE LogonType)
{
    PLSA_TOKEN_INFORMATION_V1 TokenInfo1;
    PTOKEN_GROUPS Groups;
    ULONG i, Length;
    PSID SrcSid;

    if (TokenInformationType == LsaTokenInformationV1)
    {
        TokenInfo1 = (PLSA_TOKEN_INFORMATION_V1)TokenInformation;

        if (TokenInfo1->Groups != NULL)
        {
            Length = sizeof(TOKEN_GROUPS) +
                     (TokenInfo1->Groups->GroupCount + 2 - ANYSIZE_ARRAY) * sizeof(SID_AND_ATTRIBUTES);

            Groups = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, Length);
            if (Groups == NULL)
            {
                ERR("Group buffer allocation failed!\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            Groups->GroupCount = TokenInfo1->Groups->GroupCount;

            for (i = 0; i < TokenInfo1->Groups->GroupCount; i++)
            {
                Groups->Groups[i].Sid = TokenInfo1->Groups->Groups[i].Sid;
                Groups->Groups[i].Attributes = TokenInfo1->Groups->Groups[i].Attributes;
            }

            RtlFreeHeap(RtlGetProcessHeap(), 0, TokenInfo1->Groups);

            TokenInfo1->Groups = Groups;

        }
        else
        {
            Length = sizeof(TOKEN_GROUPS) +
                     (2 - ANYSIZE_ARRAY) * sizeof(SID_AND_ATTRIBUTES);

            Groups = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, Length);
            if (Groups == NULL)
            {
                ERR("Group buffer allocation failed!\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            TokenInfo1->Groups = Groups;
        }

        /* Append the World SID (aka Everyone) */
        Length = RtlLengthSid(LsapWorldSid);
        Groups->Groups[Groups->GroupCount].Sid = RtlAllocateHeap(RtlGetProcessHeap(),
                                                                 HEAP_ZERO_MEMORY,
                                                                 Length);
        if (Groups->Groups[Groups->GroupCount].Sid == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        RtlCopyMemory(Groups->Groups[Groups->GroupCount].Sid,
                      LsapWorldSid,
                      Length);

        Groups->Groups[Groups->GroupCount].Attributes =
            SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY;

        Groups->GroupCount++;

        /* Append the logon type SID */
        switch (LogonType)
        {
            case Interactive:
                SrcSid = LsapInteractiveSid;
                break;

            case Network:
                SrcSid = LsapNetworkSid;
                break;

            case Batch:
                SrcSid = LsapBatchSid;
                break;

            case Service:
                SrcSid = LsapServiceSid;
                break;

            default:
                FIXME("LogonType %d is not supported!\n", LogonType);
                return STATUS_NOT_IMPLEMENTED;
        }

        Length = RtlLengthSid(SrcSid);
        Groups->Groups[Groups->GroupCount].Sid = RtlAllocateHeap(RtlGetProcessHeap(),
                                                                 HEAP_ZERO_MEMORY,
                                                                 Length);
        if (Groups->Groups[Groups->GroupCount].Sid == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        RtlCopyMemory(Groups->Groups[Groups->GroupCount].Sid,
                      SrcSid,
                      Length);

        Groups->Groups[Groups->GroupCount].Attributes =
            SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY;

        Groups->GroupCount++;
    }
    else
    {
        FIXME("TokenInformationType %d is not supported!\n", TokenInformationType);
        return STATUS_NOT_IMPLEMENTED;
    }

    return STATUS_SUCCESS;
}


static
NTSTATUS
LsapAppendSidToGroups(
    IN PTOKEN_GROUPS *TokenGroups,
    IN PSID DomainSid,
    IN ULONG RelativeId)
{
    PTOKEN_GROUPS Groups;
    PSID Sid;
    ULONG Length;
    ULONG i;

    Sid = LsapAppendRidToSid(DomainSid, RelativeId);
    if (Sid == NULL)
    {
        ERR("Group SID creation failed!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (*TokenGroups == NULL)
    {
        Length = sizeof(TOKEN_GROUPS) +
                 (1 - ANYSIZE_ARRAY) * sizeof(SID_AND_ATTRIBUTES);

        Groups = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, Length);
        if (Groups == NULL)
        {
            ERR("Group buffer allocation failed!\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Groups->GroupCount = 1;

        Groups->Groups[0].Sid = Sid;
        Groups->Groups[0].Attributes =
            SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY;

        *TokenGroups = Groups;
    }
    else
    {
        for (i = 0; i < (*TokenGroups)->GroupCount; i++)
        {
            if (RtlEqualSid((*TokenGroups)->Groups[i].Sid, Sid))
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, Sid);
                return STATUS_SUCCESS;
            }
        }

        Length = sizeof(TOKEN_GROUPS) +
                 ((*TokenGroups)->GroupCount + 1 - ANYSIZE_ARRAY) * sizeof(SID_AND_ATTRIBUTES);

        Groups = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, Length);
        if (Groups == NULL)
        {
            ERR("Group buffer allocation failed!\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Groups->GroupCount = (*TokenGroups)->GroupCount;

        for (i = 0; i < (*TokenGroups)->GroupCount; i++)
        {
            Groups->Groups[i].Sid = (*TokenGroups)->Groups[i].Sid;
            Groups->Groups[i].Attributes = (*TokenGroups)->Groups[i].Attributes;
        }

        Groups->Groups[Groups->GroupCount].Sid = Sid;
        Groups->Groups[Groups->GroupCount].Attributes =
            SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY;

        Groups->GroupCount++;

        RtlFreeHeap(RtlGetProcessHeap(), 0, *TokenGroups);

        *TokenGroups = Groups;
    }

    return STATUS_SUCCESS;
}


static
NTSTATUS
LsapAddSamGroups(
    IN PVOID TokenInformation,
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType)
{
    PLSA_TOKEN_INFORMATION_V1 TokenInfo1;
    SAMPR_HANDLE ServerHandle = NULL;
    SAMPR_HANDLE BuiltinDomainHandle = NULL;
    SAMPR_HANDLE AccountDomainHandle = NULL;
    SAMPR_PSID_ARRAY SidArray;
    SAMPR_ULONG_ARRAY BuiltinMembership;
    SAMPR_ULONG_ARRAY AccountMembership;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    if (TokenInformationType != LsaTokenInformationV1)
        return STATUS_SUCCESS;

    TokenInfo1 = (PLSA_TOKEN_INFORMATION_V1)TokenInformation;

    SidArray.Count = TokenInfo1->Groups->GroupCount + 1;
    SidArray.Sids = RtlAllocateHeap(RtlGetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    (TokenInfo1->Groups->GroupCount + 1) * sizeof(PRPC_SID));
    if (SidArray.Sids == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    SidArray.Sids[0].SidPointer = TokenInfo1->User.User.Sid;
    for (i = 0; i < TokenInfo1->Groups->GroupCount; i++)
        SidArray.Sids[i + 1].SidPointer = TokenInfo1->Groups->Groups[i].Sid;

    BuiltinMembership.Element = NULL;
    AccountMembership.Element = NULL;

    Status = SamIConnect(NULL,
                         &ServerHandle,
                         SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                         FALSE);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamIConnect failed (Status %08lx)\n", Status);
        goto done;
    }

    Status = SamrOpenDomain(ServerHandle,
                            DOMAIN_GET_ALIAS_MEMBERSHIP,
                            BuiltinDomainSid,
                            &BuiltinDomainHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrOpenDomain failed (Status %08lx)\n", Status);
        goto done;
    }

    Status = SamrOpenDomain(ServerHandle,
                            DOMAIN_GET_ALIAS_MEMBERSHIP,
                            AccountDomainSid,
                            &AccountDomainHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrOpenDomain failed (Status %08lx)\n", Status);
        goto done;
    }

    Status = SamrGetAliasMembership(BuiltinDomainHandle,
                                    &SidArray,
                                    &BuiltinMembership);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrGetAliasMembership failed (Status %08lx)\n", Status);
        goto done;
    }

    Status = SamrGetAliasMembership(AccountDomainHandle,
                                    &SidArray,
                                    &AccountMembership);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrGetAliasMembership failed (Status %08lx)\n", Status);
        goto done;
    }

    TRACE("Builtin Memberships: %lu\n", BuiltinMembership.Count);
    for (i = 0; i < BuiltinMembership.Count; i++)
    {
        TRACE("RID %lu: %lu (0x%lx)\n", i, BuiltinMembership.Element[i], BuiltinMembership.Element[i]);
        Status = LsapAppendSidToGroups(&TokenInfo1->Groups,
                                       BuiltinDomainSid,
                                       BuiltinMembership.Element[i]);
        if (!NT_SUCCESS(Status))
        {
            TRACE("LsapAppendSidToGroups failed (Status %08lx)\n", Status);
            goto done;
        }
    }

    TRACE("Account Memberships: %lu\n", AccountMembership.Count);
    for (i = 0; i < AccountMembership.Count; i++)
    {
        TRACE("RID %lu: %lu (0x%lx)\n", i, AccountMembership.Element[i], AccountMembership.Element[i]);
        Status = LsapAppendSidToGroups(&TokenInfo1->Groups,
                                       AccountDomainSid,
                                       AccountMembership.Element[i]);
        if (!NT_SUCCESS(Status))
        {
            TRACE("LsapAppendSidToGroups failed (Status %08lx)\n", Status);
            goto done;
        }
    }

done:
    RtlFreeHeap(RtlGetProcessHeap(), 0, SidArray.Sids);

    if (AccountMembership.Element != NULL)
        SamIFree_SAMPR_ULONG_ARRAY(&AccountMembership);

    if (BuiltinMembership.Element != NULL)
        SamIFree_SAMPR_ULONG_ARRAY(&BuiltinMembership);

    if (AccountDomainHandle != NULL)
        SamrCloseHandle(&AccountDomainHandle);

    if (BuiltinDomainHandle != NULL)
        SamrCloseHandle(&BuiltinDomainHandle);

    if (ServerHandle != NULL)
        SamrCloseHandle(&ServerHandle);

//    return Status;

    return STATUS_SUCCESS;
}


static
NTSTATUS
LsapSetTokenOwner(
    IN PVOID TokenInformation,
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType)
{
    PLSA_TOKEN_INFORMATION_V1 TokenInfo1;
    PSID_AND_ATTRIBUTES OwnerSid = NULL;
    ULONG i, Length;

    if (TokenInformationType == LsaTokenInformationV1)
    {
        TokenInfo1 = (PLSA_TOKEN_INFORMATION_V1)TokenInformation;

        if (TokenInfo1->Owner.Owner != NULL)
            return STATUS_SUCCESS;

        OwnerSid = &TokenInfo1->User.User;
        for (i = 0; i < TokenInfo1->Groups->GroupCount; i++)
        {
            if (EqualSid(TokenInfo1->Groups->Groups[i].Sid, LsapAdministratorsSid))
            {
                OwnerSid = &TokenInfo1->Groups->Groups[i];
                break;
            }
        }

        Length = RtlLengthSid(OwnerSid->Sid);
        TokenInfo1->Owner.Owner = DispatchTable.AllocateLsaHeap(Length);
        if (TokenInfo1->Owner.Owner == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        RtlCopyMemory(TokenInfo1->Owner.Owner,
                      OwnerSid->Sid,
                      Length);
        OwnerSid->Attributes |= SE_GROUP_OWNER;
    }

    return STATUS_SUCCESS;
}


static
NTSTATUS
LsapAddTokenDefaultDacl(
    IN PVOID TokenInformation,
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType)
{
    PLSA_TOKEN_INFORMATION_V1 TokenInfo1;
    PACL Dacl = NULL;
    ULONG Length;

    if (TokenInformationType == LsaTokenInformationV1)
    {
        TokenInfo1 = (PLSA_TOKEN_INFORMATION_V1)TokenInformation;

        if (TokenInfo1->DefaultDacl.DefaultDacl != NULL)
            return STATUS_SUCCESS;

        Length = sizeof(ACL) +
                 (2 * sizeof(ACCESS_ALLOWED_ACE)) +
                 RtlLengthSid(TokenInfo1->Owner.Owner) +
                 RtlLengthSid(LsapLocalSystemSid);

        Dacl = DispatchTable.AllocateLsaHeap(Length);
        if (Dacl == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        RtlCreateAcl(Dacl, Length, ACL_REVISION);

        RtlAddAccessAllowedAce(Dacl,
                               ACL_REVISION,
                               GENERIC_ALL,
                               TokenInfo1->Owner.Owner);

        /* SID: S-1-5-18 */
        RtlAddAccessAllowedAce(Dacl,
                               ACL_REVISION,
                               GENERIC_ALL,
                               LsapLocalSystemSid);

        TokenInfo1->DefaultDacl.DefaultDacl = Dacl;
    }

    return STATUS_SUCCESS;
}


static
NTSTATUS
LsapAddPrivilegeToTokenPrivileges(PTOKEN_PRIVILEGES *TokenPrivileges,
                                  PLSAPR_LUID_AND_ATTRIBUTES Privilege)
{
    PTOKEN_PRIVILEGES LocalPrivileges;
    ULONG Length, TokenPrivilegeCount, i;
    NTSTATUS Status = STATUS_SUCCESS;

    if (*TokenPrivileges == NULL)
    {
        Length = sizeof(TOKEN_PRIVILEGES) +
                 (1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES);
        LocalPrivileges = RtlAllocateHeap(RtlGetProcessHeap(),
                                          0,
                                          Length);
        if (LocalPrivileges == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        LocalPrivileges->PrivilegeCount = 1;
        LocalPrivileges->Privileges[0].Luid = Privilege->Luid;
        LocalPrivileges->Privileges[0].Attributes = Privilege->Attributes;
    }
    else
    {
        TokenPrivilegeCount = (*TokenPrivileges)->PrivilegeCount;

        for (i = 0; i < TokenPrivilegeCount; i++)
        {
            if (RtlEqualLuid(&(*TokenPrivileges)->Privileges[i].Luid, &Privilege->Luid))
                return STATUS_SUCCESS;
        }

        Length = sizeof(TOKEN_PRIVILEGES) +
                 (TokenPrivilegeCount + 1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES);
        LocalPrivileges = RtlAllocateHeap(RtlGetProcessHeap(),
                                          0,
                                          Length);
        if (LocalPrivileges == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        LocalPrivileges->PrivilegeCount = TokenPrivilegeCount + 1;
        for (i = 0; i < TokenPrivilegeCount; i++)
        {
            LocalPrivileges->Privileges[i].Luid = (*TokenPrivileges)->Privileges[i].Luid;
            LocalPrivileges->Privileges[i].Attributes = (*TokenPrivileges)->Privileges[i].Attributes;
        }

        LocalPrivileges->Privileges[TokenPrivilegeCount].Luid = Privilege->Luid;
        LocalPrivileges->Privileges[TokenPrivilegeCount].Attributes = Privilege->Attributes;

        RtlFreeHeap(RtlGetProcessHeap(), 0, *TokenPrivileges);
    }

    *TokenPrivileges = LocalPrivileges;

    return Status;
}

static
NTSTATUS
LsapSetPrivileges(
    IN PVOID TokenInformation,
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType)
{
    PLSA_TOKEN_INFORMATION_V1 TokenInfo1;
    LSAPR_HANDLE PolicyHandle = NULL;
    LSAPR_HANDLE AccountHandle = NULL;
    PLSAPR_PRIVILEGE_SET Privileges = NULL;
    ULONG i, j;
    NTSTATUS Status;

    if (TokenInformationType == LsaTokenInformationV1)
    {
        TokenInfo1 = (PLSA_TOKEN_INFORMATION_V1)TokenInformation;

        Status = LsarOpenPolicy(NULL,
                                NULL,
                                0,
                                &PolicyHandle);
        if (!NT_SUCCESS(Status))
            return Status;

        for (i = 0; i < TokenInfo1->Groups->GroupCount; i++)
        {
            Status = LsarOpenAccount(PolicyHandle,
                                     TokenInfo1->Groups->Groups[i].Sid,
                                     ACCOUNT_VIEW,
                                     &AccountHandle);
            if (!NT_SUCCESS(Status))
                continue;

            Status = LsarEnumeratePrivilegesAccount(AccountHandle,
                                                    &Privileges);
            if (NT_SUCCESS(Status))
            {
                for (j = 0; j < Privileges->PrivilegeCount; j++)
                {
                    Status = LsapAddPrivilegeToTokenPrivileges(&TokenInfo1->Privileges,
                                                               &(Privileges->Privilege[j]));
                    if (!NT_SUCCESS(Status))
                    {
                        /* We failed, clean everything and return */
                        LsaIFree_LSAPR_PRIVILEGE_SET(Privileges);
                        LsarClose(&AccountHandle);
                        LsarClose(&PolicyHandle);

                        return Status;
                    }
                }

                LsaIFree_LSAPR_PRIVILEGE_SET(Privileges);
                Privileges = NULL;
            }

            LsarClose(&AccountHandle);
        }

        LsarClose(&PolicyHandle);

        if (TokenInfo1->Privileges != NULL)
        {
            for (i = 0; i < TokenInfo1->Privileges->PrivilegeCount; i++)
            {
                if (RtlEqualLuid(&TokenInfo1->Privileges->Privileges[i].Luid, &SeChangeNotifyPrivilege) ||
                    RtlEqualLuid(&TokenInfo1->Privileges->Privileges[i].Luid, &SeCreateGlobalPrivilege) ||
                    RtlEqualLuid(&TokenInfo1->Privileges->Privileges[i].Luid, &SeImpersonatePrivilege))
                {
                    TokenInfo1->Privileges->Privileges[i].Attributes |= SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_ENABLED_BY_DEFAULT;
                }
            }
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
LsapLogonUser(PLSA_API_MSG RequestMsg,
              PLSAP_LOGON_CONTEXT LogonContext)
{
    PAUTH_PACKAGE Package;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE Qos;
    LSA_TOKEN_INFORMATION_TYPE TokenInformationType;
    PVOID TokenInformation = NULL;
    PLSA_TOKEN_INFORMATION_NULL TokenInfo0 = NULL;
    PLSA_TOKEN_INFORMATION_V1 TokenInfo1 = NULL;
    PUNICODE_STRING AccountName = NULL;
    PUNICODE_STRING AuthenticatingAuthority = NULL;
    PUNICODE_STRING MachineName = NULL;
    PVOID LocalAuthInfo = NULL;
    PTOKEN_GROUPS LocalGroups = NULL;
    HANDLE TokenHandle = NULL;
    ULONG i;
    ULONG PackageId;
    SECURITY_LOGON_TYPE LogonType;
    NTSTATUS Status;

    PUNICODE_STRING UserName = NULL;
    PUNICODE_STRING LogonDomainName = NULL;
//    UNICODE_STRING LogonServer;


    TRACE("LsapLogonUser(%p %p)\n", RequestMsg, LogonContext);

    PackageId = RequestMsg->LogonUser.Request.AuthenticationPackage;
    LogonType = RequestMsg->LogonUser.Request.LogonType;

    /* Get the right authentication package */
    Package = LsapGetAuthenticationPackage(PackageId);
    if (Package == NULL)
    {
        ERR("LsapGetAuthenticationPackage() failed to find a package\n");
        return STATUS_NO_SUCH_PACKAGE;
    }

    if (RequestMsg->LogonUser.Request.AuthenticationInformationLength > 0)
    {
        /* Allocate the local authentication info buffer */
        LocalAuthInfo = RtlAllocateHeap(RtlGetProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        RequestMsg->LogonUser.Request.AuthenticationInformationLength);
        if (LocalAuthInfo == NULL)
        {
            ERR("RtlAllocateHeap() failed\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Read the authentication info from the callers address space */
        Status = NtReadVirtualMemory(LogonContext->ClientProcessHandle,
                                     RequestMsg->LogonUser.Request.AuthenticationInformation,
                                     LocalAuthInfo,
                                     RequestMsg->LogonUser.Request.AuthenticationInformationLength,
                                     NULL);
        if (!NT_SUCCESS(Status))
        {
            ERR("NtReadVirtualMemory() failed (Status 0x%08lx)\n", Status);
            RtlFreeHeap(RtlGetProcessHeap(), 0, LocalAuthInfo);
            return Status;
        }
    }

    if (RequestMsg->LogonUser.Request.LocalGroupsCount > 0)
    {
        Status = LsapCopyLocalGroups(LogonContext,
                                     RequestMsg->LogonUser.Request.LocalGroups,
                                     RequestMsg->LogonUser.Request.LocalGroupsCount,
                                     &LocalGroups);
        if (!NT_SUCCESS(Status))
        {
            ERR("LsapCopyLocalGroups failed (Status 0x%08lx)\n", Status);
            goto done;
        }

        TRACE("GroupCount: %lu\n", LocalGroups->GroupCount);
    }

    if (Package->LsaApLogonUserEx2 != NULL)
    {
        Status = Package->LsaApLogonUserEx2((PLSA_CLIENT_REQUEST)LogonContext,
                                            RequestMsg->LogonUser.Request.LogonType,
                                            LocalAuthInfo,
                                            RequestMsg->LogonUser.Request.AuthenticationInformation,
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
        Status = Package->LsaApLogonUserEx((PLSA_CLIENT_REQUEST)LogonContext,
                                           RequestMsg->LogonUser.Request.LogonType,
                                           LocalAuthInfo,
                                           RequestMsg->LogonUser.Request.AuthenticationInformation,
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
        Status = Package->LsaApLogonUser((PLSA_CLIENT_REQUEST)LogonContext,
                                         RequestMsg->LogonUser.Request.LogonType,
                                         LocalAuthInfo,
                                         RequestMsg->LogonUser.Request.AuthenticationInformation,
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

    if (!NT_SUCCESS(Status))
    {
        ERR("LsaApLogonUser/Ex/2 failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    if (LocalGroups->GroupCount > 0)
    {
        /* Add local groups to the token information */
        Status = LsapAddLocalGroups(TokenInformation,
                                    TokenInformationType,
                                    LocalGroups);
        if (!NT_SUCCESS(Status))
        {
            ERR("LsapAddLocalGroupsToTokenInfo() failed (Status 0x%08lx)\n", Status);
            goto done;
        }
    }

    Status = LsapAddDefaultGroups(TokenInformation,
                                  TokenInformationType,
                                  LogonType);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapAddDefaultGroups() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    Status = LsapAddSamGroups(TokenInformation,
                              TokenInformationType);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapAddSamGroups() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    Status = LsapSetTokenOwner(TokenInformation,
                               TokenInformationType);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapSetTokenOwner() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    Status = LsapAddTokenDefaultDacl(TokenInformation,
                                     TokenInformationType);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapAddTokenDefaultDacl() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    Status = LsapSetPrivileges(TokenInformation,
                               TokenInformationType);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapSetPrivileges() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    if (TokenInformationType == LsaTokenInformationNull)
    {
        TOKEN_USER TokenUser;
        TOKEN_PRIMARY_GROUP TokenPrimaryGroup;
        TOKEN_GROUPS NoGroups = {0};
        TOKEN_PRIVILEGES NoPrivileges = {0};

        TokenInfo0 = (PLSA_TOKEN_INFORMATION_NULL)TokenInformation;

        TokenUser.User.Sid = LsapWorldSid;
        TokenUser.User.Attributes = 0;
        TokenPrimaryGroup.PrimaryGroup = LsapWorldSid;

        Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        Qos.ImpersonationLevel = SecurityImpersonation;
        Qos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
        Qos.EffectiveOnly = TRUE;

        ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
        ObjectAttributes.RootDirectory = NULL;
        ObjectAttributes.ObjectName = NULL;
        ObjectAttributes.Attributes = 0;
        ObjectAttributes.SecurityDescriptor = NULL;
        ObjectAttributes.SecurityQualityOfService = &Qos;

        /* Create the logon token */
        Status = NtCreateToken(&TokenHandle,
                               TOKEN_ALL_ACCESS,
                               &ObjectAttributes,
                               TokenImpersonation,
                               &RequestMsg->LogonUser.Reply.LogonId,
                               &TokenInfo0->ExpirationTime,
                               &TokenUser,
                               &NoGroups,
                               &NoPrivileges,
                               NULL,
                               &TokenPrimaryGroup,
                               NULL,
                               &RequestMsg->LogonUser.Request.SourceContext);
    }
    else if (TokenInformationType == LsaTokenInformationV1)
    {
        TOKEN_PRIVILEGES NoPrivileges = {0};
        PSECURITY_DESCRIPTOR TokenSd;
        ULONG TokenSdSize;

        TokenInfo1 = (PLSA_TOKEN_INFORMATION_V1)TokenInformation;

        /* Set up a security descriptor for token object itself */
        Status = LsapCreateTokenSd(&TokenInfo1->User, &TokenSd, &TokenSdSize);
        if (!NT_SUCCESS(Status))
        {
            TokenSd = NULL;
        }

        Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        Qos.ImpersonationLevel = SecurityImpersonation;
        Qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        Qos.EffectiveOnly = FALSE;

        ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
        ObjectAttributes.RootDirectory = NULL;
        ObjectAttributes.ObjectName = NULL;
        ObjectAttributes.Attributes = 0;
        ObjectAttributes.SecurityDescriptor = TokenSd;
        ObjectAttributes.SecurityQualityOfService = &Qos;

        /* Create the logon token */
        Status = NtCreateToken(&TokenHandle,
                               TOKEN_ALL_ACCESS,
                               &ObjectAttributes,
                               (RequestMsg->LogonUser.Request.LogonType == Network) ? TokenImpersonation : TokenPrimary,
                               &RequestMsg->LogonUser.Reply.LogonId,
                               &TokenInfo1->ExpirationTime,
                               &TokenInfo1->User,
                               TokenInfo1->Groups,
                               TokenInfo1->Privileges ? TokenInfo1->Privileges : &NoPrivileges,
                               &TokenInfo1->Owner,
                               &TokenInfo1->PrimaryGroup,
                               &TokenInfo1->DefaultDacl,
                               &RequestMsg->LogonUser.Request.SourceContext);

        /* Free the allocated security descriptor */
        RtlFreeHeap(RtlGetProcessHeap(), 0, TokenSd);

        if (!NT_SUCCESS(Status))
        {
            ERR("NtCreateToken failed (Status 0x%08lx)\n", Status);
            goto done;
        }
    }
    else
    {
        FIXME("TokenInformationType %d is not supported!\n", TokenInformationType);
        Status = STATUS_NOT_IMPLEMENTED;
        goto done;
    }

    if (LogonType == Interactive ||
        LogonType == Batch ||
        LogonType == Service)
    {
        UserName = &((PMSV1_0_INTERACTIVE_LOGON)LocalAuthInfo)->UserName;
        LogonDomainName = &((PMSV1_0_INTERACTIVE_LOGON)LocalAuthInfo)->LogonDomainName;
    }
    else
    {
        FIXME("LogonType %lu is not supported yet!\n", LogonType);
    }

    Status = LsapSetLogonSessionData(&RequestMsg->LogonUser.Reply.LogonId,
                                     LogonType,
                                     UserName,
                                     LogonDomainName,
                                     TokenInfo1->User.User.Sid);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapSetLogonSessionData failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /*
     * Duplicate the token handle into the client process.
     * This must be the last step because we cannot
     * close the duplicated token handle in case something fails.
     */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               TokenHandle,
                               LogonContext->ClientProcessHandle,
                               &RequestMsg->LogonUser.Reply.Token,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES | DUPLICATE_CLOSE_SOURCE);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtDuplicateObject failed (Status 0x%08lx)\n", Status);
        goto done;
    }

done:
    if (!NT_SUCCESS(Status))
    {
        /* Notify the authentification package of the failure */
        Package->LsaApLogonTerminated(&RequestMsg->LogonUser.Reply.LogonId);

        /* Delete the logon session */
        LsapDeleteLogonSession(&RequestMsg->LogonUser.Reply.LogonId);

        /* Release the profile buffer */
        LsapFreeClientBuffer((PLSA_CLIENT_REQUEST)LogonContext,
                             RequestMsg->LogonUser.Reply.ProfileBuffer);
        RequestMsg->LogonUser.Reply.ProfileBuffer = NULL;
    }

    if (TokenHandle != NULL)
        NtClose(TokenHandle);

    /* Free the local groups */
    if (LocalGroups != NULL)
    {
        for (i = 0; i < LocalGroups->GroupCount; i++)
        {
            if (LocalGroups->Groups[i].Sid != NULL)
                RtlFreeHeap(RtlGetProcessHeap(), 0, LocalGroups->Groups[i].Sid);
        }

        RtlFreeHeap(RtlGetProcessHeap(), 0, LocalGroups);
    }

    /* Free the local authentication info buffer */
    if (LocalAuthInfo != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, LocalAuthInfo);

    /* Free the token information */
    if (TokenInformation != NULL)
    {
        if (TokenInformationType == LsaTokenInformationNull)
        {
            TokenInfo0 = (PLSA_TOKEN_INFORMATION_NULL)TokenInformation;

            if (TokenInfo0 != NULL)
            {
                if (TokenInfo0->Groups != NULL)
                {
                    for (i = 0; i < TokenInfo0->Groups->GroupCount; i++)
                    {
                        if (TokenInfo0->Groups->Groups[i].Sid != NULL)
                            LsapFreeHeap(TokenInfo0->Groups->Groups[i].Sid);
                    }

                    LsapFreeHeap(TokenInfo0->Groups);
                }

                LsapFreeHeap(TokenInfo0);
            }
        }
        else if (TokenInformationType == LsaTokenInformationV1)
        {
            TokenInfo1 = (PLSA_TOKEN_INFORMATION_V1)TokenInformation;

            if (TokenInfo1 != NULL)
            {
                if (TokenInfo1->User.User.Sid != NULL)
                    LsapFreeHeap(TokenInfo1->User.User.Sid);

                if (TokenInfo1->Groups != NULL)
                {
                    for (i = 0; i < TokenInfo1->Groups->GroupCount; i++)
                    {
                        if (TokenInfo1->Groups->Groups[i].Sid != NULL)
                            LsapFreeHeap(TokenInfo1->Groups->Groups[i].Sid);
                    }

                    LsapFreeHeap(TokenInfo1->Groups);
                }

                if (TokenInfo1->PrimaryGroup.PrimaryGroup != NULL)
                    LsapFreeHeap(TokenInfo1->PrimaryGroup.PrimaryGroup);

                if (TokenInfo1->Privileges != NULL)
                    LsapFreeHeap(TokenInfo1->Privileges);

                if (TokenInfo1->Owner.Owner != NULL)
                    LsapFreeHeap(TokenInfo1->Owner.Owner);

                if (TokenInfo1->DefaultDacl.DefaultDacl != NULL)
                    LsapFreeHeap(TokenInfo1->DefaultDacl.DefaultDacl);

                LsapFreeHeap(TokenInfo1);
            }
        }
        else
        {
            FIXME("TokenInformationType %d is not supported!\n", TokenInformationType);
        }
    }

    /* Free the account name */
    if (AccountName != NULL)
    {
        if (AccountName->Buffer != NULL)
            LsapFreeHeap(AccountName->Buffer);

        LsapFreeHeap(AccountName);
    }

    /* Free the authentication authority */
    if (AuthenticatingAuthority != NULL)
    {
        if (AuthenticatingAuthority->Buffer != NULL)
            LsapFreeHeap(AuthenticatingAuthority->Buffer);

        LsapFreeHeap(AuthenticatingAuthority);
    }

    /* Free the machine name */
    if (MachineName != NULL)
    {
        if (MachineName->Buffer != NULL)
            LsapFreeHeap(MachineName->Buffer);

        LsapFreeHeap(MachineName);
    }

    TRACE("LsapLogonUser done (Status 0x%08lx)\n", Status);

    return Status;
}

/* EOF */
