
#ifndef _MENUSITE_H_
#define _MENUSITE_H_

//#define WANT_CBANDSITE_CLASS

//#include "bandsite.h"
#include "cwndproc.h"

// MenuSite will never have more than one client.


class CMenuSite : public IBandSite,
                  public IDeskBarClient,
                  public IOleCommandTarget,
                  public IInputObject,
                  public IInputObjectSite,
                  public IWinEventHandler,
                  public IServiceProvider,
                  public CImpWndProc
{

public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IOleCommandTarget methods ***
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt,
        VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // *** IInputObjectSite methods ***
    virtual STDMETHODIMP OnFocusChangeIS(IUnknown *punk, BOOL fSetFocus);

    // *** IInputObject methods ***
    virtual STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual STDMETHODIMP HasFocusIO();
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IServiceProvider methods ***
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID* ppvObj);

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

    // *** IDeskBarClient methods ***
    virtual STDMETHODIMP SetDeskBarSite(IUnknown* punkSite);
    virtual STDMETHODIMP SetModeDBC(DWORD dwMode);
    virtual STDMETHODIMP UIActivateDBC(DWORD dwState);
    virtual STDMETHODIMP GetSize(DWORD dwWhich, LPRECT prc);

    // *** IWinEventHandler Methods ***
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres);
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);

    // *** IBandSite ***
    virtual STDMETHODIMP AddBand(IUnknown* punk);
    virtual STDMETHODIMP EnumBands(UINT uBand, DWORD* pdwBandID);
    virtual STDMETHODIMP QueryBand(DWORD dwBandID, IDeskBand** ppstb, DWORD* pdwState, LPWSTR pszName, int cchName);
    virtual STDMETHODIMP SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState);
    virtual STDMETHODIMP RemoveBand(DWORD dwBandID);
    virtual STDMETHODIMP GetBandObject(DWORD dwBandID, REFIID riid, LPVOID *ppvObj);
    virtual STDMETHODIMP SetBandSiteInfo(const BANDSITEINFO * pbsinfo);
    virtual STDMETHODIMP GetBandSiteInfo(BANDSITEINFO * pbsinfo);


    CMenuSite();
    
protected:
    virtual ~CMenuSite();
    virtual LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void _CreateSite(HWND hwndParent);
    void _CacheSubActiveBand(IUnknown * punk);


    IUnknown*   _punkSite;
    IUnknown*   _punkSubActive;
    IDeskBand*  _pdb;
    IWinEventHandler*   _pweh;
    HWND        _hwndChild;

    int         _cRef;    

    friend HRESULT CMenuBandSite_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
};

#endif  // _MENUSITE_H_
