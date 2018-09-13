/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obp.h

Abstract:

    Private include file for the OB subcomponent of the NTOS project

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "ntos.h"
#include "seopaque.h"
#include <zwapi.h>

//
//  The Object Header structures are private, but are defined in ob.h
//  so that various macros can directly access header fields.
//

struct _OBJECT_HEADER;
struct _OBJECT_BODY_HEADER;

//
//  Setup default pool tags
//

#ifdef POOL_TAGGING
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'  bO')
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'  bO')
#endif

//
//  Define some macros that will verify that our callbacks don't give us a bad irql
//

#if DBG
#define ObpBeginTypeSpecificCallOut( IRQL ) (IRQL)=KeGetCurrentIrql()
#define ObpEndTypeSpecificCallOut( IRQL, str, ot, o ) {                                               \
    if ((IRQL)!=KeGetCurrentIrql()) {                                                                 \
        DbgPrint( "OB: ObjectType: %wZ  Procedure: %s  Object: %08x\n", &ot->Name, str, o );          \
        DbgPrint( "    Returned at %x IRQL, but was called at %x IRQL\n", KeGetCurrentIrql(), IRQL ); \
        DbgBreakPoint();                                                                              \
    }                                                                                                 \
}
#else
#define ObpBeginTypeSpecificCallOut( IRQL )
#define ObpEndTypeSpecificCallOut( IRQL, str, ot, o )
#endif // DBG

//
//  Define some more macros to validate the current irql
//

#if DBG
#define ObpValidateIrql( str ) \
    if (KeGetCurrentIrql() > APC_LEVEL) { \
        DbgPrint( "OB: %s called at IRQL %d\n", (str), KeGetCurrentIrql() ); \
        DbgBreakPoint(); \
        }
#else
#define ObpValidateIrql( str )
#endif // DBG


//
//  This global spinlock guards the following fields:
//
//      OBJECT_HEADER.PointerCount,
//      OBJECT_HEADER.HandleCount,
//      CachedGrantedAccess Variables,
//      ObpRemoveObjectQueue
//
//  The spinlock is never held longer than is necessary to modify or read the field.
//  Only the following routines are used to increment and decrement these counts.
//

KSPIN_LOCK ObpLock;
KEVENT ObpDefaultObject;
WORK_QUEUE_ITEM ObpRemoveObjectWorkItem;
PSINGLE_LIST_ENTRY ObpRemoveObjectQueue;

//
//  This global lock is used to protect the device map tear down and build up
//  We can no longer use an individual lock in the device map itself because
//  that wasn't sufficient to protect the device map itself.
//

KSPIN_LOCK ObpDeviceMapLock;

//
//  Define the following if you want to build a system where the handle count
//  is verified to always be greater than or equal to pointer count.  Useful
//  for catching drivers that over-deref object pointers, but too slow for shipping
//  system.
//

//  #define MPSAFE_HANDLE_COUNT_CHECK   1

#ifdef MPSAFE_HANDLE_COUNT_CHECK

VOID
FASTCALL
ObpIncrPointerCount (
    IN POBJECT_HEADER ObjectHeader
    );

VOID
FASTCALL
ObpDecrPointerCount (
    IN POBJECT_HEADER ObjectHeader
    );

BOOLEAN
FASTCALL
ObpDecrPointerCountWithResult (
    IN POBJECT_HEADER ObjectHeader
    );

VOID
FASTCALL
ObpIncrHandleCount (
    IN POBJECT_HEADER ObjectHeader
    );

BOOLEAN
FASTCALL
ObpDecrHandleCount (
    IN POBJECT_HEADER ObjectHeader
    );

#else

#define ObpIncrPointerCount(np)           InterlockedIncrement( &np->PointerCount )
#define ObpDecrPointerCount(np)           InterlockedDecrement( &np->PointerCount )
#define ObpDecrPointerCountWithResult(np) (InterlockedDecrement( &np->PointerCount ) == 0)

#define ObpIncrHandleCount(np)            InterlockedIncrement( &np->HandleCount )
#define ObpDecrHandleCount(np)            (InterlockedDecrement( &np->HandleCount ) == 0)

#endif // MPSAVE_HANDLE_COUNT_CHECK


//
//  Define macros to acquire and release an object type fast mutex.
//
//
//  VOID
//  ObpEnterObjectTypeMutex (
//      IN POBJECT_TYPE ObjectType
//      )
//

#define ObpEnterObjectTypeMutex(_ObjectType) {                   \
    ObpValidateIrql("ObpEnterObjectTypeMutex");                  \
    KeEnterCriticalRegion();                                     \
    ExAcquireResourceExclusiveLite(&(_ObjectType)->Mutex, TRUE); \
}

//
//  VOID
//  ObpLeaveObjectTypeMutex (
//      IN POBJECT_TYPE ObjectType
//      )
//

#define ObpLeaveObjectTypeMutex(_ObjectType) {  \
    ExReleaseResource(&(_ObjectType)->Mutex);   \
    KeLeaveCriticalRegion();                    \
    ObpValidateIrql("ObpLeaveObjectTypeMutex"); \
}

//
//  VOID
//  ObpEnterRootDirectoryMutex (
//      VOID
//      )
//

#define ObpEnterRootDirectoryMutex() {                            \
    ObpValidateIrql("ObpEnterRootDirectoryMutex");                \
    KeEnterCriticalRegion();                                      \
    ExAcquireResourceExclusiveLite(&ObpRootDirectoryMutex, TRUE); \
}

//
//  VOID
//  ObpLeaveRootDirectoryMutex (
//      VOID
//      )
//

#define ObpLeaveRootDirectoryMutex() {             \
    ExReleaseResource(&ObpRootDirectoryMutex);     \
    KeLeaveCriticalRegion();                       \
    ObpValidateIrql("ObpLeaveRootDirectoryMutex"); \
}


//
//  A Macro to return the object table for the current process
//

#define ObpGetObjectTable() (PsGetCurrentProcess()->ObjectTable)

//
//  Macro to test whether or not the object manager is responsible for
//  an object's security descriptor, or if the object has its own
//  security management routines.
//

#define ObpCentralizedSecurity(_ObjectType)                              \
    ((_ObjectType)->TypeInfo.SecurityProcedure == SeDefaultObjectMethod)

//
//  Declare a global table of object types.
//

#define OBP_MAX_DEFINED_OBJECT_TYPES 24
POBJECT_TYPE ObpObjectTypes[ OBP_MAX_DEFINED_OBJECT_TYPES ];


//
//  This is some special purpose code to keep a table of access masks correlated with
//  back traces.  If used these routines replace the GrantedAccess mask in the
//  preceding object table entry with a granted access index and a call back index.
//

#if i386 && !FPO
ACCESS_MASK
ObpTranslateGrantedAccessIndex (
    USHORT GrantedAccessIndex
    );

USHORT
ObpComputeGrantedAccessIndex (
    ACCESS_MASK GrantedAccess
    );

USHORT ObpCurCachedGrantedAccessIndex;
USHORT ObpMaxCachedGrantedAccessIndex;
PACCESS_MASK ObpCachedGrantedAccesses;
#endif // i386 && !FPO

//
//  The three low order bits of the object table entry are used for handle
//  attributes.
//
//  Define the bit mask for the protect from close handle attribute.
//

#define OBJ_PROTECT_CLOSE 0x1

//
//  The bit mask for inherit MUST be 0x2.
//

#if (OBJ_INHERIT != 0x2)

#error Object inheritance bit definition conflicts

#endif

//
//  Define the bit mask for the generate audit on close attribute.
//
//  When a handle to an object with security is created, audit routines will
//  be called to perform any auditing that may be required. The audit routines
//  will return a boolean indicating whether or not audits should be generated
//  on close.
//

#define OBJ_AUDIT_OBJECT_CLOSE 0x00000004L

//
//  The following three bits are available for handle attributes in the
//  Object field of an ObjectTableEntry.
//

#define OBJ_HANDLE_ATTRIBUTES (OBJ_PROTECT_CLOSE | OBJ_INHERIT | OBJ_AUDIT_OBJECT_CLOSE)


//
//  Security Descriptor Cache
//
//  Cache entry header.
//

typedef struct _SECURITY_DESCRIPTOR_HEADER {

    LIST_ENTRY Link;
    ULONG  RefCount;
    ULONG  FullHash;
    QUAD   SecurityDescriptor;

} SECURITY_DESCRIPTOR_HEADER, *PSECURITY_DESCRIPTOR_HEADER;

//
//  Macro to convert a security descriptor into its security descriptor header
//

#define SD_TO_SD_HEADER(_sd) \
    CONTAINING_RECORD( (_sd), SECURITY_DESCRIPTOR_HEADER, SecurityDescriptor )

//
//  Macro to convert a header link into its security descriptor header
//

#define LINK_TO_SD_HEADER(_link) \
    CONTAINING_RECORD( (_link), SECURITY_DESCRIPTOR_HEADER, Link )

//
//  Macros to traverse a list of security descriptors forwards and backwards
//

#define NEXT_SDHEADER(_sdh) (                                                 \
    (_sdh)->Link.Flink == NULL ? NULL :                                       \
    CONTAINING_RECORD( (_sdh)->Link.Flink, SECURITY_DESCRIPTOR_HEADER, Link ) \
)

#define PREV_SDHEADER(_sdh) (                                                \
    (sdh)->Link.Blink == NULL ? NULL :                                       \
    CONTAINING_RECORD( (sdh)->Link.Blink, SECURITY_DESCRIPTOR_HEADER, Link ) \
)

//
//  Number of minor hash entries
//

#define SECURITY_DESCRIPTOR_CACHE_ENTRIES    256

//
//  Routines to protect the security descriptor cache
//

VOID
ObpAcquireDescriptorCacheWriteLock (
    VOID
    );


VOID
ObpAcquireDescriptorCacheReadLock (
    VOID
    );

VOID
ObpReleaseDescriptorCacheLock (
    VOID
    );


//
//  Global data
//

POBJECT_TYPE ObpTypeObjectType;
POBJECT_TYPE ObpDirectoryObjectType;
POBJECT_TYPE ObpSymbolicLinkObjectType;
POBJECT_TYPE ObpDeviceMapObjectType;
POBJECT_DIRECTORY ObpRootDirectoryObject;
POBJECT_DIRECTORY ObpTypeDirectoryObject;

ULARGE_INTEGER ObpDosDevicesShortNamePrefix;
ULARGE_INTEGER ObpDosDevicesShortNameRoot;
UNICODE_STRING ObpDosDevicesShortName;

ERESOURCE ObpRootDirectoryMutex;
ERESOURCE SecurityDescriptorCacheLock;

//
//  Define date structures for the object creation information region.
//

NPAGED_LOOKASIDE_LIST ObpCreateInfoLookasideList;

//
//  Define data structures for the object name buffer lookaside list.
//

#define OBJECT_NAME_BUFFER_SIZE 248

NPAGED_LOOKASIDE_LIST ObpNameBufferLookasideList;

//
//  There is one global kernel handle table accessed via negative handle
//  and only in kernel mode
//

PHANDLE_TABLE ObpKernelHandleTable;

//
//  The following macros are used to test and manipulate special kernel
//  handles.  A kernel handle is just a regular handle with its sign
//  bit set.  But must exclude -1 and -2 values which are the current
//  process and current thread constants.
//

#define KERNEL_HANDLE_MASK ((ULONG_PTR)((LONG)0x80000000))

#define IsKernelHandle(H,M)                                \
    (((KERNEL_HANDLE_MASK & (ULONG_PTR)(H)) == KERNEL_HANDLE_MASK) && \
     ((M) == KernelMode) &&                                \
     ((H) != NtCurrentThread()) &&                         \
     ((H) != NtCurrentProcess()))

#define EncodeKernelHandle(H) (HANDLE)(KERNEL_HANDLE_MASK | (ULONG_PTR)(H))

#define DecodeKernelHandle(H) (HANDLE)(KERNEL_HANDLE_MASK ^ (ULONG_PTR)(H))

//
//  Test macro for overflow
//

#define ObpIsOverflow(A,B) ((A) > ((A) + (B)))


//
//  Internal Entry Points defined in obcreate.c and some associated macros
//

#define ObpFreeObjectCreateInformation(_ObjectCreateInfo) { \
    ObpReleaseObjectCreateInformation((_ObjectCreateInfo)); \
    ObpFreeObjectCreateInfoBuffer((_ObjectCreateInfo));     \
}

#define ObpReleaseObjectCreateInformation(_ObjectCreateInfo) {               \
    if ((_ObjectCreateInfo)->SecurityDescriptor != NULL) {                   \
        SeReleaseSecurityDescriptor((_ObjectCreateInfo)->SecurityDescriptor, \
                                    (_ObjectCreateInfo)->ProbeMode,          \
                                     TRUE);                                  \
        (_ObjectCreateInfo)->SecurityDescriptor = NULL;                      \
    }                                                                        \
}

NTSTATUS
ObpCaptureObjectCreateInformation (
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN KPROCESSOR_MODE ProbeMode,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PUNICODE_STRING CapturedObjectName,
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
    IN LOGICAL UseLookaside
    );

NTSTATUS
ObpCaptureObjectName (
    IN KPROCESSOR_MODE ProbeMode,
    IN PUNICODE_STRING ObjectName,
    IN OUT PUNICODE_STRING CapturedObjectName,
    IN LOGICAL UseLookaside
    );

PWCHAR
ObpAllocateObjectNameBuffer (
    IN ULONG Length,
    IN LOGICAL UseLookaside,
    IN OUT PUNICODE_STRING ObjectName
    );

VOID
FASTCALL
ObpFreeObjectNameBuffer (
    IN PUNICODE_STRING ObjectName
    );

NTSTATUS
ObpAllocateObject (
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
    IN KPROCESSOR_MODE OwnershipMode,
    IN POBJECT_TYPE ObjectType,
    IN PUNICODE_STRING ObjectName,
    IN ULONG ObjectBodySize,
    OUT POBJECT_HEADER *ReturnedObjectHeader
    );


VOID
FASTCALL
ObpFreeObject (
    IN PVOID Object
    );


/*++

POBJECT_CREATE_INFORMATION
ObpAllocateObjectCreateInfoBuffer (
    VOID
    )

Routine Description:

    This function allocates a created information buffer.

    N.B. This function is nonpageable.

Arguments:

    None.

Return Value:

    If the allocation is successful, then the address of the allocated
    create information buffer is is returned as the function value.
    Otherwise, a value of NULL is returned.

--*/

#define ObpAllocateObjectCreateInfoBuffer()             \
    (POBJECT_CREATE_INFORMATION)ExAllocateFromPPNPagedLookasideList(LookasideCreateInfoList)


/*++

VOID
FASTCALL
ObpFreeObjectCreateInfoBuffer (
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo
    )

Routine Description:

    This function frees a create information buffer.

    N.B. This function is nonpageable.

Arguments:

    ObjectCreateInfo - Supplies a pointer to a create information buffer.

Return Value:

    None.

--*/

#define ObpFreeObjectCreateInfoBuffer(ObjectCreateInfo) \
    ExFreeToPPNPagedLookasideList(LookasideCreateInfoList, ObjectCreateInfo)


//
//  Internal Entry Points defined in oblink.c
//

NTSTATUS
ObpParseSymbolicLink (
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    );

VOID
ObpDeleteSymbolicLink (
    IN PVOID Object
    );

VOID
ObpCreateSymbolicLinkName (
    POBJECT_SYMBOLIC_LINK SymbolicLink
    );

VOID
ObpDeleteSymbolicLinkName (
    POBJECT_SYMBOLIC_LINK SymbolicLink
    );


//
//  Internal Entry Points defined in obdir.c
//

PVOID
ObpLookupDirectoryEntry (
    IN POBJECT_DIRECTORY Directory,
    IN PUNICODE_STRING Name,
    IN ULONG Attributes
    );


BOOLEAN
ObpInsertDirectoryEntry (
    IN POBJECT_DIRECTORY Directory,
    IN PVOID Object
    );


BOOLEAN
ObpDeleteDirectoryEntry (
    IN POBJECT_DIRECTORY Directory
    );


NTSTATUS
ObpLookupObjectName (
    IN HANDLE RootDirectoryHandle,
    IN PUNICODE_STRING ObjectName,
    IN ULONG Attributes,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode,
    IN PVOID ParseContext OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    IN PVOID InsertObject OPTIONAL,
    IN OUT PACCESS_STATE AccessState,
    OUT PBOOLEAN DirectoryLocked,
    OUT PVOID *FoundObject
    );

VOID
ObpUnlockObjectDirectoryPath (
    IN POBJECT_DIRECTORY LockedDirectory
    );


//
//  Internal entry points defined in obref.c
//


VOID
ObpDeleteNameCheck (
    IN PVOID Object,
    IN BOOLEAN TypeMutexHeld
    );


VOID
ObpProcessRemoveObjectQueue (
    PVOID Parameter
    );

VOID
ObpRemoveObjectRoutine (
    PVOID Object
    );


//
//  Internal entry points defined in obhandle.c
//


POBJECT_HANDLE_COUNT_ENTRY
ObpInsertHandleCount (
    POBJECT_HEADER ObjectHeader
    );

NTSTATUS
ObpIncrementHandleCount (
    OB_OPEN_REASON OpenReason,
    PEPROCESS Process,
    PVOID Object,
    POBJECT_TYPE ObjectType,
    PACCESS_STATE AccessState OPTIONAL,
    KPROCESSOR_MODE AccessMode,
    ULONG Attributes
    );


VOID
ObpDecrementHandleCount (
    PEPROCESS Process,
    POBJECT_HEADER ObjectHeader,
    POBJECT_TYPE ObjectType,
    ACCESS_MASK GrantedAccess
    );

NTSTATUS
ObpCreateHandle (
    IN OB_OPEN_REASON OpenReason,
    IN PVOID Object,
    IN POBJECT_TYPE ExpectedObjectType OPTIONAL,
    IN PACCESS_STATE AccessState,
    IN ULONG ObjectPointerBias OPTIONAL,
    IN ULONG Attributes,
    IN BOOLEAN DirectoryLocked,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *ReferencedNewObject OPTIONAL,
    OUT PHANDLE Handle
    );

NTSTATUS
ObpIncrementUnnamedHandleCount (
    PACCESS_MASK DesiredAccess,
    PEPROCESS Process,
    PVOID Object,
    POBJECT_TYPE ObjectType,
    KPROCESSOR_MODE AccessMode,
    ULONG Attributes
    );


NTSTATUS
ObpCreateUnnamedHandle (
    IN PVOID Object,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG ObjectPointerBias OPTIONAL,
    IN ULONG Attributes,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *ReferencedNewObject OPTIONAL,
    OUT PHANDLE Handle
    );

NTSTATUS
ObpChargeQuotaForObject (
    IN POBJECT_HEADER ObjectHeader,
    IN POBJECT_TYPE ObjectType,
    OUT PBOOLEAN NewObject
    );

NTSTATUS
ObpValidateDesiredAccess (
    IN ACCESS_MASK DesiredAccess
    );


//
//  Internal entry points defined in obse.c
//

BOOLEAN
ObpCheckPseudoHandleAccess (
    IN PVOID Object,
    IN ACCESS_MASK DesiredAccess,
    OUT PNTSTATUS AccessStatus,
    IN BOOLEAN TypeMutexLocked
    );


BOOLEAN
ObpCheckTraverseAccess (
    IN PVOID DirectoryObject,
    IN ACCESS_MASK TraverseAccess,
    IN PACCESS_STATE AccessState OPTIONAL,
    IN BOOLEAN TypeMutexLocked,
    IN KPROCESSOR_MODE PreviousMode,
    OUT PNTSTATUS AccessStatus
    );

BOOLEAN
ObpCheckObjectReference (
    IN PVOID Object,
    IN OUT PACCESS_STATE AccessState,
    IN BOOLEAN TypeMutexLocked,
    IN KPROCESSOR_MODE AccessMode,
    OUT PNTSTATUS AccessStatus
    );


//
//  Internal entry points defined in obsdata.c
//

NTSTATUS
ObpInitSecurityDescriptorCache (
    VOID
    );

ULONG
ObpHashSecurityDescriptor (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    );

ULONG
ObpHashBuffer (
    PVOID Data,
    ULONG Length
    );

NTSTATUS
ObpLogSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR *OutputSecurityDescriptor
    );


PSECURITY_DESCRIPTOR_HEADER
ObpCreateCacheEntry (
    PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    ULONG FullHash
    );


PSECURITY_DESCRIPTOR
ObpReferenceSecurityDescriptor (
    PVOID Object
    );

VOID
ObpDereferenceSecurityDescriptor (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    );

VOID
ObpDestroySecurityDescriptorHeader (
    IN PSECURITY_DESCRIPTOR_HEADER Header
    );

BOOLEAN
ObpCompareSecurityDescriptors (
    IN PSECURITY_DESCRIPTOR SD1,
    IN PSECURITY_DESCRIPTOR SD2
    );

NTSTATUS
ObpValidateAccessMask (
    PACCESS_STATE AccessState
    );

