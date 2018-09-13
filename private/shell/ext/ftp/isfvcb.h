/*****************************************************************************\
    FILE: isfvcb.h

    DESCRIPTION:
        This is a base class that implements the default behavior of 
    IShellFolderViewCallBack.  This allows default DefView implementation with this
    callback to override specific behavior.
\*****************************************************************************/


#ifndef _CBASEFOLDERVIEWCB_H
#define _CBASEFOLDERVIEWCB_H



class CBaseFolderViewCB
                : public IShellFolderViewCB
                , public CObjectWithSite
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IShellFolderViewCB methods ***
    virtual STDMETHODIMP MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    // Friend Functions
    static HRESULT _IShellFolderViewCallBack(IShellView * psvOuter, IShellFolder * psf, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    // Private Member Variables
    int                     m_cRef;

    IShellFolderView *      m_psfv;                 // Our parent's IShellFolderView. (Same as _punkSite)
    DWORD                   m_dwSignature;

    enum { c_dwSignature = 0x43564642 }; // "BFVC" - BaseFolderViewCb

    // Private Member Functions
    CBaseFolderViewCB();
    virtual ~CBaseFolderViewCB();

    // We have implementations for these.
    virtual HRESULT _OnSetISFV(IShellFolderView * psfv);

    // The caller needs to provide implementations for these
    // or they will get default behavior.
    virtual HRESULT _OnWindowCreated(void) {return E_NOTIMPL;};
    virtual HRESULT _OnDefItemCount(LPINT pi) {return E_NOTIMPL;};
    virtual HRESULT _OnGetHelpText(LPARAM lParam, WPARAM wParam) {return E_NOTIMPL;};
    virtual HRESULT _OnGetHelpTopic(SFVM_HELPTOPIC_DATA * phtd) {return E_NOTIMPL;};
    virtual HRESULT _OnGetZone(DWORD * pdwZone, WPARAM wParam) {return E_NOTIMPL;};
    virtual HRESULT _OnGetPane(DWORD dwPaneID, DWORD * pdwPane) {return E_NOTIMPL;};
    virtual HRESULT _OnRefresh(BOOL fReload) {return E_NOTIMPL;};
    virtual HRESULT _OnDidDragDrop(DROPEFFECT de, IDataObject * pdto) {return E_NOTIMPL;};
    virtual HRESULT _OnGetDetailsOf(UINT ici, PDETAILSINFO pdi) {return E_NOTIMPL;};
    virtual HRESULT _OnInvokeCommand(UINT idc) {return E_NOTIMPL;};
    virtual HRESULT _OnMergeMenu(LPQCMINFO pqcm) {return E_NOTIMPL;};
    virtual HRESULT _OnUnMergeMenu(HMENU hMenu) {return E_NOTIMPL;};
    virtual HRESULT _OnColumnClick(UINT ici) {return E_NOTIMPL;};
    virtual HRESULT _OnGetNotify(LPITEMIDLIST * ppidl, LONG * lEvents) {return E_NOTIMPL;};
    virtual HRESULT _OnFSNotify(LPITEMIDLIST * ppidl, LONG * lEvents) {return E_NOTIMPL;};
    virtual HRESULT _OnQueryFSNotify(SHChangeNotifyEntry * pshcne) {return E_NOTIMPL;};
    virtual HRESULT _OnSize(LONG x, LONG y) {return E_NOTIMPL;};
    virtual HRESULT _OnUpdateStatusBar(void) {return E_NOTIMPL;};
    virtual HRESULT _OnThisIDList(LPITEMIDLIST * ppidl) {return E_NOTIMPL;};
    virtual HRESULT _OnAddPropertyPages(SFVM_PROPPAGE_DATA * pData) {return E_NOTIMPL;};
    virtual HRESULT _OnInitMenuPopup(HMENU hmenu, UINT idCmdFirst, UINT nIndex) {return E_NOTIMPL;};
    virtual HRESULT _OnBackGroundEnumDone(void) {return E_NOTIMPL;};
};

#endif // _CBASEFOLDERVIEWCB_H
