#include "provpch.h"
#include "vrsscan.h"
#include "util.h"
#include "vrsprov.h"
#include "provmn.h"


// Easy constructor. If you have a lot of static data, consider load on demand as well as keeping
//  one copy of global structures. 
//
// You will likely be asked to create objects on several threads, minimize overhead by sharing global data 

CVirusProvider1::CVirusProvider1(IUnknown *punkOut, IUnknown **punkRet) 
{
   cObjRef = 0;
   
   *punkRet = NULL;
   
   if(punkOut == NULL)
      *punkRet = (IVirusScanEngine *)this;

   ((IUnknown *)*punkRet)->AddRef();
}

//QueryInterface

STDMETHODIMP CVirusProvider1::QueryInterface(REFIID riid, void **ppv)
{
   *ppv = NULL;

   //IUnknown
   if((riid == IID_IUnknown) || (riid == IID_IVirusScanEngine))
      *ppv = (IVirusScanEngine *)this;

   // Sorry...
   if(*ppv == NULL)
      return ResultFromScode(E_NOINTERFACE);

   ((IUnknown *) *ppv)->AddRef();
   return NOERROR; 
}

// AddRef - pretty boring

STDMETHODIMP_(ULONG) CVirusProvider1::AddRef(void)
{
   cObjRef++;
   return cObjRef;
}

// Release - pretty boring as is
// a provider may want to do delayed unloading here

STDMETHODIMP_(ULONG) CVirusProvider1::Release(void)
{
   cObjRef--;
   if(cObjRef != 0)
      return cObjRef;

   delete this;
   return 0;
}

// Silly, trivial implementation of scan for virus.
// Probably call off into existing entry point that does some work.

STDMETHODIMP CVirusProvider1::ScanForVirus(STGMEDIUM *pstgMedium, DWORD dwFlags, LPVIRUSINFO pvrsinfo)
{
   if(MessageBox(NULL, TEXT("Do you want to report a virus?"), TEXT("Provider 1"), MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDYES)
   {
       if(pvrsinfo != NULL)
       {
          CopyWideStr((pvrsinfo)->wszVirusName, L"Ebola Virus");
          CopyWideStr((pvrsinfo)->wszVirusDescription, L"A nasty little virus");
       }
       return S_FALSE;
   }
   else
      return S_OK;
}




