/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/internal/cm_x.h
* PURPOSE:         Inlined Functions for the Configuration Manager
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

//
// Returns the hashkey corresponding to a convkey
//
#define GET_HASH_KEY(ConvKey)                                       \
    ((CMP_HASH_IRRATIONAL * (ConvKey)) % CMP_HASH_PRIME)

//
// Returns the index into the hash table, or the entry itself
//
#define GET_HASH_INDEX(ConvKey)                                     \
    GET_HASH_KEY(ConvKey) % CmpHashTableSize
#define GET_HASH_ENTRY(Table, ConvKey)                              \
    (Table[GET_HASH_INDEX(ConvKey)])
#define ASSERT_VALID_HASH(h)                                        \
    ASSERT_KCB_VALID(CONTAINING_RECORD((h), CM_KEY_CONTROL_BLOCK, KeyHash))

//
// Returns whether or not the cell is cached
//
#define CMP_IS_CELL_CACHED(c)                                       \
    (((c) & HCELL_CACHED) && ((c) != HCELL_NIL))

//
// Return data from a cached cell
//
#define CMP_GET_CACHED_CELL(c)                                      \
    (ULONG_PTR)((c) & ~HCELL_CACHED)
#define CMP_GET_CACHED_DATA(c)                                      \
    (&(((PCM_CACHED_VALUE_INDEX)(CMP_GET_CACHED_CELL(c)))->Data.CellData))
#define CMP_GET_CACHED_INDEX(c)                                     \
    (&(((PCM_CACHED_ENTRY)(CMP_GET_CACHED_CELL(c)))->CellIndex))
#define CMP_GET_CACHED_VALUE(c)                                     \
    (&(((PCM_CACHED_VALUE)(CMP_GET_CACHED_CELL(c)))->KeyValue))

//
// Makes sure that the registry is locked
//
#define CMP_ASSERT_REGISTRY_LOCK()                                  \
    ASSERT((CmpSpecialBootCondition == TRUE) ||                     \
           (CmpTestRegistryLock() == TRUE))

//
// Makes sure that the registry is locked or loading
//
#define CMP_ASSERT_REGISTRY_LOCK_OR_LOADING(h)                      \
    ASSERT((CmpSpecialBootCondition == TRUE) ||                     \
           (((PCMHIVE)h)->HiveIsLoading == TRUE) ||                 \
           (CmpTestRegistryLock() == TRUE))

//
// Makes sure that the registry is exclusively locked
//
#define CMP_ASSERT_EXCLUSIVE_REGISTRY_LOCK()                        \
    ASSERT((CmpSpecialBootCondition == TRUE) ||                     \
           (CmpTestRegistryLockExclusive() == TRUE))

//
// Makes sure that the registry is exclusively locked or loading
//
#define CMP_ASSERT_EXCLUSIVE_REGISTRY_LOCK_OR_LOADING(h)            \
    ASSERT((CmpSpecialBootCondition == TRUE) ||                     \
           (((PCMHIVE)h)->HiveIsLoading == TRUE) ||                 \
           (CmpTestRegistryLockExclusive() == TRUE))

//
// Makes sure this is a valid KCB
//
#define ASSERT_KCB_VALID(k)                                         \
    ASSERT((k)->Signature == CM_KCB_SIGNATURE)

//
// Checks if a KCB is exclusively locked
//
#define CmpIsKcbLockedExclusive(k)                                  \
    (GET_HASH_ENTRY(CmpCacheTable,                                  \
                    (k)->ConvKey).Owner == KeGetCurrentThread())

//
// Exclusively acquires a KCB
//
#define CmpAcquireKcbLockExclusive(k)                               \
{                                                                   \
    ExAcquirePushLockExclusive(&GET_HASH_ENTRY(CmpCacheTable,       \
                                              (k)->ConvKey).Lock);  \
    GET_HASH_ENTRY(CmpCacheTable,                                   \
                   (k)->ConvKey).Owner = KeGetCurrentThread();      \
}

//
// Exclusively acquires a KCB by index
//
#define CmpAcquireKcbLockExclusiveByIndex(i)                        \
{                                                                   \
    ExAcquirePushLockExclusive(&CmpCacheTable[(i)].Lock);           \
    CmpCacheTable[(i)].Owner = KeGetCurrentThread();                \
}

//
// Exclusively acquires a KCB by key
//
#define CmpAcquireKcbLockExclusiveByKey(k)                          \
{                                                                   \
    ExAcquirePushLockExclusive(&GET_HASH_ENTRY(CmpCacheTable,       \
                                              (k)).Lock);           \
    GET_HASH_ENTRY(CmpCacheTable,                                   \
                   (k)).Owner = KeGetCurrentThread();               \
}


//
// Shared acquires a KCB
//
#define CmpAcquireKcbLockShared(k)                                  \
{                                                                   \
    ExAcquirePushLockShared(&GET_HASH_ENTRY(CmpCacheTable,          \
                                            (k)->ConvKey).Lock);    \
}

//
// Shared acquires a KCB by index
//
#define CmpAcquireKcbLockSharedByIndex(i)                           \
{                                                                   \
    ExAcquirePushLockShared(&CmpCacheTable[(i)].Lock);              \
}

//
// Tries to convert a KCB lock
//
FORCEINLINE
BOOLEAN
CmpTryToConvertKcbSharedToExclusive(IN PCM_KEY_CONTROL_BLOCK k)
{
    ASSERT(CmpIsKcbLockedExclusive(k) == FALSE);
    if (ExConvertPushLockSharedToExclusive(
            &GET_HASH_ENTRY(CmpCacheTable, k->ConvKey).Lock))
    {
        GET_HASH_ENTRY(CmpCacheTable,
                       k->ConvKey).Owner = KeGetCurrentThread();
        return TRUE;
    }
    return FALSE;
}

//
// Releases an exlusively or shared acquired KCB
//
#define CmpReleaseKcbLock(k)                                        \
{                                                                   \
    GET_HASH_ENTRY(CmpCacheTable, (k)->ConvKey).Owner = NULL;       \
    ExReleasePushLock(&GET_HASH_ENTRY(CmpCacheTable,                \
                                      (k)->ConvKey).Lock);          \
}

//
// Releases an exlusively or shared acquired KCB by index
//
#define CmpReleaseKcbLockByIndex(i)                                 \
{                                                                   \
    CmpCacheTable[(i)].Owner = NULL;                                \
    ExReleasePushLock(&CmpCacheTable[(i)].Lock);                    \
}

//
// Releases an exlusively or shared acquired KCB by key
//
#define CmpReleaseKcbLockByKey(k)                                   \
{                                                                   \
    GET_HASH_ENTRY(CmpCacheTable, (k)).Owner = NULL;                \
    ExReleasePushLock(&GET_HASH_ENTRY(CmpCacheTable,                \
                                      (k)).Lock);                   \
}

//
// Converts a KCB lock
//
FORCEINLINE
VOID
CmpConvertKcbSharedToExclusive(IN PCM_KEY_CONTROL_BLOCK k)
{
    ASSERT(CmpIsKcbLockedExclusive(k) == FALSE);  
    CmpReleaseKcbLock(k);
    CmpAcquireKcbLockExclusive(k);
}

//
// Exclusively acquires an NCB
//
#define CmpAcquireNcbLockExclusive(n)                               \
{                                                                   \
    ExAcquirePushLockExclusive(&GET_HASH_ENTRY(CmpNameCacheTable,   \
                                              (n)->ConvKey).Lock);  \
}

//
// Exclusively acquires an NCB by key
//
#define CmpAcquireNcbLockExclusiveByKey(k)                          \
{                                                                   \
    ExAcquirePushLockExclusive(&GET_HASH_ENTRY(CmpNameCacheTable,   \
                                               (k)).Lock);          \
}

//
// Releases an exlusively or shared acquired NCB
//
#define CmpReleaseNcbLock(k)                                        \
{                                                                   \
    ExReleasePushLock(&GET_HASH_ENTRY(CmpNameCacheTable,            \
                                     (k)->ConvKey).Lock);           \
}

//
// Releases an exlusively or shared acquired NCB by key
//
#define CmpReleaseNcbLockByKey(k)                                   \
{                                                                   \
    ExReleasePushLock(&GET_HASH_ENTRY(CmpNameCacheTable,            \
                                      (k)).Lock);                   \
}

//
// Asserts that either the registry or the hash entry is locked
//
#define CMP_ASSERT_HASH_ENTRY_LOCK(k)                               \
{                                                                   \
    ASSERT(((GET_HASH_ENTRY(CmpCacheTable, k).Owner ==              \
            KeGetCurrentThread())) ||                               \
           (CmpTestRegistryLockExclusive() == TRUE));               \
}

//
// Asserts that either the registry or the KCB is locked
//
#define CMP_ASSERT_KCB_LOCK(k)                                      \
{                                                                   \
    ASSERT((CmpIsKcbLockedExclusive(k) == TRUE) ||                  \
           (CmpTestRegistryLockExclusive() == TRUE));               \
}

//
// Gets the page attached to the KCB
//
#define CmpGetAllocPageFromKcb(k)                                   \
    (PCM_ALLOC_PAGE)(((ULONG_PTR)(k)) & ~(PAGE_SIZE - 1))

//
// Gets the page attached to the delayed allocation
//
#define CmpGetAllocPageFromDelayAlloc(a)                            \
    (PCM_ALLOC_PAGE)(((ULONG_PTR)(a)) & ~(PAGE_SIZE - 1))

//
// Makes sure that the registry is locked for flushes
//
#define CMP_ASSERT_FLUSH_LOCK(h)                                    \
    ASSERT((CmpSpecialBootCondition == TRUE) ||                     \
           (((PCMHIVE)h)->HiveIsLoading == TRUE) ||                 \
           (CmpTestHiveFlusherLockShared((PCMHIVE)h) == TRUE) ||    \
           (CmpTestHiveFlusherLockExclusive((PCMHIVE)h) == TRUE) || \
           (CmpTestRegistryLockExclusive() == TRUE));
