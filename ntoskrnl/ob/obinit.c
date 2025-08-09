/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/obinit.c
 * PURPOSE:         Handles Object Manager Initialization and Shutdown
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

GENERIC_MAPPING ObpTypeMapping =
{
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    0x000F0001
};

GENERIC_MAPPING ObpDirectoryMapping =
{
    STANDARD_RIGHTS_READ    | DIRECTORY_QUERY               |
    DIRECTORY_TRAVERSE,
    STANDARD_RIGHTS_WRITE   | DIRECTORY_CREATE_SUBDIRECTORY |
    DIRECTORY_CREATE_OBJECT,
    STANDARD_RIGHTS_EXECUTE | DIRECTORY_QUERY               |
    DIRECTORY_TRAVERSE,
    DIRECTORY_ALL_ACCESS
};

GENERIC_MAPPING ObpSymbolicLinkMapping =
{
    STANDARD_RIGHTS_READ    | SYMBOLIC_LINK_QUERY,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE | SYMBOLIC_LINK_QUERY,
    SYMBOLIC_LINK_ALL_ACCESS
};

PDEVICE_MAP ObSystemDeviceMap = NULL;
ULONG ObpTraceLevel = 0;

CODE_SEG("INIT")
VOID
NTAPI
PsInitializeQuotaSystem(VOID);

ULONG ObpInitializationPhase;

ULONG ObpObjectSecurityMode = 0;
ULONG ObpProtectionMode = 0;

/* PRIVATE FUNCTIONS *********************************************************/

static
CODE_SEG("INIT")
NTSTATUS
NTAPI
ObpCreateKernelObjectsSD(OUT PSECURITY_DESCRIPTOR *SecurityDescriptor)
{
    PSECURITY_DESCRIPTOR Sd = NULL;
    PACL Dacl;
    ULONG AclSize, SdSize;
    NTSTATUS Status;

    AclSize = sizeof(ACL) +
              sizeof(ACE) + RtlLengthSid(SeWorldSid) +
              sizeof(ACE) + RtlLengthSid(SeAliasAdminsSid) +
              sizeof(ACE) + RtlLengthSid(SeLocalSystemSid);

    SdSize = sizeof(SECURITY_DESCRIPTOR) + AclSize;

    /* Allocate the SD and ACL */
    Sd = ExAllocatePoolWithTag(PagedPool, SdSize, TAG_SD);
    if (Sd == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize the SD */
    Status = RtlCreateSecurityDescriptor(Sd,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
        goto done;

    Dacl = (PACL)((INT_PTR)Sd + sizeof(SECURITY_DESCRIPTOR));

    /* Initialize the DACL */
    RtlCreateAcl(Dacl, AclSize, ACL_REVISION);

    /* Add the ACEs */
    RtlAddAccessAllowedAce(Dacl,
                           ACL_REVISION,
                           GENERIC_READ,
                           SeWorldSid);

    RtlAddAccessAllowedAce(Dacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           SeAliasAdminsSid);

    RtlAddAccessAllowedAce(Dacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           SeLocalSystemSid);

    /* Attach the DACL to the SD */
    Status = RtlSetDaclSecurityDescriptor(Sd,
                                          TRUE,
                                          Dacl,
                                          FALSE);
    if (!NT_SUCCESS(Status))
        goto done;

    *SecurityDescriptor = Sd;

done:
    if (!NT_SUCCESS(Status))
    {
        if (Sd != NULL)
            ExFreePoolWithTag(Sd, TAG_SD);
    }

    return Status;
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
ObInit2(VOID)
{
    CCHAR i;
    PKPRCB Prcb;
    PGENERAL_LOOKASIDE CurrentList = NULL;

    /* Now allocate the per-processor lists */
    for (i = 0; i < KeNumberProcessors; i++)
    {
        /* Get the PRCB for this CPU */
        Prcb = KiProcessorBlock[(int)i];

        /* Set the OBJECT_CREATE_INFORMATION List */
        Prcb->PPLookasideList[LookasideCreateInfoList].L = &ObpCreateInfoLookasideList;
        CurrentList = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(GENERAL_LOOKASIDE),
                                            'ICbO');
        if (CurrentList)
        {
            /* Initialize it */
            ExInitializeSystemLookasideList(CurrentList,
                                            NonPagedPool,
                                            sizeof(OBJECT_CREATE_INFORMATION),
                                            'ICbO',
                                            32,
                                            &ExSystemLookasideListHead);
        }
        else
        {
            /* No list, use the static buffer */
            CurrentList = &ObpCreateInfoLookasideList;
        }

        /* Link it */
        Prcb->PPLookasideList[LookasideCreateInfoList].P = CurrentList;

        /* Set the captured UNICODE_STRING Object Name List */
        Prcb->PPLookasideList[LookasideNameBufferList].L = &ObpNameBufferLookasideList;
        CurrentList = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(GENERAL_LOOKASIDE),
                                            'MNbO');
        if (CurrentList)
        {
            /* Initialize it */
            ExInitializeSystemLookasideList(CurrentList,
                                            PagedPool,
                                            248,
                                            'MNbO',
                                            16,
                                            &ExSystemLookasideListHead);
        }
        else
        {
            /* No list, use the static buffer */
            CurrentList = &ObpNameBufferLookasideList;
        }

        /* Link it */
        Prcb->PPLookasideList[LookasideNameBufferList].P = CurrentList;
    }

    return TRUE;
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
ObInitSystem(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    OBP_LOOKUP_CONTEXT Context;
    HANDLE Handle;
    PKPRCB Prcb = KeGetCurrentPrcb();
    PLIST_ENTRY ListHead, NextEntry;
    POBJECT_HEADER Header;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POBJECT_HEADER_NAME_INFO NameInfo;
    PSECURITY_DESCRIPTOR KernelObjectsSD = NULL;
    NTSTATUS Status;

#ifdef _M_AMD64
    /* Debug output for AMD64 */
    #define COM_PORT 0x3F8
    {
        const char msg[] = "*** OB: ObInitSystem entered ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif

    /* Check if this is actually Phase 1 initialization */
    if (ObpInitializationPhase != 0) goto ObPostPhase0;

#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Phase 0 initialization starting ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif

    /* Initialize the OBJECT_CREATE_INFORMATION List */
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Initializing CreateInfo lookaside list ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif
    ExInitializeSystemLookasideList(&ObpCreateInfoLookasideList,
                                    NonPagedPool,
                                    sizeof(OBJECT_CREATE_INFORMATION),
                                    'ICbO',
                                    32,
                                    &ExSystemLookasideListHead);

#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: CreateInfo lookaside initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif

    /* Set the captured UNICODE_STRING Object Name List */
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Initializing NameBuffer lookaside list ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif
    ExInitializeSystemLookasideList(&ObpNameBufferLookasideList,
                                    PagedPool,
                                    248,
                                    'MNbO',
                                    16,
                                    &ExSystemLookasideListHead);

#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: NameBuffer lookaside initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif

    /* Temporarily setup both pointers to the shared list */
    Prcb->PPLookasideList[LookasideCreateInfoList].L = &ObpCreateInfoLookasideList;
    Prcb->PPLookasideList[LookasideCreateInfoList].P = &ObpCreateInfoLookasideList;
    Prcb->PPLookasideList[LookasideNameBufferList].L = &ObpNameBufferLookasideList;
    Prcb->PPLookasideList[LookasideNameBufferList].P = &ObpNameBufferLookasideList;

    /* Initialize the security descriptor cache */
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Initializing SD cache ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif
    ObpInitSdCache();
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: SD cache initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif

    /* Initialize the Default Event */
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Initializing default event ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif
    KeInitializeEvent(&ObpDefaultObject, NotificationEvent, TRUE);
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Default event initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif

    /* Initialize the Dos Device Map mutex */
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Initializing device map mutex ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif
    KeInitializeGuardedMutex(&ObpDeviceMapLock);
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Device map mutex initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif

    /* Setup default access for the system process */
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Setting up default access rights ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif
    PsGetCurrentProcess()->GrantedAccess = PROCESS_ALL_ACCESS;
    PsGetCurrentThread()->GrantedAccess = THREAD_ALL_ACCESS;
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Default access rights set ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif

    /* Setup the Object Reaper */
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Initializing object reaper work item ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif
    ExInitializeWorkItem(&ObpReaperWorkItem, ObpReapObject, NULL);

    /* Initialize default Quota block */
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Initializing quota system ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif
    PsInitializeQuotaSystem();
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Quota system initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif

    /* Create kernel handle table */
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Creating kernel handle table ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif
    PsGetCurrentProcess()->ObjectTable = ExCreateHandleTable(NULL);
    ObpKernelHandleTable = PsGetCurrentProcess()->ObjectTable;
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Kernel handle table created ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif

    /* Create the Type Type */
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Creating Type object type ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Type");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.ValidAccessMask = OBJECT_TYPE_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.MaintainTypeList = TRUE;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.GenericMapping = ObpTypeMapping;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(OBJECT_TYPE);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.DeleteProcedure = ObpDeleteObjectType;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &ObpTypeObjectType);
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Type object type created ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif

    /* Create the Directory Type */
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Creating Directory object type ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif
    RtlInitUnicodeString(&Name, L"Directory");
    ObjectTypeInitializer.PoolType = PagedPool;
    ObjectTypeInitializer.ValidAccessMask = DIRECTORY_ALL_ACCESS;
    ObjectTypeInitializer.CaseInsensitive = TRUE;
    ObjectTypeInitializer.MaintainTypeList = FALSE;
    ObjectTypeInitializer.GenericMapping = ObpDirectoryMapping;
    ObjectTypeInitializer.DeleteProcedure = NULL;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(OBJECT_DIRECTORY);
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &ObpDirectoryObjectType);
    ObpDirectoryObjectType->TypeInfo.ValidAccessMask &= ~SYNCHRONIZE;
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Directory object type created ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif

    /* Create 'symbolic link' object type */
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Creating SymbolicLink object type ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif
    RtlInitUnicodeString(&Name, L"SymbolicLink");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(OBJECT_SYMBOLIC_LINK);
    ObjectTypeInitializer.GenericMapping = ObpSymbolicLinkMapping;
    ObjectTypeInitializer.ValidAccessMask = SYMBOLIC_LINK_ALL_ACCESS;
    ObjectTypeInitializer.ParseProcedure = ObpParseSymbolicLink;
    ObjectTypeInitializer.DeleteProcedure = ObpDeleteSymbolicLink;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &ObpSymbolicLinkObjectType);
    ObpSymbolicLinkObjectType->TypeInfo.ValidAccessMask &= ~SYNCHRONIZE;
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: SymbolicLink object type created ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif

    /* Phase 0 initialization complete */
#ifdef _M_AMD64
    {
        const char msg[] = "*** OB: Phase 0 initialization complete! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM_PORT + 5) & 0x20) == 0); __outbyte(COM_PORT, *p++); }
    }
#endif
    ObpInitializationPhase++;
    return TRUE;

ObPostPhase0:

    /* Re-initialize lookaside lists */
    ObInit2();

    /* Initialize Object Types directory attributes */
    RtlInitUnicodeString(&Name, L"\\");
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               SePublicDefaultUnrestrictedSd);

    /* Create the directory */
    Status = NtCreateDirectoryObject(&Handle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Get a handle to it */
    Status = ObReferenceObjectByHandle(Handle,
                                       0,
                                       ObpDirectoryObjectType,
                                       KernelMode,
                                       (PVOID*)&ObpRootDirectoryObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Close the extra handle */
    Status = NtClose(Handle);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Create a custom security descriptor for the KernelObjects directory */
    Status = ObpCreateKernelObjectsSD(&KernelObjectsSD);
    if (!NT_SUCCESS(Status))
        return FALSE;

    /* Initialize the KernelObjects directory attributes */
    RtlInitUnicodeString(&Name, L"\\KernelObjects");
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               KernelObjectsSD);

    /* Create the directory */
    Status = NtCreateDirectoryObject(&Handle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    ExFreePoolWithTag(KernelObjectsSD, TAG_SD);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Close the extra handle */
    Status = NtClose(Handle);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Initialize ObjectTypes directory attributes */
    RtlInitUnicodeString(&Name, L"\\ObjectTypes");
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               NULL);

    /* Create the directory */
    Status = NtCreateDirectoryObject(&Handle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Get a handle to it */
    Status = ObReferenceObjectByHandle(Handle,
                                       0,
                                       ObpDirectoryObjectType,
                                       KernelMode,
                                       (PVOID*)&ObpTypeDirectoryObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Close the extra handle */
    Status = NtClose(Handle);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Initialize the lookup context and lock it */
    ObpInitializeLookupContext(&Context);
    ObpAcquireLookupContextLock(&Context, ObpTypeDirectoryObject);

    /* Loop the object types */
    ListHead = &ObpTypeObjectType->TypeList;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        /* Get the creator info from the list */
        CreatorInfo = CONTAINING_RECORD(NextEntry,
                                        OBJECT_HEADER_CREATOR_INFO,
                                        TypeList);

        /* Recover the header and the name header from the creator info */
        Header = (POBJECT_HEADER)(CreatorInfo + 1);
        NameInfo = OBJECT_HEADER_TO_NAME_INFO(Header);

        /* Make sure we have a name, and aren't inserted yet */
        if ((NameInfo) && !(NameInfo->Directory))
        {
            /* Do the initial lookup to setup the context */
            if (!ObpLookupEntryDirectory(ObpTypeDirectoryObject,
                                         &NameInfo->Name,
                                         OBJ_CASE_INSENSITIVE,
                                         FALSE,
                                         &Context))
            {
                /* Insert this object type */
                ObpInsertEntryDirectory(ObpTypeDirectoryObject,
                                        &Context,
                                        Header);
            }
        }

        /* Move to the next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Cleanup after lookup */
    ObpReleaseLookupContext(&Context);

    /* Initialize DOS Devices Directory and related Symbolic Links */
    Status = ObpCreateDosDevicesDirectory();
    if (!NT_SUCCESS(Status)) return FALSE;
    return TRUE;
}

/* EOF */
