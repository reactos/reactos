/*----------------------------------------------------------------------------
/ Title;
/   idlist.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   - All the smarts to handle IDLISTs for the DS.
/   - DS IDLISTs contain UNICODE only strings
/----------------------------------------------------------------------------*/
#include "pch.h"
#include "stddef.h"
#pragma hdrstop



/*-----------------------------------------------------------------------------
/ IDLISTs are opaque structures, they can be stored at any aligment, ours
/ contain the following data.  However this should not be treated as the
/ public version.  The pack and unpack functions below are responsible for
/ generating this structure, use those.
/----------------------------------------------------------------------------*/
#pragma pack(1)

typedef struct
{
#if DELEGATE
    WORD    cbSize;                     // size of entire item ID
    WORD    wOuter;                     // private data owned by the outer folder
    WORD    cbInner;                    // size of delegate's data
#else
    WORD    cbSize;                     // size of the IDLIST
#endif
    WORD    cbToStrings;                // offset to start of string data
    DWORD   dwMagic;                    // magic guard word
    DWORD   dwFlags;                    // flags word (defines what is present)
    DWORD   dwProviderFlags;            // provider flags (sucked from the registry)
    DWORD   dwReserved;        
    WCHAR   szStringData[1];            // all strings stored UNICODE
} OURIDLIST;

typedef UNALIGNED OURIDLIST*  LPOURIDLIST;

#pragma pack()


// 
// A class which implements a linear probing hash, this is used specificly
// to cache ADSI paths and Object Class pairs for simple IDLISTs to avoid
// us having to bind into the DS to obtain that information.
//
// The code is based on an algorithm in "Algorithms in C/Sedgewick pp 237".
//
// Basiclly compute the hash, check the index, if we don't have a match
// then advance until we do or we hit an empty slot.  Then write the data
// there.
//

#define CACHE_SIZE 101                  // nb: 101 is a prime 

typedef struct
{
    LPWSTR pPath;                       // real key we are using
    LPWSTR pObjectClass;                // value we are assocating.

} CACHEENTRY, * LPCACHEENTRY;

class CObjectCache
{
    private:
        CACHEENTRY m_cache[CACHE_SIZE];

        INT _ComputeKey(LPCWSTR pPath);
   
    public:
        CObjectCache();
        ~CObjectCache();
        LPCWSTR GetObjectClass(LPCWSTR pPath);
};

CObjectCache g_ObjectCache;


/*-----------------------------------------------------------------------------
/ CObjectCache implementation
/----------------------------------------------------------------------------*/

CObjectCache::CObjectCache()
{
    TraceEnter(TRACE_IDLIST, "CObjectCache::CObjectCache");
    
    ZeroMemory(m_cache, SIZEOF(m_cache));         // nuke out the cache

    TraceLeave();
}

CObjectCache::~CObjectCache()
{
    INT i;

    TraceEnter(TRACE_IDLIST, "CObjectCache::~CObjectCache");

    for ( i = 0 ; i < CACHE_SIZE ; i++ )
    {
        LocalFreeStringW(&m_cache[i].pPath);
        LocalFreeStringW(&m_cache[i].pObjectClass);
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CObjectCache::_ComputeKey
/ -------------------------
/   Compute the key for a given ADSI path.
/
/ In:
/   pPath -> ADsPath for the object
/
/ Out:
/   INT - index into the cache
/----------------------------------------------------------------------------*/

INT CObjectCache::_ComputeKey(LPCWSTR pPath)
{
    INT result, i;
    USES_CONVERSION;

    TraceEnter(TRACE_IDLIST, "CObjectCache::_ComputeKey");
    Trace(TEXT("pPath %s"), W2CT(pPath));

    for ( result = 0, i = 0 ; i < lstrlenW(pPath) ; i++ )
        result = (64 * result + pPath[i]) % CACHE_SIZE;
    
    Trace(TEXT("result %d"), result);
    TraceLeaveValue(result);
}


/*-----------------------------------------------------------------------------
/ CObjectCache::GetObjectClass
/ ----------------------------
/   Given the path of an object then lets get the ObjectClass for it,
/   this involves searching the cache for an entry and then
/   binding if we don't find one.  
/
/ In:
/   pPath -> ADsPath for the object
/
/ Out:
/   pObjectClass -> object class record in the cache
/----------------------------------------------------------------------------*/

LPCWSTR CObjectCache::GetObjectClass(LPCWSTR pPath)
{
    HRESULT hr;
    LPCWSTR pObjectClass = NULL;
    IADs* pDsObject = NULL;
    BSTR bstrObjectClass = NULL;
    INT i;
    USES_CONVERSION;

    TraceEnter(TRACE_IDLIST, "CObjectCache::GetObjectClass");
    Trace(TEXT("pPath %s"), W2CT(pPath));

    // compute the key and lets look into the cache and see if we
    // can find a match.  

    for ( i = _ComputeKey(pPath) ; (i < CACHE_SIZE) && m_cache[i].pPath ; i++ )
    {
        if ( !StrCmpW(pPath, m_cache[i].pPath) )
        {
            Trace(TEXT("Found entry at index %d"), i);
            pObjectClass = m_cache[i].pObjectClass;
            goto exit_gracefully;
        }
    }

    // we didn't find an entry in the cache (and either hit an emty slot,
    // or hit the end of the table).  So now we must bind to the object
    // and get the information, before filling the record.

// BUGBUG: this we must revisit
    hr = ADsOpenObject((LPWSTR)pPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IADs, (LPVOID*)&pDsObject);
    FailGracefully(hr, "Failed to bind to the object");

    hr = pDsObject->get_Class(&bstrObjectClass);
    FailGracefully(hr, "Failed to get the ObjectClass");

    // discard the previous entry (if it was filled), and now lets write new information into it.
    
    if ( i >= CACHE_SIZE )
    {
        TraceMsg("Table is already filled, so lets recompute the index");
        i = _ComputeKey(pPath);
    }

    LocalFreeStringW(&m_cache[i].pPath);
    LocalFreeStringW(&m_cache[i].pObjectClass);

    if ( FAILED(LocalAllocStringW(&m_cache[i].pPath, pPath)) ||
            FAILED(LocalAllocStringW(&m_cache[i].pObjectClass, bstrObjectClass)) )
    {
        LocalFreeStringW(&m_cache[i].pPath);
        LocalFreeStringW(&m_cache[i].pObjectClass);
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to fill the record");
    }

    pObjectClass = m_cache[i].pObjectClass;
    TraceAssert(pObjectClass);

exit_gracefully:

    Trace(TEXT("pObjectClass: %s"), pObjectClass ? W2CT(pObjectClass):TEXT("<not found>"));

    DoRelease(pDsObject);
    SysFreeString(bstrObjectClass);

    TraceLeaveValue(pObjectClass);
}


/*-----------------------------------------------------------------------------
/ Helper functions
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ _GetString
/ -----------
/   Helper function to extract a string and increase the pointers
/   as required.
/
/ In:
/   ppString -> string data we are walking
/   ppResult -> receives either the pointer or NULL
/   fExtract = if the string should be read
/
/ Out:
/   ppString -> updated to point past string extracted
/   ppResult -> receives either the string pointer or NULL
/----------------------------------------------------------------------------*/
void _GetString(LPWSTR* ppString, LPWSTR* ppResult, BOOL fExtract)
{
    TraceEnter(TRACE_IDLIST, "_GetString");

    *ppResult = NULL;

    if ( fExtract )
    {
        *ppResult  = *ppString;
        *ppString += lstrlenW(*ppString)+1;
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ _PutString
/ ----------
/   Write a string at the given pointer, updating to reflect the change
/   perform.
/
/ In:
/   ppBuffer -> buffer to write the string to
/   pString -> string to be written
/   fAppend = if the string should be appended
/
/ Out:
/   ppBuffer updated to reflect new string added
/----------------------------------------------------------------------------*/
void _PutString(LPWSTR* ppBuffer, LPWSTR pString, BOOL fAppend)
{
    TraceEnter(TRACE_IDLIST, "_PutString");

    if ( fAppend )
    {
        StrCpyW(*ppBuffer, pString);
        *ppBuffer += lstrlenW(pString)+1;
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ IDLIST pack/unpack functions
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ CreateIdList
/ ------------
/   Convert a IDLISTDATA into a ITEMIDLIST that the shell can cope with,
/   including packing all the data and hiding any Unicode / ANSI nastyness.
/
/ In:
/   ppidl -> receives the newly allocated IDLIST.
/   pData -> IDLISTDATA structure to create the IDL from.
/
/ Out:
/   hresult.
/----------------------------------------------------------------------------*/
HRESULT CreateIdList(LPITEMIDLIST* ppidl, LPIDLISTDATA pData, IMalloc* pm)
{
    HRESULT hr = E_INVALIDARG;
    LPOURIDLIST pOurIdList = NULL;
    LPWSTR pStringData;
    int cbSize = SIZEOF(OURIDLIST);
    USES_CONVERSION;

    TraceEnter(TRACE_IDLIST, "CreateIdList");

    TraceAssert(ppidl);
    TraceAssert(pData);

    // Compute the size of the string using the elements we have

    cbSize += StringByteSizeW(pData->pName);
    cbSize += StringByteSizeW(pData->pObjectClass);
    cbSize += StringByteSizeW(pData->pPathElement);
    cbSize += StringByteSizeW(pData->pUNC);

    Trace(TEXT("cbSize %d (%04x)"), cbSize, cbSize);

    // if we have a IMalloc then we are a delegate object, so we must call the malloc
    // implementation and then we can indirect into the structure we get allocated.  Otherwise
    // we use the old implementation which does a SHAlloc.

#if DELEGATE
    if ( !pm )
        ExitGracefully(hr, E_INVALIDARG, "No IMalloc interface to alloc IDLIST with");

    pOurIdList = (LPOURIDLIST)pm->Alloc(cbSize+SIZEOF(WORD));
    TraceAssert(pOurIdList);
#else
    pOurIdList = (LPOURIDLIST)SHAlloc(cbSize+SIZEOF(WORD));
    TraceAssert(pOurIdList);
#endif

    // - zero the structure and then fill the fields
    // - write the pointer to the structure if it succeeded

    if ( !pOurIdList )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate IDLIST");

#if !DELEGATE
    pOurIdList->cbSize = (WORD)cbSize;
#endif

    *(LPWORD)ByteOffset(pOurIdList, pOurIdList->cbSize) = 0;       // terminate

    pOurIdList->cbToStrings = offsetof(OURIDLIST, szStringData);

    pOurIdList->dwMagic = DSIDL_MAGIC;
    pOurIdList->dwFlags = pData->dwFlags;
    pOurIdList->dwProviderFlags = pData->dwProviderFlags;

    // write the string data...

    pStringData = pOurIdList->szStringData;

    if ( pData->pName )
    {
        pOurIdList->dwFlags |= DSIDL_HASNAME;
        _PutString(&pStringData, pData->pName, TRUE);
    }

    if ( pData->pObjectClass )
    {
        pOurIdList->dwFlags |= DSIDL_HASCLASS;
        _PutString(&pStringData, pData->pObjectClass, TRUE);
    }

    _PutString(&pStringData, pData->pPathElement, TRUE);

    if ( pData->pUNC )
    {
        pOurIdList->dwFlags |= DSIDL_HASUNC;
        _PutString(&pStringData, pData->pUNC, TRUE);
    }

    *ppidl = (LPITEMIDLIST)pOurIdList;
    TraceAssert(*ppidl);

    hr = S_OK;                              // success

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ CreateIdListFromPath
/ --------------------
/   Given a ADsPath and its ObjectClass convert this to a IDLIST.
/
/ In:
/   ppidl = receives a pointer to new IDLIST
/   pName, pPath, pObjectClass => string information about object
/   pm -> IMalloc to allocate the IDLIST using
/   pdds -> IDsDisplaySpecifier object
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CreateIdListFromPath(LPITEMIDLIST* ppidl, 
                             LPWSTR pName, LPWSTR pPath, LPWSTR pObjectClass, LPWSTR pUNC, 
                             IMalloc* pm, IDsDisplaySpecifier *pdds)
{
    HRESULT hr;
    IDLISTDATA data = { 0 };
    INT i;
    USES_CONVERSION;

    TraceEnter(TRACE_IDLIST, "CreateIdListFromPath");
    Trace(TEXT("pName %s"), pName ? W2CT(pName):TEXT("<none specified>"));
    Trace(TEXT("pPath %s"), W2CT(pPath));
    Trace(TEXT("pObjectClass %s"), pObjectClass ? W2CT(pObjectClass):TEXT("<none specified>"));

    // Having plucked the useful information from the objects descriptor
    // now attempt to build an IDLIST structure that represents it,
    // to do this we build an IDLISTDATA structure and get it packed.

    if ( pObjectClass && !pUNC )
    {
        if ( pdds->IsClassContainer(pObjectClass, pPath, 0x0) )
            data.dwFlags |= DSIDL_ISCONTAINER;
    }
    else
    {
        data.dwFlags |= DSIDL_ISCONTAINER;
    }

    // if we have a parent path then lets ensure that we strim it from the path we are going
    // to put into the IDLIST.  
 
    data.dwProviderFlags = 0;
    data.pName = pName;
    data.pObjectClass = pObjectClass;
    data.pPathElement = pPath;
    data.pUNC = pUNC;

    // construct the IDLIST

    hr = CreateIdList(ppidl, &data, pm);
    FailGracefully(hr, "Failed to create IDLIST from data structure");

    hr = S_OK;              // success

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ UnpackIdList
/ ------------
/   Extract all the interesting information from the ITEMIDLIST into an
/   one of our descriptive structures.
/
/ In:
/   pidl -> ITEMIDLIST to expand.
/   dwFlags = flags indicating important fields
/   pData -> where to expand it into.
/
/ Out:
/   hresult.
/----------------------------------------------------------------------------*/
HRESULT UnpackIdList(LPCITEMIDLIST pidl, DWORD dwFlags, LPIDLISTDATA pData)
{
    LPOURIDLIST pOurIdList = (LPOURIDLIST)pidl;
    LPWSTR pStringData;
    IADs* pDsObject = NULL;
    BSTR bstrObjectClass = NULL;
    HRESULT hr;

    TraceEnter(TRACE_IDLIST, "UnpackIdList");

    TraceAssert(pOurIdList);
    TraceAssert(pData);

    // is the IDLIST valid?  If so unpack it, otherwise bail 

    if ( !pOurIdList || ILIsEmpty(pidl) )
        ExitGracefully(hr, E_INVALIDARG, "No IDLIST given");

    if ( pOurIdList->dwMagic != DSIDL_MAGIC )
    {
        Trace(TEXT("dwMagic contains %08x, rather than %08x"), pOurIdList->dwMagic, DSIDL_MAGIC);
        ExitGracefully(hr, E_INVALIDARG, "Bad guard word, cannot unpack");
    }

    pData->dwFlags = pOurIdList->dwFlags;
    pData->dwProviderFlags = pOurIdList->dwProviderFlags;

    pStringData = (LPWSTR)ByteOffset(pOurIdList, pOurIdList->cbToStrings);

    _GetString(&pStringData, &pData->pName, (pData->dwFlags & DSIDL_HASNAME));
    _GetString(&pStringData, &pData->pObjectClass, (pData->dwFlags & DSIDL_HASCLASS));
    _GetString(&pStringData, &pData->pPathElement, TRUE);
    _GetString(&pStringData, &pData->pUNC, (pData->dwFlags & DSIDL_HASUNC));

    // check the cache for the object class if we haven't unpacked it.

    if ( !(pData->dwFlags & DSIDL_HASCLASS) && (dwFlags & DSIDL_HASCLASS) )
    {
        LPCWSTR pObjectClass;

        TraceMsg("IDLIST is simple and has no ObjectClass");

// BUGBUG: crit section
        pObjectClass = g_ObjectCache.GetObjectClass(pData->pPathElement);
        TraceAssert(pObjectClass);

        if ( !pObjectClass )
            ExitGracefully(hr, E_UNEXPECTED, "Failed to get cached ObjectClass");

        StrCpyW(pData->szObjectClass, pObjectClass);
        pData->pObjectClass = pData->szObjectClass;
    }

    hr = S_OK;          // success

exit_gracefully:

    DoRelease(pDsObject);
    SysFreeString(bstrObjectClass);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ AttributesFromIDL
/ -----------------
/   Build the attributes for the given IDLIST data.  This information is
/   derived from both the IDLISTDATA structure and also the information
/   stored in the registry.
/
/ In:
/   pData -> IDLISTDATA structure
/
/ Out:
/   fAttrib = attributes matching the object
/----------------------------------------------------------------------------*/
ULONG AttributesFromIdList(LPIDLISTDATA pData)
{
    HRESULT hr;
    HKEY aKeys[UIKEY_MAX];
    ULONG fResult = SFGAO_CANLINK;
    ULONG fAttributes;
    DWORD dwType, cb;
    LONG err;
    int i;

    TraceEnter(TRACE_IDLIST, "AttributesFromIdList");

    ZeroMemory(aKeys, SIZEOF(aKeys));

    // Some bits come from the IDLIST we are looking at.

    if ( pData->dwFlags & (DSIDL_ISCONTAINER|DSIDL_HASUNC) )
        fResult |= SFGAO_FOLDER|SFGAO_HASSUBFOLDER|SFGAO_FILESYSANCESTOR;

    // Other bits come from the registry under the "ui" keys

    hr = GetKeysForIdList(NULL, pData, ARRAYSIZE(aKeys), aKeys);
    FailGracefully(hr, "Failed when getting keys");

    for ( i = 0 ; i < ARRAYSIZE(aKeys); i++ )
    {
        if ( aKeys[i] )
        {
            cb = SIZEOF(fAttributes);
            if ( ERROR_SUCCESS == RegQueryValueEx(aKeys[i], c_szAttributes, NULL, &dwType, (LPBYTE)&fAttributes, &cb) )
            {
               fResult |= fAttributes;
            }
        }
    }

    hr = S_OK;                      // success

exit_gracefully:
    
    TidyKeys(ARRAYSIZE(aKeys), aKeys);

    TraceLeaveValue(fResult);
}


/*-----------------------------------------------------------------------------
/ NameFromIDL
/ -----------
/   Crack the RDN from a IDLIST.   Using the path cracker API we are given.
/
/ In:
/   pidl -> idlist to be converted
/   pidlParent -> parent of the idlist ( == pidlParent ++ pidl )
/   ppName -> receives a pointer to the name string
/   papn -> pathname interface
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT NameFromIdList(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlParent, LPWSTR pName, INT cchName, IADsPathname* papn)
{
    HRESULT hr;
    LPWSTR pPath = NULL;
    BSTR bstrName = NULL;
    LPITEMIDLIST pidlT = NULL;
    IDLISTDATA data = { 0 };

    TraceEnter(TRACE_IDLIST, "NameFromIDL");
   
    // do we have the papn interface we need, if not then lets grab one, 
    // otherwise AddRef the one we have so we can just release on exit.

    if ( !papn )
    {
        hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (LPVOID*)&papn);
        FailGracefully(hr, "Failed to get the IADsPathname interface");
    }
    else
    {
        papn->AddRef();
    }

    // check to see if the idlist has a name encoded into it?

    hr = UnpackIdList(pidl, 0x0, &data);
    FailGracefully(hr, "Failed to unpack the IDLIST");

    if ( data.dwFlags & DSIDL_HASNAME )
    {
        TraceMsg("Copying name from the IDLISTDATA");
        StrCpyNW(pName, data.pName, cchName);
    }
    else
    {
        TraceMsg("Extracting the name fromt the full path");

        // get the name, feed it into the name cracker and then lets put the
        // result into a buffer that the caller can use...

        if ( pidlParent )
        {
            pidlT = ILCombine(pidlParent, pidl);
            TraceAssert(pidlT);

            if ( !pidlT )
                ExitGracefully(hr, E_OUTOFMEMORY, "Failed to combine the PIDL");
        }

        hr = PathFromIdList(pidlT ? pidlT:pidl, &pPath, papn);
        FailGracefully(hr, "Failed to get the path from the IDLIST");

        hr = papn->Set(pPath, ADS_SETTYPE_FULL);
        FailGracefully(hr, "Failed to set the path of the name");

        papn->SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
    
        hr = papn->Retrieve(ADS_FORMAT_LEAF, &bstrName);
        FailGracefully(hr, "Failed to get leaf name");

        StrCpyNW(pName, bstrName, cchName);
    }

    hr = S_OK;

exit_gracefully:

    LocalFreeStringW(&pPath);
    SysFreeString(bstrName);

    DoRelease(papn);
    DoILFree(pidlT);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ PathFromIdList
/ -------------
/   Convert the given IDLIST to a path, we do this by finding
/   the last element and the unpacking that.
/
/ In:
/   pidl -> idlist to be converted
/   ppPath -> receives a pointer to the newly allocated string
/   papn -> IADsPathname interface we can use for converting the path to X500 format
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT PathFromIdList(LPCITEMIDLIST pidl, LPWSTR* ppPath, IADsPathname* papn)
{
    HRESULT hr = E_FAIL;
    IDLISTDATA data = { 0 };
    
    TraceEnter(TRACE_IDLIST, "PathFromIdList");

    hr = UnpackIdList(ILFindLastID(pidl), 0x0, &data);
    FailGracefully(hr, "Failed to unpack the IDLIST");
    
    hr = LocalAllocStringW(ppPath, data.pPathElement);
    FailGracefully(hr, "Failed to allocate the path element");

    hr = S_OK;          // success

exit_gracefully:

    TraceLeaveResult(hr);
}
