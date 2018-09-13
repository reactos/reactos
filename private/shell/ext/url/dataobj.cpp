/*
 * dataobj.cpp - IDataObject implementation for URL class.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop

#include "enumfmte.hpp"


/* Global Variables
 *******************/

/* registered clipboard formats */

PUBLIC_DATA UINT g_cfURL = 0;
PUBLIC_DATA UINT g_cfFileGroupDescriptor = 0;
PUBLIC_DATA UINT g_cfFileContents = 0;


/* Module Variables
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

PRIVATE_DATA char s_szURLCF[]                   = "UniformResourceLocator";
PRIVATE_DATA char s_szFileGroupDescriptorCF[]   = CFSTR_FILEDESCRIPTOR;
PRIVATE_DATA char s_szFileContentsCF[]          = CFSTR_FILECONTENTS;

#pragma data_seg()


/***************************** Private Functions *****************************/


BOOL RegisterClipboardFormats(void)
{
   g_cfURL                 = RegisterClipboardFormat(s_szURLCF);
   g_cfFileGroupDescriptor = RegisterClipboardFormat(s_szFileGroupDescriptorCF);
   g_cfFileContents        = RegisterClipboardFormat(s_szFileContentsCF);

   return(g_cfURL &&
          g_cfFileGroupDescriptor &&
          g_cfFileContents);
}


/****************************** Public Functions *****************************/

PUBLIC_CODE void ExitDataObjectModule(void)
{
   return;
}


/********************************** Methods **********************************/


HRESULT STDMETHODCALLTYPE InternetShortcut::GetData(PFORMATETC pfmtetc,
                                                    PSTGMEDIUM pstgmed)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetData);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRUCT_PTR(pfmtetc, CFORMATETC));
   ASSERT(IS_VALID_WRITE_PTR(pstgmed, STGMEDIUM));

   // Ignore pfmtetc.ptd.  All supported data formats are device-independent.

   ZeroMemory(pstgmed, sizeof(*pstgmed));

   if (pfmtetc->dwAspect == DVASPECT_CONTENT)
   {
      if (pfmtetc->cfFormat == g_cfURL)
         hr = (pfmtetc->lindex == -1) ? TransferUniformResourceLocator(pfmtetc,
                                                                       pstgmed)
                                      : DV_E_LINDEX;
      else if (pfmtetc->cfFormat == CF_TEXT)
         hr = (pfmtetc->lindex == -1) ? TransferText(pfmtetc, pstgmed)
                                      : DV_E_LINDEX;
      else if (pfmtetc->cfFormat == g_cfFileGroupDescriptor)
         hr = (pfmtetc->lindex == -1) ? TransferFileGroupDescriptor(pfmtetc,
                                                                    pstgmed)
                                      : DV_E_LINDEX;
      else if (pfmtetc->cfFormat == g_cfFileContents)
         hr = (! pfmtetc->lindex) ? TransferFileContents(pfmtetc, pstgmed)
                                  : DV_E_LINDEX;
      else
         hr = DV_E_FORMATETC;
   }
   else
      hr = DV_E_DVASPECT;

   if (hr == S_OK)
      TRACE_OUT(("InternetShortcut::GetData(): Returning clipboard format %s.",
                 GetClipboardFormatNameString(pfmtetc->cfFormat)));
   else
      TRACE_OUT(("InternetShortcut::GetData(): Failed to return clipboard format %s.",
                 GetClipboardFormatNameString(pfmtetc->cfFormat)));

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(FAILED(hr) ||
          IS_VALID_STRUCT_PTR(pstgmed, CSTGMEDIUM));

   DebugExitHRESULT(InternetShortcut::GetData, hr);

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE InternetShortcut::GetDataHere(PFORMATETC pfmtetc,
                                                        PSTGMEDIUM pstgpmed)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetDataHere);
   ASSERT(IS_VALID_STRUCT_PTR(pfmtetc, CFORMATETC));
   ASSERT(IS_VALID_STRUCT_PTR(pstgpmed, CSTGMEDIUM));

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   TRACE_OUT(("InternetShortcut::GetDataHere(): Failed to return clipboard format %s.",
              GetClipboardFormatNameString(pfmtetc->cfFormat)));

   hr = DV_E_FORMATETC;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRUCT_PTR(pstgpmed, CSTGMEDIUM));

   DebugExitHRESULT(InternetShortcut::GetDataHere, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


HRESULT STDMETHODCALLTYPE InternetShortcut::QueryGetData(PFORMATETC pfmtetc)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::QueryGetData);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRUCT_PTR(pfmtetc, CFORMATETC));

   TRACE_OUT(("InternetShortcut::QueryGetData(): Asked for clipboard format %s.",
              GetClipboardFormatNameString(pfmtetc->cfFormat)));

   // Ignore pfmtetc.ptd.  All supported data formats are device-independent.

   if (pfmtetc->dwAspect == DVASPECT_CONTENT)
   {
      if (IS_FLAG_SET(pfmtetc->tymed, TYMED_HGLOBAL))
      {
         if (pfmtetc->cfFormat == g_cfURL)
            hr = (pfmtetc->lindex == -1) ? S_OK : DV_E_LINDEX;
         else if (pfmtetc->cfFormat == CF_TEXT)
            hr = (pfmtetc->lindex == -1) ? S_OK : DV_E_LINDEX;
         else if (pfmtetc->cfFormat == g_cfFileGroupDescriptor)
            hr = (pfmtetc->lindex == -1) ? S_OK : DV_E_LINDEX;
         else if (pfmtetc->cfFormat == g_cfFileContents)
            hr = (! pfmtetc->lindex) ? S_OK : DV_E_LINDEX;
         else
            hr = DV_E_FORMATETC;
      }
      else
         hr = DV_E_TYMED;
   }
   else
      hr = DV_E_DVASPECT;

   if (hr == S_OK)
      TRACE_OUT(("InternetShortcut::QueryGetData(): Clipboard format %s supported.",
                 GetClipboardFormatNameString(pfmtetc->cfFormat)));
   else
      TRACE_OUT(("InternetShortcut::QueryGetData(): Clipboard format %s not supported.",
                 GetClipboardFormatNameString(pfmtetc->cfFormat)));

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::QueryGetData, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::GetCanonicalFormatEtc(
                                                         PFORMATETC pfmtetcIn,
                                                         PFORMATETC pfmtetcOut)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::GetCanonicalFormatEtc);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRUCT_PTR(pfmtetcIn, CFORMATETC));
   ASSERT(IS_VALID_WRITE_PTR(pfmtetcOut, FORMATETC));

   hr = QueryGetData(pfmtetcIn);

   if (hr == S_OK)
   {
      *pfmtetcOut = *pfmtetcIn;

      if (pfmtetcIn->ptd == NULL)
         hr = DATA_S_SAMEFORMATETC;
      else
      {
         pfmtetcIn->ptd = NULL;
         ASSERT(hr == S_OK);
      }
   }
   else
      ZeroMemory(pfmtetcOut, sizeof(*pfmtetcOut));

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(FAILED(hr) ||
          IS_VALID_STRUCT_PTR(pfmtetcOut, CFORMATETC));

   DebugExitHRESULT(InternetShortcut::GetCanonicalFormatEtc, hr);

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE InternetShortcut::SetData(PFORMATETC pfmtetc,
                                                    PSTGMEDIUM pstgmed,
                                                    BOOL bRelease)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::SetData);

   // bRelease may be any value.

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRUCT_PTR(pfmtetc, CFORMATETC));
   ASSERT(IS_VALID_STRUCT_PTR(pstgmed, CSTGMEDIUM));

   hr = DV_E_FORMATETC;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::SetData, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


HRESULT STDMETHODCALLTYPE InternetShortcut::EnumFormatEtc(
                                                      DWORD dwDirFlags,
                                                      PIEnumFORMATETC *ppiefe)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::EnumFormatEtc);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(FLAGS_ARE_VALID(dwDirFlags, ALL_DATADIR_FLAGS));
   ASSERT(IS_VALID_WRITE_PTR(ppiefe, PIEnumFORMATETC));

   *ppiefe = NULL;

   if (dwDirFlags == DATADIR_GET)
   {
      FORMATETC rgfmtetc[] =
      {
         { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
         { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
         { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
         { 0, NULL, DVASPECT_CONTENT,  0, TYMED_HGLOBAL }
      };
      PEnumFormatEtc pefe;

      rgfmtetc[0].cfFormat = (CLIPFORMAT)g_cfURL;
      rgfmtetc[1].cfFormat = CF_TEXT;
      rgfmtetc[2].cfFormat = (CLIPFORMAT)g_cfFileGroupDescriptor;
      rgfmtetc[3].cfFormat = (CLIPFORMAT)g_cfFileContents;

      pefe = new(::EnumFormatEtc(rgfmtetc, ARRAY_ELEMENTS(rgfmtetc)));

      if (pefe)
      {
         hr = pefe->Status();

         if (hr == S_OK)
            *ppiefe = pefe;
         else
         {
            delete pefe;
            pefe = NULL;
         }
      }
      else
         hr = E_OUTOFMEMORY;
   }
   else
      // BUGBUG: Implement IDataObject::SetData() and add support for
      // DATADIR_SET here.
      hr = E_NOTIMPL;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT((hr == S_OK &&
           IS_VALID_INTERFACE_PTR(*ppiefe, IEnumFORMATETC)) ||
          (FAILED(hr) &&
           ! *ppiefe));

   DebugExitHRESULT(InternetShortcut::EnumFormatEtc, hr);

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE InternetShortcut::DAdvise(PFORMATETC pfmtetc,
                                                    DWORD dwAdviseFlags,
                                                    PIAdviseSink piadvsink,
                                                    PDWORD pdwConnection)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::DAdvise);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_STRUCT_PTR(pfmtetc, CFORMATETC));
   ASSERT(FLAGS_ARE_VALID(dwAdviseFlags, ALL_ADVISE_FLAGS));
   ASSERT(IS_VALID_INTERFACE_PTR(piadvsink, IAdviseSink));
   ASSERT(IS_VALID_WRITE_PTR(pdwConnection, DWORD));

   *pdwConnection = 0;
   hr = OLE_E_ADVISENOTSUPPORTED;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT((hr == S_OK &&
           *pdwConnection) ||
          (FAILED(hr) &&
           ! *pdwConnection));

   DebugExitHRESULT(InternetShortcut::DAdvise, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE InternetShortcut::DUnadvise(DWORD dwConnection)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::DUnadvise);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(dwConnection);

   hr = OLE_E_ADVISENOTSUPPORTED;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::DUnadvise, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


HRESULT STDMETHODCALLTYPE InternetShortcut::EnumDAdvise(PIEnumSTATDATA *ppiesd)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::EnumDAdvise);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_WRITE_PTR(ppiesd, PIEnumSTATDATA));

   *ppiesd = NULL;
   hr = OLE_E_ADVISENOTSUPPORTED;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT((hr == S_OK &&
           IS_VALID_INTERFACE_PTR(*ppiesd, IEnumSTATDATA)) ||
          (FAILED(hr) &&
           ! *ppiesd));

   DebugExitHRESULT(InternetShortcut::EnumDAdvise, hr);

   return(hr);
}

