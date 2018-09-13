//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dbcomp.cpp
//
//  Contents:   Hash Database Compactor
//
//  History:    9-8-1998    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
//+---------------------------------------------------------------------------
//
//  Member:     CHashDbCompactor::CHashDbCompactor, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CHashDbCompactor::CHashDbCompactor ()
{
    m_hDbLock = NULL;
    m_pwszDbDirectory = NULL;

    m_cUniqueCatalogs = 0;
    m_cAllocatedUniqueCatalogs = 0;
    m_aUniqueCatalogs = NULL;
    m_iLastUniqueCatalogFoundByName = 0;

    m_pwszTempKeyPath[0] = L'\0';
    m_pwszTempDataPath[0] = L'\0';
}

//+---------------------------------------------------------------------------
//
//  Member:     CHashDbCompactor::~CHashDbCompactor, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CHashDbCompactor::~CHashDbCompactor ()
{
    if ( m_hDbLock != NULL )
    {
        CloseHandle( m_hDbLock );
    }

    if ( m_pwszTempKeyPath[ 0 ] != L'\0' )
    {
        DeleteFileU( m_pwszTempKeyPath );
    }

    if ( m_pwszTempDataPath[ 0 ] != L'\0' )
    {
        DeleteFileU( m_pwszTempDataPath );
    }

    delete m_pwszDbDirectory;
    delete m_aUniqueCatalogs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHashDbCompactor::Initialize, public
//
//  Synopsis:   initialize the compactor
//
//----------------------------------------------------------------------------
BOOL
CHashDbCompactor::Initialize (
                      IN LPCWSTR pwszDbLock,
                      IN LPCWSTR pwszDbDirectory
                      )
{
    if ( ( m_hDbLock = CreateMutexU( NULL, FALSE, pwszDbLock ) ) == NULL )
    {
        return( FALSE );
    }

    m_pwszDbDirectory = new WCHAR [ wcslen( pwszDbDirectory ) + 1 ];
    if ( m_pwszDbDirectory != NULL )
    {
        wcscpy( m_pwszDbDirectory, pwszDbDirectory );
    }
    else
    {
        SetLastError( E_OUTOFMEMORY );
        return( FALSE );
    }

    if ( GrowUniqueCatalogs( INITIAL_UNIQUE_CATALOGS ) == FALSE )
    {
        return( FALSE );
    }

    assert( m_aUniqueCatalogs != NULL );
    assert( m_cAllocatedUniqueCatalogs == INITIAL_UNIQUE_CATALOGS );

    if ( ( GetTempFileNameU(
              pwszDbDirectory,
              NULL,
              0,
              m_pwszTempKeyPath
              ) == FALSE ) ||
         ( GetTempFileNameU(
              pwszDbDirectory,
              NULL,
              0,
              m_pwszTempDataPath
              ) == FALSE ) )
    {
        return( FALSE );
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CHashDbCompactor::LockDatabase, public
//
//  Synopsis:   lock the database
//
//----------------------------------------------------------------------------
VOID
CHashDbCompactor::LockDatabase ()
{
    WaitForSingleObject( m_hDbLock, INFINITE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CHashDbCompactor::UnlockDatabase, public
//
//  Synopsis:   unlock the database
//
//----------------------------------------------------------------------------
VOID
CHashDbCompactor::UnlockDatabase ()
{
    ReleaseMutex( m_hDbLock );
}

//+---------------------------------------------------------------------------
//
//  Member:     CHashDbCompactor::MapDatabase, public
//
//  Synopsis:   map the database
//
//----------------------------------------------------------------------------
BOOL
CHashDbCompactor::MapDatabase (
                     IN LPCWSTR pwszDbName,
                     OUT PCRYPT_DATA_BLOB pKey,
                     OUT LPWSTR* ppwszKeyPath,
                     OUT PCRYPT_DATA_BLOB pData,
                     OUT LPWSTR* ppwszDataPath
                     )
{
    BOOL   fResult = TRUE;
    LPWSTR pwszKeyDbPath = NULL;
    LPWSTR pwszDataDbPath = NULL;
    DWORD  cwDirectory = 0;
    DWORD  cwName = 0;
    HANDLE hKeyFile = INVALID_HANDLE_VALUE;
    HANDLE hDataFile = INVALID_HANDLE_VALUE;
    HANDLE hMappedKeyFile = NULL;
    HANDLE hMappedDataFile = NULL;
    LPBYTE pbKey = NULL;
    LPBYTE pbData = NULL;
    DWORD  cwKeyExt = 0;
    DWORD  cwDataExt = 0;
    DWORD  cbKeyFileSize = 0;
    DWORD  cbDataFileSize = 0;

    cwDirectory = wcslen( m_pwszDbDirectory );
    cwName = wcslen( pwszDbName );
    cwKeyExt = wcslen( DB_KEY_EXT );
    cwDataExt = wcslen( DB_DATA_EXT );

    pwszKeyDbPath = new WCHAR [ cwDirectory + cwName + cwKeyExt + 2 ];
    pwszDataDbPath = new WCHAR [ cwDirectory + cwName + cwDataExt + 2 ];

    if ( ( pwszKeyDbPath != NULL ) && ( pwszDataDbPath != NULL ) )
    {
        wcscpy( pwszKeyDbPath, m_pwszDbDirectory );
        wcscat( pwszKeyDbPath, L"\\" );
        wcscat( pwszKeyDbPath, pwszDbName );

        wcscpy( pwszDataDbPath, pwszKeyDbPath );

        wcscat( pwszKeyDbPath, DB_KEY_EXT );
        wcscat( pwszDataDbPath, DB_DATA_EXT );
    }
    else
    {
        SetLastError( E_OUTOFMEMORY );
        fResult = FALSE;
    }

    if ( fResult == TRUE )
    {
        hKeyFile = CreateFileU(
                         pwszKeyDbPath,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL
                         );

        hDataFile = CreateFileU(
                          pwszDataDbPath,
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL
                          );

        if ( ( hKeyFile != INVALID_HANDLE_VALUE ) &&
             ( hDataFile != INVALID_HANDLE_VALUE ) )
        {
            cbKeyFileSize = GetFileSize( hKeyFile, NULL );
            cbDataFileSize = GetFileSize( hDataFile, NULL );

            if ( cbKeyFileSize > 0 )
            {
                hMappedKeyFile = CreateFileMapping(
                                       hKeyFile,
                                       NULL,
                                       PAGE_READONLY,
                                       0,
                                       0,
                                       NULL
                                       );
            }

            if ( cbDataFileSize > 0 )
            {
                hMappedDataFile = CreateFileMapping(
                                        hDataFile,
                                        NULL,
                                        PAGE_READONLY,
                                        0,
                                        0,
                                        NULL
                                        );
            }
        }

        if ( hMappedKeyFile != NULL )
        {
            pbKey = (LPBYTE)MapViewOfFile(
                               hMappedKeyFile,
                               FILE_MAP_READ,
                               0,
                               0,
                               0
                               );
        }

        if ( hMappedDataFile != NULL )
        {
            pbData = (LPBYTE)MapViewOfFile(
                                hMappedDataFile,
                                FILE_MAP_READ,
                                0,
                                0,
                                0
                                );
        }

        if ( ( ( pbKey == NULL ) && ( cbKeyFileSize != 0 ) ) ||
             ( ( pbData == NULL ) && ( cbDataFileSize != 0 ) ) )
        {
            fResult = FALSE;
        }
    }

    if ( fResult == TRUE )
    {
        pKey->cbData = cbKeyFileSize;
        pKey->pbData = pbKey;
        *ppwszKeyPath = pwszKeyDbPath;

        pData->cbData = cbDataFileSize;
        pData->pbData = pbData;
        *ppwszDataPath = pwszDataDbPath;
    }
    else
    {
        delete pwszKeyDbPath;
        delete pwszDataDbPath;
    }

    if ( hKeyFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hKeyFile );
    }

    if ( hDataFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hDataFile );
    }

    if ( hMappedKeyFile != NULL )
    {
        CloseHandle( hMappedKeyFile );
    }

    if ( hMappedDataFile != NULL )
    {
        CloseHandle( hMappedDataFile );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CHashDbCompactor::UnmapDatabase, public
//
//  Synopsis:   unmap the database
//
//----------------------------------------------------------------------------
VOID
CHashDbCompactor::UnmapDatabase (
                       IN PCRYPT_DATA_BLOB pKey,
                       IN PCRYPT_DATA_BLOB pData
                       )
{
    FlushCompactionAnalysis();

    if ( pKey->pbData != NULL )
    {
        UnmapViewOfFile( pKey->pbData );
    }

    if ( pData->pbData != NULL )
    {
        UnmapViewOfFile( pData->pbData );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CHashDbCompactor::AnalyzeDataForCompaction, public
//
//  Synopsis:   analyze the database data for compaction
//
//----------------------------------------------------------------------------
BOOL
CHashDbCompactor::AnalyzeDataForCompaction (
                         IN PCRYPT_DATA_BLOB pData,
                         IN OPTIONAL LPCSTR pszUnwantedCatalog
                         )
{
    BOOL            fResult = TRUE;
    HashMastRec*    pHashMastRecord;
    DWORD           cbToNextRecord;
    DWORD           cRecord;
    DWORD           cCount;
    PUNIQUE_CATALOG pUniqueCatalog;

    if ( m_cUniqueCatalogs > 0 )
    {
        if ( FlushCompactionAnalysis() == FALSE )
        {
            return( FALSE );
        }
    }

    pHashMastRecord = (HashMastRec *)(
                           pData->pbData + BFILE_HEADERSIZE + sizeof( DWORD )
                           );

    cbToNextRecord = sizeof( HashMastRec ) + sizeof( DWORD );

    if ( pData->cbData < BFILE_HEADERSIZE )
    {
        cRecord = 0;
    }
    else
    {
        cRecord = ( pData->cbData - BFILE_HEADERSIZE ) / cbToNextRecord;
    }

    for ( cCount = 0; ( fResult == TRUE ) && ( cCount < cRecord ); cCount++ )
    {
        if ( ( pszUnwantedCatalog == NULL ) ||
             ( _strnicmp(
                   pHashMastRecord->CatName,
                   pszUnwantedCatalog,
                   MAX_PATH
                   ) != 0 ) )
        {
            pUniqueCatalog = FindUniqueCatalogByName(
                                 pHashMastRecord->CatName
                                 );

            if ( ( pUniqueCatalog == NULL ) &&
                 ( CatalogFileExists(
                          pHashMastRecord->CatName,
                          MAX_PATH
                          ) == TRUE ) )
            {
                fResult = AddUniqueCatalog( pHashMastRecord );
            }
        }

        pHashMastRecord = (HashMastRec *)(
                               (LPBYTE)pHashMastRecord + cbToNextRecord
                               );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CHashDbCompactor::FlushCompactionAnalysis, public
//
//  Synopsis:   flush the last compaction analysis done
//
//----------------------------------------------------------------------------
BOOL
CHashDbCompactor::FlushCompactionAnalysis ()
{
    DWORD           cAllocatedUniqueCatalogs;
    DWORD           cUniqueCatalogs;
    PUNIQUE_CATALOG aUniqueCatalogs;

    if ( m_cAllocatedUniqueCatalogs == INITIAL_UNIQUE_CATALOGS )
    {
        m_cUniqueCatalogs = 0;
        m_iLastUniqueCatalogFoundByName = 0;
        return( TRUE );
    }

    aUniqueCatalogs = m_aUniqueCatalogs;
    cUniqueCatalogs = m_cUniqueCatalogs;
    cAllocatedUniqueCatalogs = m_cAllocatedUniqueCatalogs;

    m_aUniqueCatalogs = NULL;
    m_cUniqueCatalogs = 0;
    m_cAllocatedUniqueCatalogs = 0;

    if ( GrowUniqueCatalogs( INITIAL_UNIQUE_CATALOGS ) == FALSE )
    {
        m_aUniqueCatalogs = aUniqueCatalogs;
        m_cUniqueCatalogs = cUniqueCatalogs;
        m_cAllocatedUniqueCatalogs = cAllocatedUniqueCatalogs;

        return( FALSE );
    }

    delete aUniqueCatalogs;

    m_iLastUniqueCatalogFoundByName = 0;

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CHashDbCompactor::WriteCompactedDatabase, public
//
//  Synopsis:   write the compacted database
//
//----------------------------------------------------------------------------
BOOL
CHashDbCompactor::WriteCompactedDatabase (
                       IN PCRYPT_DATA_BLOB pKey,
                       IN PCRYPT_DATA_BLOB pData,
                       IN OPTIONAL LPCSTR pszUnwantedCatalog
                       )
{
    BOOL            fResult = FALSE;
    HANDLE          hFile;
    HANDLE          hDataFile;
    HANDLE          hMap;
    DWORD           cCount;
    DWORD           cbSize;
    LPBYTE          pbDataFile = NULL;
    LPBYTE          pbFile = NULL;
    HashMastRec*    pHashMastRecord;
    PUNIQUE_CATALOG pUniqueCatalog;
    LPDWORD         pdw;
    DWORD           RecordId;
    DWORD           cKey;
    LPBYTE          pbKey;
    LPBYTE          pb;
    BFILE_HEADER*   pHeader = NULL;

    hDataFile = CreateFileU(
                      m_pwszTempDataPath,
                      GENERIC_READ | GENERIC_WRITE,
                      0,
                      NULL,
                      CREATE_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL
                      );

    if ( hDataFile == INVALID_HANDLE_VALUE )
    {
        return( FALSE );
    }

    cbSize = ( sizeof( DWORD ) + sizeof( HashMastRec ) ) * m_cUniqueCatalogs;
    cbSize += BFILE_HEADERSIZE;

    if ( SetFilePointer( hDataFile, cbSize, NULL, FILE_BEGIN ) != 0xFFFFFFFF )
    {
        fResult = SetEndOfFile( hDataFile );
    }

    if ( fResult == TRUE )
    {
        if ( ( hMap = CreateFileMapping(
                            hDataFile,
                            NULL,
                            PAGE_READWRITE,
                            0,
                            0,
                            NULL
                            ) ) != NULL )
        {
            pbDataFile = (LPBYTE)MapViewOfFile( hMap, FILE_MAP_WRITE, 0, 0, 0 );
            CloseHandle( hMap );
        }

        if ( pbDataFile != NULL )
        {
            memcpy( pbDataFile, BFILE_SIG, BFILE_SIZEOFSIG );

            pHeader = (BFILE_HEADER *)( pbDataFile + BFILE_SIZEOFSIG );

            memset( pHeader, 0, sizeof( BFILE_HEADER ) );

            pHeader->sVersion = BFILE_VERSION_1;
            pHeader->sIntVersion = CATDB_VERSION_1;
            pHeader->cbKey = KEY_SIZE;
            pHeader->cbData = sizeof( HashMastRec );
        }
        else
        {
            fResult = FALSE;
        }
    }

    pdw = (LPDWORD)( pbDataFile + BFILE_HEADERSIZE );
    pHashMastRecord = (HashMastRec *)( (LPBYTE)pdw + sizeof( DWORD ) );

    for ( cCount = 0;
          ( cCount < m_cUniqueCatalogs ) && ( fResult == TRUE );
          cCount++ )
    {
        RecordId = cCount + 1;

        memcpy( pdw, &RecordId, sizeof( DWORD ) );

        memcpy(
           pHashMastRecord,
           &m_aUniqueCatalogs[ cCount ].HashDbRecord,
           sizeof( HashMastRec )
           );

        pdw = (LPDWORD)(
                 (LPBYTE)pdw + sizeof( HashMastRec ) + sizeof( DWORD )
                 );

        pHashMastRecord = (HashMastRec *)( (LPBYTE)pdw + sizeof( DWORD ) );
    }

    if ( fResult == FALSE )
    {
        if ( pbDataFile != NULL )
        {
            UnmapViewOfFile( pbDataFile );
        }

        CloseHandle( hDataFile );

        return( FALSE );
    }

    fResult = FALSE;
    pbFile = NULL;

    hFile = CreateFileU(
                  m_pwszTempKeyPath,
                  GENERIC_READ | GENERIC_WRITE,
                  0,
                  NULL,
                  CREATE_ALWAYS,
                  FILE_ATTRIBUTE_NORMAL,
                  NULL
                  );

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        UnmapViewOfFile( pbDataFile );
        CloseHandle( hDataFile );
        return( FALSE );
    }

    if ( SetFilePointer( hFile, pKey->cbData, NULL, FILE_BEGIN ) != 0xFFFFFFFF )
    {
        fResult = SetEndOfFile( hFile );
    }

    if ( ( fResult == TRUE ) && ( pKey->cbData > 0 ) )
    {
        if ( ( hMap = CreateFileMapping(
                            hFile,
                            NULL,
                            PAGE_READWRITE,
                            0,
                            0,
                            NULL
                            ) ) != NULL )
        {
            pbFile = (LPBYTE)MapViewOfFile( hMap, FILE_MAP_WRITE, 0, 0, 0 );
            CloseHandle( hMap );
        }

        if ( pbFile == NULL )
        {
            fResult = FALSE;
        }
    }

    cKey = pKey->cbData / KEY_RECORD_SIZE;
    pdw = (LPDWORD)( pKey->pbData + KEY_SIZE );
    pbKey = pKey->pbData;
    pb = pbFile;
    RecordId = 0;

    __try
    {
        for ( cCount = 0; ( cCount < cKey ) && ( fResult == TRUE ); cCount++ )
        {
            if ( ( *pdw + sizeof( HashMastRec ) + sizeof( DWORD ) ) <= pData->cbData )
            {
                pHashMastRecord = (HashMastRec *)(
                                       pData->pbData + *pdw + sizeof( DWORD )
                                       );

                if ( ( pszUnwantedCatalog == NULL ) ||
                     ( _strnicmp(
                           pHashMastRecord->CatName,
                           pszUnwantedCatalog,
                           MAX_PATH
                           ) != 0 ) )
                {
                    pUniqueCatalog = FindUniqueCatalogByName(
                                         pHashMastRecord->CatName
                                         );

                    if ( pUniqueCatalog == NULL )
                    {
                        pdw = (LPDWORD)( (LPBYTE)pdw + KEY_RECORD_SIZE );
                        pbKey += KEY_RECORD_SIZE;
                        continue;
                    }

                    memcpy(
                       pb,
                       pbKey,
                       KEY_SIZE
                       );

                    pb += KEY_SIZE;

                    memcpy(
                       pb,
                       (LPBYTE)&pUniqueCatalog->UniqueOffset,
                       sizeof( DWORD )
                       );

                    pb += sizeof( DWORD );

                    RecordId += 1;
                }
            }

            pdw = (LPDWORD)( (LPBYTE)pdw + KEY_RECORD_SIZE );
            pbKey += KEY_RECORD_SIZE;
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError( GetExceptionCode() );
        fResult = FALSE;
    }

    if ( pbFile != NULL )
    {
        UnmapViewOfFile( pbFile );
    }

    if ( fResult == TRUE )
    {
        cbSize = RecordId * KEY_RECORD_SIZE;

        pHeader->cbSortedEOF = cbSize;
        pHeader->dwLastRecNum = ( RecordId > 0 ) ? ( RecordId - 1 ) : 0;
        pHeader->fDirty = FALSE;

        if ( SetFilePointer( hFile, cbSize, NULL, FILE_BEGIN ) != 0xFFFFFFFF )
        {
            fResult = SetEndOfFile( hFile );
        }
    }

    UnmapViewOfFile( pbDataFile );
    CloseHandle( hDataFile );
    CloseHandle( hFile );

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CHashDbCompactor::CommitCompactedDatabase, public
//
//  Synopsis:   commit the compacted database
//
//----------------------------------------------------------------------------
BOOL
CHashDbCompactor::CommitCompactedDatabase (
                        IN LPCWSTR pwszFinalKeyPath,
                        IN LPCWSTR pwszFinalDataPath
                        )
{
    if ( MoveFileExU(
             m_pwszTempKeyPath,
             pwszFinalKeyPath,
             MOVEFILE_COPY_ALLOWED |
             MOVEFILE_REPLACE_EXISTING |
             MOVEFILE_WRITE_THROUGH
             ) == FALSE )
    {
        return( FALSE );
    }

    return( MoveFileExU(
                m_pwszTempDataPath,
                pwszFinalDataPath,
                MOVEFILE_COPY_ALLOWED |
                MOVEFILE_REPLACE_EXISTING |
                MOVEFILE_WRITE_THROUGH
                ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CHashDbCompactor::FreeString, public
//
//  Synopsis:   free allocated string
//
//----------------------------------------------------------------------------
VOID
CHashDbCompactor::FreeString (IN LPWSTR pwsz)
{
    delete pwsz;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHashDbCompactor::GrowUniqueCatalogs, public
//
//  Synopsis:   grow the unique catalogs array
//
//----------------------------------------------------------------------------
BOOL
CHashDbCompactor::GrowUniqueCatalogs (DWORD cGrow)
{
    BOOL            fResult = FALSE;
    DWORD           cAllocatedUniqueCatalogs;
    PUNIQUE_CATALOG aUniqueCatalogs;

    cAllocatedUniqueCatalogs = m_cAllocatedUniqueCatalogs + cGrow;
    aUniqueCatalogs = new UNIQUE_CATALOG [ cAllocatedUniqueCatalogs ];

    if ( aUniqueCatalogs == NULL )
    {
        SetLastError( E_OUTOFMEMORY );
        return( FALSE );
    }

    memset(
       aUniqueCatalogs,
       0,
       sizeof( UNIQUE_CATALOG ) * cAllocatedUniqueCatalogs
       );

    if ( m_aUniqueCatalogs != NULL )
    {
        memcpy(
           aUniqueCatalogs,
           m_aUniqueCatalogs,
           m_cUniqueCatalogs * sizeof( UNIQUE_CATALOG )
           );

        delete m_aUniqueCatalogs;
    }

    m_cAllocatedUniqueCatalogs = cAllocatedUniqueCatalogs;
    m_aUniqueCatalogs = aUniqueCatalogs;

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CHashDbCompactor::FindUniqueCatalogByName, public
//
//  Synopsis:   find a unique catalog entry given a catalog name
//
//----------------------------------------------------------------------------
PUNIQUE_CATALOG
CHashDbCompactor::FindUniqueCatalogByName (LPCSTR pszCatalogName)
{
    DWORD cCount;

    if ( ( m_iLastUniqueCatalogFoundByName < m_cUniqueCatalogs ) &&
         ( _strnicmp(
               m_aUniqueCatalogs[ m_iLastUniqueCatalogFoundByName ].HashDbRecord.CatName,
               pszCatalogName,
               MAX_PATH
               ) == 0 ) )
    {
        return( &m_aUniqueCatalogs[ m_iLastUniqueCatalogFoundByName ] );
    }

    for ( cCount = 0; cCount < m_cUniqueCatalogs; cCount++ )
    {
        if ( _strnicmp(
                 m_aUniqueCatalogs[ cCount ].HashDbRecord.CatName,
                 pszCatalogName,
                 MAX_PATH
                 ) == 0 )
        {
            m_iLastUniqueCatalogFoundByName = cCount;
            return( &m_aUniqueCatalogs[ cCount ] );
        }
    }

    return( NULL );
}

//+---------------------------------------------------------------------------
//
//  Member:     CHashDbCompactor::AddUniqueCatalog, public
//
//  Synopsis:   add a unique catalog to the array
//
//----------------------------------------------------------------------------
BOOL
CHashDbCompactor::AddUniqueCatalog (HashMastRec* pHashMastRecord)
{
    DWORD UniqueOffset;

    if ( m_cUniqueCatalogs == m_cAllocatedUniqueCatalogs )
    {
        if ( GrowUniqueCatalogs( GROW_UNIQUE_CATALOGS ) == FALSE )
        {
            return( FALSE );
        }
    }

    UniqueOffset = ( m_cUniqueCatalogs * (
                              sizeof( HashMastRec ) + sizeof( DWORD )
                              ) ) + BFILE_HEADERSIZE;

    m_aUniqueCatalogs[ m_cUniqueCatalogs ].HashDbRecord = *pHashMastRecord;
    m_aUniqueCatalogs[ m_cUniqueCatalogs ].UniqueOffset = UniqueOffset;

    m_cUniqueCatalogs += 1;

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CHashDbCompactor::CatalogFileExists, public
//
//  Synopsis:   check if the catalog file exists in the database directory
//
//----------------------------------------------------------------------------
BOOL
CHashDbCompactor::CatalogFileExists (LPCSTR pszCatalogName, DWORD cbName)
{
    BOOL   fResult = FALSE;
    WCHAR  pwszFile[ MAX_PATH ];
    WCHAR  pwszPath[ MAX_PATH ];
    HANDLE hFile;

    if ( MultiByteToWideChar(
              CP_ACP,
              0,
              pszCatalogName,
              cbName,
              pwszFile,
              MAX_PATH
              ) == 0 )
    {
        return( FALSE );
    }

    if ( ( wcslen( m_pwszDbDirectory ) + wcslen( pwszFile ) + 2 ) > MAX_PATH )
    {
        return( FALSE );
    }

    wcscpy( pwszPath, m_pwszDbDirectory );
    wcscat( pwszPath, L"\\" );
    wcscat( pwszPath, pwszFile );

    if ( ( hFile = CreateFileU(
                         pwszPath,
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL
                         ) ) != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
        fResult = TRUE;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   CatalogCompactHashDatabase
//
//  Synopsis:   API for compacting the catalog hash database
//
//----------------------------------------------------------------------------
BOOL WINAPI
CatalogCompactHashDatabase (
       IN LPCWSTR pwszDbLock,
       IN LPCWSTR pwszDbDirectory,
       IN LPCWSTR pwszDbName,
       IN OPTIONAL LPCWSTR pwszUnwantedCatalog
       )
{
    BOOL             fResult;
    CHashDbCompactor HashDbCompactor;
    LPWSTR           pwszKeyPath = NULL;
    LPWSTR           pwszDataPath = NULL;
    CRYPT_DATA_BLOB  KeyMap;
    CRYPT_DATA_BLOB  DataMap;
    BOOL             fDatabaseMapped = FALSE;
    CHAR             szUnwantedCatalog[ MAX_PATH + 1 ];
    LPSTR            pszUnwantedCatalog = NULL;

    if ( pwszUnwantedCatalog != NULL )
    {
        pszUnwantedCatalog = szUnwantedCatalog;

        if ( WideCharToMultiByte(
                 CP_ACP,
                 0,
                 pwszUnwantedCatalog,
                 -1,
                 pszUnwantedCatalog,
                 MAX_PATH,
                 NULL,
                 NULL
                 ) == FALSE )
        {
            return( FALSE );
        }
    }

    fResult = HashDbCompactor.Initialize( pwszDbLock, pwszDbDirectory );
    if ( fResult == FALSE )
    {
        return( FALSE );
    }

    HashDbCompactor.LockDatabase();

    __try
    {
        fResult = HashDbCompactor.MapDatabase(
                                     pwszDbName,
                                     &KeyMap,
                                     &pwszKeyPath,
                                     &DataMap,
                                     &pwszDataPath
                                     );

        if ( fResult == TRUE )
        {
            fDatabaseMapped = TRUE;

            fResult = HashDbCompactor.AnalyzeDataForCompaction(
                                             &DataMap,
                                             pszUnwantedCatalog
                                             );
        }

        if ( fResult == TRUE )
        {
            fResult = HashDbCompactor.WriteCompactedDatabase(
                                           &KeyMap,
                                           &DataMap,
                                           pszUnwantedCatalog
                                           );
        }

        if ( fDatabaseMapped == TRUE )
        {
            HashDbCompactor.UnmapDatabase( &KeyMap, &DataMap );
        }

        if ( fResult == TRUE )
        {
            fResult = HashDbCompactor.CommitCompactedDatabase(
                                            pwszKeyPath,
                                            pwszDataPath
                                            );
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError( GetExceptionCode() );
        fResult = FALSE;
    }

    HashDbCompactor.UnlockDatabase();

    if ( pwszKeyPath != NULL )
    {
        HashDbCompactor.FreeString( pwszKeyPath );
    }

    if ( pwszDataPath != NULL )
    {
        HashDbCompactor.FreeString( pwszDataPath );
    }

    return( fResult );
}


