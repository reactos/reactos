/////////////////////////////////////////////////////////////////////////////
// PIDLLIST.CPP
//
// Implementation of CPIDLList.
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     12/09/96    Created
// jaym     01/13/97    Added Subscription additions to list.
// jaym     08/11/97    Updated to use Channels that are subscribed.
/////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "if\actsaver.h"
#include "regini.h"
#include "bsc.h"
#include "intshcut.h"
#include "pidllist.h"

#define TF_PIDLLIST     TF_ALWAYS
#define TF_PARSECDF     TF_ALWAYS

const CLSID CLSID_MDA = {0x72DDEF90,0xD243,0x11d0,{0x87,0x3B,0x00,0x80,0x5F,0x78,0x1E,0x79}};

BOOL CALLBACK ChannelAddToListCallback
(
    ISubscriptionMgr2 * pSubscriptionMgr2,
    CHANNELENUMINFO *   pci,
    int                 nItemNum,
    BOOL                bDefaultTopLevelURL,
    LPARAM              lParam
);

/////////////////////////////////////////////////////////////////////////////
// External variables
/////////////////////////////////////////////////////////////////////////////
extern BOOL         g_bPlatformNT;  // From ACTSAVER.CPP
extern IMalloc *    g_pMalloc;      // From ACTSAVER.CPP

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
#pragma data_seg(DATASEG_READONLY)
const WCHAR g_szChannelFlags[] = L"ChannelFlags";
const TCHAR g_szScreenSaverURL[] = TEXT("ScreenSaverURL");
const TCHAR g_szDesktopINI[] = TEXT("desktop.ini");
const TCHAR g_szChannel[] = TEXT("Channel");
const LPCWSTR g_pProps[] = { g_szChannelFlags };
#pragma data_seg()

/////////////////////////////////////////////////////////////////////////////
// CPIDLList
/////////////////////////////////////////////////////////////////////////////
CPIDLList::CPIDLList
(
)
{
    m_psfInternet = NULL;
    m_hdpaList = NULL;
}

CPIDLList::~CPIDLList
(
)
{
    if (m_psfInternet != NULL)
        EVAL(m_psfInternet->Release() == 0);

    if (m_hdpaList != NULL)
        Free();
}

/////////////////////////////////////////////////////////////////////////////
// CPIDLList::Build
/////////////////////////////////////////////////////////////////////////////
int CPIDLList::Build
(
    IScreenSaverConfig *    pSSConfig,
    BOOL                    bDefaultTopLevelURL
)
{
    // Empty the list and recreate it.
    Free();

    int iCount = EnumChannels(  ChannelAddToListCallback,
                                bDefaultTopLevelURL,
                                (LPARAM)this);

    return iCount;
}

/////////////////////////////////////////////////////////////////////////////
// CPIDLList::Add
/////////////////////////////////////////////////////////////////////////////
BOOL CPIDLList::Add
(
    LPITEMIDLIST pidl
)
{
    BOOL bAdded;

    if (bAdded = (Find(pidl) == -1))
    {
        if (m_hdpaList == NULL)
            m_hdpaList = DPA_Create(5);

        DPA_InsertPtr(m_hdpaList, 0, pidl);
    }

#ifdef _DEBUG
    TCHAR szPath[INTERNET_MAX_URL_LENGTH];
    GetPIDLDisplayName(NULL, pidl, szPath, INTERNET_MAX_URL_LENGTH, SHGDN_FORPARSING);

    if (bAdded)
        TraceMsg(TF_PIDLLIST, "Added %s to list", szPath);
    else
        TraceMsg(TF_PIDLLIST, "Rejected %s. Possibly already in the list", szPath);
#endif  // _DEBUG

    return bAdded;
}

/////////////////////////////////////////////////////////////////////////////
// CPIDLList::Add
/////////////////////////////////////////////////////////////////////////////
BOOL CPIDLList::Add
(
    LPTSTR pszURL
)
{
    CString strURL = pszURL;

    if (PathIsURL(strURL))
    {
        // Make sure that URL is completely escaped.
        char * szURL = strURL.GetBuffer(INTERNET_MAX_URL_LENGTH);
        DWORD cchSize = INTERNET_MAX_URL_LENGTH;
        UrlEscape(szURL, szURL, &cchSize, 0);
        strURL.ReleaseBuffer();
    }

    BOOL            bResult = FALSE;
    LPITEMIDLIST    pidlURL;

    if (CreatePIDLFromPath(strURL, &pidlURL))
    {
        // Add pidl to end of list
        if (Add(pidlURL))
            bResult = TRUE;
        else
        {
            // Not added, so throw it away.
            g_pMalloc->Free(pidlURL);
        }
    }
    else
        TraceMsg(TF_ERROR, "CreatePIDLFromPath FAILED!");
    
    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// CPIDLList::Free
/////////////////////////////////////////////////////////////////////////////
void CPIDLList::Free
(
)
{
    if (m_hdpaList == NULL)
        return;

    int cItems = DPA_GetPtrCount(m_hdpaList);
    for (int i = 0; i < cItems; i++)
    {
        LPITEMIDLIST pidl = (LPITEMIDLIST)DPA_GetPtr(m_hdpaList, i);

        ASSERT(pidl != NULL);

        g_pMalloc->Free(pidl);
    }

    EVAL(DPA_Destroy(m_hdpaList));
    m_hdpaList = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CPIDLList::ComparePIDLs
/////////////////////////////////////////////////////////////////////////////
int WINAPI CPIDLList::ComparePIDLs
(
    LPVOID pv1,
    LPVOID pv2,
    LPARAM lParam
)
{
    char *  pszURL1 = new char [INTERNET_MAX_URL_LENGTH];
    char *  pszURL2 = new char [INTERNET_MAX_URL_LENGTH];
    int     iResult = 0;

    ASSERT((pszURL1 != NULL) && (pszURL2 != NULL));

    if ((pszURL1 != NULL) && (pszURL2 != NULL))
    {
        EVAL(GetPIDLDisplayName(NULL, (LPITEMIDLIST)pv1, pszURL1, INTERNET_MAX_URL_LENGTH, SHGDN_FORPARSING));
        EVAL(GetPIDLDisplayName(NULL, (LPITEMIDLIST)pv2, pszURL2, INTERNET_MAX_URL_LENGTH, SHGDN_FORPARSING));

        iResult = lstrcmpi(pszURL1, pszURL2);
    }

    if (pszURL1)
        delete [] pszURL1;

    if (pszURL2)
        delete [] pszURL2;

    return iResult;
}

/////////////////////////////////////////////////////////////////////////////
// Helper functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// EnumChannels
/////////////////////////////////////////////////////////////////////////////
int EnumChannels
(
    CHANENUMCALLBACK    pfnec,
    BOOL                bDefaultTopLevelURL,
    LPARAM              lParam
)
{
    ISubscriptionMgr2 * pSubscriptionMgr2 = NULL;
    IChannelMgr *       pChannelMgr = NULL;
    IEnumChannels *     pChannelEnum = NULL;
    int                 iCount = 0;
    HRESULT             hrResult;

    HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    if  (
        SUCCEEDED(hrResult = CoCreateInstance( CLSID_SubscriptionMgr,
                                               NULL,
                                               CLSCTX_INPROC_SERVER,
                                               IID_ISubscriptionMgr2,
                                               (void **)&pSubscriptionMgr2))
        &&
        SUCCEEDED(hrResult = CoCreateInstance( CLSID_ChannelMgr,
                                               NULL,
                                               CLSCTX_INPROC_SERVER,
                                               IID_IChannelMgr,
                                               (void **)&pChannelMgr))
        &&
        SUCCEEDED(hrResult = pChannelMgr->EnumChannels( CHANENUM_CHANNELFOLDER 
                                                            | CHANENUM_SOFTUPDATEFOLDER
                                                            | CHANENUM_ALLDATA,
                                                        NULL,
                                                        &pChannelEnum))
        )
    {
        ULONG           celtFetched;
        CHANNELENUMINFO ciItem = { 0 };

        // Make sure the enumerator is reset.
        pChannelEnum->Reset();

        // Iterate through all the channels, making note of the URL subscriptions.
        while   (
                SUCCEEDED(hrResult = pChannelEnum->Next(1, &ciItem, &celtFetched))
                &&
                (celtFetched != 0)
                )
        {
            if (pfnec(  pSubscriptionMgr2,
                        &ciItem,
                        iCount,
                        bDefaultTopLevelURL, lParam))
            {
                iCount++;
            }

            // Cleanup and reset for next time around.
            if (ciItem.pszTitle != NULL)
                CoTaskMemFree(ciItem.pszTitle);

            if (ciItem.pszPath != NULL)
                CoTaskMemFree(ciItem.pszPath);

            if (ciItem.pszURL != NULL)
                CoTaskMemFree(ciItem.pszURL);
            
            ZeroMemory(&ciItem, SIZEOF(CHANNELENUMINFO));
        }
    }
    else
        ASSERT(FALSE);

    if (pChannelEnum != NULL)
        pChannelEnum->Release();

    if (pChannelMgr != NULL)
        EVAL(pChannelMgr->Release() == 0);

    if (pSubscriptionMgr2 != NULL)
        pSubscriptionMgr2->Release();

    if (hOldCursor != NULL)
        SetCursor(hOldCursor);

    return iCount;
}

/////////////////////////////////////////////////////////////////////////////
// ChannelAddToListCallback
/////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK ChannelAddToListCallback
(
    ISubscriptionMgr2 * pSubscriptionMgr2,
    CHANNELENUMINFO *   pci,
    int                 nItemNum,
    BOOL                bDefaultTopLevelURL,
    LPARAM              lParam
)
{
    BOOL    bResult = FALSE;
    ISubscriptionItem *pSubsItem;

    TraceMsg(TF_PIDLLIST, "pszURL = %S", (LPCTSTR)pci->pszURL);

    OLE2T(pci->pszPath, pszPath);
    TCHAR szDesktopINI[MAX_PATH];
    TCHAR szScreenSaverURL[INTERNET_MAX_URL_LENGTH];

    PathCombine(szDesktopINI, pszPath, g_szDesktopINI);

    if (GetPrivateProfileString(g_szChannel, 
                                g_szScreenSaverURL, 
                                TEXT(""), 
                                szScreenSaverURL,
                                ARRAYSIZE(szScreenSaverURL),
                                szDesktopINI) > 0)
    {
        HRESULT hr = pSubscriptionMgr2->GetItemFromURL(pci->pszURL, &pSubsItem);

        ASSERT((SUCCEEDED(hr) && pSubsItem) ||
               (FAILED(hr) && !pSubsItem));

        VARIANT vFlags = { 0 };

        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(pSubsItem->ReadProperties(1, 
                          &g_pProps[PROP_CHANNEL_FLAGS], &vFlags)) &&
                (V_VT(&vFlags) == VT_I4) &&
                (V_I4(&vFlags) & CHANNEL_AGENT_PRECACHE_SCRNSAVER))
            {
                bResult = ((CPIDLList *)(lParam))->Add(szScreenSaverURL);
            }

            pSubsItem->Release();
        }
    }
               
    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// GetPSFInternet
/////////////////////////////////////////////////////////////////////////////
#define SHID_ROOT_REGITEM   0x1F

IDLREGITEM c_idlURLRoot =
{
    {
        SIZEOF(IDREGITEM),
        SHID_ROOT_REGITEM,
        0,
        // CLSID_ShellInetRoot goes here (see code below)
    },
    0
};

HRESULT GetPSFInternet
(
    IShellFolder ** psfInternet
)
{
    HRESULT hrResult = S_OK;

    if (SUCCEEDED(hrResult = CoCreateInstance(  CLSID_CURLFolder,
                                                NULL,
                                                CLSCTX_INPROC_SERVER,
                                                IID_IShellFolder,
                                                (void **)psfInternet)))
    {
        IPersistFolder * pPersistFolder;

        // Set CLSID_ShellInetRoot for the shared struct.
        c_idlURLRoot.idri.clsid = CLSID_ShellInetRoot;

        if (SUCCEEDED(hrResult = (*psfInternet)->QueryInterface(IID_IPersistFolder,
                                                                (void **)&pPersistFolder)))
        {
            hrResult = pPersistFolder->Initialize((LPITEMIDLIST)&c_idlURLRoot);
            pPersistFolder->Release();
        }
    }

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CreatePIDLFromPath
/////////////////////////////////////////////////////////////////////////////
BOOL CreatePIDLFromPath
(
    LPTSTR          pszPath,
    LPITEMIDLIST *  ppidl
)
{
    IShellFolder *  psf = NULL;
    HRESULT         hrResult;

    PARSEDURL pu;
    pu.cbSize = sizeof(PARSEDURL);

    *ppidl = NULL;

    if  (
        FAILED(hrResult = ParseURL(pszPath, &pu))
        ||
        (pu.nScheme == URL_SCHEME_FILE)
        )
    {
        // Convert 'file:' URLs to system path's
        DWORD cbSize = INTERNET_MAX_URL_LENGTH;
        PathCreateFromUrl(pszPath, pszPath, &cbSize, 0);

        EVAL(SUCCEEDED(hrResult = SHGetDesktopFolder(&psf)));
    }
    else
    {
        // Assume Internet namespace URLs
        EVAL(SUCCEEDED(hrResult = GetPSFInternet(&psf)));
    }

    if  (
        SUCCEEDED(hrResult)
        &&
        (psf != NULL)
        )
    {
        WCHAR * pwszPath = new WCHAR [INTERNET_MAX_URL_LENGTH];

        if (NULL == pwszPath)
        {
            psf->Release();
            return FALSE;
        }

        MultiByteToWideChar(CP_ACP, 0, pszPath, -1, pwszPath, INTERNET_MAX_URL_LENGTH);
        EVAL(SUCCEEDED(hrResult = psf->ParseDisplayName(NULL, NULL, pwszPath, NULL, ppidl, NULL)));
        psf->Release();

        delete [] pwszPath;
    }

    return SUCCEEDED(hrResult);
}

/////////////////////////////////////////////////////////////////////////////
// GetPIDLDisplayName
/////////////////////////////////////////////////////////////////////////////
BOOL GetPIDLDisplayName
(
    IShellFolder *  psfRoot,
    LPCITEMIDLIST   pidl,
    LPTSTR          pszName,
    int             cbszName,
    int             fType
)
{
    IShellFolder *  psf;
    STRRET          srPath;
    HRESULT         hrResult = E_FAIL;

    if (psfRoot == NULL)
        EVAL(SUCCEEDED(SHGetDesktopFolder(&psf)));
    else
    {
        psf = psfRoot;
        psf->AddRef();
    }

    if  (
        FAILED(hrResult = psf->GetDisplayNameOf(pidl, fType, &srPath))
        &&
        (psfRoot == NULL)
        )
    {
        IShellFolder * psfInternet;
        if (SUCCEEDED(hrResult = GetPSFInternet(&psfInternet)))
        {
            hrResult = psfInternet->GetDisplayNameOf(pidl, fType, &srPath);
            psfInternet->Release();
        }
    }

    if (SUCCEEDED(hrResult))
    {
        if (g_bPlatformNT)
        {
            WCHAR * pwszName = new WCHAR [INTERNET_MAX_URL_LENGTH];

            if (NULL == pwszName)
            {
                psf->Release();
                return FALSE;
            }

            StrRetToStrN((TCHAR *)pwszName, INTERNET_MAX_URL_LENGTH, &srPath, pidl);
            WideCharToMultiByte(CP_ACP, 0, pwszName, -1, pszName, cbszName, NULL, NULL);

            delete [] pwszName;
        }
        else
            StrRetToStrN(pszName, cbszName, &srPath, pidl);
    }

    psf->Release();

    return SUCCEEDED(hrResult);
}

/////////////////////////////////////////////////////////////////////////////
// CopyPIDL
/////////////////////////////////////////////////////////////////////////////
LPITEMIDLIST CopyPIDL
(
    LPITEMIDLIST pidl
)
{ 
    // Get the size of the specified item identifier. 
    int cb = pidl->mkid.cb;      
    
    ASSERT(g_pMalloc != NULL);

    // Allocate a new item identifier list. 
    LPITEMIDLIST pidlNew = (LPITEMIDLIST) g_pMalloc->Alloc(cb + sizeof(USHORT)); 
    
    if (pidlNew == NULL)
        return NULL;  

    // Copy the specified item identifier.
    CopyMemory(pidlNew, pidl, cb);  

    // Append a terminating zero. 
    *((USHORT *) (((LPBYTE) pidlNew) + cb)) = 0;
    
    return pidlNew;
}

/////////////////////////////////////////////////////////////////////////////
// IntSiteHelper
/////////////////////////////////////////////////////////////////////////////
HRESULT IntSiteHelper
(
    LPCTSTR             pszURL,
    const PROPSPEC *    pPropspec,
    PROPVARIANT *       pPropvar,
    UINT                uPropVarArraySize,
    BOOL                fWrite)
{
    IUniformResourceLocator *   pURL = NULL;
    IPropertySetStorage *       pPropSetStg = NULL;
    IPropertyStorage *          pPropStg = NULL;
    HRESULT                     hrResult;

    for (;;)
    {
        if (FAILED(hrResult = CoCreateInstance( CLSID_InternetShortcut,
                                                NULL,
                                                CLSCTX_INPROC_SERVER,
                                                IID_IUniformResourceLocator,
                                                (void **)&pURL)))
        {
            break;
        }

        if (FAILED(hrResult = pURL->SetURL(pszURL, 0)))
            break;

        if (FAILED(hrResult = pURL->QueryInterface( IID_IPropertySetStorage,
                                                    (void **)&pPropSetStg)))
        {
            break;
        }

        if (FAILED(hrResult = pPropSetStg->Open(FMTID_InternetSite,
                                                STGM_READWRITE,
                                                &pPropStg)))
        {
            break;
        }

        if (fWrite)
        {
            hrResult = pPropStg->WriteMultiple( uPropVarArraySize,
                                                pPropspec,
                                                pPropvar,
                                                0);
            pPropStg->Commit(STGC_DEFAULT);
        }
        else
        {
            hrResult = pPropStg->ReadMultiple(  uPropVarArraySize,
                                                pPropspec,
                                                pPropvar);
        }

        break;
    }

    if (pPropStg != NULL)
        pPropStg->Release();

    if (pPropSetStg != NULL)
        pPropSetStg->Release();

    if (pURL != NULL)
        EVAL(pURL->Release() == 0);

    return hrResult;
}
