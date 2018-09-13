#ifndef BaseBar_H_
#define BaseBar_H_
#include "cwndproc.h"

#ifdef __cplusplus

//========================================================================
// class CBaseBar (CBaseBar* pwbar)
//========================================================================
class CBaseBar : public IOleCommandTarget
               , public IServiceProvider
               , public IDeskBar
                ,public IInputObjectSite
                ,public IInputObject
               , public CImpWndProc
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IOleCommandTarget methods ***
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt,
        VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // *** IServiceProvider methods ***
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID* ppvObj);

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

    // *** IDeskBar methods ***
    virtual STDMETHODIMP SetClient(IUnknown* punk);
    virtual STDMETHODIMP GetClient(IUnknown** ppunkClient);
    virtual STDMETHODIMP OnPosRectChangeDB (LPRECT prc);

    // *** IInputObjectSite methods ***
    virtual STDMETHODIMP OnFocusChangeIS(IUnknown *punk, BOOL fSetFocus);

    // *** IInputObject methods ***
    virtual STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual STDMETHODIMP HasFocusIO();
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);

protected:
    // Constructor & Destructor
    CBaseBar();
    virtual ~CBaseBar();
    friend HRESULT CBaseBar_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
    
    BOOL _CheckForwardWinEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres);    

    virtual void _NotifyModeChange(DWORD dwMode); // NOTE: we may be abhe to get rid of this virtual...
    void _GetBorderRect(RECT* prc);

    virtual STDMETHODIMP ShowDW(BOOL fShow); // match IDockingWindow::ShowDW
    virtual STDMETHODIMP CloseDW(DWORD dwReserved); // match IDockingWindow::CloseDW
    virtual LRESULT _OnCommand(UINT msg, WPARAM wparam, LPARAM lparam);
    virtual LRESULT _OnNotify(UINT msg, WPARAM wparam, LPARAM lparam);
    virtual void _OnSize(void);
    virtual void _OnCreate();
    virtual void _OnPostedPosRectChange();
    virtual DWORD _GetExStyle();
    virtual DWORD _GetClassStyle();

    // Window procedure
    virtual LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    

    // Member variables
    UINT            _cRef;                  // reference count
    IUnknown*       _punkChild;             // ptr to IUnknown  for client area
    IDeskBarClient*     _pDBC;              // cached BaseBarClient for _punkChild
    IWinEventHandler*   _pWEH;              // cached IWenEventHandler for _punkChild
    HWND            _hwndChild;             // cached HWND      for _punkChild
    HWND            _hwndSite;              // hwnd of the site

    BOOL            _fShow :1;

    DWORD           _dwMode;

    SIZE            _szChild;               // last requested size from child

private:
    // Private members
    void _RegisterDeskBarClass();
    void _CreateDeskBarWindow();

};

#endif // __cplusplus
#endif

