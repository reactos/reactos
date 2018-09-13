/*
 * inetps.cpp - Property sheet implementation for Internet class.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop

#include "clsfact.h"
#include "inetcpl.h"
#include "inetps.hpp"


/***************************** Private Functions *****************************/


PRIVATE_CODE HRESULT AddInternetPS(LPFNADDPROPSHEETPAGE pfnAddPage,
                                   LPARAM lparam)
{
   // lparam may be any value.

   ASSERT(IS_VALID_CODE_PTR(pfnAddPage, LPFNADDPROPSHEETPAGE));

   return(AddInternetPropertySheets(pfnAddPage, lparam, NULL, NULL));
}


/****************************** Public Functions *****************************/


#ifdef DEBUG

PUBLIC_CODE BOOL IsValidPCInternet(PCInternet pcinet)
{
   return(IS_VALID_READ_PTR(pcinet, CInternet) &&
          IS_VALID_STRUCT_PTR((PCRefCount)pcinet, CRefCount) &&
          IS_VALID_INTERFACE_PTR((PCIShellExtInit)pcinet, IShellExtInit) &&
          IS_VALID_INTERFACE_PTR((PCIShellPropSheetExt)pcinet, IShellPropSheetExt));
}

#endif


/********************************** Methods **********************************/


#pragma warning(disable:4705) /* "statement has no effect" warning - cl bug, see KB Q98989 */

Internet::Internet()
{
   DebugEntry(Internet::Internet);

   // Don't validate this until after construction.

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternet));

   DebugExitVOID(Internet::Internet);

   return;
}

#pragma warning(default:4705) /* "statement has no effect" warning - cl bug, see KB Q98989 */


Internet::~Internet(void)
{
   DebugEntry(Internet::~Internet);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternet));

   DebugExitVOID(Internet::~Internet);

   return;
}


ULONG STDMETHODCALLTYPE Internet::AddRef(void)
{
   ULONG ulcRef;

   DebugEntry(Internet::AddRef);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternet));

   ulcRef = RefCount::AddRef();

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternet));

   DebugExitULONG(Internet::AddRef, ulcRef);

   return(ulcRef);
}


ULONG STDMETHODCALLTYPE Internet::Release(void)
{
   ULONG ulcRef;

   DebugEntry(Internet::Release);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternet));

   ulcRef = RefCount::Release();

   DebugExitULONG(Internet::Release, ulcRef);

   return(ulcRef);
}


HRESULT STDMETHODCALLTYPE Internet::QueryInterface(REFIID riid,
                                                   PVOID *ppvObject)
{
   HRESULT hr = S_OK;

   DebugEntry(Internet::QueryInterface);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternet));
   ASSERT(IsValidREFIID(riid));
   ASSERT(IS_VALID_WRITE_PTR(ppvObject, PVOID));

   if (riid == IID_IShellExtInit)
   {
      *ppvObject = (PIShellExtInit)this;
      TRACE_OUT(("Internet::QueryInterface(): Returning IShellExtInit."));
   }
   else if (riid == IID_IShellPropSheetExt)
   {
      *ppvObject = (PIShellPropSheetExt)this;
      TRACE_OUT(("Internet::QueryInterface(): Returning IShellPropSheetExt."));
   }
   else if (riid == IID_IUnknown)
   {
      *ppvObject = (PIUnknown)(PIShellPropSheetExt)this;
      TRACE_OUT(("Internet::QueryInterface(): Returning IUnknown."));
   }
   else
   {
      TRACE_OUT(("Internet::QueryInterface(): Called on unknown interface."));
      *ppvObject = NULL;
      hr = E_NOINTERFACE;
   }

   if (hr == S_OK)
      AddRef();

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternet));

   DebugExitHRESULT(Internet::QueryInterface, hr);

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE Internet::Initialize(PCITEMIDLIST pcidlFolder,
                                               PIDataObject pido,
                                               HKEY hkeyProgID)
{
   HRESULT hr;

   DebugEntry(Internet::Initialize);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternet));
   ASSERT(! pcidlFolder ||
          IS_VALID_STRUCT_PTR(pcidlFolder, CITEMIDLIST));
   ASSERT(IS_VALID_INTERFACE_PTR(pido, IDataObject));
   ASSERT(IS_VALID_HANDLE(hkeyProgID, KEY));

   // An Internet doesn't care where it lives in the name space.

   hr = S_OK;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternet));

   DebugExitHRESULT(Internet::Initialize, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


HRESULT STDMETHODCALLTYPE Internet::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage,
                                             LPARAM lparam)
{
   HRESULT hr;

   DebugEntry(Internet::AddPages);

   // lparam may be any value.

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternet));
   ASSERT(IS_VALID_CODE_PTR(pfnAddPage, LPFNADDPROPSHEETPAGE));

   hr = AddInternetPS(pfnAddPage, lparam);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternet));

   DebugExitHRESULT(Internet::AddPages, hr);

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE Internet::ReplacePage(
                                          UINT uPageID,
                                          LPFNADDPROPSHEETPAGE pfnReplaceWith,
                                          LPARAM lparam)
{
   HRESULT hr;

   DebugEntry(Internet::ReplacePage);

   // lparam may be any value.
   // uPageID may be any value.

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternet));
   ASSERT(IS_VALID_CODE_PTR(pfnReplaceWith, LPFNADDPROPSHEETPAGE));

   // No pages to replace.

   hr = E_NOTIMPL;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternet));

   DebugExitHRESULT(Internet::ReplacePage, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


extern "C"
STDAPI CreateInstance_Internet(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hres;

    *ppvOut = NULL;

    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    Internet *pinet = new(Internet);
    if (pinet) 
    {
        hres = pinet->QueryInterface(riid, ppvOut);
        pinet->Release();
    }
    else
        hres = E_OUTOFMEMORY;

    return hres;
}

