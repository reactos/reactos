#include "priv.h"
#include "cowsite.h"

#if 0
// no default implementation for now.
// so far all clients do way more than this (e.g. deferred initialization)
// in their SetSite's.
HRESULT CObjectWithSite::SetSite(IUnknown punkSite)
{
    IUnknown_Set(&_punkSite, punkSite);
    return S_OK;
}
#endif

//***
// NOTES
//  iedisp.c!CIEFrameAutoProp::_SetValue calls us
HRESULT CObjectWithSite::GetSite(REFIID riid, void **ppvSite)
{
    // e.g. iedisp.c!CIEFrameAutoProp::_SetValue calls us
    if (_punkSite)
        return _punkSite->QueryInterface(riid, ppvSite);

    *ppvSite = NULL;
    return E_FAIL;

#if 0 // here 'tis if we ever decide we need it...
    // e.g. iedisp.c!CIEFrameAutoProp::_SetValue calls us
    TraceMsg(DM_WARNING, "cows.gs: E_NOTIMPL");
    *ppvSite = NULL;
    return E_NOTIMPL;
#endif
}
