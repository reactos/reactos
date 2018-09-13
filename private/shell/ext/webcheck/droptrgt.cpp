#include "private.h"
#include "offl_cpp.h"
#include "subsmgrp.h"

HRESULT _GetURLData(IDataObject *, int, TCHAR *, TCHAR *);
HRESULT _ConvertHDROPData(IDataObject *, BOOL);
HRESULT ScheduleDefault(LPCTSTR, LPCTSTR);

#define CITBDTYPE_HDROP     1
#define CITBDTYPE_URL       2
#define CITBDTYPE_TEXT      3

//
// Constructor
//

COfflineDropTarget::COfflineDropTarget(HWND hwndParent)
{
    m_cRefs                 = 1;
    m_hwndParent            = hwndParent;
    m_pDataObj              = NULL;
    m_grfKeyStateLast       = 0;
    m_fHasHDROP             = FALSE;
    m_fHasSHELLURL          = FALSE;
    m_fHasTEXT              = FALSE;
    m_dwEffectLastReturned  = 0;

    DllAddRef();
}

//
// Destructor
//

COfflineDropTarget::~COfflineDropTarget()
{
    DllRelease();
}

//
// QueryInterface
//

STDMETHODIMP COfflineDropTarget::QueryInterface(REFIID riid, LPVOID *ppv)
{
    HRESULT  hr = E_NOINTERFACE;

    *ppv = NULL;

    // Any interface on this object is the object pointer

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDropTarget))
    {
        *ppv = (LPDROPTARGET)this;

        AddRef();

        hr = NOERROR;
    }

    return hr;
}   

//
// AddRef
//

STDMETHODIMP_(ULONG) COfflineDropTarget::AddRef()
{
    return ++m_cRefs;
}

//
// Release
//

STDMETHODIMP_(ULONG) COfflineDropTarget::Release()
{
    if (0L != --m_cRefs)
    {
        return m_cRefs;
    }

    delete this;

    return 0L;
}

//
// DragEnter
//

STDMETHODIMP COfflineDropTarget::DragEnter(LPDATAOBJECT pDataObj, 
                                           DWORD        grfKeyState,
                                           POINTL       pt, 
                                           LPDWORD      pdwEffect)
{
    // Release any old data object we might have

//    TraceMsg(TF_SUBSFOLDER, TEXT("odt - DragEnter"));
    if (m_pDataObj)
    {
        m_pDataObj->Release();
    }

    m_grfKeyStateLast = grfKeyState;
    m_pDataObj        = pDataObj;

    //
    // See if we will be able to get CF_HDROP from this guy
    //

    if (pDataObj)
    {
        pDataObj->AddRef();
            TCHAR url[INTERNET_MAX_URL_LENGTH], name[MAX_NAME_QUICKLINK];
            FORMATETC fe = {(CLIPFORMAT) RegisterClipboardFormat(CFSTR_SHELLURL), 
                        NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

            m_fHasSHELLURL = m_fHasHDROP = m_fHasTEXT = FALSE;
            if (NOERROR == pDataObj->QueryGetData(&fe))
            {
                TraceMsg(TF_SUBSFOLDER, "odt - DragEnter : SHELLURL!");
                m_fHasSHELLURL = 
                    (NOERROR == _GetURLData(pDataObj,CITBDTYPE_URL,url,name));
            }
            if (fe.cfFormat = CF_HDROP, NOERROR == pDataObj->QueryGetData(&fe))
            {
                TraceMsg(TF_SUBSFOLDER, "odt - DragEnter : HDROP!");
                m_fHasHDROP = (NOERROR == 
                    _ConvertHDROPData(pDataObj, FALSE));
            }
            if (fe.cfFormat = CF_TEXT, NOERROR == pDataObj->QueryGetData(&fe))
            {
                TraceMsg(TF_SUBSFOLDER, "odt - DragEnter : TEXT!");
                m_fHasTEXT = 
                    (NOERROR == _GetURLData(pDataObj,CITBDTYPE_TEXT,url,name));
            }
    }

    // Save the drop effect

    if (pdwEffect)
    {
        *pdwEffect = m_dwEffectLastReturned = GetDropEffect(pdwEffect);
    }

    return S_OK;
}

//
// GetDropEffect
//

DWORD COfflineDropTarget::GetDropEffect(LPDWORD pdwEffect)
{
    ASSERT(pdwEffect);

    if (m_fHasSHELLURL || m_fHasTEXT)
    {
        return *pdwEffect & (DROPEFFECT_COPY | DROPEFFECT_LINK);
    }
    else if (m_fHasHDROP)    {
        return *pdwEffect & (DROPEFFECT_COPY );
    }
    else
    {
        return DROPEFFECT_NONE;
    }
}

//
// DragOver
//

STDMETHODIMP COfflineDropTarget::DragOver(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
//    TraceMsg(TF_SUBSFOLDER, TEXT("odt - DragOver"));
    if (m_grfKeyStateLast == grfKeyState)
    {
        // Return the effect we saved at dragenter time

        if (*pdwEffect)
        {
            *pdwEffect = m_dwEffectLastReturned;
        }
    }
    else
    {
        if (*pdwEffect)
        {
            *pdwEffect = m_dwEffectLastReturned = GetDropEffect(pdwEffect);
        }
    }

    m_grfKeyStateLast = grfKeyState;

    return S_OK;
}

//
// DragLeave
//
 
STDMETHODIMP COfflineDropTarget::DragLeave()
{
//    TraceMsg(TF_SUBSFOLDER, TEXT("odt - DragLeave"));
    if (m_pDataObj)
    {
        m_pDataObj->Release();
        m_pDataObj = NULL;
    }

    return S_OK;
}

//
// Drop
//
STDMETHODIMP COfflineDropTarget::Drop(LPDATAOBJECT pDataObj,
                                      DWORD        grfKeyState, 
                                      POINTL       pt, 
                                      LPDWORD      pdwEffect)
{
//    UINT idCmd;             // Choice from drop popup menu
    HRESULT hr = S_OK;
  
    //
    // Take the new data object, since OLE can give us a different one than
    // it did in DragEnter
    //

//    TraceMsg(TF_SUBSFOLDER, TEXT("odt - Drop"));
    if (m_pDataObj)
    {
        m_pDataObj->Release();
    }
    m_pDataObj = pDataObj;
    if (pDataObj)
    {
        pDataObj->AddRef();
    }

    // If the dataobject doesn't have an HDROP, its not much good to us

    *pdwEffect &= DROPEFFECT_COPY|DROPEFFECT_LINK; 
    if (!(*pdwEffect))  {
        DragLeave();        
        return S_OK;
    }


    hr = E_NOINTERFACE;
    if (m_fHasHDROP)
        hr = _ConvertHDROPData(pDataObj, TRUE);
    else  {
        TCHAR url[INTERNET_MAX_URL_LENGTH], name[MAX_NAME_QUICKLINK];
        if (m_fHasSHELLURL)
            hr = _GetURLData(pDataObj, CITBDTYPE_URL,   url, name);
        if (FAILED(hr) && m_fHasTEXT)
            hr = _GetURLData(pDataObj, CITBDTYPE_TEXT,  url, name);
        if (SUCCEEDED(hr))  {
            TraceMsg(TF_SUBSFOLDER, "URL: %s, Name: %s", url, name);  
            hr = ScheduleDefault(url, name);
        }
    }

    if (FAILED(hr))
    {
        TraceMsg(TF_SUBSFOLDER, "Couldn't DROP");
    }

    DragLeave();
    return hr;
}

HRESULT _CLSIDFromExtension(
    LPCTSTR pszExt, 
    CLSID *pclsid)
{
    TCHAR szProgID[80];
    long cb = SIZEOF(szProgID);
    if (RegQueryValue(HKEY_CLASSES_ROOT, pszExt, szProgID, &cb) == ERROR_SUCCESS)
    {
        TCHAR szCLSID[80];
        
        StrCatN(szProgID, TEXT("\\CLSID"), ARRAYSIZE(szProgID)); 
        cb = SIZEOF(szCLSID);

        if (RegQueryValue(HKEY_CLASSES_ROOT, szProgID, szCLSID, &cb) == ERROR_SUCCESS)
        {
            // BUGBUG (scotth): call shell32's SHCLSIDFromString once it
            //                  exports A/W versions.  This would clean this
            //                  up.
#ifdef UNICODE
            return CLSIDFromString(szCLSID, pclsid);
#else
            WCHAR wszCLSID[80];
            MultiByteToWideChar(CP_ACP, 0, szCLSID, -1, wszCLSID, ARRAYSIZE(wszCLSID));
            return CLSIDFromString(wszCLSID, pclsid);
#endif
        }
    }
    return E_FAIL;
}


// get the target of a shortcut. this uses IShellLink which 
// Internet Shortcuts (.URL) and Shell Shortcuts (.LNK) support so
// it should work generally
//
HRESULT _GetURLTarget(LPCTSTR pszPath, LPTSTR pszTarget, UINT cch)
{
    IShellLink *psl;
    HRESULT hr = E_FAIL;
    CLSID clsid;

    if (FAILED(_CLSIDFromExtension(PathFindExtension(pszPath), &clsid)))
        clsid = CLSID_ShellLink;        // assume it's a shell link

    hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&psl);

    if (SUCCEEDED(hr))
    {
        IPersistFile *ppf;

        hr = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
        if (SUCCEEDED(hr))
        {
#ifdef UNICODE
            hr = ppf->Load(pszPath, 0);
#else
            WCHAR wszPath[MAX_PATH];
            MultiByteToWideChar(CP_ACP, 0, pszPath, -1, wszPath, ARRAYSIZE(wszPath));

            hr = ppf->Load (wszPath, 0);
#endif
            ppf->Release();
        }
        if (SUCCEEDED(hr))  {
            IUniformResourceLocator * purl;

            hr = psl->QueryInterface(IID_IUniformResourceLocator,(void**)&purl);
            if (SUCCEEDED(hr))
                purl->Release();
        }
        if (SUCCEEDED(hr))
            hr = psl->GetPath(pszTarget, cch, NULL, SLGP_UNCPRIORITY);
        psl->Release();
    }
    return hr;
}

HRESULT _ConvertHDROPData(IDataObject *pdtobj, BOOL bSubscribe)
{
    HRESULT hRes = NOERROR;
    STGMEDIUM stgmedium;
    FORMATETC formatetc;
    TCHAR    url[INTERNET_MAX_URL_LENGTH];
    TCHAR    name[MAX_NAME_QUICKLINK];

    name[0] = 0;
    url[0] = 0;

    formatetc.cfFormat = CF_HDROP;
    formatetc.ptd = NULL;
    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.lindex = -1;
    formatetc.tymed = TYMED_HGLOBAL;

    // Get the parse string
    hRes = pdtobj->GetData(&formatetc, &stgmedium);
    if (SUCCEEDED(hRes))
    {
        LPTSTR pszURLData = (LPTSTR)GlobalLock(stgmedium.hGlobal);
        if (pszURLData) {
            TCHAR szPath[MAX_PATH];
            SHFILEINFO sfi;
            int cFiles, i;
            HDROP hDrop = (HDROP)stgmedium.hGlobal;

            hRes = S_FALSE;
            cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
            for (i = 0; i < cFiles; i ++)   {
                DragQueryFile(hDrop, i, szPath, SIZEOF(szPath));

                // defaults...
                StrCpyN(name, szPath, MAX_NAME_QUICKLINK);

                if (SHGetFileInfo(szPath, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME))
                    StrCpyN(name, sfi.szDisplayName, MAX_NAME_QUICKLINK);

                if (SHGetFileInfo(szPath, 0, &sfi, sizeof(sfi),SHGFI_ATTRIBUTES)
                        && (sfi.dwAttributes & SFGAO_LINK))
                {
                    if (SUCCEEDED(_GetURLTarget(szPath, url, INTERNET_MAX_URL_LENGTH)))
                    { 
                        TraceMsg(TF_SUBSFOLDER, "URL: %s, Name: %s", url, name);  
                        //  If we just want to see whether there is some urls
                        //  here, we can break now.
                        if (!bSubscribe)    
                        {
                            if ((IsHTTPPrefixed(url)) && 
                                (!SHRestricted2(REST_NoAddingSubscriptions, url, 0)))
                            {
                                hRes = S_OK;
                            }
                            break;
                        }
                        hRes = ScheduleDefault(url, name);
                    }
                }
            }
            GlobalUnlock(stgmedium.hGlobal);
            if (bSubscribe)
                hRes = S_OK;
        } else
            hRes = S_FALSE;

        ReleaseStgMedium(&stgmedium);
    }
    return hRes;
}
    
//  Takes a variety of inputs and returns a string for drop targets.
//  szUrl:    the URL
//  szName:   the name (for quicklinks and the confo dialog boxes)
//  returns:  NOERROR if succeeded
//
HRESULT _GetURLData(IDataObject *pdtobj, int iDropType, TCHAR *pszUrl, TCHAR *pszName)
{
    HRESULT hRes = NOERROR;
    STGMEDIUM stgmedium;
    FORMATETC formatetc;

    *pszName = 0;
    *pszUrl = 0;

    switch (iDropType)
    {
    case CITBDTYPE_URL:
        formatetc.cfFormat = (CLIPFORMAT) RegisterClipboardFormat(CFSTR_SHELLURL);
        break;
    case CITBDTYPE_TEXT:
        formatetc.cfFormat = CF_TEXT;
        break;
    default:
        return E_UNEXPECTED;
    }
    formatetc.ptd = NULL;
    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.lindex = -1;
    formatetc.tymed = TYMED_HGLOBAL;

    // Get the parse string
    hRes = pdtobj->GetData(&formatetc, &stgmedium);
    if (SUCCEEDED(hRes))
    {
        LPTSTR pszURLData = (LPTSTR)GlobalLock(stgmedium.hGlobal);
        if (pszURLData)
        {
            if (iDropType == CITBDTYPE_URL)
            {
                STGMEDIUM stgmediumFGD;

                 // defaults
                StrCpyN(pszUrl,  pszURLData, INTERNET_MAX_URL_LENGTH);
                StrCpyN(pszName, pszURLData, MAX_NAME_QUICKLINK);

                formatetc.cfFormat = (CLIPFORMAT) RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
                if (SUCCEEDED(pdtobj->GetData(&formatetc, &stgmediumFGD)))
                {
                    FILEGROUPDESCRIPTOR *pfgd = (FILEGROUPDESCRIPTOR *)GlobalLock(stgmediumFGD.hGlobal);
                    if (pfgd)
                    {
                        TCHAR szPath[MAX_PATH];
                        StrCpyN(szPath, pfgd->fgd[0].cFileName, SIZEOF(szPath));
                        PathRemoveExtension(szPath);
                        StrCpyN(pszName, szPath, MAX_NAME_QUICKLINK);
                        GlobalUnlock(stgmediumFGD.hGlobal);
                    }
                    ReleaseStgMedium(&stgmediumFGD);
                }
            }
            else if (iDropType == CITBDTYPE_TEXT)
            {
                if (PathIsURL(pszURLData))  {
                    StrCpyN(pszUrl, pszURLData, INTERNET_MAX_URL_LENGTH);
                    StrCpyN(pszName, pszURLData, MAX_NAME_QUICKLINK);
                } else
                    hRes = E_FAIL;
            }
            GlobalUnlock(stgmedium.hGlobal);
        }
        ReleaseStgMedium(&stgmedium);
    }

    if (SUCCEEDED(hRes))
    {
        if (!IsHTTPPrefixed(pszUrl) || SHRestricted2(REST_NoAddingSubscriptions, pszUrl, 0))
        {
            hRes = E_FAIL;
        }
    }
    return hRes;
}

HRESULT ScheduleDefault(LPCTSTR url, LPCTSTR name)
{
    if (!IsHTTPPrefixed(url))
        return E_INVALIDARG;

    ISubscriptionMgr    * pSub= NULL;
    HRESULT hr = CoInitialize(NULL);
    RETURN_ON_FAILURE(hr);

    hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER, 
                IID_ISubscriptionMgr, (void **)&pSub);
    CoUninitialize();
    RETURN_ON_FAILURE(hr);
    ASSERT(pSub);

    BSTR    bstrURL = NULL, bstrName = NULL;
    hr = CreateBSTRFromTSTR(&bstrURL, url);
    if (S_OK == hr)
        hr = CreateBSTRFromTSTR(&bstrName, name);

    //  BUGBUG. We need a perfectly valid structure.
    SUBSCRIPTIONINFO    subInfo;
    ZeroMemory((void *)&subInfo, sizeof (SUBSCRIPTIONINFO));
    subInfo.cbSize = sizeof(SUBSCRIPTIONINFO);

    if (S_OK == hr)
        hr = pSub->CreateSubscription(NULL, bstrURL, bstrName,
                                      CREATESUBS_NOUI, SUBSTYPE_URL, &subInfo);

    SAFERELEASE(pSub);
    SAFEFREEBSTR(bstrURL);
    SAFEFREEBSTR(bstrName);

    if (FAILED(hr)) {
        TraceMsg(TF_ALWAYS, "Failed to add default object.");
        TraceMsg(TF_ALWAYS, "  hr = 0x%x\n", hr);
        return (FAILED(hr))?hr:E_FAIL;
    } else if (hr == S_FALSE)   {
        TraceMsg(TF_SUBSFOLDER, "%s(%s) is already there.", url, name);
        return hr;
    }

    return S_OK;
}

