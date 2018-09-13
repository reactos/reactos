/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    cmdata.h

Abstract:

    This module contains data structures used by the 
    configuration manager.

Author:

    Dragos C. Sambotin (dragoss) 13-Jan-99

Revision History:

--*/

#ifndef __CM_DATA__
#define __CM_DATA__

// \nt\private\ntos\inc\hivedata.h
#include "hivedata.h"


//
// Limits on lengths of names, all in BYTES, all INCLUDING nulls.
//

#define MAX_KEY_PATH_LENGTH         65535       
#define MAX_KEY_NAME_LENGTH         512         // allow for 256 unicode, as promise
#define MAX_FRIENDLY_NAME_LENGTH    160         // allow for 80  unicode chars in FriendlyNames
#define MAX_KEY_VALUE_NAME_LENGTH   32767       // 32k - sanity limit for value name


//
// ----- Control structures, object manager structures ------
//


//
// CM_KEY_CONTROL_BLOCK
//
// One key control block exists for each open key.  All of the key objects
// (open instances) for the key refer to the key control block.
//


typedef ULONG HASH_VALUE;

typedef struct _CM_KEY_HASH {
    ULONG   ConvKey;
    struct _CM_KEY_HASH *NextHash;
    PHHIVE     KeyHive;                         // Hive containing CM_KEY_NODE
    HCELL_INDEX KeyCell;                        // Cell containing CM_KEY_NODE
} CM_KEY_HASH, *PCM_KEY_HASH;

#ifdef CM_DEBUG_KCB
#define KCB_SIGNATURE 'bKmC'

#define SET_KCB_SIGNATURE(_kcb_,_sig_) (_kcb_)->Signature = (_sig_)
#define ASSERT_KCB(_kcb_) ASSERT((_kcb_)->Signature == KCB_SIGNATURE)
#define ASSERT_KEY_HASH(_keyhash_) ASSERT_KCB(CONTAINING_RECORD((_keyhash_), CM_KEY_CONTROL_BLOCK, KeyHash))
#else
#define SET_KCB_SIGNATURE(_kcb_,_sig_)
#define ASSERT_KCB(_kcb_)
#define ASSERT_KEY_HASH(_keyhash_)
#endif



//
// The registry is a large data structure that has had poor locality.
// To improve performance without changing the on disk structure, we
// cache the frequently used registry data to minimize reference on
// registry data.
//
// A KCB (Key Control Block) is the core structure for registry cache.
// It uses HashValue for quick cache lookup and contains the most
// frequently used data in a key node.
//
// It contains the most frequently used data in a key node:
// Security, Flags, and Value index.
//
// A KCB may also contains additional information
// (which are cached lazily) about its subkeys, value nodes and values' data.
//
// The subkey information is distinquished by ExtFlags.  See CM_KCB_* below.
// The value nodes and data are distinguished by a bit in the vairable.
// See CMP_IS_CELL_CACHED.
//
// Caches for value data will be created during query process, the cached
// structure is shown as the following picture.  The structure is almost
// the same as the registry structure
// except they are pointers to the allocation instead of offset index on hive.
//
// To minimize the name string storage space KCB's,  we do not store the complete
// path name of the key in the kcb, instead, we implemented the tree structure
// (like the registry hive structure) to share name prefix.
// Also, knowing that there are lots of keys sharing same names,
// we create NameBlock strucuture so KCB's of same names
// can share the NameBlock.  NameBlock is compressed.
//
// Meanings when the following bits are set in ExtFlags:
// 1. The following bits are used for Parse and are for
//    non-symbolic keys.  Also, at most one bit can be set at any given time.
//    CM_KCB_KEY_NON_EXIST : This key is a fake key (no such key in the hive).
//    CM_KCB_NO_SUBKEY     : This key is has no subkey.
//    CM_KCB_SUBKEY_ONE    : This key has only one subkey and IndexHint is
//                           the first four characters of this subkey.
//    CM_KCB_SUBKEY_HINT   : This key has the first four characters of all
//                           its subkeys (buffer pointed by IndexHint).
//
// 2. CM_KCB_SYM_LINK_FOUND: This bit is only for symbolic keys.  It
//                           indicates that the symbolic link has been
//                           resolved and the KCB for the link is pointed to
//                           by ValueCache.RealKcb.
//                           In this case, the Value Index of this key is no longer
//                           available in the KCB.  (We hardly query the value
//                           of a symbolic link key other than finding the path
//                           of the real key anyway).
//
// 3. CM_KCB_NO_DELAY_CLOSE: This bit is only used for non-symbolic keys and is
//                           independent of bits on item 1. When set, it indicates that
//                           key should not be kept in delay close when the refererence
//                           count goes to zero.
//                           This is for the case when a key has no open handles but
//                           still has subkeys in the cache.
//                           When its last subkey is kicked out of cache, we do not
//                           want to keep this key around.
//                           This is done so CmpSearchForOpenSubKeysInCachen can clean
//                           up the cache properly before a key can be unloaded.
//
//
//   KCB
//   +-------------------+
//   | ...               |      (Typical case)
//   +-------------------+      Value Index
//   | ValueCache        |  +-->+---------+         Value Key (with small data)
//   +  +----------------+  |   |        o--------->+-----------+
//   |  | ValueList     o---+   +---------+         | ....      |
//   |  +---- Union -----|      |         |         +-----------+
//   |  | RealKcb       o---+   +---------+         | Data (S)  |
//   |  +----------------|  |   |         |         +-----------+
//   |                   |  |   +---------+
//   |                   |  |   |         |
//   |                   |  |   +---------+         Value Key (with large data)
//   |                   |  |   |        o--------->+-----------+
//   |                   |  |   +---------+         | ...       |
//   |                   |  |   |         |         +-----------+
//   |                   |  |   +---------+         | Data (L) o------+
//   |                   |  |                       +-----------+     |
//   |                   |  |                       |           | <---+ (Append at the end of Value Node)
//   |                   |  |                       |           |
//   |                   |  |                       |           |
//   |                   |  |                       +-----------+
//   |                   |  |
//   |                   |  |   KCB (Symbolic link key, CM_KCB_SYM_LINK_FOUND set).
//   |                   |  +-->+---------+
//   |                   |      |         |
//   |                   |      |         |
//   |                   |      |         |
//   |                   |      |         |
//   |                   |      |         |
//   |                   |      +---------+
//   |                   |
//   | ...               |
//   +-------------------+      Index Hint
//   | IndexHint        o------>+---------+
//   +-------------------+      | 4 char  |
//   |                   |      +---------+
//   |                   |      | 4 char  |
//   +-------------------+      +---------+
//   |                   |      (CM_KCB_SUBKEY_HINT)
//   |                   |
//   |                   |
//   +-------------------+                 Name Block
//   | NameBlock        o----------------->+----------+
//   +-------------------+                 |          |
//                                         +----------+
//
//
// The TotalLevels is used for quick comparison for notification and cache lookup.
//
// *** MP Synchronization ***
// The KCB lock is held for any write to KCB unless the registry is locked exclusively.
// KCB is also locked while reading fields that can be modified by another thread
// during a read operation, i.e., when the registry lock is held shared.
//
// The fields are the follows: ExtFlags, ValueCache, IndexInfo, IndexHint, or NameHint.
//
// Reading of other entries in the KCB does not need to hold the KCB lock since
// these entries will not change for any registry read operation.  When there
// are changes to these entries, registry must be locked exclusively.
//
// NOTE: the KCB size is 56 bytes now, plus the pool header of 8 bytes,
//       it fits into a 64byte allocation.  Think carefully if you want to
//       enlarge the data structure.  Also, watch it if the pool allocation code changes.
//
//       The RefCount in KCB is the number of open handles plus the number of cached subkeys.
//       We can change this by having a RefCount and a CachedSubKeyCount.  To not grow the
//       structure size, we can merge the boolean Delete into ExtFlags.

typedef struct _CM_NAME_HASH {
    ULONG   ConvKey;
    struct _CM_NAME_HASH *NextHash;
    USHORT  NameLength;      // Length of string value
    WCHAR   Name[1] ;      // The actual string value
} CM_NAME_HASH, *PCM_NAME_HASH;

typedef struct _CM_NAME_CONTROL_BLOCK {
    BOOLEAN     Compressed;       // Flags to indicate which extension we have.
    USHORT      RefCount;
    union {
        CM_NAME_HASH             NameHash;
        struct {
            ULONG   ConvKey;
            struct _CM_KEY_HASH *NextHash;
            USHORT  NameLength;      // Length of string value
            WCHAR   Name[1] ;      // The actual string value
        };
    };
} CM_NAME_CONTROL_BLOCK, *PCM_NAME_CONTROL_BLOCK;

typedef struct _CM_INDEX_HINT_BLOCK {
    ULONG   Count;
    UCHAR   NameHint[1];
} CM_INDEX_HINT_BLOCK, *PCM_INDEX_HINT_BLOCK;

typedef struct _CACHED_CHILD_LIST {
    ULONG       Count;                  // 0 for empty list
    union {
        ULONG_PTR   ValueList;
        struct _CM_KEY_CONTROL_BLOCK *RealKcb;
    };
} CACHED_CHILD_LIST, *PCACHED_CHILD_LIST;

//
// Define the HINT Length used
//
#define CM_SUBKEY_HINT_LENGTH   4
#define CM_MAX_CACHE_HINT_SIZE 14

//
// Bits used in the ExtFlags in KCB.
//

#define CM_KCB_NO_SUBKEY        0x0001      // This key has no subkeys
#define CM_KCB_SUBKEY_ONE       0x0002      // This key has only one subkey and the
                                            // first 4 char
                                            //
#define CM_KCB_SUBKEY_HINT      0x0004
#define CM_KCB_SYM_LINK_FOUND   0x0008
#define CM_KCB_KEY_NON_EXIST    0x0010
#define CM_KCB_NO_DELAY_CLOSE   0x0020

#define CM_KCB_CACHE_MASK (CM_KCB_NO_SUBKEY | \
                           CM_KCB_KEY_NON_EXIST | \
                           CM_KCB_SUBKEY_ONE | \
                           CM_KCB_SUBKEY_HINT)


#define KCB_TO_KEYBODY_LINK

typedef struct _CM_KEY_CONTROL_BLOCK {
#ifdef CM_DEBUG_KCB
    ULONG                       Signature;
#endif
    BOOLEAN                     Delete;
    USHORT                      RefCount;
    union {
        CM_KEY_HASH             KeyHash;
        struct {
            ULONG               ConvKey;
            struct _CM_KEY_HASH *NextHash;
            PHHIVE              KeyHive;        // Hive containing CM_KEY_NODE
            HCELL_INDEX         KeyCell;         // Cell containing CM_KEY_NODE
        };
    };
    USHORT                      ExtFlags;       // Flags to indicate which extension we have.
    USHORT                      Flags;          // Same Flags as KeyNode
    struct _CM_KEY_CONTROL_BLOCK *ParentKcb;
    PCM_NAME_CONTROL_BLOCK      NameBlock;
    HCELL_INDEX                 Security;
    struct _CACHED_CHILD_LIST   ValueCache;
    union {
        PCM_INDEX_HINT_BLOCK    IndexHint;                          // CM_KCB_SUBKEY_HINT
        UCHAR                   NameHint[CM_SUBKEY_HINT_LENGTH];    // CM_KCB_SUBKEY_ONE
    };
    USHORT DelayedCloseIndex;
    USHORT TotalLevels;
    struct _CM_KEY_NODE         *KeyNode;       // pointer to CM_KEY_NODE
#ifdef KCB_TO_KEYBODY_LINK
    LIST_ENTRY                  KeyBodyListHead; // head of the list with all key_nodes using this kcb 
#endif
} CM_KEY_CONTROL_BLOCK, *PCM_KEY_CONTROL_BLOCK;


//
// ----- Structures used to implement registry hierarchy -----
//

typedef enum _NODE_TYPE {
    KeyBodyNode,
    KeyValueNode
} NODE_TYPE;


typedef enum _CMP_COPY_TYPE {
    Copy,
    Sync,
    Merge
} CMP_COPY_TYPE;

typedef enum _SUBKEY_SEARCH_TYPE {
    SearchIfExist,
    SearchAndDeref,
    SearchAndCount
} SUBKEY_SEARCH_TYPE;

//
// ChildList
//
//      NOTE:   CHILD_LIST structures are normally refered to
//              with HCELL_INDEX, not PCHILD_LIST vars.
//

typedef struct _CHILD_LIST {
    ULONG       Count;                  // 0 for empty list
    HCELL_INDEX List;
} CHILD_LIST, *PCHILD_LIST;

//
// CM_KEY_REFERENCE
//

typedef struct  _CM_KEY_REFERENCE {
    HCELL_INDEX KeyCell;
    PHHIVE      KeyHive;
} CM_KEY_REFERENCE , *PCM_KEY_REFERENCE;

//
// ----- CM_KEY_INDEX -----
//
// A leaf index may be one of two types. The "old" CM_KEY_INDEX type is used for
// hives circa NT3.1, 3.5, and 3.51. NT4.0 introduces the newer CM_KEY_FAST_INDEX
// which is used for all leaf indexes that have less than CM_MAX_FAST_INDEX leaves.
//
// The main advantage of the fast index is that the first four characters of the
// names are stored within the index itself. This almost always saves us from having
// to fault in a number of unneccessary pages when searching for a given key.
//
// The main disadvantage is that each subkey requires twice as much storage. One dword
// for the HCELL_INDEX and one dword to hold the first four characters of the subkey
// name. If one of the first four characters in the subkey name is a unicode character
// where the high byte is non-zero, the actual subkey must be examined to determine the
// name.
//
// Hive version 1 & 2 do not support the fast index. Version 3 adds support for the
// fast index. All hives that are newly created on a V3-capable system are therefore
// unreadable on V1 & 2 systems.
//
// N.B. There is code in cmindex.c that relies on the Signature and Count fields of
//      CM_KEY_INDEX and CM_KEY_FAST_INDEX being at the same offset in the structure!

#define UseFastIndex(Hive) ((Hive)->Version>=3)

#define CM_KEY_INDEX_ROOT   0x6972      // ir
#define CM_KEY_INDEX_LEAF   0x696c      // il
#define CM_KEY_FAST_LEAF    0x666c      // fl

typedef struct _CM_INDEX {
    HCELL_INDEX Cell;
    UCHAR NameHint[4];                  // upcased first four chars of name
} CM_INDEX, *PCM_INDEX;

typedef struct _CM_KEY_FAST_INDEX {
    USHORT      Signature;              // also type selector
    USHORT      Count;
    CM_INDEX    List[1];                // Variable sized array
} CM_KEY_FAST_INDEX, *PCM_KEY_FAST_INDEX;

typedef struct _CM_KEY_INDEX {
    USHORT      Signature;              // also type selector
    USHORT      Count;
    HCELL_INDEX List[1];                // Variable sized array
} CM_KEY_INDEX, *PCM_KEY_INDEX;

//
// Allow index to grow to size that will cause allocation of exactly
// one logical block.  Works out to be 1013 entries.
//
#define CM_MAX_INDEX                                                        \
 ( (HBLOCK_SIZE-                                                             \
    (sizeof(HBIN)+FIELD_OFFSET(HCELL,u)+FIELD_OFFSET(CM_KEY_INDEX,List))) /  \
    sizeof(HCELL_INDEX) )

#define CM_MAX_LEAF_SIZE ((sizeof(HCELL_INDEX)*CM_MAX_INDEX) + \
                          (FIELD_OFFSET(CM_KEY_INDEX, List)))

//
// Allow index to grow to size that will cause allocation of exactly
// one logical block.  Works out to be approx. 500 entries.
//
#define CM_MAX_FAST_INDEX                                                    \
 ( (HBLOCK_SIZE-                                                             \
    (sizeof(HBIN)+FIELD_OFFSET(HCELL,u)+FIELD_OFFSET(CM_KEY_FAST_INDEX,List))) /  \
    sizeof(CM_INDEX) )

#define CM_MAX_FAST_LEAF_SIZE ((sizeof(CM_INDEX)*CM_MAX_FAST_INDEX) + \
                          (FIELD_OFFSET(CM_KEY_FAST_INDEX, List)))



//
// ----- CM_KEY_NODE -----
//

#define CM_KEY_NODE_SIGNATURE     0x6b6e           // "kn"
#define CM_LINK_NODE_SIGNATURE     0x6b6c          // "kl"

#define KEY_VOLATILE        0x0001      // This key (and all its children)
                                        // is volatile.

#define KEY_HIVE_EXIT       0x0002      // This key marks a bounary to another
                                        // hive (sort of a link).  The null
                                        // value entry contains the hive
                                        // and hive index of the root of the
                                        // child hive.

#define KEY_HIVE_ENTRY      0x0004      // This key is the root of a particular
                                        // hive.

#define KEY_NO_DELETE       0x0008      // This key cannot be deleted, period.

#define KEY_SYM_LINK        0x0010      // This key is really a symbolic link.
#define KEY_COMP_NAME       0x0020      // The name for this key is stored in a
                                        // compressed form.
#define KEY_PREDEF_HANDLE   0x0040      // There is no real key backing this,
                                        // return the predefined handle.
                                        // Predefined handles are stashed in
                                        // ValueList.Count.

#pragma pack(4)
typedef struct _CM_KEY_NODE {
    USHORT      Signature;
    USHORT      Flags;
    LARGE_INTEGER LastWriteTime;
    ULONG       Spare;                  // used to be TitleIndex
    HCELL_INDEX Parent;
    ULONG       SubKeyCounts[HTYPE_COUNT];  // Stable and Volatile
    union {
        struct {
            HCELL_INDEX SubKeyLists[HTYPE_COUNT];   // Stable and Volatile
            CHILD_LIST  ValueList;
        };
        CM_KEY_REFERENCE    ChildHiveReference;
    };

    HCELL_INDEX Security;
    HCELL_INDEX Class;
    ULONG       MaxNameLen;
    ULONG       MaxClassLen;
    ULONG       MaxValueNameLen;
    ULONG       MaxValueDataLen;

    ULONG       WorkVar;                // WARNING: This DWORD is used
                                        //          by the system at run
                                        //          time, do attempt to
                                        //          store user data in it.

    USHORT      NameLength;
    USHORT      ClassLength;
    WCHAR       Name[1];                // Variable sized array
} CM_KEY_NODE, *PCM_KEY_NODE;
#pragma pack()

//
// ----- CM_KEY_VALUE -----
//

#define CM_KEY_VALUE_SIGNATURE    0x6b76          // "kv"

#define CM_KEY_VALUE_SPECIAL_SIZE   0x80000000       // 2 gig

#define CM_KEY_VALUE_SMALL          4

#define VALUE_COMP_NAME 0x0001              // The name for this value is stored in a
                                            // compressed form.

typedef struct _CM_KEY_VALUE {
    USHORT      Signature;
    USHORT      NameLength;
    ULONG       DataLength;
    HCELL_INDEX Data;
    ULONG       Type;
    USHORT      Flags;                      // Used to be TitleIndex
    USHORT      Spare;                      // Used to be TitleIndex
    WCHAR       Name[1];                    // Variable sized array
} CM_KEY_VALUE, *PCM_KEY_VALUE;

//
// realsize is set to real size, returns TRUE if small, else FALSE
//
#define CmpIsHKeyValueSmall(realsize, size)                   \
        ((size >= CM_KEY_VALUE_SPECIAL_SIZE) ?                       \
        ((realsize) = size - CM_KEY_VALUE_SPECIAL_SIZE, TRUE) :       \
        ((realsize) = size, FALSE))

//
// ----- CM_KEY_SECURITY -----
//

#define CM_KEY_SECURITY_SIGNATURE 0x6b73              // "ks"

typedef struct _CM_KEY_SECURITY {
    USHORT                  Signature;
    USHORT                  Reserved;
    HCELL_INDEX             Flink;
    HCELL_INDEX             Blink;
    ULONG                   ReferenceCount;
    ULONG                   DescriptorLength;
    SECURITY_DESCRIPTOR_RELATIVE     Descriptor;         // Variable length
} CM_KEY_SECURITY, *PCM_KEY_SECURITY;

//
// ----- CELL_DATA -----
//
// Union of types of data that could be in a cell
//

typedef struct _CELL_DATA {
    union _u {
        CM_KEY_NODE      KeyNode;
        CM_KEY_VALUE     KeyValue;
        CM_KEY_SECURITY  KeySecurity;    // Variable security descriptor length
        CM_KEY_INDEX     KeyIndex;       // Variable sized structure
        HCELL_INDEX      KeyList[1];     // Variable sized array
        WCHAR            KeyString[1];   // Variable sized array
    } u;
} CELL_DATA, *PCELL_DATA;


//
// Unions for KEY_INFORMATION, KEY_VALUE_INFORMATION
//

typedef union _KEY_INFORMATION {
    KEY_BASIC_INFORMATION   KeyBasicInformation;
    KEY_NODE_INFORMATION    KeyNodeInformation;
    KEY_FULL_INFORMATION    KeyFullInformation;
    KEY_NAME_INFORMATION    KeyNameInformation;
} KEY_INFORMATION, *PKEY_INFORMATION;

typedef union _KEY_VALUE_INFORMATION {
    KEY_VALUE_BASIC_INFORMATION KeyValueBasicInformation;
    KEY_VALUE_FULL_INFORMATION  KeyValueFullInformation;
    KEY_VALUE_PARTIAL_INFORMATION KeyValuePartialInformation;
    KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 KeyValuePartialInformationAlign64;
} KEY_VALUE_INFORMATION, *PKEY_VALUE_INFORMATION;



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

#define CM_CACHE_DATA_NOT_CACHED 0
#define CM_CACHE_DATA_CACHED     1
#define CM_CACHE_DATA_TOO_BIG    2
#define MAXIMUM_CACHED_DATA   2048  // Maximum data size to be cached.

typedef struct _CM_CACHED_VALUE_INDEX {
    HCELL_INDEX CellIndex;
    union {
        CELL_DATA        CellData;
        ULONG_PTR        List[1];
    } Data;
} CM_CACHED_VALUE_INDEX, *PCM_CACHED_VALUE_INDEX; // This is only used as a pointer.

typedef struct _CM_CACHED_VALUE {
    USHORT DataCacheType;
    USHORT ValueKeySize;
    CM_KEY_VALUE  KeyValue;
} CM_CACHED_VALUE, *PCM_CACHED_VALUE; // This is only used as a pointer.

typedef PCM_CACHED_VALUE *PPCM_CACHED_VALUE;

#define CMP_CELL_CACHED_MASK  1

#define CMP_IS_CELL_CACHED(Cell) (((ULONG_PTR) (Cell) & CMP_CELL_CACHED_MASK) && ((Cell) != (ULONG_PTR) HCELL_NIL))
#define CMP_GET_CACHED_ADDRESS(Cell) (((ULONG_PTR) (Cell)) & ~CMP_CELL_CACHED_MASK)
#define CMP_GET_CACHED_CELLDATA(Cell) (&(((PCM_CACHED_VALUE_INDEX)(((ULONG_PTR) (Cell)) & ~CMP_CELL_CACHED_MASK))->Data.CellData))
#define CMP_GET_CACHED_KEYVALUE(Cell) (&(((PCM_CACHED_VALUE)(((ULONG_PTR) (Cell)) & ~CMP_CELL_CACHED_MASK))->KeyValue))
#define CMP_GET_CACHED_CELL(Cell) (((PCM_CACHED_ENTRY)(((ULONG_PTR) (Cell)) & ~CMP_CELL_CACHED_MASK))->CellIndex)
#define CMP_MARK_CELL_CACHED(Cell) (((ULONG_PTR) (Cell)) | CMP_CELL_CACHED_MASK)

#define CMP_GET_CACHED_CELL_INDEX(Cell) (PtrToUlong((PVOID) (Cell)))


#endif //__CM_DATA__


