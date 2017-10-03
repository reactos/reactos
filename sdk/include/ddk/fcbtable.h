#ifndef _RXFCBTABLE_
#define _RXFCBTABLE_

typedef struct _RX_FCB_TABLE_ENTRY {
    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;
    ULONG HashValue;
    UNICODE_STRING Path;
    LIST_ENTRY HashLinks;
    LONG Lookups;
} RX_FCB_TABLE_ENTRY, *PRX_FCB_TABLE_ENTRY;

#define RX_FCB_TABLE_NUMBER_OF_HASH_BUCKETS 32

typedef struct _RX_FCB_TABLE
{
    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;
    volatile ULONG Version;
    BOOLEAN CaseInsensitiveMatch;
    USHORT NumberOfBuckets;
    volatile LONG Lookups;
    volatile LONG FailedLookups;
    volatile LONG Compares;
    ERESOURCE TableLock;
    PRX_FCB_TABLE_ENTRY TableEntryForNull;
    LIST_ENTRY HashBuckets[RX_FCB_TABLE_NUMBER_OF_HASH_BUCKETS];
} RX_FCB_TABLE, *PRX_FCB_TABLE;

VOID
RxInitializeFcbTable(
    _Inout_ PRX_FCB_TABLE FcbTable,
    _In_ BOOLEAN CaseInsensitiveMatch);

VOID
RxFinalizeFcbTable(
    _Inout_ PRX_FCB_TABLE FcbTable);

PFCB
RxFcbTableLookupFcb(
    _In_ PRX_FCB_TABLE FcbTable,
    _In_ PUNICODE_STRING Path);

NTSTATUS
RxFcbTableInsertFcb(
    _Inout_ PRX_FCB_TABLE FcbTable,
    _Inout_ PFCB Fcb);

NTSTATUS
RxFcbTableRemoveFcb(
    _Inout_ PRX_FCB_TABLE FcbTable,
    _Inout_ PFCB Fcb);

#define RxAcquireFcbTableLockShared(T, W) ExAcquireResourceSharedLite(&(T)->TableLock, W)
#define RxAcquireFcbTableLockExclusive(T, W) ExAcquireResourceExclusiveLite(&(T)->TableLock, W)
#define RxReleaseFcbTableLock(T) ExReleaseResourceLite(&(T)->TableLock)

#define RxIsFcbTableLockExclusive(T) ExIsResourceAcquiredExclusiveLite(&(T)->TableLock)

#define RxIsFcbTableLockAcquired(T) (ExIsResourceAcquiredSharedLite(&(T)->TableLock) ||  \
                        	     ExIsResourceAcquiredExclusiveLite(&(T)->TableLock))

#ifdef __REACTOS__
#define FCB_HASH_BUCKET(T, H) &(T)->HashBuckets[H % (T)->NumberOfBuckets]
#endif

#endif
