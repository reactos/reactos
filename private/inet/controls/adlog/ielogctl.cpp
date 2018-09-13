// IeLogCtl.cpp : Implementation of CIeLogControl
#include "stdafx.h"
#include "ImgLog.h"
#include "IeLogCtl.h"

#include "wininet.h"
#include "urlmon.h"
#include "utils.h"

/////////////////////////////////////////////////////////////////////////////
// CIeLogControl
STDMETHODIMP CIeLogControl::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
{
    USES_CONVERSION;
    VARIANT v;
    HRESULT hr;

    VariantInit(&v);
    v.vt = VT_I2;
    hr = pPropBag->Read(L"images", &v, pErrorLog);
    if (SUCCEEDED(hr))
        m_nImages = v.iVal;
    VariantClear(&v);
    
    char    sz[10];
    for (int i=1; i<=m_nImages; i++)
    {
        litoa(i, sz);
        VariantInit(&v);
        v.vt = VT_BSTR;
        v.bstrVal = ::SysAllocString(L"");
        hr = pPropBag->Read(T2OLE(sz), &v, pErrorLog);
        if (!FAILED(hr)) 
        {
            BuildImageList(sz, OLE2T(v.bstrVal));
        }
        VariantClear(&v);
    }

    GetURLFromMoniker();
    return S_OK;        
}

STDMETHODIMP CIeLogControl::Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    return S_OK;
}

STDMETHODIMP CIeLogControl::Close(DWORD dwSaveOption)
{
    if (m_spClientSite)
    {
        if (!m_pszURL)
            GetURLFromMoniker();
    }

    return S_OK;
}

STDMETHODIMP CIeLogControl::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IIeLogControl,
    };
    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

HRESULT CIeLogControl::OnDraw(ATL_DRAWINFO& di)
{
    RECT& rc = *(RECT*)di.prcBounds;
    Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);
    DrawText(di.hdcDraw, _T("ATL 2.0 ImgLog Control"), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    return S_OK;
}

STDMETHODIMP CIeLogControl::get_images(short * pVal)
{
    *pVal = m_nImages;
    return S_OK;
}

STDMETHODIMP CIeLogControl::put_images(short newVal)
{
    if (newVal < 0 || newVal > 10)
        return Error(_T("Invalid number of images"));
    else
    {
        m_nImages = newVal;
        FireViewChange();
        return S_OK;
    }
}

STDMETHODIMP CIeLogControl::put_URL(BSTR newVal)
{
    USES_CONVERSION;
    return put_URL(OLE2T(newVal));
}

HRESULT CIeLogControl::put_URL(LPSTR newVal)
{
    if (m_pszURL)
    {
        delete [] m_pszURL;
        m_pszURL = NULL;
    }

    DWORD dwLength = MAX_PATH;
    m_pszURL = new char[dwLength+1];
    if (!m_pszURL)
        return E_OUTOFMEMORY;
                                                
    if (!InternetCanonicalizeUrl(newVal, m_pszURL, &dwLength,0))
    {
        delete [] m_pszURL;
        m_pszURL = NULL;
        return E_FAIL;
    }
    return S_OK;
}

STDMETHODIMP CIeLogControl::put_msover(BSTR bstrVal)
{
    USES_CONVERSION;
    UpdateMouseEventOnObject(OLE2T(bstrVal), IELOG_EVENT_MOUSEOVER);
    return S_OK;
}

STDMETHODIMP CIeLogControl::put_msclick(BSTR bstrVal)
{
    USES_CONVERSION;
    UpdateMouseEventOnObject(OLE2T(bstrVal), IELOG_EVENT_MOUSECLICK);
    return S_OK;
}

STDMETHODIMP CIeLogControl::ImageList(BSTR bstrKey, BSTR bstrVal)
{
    USES_CONVERSION;
    BuildImageList(OLE2T(bstrKey), OLE2T(bstrVal));
    return S_OK;
}

HRESULT CIeLogControl::UpdateMouseEventOnObject(LPTSTR psz, IELOG_EVENTS ievent)
{
    // if it hasn't changed, don't waste any time.
    //
    if (!psz || !*psz || !m_ImageList)
        return S_OK;

    IELog_ImageList*    pthis = m_ImageList;
    
    do
    {
        if( !lstrcmp(pthis->key, psz) )
        {
            pthis->evts = ievent;
            break;
        }
        
        pthis = pthis->next;    
    }
    while (pthis);
    
    return S_OK;
}


HRESULT CIeLogControl::BuildImageList(LPTSTR key, LPTSTR value)
{
    IELog_ImageList    *pTemp, *pNew = NULL;
    
    pNew = (IELog_ImageList *)LocalAlloc(LPTR, sizeof(IELog_ImageList));
    if (!pNew)
        return E_OUTOFMEMORY;

    if (!m_ImageList)
    {
        m_ImageList = pNew;
    }
    else
    {
        for (pTemp = m_ImageList; pTemp->next; pTemp = pTemp->next)
        {
            if (!lstrcmp(pTemp->key, key))
            {
                LocalFree(pNew);
                return S_OK;
            }
        }
        pTemp->next = pNew;

    }
    
    pNew->key = (LPTSTR)LocalAlloc(LPTR, lstrlen(key)+1);
    pNew->value = (LPTSTR)LocalAlloc(LPTR, lstrlen(value)+1);
    if (!pNew->key || !pNew->value)
        return E_OUTOFMEMORY;
        
    lstrcpy( pNew->key, key);
    lstrcpy( pNew->value, value);
    pNew->evts = (IELOG_EVENTS)0;
    pNew->next = NULL;
    
    m_lsize += lstrlen(key) + lstrlen(value) + 3;

    return S_OK;
}

const TCHAR INFO_CLICK[] = TEXT("1");
const TCHAR INFO_OVER[]  = TEXT("2");
const TCHAR INFO_EQUAL[] = TEXT("=");
const TCHAR INFO_PLUS[]  = TEXT("+");

const TCHAR WARNOVERFLOW[] = TEXT("Client%20buffer%20overflow>512%20bytes");

HRESULT CIeLogControl::SaveLogs()
{
    IELog_ImageList* pImg;
    BOOL             bRet;
    HIT_LOGGING_INFO pLi;

    if (!m_ImageList)
        return S_OK;    // nothing to write

    if (!IsLoggingEnabled(m_pszURL))
        return S_OK;

    pLi.lpszLoggedUrlName = new char[MAX_PATH+1];
    if (!pLi.lpszLoggedUrlName)
        return S_OK;
    lstrcpy(pLi.lpszLoggedUrlName, m_pszURL);
    
    pLi.lpszExtendedInfo = new char[m_lsize+1];
    if (!pLi.lpszExtendedInfo)
        return S_OK;
    lstrcpy(pLi.lpszExtendedInfo, "");
    
    pLi.StartTime.wYear = m_timeEnter.wYear;
    pLi.StartTime.wMonth = m_timeEnter.wMonth;
    pLi.StartTime.wDayOfWeek = m_timeEnter.wDayOfWeek;
    pLi.StartTime.wDay = m_timeEnter.wDay;
    pLi.StartTime.wHour = m_timeEnter.wHour;
    pLi.StartTime.wMinute = m_timeEnter.wMinute;
    pLi.StartTime.wSecond = m_timeEnter.wSecond;
    pLi.StartTime.wMilliseconds = m_timeEnter.wMilliseconds;
    
    pLi.dwStructSize = sizeof(HIT_LOGGING_INFO);
    GetLocalTime(&(pLi.EndTime));

    // protect buffer overflow from logging API
    if (m_lsize > 480)        // the limit of buffer can append is (512 - 32) bytes
    {
        lstrcpy(pLi.lpszExtendedInfo, WARNOVERFLOW);
    }
    else
    {
        pImg = m_ImageList; 
        while (pImg)
        {
            if (pImg->value)
                lstrcat(pLi.lpszExtendedInfo, pImg->value);
            if (pImg->evts)
            {
                lstrcat(pLi.lpszExtendedInfo, INFO_EQUAL);
                if (pImg->evts == IELOG_EVENT_MOUSECLICK)
                    lstrcat(pLi.lpszExtendedInfo, INFO_CLICK);
                if (pImg->evts == IELOG_EVENT_MOUSEOVER)
                    lstrcat(pLi.lpszExtendedInfo, INFO_OVER);
            }
        
            if (pImg->next)
                lstrcat(pLi.lpszExtendedInfo, INFO_PLUS);
            
            pImg = pImg->next;

        };
    }

    bRet = WriteHitLogging(&pLi);
        
    delete [] pLi.lpszLoggedUrlName;
    delete [] pLi.lpszExtendedInfo;

    return S_OK;
}

HRESULT CIeLogControl::GetURLFromMoniker()
{
    USES_CONVERSION;
    IMoniker    *pMk = NULL;

    if (SUCCEEDED(m_spClientSite->GetMoniker(OLEGETMONIKER_TEMPFORUSER, OLEWHICHMK_CONTAINER, &pMk)))
    {
        LPOLESTR    wszURL = NULL;
                
        if (SUCCEEDED(pMk->GetDisplayName(NULL, NULL, &wszURL)))
        {
            put_URL(OLE2T(wszURL));
        }
                    
        pMk->Release();
        CoTaskMemFree(wszURL);
        return S_OK;
    }

    return E_FAIL;
}

void CIeLogControl::Cleanup()
{
    if (m_pszURL)
    {
        delete [] m_pszURL;
        m_pszURL = NULL;
    }

    if (m_ImageList)
    {
        IELog_ImageList    *pTemp;
        do {

            pTemp = m_ImageList;
            LocalFree( pTemp->key );
            LocalFree( pTemp->value );
            m_ImageList = pTemp->next;
            LocalFree(pTemp);

        } while (m_ImageList);
    }
}


