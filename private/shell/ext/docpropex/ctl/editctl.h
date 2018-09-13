//-------------------------------------------------------------------------//
//
//  EditCtl.h
//
//-------------------------------------------------------------------------//

#ifndef __EDITCTL_H__
#define __EDITCTL_H__

#include "PTServer.h"   // generated from IDL.
#include "Resource.h"

//-------------------------------------------------------------------------//
//  Forwards
class CInPlaceBase;
class CInPlaceDropList;
class CInPlaceDropWindow;
class CInPlaceDropCalendar;
class CInPlaceDropEdit;

//-------------------------------------------------------------------------//
//  In place control window ID defs
#define INPLACE_CTLID_BASE 0x220  // arbitrary
#define MAKE_INPLACE_CTLID( ptctlid )  ((ptctlid)+INPLACE_CTLID_BASE)
#define MAKE_PTCTLID( idc )            ((idc)-INPLACE_CTLID_BASE)
#define SAFE_HWND( hwnd )              (IsWindow((hwnd)) ? (hwnd) : NULL)

//-------------------------------------------------------------------------//
//  PTCTLID_ values come from IDL file, and are known to server.
//  The following enumeration defines corresponding child window IDs
//  based on these values.
enum INPLACE_CTLID
{
    IDC_PE_NIL =0,
    IDC_PE_SINGLINE_EDIT   = MAKE_INPLACE_CTLID( PTCTLID_SINGLELINE_EDIT ),
    IDC_PE_MULTILINE_EDIT  = MAKE_INPLACE_CTLID( PTCTLID_MULTILINE_EDIT ),
    IDC_PE_DROPDOWN_COMBO  = MAKE_INPLACE_CTLID( PTCTLID_DROPDOWN_COMBO ),
    IDC_PE_DROPLIST_COMBO  = MAKE_INPLACE_CTLID( PTCTLID_DROPLIST_COMBO ),
    IDC_PE_CALENDARTIME    = MAKE_INPLACE_CTLID( PTCTLID_CALENDARTIME ),
    IDC_PE_CALENDAR        = MAKE_INPLACE_CTLID( PTCTLID_CALENDAR ),
    IDC_PE_TIME            = MAKE_INPLACE_CTLID( PTCTLID_TIME ),
};

//-------------------------------------------------------------------------//
//  Names of relevant window classes
extern const TCHAR  szEDITCLASS[],
                    szBUTTONCLASS[],
                    szCOMBOBOXCLASS[],
                    szDROPWINDOWCLASS[];

//-------------------------------------------------------------------------//
//  Private message identifiers
const UINT  WMU_INPLACE_BASE    = (WM_APP + INPLACE_CTLID_BASE),   // placeholder
            WMU_CTLFOCUS        = WMU_INPLACE_BASE + 0,             // Sent to top level ctl window when a child has gained or lost focus.
            WMU_NAVIGATION_KEY  = WMU_INPLACE_BASE + 1,             // navigation key handler
            WMU_SETITEMDATA     = WMU_INPLACE_BASE + 2,             // Assigns a value to a custom in-place edit control (msg args are specific to the control)
            WMU_SELFCOMMAND     = WMU_INPLACE_BASE + 3,             // WM_COMMAND handler
            WMU_SELFNOTIFY      = WMU_INPLACE_BASE + 4,             // WM_NOTIFY handler
            WMU_SHOWDROP        = WMU_INPLACE_BASE + 5,             // Drop window show, hide
            WMU_REDRAW          = WMU_INPLACE_BASE + 6,             // WPARAM: rdwFlags, LPARAM hwnd to redraw
            WMU_SETCAPTURE      = WMU_INPLACE_BASE + 7;            // Auto-posted to initiate mouse capture.

//-------------------------------------------------------------------------//
//  Structure passed as LPARAM member of WMU_NAVIGATION_KEY message
typedef struct tagNAVIGATION_KEY_INFO
{
    HWND    hwndTarget;
    UINT    nMsg;
    DWORD   virtKey;
    LPARAM                          lKeyData;
    BOOL    bCtrl;
    BOOL    bShift;
    BOOL    bHandled;
} NAVIGATION_KEY_INFO, *PNAVIGATION_KEY_INFO;

//  Initialization helper:
#define InitNavigationKeyInfo( pNKI, hwnd, msg, vk, keydata ) \
    (pNKI)->bHandled = FALSE;  (pNKI)->hwndTarget = (hwnd); \
    (pNKI)->nMsg = (msg);      (pNKI)->virtKey = (DWORD)(vk); \
    (pNKI)->lKeyData = keydata; \
    (pNKI)->bCtrl  = (GetKeyState(VK_CONTROL) & 0x8000)!=0; \
    (pNKI)->bShift = (GetKeyState(VK_SHIFT) & 0x8000)!=0; 

//-------------------------------------------------------------------------//
//  Additional helpers
void MakeSingleLine( LPTSTR pszText, int cchText, int cchBuf );

//-------------------------------------------------------------------------//
//  Base class for all in-place edit controls
class CInPlaceBase : public CWindowImpl< CInPlaceBase >
//-------------------------------------------------------------------------//
{
public:
    CInPlaceBase( HWND hwndTree ) : m_hwndTree( hwndTree ), m_type( CT_NIL ) {
        ASSERT( IsWindow( hwndTree ) );
    }

    static  CWndClassInfo& GetWndClassInfo()    {
        static CWndClassInfo wc =
        {
            { sizeof(WNDCLASSEX), 0, StartWindowProc,
              0, 0, 0, 0, 0, (HBRUSH)(COLOR_WINDOW+1), 0, TEXT("PropTreeInPlaceBase"), 0 },
              NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
        };
        return wc;
    }

    enum CTLTYPE  {
        CT_NIL,
        CT_EDIT,
        CT_CBDROPLIST,
        CT_CBDROPDOWN,
        CT_CALENDAR,
        CT_TIME,
        CT_CALENDARTIME,
    };
    CTLTYPE GetType() const     { return m_type; }

    virtual BOOL    SubclassWindow( HWND hwnd, BOOL bSearchChildren = TRUE );

    virtual UINT    GetDropState() const;

    #define DROPSTATE_CLOSED  0x0001
    #define DROPSTATE_DROPPED 0x0002
    #define DROPSTATE_OK      0x0010
    #define DROPSTATE_CANCEL  0x0020

    #define DROPANCHOR_TOP    0x0000
    #define DROPANCHOR_BOTTOM 0x0001

    static LRESULT  ReflectWM_COMMAND( HWND hwndCtl, UINT nCode, BOOL* pbHandled );
    static LRESULT  ReflectWM_NOTIFY( NMHDR* pHdr, BOOL* pbHandled );

protected:
    LRESULT OnSelfCommand( UINT nCode, BOOL& );
    LRESULT OnSelfNotify( NMHDR* pHdr, BOOL& );
    LRESULT OnGetDlgCode( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnKey( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnFocusChange( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnGetObject( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnPostNcDestroy( UINT, WPARAM, LPARAM, BOOL& );

    BEGIN_MSG_MAP( CInPlaceBase )
        MESSAGE_HANDLER( WM_GETDLGCODE, OnGetDlgCode );
        MESSAGE_HANDLER( WM_KEYUP, OnKey )
        MESSAGE_HANDLER( WM_KEYDOWN, OnKey )
        MESSAGE_HANDLER( WM_CHAR, OnKey )
        MESSAGE_HANDLER( WM_NCDESTROY, OnPostNcDestroy )
        MESSAGE_HANDLER( WMU_SELFCOMMAND, _selfCommand )
        MESSAGE_HANDLER( WMU_SELFNOTIFY,  _selfNotify )
        MESSAGE_HANDLER( WM_SETFOCUS,  OnFocusChange )
        MESSAGE_HANDLER( WM_KILLFOCUS, OnFocusChange )
        MESSAGE_HANDLER( WM_GETOBJECT, OnGetObject )
    END_MSG_MAP()

protected:
    void    PostNotifyCommand( UINT nNotifyCode );
    LRESULT SendNotifyCommand( UINT nNotifyCode );

    HWND     m_hwndTree;
    CTLTYPE  m_type;    

private:
    struct REFLECTNOTIFY   {
        BOOL*   pbHandled;
        NMHDR*  pHdr;
    };
    LRESULT _selfCommand( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& );
    LRESULT _selfNotify( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& );
    static  BOOL CALLBACK enumChildProc( HWND, LPARAM );
            BOOL          subclassCtl( HWND );
};

//-------------------------------------------------------------------------//
inline void CInPlaceBase::PostNotifyCommand( UINT nNotifyCode ) {
    ::PostMessage( GetParent(), WM_COMMAND, 
                   MAKEWPARAM( GetDlgCtrlID(), nNotifyCode ), (LPARAM)m_hWnd );
}
inline LRESULT CInPlaceBase::SendNotifyCommand( UINT nNotifyCode ) {
    return ::SendMessage( GetParent(), WM_COMMAND, 
                          MAKEWPARAM( GetDlgCtrlID(), nNotifyCode ), (LPARAM)m_hWnd );
}


//-------------------------------------------------------------------------//
//  In-place CBS_DROPLIST combo box class
class CInPlaceDropList : public CInPlaceBase
//-------------------------------------------------------------------------//
{
public:
    CInPlaceDropList( HWND hwndTree ) : CInPlaceBase( hwndTree ) {}
    virtual ~CInPlaceDropList() {}

    virtual BOOL    SubclassWindow( HWND, BOOL = FALSE );
    virtual LRESULT OnSelfCommand( UINT, BOOL& );
    virtual UINT    GetDropState() const;

protected:
    LRESULT OnKey( UINT, WPARAM, LPARAM, BOOL& );

    BEGIN_MSG_MAP( CInPlaceDropWindow )
        MESSAGE_HANDLER( WM_KEYDOWN, OnKey )
        MESSAGE_HANDLER( WM_KEYUP,   OnKey )
        MESSAGE_HANDLER( WM_CHAR,    OnKey )
        CHAIN_MSG_MAP( CInPlaceBase )
    END_MSG_MAP()
};


//-------------------------------------------------------------------------//
//  This window class is functions as an agent tracking mouse capture for a
//  client window which has lost capture to a child window.  
//  It accomplishes this by subclassing the child window that steals the capture, 
//  and then keeping the client informed about captured mouse activity. 
//
//  To enable tracking, the client should declare a CCaptureTracker data member, 
//  invoke this object's SubclassWindow() method in its WM_CAPTURECHANGED handler, 
//  and then respond to the notification messages sent by the object.
//  
//  Note: this class should never be Create()ed; it was intended strictly 
//  for subclassing existing windows.
//-------------------------------------------------------------------------//
class CCaptureTracker : public CWindowImpl<CCaptureTracker>
//-------------------------------------------------------------------------//
{
public:
    CCaptureTracker() : m_hwndClient(NULL) {}

    //  Initiates mouse capture tracking; called by client when it has 
    //  lost capture to a child window
    BOOL Track( HWND hwndChild, HWND hwndClient );

    //  registered message by which client is notified of captured mouse activity.
    static UINT NotifyMsg();
    
    //  WPARAM values for the notification message:
    enum {
        ForeignClick = 1,   //  LPARAM: HWND of stranger window which was clicked
        Lost,               //  LPARAM: HWND of stranger window gaining capture.
        Released,           //  LPARAM: always NULL
    };
    
protected:
    //  message handlers
    BEGIN_MSG_MAP( CCaptureTracker )
        MESSAGE_HANDLER( WM_LBUTTONDOWN,    OnSomeButtonDown )
        MESSAGE_HANDLER( WM_RBUTTONDOWN,    OnSomeButtonDown )
        MESSAGE_HANDLER( WM_MBUTTONDOWN,    OnSomeButtonDown )
        MESSAGE_HANDLER( WM_CAPTURECHANGED, OnCaptureChanged )
        MESSAGE_HANDLER( WM_DESTROY,        OnDestroy )
    END_MSG_MAP()

    LRESULT OnSomeButtonDown( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnCaptureChanged( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnDestroy       ( UINT, WPARAM, LPARAM, BOOL& );

    //  helper methods
    LRESULT NotifyClient( WPARAM nNotifyCode, HWND hwndDest = NULL );
    void    AsyncNotifyClient( WPARAM nNotifyCode, HWND hwndDest= NULL );

    //  data
    static UINT m_registeredMsg;
    HWND        m_hwndClient;
};

//-------------------------------------------------------------------------//
inline LRESULT CCaptureTracker::NotifyClient( WPARAM nNotifyCode, HWND hwndDest )  {
    return ::SendMessage( m_hwndClient, NotifyMsg(), nNotifyCode, (LPARAM)hwndDest );
}
inline void CCaptureTracker::AsyncNotifyClient( WPARAM nNotifyCode, HWND hwndDest )  {
    ::PostMessage( m_hwndClient, NotifyMsg(), nNotifyCode, (LPARAM)hwndDest );
}

//-------------------------------------------------------------------------//
//  Custom in-place drop-down class declaration
class CInPlaceDropWindow : public CInPlaceBase
//-------------------------------------------------------------------------//
{
//  Public instance methods
public:
    CInPlaceDropWindow( HWND hwndTree );
    virtual ~CInPlaceDropWindow() {}

    //  General attributes
    virtual HWND    DropHwnd() const =0;
            LPARAM  SetItemData( LPARAM lParam );
            LPARAM  GetItemData() const;
            void    SetDropHeight( int cy );
            BOOL    GetTextBox( LPRECT prc );
            BOOL    GetTextBox( int cx, int cy, LPRECT prc );
    virtual BOOL    IsMultiline() const { return FALSE; }

    //  Drop state attributes
    virtual UINT    GetDropState() const;
    UINT&           GetDropAnchor();

    //  Drop utility functions
    BOOL            IsChildOfDrop( HWND hwndChild ) const;
    BOOL            GetMonitorRect( OUT RECT& rcMonitor ) const;
    void            NormalizeDropRect( IN OUT RECT& rcDrop, OUT OPTIONAL UINT* pnDropAnchor = NULL ) const;
    BOOL            ShowDrop( BOOL bShow, BOOL bCancelled );
    BOOL            ToggleDrop( BOOL bCanceled );
    BOOL            AnimateDrop( HWND hwndDrop, ULONG dwTime = 200 );

//  Overrides and overideables
protected:
    virtual BOOL     SubclassWindow( HWND, BOOL = FALSE );
    virtual LRESULT  OnSelfCommand( UINT, BOOL& );
    virtual BOOL     OnShowDrop( HWND hwndDrop, BOOL bDrop, BOOL& bCanceled, LPRECT prcDrop );
    virtual BOOL     DrawTextBox() const { return TRUE; }
    virtual void     OnClick( HWND hwndChild, UINT uMsg, const POINT& pt, UINT nDropState );
            

//  Message handlers
protected:
    LRESULT OnCreate( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnDestroy( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnLButtonDown( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnCaptureChanged( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSize( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnPaint( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSetFocus( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnShowWindow( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnKey( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnKillFocus( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnWindowPosChanging( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnActivateApp( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSysKeyDown( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSetFont( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSetText( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnCommand( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnCaptureMsg( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnCBGetDropState( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnTimer( UINT, WPARAM, LPARAM, BOOL& );

    BEGIN_MSG_MAP( CInPlaceDropWindow )
        MESSAGE_HANDLER( WM_CREATE,             OnCreate )
        MESSAGE_HANDLER( WM_DESTROY,            OnDestroy )
        MESSAGE_HANDLER( WM_LBUTTONDOWN,        OnLButtonDown )
        MESSAGE_HANDLER( WM_CAPTURECHANGED,     OnCaptureChanged )
        MESSAGE_HANDLER( WM_SIZE,               OnSize )
        MESSAGE_HANDLER( WM_PAINT,              OnPaint )
        MESSAGE_HANDLER( WM_SETFOCUS,           OnSetFocus )
        MESSAGE_HANDLER( WM_KILLFOCUS,          OnKillFocus )
        MESSAGE_HANDLER( WM_WINDOWPOSCHANGING,  OnWindowPosChanging )
        MESSAGE_HANDLER( WM_ACTIVATEAPP,        OnActivateApp )
        MESSAGE_HANDLER( WM_SHOWWINDOW,         OnShowWindow )
        MESSAGE_HANDLER( WM_KEYDOWN,            OnKey )
        MESSAGE_HANDLER( WM_KEYUP,              OnKey )
        MESSAGE_HANDLER( WM_CHAR,               OnKey )
        MESSAGE_HANDLER( WM_SYSKEYDOWN,         OnSysKeyDown )
        MESSAGE_HANDLER( WM_SETFONT,            OnSetFont )
        MESSAGE_HANDLER( WM_SETTEXT,            OnSetText )
        MESSAGE_HANDLER( WM_COMMAND,            OnCommand )
        MESSAGE_HANDLER( WM_TIMER,              OnTimer )
        MESSAGE_HANDLER( CB_GETDROPPEDSTATE,    OnCBGetDropState )
        MESSAGE_HANDLER( CCaptureTracker::NotifyMsg(), OnCaptureMsg )
        CHAIN_MSG_MAP( CInPlaceBase )
    END_MSG_MAP()

//  Instance data
protected:
    UINT    m_nDropState,
            m_nDropAnchor;
    HFONT   m_hfText;
    RECT    m_rcBtn;
    int     m_cyDrop,
            m_cy;
    BOOL    m_fBtnPressed,
            m_bExtendedUI,
            m_fDestroyed;
    LPARAM  m_lParam;

//  Private implementation
private:
    void    AdjustSize();
    void    PositionControls( int cx, int cy );
    void    CalcHeight( LPCTSTR pszWindowText );
    BOOL    Capture( BOOL bCapture = TRUE );
    void    ClickButton();

    CCaptureTracker m_wndCaptureAgent;
};
inline BOOL CInPlaceDropWindow::SubclassWindow( HWND, BOOL ) { 
    ASSERT( FALSE ); // this class does not support window subclassing.
    return TRUE; 
}
inline BOOL CInPlaceDropWindow::OnShowDrop( HWND, BOOL, BOOL&, LPRECT ) {
    return TRUE;
}
inline LPARAM CInPlaceDropWindow::SetItemData( LPARAM lParam )    { 
    LPARAM l = m_lParam; 
    m_lParam = lParam; 
    return l;
}
inline LPARAM CInPlaceDropWindow::GetItemData() const  { 
    return m_lParam;
}
inline void CInPlaceDropWindow::SetDropHeight( int cy )    {
    ASSERT( cy>=0 );
    m_cyDrop = cy;
}
inline UINT CInPlaceDropWindow::GetDropState() const   { 
    return m_nDropState;
}
inline UINT& CInPlaceDropWindow::GetDropAnchor()    {
    return m_nDropAnchor;
}
inline BOOL CInPlaceDropWindow::IsChildOfDrop( HWND hwndChild ) const {
    return ::IsChild( DropHwnd(), hwndChild );
}

//-------------------------------------------------------------------------//
class CCalendarDrop : public CDialogImpl<CCalendarDrop>
//-------------------------------------------------------------------------//
{
public:
    CCalendarDrop(); 
    
    enum { IDD = IDD_INPLACE_CALENDAR };
    
    void SetOwner( CInPlaceDropCalendar* pOwner );
    HWND Create( HWND hwndParent, LPPOINT ptMouseActivate );
    HWND CalHwnd()     { return m_hwndCal; }
    BOOL SetDate( IN SYSTEMTIME&  st );
    BOOL GetDate( OUT SYSTEMTIME& st );
    BOOL Update();
    BOOL GetClientSize( OUT LPSIZE pSize );

protected:
    LRESULT OnCalSelChange( int, LPNMHDR, BOOL& );
    LRESULT OnCalSelect( int, LPNMHDR, BOOL& );
    
    BEGIN_MSG_MAP( CCalendarDrop )
        NOTIFY_HANDLER( IDC_CALENDAR, MCN_SELCHANGE, OnCalSelChange ) 
        NOTIFY_HANDLER( IDC_CALENDAR, MCN_SELECT, OnCalSelect ) 
    END_MSG_MAP()

    CInPlaceDropCalendar*   m_pOwner;
    HWND                    m_hwndCal;
    SYSTEMTIME              m_st;
};
//-------------------------------------------------------------------------//
inline CCalendarDrop::CCalendarDrop() 
    :   m_pOwner( NULL ), 
        m_hwndCal( NULL ) { 
    memset( &m_st, 0, sizeof(m_st) );
}
inline void CCalendarDrop::SetOwner( CInPlaceDropCalendar* pOwner ) {
    ASSERT( pOwner );
    m_pOwner = pOwner;
}

//-------------------------------------------------------------------------//
const UINT  DATEPICK_MSGMAP  = 1,
            IDC_DATEPICK = 0x00FF;   // arbitrary

//-------------------------------------------------------------------------//
//  Specialization of CInPlaceDropWindow to manipulate a CCalendarDrop 
//  dropdown host window.
class CInPlaceDropCalendar : public CInPlaceDropWindow
//-------------------------------------------------------------------------//
{
public:
    CInPlaceDropCalendar( HWND );
    virtual BOOL OnShowDrop( HWND, BOOL, BOOL&, LPRECT );

public:
    //  Utility helpers
    static void AssignCalendarDate( IN SYSTEMTIME& stSrc, OUT SYSTEMTIME& stDest );
           void UpdateDisplayDate( IN SYSTEMTIME& stSrc, IN ULONG dwFlags );

    #define UDDF_WINDOWTEXT 0x00000001
    #define UDDF_PICKERDATE 0x00000002

protected:
    void    PositionControls( int cx, int cy );
    LRESULT HandleKeyMessage( HWND hwndFrom, UINT, WPARAM, LPARAM, BOOL& );
    BOOL    IsDirectionalKey( WPARAM ) const;
    BOOL    IsInputKey( WPARAM ) const;
    BOOL    GetPickDate( OUT SYSTEMTIME& stDest );
    BOOL    SetPickDate( IN SYSTEMTIME& stSrc );

protected:
    //  overrides
    virtual HWND DropHwnd() const    { return SAFE_HWND( m_wndDrop.m_hWnd ); }
    virtual BOOL DrawTextBox() const { return FALSE; }

    //  Message handlers
    LRESULT OnCreate( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSize( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSetItemData( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnKey( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSetFocus( UINT, WPARAM, LPARAM, BOOL& );

    LRESULT OnPickerGetDlgCode( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnPickerKey( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnPickerSysKeyDown( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnPickerShowWindow( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnPickerNotify( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );

    BEGIN_MSG_MAP( CInPlaceDropCalendar )
        MESSAGE_HANDLER( WM_CREATE,         OnCreate )
        MESSAGE_HANDLER( WM_SIZE,           OnSize )
        MESSAGE_HANDLER( WM_SETFOCUS,       OnSetFocus )
        MESSAGE_HANDLER( WM_KEYDOWN,        OnKey )
        MESSAGE_HANDLER( WM_KEYUP,          OnKey )
        MESSAGE_HANDLER( WM_CHAR,           OnKey )
        MESSAGE_HANDLER( WMU_SETITEMDATA,   OnSetItemData )
        NOTIFY_ID_HANDLER( IDC_DATEPICK,  OnPickerNotify )
        CHAIN_MSG_MAP( CInPlaceDropWindow )
    ALT_MSG_MAP( DATEPICK_MSGMAP )
        MESSAGE_HANDLER( WM_CHAR,        OnPickerKey )
        MESSAGE_HANDLER( WM_KEYDOWN,     OnPickerKey )
        MESSAGE_HANDLER( WM_KEYUP,       OnPickerKey )
        MESSAGE_HANDLER( WM_SYSKEYDOWN,  OnPickerSysKeyDown )
        MESSAGE_HANDLER( WM_GETDLGCODE,  OnPickerGetDlgCode )
        MESSAGE_HANDLER( WM_SHOWWINDOW,  OnPickerShowWindow )
    END_MSG_MAP()

    CCalendarDrop       m_wndDrop;
    CContainedWindow    m_wndPick;
    SYSTEMTIME          m_st;
};

//-------------------------------------------------------------------------//
class CEditDrop : public CDialogImpl<CEditDrop>
//-------------------------------------------------------------------------//
{
public:
    CEditDrop();
    ~CEditDrop();

    enum { IDD = IDD_MULTILINE_EDIT };

    void SetOwner( CInPlaceDropEdit* pOwner );
    void QueueCharMsg( UINT nMsg, WPARAM wParam, LPARAM lParam );

protected:
    void EnforceMinWidth( LPRECT prcThis );
    void PositionControls( int cx, int cy );
    void TextFromOwner();
    BOOL TextToOwner();
    void EndDialogOK();
    void EndDialogCancel();
    void CommonEndDialog( int nResult );

    
    LRESULT OnInitDlg( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSize   ( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnNcHitTest( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnActivateApp( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnWindowPosChanging( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSetCapture( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnCaptureChanged( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnCaptureMsg( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnMouseMsg( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnEditChange( WORD, WORD, HWND, BOOL& );
    LRESULT OnCancel ( WORD, WORD, HWND, BOOL& );
    LRESULT OnOk     ( WORD, WORD, HWND, BOOL& );
    LRESULT OnDestroy( UINT, WPARAM, LPARAM, BOOL& );

    BEGIN_MSG_MAP( CEditDrop )
        MESSAGE_HANDLER( WM_INITDIALOG,          OnInitDlg )
        MESSAGE_HANDLER( WM_SIZE,                OnSize )
        MESSAGE_HANDLER( WM_NCHITTEST,           OnNcHitTest )
        MESSAGE_HANDLER( WM_ACTIVATEAPP,         OnActivateApp ) 
        MESSAGE_HANDLER( WM_WINDOWPOSCHANGING,   OnWindowPosChanging )
        MESSAGE_HANDLER( WMU_SETCAPTURE,         OnSetCapture )
        MESSAGE_HANDLER( WM_CAPTURECHANGED,      OnCaptureChanged )
        MESSAGE_RANGE_HANDLER( WM_MOUSEFIRST, WM_MOUSELAST, OnMouseMsg )
        MESSAGE_HANDLER( WM_DESTROY,             OnDestroy )
        COMMAND_HANDLER( IDC_EDIT, EN_CHANGE,    OnEditChange )
        COMMAND_ID_HANDLER( IDCANCEL,       OnCancel );
        COMMAND_ID_HANDLER( IDOK,           OnOk );
        MESSAGE_HANDLER( CCaptureTracker::NotifyMsg(), OnCaptureMsg )
    END_MSG_MAP()

    CInPlaceDropEdit*   m_pOwner;
    LPTSTR              m_pszUndo;
    MSG                 m_msgChar; // queued WM_CHAR/WM_DEADCHAR message
    CCaptureTracker     m_captureTrack;
    BOOL                m_fEndDlg;

};

//-------------------------------------------------------------------------//
//  Specialization of CInPlaceDropWindow to manipulate a CEditDrop
//  dropdown host window.
class CInPlaceDropEdit : public CInPlaceDropWindow
//-------------------------------------------------------------------------//
{
public:
    CInPlaceDropEdit( HWND );
    virtual BOOL IsMultiline() const { return TRUE; }

protected:
    virtual BOOL OnShowDrop( HWND, BOOL, BOOL&, LPRECT );
    virtual HWND DropHwnd() const  { return SAFE_HWND( m_wndDrop.m_hWnd ); }

    LRESULT OnChar( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnImeComposition( UINT, WPARAM, LPARAM, BOOL& );

    BEGIN_MSG_MAP( CInPlaceDropEdit )
        MESSAGE_HANDLER( WM_IME_COMPOSITION, OnImeComposition )
        MESSAGE_HANDLER( WM_SETTEXT, OnSetText )
        MESSAGE_HANDLER( WM_CHAR, OnChar )
        MESSAGE_HANDLER( WM_DEADCHAR, OnChar )
        CHAIN_MSG_MAP( CInPlaceDropWindow )
    END_MSG_MAP()
    
    CEditDrop  m_wndDrop;
};

//-------------------------------------------------------------------------//
inline CEditDrop::~CEditDrop()     { 
    if( m_pszUndo ) delete [] m_pszUndo; 
}
inline void CEditDrop::SetOwner( CInPlaceDropEdit* pOwner )   { 
    ASSERT( pOwner );
    m_pOwner = pOwner; 
}
inline void CEditDrop::EndDialogOK()    {
    CommonEndDialog( TextToOwner() ? IDOK : IDCANCEL );
}
inline void CEditDrop::EndDialogCancel()    {
    m_pOwner->SetWindowText( m_pszUndo );
    CommonEndDialog( IDCANCEL );
}

//-------------------------------------------------------------------------//
//  Some Combobox helper macros from windowsx.h (including this header
//  breaks build; duplicate definitions vs ATL).
#define ComboBox_LimitText(hwnd, cchLimit)          ((int)(DWORD)::SendMessage((hwnd), CB_LIMITTEXT, (WPARAM)(int)(cchLimit), 0L))
#define ComboBox_GetEditSel(hwnd)                   ((DWORD)::SendMessage((hwnd), CB_GETEDITSEL, 0L, 0L))
#define ComboBox_SetEditSel(hwnd, ichStart, ichEnd) ((int)(DWORD)::SendMessage((hwnd), CB_SETEDITSEL, 0L, MAKELPARAM((ichStart), (ichEnd))))
#define ComboBox_GetCount(hwnd)                     ((int)(DWORD)::SendMessage((hwnd), CB_GETCOUNT, 0L, 0L))
#define ComboBox_ResetContent(hwnd)                 ((int)(DWORD)::SendMessage((hwnd), CB_RESETCONTENT, 0L, 0L))
#define ComboBox_AddString(hwnd, lpsz)              ((int)(DWORD)::SendMessage((hwnd), CB_ADDSTRING, 0L, (LPARAM)(LPCTSTR)(lpsz)))
#define ComboBox_InsertString(hwnd, index, lpsz)    ((int)(DWORD)::SendMessage((hwnd), CB_INSERTSTRING, (WPARAM)(int)(index), (LPARAM)(LPCTSTR)(lpsz)))
#define ComboBox_AddItemData(hwnd, data)            ((int)(DWORD)::SendMessage((hwnd), CB_ADDSTRING, 0L, (LPARAM)(data)))
#define ComboBox_InsertItemData(hwnd, index, data)  ((int)(DWORD)::SendMessage((hwnd), CB_INSERTSTRING, (WPARAM)(int)(index), (LPARAM)(data)))
#define ComboBox_DeleteString(hwnd, index)          ((int)(DWORD)::SendMessage((hwnd), CB_DELETESTRING, (WPARAM)(int)(index), 0L))
#define ComboBox_GetLBTextLen(hwnd, index)          ((int)(DWORD)::SendMessage((hwnd), CB_GETLBTEXTLEN, (WPARAM)(int)(index), 0L))
#define ComboBox_GetLBText(hwnd, index, lpszBuffer) ((int)(DWORD)::SendMessage((hwnd), CB_GETLBTEXT, (WPARAM)(int)(index), (LPARAM)(LPCTSTR)(lpszBuffer)))
#define ComboBox_GetItemData(hwnd, index)           ((LRESULT)(DWORD)::SendMessage((hwnd), CB_GETITEMDATA, (WPARAM)(int)(index), 0L))
#define ComboBox_SetItemData(hwnd, index, data)     ((int)(DWORD)::SendMessage((hwnd), CB_SETITEMDATA, (WPARAM)(int)(index), (LPARAM)(data)))
#define ComboBox_FindString(hwnd, iStart, lpszFind) ((int)(DWORD)::SendMessage((hwnd), CB_FINDSTRING, (WPARAM)(int)(iStart), (LPARAM)(LPCTSTR)(lpszFind)))
#define ComboBox_FindItemData(hwnd, iStart, data)   ((int)(DWORD)::SendMessage((hwnd), CB_FINDSTRING, (WPARAM)(int)(iStart), (LPARAM)(data)))
#define ComboBox_GetCurSel(hwnd)                    ((int)(DWORD)::SendMessage((hwnd), CB_GETCURSEL, 0L, 0L))
#define ComboBox_SetCurSel(hwnd, index)             ((int)(DWORD)::SendMessage((hwnd), CB_SETCURSEL, (WPARAM)(int)(index), 0L))
#define ComboBox_SelectString(hwnd, iStart, lpsz)   ((int)(DWORD)::SendMessage((hwnd), CB_SELECTSTRING, (WPARAM)(int)(iStart), (LPARAM)(LPCTSTR)(lpsz)))
#define ComboBox_SelectItemData(hwnd, iStart, data) ((int)(DWORD)::SendMessage((hwnd), CB_SELECTSTRING, (WPARAM)(int)(iStart), (LPARAM)(data)))
#define ComboBox_Dir(hwnd, attrs, lpszFileSpec)     ((int)(DWORD)::SendMessage((hwnd), CB_DIR, (WPARAM)(UINT)(attrs), (LPARAM)(LPCTSTR)(lpszFileSpec)))
#define ComboBox_ShowDropdown(hwnd, fShow)          ((BOOL)(DWORD)::SendMessage((hwnd), CB_SHOWDROPDOWN, (WPARAM)(BOOL)(fShow), 0L))
#define ComboBox_FindStringExact(hwnd,iStart,lpsz)  ((int)(DWORD)::SendMessage((hwnd), CB_FINDSTRINGEXACT, (WPARAM)(int)(iStart), (LPARAM)(LPCTSTR)(lpsz)))
#define ComboBox_GetDroppedState(hwnd)              ((BOOL)(DWORD)::SendMessage((hwnd), CB_GETDROPPEDSTATE, 0L, 0L))
#define ComboBox_GetDroppedControlRect(hwnd, lprc)  ((void)::SendMessage((hwnd), CB_GETDROPPEDCONTROLRECT, 0L, (LPARAM)(RECT *)(lprc)))
#define ComboBox_GetItemHeight(hwnd)                ((int)(DWORD)::SendMessage((hwnd), CB_GETITEMHEIGHT, 0L, 0L))
#define ComboBox_SetItemHeight(hwnd, index, cyItem) ((int)(DWORD)::SendMessage((hwnd), CB_SETITEMHEIGHT, (WPARAM)(int)(index), (LPARAM)(int)cyItem))
#define ComboBox_GetExtendedUI(hwnd)                ((UINT)(DWORD)::SendMessage((hwnd), CB_GETEXTENDEDUI, 0L, 0L))
#define ComboBox_SetExtendedUI(hwnd, flags)         ((int)(DWORD)::SendMessage((hwnd), CB_SETEXTENDEDUI, (WPARAM)(UINT)(flags), 0L))


#endif __EDITCTL_H__
