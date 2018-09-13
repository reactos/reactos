//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// cdfview.cpp 
//
//   IUnknown for the cdfview class.
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
#include "persist.h"
#include "cdfview.h"
#include "view.h"
#include "xmlutil.h"
#include "dll.h"


//
// Constructor and destructor.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::CCdfView ***
//
//    Constructor.
//
////////////////////////////////////////////////////////////////////////////////
CCdfView::CCdfView (
	void
)
: CPersist(FALSE), // TRUE indicates cdf hasn't been parsed.
  m_cRef(1),
  m_fIsRootFolder(TRUE)
{
    //
    // Memory allocs are assumed to be zero init'ed.
    //

    ASSERT(NULL == m_pcdfidl);
    ASSERT(NULL == m_pIXMLElementCollection);
    ASSERT(NULL == m_pidlPath);

    //
    // As long as this class is around the dll should stay loaded.
    //

    TraceMsg(TF_OBJECTS, "+ IShellFolder");
    //TraceMsg(TF_ALWAYS,  "+ IShellFolder %0x08d", this);

    DllAddRef();

	return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::CCdfView ***
//
//    Constructor.
//
////////////////////////////////////////////////////////////////////////////////
CCdfView::CCdfView (
	PCDFITEMIDLIST pcdfidl,
    LPCITEMIDLIST pidlParentPath,
    IXMLElementCollection* pParentIXMLElementCollection
)
: CPersist(TRUE),  // TRUE indicates cdf already parsed.
  m_cRef(1),
  m_fIsRootFolder(FALSE)
{
    ASSERT(CDFIDL_IsValid(pcdfidl));
    ASSERT(pParentIXMLElementCollection == NULL ||
           XML_IsCdfidlMemberOf(pParentIXMLElementCollection, pcdfidl));

    ASSERT(NULL == m_pidlPath);
    ASSERT(NULL == m_pIXMLElementCollection);

    //
    // Note that m_pidlPath, m_pcdfidl & m_pIXMLElementCollection could be
    // NULL in low memory conditions.
    //

    m_pcdfidl = (PCDFITEMIDLIST)ILCloneFirst((LPITEMIDLIST)pcdfidl);

    ASSERT(CDFIDL_IsValid(m_pcdfidl) || NULL == m_pcdfidl);
    ASSERT(ILIsEmpty(_ILNext((LPITEMIDLIST)m_pcdfidl)) || NULL == m_pcdfidl);

    m_pidlPath = ILCombine(pidlParentPath, (LPITEMIDLIST)m_pcdfidl);

    if (pParentIXMLElementCollection)
    {
        XML_GetChildElementCollection(pParentIXMLElementCollection,
                                      CDFIDL_GetIndexId(&pcdfidl->mkid),
                                      &m_pIXMLElementCollection);
    }
    
    //
    // As long as this class is around the dll should stay loaded.
    //

    TraceMsg(TF_OBJECTS, "+ IShellFolder %s", CDFIDL_GetName(pcdfidl));
    //TraceMsg(TF_ALWAYS,  "+ IShellFolder %0x08d", this);

    DllAddRef();

	return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::~CCdfView **
//
//    Destructor.
//
////////////////////////////////////////////////////////////////////////////////
CCdfView::~CCdfView (
	void
)
{
    ASSERT(0 == m_cRef);

    if (m_pidlPath)
        ILFree(m_pidlPath);

    if (m_pcdfidl)
        CDFIDL_Free(m_pcdfidl);

    if (m_pIXMLElementCollection)
        m_pIXMLElementCollection->Release();

    //
    // Matching Release for the constructor Addref.
    //

    TraceMsg(TF_OBJECTS, "- IShellFolder");
    //TraceMsg(TF_ALWAYS,  "- IShellFolder %0x08d", this);

    DllRelease();

	return;
}


//
// IUnknown methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::QueryInterface **
//
//    Cdf view QI.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfView::QueryInterface (
    REFIID riid,
    void **ppv
)
{
    ASSERT(ppv);

    HRESULT hr;

    *ppv = NULL;

    if (IID_IUnknown == riid || IID_IShellFolder == riid)
    {
        *ppv = (IShellFolder*)this;
    }
    else if (IID_IPersist == riid || IID_IPersistFile == riid)
    {
        *ppv = (IPersistFile*)this;
    }
    else if (IID_IPersistFolder == riid)
    {
        *ppv = (IPersistFolder*)this;
    }
    else if (IID_IPersistMoniker == riid)
    {
        *ppv = (IPersistMoniker*)this;
    }
    else if (IID_IOleObject == riid)
    {
        *ppv = (IOleObject*)this;
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        hr = S_OK;
    }
    else if (IID_IShellView == riid)
    {
        ASSERT(0);  // Is this required?

        /*LPITEMIDLIST pidl = ILClone((LPITEMIDLIST)m_pcdfidl);

        if (pidl)
        {
            hr = CreateDefaultShellView((IShellFolder*)this, pidl,
                                        (IShellView**)ppv);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }*/
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
// *** CCdfView::AddRef **
//
//    Cdf view AddRef.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CCdfView::AddRef (
    void
)
{
    ASSERT(m_cRef != 0);
    ASSERT(m_cRef < (ULONG)-1);

    return ++m_cRef;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::Release **
//
//    Cdf view Release.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CCdfView::Release (
    void
)
{
    ASSERT (m_cRef != 0);

    ULONG cRef = --m_cRef;
    
    if (0 == cRef)
        delete this;

    return cRef;
}

#ifdef UNIX
void unixEnsureFileScheme(TCHAR  *pszIn)
{
    if(pszIn && *pszIn == TEXT(FILENAME_SEPARATOR))
    {
        TCHAR tmpBuffer[MAX_PATH];
        int len = lstrlen(pszIn);
        lstrcpy(tmpBuffer,TEXT("file://"));
        lstrcat(tmpBuffer,pszIn);
        lstrcpy(pszIn,tmpBuffer);
    }
}
#endif /* UNIX */

