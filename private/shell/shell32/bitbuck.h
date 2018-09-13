#ifndef _BITBUCK_INC
#define _BITBUCK_INC

#include "ids.h"
#include "undo.h"
#include "idlcomm.h" // for HIDA
#include "mrsw.h" // for MRSW

//
// whacky #defines
//
#define DELETEMAX 100000
#define MAX_BITBUCKETS 27
#define MAX_DRIVES 26
#define OPENFILERETRYTIME 500
#define OPENFILERETRYCOUNT 10
#define SERVERDRIVE 26
#define MAX_EMPTY_FILES     100     // if we have more that MAX_EMPTY_FILES then we use the generic "do you want to empty" message.  

#define TF_BITBUCKET 0x10000000
// #define TF_BITBUCKET TF_ERROR

#ifdef DEBUG
#define BB_DELETED_ENTRY_MAX 10     // smaller to force compaction more often
#else
#define BB_DELETED_ENTRY_MAX 100
#endif

#define BITBUCKET_DATAFILE_VERSION  0       // Ansi Win95 wastebasket
#define BITBUCKET_DATAFILE_VERSION2 2       // Unicode Win96/Cairo wastebasket
#define BITBUCKET_NT4_VERSION       2
#define BITBUCKET_WIN95_VERSION 0
#define BITBUCKET_CURRENT_WIN_VERSION  4    // win9x + IE4 integrated, win98
#define BITBUCKET_CURRENT_NT_VERSION  5     // NT4 + IE4 integrated, NT5

#define OPENBBINFO_READ                 0x00000000
#define OPENBBINFO_WRITE                0x00000001
#define OPENBBINFO_CREATE               0x00000003

#ifdef WINNT
#define BITBUCKET_CURRENT_VERSION BITBUCKET_CURRENT_NT_VERSION
#else
#define BITBUCKET_CURRENT_VERSION BITBUCKET_CURRENT_WIN_VERSION
#endif

#define PIDLTODATAENTRYID(pidl) ((LPBBDATAENTRYIDA)(((LPBYTE)pidl) + (((LPCITEMIDLIST)(pidl))->mkid.cb - SIZEOF(BBDATAENTRYIDA))))
#define BB_IsRealID(pidl) ( ((LPBYTE)pidl) < ((LPBYTE)PIDLTODATAENTRYID(pidl)))
#define DATAENTRYIDATOW(pbbidl) ((LPBBDATAENTRYIDW)(((LPBYTE)pbbidl) - FIELD_OFFSET(BBDATAENTRYIDW,cb)))

#define IsDeletedEntry(pbbde) (! (((BBDATAENTRYA*)pbbde)->szOriginal[0]) )
#define MarkEntryDeleted(pbbde) ((BBDATAENTRYA*)pbbde)->szOriginal[0] = '\0';

#define CHECK_PURGE_MAX 500
// the maximum we'll write to the info file at one time
#define MAX_WRITE_SIZE (((CHECK_PURGE_MAX + 1) * SIZEOF(BBDATAENTRY)) + SIZEOF(BBDATAHEADER))

//
// struct definitions
//

//
// BitBucketShellFolder stuff
//
typedef struct _CBitBucket      // dxi
{
#ifdef __cplusplus
    void *              ipf;
    void *              isf;
    void *              icm;
    void *              ips;
    void *              isei;
#else // __cplusplus
    IPersistFolder2     ipf;
    IShellFolder2       isf;
    IContextMenu        icm;
    IShellPropSheetExt  ips;
    IShellExtInit       isei;
#endif // __cplusplus
    UINT                cRef;
    LPITEMIDLIST        pidl;
} CBitBucket;


// this is the old (win95) data header.  it's maintained in the info file
// but only used for verification.  for current stuff look at the driveinfo,
// which is kept in the registry.
typedef struct {

    int idVersion;
    int cFiles;                     // the # of items in this drive's recycle bin
    int cCurrent;                   // the current file number.
    UINT cbDataEntrySize;           // size of each entry
    DWORD dwSize;                   // total size of this recycle bin drive
} BBDATAHEADER, * LPBBDATAHEADER;

typedef struct {

    CHAR szOriginal[MAX_PATH];  // original filename (if szOriginal[0] is 0, then it's a deleted entry)
    int  iIndex;                // index (key to name)
    int idDrive;                // which drive bucket it's currently in
    FILETIME ft;
    DWORD dwSize;
    // shouldn't need file attributes because we did a move file
    // which should have preserved them.
} BBDATAENTRYA, * LPBBDATAENTRYA;

typedef struct {
    CHAR szShortName[MAX_PATH]; // original filename, shortened (if szOriginal[0] is 0, then it's a deleted entry)
    int iIndex;                 // index (key to name)
    int idDrive;                // which drive bucket it's currently in
    FILETIME ft;
    DWORD dwSize;
    WCHAR szOriginal[MAX_PATH]; // original filename
} BBDATAENTRYW, * LPBBDATAENTRYW;

typedef struct {
    WORD cb;
    BBDATAENTRYA bbde;
} BBDATAENTRYIDA;
typedef UNALIGNED BBDATAENTRYIDA * LPBBDATAENTRYIDA;

typedef struct {
    WCHAR wszOriginal[MAX_PATH];
    WORD cb;
    BBDATAENTRYA bbde;
} BBDATAENTRYIDW;
typedef UNALIGNED BBDATAENTRYIDW * LPBBDATAENTRYIDW;

#ifdef UNICODE
#define BBDATAENTRY     BBDATAENTRYW
#define LPBBDATAENTRY   LPBBDATAENTRYW
#define BBDATAENTRYID   BBDATAENTRYIDW
#define LPBBDATAENTRYID LPBBDATAENTRYIDW
#else
#define BBDATAENTRY     BBDATAENTRYA
#define LPBBDATAENTRY   LPBBDATAENTRYA
#define BBDATAENTRYID   BBDATAENTRYIDA
#define LPBBDATAENTRYID LPBBDATAENTRYIDA
#endif

// On NT5 we are finally going to have cross-process syncrhonization to 
// the Recycle Bin. We replaced the global LPBBDRIVEINFO array with an
// array of the following structures:
typedef struct {
    BOOL fInited;               // is this particular BBSYNCOBJECT fully inited (needed when we race to create it)
#ifdef BB_USE_MRSW
    PMRSW pmrsw;                // Multiple-Reader-Single-Writer syncronization object for this bitbucket
#endif // BB_USE_MRSW
    HANDLE hgcNextFileNum;      // a global counter that garuntees unique deleted file names
    HANDLE hgcDirtyCount;       // a global counter to tell us if we need to re-read the bitbucket settings from the registry (percent, max size, etc)
    LONG lCurrentDirtyCount;    // out current dirty count; we compare this to hgcDirtyCount to see if we need to update the settings from the registry
    HKEY hkey;                  // HKLM reg key, under which we store the settings for this specific bucket (iPercent and fNukeOnDelete).
    HKEY hkeyPerUser;           // HKCU reg key, under which we have volital reg values indicatiog if there is a need to purge or compact this bucket

    BOOL fIsUnicode;            // is this a bitbucket on a drive whose INFO2 file uses BBDATAENTRYW structs?
    int iPercent;               // % of the drive to use for the bitbucket
    DWORD cbMaxSize;            // maximum size of bitbucket (in bytes), NOTE: we use a dword because the biggest the BB can ever grow to is 4 gigabytes.
    DWORD dwClusterSize;        // cluster size of this volume, needed to round all the file sizes
    ULONGLONG qwDiskSize;       // total size of the disk - takes into account quotas on NTFS
    BOOL fNukeOnDelete;         // I love the smell of napalm in the morning.

    LPITEMIDLIST pidl;          // pidl = bitbucket dir for this drive
    int cchBBDir;               // # of characters that makes up the bitbucket directory.

} BBSYNCOBJECT, *PBBSYNCOBJECT;


// The bitbucket datafile (INFO on win95, INFO2 on IE4/NT5) format is as follows:
//
// (binary writes)
//
//      BITBUCKETDATAHEADER     // header
//      BBDATAENTRY[X]          // array of BBDATAENTRYies
//
typedef struct _BitBucketDriveInfo
{
    int         cFiles;             // # of items in this drive's recycle bin)
    int         cCurrent;           // counts the current "delete operation"
    DWORD       dwSize;             // how much stuff is in this bit bucket
    UINT        cbDataEntrySize;    // size of each data entry


    int         iPercent;           // what percent of the drive to use
    ULONGLONG   cbMaxSize;          // maximum size of bitbucket (in bytes)
    DWORD       dwClusterSize;
    DWORD       dwTotalClusters;
    BOOL        fNukeOnDelete;

    LPITEMIDLIST pidl;              // points to the BitBucket dir for this drive

} BBDRIVEINFO, *LPBBDRIVEINFO;

typedef struct _RegBBDriveInfo // this is the info as stored in the registry
{
    int idVersion;
    BBDRIVEINFO bbdi;

} BBREGDRIVEINFO, *LPBBREGDRIVEINFO;

typedef struct {
    DWORD dwSize;
    BOOL fGlobal;
    WORD  wUsePercent[MAX_BITBUCKETS + 1];
    DWORD dwNukeOnDelete;  // BITFIELD

    DWORD dwDummy; // to force a new size
} BBREGINFO;

typedef struct {
    PROPSHEETPAGE psp;
    HIDA hida;
    LPCITEMIDLIST pidl;
} BBFILEPROPINFO;


typedef struct {
    CBitBucket *pbb;

    LPBBDATAENTRY lpbbde;
    DWORD grfFlags;
    HDPA hdpa;
    int nItem;

    // keep track of files not found but in info file
    DWORD unused1;
    DWORD unused2;

    DWORD dwUnused;

    int iUnused;

} ENUMDELETED, *LPENUMDELETED;

typedef struct _BB_FIND_DATA {
    int iIndex;
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    TCHAR   szType[ 5 ];        // Only the short filetype really (".xyz" + Nul)
} BBFINDDATA, *LPBBFINDDATA;

typedef struct {
    BOOL            bProcessedRoot; // tells if we are the first call in the recursive chain and we need to do root-specific processing
    int             cchBBDir;       // count of characters in the recycle bin dir (eg "C:\Recycler\<sid>")
    int             cchDelta;       // count of characters that the path will increase (or decrease if negative) by when moved under the recycle bin directory
    ULONGLONG       cbSize;         // size of the folder
    TCHAR           szNonDeletableFile[MAX_PATH];   // an output buffer that holds the name of the file that cannot be deleted, if one exists
    TCHAR           szPath[MAX_PATH];   // scratch buffer for stack savings when recursing
    WIN32_FIND_DATA fd;             // also for stack savings
} FOLDERDELETEINFO, *LPFOLDERDELETEINFO;


//
// constants
//
#define c_szInfo2           TEXT("INFO2")    // version 2 of the db file (used in IE4, win98, NT5, ...)
#define c_szInfo            TEXT("INFO")     // version 1 of the db file (used in win95, osr2, NT4)
#define c_szRecycled        TEXT("Recycled")
#define c_szPurgeInfo       TEXT("PurgeInfo")
#define c_szDStarDotStar    TEXT("D*.*")
#define c_szBitBucket       TEXT("BitBucket")
#define c_szRegRealMode     TEXT("Network\\Real Mode Net")
#define c_szWinDir          REGSTR_VAL_WINDIR
#define c_szRegSetup        REGSTR_PATH_SETUP


//
// globals
//
extern PBBSYNCOBJECT g_pBitBucket[MAX_BITBUCKETS];
extern HKEY g_hkBitBucket;
extern HANDLE g_hgcNumDeleters;

//
// prototypes (used mostly by bitbuck.c, bitbcksf.c and bitbuck1.cpp)
//
BOOL InitBBGlobals();
void BitBucket_Terminate();
STDAPI_(BOOL) IsBitBucketableDrive(int idDrive);
int DriveIDFromBBPath(LPNCTSTR pszPath);
void UpdateIcon(BOOL fFull);
void NukeFileInfoBeforePoint(HANDLE hfile, LPBBDATAENTRYW pbbdew, DWORD dwDataEntrySize);
BOOL ReadNextDataEntry(HANDLE hfile, LPBBDATAENTRYW pbbde, BOOL fSkipDeleted, DWORD dwDataEntrySize, int idDrive);
void CloseBBInfoFile(HANDLE hFile, int idDrive);
HANDLE OpenBBInfoFile(int idDrive, DWORD dwFlags, int iRetryCount);
int BBPathToIndex(LPCTSTR pszPath);
void GetDeletedFileName(LPTSTR pszFileName, int idDrive, int iIndex, LPNTSTR pszOriginal);
void DriveIDToBBPath(int idDrive, LPTSTR pszPath);
IShellFolderViewCB* BitBucket_CreateSFVCB(IShellFolder* psf, CBitBucket* pBBFolder);
void DriveIDToBBRoot(int idDrive, LPTSTR szPath);
void DriveIDToBBVolumeRoot(int idDrive, LPTSTR szPath);
BOOL GetNetHomeDir(LPTSTR pszPath);
BOOL PersistBBDriveSettings(int idDrive, int iPercent, BOOL fNukeOnDelete);
BOOL MakeBitBucket(int idDrive);
DWORD PurgeBBFiles(int idDrive);
BOOL PersistGlobalSettings(BOOL fUseGlobalSettings, BOOL fNukeOnDelete, int iPercent);
BOOL RefreshAllBBDriveSettings();
BOOL RefreshBBDriveSettings(int idDrive);
void CheckCompactAndPurge();
void BBPurgeAll(CBitBucket * that, HWND hwndOwner, DWORD dwFlags);
void BBSort(HWND hwndOwner, int id);
STDAPI BBHandleFSNotify(HWND hwndOwner, LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
void BBInitializeViewWindow(HWND hwndView);
BOOL BBDeleteFile(LPTSTR lpszFile, LPINT lpiReturn, LPUNDOATOM lpua, BOOL fIsDir, WIN32_FIND_DATA *pfd);
BOOL IsFileInBitBucket(LPCTSTR pszPath);
void UndoBBFileDelete(LPCTSTR lpszOriginal, LPCTSTR lpszDelFile);
BOOL BBWillRecycle(LPCTSTR lpszFile, INT* piRet);
void BBCheckRestoredFiles(LPTSTR lpszSrc);
STDAPI_(void) LaunchDiskCleanup(HWND hwnd, int idDrive);
STDAPI_(BOOL) GetDiskCleanupPath(LPTSTR pszBuf, UINT cbSize);

STDAPI_(BOOL) IsRecycleBinEmpty();
STDAPI_(void) SHUpdateRecycleBinIcon();
STDAPI_(void) SaveRecycleBinInfo();

#endif // _BITBUCK_INC
