//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// enum.cpp 
//
//   The enumerator object for the cdf viewer.
//
//   History:
//
//       3/16/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"
#include "cdfidl.h"
#include "xmlutil.h"
#include "enum.h"
#include "dll.h"


//
// Constructor and destructor.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfEnum::CCdfView ***
//
//    Constructor.
//
////////////////////////////////////////////////////////////////////////////////
CCdfEnum::CCdfEnum (
    IXMLElementCollection* pIXMLElementCollection,
    DWORD fEnumerateFlags,
    PCDFITEMIDLIST pcdfidlFolder
)
: m_cRef(1),
  m_fEnumerate(fEnumerateFlags)
{
    //
    // Zero inited memory.
    //

    ASSERT(NULL == m_pIXMLElementCollection);
    ASSERT(0 == m_nCurrentItem);

    if (pIXMLElementCollection)
    {
        pIXMLElementCollection->AddRef();
        m_pIXMLElementCollection = pIXMLElementCollection;
    }

    m_pcdfidlFolder = (PCDFITEMIDLIST)ILClone((LPITEMIDLIST)pcdfidlFolder);
    
    //
    // Don't allow the dll to be unloaded.
    //

    TraceMsg(TF_OBJECTS, "+ IEnumIDList");

    DllAddRef();
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::~CCdfView **
//
//    Destructor.
//
////////////////////////////////////////////////////////////////////////////////
CCdfEnum::~CCdfEnum(
    void
)
{
    if (m_pIXMLElementCollection)
        m_pIXMLElementCollection->Release();

    TraceMsg(TF_OBJECTS, "- IEnumIDList");

    if (m_pcdfidlFolder)
        ILFree((LPITEMIDLIST)m_pcdfidlFolder);

    DllRelease();
}

//
// IUnknown methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::CCdfEnum ***
//
//    Cdf view QI.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfEnum::QueryInterface (
    REFIID riid,
    void **ppv
)
{
    ASSERT(ppv);

    HRESULT hr;

    if (IID_IUnknown == riid || IID_IEnumIDList == riid)
    {
        AddRef();
        *ppv = (IEnumIDList*)this;
        hr = S_OK;
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    ASSERT((SUCCEEDED(hr) && *ppv) || (FAILED(hr) && NULL == *ppv));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfEnum::AddRef ***
//
//    Cdf view AddRef.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CCdfEnum::AddRef (
    void
)
{
    ASSERT(m_cRef != 0);
    ASSERT(m_cRef < (ULONG)-1);

    return ++m_cRef;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfEnum::Release ***
//
//    Cdf view Release.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CCdfEnum::Release (
    void
)
{
    ASSERT (m_cRef != 0);

    ULONG cRef = --m_cRef;
    
    if (0 == cRef)
        delete this;

    return cRef;
}


//
// IEnumIDList methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfEnum::Next ***
//
//
// Description:
//     Returns the next n item id lists associated with this enumerator.
//
// Parameters:
//     [in]  celt         - Number of item id lists to return.
//     [Out] rgelt        - A pointer to an array of item id list pointers that
//                          will receive the id item lists.
//     [Out] pceltFetched - A pointer to a ULONG that receives a count of the
//                          number of id lists fetched.
//
// Return:
//     S_OK if celt items where fetched.
//     S_FALSE if celt items where not fetched.
//
// Comments:
//     
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfEnum::Next(
    ULONG celt,	
    LPITEMIDLIST *rgelt,	
    ULONG* pceltFetched)
{
    ASSERT(rgelt || 0 == celt);
    ASSERT(pceltFetched || 1 == celt);

    //
    // pceltFetched can be NULL if and only if celt is 1.
    //

    ULONG lFetched;

    if (1 == celt && NULL == pceltFetched)
        pceltFetched = &lFetched;

    for (*pceltFetched = 0; *pceltFetched < celt; (*pceltFetched)++)
    {
        if (NULL == (rgelt[*pceltFetched] = NextCdfidl()))
            break;

        ASSERT(CDFIDL_IsValid((PCDFITEMIDLIST)rgelt[*pceltFetched]));
    }

    return (*pceltFetched == celt) ? S_OK : S_FALSE;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfEnum::Skip ***
//
//   Shell doesn't call this member.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfEnum::Skip(
    ULONG celt)
{
    return E_NOTIMPL;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfEnum::Reset ***
//
//   Set the current item to the index of the first item in CFolderItems.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfEnum::Reset(
    void
)
{
    m_nCurrentItem = 0;
    m_fReturnedFolderPidl = FALSE;

    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfEnum::Clone ***
//
//   Shell doesn't call this method.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfEnum::Clone(
    IEnumIDList **ppenum
)
{
    return E_NOTIMPL;
}

//
// Helper functions.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfEnum::NextCdfidl ***
//
//
// Description:
//     Returns a cdf item idl list for the next cdf item in the collection
//
// Parameters:
//     None.
//
// Return:
//     A pointer to a new cdf item id list.
//     NULL if there aren't any more items or if there isn't enough memory to
//     allocated an id list for the item.
//
// Comments:
//     The caller is responsible for freeing the returned item id list.
//
////////////////////////////////////////////////////////////////////////////////
LPITEMIDLIST
CCdfEnum::NextCdfidl(
    void
)
{
    PCDFITEMIDLIST pcdfidlNew = NULL;

    IXMLElement* pIXMLElement;
    ULONG        nIndex;

    //the first item in the enum is the folder's link (if it has one)
    if (!m_fReturnedFolderPidl && m_pIXMLElementCollection)
    {
        IXMLElement *pIXMLElementChild;

        XML_GetElementByIndex(m_pIXMLElementCollection, 0, &pIXMLElementChild);

        if (pIXMLElementChild)
        {
            pIXMLElementChild->get_parent(&pIXMLElement);
            if (pIXMLElement)
            {
                BSTR bstr = XML_GetAttribute(pIXMLElement, XML_HREF);
                if (bstr)
                {
                    if (*bstr)
                        pcdfidlNew = CDFIDL_CreateFolderPidl(m_pcdfidlFolder);
                    SysFreeString(bstr);
                }
                
                //get_parent doesn't addref???
                pIXMLElement->Release();
            }
            pIXMLElementChild->Release();
        }

        m_fReturnedFolderPidl = TRUE;
    }

    if (!pcdfidlNew)
    {
        HRESULT hr = GetNextCdfElement(&pIXMLElement, &nIndex);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIXMLElement);

            pcdfidlNew = CDFIDL_CreateFromXMLElement(pIXMLElement, nIndex);

            pIXMLElement->Release();
        }
    }
    
    ASSERT(CDFIDL_IsValid(pcdfidlNew) || NULL == pcdfidlNew);

    return (LPITEMIDLIST)pcdfidlNew;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfEnum::GetNextCdfElement ***
//
//
// Description:
//     Get the IXMLElement pointer and index for the next cdf item in the
//     collection.
//
// Parameters:
//     [Out] ppIXMLElement - A pointer that recieves the xml element.
//     [Out] pnIndex       - The object model index of the xml element.
//
// Return:
//     S_OK if the element was found.
//     E_FAIL otherwise.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CCdfEnum::GetNextCdfElement(
    IXMLElement** ppIXMLElement,
    ULONG* pnIndex
)
{
    ASSERT(ppIXMLElement);

    HRESULT hr;

    if (m_pIXMLElementCollection)
    {
        IXMLElement* pIXMLElement;

        hr = XML_GetElementByIndex(m_pIXMLElementCollection,
                                   m_nCurrentItem++, &pIXMLElement);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIXMLElement)

            if (IsCorrectType(pIXMLElement))
            {
                pIXMLElement->AddRef();
                *ppIXMLElement = pIXMLElement;
                *pnIndex = m_nCurrentItem - 1;
            }
            else
            {
                hr = GetNextCdfElement(ppIXMLElement, pnIndex);
            }

            pIXMLElement->Release();
        }
    }
    else
    {
        hr = E_FAIL;
    }

    ASSERT(SUCCEEDED(hr) && *ppIXMLElement || FAILED(hr));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfEnum::IsCorrectType ***
//
//
// Description:
//     Determines if the given xml element is a cdf element and if it should
//     be returned accroding to the folder non-folder enumrator flags.
//
// Parameters:
//     [In]  pIXMLElement - The xml element to check.
//
// Return:
//     TRUE if the lement is cdf displayable and the correct type for this
//     enumerator.
//     FALSE if the given element should not be enumerated.
//
// Comments:
//     Id list enumerators are created with a combination of SHCONTF_FOLDERS,
//     SHCONTF_NONFOLDERS and SHCONTF_INCLUDEHIDDEN flags.
//
////////////////////////////////////////////////////////////////////////////////
inline BOOL
CCdfEnum::IsCorrectType(
    IXMLElement* pIXMLElement
)
{
    return (XML_IsCdfDisplayable(pIXMLElement) &&
            (XML_IsFolder(pIXMLElement) ? (m_fEnumerate & SHCONTF_FOLDERS) :
                                          (m_fEnumerate & SHCONTF_NONFOLDERS)));
}
