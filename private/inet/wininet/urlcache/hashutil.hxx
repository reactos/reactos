/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:  hashutil.hxx

Abstract:

    Interfaces to hashing functions.  Assumes access is serialized.
        HashLookupItem - finds and validates URL entry
        HashGetNextItem - enumerates but does not validate URL entries
    
Author:
    Rajeev Dujari (rajeevd) 04-Nov-1996
    
--*/

#define SLOT_BITS   6
#define SLOT_COUNT  (1 << SLOT_BITS)
#define SLOT_MASK   (SLOT_COUNT - 1)

// With 64 slots, the 6 low bits of HASH_ITEM::dwHash are implied by the slot.
// Currently, the low 3 bits are used instead as flags; the other 3 reserved.
// If bit0 clear, the item indicates a URL entry.
//   Then bit1 set if locked, bit2 set if trailing redirect slash allowed
// If bit0 set, the item is free or a redirect entry.
//   Then bit1 set if free item never used before
//     OR bit2 set if a redirect entry

// hash value bits
#define HASH_BIT_NOTURL   0x0001 // bit 0
#define HASH_BIT_LOCK     0x0002 // bit 1
#define HASH_BIT_REDIR    0x0004 // bit 2
#define HASH_BIT_HASGRP   0x0008 // bit 3
#define HASH_BIT_MULTGRP  0x0010 // bit 4
#define HASH_BIT_RESERVED 0x0020 // bit 5

// hash value flag combinations
#define HASH_UNLOCKED        0  // URL entry, not locked
#define HASH_FREE            1  // free item, was once in use
#define HASH_LOCKED          2  // URL entry, locked
#define HASH_END             3  // free item, never used before
#define HASH_UNLOCKED_SLASH  4  // URL entry, not locked, trailing slash redir
#define HASH_REDIR           5  // redirect entry
#define HASH_LOCKED_SLASH    6  // URL entry, locked, trailing slash redir
#define HASH_FLAG_MASK       7  // illegal, used to mask out hash flags

struct HASH_ITEM
{
    DWORD dwHash;
    DWORD dwOffset;

    // methods on hash value flags
    DWORD GetValue(void)        {return dwHash & ~SLOT_MASK;}
    void MarkFree(void)         {dwHash = HASH_FREE;}
    void SetSlash (void)        {dwHash |= HASH_BIT_REDIR;}
    void SetRedir (void)        {dwHash |= HASH_REDIR;}
    BOOL IsLocked(void)         {return dwHash & HASH_LOCKED;}
    void SetLocked(void)        {dwHash |=  HASH_LOCKED;}
    void ClearLocked(void)      {dwHash &= ~HASH_LOCKED;} 
    void MarkGroup(void)        {dwHash |=  HASH_BIT_HASGRP;}
    void ClearGroup(void)       {dwHash &= ~HASH_BIT_HASGRP;}
    BOOL HasGroup(void)         {return dwHash & HASH_BIT_HASGRP;}
    void MarkMultGroup(void)    {dwHash |= HASH_BIT_MULTGRP | HASH_BIT_HASGRP;}
    void ClearMultGroup(void)   {dwHash &= ~HASH_BIT_MULTGRP;}
    BOOL HasMultiGroup(void)    {return dwHash & HASH_BIT_MULTGRP; }
};

// HashLookupItem bits and flags
#define LOOKUP_BIT_REDIR        1
#define LOOKUP_BIT_CREATE       2
#define LOOKUP_BIT_SPARSE       4 // enable retrieval of partial file entries

#define LOOKUP_URL_NOCREATE     0 // find url entry, don't create new if not found
#define LOOKUP_URL_TRANSLATE    1 // find url entry, translate through redirects
#define LOOKUP_URL_CREATE       2 // find url entry, create new if not found
#define LOOKUP_REDIR_CREATE     3 // find redir entry, create new if not found

BOOL // returns TRUE if found
PUBLIC
HashLookupItem
(
    IN     LPVOID       pAllocObj,      // allocator object
    IN     LPBYTE       pBase,          // base for all offsets
    IN OUT LPDWORD      pdwTableOffset, // offset of first hash table section
    IN     LPCSTR       pszKey,         // key to search for
    IN     DWORD        dwFlags,        // flags as defined above
/////////////////////////////////////////////////////////////////////////////
// WARNING: if LOOKUP_*_CREATE set, the file might be grown and remapped   //
// so all pointers into the file must be recalculated from their offsets!  //
/////////////////////////////////////////////////////////////////////////////
    OUT    HASH_ITEM**  ppItem          // item if found or free slot
);


HASH_ITEM*
PUBLIC
HashGetNextItem
(
    IN     LPVOID       pAllocObj,      // allocator object
    IN     LPBYTE       pBase,          // base for all offsets
    IN OUT LPDWORD      pdwItemOffset,  // current item offset
    IN     DWORD        dwFlags         // include redirects?
);

