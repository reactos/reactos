//
// Region Allocation.
//
// Regions can be used to replace zones where it is not essential that the
// specified spinlock be acquired.
//

typedef struct _REGION_SEGMENT_HEADER {
    struct _REGION_SEGMENT_HEADER *NextSegment;
    PVOID Reserved;
} REGION_SEGMENT_HEADER, *PREGION_SEGMENT_HEADER;

typedef struct _REGION_HEADER {
    SLIST_HEADER ListHead;
    PREGION_SEGMENT_HEADER FirstSegment;
    ULONG BlockSize;
    ULONG TotalSize;
} REGION_HEADER, *PREGION_HEADER;

NTKERNELAPI
VOID
ExInitializeRegion(
    IN PREGION_HEADER Region,
    IN ULONG BlockSize,
    IN PVOID Segment,
    IN ULONG SegmentSize
    );

NTKERNELAPI
VOID
ExInterlockedExtendRegion(
    IN PREGION_HEADER Region,
    IN PVOID Segment,
    IN ULONG SegmentSize,
    IN PKSPIN_LOCK Lock
    );

//++
//
// PVOID
// ExInterlockedAllocateFromRegion(
//     IN PREGION_HEADER Region,
//     IN PKSPIN_LOCK Lock
//     )
//
// Routine Description:
//
//     This routine removes a free entry from the specified region and returns
//     the address of the entry.
//
// Arguments:
//
//     Region - Supplies a pointer to the region header.
//
//     Lock - Supplies a pointer to a spinlock.
//
// Return Value:
//
//     The address of the removed entry is returned as the function value.
//
//--

#if defined(_ALPHA_) || defined(_MIPS_) || defined(_X86_)

#define ExInterlockedAllocateFromRegion(Region, Lock) \
    (PVOID)ExInterlockedPopEntrySList(&(Region)->ListHead, Lock)

#else

#define ExInterlockedAllocateFromRegion(Region, Lock) \
    (PVOID)ExInterlockedPopEntryList((PSINGLE_LIST_ENTRY)&(Region)->ListHead.Next, Lock)

#endif

//++
//
// PVOID
// ExInterlockedFreeToRegion(
//     IN PREGION_HEADER Region,
//     IN PVOID Block,
//     IN PKSPIN_LOCK Lock
//     )
//
// Routine Description:
//
//     This routine inserts an entry at the front of the free list for the
//     specified region.
//
// Arguments:
//
//     Region - Supplies a pointer to the region header.
//
//     Block - Supplies a pointer to the entry that is inserted at the front
//        of the region free list.
//
//     Lock - Supplies a pointer to a spinlock.
//
// Return Value:
//
//     The previous firt entry in the region free list is returned as the
//     function value. a value of NULL implies the region went from empty
//     to at least one free block.
//
//--

#if defined(_ALPHA_) || defined(_MIPS_) || defined(_X86_)

#define ExInterlockedFreeToRegion(Region, Block, Lock) \
    ExInterlockedPushEntrySList(&(Region)->ListHead, ((PSINGLE_LIST_ENTRY)(Block)), Lock)

#else

#define ExInterlockedFreeToRegion(Region, Block, Lock) \
    ExInterlockedPushEntryList((PSINGLE_LIST_ENTRY)&(Region)->ListHead.Next, ((PSINGLE_LIST_ENTRY)(Block)), Lock)

#endif

//++
//
// BOOLEAN
// ExIsFullRegion(
//     IN PREGION_HEADER Region
//     )
//
// Routine Description:
//
//     This routine determines if the specified region is full. A region is
//     considered full if the free list is empty.
//
// Arguments:
//
//     Region - Supplies a pointer the region header.
//
// Return Value:
//
//     TRUE if the region is full and FALSE otherwise.
//
//--

#define ExIsFullRegion(Region) \
    ((Region)->ListHead.Next == (PSINGLE_LIST_ENTRY)NULL)

//++
//
// BOOLEAN
// ExIsObjectInFirstRegionSegment(
//     IN PREGION_HEADER Region,
//     IN PVOID Object
//     )
//
// Routine Description:
//
//     This routine determines if the specified object is contained in the
//     first region segement.
//
// Arguments:
//
//     Region - Supplies a pointer to the region header.
//
//     Object - Supplies a pointer to an object.
//
// Return Value:
//
//     TRUE if the Object came from the first segment of region.
//
//--

#define ExIsObjectInFirstRegionSegment(Region, Object) ((BOOLEAN)   \
    (((PUCHAR)(Object) >= ((PUCHAR)((Region)->FirstSegment) + sizeof(REGION_SEGMENT_HEADER))) &&        \
     ((PUCHAR)(Object) < ((PUCHAR)((Region)->FirstSegment) + (Region)->TotalSize + sizeof(REGION_SEGMENT_HEADER)))))

