#include <windows.h>

#define CPP_FUNCTIONS
#include "crtfree.h"
#include "shlobj.h"
#include "shsemip.h"
#include "shellp.h"

#include "sfvlist.h"
#include "rnaguidp.h"

//#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
//#define SIZEOF(a) sizeof(a)

HRESULT CUnkNoRef::QueryInterface(REFIID riid, LPVOID * ppvObj,
        const IID* const priid[], const LPUNKNOWN ppObj[], REFIID riidHack)
{
    *ppvObj = NULL;

    IUnknown* pObj;
    IUnknown* pObjRef = ppObj[0];

    if (riid == IID_IUnknown)
    {
        pObj = pObjRef;
    }
    else
    {
        for (int i=0; ; ++i)
        {
            if (!priid[i])
            {
                return(E_NOINTERFACE);
            }

            if (*priid[i] == riid)
            {
                pObj = ppObj[i];

                // riidHack is the "this" pointer, so we can't AddRef() it,
                // instead we AddRef the pObjRef. we really want to call the
                // correct AddRef function when we can so function tracting works.
                if (riid != riidHack)
                {
                    pObjRef = pObj;
                }
                break;
            }
        }
    }

    // Always AddRef the first object so that we can put the "this" pointer
    // in the list
    pObjRef->AddRef();
    *ppvObj = pObj;

    return(NOERROR);
}


ULONG CUnknown::AddRef()
{
    m_cRef++;

    return(m_cRef);
}


ULONG CUnknown::Release()
{
    ULONG ul = --m_cRef;

    if (!ul)
    {
        delete this;
    }

    return(ul);
}


UINT wstrlen(LPCWSTR pwsz)
{
    for (int i=0; *pwsz; ++i, ++pwsz)
    {
        // Just loop
    }

    return(i);
}


int CGenList::Add(LPVOID pv)
{
    if (!m_hList)
    {
        m_hList = DSA_Create(m_cbItem, 8);
        if (!m_hList)
        {
            return(-1);
        }
    }

    return(DSA_AppendItem(m_hList, pv));
}


int CViewsList::Add(const SFVVIEWSDATA*pView, BOOL bCopy)
{
    if (bCopy)
    {
        pView = CopyData(pView);
        if (!pView)
        {
            return(-1);
        }
    }

    int iIndex = CGenList::Add((LPVOID)(&pView));

    if (bCopy && iIndex<0)
    {
        SHFree((LPVOID)pView);
    }

    return(iIndex);
}


TCHAR const c_szExtViews[] = TEXT("ExtShellFolderViews");
TCHAR const c_szWinDir[] = TEXT("%WinDir%");


void CViewsList::AddReg(HKEY hkParent, LPCTSTR pszSubKey)
{
    CRegKey ckClass(hkParent, pszSubKey);
    if (!ckClass)
    {
        return;
    }

    CRegKey ckShellEx(ckClass, TEXT("shellex"));
    if (!ckShellEx)
    {
        return;
    }

    CRegKey ckViews(ckShellEx, c_szExtViews);
    if (!ckViews)
    {
        return;
    }

    TCHAR szKey[40];
    DWORD dwLen = SIZEOF(szKey);
    SHELLVIEWID vid;

    if (ERROR_SUCCESS==RegQueryValue(ckViews, NULL, szKey, (LONG*)&dwLen)
        && SUCCEEDED(SHCLSIDFromString(szKey, &vid)))
    {
        m_vidDef = vid;
        m_bGotDef = TRUE;
    }


    for (int i=0; ; ++i)
    {
        LONG lRet = RegEnumKey(ckViews, i, szKey, ARRAYSIZE(szKey));
        if (lRet == ERROR_MORE_DATA)
        {
            continue;
        }
        else if (lRet != ERROR_SUCCESS)
        {
            // I assume this is ERROR_NO_MORE_ITEMS
            break;
        }

        SFVVIEWSDATA sView;
        ZeroMemory(&sView, SIZEOF(sView));

        if (FAILED(SHCLSIDFromString(szKey, &sView.idView)))
        {
            continue;
        }

        CRegKey ckView(ckViews, szKey);
        if (ckView)
        {
            TCHAR szFile[ARRAYSIZE(sView.wszMoniker)];
            DWORD dwType;

            dwLen = SIZEOF(sView.dwFlags);
            if ((ERROR_SUCCESS!=RegQueryValueEx(ckView, TEXT("Attributes"),
                    NULL, &dwType, (LPBYTE)&sView.dwFlags, &dwLen))
                || dwLen != SIZEOF(sView.dwFlags)
                || !(REG_DWORD==dwType || REG_BINARY==dwType))
            {
                sView.dwFlags = 0;
            }

            //
            // Checking for PersistFile and PersistString separately is for clarity
            // in the registry. The code doesn't pay attention to this difference.
            //
            dwLen = SIZEOF(szFile);
            if ((ERROR_SUCCESS==RegQueryValueEx(ckView, TEXT("PersistMoniker"),
                    NULL, &dwType, (LPBYTE)szFile, &dwLen))
                && REG_SZ==dwType)
            {
                StrToOleStr(sView.wszMoniker, szFile);
            }
            else
            {
                continue;
            }

            dwLen = SIZEOF(sView.lParam);
            RegQueryValueEx(ckView, TEXT("LParam"),NULL, &dwType,
                (LPBYTE) &(sView.lParam), &dwLen );
        }
        
        Add(&sView);
    }
}


void CViewsList::AddCLSID(CLSID const* pclsid)
{
    CRegKey ckCLSID(HKEY_CLASSES_ROOT, TEXT("CLSID"));
    if (!ckCLSID)
    {
        return;
    }

    WCHAR szCLSID[40];
    StringFromGUID2(*pclsid, szCLSID, ARRAYSIZE(szCLSID));

    TCHAR szCLSIDA[40];
    LPTSTR pszCLSID;
#ifdef UNICODE
    pszCLSID = szCLSID;
#else
    WideCharToMultiByte(CP_ACP, 0, szCLSID, ARRAYSIZE(szCLSID), szCLSIDA, ARRAYSIZE(szCLSIDA), NULL, NULL);
    pszCLSID = szCLSIDA;
#endif;

    AddReg(ckCLSID, pszCLSID);
}


void CViewsList::Empty()
{
    m_bGotDef = FALSE;

    for (int i=GetItemCount()-1; i>=0; --i)
    {
        SHFree(GetPtr(i));
    }

    CGenList::Empty();
}


SFVVIEWSDATA* CViewsList::CopyData(const SFVVIEWSDATA* pData)
{
    SFVVIEWSDATA* pCopy = (SFVVIEWSDATA*)SHAlloc(SIZEOF(SFVVIEWSDATA));
    if (pCopy)
    {
        memcpy(pCopy, pData, SIZEOF(SFVVIEWSDATA));
    }
    return(pCopy);
}


class CEnumCViewList : public IEnumSFVViews, private CViewsList, private CUnknown
{
public:
    CEnumCViewList(CViewsList* pViews);
    virtual ~CEnumCViewList() {}

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG, AddRef)() {return(CUnknown::AddRef());}
    STDMETHOD_(ULONG, Release)() {return(CUnknown::Release());}

    // *** IEnumSFVViews methods ***
    STDMETHOD(Next)  (THIS_ ULONG celt,
                      SFVVIEWSDATA **ppData,
                      ULONG *pceltFetched);
    STDMETHOD(Skip)  (THIS_ ULONG celt) {return(E_NOTIMPL);}
    STDMETHOD(Reset) (THIS) {m_uNext=0; return(NOERROR);}
    STDMETHOD(Clone) (THIS_ IEnumSFVViews **ppenum) {return(E_FAIL);}

private:
    UINT m_uNext;
} ;

CEnumCViewList::CEnumCViewList(CViewsList*pList) : m_uNext(0)
{
    Steal(pList);
}


STDMETHODIMP CEnumCViewList::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    IUnknown* apObj[] =
    {
        (IEnumSFVViews*)this,
    } ;
    static const IID* const c_ariid[ARRAYSIZE(apObj)+1] =
    {
        &IID_IEnumSFVViews,
        NULL,
    };

    return(CUnknown::QueryInterface(riid, ppvObj, c_ariid, apObj, IID_IUnknown));
}


STDMETHODIMP CEnumCViewList::Next  (THIS_ ULONG celt,
                      SFVVIEWSDATA **ppData,
                      ULONG *pceltFetched)
{
    *ppData = NULL;

    if (pceltFetched)
    {
        *pceltFetched = 0;
    }

    if (celt < 1)
    {
        return(E_INVALIDARG);
    }

    if (m_uNext >= GetItemCount())
    {
        return(S_FALSE);
    }

    *ppData = CopyData(GetPtr(m_uNext++));
    if (!*ppData)
    {
        return(E_OUTOFMEMORY);
    }

    if (pceltFetched)
    {
        *pceltFetched = 1;
    }
    return(S_OK);
}


HRESULT CreateEnumCViewList(CViewsList *pViews, IEnumSFVViews **ppObj)
{
    CEnumCViewList* peViews = new CEnumCViewList(pViews);

    if (!peViews)
    {
        return(E_OUTOFMEMORY);
    }

    *ppObj = peViews;

    return(NOERROR);
}


HRESULT PASCAL RemView_OnGetViews(
    SHELLVIEWID FAR *pvid,
    IEnumSFVViews FAR * FAR *ppObj)
{
    *ppObj = NULL;

    CViewsList cViews;

    // Add base class stuff
    cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("folder"));

    cViews.AddCLSID(&CLSID_Remote);

    cViews.GetDef(pvid);

    return(CreateEnumCViewList(&cViews, ppObj));

    // Note the automatic destructor will free any views still left
}

