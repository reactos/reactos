/********************************************************
 usercom.cpp

  User Manager COM interface implementation

 History:
  09/23/98: dsheldon created
********************************************************/

#include "stdafx.h"
#include "resource.h"

#include "usercom.h"
#include <initguid.h>
#include "shlguidp.h"

#include "misc.h"

/***************************************************
 CUserPropertyPages implementation

  IShellExtInit and IShellPropSheetExt implementations
***************************************************/

CUserPropertyPages::~CUserPropertyPages()
{
    TraceEnter(TRACE_USR_COM, "CUserPropertyPages::~CUserPropertyPages ");
    
    if (m_pUserInfo != NULL) 
        delete m_pUserInfo;

    if (m_pUsernamePage != NULL)
        delete m_pUsernamePage;

    if (m_pGroupPage != NULL)
        delete m_pGroupPage;

    TraceLeaveVoid();
}

HRESULT CUserPropertyPages::QueryInterface(REFIID iid, LPVOID* ppvOut)
{
    TraceEnter(TRACE_USR_COM, "CUserPropertyPages ::QueryInterface");

    *ppvOut = NULL;
    HRESULT hr = E_NOINTERFACE;

    if ((iid == IID_IUnknown) || (iid == IID_IShellPropSheetExt))
    {
        *ppvOut = (IShellPropSheetExt*) this;
        hr = S_OK;
    }
    else if (iid == IID_IShellExtInit)
    {
        *ppvOut = (IShellExtInit*) this;
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        ((IShellPropSheetExt*) this)->AddRef();
    }

    TraceLeaveResult(hr);
}

HRESULT CUserPropertyPages::Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID)
{
    TraceEnter(TRACE_USR_COM, "CUserPropertyPages ::Initialize");

    // Request the user's SID from the data object
    FORMATETC fmt;
    fmt.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_USERPROPPAGESSID);
    fmt.ptd = NULL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.tymed = TYMED_HGLOBAL;

    STGMEDIUM medium;

    HRESULT hr = lpdobj->GetData(&fmt, &medium);

    if (SUCCEEDED(hr))
    {
        // medium.hGlobal is the user's SID; make sure it isn't null and that
        // we haven't already set our copy of the SID
        if ((medium.hGlobal != NULL) && (m_pUserInfo == NULL))
        {
            PSID psid = (PSID) GlobalLock(medium.hGlobal);

            if (IsValidSid(psid))
            {
                // Create a user info structure to party on
                m_pUserInfo = new CUserInfo;
            
                if (m_pUserInfo)
                {
                    hr = m_pUserInfo->Load(psid, TRUE);

                    if (SUCCEEDED(hr))
                    {
                        // Get the groups
                        hr = m_GroupList.Initialize();
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }

            GlobalUnlock(medium.hGlobal);
        }
        else
        {
            // hGlobal was NULL or prop sheet was already init'ed
            hr = E_UNEXPECTED;
        }

        ReleaseStgMedium(&medium);
    }
        

    TraceLeaveResult(hr);
}

HRESULT CUserPropertyPages::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    TraceEnter(TRACE_USR_COM, "CUserPropertyPages ::AddPages");

    HRESULT hr = E_FAIL;

    // Settings common to all pages
    PROPSHEETPAGE psp = {0};
    psp.dwSize = sizeof (psp);
    psp.hInstance = g_hInstance;

    if (m_pUserInfo->m_userType == CUserInfo::LOCALUSER)
    {
        // Add the local user prop pages
        psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_USERNAME_PROP_PAGE);
        m_pUsernamePage = new CUsernamePropertyPage(m_pUserInfo);
    
        if (m_pUsernamePage != NULL)
        {
            m_pUsernamePage->SetPropSheetPageMembers(&psp);
            lpfnAddPage(CreatePropertySheetPage(&psp), lParam);
        }
    }

    psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_CHOOSEGROUP_PROP_PAGE);
    m_pGroupPage = new CGroupPropertyPage(m_pUserInfo, &m_GroupList);
    
    if (m_pGroupPage != NULL)
    {
        m_pGroupPage->SetPropSheetPageMembers(&psp);
        lpfnAddPage(CreatePropertySheetPage(&psp), lParam);
    }

    TraceLeaveResult(hr);
}

HRESULT CUserPropertyPages ::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
{
    TraceEnter(TRACE_USR_COM, "CUserPropertyPages ::ReplacePage");

    TraceMsg("ERROR: CUserPropertyPages ::ReplacePage called but not implemented");

    TraceLeaveResult(E_NOTIMPL);
}


/***************************************
CUserPropertyPagesFactory Implementation
***************************************/

// IUnknown
HRESULT CUserPropertyPagesFactory::QueryInterface(REFIID iid, LPVOID* ppvOut)
{
    TraceEnter(TRACE_USR_COM, "CUserPropertyPagesFactory::CreateInstance");

    *ppvOut = NULL;
    HRESULT hr = E_NOINTERFACE;

    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
        *ppvOut = (IClassFactory*) this;
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        ((IClassFactory*) this)->AddRef();
    }

    TraceLeaveResult(hr);
}

// IClassFactory Implementation
HRESULT CUserPropertyPagesFactory::CreateInstance(IUnknown* pUnkOuter, REFIID iid, void** ppvObject)
{
    TraceEnter(TRACE_USR_COM, "CUserPropertyPagesFactory::CreateInstance");
    HRESULT hr = E_FAIL;

    // Expect no aggragation
    if (pUnkOuter != NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CUserPropertyPages * pPropSheets = new CUserPropertyPages ;

        if (pPropSheets == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = pPropSheets->QueryInterface(iid, ppvObject);

            pPropSheets->Release();
        }
    }

    TraceLeaveResult(hr);
}

HRESULT CUserPropertyPagesFactory::LockServer(BOOL fLock)
{
    TraceEnter(TRACE_USR_COM, "CUserPropertyPagesFactory::LockServer");

    if (fLock)
        InterlockedIncrement((long*) &g_cLocks);
    else
        InterlockedDecrement((long*) &g_cLocks);

    TraceLeaveResult(S_OK);
}

/***********************************************
 CUserSidDataObject Implementation

  The data object passed to extended user property
  pages (IShellPropSheetExt handlers)
***********************************************/

HRESULT CUserSidDataObject::QueryInterface(REFIID iid, LPVOID* ppvOut)
{
    TraceEnter(TRACE_USR_COM, "CUserSidDataObject::QueryInterface");
    *ppvOut = NULL;
    HRESULT hr = E_NOINTERFACE;

    if ((iid == IID_IUnknown) || (iid == IID_IDataObject))
    {
        *ppvOut = (IDataObject*) this;
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        ((IDataObject*) this)->AddRef();
    }

    TraceLeaveResult(hr);
}

HRESULT CUserSidDataObject::GetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium)
{
    TraceEnter(TRACE_USR_COM, "CUserSidDataObject::GetData");

    HRESULT hr = QueryGetData(pFormatEtc);

    if (SUCCEEDED(hr))
    {
        pMedium->pUnkForRelease = (IDataObject*) this;
        
        // Since pUnkForRelease is set, addref myself
        ((IDataObject*) this)->AddRef();

        pMedium->tymed = TYMED_HGLOBAL;
        pMedium->hGlobal = (HGLOBAL) m_psid;
    }

    TraceLeaveResult(hr);
}

HRESULT CUserSidDataObject::GetDataHere(FORMATETC* pFormatEtc, STGMEDIUM* pMedium)
{
    TraceEnter(TRACE_USR_COM, "CUserSidDataObject::GetDataHere");

    TraceLeaveResult(E_NOTIMPL);
}

HRESULT CUserSidDataObject::QueryGetData(FORMATETC* pFormatEtc)
{
    TraceEnter(TRACE_USR_COM, "CUserSidDataObject::QueryGetData");

    HRESULT hr;

    if (pFormatEtc == NULL)
        hr = E_INVALIDARG;
    else if ((pFormatEtc->cfFormat != RegisterClipboardFormat(CFSTR_USERPROPPAGESSID))
        || (pFormatEtc->ptd != NULL))
        hr = DV_E_FORMATETC;
    else if (pFormatEtc->lindex != -1)
        hr = DV_E_LINDEX;
    else if (pFormatEtc->tymed != TYMED_HGLOBAL)
        hr = DV_E_TYMED;
    else if (pFormatEtc->dwAspect != DVASPECT_CONTENT)
        hr = DV_E_DVASPECT;
    else
        hr = S_OK;
    
    TraceLeaveResult(hr);

}

HRESULT CUserSidDataObject::GetCanonicalFormatEtc(FORMATETC* pFormatetcIn, FORMATETC* pFormatetcOut)
{
    TraceEnter(TRACE_USR_COM, "CUserSidDataObject::GetCanonicalFormatEtc");

    TraceLeaveResult(E_NOTIMPL);
}    

HRESULT CUserSidDataObject::SetData(FORMATETC* pFormatetc, STGMEDIUM* pmedium, BOOL fRelease)
{
    TraceEnter(TRACE_USR_COM, "CUserSidDataObject::SetData");

    TraceLeaveResult(E_NOTIMPL);
}

HRESULT CUserSidDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC ** ppenumFormatetc)
{
    TraceEnter(TRACE_USR_COM, "CUserSidDataObject::EnumFormatEtc");

    TraceLeaveResult(E_NOTIMPL);
}

HRESULT CUserSidDataObject::DAdvise(FORMATETC* pFormatetc, DWORD advf, IAdviseSink* pAdvSink, DWORD * pdwConnection)
{
    TraceEnter(TRACE_USR_COM, "CUserSidDataObject::DAdvise");

    TraceLeaveResult(E_NOTIMPL);
}

HRESULT CUserSidDataObject::DUnadvise(DWORD dwConnection)
{
    TraceEnter(TRACE_USR_COM, "CUserSidDataObject::DUnadvise");

    TraceLeaveResult(E_NOTIMPL);
}

HRESULT CUserSidDataObject::EnumDAdvise(IEnumSTATDATA ** ppenumAdvise)
{
    TraceEnter(TRACE_USR_COM, "CUserSidDataObject::EnumDAdvise");

    TraceLeaveResult(E_NOTIMPL);
}

HRESULT CUserSidDataObject::SetSid(PSID psid)
{
    TraceEnter(TRACE_USR_COM, "CUserSidDataObject::SetSid");

    HRESULT hr = E_FAIL;

    if (m_psid == NULL)
    {
        DWORD cbSid = GetLengthSid(psid);
        m_psid = (PSID) LocalAlloc(0, cbSid);

        if (m_psid != NULL)
        {
            if (CopySid(cbSid, m_psid, psid))
            {
                hr = S_OK;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    TraceLeaveResult(hr);
}

CUserSidDataObject::~CUserSidDataObject()
{
    TraceEnter(TRACE_USR_COM, "CUserSidDataObject::~CUserSidDataObject");

    if (m_psid != NULL)
    {
        LocalFree(m_psid);
    }

    TraceLeaveVoid();
}
