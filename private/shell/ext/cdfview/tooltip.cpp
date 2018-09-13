//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// infotip.cpp 
//
//   IQueryInfo for folder items.
//
//   History:
//
//       4/21/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"
#include "cdfidl.h"
#include "xmlutil.h"
#include "tooltip.h"
#include "dll.h"

//
// Constructor and destructor.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CQueryInfo::CQueryInfo ***
//
//    Constructor.
//
////////////////////////////////////////////////////////////////////////////////
CQueryInfo::CQueryInfo(
    PCDFITEMIDLIST pcdfidl,
    IXMLElementCollection* pIXMLElementCollection
)
: m_cRef(1)
{
    ASSERT(CDFIDL_IsValid(pcdfidl));
    ASSERT(ILIsEmpty(_ILNext(pcdfidl)));
    ASSERT(XML_IsCdfidlMemberOf(pIXMLElementCollection, pcdfidl));

    ASSERT(NULL == m_pIXMLElement);

    if (pIXMLElementCollection)
    {
        XML_GetElementByIndex(pIXMLElementCollection, CDFIDL_GetIndex(pcdfidl),
                              &m_pIXMLElement);
    }

    if (m_pIXMLElement)
        m_fHasSubItems = XML_IsFolder(m_pIXMLElement);
        
    TraceMsg(TF_OBJECTS, "+ IQueryInfo");

    DllAddRef();

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CQueryInfo::CQueryInfo ***
//
//    Constructor.
//
////////////////////////////////////////////////////////////////////////////////
CQueryInfo::CQueryInfo(
    IXMLElement* pIXMLElement,
    BOOL         fHasSubItems
)
: m_cRef(1)
{
    if (pIXMLElement)
    {
        pIXMLElement->AddRef();
        m_pIXMLElement = pIXMLElement;
    }

    m_fHasSubItems = fHasSubItems;
    
    TraceMsg(TF_OBJECTS, "+ IQueryInfo");

    DllAddRef();

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CQueryInfo::~CQueryInfo ***
//
//    Destructor.
//
////////////////////////////////////////////////////////////////////////////////
CQueryInfo::~CQueryInfo(
    void
)
{
    ASSERT(0 == m_cRef);

    if (m_pIXMLElement)
        m_pIXMLElement->Release();

    TraceMsg(TF_OBJECTS, "- IQueryInfo");

    DllRelease();

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CQueryInfo::QueryInterface ***
//
//    CQueryInfo QI.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CQueryInfo::QueryInterface (
    REFIID riid,
    void **ppv
)
{
    ASSERT(ppv);

    HRESULT hr;

    *ppv = NULL;

    if (IID_IUnknown == riid || IID_IQueryInfo == riid)
    {
        AddRef();
        *ppv = (IQueryInfo*)this;
        hr = S_OK;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    ASSERT((SUCCEEDED(hr) && *ppv) || (FAILED(hr) && NULL == *ppv));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CQueryInfo::AddRef ***
//
//    CQueryInfo AddRef.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CQueryInfo::AddRef (
    void
)
{
    ASSERT(m_cRef != 0);
    ASSERT(m_cRef < (ULONG)-1);

    return ++m_cRef;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CQueryInfo::Release ***
//
//    CQueryInfo Release.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CQueryInfo::Release (
    void
)
{
    ASSERT (m_cRef != 0);

    ULONG cRef = --m_cRef;
    
    if (0 == cRef)
        delete this;

    return cRef;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CQueryInfo:: ***
//
//
// Description:
//     Returns an info tip for the current pidl.
//
// Parameters:
//     [In]  dwFlags   - 
//     [Out] ppwszText - A pointer to a text buffer.  MAX_PATH length assumed!
//
// Return:
//     S_OK if the tip was extracted and returned.
//     E_OUTOFMEMORY if not enough memory is available.
//     E_FAIL otherwise.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CQueryInfo::GetInfoTip(
    DWORD dwFlags,
    WCHAR** ppwszTip
)
{
    ASSERT(ppwszTip);

    *ppwszTip = NULL;

    HRESULT hr;

    if (m_pIXMLElement)
    {
        BSTR bstr;

        bstr = XML_GetAttribute(m_pIXMLElement, XML_ABSTRACT);

        if (bstr)
        {
            if (*bstr)
            {
                //
                // REVIEW:  SHAlloc correct allocator?
                //

                int cbSize = SysStringByteLen(bstr) + sizeof(WCHAR);

                *ppwszTip = (WCHAR*)SHAlloc(cbSize);

                if (*ppwszTip)
                {
                    StrCpyW(*ppwszTip, bstr);

                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                //
                // REVIEW:  InfoTip when there is no abstract?
                //

                hr = E_FAIL;
            }

            SysFreeString(bstr);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    ASSERT((SUCCEEDED(hr) && *ppwszTip) || (FAILED(hr) && *ppwszTip == NULL));

    return hr;
}




HRESULT
CQueryInfo::GetInfoFlags(
    DWORD *pdwFlags
)
{
    ASSERT(pdwFlags);

    HRESULT hr = S_OK;

    if(!pdwFlags)
        return E_INVALIDARG;

    *pdwFlags = QIF_CACHED; // Assume cached by default
    if (!m_fHasSubItems)
        *pdwFlags |= QIF_DONTEXPANDFOLDER;

    if (m_pIXMLElement)
    {
        BSTR bstrURL = XML_GetAttribute(m_pIXMLElement, XML_HREF); 


        if (bstrURL)
        {
            if (*bstrURL)
            {
                BOOL fCached;
                //
                // REVIEW:  SHAlloc correct allocator?
                //
                fCached = CDFIDL_IsCachedURL(bstrURL);
                if(!fCached)
                    *pdwFlags &= ~QIF_CACHED;
            }

            SysFreeString(bstrURL);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        //
        // If m_pIXMLElement is NULL either its a low memory situation, or
        // the corresponding cdf is not in the cache.
        // 

        *pdwFlags &= ~QIF_CACHED;
    }

    return hr;
}

