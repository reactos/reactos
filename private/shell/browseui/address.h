/**************************************************************\
    FILE: address.h

    DESCRIPTION:
        The Class CAddressBand exists to support a "Address" Band
    ToolBar.  This will be used in the Browser's toolbar or can
    be used in the Start Menu.
\**************************************************************/

#ifndef _ADDRESS_H
#define _ADDRESS_H

#include "bands.h"
#include "bandprxy.h"


///////////////////////////////////////////////////////////////////
// #DEFINEs

///////////////////////////////////////////////////////////////////
// Data Structures

///////////////////////////////////////////////////////////////////
// Prototypes

/**************************************************************\
    CLASS: CAddressBand

    DESCRIPTION:
        This Class CAddressBand exists to support a "Address" Band
    ToolBar.  This will be used in the Browser's toolbar or can
    be used in the Start Menu/Taskbar.  If the Band is part of
    a Browser toolbar, any modifications made to the AddressBar
    will go the the browser window.  

        By default, the this AddressBand will not point to a
    browser window when it is on the Taskbar or Start Menu.  
    Anything "executed" in the AddressBar will create a new
    browser window.  Future support may allow for the AddressBand
    in the taskbar/start menu to reference a currently existing
    browser window.
\**************************************************************/
class CAddressBand 
                : public CToolBand
                , public IWinEventHandler
                , public IAddressBand
                , public IInputObjectSite
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return CToolBand::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release(void){ return CToolBand::Release(); };
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IOleCommandTarget methods ***
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
                        OLECMD rgCmds[], OLECMDTEXT *pcmdtext);        // Interface forwarding
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt,
                        VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);        // Interface forwarding

    // *** IDockingWindow methods ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dw);

    // *** IObjectWithSite methods ***
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);

    // *** IInputObject methods ***
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);
    virtual STDMETHODIMP HasFocusIO(void);
    
    // *** IInputObjectSite methods ***
    virtual STDMETHODIMP OnFocusChangeIS(IUnknown *punk, BOOL fSetFocus);

    // *** IShellToolband methods ***
    STDMETHOD(GetBandInfo)    (THIS_ DWORD dwBandID, DWORD fViewMode, 
                                DESKBANDINFO* pdbi) ;
    
    // *** IWinEventHandler methods ***
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plre);
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);

    // *** IAddressBand methods ***
    virtual STDMETHODIMP FileSysChange(DWORD dwEvent, LPCITEMIDLIST * ppidl);        
    virtual STDMETHODIMP Refresh(VARIANT * pvarType);        

    // *** IPersistStream methods ***
    virtual STDMETHODIMP GetClassID(CLSID *pClassID){ *pClassID = CLSID_AddressBand; return S_OK; }
    virtual STDMETHODIMP Load(IStream *pStm);
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);
    virtual STDMETHODIMP IsDirty(void) {return S_OK;}       // Indicate that we are dirty and ::Save() needs to be called.

protected:
    //////////////////////////////////////////////////////
    // Private Member Functions
    //////////////////////////////////////////////////////

    // Constructor / Destructor
    CAddressBand();
    virtual ~CAddressBand();

    HRESULT _CreateAddressBand(IUnknown * punkSite);
    static LRESULT CALLBACK _ComboExWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL _CreateGoButton();
    void _InitGoButton();


    // Friend Functions
    friend HRESULT CAddressBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);   

    //////////////////////////////////////////////////////
    //  Private Member Variables 
    //////////////////////////////////////////////////////
    BOOL                _fVertical :1;

    // Only valid if _fInited == TRUE
    HWND                _hwndEdit;      // Address edit control child Window
    HWND                _hwndCombo;     // Address combo control child Window
    BOOL                _fVisible:1;    // TRUE when the toolbar is visible.
    BOOL                _fGoButton:1;   // TRUE if go button is visible
    IAddressEditBox*    _paeb;          // IAddressEditBox that controls
    IWinEventHandler*   _pweh;          // IWinEventHandler interface for the AddressEditBox object.  (Cached for Perf)
    HIMAGELIST          _himlDefault;   // default gray-scale go button
    HIMAGELIST          _himlHot;       // color go button
    HWND                _hwndTools;     // toolbar containing go button
    WNDPROC             _pfnOldWndProc; // Former WndProc of ComboBoxEx
};


#endif /* _ADDRESS_H */
