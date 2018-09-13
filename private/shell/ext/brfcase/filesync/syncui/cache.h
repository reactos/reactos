//
// cache.h: Declares data, defines and struct types for the
//          cache list module.
//
//

#ifndef __CACHE_H__
#define __CACHE_H__


/////////////////////////////////////////////////////////////
//  
//  Generic cache structures
//
/////////////////////////////////////////////////////////////


typedef void (CALLBACK *PFNFREEVALUE)(void * pv, HWND hwndOwner);

typedef struct tagCACHE
    {
    CRITICAL_SECTION cs;
    HDSA hdsa;          // Actual list of CITEMs
    HDPA hdpa;          // Sorted ptr list
    HDPA hdpaFree;      // Free list
    int iPrev;          // Index into hdpa.  Used by FindFirst/FindNext
    int atomPrev;
    } CACHE;

// Generic cache APIs
//
BOOL    PUBLIC Cache_Init (CACHE  * pcache);
void    PUBLIC Cache_InitCS(CACHE  * pcache);
void    PUBLIC Cache_Term (CACHE  * pcache, HWND hwndOwner, PFNFREEVALUE pfnFree);
void    PUBLIC Cache_DeleteCS(CACHE  * pcache);
BOOL    PUBLIC Cache_AddItem (CACHE  * pcache, int atomKey, LPVOID pvValue);
int     PUBLIC Cache_DeleteItem (CACHE  * pcache, int atomKey, BOOL bNuke, HWND hwndOwner, PFNFREEVALUE pfnFree);
BOOL    PUBLIC Cache_ReplaceItem (CACHE  * pcache, int atomKey, LPVOID pvBuf, int cbBuf);
LPVOID  PUBLIC Cache_GetPtr (CACHE  * pcache, int atomKey);
BOOL    PUBLIC Cache_GetItem(CACHE  * pcache, int atomKey, LPVOID pvBuf, int cbBuf);
int     PUBLIC Cache_FindFirstKey(CACHE  * pcache);
int     PUBLIC Cache_FindNextKey(CACHE  * pcache, int atomPrev);
UINT    PUBLIC Cache_GetRefCount(CACHE  * pcache, int atomKey);


/////////////////////////////////////////////////////////////
//  
//  Cached briefcase handle list
//
/////////////////////////////////////////////////////////////

// Cache briefcase structure 
//
typedef struct tagCBS
    {
    int      atomBrf;           // Useful for reference
    HBRFCASE hbrf;              // Opened on add, closed on delete
    HWND     hwndParent;        // Volatile
    PABORTEVT pabortevt;        // Abort event object
    UINT     uFlags;            // One of CBSF_ flags

    } CBS, * PCBS;

#define CBSF_RUNWIZARD      0x0001
#define CBSF_LFNDRIVE       0x0002

extern CACHE g_cacheCBS;        // Briefcase structure cache


void CALLBACK CBS_Free(LPVOID lpv, HWND hwnd);

DEBUG_CODE( void PUBLIC CBS_DumpAll(); )

//      BOOL CBS_Init(void);
//
#define CBS_Init()                      Cache_Init(&g_cacheCBS)

//      void CBS_InitCS(void);
//
#define CBS_InitCS()                    Cache_InitCS(&g_cacheCBS)

//      void CBS_Term(HWND hwndOwner);
//
#define CBS_Term(hwndOwner)             Cache_Term(&g_cacheCBS, hwndOwner, CBS_Free)

//      void CBS_DeleteCS(void);
//
#define CBS_DeleteCS()                  Cache_DeleteCS(&g_cacheCBS)

//      HRESULT CBS_Add(PCBS * ppcbs, int atomPath, HWND hwndOwner);
//          Must call CBS_Delete for each call to this guy.
//
HRESULT PUBLIC CBS_Add(PCBS * ppcbs, int atomPath, HWND hwndOwner);

//      CBS FAR * CBS_Get(int atomPath);
//          Must call CBS_Delete for each call to this guy.
//
#define CBS_Get(atomPath)               Cache_GetPtr(&g_cacheCBS, atomPath)

//      int CBS_Delete(int atomPath, HWND hwndOwner);
//          Returns reference count (0 if deleted)
//
#define CBS_Delete(atomPath, hwndOwner) Cache_DeleteItem(&g_cacheCBS, atomPath, FALSE, hwndOwner, CBS_Free)

//      int CBS_Nuke(int atomPath, HWND hwndOwner);
//          Returns 0
//
#define CBS_Nuke(atomPath, hwndOwner)   Cache_DeleteItem(&g_cacheCBS, atomPath, TRUE, hwndOwner, CBS_Free)


/////////////////////////////////////////////////////////////
//  
//  Cached reclist 
//
/////////////////////////////////////////////////////////////

// Cache reclist structure
//
typedef struct tagCRL
    {
    int atomPath;           // Inside path for this CRL
    int atomOutside;        // Outside path of the sync copy pair
    UINT idsStatus;         // resource ID for status string

    PABORTEVT pabortevt;    // Abort event object, owned by CBS
    HBRFCASE hbrf;          // Briefcase this reclist belongs to
    int atomBrf;
    PRECLIST lprl;          // Created
    PFOLDERTWINLIST lpftl;  // Created.  May be NULL
    UINT ucUse;             // Use count (dirty entry is not cleaned until 
                            //   ucUse == 0)
    UINT uFlags;            // CRLF_* flags
    } CRL, * PCRL;

// Flags for CRL
#define CRLF_DIRTY          0x00000001      // cache item is dirty
#define CRLF_NUKE           0x00000002      // nuke when use count is 0
#define CRLF_SUBFOLDERTWIN  0x00000004      // folder is subfolder of subtree twin
#define CRLF_ISFOLDER       0x00000008      // atomPath is a folder
#define CRLF_ISLFNDRIVE     0x00000010      // is on an LFN drive
#define CRLF_ORPHAN         0x00000020      // item is orphan

extern CACHE g_cacheCRL;        // Reclist cache

void CALLBACK CRL_Free(LPVOID lpv, HWND hwndOwner);

DEBUG_CODE( void PUBLIC CRL_DumpAll(); )

#define CRL_IsOrphan(pcrl)              IsFlagSet((pcrl)->uFlags, CRLF_ORPHAN)
#define CRL_IsSubfolderTwin(pcrl)       IsFlagSet((pcrl)->uFlags, CRLF_SUBFOLDERTWIN)
#define CRL_IsFolder(pcrl)              IsFlagSet((pcrl)->uFlags, CRLF_ISFOLDER)

//      BOOL CRL_Init(void);
//
#define CRL_Init()                      Cache_Init(&g_cacheCRL)

//      void CRL_InitCS(void);
//
#define CRL_InitCS()                    Cache_InitCS(&g_cacheCRL)

//      void CRL_Term(void);
//
#define CRL_Term()             Cache_Term(&g_cacheCRL, NULL, CRL_Free)

//      void CRL_DeleteCS(void);
//
#define CRL_DeleteCS()                  Cache_DeleteCS(&g_cacheCRL)

BOOL PUBLIC IsSubfolderTwin(HBRFCASE hbrf, LPCTSTR pcszPath);

//      HRESULT CRL_Add(PCBS pcbs, int atomPath);
//          Must call CRL_Delete for each call to this function.
//
HRESULT     PUBLIC CRL_Add(PCBS pcbs, int atomPath);

//      HRESULT CRL_Get(int atomPath, PCRL * ppcrl);
//          Must call CRL_Delete for each successful call to this function.
//
HRESULT     PUBLIC CRL_Get(int atomPath, PCRL * ppcrl);

//      HRESULT CRL_Replace(int atomPath);
//
HRESULT     PUBLIC CRL_Replace(int atomPath);

//      void CRL_Delete(int atomPath);
//
void        PUBLIC CRL_Delete(int atomPath);

//      int CRL_Nuke(int atomPath);
//
void        PUBLIC CRL_Nuke(int atomPath);

//      BOOL CRL_Dirty(int atomPath);
BOOL        PUBLIC CRL_Dirty(int atomPath, int atomCabinetFolder, LONG lEvent, LPBOOL pbRefresh);

//      void CRL_DirtyAll(int atomBrf);
//
void        PUBLIC CRL_DirtyAll(int atomBrf);


/////////////////////////////////////////////////////////////
//  
//  Cached briefcase paths
//
/////////////////////////////////////////////////////////////

typedef struct tagCPATH
    {
    int atomPath;           // Useful for reference 

    } CPATH;

extern CACHE g_cacheCPATH;        // Volume ID cache

void CALLBACK CPATH_Free(LPVOID lpv, HWND hwndOwner);

DEBUG_CODE( void PUBLIC CPATH_DumpAll(); )

//      BOOL CPATH_Init(void);
//
#define CPATH_Init()                    Cache_Init(&g_cacheCPATH)

//      void CPATH_InitCS(void);
//
#define CPATH_InitCS()                  Cache_InitCS(&g_cacheCPATH)

//      void CPATH_Term();
//
#define CPATH_Term()                    Cache_Term(&g_cacheCPATH, NULL, CPATH_Free)

//      void CPATH_DeleteCS(void);
//
#define CPATH_DeleteCS()                Cache_DeleteCS(&g_cacheCPATH)

//      CPATH FAR * CPATH_Replace(int atomPath);
//          Must call CPATH_Delete for each call to this function.
//
CPATH  *  PUBLIC CPATH_Replace(int atomPath);

//      UINT CPATH_GetLocality(LPCSTR pszPath, LPSTR pszBuf);
//
UINT    PUBLIC CPATH_GetLocality(LPCTSTR pszPath, LPTSTR pszBuf);

#endif // __CACHE_H__

