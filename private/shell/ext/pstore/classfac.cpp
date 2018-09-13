
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>

#include <docobj.h>
#include "shfolder.h"

#include "classfac.h"

extern LONG       	g_DllRefCount;



CClassFactory::CClassFactory()
{
    m_ObjRefCount = 1;
    InterlockedIncrement(&g_DllRefCount);
}


CClassFactory::~CClassFactory()
{
    InterlockedDecrement(&g_DllRefCount);
}


STDMETHODIMP
CClassFactory::QueryInterface(
    REFIID riid,
    LPVOID *ppReturn
    )
{
    *ppReturn = NULL;

    if(IsEqualIID(riid, IID_IUnknown))
        *ppReturn = (IUnknown*)(CClassFactory*)this;
    else if(IsEqualIID(riid, IID_IClassFactory))
        *ppReturn = (CClassFactory*)this;

	if(*ppReturn == NULL)
		return E_NOINTERFACE;

    (*(LPUNKNOWN*)ppReturn)->AddRef();
    return S_OK;
}                                             


STDMETHODIMP_(DWORD)
CClassFactory::AddRef()
{
    return InterlockedIncrement(&m_ObjRefCount);
}


STDMETHODIMP_(DWORD)
CClassFactory::Release()
{
	LONG lDecremented = InterlockedDecrement(&m_ObjRefCount);
    
    if(lDecremented == 0)
       delete this;

    return lDecremented;
}


STDMETHODIMP
CClassFactory::CreateInstance(
    LPUNKNOWN pUnknown,
    REFIID riid,
    LPVOID *ppObject
    )
{
    if(pUnknown != NULL)
        return CLASS_E_NOAGGREGATION;


    CShellFolder *pShellFolder = new CShellFolder(NULL, NULL);
    if(NULL == pShellFolder)
        return E_OUTOFMEMORY;

    //
    // get the QueryInterface return for our return value
    //

    HRESULT hResult = pShellFolder->QueryInterface(riid, ppObject);

    //
    // call Release to decrement the ref count
    //

    pShellFolder->Release();

    return hResult;
}

STDMETHODIMP
CClassFactory::LockServer(
    BOOL fLock
    )
{
    return E_NOTIMPL;
}

