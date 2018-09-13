/*
 * ftps.cpp - File Types property sheet implementation for MIME types.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop

#include <mluisupp.h>

#include <ispriv.h>

#include "clsfact.h"
extern "C"
{
#include "filetype.h"
}
#include "ftps.hpp"
#include "resource.h"
#include "urlshell.h"

#if defined(WINNT) || defined(WINNT_ENV)
#include <shlguidp.h>
#else
#include <shlguid.h>
#endif


/****************************** Public Functions *****************************/


#ifdef DEBUG

PUBLIC_CODE BOOL IsValidPCMIMEHook(PCMIMEHook pcmimehk)
{
   return(IS_VALID_READ_PTR(pcmimehk, CMIMEHook) &&
          IS_VALID_STRUCT_PTR((PCRefCount)pcmimehk, CRefCount) &&
          IS_VALID_INTERFACE_PTR((PCIShellExtInit)pcmimehk, IShellExtInit) &&
          IS_VALID_INTERFACE_PTR((PCIShellPropSheetExt)pcmimehk, IShellPropSheetExt));
}

#endif


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

PRIVATE_CODE UINT CALLBACK MIMEFileTypesPSCallback(HWND hwnd, UINT uMsg,
                                                   LPPROPSHEETPAGE ppsp)
{
   UINT uResult = TRUE;

   // uMsg may be any value.

   ASSERT(! hwnd ||
          IS_VALID_HANDLE(hwnd, WND));
   ASSERT(IS_VALID_STRUCT_PTR(ppsp, CPROPSHEETPAGE));

   switch (uMsg)
   {
      case PSPCB_CREATE:
         TRACE_OUT(("MIMEFileTypesPSCallback(): Received PSPCB_CREATE."));
         break;

      case PSPCB_RELEASE:
         TRACE_OUT(("MIMEFileTypesPSCallback(): Received PSPCB_RELEASE."));
         delete (PFILETYPESDIALOGINFO)(ppsp->lParam);
         ppsp->lParam = NULL;
         break;

      default:
         TRACE_OUT(("MIMEFileTypesPSCallback(): Unhandled message %u.",
                    uMsg));
         break;
   }

   return(uResult);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


/********************************** Methods **********************************/


MIMEHook::MIMEHook(void)
{
   DebugEntry(MIMEHook::MIMEHook);

   // Don't validate this until after construction.

   ASSERT(IS_VALID_STRUCT_PTR(this, CMIMEHook));

   DebugExitVOID(MIMEHook::MIMEHook);

   return;
}


MIMEHook::~MIMEHook(void)
{
   DebugEntry(MIMEHook::~MIMEHook);

   ASSERT(IS_VALID_STRUCT_PTR(this, CMIMEHook));

   DebugExitVOID(MIMEHook::~MIMEHook);

   return;
}


ULONG STDMETHODCALLTYPE MIMEHook::AddRef(void)
{
   ULONG ulcRef;

   DebugEntry(MIMEHook::AddRef);

   ASSERT(IS_VALID_STRUCT_PTR(this, CMIMEHook));

   ulcRef = RefCount::AddRef();

   ASSERT(IS_VALID_STRUCT_PTR(this, CMIMEHook));

   DebugExitULONG(MIMEHook::AddRef, ulcRef);

   return(ulcRef);
}


ULONG STDMETHODCALLTYPE MIMEHook::Release(void)
{
   ULONG ulcRef;

   DebugEntry(MIMEHook::Release);

   ASSERT(IS_VALID_STRUCT_PTR(this, CMIMEHook));

   ulcRef = RefCount::Release();

   DebugExitULONG(MIMEHook::Release, ulcRef);

   return(ulcRef);
}


HRESULT STDMETHODCALLTYPE MIMEHook::QueryInterface(REFIID riid,
                                                   PVOID *ppvObject)
{
   HRESULT hr = S_OK;

   DebugEntry(MIMEHook::QueryInterface);

   ASSERT(IS_VALID_STRUCT_PTR(this, CMIMEHook));
   ASSERT(IsValidREFIID(riid));
   ASSERT(IS_VALID_WRITE_PTR(ppvObject, PVOID));

   if (riid == IID_IShellPropSheetExt)
   {
      *ppvObject = (PIShellPropSheetExt)this;
      TRACE_OUT(("MIMEHook::QueryInterface(): Returning IShellPropSheetExt."));
   }
   else if (riid == IID_IShellExtInit)
   {
      *ppvObject = (PIShellExtInit)this;
      TRACE_OUT(("MIMEHook::QueryInterface(): Returning IShellExtInit."));
   }
   else if (riid == IID_IUnknown)
   {
      *ppvObject = (PIUnknown)(PIShellPropSheetExt)this;
      TRACE_OUT(("MIMEHook::QueryInterface(): Returning IUnknown."));
   }
   else
   {
      TRACE_OUT(("MIMEHook::QueryInterface(): Called on unknown interface."));
      *ppvObject = NULL;
      hr = E_NOINTERFACE;
   }

   if (hr == S_OK)
      AddRef();

   ASSERT(IS_VALID_STRUCT_PTR(this, CMIMEHook));

   DebugExitHRESULT(MIMEHook::QueryInterface, hr);

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE MIMEHook::Initialize(LPCITEMIDLIST pidlFolder, 
                                               IDataObject *pdtobj, 
                                               HKEY hkeyProgID)
{
   HRESULT hr;

   DebugEntry(MIMEHook::Initialize);

   ASSERT(IS_VALID_STRUCT_PTR(this, CMIMEHook));

   hr = S_OK;

   ASSERT(IS_VALID_STRUCT_PTR(this, CMIMEHook));

   DebugExitHRESULT(MIMEHook::Initialize, hr);

   return(hr);
}

#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE MIMEHook::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage,
                                             LPARAM lparam)
{
   HRESULT hr;

   DebugEntry(MIMEHook::AddPages);

   // lparam may be any value.

   ASSERT(IS_VALID_STRUCT_PTR(this, CMIMEHook));
   ASSERT(IS_VALID_CODE_PTR(pfnAddPage, LPFNADDPROPSHEETPAGE));

   hr = E_FAIL;

   ASSERT(IS_VALID_STRUCT_PTR(this, CMIMEHook));

   DebugExitHRESULT(MIMEHook::AddPages, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


HRESULT STDMETHODCALLTYPE MIMEHook::ReplacePage(
                                          UINT uPageID,
                                          LPFNADDPROPSHEETPAGE pfnReplaceWith,
                                          LPARAM lparam)
{
   HRESULT hr;

   DebugEntry(MIMEHook::ReplacePage);

   // lparam may be any value.
   // uPageID is checked below.

   ASSERT(IS_VALID_STRUCT_PTR(this, CMIMEHook));
   ASSERT(IS_VALID_CODE_PTR(pfnReplaceWith, LPFNADDPROPSHEETPAGE));

   if (EVAL(uPageID == EXPPS_FILETYPES))
   {
      TRACE_OUT(("MIMEHook::ReplacePage(): Replacing File Types property sheet."));

      hr = AddMIMEFileTypesPS(pfnReplaceWith, lparam);
   }
   else
   {
      TRACE_OUT(("MIMEHook::ReplacePage(): Not replacing unknown property sheet %u.",
                 uPageID));

      hr = E_FAIL;
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CMIMEHook));

   DebugExitHRESULT(MIMEHook::ReplacePage, hr);

   return(hr);
}


/***************************** Exported Functions ****************************/


INTSHCUTPRIVAPI HRESULT WINAPI AddMIMEFileTypesPS(LPFNADDPROPSHEETPAGE pfnAddPage,
                                                  LPARAM lparam)
{
   HRESULT hr = E_OUTOFMEMORY;
   PFILETYPESDIALOGINFO pFTDInfo;
   IShellPropSheetExt* pspse;

   // Try to bind to shell32 to gain the file types box, if this works then we
   // don't need to give them our ANSI implementation.  

   hr = SHCoCreateInstance(NULL, &CLSID_FileTypes, NULL, IID_IShellPropSheetExt, (LPVOID*)&pspse);

   if (SUCCEEDED(hr))
   {
        hr = pspse->AddPages(pfnAddPage, lparam);
        pspse->Release();
   }
   else
   {
       // lparam may be any value.

       ASSERT(IS_VALID_CODE_PTR(pfnAddPage, LPFNADDPROPSHEETPAGE));

       pFTDInfo = new(FILETYPESDIALOGINFO);

       if (pFTDInfo)
       {
          PROPSHEETPAGE psp;
          HPROPSHEETPAGE hpsp;

          ZeroMemory(pFTDInfo, sizeof(*pFTDInfo));

          psp.dwSize = sizeof(psp);
          psp.dwFlags = (PSP_DEFAULT | PSP_USECALLBACK | PSP_USEREFPARENT);
          psp.hInstance = MLGetHinst();
          psp.pszTemplate = MAKEINTRESOURCE(DLG_FILETYPEOPTIONS);
          psp.hIcon = NULL;
          psp.pszTitle = NULL;
          psp.pfnDlgProc = &FT_DlgProc;
          psp.lParam = (LPARAM)pFTDInfo;
          psp.pfnCallback = &MIMEFileTypesPSCallback;
          psp.pcRefParent = (PUINT)GetDLLRefCountPtr();

          ASSERT(IS_VALID_STRUCT_PTR(&psp, CPROPSHEETPAGE));

          hpsp = CreatePropertySheetPage(&psp);

          if (hpsp)
          {
             if ((*pfnAddPage)(hpsp, lparam))
             {
                hr = S_OK;
                TRACE_OUT(("AddMIMEFileTypesPS(): Added MIME File Types property sheet."));
             }
             else
             {
                DestroyPropertySheetPage(hpsp);

                hr = E_FAIL;
                WARNING_OUT(("AddMIMEFileTypesPS(): Callback to add property sheet failed."));
             }
          }

          if (hr != S_OK)
          {
             LocalFree(pFTDInfo);
             pFTDInfo = NULL;
          }
       }
   }

   return(hr);
}


extern "C"
STDAPI CreateInstance_MIMEHook(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hres;

    *ppvOut = NULL;

    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    MIMEHook *phook = new(MIMEHook);
    if (phook) 
    {
        hres = phook->QueryInterface(riid, ppvOut);
        phook->Release();
    }
    else
        hres = E_OUTOFMEMORY;

    return hres;
}

