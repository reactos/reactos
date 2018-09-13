//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// bindstcb.cpp 
//
//   Bind status callback object.  Called by cdf file parser.
//
//   History:
//
//       3/31/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"
#include "cdfidl.h"
#include "xmlutil.h"
#include "persist.h"
#include "bindstcb.h"
#include "chanapi.h"
#include "chanenum.h"
#include "dll.h"
#include "resource.h"

//
// Constructor and destructor.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::CBindStatusCallback ***
//
//    Constructor.
//
////////////////////////////////////////////////////////////////////////////////
CBindStatusCallback::CBindStatusCallback (
	IXMLDocument* pIXMLDocument,
    LPCWSTR pszURLW
)
: m_cRef(1)
{
    ASSERT(pIXMLDocument);
    ASSERT(pszURLW);

    pIXMLDocument->AddRef();
    m_pIXMLDocument = pIXMLDocument;

    int cb = StrLenW(pszURLW) + 1;

    m_pszURL = new TCHAR[cb];

    if (m_pszURL)
        SHUnicodeToTChar(pszURLW, m_pszURL, cb);

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::~CBindStatusCallback ***
//
//    Destructor.
//
////////////////////////////////////////////////////////////////////////////////
CBindStatusCallback::~CBindStatusCallback (
	void
)
{
    ASSERT(0 == m_cRef);

    if (m_pIXMLDocument)
        m_pIXMLDocument->Release();

    if (m_pszURL)
        delete [] m_pszURL;

    if (m_pPrevIBindStatusCallback)
        m_pPrevIBindStatusCallback->Release();

	return;
}


//
// IUnknown methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::QueryInterface ***
//
//    CBindStatusCallback QI.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback::QueryInterface (
    REFIID riid,
    void **ppv
)
{
    HRESULT hr;

    ASSERT(ppv);

    if (IID_IUnknown == riid || IID_IBindStatusCallback == riid)
    {
        AddRef();
        *ppv = (IBindStatusCallback*)this;
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
// *** CBindStatusCallback::AddRef ***
//
//    CBindStatusCallback AddRef.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CBindStatusCallback::AddRef (
    void
)
{
    ASSERT(m_cRef != 0);
    ASSERT(m_cRef < (ULONG)-1);

    return ++m_cRef;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::Release ***
//
//    CContextMenu Release.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CBindStatusCallback::Release (
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
// IBindStatusCallback methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::GetBindInfo ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback::GetBindInfo(
    DWORD* pgrfBINDF,
    BINDINFO* pbindinfo
)
{
    //ASSERT(pgrfBINDF);

    //*pgrfBINDF &= ~BINDF_ASYNCHRONOUS;

    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::OnStartBinding ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback::OnStartBinding(
    DWORD dwReserved,
    IBinding* pIBinding
)
{
    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::GetPriority ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback::GetPriority(
    LONG *pnPriority
)
{
    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::OnProgress ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback::OnProgress(
    ULONG ulProgress,
    ULONG ulProgressMax,
    ULONG ulStatusCode,
    LPCWSTR szStatusText
)
{
    HRESULT hr;

    if (m_pPrevIBindStatusCallback)
    {
        hr = m_pPrevIBindStatusCallback->OnProgress(ulProgress, ulProgressMax,
                                                    ulStatusCode, szStatusText);
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::OnDataAvailable ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback::OnDataAvailable(
    DWORD grfBSCF,
    DWORD dwSize,
    FORMATETC* pfmtect,
    STGMEDIUM* pstgmed
)
{
    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::OnObjectAvialable ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback::OnObjectAvailable(
    REFIID riid,
    IUnknown* pIUnknown
)
{
    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::OnLowResource ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback::OnLowResource(
    DWORD dwReserved
)
{
    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::OnStopBinding ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback::OnStopBinding(
    HRESULT hrStatus,
    LPCWSTR szStatusText
)
{
    if (m_pPrevIBindStatusCallback)
        m_pPrevIBindStatusCallback->OnStopBinding(hrStatus, szStatusText);

    HRESULT hr = hrStatus;

    if (SUCCEEDED(hr))
    {
        if (m_pszURL)
        {
            ASSERT(m_pIXMLDocument);

            XML_DownloadImages(m_pIXMLDocument);

            // Moved to constructor.
            //Cache_AddItem(m_pszURL, m_pIXMLDocument, PARSE_NET);

            XML_MarkCacheEntrySticky(m_pszURL);

            //
            // Update the item now that the download is complete.
            //

            WCHAR wszURL[INTERNET_MAX_URL_LENGTH];

            if (SHTCharToUnicode(m_pszURL, wszURL, ARRAYSIZE(wszURL)))
                Channel_SendUpdateNotifications(wszURL);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


//
// Helper functions.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::Wait ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CBindStatusCallback::Init(
    IBindStatusCallback* pPrevIBindStatusCallback
)
{
    ASSERT(NULL == m_pPrevIBindStatusCallback);

    m_pPrevIBindStatusCallback = pPrevIBindStatusCallback;

    return S_OK;
}


//
// Constructor and destructor.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::CBindStatusCallback ***
//
//    Constructor.
//
////////////////////////////////////////////////////////////////////////////////
CBindStatusCallback2::CBindStatusCallback2 (
	HWND hwnd
)
: m_cRef(1),
  m_hwnd(hwnd)
{
    DllAddRef();

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::~CBindStatusCallback ***
//
//    Destructor.
//
////////////////////////////////////////////////////////////////////////////////
CBindStatusCallback2::~CBindStatusCallback2 (
	void
)
{
    ASSERT(0 == m_cRef);

    DllRelease();

	return;
}


//
// IUnknown methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::QueryInterface ***
//
//    CBindStatusCallback QI.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback2::QueryInterface (
    REFIID riid,
    void **ppv
)
{
    HRESULT hr;

    ASSERT(ppv);

    if (IID_IUnknown == riid || IID_IBindStatusCallback == riid)
    {
        AddRef();
        *ppv = (IBindStatusCallback*)this;
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
// *** CBindStatusCallback::AddRef ***
//
//    CBindStatusCallback AddRef.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CBindStatusCallback2::AddRef (
    void
)
{
    ASSERT(m_cRef != 0);
    ASSERT(m_cRef < (ULONG)-1);

    return ++m_cRef;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::Release ***
//
//    CContextMenu Release.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CBindStatusCallback2::Release (
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
// IBindStatusCallback methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::GetBindInfo ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback2::GetBindInfo(
    DWORD* pgrfBINDF,
    BINDINFO* pbindinfo
)
{
    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::OnStartBinding ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback2::OnStartBinding(
    DWORD dwReserved,
    IBinding* pIBinding
)
{
    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::GetPriority ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback2::GetPriority(
    LONG *pnPriority
)
{
    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::OnProgress ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback2::OnProgress(
    ULONG ulProgress,
    ULONG ulProgressMax,
    ULONG ulStatusCode,
    LPCWSTR szStatusText
)
{
    PostMessage(m_hwnd, WM_COMMAND, DOWNLOAD_PROGRESS,
                0);

    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::OnDataAvailable ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback2::OnDataAvailable(
    DWORD grfBSCF,
    DWORD dwSize,
    FORMATETC* pfmtect,
    STGMEDIUM* pstgmed
)
{
    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::OnObjectAvialable ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback2::OnObjectAvailable(
    REFIID riid,
    IUnknown* pIUnknown
)
{
    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::OnLowResource ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback2::OnLowResource(
    DWORD dwReserved
)
{
    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CBindStatusCallback::OnStopBinding ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBindStatusCallback2::OnStopBinding(
    HRESULT hrStatus,
    LPCWSTR szStatusText
)
{
    HRESULT hr = hrStatus;

    PostMessage(m_hwnd, WM_COMMAND, DOWNLOAD_COMPLETE,
                SUCCEEDED(hr) ? TRUE : FALSE);

    return hr;
}

