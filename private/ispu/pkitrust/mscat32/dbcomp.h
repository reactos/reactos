//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dbcomp.h
//
//  Contents:   Hash Database Compactor
//
//  History:    9-8-1998    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__DBCOMP_H__)
#define __DBCOMP_H__

#include <cbfile.hxx>
#include <catdb.hxx>

//
// Unique catalog structure
//

typedef struct _UNIQUE_CATALOG {

    HashMastRec HashDbRecord;
    DWORD       UniqueOffset;

} UNIQUE_CATALOG, *PUNIQUE_CATALOG;

#define INITIAL_UNIQUE_CATALOGS 10
#define GROW_UNIQUE_CATALOGS    5

#define DB_KEY_EXT              L".cbk"
#define DB_DATA_EXT             L".cbd"

#define KEY_RECORD_SIZE         24
#define KEY_SIZE                20

//
// Analyzes and compacts the current icky PBerkman hash database format with
// support for removal of unwanted catalog entries
//

class CHashDbCompactor
{
public:

    //
    // Construction
    //

    CHashDbCompactor ();
    ~CHashDbCompactor ();

    //
    // Initialize
    //

    BOOL Initialize (
             IN LPCWSTR pwszDbLock,
             IN LPCWSTR pwszDbDirectory
             );

    //
    // Original database locking
    //

    VOID LockDatabase ();
    VOID UnlockDatabase ();

    //
    // Map original database
    //

    BOOL MapDatabase (
            IN LPCWSTR pwszDbName,
            OUT PCRYPT_DATA_BLOB pKey,
            OUT LPWSTR* ppwszKeyPath,
            OUT PCRYPT_DATA_BLOB pData,
            OUT LPWSTR* ppwszDataPath
            );

    VOID UnmapDatabase (
              IN PCRYPT_DATA_BLOB pKey,
              IN PCRYPT_DATA_BLOB pData
              );

    //
    // Analyze the data for compaction
    //

    BOOL AnalyzeDataForCompaction (
                IN PCRYPT_DATA_BLOB pData,
                IN OPTIONAL LPCSTR pszUnwantedCatalog
                );

    //
    // Flush compaction analysis
    //

    BOOL FlushCompactionAnalysis ();

    //
    // Write the compacted database
    //

    BOOL WriteCompactedDatabase (
              IN PCRYPT_DATA_BLOB pKey,
              IN PCRYPT_DATA_BLOB pData,
              IN OPTIONAL LPCSTR pszUnwantedCatalog
              );

    //
    // Commit compacted database
    //

    BOOL CommitCompactedDatabase (
               IN LPCWSTR pwszFinalKeyPath,
               IN LPCWSTR pwszFinalDataPath
               );

    //
    // Free memory
    //

    VOID FreeString (IN LPWSTR pwsz);

private:

    //
    // Database lock
    //

    HANDLE          m_hDbLock;

    //
    // Database directory
    //

    LPWSTR          m_pwszDbDirectory;

    //
    // Compacted database data
    //

    DWORD           m_cAllocatedUniqueCatalogs;
    DWORD           m_cUniqueCatalogs;
    PUNIQUE_CATALOG m_aUniqueCatalogs;

    //
    // Temp database names
    //

    WCHAR           m_pwszTempKeyPath[MAX_PATH+1];
    WCHAR           m_pwszTempDataPath[MAX_PATH+1];

    //
    // Cached search data
    //

    DWORD           m_iLastUniqueCatalogFoundByName;

    //
    // Private methods
    //

    BOOL GrowUniqueCatalogs (DWORD cGrow);

    PUNIQUE_CATALOG FindUniqueCatalogByName (LPCSTR pszCatalogName);

    BOOL AddUniqueCatalog (HashMastRec* pHashMastRecord);

    BOOL CatalogFileExists (LPCSTR pszCatalogName, DWORD cbName);
};

//
// API for compacting the database
//

extern "C" {
BOOL WINAPI
CatalogCompactHashDatabase (
       IN LPCWSTR pwszDbLock,
       IN LPCWSTR pwszDbDirectory,
       IN LPCWSTR pwszDbName,
       IN OPTIONAL LPCWSTR pwszUnwantedCatalog
       );
}

#endif

