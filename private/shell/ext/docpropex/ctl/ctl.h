//-------------------------------------------------------------------------//
// Ctl.h : Declaration of the CPropertyTreeCtl ActiveX control window
//-------------------------------------------------------------------------//

#ifndef __CTL_H__
#define __CTL_H__

//--------------//
//  forwards
class CPropertyTreeCtl;
class CPropertyTreeItem;
class CPropertyFolder;
class CProperty;
#include "resource.h"
#include "metrics.h"
#include "EditCtl.h"
#include "Dictionary.h"
#include "CtlConn.h"    // event proxy wrapper

//-------------------------------------------------------------------------//
const   UINT IDC_BASE = 0x6000;// arbitrary base for child window ctl IDs
#define DEFINE_CTL_ID(idc,amm,n)    const UINT idc=(n)+IDC_BASE, amm=(n)

//-------------------------------------------------------------------------//
//  Child window and message map IDs...
DEFINE_CTL_ID( IDC_TREE,     TREE_ALTMSGMAP,     1 );
DEFINE_CTL_ID( IDC_HDR,      HDR_ALTMSGMAP,      2 );
DEFINE_CTL_ID( IDC_REBARTOP, REBARTOP_ALTMSGMAP, 3 );
DEFINE_CTL_ID( IDC_REBARBTM, REBARBTM_ALTMSGMAP, 4 );
DEFINE_CTL_ID( IDC_QTIP,     QTIP_ALTMSGMAP,     5 );
DEFINE_CTL_ID( IDC_TOOL,     TOOL_ALTMSGMAP,     6 );
#define IDC_TOOLTIP          (IDC_BASE + 48)

//-------------------------------------------------------------------------//
//  Primary control window
class ATL_NO_VTABLE CPropertyTreeCtl
 : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CPropertyTreeCtl, &CLSID_PropertyTreeCtl>,
    public CComControl<CPropertyTreeCtl>,
    public CStockPropImpl<CPropertyTreeCtl, IPropertyTreeCtl, &IID_IPropertyTreeCtl, &LIBID_PROPERTYTREELib>,
    public IProvideClassInfo2Impl<&CLSID_PropertyTreeCtl, &DIID__DIPropertyTreeCtl, &LIBID_PROPERTYTREELib>,
    public IPersistStreamInitImpl<CPropertyTreeCtl>,
    public IPersistStorageImpl<CPropertyTreeCtl>,
    public IQuickActivateImpl<CPropertyTreeCtl>,
    public IOleControlImpl<CPropertyTreeCtl>,
    public IOleObjectImpl<CPropertyTreeCtl>,
    public IOleInPlaceActiveObjectImpl<CPropertyTreeCtl>,
    public IViewObjectExImpl<CPropertyTreeCtl>,
    public IOleInPlaceObjectWindowlessImpl<CPropertyTreeCtl>,
    public IDataObjectImpl<CPropertyTreeCtl>,
    public ISupportErrorInfo,
    public IConnectionPointContainerImpl<CPropertyTreeCtl>,
    public CProxy_DIPropertyTreeCtl<CPropertyTreeCtl>, // event invocation proxy
    public ISpecifyPropertyPagesImpl<CPropertyTreeCtl>
//-------------------------------------------------------------------------//
{
public:
    CPropertyTreeCtl();
    static CWndClassInfo& GetWndClassInfo();

    CFolderDictionary&   FolderDictionary()     { return m_folders; }
    CPropertyDictionary& PropertyDictionary()   { return m_properties; }
    const HWND&          TreeHwnd() const       { return m_wndTree.m_hWnd; }
    HRESULT              GetServerForSource( IN LPCOMVARIANT pvarSrc, OUT PTSERVER& server ) const;
    HRESULT              GetDirtyCount( OUT LONG& cDirty, OUT LONG& cDirtyVis );

    CPropertyTreeItem*   GetNextTreeItem( CPropertyTreeItem* pSrc /*NULL: get root item*/, BOOL bCycle );
#if 0  // this is untested and currently unneeded.
    CPropertyTreeItem*   GetPrevTreeItem( CPropertyTreeItem* pSrc /*NULL: get root item*/, BOOL bCycle );
#endif

    //-----------------------//
    // IPropertyTreeCtl methods
	STDMETHOD( AddSource )       ( /*[in]*/ const VARIANT* pvarSrc, /*[in, optional]*/ const VARIANT* pServer, /*[in]*/ ULONG dwDisposition ); 
	STDMETHOD( RemoveSource )    ( /*[in]*/ const VARIANT* pvarSrc, /*[in]*/ ULONG dwDisposition );
	STDMETHOD( RemoveAllSources )( /*[in]*/ ULONG dwDisposition );
	STDMETHOD( Apply )           ();
	STDMETHOD( GetPropertyValue) (/*[in]*/ BSTR FmtID, /*[in]*/ LONG nPropID, /*[in]*/ LONG nDataType, /*[out]*/ BSTR* pbstrVal, /*[out,optional]*/ VARIANT_BOOL* pbDirty );
	STDMETHOD( SetPropertyValue) (/*[in]*/ BSTR FmtID, /*[in]*/ LONG nPropID, /*[in]*/ LONG nDataType, /*[in]*/ BSTR bstrVal, /*[in]*/ VARIANT_BOOL bMakeDirty );
	STDMETHOD( IsPropertyDirty ) (/*[in]*/ BSTR FmtID, /*[in]*/ LONG nPropID, /*[in]*/ LONG nDataType, /*[out]*/ VARIANT_BOOL* pbDirty );

	
    //  IPropertyTreeCtl properties
    STDMETHOD( get_EmptyVisible )(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD( get_Empty )(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD( get_PropertyCountVisible )(/*[out, retval]*/ long *pVal);
	STDMETHOD( get_FolderCountVisible )(/*[out, retval]*/ long *pVal);
	STDMETHOD( get_PropertyCount )(/*[out, retval]*/ long *pVal);
	STDMETHOD( get_FolderCount )(/*[out, retval]*/ long *pVal);
	STDMETHOD( get_DirtyCount )(/*[out, retval]*/ long *pVal);
	STDMETHOD( get_NoCommonsCaption )(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD( put_NoCommonsCaption )(/*[in]*/ BSTR newVal);
	STDMETHOD( get_NoPropertiesCaption )(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD( put_NoPropertiesCaption )(/*[in]*/ BSTR newVal);
	STDMETHOD( get_NoSourcesCaption )(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD( put_NoSourcesCaption )(/*[in]*/ BSTR newVal);

    //-----------------------//
    // ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    //-----------------------//
    // IViewObjectEx
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
    {
        TRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
        *pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
        return S_OK;
    }

    //-----------------------//
    // IOleInPlaceObject
	STDMETHOD( SetObjectRects )( LPCRECT prcPos,LPCRECT prcClip );
    
    //-----------------------//
    // IOleInPlaceActiveObject
    STDMETHOD(TranslateAccelerator)( LPMSG /* lpmsg */ );

    //-----------------------//
    //  Stock property data
public:
    OLE_COLOR   m_clrBorderColor;
    OLE_COLOR   m_clrForeColor;
    BOOL        m_bTabStop;
    BOOL        m_bInfobarVisible;
    BOOL        m_bBorderVisible;
    long        m_nBorderStyle;

    //-----------------------//
    //  Other data
private:
    CContainedWindow     m_wndTree,
                         m_wndHdr,
                         m_wndInfobar,
#ifdef _PROPERTYTREE_TOOLBAR
                         m_wndRebarTop,
                         m_wndTool,
#endif _PROPERTYTREE_TOOLBAR
                         m_wndQtip;

    CMetrics             m_metrics;
	int					 m_iChildSortDir[2];
    IAdvancedPropertyServer* m_pDefaultServer;
    CLSID                m_clsidServer;
    BSTR                 m_bstrNoSourcesCaption,
                         m_bstrNoPropertiesCaption,
                         m_bstrNoCommonsCaption;                  ;
    HWND                 m_hwndEmpty;
    HWND                 m_hwndToolTip;
    HTREEITEM            m_hToolTipTarget;
    BOOL                 m_bDestroyed;


    //  Important collections
    CMasterSourceDictionary  m_srcs;
    CFolderDictionary    m_folders;
    CPropertyDictionary  m_properties;
    SHORT                m_nEmptyStatus;

    enum {
        EMPTY_NOTEMPTY,
        EMPTY_NOSOURCES,
        EMPTY_NOPROPERTIES,
        EMPTY_NOCOMMONS,
    };

public:
    LRESULT ProcessKey( UINT, WPARAM, BOOL, BOOL, BOOL& bHandled );

private:
    //-------------------------------//
    //  Window message handlers: Main window
    LRESULT OnCreate( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnDestroy( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSize( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSetFocus( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnNotifyFormat( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnGetDlgCode( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnRedraw( UINT, WPARAM, LPARAM, BOOL& );

    //-------------------------------//
    //  Window message handlers: TreeView child window
    LRESULT OnTreeLButtonDblClk( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnTreeLButtonDown( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnTreeViewHitTest( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnTreeVScroll( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnTreeHScroll( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnTreeMouseWheel( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnNavigationKey( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnTreeChildCommand( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnTreeChildNotify( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnCtlColorStatic( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnTreeSetFocus( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnCtlFocus( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnTreeKey( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnTreeMouseMove( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnTreeMouseActivate( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnTreeGetObject( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnTreeGetAccString( UINT, WPARAM, LPARAM, BOOL& );

    //-------------------------------//
    //  Window message handlers: Header child window
    LRESULT OnHeaderSetCursor( UINT, WPARAM, LPARAM, BOOL& );

#if _ATL_VER <= 0x0300
    LRESULT OnHeaderNcDestroy( UINT, WPARAM, LPARAM, BOOL& );
#endif _ATL_VER <= 0x0300

    //-------------------------------//
    //  Control notification handlers
    LRESULT OnToolbarToolTip( int, NMHDR*, BOOL& );
    LRESULT OnTreeGetInfoTip( int, NMHDR*, BOOL& );
    LRESULT OnTreeItemExpanding( int, NMHDR*, BOOL& );
    LRESULT OnTreeItemSelChanging( int, NMHDR*, BOOL& );
    LRESULT OnTreeItemSelChanged( int, NMHDR*, BOOL& );
    LRESULT OnTreeItemExpanded( int, NMHDR*, BOOL& );
    LRESULT OnTreeItemCallback( int, NMHDR*, BOOL& );
    LRESULT OnTreeItemCustomDraw( int, NMHDR*, BOOL& );
    LRESULT OnTreeBeginLabelEdit( int, NMHDR*, BOOL& );
    LRESULT OnTreeEndLabelEdit( int, NMHDR*, BOOL& );
    LRESULT OnTreeItemDelete( int, NMHDR*, BOOL& );
    LRESULT OnTreeQueryItemWidth( int, NMHDR*, BOOL& );
    LRESULT OnHeaderItemChanging( int, NMHDR*, BOOL& );
    LRESULT OnHeaderItemBeginTrack( int, NMHDR*, BOOL& );
    LRESULT OnHeaderDividerDblClick( int, NMHDR*, BOOL& );
	LRESULT OnHeaderItemClick( int, NMHDR*, BOOL& );
	LRESULT OnRebarBtmChildSize( int, NMHDR*, BOOL& );

    //-------------------------------//
    //  State methods, helper functions and window implementation
    CPropertyTreeItem* GetTreeItem( HTREEITEM hItem );
    CPropertyTreeItem* GetTreeItem( const TV_ITEM& tvi );
    CPropertyTreeItem* GetSelection();
    CPropertyTreeItem* SetDefaultSelection();
    
    HWND    CreateToolbar( HFONT );
    HWND    CreateQtip( HFONT );
    HWND    CreateRebar( CContainedWindow& wnd, UINT nID, DWORD dwAlignStyle );
    HWND    CreateToolTips( CContainedWindow& wndTree, HFONT );

    HWND    CreateEmptyPrompt( HFONT );

    HRESULT CreateFolder( IN const VARIANT* pvarSrc, IN const PROPFOLDERITEM& item, OUT CPropertyFolder** ppFolder = NULL );
    HRESULT CreateProperty( IN const VARIANT* pvarSrc, IN const PROPERTYITEM& item, OUT LPBOOL pbExisted, OUT CProperty** ppProperty = NULL );
    HRESULT Release( ULONG dwFlags );

    void    ReconcileTreeItems( BOOL bFolders, BOOL bProperties );
    void    PendRedraw( HWND hwnd, UINT nFlags = RDW_INVALIDATE|RDW_ERASE );

    BOOL    AddHeaderColumns( int cx );
    void    PositionControls( int cx, int cy );
    void    RepositionEditControl();
    int     GetBestFitHdrWidth( BOOL bReCalc = TRUE );
    BOOL    SetHeaderImage( int iCol, int iImage );
    int     GetVisiblePropertyItemRects( RECT rect[], int cRects );
    BOOL    GetItemValueRect( IN OPTIONAL HTREEITEM hItemSel, OUT LPRECT prcRect );
    void    CalcAvailableClientRect( IN OUT RECT&, IN CContainedWindow&, OUT RECT& ) const;
    SHORT   CalcEmptyStatus( BSTR *pbstrCaption );
    void    UpdateEmptyStatus( BOOL bInit = FALSE );
    HTREEITEM   UpdateToolTipTarget( BOOL bClear, const POINT& ptTreeItem );

    void    Sort( HTREEITEM hItem, int iCol, BOOL bAdvanceDirection );
    int*    GetSortDirection( IN int iCol, IN OPTIONAL HTREEITEM hItem, OUT OPTIONAL HTREEITEM* phItem = NULL );
	static  int CALLBACK SortItemProc( LPARAM, LPARAM, LPARAM ); 

    STDMETHODIMP ServerFromVariant( const VARIANT* pvarServer, IAdvancedPropertyServer** ppIfc );
    STDMETHODIMP FindServerForSource( const VARIANT * pvarSrc, IAdvancedPropertyServer** pppts );
    STDMETHODIMP CreateServerInstance( REFCLSID clsid, IAdvancedPropertyServer** pppts );


    BEGIN_MSG_MAP( CPropertyTreeCtl )
        //  Window message handlers
        MESSAGE_HANDLER( WM_CREATE,       OnCreate )
        MESSAGE_HANDLER( WM_DESTROY,      OnDestroy )
        MESSAGE_HANDLER( WM_SIZE,         OnSize )
        MESSAGE_HANDLER( WM_NOTIFYFORMAT, OnNotifyFormat )
        MESSAGE_HANDLER( WM_GETDLGCODE,   OnGetDlgCode )
        MESSAGE_HANDLER( WMU_REDRAW,      OnRedraw )

        //  Common control notification handlers
        NOTIFY_HANDLER( IDC_TOOL,  TTN_NEEDTEXT,        OnToolbarToolTip )
        NOTIFY_HANDLER( IDC_TREE,  TVN_GETINFOTIP,      OnTreeGetInfoTip )
        NOTIFY_HANDLER( IDC_TREE,  TVN_ITEMEXPANDING,   OnTreeItemExpanding )
        NOTIFY_HANDLER( IDC_TREE,  TVN_ITEMEXPANDED,    OnTreeItemExpanded )
        NOTIFY_HANDLER( IDC_TREE,  TVN_SELCHANGING,     OnTreeItemSelChanging )
        NOTIFY_HANDLER( IDC_TREE,  TVN_SELCHANGED,      OnTreeItemSelChanged )
        NOTIFY_HANDLER( IDC_TREE,  TVN_GETDISPINFO,     OnTreeItemCallback )
        NOTIFY_HANDLER( IDC_TREE,  NM_CUSTOMDRAW,       OnTreeItemCustomDraw )
        NOTIFY_HANDLER( IDC_TREE,  TVN_BEGINLABELEDIT,  OnTreeBeginLabelEdit )
        NOTIFY_HANDLER( IDC_TREE,  TVN_ENDLABELEDIT,    OnTreeEndLabelEdit )
        NOTIFY_HANDLER( IDC_TREE,  TVN_DELETEITEM,      OnTreeItemDelete )
#ifdef TVN_QUERYITEMWIDTH
        NOTIFY_HANDLER( IDC_TREE,  TVN_QUERYITEMWIDTH,  OnTreeQueryItemWidth )
#endif
        NOTIFY_HANDLER( IDC_HDR,   HDN_ITEMCHANGING,    OnHeaderItemChanging )
        NOTIFY_HANDLER( IDC_HDR,   HDN_BEGINTRACK,      OnHeaderItemBeginTrack )
        NOTIFY_HANDLER( IDC_HDR,   HDN_DIVIDERDBLCLICK, OnHeaderDividerDblClick )
		NOTIFY_HANDLER( IDC_HDR,   HDN_ITEMCLICK,       OnHeaderItemClick )
        NOTIFY_HANDLER( IDC_REBARBTM, RBN_CHILDSIZE,    OnRebarBtmChildSize )

        //  ATL message handlers
        MESSAGE_HANDLER( WM_PAINT,     OnPaint )
        MESSAGE_HANDLER( WM_SETFOCUS,  OnSetFocus )
        MESSAGE_HANDLER( WM_KILLFOCUS, OnKillFocus )

    //  Messages sent to tree view child window
    ALT_MSG_MAP( TREE_ALTMSGMAP )
        MESSAGE_HANDLER( WM_LBUTTONDBLCLK,   OnTreeLButtonDblClk )
        MESSAGE_HANDLER( WM_LBUTTONDOWN,     OnTreeLButtonDown )
        MESSAGE_HANDLER( TVM_HITTEST,        OnTreeViewHitTest )
        MESSAGE_HANDLER( WM_VSCROLL,         OnTreeVScroll )
        MESSAGE_HANDLER( WM_HSCROLL,         OnTreeHScroll )
        MESSAGE_HANDLER( WM_MOUSEWHEEL,      OnTreeMouseWheel )
        MESSAGE_HANDLER( WM_MOUSEMOVE,       OnTreeMouseMove )
        MESSAGE_HANDLER( WM_MOUSEACTIVATE,   OnTreeMouseActivate )
        MESSAGE_HANDLER( WM_GETDLGCODE,      OnGetDlgCode )
        MESSAGE_HANDLER( WM_CTLCOLORSTATIC,  OnCtlColorStatic )
        MESSAGE_HANDLER( WM_COMMAND,         OnTreeChildCommand )
        MESSAGE_HANDLER( WM_NOTIFY,          OnTreeChildNotify )
        MESSAGE_HANDLER( WM_KEYUP,           OnTreeKey )
        MESSAGE_HANDLER( WM_KEYDOWN,         OnTreeKey )
        MESSAGE_HANDLER( WM_CHAR,            OnTreeKey )
        MESSAGE_HANDLER( WM_SETFOCUS,        OnTreeSetFocus )
        MESSAGE_HANDLER( WMU_CTLFOCUS,       OnCtlFocus )
        MESSAGE_HANDLER( WMU_NAVIGATION_KEY, OnNavigationKey )
        MESSAGE_HANDLER( WM_GETOBJECT,       OnTreeGetObject )
        MESSAGE_HANDLER( WMU_GETACCSTRING(), OnTreeGetAccString )

    ALT_MSG_MAP( HDR_ALTMSGMAP )
        MESSAGE_HANDLER( WM_SETCURSOR,       OnHeaderSetCursor )
#if _ATL_VER <= 0x0300
        MESSAGE_HANDLER( WM_NCDESTROY,       OnHeaderNcDestroy )
#endif _ATL_VER <= 0x0300

    ALT_MSG_MAP( REBARTOP_ALTMSGMAP )
    ALT_MSG_MAP( REBARBTM_ALTMSGMAP )
    ALT_MSG_MAP( QTIP_ALTMSGMAP )
    ALT_MSG_MAP( TOOL_ALTMSGMAP )
    END_MSG_MAP()

    //-----------------------//
    //  ATL stuff...
    DECLARE_REGISTRY_RESOURCEID( IDR_PROPERTYTREECTL )

    BEGIN_COM_MAP(CPropertyTreeCtl )
        COM_INTERFACE_ENTRY( IPropertyTreeCtl )
        COM_INTERFACE_ENTRY( IDispatch )
        COM_INTERFACE_ENTRY_IMPL( IViewObjectEx )
        COM_INTERFACE_ENTRY_IMPL_IID( IID_IViewObject2, IViewObjectEx )
        COM_INTERFACE_ENTRY_IMPL_IID( IID_IViewObject, IViewObjectEx )
        COM_INTERFACE_ENTRY_IMPL( IOleInPlaceObjectWindowless )
        COM_INTERFACE_ENTRY_IMPL_IID( IID_IOleInPlaceObject, IOleInPlaceObjectWindowless )
        COM_INTERFACE_ENTRY_IMPL_IID( IID_IOleWindow, IOleInPlaceObjectWindowless )
        COM_INTERFACE_ENTRY_IMPL( IOleInPlaceActiveObject )
        COM_INTERFACE_ENTRY_IMPL( IOleControl )
        COM_INTERFACE_ENTRY_IMPL( IOleObject )
        COM_INTERFACE_ENTRY_IMPL( IQuickActivate )
        COM_INTERFACE_ENTRY_IMPL( IPersistStorage )
        COM_INTERFACE_ENTRY_IMPL( IPersistStreamInit )
        COM_INTERFACE_ENTRY_IMPL( ISpecifyPropertyPages )
        COM_INTERFACE_ENTRY_IMPL( IDataObject )
        COM_INTERFACE_ENTRY( IProvideClassInfo )
        COM_INTERFACE_ENTRY( IProvideClassInfo2 )
        COM_INTERFACE_ENTRY( ISupportErrorInfo )
        COM_INTERFACE_ENTRY_IMPL( IConnectionPointContainer )
    END_COM_MAP()

    BEGIN_PROPERTY_MAP( CPropertyTreeCtl )
        // Example entry: PROP_ENTRY( "Property Description", dispid, clsid )
        PROP_PAGE( CLSID_StockColorPage )
    END_PROPERTY_MAP()

    BEGIN_CONNECTION_POINT_MAP( CPropertyTreeCtl )
        CONNECTION_POINT_ENTRY( DIID__DIPropertyTreeCtl )
    END_CONNECTION_POINT_MAP()

    HRESULT FinalConstruct();
    void    FinalRelease();
    HRESULT OnDraw( ATL_DRAWINFO& di );
};

#endif //__CTL_H__
