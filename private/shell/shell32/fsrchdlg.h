// fsrchdlg.h : Declaration of the file search band dialog classes

#ifndef __FSEARCH_DLGS_H__
#define __FSEARCH_DLGS_H__

#include "resource.h"       // main symbols
#include <shdispid.h>
#include "atldisp.h"
#include "ids.h"
#include "unicpp\sdspatch.h"

//--------------------//
//  Forwards
class CFileSearchBand;     // top-level (band/OC) window

class CBandDlg;            // base class for top-level dialogs
    class CSearchCmdDlg;       // base class for band dlgs that employ an ISearchCmdExt interface
        class CFindFilesDlg;       // top level file system search dialog
        class CFindComputersDlg;   // top level net search dialog
    class CFindPrintersDlg;    // top level printer search dialog

class CSubDlg;             // base class for subordinate dialogs
    class CDateDlg;            // file date subordinate dialog
    class CTypeDlg;            // file type subordinate dialog
    class CSizeDlg;            // file size subordinate dialog
    class CAdvancedDlg;        // advanced options subordinate dialog

interface IStringMru;

//-----------------------------------------//
//  private messages posted to subdialogs
#define WMU_COMBOPOPULATIONCOMPLETE     (WM_USER+0x200) // (wParam: HWND of combo).
#define WMU_STATECHANGE                 (WM_USER+0x201) // 
#define WMU_UPDATELAYOUT                (WM_USER+0x202) // (wParam: Band layout flags (BLF_)).
#define WMU_RESTORESEARCH               (WM_USER+0x203) // (wParam: n/a, lParam: n/a, ret: n/a).
#define WMU_BANDINFOUPDATE              (WM_USER+0x204) // (wParam: NULL).
#define WMU_NAMESPACERECONCILE          (WM_USER+0x205) // (wParam: n/a, LPARAM: n/a).

//  ISearchCommandExt::SearchFor parameters
#define SCE_SEARCHFORFILES              0
#define SCE_SEARCHFORCOMPUTERS          1

//-------------------------------------------------------------------------//
//  async state data
typedef struct tagFSEARCHTHREADSTATE 
{
    HWND    hwndCtl;
    int     cItems;
    void*   pvParam;
    ULONG   Reserved;
    BOOL    fComplete;
    BOOL    fCancel;

    //  constructor:
    tagFSEARCHTHREADSTATE() 
        :   hwndCtl(NULL), 
            cItems(0), 
            pvParam(NULL),
            Reserved(0),
            fComplete(FALSE),
            fCancel(FALSE)
            {}

} FSEARCHTHREADSTATE, *PFSEARCHTHREADSTATE;

//-------------------------------------------------------------------------//
class CSubDlg // base class for subordinate dialogs
//-------------------------------------------------------------------------//
{
public:
    CSubDlg( CFileSearchBand* pfsb ) : _pfsb(pfsb), _hwnd(NULL), _pBandDlg(NULL) {}
    virtual ~CSubDlg()  {}

    HWND      Hwnd() const                       { return _hwnd; }
    void      SetBandDlg( CBandDlg* pBandDlg )   { _pBandDlg = pBandDlg; }
    CBandDlg* BandDlg() const                    { return _pBandDlg; }

    STDMETHOD (AddConstraints)( ISearchCommandExt* pSrchCmd ) PURE;
    STDMETHOD (RestoreConstraint)( const BSTR bstrName, const VARIANT* pValue ) PURE;
    STDMETHOD (TranslateAccelerator)( LPMSG lpmsg );
    virtual int  GetIdealDeskbandWidth() const { return -1;}
    virtual BOOL Validate()     { return TRUE; }
    virtual void Clear() PURE;
    virtual void LoadSaveUIState( UINT nIDCtl, BOOL bSave ) {}
    virtual void OnWinIniChange()   {}

protected:
    BEGIN_MSG_MAP( CSubDlg)
        MESSAGE_HANDLER(WM_NCCALCSIZE, OnNcCalcsize)
        MESSAGE_HANDLER(WM_NCPAINT, OnNcPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColor)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        COMMAND_CODE_HANDLER(BN_SETFOCUS, OnChildSetFocusCmd)
        COMMAND_CODE_HANDLER(EN_SETFOCUS, OnChildSetFocusCmd)
        COMMAND_CODE_HANDLER(CBN_SETFOCUS, OnChildSetFocusCmd)
        NOTIFY_CODE_HANDLER(NM_SETFOCUS, OnChildSetFocusNotify ) 
        COMMAND_CODE_HANDLER(BN_KILLFOCUS,  OnChildKillFocusCmd)
        COMMAND_CODE_HANDLER(EN_KILLFOCUS,  OnChildKillFocusCmd)
        COMMAND_CODE_HANDLER(CBN_KILLFOCUS, OnChildKillFocusCmd)
        NOTIFY_CODE_HANDLER(NM_KILLFOCUS,   OnChildKillFocusNotify ) 
        NOTIFY_CODE_HANDLER(CBEN_ENDEDIT,   OnComboExEndEdit ) 
    END_MSG_MAP()

    LRESULT OnNcCalcsize( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnNcPaint( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnEraseBkgnd( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnPaint( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnCtlColor( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSize( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnChildSetFocusCmd( WORD, WORD, HWND, BOOL& ); 
    LRESULT OnChildSetFocusNotify( int, LPNMHDR, BOOL&);
    LRESULT OnChildKillFocusCmd( WORD, WORD, HWND, BOOL&);
    LRESULT OnChildKillFocusNotify( int, LPNMHDR, BOOL&);
    LRESULT OnComboExEndEdit( int, LPNMHDR, BOOL&);

    void _Attach( HWND hwnd )    { _hwnd = hwnd; }
    CFileSearchBand* _pfsb;
    CBandDlg*        _pBandDlg;

private:
    HWND _hwnd;
};

//-------------------------------------------------------------------------//
// CDateDlg - file date subordinate dialog
class CDateDlg : public CDialogImpl<CDateDlg>,
                 public CSubDlg
//-------------------------------------------------------------------------//
{
public:
    CDateDlg( CFileSearchBand* pfsb )
        :   CSubDlg( pfsb ) {}
    ~CDateDlg() {}

    enum { IDD = DLG_FSEARCH_DATE };

    STDMETHOD (AddConstraints)( ISearchCommandExt* pSrchCmd );
    STDMETHOD (RestoreConstraint)( const BSTR bstrName, const VARIANT* pValue );
    virtual BOOL Validate();
    virtual void Clear();

protected:
    BEGIN_MSG_MAP( CDateDlg )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE,       OnSize)
        NOTIFY_CODE_HANDLER( UDN_DELTAPOS, OnMonthDaySpin ) 
        COMMAND_CODE_HANDLER(BN_CLICKED, OnBtnClick)
        COMMAND_HANDLER( IDC_RECENT_MONTHS, EN_KILLFOCUS, OnMonthsKillFocus )
        COMMAND_HANDLER( IDC_RECENT_DAYS,   EN_KILLFOCUS, OnDaysKillFocus )
    CHAIN_MSG_MAP( CSubDlg )
    END_MSG_MAP()

    //  message handlers
    LRESULT OnInitDialog( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnBtnClick(WORD,WORD,HWND,BOOL&);
    LRESULT OnMonthsKillFocus(WORD,WORD,HWND,BOOL&);
    LRESULT OnDaysKillFocus(WORD,WORD,HWND,BOOL&);
    LRESULT OnMonthDaySpin(int, LPNMHDR, BOOL&);

    //  utility methods
    void    EnableControls();
};

//-------------------------------------------------------------------------//
// CSizeDlg   - file size subordinate dialog
class CSizeDlg : public CDialogImpl<CSizeDlg>,
                 public CSubDlg
//-------------------------------------------------------------------------//
{
public:
    CSizeDlg( CFileSearchBand* pfsb )
        :   CSubDlg( pfsb ) {}
    ~CSizeDlg() {}

    enum { IDD = DLG_FSEARCH_SIZE };

    STDMETHOD (AddConstraints)( ISearchCommandExt* pSrchCmd );
    STDMETHOD (RestoreConstraint)( const BSTR bstrName, const VARIANT* pValue );
    virtual void Clear();

protected:
    BEGIN_MSG_MAP( CSizeDlg )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        NOTIFY_HANDLER( IDC_FILESIZE_SPIN, UDN_DELTAPOS, OnSizeSpin ) 
        COMMAND_HANDLER( IDC_FILESIZE, EN_KILLFOCUS, OnSizeKillFocus )
    CHAIN_MSG_MAP( CSubDlg )
    END_MSG_MAP()

    //  message handlers
    LRESULT OnInitDialog( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSizeSpin(int, LPNMHDR, BOOL&);
    LRESULT OnSizeKillFocus(WORD,WORD,HWND,BOOL&);
};

//-------------------------------------------------------------------------//
// CTypeDlg - file type subordinate dialog
class CTypeDlg : public CDialogImpl<CTypeDlg>,
                 public CSubDlg
//-------------------------------------------------------------------------//
{
public:
    CTypeDlg( CFileSearchBand* pfsb );
    ~CTypeDlg();

    enum { IDD = DLG_FSEARCH_TYPE };

    STDMETHOD (AddConstraints)( ISearchCommandExt* pSrchCmd );
    STDMETHOD (RestoreConstraint)( const BSTR bstrName, const VARIANT* pValue );
    virtual void Clear();
    virtual void OnWinIniChange();

protected:
    BEGIN_MSG_MAP( CTypeDlg )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE,       OnSize)
        MESSAGE_HANDLER(WMU_COMBOPOPULATIONCOMPLETE, OnComboPopulationComplete)
        MESSAGE_HANDLER(WM_DESTROY,    OnDestroy)
        NOTIFY_HANDLER( IDC_FILE_TYPE, CBEN_DELETEITEM, OnFileTypeDeleteItem) 
    CHAIN_MSG_MAP( CSubDlg )
    END_MSG_MAP()

    //  message handlers
    LRESULT OnInitDialog( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnFileTypeDeleteItem(int, LPNMHDR, BOOL&);
    LRESULT OnComboPopulationComplete( UINT, WPARAM, LPARAM, BOOL& );

    //  utility methods
    static STDMETHODIMP AddItemNotify( ULONG, PCBXITEM, LPARAM );
    static DWORD        FileAssocThreadProc( void* pvParam );
    static  INT_PTR _FindExtension( HWND hwndCombo, TCHAR* pszExt );

    //  data
    HANDLE              _hFileAssocThread;
    FSEARCHTHREADSTATE  _threadState;
    TCHAR               _szRestoredExt[MAX_PATH];
};

//-------------------------------------------------------------------------//
// CAdvancedDlg - advanced options subordinate dialog
class CAdvancedDlg : public CDialogImpl<CAdvancedDlg>,
                     public CSubDlg
//-------------------------------------------------------------------------//
{
public:
    CAdvancedDlg( CFileSearchBand* pfsb )
        :   CSubDlg( pfsb ) {}
    ~CAdvancedDlg() {}
    enum { IDD = DLG_FSEARCH_ADVANCED };

    STDMETHOD (AddConstraints)( ISearchCommandExt* pSrchCmd );
    STDMETHOD (RestoreConstraint)( const BSTR bstrName, const VARIANT* pValue );
    virtual void Clear();

protected:
    BEGIN_MSG_MAP( CAdvancedDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    CHAIN_MSG_MAP( CSubDlg )
    END_MSG_MAP()

    //  message handlers
    LRESULT OnInitDialog( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL& );
};

//-------------------------------------------------------------------------//
class COptionsDlg : public CDialogImpl<COptionsDlg>,
                    public CSubDlg
//-------------------------------------------------------------------------//
{
public:
    COptionsDlg( CFileSearchBand* pfsb );
    ~COptionsDlg() {}

    STDMETHOD (AddConstraints)( ISearchCommandExt* pSrchCmd );
    STDMETHOD (RestoreConstraint)( const BSTR bstrName, const VARIANT* pValue );
    STDMETHOD (TranslateAccelerator)( LPMSG lpmsg );
    virtual void LoadSaveUIState( UINT nIDCtl, BOOL bSave );
    virtual BOOL GetMinSize( LPSIZE pSize ) const;
    virtual BOOL Validate();
    virtual void Clear();
    virtual void OnWinIniChange();

    void UpdateSearchCmdStateUI( DISPID dispid = 0 );
    LONG QueryHeight( LONG cx, LONG cy );

    BOOL IsAdvancedOptionChecked( UINT nID )    {
        return _dlgAdvanced.IsDlgButtonChecked( nID ) ? TRUE : FALSE ;
    }
    void TakeFocus();

    enum { IDD = DLG_FSEARCH_OPTIONS };

protected:
    BEGIN_MSG_MAP( COptionsDlg )
        MESSAGE_HANDLER(WM_INITDIALOG,   OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE,         OnSize)
        COMMAND_CODE_HANDLER(BN_CLICKED, OnBtnClick)
        NOTIFY_HANDLER( IDC_INDEX_SERVER, NM_CLICK,  OnIndexServerClick )
        NOTIFY_HANDLER( IDC_INDEX_SERVER, NM_RETURN, OnIndexServerClick )
    CHAIN_MSG_MAP( CSubDlg )
    END_MSG_MAP()

    //  messsage handlers
    LRESULT OnInitDialog( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnBtnClick(WORD,WORD,HWND,BOOL&);
    LRESULT OnIndexServerClick( int, LPNMHDR, BOOL&);

    //  utility methods
    void LayoutControls( int cx = -1, int cy = -1 );
    void SizeToFit( BOOL bScrollBand = FALSE );

    //  data
private:
    //  private subdialog identifiers
    enum SUBDLGID
    {
        SUBDLG_DATE,
        SUBDLG_TYPE,
        SUBDLG_SIZE,
        SUBDLG_ADVANCED,

        SUBDLG_Count
    };
    
    //  subdialog instances
    CDateDlg          _dlgDate;
    CTypeDlg          _dlgType;
    CSizeDlg          _dlgSize;
    CAdvancedDlg      _dlgAdvanced;

    //  subdialog data definition block.
    typedef struct {
        UINT nIDCheck;
        CSubDlg* pDlg;
        SIZE sizeDlg;
        RECT rcCheck;
        RECT rcDlg;
    } _SUBDLG;
    _SUBDLG      _subdlgs[SUBDLG_Count];
    
    //  misc
    UINT _nCIStatusText; 
};

//-------------------------------------------------------------------------//
// CBandDlg - top level dialog base class
class CBandDlg
//-------------------------------------------------------------------------//
{
public:
    CBandDlg( CFileSearchBand* pfsb );
    ~CBandDlg();

    HWND Hwnd() const           { return _hwnd; }

    //  manditory overrideables
    virtual HWND Create( HWND hwndParent ) = 0;
    virtual UINT GetIconID() const = 0;
    virtual UINT GetCaptionID() const = 0;
    virtual UINT GetCaptionDivID() const = 0;

    STDMETHOD (TranslateAccelerator)( LPMSG lpmsg );
    virtual void RemoveToolbarTurds( int cyOffset );

    //  optional overrideables
    virtual void LayoutControls( int cx = -1, int cy = -1 );
    virtual BOOL Validate()     { return TRUE; }
    virtual void Clear() {};
    virtual BOOL GetMinSize( HWND hwndOC, LPSIZE pSize ) const;
    virtual BOOL GetIdealSize( HWND hwndOC, LPSIZE pSize ) const;
    virtual int  GetIdealDeskbandWidth() const { return -1; }
    virtual void SetDefaultFocus();
    virtual HWND GetFirstTabItem() const    { return NULL; }
    virtual HWND GetLastTabItem() const     { return NULL; }
    virtual BOOL GetAutoCompleteObjectForWindow( HWND hwnd, IAutoComplete2** ppac2 );
    virtual void NavigateToResults( IWebBrowser2* pwb2 ) {}
    virtual void LoadSaveUIState( UINT nIDCtl, BOOL bSave ) {}
    virtual HWND ShowHelp( HWND hwndOwner ) { return NULL; }
    virtual void OnWinIniChange()    {}
    virtual void WndPosChanging( HWND hwndOC, LPWINDOWPOS pwp );
    virtual void RememberFocus( HWND hwndFocus );
    virtual BOOL RestoreFocus();

    virtual void OnBandShow( BOOL bShow )   {}          //**band** show/hide handler
    virtual void OnBandDialogShow( BOOL bShow )   {}    //band **dialog** show/hide handler
    virtual HRESULT SetScope( IN VARIANT* pvarScope, BOOL bTrack = FALSE );
    virtual HRESULT GetScope( OUT VARIANT* pvarScope );
    virtual HRESULT SetQueryFile( IN VARIANT* pvarScope );
    virtual HRESULT GetQueryFile( OUT VARIANT* pvarScope );

    virtual BOOL    SearchInProgress() const { return FALSE; };
    virtual void    StopSearch() {};
protected:
    BEGIN_MSG_MAP( CBandDlg )
        MESSAGE_HANDLER( WM_SIZE, OnSize )
        MESSAGE_HANDLER( WM_PAINT, OnPaint )
        MESSAGE_HANDLER( WM_ERASEBKGND, OnEraseBkgnd )
        MESSAGE_HANDLER( WM_CTLCOLORSTATIC, OnCtlColorStatic )
        COMMAND_CODE_HANDLER(BN_SETFOCUS,   OnChildSetFocusCmd)
        COMMAND_CODE_HANDLER(EN_SETFOCUS,   OnChildSetFocusCmd)
        COMMAND_CODE_HANDLER(CBN_SETFOCUS,  OnChildSetFocusCmd)
        NOTIFY_CODE_HANDLER(NM_SETFOCUS,    OnChildSetFocusNotify ) 

        COMMAND_CODE_HANDLER(BN_KILLFOCUS,  OnChildKillFocusCmd)
        COMMAND_CODE_HANDLER(EN_KILLFOCUS,  OnChildKillFocusCmd)
        COMMAND_CODE_HANDLER(CBN_KILLFOCUS, OnChildKillFocusCmd)
        NOTIFY_CODE_HANDLER(NM_KILLFOCUS,   OnChildKillFocusNotify ) 
        NOTIFY_CODE_HANDLER(CBEN_ENDEDIT,   OnComboExEndEdit ) 
    END_MSG_MAP()

    // message handlers
    LRESULT OnPaint( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnEraseBkgnd( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSize( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnCtlColorStatic( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSearchLink( int, LPNMHDR, BOOL&);
    LRESULT OnEditChange( WORD, WORD, HWND, BOOL&);
    LRESULT OnChildSetFocusCmd( WORD, WORD, HWND, BOOL&);
    LRESULT OnChildSetFocusNotify( int, LPNMHDR, BOOL&);
    LRESULT OnChildKillFocusCmd( WORD, WORD, HWND, BOOL&);
    LRESULT OnChildKillFocusNotify( int, LPNMHDR, BOOL&);
    LRESULT OnComboExEndEdit( int, LPNMHDR, BOOL&);

    //  utility methods:
    void _Attach( HWND hwnd )    { _hwnd = hwnd; }
    void _BeautifyCaption( UINT nIDCaption, UINT nIDIcon=0, UINT nIDIconResource=0 );
    void _LayoutCaption( UINT nIDCaption, UINT nIDIcon, UINT nIDDiv, LONG cxDlg );
    void _LayoutSearchLinks( UINT nIDCaption, UINT nIDDiv, BOOL bShowDiv, 
                             LONG left, LONG right, LONG yMargin, LONG& yStart, 
                             const int rgLinkIDs[], LONG cLinkIDs );

    //  data
    CFileSearchBand* _pfsb;      // master band object
    VARIANT          _varScope0;
    VARIANT          _varQueryFile0;
    HWND             _hwndLastFocus;

private:
    HWND             _hwnd;
};

inline BOOL CBandDlg::GetMinSize( HWND hwndOC, LPSIZE pSize ) const { 
    pSize->cx = pSize->cy = 0; return TRUE;
}

inline BOOL CBandDlg::GetAutoCompleteObjectForWindow( HWND hwnd, IAutoComplete2** ppac2 ) { 
    *ppac2 = NULL; return FALSE;
}

//-------------------------------------------------------------------------//
//  Search constraint names
enum FSB_CONSTRAINT
{
    FSBC_SEARCHFOR,         // SCE_SEARCHFORCOMPUTERS
    FSBC_INDEXEDSEARCH,     // SCE_SEARCHFORFILES
    FSBC_NAMED,             // SCE_SEARCHFORFILES
    FSBC_LOOKIN,            // SCE_SEARCHFORFILES, SCE_SEARCHFORCOMPUTERS 
    FSBC_CONTAININGTEXT,    // SCE_SEARCHFORFILES
    FSBC_SIZELE,            // SCE_SEARCHFORFILES
    FSBC_SIZEGE,            // SCE_SEARCHFORFILES
    FSBC_WHICHDATE,         // SCE_SEARCHFORFILES
    FSBC_DATENMONTHS,       // SCE_SEARCHFORFILES
    FSBC_DATENDAYS,         // SCE_SEARCHFORFILES
    FSBC_DATELE,            // SCE_SEARCHFORFILES
    FSBC_DATEGE,            // SCE_SEARCHFORFILES
    FSBC_FILETYPE,          // SCE_SEARCHFORFILES
    FSBC_SEARCHSUBFOLDERS,  // SCE_SEARCHFORFILES
    FSBC_CASE,              // SCE_SEARCHFORFILES
    FSBC_REGULAREXPR,       // SCE_SEARCHFORFILES
    FSBC_SLOWFILES,         // SCE_SEARCHFORFILES
    FSBC_QUERYDIALECT,      // SCE_SEARCHFORFILES
    FSBC_WARNINGFLAGS,      // SCE_SEARCHFORFILES
    _fsbc_count,
};

BOOL    IsConstraintName( FSB_CONSTRAINT constraint, LPCWSTR pwszName );

//-------------------------------------------------------------------------//
//  Band dialog as searchCmdExt object wrap and event sink.
class CSearchCmdDlg :   public IDispatch,
                        public CBandDlg
//-------------------------------------------------------------------------//
{
public:
    CSearchCmdDlg( CFileSearchBand* pfsb );
    ~CSearchCmdDlg();

    //-----------------------------//
    // ISearchCommandExt event sink

    //  IUnknown
    STDMETHOD (QueryInterface) ( REFIID riid, void** ppvObject );
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IDispatch methods 
    STDMETHOD(GetTypeInfoCount)(UINT*)              { return E_NOTIMPL;}
    STDMETHOD(GetTypeInfo)(UINT, LCID, ITypeInfo**) { return E_NOTIMPL;}
    STDMETHOD(GetIDsOfNames)(REFIID, OLECHAR**, UINT, LCID, DISPID*) { return E_NOTIMPL;}
    STDMETHOD(Invoke)(DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT*);

    //  CBandDlg overrides
    virtual HWND Create( HWND hwndParent ) = 0;
    virtual void Clear();
    STDMETHOD (TranslateAccelerator)( LPMSG lpmsg );


    //  CSearchCmdDlg methods
    ISearchCommandExt* GetSearchCmd();
    virtual BOOL SearchInProgress() const { return _fSearchInProgress; }
    BOOL         SearchAborted() const    { return _fSearchAborted; }

    STDMETHOD (AddConstraints)( ISearchCommandExt* pSrchCmd ) { return E_NOTIMPL; }
    STDMETHOD (RestoreConstraint)( const BSTR bstrName, const VARIANT* pValue ) { return E_NOTIMPL; }

    HRESULT      StartSearch();
    virtual void StopSearch();
    HRESULT      SetQueryFile( IN VARIANT* pvarScope );

    HRESULT      ConnectEvents( IUnknown* punk );
    HRESULT      DisconnectEvents();
    static void  EnableStartStopButton( HWND hwndBtn, BOOL bEnable );

    //  Overrideables
    virtual int  GetSearchType() const = 0; // ret: SCE_SEARCHFORxxx
    virtual HWND GetAnimation() { return NULL ; }

    virtual void UpdateSearchCmdStateUI( DISPID dispid = 0 );
    virtual BOOL OnSearchCmdError( HRESULT hr, LPCTSTR pszError );
    virtual void UpdateStatusText();
    virtual void StartStopAnimation( BOOL bStart ) ;
    virtual void RestoreSearch() {};
    virtual void OnBandShow( BOOL bShow ) ;

protected:
    BOOL ProcessCmdError();

    BEGIN_MSG_MAP( CSearchCmdDlg )
    MESSAGE_HANDLER( WMU_RESTORESEARCH, OnRestoreSearch )
    CHAIN_MSG_MAP( CBandDlg )
    END_MSG_MAP()

    //  message handlers
    LRESULT OnRestoreSearch( UINT, WPARAM, LPARAM, BOOL& );

protected:
    //  utility methods
    HRESULT Execute(BOOL bStart);
    
    //  data
    IConnectionPoint*   _pcp;
    DWORD               _dwConnection;
    ISearchCommandExt*  _pSrchCmd;
    BOOL                _fSearchInProgress,
                        _fSearchAborted,
                        _fOnDestroy;
};

//-------------------------------------------------------------------------//
// CFindFilesDlg - top level dialog for Search for Files and Folders UI
class CFindFilesDlg : public CDialogImpl<CFindFilesDlg>,
                      public CSearchCmdDlg
//-------------------------------------------------------------------------//
{
public:
    CFindFilesDlg( CFileSearchBand* pfsb );
    ~CFindFilesDlg();

    virtual HWND Create( HWND hwndParent );
    virtual UINT GetIconID() const       { return IDC_FSEARCH_ICON; }
    virtual UINT GetCaptionID() const    { return IDC_FSEARCH_CAPTION; }
    virtual UINT GetCaptionDivID() const { return IDC_FSEARCH_DIV1; }
    virtual void LayoutControls( int cx = -1, int cy = -1 );
    virtual BOOL Validate();
    virtual void Clear();
    virtual BOOL GetMinSize( HWND hwndOC, LPSIZE pSize ) const;
    virtual int  GetIdealDeskbandWidth() const;
    virtual void NavigateToResults( IWebBrowser2* pwb2 );
    virtual HWND ShowHelp( HWND hwndOwner );

    virtual HWND GetFirstTabItem() const;
    virtual HWND GetLastTabItem() const;
    virtual BOOL GetAutoCompleteObjectForWindow( HWND hwnd, IAutoComplete2** ppac2 );
    STDMETHOD (TranslateAccelerator)( LPMSG lpmsg );

    virtual int  GetSearchType() const  { return SCE_SEARCHFORFILES; }
    virtual HWND GetAnimation() { return GetDlgItem( IDC_FSEARCH_ICON ); }

    STDMETHOD (AddConstraints)( ISearchCommandExt* pSrchCmd );
    STDMETHOD (RestoreConstraint)( const BSTR bstrName, const VARIANT* pValue );
    virtual void UpdateSearchCmdStateUI( DISPID eventID = 0 );
    virtual void RestoreSearch();
    virtual void LoadSaveUIState( UINT nIDCtl, BOOL bSave );
    virtual BOOL OnSearchCmdError( HRESULT hr, LPCTSTR pszError );


    BOOL SetDefaultScope();
    virtual void OnBandShow( BOOL bShow );
    virtual void OnBandDialogShow( BOOL bShow );

    virtual HRESULT SetScope( IN VARIANT* pvarScope, BOOL bTrack = FALSE );
    BOOL AssignNamespace( LPCTSTR pszNamespace, LPCTSTR pszPath, BOOL bPassive /*only if no current selection*/ );
    BOOL GetMinMaxInfo( HWND hwndOC, LPMINMAXINFO pmmi );
    void OnWinIniChange();

    enum { IDD = DLG_FSEARCH_MAIN };

    BEGIN_MSG_MAP( CFindFilesDlg)
        MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
        MESSAGE_HANDLER( WM_ERASEBKGND, OnEraseBkgnd )
        MESSAGE_HANDLER( WM_TIMER, OnTimer )
        MESSAGE_HANDLER( WM_DESTROY, OnDestroy )
        MESSAGE_HANDLER( WMU_COMBOPOPULATIONCOMPLETE, OnComboPopulationComplete )
        MESSAGE_HANDLER( WMU_STATECHANGE, OnStateChange )
        MESSAGE_HANDLER( WMU_UPDATELAYOUT, OnUpdateLayout ) 
        MESSAGE_HANDLER( WMU_NAMESPACERECONCILE, OnNamespaceReconcileMsg )
        COMMAND_HANDLER( IDC_FILESPEC,   EN_CHANGE, OnEditChange )
        COMMAND_HANDLER( IDC_GREPTEXT,   EN_CHANGE, OnEditChange )
        COMMAND_HANDLER( IDC_NAMESPACE,  CBN_EDITCHANGE, OnNamespaceEditChange)
        COMMAND_HANDLER( IDC_NAMESPACE,  CBN_SELENDOK, OnNamespaceSelEndOk )
        COMMAND_HANDLER( IDC_NAMESPACE,  CBN_SELENDCANCEL, OnNamespaceReconcileCmd )
        COMMAND_HANDLER( IDC_NAMESPACE,  CBN_DROPDOWN, OnNamespaceReconcileCmd )
        COMMAND_CODE_HANDLER(BN_CLICKED, OnBtnClick);
        NOTIFY_HANDLER( IDC_NAMESPACE, CBEN_DELETEITEM, OnNamespaceDeleteItem ) 
        NOTIFY_HANDLER( IDC_NAMESPACE, CBEN_ENDEDIT, OnNamespaceReconcileNotify ) 
        NOTIFY_HANDLER( IDC_SEARCHLINK_OPTIONS,   NM_CLICK, OnOptions )
        NOTIFY_HANDLER( IDC_SEARCHLINK_OPTIONS,   NM_RETURN, OnOptions )
        NOTIFY_HANDLER( IDC_GROUPBTN_OPTIONS,     NM_CLICK, OnOptions )
        NOTIFY_HANDLER( IDC_GROUPBTN_OPTIONS,     NM_RETURN, OnOptions )
        NOTIFY_HANDLER( IDC_GROUPBTN_OPTIONS,     GBN_QUERYBUDDYHEIGHT, OnQueryOptionsHeight )
        NOTIFY_HANDLER( IDC_SEARCHLINK_FILES,     NM_CLICK, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_FILES,     NM_RETURN, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_COMPUTERS, NM_CLICK, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_COMPUTERS, NM_RETURN, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_PRINTERS,  NM_CLICK, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_PRINTERS,  NM_RETURN, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_PEOPLE,    NM_CLICK, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_PEOPLE,    NM_RETURN, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_INTERNET,  NM_CLICK, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_INTERNET,  NM_RETURN, OnSearchLink )

        CHAIN_MSG_MAP( CSearchCmdDlg ) // fall through to base class handlers
    END_MSG_MAP()

protected:
    //  message handlers
    LRESULT OnInitDialog( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnEraseBkgnd( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnComboPopulationComplete( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnStateChange( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnTimer( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnUpdateLayout( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnBtnClick( WORD, WORD, HWND, BOOL&);
    LRESULT OnNamespaceDeleteItem( int, LPNMHDR, BOOL&);
    LRESULT OnNamespaceEditChange( WORD, WORD, HWND, BOOL&);
    LRESULT OnNamespaceSelEndOk( WORD, WORD, HWND, BOOL&);
    LRESULT OnNamespaceReconcileCmd( WORD, WORD, HWND, BOOL&);
    LRESULT OnNamespaceReconcileNotify( int, LPNMHDR, BOOL&);
    LRESULT OnNamespaceReconcileMsg( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnOptions( int, LPNMHDR, BOOL&);
    LRESULT OnQueryOptionsHeight( int, LPNMHDR, BOOL&);
    LRESULT OnDestroy( UINT, WPARAM, LPARAM, BOOL& );

private:
    //  misc utility methods
    void                _ShowOptions( BOOL bShow = TRUE );

    //  namespace scoping
    BOOL                _SetPreassignedScope();
    BOOL                _SetFolderScope();
    BOOL                _SetLocalDefaultScope();
    void                _ShowNamespaceEditImage( BOOL bShow );

    //  ad hoc namespace handling
    BOOL                _PathFixup( LPTSTR pszDst, LPCTSTR pszSrc );
    BOOL                _ShouldReconcileAdHocNamespace();
    void                _UIReconcileAdHocNamespace( IN HWND hwndComboBox, 
                                                    IN OPTIONAL LPCTSTR pszNamespace = NULL, 
                                                    BOOL bAsync = FALSE);
    INT_PTR             _ReconcileAdHocNamespace( IN HWND hwndComboBox,
                                                  IN OPTIONAL LPCTSTR pszNamespace = NULL,
                                                  IN OPTIONAL BOOL bAsync = FALSE );
    static INT_PTR      _AddAdHocNamespace( IN HWND hwndComboBox, IN LPCTSTR pszPath, BOOL bSelectItem );

    //  namespace browsing
    void                _BrowseAndAssignNamespace();
    static STDMETHODIMP _BrowseForNamespace( IN HWND hwndOwner, IN OUT LPTSTR pszPath, IN UINT cchPath,
                                             OUT OPTIONAL LPBOOL pbForParsing = NULL, 
                                             OUT OPTIONAL LPITEMIDLIST* ppidlRet = NULL );
    static int          _BrowseCallback( HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData );

    //  more namespace helpers
    static BOOL         _IsSearchableFolder( IN LPCITEMIDLIST pidlFolder, IN OPTIONAL HWND hwndDlg = NULL );
    STDMETHODIMP        _GetTargetNamespace( OUT LPTSTR pszText, IN int cchText, 
                                             IN OUT OPTIONAL COMBOBOXEXITEM* pItem = NULL, 
                                             OUT OPTIONAL UINT* pnErrStr = NULL );
    STDMETHODIMP        _GetNextNamespace(   OUT LPTSTR pszText, IN int cchText, 
                                             IN OUT COMBOBOXEXITEM* pItem );
    static INT_PTR      _FindNamespace( IN HWND hwndComboBox, IN LPCTSTR pszNamespace, IN BOOL bForParsing );
    static INT_PTR      _FindNamespace( IN HWND hwndComboBox, IN LPCITEMIDLIST pidl );
    static BOOL         _IsPathSingleton( IN LPCTSTR pszText );
    

private:
    //  namespace combo thread
    static STDMETHODIMP AddNamespaceItemNotify( ULONG, PCBXITEM, LPARAM );
    static DWORD        NamespaceThreadProc( void* pvParam );

    //  data
    COptionsDlg         _dlgOptions;
    HANDLE              _hNamespaceThread; // background thread to populate namespace control
    FSEARCHTHREADSTATE  _threadState;
    TCHAR               _szInitialNamespace[MAX_URL_STRING],  // SHGDN_NORMAL
                        _szInitialPath[MAX_PATH];            // SHGDN_FORPARSING
    TCHAR               _szCurrentPath[MAX_PATH];
    TCHAR               _szLocalDrives[MAX_URL_STRING];
    ULONG               _dwWarningFlags;        // docfind warning bits.
    BOOL                _dwRunOnceWarningFlags; 

    LRESULT             _iCurNamespace;     // combobox item index for currently selected namespace.
    BOOL                _fDebuted,          // this band dialog has been displayed before.
                        _bScoped,           // ve assigned a value to the namespace combo.
                        _fAdHocNamespace,   // TRUE if the user has been typing in the namespace combo.
                        _fDisplayOptions;   // Search Options group box is displayed

    IAutoComplete2*     _pacGrepText;       // 'Containing Text' autocomplete object    
    IStringMru*         _pmruGrepText;      // 'Containing Text' mru object
    IAutoComplete2*     _pacFileSpec;       // 'Files Named' autocomplete object    
    IStringMru*         _pmruFileSpec;      // 'Files Named' mru object

    enum {
        TRACKSCOPE_NONE,
        TRACKSCOPE_GENERAL,
        TRACKSCOPE_SPECIFIC,
    } ;
    ULONG               _fTrackScope;          // defines scope-tracking behavior.  See TRACKSCOPE_xxx flags
};

inline HWND CFindFilesDlg::Create( HWND hwndParent ) {
    return CDialogImpl<CFindFilesDlg>::Create( hwndParent );
}

inline void CFindFilesDlg::_ShowNamespaceEditImage( BOOL bShow ) {
    SendDlgItemMessage( IDC_NAMESPACE, CBEM_SETEXTENDEDSTYLE, 
                        CBES_EX_NOEDITIMAGE, bShow ? 0 : CBES_EX_NOEDITIMAGE );
}


#ifdef __PSEARCH_BANDDLG__
//-------------------------------------------------------------------------//
// CFindPrintersDlg - top level dialog for Search for Printers UI
class CFindPrintersDlg : public CDialogImpl<CFindPrintersDlg>,
                         public CBandDlg
//-------------------------------------------------------------------------//
{
public:
    CFindPrintersDlg( CFileSearchBand* pfsb );
    ~CFindPrintersDlg();

    // overrides of CBandDlg    
    virtual HWND Create( HWND hwndParent );
    virtual UINT GetIconID() const       { return IDC_PSEARCH_ICON; }
    virtual UINT GetCaptionID() const    { return IDC_PSEARCH_CAPTION; }
    virtual UINT GetCaptionDivID() const { return IDC_FSEARCH_DIV1; }
    virtual void LayoutControls( int cx = -1, int cy = -1 );
    virtual BOOL Validate();
    virtual void Clear();
    virtual BOOL GetMinSize( HWND hwndOC, LPSIZE pSize ) const;
    virtual HWND GetFirstTabItem() const;
    virtual HWND GetLastTabItem() const;
    STDMETHOD (TranslateAccelerator)( LPMSG lpmsg );
    virtual void OnWinIniChange();

    enum { IDD = DLG_PSEARCH };

protected:
    BEGIN_MSG_MAP( CFindPrintersDlg)
        MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
        COMMAND_HANDLER( IDC_PSEARCH_NAME,      EN_CHANGE, OnEditChange )
        COMMAND_HANDLER( IDC_PSEARCH_LOCATION,  EN_CHANGE, OnEditChange )
        COMMAND_HANDLER( IDC_PSEARCH_MODEL,     EN_CHANGE, OnEditChange )
        COMMAND_HANDLER( IDC_SEARCH_START,      BN_CLICKED,  OnSearchStartBtn )
        NOTIFY_HANDLER( IDC_SEARCHLINK_COMPUTERS, NM_CLICK, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_COMPUTERS, NM_RETURN, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_FILES,  NM_CLICK, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_FILES,     NM_RETURN, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_PEOPLE,    NM_CLICK, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_PEOPLE,    NM_RETURN, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_INTERNET,  NM_CLICK, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_INTERNET,  NM_RETURN, OnSearchLink )
    CHAIN_MSG_MAP( CBandDlg )
    END_MSG_MAP()

    //  message handlers
    LRESULT OnInitDialog( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSearchStartBtn( WORD, WORD, HWND, BOOL&);
};

inline HWND CFindPrintersDlg::Create( HWND hwndParent ) {
    return CDialogImpl<CFindPrintersDlg>::Create( hwndParent );
}
#endif __PSEARCH_BANDDLG__

//-------------------------------------------------------------------------//
// CFindComputersDlg - top level dialog for Search for Computers UI
class CFindComputersDlg : public CDialogImpl<CFindComputersDlg>,
                          public CSearchCmdDlg
//-------------------------------------------------------------------------//
{
public:
    CFindComputersDlg( CFileSearchBand* pfsb );
    ~CFindComputersDlg();

    // overrides of CBandDlg    
    virtual HWND Create( HWND hwndParent );
    virtual UINT GetIconID() const       { return IDC_CSEARCH_ICON; }
    virtual UINT GetCaptionID() const    { return IDC_CSEARCH_CAPTION; }
    virtual UINT GetCaptionDivID() const { return IDC_FSEARCH_DIV1; }
    virtual void LayoutControls( int cx = -1, int cy = -1 );
    virtual BOOL Validate();
    virtual void Clear();
    virtual BOOL GetMinSize( HWND hwndOC, LPSIZE pSize ) const;
    virtual void NavigateToResults( IWebBrowser2* pwb2 );

    virtual HWND ShowHelp( HWND hwndOwner );
    virtual HWND GetFirstTabItem() const;
    virtual HWND GetLastTabItem() const;
    virtual BOOL GetAutoCompleteObjectForWindow( HWND hwnd, IAutoComplete2** ppac2 );
    STDMETHOD (TranslateAccelerator)( LPMSG lpmsg );
    virtual void OnWinIniChange();
    
    virtual int  GetSearchType() const  { return SCE_SEARCHFORCOMPUTERS; }
    virtual HWND GetAnimation() { return GetDlgItem( IDC_CSEARCH_ICON ); }

    STDMETHOD (AddConstraints)( ISearchCommandExt* pSrchCmd );
    virtual void UpdateStatusText();
    virtual void RestoreSearch();

    enum { IDD = DLG_CSEARCH };

protected:

    BEGIN_MSG_MAP( CFindComputersDlg)
        MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
        MESSAGE_HANDLER( WM_DESTROY, OnDestroy )
        COMMAND_HANDLER( IDC_CSEARCH_NAME,      EN_CHANGE, OnEditChange )
        COMMAND_HANDLER( IDC_SEARCH_START,      BN_CLICKED,  OnSearchStartBtn )
        COMMAND_HANDLER( IDC_SEARCH_STOP,       BN_CLICKED,  OnSearchStopBtn )
        NOTIFY_HANDLER( IDC_SEARCHLINK_FILES,     NM_CLICK, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_FILES,     NM_RETURN, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_PRINTERS,  NM_CLICK, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_PRINTERS,  NM_RETURN, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_PEOPLE,    NM_CLICK, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_PEOPLE,    NM_RETURN, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_INTERNET,  NM_CLICK, OnSearchLink )
        NOTIFY_HANDLER( IDC_SEARCHLINK_INTERNET,  NM_RETURN, OnSearchLink )
    CHAIN_MSG_MAP( CSearchCmdDlg )
    END_MSG_MAP()

    //  message handlers
    LRESULT OnInitDialog( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnDestroy( UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnSearchStartBtn( WORD, WORD, HWND, BOOL&);
    LRESULT OnSearchStopBtn( WORD, WORD, HWND, BOOL&);

    //  data
    IAutoComplete2*     _pacComputerName;    // 'Files Named' autocomplete object    
    IStringMru*         _pmruComputerName;   // 'Files Named' mru object
};

inline HWND CFindComputersDlg::Create( HWND hwndParent ) {
    return CDialogImpl<CFindComputersDlg>::Create( hwndParent );
}

//-------------------------------------------------------------------------//
DECLARE_INTERFACE_(IStringMru, IUnknown)
{
    // *** IStringMru specific methods ***
    STDMETHOD(Add)(LPCOLESTR pwszAdd) PURE;
};
extern const IID IID_IStringMru;

//-------------------------------------------------------------------------//
class CStringMru : public IStringMru, public IEnumString
//-------------------------------------------------------------------------//
{
public:
    static  HRESULT CreateInstance( HKEY hKey, LPCTSTR szSubKey, LONG cMaxStrings, BOOL bCaseSensitive,
                                    REFIID riid, LPVOID* ppv ); 

protected:
    // *** IStringMru ***//
    virtual STDMETHODIMP Add( LPCOLESTR pwsz );  // adds or promotes a string

    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IEnumString ***
    virtual STDMETHODIMP Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched);
    virtual STDMETHODIMP Skip(ULONG celt);
    virtual STDMETHODIMP Reset(void);
    virtual STDMETHODIMP Clone(IEnumString **ppenum)    { return E_NOTIMPL; }

private:
    CStringMru();
    ~CStringMru();

    HRESULT _Open();
    HRESULT _Read( OUT OPTIONAL LONG* pcRead = NULL /*count of strings read*/);
    HRESULT _Write( OUT OPTIONAL LONG* pcWritten = NULL /*count of strings written*/);
    HRESULT _Promote( LONG iString );
    void    _Close();
    void    _Delete();
    void    _Clear(); 

    HKEY    _hKey,
            _hKeyRoot;
    TCHAR   _szSubKey[MAX_PATH];
    ULONG   _cRef;
    BOOL    _bCaseSensitive;
    LONG    _cMax;
    LONG    _iString;
    HDPA    _hdpaStrings;
};

//-------------------------------------------------------------------------//
//  DivWindow registration
#define  DIVWINDOW_CLASS     TEXT("DivWindow")
EXTERN_C BOOL WINAPI DivWindow_RegisterClass();
EXTERN_C BOOL WINAPI DivWindow_UnregisterClass( HINSTANCE );

#define  DWM_FIRST          (WM_USER+0x300)
#define  DWM_SETHEIGHT      (DWM_FIRST+0)   // WPARAM: height in pixels, LPARAM: n/a, ret: BOOL
#define  DWM_SETBKCOLOR     (DWM_FIRST+1)   // WPARAM: COLORREF, LPARAM: n/a, ret: BOOL

//--------------------//
//  Helper macros
#ifndef RECTWIDTH
#define RECTWIDTH(prc)  ((prc)->right - (prc)->left)
#endif//RECTWIDTH
#ifndef RECTHEIGHT
#define RECTHEIGHT(prc) ((prc)->bottom - (prc)->top)
#endif//RECTHEIGHT
#define POINTSPERRECT   (sizeof(RECT)/sizeof(POINT))

//---------------------//
//  Misc utility
LONG _PixelsForDbu( HWND hwndDlg, LONG cDbu, BOOL bHorz );

#endif //__FSEARCH_DLGS_H__
