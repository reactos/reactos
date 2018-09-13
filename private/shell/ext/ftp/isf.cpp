/*****************************************************************************\
    FILE:   isf.cpp

    DESCRIPTION:
        This is a base class that implements the default behavior of IShellFolder.
\*****************************************************************************/

#include "priv.h"
#include "isf.h"
#include <shlobj.h>



/*****************************************************************************\
    FUNCTION: IShellFolder::ParseDisplayName

    DESCRIPTION:
\*****************************************************************************/
HRESULT CBaseFolder::ParseDisplayName(HWND hwnd, LPBC pbcReserved, LPOLESTR pwszDisplayName,
                        ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes)
{
    if (pdwAttributes)
        *pdwAttributes = 0;

    if (ppidl)
        *ppidl = NULL;

    return E_NOTIMPL;
}

/*****************************************************************************\
    FUNCTION: IShellFolder::EnumObjects

    DESCRIPTION:
\*****************************************************************************/
HRESULT CBaseFolder::EnumObjects(HWND hwndOwner, DWORD grfFlags, IEnumIDList ** ppenumIDList)
{
    if (ppenumIDList)
        *ppenumIDList = NULL;

    return E_NOTIMPL;
}


/*****************************************************************************\
    FUNCTION: IShellFolder::BindToObject

    DESCRIPTION:
\*****************************************************************************/
HRESULT CBaseFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, LPVOID * ppvObj)
{
    if (ppvObj)
        *ppvObj = NULL;

    return E_NOTIMPL;
}


/*****************************************************************************\
     FUNCTION: IShellFolder::BindToStorage
 
    DESCRIPTION:
         This should be implemented so people can use the File.Open and File.SaveAs
    dialogs with this ShellFolder.
\*****************************************************************************/
HRESULT CBaseFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, LPVOID * ppvObj)
{
    if (ppvObj)
        *ppvObj = NULL;

    return E_NOTIMPL;
}


/*****************************************************************************\
     FUNCTION: IShellFolder::CompareIDs
 
    DESCRIPTION:
         This should be implemented so people can use the File.Open and File.SaveAs
    dialogs with this ShellFolder.
\*****************************************************************************/
HRESULT CBaseFolder::CompareIDs(LPARAM ici, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    return E_NOTIMPL;
}


/*****************************************************************************\
     FUNCTION: IShellFolder::CreateViewObject
 
    DESCRIPTION:
         This should be implemented so people can use the File.Open and File.SaveAs
    dialogs with this ShellFolder.
\*****************************************************************************/
HRESULT CBaseFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    *ppvObj = NULL;
    if (IsEqualIID(riid, IID_IShellView))
        hr = _CreateShellView(hwndOwner, ppvObj);
    else if (IsEqualIID(riid, IID_IContextMenu))
        hr = _GetUIObjectOf(hwndOwner, 0, NULL, riid, 0, ppvObj, TRUE);
    else
        hr = E_NOINTERFACE;

    return hr;
}


BOOL IsShellIntegration(void)
{
    BOOL fResult = FALSE;
    HINSTANCE hInst = LoadLibrary(TEXT("shell32.dll"));

    if (EVAL(hInst))
    {
        LPVOID pv = GetProcAddress(hInst, "DllGetVersion");
        if (pv)
            fResult = TRUE;
        FreeLibrary(hInst);
    }
    
    return fResult;
}

HRESULT CBaseFolder::_CreateShellView(HWND hwndOwner, void ** ppvObj, LONG lEvents, FOLDERVIEWMODE fvm, 
                                       IShellFolderViewCB * psfvCallBack, LPCITEMIDLIST pidl, LPFNVIEWCALLBACK pfnCallback)
{
    HRESULT hr;
    IShellFolder * psf;

    hr = this->QueryInterface(IID_IShellFolder, (LPVOID *) &psf);
    if (EVAL(SUCCEEDED(hr)))
    {
        SFV_CREATE sfvCreate =      // SHCreateShellFolderView struct
        {
                sizeof(SFV_CREATE),
                psf,            // psf
                NULL,           // psvOuter
                psfvCallBack    // psfvcb - (IShellFolderViewCB *)
        };

        // SHCreateShellFolderView isn't in the original shell.  We can't rely on the 
        // the Delayload code because it's exported by ordinal and the original
        // shell had a different exports by the same number.
        if (IsShellIntegration())
            hr = _SHCreateShellFolderView(&sfvCreate, (LPSHELLVIEW FAR*)ppvObj);
        else
            hr = E_FAIL;  // Force us to go into the next try.

        // If we aren't running on a machine with Shell Integration, SHCreateShellFolderView will fail.
        if (FAILED(hr))
        {
            CSFV csfv;

            csfv.cbSize = sizeof(csfv);
            csfv.pshf = psf;
            csfv.psvOuter = (IShellView *) psfvCallBack;      // Hack but it works...
            csfv.pidl = pidl;           // This is feed to SFVM_GETNOTIFY so it needs to be a pidlTarget.
            csfv.lEvents = lEvents;
            csfv.pfnCallback = pfnCallback;
            csfv.fvm = fvm;         // vs. FVM_ICON, ...

            hr = SHCreateShellFolderViewEx(&csfv, (LPSHELLVIEW FAR*)ppvObj);
            if (SUCCEEDED(hr))
                psfvCallBack->AddRef();     // We gave them a ref.
        }

        psf->Release();
    }

    return hr;
}


/*****************************************************************************\
     FUNCTION: IShellFolder::GetAttributesOf
 
    DESCRIPTION:
\*****************************************************************************/
HRESULT CBaseFolder::GetAttributesOf(UINT cpidl, LPCITEMIDLIST *apidl, ULONG *rgfInOut)
{
    return E_NOTIMPL;
}


/*****************************************************************************\
     FUNCTION: IShellFolder::GetUIObjectOf
 
    DESCRIPTION:
\*****************************************************************************/
HRESULT CBaseFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST rgpidl[],
                                REFIID riid, UINT * prgfInOut, LPVOID * ppvObj)
{
    return E_NOTIMPL;
}


/*****************************************************************************\
    DESCRIPTION:
\*****************************************************************************/
HRESULT CBaseFolder::_GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST rgpidl[],
                                REFIID riid, UINT * prgfInOut, LPVOID * ppvOut, BOOL fFromCreateViewObject)
{
    return GetUIObjectOf(hwndOwner, cidl, rgpidl, riid, prgfInOut, ppvOut);
}


/*****************************************************************************\
     FUNCTION: IShellFolder::GetDisplayNameOf
 
    DESCRIPTION:
\*****************************************************************************/
HRESULT CBaseFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD shgno, LPSTRRET pStrRet)
{
    return E_NOTIMPL;
}


/*****************************************************************************\
     FUNCTION: IShellFolder::SetNameOf
 
    DESCRIPTION:
\*****************************************************************************/
HRESULT CBaseFolder::SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl, LPCOLESTR pwszName,
                                DWORD dwReserved, LPITEMIDLIST *ppidlOut)
{
    return E_NOTIMPL;
}


//===========================
// *** IShellFolder2 Interface ***
//===========================

//===========================
// *** IPersist Interface ***
//===========================

/*****************************************************************************\
     FUNCTION: IPersist::GetClassID
 
    DESCRIPTION:
\*****************************************************************************/
HRESULT CBaseFolder::GetClassID(LPCLSID pClassID)
{
    HRESULT hr = E_INVALIDARG;

    if (EVAL(pClassID))
    {
        if (EVAL(m_pClassID))
        {
            *pClassID = *m_pClassID;
            hr = S_OK;
        }
        else
            hr = E_FAIL;
    }

    return hr;
}

//===========================
// *** IPersistFolder Interface ***
//===========================

/*****************************************************************************\
    DESCRIPTION:
\*****************************************************************************/
HRESULT CBaseFolder::Initialize(LPCITEMIDLIST pidl)
{
    ASSERT(!m_pidl);   // Don't reroot us.
    return _Initialize(pidl, NULL, ILGetSize(pidl) - sizeof(pidl->mkid.cb));
}

//===========================
// *** IPersistFolder2 Interface ***
//===========================

/*****************************************************************************\ 
    DESCRIPTION:
\*****************************************************************************/
HRESULT CBaseFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    HRESULT hr = E_INVALIDARG;

    if (EVAL(ppidl))
    {
        hr = E_FAIL;

        if (m_pidlRoot)
        {
            *ppidl = ILClone(m_pidlRoot);
        }
        else if (EVAL(m_pidl))
        {
            *ppidl = GetPublicTargetPidlClone();
        }

        if (*ppidl)
            hr = S_OK;
    }

    return hr;
}

//===========================
// *** IPersistFolder3 Interface ***
//===========================
HRESULT GetPidlFromPersistFolderTargetInfo(const PERSIST_FOLDER_TARGET_INFO *ppfti, LPITEMIDLIST * ppidl, BOOL fFree)
{
    HRESULT hr = E_INVALIDARG;

    if (ppidl)
    {
        *ppidl = NULL;
        if (ppfti->pidlTargetFolder)
        {
            *ppidl = (fFree ? ppfti->pidlTargetFolder : ILClone(ppfti->pidlTargetFolder));
            if (*ppidl)
                hr = S_OK;
            else
                hr = E_OUTOFMEMORY;
        }
        else
        {
            if (ppfti->szTargetParsingName[0])
            {
                hr = IEParseDisplayNameWithBCW(CP_ACP, ppfti->szTargetParsingName, NULL, ppidl);
            }

            if (!*ppidl && (-1 != ppfti->csidl))
            {
                hr = SHGetSpecialFolderLocation(NULL, ppfti->csidl, ppidl);
            }
        }
    }
    
    return hr;
}


/*****************************************************************************\
    DESCRIPTION:
\*****************************************************************************/
HRESULT CBaseFolder::InitializeEx(IBindCtx *pbc, LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *ppfti)
{
    HRESULT hr = E_INVALIDARG;

    if (EVAL(pidlRoot))
    {
        if (ppfti)
        {
            // We are a Folder Shortcut.
            LPITEMIDLIST pidlTarget;

            hr = GetPidlFromPersistFolderTargetInfo(ppfti, &pidlTarget, FALSE);  // Get the real root.           
            TraceMsg(TF_FOLDER_SHRTCUTS, "CBaseFolder::InitializeEx() this=%#08lx, pidlTarget=%#08lx, pidlRoot=%#08lx", this, pidlTarget, pidlRoot);

            AssertMsg((NULL != pidlTarget), TEXT("CBaseFolder::InitializeEx() We are useless without a pidlTarget so watch me go limp."));
            if (pidlTarget)
            {
                hr = _Initialize(pidlTarget, pidlRoot, m_nIDOffsetToPrivate);
                ILFree(pidlTarget);
            }
        }
        else
        {
            // We aren't a folder shortcut.
            hr = Initialize(pidlRoot);
        }
    }

    return hr;
}


HRESULT CBaseFolder::GetFolderTargetInfo(PERSIST_FOLDER_TARGET_INFO *ppfti)
{
    HRESULT hr = E_INVALIDARG;

    AssertMsg((NULL != ppfti), TEXT("CBaseFolder::GetFolderTargetInfo() Caller passed an invalid param."));
    if (ppfti)
    {
        ZeroMemory(ppfti, sizeof(*ppfti)); 

        ppfti->pidlTargetFolder = ILClone(m_pidlRoot);
        ppfti->dwAttributes = -1;
        ppfti->csidl = -1;
        hr = S_OK;
    }

    return hr;
}


LPCITEMIDLIST CBaseFolder::GetPrivatePidlReference(void)
{
    return _ILSkip(m_pidl, m_nIDOffsetToPrivate);
}

// This function always needs the InternetExplorer pidl.
LPITEMIDLIST CBaseFolder::GetPublicPidlRootIDClone(void)
{
    LPITEMIDLIST pidlFull = ILClone(m_pidl);
    LPITEMIDLIST pidlPrivStart = _ILSkip(pidlFull, m_nIDOffsetToPrivate);

    // Strip all Private ItemIDs
    while (!ILIsEmpty(pidlPrivStart))
        ILRemoveLastID(pidlPrivStart);

    return pidlFull;
}

LPITEMIDLIST CBaseFolder::CreateFullPrivatePidl(LPCITEMIDLIST pidlPrivateSubPidl)
{
    return ILCombine(GetPrivatePidlReference(), pidlPrivateSubPidl);
}

LPITEMIDLIST CBaseFolder::CreateFullPublicPidlFromRelative(LPCITEMIDLIST pidlPrivateSubPidl)
{
    return ILCombine(GetPublicRootPidlReference(), pidlPrivateSubPidl);
}

LPITEMIDLIST CBaseFolder::CreateFullPublicPidl(LPCITEMIDLIST pidlPrivatePidl)
{
    LPITEMIDLIST pidlRoot = GetPublicPidlRootIDClone();
    LPITEMIDLIST pidlResult = NULL;
    
    if (pidlRoot)
    {
        pidlResult = ILCombine(pidlRoot, pidlPrivatePidl);
        ILFree(pidlRoot);
    }

    return pidlResult;
}

HRESULT CBaseFolder::_Initialize(LPCITEMIDLIST pidlTarget, LPCITEMIDLIST pidlRoot, int nBytesToPrivate)
{
    HRESULT hr = E_INVALIDARG;

    if (pidlTarget)
    {
        ILFree(m_pidl);
        ILFree(m_pidlRoot);    
        m_pidl = ILClone(pidlTarget);
        m_pidlRoot = ILClone(pidlRoot); // This is the Folder Shortcut pidl.  We don't use it outselves.

        if (m_pidl)
        {
            m_nIDOffsetToPrivate = nBytesToPrivate;
            hr = S_OK;
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}


/****************************************************\
    Constructor
\****************************************************/
CBaseFolder::CBaseFolder(LPCLSID pClassID) : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_pidl);
    ASSERT(!m_nIDOffsetToPrivate);
    ASSERT(!m_pClassID);

    m_pClassID = pClassID;
    ASSERT(pClassID);
}


/****************************************************\
    Destructor
\****************************************************/
CBaseFolder::~CBaseFolder()
{
    Pidl_Set(&m_pidlRoot, NULL);    // Folder Shortcut pidl
    Pidl_Set(&m_pidl, NULL);
    DllRelease();
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CBaseFolder::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CBaseFolder::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CBaseFolder::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CBaseFolder, IShellFolder, IShellFolder2),
        QITABENTMULTI(CBaseFolder, IPersist, IPersistFolder),
        QITABENTMULTI(CBaseFolder, IPersist, IPersistFolder3),
        QITABENTMULTI(CBaseFolder, IPersistFolder, IPersistFolder3),
        QITABENTMULTI(CBaseFolder, IPersistFolder2, IPersistFolder3),
        QITABENT(CBaseFolder, IShellFolder2),
        QITABENT(CBaseFolder, IPersistFolder3),
        QITABENT(CBaseFolder, IObjectWithSite),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}
