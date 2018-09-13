//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// cache.cpp 
//
//   XML document cache.
//
//   History:
//
//       4/15/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"
#include "persist.h"
#include "cache.h"
#include "cdfidl.h"
#include "xmlutil.h"
#include "dll.h"

//
// Cache functions.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** Cache_Initialize *** 
//
//   Prepare the XML document cache for use.
//
////////////////////////////////////////////////////////////////////////////////
void
Cache_Initialize(
    void
)
{
    ASSERT(NULL == g_pCache);

    InitializeCriticalSection(&g_csCache);

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** Cache_Initialize *** 
//
//   Deactivate the cache.
//
////////////////////////////////////////////////////////////////////////////////
void
Cache_Deinitialize(
    void
)
{
    // BUGBUG: MSXML has gone away at this point
    // Cache_FreeAll();

    DeleteCriticalSection(&g_csCache);

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** Cache_EnterWriteLock ***
//
//    Obtain exclusive use of the XML document cache.
//
////////////////////////////////////////////////////////////////////////////////
void
Cache_EnterWriteLock(
    void
)
{
    EnterCriticalSection(&g_csCache);

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** Cache_EnterWriteLock ***
//
//    Release exclusive use of the XML document cache.
//
////////////////////////////////////////////////////////////////////////////////
void
Cache_LeaveWriteLock(
    void
)
{
    LeaveCriticalSection(&g_csCache);

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** Cache_EnterReadLock ***
//
//    Exclude writes to the the items list.  Currently this also excludes other
//    reads.  If need be this can be modified to allow multiple reads while
//    still excluding writes.
//
////////////////////////////////////////////////////////////////////////////////
void
Cache_EnterReadLock(
    void
)
{
    EnterCriticalSection(&g_csCache);

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** Cache_LeaveReadLock ***
//
//    Release a read hold on the use of the XML document cache.
//
////////////////////////////////////////////////////////////////////////////////
void
Cache_LeaveReadLock(
    void
)
{
    LeaveCriticalSection(&g_csCache);

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** Cache_AddItem ***
//
//
// Description:
//     Add an xml document to the cache.
//
// Parameters:
//     [In]  szURL         - The URL of the cdf file.
//     [In]  pIXMLDocument - The already parsed xml document.
//
// Return:
//     S_OK if the document was added to the cache.
//     E_OUTOFMEMORY if the document couldn't be aded to the cache.
//
// Comments:
//     The xml document is AddRefed when inserted into the cache and
//     Released on removal from the cache.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
Cache_AddItem(
    LPTSTR szURL,
    IXMLDocument* pIXMLDocument,
    DWORD dwParseFlags,
    FILETIME ftLastMod,
    DWORD dwCacheCount
)
{
    ASSERT(szURL);
    ASSERT(pIXMLDocument);

    Cache_EnterWriteLock();

    HRESULT hr;

    PCACHEITEM pNewItem = new CACHEITEM;

    if (pNewItem)
    {
        LPTSTR szURLCopy = (LPTSTR)new TCHAR[(StrLen(szURL) + 1)];

        if (szURLCopy)
        {
            //
            // Limit the cache to one item by freeing all current items.
            //

            Cache_FreeAll();

            //
            // Remove an old cache entry for this url if it exists.
            //

            // Check no longer needed since we just cleared the cache.
            /*IXMLDocument* pIXMLDocumentOld;

            if (SUCCEEDED(Cache_QueryItem(szURL, &pIXMLDocumentOld,
                                          PARSE_LOCAL)))
            {
                ASSERT(pIXMLDocumentOld);

                Cache_RemoveItem(szURL);
                pIXMLDocumentOld->Release();
            }*/

            StrCpy(szURLCopy, szURL);

            pIXMLDocument->AddRef();

            pNewItem->szURL         = szURLCopy;
            pNewItem->dwParseFlags  = dwParseFlags;
            pNewItem->ftLastMod     = ftLastMod;
            pNewItem->dwCacheCount  = dwCacheCount;
            pNewItem->pIXMLDocument = pIXMLDocument;

            //
            // REVIEW:  Check for duplicate cache items?
            //

            pNewItem->pNext = g_pCache;
            g_pCache = pNewItem;


            hr = S_OK;
        }
        else
        {
            delete pNewItem;

            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    Cache_LeaveWriteLock();

    return hr;
}

//
//
//

BOOL
IsEmptyTime(
    FILETIME ft
)
{
    return (0 == ft.dwLowDateTime && 0 == ft.dwHighDateTime);
}

BOOL
IsEqualTime(
    FILETIME ft1,
    FILETIME ft2
)
{
    return ((ft1.dwLowDateTime == ft2.dwLowDateTime) && 
            (ft1.dwHighDateTime == ft2.dwHighDateTime));
}

void
Cache_RefreshItem(
    CACHEITEM* pItem,
    LPTSTR pszLocalFile,
    FILETIME ftLastMod
)
{
    ASSERT(pItem);

    //
    // Try and parse the cdf from the wininet cache.
    //

    IXMLDocument* pIXMLDocument;

    HRESULT hr;

    DLL_ForcePreloadDlls(PRELOAD_MSXML);
    
    hr = CoCreateInstance(CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                          IID_IXMLDocument, (void**)&pIXMLDocument);

    BOOL bCoInit = FALSE;

    if ((CO_E_NOTINITIALIZED == hr || REGDB_E_IIDNOTREG == hr) &&
        SUCCEEDED(CoInitialize(NULL)))
    {
        bCoInit = TRUE;
        hr = CoCreateInstance(CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                              IID_IXMLDocument, (void**)&pIXMLDocument);
    }

    if (SUCCEEDED(hr))
    {
        ASSERT(pIXMLDocument);

        hr = XML_SynchronousParse(pIXMLDocument, pszLocalFile);

        if (FAILED(hr))
            pIXMLDocument->Release();
    }

    if (bCoInit)
        CoUninitialize();

    //
    // If the new cdf was parsed, replace the old one.
    //

    if (SUCCEEDED(hr))
    {
        pItem->pIXMLDocument->Release();
        pItem->pIXMLDocument = pIXMLDocument;
        pItem->ftLastMod = ftLastMod;
    }

    return;
}

BOOL
Cache_IsItemFresh(
    CACHEITEM* pItem,
    DWORD dwParseFlags
)
{
    ASSERT(pItem);

    BOOL  fRet;
    DWORD dwCurrentCacheCount = g_dwCacheCount;                            

    //
    // If the caller asked for "Net" quality data and we only have "Local" data
    // then throw the "Local" data away.  The resultant cache miss will cause
    // the caller to pick up fresher data.
    //

    if ((dwParseFlags & PARSE_NET) && (pItem->dwParseFlags & PARSE_LOCAL))
    {
        fRet = FALSE;
    }
    else
    {
        fRet = TRUE;

        //
        // If the global cache counter is greater than the counter for this
        // item, then a cdf has been added to the cache.
        //

        if (dwCurrentCacheCount > pItem->dwCacheCount)
        {
            //
            // Get the last mod time from the the cdf in the wininet cache.
            //

            FILETIME ftLastMod;
            TCHAR    szLocalFile[MAX_PATH];

            if (SUCCEEDED(URLGetLocalFileName(pItem->szURL, szLocalFile,
                                ARRAYSIZE(szLocalFile), &ftLastMod)))
            {
                //
                //  If the last mod times are different then the cdf in the
                //  wininet cache is newer, pick it up.
                //  If there are no last modified times then do the conservative
                //  thing and pick up the cdf from the wininet cache.
                //

                if ((IsEmptyTime(ftLastMod) && IsEmptyTime(pItem->ftLastMod)) ||
                    !IsEqualTime(ftLastMod, pItem->ftLastMod))
                {
                    Cache_RefreshItem(pItem, szLocalFile, ftLastMod);
                }
            }

            pItem->dwCacheCount = dwCurrentCacheCount;
        }
    }

    return fRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** Cache_QueryItem ***
//
//
// Description:
//     Returns a xml document from the cache if it is found.
//
// Parameters:
//     [In]  szURL          - The URL associated with the xml document.
//     [Out] ppIXMLDocument - A pointer that receives the xml document.
//
// Return:
//     S_OK if the document associtaed with the given URL is found in the cache.
//     E_FAIL if the document isn't in the cache.
//
// Comments:
//     The returned pointer is AddRefed.  The caller isresposible for releasing
//     this pointer.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
Cache_QueryItem(
    LPTSTR szURL,
    IXMLDocument** ppIXMLDocument,
    DWORD dwParseFlags
)
{
    ASSERT(szURL);
    ASSERT(ppIXMLDocument);

    HRESULT hr = E_FAIL;

    Cache_EnterReadLock();

    PCACHEITEM pItem = g_pCache;

    //
    // REVIEW: Use CompareUrl from shlwapip?
    //

    while (pItem && !StrEql(szURL, pItem->szURL))
        pItem = pItem->pNext;

    if (pItem)
    {
        if (Cache_IsItemFresh(pItem, dwParseFlags))
        {
            ASSERT(pItem->pIXMLDocument);

            pItem->pIXMLDocument->AddRef();

            *ppIXMLDocument = pItem->pIXMLDocument;

            hr = S_OK;
        }
        else
        {
            Cache_RemoveItem(szURL);
        }

    }

    Cache_LeaveReadLock();

    ASSERT(SUCCEEDED(hr) && ppIXMLDocument || FAILED(hr));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** Cache_FreeAll ***
//
//
// Description:
//     Frees all items from the xml document cache.
//
// Parameters:
//     None.
//
// Return:
//     None.
//
// Comments:
//     Frees all memory held in the xml document cache.
//
////////////////////////////////////////////////////////////////////////////////
void
Cache_FreeAll(
    void
)
{
    Cache_EnterWriteLock();
 
    PCACHEITEM pItem = g_pCache;
    g_pCache = NULL;

    Cache_LeaveWriteLock();

    while (pItem)
    {
        PCACHEITEM pNext = pItem->pNext;

        ASSERT(pItem->szURL);
        ASSERT(pItem->pIXMLDocument);

        pItem->pIXMLDocument->Release();

        delete [] pItem->szURL;
        delete pItem;

        pItem = pNext;
    }

    return;
}
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** Cache_FreeItem ***
//
//
// Description:
//     Frees item associated with given URL from the xml document cache.
//
// Parameters:
//     LPTSTR szURL
//
// Return:
//     HRESULT S_OK if item in cache and deleted, E_FAIL if item not in cache
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
Cache_RemoveItem(
    LPCTSTR szURL
)
{
    ASSERT(szURL);

    HRESULT hr;

    Cache_EnterWriteLock();

    PCACHEITEM pItem = g_pCache;
    PCACHEITEM pItemPrev = NULL;

    //
    // REVIEW: Use CompareUrl from slwapip?.
    //

    while (pItem && !StrEql(szURL, pItem->szURL))
    {
        pItemPrev = pItem;
        pItem = pItem->pNext;
    }

    if (pItem)
    {
        ASSERT(pItem->pIXMLDocument);

        if (pItemPrev)
        {
            pItemPrev->pNext = pItem->pNext;
        }
        else
        {
            g_pCache = pItem->pNext; // handle remove first item case
        }

        pItem->pIXMLDocument->Release();
        delete [] pItem->szURL;
        delete pItem;

        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    Cache_LeaveWriteLock();

    return hr;
}
