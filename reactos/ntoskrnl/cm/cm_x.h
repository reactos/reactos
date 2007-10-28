/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/cm/cm_x.h
* PURPOSE:         Inlined Functions for the Configuration Manager
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

//
// Returns whether or not this is a small valued key
//
BOOLEAN
FORCEINLINE
CmpIsKeyValueSmall(OUT PULONG RealLength,
                   IN ULONG Length)
{
    /* Check if the length has the special size value */
    if (Length >= CM_KEY_VALUE_SPECIAL_SIZE)
    {
        /* It does, so this is a small key: return the real length */
        *RealLength = Length - CM_KEY_VALUE_SPECIAL_SIZE;
        return TRUE;
    }

    /* This is not a small key, return the length we read */
    *RealLength = Length;
    return FALSE;
}

//
// Returns whether or not this is a big valued key
//
BOOLEAN
FORCEINLINE
CmpIsKeyValueBig(IN PHHIVE Hive,
                 IN ULONG Length)
{
    /* Check if the hive is XP Beta 1 or newer */
    if (Hive->Version >= HSYS_WHISTLER_BETA1)
    {
        /* Check if the key length is valid for a big value key */
        if ((Length < CM_KEY_VALUE_SPECIAL_SIZE) && (Length > CM_KEY_VALUE_BIG))
        {
            /* Yes, this value is big */
            return TRUE;
        }
    }

    /* Not a big value key */
    return FALSE;
}

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
// Makes sure that the registry is exclusively locked
//
#define CMP_ASSERT_EXCLUSIVE_REGISTRY_LOCK()                        \
    ASSERT((CmpSpecialBootCondition == TRUE) ||                     \
           (CmpTestRegistryLockExclusive() == TRUE))

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
// Exclusively acquires an NCB
//
#define CmpAcquireNcbLockExclusive(n)                               \
{                                                                   \
    ExAcquirePushLockExclusive(&GET_HASH_ENTRY(CmpNameCacheTable,   \
                                              (n)->ConvKey).Lock);  \
}

//
// Releases an exlusively or shared acquired NCB
//
#define CmpReleaseNcbLock(k)                                        \
{                                                                   \
    ExReleasePushLock(&GET_HASH_ENTRY(CmpNameCacheTable,            \
                                     (k)->ConvKey).Lock);           \
}
