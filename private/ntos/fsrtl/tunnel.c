/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    Tunnel.c

Abstract:

    The tunnel package provides a set of routines that allow compatibility
    with applications that rely on filesystems being able to "hold onto"
    file meta-info for a short period of time after deletion/renaming and
    reinstantiating a new directory entry with that meta-info if a
    create/rename occurs to cause a file of that name to appear again in a
    short period of time.

    Note that this violates POSIX rules. This package should not be used
    on POSIX fileobjects, i.e. fileobjects that have case-sensitive names.

    Entries are keyed by directory and one of the short/long names. An opaque
    rock of information is also associated (create time, last write time, etc.).
    This is expected to vary on a per-filesystem basis.

    A TUNNEL variable should be initialized for every volume in the system
    at mount time. Thereafter, each delete/rename-out should add to the tunnel
    and each create/rename-in should read from the tunnel. Each directory
    deletion should also notify the package so that all associated entries can
    be flushed. The package is responsible for cleaning out aged entries.

    Tunneled information is in the paged pool.

    Concurrent access to the TUNNEL variable is controlled by this package.
    Callers are responsible for synchronizing access to the FsRtlDeleteTunnelCache
    call.

    The functions provided in this package are as follows:

      o  FsRtlInitializeTunnel - Initializes the TUNNEL package (called once per boot)

      o  FsRtlInitializeTunnelCache - Initializes a TUNNEL structure (called once on mount)

      o  FsRtlAddToTunnelCache - Adds a new key/value pair to the tunnel

      o  FsRtlFindInTunnelCache - Finds and returns a key/value from the tunnel

      o  FsRtlDeleteKeyFromTunnelCache - Deletes all entries with a given
           directory key from the tunnel

      o  FsRtlDeleteTunnelCache - Deletes a TUNNEL structure

Author:

    Dan Lovinger     [DanLo]    8-Aug-1995

Revision History:

--*/

#include "FsRtlP.h"

#ifndef INLINE
#define INLINE __inline
#endif

//
//  Registry keys/values for controlling tunneling
//

#define TUNNEL_KEY_NAME           L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\FileSystem"
#define TUNNEL_AGE_VALUE_NAME     L"MaximumTunnelEntryAgeInSeconds"
#define TUNNEL_SIZE_VALUE_NAME    L"MaximumTunnelEntries"
#define KEY_WORK_AREA ((sizeof(KEY_VALUE_FULL_INFORMATION) + sizeof(ULONG)) + 64)

//
//  Tunnel expiration paramters (cached once at startup)
//

ULONG   TunnelMaxEntries;
ULONG   TunnelMaxAge;

//
//  We use a lookaside list to manage the common size tunnel entry. The common size
//  is contrived to be 128 bytes by adjusting the size we defer for the long name
//  to 16 characters, which is pretty reasonable. If we ever expect to get more than
//  a ULONGLONG data element or common names are observed to become larger, adjusting
//  this may be required.
//

PAGED_LOOKASIDE_LIST    TunnelLookasideList;
#define MAX_LOOKASIDE_DEPTH     256

#define LOOKASIDE_NODE_SIZE     ( sizeof(TUNNEL_NODE) +     \
                                  sizeof(WCHAR)*(8+1+3) +   \
                                  sizeof(WCHAR)*(16) +      \
                                  sizeof(ULONGLONG) )

//
//  Flag bits in the TUNNEL_NODE
//

#define TUNNEL_FLAG_NON_LOOKASIDE    0x1
#define TUNNEL_FLAG_KEY_SHORT        0x2

//
//  A node of tunneled information in the cache
//
//  A TUNNEL is allocated in each VCB and initialized at mount time.
//
//  TUNNEL_NODES are then arranged off of the TUNNEL in a splay tree keyed
//  by DirKey ## Name, where Name is whichever of the names was removed from
//  the directory (short or long). Each node is also timestamped and inserted
//  into a timer queue for age expiration.
//

typedef struct {

    //
    //  Splay links in the Cache tree
    //

    RTL_SPLAY_LINKS      CacheLinks;

    //
    //  List links in the timer queue
    //

    LIST_ENTRY           ListLinks;

    //
    //  Time this entry was created (for constant time insert)
    //

    LARGE_INTEGER        CreateTime;

    //
    //  Directory these names are associated with
    //

    ULONGLONG            DirKey;

    //
    //  Flags for the entry
    //

    ULONG                Flags;

    //
    //  Long/Short names of the file
    //

    UNICODE_STRING       LongName;
    UNICODE_STRING       ShortName;

    //
    //  Opaque tunneled data
    //

    PVOID                TunnelData;
    ULONG                TunnelDataLength;

} TUNNEL_NODE, *PTUNNEL_NODE;

//
//  Internal utility functions
//

NTSTATUS
FsRtlGetTunnelParameterValue (
    IN PUNICODE_STRING ValueName,
    IN OUT PULONG Value);

VOID
FsRtlPruneTunnelCache (
    IN PTUNNEL Cache,
    IN OUT PLIST_ENTRY FreePoolList);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FsRtlInitializeTunnels)
#pragma alloc_text(PAGE, FsRtlInitializeTunnelCache)
#pragma alloc_text(PAGE, FsRtlAddToTunnelCache)
#pragma alloc_text(PAGE, FsRtlFindInTunnelCache)
#pragma alloc_text(PAGE, FsRtlDeleteKeyFromTunnelCache)
#pragma alloc_text(PAGE, FsRtlDeleteTunnelCache)
#pragma alloc_text(PAGE, FsRtlPruneTunnelCache)
#pragma alloc_text(PAGE, FsRtlGetTunnelParameterValue)
#endif

//
//  Testing and usermode rig support. Define TUNNELTEST to get verbose debugger
//  output on various operations. Define USERTEST to transform the code into
//  a form which can be compiled in usermode for more efficient debugging.
//

#if defined(TUNNELTEST) || defined(KEYVIEW)
VOID DumpUnicodeString(UNICODE_STRING *s);
VOID DumpNode( TUNNEL_NODE *Node, ULONG Indent );
VOID DumpTunnel( TUNNEL *Tunnel );
#define DblHex64(a) (ULONG)((a >> 32) & 0xffffffff),(ULONG)(a & 0xffffffff)
#endif // TUNNELTEST

#ifdef USERTEST
#include <stdio.h>
#undef KeQuerySystemTime
#define KeQuerySystemTime NtQuerySystemTime
#undef ExInitializeFastMutex
#define ExInitializeFastMutex(arg)
#define ExAcquireFastMutex(arg)
#define ExReleaseFastMutex(arg)
#define DbgPrint printf
#undef PAGED_CODE
#define PAGED_CODE()
#endif


INLINE
LONG
FsRtlCompareNodeAndKey (
    TUNNEL_NODE *Node,
    ULONGLONG DirectoryKey,
    PUNICODE_STRING Name
    )
/*++

Routine Description:

    Compare a tunnel node with a key/name pair

Arguments:

    Node              - a tunnel node

    DirectoryKey      - a key value

    Name              - a filename

Return Value:

    Signed comparison result

--*/

{
    return  (Node->DirKey > DirectoryKey ?  1 :
            (Node->DirKey < DirectoryKey ? -1 :
            RtlCompareUnicodeString((FlagOn(Node->Flags, TUNNEL_FLAG_KEY_SHORT) ?
                                        &Node->ShortName : &Node->LongName),
                                    Name,
                                    TRUE)));
}


INLINE
VOID
FsRtlFreeTunnelNode (
    PTUNNEL_NODE Node,
    PLIST_ENTRY FreePoolList OPTIONAL
    )
/*++

Routine Description:

    Free a node

Arguments:

    Node            - a tunnel node to free

    FreePoolList    - optional list to hold freeable pool memory

Return Value:

    None

-*/
{
    if (FreePoolList) {

        InsertHeadList(FreePoolList, &Node->ListLinks);

    } else {

        if (FlagOn(Node->Flags, TUNNEL_FLAG_NON_LOOKASIDE)) {
    
            ExFreePool(Node);
    
        } else {
    
            ExFreeToPagedLookasideList(&TunnelLookasideList, Node);
        }
    }
}


INLINE
VOID
FsRtlEmptyFreePoolList (
    PLIST_ENTRY FreePoolList
    )
/*++

Routine Description:

    Free all pool memory that has been delayed onto a free list.

Arguments:

    FreePoolList    - a list of freeable pool memory

Return Value:

    None

-*/
{
    PTUNNEL_NODE FreeNode;

    while (!IsListEmpty(FreePoolList)) {

        FreeNode = CONTAINING_RECORD(FreePoolList->Flink, TUNNEL_NODE, ListLinks);
        RemoveEntryList(FreePoolList->Flink);

        FsRtlFreeTunnelNode(FreeNode, NULL);
    }
}


INLINE
VOID
FsRtlRemoveNodeFromTunnel (
    IN PTUNNEL Cache,
    IN PTUNNEL_NODE Node,
    IN PLIST_ENTRY FreePoolList,
    IN PBOOLEAN Splay OPTIONAL
    )
/*++

Routine Description:

    Performs the common work of deleting a node from a tunnel cache. Pool memory
    is not deleted immediately but is saved aside on a list for deletion later
    by the calling routine.

Arguments:

    Cache - the tunnel cache the node is in

    Node - the node being removed

    FreePoolList - an initialized list to take the node if it was allocated from
        pool

    Splay - an optional flag to indicate whether the tree should be splayed on
        the delete. Set to FALSE if splaying was performed.

Return Value:

    None.

--*/
{
    if (Splay && *Splay) {

        Cache->Cache = RtlDelete(&Node->CacheLinks);

        *Splay = FALSE;

    } else {

        RtlDeleteNoSplay(&Node->CacheLinks, &Cache->Cache);
    }

    RemoveEntryList(&Node->ListLinks);

    Cache->NumEntries--;

    FsRtlFreeTunnelNode(Node, FreePoolList);
}


VOID
FsRtlInitializeTunnels (
    VOID
    )
/*++

Routine Description:

    Initializes the global part of the tunneling package.

Arguments:

    None

Return Value:

    None

--*/
{
    UNICODE_STRING  ValueName;
    USHORT          LookasideDepth;

    PAGED_CODE();

    if (MmIsThisAnNtAsSystem()) {

        TunnelMaxEntries = 1024;

    } else {

        TunnelMaxEntries = 256;
    }

    TunnelMaxAge = 15;

    //
    //  Query our configurable parameters
    //
    //  Don't worry about failure in retrieving from the registry. We've gotten
    //  this far so fall back on defaults even if there was a problem with resources.
    //

    ValueName.Buffer = TUNNEL_SIZE_VALUE_NAME;
    ValueName.Length = sizeof(TUNNEL_SIZE_VALUE_NAME) - sizeof(WCHAR);
    ValueName.MaximumLength = sizeof(TUNNEL_SIZE_VALUE_NAME);
    (VOID) FsRtlGetTunnelParameterValue(&ValueName, &TunnelMaxEntries);

    ValueName.Buffer = TUNNEL_AGE_VALUE_NAME;
    ValueName.Length = sizeof(TUNNEL_AGE_VALUE_NAME) - sizeof(WCHAR);
    ValueName.MaximumLength = sizeof(TUNNEL_AGE_VALUE_NAME);
    (VOID) FsRtlGetTunnelParameterValue(&ValueName, &TunnelMaxAge);

    if (TunnelMaxAge == 0) {

        //
        //  If the registry has been set so the timeout is zero, we should force
        //  the number of entries to zero also. This preserves expectations and lets
        //  us key off of max entries alone in performing the hard disabling of the
        //  caching code.
        //

        TunnelMaxEntries = 0;
    }

    //
    //  Convert from seconds to 10ths of msecs, the internal resolution
    //

    TunnelMaxAge *= 10000000;

    //
    //  Build the lookaside list for common node allocation
    //

    if (TunnelMaxEntries > MAXUSHORT) {

        //
        //  User is hinting a big need to us
        //

        LookasideDepth = MAX_LOOKASIDE_DEPTH;

    } else {

        LookasideDepth = ((USHORT)TunnelMaxEntries)/16;
    }

    if (LookasideDepth == 0 && TunnelMaxEntries) {

        //
        //  Miniscule number of entries allowed. Lookaside 'em all.
        //

        LookasideDepth = (USHORT)TunnelMaxEntries + 1;
    }

    if (LookasideDepth > MAX_LOOKASIDE_DEPTH) {

        //
        //  Finally, restrict the depth to something reasonable.
        //

        LookasideDepth = MAX_LOOKASIDE_DEPTH;
    }

    ExInitializePagedLookasideList( &TunnelLookasideList,
                                    NULL,
                                    NULL,
                                    0,
                                    LOOKASIDE_NODE_SIZE,
                                    'LnuT',
                                    LookasideDepth );

    return;
}


//
//  *** SPEC
//
//    FsRtlInitializeTunnelCache - Initialize a tunneling cache for a volume
//
//    FsRtlInitializeTunnelCache will allocate a default cache (resizing policy is common
//    to all file systems) and initialize it to be empty.  File systems will store a pointer to
//    this cache in their per-volume structures.
//
//    Information is retained in the tunnel cache for a fixed period of time.  MarkZ would
//    assume that a value of 10 seconds would satisfy the vast majority of situations.  This
//    could be controlled by the registry or could be a compilation constant.
//
//  Change: W95 times out at 15 seconds. Would be a registry value initialized at tunnel
//  creation time, with a proposed default of 15 seconds.
//

VOID
FsRtlInitializeTunnelCache (
    IN PTUNNEL Cache
    )
/*++

Routine Description:

    Initialize a new tunnel cache.

Arguments:

    None

Return Value:

    None

--*/
{
    PAGED_CODE();

    ExInitializeFastMutex(&Cache->Mutex);

    Cache->Cache = NULL;
    InitializeListHead(&Cache->TimerQueue);
    Cache->NumEntries = 0;

    return;
}


//
//  *** SPEC
//
//    FsRtlAddToTunnelCache - add information to a tunnel cache
//
//    FsRtlAddToTunnelCache is called by file systems when a name disappears from a
//    directory.  This typically occurs in both the delete and the rename paths.  When
//    a name is deleted, all information needed to be cached is extracted from the file
//    and passed in a single buffer.  This information is stored keyed by the directory key
//    (a ULONG that is unique to the directory) and the short-name of the file.
//
//    The caller is required to synchronize this call against FsRtlDeleteTunnelCache.
//
//    Arguments:
//        Cache        pointer to cache initialized by FsRtlInitializeTunnelCache
//        DirectoryKey    ULONG unique ID of the directory containing the deleted file
//        ShortName    UNICODE_STRING* short (8.3) name of the file
//        LongName    UNICODE_STRING* full name of the file
//        DataLength    ULONG length of data to be cached with these names
//        Data        VOID* data that will be cached.
//
//    It is acceptable for the Cache to ignore this request based upon memory constraints.
//
//  Change: W95 maintains 10 items in the tunnel cache. Since we are a potential server
//  this should be much higher. The max count would be initialized from the registry with
//  a proposed default of 1024. Adds which run into the limit would cause least recently
//  inserted recycling (i.e., off of the top of the timer queue).
//
//  Change: Key should be by the name removed, not neccesarily the short name. If a long name
//  is removed, it would be incorrect to miss the tunnel. Use KeyByShortName boolean to specify
//  which.
//
//  Change: Specify that Data, ShortName, and LongName are copied for storage.
//

VOID
FsRtlAddToTunnelCache (
    IN PTUNNEL Cache,
    IN ULONGLONG DirKey,
    IN PUNICODE_STRING ShortName,
    IN PUNICODE_STRING LongName,
    IN BOOLEAN KeyByShortName,
    IN ULONG DataLength,
    IN PVOID Data
    )
/*++

Routine Description:

    Adds an entry to the tunnel cache keyed by

        DirectoryKey ## (KeyByShortName ? ShortName : LongName)

    ShortName, LongName, and Data are copied and stored in the tunnel. As a side
    effect, if there are too many entries in the tunnel cache, this routine will
    initiate expiration in the tunnel cache.

Arguments:

    Cache - a tunnel cache initialized by FsRtlInitializeTunnelCache()

    DirKey - the key value of the directory the name appeared in

    ShortName - (optional if !KeyByShortName) the 8.3 name of the file

    LongName - (optional if KeyByShortName) the long name of the file

    KeyByShortName - specifies which name is keyed in the tunnel cache

    DataLength - specifies the length of the opaque data segment (file
    system specific) which contains the tunnelling information for this
    file

    Data - pointer to the opaque tunneling data segment

Return Value:

    None

--*/
{
    LONG Compare;
    ULONG NodeSize;
    PUNICODE_STRING NameKey;
    PRTL_SPLAY_LINKS *Links;
    LIST_ENTRY FreePoolList;

    PTUNNEL_NODE Node = NULL;
    PTUNNEL_NODE NewNode = NULL;
    BOOLEAN FreeOldNode = FALSE;
    BOOLEAN AllocatedFromPool = FALSE;

    PAGED_CODE();

    //
    //  If MaxEntries is 0 then tunneling is disabled.
    //

    if (TunnelMaxEntries == 0) return;

    InitializeListHead(&FreePoolList);

    //
    //  Grab a new node for this data
    //

    NodeSize = sizeof(TUNNEL_NODE) + ShortName->Length + LongName->Length + DataLength;

    if (LOOKASIDE_NODE_SIZE >= NodeSize) {

        NewNode = ExAllocateFromPagedLookasideList(&TunnelLookasideList);
    }

    if (NewNode == NULL) {

        //
        //  Data doesn't fit in lookaside nodes
        //

        NewNode = ExAllocatePoolWithTag(PagedPool, NodeSize, 'PnuT');

        if (NewNode == NULL) {

            //
            //  Give up tunneling this entry
            //

            return;
        }

        AllocatedFromPool = TRUE;
    }

    //
    //  Traverse the cache to find our insertion point
    //

    NameKey = (KeyByShortName ? ShortName : LongName);

    ExAcquireFastMutex(&Cache->Mutex);

    Links = &Cache->Cache;

    while (*Links) {

        Node = CONTAINING_RECORD(*Links, TUNNEL_NODE, CacheLinks);

        Compare = FsRtlCompareNodeAndKey(Node, DirKey, NameKey);

        if (Compare > 0) {

            Links = &RtlLeftChild(&Node->CacheLinks);

        } else {

            if (Compare < 0) {

                Links = &RtlRightChild(&Node->CacheLinks);

            } else {

                break;
            }
        }
    }

    //
    //  Thread new data into the splay tree
    //

    RtlInitializeSplayLinks(&NewNode->CacheLinks);

    if (Node) {

        //
        //  Not inserting first node in tree
        //

        if (*Links) {

            //
            //  Entry exists in the cache, so replace by swapping all splay links
            //

            RtlRightChild(&NewNode->CacheLinks) = RtlRightChild(*Links);
            RtlLeftChild(&NewNode->CacheLinks) = RtlLeftChild(*Links);

            if (RtlRightChild(*Links)) RtlParent(RtlRightChild(*Links)) = &NewNode->CacheLinks;
            if (RtlLeftChild(*Links)) RtlParent(RtlLeftChild(*Links)) = &NewNode->CacheLinks;

            if (!RtlIsRoot(*Links)) {

                //
                //  Change over the parent links. Note that we've messed with *Links now
                //  since it is pointing at the parent member.
                //

                RtlParent(&NewNode->CacheLinks) = RtlParent(*Links);

                if (RtlIsLeftChild(*Links)) {

                    RtlLeftChild(RtlParent(*Links)) = &NewNode->CacheLinks;

                } else {

                    RtlRightChild(RtlParent(*Links)) = &NewNode->CacheLinks;
                }

            } else {

                //
                //  Set root of the cache
                //

                Cache->Cache = &NewNode->CacheLinks;
            }

            //
            //  Free old node
            //

            RemoveEntryList(&Node->ListLinks);

            FsRtlFreeTunnelNode(Node, &FreePoolList);

            Cache->NumEntries--;

        } else {

            //
            //  Simple insertion as a leaf
            //

            NewNode->CacheLinks.Parent = &Node->CacheLinks;
            *Links = &NewNode->CacheLinks;
        }

    } else {

        Cache->Cache = &NewNode->CacheLinks;
    }

    //
    //  Thread onto the timer list
    //

    KeQuerySystemTime(&NewNode->CreateTime);
    InsertTailList(&Cache->TimerQueue, &NewNode->ListLinks);

    Cache->NumEntries++;

    //
    //  Stash tunneling information
    //

    NewNode->DirKey = DirKey;

    if (KeyByShortName) {

        NewNode->Flags = TUNNEL_FLAG_KEY_SHORT;

    } else {

        NewNode->Flags = 0;
    }

    //
    //  Initialize the internal UNICODE_STRINGS to point at the buffer segments. For various
    //  reasons (UNICODE APIs are incomplete, we're avoiding calling any allocate routine more
    //  than once, UNICODE strings are not guaranteed to be null terminated) we have to do a lot
    //  of this by hand.
    //
    //  The data is layed out like this in the allocated block:
    //
    //  -----------------------------------------------------------------------------------
    //  | TUNNEL_NODE | Node->ShortName.Buffer | Node->LongName.Buffer | Node->TunnelData |
    //  -----------------------------------------------------------------------------------
    //

    NewNode->ShortName.Buffer = (PWCHAR)((PCHAR)NewNode + sizeof(TUNNEL_NODE));
    NewNode->LongName.Buffer = (PWCHAR)((PCHAR)NewNode + sizeof(TUNNEL_NODE) + ShortName->Length);

    NewNode->ShortName.Length = NewNode->ShortName.MaximumLength = ShortName->Length;
    NewNode->LongName.Length = NewNode->LongName.MaximumLength = LongName->Length;

    if (ShortName->Length) {

        RtlCopyMemory(NewNode->ShortName.Buffer, ShortName->Buffer, ShortName->Length);
    }

    if (LongName->Length) {

        RtlCopyMemory(NewNode->LongName.Buffer, LongName->Buffer, LongName->Length);
    }

    NewNode->TunnelData = (PVOID)((PCHAR)NewNode + sizeof(TUNNEL_NODE) + ShortName->Length + LongName->Length);

    NewNode->TunnelDataLength = DataLength;

    RtlCopyMemory(NewNode->TunnelData, Data, DataLength);

    if (AllocatedFromPool) {

        SetFlag(NewNode->Flags, TUNNEL_FLAG_NON_LOOKASIDE);
    }

#if defined(TUNNELTEST) || defined (KEYVIEW)
    DbgPrint("FsRtlAddToTunnelCache:\n");
    DumpNode(NewNode, 1);
#ifndef KEYVIEW
    DumpTunnel(Cache);
#endif
#endif // TUNNELTEST

    //
    //  Clean out the cache, release, and then drop any pool memory we need to
    //

    FsRtlPruneTunnelCache(Cache, &FreePoolList);

    ExReleaseFastMutex(&Cache->Mutex);

    FsRtlEmptyFreePoolList(&FreePoolList);

    return;
}


//
//  *** SPEC
//
//    FsRtlFindInTunnelCache - retrieve information from tunnel cache
//
//    FsRtlFindInTunnelCache consults the cache to see if an entry with the same
//    DirectoryKey and ShortName exist.  If so, it returns the data associated with the
//    cache entry.  The entry may or may not be freed from the cache.  Information that is
//    stale but not yet purged (older than the retention threshold but not yet cleaned out)
//    may be returned.
//
//    File systems call FsRtlFindInTunnel cache in the create path when a new file is
//    being created and in the rename path when a new name is appearing in a directory.
//
//    The caller is required to synchronize this call against FsRtlDeleteTunnelCache.
//
//    Arguments:
//        Cache        a tunnel cache initialized by FsRtlInitializeTunnelCache()
//        DirectoryKey    ULONG unique ID of the directory where a name is appearing
//        Name        UNICODE_STRING* name that is being created
//        DataLength     in length of buffer, out returned length of data found
//        Data        pointer to buffer
//
//    Returns:
//        TRUE iff a matching DirectoryKey/Name pair are found, FALSE otherwise
//
//  Change: Add out parameters ShortName and LongName to capture the file naming information.
//  Plus: this avoids the need for marshalling/unmarshalling steps for the current desired use of
//  this code since otherwise we'd have variable length unaligned structures to contain the
//  strings along with the other meta-info.
//  Minus: Possibly a bad precedent.
//
//  Change: spec reads "may or may not be freed from cache" on a hit. This complicates unwinding
//  from aborted operations. Data will not be freed on a hit, but will expire like normal entries.
//

BOOLEAN
FsRtlFindInTunnelCache (
    IN PTUNNEL Cache,
    IN ULONGLONG DirKey,
    IN PUNICODE_STRING Name,
    OUT PUNICODE_STRING ShortName,
    OUT PUNICODE_STRING LongName,
    IN OUT PULONG  DataLength,
    OUT PVOID Data
    )
/*++

Routine Description:

    Looks up the key

        DirKey ## Name

    in the tunnel cache and removes it. As a side effect, this routine will initiate
    expiration of the aged entries in the tunnel cache.

Arguments:

    Cache - a tunnel cache initialized by FsRtlInitializeTunnelCache()

    DirKey - the key value of the directory the name will appear in

    Name - the name of the entry

    ShortName - return string to hold the short name of the tunneled file. Must
        already be allocated and large enough for max 8.3 name

    LongName -  return string to hold the long name of the tunneled file. If
        already allocated, may be grown if not large enough. Caller is
        responsible for noticing this and freeing data regardless of return value.

    DataLength - provides the length of the buffer avaliable to hold the
        tunneling information, returns the size of the tunneled information
        read out

Return Value:

    Boolean true if found, false otherwise

--*/
{
    PRTL_SPLAY_LINKS Links;
    PTUNNEL_NODE Node;
    LONG Compare;
    LIST_ENTRY FreePoolList;

    BOOLEAN Status = FALSE;

    PAGED_CODE();

    //
    //  If MaxEntries is 0 then tunneling is disabled.
    //

    if (TunnelMaxEntries == 0) return FALSE;

    InitializeListHead(&FreePoolList);

#ifdef KEYVIEW
    DbgPrint("++\nSearching for %wZ , %08x%08x\n--\n", Name, DblHex64(DirKey));
#endif

    ExAcquireFastMutex(&Cache->Mutex);

    //
    //  Expire aged entries first so we don't grab old data
    //

    FsRtlPruneTunnelCache(Cache, &FreePoolList);

    Links = Cache->Cache;

    while (Links) {

        Node = CONTAINING_RECORD(Links, TUNNEL_NODE, CacheLinks);

        Compare = FsRtlCompareNodeAndKey(Node, DirKey, Name);

        if (Compare > 0) {

            Links = RtlLeftChild(&Node->CacheLinks);

        } else {

            if (Compare < 0) {

                Links = RtlRightChild(&Node->CacheLinks);

            } else {

                //
                //  Found tunneling information
                //

#if defined(TUNNELTEST) || defined(KEYVIEW)
                DbgPrint("FsRtlFindInTunnelCache:\n");
                DumpNode(Node, 1);
#ifndef KEYVIEW
                DumpTunnel(Cache);
#endif
#endif // TUNNELTEST

                break;
            }
        }
    }

    try {

        if (Links) {
    
            //
            //  Copy node data into caller's area
            //
    
            ASSERT(ShortName->MaximumLength >= (8+1+3)*sizeof(WCHAR));
            RtlCopyUnicodeString(ShortName, &Node->ShortName);
    
            if (LongName->MaximumLength >= Node->LongName.Length) {
    
                RtlCopyUnicodeString(LongName, &Node->LongName);
    
            } else {
    
                //
                //  Need to allocate more memory for the long name
                //
    
                LongName->Buffer = FsRtlAllocatePoolWithTag(PagedPool, Node->LongName.Length, '4nuT');
                LongName->Length = LongName->MaximumLength = Node->LongName.Length;
    
                RtlCopyMemory(LongName->Buffer, Node->LongName.Buffer, Node->LongName.Length);
            }
    
            ASSERT(*DataLength >= Node->TunnelDataLength);
            RtlCopyMemory(Data, Node->TunnelData, Node->TunnelDataLength);
            *DataLength = Node->TunnelDataLength;
    
            Status = TRUE;
        }

    } finally {

        ExReleaseFastMutex(&Cache->Mutex);
    
        FsRtlEmptyFreePoolList(&FreePoolList);
    }
    
    return Status;
}


//
//  *** SPEC
//
//    FsRtlDeleteKeyFromTunnelCache - delete all cached information associated with
//    a DirectoryKey
//
//    When file systems delete a directory, all cached information relating to that directory
//    must be purged.  File systems call FsRtlDeleteKeyFromTunnelCache in the rmdir path.
//
//    The caller is required to synchronize this call against FsRtlDeleteTunnelCache.
//
//    Arguments:
//        Cache        a tunnel cache initialized by FsRtlInitializeTunnelCache()
//        DirectoryKey    ULONGLONG unique ID of the directory that is being deleted
//

VOID
FsRtlDeleteKeyFromTunnelCache (
    IN PTUNNEL Cache,
    IN ULONGLONG DirKey
    )
/*++

Routine Description:

    Deletes all entries in the cache associated with a specific directory

Arguments:

    Cache - a tunnel cache initialized by FsRtlInitializeTunnelCache()

    DirKey - the key value of the directory (presumeably being removed)

Return Value:

    None

--*/
{
    PRTL_SPLAY_LINKS Links;
    PRTL_SPLAY_LINKS SuccessorLinks;
    PTUNNEL_NODE Node;
    LIST_ENTRY FreePoolList;

    PRTL_SPLAY_LINKS LastLinks = NULL;
    BOOLEAN Splay = TRUE;

    PAGED_CODE();

    //
    //  If MaxEntries is 0 then tunneling is disabled.
    //

    if (TunnelMaxEntries == 0) return;

    InitializeListHead(&FreePoolList);

#ifdef KEYVIEW
    DbgPrint("++\nDeleting key %08x%08x\n--\n", DblHex64(DirKey));
#endif

    ExAcquireFastMutex(&Cache->Mutex);

    Links = Cache->Cache;

    while (Links) {

        Node = CONTAINING_RECORD(Links, TUNNEL_NODE, CacheLinks);

        if (Node->DirKey > DirKey) {

            //
            //  All nodes to the right are bigger, go left
            //

            Links = RtlLeftChild(&Node->CacheLinks);

        } else {

            if (Node->DirKey < DirKey) {

                if (LastLinks) {

                    //
                    //  If we have previously seen a candidate node to delete
                    //  and we have now gone too far left - we know where to start.
                    //

                    break;
                }

                Links = RtlRightChild(&Node->CacheLinks);

            } else {

                //
                //  Node is a candidate to be deleted, but we might have more nodes
                //  to the left in the tree. Note this location and go on.
                //

                LastLinks = Links;
                Links = RtlLeftChild(&Node->CacheLinks);
            }
        }
    }

    for (Links = LastLinks;
         Links;
         Links = SuccessorLinks) {

        SuccessorLinks = RtlRealSuccessor(Links);
        Node = CONTAINING_RECORD(Links, TUNNEL_NODE, CacheLinks);

        if (Node->DirKey != DirKey) {

            //
            //  Reached nodes which have a different key, so we're done
            //

            break;
        }

        FsRtlRemoveNodeFromTunnel(Cache, Node, &FreePoolList, &Splay);
    }

#ifdef TUNNELTEST
    DbgPrint("FsRtlDeleteKeyFromTunnelCache:\n");
#ifndef KEYVIEW
    DumpTunnel(Cache);
#endif
#endif // TUNNELTEST

    ExReleaseFastMutex(&Cache->Mutex);

    //
    //  Free delayed pool
    //

    FsRtlEmptyFreePoolList(&FreePoolList);

    return;
}


//
//  *** SPEC
//
//    FsRtlDeleteTunnelCache - free a tunnel cache
//
//    FsRtlDeleteTunnelCache deletes all cached information.  The Cache is no longer
//    valid.
//
//    Arguments:
//        Cache        a tunnel cache initialized by FsRtlInitializeTunnelCache()
//

VOID
FsRtlDeleteTunnelCache (
    IN PTUNNEL Cache
    )
/*++

Routine Description:

    Deletes a tunnel cache

Arguments:

    Cache - the cache to delete, initialized by FsRtlInitializeTunnelCache()

Return Value:

    None

--*/
{
    PTUNNEL_NODE Node;
    PLIST_ENTRY Link, Next;

    PAGED_CODE();

    //
    //  If MaxEntries is 0 then tunneling is disabled.
    //

    if (TunnelMaxEntries == 0) return;

    //
    //  Zero out the cache and delete everything on the timer list
    //

    Cache->Cache = NULL;
    Cache->NumEntries = 0;

    for (Link = Cache->TimerQueue.Flink;
         Link != &Cache->TimerQueue;
         Link = Next) {

        Next = Link->Flink;

        Node = CONTAINING_RECORD(Link, TUNNEL_NODE, ListLinks);

        FsRtlFreeTunnelNode(Node, NULL);
    }

    InitializeListHead(&Cache->TimerQueue);

    return;
}


VOID
FsRtlPruneTunnelCache (
    IN PTUNNEL Cache,
    IN OUT PLIST_ENTRY FreePoolList
    )
/*++

Routine Description:

    Removes deadwood entries from the tunnel cache as defined by TunnelMaxAge and TunnelMaxEntries.
    Pool memory is returned on a list for deletion by the calling routine at a time of
    its choosing.

    For performance reasons we don't want to force freeing of memory inside a mutex.

Arguments:

    Cache - the tunnel cache to prune

    FreePoolList - a list to queue pool memory on to

Return Value:

    None
--*/
{
    PTUNNEL_NODE Node;
    LARGE_INTEGER ExpireTime;
    LARGE_INTEGER CurrentTime;
    BOOLEAN Splay = TRUE;

    PAGED_CODE();

    //
    //  Calculate the age of the oldest entry we want to keep
    //

    KeQuerySystemTime(&CurrentTime);
    ExpireTime.QuadPart = CurrentTime.QuadPart - TunnelMaxAge;

    //
    //  Expire old entries off of the timer queue.  We have to check
    //  for future time because the clock may jump as a result of
    //  hard clock change.  If we did not do this, a rogue entry
    //  with a future time could sit at the top of the queue and
    //  prevent entries from going away.
    //

    while (!IsListEmpty(&Cache->TimerQueue)) {

        Node = CONTAINING_RECORD(Cache->TimerQueue.Flink, TUNNEL_NODE, ListLinks);

        if (Node->CreateTime.QuadPart < ExpireTime.QuadPart ||
            Node->CreateTime.QuadPart > CurrentTime.QuadPart) {

#if defined(TUNNELTEST) || defined(KEYVIEW)
            DbgPrint("Expiring node %x (%ud%ud 1/10 msec too old)\n", Node, DblHex64(ExpireTime.QuadPart - Node->CreateTime.QuadPart));
#endif // TUNNELTEST

            FsRtlRemoveNodeFromTunnel(Cache, Node, FreePoolList, &Splay);

        } else {

            //
            //  No more nodes to be expired
            //

            break;
        }
    }

    //
    //  Remove entries until we're under the TunnelMaxEntries limit
    //

    while (Cache->NumEntries > TunnelMaxEntries) {

        Node = CONTAINING_RECORD(Cache->TimerQueue.Flink, TUNNEL_NODE, ListLinks);

#if defined(TUNNELTEST) || defined(KEYVIEW)
            DbgPrint("Dumping node %x (%d > %d)\n", Node, Cache->NumEntries, TunnelMaxEntries);
#endif // TUNNELTEST

        FsRtlRemoveNodeFromTunnel(Cache, Node, FreePoolList, &Splay);
    }

    return;
}


NTSTATUS
FsRtlGetTunnelParameterValue (
    IN PUNICODE_STRING ValueName,
    IN OUT PULONG Value
    )

/*++

Routine Description:

    Given a unicode value name this routine will go into the registry
    location for the Tunnel parameter information and get the
    value.

Arguments:

    ValueName - the unicode name for the registry value located in the
                double space configuration location of the registry.
    Value   - a pointer to the ULONG for the result.

Return Value:

    NTSTATUS

    If STATUS_SUCCESSFUL is returned, the location *Value will be
    updated with the DWORD value from the registry.  If any failing
    status is returned, this value is untouched.

--*/

{
    HANDLE Handle;
    NTSTATUS Status;
    ULONG RequestLength;
    ULONG ResultLength;
    UCHAR Buffer[KEY_WORK_AREA];
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;

    KeyName.Buffer = TUNNEL_KEY_NAME;
    KeyName.Length = sizeof(TUNNEL_KEY_NAME) - sizeof(WCHAR);
    KeyName.MaximumLength = sizeof(TUNNEL_KEY_NAME);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&Handle,
                       KEY_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {

        return Status;
    }

    RequestLength = KEY_WORK_AREA;

    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)Buffer;

    while (1) {

        Status = ZwQueryValueKey(Handle,
                                 ValueName,
                                 KeyValueFullInformation,
                                 KeyValueInformation,
                                 RequestLength,
                                 &ResultLength);

        ASSERT( Status != STATUS_BUFFER_OVERFLOW );

        if (Status == STATUS_BUFFER_OVERFLOW) {

            //
            // Try to get a buffer big enough.
            //

            if (KeyValueInformation != (PKEY_VALUE_FULL_INFORMATION)Buffer) {

                ExFreePool(KeyValueInformation);
            }

            RequestLength += 256;

            KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)
                                  ExAllocatePoolWithTag(PagedPool,
                                                        RequestLength,
                                                        'KnuT');

            if (!KeyValueInformation) {
                return STATUS_NO_MEMORY;
            }

        } else {

            break;
        }
    }

    ZwClose(Handle);

    if (NT_SUCCESS(Status)) {

        if (KeyValueInformation->DataLength != 0) {

            PULONG DataPtr;

            //
            // Return contents to the caller.
            //

            DataPtr = (PULONG)
              ((PUCHAR)KeyValueInformation + KeyValueInformation->DataOffset);
            *Value = *DataPtr;

        } else {

            //
            // Treat as if no value was found
            //

            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    if (KeyValueInformation != (PKEY_VALUE_FULL_INFORMATION)Buffer) {

        ExFreePool(KeyValueInformation);
    }

    return Status;
}


#if defined(TUNNELTEST) || defined(KEYVIEW)

VOID
DumpTunnel (
    PTUNNEL Tunnel
    )
{
    PRTL_SPLAY_LINKS SplayLinks, Ptr;
    PTUNNEL_NODE Node;
    PLIST_ENTRY Link;
    ULONG Indent = 1, i;
    ULONG EntryCount = 0;
    BOOLEAN CountOff = FALSE;

    DbgPrint("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

    DbgPrint("NumEntries = %d\n", Tunnel->NumEntries);
    DbgPrint("****** Cache Tree\n");

    SplayLinks = Tunnel->Cache;

    if (SplayLinks == NULL) {

        goto end;
    }

    while (RtlLeftChild(SplayLinks) != NULL) {

        SplayLinks = RtlLeftChild(SplayLinks);
        Indent++;
    }

    while (SplayLinks) {

        Node = CONTAINING_RECORD( SplayLinks, TUNNEL_NODE, CacheLinks );

        EntryCount++;

        DumpNode(Node, Indent);

        Ptr = SplayLinks;

        /*
          first check to see if there is a right subtree to the input link
          if there is then the real successor is the left most node in
          the right subtree.  That is find and return P in the following diagram

                      Links
                         \
                          .
                         .
                        .
                       /
                      P
                       \
        */

        if ((Ptr = RtlRightChild(SplayLinks)) != NULL) {

            Indent++;
            while (RtlLeftChild(Ptr) != NULL) {

                Indent++;
                Ptr = RtlLeftChild(Ptr);
            }

            SplayLinks = Ptr;

        } else {
            /*
              we do not have a right child so check to see if have a parent and if
              so find the first ancestor that we are a left decendent of. That
              is find and return P in the following diagram

                               P
                              /
                             .
                              .
                               .
                              Links
            */

            Ptr = SplayLinks;
            while (RtlIsRightChild(Ptr)) {

                Indent--;
                Ptr = RtlParent(Ptr);
            }

            if (!RtlIsLeftChild(Ptr)) {

                //
                //  we do not have a real successor so we simply return
                //  NULL
                //
                SplayLinks = NULL;

            } else {

                Indent--;
                SplayLinks = RtlParent(Ptr);
            }
        }
    }

    end:

    if (CountOff = (EntryCount != Tunnel->NumEntries)) {

        DbgPrint("!!!!!!!!!! Splay Tree Count Mismatch (%d != %d)\n", EntryCount, Tunnel->NumEntries);
    }

    EntryCount = 0;

    DbgPrint("****** Timer Queue\n");

    for (Link = Tunnel->TimerQueue.Flink;
         Link != &Tunnel->TimerQueue;
         Link = Link->Flink) {

        Node = CONTAINING_RECORD( Link, TUNNEL_NODE, ListLinks );

        EntryCount++;

        DumpNode(Node, 1);
    }

    if (CountOff |= (EntryCount != Tunnel->NumEntries)) {

        DbgPrint("!!!!!!!!!! Timer Queue Count Mismatch (%d != %d)\n", EntryCount, Tunnel->NumEntries);
    }

    ASSERT(!CountOff);

    DbgPrint("------------------------------------------------------------------\n");
}

#define MAXINDENT  128
#define INDENTSTEP 3

VOID
DumpNode (
    PTUNNEL_NODE Node,
    ULONG Indent
    )
{
    ULONG i;
    CHAR  SpaceBuf[MAXINDENT*INDENTSTEP + 1];

    Indent--;
    if (Indent > MAXINDENT) {
        Indent = MAXINDENT;
    }

    //
    //  DbgPrint is really expensive to iteratively call to do the indenting,
    //  so just build up the indentation all at once on the stack.
    //

    RtlFillMemory(SpaceBuf, Indent*INDENTSTEP, ' ');
    SpaceBuf[Indent*INDENTSTEP] = '\0';

    DbgPrint("%sNode 0x%x  CreateTime = %08x%08x, DirKey = %08x%08x, Flags = %d\n",
             SpaceBuf,
             Node,
             DblHex64(Node->CreateTime.QuadPart),
             DblHex64(Node->DirKey),
             Node->Flags );

    DbgPrint("%sShort = %wZ, Long = %wZ\n", SpaceBuf,
                                            &Node->ShortName,
                                            &Node->LongName );

    DbgPrint("%sP = %x, R = %x, L = %x\n", SpaceBuf,
                                           RtlParent(&Node->CacheLinks),
                                           RtlRightChild(&Node->CacheLinks),
                                           RtlLeftChild(&Node->CacheLinks) );
}
#endif // TUNNELTEST

