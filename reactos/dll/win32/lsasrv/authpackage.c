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


typedef PVOID (NTAPI *PLSA_ALLOCATE_LSA_HEAP)(ULONG);
typedef VOID (NTAPI *PLSA_FREE_LSA_HEAP)(PVOID);

typedef struct LSA_DISPATCH_TABLE
{
    PVOID /*PLSA_CREATE_LOGON_SESSION */ CreateLogonSession;
    PVOID /*PLSA_DELETE_LOGON_SESSION */ DeleteLogonSession;
    PVOID /*PLSA_ADD_CREDENTIAL */ AddCredential;
    PVOID /*PLSA_GET_CREDENTIALS */ GetCredentials;
    PVOID /*PLSA_DELETE_CREDENTIAL */ DeleteCredential;
    PLSA_ALLOCATE_LSA_HEAP AllocateLsaHeap;
    PLSA_FREE_LSA_HEAP FreeLsaHeap;
    PVOID /*PLSA_ALLOCATE_CLIENT_BUFFER */ AllocateClientBuffer;
    PVOID /*PLSA_FREE_CLIENT_BUFFER */ FreeClientBuffer;
    PVOID /*PLSA_COPY_TO_CLIENT_BUFFER */ CopyToClientBuffer;
    PVOID /*PLSA_COPY_FROM_CLIENT_BUFFER */ CopyFromClientBuffer;
} LSA_DISPATCH_TABLE, *PLSA_DISPATCH_TABLE;


typedef NTSTATUS (NTAPI *PLSA_AP_INITIALIZE_PACKAGE)(ULONG, PLSA_DISPATCH_TABLE,
 PLSA_STRING, PLSA_STRING, PLSA_STRING *);
typedef NTSTATUS (NTAPI *PLSA_AP_CALL_PACKAGE)(PUNICODE_STRING, PVOID, ULONG,
 PVOID *, PULONG, PNTSTATUS);
typedef NTSTATUS (NTAPI *PLSA_AP_CALL_PACKAGE_PASSTHROUGH)(PUNICODE_STRING,
 PVOID, PVOID, ULONG, PVOID *, PULONG, PNTSTATUS);
typedef NTSTATUS (NTAPI *PLSA_AP_CALL_PACKAGE_UNTRUSTED)(PVOID/*PLSA_CLIENT_REQUEST*/,
 PVOID, PVOID, ULONG, PVOID *, PULONG, PNTSTATUS);
typedef VOID (NTAPI *PLSA_AP_LOGON_TERMINATED)(PLUID);
typedef NTSTATUS (NTAPI *PLSA_AP_LOGON_USER_EX2)(PVOID /*PLSA_CLIENT_REQUEST*/,
 SECURITY_LOGON_TYPE, PVOID, PVOID, ULONG, PVOID *, PULONG, PLUID, PNTSTATUS,
 PVOID /*PLSA_TOKEN_INFORMATION_TYPE*/, PVOID *, PUNICODE_STRING *, PUNICODE_STRING *,
 PUNICODE_STRING *, PVOID /*PSECPKG_PRIMARY_CRED*/, PVOID /*PSECPKG_SUPPLEMENTAL_CRED_ARRAY **/);
typedef NTSTATUS (NTAPI *PLSA_AP_LOGON_USER_EX)(PVOID /*PLSA_CLIENT_REQUEST*/,
 SECURITY_LOGON_TYPE, PVOID, PVOID, ULONG, PVOID *, PULONG, PLUID, PNTSTATUS,
 PVOID /*PLSA_TOKEN_INFORMATION_TYPE*/, PVOID *, PUNICODE_STRING *, PUNICODE_STRING *,
 PUNICODE_STRING *);
typedef NTSTATUS (NTAPI *PLSA_AP_LOGON_USER)(LPWSTR, LPWSTR, LPWSTR, LPWSTR,
 DWORD, DWORD, PHANDLE);

typedef struct _AUTH_PACKAGE
{
    LIST_ENTRY Entry;
    PSTRING Name;
    ULONG Id;
    PVOID ModuleHandle;

    PLSA_AP_INITIALIZE_PACKAGE LsaApInitializePackage;
    PLSA_AP_CALL_PACKAGE LsaApCallPackage;
    PLSA_AP_CALL_PACKAGE_PASSTHROUGH LsaApCallPackagePassthrough;
    PLSA_AP_CALL_PACKAGE_UNTRUSTED LsaApCallPackageUntrusted;
    PLSA_AP_LOGON_TERMINATED LsaApLogonTerminated;
    PLSA_AP_LOGON_USER_EX2 LsaApLogonUserEx2;
    PLSA_AP_LOGON_USER_EX LsaApLogonUserEx;
    PLSA_AP_LOGON_USER LsaApLogonUser;
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
PVOID
NTAPI
LsapAllocateHeap(ULONG Size)
{
    return RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, Size);
}

static
VOID
NTAPI
LsapFreeHeap(PVOID Ptr)
{
    RtlFreeHeap(RtlGetProcessHeap(), 0, Ptr);
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
    DispatchTable.AllocateClientBuffer = NULL;
    DispatchTable.FreeClientBuffer = NULL;
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
LsapLookupAuthenticationPackageByName(IN PSTRING PackageName,
                                      OUT PULONG PackageId)
{
    PLIST_ENTRY ListEntry;
    PAUTH_PACKAGE Package;

    ListEntry = PackageListHead.Flink;
    while (ListEntry != &PackageListHead)
    {
        Package = CONTAINING_RECORD(ListEntry, AUTH_PACKAGE, Entry);

        if ((PackageName->Length == Package->Name->Length) &&
            (_strnicmp(PackageName->Buffer, Package->Name->Buffer, Package->Name->Length) == 0))
        {
            *PackageId = Package->Id;
            return STATUS_SUCCESS;
        }

        ListEntry = ListEntry->Flink;
    }

    return STATUS_NO_SUCH_PACKAGE;
}

/* EOF */
