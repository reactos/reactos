#include "provpch.h"
#include "provmn.h"
#include "provfct.h"
#include "vrsscan.h"
#include "vrsprov.h"

CProvider1Factory *pcf;

// *********************************************************************************************
//
// QueryInterface - very boring
//
//**********************************************************************************************

STDMETHODIMP CProvider1Factory::QueryInterface(REFIID riid, void **ppv)
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

// *********************************************************************************************
//
// AddRef - very boring
//
//**********************************************************************************************

STDMETHODIMP_(ULONG) CProvider1Factory::AddRef()
{
   return(++cRef);
}

// *********************************************************************************************
//
// Release - Totally boring
//
//**********************************************************************************************


STDMETHODIMP_(ULONG) CProvider1Factory::Release()
{
   return(--cRef);
}

//**********************************************************************************************
//
// CreateInstance - creates a object of CVirusProvider1 type
//
//**********************************************************************************************

STDMETHODIMP CProvider1Factory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
   CVirusProvider1 *pvp = NULL;
   IUnknown *punk;
   HRESULT hr;

   if(ppv == NULL)
      return E_INVALIDARG;
   else
      *ppv = NULL;

   pvp = new CVirusProvider1(pUnkOuter, &punk);
   if(!pvp)
      return (E_OUTOFMEMORY);

   // this object is not interested in aggregation
   if(punk == NULL)
      return CLASS_E_NOAGGREGATION;

   hr = punk->QueryInterface(riid, ppv);
   if(FAILED(hr))
      delete pvp;
   else
      DllAddRef();
   
   punk->Release();
   return hr;
}

//**********************************************************************************************
//
// LockServer - fine
//
//**********************************************************************************************

STDMETHODIMP CProvider1Factory::LockServer(BOOL fLock)
{
   if(fLock)
      DllAddRef();
   else
      DllRelease();
   return NOERROR;
}

