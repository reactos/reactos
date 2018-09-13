#include "priv.h"
#include "comcatex.h"
#include "runtask.h"
#include "dbgmem.h"     // remove_from_memlist


//------------------//
//  Misc constants
static LPCTSTR
    REGVAL_COMCATEX              = STRREG_DISCARDABLE STRREG_POSTSETUP TEXT("\\Component Categories"),
    REGKEY_COMCATEX_ENUM         = TEXT("Enum"),        // HKCR\ComponentClasses\{catid}\Enum
    REGVAL_COMCATEX_IMPLEMENTING = TEXT("Implementing"),// HKCR\ComponentClasses\{catid}\Enum\Implementing
    REGVAL_COMCATEX_REQUIRING    = TEXT("Requiring");   // HKCR\ComponentClasses\{catid}\Enum\Requiring

static const ULONG 
    COMCAT_CACHE_CURRENTVERSION = MAKELONG(1,0); // current cache version.

//-------------//
//  Cache header
typedef struct {
    ULONG       cbStruct;      // structure size
    ULONG       ver;           // version string (COMCAT_CACHE_CURRENTVERSION)
    SYSTEMTIME  stLastUpdate;  // UTC date, time of last update.
    ULONG       cClsid;        // number of CLSIDs to follow
    CLSID       clsid[];       // array of CLSIDs
} COMCAT_CACHE_HEADER;

//----------------//
//  Impl helpers
STDMETHODIMP _EnumerateGuids( IN IEnumGUID* pEnumGUID, OUT HDSA* phdsa );
STDMETHODIMP _ComCatCacheFromDSA( IN HDSA hdsa, OUT LPBYTE* pBuf, OUT LPDWORD pcbBuf );
STDMETHODIMP _DSAFromComCatCache( IN LPBYTE pBuf, IN ULONG cbBuf, OUT HDSA* phdsa );
STDMETHODIMP _MakeComCatCacheKey( IN REFCATID refcatid, OUT LPTSTR pszKey, IN ULONG cchKey );
STDMETHODIMP _ReadClassesOfCategory( IN REFCATID refcatid, OUT HDSA* phdsa, LPCTSTR pszRegValueName );
STDMETHODIMP _WriteImplementingClassesOfCategory( IN REFCATID refcatid, IN HDSA hdsa );
STDMETHODIMP _WriteRequiringClassesOfCategory( IN REFCATID refcatid, IN HDSA hdsa );
STDMETHODIMP _WriteClassesOfCategories( IN ULONG, IN CATID [], IN ULONG, IN CATID [], BOOL );
STDAPI       _CComCatCache_CommonCreateInstance( BOOL, OUT void**);

//-----------------------//
//  Higher-level methods
STDMETHODIMP SHReadImplementingClassesOfCategory( REFCATID refcatid, OUT HDSA* phdsa );
STDMETHODIMP SHReadRequiringClassesOfCategory( REFCATID refcatid, OUT HDSA* phdsa );
STDMETHODIMP SHWriteImplementingClassesOfCategory( REFCATID refcatid );
STDMETHODIMP SHWriteRequiringClassesOfCategory( REFCATID refcatid );

#define SAFE_DESTROY_CLSID_DSA(hdsa) \
    if((hdsa)) { DSA_Destroy((hdsa)); (hdsa)=NULL; }

//-------------------------------------------------------------------------//
//  Cache-aware component categories enumerator object
class CSHEnumClassesOfCategories : public IEnumGUID
//-------------------------------------------------------------------------//
{
public:
    //  IUnknown methods
    STDMETHOD_ (ULONG, AddRef)()    { 
        return InterlockedIncrement( &_cRef );
    }
    STDMETHOD_ (ULONG, Release)()   { 
        if( InterlockedDecrement( &_cRef )==0 ) {
            delete this; return 0;
        }
        return _cRef; 
    }
    STDMETHOD  (QueryInterface)( REFIID riid, void **ppvObj);

    //  IEnum methods
    STDMETHOD (Next)( ULONG celt, GUID* rgelt, ULONG* pceltFetched );
    STDMETHOD (Skip)( ULONG celt );
    STDMETHOD (Reset)();
    STDMETHOD (Clone)( IEnumGUID ** ppenum );

protected:
    CSHEnumClassesOfCategories();
    virtual ~CSHEnumClassesOfCategories();

    STDMETHOD (Initialize)( ULONG cImpl, CATID rgcatidImpl[], ULONG cReq, CATID rgcatidReq[]); 
        // invoke immediately after construction for arg validation.
    
    LONG      _cRef,        // ref count
              _iEnum;      // enumerator index
    HDSA      _hdsa;       // CLSID DSA handle

    ULONG     _cImpl,        // count of catids to enumerate for implementing classes
              _cReq;        // count of catids to enumerate for requiring classes
    CATID     *_rgcatidImpl, // catids to enumerate for implementing classes
              *_rgcatidReq; // catids to enumerate for requiring classes
              
    friend STDMETHODIMP SHEnumClassesOfCategories( ULONG, CATID[], ULONG, CATID[], IEnumGUID**);
};

//-------------------------------------------------------------------------//
//  IRunnableTask derivative for asynchronous update of 
//  component categories cache.
class CComCatCacheTask : public CRunnableTask
//-------------------------------------------------------------------------//
{
public:
    CComCatCacheTask();
    virtual ~CComCatCacheTask();

    STDMETHOD   (Initialize)( ULONG cImplemented,
                              CATID rgcatidImpl[],
                              ULONG cRequired,
                              CATID rgcatidReq[],
                              BOOL  bForceUpdate );

    STDMETHOD   (Go)();

protected:
    STDMETHOD   (RunInitRT)()   {
        return _WriteClassesOfCategories( _cImpl, _rgcatidImpl,
                                          _cReq, _rgcatidReq, _bForceUpdate );
    }

    ULONG _cImpl, _cReq;
    CATID *_rgcatidImpl,
          *_rgcatidReq;
    BOOL  _bForceUpdate;

    friend HRESULT _CComCatCache_CommonCreateInstance( BOOL, OUT void**);
};

//-------------------------------------------------------------------------//
//  Entrypoint: retrieves cache-aware enumerator over classes which require or 
//  implement the specified component catagory(ies).
STDMETHODIMP SHEnumClassesOfCategories(
      ULONG cImplemented,       //Number of category IDs in the rgcatidImpl array
      CATID rgcatidImpl[],        //Array of category identifiers
      ULONG cRequired,          //Number of category IDs in the rgcatidReq array
      CATID rgcatidReq[],         //Array of category identifiers
      IEnumGUID** ppenumGUID )  //Location in which to return an IEnumGUID interface
{
    HRESULT hr = S_OK;
    CSHEnumClassesOfCategories* pEnum = NULL;
    
    if( NULL == ppenumGUID )
        return E_INVALIDARG;

    *ppenumGUID = NULL;

    //  Construct and initialize enumerator object
    if( NULL == (pEnum = new CSHEnumClassesOfCategories) )
        return E_OUTOFMEMORY;

    if( FAILED( (hr = pEnum->Initialize( 
                    cImplemented, rgcatidImpl, cRequired, rgcatidReq )) ) )
    {
        pEnum->Release();
        return hr;
    }

    *ppenumGUID = pEnum;
    return hr;
}

//-------------------------------------------------------------------------//
//  Determines whether a cache exists for the indicated CATID.
//  If bImplementing is TRUE, the function checks for a cache of
//  implementing classes; otherwise the function checks for a cache of
//  requiring classes.
STDMETHODIMP SHDoesComCatCacheExist( REFCATID refcatid, BOOL bImplementing )
{
    TCHAR szKey[MAX_PATH];
    HRESULT hr;

    if( SUCCEEDED( (hr = _MakeComCatCacheKey( refcatid, szKey, ARRAYSIZE(szKey) )) ) )
    {
        HKEY  hkeyCache;
        
        DWORD dwRet = RegOpenKeyEx( HKEY_CLASSES_ROOT, szKey, 0L, KEY_READ, &hkeyCache );
        hr = S_FALSE;

        if( ERROR_SUCCESS == dwRet )
        {
            DWORD   dwType, cbData = 0;

            dwRet = RegQueryValueEx( hkeyCache, 
                                     bImplementing ? REGVAL_COMCATEX_IMPLEMENTING : 
                                                     REGVAL_COMCATEX_REQUIRING,
                                     0L, &dwType, NULL, &cbData );

            //  We'll confirm only on value type and size of data.
            if( ERROR_SUCCESS == dwRet && 
                dwType == REG_BINARY &&  
                sizeof(COMCAT_CACHE_HEADER) <= cbData )
            {
                hr = S_OK;
            }

            RegCloseKey( hkeyCache );
        }
    }
    return hr;
}

//-------------------------------------------------------------------------//
//  Entrypoint: Caches implementing and requiring classes for the 
//  specified categories with asynchronous option.
STDMETHODIMP SHWriteClassesOfCategories( 
      ULONG cImplemented,       //Number of category IDs in the rgcatidImpl array
      CATID rgcatidImpl[],      //Array of category identifiers
      ULONG cRequired,          //Number of category IDs in the rgcatidReq array
      CATID rgcatidReq[],       //Array of category identifiers
      BOOL  bForceUpdate,       // TRUE: Unconditionally update the cache; FALSE: create cache iif doesn't exist.
      BOOL  bWait )             //If FALSE, the function returns immediately and the
                                //   caching occurs asynchronously; otherwise
                                //   the function returns only after the caching
                                //   operation has completed.
{
    HRESULT hr = S_OK;

    if( bWait )
        //  Synchronous update
        return _WriteClassesOfCategories( cImplemented, rgcatidImpl, cRequired, rgcatidReq, bForceUpdate );

    //  Asynchronous update
    CComCatCacheTask* pTask;
    if( NULL == (pTask = new CComCatCacheTask) )
        return E_OUTOFMEMORY;

    //  Initialize with caller's args:
    if( SUCCEEDED( (hr = pTask->Initialize( 
            cImplemented, rgcatidImpl, cRequired, rgcatidReq, bForceUpdate )) ) )
    {
        hr = pTask->Go();
    }
    
    pTask->Release();
    return hr;
}

//-------------------------------------------------------------------------//
//  CSHEnumClassesOfCategories class implementation
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
inline CSHEnumClassesOfCategories::CSHEnumClassesOfCategories()
    :   _cImpl(0), _rgcatidImpl(NULL),
        _cReq(0), _rgcatidReq(NULL),
        _cRef(1),  _iEnum(0), _hdsa(NULL)
{
    DllAddRef();
}

//-------------------------------------------------------------------------//
CSHEnumClassesOfCategories::~CSHEnumClassesOfCategories()
{
    delete [] _rgcatidImpl;
    delete [] _rgcatidReq;
    SAFE_DESTROY_CLSID_DSA( _hdsa );
    DllRelease();
}

//-------------------------------------------------------------------------//
STDMETHODIMP CSHEnumClassesOfCategories::QueryInterface( REFIID riid, void **ppvObj )
{
    static const QITAB qit[] = {
        QITABENT(CSHEnumClassesOfCategories, IEnumGUID),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

//-------------------------------------------------------------------------//
STDMETHODIMP CSHEnumClassesOfCategories::Initialize(
    ULONG cImplemented, 
    CATID rgcatidImpl[], 
    ULONG cRequired, 
    CATID rgcatidReq[]
)
{
    //  Disallow multiple initialization.
    if( _hdsa || _rgcatidImpl || _rgcatidReq )
        return S_FALSE;
    
    //  Superficial arg validation:
    if( (0==cImplemented && 0==cRequired) ||
        (cImplemented && NULL == rgcatidImpl) ||
        (cRequired && NULL == rgcatidReq) )
    {
        return E_INVALIDARG;
    }

    //  Allocate and make copies of CATID arrays
    if( cImplemented )
    {
        if( NULL == (_rgcatidImpl = new CATID[cImplemented]) )
            return E_OUTOFMEMORY;
        CopyMemory( _rgcatidImpl, rgcatidImpl, sizeof(CATID) * cImplemented );
    }
    _cImpl = cImplemented;

    if( cRequired )
    {
        if( NULL == (_rgcatidReq = new CATID[cRequired]) )
            return E_OUTOFMEMORY;
        CopyMemory( _rgcatidReq, rgcatidReq, sizeof(CATID) * cRequired );
    }
    _cReq = cRequired;

    return S_OK;
}

//-------------------------------------------------------------------------//
//  Iterates implementing and/or requiring classes for the caller-specified
//  component categories.
STDMETHODIMP CSHEnumClassesOfCategories::Next( 
    ULONG celt, 
    GUID* rgelt, 
    ULONG* pceltFetched )
{
    if( pceltFetched )
        *pceltFetched = 0;
    
    HRESULT hr = S_FALSE;
    ULONG celtFetched = 0;

    //  Have we assembled our collection?
    if( NULL == _hdsa )
    {
        _iEnum = 0;

        ULONG i;
        for( i=0; SUCCEEDED( hr ) && i < _cImpl; i++ )
        {
            //  Try reading implementing classes from cache
            if( FAILED( (hr = SHReadImplementingClassesOfCategory( _rgcatidImpl[i], &_hdsa )) ) )
            {
                //  Uncached; try caching and then re-read.
                if( FAILED( (hr = SHWriteImplementingClassesOfCategory( _rgcatidImpl[i] )) ) ||
                    FAILED( (hr = SHReadImplementingClassesOfCategory( _rgcatidImpl[i], &_hdsa )) ) )
                    break;
            }
        }

        for( i=0; SUCCEEDED( hr ) && i < _cReq; i++ )
        {
            //  Try reading requiring classes from cache
            if( FAILED( (hr = SHReadRequiringClassesOfCategory( _rgcatidReq[i], &_hdsa )) ) )
            {
                //  Uncached; try caching and then re-read.
                if( FAILED( (hr = SHWriteRequiringClassesOfCategory( _rgcatidReq[i] )) ) ||
                    FAILED( (hr = SHReadRequiringClassesOfCategory( _rgcatidReq[i], &_hdsa )) ) )
                    break;
            }
        }
    }

    if( NULL != _hdsa )
    {
        LONG count = DSA_GetItemCount( _hdsa );
        while( celtFetched < celt && _iEnum < count  )
        {
            if( DSA_GetItem( _hdsa, _iEnum, &rgelt[celtFetched] ) )
                celtFetched++;

            _iEnum++;
        }

        return celtFetched == celt ? S_OK : S_FALSE;
    }

    return SUCCEEDED( hr ) ? S_FALSE : hr;
}

//-------------------------------------------------------------------------//
inline STDMETHODIMP CSHEnumClassesOfCategories::Skip( ULONG celt )      
{ 
    InterlockedExchange( &_iEnum, _iEnum + celt );
    return S_OK; 
}

//-------------------------------------------------------------------------//
inline STDMETHODIMP CSHEnumClassesOfCategories::Reset( void )      
{ 
    InterlockedExchange( &_iEnum, 0 );
    return S_OK; 
}

//-------------------------------------------------------------------------//
inline STDMETHODIMP CSHEnumClassesOfCategories::Clone( IEnumGUID ** ppenum )
{
    return E_NOTIMPL;
}

//-------------------------------------------------------------------------//
//  CComCatCacheTask class implementation
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
STDAPI CComCatConditionalCacheTask_CreateInstance( IN IUnknown*, OUT void** ppOut, LPCOBJECTINFO )
{
    return _CComCatCache_CommonCreateInstance( FALSE /* iif not exists */, ppOut );
}

//-------------------------------------------------------------------------//
STDAPI CComCatCacheTask_CreateInstance( IN IUnknown*, OUT void** ppOut, LPCOBJECTINFO poi )
{
    return _CComCatCache_CommonCreateInstance( TRUE /* unconditionally update */, ppOut );
}

//-------------------------------------------------------------------------//
STDAPI _CComCatCache_CommonCreateInstance( 
    BOOL bForceUpdate, 
    OUT void** ppOut )
{
    CComCatCacheTask* pTask;
    if( NULL == (pTask = new CComCatCacheTask) )
        return E_OUTOFMEMORY;

    HRESULT hr = S_OK;

    //  We're being CoCreated without args, so we'll use
    //  a hard-coded list of likely suspects (catids) to cache.
    static CATID rgcatid[2];
    rgcatid[0] = CATID_InfoBand;
    rgcatid[1] = CATID_CommBand;
    if( FAILED( (hr = pTask->Initialize( ARRAYSIZE(rgcatid), rgcatid, 0, NULL, bForceUpdate )) ) )
    {
        pTask->Release();
        return hr;
    }

    *ppOut = SAFECAST( pTask, IRunnableTask*);
    return hr;
}

//-------------------------------------------------------------------------//
inline CComCatCacheTask::CComCatCacheTask()
    :  CRunnableTask( RTF_DEFAULT ), 
       _cImpl(0), _cReq(0), _rgcatidImpl(NULL), _rgcatidReq(NULL), _bForceUpdate(TRUE)
{
}

//-------------------------------------------------------------------------//
inline CComCatCacheTask::~CComCatCacheTask()
{
    delete [] _rgcatidImpl;
    delete [] _rgcatidReq;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CComCatCacheTask::Initialize(
    ULONG cImplemented,
    CATID rgcatidImpl[],
    ULONG cRequired,
    CATID rgcatidReq[],
    BOOL  bForceUpdate )
{
    //  Superficial arg validation:
    if( (0==cImplemented && 0==cRequired) ||
        (cImplemented && NULL == rgcatidImpl) ||
        (cRequired && NULL == rgcatidReq) )
    {
        return E_INVALIDARG;
    }

    //  Disallow multiple initialization.
    if( _rgcatidImpl || _rgcatidReq )
        return S_FALSE;

    //  Allocate and make copies of CATID arrays
    if( cImplemented )
    {
        if( NULL == (_rgcatidImpl = new CATID[cImplemented]) )
            return E_OUTOFMEMORY;
        CopyMemory( _rgcatidImpl, rgcatidImpl, sizeof(CATID) * cImplemented );
    }
    _cImpl = cImplemented;

    if( cRequired )
    {
        if( NULL == (_rgcatidReq = new CATID[cRequired]) )
            return E_OUTOFMEMORY;
        CopyMemory( _rgcatidReq, rgcatidReq, sizeof(CATID) * cRequired );
    }
    _cReq = cRequired;

    _bForceUpdate = bForceUpdate;

    return S_OK;
}

//-------------------------------------------------------------------------//
//  Initiates asynchronous update of component categories cache.
STDMETHODIMP CComCatCacheTask::Go()
{
    //  Run the task from the shared thread pool
    IShellTaskScheduler* pScheduler;
    HRESULT hr = CoCreateInstance( CLSID_SharedTaskScheduler, NULL, CLSCTX_INPROC_SERVER, 
                                   IID_IShellTaskScheduler, (LPVOID*)&pScheduler );

    if( SUCCEEDED( hr ) )
    {
        hr = pScheduler->AddTask( this, CLSID_ComCatCacheTask, 0L, ITSAT_DEFAULT_PRIORITY );
        
        // heap alloc'd memory belongs to scheduler thread.
        if( _rgcatidImpl ) remove_from_memlist( _rgcatidImpl );
        if( _rgcatidReq  ) remove_from_memlist( _rgcatidReq  );
        remove_from_memlist( this );

        pScheduler->Release(); // OK to release shared scheduler before task has completed.
    }
    return hr;
}

//-------------------------------------------------------------------------//
//  Component cache implementation
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  Reads a series of CLSIDs from a registry-based cache of
//  implementing classes for the specified component category into a DSA.
//  If the DSA is NULL, a new DSA is created; otherwise the CLSIDS are appended to
//  the provided DSA.
inline STDMETHODIMP SHReadImplementingClassesOfCategory( 
    IN REFCATID refcatid, 
    OUT HDSA* phdsa )
{
    return _ReadClassesOfCategory( refcatid, phdsa, 
                                   REGVAL_COMCATEX_IMPLEMENTING );
}

//-------------------------------------------------------------------------//
//  Reads a series of CLSIDs from a registry-based cache of
//  requiring classes for the specified component category into a DSA.
//  If the DSA is NULL, a new DSA is created; otherwise the CLSIDS are appended to
//  the provided DSA.
inline STDMETHODIMP SHReadRequiringClassesOfCategory( 
    IN REFCATID refcatid, 
    OUT HDSA* phdsa )
{
    return _ReadClassesOfCategory( refcatid, phdsa,
                                   REGVAL_COMCATEX_REQUIRING );
}

//-------------------------------------------------------------------------//
//  Caches a list of classes which implement the indicated component category.
STDMETHODIMP SHWriteImplementingClassesOfCategory( IN REFCATID refcatid )
{
    HRESULT hr;
    
    //  Retrieve OLE component category manager
    ICatInformation* pci;
    if( SUCCEEDED( (hr = CoCreateInstance( CLSID_StdComponentCategoriesMgr, 
                                           NULL, CLSCTX_INPROC_SERVER, 
                                           IID_ICatInformation, (LPVOID*)&pci)) ) )
    {
        //  Retrieve enumerator over classes that implement the category
        IEnumGUID* pEnumGUID;
        if( SUCCEEDED( (hr = pci->EnumClassesOfCategories( 1, (CATID*)&refcatid, 
                                                           0, NULL, &pEnumGUID )) ) )
        {
            HDSA  hdsa = NULL;
            if( SUCCEEDED( (hr = _EnumerateGuids( pEnumGUID, &hdsa )) ) )
            {
                //  Write to cache
                hr = _WriteImplementingClassesOfCategory( refcatid, hdsa );
                SAFE_DESTROY_CLSID_DSA( hdsa );
            }
            pEnumGUID->Release();
        }
        pci->Release();
    }        
    return hr;
}

//-------------------------------------------------------------------------//
//  Caches a list of classes which require the indicated component category.
STDMETHODIMP SHWriteRequiringClassesOfCategory( IN REFCATID refcatid )
{
    HRESULT hr;
    
    //  Retrieve OLE component category manager
    ICatInformation* pci;
    if( SUCCEEDED( (hr = CoCreateInstance( CLSID_StdComponentCategoriesMgr, 
                                           NULL, CLSCTX_INPROC_SERVER, 
                                           IID_ICatInformation, (LPVOID*)&pci)) ) )
    {
        //  Retrieve enumerator over classes that require the category
        IEnumGUID* pEnumGUID;
        if( SUCCEEDED( (hr = pci->EnumClassesOfCategories( 0, NULL, 1, 
                                                           (CLSID*)&refcatid, 
                                                           &pEnumGUID )) ) )
        {
            HDSA  hdsa = NULL;
            if( SUCCEEDED( (hr = _EnumerateGuids( pEnumGUID, &hdsa )) ) )
            {
                //  Write to cache
                hr = _WriteRequiringClassesOfCategory( refcatid, hdsa );
                SAFE_DESTROY_CLSID_DSA( hdsa );
            }
            pEnumGUID->Release();
        }
        pci->Release();
    }        
    return hr;
}

//-------------------------------------------------------------------------//
//  Accepts a valid GUID enumerator and constructs an HDSA containing the GUIDS.
//  The caller is responsible for freeing the HDSA which may or may not
//  have been allocated.
STDMETHODIMP _EnumerateGuids( IEnumGUID* pEnumGUID, OUT HDSA* phdsa )
{
    ASSERT( pEnumGUID );
    ASSERT( phdsa );
    
    ULONG   celtFetched;
    CLSID   clsid;
    HRESULT hr;

    while( SUCCEEDED( (hr = pEnumGUID->Next( 1, &clsid, &celtFetched )) ) &&
           celtFetched > 0 )
    {
        if( NULL == *phdsa &&
            NULL == (*phdsa = DSA_Create( sizeof(CLSID), 4 )) )
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        DSA_AppendItem( *phdsa, &clsid );
    }

    // translate S_FALSE.
    return SUCCEEDED( hr ) ? S_OK : hr;
}     

//-------------------------------------------------------------------------//
//  Generates a persistable cache of CLSIDs derived from the CLSID* DSA.
STDMETHODIMP _ComCatCacheFromDSA( IN HDSA hdsa, OUT LPBYTE* pBuf, OUT LPDWORD pcbBuf )
{
    ASSERT( pBuf );
    ASSERT( pcbBuf );

    ULONG   cClsid = hdsa ? DSA_GetItemCount( hdsa ) : 0,
            cbBuf = sizeof(COMCAT_CACHE_HEADER) + (cClsid * sizeof(CLSID));
    HRESULT hr = S_OK;

    //  Allocate blob
    *pcbBuf = 0;
    if( NULL != (*pBuf = new BYTE[cbBuf]) )
    {
        //  Initialize header
        COMCAT_CACHE_HEADER* pCache = (COMCAT_CACHE_HEADER*)(*pBuf);
        pCache->cbStruct = sizeof(*pCache);
        pCache->ver      = COMCAT_CACHE_CURRENTVERSION;
        pCache->cClsid   = 0;
        GetSystemTime( &pCache->stLastUpdate );

        //  Copy CLSIDs
        for( ULONG i = 0; i< cClsid; i++ )
            DSA_GetItem( hdsa, i, &pCache->clsid[pCache->cClsid++] );

        //  Adjust output size.
        *pcbBuf = sizeof(*pCache) + (pCache->cClsid * sizeof(CLSID));
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

//-------------------------------------------------------------------------//
//  Appends CLSIDS from the cache buffer to the specified DSA.  If the DSA is
//  NULL, a new DSA is created.
STDMETHODIMP _DSAFromComCatCache( IN LPBYTE pBuf, IN ULONG cbBuf, OUT HDSA* phdsa )
{
    ASSERT( pBuf );
    ASSERT( phdsa );

    HRESULT hr = S_OK;
    COMCAT_CACHE_HEADER* pCache = (COMCAT_CACHE_HEADER*)pBuf;

    //  Validate header
    if( !( sizeof(*pCache) <= cbBuf && 
           sizeof(*pCache) == pCache->cbStruct &&
           COMCAT_CACHE_CURRENTVERSION == pCache->ver ) )
        return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

    //  Create the DSA if necessary
    if( 0 == pCache->cClsid )
        return S_FALSE;

    if( NULL == *phdsa && NULL == (*phdsa = DSA_Create( sizeof(CLSID), 4 )) )
        return E_OUTOFMEMORY;

    //  Copy CLSIDs from the cache to the DSA.
    for( ULONG i = 0; i< pCache->cClsid; i++ )
        DSA_AppendItem( *phdsa, &pCache->clsid[i] );

    return hr;
}

//-------------------------------------------------------------------------//
//  Constructs a component category registry cache key based on the 
//  specified CATID.
STDMETHODIMP _MakeComCatCacheKey( 
    IN REFCATID refcatid, 
    OUT LPTSTR pszKey, 
    IN ULONG cchKey )
{
    TCHAR szCLSID[GUIDSTR_MAX];
    
    if( SHStringFromGUID( refcatid, szCLSID, ARRAYSIZE(szCLSID) )<=0 )
        return E_INVALIDARG;

    ASSERT( cchKey > (ULONG)(lstrlen( REGVAL_COMCATEX ) + GUIDSTR_MAX) );

    //  "Component Categories\{clsid}\Enum"
    if( wnsprintf( pszKey, cchKey, TEXT("%s\\%s\\%s"),
                   REGVAL_COMCATEX, szCLSID, REGKEY_COMCATEX_ENUM ) > 0 )
        return S_OK;

    return E_FAIL;
}

//-------------------------------------------------------------------------//
//  Reads a cache of implementing or requiring classes info a CLSID DSA.
STDMETHODIMP _ReadClassesOfCategory( 
    IN REFCATID refcatid, 
    OUT HDSA* phdsa, 
    LPCTSTR pszRegValueName /*REGVAL_COMCATEX_IMPLEMENTING/REQUIRING*/ )
{
    TCHAR szKey[MAX_PATH];
    HRESULT hr;

    //  Create/Open key HKCR\Component Categories\{catid}\Enum
    if( SUCCEEDED( (hr = _MakeComCatCacheKey( refcatid, szKey, ARRAYSIZE(szKey) )) ) )
    {
        HKEY hkeyCache = NULL;
        DWORD dwRet = RegOpenKeyEx( HKEY_CURRENT_USER, szKey, 0L, KEY_READ, &hkeyCache );
        hr = HRESULT_FROM_WIN32( dwRet );

        if( SUCCEEDED( hr ) )
        {
            //  Determine required buffer size.
            LPBYTE  pBuf = NULL;
            ULONG   cbBuf = 0,
                    dwType,
                    dwRet = RegQueryValueEx( hkeyCache, pszRegValueName, 0L,
                                             &dwType, NULL, &cbBuf );
            
            if( ERROR_SUCCESS == dwRet )
            {
                //  Allocate buffer and read
                if( NULL != (pBuf = new BYTE[cbBuf]) )
                {
                    dwRet = RegQueryValueEx( hkeyCache, pszRegValueName, 0L,
                                             &dwType, pBuf, &cbBuf );
                    hr = HRESULT_FROM_WIN32( dwRet );
                }
                else
                    hr = E_OUTOFMEMORY;
            }
            else
                //  Failed read.
                hr = HRESULT_FROM_WIN32( dwRet );

            if( SUCCEEDED( hr ) ) 
                //  Gather CLSIDs into the DSA
                hr = REG_BINARY == dwType ? 
                     _DSAFromComCatCache( pBuf, cbBuf, phdsa ) : E_ABORT;
                
            if( pBuf ) delete [] pBuf;
            RegCloseKey( hkeyCache );
        }
    }
    return hr;
}

//-------------------------------------------------------------------------//
//  Writes a series of CLSIDs from a DSA to a registry-based cache of
//  implementing classes for the specified component category.
STDMETHODIMP _WriteImplementingClassesOfCategory( 
    IN REFCATID refcatid, 
    IN HDSA hdsa )
{
    
    TCHAR szKey[MAX_PATH];
    HRESULT hr;

    //  Create/Open key HKCR\Component Categories\{catid}\Enum
    if( SUCCEEDED( (hr = _MakeComCatCacheKey( refcatid, szKey, ARRAYSIZE(szKey) )) ) )
    {
        HKEY hkeyCache = NULL;
        ULONG dwRet, dwDisposition;

        dwRet = RegCreateKeyEx( HKEY_CURRENT_USER, szKey, 0L, 
                                NULL, 0L, KEY_WRITE, NULL,
                                &hkeyCache, &dwDisposition );
        hr = HRESULT_FROM_WIN32( dwRet );

        if( SUCCEEDED( hr ) )
        {
            //  Construct a blob containing cache data.
            LPBYTE pBuf;
            ULONG  cbBuf;
            if( SUCCEEDED( (hr = _ComCatCacheFromDSA( hdsa, &pBuf, &cbBuf )) ) )
            {
                //  Write it to 'Implementing' reg value
                hr = RegSetValueEx( hkeyCache, REGVAL_COMCATEX_IMPLEMENTING, 0L,
                                    REG_BINARY, pBuf, cbBuf );
                if( pBuf )
                    delete [] pBuf;
            }
            RegCloseKey( hkeyCache );
        }
    }
    return hr;
}

//-------------------------------------------------------------------------//
//  Writes a series of CLSIDs from a DSA to a registry-based cache of
//  requiring classes for the specified component category.
STDMETHODIMP _WriteRequiringClassesOfCategory( 
    IN REFCATID refcatid, 
    IN HDSA hdsa )
{
    
    TCHAR szKey[MAX_PATH];
    HRESULT hr;

    //  Create/Open key HKCR\Component Categories\{catid}\Enum
    if( SUCCEEDED( (hr = _MakeComCatCacheKey( refcatid, szKey, ARRAYSIZE(szKey) )) ) )
    {
        HKEY hkeyCache = NULL;
        ULONG dwRet, 
              dwDisposition;

        dwRet = RegCreateKeyEx( HKEY_CURRENT_USER, szKey, 0L, 
                                NULL, 0L, KEY_WRITE, NULL,
                                &hkeyCache, &dwDisposition );
        hr = HRESULT_FROM_WIN32( dwRet ); 

        if( SUCCEEDED( hr ) )
        {
            //  Construct a blob containing cache data.
            LPBYTE pBuf;
            ULONG  cbBuf;
            if( SUCCEEDED( (hr = _ComCatCacheFromDSA( hdsa, &pBuf, &cbBuf )) ) )
            {
                //  Write it to 'Requirng' reg value
                hr = RegSetValueEx( hkeyCache, REGVAL_COMCATEX_REQUIRING, 0L,
                                    REG_BINARY, pBuf, cbBuf );
                if( pBuf )
                    delete [] pBuf;
            }
            RegCloseKey( hkeyCache );
        }
    }
    return hr;
}

//-------------------------------------------------------------------------//
//  Does work of caching implementing and requiring classes for the specified categories
STDMETHODIMP _WriteClassesOfCategories( 
      ULONG cImplemented,       //Number of category IDs in the rgcatidImpl array
      CATID rgcatidImpl[],      //Array of category identifiers
      ULONG cRequired,          //Number of category IDs in the rgcatidReq array
      CATID rgcatidReq[],       //Array of category identifiers
      BOOL  bForceUpdate )      //TRUE: unconditionally update the cache; otherwise
                                // update iif the cache doesn't exist.
{
    HRESULT hr = S_OK;
    ULONG   i;

    //  Cache implementing classes of each category.
    for( i = 0; i< cImplemented; i++ )
    {
        if( bForceUpdate || S_OK != SHDoesComCatCacheExist( rgcatidImpl[i], TRUE ) )
        {
            HRESULT hrCatid;
            if( FAILED( (hrCatid = SHWriteImplementingClassesOfCategory( rgcatidImpl[i] )) ) )
                hr = hrCatid;
        }
    }

    //  Cache requiring classes of each category.
    for( i = 0; i< cRequired; i++ )
    {
        if( bForceUpdate || S_OK != SHDoesComCatCacheExist( rgcatidReq[i], FALSE ) )
        {
            HRESULT hrCatid;
            if( FAILED( (hrCatid = SHWriteRequiringClassesOfCategory( rgcatidReq[i] )) ) )
                hr = hrCatid;
        }
    }
    return hr;
}

