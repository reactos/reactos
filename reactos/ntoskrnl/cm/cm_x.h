/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/cm/cm_x.h
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
    (*Table[GET_HASH_INDEX(ConvKey)])

//
// Makes sure that the registry is locked
//
#define CMP_ASSERT_REGISTRY_LOCK                                    \
    ASSERT((CmpSpecialBootCondition == TRUE) ||                     \
           (CmpTestRegistryLock() == TRUE))

//
// Makes sure that the registry is exclusively locked
//
#define CMP_ASSERT_EXCLUSIVE_REGISTRY_LOCK                          \
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
    ExAcquirePushLockExclusive(GET_HASH_ENTRY(CmpCacheTable,        \
                                              (k)->ConvKey).Lock);  \
    GET_HASH_ENTRY(CmpCacheTable,                                   \
                   (k)->ConvKey).Owner = KeGetCurrentThread();      \
}

//
// Releases an exlusively or shared acquired KCB
//
#define CmpReleaseKcbLock(k)                                        \
{                                                                   \
    GET_HASH_ENTRY(CmpCacheTable, (k)->ConvKey).Owner = NULL;       \
    ExReleasePushLock(GET_HASH_ENTRY(CmpCacheTable,                 \
                                     (k)->ConvKey).Lock);           \
}

