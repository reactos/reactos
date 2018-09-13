#ifndef BANDS_H_
#define BANDS_H_

#include "cowsite.h"

// this is a virtual class!
class CToolBand : public IDeskBand
                , public CObjectWithSite
                , public IInputObject
                , public IPersistStream
                , public IOleCommandTarget
                , public IServiceProvider
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IPersistStreamInit methods ***
    virtual STDMETHODIMP GetClassID(CLSID *pClassID) = 0;
    virtual STDMETHODIMP IsDirty(void);
    virtual STDMETHODIMP Load(IStream *pStm) = 0;
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty) = 0;
    virtual STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);

    // *** IOleCommandTarget methods ***
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // *** IServiceProvider methods ***
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID* ppvObj);

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return E_NOTIMPL; }

    // *** IDockingWindow methods ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dwReserved);
    virtual STDMETHODIMP ResizeBorderDW(LPCRECT prcBorder,
                                             IUnknown* punkToolbarSite,
                                             BOOL fReserved);

    // *** IObjectWithSite methods ***
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);

    // *** IDeskBand methods ***
    virtual STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                   DESKBANDINFO* pdbi) PURE;

    // *** IInputObject methods ***
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);
    virtual STDMETHODIMP HasFocusIO();
    virtual STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);

protected:
    CToolBand();
    virtual ~CToolBand();

    HRESULT _BandInfoChanged();

    int         _cRef;
    HWND        _hwnd;
    HWND        _hwndParent;
    //IUnknown* CObjectWithSite::_punkSite;
    BOOL        _fCanFocus:1;   // we accept focus (see UIActivateIO)
    DWORD       _dwBandID;
};


IDeskBand* CBrowserBand_Create(LPCITEMIDLIST pidl);
IDeskBand* CSearchBand_Create();

#define CX_FILENAME_AVG    (6 * 12)    // '8.3' name in 'typical' font (approx)






class CToolbarBand: public CToolBand,
                    public IWinEventHandler
{
public:
    // *** IWinEventHandler methods ***
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres);
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);

protected:
    HRESULT _PushChevron(BOOL bLast);
    LRESULT _OnHotItemChange(LPNMTBHOTITEM pnmtb);
    virtual LRESULT _OnNotify(LPNMHDR pnmh);
};



#endif  // BANDS_H_
