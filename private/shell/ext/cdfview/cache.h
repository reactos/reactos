//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// cache.h 
//
//   XML document cache.
//
//   History:
//
//       4/15/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////


//
// Check for previous includes of this file.
//

#ifndef _CACHE_H_

#define _CACHE_H_

//
// Type definitions.
//

typedef struct _tagCACHEITEM
{
    LPTSTR          szURL;
    DWORD           dwParseFlags;
    FILETIME        ftLastMod;
    DWORD           dwCacheCount;
    IXMLDocument*   pIXMLDocument;
    _tagCACHEITEM*  pNext;
} CACHEITEM, *PCACHEITEM;

//
// Function prototypes.
//

void    Cache_Initialize(void);
void    Cache_Deinitialize(void);
void    Cache_EnterWriteLock(void);
void    Cache_LeaveWriteLock(void);
void    Cache_EnterReadLock(void);
void    Cache_LeaveReadLock(void);

HRESULT Cache_AddItem(LPTSTR szURL,
                      IXMLDocument* pIXMLDocument,
                      DWORD dwParseFlags,
                      FILETIME ftLastMod,
                      DWORD dwCacheCount);

HRESULT Cache_QueryItem(LPTSTR szURL,
                        IXMLDocument** ppIXMLDocument,
                        DWORD dwParseFlags);

HRESULT Cache_RemoveItem(LPCTSTR szURL);

void    Cache_FreeAll(void);

#endif // _CACHE_H_