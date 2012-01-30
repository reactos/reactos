/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/smss.c
 * PURPOSE:         Main SMSS Code
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "smss.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS ********************************************************************/

UNICODE_STRING SmpSubsystemName, PosixName, Os2Name;
LIST_ENTRY NativeProcessList;
ULONG SmBaseTag;
PVOID SmpHeap;
HANDLE SmpDebugPort;
ULONG SmpInitProgressByLine;
NTSTATUS SmpInitReturnStatus;
PVOID SmpInitLastCall;
SECURITY_DESCRIPTOR SmpPrimarySDBody, SmpLiberalSDBody, SmpKnownDllsSDBody;
SECURITY_DESCRIPTOR SmpApiPortSDBody;
PSECURITY_DESCRIPTOR SmpPrimarySecurityDescriptor, SmpLiberalSecurityDescriptor;
PSECURITY_DESCRIPTOR SmpKnownDllsSecurityDescriptor, SmpApiPortSecurityDescriptor;
ULONG SmpProtectionMode = 1;

#define SMSS_CHECKPOINT(x, y)           \
{                                       \
    SmpInitProgressByLine = __LINE__;   \
    SmpInitReturnStatus = (y);          \
    SmpInitLastCall = (x);              \
}

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
SmpCreateSecurityDescriptors(IN BOOLEAN InitialCall)
{
    NTSTATUS Status;
    PSID WorldSid = NULL, AdminSid = NULL, SystemSid = NULL;
    PSID RestrictedSid = NULL, OwnerSid = NULL;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY CreatorAuthority = {SECURITY_CREATOR_SID_AUTHORITY};
    ULONG AclLength, SidLength;
    PACL Acl;
    PACE_HEADER Ace;
    BOOLEAN ProtectionRequired = FALSE;

    /* Check if this is the first call */
    if (InitialCall)
    {
        /* Create and set the primary descriptor */
        SmpPrimarySecurityDescriptor = &SmpPrimarySDBody;
        Status = RtlCreateSecurityDescriptor(SmpPrimarySecurityDescriptor,
                                             SECURITY_DESCRIPTOR_REVISION);
        ASSERT(NT_SUCCESS(Status));
        Status = RtlSetDaclSecurityDescriptor(SmpPrimarySecurityDescriptor,
                                              TRUE,
                                              NULL,
                                              FALSE);
        ASSERT(NT_SUCCESS(Status));

        /* Create and set the liberal descriptor */
        SmpLiberalSecurityDescriptor = &SmpLiberalSDBody;
        Status = RtlCreateSecurityDescriptor(SmpLiberalSecurityDescriptor,
                                             SECURITY_DESCRIPTOR_REVISION);
        ASSERT(NT_SUCCESS(Status));
        Status = RtlSetDaclSecurityDescriptor(SmpLiberalSecurityDescriptor,
                                              TRUE,
                                              NULL,
                                              FALSE);
        ASSERT(NT_SUCCESS(Status));

        /* Create and set the \KnownDlls descriptor */
        SmpKnownDllsSecurityDescriptor = &SmpKnownDllsSDBody;
        Status = RtlCreateSecurityDescriptor(SmpKnownDllsSecurityDescriptor,
                                             SECURITY_DESCRIPTOR_REVISION);
        ASSERT(NT_SUCCESS(Status));
        Status = RtlSetDaclSecurityDescriptor(SmpKnownDllsSecurityDescriptor,
                                              TRUE,
                                              NULL,
                                              FALSE);
        ASSERT(NT_SUCCESS(Status));

        /* Create and Set the \ApiPort descriptor */
        SmpApiPortSecurityDescriptor = &SmpApiPortSDBody;
        Status = RtlCreateSecurityDescriptor(SmpApiPortSecurityDescriptor,
                                             SECURITY_DESCRIPTOR_REVISION);
        ASSERT(NT_SUCCESS(Status));
        Status = RtlSetDaclSecurityDescriptor(SmpApiPortSecurityDescriptor,
                                              TRUE,
                                              NULL,
                                              FALSE);
        ASSERT(NT_SUCCESS(Status));
    }

    /* Check if protection was requested in the registry (on by default) */
    if (SmpProtectionMode & 1) ProtectionRequired = TRUE;

    /* Exit if there's nothing to do */
    if (!(InitialCall || ProtectionRequired)) return STATUS_SUCCESS;

    /* Build the world SID */
    Status = RtlAllocateAndInitializeSid(&WorldAuthority, 1,
                                         SECURITY_WORLD_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &WorldSid);
    if (!NT_SUCCESS(Status))
    {
        WorldSid = NULL;
        goto Quickie;
    }

    /* Build the admin SID */
    Status = RtlAllocateAndInitializeSid(&NtAuthority, 2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0, 0, 0, 0, 0, 0,
                                         &AdminSid);
    if (!NT_SUCCESS(Status))
    {
        AdminSid = NULL;
        goto Quickie;
    }

    /* Build the owner SID */
    Status = RtlAllocateAndInitializeSid(&CreatorAuthority, 1,
                                         SECURITY_CREATOR_OWNER_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &OwnerSid);
    if (!NT_SUCCESS(Status))
    {
        OwnerSid = NULL;
        goto Quickie;
    }

    /* Build the restricted SID */
    Status = RtlAllocateAndInitializeSid(&NtAuthority, 1,
                                         SECURITY_RESTRICTED_CODE_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &RestrictedSid);
    if (!NT_SUCCESS(Status))
    {
        RestrictedSid = NULL;
        goto Quickie;
    }

    /* Build the system SID */
    Status = RtlAllocateAndInitializeSid(&NtAuthority, 1,
                                         SECURITY_LOCAL_SYSTEM_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &SystemSid);
    if (!NT_SUCCESS(Status))
    {
        SystemSid = NULL;
        goto Quickie;
    }

    /* Now check if we're creating the core descriptors */
    if (!InitialCall)
    {
        /* We're skipping NextAcl so we have to do this here */
        SidLength = RtlLengthSid(WorldSid) + RtlLengthSid(RestrictedSid) + RtlLengthSid(AdminSid);
        SidLength *= 2;
        goto NotInitial;
    }

    /* Allocate an ACL with two ACEs with two SIDs each */
    SidLength = RtlLengthSid(SystemSid) + RtlLengthSid(AdminSid);
    AclLength = sizeof(ACL) + 2 * sizeof(ACCESS_ALLOWED_ACE) + SidLength;
    Acl = RtlAllocateHeap(RtlGetProcessHeap(), 0, AclLength);
    if (!Acl) Status = STATUS_NO_MEMORY;
    if (!NT_SUCCESS(Status)) goto NextAcl;

    /* Now build the ACL and add the two ACEs */
    Status = RtlCreateAcl(Acl, AclLength, ACL_REVISION2);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, AdminSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, SystemSid);
    ASSERT(NT_SUCCESS(Status));

    /* Set this as the DACL */
    Status = RtlSetDaclSecurityDescriptor(SmpApiPortSecurityDescriptor,
                                          TRUE,
                                          Acl,
                                          FALSE);
    ASSERT(NT_SUCCESS(Status));

NextAcl:
    /* Allocate an ACL with 6 ACEs, two ACEs per SID */
    SidLength = RtlLengthSid(WorldSid) + RtlLengthSid(RestrictedSid) + RtlLengthSid(AdminSid);
    SidLength *= 2;
    AclLength = sizeof(ACL) + 6 * sizeof(ACCESS_ALLOWED_ACE) + SidLength;
    Acl = RtlAllocateHeap(RtlGetProcessHeap(), 0, AclLength);
    if (!Acl) Status = STATUS_NO_MEMORY;
    if (!NT_SUCCESS(Status)) goto NotInitial;

    /* Now build the ACL and add the six ACEs */
    Status = RtlCreateAcl(Acl, AclLength, ACL_REVISION2);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE, WorldSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE, RestrictedSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, AdminSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE, WorldSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE, RestrictedSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, AdminSid);
    ASSERT(NT_SUCCESS(Status));

    /* Now edit the last three ACEs and make them inheritable */
    Status = RtlGetAce(Acl, 3, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
    Status = RtlGetAce(Acl, 4, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
    Status = RtlGetAce(Acl, 5, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;

    /* Set this as the DACL */
    Status = RtlSetDaclSecurityDescriptor(SmpKnownDllsSecurityDescriptor,
                                          TRUE,
                                          Acl,
                                          FALSE);
    ASSERT(NT_SUCCESS(Status));

NotInitial:
    /* The initial ACLs have been created, are we also protecting objects? */
    if (!ProtectionRequired) goto Quickie;

    /* Allocate an ACL with 7 ACEs, two ACEs per SID, and one final owner ACE */
    SidLength += RtlLengthSid(OwnerSid);
    AclLength = sizeof(ACL) + 7 * sizeof (ACCESS_ALLOWED_ACE) + 2 * SidLength;
    Acl = RtlAllocateHeap(RtlGetProcessHeap(), 0, AclLength);
    if (!Acl) Status = STATUS_NO_MEMORY;
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Build the ACL and add the seven ACEs */
    Status = RtlCreateAcl(Acl, AclLength, ACL_REVISION2);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ, WorldSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ, RestrictedSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, AdminSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ, WorldSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ, RestrictedSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, AdminSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, OwnerSid);
    ASSERT(NT_SUCCESS(Status));

    /* Edit the last 4 ACEs to make then inheritable */
    Status = RtlGetAce(Acl, 3, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
    Status = RtlGetAce(Acl, 4, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
    Status = RtlGetAce(Acl, 5, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
    Status = RtlGetAce(Acl, 6, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;

    /* Set this as the DACL for the primary SD */
    Status = RtlSetDaclSecurityDescriptor(SmpPrimarySecurityDescriptor,
                                          TRUE,
                                          Acl,
                                          FALSE);
    ASSERT(NT_SUCCESS(Status));

    /* Allocate an ACL with 7 ACEs, two ACEs per SID, and one final owner ACE */
    AclLength = sizeof(ACL) + 7 * sizeof (ACCESS_ALLOWED_ACE) + 2 * SidLength;
    Acl = RtlAllocateHeap(RtlGetProcessHeap(), 0, AclLength);
    if (!Acl) Status = STATUS_NO_MEMORY;
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Build the ACL and add the seven ACEs */
    Status = RtlCreateAcl(Acl, AclLength, ACL_REVISION2);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE, WorldSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE, RestrictedSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, AdminSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE, WorldSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE, RestrictedSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, AdminSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, OwnerSid);
    ASSERT(NT_SUCCESS(Status));

    /* Edit the last 4 ACEs to make then inheritable */
    Status = RtlGetAce(Acl, 3, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
    Status = RtlGetAce(Acl, 4, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
    Status = RtlGetAce(Acl, 5, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
    Status = RtlGetAce(Acl, 6, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;

    /* Now set this as the DACL for the liberal SD */
    Status = RtlSetDaclSecurityDescriptor(SmpLiberalSecurityDescriptor,
                                          TRUE,
                                          Acl,
                                          FALSE);
    ASSERT(NT_SUCCESS(Status));

Quickie:
    /* Cleanup the SIDs */
    if (OwnerSid) RtlFreeHeap(RtlGetProcessHeap(), 0, OwnerSid);
    if (AdminSid) RtlFreeHeap(RtlGetProcessHeap(), 0, AdminSid);
    if (WorldSid) RtlFreeHeap(RtlGetProcessHeap(), 0, WorldSid);
    if (SystemSid) RtlFreeHeap(RtlGetProcessHeap(), 0, SystemSid);
    if (RestrictedSid) RtlFreeHeap(RtlGetProcessHeap(), 0, RestrictedSid);
    return Status;
}

NTSTATUS
NTAPI
SmpLoadDataFromRegistry(OUT PUNICODE_STRING InitialCommand)
{
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SmpInit(IN PUNICODE_STRING InitialCommand,
        OUT PHANDLE ProcessHandle)
{
    NTSTATUS Status, Status2;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING PortName, EventName;
    HANDLE EventHandle, PortHandle;
    ULONG HardErrorMode;

    /* Create the SMSS Heap */
    SmBaseTag = RtlCreateTagHeap(RtlGetProcessHeap(),
                                 0,
                                 L"SMSS!",
                                 L"INIT");
    SmpHeap = RtlGetProcessHeap();

    /* Enable hard errors */
    HardErrorMode = TRUE;
    NtSetInformationProcess(NtCurrentProcess(),
                            ProcessDefaultHardErrorMode,
                            &HardErrorMode,
                            sizeof(HardErrorMode));

    /* Initialize the subsystem list and the session list, plus their locks */
    RtlInitializeCriticalSection(&SmpKnownSubSysLock);
    InitializeListHead(&SmpKnownSubSysHead);
    RtlInitializeCriticalSection(&SmpSessionListLock);
    InitializeListHead(&SmpSessionListHead);

    /* Initialize the process list */
    InitializeListHead(&NativeProcessList);

    /* Initialize session parameters */
    SmpNextSessionId = 1;
    SmpNextSessionIdScanMode = 0;
    SmpDbgSsLoaded = FALSE;
    
    /* Create the initial security descriptors */
    Status = SmpCreateSecurityDescriptors(TRUE);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        SMSS_CHECKPOINT(SmpCreateSecurityDescriptors, Status);
        return Status;
    }

    /* Initialize subsystem names */
    RtlInitUnicodeString(&SmpSubsystemName, L"NT-Session Manager");
    RtlInitUnicodeString(&PosixName, L"POSIX");
    RtlInitUnicodeString(&Os2Name, L"OS2");

    /* Create the SM API Port */
    RtlInitUnicodeString(&PortName, L"\\Sm2ApiPort");
    InitializeObjectAttributes(&ObjectAttributes, &PortName, 0, NULL, NULL);
    Status = NtCreatePort(&PortHandle,
                          &ObjectAttributes,
                          sizeof(SB_CONNECTION_INFO),
                          sizeof(SM_API_MSG),
                          sizeof(SB_API_MSG) * 32);
    ASSERT(NT_SUCCESS(Status));
    SmpDebugPort = PortHandle;

    /* Create two SM API threads */
    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 FALSE,
                                 0,
                                 0,
                                 0,
                                 SmpApiLoop,
                                 PortHandle,
                                 NULL,
                                 NULL);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 FALSE,
                                 0,
                                 0,
                                 0,
                                 SmpApiLoop,
                                 PortHandle,
                                 NULL,
                                 NULL);
    ASSERT(NT_SUCCESS(Status));

    /* Create the write event that autochk can set after running */
    RtlInitUnicodeString(&EventName, L"\\Device\\VolumesSafeForWriteAccess");
    InitializeObjectAttributes(&ObjectAttributes,
                               &EventName,
                               OBJ_PERMANENT,
                               NULL,
                               NULL);
    Status2 = NtCreateEvent(&EventHandle,
                            EVENT_ALL_ACCESS,
                            &ObjectAttributes,
                            0,
                            0);
    if (!NT_SUCCESS(Status2))
    {
        /* Should never really fail */
        DPRINT1("SMSS: Unable to create %wZ event - Status == %lx\n",
                &EventName, Status2);
        ASSERT(NT_SUCCESS(Status2));
    }

    /* Now initialize everything else based on the registry parameters */
    Status = SmpLoadDataFromRegistry(InitialCommand);
    if (NT_SUCCESS(Status))
    {
        /* Autochk should've run now. Set the event and save the CSRSS handle */
        *ProcessHandle = SmpWindowsSubSysProcess;
        NtSetEvent(EventHandle, 0);
        NtClose(EventHandle);
    }

    /* All done */
    return Status;
}
