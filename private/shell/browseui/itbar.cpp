#include "priv.h"
#include "sccls.h"

#include "itbdrop.h"
#include <urlhist.h>
#include "autocomp.h"
#include "itbar.h"
#include "address.h"
#include "isfband.h"
#include <winbase.h>
#include "qlink.h"
#include "inpobj.h"
#include "basebar.h"
#include "shbrowse.h"
#include "menuisf.h"
#include "menuband.h"
#include "brand.h"
#include "resource.h"
#include "theater.h"
#include "browmenu.h"
#include "util.h"
#include "tbmenu.h"
#include "apithk.h"
#include "shbrows2.h"
#include "stdenum.h"
#include "iehelpid.h"

#define WANT_CBANDSITE_CLASS
#include "bandsite.h"
#include "schedule.h"
#include "uemapp.h"

#include "mluisupp.h"

#ifdef UNIX
extern "C"  const GUID  CLSID_MsgBand;
#endif

// The edit button hackery needs to be moved to shdocvw.  This define identifies this code.
#define EDIT_HACK

// Offset of the comctl32 default bitmaps
#define OFFSET_HIST             (MAX_TB_BUTTONS - 1 + 0)   // 15
#define OFFSET_STD              (MAX_TB_BUTTONS - 1 + 6)   // 21
#define OFFSET_VIEW             (MAX_TB_BUTTONS - 1 + 21)  // 36

// This is the offset in the toolbar for the Shell glyphs and the Shell toolbar labels
#define SHELLTOOLBAR_OFFSET     (MAX_TB_BUTTONS - 1 + 1)  // 16
#define FONTGLYPH_OFFSET        (MAX_TB_BUTTONS - 1 + 38) // 53
#define BRIEFCASEGLYPH_OFFSET   (MAX_TB_BUTTONS - 1 + 34) // 49
#define RNAUIGLYPH_OFFSET       (MAX_TB_BUTTONS - 1 + 36) // 51
#define WEBCHECKGLYPH_OFFSET    (MAX_TB_BUTTONS - 1 + 42) // 57
#ifdef EDIT_HACK
#define EDITGLYPH_OFFSET        (9)
#endif

#define IDT_UPDATETOOLBAR       0x1
#define TIMEOUT_UPDATETOOLBAR   400

const GUID CLSID_Separator = { 0x67077B90L, 0x4F9D, 0x11D0, 0xB8, 0x84, 0x00, 0xAA, 0x00, 0xB6, 0x01, 0x04 };

extern HRESULT VariantClearLazy(VARIANTARG *pvarg);

// How many CT_TABLE structures to allocated at a time.
#define TBBMPLIST_CHUNK     5

#define MAX_EXTERNAL_BAND_NAME_LEN 64

#define MAX_TB_COMPRESSED_WIDTH 42
// 16 is added to the the MAX_TB defines. This is added through the strings
// in the RC file. This is done so that the localization folks can increase
// or decrease the width of the toolbar buttons
#define MAX_TB_WIDTH_LORES      38
#define MAX_TB_WIDTH_HIRES      60

// Dimensions of Coolbar Glyphs ..
#define TB_SMBMP_CX               16
#define TB_SMBMP_CY               16
#define TB_BMP_CX               20
#define TB_BMP_CY               20

#define CX_SEPARATOR    6     // we override toolbar control's default separator width of 8

#define DM_TBSITE   0
#define DM_TBCMD    0
#define DM_TBREF    TF_SHDREF
#define DM_LAYOUT   0
#define DM_ITBAR    0

#define TF_TBCUST   0x01000000

#if CBIDX_LAST != 5
#error Expected CBIDX_LAST to have value of 5
#endif

#if (FCIDM_EXTERNALBANDS_LAST - FCIDM_EXTERNALBANDS_FIRST + 1) < MAXEXTERNALBANDS
#error Insufficient range for FCIDM_EXTERNALBANDS_FIRST to FCIDM_EXTERNALBANDS_LAST
#endif


__inline UINT EXTERNALBAND_VBF_BIT(UINT uiBandExt)
{
    ASSERT(uiBandExt < MAXEXTERNALBANDS);

    // Formula: take 1, shift left by uiBandExt + 16
    //      => a bit in range (0x80000000, 0x00010000)
    UINT uBit = 1 << (uiBandExt + 16);
    ASSERT(uBit & VBF_EXTERNALBANDS);

    return uBit;
}

__inline BOOL IS_EXTERNALBAND(int idBand)
{
    return (InRange(idBand, CBIDX_EXTERNALFIRST, CBIDX_EXTERNALLAST));
}

__inline int MAP_TO_EXTERNAL(int idBand)
{
    ASSERT(IS_EXTERNALBAND(idBand));

    // CBIDX_LAST is one-based, mapping is zero-based
    return (idBand - (1 + CBIDX_LAST));
}


// maximum number of menu items in the context menus for back and forward.
#define MAX_NAV_MENUITEMS               9

#define DEFAULT_SEARCH_GUID    SRCID_SFileSearch //SRCID_SWebSearch

#define SZ_PROP_CUSTDLG     TEXT("Itbar custom dialog hwnd")

#define REG_KEY_BANDSTATE  TEXT("Software\\Microsoft\\Internet Explorer\\Toolbar")

DWORD DoNetConnect(HWND hwnd);
DWORD DoNetDisconnect(HWND hwnd);


void _LoadToolbarGlyphs(HWND hwnd, IMLCACHE *pimlCache, int cx, int idBmp);

BOOL _UseSmallIcons();


typedef struct tagTBBMP_LIST
{
    HINSTANCE hInst;
    UINT_PTR  uiResID;
    UINT  uiOffset;
    BITBOOL  fNormal:1;
    BITBOOL  fHot:1;
    BITBOOL  fDisabled:1;
} TBBMP_LIST;

typedef struct tagCMDMAP
{
    GUID    guidButtonGroup;
    UINT    nCmdID;
    LPARAM lParam;  // app's data
} CMDMAP;

typedef struct tagCMDMAPCUSTOMIZE
{
    TBBUTTON btn;
    CMDMAP cm;
} CMDMAPCUSTOMIZE;

typedef struct {

    // the IOleCommandTarget info:
    GUID guid;
    UINT nCmdID;
    UINT fButtonState;
} BUTTONSAVEINFO;

#define TBSI_VERSION            7
typedef struct {
    int cVersion;
} TOOLBARSAVEINFO;

typedef struct {
    HDSA hdsa;
    BITBOOL fAdjust:1;
    BITBOOL fDirty:1;
} CUSTOMIZEINFO, *LPCUSTOMIZEINFO;

//Current latest version.
#define CBS_VERSION             17

// NOTE: Be very careful changing COOLBARSAVE because _LoadUpgradeSettings makes
// assumptions about the layout of the structure.  To avoid breaking that
// upgrade code, be sure you:
//
//  - don't change the order of existing members
//  - always add new members to the end of the structure.
//  - update _LoadUpgradeSettings if appropriate
//
typedef struct tagCOOLBARSAVE
{
    UINT        cbVer;
    UINT        uiMaxTBWidth;
    UINT        uiMaxQLWidth;
#ifdef UNIX
    BITBOOL     fUnUsed : 28;       // unused
#endif
    BITBOOL     fVertical : 1;      // The bar is oriented vertically
    BITBOOL     fNoText :1;         // "NoText"
    BITBOOL     fList : 1;          // toolbar is TBSTYLE_LIST (text on right) + TBSTYLE_EX_MIXEDBUTTONS
    BITBOOL     fAutoHide : 1;      // Auto hide toolbar in theater mode
    BITBOOL     fStatusBar : 1;     // Status bar in theater mode
    BITBOOL     fSaveInShellIntegrationMode : 1;     // Did we save in shell integration mode?
    UINT        uiVisible;          // "Visible bands"
    UINT        cyRebar;
    BANDSAVE    bs[CBANDSMAX];
    CLSID       aclsidExternalBands[ MAXEXTERNALBANDS ];  // Check classid
    CLSID       clsidVerticalBar;       //clsid of bar persisted within vertical band
    CLSID       clsidHorizontalBar;
} COOLBARSAVE, *LPCOOLBARSAVE;

//Flags for dwFlags passed to UpdateToolbarDisplay()
#define UTD_TEXTLABEL  0x00000001
#define UTD_VISIBLE    0x00000002

static const TCHAR c_szRegKeyCoolbar[] = TSZIEPATH TEXT("\\Toolbar");
static const TCHAR c_szValueTheater[]  = TEXT("Theater");

typedef struct tagFOLDERSEARCHITEM
{
    UINT    idCmd;
    GUID    guidSearch;
    int     iIcon;
    WCHAR   wszUrl[MAX_URL_STRING];
    WCHAR   wszName[80];           // friendly name
}FOLDERSEARCHITEM, *LPFOLDERSEARCHITEM;

BOOL NavigateSearchBar(IWebBrowser2 *pwb2, LPCWSTR pwszUrl);
BOOL _GetSearchHKEY(LPGUID lpguidSearch, HKEY *phkey);

#define REG_SZ_STATIC       TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FindExtensions\\Static")
#define REG_SZ_SEARCH_GUID  TEXT("SearchGUID")
#define REG_SZ_SEARCH_URL   TEXT("SearchGUID\\Url")

#define VIEW_OFFSET (SHELLGLYPHS_OFFSET + HIST_MAX + STD_MAX)
#define VIEW_ALLFOLDERS  (VIEW_NETCONNECT + 14)

static const TBBUTTON    c_tbExplorer[] =
{
    // override default toolbar width for separators; iBitmap member of
    // TBBUTTON struct is a union of bitmap index & separator width

    { 0, TBIDM_BACK  ,      0,               BTNS_DROPDOWN | BTNS_SHOWTEXT, {0,0}, 0, 0 },
    { 1, TBIDM_FORWARD,     0,               BTNS_DROPDOWN, {0,0}, 0, 1 },

    { 2, TBIDM_STOPDOWNLOAD, TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, 2 },
    { 3, TBIDM_REFRESH,      TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, 3 },
    { 4, TBIDM_HOME,         TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, 4 },

    { VIEW_PARENTFOLDER + VIEW_OFFSET,    TBIDM_PREVIOUSFOLDER,   TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, VIEW_PARENTFOLDER + VIEW_OFFSET },
    { VIEW_NETCONNECT + VIEW_OFFSET,      TBIDM_CONNECT,          TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, VIEW_NETCONNECT + VIEW_OFFSET },
    { VIEW_NETDISCONNECT + VIEW_OFFSET,   TBIDM_DISCONNECT,       TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, VIEW_NETDISCONNECT + VIEW_OFFSET },

    { CX_SEPARATOR, 0,          TBSTATE_ENABLED, BTNS_SEP, {0,0}, 0, -1 },
    { 5, TBIDM_SEARCH,       TBSTATE_ENABLED, BTNS_SHOWTEXT, {0,0}, 0, 5 },
    { VIEW_ALLFOLDERS + VIEW_OFFSET,    TBIDM_ALLFOLDERS,         TBSTATE_ENABLED, BTNS_SHOWTEXT, {0,0}, 0, VIEW_ALLFOLDERS + VIEW_OFFSET },
    { 6, TBIDM_FAVORITES,       TBSTATE_ENABLED,  BTNS_SHOWTEXT, {0,0}, 0, 6 },
    { 12, TBIDM_HISTORY,        TBSTATE_ENABLED, BTNS_SHOWTEXT, {0,0}, 0, 12},
    { CX_SEPARATOR,    0,       TBSTATE_ENABLED, BTNS_SEP, {0,0}, 0, -1 },
#ifndef DISABLE_FULLSCREEN
    // IE UNIX : No theater mode for beta1
    { 14, TBIDM_THEATER,         TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, 14 },
#endif
};

static const BROWSER_RESTRICTIONS c_rest[] = {
    REST_BTN_BACK,
    REST_BTN_FORWARD,
    REST_BTN_STOPDOWNLOAD,
    REST_BTN_REFRESH,
    REST_BTN_HOME,
    REST_BROWSER_NONE,      // no policy for up
    REST_BROWSER_NONE,      // no policy for map drive
    REST_BROWSER_NONE,      // no policy for disconnect drive
    REST_BROWSER_NONE,      // separator
    REST_BTN_SEARCH,
    REST_BTN_ALLFOLDERS,
    REST_BTN_FAVORITES,
    REST_BTN_HISTORY,
    REST_BROWSER_NONE,      // separator
#ifndef DISABLE_FULLSCREEN
    REST_BTN_THEATER,
#endif
};

#define SUPERCLASS CBaseBar

class CInternetToolbar :
   public CBaseBar,
   public IDockingWindow,
   public IObjectWithSite,  // *not* CObjectWithSite (want _ptbSite)
   public IExplorerToolbar,
   public DWebBrowserEvents,
   public IPersistStreamInit,
   public IShellChangeNotify,
   public ISearchItems
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return SUPERCLASS::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release(void){ return SUPERCLASS::Release(); };

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd) { return SUPERCLASS::GetWindow(lphwnd);};
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) {return SUPERCLASS::ContextSensitiveHelp(fEnterMode);};

    // *** IDockingWindow methods ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dwReserved);
    virtual STDMETHODIMP ResizeBorderDW(LPCRECT prcBorder, IUnknown* punkToolbarSite, BOOL fReserved);

    // *** IObjectWithSite methods ***
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);
    // BUGBUG is E_NOTIMPL ok?
    virtual STDMETHODIMP GetSite(REFIID riid, void** ppvSite) { ASSERT(0); return E_NOTIMPL; };

    // *** IInputObjectSite methods ***
    virtual STDMETHODIMP OnFocusChangeIS(IUnknown *punk, BOOL fSetFocus);

    // *** IInputObject methods ***
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IServiceProvider methods ***
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID* ppvObj);

    // *** IExplorerToolbar method ***
    virtual STDMETHODIMP SetCommandTarget(IUnknown* punkCmdTarget, const GUID* pguidButtonGroup, DWORD dwFlags);
    virtual STDMETHODIMP AddStdBrowserButtons(void);

    virtual STDMETHODIMP AddButtons(const GUID* pguidButtonGroup, UINT nButtons, const TBBUTTON * lpButtons);
    virtual STDMETHODIMP AddString(const GUID * pguidButtonGroup, HINSTANCE hInst, UINT_PTR uiResID, LONG_PTR *pOffset);
    virtual STDMETHODIMP GetButton(const GUID* pguidButtonGroup, UINT uiCommand, LPTBBUTTON lpButton);
    virtual STDMETHODIMP GetState(const GUID* pguidButtonGroup, UINT uiCommand, UINT * pfState);
    virtual STDMETHODIMP SetState(const GUID* pguidButtonGroup, UINT uiCommand, UINT fState);
    virtual STDMETHODIMP AddBitmap(const GUID * pguidButtonGroup, UINT uiBMPType, UINT uiCount, TBADDBITMAP * ptb,
                                   LRESULT * pOffset, COLORREF rgbMask);
    virtual STDMETHODIMP GetBitmapSize(UINT * uiID);
    virtual STDMETHODIMP SendToolbarMsg(const GUID * pguidButtonGroup, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT * plRes);

    virtual STDMETHODIMP SetImageList( const GUID* pguidCmdGroup, HIMAGELIST himlNormal, HIMAGELIST himlHot, HIMAGELIST himlDisabled);
    virtual STDMETHODIMP ModifyButton( const GUID * pguidButtonGroup, UINT uiCommand, LPTBBUTTON lpButton);

    // IOleCommandTarget
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup,
                                     ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
                              DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn,
                              VARIANTARG *pvarargOut);

    // IPersistStreamInit
    STDMETHOD(GetClassID)(GUID *pguid);
    STDMETHOD(Load)(IStream *pStm);
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);
    STDMETHOD(InitNew)(void);
    STDMETHOD(IsDirty)(void);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER  *pcbSize);

    /* IDispatch methods */
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo);

    virtual STDMETHODIMP GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo **pptinfo);

    virtual STDMETHODIMP GetIDsOfNames(REFIID riid,OLECHAR **rgszNames,UINT cNames,
                                       LCID lcid, DISPID * rgdispid);

    virtual STDMETHODIMP Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags,
                                DISPPARAMS * pdispparams, VARIANT * pvarResult,
                                EXCEPINFO * pexcepinfo,UINT * puArgErr);

    // IShellChangeNotify
    virtual STDMETHODIMP OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

    // CBaseBar overrides
    virtual LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // *** ISearchItems methods ***
    virtual STDMETHODIMP GetDefaultSearchUrl(LPWSTR pwzUrl, UINT cch);

    CInternetToolbar();
protected:
    virtual ~CInternetToolbar();
    static LRESULT CALLBACK SizableWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void _OnCommand(WPARAM wParam, LPARAM lParam);
    BOOL _SendToToolband(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres);
    LRESULT _OnNotify(LPNMHDR pnmh);
    void _OnTooltipNeeded(LPTOOLTIPTEXT pnmTT);
    void _QueryStatusTip(IOleCommandTarget *pct, LPTOOLTIPTEXT pnmTT, UINT uiCmd, const GUID* pguid);

    BOOL _UpEnabled();
    void _UpdateCommonButton(int iCmd, UINT nCmdID);
    void _UpdateToolbar(BOOL fForce);
    void _UpdateToolbarNow();
    void _UpdateGroup(const GUID *pguidCmdGroup, int cnt, OLECMD rgcmds[], const GUID* pguidButton, const int buttonsInternal[]);
    void _CSHSetStatusBar(BOOL fOn);
    void _StartDownload();
    void _StopDownload(BOOL fClosing);
    void _SendDocCommand(UINT idCmd);
    BOOL _CompressBands(BOOL fCompress, UINT uRowsNew, BOOL fForceUpdate);
    void _TrackSliding(int x, int y);
    HRESULT _DoNavigateA(LPSTR pszURL,int iNewSelection);
    HRESULT _DoNavigateW(LPWSTR pwzURL,int iNewSelection);
    HRESULT _DoNavigate(BSTR bstrURL,int iNewSelection);
    void _Unadvise(void);
    LRESULT _OnBeginDrag(NMREBAR* pnm);

    void _InsertURL(LPTSTR pszURL);

    void _ShowContextMenu(HWND hwnd, LPARAM lParam, LPRECT prcExclude);
    BOOL _ShowBackForwardMenu(BOOL fForward, POINT pt, LPRECT prcExclude);
    // search helper methods
    BOOL _GetFolderSearchData();
    void _SetSearchStuff();
    BOOL _GetSearchUrl(LPWSTR pwszUrl, DWORD cch);
    HRESULT _GetFolderSearches(IFolderSearches **ppfs);


    void _ReloadButtons();
    void _UpdateToolsStyle(BOOL fList);
    void _InitBitmapDSA();
    void _InitForScreenSize();
    void _InitToolbar();
    BOOL _FoldersButtonAvailable();
    void _AdminMarkDefaultButtons(PTBBUTTON ptbb, UINT cButtons);
    void _MarkDefaultButtons(PTBBUTTON ptbb, UINT cButtons);
    void _AddCommonButtons();
    HRESULT _CreateBands();
    BOOL    _ShowBands(UINT fVisible);
    HRESULT _ShowTools(PBANDSAVE pbs);
    HRESULT _ShowAddressBand(PBANDSAVE pbs);
    HRESULT _ShowExternalBand(PBANDSAVE pbs, int idBand );
    HRESULT _ShowLinks(PBANDSAVE pbs);
    HRESULT _ShowBrand(PBANDSAVE pbs);
    HRESULT _ShowMenu(PBANDSAVE pbs);
    void _ShowBandCommon(PBANDSAVE pbs, LPBANDITEMDATA pbid, BOOL fShow);
    void _EnsureAllBandsShown();
    HRESULT _GetMinRowHeight();

    HBITMAP _LoadBackBmp(LPTSTR * ppszBitmap, BMPCACHE * pbmpCache, BOOL fInternet);
    HBITMAP _LoadBackBitmap();
    void    _SetBackground();
    void    _CommonHandleFileSysChange(LONG lEvent, LPITEMIDLIST* ppidl);
    LPITEMIDLIST _GetCurrentPidl(void);
    int     _ConvertHwndToID(HWND hwnd);

    HRESULT _GetPersistedBand(const CLSID clsid, REFIID riid, void ** ppiface);

    // Multiple command target
    LRESULT _AddBitmapFromForeignModule(UINT uiGetMSG, UINT uiSetMSG, UINT uiCount, HINSTANCE hinst,
                                        UINT_PTR nID, COLORREF rgbMask);

    HRESULT _LoadDefaultSettings(void);
    HRESULT _LoadUpgradeSettings(ULONG cbRead);
    HRESULT _LoadDefaultWidths(void);
    void _TryLoadIE3Settings();
    HRESULT _UpdateToolbarDisplay(DWORD dwFlags, UINT uVisibleBands, BOOL fNoText, BOOL fPersist);
    void _UpdateBrandSize();
    void _ShowVisible(DWORD dwVisible, BOOL fPersist);
    void _BuildSaveStruct(COOLBARSAVE* pcs);
    void _RestoreSaveStruct(COOLBARSAVE* pcs);
    void _GetVisibleBrowserBar(UINT idBar, CLSID *pclsidOut);

    LPBANDITEMDATA _AddNewBand(IDeskBand* pdb, DWORD dwID);

    void _TheaterModeLayout(BOOL fEnter);

    HBITMAP          _bmpBack; // this is the state we think the itbar is in
    static BMPCACHE  s_bmpBackShell; // this is the state of the shell bmp cache
    static BMPCACHE  s_bmpBackInternet; // this is the state of the internet bmp cache
    static IMLCACHE  s_imlTBGlyphs;

    HWND            _hwndMenu;
    HWND            _hwndAddressBand;

    IDockingWindowSite* _ptbsite;
    IOleCommandTarget*  _ptbsitect;
    IBrowserService2*   _pbs2;
    IServiceProvider*   _psp;
    IBandProxy *        _pbp;

    BITBOOL            _fCreatedBandProxy:1;
    BITBOOL            _fBackEnabled:1;
    BITBOOL            _fForwardEnabled:1;
    BITBOOL            _fEditEnabled:1;
    BITBOOL            _fShow:1;
    BITBOOL            _fAnimating:1;
    BITBOOL            _fCompressed:1;
    BITBOOL            _fUserNavigated :1;
    BITBOOL            _fAutoCompInitialized :1;
    BITBOOL            _fDirty:1;
    BITBOOL            _fUsingDefaultBands:1;
    BITBOOL            _fTransitionToHTML:1;
    BITBOOL            _fInitialPidlIsWeb:1;
    BITBOOL            _fTheater: 1; // are we in theater mode?  claim no border space
    BITBOOL            _fAutoHide :1;
    BITBOOL            _fRebarDragging :1;
    BITBOOL            _fShellView:1;   // are we in shell view or web view?
    BITBOOL            _fNoShowMenu:1;    // can show menu band?
    BITBOOL            _fUpdateToolbarTimer:1;
    BITBOOL            _fNeedUpdateToolbar:1;
    BITBOOL            _fNavigateComplete:1;
    BITBOOL            _fLoading:1;     // are we still loading the bar?
    BITBOOL            _fDestroyed:1;   // Did we destroy our member varibles and are shutting down? If so, don't use the varibles. (Stress bug w/messages coming in)

    UINT            _nVisibleBands;     // bitmask of which bands are visible: VBF_*

    IWebBrowser2*   _pdie;
    DWORD           _dwcpCookie;        // DIID_DWebBrowserEvents2
    int             _xCapture;
    int             _yCapture;
    // for multiple command target support
    HDSA            _hdsaTBBMPs;
    UINT            _uiMaxTBWidth;
    UINT            _uiTBTextRows;
    UINT            _uiTBDefaultTextRows;
    // search stuff
    HDPA            _hdpaFSI; // folder search items
    GUID            _guidCurrentSearch;
    GUID            _guidDefaultSearch;

    COOLBARSAVE     _cs;             //Coolbar layout info from registry!

    struct EXTERNALBANDINFO {
        CLSID       clsid;          // CLSID of the band
        LPWSTR      pwszName;       // Band name
        LPWSTR      pwszHelp;       // Band help text
    };
    EXTERNALBANDINFO _rgebi[ MAXEXTERNALBANDS ];

    void _LoadExternalBandInfo();

    TBBUTTON _tbExplorer[ARRAYSIZE(c_tbExplorer)];
    int      _iButtons;


#ifdef EDIT_HACK
    // Variables for customizing the edit button glyph
    HIMAGELIST      _himlEdit;          // Monochrome Image list for the edit button
    HIMAGELIST      _himlEditHot;       // Hot image list for edit button
    int             _iEditIcon;         // index of current edit icon

    // Functions for managing a custom edit glyph
    void _InitEditButtonStyle();
    void _SetEditGlyph(int iIcon);
    void _RefreshEditGlyph();
    void _UpdateEditButton();
    static HIMAGELIST _CreateGrayScaleImagelist(HBITMAP hbmpImage, HBITMAP hbmpMask);
    static BSTR _GetEditProgID(IHTMLDocument2* pHTMLDocument);

    //
    // We can have multiple edit verbs associated with a document.  The following class
    // maintains a list of verbs.
    //
    #define FCIDM_EDITFIRST  2000
    #define FCIDM_EDITLAST   2100
    #define SZ_EDITVERB_PROP  TEXT("CEditVerb_This")
    #define IL_EDITBUTTON 2     // Index of image list used for the edit button
    #define IL_SEARCHBUTTON 3   //                   ||             search button

    // MSAA Menu Info declarations.
    // These will eventually be incorporated into oleacc.h - but for the
    // moment, we declare them privately...
    #define MSAA_MENU_SIG  0xAA0DF00DL

    class CEditVerb
    {
    public:
        CEditVerb();
        ~CEditVerb();

        // Functions for managing the verbs
        BOOL Add(LPTSTR pszProgID);
        UINT GetSize() { return _nElements; }
        void RemoveAll();

        // Functions to access the default edit verb
        int   GetIcon() { return (_nElements && _pVerb[_nDefault].fShowIcon) ? _GetVerb(_nDefault).iIcon : -1; }
        BOOL  GetToolTip(LPTSTR pszToolTip, UINT cchMax, BOOL fStripAmpersands = TRUE);
        BOOL  GetMenuText(LPTSTR pszText, UINT cchMax) { return GetToolTip(pszText, cchMax, FALSE); }
        void  Edit(LPCTSTR pszUrl) { _Edit(pszUrl, _nDefault); }

        // Pop-up menu
        BOOL ShowEditMenu(POINT pt, HWND hwnd, LPTSTR pszUrl);

        // Get default editor from the registry
        void InitDefaultEditor(HKEY hkey = NULL);

    protected:
        struct MSAAMenuInfo
        {
            DWORD m_MSAASig;  // Must be MSAA_MENU_SIG
            DWORD m_CharLen;  // Length in characters of text, excluding terminating NUL
            LPWSTR m_pWStr;   // Menu text, in UNICODE, with terminating UNICODE-NUL.
        };

        struct EDITVERB
        {
            MSAAMenuInfo m_MSAA;     // MSAA info - must be first element.
            HKEY    hkeyProgID;      // Key the we shellexec
            BITBOOL fUseOpenVerb:1;  // use open verb instead of edit
            BITBOOL fInit:1;         // true if the data below has beed initialized
            BITBOOL fShowIcon:1;     // true if icon should show up on button
            int     iIcon;           // cached icon index
            UINT    idCmd;           // menu id
            LPTSTR  pszDesc;         // executable name or document name
            LPTSTR  pszMenuText;     // Menu text
            LPTSTR  pszExe;          // Path of the exe used to edit
        };

        EDITVERB* _Add(HKEY hkeyProgID, BOOL fPermitOpenVerb, BOOL fCheckForOfficeApp, BOOL fShowIcon);
        EDITVERB& _GetVerb(UINT nIndex);
        void      _FetchInfo(UINT nIndex);
        void      _Edit(LPCTSTR pszUrl, UINT nIndex);
        LPCTSTR   _GetDescription(EDITVERB& rVerb);
        void      _SetMSAAMenuInfo(EDITVERB& rVerb);
        void      _ClearMSAAMenuInfo(EDITVERB& rVerb);
        void      _FormatMenuText(UINT nIndex);
        BOOL      _IsUnique(EDITVERB& rNewVerb);
        BOOL      _IsHtmlStub(LPCWSTR pszPath);
        LPCTSTR   _GetExePath(EDITVERB& rVerb);
        LPCTSTR   _GetDefaultEditor();

        static LRESULT CALLBACK _WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        // Member data
        UINT        _nElements;         // number of edit verbs
        UINT        _nDefault;          // Default edit verb
        EDITVERB*   _pVerb;             // array of edit verbs
        WNDPROC     _lpfnOldWndProc;    // former wndProc
        LPWSTR      _pszDefaultEditor;  // Friendly name of default HTML editor
        BOOL        _fInitEditor;       // if we checked for a default editor
    };
    CEditVerb  _aEditVerb;

#endif

    // internal bandsite class
    class CBrowserToolsBand;
    class CITBandSite : public CBandSite
    {
        CITBandSite();

        virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
        virtual STDMETHODIMP AddBand(IUnknown *punk);
        virtual STDMETHODIMP HasFocusIO();

    protected:
        virtual void v_SetTabstop(LPREBARBANDINFO prbbi);
        BOOL _SetMinDimensions();
        friend class CInternetToolbar;
        friend class CBrowserToolsBand;

        virtual HRESULT _OnContextMenu(WPARAM wParm, LPARAM lParam);
        virtual HRESULT _Initialize(HWND hwndParent);

    };
    CITBandSite _bs;


#define TOOLSBANDCLASS CInternetToolbar::CBrowserToolsBand
    class CBrowserToolsBand : public CToolbarBand
    {
        CMDMAP* _GetCmdMapByIndex(int nIndex) { return _GetCmdMap(nIndex, TRUE);};
        CMDMAP* _GetCmdMapByID(int id)  { return _GetCmdMap(id, FALSE);};
        LRESULT _ToolsCustNotify (LPNMHDR pnmh);  // Handle TBCustomization Notify
        BOOL _SaveRestoreToolbar(BOOL fSave);
        void _FreeCustomizeInfo();
        void _FreeCmdMap(CMDMAP*);
        BOOL _RemoveAllButtons();
        int _CommandFromIndex(UINT uIndex);
        HRESULT _ConvertCmd(const GUID* pguidButtonGroup, UINT id, GUID* pguidOut, UINT * pid);
        void _OnDeletingButton(TBNOTIFY* ptbn);
        LONG_PTR _AddString(LPWSTR pwstr);
        void _PreProcessButtonString(TBBUTTON *ptbn, DWORD dwFlags);
        void _PreProcessExternalTBButton(TBBUTTON *ptbn);
        UINT _ProcessExternalButtons(PTBBUTTON ptbb, UINT cButtons);
        void _GetButtons(IOleCommandTarget* pct, const GUID* pguid, HDSA hdsa);
        void _RecalcButtonWidths();

        void            _UpdateTextSettings(INT_PTR ids);
        static BOOL_PTR CALLBACK _BtnAttrDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static void     _PopulateComboBox(HWND hwnd, const int iResource[], UINT cResources);
        static void     _SetComboSelection(HWND hwnd, int iCurOption);
        void            _SetDialogSelections(HWND hDlg, BOOL fSmallIcons);
        static void     _PopulateDialog(HWND hDlg);
        void            _OnBeginCustomize(LPNMTBCUSTOMIZEDLG pnm);

        BOOL _BuildButtonDSA();
        CMDMAPCUSTOMIZE* _GetCmdMapCustomize(GUID* guid, UINT nCmdID);

        virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

        virtual STDMETHODIMP GetClassID(CLSID *pClassID) {return E_NOTIMPL;};
        virtual STDMETHODIMP Load(IStream *pStm) {return E_NOTIMPL;};
        virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty) {return E_NOTIMPL;};

        // *** IUnknown ***
        virtual STDMETHODIMP_(ULONG) AddRef(void) { return CToolBand::AddRef(); };
        virtual STDMETHODIMP_(ULONG) Release(void){ return CToolBand::Release(); };
        virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

        // *** IDeskBand methods ***
        virtual STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD fViewMode, DESKBANDINFO* pdbi);

        // *** IWinEventHandler methods ***
        virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres);

        // *** IDockingWindow methods ***
        virtual STDMETHODIMP CloseDW(DWORD dwReserved) { return S_OK;};

        // *** IInputObject methods ***
        virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);

    protected:
        IOleCommandTarget* _CommandTargetFromCmdMap(CMDMAP* pcm);
        LRESULT _OnToolbarDropDown(TBNOTIFY *ptbn);
        virtual LRESULT _OnNotify(LPNMHDR pnmh);
        LRESULT _OnContextMenu(LPARAM lParam, WPARAM wParam);
        CMDMAP* _GetCmdMap(int i, BOOL fByIndex);
        void _OnEndCustomize();
        LRESULT _TryShowBackForwardMenu(DWORD dwItemSpec, LPPOINT ppt, LPRECT prcExclude);
        CBrowserToolsBand();
        void _FreeBtnsAdded();

        friend class CInternetToolbar;
        friend class CITBandSite;

        GUID            _guidCurrentButtonGroup;
        IOleCommandTarget* _pctCurrentButtonGroup;
        LPTBBUTTON      _pbtnsAdded;
        int             _cBtnsAdded;
        DWORD            _nNextCommandID;
        CUSTOMIZEINFO *_pcinfo;
        BITBOOL    _fCustomize :1;
        BITBOOL    _fNeedFreeCmdMapsAdded :1;
    };

    CBrowserToolsBand _btb;

    friend class CBrowserToolsBand;
    friend class CITBandSite;
    friend void CInternetToolbar_CleanUp();
    friend void CInternetToolbar_Preload();
    friend void ITBar_LoadToolbarGlyphs(HWND hwnd);
};

//
// Gets the stream corresponding to the type of the given pidl
//     If the stream already doesn't exist, then it returns NULL.

HRESULT _GetStreamName(DWORD dwITBS, LPTSTR pszName, DWORD cchSize)
{
    HRESULT hr = S_OK;

    ASSERT(pszName);

    switch (dwITBS)
    {
    case ITBS_WEB:
        StrCpyN(pszName, TEXT("WebBrowser"), cchSize);
        break;

    case ITBS_SHELL:
        StrCpyN(pszName, TEXT("ShellBrowser"), cchSize);
        break;

    case ITBS_EXPLORER:
        StrCpyN(pszName, TEXT("Explorer"), cchSize);
        break;

    default:
        hr = E_FAIL;
        break;
    }

    if (FAILED(hr))
        pszName[0] = '\0';

    return hr;
}


//
// Gets the stream corresponding to the type of the given pidl
//     If the stream already doesn't exist, then it returns NULL.

IStream *GetRegStream(BOOL fInternet, LPCTSTR pszValue, DWORD grfMode)
{
    IStream *pstm = NULL;
    HKEY    hkToolbar;

    if (RegCreateKey(HKEY_CURRENT_USER, c_szRegKeyCoolbar, &hkToolbar) == ERROR_SUCCESS)
    {
        TCHAR   szStreamName[MAX_PATH];

        if (SUCCEEDED(_GetStreamName(fInternet, szStreamName, ARRAYSIZE(szStreamName))))
            pstm = OpenRegStream(hkToolbar, szStreamName, pszValue, grfMode);

        RegCloseKey(hkToolbar);
    }

    return(pstm);
}


//
// Gets the stream corresponding to the type of the given pidl
//     If the stream already doesn't exist, then it returns NULL.

IStream *GetITBarStream(BOOL fInternet, DWORD grfMode)
{
    return GetRegStream(fInternet, TEXT("ITBarLayout"), grfMode);
}


IMLCACHE CInternetToolbar::s_imlTBGlyphs = {NULL};
BMPCACHE CInternetToolbar::s_bmpBackShell = {NULL};
BMPCACHE CInternetToolbar::s_bmpBackInternet = {NULL};
BOOL g_fSmallIcons = FALSE;

void IMLCACHE_CleanUp(IMLCACHE * pimlCache, DWORD dwFlags)
{
    for (int i = 0; i < CIMLISTS; i++)
    {
        if (pimlCache->arhimlPendingDelete[i])
            ImageList_Destroy(pimlCache->arhimlPendingDelete[i]);

        if ((dwFlags & IML_DESTROY) && pimlCache->arhiml[i])
            ImageList_Destroy(pimlCache->arhiml[i]);
    }
}

void ITBar_LoadToolbarGlyphs(HWND hwnd)
{
    int cx, idBmp;

    g_fSmallIcons = _UseSmallIcons();

    if (g_fSmallIcons) {
        cx = TB_SMBMP_CX;
        idBmp = IDB_IETOOLBAR16;
    } else {
        cx = TB_BMP_CX;
        idBmp = IDB_IETOOLBAR;
    }

    if (SHGetCurColorRes() > 8)
        idBmp += DELTA_HICOLOR;

    _LoadToolbarGlyphs(hwnd, &CInternetToolbar::s_imlTBGlyphs, cx, idBmp);
}


void CInternetToolbar_Preload()
{
   ENTERCRITICAL;
   ITBar_LoadToolbarGlyphs(NULL);
   Brand_InitBrandContexts();
   LEAVECRITICAL;
}


void CInternetToolbar_CleanUp()
{
    TraceMsg(DM_ITBAR, "CInternetToolbar: Destroying shared GDI objects");
    if (CInternetToolbar::s_bmpBackInternet.hbmp)
        DeleteObject(CInternetToolbar::s_bmpBackInternet.hbmp);
    if (CInternetToolbar::s_bmpBackShell.hbmp)
        DeleteObject(CInternetToolbar::s_bmpBackShell.hbmp);

    IMLCACHE_CleanUp(&CInternetToolbar::s_imlTBGlyphs, IML_DESTROY);
}

STDAPI CInternetToolbar_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    CInternetToolbar *pitbar = new CInternetToolbar();
    if (pitbar)
    {
        *ppunk = SAFECAST(pitbar, IDockingWindow *);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

LRESULT CInternetToolbar::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ( uMsg == WM_SYSCOLORCHANGE )
    {
        // refresh the back drop incase the colours have changed
        _SetBackground();
    }

    return SUPERCLASS::v_WndProc( hwnd, uMsg, wParam, lParam );
}

void CInternetToolbar::_LoadExternalBandInfo()
{
#ifdef DEBUG
    int i;
    // Should have been zero-initialized
    for (i = 0; i < ARRAYSIZE(_rgebi); i++)
    {

        ASSERT(IsEqualGUID(_rgebi[i].clsid, GUID_NULL));
        ASSERT(_rgebi[i].pwszName == NULL);
        ASSERT(_rgebi[i].pwszHelp == NULL);
    }
#endif

    HKEY hkey;
    DWORD dwClsidIndex = 0;
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_KEY_BANDSTATE, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        TCHAR tszReg[MAX_PATH];
        StrCpy(tszReg, TEXT("CLSID\\"));
        const int cchClsidPrefix = 6;      // 6 = strlen("CLSID\\")
        LPTSTR ptszClsid = tszReg + cchClsidPrefix;
        DWORD cchClsid;
        for (DWORD dwIndex = 0;
             cchClsid = ARRAYSIZE(tszReg) - cchClsidPrefix,
             dwClsidIndex < ARRAYSIZE(_rgebi) &&
             RegEnumValue( hkey, dwIndex, ptszClsid, &cchClsid, NULL, NULL, NULL, NULL ) == ERROR_SUCCESS;
             dwIndex++)
        {
            CLSID clsid;
            if (GUIDFromString( ptszClsid, &clsid ))
            {
                // Don't save the CLSID until we're sure it worked
                _rgebi[dwClsidIndex].clsid = clsid;

                HKEY hkeyClsid;
                if (RegOpenKeyEx(HKEY_CLASSES_ROOT, tszReg, 0, KEY_READ, &hkeyClsid) == ERROR_SUCCESS)
                {
                    WCHAR wszBuf[MAX_PATH];

                    // Get the name; use SHLoadRegUIString so the app can localize
                    SHLoadRegUIStringW( hkeyClsid, L"", wszBuf, ARRAYSIZE(wszBuf) );
                    Str_SetPtrW( &_rgebi[dwClsidIndex].pwszName, wszBuf);

                    // Get the help; use SHLoadRegUIString so the app can localize
                    SHLoadRegUIStringW( hkeyClsid, L"HelpText", wszBuf, ARRAYSIZE(wszBuf) );
                    Str_SetPtrW( &_rgebi[dwClsidIndex].pwszHelp, wszBuf);

                    RegCloseKey(hkeyClsid);
                }

                dwClsidIndex++;
            }
        }
        RegCloseKey( hkey );
    }
}

CInternetToolbar::CInternetToolbar() : CBaseBar(), _yCapture(-1), _iButtons(-1)
#ifdef EDIT_HACK
, _iEditIcon(-1)
#endif
{

    DllAddRef();

    if (GetSystemMetrics(SM_CXSCREEN) < 650)
        _uiMaxTBWidth = MAX_TB_WIDTH_LORES;
    else
        _uiMaxTBWidth = MAX_TB_WIDTH_HIRES;

    ASSERT(_fLoading == FALSE);
    ASSERT(_btb._guidCurrentButtonGroup == CLSID_NULL);
    _btb._nNextCommandID = 1000;

    _LoadExternalBandInfo();
}

void CInternetToolbar::_Unadvise(void)
{
    if(_dwcpCookie)
    {
        ConnectToConnectionPoint(NULL, DIID_DWebBrowserEvents2, FALSE, _pdie, &_dwcpCookie, NULL);
    }
}

int CALLBACK DeleteDPAPtrCB(void *pItem, void *pData)
{
    LocalFree(pItem);
    return TRUE;
}

CInternetToolbar::~CInternetToolbar()
{
    ATOMICRELEASE(_pdie);

    if(_pbp && _fCreatedBandProxy)
        _pbp->SetSite(NULL);

    ATOMICRELEASE(_pbp);

    ASSERT(!_ptbsite && !_ptbsitect && !_psp && !_pbs2);
    SetSite(NULL);

    DPA_DestroyCallback(_hdpaFSI, DeleteDPAPtrCB, NULL);

    for (int i = 0; i < ARRAYSIZE(_rgebi); i++)
    {
        Str_SetPtrW( &_rgebi[i].pwszName, NULL);
        Str_SetPtrW( &_rgebi[i].pwszHelp, NULL);
    }

    TraceMsg(TF_SHDLIFE, "dtor CInternetToolbar %x", this);
    DllRelease();
}

#define IID_DWebBrowserEvents DIID_DWebBrowserEvents


HRESULT CInternetToolbar::QueryInterface(REFIID riid, void ** ppvObj)
{
    static const QITAB qit[] = {
        // perf: last tuned 980728
        QITABENTMULTI(CInternetToolbar, IDispatch, DWebBrowserEvents),  // IID_IDispatch
        QITABENT(CInternetToolbar, IExplorerToolbar),       // IID_IDispatch
        QITABENT(CInternetToolbar, IObjectWithSite),        // IID_IObjectWithSite
        QITABENT(CInternetToolbar, IPersistStreamInit),     // IID_IPersistStreamInit
        QITABENT(CInternetToolbar, IDockingWindow),         // IID_IDockingWindow
        QITABENT(CInternetToolbar, DWebBrowserEvents),      // IID_DWebBrowserEvents
        QITABENT(CInternetToolbar, IShellChangeNotify),     // rare IID_IShellChangeNotify
        QITABENT(CInternetToolbar, ISearchItems),           // rare IID_ISearchItems
        { 0 },
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hres))
        hres = SUPERCLASS::QueryInterface(riid, ppvObj);

    return hres;
}

/* IDispatch methods */
HRESULT CInternetToolbar::GetTypeInfoCount(UINT *pctinfo)
{
    return(E_NOTIMPL);
}

HRESULT CInternetToolbar::GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo **pptinfo)
{
    return(E_NOTIMPL);
}

HRESULT CInternetToolbar::GetIDsOfNames(REFIID riid,OLECHAR **rgszNames,UINT cNames,
                                        LCID lcid, DISPID * rgdispid)
{
    return(E_NOTIMPL);
}

#if 0
//  BUGBUG - StevePro changed it so this code isnt called
//  this is a goodness, because it calls SHVerbExists() which
//  is a TCHAR API, that is actually compiled as an ANSI API
//  and since we are UNICODE it just always fails.
//  leaving this in so that we know about the issue of
//  frontpad.exe possibly needing to be disabled.
BOOL _ShowEditForExtension(LPCTSTR pszExtension)
{
    TCHAR szBuf[MAX_PATH];
    if (SHVerbExists(pszExtension, TEXT("edit"), szBuf)) {
        // don't show it if it's just our own
        if (StrStrI(szBuf, TEXT("frontpad.exe"))) {
            return FALSE;
        }
        return TRUE;
    }

    return FALSE;
}
#endif

#ifdef EDIT_HACK
//+-------------------------------------------------------------------------
// This function scans the html document for META tags that indicate the
// program that was used to create the HTML page.  Examples are:
//
//  <meta name="ProgID" content="word.document" >
//  <meta name="ProgID" content="excel.sheet" >
//
// If a match is found, the content of the first match is returned.  This
// progid is used to edit the document.
//--------------------------------------------------------------------------
BSTR CInternetToolbar::_GetEditProgID
(
    IHTMLDocument2* pHTMLDocument
)
{
    BSTR bstrProgID = NULL;

    //
    // First get all document elements.  Note that this is very fast in
    // ie5 because the collection directly accesses the internal tree.
    //
    IHTMLElementCollection * pAllCollection;
    if (SUCCEEDED(pHTMLDocument->get_all(&pAllCollection)))
    {
        IHTMLMetaElement* pMetaElement;
        IHTMLBodyElement* pBodyElement;
        IHTMLFrameSetElement* pFrameSetElement;
        IDispatch* pDispItem;

        //
        // Now we scan the document for meta tags.  Since these must reside in
        // in the head, and since Trident always creates a body tag, we can
        // stop looking when we hit the body.
        //
        // Note, the alternative of using pAllCollection->tags to return the
        // collection of meta tags is likely more expensive because it will
        // walk the whole tree (unless Trident optimizes this).
        //
        long lItemCnt;
        VARIANT vEmpty;
        V_VT(&vEmpty) = VT_EMPTY;

        VARIANT vIndex;
        V_VT(&vIndex) = VT_I4;

        EVAL(SUCCEEDED(pAllCollection->get_length(&lItemCnt)));

        for (long lItem = 0; lItem < lItemCnt; lItem++)
        {
            V_I4(&vIndex) = lItem;

            if (S_OK == pAllCollection->item(vIndex, vEmpty, &pDispItem))
            {
                //
                // First see if it's a meta tag
                //
                if (SUCCEEDED(pDispItem->QueryInterface(IID_IHTMLMetaElement,
                                                    (void **)&pMetaElement)))
                {
                    BSTR bstrName = NULL;

                    //
                    // We have a META element, check its NAME and CONTENT
                    //
                    if ( SUCCEEDED(pMetaElement->get_name(&bstrName)) && (bstrName != NULL) &&
                         (StrCmpIW(bstrName, OLESTR("ProgId")) == 0) &&
                         SUCCEEDED(pMetaElement->get_content(&bstrProgID)) && (bstrProgID != NULL)
                       )
                    {
                        // We got the ProgID, so terminate the search;
                        lItem = lItemCnt;
                    }

                    if (bstrName != NULL)
                        SysFreeString(bstrName);

                    pMetaElement->Release();
                }
                //
                // Next check for the body tag
                //
                else if (SUCCEEDED(pDispItem->QueryInterface(IID_IHTMLBodyElement,
                                                    (void **)&pBodyElement)) )
                {
                    // Found the body tag, so terminate the search
                    lItem = lItemCnt;
                    pBodyElement->Release();
                }
                //
                // Finally, check for a frameset tag
                //
                else if (SUCCEEDED(pDispItem->QueryInterface(IID_IHTMLFrameSetElement,
                                                    (void **)&pFrameSetElement)) )
                {
                    // Found a frameset tag, so terminate the search
                    lItem = lItemCnt;
                    pFrameSetElement->Release();
                }
                pDispItem->Release();
            }
        }
        // Make sure that these don't have to be cleared (should not have been modified)
        ASSERT(vEmpty.vt == VT_EMPTY);
        ASSERT(vIndex.vt == VT_I4);

        pAllCollection->Release();
    }

    return bstrProgID;
}

//+-------------------------------------------------------------------------
// Returns grey-scale image from the icon passed in.
//--------------------------------------------------------------------------
HIMAGELIST CInternetToolbar::_CreateGrayScaleImagelist(HBITMAP hbmpImage, HBITMAP hbmpMask)
{
    // Determine the button dimensions
    int cx = g_fSmallIcons ? TB_SMBMP_CX : TB_BMP_CX;
    int cy = g_fSmallIcons ? TB_SMBMP_CY : TB_BMP_CY;

    // Start with a 24 bit color image list
    HIMAGELIST himlEdit = ImageList_Create(cx, cy, ILC_COLOR24 | ILC_MASK, 1, 1);
    if (NULL == himlEdit)
    {
        return NULL;
    }

    ImageList_Add(himlEdit, hbmpImage, hbmpMask);

    // Get the dib section from the image list
    IMAGEINFO ii;
    if (ImageList_GetImageInfo(himlEdit, 0, &ii))
    {
        DIBSECTION ds = {0};
        if (GetObject(ii.hbmImage, sizeof(ds), &ds))
        {
            //
            // Map each pixel to a monochrome equivalent.
            //
            BYTE* pBits = (BYTE*)ds.dsBm.bmBits;
            BYTE* pScan = pBits;
            int xWid = ds.dsBm.bmWidth;
            int yHei = ds.dsBm.bmHeight;
            long cbScan = ((xWid * 24 + 31) & ~31) / 8;

            for (int y=0; y < yHei; ++y)
            {
                for (int x=0; x < xWid; ++x)
                {
                    //
                    // Map to equivalent gray color by setting r,g,b to the same value.
                    // Using the average of r,g,b can be too dark, and using the max
                    // of r,g,b can be too bright.  So, as a simple algorithm we use
                    // the average of the two schemes.  This is cheaper than using true
                    // intensity matching.
                    //
                    BYTE nMax = max(max(pScan[0], pScan[1]), pScan[2]);
                    BYTE nAve = ((UINT)pScan[0] + pScan[1] + pScan[2])/3;
                    pScan[0] = pScan[1] = pScan[2] = ((UINT)nMax + nAve)/2;

                    // Increment to next pixel
                    pScan += 3;
                }

                // Increment to the next scan line
                pBits += cbScan;
                pScan = pBits;
            }
        }
    }
    return himlEdit;
}

//+-------------------------------------------------------------------------
// Returns image and mask bitmaps for the desired image list item
//--------------------------------------------------------------------------
BOOL MyImageList_GetBitmaps
(
    HIMAGELIST himl,        // image list to use
    int iImage,             // image to copy
    int x,                  // x-offset to draw in bitmap
    int y,                  // x-offset to draw in bitmap
    int cx,                 // width of bitmap
    int cy,                 // height of bitmap
    HBITMAP* phbmpImage,    // returned color bitmap
    HBITMAP* phbmpMask      // returned mask bitmap
)
{
    ASSERT(phbmpImage);
    ASSERT(phbmpMask);

    BOOL fRet = FALSE;
    HDC hdc = GetDC(NULL);
    HDC hdcDst = CreateCompatibleDC(hdc);
    if (hdcDst)
    {
        HBITMAP hbmpImage = CreateCompatibleBitmap(hdc, cx, cy);
        if (hbmpImage)
        {
            HBITMAP hbmpMask = CreateBitmap(cx, cy, 1, 1, NULL);
            if (hbmpMask)
            {
                // Draw  mask bitmap
                HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcDst, hbmpMask);
                PatBlt(hdcDst, 0, 0, cx, cy, WHITENESS);
                ImageList_Draw(himl, iImage, hdcDst, x, y, ILD_MASK);

                // Draw image bitmap
                SelectObject(hdcDst, hbmpImage);
                ImageList_Draw(himl, iImage, hdcDst, x, y, ILD_NORMAL);

                SelectObject(hdcDst, hbmpOld);

                *phbmpImage = hbmpImage;
                *phbmpMask  = hbmpMask;
                fRet = TRUE;
            }
            else
            {
                DeleteObject(hbmpImage);
            }
        }
        DeleteDC(hdcDst);
    }
    ReleaseDC(NULL, hdc);

    return fRet;
}
extern HBITMAP CreateMirroredBitmap( HBITMAP hbmOrig);

//+-------------------------------------------------------------------------
// Creates a special image list for the edit button and configures the edit
// button to use it.  If the hIcon is -1, the edit button is reset to use
// it's default glyph.
//--------------------------------------------------------------------------
void CInternetToolbar::_SetEditGlyph
(
    int iIcon   // new edit button glyph, index into shell image cache
)
{
    // If no toolbar, we just need to see if we need to free the old image lists.
    if (_btb._hwnd == NULL)
    {
        if (iIcon == -1)
        {
            if (_himlEdit)
            {
                ImageList_Destroy(_himlEdit);
                _himlEdit = NULL;
            }
            if (_himlEditHot)
            {
                ImageList_Destroy(_himlEditHot);
                _himlEditHot = NULL;
            }
        }
        else
        {
            // Can't set the glyph if no toolbar!
            ASSERT(FALSE);
        }
        return;
    }

    UINT uiCmd = -1;
    // Dochost merges under one of two clsids, so have to check both
    if (FAILED(_btb._ConvertCmd(&CLSID_InternetButtons, DVIDM_EDITPAGE, NULL, &uiCmd)) &&
        FAILED(_btb._ConvertCmd(&CLSID_MSOButtons, DVIDM_EDITPAGE, NULL, &uiCmd)))
    {
        // The edit button is not on toolbar, so free the edit glyphs
        iIcon = -1;
    }

    // If the current icon is already set, we are done
    if (_iEditIcon == iIcon)
    {
        if (_himlEdit)
        {
            // Set up the new image lists
            SendMessage(_btb._hwnd, TB_SETIMAGELIST, IL_EDITBUTTON, (LPARAM)_himlEdit);
            if (_himlEditHot)
            {
                SendMessage(_btb._hwnd, TB_SETHOTIMAGELIST, IL_EDITBUTTON, (LPARAM)_himlEditHot);
            }

            // Redirect the edit button to the new image list
            TBBUTTONINFO tbi = {0};
            tbi.cbSize = sizeof(tbi);
            tbi.dwMask = TBIF_IMAGE;
            tbi.iImage = MAKELONG(0, IL_EDITBUTTON);

            SendMessage(_btb._hwnd, TB_SETBUTTONINFO, uiCmd, (LPARAM)&tbi);
        }
        return;
    }

    _iEditIcon = iIcon;

    if (-1 == iIcon)
    {
        if (_himlEdit)
        {
            if (uiCmd != -1)
            {
                // Reset to the original edit glyph
                TBBUTTONINFO tbi = {0};
                tbi.cbSize = sizeof(tbi);
                tbi.dwMask = TBIF_IMAGE;
                tbi.iImage = EDITGLYPH_OFFSET;
                SendMessage(_btb._hwnd, TB_SETBUTTONINFO, uiCmd, (LPARAM)&tbi);
            }

            // Destroy the custom edit glyphs.  Note that we have to reset the primary image list
            // or the image sizes are screwed up.
            SendMessage(_btb._hwnd, TB_SETIMAGELIST, IL_EDITBUTTON, (LPARAM)NULL);
            ImageList_Destroy(_himlEdit);
            _himlEdit = NULL;
        }

        if (_himlEditHot)
        {
            SendMessage(_btb._hwnd, TB_SETHOTIMAGELIST, IL_EDITBUTTON, (LPARAM)NULL);
            ImageList_Destroy(_himlEditHot);
            _himlEditHot = NULL;
        }
    }
    else
    {
        // Determine the button dimensions
        int cx = g_fSmallIcons ? TB_SMBMP_CX : TB_BMP_CX;
        int cy = g_fSmallIcons ? TB_SMBMP_CY : TB_BMP_CY;

        // Get the image bitmaps
        HBITMAP hbmpImage = NULL;
        HBITMAP hbmpMask = NULL;
        BOOL bMirrored = IS_WINDOW_RTL_MIRRORED(_btb._hwnd);
        HIMAGELIST himlSmall;
        int cxSmall;
        int cySmall;

        if (Shell_GetImageLists(NULL, &himlSmall) &&
            ImageList_GetIconSize(himlSmall, &cxSmall, &cySmall) &&
            MyImageList_GetBitmaps(himlSmall, iIcon, (cx - cxSmall)/2, (cy - cySmall)/2,
                                   cx, cy, &hbmpImage, &hbmpMask))
        {
            if (bMirrored) 
            {
                HBITMAP hbmpTemp;

                hbmpTemp = CreateMirroredBitmap(hbmpImage);
                if (hbmpTemp)
                {
                    DeleteObject(hbmpImage);
                    hbmpImage = hbmpTemp;
                }
                hbmpTemp = CreateMirroredBitmap(hbmpMask);
                if (hbmpTemp)
                {
                    DeleteObject(hbmpMask);
                    hbmpMask = hbmpTemp;
                }
            }
            // Create a monochrome glyph for the edit button
            HIMAGELIST himlEdit = _CreateGrayScaleImagelist(hbmpImage, hbmpMask);
            SendMessage(_btb._hwnd, TB_SETIMAGELIST, IL_EDITBUTTON, (LPARAM)himlEdit);
            if (_himlEdit)
            {
                ImageList_Destroy(_himlEdit);
            }
            _himlEdit = himlEdit;

            // Create a hot glyph for the edit button
            HIMAGELIST himlEditHot = ImageList_Create(cx, cy, ILC_COLORDDB | ILC_MASK, 1, 1);
            int nIndex = ImageList_Add(himlEditHot, hbmpImage, hbmpMask);

            SendMessage(_btb._hwnd, TB_SETHOTIMAGELIST, IL_EDITBUTTON, (LPARAM)himlEditHot);
            if (_himlEditHot)
            {
                ImageList_Destroy(_himlEditHot);
            }
            _himlEditHot = himlEditHot;

            // Redirect the edit button to the new image list
            if (_himlEdit)
            {
                TBBUTTONINFO tbi = {0};
                tbi.cbSize = sizeof(tbi);
                tbi.dwMask = TBIF_IMAGE;
                tbi.iImage = MAKELONG(nIndex, IL_EDITBUTTON);

                SendMessage(_btb._hwnd, TB_SETBUTTONINFO, uiCmd, (LPARAM)&tbi);
            }

            DeleteObject(hbmpImage);
            DeleteObject(hbmpMask);
        }
        else
        {
            // Couldn't create images so use the default edit glyph
            _SetEditGlyph(-1);
        }
    }
}

//+-------------------------------------------------------------------------
// Initializes the edit button to display a drop-down menu if there are
// multiple verbs.  Also optionally displays a custion glyph.
//--------------------------------------------------------------------------
void CInternetToolbar::_InitEditButtonStyle()
{
    // If we have or want a custon edit glyph, load it
    _SetEditGlyph(_aEditVerb.GetIcon());

    UINT uiCmd;

    // Dochost merges under one of two clsids, so have to check both
    if (SUCCEEDED(_btb._ConvertCmd(&CLSID_InternetButtons, DVIDM_EDITPAGE, NULL, &uiCmd)) ||
        SUCCEEDED(_btb._ConvertCmd(&CLSID_MSOButtons, DVIDM_EDITPAGE, NULL, &uiCmd)))
    {
        ASSERT(uiCmd != -1);

        // If multiple verbs, make the button a split button
        TBBUTTONINFO tbi = {0};
        tbi.cbSize = sizeof(tbi);
        tbi.dwMask = TBIF_STYLE | TBIF_STATE;
        tbi.fsState = 0;

        if (_aEditVerb.GetSize() > 1)
        {
            tbi.fsStyle |= BTNS_DROPDOWN;
        }

        if (_aEditVerb.GetSize() > 0)
        {
            tbi.fsState = TBSTATE_ENABLED;
        }
        SendMessage(_btb._hwnd, TB_SETBUTTONINFO, uiCmd, (LPARAM)&tbi);
    }
}

//+-------------------------------------------------------------------------
// If the edit button is displaying a custon glyph, this function reloads
// the glyph.
//--------------------------------------------------------------------------
void CInternetToolbar::_RefreshEditGlyph()
{
    // If we have a custon edit glyph, reload it
    if (_himlEdit)
    {
        // Refresh the edit glyph
        _iEditIcon = -1;
        _InitEditButtonStyle();
    }
}

//+-------------------------------------------------------------------------
// Updates the edit button based on the document type currently loaded
//--------------------------------------------------------------------------
void CInternetToolbar::_UpdateEditButton()
{
    _aEditVerb.RemoveAll();
    _fEditEnabled = FALSE;

    //
    // First add editors associated with the url
    //
    BSTR bstrUrl = NULL;
    _pdie->get_LocationURL(&bstrUrl);
    if (bstrUrl)
    {
        LPTSTR pszExt;
        //
        // Find the cache file associated with the url.  The file extension for this entry
        // is based off of the mime type. (Note that get_mimeType on the document
        // returns a frindly name that is hard to translate back to an actual mimetype.
        // So we use the file extension instead.)
        //
        WCHAR szCacheFileName[MAX_PATH];
        *szCacheFileName = 0;
        if (FAILED(URLToCacheFile(bstrUrl, szCacheFileName, ARRAYSIZE(szCacheFileName))))
        {
            // If we can't get a file associated with the url, probably want to disable the edit button
            // because most apps need a file to edit.
            SysFreeString(bstrUrl);
            return;
        }

        pszExt = PathFindExtension(szCacheFileName);

        // bug 79055 - The cache has a bug where some html entries are not
        // given a file extension.  Too risky to fix for 5.x, so we'll just
        // assume .htm for http if no extension is present.
        if (L'\0' == *pszExt && GetUrlScheme(bstrUrl) == URL_SCHEME_HTTP)
        {
            StrCpyN(szCacheFileName, L".htm", ARRAYSIZE(szCacheFileName));
            pszExt = szCacheFileName;
        }

        if (*pszExt)
        {
            _aEditVerb.Add(pszExt);

            // If ".html", use the ".htm" editors too
            if (StrCmpI(pszExt, L".html") == 0 )
            {
                //  This is an html document, so add the .htm editors
                if (!_aEditVerb.Add(TEXT(".htm")) && StrCmpI(pszExt, L".html") != 0)
                {
                    _aEditVerb.Add(TEXT(".html"));
                }
            }
        }

        SysFreeString(bstrUrl);
    }

    //
    // See if the feature to search the doc for the progid is enabled
    //
    static int fCheckDocForProgID = -1;
    if (fCheckDocForProgID == -1)
    {
        fCheckDocForProgID = SHRegGetBoolUSValue(REGSTR_PATH_MAIN,
                 TEXT("CheckDocumentForProgID"), FALSE, TRUE) ? 1 : 0;
    }

    // Check for a meta tag that specifies a progid for editing this document
    if (fCheckDocForProgID)
    {
        //
        // Next see if this is an html document with a progid
        //
        IWebBrowser2*       pWB2 = NULL;
        IDispatch *         pDispatch = NULL;
        IHTMLDocument2 *    pHTMLDocument = NULL;

        // Get the html document currently loaded
        if (_psp &&
            SUCCEEDED(_psp->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (void **)&pWB2)) &&
            SUCCEEDED(pWB2->get_Document(&pDispatch)) &&
            SUCCEEDED(pDispatch->QueryInterface(IID_IHTMLDocument2, (void **)&pHTMLDocument)))
        {
            //
            // Check the current document for a META tag specifying the program to use to
            // edit this file.
            //
            BSTR bstrProgID = _GetEditProgID(pHTMLDocument);
            if (bstrProgID)
            {
                USES_CONVERSION;
                _aEditVerb.Add(W2T(bstrProgID));
                SysFreeString(bstrProgID);
            }
        }

        SAFERELEASE(pWB2);
        SAFERELEASE(pDispatch);
        SAFERELEASE(pHTMLDocument);
    }


    _fEditEnabled = (_aEditVerb.GetSize() > 0);

    // Update edit glyph, drop-down style, & enabled state
    _InitEditButtonStyle();
}
#endif //EDIT_HACK

HRESULT CInternetToolbar::Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags,
                                 DISPPARAMS * pdispparams, VARIANT * pvarResult,
                                 EXCEPINFO * pexcepinfo,UINT * puArgErr)
{
    ASSERT(pdispparams);
    if(!pdispparams)
        return E_INVALIDARG;

    switch(dispidMember)
    {

    case DISPID_NAVIGATECOMPLETE2:
    {
        //
        // Notify the brand and theater mode objects about whether we're in shell or
        // web mode. Wait til now to do it (rather than doing it in SetCommandTarget)
        // because they might want to ask the browser about the new pidl, which isn't
        // yet filled in at SetCommandTarget time.
        //
        DWORD nCmdexecopt = _fShellView ? CITE_SHELL : CITE_INTERNET;

        LPBANDITEMDATA pbid = _bs._GetBandItemDataStructByID(CBIDX_BRAND);
        if (pbid)
        {
            IUnknown_Exec(pbid->pdb, &CGID_PrivCITCommands, CITIDM_ONINTERNET, nCmdexecopt, NULL, NULL);
        }

        if (_fTheater)
        {
            IUnknown_Exec(_ptbsite, &CGID_Theater, THID_ONINTERNET, nCmdexecopt, NULL, NULL);
        }

        _fNavigateComplete = TRUE;
    }
    break;

    case DISPID_BEFORENAVIGATE:
    {
        BOOL fWeb = FALSE;

        ASSERT((pdispparams->rgvarg[5].vt == VT_BSTR) &&
               (pdispparams->rgvarg[5].bstrVal != NULL));

        PARSEDURL pu = { 0 };
        USES_CONVERSION;

        pu.cbSize = SIZEOF(pu);
        ParseURL(W2T(pdispparams->rgvarg[5].bstrVal), &pu);

        if ((URL_SCHEME_UNKNOWN != pu.nScheme) && (URL_SCHEME_FILE != pu.nScheme))
            fWeb = TRUE;

        UINT uiState = 0;
        GetState(&CLSID_CommonButtons, TBIDM_STOPDOWNLOAD, &uiState);

        if ((uiState & TBSTATE_HIDDEN) && fWeb)
        {

            _fTransitionToHTML = TRUE;
            uiState &= ~TBSTATE_HIDDEN;
            SetState(&CLSID_CommonButtons, TBIDM_STOPDOWNLOAD, uiState);
        }

        // Default to the edit button hidden
        _fEditEnabled = FALSE;
    }
    break;

    case DISPID_DOWNLOADBEGIN:// This is when we just started to navigate?  No bits?
        _StartDownload();
        break;

    case DISPID_DOWNLOADCOMPLETE:    // we be done
        _fTransitionToHTML = FALSE;
        _StopDownload(FALSE);
        break;

    case DISPID_DOCUMENTCOMPLETE:   // This is where we have all the bits
    {
        //
        // Sometimes we get a premature document complete.  We can catch this
        // by checking to see if we have received a DISPID_NAVIGATECOMPLETE2 event.
        //
        if (_fNavigateComplete)
        {
            _fNavigateComplete = FALSE;
            _UpdateEditButton();
        }
        break;
    }

    case DISPID_COMMANDSTATECHANGE:
        BOOL fEnable;

        if(!pdispparams || (pdispparams->cArgs != 2) ||
           (pdispparams->rgvarg[0].vt != VT_BOOL) ||
           (pdispparams->rgvarg[1].vt != VT_I4))
            return E_INVALIDARG;

        fEnable = (BOOL) pdispparams->rgvarg[0].boolVal;
        UINT uiCmd;

        switch (pdispparams->rgvarg[1].lVal)
        {
        case CSC_UPDATECOMMANDS:
            // corresponds to OLECMDID_UPDATECOMMANDS from Exec()
            _UpdateToolbar(FALSE);
            break;

        case CSC_NAVIGATEBACK:
            _fBackEnabled = fEnable;
            _btb._ConvertCmd(&CLSID_CommonButtons, TBIDM_BACK, NULL, &uiCmd);
            SendMessage(_btb._hwnd, TB_ENABLEBUTTON, uiCmd,    MAKELONG(fEnable, 0));
            break;

        case CSC_NAVIGATEFORWARD:
            _fForwardEnabled = fEnable;
            _btb._ConvertCmd(&CLSID_CommonButtons, TBIDM_FORWARD, NULL, &uiCmd);
            SendMessage(_btb._hwnd, TB_ENABLEBUTTON, uiCmd, MAKELONG(fEnable, 0));
            break;

        default:
            return(E_INVALIDARG);
        }

        // BUGBUG need to handle the case of navigation failure and
        // do some cleanup

    }

    return S_OK;
}

//***   CInternetToolbar::IInputObjectSite::* {

HRESULT CInternetToolbar::OnFocusChangeIS(IUnknown *punk, BOOL fSetFocus)
{
    return UnkOnFocusChangeIS(_ptbsite, SAFECAST(this, IInputObject*), fSetFocus);
}

// }

//***   CInternetToolbar::IInputObject::* {

HRESULT CInternetToolbar::TranslateAcceleratorIO(LPMSG lpMsg)
{
    LPBANDITEMDATA pbid;

    if (_fShow)
    {
        if (lpMsg->message == WM_KEYDOWN)
        {
            switch (lpMsg->wParam)
            {
            case VK_F4:
        Laddrband:
                if (_nVisibleBands & VBF_ADDRESS)
                {
                    pbid = _bs._GetBandItemDataStructByID(CBIDX_ADDRESS);
                    if (EVAL(pbid))
                    {
                        HRESULT hrT;

                        hrT = UnkTranslateAcceleratorIO(pbid->pdb, lpMsg);
                        ASSERT(hrT == S_OK);
                    }
                }
                return S_OK;    // (even if we just eat it)
            }
        }
        else if(lpMsg->message == WM_SYSCHAR)
        {
            static CHAR szAccel[2] = "\0";
            CHAR   szChar [2] = "\0";

            if ('\0' == szAccel[0])
                MLLoadStringA(IDS_ADDRBAND_ACCELLERATOR, szAccel, ARRAYSIZE(szAccel));

            szChar[0] = (CHAR)lpMsg->wParam;
            
            if (lstrcmpiA(szChar,szAccel) == 0)
            {
                goto Laddrband;
            }
        }
        return _bs.TranslateAcceleratorIO(lpMsg);
    }
    return S_FALSE;
}


// }

HRESULT CInternetToolbar::SetSite(IUnknown* punkSite)
{
    ATOMICRELEASE(_ptbsite);
    ATOMICRELEASE(_ptbsitect);
    ATOMICRELEASE(_pbs2);
    ATOMICRELEASE(_psp);

    _Unadvise();

    ATOMICRELEASE(_pdie);

    ASSERT(_ptbsite==NULL);
    ASSERT(_ptbsitect==NULL);
    ASSERT(_pbs2==NULL);
    ASSERT(_pdie==NULL);

    if (_pbp && _fCreatedBandProxy)
        _pbp->SetSite(punkSite);

    if (punkSite)
    {
        punkSite->QueryInterface(IID_IDockingWindowSite, (void **)&_ptbsite);
        punkSite->QueryInterface(IID_IOleCommandTarget, (void **)&_ptbsitect);
        punkSite->QueryInterface(IID_IBrowserService2, (void **)&_pbs2);
        punkSite->QueryInterface(IID_IServiceProvider, (void **)&_psp);

        if (_psp)
        {
            _psp->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (void **)&_pdie);
            ASSERT(_pdie);
        }
        else
        {
            ASSERT(0);
        }

    }
    else
    {
        SetClient(NULL);
    }


    return S_OK;
}


//***
//
void CInternetToolbar::_UpdateGroup(const GUID *pguidCmdGroup, int cnt,
    OLECMD rgcmds[], const GUID* pguidButton, const int buttonsInternal[])
{

    if (!IsEqualGUID(*pguidButton, CLSID_CommonButtons) &&
        !IsEqualGUID(*pguidButton, _btb._guidCurrentButtonGroup))
        return; // we don't have any buttons at this time, so no use checking

    if (_ptbsitect) {
        _ptbsitect->QueryStatus(pguidCmdGroup, cnt, rgcmds, NULL);

        // make sure stop is enabled while we are animating
        if (_fAnimating && pguidCmdGroup == NULL && rgcmds[0].cmdID == OLECMDID_STOP) {
            rgcmds[0].cmdf = OLECMDF_ENABLED;
        }
    }

    for (int i = 0; i < cnt; i++)
    {
        // do nothing if command is not available or not in our table
        if (rgcmds[i].cmdf & OLECMDF_SUPPORTED)
        {
            UINT idBut;
            if (SUCCEEDED(_btb._ConvertCmd(pguidButton, buttonsInternal[i], NULL, (UINT*)&idBut)))
            {
                SendMessage(_btb._hwnd, TB_ENABLEBUTTON, idBut,
                    (rgcmds[i].cmdf & OLECMDF_ENABLED) ? TRUE : FALSE);

                SendMessage(_btb._hwnd, TB_CHECKBUTTON, idBut,
                    (rgcmds[i].cmdf & OLECMDF_LATCHED) ? TRUE : FALSE);
            }
        }
    }
    return;
}

void CInternetToolbar::_UpdateToolbar(BOOL fForce)
{
    if (fForce || SHIsChildOrSelf(GetForegroundWindow(), _hwnd) == S_OK)
    {
        if (!_fUpdateToolbarTimer)
        {
            SetTimer(_hwnd, IDT_UPDATETOOLBAR, TIMEOUT_UPDATETOOLBAR, NULL);
            _fUpdateToolbarTimer = TRUE;
            _UpdateToolbarNow();
        }
        else
        {
            _fNeedUpdateToolbar = TRUE;
        }
    }
}

BOOL CInternetToolbar::_UpEnabled()
{
    OLECMD rgcmd = { FCIDM_PREVIOUSFOLDER, 0 };
    _ptbsitect->QueryStatus(&CGID_ShellBrowser, 1, &rgcmd, NULL);

    return (rgcmd.cmdf & OLECMDF_ENABLED);
}

void CInternetToolbar::_UpdateCommonButton(int iCmd, UINT nCmdID)
{
    switch (nCmdID) {
    case TBIDM_THEATER:
        SendMessage(_btb._hwnd, TB_CHECKBUTTON, iCmd, _fTheater);
        break;

    case TBIDM_PREVIOUSFOLDER:
    case TBIDM_BACK:
    case TBIDM_FORWARD:
        {
            BOOL fEnabled;

            switch (nCmdID) {
            case TBIDM_PREVIOUSFOLDER:  fEnabled = _UpEnabled();       break;
            case TBIDM_BACK:            fEnabled = _fBackEnabled;      break;
            case TBIDM_FORWARD:         fEnabled = _fForwardEnabled;   break;
            }

            SendMessage(_btb._hwnd, TB_ENABLEBUTTON, iCmd, MAKELONG(fEnabled, 0));
        }
        break;
    }
}

void CInternetToolbar::_UpdateToolbarNow()
{
    _fNeedUpdateToolbar = FALSE;

    {
        // MUST not be static (due to ConvertCmd overwrite)
        OLECMD rgcmds[] = {
            { OLECMDID_STOP, 0 }, // NOTE: must be first
            { OLECMDID_REFRESH, 0 },
        };

        static const int buttonsInternal[] = { // MUST be in same order as above array
            TBIDM_STOPDOWNLOAD,
            TBIDM_REFRESH,
        };
        _UpdateGroup(NULL, ARRAYSIZE(buttonsInternal), rgcmds, &CLSID_CommonButtons, buttonsInternal);
    }

    {
        OLECMD rgcmds[] = {
            { SBCMDID_SEARCHBAR, 0 },
            { SBCMDID_FAVORITESBAR, 0 },
            { SBCMDID_HISTORYBAR, 0 },
#ifdef ENABLE_CHANNELPANE
            { SBCMDID_CHANNELSBAR, 0 },
#endif
            { SBCMDID_EXPLORERBAR, 0 },
        };
        static const int buttonsInternal[] = { // MUST be in same order as above array
            TBIDM_SEARCH,
            TBIDM_FAVORITES,
            TBIDM_HISTORY,
#ifdef ENABLE_CHANNELPANE
            TBIDM_CHANNELS,
#endif
            TBIDM_ALLFOLDERS,
        };

        _UpdateGroup(&CGID_Explorer, ARRAYSIZE(buttonsInternal), rgcmds, &CLSID_CommonButtons, buttonsInternal);
    }

    int nButtons = (int) SendMessage(_btb._hwnd, TB_BUTTONCOUNT, 0, 0L);

    for (int nIndex = 0; nIndex < nButtons; nIndex++)
    {
        CMDMAP* pcm = _btb._GetCmdMapByIndex(nIndex);
        if (pcm)
        {
            int iCmd = _btb._CommandFromIndex(nIndex);
            if (IsEqualGUID(pcm->guidButtonGroup, CLSID_CommonButtons))
            {
                _UpdateCommonButton(iCmd, pcm->nCmdID);
            }
            else
            {
                // If either of these rip, the button is stale
                ASSERT(IsEqualGUID(pcm->guidButtonGroup, _btb._guidCurrentButtonGroup));
                ASSERT(_btb._pctCurrentButtonGroup);

                OLECMD ocButton;
                ocButton.cmdID = pcm->nCmdID;
                ocButton.cmdf = 0;

                if (SUCCEEDED(_btb._pctCurrentButtonGroup->QueryStatus(&pcm->guidButtonGroup, 1, &ocButton, NULL)))
                {
                    SendMessage(_btb._hwnd, TB_ENABLEBUTTON, iCmd,
                                (ocButton.cmdf & OLECMDF_ENABLED) ? TRUE : FALSE);

                    SendMessage(_btb._hwnd, TB_CHECKBUTTON, iCmd,
                                (ocButton.cmdf & OLECMDF_LATCHED) ? TRUE : FALSE);
                }
            }
        }
    }

    _btb._BandInfoChanged();
}

void CInternetToolbar::_StartDownload()
{
    UINT uiCmd;
    if (SUCCEEDED(_btb._ConvertCmd(&CLSID_CommonButtons, TBIDM_STOPDOWNLOAD, NULL, &uiCmd)))
    {
        SendMessage(_btb._hwnd, TB_ENABLEBUTTON, uiCmd, MAKELONG(TRUE, 0));

        _fAnimating = TRUE;
    }
}

//
// Parameters:
//  fClosing -- TRUE only if we are calling this from CloseDW member.
//              In that case, we can skip all UI-update code.
//
void CInternetToolbar::_StopDownload(BOOL fClosing)
{
    _fAnimating = FALSE;
}

HRESULT CInternetToolbar::CloseDW(DWORD dwReserved)
{
    _fDestroyed = TRUE; // Stop using the member variables, they are invalid.
    _StopDownload(TRUE);

    ASSERT(!_btb._pcinfo);
    ATOMICRELEASE(_btb._pctCurrentButtonGroup);

    _btb._FreeBtnsAdded();

    if (_btb._hwnd)
    {
        _btb._RemoveAllButtons();

        SendMessage(_btb._hwnd, TB_SETIMAGELIST, 0, NULL);
        SendMessage(_btb._hwnd, TB_SETHOTIMAGELIST, 0, NULL);

        DSA_Destroy(_hdsaTBBMPs);
        _hdsaTBBMPs = NULL;  // So we don't try to re-destroy in _InitBitmapDSA()
        _hdsaTBBMPs = NULL;
    }
#ifdef EDIT_HACK
    _SetEditGlyph(-1);
#endif

    _bs._Close();

    SUPERCLASS::CloseDW(dwReserved);

    _btb._hwnd = NULL;

    // We advise during ShowDW, so unadvise here. Also, we hit a stress
    // case where it seems that an event came in after closedw but before
    // one of the other _Unadvise calls. This event percolated down to
    // a reference to _hdsaCT which we freed above, causing a GPF.
    //
    _Unadvise();

    return S_OK;
}

void CInternetToolbar::CITBandSite::v_SetTabstop(LPREBARBANDINFO prbbi)
{
    // Don't set tabstops for all bands in the browser case.  A band
    // can still make itself a tabstop by setting WS_TABSTOP.
    return;
}

BOOL CInternetToolbar::CITBandSite::_SetMinDimensions()
{
    INT_PTR fRedraw = SendMessage(_hwnd, WM_SETREDRAW, FALSE, 0);

    int icBands = (int) SendMessage( _hwnd, RB_GETBANDCOUNT, 0, 0 );
    for (int i = 0; i < icBands; i++)
    {
        REBARBANDINFO rbbi;
        rbbi.cbSize = sizeof(REBARBANDINFO);
        rbbi.fMask = RBBIM_ID | RBBIM_CHILDSIZE;
        if (SendMessage(_hwnd, RB_GETBANDINFO, i, (LPARAM) &rbbi))
        {
            LPBANDITEMDATA pbid = (LPBANDITEMDATA)_GetBandItemDataStructByID(rbbi.wID);
            if (EVAL(pbid) && IS_VALID_HANDLE(pbid->hwnd, WND))
            {
                rbbi.cxMinChild = pbid->ptMinSize.x;
                rbbi.cyMinChild = pbid->ptMinSize.y;
            }
            else
            {
                rbbi.cxMinChild = 0;
                rbbi.cyMinChild = 0;
            }

            SendMessage(_hwnd, RB_SETBANDINFO, i, (LPARAM) &rbbi);
        }
    }

    SendMessage(_hwnd, WM_SETREDRAW, fRedraw, 0);

    return TRUE;
}


BOOL HimlCacheDirty(IMLCACHE* pimlCache, BOOL fSmallIcons)
{

    if (fSmallIcons != pimlCache->fSmallIcons)
        return TRUE;

    COLORREF cr3D = GetSysColor(COLOR_3DFACE);

    if (cr3D != pimlCache->cr3D)
        return TRUE;

    for (int i = 0; i < CIMLISTS; i++)
        if (!pimlCache->arhiml[i])
            return TRUE;

    return FALSE;
}


#define SZ_REGKEY_SMALLICONS       REGSTR_PATH_EXPLORER TEXT("\\SmallIcons")
#define SZ_REGVALUE_SMALLICONS     TEXT("SmallIcons")

BOOL _DefaultToSmallIcons()
{
    // On NT5, we want to default to small icons.
    return (GetUIVersion() >= 5) || SHRestricted2(REST_SMALLICONS, NULL, 0);
}

BOOL _UseSmallIcons()
{
    BOOL fDefaultToSmall = _DefaultToSmallIcons();

    return SHRegGetBoolUSValue(SZ_REGKEY_SMALLICONS, SZ_REGVALUE_SMALLICONS,
                                        FALSE, fDefaultToSmall);
}


BOOL _UseMapNetDrvBtns()
{
#define SZ_REGKEY_ADVFOLDER        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced")
#define SZ_REGVALUE_MAPNETDRV      TEXT("MapNetDrvBtn")

    DWORD dwData = 0;
    if (GetUIVersion() >= 4)
    {
        DWORD cbData = SIZEOF(dwData);
        DWORD dwDefault = 0;
        DWORD cbDefault = SIZEOF(dwDefault);

        SHRegGetUSValue(SZ_REGKEY_ADVFOLDER, SZ_REGVALUE_MAPNETDRV, NULL,
                            &dwData, &cbData, FALSE, &dwDefault, cbDefault);
    }
    return dwData;
}

void _LoadToolbarGlyphs(HWND hwnd, IMLCACHE *pimlCache, int cx, int idBmp)
{
    // set uMsg and uFlags for first iteration of loop (default state)
    UINT uMsg = TB_SETIMAGELIST;
    UINT uFlags = LR_CREATEDIBSECTION;
    int i;
    HBITMAP hBMP;
    BOOL fSmallIcons = g_fSmallIcons;

    if (HimlCacheDirty(pimlCache, fSmallIcons))
    {
        COLORREF cr3D   = GetSysColor(COLOR_3DFACE);
        COLORREF crMask = RGB( 255, 0, 255 );

#ifdef UNIX
        if (SHGetCurColorRes() < 2 )
        {
            crMask = CLR_NONE;
        }
#endif

        ENTERCRITICAL;

        if (!HimlCacheDirty(pimlCache, fSmallIcons) )
            goto DontReload;


        for (i = 0; i < CIMLISTS; i++)
        {
            if ((!pimlCache->arhiml[i]) || (cr3D != pimlCache->cr3D) || fSmallIcons != pimlCache->fSmallIcons)
            {

                TraceMsg(DM_ITBAR, "CInternetToolbar: Loading New Images");

                if (pimlCache->arhimlPendingDelete[i])
                    ImageList_Destroy(pimlCache->arhimlPendingDelete[i]);


                pimlCache->arhimlPendingDelete[i] = pimlCache->arhiml[i];
                pimlCache->arhiml[i] = ImageList_LoadImage(HINST_THISDLL,
                                               MAKEINTRESOURCE(idBmp + i), cx, 0, crMask,
                                               IMAGE_BITMAP, uFlags);

                if (pimlCache->arhiml[i])
                {
                    // add shell glyphs
                    int idShellBmp = IDB_SHSTD;
                    int iDelta = idBmp - IDB_IETOOLBAR;
                    idShellBmp += iDelta;
                    hBMP = LoadBitmap(HINST_THISDLL, MAKEINTRESOURCE(idShellBmp + i));
                    ImageList_AddMasked(pimlCache->arhiml[i], (HBITMAP)hBMP, crMask);
                    DeleteObject(hBMP);
                }
            }
        }
        pimlCache->cr3D = cr3D;
        pimlCache->fSmallIcons = fSmallIcons;
DontReload:
        LEAVECRITICAL;
    }

    if (hwnd) {
        ASSERT(IS_VALID_HANDLE(hwnd, WND));

        for (i = 0; i < CIMLISTS; i++)
        {
            SendMessage(hwnd, uMsg, 0, (LPARAM) pimlCache->arhiml[i]);

            // set uMsg and uFlags for last iteration of loop (hot state)
            uMsg = TB_SETHOTIMAGELIST;
            uFlags = 0;
        }
    }
}


void CInternetToolbar::_InitBitmapDSA()
{
    DSA_Destroy(_hdsaTBBMPs);
    _hdsaTBBMPs = DSA_Create(SIZEOF(TBBMP_LIST), TBBMPLIST_CHUNK);

    if (_hdsaTBBMPs) {
        TBBMP_LIST tbl = { HINST_COMMCTRL, 0, 0, TRUE, TRUE, FALSE };

        tbl.uiResID = IDB_STD_SMALL_COLOR;
        tbl.uiOffset = OFFSET_STD;
        DSA_AppendItem(_hdsaTBBMPs, &tbl);
        tbl.uiResID = IDB_STD_LARGE_COLOR;
        DSA_AppendItem(_hdsaTBBMPs, &tbl);

        tbl.uiResID = IDB_VIEW_SMALL_COLOR;
        tbl.uiOffset = OFFSET_VIEW;
        DSA_AppendItem(_hdsaTBBMPs, &tbl);
        tbl.uiResID = IDB_VIEW_LARGE_COLOR;
        DSA_AppendItem(_hdsaTBBMPs, &tbl);

        tbl.uiResID = IDB_HIST_SMALL_COLOR;
        tbl.uiOffset = OFFSET_HIST;
        DSA_AppendItem(_hdsaTBBMPs, &tbl);
        tbl.uiResID = IDB_HIST_LARGE_COLOR;
        DSA_AppendItem(_hdsaTBBMPs, &tbl);
    }
}

void CInternetToolbar::_InitForScreenSize()
{
    TCHAR szScratch[16];
    if (GetSystemMetrics(SM_CXSCREEN) < 650) 
    {
        MLLoadString(IDS_TB_WIDTH_EXTRA_LORES, szScratch, ARRAYSIZE(szScratch));
        _uiMaxTBWidth = MAX_TB_WIDTH_LORES;
    } 
    else 
    {
        MLLoadString(IDS_TB_WIDTH_EXTRA_HIRES, szScratch, ARRAYSIZE(szScratch));
        _uiMaxTBWidth = MAX_TB_WIDTH_HIRES;
    }
    _uiMaxTBWidth += StrToInt(szScratch) * WIDTH_FACTOR;
}


// removes all buttons marked hidden.  returns the number
// of buttons left
int RemoveHiddenButtons(TBBUTTON* ptbn, int iCount)
{
    int i;
    int iTotal = 0;
    TBBUTTON* ptbn1 = ptbn;
    for (i = 0; i < iCount; i++, ptbn1++) 
    {
        if (!(ptbn1->fsState & TBSTATE_HIDDEN)) 
        {
            if (ptbn1 != ptbn) 
            {
                *ptbn = *ptbn1;
            }
            ptbn++;
            iTotal++;
        }
    }
    return iTotal;
}

#ifdef DEBUG
void _AssertRestrictionOrderIsCorrect()
{
    COMPILETIME_ASSERT(ARRAYSIZE(c_tbExplorer) == ARRAYSIZE(c_rest));

    for (UINT i = 0; i < ARRAYSIZE(c_tbExplorer); i++)
    {
        // If any of these rip, it means that c_rest and c_tbExplorer have
        // gotten out of sync.  Need to fix up c_rest to match c_tbExplorer.
        switch (c_tbExplorer[i].idCommand)
        {
            case TBIDM_BACK:            ASSERT(c_rest[i] == REST_BTN_BACK);         break;
            case TBIDM_FORWARD:         ASSERT(c_rest[i] == REST_BTN_FORWARD);      break;
            case TBIDM_STOPDOWNLOAD:    ASSERT(c_rest[i] == REST_BTN_STOPDOWNLOAD); break;
            case TBIDM_REFRESH:         ASSERT(c_rest[i] == REST_BTN_REFRESH);      break;
            case TBIDM_HOME:            ASSERT(c_rest[i] == REST_BTN_HOME);         break;
            case TBIDM_SEARCH:          ASSERT(c_rest[i] == REST_BTN_SEARCH);       break;
            case TBIDM_HISTORY:         ASSERT(c_rest[i] == REST_BTN_HISTORY);      break;
            case TBIDM_FAVORITES:       ASSERT(c_rest[i] == REST_BTN_FAVORITES);    break;
            case TBIDM_ALLFOLDERS:      ASSERT(c_rest[i] == REST_BTN_ALLFOLDERS);   break;
            case TBIDM_THEATER:         ASSERT(c_rest[i] == REST_BTN_THEATER);      break;
            default:                    ASSERT(c_rest[i] == REST_BROWSER_NONE);     break;
        }
    }
}
#endif

__inline BOOL CInternetToolbar::_FoldersButtonAvailable()
{
    return (GetUIVersion() >= 4);
}

void CInternetToolbar::_AdminMarkDefaultButtons(PTBBUTTON ptbb, UINT cButtons)
{
    // We only have policies for web buttons.
    ASSERT(!_fShellView);

    // Caller should have checked this.
    ASSERT(SHRestricted2(REST_SPECIFYDEFAULTBUTTONS, NULL, 0));

    // SHRestricted2 returns 0 if it can't find the policy.  Assert that
    // this lines up with RESTOPT_BTN_STATE_DEFAULT.
    COMPILETIME_ASSERT(RESTOPT_BTN_STATE_DEFAULT == 0);

    for (UINT i = 0; i < cButtons; i++) 
    {
        if (c_rest[i] != 0) {
            DWORD dwRest = SHRestricted2(c_rest[i], NULL, 0);
            ptbb[i].fsState = SHBtnStateFromRestriction(dwRest, ptbb[i].fsState);
        }
    }

    // Folders button is not available on non-integrated platforms, so
    // set state to hidden even if policy specifies that it should be shown.
    ASSERT(c_tbExplorer[10].idCommand == TBIDM_ALLFOLDERS);
    if (!_FoldersButtonAvailable())
        ptbb[10].fsState |= TBSTATE_HIDDEN;
}

void CInternetToolbar::_MarkDefaultButtons(PTBBUTTON ptbb, UINT cButtons)
{
    if (_fShellView) 
    {
        ASSERT(c_tbExplorer[2].idCommand == TBIDM_STOPDOWNLOAD);
        ptbb[2].fsState |= TBSTATE_HIDDEN;
        ASSERT(c_tbExplorer[3].idCommand == TBIDM_REFRESH);
        ptbb[3].fsState |= TBSTATE_HIDDEN;
        ASSERT(c_tbExplorer[4].idCommand == TBIDM_HOME);
        ptbb[4].fsState |= TBSTATE_HIDDEN;

        ASSERT(c_tbExplorer[9].idCommand == TBIDM_SEARCH);
        ASSERT(c_tbExplorer[12].idCommand == TBIDM_HISTORY);
        ASSERT(c_tbExplorer[13].idCommand == 0);    // (a separator)

        if (GetUIVersion() < 5) 
        {
            ptbb[9].fsState |= TBSTATE_HIDDEN;
            ptbb[12].fsState |= TBSTATE_HIDDEN;
            ptbb[13].fsState |= TBSTATE_HIDDEN;
        }
        else
        {
            if (SHRestricted(REST_NOSHELLSEARCHBUTTON))
            {
                ptbb[9].fsState |= TBSTATE_HIDDEN;
            }
            ASSERT(c_tbExplorer[6].idCommand == TBIDM_CONNECT);
            ASSERT(c_tbExplorer[7].idCommand == TBIDM_DISCONNECT);
            if (SHRestricted(REST_NONETCONNECTDISCONNECT))
            {
                ptbb[6].fsState |= TBSTATE_HIDDEN;
                ptbb[7].fsState |= TBSTATE_HIDDEN;
            }
        }

        ASSERT(c_tbExplorer[11].idCommand == TBIDM_FAVORITES);
        ptbb[11].fsState |= TBSTATE_HIDDEN;
    }

    ASSERT(c_tbExplorer[5].idCommand == TBIDM_PREVIOUSFOLDER);
    if (!_fShellView)
        ptbb[5].fsState |= TBSTATE_HIDDEN;

    ASSERT(c_tbExplorer[6].idCommand == TBIDM_CONNECT);
    ASSERT(c_tbExplorer[7].idCommand == TBIDM_DISCONNECT);
    if (!_fShellView || !_UseMapNetDrvBtns()) 
    {
        ptbb[6].fsState |= TBSTATE_HIDDEN;
        ptbb[7].fsState |= TBSTATE_HIDDEN;
    }

    // If this TBIDM_ALLFOLDERS assertion rips, remember to fix up _AdminMarkDefaultButtons too.
    ASSERT(c_tbExplorer[10].idCommand == TBIDM_ALLFOLDERS);
    if (!_fShellView || GetUIVersion() < 5)
        ptbb[10].fsState |= TBSTATE_HIDDEN;

    ASSERT(c_tbExplorer[14].idCommand == TBIDM_THEATER);
    ptbb[14].fsState |= TBSTATE_HIDDEN;
}

void CInternetToolbar::_AddCommonButtons()
{
    TBBUTTON    tbExplorer[ARRAYSIZE(c_tbExplorer)];

    memcpy(tbExplorer, c_tbExplorer, SIZEOF(TBBUTTON) * ARRAYSIZE(c_tbExplorer));

    _MarkDefaultButtons(tbExplorer, ARRAYSIZE(c_tbExplorer));

#ifdef DEBUG
    _AssertRestrictionOrderIsCorrect();
#endif

    if (!_fShellView && SHRestricted2(REST_SPECIFYDEFAULTBUTTONS, NULL, 0))
        _AdminMarkDefaultButtons(tbExplorer, ARRAYSIZE(c_tbExplorer));

    int iButtons = RemoveHiddenButtons(tbExplorer, ARRAYSIZE(tbExplorer));

    for (int i = 0; i < iButtons; i++) 
    {
        if (!(tbExplorer[i].fsStyle & BTNS_SEP)) 
        {
            CMDMAP* pcm = (CMDMAP*)LocalAlloc(LPTR, SIZEOF(CMDMAP));
            if (pcm) 
            {
                pcm->guidButtonGroup = CLSID_CommonButtons;
                pcm->nCmdID = tbExplorer[i].idCommand;

                tbExplorer[i].idCommand = _btb._nNextCommandID++;
                tbExplorer[i].dwData = (LPARAM)pcm;
            }
        }
    }

    SendMessage(_btb._hwnd, TB_ADDBUTTONS, iButtons, (LPARAM) tbExplorer);

    _btb._RecalcButtonWidths();
}

#define IS_LIST_STYLE(hwnd) (BOOLIFY(GetWindowLong(hwnd, GWL_STYLE) & TBSTYLE_LIST))

void CInternetToolbar::_UpdateToolsStyle(BOOL fList)
{
    if (BOOLIFY(fList) != IS_LIST_STYLE(_btb._hwnd))
    {
        _fDirty = TRUE;

        // toggle TBSTYLE_LIST
        SHSetWindowBits(_btb._hwnd, GWL_STYLE, TBSTYLE_LIST, fList ? TBSTYLE_LIST : 0);
        // toggle TBSTYLE_EX_MIXEDBUTTONS
        SendMessage(_btb._hwnd, TB_SETEXTENDEDSTYLE, TBSTYLE_EX_MIXEDBUTTONS, fList ? TBSTYLE_EX_MIXEDBUTTONS : 0);
    }
}

void CInternetToolbar::_InitToolbar()
{
    TCHAR szShellTBText[1024];  // This should be enough
    ZeroMemory(szShellTBText, sizeof(szShellTBText));
    int nRows = _fCompressed ? 0 : _uiTBTextRows;

    // this tells the toolbar what version we are
    SendMessage(_btb._hwnd, TB_BUTTONSTRUCTSIZE,    SIZEOF(TBBUTTON), 0);
    SendMessage(_btb._hwnd, TB_SETEXTENDEDSTYLE,    0,
                    TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_HIDECLIPPEDBUTTONS);
    SendMessage(_btb._hwnd, TB_SETMAXTEXTROWS,      nRows, 0L);
    SendMessage(_btb._hwnd, TB_SETDROPDOWNGAP,  GetSystemMetrics(SM_CXEDGE) / 2, 0);
    SendMessage(_btb._hwnd, CCM_SETVERSION, COMCTL32_VERSION, 0);

    _UpdateToolsStyle(_cs.fList);

    ITBar_LoadToolbarGlyphs(_btb._hwnd);
    _InitBitmapDSA();

    _InitForScreenSize();

    SendMessage(_btb._hwnd, TB_ADDSTRING, (WPARAM)MLGetHinst(), IDS_IE_TB_LABELS);

    _AddCommonButtons();

    INT_PTR nRet = SendMessage(_btb._hwnd, TB_ADDSTRING, (WPARAM)MLGetHinst(), IDS_SHELL_TB_LABELS);
    ASSERT(nRet == SHELLTOOLBAR_OFFSET);
}

HRESULT CInternetToolbar::_ShowTools(PBANDSAVE pbs)
{
    HRESULT         hr  = S_OK;
    LPBANDITEMDATA  pbid = _bs._GetBandItemDataStructByID(CBIDX_TOOLS);

    if (!pbid) {
        ASSERT(!_btb._hwnd);

        _btb._hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
                                WS_CHILD | TBSTYLE_FLAT |
                                TBSTYLE_TOOLTIPS |
                                WS_CLIPCHILDREN |
                                WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NOPARENTALIGN |
                                CCS_NORESIZE,
                                0, 0, 0, 0, _bs._hwnd, (HMENU) FCIDM_TOOLBAR, HINST_THISDLL, NULL);

        if (_btb._hwnd) {
            _btb._hwnd = _btb._hwnd;
            _InitToolbar();
            pbid = _AddNewBand((IDeskBand*)&_btb, CBIDX_TOOLS);
        }

        if (!pbid)
            return E_OUTOFMEMORY;
    } else {
        pbs = NULL;
    }

    _ShowBandCommon(pbs, pbid, _nVisibleBands & VBF_TOOLS);
    return hr;
}

void CInternetToolbar::_ShowBandCommon(PBANDSAVE pbs, LPBANDITEMDATA pbid, BOOL fShow)
{
    REBARBANDINFO   rbbi;

    pbid->fShow = BOOLIFY(fShow);
    pbid->pdb->ShowDW(pbid->fShow);

    INT_PTR i = BandIDtoIndex(_bs._hwnd, pbid->dwBandID);

    if (pbs)
    {
        rbbi.cbSize = sizeof(REBARBANDINFO);
        rbbi.fMask = RBBIM_SIZE | RBBIM_STYLE;

        // we just want to change the RBBS_BREAK bit
        // assert that our caller doesn't expect to set any other bits
        // ASSERT(!(pbs->fStyle & ~RBBS_BREAK)); <--- I hit this assert all the time

        // get old style
        SendMessage(_bs._hwnd, RB_GETBANDINFO, i, (LPARAM)&rbbi);

        rbbi.fStyle = (rbbi.fStyle & ~RBBS_BREAK) | (pbs->fStyle & RBBS_BREAK);
        rbbi.cx = pbs->cx;

        SendMessage(_bs._hwnd, RB_SETBANDINFO, i, (LPARAM)&rbbi);
    }


    if ( pbid->dwModeFlags & DBIMF_BREAK )
    {
        rbbi.cbSize = sizeof(REBARBANDINFO);
        rbbi.fMask = RBBIM_STYLE;
        if (SendMessage(_bs._hwnd, RB_GETBANDINFO, i, (LPARAM) &rbbi))
        {
            // in theater mode we don't allow bands to have breaks
            if ((rbbi.fStyle & RBBS_BREAK ) && _fTheater)
            {
                rbbi.fStyle &= ~RBBS_BREAK;
                SendMessage(_bs._hwnd, RB_SETBANDINFO, i, (LPARAM) &rbbi);
            }
        }
    }

    SendMessage(_bs._hwnd, RB_SHOWBAND, i, pbid->fShow);
}


HRESULT CInternetToolbar::_GetPersistedBand(const CLSID clsid, REFIID riid, void ** ppiface)
{
    HRESULT hr  = E_FAIL;
    TCHAR szStreamName[MAX_PATH];

    if (SUCCEEDED(_GetStreamName(_fInitialPidlIsWeb, szStreamName, ARRAYSIZE(szStreamName))))
    {
        static BOOL fBrowserOnly = (WhichPlatform() != PLATFORM_INTEGRATED);
        TCHAR szKey[MAX_PATH];
        TCHAR szGUID[MAX_PATH];

        wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("Software\\Microsoft\\Internet Explorer\\Toolbar\\%s"), szStreamName);
        SHStringFromGUID(clsid, szGUID, ARRAYSIZE(szGUID));

        if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, szKey, szGUID, NULL, NULL, NULL))
        {
            // Was the stream saved by an Integrated shell and we are in browser only mode?
            if ((_cs.fSaveInShellIntegrationMode) && fBrowserOnly)
            {
                // Yes, so we need to ignore the stream.
            }
            else
            {
                IStream * pstm = GetRegStream(_fInitialPidlIsWeb, szGUID, STGM_READ);
                if (pstm)
                {
                    hr = _bs.LoadFromStreamBS(pstm, riid, ppiface);
                    pstm->Release();
                }
            }
        }
    }

    if (FAILED(hr))
    {
        hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, riid, ppiface);
        if (SUCCEEDED(hr))
        {
            IPersistStreamInit * ppsi;
            ((IUnknown *) *ppiface)->QueryInterface(IID_IPersistStreamInit, (void **) &ppsi);
            if (ppsi)
            {
                ppsi->InitNew();
                ppsi->Release();
            }
        }
    }

    return hr;
}


HRESULT CInternetToolbar::_ShowExternalBand( PBANDSAVE pbs, int idBand )
{
    HRESULT hr;
    if (IS_EXTERNALBAND(idBand))
    {
        int idBandExt = MAP_TO_EXTERNAL(idBand);

        if (!IsEqualCLSID(_rgebi[idBandExt].clsid, GUID_NULL))
        {
            LPBANDITEMDATA pbid = _bs._GetBandItemDataStructByID( idBand );
            BOOL fIsVisible = _nVisibleBands & EXTERNALBAND_VBF_BIT(idBandExt);
            if (!pbid && fIsVisible)
            {
                IDeskBand *pitbBand;
                hr = _GetPersistedBand(_rgebi[idBandExt].clsid, IID_IDeskBand, (void **) &pitbBand);
                if (SUCCEEDED(hr))
                {
                    pbid = _AddNewBand( pitbBand, idBand );
                    pitbBand->Release();
                }
                if (!pbid)
                    return E_OUTOFMEMORY;
            } else {
                pbs = NULL;
                if (!pbid)
                    return S_OK;
            }
            _ShowBandCommon(pbs, pbid, fIsVisible );
        }
    }
    return S_OK;
}


HRESULT CInternetToolbar::_ShowAddressBand(PBANDSAVE pbs)
{
    HRESULT         hr  = S_OK;
    LPBANDITEMDATA  pbid = _bs._GetBandItemDataStructByID(CBIDX_ADDRESS);
    if (!pbid)
    {
        IDeskBand *pitbAddressBand;

        hr = _GetPersistedBand(CLSID_AddressBand, IID_IDeskBand, (void **)&pitbAddressBand);
        if (SUCCEEDED(hr))
        {
            pbid = _AddNewBand(pitbAddressBand, CBIDX_ADDRESS);
            if (pbid)
                _hwndAddressBand = pbid->hwnd;

            pitbAddressBand->Release();
        }

        ASSERT(IS_VALID_HANDLE(_hwndAddressBand, WND));
        if (!pbid)
            return E_OUTOFMEMORY;
    } else
        pbs = NULL;


    _ShowBandCommon(pbs, pbid, _nVisibleBands & VBF_ADDRESS);
    return S_OK;
}

LPBANDITEMDATA CInternetToolbar::_AddNewBand(IDeskBand* pdb, DWORD dwID)
{
    if (SUCCEEDED(_bs._AddBandByID(pdb, dwID)))
    {
        return _bs._GetBandItemDataStructByID(dwID);
    }
    return NULL;
}


HRESULT CInternetToolbar::_ShowLinks(PBANDSAVE pbs)
{
    HRESULT hr = S_OK;

    LPBANDITEMDATA pbid = _bs._GetBandItemDataStructByID(CBIDX_LINKS);
    if (!pbid)
    {
        IDeskBand* pdbLinks = NULL;

        // Check if custom link band GUID is present in the registry,
        // and if so, do a full CoCreateInstance using this GUID.
        // Otherwise, just do the normal internal call to the link's band factory.

        if (!_fInitialPidlIsWeb ||
            FAILED(CreateFromRegKey(c_szRegKeyCoolbar, TEXT("QuickLinksCLSID"), IID_IDeskBand, (void **)&pdbLinks)))
        {
            hr = _GetPersistedBand(CLSID_QuickLinks, IID_IDeskBand, (void **)&pdbLinks);
            IUnknown_Exec(pdbLinks, &CLSID_QuickLinks, QLCMD_SINGLELINE, 1, NULL, NULL);
        }

        if (pdbLinks)
        {
            // mark it so ISFBand knows it's qlinks (for UAssist)
            VARIANTARG v;
#ifdef DEBUG
            {
                // n.b. we overwrite old persisted guys (which should be -1)
                IUnknown_Exec(pdbLinks, &CGID_ISFBand, ISFBID_PRIVATEID, 0, NULL, &v);
                ASSERT(v.lVal == -1 || v.lVal == CSIDL_FAVORITES);
            }
#endif
            v.vt = VT_I4;
            v.lVal = CSIDL_FAVORITES;   // close enough for our purposes...
            IUnknown_Exec(pdbLinks, &CGID_ISFBand, ISFBID_PRIVATEID, 0, &v, NULL);
            pbid = _AddNewBand(pdbLinks, CBIDX_LINKS);

            pdbLinks->Release();
        }

        if (!pbid)
            return E_OUTOFMEMORY;
    } else
        pbs = NULL;

    _ShowBandCommon(pbs, pbid, _nVisibleBands & VBF_LINKS);
    return hr;
}

HRESULT CInternetToolbar::_ShowMenu(PBANDSAVE pbs)
{
    LPBANDITEMDATA pbid = _bs._GetBandItemDataStructByID(CBIDX_MENU);
    if (!pbid)
    {
        CFavoritesCallback* pfcb = new CFavoritesCallback();
        if (pfcb)
        {
            IShellMenu* psm = (IShellMenu*)new CMenuBand();
            if (psm)
            {
                VARIANTARG var;

                if (SUCCEEDED(IUnknown_Exec(_pbs2, &CGID_Explorer, SBCMDID_GETCURRENTMENU, 0, NULL, &var)) &&
                        var.vt == VT_INT_PTR && var.byref)
                {
                    IDeskBand* pdbMenu;
                    psm->Initialize(pfcb, -1, ANCESTORDEFAULT, SMINIT_HORIZONTAL | SMINIT_TOPLEVEL);
                    if (SUCCEEDED(psm->SetMenu((HMENU)var.byref, GetParent(_hwnd), SMSET_DONTOWN)))
                    {
                        if (SUCCEEDED(psm->QueryInterface(IID_IDeskBand, (void**)&pdbMenu)))
                        {
                            pbid = _AddNewBand(pdbMenu, CBIDX_MENU);
                            if (pbid)
                            {
                                // Tell the menuband we're not a real bar/bandsite/band
                                IUnknown_Exec(pbid->pdb, &CGID_MenuBand, MBANDCID_NOTAREALSITE, TRUE, NULL, NULL);

                                _bs.SetBandState(CBIDX_MENU, BSSF_NOTITLE, BSSF_NOTITLE);
                                _hwndMenu = pbid->hwnd;
                            }

                            ASSERT(IS_VALID_HANDLE(_hwndMenu, WND));

                            pdbMenu->Release();
                        }
                    }
                }
                psm->Release();
            }
            pfcb->Release();
        }

        if (!pbid)
            return E_OUTOFMEMORY;
    } else
        pbs = NULL;


    _ShowBandCommon(pbs, pbid, _nVisibleBands & VBF_MENU);
    return S_OK;
}

HRESULT _GetBackBitmapLocation(LPTSTR psz, BOOL fInternet)
{
    HRESULT hres = E_FAIL;
    DWORD dwType;
    DWORD dwcbData;

    // IE4 shipped back bitmap customization affecting both browser and shell.
    // IE5 wants these to be separate customizations.  But in the roaming
    // case a customized IE4 customer shouldn't lose customization when going
    // to the IE5 machine.  So we might need to check twice:
    //
    if (fInternet)
    {
        // Try the IE5 internet location.
        dwcbData = MAX_PATH * SIZEOF(TCHAR);
        hres = SHGetValue(HKEY_CURRENT_USER, c_szRegKeyCoolbar, TEXT("BackBitmapIE5"), &dwType, psz, &dwcbData);
    }
    else
    {
        // Try the NT5 shell location.
        dwcbData = MAX_PATH * SIZEOF(TCHAR);
        hres = SHGetValue(HKEY_CURRENT_USER, c_szRegKeyCoolbar, TEXT("BackBitmapShell"), &dwType, psz, &dwcbData);
    }
    if (ERROR_SUCCESS != hres)
    {
        // Try the old combined internet/shell location
        dwcbData = MAX_PATH * SIZEOF(TCHAR);
        hres = SHGetValue(HKEY_CURRENT_USER, c_szRegKeyCoolbar, TEXT("BackBitmap"), &dwType, psz, &dwcbData);
    }
    if (ERROR_SUCCESS != hres)
    {
        *psz = '\0';
    }

    return hres;
}

HBITMAP CInternetToolbar::_LoadBackBmp(LPTSTR * ppszBitmap, BMPCACHE * pbmpCache, BOOL fInternet)
{
    HIGHCONTRAST    hc;
    HBITMAP     hbmp = pbmpCache->hbmp;
    COLORREF    cr3D = GetSysColor(COLOR_3DFACE);
    TCHAR       szScratch[MAX_PATH];
    LPTSTR      pszBitmap = NULL;
    BOOL        fBitmapInvalid = FALSE;


    ENTERCRITICAL;

    // If the stashed hbmp's cr3D color changed, we need to mark invalid
    if (pbmpCache->hbmp && pbmpCache->cr3D != cr3D)
        fBitmapInvalid = TRUE;

    // get the location spec for the bitmap
    hc.cbSize = sizeof(HIGHCONTRAST);
    if ((SystemParametersInfoA(SPI_GETHIGHCONTRAST, hc.cbSize, (LPVOID) &hc, FALSE)) &&
        (hc.dwFlags & HCF_HIGHCONTRASTON))
    {
        // we have no bitmap in high contrast
    }
    else if (SUCCEEDED(_GetBackBitmapLocation(szScratch, fInternet)))
    {
        pszBitmap = szScratch;
    }

    // if they are removing the bitmap, we need to mark invalid
    if (!pszBitmap && *ppszBitmap)
        fBitmapInvalid = TRUE;

    // or it's location has been changed, we need to mark invalid
    if (pszBitmap && (!*ppszBitmap || lstrcmpi(pszBitmap, *ppszBitmap)))
        fBitmapInvalid = TRUE;

    if (fBitmapInvalid)
    {
        TraceMsg(DM_ITBAR, "CInternetToolbar: Loading Background Bitmap");

        Str_SetPtr(ppszBitmap, pszBitmap);

        hbmp=NULL;
        if (*ppszBitmap)
        {
            if ((*ppszBitmap)[0])
                hbmp = (HBITMAP) LoadImage(NULL, szScratch, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_CREATEDIBSECTION | LR_LOADFROMFILE | LR_LOADMAP3DCOLORS );

            if (!hbmp)
            {
#ifdef OLD_SWIRLY_BACKDROP
                if (SHGetCurColorRes() <= 8)
                    hbmp = (HBITMAP) LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDB_BACK), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_CREATEDIBSECTION | LR_LOADMAP3DCOLORS );
#endif
            }
        }

#ifdef OLD_LEGACY_BAD_COLOUR_CODE
        if (hbmp)
        {
            // mapping needed ?
            // BUGBUG: this will be removed as soon as I get the new backdrop....
            if ( /* cr3D != RGB(192,192,192) */ FALSE)
            {
                RGBQUAD     rgbTable[256];
                RGBQUAD     rgbFace;
                HDC         dc;
                HBITMAP     hbmSave;
                UINT        i;
                UINT        n;

                dc = CreateCompatibleDC(NULL);
                hbmSave = (HBITMAP)SelectObject(dc, hbmp);
                n = GetDIBColorTable(dc, 0, 256, rgbTable);

                rgbFace.rgbRed   = GetRValue(cr3D);
                rgbFace.rgbGreen = GetGValue(cr3D);
                rgbFace.rgbBlue  = GetBValue(cr3D);

                for (i = 0; i < n; i++)
                {
                    if ( rgbTable[i].rgbRed == 192 && rgbTable[i].rgbGreen == 192 && rgbTable[i].rgbBlue == 192 )
                    {
                        rgbTable[i] = rgbFace;
                    }
                    else
                    {
                        rgbTable[i].rgbRed   = (rgbTable[i].rgbRed   * rgbFace.rgbRed  ) / 192;
                        rgbTable[i].rgbGreen = (rgbTable[i].rgbGreen * rgbFace.rgbGreen) / 192;
                        rgbTable[i].rgbBlue  = (rgbTable[i].rgbBlue  * rgbFace.rgbBlue ) / 192;
                    }
                }

                SetDIBColorTable(dc, 0, n, rgbTable);
                SelectObject(dc, hbmSave);
                DeleteDC(dc);
            }
        }
#endif

        if (pbmpCache->hbmp)
            DeleteObject(pbmpCache->hbmp);
        pbmpCache->hbmp = hbmp;
        pbmpCache->cr3D = cr3D;
    }

    LEAVECRITICAL;

    return hbmp;
}

HBITMAP CInternetToolbar::_LoadBackBitmap()
{
    if (SHIsLowMemoryMachine(ILMM_IE4))
        return NULL;

    if (_fInitialPidlIsWeb)
    {
        static LPTSTR s_pszBitmapInternet = NULL;
        return _LoadBackBmp(&s_pszBitmapInternet, &s_bmpBackInternet, _fInitialPidlIsWeb);
    }
    else
    {
        static LPTSTR s_pszBitmapShell = NULL;
        return _LoadBackBmp(&s_pszBitmapShell, &s_bmpBackShell, _fInitialPidlIsWeb);
    }
}

void CInternetToolbar::_SetBackground()
{
    REBARBANDINFO   rbbi;
    HBITMAP         hbmp;

    // Theater mode doesn't allow bitmap customization, so don't bother loading one from the cache
    if (_fTheater)
        hbmp = NULL;
    else
        hbmp = _LoadBackBitmap();

    // don't bother updating the bkcolor if we know we'll just set it to CLR_NONE below (otherwise rebar invalidates)
    if (!hbmp)
        SendMessage(_bs._hwnd, RB_SETBKCOLOR, 0, (LPARAM)GetSysColor(COLOR_BTNFACE));

    // If we think we have a bitmap, or the cache thinks we have a bitmap, we have some work to do
    if (_bmpBack || hbmp)
    {
        BOOL fRemove = (NULL!=_bmpBack && NULL==hbmp);

        if (hbmp)
            SendMessage(_bs._hwnd, RB_SETBKCOLOR, 0, (LPARAM)CLR_NONE);
        _bmpBack = hbmp;

        rbbi.cbSize = sizeof(REBARBANDINFO);

        INT_PTR fRedraw = SendMessage(_bs._hwnd, WM_SETREDRAW, FALSE, 0);

        INT icBands = (INT) SendMessage( _bs._hwnd, RB_GETBANDCOUNT, 0, 0 );
        for (int i = 0; i < icBands; i++)
        {
            rbbi.fMask = RBBIM_ID | RBBIM_CHILD | RBBIM_BACKGROUND;
            if (SendMessage(_bs._hwnd, RB_GETBANDINFO, i, (LPARAM) &rbbi))
            {
                if (rbbi.wID != CBIDX_BRAND && rbbi.hbmBack != hbmp)
                {
                    rbbi.fMask = RBBIM_BACKGROUND;
                    rbbi.hbmBack = hbmp;
                    SendMessage(_bs._hwnd, RB_SETBANDINFO, i, (LPARAM) &rbbi);
                    InvalidateRect(rbbi.hwndChild, NULL, TRUE);
                }
            }
        }
        SendMessage(_bs._hwnd, WM_SETREDRAW, fRedraw, 0);

        // When removing the background bitmap, we need to invalidate *outside*
        // of the WM_SETREDRAW so we actually erase the background properly
        //
        if (fRemove)
            InvalidateRect(_bs._hwnd, NULL, TRUE);

    }
}


HRESULT CInternetToolbar::_ShowBrand(PBANDSAVE pbs)
{
    REBARBANDINFO       rbbi;
    LPBANDITEMDATA      pbid;
    INT_PTR             i;
    HRESULT             hr = S_OK;
    BOOL                fCreated = FALSE;

    pbid = _bs._GetBandItemDataStructByID(CBIDX_BRAND);
    if (!pbid)
    {
        IDeskBand *pdbBrandBand;
        hr = CBrandBand_CreateInstance(NULL, (IUnknown **)&pdbBrandBand, NULL);
        if (SUCCEEDED(hr))
        {
            pbid = _AddNewBand(pdbBrandBand, CBIDX_BRAND);
            fCreated = TRUE;
            pdbBrandBand->Release();
        }
        else
            return hr;
    }

    if (!pbid)
        return E_OUTOFMEMORY;

    pbid->pdb->ShowDW(TRUE);

    i = BandIDtoIndex(_bs._hwnd, CBIDX_BRAND);
    if (fCreated)
    {
        // add these to ::IDeskBand::GetBandInfo()
        rbbi.cbSize = sizeof(REBARBANDINFO);
        rbbi.fMask = RBBIM_STYLE;
        rbbi.fStyle = RBBS_FIXEDSIZE | RBBS_VARIABLEHEIGHT;

        if (pbs)
        {
            rbbi.fMask |= RBBIM_SIZE;
            rbbi.fStyle |= pbs->fStyle;
            rbbi.cx = pbs->cx;
        }
        SendMessage(_bs._hwnd, RB_SETBANDINFO, i, (LPARAM)&rbbi);
        // this can cause the band to move because a fixed size band
        // is forced in a particular location.
        // so we need to re-fetch the index
        i = BandIDtoIndex(_bs._hwnd, CBIDX_BRAND);
    }
    SendMessage(_bs._hwnd, RB_SHOWBAND, i, _nVisibleBands & VBF_BRAND);
    return S_OK;
}

void CInternetToolbar::_EnsureAllBandsShown()
{
    if (_hwnd) {

        INT_PTR fRedraw = SendMessage(_bs._hwnd, WM_SETREDRAW, FALSE, 0);

        _ShowMenu(NULL);
        _ShowTools(NULL);
        _ShowAddressBand(NULL);
        _ShowLinks(NULL);
        _ShowBrand(NULL);
        for (int i = CBIDX_EXTERNALFIRST; i <= CBIDX_EXTERNALLAST; i++) {
            _ShowExternalBand( NULL, i );
        }
        _SetBackground();
        _bs._SetMinDimensions();
        SendMessage(_bs._hwnd, WM_SETREDRAW, fRedraw, 0);
    }
}

BOOL CInternetToolbar::_ShowBands(UINT fVisible)
{
    fVisible &= VBF_VALID;

    if (fVisible == _nVisibleBands)
        return(TRUE);

    _nVisibleBands = fVisible;
    _EnsureAllBandsShown();
    ShowDW(_fShow);

    return(TRUE);
}

HRESULT CInternetToolbar::_CreateBands()
{
    HRESULT hres = S_OK;

    if (!_hwnd && _ptbsite)
    {
        HWND hwndParent;

        hres= _ptbsite->GetWindow(&hwndParent);
        if (SUCCEEDED(hres))
        {
            TCHAR szScratch[16];
            int i;

            // Check if coolbar layout had already been loaded from the registry
            if(_cs.cbVer != CBS_VERSION)
            {
                TraceMsg(DM_ITBAR, "CInternetToolbar:ShowDW failed. Bad Version");
                ASSERT(0);

                return(S_FALSE);
            }

            _nVisibleBands = _cs.uiVisible;

            _InitComCtl32();    // don't check result, if this fails our CreateWindows will fail

            MLLoadString(IDS_WEB_TB_TEXTROWS, szScratch, ARRAYSIZE(szScratch));
            _uiTBTextRows = _uiTBDefaultTextRows = StrToInt(szScratch);

            _fCompressed = (_cs.fNoText != FALSE);

            _hwnd = SHCreateWorkerWindow(SizableWndProc, hwndParent, 0, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                       (HMENU)FCIDM_REBAR, this);

            if (!IS_VALID_HANDLE(_hwnd, WND))
            {
                TraceMsg(TF_ERROR, "_hwnd failed");
                return E_OUTOFMEMORY;
            }

            // delay until now
            // this sets up the parent child chain so that these children can
            // queryservice through us
            SetClient(SAFECAST(&_bs, IInputObjectSite*));

            INT_PTR fRedraw = SendMessage(_bs._hwnd, WM_SETREDRAW, FALSE, 0);

            for (i = 0; i < CBANDSMAX; i++)
            {
                hres = S_OK;
                switch (_cs.bs[i].wID)
                {
                case CBIDX_TOOLS:
                    if(!SHRestricted2W(REST_NoToolBar, NULL, 0))
                    {
                        hres = _ShowTools(_cs.bs + i);
                    }
                    else
                    {
                        _nVisibleBands &= ~VBF_TOOLS;
                    }
                    break;

                case CBIDX_ADDRESS:
                    if(!SHRestricted2W(REST_NoAddressBar, NULL, 0))
                    {
                        hres = _ShowAddressBand(_cs.bs + i);
                    }
                    else
                    {
                        _nVisibleBands &= ~VBF_ADDRESS;
                    }
                    break;

                case CBIDX_LINKS:
                    if(!SHRestricted2W(REST_NoLinksBar, NULL, 0))
                    {
                        hres = _ShowLinks(_cs.bs + i);
                    }
                    else
                    {
                        _nVisibleBands &= ~VBF_LINKS;
                    }
                    break;

                case CBIDX_BRAND:
                    hres = _ShowBrand(_cs.bs + i);
                    break;

                case CBIDX_MENU:
                    hres = _ShowMenu(_cs.bs + i);
                    break;

                case 0xFFFFFFFF:
                {
                    // Out of bands; stop loop.
                    i = CBANDSMAX;
                    break;
                }

                default:
                    if (IS_EXTERNALBAND(_cs.bs[i].wID))
                    {
                        for (DWORD j = 0; j < MAXEXTERNALBANDS; j++)
                        {
                            if (_cs.aclsidExternalBands[MAP_TO_EXTERNAL(_cs.bs[i].wID)] == _rgebi[j].clsid)
                            {
                                CLSID clsidTemp = _rgebi[j].clsid;
                                _rgebi[j].clsid = _rgebi[MAP_TO_EXTERNAL(_cs.bs[i].wID)].clsid;
                                _rgebi[MAP_TO_EXTERNAL(_cs.bs[i].wID)].clsid = clsidTemp;
                                hres = _ShowExternalBand(_cs.bs + i, _cs.bs[i].wID);
                                break;
                            }
                        }
                    }
                    break;
                }

                if (hres != S_OK)
                    TraceMsg(TF_ERROR, "CInternetToolbar::_CreateBands -- band ID %x creation failed", _cs.bs[i].wID);
            }

            // Add any remaining bands
            _EnsureAllBandsShown();

            _SetBackground();
            _bs._SetMinDimensions();

            SendMessage(_bs._hwnd, WM_SETREDRAW, fRedraw, 0);
        }
    }
    return hres;
}

HRESULT CInternetToolbar::ShowDW(BOOL fShow)
{
    if ((g_dwProfileCAP & 0x00000008) && s_imlTBGlyphs.arhiml[0]) {
        StartCAP();
    }

    HRESULT hres = _CreateBands();
    if (FAILED(hres))
        return hres;

    if (!_nVisibleBands && fShow)
        return(FALSE);

    _fShow = fShow;

    _bs.UIActivateDBC(fShow ? DBC_SHOW : DBC_HIDE);

    ResizeBorderDW(NULL, NULL, FALSE);
    ShowWindow(_hwnd, fShow ? SW_SHOW : SW_HIDE);


    BOOL fConnect = (fShow && _dwcpCookie == 0);
    if (fConnect || (!fShow && _dwcpCookie != 0))
    {
        ConnectToConnectionPoint(SAFECAST(this, IDockingWindow*), DIID_DWebBrowserEvents2, fConnect, _pdie, &_dwcpCookie, NULL);
    }

    return hres;
}

int ITBar_TrackPopupMenuEx(HMENU hmenu, UINT uFlags, int x, int y, HWND hwnd, LPRECT prcExclude)
{
    TPMPARAMS tpm;
    if (prcExclude) {
        tpm.cbSize = SIZEOF(TPMPARAMS);
        CopyRect(&tpm.rcExclude, prcExclude);
    }
    return TrackPopupMenuEx(hmenu, uFlags, x, y, hwnd, prcExclude ? &tpm : NULL);
}

/*******************************************************************

NAME:       CInternetToolbar::_ShowBackForwardMenu

SYNOPSIS:
NOTES:
********************************************************************/
BOOL CInternetToolbar::_ShowBackForwardMenu(BOOL fForward, POINT pt, LPRECT prcExclude)
{
    BOOL fRet = FALSE;
    HMENU hmenuBF = CreatePopupMenu();

    ASSERT(hmenuBF);

    ASSERT(_pbs2);
    ITravelLog *ptl;
    _pbs2->GetTravelLog(&ptl);
    if(ptl)
    {
        if(S_OK == ptl->InsertMenuEntries(_pbs2, hmenuBF, 0, 1, MAX_NAV_MENUITEMS, fForward ? TLMENUF_FORE : TLMENUF_BACK))
        {
        // If any menu items were added, show the menu and navigate to it
            int nIndex;

#ifndef MAINWIN
            if (nIndex = ITBar_TrackPopupMenuEx (hmenuBF, TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, _hwnd, prcExclude))
#else
            // Because mainwin doesn't support win95 look and feel we are
            // having a problem to keep the popup from dismissing when we
            // pass NULL as noDismissal area.
            RECT rect;
            GetWindowRect( _hwnd, &rect );
            if (nIndex = (int)TrackPopupMenu (hmenuBF,
                TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                pt.x, pt.y, 0, _hwnd,
                &rect))
#endif
                ptl->Travel(_pbs2, (fForward ? nIndex : -nIndex));

            fRet = TRUE;
        }
        ptl->Release();
    }

    DestroyMenu (hmenuBF);
    return fRet;
}

// get per folder search items and default search
BOOL CInternetToolbar::_GetFolderSearchData()
{
    int iInserted=0;

    if (_pbs2)
    {
        LPCBASEBROWSERDATA pbbd;

        if (SUCCEEDED(_pbs2->GetBaseBrowserData(&pbbd)) && (pbbd->_psfPending || pbbd->_psf))
        {
            IShellFolder2 * psf2;
            IShellFolder*   psf = pbbd->_psfPending ? pbbd->_psfPending : pbbd->_psf;

            if (SUCCEEDED(psf->QueryInterface(IID_IShellFolder2, (LPVOID*)&psf2)))
            {
                LPENUMEXTRASEARCH penum;
                GUID              guid;

                if (SUCCEEDED(psf2->GetDefaultSearchGUID(&guid)))
                    _guidDefaultSearch = guid;

                // get per folder search items
                if (_hdpaFSI && SUCCEEDED(psf2->EnumSearches(&penum)))
                {
                    EXTRASEARCH  xs;

                    while(penum->Next(1, &xs, NULL) == S_OK)
                    {
                        LPFOLDERSEARCHITEM pfsi = (LPFOLDERSEARCHITEM)LocalAlloc(LPTR, sizeof(FOLDERSEARCHITEM));
                        if (pfsi)
                        {
                            pfsi->idCmd = -1;
                            pfsi->guidSearch = xs.guidSearch;
                            StrCpyNW(pfsi->wszUrl, xs.wszUrl, ARRAYSIZE(pfsi->wszUrl));
                            StrCpyNW(pfsi->wszName, xs.wszFriendlyName, ARRAYSIZE(pfsi->wszName));

                            if (DPA_InsertPtr(_hdpaFSI, iInserted, pfsi) != -1)
                                iInserted++;
                            else
                                LocalFree(pfsi);
                        }
                    }
                    penum->Release();
                }
                psf2->Release();
            }
        }
    }

    return (iInserted > 0);
}

BOOL NavigateSearchBar(IWebBrowser2 *pwb2, LPCWSTR pwszUrl)
{
    BOOL          bRet  = FALSE;
    SA_BSTR       bstr;
    VARIANT       var;
    VARIANT       varEmpty = {0};

    SHStringFromGUIDW(CLSID_SearchBand, bstr.wsz, ARRAYSIZE(bstr.wsz));
    bstr.cb = lstrlenW(bstr.wsz) * sizeof(WCHAR);

    var.vt = VT_BSTR;
    var.bstrVal = bstr.wsz;

    // show a search bar
    if (SUCCEEDED(pwb2->ShowBrowserBar(&var, &varEmpty, &varEmpty)))
    {
        VARIANT varTargetFrame = {0};
        VARIANT varFlags = {0};

        StrCpyNW(bstr.wsz, pwszUrl, ARRAYSIZE(bstr.wsz));
        bstr.cb = lstrlenW(bstr.wsz) * sizeof(WCHAR);

        varFlags.vt = VT_I4;
        varFlags.lVal = navBrowserBar;

        var.vt = VT_BSTR;
        var.bstrVal = bstr.wsz;

        // navigate the search bar to the correct url
        if (SUCCEEDED(pwb2->Navigate2(&var, &varFlags, &varTargetFrame, &varEmpty, &varEmpty)))
            bRet = TRUE;
    }
    return bRet;
}

void RestrictItbarViewMenu(HMENU hmenu, IUnknown *punkBar )
{
    BOOL fIsRestricted = SHRestricted2(REST_NOBANDCUSTOMIZE, NULL, 0);
    if (fIsRestricted) {
        _EnableMenuItem(hmenu, FCIDM_VIEWLINKS, FALSE);
        _EnableMenuItem(hmenu, FCIDM_VIEWMENU, FALSE);
        _EnableMenuItem(hmenu, FCIDM_VIEWADDRESS, FALSE);
        _EnableMenuItem(hmenu, FCIDM_VIEWTOOLS, FALSE);
    }

    for (int i = 0; i < MAXEXTERNALBANDS; i++) {
        OLECMD cmd = { CITIDM_VIEWEXTERNALBAND_FIRST + i, 0 };
        OLECMDTEXTV<MAX_EXTERNAL_BAND_NAME_LEN> cmdtv;
        OLECMDTEXT *pcmdText = &cmdtv;
        pcmdText->cmdtextf = OLECMDTEXTF_NAME;
        pcmdText->cwActual = 0;
        pcmdText->cwBuf = MAX_EXTERNAL_BAND_NAME_LEN;

        IUnknown_QueryStatus( punkBar, &CGID_PrivCITCommands, 1, &cmd, pcmdText );
        if (cmd.cmdf & OLECMDF_SUPPORTED) {
            DWORD dwMenuCommand = FCIDM_EXTERNALBANDS_FIRST + i;
            InsertMenu( hmenu, FCIDM_VIEWCONTEXTMENUSEP, MF_BYCOMMAND, dwMenuCommand, pcmdText->rgwz );
            if (cmd.cmdf & OLECMDF_ENABLED) {
                _CheckMenuItem( hmenu, dwMenuCommand, TRUE );
            }
            if (fIsRestricted) {
                _EnableMenuItem( hmenu, dwMenuCommand, FALSE );
            }
        }
    }
}

void CInternetToolbar::_ShowContextMenu(HWND hwnd, LPARAM lParam, LPRECT prcExclude)
{
    // Bail if this context menu doesn't correspond to a band (fixes NT5 #181899)
    POINT pt;
    int iIndex = _bs._ContextMenuHittest(lParam, &pt);
    int idBandActive = _bs._IndexToBandID(iIndex);
    if (!InRange(idBandActive, CBIDX_FIRST, CBANDSMAX))
        return;

    // Bail if we can't find the resource
    HMENU hmenuITB = LoadMenuPopup(MENU_ITOOLBAR);
    if (!hmenuITB)
        return;

    UEMFireEvent(&UEMIID_SHELL, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_UICONTEXT, idBandActive == -1 ? UIBL_CTXTITBBKGND : UIBL_CTXTITBITEM);

    // Set the initial state of the menu
    _CheckMenuItem (hmenuITB, FCIDM_VIEWTOOLS, _nVisibleBands & VBF_TOOLS);
    _CheckMenuItem (hmenuITB, FCIDM_VIEWADDRESS, _nVisibleBands & VBF_ADDRESS);
    _CheckMenuItem (hmenuITB, FCIDM_VIEWLINKS, _nVisibleBands & VBF_LINKS);

    int cItemsBelowSep = 4;

    // only in theater mode can we autohide
    if (!_fTheater)
    {
        DeleteMenu(hmenuITB, FCIDM_VIEWAUTOHIDE, MF_BYCOMMAND);
        cItemsBelowSep--;
        if (_nVisibleBands & VBF_MENU || _fNoShowMenu)
            DeleteMenu(hmenuITB, FCIDM_VIEWMENU, MF_BYCOMMAND);
    }
    else
    {
        if (_fNoShowMenu)
            DeleteMenu(hmenuITB, FCIDM_VIEWMENU, MF_BYCOMMAND);
        DeleteMenu(hmenuITB, FCIDM_VIEWTOOLS, MF_BYCOMMAND);
        _CheckMenuItem (hmenuITB, FCIDM_VIEWAUTOHIDE, _fAutoHide);
        _CheckMenuItem (hmenuITB, FCIDM_VIEWMENU, _nVisibleBands & VBF_MENU);
    }

    // if it was done via the keyboard, but focus wasn't on the tools band,
    // then don't have customize menu option
    // or if click didn't happen on the band
    if (!(_btb._fCustomize && idBandActive == CBIDX_TOOLS))
    {
        DeleteMenu(hmenuITB, FCIDM_VIEWTOOLBARCUSTOMIZE, MF_BYCOMMAND);
        cItemsBelowSep--;
    }

    BOOL fGoButtonAvailable =
        WasOpenedAsBrowser(static_cast<IExplorerToolbar *>(this)) || (GetUIVersion() >= 5);

    // Only show the go button item when you click on the address bar
    if (idBandActive != CBIDX_ADDRESS || !fGoButtonAvailable)
    {
        DeleteMenu(hmenuITB, FCIDM_VIEWGOBUTTON, MF_BYCOMMAND);
        cItemsBelowSep--;
    }
    else
    {
        BOOL fShowGoButton = SHRegGetBoolUSValue(REGSTR_PATH_MAIN,
                                TEXT("ShowGoButton"), FALSE, /*default*/TRUE);
        _CheckMenuItem(hmenuITB, FCIDM_VIEWGOBUTTON, fShowGoButton);
    }

    if (_fTheater || _btb._fCustomize || SHRestricted2(REST_LOCKICONSIZE, NULL, 0))
    {
        DeleteMenu(hmenuITB, FCIDM_VIEWTEXTLABELS, MF_BYCOMMAND);
        cItemsBelowSep--;
    }
    else
    {
        // If customize is unavailable, then there's no way for the user to
        // turn list style on/off.  In this case we want toggling text labels
        // to work the way it did in IE4 -- that is, switch between "text on
        // all buttons" and "text on no buttons".  So, if we're in "selective
        // text on right" mode, we say that labels are turned off.  If the user
        // picks this menu option, we'll go into "text on all buttons" mode.

        BOOL fChecked = !_fCompressed && !IS_LIST_STYLE(_btb._hwnd);
        _CheckMenuItem(hmenuITB, FCIDM_VIEWTEXTLABELS, fChecked);
    }

    if (!cItemsBelowSep)
        DeleteMenu(hmenuITB, FCIDM_VIEWCONTEXTMENUSEP, MF_BYCOMMAND);

    RestrictItbarViewMenu(hmenuITB, SAFECAST( this, IOleCommandTarget* ) );
    ITBar_TrackPopupMenuEx(hmenuITB, TPM_LEFTBUTTON | TPM_RIGHTBUTTON, pt.x, pt.y, _hwnd, prcExclude);

    // HACK: since the ITBar isn't a real bar/bandsite, we have to
    // do this so any menuband that might be up can take back the
    // mouse capture.
    LPBANDITEMDATA pbid = _bs._GetBandItemDataStructByID(CBIDX_MENU);
    if (pbid)
        IUnknown_Exec(pbid->pdb, &CGID_MenuBand, MBANDCID_RECAPTURE, 0, NULL, NULL);

    DestroyMenu (hmenuITB);
}


void CInternetToolbar::_QueryStatusTip(IOleCommandTarget *pct, LPTOOLTIPTEXT pnmTT, UINT uiCmd, const GUID* pguid)
{
    OLECMD rgcmd = { uiCmd, 0 };
    OLECMDTEXTV<MAX_TOOLTIP_STRING> cmdtv;
    OLECMDTEXT *pcmdText = &cmdtv;

    pcmdText->cwBuf = MAX_TOOLTIP_STRING;
    pcmdText->cmdtextf = OLECMDTEXTF_NAME;
    pcmdText->rgwz[0] = 0;
    pct->QueryStatus(pguid, 1, &rgcmd, pcmdText);
    if (rgcmd.cmdf & OLECMDF_ENABLED)
    {
        SHUnicodeToTChar(pcmdText->rgwz, pnmTT->szText, MAX_TOOLTIP_STRING);
    }

}

BOOL _IsDocHostGUID(const GUID* pguid)
{
    // Dochost merges under one of two clsids, so have to check both
    BOOL fRet = IsEqualGUID(*pguid, CLSID_InternetButtons) ||
                IsEqualGUID(*pguid, CLSID_MSOButtons);
    return fRet;
}

void CInternetToolbar::_OnTooltipNeeded(LPTOOLTIPTEXT pnmTT)
{

    UINT uiCmd;
    GUID guid;

    ASSERT(pnmTT->hdr.hwndFrom == (HWND)SendMessage(_btb._hwnd, TB_GETTOOLTIPS, 0, 0));
    _btb._ConvertCmd(NULL, (UINT)pnmTT->hdr.idFrom, &guid, &uiCmd);

    // Make sure tooltips don't filter out ampersands
    LONG lStyle = GetWindowLong(pnmTT->hdr.hwndFrom, GWL_STYLE);
    if (!IsFlagSet(lStyle, TTS_NOPREFIX))
    {
        SetWindowLong(pnmTT->hdr.hwndFrom, GWL_STYLE, lStyle | TTS_NOPREFIX);
    }

    if (IsEqualGUID(guid, CLSID_CommonButtons))
    {
        switch (uiCmd)
        {

        case TBIDM_FORWARD:
        case TBIDM_BACK:
            if (_ptbsite)
            {
                IBrowserService *pbsvc;

                if (SUCCEEDED(_ptbsite->QueryInterface(IID_IBrowserService, (void **)&pbsvc)))
                {
                    // BUGBUG raymondc - make ITravelLog UNICODE someday
                    ITravelLog *ptl;
                    pbsvc->GetTravelLog( &ptl );
                    if (ptl)
                    {
                        WCHAR szTemp[ARRAYSIZE(pnmTT->szText)];
                        if (uiCmd == TBIDM_BACK)
                            ptl->GetToolTipText(pbsvc, TLOG_BACK, 0, szTemp, ARRAYSIZE(szTemp));
                        else if (uiCmd == TBIDM_FORWARD)
                            ptl->GetToolTipText(pbsvc, TLOG_FORE, 0, szTemp, ARRAYSIZE(szTemp));

                        SHUnicodeToTChar(szTemp, pnmTT->szText, ARRAYSIZE(pnmTT->szText));

                        ptl->Release();
                    }
                    pbsvc->Release();
                    return;
                }
            }
        }
    }
    // Dochost merges under one of two clsids, so have to check both
    else if (_IsDocHostGUID(&guid))
    {
#ifdef EDIT_HACK
        if (uiCmd == DVIDM_EDITPAGE)
            _aEditVerb.GetToolTip(pnmTT->szText, ARRAYSIZE(pnmTT->szText));
#endif
    }
}

LRESULT CInternetToolbar::_OnBeginDrag(NMREBAR *pnm)
{
    if (SHRestricted2(REST_NOBANDCUSTOMIZE, NULL, 0)) {
        return 1;
    }

    if (_fTheater) {
        // if we're in theater mode, we do our own drag handling where we force
        // all  the mouse moves into the middle of the band, thereby disallowing
        // the user to make a multi line rebar
        SetCapture(_hwnd);
        SendMessage(_bs._hwnd, RB_BEGINDRAG, pnm->uBand, (LPARAM)-2);
        _fRebarDragging = TRUE;
        return 1;
    }
    return SHRestricted2(REST_NoToolbarOptions, NULL, 0);
}

LRESULT CInternetToolbar::_OnNotify(LPNMHDR pnmh)
{
    LRESULT lres = 0;
    if (!_pdie)
        return 0;

    if (pnmh->code == TTN_NEEDTEXT  && pnmh->hwndFrom == (HWND)SendMessage(_btb._hwnd, TB_GETTOOLTIPS, 0, 0)) {
        _OnTooltipNeeded((LPTOOLTIPTEXT)pnmh);
        return 0;
    }

    if(_SendToToolband(pnmh->hwndFrom, WM_NOTIFY,0, (LPARAM)pnmh, &lres))
        return lres;

    switch (pnmh->idFrom) {
    case FCIDM_REBAR:
        switch (pnmh->code) {

        case RBN_BEGINDRAG:
            return _OnBeginDrag((NMREBAR*)pnmh);

        case RBN_HEIGHTCHANGE:
            ResizeBorderDW(NULL, NULL, FALSE);
            break;

        case RBN_CHILDSIZE:
        {
            // make the brand always take the full height
            NMREBARCHILDSIZE *pnm = (NMREBARCHILDSIZE*)pnmh;
            if (pnm->wID == CBIDX_BRAND) {
                pnm->rcChild.top = pnm->rcBand.top;
                pnm->rcChild.bottom = pnm->rcBand.bottom;
            }
            break;
        }

        case RBN_LAYOUTCHANGED:
            //Because the layout has changed, remember to save later!
            _fDirty = TRUE;
            _UpdateBrandSize();

            if (_ptbsitect)
                _ptbsitect->Exec(&CGID_ShellBrowser, FCIDM_PERSISTTOOLBAR, 0, NULL, NULL);
            break;

        case RBN_GETOBJECT:
        {
            NMOBJECTNOTIFY *pnmon = (NMOBJECTNOTIFY *)pnmh;
            if (IsEqualIID(*pnmon->piid, IID_IDropTarget))
            {
                HWND hwnd;

                switch (pnmon->iItem) {
                case CBIDX_MENU:
                case CBIDX_LINKS:
                {
                     LPBANDITEMDATA pbid = _bs._GetBandItemDataStructByID(pnmon->iItem);
                     if (pbid && pbid->pdb)
                     {
                        pnmon->hResult = pbid->pdb->QueryInterface(IID_IDropTarget, (LPVOID*)&pnmon->pObject);
                     }
                     break;
                }

                case CBIDX_TOOLS:
                    hwnd = _btb._hwnd;
                    pnmon->hResult = (HRESULT)SendMessage(hwnd, TB_GETOBJECT,
                                        (WPARAM)&IID_IDropTarget, (LPARAM)&pnmon->pObject);
                    break;

                case CBIDX_ADDRESS:
                    if (_ptbsite) {
                        pnmon->hResult = _ptbsite->QueryInterface(IID_IDropTarget, (LPVOID*)&pnmon->pObject);
                    }
                    break;
                }
            }
            ASSERT((SUCCEEDED(pnmon->hResult) && pnmon->pObject) ? (IS_VALID_CODE_PTR(pnmon->pObject, IUnknown)) : (pnmon->pObject == NULL));
            return TRUE;
        }

        case RBN_CHEVRONPUSHED:
        {
            LPNMREBARCHEVRON pnmch = (LPNMREBARCHEVRON) pnmh;
            if (pnmch->wID == CBIDX_TOOLS)
            {
                int idMenu = MENU_TBMENU;
                // this must be the tools band (not enumerated in bandsite)
                MapWindowPoints(pnmh->hwndFrom, HWND_DESKTOP, (LPPOINT)&pnmch->rc, 2);
                if (!_btb._fCustomize)
                    idMenu = 0;

                ToolbarMenu_Popup(_hwnd, &pnmch->rc, NULL, _btb._hwnd, idMenu, (DWORD)pnmch->lParamNM);
                return TRUE;
            }
            _bs._OnNotify(pnmh);
            break;
        }

        default:
            return _bs._OnNotify(pnmh);

        } // switch (pnmh->code)
        break;

    default:
        //AssertMsg(0, "Unknown notify from hwnd(%x), id(%x)", pnmh->hwndFrom, pnmh->idFrom);
        break;

    } // switch (pnmh->idFrom)
    return 0;
}


/*******************************************************************

NAME:       CInternetToolbar::_DoNavigateA

SYNOPSIS:   Called when the user types in or selects a URL
to navigate to through the address bar

NOTES:      This function calls the helper function _DoNavigate.
This is just here to convert the ANSI string to
a BSTR.

********************************************************************/
HRESULT CInternetToolbar::_DoNavigateA(LPSTR pszURL, int iNewSelection)
{
    ASSERT(pszURL);    // must have valid URL to navigate to

    SA_BSTR sstrText;
    sstrText.cb = SHAnsiToUnicode(pszURL, sstrText.wsz, ARRAYSIZE(sstrText.wsz));

    return _DoNavigate(sstrText.wsz, iNewSelection);
}

/*******************************************************************

NAME:       CInternetToolbar::_DoNavigateW

SYNOPSIS:   Called when the user types in or selects a URL
to navigate to through the address bar

NOTES:      This function calls the helper function _DoNavigate.
This is just here to convert the Unicode string to
a BSTR.

********************************************************************/
HRESULT CInternetToolbar::_DoNavigateW(LPWSTR pwzURL, int iNewSelection)
{
    ASSERT(pwzURL);    // must have valid URL to navigate to

    SA_BSTR sstrPath;
    StrCpyNW(sstrPath.wsz, pwzURL, ARRAYSIZE(sstrPath.wsz));
    sstrPath.cb = lstrlenW(sstrPath.wsz) * SIZEOF(WCHAR);

    return _DoNavigate(sstrPath.wsz, iNewSelection);
}

/*******************************************************************

NAME:       CInternetToolbar::_DoNavigate

SYNOPSIS:   Called when the user types in or selects a URL
to navigate to through the address bar

ENTRY:      bstrURL - string of URL to navigate to, in BSTR form
iNewSelection - index of current selection in address
bar combo box

********************************************************************/
HRESULT CInternetToolbar::_DoNavigate(BSTR bstrURL,int iNewSelection)
{
    HRESULT hr;

    ASSERT(bstrURL); // must have valid URL to browse to
    ASSERT(_pdie);  // must have valid pointer to automation interface

    VARIANTARG v;
    VariantInit (&v);

    v.vt = VT_I4;
    v.lVal = navAllowAutosearch;

    // call automation interface to make browser navigate to this URL
    hr = _pdie->Navigate(bstrURL, &v, PVAREMPTY, PVAREMPTY, PVAREMPTY);

    VariantClearLazy(&v);

    return hr;
}

LPITEMIDLIST CInternetToolbar::_GetCurrentPidl(void)
{
    LPITEMIDLIST pidl = NULL;

    ASSERT(_pbs2);
    if (_pbs2)
    {
        _pbs2->GetPidl(&pidl);
    }

    return pidl;
}

void CInternetToolbar::_CommonHandleFileSysChange(LONG lEvent, LPITEMIDLIST* ppidl)
{
    // stuff that needs to be done tree or no tree
    switch (lEvent) {

        // README:
        // If you need to add events here, then you must change SHELLBROWSER_FSNOTIFY_FLAGS in
        // shbrowse.cpp in order to get the notifications
    case SHCNE_DRIVEREMOVED:
    case SHCNE_MEDIAREMOVED:
    case SHCNE_MEDIAINSERTED:
    case SHCNE_DRIVEADD:
    case SHCNE_UPDATEIMAGE:
        // Forward this command to CAddressBand::FileSysChange()
        // by using IToolbandHelper::OnWinEvent().
        ASSERT(_hwndAddressBand);
        {
            LPBANDITEMDATA pbid = _bs._GetBandItemDataStructByID(CBIDX_ADDRESS);

            ASSERT(pbid && pbid->pdb);     // pbid->pdb is a IDeskBand pointer
            if (pbid)
            {
                // REVIEW: why don't we use IShellChangeNotify here?
                //
                IUnknown_FileSysChange(pbid->pdb, (DWORD)lEvent, (LPCITEMIDLIST*)ppidl);
            }
        }
        break;
    }
}

HRESULT CInternetToolbar::OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
#ifdef DEBUG
    //
    // We are only called after we have been initialized
    //
    LPITEMIDLIST pidl = _GetCurrentPidl();
    if (EVAL(pidl))
    {
        ILFree(pidl);
    }
#endif

    LPITEMIDLIST ppidl[2] = {(LPITEMIDLIST)pidl1, (LPITEMIDLIST)pidl2};
    _CommonHandleFileSysChange(lEvent, ppidl);

    return S_OK;
}

void CInternetToolbar::_OnCommand(WPARAM wParam, LPARAM lParam)
{
    if (!_pdie)
        return;

    HWND hwndControl = GET_WM_COMMAND_HWND(wParam, lParam);
    UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);

    // If this is a command from the toolbar, and it is not one of the StdBrowseButtons
    // call Exec() on the appropriate CmdTarget
    if (hwndControl == _btb._hwnd)
    {
        UINT uiInternalCmdID = idCmd;

        // Convert to the real thing and get the guid
        CMDMAP* pcm = _btb._GetCmdMapByID(idCmd);

        IOleCommandTarget* pct = _btb._CommandTargetFromCmdMap(pcm);
        if (pct) {
            VARIANTARG var;
            var.vt = VT_I4;
            var.lVal = uiInternalCmdID;
            if (SHIsSameObject(_btb._pctCurrentButtonGroup, pct)) {
                // give the browser a chance to pick this off in case
                // focus doesn't belong to the view currently
                if (SUCCEEDED(_ptbsitect->Exec(&IID_IExplorerToolbar, pcm->nCmdID, 0, NULL, &var)))
                    return;
            }

            UEMFireEvent(&UEMIID_BROWSER, UEME_UITOOLBAR, UEMF_XEVENT,
                UIG_COMMON, (LPARAM)pcm->nCmdID);
            pct->Exec(&pcm->guidButtonGroup, (DWORD)pcm->nCmdID, 0, NULL, &var);
        }
        return;
    }

    if (_SendToToolband(hwndControl, WM_COMMAND, wParam, lParam, NULL))
        return;


    // this switch block actually executes
    switch(idCmd)
    {
    case FCIDM_VIEWTOOLBARCUSTOMIZE:
        ASSERT(!SHRestricted2(REST_NOTOOLBARCUSTOMIZE, NULL, 0));
        SendMessage (_btb._hwnd, TB_CUSTOMIZE, 0, 0L);
        break;

    case FCIDM_DRIVELIST:
        _SendToToolband(_hwndAddressBand, WM_COMMAND, wParam, lParam, NULL);
        break;

    case FCIDM_VIEWADDRESS:
    case FCIDM_VIEWTOOLS:
    case FCIDM_VIEWMENU:
    case FCIDM_VIEWLINKS:
        if (!SHRestricted2(REST_NOBANDCUSTOMIZE, NULL, 0)
            && !SHRestricted2(REST_NoToolbarOptions, NULL, 0)) {
            DWORD dw = _nVisibleBands;
            switch (idCmd)
            {
            case FCIDM_VIEWTOOLS:
                dw ^= VBF_TOOLS;
                break;
            case FCIDM_VIEWMENU:
                dw ^= VBF_MENU;
                break;
            case FCIDM_VIEWADDRESS:
                dw ^= VBF_ADDRESS;
                break;
            case FCIDM_VIEWLINKS:
                dw ^= VBF_LINKS;
                break;
            }
            if ( !( dw & ~VBF_BRAND))
            {
                _pdie->put_ToolBar( FALSE );
            }

            _ShowVisible(dw, TRUE);
        }
        return;

    case FCIDM_VIEWAUTOHIDE:
    {
        ASSERT(_fTheater);
        _fAutoHide = !_fAutoHide;

        VARIANTARG v = {0};
        v.vt = VT_I4;
        v.lVal = _fAutoHide;
        IUnknown_Exec(_ptbsite, &CGID_Theater, THID_SETTOOLBARAUTOHIDE, 0, &v, NULL);

        ResizeBorderDW(NULL, NULL, FALSE);

        break;
    }

    case FCIDM_VIEWTEXTLABELS:
        if(!SHRestricted2(REST_NoToolbarOptions, NULL, 0))
        {
            if (!_btb._fCustomize && IS_LIST_STYLE(_btb._hwnd))
            {
                // If customize is unavailable, then there's no way for the user to
                // turn list style on/off.  In this case we want toggling text labels
                // to work the way it did in IE4 -- that is, switch between "text on
                // all buttons" and "text on no buttons".  So, if we're in "selective
                // text on right" mode, we say that labels are turned off.  If the user
                // picks this menu option, we'll go into "text on all buttons" mode.

                _UpdateToolsStyle(FALSE);

                // Make ourselves believe that text labels are turned off (so
                // that _UpdateToolbarDisplay will turn them on)

                _fCompressed = TRUE;
            }
            _UpdateToolbarDisplay(UTD_TEXTLABEL, 0, !_fCompressed, TRUE);
        }
        return;

    case FCIDM_VIEWGOBUTTON:
        ASSERT(_hwndAddressBand);
        _SendToToolband(_hwndAddressBand, WM_COMMAND, wParam, lParam, NULL);
        break;

    default:
        if (InRange( idCmd, FCIDM_EXTERNALBANDS_FIRST, FCIDM_EXTERNALBANDS_LAST ))
        {
            if (!SHRestricted2(REST_NOBANDCUSTOMIZE, NULL, 0))
            {
                DWORD dw = _nVisibleBands;
                dw ^= EXTERNALBAND_VBF_BIT(idCmd - FCIDM_EXTERNALBANDS_FIRST);
                if ( !( dw & ~VBF_BRAND))
                {
                    _pdie->put_ToolBar( FALSE );
                }
                _ShowVisible(dw, TRUE);
            }
            return;
        }
        break;
    }
}

// get the doc property, then get the command target from that and do it
void CInternetToolbar::_SendDocCommand(UINT idCmd)
{
    // BUGBUG: if this fails, we should maybe do the invoke with a
    // proper verb?
    if (_ptbsitect)
    {
        VARIANTARG varIn;
        VARIANTARG varOut;
        VARIANTARG *pvarIn = &varIn;
        VARIANTARG *pvarOut = &varOut;

        VariantInit(&varIn);
        VariantInit(&varOut);


        switch (idCmd) {
        case OLECMDID_HIDETOOLBARS:
        case OLECMDID_PRINT:
            // word & excel barf if there are arguments passed to print
            pvarIn = NULL;
            pvarOut = NULL;
            break;

        case OLECMDID_ZOOM:
            // get the zoom range
            int iMax, iMin;
            if (FAILED(_ptbsitect->Exec(NULL, OLECMDID_GETZOOMRANGE, OLECMDEXECOPT_DONTPROMPTUSER, NULL, &varOut)))
                goto Bail;

            if (varOut.vt == VT_I4)
            {
                iMin = (int)(short)LOWORD(varOut.lVal);
                iMax = (int)(short)HIWORD(varOut.lVal);
            }
            else
                goto Bail;

            varOut.vt = VT_EMPTY; // return to VariantInit state

            // get the current zoom depth
            if (FAILED(_ptbsitect->Exec(NULL, OLECMDID_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER, NULL, &varIn)))
                goto Bail;

            if (varIn.vt == VT_I4) {
                varIn.lVal++;
                if ((int)varIn.lVal > iMax) {
                    varIn.lVal = iMin;
                }
            } else {
                goto Bail;
            }

            break;

        }

#ifdef FEATURE_IE40
        _ptbsitect->Exec(NULL, idCmd, OLECMDEXECOPT_DONTPROMPTUSER, pvarIn, pvarOut);
#else
        _ptbsitect->Exec(NULL, idCmd, OLECMDEXECOPT_PROMPTUSER, pvarIn, pvarOut);
#endif

Bail:
    VariantClearLazy(&varIn);
    VariantClearLazy(&varOut);

    }
}

BOOL CInternetToolbar::_CompressBands(BOOL fCompress, UINT uRowsNew, BOOL fForceUpdate)
{
    UINT_PTR uRowsOld = SendMessage(_btb._hwnd, TB_GETTEXTROWS, 0, 0L);
    if (fCompress)
        uRowsNew = 0;

    if (!fForceUpdate && (uRowsOld == uRowsNew)) {
        // same as what we've already got, blow it off
        return FALSE;
    }

    _fCompressed = fCompress;

    // Change the size of the Brand window and add ot remove the text
    SendMessage(_btb._hwnd, TB_SETMAXTEXTROWS, uRowsNew, 0L);

    UINT uWidthNew = _fCompressed ? MAX_TB_COMPRESSED_WIDTH : _uiMaxTBWidth;
    SendMessage(_btb._hwnd, TB_SETBUTTONWIDTH, 0, (LPARAM) MAKELONG(0, uWidthNew));

    _btb._BandInfoChanged();

    _UpdateBrandSize();
    _bs._SetMinDimensions();

    return TRUE;
}

#define ABS(x)  (((x) < 0) ? -(x) : (x))

void CInternetToolbar::_TrackSliding(int x, int y)
{
    INT_PTR cBands    = SendMessage(_bs._hwnd, RB_GETBANDCOUNT, 0, 0L);
    INT_PTR cRows     = SendMessage(_bs._hwnd, RB_GETROWCOUNT, 0, 0L);
    INT_PTR cyHalfRow = SendMessage(_bs._hwnd, RB_GETROWHEIGHT, cBands-1, 0L) / 2;
    RECT rc;
    int cyBefore;
    int c;
    BOOL_PTR fChanged = FALSE;

    // do this instead of GetClientRect so that we include borders
    GetWindowRect(_bs._hwnd, &rc);
    MapWindowPoints(HWND_DESKTOP, _bs._hwnd, (LPPOINT)&rc, 2);
    cyBefore = rc.bottom - rc.top;

    c = y - _yCapture;
    rc.bottom = y;

    // was there enough change?
    if (ABS(c) <= cyHalfRow)
        return;

    if ((cRows == 1) || _fCompressed) {

        if (c < -cyHalfRow)
            fChanged = _CompressBands(TRUE, 0, FALSE);
        else
            fChanged = _CompressBands(FALSE, _uiTBTextRows, FALSE);

    }

    if (!fChanged) {
        // if the compressing bands didn't change anything, try to fit it to size
        fChanged = SendMessage(_bs._hwnd, RB_SIZETORECT, 0, (LPARAM)&rc);
    }


    // TODO: There is a drawing glitch when you resize from 3 bars (No Text) to 3 bars
    // with text. The _yCapture gets set to a value greater than y. So on the
    // next MOUSEMOVE it figures that the user moved up and switches from 3 bars with text
    // to 2 bars with text.
    if (fChanged)
    {
        _UpdateBrandSize();
        GetWindowRect(_bs._hwnd, &rc);
        _yCapture += (rc.bottom - rc.top) - cyBefore;
        _fDirty = TRUE; //Since the band layout changed, set the dirty bit ON.
        if (_ptbsitect)
            _ptbsitect->Exec(&CGID_ShellBrowser, FCIDM_PERSISTTOOLBAR, 0, NULL, NULL);
    }
}


void CInternetToolbar::_ShowVisible(DWORD dwVisibleBands, BOOL fPersist)
{
    // BUGBUG (scotth): is this even necessary now that we have a
    //  menu band always showing?
    BOOL fShowInitial = (! (_nVisibleBands & ~VBF_BRAND));

    _UpdateToolbarDisplay(UTD_VISIBLE, dwVisibleBands, _fCompressed, fPersist);

    if (fShowInitial)
        _pdie->put_ToolBar(TRUE);
}


HRESULT CInternetToolbar::_UpdateToolbarDisplay(DWORD dwFlags, UINT uVisibleBands, BOOL fNoText, BOOL fPersist)
{
    _fDirty = TRUE;  //Since we are making changes, set the dirty bit!

    //Update the back bitmap
    _SetBackground();

    //Show the bands.
    if(dwFlags & UTD_VISIBLE)
        _ShowBands(uVisibleBands);

    //Show/Hide the text.
    if(dwFlags & UTD_TEXTLABEL)
        _CompressBands(fNoText, _uiTBTextRows, TRUE);

    if (!_fTheater && fPersist && _ptbsitect)
        _ptbsitect->Exec(&CGID_ShellBrowser, FCIDM_PERSISTTOOLBAR, 0, NULL, NULL);

    return S_OK;
}

void CInternetToolbar::_UpdateBrandSize()
{
    LPBANDITEMDATA pbid = _bs._GetBandItemDataStructByID(CBIDX_BRAND);
    if (pbid && ((_nVisibleBands & (VBF_TOOLS | VBF_BRAND)) == (VBF_TOOLS | VBF_BRAND))) {
        BOOL fMinAlways = _fCompressed;

        if (!fMinAlways) {

            INT_PTR iTools = BandIDtoIndex(_bs._hwnd, CBIDX_TOOLS);
            INT_PTR iBrand = BandIDtoIndex(_bs._hwnd, CBIDX_BRAND);

            if (iBrand < iTools && !_fTheater)
                fMinAlways = TRUE;
        }

        VARIANTARG v = {0};
        v.vt = VT_I4;
        v.lVal = fMinAlways;
        IUnknown_Exec(pbid->pdb, &CGID_PrivCITCommands, CITIDM_BRANDSIZE, 0, &v, NULL);
    }
}

LRESULT CALLBACK CInternetToolbar::SizableWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CInternetToolbar* pitbar = (CInternetToolbar*)GetWindowPtr0(hwnd); // GetWindowLong(hwnd, 0)

    switch(uMsg)
    {
    case WM_SETCURSOR:
        if ((HWND)wParam == hwnd && LOWORD(lParam) == HTCLIENT && !SHRestricted2(REST_NOBANDCUSTOMIZE, NULL, 0)) {
            SetCursor(LoadCursor(NULL, IDC_SIZENS));
            return TRUE;
        }
        goto DoDefault;

    case WM_SYSCOLORCHANGE:
    case WM_WININICHANGE:
        if (FALSE == pitbar->_fDestroyed)
        {
            DWORD dwSection = SHIsExplorerIniChange(wParam, lParam);

            BOOL fRebuild = (uMsg == WM_SYSCOLORCHANGE) ||
                            (dwSection == EICH_KWINEXPLSMICO) ||
                            (wParam == SPI_SETNONCLIENTMETRICS);

            if (fRebuild)
            {
                pitbar->_InitForScreenSize();
                ITBar_LoadToolbarGlyphs(pitbar->_btb._hwnd);
                pitbar->_InitBitmapDSA();
                pitbar->_SetSearchStuff();
                pitbar->_ReloadButtons();
#ifdef EDIT_HACK
                if (uMsg == WM_SYSCOLORCHANGE)
                {
                    pitbar->_RefreshEditGlyph();
                }
#endif
            }
            
#ifdef EDIT_HACK
            if (dwSection == EICH_KINET)
            {
                pitbar->_aEditVerb.InitDefaultEditor();
                pitbar->_UpdateEditButton();
            }
#endif

            SendMessage(pitbar->_bs._hwnd, uMsg, wParam, lParam);
            pitbar->_SendToToolband(HWND_BROADCAST, uMsg, wParam, lParam, NULL);

            if (fRebuild)
            {
                pitbar->_SetBackground();
                InvalidateRect(pitbar->_bs._hwnd, NULL, TRUE);
                pitbar->_bs._SetMinDimensions();
            }
        }
        break;

    case WM_LBUTTONDOWN:
        //            RelayToToolTips(prb->hwndToolTips, hwnd, wMsg, wParam, lParam);
        // Don't allow toolbar resizing in theater mode
        if (!pitbar->_fTheater && !SHRestricted2(REST_NOBANDCUSTOMIZE, NULL, 0)) {
            pitbar->_xCapture = GET_X_LPARAM(lParam);
            pitbar->_yCapture = GET_Y_LPARAM(lParam);
            SetCapture(hwnd);
        }
        break;

    case WM_MOUSEMOVE:
        //            RelayToToolTips(prb->hwndToolTips, hwnd, wMsg, wParam, lParam);

        if (pitbar->_yCapture != -1)
        {
            if (hwnd != GetCapture())
                pitbar->_yCapture = -1;
            else
                pitbar->_TrackSliding(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        } else if (pitbar->_fRebarDragging) {
            RECT rc;
            POINT pt;
            GetClientRect(pitbar->_bs._hwnd, &rc);
            GetCursorPos(&pt);
            MapWindowPoints(HWND_DESKTOP, pitbar->_bs._hwnd, &pt, 1);
            rc.bottom /= 2;
            if (pt.y > rc.bottom)
                pt.y = rc.bottom;
            SendMessage(pitbar->_bs._hwnd, RB_DRAGMOVE, 0, MAKELPARAM(pt.x, pt.y));
        }
        break;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
        //            RelayToToolTips(prb->hwndToolTips, hwnd, wMsg, wParam, lParam);

        pitbar->_yCapture = -1;
        if (pitbar->_fRebarDragging) {
            pitbar->_fRebarDragging = FALSE;
            SendMessage(pitbar->_bs._hwnd, RB_ENDDRAG, 0, 0);
        }
        if (GetCapture() == hwnd)
            ReleaseCapture();
        break;

    case WM_CONTEXTMENU:
        pitbar->_bs.OnWinEvent(pitbar->_hwnd, uMsg, wParam, lParam, NULL);
        break;

    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
        // We must swallow these messages to avoid infinit SendMessage
        break;

    case WM_NOTIFY:
        // We must swallow these messages to avoid infinit SendMessage
        return pitbar->_OnNotify((LPNMHDR)lParam);

    case WM_NOTIFYFORMAT:
        if (NF_QUERY == lParam)
            return (DLL_IS_UNICODE ? NFR_UNICODE : NFR_ANSI);
        break;

    case WM_COMMAND:
        pitbar->_OnCommand(wParam, lParam);
        break;

    case WM_ERASEBKGND:
        {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hwnd, &rc);
            SHFillRectClr(hdc, &rc, (pitbar->_fTheater) ? RGB(0,0,0) : GetSysColor(COLOR_3DFACE));
            break;
         }

    case WM_PALETTECHANGED:
        //
        // BUGBUG: we could optimize this by realizing and checking the
        // return value
        //
        // for now we will just invalidate ourselves and all children...
        //
        RedrawWindow(hwnd, NULL, NULL,
                     RDW_INVALIDATE  | RDW_ERASE | RDW_ALLCHILDREN);
        break;

    case WM_TIMER:
        switch (wParam) {
        case IDT_UPDATETOOLBAR:
            pitbar->_fUpdateToolbarTimer = FALSE;
            KillTimer(hwnd, wParam);
            if (pitbar->_fNeedUpdateToolbar)
                pitbar->_UpdateToolbarNow();
            break;
        }
        break;

    case WM_DESTROY:
        pitbar->_Unadvise(); // remove ref-loop with _pdie
        TraceMsg(DM_TBREF, "Called RemoveProp. Called Release new _cRef=%d", pitbar->_cRef);
        goto DoDefault;

DoDefault:
    default:
        return(DefWindowProcWrap(hwnd, uMsg, wParam, lParam));
    }

    return 0L;
}


HRESULT CInternetToolbar::ResizeBorderDW(LPCRECT prcBorder,
                                         IUnknown* punkToolbarSite,
                                         BOOL fReserved)
{
    TraceMsg(DM_LAYOUT, "CITB::ResizeBorder called (_fShow==%d)", _fShow);
    HRESULT hres = S_OK;

    ASSERT(_ptbsite);
    if (_ptbsite)
    {
        RECT rcRequest = { 0, 0, 0, 0 };

        if (_fShow)
        {
            RECT rcRebar, rcBorder;
            int  cx,cy;

            GetWindowRect(_bs._hwnd, &rcRebar);
            cx = rcRebar.right - rcRebar.left;
            cy = rcRebar.bottom - rcRebar.top;

            int iExtra = 3;
            if (_fTheater) {
                // 1 for the 1 pixel border on the bottom
                iExtra = 1;
            }

            TraceMsg(DM_LAYOUT, "CITB::ResizeBorder cy = %d", cy);

            if (!prcBorder) {
                _ptbsite->GetBorderDW(SAFECAST(this, IDockingWindow*), &rcBorder);
                prcBorder = &rcBorder;
            }

            cx = prcBorder->right - prcBorder->left;


            SetWindowPos(_bs._hwnd, NULL, 0, 0,
                         cx, cy,  SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

            GetWindowRect(_bs._hwnd, &rcRebar);


            rcRequest.top = rcRebar.bottom - rcRebar.top + iExtra;
            SetWindowPos(_hwnd, NULL, prcBorder->left, prcBorder->top,
                         rcRebar.right - rcRebar.left, rcRequest.top, SWP_NOZORDER | SWP_NOACTIVATE);

        }

        if (_fTheater && _fAutoHide) {
            // if we're in theater mode, then we should request no space
            rcRequest.left = rcRequest.top = 0;
        }

        TraceMsg(DM_LAYOUT, "CITB::ResizeBorder calling RequstBS with %d,%d,%d,%d",
                 rcRequest.left, rcRequest.top, rcRequest.right, rcRequest.bottom);
        _ptbsite->RequestBorderSpaceDW(SAFECAST(this, IDockingWindow*), &rcRequest);

        TraceMsg(DM_LAYOUT, "CITB::ResizeBorder calling SetBS with %d,%d,%d,%d",
                 rcRequest.left, rcRequest.top, rcRequest.right, rcRequest.bottom);
        _ptbsite->SetBorderSpaceDW(SAFECAST(this, IDockingWindow*), &rcRequest);
    }

    return hres;
}

HRESULT CInternetToolbar::QueryStatus(const GUID *pguidCmdGroup,
                                      ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;

    ASSERT(_hwndAddressBand);
    LPBANDITEMDATA pbid = _bs._GetBandItemDataStructByID(CBIDX_ADDRESS);
    if (pbid && pbid->pdb)
    {
        IOleCommandTarget *poct;
        if (SUCCEEDED(pbid->pdb->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&poct)))
        {
            hr = poct->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
            poct->Release();
        }
    }

    if (pguidCmdGroup && IsEqualGUID(CGID_PrivCITCommands, *pguidCmdGroup))
    {
        hr = S_OK;
        for (ULONG i = 0 ; i < cCmds; i++)
        {
            rgCmds[i].cmdf = 0;
            switch (rgCmds[i].cmdID)
            {
            case CITIDM_VIEWTOOLS:
                if (_nVisibleBands & VBF_TOOLS)
                    rgCmds[i].cmdf = OLECMDF_ENABLED;
                break;

            case CITIDM_VIEWMENU:
                if (_nVisibleBands & VBF_MENU)
                    rgCmds[i].cmdf = OLECMDF_ENABLED;
                break;

            case CITIDM_VIEWTOOLBARCUSTOMIZE:
                if (_btb._fCustomize)
                    rgCmds[i].cmdf = OLECMDF_ENABLED;
                break;

            case CITIDM_VIEWAUTOHIDE:
                if (_fAutoHide)
                    rgCmds[i].cmdf = OLECMDF_ENABLED;
                break;

            case CITIDM_VIEWADDRESS:
                if (_nVisibleBands & VBF_ADDRESS)
                    rgCmds[i].cmdf = OLECMDF_ENABLED;
                break;

            case CITIDM_VIEWLINKS:
                if (_nVisibleBands & VBF_LINKS)
                    rgCmds[i].cmdf = OLECMDF_ENABLED;
                break;

            case CITIDM_TEXTLABELS:
                if (!_fCompressed)
                    rgCmds[i].cmdf = OLECMDF_ENABLED;
                break;

            case CITIDM_EDITPAGE:
                if (_fEditEnabled)
                    rgCmds[i].cmdf = OLECMDF_ENABLED;
#ifdef EDIT_HACK
                // The tooltip text is also used for the menu
                if (pcmdtext)
                {
                    TCHAR szBuf[MAX_PATH];
                    if ((pcmdtext->cmdtextf == OLECMDTEXTF_NAME) &&
                         _aEditVerb.GetMenuText(szBuf, ARRAYSIZE(szBuf)))
                    {
                        SHTCharToUnicode(szBuf, pcmdtext->rgwz, pcmdtext->cwBuf);
                        pcmdtext->cwActual = lstrlenW(pcmdtext->rgwz) + 1;
                    }
                    else
                    {
                        pcmdtext->cwActual = 0;
                    }
                }
#endif
                break;
            default:
                if (InRange( rgCmds[i].cmdID, CITIDM_VIEWEXTERNALBAND_FIRST, CITIDM_VIEWEXTERNALBAND_LAST)) {
                    int iBand = rgCmds[i].cmdID - CITIDM_VIEWEXTERNALBAND_FIRST;
                    if (!IsEqualCLSID( _rgebi[iBand].clsid, GUID_NULL )) {
                        rgCmds[i].cmdf |= OLECMDF_SUPPORTED;
                        if (_nVisibleBands & EXTERNALBAND_VBF_BIT( iBand )) {
                            rgCmds[i].cmdf |= OLECMDF_ENABLED;
                        }
                        if (pcmdtext)
                        {
                            pcmdtext->rgwz[0] = TEXT('\0');
                            switch (pcmdtext->cmdtextf)
                            {
                            case OLECMDTEXTF_NAME:
                                if (_rgebi[iBand].pwszName)
                                    Str_GetPtrW(_rgebi[iBand].pwszName, pcmdtext->rgwz, pcmdtext->cwBuf );
                                break;

                            case OLECMDTEXTF_STATUS:
                                if (_rgebi[iBand].pwszHelp)
                                    Str_GetPtrW(_rgebi[iBand].pwszHelp, pcmdtext->rgwz, pcmdtext->cwBuf );
                                break;

                            default:
                                break;
                            }
                            pcmdtext->cwActual = lstrlen( pcmdtext->rgwz );
                        }
                    }
                }
                break;
            }
        }
    }
    return hr;
}

void CInternetToolbar::_RestoreSaveStruct(COOLBARSAVE* pcs)
{
    REBARBANDINFO rbbi;
    rbbi.cbSize = sizeof(REBARBANDINFO);
    int i;

    _fAutoHide = pcs->fAutoHide;
    _ShowVisible(pcs->uiVisible, FALSE);

    BOOL fAllowRetry = TRUE;
    BOOL fNeedRetry = FALSE;
    INT_PTR fRedraw = SendMessage(_bs._hwnd, WM_SETREDRAW, FALSE, 0);
Retry:
    for (i = 0; i < CBANDSMAX; i++) {

        INT_PTR iIndex = SendMessage(_bs._hwnd, RB_IDTOINDEX, pcs->bs[i].wID, 0);
        if (iIndex != -1) {
            SendMessage(_bs._hwnd, RB_MOVEBAND, iIndex, i);
            rbbi.fMask = RBBIM_STYLE;
            if (SendMessage(_bs._hwnd, RB_GETBANDINFO, i, (LPARAM) &rbbi)) {
                rbbi.fMask = RBBIM_SIZE | RBBIM_STYLE;
                rbbi.cx = pcs->bs[i].cx;
                rbbi.fStyle = pcs->bs[i].fStyle;
                SendMessage(_bs._hwnd, RB_SETBANDINFO, i, (LPARAM) &rbbi);
            }

            // the SetBandInfo could have potentially caused items to shift around
            // verify that this didn't happen.
            iIndex = SendMessage(_bs._hwnd, RB_IDTOINDEX, pcs->bs[i].wID, 0);
            if (iIndex != i) {
                fNeedRetry = TRUE;
            }
        }
    }

    if (fAllowRetry && fNeedRetry) {
        fAllowRetry = FALSE;
        goto Retry;
    }

    _CSHSetStatusBar(pcs->fStatusBar);
    _UpdateToolsStyle(pcs->fList);

    RECT rc;
    GetWindowRect(_bs._hwnd, &rc);
    SetWindowPos(_bs._hwnd, NULL, 0,0, RECTWIDTH(rc), pcs->cyRebar, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    SendMessage(_bs._hwnd, WM_SETREDRAW, fRedraw, 0);
}

void CInternetToolbar::_CSHSetStatusBar(BOOL fOn)
{
    VARIANTARG v = { 0 };
    v.vt = VT_I4;
    v.lVal = fOn;
    IUnknown_Exec(_ptbsite, &CGID_ShellBrowser, FCIDM_SETSTATUSBAR,
        0, &v, NULL);
}

void CInternetToolbar::_TheaterModeLayout(BOOL fEnter)
{
    static const struct {
        int id;
        int cx;
    } c_layout[] =
    {
        { CBIDX_TOOLS, 400 },
        { CBIDX_MENU, 200 },
        { CBIDX_ADDRESS, 300 },
        { CBIDX_LINKS, 40 }
    };

    REBARBANDINFO rbbi;
    rbbi.cbSize = sizeof(REBARBANDINFO);

    BOOL_PTR fRedraw = SendMessage(_bs._hwnd, WM_SETREDRAW, FALSE, 0);
    SHSetWindowBits(_bs._hwnd, GWL_STYLE, RBS_AUTOSIZE, 0);
    if (fEnter) {
        _BuildSaveStruct(&_cs);

        // turn off text labels
        COOLBARSAVE cs;
        DWORD dwType, cbSize = sizeof(COOLBARSAVE);
        if (SHRegGetUSValue(c_szRegKeyCoolbar, c_szValueTheater, &dwType, (LPVOID)&cs, &cbSize, FALSE, NULL, 0) == ERROR_SUCCESS &&
           cs.cbVer == CBS_VERSION) {
            _RestoreSaveStruct(&cs);
            _UpdateToolbarDisplay(UTD_TEXTLABEL, 0, TRUE, TRUE);
        } else {
            _UpdateToolbarDisplay(UTD_TEXTLABEL, 0, TRUE, TRUE);
            _ShowVisible(VBF_TOOLS | VBF_BRAND, FALSE); // only show tools band by default
            RECT rc = { 0, 0, GetSystemMetrics(SM_CXSCREEN), 20 }; // something arbitrarily small vertically
            SendMessage(_bs._hwnd, RB_SIZETORECT, 0, (LPARAM)&rc);

            int cBands = (int) SendMessage(_bs._hwnd, RB_GETBANDCOUNT, 0, 0L);
            int i;
            // strip off all blanks
            rbbi.fMask = RBBIM_STYLE;
            for (i = 0; i < cBands; i++) {
                if (SendMessage(_bs._hwnd, RB_GETBANDINFO, i, (LPARAM) &rbbi))
                {
                    rbbi.fStyle &= ~RBBS_BREAK;
                    SendMessage(_bs._hwnd, RB_SETBANDINFO, i, (LPARAM) &rbbi);
                }
            }

            // then move into the proper order and size
            for (i = 0; i < ARRAYSIZE(c_layout); i++) {
                INT_PTR iIndex = SendMessage(_bs._hwnd, RB_IDTOINDEX, c_layout[i].id, 0);
                if (iIndex != -1) {
                    SendMessage(_bs._hwnd, RB_MOVEBAND, iIndex, i);

                    rbbi.fMask = RBBIM_SIZE;
                    rbbi.cx = c_layout[i].cx;
                    SendMessage(_bs._hwnd, RB_SETBANDINFO, i, (LPARAM) &rbbi);
                }
            }
            _CSHSetStatusBar(FALSE);  // default value in theater mode
        }
        SHSetWindowBits(_bs._hwnd, GWL_STYLE, RBS_BANDBORDERS | WS_BORDER, RBS_BANDBORDERS);
    } else {
        COOLBARSAVE cs;
        _BuildSaveStruct(&cs);
        SHRegSetUSValue(c_szRegKeyCoolbar, c_szValueTheater, REG_BINARY,
                        (LPVOID)&cs, sizeof(COOLBARSAVE), SHREGSET_HKCU | SHREGSET_FORCE_HKCU);
        _RestoreSaveStruct(&_cs);
        _UpdateToolbarDisplay(UTD_TEXTLABEL, 0, _cs.fNoText, FALSE);
        SHSetWindowBits(_bs._hwnd, GWL_STYLE, RBS_BANDBORDERS | WS_BORDER, RBS_BANDBORDERS | WS_BORDER);
    }

    _SetBackground();
    SHSetWindowBits(_bs._hwnd, GWL_STYLE, RBS_AUTOSIZE, RBS_AUTOSIZE);
    SendMessage(_bs._hwnd, WM_SETREDRAW, fRedraw, 0);

    SetWindowPos(_bs._hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
}

HRESULT CInternetToolbar::_GetMinRowHeight()
{
    UINT iHeight = 0;
    int icBands = (int) SendMessage( _bs._hwnd, RB_GETBANDCOUNT, 0, 0 );
    for (int i = 0; i < icBands; i++) {
        REBARBANDINFO rbbi;
        rbbi.cbSize = sizeof(REBARBANDINFO);
        rbbi.fMask = RBBIM_CHILDSIZE | RBBIM_STYLE;
        if (SendMessage(_bs._hwnd, RB_GETBANDINFO, i, (LPARAM)&rbbi)) {
            // go until the end of the row
            if (rbbi.fStyle & RBBS_BREAK)
                break;

            if (!(rbbi.fStyle & RBBS_HIDDEN)) {
                if (rbbi.cyMinChild > iHeight)
                    iHeight = rbbi.cyMinChild;
            }
        }
    }

    return ResultFromShort(iHeight);
}

BOOL IsBarRefreshable(IDeskBar* pdb)
{
    ASSERT(pdb);
    BOOL fIsRefreshable = TRUE;
    VARIANT varClsid = {0};

    if (SUCCEEDED(IUnknown_Exec(pdb, &CGID_DeskBarClient, DBCID_CLSIDOFBAR, 1, NULL, &varClsid)) && (varClsid.vt == VT_BSTR))
    {
        CLSID clsidBar;

        //if the bar is hidden, it returns GUID_NULL, so don't refresh it
        if ( GUIDFromString(varClsid.bstrVal, &clsidBar) &&
             (IsEqualGUID(clsidBar, GUID_NULL)) )
        {
            fIsRefreshable = FALSE;
        }
        else
        {
            //APPHACK for office discussions band (and possibly others)
            //CLSID\GUID\Instance
            WCHAR wszKey[6+40+1+9];
            DWORD dwValue, dwType=REG_DWORD, dwcbData = 4;
            wnsprintf(wszKey, ARRAYSIZE(wszKey), L"CLSID\\%s\\Instance", varClsid.bstrVal);

            if ( (SHGetValue(HKEY_CLASSES_ROOT, wszKey, L"DontRefresh", &dwType, &dwValue, &dwcbData) == ERROR_SUCCESS) &&
                 (dwValue != 0) )
            {
                fIsRefreshable = FALSE;
            }
        }
        VariantClear(&varClsid);
    }
    return fIsRefreshable;
}

HRESULT CInternetToolbar::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;  // assume failure

    if (!pguidCmdGroup) {
        goto Lother;
    } else if (IsEqualGUID(CLSID_CommonButtons, *pguidCmdGroup)) {
        if (pvarargOut) {
            ASSERT(pvarargOut && pvarargOut->vt == VT_I4);
            UINT uiInternalCmdID = pvarargOut->lVal;

            if (nCmdID == TBIDM_SEARCH && uiInternalCmdID == -1)
                _btb._ConvertCmd(pguidCmdGroup, nCmdID, NULL, &uiInternalCmdID);

            switch (nCmdID) {
            case TBIDM_BACK:
            case TBIDM_FORWARD:
            case TBIDM_STOPDOWNLOAD:
            case TBIDM_REFRESH:
            case TBIDM_HOME:
            case TBIDM_SEARCH:
            case TBIDM_FAVORITES:
            case TBIDM_HISTORY:
            case TBIDM_ALLFOLDERS:
#ifdef ENABLE_CHANNELPANE
            case TBIDM_CHANNELS:
#endif
                if (!SendMessage(_btb._hwnd, TB_ISBUTTONENABLED, uiInternalCmdID, 0))
                    return S_OK;
                break;
            }
            if (nCmdexecopt == OLECMDEXECOPT_PROMPTUSER) {
                // the user hit the drop down
                if (_ptbsitect && pvarargIn && pvarargIn->vt == VT_INT_PTR) {
                    // v.vt = VT_I4;
                    POINT pt;
                    RECT* prc = (RECT*)pvarargIn->byref;
                    pt.x = prc->left;
                    pt.y = prc->bottom;

                    switch (nCmdID) {
                    case TBIDM_BACK:
                        _ShowBackForwardMenu(FALSE, pt, prc);
                        break;

                    case TBIDM_FORWARD:
                        _ShowBackForwardMenu(TRUE, pt, prc);
                        break;
                    }
                    // VariantClearLazy(&v);
                }
                return S_OK;
            }

            switch(nCmdID) {

            case TBIDM_PREVIOUSFOLDER:
                _ptbsitect->Exec(&CGID_ShellBrowser, FCIDM_PREVIOUSFOLDER, 0, NULL, NULL);
                break;

            case TBIDM_CONNECT:
                DoNetConnect(_hwnd);
                break;

            case TBIDM_DISCONNECT:
                DoNetDisconnect(_hwnd);
                break;

            case TBIDM_BACK:
                _pdie->GoBack();
                break;

            case TBIDM_FORWARD:
                _pdie->GoForward();
                break;

            case TBIDM_HOME:
                _pdie->GoHome();
                break;

            case TBIDM_SEARCH:
                if (_ptbsitect)
                {
                    VARIANTARG vaOut = {0};
                    VARIANTARG* pvaOut = NULL;
                    LPITEMIDLIST pidl = NULL;

                    // i'm leaving this not #ifdefed out because it is used by explorer bar
                    // persistance (reljai)
                    //_SetSearchStuff initializes _guidDefaultSearch, which may or may not have
                    // been called yet
                    if (IsEqualGUID(_guidDefaultSearch, GUID_NULL))
                        _SetSearchStuff();

                    // see if what the state of search pane is, so we can toggle it...
                    OLECMD rgcmds[] = {{ SBCMDID_SEARCHBAR, 0 },};
                    
                    if (_ptbsitect)
                        _ptbsitect->QueryStatus(&CGID_Explorer, ARRAYSIZE(rgcmds), rgcmds, NULL);
                    // not pressed, then show the pane
                    if (!(rgcmds[0].cmdf & OLECMDF_LATCHED))
                    {
                        WCHAR       wszUrl[MAX_URL_STRING];

                        if (_GetSearchUrl(wszUrl, ARRAYSIZE(wszUrl)))
                        {
                            CLSID clsid;

                            if (GUIDFromString(wszUrl, &clsid))
                            {
                                IContextMenu* pcm;

                                if (SUCCEEDED(SHCoCreateInstance(NULL, &clsid, NULL, IID_IContextMenu, (void **)&pcm)))
                                {
                                    CMINVOKECOMMANDINFO ici = {0};
                                    CHAR                szGuid[GUIDSTR_MAX];
                                    BOOL                bSetSite = TRUE;

                                    ici.cbSize = SIZEOF(ici);
                                    ici.hwnd = _hwnd;
                                    ici.lpVerb = (LPSTR)MAKEINTRESOURCE(0);
                                    ici.nShow  = SW_NORMAL;
                                    SHStringFromGUIDA(_guidDefaultSearch, szGuid, ARRAYSIZE(szGuid));
                                    ici.lpParameters = szGuid;

                                    // in case of rooted browser we need to open the search in the new window
                                    // 'coz otherwise the pane opens in the same window and user starts search
                                    // and browseobject (or someone) detects the rooted case and launches new
                                    // browser for our search results view (which is blank because it cannot do
                                    // search by itself and also there is not search pane) (reljai)
                                    if (_pbs2)
                                    {
                                        LPITEMIDLIST pidl;

                                        if (SUCCEEDED(_pbs2->GetPidl(&pidl)))
                                        {
                                            bSetSite = !ILIsRooted(pidl);
                                            ILFree(pidl);
                                        }
                                    }
                                    // if there is no site, InvokeCommand bellow will launch new browser w/ the 
                                    // search pane open
                                    if (bSetSite)
                                        IUnknown_SetSite(pcm, _psp);
                                    hr = pcm->InvokeCommand(&ici);
                                    if (bSetSite)
                                        IUnknown_SetSite(pcm, NULL);
                                    pcm->Release();
                                }
                                break;
                            }
                            //_guidCurrentSearch = _guidDefaultSearch;//done on set state
                            IECreateFromPathW(wszUrl, &pidl);
                            // this is a hack but VariantToConstIDList supports it, and it's the easiest
                            // way to pass the pidl, so...
                            InitVariantFromIDList(&vaOut, pidl);
                            pvaOut = &vaOut;
                        }
                    }

                    hr = _ptbsitect->Exec(&CGID_Explorer, SBCMDID_SEARCHBAR, OLECMDEXECOPT_DONTPROMPTUSER, NULL, pvaOut); // vaIn:NULL means toggle
                    ASSERT(SUCCEEDED(hr));
                    if (pvaOut)
                        VariantClear(pvaOut);
                    ILFree(pidl);
                }
                else
                {
                    TraceMsg(DM_ERROR, "CIEA::GS: no IOleCommandTarget!");
                }
                break;

            case TBIDM_FAVORITES:
            case TBIDM_HISTORY:
            case TBIDM_ALLFOLDERS:
#ifdef ENABLE_CHANNELPANE
            case TBIDM_CHANNELS:
#endif
                if (_ptbsitect) {
                    static const int tbtab[] = {
                        TBIDM_FAVORITES    , TBIDM_HISTORY    ,   TBIDM_ALLFOLDERS  ,
#ifdef ENABLE_CHANNELPANE
                        TBIDM_CHANNELS  ,
#endif
                    };
                    static const int cttab[] = {
                        SBCMDID_FAVORITESBAR, SBCMDID_HISTORYBAR, SBCMDID_EXPLORERBAR,
#ifdef ENABLE_CHANNELPANE
                        SBCMDID_CHANNELSBAR ,
#endif
                    };
                    HRESULT hres;
                    int idCT;

                    idCT = SHSearchMapInt(tbtab, cttab, ARRAYSIZE(tbtab), nCmdID);
                    hres = _ptbsitect->Exec(&CGID_Explorer, idCT, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);  // vaIn:NULL means toggle
                    ASSERT(SUCCEEDED(hres));
                }
                else
                {
                    TraceMsg(DM_ERROR, "CIEA::GS: no IOleCommandTarget!");
                }
                break;

            case TBIDM_THEATER:
                {
                    VARIANT_BOOL b;
                    if (SUCCEEDED(_pdie->get_TheaterMode(&b)))
                        _pdie->put_TheaterMode( b == VARIANT_TRUE ? VARIANT_FALSE : VARIANT_TRUE);
                    break;
                }

            case TBIDM_STOPDOWNLOAD:
                if (_fTransitionToHTML)
                {
                    UINT uiState;
                    _fTransitionToHTML = FALSE;
                    if (SUCCEEDED(GetState(&CLSID_CommonButtons, TBIDM_STOPDOWNLOAD, &uiState)))
                    {
                        uiState |= TBSTATE_HIDDEN;
                        SetState(&CLSID_CommonButtons, TBIDM_STOPDOWNLOAD, uiState);
                    }
                    SendMessage(_hwndAddressBand, CB_SETEDITSEL, NULL, (LPARAM)MAKELONG(-1,0));
                }
                _pdie->Stop();
                break;

            case TBIDM_REFRESH:
            {
                VARIANT v = {0};
                v.vt = VT_I4;
                v.lVal = (GetAsyncKeyState(VK_CONTROL) < 0) ?
                         OLECMDIDF_REFRESH_COMPLETELY|OLECMDIDF_REFRESH_PROMPTIFOFFLINE :
                         OLECMDIDF_REFRESH_NO_CACHE|OLECMDIDF_REFRESH_PROMPTIFOFFLINE;
                _pdie->Refresh2(&v);

                if (_hwndAddressBand)
                {
                    LPBANDITEMDATA pbid = _bs._GetBandItemDataStructByID(CBIDX_ADDRESS);

                    ASSERT(pbid && pbid->pdb);     // pbid->pdb is a IDeskBand pointer
                    if (pbid && pbid->pdb)
                    {
                        IAddressBand *pab = NULL;
                        if (SUCCEEDED(pbid->pdb->QueryInterface(IID_IAddressBand, (LPVOID*)&pab)))
                        {
                            VARIANTARG varType = {0};
                            varType.vt = VT_I4;
                            varType.lVal = OLECMD_REFRESH_TOPMOST;
                            pab->Refresh(&varType);
                            pab->Release();
                        }
                    }
                }

                // pass this to vert and horz bars
                IDockingWindowFrame *psb;
                if (_psp && SUCCEEDED(_psp->QueryService(SID_STopLevelBrowser, IID_IDockingWindowFrame, (void**)&psb)))
                {
                    IDeskBar* pdb;

                    if (SUCCEEDED(psb->FindToolbar(INFOBAR_TBNAME, IID_IDeskBar, (void **)&pdb)) && pdb &&
                        IsBarRefreshable(pdb))
                    {
                        IUnknown_Exec(pdb, NULL, OLECMDID_REFRESH, OLECMDIDF_REFRESH_NORMAL|OLECMDIDF_REFRESH_PROMPTIFOFFLINE, NULL, NULL);
                        pdb->Release();
                    }
                    if (SUCCEEDED(psb->FindToolbar(COMMBAR_TBNAME, IID_IDeskBar, (void **)&pdb)) && pdb &&
                        IsBarRefreshable(pdb))
                    {
                        IUnknown_Exec(pdb, NULL, OLECMDID_REFRESH, OLECMDIDF_REFRESH_NORMAL|OLECMDIDF_REFRESH_PROMPTIFOFFLINE, NULL, NULL);
                        pdb->Release();
                    }
                    psb->Release();
                }

            }
                break;
            }
        }
    } 
    else if (IsEqualGUID(IID_IExplorerToolbar, *pguidCmdGroup)) 
    {
        switch (nCmdID)
        {
        case ETCMDID_GETBUTTONS:
            {
                if (_iButtons == -1) 
                {
                    // haven't initialized yet
                    _iButtons = ARRAYSIZE(c_tbExplorer);
                    memcpy(_tbExplorer, c_tbExplorer, SIZEOF(TBBUTTON) * ARRAYSIZE(c_tbExplorer));
                    if (GetUIVersion() < 5) 
                    {
                        // we don't want up button and network drive buttons available
                        // on < nt5 shell (by trident pm design)

                        // no customization in shell view on < nt5
                        ASSERT(!_fShellView);

                        ASSERT(c_tbExplorer[5].idCommand == TBIDM_PREVIOUSFOLDER);
                        _tbExplorer[5].fsState |= TBSTATE_HIDDEN;

                        ASSERT(c_tbExplorer[6].idCommand == TBIDM_CONNECT);
                        _tbExplorer[6].fsState |= TBSTATE_HIDDEN;

                        ASSERT(c_tbExplorer[7].idCommand == TBIDM_DISCONNECT);
                        _tbExplorer[7].fsState |= TBSTATE_HIDDEN;

                        ASSERT(c_tbExplorer[10].idCommand == TBIDM_ALLFOLDERS);
                        if (!_FoldersButtonAvailable())
                            _tbExplorer[10].fsState |= TBSTATE_HIDDEN;
                    }
                    else
                    {
                        ASSERT(c_tbExplorer[9].idCommand == TBIDM_SEARCH);
                        if (_fShellView && SHRestricted(REST_NOSHELLSEARCHBUTTON))
                            _tbExplorer[9].fsState |= TBSTATE_HIDDEN;
                            
                        ASSERT(c_tbExplorer[6].idCommand == TBIDM_CONNECT);
                        ASSERT(c_tbExplorer[7].idCommand == TBIDM_DISCONNECT);
                        if (SHRestricted(REST_NONETCONNECTDISCONNECT))
                        {
                            _tbExplorer[6].fsState |= TBSTATE_HIDDEN;
                            _tbExplorer[7].fsState |= TBSTATE_HIDDEN;
                        }
                    }
                    _iButtons = RemoveHiddenButtons(_tbExplorer, ARRAYSIZE(_tbExplorer));
                }

                pvarargOut->vt = VT_BYREF;
                pvarargOut->byref = (LPVOID)_tbExplorer;
                *pvarargIn->plVal = _iButtons;
            }
            return S_OK;
        }
    }
    else if (IsEqualGUID(CGID_PrivCITCommands, *pguidCmdGroup))
    {
        DWORD dw;
        hr = S_OK;
        switch (nCmdID)
        {
        case CITIDM_GETFOLDERSEARCHES:
            {
                hr = E_INVALIDARG;

                if (pvarargOut)
                {
                    IFolderSearches *pfs;
                    hr = _GetFolderSearches(&pfs);

                    if (SUCCEEDED(hr))
                    {
                        VariantClear(pvarargOut);
                        pvarargOut->vt = VT_UNKNOWN;
                        pvarargOut->punkVal = pfs;
                    }
                }
            }
            break;

        case CITIDM_SET_DIRTYBIT:
            _fDirty = BOOLIFY(nCmdexecopt);
            break;

        case CITIDM_GETMINROWHEIGHT:
            hr = _GetMinRowHeight();
            break;

        case CITIDM_VIEWTOOLBARCUSTOMIZE:
            _OnCommand(GET_WM_COMMAND_MPS(FCIDM_VIEWTOOLBARCUSTOMIZE, _hwnd, 0));
            break;

        case CITIDM_TEXTLABELS:
            _OnCommand(GET_WM_COMMAND_MPS(FCIDM_VIEWTEXTLABELS, _hwnd, 0));
            break;

        case CITIDM_EDITPAGE:
            // BUGBUG: temp code -- edit code moving to dochost.cpp
            _btb.Exec(&CLSID_InternetButtons, DVIDM_EDITPAGE, 0, NULL, NULL);
            break;

        case CITIDM_ONINTERNET:
            switch (nCmdexecopt) {
            case CITE_INTERNET:
                _fInitialPidlIsWeb = TRUE;
                _fShellView = !_fInitialPidlIsWeb;
                break;
            case CITE_SHELL:
                _fInitialPidlIsWeb = FALSE;
                _fShellView = !_fInitialPidlIsWeb;
                break;
            case CITE_QUERY:
                return ResultFromScode(_fShellView ? CITE_SHELL : CITE_INTERNET);
                break;
            }
            return ResultFromScode(_fInitialPidlIsWeb ? CITE_INTERNET : CITE_SHELL);

        case CITIDM_VIEWTOOLS:
            _OnCommand(GET_WM_COMMAND_MPS(FCIDM_VIEWTOOLS, _hwnd, 0));
            break;

        case CITIDM_VIEWAUTOHIDE:
            _OnCommand(GET_WM_COMMAND_MPS(FCIDM_VIEWAUTOHIDE, _hwnd, 0));
            break;

        case CITIDM_VIEWADDRESS:
            _OnCommand(GET_WM_COMMAND_MPS(FCIDM_VIEWADDRESS, _hwnd, 0));
            break;

        case CITIDM_VIEWLINKS:
            _OnCommand(GET_WM_COMMAND_MPS(FCIDM_VIEWLINKS, _hwnd, 0));
            break;

        case CITIDM_VIEWMENU:
            _OnCommand(GET_WM_COMMAND_MPS(FCIDM_VIEWMENU, _hwnd, 0));
            break;

        case CITIDM_SHOWTOOLS:
            dw = VBF_TOOLS;
            goto ShowABand;

        case CITIDM_SHOWADDRESS:
            dw = VBF_ADDRESS;
            goto ShowABand;

        case CITIDM_SHOWLINKS:
            dw = VBF_LINKS;
            goto ShowABand;

#ifdef UNIX
        case CITIDM_SHOWBRAND:
            dw = VBF_BRAND;
            goto ShowABand;
#endif

        case CITIDM_SHOWMENU:
            dw = VBF_MENU;
ShowABand:
            if (nCmdexecopt)
                dw |= _nVisibleBands;           // Set
            else
                dw = (_nVisibleBands & ~dw);    // Clear

            _ShowVisible(dw, TRUE);
            _fUsingDefaultBands = FALSE;
            break;

        case CITIDM_DISABLESHOWMENU:
            _fNoShowMenu = BOOLIFY(nCmdexecopt);
            break;

        case CITIDM_STATUSCHANGED:
            _fDirty = TRUE;
            if (_ptbsitect)
                _ptbsitect->Exec(&CGID_ShellBrowser, FCIDM_PERSISTTOOLBAR, 0, NULL, NULL);
            break;

        case CITIDM_THEATER:

            if (_fShow) {

                // IF YOU SEE HTIS ASSERT, TRY TO REMEMBER WHAT YOU DID AND CALL CHEE
                ASSERT(_fTheater || _nVisibleBands & VBF_MENU);

                switch (nCmdexecopt)
                {

                case THF_ON:
                    _fTheater = TRUE;
                    ResizeBorderDW(NULL, NULL, FALSE);
                    _TheaterModeLayout(TRUE);
                    // theater has its own brand, so needs to know whether we're in shell or web view so it can show the right brand
                    IUnknown_Exec(_ptbsite, &CGID_Theater, THID_ONINTERNET, _fShellView ? CITE_SHELL : CITE_INTERNET, NULL, NULL);

                    // pass back _fAutoHide
                    pvarargOut->vt = VT_I4;
                    pvarargOut->lVal = _fAutoHide;

                    goto notify_bands;


                case THF_OFF:
                    _fTheater = FALSE;
                    ResizeBorderDW(NULL, NULL, FALSE);
                    _TheaterModeLayout(FALSE);

                    // position everything properly (needed after reparenting)
                    SendMessage(_hwnd, RB_PRIV_RESIZE, 0, 0);
                    goto notify_bands;

notify_bands:
                    {
                        int icBands = (int) SendMessage( _bs._hwnd, RB_GETBANDCOUNT, 0, 0 );
                        for (int i = 0; i < icBands; i++)
                        {
                            REBARBANDINFO rbbi;
                            rbbi.cbSize = sizeof(REBARBANDINFO);
                            rbbi.fMask = RBBIM_ID;

                            if (SendMessage(_bs._hwnd, RB_GETBANDINFO, i, (LPARAM) &rbbi))
                            {
                                LPBANDITEMDATA pbid = (LPBANDITEMDATA)_bs._GetBandItemDataStructByID(rbbi.wID);
                                if (pbid)
                                    IUnknown_Exec(pbid->pdb, pguidCmdGroup, CITIDM_THEATER, nCmdexecopt, NULL, NULL);
                            }
                        }
                    }
                    break;

                case THF_UNHIDE:
                    // position everything properly (needed after reparenting)
                    SendMessage(_hwnd, RB_PRIV_RESIZE, 0, 0);
                    break;
                }

                // IF YOU SEE HTIS ASSERT, TRY TO REMEMBER WHAT YOU DID AND CALL CHEE
                ASSERT(_fTheater || _nVisibleBands & VBF_MENU);
            }

            break;
        case CITIDM_VIEWEXTERNALBAND_BYCLASSID:
            if ((pvarargIn->vt == VT_BSTR) && pvarargIn->bstrVal) {
                CLSID clsid;
                if (GUIDFromString( pvarargIn->bstrVal, &clsid )) {
                    hr = E_FAIL;
                    for (DWORD i = 0; i < MAXEXTERNALBANDS; i++) {
                        if (clsid == _rgebi[i].clsid) {
                            DWORD dw = _nVisibleBands;
                            DWORD dwBit = EXTERNALBAND_VBF_BIT( i );
                            dw = (nCmdexecopt) ? dw | dwBit : dw & ~dwBit;
                            if ( !( dw & ~VBF_BRAND)) {
                                _pdie->put_ToolBar( FALSE );
                            }
                            _ShowVisible(dw, TRUE);
                            _fUsingDefaultBands = FALSE;
                            hr = S_OK;
                            break;
                        }
                    }
                }
            }
            break;
        default:
            if (InRange( nCmdID, CITIDM_VIEWEXTERNALBAND_FIRST, CITIDM_VIEWEXTERNALBAND_LAST )) {
                _OnCommand(GET_WM_COMMAND_MPS( nCmdID - CITIDM_VIEWEXTERNALBAND_FIRST + FCIDM_EXTERNALBANDS_FIRST, _hwnd, 0));
                break;
            }
            ASSERT(0);
            break;
        }
    }
    else
    {
Lother:
        LPBANDITEMDATA pbid = _bs._GetBandItemDataStructByID(CBIDX_ADDRESS);
        if (pbid)
        {
            hr = IUnknown_Exec(pbid->pdb, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
        }
    }

    return hr;
}

BOOL _GetSearchHKEY(REFGUID guidSearch, HKEY *phkey)
{
    HKEY hkey;
    BOOL bRet = FALSE;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_SZ_STATIC, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        TCHAR szExt[MAX_PATH];//extension key name
        DWORD cchExt = ARRAYSIZE(szExt);
        int  iExt;
        BOOL bNoUrl = FALSE; // true iff guidSearch is found and there is no Url subkey

        for (iExt=0;
             !bRet && RegEnumKeyEx(hkey, iExt, szExt, &cchExt, NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
             cchExt = ARRAYSIZE(szExt), iExt++)
        {
            HKEY hkeyExt; // static extension key

            if (RegOpenKeyEx(hkey, szExt, 0, KEY_READ, &hkeyExt) == ERROR_SUCCESS)
            {
                int i;
                TCHAR szSubKey[32];
                HKEY  hkeySub;

                for (i = 0; !bRet && (wnsprintf(szSubKey, ARRAYSIZE(szSubKey), TEXT("%d"), i),
                            RegOpenKey(hkeyExt, szSubKey, &hkeySub) == ERROR_SUCCESS);
                     i++)
                {
                    TCHAR szSearchGuid[GUIDSTR_MAX];
                    DWORD cb;
                    DWORD dwType;

                    cb = SIZEOF(szSearchGuid);
                    if (SHGetValue(hkeySub, REG_SZ_SEARCH_GUID, NULL, &dwType, (BYTE*)szSearchGuid, &cb) == ERROR_SUCCESS)
                    {
                        GUID guid;

                        if (GUIDFromString(szSearchGuid, &guid) &&
                            IsEqualGUID(guid, guidSearch))
                        {
                            HKEY hkeyTmp;

                            if (RegOpenKey(hkeySub, REG_SZ_SEARCH_URL, &hkeyTmp) == ERROR_SUCCESS)
                                RegCloseKey(hkeyTmp);
                            else
                                bNoUrl = TRUE;

                            bRet = TRUE;
                        }
                    }
                    if (!bRet || bNoUrl)
                        RegCloseKey(hkeySub);
                    else
                        *phkey = hkeySub;

                }
                if (!bNoUrl)
                    RegCloseKey(hkeyExt);
                else
                {
                    ASSERT(bRet);
                    *phkey = hkeyExt;
                }
            }
        }
        RegCloseKey(hkey);
    }
    return bRet;
}

HRESULT CInternetToolbar::GetDefaultSearchUrl(LPWSTR pwszUrl, UINT cch)
{
    HRESULT hr = E_FAIL;

    if (GetDefaultInternetSearchUrlW(pwszUrl, cch, TRUE))
        hr = S_OK;
    return hr;
}

void WINAPI CopyEnumElement(void *pDest, const void *pSource, DWORD dwSize)
{
    if (!pDest)
        return;

    memcpy(pDest, pSource, dwSize);
}

class CFolderSearches : public IFolderSearches
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    // *** IFolderSearches ***
    STDMETHODIMP EnumSearches(IEnumUrlSearch **ppenum);
    STDMETHODIMP DefaultSearch(GUID *pguid);

    CFolderSearches(GUID *pguid, int iCount, URLSEARCH *pUrlSearch);
    ~CFolderSearches();
private:
    LONG _cRef;
    int  _iCount;
    GUID _guidDefault;
    URLSEARCH *_pUrlSearch;
};

CFolderSearches::CFolderSearches(GUID *pguid, int iCount, URLSEARCH *pUrlSearch)
{
    _cRef = 1;
    _iCount = iCount;
    _guidDefault = *pguid;
    _pUrlSearch = pUrlSearch;
}

CFolderSearches::~CFolderSearches()
{
    if (_pUrlSearch)
        LocalFree(_pUrlSearch);
}

HRESULT CFolderSearches::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CFolderSearches, IFolderSearches),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

ULONG CFolderSearches::AddRef()
{
    return ++_cRef;
}

ULONG CFolderSearches::Release()
{
    if (--_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CFolderSearches::EnumSearches(IEnumUrlSearch **ppenum)
{
    HRESULT hres = E_OUTOFMEMORY;

    *ppenum = (IEnumUrlSearch *)new CStandardEnum(IID_IEnumUrlSearch, FALSE,
                _iCount, sizeof(URLSEARCH), _pUrlSearch, CopyEnumElement);
    if (*ppenum)
    {
        _pUrlSearch = NULL;
        _iCount = 0;
        hres = S_OK;
    }
    return hres;
}

HRESULT CFolderSearches::DefaultSearch(GUID *pguid)
{
    *pguid = _guidDefault;
    return S_OK;
}

HRESULT CInternetToolbar::_GetFolderSearches(IFolderSearches **ppfs)
{
    HRESULT hres = E_FAIL;

    *ppfs = NULL;
    if (_hdpaFSI)
    {
        LPURLSEARCH pUrlSearch = NULL;
        int iCount = 0;
        int cFSIs = DPA_GetPtrCount(_hdpaFSI);

        hres = E_OUTOFMEMORY;
        if (cFSIs > 0
            && ((pUrlSearch = (LPURLSEARCH)LocalAlloc(LPTR, SIZEOF(URLSEARCH)*cFSIs)) != NULL))
        {
            LPFOLDERSEARCHITEM pfsi;
            int i;

            // insert per folder items
            for (i = 0; i < cFSIs && (pfsi = (LPFOLDERSEARCHITEM)DPA_GetPtr(_hdpaFSI, i)) != NULL; i++)
            {
                CLSID  clsid;

                // check if Url is actually a GUID. if yes we cannot enumerate it because
                // we need Title/Url pair.
                if (!GUIDFromStringW(pfsi->wszUrl, &clsid))
                {
                    lstrcpynW(pUrlSearch[iCount].wszName, pfsi->wszName, ARRAYSIZE(pUrlSearch[iCount].wszName));
                    lstrcpynW(pUrlSearch[iCount].wszUrl,  pfsi->wszUrl,  ARRAYSIZE(pUrlSearch[iCount].wszUrl));
                    iCount++;
                }
            }
        }
        *ppfs = new CFolderSearches(&_guidDefaultSearch, iCount, pUrlSearch);
        if (*ppfs)
            hres = S_OK;
        else
            LocalFree(pUrlSearch);
    }
    return hres;
}

BOOL CInternetToolbar::_GetSearchUrl(LPWSTR pwszUrl, DWORD cch)
{
    BOOL        bRet = FALSE;
    HKEY        hkey;

    if (pwszUrl)
    {
        pwszUrl[0] = L'\0';

        // if we are looking for web search url bypass the registry lookup and the
        // per folder items and go straight to GetDefaultSearchUrl which call
        // GetSearchAssistantUrlW
        if (!IsEqualGUID(_guidDefaultSearch, SRCID_SWebSearch))
        {
            // _GetSearchHKEY looks in the registry where shell search items are registered
            // if we have old shell32 then we don't display shell search items so we should
            // not look in the registry
            if (GetUIVersion() >= 5 && _GetSearchHKEY(_guidDefaultSearch, &hkey))
            {
                DWORD cb = cch*sizeof(TCHAR);
                TCHAR szGuid[GUIDSTR_MAX];
                DWORD cbGuid = SIZEOF(szGuid);

                // is there a url key
                if (SHGetValueW(hkey, REG_SZ_SEARCH_URL, NULL, NULL, pwszUrl, &cb) == ERROR_SUCCESS)
                    bRet = TRUE;
                // no? try the default value, maybe it's the clsid
                else if (SHGetValueW(hkey, NULL, NULL, NULL, szGuid, &cbGuid) == ERROR_SUCCESS)
                {
                    GUID guid;
                    // is it a valid guid string
                    if (GUIDFromString(szGuid, &guid))
                    {
                        StrCpyNW(pwszUrl, szGuid, cch);
                        bRet = TRUE;
                    }
                }

                RegCloseKey(hkey);
            }
            // maybe it's one of the per-folder items...
            else if (_hdpaFSI) //FSI = folder search items
            {
                int i;
                LPFOLDERSEARCHITEM pfsi;

                for (i=0; (pfsi = (LPFOLDERSEARCHITEM)DPA_GetPtr(_hdpaFSI, i)) != NULL; i++)
                {
                    if (IsEqualGUID(_guidDefaultSearch, pfsi->guidSearch))
                    {
                        StrCpyNW(pwszUrl, pfsi->wszUrl, cch);
                        bRet = TRUE;
                        break;
                    }
                }
            }
        }

        if (!bRet)
            bRet = SUCCEEDED(GetDefaultSearchUrl(pwszUrl, cch));
    }

    return bRet;
}

void CInternetToolbar::_SetSearchStuff()
{
    UINT uiState;
    BOOL bChecked = FALSE;

    if (SUCCEEDED(GetState(&CLSID_CommonButtons, TBIDM_SEARCH, &uiState)))
        bChecked = uiState & TBSTATE_CHECKED;

    if (!_hdpaFSI)
        _hdpaFSI = DPA_Create(2);
    else
    {
        DPA_EnumCallback(_hdpaFSI, DeleteDPAPtrCB, NULL); // delete all ptrs
        DPA_DeleteAllPtrs(_hdpaFSI); // now tell hdpa to forget about them
    }

    // BUGBUG: this is bogus -- _fShellView is always FALSE when using automation
    if (_fShellView)
        _guidDefaultSearch = SRCID_SFileSearch;
    else
        _guidDefaultSearch = SRCID_SWebSearch;

    // get per folder search items and the default search (if any)
    // and insert them to _himlSrc
    _GetFolderSearchData();

    if (!bChecked)
    {
        _guidCurrentSearch = _guidDefaultSearch;
    }
}

//
// CInternetToolbar::SetCommandTarget()
//
// This function sets the current command target and button group.  A client calls this
// before merging in buttons with the AddButtons method.
//
// There are a couple of tricky things about this function.
//
// One is that the client can pass some flags (dwFlags param) specifying the bands it wants
// showing by default (menu, links, address, tools, brand, external).  But we don't let them change
// the state of the menu band.  And, if another client has already set the default bands, we
// don't let them change the state of any of the bands.
//
// The other is that we do some stuff to figure out if the caller is just another instantiation of the same
// client.  If we think this is a new client (new guidButtonGroup), we flush the toolbar and return S_OK.
// But if we think this is the same client reincarnated (same guidButtonGroup and non-NULL command target),
// we return S_FALSE without flushing the toolbar.  This is done for performance.  A new dochost is instantiated
// on each navigation, but its toolbar buttons never change, so don't bother remerging its toolbar buttons.
//
HRESULT CInternetToolbar::SetCommandTarget(IUnknown* punkCmdTarget, const GUID* pguidButtonGroup, DWORD dwFlags)
{
    if (!pguidButtonGroup || !punkCmdTarget || IsEqualGUID(CLSID_CommonButtons, *pguidButtonGroup))
        return E_INVALIDARG;

    // this should not change the menu bit or external bands.
    dwFlags |= (_nVisibleBands & (VBF_MENU | VBF_EXTERNALBANDS));

    _btb._fCustomize = !((dwFlags & VBF_NOCUSTOMIZE) || SHRestricted2(REST_NOTOOLBARCUSTOMIZE, NULL, 0));

    // if the new button group is the internet button group, then we're
    // in internet mode; else we're in shell mode
    _fShellView = !(_IsDocHostGUID(pguidButtonGroup));

    _SetSearchStuff();

    HRESULT hr = S_FALSE;

    BOOL fNewButtonGroup = !IsEqualGUID(*pguidButtonGroup, _btb._guidCurrentButtonGroup);
    BOOL fNewCommandTarget = !SHIsSameObject(_btb._pctCurrentButtonGroup, punkCmdTarget);

    // when changing button groups we need to invalidate our cache of buttons for customization
    // why? well, with browse in separate process not turned on, navigating from shell to web
    // reuses the toolbar and some buttons may be disabled for shell but not for browser and vice versa.
    if (fNewButtonGroup)
        _iButtons = -1;
        
    if (fNewButtonGroup || fNewCommandTarget) 
    {
        if (_btb._pctCurrentButtonGroup)
            _btb._pctCurrentButtonGroup->Exec(&IID_IExplorerToolbar, ETCMDID_NEWCOMMANDTARGET, 0, NULL, NULL);

        _btb._guidCurrentButtonGroup = *pguidButtonGroup;
        ATOMICRELEASE(_btb._pctCurrentButtonGroup);
        punkCmdTarget->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&_btb._pctCurrentButtonGroup);

        // A new view can tell us how many rows of text it NEEDs.
        // if it doesn't specify, we give it the default. (stored in _uiDefaultTBTextRows)
        if (dwFlags & VBF_ONELINETEXT )
            _uiTBTextRows = 1;
        else if (dwFlags & VBF_TWOLINESTEXT)
            _uiTBTextRows = 2;
        else
            _uiTBTextRows = _uiTBDefaultTextRows;

        _CompressBands(_fCompressed, _uiTBTextRows, FALSE);

        if (fNewButtonGroup) 
        {
            // new button group; flush toolbar
            _btb._RemoveAllButtons();
            hr = S_OK;
        }

        if (_fUsingDefaultBands && !_fTheater)
            _fUsingDefaultBands = FALSE;
        else
            dwFlags = _nVisibleBands;

        if (dwFlags)
            _ShowBands(dwFlags);
    }

    return hr;
}

HRESULT CInternetToolbar::AddStdBrowserButtons()
{
    //
    // code to add browser buttons has moved to CDocObjectHost::_AddButtons
    //
    ASSERT(0);
    return E_NOTIMPL;
}

void CInternetToolbar::_ReloadButtons()
{
    if (!IsEqualGUID(_btb._guidCurrentButtonGroup, CLSID_CommonButtons) &&
            _btb._pctCurrentButtonGroup)
    {
        HRESULT hres = _btb._pctCurrentButtonGroup->Exec(&IID_IExplorerToolbar, ETCMDID_RELOADBUTTONS, 0, NULL, NULL);
        if (FAILED(hres))
            AddButtons(&_btb._guidCurrentButtonGroup, _btb._cBtnsAdded, _btb._pbtnsAdded);

#ifdef EDIT_HACK
        _InitEditButtonStyle();
#endif

        _UpdateToolbar(TRUE);
    }
}

// The cmdTarget should have already added the Imagelists and the strings.
HRESULT CInternetToolbar::AddButtons(const GUID* pguidButtonGroup, UINT nNewButtons, const TBBUTTON * lpButtons)
{
    if (!pguidButtonGroup || !IsEqualGUID(*pguidButtonGroup, _btb._guidCurrentButtonGroup))
        return E_INVALIDARG;

    if (!IsWindow(_btb._hwnd))
        return E_FAIL;

    LPTBBUTTON lpTBCopy = (LPTBBUTTON)LocalAlloc(LPTR, nNewButtons * sizeof(TBBUTTON));
    if (!lpTBCopy)
        return E_OUTOFMEMORY;

    _CreateBands();
    _btb._RemoveAllButtons();

    memcpy(lpTBCopy, lpButtons, SIZEOF(TBBUTTON) * nNewButtons);

    nNewButtons = _btb._ProcessExternalButtons(lpTBCopy, nNewButtons);

    // Free the old button array
    _btb._FreeBtnsAdded();

    _btb._pbtnsAdded = lpTBCopy;
    _btb._cBtnsAdded = nNewButtons;

    if (_btb._fCustomize && _btb._SaveRestoreToolbar(FALSE))
    {
        // Customization mechanism filled the toolbar for us

        // 
        // The customization mechanism allocated its own set of
        // cmdmaps for the buttons, which means that we need to
        // free those hanging off _pbtnsAdded when _pbtnsAdded is
        // freed.
        //
        _btb._fNeedFreeCmdMapsAdded = TRUE;
        
        _btb._RecalcButtonWidths();

#ifdef EDIT_HACK
        //
        // If we had a custom edit glyph, reload it so that we
        // don't momentarily flash the default glyph during
        // navigation.  We'll update it again when we get a
        // DISPID_DOCUMENTCOMPLETE event.
        //
        _RefreshEditGlyph();
#endif //EDIT_HACK
    }
    else
    {
        // No customization found for this button group

        //
        // We're adding the button array to toolbar directly,
        // and the cmdmaps get freed on TBN_DELETINGBUTTON, so
        // we shouldn't also try to free them when _pbtnsAdded
        // is freed.
        //
        _btb._fNeedFreeCmdMapsAdded = FALSE;

        _AddCommonButtons();
        SendMessage(_btb._hwnd, TB_ADDBUTTONS, nNewButtons, (LPARAM)lpTBCopy);
    }

    _bs._SetMinDimensions();
    return S_OK;
}

HRESULT CInternetToolbar::AddString(const GUID * pguidButtonGroup, HINSTANCE hInst, UINT_PTR uiResID, LONG_PTR *pOffset)
{
    TraceMsg(DM_ITBAR, "CITBar::AddString called");

    *pOffset = -1;

    if (!IsWindow(_btb._hwnd))
    {
        TraceMsg(DM_ERROR, "CITBar::AddString failed");
        return E_FAIL;
    }

    *pOffset= SendMessage(_btb._hwnd, TB_ADDSTRING, (WPARAM)hInst, (LPARAM)uiResID);

    if (*pOffset != -1)
        return S_OK;

    TraceMsg(DM_ERROR, "CITBar::AddString failed");
    return E_FAIL;
}

HRESULT CInternetToolbar::GetButton(const GUID* pguidButtonGroup, UINT uiCommand, LPTBBUTTON lpButton)
{
    UINT_PTR uiIndex = 0;
    TraceMsg(DM_ITBAR, "CITBar::GetButton called");

    if (!pguidButtonGroup || !IsWindow(_btb._hwnd))
        return E_FAIL;

    if (SUCCEEDED(_btb._ConvertCmd(pguidButtonGroup, uiCommand, NULL, &uiCommand)))
    {
        uiIndex = SendMessage(_btb._hwnd, TB_COMMANDTOINDEX, uiCommand, 0L);
        if (SendMessage(_btb._hwnd, TB_GETBUTTON, uiIndex, (LPARAM)lpButton))
        {
            GUID guid;
            _btb._ConvertCmd(NULL, lpButton->idCommand, &guid, (UINT*)&lpButton->idCommand);
            return S_OK;
        }
    }
    return E_FAIL;
}

HRESULT CInternetToolbar::GetState(const GUID* pguidButtonGroup, UINT uiCommand, UINT * pfState)
{
    TraceMsg(DM_ITBAR, "CITBar::GetState called");

    if (!pguidButtonGroup || !IsWindow(_btb._hwnd))
        return E_FAIL;

    if (SUCCEEDED(_btb._ConvertCmd(pguidButtonGroup, uiCommand, NULL, &uiCommand)))
    {
        *pfState = (UINT)SendMessage(_btb._hwnd, TB_GETSTATE, uiCommand, 0L);
        return S_OK;
    }

    return E_FAIL;
}

HRESULT CInternetToolbar::SetState(const GUID* pguidButtonGroup, UINT uiCommand, UINT fState)
{
    BOOL bIsSearchBtn;

    if (!pguidButtonGroup || !IsWindow(_btb._hwnd))
        return E_FAIL;

    TraceMsg(DM_ITBAR, "CITBar::SetState called");

    bIsSearchBtn = uiCommand == TBIDM_SEARCH;
    if (SUCCEEDED(_btb._ConvertCmd(pguidButtonGroup, uiCommand, NULL, &uiCommand)))
    {
        UINT_PTR uiState;

        uiState = SendMessage(_btb._hwnd, TB_GETSTATE, uiCommand, NULL);
        uiState ^= fState;
        if (uiState)
        {
            // search button is being unchecked, change the icon to the default search's
            if (bIsSearchBtn && !(fState & TBSTATE_CHECKED) && !IsEqualGUID(_guidCurrentSearch, _guidDefaultSearch))
            {
                _guidCurrentSearch = _guidDefaultSearch;
            }
            if (SendMessage(_btb._hwnd, TB_SETSTATE, uiCommand, (LPARAM)fState))
            {
                if (uiState & TBSTATE_HIDDEN)
                    _bs._SetMinDimensions();
            }
        }
        return S_OK;
    }
    return E_FAIL;
}

//
//  A bitmap can be added in two ways:
//  1.  Send a bitmap in the hBMPNew field. The uiBMPType parameter needs to be a BITMAP_BMP*
//      The uiCount and the ptb parameters are ignored
//      The offset is placed in puiOffset
//
//  2.  A TBADDBITMAP struct can be sent. The uiCount should have the count
//      uiBMPType parameter needs to be a BITMAP_TBA* value
//      The offset is placed in puiOffset
HRESULT CInternetToolbar::AddBitmap(const GUID * pguidButtonGroup, UINT uiBMPType, UINT uiCount, TBADDBITMAP * ptb, LRESULT * pOffset, COLORREF rgbMask)
{
    UINT uiGetMSG, uiSetMSG;
    TBBMP_LIST tbl = {NULL};

    TraceMsg(DM_ITBAR, "CITBar::AddBitmaP called");
    *pOffset = -1;

    _CreateBands();
    if ((!pguidButtonGroup) || (!IsWindow(_btb._hwnd)) || !_hdsaTBBMPs)
    {
        TraceMsg(DM_ERROR, "CITBar::AddBitmaP failed");
        return E_FAIL;
    }

    // See if we already have the bitmap loaded.
    TBBMP_LIST * pTBBs = NULL;
    int nCount = DSA_GetItemCount(_hdsaTBBMPs);
    for (int nIndex = 0; nIndex < nCount; nIndex++)
    {
        pTBBs = (TBBMP_LIST*)DSA_GetItemPtr(_hdsaTBBMPs, nIndex);
        if ((pTBBs) && (pTBBs->hInst == ptb->hInst) && (pTBBs->uiResID == ptb->nID))
            break;
        pTBBs = NULL;
    }

    // If it was in the commctrl, then we should already have an entry in the DSA
    if ((ptb->hInst == HINST_COMMCTRL) && (!pTBBs))
    {
        TraceMsg(DM_ERROR, "CITBar::AddBitmap failed - bogus ResID for HINST_COMMCTL");
        return E_FAIL;
    }

    // If the icons being added are from fontsext.dll or from dialup networking
    // or the briefcase, then we have it. So just send return the offset
    if (ptb->hInst != HINST_COMMCTRL)
    {
        TCHAR szDLLFileName[MAX_PATH], *pszFN;
        memset(szDLLFileName, 0, ARRAYSIZE(szDLLFileName));
        if (GetModuleFileName(ptb->hInst, szDLLFileName, ARRAYSIZE(szDLLFileName)))
        {
            pszFN = PathFindFileName(szDLLFileName);
            if(!lstrcmpi(pszFN, TEXT("fontext.dll")))
                *pOffset = FONTGLYPH_OFFSET;
            else if (!lstrcmpi(pszFN, TEXT("shell32.dll"))) {
                // 140 and 141 are the glyphs that Shell32.dll uses:
                // IDB_BRF_TB_SMALL        140
                // IDB_BRF_TB_LARGE        141
                if ((ptb->nID == 140) || (ptb->nID == 141))
                    *pOffset = BRIEFCASEGLYPH_OFFSET;
            }
            else if (!lstrcmpi(pszFN, TEXT("rnaui.dll")))
                *pOffset = RNAUIGLYPH_OFFSET;
            else if (!lstrcmpi(pszFN, TEXT("webcheck.dll")))
                *pOffset = WEBCHECKGLYPH_OFFSET;
            if (*pOffset != -1)
                return S_OK;
        }
    }

    // So the bitmaps is not from commctrl. And we have never seen this before.
    // Add an entry into the DSA and then add the bitmap to the himage list.
    if (!pTBBs)
    {
        tbl.hInst = ptb->hInst;
        tbl.uiResID = ptb->nID;
        nIndex = DSA_AppendItem(_hdsaTBBMPs, &tbl);
        if (nIndex  < 0)
        {
            TraceMsg(DM_ERROR, "CITBar::AddBitmap failed");
            return E_FAIL;
        }
        pTBBs = (TBBMP_LIST*)DSA_GetItemPtr(_hdsaTBBMPs, nIndex);
        if (!pTBBs)
        {
            TraceMsg(DM_ERROR, "CITBar::AddBitmap failed");
            return E_FAIL;
        }
    }


    switch(uiBMPType)
    {
    case BITMAP_NORMAL:
        if ((pTBBs) && (pTBBs->fNormal))
        {
            *pOffset = pTBBs->uiOffset;
            return S_OK;
        }
        else if (pTBBs)
            pTBBs->fNormal = TRUE;

        uiGetMSG = TB_GETIMAGELIST; uiSetMSG = TB_SETIMAGELIST;
        break;

    case BITMAP_HOT:
        if ((pTBBs) && (pTBBs->fHot))
        {
            *pOffset = pTBBs->uiOffset;
            return S_OK;
        }
        else if (pTBBs)
            pTBBs->fHot = TRUE;

        uiGetMSG = TB_GETHOTIMAGELIST; uiSetMSG = TB_SETHOTIMAGELIST;
        break;

    case BITMAP_DISABLED:
        if ((pTBBs) && (pTBBs->fDisabled))
        {
            *pOffset = pTBBs->uiOffset;
            return S_OK;
        }
        else if (pTBBs)
            pTBBs->fDisabled = TRUE;

        uiGetMSG = TB_GETDISABLEDIMAGELIST; uiSetMSG = TB_SETDISABLEDIMAGELIST;
        break;

    default:
        ASSERT(FALSE);
        return E_FAIL;
    }

    *pOffset = _AddBitmapFromForeignModule(uiGetMSG, uiSetMSG, uiCount, ptb->hInst, ptb->nID, rgbMask);
    if (pTBBs)
        pTBBs->uiOffset = (UINT)*pOffset;

    return S_OK;
}

// the CmdTarget needs to call this to see what size of bmps we are using.
HRESULT CInternetToolbar::GetBitmapSize(UINT * uiSize)
{
    TraceMsg(DM_ITBAR, "CITBar::GetBitmapSize called");

    *uiSize = g_fSmallIcons ? MAKELONG(TB_SMBMP_CX, TB_SMBMP_CY) : MAKELONG(TB_BMP_CX,TB_BMP_CY);
    return S_OK;
}

HRESULT CInternetToolbar::SetImageList( const GUID* pguidCmdGroup, HIMAGELIST himlNormal, HIMAGELIST himlHot, HIMAGELIST himlDisabled)
{
    if (IsEqualGUID(*pguidCmdGroup, _btb._guidCurrentButtonGroup)) {
        SendMessage(_btb._hwnd, TB_SETIMAGELIST, 1, (LPARAM)himlNormal);
        SendMessage(_btb._hwnd, TB_SETHOTIMAGELIST, 1, (LPARAM)himlHot);
        SendMessage(_btb._hwnd, TB_SETDISABLEDIMAGELIST, 1, (LPARAM)himlDisabled);
    }
    return S_OK;
}

HRESULT CInternetToolbar::ModifyButton( const GUID * pguidButtonGroup, UINT uiCommand, LPTBBUTTON lpButton)
{
    UINT uiIndex = 0;
    TraceMsg(DM_ITBAR, "CITBar::ModifyButton called");

    if (!pguidButtonGroup || !IsWindow(_btb._hwnd))
        return E_FAIL;

    if (SUCCEEDED(_btb._ConvertCmd(pguidButtonGroup, uiCommand, NULL, &uiCommand)))
    {
        TBBUTTONINFO tbbi;
        tbbi.cbSize = SIZEOF(tbbi);
        tbbi.dwMask = TBIF_STATE | TBIF_IMAGE;
        tbbi.fsState = lpButton->fsState;
        tbbi.iImage = lpButton->iBitmap;

        if (SendMessage(_btb._hwnd, TB_SETBUTTONINFO, uiCommand, (LPARAM)&tbbi))
        {
            return S_OK;
        }
    }
    return E_FAIL;
}

HRESULT CInternetToolbar::SendToolbarMsg(const GUID* pguidButtonGroup, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT * plRes)
{
    LRESULT lRes;
    if (!IsWindow(_btb._hwnd))
    {
        TraceMsg(DM_ERROR, "CITBar::SendToolbar Message failed");
        return E_FAIL;
    }

    if (
        // this api is only here for back compat, and these messages didn't
        // exist when the old clients were written

        uMsg == TB_GETBUTTONINFOA ||
        uMsg == TB_GETBUTTONINFOW ||
        uMsg == TB_SETBUTTONINFOA ||
        uMsg == TB_SETBUTTONINFOW ||

        // unsupported right now
        uMsg == TB_ADDBUTTONSA || uMsg == TB_ADDBUTTONSW
       ) {

        ASSERT(0);
        return E_FAIL;
    }



    if ((uMsg == TB_ENABLEBUTTON) || (uMsg == TB_HIDEBUTTON) || (uMsg == TB_CHECKBUTTON) ||
        (uMsg == TB_PRESSBUTTON) || (uMsg == TB_MARKBUTTON))
    {
        unsigned int uiTemp;
        _btb._ConvertCmd(pguidButtonGroup, (UINT)wParam, NULL, &uiTemp);
        wParam = uiTemp;
    }

    if (uMsg == TB_INSERTBUTTON && lParam) {
        TBBUTTON btn = (*(TBBUTTON*)lParam);
        _btb._PreProcessExternalTBButton(&btn);
        lRes = SendMessage(_btb._hwnd, uMsg, wParam, (LPARAM)&btn);
    } else {
        lRes = SendMessage(_btb._hwnd, uMsg, wParam, lParam);

        if (uMsg == TB_GETBUTTON) {
            TBBUTTON* pbtn = (TBBUTTON*)lParam;
            if (pbtn && pbtn->dwData) {
                CMDMAP* pcm = (CMDMAP*)pbtn->dwData;
                pbtn->idCommand = pcm->nCmdID;
                pbtn->dwData = pcm->lParam;
            }
        }
    }

    if (plRes)
        *plRes = lRes;
    return S_OK;
}

TOOLSBANDCLASS::CBrowserToolsBand() : CToolbarBand()
{
    _fCanFocus = TRUE;
}

#define DEFAULT_LIST_VALUE()    (GetUIVersion() >= 5)

void TOOLSBANDCLASS::_FreeBtnsAdded()
{
    if (_pbtnsAdded)
    {
        if (_fNeedFreeCmdMapsAdded)
        {
            for (int i = 0; i < _cBtnsAdded; i++)
            {
                CMDMAP* pcm = (CMDMAP*)_pbtnsAdded[i].dwData;
                _FreeCmdMap(pcm);
            }
        }

        LocalFree(_pbtnsAdded);

        _pbtnsAdded = NULL;
        _cBtnsAdded = 0;
    }
}

LRESULT TOOLSBANDCLASS::_ToolsCustNotify (LPNMHDR pnmh)
{
    LPTBNOTIFY ptbn = (LPTBNOTIFY) pnmh;

    switch (pnmh->code)
    {

    case TBN_SAVE:
    {
        NMTBSAVE *pnmtbs = (NMTBSAVE*)pnmh;
        if (pnmtbs->iItem == -1)
        {
            // before the save
            int nButtons = (int) SendMessage(_hwnd, TB_BUTTONCOUNT, 0, 0L);
            int uSize = pnmtbs->cbData +
                        SIZEOF(BUTTONSAVEINFO) * nButtons +  // stuff for each button
                        SIZEOF(TOOLBARSAVEINFO); // stuff for the toolbar
            pnmtbs->pData = (LPDWORD)LocalAlloc(LPTR, uSize);
            pnmtbs->pCurrent = pnmtbs->pData;
            pnmtbs->cbData = uSize;

            if (pnmtbs->pData)
            {
                TOOLBARSAVEINFO *ptbsi = (TOOLBARSAVEINFO*)pnmtbs->pData;
                ptbsi->cVersion = TBSI_VERSION;
                pnmtbs->pCurrent = (LPDWORD)(ptbsi+1);
            }
        }
        else
        {
            CMDMAP *pcm = (CMDMAP*)pnmtbs->tbButton.dwData;
            BUTTONSAVEINFO* pbsi = (BUTTONSAVEINFO*)pnmtbs->pCurrent;
            pnmtbs->pCurrent = (LPDWORD)(pbsi+1);
            if (pcm)
            {
                pbsi->guid = pcm->guidButtonGroup;
                pbsi->nCmdID = pcm->nCmdID;
                pbsi->fButtonState = pnmtbs->tbButton.fsState;
#ifdef DEBUG
                TCHAR szGuid[80];
                SHStringFromGUID(pcm->guidButtonGroup, szGuid, ARRAYSIZE(szGuid));
                TraceMsg(TF_TBCUST, "Saving: %s - %d (%x)", szGuid, pbsi->nCmdID, pbsi->nCmdID);
#endif
            }
            else
            {
                ASSERT(pnmtbs->tbButton.fsStyle & BTNS_SEP);
                if (pnmtbs->tbButton.idCommand)
                {
                    TraceMsg(TF_TBCUST, "Saving: a separator w/ id %d (%x)", pnmtbs->tbButton.idCommand, pnmtbs->tbButton.idCommand);
                    pbsi->guid = CLSID_Separator;
                    pbsi->nCmdID = pnmtbs->tbButton.idCommand;
                }
                else
                    TraceMsg(TF_TBCUST, "Saving: a separator");
            }
        }
        break;
    }

    case TBN_RESTORE:
        {
            NMTBRESTORE* pnmtbr = (NMTBRESTORE*)pnmh;
            if (pnmtbr->iItem == -1)
            {
                // before the restore.
                // take the data, verify the version,
                // fill in the button count, bytes per record
                // initialize the pCurrent to the end of the tb header
                //
                TOOLBARSAVEINFO* ptbsi = (TOOLBARSAVEINFO*)pnmtbr->pCurrent;
                if (ptbsi->cVersion != TBSI_VERSION)
                    return 1; // abort


                // we're actually going to do a restore.  initialize our database:
                _BuildButtonDSA();

                pnmtbr->pCurrent = (LPDWORD)(ptbsi+1);
                pnmtbr->cbBytesPerRecord += SIZEOF(BUTTONSAVEINFO);
                pnmtbr->cButtons = (pnmtbr->cbData - SIZEOF(TOOLBARSAVEINFO*)) / pnmtbr->cbBytesPerRecord;
                // make sure we did the math right and there are no remainders
                ASSERT(((pnmtbr->cbData - SIZEOF(TOOLBARSAVEINFO*)) % pnmtbr->cbBytesPerRecord) == 0);

                //this is going to clobber all of the buttons in the current toolbar.
                // since toolbar control just writes over the dwords, we need to go free them now.
                int nButtons = (int) SendMessage(_hwnd, TB_BUTTONCOUNT, 0, 0L);

                for (int nTemp = 0; nTemp < nButtons; nTemp++)
                {
                    CMDMAP *pcm = _GetCmdMapByIndex(nTemp);
                    _FreeCmdMap(pcm);
                    TBBUTTONINFO tbbi;
                    tbbi.cbSize = SIZEOF(tbbi);
                    tbbi.lParam = (LPARAM)NULL;
                    tbbi.dwMask = TBIF_LPARAM | TBIF_BYINDEX;
                    SendMessage(_hwnd, TB_SETBUTTONINFO, nTemp, (LPARAM)&tbbi);
                }
            }
            else
            {
                BUTTONSAVEINFO* pbsi = (BUTTONSAVEINFO*)pnmtbr->pCurrent;
                pnmtbr->pCurrent = (LPDWORD)(pbsi+1);
                pnmtbr->tbButton.dwData = 0;
                pnmtbr->tbButton.iString = -1;

                if (IsEqualGUID(CLSID_Separator, pbsi->guid))
                {
                    // restore a separator with a command id
                    pnmtbr->tbButton.fsStyle = BTNS_SEP;
                    TraceMsg(TF_TBCUST, "Restoring: a separator w/ id %d (%x)", pnmtbr->tbButton.idCommand, pnmtbr->tbButton.idCommand);
                }
                else if (!(pnmtbr->tbButton.fsStyle & BTNS_SEP))
                {
                    // Make sure that the button exists for this site
                    CMDMAPCUSTOMIZE* pcmc = _GetCmdMapCustomize(&pbsi->guid, pbsi->nCmdID);
                    if (pcmc == NULL)
                    {
                        // Ignore this button
                        return 1;
                    }

                    CMDMAP* pcm = (CMDMAP*)LocalAlloc(LPTR, SIZEOF(CMDMAP));
                    if (pcm)
                    {
                        pcm->guidButtonGroup = pbsi->guid;
                        pcm->nCmdID = pbsi->nCmdID;

#ifdef DEBUG
                        TCHAR szGuid[80];
                        SHStringFromGUID(pcm->guidButtonGroup, szGuid, ARRAYSIZE(szGuid));
                        TraceMsg(TF_TBCUST, "Restoring: %s - %d (%x)", szGuid, pbsi->nCmdID, pbsi->nCmdID);
#endif

                        // fill in the rest of the info
                        pnmtbr->tbButton = pcmc->btn;
                        pnmtbr->tbButton.fsState = pbsi->fButtonState;
                        pnmtbr->tbButton.dwData = (DWORD_PTR) pcm;

                    }
                }
                else
                {
                    TraceMsg(TF_TBCUST, "Restoring: a separator");
                }
            }
        }
        break;

    case TBN_ENDADJUST:
        _OnEndCustomize();
        break;

    case TBN_TOOLBARCHANGE:
        _pcinfo->fDirty = TRUE;
        break;

    case TBN_INITCUSTOMIZE:
        _OnBeginCustomize((NMTBCUSTOMIZEDLG*)pnmh);
        return TBNRF_HIDEHELP;

    case TBN_RESET:
        _pcinfo->fDirty = FALSE;
        if (_pctCurrentButtonGroup)
        {
            NMTBCUSTOMIZEDLG *pnm = (NMTBCUSTOMIZEDLG*)pnmh;
            CInternetToolbar* pitbar = IToClass(CInternetToolbar, _btb, this);
            TCHAR szGuid[GUIDSTR_MAX];
            SHStringFromGUID(_guidCurrentButtonGroup, szGuid, ARRAYSIZE(szGuid));
            SHDeleteValue(HKEY_CURRENT_USER, REGSTR_PATH_TOOLBAR, szGuid);

            // Default text labels setting should be as follows:
            //
            //  If fullscreen mode, any platform -- "No text labels"
            //  Else if NT5 -- "Selective text on right"
            //  Else  -- "Show text labels"
            //
            int idsDefault;

            if (pitbar->_fTheater)
                idsDefault = IDS_NOTEXTLABELS;
            else if (DEFAULT_LIST_VALUE())
                idsDefault = IDS_PARTIALTEXT;
            else
                idsDefault = IDS_TEXTLABELS;

            _UpdateTextSettings(idsDefault);

            HWND hwnd = (HWND) GetProp(pnm->hDlg, SZ_PROP_CUSTDLG);
            if (hwnd)
            {
                // update our dialog's control selection states
                _SetDialogSelections(hwnd, _DefaultToSmallIcons());
            }

            _RemoveAllButtons();
            _OnEndCustomize();
            if (_pbtnsAdded)
            {
                pitbar->AddButtons(&_guidCurrentButtonGroup, _cBtnsAdded, _pbtnsAdded);

#ifdef EDIT_HACK
                // Restore the edit button
                pitbar->_InitEditButtonStyle();
#endif

                pitbar->_UpdateToolbar(TRUE);
            }
            else
            {
                return TBNRF_ENDCUSTOMIZE;
            }
        }
        break;

    case TBN_QUERYINSERT:
        return TRUE;

    case TBN_QUERYDELETE:
        return (SendMessage(_hwnd, TB_ISBUTTONHIDDEN,
                            (WPARAM) ptbn->tbButton.idCommand,
                            (LPARAM) 0)) ? FALSE : TRUE;

    case TBN_GETBUTTONINFO:
        if (ptbn->iItem < DSA_GetItemCount(_pcinfo->hdsa))
        {
            CMDMAPCUSTOMIZE *pcmc;
            pcmc = (CMDMAPCUSTOMIZE*)DSA_GetItemPtr(_pcinfo->hdsa, ptbn->iItem);
            ptbn->tbButton = pcmc->btn;
            ptbn->tbButton.fsState &= ~TBSTATE_HIDDEN;
            return TRUE;
        }
        return FALSE;

    case TBN_BEGINADJUST:
        if (!_pcinfo || !_pcinfo->fAdjust)
            return 1;
        break;

    }
    return FALSE;
}

BOOL TOOLSBANDCLASS::_SaveRestoreToolbar(BOOL fSave)
{
    TBSAVEPARAMS tbsp;
    TCHAR szGuid[GUIDSTR_MAX];
    SHStringFromGUID(_guidCurrentButtonGroup, szGuid, ARRAYSIZE(szGuid));

    tbsp.hkr = HKEY_CURRENT_USER;
    tbsp.pszSubKey = REGSTR_PATH_TOOLBAR;
    tbsp.pszValueName = szGuid;
    BOOL fRet = BOOLFROMPTR(SendMessage(_hwnd, TB_SAVERESTORE, (WPARAM) fSave, (LPARAM) &tbsp));

    _FreeCustomizeInfo();
    return fRet;
}

int TOOLSBANDCLASS::_CommandFromIndex(UINT uIndex)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = SIZEOF(tbbi);
    tbbi.dwMask = TBIF_COMMAND | TBIF_BYINDEX;
    SendMessage(_hwnd, TB_GETBUTTONINFO, uIndex, (LPARAM)&tbbi);
    return tbbi.idCommand;
}

//  _btb._ConvertCmd()
//  This is used to covert a external Command ID to an internal ID or vice versa
//  If we are converting to an external ID then
//      call with pguidButtonGroup == NULL                  (to external:  pguidButtonGroup == NULL)
//      otherwise call with the external button group GUID  (to internal:  pguidOut == NULL)
HRESULT TOOLSBANDCLASS::_ConvertCmd(const GUID* pguidButtonGroup, UINT id, GUID* pguidOut, UINT * pid)
{
    HRESULT hres = E_FAIL;
    BOOL fToInternal = (bool) (pguidButtonGroup);

    ASSERT((pguidButtonGroup == NULL) ^ (pguidOut == NULL));

    // First look for the command
    if (fToInternal && _hwnd) {
        int nCount = (int) SendMessage(_hwnd, TB_BUTTONCOUNT, 0, 0);
        for (int i = 0; i < nCount; i++) {
            CMDMAP *pcm = _GetCmdMapByIndex(i);

            if (pcm) {
                ASSERT(pcm);

                // loop through the command mapping structures until we
                // find this guid and id
                if (IsEqualGUID(pcm->guidButtonGroup, *pguidButtonGroup) &&
                    id == pcm->nCmdID) {

                    *pid = _CommandFromIndex(i);
                    hres = S_OK;
                    break;
                }
            }
        }

    } else {

        // going from toolbar id to commandtarget info
        CMDMAP *pcm = _GetCmdMapByID(id);
        if (pcm) {
            *pguidOut = pcm->guidButtonGroup;
            *pid = pcm->nCmdID;
            hres = S_OK;
        }
    }
    return hres;
}



LRESULT CInternetToolbar::_AddBitmapFromForeignModule(UINT uiGetMSG, UINT uiSetMSG, UINT uiCount, HINSTANCE hinst, UINT_PTR nID, COLORREF rgbMask)
{
    HBITMAP hBMPRaw = NULL, hBMPFixedUp = NULL;
    HBITMAP * phBmp = &hBMPFixedUp;
    BITMAP bmp;
    HIMAGELIST himlTemp;
    LRESULT lRes = 1L;
    BOOL fOk = TRUE;
    HDC dc = NULL, dcMemSrc = NULL, dcMemDest = NULL;
    HBITMAP hbmpOldDest = NULL, hbmpOldSrc = NULL;
    int cxOrg = 0;
    int xDest = 0, yDest = 0;
    RECT rect = {0,0,0,0};
    HBRUSH hbr = NULL;

    // BUGBUG What if hinst == NULL?  That means that nID is really an HBITMAP

    if (!(hBMPRaw = LoadBitmap(hinst, MAKEINTRESOURCE(nID))))
        return 0L;

    fOk = (BOOL)(GetObject(hBMPRaw, sizeof(BITMAP), &bmp) != 0);

    // Check is the size is OK
    if (fOk && (bmp.bmWidth != (LONG)(TB_BMP_CX * uiCount)) || (bmp.bmHeight != (LONG)TB_BMP_CY) )
    {
        int cxBmp;
        int cyBmp;

        if (g_fSmallIcons) {
            cxBmp = TB_SMBMP_CX;
            cyBmp = TB_SMBMP_CY;
        } else {
            cxBmp = TB_BMP_CX;
            cyBmp = TB_BMP_CY;
        }

        // If the height is 15, the we assume that this is one of the old bitmaps therefore
        // the width is 16. We cannot rely on the (bmp.bmWidth / uiCount) because some apps
        // like SecureFile give us a bitmap 192 wide and say that there are 10 glyphs in it.
        if (bmp.bmHeight == 15)
            cxOrg = 16;
        else
            cxOrg = bmp.bmWidth / (uiCount ? uiCount : 1);

        if (rgbMask)
            fOk = (BOOL)((hbr = CreateSolidBrush(rgbMask))!= NULL);

        if (fOk)
            fOk = (BOOL)((dc = GetDC(_btb._hwnd)) != NULL);

        if (fOk)
            fOk = (BOOL)((hBMPFixedUp = CreateCompatibleBitmap(dc, (cxBmp * uiCount), cyBmp)) != NULL);

        if (fOk)
            fOk = (BOOL)((dcMemSrc = CreateCompatibleDC(dc)) != NULL);

        if (fOk)
            fOk = (BOOL)((dcMemDest = CreateCompatibleDC(dc)) != NULL);

        if (!fOk)
            goto Error;

        hbmpOldSrc = (HBITMAP)SelectObject(dcMemSrc, hBMPRaw);
        hbmpOldDest = (HBITMAP)SelectObject(dcMemDest, hBMPFixedUp);

        rect.right = (cxBmp * uiCount);
        rect.bottom = cyBmp;
        if (rgbMask)
            FillRect(dcMemDest, &rect, hbr);

        for (UINT n = 0; n < uiCount; n++)
        {

            int cxCopy;
            int cyCopy;

            xDest = (n * cxBmp);
            if (cxOrg < cxBmp) {
                // if the bitmap is too small, we need to center it.
                // the amount we copy is the full bitmap
                cxCopy = cxOrg;
                xDest += ((cxBmp - cxOrg) / 2);
            } else {
                // if the bitmap is big enough, we align it to top left and
                // we strecth(shrink) it down to fit
                cxCopy = cxBmp;
            }

            if (bmp.bmHeight < cyBmp) {
                cyCopy = bmp.bmHeight;
                yDest = ((cyBmp - bmp.bmHeight) / 2);
            } else {
                cyCopy = cyBmp;
                yDest = 0;
            }
            StretchBlt(dcMemDest, xDest, yDest, cxOrg, bmp.bmHeight,
                   dcMemSrc, (cxOrg * n), 0, cxCopy, cyCopy, SRCCOPY);

        }

        SelectObject(dcMemDest, hbmpOldDest);
        SelectObject(dcMemSrc, hbmpOldSrc);
    }
    else
        phBmp = &hBMPRaw;

    if (!(himlTemp = (HIMAGELIST)SendMessage(_btb._hwnd, uiGetMSG, 0, 0L)))
    {
        TraceMsg(DM_ERROR, "CITBar::AddBitmaPFromForeignModulefailed");
        fOk = FALSE;
        goto Error;
    }

    if (rgbMask)
        lRes = ImageList_AddMasked(himlTemp, (HBITMAP)*phBmp, rgbMask);
    else
        lRes = ImageList_Add(himlTemp, (HBITMAP)*phBmp, NULL);

    if (lRes == -1)
    {
        TraceMsg(DM_ERROR, "CITBar::AddBitmaPFromForeignModulefailed");
        fOk = FALSE;
        goto Error;
    }


    if (!SendMessage(_btb._hwnd, uiSetMSG, 0, (LPARAM)himlTemp))
    {
        TraceMsg(DM_ERROR, "CITBar::AddBitmaPFromForeignModulefailed");
        fOk = FALSE;
        goto Error;
    }

Error:
    if (hBMPFixedUp)
        DeleteObject(hBMPFixedUp);

    if (hBMPRaw)
        DeleteObject(hBMPRaw);

    if (dc)
        ReleaseDC(_btb._hwnd, dc);

    if (dcMemSrc)
        DeleteDC(dcMemSrc);

    if (dcMemDest)
        DeleteDC(dcMemDest);

    if (hbr)
        DeleteObject(hbr);

    if (!fOk)
        lRes = 0L;

    return lRes;
}

#define VERY_HIGH_NUMBER    4000
HRESULT CInternetToolbar::_LoadDefaultSettings()
{
    memset(&_cs, 0, sizeof(COOLBARSAVE));
    _cs.cbVer       = CBS_VERSION;

    _cs.bs[0].wID    = CBIDX_MENU;
    _cs.bs[0].cx     = VERY_HIGH_NUMBER;

    _cs.bs[1].wID    = CBIDX_BRAND;

    _cs.bs[2].wID    = CBIDX_TOOLS;
    _cs.bs[2].cx     = VERY_HIGH_NUMBER;
    _cs.bs[2].fStyle = RBBS_BREAK;

    _cs.bs[3].wID    = CBIDX_ADDRESS;
    _cs.bs[3].cx     = VERY_HIGH_NUMBER;
    _cs.bs[3].fStyle = RBBS_BREAK;

    _cs.bs[4].wID    = CBIDX_LINKS;

    if (!_fInitialPidlIsWeb) {
        // we're in shell view, or we're rooted.  for perf,
        // don't bother creating the links band.
        _cs.uiVisible = (VBF_MENU | VBF_TOOLS | VBF_ADDRESS | VBF_BRAND);
    } else {
        // web page
        _cs.uiVisible = (VBF_MENU | VBF_TOOLS | VBF_ADDRESS | VBF_LINKS | VBF_BRAND);
    }

    _cs.clsidVerticalBar = GUID_NULL;
    _cs.clsidHorizontalBar = GUID_NULL;
    _cs.fNoText = FALSE;
    _cs.fList = DEFAULT_LIST_VALUE();

    _fUsingDefaultBands = TRUE;

    return(NOERROR);
}

typedef struct tagCOOLBARSAVEv12    // IE4
{
    UINT        cbVer;
    UINT        uiMaxTBWidth;
    UINT        uiMaxQLWidth;
#ifdef UNIX
    BITBOOL     fUnUsed : 28;       // unused
#endif
    BOOL        fVertical : 1;      // The bar is oriented vertically
    BOOL        fNoText :1;         // "NoText"
    BOOL        fAutoHide : 1;      // Auto hide toolbar in theater mode
    BOOL        fStatusBar : 1;     // Status bar in theater mode
    BOOL        fSaveInShellIntegrationMode : 1;     // Did we save in shell
    UINT        uiVisible;          // "Visible bands"
    UINT        cyRebar;
    BANDSAVE    bs[5];
} COOLBARSAVEv12;

typedef struct tagCOOLBARSAVEv15    // IE5 Beta2
{
    UINT        cbVer;
    UINT        uiMaxTBWidth;
    UINT        uiMaxQLWidth;
#ifdef UNIX
    BITBOOL     fUnUsed : 28;       // unused
#endif
    BITBOOL     fVertical : 1;      // The bar is oriented vertically
    BITBOOL     fNoText :1;         // "NoText"
    BITBOOL     fList : 1;          // toolbar is TBSTYLE_LIST (text on right) + TBSTYLE_EX_MIXEDBUTTONS
    BITBOOL     fAutoHide : 1;      // Auto hide toolbar in theater mode
    BITBOOL     fStatusBar : 1;     // Status bar in theater mode
    BITBOOL     fSaveInShellIntegrationMode : 1;     // Did we save in shell integration mode?
    UINT        uiVisible;          // "Visible bands"
    UINT        cyRebar;
    BANDSAVE    bs[5];
    CLSID       clsidVerticalBar;       //clsid of bar persisted within vertical band
    CLSID       clsidHorizontalBar;
} COOLBARSAVEv15;

#define CB_V12  (sizeof(COOLBARSAVEv12))
#define CB_V13  (sizeof(COOLBARSAVEv15))
#define CB_V14  CB_V13          // 14: added fList:1 (in middle!)
#define CB_V15  CB_V14          // 15: new rbbi.fStyle semantics
#define CB_V17  (sizeof(COOLBARSAVE))

HRESULT CInternetToolbar::_LoadUpgradeSettings(ULONG cbRead)
{
    // If we shipped with the CBS_VERSION you're incrementing, you need
    // to add upgrade code here for that version, then update this assertion.
    COMPILETIME_ASSERT(CBS_VERSION == 17);

    // Double-check our size calculations.
#ifndef UNIX
    COMPILETIME_ASSERT(CB_V12 == (6 * sizeof(UINT) + CBIDX_LAST * sizeof(BANDSAVE)));
#endif
    COMPILETIME_ASSERT(CB_V12 == (CB_V15 - SIZEOF(CLSID) * 2));
    COMPILETIME_ASSERT(CB_V13 == (CB_V12 + 2 * sizeof(CLSID)));
    COMPILETIME_ASSERT(CB_V14 == (CB_V13 + 0));
    COMPILETIME_ASSERT(CB_V15 == (CB_V14 + 0));
    COMPILETIME_ASSERT(CB_V17 == (CB_V15 + (MAXEXTERNALBANDS * sizeof(BANDSAVE)) + (MAXEXTERNALBANDS * sizeof(CLSID))));

    // If the stream was shorter than the version data field, there's nothing we can do.
    if (cbRead < SIZEOF(_cs.cbVer))
    {
        return E_FAIL;
    }

    // Check to see if the version is one we know about and that the stream
    // size is the same as that version's structure size.
    if (!((_cs.cbVer == 12 && cbRead == CB_V12) ||      // IE4
          (_cs.cbVer == 13 && cbRead == CB_V13) ||      // ?
          (_cs.cbVer == 14 && cbRead == CB_V14) ||      // ?
          (_cs.cbVer == 15 && cbRead == CB_V15)))       // IE5 Beta2
    {
        return E_FAIL;
    }

    TraceMsg(DM_WARNING, "citb._lus: try upgrade %d->%d", _cs.cbVer, CBS_VERSION);

    // Make a scratch copy of _cs so we don't worry about overwriting
    // parts of _cs we need to read later.
    COOLBARSAVE cs = _cs;

    if (_cs.cbVer == 12)
    {
        // clsidVerticalBar/clsidHorizontalBar weren't a part
        // of the structure until v13
        cs.clsidVerticalBar = GUID_NULL;
        cs.clsidHorizontalBar = GUID_NULL;
    }
    else
    {
        ASSERT(_cs.cbVer < 16);

        // Band array (bs) grew in v16 to include external bands, so
        // clsidVerticalBar/clsidHorizontalBar were at a different offset.
        COOLBARSAVEv15 *pv15 = (COOLBARSAVEv15 *) &_cs;
        cs.clsidVerticalBar = pv15->clsidVerticalBar;
        cs.clsidHorizontalBar = pv15->clsidHorizontalBar;
        cs.bs[CBIDX_LAST].wID = 0xFFFFFFFF;
    }

    if (InRange(_cs.cbVer, 12, 13))
    {
        // fList was inserted into the middle of the bitbool list in v14.
        // Copy the displaced bitbools and initialize fList.
        COOLBARSAVEv12 *pv12 = (COOLBARSAVEv12 *) &_cs;
        cs.fAutoHide = pv12->fAutoHide;
        cs.fStatusBar = pv12->fStatusBar;
        cs.fSaveInShellIntegrationMode = pv12->fSaveInShellIntegrationMode;
        cs.fList = DEFAULT_LIST_VALUE();
    }

    // Force FALSE as no longer support vertical itbar mode.
    cs.fVertical = FALSE;

    // Strip off any invalid visible band bits
    cs.uiVisible &= VBF_VALID;

    // Set current version and copy scratch cs back to _cs
    cs.cbVer = CBS_VERSION;
    _cs = cs;

    return S_OK;
}

HRESULT CInternetToolbar::_LoadDefaultWidths()
{
    // If there was no max width set for the QL bar or the Toolbar, then
    // before we use the default, check it the localization people wanted to
    // increase the width. The RC file string range from '0' to '9'
    TCHAR szScratch[16];
    UINT uiExtraWidth = 0;

    if (GetSystemMetrics(SM_CXSCREEN) < 650) {
        MLLoadString(IDS_TB_WIDTH_EXTRA_LORES, szScratch, ARRAYSIZE(szScratch));
        _uiMaxTBWidth = MAX_TB_WIDTH_LORES;
    } else {
        MLLoadString(IDS_TB_WIDTH_EXTRA_HIRES, szScratch, ARRAYSIZE(szScratch));
        _uiMaxTBWidth = MAX_TB_WIDTH_HIRES;
    }
    _uiMaxTBWidth += StrToInt(szScratch) * WIDTH_FACTOR;


    return(NOERROR);
}

BOOL IsClsidInHKCR(REFGUID pclsid)
{
    HKEY hkeyResult;

    if (SHRegGetCLSIDKeyW(pclsid, NULL, FALSE, FALSE, &hkeyResult) == ERROR_SUCCESS)
    {
        RegCloseKey(hkeyResult);
        return TRUE;
    }
    return FALSE;
}

typedef struct tagCOOLBARSAVEv2     // IE3
{
    UINT        cbVer;
    UINT        uiMaxTBWidth;
    UINT        uiMaxQLWidth;
    BOOL        fVertical;             // The bar is oriented vertically
    BANDSAVE    bs[4];
} COOLBARSAVEv2;

#define VBF_VALIDv2               (VBF_TOOLS | VBF_ADDRESS | VBF_LINKS)

void CInternetToolbar::_TryLoadIE3Settings()
{
    HKEY hKey;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegKeyCoolbar, 0, KEY_QUERY_VALUE, &hKey))
    {
        COOLBARSAVEv2 cbv2;
        DWORD dwcbData = SIZEOF(cbv2);
        if (SHQueryValueEx(hKey, TEXT("Layout"), NULL, NULL, (LPBYTE)&cbv2, &dwcbData) == ERROR_SUCCESS)
        {
            _cs.uiMaxTBWidth = cbv2.uiMaxTBWidth;
            _cs.uiMaxQLWidth = cbv2.uiMaxQLWidth;
            // BUGBUG: todo -- read in bs field too; need to do some conversions as
            // CBIDX_ numbers were zero-based and there was no menuband in IE3.
        }

        BOOL fNoText;
        dwcbData = SIZEOF(fNoText);
        if (SHQueryValueEx(hKey, TEXT("NoText"), NULL, NULL, (LPBYTE)&fNoText, &dwcbData) == ERROR_SUCCESS)
        {
            // Set the no-text flag.
            _cs.fNoText = BOOLIFY(fNoText);
        }

        UINT uiVisible;
        dwcbData = SIZEOF(uiVisible);
        if (SHQueryValueEx(hKey, TEXT("VisibleBands"), NULL, NULL, (LPBYTE)&uiVisible, &dwcbData) == ERROR_SUCCESS)
        {
            // Set the visible bands, changing only the ones that IE3 knew about.
            _cs.uiVisible = (_cs.uiVisible &~ VBF_VALIDv2) | (uiVisible & VBF_VALIDv2);
        }

        RegCloseKey(hKey);
    }
}

HRESULT CInternetToolbar::Load(IStream *pstm)
{
    ULONG  ulRead;

    //Read from the given stream and initialize the Toolbar data!

    _fLoading = TRUE;

    HRESULT hr = pstm->Read(&_cs, sizeof(COOLBARSAVE), &ulRead);

    if (SUCCEEDED(hr))
    {
        if (ulRead != sizeof(COOLBARSAVE) || _cs.cbVer != CBS_VERSION)
            hr = _LoadUpgradeSettings(ulRead);
    }

    if (FAILED(hr))
    {
        _LoadDefaultSettings();
    }

    ASSERT(_cs.uiVisible & VBF_MENU);
    // make sure that the settings include a menu
    _cs.uiVisible |= VBF_MENU;

    _LoadDefaultWidths();
    _CreateBands();

    //if in web view, show the last visible browser bars too
    if (!_fShellView)
    {
        VARIANT varOut = {0};
        varOut.vt = VT_I4;

        if (!IsEqualGUID(_cs.clsidVerticalBar, GUID_NULL) && IsClsidInHKCR(_cs.clsidVerticalBar))
        {
#ifdef UNIX
            // IEUNIX: Donot persist/load MsgBand
            if (!IsEqualGUID(_cs.clsidVerticalBar, CLSID_MsgBand))
#endif
            if (!IsEqualGUID(_cs.clsidVerticalBar, CLSID_SearchBand) &&
                !IsEqualGUID(_cs.clsidVerticalBar, CLSID_FileSearchBand))
            {
                WCHAR wsz[GUIDSTR_MAX];
                SHStringFromGUID((const CLSID)_cs.clsidVerticalBar, wsz, ARRAYSIZE(wsz));

                VARIANT varClsid;
                varClsid.vt = VT_BSTR;
                varClsid.bstrVal = wsz;

                IUnknown_Exec(_pbs2, &CGID_ShellDocView, SHDVID_SHOWBROWSERBAR, 1, &varClsid, &varOut);
            }
            else
            {
                //if it's the search band, must be shown in this way to get correct search
                VARIANTARG var;
                var.vt = VT_I4;
                var.lVal = -1;

                Exec(&CLSID_CommonButtons, TBIDM_SEARCH, 0, NULL, &var);
            }
        }

        if (!IsEqualGUID(_cs.clsidHorizontalBar, GUID_NULL) && IsClsidInHKCR(_cs.clsidHorizontalBar))
        {
            WCHAR wsz[GUIDSTR_MAX];
            SHStringFromGUID((const CLSID)_cs.clsidHorizontalBar, wsz, ARRAYSIZE(wsz));

            VARIANT varClsid;
            varClsid.vt = VT_BSTR;
            varClsid.bstrVal = wsz;

            IUnknown_Exec(_pbs2, &CGID_ShellDocView, SHDVID_SHOWBROWSERBAR, 1, &varClsid, &varOut);
        }
    }
    _fLoading = FALSE;

    return(S_OK);
}

//see APPHACK note below
const GUID CLSID_AlexaVert = { 0xBA0B386CL, 0x7143, 0x11d1, 0xba, 0x8c, 0x00, 0x60, 0x08, 0x27, 0x87, 0x8d };
const GUID CLSID_AlexaHorz = { 0xBA0B386EL, 0x7143, 0x11d1, 0xba, 0x8c, 0x00, 0x60, 0x08, 0x27, 0x87, 0x8d };

void CInternetToolbar::_GetVisibleBrowserBar(UINT idBar, CLSID *pclsidOut)
{
    *pclsidOut = GUID_NULL;

    ASSERT(idBar == IDBAR_VERTICAL || idBar == IDBAR_HORIZONTAL);

    IDockingWindowFrame *psb;
    if (_psp && SUCCEEDED(_psp->QueryService(SID_STopLevelBrowser, IID_IDockingWindowFrame, (void**)&psb)))
    {
        IDeskBar* pdb;

        if ( (IDBAR_VERTICAL   == idBar && (SUCCEEDED(psb->FindToolbar(INFOBAR_TBNAME, IID_IDeskBar, (void **)&pdb)) && pdb)) ||
             (IDBAR_HORIZONTAL == idBar && (SUCCEEDED(psb->FindToolbar(COMMBAR_TBNAME, IID_IDeskBar, (void **)&pdb)) && pdb)) )
        {
            VARIANT varClsid = {0};

            if (SUCCEEDED(IUnknown_Exec(pdb, &CGID_DeskBarClient, DBCID_CLSIDOFBAR, 1, NULL, &varClsid)))
            {
                if (varClsid.vt == VT_BSTR)
                {
                    GUIDFromString(varClsid.bstrVal, pclsidOut);
                    VariantClear(&varClsid);
                }

//APPHACK
// Alexa 3.0 has some code so that their explorer bar persists that works in ie4. however, when ie5
// persists them, they don't handle the case where the main page has not finished loading yet, which
// causes them to fault on launch of the browser. see IE5 55895.
                if ( (IDBAR_VERTICAL   == idBar && (IsEqualGUID(*pclsidOut, CLSID_AlexaVert))) ||
                     (IDBAR_HORIZONTAL == idBar && (IsEqualGUID(*pclsidOut, CLSID_AlexaHorz))) )
                {
                    *pclsidOut = GUID_NULL;
                }
//END APPHACK
            }

            pdb->Release();
        }
        psb->Release();
    }
}

void CInternetToolbar::_BuildSaveStruct(COOLBARSAVE* pcs)
{
    REBARBANDINFO   rbbi;
    RECT rc;
    static BOOL fBrowserOnly = (WhichPlatform() != PLATFORM_INTEGRATED);

    //Save into the given stream!
    memset(pcs, 0, sizeof(COOLBARSAVE));
    pcs->cbVer = CBS_VERSION;

    // Browser Only can't load Shell Integrated streams because of the Favorites
    // shell extension created pidls unreadable by browser only which doesn't have the Favorites ShellExt
    pcs->fSaveInShellIntegrationMode = !fBrowserOnly;

    GetWindowRect(_bs._hwnd, &rc);
    pcs->cyRebar = RECTHEIGHT(rc);
    //Save the new fields.
    pcs->fAutoHide = _fAutoHide;
    pcs->fNoText = _fCompressed;
    pcs->fList = IS_LIST_STYLE(_btb._hwnd);
    pcs->uiVisible = _nVisibleBands;

    //only persist the visible bars for web view
    if (!_fShellView)
    {
        _GetVisibleBrowserBar(IDBAR_VERTICAL, &pcs->clsidVerticalBar);
        _GetVisibleBrowserBar(IDBAR_HORIZONTAL, &pcs->clsidHorizontalBar);
    }
    //else pcs->clsid*Bar nulled out by memset above

    LRESULT lStyle = GetWindowLong(_bs._hwnd, GWL_STYLE);
    pcs->fVertical = BOOLIFY(lStyle & CCS_VERT);

    pcs->uiMaxTBWidth = _uiMaxTBWidth;

    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_STYLE | RBBIM_SIZE | RBBIM_ID;
    int icBands = (int) SendMessage( _bs._hwnd, RB_GETBANDCOUNT, 0, 0 );
    for (int i = 0; i < icBands; i++)
    {
        pcs->bs[i].wID = 0xFFFFFFFF;
        if (SendMessage(_bs._hwnd, RB_GETBANDINFO, i, (LPARAM) &rbbi))
        {
            if (rbbi.wID < CBANDSMAX)
            {
                // desk band objects have the choice of not saving there visibility
                // state
                BANDITEMDATA *pbid = _bs._GetBandItem( i );
                UINT uiMask = rbbi.wID <= CBIDX_LAST ? ( 1 << (rbbi.wID - 1) ) : EXTERNALBAND_VBF_BIT(rbbi.wID - CBIDX_LAST-1);
                if (pbid && pbid->pdb && (pcs->uiVisible & uiMask))
                {
                    OLECMD cmd;
                    cmd.cmdID = CITIDM_DISABLEVISIBILITYSAVE;
                    cmd.cmdf = 0;
                    IUnknown_QueryStatus( pbid->pdb, &CGID_PrivCITCommands, 1, &cmd, NULL );
                    if ( cmd.cmdf &  OLECMDF_ENABLED )
                    {
                        pcs->uiVisible &= ~uiMask;
                        rbbi.fStyle |= RBBS_HIDDEN;
                    }
                }
                pcs->bs[i].fStyle = rbbi.fStyle;
                pcs->bs[i].cx = rbbi.cx;
                pcs->bs[i].wID = rbbi.wID;
                if (IS_EXTERNALBAND(rbbi.wID))
                {
                    pcs->aclsidExternalBands[MAP_TO_EXTERNAL(rbbi.wID)] = _rgebi[MAP_TO_EXTERNAL(rbbi.wID)].clsid;
                }
            }
        }
    }
    // Query CShellBrowser for status bar state
    VARIANTARG v = { 0 };
    v.vt = VT_I4;
    IUnknown_Exec(_ptbsite, &CGID_ShellBrowser, FCIDM_GETSTATUSBAR,
        0, NULL, &v);
    pcs->fStatusBar = v.lVal;
}

typedef struct tagCLSID_BANDTYPE
{
    const CLSID * pclsid;
    DWORD dwBandID;
} CLSID_BANDTYPE;

CLSID_BANDTYPE c_CLSIDsToSave[] =
{
    {&CLSID_AddressBand, CBIDX_ADDRESS},
    {&CLSID_QuickLinks, CBIDX_LINKS},
};

HRESULT CInternetToolbar::Save(IStream *pstm, BOOL fClearDirty)
{
    COOLBARSAVE cs;
    HRESULT hr = S_FALSE;

    // avoid the hit of saving when we are still loading.  State will not have
    // changed, at least not enough to justify saving, until after we are loaded.
    if (_fLoading)
        return(S_OK);

    //Check the dirty bit to see if we need to save.
    if(!_fDirty)
        return(S_OK);

    ASSERT(!_fTheater);
    _BuildSaveStruct(&cs);

    if(SUCCEEDED(hr = pstm->Write(&cs, sizeof(COOLBARSAVE), NULL)) && fClearDirty)
        _fDirty = FALSE;

    REBARBANDINFO rbbi;
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_ID;

    int icBands = (int) SendMessage( _bs._hwnd, RB_GETBANDCOUNT, 0, 0 );
    for (int i = 0; i < icBands; i++)
    {
        if (SendMessage(_bs._hwnd, RB_GETBANDINFO, i, (LPARAM) &rbbi))
        {
            if ((rbbi.wID == CBIDX_ADDRESS) || (rbbi.wID == CBIDX_LINKS) || IS_EXTERNALBAND(rbbi.wID))
            {
                BANDITEMDATA *pbid = _bs._GetBandItem( i );
                if (pbid && pbid->pdb) {
                    CLSID clsid;
                    IPersistStream *pStream;
                    if (SUCCEEDED(pbid->pdb->QueryInterface( IID_IPersistStream, (LPVOID *) &pStream ))) {
                        pStream->GetClassID( &clsid );
                        IStream * pstm;
                        TCHAR szGUID[MAX_PATH];
                        SHStringFromGUID( clsid, szGUID, ARRAYSIZE(szGUID) );
                        pstm = GetRegStream( _fInitialPidlIsWeb, szGUID, STGM_WRITE | STGM_CREATE );
                        if (pstm)
                        {
                            HRESULT hrInternal = _bs.SaveToStreamBS(pbid->pdb, pstm);

                            // Only return Success values
                            if (SUCCEEDED(hrInternal))
                                hr = S_OK;
                            pstm->Release();
                        }
                        pStream->Release();
                    }
                }
            }
        }
    }
    return(hr);
}

HRESULT CInternetToolbar::InitNew(void)
{
    // This shouldn't get called if Load has already been called, so assert
    // that _cs is uninitialized.
    ASSERT(_cs.cbVer == 0);

    _LoadDefaultSettings();

    // Look for any IE3 settings and override defaults with those.  (IE3
    // wrote structures directly to registry, rather than via IPersistStream).
    _TryLoadIE3Settings();

    _LoadDefaultWidths();

    return S_OK;
}

BOOL CInternetToolbar::_SendToToolband(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    return _bs._SendToToolband(hwnd, uMsg, wParam, lParam, plres);
}


HRESULT CInternetToolbar::IsDirty(void)
{
    if (_fDirty)
        return S_OK;
    else
        return S_FALSE;
}


HRESULT CInternetToolbar::QueryService(REFGUID guidService,
                                       REFIID riid, void **ppvObj)
{
    HRESULT hres = E_NOTIMPL;

    if (IsEqualIID(guidService, SID_IBandProxy))
    {
        hres = QueryService_SID_IBandProxy(SAFECAST(_ptbsitect, IUnknown *), riid, &_pbp, ppvObj);
        if(!_pbp)
        {
            // We need to create it ourselves since our parent couldn't help
            ASSERT(FALSE == _fCreatedBandProxy);

            hres = CreateIBandProxyAndSetSite(SAFECAST(_ptbsitect, IUnknown *), riid, &_pbp, ppvObj);
            if(_pbp)
            {
                ASSERT(S_OK == hres);
                _fCreatedBandProxy = TRUE;
            }
        }
        return hres;
    }
    else if (IsEqualGUID(guidService, IID_IBandSite))
    {
        return _bs.QueryInterface(riid, ppvObj);
    }
    else if (IsEqualGUID(guidService, IID_IAddressBand))
    {
        LPBANDITEMDATA pbid = _bs._GetBandItemDataStructByID(CBIDX_ADDRESS);

        ASSERT(pbid && pbid->pdb);     // pbid->pdb is a IDeskBand pointer
        if (pbid && pbid->pdb)
        {
            return pbid->pdb->QueryInterface(riid, ppvObj);
        }
        else
        {
            *ppvObj = NULL;
            return E_FAIL;
        }
    }

    if (_psp)
        return _psp->QueryService(guidService, riid, ppvObj);


    return SUPERCLASS::QueryService(guidService, riid, ppvObj);
}

//
// BUGBUG: Do we really need to implement the following two functions?
// Currently nobody uses them.
//
HRESULT CInternetToolbar::GetClassID(GUID *pguid)
{
    *pguid = CLSID_InternetToolbar;
    return(S_OK);
}

HRESULT CInternetToolbar::GetSizeMax(ULARGE_INTEGER *ulMaxSize)
{
    ulMaxSize->LowPart = sizeof(COOLBARSAVE);
    ulMaxSize->HighPart = 0;
    return(S_OK);
}


CInternetToolbar::CITBandSite::CITBandSite() : CBandSite(NULL)
{
    // HACKHACK: set the initial band ID to something bigger
    //           than the number of toolbars that is in this
    //           object.  Currently those toolbars are not
    //           individual bands, but we want CBandSite to
    //           at least be aware of them.
    //
    _dwBandIDNext = CBANDSMAX;
}

HRESULT CInternetToolbar::CITBandSite::_OnContextMenu(WPARAM wParam, LPARAM lParam)
{
    CInternetToolbar* pitbar = IToClass(CInternetToolbar, _bs, this);
    pitbar->_ShowContextMenu((HWND)wParam, lParam, NULL);
    return S_OK;
}

HRESULT CInternetToolbar::CITBandSite::_Initialize(HWND hwndParent)
{
    _hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
                           RBS_VARHEIGHT | RBS_BANDBORDERS | RBS_REGISTERDROP | RBS_DBLCLKTOGGLE |
                           WS_VISIBLE | WS_BORDER | WS_CHILD | WS_CLIPCHILDREN |
                           WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NOPARENTALIGN,
                           0, 0, 100, 36, hwndParent, (HMENU) FCIDM_REBAR, HINST_THISDLL, NULL);

    if (_hwnd)
    {
        SendMessage(_hwnd, RB_SETTEXTCOLOR, 0, CLR_DEFAULT);
        SendMessage(_hwnd, RB_SETBKCOLOR, 0, CLR_DEFAULT);
        SendMessage(_hwnd, CCM_SETVERSION, COMCTL32_VERSION, 0);
    }

    return CBandSite::_Initialize(hwndParent);
}


HRESULT CInternetToolbar::CITBandSite::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (!pguidCmdGroup) {
        /*NOTHING*/
    } else if (IsEqualGUID(CGID_PrivCITCommands, *pguidCmdGroup)) {

        CInternetToolbar* pitbar = IToClass(CInternetToolbar, _bs, this);
        return pitbar->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
    }
    else if (IsEqualGUID(CGID_Theater, *pguidCmdGroup))
    {
        CInternetToolbar* pitbar = IToClass(CInternetToolbar, _bs, this);
        return IUnknown_Exec(pitbar->_ptbsite, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
    }
    return CBandSite::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}

HRESULT CInternetToolbar::CITBandSite::AddBand(IUnknown *punk)
{
    HRESULT hres = CBandSite::AddBand(punk);
    if (SUCCEEDED(hres)) {
        CInternetToolbar* pitbar = IToClass(CInternetToolbar, _bs, this);
        pitbar->_SetBackground();
    }
    return hres;
}

HRESULT CInternetToolbar::CITBandSite::HasFocusIO()
{
    HRESULT hres = CBandSite::HasFocusIO();
    if (hres == S_FALSE) {
        CInternetToolbar* pitbar = IToClass(CInternetToolbar, _bs, this);
        if (pitbar->_btb._hwnd == GetFocus())
            hres = S_OK;

    }
    return hres;
}

// This will remove all the buttons except the first 2
BOOL TOOLSBANDCLASS::_RemoveAllButtons()
{
    INT_PTR nCount = SendMessage(_hwnd, TB_BUTTONCOUNT, 0, 0L);

    if (!nCount)
        return FALSE;

    while (nCount-- > 0)
    {
        SendMessage(_hwnd, TB_DELETEBUTTON, nCount, 0L);
    }

    return S_OK;
}


HRESULT TOOLSBANDCLASS::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
     CInternetToolbar* pitbar = IToClass(CInternetToolbar, _btb, this);
    if (!pguidCmdGroup) {
        /*NOTHING*/
#ifdef DEBUG
    } else if (IsEqualGUID(*pguidCmdGroup, IID_IExplorerToolbar)) {
        switch(nCmdID) {
        case ETCMDID_GETBUTTONS:
            // if this rips call tjgreen
            ASSERT(0);
            return E_FAIL;
        }
#endif
    } else if (_IsDocHostGUID(pguidCmdGroup)) {
        UEMFireEvent(&UEMIID_BROWSER, UEME_UITOOLBAR, UEMF_XEVENT, UIG_INET, nCmdID);

        if (nCmdexecopt == OLECMDEXECOPT_PROMPTUSER) {
            // the user hit the drop down
            if (pitbar->_ptbsitect && pvarargIn && pvarargIn->vt == VT_INT_PTR)
            {
                // v.vt = VT_I4;
                POINT pt;
                RECT* prc = (RECT*)pvarargIn->byref;
                pt.x = prc->left;
                pt.y = prc->bottom;

                switch (nCmdID)
                {
#ifdef EDIT_HACK
                case DVIDM_EDITPAGE:
                    {
                        // Show the edit pop-up
                        BSTR bstrURL;
                        pitbar->_pdie->get_LocationURL(&bstrURL);
                        if (bstrURL)
                        {
                            USES_CONVERSION;
                            pitbar->_aEditVerb.ShowEditMenu(pt,  pitbar->_hwnd, W2T(bstrURL));
                            SysFreeString(bstrURL);
                        }
                        break;
                    }
#endif

                default:
                    // if this rips find tjgreen
                    ASSERT(0);
                    break;
                }
            }
            return S_OK;
        }

        switch(nCmdID) {

        case DVIDM_EDITPAGE:
        {
            BSTR bstrURL;
            ULONG fMask = 0;
            TCHAR szCacheFileName[MAX_PATH + MAX_URL_STRING + 2];
            memset(szCacheFileName, 0, SIZEOF(szCacheFileName));

            pitbar->_pdie->get_LocationURL(&bstrURL);
            if (NULL == bstrURL)
                break;

            USES_CONVERSION;
            LPCTSTR szURL = W2T(bstrURL);

#ifdef EDIT_HACK
            // Use the default edit verb
            pitbar->_aEditVerb.Edit(szURL);
#else
            SHELLEXECUTEINFO sei = {0};
            LPTSTR pszTemp = PathFindExtension(szURL);

            // If we did not find an extension, we assume it is an .htm or .html
            if (*pszTemp != TEXT('.'))
            {
                if (SHVerbExists(TEXT(".htm"), TEXT("edit"), NULL))
                    sei.lpClass = TEXT(".htm");
                else
                    sei.lpClass = TEXT(".html");
            }
            else
            {
                sei.lpClass = pszTemp;
            }

            if (PathIsURL(szURL))
            {
                // (reinerf)
                // Some apps (FrontPad, Office99, etc) want the URL passed to
                // them instead of the cache filename. We therefore create a string
                // that has the URL name after the null:
                //
                //  "CacheFileName/0UrlName"
                //
                // and pass it as the lpFile parameter to shellexecute.
                // We also pass SEE_MASK_FILEANDURL, so shellexecute can
                // recognize this case.
                int iLength;

                URLDownloadToCacheFile(NULL, szURL, szCacheFileName, 0, URLOSTRM_USECACHEDCOPY, NULL);
                iLength = lstrlen(szCacheFileName);

                // copy in the URL name
                StrCpy(&szCacheFileName[iLength + 1], szURL);

                // add the mask so shellexecute knows to check for the URL, if necessary.
                fMask |= SEE_MASK_FILEANDURL;
            }
            else
            {
                StrCpy(szCacheFileName, szURL);
            }

            sei.cbSize = sizeof(SHELLEXECUTEINFO);
            sei.fMask = fMask;
            sei.hwnd = NULL;
            sei.lpVerb = TEXT("edit");
            sei.lpFile = szCacheFileName;
            sei.lpParameters = NULL;
            sei.lpDirectory = NULL;
            sei.nShow = SW_SHOWNORMAL;
            sei.hInstApp = NULL;

            ShellExecuteEx(&sei);

            SysFreeString(bstrURL);
#endif //!EDIT_HACK
        }
        break;

        default:
            // if this rips call tjgreen
            ASSERT(0);
            break;
        }
    }

    return S_OK;
}

// *** IInputObject methods ***
HRESULT TOOLSBANDCLASS::TranslateAcceleratorIO(LPMSG lpMsg)
{
    if (SendMessage(_hwnd, TB_TRANSLATEACCELERATOR, 0, (LPARAM)lpMsg))
        return S_OK;

    return CToolBand::TranslateAcceleratorIO(lpMsg);
}

// *** IUnknown methods ***
HRESULT TOOLSBANDCLASS::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(TOOLSBANDCLASS, IWinEventHandler),
        { 0 },
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hres))
        hres = CToolBand::QueryInterface(riid, ppvObj);

    return hres;
}

// *** IDeskBand methods ***
HRESULT TOOLSBANDCLASS::GetBandInfo(DWORD dwBandID, DWORD fViewMode, DESKBANDINFO* pdbi)
{

    _dwBandID = dwBandID;

    // set dwModeFlags
    pdbi->dwModeFlags = DBIMF_FIXEDBMP | DBIMF_USECHEVRON;

    // set ptMinSize
    {
        if (SendMessage(_hwnd, TB_BUTTONCOUNT, 0, 0)) {
            // make our min size just big enough to show the first button
            RECT rc;
            SendMessage(_hwnd, TB_GETITEMRECT, 0, (LPARAM)&rc);
            pdbi->ptMinSize.x = RECTWIDTH(rc);
            pdbi->ptMinSize.y = RECTHEIGHT(rc);
        } else {
            // we don't have any buttons; so use standard button size
            LONG lButtonSize = (long) SendMessage(_hwnd, TB_GETBUTTONSIZE, 0, 0);
            pdbi->ptMinSize.x = LOWORD(lButtonSize);
            pdbi->ptMinSize.y = HIWORD(lButtonSize);
        }

        CInternetToolbar* pitbar = IToClass(CInternetToolbar, _btb, this);
        if (pitbar->_fTheater && (pdbi->ptMinSize.y < (THEATER_CYTOOLBAR - 1)))
            pdbi->ptMinSize.y = (THEATER_CYTOOLBAR - 1);
    }

    // set ptActual
    {
        SIZE size;
        size.cy = pdbi->ptMinSize.y;
        SendMessage(_hwnd, TB_GETIDEALSIZE, FALSE, (LPARAM)&size);
        pdbi->ptActual.x = size.cx;
        pdbi->ptActual.y = size.cy;
    }

    // no title
    pdbi->dwMask &= ~DBIM_TITLE;

    return S_OK;
}

IOleCommandTarget* TOOLSBANDCLASS::_CommandTargetFromCmdMap(CMDMAP* pcm)
{
    IOleCommandTarget* pct = NULL;

    if (pcm) {
        if (IsEqualGUID(pcm->guidButtonGroup, CLSID_CommonButtons)) {
            CInternetToolbar* pitbar = IToClass(CInternetToolbar, _btb, this);
            pct = SAFECAST(pitbar, IOleCommandTarget*);
        } else {
            // If either of these rip, the button is stale
            ASSERT(IsEqualGUID(pcm->guidButtonGroup, _guidCurrentButtonGroup));
            ASSERT(_pctCurrentButtonGroup);

            pct = _pctCurrentButtonGroup;
        }
    }

    return pct;
}

LRESULT TOOLSBANDCLASS::_OnToolbarDropDown(TBNOTIFY *ptbn)
{
    UINT uiCmd;
    if (ptbn->hdr.hwndFrom == _hwnd)
    {
        GUID guid;
        _ConvertCmd(NULL, ptbn->iItem, &guid, &uiCmd);
        CMDMAP* pcm = _GetCmdMapByID(ptbn->iItem);
        IOleCommandTarget* pct = _CommandTargetFromCmdMap(pcm);

        if (pct) {
            VARIANTARG var;
            var.vt = VT_I4;
            var.lVal = ptbn->iItem;

            // BUGBUG: use VARIANT[TO/FROM]BUFFER here to fix win64 problem

            VARIANT v = {VT_INT_PTR};
            v.byref = &ptbn->rcButton;

            MapWindowRect(_hwnd, HWND_DESKTOP, &ptbn->rcButton);

            //
            // If this window is mirrored, then let's take the
            // other coordinate [samera]
            //
            if (IS_WINDOW_RTL_MIRRORED(_hwnd))
            {
                int iTmp = ptbn->rcButton.right;
                ptbn->rcButton.right = ptbn->rcButton.left;
                ptbn->rcButton.left  = iTmp;
            }

#ifdef EDIT_HACK
            // BUGBUG: temp code -- edit code moving to dochost.cpp
            if (_IsDocHostGUID(&pcm->guidButtonGroup) && pcm->nCmdID == DVIDM_EDITPAGE)
                Exec(&pcm->guidButtonGroup, (DWORD)pcm->nCmdID, OLECMDEXECOPT_PROMPTUSER, &v, &var);
            else
#endif
                pct->Exec(&pcm->guidButtonGroup, (DWORD)pcm->nCmdID, OLECMDEXECOPT_PROMPTUSER, &v, &var);
        }
    }

    return TBDDRET_DEFAULT;
}

LRESULT TOOLSBANDCLASS::_TryShowBackForwardMenu(DWORD dwItemSpec, LPPOINT ppt, LPRECT prcExclude)
{
    LRESULT lres = 0;

    GUID guid;
    UINT id;
    _ConvertCmd(NULL, dwItemSpec, &guid, &id);

    // If the user right clicked on the the back or forward button, show the context menu
    // On all other buttons show the regular shortcut menu
    if (IsEqualGUID(guid, CLSID_CommonButtons))
    {
        CInternetToolbar* pitbar = IToClass(CInternetToolbar, _btb, this);
        if (id == TBIDM_BACK) {
            pitbar->_ShowBackForwardMenu(FALSE, *ppt, prcExclude);
            lres = 1;
        } else if (id == TBIDM_FORWARD) {
            pitbar->_ShowBackForwardMenu(TRUE, *ppt, prcExclude);
            lres = 1;
        }
    }
    return lres;
}

LRESULT TOOLSBANDCLASS::_OnNotify(LPNMHDR pnmh)
{
    LRESULT lres = 0;

    ASSERT(pnmh->idFrom == FCIDM_TOOLBAR);

    switch (pnmh->code)
    {

    case NM_RCLICK:
        {
            NMCLICK * pnm = (LPNMCLICK)pnmh;

            if (!pnm)
                break;

            // Convert to Screen coordinates
            MapWindowPoints(pnmh->hwndFrom, HWND_DESKTOP, &pnm->pt, 1);

            if (pnmh->hwndFrom == _hwnd)
                lres = _TryShowBackForwardMenu((DWORD)pnm->dwItemSpec, &pnm->pt, NULL);
        }
        break;

    case TBN_DROPDOWN:
        lres = _OnToolbarDropDown((TBNOTIFY *)pnmh);
        break;

    case TBN_DELETINGBUTTON:
        _OnDeletingButton((TBNOTIFY*)pnmh);
        break;

    case TBN_SAVE:
    case TBN_RESET:
    case TBN_INITCUSTOMIZE:
    case TBN_RESTORE:
    case TBN_BEGINADJUST:
    case TBN_GETBUTTONINFO:
    case TBN_ENDADJUST:
    case TBN_QUERYDELETE:
    case TBN_QUERYINSERT:
    case TBN_TOOLBARCHANGE:
        if (pnmh->hwndFrom == _hwnd)
            lres = _ToolsCustNotify (pnmh);
        break;

    case TBN_GETOBJECT:
        {
            NMOBJECTNOTIFY *pnmon = (NMOBJECTNOTIFY *)pnmh;
            if (IsEqualIID(*pnmon->piid, IID_IDropTarget))
            {
                if (pnmh->hwndFrom == _hwnd)
                {
                    UINT uiCmd;
                    GUID guid;
                    _ConvertCmd(NULL, pnmon->iItem, &guid, &uiCmd);

                    if (IsEqualGUID(guid, CLSID_CommonButtons) &&
                            (uiCmd == TBIDM_HOME || uiCmd == TBIDM_FAVORITES)) {

                        CITBarDropTarget *pdtgt = new CITBarDropTarget(_hwnd, uiCmd);
                        if (pdtgt)
                        {
                            pnmon->pObject = SAFECAST(pdtgt, IDropTarget*);
                            pnmon->hResult = NOERROR;
                        }
                    }
                    else     // pass back CDropDummy to handle basics.
                    {
                        CDropDummy *pdtgt = new CDropDummy(_hwnd);
                        if (pdtgt)
                        {
                            pnmon->pObject = SAFECAST(pdtgt, IDropTarget*);
                            pnmon->hResult = NOERROR;
                        }
                    }

                }
                lres = TRUE;
            }
        }
        break;

    default:
        lres = CToolbarBand::_OnNotify(pnmh);
        break;
    }

    return lres;
}

LRESULT TOOLSBANDCLASS::_OnContextMenu(LPARAM lParam, WPARAM wParam)
{
    LRESULT lres = 0;

    if (IS_WM_CONTEXTMENU_KEYBOARD(lParam)) {

        // keyboard context menu.  figure out where to pop up menu and
        // which context menu to use, and tell itbar to pop it up.
        RECT rc;
        BOOL fBackForward = FALSE;

        // figure out coordinates to use
        INT_PTR iBtn = SendMessage(_hwnd, TB_GETHOTITEM, 0, 0);
        if (iBtn != -1) {
            // use lower left corner of current hot button
            SendMessage(_hwnd, TB_GETITEMRECT, iBtn, (LPARAM)&rc);
        } else {
            // no hot button; use top left corner of tools window
            SetRect(&rc, 0, 0, 0, 0);
        }
        MapWindowPoints(_hwnd, HWND_DESKTOP, (LPPOINT)&rc, 2);

        if (iBtn != -1) {
            // get hot button's command
            TBBUTTONINFO tbbi;
            tbbi.cbSize = SIZEOF(TBBUTTONINFO);
            tbbi.dwMask = TBIF_BYINDEX | TBIF_COMMAND;
            SendMessage(_hwnd, TB_GETBUTTONINFO, iBtn, (LPARAM)&tbbi);

            POINT pt = {rc.left, rc.bottom};

            // try popping up the back/forward context menu
            if (_TryShowBackForwardMenu(tbbi.idCommand, &pt, &rc))
                fBackForward = TRUE;
        }

        if (!fBackForward) {
            // pop up the standard context menu
            CInternetToolbar* pitbar = IToClass(CInternetToolbar, _btb, this);
            pitbar->_ShowContextMenu((HWND)wParam, MAKELONG(rc.left, rc.bottom), (iBtn == -1 ? NULL : &rc));
        }

        lres = 1;
    }
    return lres;
}

void TOOLSBANDCLASS::_RecalcButtonWidths()
{
    // We need the toolbars buttons to use ONLY exactly as much space as is needed.
    // By setting the size a a really small number like 10, and then setting it to
    // the real number we can accomplish this.
    // If we don't use do this, then when we add new buttons after doing this
    // RemoveAllButtons(), the new buttons will be at least as wide as the widest
    // button that existed on the last set of buttons (the ones we are just removing)
    CInternetToolbar* pitbar = IToClass(CInternetToolbar, _btb, this);
    SendMessage(_hwnd, TB_SETBUTTONWIDTH, 0, (LPARAM)MAKELONG(0, 10));
    SendMessage(_hwnd, TB_SETBUTTONWIDTH, 0, (LPARAM)(pitbar->_fCompressed ? MAKELONG(0, MAX_TB_COMPRESSED_WIDTH) : MAKELONG(0, pitbar->_uiMaxTBWidth)));
}

// *** IWinEventHandler methods ***
HRESULT TOOLSBANDCLASS::OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    HRESULT hres = S_OK;

    switch (dwMsg) {
    case WM_CONTEXTMENU:
        *plres = _OnContextMenu(lParam, wParam);
        break;

    case WM_NOTIFY:
        *plres = _OnNotify((LPNMHDR)lParam);
        break;

    case WM_WININICHANGE:
        *plres = SendMessage(_hwnd, dwMsg, wParam, lParam);
        if (wParam == SPI_SETNONCLIENTMETRICS)
        {
            _RecalcButtonWidths();
            _BandInfoChanged();
        }
        break;

    default:
        hres = CToolbarBand::OnWinEvent(hwnd, dwMsg, wParam, lParam, plres);
        break;
    }

    return hres;
}

CMDMAP* TOOLSBANDCLASS::_GetCmdMap(int i, BOOL fByIndex)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = SIZEOF(tbbi);
    tbbi.dwMask = TBIF_LPARAM;
    tbbi.lParam = 0;
    if (fByIndex)
        tbbi.dwMask |= TBIF_BYINDEX;
    SendMessage(_hwnd, TB_GETBUTTONINFO, i, (LPARAM)&tbbi);
    return (CMDMAP*)(LPVOID)tbbi.lParam;
}


void TOOLSBANDCLASS::_FreeCmdMap(CMDMAP* pcm)
{
    if (pcm)
        LocalFree(pcm);
}

void TOOLSBANDCLASS::_OnDeletingButton(TBNOTIFY* ptbn)
{
    CMDMAP *pcm = (CMDMAP*)(LPVOID)ptbn->tbButton.dwData;
    _FreeCmdMap(pcm);
}

LONG_PTR TOOLSBANDCLASS::_AddString(LPWSTR pwstr)
{
    LONG_PTR lOffset;
    CInternetToolbar* pitbar = IToClass(CInternetToolbar, _btb, this);
    pitbar->AddString(&_guidCurrentButtonGroup, 0, (UINT_PTR)pwstr, &lOffset);

    return lOffset;
}

#define PPBS_LOOKINTOOLBAR  0x00000001
#define PPBS_EXTERNALBUTTON 0x00000002

void TOOLSBANDCLASS::_PreProcessButtonString(TBBUTTON *ptbn, DWORD dwFlags)
{
    // Caller should have checked this.
    ASSERT(!(ptbn->fsStyle & BTNS_SEP));

    // If we don't have a command target, we shouldn't have any external buttons.
    ASSERT(_pctCurrentButtonGroup || !(dwFlags & PPBS_EXTERNALBUTTON));

    if (ptbn->iString < 0 && ptbn->iBitmap <= MAX_SHELLGLYPHINDEX)
    {
        // total hack
        // we're hard coding the strings in to match
        // the bitmap.  so if anyone uses the shell bitmaps,
        // they're going to get our text labels
        // also hacking in that the bitmap array and string array are
        // matched
        // who designed reviewed this???

        ptbn->iString = ptbn->iBitmap;
    }
    else if (!ptbn->iString && (dwFlags & PPBS_EXTERNALBUTTON))
    {
        // Some Extensions are giving us bogus string ids (Font ext sends 0)
        ptbn->iString = -1;
    }
    else if (ptbn->iString != -1 && !IS_INTRESOURCE(ptbn->iString))
    {
        // It's a string pointer.  The customization mechanism requires that all buttons
        // use strings from the tb string pool.  So add the string to the pool and set
        // iString to the pool index.
        ptbn->iString = _AddString((LPWSTR)ptbn->iString);
    }

    if (ptbn->iString == -1 && IsFlagSet(dwFlags, PPBS_LOOKINTOOLBAR | PPBS_EXTERNALBUTTON))
    {
        // If we're building the customization dsa rather than adding new buttons to the
        // toolbar, we may already have this button in the toolbar.  If so, use that string.

        UINT idCommand;
        if (SUCCEEDED(_ConvertCmd(&_guidCurrentButtonGroup, ptbn->idCommand, NULL, &idCommand)))
        {
            TBBUTTON tbb;
            if (SendMessage(_hwnd, TB_GETBUTTON, idCommand, (LPARAM)&tbb))
                ptbn->iString = tbb.iString;
        }
    }

    if (ptbn->iString == -1 && (dwFlags & PPBS_EXTERNALBUTTON))
    {
        // Still don't have a string for this puppy.  Last resort is to ask via QueryStatus.
        OLECMDTEXTV<MAX_TOOLTIP_STRING> cmdtv;
        OLECMDTEXT *pcmdText = &cmdtv;

        pcmdText->cwBuf = MAX_TOOLTIP_STRING;
        pcmdText->cmdtextf = OLECMDTEXTF_NAME;
        pcmdText->cwActual = 0;

        OLECMD rgcmd = {ptbn->idCommand, 0};

        HRESULT hr = _pctCurrentButtonGroup->QueryStatus(&_guidCurrentButtonGroup, 1, &rgcmd, pcmdText);
        if (SUCCEEDED(hr) && (pcmdText->cwActual))
            ptbn->iString = _AddString(pcmdText->rgwz);
    }

    // If it's an internal button, we'd better have found a string for it.
    ASSERT(ptbn->iString != -1 || (dwFlags & PPBS_EXTERNALBUTTON));
}

void TOOLSBANDCLASS::_PreProcessExternalTBButton(TBBUTTON *ptbn)
{
    if (!(ptbn->fsStyle & BTNS_SEP)) {
        CMDMAP* pcm = (CMDMAP*)LocalAlloc(LPTR, SIZEOF(CMDMAP));
        if (pcm) {
            pcm->guidButtonGroup = _guidCurrentButtonGroup;
            pcm->nCmdID = ptbn->idCommand;

            _PreProcessButtonString(ptbn, PPBS_EXTERNALBUTTON);

            _nNextCommandID++;
            pcm->lParam = ptbn->dwData;
        }

        ptbn->dwData = (LPARAM)pcm;
    } else {
        ptbn->dwData = 0;

        // override default toolbar width for separators; iBitmap member of
        // TBBUTTON struct is a union of bitmap index & separator width
        ptbn->iBitmap = CX_SEPARATOR;
    }
}

UINT TOOLSBANDCLASS::_ProcessExternalButtons(PTBBUTTON ptbb, UINT cButtons)
{
    cButtons = RemoveHiddenButtons(ptbb, cButtons);

    for (UINT i = 0; i < cButtons; i++)
        _PreProcessExternalTBButton(&ptbb[i]);

    return cButtons;
}

void TOOLSBANDCLASS::_GetButtons(IOleCommandTarget* pct, const GUID* pguid, HDSA hdsa)
{
    LONG lCount;
    VARIANTARG v1;
    VariantInit(&v1);
    v1.vt = VT_BYREF | VT_I4;
    v1.plVal = &lCount;

    VARIANTARG v2;
    VariantInit(&v2);
    if (SUCCEEDED(pct->Exec(&IID_IExplorerToolbar, ETCMDID_GETBUTTONS, 0, &v1, &v2)) && v2.vt == VT_BYREF) {
        CMDMAPCUSTOMIZE cmc;
        TBBUTTON* pbtn = (TBBUTTON*)v2.byref;

        cmc.cm.guidButtonGroup = *pguid;

        DWORD dwFlags = PPBS_LOOKINTOOLBAR;

        if (!IsEqualGUID(*pguid, CLSID_CommonButtons))
            dwFlags |= PPBS_EXTERNALBUTTON;

        for (long l = 0; l < lCount; l++) {

            cmc.btn = pbtn[l];
            if (!(cmc.btn.fsStyle & BTNS_SEP)) {
                cmc.cm.nCmdID = pbtn[l].idCommand;

                _PreProcessButtonString(&cmc.btn, dwFlags);

                if (FAILED(_ConvertCmd(pguid, cmc.cm.nCmdID, NULL, (UINT*)&cmc.btn.idCommand))) {
                    // not already in the toolbar, generate a new id
                    cmc.btn.idCommand = _nNextCommandID++;
                }

                DSA_AppendItem(hdsa, &cmc);
            } else {
                cmc.btn.dwData = 0;
            }
        }
    }
}

void TOOLSBANDCLASS::_OnEndCustomize()
{

    if (_pcinfo) {
        // loop through and make sure that any items added have the appropriate cmdmap
        int i;
        INT_PTR nCount = SendMessage(_hwnd, TB_BUTTONCOUNT, 0, 0L);
        _pcinfo->fAdjust = FALSE;
        for(i = 0; i < nCount; i++) {
            CMDMAP* pcm = _GetCmdMapByIndex(i);
            if (!pcm) {
                // no command map for this item
                // find the corresponding CMDMAP in our hdsa, clone it and give it to this button.

                // the command id's are the same, so get the toolbar command id, find the corresponding
                // one in the hdsa and clone away.
                TBBUTTONINFO tbbi;
                tbbi.cbSize = SIZEOF(tbbi);
                tbbi.dwMask = TBIF_COMMAND | TBIF_BYINDEX;
                SendMessage(_hwnd, TB_GETBUTTONINFO, i, (LPARAM)&tbbi);

                int j;
                for (j = 0; j < DSA_GetItemCount(_pcinfo->hdsa); j++) {
                    CMDMAPCUSTOMIZE* pcmc = (CMDMAPCUSTOMIZE*)DSA_GetItemPtr(_pcinfo->hdsa, j);
                    ASSERT(pcmc);
                    if (pcmc->btn.idCommand == tbbi.idCommand) {
                        // found it!

                        // clone the cmdmap
                        CMDMAP *pcm = (CMDMAP*)LocalAlloc(LPTR, SIZEOF(CMDMAP));
                        if (pcm) {
                            *pcm = pcmc->cm;
                            tbbi.lParam = (LPARAM)pcm;
                            tbbi.dwMask = TBIF_LPARAM | TBIF_BYINDEX;
                            SendMessage(_hwnd, TB_SETBUTTONINFO, i, (LPARAM)&tbbi);
                        }
                    }
                }
            }
        }

        if (_pcinfo->fDirty)
            _SaveRestoreToolbar(TRUE);

        _FreeCustomizeInfo();

        _RecalcButtonWidths();
        CInternetToolbar* pitbar = IToClass(CInternetToolbar, _btb, this);

#ifdef EDIT_HACK
        pitbar->_InitEditButtonStyle();
#endif
        if (g_fSmallIcons != _UseSmallIcons())
            SendShellIEBroadcastMessage(WM_WININICHANGE, 0, (LPARAM)SZ_REGKEY_SMALLICONS, 3000);

        pitbar->_UpdateToolbar(TRUE);
    }
}

void TOOLSBANDCLASS::_FreeCustomizeInfo()
{
    if (_pcinfo) {
        DSA_Destroy(_pcinfo->hdsa);
        LocalFree(_pcinfo);
        _pcinfo = NULL;
    }
}

CMDMAPCUSTOMIZE* TOOLSBANDCLASS::_GetCmdMapCustomize(GUID* pguid, UINT nCmdID)
{
    int j;
    for (j = 0; j < DSA_GetItemCount(_pcinfo->hdsa); j++) {
        CMDMAPCUSTOMIZE* pcmc = (CMDMAPCUSTOMIZE*)DSA_GetItemPtr(_pcinfo->hdsa, j);

        if (pcmc->cm.nCmdID == nCmdID &&
            IsEqualGUID(*pguid, pcmc->cm.guidButtonGroup)) {

            return pcmc;
        }
    }

    return NULL;
}

BOOL TOOLSBANDCLASS::_BuildButtonDSA()
{
    CInternetToolbar* pitbar = IToClass(CInternetToolbar, _btb, this);

    ASSERT(!_pcinfo);
    _pcinfo = (CUSTOMIZEINFO*)LocalAlloc(LPTR, SIZEOF(CUSTOMIZEINFO));

    if (_pcinfo) {
        // build a CMDMAP array of all the buttons available
        _pcinfo->hdsa = DSA_Create(SIZEOF(CMDMAPCUSTOMIZE), 4);

        // add the common set (back,forward, stop, refresh, home and search
        _GetButtons(pitbar, &CLSID_CommonButtons, _pcinfo->hdsa);

        _GetButtons(_pctCurrentButtonGroup, &_guidCurrentButtonGroup, _pcinfo->hdsa);

        return TRUE;
    }
    return FALSE;
}

void TOOLSBANDCLASS::_UpdateTextSettings(INT_PTR ids)
{
    CInternetToolbar* pitbar = IToClass(CInternetToolbar, _btb, this);

    BOOL fText, fList;

    switch (ids) {
    case IDS_TEXTLABELS:
        fList = FALSE;
        fText = TRUE;
        break;

    case IDS_PARTIALTEXT:
        fList = TRUE;
        fText = TRUE;
        break;

    case IDS_NOTEXTLABELS:
        fList = FALSE;  // (but we really don't care)
        fText = FALSE;
        break;
    }

    pitbar->_UpdateToolsStyle(fList);

    // (_fCompressed == TRUE means no text labels)
    pitbar->_UpdateToolbarDisplay(UTD_TEXTLABEL, 0, !fText, TRUE);
}

const static DWORD c_aBtnAttrHelpIDs[] = {
    IDC_SHOWTEXT,       IDH_BROWSEUI_TB_TEXTOPTNS,
    IDC_SMALLICONS,     IDH_BROWSEUI_TB_ICONOPTNS,
    0, 0
};

BOOL_PTR CALLBACK TOOLSBANDCLASS::_BtnAttrDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CInternetToolbar* pitbar = (CInternetToolbar*)GetWindowPtr(hDlg, DWLP_USER);

    switch (uMsg) {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);  /* LPADJUSTDLGDATA pointer */
        return TRUE;

    case WM_COMMAND:
        if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_SHOWTEXT)
        {
            if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELENDOK ||
                GET_WM_COMMAND_CMD(wParam, lParam) == CBN_CLOSEUP)
            {
                // what'd they pick?
                HWND hwndText = GET_WM_COMMAND_HWND(wParam, lParam);
                INT_PTR iSel = SendMessage(hwndText, CB_GETCURSEL, 0, 0);
                INT_PTR idsSel = SendMessage(hwndText, CB_GETITEMDATA, iSel, 0);

                pitbar->_btb._UpdateTextSettings(idsSel);

                return TRUE;
            }
        }
        break;

    case WM_CONTEXTMENU:
        SHWinHelpOnDemandWrap((HWND) wParam, c_szHelpFile,
            HELP_CONTEXTMENU, (DWORD_PTR)(LPTSTR) c_aBtnAttrHelpIDs);
        return TRUE;

    case WM_HELP:
        SHWinHelpOnDemandWrap((HWND) ((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile,
            HELP_WM_HELP, (DWORD_PTR)(LPTSTR) c_aBtnAttrHelpIDs);
        return TRUE;

    case WM_DESTROY:
        {
#define SZ_YES  TEXT("yes")
#define SZ_NO   TEXT("no")

            HWND hwndIcons = GetDlgItem(hDlg, IDC_SMALLICONS);
            if (TPTR(hwndIcons)) {

                INT_PTR iSel = SendMessage(hwndIcons, CB_GETCURSEL, 0, 0);
                BOOL fSmallIcons = (SendMessage(hwndIcons, CB_GETITEMDATA, iSel, 0) == IDS_SMALLICONS);

                LPCTSTR szData;
                DWORD cbData;

                if (fSmallIcons) {
                    szData = SZ_YES;
                    cbData = SIZEOF(SZ_YES);
                } else {
                    szData = SZ_NO;
                    cbData = SIZEOF(SZ_NO);
                }
                SHRegSetUSValue(SZ_REGKEY_SMALLICONS, SZ_REGVALUE_SMALLICONS, REG_SZ, (LPVOID)szData, cbData, SHREGSET_FORCE_HKCU);
            }
        }
        return TRUE;
    }

    return FALSE;
}

void TOOLSBANDCLASS::_PopulateComboBox(HWND hwnd, const int iResource[], UINT cResources)
{
    TCHAR sz[256];

    // loop through iResource[], load each string resource and insert into combobox
    for (UINT i = 0; i < cResources; i++) {
        if (MLLoadString(iResource[i], sz, ARRAYSIZE(sz))) {
            INT_PTR iIndex = SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)sz);
            SendMessage(hwnd, CB_SETITEMDATA, iIndex, iResource[i]);
        }
    }
}

void TOOLSBANDCLASS::_SetComboSelection(HWND hwnd, int iCurOption)
{
    INT_PTR cItems = SendMessage(hwnd, CB_GETCOUNT, 0, 0);

    while (cItems--) {

        INT_PTR iItemData = SendMessage(hwnd, CB_GETITEMDATA, cItems, 0);

        if (iItemData == iCurOption) {
            SendMessage(hwnd, CB_SETCURSEL, cItems, 0);
            break;
        }
        else {
            // iCurOption should be in list somewhere;
            // assert that we're not done looking
            ASSERT(cItems);
        }
    }
}

void TOOLSBANDCLASS::_SetDialogSelections(HWND hDlg, BOOL fSmallIcons)
{
    CInternetToolbar* pitbar = IToClass(CInternetToolbar, _btb, this);

    int iCurOption;
    HWND hwnd;

    hwnd = GetDlgItem(hDlg, IDC_SHOWTEXT);

    if (pitbar->_fCompressed)
        iCurOption = IDS_NOTEXTLABELS;
    else if (IS_LIST_STYLE(_hwnd))
        iCurOption = IDS_PARTIALTEXT;
    else
        iCurOption = IDS_TEXTLABELS;

    _SetComboSelection(hwnd, iCurOption);
    if (pitbar->_fTheater)
        SHSetWindowBits(hwnd, GWL_STYLE, WS_DISABLED, WS_DISABLED);

    hwnd = GetDlgItem(hDlg, IDC_SMALLICONS);
    iCurOption = (fSmallIcons ? IDS_SMALLICONS : IDS_LARGEICONS);
    _SetComboSelection(hwnd, iCurOption);
}

static const int c_iTextOptions[] = {
    IDS_TEXTLABELS,
    IDS_PARTIALTEXT,
    IDS_NOTEXTLABELS,
};

static const int c_iIconOptions[] = {
    IDS_SMALLICONS,
    IDS_LARGEICONS,
};

void TOOLSBANDCLASS::_PopulateDialog(HWND hDlg)
{
    HWND hwnd;

    hwnd = GetDlgItem(hDlg, IDC_SHOWTEXT);
    _PopulateComboBox(hwnd, c_iTextOptions, ARRAYSIZE(c_iTextOptions));

    hwnd = GetDlgItem(hDlg, IDC_SMALLICONS);
    _PopulateComboBox(hwnd, c_iIconOptions, ARRAYSIZE(c_iIconOptions));
}

void TOOLSBANDCLASS::_OnBeginCustomize(LPNMTBCUSTOMIZEDLG pnm)
{
    CInternetToolbar* pitbar = IToClass(CInternetToolbar, _btb, this);
    HWND hwnd = (HWND) GetProp(pnm->hDlg, SZ_PROP_CUSTDLG);

    if (!hwnd) {
        //
        // hasn't been initialized.
        //
        // we need to check this because this init will be called
        // when the user hits reset as well

        hwnd = CreateDialogParam(MLGetHinst(), MAKEINTRESOURCE(DLG_TEXTICONOPTIONS), pnm->hDlg, _BtnAttrDlgProc, (LPARAM)pitbar);
        if (hwnd) {
            // store hwnd of our dialog as property on tb cust dialog
            SetProp(pnm->hDlg, SZ_PROP_CUSTDLG, hwnd);

            // populate dialog controls
            _PopulateDialog(hwnd);

            // initialize dialog control selection states
            _SetDialogSelections(hwnd, g_fSmallIcons);

            RECT rc, rcWnd, rcClient;
            GetWindowRect(pnm->hDlg, &rcWnd);
            GetClientRect(pnm->hDlg, &rcClient);
            GetWindowRect(hwnd, &rc);

            // enlarge tb dialog to make room for our dialog
            SetWindowPos(pnm->hDlg, NULL, rcWnd.left, rcWnd.top + 64, RECTWIDTH(rcWnd), RECTHEIGHT(rcWnd) + RECTHEIGHT(rc), SWP_NOZORDER);

            // position our dialog at the bottom of the tb dialog
            SetWindowPos(hwnd, HWND_TOP, rcClient.left, rcClient.bottom, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
        }
    }

    if (_BuildButtonDSA()) {
        _pcinfo->fAdjust = TRUE;
    }
}

class CBitmapPreload : public IRunnableTask
{
    public:
        STDMETHOD ( QueryInterface ) ( REFIID riid, LPVOID * ppvObj );
        STDMETHOD_( ULONG, AddRef ) ();
        STDMETHOD_( ULONG, Release ) ();

        STDMETHOD (Run)( void );
        STDMETHOD (Kill)( BOOL fWait );
        STDMETHOD (Suspend)( );
        STDMETHOD (Resume)( );
        STDMETHOD_( ULONG, IsRunning )( void );

    protected:
        friend HRESULT CBitmapPreload_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

        CBitmapPreload();
        ~CBitmapPreload();

        LONG            m_cRef;
        LONG            m_lState;
};

STDAPI CBitmapPreload_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking and *ppunk zeroing are handled in class factory
    ASSERT(pUnkOuter == NULL);

    CBitmapPreload* pbp = new CBitmapPreload();

    if (pbp)
    {
        *ppunk = SAFECAST(pbp, IRunnableTask*);
        return S_OK;
    }
    else
    {
        *ppunk = NULL; // redundant but doesn't hurt
        return E_OUTOFMEMORY;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
CBitmapPreload::CBitmapPreload() : m_cRef(1)
{
    m_lState = IRTIR_TASK_NOT_RUNNING;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
CBitmapPreload::~CBitmapPreload()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBitmapPreload::QueryInterface ( REFIID riid, LPVOID * ppvObj )
{
    *ppvObj = NULL;

    if ( riid == IID_IRunnableTask || riid == IID_IUnknown )
    {
        *ppvObj = SAFECAST( this, IRunnableTask *);
        AddRef();
    }
    else
    {
        return E_NOINTERFACE;
    }

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CBitmapPreload:: AddRef ()
{
    InterlockedIncrement( &m_cRef );
    return m_cRef;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CBitmapPreload:: Release ()
{
    if (InterlockedDecrement( &m_cRef ) == 0 )
    {
        delete this;
        return 0;
    }

    return m_cRef;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBitmapPreload::Run ( void )
{
    if ( m_lState != IRTIR_TASK_NOT_RUNNING )
    {
        return E_FAIL;
    }

    InterlockedExchange( &m_lState, IRTIR_TASK_RUNNING );

    CInternetToolbar_Preload( );

    InterlockedExchange( &m_lState, IRTIR_TASK_FINISHED );

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBitmapPreload::Kill ( BOOL fWait )
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBitmapPreload::Suspend ( )
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBitmapPreload::Resume ( )
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CBitmapPreload:: IsRunning ( void )
{
    return m_lState;
}



#ifdef EDIT_HACK

//+-------------------------------------------------------------------------
//  Constructor
//--------------------------------------------------------------------------
CInternetToolbar::CEditVerb::CEditVerb()
{
    ASSERT(_nElements == 0);
    ASSERT(_nDefault == 0);
    ASSERT(_pVerb == NULL);
    ASSERT(_lpfnOldWndProc == NULL);
    ASSERT(_pszDefaultEditor == NULL);
    ASSERT(_fInitEditor == FALSE);
}

//+-------------------------------------------------------------------------
//  Destructor
//--------------------------------------------------------------------------
CInternetToolbar::CEditVerb::~CEditVerb()
{
    if (_pVerb) RemoveAll();
    SetStr(&_pszDefaultEditor, NULL);
}

//+-------------------------------------------------------------------------
// Removes all cached edit verbs and associated memory
//--------------------------------------------------------------------------
void CInternetToolbar::CEditVerb::RemoveAll()
{
    if (_nElements > 0)
    {
        for (UINT i=0; i < _nElements; ++i)
        {
            EDITVERB& rVerb = _pVerb[i];

            SetStr(&rVerb.pszDesc, NULL);
            SetStr(&rVerb.pszMenuText, NULL);
            SetStr(&rVerb.pszExe, NULL);
            if (rVerb.hkeyProgID)
            {
                RegCloseKey(rVerb.hkeyProgID);
            }
            _ClearMSAAMenuInfo(rVerb);
        }

        LocalFree(_pVerb);

        _pVerb = NULL;
        _nElements = 0;
        _nDefault = 0;
    }
}

//+-------------------------------------------------------------------------
// Check registry for the friendly name of the default html editor.  This
// editor is configured by inetcpl or by office 2000.  If necessary, the
// associated verb is moved to the OpenWithList for .htm files.
//--------------------------------------------------------------------------
void CInternetToolbar::CEditVerb::InitDefaultEditor(HKEY hkey)
{
    //
    // First see if the default editor is in HKCU
    //
    HKEY hkeyEdit = hkey;
    if (hkey ||
        ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_DEFAULT_HTML_EDITOR,
                                      0, KEY_READ | KEY_WRITE, &hkeyEdit))
    {
        //
        // See if we have a default editor selected
        //
        WCHAR szBuf[MAX_PATH];
        DWORD cbBuf = sizeof(szBuf);
        if (ERROR_SUCCESS == SHGetValue(hkeyEdit, NULL, L"Description", NULL, szBuf, &cbBuf))
        {
            // We got it!  Save the friendly name.
            PathRemoveBlanks(szBuf);
            SetStr(&_pszDefaultEditor, szBuf);
        }
        else
        {
            IQueryAssociations *pqa;

            if (SUCCEEDED(AssocCreate(CLSID_QueryAssociations, IID_IQueryAssociations, (LPVOID *)&pqa)))
            {
                //  need to release
                if (SUCCEEDED(pqa->Init(0, NULL, hkeyEdit, NULL)) &&
                ( SUCCEEDED(pqa->GetString(ASSOCF_VERIFY, ASSOCSTR_FRIENDLYAPPNAME, L"edit", szBuf, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(szBuf))))
                || SUCCEEDED(pqa->GetString(ASSOCF_VERIFY, ASSOCSTR_FRIENDLYAPPNAME, NULL, szBuf, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(szBuf))))))
                {
                    PathRemoveBlanks(szBuf);

#ifdef COPY_DEFAULT_TO_OPENWITHLIST
                    //
                    // Add this verb to our OpenWithList if it isn't already there.  We add using
                    // the following format:
                    //
                    // .htm\OpenWithList\<friendly name>\shell\<verb>\command
                    //
                    HKEY hkeyDest;
                    WCHAR szKey[MAX_PATH];
                    wnsprintf(szKey, SIZECHARS(szKey), L".htm\\OpenWithList\\%s", szBuf);

                    if (ERROR_SUCCESS != RegOpenKey(HKEY_CLASSES_ROOT, szKey, NULL) &&

                        // Doesn't exist so create it!
                        ERROR_SUCCESS == RegCreateKey(HKEY_CLASSES_ROOT, szKey, &hkeyDest))
                    {
                        // Copy the verb to the OpenWithList
                        SHCopyKey(hkeyEdit, NULL, hkeyDest, 0);
                        RegCloseKey(hkeyDest);
                    }
/*
                    // Copy to the OpenWithList for .html too
                    wnsprintf(szKey, SIZECHARS(szKey), L"%s\\OpenWithList\\%s", L".html", szBuf);

                    if (ERROR_SUCCESS != RegOpenKey(HKEY_CLASSES_ROOT, szKey, NULL) &&

                        // Doesn't exist so create it!
                        ERROR_SUCCESS == RegCreateKey(HKEY_CLASSES_ROOT, szKey, &hkeyDest))
                    {
                        // Copy the verb to the OpenWithList
                        SHCopyKey(hkeyEdit, NULL, hkeyDest, 0);
                        RegCloseKey(hkeyDest);
                    }
*/
#endif // COPY_DEFAULT_TO_OPENWITHLIST

                    // Save the name of the default editor
                    SetStr(&_pszDefaultEditor, szBuf);

                    SHSetValue(hkeyEdit, NULL, L"Description", REG_SZ, szBuf, CbFromCch(lstrlen(szBuf) +1));
                }

                pqa->Release();
            }
        }

        // Close the key if it wasn't passed in
        if (hkeyEdit && NULL == hkey)
        {
            RegCloseKey(hkeyEdit);
        }
    }

    // During setup, Office places the orginial edit verb in HKLM. We need to copy this to HKCU.
    else if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_DEFAULT_HTML_EDITOR, 0, KEY_READ, &hkeyEdit))
    {
        // Migrate this key into HKCU
        HKEY hkeyDest;
        if (ERROR_SUCCESS == RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_DEFAULT_HTML_EDITOR, &hkeyDest))
        {
            SHCopyKey(hkeyEdit, NULL, hkeyDest, 0);

            // Try again
            InitDefaultEditor(hkeyDest);
            RegCloseKey(hkeyDest);
        }
        RegCloseKey(hkeyEdit);
    }
}

//+-------------------------------------------------------------------------
// Make sure that notepad is registered in the OpenWithList for .htm files.
// This is called when this dll is registered (at setup time)
//--------------------------------------------------------------------------
void AddNotepadToOpenWithList()
{
    TCHAR szPath[MAX_PATH];
    GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
    PathAddBackslash(szPath);
    StrCatBuff(szPath, L"Notepad.exe", ARRAYSIZE(szPath));
    TCHAR szNotepadFriendly[MAX_PATH];
    HKEY hkeyOpenWith;

    if (SUCCEEDED(AssocQueryString(ASSOCF_INIT_BYEXENAME | ASSOCF_VERIFY,
        ASSOCSTR_FRIENDLYAPPNAME, szPath, NULL, szNotepadFriendly, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(szNotepadFriendly)))))
    {
        // If not already in the OpenWithList, add it
        WCHAR szKey[MAX_PATH];
        wnsprintf(szKey, SIZECHARS(szKey), L".htm\\OpenWithList\\%s", szNotepadFriendly);

        if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, KEY_READ, &hkeyOpenWith))
        {
            // Compose command for the verb "c:\windows\notepad.exe %1"
            StrCatBuff(szPath, L" %1", ARRAYSIZE(szPath));

            // The verb goes under the friendly name
            StrCatBuff(szKey, L"\\shell\\edit\\command", ARRAYSIZE(szKey));
            SHSetValue(HKEY_CLASSES_ROOT, szKey, NULL, REG_SZ, szPath, CbFromCch(lstrlen(szPath) + 1));
        }
        else
        {
            RegCloseKey(hkeyOpenWith);
        }
    }
}

//+-------------------------------------------------------------------------
// Returns the friendly name of the default HTML editor
//--------------------------------------------------------------------------
LPCTSTR CInternetToolbar::CEditVerb::_GetDefaultEditor()
{
    // Do a lazy init of the default editor
    if (!_fInitEditor)
    {
        InitDefaultEditor();
        _fInitEditor = TRUE;
    }
    return _pszDefaultEditor;
}

//+-------------------------------------------------------------------------
// Gets the path of the exe associated with the verb and stores the
// result in rVerb.  The caller is responsible for freeing the string
// returned.
//--------------------------------------------------------------------------
LPCTSTR CInternetToolbar::CEditVerb::_GetExePath(EDITVERB& rVerb)
{
    // If we already have the path, simply return it
    if (NULL == rVerb.pszExe)
    {
        ASSERT(rVerb.hkeyProgID);
        TCHAR sz[MAX_PATH];
        if (SUCCEEDED(AssocQueryStringByKey(ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, rVerb.hkeyProgID,
            rVerb.fUseOpenVerb ? NULL : L"edit", sz, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(sz)))))
            rVerb.pszExe = StrDup(sz);
    }

    return rVerb.pszExe;
}

//+-------------------------------------------------------------------------
// Returns TRUE if path of the exe associated with the verb is not found in
// any of the existing verbs.
//--------------------------------------------------------------------------
BOOL CInternetToolbar::CEditVerb::_IsUnique(EDITVERB& rNewVerb)
{
    // Get the friendly name of the new element
    LPCTSTR pszNewDesc = _GetDescription(rNewVerb);
    if (NULL == pszNewDesc)
    {
        // Executable must not exist
        return FALSE;
    }

    // Scan existing elements for the same executable
    for (UINT i=0; i < _nElements; ++i)
    {
        LPCTSTR pszDesc = _GetDescription(_pVerb[i]);
        if (pszDesc && (StrCmpI(pszNewDesc, pszDesc) == 0))
        {
            // Match found, so free the friendly name for the new verb
            SetStr(&rNewVerb.pszDesc, NULL);

            // If the new item shows its icon on the button, make the duplicate
            // do the same.
            if (rNewVerb.fShowIcon)
            {
                _pVerb[i].fShowIcon = TRUE;
                _nDefault = i;
            }
            return FALSE;
        }
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
// Some programs such as msothmed.exe act as stubs that redirect the edit
// command to the appropriate executable. This function returns true if
// the path contains the name of a known stub.
//--------------------------------------------------------------------------
BOOL CInternetToolbar::CEditVerb::_IsHtmlStub
(
    LPCWSTR pszPath
)
{
    BOOL fRet = FALSE;

    // Get the MULTISZ list of known redirectors
    TCHAR szRedir[MAX_PATH];
    ZeroInit(szRedir, ARRAYSIZE(szRedir)); // Protect against non-multisz strings in the reg
    DWORD dwType;
    DWORD cb = sizeof(szRedir) - 4;
    if (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_DEFAULT_HTML_EDITOR, L"Stubs", &dwType, szRedir, &cb))
    {
        // Nothing in registry, so default to ignore the Office redirector
        StrCpyN(szRedir, L"msohtmed.exe\0", ARRAYSIZE(szRedir));
    }

    // See if the path contains the name of a redirectors
    // Note that PathFindFileName doesn't work well for pathes with parameters so we just
    // check for the exe name in the path)
    for (LPTSTR p = szRedir; *p != NULL; p += lstrlen(p) + 1)
    {
        if (StrStrI(pszPath, p))
        {
            fRet = TRUE;
            break;
        }
    }
    return fRet;
}

//+-------------------------------------------------------------------------
//  Adds a new edit verb.  Returns a pointer to the new verb if it
//  successfully added.
//--------------------------------------------------------------------------
CInternetToolbar::CEditVerb::EDITVERB* CInternetToolbar::CEditVerb::_Add
(
    HKEY hkeyProgID,        // location of of verb
    BOOL fPermitOpenVerb,   // permit open as well as edit verb
    BOOL fCheckForOfficeApp,// redirect to office app
    BOOL fShowIcon          // if button face icon should be customized
)
{
    EDITVERB* pNewVerb = NULL;

    if (hkeyProgID)
    {
        BOOL fUseOpenVerb = FALSE;

        //
        // See if an appropriate verb exists.
        //
        TCHAR szCommand[MAX_PATH];
        HRESULT hr = AssocQueryStringByKey(0, ASSOCSTR_COMMAND, hkeyProgID, L"edit", szCommand, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(szCommand)));
        if (FAILED(hr) && fPermitOpenVerb)
        {
            hr = AssocQueryStringByKey(0, ASSOCSTR_COMMAND, hkeyProgID, NULL, szCommand, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(szCommand)));
            if (SUCCEEDED(hr))
            {
                fUseOpenVerb = TRUE;
            }
        }

        // If no verb or if this is the office redirector, ignore this progid
        // Otherwise we can get two entries that do the same thing.
        if (FAILED(hr) || _IsHtmlStub(szCommand))
        {
            RegCloseKey(hkeyProgID);
            return NULL;
        }

        if (fCheckForOfficeApp)
        {
            ASSERT(*szCommand);

            //
            // HACK: Office2000 needs us to call a special proxy to get around thier DDE bugs and
            // to check the HTML document for the name of the original document.  These problems
            // should be fixed in the apps themselves.  Morons!
            //
            // So if this is an office app, we will redirect to the appropriate progid. Note that
            // we don't need to do this if a progid was from the html meta tag because this progid
            // already supports the proxy.
            //
            struct OfficeHackery {LPCWSTR pszApp; LPCWSTR pszProgID;};

            // Must not have been a progid passed in.
            static const OfficeHackery exeToProgID[] =
            {
                {L"winword",   L"Word.Document"},
                {L"excel",     L"Excel.Sheet"},
                {L"powerpnt",  L"PowerPoint.Slide"},
                {L"msaccess",  L"Access.Application"},
                {L"frontpg",   L"FrontPage.Editor.Document"},
            };

            for (int i=0; i < ARRAYSIZE(exeToProgID); ++i)
            {
                if (StrStrI(szCommand, exeToProgID[i].pszApp))
                {
                    // Match found!
                    HKEY hkeyOffice = NULL;
                    if (SUCCEEDED(AssocQueryKey(0, ASSOCKEY_SHELLEXECCLASS, exeToProgID[i].pszProgID, NULL, &hkeyOffice)))
                    {
                        // Redirect to the office progid
                        RegCloseKey(hkeyProgID);
                        hkeyProgID = hkeyOffice;

                        // The office apps always use the open verb
                        fUseOpenVerb = TRUE;

                        // The icon is shown on the button face for office apps
                        fShowIcon = TRUE;
                    }
                    break;
                }
            }
        }

        EDITVERB newVerb = {0};
        newVerb.hkeyProgID = hkeyProgID;
        newVerb.fUseOpenVerb = fUseOpenVerb;
        newVerb.fShowIcon = fShowIcon;

        // Ignore it if we have another verb to the same exe.
        if (!_IsUnique(newVerb))
        {
            RegCloseKey(hkeyProgID);
        }
        else
        {
            EDITVERB* pVerbsNew;
            if (_pVerb == NULL)
            {
                pVerbsNew = (EDITVERB*)LocalAlloc(LPTR, sizeof(EDITVERB));
            }
            else
            {
                pVerbsNew = (EDITVERB*)LocalReAlloc(_pVerb, (_nElements+1) * sizeof(EDITVERB), LMEM_MOVEABLE | LMEM_ZEROINIT);
            }

            if (pVerbsNew == NULL)
            {
                RegCloseKey(hkeyProgID);
            }
            else
            {
                _pVerb = pVerbsNew;
                pNewVerb = &_pVerb[_nElements];
                *pNewVerb = newVerb;

                //
                // If the description of the executable matches that of the default editor, make
                // it our default edit verb.  If we are not checking for the office app, we
                // can assume that this verb was from a progid in an html file and we will also
                // make it our default.
                //
                LPCWSTR pszDefDesc = _GetDefaultEditor();
                LPCWSTR pszNewDesc = _GetDescription(*pNewVerb);

                if (!fCheckForOfficeApp ||
                    (pszDefDesc && pszNewDesc && StrCmp(pszDefDesc, pNewVerb->pszDesc) == 0))
                {
                    _nDefault = _nElements;
                }

                ++_nElements;
            }
        }
    }

    return pNewVerb;
}

//+-------------------------------------------------------------------------
//  Adds a new edit verb.  Returns TRUE if a verb was successfully added.
//--------------------------------------------------------------------------
BOOL CInternetToolbar::CEditVerb::Add
(
    LPTSTR pszProgID    // program id or file extension associated with verb
)
{
    ASSERT(pszProgID);

    BOOL fRet = FALSE;
    BOOL fFileExt = (pszProgID[0] == TEXT('.'));

    //
    // Open the associated reg key and try to add it to our list of verbs
    //
    BOOL fUseOpenVerb = FALSE;
    HKEY hkeyProgID = NULL;
    BOOL fPermitOpenVerb = !fFileExt;
    BOOL fShowIcon = !fFileExt;    // If a progid was passed in, we will show the icon on the button face

    if (SUCCEEDED(AssocQueryKey(0, ASSOCKEY_SHELLEXECCLASS, pszProgID, NULL, &hkeyProgID)))
    {
        EDITVERB* pNewVerb = _Add(hkeyProgID, fPermitOpenVerb, fFileExt, fShowIcon);
        if (pNewVerb)
        {
            fRet = TRUE;
        }
    }

    //
    // If a file extension was passed in, we also add the alternative editors from the
    // OpenWithList
    //
    if (fFileExt)
    {
        WCHAR szOpenWith[MAX_PATH];
        StrCpyN(szOpenWith, pszProgID, ARRAYSIZE(szOpenWith));
        StrCatBuff(szOpenWith, L"\\OpenWithList", ARRAYSIZE(szOpenWith));

        HKEY hkeyOpenWithList;

        // See if there is an OpenWithList
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, szOpenWith, 0, KEY_READ, &hkeyOpenWithList))
        {
            DWORD dwIndex = 0;
            DWORD dwSize = ARRAYSIZE(szOpenWith);
            HKEY hkeyOpenWith = NULL;
            while (ERROR_SUCCESS == RegEnumKeyEx(hkeyOpenWithList, dwIndex, szOpenWith, &dwSize, NULL, NULL, NULL, NULL))
            {
                if (ERROR_SUCCESS == RegOpenKeyEx(hkeyOpenWithList, szOpenWith, 0, KEY_READ, &hkeyOpenWith))
                {
                    // We only permit the edit verbs from here
                    EDITVERB* pNewVerb = _Add(hkeyOpenWith, FALSE, TRUE, FALSE);
                    if (pNewVerb)
                    {
                        // The key name is the friendly name!
                        PathRemoveBlanks(szOpenWith);
                        SetStr(&pNewVerb->pszDesc, szOpenWith);
                        fRet = TRUE;
                    }

                    // Note that we don't close hkeyOpenWith here.  It is either closed if it was not added, or
                    // it will be closed later.
                }
                ++dwIndex;
                dwSize = ARRAYSIZE(szOpenWith);
            }

            RegCloseKey(hkeyOpenWithList);
        }

        //
        // If a ".htm" or ".html" was passed in, add our default html editor
        //
        if ((StrCmpI(pszProgID, L".htm") == 0 || StrCmpI(pszProgID, L".html") == 0) &&
            _GetDefaultEditor())
        {
            HKEY hkeyDefault;
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_DEFAULT_HTML_EDITOR, 0, KEY_READ, &hkeyDefault))
            {
                if (_Add(hkeyDefault, TRUE, TRUE, FALSE))
                {
                    fRet = TRUE;
                }
            }
        }
    }

    return fRet;
}

//+-------------------------------------------------------------------------
// Returns the tooltip for the default edit verb
//--------------------------------------------------------------------------
BOOL CInternetToolbar::CEditVerb::GetToolTip
(
    LPTSTR pszToolTip,
    UINT cchMax,
    BOOL fStripAmpersands
)
{
    if (_nElements == 0)
    {
        return FALSE;
    }

    // Use the menu text for the tooltip.
    _FormatMenuText(_nDefault);

    // Copy text stripping out any ampersands
    LPWSTR pszDest = pszToolTip;
    LPWSTR pszSrc = _GetVerb(_nDefault).pszMenuText;
    if (0 < cchMax)
    {
        // Leave room for the null terminator
        while (0 < --cchMax)
        {
            // strip out '&'
            if (fStripAmpersands)
            {
                while (*pszSrc == L'&')
                {
                    ++pszSrc;
                }
            }

            if ( !(*pszDest++ = *pszSrc++) )
            {
                --pszDest;
                break;
            }
        }

        if (0 == cchMax)
            *pszDest = L'\0';

        ASSERT(*pszDest == 0);

        //
        // In some locals, the accelerator is identified in brackets at the
        // end of the string, so if we strip ampersands, we strip these too.
        //
        if (fStripAmpersands && --pszDest >= pszToolTip && *pszDest == L')')
        {
            while (--pszDest >= pszToolTip)
            {
                if (*pszDest == L'(')
                {
                    *pszDest = L'\0';
                    break;
                }
            }
        }
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
// "Lazy-fetches" the verb info, and returns the desired info.
//--------------------------------------------------------------------------
CInternetToolbar::CEditVerb::EDITVERB& CInternetToolbar::CEditVerb::_GetVerb(UINT nIndex)
{
    ASSERT(nIndex < _nElements);

    // We fetch the info when first asked for it.
    if (!_pVerb[nIndex].fInit)
    {
        _FetchInfo(nIndex);
        _pVerb[nIndex].fInit = TRUE;
    }
    return _pVerb[nIndex];
}

//+-------------------------------------------------------------------------
// Gets the name of the app associated with the verb.
//--------------------------------------------------------------------------
LPCTSTR CInternetToolbar::CEditVerb::_GetDescription(EDITVERB& rVerb)
{
    // If we already have a description, we are done
    if (NULL == rVerb.pszDesc)
    {
        ASSERT(rVerb.hkeyProgID);

        TCHAR sz[MAX_PATH];
        if (SUCCEEDED(AssocQueryStringByKey(ASSOCF_VERIFY, ASSOCSTR_FRIENDLYAPPNAME, rVerb.hkeyProgID,
            rVerb.fUseOpenVerb ? NULL : L"edit", sz, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(sz)))))
        {
            rVerb.pszDesc = StrDup(sz);
            if (rVerb.pszDesc)
            {
                // Remove preceeding and trailing blanks
                PathRemoveBlanks(rVerb.pszDesc);
            }
        }
    }

    return rVerb.pszDesc;
}

//+-------------------------------------------------------------------------
// Reads the info associated with the progid at the given index.  This
// function allows us to do a lazy fetch of the info when requested.
//--------------------------------------------------------------------------
void CInternetToolbar::CEditVerb::_FetchInfo(UINT nIndex)
{
    ASSERT(nIndex < _nElements);
    ASSERT(_pVerb[nIndex].hkeyProgID != NULL);

    EDITVERB& rVerb = _pVerb[nIndex];

    //
    // Get the path to the edit verb's exe
    //
    if (_GetExePath(rVerb))
    {
        ASSERT(rVerb.pszExe);

        // Note that we fetched the friendly name earlier
        ASSERT(rVerb.pszDesc);

        // Now get the icon
        rVerb.iIcon = Shell_GetCachedImageIndex(rVerb.pszExe, 0, 0);
    }
    else
    {
        rVerb.iIcon = -1;
    }
}

//+-------------------------------------------------------------------------
// SetMSAAMenuInfo()
//
// Fills in MSAAMenuInfo part of EDITVERB from the other fields of the rVerb
//--------------------------------------------------------------------------

void CInternetToolbar::CEditVerb::_SetMSAAMenuInfo( EDITVERB& rVerb )
{

#ifdef UNICODE

    // If we're UNICODE, we can just refer to the m_pName of the MenuEntry itself...
    rVerb.m_MSAA.m_CharLen = lstrlen( rVerb.pszMenuText );
    rVerb.m_MSAA.m_pWStr = rVerb.pszMenuText;

#else // UNICODE

    // If we're ANSI, need to create a UNICODE string for the MSAA text...

    // Call MultiByteToWideChar first with 0 to get the size needed. (Assumes
    // m_pName is ASCII NUL terminated - cChars will include the terminating NUL)
    int cChars = MultiByteToWideChar( CP_ACP, 0, rVerb.pszMenuText, -1, NULL, 0 );

    // Don't want to include NUL in character length, so subtract one...
    rVerb.m_MSAA.m_CharLen = cChars - 1;

    // Now call MultiByteToWideChar to do the conversion.
    // MultiByteToWideChar adds the terminating WIDE-NUL for us, so we don't have to
    // add it explicitly...
    rVerb.m_MSAA.m_pWStr = new WCHAR [ cChars ];
    if (rVerb.m_MSAA.m_pWStr)
    {
        // Note - we don't delete[] the above allocated memory in this sample
        // code because we know that in this case it will be reclaimed by the system on
        // exit and won't give a leak.
        MultiByteToWideChar( CP_ACP, 0, rVerb.pszMenuText, -1, rVerb.m_MSAA.m_pWStr, cChars );
    }

#endif // UNICODE

    // Finally, add MSAAINFO signature...
    rVerb.m_MSAA.m_MSAASig = MSAA_MENU_SIG;
}


//+-------------------------------------------------------------------------
// ClearMSAAMenuInfo()
//
// Clean up MSAAMenuInfo - specifically, release the allocated
// UNICODE string, if appropriate...
//--------------------------------------------------------------------------

void CInternetToolbar::CEditVerb::_ClearMSAAMenuInfo( EDITVERB& rVerb )
{
    // Paranoia - clear signature...
    rVerb.m_MSAA.m_MSAASig = 0;

#ifdef UNICODE

    // We're unicode - nothing to do, since we didn't allocate anything.

#else // UNICODE

    // We're ANSI - release allocated UNICODE string...
    delete [] rVerb.m_MSAA.m_pWStr;

#endif // UNICODE
}


//+-------------------------------------------------------------------------
// Shows the edit pop-up menu.
//--------------------------------------------------------------------------
BOOL CInternetToolbar::CEditVerb::ShowEditMenu(POINT pt, HWND hwnd, LPTSTR pszURL)
{
    BOOL  bRet  = FALSE;
    HMENU hmEdit = CreatePopupMenu();

    if (hmEdit)
    {
        UINT idCmd = FCIDM_EDITFIRST;
        UINT nMax = FCIDM_EDITLAST - FCIDM_EDITFIRST;

        // Add each verb to the menu
        for (UINT i=0; i<_nElements && i < nMax; ++i)
        {
            EDITVERB& rVerb = _GetVerb(i);
            _FormatMenuText(i);
            rVerb.idCmd = idCmd;
            AppendMenu(hmEdit, MF_OWNERDRAW, idCmd, (LPCTSTR) &rVerb );

            // Fix up MSAAMenuInfo part...
            _SetMSAAMenuInfo( rVerb );

            ++idCmd;
        }

        // Temporarily subclass the hwnd to intercept the owner-draw messages
        if (SetProp(hwnd, SZ_EDITVERB_PROP, this))
        {
            ASSERT(!_lpfnOldWndProc);
            _lpfnOldWndProc = (WNDPROC) SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR) _WndProc);

            idCmd = ITBar_TrackPopupMenuEx(hmEdit, TPM_RETURNCMD, pt.x, pt.y, hwnd, NULL);

            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)_lpfnOldWndProc);
            _lpfnOldWndProc = NULL;
            RemoveProp(hwnd, SZ_EDITVERB_PROP);

            if (InRange(idCmd, FCIDM_EDITFIRST, FCIDM_EDITLAST))
            {
                // Execute the selected edit verb
                _Edit(pszURL, idCmd - FCIDM_EDITFIRST);
            }
        }

        DestroyMenu(hmEdit);
    }

    return bRet;
}

//+-------------------------------------------------------------------------
// Creates a menu string from the progid's description
//--------------------------------------------------------------------------
void CInternetToolbar::CEditVerb::_FormatMenuText(UINT nIndex)
{
    ASSERT(nIndex < _nElements);

    EDITVERB& rVerb = _GetVerb(nIndex);
    if (rVerb.pszMenuText == NULL)
    {
        if (_GetDescription(rVerb))
        {
            TCHAR szFormat[100];
            TCHAR szMenuText[200];

            MLLoadString(IDS_EDITWITH, szFormat, ARRAYSIZE(szFormat));
            wnsprintf(szMenuText, ARRAYSIZE(szMenuText), szFormat, rVerb.pszDesc);
            SetStr(&((EDITVERB&)rVerb).pszMenuText, szMenuText);
        }
        else
        {
            // Things are really screwed up
            ASSERT(FALSE);
            SetStr(&((EDITVERB&)rVerb).pszMenuText, TEXT(""));
        }
    }
}


//+-------------------------------------------------------------------------
// Executes the edit verb indicated by nIndex.
//--------------------------------------------------------------------------
void CInternetToolbar::CEditVerb::_Edit
(
    LPCTSTR pszURL,     // url assocated with the verb
    UINT nIndex         // verb to execute
)
{
    ASSERT(pszURL);

    if (nIndex >= _nElements)
    {
        return;
    }

    EDITVERB& rVerb = _pVerb[nIndex];
    int fMask = SEE_MASK_CLASSKEY;

    SHELLEXECUTEINFO sei = {0};

    TCHAR szCacheFileName[MAX_PATH + MAX_URL_STRING + 2];
    memset(szCacheFileName, 0, SIZEOF(szCacheFileName));

    if (PathIsURL(pszURL))
    {
        // We pass the url if the app has register that it wants this
        if ((WhichPlatform() == PLATFORM_BROWSERONLY) && DoesAppWantUrl(rVerb.pszExe))
        {
            //
            // Old versions of shell32 (PLATFORM_BROWSERONLY) ignore the SEE_MASK_FILEANDURL
            // flag, so on these platforms we check ourselves to see if the app
            // wants the url instead of the cache file name.
            //
            StrCpyN(szCacheFileName, pszURL, ARRAYSIZE(szCacheFileName));
            sei.lpFile = szCacheFileName;
        }
        else
        {
            // (reinerf)
            // Some apps (FrontPad, Office99, etc) want the URL passed to
            // them instead of the cache filename. We therefore create a string
            // that has the URL name after the null:
            //
            //  "CacheFileName/0UrlName"
            //
            // and pass it as the lpFile parameter to shellexecute.
            // We also pass SEE_MASK_FILEANDURL, so shellexecute can
            // recognize this case.
            //
            int iLength;

            if (FAILED(URLToCacheFile(pszURL, szCacheFileName, ARRAYSIZE(szCacheFileName))))
            {
                // Frontpage express crashes if we pass a null file name, so if the app doesn't
                // prefer the url instead, we bail.
                if (!DoesAppWantUrl(rVerb.pszExe))
                {
                    return;
                }
            }
            iLength = lstrlen(szCacheFileName);

            // copy in the URL name
            StrCpyN(&szCacheFileName[iLength + 1], pszURL, ARRAYSIZE(szCacheFileName) - (iLength + 1));

            // add the mask so shellexecute knows to check for the URL, if necessary.
            fMask |= SEE_MASK_FILEANDURL;
            sei.lpFile = szCacheFileName;
        }
    }
    else
    {
        // Not a URL, so pass the filename
        StrCpyN(szCacheFileName, pszURL, ARRAYSIZE(szCacheFileName));
        sei.lpFile = szCacheFileName;
    }

    // Hack for IE5 bug 50033 - Can remove when fpxpress fixes mru buffer overrun
    _GetExePath(rVerb);
    if(StrStr(rVerb.pszExe, TEXT("fpxpress.exe")) != NULL)
        szCacheFileName[256] = TEXT('\0');

    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.fMask = fMask;
    sei.hwnd = NULL;
    sei.lpVerb = rVerb.fUseOpenVerb ? NULL : TEXT("edit");
//    sei.lpFile = szCacheFileName;
//    sei.lpParameters = NULL;
    sei.lpDirectory = NULL;
    sei.nShow = SW_SHOWNORMAL;
    sei.hInstApp = NULL;
    sei.hkeyClass= rVerb.hkeyProgID;

    //
    // The office guys want us to call a special proxy to get around some DDE problems
    // and to sniff the html file for the original document name. Hackers! So let's
    // see if it is registered.
    //
    HKEY hkeyProxy = NULL;
    if (ERROR_SUCCESS == RegOpenKeyEx(rVerb.hkeyProgID, TEXT("HTML Handler"), 0, KEY_READ, &hkeyProxy))
    {
        DWORD cch;
        if (SUCCEEDED(AssocQueryStringByKey(0, ASSOCSTR_COMMAND, hkeyProxy, L"edit", NULL, &cch)))
        {
            sei.lpVerb = L"edit";
            sei.hkeyClass = hkeyProxy;
        }
        else if (SUCCEEDED(AssocQueryStringByKey(0, ASSOCSTR_COMMAND, hkeyProxy, NULL, NULL, &cch)))
        {
            sei.lpVerb = NULL;
            sei.hkeyClass = hkeyProxy;
        }
    }

    ShellExecuteEx(&sei);

    if (hkeyProxy)
    {
        RegCloseKey(hkeyProxy);
    }
}

//+-------------------------------------------------------------------------
// This window procedure intercepts owner-draw menu messages when the edit
// pop-up menu is displayed.
//--------------------------------------------------------------------------
LRESULT CALLBACK CInternetToolbar::CEditVerb::_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CEditVerb* pThis = (CEditVerb*)GetProp(hwnd, SZ_EDITVERB_PROP);

    if (!pThis)
        return DefWindowProcWrap(hwnd, uMsg, wParam, lParam);

    switch(uMsg)
    {
        case WM_DRAWITEM:
        case WM_MEASUREITEM:
        {
            UINT idCmd;

            switch (uMsg)
            {
                case WM_DRAWITEM:
                    idCmd = ((EDITVERB*)((DRAWITEMSTRUCT*)lParam)->itemData)->idCmd;
                    break;
                case WM_MEASUREITEM:
                    idCmd = ((EDITVERB*)((MEASUREITEMSTRUCT*)lParam)->itemData)->idCmd;
                    break;
            }

            if (InRange(idCmd, FCIDM_EDITFIRST, FCIDM_EDITLAST))
            {
                // do our own measuring
                UINT index  = idCmd - FCIDM_EDITFIRST;
                const EDITVERB& rVerb = pThis->_GetVerb(index);

                // We don't want the same accelerator on all items,
                // so remove underlines
                WCHAR wzBuf[MAX_PATH];
                UINT cchMax = ARRAYSIZE(wzBuf);
                LPWSTR pszTo = wzBuf;
                LPWSTR pszFrom = rVerb.pszMenuText;
                if (pszFrom)
                {
                    while (0 < --cchMax)
                    {
                        if (*pszFrom == L'&')
                        {
                            pszFrom++;
                            continue;
                        }

                        if ( !(*pszTo++ = *pszFrom++) )
                        {
                            --pszTo;
                            break;
                        }
                    }

                    if (0 == cchMax)
                        *pszTo = L'\0';

                    //
                    // In some locals, the accelerator is identified in brackets at the
                    // end of the string, so if we strip ampersands, we strip these too.
                    //
                    if (--pszTo >= wzBuf && *pszTo == L')')
                    {
                        while (--pszTo >= wzBuf)
                        {
                            if (*pszTo == L'(')
                            {
                                *pszTo = L'\0';
                                break;
                            }
                        }
                    }
                }
                else
                {
                    wzBuf[0] = 0;
                }

                switch (uMsg)
                {
                    case WM_MEASUREITEM:
                        MeasureMenuItem((MEASUREITEMSTRUCT *)lParam, wzBuf);
                        break;
                    case WM_DRAWITEM:
                        int iIcon = (rVerb.iIcon != -1) ? rVerb.iIcon : 0;
                        DrawMenuItem((LPDRAWITEMSTRUCT)lParam, wzBuf, iIcon);
                        break;
                }
            }
        }
        default:
           return CallWindowProc(pThis->_lpfnOldWndProc, hwnd, uMsg, wParam, lParam);
    }
    return 0L;
}

#endif // EDIT_HACK
