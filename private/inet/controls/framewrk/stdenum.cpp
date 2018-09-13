//=--------------------------------------------------------------------------=
// StdEnum.Cpp
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// implementation of a generic enumerator object.
//
#include "IPServer.H"
#include "StdEnum.H"
#include "Globals.H"

SZTHISFILE

//=--------------------------------------------------------------------------=
// CStandardEnum::CStandardEnum
//=--------------------------------------------------------------------------=
// create the object and initialize the refcount
//
// Parameters:
//    REFCLSID        - [in] type of enumerator that we are
//    int             - [in] number of elements in the enumeration
//    int             - [in] size of each element
//    void *          - [in] pointer to element data
//    void (WINAPI *pfnCopyElement)(void *, const void *, DWORD)
//                    - [in] copying function
//
// Notes:
//
#pragma warning(disable:4355)  // using 'this' in constructor
CStandardEnum::CStandardEnum
(
    REFCLSID rclsid,
    int      cElements,
    int      cbElementSize,
    void    *rgElements,
    void (WINAPI *pfnCopyElement)(void *, const void *, DWORD)
)
: CUnknownObject(NULL, (IEnumGeneric *)this),
  m_iid(rclsid),
  m_cElements(cElements),
  m_cbElementSize(cbElementSize),
  m_iCurrent(0),
  m_rgElements(rgElements),
  m_pfnCopyElement(pfnCopyElement)
{
    m_pEnumClonedFrom = NULL;
}
#pragma warning(default:4355)  // using 'this' in constructor


//=--------------------------------------------------------------------------=
// CStandardEnum::CStandardEnum
//=--------------------------------------------------------------------------=
// "it is not death, but dying, which is terrible."
//    - Henry Fielding (1707-54)
//
// Notes:
//
CStandardEnum::~CStandardEnum ()
{
    // if we're a cloned object, then just release our parent object and
    // we're done. otherwise, free up the allocated memory we were given
    //
    if (m_pEnumClonedFrom)
        m_pEnumClonedFrom->Release();
    else {
        if (m_rgElements) HeapFree(g_hHeap, 0, m_rgElements);
    }
}

//=--------------------------------------------------------------------------=
// CStandardEnum::InternalQueryInterface
//=--------------------------------------------------------------------------=
// we support our internal iid, and that's all
//
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CStandardEnum::InternalQueryInterface
(
    REFIID riid,
    void **ppvObjOut
)
{
    if (DO_GUIDS_MATCH(riid, m_iid)) {
        ExternalAddRef();
        *ppvObjOut = (IEnumGeneric *)this;
        return S_OK;
    }

    return E_NOINTERFACE;
}

//=--------------------------------------------------------------------------=
// CStandardEnum::Next
//=--------------------------------------------------------------------------=
// returns the next dude in our iteration
//
// Parameters:
//    unsigned long     - [in]  count of elements requested
//    void    *         - [out] array of slots to put values in.
//    unsigned long *   - [out] actual number fetched
//
// Output:
//    HRESULT           - S_OK, E_INVALIDARG, S_FALSE
//
// Notes:
//
STDMETHODIMP CStandardEnum::Next
(
    unsigned long  cElm,
    void          *rgDest,
    unsigned long *pcElmOut
)
{
    unsigned long cElementsFetched = 0;
    void         *pElementDest = rgDest;
    const void   *pElementSrc = (const BYTE *)m_rgElements + (m_cbElementSize * m_iCurrent);

    while (cElementsFetched < cElm) {

        // if we hit EOF, break out
        //
        if (m_iCurrent >= m_cElements)
            break;

        // copy the element out for them
        //
        m_pfnCopyElement(pElementDest, pElementSrc, m_cbElementSize);

        // increase the counters
        //
        pElementDest = (LPBYTE)pElementDest + m_cbElementSize;
        pElementSrc  = (const BYTE *)pElementSrc + m_cbElementSize;
        m_iCurrent++;
        cElementsFetched++;
    }

    if (pcElmOut)
        *pcElmOut = cElementsFetched;

    return (cElementsFetched < cElm)? S_FALSE : S_OK;
}

//=--------------------------------------------------------------------------=
// CStandardEnum::Skip
//=--------------------------------------------------------------------------=
// skips the requested number of rows.
//
// Parameters:
//    unsigned long     - [in] number to skip
//
// Output:
//    HRESULT           - S_OK, S_FALSE
//
// Notes:
//
STDMETHODIMP CStandardEnum::Skip
(
    unsigned long cSkip
)
{
    // handle running off the end
    //
    if (m_iCurrent + (int)cSkip > m_cElements) {
        m_iCurrent = m_cElements;
        return S_FALSE;
    }

    m_iCurrent += cSkip;
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CStandardEnum::Reset
//=--------------------------------------------------------------------------=
// reset the counter.
//
// Output:
//    HRESULT        - S_OK
//
// Notes:
//
STDMETHODIMP CStandardEnum::Reset
(
    void
)
{
    m_iCurrent = 0;
    return S_OK;
}


//=--------------------------------------------------------------------------=
// CStandardEnum::Clone
//=--------------------------------------------------------------------------=
// clones the object and gives the new one the same position
//
// Parameters:
//    IEnumVARIANT **    - [out] where to put the new object.
//
// Output;
//    HRESULT            - S_OK, E_OUTOFMEMORY
//
// Notes:
//
STDMETHODIMP CStandardEnum::Clone
(
    IEnumGeneric **ppEnumClone
)
{
    CStandardEnum *pNewEnum;

    pNewEnum = new CStandardEnum(m_iid, m_cElements, m_cbElementSize, m_rgElements, m_pfnCopyElement);
    RETURN_ON_NULLALLOC(pNewEnum);

    // hold on to who we were cloned from so m_rgElements stays alive, and we don't
    // have to copy it.
    //
    pNewEnum->m_pEnumClonedFrom = this;

    // AddRef() ourselves on their behalf.
    //
    AddRef();
    *ppEnumClone = (IEnumGeneric *)pNewEnum;
    return S_OK;
}
