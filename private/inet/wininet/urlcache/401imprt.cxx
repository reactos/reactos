#include <cache.hxx>

#include "401imprt.hxx"
#include "401inc.hxx"

#ifdef UNICODE
#error "401imprt.cxx doesn't support UNICODE compilation."
#endif

#define PRE5_CACHE_KEY  "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Cache"
#define IE401_HIST_ROOT "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Cache\\Extensible Cache"

//IE401_PER_USER_CACHE_LOCATION is in HKCU
#define IE401_PER_USER_CACHE_LOCATION \
        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"
#define IE401_PER_USER_CACHE_KEY "Cache"
//IE401_ONE_USER_CACHE_LOCATION is in HKLM
#define IE401_ONE_USER_CACHE_LOCATION \
        "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Cache\\Content"
#define IE401_ONE_USER_CACHE_KEY "CachePath"

#define MSHIST_DIR_SZ       "MSHIST011998020119980225"##FILENAME_SEPARATOR_STR
#define MSHIST_KEY_SZ       "MSHIST011998020119980225"
#define MSHIST_SZ           "MSHIST"
#define MSHIST_PREFIX_SZ    ":1998010119980101:"
#define VISITED_PREFIX_SZ   "Visited:"
#define INDEX_DAT_SZ        "index.dat"
#define CACHE_LIMIT_SZ      "CacheLimit"
#define CACHE_OPTIONS_SZ    "CacheOptions"
#define CACHE_PATH_SZ       "CachePath"
#define CACHE_PREFIX_SZ     "CachePrefix"

//----------------------------------------
//The following tructures and macro, _HISTDATA_V001 and SET_HISTORY_FLAG()
//allow us to notify members of the Visited: cache that they are to be
//listed in the History view.  The structure was cut'n'pasted from
//shdocvw\urlhist.cpp and any changes made to the original need
//to be reflected here as well.
#define PIDISF_HISTORY 0x10000000
#define PID_INTSITE_TITLE         16
//** BUGBUG

struct _HISTDATA_V001
{
    UINT  cbSize : 16;           // size of this header
    UINT  cbVer  : 16;           // version
    DWORD           dwFlags;    // PID_INTSITE_FLAGS (PIDISF_ flags)
    DWORD           dwWatch;    // PID_INTSITE_WATCH (PIDISM_ flags)
    DWORD           dwVisits;   // PID_INTSITE_VISITCOUNT
};

#define SET_HISTORY_FLAG(lpHeaderInfo) \
            (((_HISTDATA_V001*)lpHeaderInfo)->dwFlags |= PIDISF_HISTORY)

//
//  Right after HISTDATA (always at cbSize), we have optional (typically
// variable length) data which has following data structure. It may have
// more than one but always has a null-terimiator (cbExtra == 0).
//
//HISTEXTRA is also cut'n'pasted from shdocvw.
struct HISTEXTRA
{
    UINT cbExtra : 16;
    UINT idExtra : 8;   // PID_INTSITE_*
    UINT vtExtra : 8;   // VT_*
    BYTE abExtra[1];    // abExtra[cbExtra-4];
};

// HISTEXTRA without the abExtra  ==> (sizeof(HISTEXTRA) - sizeof(BYTE))
#define HISTEXTRA_HEAD_SIZE 4
//-----------------------------------------


BOOL IsPerUserCache();

namespace ie401
{


//--------------------------------------------------------

//  The code that enumerates through a IE401 index.dat file is provided
//by IE401IndexFile.  Classes derived from IE401IndexFile override HandleHashElement
//to handle each HASH_ELEMENT as the index file is enumerated.
//  IE401IndexFile::Import401Url is provided to import URLs without overwriting
//already existing entries.

class IE401IndexFile
{
public:
    //  EnumHashValues enumerates hash tables, calling HandleHashElement for each.
    virtual BOOL EnumHashValues();

protected:
    IE401IndexFile( LPCSTR szFilename);
    IE401IndexFile();
    virtual ~IE401IndexFile();

    //  given by derived class
    virtual BOOL HandleHashElement( ie401::HASH_ITEM* pEntry) = 0;

    //  probably useful to many, default URL import.  Not for REDIRs.
    virtual BOOL Import401Url( ie401::URL_FILEMAP_ENTRY* pEntry);

    BYTE* m_pBuf;
};


//--------------------------------------------------------

//  IE401Visited overrides HandleHashElement to import all URLs,
//translating CEI to its current format.
//  All the dependencies on the change in CEI format are
//contained in UpgradeHeaderData.

class IE401Visited : public IE401IndexFile
{
public:
    IE401Visited( LPCSTR szFilename);

protected:
    IE401Visited() {}

    virtual BOOL HandleHashElement( ie401::HASH_ITEM* pEntry);

    virtual BOOL Import401UrlTranslatingCEI( ie401::URL_FILEMAP_ENTRY* pEntry);

    virtual BOOL UpgradeHeaderData(
            IN      const CHAR* pIn,
            OUT     CHAR* pOut,
            IN OUT  DWORD* pcbOutSize);
};


//--------------------------------------------------------

//  IE401History overrides HandleHashElement to import all URLs
//and mark them in the Visited: container.
//  All the dependencies on the format for the Visited mark
//are contained in MarkUrlAsVisited.
//(colliding items are not imported and associated data
//files are not copied.)

class IE401History : public IE401IndexFile
{
public:
    IE401History( LPCSTR szFilename);

protected:
    IE401History() {}

    virtual BOOL HandleHashElement( ie401::HASH_ITEM* pEntry);

    static BOOL MarkUrlAsVisited( LPSTR szUrlName);
};


//--------------------------------------------------------


// pre-declaration included so IE401Redirects can be declared as a friend
class IE401Redirects;


//  IE401Content overrides HandleHashElement to import all URLs
//and also copy associated data files.
//(colliding items are not imported)
class IE401Content : public IE401IndexFile
{
public:
    IE401Content( LPCSTR szFilename);

    friend IE401Redirects;

protected:
    IE401Content() {}

    virtual BOOL HandleHashElement( ie401::HASH_ITEM* pEntry);

    //  Extends the default to import files
    BOOL Import401UrlWithFile( ie401::URL_FILEMAP_ENTRY* pEntry);

    CHAR  m_szRootPath[MAX_PATH];
    DWORD m_cbRootPathLength;
    DWORD m_nDirs;
    CHAR  m_szDir[DEFAULT_MAX_DIRS][DIR_NAME_SIZE + 1];
};


//--------------------------------------------------------


//  IE401Redirects override HandleHashElement to import redirects.
//  This should be done to an index file after IE401Content has
//enumerated over it, so the constructor takes a IE401Content object
//rather than a filename.  This IE401Content represents a process
//that is finished, so its functionality is taken away.
//  Importing redirects is done with a separate enumerator than
//IE401Content since its less work then retooling IE401IndexFile
//to enumerate an arbitrary number of times.
class IE401Redirects : public IE401IndexFile
{
public:
    IE401Redirects( IE401Content* pContent);

protected:
    IE401Redirects() {}

    virtual BOOL HandleHashElement( ie401::HASH_ITEM* pEntry);

    BOOL Import401Redirect( ie401::REDIR_FILEMAP_ENTRY* pEntry);
};


//******************************************************
//
// class OutputStream  - utility
//
//  outputs data to a buffer, tracking buffer used
//and checking for overflow.


class OutputStream
{
public:
    OutputStream( VOID* pBuffer, DWORD cbBufferSize)
    : m_pBuffer( (BYTE*)pBuffer), m_cbBufferSize( cbBufferSize), m_cbPosition(0)
    {
    }

    BOOL Memcpy( const VOID* pSource, DWORD cbSize)
    {
        if( cbSize + m_cbPosition <= m_cbBufferSize)
        {
            memcpy(&m_pBuffer[m_cbPosition], (BYTE*)pSource, cbSize);
            m_cbPosition += cbSize;
            return TRUE;
        }
        else
            return FALSE;
    }

    BOOL CopyAnsiToUnicode( const CHAR* pSource, DWORD cSize)
    {
        if( m_cbPosition + cSize * sizeof(WCHAR) / sizeof(CHAR)
            <= m_cbBufferSize)
        {
            DWORD dwSizeCopied;

            // the semantics of MultiByteToWideChar is different
            //if you give it a zero-length buffer.
            INET_ASSERT( m_cbBufferSize - m_cbPosition != 0);

            dwSizeCopied =
                MultiByteToWideChar( CP_ACP, 0, pSource, cSize,
                    (WCHAR*)&m_pBuffer[m_cbPosition],
                    (m_cbBufferSize - m_cbPosition) / sizeof(WCHAR));

            if( dwSizeCopied != 0)
            {
                m_cbPosition += dwSizeCopied * sizeof(WCHAR);
                return TRUE;
            }
            else
                return FALSE;
        }
        else
            return FALSE;
    }

    DWORD GetFinalLength()
    {
        return m_cbPosition;
    }

private:
    BYTE* m_pBuffer;

    DWORD m_cbBufferSize, m_cbPosition;
};


//*************************************************************************
//
//  IE401IndexFile
//

//  On creation, load the contents of the given file into memory.

IE401IndexFile::IE401IndexFile( LPCSTR szFilename)
{
    m_pBuf = NULL;

    //  load the file into the buffer
    DWORD cbBufSize;
    if( ReadFileToBuffer( szFilename, &m_pBuf, &cbBufSize) == FALSE)
    {
        if( m_pBuf != NULL)
        {
            delete [] m_pBuf;
            m_pBuf = NULL;
        }
    }
    else if( cbBufSize < sizeof(_MEMMAP_HEADER_SMALL)
        || strcmp((LPSTR) m_pBuf, "Client UrlCache MMF Ver 4.7"))
    {
        //  If this file doesn't even have a memmap header, forget it.
        //  Now all derived classes can assume they have at least a memmap header
        //if m_pBuf != NULL
        delete [] m_pBuf;
        m_pBuf = NULL;
    }
}


//  Default constructor made protected to prevent direct creation
IE401IndexFile::IE401IndexFile()
: m_pBuf(NULL)
{
}


IE401IndexFile::~IE401IndexFile()
{
    if( m_pBuf != NULL)
        delete [] m_pBuf;
}


//---------------------------------------------------
//

//  Enumerate through the entries in an ie401 index
//file, calling HandleHashElement on each entry.

BOOL IE401IndexFile::EnumHashValues()
{
    BOOL retVal = FALSE;

    if( m_pBuf == NULL)
        goto doneEnumHashValues;

    HASH_FILEMAP_ENTRY* pTable;
    HASH_ITEM* pItem;
    //  pTable is located by an offset which is located at dwHashTableOffset
    pTable = (HASH_FILEMAP_ENTRY*)(m_pBuf + (((_MEMMAP_HEADER_SMALL*)m_pBuf)->dwHashTableOffset));
    // The first location in the table follows immediately after the HASH_FILEMAP_ENTRY pTable
    pItem = (HASH_ITEM*)(pTable + 1);

    if (pTable->dwSig != SIG_HASH)
        goto doneEnumHashValues;

    do // Scan the list of tables.
    {
        // Scan the current table.
        for (; (LPBYTE)pItem < (LPBYTE)pTable + BYTES_PER_TABLE; pItem++)
        {
            //  call virtual entry handler
            if( HandleHashElement( pItem) == FALSE)
                goto doneEnumHashValues;
        }

        // Follow the link to the next table.
        if (!pTable->dwNext)
        {
            pTable = NULL;
        }
        else
        {
            // Validate the table signature and sequence number.
            DWORD nBlock;
            nBlock = pTable->nBlock;
            pTable = (HASH_FILEMAP_ENTRY*) (m_pBuf + pTable->dwNext);
            if (pTable->dwSig != SIG_HASH || pTable->nBlock != nBlock + 1)
                goto doneEnumHashValues;

            // Set pointer to first location in table.
            pItem = (HASH_ITEM*) (pTable + 1);
        }
    }
    while (pTable);

    retVal = TRUE;

doneEnumHashValues:
    return retVal;
}


//---------------------------------------------------
//


//  Imports an URL entry without overwriting existing
//cache entries or copying any associated data files.

BOOL IE401IndexFile::Import401Url( ie401::URL_FILEMAP_ENTRY* pEntry)
{
    BOOL retVal = FALSE;

    //  don't import Url if it is already in the buffer
    if( GetUrlCacheEntryInfo( (LPSTR)((BYTE*)pEntry + pEntry->UrlNameOffset), NULL, 0) == TRUE
        || GetLastError() != ERROR_FILE_NOT_FOUND)
    {
        goto doneImport401Url;
    }

    if( pEntry->FileSize != 0)
    {
        INET_ASSERT(0);  // Are you importing URL cache entries with external data?
        goto doneImport401Url;
    }

    if( !CommitUrlCacheEntry(
            (LPCSTR)((BYTE*)pEntry + pEntry->UrlNameOffset),
            NULL,
            *LONGLONG_TO_FILETIME(&pEntry->ExpireTime),
            *LONGLONG_TO_FILETIME(&pEntry->LastModifiedTime),
            pEntry->CacheEntryType,
            pEntry->HeaderInfoOffset != NULL ?
                (BYTE*)((BYTE*)pEntry + pEntry->HeaderInfoOffset) : NULL,
            pEntry->HeaderInfoSize,
            pEntry->FileExtensionOffset != 0 ?
                (LPCSTR)((BYTE*)pEntry + pEntry->FileExtensionOffset) : NULL,
            NULL))
        goto doneImport401Url;

    CACHE_ENTRY_INFO cei;

    cei.dwStructSize = sizeof(cei);
    cei.LastAccessTime = *LONGLONG_TO_FILETIME(&pEntry->LastAccessedTime);
    cei.dwHitRate = pEntry->NumAccessed;
    cei.ExpireTime = *LONGLONG_TO_FILETIME(&pEntry->ExpireTime);
    cei.LastModifiedTime = *LONGLONG_TO_FILETIME(&pEntry->LastModifiedTime);

    if( !SetUrlCacheEntryInfo(
            (LPCSTR)((BYTE*)pEntry + pEntry->UrlNameOffset),
            &cei,
            CACHE_ENTRY_ACCTIME_FC | CACHE_ENTRY_HITRATE_FC
            | CACHE_ENTRY_EXPTIME_FC | CACHE_ENTRY_MODTIME_FC))
       goto doneImport401Url;

    retVal = TRUE;

doneImport401Url:
    return retVal;
}


//************************************************************************8
//
//  IE401Visited : public IE401IndexFile
//

IE401Visited::IE401Visited( LPCSTR szFilename) : IE401IndexFile( szFilename)
{
}


//  imports only URLs using Import401Url- nothing special
BOOL IE401Visited::HandleHashElement( ie401::HASH_ITEM* pEntry)
{
    // No reserved bits should be set.
    INET_ASSERT (!(pEntry->dwHash & HASH_BIT_RESERVED));

    if( !(pEntry->dwHash & HASH_BIT_NOTURL)
        && ((ie401::FILEMAP_ENTRY*)(m_pBuf + pEntry->dwOffset))->dwSig == SIG_URL)
    {
        Import401UrlTranslatingCEI( (ie401::URL_FILEMAP_ENTRY*)(m_pBuf + pEntry->dwOffset));
    }

    // after wrapping above in try-catch block, return FALSE on unhandled exception.
    return TRUE;
}


//----------------------------------------
//
//  IE401Visited::Import401UrlTranslatingCEI
//
//  Very much like Import401Url except it makes
//a call to UpgradeHeaderData before calling CommitCacheEntry.

BOOL IE401Visited::Import401UrlTranslatingCEI( ie401::URL_FILEMAP_ENTRY* pEntry)
{
    BOOL retVal = FALSE;

    //  don't import Url if it is already in the buffer
    if( GetUrlCacheEntryInfo( (LPSTR)((BYTE*)pEntry + pEntry->UrlNameOffset), NULL, 0) == TRUE
        || GetLastError() != ERROR_FILE_NOT_FOUND)
    {
        goto doneImport401UrlTranslatingCEI;
    }

    if( pEntry->FileSize != 0)
    {
        INET_ASSERT(0);  // Are you importing URL cache entries with external data?
        goto doneImport401UrlTranslatingCEI;
    }

    DWORD cbHeaderBufSize;
    //BUGBUG:  Does shdocvw still obey MAX_CACHE_ENTRY_INFO_SIZE
    //in the version being imported?
    CHAR szHeaderBuf[ MAX_CACHE_ENTRY_INFO_SIZE];
    cbHeaderBufSize = MAX_CACHE_ENTRY_INFO_SIZE;

    if( pEntry->HeaderInfoOffset != 0)
    {
        if( UpgradeHeaderData(  (CHAR*)((BYTE*)pEntry + pEntry->HeaderInfoOffset),
                                szHeaderBuf, &cbHeaderBufSize) != TRUE)
        {
            goto doneImport401UrlTranslatingCEI;
        }
    }
    else
        cbHeaderBufSize = 0;

    if( !CommitUrlCacheEntry(
            (LPCSTR)((BYTE*)pEntry + pEntry->UrlNameOffset),
            NULL,
            *LONGLONG_TO_FILETIME(&pEntry->ExpireTime),
            *LONGLONG_TO_FILETIME(&pEntry->LastModifiedTime),
            pEntry->CacheEntryType,
            cbHeaderBufSize != 0 ?
                (BYTE*)szHeaderBuf : NULL,
            cbHeaderBufSize,
            pEntry->FileExtensionOffset != 0 ?
                ((CHAR*)pEntry + pEntry->FileExtensionOffset) : NULL,
            NULL))
        goto doneImport401UrlTranslatingCEI;

    CACHE_ENTRY_INFO cei;

    cei.dwStructSize = sizeof(cei);
    cei.LastAccessTime = *LONGLONG_TO_FILETIME(&pEntry->LastAccessedTime);
    cei.dwHitRate = pEntry->NumAccessed;
    cei.ExpireTime = *LONGLONG_TO_FILETIME(&pEntry->ExpireTime);
    cei.LastModifiedTime = *LONGLONG_TO_FILETIME(&pEntry->LastModifiedTime);

    if( !SetUrlCacheEntryInfo(
            (LPCSTR)((BYTE*)pEntry + pEntry->UrlNameOffset),
            &cei,
            CACHE_ENTRY_ACCTIME_FC | CACHE_ENTRY_HITRATE_FC
            | CACHE_ENTRY_EXPTIME_FC | CACHE_ENTRY_MODTIME_FC))
        goto doneImport401UrlTranslatingCEI;

    retVal = TRUE;

doneImport401UrlTranslatingCEI:
    return retVal;

}


BOOL IE401Visited::UpgradeHeaderData(
            IN      const CHAR* pIn,
            OUT     CHAR* pOut,
            IN OUT  DWORD* pcbOutSize)
{
    BOOL retVal = FALSE;

    OutputStream op( pOut, *pcbOutSize);

    //  The header info struct contains a HISTDATA followed by
    //a list of HISTEXTRAs..  Their sizes can vary but they should
    //be adjacent.  The last HISTEXTRA has a cbExtra of 0 and a sizeof(UINT).
    //  When we import a HISTEXTRA (idExtra = PID_INTSITE_TITLE) then
    //we must convert the attached string from ANSI to Unicode
    HISTEXTRA* pExtra = NULL;

    //first copy the HISTDATA
    if( op.Memcpy( pIn, ((_HISTDATA_V001*)pIn)->cbSize) == FALSE)
        goto doneUpgradeCEIData;

    for(pExtra = (HISTEXTRA*) (pIn + ((_HISTDATA_V001*)pIn)->cbSize);
        pExtra->cbExtra != 0;
        pExtra = (HISTEXTRA*)((BYTE*)pExtra + pExtra->cbExtra))
    {
        if( pExtra->idExtra != PID_INTSITE_TITLE)
        {
            if( op.Memcpy( pExtra, pExtra->cbExtra) == FALSE)
                goto doneUpgradeCEIData;
        }
        else
        {
            HISTEXTRA* pNew = (HISTEXTRA*)((BYTE*)pOut + op.GetFinalLength());

            // copy the HISTEXTRA head
            INET_ASSERT( pExtra->cbExtra >= HISTEXTRA_HEAD_SIZE);
            if( op.Memcpy( pExtra, HISTEXTRA_HEAD_SIZE) == FALSE)
                goto doneUpgradeCEIData;

            if( op.CopyAnsiToUnicode( (CHAR*)&(pExtra->abExtra),
                                      pExtra->cbExtra - HISTEXTRA_HEAD_SIZE)
                == FALSE)
                goto doneUpgradeCEIData;

            pNew->vtExtra = VT_LPWSTR;

            // cbExtra(size) is the change in position of the output stream.
            pNew->cbExtra = ((BYTE*)pOut + op.GetFinalLength()) - (BYTE*)pNew;
        }
    }

    // the final member in the list is just a DWORD, value == 0.
    // determined from assertions in Urlhist.cpp:
    //ASSERT( phext->cbExtra == 0); // terminator
    //ASSERT( (LPBYTE)phdNew+cbHeader == (LPBYTE)phext+SIZEOF(DWORD) );
    if( op.Memcpy( pExtra, sizeof(DWORD)) == FALSE)
        goto doneUpgradeCEIData;

    retVal = TRUE;

doneUpgradeCEIData:

    if( retVal == TRUE)
        *pcbOutSize = op.GetFinalLength();

    return retVal;
}


//************************************************************************8
//
//  IE401History : public IE401IndexFile
//

IE401History::IE401History( LPCSTR szFilename) : IE401IndexFile( szFilename)
{
}


//  imports only URLs using Import401Url then marks them
//as Visited
BOOL IE401History::HandleHashElement( ie401::HASH_ITEM* pEntry)
{
    // No reserved bits should be set.
    INET_ASSERT (!(pEntry->dwHash & HASH_BIT_RESERVED));

    if( !(pEntry->dwHash & HASH_BIT_NOTURL)
        && ((ie401::FILEMAP_ENTRY*)(m_pBuf + pEntry->dwOffset))->dwSig == SIG_URL)
    {
        ie401::URL_FILEMAP_ENTRY* pUrlToImport = (ie401::URL_FILEMAP_ENTRY*)(m_pBuf + pEntry->dwOffset);

        if( Import401Url( pUrlToImport) == TRUE)
            MarkUrlAsVisited( (CHAR*)pUrlToImport + pUrlToImport->UrlNameOffset);
    }

    // after wrapping above in try-catch block, return FALSE on unhandled exception.
    return TRUE;
}


//  Marks an Url given with a history prefix in the Visited container
BOOL IE401History::MarkUrlAsVisited( LPSTR szUrlName)
{
    BOOL retVal = FALSE;
    LPCACHE_ENTRY_INFO pCei = NULL;
    DWORD cbCei = 0;

    //I'm changing the string from "MSHIST_PREFIX_SZhttp:\\www.urlname"
    //to "VISITED_PREFIX_SZhttp:\\www.urlname" in order to locate/change
    //the cache entry.
    //This requires backing up the old prefix, and setting a new pointer
    //to the correct location to place the new prefix.  Once the modified
    //szUrlName is used the old prefix is always restored.

    // backup the old prefix
    CHAR szBackup[sizeof(MSHIST_PREFIX_SZ)];
    memcpy( szBackup, szUrlName, sizeof(MSHIST_PREFIX_SZ));

    //  Move the pointer to later in the string so that the new, smaller
    //prefix fits, and put the new prefix there.
    LPSTR szModifiedUrl = szUrlName + sizeof(MSHIST_PREFIX_SZ) - sizeof(VISITED_PREFIX_SZ);
    memcpy( szModifiedUrl, VISITED_PREFIX_SZ, sizeof(VISITED_PREFIX_SZ) - 1);

    //  Get the cei
    if( GetUrlCacheEntryInfo( szModifiedUrl, NULL, &cbCei) == TRUE)
        goto doneMarkUrlAsVisited;

    pCei = (CACHE_ENTRY_INFO*)(new BYTE[cbCei]);
    if( pCei == NULL)
        goto doneMarkUrlAsVisited;

    if( GetUrlCacheEntryInfo( szModifiedUrl, pCei, &cbCei) != TRUE)
        goto doneMarkUrlAsVisited;

    if( pCei->dwHeaderInfoSize < sizeof(_HISTDATA_V001))
        goto doneMarkUrlAsVisited;

    //  set the Visited flag
    SET_HISTORY_FLAG(pCei->lpHeaderInfo);

    CHAR* pStrStr;
    if( pCei->lpHeaderInfo != NULL
        && (pStrStr = StrStr( pCei->lpHeaderInfo, CACHE_CONTROL_SZ)) != NULL
        && (pStrStr = StrStr( pStrStr, MUST_REVALIDATE_SZ)) != NULL)
    {
        pCei->CacheEntryType |= MUST_REVALIDATE_CACHE_ENTRY;
    }

    if( CommitUrlCacheEntry( szModifiedUrl, NULL, pCei->ExpireTime,
          pCei->LastModifiedTime, pCei->CacheEntryType, (BYTE*)pCei->lpHeaderInfo,
          pCei->dwHeaderInfoSize, pCei->lpszFileExtension, NULL) != TRUE)
        goto doneMarkUrlAsVisited;

    if( SetUrlCacheEntryInfo(
            szModifiedUrl,
            pCei,
            CACHE_ENTRY_ACCTIME_FC | CACHE_ENTRY_HITRATE_FC | CACHE_ENTRY_EXPTIME_FC ) != TRUE)
        goto doneMarkUrlAsVisited;

    retVal = TRUE;

doneMarkUrlAsVisited:
    memcpy( szUrlName, szBackup, sizeof(MSHIST_PREFIX_SZ));

    if( pCei != NULL)
        delete [] pCei;

    return retVal;
}


//************************************************************************8
//
//  IE401Content : public IE401IndexFile
//


//  on the creation of a IE401Content object, we
//prepare for the enumeration of its entries by:
// - identifying the subdirectories of the old cache
// - register each subdirectory in the new cache
// - move each subdirectory into the new cache's location
IE401Content::IE401Content( LPCSTR szFilename)
: IE401IndexFile( szFilename)
{
    BOOL fConstructionSuccessful = FALSE;

    //  make sure the index file loaded alright.
    if( m_pBuf == NULL)
        goto exitIE401Construct;

    //  ConfigInfo is retrieved since it contains the path of the new cachefile.
    CACHE_CONFIG_INFO sConfigInfo;
    DWORD dwTemp;
    if( GetUrlCacheConfigInfo( &sConfigInfo,
                               &(dwTemp = sizeof(sConfigInfo)),
                               CACHE_CONFIG_CONTENT_PATHS_FC)
        == FALSE)
    {
        goto exitIE401Construct;
    }

    //  get the target path for subcontainer move
    m_cbRootPathLength = lstrlen( sConfigInfo.CachePath);
    memcpy( m_szRootPath, sConfigInfo.CachePath, m_cbRootPathLength + 1);
    //target path example: m_szRootPath = "c:\winnt\content.ie5"

    //  get the source path for the subcontainers from the given filename
    DWORD cbSourcePathLength;
    CHAR szSourcePath[MAX_PATH];
    cbSourcePathLength = lstrlen( szFilename);
    memcpy( szSourcePath, szFilename, cbSourcePathLength + 1);
    // clip off the filename so that we have just the path.
    while( cbSourcePathLength > 0 && szSourcePath[cbSourcePathLength] != FILENAME_SEPARATOR)
    {
        cbSourcePathLength--;
    }
    szSourcePath[ ++cbSourcePathLength] = '\0';
    //source path example:  szSourcePath = "c:\winnt\content\"

    //  enumerate through the subdirectories,
    //      attempt to register that directory in the new cache
    //      then move the old directory into the new cache.
    //  If the directory cannot be registered or moved, then
    //m_szDir contains "" for that directory index.
    m_nDirs = ((_MEMMAP_HEADER_SMALL*)m_pBuf)->nDirCount;
    DWORD index;
    for( index = 0; index < m_nDirs; index++)
    {
        //  get the name of the old subdirectory from the cache.
        memcpy( m_szDir[index],
                ((_MEMMAP_HEADER_SMALL*)m_pBuf)->DirArray[index].sDirName,
                DIR_NAME_SIZE);
        m_szDir[index][DIR_NAME_SIZE] = '\0';

        if( GlobalUrlContainers->CreateContentDirWithSecureName( m_szDir[index]) != TRUE)
        {
            // signal that the directory couldn't be imported and try the next.
            m_szDir[index][0] = '\0';
            continue;
        }

        //  append the subcontainer names to the appropiate destination and source
        //paths.
        memcpy( m_szRootPath + m_cbRootPathLength, m_szDir[index], DIR_NAME_SIZE + 1);
        memcpy( szSourcePath + cbSourcePathLength, m_szDir[index], DIR_NAME_SIZE + 1);

#ifndef UNIX
        if( MoveFile( szSourcePath, m_szRootPath) == 0)
#else
        if (!hConstructSubDirs(m_szRootPath) ||
            CopyDir(szSourcePath, m_szRootPath))
#endif /* UNIX */
        {

            // signal that the directory couldn't be imported and try the next.
            m_szDir[index][0] = '\0';
            continue;
        }
    }

    szSourcePath[ cbSourcePathLength] = '\0';
    m_szRootPath[ m_cbRootPathLength] = '\0';

#ifndef UNIX
    //  The old index file now is full of dead entries,
    //so we don't keep it around.
    DeleteFile( szFilename);
#endif /* UNIX */

    fConstructionSuccessful = TRUE;

exitIE401Construct:

    if( fConstructionSuccessful != TRUE)
    {
        if( m_pBuf != NULL)
            delete [] m_pBuf;

        m_pBuf = NULL;
    }
}


//  imports only URLs using Import401Url
BOOL IE401Content::HandleHashElement( ie401::HASH_ITEM* pEntry)
{
    // No reserved bits should be set.
    INET_ASSERT (!(pEntry->dwHash & HASH_BIT_RESERVED));

    if( !(pEntry->dwHash & HASH_BIT_NOTURL)
        && ((ie401::FILEMAP_ENTRY*)(m_pBuf + pEntry->dwOffset))->dwSig == SIG_URL)
    {
        Import401UrlWithFile( (ie401::URL_FILEMAP_ENTRY*)(m_pBuf + pEntry->dwOffset));
    }

    // after wrapping above in try-catch block, return FALSE on unhandled exception.
    return TRUE;
}


//  Extends the default to import files
BOOL IE401Content::Import401UrlWithFile( ie401::URL_FILEMAP_ENTRY* pEntry)
{
    BOOL retVal = FALSE;

    //  don't import Url if it is already in the buffer
    if( GetUrlCacheEntryInfo( (LPSTR)((BYTE*)pEntry + pEntry->UrlNameOffset), NULL, 0) == TRUE
        || GetLastError() != ERROR_FILE_NOT_FOUND)
    {
       goto doneImport401Url;
    }

    //  don't import an URL if its one of those weird registered files.
    if( (pEntry->DirIndex == IE401_NOT_A_CACHE_SUBDIRECTORY)
        || (pEntry->CacheEntryType & IE401_EDITED_CACHE_ENTRY))
    {
        goto doneImport401Url;
    }

    if( pEntry->FileSize != 0)
    {
        //  don't import Url if it's subdirectory didn't get created
        if( m_szDir[pEntry->DirIndex][0] == '\0')
            goto doneImport401Url;

        // store the new file path in m_szRoot
        memcpy( m_szRootPath + m_cbRootPathLength, m_szDir[pEntry->DirIndex], DIR_NAME_SIZE);
        m_szRootPath[ m_cbRootPathLength + DIR_NAME_SIZE] = FILENAME_SEPARATOR;
        m_szRootPath[ m_cbRootPathLength + DIR_NAME_SIZE + 1] = '\0';

        // This may result in truncation of the filename, when the 401 generated filename to
        // particularly long, causing the total path to exceed MAX_PATH.
        // We won't worry abou this.
        StrCatBuff(m_szRootPath,
                   (CHAR*)pEntry + pEntry->InternalFileNameOffset,
                   MAX_PATH);
    }

    if( !CommitUrlCacheEntry(
            (LPCSTR)((BYTE*)pEntry + pEntry->UrlNameOffset),
            (pEntry->FileSize != 0)
              ? m_szRootPath : NULL,
            *LONGLONG_TO_FILETIME(&pEntry->ExpireTime),
            *LONGLONG_TO_FILETIME(&pEntry->LastModifiedTime),
            pEntry->CacheEntryType,
            pEntry->HeaderInfoOffset != NULL ?
                (BYTE*)((BYTE*)pEntry + pEntry->HeaderInfoOffset) : NULL,
            pEntry->HeaderInfoSize,
            pEntry->FileExtensionOffset != 0 ?
                (LPCSTR)((BYTE*)pEntry + pEntry->FileExtensionOffset) : NULL,
            NULL))
        goto doneImport401Url;

    CACHE_ENTRY_INFO cei;

    cei.dwStructSize = sizeof(cei);
    cei.LastAccessTime = *LONGLONG_TO_FILETIME(&pEntry->LastAccessedTime);
    cei.dwHitRate = pEntry->NumAccessed;
    cei.ExpireTime = *LONGLONG_TO_FILETIME(&pEntry->ExpireTime);
    cei.LastModifiedTime = *LONGLONG_TO_FILETIME(&pEntry->LastModifiedTime);

    if( !SetUrlCacheEntryInfo(
            (LPCSTR)((BYTE*)pEntry + pEntry->UrlNameOffset),
            &cei,
            CACHE_ENTRY_ACCTIME_FC | CACHE_ENTRY_HITRATE_FC
            | CACHE_ENTRY_EXPTIME_FC | CACHE_ENTRY_MODTIME_FC))
        goto doneImport401Url;

    retVal = TRUE;

doneImport401Url:

    //  Remove appended data
    m_szRootPath[m_cbRootPathLength] = '\0';

    return retVal;
}



//*************************************************************************
//
//  Import401Redirects()
//

IE401Redirects::IE401Redirects( IE401Content* pContent)
{
    INET_ASSERT( m_pBuf == NULL);

    m_pBuf = pContent->m_pBuf;
    pContent->m_pBuf = NULL;
}


//  import items that are redirects
BOOL IE401Redirects::HandleHashElement( ie401::HASH_ITEM* pEntry)
{
    // No reserved bits should be set.
    INET_ASSERT (!(pEntry->dwHash & HASH_BIT_RESERVED));

    if((pEntry->dwOffset != HASH_END) && ((ie401::FILEMAP_ENTRY*)(m_pBuf + pEntry->dwOffset))->dwSig == SIG_REDIR)
    {
        Import401Redirect( (ie401::REDIR_FILEMAP_ENTRY*)(m_pBuf + pEntry->dwOffset));
    }

    // after wrapping above in try-catch block, return FALSE on unhandled exception.
    return TRUE;
}


//  Imports entries that are redirects
BOOL IE401Redirects::Import401Redirect( ie401::REDIR_FILEMAP_ENTRY* pEntry)
{
    LPSTR szTargetUrl = NULL;

    //** wrap this one in an exception block, because just one of these guys
    //might just try to reference into space.

    // pEntry is an entry to a redirect entry, which contains an offset to a hash entry.
    // That hash entry contains an offset to the filemap entry of the redirect target.
    FILEMAP_ENTRY* pRedirTarget =
        (FILEMAP_ENTRY*)(m_pBuf + ((HASH_ITEM*)(m_pBuf + pEntry->dwItemOffset))->dwOffset);
    //  The filemap entry of the redirect target could be a URL filmap entry or another
    //redirect filemap entry.  Either way extract the url of that entry as the target url.
    switch( pRedirTarget->dwSig)
    {
    case SIG_REDIR:
        szTargetUrl = ((REDIR_FILEMAP_ENTRY*)pRedirTarget)->szUrl;
        break;
    case SIG_URL:
        szTargetUrl = (CHAR*)pRedirTarget + ((URL_FILEMAP_ENTRY*)pRedirTarget)->UrlNameOffset;
        break;
    default:
        return FALSE;
    }

    return GlobalUrlContainers->CreateContentRedirect( szTargetUrl, pEntry->szUrl);
}


//*************************************************************************
//
//  Import401History()
//


//  Inside IE401_HIST_ROOT{hHistRoot}, there are some keys that list
//the history containers we want to import.  Before we can import those containers,
//we import the Visited: container.  This file will be found in the subdirectory of
//the location of the first history subcontainer.
//  When we reach a new container, we create it but don't worry about collisions.
//Existing Url entries are never overwritten by the import process.

BOOL Import401History()
{
    BOOL retVal = FALSE;

    HKEY hHistRoot = (HKEY) INVALID_HANDLE_VALUE;

    // get key to root of history information (which is the root of all extensible containers)
    if( REGOPENKEYEX( IsPerUserCache() ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE,
                      IE401_HIST_ROOT, 0, KEY_READ, &hHistRoot)
        != ERROR_SUCCESS)
    {
        hHistRoot = (HKEY) INVALID_HANDLE_VALUE;
        goto doneImport401History;
    }

    DWORD index; index = 0;
    CHAR szContainerName[MAX_PATH];
    DWORD cbContainerNameLength;
    DWORD dwCacheLimit;
    DWORD dwCacheOptions;
    CHAR szCachePath[MAX_PATH];
    DWORD cbCachePathSize;
    CHAR szCachePrefix[MAX_PATH];

    //  Enumerate through the extensible containers, if they are History containers import them.
    //  When the first history container is found, its path can be used to locate the Visited:
    //container which also must be imported.
    while( RegEnumKeyEx( hHistRoot, index++, szContainerName, &(cbContainerNameLength = MAX_PATH),
            NULL,NULL,NULL,NULL) == ERROR_SUCCESS)
    {
        static BOOL fFirstRun = TRUE;

        //  we can't be sure all the extended containers are history containers.
        //This check verifies we only import history containers.
        if( StrCmpNI( szContainerName, MSHIST_SZ, sizeof(MSHIST_SZ) - 1) != 0
            || cbContainerNameLength != sizeof(MSHIST_KEY_SZ) - 1)
        {
            continue;
        }

        HKEY hHistContainer = (HKEY) INVALID_HANDLE_VALUE;
        DWORD dwTemp;
        DWORD dwType;

        if( REGOPENKEYEX( hHistRoot, szContainerName, 0, KEY_READ, &hHistContainer) != ERROR_SUCCESS)
            goto doneImportHistContainer;

        if( RegQueryValueEx( hHistContainer, CACHE_LIMIT_SZ, 0, &dwType,
                            (BYTE*)&dwCacheLimit, &(dwTemp = sizeof(dwCacheLimit))) != ERROR_SUCCESS
            || dwType != REG_DWORD)
        {
            goto doneImportHistContainer;
        }

        if( RegQueryValueEx( hHistContainer, CACHE_OPTIONS_SZ, 0, &dwType,
                            (BYTE*)&dwCacheOptions, &(dwTemp = sizeof(dwCacheOptions))) != ERROR_SUCCESS
            || dwType != REG_DWORD)
        {
            goto doneImportHistContainer;
        }

        if( RegQueryValueEx( hHistContainer, CACHE_PATH_SZ, 0, &dwType,
                            (BYTE*)szCachePath, &(cbCachePathSize = sizeof(szCachePath))) != ERROR_SUCCESS
            || dwType != REG_SZ)
        {
            goto doneImportHistContainer;
        }

        if( RegQueryValueEx( hHistContainer, CACHE_PREFIX_SZ, 0, &dwType,
                            (BYTE*)&szCachePrefix, &(dwTemp = sizeof(szCachePrefix))) != ERROR_SUCCESS
            || dwType != REG_SZ)
        {
            goto doneImportHistContainer;
        }

        //  After finding the first container, import the Visited: container.
        if( fFirstRun == TRUE)
        {
            //  Clip off the path of the history container, and substitute 'index.dat'
            //to identify the Visited container.  Import the visited container, and
            //restore the path to the history container.
            CHAR szBuf[sizeof(MSHIST_DIR_SZ)];
            LPSTR szMSHIST = szCachePath + cbCachePathSize - sizeof(MSHIST_DIR_SZ);
            // ex result: szCachePath:"c:\path\MSHIST011998020119980225\", szMSHIST:"MSHIST011998020119980225\"

            memcpy( szBuf, szMSHIST, sizeof(MSHIST_DIR_SZ));
            // szBuf:"MSHIST011998020119980225\"
            memcpy( szMSHIST, "index.dat", sizeof("index.dat"));
            //  szMSHIST:"index.dat"  --> szCachePath:"c:\path\index.dat"
            IE401Visited Visited(szCachePath);
            Visited.EnumHashValues();
            memcpy( szMSHIST, szBuf, sizeof(MSHIST_DIR_SZ));
            // szMSHIST:"MSHIST011998020119980225\"  --> szCachePath:"c:\path\MSHIST011998020119980225\"

            fFirstRun = FALSE;
        }

        //  we don't pass the old path so that the container is put in the new one.
        if( CreateUrlCacheContainer( szContainerName, szCachePrefix, NULL,
                                 dwCacheLimit, 0, dwCacheOptions, NULL, NULL) != TRUE
            && GetLastError() != ERROR_ALREADY_EXISTS)
        {
            goto doneImportHistContainer;
        }

        if( cbCachePathSize + (sizeof(INDEX_DAT_SZ) - 1) > MAX_PATH)
            goto doneImportHistContainer;

        memcpy( szCachePath + cbCachePathSize - 1, INDEX_DAT_SZ, sizeof(INDEX_DAT_SZ));

        {
            IE401History History( szCachePath);

            History.EnumHashValues();
        }

    doneImportHistContainer:

        if( hHistContainer != (HKEY) INVALID_HANDLE_VALUE)
            REGCLOSEKEY( hHistContainer);
    }

    retVal = TRUE;

doneImport401History:

    if( hHistRoot != (HKEY) INVALID_HANDLE_VALUE)
        REGCLOSEKEY( hHistRoot);

    return retVal;
}


//*************************************************************************
//
//  Import401Content()
//


BOOL Import401Content()
{
    BOOL retVal = FALSE;
    HKEY hContentKey = (HKEY) INVALID_HANDLE_VALUE;
    CHAR szContentFilename[MAX_PATH];
    LPSTR szKeyName = NULL;

    if( !GetIE5ContentPath(szContentFilename))
        goto doneImport401Content;

    //  we now have something like 'c:\..\content\content.ie5'
    //and we want something like 'c:\..\content\index.dat'
    DWORD index;

    //  find the position of the last '\\'
    index = lstrlen( szContentFilename);
    while( index >= 0 && szContentFilename[index] != FILENAME_SEPARATOR)
        index--;

    // append an 'index.dat'
    memcpy( szContentFilename + index + 1, INDEX_DAT_SZ, sizeof(INDEX_DAT_SZ));

#ifdef UNIX
    {
       // HACK HACK
       //
       // we now have something like
       //        '/home/blah/.microsoft/ie5/TempInternetFiles/index.dat'
       // and we want something like
       //        /home/blah/.microsoft/TempInternetFiles/index.dat
       char szSearchStr[] = "ie5/";
       char *pszWhere = StrStr(szContentFilename, szSearchStr);
       if (pszWhere)
       {
          memmove(pszWhere, pszWhere+sizeof(szSearchStr)-1,
                  lstrlen(pszWhere+sizeof(szSearchStr)-1)+1);
       }
    }
#endif /* UNIX */

    {
        IE401Content Content(szContentFilename);
        if( Content.EnumHashValues() == TRUE)
        {
            IE401Redirects Redirects( &Content);
            Redirects.EnumHashValues();
        }
    }

doneImport401Content:

    if( hContentKey != (HKEY) INVALID_HANDLE_VALUE)
        REGCLOSEKEY( hContentKey);

    return retVal;
}


//-- end of namespace ie401
}
//--


//
//
//  Returns if caches are per user.
//
BOOL IsPerUserCache()
{
    BOOL fProfilesEnabled = FALSE;
    static BOOL fPerUser = FALSE;
#ifndef UNIX
    // Is the OS version Windows 95 or Windows NT?
    if (GlobalPlatformType == PLATFORM_TYPE_WIN95)
    {
        // Operating System is Windows 95.
        //  Look for special key indicating profiles are enables.  If its not found,
        //know that profiles aren't enabled.

        // Determine if profiles are enabled by OS.
        REGISTRY_OBJ roProfilesEnabled(HKEY_LOCAL_MACHINE, PROFILES_ENABLED_VALUE);
        if (roProfilesEnabled.GetStatus() == ERROR_SUCCESS)
        {
            DWORD dwProfilesEnabled = 0;

            if( roProfilesEnabled.GetValue(PROFILES_ENABLED, &dwProfilesEnabled) == ERROR_SUCCESS)
            {
                // Found the registry entry.
                fProfilesEnabled = (BOOL) dwProfilesEnabled;
            }
        }

    }
    else if (GlobalPlatformType == PLATFORM_TYPE_WINNT)
    {
        // Profiles always enabled for NT.
        fProfilesEnabled = TRUE;
    }
#else
    fProfilesEnabled = TRUE;
#endif /* UNIX */

    // Determine if per user cache is allowed.
    REGISTRY_OBJ roProfilesAllowed(HKEY_LOCAL_MACHINE, PRE5_CACHE_KEY);
    if( fProfilesEnabled && roProfilesAllowed.GetStatus() == ERROR_SUCCESS)
    {
        DWORD dwPerUserCache = 0;

        if( roProfilesAllowed.GetValue(PROFILES_ENABLED,&dwPerUserCache) == ERROR_SUCCESS)
        {
            // Found the registry entry. Set g_fPerUserCache to
            // TRUE only if profiles are enabled.
            fPerUser = ((BOOL) dwPerUserCache);
        }
        else
        {
            // No entry. If profiles are enabled, assume they're allowed.
            fPerUser = fProfilesEnabled;
        }
    }
    else
    {
        fPerUser = fProfilesEnabled;
    }

    return fPerUser;
}

