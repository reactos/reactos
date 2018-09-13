#include "shellprv.h"
#include <shellp.h>
#include "ole2dup.h"
#include "defview.h"
#include "bookmk.h"
#include <webvw.h>

#include <sfview.h>
#include "sfviewp.h"
#include "ids.h"
#include <htiface.h>
#include <olectl.h>
#include "sfvext.h"
#include "mshtml.h"
#include <mshtmdid.h>
#include <shguidp.h>    // get the CLSID definitions, the bits are built into shguidp.lib

#define TF_FOCUS    TF_ALLOC

void DSV_SetFolderColors(CDefView *pdsv);

const DISPPARAMS c_dispparamsNoArgs = {NULL, NULL, 0, 0};
#define g_dispparamsNoArgs ((DISPPARAMS)c_dispparamsNoArgs)

// This value tells the default size of our DefView DPA
// It should be whatever the normal number of OCs our
// template authors use.
#define MAX_OCXS_PER_PAGE  1


class CEnsureSHFree
{
public:
    CEnsureSHFree(LPVOID pObj) : m_pObj(pObj) {}
    ~CEnsureSHFree() { if (m_pObj) SHFree(m_pObj); }

private:
    LPVOID m_pObj;
} ;
#define ENSURESHFREE(_pobj) CEnsureSHFree esf_##_pobj((LPVOID)_pobj)

int CGenList::Add(LPVOID pv, int nInsert)
{
    if (!m_hList)
    {
        m_hList = DSA_Create(m_cbItem, 8);
        if (!m_hList)
        {
            return(-1);
        }
    }

    return(DSA_InsertItem(m_hList, nInsert, pv));
}


int CViewsList::Add(const SFVVIEWSDATA*pView, int nInsert, BOOL bCopy)
{
    if (bCopy)
    {
        pView = CopyData(pView);
        if (!pView)
        {
            return(-1);
        }
    }

    int iIndex = CGenList::Add((LPVOID)(&pView), nInsert);

    if (bCopy && iIndex<0)
    {
        SHFree((LPVOID)pView);
    }

    return(iIndex);
}


TCHAR const c_szExtViews[] = TEXT("ExtShellFolderViews");
void CViewsList::AddReg(HKEY hkParent, LPCTSTR pszSubKey)
{
    CSHRegKey ckClass(hkParent, pszSubKey);
    if (!ckClass)
    {
        return;
    }

    CSHRegKey ckShellEx(ckClass, TEXT("shellex"));
    if (!ckShellEx)
    {
        return;
    }

    CSHRegKey ckViews(ckShellEx, c_szExtViews);
    if (!ckViews)
    {
        return;
    }

    TCHAR szKey[40];
    DWORD dwLen = SIZEOF(szKey);
    SHELLVIEWID vid;

    if (ERROR_SUCCESS==SHRegQueryValue(ckViews, NULL, szKey, (LONG*)&dwLen)
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

        CSHRegKey ckView(ckViews, szKey);
        if (ckView)
        {
            TCHAR szFile[ARRAYSIZE(sView.wszMoniker)];
            DWORD dwType;

            // NOTE: This app "Nuts&Bolts" munges the registry and remove the last NULL byte
            // from the PersistMoniker string. When we read that string into un-initialized 
            // local buffer, we do not get a properly null terminated string and we fail to 
            // create the moniker. So, I zero Init the mem here.
            ZeroMemory(szFile, sizeof(szFile));
            
            // Attributes affect all extended views
            dwLen = SIZEOF(sView.dwFlags);
            if ((ERROR_SUCCESS != SHQueryValueEx(ckView, TEXT("Attributes"),
                    NULL, &dwType, &sView.dwFlags, &dwLen))
                || dwLen != SIZEOF(sView.dwFlags)
                || !(REG_DWORD==dwType || REG_BINARY==dwType))
            {
                sView.dwFlags = 0;
            }

            // We either have a PersistMoniker (docobj) extended view
            // or we have an IShellView extended view
            //
            dwLen = SIZEOF(szFile);
            if (ERROR_SUCCESS == SHQueryValueEx(ckView, TEXT("PersistMoniker"),
                    NULL, &dwType, szFile, &dwLen) && REG_SZ == dwType)
            {
                //if the %UserAppData% exists, expand it!
                ExpandOtherVariables(szFile, ARRAYSIZE(szFile));
                SHTCharToUnicode(szFile, sView.wszMoniker, ARRAYSIZE(sView.wszMoniker));
            }
            else
            {
                dwLen = SIZEOF(szKey);
                if (ERROR_SUCCESS == SHQueryValueEx(ckView, TEXT("ISV"),
                    NULL, &dwType, szKey, &dwLen) && REG_SZ == dwType
                    && SUCCEEDED(SHCLSIDFromString(szKey, &vid)))
                {
                    sView.idExtShellView = vid;

                    // Only IShellView extended vies use LParams
                    dwLen = SIZEOF(sView.lParam);
                    if ((ERROR_SUCCESS != SHQueryValueEx(ckView, TEXT("LParam"),
                            NULL, &dwType, &sView.lParam, &dwLen))
                        || dwLen != SIZEOF(sView.lParam)
                        || !(REG_DWORD==dwType || REG_BINARY==dwType))
                    {
                        sView.lParam = 0;
                    }
        
                }
                else
                {
                    if (VID_FolderState != sView.idView)
                    {
                        // No moniker, no IShellView extension, this must be a VID_FolderState
                        // kinda thing. (Otherwise it's a bad desktop.ini.)
                        //
                        RIPMSG(0, "Extended view is registered incorrectly.");
                        continue;
                    }
                }
            }

            // It has been requested (by OEMs) to allow specifying background
            // bitmap and text colors for the regular views. That way they could
            // brand the Control Panel page by putting their logo recessed on
            // the background. We'd do that here by pulling the stuff out of
            // the registry and putting it in the LPCUSTOMVIEWSDATA section...
        }

        // if docobjextended view that is not webview, DO NOT ADD IT, UNSUPPORTED.
        if (!(sView.dwFlags & SFVF_NOWEBVIEWFOLDERCONTENTS)
                && !IsEqualGUID(sView.idView, VID_WebView))
            continue;
        Add(&sView);
    }
}


void CViewsList::AddCLSID(CLSID const* pclsid)
{
    CSHRegKey ckCLSID(HKEY_CLASSES_ROOT, c_szCLSID);
    if (!ckCLSID)
    {
        return;
    }

    TCHAR szCLSID[40];
    SHStringFromGUID(*pclsid, szCLSID, ARRAYSIZE(szCLSID));

    AddReg(ckCLSID, szCLSID);
}

#ifdef DEBUG
//In debug, I want to see if all the realloc code-path works fine!
//So, I deliberately alloc very small amounts.
#define CUSTOM_INITIAL_ALLOC      16
#define CUSTOM_REALLOC_INCREMENT  16
#else
#define CUSTOM_INITIAL_ALLOC      20*64
#define CUSTOM_REALLOC_INCREMENT  512
#endif //DEBUG

//Returns the offset of the string read into the block of memory (in chars)
// NOTE: the index (piCurOffset) and the sizes *piTotalSize, *piSizeRemaining are 
// NOTE: in WCHARs, not BYTES
int GetCustomStrData(LPWSTR *pDataBegin, int *piSizeRemaining, int *piTotalSize, 
                       int *piCurOffset, LPCTSTR szSectionName, LPCTSTR szKeyName, 
                       LPCTSTR szIniFile, LPCTSTR lpszPath)
{
    TCHAR szStrData[INFOTIPSIZE], szTemp[INFOTIPSIZE];
#ifndef UNICODE
    WCHAR wszStrData[MAX_PATH];
#endif
    LPWSTR pszStrData;
    int iLen, iOffsetBegin = *piCurOffset;

    //See if the data is present.
    if(!SHGetIniString(szSectionName, szKeyName, szTemp, ARRAYSIZE(szTemp), szIniFile))
    {
        return(-1);  //The given data is not present.
    }

    SHExpandEnvironmentStrings(szTemp, szStrData, ARRAYSIZE(szStrData));   // Expand the env vars if any
    
    //Get the full pathname if required.
    if(lpszPath)
        PathCombine(szStrData, lpszPath, szStrData);

#ifdef UNICODE
    iLen = lstrlen(szStrData);
    pszStrData = szStrData;
#else
    iLen = MultiByteToWideChar(CP_ACP, 0, szStrData, -1, wszStrData, ARRAYSIZE(wszStrData));
    pszStrData = wszStrData;
#endif
    iLen++;   //Include the NULL character.

    while(*piSizeRemaining < iLen)
    {
        LPWSTR lpNew;
        //We need to realloc the block of memory
        if(NULL == (lpNew = (LPWSTR)SHRealloc(*pDataBegin, ( *piTotalSize + CUSTOM_REALLOC_INCREMENT) * sizeof(WCHAR))))
            return(-1);  //Unable to realloc; out of mem.
        
        //Note: The begining address of the block could have changed.
        *pDataBegin = lpNew;
        *piTotalSize += CUSTOM_REALLOC_INCREMENT;
        *piSizeRemaining += CUSTOM_REALLOC_INCREMENT;
    }

    //Add the current directory if required.
    StrCpyW((*pDataBegin)+(*piCurOffset), pszStrData);

    *piSizeRemaining -= iLen;
    *piCurOffset += iLen;

    return(iOffsetBegin);
}

HRESULT ReadWebViewTemplate(LPCTSTR pszPath, LPTSTR pszWebViewTemplate, int cchWebViewTemplate)
{
    SHFOLDERCUSTOMSETTINGS fcs = {sizeof(fcs), FCSM_WEBVIEWTEMPLATE, 0};
    fcs.pszWebViewTemplate = pszWebViewTemplate;   // template path
    fcs.cchWebViewTemplate = cchWebViewTemplate;
    return SHGetSetFolderCustomSettings(&fcs, pszPath, FCS_READ);
}


const LPCTSTR c_szExtViewUIRegKeys[ID_EXTVIEWUICOUNT] =
{
    TEXT("MenuName"),
    TEXT("HelpText"),
    TEXT("TooltipText"),
    TEXT("IconArea_Image"),
    TEXT("IconArea_TextBackground"),
    TEXT("IconArea_Text")
};

void CViewsList::AddIni(LPCTSTR szIniFile, LPCTSTR szPath)
{
    TCHAR szViewIDs[12*45];  // Room for about 12 GUIDs including Default=
    SHELLVIEWID vid;

    //
    //First check if the INI file exists before trying to get data from it.
    //
    if (!PathFileExistsAndAttributes(szIniFile, NULL))
        return;

    if (GetPrivateProfileString(c_szExtViews, TEXT("Default"), c_szNULL,
        szViewIDs, ARRAYSIZE(szViewIDs), szIniFile)
        && SUCCEEDED(SHCLSIDFromString(szViewIDs, &vid)))
    {
        m_vidDef = vid;
        m_bGotDef = TRUE;
    }

    GetPrivateProfileString(c_szExtViews, NULL, c_szNULL,
        szViewIDs, ARRAYSIZE(szViewIDs), szIniFile);

    for (LPCTSTR pNextID=szViewIDs; *pNextID; pNextID+=lstrlen(pNextID)+1)
    {
        SFVVIEWSDATA sViewData;
        CUSTOMVIEWSDATA sCustomData;
        LPWSTR         pszDataBegin = NULL;
        int            iSizeRemaining = CUSTOM_INITIAL_ALLOC; //Let's begin with 12 strings.
        int            iTotalSize;
        int            iCurOffset;

        ZeroMemory(&sViewData, SIZEOF(sViewData));

        ZeroMemory(&sCustomData, SIZEOF(sCustomData));

        // there must be a view id
        if (FAILED(SHCLSIDFromString(pNextID, &sViewData.idView)))
        {
            continue;
        }

        // we blow off IE4b2 customized views. This forces them to run
        // the wizard again which will clean this junk up
        if (IsEqualIID(sViewData.idView, VID_DefaultCustomWebView))
        {
            continue;
        }

        // get the IShellView extended view, if any
        BOOL fExtShellView = FALSE;
        TCHAR szPreProcName[45];
        if (GetPrivateProfileString(c_szExtViews, pNextID, c_szNULL,
            szPreProcName, ARRAYSIZE(szPreProcName), szIniFile))
        {
            fExtShellView = SUCCEEDED(SHCLSIDFromString(szPreProcName, &sViewData.idExtShellView));
        }

        // All extended views use Attributes
        sViewData.dwFlags = GetPrivateProfileInt(pNextID, TEXT("Attributes"), 0, szIniFile) | SFVF_CUSTOMIZEDVIEW;

        // For some reason this code uses a much larger buffer
        // than seems necessary. I don't know why... [mikesh 29jul97]
        TCHAR szViewData[MAX_PATH+MAX_PATH];
        szViewData[0] = TEXT('\0'); // For the non-webview case

        if (IsEqualGUID(sViewData.idView, VID_WebView) && SUCCEEDED(ReadWebViewTemplate(szPath, szViewData, MAX_PATH)))
        {
            LPTSTR pszPath = szViewData;
            // We want to allow relative paths for the file: protocol
            //
            if (0 == StrCmpNI(TEXT("file://"), szViewData, 7)) // ARRAYSIZE(TEXT("file://"))
            {
                pszPath += 7;   // ARRAYSIZE(TEXT("file://"))
            }
            // for webview:// compatibility, keep this working:
            else if (0 == StrCmpNI(TEXT("webview://file://"), szViewData, 17)) // ARRAYSIZE(TEXT("file://"))
            {
                pszPath += 17;  // ARRAYSIZE(TEXT("webview://file://"))
            }
            // handle relative references...
            PathCombine(pszPath, szPath, pszPath);

            // Avoid overwriting buffers
            szViewData[MAX_PATH-1] = NULL;
        }
        else
        {
            // only IShellView extensions use LParams
            sViewData.lParam = GetPrivateProfileInt( pNextID, TEXT("LParam"), 0, szIniFile );

            if (!fExtShellView && VID_FolderState != sViewData.idView)
            {
                // No moniker, no IShellView extension, this must be a VID_FolderState
                // kinda thing. (Otherwise it's a bad desktop.ini.)
                //
                RIPMSG(0, "Extended view is registered incorrectly.");
                continue;
            }
        }
        SHTCharToUnicode(szViewData, sViewData.wszMoniker, ARRAYSIZE(sViewData.wszMoniker));

        // NOTE: the size is in WCHARs not in BYTES
        pszDataBegin = (LPWSTR)SHAlloc(iSizeRemaining * sizeof(WCHAR));
        if(NULL == pszDataBegin)
            continue;

        iTotalSize = iSizeRemaining;
        iCurOffset = 0;

        int i;
        // Read the custom colors
        for (i = 0; i < CRID_COLORCOUNT; i++)
            sCustomData.crCustomColors[i] = GetPrivateProfileInt(pNextID, c_szExtViewUIRegKeys[ID_EXTVIEWCOLORSFIRST + i], CLR_MYINVALID, szIniFile);
        
        // Read the extended view strings
        for (i = 0; i <= ID_EXTVIEWSTRLAST; i++)
        {
            sCustomData.acchOffExtViewUIstr[i] = GetCustomStrData(&pszDataBegin,
                       &iSizeRemaining, &iTotalSize, &iCurOffset,
                       pNextID, c_szExtViewUIRegKeys[i], szIniFile,
                       (i == ID_EXTVIEWICONAREAIMAGE ? szPath : NULL));
        }

        sCustomData.cchSizeOfBlock = (iTotalSize - iSizeRemaining);
        sCustomData.lpDataBlock = pszDataBegin;
        sViewData.pCustomData = &sCustomData;

        // if docobjextended view that is not webview, DO NOT ADD IT, UNSUPPORTED.
        if (!(sViewData.dwFlags & SFVF_NOWEBVIEWFOLDERCONTENTS)
                && !IsEqualGUID(sViewData.idView, VID_WebView)
                && !IsEqualGUID(sViewData.idView, VID_FolderState))
            continue;
        Add(&sViewData);

        //We already copied the data. So, we can free it!
        SHFree(pszDataBegin);
    }
}


void CViewsList::Empty()
{
    m_bGotDef = FALSE;

    for (int i=GetItemCount()-1; i>=0; --i)
    {
        SFVVIEWSDATA  *sfvData = GetPtr(i);

        if(sfvData->dwFlags & SFVF_CUSTOMIZEDVIEW)
        {
            CUSTOMVIEWSDATA  *pCustomPtr = sfvData->pCustomData;
            if(pCustomPtr)
            {
                if(pCustomPtr->lpDataBlock)
                    SHFree(pCustomPtr->lpDataBlock);
                SHFree(pCustomPtr);
            }
        }
        SHFree(sfvData);
    }

    CGenList::Empty();
}


SFVVIEWSDATA* CViewsList::CopyData(const SFVVIEWSDATA* pData)
{
    SFVVIEWSDATA* pCopy = (SFVVIEWSDATA*)SHAlloc(SIZEOF(SFVVIEWSDATA));
    if (pCopy)
    {
        hmemcpy(pCopy, pData, SIZEOF(SFVVIEWSDATA));
        if((pData->dwFlags & SFVF_CUSTOMIZEDVIEW) && pData->pCustomData)
        {
            CUSTOMVIEWSDATA *pCustomData = (CUSTOMVIEWSDATA *)SHAlloc(SIZEOF(CUSTOMVIEWSDATA));
            if(pCustomData)
            {
                hmemcpy(pCustomData, pData->pCustomData, SIZEOF(CUSTOMVIEWSDATA));
                pCopy->pCustomData = pCustomData;

                if(pCustomData->lpDataBlock)
                {
                    // NOTE: DataBlock size is in WCHARs
                    LPWSTR lpDataBlock = (LPWSTR)SHAlloc(pCustomData->cchSizeOfBlock * sizeof(WCHAR));
                    if(lpDataBlock)
                    {
                        // NOTE: DataBlock size is in WCHARs
                        hmemcpy(lpDataBlock, pCustomData->lpDataBlock, pCustomData->cchSizeOfBlock * sizeof(WCHAR));
                        pCustomData->lpDataBlock = lpDataBlock;
                    }
                    else
                    {
                        SHFree(pCustomData);
                        goto Failed;
                    }
                }
            }
            else
            {
Failed:
                SHFree(pCopy);
                pCopy = NULL;
            }
        }
    }

    return(pCopy);
}


int CViewsList::NextUnique(int nLast)
{
    for (int nNext=nLast+1; ; ++nNext)
    {
        SFVVIEWSDATA* pItem = GetPtr(nNext);
        if (!pItem)
        {
            break;
        }

        for (int nPrev=nNext-1; nPrev>=0; --nPrev)
        {
            SFVVIEWSDATA*pPrev = GetPtr(nPrev);
            if (pItem->idView == pPrev->idView)
            {
                break;
            }
        }

        if (nPrev < 0)
        {
            return(nNext);
        }
    }

    return(-1);
}


// Note this is 1-based
int CViewsList::NthUnique(int nUnique)
{
    for (int nNext=-1; nUnique>0; --nUnique)
    {
        nNext = NextUnique(nNext);
        if (nNext < 0)
        {
            return(-1);
        }
    }

    return(nNext);
}


UINT wstrlen(LPCWSTR pwsz)
{
    for (int i=0; *pwsz; ++i, ++pwsz)
    {
        // Just loop
    }

    return(i);
}


class CEnumCViewList : public IEnumSFVViews, private CViewsList
{
public:
    CEnumCViewList(CViewsList* pViews);

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)(THIS);
    STDMETHOD_(ULONG,Release)(THIS);

    // *** IEnumSFVViews methods ***
    STDMETHOD(Next)  (THIS_ ULONG celt, SFVVIEWSDATA **ppData, ULONG *pceltFetched);
    STDMETHOD(Skip)  (THIS_ ULONG celt) {return E_NOTIMPL;}
    STDMETHOD(Reset) (THIS) {m_uNext=0; return NOERROR;}
    STDMETHOD(Clone) (THIS_ IEnumSFVViews **ppenum) {return E_FAIL;}

private:
    UINT m_uNext;
    ULONG m_cRef;
} ;

CEnumCViewList::CEnumCViewList(CViewsList*pList) : m_cRef(1), m_uNext(0)
{
    Steal(pList);
}


STDMETHODIMP CEnumCViewList::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CEnumCViewList, IEnumSFVViews),  // IID_IEnumSFVViews
        { 0 }
    };
    return QISearch(this, qit, riid, ppvObj);
}


STDMETHODIMP_(ULONG) CEnumCViewList::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CEnumCViewList::Release()
{
    m_cRef--;
    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
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
    return S_OK;
}


HRESULT CreateEnumCViewList(CViewsList *pViews, IEnumSFVViews **ppObj)
{
    HRESULT hres = S_OK;
    
    CEnumCViewList* peViews = new CEnumCViewList(pViews);

    if (!peViews)
    {
        hres = E_OUTOFMEMORY;
    }

    *ppObj = peViews;

    return hres;
}

CSFVFrame::~CSFVFrame()
{
    ATOMICRELEASE(m_pActiveSVExt);
    ATOMICRELEASE(m_pDefViewExtInit2);
    ATOMICRELEASE(m_pActiveSFV);
    ATOMICRELEASE(m_pvoActive);
    ATOMICRELEASE(m_pActive);
    ATOMICRELEASE(m_pDocView);
    ATOMICRELEASE(m_pOleObj);
    if (m_dwConnectionCookie)
        _RemoveReadyStateNotifyCapability();
    ATOMICRELEASE(m_pOleObjNew);
}


HRESULT CSFVFrame::_DefaultGetExtViews(SHELLVIEWID * pvid, IEnumSFVViews ** ppev)
{
    CDefView* pView = IToClass(CDefView, m_cFrame, this);
    HRESULT hres;
    
    CViewsList cViews;

    cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("folder"));

    IPersist *pPersist;
    hres = pView->_pshf->QueryInterface(IID_IPersist, (void **)&pPersist);
    // NOTE: some implementors of IPersistFolder forgot to answer the QI
    // for IPersist (such as Printers and Control Panel -- what lameos)...
    if (FAILED(hres))
        hres = pView->_pshf->QueryInterface(IID_IPersistFolder, (void **)&pPersist);
    if (SUCCEEDED(hres))
    {
        GUID clsid;
        hres = pPersist->GetClassID(&clsid);
        if (SUCCEEDED(hres))
            cViews.AddCLSID(&clsid);
        pPersist->Release();
    }

    if (!cViews.GetDef(pvid))
    {
        // we failed to get a view the new way, so try the old win95 way
        FOLDERVIEWMODE fvm;

        if (SUCCEEDED(pView->CallCB(SFVM_DEFVIEWMODE, 0, (LPARAM)&fvm)))
        {
            switch (fvm)
            {
            case FVM_SMALLICON:
                *pvid = VID_SmallIcons;
                break;

            case FVM_LIST:
                *pvid = VID_List;
                break;

            case FVM_DETAILS:
                *pvid = VID_Details;
                break;

            case FVM_ICON:
            default:
                *pvid = VID_LargeIcons;
                break;
            }
        }
    }

    hres = CreateEnumCViewList(&cViews, ppev);

    return hres;
}

void CSFVFrame::GetExtViews(BOOL bForce)
{
    CDefView* pView = IToClass(CDefView, m_cFrame, this);

    IEnumSFVViews *pev = NULL;

    if (bForce)
    {
        m_bGotViews = FALSE;
    }

    if (m_bGotViews)
    {
        return;
    }

    m_lViews.Empty();

    SHELLVIEWID vid = VID_LargeIcons;
    if ((FAILED(pView->CallCB(SFVM_GETVIEWS, (WPARAM)&vid, (LPARAM)&pev)) &&
         FAILED(_DefaultGetExtViews(&vid, &pev))) ||
        !pev)
    {
        return;
    }

    m_lViews.SetDef(&vid);

    SFVVIEWSDATA *pData;
    ULONG uFetched;

    while ((pev->Next(1, &pData, &uFetched) == S_OK) && (uFetched == 1))
    {
        // The list comes to us in general to specific order, but we want
        // to search it in specific->general order. Inverting the list
        // is easiest here, even though it causes a bunch of memcpy calls.
        //
        m_lViews.Prepend(pData, FALSE);
    }

    ATOMICRELEASE(pev);

    m_bGotViews = TRUE;

    //
    // now check to see if we should include the standard Win95 views
    //
    m_bEnableStandardViews = TRUE;
    pView->CallCB(SFVM_QUERYSTANDARDVIEWS, 0, (LPARAM)&m_bEnableStandardViews);
}

BOOL ItemFromGUID(GUID const* pid, LPBYTE pbData, UINT cbData, UINT idString)
{
    TCHAR szGUID[64];
    LONG lRetVal;

    if (!SHStringFromGUID(*pid, szGUID, ARRAYSIZE(szGUID)))
    {
        return FALSE;
    }

    CSHRegKey crCLSID(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\ExtShellViews"));
    if (!crCLSID)
    {
        return FALSE;
    }

    CSHRegKey crGUID(crCLSID, szGUID);
    if (!crGUID)
    {
        return FALSE;
    }

    lRetVal = crGUID.QueryValueEx(c_szExtViewUIRegKeys[idString], pbData, cbData);

    return (lRetVal == ERROR_SUCCESS);
}

BOOL StringFromGUID(GUID const* pid, LPTSTR pszString, UINT cb, UINT idString)
{
    *pszString = 0;
    return ItemFromGUID(pid, (LPBYTE)pszString, cb * sizeof(TCHAR), idString);
}

BOOL ColorFromGUID(GUID const* pid, COLORREF * pcr, UINT idCR)
{
    *pcr = CLR_MYINVALID;
    return ItemFromGUID(pid, (LPBYTE)pcr, sizeof(COLORREF), idCR);
}

BOOL StringFromCustomViewData(SFVVIEWSDATA *pItem, LPTSTR pszString, UINT cb, UINT idString)
{
    if (!(pItem->dwFlags & SFVF_CUSTOMIZEDVIEW))
        return FALSE;
    
    // Get the string from the pItem->structure. If it is not there,
    // pick it up from the registry using idView
    CUSTOMVIEWSDATA *pCustomDataPtr;
    
    if(pCustomDataPtr = pItem->pCustomData)
    {
        if((pCustomDataPtr->lpDataBlock) && (pCustomDataPtr->acchOffExtViewUIstr[idString] >= 0))
        {
            LPWSTR  lpStr = pCustomDataPtr->lpDataBlock + pCustomDataPtr->acchOffExtViewUIstr[idString];
            SHUnicodeToTChar(lpStr, pszString, cb);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL ColorFromCustomViewData(SFVVIEWSDATA *pItem, COLORREF * pcr, UINT idCR)
{
    ASSERT(idCR < CRID_COLORCOUNT);

    if (!(pItem->dwFlags & SFVF_CUSTOMIZEDVIEW) || !pItem->pCustomData)
        return FALSE;

    // Get the color from the pItem->structure. If it is not there,
    // return FALSE;
    *pcr = pItem->pCustomData->crCustomColors[idCR];
    return ISVALIDCOLOR(*pcr);
}

BOOL StringFromViewPtr(SFVVIEWSDATA *pItem, LPTSTR pszString, UINT cb, UINT idString)
{
    //Check if the given view is a customized view!
    if(pItem->dwFlags & SFVF_CUSTOMIZEDVIEW)
    {
        if(StringFromCustomViewData(pItem, pszString, cb, idString))
            return(TRUE);
        //Else, fall through!
    }
    //Get it from the registry.
    return(StringFromGUID(&(pItem->idView), pszString, cb, idString));
}

BOOL ColorFromViewPtr(SFVVIEWSDATA *pItem, COLORREF * pcr, int idColor)
{
    //Check if the given view is a customized view!
    if(pItem->dwFlags & SFVF_CUSTOMIZEDVIEW)
    {
        if(ColorFromCustomViewData(pItem, pcr, idColor))
            return(TRUE);
        //Else, fall through!
    }

    //We have no way to specify this in the registry currently
    return(ColorFromGUID(&(pItem->idView), pcr, idColor));
}

BOOL CSFVFrame::_StringFromView(int uView, LPTSTR pszString, UINT cb, UINT idString)
{
    int iView = m_lViews.NthUnique(uView+1);
    if (-1 != iView)
    {
        SFVVIEWSDATA* pItem = m_lViews.GetPtr(iView);
        if (pItem)
            return StringFromViewPtr(pItem, pszString, cb, idString);
    }
    return FALSE;
}

HRESULT CSFVFrame::SetViewWindowStyle(DWORD dwBits, DWORD dwVal)
{
    if (m_pDefViewExtInit2)
    {
        m_pDefViewExtInit2->SetViewWindowStyle(dwBits, dwVal); 
    }
    return S_OK;
}
BOOL CSFVFrame::_IsHoster(int uView)
{
    int iView = m_lViews.NthUnique(uView+1);
    if (-1 != iView)
    {
        SFVVIEWSDATA* pItem = m_lViews.GetPtr(iView);
    
        if (pItem)
            return (!(pItem->dwFlags & SFVF_NOWEBVIEWFOLDERCONTENTS));
    }
    return FALSE;

}

BOOL CSFVFrame::_StringFromViewID(SHELLVIEWID const *pvid, LPTSTR pszString, UINT cb, UINT idString)
{
    SFVVIEWSDATA* pItem;

    GetViewIdFromGUID(pvid, &pItem);

    if (pItem)
        return StringFromViewPtr(pItem, pszString, cb, idString);
    else
        return FALSE;
}

BOOL CSFVFrame::_ColorFromView(int uView, COLORREF * pcr, int crid)
{
    int iView = m_lViews.NthUnique(uView+1);
    if (-1 != iView)
    {
        SFVVIEWSDATA* pItem = m_lViews.GetPtr(iView);
    
        if (pItem)
            return ColorFromViewPtr(pItem, pcr, crid);
    }
    return FALSE;
}

BOOL CSFVFrame::_ColorFromViewID(SHELLVIEWID const *pvid, COLORREF * pcr, int crid)
{
    SFVVIEWSDATA* pItem;

    GetViewIdFromGUID(pvid, &pItem);

    if (pItem)
        return ColorFromViewPtr(pItem, pcr, crid);
    else
        return FALSE;
}



void CSFVFrame::MergeExtViewsMenu(HMENU hmExtViews, CDefView * pdsv)
{
    int iAddedToggleView = 0;

    m_cSFVExt = 0;
    FillMemory(m_mpCmdIdToUid,sizeof(int) * MAX_EXT_VIEWS, -1);
    {
        HMENU hmTemp = _GetMenuFromID(hmExtViews, SFVIDM_MENU_VIEW);
        if (hmTemp)
            hmExtViews = hmTemp;
    }

    int index = MenuIndexFromID(hmExtViews, SFVIDM_VIEW_LASTVIEW);
    if (index < 0)
        index = 0;

    BOOL fWebView = FALSE;
    int nIDExt = SFVIDM_VIEW_EXTENDEDFIRST;
    int nIDSVExt = SFVIDM_VIEW_SVEXTFIRST;
    // save the start index incase we need to add "WebView" at the top...
    for (int i=-1; ; )
    {
        int nID;
        
        i = m_lViews.NextUnique(i);
        if (i < 0)
            break;

        SFVVIEWSDATA* pItem = m_lViews.GetPtr(i);

        // all views that host defview OC are "checkable"
        // no need to special case web view, since SFVF_NOWEBVIEWFOLDERCONTENTS is NOT set there.
        if ( !(pItem->dwFlags & SFVF_NOWEBVIEWFOLDERCONTENTS))
        {
            fWebView = TRUE;
            nID = nIDExt;
        }
        else
        {
            fWebView = FALSE;
            nID = nIDSVExt;
            m_cSFVExt++;
        }

        m_mpCmdIdToUid[nID - SFVIDM_VIEW_EXTFIRST] = i;     // this command id is this UID
        // The folder state stuff is piggy-backing on the viewlist code as VID_FolderState.
        // It's not really an extended view.
        if (IsEqualGUID(pItem->idView, VID_FolderState))
        {
            if (fWebView)
                nIDExt++;  
            else 
                nIDSVExt++;
            continue;
        }

        TCHAR szName[80];
        if (!StringFromViewPtr(pItem, szName, ARRAYSIZE(szName), ID_EXTVIEWNAME))
        {
            if (fWebView)
                nIDExt++;  
            else 
                nIDSVExt++;
            continue;
        }

        // is this extension view wanting to be consider a "normal" full-view
        // (it will show up in the file-open dialogs)
        if ( pdsv->_pcdb && !(pItem->dwFlags & SFVF_TREATASNORMAL))
        {
            if (fWebView)
                nIDExt++;  
            else 
                nIDSVExt++;
            continue;
        }

        // Don't display our webview for the desktop on the folder view menu or if we have
        // the classic shell restriction enabled.
        if ((pdsv->_IsViewDesktop() && !(pItem->dwFlags & SFVF_TREATASNORMAL)) ||
            SHRestricted(REST_CLASSICSHELL)) 
        {
            if (fWebView)
                nIDExt++;  
            else 
                nIDSVExt++;
            continue;
        }

        // all views that host defview OC are "checkable"
        // no need to special case web view, since SFVF_NOWEBVIEWFOLDERCONTENTS is NOT set there.
        if (!fWebView)
        {
            MENUITEMINFO mii;
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_TYPE|MIIM_ID;
            mii.fType = MFT_STRING;
            mii.wID = nID;
            mii.dwTypeData = szName;
            mii.cch = ARRAYSIZE(szName);
            InsertMenuItem(hmExtViews, ++index, TRUE, &mii);
        }
        
        if (fWebView)
            ++nIDExt;  
        else 
            ++nIDSVExt;
        if ((nIDExt > SFVIDM_VIEW_EXTENDEDLAST) || (nIDSVExt > SFVIDM_VIEW_SVEXTLAST))
            break;
    }
}


void CleanUpDocView(IOleDocumentView* pDocView, IOleObject* pOleObj)
{
    pDocView->UIActivate(FALSE);

    IOleInPlaceObject* pipo;
    if (SUCCEEDED(pOleObj->QueryInterface(IID_IOleInPlaceObject, (void **)&pipo)))
    {
        pipo->InPlaceDeactivate();
        pipo->Release();
    }
    pDocView->CloseView(0);
    pDocView->SetInPlaceSite(NULL);
    pDocView->Release();
}

void CSFVFrame::_CleanupOldDocObject( )
{
    //See if we have already switched to the new Ole Obj
    if (m_pDocView)
    {
        //Save the current values first!
        IOleObject          *pOleObjOld = m_pOleObj;
        IOleDocumentView    *pDocViewOld = m_pDocView;

        m_pDocView = NULL;
        m_pOleObj = NULL;
        m_uActiveExtendedView = NOEXTVIEW;
        CleanUpDocView(pDocViewOld, pOleObjOld);
        _CleanUpOleObjAndDt(pOleObjOld);
        SetActiveObject(NULL, NULL);
    }

    if (m_dwConnectionCookie)
        _RemoveReadyStateNotifyCapability();

    _CleanupNewOleObj();
}

void CSFVFrame::_CleanUpOleObj(IOleObject* pOleObj)
{
    pOleObj->Close(OLECLOSE_NOSAVE);
    pOleObj->SetClientSite(NULL);
    pOleObj->Release();
}

void CSFVFrame::_CleanUpOleObjAndDt(IOleObject* pOleObj)
{
    IDropTarget* pdtTemp;

    _CleanUpOleObj(pOleObj);

    // If we have a wrapping droptarget, release it now. 
    pdtTemp = m_cSite._dt._pdtDoc;
    if (pdtTemp) 
    {
        m_cSite._dt._pdtDoc = NULL;
        pdtTemp->Release();
    }
    pdtTemp = m_cSite._dt._pdtFrame;
    if (pdtTemp) 
    {
        m_cSite._dt._pdtFrame = NULL;
        pdtTemp->Release();
    }
}

void CSFVFrame::_CleanupNewOleObj()
{
    IOleObject *pOleObj;

    pOleObj = m_pOleObjNew;
    if(pOleObj)
    {
        m_pOleObjNew = NULL;
        _CleanUpOleObj(pOleObj);
    }
}

int CSFVFrame::GetViewIdFromGUID(SHELLVIEWID const *pvid, SFVVIEWSDATA** ppItem)
{
    int iView = -1;
    for (UINT uView=0; uView<MAX_EXT_VIEWS; ++uView)
    {
        iView = m_lViews.NextUnique(iView);

        SFVVIEWSDATA* pItem = m_lViews.GetPtr(iView);
        if (!pItem)
        {
            break;
        }

        if (*pvid == pItem->idView)
        {
            if (ppItem)
                *ppItem = pItem;

            return((int)uView);
        }
    }

    if (ppItem)
        *ppItem = NULL;
    return(-1);
}


HRESULT CSFVFrame::ShowExtView(SHELLVIEWID const *pvid, BOOL bForce)
{
    int iView = GetViewIdFromGUID(pvid);

    return (iView >= 0)?
        ShowExtView((UINT)iView, bForce) : E_INVALIDARG;
}

//
// Funciton: _MakeShortPsuedoUniqueName
//
// Parameters:
//  pszUniqueName -- Specify the buffer where the unique name should be copied
//  cchMax        -- Specify the size of the buffer
//  pszTemplate   -- Specify the base name
//  pszDir        -- Specify the directory
//
void _MakeShortPsuedoUniqueName(LPTSTR  pszUniqueName,
                               UINT   cchMax,
                               LPCTSTR pszTemplate,
                               LPCTSTR pszDir)
{

    TCHAR szFullPath[MAX_PATH];

    // It is assuming that the template has a %d in it...
    lstrcpy(szFullPath, pszDir);
    PathAppend(szFullPath, pszTemplate);
    wsprintf(pszUniqueName, szFullPath, LOWORD(GetTickCount()));
}

void DisableActiveDesktop()
{
    SHELLSTATE  ss;

    // Disable this settings in the registry!
    ss.fDesktopHTML = FALSE;
    SHGetSetSettings(&ss, SSF_DESKTOPHTML, TRUE);  // Write back the new

    // Tell the user that we have just disabled the active desktop!
    ShellMessageBox(HINST_THISDLL, NULL, MAKEINTRESOURCE(IDS_HTMLFILE_NOTFOUND),
                       MAKEINTRESOURCE(IDS_DESKTOP),
                       MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND);
}

HRESULT CSFVFrame::_UpdateZonesStatusPane(IOleObject *pOleObj)
{
    HRESULT hres = S_OK;
    CDefView* pView = IToClass(CDefView, m_cFrame, this);
    
    VARIANT var = { 0 };
    V_VT(&var) = VT_EMPTY;

    IOleCommandTarget* pct;
    if (pOleObj && SUCCEEDED(GetCommandTarget(&pct)))
    {
        hres = pct->Exec(&CGID_Explorer, SBCMDID_MIXEDZONE, 0, 0, &var);
        pct->Release();
    } 
    else 
    {
        V_VT(&var) = VT_UI4;
        V_UI4(&var) = URLZONE_LOCAL_MACHINE; // Default is "My Computer"
        pView->CallCB(SFVM_GETZONE, 0, (LPARAM)&V_UI4(&var));
    }

    if (V_VT(&var) == VT_UI4) // We were able to figure out what zone we are in
        V_UI4(&var) = MAKELONG(2, V_UI4(&var));    
    else if (V_VT(&var) == VT_NULL)  // MSHTML has figured us to be in a mixed zone
        V_UI4(&var) = MAKELONG(2, ZONE_MIXED);    
    else // We don't have zone info
        V_UI4(&var) = MAKELONG(2, ZONE_UNKNOWN);    
       
    V_VT(&var) = VT_UI4;
            
    // Tell CShellbrowser to show the zone stuff in the 2nd pane
    if (pView && pView->_psb && SUCCEEDED(pView->_psb->QueryInterface(IID_IOleCommandTarget, (void **)&pct)))
    {
        pct->Exec(&CGID_Explorer, SBCMDID_MIXEDZONE, 0, &var, NULL);
        pct->Release();
    }   
    ASSERT((V_VT(&var) == VT_I4)    || (V_VT(&var) == VT_UI4)  || 
           (V_VT(&var) == VT_EMPTY) || (V_VT(&var) == VT_NULL));

    return hres;
}

HRESULT GetRGBFromBStr(COLORREF  *pclr, BSTR  bstr)
{
    CHAR     szColor[11] = {'0','x',0, };
    CHAR     szTemp[9] = {0};
    INT      rgb = 0;
    HRESULT  hr = S_OK;

    if(pclr == NULL)
        return E_INVALIDARG;

    if(bstr)
    {
        LPSTR pszPound;

        WideCharToMultiByte(CP_ACP, 0, bstr, -1, (LPSTR)&szTemp[0], 8, NULL, NULL);

        if(pszPound = StrChrA(szTemp, '#'))
            pszPound++; //Skip the pound sign!
        else
            pszPound = szTemp;  //Pound sign is missing. Use the whole strng
        lstrcatA(szColor, pszPound);
    }

    if(StrToIntExA(szColor, STIF_SUPPORT_HEX, &rgb))
    {
        *pclr = (COLORREF)(((rgb & 0x000000ff) << 16) | (rgb & 0x0000ff00) | ((rgb & 0x00ff0000) >> 16));
    }
    else
    {
        *pclr = CLR_INVALID;
        hr = E_FAIL;
    }

    return hr;
}

HRESULT CSFVFrame::_SetDesktopListViewIconTextColors(BOOL fNotify)
{
    HRESULT hres;

//  98/11/19 #250276 vtan: This method can now be called publicly.
//  Safe guard against an AV and return the system color if NULL.

    if (m_pOleObj == NULL)
    {
        m_bgColor = GetSysColor(COLOR_BACKGROUND);
        hres = S_OK;
    }
    else
    {
        // Get the HTMLDocument2
        IHTMLDocument2 *pDoc;
        hres = m_pOleObj->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc);
        if (SUCCEEDED(hres))
        {
            VARIANT v;

            v.vt = VT_BSTR;
            v.bstrVal = NULL;

            hres = pDoc->get_bgColor(&v);

            if (SUCCEEDED(hres))
            {
                CDefView* pView = IToClass(CDefView, m_cFrame, this);
                COLORREF  clrNewIconTextBk;

                if(SUCCEEDED(GetRGBFromBStr(&clrNewIconTextBk, v.bstrVal)))
                {
                    //See if we haven't set this yet OR the colors have changed!
                    if((m_bgColor == CLR_INVALID) || (m_bgColor != clrNewIconTextBk))
                    {
                        m_bgColor = clrNewIconTextBk;

//  vtan: DSV_SetFolderColors() calls this function. To prevent
//  recursion this is only executed explicitly.

                        if (fNotify)
                        {
                            DSV_SetFolderColors(pView); //Tell the listview about color change!

                            //Because the colors have changed, repaint the listview!
                            if (pView->_hwndListview)
                                InvalidateRect(pView->_hwndListview, NULL, TRUE);
                        }
                    }
                }
            }

            if (v.bstrVal)
                SysFreeString(v.bstrVal);

            pDoc->Release();
        }
    }
    return hres;
}

// ready state complete has occured, ready to do the switch thing.  

HRESULT CSFVFrame::_SwitchToNewOleObj()
{
    HRESULT hres = S_OK;

    if (!m_fSwitchedToNewOleObj && m_pOleObjNew)
    {
        m_fSwitchedToNewOleObj = TRUE;

        CDefView* pView = IToClass(CDefView, m_cFrame, this);
    
        //Save the current values first!
        IOleObject          *pOleObjOld = m_pOleObj;
        IOleDocumentView    *pDocViewOld = m_pDocView;
        IOleObject          *pOleObjNew = m_pOleObjNew;

        
        m_pDocView = NULL;
        m_pOleObj = NULL;
        m_pOleObjNew = NULL;
    
        //If we already have one, destroy it!
        if (pDocViewOld)
        {
            //To prevent flashing, set the flag that avoids the painting
            SendMessage(pView->_hwndView, WM_SETREDRAW, 0, 0);
    
            CleanUpDocView(pDocViewOld, pOleObjOld);
            _CleanUpOleObjAndDt(pOleObjOld);
            SetActiveObject(NULL, NULL);
    
            //Is the ViewWindow still around?
            if (IsWindow(pView->_hwndView))
            {
                SendMessage(pView->_hwndView, WM_SETREDRAW, TRUE, 0);
                if (pView->_hwndListview)
                    InvalidateRect(pView->_hwndListview, NULL, TRUE);
            }
        }
    
        // HACK: We need to set the host names for Word to force embedding mode
        pOleObjNew->SetHostNames(L"1", L"2");
    
        OleRun(pOleObjNew);
    
        IOleDocumentView* pDocView = NULL;
    
        IOleDocument* pDocObj;
        hres = pOleObjNew->QueryInterface(IID_IOleDocument, (void **)&pDocObj);
        if (SUCCEEDED(hres))
        {
            hres = pDocObj->CreateView(&m_cSite, NULL, 0, &pDocView);
            if (SUCCEEDED(hres))
            {
                RECT rcView;
    
                pDocView->SetInPlaceSite(&m_cSite);
    
                GetClientRect(pView->_hwndView, &rcView);
                hres = pOleObjNew->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL,
                                &m_cSite, (UINT)-1, pView->_hwndView, &rcView);
                if (FAILED(hres))
                    CleanUpDocView(pDocView, pOleObjNew);
            }
    
            pDocObj->Release();
        }
    
        if (SUCCEEDED(hres))
        {
            hres = NOERROR; // S_FALSE -> S_OK, needed?

            ASSERT(m_pOleObj == NULL);
            ASSERT(m_pDocView == NULL);

            m_pDocView = pDocView;
            pDocView->AddRef();     // copy hold the ref for our copy

            m_pOleObj = pOleObjNew;

            m_uActiveExtendedView = m_uViewNew;
    
            m_pOleObjNew = NULL;
            m_uViewNew = NOEXTVIEW;
    
            RECT rcClient;
    
            // Make sure the new view is the correct size
            GetClientRect(pView->_hwndView, &rcClient);
            SetRect(&rcClient);

            // If this is desktop, then we need to see the listview's background color
            if (pView->_IsDesktop())
            {
                _SetDesktopListViewIconTextColors(TRUE);
            }
        }
        else
        {
            if (pView->_IsDesktop())
                PostMessage(pView->_hwndView, WM_DSV_DISABLEACTIVEDESKTOP, 0, 0);
    
            // Clean up if necessary
            _CleanupNewOleObj();
        }

        ATOMICRELEASE(pDocView);
    }

    return hres;
}

// IBindStatusCallback impl
HRESULT CSFVFrame::CBindStatusCallback::QueryInterface(REFIID riid, void ** ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CSFVFrame::CBindStatusCallback, IBindStatusCallback),  // IID_IBindStatusCallback
        QITABENT(CSFVFrame::CBindStatusCallback, IServiceProvider),     // IID_IServiceProvider
        { 0 }
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CSFVFrame::CBindStatusCallback::AddRef(void)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, _bsc, this);
    return pFrame->AddRef();
}

ULONG CSFVFrame::CBindStatusCallback::Release(void)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, _bsc, this);
    return pFrame->Release();
}


HRESULT CSFVFrame::CBindStatusCallback::OnStartBinding(DWORD grfBSCOption, IBinding *pib)
{
    return S_OK;
}
HRESULT CSFVFrame::CBindStatusCallback::GetPriority(LONG *pnPriority)
{
    *pnPriority = NORMAL_PRIORITY_CLASS;
    return S_OK;
}
HRESULT CSFVFrame::CBindStatusCallback::OnLowResource(DWORD reserved)
{
    return S_OK;
}
HRESULT CSFVFrame::CBindStatusCallback::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    return S_OK;
}
HRESULT CSFVFrame::CBindStatusCallback::OnStopBinding(HRESULT hresult, LPCWSTR szError)
{
    return S_OK;
}
HRESULT CSFVFrame::CBindStatusCallback::GetBindInfo(DWORD *grfBINDINFOF, BINDINFO *pbindinfo)
{
    return S_OK;
}
HRESULT CSFVFrame::CBindStatusCallback::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    return S_OK;
}
HRESULT CSFVFrame::CBindStatusCallback::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    return S_OK;
}
HRESULT CSFVFrame::CBindStatusCallback::QueryService(REFGUID guidService, REFIID riid, void ** ppvObj)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, _bsc, this);
    CDefView* pView = IToClass(CDefView, m_cFrame, pFrame);

    if (IsEqualGUID(guidService, SID_DefView))
    {
        // QueryService from a pluggable protocol/mime filter winds up
        // here during the Bind, but Trident re-binds during F5 processing
        // so the QueryService winds up directly at m_cSite. Handle all
        // SID_DefView processing there so there's no discrepencies.
        //
        return pFrame->m_cSite.QueryService(guidService, riid, ppvObj);
    }

    *ppvObj = NULL;
    return E_FAIL;
}


// BUGBUG TODO CLEANUP: Why does the IOleObject case return everything
// yet the IShellView case slams everything into member variables?
//
HRESULT CSFVFrame::_CreateNewOleObj(IOleObject **ppOleObj, int uView)
{
    HRESULT hres;
    CDefView* pView = IToClass(CDefView, m_cFrame, this);

    //Initialize the return values to NULL first!
    *ppOleObj = NULL;

    int iView = m_lViews.NthUnique(uView+1);
    SFVVIEWSDATA* pItem = m_lViews.GetPtr(iView);

    // Check if this is desktop. If so, see if we need to generate a new
    // desktop.htm from the registry
    if (pView->_IsDesktop())
    {
        IActiveDesktopP *piadp;
        if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_ActiveDesktop, NULL, IID_IActiveDesktopP, (void **)&piadp)))
        {
            piadp->EnsureUpdateHTML();
            piadp->Release();
        }
    }

    IOleObject* pOleObj = NULL;

    if (pItem->wszMoniker[0])
    {
        IMoniker * pMoniker;
        hres = CreateURLMoniker(NULL, pItem->wszMoniker, &pMoniker);
        if (SUCCEEDED(hres))
        {
            IBindCtx * pbc;
            hres = CreateBindCtx(0, &pbc);
            if (SUCCEEDED(hres))
            {
                // NOTE: We only support synchronous bind here!
                //
                //
                //  Associate the client site as an object parameter to this
                // bind context so that Trident can pick it up while processing
                // IPersistMoniker::Load().
                //
                pbc->RegisterObjectParam(WSZGUID_OPID_DocObjClientSite,
                                            SAFECAST((&m_cSite), IOleClientSite*));
                                            
                RegisterBindStatusCallback(pbc, SAFECAST(&_bsc, IBindStatusCallback*), 0, 0);

                hres = pMoniker->BindToObject(pbc, NULL, IID_IOleObject, (void **)&pOleObj);

                if (FAILED(hres))
                {
                    if (pView->_IsDesktop())
                        PostMessage(pView->_hwndView, WM_DSV_DISABLEACTIVEDESKTOP, 0, 0);
                }

                RevokeBindStatusCallback(pbc, SAFECAST(&_bsc, IBindStatusCallback*));
                pbc->Release();
            }
            pMoniker->Release();
        }
    }
    else
    {
        // There is no pre-processor, so the view is just the CLSID that the
        // callback gave to us

        // see if it supports IShellView
        // this is not in the preproc codepath because it is slower and we want
        // to kill the preprocessor in the long run anyway

        // we will simply QI for IShellView or IShellView2
        // and use that to detect if it is an IshellView extension
        IDefViewExtInit * pShellView;
        hres = CoCreateInstance(pItem->idExtShellView, NULL, CLSCTX_INPROC_SERVER, IID_IDefViewExtInit, (void **)&pShellView);
        if (SUCCEEDED(hres))
        {
            // do the initialisation stuff for the new view. ....
            // need to go back to the def-view to get the information to pass
            // on to the CreateView
            ASSERT( NULL == m_pActiveSVExt );
            ASSERT( NULL == m_pDefViewExtInit2 );
            ASSERT( NULL != pView );
    
            IShellView2 * pSFV = NULL;
            IShellFolderView * pShellFV = NULL;
    
            // give the view extension the information it would have if it was a whole view ...
            hres = pShellView->SetOwnerDetails(pView->_pshf, pItem->lParam);
            if (SUCCEEDED(hres))
            {
                hres = pShellView->QueryInterface(IID_IShellView2, (void **)&pSFV);
                if (SUCCEEDED(hres))
                {
                    hres = pShellView->QueryInterface(IID_IShellFolderView, (void **)&pShellFV);
                    if (SUCCEEDED(hres))
                    {
                        // set the callback ...
                        IShellFolderViewCB * pOld = NULL;
                        hres = pShellFV->SetCallback(pView->m_cCallback.GetSFVCB(), &pOld);

                        // we have just created the view, so it shouldn't have an old callback..
                        ASSERT( NULL == pOld );
                    }
                }
            }
    
            if (SUCCEEDED(hres))
            {
                RECT rcSize;
                GetClientRect(pView->_hwndView, &rcSize);
    
                SV2CVW2_PARAMS rgCVW2;
                rgCVW2.cbSize = sizeof( rgCVW2 );
                rgCVW2.psvPrev = (IShellView *) pView;
                rgCVW2.psbOwner = pView->_psb;
                // Always create this hidden (just like listview),
                // we'll show it when the time is right
                pView->_fs.fFlags |= FWF_NOVISIBLE;
                rgCVW2.pfs = &( pView->_fs );
                rgCVW2.prcView = & rcSize;
                rgCVW2.pvid = NULL;
                rgCVW2.hwndView = NULL;
    
                hres = pSFV->CreateViewWindow2( &rgCVW2 );
                if (S_OK == hres)
                    m_hActiveSVExtHwnd = rgCVW2.hwndView;
            }
    
            if (S_OK == hres)
            {
                ASSERT( NULL != m_hActiveSVExtHwnd );
    
                // only change view if not extended (weblike) view to keep track for toggling.
                m_uView = uView;
                m_pActiveSVExt = pSFV;
                pSFV->AddRef();     // ref our copy

                m_pActiveSVExt->QueryInterface(IID_IDefViewExtInit2, (void **)&m_pDefViewExtInit2);

                m_pActiveSFV = pShellFV;
                pShellFV->AddRef(); // ref our copy
    
                if (IsWebView())
                    SetViewWindowStyle(WS_EX_CLIENTEDGE, 0); // no client edge on the view window

                ShowWindow(pView->_hwndListview, SW_HIDE);
    
                // we must have both the IShellView2 and the IShellfolderView by this point..
                ASSERT( NULL != m_pActiveSVExt &&
                        NULL != m_pActiveSFV &&
                        NULL != m_hActiveSVExtHwnd );
            }
            else if (hres == S_FALSE)
                hres = NOERROR; // we use S_FALSE to mean we swiped the view

            // cleanup, other refs to these have been taken above as needed
            ATOMICRELEASE(pShellView);
            ATOMICRELEASE(pShellFV);
            ATOMICRELEASE(pSFV);
    
            pView->RegisterSFVEvents(m_pActiveSFV, TRUE);  // hook view to sfv extension

            // If we're visible, show the window immediately
            //
            if (pView->_uState != SVUIA_DEACTIVATE)
            {
                // only swap the sfvext in forcefully if switching hosted view to sfvext, not
                // when currently "syncronously" creating a hoster.
                //
                // SwapWindow does the resize+visible at the same time, no need to make visible
                // before calling
                //
                if (IsWebView())
                    pView->SwapWindow();
                else
                    SetWindowPos(m_hActiveSVExtHwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW | SWP_NOZORDER);
            }
        }   
    }

    *ppOleObj = pOleObj;
    return hres;
}


BOOL CSFVFrame::IsExtendedSFVModal()
{
    if ( m_pActiveSFV )
    {
        if (m_pDefViewExtInit2)
        {
            return m_pDefViewExtInit2->IsModal() == S_OK;
        }
    }
    return FALSE;
}

HRESULT CSFVFrame::SFVAutoAutoArrange( DWORD dwReserved )
{
    if (m_pDefViewExtInit2)
    {
        return m_pDefViewExtInit2->AutoAutoArrange(dwReserved);
    }
    return E_FAIL;
}

void CSFVFrame::KillActiveSFV(void)
{
    CDefView* pView = IToClass(CDefView, m_cFrame, this);
    pView->RegisterSFVEvents(m_pActiveSFV, FALSE);    // unhook the view from the sfv extension.
    // if old view is view extension, destroy the IShellView extension
    m_pActiveSVExt->DestroyViewWindow();
    ATOMICRELEASE(m_pActiveSVExt);
    ATOMICRELEASE(m_pDefViewExtInit2);
    ATOMICRELEASE(m_pActiveSFV);
    m_uView = NOEXTVIEW;
}


//
// bForce indicates whether you want to Reload the view even if it is already
// in that view. This is used while doing "Refresh".
//

HRESULT CSFVFrame::ShowExtView(int uView, BOOL bForce)
{
    HRESULT hres;

    CDefView* pView = IToClass(CDefView, m_cFrame, this);

    BOOL bRet = FALSE;

    if ((uView == m_uView) && (!bForce))
    {
        // Nothing to do
        return NOERROR;
    }

    if (GetSystemMetrics(SM_CLEANBOOT) && uView != NOEXTVIEW && uView != HIDEEXTVIEW)
        return E_FAIL;

    // If we are hiding or destroying the Extended view, destroy the ole objects
    if (uView == HIDEEXTVIEW || (uView == NOEXTVIEW && m_uActiveExtendedView >= 0))
    {
        _CleanupOldDocObject();
    }

    if (m_pActiveSVExt && uView == NOEXTVIEW)
        KillActiveSFV();

    int iView = m_lViews.NthUnique(uView+1);
    SFVVIEWSDATA* pItem = m_lViews.GetPtr(iView);
    if (!pItem)
    {
        ASSERT(NULL == m_pDocView);
        ASSERT(NULL == m_pOleObj);
        ASSERT(NULL == m_pActive);

        if (m_uView == uView)
            return NOERROR;
        else if (m_uView == NOEXTVIEW && uView == HIDEEXTVIEW)
            return NOERROR;
        else
            return E_FAIL;
    }

    // only destroy shell view extension if ew view not a hoster. (or we are closing down)
    if (m_pActiveSVExt && (pItem->dwFlags & SFVF_NOWEBVIEWFOLDERCONTENTS))
        KillActiveSFV();

    // if we are switching to a webview like view (hoster), 
    // kill previous readystatenotify and cleanup old pending hoster.
    if (!(pItem->dwFlags & SFVF_NOWEBVIEWFOLDERCONTENTS))
    {
        // TODO: move into _CleanupNewOleObj
        if (m_dwConnectionCookie)
            _RemoveReadyStateNotifyCapability();

        // Clean up if a new Ole object is already awaiting ready state
        if (m_pOleObjNew)
            _CleanupNewOleObj();    // TODO: rename to _CleanupPendingView
        ASSERT(m_dwConnectionCookie == NULL);
        ASSERT(m_pOleObjNew == NULL);
    }

    // Create and initialize the new old object!
    IOleObject *pOleObj;

    hres = _CreateNewOleObj(&pOleObj, uView);
    if (SUCCEEDED(hres) && pOleObj)
    {
        if (!m_pOleObjNew)
        {
            hres = _ShowExtView_Helper(pOleObj, uView);
            pOleObj->SetClientSite(&m_cSite);
        }
        else
        {
            // Yikes!  We got reentered during the creation of the OleObj, blow away the object
            // and just return.
            pOleObj->Release();
        }
    }

    return hres;
}


HRESULT CSFVFrame::_ShowExtView_Helper(IOleObject* pOleObj, int uView)
{
    HRESULT hres;

    // Don't leak the old object, it must be NULL at this point
    ASSERT(m_pOleObjNew == NULL);

    //Save the new ole object
    m_pOleObjNew = pOleObj;
    m_uViewNew  = uView;
    m_fSwitchedToNewOleObj = FALSE;

    //Establish to connection point to receive the READYSTATE notification.
    if(!_SetupReadyStateNotifyCapability())
    {
        _SwitchToNewOleObj();
        _UpdateZonesStatusPane(m_pOleObj);   
        // If the object doesn't support readystate (or it's already interactive)
        // then we return S_OK to indicate synchronous switch.
        hres = S_OK;
    }
    else
    {
        // We're waiting on the docobj, we'll call _SwitchToNewOleObj
        // when it goes interactive...
        hres = S_FALSE;
    }

    return hres;
}


#ifdef NO_SHLWAPI_INTERNAL // Also defined in shlwapi
HRESULT ConnectToConnectionPoint(IUnknown* punkThis, REFIID riidEvent, BOOL fConnect, IUnknown* punkTarget, DWORD* pdwCookie, IConnectionPoint** ppcpOut)
{
    // We always need punkTarget, we only need punkThis on connect
    if (!punkTarget || (fConnect && !punkThis))
    {
        return E_FAIL;
    }

    if (ppcpOut)
        *ppcpOut = NULL;

    HRESULT hr;
    IConnectionPointContainer *pcpContainer;

    // Let's now have the Browser Window give us notification when something happens.
    if (SUCCEEDED(hr = punkTarget->QueryInterface(IID_IConnectionPointContainer, (void **)&pcpContainer)))
    {
        IConnectionPoint *pcp;
        if(SUCCEEDED(hr = pcpContainer->FindConnectionPoint(riidEvent, &pcp)))
        {
            if(fConnect)
            {
                // Add us to the list of people interested...
                hr = pcp->Advise(punkThis, pdwCookie);
                if (FAILED(hr))
                    *pdwCookie = 0;
            }
            else
            {
                // Remove us from the list of people interested...
                hr = pcp->Unadvise(*pdwCookie);
                *pdwCookie = 0;
            }

            if (ppcpOut && SUCCEEDED(hr))
                *ppcpOut = pcp;
            else
                ATOMICRELEASE(pcp);
        }
        ATOMICRELEASE(pcpContainer);
    }
    return hr;
}
#endif

BOOL CSFVFrame::_SetupReadyStateNotifyCapability()
{
    // By default we don't have gray-flash communication
    BOOL fSupportsReadystate = FALSE;
    long lReadyState;
    
    // Sanity Check
    if (!m_pOleObjNew)  
        return(fSupportsReadystate);
    
    // Check for proper readystate support
    BOOL fReadyStateOK = FALSE;
    IDispatch * p_idispatch;
    if (SUCCEEDED(m_pOleObjNew->QueryInterface( IID_IDispatch, (void **) &p_idispatch)))
    {
        VARIANTARG va;
        EXCEPINFO exInfo;

        if (SUCCEEDED(p_idispatch->Invoke( DISPID_READYSTATE, IID_NULL, LOCALE_USER_DEFAULT,  DISPATCH_PROPERTYGET, &g_dispparamsNoArgs, &va, &exInfo, NULL)))
        {
            if ((va.vt == VT_I4) && (va.lVal < READYSTATE_COMPLETE))
            {
                lReadyState = va.lVal;
                fReadyStateOK = TRUE;
            }
        }

        ATOMICRELEASE(p_idispatch);
    }

    if (fReadyStateOK)
    {
        // Check and Set-Up IPropertyNotifySink
        if (SUCCEEDED(ConnectToConnectionPoint(SAFECAST(this, IPropertyNotifySink*), IID_IPropertyNotifySink, TRUE, m_pOleObjNew, &m_dwConnectionCookie, NULL)))
        {
            fSupportsReadystate = TRUE;
            m_fReadyStateInteractiveProcessed = FALSE;
            m_fReadyStateComplete = FALSE;
            m_pOleObjReadyState = m_pOleObjNew;
            m_pOleObjReadyState->AddRef();
        }
    }

    return(fSupportsReadystate);
}

BOOL CSFVFrame::_RemoveReadyStateNotifyCapability()
{
    BOOL fRet = FALSE;

    if (m_dwConnectionCookie)
    {
        ASSERT(m_pOleObjReadyState);
        ConnectToConnectionPoint(NULL, IID_IPropertyNotifySink, FALSE, m_pOleObjReadyState, &m_dwConnectionCookie, NULL);
        ATOMICRELEASE(m_pOleObjReadyState);
        fRet = TRUE;
        m_dwConnectionCookie = 0;
    }

    return(fRet);
}
HWND CSFVFrame::GetExtendedViewWindow()
{
    HWND hwnd;

    if (m_pActiveSVExt)
        return m_hActiveSVExtHwnd;

    if (m_pDocView)
    {
        IOleWindow * pOW;
        if ( SUCCEEDED( m_pDocView->QueryInterface( IID_IOleWindow, (void **) & pOW)))
        {
            hwnd = NULL;
            pOW->GetWindow( & hwnd );
            pOW->Release();
            if (hwnd)
                return hwnd;
        }
    }
        
    if (m_pActive && SUCCEEDED(m_pActive->GetWindow(&hwnd)))
        return hwnd;

    return NULL;
}


// We want to QueryInterface only to some interfaces, but want
// the AddRef and Release to be for the whole object
STDMETHODIMP CSFVSite::QueryInterface(REFIID riid, void **ppvObj)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, m_cSite, this);
    CDefView* pView = IToClass(CDefView, m_cFrame, pFrame);

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IOleWindow) ||
        IsEqualIID(riid, IID_IOleInPlaceSite))
    {
        *ppvObj = SAFECAST(this, IOleInPlaceSite*);
    }
    else if (IsEqualIID(riid, IID_IOleClientSite))
    {
        *ppvObj = SAFECAST(this, IOleClientSite*);
    }
    else if (IsEqualIID(riid, IID_IOleDocumentSite))
    {
        *ppvObj = SAFECAST(this, IOleDocumentSite*);
    }
    else if (IsEqualIID(riid, IID_IServiceProvider))
    {
        *ppvObj = SAFECAST(this, IServiceProvider*);
    }
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
    {
        *ppvObj = SAFECAST(this, IOleCommandTarget*);
    }
    else if (IsEqualIID(riid, IID_IDocHostUIHandler))
    {
        *ppvObj = SAFECAST(this, IDocHostUIHandler*);
    }
    else if (IsEqualIID(riid, IID_IOleControlSite))
    {
        *ppvObj = SAFECAST(this, IOleControlSite*);
    }
    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = SAFECAST(this, IDispatch*);
    }
    else if (IsEqualIID(riid, IID_IInternetSecurityManager))
    {
        *ppvObj = SAFECAST(this, IInternetSecurityManager*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}


STDMETHODIMP_(ULONG) CSFVSite::AddRef()
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, m_cSite, this);

    DBG_ENTER(FTF_DEFVIEW, CSFVSite::AddRef);

    ULONG ul = pFrame->AddRef();

    DBG_EXIT_UL(FTF_DEFVIEW, CSFVSite::AddRef, ul);

    return(ul);
}


STDMETHODIMP_(ULONG) CSFVSite::Release()
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, m_cSite, this);

    DBG_ENTER(FTF_DEFVIEW, CSFVSite::Release);

    ULONG ul = pFrame->Release();

    DBG_EXIT_UL(FTF_DEFVIEW, CSFVSite::Release, ul);

    return(ul);
}


// IOleWindow
STDMETHODIMP CSFVSite::GetWindow(HWND *phwnd)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, m_cSite, this);
    return pFrame->GetWindow(phwnd);
}

STDMETHODIMP CSFVSite::ContextSensitiveHelp(BOOL fEnterMode)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, m_cSite, this);
    return pFrame->ContextSensitiveHelp(fEnterMode);
}

// IInternetSecurityManager
HRESULT CSFVSite::ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction, BYTE * pPolicy, DWORD cbPolicy, BYTE * pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved)
{
    HRESULT hr = INET_E_DEFAULT_ACTION;

    if ((((URLACTION_ACTIVEX_MIN <= dwAction) &&
          (URLACTION_ACTIVEX_MAX >= dwAction)) ||
         ((URLACTION_SCRIPT_MIN <= dwAction) &&
          (URLACTION_SCRIPT_MAX >= dwAction))) &&
        pContext &&
        (sizeof(CLSID) == cbContext) &&
        (IsEqualIID(*(CLSID *) pContext, CLSID_CDefViewOC) ||
        (IsEqualIID(*(CLSID *) pContext, CLSID_ThumbCtl))))
    {
        if (EVAL(pPolicy) && EVAL(sizeof(DWORD) == cbPolicy))
        {
            *pPolicy = (DWORD) URLPOLICY_ALLOW;
            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    return hr;
}

// IOleInPlaceSite
STDMETHODIMP CSFVSite::CanInPlaceActivate(void)
{
    return S_OK;
}


STDMETHODIMP CSFVSite::OnInPlaceActivate(void)
{
    TraceMsg(TF_FOCUS, "sfvf.oipa: m_pAct=%x",
        IToClass(CSFVFrame, m_cSite, this)->m_pActive);
    IToClass(CSFVFrame, m_cSite, this)->m_uState = SVUIA_ACTIVATE_NOFOCUS;
    return S_OK;
}


STDMETHODIMP CSFVSite::OnUIActivate(void)
{
    HRESULT hr;
    CSFVFrame* pFrame = IToClass(CSFVFrame, m_cSite, this);
    CDefView* pView = IToClass(CDefView, m_cFrame.m_cSite, this);

    hr = pView->_OnViewWindowActive();

    pFrame->m_uState = SVUIA_ACTIVATE_FOCUS;
    TraceMsg(TF_FOCUS, "sfvf.ouia: m_pAct'=%x",
        IToClass(CSFVFrame, m_cSite, this)->m_pActive);
    return hr;
}


STDMETHODIMP CSFVSite::GetWindowContext(
    /* [out] */ IOleInPlaceFrame **ppFrame,
    /* [out] */ IOleInPlaceUIWindow **ppDoc,
    /* [out] */ LPRECT lprcPosRect,
    /* [out] */ LPRECT lprcClipRect,
    /* [out][in] */ LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, m_cSite, this);
    CDefView* pView = IToClass(CDefView, m_cFrame, pFrame);

    *ppFrame = pFrame; pFrame->AddRef();
    *ppDoc = NULL; // indicating that doc window == frame window

    GetClientRect(pView->_hwndView, lprcPosRect);
    *lprcClipRect = *lprcPosRect;

    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->hwndFrame = pView->_hwndView;   // Yes, should be view window
    lpFrameInfo->haccel = NULL;
    lpFrameInfo->cAccelEntries = 0;

    return S_OK;
}


STDMETHODIMP CSFVSite::Scroll(SIZE scrollExtant)
{
    return S_OK;
}

STDMETHODIMP CSFVSite::OnUIDeactivate(BOOL fUndoable)
{
    return S_OK;
}

STDMETHODIMP CSFVSite::OnInPlaceDeactivate(void)
{
    return S_OK;
}

STDMETHODIMP CSFVSite::DiscardUndoState(void)
{
    return S_OK;
}

STDMETHODIMP CSFVSite::DeactivateAndUndo(void)
{
    return S_OK;
}

STDMETHODIMP CSFVSite::OnPosRectChange(LPCRECT lprcPosRect)
{
    return S_OK;
}

// IOleClientSite
STDMETHODIMP CSFVSite::SaveObject(void)
{
    return S_OK;
}

STDMETHODIMP CSFVSite::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    return E_FAIL;
}

STDMETHODIMP CSFVSite::GetContainer(IOleContainer **ppContainer)
{
    *ppContainer = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CSFVSite::ShowObject(void)
{
    return NOERROR;
}

STDMETHODIMP CSFVSite::OnShowWindow(BOOL fShow)
{
    return S_OK;
}

STDMETHODIMP CSFVSite::RequestNewObjectLayout(void)
{
    return S_OK;
}

// IOleDocumentSite
STDMETHODIMP CSFVSite::ActivateMe(IOleDocumentView *pviewToActivate)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, m_cSite, this);
    CDefView* pView = IToClass(CDefView, m_cFrame, pFrame);

    if (pView->_uState == SVUIA_ACTIVATE_FOCUS)
        pviewToActivate->UIActivate(TRUE);
    pviewToActivate->Show(TRUE);
    return S_OK;
}


//
// IOleCommandTarget stuff - just forward to _psb
//
STDMETHODIMP CSFVSite::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, m_cSite, this);
    CDefView* pView = IToClass(CDefView, m_cFrame, pFrame);
    IOleCommandTarget* pct;
    HRESULT hres = OLECMDERR_E_UNKNOWNGROUP;

    if (SUCCEEDED(pView->_psb->QueryInterface(IID_IOleCommandTarget, (void **)&pct)))
    {
        hres = pct->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
        pct->Release();
    }

    return hres;
}
HRESULT _VariantBSTR(LPCWSTR psz, VARIANTARG *pvarargOut)
{
    HRESULT hr;
    pvarargOut->bstrVal = SysAllocString(psz);
    if (pvarargOut->bstrVal)
    {
        pvarargOut->vt = VT_BSTR;
        hr = S_OK;
    }
    else
    {
        pvarargOut->vt = VT_ERROR;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT _VariantBSTRT(LPCTSTR psz, VARIANTARG *pvarargOut)
{
    HRESULT hr;
    pvarargOut->bstrVal = SysAllocStringT(psz);
    if (pvarargOut->bstrVal)
    {
        pvarargOut->vt = VT_BSTR;
        hr = S_OK;
    }
    else
    {
        pvarargOut->vt = VT_ERROR;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CSFVSite::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, m_cSite, this);
    CDefView* pView = IToClass(CDefView, m_cFrame, pFrame);
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;

    if (pguidCmdGroup)
    {
        if (IsEqualIID(*pguidCmdGroup, CGID_DefView))
        {
            hr = E_INVALIDARG;
            if (pvarargOut) 
            {
                VariantClear(pvarargOut);
                TCHAR szPath[MAX_PATH];

                switch (nCmdID)
                {
                case DVCMDID_GETTHISDIRPATH:
                case DVCMDID_GETTHISDIRNAME:
                    hr = pView->_GetNameAndFlags(nCmdID == DVCMDID_GETTHISDIRPATH ? 
                            SHGDN_FORPARSING : SHGDN_INFOLDER, 
                            szPath, ARRAYSIZE(szPath), NULL);
                    if (SUCCEEDED(hr))
                    {
                        hr = _VariantBSTRT(szPath, pvarargOut);
                    }
                    break;

                case DVCMDID_GETTEMPLATEDIRNAME:
                    if (pFrame->IsWebView()) 
                    {
                        SFVVIEWSDATA* pView;
                        pFrame->GetViewIdFromGUID(&VID_WebView, &pView);
                        if (pView) 
                        {
                            hr = _VariantBSTR(pView->wszMoniker, pvarargOut);
                        }
                    }
                    break;
                }
            }
            return hr;
        }
        // fall through on other cmd groups...
    }
    else if ((OLECMDID_SETTITLE == nCmdID) && !pView->_fCanActivateNow)
    {
        // NT #282632: Don't forward this message if we aren't the active view.
        return S_OK;
    }

    IOleCommandTarget* pct;
    if (SUCCEEDED(pView->_psb->QueryInterface(IID_IOleCommandTarget, (void **)&pct)))
    {
        hr = pct->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
        pct->Release();
    }

    return hr;
}

//***   IOleControlSite {

//***   IsVK_TABCycler -- is key a TAB-equivalent
// ENTRY/EXIT
//  dir     0 if not a TAB, non-0 if a TAB
// NOTES
//  NYI: -1 for shift+tab, 1 for tab
//
int IsVK_TABCycler(MSG *pMsg)
{
    int nDir = 0;

    if (!pMsg)
        return nDir;

    if (pMsg->message != WM_KEYDOWN)
        return nDir;
    if (! (pMsg->wParam == VK_TAB || pMsg->wParam == VK_F6))
        return nDir;

    nDir = (GetKeyState(VK_SHIFT) < 0) ? -1 : 1;

#ifdef KEYBOARDCUES
    SendMessage(GetParent(pMsg->hwnd), WM_CHANGEUISTATE,
        MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS), 0);
#endif
    
    return nDir;
}

//***   CSFVSite::TranslateAccelerator (IOCS::TranslateAccelerator)
// NOTES
//  (following comment/logic stolen from shdocvw/dochost.cpp)
//  trident (or any other DO that uses IOCS::TA) calls us back when TABing
//  off the last link.  to handle it, we flag it for our original caller
//  (IOIPAO::TA), and then pretend we handled it by telling trident S_OK.
//  trident returns S_OK to IOIPAO::TA, which checks the flag and says
//  'trident did *not* handle it' by returning S_FALSE.  that propagates
//  way up to the top where it sees it was a TAB so it does a CycleFocus.
//
//  that's how we do it when we're top-level.  when we're a frameset, we
//  need to do it the 'real' way, sending it up to our parent IOCS.
HRESULT CSFVSite::TranslateAccelerator(MSG __RPC_FAR *pMsg,DWORD grfModifiers)
{
    if (IsVK_TABCycler(pMsg)) {
        CSFVFrame* pFrame = IToClass(CSFVFrame, m_cSite, this);
        CDefView* pView = IToClass(CDefView, m_cFrame, pFrame);

        TraceMsg(TF_FOCUS, "csfvs::IOCS::TA(wParam=VK_TAB) ret _fCycleFocus=TRUE hr=S_OK (lie)");
        // defer it, set flag for cdv::IOIPAO::TA, and pretend we handled it
        ASSERT(!pView->_fCycleFocus);
        pView->_fCycleFocus = TRUE;
        return S_OK;
    }
    //ASSERT(!pView->_fCycleFocus);
    return S_FALSE;
}

// }

//
// IServiceProvider
//
HRESULT CSFVSite::QueryService(REFGUID guidService, REFIID riid, void ** ppvObj)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, m_cSite, this);
    CDefView* pView = IToClass(CDefView, m_cFrame, pFrame);

    if (IsEqualGUID(guidService, SID_DefView))
    {
        if (riid == IID_IDefViewFrame && pView->_IsDesktop()) 
        {
            *ppvObj = NULL;
            return E_FAIL; 
        } 
        // Try site QI
        if (SUCCEEDED(QueryInterface(riid, ppvObj))) 
            return S_OK;
    }

    // Delegate to defview QS
    if (SUCCEEDED(pView->QueryService(guidService, riid, ppvObj)))
        return S_OK;
    
    // Look for IID_IInternetSecurityManager
    if (IsEqualGUID(guidService, IID_IInternetSecurityManager))
    {
        ASSERT(riid == IID_IInternetSecurityManager);
        return QueryInterface(riid, ppvObj);
    }
    
    *ppvObj = NULL;
    return E_FAIL;
}

HRESULT CSFVSite::Invoke(DISPID dispidMember, REFIID iid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams,
                        VARIANT * pVarResult,EXCEPINFO * pexcepinfo,UINT * puArgErr)
{
    if (!pVarResult)
        return E_INVALIDARG;

    //Get pointer to the defview
    CSFVFrame* pFrame = IToClass(CSFVFrame, m_cSite, this);
    CDefView* pView = IToClass(CDefView, m_cFrame, pFrame);

    // We handle the query of whether we want to show images, ourselves.
    if (wFlags == DISPATCH_PROPERTYGET)
    {
        switch (dispidMember)
        {
            case DISPID_AMBIENT_DLCONTROL:
            {
                // Do the following only if this is NOT the desktop. 
                // (Because Desktop is in offline mode, it should 
                // return DLCTL_OFFLINEIFNOTCONNECTED flag). The following code
                // should be executed only for NON-desktop folders.
                if(!(pView->_IsDesktop()))
                {
                    pVarResult->vt = VT_I4;
                    pVarResult->lVal = DLCTL_DLIMAGES | DLCTL_VIDEOS;
                    return S_OK;
                }
            }
        }
    }

    // We delegate all other queries to shdocvw.
    if (!m_peds)
    {
        IServiceProvider *psp;

        if (SUCCEEDED(pView->_psb->QueryInterface(IID_IServiceProvider, (void **)&psp)))
        {
            psp->QueryService(IID_IExpDispSupport, IID_IExpDispSupport, (void **)&m_peds);
            psp->Release();
        }
    }

    if (!m_peds)
        return E_NOTIMPL;

    return m_peds->OnInvoke(dispidMember, iid, lcid, wFlags, pdispparams, pVarResult,pexcepinfo,puArgErr);
}

//
//  This is a proxy IDropTarget object, which wraps Trident's droptarget.
//
#define STEAL_ALL_DRAGDROP

HRESULT CHostDropTarget::QueryInterface(REFIID riid, void ** ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CHostDropTarget, IDropTarget),  // IID_IDropTarget
        { 0 }
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CHostDropTarget::AddRef(void)
{
    CSFVSite* pcsfvs = IToClass(CSFVSite, _dt, this);
    return pcsfvs->AddRef();
}

ULONG CHostDropTarget::Release(void)
{
    CSFVSite* pcsfvs = IToClass(CSFVSite, _dt, this);
    return pcsfvs->Release();
}

HRESULT CHostDropTarget::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
#ifdef STEAL_ALL_DRAGDROP
    return _pdtFrame->DragEnter(pdtobj, grfKeyState, pt, pdwEffect);
#else
    ASSERT(_pdtDoc);
    ASSERT(_pdtFrame);
    DWORD dwEffectFrame = *pdwEffect;     // Copy it for frame's droptarget

    // Call the document first (the order is NOT significant).
    HRESULT hres = _pdtDoc->DragEnter(pdtobj, grfKeyState, pt, pdwEffect);
    if (SUCCEEDED(hres)) {
        // Let the frame know that we are entering ALWAYS.
        _pdtFrame->DragEnter(pdtobj, grfKeyState, pt, &dwEffectFrame);
    
        // If the document refuses it, returns the flags from the frame.
        if (*pdwEffect == 0)
            *pdwEffect = dwEffectFrame;
    }

    DebugMsg(DM_DOCHOSTUIHANDLER, TEXT("CHDT::DragEnter returning %x with %x"),
             hres, *pdwEffect);

    return hres;
#endif
}

HRESULT CHostDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
#ifdef STEAL_ALL_DRAGDROP
    return _pdtFrame->DragOver(grfKeyState, pt, pdwEffect);
#else
    ASSERT(_pdtDoc);
    ASSERT(_pdtFrame);
    DWORD dwEffectFrame = *pdwEffect;     // Copy it for frame's droptarget

    // Call the document first (the order is significant).
    HRESULT hres = _pdtDoc->DragOver(grfKeyState, pt, pdwEffect);

    // If the document accepts it, let it do. 
    if (hres==S_OK && *pdwEffect) {
        DebugMsg(DM_DOCHOSTUIHANDLER, TEXT("CHDT::DragEnter returning %x with %x (Doc)"),
             hres, *pdwEffect);
        return S_OK;
    }

    // Let the frame process it.
    *pdwEffect = dwEffectFrame;
    hres = _pdtFrame->DragOver(grfKeyState, pt, pdwEffect);
    DebugMsg(DM_DOCHOSTUIHANDLER, TEXT("CHDT::DragEnter returning %x with %x (Frame)"),
             hres, *pdwEffect);
    return hres;
#endif
}

HRESULT CHostDropTarget::DragLeave(void)
{
#ifdef STEAL_ALL_DRAGDROP
    return _pdtFrame->DragLeave();
#else
    DebugMsg(DM_DOCHOSTUIHANDLER, TEXT("CHDT::DragLeave called"));

    ASSERT(_pdtDoc);
    ASSERT(_pdtFrame);
    _pdtDoc->DragLeave();
    _pdtFrame->DragLeave();

    return S_OK;
#endif
}

HRESULT CHostDropTarget::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
#ifdef STEAL_ALL_DRAGDROP
    return _pdtFrame->Drop(pdtobj, grfKeyState, pt, pdwEffect);
#else
    ASSERT(_pdtDoc);
    ASSERT(_pdtFrame);
    DWORD dwEffectFrame = *pdwEffect;     // Copy it for frame's droptarget

    // Check if the document wants to accept it or not.
    HRESULT hres = _pdtDoc->DragOver(grfKeyState, pt, pdwEffect);

    if (hres==S_OK && *pdwEffect) {
        // Document accepts it.
        hres = _pdtDoc->Drop(pdtobj, grfKeyState, pt, pdwEffect);
        _pdtFrame->DragLeave();
        DebugMsg(DM_DOCHOSTUIHANDLER, TEXT("CHDT::Drop returning %x with %x (Doc)"),
                 hres, *pdwEffect);
    } else {
        // Let the frame process it.
        *pdwEffect = dwEffectFrame;
        hres = _pdtFrame->Drop(pdtobj, grfKeyState, pt, pdwEffect);
        _pdtDoc->DragLeave();
        DebugMsg(DM_DOCHOSTUIHANDLER, TEXT("CHDT::Drop returning %x with %x (Frame)"),
                 hres, *pdwEffect);
    }

    return hres;
#endif
}


// We want to QueryInterface only to some interfaces, but want
// the AddRef and Release to be for the whole object

STDMETHODIMP CSFVFrame::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CSFVFrame, IOleWindow, IOleInPlaceFrame),  // IID_IOleWindow
        QITABENTMULTI(CSFVFrame, IOleInPlaceUIWindow, IOleInPlaceFrame),  // IID_IOleInPlaceUIWindow
        QITABENT(CSFVFrame, IOleInPlaceFrame),        // IID_IOleInPlaceFrame
        QITABENT(CSFVFrame, IAdviseSink),             // IID_IAdviseSink
        QITABENT(CSFVFrame, IPropertyNotifySink),     // IID_IPropertyNotifySink
        { 0 }
    };
    return QISearch(this, qit, riid, ppvObj);
}


STDMETHODIMP_(ULONG) CSFVFrame::AddRef()
{
    CDefView* pView = IToClass(CDefView, m_cFrame, this);
    return pView->AddRef();
}


STDMETHODIMP_(ULONG) CSFVFrame::Release()
{
    CDefView* pView = IToClass(CDefView, m_cFrame, this);
    return pView->Release();
}


// IOleWindow
STDMETHODIMP CSFVFrame::GetWindow(HWND *phwnd)
{
    CDefView* pView = IToClass(CDefView, m_cFrame, this);
    return pView->GetWindow(phwnd);
}


STDMETHODIMP CSFVFrame::ContextSensitiveHelp(BOOL fEnterMode)
{
    CDefView* pView = IToClass(CDefView, m_cFrame, this);
    return pView->ContextSensitiveHelp(fEnterMode);
}


// IOleInPlaceUIWindow
STDMETHODIMP CSFVFrame::GetBorder(LPRECT lprectBorder)
{
    CDefView* pView = IToClass(CDefView, m_cFrame, this);
    GetClientRect(pView->_hwndView, lprectBorder);
    return S_OK;
}


STDMETHODIMP CSFVFrame::RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    return INPLACE_E_NOTOOLSPACE;
}


STDMETHODIMP CSFVFrame::SetBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    return INPLACE_E_NOTOOLSPACE;
}


STDMETHODIMP CSFVFrame::SetActiveObject(
    /* [unique][in] */ IOleInPlaceActiveObject *pActiveObject,
    /* [unique][string][in] */ LPCOLESTR pszObjName)
{
    TraceMsg(TF_FOCUS, "sfvf.sao(pAct'=%x): m_pAct=%x", pActiveObject, m_pActive);
    if (pActiveObject != m_pActive)
    {
        IAdviseSink* pOurSink = SAFECAST(this, IAdviseSink*);
#ifdef DEBUG
        QueryInterface(IID_IAdviseSink, (void **)&pOurSink);
#endif

        if (m_pActive)
        {
            //
            // if we had an OLE view object then disconnect our advise sink and
            // release the view object.
            //
            if (m_pvoActive)
            {
                IAdviseSink *pSink;
                if (SUCCEEDED(m_pvoActive->GetAdvise(NULL, NULL, &pSink)))
                {
                    // Be polite, only blow away the advise if we're the
                    // one who's listening
                    if (pSink == pOurSink)
                    {
                        m_pvoActive->SetAdvise(0, 0, NULL);
                    }

                    // If there was no sink, GetAdvise succeeds but sets
                    // pSink to NULL, so need to check pSink here.
                    if (pSink)
                        pSink->Release();
                }
                ATOMICRELEASE(m_pvoActive);
            }

            ATOMICRELEASE(m_pActive);
        }
    
        m_pActive = pActiveObject;
    
        if (m_pActive)
        {
            m_pActive->AddRef();

            //
            // try to get an OLE view object and set up an advisory connection.
            //
            if (SUCCEEDED(m_pActive->QueryInterface(IID_IViewObject,
                (void **)&m_pvoActive)))
            {
                ASSERT(m_pvoActive);
                m_pvoActive->SetAdvise(DVASPECT_CONTENT, 0, pOurSink);
            }
        }

        //
        // since we changed the active view, tell our owner that the content
        // may have changed...
        //
        OnViewChange(DVASPECT_CONTENT, -1);

#ifdef DEBUG
        ATOMICRELEASE(pOurSink);
#endif
    }

    return S_OK;
}


// IOleInPlaceFrame
STDMETHODIMP CSFVFrame::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    if (hmenuShared)
    {
        // No menu merging
        // or fill lpMenuWidths with 0 and return success
        lpMenuWidths->width[0] = 0;
        lpMenuWidths->width[2] = 0;
        lpMenuWidths->width[4] = 0;
    }
    return NOERROR;
}


STDMETHODIMP CSFVFrame::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    return NOERROR;    // No menu merging
}

STDMETHODIMP CSFVFrame::RemoveMenus(HMENU hmenuShared)
{
    return E_FAIL;      // No menu merging
}

//
//  This one is a bit tricky.  If the client wants to clear the status
//  area, then restore it to the original defview status area text.
//
//  For example, in webview, MSHTML will keep clearing the status area
//  whenever you are not over a link, which would otherwise keep erasing
//  the "n object(s) selected" message from defview.
//
//  To really clear the status area, set the text to " " instead of "".
//
STDMETHODIMP CSFVFrame::SetStatusText(LPCOLESTR pszStatusText)
{
    CDefView* pView = IToClass(CDefView, m_cFrame, this);

    if (pszStatusText && pszStatusText[0])
        return(pView->_psb->SetStatusTextSB(pszStatusText));

    // Client had nothing to say, so let defview set it instead - see if the extended view handles the text first.
    if (!m_pDefViewExtInit2 || FAILED(m_pDefViewExtInit2->SetStatusText(pszStatusText)))
    {
        DV_UpdateStatusBar(pView, FALSE);
    }
    return S_OK;
}


STDMETHODIMP CSFVFrame::EnableModeless(BOOL fEnable)
{
    CDefView* pView = IToClass(CDefView, m_cFrame, this);

    if (pView->_IsDesktop())
    {
        if(fEnable)
        {
            pView->_fDesktopModal = FALSE;
            if (pView->_fDesktopRefreshPending)  //Was a refresh pending?
            {
                pView->_fDesktopRefreshPending = FALSE;
                //Let's do a refresh asynchronously. 
                PostMessage(pView->_hwndView, WM_KEYDOWN, (WPARAM)VK_F5, 0);
            }
            TraceMsg(TF_DEFVIEW, "A Modal dlg is going away!");
        }
        else
        {
            pView->_fDesktopModal = TRUE;
            TraceMsg(TF_DEFVIEW, "A Modal dlg is coming up for Desktop!");
        }
    }

    return(pView->_psb->EnableModelessSB(fEnable));
}

STDMETHODIMP CSFVFrame::TranslateAccelerator(LPMSG lpmsg,WORD wID)
{
    CDefView* pView = IToClass(CDefView, m_cFrame, this);
    return pView->_psb->TranslateAcceleratorSB(lpmsg, wID);
}

// IAdviseSink
void CSFVFrame::OnDataChange(FORMATETC *, STGMEDIUM *)
{
}

void CSFVFrame::OnViewChange(DWORD dwAspect, LONG lindex)
{
    if (IsWebView() && m_pvoActive)
    {
        CDefView *pView = IToClass(CDefView, m_cFrame, this);
        pView->PropagateOnViewChange(dwAspect, lindex);
    }
}

void CSFVFrame::OnRename(IMoniker *)
{
}


void CSFVFrame::OnSave()
{
}


void CSFVFrame::OnClose()
{
    if (IsWebView() && m_pvoActive)
    {
        CDefView *pView = IToClass(CDefView, m_cFrame, this);
        pView->PropagateOnClose();
    }
}

HRESULT CSFVFrame::OnChanged(DISPID dispid)
{
    if (DISPID_READYSTATE == dispid || DISPID_UNKNOWN == dispid)
    {
        IDispatch * p_idispatch;

        ASSERT(m_pOleObjReadyState);
        if (!m_pOleObjReadyState)
            return S_OK;  //Documentation says we need to return this all the time

        if (SUCCEEDED(m_pOleObjReadyState->QueryInterface( IID_IDispatch, (void **) &p_idispatch)))
        {
            CDefView* pView = IToClass(CDefView, m_cFrame, this);
            VARIANTARG va;
            EXCEPINFO exInfo;

            va.vt = 0;
            if (EVAL(SUCCEEDED(p_idispatch->Invoke( DISPID_READYSTATE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &g_dispparamsNoArgs, &va, &exInfo, NULL))
                && va.vt == VT_I4))
            {
                if(va.lVal >= READYSTATE_INTERACTIVE)
                {
                    if(!m_fReadyStateInteractiveProcessed)
                    {
                
                        m_fReadyStateInteractiveProcessed = TRUE;
#if 0
                        if (pView->_pvidPending)
                            // if we fail this, there will be an active ViewMode to fall back on.
                            pView->_SwitchToViewPVID(pView->_pvidPending, FALSE);
#endif

                        // First time through this function we need to request
                        // activation.  After that, we can switch immediately.
                        //
                        // Switch the bit early since SHDVID_ACTIVATEMENOW calls
                        // SHDVID_CANACTIVATENOW which checks it.
                        //
                        BOOL fTmp = !pView->_fCanActivateNow;
                        pView->_fCanActivateNow = TRUE;
                        if (fTmp)
                        {
                            // If we did an async CreateViewWindow2, then we
                            // need to notify the browser that we're ready
                            // to be activated - it will uiactivate us which
                            // will cause us to switch.

                            //Don't Make the View Visible, if it is in
                            //DEACTIVATE State. The View would be made visible
                            //during uiactivate call. - KishoreP 

                            if (pView->_uState != SVUIA_DEACTIVATE)
                            {
                                SetWindowPos(pView->_hwndView, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
                            }
                            IUnknown_Exec(pView->_psb, &CGID_ShellDocView, SHDVID_ACTIVATEMENOW, NULL, NULL, NULL);
                        }
                        else
                        {
                            // Technically we only want to do this iff our view is currently
                            // the active visible view.  !fCanActivateNow => we are definitely not visible,
                            // but _fCanActivateNow does NOT imply visible, it implies we are ready to be
                            // made the active visible guy, and that we've requested to be made the active
                            // visible view, but not necessarily that we *are* the active visible view.  If
                            // the previous view wasn't ready to go away, then we are in limbo.  But if that's
                            // the case, then our menus aren't merged, so there's no way the user could
                            // switch views on us...  Verify this.
#ifdef DEBUG
                            CDefView* pView = IToClass(CDefView, m_cFrame, this);
                            IShellView* psvCurrent;
                            if (EVAL(SUCCEEDED(pView->_psb->QueryActiveShellView(&psvCurrent))))
                            {
                                ASSERT(SHIsSameObject(SAFECAST(pView, IShellView2*), psvCurrent));
                                psvCurrent->Release();
                            }

                            ASSERT(pView->_uState != SVUIA_DEACTIVATE)
#endif
                        
                            // If we're simply switching views, go ahead
                            // and do it.
                            _SwitchToNewOleObj();
                        }

                        if (g_dwProfileCAP & 0x00010000)
                            StopCAP();
                    }
                }
            }
            ATOMICRELEASE(p_idispatch);

            if(va.lVal == READYSTATE_COMPLETE)
            {
                _UpdateZonesStatusPane(m_pOleObjReadyState);
                
                _RemoveReadyStateNotifyCapability();

                m_fReadyStateComplete = TRUE;
                pView->_PostEnumDoneMessage();

                pView->SelectSelectedItems();  // we may have had a selection pending,
                                              // do the selection thing on complete.
            }    
        }
    }

    return S_OK;
}

HRESULT CSFVFrame::OnRequestEdit(DISPID dispid)
{
    return E_NOTIMPL;
}


//
// query the current extended view.  
//
HRESULT CSFVFrame::GetView(SHELLVIEWID* pvid, ULONG uView)
{
    // Make sure we are up-to-date
    GetExtViews();

    SFVVIEWSDATA *pView;

    switch (uView)
    {
    case SV2GV_CURRENTVIEW: // query the current extended view.  
                            // THIS ONLY DEALS WITH NON DOCOBJ EXTENDED VIEWS!!!!
        if (m_uView < 0)
        {
            return(S_FALSE);
        }
        pView = m_lViews.GetPtr(m_lViews.NthUnique(m_uView+1));
        if (!pView)
        {
            return(S_FALSE);
        }
        *pvid = pView->idView;
        break;

    case SV2GV_ISEXTENDEDVIEW:
        if (pvid)
        {
            SFVVIEWSDATA* pView;
            for (int nExtView = 1; (pView = m_lViews.GetPtr(m_lViews.NthUnique(nExtView)));
                    ++nExtView)
            {
                if (IsEqualGUID(pView->idView, *pvid))
                    return NOERROR;
            }
        }
        return S_FALSE;

    case SV2GV_DEFAULTVIEW:
        return(m_lViews.GetDef(pvid) ? NOERROR : S_FALSE);

    default:
        if ((int)uView < 0)
            return(E_INVALIDARG);

        UINT uFirstExtView;
        if (m_bEnableStandardViews)
        {
            static SHELLVIEWID const * const c_pvidStd[] =
            {
                &VID_LargeIcons,
                &VID_SmallIcons,
                &VID_List,
                &VID_Details,
            };

            uFirstExtView = ARRAYSIZE(c_pvidStd);

            if (uView < uFirstExtView)
            {
                *pvid = *c_pvidStd[uView];
                break;
            }
        }
        else
            uFirstExtView = 0;

        SFVVIEWSDATA *pView =
            m_lViews.GetPtr(m_lViews.NthUnique(1 + uView - uFirstExtView));

        if (!pView)
        {
            // Must have gone off the end of the list
            return(S_FALSE);
        }

        *pvid = pView->idView;
        break;
    }

    return NOERROR;
}

// randum stuff
HRESULT CSFVFrame::GetCommandTarget(IOleCommandTarget** ppct)
{
    if (m_pDocView)
    {
        return(m_pDocView->QueryInterface(IID_IOleCommandTarget, (void **)ppct));
    }
    *ppct = NULL;
    return E_FAIL;
}

HRESULT CSFVFrame::SetRect(LPRECT prc)
{
    if (IsWebView() && m_pDocView)
        return m_pDocView->SetRect(prc);
    else if (IsSFVExtension())
    {
        MoveWindow(m_hActiveSVExtHwnd, prc->left, prc->top, prc->right - prc->left, prc->bottom - prc->top, TRUE);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT CSFVFrame::OnTranslateAccelerator(LPMSG pmsg, BOOL* pbTabOffLastTridentStop)
{
    HRESULT hr = E_FAIL;

    *pbTabOffLastTridentStop = FALSE;
    if (IsWebView())
    {
        if (m_pActive)
        {
            hr = m_pActive->TranslateAccelerator(pmsg);
        }
        else if (m_pOleObj)
        {
            IOleInPlaceActiveObject* pIAO;
            if (SUCCEEDED(m_pOleObj->QueryInterface(IID_IOleInPlaceActiveObject, (void **)&pIAO)))
            {
                hr = pIAO->TranslateAccelerator(pmsg);
                pIAO->Release();
            }
        }
        if (hr == S_OK)
        {
            CDefView* pView = IToClass(CDefView, m_cFrame, this);

            if (pView->_fCycleFocus)
            {
                // we got called back by trident (IOCS::TA), but deferred it.
                // time to pay the piper.
                *pbTabOffLastTridentStop = TRUE;
                TraceMsg(TF_FOCUS, "sfvf::IOIPAO::OnTA piao->TA==S_OK ret _fCycleFocus=FALSE hr=S_FALSE (piper)");
                pView->_fCycleFocus = FALSE;
                // _UIActivateIO(FALSE, NULL);
                hr = S_FALSE;       // time to pay the piper
            }
        }

        ASSERT(! IToClass(CDefView, m_cFrame, this)->_fCycleFocus);
    }

    return hr;
}

HRESULT CSFVFrame::_HasFocusIO()
{
    TraceMsg(TF_FOCUS, "sfvf._hfio: uState=%x hr=%x", m_uState, (m_uState == SVUIA_ACTIVATE_FOCUS) ? S_OK : S_FALSE);
    if (IsSFVExtension())
        return m_pActiveSVExt ? S_OK : S_FALSE;
    else if (IsWebView()) {
        return (m_uState == SVUIA_ACTIVATE_FOCUS) ? S_OK : S_FALSE;
    }
    return S_FALSE;
}

HRESULT CSFVFrame::_UIActivateIO(BOOL fActivate, MSG *pMsg)
{
    if (IsWebView()) {
        HRESULT hr;
        CSFVFrame* pFrame = IToClass(CSFVFrame, m_cSite, this);
        CDefView* pView = IToClass(CDefView, m_cFrame, pFrame);
        LONG iVerb;
        RECT rcView;

        if (fActivate) {
            iVerb = OLEIVERB_UIACTIVATE;
            m_uState = SVUIA_ACTIVATE_FOCUS;
        }
        else {
            iVerb = OLEIVERB_INPLACEACTIVATE;
            m_uState = SVUIA_ACTIVATE_NOFOCUS;
        }


#if 0
        // BUGBUG trident doesn't handle DoVerb(INPLACEACT) correctly
        GetClientRect(pView->_hwndView, &rcView);
        hr = m_pOleObj->DoVerb(iVerb, pMsg,
            &m_cSite, (UINT)-1, pView->_hwndView, &rcView);
#else
        if (fActivate) {
            GetClientRect(pView->_hwndView, &rcView);
            hr = m_pOleObj->DoVerb(iVerb, pMsg,
                &m_cSite, (UINT)-1, pView->_hwndView, &rcView);
        }
        else {
            IOleInPlaceObject *pipo;
            hr = m_pOleObj->QueryInterface(IID_IOleInPlaceObject, (void **)&pipo);
            if (SUCCEEDED(hr)) {
                hr = pipo->UIDeactivate();
                pipo->Release();
            }
        }
#endif

        ASSERT(SUCCEEDED(hr));
        ASSERT(m_uState == (UINT)(fActivate ? SVUIA_ACTIVATE_FOCUS : SVUIA_ACTIVATE_NOFOCUS));
        TraceMsg(TF_FOCUS, "sfvf._uiaio(fAct=%d) ExtView DoVerb S_OK", fActivate);
        return S_OK;
    }
    TraceMsg(TF_FOCUS, "sfvf._uiaio(fAct=%d) else S_FALSE", fActivate);
    return S_FALSE;
}

int CSFVFrame::CmdIdFromUid(int uid)
{
    for (int i = 0; i < ARRAYSIZE(m_mpCmdIdToUid); i++)
    {
        if (m_mpCmdIdToUid[i] == uid)
            return (i + SFVIDM_VIEW_EXTFIRST);
    }
    return -1;
}
