#ifndef _RXPREFIX_
#define _RXPREFIX_

typedef struct _RX_CONNECTION_ID
{
    union
    {
        ULONG SessionID;
        LUID Luid;
    };
} RX_CONNECTION_ID, *PRX_CONNECTION_ID;

ULONG
RxTableComputeHashValue(
    _In_ PUNICODE_STRING Name);

PVOID
RxPrefixTableLookupName(
    _In_ PRX_PREFIX_TABLE ThisTable,
    _In_ PUNICODE_STRING CanonicalName,
    _Out_ PUNICODE_STRING RemainingName,
    _In_ PRX_CONNECTION_ID ConnectionId);

PRX_PREFIX_ENTRY
RxPrefixTableInsertName(
    _Inout_ PRX_PREFIX_TABLE ThisTable,
    _Inout_ PRX_PREFIX_ENTRY ThisEntry,
    _In_ PVOID Container,
    _In_ PULONG ContainerRefCount,
    _In_ USHORT CaseInsensitiveLength,
    _In_ PRX_CONNECTION_ID ConnectionId);

VOID
RxRemovePrefixTableEntry(
    _Inout_ PRX_PREFIX_TABLE ThisTable,
    _Inout_ PRX_PREFIX_ENTRY Entry);

VOID
RxInitializePrefixTable(
    _Inout_ PRX_PREFIX_TABLE ThisTable,
    _In_opt_ ULONG TableSize,
    _In_ BOOLEAN CaseInsensitiveMatch);

typedef struct _RX_PREFIX_ENTRY
{
    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;
    USHORT CaseInsensitiveLength;
    USHORT Spare1;
    ULONG SavedHashValue;
    LIST_ENTRY HashLinks;
    LIST_ENTRY MemberQLinks;
    UNICODE_STRING Prefix;
    PULONG ContainerRefCount;
    PVOID ContainingRecord;
    PVOID Context;
    RX_CONNECTION_ID ConnectionId;
} RX_PREFIX_ENTRY, *PRX_PREFIX_ENTRY;

#define RX_PREFIX_TABLE_DEFAULT_LENGTH 32

typedef struct _RX_PREFIX_TABLE {
    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;
    ULONG Version;
    LIST_ENTRY MemberQueue;
    ERESOURCE TableLock;
    PRX_PREFIX_ENTRY TableEntryForNull;
    BOOLEAN CaseInsensitiveMatch;
    BOOLEAN IsNetNameTable;
    ULONG TableSize;
#if DBG
    ULONG Lookups;
    ULONG FailedLookups;
    ULONG Considers;
    ULONG Compares;
#endif
    LIST_ENTRY HashBuckets[RX_PREFIX_TABLE_DEFAULT_LENGTH];
} RX_PREFIX_TABLE, *PRX_PREFIX_TABLE;

#if (_WIN32_WINNT < 0x0600)
#define RxAcquirePrefixTableLockShared(T, W) RxpAcquirePrefixTableLockShared((T),(W),TRUE)
#define RxAcquirePrefixTableLockExclusive(T, W) RxpAcquirePrefixTableLockExclusive((T), (W), TRUE)
#define RxReleasePrefixTableLock(T) RxpReleasePrefixTableLock((T), TRUE)

BOOLEAN
RxpAcquirePrefixTableLockShared(
   _In_ PRX_PREFIX_TABLE pTable,
   _In_ BOOLEAN Wait,
   _In_ BOOLEAN ProcessBufferingStateChangeRequests);

BOOLEAN
RxpAcquirePrefixTableLockExclusive(
   _In_ PRX_PREFIX_TABLE pTable,
   _In_ BOOLEAN Wait,
   _In_ BOOLEAN ProcessBufferingStateChangeRequests);

VOID
RxpReleasePrefixTableLock(
   _In_ PRX_PREFIX_TABLE pTable,
   _In_ BOOLEAN ProcessBufferingStateChangeRequests);
#else
#define RxAcquirePrefixTableLockShared(T, W) ExAcquireResourceSharedLite(&(T)->TableLock, (W))
#define RxAcquirePrefixTableLockExclusive(T, W) ExAcquireResourceExclusiveLite(&(T)->TableLock, (W))
#define RxReleasePrefixTableLock(T) ExReleaseResourceLite(&(T)->TableLock)
#endif

VOID
RxExclusivePrefixTableLockToShared(
    _In_ PRX_PREFIX_TABLE Table);

#define RxIsPrefixTableLockExclusive(T) ExIsResourceAcquiredExclusiveLite(&(T)->TableLock)
#define RxIsPrefixTableLockAcquired(T) (ExIsResourceAcquiredSharedLite(&(T)->TableLock) || \
                                        ExIsResourceAcquiredExclusiveLite(&(T)->TableLock))

#ifdef __REACTOS__
#define HASH_BUCKET(T, H) &(T)->HashBuckets[H % (T)->TableSize]
#endif

#endif
