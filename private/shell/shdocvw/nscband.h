/**************************************************************\
    FILE: NSCBand.h

    DESCRIPTION:  the class CNscBand exists to support name 
        space control bands.  A name space control uses IShellFolder
        rooted in various namespaces including Favorites, history, 
        Shell Name Space, etc. to depict a hierarchical UI 
        representation of the given name space.  
    
    AUTHOR:  chrisny

\**************************************************************/
#include "../lib/bands.h"
#include "nsc.h"

#ifndef _NSCBAND_H
#define _NSCBAND_H

// for degug trace messages.
#define DM_PERSIST      0           // trace IPS::Load, ::Save, etc.
#define DM_MENU         0           // menu code
#define DM_FOCUS        0           // focus
#define DM_FOCUS2       0           // like DM_FOCUS, but verbose

const short CSIDL_NIL = -32767;

////////////////
///  NSC band

class CNSCBand : public CToolBand
               , public IContextMenu
               , public IBandNavigate
               , public IWinEventHandler
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void) { return CToolBand::AddRef(); };
    STDMETHODIMP_(ULONG) Release(void) { return CToolBand::Release(); };

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);

    // *** IDockingWindow methods ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dw);

    // *** IDeskBand methods ***
    virtual STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                   DESKBANDINFO* pdbi);

    // *** IPersistStream methods ***
    // (others use base class implementation) 
    virtual STDMETHODIMP GetClassID(CLSID *pClassID);
    virtual STDMETHODIMP Load(IStream *pStm);
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);

    // *** IWinEventHandler methods ***
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg
                                    , WPARAM wParam, LPARAM lParam
                                    , LRESULT *plres);
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);

    // *** IContextMenu methods ***
    STDMETHOD(QueryContextMenu)(HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags);

    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR    idCmd,
                                UINT        uType,
                                UINT      * pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax) { return E_NOTIMPL; };

    // *** IOleCommandTarget methods ***
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);

    // *** IBandNavigate methods ***
    virtual STDMETHODIMP Select(LPCITEMIDLIST pidl);
    

    // *** IInputObject methods ***
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);
    
    void SetNscMode(UINT nMode) { _pns->SetNscMode(nMode); };
protected:
    HRESULT _Init(LPCITEMIDLIST pidl);
    virtual ~CNSCBand();
    virtual HRESULT _OnRegisterBand(IOleCommandTarget *poctProxy) { return S_OK; } // meant to be overridden
    
    void _UnregisterBand();
    void _EnsureImageListsLoaded();

    HRESULT _QueryContextMenuSelection(IContextMenu ** ppcm);
    HRESULT _InvokeCommandOnItem(LPCTSTR pszVerb);

#ifndef ENABLE_CCHANNELBAND
    friend HRESULT CNSCBand_CreateInstanceEx(IUnknown *punkOuter, IUnknown **ppunk
                                            , LPCOBJECTINFO poi, LPCITEMIDLIST pidl);
#endif
    friend HRESULT CHistBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk
                                            , LPCOBJECTINFO poi);      

#ifndef ENABLE_CCHANNELBAND
    friend HRESULT CChannelBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk
                                            , LPCOBJECTINFO poi);      
#endif  // ENABLE_CCHANNELBAND
                        
    LPITEMIDLIST        _pidl;
    WCHAR               _szTitle[40];
                        
    INSCTree *          _pns;               // name space control data.
    IWinEventHandler *  _pweh;              // name space control's OnWinEvent handler
    BITBOOL             _fInited :1;        // true if band has been inited.
    BITBOOL             _fVisible :1;       // true if band is showing
    LPCOBJECTINFO       _poi;               // cached object info.
    HACCEL              _haccTree;

    HIMAGELIST          _himlNormal;        // shared image list
    HIMAGELIST          _himlHot;
};

#endif /* _NSCBAND_H */





