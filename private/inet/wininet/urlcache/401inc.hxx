#ifndef _401INC_H_
#define _401INC_H_

namespace ie401
{

//  on content upgrade, we need to check if the MUST_REVALIDATE_SZ symbol
//is set in the CACHE_CONTROL_SZ list.  This is now represented by the
//MUST_REVALIDATE_CACHE_ENTRY flag in the CacheEntryType.

#define CACHE_CONTROL_SZ "Cache-Control"
#define MUST_REVALIDATE_SZ "must-revalidate"

#define IE401_NOT_A_CACHE_SUBDIRECTORY 255
#define IE401_EDITED_CACHE_ENTRY              0x00000008

//  We need to know the offset into a ie401 indexfile to find the offset
//that points to the first hash table.  This is at 
//_MEMMAP_HEADER_SMALL.dwHashTableOffset..  The following has been cut from
//401 source to calculate that offset.

#define DIR_NAME_SIZE 8

struct CacheDir
{
    DWORD nFileCount;
    CHAR sDirName[DIR_NAME_SIZE];
};

#define DEFAULT_MAX_DIRS                32
//  the ie401_ was prefixed below to prevent a collision with a #define
//in the current code.
#define ie401_NUM_HEADER_DATA_DWORDS          8

struct _MEMMAP_HEADER_SMALL 
{

    TCHAR    FileSignature[(sizeof(TEXT("Client UrlCache MMF Ver 4.7")) / sizeof(TCHAR))];
    DWORD    FileSize;
    DWORD    dwHashTableOffset;
    DWORD    NumUrlInternalEntries;
    DWORD    NumUrlEntriesAlloced;
//  DWORD    dwGarbage; // due to /Zp8 struct alignment
    LONGLONG CacheLimit;
    LONGLONG CacheSize;
    LONGLONG ExemptUsage;
    DWORD    nDirCount;
    CacheDir DirArray[DEFAULT_MAX_DIRS];
    DWORD    dwHeaderData[ie401_NUM_HEADER_DATA_DWORDS];
};


//  The rest was cut'n'pasted from 401 source to allow to define
//the structures of the hash entries themselves.


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


struct FILEMAP_ENTRY
{
    DWORD dwSig;
    DWORD nBlocks;
};

struct URL_FILEMAP_ENTRY : FILEMAP_ENTRY
{
    LONGLONG LastModifiedTime;
    LONGLONG LastAccessedTime;
    LONGLONG ExpireTime;
    LONGLONG FileSize;               // should be DWORD
    GROUPID  GroupId;                // should be list
    DWORD    dwExemptDelta;
    DWORD    Size;                   // should be WORD
    DWORD    UrlNameOffset;          // should be WORD
    DWORD    DirIndex;               // should be BYTE
    DWORD    InternalFileNameOffset; // should be WORD
    DWORD    CacheEntryType;
    DWORD    HeaderInfoOffset;       // should be WORD
    DWORD    HeaderInfoSize;         // should be WORD
    DWORD    FileExtensionOffset;    // should be WORD
    DWORD    dostLastSyncTime; 
    DWORD    NumAccessed;            // should be WORD
    DWORD    NumReferences;          // should be WORD
    DWORD    dostFileCreationTime;
};


struct REDIR_FILEMAP_ENTRY : FILEMAP_ENTRY
{
    DWORD dwItemOffset;  // offset to hash table item of destination URL
    DWORD dwHashValue;   // destination URL hash value (BUGBUG: collisions?)
    char  szUrl[4];      // original URL, can occupy more bytes
};


struct HASH_FILEMAP_ENTRY : FILEMAP_ENTRY
{
    DWORD dwNext; // offset to next element in list
    DWORD nBlock; // sequence number for this block
};

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

// hash table parameters
#define BYTES_PER_PAGE 4096

#define ITEMS_PER_BUCKET ((BYTES_PER_PAGE - sizeof(HASH_FILEMAP_ENTRY))\
    / (SLOT_COUNT * sizeof(HASH_ITEM)))
#define BYTES_PER_TABLE (sizeof(HASH_FILEMAP_ENTRY) \
    + SLOT_COUNT * ITEMS_PER_BUCKET * sizeof(HASH_ITEM))

// HashLookupItem bits and flags
#define LOOKUP_BIT_REDIR        1
#define LOOKUP_BIT_CREATE       2
#define LOOKUP_BIT_SPARSE       4 // enable retrieval of partial file entries

#define LOOKUP_URL_NOCREATE     0 // find url entry, don't create new if not found
#define LOOKUP_URL_TRANSLATE    1 // find url entry, translate through redirects
#define LOOKUP_URL_CREATE       2 // find url entry, create new if not found
#define LOOKUP_REDIR_CREATE     3 // find redir entry, create new if not found


#define SIG_FREE   0xbadf00d
#define SIG_ALLOC  0xdeadbeef
#define SIG_HASH ('H'|('A'<<8)|('S'<<16)|('H'<<24))
#define SIG_URL    ' LRU'   // URL_FILEMAP_ENTRY
#define SIG_REDIR  'RDER'   // REDR_FILEMAP_ENTRY

}


#endif
