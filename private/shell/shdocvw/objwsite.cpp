#include "priv.h"

#include "objwsite.h"

//
// Default IObjectWithSite
//


CObjWithSite::CObjWithSite()
{
    _punkSite = NULL;
}

CObjWithSite::~CObjWithSite()
{
    if (_punkSite) 
        _punkSite->Release();
}

HRESULT CObjWithSite::SetSite(IUnknown *punkSite)
{
    ATOMICRELEASE(_punkSite);

    ASSERT(_punkSite == NULL);  // don't lose a reference to this

    _punkSite = punkSite;

    if (_punkSite)
        _punkSite->AddRef();

    return S_OK;
}

HRESULT CObjWithSite::GetSite(REFIID riid, void **ppvSite)
{
    if (_punkSite) 
        return _punkSite->QueryInterface(riid, ppvSite);

    *ppvSite = NULL;
    return E_FAIL;
}
