
#ifndef _bandsite_h
#define _bandsite_h

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#define SZ_REGKEY_GLOBALADMINSETTINGS TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\IEAK")
#define SZ_REGVALUE_GLOBALADMINSETTINGS TEXT("Admin Band Settings")

// Admin Settings (dwAdminSettings)
#define BAND_ADMIN_NORMAL       0x00000000
#define BAND_ADMIN_NODDCLOSE    0x00000001  // Disable Drag & Drop, and Close
#define BAND_ADMIN_NOMOVE       0x00000002  // Disable Moving within a Bar.
#define BAND_ADMIN_ADMINMACHINE 0x80000000  // This is an admin machine and this bit displays the two Admin Context Menu items

BOOL BandSite_HandleMessage(IUnknown *punk, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);
void BandSite_SetMode(IUnknown *punk, DWORD dwMode);

BOOL ConfirmRemoveBand(HWND hwnd, UINT uID, LPCTSTR szName);

#ifdef WANT_CBANDSITE_CLASS

// UIActivateIO callback instance data
typedef struct tagACTDATA {
    LPMSG   lpMsg;  // IN
    HRESULT hres;
    IUnknown *punk;
} ACTDATA;


typedef struct tagBANDITEMDATA
{
    HWND hwnd;
    IDeskBand *pdb;
    IWinEventHandler *pweh;
    POINTL ptMinSize;
    POINTL ptMaxSize;
    POINTL ptIntegral;
    POINTL ptActual;
    WCHAR szTitle[256];
    DWORD dwModeFlags;
    DWORD dwBandID;
    BITBOOL fShow:1;            // current show state
    BITBOOL fNoTitle:1;         // 1:don't show title
    DWORD dwAdminSettings;
    COLORREF crBkgnd;
} BANDITEMDATA, *LPBANDITEMDATA;

typedef int (*PFNBANDITEMENUMCALLBACK)(LPBANDITEMDATA pbid, LPVOID pData);

int     _UIActIOCallback(LPBANDITEMDATA pbid, void *pv);

typedef struct {
    HRESULT hres;
    const GUID * pguidService;
    const IID * piid;
    void ** ppvObj;
} QSDATA;

int     _QueryServiceCallback(LPBANDITEMDATA pbid, void *pv);

#include "caggunk.h"

class CBandSite : public CAggregatedUnknown
                , public IBandSite
                , public IInputObjectSite
                , public IInputObject
                , public IDeskBarClient
                , public IWinEventHandler
                , public IPersistStream
                , public IDropTarget
                , public IServiceProvider
                , public IBandSiteHelper
                , public IOleCommandTarget
{

public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) { return CAggregatedUnknown::QueryInterface(riid, ppvObj);};
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return CAggregatedUnknown::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release(void) { return CAggregatedUnknown::Release();};

    // *** IPersistStream methods ***
    virtual STDMETHODIMP GetClassID(CLSID *pClassID);
    virtual STDMETHODIMP IsDirty(void);
    virtual STDMETHODIMP Load(IStream *pStm);
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);
    virtual STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);

    // *** IBandSite methods ***
    virtual STDMETHODIMP AddBand(IUnknown* punk);
    virtual STDMETHODIMP EnumBands(UINT uBand, DWORD* pdwBandID);
    virtual STDMETHODIMP QueryBand(DWORD dwBandID, IDeskBand** ppstb, DWORD* pdwState, LPWSTR pszName, int cchName);
    virtual STDMETHODIMP SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState);
    virtual STDMETHODIMP RemoveBand(DWORD dwBandID);
    virtual STDMETHODIMP GetBandObject(DWORD dwBandID, REFIID riid, LPVOID* ppvObj);
    virtual STDMETHODIMP SetBandSiteInfo(const BANDSITEINFO * pbsinfo);
    virtual STDMETHODIMP GetBandSiteInfo(BANDSITEINFO * pbsinfo);
    
    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return E_NOTIMPL; }
    
    // *** IInputObjectSite methods ***
    virtual STDMETHODIMP OnFocusChangeIS(IUnknown *punk, BOOL fSetFocus);

    // *** IInputObject methods ***
    virtual STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual STDMETHODIMP HasFocusIO();
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IDeskBarClient methods ***
    virtual STDMETHODIMP SetDeskBarSite(THIS_ IUnknown* punkSite) ;
    virtual STDMETHODIMP SetModeDBC (THIS_ DWORD dwMode) ;
    virtual STDMETHODIMP UIActivateDBC(THIS_ DWORD dwState) ;
    virtual STDMETHODIMP GetSize    (THIS_ DWORD dwWhich, LPRECT prc);
    
    // *** IWinEventHandler methods ***
    virtual STDMETHODIMP OnWinEvent (HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);
    
    // *** IDropTarget ***
    virtual STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragLeave(void);
    virtual STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // *** IServiceProvider methods ***
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID* ppvObj);

    // *** IBandSiteHelper methods ***
    virtual STDMETHODIMP LoadFromStreamBS(IStream *pstm, REFIID riid, LPVOID *ppv);
    virtual STDMETHODIMP SaveToStreamBS(IUnknown *punk, IStream *pstm);

    // *** IOleCommandTarget ***
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    CBandSite(IUnknown *punkAgg);

protected:
    virtual HRESULT v_InternalQueryInterface(REFIID riid, LPVOID * ppvObj);
    IDeskBar* _pdb;
    IUnknown* _punkSite;
    IBandSite* _pbsOuter; // the aggregating bandsite
    
    virtual HRESULT _Initialize(HWND hwndParent);
    virtual void _OnCloseBand(DWORD dwBandID);
    virtual LRESULT _OnBeginDrag(NMREBAR* pnm);
    virtual LRESULT _OnNotify(LPNMHDR pnm);
    virtual IDropTarget* _WrapDropTargetForBand(IDropTarget* pdtBand);
    virtual DWORD _GetWindowStyle(DWORD* pdwExStyle);
    virtual HMENU _LoadContextMenu();
    HRESULT _OnBSCommand(int idCmd, DWORD idBandActive, LPBANDITEMDATA pbid);

    HRESULT _AddBand(IUnknown* punk);

    virtual HRESULT _OnContextMenu(WPARAM wParam, LPARAM lParam);
    IDataObject* _DataObjForBand(DWORD dwBandID);
    LPBANDITEMDATA _GetBandItemDataStructByID(DWORD uID);
    virtual int _ContextMenuHittest(LPARAM lParam, POINT* ppt);

    // container specific (rebar) members

    virtual BOOL            _AddBandItem(LPBANDITEMDATA pbid);
    virtual void            _DeleteBandItem(int i);
    LPBANDITEMDATA  _GetBandItem(int i);
    int             _GetBandItemCount();
    void            _BandItemEnumCallback(int dincr, PFNBANDITEMENUMCALLBACK pfnCB, void *pv);
    void            _DeleteAllBandItems();
    virtual void    _ShowBand(LPBANDITEMDATA pbid, BOOL fShow);

    int             _BandIDToIndex(DWORD dwBandID);
    DWORD           _IndexToBandID(int i);
    DWORD           _BandIDFromPunk(IUnknown* punk);

    HRESULT         _SetBandStateHelper(DWORD dwBandID, DWORD dwMask, DWORD dwState);
    virtual void    _UpdateAllBands(BOOL fBSOnly, BOOL fNoAutoSize);
    void            _UpdateBand(DWORD dwBandID);
    BOOL            _UpdateBandInfo(LPBANDITEMDATA pbid, BOOL fBSOnly);

    void            _GetBandInfo(LPBANDITEMDATA pbid, DESKBANDINFO *pdbi);
    virtual void    _BandInfoFromBandItem(REBARBANDINFO *prbbi, LPBANDITEMDATA pbid, BOOL fBSOnly);
    virtual void    v_SetTabstop(LPREBARBANDINFO prbbi);
    BOOL            _IsEnableTitle(LPBANDITEMDATA pbid);

    HRESULT         _LoadBandInfo(IStream *pstm, int i, DWORD dwVersion);
    HRESULT         _SaveBandInfo(IStream *pstm, int i);

    HRESULT _AddBandByID(IUnknown *punk, DWORD dwID);
    BOOL _SendToToolband(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres);
    BOOL _HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres);
    int _BandIndexFromPunk(IUnknown *punk);
    BOOL _IsBandTabstop(LPBANDITEMDATA pbid);
    IUnknown* _GetNextTabstopBand(IUnknown* ptb, BOOL fBackwards);
    virtual HRESULT _CycleFocusBS(LPMSG lpMsg);
    void _OnRBAutoSize(NMRBAUTOSIZE* pnm);
    void _DoDragDrop();
    BOOL _PreDragDrop();
    virtual void _Close();

    
    BOOL _IsBandDeleteable(DWORD dwBandID);
    void _MaximizeBand(DWORD dwBandID);
    void _CheckNotifyOnAddRemove(DWORD dwBandID, int iCode);

    DWORD _GetAdminSettings(DWORD dwBandID);
    void _SetAdminSettings(DWORD dwBandID, DWORD dwNewAdminSettings);

    void _ReleaseBandItemData(LPBANDITEMDATA pbid, int iIndex);    

    void _CacheActiveBand(IUnknown* ptb);
    HRESULT _IsRestricted(DWORD dwBandID, DWORD dwRestrictAction, DWORD dwBandFlags);

    virtual ~CBandSite();
    
    HWND  _hwnd;
    HDSA  _hdsaBandItemData;
    DWORD _dwMode;
    DWORD _dwShowState;
    DWORD _dwBandIDNext;
    DWORD _dwStyle;

    IDataObject* _pdtobj;   // the stored drag drop data object;
    UINT    _uDragBand;
    DWORD   _dwDropEffect;
    
    IOleCommandTarget *_pct;
    IUnknown *_ptbActive;

    // cache for message reflector stuff
    HWND    _hwndCache;
    IWinEventHandler *_pwehCache;

    IShellLinkA *_plink;
    IBandProxy * _pbp;
    BITBOOL _fCreatedBandProxy:1;
    BITBOOL _fDragSource :1;
    BITBOOL _fNoDropTarget :1;
    BITBOOL _fIEAKInstalled :1;    // If TRUE, then display 2 extra contex menu items for Admins to use.
    UINT    _fDragging:2;           // we're dragging (0:FALSE 1:move [2:size])
    HWND    _hwndDD;                // window for cool D&D cursor drawing.
    
    friend HRESULT CBandSite_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
};

typedef enum {
    CNOAR_ADDBAND       =   1,
    CNOAR_REMOVEBAND    =   2,
    CNOAR_CLOSEBAR      =   3,
} CNOAR_CODES;

#endif // WANT_CBANDSITE_CLASS


#ifdef __cplusplus
};       /* End of extern "C" { */
#endif // __cplusplus

#endif
