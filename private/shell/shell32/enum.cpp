#include "shellprv.h"
#include "enum.h"

#ifdef WINNT
#define ILEnumClone ILClone
#define ILEnumFree  ILFree
#else
// on win95, the cache is global to all processes
#define ILEnumClone ILGlobalClone
#define ILEnumFree  ILGlobalFree
#endif

class SFENUMCACHEELT
{
public:
    DWORD grfFlags;
    LPITEMIDLIST pidl;
};

class SFEnumCache : public IEnumIDList
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // *** IEnumIDList methods ***
    STDMETHOD(Next)(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumIDList **ppenum);
    

    SFEnumCache(IEnumIDList* penum, DWORD grfFlags, SFEnumCacheData* pcache, IShellFolder* psf);
    virtual ~SFEnumCache();
protected:
    
    
    void _InitCache();
    BOOL _IsCacheValid();
    static _CacheFreeCallback(LPVOID p, LPVOID d);
    void _FreeCache(HDSA hdsa) { DSA_DestroyCallback(hdsa, _CacheFreeCallback, 0); };
    void _FreePidlDPA();
    void _InitGlobalCache();
    HDSA _GetGlobalCacheDSA();

    LONG _cRef;
    int _iIndex;
    IEnumIDList *_penum;  // the thing we're wrapping
    DWORD _grfFlags;
    SFEnumCacheData *_pcache;
    IShellFolder *_psf;
    HDPA _hdpa;
};

SFEnumCache::~SFEnumCache()
{
    _FreePidlDPA();
    _penum->Release();
    _psf->Release();
}

void SFEnumCache_Terminate(SFEnumCacheData * pcache)
{
    // we can have a process detach before we put anything into the dsa
    // avoid the rip and don't call DSA_Destroy unless we really have one.
    if (pcache->hdsa)
    {
        // we don't free the id's because this is called on process detach
        // and the system process cleanup will take care of this for us
        DSA_Destroy(pcache->hdsa);
        pcache->hdsa = NULL;
    }
}

void SFEnumCache_Invalidate(SFEnumCacheData * pcache, DWORD grfFlags)
{
    ENTERCRITICAL;
    pcache->grfInvalid |= grfFlags;
    LEAVECRITICAL;
}

HRESULT SFEnumCache_Create(IEnumIDList* penum, DWORD grfFlags, SFEnumCacheData* pcache, IShellFolder* psf, IEnumIDList **ppenumUnknown)
{
    HRESULT hres = E_OUTOFMEMORY;
    SFEnumCache* pec = new SFEnumCache(penum, grfFlags, pcache, psf);
    
    if (pec) {
        *ppenumUnknown = pec;
        hres = S_OK;
    }
    return hres;
}

//SHCONTF_FOLDERS         = 32,       // for shell browser
//    SHCONTF_NONFOLDERS      = 64,       // for default view
//    SHCONTF_INCLUDEHIDDEN   = 128,      // for hidden/system objects

SFEnumCache::SFEnumCache(IEnumIDList* penum, DWORD grfFlags, SFEnumCacheData* pcache, IShellFolder* psf) :
    _cRef(1), _penum(penum), _grfFlags(grfFlags), _psf(psf)
{
    ASSERT(_penum);
    ASSERT(_psf);
    _penum->AddRef();
    _psf->AddRef();
    _pcache = pcache;
}

STDMETHODIMP_(ULONG) SFEnumCache::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP SFEnumCache::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IEnumIDList))
    {
        *ppv = SAFECAST(this, IEnumIDList*);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    ((IUnknown *)*ppv)->AddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG) SFEnumCache::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

BOOL SFEnumCache::_IsCacheValid()
{
    BOOL fValid;
    
    fValid = (_pcache->hdsa && !(_pcache->grfInvalid & _grfFlags));
    
    return fValid;
}

int SFEnumCache::_CacheFreeCallback(LPVOID p, LPVOID d)
{
    SFENUMCACHEELT* pce = (SFENUMCACHEELT*)p;
    ILEnumFree(pce->pidl);
    return 1;
}

STDAPI_(int) DPA_ILFreeCallback(LPVOID p, LPVOID d)
{
    ILFree((LPITEMIDLIST)p);    // ILFree checks for NULL pointer
    return 1;
}

STDAPI_(void) DPA_FreeIDArray(HDPA hdpa)
{
    if (hdpa)
        DPA_DestroyCallback(hdpa, DPA_ILFreeCallback, 0);
}

void SFEnumCache::_FreePidlDPA()
{
    DPA_FreeIDArray(_hdpa);
    _hdpa = NULL;
}

void SFEnumCache::_InitGlobalCache()
{
    if (!_IsCacheValid()) 
    {
        HDSA hdsa;
        LPITEMIDLIST pidl;
        ULONG celt;

        ASSERTNONCRITICAL;
        
        hdsa = DSA_Create(SIZEOF(SFENUMCACHEELT), 16);
        
        while ((_penum->Next(1, &pidl, &celt) == S_OK) &&
               (celt == 1))
        {
            SFENUMCACHEELT ce;
            DWORD dwAttr = SFGAO_GHOSTED | SFGAO_FOLDER;
            ce.grfFlags = 0;
            
            _psf->GetAttributesOf(1, (LPCITEMIDLIST*)&pidl, &dwAttr);
            
            if (dwAttr & SFGAO_GHOSTED)
                ce.grfFlags |= SHCONTF_INCLUDEHIDDEN;
            
            if (dwAttr & SFGAO_FOLDER)
                ce.grfFlags |= SHCONTF_FOLDERS;
            else
                ce.grfFlags |= SHCONTF_NONFOLDERS;

#ifdef WINNT
            ce.pidl = pidl;
#else
            ce.pidl = ILEnumClone(pidl);
            ILFree(pidl);
#endif
            DSA_AppendItem(hdsa, &ce);
        }
        
        ENTERCRITICAL; 

        // between the outside IsCacheValid and now, antoher thread could have
        // validated this cache.  we either take this hdsa and free the invalid one
        // or free this local hdsa.
        
        HDSA hdsaFree = hdsa;
        if(!_IsCacheValid()) {
            
            hdsaFree = _pcache->hdsa;
            _pcache->hdsa = hdsa;
            _pcache->grfInvalid = 0;
        }
        
        LEAVECRITICAL;
        
        if (hdsaFree)
            _FreeCache(hdsaFree);
    }
}

HDSA SFEnumCache::_GetGlobalCacheDSA()
{
    if (_IsCacheValid())
        return _pcache->hdsa;
   
    return NULL;
}

// this is a little strange... but it's done this way to avoid holding the
// critical section while doing the enumeration.

// 1) InitGlobalCache verifies that the cache is valid for this type of
//    enumeration
// 2) GetGlobalCacheDSA gets the dsa, and revalidates (but within the crit section)
//    that the dsa is valid for this enumeration.  if it's invalid, it returns NULL
// 3) if it's valid, we do a clone of just the IDs we need.  otherwise we
//    go back to 1
void SFEnumCache::_InitCache()
{
    if (_hdpa)  // this means we've initialized already
        return;
     
    HDSA hdsa;
    _hdpa = DPA_Create(16);
    
    do {
        
        // this enumerates the folder and stores it into the global dsa
        _InitGlobalCache();
    
        ENTERCRITICAL;
    
        hdsa = _GetGlobalCacheDSA();

        if (hdsa) {
            int i;

            for (i = 0; i < DSA_GetItemCount(_pcache->hdsa); i++) {

                SFENUMCACHEELT *pce = (SFENUMCACHEELT*)DSA_GetItemPtr(_pcache->hdsa, i);
                
                // make sure it's appropriately a folder or non-folder
                // then makes ure that either it's not hidden, or that we're allowing
                // hidden items
                if ((pce->grfFlags & (_grfFlags & ~SHCONTF_INCLUDEHIDDEN)) && 
                    ((!(pce->grfFlags & SHCONTF_INCLUDEHIDDEN)) || (_grfFlags & SHCONTF_INCLUDEHIDDEN))) {

                    // this is our type.  add it to our own dpa
                    LPITEMIDLIST pidl = ILClone(pce->pidl);
                    if (pidl) {
                        DPA_AppendPtr(_hdpa, pidl);
                    }
                }
            }
        }
        LEAVECRITICAL;
        
    } while (!hdsa);
}

// *** IEnumIDList methods ***
STDMETHODIMP SFEnumCache::Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched) 
{
    HRESULT hres = S_OK;
    UINT cFetched = 0;

    _InitCache();
    
    if (_hdpa) {

        for ( ; _iIndex < DPA_GetPtrCount(_hdpa) && cFetched < celt; _iIndex++, cFetched++)
        {
            rgelt[0] = ILClone((LPITEMIDLIST)DPA_GetPtr(_hdpa, _iIndex));
            if (!rgelt[0]) {
                hres = E_OUTOFMEMORY;
                break;
            }
            rgelt++;
        }
        
        if (SUCCEEDED(hres) && !cFetched){
            // no more elements
            hres = S_FALSE;
        }

    } else {
        hres = E_OUTOFMEMORY;
    }

    if (pceltFetched)
        *pceltFetched = cFetched;
    return hres;
}

STDMETHODIMP SFEnumCache::Skip( ULONG celt) 
{
    // REVIEW: Implement it later.
    return E_NOTIMPL;
}

STDMETHODIMP SFEnumCache::Reset() 
{
    _iIndex = 0;
    _FreePidlDPA();
    return S_OK;
}

STDMETHODIMP SFEnumCache::Clone( IEnumIDList **ppenum) 
{
    IEnumIDList* penum;
    HRESULT hres = _penum->Clone(&penum);
    if (SUCCEEDED(hres)) 
    {
        SFEnumCache* pec = new SFEnumCache(penum, _grfFlags, _pcache, _psf);
        
        if (pec) 
        {
            *ppenum = SAFECAST(pec, IEnumIDList*);
        } 
        else 
        {
            // we can't just return the penum on failed enum
            // because the penum is a full enumerator, not a grfFlags limited one
            hres = E_OUTOFMEMORY;
        }
        penum->Release();
    }
    return hres;
}

 
