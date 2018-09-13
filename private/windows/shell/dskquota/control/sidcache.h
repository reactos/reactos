#ifndef _INC_DSKQUOTA_SIDCACHE_H
#define _INC_DSKQUOTA_SIDCACHE_H
///////////////////////////////////////////////////////////////////////////////
/*  File: sidcache.h

    Description: Header for classes associated with the SID/Name cache.


   Here's a ER-style diagram of the SID cache.  The cache consists
   of a data file and index file.  The index is a hash table consisting
   of an array of hash buckets, each containing 0 or more hash bucket entries.
   Each hash bucket entry contains the index of it's corresponding user record
   in the data file.  The index hash code is based on the user SID.

   The index is designed for fast lookup of queue entries.  Hash values are
   calculated by simply summing the value of the bytes in a SID and
   performing modulo division with the number of hash buckets.  Each hash
   bucket contains a doubly-linked list to handle hash value collisions
   and removal of expired entries. Each hash bucket entry contains the index
   of the starting block for the user's record in the data file.


                         +-----------+      +--------+<---> User SID
                     +-->| data file |<--->>| record |<---> User Domain
                     |   +-----------+      +--------+<---> User Name
                     |                            ^   <---> User Full User Name
    +-----------+<---+                            |
    |  cache    |                                 |
    +-----------+<---+                            +--- points to ----+
                     |                                               |
                     |                                               |
                     |   +----------+      +-------------+     +--------------+
                     +-->| ndx file |<--->>| hash bucket |<-->>| bucket entry |
                         +----------+      +-------------+     +--------------+

    <--->  = One-to-one
    <-->>  = One-to-many


   Notes:
     1. Because SID->Name resolution over the network is very slow
        (0-10 seconds), I anticipate that this cache will be used heavily
        and may contain 100's or 1000's of entries depending upon the
        volume's usage.

    Index file structure ------------------------------------------------------

        +------------------------+
        |      Header            | <--- Type INDEX_FILE_HDR
        +------------------------+
        |                        |
        |                        | <--- Array of DWORDs.  Count should be prime.
        |    Hash bucket array   |      Each element represents a hash bucket.
        |                        |      Unused elements contain NULL.
        |                        |      Used elements contain address of first
        |                        |      entry in hash bucket's entry list.
        +------------------------+
        |                        |
        |                        |
        |                        | <--- Array of INDEX_ENTRY.
        |      Index entries     |      Each entry is a node in a linked list.
        |                        |      Initially, all are on the "free list".
        |                        |      As entries are allocated, they are
        |                        |      transfered to the appropriate hash bucket's
        |                        |      list.  Each used entry contains the
        |                        |      starting block number of the corresponding
        |                        |      record in the data file.
        +------------------------+

    Data file structure -------------------------------------------------------

        +------------------------+
        |      Header            | <--- Type DATA_FILE_HDR
        +------------------------+
        |                        |
        |    Block alloc map     | <--- Array of DWORDs.  Each bit represents the
        |                        |      allocation state of a record block.
        |                        |      0 = free, 1 = allocated.
        +------------------------+
        |                        |
        |                        |
        |                        | <--- Array of BLOCKs. (32-bytes each)
        |    Record blocks       |      Each data record consists of one or more
        |                        |      blocks.  The first block in each record
        |                        |      is of type RECORD_HDR.  The remaining
        |                        |      blocks contain the variable-length SID,
        |                        |      Domain name, User name and Full User
        |                        |      name.  Blocks are aligned on quadword
        |                        |      boundaries for the FILETIME structure
        |                        |      in RECORD_HDR.  The 3 name string fields
        |                        |      are UNICODE and are WORD-aligned.
        |                        |      Unused blocks are filled with 0xCC.
        |                        |      A BLOCK is the smallest allocation unit.
        +------------------------+


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/12/96    Initial creation.                                    BrianAu
    08/14/96    Added SidCacheQueueIterator.                         BrianAu
    09/05/96    Added domain name string.                            BrianAu
    09/20/96    Total redesign.  Old design loaded data from file    BrianAu
                into an in-memory hash table.  New design leaves
                everything on disk and merely maps the file into
                memory.  Much more efficient with respect to
                speed and size.
    03/18/98    Replaced "domain", "name" and "full name" with       BrianAu
                "container", "logon name" and "display name" to
                better match the actual contents.  This was in
                reponse to making the quota UI DS-aware.  The
                "logon name" is now a unique key as it contains
                both account name and domain-like information.
                i.e. "REDMOND\brianau" or "brianau@microsoft.com".
*/
///////////////////////////////////////////////////////////////////////////////
const DWORD BITS_IN_BYTE         = 8;
const DWORD BITS_IN_DWORD        = BITS_IN_BYTE * sizeof(DWORD);


//
// On-disk structure of a single index entry.
// There's an entry for each record in the cache.
// Length = 16 bytes.
//
typedef struct index_entry
{
    index_entry *pPrev;
    index_entry *pNext;
    DWORD        iBlock;
    DWORD        iBucket;

} INDEX_ENTRY, *PINDEX_ENTRY;

//
// On-disk structure of index file header.
// Length = 48 bytes
// Must be even multiple of 8 bytes (quadword-align).
//
typedef struct index_file_hdr
{
    DWORD dwSignature;       // ID's file as a quota cache index.
    DWORD dwVersion;         // SW version that created file.
    GUID  guid;              // Verifies validity of data.
    DWORD cBuckets;          // Number of hash buckets.
    DWORD cMaxEntries;       // Max number of entries.
    DWORD cEntries;          // Number of used entries.
    PINDEX_ENTRY *pBuckets;  // Address of first hash bucket.
    PINDEX_ENTRY pEntries;   // Offset to first entry.
    PINDEX_ENTRY pFirstFree; // Offset to first entry in free list.

} INDEX_FILE_HDR, *PINDEX_FILE_HDR;

//
// Define what a data file "block" is.
// Keep the block size a power of 2.
//
const DWORD BLOCK_SIZE = 32;    // Size is in bytes.

typedef BYTE BLOCK[BLOCK_SIZE];
typedef BLOCK *PBLOCK;

//
// On-disk structure of data file header.
// Length = 48 bytes.
//
typedef struct data_file_hdr
{
    DWORD dwSignature;  // ID's file as quota cache data.
    DWORD dwVersion;    // SW version that created file.
    GUID  guid;         // Verifies validity of data.
    DWORD cBlocks;      // Total number of allocation blocks.
    DWORD cBlocksUsed;  // Number of used allocation blocks.
    DWORD cMapElements; // Number of alloc map elements.
    DWORD iFirstFree;   // Index of first free block.
    LPDWORD pdwMap;     // Address of alloc map.
    PBLOCK pBlocks;     // Address of block pool.

} DATA_FILE_HDR, *PDATA_FILE_HDR;

//
// On-disk structure of the header info for each record.
// This fixed length structure precedes the variable-length part.
// Quad-word aligned.
// Length = 32 bytes (1 block).
// Strings are UNICODE, WORD-aligned.
//
typedef struct record_hdr
{
    DWORD dwSignature;       // Rec hdr signature. (0xAAAA5555)
    DWORD cBlocks;           // Blks in record.
    FILETIME Birthday;       // When was record added to cache?
    DWORD cbOfsSid;          // Ofs from start of record.
    DWORD cbOfsContainer;    // Ofs from start of record.
    DWORD cbOfsLogonName;    // Ofs from start of record.
    DWORD cbOfsDisplayName;  // Ofs from start of record.

} RECORD_HDR, *PRECORD_HDR;


class SidNameCache
{
    private:
        //
        // Private class encapsulating the functions of the cache index.
        //
        class IndexMgr
        {
            private:

                SidNameCache&    m_refCache;     // Ref to outer cache object.
                PINDEX_FILE_HDR  m_pFileHdr;     // Same as g_pbMappedIndexFile.
                HANDLE           m_hFile;        // Handle to index file.
                HANDLE           m_hFileMapping; // Mapped file.
                CString          m_strFileName;  // Full path\name of index file.


                LPBYTE CreateIndexFile(LPCTSTR pszFile,
                                       DWORD cbFileHigh,
                                       DWORD cbFileLow);

                LPBYTE OpenIndexFile(LPCTSTR pszFile);

                LPBYTE GrowIndexFile(DWORD cGrowEntries);

                VOID CloseIndexFile(VOID);

                VOID InitNewIndexFile(DWORD cBuckets, DWORD cMaxEntries);

                PINDEX_ENTRY AllocEntry(VOID);

                VOID FreeEntry(PINDEX_ENTRY pEntry);
                VOID AddEntryToFreeList(PINDEX_ENTRY pEntry);
                PINDEX_ENTRY DetachEntry(PINDEX_ENTRY pEntry);

                PINDEX_ENTRY Find(PSID pSid);
                PINDEX_ENTRY Find(PSID pSid, DWORD dwHashCode);
                PINDEX_ENTRY Find(LPCTSTR pszLogonName);

                DWORD Hash(PSID pSid);

                PINDEX_ENTRY GetHashBucketValue(DWORD iBucket);
                VOID SetHashBucketValue(DWORD iBucket, PINDEX_ENTRY pEntry);

                //
                // Prevent copy.
                //
                IndexMgr(const IndexMgr& rhs);
                IndexMgr& operator = (const IndexMgr& rhs);


            public:
                IndexMgr(SidNameCache& refCache);
                ~IndexMgr(VOID);

                LPBYTE Initialize(LPCTSTR pszFile,
                                  DWORD cBuckets = 0,
                                  DWORD cMaxEntries = 0);

                DWORD Lookup(PSID pSid);
                DWORD Lookup(LPCTSTR pszLogonName);

                PINDEX_ENTRY Add(PSID pSid, DWORD iBlock);

                static UINT64 FileSize(DWORD cMaxEntries, DWORD cBuckets)
                    {
                        return (UINT64)(sizeof(INDEX_FILE_HDR)) +
                               (UINT64)(sizeof(PINDEX_ENTRY) * cBuckets) +
                               (UINT64)(sizeof(INDEX_ENTRY) * cMaxEntries);
                    }

                VOID SetFileGUID(const GUID *pguid);
                VOID GetFileGUID(LPGUID pguid);
                VOID FreeEntry(PSID pSid);
                VOID Clear(VOID);
#ifdef DBG
                VOID Dump(VOID);
#endif
        };

        //
        // Private class that manages the cache's stored data records.
        //
        class RecordMgr
        {
            private:
                SidNameCache&   m_refCache;          // Ref to outer cache object.
                PDATA_FILE_HDR  m_pFileHdr;          // Same as g_pbMappedDataFile
                HANDLE          m_hFile;             // Handle to data file.
                HANDLE          m_hFileMapping;      // Mapped data file.
                DWORD           m_cDaysRecLifeMin;   // Min and range are used to
                DWORD           m_cDaysRecLifeRange; // calc lifetime of records.
                CString         m_strFileName;       // Full path\name of data file.

                BOOL ValidBlockNumber(DWORD iBlock);
                VOID FillBlocks(DWORD iBlock, DWORD cBlocks, BYTE b);
                BOOL IsBitSet(LPDWORD pdwBase, DWORD iBit);
                VOID SetBit(LPDWORD pdwBase, DWORD iBit);
                VOID ClrBit(LPDWORD pdwBase, DWORD iBit);
                BOOL IsBlockUsed(DWORD iBlock);
                VOID MarkBlockUsed(DWORD iBlock);
                VOID MarkBlockUnused(DWORD iBlock);
                DWORD BlocksRequired(DWORD cb);
                DWORD BytesRequiredForRecord(
                    PSID pSid,
                    LPDWORD pcbSid,
                    LPCTSTR pszContainer,
                    LPDWORD pcbContainer,
                    LPCTSTR pszLogonName,
                    LPDWORD pcbLogonName,
                    LPCTSTR pszDisplayName,
                    LPDWORD pcbDisplayName);

                VOID FreeBlock(DWORD iBlock);
                VOID FreeBlocks(DWORD iFirstBlock, DWORD cBlocks);
                BLOCK *BlockAddress(DWORD iBlock);
                DWORD AllocBlocks(DWORD cbRecord);
                LPBYTE CreateDataFile(LPCTSTR pszFile, DWORD cbFileHigh, DWORD cbFileLow);
                LPBYTE OpenDataFile(LPCTSTR pszFile);
                LPBYTE GrowDataFile(DWORD cGrowBlocks);
                VOID InitNewDataFile(DWORD cBlocks);
                VOID CloseDataFile(VOID);

                //
                // Prevent copy.
                //
                RecordMgr(const RecordMgr& rhs);
                RecordMgr& operator = (const RecordMgr& rhs);


            public:

                RecordMgr(SidNameCache& refCache);
                ~RecordMgr(VOID);

                LPBYTE Initialize(
                    LPCTSTR pszFile,
                    DWORD cBlocks = 0);

                static UINT64 FileSize(DWORD cBlocks);

                DWORD Store(
                    PSID pSid,
                    LPCTSTR pszContainer,
                    LPCTSTR pszLogonName,
                    LPCTSTR pszDisplayName);

                BOOL RecordExpired(DWORD iBlock);

                HRESULT Retrieve(
                    DWORD iBlock,
                    PSID *ppSid,
                    LPTSTR *ppszContainer,
                    LPTSTR *ppszLogonName,
                    LPTSTR *ppszDisplayName);

                VOID SetFileGUID(const GUID *pguid);
                VOID GetFileGUID(LPGUID pguid);
                VOID FreeRecord(DWORD iFirstBlock);
                VOID Clear(VOID);
#ifdef DBG
                VOID Dump(VOID);
#endif
        };

        //
        // Private class for handling locks on cache files that must
        // be automatically released when an exception is thrown.
        //
        class CacheAutoLock
        {
            public:
                explicit CacheAutoLock(SidNameCache& Cache)
                    : m_Cache(Cache) { };

                BOOL Lock(VOID)
                    { return m_Cache.Lock(); }

                ~CacheAutoLock(VOID)
                    {m_Cache.ReleaseLock(); }

            private:
                SidNameCache& m_Cache;
                //
                // Prevent copy.
                //
                CacheAutoLock(const CacheAutoLock& rhs);
                CacheAutoLock& operator = (const CacheAutoLock& rhs);
        };

        friend class CacheAutoLock;

        HANDLE        m_hMutex;        // For locking during updates.
        IndexMgr     *m_pIndexMgr;     // Manages index file.
        RecordMgr    *m_pRecordMgr;    // Manages data file.
        CString       m_strFilePath;   // Path to cache files.

        BOOL Lock(VOID);
        VOID ReleaseLock(VOID);
        BOOL FilesAreValid(VOID);
        VOID ValidateFiles(VOID);
        VOID InvalidateFiles(VOID);
        VOID SetCacheFilePath(VOID);
        BOOL CreateCacheFileDirectory(LPCTSTR pszPath);

    public:
        SidNameCache(VOID);
        ~SidNameCache(VOID);

        static HRESULT
        CreateNewCache(
            SidNameCache **ppCache);

        HRESULT Initialize(
            BOOL bOpenExisting);

        HRESULT Lookup(
            PSID pSid,
            LPTSTR *ppszContainer,
            LPTSTR *ppszLogonName,
            LPTSTR *ppszDisplayName);

        HRESULT Lookup(
            LPCTSTR pszLogonName,
            PSID *ppSid);

        HRESULT Add(
            PSID pSid,
            LPCTSTR pszContainer,
            LPCTSTR pszLogonName,
            LPCTSTR pszDisplayName);

//        HRESULT CheckConsistency(VOID);

        BOOL Clear(VOID);

        HRESULT BeginTransaction(VOID);
        VOID EndTransaction(VOID);

        static LPBYTE OpenMappedFile(
                        LPCTSTR pszDataFile,
                        LPCTSTR pszMapping,
                        DWORD dwCreation,
                        DWORD cbFileHigh,
                        DWORD cbFileLow,
                        PHANDLE phFile,
                        PHANDLE phFileMapping);

        static BOOL IsQuadAligned(LPVOID pv);
        static BOOL IsQuadAligned(DWORD_PTR dw);
        static VOID QuadAlign(LPDWORD lpdwValue);
        static VOID WordAlign(LPDWORD lpdwValue);

#if DBG
        VOID Dump(VOID)
            { m_pIndexMgr->Dump(); m_pRecordMgr->Dump(); }
#endif

        friend class IndexMgr;
        friend class RecordMgr;

};

//
// We have one global SID cache.  Definition is in dskquota.cpp.
//
extern SidNameCache *g_pSidCache;

#endif // _INC_DSKQUOTA_SIDCACHE_H
