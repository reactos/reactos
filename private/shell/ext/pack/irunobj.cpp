#include "priv.h"
#include "privcpp.h"

CPackage_IRunnableObject::CPackage_IRunnableObject(CPackage *pPackage) : 
    _pPackage(pPackage)
{
    ASSERT(_cRef == 0);
}

CPackage_IRunnableObject::~CPackage_IRunnableObject()
{
    DebugMsg(DM_TRACE,"CPackage_IRunnableObject destroyed with ref count %d",_cRef);
}


//////////////////////////////////
//
// IUnknown Methods...
//
HRESULT CPackage_IRunnableObject::QueryInterface(REFIID iid, void ** ppv)
{
    return _pPackage->QueryInterface(iid,ppv);
}

ULONG CPackage_IRunnableObject::AddRef(void) 
{
    _cRef++;    // interface ref count for debugging
    return _pPackage->AddRef();
}

ULONG CPackage_IRunnableObject::Release(void)
{
    _cRef--;    // interface ref count for debugging
    return _pPackage->Release();
}

//////////////////////////////////
//
// IRunnable Object Methods...
//
// NOTE: To answer your question, yes, this is kind of a pointless interface,
// but some apps feel better when it's around.  Basically we just tell what
// they want to hear (ie. return S_OK).
//
HRESULT CPackage_IRunnableObject::GetRunningClass(LPCLSID pclsid)
{
    DebugMsg(DM_TRACE, "pack ro - GetRunningClass() called.");
    
    if (pclsid == NULL)
        return E_INVALIDARG;
    
    *pclsid = CLSID_CPackage;
    return S_OK;
}

HRESULT CPackage_IRunnableObject::Run(LPBC lpbc)
{
    // we're an inproc-server, so telling us to run is kind of pointless
    DebugMsg(DM_TRACE, "pack ro - Run() called.");
    return S_OK;
}

BOOL CPackage_IRunnableObject::IsRunning() 
{
    DebugMsg(DM_TRACE, "pack ro - IsRunning() called.");
    
    // we're an inproc-server, so this is kind of pointless
    return TRUE;
} 

HRESULT CPackage_IRunnableObject::LockRunning(BOOL, BOOL)
{
    DebugMsg(DM_TRACE, "pack ro - LockRunning() called.");
    
    // again, we're an inproc-server, so this is also pointless
    return S_OK;
} 

HRESULT CPackage_IRunnableObject::SetContainedObject(BOOL)
{
    DebugMsg(DM_TRACE, "pack ro - SetContainedObject() called.");
    // again, we don't really care about this
    return S_OK;
} 

