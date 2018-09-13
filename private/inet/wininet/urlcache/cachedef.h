/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    cachedef.h

Abstract:

    contains data definitions for cache code.

Author:

    Madan Appiah (madana)  12-Apr-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _CACHEDEF_
#define _CACHEDEF_

//
// C++ inline code definition for retail build only.
//

#if DBG
#undef CHECKLOCK_NORMAL
#undef CHECKLOCK_PARANOID
#define INLINE
#else
#define INLINE      inline
#endif

#define PAGE_SIZE                        4096            // Grow memory mapped file by 1 page.
#define ALLOC_PAGES                      4               // was 2
#define HEADER_ENTRY_SIZE                ALLOC_PAGES * PAGE_SIZE
#define NORMAL_ENTRY_SIZE                128

#define DEFAULT_CLEANUP_FACTOR           10  // % free goal once cache quota exceeded
#define MAX_EXEMPT_PERCENTAGE            70

#define MEMMAP_FILE_NAME                 TEXT("index.dat")
#define DESKTOPINI_FILE_NAME             TEXT("desktop.ini")

#define DEFAULT_FILE_EXTENSION           ""

// Cache configuration and signature.
#define CACHE_SIGNATURE_VALUE           TEXT("Signature")
#define CACHE_SIGNATURE                 TEXT("Client UrlCache MMF Ver 5.2")
#define NUM_HEADER_DATA_DWORDS          (CACHE_HEADER_DATA_LAST + 1)
#define MAX_SIG_SIZE                     (sizeof(CACHE_SIGNATURE) / sizeof(TCHAR))

// The following values parametrize the schema for URL entries.
#define ENTRY_COPYSIZE_IE5    \
    (sizeof(IE5_URL_FILEMAP_ENTRY) - sizeof(FILEMAP_ENTRY))
#define ENTRY_VERSION_IE5               0
#define ENTRY_COPYSIZE_CURRENT          ENTRY_COPYSIZE_IE5
#define ENTRY_VERSION_CURRENT           ENTRY_VERSION_IE5

// If IE5-IE? sees an entry with low bits of version set, it will be placed
// on async fixup list rather than being destroyed.
#define ENTRY_VERSION_NONCOMPAT_MASK    0x0F


// Roundup
#define ROUNDUPTOPOWEROF2(bytesize, powerof2) (((bytesize) + (powerof2) - 1) & ~((powerof2) - 1))
#define ROUNDUPBLOCKS(bytesize) ((bytesize + NORMAL_ENTRY_SIZE-1) & ~(NORMAL_ENTRY_SIZE-1))
#define ROUNDUPDWORD(bytesize) ((bytesize + sizeof(DWORD)-1) & ~(sizeof(DWORD)-1))
#define ROUNDUPPAGE(bytesize) ((bytesize + PAGE_SIZE-1) & ~(PAGE_SIZE-1))
#define NUMBLOCKS(bytesize) (bytesize / NORMAL_ENTRY_SIZE)

// Power of 2 macros
#define ISPOWEROF2(val) (!((val) & ((val)-1)))
#define ASSERT_ISPOWEROF2(val) INET_ASSERT(ISPOWEROF2(val))

#define URL_CACHE_VERSION_NUM  sizeof(CACHE_ENTRY_INFO);

// Default profiles directory under %SystemRoot%.
#define DEFAULT_PROFILES_DIRECTORY TEXT("Profiles")


//
// Registry key and value names for persistent URL management.
//

// BUGBUG - wasting space. 

#define MS_BASE TEXT("Software\\Microsoft")

#define CV_BASE MS_BASE TEXT("\\Windows\\CurrentVersion")

#define EX_BASE TEXT("\\Explorer")

#define IS_BASE TEXT("\\Internet Settings")

#define CACHE_T TEXT("\\Cache")

#define SHELL_FOLDER_KEY        CV_BASE EX_BASE TEXT("\\Shell Folders")
#define USER_SHELL_FOLDER_KEY   CV_BASE EX_BASE TEXT("\\User Shell Folders")
#define CACHE5_KEY              CV_BASE IS_BASE TEXT("\\5.0") CACHE_T
#define OLD_CACHE_KEY           CV_BASE IS_BASE CACHE_T
#define CACHE_PATHS_FULL_KEY    CV_BASE IS_BASE CACHE_T TEXT("\\Paths")
#define RUN_ONCE_KEY            CV_BASE TEXT("\\RunOnce")
#define PROFILELESS_USF_KEY     TEXT(".Default\\") USER_SHELL_FOLDER_KEY
#define CONTENT_CACHE_HARD_NAME TEXT("Content.IE5")
#define OLD_VERSION_KEY         MS_BASE TEXT("\\IE Setup\\SETUP")
#define OLD_VERSION_VALUE       TEXT("UpgradeFromIESysFile")


//
// Cache parameters
//
#ifndef unix
#define PATH_CONNECT_STRING                    TEXT("\\")
#define PATH_CONNECT_CHAR                      TEXT('\\')
#define ALLFILES_WILDCARD_STRING               TEXT("*.*")
#define UNIX_RETURN_IF_READONLY_CACHE 
#define UNIX_RETURN_ERR_IF_READONLY_CACHE(error) 
#define UNIX_NORMALIZE_PATH_ALWAYS(szOrigPath, szEnvVar)
#define UNIX_NORMALIZE_IF_CACHE_PATH(szOrigPath, szEnvVar, szKeyName)
#else
#define PATH_CONNECT_STRING                    TEXT("/")
#define PATH_CONNECT_CHAR                      TEXT('/')
#define ALLFILES_WILDCARD_STRING               TEXT("*")
#define UNIX_RETURN_IF_READONLY_CACHE   {                        \
                                           if (g_ReadOnlyCaches) \
                                              return;            \
                                        }
#define UNIX_RETURN_ERR_IF_READONLY_CACHE(error) {                      \
                                                   if (g_ReadOnlyCaches)\
                                                      return (error);   \
                                               }
#define UNIX_NORMALIZE_PATH_ALWAYS(szOrigPath, szEnvVar) \
                                        UnixNormalisePath(szOrigPath, szEnvVar);
#define UNIX_NORMALIZE_IF_CACHE_PATH(szOrigPath, szEnvVar, szKeyName) \
                      UnixNormaliseIfCachePath(szOrigPath, szEnvVar, szKeyName);

#define UNIX_SHARED_CACHE_PATH TEXT("%HOME%/.microsoft")
#endif /* !unix */

#define CACHE_PERSISTENT                TEXT("Persistent")

// Retrieval methods
#define RETRIEVE_WITHOUT_CHECKS     0
#define RETRIEVE_WITH_CHECKS        1
#define RETRIEVE_WITH_ALLOCATION    2
#define RETRIEVE_ONLY_FILENAME      4
#define RETRIEVE_ONLY_STRUCT_INFO   8
//
// Multiple URL containters can be configured under the above key such
// as :
//
//  Cache\Paths\Path1
//  Cache\Paths\Path2
//    ...
//
// Each containter will have the following two parameters.
//

// CConMgr related defines.
#define CACHE_PATHS_KEY                 TEXT("Paths")
#define CACHE_PATH_VALUE                TEXT("CachePath")
#define CACHE_PATH_VALUE_TYPE           REG_SZ
#define CACHE_LIMIT_VALUE               TEXT("CacheLimit")
#define CACHE_LIMIT_VALUE_TYPE          REG_DWORD
#define CACHE_OPTIONS_VALUE             TEXT("CacheOptions")
#define CACHE_OPTIONS_VALUE_TYPE        REG_DWORD

#define EXTENSIBLE_CACHE_PATH_KEY       "Extensible Cache"
#define CONTENT_PATH_KEY                "Content"
#define COOKIE_PATH_KEY                 "Cookies"
#define HISTORY_PATH_KEY                "History"
#define URL_HISTORY_KEY                 "Url History"

#define PER_USER_KEY                    "PerUserItem"
#define PROFILES_ENABLED_VALUE          "Network\\Logon"
#define PROFILES_ENABLED                "UserProfiles"
#define PROFILES_PATH_VALUE             CV_BASE "\\ProfileList"
#define PROFILES_PATH                   "ProfileImagePath"

#define CONTENT_PREFIX                  ""
#define COOKIE_PREFIX                   "Cookie:"
#define HISTORY_PREFIX                  "Visited:" 

#define CONTENT_VERSION_SUBDIR          "Content.IE5"
#define IE3_COOKIES_PATH_KEY            OLD_CACHE_KEY TEXT("\\Special Paths\\Cookies")
#define IE3_HISTORY_PATH_KEY            OLD_CACHE_KEY TEXT("\\Special Paths\\History")
#define IE3_PATCHED_USER_KEY            TEXT("Patched User")
#define CACHE_SPECIAL_PATHS_KEY         TEXT("Special Paths")
#define CACHE_DIRECTORY_VALUE           TEXT("Directory")
#define CACHE_DIRECTORY_TYPE            REG_EXPAND_SZ
#define CACHE_NEWDIR_VALUE              TEXT("NewDirectory")
#define CACHE_NEWDIR_TYPE               REG_EXPAND_SZ
#define CACHE_PREFIX_VALUE	            TEXT("CachePrefix")
#define CACHE_PREFIX_MAP_VALUE          "PrefixMap"
#define CACHE_VOLUME_LABLE_VALUE        "VolumeLabel"
#define CACHE_VOLUME_TITLE_VALUE        "VolumeTitle"
#define CACHE_PREFIX_TYPE               REG_SZ
#define NEW_DIR                         TEXT("NewDirectory")
#define USER_PROFILE_SZ                 "%USERPROFILE%"
#define USER_PROFILE_LEN                (sizeof(USER_PROFILE_SZ) - 1)

// URL_CONTAINER related defines.
#define DEF_NUM_PATHS                   4
#define DEF_CACHE_LIMIT                 (2048 * DEF_NUM_PATHS)
#define NO_SPECIAL_CONTAINER            0xffffffff
#define MAX_ENTRY_SIZE                  0xFFFF
#define LONGLONG_TO_FILETIME( _p_ )     ((FILETIME *)(_p_))

// Content limit defines.
#define OLD_CONTENT_QUOTA_DEFAULT_DISK_FRACTION      64
#define NEW_CONTENT_QUOTA_DEFAULT_DISK_FRACTION      32
#define CONTENT_QUOTA_ADJUST_CHECK                   "QuotaAdjustCheck"

// CInstCon related defines.
#define INTERNET_CACHE_CONTAINER_PREFIXMAP INTERNET_CACHE_CONTAINER_RESERVED1
#define MAX_FILE_SIZE_TO_MIGRATE  50000
#define MAX_EXTENSION_LEN        3


// FileMgr related defines.
#define DEFAULT_DIR_TABLE_GROW_SIZE     4
#define DEFAULT_MAX_DIRS                32
#define MAX_FILES_PER_CACHE_DIRECTORY   1024
#define MAX_COLLISSION_ATTEMPTS         150
#define INSTALLED_DIRECTORY_KEY         0xFF
#define NOT_A_CACHE_SUBDIRECTORY        0XFE

#ifdef CHECKLOCK_PARANOID
void CheckEnterCritical(CRITICAL_SECTION *_cs);
void CheckLeaveCritical(CRITICAL_SECTION *_cs);
#define ENTERCRITICAL CheckEnterCritical
#define LEAVECRITICAL CheckLeaveCritical
#else
#define ENTERCRITICAL EnterCriticalSection
#define LEAVECRITICAL LeaveCriticalSection
#endif

// Cache global variable lock -- this should not be entered while holding
// lower-level locks like URL_CONTAINER::LockContainer cross-process mutex.
#define LOCK_CACHE()                    ENTERCRITICAL( &GlobalCacheCritSect )
#define UNLOCK_CACHE()                  LEAVECRITICAL( &GlobalCacheCritSect )

//
// parameter check macros.
//

#define IsBadUrl( _x_ )               IsBadStringPtrA( _x_, (DWORD) -1)
#define IsBadUrlW( _x_ )              IsBadStringPtrW( _x_, (DWORD) -1)
#define IsBadReadFileName( _x_ )      IsBadStringPtr( _x_, MAX_PATH )
#define IsBadWriteFileName( _x_ )     IsBadWritePtr( (PVOID)_x_, MAX_PATH)
#define IsBadWriteBoolPtr( _x_ )      IsBadWritePtr( _x_, sizeof(BOOL))
#define IsBadReadUrlInfo( _x_ )       IsBadReadPtr( _x_, sizeof(CACHE_ENTRY_INFO))
#define IsBadWriteUrlInfo( _x_, _y_ ) IsBadWritePtr( _x_, _y_ )

#define MAX_URL_ENTRIES                 (BIT_MAP_ARRAY_SIZE * sizeof(DWORD) * 8)

#define OFFSET_TO_POINTER( _ep_, _offset_) \
    (LPVOID)((LPBYTE)(_ep_) + (_offset_))


#define FIND_FLAGS_OLD_SEMANTICS                0x1
#define FIND_FLAGS_RETRIEVE_ONLY_STRUCT_INFO    0x2
#define FIND_FLAGS_RETRIEVE_ONLY_FIXED_AND_FILENAME 0x04

//---------------- BUGBUG : for History Only -------------------------------
#define MAX_FILETIME   0x7fffffffffffffff
#define MAX_DOSTIME    -1
//---------------- END BUGBUG ----------------------------------------------


//
// ----------------- Allocation entry header -----------------------------//
//

#define SIG_FREE   0xbadf00d
#define SIG_ALLOC  0xdeadbeef

#define SIG_URL         ' LRU'   // URL_FILEMAP_ENTRY
#define SIG_REDIR       'RDER'   // REDR_FILEMAP_ENTRY
#define SIG_LEAK        'KAEL'   // URL_FILEMAP_ENTRY
#define SIG_GLIST       'GLST'   // LIST_GROUP_ENTRY

// signatures for entries placed on fixup list
#define SIG_UPDATE      ' DPU'   // URL_FILEMAP_ENTRY
#define SIG_DELETE      ' LED'   // URL_FILEMAP_ENTRY

enum MemMapStatus
{
    MEMMAP_STATUS_OPENED_EXISTING = 0,
    MEMMAP_STATUS_REINITIALIZED = 1
};

typedef struct FILEMAP_ENTRY
{
    DWORD dwSig;
    DWORD nBlocks;
}
    *LPFILEMAP_ENTRY;

struct LIST_FILEMAP_ENTRY : FILEMAP_ENTRY
{
    DWORD dwNext; // offset to next element in list
    DWORD nBlock; // sequence number for this block
};

//
// URL entry
//

struct IE5_URL_FILEMAP_ENTRY : FILEMAP_ENTRY
{
    LONGLONG LastModifiedTime;       // must be LONGLONG
    LONGLONG LastAccessedTime;       // should be DWORD
    DWORD    dostExpireTime;
    DWORD    dostPostCheckTime;

    DWORD    dwFileSize;
    DWORD    dwRedirHashItemOffset;  // ask DanpoZ

    DWORD    dwGroupOffset;

    union
    {
        DWORD  dwExemptDelta;   // for SIG_URL
        DWORD  dwNextLeak;      // for SIG_LEAK
    };
    
    DWORD    CopySize;               // should be WORD
    DWORD    UrlNameOffset;          // should be WORD
    
    BYTE     DirIndex;           // subdirectory bucket
    BYTE     bSyncState;         // automatic sync mode state
    BYTE     bVerCreate;         // cache version that created this entry
    BYTE     bVerUpdate;         // cache version last updated this entry
        
    DWORD    InternalFileNameOffset; // should be WORD
    DWORD    CacheEntryType;
    DWORD    HeaderInfoOffset;       // should be WORD
    DWORD    HeaderInfoSize;         // should be WORD
    DWORD    FileExtensionOffset;    // should be WORD
    DWORD    dostLastSyncTime;       
    DWORD    NumAccessed;            // should be WORD
    DWORD    NumReferences;          // should be WORD
    DWORD    dostFileCreationTime;   // should be LONGLONG?

// Do not extend this structure; use inheritance instead.
};

typedef IE5_URL_FILEMAP_ENTRY URL_FILEMAP_ENTRY, *LPURL_FILEMAP_ENTRY;

// FILETIME is measured in 100-ns units.
#define FILETIME_SEC    ((LONGLONG) 10000000)
#define FILETIME_DAY    (FILETIME_SEC * 60 * 60 * 24)

// Possible values for bSyncState:
#define SYNCSTATE_VOLATILE   0 // once zero, stuck at zero
#define SYNCSTATE_IMAGE      1 // eligible to increment after MIN_AGESYNC
#define SYNCSTATE_STATIC     6 // max value

// Parameters controlling transition from _IMAGE to _VOLATILE.
// #define MIN_AGESYNC  ((LONGLONG) 5 * 60 * 10000000)  // 5 min in filetime
#define MIN_AGESYNC     (FILETIME_DAY * 7)

//
// Redirect Entry
//

struct REDIR_FILEMAP_ENTRY : FILEMAP_ENTRY
{
    DWORD dwItemOffset;  // offset to hash table item of destination URL
    DWORD dwHashValue;   // destination URL hash value (BUGBUG: collisions?)
    char  szUrl[4];      // original URL, can occupy more bytes
};

//
// Group Record
//

typedef struct GROUP_ENTRY
{
    GROUPID  gid;
    DWORD    dwGroupFlags;
    DWORD    dwGroupType;
    LONGLONG llDiskUsage;       // in Bytes (Actual Usage)
    DWORD    dwDiskQuota;       // in KB
    DWORD    dwGroupNameOffset;
    DWORD    dwGroupStorageOffset;
}
    *LPGROUP_ENTRY;


#define PAGE_SIZE_FOR_GROUPS    (PAGE_SIZE - sizeof(FILEMAP_ENTRY))
#define GROUPS_PER_PAGE         PAGE_SIZE_FOR_GROUPS / sizeof(GROUP_ENTRY)

typedef struct _GROUP_DATA_ENTRY
{
    CHAR    szName[GROUPNAME_MAX_LENGTH];
    DWORD   dwOwnerStorage[GROUP_OWNER_STORAGE_SIZE];
    DWORD   dwOffsetNext;
} GROUP_DATA_ENTRY, *LPGROUP_DATA_ENTRY;

#define GROUPS_DATA_PER_PAGE    PAGE_SIZE_FOR_GROUPS / sizeof(GROUP_DATA_ENTRY)

//
// so the sizeof(GROUPS_PAGE_FILEMAP_ENTRY) = PAGE_SIZE
// this is the allocation unit for groups entry
//
typedef struct _GROUPS_ALLOC_FILEMAP_ENTRY : FILEMAP_ENTRY
{
    BYTE    pGroupBlock[PAGE_SIZE_FOR_GROUPS];    
} GROUPS_ALLOC_FILEMAP_ENTRY, *LPGROUPS_ALLOC_FILEMAP_ENTRY;


typedef struct _LIST_GROUP_ENTRY 
{
    DWORD   dwGroupOffset;
    DWORD   dwNext;

} LIST_GROUP_ENTRY, *LPLIST_GROUP_ENTRY;

#define LIST_GROUPS_PER_PAGE    PAGE_SIZE_FOR_GROUPS / sizeof(LIST_GROUP_ENTRY)


#define SIGNATURE_CONTAINER_FIND 0xFAFAFAFA
#define SIG_CACHE_FIND 0XFBFBFBFB
#define SIG_GROUP_FIND 0XFCFCFCFC

typedef struct _CONTAINER_FIND_FIRST_HANDLE 
{
    DWORD dwSignature;
    DWORD dwNumContainers;
    DWORD dwContainer;
    LPSTR *ppNames;
    LPSTR *ppPrefixes;
    LPSTR *ppLabels;
    LPSTR *ppTitles;
    // DATA follows for Names, Prefixes, Volume labels and Volume titles.
} CONTAINER_FIND_FIRST_HANDLE, *LPCONTAINER_FIND_FIRST_HANDLE;

typedef struct _CACHE_FIND_FIRST_HANDLE 
{
    DWORD dwSig;
    BOOL  fFixed;
    DWORD nIdx;
    DWORD dwHandle;
    GROUPID GroupId;
    DWORD dwFilter;
    DWORD dwFlags;
} CACHE_FIND_FIRST_HANDLE, *LPCACHE_FIND_FIRST_HANDLE;

typedef struct _CACHE_STREAM_CONTEXT_HANDLE 
{
    HANDLE FileHandle;
    LPSTR SourceUrlName;
}  CACHE_STREAM_CONTEXT_HANDLE, *LPCACHE_STREAM_CONTEXT_HANDLE;

typedef struct _GROUP_FIND_FIRST_HANDLE : CACHE_FIND_FIRST_HANDLE
{
    DWORD dwLastItemOffset;
} GROUP_FIND_FIRST_HANDLE, *LPGROUP_FIND_FIRST_HANDLE;

#define OFFSET_NO_MORE_GROUP    -1
#define GID_INDEX_TO_NEXT_PAGE	-1
#define OFFSET_TO_NEXT_PAGE     -1

#define GID_MASK            0x0fffffffffffffff
#define GID_STICKY_BIT      0x1000000000000000

#define IsStickyGroup(gid)  (gid & GID_STICKY_BIT)
#define SetStickyBit(gid)   (gid | GID_STICKY_BIT)
#define IsInvalidGroup(gid) (gid & 0xE000000000000000)

//
// RealFileSize() - given the actual filesize,
// this macro computes the approximate real space that a file takes up
// on the disk. It only takes care of rounding to the cluster size
// It doesn't take into account any per-file overhead used in the filesystem
//

#define RealFileSize(fs)  ((LONGLONG) (fs + _ClusterSizeMinusOne) & _ClusterSizeMask)

#define MUTEX_DBG_TIMEOUT   5 * 1000    // 5 secs.

#define URLCACHE_OP_SET_STICKY   1
#define URLCACHE_OP_UNSET_STICKY 2

#ifdef unix
extern BOOL CreateAtomicCacheLockFile(BOOL *pfReadOnlyCaches, char **pszLockingHost);
extern BOOL DeleteAtomicCacheLockFile();
extern void UnixNormalisePath(LPTSTR pszOrigPath, LPCTSTR pszEnvVar);
extern void UnixNormaliseIfCachePath(LPTSTR pszOrigPath, LPCTSTR pszEnvVar,LPCTSTR szKeyName);
extern int  CopyDir(const char* src_dir, const char* dest_dir);
#endif /* unix */

extern VOID FileTime2DosTime(FILETIME, DWORD*);
extern VOID DosTime2FileTime(DWORD, FILETIME*);

#endif  // _CACHEDEF_

