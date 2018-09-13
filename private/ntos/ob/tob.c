/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tex.c

Abstract:

    Test program for the OB subcomponent of the NTOS project

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "obp.h"

GENERIC_MAPPING MyGenericMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_READ |
        STANDARD_RIGHTS_WRITE |
        STANDARD_RIGHTS_EXECUTE
};

typedef struct _OBJECTTYPEA {
    KEVENT  Event;
    ULONG   TypeALength;
    ULONG   Stuff[ 4 ];
} OBJECTTYPEA, *POBJECTTYPEA;


typedef struct _OBJECTTYPEB {
    KSEMAPHORE Semaphore;
    ULONG   TypeBLength;
    ULONG   Stuff[ 16 ];
} OBJECTTYPEB, *POBJECTTYPEB;

OBJECT_ATTRIBUTES    DirectoryObjA;
OBJECT_ATTRIBUTES    ObjectAObjA;
OBJECT_ATTRIBUTES    ObjectBObjA;
STRING  DirectoryName;
STRING  ObjectAName;
STRING  ObjectBName;
STRING  ObjectAPathName;
STRING  ObjectBPathName;
STRING  ObjectTypeAName;
STRING  ObjectTypeBName;
POBJECT_TYPE    ObjectTypeA;
POBJECT_TYPE    ObjectTypeB;
PVOID   ObjectBodyA;
PVOID   ObjectBodyB;
PVOID   ObjectBodyA1;
PVOID   ObjectBodyA2;
POBJECTTYPEA ObjectA;
POBJECTTYPEB ObjectB;
HANDLE  DirectoryHandle;
HANDLE  ObjectHandleA1;
HANDLE  ObjectHandleB1;
HANDLE  ObjectHandleA2;
HANDLE  ObjectHandleB2;


VOID
DumpAProc(
    IN PVOID Object,
    IN POB_DUMP_CONTROL Control OPTIONAL
    )
{
    POBJECTTYPEA p = (POBJECTTYPEA)Object;
    ULONG i;

    DbgPrint( "DumpAProc: %lx\n", p );
    DbgPrint( "    Length: %ld\n", p->TypeALength );
    for (i=0; i<4; i++) {
        DbgPrint( "    Stuff[%ld]: %ld\n", i, p->Stuff[i] );
        }
}

char *OpenReasonStrings[] = {
    "ObCreateHandle",
    "ObOpenHandle",
    "ObDuplicateHandle",
    "ObInheritHandle"
};

VOID
OpenAProc(
    IN OB_OPEN_REASON OpenReason,
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG HandleCount
    )
{
    DbgPrint( "OpenAProc: OpenReason = %s  Process: %lx  \n",
             OpenReasonStrings[ OpenReason ], Process );
    DbgPrint( "    Object: %lx  Access: %lx  Count: %lu\n",
             Object, GrantedAccess, HandleCount );
}


VOID
CloseAProc(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    )
{
    DbgPrint( "CloseAProc: Process: %lx  \n", Process );
    DbgPrint( "    Object: %lx  Access: %lx  ProcessHandleCount: %lu  SystemHandleCount: %lu\n",
             Object, GrantedAccess, ProcessHandleCount, SystemHandleCount );
}


VOID
DeleteAProc(
    IN PVOID Object
    )
{
    DbgPrint( "DeleteAProc: %lx\n", Object );
}

NTSTATUS
ParseAProc(
    IN PVOID ParseObject,
    IN ULONG DesiredAccess,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PSTRING CompleteName,
    IN OUT PSTRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    OUT PVOID *Object
    )
{
    DbgPrint( "ParseAProc: %lx\n", ParseObject );
    DbgPrint( "    CompleteName:  %.*s\n", CompleteName->Length,
                                         CompleteName->Buffer );
    DbgPrint( "    RemainingName: %.*s\n", RemainingName->Length,
                                         RemainingName->Buffer );
    ObReferenceObjectByPointer(
        ParseObject,
        DesiredAccess,
        ObjectTypeA,
        AccessMode
        );

    *Object = ParseObject;
    return( STATUS_SUCCESS );
}


VOID
DumpBProc(
    IN PVOID Object,
    IN POB_DUMP_CONTROL Control OPTIONAL
    )
{
    POBJECTTYPEB p = (POBJECTTYPEB)Object;
    ULONG i;

    DbgPrint( "DumpBProc: %lx\n", p );
    DbgPrint( "    Length: %ld\n", p->TypeBLength );
    for (i=0; i<16; i++) {
        DbgPrint( "    Stuff[%ld]: %ld\n", i, p->Stuff[i] );
        }
}

VOID
DeleteBProc(
    IN PVOID Object
    )
{
    DbgPrint( "DeleteBProc: %lx\n", Object );
}


BOOLEAN
obtest( void )
{
    ULONG i;
    HANDLE Handles[ 2 ];
    NTSTATUS Status;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;

    ObpDumpObjectTable( ObpGetObjectTable(), NULL );

    RtlInitString( &ObjectTypeAName, "ObjectTypeA" );
    RtlInitString( &ObjectTypeBName, "ObjectTypeB" );

    RtlZeroMemory( &ObjectTypeInitializer, sizeof( ObjectTypeInitializer ) );
    ObjectTypeInitializer.Length = sizeof( ObjectTypeInitializer );
    ObjectTypeInitializer.ValidAccessMask = -1;

    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.MaintainHandleCount = TRUE;
    ObjectTypeInitializer.DumpProcedure = DumpAProc;
    ObjectTypeInitializer.OpenProcedure = OpenAProc;
    ObjectTypeInitializer.CloseProcedure = CloseAProc;
    ObjectTypeInitializer.DeleteProcedure = DeleteAProc;
    ObjectTypeInitializer.ParseProcedure = ParseAProc;
    ObCreateObjectType(
        &ObjectTypeAName,
        &ObjectTypeInitializer,
        (PSECURITY_DESCRIPTOR)NULL,
        &ObjectTypeA
        );

    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.MaintainHandleCount = FALSE;
    ObjectTypeInitializer.GenericMapping = MyGenericMapping;
    ObjectTypeInitializer.DumpProcedure = DumpBProc;
    ObjectTypeInitializer.OpenProcedure = NULL;
    ObjectTypeInitializer.CloseProcedure = NULL;
    ObjectTypeInitializer.DeleteProcedure = DeleteBProc;
    ObjectTypeInitializer.ParseProcedure = NULL;
    ObCreateObjectType(
        &ObjectTypeBName,
        &ObjectTypeInitializer,
        (PSECURITY_DESCRIPTOR)NULL,
        &ObjectTypeB
        );

    ObpDumpTypes( NULL );

    RtlInitString( &DirectoryName, "\\MyObjects" );
    InitializeObjectAttributes( &DirectoryObjA,
                                &DirectoryName,
                                OBJ_PERMANENT |
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    NtCreateDirectoryObject( &DirectoryHandle,
                             0,
                             &DirectoryObjA
                           );
    NtClose( DirectoryHandle );

    RtlInitString( &ObjectAName, "\\myobjects\\ObjectA" );
    InitializeObjectAttributes( &ObjectAObjA,
                                &ObjectAName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    RtlInitString( &ObjectBName, "\\myobjects\\ObjectB" );
    InitializeObjectAttributes( &ObjectBObjA,
                                &ObjectBName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = ObCreateObject(
        KernelMode,
        ObjectTypeA,
        &ObjectAObjA,
        KernelMode,
        NULL,
        (ULONG)sizeof( OBJECTTYPEA ),
        0L,
        0L,
        (PVOID *)&ObjectBodyA
        );

    ObjectA = (POBJECTTYPEA)ObjectBodyA;
    ObjectA->TypeALength = sizeof( *ObjectA );
    for (i=0; i<4; i++) {
        ObjectA->Stuff[i] = i+1;
        }
    KeInitializeEvent( &ObjectA->Event, NotificationEvent, TRUE );

    Status = ObCreateObject(
        KernelMode,
        ObjectTypeB,
        &ObjectBObjA,
        KernelMode,
        NULL,
        (ULONG)sizeof( OBJECTTYPEB ),
        0L,
        0L,
        (PVOID *)&ObjectBodyB
        );

    ObjectB = (POBJECTTYPEB)ObjectBodyB;
    ObjectB->TypeBLength = sizeof( *ObjectB );
    for (i=0; i<16; i++) {
        ObjectB->Stuff[i] = i+1;
        }
    KeInitializeSemaphore ( &ObjectB->Semaphore, 2L, 2L );

    Status = ObInsertObject(
        ObjectBodyA,
        SYNCHRONIZE | 0x3,
        NULL,
        1,
        &ObjectBodyA,
        &ObjectHandleA1
        );

    DbgPrint( "Status: %lx  ObjectBodyA: %lx  ObjectHandleA1: %lx\n",
             Status, ObjectBodyA, ObjectHandleA1
           );

    Status = ObInsertObject(
        ObjectBodyB,
        SYNCHRONIZE | 0x1,
        NULL,
        1,
        &ObjectBodyB,
        &ObjectHandleB1
        );

    DbgPrint( "Status: %lx  ObjectBodyB: %lx  ObjectHandleB1: %lx\n",
             Status, ObjectBodyB, ObjectHandleB1
           );

    ObpDumpObjectTable( ObpGetObjectTable(), NULL );

    RtlInitString( &ObjectAName, "\\MyObjects\\ObjectA" );
    InitializeObjectAttributes( &ObjectAObjA,
                                &ObjectAName,
                                OBJ_OPENIF,
                                NULL,
                                NULL
                              );

    Status = ObCreateObject(
        KernelMode,
        ObjectTypeA,
        &ObjectAObjA,
        KernelMode,
        NULL,
        (ULONG)sizeof( OBJECTTYPEA ),
        0L,
        0L,
        (PVOID *)&ObjectBodyA1
        );


    Status = ObInsertObject(
        ObjectBodyA1,
        SYNCHRONIZE | 0x3,
        NULL,
        1,
        &ObjectBodyA2,
        &ObjectHandleA2
        );

    DbgPrint( "Status: %lx  ObjectBodyA1: %lx  ObjectBodyA2: %lx  ObjectHandleA2: %lx\n",
             Status, ObjectBodyA1, ObjectBodyA2, ObjectHandleA2
           );

    ObpDumpObjectTable( ObpGetObjectTable(), NULL );
    NtClose( ObjectHandleA2 );
    ObDereferenceObject( ObjectBodyA2 );    // ObInsertObject,ObjectPointerBias

    NtWaitForSingleObject( ObjectHandleB1, TRUE, NULL );
    Handles[ 0 ] = ObjectHandleA1;
    Handles[ 1 ] = ObjectHandleB1;
    NtWaitForMultipleObjects( 2, Handles, WaitAny, TRUE, NULL );

    ObReferenceObjectByHandle(
        ObjectHandleA1,
        0L,
        ObjectTypeA,
        KernelMode,
        &ObjectBodyA,
        NULL
        );

    ObReferenceObjectByHandle(
        ObjectHandleB1,
        0L,
        ObjectTypeB,
        KernelMode,
        &ObjectBodyB,
        NULL
        );
    DbgPrint( "Reference Handle %lx = %lx\n", ObjectHandleA1, ObjectBodyA );

    DbgPrint( "Reference Handle %lx = %lx\n", ObjectHandleB1, ObjectBodyB );

    ObpDumpObjectTable( ObpGetObjectTable(), NULL );

    ObReferenceObjectByPointer(
        ObjectBodyA,
        0L,
        ObjectTypeA,
        KernelMode
        );

    ObReferenceObjectByPointer(
        ObjectBodyB,
        0L,
        ObjectTypeB,
        KernelMode
        );

    ObpDumpObjectTable( ObpGetObjectTable(), NULL );

    RtlInitString( &ObjectAPathName, "\\MyObjects\\ObjectA" );
    RtlInitString( &ObjectBPathName, "\\MyObjects\\ObjectB" );
    ObReferenceObjectByName(
        &ObjectAPathName,
        OBJ_CASE_INSENSITIVE,
        0L,
        ObjectTypeA,
        KernelMode,
        NULL,
        &ObjectBodyA
        );

    ObReferenceObjectByName(
        &ObjectBPathName,
        OBJ_CASE_INSENSITIVE,
        0L,
        ObjectTypeB,
        KernelMode,
        NULL,
        &ObjectBodyB
        );

    DbgPrint( "Reference Name %s = %lx\n", ObjectAPathName.Buffer,
            ObjectBodyA );

    DbgPrint( "Reference Name %s = %lx\n", ObjectBPathName.Buffer,
            ObjectBodyB );

    ObpDumpObjectTable( ObpGetObjectTable(), NULL );

    ObDereferenceObject( ObjectBodyA );     // ObInsertObject,ObjectPointerBias
    ObDereferenceObject( ObjectBodyB );

    ObDereferenceObject( ObjectBodyA );     // ObReferenceObjectByHandle
    ObDereferenceObject( ObjectBodyB );

    ObDereferenceObject( ObjectBodyA );     // ObReferenceObjectByPointer
    ObDereferenceObject( ObjectBodyB );

    ObDereferenceObject( ObjectBodyA );     // ObReferenceObjectByName
    ObDereferenceObject( ObjectBodyB );

    ObpDumpObjectTable( ObpGetObjectTable(), NULL );

    InitializeObjectAttributes( &ObjectAObjA,
                                &ObjectAPathName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    ObOpenObjectByName(
        &ObjectAObjA,
        0L,
        NULL,
        ObjectTypeA,
        KernelMode,
        NULL,
        &ObjectHandleA2
        );

    InitializeObjectAttributes( &ObjectBObjA,
                                &ObjectBPathName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    ObOpenObjectByName(
        &ObjectBObjA,
        0L,
        NULL,
        ObjectTypeB,
        KernelMode,
        NULL,
        &ObjectHandleB2
        );

    DbgPrint( "Open Object Name %s = %lx\n", ObjectAPathName.Buffer,
            ObjectHandleA2 );

    DbgPrint( "Open Object Name %s = %lx\n", ObjectBPathName.Buffer,
            ObjectHandleB2 );

    ObpDumpObjectTable( ObpGetObjectTable(), NULL );

    NtClose( ObjectHandleA1 );
    NtClose( ObjectHandleB1 );

    ObpDumpObjectTable( ObpGetObjectTable(), NULL );

    ObReferenceObjectByHandle(
        ObjectHandleA2,
        0L,
        ObjectTypeA,
        KernelMode,
        &ObjectBodyA,
        NULL
        );

    ObReferenceObjectByHandle(
        ObjectHandleB2,
        0L,
        ObjectTypeB,
        KernelMode,
        &ObjectBodyB,
        NULL
        );
    DbgPrint( "Reference Handle %lx = %lx\n", ObjectHandleA2, ObjectBodyA );

    DbgPrint( "Reference Handle %lx = %lx\n", ObjectHandleB2, ObjectBodyB );

    ObpDumpObjectTable( ObpGetObjectTable(), NULL );

    ObOpenObjectByPointer(
        ObjectBodyA,
        OBJ_CASE_INSENSITIVE,
        0L,
        NULL,
        ObjectTypeA,
        KernelMode,
        &ObjectHandleA1
        );

    ObOpenObjectByPointer(
        ObjectBodyB,
        OBJ_CASE_INSENSITIVE,
        0L,
        NULL,
        ObjectTypeB,
        KernelMode,
        &ObjectHandleB1
        );

    DbgPrint( "Open Object Pointer %lx = %lx\n", ObjectBodyA,
            ObjectHandleA1 );

    DbgPrint( "Open Object Pointer %lx = %lx\n", ObjectBodyB,
            ObjectHandleB1 );

    ObpDumpObjectTable( ObpGetObjectTable(), NULL );

    ObReferenceObjectByHandle(
        ObjectHandleA1,
        0L,
        ObjectTypeA,
        KernelMode,
        &ObjectBodyA,
        NULL
        );

    ObReferenceObjectByHandle(
        ObjectHandleB1,
        0L,
        ObjectTypeB,
        KernelMode,
        &ObjectBodyB,
        NULL
        );
    DbgPrint( "Reference Handle %lx = %lx\n", ObjectHandleA1, ObjectBodyA );

    DbgPrint( "Reference Handle %lx = %lx\n", ObjectHandleB1, ObjectBodyB );

    ObpDumpObjectTable( ObpGetObjectTable(), NULL );

    ObDereferenceObject( ObjectBodyA );     // ObReferenceObjectByHandle
    ObDereferenceObject( ObjectBodyB );

    ObDereferenceObject( ObjectBodyA );     // ObReferenceObjectByHandle
    ObDereferenceObject( ObjectBodyB );

    NtClose( ObjectHandleA1 );
    NtClose( ObjectHandleB1 );

    NtClose( ObjectHandleA2 );
    NtClose( ObjectHandleB2 );

    ObpDumpObjectTable( ObpGetObjectTable(), NULL );

    TestFunction = NULL;

    return( TRUE );
}


int
_CDECL
main(
    int argc,
    char *argv[]
    )
{
#ifdef SIMULATOR
    extern ULONG MmNumberOfPhysicalPages;
    char *s;

    while (--argc) {
        s = *++argv;
        if (*s == '-') {
            s++;
            if (*s >= '0' && *s <= '9') {
                MmNumberOfPhysicalPages = atol( s );
                DbgPrint( "INIT: Configured with %d pages of physical memory.\n",
                          MmNumberOfPhysicalPages
                        );
                }
            else
            if (!strcmp( s, "SCR" )) {
                IoInitIncludeDevices |= IOINIT_SCREEN;
                DbgPrint( "INIT: Configured with Screen device driver.\n" );
                }
            else
            if (!strcmp( s, "MOU" )) {
                IoInitIncludeDevices |= IOINIT_MOUSE;
                DbgPrint( "INIT: Configured with Mouse device driver.\n" );
                }
            else
            if (!strcmp( s, "KBD" )) {
                IoInitIncludeDevices |= IOINIT_KEYBOARD;
                DbgPrint( "INIT: Configured with Keyboard device driver.\n" );
                }
            else
            if (!strcmp( s, "RAW" )) {
                IoInitIncludeDevices |= IOINIT_RAWFS;
                DbgPrint( "INIT: Configured with RAW File System driver.\n" );
                }
            else
            if (!strcmp( s, "FAT" )) {
                IoInitIncludeDevices |= IOINIT_FATFS;
                DbgPrint( "INIT: Configured with FAT File System driver.\n" );
                }
            else
            if (!strcmp( s, "SVR" )) {
                IoInitIncludeDevices |= IOINIT_DDFS |
                                        IOINIT_FATFS |
                                        IOINIT_SERVER_FSD |
                                        IOINIT_SERVER_LOOPBACK |
                                        IOINIT_NBF;
                if ( MmNumberOfPhysicalPages < 512 ) {
                    MmNumberOfPhysicalPages = 512;
                }
                DbgPrint( "INIT: Configured for LAN Manager server.\n" );
                }
            else {
                DbgPrint( "INIT: Invalid switch - %s\n", s );
                }
            }
        else {
            break;
            }
        }

#endif // SIMULATOR
    TestFunction = NULL;
    KiSystemStartup();
    return( 0 );
}
