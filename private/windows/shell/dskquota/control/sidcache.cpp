///////////////////////////////////////////////////////////////////////////////
/*  File: sidcache.cpp

    Description: This module provides the functionality for a cache of user
        SID/Name pairs.  See the file header in sidcache.h for details.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/12/96    Initial creation.                                    BrianAu
    08/14/96    Added SidCacheQueueIterator.                         BrianAu
    09/20/96    Total redesign.  Old design loaded data from file    BrianAu
                into an in-memory hash table.  New design leaves
                everything on disk and merely maps the file into
                memory.  Much more efficient with respect to
                speed and size.
    07/02/97    Added SidNameCache::GetFileNames.                    Brianau
                Changed logic for identifying cache file path.
                Removed index bucket count param from registry.
    03/18/98    Replaced "domain", "name" and "full name" with       BrianAu
                "container", "logon name" and "display name" to
                better match the actual contents.  This was in
                reponse to making the quota UI DS-aware.  The
                "logon name" is now a unique key as it contains
                both account name and domain-like information.
                i.e. "REDMOND\brianau" or "brianau@microsoft.com".
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h" // PCH
#pragma hdrstop

#include "dskquota.h"
#include "sidcache.h"
#include "registry.h"

//
// Verify that build is UNICODE.
//
#if !defined(UNICODE)
#   error This module must be compiled UNICODE.
#endif

//
// How long (milliseconds) we'll wait to get a lock on the cache.
//
const DWORD MUTEX_WAIT_TIMEOUT      = 5000;
//
// Byte value used to fill in unused blocks in the data file.
//
const BYTE  RECORD_UNUSED_BYTE      = 0xCC;
//
// A value to signify the start of a record in the data file.
// The bit pattern is  1010101010101010 0101010101010101
// Highly unlikely that any data file data will produce this
// pattern.
//
const DWORD RECORD_SIGNATURE        = 0xAAAA5555;
//
// Signatures written into the header of the index and data files.
// For validating a file just in case someone's put another file
// in their place.  The numbers are arbitrary.
// 2600 means "MS building 26N (where I'm working now).
// 3209 is my office number.
// 3210 is BobDay's office number (across the hall).
// Hey, I had to pick something.
//
const DWORD INDEX_FILE_SIGNATURE    = 0x26003209;
const DWORD DATA_FILE_SIGNATURE     = 0x26003210;
//
// A version number so a future build of the software won't be confused
// by a change in file formats.  Bump this if the file format changes.
//
const DWORD FILE_VERSION            = 0x00000003;
//
// Average number of 32-byte blocks per cache entry.
// Entries are variable length (SID, Name etc).  This average is used
// in initially sizing the data file.  I've found that most entries (by far)
// require 4 blocks. Both the data file and index file grow independently
// as needed so it isn't a problem if this isn't always accurate.
//
const DWORD AVG_BLOCKS_PER_ENTRY    = 4;
//
// Create space for this many records in a new data file.
// Since the data and index files grow automatically as needed,
// this can change as you see fit.
//
#if DBG
    const DWORD NEW_CACHE_ENTRY_COUNT   = 4;   // Force frequent file growth.
#else
    const DWORD NEW_CACHE_ENTRY_COUNT   = 128;
#endif
//
// The index and data files automatically grow when needed.  These
// values control how much they grow by.
//
#if DBG
    const DWORD DATA_FILE_GROW_BLOCKS   = 4;  // Force frequent file growth.
#else
    const DWORD DATA_FILE_GROW_BLOCKS   = 512;
#endif

const DWORD INDEX_FILE_GROW_ENTRIES = (DATA_FILE_GROW_BLOCKS / AVG_BLOCKS_PER_ENTRY);
//
// The number of buckets in the cache index hash table.
// Number should be prime.  Remember, this index is on disk so we can afford to
// have a reasonably large hash table.  While it would be nice to fit the
// buckets within a single page of memory, that would be too small to be effective
// 512 / 4 == 64 buckets.  There's also no guarantee that all buckets would be
// mapped to a single physical page.
//
const DWORD INDEX_BUCKET_COUNT = 503;
//
// Convert between blocks and bytes.
// BLOCK_SIZE is a power of 2 so the multiply and division
// can be optimized to shifts.
//
#define BYTE_TO_BLOCK(b)  ((b) / BLOCK_SIZE)
#define BLOCK_TO_BYTE(b)  ((b) * BLOCK_SIZE)
//
// Base pointers for mapped data and index files.
// When the files are mapped into memory, these globals contain
// the address of the mapped memory.
//
LPBYTE g_pbMappedDataFile;
LPBYTE g_pbMappedIndexFile;

//
// Macros for working with based pointers.
// Pointer members in the file structures contain offsets relative
// to the start of the file.  When dereferencing these pointers,
// they must be converted to "based" pointers which add the file's
// base address to the pointer value.  This results in a true
// virtual address that can be accessed.
//
#if defined(_X86_)
#   define NDX_BASED(t)  t __based(g_pbMappedIndexFile)
#   define DAT_BASED(t)  t __based(g_pbMappedDataFile)
#   define NDX_BASED_CAST(t,e)  (NDX_BASED(t) *)((DWORD)(e))
#   define DAT_BASED_CAST(t,e)  (DAT_BASED(t) *)((DWORD)(e))
#else
//
// BUGBUG:
// I think there's a bug in the ALPHA compiler that is preventing __based pointers
// from working as I have used them.
// This is a workaround until the bug is fixed or I find out what I'm doing wrong.
//
#   define NDX_BASED(t)  t
#   define DAT_BASED(t)  t
#   define NDX_BASED_CAST(t,e)  ((NDX_BASED(t) *)(((BYTE *)g_pbMappedIndexFile) + ((DWORD_PTR)(e))))
#   define DAT_BASED_CAST(t,e)  ((DAT_BASED(t) *)(((BYTE *)g_pbMappedDataFile) + ((DWORD_PTR)(e))))
#endif

//
// Macros to verify that the files have been mapped.
// These are primarily used in assertions.
//
#define INDEX_FILE_MAPPED  (NULL != g_pbMappedIndexFile)
#define DATA_FILE_MAPPED   (NULL != g_pbMappedDataFile)
//
// Names for system objects.  Mutex and maps are named so they can
// be shared between processes.
//
const LPCTSTR g_szSidCacheMutex     = TEXT("DSKQUOTA_SIDCACHE_MUTEX");
const LPCTSTR g_pszIndexFileMapping = TEXT("DSKQUOTA_SIDCACHE_INDEX");
const LPCTSTR g_pszDataFileMapping  = TEXT("DSKQUOTA_SIDCACHE_DATA");
//
// Use to clear a file's GUID and to test for a NULL guid.
//
static const GUID GUID_Null =
{ 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

//
// Registry parameter value names.
//
const TCHAR g_szSidCacheRecLifeMin[]   = TEXT("SidCacheRecLifeMin");
const TCHAR g_szSidCacheRecLifeMax[]   = TEXT("SidCacheRecLifeMax");


//***********************************************************************************
//***********************************************************************************
//  C A C H E   M A N A G E R
//***********************************************************************************
//***********************************************************************************

///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::SidNameCache

    Description: Constructor.  Creates an empty SID/Name cache object.
        Call one of the Initialize() methods to either create a new index and
        data file or to open existing ones.

    Arguments: None.

    Returns: Nothing.

    Exceptions: SyncObjErrorCreate.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
SidNameCache::SidNameCache(
    VOID
    ) : m_hMutex(NULL),
        m_pIndexMgr(NULL),
        m_pRecordMgr(NULL)
{
    DBGTRACE((DM_SIDCACHE, DL_MID, TEXT("SidNameCache::SidNameCache")));
    if (NULL == (m_hMutex = CreateMutex(NULL,                    // No security
                                        FALSE,                   // Non-owned
                                        g_szSidCacheMutex)))
    {
        throw CSyncException(CSyncException::mutex, CSyncException::create);
    }

    SetCacheFilePath();
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::~SidNameCache

    Description: Destructor.  Destroys the cache object by deleting the
        Index Manager and Record Manager objects.  The respective destructor's
        for each of the managers will handle closing their files and mapping
        objects.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
SidNameCache::~SidNameCache(
    VOID
    )
{
    DBGTRACE((DM_SIDCACHE, DL_MID, TEXT("SidNameCache::~SidNameCache")));
    if (NULL != m_hMutex)
        Lock();

    delete m_pIndexMgr;
    delete m_pRecordMgr;

    if (NULL != m_hMutex)
    {
        ReleaseLock();
        CloseHandle(m_hMutex);
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::CreateNewCache

    Description: This static member function is used to create a new
        SID/Name cache object.
        Even though this method accepts the address of ANY SidNameCache
        pointer, it should only be used to initialize the g_SidNameCache
        pointer (See assertion).   This method could have gone in the UTILS
        module but I thought functionally it is more tied to the SidNameCache
        class.  Note that it is static so there is no "this" pointer.

    Arguments:
        ppCache - Address of pointer variable to receive address of newly
            create SidNameCache object.

    Returns:
        NO_ERROR        - Success.
        E_FAIL          - Error initializing cache.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/14/96    Initial creation.                                    BrianAu
                Code was moved from within
                SidNameResolver::Initialize().
    09/05/96    Added exception handling.                            BrianAu
    09/20/96    Module re-design.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
SidNameCache::CreateNewCache(
    SidNameCache **ppCache
    )
{
    DBGTRACE((DM_SIDCACHE, DL_HIGH, TEXT("SidNameCache::CreateNewCache")));

    DBGASSERT((NULL != ppCache));
    DBGASSERT((NULL == *ppCache));
    DBGASSERT((&g_pSidCache == ppCache)); // Only initialize global cache.

    HRESULT hr = NOERROR;

    *ppCache = NULL;
    autoptr<SidNameCache> ptrCache(new SidNameCache);

    //
    // Open/Create new cache data and index files.
    // Will first try to open existing.  If either the index or
    // data file doesn't exist or is invalid, new files are created.
    //
    hr = ptrCache->Initialize(TRUE);
    if (SUCCEEDED(hr))
    {
        *ppCache = ptrCache.get();
        ptrCache.disown();
    }
    else
    {
        DBGERROR((TEXT("SID cache initialization failed with error 0x%08X"), hr));
    }

    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::Initialize

    Description: Initializes a new cache object by creating the Index and
        Record manager objects then initializing each.  Initialization first
        tries to open existing index and data files.  If one (or both) of the
        files does not exist OR one (or both) of the files is considered
        "invalid", new files are created.  Need to take a "fail safe" approach
        to this.

    Arguments:
        bOpenExisting - TRUE = Try to open an existing cache index and data file.
            If it can't, it creates a new one.  FALSE = Just create a new one.

    Returns:
        NO_ERROR    - Success.
        E_FAIL      - Could not open nor create required files.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
SidNameCache::Initialize(
    BOOL bOpenExisting
    )
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("SidNameCache::Initialize")));
    DBGASSERT((NULL == m_pIndexMgr));
    DBGASSERT((NULL == m_pRecordMgr));

    HRESULT hResult = E_FAIL;
    CacheAutoLock lock(*this);

    if (lock.Lock())
    {
        try
        {
            if (m_strFilePath.IsEmpty())
            {
                //
                // If file path is empty, it means we couldn't get the
                // user's profile directory from the registry.
                //
                DBGERROR((TEXT("Error creating SID cache files.  No path.")));
            }
            else
            {
                //
                // Name for our cache data and index files.
                // Will append .DAT and .NDX respectively.
                // This is where you change the file name or extension(s)
                // if you want to do that.
                //
                const TCHAR szSidCacheFile[] = TEXT("NTDiskQuotaSidCache");

                //
                // Create a fully-qualified path for the cache data and index
                // files.  m_strFilePath was set in the cache object ctor.
                //
                CString strDataFile(m_strFilePath);
                CString strIndexFile(m_strFilePath);

                strDataFile.Format(TEXT("%1\\%2.dat"), (LPCTSTR)m_strFilePath, szSidCacheFile);
                strIndexFile.Format(TEXT("%1\\%2.ndx"),(LPCTSTR)m_strFilePath, szSidCacheFile);

                //
                // Create the record and index manager objects.
                //
                m_pRecordMgr = new RecordMgr(*this);
                m_pIndexMgr  = new IndexMgr(*this);

                DBGPRINT((DM_CONTROL, DL_MID, TEXT("Create SID cache DataFile = %s  IndexFile = %s"),
                         (LPCTSTR)strDataFile, (LPCTSTR)strIndexFile));

                if (bOpenExisting)
                {
                    //
                    // First try opening existing data and index files.
                    //
                    if (NULL != m_pRecordMgr->Initialize(strDataFile))
                    {
                        if (NULL != m_pIndexMgr->Initialize(strIndexFile))
                            hResult = NO_ERROR;
                    }
                }

                if (FAILED(hResult) || !FilesAreValid())
                {
                    hResult = E_FAIL;
                    //
                    // Couldn't open existing files, try creating new ones.
                    // Any open files/mappings will be closed.
                    //
                    if (NULL != m_pRecordMgr->Initialize(strDataFile,
                                          NEW_CACHE_ENTRY_COUNT * AVG_BLOCKS_PER_ENTRY))
                    {
                        if (NULL != m_pIndexMgr->Initialize(strIndexFile,
                                                            INDEX_BUCKET_COUNT,
                                                            NEW_CACHE_ENTRY_COUNT))
                        {
                            hResult = NO_ERROR;
                        }
                    }
                }
            }
        }
        catch(...)
        {
            delete m_pRecordMgr;
            m_pRecordMgr = NULL;
            delete m_pIndexMgr;
            m_pIndexMgr = NULL;
            throw;  // Re-throw to caller.
        }
        //
        // Mark files as "valid".
        //
        if (SUCCEEDED(hResult))
            ValidateFiles();
    }
    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::OpenMappedFile

    Description: Opens or creates a file, maps the file into memory and
        returns the address of the mapping.

    Arguments:
        pszFile - Address of name of file to create/open.

        pszMapping - Name to give the mapping object.  This object is
            named so that if multiple processes map the same file (using
            the same mapping name), the file is only mapped once.

        dwCreation - Creation flag (CREATE_ALWAYS, OPEN_EXISTING);

        cbFileHigh/Low - If creating a new file or extending an existing
            these two arguments contain the desired size in bytes.

        phFile - Address of handle variable to receive the open file's
            handle value.  Call CloseHandle on this to close the file.

        phFileMapping - Address of handle variable to receive the open
            file mapping's handle value.  Call CloseFileMapping on this
            to close the mapping.

    Returns:
        Address of the mapped file in memory.  NULL on failure.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
    07/21/97    Files are now created in user's profile under a      BrianAu
                "DiskQuota" subdirectory.
*/
///////////////////////////////////////////////////////////////////////////////
LPBYTE
SidNameCache::OpenMappedFile(
    LPCTSTR pszFile,
    LPCTSTR pszMapping,
    DWORD dwCreation,
    DWORD cbFileHigh,
    DWORD cbFileLow,
    PHANDLE phFile,
    PHANDLE phFileMapping
    )
{
    DBGTRACE((DM_SIDCACHE, DL_MID, TEXT("SidNameCache::OpenMappedFile")));
    LPBYTE pbBase = NULL;

    DBGASSERT((NULL != pszFile));
    DBGASSERT((NULL != pszMapping));
    DBGASSERT((NULL != phFile));
    DBGASSERT((NULL != phFileMapping));

    *phFile = CreateFile(pszFile,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         dwCreation,
                         FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN,
                         NULL);

    if (INVALID_HANDLE_VALUE != *phFile)
    {
        if ((*phFileMapping = CreateFileMapping(*phFile,
                                                NULL,
                                                PAGE_READWRITE,
                                                cbFileHigh,
                                                cbFileLow,
                                                pszMapping)) != NULL)
        {
            pbBase = (LPBYTE)MapViewOfFile(*phFileMapping,
                                           FILE_MAP_WRITE,
                                           0,
                                           0,
                                           0);
            if (NULL == pbBase)
                DBGERROR((TEXT("SIDCACHE - Failed to map view of file %s"),
                         pszFile));
        }
        else
            DBGERROR((TEXT("SIDCACHE - Failed to create mapping %s for file %s"),
                     pszMapping, pszFile));
    }
    else
        DBGERROR((TEXT("SIDCACHE - Failed to open file %s"), pszFile));


    return pbBase;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::SetCacheFilePath

    Description: Obtains the fully-qualified path for the cache's data
        and index files and stores the value in m_strFilePath.  The files
        are to be created under in the user's profile under the directory
        \AppData\Microsoft\Windows NT\DiskQuota.  We have to read the registry
        to find exactly where this subtree lives for this user.

    Arguments: None.

    Returns: Nothing.
             On return, m_strFilePath contains the path to the files.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/02/97    Initial creation.                                    BrianAu
    07/21/97    Removed default file path.                           BrianAu
                Files can only be stored in user's profile.
                Can't allow unsecure access to SID/Name pairs.
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::SetCacheFilePath(
    VOID
    )
{
    DBGTRACE((DM_SIDCACHE, DL_HIGH, TEXT("SidNameCache::SetCacheFilePath")));
    //
    // Get the user's %UserProfile%\Application Data directory.
    // Normally, an app gets this through SHGetSpecialFolderLocation or
    // SHGetSpecialFolderPath.  However, I don't want to load shell32.dll
    // just for that purpose (I've tried to keep shell32 out of this dll).
    // Therefore, we read the registry values just like the shell does.
    // EricFlo suggested this so it's OK ZAW-wise.
    //
    LONG lResult        = ERROR_SUCCESS;
    HKEY hKey           = NULL;
    DWORD dwDisposition = 0;
    const TCHAR szKeyNameRoot[]      = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer");
    const TCHAR szMSWinNTDiskQuota[] = TEXT("Microsoft\\Windows NT\\DiskQuota");

    LPCTSTR rgpszKeys[] = {
                            TEXT("\\User Shell Folders"),
                            TEXT("\\Shell Folders"),
                          };

    //
    // Start with an empty path buffer.
    //
    m_strFilePath.Empty();

    for (INT i = 0; i < ARRAYSIZE(rgpszKeys) && m_strFilePath.IsEmpty(); i++)
    {
        //
        // Create the key name.
        //
        CString strKeyName(szKeyNameRoot);
        strKeyName += CString(rgpszKeys[i]);

        //
        // Open the reg key.
        //
        lResult = RegCreateKeyEx(HKEY_CURRENT_USER,
                                 strKeyName,
                                 0,
                                 NULL,
                                 0,
                                 KEY_READ,
                                 NULL,
                                 &hKey,
                                 &dwDisposition);

        if (ERROR_SUCCESS == lResult)
        {
            try
            {
                //
                // Get the path to the user's "Application Data" directory.
                //
                DBGASSERT((NULL != hKey));

                DWORD dwValueType = 0;
                DWORD cbValue     = MAX_PATH * sizeof(TCHAR);

                lResult = RegQueryValueEx(hKey,
                                          TEXT("AppData"),
                                          0,
                                          &dwValueType,
                                          (LPBYTE)m_strFilePath.GetBuffer(MAX_PATH),
                                          &cbValue);

                m_strFilePath.ReleaseBuffer();

                if (ERROR_SUCCESS == lResult)
                {
                    //
                    // Ensure the path has a trailing backslash.
                    //
                    INT cchPath = m_strFilePath.Length();
                    if (0 < cchPath && TEXT('\\') != m_strFilePath[cchPath-1])
                    {
                        m_strFilePath += CString(TEXT("\\"));
                    }
                    //
                    // Append "Microsoft\Windows NT\DiskQuota" to the path.
                    //
                    m_strFilePath += CString(szMSWinNTDiskQuota);
                }
                else
                {
                    //
                    // Something failed.  Ensure m_strFilePath is empty.
                    //
                    m_strFilePath.Empty();
                    if (ERROR_FILE_NOT_FOUND != lResult)
                    {
                        DBGERROR((TEXT("SIDCACHE - Error %d getting \"AppData\" reg value."), lResult));
                    }
                }
            }
            catch(...)
            {
                RegCloseKey(hKey);  // Don't want to leak this key handle.
                throw;
            }
            RegCloseKey(hKey);
        }
        else if (ERROR_FILE_NOT_FOUND != lResult)
        {
            DBGERROR((TEXT("SIDCACHE - Error %d opening \"\\User Shell Folders\" or \"Shell Folders\" reg key"), lResult));
        }
    }

    if (!m_strFilePath.IsEmpty())
    {
        //
        // Expand any embedded environment strings.
        //
        m_strFilePath.ExpandEnvironmentStrings();

        //
        // Ensure the path DOES NOT have a trailing backslash.
        //
        INT cchPath = m_strFilePath.Length();
        if (0 < cchPath && TEXT('\\') == m_strFilePath[cchPath-1])
        {
            m_strFilePath[cchPath-1] = TEXT('\0');
        }

        if ((DWORD)-1 == ::GetFileAttributes(m_strFilePath))
        {
            //
            // If the directory doesn't exist, try to create it.
            //
            if (0 == CreateCacheFileDirectory(m_strFilePath))
            {
                //
                // Couldn't create the directory, make sure the path
                // is empty so we don't try to write to a non-existent
                // directory.
                //
                DBGERROR((TEXT("SIDCACHE - Error %d creating directory \"%s\""),
                         GetLastError(), (LPCTSTR)m_strFilePath));
                m_strFilePath.Empty();
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::CreateCacheFileDirectory

    Description: Creates the directory for the SID/Name cache files.  Since
        the directory lives several levels below "Application Data", we
        may need to create several directories before we get to DiskQuota.

    Arguments: pszPath - This is a fully-qualified directory path.

    Returns: TRUE  = Directory created.
             FALSE = Directory not created.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/21/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
SidNameCache::CreateCacheFileDirectory(
    LPCTSTR pszPath
    )
{
    DBGTRACE((DM_SIDCACHE, DL_HIGH, TEXT("SidNameCache::CreateCacheFileDirectory")));
    BOOL bResult = TRUE;
    CString s(pszPath);     // Local copy we can play with.
    LPTSTR psz = (LPTSTR)s; // Ptr to C string.

    while(TEXT('\0') != *psz && bResult)
    {
        //
        // Find the next backslash (or end-of-string).
        //
        while(TEXT('\0') != *psz && TEXT('\\') != *psz)
        {
            psz++;
        }
        //
        // Replace backslash with a temporary NUL.
        //
        TCHAR chSaved = *psz;
        *psz = TEXT('\0');
        //
        // See if the directory already exists.
        //
        if ((DWORD)-1 == ::GetFileAttributes(s))
        {
            //
            // It doesn't.  Try to create it.
            //
            if (0 == ::CreateDirectory(s, NULL))
            {
                DBGERROR((TEXT("SIDCACHE - Error %d creating directory \"%s\""),
                         GetLastError(), (LPCTSTR)s));
                bResult = FALSE;
            }
        }
        //
        // Replace temp NUL with original backslash and advance ptr
        // to next character in path.
        //
        *psz++ = chSaved;
    }

    if (bResult)
    {
        //
        // Created directory. Set SYSTEM & HIDDEN attribute bits on the final
        // subdirectory ("\DiskQuota").
        //
        SetFileAttributes(pszPath,
                          GetFileAttributes(pszPath) | (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN));
    }

    return bResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::Lock

    Description: Obtains an exclusive lock on the cache.  This lock is
        system-wide so that multiple processes can access the cache files.

    Arguments: None.

    Returns:
        TRUE  - Exclusive lock obtained or the lock was abandoned.
                No matter how the lock was obtained, the caller should always
                check the validity of the index and data files before trusting
                their contents.
        FALSE - Lock could not be obtained in the required timeout period.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
SidNameCache::Lock(
    VOID
    )
{
    BOOL bResult = FALSE;
    DBGASSERT((NULL != m_hMutex));

    //
    // IMPORTANT:  Don't try handling thread messages with MsgWaitForMultipleObjects.
    //             This lock function can be called on the resolver's background
    //             thread.  If you suck up that thread's messages, it won't receive
    //             the WM_QUIT message commanding it to shutdown.
    //
    switch(WaitForSingleObject(m_hMutex, MUTEX_WAIT_TIMEOUT))
    {
        case WAIT_OBJECT_0:
        case WAIT_ABANDONED:
            bResult = TRUE;
            break;

        case WAIT_FAILED:
        case WAIT_TIMEOUT:
        default:
            break;
    }
    return bResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::ReleaseLock

    Description: Releases the exclusive lock on the cache.  This function must
        always be paired with a call to Lock().  Be careful of conditions
        which may throw an exception and bypass releasing the lock.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::ReleaseLock(
    VOID
    )
{
    DBGASSERT((NULL != m_hMutex));
    ReleaseMutex(m_hMutex);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::BeginTransaction

    Description: Called at the beginning of any "transaction" to obtain
        exclusive access to the cache and verify the cache files are valid.
        Although this is of course not true transaction processing, it provides
        a simple approximation that is sufficient for this cache implementation.
        Note that before returning, the files are "invalidated".  After the
        transaction is completed, the caller must call EndTransaction to
        release the exclusive lock and mark the files as "valid".

    Arguments: None.

    Returns:
        NO_ERROR            - Success.  Transaction can be carried out.
        ERROR_INVALID_DATA (hr) - Index or data file is invalid.  Caller should
            re-initialize both index and data files.
        ERROR_LOCK_FAILED (hr) - Could not obtain exclusive access to the
            index and data files.  Caller can either repeat the call or
            simply fail the cache access.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
SidNameCache::BeginTransaction(
    VOID
    )
{
    HRESULT hResult = NO_ERROR;
    if (Lock())
    {
        if (FilesAreValid())
        {
            InvalidateFiles();
        }
        else
        {
            ReleaseLock();
            hResult = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }
    }
    else
        hResult = HRESULT_FROM_WIN32(ERROR_LOCK_FAILED);

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::EndTransaction

    Description: Called at the end of any "transaction" to release
        exclusive access to the cache and validate the cache files.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::EndTransaction(
    VOID
    )
{
    //
    // If you hit this assertion, probably didn't call BeginTransaction
    // first.
    //
    DBGASSERT((!FilesAreValid()));

    ValidateFiles();
    ReleaseLock();
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::Clear

    Description: Resets both the index and data files to an empty state.
        Note that both files are reset to their initial size.

    Arguments: None.

    Returns:
        TRUE  - Files reset.
        FALSE - Couldn't obtain lock to reset files or another process
                is also using the cache.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
SidNameCache::Clear(
    VOID
    )
{
    DBGTRACE((DM_SIDCACHE, DL_HIGH, TEXT("SidNameCache::Clear")));
    BOOL bResult = FALSE;
    CacheAutoLock lock(*this);
    if (lock.Lock())
    {
        if (NULL != m_pIndexMgr)
        {
            delete m_pIndexMgr;
            m_pIndexMgr = NULL;
        }
        if (NULL != m_pRecordMgr)
        {
            delete m_pRecordMgr;
            m_pRecordMgr = NULL;
        }
        bResult = SUCCEEDED(Initialize(FALSE));  // Initialize cache.  Create new files.
    }
    return bResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::FilesAreValid

    Description: Examines the guid field in the index and data files.
        If the guids are non-zero and are equal, the files are considered
        "valid".  BeginTransaction sets each guid to all 0's while
        EndTransaction fills them with a new GUID.

    Arguments: None.

    Returns:
        TRUE/FALSE indicating file validity.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
SidNameCache::FilesAreValid(
    VOID
    )
{
    DBGASSERT((DATA_FILE_MAPPED));
    DBGASSERT((INDEX_FILE_MAPPED));

    GUID guidIndexFile;
    GUID guidDataFile;

    m_pIndexMgr->GetFileGUID(&guidIndexFile);
    if (GUID_Null != guidIndexFile)
    {
        m_pRecordMgr->GetFileGUID(&guidDataFile);
        return guidDataFile == guidIndexFile;
    }
    return FALSE; // At least one GUID was all 0's
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::ValidateFiles

    Description: Generates a new GUID and writes it to the header of the
        index and data files.  This should only be called when a transaction
        has been successfully completed.  EndTransaction calls this method
        to mark the files as "valid".

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::ValidateFiles(
    VOID
    )
{
    DBGASSERT((DATA_FILE_MAPPED));
    DBGASSERT((INDEX_FILE_MAPPED));

    GUID guid;
    CoCreateGuid(&guid);

    m_pRecordMgr->SetFileGUID(&guid);
    m_pIndexMgr->SetFileGUID(&guid);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::InvalidateFiles

    Description: Sets the guid field in each file to all 0's.   This marks
        a file as "invalid".  BeginTransaction calls this.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::InvalidateFiles(
    VOID
    )
{
    DBGASSERT((DATA_FILE_MAPPED));
    DBGASSERT((INDEX_FILE_MAPPED));

    m_pRecordMgr->SetFileGUID(&GUID_Null);
    m_pIndexMgr->SetFileGUID(&GUID_Null);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::Lookup

    Description: Given a user SID, this method locates the corresponding
        data record in the cache and returns the requested information.
        (i.e. container, name, full name).  Several conditions will cause
        the lookup to fail.
            1. SID not found in cache.
            2. Can't get exclusive lock on cache.
            3. Index or data file (or both) are invalid.
            4. Record found but expired.

    Arguments:
        pKeySid - Address of SID to use as lookup key.

        ppszContainer [optional] - Address of pointer variable to receive the
            address of buffer containing the "container" name string. Caller is
            responsible for freeing the buffer with delete[].
            May be NULL if the container is not desired.

        ppszLogonName [optional] - Address of pointer variable to receive the
            address of buffer containing logon name string. Caller is
            responsible for freeing the buffer with delete[].
            May be NULL if logon name is not desired.

        ppszDisplayName [optional] - Address of pointer variable to receive the
            address of buffer containing account display string. Caller is
            responsible for freeing the buffer with delete[].
            May be NULL if display name is not desired.

    Returns:
        NO_ERROR                  - Success.
        ERROR_FILE_NOT_FOUND (hr) - Sid not found in cache.
        ERROR_LOCK_FAILED (hr)    - Couldn't get exclusive lock on cache.
        ERROR_INVALID_DATA (hr)   - Index or data file is invalid.

    Exceptions: OutOfMemory


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
SidNameCache::Lookup(
    PSID pKeySid,
    LPTSTR *ppszContainer,
    LPTSTR *ppszLogonName,
    LPTSTR *ppszDisplayName
    )
{
    DBGTRACE((DM_SIDCACHE, DL_MID, TEXT("SidNameCache::Lookup [SID]")));
    DBGASSERT((NULL != pKeySid));

    HRESULT hResult = BeginTransaction();

    if (SUCCEEDED(hResult))
    {
        try
        {
            DWORD iBlock = m_pIndexMgr->Lookup(pKeySid);

            if ((DWORD)-1 != iBlock)
            {
                if (!m_pRecordMgr->RecordExpired(iBlock))
                {
                    PSID pSid = NULL;
                    //
                    // This can throw OutOfMemory.
                    //
                    hResult = m_pRecordMgr->Retrieve(iBlock,
                                                     &pSid,
                                                     ppszContainer,
                                                     ppszLogonName,
                                                     ppszDisplayName);
                    if (SUCCEEDED(hResult))
                        DBGASSERT((EqualSid(pSid, pKeySid)));

                    if (NULL != pSid)
                        delete[] pSid;
                }
                else
                {
                    //
                    // Record is outdated.  Delete it from the cache.
                    // Returning "not found" will cause the caller to get
                    // a fresh one from the domain controller - which will
                    // then again be added to the cache.
                    //
                    DBGPRINT((DM_SIDCACHE, DL_HIGH,
                             TEXT("SIDCACHE - Record at block %d has expired."),
                             iBlock));

                    m_pIndexMgr->FreeEntry(pKeySid);
                    m_pRecordMgr->FreeRecord(iBlock);
                    hResult = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                }
            }
            else
                hResult = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);  // SID not in cache.
        }
        catch(CAllocException &e)
        {
            EndTransaction(); // Files are still valid.
            throw;
        }
        catch(...)
        {
            ReleaseLock();  // Files may not be valid.
            throw;
        }

        EndTransaction();
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::Lookup

    Description: Given a user's logon name name, this method locates
        the corresponding SID in the cache.  All comments in the previous
        (above) version of this method apply.

    Arguments:
        pszLogonName - Address of account logon name string.

        ppSid - Address of pointer variable to receive the address of buffer
            containing the SID. Caller is responsible for freeing the buffer
            with delete[].

    Returns:
        See list in previous method.

    Exception: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
    03/18/98    Replaced domain\name key arguments with single       BrianAu
                logon name key.
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
SidNameCache::Lookup(
    LPCTSTR pszLogonName,
    PSID *ppSid
    )
{
    DBGTRACE((DM_SIDCACHE, DL_MID, TEXT("SidNameCache::Lookup [name]")));
    DBGASSERT((NULL != pszLogonName));
    DBGASSERT((NULL != ppSid));

    HRESULT hResult = BeginTransaction();

    if (SUCCEEDED(hResult))
    {
        try
        {
            //
            // Can throw OutOfMemory.
            //
            DWORD iBlock = m_pIndexMgr->Lookup(pszLogonName);

            if ((DWORD)-1 != iBlock)
            {
                //
                // Can throw OutOfMemory.
                //
                hResult = m_pRecordMgr->Retrieve(iBlock,
                                                 ppSid,
                                                 NULL,
                                                 NULL,
                                                 NULL);

                if (m_pRecordMgr->RecordExpired(iBlock))
                {
                    //
                    // Record is outdated.  Delete it from the cache.
                    // Returning "not found" will cause the caller to get
                    // a fresh one from the domain controller - which will
                    // then again be added to the cache.
                    //
                    DBGASSERT((NULL != *ppSid));
                    m_pIndexMgr->FreeEntry(*ppSid);
                    m_pRecordMgr->FreeRecord(iBlock);
                    delete[] *ppSid;
                    *ppSid = NULL;
                    hResult = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                }
            }
            else
                hResult = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);  // SID not in cache.
        }
        catch(CAllocException& e)
        {
            EndTransaction();  // Files are still valid.
            throw;
        }
        catch(...)
        {
            ReleaseLock();  // Files may not be valid.
            throw;
        }

        EndTransaction();
    }

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::Add

    Description: Add user's information to the cache.  Information consists
        of the SID (key), container name, account logon name and
        account display name.

    Arguments:
        pSid - Address of user's SID.

        pszContainer - Address of account container name string.
            i.e. "REDMOND" or "ntdev.microsoft.com\US-SOS ....."

        pszLogonName - Address of account logon name string.
            i.e. "REDMOND\brianau" or "brianau@microsoft.com"

        pszDisplayName - Address of display name string.
            i.e. "Brian Aust"

    Returns:
        NO_ERROR                  - Success.
        S_FALSE                   - Already exists in cache.  Not added.
        ERROR_LOCK_FAILED (hr)    - Couldn't get exclusive lock on cache.
        ERROR_INVALID_DATA (hr)   - Index or data file is invalid.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
SidNameCache::Add(
    PSID pSid,
    LPCTSTR pszContainer,
    LPCTSTR pszLogonName,
    LPCTSTR pszDisplayName
    )
{
    DBGTRACE((DM_SIDCACHE, DL_MID, TEXT("SidNameCache::Add")));

    DBGASSERT((NULL != pSid));
    DBGASSERT((NULL != pszContainer));
    DBGASSERT((NULL != pszLogonName));
    DBGASSERT((NULL != pszDisplayName));

    HRESULT hResult = BeginTransaction();

    if (SUCCEEDED(hResult))
    {
        try
        {
            //
            // Can throw OutOfMemory.
            //
            if ((DWORD)-1 == m_pIndexMgr->Lookup(pSid))
            {
                DWORD iBlock = m_pRecordMgr->Store(pSid,
                                                   pszContainer,
                                                   pszLogonName,
                                                   pszDisplayName);

                if ((DWORD)-1 != iBlock)
                {
                    m_pIndexMgr->Add(pSid, iBlock);
                    hResult = NO_ERROR;
                }
            }
            else
                hResult = S_FALSE;  // Already exists. Not a failure.
        }
        catch(CAllocException& e)
        {
            EndTransaction(); // Files are still valid.
            throw;
        }
        catch(...)
        {
            ReleaseLock(); // Files may not be valid.
            throw;
        }

        EndTransaction();
    }

    return hResult;
}



//***********************************************************************************
//***********************************************************************************
//   I N D E X   F I L E   M A N A G E R
//***********************************************************************************
//***********************************************************************************


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::IndexMgr

    Description: Index manager constructor.

    Arguments:
        refCache - Reference to containing cache object.  Used to call
            record manager and cache manager public methods.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
SidNameCache::IndexMgr::IndexMgr(
    SidNameCache& refCache
    ) : m_refCache(refCache),
        m_pFileHdr(NULL),
        m_hFile(NULL),
        m_hFileMapping(NULL)
{
    DBGTRACE((DM_SIDCACHE, DL_MID, TEXT("SidNameCache::SidNameCache::IndexMgr")));
    //
    // Nothing to do.
    //
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::~IndexMgr

    Description: Index manager destructor.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
SidNameCache::IndexMgr::~IndexMgr(
    VOID
    )
{
    DBGTRACE((DM_SIDCACHE, DL_MID, TEXT("SidNameCache::SidNameCache::~IndexMgr")));
    CloseIndexFile();
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::Initialize

    Description: Initializes a new index manager object.  If both cBuckets
        and cMaxEntries are 0 (default), existing cache files are opened.
        Otherwise, new cache files are created.

    Arguments:
        pszFile - Address of full path for new file.

        cBuckets - Number of hash table buckets in index file.  Should be
            prime.

        cMaxEntries - Initial max number of entries for the index.  Note
            that the index file grows automatically as required.

    Returns: Address of mapped file or NULL on failure.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LPBYTE
SidNameCache::IndexMgr::Initialize(
    LPCTSTR pszFile,
    DWORD cBuckets,
    DWORD cMaxEntries
    )
{
    DBGTRACE((DM_SIDCACHE, DL_HIGH, TEXT("SidNameCache::IndexMgr::Initialize")));
    DBGASSERT((NULL != pszFile));

    //
    // Store the file name in our CString object.
    //
    m_strFileName = pszFile;

    if (0 == cBuckets && 0 == cMaxEntries)
    {
        //
        // Initialize manager using existing cache files.
        //
        m_pFileHdr = (PINDEX_FILE_HDR)OpenIndexFile(pszFile);
        if (NULL != m_pFileHdr)
        {
            if (FILE_VERSION != m_pFileHdr->dwVersion ||
                INDEX_FILE_SIGNATURE != m_pFileHdr->dwSignature)
            {
                //
                // This version of the software doesn't understand this
                // version of the file or file has an invalid signature.
                // Don't take any chances.  We'll just create a new one.
                //
                DBGERROR((TEXT("SIDCACHE - Index file is invalid or incorrect version. A new index file will be created.")));

                CloseIndexFile();
                m_pFileHdr = NULL;
            }
        }
    }
    else
    {
        //
        // Initialize manager by creating new cache files.
        //
        ULARGE_INTEGER uliFileSize;
        uliFileSize.QuadPart = FileSize(cMaxEntries, cBuckets);

        m_pFileHdr = (PINDEX_FILE_HDR)CreateIndexFile(pszFile,
                                                      uliFileSize.HighPart,
                                                      uliFileSize.LowPart);
        if (NULL != m_pFileHdr)
        {
            InitNewIndexFile(cBuckets, cMaxEntries);
        }
    }
    return (LPBYTE)m_pFileHdr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::CreateIndexFile

    Description: Creates and initializes a new index file.

    Arguments:
        pszFile - Address of full path for new file.

        cbFileHigh/Low - Size of file in bytes.

    Returns: Address of mapped file or NULL on failure.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LPBYTE
SidNameCache::IndexMgr::CreateIndexFile(
    LPCTSTR pszFile,
    DWORD cbFileHigh,
    DWORD cbFileLow
    )
{
    DBGTRACE((DM_SIDCACHE, DL_LOW, TEXT("SidNameCache::IndexMgr::CreateIndexFile")));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("\tFile: \"%s\""), pszFile ? pszFile : TEXT("<null>")));

    DBGASSERT((NULL != pszFile));

    CloseIndexFile();  // Make sure any existing index file is closed.

    g_pbMappedIndexFile = SidNameCache::OpenMappedFile(
                                pszFile,
                                g_pszIndexFileMapping,
                                CREATE_ALWAYS,
                                cbFileHigh,
                                cbFileLow,
                                &m_hFile,
                                &m_hFileMapping);

    return g_pbMappedIndexFile;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::OpenIndexFile

    Description: Opens an existing index file.

    Arguments:
        pszFile - Address of full path for new file.

    Returns: Address of mapped file or NULL on failure.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LPBYTE
SidNameCache::IndexMgr::OpenIndexFile(
    LPCTSTR pszFile
    )
{
    DBGTRACE((DM_SIDCACHE, DL_LOW, TEXT("SidNameCache::IndexMgr::OpenIndexFile")));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("\tFile: \"%s\""), pszFile ? pszFile : TEXT("<null>")));
    DBGASSERT((NULL != pszFile));

    CloseIndexFile();  // Make sure any existing index file is closed.

    g_pbMappedIndexFile = SidNameCache::OpenMappedFile(
                                pszFile,
                                g_pszIndexFileMapping,
                                OPEN_EXISTING,
                                0,
                                0,
                                &m_hFile,
                                &m_hFileMapping);
    return g_pbMappedIndexFile;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::CloseIndexFile

    Description: Closes the current index mapping and file.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::IndexMgr::CloseIndexFile(
    VOID
    )
{
    DBGTRACE((DM_SIDCACHE, DL_LOW, TEXT("SidNameCache::IndexMgr::CloseIndexFile")));
    if (NULL != g_pbMappedIndexFile)
    {
        UnmapViewOfFile(g_pbMappedIndexFile);
        g_pbMappedIndexFile = NULL;
        m_pFileHdr = NULL;
    }
    if (NULL != m_hFileMapping)
    {
        CloseHandle(m_hFileMapping);
        m_hFileMapping = NULL;
    }
    if (NULL != m_hFile && INVALID_HANDLE_VALUE != m_hFile)
    {
        CloseHandle(m_hFile);
        m_hFile = NULL;
    }
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::GrowIndexFile

    Description: Increases the size of the current index file.

    Arguments:
        cGrowEntries - Make room for this many more entries.  Note that the
            size of the hash table is fixed.  If we were to allow this to
            change, it would invalidate any existing hash values in the
            table (hash code is a function of the SID and table size).

    Returns: Address of mapped file or NULL on failure.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LPBYTE
SidNameCache::IndexMgr::GrowIndexFile(
    DWORD cGrowEntries
    )
{
    DBGTRACE((DM_SIDCACHE, DL_LOW, TEXT("SidNameCache::IndexMgr::GrowIndexFile")));
    DBGASSERT((INDEX_FILE_MAPPED));

    DWORD cOldMaxEntries = m_pFileHdr->cMaxEntries;
    DWORD cNewMaxEntries = cOldMaxEntries + cGrowEntries;

    DBGPRINT((DM_SIDCACHE, DL_MID,
             TEXT("Growing SID cache index %d -> %d entries."),
             cOldMaxEntries, cNewMaxEntries));

    //
    // Open the existing file and map with a new larger size.
    // Must calc new size BEFORE closing current index file so that m_pFileHdr
    // is still valid.
    //
    ULARGE_INTEGER uliFileSize;
    uliFileSize.QuadPart = FileSize(cNewMaxEntries, m_pFileHdr->cBuckets);

    CloseIndexFile();

    g_pbMappedIndexFile = SidNameCache::OpenMappedFile(
                                m_strFileName,
                                g_pszIndexFileMapping,
                                OPEN_EXISTING,
                                uliFileSize.HighPart,
                                uliFileSize.LowPart,
                                &m_hFile,
                                &m_hFileMapping);

    m_pFileHdr = (PINDEX_FILE_HDR)g_pbMappedIndexFile;

    if (NULL != g_pbMappedIndexFile)
    {
        m_pFileHdr->cMaxEntries = cNewMaxEntries;

        //
        // Growing the index only expands the number of index
        // pool entries. The index hash table is left alone.
        // Good reason to make it large to start with.  If we changed
        // the hash table size, existing hash codes would be
        // invalid.
        //
        PINDEX_ENTRY pEntry = m_pFileHdr->pEntries + cOldMaxEntries;
        for (UINT i = 0; i < cGrowEntries; i++)
        {
            AddEntryToFreeList(pEntry++);
        }
        DBGPRINT((DM_SIDCACHE, DL_HIGH, TEXT("SIDCACHE - Index growth complete.")));
    }

    return g_pbMappedIndexFile;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::InitNewIndexFile

    Description: Initializes a new index file filling in the header information
        and clearing the index entries.

    Arguments:
        cBuckets - Number of hash table buckets in index file.  Should be
            prime.

        cMaxEntries - Initial max number of entries for the index.  Note
            that the index file grows automatically as required.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::IndexMgr::InitNewIndexFile(
    DWORD cBuckets,
    DWORD cMaxEntries
    )
{
    DBGTRACE((DM_SIDCACHE, DL_LOW, TEXT("SidNameCache::IndexMgr::InitNewIndexFile")));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("\tcBuckets = %d, cMaxEntries = %d"),
               cBuckets, cMaxEntries));
    DBGASSERT((INDEX_FILE_MAPPED));

    m_pFileHdr->dwSignature   = INDEX_FILE_SIGNATURE;
    m_pFileHdr->dwVersion     = FILE_VERSION;
    m_pFileHdr->cBuckets      = cBuckets;
    m_pFileHdr->cMaxEntries   = cMaxEntries;
    m_pFileHdr->pBuckets      = (PINDEX_ENTRY *)(sizeof(INDEX_FILE_HDR));
    m_pFileHdr->pEntries      = (PINDEX_ENTRY)(m_pFileHdr->pBuckets + cBuckets);

    //
    // Initialize the hash table and return all entries to the free list.
    //
    Clear();
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::Clear

    Description: Fills the hash table with NULL pointers and returns all
        entry nodes to the free list.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::IndexMgr::Clear(
    VOID
    )
{
    DBGTRACE((DM_SIDCACHE, DL_LOW, TEXT("SidNameCache::IndexMgr::Clear")));
    DBGASSERT((INDEX_FILE_MAPPED));

    m_pFileHdr->cEntries = 0;
    SetFileGUID(&GUID_Null);

    //
    // Initialize all hash buckets to NULL.
    //
    DBGASSERT((0 < m_pFileHdr->cBuckets));
    ZeroMemory(NDX_BASED_CAST(BYTE, m_pFileHdr->pBuckets),
               m_pFileHdr->cBuckets * sizeof(PINDEX_ENTRY *));

    //
    // Return all index entry nodes to the free list.
    //
    PINDEX_ENTRY pEntry = m_pFileHdr->pEntries;
    DBGASSERT((0 < m_pFileHdr->cMaxEntries));
    for (UINT i = 0; i < m_pFileHdr->cMaxEntries; i++)
    {
        //
        // We're iterating through all entries.  No need to detach first.
        //
        AddEntryToFreeList(pEntry++);
    }
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::SetFileGUID
    Function: SidNameCache::IndexMgr::GetFileGUID

    Description: These functions manipulate the guid field in the index file's
        header.
        The GUID is used to ensure integrity between
        the data and index files.  Before any change to either file, the
        GUID's are both set to 0.  When the change operation is complete,
        a new GUID is generated and written to both files.  Therefore, before
        any transaction, we can validate the data and index files by reading
        and comparing GUIDs.  If the GUIDs are not 0 and are equal, the
        file can be assumed to be valid.

    Arguments:
        pguid - Address of source or destination GUID.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::IndexMgr::SetFileGUID(
    const GUID *pguid
    )
{
    DBGASSERT((INDEX_FILE_MAPPED));
    DBGASSERT((NULL != pguid));
    CopyMemory(&m_pFileHdr->guid, pguid, sizeof(GUID));
}

VOID
SidNameCache::IndexMgr::GetFileGUID(
    LPGUID pguid
    )
{
    DBGASSERT((INDEX_FILE_MAPPED));
    DBGASSERT((NULL != pguid));
    CopyMemory(pguid, &m_pFileHdr->guid, sizeof(GUID));
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::AllocEntry

    Description: Allocates an entry from the free list.

    Arguments: None.

    Returns:
        Address of new entry node or NULL if free list is empty.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
PINDEX_ENTRY
SidNameCache::IndexMgr::AllocEntry(
    VOID
    )
{
    DBGASSERT((INDEX_FILE_MAPPED));
    PINDEX_ENTRY pEntry = m_pFileHdr->pFirstFree;

    if (NULL != pEntry)
    {
        NDX_BASED(INDEX_ENTRY) *pBasedEntry = NDX_BASED_CAST(INDEX_ENTRY, pEntry);

        //
        // Unlink the entry from the free list.
        //
        m_pFileHdr->pFirstFree = pBasedEntry->pNext;

        //
        // Clear it's "prev" and "next" pointers.
        //
        pBasedEntry->pNext = pBasedEntry->pPrev = NULL;

        if (NULL != m_pFileHdr->pFirstFree)
        {
            //
            // If there is at least one entry in the free list, set the "prev"
            // pointer of the new "first" entry to NULL.
            //
            pBasedEntry = NDX_BASED_CAST(INDEX_ENTRY, m_pFileHdr->pFirstFree);
            pBasedEntry->pPrev = NULL;
        }

        m_pFileHdr->cEntries++;
    }
    return pEntry;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::FreeEntry

    Description: Removes an entry from it's current list and returns it to the
        free list.  Two versions are provided.  One accepts a SID as an entry
        identifier, the other accepts the address of the index entry.

    Arguments:
        pSid - Address of SID associated with entry to be free'd.

        pEntry - Address of index entry to be free'd

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::IndexMgr::FreeEntry(
    PSID pSid
    )
{
    DBGASSERT((NULL != pSid));
    PINDEX_ENTRY pEntry = Find(pSid);
    if (NULL != pEntry)
    {
        FreeEntry(pEntry);
    }
}


VOID
SidNameCache::IndexMgr::FreeEntry(
    PINDEX_ENTRY pEntry
    )
{
    DBGASSERT((NULL != pEntry));
    DetachEntry(pEntry);
    AddEntryToFreeList(pEntry);

    m_pFileHdr->cEntries--;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::AddEntryToFreeList

    Description: Returns a detached index entry to the free list.

    Arguments:
        pEntry - Address of index entry to be added to free list.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::IndexMgr::AddEntryToFreeList(
    PINDEX_ENTRY pEntry
    )
{
    DBGASSERT((INDEX_FILE_MAPPED));
    DBGASSERT((NULL != pEntry));

    NDX_BASED(INDEX_ENTRY) *pBased = NDX_BASED_CAST(INDEX_ENTRY, pEntry);

    //
    // Insert the node at the head of the free list.
    // Note that double-linking isn't necessary in the free list
    // (we always add and remove free list entries at the head)
    // therefore we don't need to set the next node's "prev" pointer.
    //
    pBased->iBucket        = (DWORD)-1;
    pBased->iBlock         = (DWORD)-1;
    pBased->pPrev          = NULL;
    pBased->pNext          = m_pFileHdr->pFirstFree;
    m_pFileHdr->pFirstFree = pEntry;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::DetachEntry

    Description: Detaches an index entry from its current list.
        Note that this function assumes that the node exists in a valid
        linked list of nodes.  Do not call this function on an uninitialized
        index entry.

    Arguments:
        pEntry - Address of index entry to be detached.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
PINDEX_ENTRY
SidNameCache::IndexMgr::DetachEntry(
    PINDEX_ENTRY pEntry
    )
{
    DBGASSERT((INDEX_FILE_MAPPED));
    DBGASSERT((NULL != pEntry));

    NDX_BASED(INDEX_ENTRY) *pBased = NDX_BASED_CAST(INDEX_ENTRY, pEntry);
    NDX_BASED(INDEX_ENTRY) *pBasedNext;
    NDX_BASED(INDEX_ENTRY) *pBasedPrev;

    //
    // Unlink the entry from it's list.
    //
    if (NULL != pBased->pPrev)
    {
        pBasedPrev        = NDX_BASED_CAST(INDEX_ENTRY, pBased->pPrev);
        pBasedPrev->pNext = pBased->pNext;
    }
    if (NULL != pBased->pNext)
    {
        pBasedNext        = NDX_BASED_CAST(INDEX_ENTRY, pBased->pNext);
        pBasedNext->pPrev = pBased->pPrev;
    }
    //
    // If we're detaching the entry that's attached to the hash array element,
    // adjust the element's value.
    //
    if (GetHashBucketValue(pBased->iBucket) == pEntry)
        SetHashBucketValue(pBased->iBucket, pBased->pNext);

    pBased->pNext = pBased->pPrev = NULL;

    return pEntry;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::GetHashBucketValue

    Description: Returns the address of the first index entry in a hash bucket.

    Arguments:
        iBucket - Array index of bucket in hash table array.

    Returns:
        Address of first entry in bucket's entry list.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
PINDEX_ENTRY
SidNameCache::IndexMgr::GetHashBucketValue(
    DWORD iBucket
    )
{
    DBGASSERT((INDEX_FILE_MAPPED));
    DBGASSERT((iBucket < m_pFileHdr->cBuckets));

    NDX_BASED(PINDEX_ENTRY) *pBased = NDX_BASED_CAST(PINDEX_ENTRY, m_pFileHdr->pBuckets + iBucket);
    return *pBased;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::SetHashBucketValue

    Description: Sets the address of the first index entry in a hash bucket.
        OK to set it as NULL.

    Arguments:
        iBucket - Array index of bucket in hash table array.

        pEntry - Address of entry node.

    Returns:
        Address of first entry in bucket's entry list.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::IndexMgr::SetHashBucketValue(
    DWORD iBucket,
    PINDEX_ENTRY pEntry
    )
{
    DBGASSERT((INDEX_FILE_MAPPED));
    DBGASSERT((iBucket < m_pFileHdr->cBuckets));
    //
    // pEntry == NULL is OK.
    //

    NDX_BASED(PINDEX_ENTRY) *pBased = NDX_BASED_CAST(PINDEX_ENTRY,
                                                     m_pFileHdr->pBuckets + iBucket);
    *pBased = pEntry;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::Find (3 overloaded methods)

    Description: Given either a SID, a SID and hash value, or a user logon name,
        this method returns the address of the index entry
        representing that cache entry.

    Arguments:
        pKeySid - Address of SID to use as a lookup key.

        dwHashCode - Result of calling Hash(pKeySid).

        pszKeyLogonName - Address of account logon name string.

    Returns:
        Address of index entry representing the user.
        NULL if not found.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
PINDEX_ENTRY
SidNameCache::IndexMgr::Find(
    PSID pKeySid
    )
{
    DBGASSERT((INDEX_FILE_MAPPED));
    DBGASSERT((NULL != pKeySid));

    return Find(pKeySid, Hash(pKeySid));  // Can throw OutOfMemory.
}


PINDEX_ENTRY
SidNameCache::IndexMgr::Find(
    PSID pKeySid,
    DWORD dwHashCode
    )
{
    DBGASSERT((INDEX_FILE_MAPPED));
    DBGASSERT((NULL != pKeySid));

    BOOL bFound         = FALSE;
    PINDEX_ENTRY pEntry = GetHashBucketValue(dwHashCode);
    while(NULL != pEntry && !bFound)
    {
        PSID pSid = NULL;
        NDX_BASED(INDEX_ENTRY) *pBased = NDX_BASED_CAST(INDEX_ENTRY, pEntry);

        //
        // This can throw OutOfMemory.
        //
        m_refCache.m_pRecordMgr->Retrieve(pBased->iBlock,
                                          &pSid,
                                          NULL,
                                          NULL,
                                          NULL);

        if (EqualSid(pKeySid, pSid))
            bFound = TRUE;
        else
            pEntry = pBased->pNext;

        delete[] pSid;
    }

    return pEntry;
}


//
// This version of Find() performs a linear search of the index to locate
// the specified logon name.  The cache is currently indexed only
// on user SID because this is the only key used for very rapid lookups.
// The cache implementation could be easily extended to include a
// logon name index.  All that's needed is a second hash bucket array,
// a hash-on-name function and some adjustments to the placement of
// file data.  I just don't think the benefit is worth the cost.
//
PINDEX_ENTRY
SidNameCache::IndexMgr::Find(
    LPCTSTR pszKeyLogonName
    )
{
    DBGASSERT((INDEX_FILE_MAPPED));
    DBGASSERT((NULL != pszKeyLogonName));

    BOOL bFound         = FALSE;
    PINDEX_ENTRY pEntry = NULL;

    for (UINT i = 0; !bFound && (i < m_pFileHdr->cBuckets); i++)
    {
        pEntry = GetHashBucketValue(i);
        while(NULL != pEntry && !bFound)
        {
            array_autoptr<TCHAR> ptrLogonName;

            NDX_BASED(INDEX_ENTRY) *pBased = NDX_BASED_CAST(INDEX_ENTRY, pEntry);

            //
            // This can throw OutOfMemory.
            //
            m_refCache.m_pRecordMgr->Retrieve(pBased->iBlock,
                                        NULL,
                                        NULL,  // no container.
                                        ptrLogonName.getaddr(),
                                        NULL); // no display name

            DBGASSERT((NULL != ptrLogonName.get()));

            if (0 == lstrcmpi(ptrLogonName.get(), pszKeyLogonName))
            {
                bFound = TRUE;
            }
            else
                pEntry = pBased->pNext;
        }
    }
    return pEntry;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::Lookup (2 overloaded methods)

    Description: Given either a SID or an account logon name,
        this method returns the starting block index of the corresponding
        record in the data file.

    Arguments:
        pSid - Address of SID to use as a lookup key.

        pszLogonName - Address of account logon name string.

    Returns:
        Index of the starting block for the record in the data file.
        (DWORD)-1 if the record is not found.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DWORD
SidNameCache::IndexMgr::Lookup(
    PSID pSid
    )
{
    DBGASSERT((NULL != pSid));

    //
    // This can throw OutOfMemory.
    //
    PINDEX_ENTRY pEntry = Find(pSid, Hash(pSid));

    if (NULL != pEntry)
    {
        NDX_BASED(INDEX_ENTRY) *pBased = NDX_BASED_CAST(INDEX_ENTRY, pEntry);
        return pBased->iBlock;
    }

    return (DWORD)-1;
}



DWORD
SidNameCache::IndexMgr::Lookup(
    LPCTSTR pszLogonName
    )
{
    DBGASSERT((NULL != pszLogonName));

    //
    // This can throw OutOfMemory.
    //
    PINDEX_ENTRY pEntry = Find(pszLogonName);

    if (NULL != pEntry)
    {
        NDX_BASED(INDEX_ENTRY) *pBased = NDX_BASED_CAST(INDEX_ENTRY, pEntry);
        return pBased->iBlock;
    }

    return (DWORD)-1;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::Add

    Description: Adds a SID/data-file-index pair to the index file.  This
        entry can later be used to locate the SID's record in the data file.

    Arguments:
        pSid - Address of SID to use as a lookup key.

        iBlock - Index of the starting block for the SID's record in the
            data file.

    Returns: Address of the item's new index entry.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
PINDEX_ENTRY
SidNameCache::IndexMgr::Add(
    PSID pSid,
    DWORD iBlock
    )
{
    DWORD dwHashCode    = Hash(pSid);
    PINDEX_ENTRY pEntry = Find(pSid, dwHashCode); // Can throw OutOfMemory.

    //
    // Don't create duplicate entries.
    //
    if (NULL == pEntry)
    {
        //
        // Try to allocate an index entry from the free list.
        //
        pEntry = AllocEntry();
        if (NULL == pEntry)
        {
            //
            // Grow the index file and try again.
            //
            GrowIndexFile(INDEX_FILE_GROW_ENTRIES);
            pEntry = AllocEntry();
        }
        if (NULL != pEntry)
        {
            NDX_BASED(INDEX_ENTRY) *pBasedEntry = NDX_BASED_CAST(INDEX_ENTRY, pEntry);
            NDX_BASED(INDEX_ENTRY) *pBasedNext;

            //
            // Fill in the members of the new entry.
            //
            pBasedEntry->iBucket = dwHashCode;
            pBasedEntry->iBlock  = iBlock;
            pBasedEntry->pNext   = GetHashBucketValue(dwHashCode);
            pBasedEntry->pPrev   = NULL;
            //
            // Now insert it at the head of the hash bucket's entry list.
            //
            if (NULL != pBasedEntry->pNext)
            {
                pBasedNext = NDX_BASED_CAST(INDEX_ENTRY, pBasedEntry->pNext);
                pBasedNext->pPrev = pEntry;
            }
            SetHashBucketValue(dwHashCode, pEntry);
        }
    }

    return pEntry;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::Hash

    Description: Given a SID, this method calculates a hash value to be used
        as an offset into the index's hash table bucket array.  The
        algorithm simply sums the value of the SID's bytes.  The resulting
        hash code is this sum modulo the size of the hash table.  For this
        simple algorithm to be effective, it is important that the hash
        table size be a prime number.

        Here's some representative primes: 101, 503, 1009, 5003, 10007

    Arguments:
        pSid - Address of SID to use as an index lookup key.

    Returns: Hashed SID.  The value will be between 0 and m_pFileHdr->cBuckets.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DWORD
SidNameCache::IndexMgr::Hash(
    PSID pSid
    )
{
    DBGASSERT((INDEX_FILE_MAPPED));
    DWORD dwCode   = 0;
    PBYTE pbSid    = (PBYTE)pSid;
    PBYTE pbEndSid = pbSid + GetLengthSid(pSid);

    for ( ;pbSid < pbEndSid; pbSid++)
        dwCode += *pbSid;

    return dwCode % m_pFileHdr->cBuckets;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IndexMgr::Dump [DEBUG only]

    Description: Dumps the contents of the index file to the debugger output.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#if DBG
VOID
SidNameCache::IndexMgr::Dump(
    VOID
    )
{
    UINT i, j;
    DBGASSERT((INDEX_FILE_MAPPED));

    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("Dumping SidNameCache IndexMgr at 0x%p"), this));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("  Base...............: 0x%p"), g_pbMappedIndexFile));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("  m_pFileHdr.........: 0x%p"), m_pFileHdr));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("    dwSignature......: 0x%08X"), (DWORD)m_pFileHdr->dwSignature));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("    dwVersion........: 0x%08X"), (DWORD)m_pFileHdr->dwVersion));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("    cBuckets.........: %d"),     (DWORD)m_pFileHdr->cBuckets));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("    cMaxEntries......: %d"),     (DWORD)m_pFileHdr->cMaxEntries));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("    cEntries.........: %d"),     (DWORD)m_pFileHdr->cEntries));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("    pBuckets.........: 0x%p"), m_pFileHdr->pBuckets));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("    pEntries.........: 0x%p"), m_pFileHdr->pEntries));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("    pFirstFree.......: 0x%p"), m_pFileHdr->pFirstFree));

    for (i = 0; i < m_pFileHdr->cBuckets; i++)
    {
        PINDEX_ENTRY pEntry = GetHashBucketValue(i);
        DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("  Bucket[%03d] = 0x%p"), i, pEntry));

        while(NULL != pEntry)
        {
            NDX_BASED(INDEX_ENTRY) *pBased = NDX_BASED_CAST(INDEX_ENTRY, pEntry);
            DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("     Bkt = %3d  P = 0x%08X  N = 0x%08X  Blk = %d"),
                       pBased->iBucket,
                       pBased->pPrev,
                       pBased->pNext,
                       pBased->iBlock));

            pEntry = pBased->pNext;
        }
    }

    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("  FreeList")));
    PINDEX_ENTRY pEntry = m_pFileHdr->pFirstFree;
    while(NULL != pEntry)
    {
        NDX_BASED(INDEX_ENTRY) *pBased = NDX_BASED_CAST(INDEX_ENTRY, pEntry);
        DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("     Bkt = %3d  P = 0x%08X  N = 0x%08X  Blk = %d"),
                   pBased->iBucket,
                   pBased->pPrev,
                   pBased->pNext,
                   pBased->iBlock));

        pEntry = pBased->pNext;
    }
}
#endif


//***********************************************************************************
//***********************************************************************************
//   D A T A    F I L E    M A N A G E R
//***********************************************************************************
//***********************************************************************************

//
// Default cache record life values in days.
//
const DWORD DEF_REC_LIFE_MIN = 30;
const DWORD DEF_REC_LIFE_MAX = 60;

///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::RecordMgr

    Description: Record manager constructor.

    Arguments:
        refCache - Reference to containing cache object.  Used to call
            index manager and cache manager public methods.

    Returns: Nothing.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
SidNameCache::RecordMgr::RecordMgr(
    SidNameCache& refCache
    ) : m_refCache(refCache),
        m_pFileHdr(NULL),
        m_hFile(NULL),
        m_hFileMapping(NULL),
        m_cDaysRecLifeMin(DEF_REC_LIFE_MIN),
        m_cDaysRecLifeRange(DEF_REC_LIFE_MAX - DEF_REC_LIFE_MIN)
{
    DWORD cDaysRecLifeMax = DEF_REC_LIFE_MAX;

    DBGTRACE((DM_SIDCACHE, DL_MID, TEXT("SidNameCache::SidNameCache::RecordMgr")));

    //
    // Retrieve the min/max record life (days) from the registry.
    // Note that we store record life min and range since those are
    // what's used in the record life calculation.  Seems more
    // self-explanatory to store min/max in the registry rather than
    // min/range.
    //
    RegKey key(HKEY_CURRENT_USER, REGSTR_KEY_DISKQUOTA);
    if (key.Open(KEY_WRITE, true))
    {
        if (FAILED(key.GetValue(g_szSidCacheRecLifeMin, &m_cDaysRecLifeMin)) ||
            65536 <= m_cDaysRecLifeMin)
        {
            m_cDaysRecLifeMin = DEF_REC_LIFE_MIN; // Default;
            key.SetValue(g_szSidCacheRecLifeMin, m_cDaysRecLifeMin);
        }
        if (FAILED(key.GetValue(g_szSidCacheRecLifeMax, &cDaysRecLifeMax)) ||
            65536 <= cDaysRecLifeMax)
        {
            cDaysRecLifeMax = DEF_REC_LIFE_MAX; // Default;
            key.SetValue(g_szSidCacheRecLifeMax, cDaysRecLifeMax);
        }
    }

    if (cDaysRecLifeMax < m_cDaysRecLifeMin)
        cDaysRecLifeMax = m_cDaysRecLifeMin;

    m_cDaysRecLifeRange = cDaysRecLifeMax - m_cDaysRecLifeMin;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::~RecordMgr

    Description: Record manager destructor. Closes the data file.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
SidNameCache::RecordMgr::~RecordMgr(
    VOID
    )
{
    DBGTRACE((DM_SIDCACHE, DL_MID, TEXT("SidNameCache::SidNameCache::~RecordMgr")));
    CloseDataFile();
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::Initialize

    Description: Initializes a new record manager object.

    Arguments:
        pszFile - Address of full path for new file.

        cBlocks - Number of storage blocks in data file. Each is 32 bytes.
            If this argument is 0, the function tries to open an existing
            cache data file.

    Returns: Address of mapped file or NULL on failure.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LPBYTE
SidNameCache::RecordMgr::Initialize(
    LPCTSTR pszFile,
    DWORD cBlocks
    )
{
    DBGTRACE((DM_SIDCACHE, DL_HIGH, TEXT("SidNameCache::RecordMgr::Initialize")));
    DBGPRINT((DM_SIDCACHE, DL_HIGH, TEXT("\tpszFile = \"%s\", cBlocks = %d"),
              pszFile ? pszFile : TEXT("<null>"), cBlocks));
    //
    // Store the file name in our CString object.
    //
    m_strFileName = pszFile;

    if (0 != cBlocks)
    {
        //
        // Create a new data file.
        //
        // cBlocks must be a multiple of 32.
        // This satisfies the quadword alignment and makes sizing the
        // allocation bitmap much easier.  We let the caller pass in any value
        // they want and we just adjust it upward as needed.
        //
        if (cBlocks & 0x0000001F)
            cBlocks = (cBlocks & 0xFFFFFFE0) + 32;

        ULARGE_INTEGER uliFileSize;
        uliFileSize.QuadPart = FileSize(cBlocks);

        m_pFileHdr = (PDATA_FILE_HDR)CreateDataFile(pszFile,
                                                    uliFileSize.HighPart,
                                                    uliFileSize.LowPart);
        if (NULL != m_pFileHdr)
        {
            InitNewDataFile(cBlocks);
        }
    }
    else
    {
        //
        // Open an existing data file.
        //
        m_pFileHdr = (PDATA_FILE_HDR)OpenDataFile(pszFile);
        if (NULL != m_pFileHdr)
        {
            if (FILE_VERSION != m_pFileHdr->dwVersion ||
                DATA_FILE_SIGNATURE != m_pFileHdr->dwSignature)
            {
                //
                // This version of the software doesn't understand this
                // version of the file or the signature is invalid.
                // Don't take any chances.  We'll just create a new one.
                //
                DBGERROR((TEXT("SIDCACHE - Data file is invalid or incorrect version. A new data file will be created.")));

                CloseDataFile();
                m_pFileHdr = NULL;
            }
        }
    }
    return (LPBYTE)m_pFileHdr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::CreateDataFile

    Description: Creates and initializes a new data file.

    Arguments:
        pszFile - Address of full path for new file.

        cbFileHigh/Low - Size of file in bytes.

    Returns: Address of mapped file or NULL on failure.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LPBYTE
SidNameCache::RecordMgr::CreateDataFile(
    LPCTSTR pszFile,
    DWORD cbFileHigh,
    DWORD cbFileLow
    )
{
    DBGTRACE((DM_SIDCACHE, DL_LOW, TEXT("SidNameCache::RecordMgr::CreateDataFile")));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("\tpszFile = \"%s\""),
              pszFile ? pszFile : TEXT("<null>")));

    CloseDataFile();  // Make sure any existing data file is closed.

    g_pbMappedDataFile = SidNameCache::OpenMappedFile(
                                pszFile,
                                g_pszDataFileMapping,
                                CREATE_ALWAYS,
                                cbFileHigh,
                                cbFileLow,
                                &m_hFile,
                                &m_hFileMapping);

    return g_pbMappedDataFile;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::OpenDataFile

    Description: Opens an existing data file.

    Arguments:
        pszFile - Address of full path of existing file.

    Returns: Address of mapped file or NULL on failure.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LPBYTE
SidNameCache::RecordMgr::OpenDataFile(
    LPCTSTR pszFile
    )
{
    DBGTRACE((DM_SIDCACHE, DL_LOW, TEXT("SidNameCache::RecordMgr::OpenDataFile")));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("\tpszFile = \"%s\""),
              pszFile ? pszFile : TEXT("<null>")));

    CloseDataFile();  // Make sure any existing data file is closed.

    g_pbMappedDataFile = SidNameCache::OpenMappedFile(
                                pszFile,
                                g_pszDataFileMapping,
                                OPEN_EXISTING,
                                0,
                                0,
                                &m_hFile,
                                &m_hFileMapping);

    return g_pbMappedDataFile;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::CloseDataFile

    Description: Closes the current data mapping and file.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::RecordMgr::CloseDataFile(
    VOID
    )
{
    DBGTRACE((DM_SIDCACHE, DL_LOW, TEXT("SidNameCache::RecordMgr::CloseDataFile")));
    if (NULL != g_pbMappedDataFile)
    {
        UnmapViewOfFile(g_pbMappedDataFile);
        g_pbMappedDataFile = NULL;
        m_pFileHdr = NULL;
    }
    if (NULL != m_hFileMapping)
    {
        CloseHandle(m_hFileMapping);
        m_hFileMapping = NULL;
    }
    if (NULL != m_hFile && INVALID_HANDLE_VALUE != m_hFile)
    {
        CloseHandle(m_hFile);
        m_hFile = NULL;
    }
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::GrowDataFile

    Description: Increases the size of the current data file.

    Arguments:
        cGrowBlocks - Add this many more blocks to the data file.  The
            block allocation bitmap is also extended to accomdate the new
            block count.

    Returns: Address of mapped file or NULL on failure.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LPBYTE
SidNameCache::RecordMgr::GrowDataFile(
    DWORD cGrowBlocks
    )
{
    DBGTRACE((DM_SIDCACHE, DL_LOW, TEXT("SidNameCache::RecordMgr::GrowDataFile")));

    DBGASSERT((DATA_FILE_MAPPED));

    DWORD cOldBlocks = m_pFileHdr->cBlocks;
    DWORD cNewBlocks = cOldBlocks + cGrowBlocks;
    DWORD cOldMapEle = m_pFileHdr->cMapElements;
    DWORD cNewMapEle = 0; // Will calc later.

    //
    // cBlocks must be a multiple of 32.
    // This satisfies the quadword alignment and makes sizing the
    // allocation bitmap much easier.
    //
    if (cNewBlocks & 0x0000001F)
        cNewBlocks = (cNewBlocks & 0xFFFFFFE0) + 32;
    DBGASSERT((cNewBlocks >= cOldBlocks));

    //
    // Adjust cGrowBlocks for any adjustments in cNewBlocks.
    //
    cGrowBlocks = cNewBlocks - cOldBlocks;

    //
    // How many alloc map elements (DWORDs) do we need now?
    //
    cNewMapEle = cNewBlocks / BITS_IN_DWORD;

    DBGPRINT((DM_SIDCACHE, DL_MID,
             TEXT("Growing SID cache data file\nMap Ele %d -> %d\nBlocks %d -> %d"),
             cOldMapEle, cNewMapEle, cOldBlocks, cNewBlocks));

    //
    // Open the mapped file with a new larger size.
    //
    ULARGE_INTEGER uliFileSize;
    uliFileSize.QuadPart = FileSize(cNewBlocks);

    CloseDataFile();

    g_pbMappedDataFile = SidNameCache::OpenMappedFile(
                                m_strFileName,
                                g_pszDataFileMapping,
                                OPEN_EXISTING,
                                uliFileSize.HighPart,
                                uliFileSize.LowPart,
                                &m_hFile,
                                &m_hFileMapping);

    m_pFileHdr = (PDATA_FILE_HDR)g_pbMappedDataFile;

    if (NULL != g_pbMappedDataFile)
    {
        UINT i = 0;

        //
        // Block count and map size both increase.
        // Since map size increases, blocks must be moved to accomodate new
        // map elements.  Since the index file tracks records by block index,
        // this movement doesn't affect existing index file entries.
        //
        m_pFileHdr->cBlocks       = cNewBlocks;
        m_pFileHdr->cMapElements  = cNewMapEle;
        //
        // Save current block base for when we move the blocks to make room for
        // the growth of the allocation bitmap.
        //
        PBLOCK pBlocksOld = m_pFileHdr->pBlocks;
        //
        // Calculate the new address of block 0.
        // We want all of the data blocks quadword aligned because they contain
        // a FILETIME structure (64-bits).
        //
        m_pFileHdr->pBlocks = (PBLOCK)(m_pFileHdr->pdwMap + m_pFileHdr->cMapElements);
        QuadAlign((LPDWORD)(&m_pFileHdr->pBlocks));

        //
        // Move all of the existing blocks to their new locations.
        //
        MoveMemory(DAT_BASED_CAST(BLOCK, m_pFileHdr->pBlocks),
                   DAT_BASED_CAST(BLOCK, pBlocksOld),
                   cOldBlocks * sizeof(BLOCK));
        //
        // Initialize the new map elements to 0 (un-allocated).
        //
        ZeroMemory(DAT_BASED_CAST(BYTE, m_pFileHdr->pdwMap + cOldMapEle),
                   (cNewMapEle - cOldMapEle) * sizeof(DWORD));
        //
        // Initialize the new data blocks to 0xCC pattern.
        //
        FillBlocks(cOldBlocks, cGrowBlocks, RECORD_UNUSED_BYTE);

        DBGPRINT((DM_SIDCACHE, DL_MID, TEXT("SIDCACHE - Data file growth complete.")));
    }
    return g_pbMappedDataFile;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::SetFileGUID
    Function: SidNameCache::RecordMgr::GetFileGUID

    Description: These functions manipulate the guid field in the data file's
        header.
        The GUID is used to ensure integrity between
        the data and index files.  Before any change to either file, the
        GUID's are both set to 0.  When the change operation is complete,
        a new GUID is generated and written to both files.  Therefore, before
        any transaction, we can validate the data and index files by reading
        and comparing GUIDs.  If the GUIDs are not 0 and are equal, the
        file can be assumed to be valid.

    Arguments:
        pguid - Address of source or destination GUID.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::RecordMgr::SetFileGUID(
    const GUID *pguid
    )
{
    DBGASSERT((DATA_FILE_MAPPED));
    DBGASSERT((NULL != pguid));
    CopyMemory(&m_pFileHdr->guid, pguid, sizeof(GUID));
}


VOID
SidNameCache::RecordMgr::GetFileGUID(
    LPGUID pguid
    )
{
    DBGASSERT((DATA_FILE_MAPPED));
    DBGASSERT((NULL != pguid));
    CopyMemory(pguid, &m_pFileHdr->guid, sizeof(GUID));
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::InitNewDataFile

    Description: Initializes a new data file filling in the header information
        and writing 0xCC in all the data block bytes.

    Arguments:
        cBlocks - Number of data blocks (32 bytes each) in data file.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::RecordMgr::InitNewDataFile(
    DWORD cBlocks
    )
{
    DBGTRACE((DM_SIDCACHE, DL_LOW, TEXT("SidNameCache::RecordMgr::InitNewDataFile")));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("\tcBlocks = %d"), cBlocks));
    UINT i = 0;

    DBGASSERT((DATA_FILE_MAPPED));

    //
    // Initialize file header.
    //
    m_pFileHdr->dwSignature   = DATA_FILE_SIGNATURE;
    m_pFileHdr->dwVersion     = FILE_VERSION;
    m_pFileHdr->cBlocks       = cBlocks;
    m_pFileHdr->cMapElements  = cBlocks / BITS_IN_DWORD;
    m_pFileHdr->pdwMap        = (LPDWORD)(sizeof(DATA_FILE_HDR));
    m_pFileHdr->pBlocks       = (PBLOCK)(m_pFileHdr->pdwMap + m_pFileHdr->cMapElements);

    //
    // We want all of the data blocks quadword aligned because they contain
    // a FILETIME structure (64-bits).
    //
    QuadAlign((LPDWORD)(&m_pFileHdr->pBlocks));

    //
    // Write 0xCC to all data block bytes.
    //
    Clear();
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::Clear

    Description: Zero's the file header guid, clears all bits in the
        block allocation bitmap and fills the data blocks with 0xCC.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::RecordMgr::Clear(
    VOID
    )
{
    DBGTRACE((DM_SIDCACHE, DL_LOW, TEXT("SidNameCache::RecordMgr::Clear")));
    DBGASSERT((DATA_FILE_MAPPED));

    m_pFileHdr->cBlocksUsed   = 0;
    m_pFileHdr->iFirstFree    = 0;
    SetFileGUID(&GUID_Null);

    //
    // Initialize all block allocation map bits to 0 (un-allocated).
    //
    ZeroMemory(DAT_BASED_CAST(BYTE, m_pFileHdr->pdwMap),
               m_pFileHdr->cMapElements * sizeof(DWORD));
    //
    // Initialize all data blocks to 0xCC pattern.
    //
    FillBlocks(0, m_pFileHdr->cBlocks, RECORD_UNUSED_BYTE);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::FillBlocks

    Description: Fills a range of blocks with a specified byte.

    Arguments:
        iBlock - Index of first block in range.

        cBlocks - Number of blocks to fill.

        b - Byte to write to blocks.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::RecordMgr::FillBlocks(
    DWORD iBlock,
    DWORD cBlocks,
    BYTE b
    )
{
    DBGASSERT((DATA_FILE_MAPPED));
    DBGASSERT((ValidBlockNumber(iBlock)));

    DAT_BASED(BYTE) *pb = DAT_BASED_CAST(BYTE, BlockAddress(iBlock));
    DBGASSERT((SidNameCache::IsQuadAligned(pb)));

    //
    // Just in case the fill request would extend over the
    // end of the file, truncate the requested block count.  The assertion
    // will catch it during development.
    //
    DWORD iLastBlock = iBlock + cBlocks - 1;
    DBGASSERT((ValidBlockNumber(iLastBlock)));
    if (iLastBlock >= m_pFileHdr->cBlocks)
        cBlocks = m_pFileHdr->cBlocks - iBlock;

    FillMemory(pb, sizeof(BLOCK) * cBlocks, b);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::IsBitSet
    Function: SidNameCache::RecordMgr::SetBit
    Function: SidNameCache::RecordMgr::ClrBit

    Description: Tests and sets bits in the block allocation bitmap.

    Arguments:
        pdwBase - Address of 1st DWORD in bitmap.

        iBit - 0-based index of bit in bitmap.

    Returns: IsBitSet returns TRUE/FALSE indicating the state of the bit.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
SidNameCache::RecordMgr::IsBitSet(
    LPDWORD pdwBase,
    DWORD iBit
    )
{
    DBGASSERT((NULL != pdwBase));
    DWORD b = iBit & 0x0000001F;
    DWORD i = iBit >> 5;

    return (*(pdwBase + i)) & (1 << b);
}


VOID
SidNameCache::RecordMgr::SetBit(
    LPDWORD pdwBase,
    DWORD iBit
    )
{
    DBGASSERT((NULL != pdwBase));
    DWORD b = iBit & 0x0000001F;
    DWORD i = iBit >> 5;

    (*(pdwBase + i)) |= (1 << b);
}

VOID
SidNameCache::RecordMgr::ClrBit(
    LPDWORD pdwBase,
    DWORD iBit
    )
{
    DBGASSERT((NULL != pdwBase));
    DWORD b = iBit & 0x0000001F;
    DWORD i = iBit >> 5;

    (*(pdwBase + i)) &= ~(1 << b);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::ValidBlockNumber

    Description: Determine if a given block number is valid for the
        current data file.  Primarily meant for use in assertions.

    Arguments:
        iBlock - 0-based index of block in data file.

    Returns: TRUE/FALSE indicating validity of block number.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
SidNameCache::RecordMgr::ValidBlockNumber(
    DWORD iBlock
    )
{
    DBGASSERT((DATA_FILE_MAPPED));
    return (iBlock < m_pFileHdr->cBlocks);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::IsBlockUsed
    Function: SidNameCache::RecordMgr::MarkBlockUsed
    Function: SidNameCache::RecordMgr::MarkBlockUnused

    Description: Examines and changes the allocation state of a block in the
        data file.

    Arguments:
        iBlock - 0-based index of block in data file.

    Returns: IsBlockUsed returns TRUE/FALSE indicating the allocation state.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
SidNameCache::RecordMgr::IsBlockUsed(
    DWORD iBlock
    )
{
    DBGASSERT((DATA_FILE_MAPPED));
    DBGASSERT((ValidBlockNumber(iBlock)));
    DAT_BASED(DWORD) *pdwBased = DAT_BASED_CAST(DWORD, m_pFileHdr->pdwMap);

    return IsBitSet((LPDWORD)pdwBased, iBlock);
}


VOID
SidNameCache::RecordMgr::MarkBlockUsed(
    DWORD iBlock
    )
{
    DBGASSERT((DATA_FILE_MAPPED));
    DBGASSERT((ValidBlockNumber(iBlock)));

    DAT_BASED(DWORD) *pdwBased = DAT_BASED_CAST(DWORD, m_pFileHdr->pdwMap);
    DBGASSERT((!IsBitSet((LPDWORD)pdwBased, iBlock)));
    SetBit((LPDWORD)pdwBased, iBlock);
}

VOID
SidNameCache::RecordMgr::MarkBlockUnused(
    DWORD iBlock
    )
{
    DBGASSERT((DATA_FILE_MAPPED));
    DBGASSERT((ValidBlockNumber(iBlock)));

    DAT_BASED(DWORD) *pdwBased = DAT_BASED_CAST(DWORD, m_pFileHdr->pdwMap);
    DBGASSERT((IsBitSet((LPDWORD)pdwBased, iBlock)));
    ClrBit((LPDWORD)pdwBased, iBlock);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::BlocksRequired

    Description: Calculate the number of blocks required to store a given
        number of bytes.

    Arguments:
        cb - Number of bytes requested.

    Returns: Number of 32-byte blocks required to store the bytes.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DWORD
SidNameCache::RecordMgr::BlocksRequired(
    DWORD cb
    )
{
    //
    // Round byte request up to nearest 32-byte block.
    //
    if (cb & 0x0000001F)
        cb = (cb & 0xFFFFFFE0) + 32;

    //
    // How many "blocks" are required?
    //
    return BYTE_TO_BLOCK(cb);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::IsQuadAligned
    Function: SidNameCache::QuadAlign
    Function: SidNameCache::WordAlign

    Description: Methods that determine if a value is quad aligned and for
        updating a value so that it is quad or word aligned.

    Arguments:
        See the individual methods.  It's pretty self-explanatory.

    Returns:
        See the individual methods.  It's pretty self-explanatory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
SidNameCache::IsQuadAligned(
    LPVOID pv
    )
{
    return (IsQuadAligned((DWORD_PTR)pv));
}


BOOL
SidNameCache::IsQuadAligned(
    DWORD_PTR dw
    )
{
    return (0 == (dw & 0x00000007));
}


VOID
SidNameCache::QuadAlign(
    LPDWORD pdwValue
    )
{
    DBGASSERT((NULL != pdwValue));
    if (*pdwValue & 0x00000007)
    {
        //
        // Round up to next whole multiple of 8.
        //
        *pdwValue = (*pdwValue & 0xFFFFFFF8) + 8;
    }
}


VOID
SidNameCache::WordAlign(
    LPDWORD pdwValue
    )
{
    DBGASSERT((NULL != pdwValue));
    if (*pdwValue & 0x00000001)
    {
        //
        // Round up to next whole multiple of 2.
        //
        *pdwValue = (*pdwValue & 0xFFFFFFFE) + 2;
    }
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::BytesRequiredForRecord

    Description: Calculates the number of bytes required to store a given
        data file record.  Optionally returns the required size for each
        field in the record (SID, name, domain etc).  The function makes
        allowances for any required data type alignments in the file.

    Arguments:
        pSid - Address of user's SID.

        pcbSid [optional] - Address of variable to receive required SID length.

        pszContainer - Address of user's account container name string.

        pcbContainer [optional] - Address of variable to receive required length
            of container name string.

        pszLogonName - Address of user's logon name string.

        pcbLogonName [optional] - Address of variable to receive required length
            of logon name string.

        pszDisplayName - Address of user's display name string.

        pcbDisplayName [optional] - Address of variable to receive required
            length of display name string.

    Returns: Number of bytes required to store the record.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DWORD
SidNameCache::RecordMgr::BytesRequiredForRecord(
    PSID pSid,
    LPDWORD pcbSid,
    LPCTSTR pszContainer,
    LPDWORD pcbContainer,
    LPCTSTR pszLogonName,
    LPDWORD pcbLogonName,
    LPCTSTR pszDisplayName,
    LPDWORD pcbDisplayName
    )
{
    DWORD cb      = 0;
    DWORD cbTotal = sizeof(RECORD_HDR);

    //
    // SID follows the header so it IS quadword aligned.
    // It's a byte structure so it doesn't have to be; but it is anyway.
    //
    cb = GetLengthSid(pSid);
    cbTotal += cb;
    if (NULL != pcbSid)
        *pcbSid = cb;

    //
    // Strings are UNICODE and must be word-aligned.  Just align the first.
    // All subsequent are guaranteed to be properly aligned.
    //
    SidNameCache::WordAlign(&cbTotal);
    cb = (lstrlen(pszContainer) + 1) * sizeof(TCHAR);
    cbTotal += cb;
    if (NULL != pcbContainer)
        *pcbContainer = cb;

    cb = (lstrlen(pszLogonName) + 1) * sizeof(TCHAR);
    cbTotal += cb;
    if (NULL != pcbLogonName)
        *pcbLogonName = cb;

    cb = (lstrlen(pszDisplayName) + 1) * sizeof(TCHAR);
    cbTotal += cb;
    if (NULL != pcbDisplayName)
        *pcbDisplayName = cb;

    return cbTotal;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::FreeBlock

    Description: Frees a single block in the data file.

    Arguments:
        iBlock - 0-based index of the block.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::RecordMgr::FreeBlock(
    DWORD iBlock
    )
{
    DBGASSERT((DATA_FILE_MAPPED));
    DBGASSERT((ValidBlockNumber(iBlock)));
    DBGASSERT((IsBlockUsed(iBlock)));

    MarkBlockUnused(iBlock);
    DBGASSERT((!IsBlockUsed(iBlock)));

    FillBlocks(iBlock, 1, RECORD_UNUSED_BYTE);

    //
    // Update the "first free" index if needed.
    //
    if (iBlock < m_pFileHdr->iFirstFree)
        m_pFileHdr->iFirstFree = iBlock;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::FreeBlocks

    Description: Frees a series of contiguous blocks in the data file.

    Arguments:
        iBlock - 0-based index of the first block in the series.

        cBlocks - Number of blocks in the series.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::RecordMgr::FreeBlocks(
    DWORD iFirstBlock,
    DWORD cBlocks
    )
{
    for (UINT i = 0; i < cBlocks; i++)
        FreeBlock(iFirstBlock + i);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::FreeRecord

    Description: Frees all blocks in a data file record.

    Arguments:
        iFirstBlock - 0-based index of the first block in the record.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
SidNameCache::RecordMgr::FreeRecord(
    DWORD iFirstBlock
    )
{
    DBGASSERT((DATA_FILE_MAPPED));
    DBGASSERT((ValidBlockNumber(iFirstBlock)));
    DAT_BASED(RECORD_HDR) *pRecHdr = DAT_BASED_CAST(RECORD_HDR, BlockAddress(iFirstBlock));

    DBGASSERT((RECORD_SIGNATURE == pRecHdr->dwSignature));
    FreeBlocks(iFirstBlock, pRecHdr->cBlocks);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::BlockAddress

    Description: Calculates the non-based address of a given block in the
        data file.

    Arguments:
        iBlock - 0-based index of the block.

    Returns: Address of the block in the data file.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
PBLOCK
SidNameCache::RecordMgr::BlockAddress(
    DWORD iBlock
    )
{
    DBGASSERT((DATA_FILE_MAPPED));
    DBGASSERT((ValidBlockNumber(iBlock)));

    return m_pFileHdr->pBlocks + iBlock;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::FileSize

    Description: Calculate the data file size required for a given number of
        data blocks.  Accounts for data type alignment.

    Arguments:
        cBlocks - Number of blocks required in the data file.

    Returns: Bytes required.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/24/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
UINT64
SidNameCache::RecordMgr::FileSize(
    DWORD cBlocks
    )

{
    DWORD dwTemp = sizeof(DATA_FILE_HDR) +
                  ((cBlocks / BITS_IN_DWORD) * sizeof(DWORD));

    //
    // Start of blocks must be quad-aligned.
    //
    SidNameCache::QuadAlign(&dwTemp);

    return (UINT64)(dwTemp) +
           (UINT64)(sizeof(BLOCK) * cBlocks);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::AllocBlocks

    Description: Allocates a specified number of contiguous blocks in the
        data file.

    Arguments:
        cBlocksReqd - Number of blocks required in the allocation.

    Returns: If successful, returns the index of the first block in the
        allocation.  Returns (DWORD)-1 if the block's can't be allocated.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DWORD
SidNameCache::RecordMgr::AllocBlocks(
    DWORD cBlocksReqd
    )
{
    DBGASSERT((DATA_FILE_MAPPED));

    DWORD iBlock = m_pFileHdr->iFirstFree;

    DBGPRINT((DM_SIDCACHE, DL_MID,
             TEXT("SIDCACHE - AllocBlocks: Allocate %d blocks"),
             cBlocksReqd));

    while(iBlock < m_pFileHdr->cBlocks)
    {
        DBGPRINT((DM_SIDCACHE, DL_MID,
                 TEXT("   Start scan at block %d"), iBlock));
        //
        // Look for cBlocksReqd consecutive free blocks.
        //
        for (UINT j = 0; j < cBlocksReqd && (iBlock + j) < m_pFileHdr->cBlocks ; j++)
        {
            DBGPRINT((DM_SIDCACHE, DL_MID, TEXT("      Checking %d"), iBlock + j));
            if (IsBlockUsed(iBlock + j))
            {
                //
                // This one's used. Start searching again.
                //
                DBGPRINT((DM_SIDCACHE, DL_MID, TEXT("      %d is used"), iBlock + j));
                break;
            }
#if DBG
            //
            // If a block is marked "unused", it should contain all 0xCC.
            //
            DAT_BASED(BYTE) *pb = DAT_BASED_CAST(BYTE, BlockAddress(iBlock + j));
            for (UINT k = 0; k < sizeof(BLOCK); k++)
            {
                DBGASSERT((RECORD_UNUSED_BYTE == *(pb + k)));
            }
#endif
        }

        DBGPRINT((DM_SIDCACHE, DL_MID, TEXT("   Scan complete.  %d blocks checked"), j));
        if (j == cBlocksReqd)
        {
            //
            // Found a sufficient range of free blocks.
            // Mark the blocks as allocated in the allocation bitmap.
            //
            for (UINT i = 0; i < cBlocksReqd; i++)
                MarkBlockUsed(iBlock + i);

            if (iBlock == m_pFileHdr->iFirstFree)
            {
                //
                // Now scan to find the next free block.
                // We'll save it's location to help with future free-block searches.
                //
                for (m_pFileHdr->iFirstFree = iBlock + cBlocksReqd;
                     m_pFileHdr->iFirstFree < m_pFileHdr->cBlocks && IsBlockUsed(m_pFileHdr->iFirstFree);
                     m_pFileHdr->iFirstFree++)
                {
                    DBGPRINT((DM_SIDCACHE, DL_MID,
                             TEXT("SIDCACHE - Advancing first free %d"),
                             m_pFileHdr->iFirstFree));
                    NULL;
                }
            }
            DBGPRINT((DM_SIDCACHE, DL_MID, TEXT("SIDCACHE - Found free block range at %d"), iBlock));
            return iBlock;
        }

        iBlock += (j + 1);  // Continue search.
    }
    DBGPRINT((DM_SIDCACHE, DL_MID, TEXT("SIDCACHE - No blocks available")));

    return (DWORD)-1;  // No blocks available of sufficient size.
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::RecordExpired

    Description: Determine if a given record has expired.  A record has
        "expired" if it's expiration date is prior to "today".

    Arguments:
        iBlock - 0-based index of first block in record.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
SidNameCache::RecordMgr::RecordExpired(
    DWORD iBlock
    )
{
    DBGASSERT((ValidBlockNumber(iBlock)));
    DAT_BASED(RECORD_HDR) *pRec = DAT_BASED_CAST(RECORD_HDR, BlockAddress(iBlock));
    DBGASSERT((SidNameCache::IsQuadAligned(pRec)));
    DBGASSERT((RECORD_SIGNATURE == pRec->dwSignature));

    SYSTEMTIME SysNow;
    FILETIME FileNow;
    ULARGE_INTEGER uliFileNow;
    ULARGE_INTEGER uliBirthday;

    uliBirthday.LowPart  = pRec->Birthday.dwLowDateTime;
    uliBirthday.HighPart = pRec->Birthday.dwHighDateTime;

    GetSystemTime(&SysNow);
    SystemTimeToFileTime(&SysNow, &FileNow);

    uliFileNow.LowPart  = FileNow.dwLowDateTime;
    uliFileNow.HighPart = FileNow.dwHighDateTime;

    DWORD cDaysVariation = 0;
    //
    // Avoid div-by-zero.
    //
    if (0 < m_cDaysRecLifeRange)
        cDaysVariation = SysNow.wMilliseconds % m_cDaysRecLifeRange;

    //
    // Add time specified in registry to the record's birthday.
    // 864,000,000,000L is the number of 100 nanosecond periods in a day
    // which is the unit the FILETIME structure is based on.
    //
    uliBirthday.QuadPart += ((UINT64)864000000000L *
                            (UINT64)(m_cDaysRecLifeMin + cDaysVariation));
    //
    // If it's still less than "now", the record is considered good.
    //
    return uliFileNow.QuadPart > uliBirthday.QuadPart;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::Store

    Description: Stores a record in the data file.

    Arguments:
        pSid - Address of user's SID.

        pszContainer - Address of user's account container name string.

        pszLogonName - Address of user's logon name string.

        pszDisplayName - Address of user's display name string.

    Returns: Index of the first block used for the record.
        (DWORD)-1 if the block couldn't be stored.  Could mean out-of-disk
        space.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DWORD
SidNameCache::RecordMgr::Store(
    PSID pSid,
    LPCTSTR pszContainer,
    LPCTSTR pszLogonName,
    LPCTSTR pszDisplayName
    )
{
    DWORD cbSid         = 0;
    DWORD cbContainer   = 0;
    DWORD cbLogonName   = 0;
    DWORD cbDisplayName = 0;

    DWORD cbRequired = BytesRequiredForRecord(
                            pSid,
                            &cbSid,
                            pszContainer,
                            &cbContainer,
                            pszLogonName,
                            &cbLogonName,
                            pszDisplayName,
                            &cbDisplayName);

    DWORD cBlocksRequired = BlocksRequired(cbRequired);
    DBGPRINT((DM_SIDCACHE, DL_MID,
             TEXT("SIDCACHE - Store: %s (%s) in \"%s\"  %d bytes, %d blocks"),
             pszDisplayName, pszLogonName, pszContainer, cbRequired, cBlocksRequired));

    //
    // Try to allocate the required blocks.
    //
    DWORD iBlock = AllocBlocks(cBlocksRequired);
    if ((DWORD)-1 == iBlock)
    {
        //
        // Couldn't allocate blocks.  Extend the data file.
        //
        GrowDataFile(DATA_FILE_GROW_BLOCKS);
        iBlock = AllocBlocks(cBlocksRequired);
    }
    if ((DWORD)-1 != iBlock)
    {
        //
        // Got the required number of blocks.
        //
        DBGASSERT((ValidBlockNumber(iBlock)));
        PBLOCK pBlock               = BlockAddress(iBlock);
        DAT_BASED(RECORD_HDR) *pRec = DAT_BASED_CAST(RECORD_HDR, pBlock);
        DAT_BASED(BYTE) *pbRec      = DAT_BASED_CAST(BYTE, pBlock);

        //
        // Fill in the record header.
        // Includes storing the item offset values from the start of the record.
        // Storing these values in the record hdr will help with data retrieval.
        //
        pRec->dwSignature       = RECORD_SIGNATURE;
        pRec->cBlocks           = cBlocksRequired;
        pRec->cbOfsSid          = sizeof(RECORD_HDR);
        pRec->cbOfsContainer    = pRec->cbOfsSid    + cbSid;
        WordAlign(&pRec->cbOfsContainer);

        pRec->cbOfsLogonName   = pRec->cbOfsContainer + cbContainer;
        pRec->cbOfsDisplayName = pRec->cbOfsLogonName + cbLogonName;;

        //
        // Record the "birthday" so we can age the record.
        //
        SYSTEMTIME SysNow;
        GetSystemTime(&SysNow);
        SystemTimeToFileTime(&SysNow, &pRec->Birthday);

        //
        // Store the record's data.
        // Use ACTUAL length values for memory transfers.
        //
        CopySid(cbSid, (PSID)(pbRec + pRec->cbOfsSid), pSid);
        lstrcpy((LPTSTR)(pbRec + pRec->cbOfsContainer),   pszContainer);
        lstrcpy((LPTSTR)(pbRec + pRec->cbOfsLogonName),   pszLogonName);
        lstrcpy((LPTSTR)(pbRec + pRec->cbOfsDisplayName), pszDisplayName);

        //
        // Update the file's "blocks used" count.
        //
        m_pFileHdr->cBlocksUsed += pRec->cBlocks;
    }

    return iBlock;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::Retrieve

    Description: Retrieves a record in the data file.

    Arguments:
        iBlock - 0-based index of the starting block in the record.

        ppSid [optional] - Address of SID pointer variable to receive the
            address of the SID buffer.  The caller is responsible for freeing
            the buffer.  May be NULL.

        ppszContainer [optional] - Address of pointer variable to receive the
            address of the container name string buffer.  The caller is
            responsible for freeing the buffer.  May be NULL.

        ppszLogonName [optional] - Address of pointer variable to receive the
            address of the logon name string buffer.  The caller is
            responsible for freeing the buffer.  May be NULL.

        ppszDisplayName [optional] - Address of pointer variable to receive the
            address of the display name string buffer.  The caller is
            responsible for freeing the buffer.  May be NULL.

    Returns:
        NO_ERROR          - Success.
        ERROR_INVALID_SID (hr) - The record contains an invalid SID.  Probably
            a corrupt record.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
SidNameCache::RecordMgr::Retrieve(
    DWORD iBlock,
    PSID *ppSid,
    LPTSTR *ppszContainer,
    LPTSTR *ppszLogonName,
    LPTSTR *ppszDisplayName
    )
{
    PBLOCK pBlock               = BlockAddress(iBlock);
    DAT_BASED(RECORD_HDR) *pRec = DAT_BASED_CAST(RECORD_HDR, pBlock);
    DAT_BASED(BYTE) *pbRec      = DAT_BASED_CAST(BYTE, pBlock);

    DBGASSERT((SidNameCache::IsQuadAligned(pRec)));
    DBGASSERT((RECORD_SIGNATURE == pRec->dwSignature));

    if (NULL != ppSid)
    {
        PSID pSid = (PSID)(pbRec + pRec->cbOfsSid);
        if (IsValidSid(pSid))
        {
            *ppSid = SidDup(pSid);
        }
        else
            return HRESULT_FROM_WIN32(ERROR_INVALID_SID);
    }

    if (NULL != ppszContainer)
    {
        *ppszContainer = StringDup((LPTSTR)(pbRec + pRec->cbOfsContainer));
    }

    if (NULL != ppszLogonName)
    {
        *ppszLogonName = StringDup((LPTSTR)(pbRec + pRec->cbOfsLogonName));
    }

    if (NULL != ppszDisplayName)
    {
        *ppszDisplayName = StringDup((LPTSTR)(pbRec + pRec->cbOfsDisplayName));
    }

    return NO_ERROR;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameCache::RecordMgr::Dump

    Description: Dumps the contents of the data file to the debugger output.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#if DBG
VOID
SidNameCache::RecordMgr::Dump(
    VOID
    )
{
    UINT i, j;
    DBGASSERT((DATA_FILE_MAPPED));

    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("Dumping SidNameCache RecordMgr at 0x%p"), this));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("  Base...............: 0x%p"), g_pbMappedDataFile));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("  m_pFileHdr.........: 0x%p"), m_pFileHdr));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("    dwSignature......: 0x%08X"), (DWORD)m_pFileHdr->dwSignature));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("    dwVersion........: 0x%08X"), (DWORD)m_pFileHdr->dwVersion));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("    cBlocks..........: %d"),     (DWORD)m_pFileHdr->cBlocks));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("    cBlocksUsed......: %d"),     (DWORD)m_pFileHdr->cBlocksUsed));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("    cMapElements.....: %d"),     (DWORD)m_pFileHdr->cMapElements));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("    iFirstFree.......: %d"),     (DWORD)m_pFileHdr->iFirstFree));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("    pdwMap...........: 0x%p"), m_pFileHdr->pdwMap));
    DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("    pBlocks..........: 0x%p"), m_pFileHdr->pBlocks));

    PBLOCK pBlock = m_pFileHdr->pBlocks;

    for (i = 0; i < m_pFileHdr->cBlocks; i++)
    {
        DAT_BASED(BYTE) *pb = DAT_BASED_CAST(BYTE, pBlock);

        DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("BLOCK %d --------------------"), i));
        for (UINT row = 0; row < 2; row++)
        {
            TCHAR szHex[MAX_PATH];
            TCHAR szAscii[MAX_PATH];

            LPTSTR pszHex = szHex;
            pb = DAT_BASED_CAST(BYTE, pBlock) + (row * (sizeof(BLOCK)/2));
            for (j = 0; j < 16; j++)
            {
                wsprintf(pszHex, TEXT("%02X "), *pb);
                pb++;
                pszHex += 2;
            }

            LPTSTR pszAscii = szAscii;
            pb = DAT_BASED_CAST(BYTE, pBlock) + (row * (sizeof(BLOCK)/2));
            for (j = 0; j < 16; j++)
            {
                wsprintf(pszAscii, TEXT("%c"), *pb > 31 ? *pb : TEXT('.'));
                pb++;
                pszAscii++;
            }
            DBGPRINT((DM_SIDCACHE, DL_LOW, TEXT("%s %s"), szHex, szAscii));
        }
        pBlock++;
    }
}

#endif // DEBUG

