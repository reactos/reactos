#include "pch.h"
#include "stddef.h"
#include "dsrole.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Class cache
/----------------------------------------------------------------------------*/

//
// Class cache state and functions
//

#define ALL_PREFIXED_ATTRIBUTES         \
            (CLASSCACHE_PROPPAGES|      \
             CLASSCACHE_CONTEXTMENUS)

#define ALL_NONPREFIXED_ATTRIBUTES      \
            (CLASSCACHE_ICONS|          \
             CLASSCACHE_FRIENDLYNAME|   \
             CLASSCACHE_TREATASLEAF|    \
             CLASSCACHE_ATTRIBUTENAMES| \
             CLASSCACHE_CREATIONINFO)
            
#define ALL_DISPLAY_SPEC_VALUES         \
            (CLASSCACHE_PROPPAGES|      \
             CLASSCACHE_CONTEXTMENUS|   \
             CLASSCACHE_ICONS|          \
             CLASSCACHE_FRIENDLYNAME|   \
             CLASSCACHE_TREATASLEAF|    \
             CLASSCACHE_ATTRIBUTENAMES| \
             CLASSCACHE_CREATIONINFO)

HANDLE g_hCacheLock = NULL;
BOOL g_fClassCacheSorted = FALSE;
HDPA g_hdpaClassCache = NULL;

INT _CompareCacheEntry(LPVOID p1, LPVOID p2, LPARAM lParam);
VOID _FreeCacheEntry(LPCLASSCACHEENTRY* ppCacheEntry);

//
// Cache fillers
//

HRESULT GetPropertyPageList(LPCLASSCACHEENTRY pCacheEntry, LPWSTR pPrefix, IADs* pDisplaySpecifier);
VOID FreePropertyPageList(HDSA* pHDSA);

HRESULT GetMenuHandlerList(LPCLASSCACHEENTRY pCacheEntry, LPWSTR pPrefix, IADs* pDisplaySpecifier);
VOID FreeMenuHandlerList(HDSA* pHDSA);

HRESULT GetIconList(LPCLASSCACHEENTRY pCacheEntry, IADs* pDisplaySpecifier);
VOID FreeIconList(LPCLASSCACHEENTRY pCacheEntry);

HRESULT GetAttributeNames(LPCLASSCACHEENTRY pCacheEntry, LPCLASSCACHEGETINFO pccgi, IADs* pDisplaySpecifier);
INT CALLBACK _FreeAttributeNameCB(LPVOID p, LPVOID pData);
VOID FreeAttributeNames(HDPA* pHDPA);


//
// Constant strings for the properties we expect
//

#define DISPLAY_SPECIFICATION L"displaySpecification"
#define PROPERTY_PAGES        L"propertyPages"
#define CONTEXT_MENU          L"contextMenu"
#define ICON_LOCATION         L"iconPath"
#define FRIENDLY_NAME         L"classDisplayName"
#define ATTRIBUTE_NAMES       L"attributeDisplayNames"
#define TREAT_AS_LEAF         L"treatAsLeaf"
#define CREATION_DIALOG       L"createDialog"
#define CREATION_WIZARD       L"creationWizard"
#define CREATION_WIZARD_EXTN  L"createWizardExt"


//
// property cache is used to store the property name (with optional server) and the ADsType.
//

HANDLE g_hPropCacheLock = NULL;
HDPA g_hdpaPropCache = NULL;

typedef struct
{
    LPWSTR pName;                       // property name (inc server if needed)
    ADSTYPE dwADsType;                  // attribute type
} PROPCACHEENTRY, * LPPROPCACHEENTRY;

INT _ComparePropCacheEntry(LPVOID p1, LPVOID p2, LPARAM lParam);
VOID _FreePropCacheEntry(LPPROPCACHEENTRY *ppCacheEntry);
HRESULT _GetDsSchemaMgmt(LPCLASSCACHEGETINFO pccgi, IDirectorySchemaMgmt **ppdsm);
HRESULT _AddPropToPropCache(LPCLASSCACHEGETINFO pccgi, IDirectorySchemaMgmt *pdsm, LPCWSTR pAttributeName, ADSTYPE *padt);


//
// cache unlock macros
//

#define UNLOCK_CLASS_CACHE              \
            ReleaseMutex(g_hCacheLock);

#define UNLOCK_PROP_CACHE               \
            ReleaseMutex(g_hPropCacheLock);

#define UNLOCK_CACHE_ENTRY(pcce)        \
            ReleaseMutex((pcce)->hLock)
        


/*-----------------------------------------------------------------------------
/ _FreeCacheEntry
/ ---------------
/   Cache entries are stored as a LocalAlloc pointed to be the DPA.  Here
/   we tidy up such allocations.
/
/ In:
/   ppCacheEntry = pointer to block to be free'd
/
/ Out:
/   VOID
/----------------------------------------------------------------------------*/
VOID _FreeCacheEntry(LPCLASSCACHEENTRY* ppCacheEntry)
{
    LPCLASSCACHEENTRY pCacheEntry;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "_FreeCacheEntry");

    TraceAssert(ppCacheEntry);
    pCacheEntry = *ppCacheEntry;

    if ( pCacheEntry )
    {
        Trace(TEXT("About to wait for multiple object for cache entry: %s"), W2T(pCacheEntry->pObjectClass));

        WaitForSingleObject(pCacheEntry->hLock, INFINITE);

        LocalFreeStringW(&pCacheEntry->pKey);

        LocalFreeStringW(&pCacheEntry->pObjectClass);
        LocalFreeStringW(&pCacheEntry->pServer);
        LocalFreeStringW(&pCacheEntry->pFriendlyClassName);

        FreePropertyPageList(&pCacheEntry->hdsaPropertyPages);
        FreeMenuHandlerList(&pCacheEntry->hdsaMenuHandlers);
        FreeIconList(pCacheEntry);
        FreeAttributeNames(&pCacheEntry->hdpaAttributeNames);

        if ( pCacheEntry->hdsaWizardExtn )
            DSA_Destroy(pCacheEntry->hdsaWizardExtn);

        CloseHandle(pCacheEntry->hLock);

        LocalFree((HLOCAL)pCacheEntry);
        *ppCacheEntry = NULL;
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ ClassCache_Init
/ ---------------
/   Initialize the cache objects we are going to use, mostly the syncronization
/   things that we need.
/
/ In:
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID ClassCache_Init(VOID)
{
    TraceEnter(TRACE_CACHE, "ClassCache_Init");

    g_hCacheLock = CreateMutex(NULL, FALSE, NULL);
    g_hPropCacheLock = CreateMutex(NULL, FALSE, NULL);

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ ClassCache_GetClassInfo
/ -----------------------
/   Query cache code which selectivitly caches information based
/   on the given object and the flags.
/
/ In:
/   pGetInfo -> structure containing parameters for object
/      pPath = ADS path for the object we are tyring to cache on
/       pObjectClass = objectClass to key the cache entry on
/       pAttributePrefix = prefix used when querying for properties (also used in cache key)
/       dwFlags = flags indicating which cache fields are required
/
/   ppCacheEntry -> receieves pointer to the cache entry
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

HRESULT CALLBACK _AddWizardExtnGUID(DWORD dwIndex, BSTR pString, LPVOID pData)
{
    HRESULT hr;
    HDSA hdsa = (HDSA)pData;
    GUID guid;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "_AddWizardExtnGUID");
    Trace(TEXT("dwIndex %08x, pString: %s"), dwIndex, W2T(pString));

    if ( GetGUIDFromStringW(pString, &guid) )
    {
        if ( -1 == DSA_AppendItem(hdsa, &guid) )
            ExitGracefully(hr, E_FAIL, "Failed to add wizard GUID");
    }
   
    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}

INT _CompareCacheEntryCB(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    INT iResult = -1;
    LPCLASSCACHEENTRY pEntry1 = (LPCLASSCACHEENTRY)p1;
    LPCLASSCACHEENTRY pEntry2 = (LPCLASSCACHEENTRY)p2;

    if ( pEntry1 && pEntry2 )
        iResult = StrCmpIW(pEntry1->pKey, pEntry2->pKey);
    
    return iResult;
}

HRESULT ClassCache_GetClassInfo(LPCLASSCACHEGETINFO pInfo, LPCLASSCACHEENTRY* ppCacheEntry)
{
    HRESULT hr;
    LPCLASSCACHEENTRY pCacheEntry = NULL;
    WCHAR szClassKey[MAX_PATH*2];
    INT index;
    IADs* pDisplaySpecifier = NULL;
    IADs* pDsObject = NULL;
    IADsClass* pDsClass = NULL;
    BSTR bstrSchemaObject = NULL;
    HICON hSmallIcon = NULL;
    HICON hLargeIcon = NULL;
    VARIANT variant;
    VARIANT_BOOL vbIsContainer;
    DWORD dwFlags;
    DWORD dwWaitRes;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "ClassCache_GetClassInfo");

    if ( !pInfo || !pInfo->pObjectClass || !ppCacheEntry )
        ExitGracefully(hr, E_FAIL, "Bad parameters for ClassCache_GetClassInfo");

    dwFlags = pInfo->dwFlags;

    // Build the key string, this is "className[:attributePrefix]" that way the shell and the 
    // admin tools can share the same cache structure
    
    VariantInit(&variant);

    StrCpyW(szClassKey, pInfo->pObjectClass);
    
    if ( pInfo->pAttributePrefix )
    {
        StrCatW(szClassKey, L":");
        StrCatW(szClassKey, pInfo->pAttributePrefix);

        if ( dwFlags & ALL_PREFIXED_ATTRIBUTES )
            dwFlags |= ALL_PREFIXED_ATTRIBUTES;
    }
    else
    {
        if ( dwFlags & ALL_NONPREFIXED_ATTRIBUTES )
            dwFlags |= ALL_NONPREFIXED_ATTRIBUTES;
    }

    // add the server name to the class key

    if ( pInfo->pServer ) 
    {
        StrCatW(szClassKey, L":");
        StrCatW(szClassKey, pInfo->pServer);
    }

    Trace(TEXT("Cache key is: %s"), W2T(szClassKey));

    // do we have a cache? if so then look in there to see if we have
    // already cached information about this class

    Trace(TEXT("About to wait for global cache lock when getting cache entry: %s"), W2T(pInfo->pObjectClass));

    dwWaitRes = WaitForSingleObject(g_hCacheLock, INFINITE);
    if ( WAIT_OBJECT_0 != dwWaitRes )
    {
        Trace(TEXT("dwWait res (%x) != WAIT_OBJECT_0"), dwWaitRes);
        ExitGracefully(hr, E_FAIL, "Failed when waiting for cache lock");
    }

    TraceMsg("Global cache lock aquired, so can now modify cache content");

    if ( g_hdpaClassCache )
    {
        // sort it if its not already sorted, then do a sorted search for the
        // best performance so we can pick up the information.

        if ( !g_fClassCacheSorted )
        {
            TraceMsg("!!! Cache not sorted, just about to call DPA_Sort !!!");
            DPA_Sort(g_hdpaClassCache, _CompareCacheEntryCB, NULL);
            g_fClassCacheSorted = TRUE;
        }

        CLASSCACHEENTRY cce;
        cce.pKey = szClassKey;

        Trace(TEXT("Searching the cache for entry %s"), W2T(szClassKey));
        index = DPA_Search(g_hdpaClassCache, &cce, 0, _CompareCacheEntryCB, NULL, DPAS_SORTED);

        if ( index >= 0 )
        {
            Trace(TEXT("Cache hit at location %d"), index);
            pCacheEntry = (LPCLASSCACHEENTRY)DPA_FastGetPtr(g_hdpaClassCache, index);

            Trace(TEXT("About to wait for multiple object for cache entry: %s"), W2T(pCacheEntry->pObjectClass));

            dwWaitRes = WaitForSingleObject(pCacheEntry->hLock, INFINITE);
            if ( dwWaitRes != WAIT_OBJECT_0 )
            {
                UNLOCK_CLASS_CACHE;
                ExitGracefully(hr, E_FAIL, "Failed to lock cache record");
            }
        }
    }
    else
    {
        g_hdpaClassCache = DPA_Create(4);         // create the new cache

        if ( !g_hdpaClassCache )
        {
            UNLOCK_CLASS_CACHE;
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to cache object info");
        }
    }

    // pCacheEntry == NULL if we haven't hit anything yet, therefore lets 
    // create a new entry if that happens, or fall through!

    if ( !pCacheEntry )
    {
        // allocate a new entry, initialize it and put it into the DSA, having done
        // this we can search it, fill in the gaps etc.

        pCacheEntry = (LPCLASSCACHEENTRY)LocalAlloc(LPTR, SIZEOF(CLASSCACHEENTRY));

        if ( !pCacheEntry )
        {
            UNLOCK_CLASS_CACHE;
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate new cache structure");
        }

        pCacheEntry->hLock = CreateMutex(NULL, TRUE, NULL);
        // pCacheEntry->pKey = NULL;
        // pCacheEntry->dwFlags = 0x0;
        // pCacheEntry->dwCached = 0x0;
        // pCacheEntry->fHasWizardDailogCLSID = FALSE;
        // pCacheEntry->fHasWizardPrimaryPageCLSID = FALSE;
        // pCacheEntry->pObjectClass = NULL;
        // pCacheEntry->pServer = NULL;
        // pCacheEntry->pFriendlyClassName = NULL;
        // pCacheEntry->hdsaPropertyPages = NULL;
        // pCacheEntry->hdsaMenuHandlers = NULL;
        // ZeroMemory(pCacheEntry->pIconName, SIZEOF(pCacheEntry->pIconName));
        // ZeroMemory(pCacheEntry->iImage, SIZEOF(pCacheEntry->iImage));
        // pCacheEntry->fIsContainer = FALSE;
        // pCacheEntry->hdpaAttributeNames = NULL;
        // pCacheEntry->clsidWizardDialog = { 0 };
        // pCacheEntry->clsidWizardPrimary = { 0 };
        // pCacheEntry->hdsaWizardExtn = NULL;

        hr = LocalAllocStringW(&pCacheEntry->pKey, szClassKey);

        if ( SUCCEEDED(hr) && !pCacheEntry->hLock )
        {
            TraceMsg("Failed to create hLock");
            hr = E_FAIL;
        }

        if ( SUCCEEDED(hr) )
            hr = LocalAllocStringW(&pCacheEntry->pObjectClass, pInfo->pObjectClass);

        if ( SUCCEEDED(hr) && pInfo->pServer )
            hr = LocalAllocStringW(&pCacheEntry->pServer, pInfo->pServer);

        if ( FAILED(hr) || (-1 == DPA_AppendPtr(g_hdpaClassCache, pCacheEntry)) )
        {
            UNLOCK_CLASS_CACHE;
            UNLOCK_CACHE_ENTRY(pCacheEntry);
            _FreeCacheEntry(&pCacheEntry);
            ExitGracefully(hr, E_UNEXPECTED, "Failed to add cache entry to DPA");
        }

        g_fClassCacheSorted = FALSE;
    }

    UNLOCK_CLASS_CACHE;

    // ensure we have a display specifier if we need one, that boils down to be
    // dwFlags expresses fields we are interested in, so do we have those in
    // the cache record, if not then lets check to see if those bits match
    // ones which come from the specifier, if so then we better grab one.

    if ( dwFlags & ALL_DISPLAY_SPEC_VALUES ) 
    {
        if ( (pCacheEntry->dwFlags & dwFlags) != dwFlags )
        {
            Trace(TEXT("Binding to the display specifier %08x,%08x"), pCacheEntry->dwFlags & dwFlags, dwFlags);

            if ( FAILED(GetDisplaySpecifier(pInfo, IID_IADs, (LPVOID*)&pDisplaySpecifier)) )
            {
                TraceMsg("Failed to bind to display specifier, pDisplaySpecifier == NULL");
                TraceAssert(pDisplaySpecifier == NULL);

                // ensure that we don't try and cache display specifier information and
                // we mark the cache record as dirty.
                
                dwFlags &= ~(ALL_DISPLAY_SPEC_VALUES & ~CLASSCACHE_FRIENDLYNAME);
            }
        }
    }

    // container flag for the objects

    if ( dwFlags & CLASSCACHE_CONTAINER )
    {
        if ( !(pCacheEntry->dwFlags & CLASSCACHE_CONTAINER) ) 
        {
            if ( pInfo->pPath )
            {
                TraceMsg("!!! Binding to the object to get container flags !!!");

                if ( SUCCEEDED(ADsOpenObject(pInfo->pPath, 
                                             pInfo->pUserName, pInfo->pPassword, 
                                             (pInfo->dwFlags & CLASSCACHE_SIMPLEAUTHENTICATE) ? 0:ADS_SECURE_AUTHENTICATION, 
                                             IID_IADs, (LPVOID*)&pDsObject)) )
                {
                    // Try to deterimine if the object is a container by binding to the
                    // schema object and getting its container property.

                    hr = pDsObject->get_Schema(&bstrSchemaObject);
                    FailGracefully(hr, "Failed to get the objects schema");

                    Trace(TEXT("Path to schema object is %s"), W2T(bstrSchemaObject));

                    if ( SUCCEEDED(ADsOpenObject(bstrSchemaObject, 
                                                 pInfo->pUserName, pInfo->pPassword, 
                                                 (pInfo->dwFlags & CLASSCACHE_SIMPLEAUTHENTICATE) ? 0:ADS_SECURE_AUTHENTICATION, 
                                                 IID_IADsClass, (LPVOID*)&pDsClass)) )
                    {
                        if ( SUCCEEDED(pDsClass->get_Container(&vbIsContainer)) )
                        {
                            TraceMsg("Cached container flag");
                            pCacheEntry->fIsContainer = (vbIsContainer == -1);
                            pCacheEntry->dwCached |= CLASSCACHE_CONTAINER;
                        }
                    }
                }
            }
            else
            {
                TraceMsg("**** No ADsPath, cannot get container flag from schema ****");
            }
        }
    }

    // all the following attributes require a pDisplaySpecifier

    if ( pDisplaySpecifier )
    {
        // property pages?

        if ( dwFlags & CLASSCACHE_PROPPAGES )   
        {
            if ( !(pCacheEntry->dwFlags & CLASSCACHE_PROPPAGES) )
            {
                TraceMsg("Caching property page list");

                if ( SUCCEEDED(GetPropertyPageList(pCacheEntry, pInfo->pAttributePrefix, pDisplaySpecifier)) )
                {
                    TraceMsg("Fetching property page list");
                    pCacheEntry->dwCached |= CLASSCACHE_PROPPAGES;  
                }
            }
        }

        // context menu handlers?

        if ( dwFlags & CLASSCACHE_CONTEXTMENUS )   
        {
            if ( !(pCacheEntry->dwFlags & CLASSCACHE_CONTEXTMENUS) )
            {
                TraceMsg("Caching menu handler list");

                if ( SUCCEEDED(GetMenuHandlerList(pCacheEntry, pInfo->pAttributePrefix, pDisplaySpecifier)) )
                {
                    TraceMsg("Fetched context menu list");
                    pCacheEntry->dwCached |= CLASSCACHE_CONTEXTMENUS;
                }
            }
        }

        // icon location?

        if ( dwFlags & CLASSCACHE_ICONS )   
        {
            if ( !(pCacheEntry->dwFlags & CLASSCACHE_ICONS) )
            {
                TraceMsg("Caching icon list");

                if ( SUCCEEDED(GetIconList(pCacheEntry, pDisplaySpecifier)) )
                {
                    TraceMsg("Fetched icon list");
                    pCacheEntry->dwCached |= CLASSCACHE_ICONS;
                }
            }
        }

        // attribute name caching?

        if ( dwFlags & CLASSCACHE_ATTRIBUTENAMES )
        {
            if ( !(pCacheEntry->dwFlags & CLASSCACHE_ATTRIBUTENAMES) )
            {
                TraceMsg("Caching attribute list");

                if ( SUCCEEDED(GetAttributeNames(pCacheEntry, pInfo, pDisplaySpecifier)) )
                {
                    TraceMsg("Fetched attribute names");
                    pCacheEntry->dwCached |= CLASSCACHE_ATTRIBUTENAMES;
                }
            }
        }

        // get the treat as leaf

        if ( dwFlags & CLASSCACHE_TREATASLEAF )
        {
            if ( !(pCacheEntry->dwFlags & CLASSCACHE_TREATASLEAF) )
            {
                TraceMsg("Caching the treat as leaf flag");

                // pick up the "treatAsLeaf" attribute from the display specifier, if
                // this is undefined then use the normal container flag from the
                // schema.

                VariantClear(&variant);

                if ( SUCCEEDED(pDisplaySpecifier->Get(TREAT_AS_LEAF, &variant)) && (V_VT(&variant) == VT_BOOL) )
                {
                    TraceMsg("Caching fTreatAsLeaf");
                    pCacheEntry->fTreatAsLeaf = V_BOOL(&variant) == 1;
                    pCacheEntry->dwCached |= CLASSCACHE_TREATASLEAF;
                }
            }
        }

        // get the CLSID that implements the creation dialog
      
        if ( dwFlags & CLASSCACHE_WIZARDDIALOG )
        {
            if ( !(pCacheEntry->dwFlags & CLASSCACHE_WIZARDDIALOG) )
            {
                TraceMsg("Caching the creation wizard dialog CLSID");

                VariantClear(&variant);

                if ( SUCCEEDED(pDisplaySpecifier->Get(CREATION_DIALOG, &variant)) )
                {
                    if ( V_VT(&variant) == VT_BSTR )
                    {
                        if ( GetGUIDFromStringW(V_BSTR(&variant), &pCacheEntry->clsidWizardDialog) )
                        {
                            TraceGUID("CLSID of wizard dialog: ", pCacheEntry->clsidWizardDialog);
                            pCacheEntry->dwCached |= CLASSCACHE_WIZARDDIALOG;
                        }
                        else
                        {
                            Trace(TEXT("GUID string failed to parse: %s"), W2T(V_BSTR(&variant)));
                        }
                    }
                }
            }
        }

        // get the CLSID that implements the primary pages of the wizard

        if ( dwFlags & CLASSCACHE_WIZARDPRIMARYPAGE )
        {
            if ( !(pCacheEntry->dwFlags & CLASSCACHE_WIZARDPRIMARYPAGE) )
            {
                TraceMsg("Caching the creation wizard's primary page");

                VariantClear(&variant);

                if ( SUCCEEDED(pDisplaySpecifier->Get(CREATION_WIZARD, &variant)) )
                {
                    if ( V_VT(&variant) == VT_BSTR )
                    {
                        if ( GetGUIDFromStringW(V_BSTR(&variant), &pCacheEntry->clsidWizardPrimaryPage) )
                        {
                            TraceGUID("CLSID of primary pages: ", pCacheEntry->clsidWizardPrimaryPage);
                            pCacheEntry->dwCached |= CLASSCACHE_WIZARDPRIMARYPAGE;
                        }
                        else
                        {
                            Trace(TEXT("GUID string failed to parse: %s"), W2T(V_BSTR(&variant)));
                        }
                    }
                }
            }
        }

        // get the CLSID of the extensions for the wizard

        if ( dwFlags & CLASSCACHE_WIZARDEXTN )
        {
            if ( !(pCacheEntry->dwFlags & CLASSCACHE_WIZARDEXTN) )
            {
                TraceMsg("Caching the list of extension pages for the wizard");

                VariantClear(&variant);

                if ( SUCCEEDED(pDisplaySpecifier->Get(CREATION_WIZARD_EXTN, &variant)) )
                {
                    if ( !pCacheEntry->hdsaWizardExtn )
                    {
                        TraceMsg("Creating DSA to store GUIDs in");
                        pCacheEntry->hdsaWizardExtn = DSA_Create(SIZEOF(GUID), 4);
                        TraceAssert(pCacheEntry->hdsaWizardExtn);
                    }

                    if ( pCacheEntry->hdsaWizardExtn )
                    {
                        TraceMsg("Attempting to cache extention GUIDs into the DPA");
                        GetArrayContents(&variant, _AddWizardExtnGUID, (LPVOID)pCacheEntry->hdsaWizardExtn);
                    }
                }

            }
        }
    }

    // friendly class anme for the object
    
    if ( dwFlags & CLASSCACHE_FRIENDLYNAME )
    {
        if ( !(pCacheEntry->dwFlags & CLASSCACHE_FRIENDLYNAME) )
        {
            TraceMsg("Checking for the friendly class name");            

            VariantClear(&variant);

            // if there is a display specifier and a friendly name then lets
            // pick it up and store it in the cache entry.

            if ( pDisplaySpecifier )
            {
                if ( SUCCEEDED(pDisplaySpecifier->Get(FRIENDLY_NAME, &variant)) )
                {
                    if ( V_VT(&variant) == VT_BSTR )
                    {
                        Trace(TEXT("Friendly name: %s"), W2T(V_BSTR(&variant)));

                        hr = LocalAllocStringW(&pCacheEntry->pFriendlyClassName, V_BSTR(&variant));
                        FailGracefully(hr, "Failed to copy the friendly name");

                        pCacheEntry->dwCached |= CLASSCACHE_FRIENDLYNAME;
                    }
                }
            }

            // the friendly name is a special case, if we haven't been able to get the display
            // specifier or the friendly name from it then populate the cache with the
            // existing class name to avoid hitting the wire repeatedly.

            if ( !(pCacheEntry->dwCached & CLASSCACHE_FRIENDLYNAME) )
            {
                TraceMsg("Defaulting to un-friendly class name");
                hr = LocalAllocStringW(&pCacheEntry->pFriendlyClassName, pCacheEntry->pObjectClass);
                FailGracefully(hr, "Failed to allocate friendly class name");
            }

            pCacheEntry->dwCached |= CLASSCACHE_FRIENDLYNAME;
        }
    }

    hr = S_OK;                                  // success!

exit_gracefully:

    DoRelease(pDisplaySpecifier);
    DoRelease(pDsObject);
    DoRelease(pDsClass);

    VariantClear(&variant);
    SysFreeString(bstrSchemaObject);

    if ( hSmallIcon )
        DestroyIcon(hSmallIcon);
    if ( hLargeIcon )
        DestroyIcon(hLargeIcon);
  
    if ( pCacheEntry )
    {
        // make the attributes as cached now, and if we succeeded then we
        // can pass out the locked cache entry, otherwise we must
        // unlock it - otherwise others will not be ableto updated!

        pCacheEntry->dwFlags |= dwFlags;

        if  ( SUCCEEDED(hr) )
        {
            *ppCacheEntry = pCacheEntry;
        }
        else
        {
            UNLOCK_CACHE_ENTRY(pCacheEntry);
        }
    }

    TraceLeaveResult(hr);
}



/*-----------------------------------------------------------------------------
/ ClassCache_ReleaseClassInfo
/ ---------------------------
/   Each cache entry has a lock, this releases the lock.  If the lock is
/   non-zero then the record cannot be updated, or released.
/
/ In:
/   ppCacheEntry -> cache entry, NULL'd on exit.
/
/ Out:
/   VOID
/----------------------------------------------------------------------------*/
VOID ClassCache_ReleaseClassInfo(LPCLASSCACHEENTRY* ppCacheEntry)
{
    TraceEnter(TRACE_CACHE, "ClassCache_ReleaseClassInfo");

    if ( ppCacheEntry )
    {
        LPCLASSCACHEENTRY pCacheEntry = *ppCacheEntry;
        if ( pCacheEntry )
        {
            UNLOCK_CACHE_ENTRY(pCacheEntry);
            *ppCacheEntry = NULL;
        }
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ ClassCache_Discard
/ ------------------
/   Discard the cached information we have for the DS classes we have
/   seen (including the cache DPA & the image lists)
/
/ In:
/   -
/ Out:
/   VOID
/----------------------------------------------------------------------------*/

INT _FreePropCacheEntryCB(LPVOID pVoid, LPVOID pData)
{
    LPPROPCACHEENTRY pCacheEntry = (LPPROPCACHEENTRY)pVoid;

    TraceEnter(TRACE_CACHE, "_FreePropCacheEntryCB");
    _FreePropCacheEntry(&pCacheEntry);
    TraceLeaveValue(TRUE);
}

INT _FreeCacheEntryCB(LPVOID pVoid, LPVOID pData)
{
    LPCLASSCACHEENTRY pCacheEntry = (LPCLASSCACHEENTRY)pVoid;

    TraceEnter(TRACE_CACHE, "_FreeCacheEntryCB");
    _FreeCacheEntry(&pCacheEntry);
    TraceLeaveValue(TRUE);
}

VOID ClassCache_Discard(VOID)
{
    HRESULT hr;
    DWORD dwWaitRes;

    TraceEnter(TRACE_CACHE, "ClassCache_Discard");

    // avoid destroying the cache whilst its being updated, ths is a simple
    // mutex.


    TraceMsg("About to wait for global cache lock");

    dwWaitRes = WaitForSingleObject(g_hCacheLock, INFINITE);
    if ( WAIT_OBJECT_0 != dwWaitRes )
    {
        Trace(TEXT("dwWait res (%x) != WAIT_OBJECT_0"), dwWaitRes);
        ExitGracefully(hr, E_FAIL, "Failed when waiting for cache lock");
    }

    TraceMsg("Global cache lock aquired, so can now modify cache content");

    if ( g_hdpaClassCache )
    {
        DPA_DestroyCallback(g_hdpaClassCache, _FreeCacheEntryCB, NULL);
        g_hdpaClassCache = NULL;
    }

    // the property cache is protected also, so wait until we can get the 
    // lock on it before partying on the structure.

    
    TraceMsg("About to wait for global property cache lock");

    dwWaitRes = WaitForSingleObject(g_hPropCacheLock, INFINITE);
    if ( WAIT_OBJECT_0 != dwWaitRes )
    {
        Trace(TEXT("dwWait res (%x) != WAIT_OBJECT_0"), dwWaitRes);
        ExitGracefully(hr, E_FAIL, "Failed when waiting for property cache lock");
    }

    TraceMsg("Global property cache lock aquired, so can now modify cache content");
    
    if ( g_hdpaPropCache )
    {
        DPA_DestroyCallback(g_hdpaPropCache, _FreePropCacheEntryCB, NULL);
        g_hdpaPropCache = NULL;
    }
    
exit_gracefully:

    CloseHandle(g_hCacheLock);
    CloseHandle(g_hPropCacheLock);

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ Property page list
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ GetPropertyPageList
/ -------------------
/   Build the list of property pages that we are going to be displaying
/   the code builds the list from the display specifier lists.
/
/ In:
/   pCacheEntry -> Cache entry to update
/   pAttributePrefix -> suitable prefix for getting Admin/Shell pages
/   pDataObject -> IDataObject for getting cached information from
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

HRESULT CALLBACK _AddPropertyPageItemCB(DWORD dwIndex, BSTR pString, LPVOID pData)
{
    HRESULT hr;
    DSPROPERTYPAGE item;
    HDSA hdsa = (HDSA)pData;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "_AddPropertyPageItemCB");
    Trace(TEXT("dwIndex %08x, pString: %s"), dwIndex, W2T(pString));

    hr = LocalAllocStringW(&item.pPageReference, pString);
    FailGracefully(hr, "Failed to clone string");

    if ( -1 == DSA_AppendItem(hdsa, &item) )
        ExitGracefully(hr, E_FAIL, "Failed to property page reference to DSA");

    hr = S_OK;

exit_gracefully:

    if ( FAILED(hr) )
        LocalFreeStringW(&item.pPageReference);

    TraceLeaveResult(hr);
}

HRESULT GetPropertyPageList(LPCLASSCACHEENTRY pCacheEntry, LPWSTR pAttributePrefix, IADs* pDisplaySpecifier)
{
    HRESULT hr;
    VARIANT variant;
    WCHAR szProperty[MAX_PATH] = { TEXT('\0') };
    USES_CONVERSION;
    INT i;

    TraceEnter(TRACE_CACHE, "GetPropertyPageList");

    VariantInit(&variant);

    pCacheEntry->hdsaPropertyPages = DSA_Create(SIZEOF(DSPROPERTYPAGE), 4);
    TraceAssert(pCacheEntry->hdsaPropertyPages);

    if ( !pCacheEntry->hdsaPropertyPages )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate page DPA");

    // build the property we are going to key off and then lets start
    // to walk the list of diplay specifiers checking each one for 
    // a list of property pages.
    
    if ( pAttributePrefix )
        StrCatW(szProperty, pAttributePrefix);
    
    StrCatW(szProperty, PROPERTY_PAGES);

    Trace(TEXT("Enumerating property pages from: %s"), W2T(szProperty));

    if ( SUCCEEDED(pDisplaySpecifier->Get(szProperty, &variant)) )
    {
        hr = GetArrayContents(&variant, _AddPropertyPageItemCB, (LPVOID)pCacheEntry->hdsaPropertyPages);
        FailGracefully(hr, "Failed to add property pages to DSA");
    
        VariantClear(&variant);
    }

    if ( SUCCEEDED(pDisplaySpecifier->Get(PROPERTY_PAGES, &variant)) )
    {
        hr = GetArrayContents(&variant, _AddPropertyPageItemCB, (LPVOID)pCacheEntry->hdsaPropertyPages);
        FailGracefully(hr, "Failed to add property pages to DSA");
    
        VariantClear(&variant);
    }

    hr = S_OK;                      

exit_gracefully:

    if ( FAILED(hr) )
        FreePropertyPageList(&pCacheEntry->hdsaPropertyPages);

    VariantClear(&variant);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ FreePropertyPageList
/ --------------------
/   Free the property page list associated with a particular cache entry
/
/ In:
/   pHDSA = pointer to a HDSA to be free'd, and NULL'd
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

INT _FreePropertyPageItemCB(LPVOID p, LPVOID pData)
{
    LPDSPROPERTYPAGE pItem = (LPDSPROPERTYPAGE)p;
    
    TraceEnter(TRACE_CACHE, "_FreePropertyPageItemCB");
    TraceAssert(pItem);

    LocalFreeStringW(&pItem->pPageReference);

    TraceLeaveValue(1);
}

VOID FreePropertyPageList(HDSA* pHDSA)
{
    TraceEnter(TRACE_CACHE, "FreePropertyPageList");

    if ( *pHDSA )
        DSA_DestroyCallback(*pHDSA, _FreePropertyPageItemCB, 0L);

    *pHDSA = NULL;

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ Menu item lists
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ GetMenuHandlerList
/ ------------------
/   The "contextMenu" property on a DS object contains a list of 
/   the menu handlers that we want to interact with.
/
/ In:
/   pCacheEntry -> Cache entry to update
/   pAttributePrefix -> suitable prefix for getting Admin/Shell pages
/   pDataObject -> IDataObject for getting cached information from
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

HRESULT CALLBACK _AddMenuHandlerCB(DWORD dwIndex, BSTR pString, LPVOID pData)
{
    HRESULT hr;
    DSMENUHANDLER item;
    HDSA hdsa = (HDSA)pData;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "_AddMenuHandlerCB");
    Trace(TEXT("dwIndex %08x, pString: %s"), dwIndex, W2T(pString));

    hr = LocalAllocStringW(&item.pMenuReference, pString);
    FailGracefully(hr, "Failed to clone string");

    if ( -1 == DSA_AppendItem(hdsa, &item) )
        ExitGracefully(hr, E_FAIL, "Failed to add menu reference to DSA");

    hr = S_OK;

exit_gracefully:

    if ( FAILED(hr) )
        LocalFreeStringW(&item.pMenuReference);

    TraceLeaveResult(hr);
}

HRESULT GetMenuHandlerList(LPCLASSCACHEENTRY pCacheEntry, LPWSTR pAttributePrefix, IADs* pDisplaySpecifier)
{
    HRESULT hr;
    WCHAR szProperty[MAX_PATH] = { TEXT('\0') };
    VARIANT variant;
    INT i;

    TraceEnter(TRACE_CACHE, "GetMenuHandlerList");

    VariantInit(&variant);

    pCacheEntry->hdsaMenuHandlers = DSA_Create(SIZEOF(DSPROPERTYPAGE), 4);
    TraceAssert(pCacheEntry->hdsaMenuHandlers);

    if ( !pCacheEntry->hdsaMenuHandlers )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate page DPA");
  
    // first try "<attributePrefix>ContextMenu" to pick up the provider specific menus

    if ( pAttributePrefix )
        StrCatW(szProperty, pAttributePrefix);

    StrCatW(szProperty, CONTEXT_MENU);

    if ( SUCCEEDED(pDisplaySpecifier->Get(szProperty, &variant)) )
    {
        hr = GetArrayContents(&variant, _AddMenuHandlerCB, (LPVOID)pCacheEntry->hdsaMenuHandlers);
        FailGracefully(hr, "Failed to add property pages to DSA");

        VariantClear(&variant);
    }

    if ( SUCCEEDED(pDisplaySpecifier->Get(CONTEXT_MENU, &variant)) )
    {
        hr = GetArrayContents(&variant, _AddMenuHandlerCB, (LPVOID)pCacheEntry->hdsaMenuHandlers);
        FailGracefully(hr, "Failed to add property pages to DSA");

        VariantClear(&variant);
    }

    hr = S_OK;              // success

exit_gracefully:

    if ( FAILED(hr) )
        FreeMenuHandlerList(&pCacheEntry->hdsaMenuHandlers);

    VariantClear(&variant);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ FreeMenuHandlerList
/ ------------=------
/   Free the list of menu items that are stored in the cache DSA.
/
/ In:
/   pHDSA = pointer to a HDSA to be free'd, and NULL'd
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

INT _FreeMenuHandlerCB(LPVOID p, LPVOID pData)
{
    LPDSMENUHANDLER pItem = (LPDSMENUHANDLER)p;
    
    TraceEnter(TRACE_CACHE, "_FreeMenuHandlerCB");
    TraceAssert(pItem);

    LocalFreeStringW(&pItem->pMenuReference);

    TraceLeaveValue(1);
}

VOID FreeMenuHandlerList(HDSA* pHDSA)
{
    TraceEnter(TRACE_CACHE, "FreeMenuHandlerList");

    if ( *pHDSA )
        DSA_DestroyCallback(*pHDSA, _FreeMenuHandlerCB, 0L);

    *pHDSA = NULL;

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ Property page list
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ GetIconList
/ -----------
/   Get the icon list from the class specifier.  Bind to the class specifier
/   and then enumerate the icon property.  We store an array which contains
/   the icon locations for multiple states, therefore as we are called to
/   add the entries we clear out that previous index.
/
/ In:
/   pCacheEntry -> Cache entry to update
/   pDataObject -> pData object for extra information
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

HRESULT CALLBACK _AddIconToCacheEntryCB(DWORD dwIndex, BSTR pString, LPVOID pData)
{
    HRESULT hr;
    LPCLASSCACHEENTRY pCacheEntry = (LPCLASSCACHEENTRY)pData;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "_AddIconToCacheEntryCB");
    Trace(TEXT("dwIndex %08x, pString: %s"), dwIndex, W2T(pString));

    if ( dwIndex < ARRAYSIZE(pCacheEntry->pIconName) )
    {
        LocalFreeStringW(&pCacheEntry->pIconName[dwIndex]);
        hr = LocalAllocStringW(&pCacheEntry->pIconName[dwIndex], pString);
        FailGracefully(hr, "Failed to copy icon location");
    }

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}

HRESULT GetIconList(LPCLASSCACHEENTRY pCacheEntry, IADs* pDisplaySpecifier)
{
    HRESULT hr;
    VARIANT variant;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "GetIconList");

    VariantInit(&variant);

    if ( SUCCEEDED(pDisplaySpecifier->Get(ICON_LOCATION, &variant)) )
    {
        hr = GetArrayContents(&variant, _AddIconToCacheEntryCB, (LPVOID)pCacheEntry);
        FailGracefully(hr, "Failed to get the icon list into the cache entry");            
    }

    hr = S_OK;           // success

exit_gracefully:

    if ( FAILED(hr) )
        FreeIconList(pCacheEntry);

    VariantClear(&variant);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ FreeIconList
/ ------------
/   Clear out the icon list, this an array of string pointers allocated by
/   LocalAllocString.
/
/ In:
/   pCacheEntry -> Cache entry to update
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
VOID FreeIconList(LPCLASSCACHEENTRY pCacheEntry)
{
    TraceEnter(TRACE_CACHE, "FreeIconList");

    for ( INT i = 0 ; i < ARRAYSIZE(pCacheEntry->pIconName); i++ )
        LocalFreeStringW(&pCacheEntry->pIconName[i]);

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ Attribute Name helpers
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ GetAttributeNames
/ -----------------
/   Get the attribute names given the cache entry and the variant to store
/   them into.
/
/ In:
/   pCacheEntry -> cache entry to be filled
/   pDataObject -> dataobject used for IDataObject caching
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

VOID _AddAttributeName(HDPA hdpaAttributeNames, LPWSTR pName, LPWSTR pDisplayName, DWORD dwFlags, HDPA hdpaNewAttributes)
{
    HRESULT hr;
    LPATTRIBUTENAME pAttributeName = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "_AddAttributeName");
    Trace(TEXT("pName: %s"), W2T(pName));
    Trace(TEXT("pDisplayName: %s"), W2T(pDisplayName));

    pAttributeName = (LPATTRIBUTENAME)LocalAlloc(LPTR, SIZEOF(ATTRIBUTENAME));
    TraceAssert(pAttributeName);

    if ( !pAttributeName )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate ATTRIBUTENAME");

    //pAttributeName->pName = NULL;
    //pAttributeName->pDisplayName = NULL;
    pAttributeName->dwADsType = ADSTYPE_UNKNOWN;
    pAttributeName->dwFlags = dwFlags;

    hr = LocalAllocStringW(&pAttributeName->pName, pName);
    FailGracefully(hr, "Failed to allocate attribute name")

    hr = LocalAllocStringW(&pAttributeName->pDisplayName, pDisplayName);
    FailGracefully(hr, "Failed to allocate display name");

    if ( -1 == DPA_AppendPtr(hdpaAttributeNames, pAttributeName) )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to add to the DPA");

    // do we need to add the attribute to the new "attribtes list"?

    Trace(TEXT("About to search cache for: %s"), W2T(pName));

    if ( g_hdpaPropCache )
    {
        PROPCACHEENTRY pce = { 0 };
        pce.pName = pName;
        if ( -1 == DPA_Search(g_hdpaPropCache, &pce, 0, _ComparePropCacheEntry, NULL, DPAS_SORTED) )
        {
            hr = StringDPA_AppendStringW(hdpaNewAttributes, pName, NULL);
            FailGracefully(hr, "Failed to add the property to the new attribute list");
        }
    }
    else
    {
        hr = StringDPA_AppendStringW(hdpaNewAttributes, pName, NULL);
        FailGracefully(hr, "Failed to add the property to the new attribute list");
    }

    hr = S_OK;

exit_gracefully:

    if ( FAILED(hr) )
        _FreeAttributeNameCB(pAttributeName, NULL);

    TraceLeave();
}

INT _CompareAttributeNameCB(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    LPATTRIBUTENAME pEntry1 = (LPATTRIBUTENAME)p1;
    LPATTRIBUTENAME pEntry2 = (LPATTRIBUTENAME)p2;
    return StrCmpIW(pEntry1->pName, pEntry2->pName);
}

HRESULT GetAttributeNames(LPCLASSCACHEENTRY pCacheEntry, LPCLASSCACHEGETINFO pccgi, IADs* pDisplaySpecifier)
{
    HRESULT hr;
    LONG l, lower, upper;
    LPVARIANT pArray = NULL;
    HDPA hdpaNewAttributes = NULL;
    VARIANT variant;
    WCHAR szProperty[MAX_PATH], szDisplayName[MAX_PATH], szHide[10];
    DWORD dwFlags;
    IDirectorySchemaMgmt *pdsm = NULL;
    INT i;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "GetAttributeNames");

    // allocate a DPA for storing the new attribute list into

    hdpaNewAttributes = DPA_Create(16);
    TraceAssert(hdpaNewAttributes);

    if ( !hdpaNewAttributes )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate new attribute DPA");

    // get the property the user specified, this should be an array of
    // property values that will be associated with the class

    VariantInit(&variant);

    if ( !pCacheEntry->hdpaAttributeNames )
    {
        pCacheEntry->hdpaAttributeNames = DPA_Create(16);
        TraceAssert(pCacheEntry->hdpaAttributeNames);

        if ( !pCacheEntry->hdpaAttributeNames )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate attribute name DSA");
    }

    if ( SUCCEEDED(pDisplaySpecifier->Get(ATTRIBUTE_NAMES, &variant)) )
    {
        if ( V_VT(&variant) == VT_BSTR )
        {
            // Parse the name from the string format <property>[,<display name>]
            // and add it to the property DPA we have been given.

            GetStringElementW(V_BSTR(&variant), 0, szProperty, ARRAYSIZE(szProperty));
            GetStringElementW(V_BSTR(&variant), 1, szDisplayName, ARRAYSIZE(szDisplayName));

            dwFlags = 0x0;

            if ( SUCCEEDED(GetStringElementW(V_BSTR(&variant), 2, szHide, ARRAYSIZE(szHide))) )
            {
                Trace(TEXT("Parsing hide flag: %s"), W2T(szHide));
                dwFlags = StringToDWORD(szHide);
            }

            _AddAttributeName(pCacheEntry->hdpaAttributeNames, szProperty, szDisplayName, dwFlags, hdpaNewAttributes);
        }
        else
        {
            if ( V_VT(&variant) != (VT_VARIANT|VT_ARRAY) )
                ExitGracefully(hr, E_FAIL, "Exported VARIANT array as result from property query");

            hr = SafeArrayGetLBound(V_ARRAY(&variant), 1, (LONG*)&lower);
            FailGracefully(hr, "Failed to get lower bounds of array");

            hr = SafeArrayGetUBound(V_ARRAY(&variant), 1, (LONG*)&upper);
            FailGracefully(hr, "Failed to get upper bounds of array");

            hr = SafeArrayAccessData(V_ARRAY(&variant), (LPVOID*)&pArray);
            FailGracefully(hr, "Failed to get 'safe' accessor to array");            
           
            for ( l = lower; l <= upper ; l++ )
            {
                LPVARIANT pVariant = &pArray[l];
                TraceAssert(pVariant);

                if ( V_VT(pVariant) == VT_BSTR  )
                {
                    // Parse the name from the string format <property>[,<display name>]
                    // and add it to the property DPA we have been given.

                    GetStringElementW(V_BSTR(pVariant), 0, szProperty, ARRAYSIZE(szProperty));
                    GetStringElementW(V_BSTR(pVariant), 1, szDisplayName, ARRAYSIZE(szDisplayName));

                    if ( SUCCEEDED(GetStringElementW(V_BSTR(pVariant), 2, szHide, ARRAYSIZE(szHide))) )
                    {
                        Trace(TEXT("Parsing hide flag: %s"), W2T(szHide));
                        dwFlags = StringToDWORD(szHide);
                    }

                    _AddAttributeName(pCacheEntry->hdpaAttributeNames, szProperty, szDisplayName, dwFlags, hdpaNewAttributes);
                }
            }

            DPA_Sort(pCacheEntry->hdpaAttributeNames, _CompareAttributeNameCB, NULL);
        }
    }

    // walk the cache adding the entries

    hr = _GetDsSchemaMgmt(pccgi, &pdsm);
    FailGracefully(hr, "Failed to get schema management object");

    for ( i = 0 ; i < DPA_GetPtrCount(hdpaNewAttributes) ; i++ )
    {
        LPCWSTR pAttributeName = StringDPA_GetStringW(hdpaNewAttributes, i);
        TraceAssert(pAttributeName);

        hr = _AddPropToPropCache(pccgi, pdsm, pAttributeName, NULL);
        FailGracefully(hr, "Failed to add property to cache");
    }    
    
    hr = S_OK;
    
exit_gracefully:

    if ( FAILED(hr) )
        FreeAttributeNames(&pCacheEntry->hdpaAttributeNames);

    VariantClear(&variant);
    DoRelease(pdsm);

    if ( g_hdpaPropCache )
    {
        TraceMsg("Sorting the property cache");
        DPA_Sort(g_hdpaPropCache, _ComparePropCacheEntry, NULL);
    }

    StringDPA_Destroy(&hdpaNewAttributes);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ FreeAttributeNames
/ --------------------
/   Free the DSA containiing the attribute names and their display
/   name.
/
/ In:
/   pHDSA = pointer to a HDSA to be free'd, and NULL'd
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

INT _FreeAttributeNameCB(LPVOID p, LPVOID pData)
{
    LPATTRIBUTENAME pItem = (LPATTRIBUTENAME)p;
    
    TraceEnter(TRACE_CACHE, "_FreeAttributeNameCB");
    TraceAssert(pItem && pItem->pName && pItem->pDisplayName);

    LocalFreeStringW(&pItem->pName);
    LocalFreeStringW(&pItem->pDisplayName);
    LocalFree(pItem);

    TraceLeaveValue(1);
}

VOID FreeAttributeNames(HDPA* pHDPA)
{
    TraceEnter(TRACE_CACHE, "FreeAttributeNames");

    if ( *pHDPA )
    {
        DPA_DestroyCallback(*pHDPA, _FreeAttributeNameCB, 0L);
        *pHDPA = NULL;
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ Property cache helpers and fillers
/----------------------------------------------------------------------------*/

//
// cache house keeping functions (delete and compare)
//

INT _ComparePropCacheEntry(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    LPPROPCACHEENTRY pEntry1 = (LPPROPCACHEENTRY)p1;
    LPPROPCACHEENTRY pEntry2 = (LPPROPCACHEENTRY)p2;
    return StrCmpIW(pEntry1->pName, pEntry2->pName);
}

VOID _FreePropCacheEntry(LPPROPCACHEENTRY *ppCacheEntry)
{
    if ( *ppCacheEntry )
    {
        LPPROPCACHEENTRY pCacheEntry = *ppCacheEntry;
        LocalFreeStringW(&pCacheEntry->pName);
        LocalFree(pCacheEntry);
        *ppCacheEntry = NULL;    
    }
}

//
// get the IDirectorySchemaManagement object for the server
//

HRESULT _GetDsSchemaMgmt(LPCLASSCACHEGETINFO pccgi, IDirectorySchemaMgmt **ppdsm)
{
    HRESULT hres;
    IADs *pRootDSE = NULL;
    VARIANT variant;
    LPWSTR pszPath = NULL;
    LPWSTR pszServer = pccgi->pServer;
    LPWSTR pszMachineServer = NULL;
    INT cchPath;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "_GetDsSchemaMgmt");

    *ppdsm = NULL;
    VariantInit(&variant);

    hres = GetCacheInfoRootDSE(pccgi, &pRootDSE);
#if !DOWNLEVEL_SHELL
    if ( (hres == HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN)) && !pccgi->pServer )
    {
        TraceMsg("Failed to get the RootDSE from the server - not found");

        DSROLE_PRIMARY_DOMAIN_INFO_BASIC *pInfo;
        if ( DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (BYTE**)&pInfo) == WN_SUCCESS )
        {
            if ( pInfo->DomainNameDns )
            {
                Trace(TEXT("Machine domain is: %s"), W2T(pInfo->DomainNameDns));

                CLASSCACHEGETINFO ccgi = *pccgi;
                ccgi.pServer = pInfo->DomainNameDns;

                hres = GetCacheInfoRootDSE(&ccgi, &pRootDSE);
                if ( SUCCEEDED(hres) )
                {
                    hres = LocalAllocStringW(&pszMachineServer, pInfo->DomainNameDns);
                    pszServer = pszMachineServer;
                }
            }

            DsRoleFreeMemory(pInfo);
        }
    }
#endif
    FailGracefully(hres, "Failed to get the RootDSE");

    hres = pRootDSE->Get(L"defaultNamingContext", &variant);
    FailGracefully(hres, "Failed to get default naming context for this object");

    if ( V_VT(&variant) != VT_BSTR )
        ExitGracefully(hres, E_FAIL, "defaultNamingContext is not a BSTR");

    cchPath = lstrlenW(L"LDAP://") + lstrlenW(V_BSTR(&variant));
    if ( pszServer )    
        cchPath += 1 + lstrlenW(pszServer);

    hres = LocalAllocStringLenW(&pszPath, cchPath);
    FailGracefully(hres, "Failed to allocate the buffer for the name");

    StrCpyW(pszPath, L"LDAP://");

    if ( pszServer )
    {
        StrCatW(pszPath, pszServer);
        StrCatW(pszPath, L"/");
    }

    StrCatW(pszPath, V_BSTR(&variant));
    Trace(TEXT("Default naming context is (with prefix) %s"), W2T(pszPath));

    hres = ADsOpenObject(pszPath, 
                         pccgi->pUserName, pccgi->pPassword,
                         (pccgi->dwFlags & CLASSCACHE_SIMPLEAUTHENTICATE) ? 0:ADS_SECURE_AUTHENTICATION, 
                         IID_IDirectorySchemaMgmt, (void **)ppdsm);

    FailGracefully(hres, "Failed to open the default naming context object");

exit_gracefully:

    LocalFreeStringW(&pszPath);
    LocalFreeStringW(&pszMachineServer);

    VariantClear(&variant);
    DoRelease(pRootDSE);

    TraceLeaveResult(hres);
}              

//
// allocate the cache (if needed) and add a new entry to it, reading the schema to find out the type
// of attribute this is.
//

HRESULT _AddPropToPropCache(LPCLASSCACHEGETINFO pccgi, IDirectorySchemaMgmt *pdsm, LPCWSTR pAttributeName, ADSTYPE *padt)
{
    HRESULT hres;
    PADS_ATTR_DEF pad = NULL;
    WCHAR szAttributeName[MAX_PATH];
    DWORD dwReturned;
    LPPROPCACHEENTRY pCacheEntry = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "_AddPropToPropCache");

    // compute the property name

    StrCpyW(szAttributeName, pAttributeName);
    
    if ( pccgi->pServer )
    {
        StrCatW(szAttributeName, L":");
        StrCatW(szAttributeName, pccgi->pServer);
    }

    // check to see if we have a cache already

    if ( !g_hdpaPropCache )
    {
        g_hdpaPropCache = DPA_Create(16);
        TraceAssert(g_hdpaPropCache);

        if ( !g_hdpaPropCache )
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate property cache");
    }

    // allocate a new cache entry, fill it and add it to the DPA.

    pCacheEntry = (LPPROPCACHEENTRY)LocalAlloc(LPTR, SIZEOF(PROPCACHEENTRY));
    TraceAssert(pCacheEntry);

    if ( !pCacheEntry )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate new property cache entry");

    // pCacheEntry->pName = NULL;
    pCacheEntry->dwADsType = ADSTYPE_UNKNOWN;

    // fill the record from the schema information we have

    hres = LocalAllocStringW(&pCacheEntry->pName, szAttributeName);
    FailGracefully(hres, "Failed to add name to entry");

    hres = pdsm->EnumAttributes((LPWSTR *)&pAttributeName, 1, &pad, &dwReturned);
    FailGracefully(hres, "Failed to read the property information");

    if ( dwReturned )
    {
        pCacheEntry->dwADsType = pad->dwADsType;
    }
    else
    {
        TraceMsg("*** Failed to read property type from schema, defaulting to ADSTYPE_UNKNOWN ***");
    }

    Trace(TEXT("Attribute: %s is %08x"), W2CT(pCacheEntry->pName), pCacheEntry->dwADsType);

    if ( -1 == DPA_AppendPtr(g_hdpaPropCache, pCacheEntry) )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to add the entry to the property cache DPA");

    hres = S_OK;

exit_gracefully:

    if ( FAILED(hres) )
        _FreePropCacheEntry(&pCacheEntry);

    if ( pad )
        FreeADsMem(pad);

    if ( SUCCEEDED(hres) && padt )
        *padt = pCacheEntry->dwADsType;

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ ClassCache_GetADsTypeFromAttribute
/ ----------------------------------
/   Given a property name return the ADsType for it, this is based off the
/   global property cache, not the display specifier information we have.
/
/ In:
/   pccgi -> CLASSCACHEGETINFO structure (credentials)
/   pAttributeName -> attribute name to look up
/
/ Out:
/   ADSTYPE 
/----------------------------------------------------------------------------*/
ADSTYPE ClassCache_GetADsTypeFromAttribute(LPCLASSCACHEGETINFO pccgi, LPCWSTR pAttributeName)
{
    ADSTYPE dwResult = ADSTYPE_UNKNOWN;
    WCHAR szAttributeName[MAX_PATH];
    INT iFound = -1;
    IDirectorySchemaMgmt *pdsm = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "ClassCache_GetADsTypeFromAttribute");
    Trace(TEXT("Looking up property in cache: %s"), W2CT(pAttributeName));

    // get the lock on the cache, then search it for the property we have been given

    TraceMsg("Waiting to get cache lock for property cache");

    if ( WAIT_OBJECT_0 == WaitForSingleObject(g_hPropCacheLock, INFINITE) )
    {
        // formulate a key into the property cache

        Trace(TEXT("Lock aquired, building key for: %s"), W2CT(pAttributeName));           

        StrCpyW(szAttributeName, pAttributeName);
    
        if ( pccgi->pServer )
        {
            StrCatW(szAttributeName, L":");
            StrCatW(szAttributeName, pccgi->pServer);
        }

        Trace(TEXT("Key for attribute in cache is: %s"), W2T(szAttributeName));

        // and search for it...

        if ( g_hdpaPropCache )
        {
            PROPCACHEENTRY pce = { 0 };

            pce.pName = (LPWSTR)szAttributeName;
            iFound =  DPA_Search(g_hdpaPropCache, &pce, 0, _ComparePropCacheEntry, NULL, DPAS_SORTED);
            Trace(TEXT("Entry found in cache at %d"), iFound);
        }

        // iFound != -1 if we found something, otherwise we need to allocate a new entry

        if ( iFound != -1 )
        {   
            LPPROPCACHEENTRY pCacheEntry = (LPPROPCACHEENTRY)DPA_GetPtr(g_hdpaPropCache, iFound);
            TraceAssert(pCacheEntry);

            dwResult = pCacheEntry->dwADsType;
            Trace(TEXT("Property found in cache, result %d"), dwResult);
        }
        else if ( SUCCEEDED(_GetDsSchemaMgmt(pccgi, &pdsm)) )
        {
            if ( SUCCEEDED(_AddPropToPropCache(pccgi, pdsm, pAttributeName, &dwResult)) )
            {
                TraceMsg("Added the property to the cache, therefore sorting");
                DPA_Sort(g_hdpaPropCache, _ComparePropCacheEntry, NULL);
            }
        }
    
        UNLOCK_PROP_CACHE;
    }

    DoRelease(pdsm);

    TraceLeaveValue(dwResult);
}
