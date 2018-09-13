void    AddToIconTable (LPCTSTR szFile, int iIconIndex, UINT uFlags, int iIndex);
void    RemoveFromIconTable(LPCTSTR szFile);
void    FlushIconCache(void);
int     GetFreeImageIndex(void);

void    Icon_FSEvent (LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra);

void    _IconCacheDump(void);       // DEBUG ONLY
void    _IconCacheFlush(BOOL fForce);
BOOL    _IconCacheSave(void);
BOOL    _IconCacheRestore(int cxIcon, int cyIcon, int cxSmIcon, int cySmIcon, UINT flags);

int     LookupIconIndex(LPCTSTR pszFile, int iIconIndex, UINT uFlags);
DWORD   LookupFileClass(LPCTSTR szClass);
void    AddFileClass(LPCTSTR szClass, DWORD dw);
void    FlushFileClass(void);

LPCTSTR  LookupFileClassName(LPCTSTR szClass);
LPCTSTR  AddFileClassName(LPCTSTR szClass, LPCTSTR szClassName);

//
// these GIL_ (GetIconLocation) flags are used when searching for a
// match in the icon table. all other flags are ignored (when searching
// for a match)
//
#define GIL_COMPARE (GIL_SIMULATEDOC|GIL_NOTFILENAME)


//
//  Icon Time
//
//  all file/icons in the location table are "time stamped"
//  each time they are accessed.
//
//  this way we know the most important ones (MRU)
//
//  when the icon cache get tooooo big we sort them all
//  and throw out the old ones.
//

#define ICONTIME_ZERO   0
#define ICONTIME_MAX    0xFFFFFFFF

extern DWORD IconTimeBase;      // base time.
extern DWORD IconTimeFlush;     // time of last flush

//
//  GetIconTime() returns the "clock" used to timestamp icons
//  in the icon table for MRU.  the clock incrments once every 1024ms
//  (about once every second)
//
#define GetIconTime()   (IconTimeBase + (GetTickCount() >> 10))

//
//  g_MaxIcons is limit on the number of icons in the cache
//  when we reach this limit we will start to throw icons away.
//
extern int g_MaxIcons;               // panic limit for cache size
#ifdef DEBUG
#define DEF_MAX_ICONS   200         // to test the flush code more offten
#else
#define DEF_MAX_ICONS   500         // normal end user number, BUGBUG read from registry??
#endif

//
// refreshes g_MaxIcons from registry.  returns TRUE if value changed.
//
BOOL QueryNewMaxIcons(void);

//
//  MIN_FLUSH is the minimum time interval between flushing the icon cache
//  this number is in IconTime
//
#define MIN_FLUSH   900         // 900 == about 15min

//
// g_iLastSysIcon is an indicator that is used to help determine which icons
// should be flushed and which icons shouldn't.  In the EXPLORER.EXE process,
// the first 40 or so icons should be saved.  On all other processes, only
// the icon overlay's should be saved.
extern UINT g_iLastSysIcon;

//------------------ location table ------------------------

typedef struct {
    LPCTSTR  szName;     // key: file name
    int     iIconIndex; // key: icon index (or random DWORD for GIL_NOTFILE)
    UINT    uFlags;     // GIL_* flags
    int     iIndex;     // data: system image list index
    UINT    Access;     // last access.
} LOCATION_ENTRY, *PLOCATION_ENTRY;

// LOCATION_ENTRY32 is the version of LOCATION_ENTRY that gets written to disk
// It must be declared explicitly 32-bit for Win32/Win64 interop.
typedef struct {
    DWORD   dwszName;   // (garbage in file)
    int     iIconIndex; // key: icon index (or random DWORD for GIL_NOTFILE)
    UINT    uFlags;     // GIL_* flags
    int     iIndex;     // data: system image list index
    UINT    Access;     // last access.
} LOCATION_ENTRY32, *PLOCATION_ENTRY32;

typedef struct {
    DWORD cbSize;       // size of this header.
    DWORD uMagic;       // magic number
    DWORD uVersion;     // version of this saved icon cache
    DWORD dwBuild;      // windows build number
    DWORD uNumIcons;    // number of icons in cache
    DWORD uColorRes;    // color resolution of device at last save
    DWORD flags;        // ILC_* flags
    DWORD cxIcon;       // x icon size of cache
    DWORD cyIcon;       // y icon size of cache
    DWORD cxSmIcon;     // x icon size of cache
    DWORD cySmIcon;     // y icon size of cache
    DWORD TimeSave;     // icon time this file was saved
    DWORD TimeFlush;    // icon time we last flushed.
    DWORD FreeImageCount;
    DWORD FreeEntryCount;
} IC_HEAD;

#define ICONCACHE_MAGIC  (TEXT('W') + (TEXT('i') << 8) + (TEXT('n') << 16) + (TEXT('4') << 24))
#ifdef UNICODE
#define ICONCACHE_VERSION 0x0403        // Unicode file names
#else
#define ICONCACHE_VERSION 0x0402        // Ansi file names
#endif
