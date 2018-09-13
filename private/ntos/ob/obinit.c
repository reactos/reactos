/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obinit.c

Abstract:

    Initialization module for the OB subcomponent of NTOS

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "obp.h"

//
//  Form some default access masks for the various object types
//

GENERIC_MAPPING ObpTypeMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    OBJECT_TYPE_ALL_ACCESS
};

GENERIC_MAPPING ObpDirectoryMapping = {
    STANDARD_RIGHTS_READ |
        DIRECTORY_QUERY |
        DIRECTORY_TRAVERSE,
    STANDARD_RIGHTS_WRITE |
        DIRECTORY_CREATE_OBJECT |
        DIRECTORY_CREATE_SUBDIRECTORY,
    STANDARD_RIGHTS_EXECUTE |
        DIRECTORY_QUERY |
        DIRECTORY_TRAVERSE,
    DIRECTORY_ALL_ACCESS
};

GENERIC_MAPPING ObpSymbolicLinkMapping = {
    STANDARD_RIGHTS_READ |
        SYMBOLIC_LINK_QUERY,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE |
        SYMBOLIC_LINK_QUERY,
    SYMBOLIC_LINK_ALL_ACCESS
};

//
//  Local procedure prototypes
//

NTSTATUS
ObpCreateDosDevicesDirectory (
    VOID
    );

NTSTATUS
ObpGetDosDevicesProtection (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    );

VOID
ObpFreeDosDevicesProtection (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,ObInitSystem)
#pragma alloc_text(INIT,ObpCreateDosDevicesDirectory)
#pragma alloc_text(INIT,ObpGetDosDevicesProtection)
#pragma alloc_text(INIT,ObpFreeDosDevicesProtection)
#pragma alloc_text(PAGE,ObKillProcess)
#endif

//
//  The default quota block is setup by obinitsystem
//

extern EPROCESS_QUOTA_BLOCK PspDefaultQuotaBlock;

//
//  This is really a global variable used to coordinate access to
//  the process object table.
//

KMUTANT ObpInitKillMutant;

//
//  CurrentControlSet values set by code in config\cmdat3.c at system load time
//  These are private variables within obinit.c
//

ULONG ObpProtectionMode;
ULONG ObpAuditBaseDirectories;
ULONG ObpAuditBaseObjects;

//
//  These are global variables
//

UNICODE_STRING ObpDosDevicesShortName;
ULARGE_INTEGER ObpDosDevicesShortNamePrefix;
ULARGE_INTEGER ObpDosDevicesShortNameRoot;
PDEVICE_MAP ObSystemDeviceMap;


BOOLEAN
ObInitSystem (
    VOID
    )

/*++

Routine Description:

    This function performs the system initialization for the object
    manager.  The object manager data structures are self describing
    with the exception of the root directory, the type object type and
    the directory object type.  The initialization code then constructs
    these objects by hand to get the ball rolling.

Arguments:

    None.

Return Value:

    TRUE if successful and FALSE if an error occurred.

    The following errors can occur:

    - insufficient memory

--*/

{
    USHORT CreateInfoMaxDepth;
    USHORT NameBufferMaxDepth;
    ULONG RegionSegmentSize;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING TypeTypeName;
    UNICODE_STRING SymbolicLinkTypeName;
    UNICODE_STRING DosDevicesDirectoryName;
    UNICODE_STRING DirectoryTypeName;
    UNICODE_STRING RootDirectoryName;
    UNICODE_STRING TypeDirectoryName;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE RootDirectoryHandle;
    HANDLE TypeDirectoryHandle;
    PLIST_ENTRY Next, Head;
    POBJECT_HEADER ObjectTypeHeader;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POBJECT_HEADER_NAME_INFO NameInfo;
    MM_SYSTEMSIZE SystemSize;
    SECURITY_DESCRIPTOR AuditSd;
    PSECURITY_DESCRIPTOR EffectiveSd;
    PACL    AuditAllAcl;
    UCHAR   AuditAllBuffer[250];  // Ample room for the ACL
    ULONG   AuditAllLength;
    PACE_HEADER Ace;
    PNPAGED_LOOKASIDE_LIST Lookaside;
    ULONG Index;
    PKPRCB Prcb;

    //
    //  Determine the the size of the object creation and the name buffer
    //  lookaside lists.
    //

    SystemSize = MmQuerySystemSize();

    if (SystemSize == MmLargeSystem) {

        if (MmIsThisAnNtAsSystem()) {

            CreateInfoMaxDepth = 64;
            NameBufferMaxDepth = 32;

        } else {

            CreateInfoMaxDepth = 32;
            NameBufferMaxDepth = 16;
        }

    } else {

        CreateInfoMaxDepth = 3;
        NameBufferMaxDepth = 3;
    }

    //
    //  PHASE 0 Initialization
    //

    if (InitializationPhase == 0) {

        //
        //  Initialize the object creation lookaside list.
        //

        ExInitializeNPagedLookasideList( &ObpCreateInfoLookasideList,
                                         NULL,
                                         NULL,
                                         0,
                                         sizeof(OBJECT_CREATE_INFORMATION),
                                         'iCbO',
                                         CreateInfoMaxDepth );

        //
        //  Initialize the name buffer lookaside list.
        //

        ExInitializeNPagedLookasideList( &ObpNameBufferLookasideList,
                                         NULL,
                                         NULL,
                                         0,
                                         OBJECT_NAME_BUFFER_SIZE,
                                         'mNbO',
                                         NameBufferMaxDepth );

        //
        //  Initialize the system create info and name buffer lookaside lists
        //  for the current processor.
        //
        // N.B. Temporarily during the initialization of the system both
        //      lookaside list pointers in the processor block point to
        //      the same lookaside list structure. Later in initialization
        //      another lookaside list structure is allocated and filled
        //      for the per processor list.
        //

        Prcb = KeGetCurrentPrcb();
        Prcb->PPLookasideList[LookasideCreateInfoList].L = &ObpCreateInfoLookasideList;
        Prcb->PPLookasideList[LookasideCreateInfoList].P = &ObpCreateInfoLookasideList;
        Prcb->PPLookasideList[LookasideNameBufferList].L = &ObpNameBufferLookasideList;
        Prcb->PPLookasideList[LookasideNameBufferList].P = &ObpNameBufferLookasideList;

        //
        //  Initialize the object removal queue listhead.
        //

        ObpRemoveObjectQueue = NULL;

        //
        //  Initialize security descriptor cache
        //

        ObpInitSecurityDescriptorCache();

        KeInitializeMutant( &ObpInitKillMutant, FALSE );
        KeInitializeEvent( &ObpDefaultObject, NotificationEvent, TRUE );
        KeInitializeSpinLock( &ObpLock );
        PsGetCurrentProcess()->GrantedAccess = PROCESS_ALL_ACCESS;
        PsGetCurrentThread()->GrantedAccess = THREAD_ALL_ACCESS;

        KeInitializeSpinLock( &ObpDeviceMapLock );

        //
        //  Initialize the quota block and have the eprocess structure
        //  point to it.
        //

        KeInitializeSpinLock(&PspDefaultQuotaBlock.QuotaLock);
        PspDefaultQuotaBlock.ReferenceCount = 1;
        PspDefaultQuotaBlock.QuotaPoolLimit[PagedPool] = (ULONG)-1;
        PspDefaultQuotaBlock.QuotaPoolLimit[NonPagedPool] = (ULONG)-1;
        PspDefaultQuotaBlock.PagefileLimit = (ULONG)-1;

        PsGetCurrentProcess()->QuotaBlock = &PspDefaultQuotaBlock;

        //
        //  Initialize the handle table for the system process and also the global
        //  kernel handle table
        //

        PsGetCurrentProcess()->ObjectTable = ExCreateHandleTable( NULL );
        ObpKernelHandleTable = ExCreateHandleTable( NULL );

        //
        //  Create an object type for the "Type" object.  This is the start of
        //  of the object types and goes in the ObpTypeDirectoryObject.
        //

        RtlZeroMemory( &ObjectTypeInitializer, sizeof( ObjectTypeInitializer ) );
        ObjectTypeInitializer.Length = sizeof( ObjectTypeInitializer );
        ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
        ObjectTypeInitializer.PoolType = NonPagedPool;

        RtlInitUnicodeString( &TypeTypeName, L"Type" );
        ObjectTypeInitializer.ValidAccessMask = OBJECT_TYPE_ALL_ACCESS;
        ObjectTypeInitializer.GenericMapping = ObpTypeMapping;
        ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( OBJECT_TYPE );
        ObjectTypeInitializer.MaintainTypeList = TRUE;
        ObjectTypeInitializer.UseDefaultObject = TRUE;
        ObCreateObjectType( &TypeTypeName,
                            &ObjectTypeInitializer,
                            (PSECURITY_DESCRIPTOR)NULL,
                            &ObpTypeObjectType );

        //
        //  Create the object type for the "Directory" object.
        //

        RtlInitUnicodeString( &DirectoryTypeName, L"Directory" );
        ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( OBJECT_DIRECTORY );
        ObjectTypeInitializer.ValidAccessMask = DIRECTORY_ALL_ACCESS;
        ObjectTypeInitializer.GenericMapping = ObpDirectoryMapping;
        ObjectTypeInitializer.UseDefaultObject = FALSE;
        ObjectTypeInitializer.MaintainTypeList = FALSE;
        ObCreateObjectType( &DirectoryTypeName,
                            &ObjectTypeInitializer,
                            (PSECURITY_DESCRIPTOR)NULL,
                            &ObpDirectoryObjectType );

        //
        //  Create the object type for the "SymbolicLink" object.
        //

        RtlInitUnicodeString( &SymbolicLinkTypeName, L"SymbolicLink" );
        ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( OBJECT_SYMBOLIC_LINK );
        ObjectTypeInitializer.ValidAccessMask = SYMBOLIC_LINK_ALL_ACCESS;
        ObjectTypeInitializer.GenericMapping = ObpSymbolicLinkMapping;
        ObjectTypeInitializer.DeleteProcedure = ObpDeleteSymbolicLink;
        ObjectTypeInitializer.ParseProcedure = ObpParseSymbolicLink;
        ObCreateObjectType( &SymbolicLinkTypeName,
                            &ObjectTypeInitializer,
                            (PSECURITY_DESCRIPTOR)NULL,
                            &ObpSymbolicLinkObjectType );

        //
        //  Initialize the resource that protects the object name space directory structure
        //

        ExInitializeResourceLite( &ObpRootDirectoryMutex );

#if i386 && !FPO

        //
        //  Initialize the cached granted access structure.  These variables are used
        //  in place of the access mask in the object table entry.
        //

        ObpCurCachedGrantedAccessIndex = 0;
        ObpMaxCachedGrantedAccessIndex = PAGE_SIZE / sizeof( ACCESS_MASK );
        ObpCachedGrantedAccesses = ExAllocatePoolWithTag( NonPagedPool, PAGE_SIZE, 'gAbO' );

#endif // i386 && !FPO

    } // End of Phase 0 Initialization


    //
    //  PHASE 1 Initialization
    //

    if (InitializationPhase == 1) {

        //
        //  Initialize the per processor nonpaged lookaside lists and descriptors.
        //

        for (Index = 0; Index < (ULONG)KeNumberProcessors; Index += 1) {
            Prcb = KiProcessorBlock[Index];

            //
            //  Initialize the create information per processor lookaside pointers.
            //

            Prcb->PPLookasideList[LookasideCreateInfoList].L = &ObpCreateInfoLookasideList;
            Lookaside = (PNPAGED_LOOKASIDE_LIST)ExAllocatePoolWithTag( NonPagedPool,
                                                                       sizeof(NPAGED_LOOKASIDE_LIST),
                                                                       'ICbO');

            if (Lookaside != NULL) {
                ExInitializeNPagedLookasideList( Lookaside,
                                                 NULL,
                                                 NULL,
                                                 0,
                                                 sizeof(OBJECT_CREATE_INFORMATION),
                                                 'ICbO',
                                                 CreateInfoMaxDepth );

            } else {
                Lookaside = &ObpCreateInfoLookasideList;
            }

            Prcb->PPLookasideList[LookasideCreateInfoList].P = Lookaside;

            //
            //  Initialize the name buffer per processor lookaside pointers.
            //

            Prcb->PPLookasideList[LookasideNameBufferList].L = &ObpNameBufferLookasideList;
            Lookaside = (PNPAGED_LOOKASIDE_LIST)ExAllocatePoolWithTag( NonPagedPool,
                                                                       sizeof(NPAGED_LOOKASIDE_LIST),
                                                                       'MNbO');

            if (Lookaside != NULL) {
                ExInitializeNPagedLookasideList( Lookaside,
                                                 NULL,
                                                 NULL,
                                                 0,
                                                 OBJECT_NAME_BUFFER_SIZE,
                                                 'MNbO',
                                                 NameBufferMaxDepth);

            } else {
                Lookaside = &ObpNameBufferLookasideList;
            }

            Prcb->PPLookasideList[LookasideNameBufferList].P = Lookaside;
        }

        EffectiveSd = SePublicDefaultUnrestrictedSd;

        //
        //  This code is only executed if base auditing is turned on.
        //

        if ((ObpAuditBaseDirectories != 0) || (ObpAuditBaseObjects != 0)) {

            //
            //  build an SACL to audit
            //

            AuditAllAcl = (PACL)AuditAllBuffer;
            AuditAllLength = (ULONG)sizeof(ACL) +
                               ((ULONG)sizeof(SYSTEM_AUDIT_ACE)) +
                               SeLengthSid(SeWorldSid);

            ASSERT( sizeof(AuditAllBuffer)   >   AuditAllLength );

            Status = RtlCreateAcl( AuditAllAcl, AuditAllLength, ACL_REVISION2);

            ASSERT( NT_SUCCESS(Status) );

            Status = RtlAddAuditAccessAce ( AuditAllAcl,
                                            ACL_REVISION2,
                                            GENERIC_ALL,
                                            SeWorldSid,
                                            TRUE,  TRUE ); //Audit success and failure
            ASSERT( NT_SUCCESS(Status) );

            Status = RtlGetAce( AuditAllAcl, 0,  (PVOID)&Ace );

            ASSERT( NT_SUCCESS(Status) );

            if (ObpAuditBaseDirectories != 0) {

                Ace->AceFlags |= (CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
            }

            if (ObpAuditBaseObjects != 0) {

                Ace->AceFlags |= (OBJECT_INHERIT_ACE    |
                                  CONTAINER_INHERIT_ACE |
                                  INHERIT_ONLY_ACE);
            }

            //
            //  Now create a security descriptor that looks just like
            //  the public default, but has auditing in it as well.
            //

            EffectiveSd = (PSECURITY_DESCRIPTOR)&AuditSd;
            Status = RtlCreateSecurityDescriptor( EffectiveSd,
                                                  SECURITY_DESCRIPTOR_REVISION1 );

            ASSERT( NT_SUCCESS(Status) );

            Status = RtlSetDaclSecurityDescriptor( EffectiveSd,
                                                   TRUE,        // DaclPresent
                                                   SePublicDefaultUnrestrictedDacl,
                                                   FALSE );     // DaclDefaulted

            ASSERT( NT_SUCCESS(Status) );

            Status = RtlSetSaclSecurityDescriptor( EffectiveSd,
                                                   TRUE,        // DaclPresent
                                                   AuditAllAcl,
                                                   FALSE );     // DaclDefaulted

            ASSERT( NT_SUCCESS(Status) );
        }

        //
        //  We only need to use the EffectiveSd on the root.  The SACL
        //  will be inherited by all other objects.
        //

        //
        //  Create an directory object for the root directory
        //

        RtlInitUnicodeString( &RootDirectoryName, L"\\" );

        InitializeObjectAttributes( &ObjectAttributes,
                                    &RootDirectoryName,
                                    OBJ_CASE_INSENSITIVE |
                                    OBJ_PERMANENT,
                                    NULL,
                                    EffectiveSd );

        Status = NtCreateDirectoryObject( &RootDirectoryHandle,
                                          DIRECTORY_ALL_ACCESS,
                                          &ObjectAttributes );

        if (!NT_SUCCESS( Status )) {

            return( FALSE );
        }

        Status = ObReferenceObjectByHandle( RootDirectoryHandle,
                                            0,
                                            ObpDirectoryObjectType,
                                            KernelMode,
                                            (PVOID *)&ObpRootDirectoryObject,
                                            NULL );

        if (!NT_SUCCESS( Status )) {

            return( FALSE );
        }

        Status = NtClose( RootDirectoryHandle );

        if (!NT_SUCCESS( Status )) {

            return( FALSE );
        }

        //
        //  Create an directory object for the directory of object types
        //

        RtlInitUnicodeString( &TypeDirectoryName, L"\\ObjectTypes" );

        InitializeObjectAttributes( &ObjectAttributes,
                                    &TypeDirectoryName,
                                    OBJ_CASE_INSENSITIVE |
                                    OBJ_PERMANENT,
                                    NULL,
                                    NULL );

        Status = NtCreateDirectoryObject( &TypeDirectoryHandle,
                                          DIRECTORY_ALL_ACCESS,
                                          &ObjectAttributes );

        if (!NT_SUCCESS( Status )) {

            return( FALSE );
        }

        Status = ObReferenceObjectByHandle( TypeDirectoryHandle,
                                            0,
                                            ObpDirectoryObjectType,
                                            KernelMode,
                                            (PVOID *)&ObpTypeDirectoryObject,
                                            NULL );

        if (!NT_SUCCESS( Status )) {

            return( FALSE );
        }

        Status = NtClose( TypeDirectoryHandle );

        if (!NT_SUCCESS( Status )) {

            return( FALSE );
        }

        //
        //  Lock the object directory name space
        //

        ObpEnterRootDirectoryMutex();

        //
        //  For every object type that has already been created we will
        //  insert it in the type directory.  We do this looking down the
        //  linked list of type objects and for every one that has a name
        //  and isn't already in a directory we'll look the name up and
        //  then put it in the directory.  Be sure to skip the first
        //  entry in the type object types lists.
        //

        Head = &ObpTypeObjectType->TypeList;
        Next = Head->Flink;

        while (Next != Head) {

            //
            //  Right after the creator info is the object header.  Get\
            //  the object header and then see if there is a name
            //

            CreatorInfo = CONTAINING_RECORD( Next,
                                             OBJECT_HEADER_CREATOR_INFO,
                                             TypeList );

            ObjectTypeHeader = (POBJECT_HEADER)(CreatorInfo+1);

            NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectTypeHeader );

            //
            //  Check if we have a name and we're not in a directory
            //


            if ((NameInfo != NULL) && (NameInfo->Directory == NULL)) {

                if (!ObpLookupDirectoryEntry( ObpTypeDirectoryObject,
                                              &NameInfo->Name,
                                              OBJ_CASE_INSENSITIVE )) {

                    ObpInsertDirectoryEntry( ObpTypeDirectoryObject,
                                             &ObjectTypeHeader->Body );
                }
            }

            Next = Next->Flink;
        }

        //
        //  Unlock the object directory name space
        //

        ObpLeaveRootDirectoryMutex();

        //
        //  Create \DosDevices object directory for drive letters and Win32 device names
        //

        Status = ObpCreateDosDevicesDirectory();

        if (!NT_SUCCESS( Status )) {

            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
ObDupHandleProcedure (
    PEPROCESS Process,
    PHANDLE_TABLE_ENTRY ObjectTableEntry
    )

/*++

Routine Description:

    This is the worker routine for ExDupHandleTable and
    is invoked via ObInitProcess.

Arguments:

    Process - Supplies a pointer to the new process

    ObjectTableEntry - Supplies a pointer to the newly
        created handle table entry

Return Value:

    TRUE if the item can be inserted in the new table
        and FALSE otherwise

--*/

{
    NTSTATUS Status;
    POBJECT_HEADER ObjectHeader;
    PVOID Object;
    ACCESS_STATE AccessState;

    //
    //  If the object table should not inherited then return false
    //

    if (!(ObjectTableEntry->ObAttributes & OBJ_INHERIT)) {

        return( FALSE );
    }

    //
    //  Get a pointer to the object header and body
    //

    ObjectHeader = (POBJECT_HEADER)(((ULONG_PTR)(ObjectTableEntry->Object)) & ~OBJ_HANDLE_ATTRIBUTES);

    Object = &ObjectHeader->Body;

    //
    //  If we are tracing the call stacks for cached security indices then we do got a
    //  translation to do otherwise the table entry contains straight away the granted
    //  access mask
    //

#if i386 && !FPO

    if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB) {

        AccessState.PreviouslyGrantedAccess = ObpTranslateGrantedAccessIndex( ObjectTableEntry->GrantedAccessIndex );

    } else {

        AccessState.PreviouslyGrantedAccess = ObjectTableEntry->GrantedAccess;
    }

#else

    AccessState.PreviouslyGrantedAccess = ObjectTableEntry->GrantedAccess;

#endif // i386 && !FPO

    //
    //  Increment the handle count on the object because we've just added
    //  another handle to it.
    //

    Status = ObpIncrementHandleCount( ObInheritHandle,
                                      Process,
                                      Object,
                                      ObjectHeader->Type,
                                      &AccessState,
                                      KernelMode,     // BUGBUG this is probably wrong
                                      0 );

    if (!NT_SUCCESS( Status )) {

        return( FALSE );
    }

    //
    //  Likewise we need to increment the pointer count to the object
    //

    ObpIncrPointerCount( ObjectHeader );

    return( TRUE );
}


BOOLEAN
ObAuditInheritedHandleProcedure (
    IN PHANDLE_TABLE_ENTRY ObjectTableEntry,
    IN HANDLE HandleId,
    IN PVOID EnumParameter
    )

/*++

Routine Description:

    ExEnumHandleTable worker routine to generate audits when handles are
    inherited.  An audit is generated if the handle attributes indicate
    that the handle is to be audited on close.

Arguments:

    ObjectTableEntry - Points to the handle table entry of interest.

    HandleId - Supplies the handle.

    EnumParameter - Supplies information about the source and target processes.

Return Value:

    FALSE, which tells ExEnumHandleTable to continue iterating through the
    handle table.

--*/

{
    PSE_PROCESS_AUDIT_INFO ProcessAuditInfo = EnumParameter;

    //
    //  Check if we have to do an audit
    //

    if (!(ObjectTableEntry->ObAttributes & OBJ_AUDIT_OBJECT_CLOSE)) {

        return( FALSE );
    }

    //
    //  Do the audit then return for more
    //

    SeAuditHandleDuplication( HandleId,
                              HandleId,
                              ProcessAuditInfo->Parent,
                              ProcessAuditInfo->Process );

    return( FALSE );
}



NTSTATUS
ObInitProcess (
    PEPROCESS ParentProcess OPTIONAL,
    PEPROCESS NewProcess
    )

/*++

Routine Description:

    This function initializes a process object table.  If the ParentProcess
    is specified, then all object handles with the OBJ_INHERIT attribute are
    copied from the parent object table to the new process' object table.
    The HandleCount field of each object copied is incremented by one.  Both
    object table mutexes remained locked for the duration of the copy
    operation.

Arguments:

    ParentProcess - optional pointer to a process object that is the
        parent process to inherit object handles from.

    NewProcess - pointer to the process object being initialized.

Return Value:

    Status code.

    The following errors can occur:

    - insufficient memory

--*/

{
    PHANDLE_TABLE OldObjectTable;
    PHANDLE_TABLE NewObjectTable;
    ULONG PoolCharges[ MaxPoolType ];
    SE_PROCESS_AUDIT_INFO ProcessAuditInfo;

    RtlZeroMemory( PoolCharges, sizeof( PoolCharges ) );

    //
    //  If we have a parent process then we need to lock it down
    //  check that it is not going away and then make a copy
    //  of its handle table.  If there isn't a parent then
    //  we'll start with an empty handle table.
    //

    if (ARGUMENT_PRESENT( ParentProcess )) {

        KeEnterCriticalRegion();
        KeWaitForSingleObject( &ObpInitKillMutant,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        OldObjectTable = ParentProcess->ObjectTable;

        if ( !OldObjectTable ) {

            KeReleaseMutant( &ObpInitKillMutant, 0, FALSE, FALSE );
            KeLeaveCriticalRegion();

            return STATUS_PROCESS_IS_TERMINATING;
        }

        NewObjectTable = ExDupHandleTable( NewProcess,
                                           OldObjectTable,
                                           ObDupHandleProcedure );

    } else {

        OldObjectTable = NULL;
        NewObjectTable = ExCreateHandleTable( NewProcess );
    }

    //
    //  Check that we really have a new handle table otherwise
    //  we must be out of resources
    //

    if (NewObjectTable) {

        //
        //  Set the new processes object table and if we are
        //  auditing then enumerate the new table calling
        //  the audit procedure
        //

        NewProcess->ObjectTable = NewObjectTable;

        if ( SeDetailedAuditing ) {

            ProcessAuditInfo.Process = NewProcess;
            ProcessAuditInfo.Parent  = ParentProcess;

            ExEnumHandleTable( NewObjectTable,
                               ObAuditInheritedHandleProcedure,
                               (PVOID)&ProcessAuditInfo,
                               (PHANDLE)NULL );
        }

        //
        //  Free the old table if it exists and then
        //  return our caller
        //

        if ( OldObjectTable ) {

            KeReleaseMutant( &ObpInitKillMutant, 0, FALSE, FALSE );
            KeLeaveCriticalRegion();
        }

        return( STATUS_SUCCESS );

    } else {

        //
        //  We're out of resources to null out the new object table field,
        //  unlock the old object table, and tell our caller that this
        //  didn't work
        //

        NewProcess->ObjectTable = NULL;

        if ( OldObjectTable ) {

            KeReleaseMutant( &ObpInitKillMutant, 0, FALSE, FALSE );
            KeLeaveCriticalRegion();
        }

        return( STATUS_INSUFFICIENT_RESOURCES );
    }
}


VOID
ObInitProcess2 (
    PEPROCESS NewProcess
    )

/*++

Routine Description:

    This function is called after an image file has been mapped into the address
    space of a newly created process.  Allows the object manager to set LIFO/FIFO
    ordering for handle allocation based on the SubSystemVersion number in the
    image.

Arguments:

    NewProcess - pointer to the process object being initialized.

Return Value:

    None.

--*/

{
    //
    //  Set LIFO ordering of handles for images <= SubSystemVersion 3.50
    //

    if (NewProcess->ObjectTable) {

        ExSetHandleTableOrder( NewProcess->ObjectTable, (BOOLEAN)(NewProcess->SubSystemVersion <= 0x332) );
    }

    return;
}


VOID
ObDestroyHandleProcedure (
    IN HANDLE HandleIndex
    )

/*++

Routine Description:

    This function is used to close a handle but takes as input a
    handle table index that it first translates to an handle
    before calling close.  Note that the handle index is really
    just the offset within the handle table entries.

Arguments:

    HandleIndex - Supplies a handle index for the handle being closed.

Return Value:

    None.

--*/

{
    ZwClose( HandleIndex );

    return;
}


VOID
ObKillProcess (
    BOOLEAN AcquireLock,
    PEPROCESS Process
    )
/*++

Routine Description:

    This function is called whenever a process is destroyed.  It loops over
    the process' object table and closes all the handles.

Arguments:

    AcquireLock - TRUE if there are other pointers to this process and therefore
        this operation needs to be synchronized.  False if this is being called
        from the Process delete routine and therefore this is the only pointer
        to the process.

    Process - Pointer to the process that is being destroyed.

Return Value:

    None.

--*/

{
    PVOID ObjectTable;
    BOOLEAN PreviousIOHardError;

    PAGED_CODE();

    ObpValidateIrql( "ObKillProcess" );

    //
    //  Check if we need to get the lock
    //

    if (AcquireLock) {

        KeEnterCriticalRegion();

        KeWaitForSingleObject( &ObpInitKillMutant,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );
    }

    //
    //  If the process does NOT have an object table, return
    //

    ObjectTable = Process->ObjectTable;

    if (ObjectTable != NULL) {
        
        PreviousIOHardError = IoSetThreadHardErrorMode(FALSE); 
        
        //
        //  For each valid entry in the object table, close the handle
        //  that points to that entry.
        //

        ExDestroyHandleTable( ObjectTable, ObDestroyHandleProcedure );

        Process->ObjectTable = NULL;
        
        IoSetThreadHardErrorMode( PreviousIOHardError ); 
    }

    //
    //  Release the lock
    //

    if (AcquireLock) {

        KeReleaseMutant( &ObpInitKillMutant, 0, FALSE, FALSE );

        KeLeaveCriticalRegion();
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  The following structure is only used by the enumeration routine
//  and the callback.  It provides context for the comparison of
//  the objects.
//

typedef struct _OBP_FIND_HANDLE_DATA {

    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    POBJECT_HANDLE_INFORMATION HandleInformation;

} OBP_FIND_HANDLE_DATA, *POBP_FIND_HANDLE_DATA;

BOOLEAN
ObpEnumFindHandleProcedure (
    PHANDLE_TABLE_ENTRY ObjectTableEntry,
    HANDLE HandleId,
    PVOID EnumParameter
    )

/*++

Routine Description:

    Call back routine when enumerating an object table to find a handle
    for a particular object

Arguments:

    HandleTableEntry - Supplies a pointer to the handle table entry
        being examined.

    HandleId - Supplies the actual handle value for the preceding entry

    EnumParameter - Supplies context for the matching.

Return Value:

    Returns TRUE if a match is found and the enumeration should stop.  Returns FALSE
    otherwise, so the enumeration will continue.

--*/

{
    POBJECT_HEADER ObjectHeader;
    ACCESS_MASK GrantedAccess;
    ULONG HandleAttributes;
    POBP_FIND_HANDLE_DATA MatchCriteria = EnumParameter;

    //
    //  Get the object header from the table entry and see if
    //  object types and headers match if specified.
    //

    ObjectHeader = (POBJECT_HEADER)((ULONG_PTR)ObjectTableEntry->Object & ~OBJ_HANDLE_ATTRIBUTES);

    if ((MatchCriteria->ObjectHeader != NULL) &&
        (MatchCriteria->ObjectHeader != ObjectHeader)) {

        return FALSE;
    }

    if ((MatchCriteria->ObjectType != NULL) &&
        (MatchCriteria->ObjectType != ObjectHeader->Type)) {

        return FALSE;
    }

    //
    //  Check if we have handle information that we need to compare
    //

    if (ARGUMENT_PRESENT( MatchCriteria->HandleInformation )) {

        //
        //  If we are tracing the call stacks for cached security indices then we do got a
        //  translation to do otherwise the table entry contains straight away the granted
        //  access mask
        //

#if i386 && !FPO

        if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB) {

            GrantedAccess = ObpTranslateGrantedAccessIndex( ObjectTableEntry->GrantedAccessIndex );

        } else {

            GrantedAccess = ObjectTableEntry->GrantedAccess;
        }
#else

        GrantedAccess = ObjectTableEntry->GrantedAccess;

#endif // i386 && !FPO

        //
        //  Get the handle attributes from table entry and see if the
        //  fields match.  If they do not match we will return false to
        //  continue the search.
        //

        HandleAttributes = (ULONG)((ULONG_PTR)ObjectTableEntry->Object & OBJ_HANDLE_ATTRIBUTES);

        if (MatchCriteria->HandleInformation->HandleAttributes != HandleAttributes ||
            MatchCriteria->HandleInformation->GrantedAccess != GrantedAccess ) {

            return FALSE;
        }
    }

    //
    //  We found something that matches our criteria so return true to
    //  our caller to stop the enumeration
    //

    return TRUE;
}


BOOLEAN
ObFindHandleForObject (
    IN PEPROCESS Process,
    IN PVOID Object OPTIONAL,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL,
    OUT PHANDLE Handle
    )

/*++

Routine Description:

    This routine searches the handle table for the specified process,
    looking for a handle table entry that matches the passed parameters.
    If an an Object pointer is specified it must match.  If an
    ObjectType is specified it must match.  If HandleInformation is
    specified, then both the HandleAttributes and GrantedAccess mask
    must match.  If all three match parameters are NULL, then will
    match the first allocated handle for the specified process that
    matches the specified object pointer.

Arguments:

    Process - Specifies the process whose object table is to be searched.

    Object - Specifies the object pointer to look for.

    ObjectType - Specifies the object type to look for.

    HandleInformation - Specifies additional match criteria to look for.

    Handle - Specifies the location to receive the handle value whose handle
        entry matches the supplied object pointer and optional match criteria.

Return Value:

    TRUE if a match was found and FALSE otherwise.

--*/

{
    HANDLE_TABLE_ENTRY ObjectTableEntry;
    OBP_FIND_HANDLE_DATA EnumParameter;
    BOOLEAN Result;

    Result = FALSE;

    //
    //  Lock the object object name space
    //

    KeEnterCriticalRegion();

    KeWaitForSingleObject( &ObpInitKillMutant,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );

    //
    //  We only do the work if the process has an object table meaning
    //  it isn't going away
    //

    if (Process->ObjectTable != NULL) {

        //
        //  Set the match parameters that our caller supplied
        //

        if (ARGUMENT_PRESENT( Object )) {

            EnumParameter.ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

        } else {

            EnumParameter.ObjectHeader = NULL;
        }

        EnumParameter.ObjectType = ObjectType;
        EnumParameter.HandleInformation = HandleInformation;

        //
        //  Call the routine the enumerate the object table, this will
        //  return true if we get match.  The enumeration routine really
        //  returns a index into the object table entries we need to
        //  translate it to a real handle before returning.
        //

        if (ExEnumHandleTable( Process->ObjectTable,
                               ObpEnumFindHandleProcedure,
                               &EnumParameter,
                               Handle )) {

            Result = TRUE;
        }
    }

    //
    //  Unlock the object name space and return to our caller
    //

    KeReleaseMutant( &ObpInitKillMutant, 0, FALSE, FALSE );

    KeLeaveCriticalRegion();

    return Result;
}


//
//  Local support routine
//

NTSTATUS
ObpCreateDosDevicesDirectory (
    VOID
    )

/*++

Routine Description:

    This routine creates the directory object for the dos devices and sets
    the device map for the system process.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS or an appropriate error

--*/

{
    NTSTATUS Status;
    UNICODE_STRING NameString;
    UNICODE_STRING LinkNameString;
    UNICODE_STRING TargetString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE DirectoryHandle;
    HANDLE SymbolicLinkHandle;
    SECURITY_DESCRIPTOR DosDevicesSD;

    //
    //  Create the security descriptor to use for the \?? directory
    //

    Status = ObpGetDosDevicesProtection( &DosDevicesSD );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    //
    //  Create the root directory object for the \?? directory.
    //

    RtlInitUnicodeString( &NameString, L"\\??" );

    InitializeObjectAttributes( &ObjectAttributes,
                                &NameString,
                                OBJ_PERMANENT,
                                (HANDLE) NULL,
                                &DosDevicesSD );

    Status = NtCreateDirectoryObject( &DirectoryHandle,
                                      DIRECTORY_ALL_ACCESS,
                                      &ObjectAttributes );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    //
    //  Create a device map that will control this directory.  It will be
    //  stored in the each EPROCESS for use by ObpLookupObjectName when
    //  translating names that begin with \??\
    //

    Status = ObSetDeviceMap( NULL, DirectoryHandle );


    //
    //  Now create a symbolic link, \??\GLOBALROOT, that points to \
    //  WorkStation service needs some mechanism to access a session specific
    //  DosDevicesDirectory. DosPathToSessionPath API will take a DosPath
    //  e.g (C:) and convert it into session specific path
    //  (e.g GLOBALROOT\Sessions\6\DosDevices\C:). The GLOBALROOT symbolic
    //  link is used to escape out of the current process's DosDevices directory
    //

    RtlInitUnicodeString( &LinkNameString, L"GLOBALROOT" );
    RtlInitUnicodeString( &TargetString, L"" );

    InitializeObjectAttributes( &ObjectAttributes,
                                &LinkNameString,
                                OBJ_PERMANENT,
                                DirectoryHandle,
                                &DosDevicesSD );

    Status = NtCreateSymbolicLinkObject( &SymbolicLinkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &ObjectAttributes,
                                         &TargetString );

    if (NT_SUCCESS( Status )) {

        NtClose( SymbolicLinkHandle );
    }

    //
    //  Create a symbolic link, \??\Global, that points to \??
    //  Drivers loaded dynamically create the symbolic link in the global
    //  DosDevices directory. User mode components need some way to access this
    //  symbolic link in the global dosdevices directory. The Global symbolic
    //  link is used to escape out of the current sessions's DosDevices directory
    //  and use the global dosdevices directory. e.g CreateFile("\\\\.\\Global\\NMDev"..);
    //

    RtlInitUnicodeString( &LinkNameString, L"Global" );
    RtlInitUnicodeString( &TargetString, L"\\??" );

    InitializeObjectAttributes( &ObjectAttributes,
                                &LinkNameString,
                                OBJ_PERMANENT,
                                DirectoryHandle,
                                &DosDevicesSD );

    Status = NtCreateSymbolicLinkObject( &SymbolicLinkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &ObjectAttributes,
                                         &TargetString );

    if (NT_SUCCESS( Status )) {

        NtClose( SymbolicLinkHandle );
    }


    NtClose( DirectoryHandle );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    //
    //  Now copy the \?? string to a ULONGLONG aligned global variable
    //  for use by ObpLookupObjectName for quick comparisons.
    //

    ObpDosDevicesShortName.Buffer = (PWSTR)&ObpDosDevicesShortNamePrefix.QuadPart;
    ObpDosDevicesShortName.Length = 0;
    ObpDosDevicesShortName.MaximumLength = sizeof( ObpDosDevicesShortNamePrefix );

    RtlCopyUnicodeString( &ObpDosDevicesShortName, &NameString );

    ObpDosDevicesShortName.Buffer[ 3 ] = UNICODE_NULL;
    ObpDosDevicesShortNameRoot.QuadPart = ObpDosDevicesShortNamePrefix.QuadPart;

    //
    //  Now create a symbolic link, \DosDevices, that points to \??
    //  for backwards compatibility with old drivers that use the old
    //  name.
    //

    RtlCreateUnicodeString( &NameString, L"\\DosDevices" );

    InitializeObjectAttributes( &ObjectAttributes,
                                &NameString,
                                OBJ_PERMANENT,
                                (HANDLE) NULL,
                                &DosDevicesSD );

    Status = NtCreateSymbolicLinkObject( &SymbolicLinkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &ObjectAttributes,
                                         &ObpDosDevicesShortName );

    if (NT_SUCCESS( Status )) {

        NtClose( SymbolicLinkHandle );
    }

    //
    //  Finish setting up the global variable for ObpLookupObjectName
    //

    ObpDosDevicesShortName.Buffer[ 3 ] = OBJ_NAME_PATH_SEPARATOR;
    ObpDosDevicesShortName.Length += sizeof( OBJ_NAME_PATH_SEPARATOR );

    //
    //  All done with the security descriptor for \??
    //

    ObpFreeDosDevicesProtection( &DosDevicesSD );

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
ObpGetDosDevicesProtection (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    This routine builds a security descriptor for use in creating
    the \DosDevices object directory.  The protection of \DosDevices
    must establish inheritable protection which will dictate how
    dos devices created via the DefineDosDevice() and
    IoCreateUnprotectedSymbolicLink() apis can be managed.

    The protection assigned is dependent upon an administrable registry
    key:

        Key: \hkey_local_machine\System\CurrentControlSet\Control\Session Manager
        Value: [REG_DWORD] ProtectionMode

    If this value is 0x1, then

            Administrators may control all Dos devices,
            Anyone may create new Dos devices (such as net drives
                or additional printers),
            Anyone may use any Dos device,
            The creator of a Dos device may delete it.
            Note that this protects system-defined LPTs and COMs so that only
                administrators may redirect them.  However, anyone may add
                additional printers and direct them to wherever they would
                like.

           This is achieved with the following protection for the DosDevices
           Directory object:

                    Grant:  World:   Execute | Read         (No Inherit)
                    Grant:  System:  All Access             (No Inherit)
                    Grant:  World:   Execute                (Inherit Only)
                    Grant:  Admins:  All Access             (Inherit Only)
                    Grant:  System:  All Access             (Inherit Only)
                    Grant:  Owner:   All Access             (Inherit Only)

    If this value is 0x0, or not present, then

            Administrators may control all Dos devices,
            Anyone may create new Dos devices (such as net drives
                or additional printers),
            Anyone may use any Dos device,
            Anyone may delete Dos devices created with either DefineDosDevice()
                or IoCreateUnprotectedSymbolicLink().  This is how network drives
                and LPTs are created (but not COMs).

           This is achieved with the following protection for the DosDevices
           Directory object:

                    Grant:  World:   Execute | Read | Write (No Inherit)
                    Grant:  System:  All Access             (No Inherit)
                    Grant:  World:   All Access             (Inherit Only)


Arguments:

    SecurityDescriptor - The address of a security descriptor to be
        initialized and filled in.  When this security descriptor is no
        longer needed, you should call ObpFreeDosDevicesProtection() to
        free the protection information.


Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_NO_MEMORY - not enough memory


--*/

{
    NTSTATUS Status;
    ULONG aceIndex, aclLength;
    PACL dacl;
    PACE_HEADER ace;
    ACCESS_MASK accessMask;

    UCHAR inheritOnlyFlags = (OBJECT_INHERIT_ACE    |
                              CONTAINER_INHERIT_ACE |
                              INHERIT_ONLY_ACE
                             );

    //
    //  NOTE:  This routine expects the value of ObpProtectionMode to have been set
    //

    Status = RtlCreateSecurityDescriptor( SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );

    ASSERT( NT_SUCCESS( Status ) );

    if (ObpProtectionMode & 0x00000001) {

        //
        //  Dacl:
        //          Grant:  World:   Execute | Read         (No Inherit)
        //          Grant:  System:  All Access             (No Inherit)
        //          Grant:  World:   Execute                (Inherit Only)
        //          Grant:  Admins:  All Access             (Inherit Only)
        //          Grant:  System:  All Access             (Inherit Only)
        //          Grant:  Owner:   All Access             (Inherit Only)
        //

        aclLength = sizeof( ACL )                           +
                    6 * sizeof( ACCESS_ALLOWED_ACE )        +
                    (2*RtlLengthSid( SeWorldSid ))          +
                    (2*RtlLengthSid( SeLocalSystemSid ))    +
                    RtlLengthSid( SeAliasAdminsSid )        +
                    RtlLengthSid( SeCreatorOwnerSid );

        dacl = (PACL)ExAllocatePool(PagedPool, aclLength );

        if (dacl == NULL) {

            return STATUS_NO_MEMORY;
        }

        Status = RtlCreateAcl( dacl, aclLength, ACL_REVISION2);
        ASSERT( NT_SUCCESS( Status ) );

        //
        //  Non-inheritable ACEs first
        //      World
        //      System
        //

        aceIndex = 0;
        accessMask = (GENERIC_READ | GENERIC_EXECUTE);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeWorldSid );
        ASSERT( NT_SUCCESS( Status ) );
        aceIndex++;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeLocalSystemSid );
        ASSERT( NT_SUCCESS( Status ) );

        //
        //  Inheritable ACEs at the end of the ACL
        //          World
        //          Admins
        //          System
        //          Owner
        //

        aceIndex++;
        accessMask = (GENERIC_EXECUTE);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeWorldSid );
        ASSERT( NT_SUCCESS( Status ) );
        Status = RtlGetAce( dacl, aceIndex, (PVOID)&ace );
        ASSERT( NT_SUCCESS( Status ) );
        ace->AceFlags |= inheritOnlyFlags;

        aceIndex++;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeAliasAdminsSid );
        ASSERT( NT_SUCCESS( Status ) );
        Status = RtlGetAce( dacl, aceIndex, (PVOID)&ace );
        ASSERT( NT_SUCCESS( Status ) );
        ace->AceFlags |= inheritOnlyFlags;

        aceIndex++;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeLocalSystemSid );
        ASSERT( NT_SUCCESS( Status ) );
        Status = RtlGetAce( dacl, aceIndex, (PVOID)&ace );
        ASSERT( NT_SUCCESS( Status ) );
        ace->AceFlags |= inheritOnlyFlags;

        aceIndex++;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeCreatorOwnerSid );
        ASSERT( NT_SUCCESS( Status ) );
        Status = RtlGetAce( dacl, aceIndex, (PVOID)&ace );
        ASSERT( NT_SUCCESS( Status ) );
        ace->AceFlags |= inheritOnlyFlags;

        Status = RtlSetDaclSecurityDescriptor( SecurityDescriptor,
                                               TRUE,               //DaclPresent,
                                               dacl,               //Dacl
                                               FALSE );            //!DaclDefaulted

        ASSERT( NT_SUCCESS( Status ) );

    } else {

        //
        //  DACL:
        //          Grant:  World:   Execute | Read | Write (No Inherit)
        //          Grant:  System:  All Access             (No Inherit)
        //          Grant:  World:   All Access             (Inherit Only)
        //

        aclLength = sizeof( ACL )                           +
                    3 * sizeof( ACCESS_ALLOWED_ACE )        +
                    (2*RtlLengthSid( SeWorldSid ))          +
                    RtlLengthSid( SeLocalSystemSid );

        dacl = (PACL)ExAllocatePool(PagedPool, aclLength );

        if (dacl == NULL) {

            return STATUS_NO_MEMORY;
        }

        Status = RtlCreateAcl( dacl, aclLength, ACL_REVISION2);
        ASSERT( NT_SUCCESS( Status ) );

        //
        //  Non-inheritable ACEs first
        //      World
        //      System
        //

        aceIndex = 0;
        accessMask = (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeWorldSid );
        ASSERT( NT_SUCCESS( Status ) );

        aceIndex++;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeLocalSystemSid );
        ASSERT( NT_SUCCESS( Status ) );

        //
        //  Inheritable ACEs at the end of the ACL
        //          World
        //

        aceIndex++;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeWorldSid );
        ASSERT( NT_SUCCESS( Status ) );
        Status = RtlGetAce( dacl, aceIndex, (PVOID)&ace );
        ASSERT( NT_SUCCESS( Status ) );
        ace->AceFlags |= inheritOnlyFlags;

        Status = RtlSetDaclSecurityDescriptor( SecurityDescriptor,
                                               TRUE,               //DaclPresent,
                                               dacl,               //Dacl
                                               FALSE );            //!DaclDefaulted

        ASSERT( NT_SUCCESS( Status ) );
    }

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

VOID
ObpFreeDosDevicesProtection (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    This routine frees memory allocated via ObpGetDosDevicesProtection().

Arguments:

    SecurityDescriptor - The address of a security descriptor initialized by
        ObpGetDosDevicesProtection().

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    PACL Dacl;
    BOOLEAN DaclPresent, Defaulted;

    Status = RtlGetDaclSecurityDescriptor ( SecurityDescriptor,
                                            &DaclPresent,
                                            &Dacl,
                                            &Defaulted );

    ASSERT( NT_SUCCESS( Status ) );
    ASSERT( DaclPresent );
    ASSERT( Dacl != NULL );

    ExFreePool( (PVOID)Dacl );

    return;
}
