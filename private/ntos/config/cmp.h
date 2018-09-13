/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmp.h

Abstract:

    This module contains the private (internal) header file for the
    configuration manager.

Author:

    Bryan M. Willman (bryanwi) 10-Sep-91

Environment:

    Kernel mode only.

Revision History:

    13-Jan-99 Dragos C. Sambotin (dragoss) - factoring the data structure declarations
        in \nt\private\ntos\inc\cmdata.h :: to be available from outside.
--*/

#ifndef _CMP_
#define _CMP_

#include "ntos.h"
#include "hive.h"
#include "wchar.h"
#include "zwapi.h"
#include <stdio.h>
#include <profiles.h>



// CM data structure declarations
// file location: \nt\private\ntos\inc
#include "cmdata.h"

// this is to catch twice deletion of an entry from a list
#define _CM_ENTRYLIST_MANIPULATION
#ifdef _CM_ENTRYLIST_MANIPULATION

#define CmpRemoveEntryList(a) 					        \
    if(((a)->Flink != NULL) && ((a)->Blink != NULL) ) {	\
	    RemoveEntryList(a);						        \
	    (a)->Flink = (a)->Blink = NULL;				    \
    }										

#define CmpClearListEntry(a) (a)->Flink = (a)->Blink = NULL

#else
#define CmpRemoveEntryList(a) RemoveEntryList(a)
#define CmpClearListEntry(a) //nothing
#endif

// tracing registry calls support
#define _WMI_TRACING_REGISTRYCALLS
#ifdef _WMI_TRACING_REGISTRYCALLS

//NTSTATUS
//CmpWmiFireEvent(
//    IN NTSTATUS Status,
//    IN HANDLE KeyHandle,
//    IN LONGLONG  ElapsedTime,
//    IN PUNICODE_STRING KeyName,
//    IN UCHAR Type
//    );
extern ULONG WmiUsePerfClock;
extern PCM_TRACE_NOTIFY_ROUTINE CmpTraceRoutine;

#define CmpWmiFireEvent(Status,KeyHandle,ElapsedTime,KeyName,Type)  \
{                                                                   \
    PCM_TRACE_NOTIFY_ROUTINE TraceRoutine = CmpTraceRoutine;        \
    if( TraceRoutine != NULL ) {                                    \
        (*TraceRoutine)(Status,KeyHandle,ElapsedTime,KeyName,Type); \
    }                                                               \
}

#define StartWmiCmTrace()\
    LARGE_INTEGER StartSystemTime;\
    LARGE_INTEGER EndSystemTime;\
    if (CmpTraceRoutine) {\
        if (WmiUsePerfClock) {\
            StartSystemTime = KeQueryPerformanceCounter(NULL);\
        }\
        else {\
            KeQuerySystemTime(&StartSystemTime);\
        }\
    }


#define EndWmiCmTrace(Status,Handle,KeyName,Type)\
    if (CmpTraceRoutine) {\
        try {\
            if (WmiUsePerfClock) {\
                EndSystemTime = KeQueryPerformanceCounter(NULL);\
            }\
            else {\
                KeQuerySystemTime(&EndSystemTime);\
            }\
            CmpWmiFireEvent(Status,Handle,EndSystemTime.QuadPart - StartSystemTime.QuadPart,KeyName,Type);\
        } except (EXCEPTION_EXECUTE_HANDLER) {\
        }\
    }

#else
#define StartWmiCmTrace()                           //nothing
#define EndWmiCmTrace(Status,Handle,KeyName,Type)   //nothing
#endif

//#define _WRITE_PROTECTED_VALUE_CACHE
#ifdef _WRITE_PROTECTED_VALUE_CACHE

#define CmpMakeSpecialPoolReadOnly(PoolAddress) \
    { \
        if( !MmProtectSpecialPool( (PVOID) PoolAddress, PAGE_READONLY) ) \
        KdPrint(("[CmpMakeSpecialPoolReadOnly]: Failed to Mark SpecialPool %lx as ReadOnly", PoolAddress )); \
    }

#define CmpMakeSpecialPoolReadWrite(PoolAddress) \
    { \
        if( !MmProtectSpecialPool( (PVOID) PoolAddress, PAGE_READWRITE) ) { \
           KdPrint(("[CmpMakeSpecialPoolReadWrite]: Failed to Mark SpecialPool %lx as ReadWrite", PoolAddress )); \
        } \
    }
#define CmpMakeValueCacheReadOnly(ValueCached,PoolAddress) \
    if(ValueCached) { \
        CmpMakeSpecialPoolReadOnly( PoolAddress );\
    }

#define CmpMakeValueCacheReadWrite(ValueCached,PoolAddress) \
    if(ValueCached) { \
        CmpMakeSpecialPoolReadWrite( PoolAddress );\
    }

#else
#define CmpMakeSpecialPoolReadOnly(a)  //nothing
#define CmpMakeSpecialPoolReadWrite(a)  //nothing
#define CmpMakeValueCacheReadOnly(a,b) //nothing
#define CmpMakeValueCacheReadWrite(a,b) //nothing
#endif

//#define _WRITE_PROTECTED_REGISTRY_POOL

#ifdef _WRITE_PROTECTED_REGISTRY_POOL

VOID
HvpMarkBinReadWrite(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    );

VOID
HvpChangeBinAllocation(
    PHBIN       Bin,
    BOOLEAN     ReadOnly
    );

VOID
CmpMarkAllBinsReadOnly(
    PHHIVE      Hive
    );

#else
#define HvpChangeBinAllocation(a,b) //nothing
#define HvpMarkBinReadWrite(a,b) //nothing
#define CmpMarkAllBinsReadOnly(a) //nothing
#endif

#ifdef POOL_TAGGING
//
// Pool Tag
//
#define  CM_POOL_TAG '  MC'
#define  CM_KCB_TAG  'bkMC'
#define  CM_POSTBLOCK_TAG  'bpMC'
#define  CM_NOTIFYBLOCK_TAG 'bnMC'
#define  CM_POSTEVENT_TAG 'epMC'
#define  CM_POSTAPC_TAG 'apMC'

#ifdef _WANT_MACHINE_IDENTIFICATION

#define CM_PARSEINI_TAG 'ipMC'
#define CM_GENINST_TAG  'igMC'

#endif

//
// Extra Tags for cache.
// We may want to merge these tags later.
//
#define  CM_CACHE_VALUE_INDEX_TAG 'IVMC'
#define  CM_CACHE_VALUE_TAG       'aVMC'
#define  CM_CACHE_INDEX_TAG       'nIMC'
#define  CM_CACHE_VALUE_DATA_TAG  'aDMC'
#define  CM_NAME_TAG              'bNMC'


#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,CM_POOL_TAG)
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,CM_POOL_TAG)

PVOID
CmpAllocateTag(
    ULONG   Size,
    BOOLEAN UseForIo,
    ULONG   Tag
    );
#else
#define CmpAllocateTag(a,b,c) CmpAllocate(a,b)
#endif

//
// A variable so can turn on/off certain performance features.
//
extern ULONG CmpCacheOnFlag;

#define CM_CACHE_FAKE_KEY  0x00000001      // Create Fake key KCB

//
// This lock protects the KCB cache, including the KCB structures,
// NameBlock and Value Index.
//

extern FAST_MUTEX CmpKcbLock;
#define LOCK_KCB_TREE() ExAcquireFastMutexUnsafe(&CmpKcbLock)
#define UNLOCK_KCB_TREE() ExReleaseFastMutexUnsafe(&CmpKcbLock)

//
// Logging.  CmLogLevel <= level
//           CmLogSelect anded bit mask select
//

//
// Logging Levels:
//

#define CML_BUGCHECK  1
#define CML_API       2
#define CML_API_ARGS  3
#define CML_WORKER    4
#define CML_MAJOR     5
#define CML_MINOR     6
#define CML_FLOW      7
#define CML_BIN       8

//
// Logging selection sets:
//

#define CMS_MAP             0x00000001
#define CMS_INIT            0x00000002
#define CMS_NTAPI           0x00000004
#define CMS_HIVE            0x00000008
#define CMS_IO              0x00000010
#define CMS_PARSE           0x00000020
#define CMS_SAVRES          0x00000040
#define CMS_CM              0x00000080
#define CMS_SEC             0x00000100
#define CMS_POOL            0x00000200
#define CMS_LOCKING         0x00000400
#define CMS_NOTIFY          0x00000800
#define CMS_EXCEPTION       0x00001000
#define CMS_INDEX           0x00002000
#define CMS_BIN_MAP         0x00004000

#define CMS_MAP_ERROR       0x00010000
#define CMS_INIT_ERROR      0x00020000
#define CMS_NTAPI_ERROR     0x00040000
#define CMS_HIVE_ERROR      0x00080000
#define CMS_IO_ERROR        0x00100000
#define CMS_PARSE_ERROR     0x00200000
#define CMS_SAVRES_ERROR    0x00400000
#define CMS_CM_ERROR        0x00800000
#define CMS_SEC_ERROR       0x01000000
#define CMS_POOL_ERROR      0x02000000
#define CMS_LOCKING_ERROR   0x04000000
#define CMS_NOTIFY_ERROR    0x08000000
#define CMS_INDEX_ERROR     0x10000000


#define CMS_DEFAULT         ((~(CMS_MAP)) & 0xffffffff)

#if DBG
extern  ULONG CmLogLevel;
extern  ULONG CmLogSelect;
#define CMLOG(level, select) if ((level <= CmLogLevel) && ((select) & CmLogSelect))
#else
#define CMLOG(level, select) if (0) {}
#endif


#define REGCHECKING 1

#if DBG

#if REGCHECKING
#define DCmCheckRegistry(a) if(HvHiveChecking) ASSERT(CmCheckRegistry(a, FALSE) == 0)
#else
#define DCmCheckRegistry(a)
#endif

#else
#define DCmCheckRegistry(a)
#endif

#define REGISTRY_LOCK_CHECKING

#ifdef  REGISTRY_LOCK_CHECKING
ULONG
CmpCheckLockExceptionFilter(
    IN PEXCEPTION_POINTERS ExceptionPointers
    );

#define BEGIN_LOCK_CHECKPOINT                                               \
    {                                                                       \
        ULONG   LockCountBefore,LockCountAfter;                             \
        LockCountBefore = ExIsResourceAcquiredShared(&CmpRegistryLock);     \
        LockCountBefore += ExIsResourceAcquiredExclusive(&CmpRegistryLock); \
        try {
#define END_LOCK_CHECKPOINT                                                     \
        } except(CmpCheckLockExceptionFilter(GetExceptionInformation())) {}     \
        LockCountAfter = ExIsResourceAcquiredShared(&CmpRegistryLock);          \
        LockCountAfter += ExIsResourceAcquiredExclusive(&CmpRegistryLock);      \
        if( LockCountBefore != LockCountAfter ) {                               \
            KeBugCheckEx(REGISTRY_ERROR,13,LockCountBefore,LockCountAfter,0);   \
        }                                                                       \
    }

#else
#define BEGIN_LOCK_CHECKPOINT              
#define END_LOCK_CHECKPOINT              
#endif

#if DBG
#define ASSERT_CM_LOCK_OWNED() \
    ASSERT(CmpTestRegistryLock() == TRUE)
#define ASSERT_CM_LOCK_OWNED_EXCLUSIVE() \
    ASSERT(CmpTestRegistryLockExclusive() == TRUE)
#else
#define ASSERT_CM_LOCK_OWNED()
#define ASSERT_CM_LOCK_OWNED_EXCLUSIVE()
#endif

#if DBG                                                                     
#define ASSERT_PASSIVE_LEVEL()                                              \
    {                                                                       \
        KIRQL   Irql;                                                       \
        Irql = KeGetCurrentIrql();                                          \
        if( KeGetCurrentIrql() != PASSIVE_LEVEL ) {                         \
            DbgPrint("ASSERT_PASSIVE_LEVEL failed ... Irql = %lu\n",Irql);  \
            ASSERT( FALSE );                                                \
        }                                                                   \
    }
#else
#define ASSERT_PASSIVE_LEVEL() 
#endif

#if defined(_CM_LDR_)

//
// KeBugCheckEx() is not available to boot code.
//

#define CM_BUGCHECK( Code, Parm1, Parm2, Parm3, Parm4 ) ASSERT(FALSE)

#else

#define CM_BUGCHECK( Code, Parm1, Parm2, Parm3, Parm4 ) \
    KeBugCheckEx( Code, Parm1, Parm2, Parm3, Parm4 )

#endif

#define VALIDATE_CELL_MAP(LINE,Map,Hive,Address)                                                    \
    if( Map == NULL ) {                                                                             \
            CM_BUGCHECK (REGISTRY_ERROR,11,(ULONG_PTR)(Hive),(ULONG)(Address),(ULONG)(LINE)) ;     \
    }

#if DBG
VOID
SepDumpSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSZ TitleString
    );

extern BOOLEAN SepDumpSD;

#define CmpDumpSecurityDescriptor(x,y) \
        { \
            SepDumpSD=TRUE;     \
            SepDumpSecurityDescriptor(x, y);  \
            SepDumpSD=FALSE;    \
        }
#else

#define CmpDumpSecurityDescriptor(x,y)

#endif


//
// misc stuff
//

extern  UNICODE_STRING  CmRegistrySystemCloneName;

//
// Determines whether the Current Control Set used during booting
// is cloned in order to fully preserve it for being saved
// as the LKG Control Set.
//

#define CLONE_CONTROL_SET FALSE


#define NUMBER_TYPES (MaximumType + 1)

#define CM_WRAP_LIMIT               0x7fffffff


//
// Tuning and control constants
//
#define CM_MAX_STASH           1024*1024        // If size of data for a set
                                                // is bigger than this,

#define CM_MAX_REASONABLE_VALUES    100         // If number of values for a
                                                // key is greater than this,
                                                // round up value list size


//
// Limit on the number of layers of hive there may be.  We allow only
// the master hive and hives directly linked into it for now, for currently
// value is always 2..
//

#define MAX_HIVE_LAYERS         2


//
// structure used to create and sort ordered list of drivers to be loaded.
// This is also used by the OS Loader when loading the boot drivers.
// (Particularly the ErrorControl field)
//

typedef struct _BOOT_DRIVER_NODE {
    BOOT_DRIVER_LIST_ENTRY ListEntry;
    UNICODE_STRING Group;
    UNICODE_STRING Name;
    ULONG Tag;
    ULONG ErrorControl;
} BOOT_DRIVER_NODE, *PBOOT_DRIVER_NODE;

//
// extern for object type pointer
//

extern  POBJECT_TYPE CmpKeyObjectType;


//
// Miscelaneous Hash routines
//
#define RNDM_CONSTANT   314159269    /* default value for "scrambling constant" */
#define RNDM_PRIME     1000000007    /* prime number, also used for scrambling  */

#define HASH_KEY(_convkey_) ((RNDM_CONSTANT * (_convkey_)) % RNDM_PRIME)

#define GET_HASH_INDEX(Key) HASH_KEY(Key) % CmpHashTableSize
#define GET_HASH_ENTRY(Table, Key) Table[GET_HASH_INDEX(Key)]

//
// CM_KEY_BODY
//
//  Same structure used for KEY_ROOT and KEY objects.  This is the
//  Cm defined part of the object.
//
//  This object represents an open instance, several of them could refer
//  to a single key control block.
//
#define KEY_BODY_TYPE           0x6b793032      // "ky02"

struct _CM_NOTIFY_BLOCK; //forward

typedef struct _CM_KEY_BODY {
    ULONG                   Type;
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock;
    struct _CM_NOTIFY_BLOCK *NotifyBlock;
    PEPROCESS               Process; // the owner process
#ifdef KCB_TO_KEYBODY_LINK
    LIST_ENTRY              KeyBodyList; // key_nodes using the same kcb
#endif
} CM_KEY_BODY, *PCM_KEY_BODY;


#ifdef KCB_TO_KEYBODY_LINK

#define INIT_KCB_KEYBODY_LIST(kcb)                  \
    LOCK_KCB_TREE();                                \
    InitializeListHead(&(kcb->KeyBodyListHead));    \
    UNLOCK_KCB_TREE()
    
#define ASSERT_KEYBODY_LIST_EMPTY(kcb)  ASSERT(IsListEmpty(&(kcb->KeyBodyListHead)) == TRUE)

#define ENLIST_KEYBODY_IN_KEYBODY_LIST(KeyBody)                                             \
    ASSERT( KeyBody->KeyControlBlock != NULL );                                             \
    LOCK_KCB_TREE();                                                                        \
    InsertTailList(&(KeyBody->KeyControlBlock->KeyBodyListHead),&(KeyBody->KeyBodyList));   \
    UNLOCK_KCB_TREE()

#define DELIST_KEYBODY_FROM_KEYBODY_LIST(KeyBody)                                           \
    ASSERT( KeyBody->KeyControlBlock != NULL );                                             \
    ASSERT(IsListEmpty(&(KeyBody->KeyControlBlock->KeyBodyListHead)) == FALSE);             \
    LOCK_KCB_TREE();                                                                        \
    RemoveEntryList(&(KeyBody->KeyBodyList));                                               \
    UNLOCK_KCB_TREE()

#else
#define INIT_KCB_KEYBODY_LIST(kcb)      //nothing
#define ASSERT_KEYBODY_LIST_EMPTY(kcb)  //nothing
#define ENLIST_KEYBODY_IN_KEYBODY_LIST(KeyBody)     //nothing
#define DELIST_KEYBODY_FROM_KEYBODY_LIST(KeyBody)   //nothing
#endif

#define ASSERT_KEY_OBJECT(x) ASSERT(((PCM_KEY_BODY)x)->Type == KEY_BODY_TYPE)
#define ASSERT_NODE(x) ASSERT(((PCM_KEY_NODE)x)->Signature == CM_KEY_NODE_SIGNATURE)
#define ASSERT_SECURITY(x) ASSERT(((PCM_KEY_SECURITY)x)->Signature == CM_KEY_SECURITY_SIGNATURE)

//
// CM_POST_KEY_BODY
//
// A post block can have attached a keybody which has to be dereferenced 
// when the post block goes out of scope. This structure allows the 
// implementation of keybody "delayed dereferencing". (see CmpPostNotify for comments)
//

typedef struct _CM_POST_KEY_BODY {
    LIST_ENTRY                  KeyBodyList;
    struct _CM_KEY_BODY         *KeyBody;        // this key body object
} CM_POST_KEY_BODY, *PCM_POST_KEY_BODY;


//
// CM_NOTIFY_BLOCK
//
//  A notify block tracks an active notification waiting for notification.
//  Any one open instance (CM_KEY_BODY) will refer to at most one
//  notify block.  A given key control block may have as many notify
//  blocks refering to it as there are CM_KEY_BODYs refering to it.
//  Notify blocks are attached to hives and sorted by length of name.
//

typedef struct _CM_NOTIFY_BLOCK {
    LIST_ENTRY                  HiveList;        // sorted list of notifies
    PCM_KEY_CONTROL_BLOCK       KeyControlBlock; // Open instance notify is on
    struct _CM_KEY_BODY         *KeyBody;        // our owning key handle object
    ULONG                       Filter;          // Events of interest
    LIST_ENTRY                  PostList;        // Posts to fill
    SECURITY_SUBJECT_CONTEXT    SubjectContext;  // Security stuff
    BOOLEAN                     WatchTree;
    BOOLEAN                     NotifyPending;
} CM_NOTIFY_BLOCK, *PCM_NOTIFY_BLOCK;

//
// CM_POST_BLOCK
//
//  Whenever a notify call is made, a post block is created and attached
//  to the notify block.  Each time an event is posted against the notify,
//  the waiter described by the post block is signaled.  (i.e. APC enqueued,
//  event signalled, etc.)
//

//
//  The NotifyType ULONG is a combination of POST_BLOCK_TYPE enum and flags 
//

typedef enum _POST_BLOCK_TYPE {
    PostSynchronous = 1,
    PostAsyncUser = 2,
    PostAsyncKernel = 3
} POST_BLOCK_TYPE;

typedef struct _CM_SYNC_POST_BLOCK {
    PKEVENT                 SystemEvent;
    NTSTATUS                Status;
} CM_SYNC_POST_BLOCK, *PCM_SYNC_POST_BLOCK;

typedef struct _CM_ASYNC_USER_POST_BLOCK {
    PKEVENT                 UserEvent;
    PKAPC                   Apc;
    PIO_STATUS_BLOCK        IoStatusBlock;
} CM_ASYNC_USER_POST_BLOCK, *PCM_ASYNC_USER_POST_BLOCK;

typedef struct _CM_ASYNC_KERNEL_POST_BLOCK {
    PKEVENT                 Event;
    PWORK_QUEUE_ITEM        WorkItem;
    WORK_QUEUE_TYPE         QueueType;
} CM_ASYNC_KERNEL_POST_BLOCK, *PCM_ASYNC_KERNEL_POST_BLOCK;

typedef union _CM_POST_BLOCK_UNION {
    CM_SYNC_POST_BLOCK  Sync;
    CM_ASYNC_USER_POST_BLOCK AsyncUser;
    CM_ASYNC_KERNEL_POST_BLOCK AsyncKernel;
} CM_POST_BLOCK_UNION, *PCM_POST_BLOCK_UNION;

typedef struct _CM_POST_BLOCK {
#if DBG
    BOOLEAN                     TraceIntoDebugger;
#endif
    LIST_ENTRY                  NotifyList;
    LIST_ENTRY                  ThreadList;
    LIST_ENTRY                  CancelPostList; // slave notifications that are attached to this notification
    struct _CM_POST_KEY_BODY    *PostKeyBody; 
    ULONG                       NotifyType; 
    PCM_POST_BLOCK_UNION        u;
} CM_POST_BLOCK, *PCM_POST_BLOCK;

#define REG_NOTIFY_POST_TYPE_MASK (0x0000FFFFL)   // mask for finding out the type of the post block

#define REG_NOTIFY_MASTER_POST    (0x00010000L)   // The current post block is a master one

//
// Usefull macros to manipulate the NotifyType field in CM_POST_BLOCK
//
#define PostBlockType(_post_) ((POST_BLOCK_TYPE)( ((_post_)->NotifyType) & REG_NOTIFY_POST_TYPE_MASK ))

#define IsMasterPostBlock(_post_)           ( ((_post_)->NotifyType) &   REG_NOTIFY_MASTER_POST )
#define SetMasterPostBlockFlag(_post_)      ( ((_post_)->NotifyType) |=  REG_NOTIFY_MASTER_POST )
#define ClearMasterPostBlockFlag(_post_)    ( ((_post_)->NotifyType) &= ~REG_NOTIFY_MASTER_POST )

//
// This lock protects the PostList(s) in Notification objects.
// It is used to prevent attempts for simultaneous changes of 
// CancelPostList list in PostBlocks
//

extern FAST_MUTEX CmpPostLock;
#define LOCK_POST_LIST() ExAcquireFastMutexUnsafe(&CmpPostLock)
#define UNLOCK_POST_LIST() ExReleaseFastMutexUnsafe(&CmpPostLock)

// ----- Cm version of Hive structure (CMHIVE) -----
//
typedef struct _CMHIVE {
    HHIVE           Hive;
    HANDLE          FileHandles[HFILE_TYPE_MAX];
    LIST_ENTRY      NotifyList;
    LIST_ENTRY      HiveList;           // Used to find hives at shutdown
    PFAST_MUTEX     HiveLock;           // Used to synchronize operations on the hive (NotifyList and Flush)
} CMHIVE, *PCMHIVE;

//
// Hive locking support
//
//
#define CmLockHive(_hive_)  ASSERT( (_hive_)->HiveLock );\
                            ExAcquireFastMutexUnsafe((_hive_)->HiveLock)
#define CmUnlockHive(_hive_) ASSERT( (_hive_)->HiveLock );\
                             ExReleaseFastMutexUnsafe((_hive_)->HiveLock)


//
// Macros
//

//
// ----- CM_KEY_NODE -----
//
#define CmpHKeyNameLen(Key) \
        (((Key)->Flags & KEY_COMP_NAME) ? \
            CmpCompressedNameSize((Key)->Name,(Key)->NameLength) : \
            (Key)->NameLength)

#define CmpNcbNameLen(Ncb) \
        (((Ncb)->Compressed) ? \
            CmpCompressedNameSize((Ncb)->Name,(Ncb)->NameLength) : \
            (Ncb)->NameLength)

#define CmpHKeyNodeSize(Hive, KeyName) \
    (FIELD_OFFSET(CM_KEY_NODE, Name) + CmpNameSize(Hive, KeyName))


//
// ----- CM_KEY_VALUE -----
//


#define CmpValueNameLen(Value)                                       \
        (((Value)->Flags & VALUE_COMP_NAME) ?                           \
            CmpCompressedNameSize((Value)->Name,(Value)->NameLength) :  \
            (Value)->NameLength)

#define CmpHKeyValueSize(Hive, ValueName) \
    (FIELD_OFFSET(CM_KEY_VALUE, Name) + CmpNameSize(Hive, ValueName))


//
// Communication area
//
//
//  Protocol (server side):
//      Wait for StartRegistryCommand event
//      read message, do work
//      signal EndRegistryCommand event
//
//  Protocal (client side):
//      Acquire RegistryCommandMutex
//      write message
//      signal StartRegistryCommand
//      wait for EndRegistryCommand
//      Release RegistryCommandMutex
//


#define REG_CMD_INIT                1
#define REG_CMD_FLUSH_KEY           2
#define REG_CMD_FILE_SET_SIZE       3
#define REG_CMD_HIVE_OPEN           4
#define REG_CMD_HIVE_CLOSE          5
#define REG_CMD_SHUTDOWN            6
#define REG_CMD_RENAME_HIVE         7
#define REG_CMD_ADD_HIVE_LIST       8
#define REG_CMD_REMOVE_HIVE_LIST    9
#define REG_CMD_REFRESH_HIVE       10
#define REG_CMD_HIVE_READ          11

//
// WARNNOTE:    Why do we have such a random structure?
//              change this to pass a pointer to a function specific struct
//


typedef struct _REGISTRY_COMMAND {
    ULONG       Command;
    PHHIVE      Hive;
    HCELL_INDEX Cell;
    ULONG       FileType;
    ULONG       FileSize;
    NTSTATUS    Status;
    POBJECT_ATTRIBUTES FileAttributes;
    PCMHIVE     CmHive;
    PVOID       Buffer;
    PVOID       Offset;
    BOOLEAN     Allocate;
    BOOLEAN     SetupBoot;
    BOOLEAN     RegistryLockAquired;    // needed to avoid recursivity deadlock with ZwCreate calls calling back into registry
    PUNICODE_STRING NewName;
    POBJECT_NAME_INFORMATION OldName;
    ULONG NameInfoLength;
    PSECURITY_CLIENT_CONTEXT ImpersonationContext;
} REGISTRY_COMMAND, *PREGISTRY_COMMAND;


//
// ----- Procedure Prototypes -----
//

//
// Configuration Manager private procedure prototypes
//

#define REG_OPTION_PREDEF_HANDLE (0x00000008L)
#define REG_PREDEF_HANDLE_MASK   (0x80000000L)

typedef struct _CM_PARSE_CONTEXT {
    ULONG               TitleIndex;
    UNICODE_STRING      Class;
    ULONG               CreateOptions;
    ULONG               Disposition;
    BOOLEAN             CreateLink;
    CM_KEY_REFERENCE    ChildHive;
    HANDLE              PredefinedHandle;
} CM_PARSE_CONTEXT, *PCM_PARSE_CONTEXT;

NTSTATUS
CmpParseKey(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN OUT PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    );

NTSTATUS
CmpDoCreate(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PACCESS_STATE AccessState,
    IN PUNICODE_STRING Name,
    IN KPROCESSOR_MODE AccessMode,
    IN PCM_PARSE_CONTEXT Context,
    IN PCM_KEY_CONTROL_BLOCK ParentKcb,
    OUT PVOID *Object
    );

NTSTATUS
CmpDoCreateChild(
    IN PHHIVE Hive,
    IN HCELL_INDEX ParentCell,
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PACCESS_STATE AccessState,
    IN PUNICODE_STRING Name,
    IN KPROCESSOR_MODE AccessMode,
    IN PCM_PARSE_CONTEXT Context,
    IN PCM_KEY_CONTROL_BLOCK ParentKcb,
    IN USHORT Flags,
    OUT PHCELL_INDEX KeyCell,
    OUT PVOID *Object
    );

NTSTATUS
CmpQueryKeyName(
    IN PVOID Object,
    IN BOOLEAN HasObjectName,
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength
    );

VOID
CmpDeleteKeyObject(
    IN PVOID Object
    );

VOID
CmpCloseKeyObject(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    );

NTSTATUS
CmpSecurityMethod (
    IN PVOID Object,
    IN SECURITY_OPERATION_CODE OperationCode,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG CapturedLength,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
    );

#define KCB_WORKER_CONTINUE     0
#define KCB_WORKER_DONE         1
#define KCB_WORKER_DELETE       2

typedef
ULONG
(*PKCB_WORKER_ROUTINE) (
    PCM_KEY_CONTROL_BLOCK Current,
    PVOID                 Context1,
    PVOID                 Context2
    );


VOID
CmpSearchKeyControlBlockTree(
    PKCB_WORKER_ROUTINE WorkerRoutine,
    PVOID               Context1,
    PVOID               Context2
    );

//
// Wrappers
//

PVOID
CmpAllocate(
    ULONG   Size,
    BOOLEAN UseForIo
    );

VOID
CmpFree(
    PVOID   MemoryBlock,
    ULONG   GlobalQuotaSize
    );

BOOLEAN
CmpFileSetSize(
    PHHIVE      Hive,
    ULONG       FileType,
    ULONG       FileSize
    );

NTSTATUS
CmpDoFileSetSize(
    PHHIVE      Hive,
    ULONG       FileType,
    ULONG       FileSize
    );

BOOLEAN
CmpFileWrite(
    PHHIVE      Hive,
    ULONG       FileType,
    PCMP_OFFSET_ARRAY offsetArray,
    ULONG offsetArrayCount,
    PULONG      FileOffset
    );

BOOLEAN
CmpFileRead (
    PHHIVE      Hive,
    ULONG       FileType,
    PULONG      FileOffset,
    PVOID       DataBuffer,
    ULONG       DataLength
    );

BOOLEAN
CmpFileFlush (
    PHHIVE      Hive,
    ULONG       FileType
    );

NTSTATUS
CmpCreateEvent(
    IN EVENT_TYPE  eventType,
    OUT PHANDLE eventHandle,
    OUT PKEVENT *event
    );


//
// Configuration Manager CM level registry functions
//

NTSTATUS
CmDeleteKey(
    IN PCM_KEY_BODY KeyBody
    );

NTSTATUS
CmDeleteValueKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN UNICODE_STRING ValueName
    );

NTSTATUS
CmEnumerateKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    );

NTSTATUS
CmEnumerateValueKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    );

NTSTATUS
CmFlushKey(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
    );

NTSTATUS
CmQueryKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    );

NTSTATUS
CmQueryValueKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN UNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    );

NTSTATUS
CmQueryMultipleValueKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN PKEY_VALUE_ENTRY ValueEntries,
    IN ULONG EntryCount,
    IN PVOID ValueBuffer,
    IN OUT PULONG BufferLength,
    IN OPTIONAL PULONG ResultLength
    );

NTSTATUS
CmRenameValueKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN UNICODE_STRING SourceValueName,
    IN UNICODE_STRING TargetValueName,
    IN ULONG TargetIndex
    );

NTSTATUS
CmReplaceKey(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PUNICODE_STRING NewHiveName,
    IN PUNICODE_STRING OldFileName
    );

NTSTATUS
CmRestoreKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN HANDLE  FileHandle,
    IN ULONG Flags
    );

NTSTATUS
CmSaveKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN HANDLE  FileHandle
    );

NTSTATUS
CmSaveMergedKeys(
    IN PCM_KEY_CONTROL_BLOCK    HighPrecedenceKcb,
    IN PCM_KEY_CONTROL_BLOCK    LowPrecedenceKcb,
    IN HANDLE   FileHandle
    );

NTSTATUS
CmSetValueKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN PUNICODE_STRING ValueName,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
    );

NTSTATUS
CmSetLastWriteTimeKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN PLARGE_INTEGER LastWriteTime
    );

NTSTATUS
CmpNotifyChangeKey(
    IN PCM_KEY_BODY     KeyBody,
    IN PCM_POST_BLOCK   PostBlock,
    IN ULONG            CompletionFilter,
    IN BOOLEAN          WatchTree,
    IN PVOID            Buffer,
    IN ULONG            BufferSize,
    IN PCM_POST_BLOCK   MasterPostBlock
    );

NTSTATUS
CmLoadKey(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN POBJECT_ATTRIBUTES SourceFile,
    IN ULONG Flags
    );

NTSTATUS
CmUnloadKey(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PCM_KEY_CONTROL_BLOCK Kcb
    );

//
// Procedures private to CM
//

BOOLEAN
CmpMarkKeyDirty(
    PHHIVE Hive,
    HCELL_INDEX Cell
    );

BOOLEAN
CmpDoFlushAll(
    VOID
    );

extern BOOLEAN CmpLazyFlushPending;

VOID
CmpLazyFlush(
    VOID
    );

VOID
CmpQuotaWarningWorker(
    IN PVOID WorkItem
    );

VOID
CmpComputeGlobalQuotaAllowed(
    VOID
    );

BOOLEAN
CmpClaimGlobalQuota(
    IN ULONG    Size
    );

VOID
CmpReleaseGlobalQuota(
    IN ULONG    Size
    );

VOID
CmpSetGlobalQuotaAllowed(
    VOID
    );

//
// security functions (cmse.c)
//

NTSTATUS
CmpAssignSecurityDescriptor(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PCM_KEY_NODE Node,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );

BOOLEAN
CmpCheckCreateAccess(
    IN PUNICODE_STRING RelativeName,
    IN PSECURITY_DESCRIPTOR Descriptor,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE PreviousMode,
    IN ACCESS_MASK AdditionalAccess,
    OUT PNTSTATUS AccessStatus
    );

BOOLEAN
CmpCheckNotifyAccess(
    IN PCM_NOTIFY_BLOCK NotifyBlock,
    IN PHHIVE Hive,
    IN PCM_KEY_NODE Node
    );

PSECURITY_DESCRIPTOR
CmpHiveRootSecurityDescriptor(
    VOID
    );

VOID
CmpFreeSecurityDescriptor(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
    );


//
// Access to the registry is serialized by a shared resource, CmpRegistryLock.
//
extern ERESOURCE CmpRegistryLock;

#if 0
#define CmpLockRegistry() KeEnterCriticalRegion(); \
                          ExAcquireResourceShared(&CmpRegistryLock, TRUE)

#define CmpLockRegistryExclusive() KeEnterCriticalRegion(); \
                                   ExAcquireResourceExclusive(&CmpRegistryLock,TRUE)

#else
VOID
CmpLockRegistryExclusive(
    VOID
    );
VOID
CmpLockRegistry(
    VOID
    );
#endif
BOOLEAN
CmpIsLastKnownGoodBoot(
    VOID
    );

VOID
CmpUnlockRegistry(
    );

#if DBG
BOOLEAN
CmpTestRegistryLock(
    VOID
    );
BOOLEAN
CmpTestRegistryLockExclusive(
    VOID
    );
#endif

NTSTATUS
CmpQueryKeyData(
    PHHIVE Hive,
    PCM_KEY_NODE Node,
    KEY_INFORMATION_CLASS KeyInformationClass,
    PVOID KeyInformation,
    ULONG Length,
    PULONG ResultLength
    );


VOID
CmpFreeKeyBody(
    PHHIVE Hive,
    HCELL_INDEX Cell
    );

VOID
CmpFreeValue(
    PHHIVE Hive,
    HCELL_INDEX Cell
    );

HCELL_INDEX
CmpFindValueByName(
    PHHIVE Hive,
    PCM_KEY_NODE KeyNode,
    PUNICODE_STRING Name
    );

#define CmpFindValueByName(h,k,n) CmpFindNameInList(h,&((k)->ValueList),n,NULL,NULL)

NTSTATUS
CmpDeleteChildByName(
    PHHIVE  Hive,
    HCELL_INDEX Cell,
    UNICODE_STRING  Name,
    PHCELL_INDEX    ChildCell
    );

NTSTATUS
CmpFreeKeyByCell(
    PHHIVE Hive,
    HCELL_INDEX Cell,
    BOOLEAN Unlink
    );

HCELL_INDEX
CmpFindNameInList(
    IN PHHIVE  Hive,
    IN PCHILD_LIST ChildList,
    IN PUNICODE_STRING Name,
    IN OPTIONAL PCELL_DATA *ChildAddress,
    IN OPTIONAL PULONG ChildIndex
    );

HCELL_INDEX
CmpCopyCell(
    PHHIVE  SourceHive,
    HCELL_INDEX SourceCell,
    PHHIVE  TargetHive,
    HSTORAGE_TYPE   Type
    );

HCELL_INDEX
CmpCopyValue(
    PHHIVE  SourceHive,
    HCELL_INDEX SourceValueCell,
    PHHIVE  TargetHive,
    HSTORAGE_TYPE Type
    );

HCELL_INDEX
CmpCopyKeyPartial(
    PHHIVE  SourceHive,
    HCELL_INDEX SourceKeyCell,
    PHHIVE  TargetHive,
    HCELL_INDEX Parent,
    BOOLEAN CopyValues
    );

BOOLEAN
CmpCopySyncTree(
    PHHIVE                  SourceHive,
    HCELL_INDEX             SourceCell,
    PHHIVE                  TargetHive,
    HCELL_INDEX             TargetCell,
    BOOLEAN                 CopyVolatile,
    CMP_COPY_TYPE           CopyType
    );

//
// BOOLEAN
// CmpCopyTree(
//    PHHIVE      SourceHive,
//    HCELL_INDEX SourceCell,
//    PHHIVE      TargetHive,
//    HCELL_INDEX TargetCell
//    );
//

#define CmpCopyTree(s,c,t,l) CmpCopySyncTree(s,c,t,l,FALSE,Copy)

//
// BOOLEAN
// CmpCopyTreeEx(
//    PHHIVE      SourceHive,
//    HCELL_INDEX SourceCell,
//    PHHIVE      TargetHive,
//    HCELL_INDEX TargetCell,
//    BOOLEAN     CopyVolatile
//    );
//

#define CmpCopyTreeEx(s,c,t,l,f) CmpCopySyncTree(s,c,t,l,f,Copy)

//
// BOOLEAN
// CmpSyncTrees(
//   PHHIVE      SourceHive,
//   HCELL_INDEX SourceCell,
//   PHHIVE      TargetHive,
//   HCELL_INDEX TargetCell,
//   BOOLEAN     CopyVolatile);
//

#define CmpSyncTrees(s,c,t,l,f) CmpCopySyncTree(s,c,t,l,f,Sync)


//
// BOOLEAN
// CmpMergeTrees(
//   PHHIVE      SourceHive,
//   HCELL_INDEX SourceCell,
//   PHHIVE      TargetHive,
//   HCELL_INDEX TargetCell);
//

#define CmpMergeTrees(s,c,t,l) CmpCopySyncTree(s,c,t,l,FALSE,Merge)

VOID
CmpDeleteTree(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    );

VOID
CmpSetVersionData(
    VOID
    );

NTSTATUS
CmpInitializeHardwareConfiguration(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTSTATUS
CmpInitializeMachineDependentConfiguration(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTSTATUS
CmpInitializeRegistryNode(
    IN PCONFIGURATION_COMPONENT_DATA CurrentEntry,
    IN HANDLE ParentHandle,
    OUT PHANDLE NewHandle,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN PUSHORT DeviceIndexTable
    );

NTSTATUS
CmpInitializeHive(
    PCMHIVE         *CmHive,
    ULONG           OperationType,
    ULONG           HiveFlags,
    ULONG           FileType,
    PVOID           HiveData OPTIONAL,
    HANDLE          Primary,
    HANDLE          Alternate,
    HANDLE          Log,
    HANDLE          External,
    PUNICODE_STRING FileName
    );

BOOLEAN
CmpDestroyHive(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
    );

VOID
CmpInitializeRegistryNames(
    VOID
    );

VOID
CmpInitializeCache(
    VOID
    );

PCM_KEY_CONTROL_BLOCK
CmpCreateKeyControlBlock(
    PHHIVE          Hive,
    HCELL_INDEX     Cell,
    PCM_KEY_NODE    Node,
    PCM_KEY_CONTROL_BLOCK ParentKcb,
    BOOLEAN FakeKey,
    PUNICODE_STRING KeyName
    );

ULONG
CmpSearchForOpenSubKeys(
    IN PCM_KEY_CONTROL_BLOCK SearchKey,
    IN SUBKEY_SEARCH_TYPE SearchType
    );

VOID
CmpDereferenceKeyControlBlock(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    );

VOID
CmpRemoveKeyControlBlock(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    );

VOID
CmpReportNotify(
    PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    PHHIVE          Hive,
    HCELL_INDEX     Cell,
    ULONG           NotifyMask
    );

VOID
CmpPostNotify(
    PCM_NOTIFY_BLOCK    NotifyBlock,
    PUNICODE_STRING     Name OPTIONAL,
    ULONG               Filter,
    NTSTATUS            Status,
    PLIST_ENTRY         ExternalKeyDeref OPTIONAL
    );

PCM_POST_BLOCK
CmpAllocatePostBlock(
    IN POST_BLOCK_TYPE BlockType,
    IN ULONG           PostFlags,
    IN PCM_KEY_BODY    KeyBody,
    IN PCM_POST_BLOCK  MasterBlock 
    );

//
//PCM_POST_BLOCK
//CmpAllocateMasterPostBlock(
//    IN POST_BLOCK_TYPE BlockType
//     );
//
#define CmpAllocateMasterPostBlock(b) CmpAllocatePostBlock(b,REG_NOTIFY_MASTER_POST,NULL,NULL)

//
//PCM_POST_BLOCK
//CmpAllocateSlavePostBlock(
//    IN POST_BLOCK_TYPE BlockType,
//    IN PCM_KEY_BODY     KeyBody,
//    IN PCM_POST_BLOCK  MasterBlock
//     );
//
#define CmpAllocateSlavePostBlock(b,k,m) CmpAllocatePostBlock(b,0,k,m)

VOID
CmpFreePostBlock(
    IN PCM_POST_BLOCK PostBlock
    );

VOID
CmpPostApc(
    struct _KAPC *Apc,
    PKNORMAL_ROUTINE *NormalRoutine,
    PVOID *NormalContext,
    PVOID *SystemArgument1,
    PVOID *SystemArgument2
    );

VOID
CmpFlushNotify(
    PCM_KEY_BODY    KeyBody
    );

VOID
CmpPostApcRunDown(
    struct _KAPC *Apc
    );

NTSTATUS
CmpOpenHiveFiles(
    PUNICODE_STRING     BaseName,
    PWSTR               Extension OPTIONAL,
    PHANDLE             Primary,
    PHANDLE             Secondary,
    PULONG              PrimaryDisposition,
    PULONG              SecondaryDispoition,
    BOOLEAN             CreateAllowed,
    BOOLEAN             MarkAsSystemHive,
    PULONG              ClusterSize
    );

NTSTATUS
CmpLinkHiveToMaster(
    PUNICODE_STRING LinkName,
    HANDLE RootDirectory,
    PCMHIVE CmHive,
    BOOLEAN Allocate,
    PSECURITY_DESCRIPTOR SecurityDescriptor
    );

VOID
CmpWorker(
    IN OUT PREGISTRY_COMMAND Command
    );

NTSTATUS
CmpSaveBootControlSet(
     IN USHORT ControlSetNum
     );

//
// checkout procedure
//


ULONG
CmCheckRegistry(
    PCMHIVE CmHive,
    BOOLEAN Clean
    );

BOOLEAN
CmpValidateHiveSecurityDescriptors(
    IN PHHIVE Hive
    );

#define SetUsed(Hive, Cell) \
    {                                               \
        PCELL_DATA  p;                              \
        p = HvGetCell(Hive, Cell);                  \
        CmpUsedStorage += HvGetCellSize(Hive, p);   \
    }

//
// cmboot - functions for determining driver load lists
//

#define CM_HARDWARE_PROFILE_STR_DATABASE L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\IDConfigDB"
#define CM_HARDWARE_PROFILE_STR_CCS_HWPROFILE L"\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles"
#define CM_HARDWARE_PROFILE_STR_CCS_CURRENT L"\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current"
//
// Alias table key names in IDConfigDB
//
#define CM_HARDWARE_PROFILE_STR_ALIAS L"Alias"
#define CM_HARDWARE_PROFILE_STR_ACPI_ALIAS L"AcpiAlias"
#define CM_HARDWARE_PROFILE_STR_HARDWARE_PROFILES L"Hardware Profiles"

//
// Entries in the alias tables (value names)
//
#define CM_HARDWARE_PROFILE_STR_DOCKING_STATE L"DockingState"
#define CM_HARDWARE_PROFILE_STR_CAPABILITIES L"Capabilities"
#define CM_HARDWARE_PROFILE_STR_DOCKID L"DockID"
#define CM_HARDWARE_PROFILE_STR_SERIAL_NUMBER L"SerialNumber"
#define CM_HARDWARE_PROFILE_STR_ACPI_SERIAL_NUMBER L"AcpiSerialNumber"
#define CM_HARDWARE_PROFILE_STR_PROFILE_NUMBER L"ProfileNumber"
#define CM_HARDWARE_PROFILE_STR_ALIASABLE L"Aliasable"
#define CM_HARDWARE_PROFILE_STR_CLONED L"Cloned"
//
// Entries in the profile tables.
//
#define CM_HARDWARE_PROFILE_STR_PRISTINE L"Pristine"
#define CM_HARDWARE_PROFILE_STR_PREFERENCE_ORDER L"PreferenceOrder"
#define CM_HARDWARE_PROFILE_STR_FRIENDLY_NAME L"FriendlyName"
#define CM_HARDWARE_PROFILE_STR_CURRENT_DOCK_INFO L"CurrentDockInfo"
#define CM_HARDWARE_PROFILE_STR_HW_PROFILE_GUID L"HwProfileGuid"
//
// Entries for the root Hardware Profiles key.
//
#define CM_HARDWARE_PROFILE_STR_DOCKED L"Docked"
#define CM_HARDWARE_PROFILE_STR_UNDOCKED L"Undocked"
#define CM_HARDWARE_PROFILE_STR_UNKNOWN L"Unknown"

//
// List structure used in config manager init
//

typedef struct _HIVE_LIST_ENTRY {
    PWSTR   Name;
    PWSTR   BaseName;                       // MACHINE or USER
    PCMHIVE CmHive;
    ULONG   Flags;
} HIVE_LIST_ENTRY, *PHIVE_LIST_ENTRY;

//
// structure definitions shared with the boot loader
// to select the hardware profile.
//
typedef struct _CM_HARDWARE_PROFILE {
    ULONG   NameLength;
    PWSTR   FriendlyName;
    ULONG   PreferenceOrder;
    ULONG   Id;
    ULONG   Flags;
} CM_HARDWARE_PROFILE, *PCM_HARDWARE_PROFILE;

#define CM_HP_FLAGS_ALIASABLE  1
#define CM_HP_FLAGS_TRUE_MATCH 2
#define CM_HP_FLAGS_PRISTINE   4
#define CM_HP_FLAGS_DUPLICATE  8

typedef struct _CM_HARDWARE_PROFILE_LIST {
    ULONG MaxProfileCount;
    ULONG CurrentProfileCount;
    CM_HARDWARE_PROFILE Profile[1];
} CM_HARDWARE_PROFILE_LIST, *PCM_HARDWARE_PROFILE_LIST;

typedef struct _CM_HARDWARE_PROFILE_ALIAS {
    ULONG   ProfileNumber;
    ULONG   DockState;
    ULONG   DockID;
    ULONG   SerialNumber;
} CM_HARDWARE_PROFILE_ALIAS, *PCM_HARDWARE_PROFILE_ALIAS;

typedef struct _CM_HARDWARE_PROFILE_ALIAS_LIST {
    ULONG MaxAliasCount;
    ULONG CurrentAliasCount;
    CM_HARDWARE_PROFILE_ALIAS Alias[1];
} CM_HARDWARE_PROFILE_ALIAS_LIST, *PCM_HARDWARE_PROFILE_ALIAS_LIST;

typedef struct _CM_HARDWARE_PROFILE_ACPI_ALIAS {
    ULONG   ProfileNumber;
    ULONG   DockState;
    ULONG   SerialLength;
    PCHAR   SerialNumber;
} CM_HARDWARE_PROFILE_ACPI_ALIAS, *PCM_HARDWARE_PROFILE_ACPI_ALIAS;

typedef struct _CM_HARDWARE_PROFILE_ACPI_ALIAS_LIST {
    ULONG   MaxAliasCount;
    ULONG   CurrentAliasCount;
    CM_HARDWARE_PROFILE_ACPI_ALIAS Alias[1];
} CM_HARDWARE_PROFILE_ACPI_ALIAS_LIST, *PCM_HARDWARE_PROFILE_ACPI_ALIAS_LIST;

HCELL_INDEX
CmpFindControlSet(
     IN PHHIVE SystemHive,
     IN HCELL_INDEX RootCell,
     IN PUNICODE_STRING SelectName,
     OUT PBOOLEAN AutoSelect
     );

BOOLEAN
CmpFindDrivers(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    IN SERVICE_LOAD_TYPE LoadType,
    IN PWSTR BootFileSystem OPTIONAL,
    IN PLIST_ENTRY DriverListHead
    );

BOOLEAN
CmpFindNLSData(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    OUT PUNICODE_STRING AnsiFilename,
    OUT PUNICODE_STRING OemFilename,
    OUT PUNICODE_STRING CaseTableFilename,
    OUT PUNICODE_STRING OemHalFilename
    );

HCELL_INDEX
CmpFindProfileOption(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    OUT PCM_HARDWARE_PROFILE_LIST *ProfileList,
    OUT PCM_HARDWARE_PROFILE_ALIAS_LIST *AliasList,
    OUT PULONG Timeout
    );

VOID
CmpSetCurrentProfile(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    IN PCM_HARDWARE_PROFILE Profile
    );

BOOLEAN
CmpResolveDriverDependencies(
    IN PLIST_ENTRY DriverListHead
    );

BOOLEAN
CmpSortDriverList(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    IN PLIST_ENTRY DriverListHead
    );

HCELL_INDEX
CmpFindSubKeyByName(
    PHHIVE          Hive,
    PCM_KEY_NODE    Parent,
    PUNICODE_STRING SearchName
    );

HCELL_INDEX
CmpFindSubKeyByNumber(
    PHHIVE          Hive,
    PCM_KEY_NODE    Parent,
    ULONG           Number
    );

BOOLEAN
CmpAddSubKey(
    PHHIVE          Hive,
    HCELL_INDEX     Parent,
    HCELL_INDEX     Child
    );

BOOLEAN
CmpMarkIndexDirty(
    PHHIVE          Hive,
    HCELL_INDEX     ParentKey,
    HCELL_INDEX     TargetKey
    );

BOOLEAN
CmpRemoveSubKey(
    PHHIVE          Hive,
    HCELL_INDEX     ParentKey,
    HCELL_INDEX     TargetKey
    );

BOOLEAN
CmpGetNextName(
    IN OUT PUNICODE_STRING  RemainingName,
    OUT    PUNICODE_STRING  NextName,
    OUT    PBOOLEAN  Last
    );

NTSTATUS
CmpAddToHiveFileList(
    PCMHIVE CmHive
    );

VOID
CmpRemoveFromHiveFileList(
    );

NTSTATUS
CmpInitHiveFromFile(
    IN PUNICODE_STRING FileName,
    IN ULONG HiveFlags,
    OUT PCMHIVE *CmHive,
    IN OUT PBOOLEAN Allocate,
    IN OUT PBOOLEAN RegistryLocked
    );

NTSTATUS
CmpCloneHwProfile (
    IN HANDLE IDConfigDB,
    IN HANDLE Parent,
    IN HANDLE OldProfile,
    IN ULONG  OldProfileNumber,
    IN USHORT DockingState,
    OUT PHANDLE NewProfile,
    OUT PULONG  NewProfileNumber
    );

NTSTATUS
CmpCreateHwProfileFriendlyName (
    IN HANDLE IDConfigDB,
    IN ULONG  DockingState,
    IN ULONG  NewProfileNumber,
    OUT PUNICODE_STRING FriendlyName
    );

typedef
NTSTATUS
(*PCM_ACPI_SELECTION_ROUTINE) (
    IN  PCM_HARDWARE_PROFILE_LIST ProfileList,
    OUT PULONG ProfileIndexToUse, // Set to -1 for none.
    IN  PVOID Context
    );

NTSTATUS
CmSetAcpiHwProfile (
    IN  PPROFILE_ACPI_DOCKING_STATE DockState,
    IN  PCM_ACPI_SELECTION_ROUTINE,
    IN  PVOID Context,
    OUT PHANDLE NewProfile,
    OUT PBOOLEAN ProfileChanged
    );

NTSTATUS
CmpAddAcpiAliasEntry (
    IN HANDLE                       IDConfigDB,
    IN PPROFILE_ACPI_DOCKING_STATE  NewDockState,
    IN ULONG                        ProfileNumber,
    IN PWCHAR                       nameBuffer,
    IN PVOID                        valueBuffer,
    IN ULONG                        valueBufferLength,
    IN BOOLEAN                      PreventDuplication
    );

//
// Routines for handling registry compressed names
//
USHORT
CmpNameSize(
    IN PHHIVE Hive,
    IN PUNICODE_STRING Name
    );

USHORT
CmpCopyName(
    IN PHHIVE Hive,
    IN PWCHAR Destination,
    IN PUNICODE_STRING Source
    );

VOID
CmpCopyCompressedName(
    IN PWCHAR Destination,
    IN ULONG DestinationLength,
    IN PWCHAR Source,
    IN ULONG SourceLength
    );

LONG
CmpCompareCompressedName(
    IN PUNICODE_STRING SearchName,
    IN PWCHAR CompressedName,
    IN ULONG NameLength
    );

USHORT
CmpCompressedNameSize(
    IN PWCHAR Name,
    IN ULONG Length
    );


#endif

//
// ----- CACHED_DATA -----
//
// When values are not cached, List in ValueCache is the Hive cell index to the value list.
// When they are cached, List will be pointer to the allocation.  We distinguish them by
// marking the lowest bit in the variable to indicate it is a cached allocation.
//
// Note that the cell index for value list
// is stored in the cached allocation.  It is not used now but may be in further performance
// optimization.
//
// When value key and vaule data are cached, there is only one allocation for both.
// Value data is appended that the end of value key.  DataCacheType indicates
// whether data is cached and ValueKeySize tells how big is the value key (so
// we can calculate the address of cached value data)
//
//

PCM_NAME_CONTROL_BLOCK
CmpGetNameControlBlock(
    PUNICODE_STRING NodeName
    );

VOID
CmpDereferenceKeyControlBlockWithLock(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    );

VOID
CmpCleanUpSubKeyInfo(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    );

VOID
CmpCleanUpKcbValueCache(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    );

VOID
CmpCleanUpKcbCacheWithLock(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    );

VOID
CmpRemoveFromDelayedClose(
    IN PCM_KEY_CONTROL_BLOCK kcb
    );

PUNICODE_STRING
CmpConstructName(
    PCM_KEY_CONTROL_BLOCK kcb
);

PCELL_DATA
CmpGetValueListFromCache(
    IN PHHIVE  Hive,
    IN PCACHED_CHILD_LIST ChildList,
    IN OUT BOOLEAN    *IndexCached
);

PCM_KEY_VALUE
CmpGetValueKeyFromCache(
    IN PHHIVE         Hive,
    IN PCELL_DATA     List,
    IN ULONG          Index,
    OUT PPCM_CACHED_VALUE *ContainingList,
    IN BOOLEAN        IndexCached,
    OUT BOOLEAN    *ValueCached
);

PCM_KEY_VALUE
CmpFindValueByNameFromCache(
    IN PHHIVE  Hive,
    IN PCACHED_CHILD_LIST ChildList,
    IN PUNICODE_STRING Name,
    OUT PPCM_CACHED_VALUE *ContainingList,
    OUT ULONG *Index,
    OUT BOOLEAN *ValueCached
    );

NTSTATUS
CmpQueryKeyValueData(
    PHHIVE Hive,
    PCM_CACHED_VALUE *ContainingList,
    PCM_KEY_VALUE ValueKey,
    BOOLEAN       ValueCached,
    KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    PVOID KeyValueInformation,
    ULONG Length,
    PULONG ResultLength
    );

BOOLEAN
CmpReferenceKeyControlBlock(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    );

VOID 
CmpInitializeKeyNameString(PCM_KEY_NODE Cell, 
                           PUNICODE_STRING KeyName,
                           WCHAR *NameBuffer
                           );

VOID 
CmpInitializeValueNameString(PCM_KEY_VALUE Cell, 
                             PUNICODE_STRING ValueName,
                             WCHAR *NameBuffer
                             );

#ifdef KCB_TO_KEYBODY_LINK
VOID
CmpFlushNotifiesOnKeyBodyList(
    IN PCM_KEY_CONTROL_BLOCK   kcb
    );

#endif

extern ULONG CmpHashTableSize;
extern PCM_KEY_HASH *CmpCacheTable;

#ifdef _WANT_MACHINE_IDENTIFICATION

BOOLEAN
CmpGetBiosDateFromRegistry(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    OUT PUNICODE_STRING Date
    );

BOOLEAN
CmpGetBiosinfoFileNameFromRegistry(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    OUT PUNICODE_STRING InfName
    );


#endif

// Utility macro to set the fields of an IO_STATUS_BLOCK.  On sundown, 32bit processes
// will pass in a 32bit Iosb, and 64bit processes will pass in a 64bit Iosb.
#if defined(_WIN64)

#define CmpSetIoStatus(Iosb, s, i, UseIosb32)                              \
if ((UseIosb32)) {                                                         \
    ((PIO_STATUS_BLOCK32)(Iosb))->Status = (NTSTATUS)(s);                  \
    ((PIO_STATUS_BLOCK32)(Iosb))->Information = (ULONG)(i);                \
}                                                                          \
else {                                                                     \
    (Iosb)->Status = (s);                                                  \
    (Iosb)->Information = (i);                                             \
}                                                                          \

#else

#define CmpSetIoStatus(Iosb, s, i, UseIosb32)                              \
(Iosb)->Status = (s);                                                      \
(Iosb)->Information = (i);                                                 \

#endif
                                                               

