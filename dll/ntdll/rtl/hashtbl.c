// Placeholder for RTL_DYNAMIC_HASH_TABLE structure and related functions
// For tests, create a separate file per each function under
// .\reactos\modules\rostests\apitests\ntdll
// TODO: Write tests per public function

/*
 * PROJECT:     ReactOS NT User-Mode DLL
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Hashtable implementation in kernel
 * COPYRIGHT:   Copyright 2023 Zafer Balkan <zafer@zaferbalkan.com>
 */

/**
 * Note: Currently only supports "weak" enumeration. "Weak"
 * enumeration means enumeration that requires exclusive access to the table
 * during the entire enumeration.
 */

/*
Questions:
1. Where to use  #if (NTDDI_VERSION >= NTDDI_WIN7)
2. Is NT_ASSERT the correct assert function?
3. Is the #define NDEBUG applicable to this one?
4. Are the undocumented utility method signatures okay for this structure?
5. RTL_DYNAMIC_HASH_TABLE_CONTRACT_SUPPORT
*/
#if (NTDDI_VERSION >= NTDDI_WIN7)

/* INCLUDES *****************************************************************/
#include <ntdll.h>
#include <stdint.h>

#define NDEBUG
#include <debug.h>

/* DEFINES *****************************************************************/

#define RTL_DYNAMIC_HASH_TABLE_MIN_SIZE 128

#define RTL_HASH_RESERVED_SIGNATURE 0

// Inserts with hash = RTL_HASH_RESERVED_SIGNATURE aren't allowed.
#define RTL_HASH_ALT_SIGNATURE (RTL_HASH_RESERVED_SIGNATURE + 1)

// Define table sizes.

#define HT_FIRST_LEVEL_DIR_SIZE 16
#define HT_SECOND_LEVEL_DIR_SHIFT 7
#define HT_SECOND_LEVEL_DIR_MIN_SIZE (1 << HT_SECOND_LEVEL_DIR_SHIFT)
#define HT_FIRST_LEVEL_DIR_ALLOCATION sizeof(LIST_ENTRY *) * HT_FIRST_LEVEL_DIR_SIZE
#define HT_SECOND_LEVEL_DIR_ALLOCATION RtlComputeSecondLevelDirSize(0) * sizeof(_LIST_ENTRY)

// First level dir[0] covers a mininum-size 2nd-level dir.
// First level dir[1] covers a 2*minimum-size 2nd-level dir.
// First level dir[2] covers a 4*minimum-size 2nd-level dirs. So on...
// Hence, we can have at most (2^HT_FIRST_LEVEL_DIR_SIZE)-1
// minimum-size hash bucket directories.
// With a first-level directory size of 16 and a 2nd-level directory
// minimum-size of 128, we get a max hash table size of 8,388,480 buckets.
#define MAX_HASH_TABLE_SIZE (((1 << HT_FIRST_LEVEL_DIR_SIZE) - 1) * HT_SECOND_LEVEL_DIR_MIN_SIZE)

#define BASE_HASH_TABLE_SIZE HT_SECOND_LEVEL_DIR_MIN_SIZE

// 1 << 7 will always match to 128. Why this assert?
// Note: "Hash table sizes should match!"
NT_ASSERT(RTL_DYNAMIC_HASH_TABLE_MIN_SIZE == BASE_HASH_TABLE_SIZE);

// The maximum number of hash table resizes allowed at a time. This limits
// the time spent expanding/contracting a hash table at dispatch.
#define RTL_DYNAMIC_HASH_TABLE_MAX_RESIZE_ATTEMPTS 1

// The maximum average chain length in a hash table bucket. If a hash table's
// average chain length goes above this limit, it needs to be expanded.
#define RTL_DYNAMIC_HASH_TABLE_MAX_CHAIN_LENGTH 4

// The maximum percentage of empty buckets in the hash table. If a hash table
// has more empty buckets, it needs to be contracted.
#define RTL_DYNAMIC_HASH_TABLE_MAX_EMPTY_BUCKET_PERCENTAGE 25

#ifndef BitScanReverse
/**
 * @brief Find the most significant set bit.
 *
 * @param Index - Returns the most significant set bit.
 * @param Mask - Mask to find most signifcant set bit in.
 *
 * @return 1 if most significant set bit is found, 0 if no bit is set.
 */
static uint8_t
RtlBitScanReverse(_Out_ uint32_t *Index, _In_ uint32_t Mask)
{
    int ii = 0;

    if (Mask == 0 || Index == 0)
    {
        return 0;
    }

    for (ii = (sizeof(uint32_t) * 8); ii >= 0; --ii)
    {
        uint32_t TempMask = 1UL << ii;

        if ((Mask & TempMask) != 0)
        {
            *Index = ii;
            break;
        }
    }

    return (ii >= 0 ? (uint8_t)1 : (uint8_t)0);
}
#else
#define RtlBitScanReverse(A, B) BitScanReverse((ULONG *)A, (ULONG)B)
#endif // BitScanReverse

// tags used for marking allocations.
// Using the same tags MSQUIC uses for now.
#define TAG_HASHTABLE 'E2cQ'        // Qc2E - hashtable
#define TAG_HASHTABLE_MEMBER 'F2cQ' // Qc2F - hashtable member lists

/* FUNCTIONS ***************************************************************/

#pragma region UTILITY_FUNCTIONS
/**
 * @brief Given a bucket index, computes the first level dir index that points to the
 * corresponding second level dir, and the second level dir index that points
 * to the hash bucket.
 *
 * @param BucketIndex - [0, MAX_HASH_TABLE_SIZE-1]
 * @param FirstLevelIndex - Pointer to a uint32_t that will be assigned the first
 * level index upon return.
 * @param SecondLevelIndex - Pointer to a uint32_t that will be assigned the second
 * level index upon return.
 */
static
VOID
RtlComputeDirIndices(
    _In_range_(<, MAX_HASH_TABLE_SIZE) uint32_t BucketIndex,
    _Out_range_(<, HT_FIRST_LEVEL_DIR_SIZE) uint32_t *FirstLevelIndex,
    _Out_range_(<, 1 << (*FirstLevelIndex + HT_SECOND_LEVEL_DIR_SHIFT)) uint32_t *SecondLevelIndex)
{
    NT_ASSERT(BucketIndex < MAX_HASH_TABLE_SIZE);

    uint32_t AbsoluteIndex = BucketIndex + HT_SECOND_LEVEL_DIR_MIN_SIZE;
    NT_ASSERT(AbsoluteIndex != 0);

    // Find the most significant set bit. Since AbsoluteIndex is always nonzero,
    // we don't need to check the return value.
    RtlBitScanReverse(FirstLevelIndex, AbsoluteIndex);

    // The second level index is the absolute index with the most significant
    // bit cleared.
    *SecondLevelIndex = (AbsoluteIndex ^ (1 << *FirstLevelIndex));

    // The first level index is the position of the most significant bit
    // adjusted for the size of the minimum second level dir size.
    *FirstLevelIndex -= HT_SECOND_LEVEL_DIR_SHIFT;

    NT_ASSERT(*FirstLevelIndex < HT_FIRST_LEVEL_DIR_SIZE);
}

/**
 * @brief Computes size of 2nd level directory. The size of the second level dir is
 * determined by its position in the first level dir.
 *
 * @param FirstLevelIndex - The first level index.
 *
 * @return The directory size.
 */
static
uint32_t
RtlComputeSecondLevelDirSize(_In_range_(<, HT_FIRST_LEVEL_DIR_SIZE) uint32_t FirstLevelIndex)
{
    return (1 << (FirstLevelIndex + HT_SECOND_LEVEL_DIR_SHIFT));
}

/*
 * @brief Initializes a second level dir.
 *
 * @param SecondLevelDir - The 2nd level dir to initialize.
 * @param NumberOfBucketsToInitialize - Number of buckets to initialize.
 */
static
VOID
RtlInitializeSecondLevelDir(
    _Out_writes_all_(NumberOfBucketsToInitialize) PLIST_ENTRY *SecondLevelDir,
    _In_ uint32_t NumberOfBucketsToInitialize)
{
    for (uint32_t i = 0; i < NumberOfBucketsToInitialize; i += 1)
    {
        InitializeListHead(&SecondLevelDir[i]);
    }
}

/**
 * @brief Returns a bucket index of a Signature within a given HashTable
 *
 * @param HashTable - Pointer to hash table to operate on.
 * @param Signature - The signature.
 *
 * Synchronization:none
 *
 * @return Returns the index of the bucket within the HashTable
 */
static
uint32_t
RtlGetBucketIndex(
    _In_ const RTL_DYNAMIC_HASH_TABLE *HashTable,
    _In_ ULONG_PTR Signature)
{
    uint32_t BucketIndex = ((uint32_t)Signature) & HashTable->DivisorMask;
    if (BucketIndex < HashTable->Pivot)
    {
        BucketIndex = ((uint32_t)Signature) & ((HashTable->DivisorMask << 1) | 1);
    }
    return BucketIndex;
}

/**
 * @brief Does the basic hashing and lookup and returns a pointer to either the entry
 * before the entry with the queried signature, or to the entry after which
 * such a entry would exist (if it doesn't exist).
 *
 * @param HashTable - Pointer to hash table to operate on.
 * @param Context - The context structure that is to be filled.
 * @param Signature - The signature to be looked up.
 *
 * Synchronization: Hash Table lock should be held in shared mode by caller.
 *
 * @return Returns nothing, but fills the Context structure with the relevant information.
 */
static
VOID
RtlPopulateContext(
    _In_ const RTL_DYNAMIC_HASH_TABLE *HashTable,
    _Out_ RTL_DYNAMIC_HASH_TABLE_LOOKUP_CONTEXT *Context,
    _In_ ULONG_PTR Signature)
{
    // Compute the hash.
    uint32_t BucketIndex = RtlGetBucketIndex(HashTable, Signature);

    LIST_ENTRY *BucketPtr = RtlGetChainHead(HashTable, BucketIndex);
    NT_ASSERT(NULL != BucketPtr);

    LIST_ENTRY *CurEntry = BucketPtr;
    while (CurEntry->Flink != BucketPtr)
    {
        LIST_ENTRY *NextEntry = CurEntry->Flink;
        RTL_DYNAMIC_HASH_TABLE_ENTRY *NextHashEntry = RtlFlinkToHashEntry(&NextEntry->Flink);

        if ((RTL_HASH_RESERVED_SIGNATURE == NextHashEntry->Signature) || (NextHashEntry->Signature < Signature))
        {
            CurEntry = NextEntry;
            continue;
        }

        break;
    }

    // At this point, the signature is either equal or greater, or the end of
    // the chain. Either way, this is where we want to be.
    Context->ChainHead = BucketPtr;
    Context->PrevLinkage = CurEntry;
    Context->Signature = Signature;
}

/**
 * @brief Given a table index, it retrieves the pointer to the head of the hash chain.
 * This routine expects that the index passed will be less than the table size.
 * N.B. It was initially designed such that if the index asked for is greater
 * than table size, this routine should just increase the table size so that
 * the index asked for exists. But that increases the path length for the
 * regular callers, and so that functionality was removed.
 *
 * @param HashTable - Pointer to hash table to operate on.
 * @param BucketIndex - Index of chain to be returned.
 * Synchronization: Hash table lock should be held in shared mode by caller.
 *
 * @return Returns the pointer to the head of the hash chain.
 */
static
LIST_ENTRY *
RtlGetChainHead(
  _In_ const RTL_DYNAMIC_HASH_TABLE *HashTable,
  _In_range_(<, HashTable->TableSize) uint32_t BucketIndex)
{
    uint32_t SecondLevelIndex;
    LIST_ENTRY *SecondLevelDir;

    NT_ASSERT(BucketIndex < HashTable->TableSize);

    // 'Directory' field of the hash table points either
    // to the first level directory or to the second-level directory
    // itself depending to the allocated size..

    if (HashTable->TableSize <= HT_SECOND_LEVEL_DIR_MIN_SIZE)
    {
        SecondLevelDir = HashTable->SecondLevelDir;
        SecondLevelIndex = BucketIndex;
    }
    else
    {
        uint32_t FirstLevelIndex = 0;
        RtlComputeDirIndices(BucketIndex, &FirstLevelIndex, &SecondLevelIndex);
        SecondLevelDir = *(HashTable->FirstLevelDir + FirstLevelIndex);
    }

    NT_ASSERT(SecondLevelDir != NULL);

    return SecondLevelDir + SecondLevelIndex;
}

uint32_t
RtlHashtableGetEmptyBuckets(_In_ const RTL_DYNAMIC_HASH_TABLE *HashTable)
{
    NT_ASSERT(HashTable->TableSize >= HashTable->NonEmptyBuckets);
    return HashTable->TableSize - HashTable->NonEmptyBuckets;
}

/**
 * @brief Converts the pointer to the Flink in LIST_ENTRY into a RTL_DYNAMIC_HASH_TABLE_ENTRY
 * structure.
 *
 * @param FlinkPtr - supplies the pointer to the Flink field in LIST_ENTRY
 *
 * @return Returns the RTL_DYNAMIC_HASH_TABLE_ENTRY that contains the LIST_ENTRY which contains
 * the Flink whose pointer was passed above.
 */
static
PRTL_DYNAMIC_HASH_TABLE_ENTRY
RtlFlinkToHashEntry(_In_ LIST_ENTRY **FlinkPtr)
{
    // TODO: Where does this Linkage come from?
    return (PRTL_DYNAMIC_HASH_TABLE_ENTRY)_SEH_CONTAINING_RECORD(FlinkPtr, RTL_DYNAMIC_HASH_TABLE_ENTRY, Linkage);
}

#pragma endregion

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlCreateHashTable(
    _Inout_ _When_(NULL == *HashTable, __drv_allocatesMem(Mem)) RTL_DYNAMIC_HASH_TABLE **HashTable,
    _In_ ULONG Shift,
    _In_ _Reserved_ ULONG Flags)
{
    DPRINT1("RtlCreateHashTable() called\n");

    uint16_t InitialSize = RTL_DYNAMIC_HASH_TABLE_MIN_SIZE;

    // TODO: If it is 128 all the time, why there is a check?
    // There must be another function, probably allowing InitialSize as a parameter, and for
    // undocumented internal use. By default, it is 128 here. When the other data structure is
    // discovered, we can create the new internal function. Then we can push all this code into
    // internal function, with RTL_DYNAMIC_HASH_TABLE_MIN_SIZE as parameter.

    // Initial size must be a power of two and within the allowed range.
    // if (!IS_POWER_OF_TWO(InitialSize) ||
    //     (InitialSize > MAX_HASH_TABLE_SIZE) ||
    //     (InitialSize < BASE_HASH_TABLE_SIZE)) {
    //     return FALSE;
    //     }

    // First allocate the hash table header.
    uint32_t LocalFlags = 0;
    RTL_DYNAMIC_HASH_TABLE **Table;
    if (*HashTable == NULL)
    {
        Table = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(RTL_DYNAMIC_HASH_TABLE), TAG_HASHTABLE);
        if (Table == NULL)
        {
            DPRINT1("Allocation of '%s' failed. (%llu bytes)", "RTL_DYNAMIC_HASH_TABLE", sizeof(RTL_DYNAMIC_HASH_TABLE));
            return FALSE;
        }

        LocalFlags = RTL_HASH_ALLOCATED_HEADER;
    }
    else
    {
        Table = *HashTable;
    }

    RtlZeroMemory(Table, sizeof(RTL_DYNAMIC_HASH_TABLE));
    Table->Flags = LocalFlags | Flags;
    Table->TableSize = InitialSize;
    Table->DivisorMask = Table->TableSize - 1;
    Table->Pivot = 0;
    Table->Shift = Shift; // TODO: Not in the msquic version. Find where it is used.

    // Now we allocate the second level entries.
    // Since TableSize = InitialSize = RTL_DYNAMIC_HASH_TABLE_MIN_SIZE = 128,
    // It is always equal to HT_SECOND_LEVEL_DIR_MIN_SIZE.
    // This check also points an undocumented internal function with a different initial size.
    if (Table->TableSize <= HT_SECOND_LEVEL_DIR_MIN_SIZE)
    {
        // Directory pointer in the Table header structure points directly to
        // the single second-level directory.
        PVOID SecondLevelDir;
        SecondLevelDir = ExAllocatePoolWithTag(NonPagedPoolNx, HT_SECOND_LEVEL_DIR_ALLOCATION, TAG_HASHTABLE_MEMBER);

        if (SecondLevelDir == NULL)
        {
            DPRINT1("Allocation of '%s' failed. (%llu bytes)", "second level dir (0)", HT_SECOND_LEVEL_DIR_ALLOCATION);
            RtlDeleteHashTable(Table);
            return FALSE;
        }

        Table->Directory = SecondLevelDir;

        RtlInitializeSecondLevelDir(Table->Directory, Table->TableSize);
    }
    else
    {
        // In current configuration (InitialSize = RTL_DYNAMIC_HASH_TABLE_MIN_SIZE), this block will
        // never be used.

        // Allocate and initialize the first-level directory entries required to
        // fit upper bound.
        uint32_t FirstLevelIndex = 0, SecondLevelIndex = 0;
        RtlComputeDirIndices((Table->TableSize - 1), &FirstLevelIndex, &SecondLevelIndex);

        PVOID FirstLevelDir;
        FirstLevelDir = ExAllocatePoolWithTag(NonPagedPoolNx, HT_FIRST_LEVEL_DIR_ALLOCATION, TAG_HASHTABLE_MEMBER);
        if (FirstLevelDir == NULL)
        {
            RtlDeleteHashTable(Table);
            return FALSE;
        }

        RtlZeroMemory(FirstLevelDir, HT_FIRST_LEVEL_DIR_ALLOCATION);

        for (uint32_t i = 0; i <= FirstLevelIndex; i++)
        {
            FirstLevelDir[i] =
                ExAllocatePoolWithTag(NonPagedPoolNx, HT_SECOND_LEVEL_DIR_ALLOCATION, TAG_HASHTABLE_MEMBER);
            if (FirstLevelDir[i] == NULL)
            {
                DPRINT1("Allocation of '%s' failed. (%llu bytes)", "second level dir (0)", HT_SECOND_LEVEL_DIR_ALLOCATION);
                RtlDeleteHashTable(Table);
                return FALSE;
            }

            RtlInitializeSecondLevelDir(
                FirstLevelDir[i], (i < FirstLevelIndex) ? RtlComputeSecondLevelDirSize(i) : (SecondLevelIndex + 1));
        }

        Table->Direcory = FirstLevelDir;
    }

    *HashTable = Table;

    return TRUE;
}

/**
 * @brief Called to remove all resources allocated either in RtlCreateHashTable,
 * or later while expanding the table. This function just walks the entire
 * table checking that all hash buckets are null, and then removing all the
 * memory allocated for the directories behind it. This function is also called
 * from RtlCreateHashTable to cleanup the allocations just in case, an
 * error occurs (like failed memory allocation).
 *
 * Synchronization: None
 * @param HashTable - Pointer to hash Table to be deleted.
 */
NTSYSAPI
VOID
NTAPI
RtlDeleteHashTable(
  _In_ _When_((HashTable->Flags & RTL_HASH_ALLOCATED_HEADER), __drv_freesMem(Mem) _Post_invalid_)
                       RTL_DYNAMIC_HASH_TABLE *HashTable)
{
    DPRINT1("RtlDeleteHashTable() called\n");
    NT_ASSERT(HashTable == NULL);
    NT_ASSERT(HashTable->NumEnumerators == 0);
    NT_ASSERT(HashTable->NumEntries == 0);
    // Since TableSize = InitialSize = RTL_DYNAMIC_HASH_TABLE_MIN_SIZE = 128,
    // It is always equal to HT_SECOND_LEVEL_DIR_MIN_SIZE.
    // This check also points an undocumented internal function with a different initial size.
    if (HashTable->TableSize <= HT_SECOND_LEVEL_DIR_MIN_SIZE)
    {
        if (HashTable->SecondLevelDir != NULL)
        {
            RtlZeroMemory(HashTable, sizeof(RTL_DYNAMIC_HASH_TABLE));
            ExFreePoolWithTag(HashTable->SecondLevelDir, TAG_HASHTABLE_MEMBER);
            HashTable->SecondLevelDir = NULL;
        }
    }
    else
    {
        // In current configuration (InitialSize = RTL_DYNAMIC_HASH_TABLE_MIN_SIZE), this block will
        // never be used.
        if (HashTable->FirstLevelDir != NULL)
        {
#if DEBUG
            uint32_t largestFirstLevelIndex = 0, largestSecondLevelIndex = 0;
            RtlComputeDirIndices((HashTable->TableSize - 1), &largestFirstLevelIndex, &largestSecondLevelIndex);
#endif

            uint32_t FirstLevelIndex;
            for (FirstLevelIndex = 0; FirstLevelIndex < HT_FIRST_LEVEL_DIR_SIZE; FirstLevelIndex++)
            {
                LIST_ENTRY *SecondLevelDir = HashTable->FirstLevelDir[FirstLevelIndex];
                if (NULL == SecondLevelDir)
                {
                    break;
                }

#if DEBUG
                uint32_t initializedBucketCountInSecondLevelDir = (FirstLevelIndex < largestFirstLevelIndex)
                                                                      ? RtlComputeSecondLevelDirSize(FirstLevelIndex)
                                                                      : largestSecondLevelIndex + 1;

                for (uint32_t SecondLevelIndex = 0; SecondLevelIndex < initializedBucketCountInSecondLevelDir;
                     SecondLevelIndex++)
                {
                    NT_ASSERT(IsListEmpty(&SecondLevelDir[SecondLevelIndex]));
                }
#endif
                FreeEnvironmentStrings(SecondLevelDir);
            }

#if DEBUG
            for (; FirstLevelIndex < HT_FIRST_LEVEL_DIR_SIZE; FirstLevelIndex++)
            {
                NT_ASSERT(NULL == HashTable->FirstLevelDir[FirstLevelIndex]);
            }
#endif

            ExFreePoolWithTag(HashTable->FirstLevelDir, TAG_HASHTABLE_MEMBER);
            HashTable->FirstLevelDir = NULL;
        }
    }

    if (HashTable->Flags & RTL_HASH_ALLOCATED_HEADER)
    {
        ExFreePoolWithTag(HashTable, TAG_HASHTABLE);
        HashTable = NULL;
    }
}

/**
 * @brief Inserts an entry into a hash table, given the pointer to a
 * RTL_DYNAMIC_HASH_TABLE_ENTRY and a signature. An optional context can be passed in
 * which, if possible, will be used to quickly get to the relevant bucket chain.
 * This routine will not take the contents of the Context structure passed in
 * on blind faith -- it will check if the signature in the Context structure
 * matches the signature of the entry that needs to be inserted. This adds an
 * extra check on the hot path, but I deemed it necessary.
 *
 * Synchronization: Hash lock has to be held by caller in exclusive mode.
 * @param HashTable - Pointer to hash table in which we wish to insert entry
 * @param Entry - Pointer to entry to be inserted.
 * @param Signature - Signature of the entry to be inserted.
 * @param Context - Pointer to optional context that can be passed in.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlInsertEntryHashTable(
    _In_ RTL_DYNAMIC_HASH_TABLE *HashTable,
    _In_ __drv_aliasesMem RTL_DYNAMIC_HASH_TABLE_ENTRY *Entry,
    _In_ ULONG_PTR Signature,
    _Inout_opt_ RTL_DYNAMIC_HASH_TABLE_CONTEXT *Context)
{
    DPRINT1("RtlInsertEntryHashTable() called\n");
    RTL_DYNAMIC_HASH_TABLE_LOOKUP_CONTEXT LocalContext;
    RTL_DYNAMIC_HASH_TABLE_LOOKUP_CONTEXT *ContextPtr = NULL;

    if (Signature == RTL_HASH_RESERVED_SIGNATURE)
    {
        Signature = RTL_HASH_ALT_SIGNATURE;
    }

    Entry->Signature = Signature;

    HashTable->NumEntries++;

    if (Context == NULL)
    {
        RtlPopulateContext(HashTable, &LocalContext, Signature);
        ContextPtr = &LocalContext;
    }
    else
    {
        if (Context->ChainHead == NULL)
        {
            RtlPopulateContext(HashTable, Context, Signature);
        }

        NT_ASSERT(Signature == Context->Signature);
        ContextPtr = Context;
    }

    NT_ASSERT(ContextPtr->ChainHead != NULL);

    if (IsListEmpty(ContextPtr->ChainHead))
    {
        HashTable->NonEmptyBuckets++;
    }

    InitializeListHead(ContextPtr->PrevLinkage, &Entry->Linkage);

    // Expand the table if necessary.
    if (HashTable->NumEntries > RTL_DYNAMIC_HASH_TABLE_MAX_CHAIN_LENGTH * HashTable->NonEmptyBuckets)
    {
        uint32_t RestructAttempts = RTL_DYNAMIC_HASH_TABLE_MAX_RESIZE_ATTEMPTS;
        do
        {
            if (!RtlHashTableExpand(HashTable))
            {
                break;
            }

            RestructAttempts--;

        } while ((RestructAttempts > 0) &&
                 (HashTable->NumEntries > RTL_DYNAMIC_HASH_TABLE_MAX_CHAIN_LENGTH * HashTable->NonEmptyBuckets));
    }
}

/**
 * @brief This function will remove an entry from the hash table. Since the bucket
 * chains are doubly-linked lists, removal does not require identification of
 * the bucket, and is a local operation.
 *
 * If a Context is specified, the function takes care of both possibilities --
 * if the Context is already filled, it remains untouched, otherwise, it is
 * filled appropriately.
 *
 * Synchronization: Requires the caller to hold the lock protecting the hash table in
 * exclusive-mode
 *
 * @param HashTable - Pointer to hash table from which the entry is to be removed.
 * @param Entry - Pointer to the entry that is to be removed.
 * @param Context - Optional pointer which stores information about the location in
 * about the location in the hash table where that particular signature
 * resides.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlRemoveEntryHashTable(
    _In_ RTL_DYNAMIC_HASH_TABLE *HashTable,
    _In_ RTL_DYNAMIC_HASH_TABLE_ENTRY *Entry,
    _Inout_opt_ RTL_DYNAMIC_HASH_TABLE_CONTEXT *Context)
{
    ULONG_PTR Signature = Entry->Signature;

    NT_ASSERT(HashTable->NumEntries > 0);
    HashTable->NumEntries--;

    if (Entry->Linkage.Flink == Entry->Linkage.Blink)
    {
        // This is the last element in this chain.
        NT_ASSERT(HashTable->NonEmptyBuckets > 0);
        HashTable->NonEmptyBuckets--;
    }

    RemoveEntryList(&Entry->Linkage);

    if (Context != NULL)
    {
        if (Context->ChainHead == NULL)
        {
            RtlPopulateContext(HashTable, Context, Signature);
        }
        else
        {
            NT_ASSERT(Signature == Context->Signature);
        }
    }

#if RTL_DYNAMIC_HASH_TABLE_CONTRACT_SUPPORT
    // Contract the table if necessary.
    uint32_t EmptyBuckets = RtlHashtableGetEmptyBuckets(HashTable);
    if (EmptyBuckets > (RTL_DYNAMIC_HASH_TABLE_MAX_EMPTY_BUCKET_PERCENTAGE * HashTable->TableSize / 100))
    {
        uint32_t RestructAttempts = RTL_DYNAMIC_HASH_TABLE_MAX_RESIZE_ATTEMPTS;
        do
        {
            if (!RtlHashTableContract(HashTable))
            {
                break;
            }

            EmptyBuckets = RtlEmptyBucketsHashTable(HashTable);
            RestructAttempts--;

        } while ((RestructAttempts > 0) &&
                 (EmptyBuckets > (RTL_DYNAMIC_HASH_TABLE_MAX_EMPTY_BUCKET_PERCENTAGE * HashTable->TableSize / 100)));
    }
#endif // RTL_DYNAMIC_HASH_TABLE_CONTRACT_SUPPORT
}

/**
 * @brief This function will look up an entry in the hash table. Since our hash table
 * only recognizes signatures, lookups need to generate all possible matches
 * for the requested signature. This is achieved by storing all entries with
 * the same signature in a contiguous subsequence, and returning the
 * subsequence. The caller can walk this subsequence by calling
 * RtlGetNextEntryHashTable. If specified, the context is always initialized in
 * this operation.
 *
 * @param HashTable - Pointer to the hash table in which the signature is to be looked
 * up.
 * @param Signature - Signature to be looked up.
 * @param Context - Optional pointer which stores information about the location in
 * the hash table where that particular signature resides.
 *
 * @return Returns the first hash entry found that matches the signature. All the other
 * hash entries with the same signature are linked behind this value.
 */
_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlLookupEntryHashTable(
    _In_ RTL_DYNAMIC_HASH_TABLE *HashTable,
    _In_ ULONG_PTR Signature,
    _Out_opt_ RTL_DYNAMIC_HASH_TABLE_CONTEXT *Context)
{
    DPRINT1("RtlLookupEntryHashTable() called\n");
    if (Signature == RTL_HASH_RESERVED_SIGNATURE)
    {
        Signature = RTL_HASH_ALT_SIGNATURE;
    }

    RTL_DYNAMIC_HASH_TABLE_LOOKUP_CONTEXT LocalContext;
    RTL_DYNAMIC_HASH_TABLE_LOOKUP_CONTEXT *ContextPtr =
        (Context != NULL) ? Context : &LocalContext; // cppcheck-suppress uninitvar

    RtlPopulateContext(HashTable, ContextPtr, Signature);

    LIST_ENTRY *CurEntry = ContextPtr->PrevLinkage->Flink;
    if (ContextPtr->ChainHead == CurEntry)
    {
        return NULL;
    }

    RTL_HASHTABLE_ENTRY *CurHashEntry = RtlFlinkToHashEntry(&CurEntry->Flink);

    // RtlPopulateContext will never return a PrevLinkage whose next points to
    // an enumerator.
    NT_ASSERT(RTL_HASH_RESERVED_SIGNATURE != CurHashEntry->Signature);

    if (CurHashEntry->Signature == Signature)
    {
        return CurHashEntry;
    }

    return NULL;
}

/**
 * @brief This function will continue a lookup on a hash table. We assume that the
 * user is not stupid and will call it only after Lookup has returned a
 * non-NULL entry.
 *
 * Also note that this function has the responsibility to skip through any
 * enumerators that may be in the chain. In such a case, the Context structure's
 * PrevLinkage will *still* point to the last entry WHICH IS NOT A ENUMERATOR.
 *
 * @param HashTable - Pointer to the hash table in which the lookup is to be performed
 * @param Context - Pointer to context which remains untouched during this operation.
 * However that entry must be non-NULL so that we can figure out whether we
 * have reached the end of the list.
 * @return Returns the next entry with the same signature as the entry passed in, or
 * NULL if no such entry exists.
 */
_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlGetNextEntryHashTable(_In_ RTL_DYNAMIC_HASH_TABLE *HashTable, _In_ RTL_DYNAMIC_HASH_TABLE_CONTEXT *Context)
{
    DPRINT1("RtlGetNextEntryHashTable() called\n");

    NT_ASSERT(NULL != Context);
    NT_ASSERT(NULL != Context->ChainHead);
    NT_ASSERT(Context->PrevLinkage->Flink != Context->ChainHead);

    // We know that the next entry is a valid, kosher entry,
    LIST_ENTRY *CurEntry = Context->PrevLinkage->Flink;
    NT_ASSERT(CurEntry != Context->ChainHead);
    NT_ASSERT(RTL_HASH_RESERVED_SIGNATURE != (RtlFlinkToHashEntry(&CurEntry->Flink)->Signature));

    // Is this the end of the chain?
    if (CurEntry->Flink == Context->ChainHead)
    {
        return NULL;
    }

    LIST_ENTRY *NextEntry;
    RTL_DYNAMIC_HASH_TABLE_ENTRY *NextHashEntry;
    if (HashTable->NumEnumerators == 0)
    {
        NextEntry = CurEntry->Flink;
        NextHashEntry = RtlFlinkToHashEntry(&NextEntry->Flink);
    }
    else
    {
        NT_ASSERT(CurEntry->Flink != Context->ChainHead);
        NextHashEntry = NULL;
        while (CurEntry->Flink != Context->ChainHead)
        {
            NextEntry = CurEntry->Flink;
            NextHashEntry = RtlFlinkToHashEntry(&NextEntry->Flink);

            if (RTL_HASH_RESERVED_SIGNATURE != NextHashEntry->Signature)
            {
                break;
            }

            CurEntry = NextEntry;
        }
    }

    NT_ASSERT(NextHashEntry != NULL);
    if (NextHashEntry->Signature == Context->Signature)
    {
        Context->PrevLinkage = CurEntry;
        return NextHashEntry;
    }

    // If we have found no other entry matching that signature, the Context
    // remains untouched, free for the caller to use for other insertions and
    // removals.
    return NULL;
}

/**
 * @brief This routine initializes state for the main type of enumeration supported
 * in which the lock is held during the entire duration of the enumeration.
 * Currently, the enumeration always starts from the start of the table and
 * proceeds till the end, but we leave open the possibility that the Context
 * passed in will be used to initialize the place from which the enumeration
 * starts.
 *
 * This routine also increments the counter in the hash table tracking the
 * number of enumerators active on the hash table -- as long as this number is
 * positive, no hash table restructuring is possible.
 *
 * Synchronization: The lock protecting the hash table must be acquired in exclusive mode.
 *
 * @param HashTable - Pointer to hash Table on which the enumeration will take place.
 * @param Enumerator - Pointer to RTL_DYNAMIC_HASH_TABLE_ENUMERATOR structure that stores
 * enumeration state.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlInitEnumerationHashTable(_In_ RTL_DYNAMIC_HASH_TABLE *HashTable, _Out_ RTL_DYNAMIC_HASH_TABLE_ENUMERATOR *Enumerator)
{
    DPRINT1("RtlInitEnumerationHashTable() called\n");
    NT_ASSERT(Enumerator != NULL);

    RTL_DYNAMIC_HASH_TABLE_LOOKUP_CONTEXT LocalContext;
    RtlPopulateContext(HashTable, &LocalContext, 0);
    HashTable->NumEnumerators++;

    if (IsListEmpty(LocalContext.ChainHead))
    {
        HashTable->NonEmptyBuckets++;
    }

    InitializeListHead(LocalContext.ChainHead, &(Enumerator->HashEntry.Linkage));
    Enumerator->BucketIndex = 0;
    Enumerator->ChainHead = LocalContext.ChainHead;
    Enumerator->HashEntry.Signature = RTL_HASH_RESERVED_SIGNATURE;
}

/**
 * @brief Get the next entry to be enumerated. If the hash chain still has entries
 * that haven't been given to the user, the next such entry in the hash chain
 * is returned. If the hash chain has ended, this function searches for the
 * next non-empty hash chain and returns the first element in that chain. If no
 * more non-empty hash chains exists, the function returns NULL. The caller
 * must call RtlHashtableEnumerateEnd() to explicitly end the enumeration and
 * cleanup state.
 * This call is robust in the sense, that if this function returns NULL,
 * subsequent calls to this function will not fail, and will still return NULL.
 *
 * Synchronization: The hash lock must be held in exclusive mode.
 *
 * @param Hash Table - Pointer to the hash table to be enumerated.
 * @param Enumerator - Pointer to RTL_DYNAMIC_HASH_TABLE_ENUMERATOR structure that stores
 * enumeration state.
 *
 * @return Pointer to RTL_DYNAMIC_HASH_TABLE_ENTRY if one can be enumerated, and NULL other
 * wise.
 */
_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlEnumerateEntryHashTable(
    _In_ RTL_DYNAMIC_HASH_TABLE *HashTable,
    _Inout_ RTL_DYNAMIC_HASH_TABLE_ENUMERATOR *Enumerator)
{
    DPRINT1("RtlEnumerateEntryHashTable() called\n");
    NT_ASSERT(Enumerator != NULL);
    NT_ASSERT(Enumerator->ChainHead != NULL);
    NT_ASSERT(RTL_HASH_RESERVED_SIGNATURE == Enumerator->HashEntry.Signature);

    // We are trying to find the next valid entry. We need
    // to skip over other enumerators AND empty buckets.
    for (uint32_t i = Enumerator->BucketIndex; i < HashTable->TableSize; i++)
    {
        RTL_LIST_ENTRY *CurEntry, *ChainHead;
        if (i == Enumerator->BucketIndex)
        {
            // If this is the first bucket, start searching from enumerator.
            CurEntry = &(Enumerator->HashEntry.Linkage);
            ChainHead = Enumerator->ChainHead;
        }
        else
        {
            // Otherwise start searching from the head of the chain.
            ChainHead = RtlGetChainHead(HashTable, i);
            CurEntry = ChainHead;
        }

        while (CurEntry->Flink != ChainHead)
        {
            RTL_LIST_ENTRY *NextEntry = CurEntry->Flink;
            RTL_DYNAMIC_HASH_TABLE_ENTRY *NextHashEntry = RtlFlinkToHashEntry(&NextEntry->Flink);
            if (RTL_HASH_RESERVED_SIGNATURE != NextHashEntry->Signature)
            {
                RemoveEntryList(&(Enumerator->HashEntry.Linkage));

                NT_ASSERT(Enumerator->ChainHead != NULL);

                if (Enumerator->ChainHead != ChainHead)
                {
                    if (IsListEmpty(Enumerator->ChainHead))
                    {
                        HashTable->NonEmptyBuckets--;
                    }

                    if (IsListEmpty(ChainHead))
                    {
                        HashTable->NonEmptyBuckets++;
                    }
                }

                Enumerator->BucketIndex = i;
                Enumerator->ChainHead = ChainHead;

                InitializeListHead(NextEntry, &(Enumerator->HashEntry.Linkage));
                return NextHashEntry;
            }

            CurEntry = NextEntry;
        }
    }

    return NULL;
}

/**
 * @brief This routine reverses the effect of InitEnumeration. It decrements the
 * NumEnumerators counter in HashTable and cleans up Enumerator state.
 *
 * Synchronization: The hash table lock must be held in exclusive mode.
 *
 * @param HashTable - Pointer to hash table on which enumerator was operating.
 * @param Enumerator - Pointer to enumerator representing the enumeration that needs
 * to be ended.
 */
NTSYSAPI
VOID
NTAPI
RtlEndEnumerationHashTable(
    _In_ RTL_DYNAMIC_HASH_TABLE *HashTable,
    _Inout_ RTL_DYNAMIC_HASH_TABLE_ENUMERATOR *Enumerator)
{
    DPRINT1("RtlEndEnumerationHashTable() called\n");
    NT_ASSERT(Enumerator != NULL);
    NT_ASSERT(HashTable->NumEnumerators > 0);
    HashTable->NumEnumerators--;

    if (!IsListEmpty(&(Enumerator->HashEntry.Linkage)))
    {
        NT_ASSERT(Enumerator->ChainHead != NULL);

        RemoveEntryList(&(Enumerator->HashEntry.Linkage));

        if (IsListEmpty(Enumerator->ChainHead))
        {
            NT_ASSERT(HashTable->NonEmptyBuckets > 0);
            HashTable->NonEmptyBuckets--;
        }
    }

    Enumerator->ChainHead = FALSE;
}

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlInitWeakEnumerationHashTable(
    _In_ RTL_DYNAMIC_HASH_TABLE *HashTable,
    _Out_ RTL_DYNAMIC_HASH_TABLE_ENUMERATOR *Enumerator)
{
    DPRINT1("RtlInitWeakEnumerationHashTable() called\n");
    return RtlInitEnumerationHashTable(HashTable, Enumerator);
}

_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlWeaklyEnumerateEntryHashTable(
    _In_ RTL_DYNAMIC_HASH_TABLE *HashTable,
    _Inout_ RTL_DYNAMIC_HASH_TABLE_ENUMERATOR *Enumerator)
{
    DPRINT1("RtlWeaklyEnumerateEntryHashTable() called\n");
    return RtlEnumerateEntryHashTable(HashTable, Enumerator);
}
NTSYSAPI
VOID
NTAPI
RtlEndWeakEnumerationHashTable(
    _In_ RTL_DYNAMIC_HASH_TABLE *HashTable,
    _Inout_ RTL_DYNAMIC_HASH_TABLE_ENUMERATOR *Enumerator)
{
    DPRINT1("RtlEndWeakEnumerationHashTable() called\n");
    return RtlEndEnumerationHashTable(HashTable, Enumerator);
}

NTSYSAPI
BOOLEAN
NTAPI
RtlExpandHashTable(_In_ PRTL_DYNAMIC_HASH_TABLE HashTable)
{
    DPRINT1("RtlExpandHashTable() called\n");
    // Can't expand if we've reached the maximum.
    if (HashTable->TableSize == MAX_HASH_TABLE_SIZE)
    {
        return FALSE;
    }

    if (HashTable->NumEnumerators > 0)
    {
        return FALSE;
    }

    NT_ASSERT(HashTable->TableSize < MAX_HASH_TABLE_SIZE);

    // First see if increasing the table size will mean new allocations. After
    // the hash table is increased by one, the highest bucket index will be the
    // current table size, which is what we use in the calculations below
    uint32_t FirstLevelIndex = 0, SecondLevelIndex;
    RtlComputeDirIndices(HashTable->TableSize, &FirstLevelIndex, &SecondLevelIndex);

    // Switch to the multi-dir mode in case of the only second-level directory
    // is about to be expanded.
    LIST_ENTRY *SecondLevelDir;
    LIST_ENTRY **FirstLevelDir;
    if (HT_SECOND_LEVEL_DIR_MIN_SIZE == HashTable->TableSize)
    {
        SecondLevelDir = HashTable->SecondLevelDir;
        FirstLevelDir =
            ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(LIST_ENTRY *) * HT_FIRST_LEVEL_DIR_SIZE, TAG_HASHTABLE_MEMBER);
        if (FirstLevelDir == NULL)
        {
            return FALSE;
        }

        RtlZeroMemory(FirstLevelDir, sizeof(LIST_ENTRY *) * HT_FIRST_LEVEL_DIR_SIZE);

        FirstLevelDir[0] = SecondLevelDir;

        HashTable->FirstLevelDir = FirstLevelDir;
    }

    NT_ASSERT(HashTable->FirstLevelDir != NULL);
    FirstLevelDir = HashTable->FirstLevelDir;
    SecondLevelDir = FirstLevelDir[FirstLevelIndex];

    if (SecondLevelDir == NULL)
    {
        // Allocate second level directory.
        SecondLevelDir = ExAllocatePoolWithTag(
            NonPagedPoolNx, RtlComputeSecondLevelDirSize(FirstLevelIndex) * sizeof(LIST_ENTRY), TAG_HASHTABLE_MEMBER);
        if (NULL == SecondLevelDir)
        {
            // If allocation failure happened on attempt to restructure the
            // table, switch it back to direct mode.
            if (HT_SECOND_LEVEL_DIR_MIN_SIZE == HashTable->TableSize)
            {
                NT_ASSERT(FirstLevelIndex == 1);

                HashTable->SecondLevelDir = FirstLevelDir[0];
                ExFreePoolWithTag(FirstLevelDir, TAG_HASHTABLE_MEMBER);
            }

            return FALSE;
        }

        FirstLevelDir[FirstLevelIndex] = SecondLevelDir;
    }

    HashTable->TableSize++;

    // The allocations are out of the way. Now actually increase
    // the Table size and split the pivot bucket.
    LIST_ENTRY *ChainToBeSplit = RtlGetChainHead(HashTable, HashTable->Pivot);
    HashTable->Pivot++;

    LIST_ENTRY *NewChain = &(SecondLevelDir[SecondLevelIndex]);
    InitializeListHead(NewChain);

    if (!IsListEmpty(ChainToBeSplit))
    {
        LIST_ENTRY *CurEntry = ChainToBeSplit;
        while (CurEntry->Flink != ChainToBeSplit)
        {
            LIST_ENTRY *NextEntry = CurEntry->Flink;
            RTL_DYNAMIC_HASH_TABLE_ENTRY *NextHashEntry = RtlFlinkToHashEntry(&NextEntry->Flink);

            uint32_t BucketIndex = ((uint32_t)NextHashEntry->Signature) & ((HashTable->DivisorMask << 1) | 1);

            NT_ASSERT((BucketIndex == (HashTable->Pivot - 1)) || (BucketIndex == (HashTable->TableSize - 1)));

            if (BucketIndex == (HashTable->TableSize - 1))
            {
                RemoveEntryList(NextEntry);
                InsertTailList(NewChain, NextEntry);
                continue;
            }

            // If the NextEntry falls in the same bucket, move on.
            CurEntry = NextEntry;
        }

        if (!IsListEmpty(NewChain))
        {
            HashTable->NonEmptyBuckets++;
        }

        if (IsListEmpty(ChainToBeSplit))
        {
            NT_ASSERT(HashTable->NonEmptyBuckets > 0);
            HashTable->NonEmptyBuckets--;
        }
    }

    if (HashTable->Pivot == (HashTable->DivisorMask + 1))
    {
        HashTable->DivisorMask = (HashTable->DivisorMask << 1) | 1;
        HashTable->Pivot = 0;

        // Assert that at this point, TableSize is a power of 2.
        NT_ASSERT(0 == (HashTable->TableSize & (HashTable->TableSize - 1)));
    }

    return TRUE;
}

NTSYSAPI
BOOLEAN
NTAPI
RtlContractHashTable(_In_ RTL_DYNAMIC_HASH_TABLE *HashTable)
{
    DPRINT1("RtlContractHashTable() called\n");

    // Can't take table size lower than BASE_DYNAMIC_HASH_TABLE_SIZE.
    NT_ASSERT(HashTable->TableSize >= BASE_HASH_TABLE_SIZE);

    if (HashTable->TableSize == BASE_HASH_TABLE_SIZE)
    {
        return FALSE;
    }

    if (HashTable->NumEnumerators > 0)
    {
        return FALSE;
    }

    // Bring the table size down by 1 bucket, and change all state variables
    // accordingly.
    if (HashTable->Pivot == 0)
    {
        HashTable->DivisorMask = HashTable->DivisorMask >> 1;
        HashTable->Pivot = HashTable->DivisorMask;
    }
    else
    {
        HashTable->Pivot--;
    }

    // Need to combine two buckets. Since table-size is down by 1 and we need
    // the bucket that was the last bucket before table size was lowered, the
    // index of the last bucket is exactly equal to the current table size.
    LIST_ENTRY *ChainToBeMoved = RtlGetChainHead(HashTable, HashTable->TableSize - 1);
    LIST_ENTRY *CombinedChain = RtlGetChainHead(HashTable, HashTable->Pivot);

    HashTable->TableSize--;

    NT_ASSERT(ChainToBeMoved != NULL);
    NT_ASSERT(CombinedChain != NULL);

    if (!IsListEmpty(ChainToBeMoved) && !IsListEmpty(CombinedChain))
    {
        // Both lists are non-empty.

        NT_ASSERT(HashTable->NonEmptyBuckets > 0);
        HashTable->NonEmptyBuckets--;
    }

    LIST_ENTRY *CurEntry = CombinedChain;
    while (!IsListEmpty(ChainToBeMoved))
    {
        LIST_ENTRY *EntryToBeMoved = RemoveHeadList(ChainToBeMoved);
        RTL_DYNAMIC_HASH_TABLE_ENTRY *HashEntryToBeMoved = RtlFlinkToHashEntry(&EntryToBeMoved->Flink);

        while (CurEntry->Flink != CombinedChain)
        {
            LIST_ENTRY *NextEntry = CurEntry->Flink;
            RTL_DYNAMIC_HASH_TABLE_ENTRY *NextHashEntry = RtlFlinkToHashEntry(&NextEntry->Flink);

            if (NextHashEntry->Signature >= HashEntryToBeMoved->Signature)
            {
                break;
            }

            CurEntry = NextEntry;
        }

        InitializeListHead(CurEntry, &(HashEntryToBeMoved->Linkage));
    }

    // Finally free any extra memory if possible.
    uint32_t FirstLevelIndex = 0, SecondLevelIndex;
    RtlComputeDirIndices(HashTable->TableSize, &FirstLevelIndex, &SecondLevelIndex);

    if (SecondLevelIndex == 0)
    {
        LIST_ENTRY **FirstLevelDir = HashTable->FirstLevelDir;
        LIST_ENTRY *SecondLevelDir = FirstLevelDir[FirstLevelIndex];

        ExFreePoolWithTag(SecondLevelDir, TAG_HASHTABLE_MEMBER);

        FirstLevelDir[FirstLevelIndex] = NULL;

        // Switch to a single-dir mode if fits within a single second-level.
        if (HT_SECOND_LEVEL_DIR_MIN_SIZE == HashTable->TableSize)
        {
            HashTable->SecondLevelDir = FirstLevelDir[0];
            ExFreePoolWithTag(FirstLevelDir, TAG_HASHTABLE_MEMBER);
        }
    }

    return TRUE;
}

#endif
/* EOF */
