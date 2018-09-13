// fsearch.h : Declaration of the CFileSearchBand

#ifndef __FSEARCH_H__
#define __FSEARCH_H__

#include "unicpp/stdafx.h"
#include "atldisp.h"    
#include "shcombox.h"   // shell combo methods
#include "fsrchdlg.h"

class CFindFilesDlg;

#define FILESEARCHCTL_CLASS     TEXT("ShellFileSearchControl")
#define _ENABLE_DESK_BAND_IMPL_ // enable deskband implementation

//  Band layout flags passed thru CFileSearchBand::UpdateLayout().
#define BLF_CALCSCROLL       0x00000001 // recalc scroll bars
#define BLF_SCROLLWINDOW     0x00000002 // scroll subdialog
#define BLF_RESIZECHILDREN   0x00000004 // resize subdialog
#define BLF_ALL              0xFFFFFFFF // do all layout ops

const UINT _icons[] = {
    //  replaced icons for fsearch, csearch with riff animations.
    IDI_PSEARCH,
};

//-------------------------------------------------------------------------//
//  CMetrics: maintains ctl metrics and resources.
class CMetrics
//-------------------------------------------------------------------------//
{
public:
    CMetrics();
    ~CMetrics() { 
        DestroyResources(); 
    }

    void  Init( HWND hwndDlg );
    void  OnWinIniChange( HWND hwndDlg );
    static COLORREF TextColor()   { return GetSysColor( COLOR_WINDOWTEXT ); }
    static COLORREF BkgndColor()  { return GetSysColor( COLOR_WINDOW ); }
    static COLORREF BorderColor() { return GetSysColor( COLOR_WINDOWTEXT ); }
    const HBRUSH&   BkgndBrush() const  { return _hbrBkgnd; }
    const HBRUSH&   BorderBrush() const { return _hbrBorder; }

    POINT&  ExpandOrigin() { return _ptExpandOrigin; }
    RECT&   CheckBoxRect() { return _rcCheckBox; }
    int&    TightMarginY() { return _cyTightMargin; }
    int&    LooseMarginY() { return _cyLooseMargin; }
    int&    CtlMarginX()   { return _cxCtlMargin; }
    HFONT   BoldFont( HWND hwndDlg );
    HICON   CaptionIcon( UINT nIDIconResource );

protected:
    BOOL    CreateResources();
    VOID    DestroyResources();
    static  BOOL GetWindowLogFont( HWND hwnd, OUT LOGFONT* plf );

    
    HBRUSH  _hbrBkgnd;
    HBRUSH  _hbrBorder;
    POINT   _ptExpandOrigin; // left-top origin of subdlg expansion 
    RECT    _rcCheckBox;     // size of a check box
    int     _cyTightMargin;  // v. distance between two tightly associated controls.
    int     _cyLooseMargin;  // v. distance between two loosely associated controls.
    int     _cxCtlMargin;    // distance between left or right dlg border and child window border.
    HFONT   _hfBold;         // Bold font
    HICON   _rghiconCaption[ARRAYSIZE(_icons)];
};

//-------------------------------------------------------------------------//
// CFileSearchBand
class ATL_NO_VTABLE CFileSearchBand : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CFileSearchBand, &CLSID_FileSearchBand>,
    public CComControl<CFileSearchBand>,
    public CStockPropImpl<CFileSearchBand, IFileSearchBand, &IID_IFileSearchBand, &LIBID_Shell32>,
    public IProvideClassInfo2Impl<&CLSID_FileSearchBand, NULL, &LIBID_Shell32>,
    public IPersistStreamInitImpl<CFileSearchBand>,
    public IPersistStorageImpl<CFileSearchBand>,
    public IQuickActivateImpl<CFileSearchBand>,
    public IOleControlImpl<CFileSearchBand>,
    public IOleInPlaceActiveObjectImpl<CFileSearchBand>,
    public IViewObjectExImpl<CFileSearchBand>,
    public IOleInPlaceObjectWindowlessImpl<CFileSearchBand>,
    public IDataObjectImpl<CFileSearchBand>,
    public ISpecifyPropertyPagesImpl<CFileSearchBand>,

#ifdef _ENABLE_DESK_BAND_IMPL_
    //  Add'l interfaces for Band functionality
    public IDeskBand,
    public IObjectWithSite,
    public IPersistStream,
    public IInputObject,
    public IServiceProvider,
    public IOleCommandTarget,
#endif _ENABLE_DESK_BAND_IMPL_

    //  Must derive from CShell32AtlIDispatch<> and not IOleObjecImpl 
    //  if this control resides in shell32.dll
    public CShell32AtlIDispatch<CFileSearchBand, &CLSID_FileSearchBand, &IID_IFileSearchBand, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>

//-------------------------------------------------------------------------//
{
public:
    CFileSearchBand();
    ~CFileSearchBand();
    static  CWndClassInfo& GetWndClassInfo( );
    HWND    Create( HWND hWndParent, RECT& rcPos, LPCTSTR szWindowName = NULL, 
                    DWORD dwStyle = WS_CHILD|WS_VISIBLE, DWORD dwExStyle = 0, UINT nID = 0 );
    void    SetDeskbandWidth( int cx );
    void    FinalRelease();

public:
    BOOL        IsDeskBand() const       { return _fDeskBand; }
    static BOOL IsBandDebut();
    CMetrics&   GetMetrics()             { return _metrics; }
    static int  MakeBandKey( OUT LPTSTR pszKey, IN UINT cchKey );
    static int  MakeBandSubKey( IN LPCTSTR pszSubKey, OUT LPTSTR pszKey, IN UINT cchKey );
    static HKEY GetBandRegKey( BOOL bForceCreate = FALSE );
    void        UpdateLayout( ULONG fLayoutFlags = BLF_ALL );
    void        EnsureVisible( LPCRECT lprc /* in screen coords */);
    BOOL        IsKeyboardScroll( MSG* pmsg );
    HRESULT     IsDlgMessage( HWND hwnd, LPMSG pmsg );
    HRESULT     AutoActivate();

    void        SetDirty( BOOL bDirty = TRUE );
    BOOL        IsDirty() const          { return _fDirty; }
    BOOL        IsValid() const          { return _fValid; }

    IUnknown*       BandSite()           { return _punkSite ? _punkSite : (IUnknown*)m_spClientSite; }
    IOleClientSite* OleClientSite()      { return m_spClientSite; }


    DECLARE_NO_REGISTRY();

    //------------------//
    //  Interface map:
    BEGIN_COM_MAP(CFileSearchBand)
        COM_INTERFACE_ENTRY(IFileSearchBand)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
        COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
        COM_INTERFACE_ENTRY_IMPL(IOleControl)
        COM_INTERFACE_ENTRY_IMPL(IOleObject)
        COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        //COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_IMPL(IDataObject)
        COM_INTERFACE_ENTRY(IProvideClassInfo)
        COM_INTERFACE_ENTRY(IProvideClassInfo2)

        COM_INTERFACE_ENTRY(IDeskBand)
        COM_INTERFACE_ENTRY(IOleCommandTarget)
        COM_INTERFACE_ENTRY(IInputObject)
        COM_INTERFACE_ENTRY(IObjectWithSite)
        COM_INTERFACE_ENTRY(IPersist)
        COM_INTERFACE_ENTRY(IPersistStream)
    END_COM_MAP()


    //------------------//
    //  Properties
public:
    BEGIN_PROPERTY_MAP(CFileSearchBand)
        // PROP_ENTRY("Property Description", dispid, clsid)
        PROP_PAGE(CLSID_StockColorPage)
    END_PROPERTY_MAP()

    //  message map
    BEGIN_MSG_MAP(CFileSearchBand)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
        MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_HSCROLL, OnScroll)
        MESSAGE_HANDLER(WM_VSCROLL, OnScroll)
        MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate)
        MESSAGE_HANDLER(WM_SETTINGCHANGE,  OnWinIniChange)
        MESSAGE_HANDLER(WM_SYSCOLORCHANGE,  OnWinIniChange)
        MESSAGE_HANDLER(WM_WININICHANGE,  OnWinIniChange)
        MESSAGE_HANDLER(WMU_BANDINFOUPDATE, OnBandInfoUpdate)
    END_MSG_MAP()

    //---------------------//
    //  Message handling
protected:
    LRESULT OnCreate( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnScroll( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnEraseBkgnd( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnMouseActivate(UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnWinIniChange(UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnBandInfoUpdate(UINT, WPARAM, LPARAM, BOOL& );

public:
    //---------------------//
    // IFileSearchBand methods
    STDMETHOD (SetFocus)();
    STDMETHOD (SetSearchParameters)( IN BSTR* pbstrSearchID, 
                                     IN VARIANT_BOOL bNavToResults,
                                     IN OPTIONAL VARIANT *pvarScope, 
                                     IN OPTIONAL VARIANT* pvarQueryFile );
    STDMETHOD (get_SearchID)( OUT BSTR* pbstrSearchID );
    STDMETHOD (get_Scope)( OUT VARIANT *pvarScope );
    STDMETHOD (get_QueryFile)( OUT VARIANT *pvarFile );

    STDMETHOD (FindFilesOrFolders)( BOOL bNavigateToResults = FALSE, 
                                    BOOL bDefaultFocusCtl = FALSE );
    STDMETHOD (FindComputer)( BOOL bNavigateToResults = FALSE, 
                              BOOL bDefaultFocusCtl = FALSE );
    STDMETHOD (FindPrinter) ( BOOL bNavigateToResults = FALSE, 
                              BOOL bDefaultFocusCtl = FALSE );
    STDMETHOD (FindPeople)  ( BOOL bNavigateToResults = FALSE, 
                              BOOL bDefaultFocusCtl = FALSE );
    STDMETHOD (FindOnWeb)   ( BOOL bNavigateToResults = FALSE, 
                              BOOL bDefaultFocusCtl = FALSE );

    HRESULT OnDraw(ATL_DRAWINFO& di)    { return S_OK; }
    STDMETHOD (SetObjectRects)( LPCRECT lprcPosRect, LPCRECT lprcClipRect ); 

    //----------------------//
    //  CShell32AtlIDispatch/IOleObject methods
    STDMETHOD (PrivateQI)(REFIID iid, void** ppvObject);
    STDMETHOD (DoVerbUIActivate)( LPCRECT prcPosRect, HWND hwndParent );
    STDMETHOD (TranslateAcceleratorInternal)( MSG *pMsg, IOleClientSite * pocs );

    //---------------------//
    // IViewObjectEx methods
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
    {
        ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
        *pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
        return S_OK;
    }

    //---------------------//
    //  IOleInPlaceActiveObject methods
    STDMETHOD (TranslateAccelerator)( LPMSG lpmsg );

    //---------------------//
    //  IDeskBand
    STDMETHOD (GetBandInfo)( DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi );

    //---------------------//
    //  IDockingWindow
    STDMETHOD (ShowDW)(BOOL fShow);
    STDMETHOD (CloseDW)(DWORD dwReserved);
    STDMETHOD (ResizeBorderDW)(LPCRECT prcBorder, IUnknown* punkToolbarSite, BOOL fReserved);
    
    //---------------------//
    //  IOleWindow
    STDMETHOD (GetWindow)( HWND * lphwnd )              { *lphwnd = m_hWnd; return IsWindow( m_hWnd ) ? S_OK : S_FALSE; }
    STDMETHOD (ContextSensitiveHelp)( BOOL fEnterMode ) { return E_NOTIMPL; }

    //---------------------//
    //  IObjectWithSite
    STDMETHOD (SetSite)( IUnknown* punkSite );
    STDMETHOD (GetSite)( REFIID riid, void** ppunkSite );

    //---------------------//
    //  IInputObject
    STDMETHOD (HasFocusIO)(void);
    STDMETHOD (TranslateAcceleratorIO)(LPMSG lpMsg);
    STDMETHOD (UIActivateIO)(BOOL fActivate, LPMSG lpMsg);

    //---------------------//
    //  IOleCommandTarget
    STDMETHOD (QueryStatus)( const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText );
    STDMETHOD (Exec)( const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut );

    //---------------------//
    //  IServiceProvider
    STDMETHOD (QueryService)( REFGUID guidService, REFIID riid, void** ppv );

    //---------------------//
    //  IPersist
    STDMETHOD (GetClassID)(CLSID *pClassID); //

    //---------------------//
    //  IPersistStream
    STDMETHOD (IsDirty)(void);
    STDMETHOD (Load)(IStream *pStm); //
    STDMETHOD (Save)(IStream *pStm, BOOL fClearDirty); //
    STDMETHOD (GetSizeMax)(ULARGE_INTEGER *pcbSize);

    BOOL  SearchInProgress() const 
    {   
        if (_pBandDlg)
            return _pBandDlg->SearchInProgress();
        return FALSE;
    };
    void  StopSearch() 
    {
        if (_pBandDlg)
            _pBandDlg->StopSearch();
    };
    //---------------------//
    //  Private methods and data
private:
    CBandDlg*           GetBandDialog( REFGUID guidSearch );
    HRESULT             ShowBandDialog( REFGUID guidSearch, 
                                        BOOL bNavigateToResults = FALSE, 
                                        BOOL bDefaultFocusCtl = FALSE /* force focus to the band's default control */ );
    HRESULT             AdvertiseBand( BOOL bAdvertise );
    HRESULT             BandInfoChanged();
    void                AddButtons( BOOL );
    BOOL                LoadImageLists();
    void                LayoutControls( int cx, int cy, ULONG fLayoutFlags = BLF_ALL );
    void                EnableControls();
    void                Scroll( int nBar, UINT uSBCode, int nNewPos = 0 );
    IShellBrowser*      GetTopLevelBrowser();
    BOOL                IsBrowserAccelerator( LPMSG pmsg );

    CBandDlg*           BandDlg();

    CFindFilesDlg       _dlgFSearch;
    CFindComputersDlg   _dlgCSearch;
#ifdef __PSEARCH_BANDDLG__
    CFindPrintersDlg    _dlgPSearch;
#endif __PSEARCH_BANDDLG__
    CBandDlg*           _pBandDlg;     // the active/visible band.
    GUID                _guidSearch;   // the GUID of the active/visible band.

    CMetrics            _metrics;
    SIZE                _sizeMin,
                        _sizeMax;
    IUnknown*           _punkSite;
    SCROLLINFO          _siHorz,
                        _siVert;
    BITBOOL             _fValid : 1,
                        _fDirty : 1,
                        _fDeskBand : 1,
                        _fStrings : 1;
    DWORD               _dwBandID,
                        _dwBandViewMode;
    HIMAGELIST          _hilHot,                 // toolbar image lists
                        _hilDefault;
    IShellBrowser*      _psb;           // top-level browser.
    LONG_PTR            _cbOffset;
};

//-------------------------------------------------------------------------//
//  Index Server control
//-------------------------------------------------------------------------//
//  Query dialects:
#ifndef ISQLANG_V1
#define ISQLANG_V1 1 
#endif  //ISQLANG_V1

#ifndef ISQLANG_V2
#define ISQLANG_V2 2
#endif  //ISQLANG_V2


extern "C" 
{
    HRESULT GetCIStatus( LPBOOL pbRunning, LPBOOL pbIndexed, LPBOOL pbPermission );
    HRESULT QueryCIStatus( LPDWORD pdwStatus, LPBOOL pbConfigAccess );
    HRESULT StartStopCI( BOOL bStart, BOOL bPersist );
    HRESULT MakeDefaultCiQuery( IN const VARIANT* pvarRaw, OUT VARIANT* pvarQuery, OUT ULONG* pulDialect );
    BOOL    IsCiQuery( IN const VARIANT* pvarRaw, OUT VARIANT* pvarQuery, OUT ULONG* pulDialect, BOOL bBangRequired );

}

#endif //__FSEARCH_H__
