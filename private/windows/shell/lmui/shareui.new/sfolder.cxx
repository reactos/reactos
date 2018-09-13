//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       sfolder.cxx
//
//  Contents:   Implementation of IShellFolder
//
//  History:    13-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "dutil.hxx"
#include "enum.hxx"
#include "menuutil.hxx"
#include "menu.hxx"
#include "menubg.hxx"
#include "sdetails.hxx"
#include "sfolder.hxx"
#include "shares.h"
#include "shares.hxx"
#include "util.hxx"
#include "xicon.hxx"
#include "shri.hxx"
#include "resource.h"

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CSharesSF::ParseDisplayName(
    HWND hwndOwner,
    LPBC pbc,
    LPOLESTR lpszDisplayName,
    ULONG* pchEaten,
    LPITEMIDLIST* ppidlOutm,
    ULONG* pdwAttributes
    )
{
    return E_NOTIMPL;
}


STDMETHODIMP
CSharesSF::GetAttributesOf(
    UINT cidl,
    LPCITEMIDLIST* apidl,
    ULONG* pdwInOut
    )
{
    ULONG fMask = 0;

    if (cidl == 0)
    {
        // What things in general can be done in the folder? Return a
        // mask of everything possible.
        fMask = SFGAO_CANDELETE | SFGAO_HASPROPSHEET | SFGAO_CANRENAME;
    }
    else if (cidl == 1)
    {
        LPIDSHARE pids = (LPIDSHARE)apidl[0];
        fMask = SFGAO_CANDELETE | SFGAO_HASPROPSHEET;
        if (!(Share_GetType(pids) & STYPE_SPECIAL))
        {
            fMask |= SFGAO_CANRENAME;
        }
    }
    else if (cidl > 1)
    {
        fMask = SFGAO_CANDELETE;
    }

    *pdwInOut &= fMask;
    return S_OK;
}


STDMETHODIMP
CSharesSF::GetUIObjectOf(
    HWND hwndOwner,
    UINT cidl,
    LPCITEMIDLIST* apidl,
    REFIID riid,
    UINT* prgfInOut,
    LPVOID* ppvOut
    )
{
    CShares* This = IMPL(CShares,m_ShellFolder,this);
    HRESULT hr = E_NOINTERFACE;

    *ppvOut = NULL;

    if (cidl == 1 && IsEqualIID(riid, IID_IExtractIcon))
    {
        LPIDSHARE pids = (LPIDSHARE)apidl[0];

        CSharesEI* pObj = new CSharesEI(Share_GetFlags(pids), Share_GetService(pids));
        if (NULL == pObj)
        {
            return E_OUTOFMEMORY;
        }

        hr = pObj->QueryInterface(riid, ppvOut);
        pObj->Release();
    }
#ifdef UNICODE
    else if (cidl == 1 && IsEqualIID(riid, IID_IExtractIconA))
    {
        LPIDSHARE pids = (LPIDSHARE)apidl[0];

        CSharesEIA* pObj = new CSharesEIA(Share_GetFlags(pids), Share_GetService(pids));
        if (NULL == pObj)
        {
            return E_OUTOFMEMORY;
        }

        hr = pObj->QueryInterface(riid, ppvOut);
        pObj->Release();
    }
#endif // UNICODE
    else if (cidl > 0 && IsEqualIID(riid, IID_IContextMenu))
    {
        // Create a context menu for selected items.

        if (This->m_level < 2)
        {
            // user has insufficient privilege to perform any operations.
            return E_NOINTERFACE;
        }

        LPIDSHARE pids = (LPIDSHARE)apidl[0];
        CSharesCM* pObj = new CSharesCM(hwndOwner);
        if (NULL == pObj)
        {
            return E_OUTOFMEMORY;
        }

        hr = pObj->InitInstance(This->m_pszMachine, cidl, apidl, this);
        if (FAILED(hr))
        {
			delete pObj;
            return hr;
        }

        hr = pObj->QueryInterface(riid, ppvOut);
        pObj->Release();
    }
    else if (cidl > 0 && IsEqualIID(riid, IID_IDataObject))
    {
        hr = CIDLData_CreateFromIDArray(
                        This->m_pidl,
                        cidl,
                        apidl,
                        (LPDATAOBJECT *)ppvOut);
    }

    return hr;
}


STDMETHODIMP
CSharesSF::EnumObjects(
    HWND hwndOwner,
    DWORD grfFlags,
    LPENUMIDLIST* ppenumUnknown
    )
{
    CShares* This = IMPL(CShares,m_ShellFolder,this);
    HRESULT hr = E_FAIL;

    *ppenumUnknown = NULL;

    if (!(grfFlags & SHCONTF_NONFOLDERS))
    {
        return hr;
    }

    appAssert(0 != This->m_level);
    CSharesEnum* pEnum = new CSharesEnum(This->m_pszMachine, This->m_level);
    if (NULL == pEnum)
    {
        return E_OUTOFMEMORY;
    }

    hr = pEnum->Init(grfFlags);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = pEnum->QueryInterface(IID_IEnumIDList, (LPVOID*)ppenumUnknown);
    pEnum->Release();
    return hr;
}


STDMETHODIMP
CSharesSF::BindToObject(
    LPCITEMIDLIST pidl,
    LPBC pbc,
    REFIID riid,
    LPVOID* ppvOut
    )
{
    //
    // Shares folder doesn't contain sub-folders
    //

    *ppvOut = NULL;
    return E_FAIL;
}


// not used in Win95
STDMETHODIMP
CSharesSF::BindToStorage(
    LPCITEMIDLIST pidl,
    LPBC pbcReserved,
    REFIID riid,
    LPVOID* ppvOut
    )
{
    *ppvOut = NULL;
    return E_NOTIMPL;
}

#define PlusMinus(x) (((x) < 0) ? -1 : ( ((x) > 0) ? 1 : 0 ))

////////////////////////////////////////////////////////////////////////////
////////////////////// BUGBUG: use same strings/numbers as IShellDetails!!!!
////////////////////////////////////////////////////////////////////////////

int
CSharesSF::_CompareOne(
    DWORD iCol,
    LPIDSHARE pids1,
    LPIDSHARE pids2
    )
{
    switch (iCol)
    {
    case ICOL2_NAME:
        return lstrcmpi(Share_GetName(pids1), Share_GetName(pids2));

    case ICOL2_COMMENT:
        return lstrcmpi(Share_GetComment(pids1), Share_GetComment(pids2));

    case ICOL2_PATH:
        return lstrcmpi(Share_GetPath(pids1), Share_GetPath(pids2));

    case ICOL2_MAXUSES:
    {
        DWORD max1 = Share_GetMaxUses(pids1);
        DWORD max2 = Share_GetMaxUses(pids2);
        if (max1 == SHI_USES_UNLIMITED && max2 == SHI_USES_UNLIMITED)
        {
            return 0;
        }
        else if (max1 == SHI_USES_UNLIMITED)
        {
            return 1;
        }
        else if (max2 == SHI_USES_UNLIMITED)
        {
            return -1;
        }
        else
        {
            return max1 - max2;
        }
    }

    case ICOL2_SERVICE:
        return lstrcmpi(Share_GetPath(pids1), Share_GetPath(pids2));

    default: appAssert(!"Illegal column"); return 0;
    }
}

STDMETHODIMP
CSharesSF::CompareIDs(
    LPARAM iCol,
    LPCITEMIDLIST pidl1,
    LPCITEMIDLIST pidl2
    )
{
    CShares* This = IMPL(CShares,m_ShellFolder,this);

    // If one item is a special item, then put it ahead of the other one.
    // If they are both special items, sort on name.

    LPIDSHARE pids1 = (LPIDSHARE)pidl1;
    LPIDSHARE pids2 = (LPIDSHARE)pidl2;
    int iCmp;

    // Documentation says iCol is always zero, but that is wrong! It will
    // be non-zero in case the user has clicked on a column heading to sort
    // the column. In general, we want the entire item to be equal before we
    // return 0 for equality. To do this, we first check the chosen element.
    // If it is not equal, return the value. Otherwise, check all elements in
    // this standard order:
    //      name
    //      comment
    //      path
    //      max uses
    //      current uses
    // Only after all these checks return 0 (equality) do we return 0 (equality)

    iCmp = _CompareOne(iCol, pids1, pids2);
    if (iCmp != 0)
    {
        return ResultFromShort(PlusMinus(iCmp));
    }

    // now, check each in turn

    iCmp = _CompareOne(ICOL2_NAME, pids1, pids2);
    if (iCmp != 0)
    {
        return ResultFromShort(PlusMinus(iCmp));
    }

    iCmp = _CompareOne(ICOL2_COMMENT, pids1, pids2);
    if (iCmp != 0)
    {
        return ResultFromShort(PlusMinus(iCmp));
    }

    if (This->m_level == 2)
    {
        iCmp = _CompareOne(ICOL2_PATH, pids1, pids2);
        if (iCmp != 0)
        {
            return ResultFromShort(PlusMinus(iCmp));
        }

        iCmp = _CompareOne(ICOL2_MAXUSES, pids1, pids2);
        if (iCmp != 0)
        {
            return ResultFromShort(PlusMinus(iCmp));
        }
    }

    return 0;   // the same!
}


STDMETHODIMP
CSharesSF::CreateViewObject(
    HWND hwnd,
    REFIID riid,
    LPVOID* ppvOut
    )
{
    CShares* This = IMPL(CShares,m_ShellFolder,this);
    HRESULT hr = E_NOINTERFACE;

    *ppvOut = NULL;

    if (IsEqualIID(riid, IID_IShellView))
    {
        CSFV csfv =
        {
            sizeof(CSFV),         // cbSize
            (IShellFolder*)this,  // pshf
            NULL,                 // psvOuter
            NULL,                 // pidl to monitor (NULL == all)
            SHCNE_NETSHARE | SHCNE_NETUNSHARE | SHCNE_UPDATEITEM, // events
            _SFVCallBack,         // pfnCallback
            FVM_DETAILS
        };

        hr = SHCreateShellFolderViewEx(&csfv, (LPSHELLVIEW *)ppvOut);
    }
    else if (IsEqualIID(riid, IID_IShellDetails))
    {
        appAssert(This->m_level != 0);
        CSharesSD* pObj = new CSharesSD(hwnd, This->m_level);
        if (NULL == pObj)
        {
            return E_OUTOFMEMORY;
        }

        hr = pObj->QueryInterface(riid, ppvOut);
        pObj->Release();
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        // Create a context menu for the background
        CSharesCMBG* pObj = new CSharesCMBG(hwnd, This->m_pszMachine, This->m_level);
        if (NULL == pObj)
        {
            return E_OUTOFMEMORY;
        }

        hr = pObj->QueryInterface(riid, ppvOut);
        pObj->Release();
    }

    return hr;
}


STDMETHODIMP
CSharesSF::GetDisplayNameOf(
    LPCITEMIDLIST pidl,
    DWORD uFlags,
    LPSTRRET lpName
    )
{
    CShares* This = IMPL(CShares,m_ShellFolder,this);

    LPIDSHARE pids = (LPIDSHARE)pidl;
    if (uFlags == SHGDN_FORPARSING)
    {
        return E_NOTIMPL;   // don't support parsing.
    }
    else if (uFlags == SHGDN_INFOLDER)
    {
        return STRRETCopy(Share_GetName(pids), lpName);
    }
    else if (uFlags == SHGDN_NORMAL)
    {
        if (NULL == This->m_pszMachine)
        {
            return STRRETCopy(Share_GetName(pids), lpName);
        }
        else
        {
            LPWSTR pszMachine = This->m_pszMachine;
            if (pszMachine[0] == TEXT('\\') && pszMachine[1] == TEXT('\\'))
            {
                pszMachine += 2;
            }

            WCHAR szBuf[MAX_PATH];
            szBuf[0] = L'\0';
            MyFormatMessage(
                    MSG_TEMPLATE_WITH_ON,
                    szBuf,
                    ARRAYLEN(szBuf),
                    pszMachine,
                    Share_GetName(pids));
#ifdef UNICODE
            LPTSTR pszCopy = (LPTSTR)SHAlloc((lstrlen(szBuf)+1) * sizeof(TCHAR));
            if (pszCopy)
            {
                wcscpy(pszCopy, szBuf);
                lpName->uType = STRRET_OLESTR;
                lpName->pOleStr = pszCopy;
            }
            else
            {
                lpName->uType = STRRET_CSTR;
                lpName->cStr[0] = '\0';
            }
#else
            lpName->uType = STRRET_CSTR;
            lstrcpyn(lpName->cStr, szBuf, ARRAYSIZE(lpName->cStr));
            SHFree(pszRet);
#endif

            return S_OK;
        }
    }
    else
    {
        return E_INVALIDARG;
    }
}


// BUGBUG: only for SMB!

STDMETHODIMP
CSharesSF::SetNameOf(
    HWND hwndOwner,
    LPCITEMIDLIST pidl,
    LPCOLESTR lpszName,
    DWORD uFlags,
    LPITEMIDLIST* ppidlOut
    )
{
    CShares* This = IMPL(CShares,m_ShellFolder,this);

    if (uFlags != SHGDN_INFOLDER)
    {
        return E_NOTIMPL;
    }

    if (NULL == lpszName || L'\0' == *lpszName)
    {
        // can't change name to nothing
        MessageBeep(0);
        return E_FAIL;
    }

    NET_API_STATUS status;
    WCHAR szBuf[MAX_PATH];
    LPSHARE_INFO_502 pInfo;
    LPIDSHARE pids = (LPIDSHARE)pidl;

    // Get information about the existing share before deleting it.
    status = NetShareGetInfo(This->m_pszMachine, Share_GetName(pids), 502, (LPBYTE*)&pInfo);
    if (status != NERR_Success)
    {
        DisplayError(hwndOwner, IERR_CANT_DEL_SHARE, status, Share_GetName(pids));
        return HRESULT_FROM_WIN32(status);
    }

    // Validate the new share name

    // Trying to create a reserved share?
    if (   (0 == _wcsicmp(g_szIpcShare,   lpszName))
        || (0 == _wcsicmp(g_szAdminShare, lpszName)))
    {
        MyErrorDialog(hwndOwner, MSG_ADDSPECIAL2);
        NetApiBufferFree(pInfo);
        return E_FAIL;
    }

    HRESULT hrTemp;
    if (!IsValidShareName(lpszName, &hrTemp))
    {
        MyErrorDialog(hwndOwner, hrTemp);
        NetApiBufferFree(pInfo);
        return E_FAIL;
    }

    // Check to see that the same share isn't already used, for either the
    // same path or for another path.

    SHARE_INFO_2* pInfo2;
    status = NetShareGetInfo(This->m_pszMachine, (LPWSTR)lpszName, 2, (LPBYTE*)&pInfo2);
    if (status == NERR_Success)
    {
        // Is it already shared for the same path?
        if (0 == _wcsicmp(pInfo2->shi2_path, pInfo->shi502_path))
        {
            MyErrorDialog(hwndOwner, IERR_AlreadyExists, lpszName);
            NetApiBufferFree(pInfo);
            NetApiBufferFree(pInfo2);
            return TRUE;
        }

        // Shared for a different path. Ask the user if they wish to delete
        // the old share and create the new one using the name.

        DWORD id = ConfirmReplaceShare(hwndOwner, lpszName, pInfo2->shi2_path, pInfo->shi502_path);
        if (id == IDNO || id == IDCANCEL)   // BUGBUG: should be only yes/no
        {
            NetApiBufferFree(pInfo);
            NetApiBufferFree(pInfo2);
            return E_FAIL;
        }

        // User said to replace the old share. Do it.
        status = NetShareDel(This->m_pszMachine, (LPWSTR)lpszName, 0);
        if (status != NERR_Success)
        {
            DisplayError(hwndOwner, IERR_CANT_DEL_SHARE, status, (LPWSTR)lpszName);
            NetApiBufferFree(pInfo);
            NetApiBufferFree(pInfo2);
            return FALSE;
        }
        else
        {
            SHChangeNotify(SHCNE_NETUNSHARE, SHCNF_PATH, pInfo2->shi2_path, NULL);
        }

        NetApiBufferFree(pInfo2);
    }

    // Check for downlevel accessibility
    ULONG nType;
    if (NERR_Success != NetpPathType(NULL, (LPWSTR)lpszName, &nType, INPT_FLAGS_OLDPATHS))
    {
        DWORD id = MyConfirmationDialog(
                        hwndOwner,
                        IERR_InaccessibleByDos,
                        MB_YESNO | MB_ICONEXCLAMATION,
                        lpszName);
        if (id == IDNO)
        {
            return TRUE;
        }
    }

    // delete the existing share
    status = NetShareDel(This->m_pszMachine, Share_GetName(pids), 0);
    if (status != NERR_Success)
    {
        NetApiBufferFree(pInfo);
        DisplayError(hwndOwner, IERR_CANT_DEL_SHARE, status, Share_GetName(pids));
        return HRESULT_FROM_WIN32(status);
    }

    // Create a new share with the new name.
    LPWSTR ptmp = pInfo->shi502_netname;
    pInfo->shi502_netname = (LPWSTR)lpszName;   // cast away const
    status = NetShareAdd(This->m_pszMachine, 502, (LPBYTE)pInfo, NULL);
    if (status != NERR_Success)
    {
        pInfo->shi502_netname = ptmp;
        NetApiBufferFree(pInfo);
        DisplayError(hwndOwner, IERR_CANT_ADD_SHARE, status, (LPWSTR)lpszName); // cast away const
        return HRESULT_FROM_WIN32(status);
    }

    // Ok, now I've renamed it. So, fill an ID list with the new guy, and
    // return it in *ppidlOut.

    HRESULT hr = S_OK;
    if (NULL != ppidlOut)
    {
        IDSHARE ids;
        CShare* pShare = new CShare();
        if (NULL == pShare)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pShare->AddSmb((LPSHARE_INFO_2)pInfo); // ignore security at end of level 502
            pShare->FillID(&ids);
            delete pShare;

            *ppidlOut = ILClone((LPCITEMIDLIST)(&ids));
            if (NULL == *ppidlOut)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    // force a view refresh
    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_IDLIST, NULL, 0);

    pInfo->shi502_netname = ptmp;
    NetApiBufferFree(pInfo);
    return hr;
}


//
// Callback from SHCreateShellFolderViewEx
//

HRESULT CALLBACK
CSharesSF::_SFVCallBack(
    LPSHELLVIEW psvOuter,
    LPSHELLFOLDER psf,
    HWND hwndOwner,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CShares* This = IMPL(CShares,m_ShellFolder,psf);
    HRESULT hr = S_OK;     // assume no error

    switch (uMsg)
    {
    case DVM_UPDATESTATUSBAR:
    {
        IShellBrowser* psb = FileCabinet_GetIShellBrowser(hwndOwner);
        UINT cidl = ShellFolderView_GetSelectedCount(hwndOwner);
        if (cidl == 1)
        {
            LPITEMIDLIST *apidl;
            LPIDSHARE pids;
            LPTSTR lpsz = TEXT("");

            ShellFolderView_GetSelectedObjects(hwndOwner, &apidl);
            if (apidl)
            {
                pids = (LPIDSHARE)apidl[0];
                BYTE bService = Share_GetService(pids);
                DWORD dwServiceCount = 0;
                if (bService & SHARE_SERVICE_SMB)  ++dwServiceCount;
                if (bService & SHARE_SERVICE_SFM)  ++dwServiceCount;
                if (bService & SHARE_SERVICE_FPNW) ++dwServiceCount;
                appAssert(dwServiceCount > 0);
                appAssert(dwServiceCount == 1);	// until we merge shares

        		// SMB is only service with a comment field
                if (bService & SHARE_SERVICE_SMB)
                {
                    lpsz = Share_GetComment(pids);
                }
                FSSetStatusText(hwndOwner, &lpsz, 0, 0);
                LocalFree(apidl);
            }
        }
        else
        {
            hr = E_FAIL;
        }
        break;
    }

    case DVM_MERGEMENU:
    {
        appDebugOut((DEB_TRACE, "DVM_MERGEMENU\n"));
        appAssert(This->m_pMenuBg == NULL);
        hr = psf->CreateViewObject(hwndOwner, IID_IContextMenu, (LPVOID*)&This->m_pMenuBg);
        if (SUCCEEDED(hr))
        {
            LPQCMINFO pqcm = (LPQCMINFO)lParam;
            hr = This->m_pMenuBg->QueryContextMenu(
                                    pqcm->hmenu,
                                    pqcm->indexMenu,
                                    pqcm->idCmdFirst,
                                    pqcm->idCmdLast,
                                    CMF_DVFILE);
        }
        break;
    }

    case DVM_UNMERGEMENU:
    {
        appDebugOut((DEB_TRACE, "DVM_UNMERGEMENU\n"));
        if (NULL != This->m_pMenuBg)
        {
            This->m_pMenuBg->Release();
            This->m_pMenuBg = NULL;
        }
        break;
    }

    case DVM_INVOKECOMMAND:
    {
        appDebugOut((DEB_TRACE, "DVM_INVOKECOMMAND\n"));
        appAssert(This->m_pMenuBg != NULL);
        CMINVOKECOMMANDINFO ici =
        {
            sizeof(ici),
            0,  // mask
            hwndOwner,
            (LPCSTR)wParam,
            NULL,
            NULL,
            0,
            0,
            NULL
        };
        hr = This->m_pMenuBg->InvokeCommand(&ici);
        break;
    }

    case DVM_GETHELPTEXT:
    {
        appDebugOut((DEB_TRACE, "DVM_GETHELPTEXT\n"));
        hr = This->m_pMenuBg->GetCommandString(LOWORD(wParam), GCS_HELPTEXT, NULL, (LPSTR)lParam, HIWORD(wParam));
        break;
    }

    case DVM_DEFITEMCOUNT:
        //
        // If DefView times out enumerating items, let it know we probably only
        // have about 20 items
        //

        *(int *)lParam = 20;
        break;

    case DVM_FSNOTIFY:
    {
        LPCITEMIDLIST* ppidl = (LPCITEMIDLIST*)wParam;
        LONG lEvent = lParam;

        switch (lEvent)
        {
        case SHCNE_NETSHARE:
        case SHCNE_NETUNSHARE:
            // a share was added, removed, or changed. Force a view refresh.
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_IDLIST, NULL, 0);
            return S_OK;
        }
        break;
    }

    default:
        hr = E_FAIL;
    }
    return hr;
}
