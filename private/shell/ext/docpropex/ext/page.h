//-------------------------------------------------------------------------//
// page.h : property page decls
//-------------------------------------------------------------------------//

#ifndef __BASICPROPPAGE_H_
#define __BASICPROPPAGE_H_

//  Forwards
#include "resource.h"       // resource symbols

class       CShellExt ;
class       CAdvDlg ;
interface   IPropertyTreeCtl ;

//-------------------------------------------------------------------------//
//  constants
#define CCH_PROPERTYTEXT_MAX    MAX_PATH

//-------------------------------------------------------------------------//
//  private messages exchanged between page and 'advanced properties' dlg.
#define WMU_ADVDLG_BASE    (WM_USER + 0x400)   // arbitrary
#define WMU_ADVDLG_OK      (WMU_ADVDLG_BASE + 1)
#define WMU_ADVDLG_CANCEL  (WMU_ADVDLG_BASE + 2)
#define WMU_ADVDLG_ABORT   (WMU_ADVDLG_BASE + 3)
#define WMU_ADVDLG_HELP    (WMU_ADVDLG_BASE + 4)
#define WMU_ADVDLG_LAST    WMU_ADVDLG_HELP
            
//  helper macs
#define DECLARE_WM_HANDLER( handler ) \
    LRESULT handler( UINT, WPARAM, LPARAM, BOOL& )
#define DECLARE_CM_HANDLER( handler ) \
    LRESULT handler( WORD, WORD, HWND, BOOL& ) ;
#define DECLARE_NM_HANDLER( handler ) \
    LRESULT handler( int, LPNMHDR, BOOL& );

//-------------------------------------------------------------------------//
// CPropEditCtl - property value edit control.
class CPropEditCtl : public CWindowImpl<CPropEditCtl>
{
public:
    CPropEditCtl() ;
    BOOL SubclassWindow( HWND hwnd, UINT nIDLabel ) ;
    BOOL ShowWindow( UINT nShowCmd ) ;
    void SetCompositeMismatch( BOOL fCompositeMismatch, BOOL bRedraw = FALSE ) ;

protected:
    DECLARE_WM_HANDLER( OnKeyDown ) ;
    DECLARE_WM_HANDLER( OnChar ) ;
    DECLARE_WM_HANDLER( OnPaint ) ;
    DECLARE_WM_HANDLER( OnLButton ) ;
    DECLARE_WM_HANDLER( OnMouseMove ) ;
    DECLARE_WM_HANDLER( OnSetReadOnly ) ;

    BEGIN_MSG_MAP( CPropEditCtl() )
        MESSAGE_HANDLER( EM_SETREADONLY,   OnSetReadOnly )
        MESSAGE_HANDLER( WM_KEYDOWN,       OnKeyDown )
        MESSAGE_HANDLER( WM_CHAR,          OnChar )
        MESSAGE_HANDLER( WM_LBUTTONDOWN,   OnLButton )
        MESSAGE_HANDLER( WM_LBUTTONDBLCLK, OnLButton )
        MESSAGE_HANDLER( WM_MOUSEMOVE,     OnMouseMove )
        MESSAGE_HANDLER( WM_PAINT,         OnPaint )
    END_MSG_MAP()

    void PaintCompositeMismatch( HDC hdc ) ;
    
    BOOL         m_bReadOnly,
                 m_fCompositeMismatch ;
    static TCHAR m_szCompositeMismatch[128] ;
    UINT         m_nIDLabel ;
} ;

//-------------------------------------------------------------------------//
//  Hack to generate an array of child window ctlID - topic ID pairs on the 
//  fly for the property tree control.   We should create a method on the 
//  control, DoContextHelp(pszHelpFile, dwFlags) that takes care of this, 
//  but for now I don't want to tie a monolithic topic model to the control.
#define MAX_PROPTREE_CHILDREN   (18)
typedef struct {
    int     iTopic;
    DWORD   rgTopics[MAX_PROPTREE_CHILDREN];
} PROPTREE_HELPTOPICS;

//-------------------------------------------------------------------------//
// CPage0 - Standard summaryinfo properties page
class CPage0 : 
    public CDialogImpl<CPage0>
//-------------------------------------------------------------------------//
{
public:
    CPage0( ) ;
    ~CPage0( ) ;

    HRESULT  Add( CShellExt* pExt, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam ) ;
    BOOL     IsDirty() const ;
    BOOL     IsDirty( BASICPROPERTY nProp ) const ;

//  Message handlers
    DECLARE_WM_HANDLER( OnDestroy ) ;
    DECLARE_WM_HANDLER( OnInitDialog ) ;
    DECLARE_WM_HANDLER( OnSize ) ;
    DECLARE_WM_HANDLER( OnHelp ) ;
    DECLARE_WM_HANDLER( OnWinIniChange ) ;
    DECLARE_WM_HANDLER( OnCtlColorEdit ) ;
    DECLARE_WM_HANDLER( OnContextMenu ) ;

    DECLARE_NM_HANDLER( OnApply ) ;
    DECLARE_NM_HANDLER( OnCtlEvent ) ;
    DECLARE_CM_HANDLER( OnEditChange ) ;
    DECLARE_CM_HANDLER( OnEditSetFocus ) ;
    DECLARE_CM_HANDLER( OnEditKillFocus ) ;
    DECLARE_CM_HANDLER( OnAdvanced ) ;
    DECLARE_CM_HANDLER( OnBasic ) ;

//  ATL chit
    enum { IDD = IDD_PROPTREEPAGE };

    BEGIN_MSG_MAP(CPage0)
        MESSAGE_HANDLER( WM_DESTROY,        OnDestroy )
        MESSAGE_HANDLER( WM_INITDIALOG,     OnInitDialog )
        MESSAGE_HANDLER( WM_SIZE,           OnSize )
        MESSAGE_HANDLER( WM_HELP,           OnHelp )
        MESSAGE_HANDLER( WM_WININICHANGE,   OnWinIniChange )
        MESSAGE_HANDLER( WM_CTLCOLOREDIT,   OnCtlColorEdit )
        MESSAGE_HANDLER( WM_CONTEXTMENU,   OnContextMenu )
        NOTIFY_HANDLER( IDC_CTL, OCN_OCEVENT, OnCtlEvent )
        NOTIFY_CODE_HANDLER( PSN_APPLY,      OnApply )
        COMMAND_ID_HANDLER( IDC_ADVANCED,    OnAdvanced )
        COMMAND_ID_HANDLER( IDC_SIMPLE,     OnBasic )
        COMMAND_CODE_HANDLER( EN_CHANGE,    OnEditChange ) ;
        COMMAND_CODE_HANDLER( EN_SETFOCUS,  OnEditSetFocus ) ;
        COMMAND_CODE_HANDLER( EN_KILLFOCUS, OnEditKillFocus ) ;
    END_MSG_MAP()

//  Implementation
public:
    enum UIMODE   {
        UIMODE_NONE,
        UIMODE_ADVANCED,
        UIMODE_BASIC,
    } ;
    #define UIMODE_EITHER   UIMODE_NONE

protected:
    void    Attach( CShellExt* pExt ) ;
    void    Detach() ;
    void    InitCtlDisplay( UINT nIDC, IN OPTIONAL BASICPROPERTY* pnProp = NULL ) ;
    void    PositionControls( UIMODE mode, int, int ) ;
    void    UpdateControls() ;
    HRESULT UpdateAdvancedValue( ULONG nProp, BOOL bUpdateTree ) ;
    void    UpdateAdvancedValues( BOOL bTreeValues ) ;
    void    SetBasicPropertyText( BASICPROPERTY nProp, LPCTSTR pszText );
    void    CoInit( BOOL bInit ) ;
    HBRUSH  CreateCompositeMismatchBrush( BOOL bRecreate = FALSE ) ;
    void    OnChange( DWORD dwPropDirty ) ;

    int     DisplayError( ERROR_CONTEXT errctx, HRESULT hr )    {
                return m_pExt->DisplayError( m_hWnd, IDS_PAGE0_MSGCAPTION, errctx, hr );
            }

    HRESULT CreateAdvancedPropertyUI();
    void    Toggle( BOOL fAdvanced ) ;
    void    SetAdvancedMode( BOOL fAdvanced ) ;
    void    SetBasicMode( BOOL fBasic ) ;

    void    PersistMode() ;
    BOOL    RecallMode() ;
    
    static UINT CALLBACK PageCallback( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp ) ;

//  Data:
protected:
    CShellExt*          m_pExt ;
    IPropertyTreeCtl*   m_pIPropTree ;
    HWND                m_hwndSite,
                        m_hwndTree ;
    CPropEditCtl        m_edit[BASICPROP_COUNT] ;
    BOOL                m_fInitializing,
                        m_fAdvanced ;
    int                 m_cWinHelp ;
    ULONG               m_cAdvDirty ;
    PROPSHEETPAGE       m_psp ;
    HPROPSHEETPAGE      m_hPage;
    CAdvDlg*            m_pAdvDlg ;
    HBRUSH              m_hbrComposite ;
    UIMODE              m_nSelectedMode ; // UI mode explicitly selected by user.
    PROPTREE_HELPTOPICS m_iht;
};

//-------------------------------------------------------------------------//
inline BOOL CPage0::IsDirty() const {
    return m_cAdvDirty > 0 || 
           ((NULL != m_pExt) ? m_pExt->IsDirty() : FALSE) ;
}
inline BOOL CPage0::IsDirty( BASICPROPERTY nProp ) const {
    return m_pExt ? m_pExt->IsDirty( nProp ) : FALSE ;
}

#endif //__BASICPROPPAGE_H_
