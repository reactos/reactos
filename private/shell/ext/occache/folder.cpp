#include "general.h"
#include "folder.h"
#include "utils.h"

#include <mluisupp.h>

#define CPP_FUNCTIONS
#include <crtfree.h>

// string displayed to represent missing data
TCHAR g_szUnknownData[64];

int CompareVersion(LPCTSTR lpszVersion1, LPCTSTR lpszVersion2);

///////////////////////////////////////////////////////////////////////////////
// IShellFolder methods

CControlFolder::CControlFolder()
{
    DebugMsg(DM_TRACE,TEXT("cf - CControlFolder() called."));
    m_cRef = 1;
    DllAddRef();

    // initialize g_szUnknownData, a string used to represent missing data
    if (g_szUnknownData[0] == 0)
        MLLoadString(IDS_UNKNOWNDATA, g_szUnknownData, ARRAYSIZE(g_szUnknownData));
}       

CControlFolder::~CControlFolder()
{
    Assert(m_cRef == 0);                 // should always have zero
    DebugMsg(DM_TRACE, TEXT("cf - ~CControlFolder() called."));
    DllRelease();
}    

STDAPI ControlFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;                     // null the out param

    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    CControlFolder *pCFolder = new CControlFolder;
    if (!pCFolder)
        return E_OUTOFMEMORY;

    HRESULT hr = pCFolder->QueryInterface(riid, ppvOut);
    pCFolder->Release();

    return hr;
}

HRESULT CControlFolder::QueryInterface(REFIID iid, void **ppv)
{
    DebugMsg(DM_TRACE, TEXT("cf - QueryInterface() called."));
    
    if ((iid == IID_IUnknown) || (iid == IID_IShellFolder))
    {
        *ppv = (void *)(IShellFolder*)this;
    }
    else if ((iid == IID_IPersistFolder) || (iid == IID_IPersist)) 
    {
        *ppv = (void *)(IPersistFolder*)this;
    }
    else if (iid == IID_IContextMenu)
    {
        *ppv = (void *)(IContextMenu*)this;
    }
    else if (iid == IID_IShellView)
    {
        // this is a total hack... return our view object from this folder
        //
        // the desktop.ini file for "Temporary Internet Files" has UICLSID={guid of this object}
        // this lets us implment only ths IShellView for this folder, leaving the IShellFolder
        // to the default file system. this enables operations on the pidls that are stored in
        // this folder that would otherwise faile since our IShellFolder is not as complete
        // as the default (this is the same thing the font folder does).
        //
        // to support this with defview we would either have to do a complete wrapper object
        // for the view implemenation, or add this hack that hands out the view object, this
        // assumes we know the order of calls that the shell makes to create this object
        // and get the IShellView implementation
        // 
        return ControlFolderView_CreateInstance(this, m_pidl, ppv);
    }
    else
    {
        *ppv = NULL;     // null the out param
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG CControlFolder::AddRef()
{
    return ++m_cRef;
}

ULONG CControlFolder::Release()
{
    if (--m_cRef)
        return m_cRef;

    delete this;
    return 0;   
}

HRESULT CControlFolder::ParseDisplayName(
                                    HWND hwndOwner, 
                                    LPBC pbcReserved,
                                    LPOLESTR lpszDisplayName, 
                                    ULONG *pchEaten,
                                    LPITEMIDLIST *ppidl, 
                                    ULONG *pdwAttributes)
{
    DebugMsg(DM_TRACE, TEXT("cf - sf - ParseDisplayName() called."));
    *ppidl = NULL;              // null the out param
    return E_FAIL;
}

HRESULT CControlFolder::EnumObjects(
                               HWND hwndOwner, 
                               DWORD grfFlags,
                               LPENUMIDLIST *ppenumIDList)
{
    DebugMsg(DM_TRACE, TEXT("cf - sf - EnumObjects() called."));

    // Cannot filter on grfFlags yet - Corel Gallery expects to receive
    // an empty enumerator.

    return CControlFolderEnum_CreateInstance(m_pidl, grfFlags, ppenumIDList);
}

HRESULT CControlFolder::BindToObject(
                                LPCITEMIDLIST pidl, 
                                LPBC pbcReserved,
                                REFIID riid, 
                                void **ppvOut)
{
    DebugMsg(DM_TRACE, TEXT("cf - sf - BindToObject() called."));
    *ppvOut = NULL;         // null the out param
    return E_FAIL;
}

HRESULT CControlFolder::BindToStorage(
                                   LPCITEMIDLIST pidl, 
                                   LPBC pbcReserved,
                                   REFIID riid, 
                                   void **ppv)
{
    DebugMsg(DM_TRACE, TEXT("cf - sf - BindToStorage() called."));
    *ppv = NULL;         // null the out param
    return E_NOTIMPL;
}

HRESULT CControlFolder::CompareIDs(
                              LPARAM lParam, 
                              LPCITEMIDLIST pidl1, 
                              LPCITEMIDLIST pidl2)
{
    DebugMsg(DM_TRACE, TEXT("cf - sf - CompareIDs() called."));

    int iRet;
    LPCONTROLPIDL pcpidl1 = (LPCONTROLPIDL)pidl1;
    LPCONTROLPIDL pcpidl2 = (LPCONTROLPIDL)pidl2;
    LPCSTR lpszStr[2] = {NULL, NULL};

    switch (lParam) {
    case SI_CONTROL:
        iRet = lstrcmpi(
                 GetStringInfo(pcpidl1, SI_CONTROL),      
                 GetStringInfo(pcpidl2, SI_CONTROL));
        break;

    case SI_VERSION:
        lpszStr[0] = GetStringInfo(pcpidl1, SI_VERSION);
        lpszStr[1] = GetStringInfo(pcpidl2, SI_VERSION);
        if (lstrcmp(lpszStr[0], g_szUnknownData) == 0)
                        iRet = -1;
                else if (lstrcmp(lpszStr[1], g_szUnknownData) == 0)
                        iRet = 1;
        else
            iRet = CompareVersion(lpszStr[0], lpszStr[1]);
        break;

    case SI_CREATION:
    case SI_LASTACCESS:
        {
            FILETIME time[2];
            GetTimeInfo(pcpidl1, (int)lParam, &(time[0]));
            GetTimeInfo(pcpidl2, (int)lParam, &(time[1]));
            iRet = CompareFileTime(&(time[0]), &(time[1]));
        }
        break;

    case SI_STATUS:
        iRet = GetStatus(pcpidl1) - GetStatus(pcpidl2);
        break;
            
    case SI_TOTALSIZE:
        {
            DWORD dwSize1 = GetSizeSaved((LPCONTROLPIDL)pidl1); 
            DWORD dwSize2 = GetSizeSaved((LPCONTROLPIDL)pidl2);
            iRet = (dwSize1 == dwSize2 ? 0 : (dwSize1 > dwSize2 ? 1 : -1));
        }
        break;

    default:
        iRet = -1;
    }

    return ResultFromShort((SHORT)iRet);
}

HRESULT CControlFolder::CreateViewObject(
                                    HWND hwndOwner, 
                                    REFIID riid, 
                                    void **ppvOut)
{
    HRESULT hres;

    DebugMsg(DM_TRACE, TEXT("cf - sf - CreateViewObject() called."));

    if (riid == IID_IShellView)
    {
        hres = ControlFolderView_CreateInstance(this, m_pidl, ppvOut);
    }
    else if (riid == IID_IContextMenu)
    {
        hres = ControlFolder_CreateInstance(NULL, riid, ppvOut);
    }
    else
    {
        *ppvOut = NULL;         // null the out param
        hres = E_NOINTERFACE;
    }
    
    return hres;    
}

HRESULT CControlFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *prgfInOut)
{
    // BUGBUG: Should we initialize this for each item in here?  In other words,
    // if cidl > 1, then we should initialize each entry in the prgInOut array
    
    *prgfInOut = SFGAO_CANCOPY | SFGAO_HASPROPSHEET | SFGAO_CANMOVE | SFGAO_CANDELETE;
    return NOERROR;
}

HRESULT CControlFolder::GetUIObjectOf(
                                 HWND hwndOwner, 
                                 UINT cidl, 
                                 LPCITEMIDLIST *apidl,
                                 REFIID riid, 
                                 UINT *prgfInOut, 
                                 void **ppvOut)
{
    HRESULT hres;

    if ((riid == IID_IDataObject) || 
        (riid == IID_IExtractIcon) || 
        (riid == IID_IContextMenu))
    {
       hres = CControlItem_CreateInstance(this, cidl, apidl, riid, ppvOut);
    }
    else
    {
        *ppvOut = NULL;         // null the out param
        hres = E_FAIL;
    }
    return hres;    
}

HRESULT CControlFolder::GetDisplayNameOf(
                                    LPCITEMIDLIST pidl, 
                                    DWORD uFlags, 
                                    LPSTRRET lpName)
{
    DebugMsg(DM_TRACE, TEXT("cf - sf - GetDisplayNameOf() called."));
    
    lpName->uType = STRRET_CSTR;

    // for the history, we'll use the title if we have one, otherwise we'll use
    // the url filename.
    if (uFlags & SHGDN_FORPARSING)
        lstrcpyn(
             lpName->cStr, 
             GetStringInfo((LPCONTROLPIDL)pidl, SI_LOCATION), 
             ARRAYSIZE(lpName->cStr));
    else
        lstrcpyn(
             lpName->cStr, 
             GetStringInfo((LPCONTROLPIDL)pidl, SI_CONTROL), 
             ARRAYSIZE(lpName->cStr));

    return NOERROR;    
}

HRESULT CControlFolder::SetNameOf(
                             HWND hwndOwner, 
                             LPCITEMIDLIST pidl,
                             LPCOLESTR lpszName, 
                             DWORD uFlags, 
                             LPITEMIDLIST *ppidlOut)
{
    DebugMsg(DM_TRACE, TEXT("cf - sf - SetNameOf() called."));
    
    *ppidlOut = NULL;               // null the out param
    return E_FAIL;    
}

///////////////////////////////////////////////////////////////////////////////
// Helper functions

int CompareVersion(LPCTSTR lpszVersion1, LPCTSTR lpszVersion2)
{
    LPCTSTR pszVerNum[2] = {lpszVersion1, lpszVersion2};
        int nVerNum[2];
        int nResult = 0;

        while (nResult == 0 && *(pszVerNum[0]) != '\0' && *(pszVerNum[1]) != '\0')
        {
                nVerNum[0] = StrToInt(pszVerNum[0]++);
                nVerNum[1] = StrToInt(pszVerNum[1]++);
                nResult = ((nVerNum[0] < nVerNum[1]) ? 
                                        (-1) :
                                    (nVerNum[0] > nVerNum[1] ? 1 : 0));
        }

        return nResult;
}
