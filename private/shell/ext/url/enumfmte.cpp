/*
 * enumfmte.cpp - EnumFormatEtc class implementation.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop

#include "enumfmte.hpp"


/***************************** Private Functions *****************************/


#ifdef DEBUG

PRIVATE_CODE BOOL IsValidArrayOfFORMATETCs(CFORMATETC rgcfmtetc[],
                                           ULONG ulcFormats)
{
   BOOL bResult = TRUE;
   ULONG ul;

   for (ul = 0; ul < ulcFormats; ul++)
      bResult = (EVAL(IS_VALID_STRUCT_PTR(&(rgcfmtetc[ul]), CFORMATETC)) &&
                 bResult);

   return(bResult);
}


PRIVATE_CODE BOOL IsValidPCEnumFormatEtc(PCEnumFormatEtc pcefe)
{
   return(IS_VALID_READ_PTR(pcefe, CEnumFormatEtc) &&
          EVAL(IsValidArrayOfFORMATETCs(pcefe->m_pfmtetc, pcefe->m_ulcFormats)) &&
          EVAL(pcefe->m_uliCurrent <= pcefe->m_ulcFormats) &&
          IS_VALID_STRUCT_PTR((PCRefCount)pcefe, CRefCount) &&
          IS_VALID_INTERFACE_PTR((PCIEnumFORMATETC)pcefe, IEnumFORMATETC));
}

#endif


/********************************** Methods **********************************/


EnumFormatEtc::EnumFormatEtc(CFORMATETC rgcfmtetc[], ULONG ulcFormats)
{
   DebugEntry(EnumFormatEtc::EnumFormatEtc);

   // Don't validate this until after construction.

   ASSERT(IsValidArrayOfFORMATETCs(rgcfmtetc, ulcFormats));

   m_pfmtetc = new(FORMATETC[ulcFormats]);

   if (m_pfmtetc)
   {
      CopyMemory(m_pfmtetc, rgcfmtetc, ulcFormats * sizeof(rgcfmtetc[0]));
      m_ulcFormats = ulcFormats;
   }
   else
      m_ulcFormats = 0;

   m_uliCurrent = 0;

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));

   DebugExitVOID(EnumFormatEtc::EnumFormatEtc);

   return;
}


EnumFormatEtc::~EnumFormatEtc(void)
{
   DebugEntry(EnumFormatEtc::~EnumFormatEtc);

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));

   if (m_pfmtetc)
   {
      delete m_pfmtetc;
      m_pfmtetc = NULL;
   }

   m_ulcFormats = 0;
   m_uliCurrent = 0;

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));

   DebugExitVOID(EnumFormatEtc::~EnumFormatEtc);

   return;
}


ULONG STDMETHODCALLTYPE EnumFormatEtc::AddRef(void)
{
   ULONG ulcRef;

   DebugEntry(EnumFormatEtc::AddRef);

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));

   ulcRef = RefCount::AddRef();

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));

   DebugExitULONG(EnumFormatEtc::AddRef, ulcRef);

   return(ulcRef);
}


ULONG STDMETHODCALLTYPE EnumFormatEtc::Release(void)
{
   ULONG ulcRef;

   DebugEntry(EnumFormatEtc::Release);

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));

   ulcRef = RefCount::Release();

   DebugExitULONG(EnumFormatEtc::Release, ulcRef);

   return(ulcRef);
}


HRESULT STDMETHODCALLTYPE EnumFormatEtc::QueryInterface(REFIID riid,
                                                       PVOID *ppvObject)
{
   HRESULT hr = S_OK;

   DebugEntry(EnumFormatEtc::QueryInterface);

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));
   ASSERT(IsValidREFIID(riid));
   ASSERT(IS_VALID_WRITE_PTR(ppvObject, PVOID));

   if (riid == IID_IEnumFORMATETC)
   {
      *ppvObject = (PIEnumFORMATETC)this;
      ASSERT(IS_VALID_INTERFACE_PTR((PIEnumFORMATETC)*ppvObject, IEnumFORMATETC));
      TRACE_OUT(("EnumFormatEtc::QueryInterface(): Returning IEnumFORMATETC."));
   }
   else if (riid == IID_IUnknown)
   {
      *ppvObject = (PIUnknown)this;
      ASSERT(IS_VALID_INTERFACE_PTR((PIUnknown)*ppvObject, IUnknown));
      TRACE_OUT(("EnumFormatEtc::QueryInterface(): Returning IUnknown."));
   }
   else
   {
      *ppvObject = NULL;
      hr = E_NOINTERFACE;
      TRACE_OUT(("EnumFormatEtc::QueryInterface(): Called on unknown interface."));
   }

   if (hr == S_OK)
      AddRef();

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));
#ifndef MAINWIN
// INTERFACE is changed to CINTERFACE which is not defined ??
// LOOK AT THIS LATER.
   ASSERT(FAILED(hr) ||
          IS_VALID_INTERFACE_PTR(*ppvObject, INTERFACE));
#endif

   DebugExitHRESULT(EnumFormatEtc::QueryInterface, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE EnumFormatEtc::Next(ULONG ulcToFetch,
                                              PFORMATETC pfmtetc,
                                              PULONG pulcFetched)
{
   HRESULT hr = S_FALSE;
   ULONG ulcFetched;

   DebugEntry(EnumFormatEtc::Next);

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));

   if (m_uliCurrent < m_ulcFormats)
   {
      ULONG ulcCanFetch = m_ulcFormats - m_uliCurrent;

      ulcFetched = min(ulcCanFetch, ulcToFetch);

      CopyMemory(pfmtetc, &(m_pfmtetc[m_uliCurrent]),
                 ulcFetched * sizeof(*pfmtetc));

      m_uliCurrent += ulcFetched;
   }
   else
      // End of the list.
      ulcFetched = 0;

   if (pulcFetched)
      *pulcFetched = ulcFetched;
   else
      ASSERT(ulcToFetch == 1);

   if (ulcFetched < ulcToFetch)
      hr = S_FALSE;
   else
   {
      ASSERT(ulcFetched == ulcToFetch);
      hr = S_OK;
   }

   TRACE_OUT(("EnumFormatEtc::Next(): Fetched %lu FORMATETCs.",
              ulcFetched));

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));
   ASSERT((((hr == S_OK &&
             EVAL((! pulcFetched &&
                   ulcToFetch == 1) ||
                  *pulcFetched == ulcToFetch)) ||
            (hr == S_FALSE &&
             EVAL((! pulcFetched &&
                   ulcToFetch == 1) ||
                  *pulcFetched < ulcToFetch))) &&
            EVAL((! pulcFetched &&
                  IS_VALID_STRUCT_PTR(pfmtetc, CFORMATETC)) ||
                 IsValidArrayOfFORMATETCs(pfmtetc, *pulcFetched))) ||
          (FAILED(hr) &&
           EVAL((! pulcFetched &&
                 ulcToFetch == 1) ||
                ! *pulcFetched)));

   DebugExitHRESULT(EnumFormatEtc::Next, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE EnumFormatEtc::Skip(ULONG ulcToSkip)
{
   HRESULT hr;

   DebugEntry(EnumFormatEtc::Skip);

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));

   if (ulcToSkip <= m_ulcFormats - m_uliCurrent)
   {
      m_uliCurrent += ulcToSkip;
      hr = S_OK;

      TRACE_OUT(("EnumFormatEtc::Skip(): Skipped %lu FORMATETCs, as requested.",
                 ulcToSkip));
   }
   else
   {
      TRACE_OUT(("EnumFormatEtc::Skip(): Skipped %lu of %lu FORMATETCs.",
                 m_ulcFormats - m_uliCurrent,
                 ulcToSkip));

      m_uliCurrent = m_ulcFormats;
      hr = S_FALSE;
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));
   ASSERT((hr == S_OK &&
           m_uliCurrent <= m_ulcFormats) ||
          (hr == S_FALSE &&
           m_uliCurrent == m_ulcFormats));

   DebugExitHRESULT(EnumFormatEtc::Skip, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE EnumFormatEtc::Reset(void)
{
   HRESULT hr;

   DebugEntry(EnumFormatEtc::Reset);

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));

   m_uliCurrent = 0;
   hr = S_OK;

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));
   ASSERT(hr == S_OK &&
          ! m_uliCurrent);

   DebugExitHRESULT(EnumFormatEtc::Reset, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE EnumFormatEtc::Clone(PIEnumFORMATETC *ppiefe)
{
   HRESULT hr;
   PEnumFormatEtc pefe;

   DebugEntry(EnumFormatEtc::Clone);

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));

   pefe = new(EnumFormatEtc(m_pfmtetc, m_ulcFormats));

   if (pefe)
   {
      hr = pefe->Status();

      if (hr == S_OK)
      {
         hr = pefe->Skip(m_uliCurrent);

         if (hr == S_OK)
            *ppiefe = pefe;
         else
            hr = E_UNEXPECTED;
      }

      if (hr != S_OK)
      {
         delete pefe;
         pefe = NULL;
      }
   }
   else
      hr = E_OUTOFMEMORY;

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));
   ASSERT((hr == S_OK &&
           IS_VALID_INTERFACE_PTR(*ppiefe, IEnumFORMATETC)) ||
          (FAILED(hr) &&
           ! *ppiefe));

   DebugExitHRESULT(EnumFormatEtc::Clone, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE EnumFormatEtc::Status(void)
{
   HRESULT hr;

   DebugEntry(EnumFormatEtc::Status);

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));

   hr = (m_pfmtetc ? S_OK : E_OUTOFMEMORY);

   ASSERT(IS_VALID_STRUCT_PTR(this, CEnumFormatEtc));

   DebugExitHRESULT(EnumFormatEtc::Status, hr);

   return(hr);
}

