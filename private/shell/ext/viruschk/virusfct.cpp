#include "viruspch.h"
#include "virusmn.h"
#include "virusfct.h"
#include "vrsscan.h"
#include "viruschk.h"

CVirusFactory *pcf;

STDMETHODIMP CVirusFactory::QueryInterface(REFIID riid, void **ppv)
{
   if((riid == IID_IClassFactory) || (riid == IID_IUnknown))
   {
      cRef++;
      *ppv = (void *)this;
      return NOERROR;
   }

   *ppv = NULL;
   return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CVirusFactory::AddRef()
{
   return(++cRef);
}

STDMETHODIMP_(ULONG) CVirusFactory::Release()
{
   return(--cRef);
}

STDMETHODIMP CVirusFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
   CVirusCheck *vc = NULL;
   IUnknown *punk;
   HRESULT hr;

   vc = new CVirusCheck(pUnkOuter, &punk);
   if(!vc)
      return (E_OUTOFMEMORY);

   if(punk == NULL)
      return CLASS_E_NOAGGREGATION;

   hr = punk->QueryInterface(riid, ppv);
   if(FAILED(hr))
      delete vc;
   else
      DllAddRef();
   
   punk->Release();
   return hr;
}

STDMETHODIMP CVirusFactory::LockServer(BOOL fLock)
{
   if(fLock)
      DllAddRef();
   else
      DllRelease();
   return NOERROR;
}

