/*
 * fcache.h - File cache ADT description.
 */


/* Types
 ********/

/* return code */

typedef enum _fcresult
{
   FCR_SUCCESS,
   FCR_OUT_OF_MEMORY,
   FCR_OPEN_FAILED,
   FCR_CREATE_FAILED,
   FCR_WRITE_FAILED,
   FCR_FILE_LOCKED
}
FCRESULT;
DECLARE_STANDARD_TYPES(FCRESULT);

/* handles */

#ifdef NOFCACHE
typedef HANDLE HCACHEDFILE;
#else
DECLARE_HANDLE(HCACHEDFILE);
#endif
DECLARE_STANDARD_TYPES(HCACHEDFILE);

/* cached file description */

typedef struct _cachedfile
{
   LPCTSTR pcszPath;

   DWORD dwcbDefaultCacheSize;

   DWORD dwOpenMode;

   DWORD dwSharingMode;

   PSECURITY_ATTRIBUTES psa;

   DWORD dwCreateMode;

   DWORD dwAttrsAndFlags;

   HANDLE hTemplateFile;
}
CACHEDFILE;
DECLARE_STANDARD_TYPES(CACHEDFILE);


/* Prototypes
 *************/

/* fcache.c */

extern FCRESULT CreateCachedFile(PCCACHEDFILE, PHCACHEDFILE);
extern FCRESULT SetCachedFileCacheSize(HCACHEDFILE, DWORD);
extern DWORD SeekInCachedFile(HCACHEDFILE, DWORD, DWORD);
extern BOOL SetEndOfCachedFile(HCACHEDFILE);
extern DWORD GetCachedFilePointerPosition(HCACHEDFILE);
extern DWORD GetCachedFileSize(HCACHEDFILE);
extern BOOL ReadFromCachedFile(HCACHEDFILE, PVOID, DWORD, PDWORD);
extern BOOL WriteToCachedFile(HCACHEDFILE, PCVOID, DWORD, PDWORD);
extern BOOL CommitCachedFile(HCACHEDFILE);
extern HANDLE GetFileHandle(HCACHEDFILE);
extern BOOL CloseCachedFile(HCACHEDFILE);
extern HANDLE GetFileHandle(HCACHEDFILE);

#if defined(DEBUG) || defined(VSTF)

extern BOOL IsValidHCACHEDFILE(HCACHEDFILE);

#endif

