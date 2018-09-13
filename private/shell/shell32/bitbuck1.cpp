#include "shellprv.h"

extern "C" {
#include <shellp.h>
#include "idlcomm.h"
#include "pidl.h"
#include "fstreex.h"
#include "views.h"
#include "ids.h"
#include "shitemid.h"

#include "bitbuck.h"

void SHChangeNotifyDeregisterWindow(HWND hwnd);
} ;

#include "sfviewp.h"
#include "shguidp.h"


class CBitBucketSFVCB : public CBaseShellFolderViewCB
{
public:
    CBitBucketSFVCB(IShellFolder* psf, CBitBucket* pBBFolder)
        : CBaseShellFolderViewCB(psf, NULL, 0), m_pBBFolder(pBBFolder)
    { 
        memset(&m_fssci, 0, sizeof(m_fssci));
    }

    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    CBitBucket* m_pBBFolder;
    FSSELCHANGEINFO m_fssci;

private:
    HRESULT OnMergeMenu(DWORD pv, QCMINFO*lP)
    {
        CDefFolderMenu_MergeMenu(HINST_THISDLL, 0, POPUP_BITBUCKET_POPUPMERGE, lP);
        return S_OK;
    }

    HRESULT OnInitMenuPopup(DWORD pv, UINT wPl, UINT wPh, HMENU lP)
    {
        EnableMenuItem(lP, wPl + FSIDM_PURGEALL, IsRecycleBinEmpty() ? (MF_GRAYED | MF_BYCOMMAND) : MF_BYCOMMAND);
        return S_OK;
    }

    HRESULT OnInvokeCommand(DWORD pv, UINT wP)
    {
        switch(wP)
        {
        case FSIDM_PURGEALL:
            BBPurgeAll(m_pBBFolder, m_hwndMain, 0);
            break;

        case FSIDM_SORTBYNAME:
        case FSIDM_SORTBYORIGIN:
        case FSIDM_SORTBYDELETEDDATE:
        case FSIDM_SORTBYTYPE:
        case FSIDM_SORTBYSIZE:
            BBSort(m_hwndMain, wP);
            break;

        }
        return S_OK;
    }

    HRESULT OnGetHelpText(DWORD pv, UINT wPl, UINT wPh, LPTSTR psz)
    {
        LoadString(HINST_THISDLL, LOWORD(wPl) + IDS_MH_FSIDM_FIRST, psz, wPh);
        return S_OK;
    }

    HRESULT OnGetCCHMax(DWORD pv, LPCITEMIDLIST wP, UINT *lP)
    {
        return S_OK;
    }

    HRESULT OnSelChange(DWORD pv, UINT wPl, UINT wPh, SFVM_SELCHANGE_DATA*lP)
    {
        FSOnSelChange(NULL, lP, &m_fssci);
        return S_OK;
    }

    HRESULT OnFSNotify(DWORD pv, LPCITEMIDLIST *ppidl, LPARAM lP)
    {
        return BBHandleFSNotify(m_hwndMain, (LONG)lP, ppidl[0], ppidl[1]);
    }

    HRESULT OnUpdateStatusBar(DWORD pv, BOOL wP)
    {
        return FSUpdateStatusBar(_punkSite, &m_fssci);
    }

    HRESULT OnWindowCreated(DWORD pv, HWND wP)
    {
        BBInitializeViewWindow(wP);
        m_fssci.idDrive = -1;   // no drive specific stuff
        InitializeStatus(_punkSite);
        return S_OK;
    }

    HRESULT OnInsertDeleteItem(int iMul, LPCITEMIDLIST pidl)
    {
        FSOnInsertDeleteItem(NULL, &m_fssci, pidl, iMul);
        return S_OK;
    }

    HRESULT OnWindowDestroy(DWORD pv, HWND hwnd)
    {
        SHChangeNotifyDeregisterWindow(hwnd);
        return S_OK;
    }

    HRESULT OnSize(DWORD pv, UINT cx, UINT cy)
    {
        ResizeStatus(_punkSite, cx);
        return S_OK;
    }

    HRESULT OnDEFVIEWMODE(DWORD pv, FOLDERVIEWMODE*lP)
    {
        *lP = FVM_DETAILS;
        return S_OK;
    }
};


STDMETHODIMP CBitBucketSFVCB::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_MERGEMENU, OnMergeMenu);
    HANDLE_MSG(0, SFVM_INITMENUPOPUP, OnInitMenuPopup);
    HANDLE_MSG(0, SFVM_INVOKECOMMAND, OnInvokeCommand);
    HANDLE_MSG(0, SFVM_GETHELPTEXT, OnGetHelpText);
    HANDLE_MSG(0, SFVM_GETCCHMAX, OnGetCCHMax);
    HANDLE_MSG(0, SFVM_SELCHANGE, OnSelChange);
    HANDLE_MSG(0, SFVM_FSNOTIFY, OnFSNotify);
    HANDLE_MSG(0, SFVM_UPDATESTATUSBAR, OnUpdateStatusBar);
    HANDLE_MSG(0, SFVM_WINDOWCREATED, OnWindowCreated);
    HANDLE_MSG(1 , SFVM_INSERTITEM, OnInsertDeleteItem);
    HANDLE_MSG(-1, SFVM_DELETEITEM, OnInsertDeleteItem);
    HANDLE_MSG(0, SFVM_WINDOWDESTROY, OnWindowDestroy);
    HANDLE_MSG(0, SFVM_DEFVIEWMODE, OnDEFVIEWMODE);
    HANDLE_MSG(0, SFVM_SIZE, OnSize);

    default:
        return E_FAIL;
    }

    return NOERROR;
}

IShellFolderViewCB* BitBucket_CreateSFVCB(IShellFolder* psf, CBitBucket* pBBFolder)
{
    return new CBitBucketSFVCB(psf, pBBFolder);
}
