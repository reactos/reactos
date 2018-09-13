/*
 * intshcut.cpp - IUnknown implementation for InternetShortcut class.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop

#include "assoc.h"
#include "clsfact.h"


/* Global Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

PUBLIC_DATA const int g_nDefaultShowCmd  = SW_NORMAL;

#pragma data_seg()


/****************************** Public Functions *****************************/


PUBLIC_CODE HRESULT IsProtocolRegistered(PCSTR pcszProtocol)
{
   HRESULT hr;
   PSTR pszKey;

   ASSERT(IS_VALID_STRING_PTR(pcszProtocol, CSTR));

   hr = GetProtocolKey(pcszProtocol, EMPTY_STRING, &pszKey);

   if (hr == S_OK)
   {
      hr = (GetRegKeyValue(g_hkeyURLProtocols, pszKey, g_cszURLProtocol, NULL,
                           NULL, NULL) == ERROR_SUCCESS)
           ? S_OK
           : URL_E_UNREGISTERED_PROTOCOL;

      delete pszKey;
      pszKey = NULL;
   }

   if (hr != S_OK) {
      TRACE_OUT(("IsProtocolRegistered(): Protocol \"%s\" is not registered.",
                 pcszProtocol));
   }

   return(hr);
}


PUBLIC_CODE HRESULT ValidateURL(PCSTR pcszURL)
{
   HRESULT hr;
   PSTR pszProtocol;

   ASSERT(IS_VALID_STRING_PTR(pcszURL, CSTR));

   hr = CopyURLProtocol(pcszURL, &pszProtocol);

   if (hr == S_OK)
   {
      hr = IsProtocolRegistered(pszProtocol);

      delete pszProtocol;
      pszProtocol = NULL;
   }

   return(hr);
}


PUBLIC_CODE HRESULT ValidateWorkingDirectory(PCSTR pcszWorkingDirectory)
{
   ASSERT(IS_VALID_STRING_PTR(pcszWorkingDirectory, CSTR));

   return(IsPathDirectory(pcszWorkingDirectory) ? S_OK : E_PATH_NOT_FOUND);
}


#ifdef DEBUG

PUBLIC_CODE BOOL IsValidPCInternetShortcut(PCInternetShortcut pcintshcut)
{
   return(IS_VALID_READ_PTR(pcintshcut, CInternetShortcut) &&
          FLAGS_ARE_VALID(pcintshcut->m_dwFlags, ALL_INTSHCUT_FLAGS) &&
          (! pcintshcut->m_pszFile ||
           IS_VALID_STRING_PTR(pcintshcut->m_pszFile, STR)) &&
          (! pcintshcut->m_pszURL ||
           IS_VALID_STRING_PTR(pcintshcut->m_pszURL, STR)) &&
          ((! pcintshcut->m_pszIconFile &&
            ! pcintshcut->m_niIcon) ||
           EVAL(IsValidIconIndex(S_OK, pcintshcut->m_pszIconFile, MAX_PATH_LEN, pcintshcut->m_niIcon))) &&
          (! pcintshcut->m_pszWorkingDirectory ||
           EVAL(IsFullPath(pcintshcut->m_pszWorkingDirectory))) &&
          EVAL(IsValidShowCmd(pcintshcut->m_nShowCmd)) &&
          EVAL(! pcintshcut->m_pszFolder ||
               IsValidPath(pcintshcut->m_pszFolder)) &&
          EVAL(! pcintshcut->m_wHotkey ||
               IsValidHotkey(pcintshcut->m_wHotkey)) &&
          IS_VALID_STRUCT_PTR((PCRefCount)pcintshcut, CRefCount) &&
          IS_VALID_INTERFACE_PTR((PCIDataObject)pcintshcut, IDataObject) &&
          IS_VALID_INTERFACE_PTR((PCIExtractIcon)pcintshcut, IExtractIcon) &&
          IS_VALID_INTERFACE_PTR((PCINewShortcutHook)pcintshcut, INewShortcutHook) &&
          IS_VALID_INTERFACE_PTR((PCIPersistFile)pcintshcut, IPersistFile) &&
          IS_VALID_INTERFACE_PTR((PCIPersistStream)pcintshcut, IPersistStream) &&
          IS_VALID_INTERFACE_PTR((PCIShellExecuteHook)pcintshcut, IShellExecuteHook) &&
          IS_VALID_INTERFACE_PTR((PCIShellExtInit)pcintshcut, IShellExtInit) &&
          IS_VALID_INTERFACE_PTR((PCIShellLink)pcintshcut, IShellLink) &&
          IS_VALID_INTERFACE_PTR((PCIShellPropSheetExt)pcintshcut, IShellPropSheetExt) &&
          IS_VALID_INTERFACE_PTR((PCIUniformResourceLocator)pcintshcut, IUniformResourceLocator));
}

#endif


/********************************** Methods **********************************/


#pragma warning(disable:4705) /* "statement has no effect" warning - cl bug, see KB Q98989 */

InternetShortcut::InternetShortcut()
{
   DebugEntry(InternetShortcut::InternetShortcut);

   // Don't validate this until after construction.

   m_dwFlags = 0;
   m_pszFile = NULL;
   m_pszURL = NULL;
   m_pszIconFile = NULL;
   m_niIcon = 0;
   m_pszWorkingDirectory = NULL;
   m_nShowCmd = g_nDefaultShowCmd;
   m_pszFolder = NULL;
   m_wHotkey = 0;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitVOID(InternetShortcut::InternetShortcut);

   return;
}

#pragma warning(default:4705) /* "statement has no effect" warning - cl bug, see KB Q98989 */


InternetShortcut::~InternetShortcut(void)
{
   DebugEntry(InternetShortcut::~InternetShortcut);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   if (m_pszFile)
   {
      delete m_pszFile;
      m_pszFile = NULL;
   }

   if (m_pszURL)
   {
      delete m_pszURL;
      m_pszURL = NULL;
   }

   if (m_pszIconFile)
   {
      delete m_pszIconFile;
      m_pszIconFile = NULL;
      m_niIcon = 0;
   }

   if (m_pszWorkingDirectory)
   {
      delete m_pszWorkingDirectory;
      m_pszWorkingDirectory = NULL;
   }

   if (m_pszFolder)
   {
      delete m_pszFolder;
      m_pszFolder = NULL;
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitVOID(InternetShortcut::~InternetShortcut);

   return;
}


ULONG STDMETHODCALLTYPE InternetShortcut::AddRef(void)
{
   ULONG ulcRef;

   DebugEntry(InternetShortcut::AddRef);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   ulcRef = RefCount::AddRef();

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitULONG(InternetShortcut::AddRef, ulcRef);

   return(ulcRef);
}


ULONG STDMETHODCALLTYPE InternetShortcut::Release(void)
{
   ULONG ulcRef;

   DebugEntry(InternetShortcut::Release);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   ulcRef = RefCount::Release();

   DebugExitULONG(InternetShortcut::Release, ulcRef);

   return(ulcRef);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::QueryInterface(REFIID riid,
                                                           PVOID *ppvObject)
{
   HRESULT hr = S_OK;

   DebugEntry(InternetShortcut::QueryInterface);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IsValidREFIID(riid));
   ASSERT(IS_VALID_WRITE_PTR(ppvObject, PVOID));

   if (riid == IID_IDataObject)
   {
      *ppvObject = (PIDataObject)this;
      TRACE_OUT(("InternetShortcut::QueryInterface(): Returning IDataObject."));
   }
   else if (riid == IID_IExtractIcon)
   {
      *ppvObject = (PIExtractIcon)this;
      TRACE_OUT(("InternetShortcut::QueryInterface(): Returning IExtractIcon."));
   }
   else if (riid == IID_INewShortcutHook)
   {
      *ppvObject = (PINewShortcutHook)this;
      TRACE_OUT(("InternetShortcut::QueryInterface(): Returning INewShortcutHook."));
   }
   else if (riid == IID_IPersist)
   {
      *ppvObject = (PIPersist)(PIPersistStream)this;
      TRACE_OUT(("InternetShortcut::QueryInterface(): Returning IPersist."));
   }
   else if (riid == IID_IPersistFile)
   {
      *ppvObject = (PIPersistFile)this;
      TRACE_OUT(("InternetShortcut::QueryInterface(): Returning IPersistFile."));
   }
   else if (riid == IID_IPersistStream)
   {
      *ppvObject = (PIPersistStream)this;
      TRACE_OUT(("InternetShortcut::QueryInterface(): Returning IPersistStream."));
   }
   else if (riid == IID_IShellExecuteHook)
   {
      *ppvObject = (PIShellExecuteHook)this;
      TRACE_OUT(("InternetShortcut::QueryInterface(): Returning IShellExecuteHook."));
   }
   else if (riid == IID_IShellExtInit)
   {
      *ppvObject = (PIShellExtInit)this;
      TRACE_OUT(("InternetShortcut::QueryInterface(): Returning IShellExtInit."));
   }
   else if (riid == IID_IShellLink)
   {
      *ppvObject = (PIShellLink)this;
      TRACE_OUT(("InternetShortcut::QueryInterface(): Returning IShellLink."));
   }
   else if (riid == IID_IShellPropSheetExt)
   {
      *ppvObject = (PIShellPropSheetExt)this;
      TRACE_OUT(("InternetShortcut::QueryInterface(): Returning IShellPropSheetExt."));
   }
   else if (riid == IID_IUniformResourceLocator)
   {
      *ppvObject = (PIUniformResourceLocator)this;
      TRACE_OUT(("InternetShortcut::QueryInterface(): Returning IUniformResourceLocator."));
   }
   else if (riid == IID_IUnknown)
   {
      *ppvObject = (PIUnknown)(PIUniformResourceLocator)this;
      TRACE_OUT(("InternetShortcut::QueryInterface(): Returning IUnknown."));
   }
   else
   {
      TRACE_OUT(("InternetShortcut::QueryInterface(): Called on unknown interface."));
      *ppvObject = NULL;
      hr = E_NOINTERFACE;
   }

   if (hr == S_OK)
      AddRef();

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::QueryInterface, hr);

   return(hr);
}


extern "C"
STDAPI CreateInstance_Intshcut(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hres;

    *ppvOut = NULL;

    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    InternetShortcut *pintshcut = new(InternetShortcut);
    if (pintshcut) 
    {
        hres = pintshcut->QueryInterface(riid, ppvOut);
        pintshcut->Release();
    }
    else
        hres = E_OUTOFMEMORY;

    return hres;
}

