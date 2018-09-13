//+---------------------------------------------------------------------------
//
//   Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       formkrnl.hxx
//
//  Contents:   Definition of the CDoc class plus other related stuff
//
//  Classes:    CDoc + more
//
//----------------------------------------------------------------------------

#ifndef _FORMKRNL_HXX_
#define _FORMKRNL_HXX_

#ifndef X_ATOMTBL_HXX_
#define X_ATOMTBL_HXX_
#include "atomtbl.hxx"
#endif

#ifndef X_CLSTAB_HXX_
#define X_CLSTAB_HXX_
#include "clstab.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_HTPVPV_HXX_
#define X_HTPVPV_HXX_
#include "htpvpv.hxx"
#endif

#ifndef X_EDROUTER_HXX_
#define X_EDROUTER_HXX_
#include "edrouter.hxx"
#endif

#ifndef X_MSHTMEXT_H_
#define X_MSHTMEXT_H_
#include "mshtmext.h"
#endif

#ifndef X_INTERNED_H_
#define X_INTERNED_H_
#include "internal.h"
#endif

#ifndef X_CARET_HXX_
#define X_CARET_HXX_
#include "caret.hxx"
#endif

#ifndef X_RECALC_H_
#define X_RECALC_H_
#include "recalc.h"
#endif

#ifndef X_RECALC_HXX_
#define X_RECALC_HXX_
#include "recalc.hxx"
#endif

#ifndef X_FFRAME_HXX_
#define X_FFRAME_HXX_
#include "fframe.hxx"
#endif

#ifndef X_SCRPTLET_H_
#define X_SCRPTLET_H_
#include "scrptlet.h"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#define _hxx_
#include "document.hdl"

#ifndef X_VIEW_HXX_
#define X_VIEW_HXX_
#include "view.hxx"
#endif

#if TREE_SYNC
#ifndef X_SYNCMKP_HXX_
#define X_SYNCMKP_HXX_
#include "syncmkp.hxx"
#endif
#endif // TREE_SYNC

MtExtern(OptionSettings)
MtExtern(OSCodePageAry_pv)
MtExtern(OSContextMenuAry_pv)
MtExtern(CPendingEvents)
MtExtern(CPendingEvents_aryPendingEvents_pv)
MtExtern(CPendingEvents_aryEventType_pv)
MtExtern(CTimeoutEventList)
MtExtern(CTimeoutEventList_aryTimeouts_pv)
MtExtern(CTimeoutEventList_aryPendingTimeouts_pv)
MtExtern(CTimeoutEventList_aryPendingClears_pv)
MtExtern(CDoc)
MtExtern(CDoc_arySitesUnDetached_pv)
MtExtern(CDoc_aryElementDeferredScripts_pv)
MtExtern(CDoc_aryElementReleaseNotify_pv)
MtExtern(CDoc_aryDefunctObjects_pv)
MtExtern(CDoc_aryChildDownloads_pv)
MtExtern(CDoc_aryUrlImgCtx_aryElems_pv)
MtExtern(CDoc_aryUrlImgCtx_pv)
MtExtern(CDoc_aryPeerFactoriesUrl_pv)
MtExtern(CDoc_aryPeerQueue_pv)
MtExtern(CDoc_aryFocusItems_pv)
MtExtern(CDoc_aryAccessKeyItems_pv)
MtExtern(CDoc_aryActiveModelessDlgs)
MtExtern(CDoc_aryDelayReleaseItems_pv)
MtExtern(CDoc_CLock)
MtExtern(CDoc_aryMarkupNotifyInPlace)

MtExtern(CFilter)

//----------------------------------------------------------------------------
//
//  Forward class declarations
//
//----------------------------------------------------------------------------

class CDoc;
class CPrintDoc;
class CSite;
class CCollectionCache;
class CDwnCtx;
class CHtmCtx;
class CImgCtx;
class CBitsCtx;
class CDwnPost;
class CDwnDoc;
class CDwnChan;
class CDwnBindInfo;
class CProgSink;
class COmWindowProxy;
class CTitleElement;
class CAutoRange;
class CDataBindTask;
class CHTMLDlg;
class CFrameSite;
class CTaskLookForBookmark;
class CBodyElement;
class CHistoryLoadCtx;
class CDocUpdateIntSink;
class CScriptElement;
class CHistorySaveCtx;
class CCollectionBuildContext;
class CObjectElement;
class CDwnBindData;
class CProgressBindStatusCallback;
class CDispRoot;
class CScriptCollection;
class CDragDropSrcInfo;
class CDragDropTargetInfo;
class CDragStartInfo;
class CPeerFactoryBinary;
class CPeerFactoryUrl;
#ifdef VSTUDIO7
class CIdentityPeerFactoryUrl;
#endif //VSTUDIO7
class CRect;
class CVersions;
class CMapElement;
class CDeferredCalls;
class CHtmRootParseCtx;
class CElementAdorner;
class CMarkupPointer;
class CGlyph;
class CParentUndo;
class CGlyphRenderInfoType;
class CXmlUrnAtomTable;
class CScriptCookieTable;
class CSelDragDropSrcInfo;
class CScriptDebugDocument;
#ifdef QUILL
class CQDocGlue;
#endif // QUILL
class CAccWindow;

interface INamedPropertyBag;
interface IHTMLWindow2;
interface IHTMLDocument2;
interface IMarkupPointer;
interface IMarkupContainer;
interface IUrlHistoryStg;
interface IProgSink;
interface ITimer;
interface DataSourceListener;
interface ISimpleDataConverter;
interface IHlinkBrowseContext;
interface IHlinkFrame;
interface IActiveXSafetyProvider;
interface IDownloadNotify;
interface IDocHostShowUI;
interface IDocHostUIHandler;
interface IElementBehavior;
interface IElementBehaviorFactory;
interface ILocalRegistry;
interface IScriptletError;
interface IActiveScriptError;
interface IHTMLDOMNode;
#if TREE_SYNC
interface IMarkupSyncServices;
interface IMarkupSyncLogSink;
#endif // TREE_SYNC
interface IActiveIMMApp;

typedef BSTR DataMember;

struct MIMEINFO;
struct DWNLOADINFO;
struct IMGANIMSTATE;

#define XML_DECL            _T("xml:namespace")
#define XML_DECL_SCOPENAME  _T("xml")
#define XML_DECL_TAGNAME    _T("namespace")
#ifdef VSTUDIO7
#define FACTORY_TAGNAME     _T("?FACTORY")
#define TAGSOURCE_TAGNAME   _T("?TAGSOURCE")
#endif //VSTUDIO7

#define JIT_OK              0x0
#define JIT_IN_PROGRESS     0x1
#define JIT_DONT_ASK        0x2
#define JIT_PENDING         0x3
#define JIT_NEED_JIT        0x4

extern BOOL g_fInMoney98;
extern BOOL g_fInHomePublisher98;

const UINT SELTRACK_DBLCLK_TIMERID   = 0x0002; // Selection Dbl Click Timer Id.

//+---------------------------------------------------------------------------
//
//  Enumeration:    PEERTASK
//
//  Synopsis:       peer queue task 
//
//----------------------------------------------------------------------------

enum PEERTASK
{
    // element peer tasks

    PEERTASK_ELEMENT_FIRST,

    PEERTASK_ENTERTREE_UNSTABLE,
    PEERTASK_EXITTREE_UNSTABLE,
    PEERTASK_ENTEREXITTREE_STABLE,

    PEERTASK_RECOMPUTEBEHAVIORS,

    PEERTASK_APPLYSTYLE_UNSTABLE,
    PEERTASK_APPLYSTYLE_STABLE,

    PEERTASK_ELEMENT_LAST,

    // markup peer tasks

    PEERTASK_MARKUP_FIRST,

    PEERTASK_MARKUP_RECOMPUTEPEERS_UNSTABLE,
    PEERTASK_MARKUP_RECOMPUTEPEERS_STABLE,

    PEERTASK_MARKUP_LAST
};

//+---------------------------------------------------------------------------
//
//  Enumeration:    LOADSTATUS
//
//  Synopsis:       Load status that the downloader tells the doc about
//
//----------------------------------------------------------------------------

enum LOADSTATUS
{
    LOADSTATUS_UNINITIALIZED = 0,       // Document hasn't started loading
    LOADSTATUS_INTERACTIVE   = 1,       // Document is now interactive
    LOADSTATUS_PARSE_DONE    = 2,       // HTML has been fully parsed
    LOADSTATUS_QUICK_DONE    = 3,       // Script blocking actions are done
    LOADSTATUS_DONE          = 4,       // Document is fully loaded
    LOADSTATUS_Last_Enum
};

//+---------------------------------------------------------------------------
//
//  Enumeration:    SAFETYLEVEL
//
//  Synopsis:       The current object-safety setting for this object (could
//                  be for the entire document or a specific site).
//
//----------------------------------------------------------------------------
typedef byte SAFETYLEVEL;

#define SAFETY_NOTSET   0
#define SAFETY_NOALL    1
#define SAFETY_NO       2
#define SAFETY_YES      3
#define SAFETY_YESALL   4

// Safety constants.
#define SAFETY_SucceedSilent            0
#define SAFETY_Query                    1
#define SAFETY_FailInform               2
#define SAFETY_FailSilent               3

#define SAFETY_DEFAULT                  2   // Default is Fail/Inform

//+---------------------------------------------------------------------------
//
//  Enumeration:    SSL_SECURITY_STATE
//
//  Synopsis:       Security state for security (HTTP vs HTTPS)
//
//----------------------------------------------------------------------------
enum SSL_SECURITY_STATE
{
      SSL_SECURITY_UNSECURE = 0,// Unsecure (no lock)
      SSL_SECURITY_MIXED,       // Mixed security (broken lock?)
      SSL_SECURITY_SECURE,      // Uniform security
      SSL_SECURITY_SECURE_40,   // Uniform security of >= 40 bits
      SSL_SECURITY_SECURE_56,   // Uniform security of >= 56 bits
      SSL_SECURITY_FORTEZZA,    // Fortezza with Skipjack @80 bits
      SSL_SECURITY_SECURE_128   // Uniform security of >= 128 bits
};

//+---------------------------------------------------------------------------
//
//  Enumeration:    SSL_PROMPT_STATE
//
//  Synopsis:       Allow/Query/Deny unsecure subdownloads
//
//----------------------------------------------------------------------------
enum SSL_PROMPT_STATE
{
      SSL_PROMPT_ALLOW = 0, // Allow nonsecure subdownloads, no prompting
      SSL_PROMPT_QUERY,     // Query on nonsecure subdownloads
      SSL_PROMPT_DENY,      // Deny nonsecure subdownloads, no prompting
      SSL_PROMPT_STATE_Last_Enum
};

//+---------------------------------------------------------------------------
//
//  Enumeration:    STATUS_TEXT_LAYER
//
//  Synopsis:       Allow/Query/Deny unsecure subdownloads
//
//----------------------------------------------------------------------------
enum STATUS_TEXT_LAYER
{
      STL_ROLLSTATUS = 0,    // Hyperlink rollover text layer
      STL_TOPSTATUS  = 1,    // Object model window.status
      STL_DEFSTATUS  = 2,    // Object model window.defaultStatus
      STL_PROGSTATUS = 3,    // Progress status layer
      STL_LAYERS     = 4,    // number of layers
};

//+---------------------------------------------------------------------------
//
//  Constants:      Markup Services Flags
//
//----------------------------------------------------------------------------

#define MUS_DOMOPERATION    (0x1 << 0)      // DOM operations -> text id is content

//+---------------------------------------------------------------------------
//
//  defines:        PRINT_FLAGS
//
//  Synopsis:       See CDoc::DoPrint for description.
//
//----------------------------------------------------------------------------
#define PRINT_DEFAULT               (0x00)
#define PRINT_WAITFORCOMPLETION     (0x01)
#define PRINT_RELEASESPOOLER        (0x02)
#define PRINT_PARSECMD              (0x04)
#define PRINT_DONTBOTHERUSER        (0x08)
#define PRINT_DRTPRINT              (0x10)
#define PRINT_HTMLONLY              (0x20)
#define PRINT_NOGLOBALWINDOW        (0x40)

//+---------------------------------------------------------------------------
//
// defines: NEED3DBORDER
//
// Synopsis: See CDoc::CheckDoc3DBorder for description
//
//----------------------------------------------------------------------------

#define NEED3DBORDER_NO     (0x00)
#define NEED3DBORDER_TOP    (0x01)
#define NEED3DBORDER_LEFT   (0x02)
#define NEED3DBORDER_BOTTOM (0x04)
#define NEED3DBORDER_RIGHT  (0x08)

//+---------------------------------------------------------------------------
//
//  CMDIDs used with CGID_ScriptSite
//
//----------------------------------------------------------------------------

#define CMDID_SCRIPTSITE_URL            0
#define CMDID_SCRIPTSITE_HTMLDLGTRUST   1
#define CMDID_SCRIPTSITE_SECSTATE       2
#define CMDID_SCRIPTSITE_SID            3
#define CMDID_SCRIPTSITE_TRUSTEDDOC     4

//+---------------------------------------------------------------------------
//  Registry Entries for IEAK restrictions
//
//----------------------------------------------------------------------------

#define EXPLORER_REG_KEY        _T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer")
#define NO_FILE_MENU_RESTR      _T("NoFileMenu")

//+---------------------------------------------------------------------------
//
//  Struct : FAVORITES_NOTIFY_INFO
//
//  Synopsis : stucture for passing information through the site notification
//      for the purpose of creating a shortcut/favorite.
//----------------------------------------------------------------------------
struct FAVORITES_NOTIFY_INFO
{
    INamedPropertyBag * pINPB;
    BSTR                bstrNameDomain;
};

#define PERSIST_XML_DATA_SIZE_LIMIT  0x8000

//+---------------------------------------------------------------------------
//
//  Struct:     CODEPAGESETTINGS, CONTEXTMENUEXT, OPTIONSETTINGS
//
//  Synopsis:   Structs that contain user option settings obtained from the
//              registry.
//
//----------------------------------------------------------------------------

#define ACCEPT_LANG_MAX     256

struct CODEPAGESETTINGS
{
    void     Init( )    { fSettingsRead = FALSE; }
    void     SetDefaults(CODEPAGE codepage, SHORT sOptionSettingsBaselineFontDefault);

    BYTE     fSettingsRead;                         // true if we read once from registry
    BYTE     bCharSet;                              // the character set for this codepage
    SHORT    sBaselineFontDefault;                  // must be [0-4]
    UINT     uiFamilyCodePage;                      // the family codepage for which these settings are for
    LONG     latmFixedFontFace;                     // fixed font
    LONG     latmPropFontFace;                      // proportional font
};

struct CONTEXTMENUEXT
{
    CONTEXTMENUEXT() { dwFlags = 0L; dwContexts = 0xFFFFFFFF; }
    CStr     cstrMenuValue;
    CStr     cstrActionUrl;
    DWORD    dwFlags;
    DWORD    dwContexts;
};

enum {
    RTFCONVF_ENABLED = 1,       // General flag to enable rtf to html conversion
    RTFCONVF_DBCSENABLED = 2,   // Use converter for DBCS pages
    RTFCONVF_Last_Enum
};

struct OPTIONSETTINGS
{
    // These operators are defined since we want to allocate extra memory for the string
    //  at the end of the structure.  We cannot simply call MemAlloc directly since we want
    //  to make sure that the constructor for aryCodepageSettings gets called.
    void * operator new(size_t size, size_t extra)      { return MemAlloc( Mt(OptionSettings), size + extra ); }
    void   operator delete(void * pOS)                  { MemFree( pOS ); }

    void Init( TCHAR *psz, BOOL fUseCodePageBasedFontLinking );
    void SetDefaults();
    void ReadCodepageSettingsFromRegistry( CODEPAGESETTINGS * pCS, DWORD dwFlags, SCRIPT_ID sid );

    COLORREF  crBack()   { return ColorRefFromOleColor(colorBack); }
    COLORREF  crText()   { return ColorRefFromOleColor(colorText); }
    COLORREF  crAnchor() { return ColorRefFromOleColor(colorAnchor); }
    COLORREF  crAnchorVisited() { return ColorRefFromOleColor(colorAnchorVisited); }
    COLORREF  crAnchorHovered() { return ColorRefFromOleColor(colorAnchorHovered); }

    OLE_COLOR colorBack;
    OLE_COLOR colorText;
    OLE_COLOR colorAnchor;
    OLE_COLOR colorAnchorVisited;
    OLE_COLOR colorAnchorHovered;

    int     nSafetyWarningLevel;
    int     nAnchorUnderline;

    //$BUGBUG (dinartem) Make these bit fields in beta2 to save space!

    BYTE    fSettingsRead;
    BYTE    fUseDlgColors;
    BYTE    fExpandAltText;
    BYTE    fShowImages;
#ifndef NO_AVI
    BYTE    fShowVideos;
#endif // ndef NO_AVI
    BYTE    fPlaySounds;
    BYTE    fPlayAnimations;
    BYTE    fUseStylesheets;
    BYTE    fSmoothScrolling; // Set to TRUE if the smooth scrolling is allowed
    BYTE    fShowFriendlyUrl;
    BYTE    fSmartDithering;
    BYTE    fAlwaysUseMyColors;
    BYTE    fAlwaysUseMyFontSize;
    BYTE    fAlwaysUseMyFontFace;
    BYTE    fUseMyStylesheet;
    BYTE    fUseHoverColor;
    BYTE    fDisableScriptDebugger;
    BYTE    fMoveSystemCaret;
    BYTE    fHaveAcceptLanguage; // true if cstrLang is valid
    BYTE    fCpAutoDetect;       // true if cp autodetect mode is set
    BYTE    fShowImagePlaceholder;
    BYTE    fUseCodePageBasedFontLinking;
    BYTE    fAllowCutCopyPaste;

    SHORT   sBaselineFontDefault;

    CStr    cstrUserStylesheet;

    CODEPAGE codepageDefault;   // codepage of last resort

    DWORD   dwMaxStatements; // Maximum number of script statements before timeout
    DWORD   dwRtfConverterf; // rtf converter flags

    DWORD   dwMiscFlags;   // visibility of noscope elements, etc...

    DWORD   dwNoChangingWallpaper;

    CStr    cstrLang;

    DECLARE_CPtrAry(OSCodePageAry, CODEPAGESETTINGS *, Mt(Mem), Mt(OSCodePageAry_pv))
    DECLARE_CPtrAry(OSContextMenuAry, CONTEXTMENUEXT *, Mt(Mem), Mt(OSContextMenuAry_pv))

    // We keep an array for the codepage settings here
    OSCodePageAry       aryCodepageSettingsCache;

    // The context menu extentions
    OSContextMenuAry    aryContextMenuExts;

    // Script-based font info.  Array of LONG atoms.
#ifndef NO_UTF16
    LONG alatmProporitionalFonts[sidLimPlusSurrogates];
    LONG alatmFixedPitchFonts[sidLimPlusSurrogates];
#else
    LONG alatmProporitionalFonts[sidLim];
    LONG alatmFixedPitchFonts[sidLim];
#endif

    // The following member must be the last member in the struct.
    TCHAR    achKeyPath[1];

};

enum REGUPDATE_FLAGS {
    REGUPDATE_REFRESH = 1,              // always read from registry, not cache
    REGUPDATE_OVERWRITELOCALSTATE = 2   // re-read local state settings as well
};

struct  RADIOGRPNAME
{
    LPCTSTR         lpstrName;
    RADIOGRPNAME    *_pNext;
};


#ifndef _MAC
enum FOCUS_DIRECTION
{
    DIRECTION_BACKWARD,
    DIRECTION_FORWARD,
};
#endif

struct TABINFO
{
    FOCUS_DIRECTION dir;
    CElement *      pElement;
    BOOL            fFindNext;
};

// Error structure used by ReportScriptError (both scripts and scriptoids).
struct ErrorRecord
    {
    LPTSTR                  _pchURL;
    CExcepInfo              _ExcepInfo;
    ULONG                   _uColumn;
    ULONG                   _uLine;
    DWORD                   _dwSrcContext;
    IHTMLElement *          _pElement;
    CScriptDebugDocument *  _pScriptDebugDocument;

    ErrorRecord() { memset (this, 0, sizeof(*this)); }

    HRESULT Init(IScriptletError *pSErr, LPTSTR pchDocURL);
    HRESULT Init(IActiveScriptError *pASErr, CDoc * pDoc);
    HRESULT Init(HRESULT hrError, LPTSTR pchErrorMessage, LPTSTR pchDocURL);

    void SetElement(IHTMLElement *pElement)
      { _pElement = pElement; }
};

BOOL IsTridentHwnd( HWND hwnd );

static BOOL CALLBACK OnChildBeginSelection( HWND hwnd, LPARAM lParam );

class CSelectionObject;


struct EVENTPARAM;

//+---------------------------------------------------------------------------
//
//  Struct:     SITE_STG
//
//  Synopsys:   Struct used for entries in the site-storage look-aside table.
//
//----------------------------------------------------------------------------

struct  GROUP_INFO
{
    USHORT     usID;    // Group ID
    USHORT     cSites;  // count of Sites in group
};

struct TIMEOUTEVENTINFO
{
    IDispatch    *_pCode;           // PCODE code to execute if set don't use _code.
    CStr          _code;            // _pCode is NULL used _code and _lang to parse and execute.
    CStr          _lang;            // script engine to use to parse _code
    UINT          _uTimerID;        // timer id
    DWORD         _dwTargetTime;    // System time (in  milliseconds) when
                                    // the timer is going to time out
    DWORD         _dwInterval;      // interval for repeating timer. set to
                                    // zero if called by setTimeout

public:
    TIMEOUTEVENTINFO() : _pCode(NULL)
      {  }
    ~TIMEOUTEVENTINFO()
      { ReleaseInterface(_pCode); }
};

class CTimeoutEventList
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CTimeoutEventList))

    CTimeoutEventList()
    {
#ifndef WIN16
        _uNextTimerID = 1;
#endif
    }
    // Gets the first event that has min target time. If uTimerID not found
    //      returns S_FALSE
    // Removes the event from the list before returning
    HRESULT GetFirstTimeoutEvent(UINT uTimerID, TIMEOUTEVENTINFO **pTimeout);

    // Returns the timer event with given timer ID, or returns S_FALSE
    //      if timer not found
    HRESULT GetTimeout(UINT uTimerID, TIMEOUTEVENTINFO **ppTimeout);

    // Inserts given timer event object into the list  and returns timer ID
    HRESULT InsertIntoTimeoutList(TIMEOUTEVENTINFO *pTimeoutToInsert, UINT *puTimerID = NULL, BOOL fNewID=TRUE);

    // Kill all the script times for given object
    void    KillAllTimers(void * pvObject);

    // Add an Interval timeout to a pending list to be requeued after script processing
    void    AddPendingTimeout( TIMEOUTEVENTINFO *pTimeout ) {_aryPendingTimeouts.Append(pTimeout);}

    // Returns timer event on pending queue. Removes timer event from the list
    BOOL    GetPendingTimeout( TIMEOUTEVENTINFO **ppTimeout );

    // If an interval timeout is cleared while in script, remove it from the pending list
    BOOL    ClearPendingTimeout( UINT uTimerID );

    // a clear was called during timer script processing, clear after all processing done.
    void    AddPendingClear( LONG lTimerID ) {_aryPendingClears.Append(lTimerID);}

    // Returns the timer ID of the timer to clear
    BOOL    GetPendingClear( LONG *plTimerID );

private:
    DECLARE_CPtrAry(CAryTimeouts, TIMEOUTEVENTINFO *, Mt(Mem), Mt(CTimeoutEventList_aryTimeouts_pv))
    DECLARE_CPtrAry(CAryPendingTimeouts, TIMEOUTEVENTINFO *, Mt(Mem), Mt(CTimeoutEventList_aryPendingTimeouts_pv))
    DECLARE_CPtrAry(CAryPendingClears, LONG_PTR, Mt(Mem), Mt(CTimeoutEventList_aryPendingClears_pv))

    CAryTimeouts            _aryTimeouts;
    CAryPendingTimeouts     _aryPendingTimeouts;
    CAryPendingClears       _aryPendingClears;
#ifdef WIN16
    static
#endif
    UINT _uNextTimerID;
};

HRESULT EnsureWindowInfo();
HRESULT InitDocClass(void);

//+---------------------------------------------------------------------------
//
//  Flag values for CServer::CLock
//
//----------------------------------------------------------------------------

enum FORMLOCK_FLAG
{
    FORMLOCK_ARYSITE        = (SERVERLOCK_LAST << 1),   // don't allow any mods to site lists
    FORMLOCK_SITES          = (FORMLOCK_ARYSITE << 1),  // don't delete sites
    FORMLOCK_CURRENT        = (FORMLOCK_SITES << 1),    // don't allow current change
    FORMLOCK_LOADING        = (FORMLOCK_CURRENT << 1),  // don't allow loading
    FORMLOCK_UNLOADING      = (FORMLOCK_LOADING << 1),  // unloading now
    FORMLOCK_QSEXECCMD      = (FORMLOCK_UNLOADING << 1),// In the middle of queryCommand/execCommand
    FORMLOCK_FILTER         = (FORMLOCK_QSEXECCMD << 1),// Doing filter hookup
    FORMLOCK_FLAG_Last_Enum = 0
};

//+---------------------------------------------------------------------------
//
//  Flag values for Markup Services
//
//----------------------------------------------------------------------------

enum VALIDATE_ELEMENTS_FLAG
{
    VALIDATE_ELEMENTS_REQUIREDCONTAINERS = 1,
};


//
//  Classes
//


//+---------------------------------------------------------------------------
//
//  Class:      CFormInPlace
//
//  Purpose:    Implementation of CInPlace that doesn't delete itself when
//              _ulRefs=0 so we can embed it inside CDoc.
//
//----------------------------------------------------------------------------

class CFormInPlace : public CInPlace
{
public:

    ~CFormInPlace();
#if DBG == 1
    HMENU                   _hmenu;         //  Our menu
    HOLEMENU                _hOleMenu;      //  Menu descriptor
    HMENU                   _hmenuShared;   //  Shared menu when UI Active
    OLEMENUGROUPWIDTHS      _mgw;           //  Menu interleaving info
#endif

    IDocHostShowUI *        _pHostShowUI;

    unsigned                _fHostShowUI:1;     // True if host display UI.
    unsigned                _fForwardSetBorderSpace:1;  // Forward SetBorderSpace

                                                       //   to frame
    unsigned                _fForwardSetMenu:1; // Forward SetMenu to frame
};

class CFrameSetSite;



//+---------------------------------------------------------------
//
//  Class:      COmDocument
//
//  Purpose:    Om document implementation
//
//---------------------------------------------------------------

class CDoc;

class COmDocument : public IUnknown
{
public:

    // CBase methods
    DECLARE_SUBOBJECT_IUNKNOWN(CDoc, Doc)
};


//+---------------------------------------------------------------
//
//  Class:      CDoEnableModeless
//
//  Purpose:    Class to handle EnableModeless.  
//
//---------------------------------------------------------------

class CDoEnableModeless
{
public:
    CDoEnableModeless(CDoc *pDoc, BOOL fAuto = TRUE);
    ~CDoEnableModeless();

    void        DisableModeless();
    void        EnableModeless(BOOL fForce = FALSE);
    
    CDoc *      _pDoc;
    BOOL        _fCallEnableModeless;
    BOOL        _fAuto;
    HWND        _hwnd;
};


//+---------------------------------------------------------------
//
//  Class:      CSecurityMgrSite
//
//  Purpose:    Site implementation for the security manager
//              It's a subobject to avoid ref-count loops.
//
//---------------------------------------------------------------

class CSecurityMgrSite :
    public IInternetSecurityMgrSite,
    public IServiceProvider

{
public:
    DECLARE_SUBOBJECT_IUNKNOWN(CDoc, Doc)

    // IInternetSecurityMgrSite methods
    STDMETHOD(GetWindow)(HWND *phwnd);
    STDMETHOD(EnableModeless)(BOOL fEnable);

    // IServiceProvider methods
    STDMETHOD(QueryService)(REFGUID guidService, REFIID iid, LPVOID * ppv);
};


class CParser;
class CHtmLoadCtx;
class CLinkElement;

struct REGKEYINFORMATION;

enum ELEMENT_TAG;

#define UNIQUE_NAME_PREFIX  _T("ms__id")


//+---------------------------------------------------------------
//
//  Class:      CDefaultElement
//
//  Purpose:    There are assumptions made in CDoc that there
//              always be a site available to point to.  The
//              default site is used for these purposes.
//
//              The root site used to erroneously serve this
//              purpose.
//
//---------------------------------------------------------------

MtExtern(CDefaultElement)

class CDefaultElement : public CElement
{
    typedef CElement super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDefaultElement))

    CDefaultElement ( CDoc * pDoc );

    DECLARE_CLASSDESC_MEMBERS;
};

// CMessage helpers
void CMessageToSelectionMessage( const CMessage* pMessage, SelectionMessage *pSelMessage );
void SelectionMessageToCMessage( const SelectionMessage *pSelMessage, CMessage* pMessage );

//+---------------------------------------------------------------
//
//  Class:      CDoc
//
//  Purpose:    Document implementation
//
//---------------------------------------------------------------

class CDoc :
        public CServer,
        public IHTMLDocument2,
        public IHTMLDocument3,
        public IDispatchEx

{
    DECLARE_CLASS_TYPES(CDoc, CServer);
private:
    DataSourceListener *_pDSL;

public:
    friend class CHtmLoad;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDoc))

    DECLARE_TEAROFF_TABLE(IRequireClasses)
    DECLARE_TEAROFF_TABLE(IMarqueeInfo)
    DECLARE_TEAROFF_TABLE(IOleItemContainer)
    DECLARE_TEAROFF_TABLE(IServiceProvider)
    DECLARE_TEAROFF_TABLE(IPersistFile)
    DECLARE_TEAROFF_TABLE(IPersistMoniker)
    DECLARE_TEAROFF_TABLE(IPersistHistory)
    DECLARE_TEAROFF_TABLE(IHlinkTarget)
    DECLARE_TEAROFF_TABLE(DataSource)
    DECLARE_TEAROFF_TABLE(ITargetContainer)
    DECLARE_TEAROFF_TABLE(IShellPropSheetExt)
    DECLARE_TEAROFF_TABLE(IObjectSafety)
    DECLARE_TEAROFF_TABLE(IInternetHostSecurityManager)
    DECLARE_TEAROFF_TABLE(ICustomDoc)
#ifdef WIN16
    DECLARE_TEAROFF_TABLE(IPersistStreamInit)
#endif
    DECLARE_TEAROFF_TABLE(IAuthenticate)
    DECLARE_TEAROFF_TABLE(IWindowForBindingUI)
    DECLARE_TEAROFF_TABLE(IMarkupServices)
    DECLARE_TEAROFF_TABLE(IHTMLViewServices)
#if TREE_SYNC
    DECLARE_TEAROFF_TABLE(IMarkupSyncServices)
#endif // TREE_SYNC
#ifdef XMV_PARSE
    DECLARE_TEAROFF_TABLE(IXMLGenericParse)
#endif // XMV_PARSE
#if DBG == 1
    DECLARE_TEAROFF_TABLE(IEditDebugServices)
#endif

    DECLARE_AGGREGATED_IUNKNOWN(CDoc)

    DECLARE_PRIVATE_QI_FUNCS(CServer)

    //+-----------------------------------------------------------------------
    //
    //  CDoc::CLock
    //
    //------------------------------------------------------------------------

    class CLock : public CServer::CLock
    {
    public:
        DECLARE_MEMALLOC_NEW_DELETE(Mt(CDoc_CLock))
        CLock(CDoc *pDoc, WORD wLockFlags = 0);
        ~CLock();

    private:
        CScriptCollection * _pScriptCollection;
    };

    // IDispatch methods
    DECLARE_TEAROFF_METHOD(GetTypeInfoCount, gettypeinfocount,  (UINT FAR* pctinfo))
        { RRETURN(THR(super::GetTypeInfoCount(pctinfo))); }

    DECLARE_TEAROFF_METHOD(GetTypeInfo, gettypeinfo,               (
                UINT itinfo,
                LCID lcid,
                ITypeInfo ** pptinfo))
        { RRETURN(THR(super::GetTypeInfo(itinfo, lcid, pptinfo))); }

    DECLARE_TEAROFF_METHOD(GetIDsOfNames, getidsofnames,             (
                REFIID                riid,
                LPOLESTR *            rgszNames,
                UINT                  cNames,
                LCID                  lcid,
                DISPID FAR*           rgdispid));

    DECLARE_TEAROFF_METHOD(Invoke, invoke,                    (
                DISPID          dispidMember,
                REFIID          riid,
                LCID            lcid,
                WORD            wFlags,
                DISPPARAMS *    pdispparams,
                VARIANT *       pvarResult,
                EXCEPINFO *     pexcepinfo,
                UINT *          puArgErr));

    // IDispatchEx methods

    DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (BSTR bstrName, DWORD grfdex, DISPID *pid));

    DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (DISPID dispid,
                         LCID lcid,
                         WORD wFlags,
                         DISPPARAMS *pdispparams,
                         VARIANT *pvarResult,
                         EXCEPINFO *pexcepinfo,
                         IServiceProvider *pSrvProvider));

    DECLARE_TEAROFF_METHOD(GetMemberProperties, getmemberproperties, (
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex));

    HRESULT STDMETHODCALLTYPE DeleteMemberByName(BSTR bstr,DWORD grfdex);

    HRESULT STDMETHODCALLTYPE DeleteMemberByDispID(DISPID id);

    STDMETHOD(GetMemberName) (DISPID id,
                              BSTR *pbstrName);

    DECLARE_TEAROFF_METHOD(GetNextDispID, getnextdispid, (
                DWORD grfdex,
                DISPID id,
                DISPID *prgid));

    DECLARE_TEAROFF_METHOD(GetNameSpaceParent, getnamespaceparent, (IUnknown **ppunk));

    //  IOleObject methods

    HRESULT STDMETHODCALLTYPE Update();
    HRESULT STDMETHODCALLTYPE IsUpToDate();
    HRESULT STDMETHODCALLTYPE GetUserClassID(CLSID * pClsid);
    HRESULT STDMETHODCALLTYPE SetClientSite(LPOLECLIENTSITE pClientSite);
    HRESULT STDMETHODCALLTYPE SetExtent(DWORD dwDrawAspect, SIZEL *psizel);
    HRESULT STDMETHODCALLTYPE SetHostNames(LPCTSTR szContainerApp, LPCTSTR szContainerObj);
    HRESULT STDMETHODCALLTYPE Close(DWORD dwSaveOption);
    HRESULT STDMETHODCALLTYPE GetMoniker (DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER * ppmk);

    //  IPersist methods

    NV_DECLARE_TEAROFF_METHOD(IsDirty, isdirty, ());

    //  IOleInPlaceObject methods

    HRESULT STDMETHODCALLTYPE SetObjectRects(LPCOLERECT prcPos, LPCOLERECT prcClip);
    HRESULT STDMETHODCALLTYPE ReactivateAndUndo();

    //  IOleInPlaceObjectWindowless methods

    HRESULT STDMETHODCALLTYPE OnWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

    // IMsoCommandTarget methods

    struct CTQueryStatusArg
    {
        ULONG           cCmds;
        MSOCMD *        rgCmds;
        MSOCMDTEXT *    pcmdtext;
    };

    struct CTExecArg
    {
        DWORD       nCmdID;
        DWORD       nCmdexecopt;
        VARIANTARG *pvarargIn;
        VARIANTARG *pvarargOut;
    };
    
    struct CTArg
    {
        BOOL    fQueryStatus;
        GUID *  pguidCmdGroup;
        union
        {
            CTQueryStatusArg *  pqsArg;
            CTExecArg *         pexecArg;
        };
    };
    
    HRESULT STDMETHODCALLTYPE QueryStatus(
            GUID * pguidCmdGroup,
            ULONG cCmds,
            MSOCMD rgCmds[],
            MSOCMDTEXT * pcmdtext);
    HRESULT STDMETHODCALLTYPE Exec(
            GUID * pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut);
    HRESULT RouteCTElement(CElement *pElement, CTArg *parg);
    
    HRESULT OnContextMenuExt(UINT idm, VARIANTARG * pvarargIn);

    //  IOleControl methods

    HRESULT STDMETHODCALLTYPE GetControlInfo(CONTROLINFO * pCI);
    HRESULT STDMETHODCALLTYPE OnMnemonic(LPMSG pMsg);
    HRESULT STDMETHODCALLTYPE OnAmbientPropertyChange(DISPID dispid);
    HRESULT STDMETHODCALLTYPE FreezeEvents(BOOL fFreeze);

    //  IParseDisplayName methods

    NV_DECLARE_TEAROFF_METHOD(ParseDisplayName , parsedisplayname , (LPBC pbc,
            LPTSTR lpszDisplayName,
            ULONG FAR* pchEaten,
            LPMONIKER FAR* ppmkOut));

    //  IOleContainer methods

    NV_DECLARE_TEAROFF_METHOD(EnumObjects , enumobjects , (DWORD grfFlags, LPENUMUNKNOWN FAR* ppenumUnknown));
    NV_DECLARE_TEAROFF_METHOD(LockContainer , lockcontainer , (BOOL fLock));

    //  IOleItemContainer methods

    DECLARE_TEAROFF_METHOD(GetObject , getobject , (LPTSTR lpszItem,
            DWORD dwSpeedNeeded,
            LPBINDCTX pbc,
            REFIID riid,
            void ** ppvObject));
    NV_DECLARE_TEAROFF_METHOD(GetObjectStorage , getobjectstorage , (LPTSTR lpszItem,
            LPBINDCTX pbc,
            REFIID riid,
            void ** ppvStorage));
    NV_DECLARE_TEAROFF_METHOD(IsRunning , isrunning , (LPTSTR lpszItem));

    //  IPersistFile methods

    STDMETHOD(Load)(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHOD(Save)(LPCOLESTR pszFileName, BOOL fRemember);
    STDMETHOD(SaveCompleted)(LPCOLESTR pszFileName);
    NV_DECLARE_TEAROFF_METHOD(GetCurFile, getcurfile, (LPOLESTR *ppszFileName));
    DECLARE_TEAROFF_METHOD(GetClassID, getclassid, (CLSID *));

    //  IPersistMoniker methods

    DECLARE_TEAROFF_METHOD(Load, load, (BOOL fFullyAvailable, IMoniker *pmkName, LPBC pbc, DWORD grfMode));
    DECLARE_TEAROFF_METHOD(Save, save, (IMoniker *pmkName, LPBC pbc, BOOL fRemember));
    DECLARE_TEAROFF_METHOD(SaveCompleted, savecompleted, (IMoniker *pmkName, LPBC pibc));
    NV_DECLARE_TEAROFF_METHOD(GetCurMoniker, getcurmoniker, (IMoniker  **ppmkName));

    //  IPersistHistory methods

    NV_DECLARE_TEAROFF_METHOD(LoadHistory, loadhistory, (IStream *pStream, IBindCtx *pbc));
    NV_DECLARE_TEAROFF_METHOD(SaveHistory, savehistory, (IStream *pStream));
    NV_DECLARE_TEAROFF_METHOD(SetPositionCookie, setpositioncookie, (DWORD dwPositioncookie));
    NV_DECLARE_TEAROFF_METHOD(GetPositionCookie, getpositioncookie, (DWORD *pdwPositioncookie));

    HRESULT LoadHistoryInternal(IStream *pStream, IBindCtx *pbc, DWORD dwBindf, IMoniker *pmkHint, IStream *pstmLeader, CDwnBindData *pDwnBindData, CODEPAGE codepage);
    HRESULT SaveHistoryInternal(IStream *pStream, DWORD dwSavehistOptions);
    HRESULT LoadFailureUrl(TCHAR *pchUrl, IStream *pStreamRefresh);

    //  IPersistStream methods

    NV_STDMETHOD(Load)(IStream *pStm)
            { return super::Load(pStm); }
    NV_STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty)
            { return super::Save(pStm, fClearDirty); }
    NV_STDMETHOD(InitNew)(void);

    //  IHlinkTarget methods

    NV_DECLARE_TEAROFF_METHOD(SetBrowseContext, setbrowsecontext, (IHlinkBrowseContext *pihlbc));
    NV_DECLARE_TEAROFF_METHOD(GetBrowseContext, getbrowsecontext, (IHlinkBrowseContext **ppihlbc));
    NV_DECLARE_TEAROFF_METHOD(Navigate, navigate, (DWORD grfHLNF, LPCWSTR wzJumpLocation));
    // NOTE: the following is renamed in tearoff to avoid multiple inheritance problem with IOleObject::GetMoniker
    NV_DECLARE_TEAROFF_METHOD(GetMonikerHlink, getmonikerhlink, (LPCWSTR wzLocation, DWORD dwAssign, IMoniker **ppimkLocation));
    NV_DECLARE_TEAROFF_METHOD(GetFriendlyName, getfriendlyname, (LPCWSTR wzLocation, LPWSTR *pwzFriendlyName));

    // DataSource methods
    HRESULT STDMETHODCALLTYPE getDataMember(DataMember bstrDM,REFIID riid, IUnknown **ppunk);
    HRESULT STDMETHODCALLTYPE getDataMemberName(long lIndex, DataMember *pbstrDM);
    HRESULT STDMETHODCALLTYPE getDataMemberCount(long *plCount);
    HRESULT STDMETHODCALLTYPE addDataSourceListener(DataSourceListener *pDSL);
    HRESULT STDMETHODCALLTYPE removeDataSourceListener(DataSourceListener *pDSL);

    // ITargetContainer methods

    NV_DECLARE_TEAROFF_METHOD(GetFrameUrl, getframeurl, (LPWSTR *ppszFrameSrc));
    NV_DECLARE_TEAROFF_METHOD(GetFramesContainer, getframescontainer, (IOleContainer **ppContainer));

    //  IDropTarget methods

    HRESULT STDMETHODCALLTYPE DragEnter(
            LPDATAOBJECT pDataObj,
            DWORD grfKeyState,
            POINTL pt,
            LPDWORD pdwEffect);
    HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    HRESULT STDMETHODCALLTYPE DragLeave(BOOL fDrop);
    HRESULT STDMETHODCALLTYPE Drop(
            LPDATAOBJECT pDataObj,
            DWORD grfKeyState,
            POINTL pt,
            LPDWORD pdwEffect);

    // IRequireClasses methods

    NV_DECLARE_TEAROFF_METHOD(CountRequiredClasses, countrequiredclasses, (ULONG * pcClasses));
    NV_DECLARE_TEAROFF_METHOD(GetRequiredClasses, getrequiredclasses, (ULONG index, CLSID *pclsid));

    // IMarqueeInfo method

    NV_DECLARE_TEAROFF_METHOD(GetDocCoords, getdoccoords, (LPRECT pViewRect, BOOL bGetOnlyIfFullyLoaded,
                        BOOL *pfFullyLoaded, int WidthToFormatPageTo));

#ifndef WIN16
    // IShellPropSheetExt methods
    HRESULT STDMETHODCALLTYPE AddPages(
                LPFNADDPROPSHEETPAGE lpfnAddPage,
                LPARAM lParam);
    HRESULT STDMETHODCALLTYPE ReplacePage(
                UINT uPageID,
                LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                LPARAM lParam)
        { return E_NOTIMPL; }
#endif //!WIN16

    // IObjectSafety methods

    NV_DECLARE_TEAROFF_METHOD(GetInterfaceSafetyOptions, getinterfacesafetyoptions, (
                REFIID riid,
                DWORD *pdwSupportedOptions,
                DWORD *pdwEnabledOptions));
    NV_DECLARE_TEAROFF_METHOD(SetInterfaceSafetyOptions, setinterfacesafetyoptions, (
                REFIID riid,
                DWORD dwOptionSetMask,
                DWORD dwEnabledOptions));

    // IViewObject methods

    STDMETHOD(GetColorSet)(DWORD dwDrawAspect, LONG lindex, void * pvAspect, DVTARGETDEVICE * ptd, HDC hicTargetDev, LPLOGPALETTE * ppColorSet);

    // IInternetHostSecurityManager methods

    NV_DECLARE_TEAROFF_METHOD(HostGetSecurityId, hostgetsecurityid, (BYTE *pbSID, DWORD *pcb, LPCWSTR pwszDomain));
    NV_DECLARE_TEAROFF_METHOD(HostProcessUrlAction, hostprocessurlaction, (DWORD dwAction, BYTE *pPolicy, DWORD cbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved));
    NV_DECLARE_TEAROFF_METHOD(HostQueryCustomPolicy, hostquerycustompolicy, (REFGUID guidKey, BYTE **ppPolicy, DWORD *pcbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwReserved));

    // ICustomDoc methods

    NV_DECLARE_TEAROFF_METHOD(SetUIHandler, setuihandler, (
                IDocHostUIHandler * pUIHandler));

    // IAuthenticate methods

    NV_DECLARE_TEAROFF_METHOD(Authenticate, authenticate, (HWND * phwnd, LPWSTR * pszUsername, LPWSTR * pszPassword));

    // IWindowForBindingUI methods

    NV_DECLARE_TEAROFF_METHOD(GetWindowBindingUI, getwindowbindingui, (REFGUID rguidReason, HWND * phwnd));

    // IMarkupServices methods

    NV_DECLARE_TEAROFF_METHOD(
        CreateMarkupPointer, createmarkuppointer, (
            IMarkupPointer **ppPointer));

    NV_DECLARE_TEAROFF_METHOD(
        CreateMarkupContainer, createmarkupcontainer, (
            IMarkupContainer **ppMarkupContainer));

    NV_DECLARE_TEAROFF_METHOD(
        CreateElement, createelement, (
            ELEMENT_TAG_ID tagID,
            OLECHAR * pchAttributes,
            IHTMLElement **ppElement));

    NV_DECLARE_TEAROFF_METHOD(
        CloneElement, cloneelement, (
            IHTMLElement *   pElemCloneThis,
            IHTMLElement * * ppElemClone ));

    NV_DECLARE_TEAROFF_METHOD(
        InsertElement, insertelement, (
            IHTMLElement *pElementInsert,
            IMarkupPointer *pPointerStart,
            IMarkupPointer *pPointerFinish));

    NV_DECLARE_TEAROFF_METHOD(
        RemoveElement, removeelement, (
            IHTMLElement *pElementRemove));

    NV_DECLARE_TEAROFF_METHOD(
        Remove, remove, (
            IMarkupPointer *pPointerStart,
            IMarkupPointer *pPointerFinish));

    NV_DECLARE_TEAROFF_METHOD(
        Copy, copy, (
            IMarkupPointer *pPointerSourceStart,
            IMarkupPointer *pPointerSourceFinish,
            IMarkupPointer *pPointerTarget));

    NV_DECLARE_TEAROFF_METHOD(
        Move, move, (
            IMarkupPointer *pPointerSourceStart,
            IMarkupPointer *pPointerSourceFinish,
            IMarkupPointer *pPointerTarget));

    NV_DECLARE_TEAROFF_METHOD(
        InsertText, inserttext, (
            OLECHAR * pchText,
            long cch,
            IMarkupPointer *pPointerTarget));

    NV_DECLARE_TEAROFF_METHOD(
        ParseString, parsestring, (
            OLECHAR * pchHTML,
            DWORD dwFlags,
            IMarkupContainer **pContainerResult,
            IMarkupPointer *,
            IMarkupPointer *));

    NV_DECLARE_TEAROFF_METHOD(
        ParseGlobal, parseglobal, (
            HGLOBAL hGlobal,
            DWORD dwFlags,
            IMarkupContainer **pContainerResult,
            IMarkupPointer *,
            IMarkupPointer *));

    NV_DECLARE_TEAROFF_METHOD(
        IsScopedElement, isscopedelement, (
            IHTMLElement * pElement,
            BOOL * pfScoped));

    NV_DECLARE_TEAROFF_METHOD(
        GetElementTagId, getelementtagid, (
            IHTMLElement * pElement,
            ELEMENT_TAG_ID * ptagId));

    NV_DECLARE_TEAROFF_METHOD(
        GetTagIDForName, gettagidforname, (
            BSTR bstrName,
            ELEMENT_TAG_ID * ptagId));

    NV_DECLARE_TEAROFF_METHOD(
        GetNameForTagID, getnamefortagid, (
            ELEMENT_TAG_ID tagId,
            BSTR * pbstrName));

    NV_DECLARE_TEAROFF_METHOD(
        MovePointersToRange, movepointerstorange, (
            IHTMLTxtRange *pIRange,
            IMarkupPointer *pPointerStart,
            IMarkupPointer *pPointerFinish));

    NV_DECLARE_TEAROFF_METHOD(
        MoveRangeToPointers, moverangetopointers, (
            IMarkupPointer *pPointerStart,
            IMarkupPointer *pPointerFinish,
            IHTMLTxtRange *pIRange));

    NV_DECLARE_TEAROFF_METHOD(
        BeginUndoUnit, beginundounit, (
            OLECHAR * pchUnitTitle));

    NV_DECLARE_TEAROFF_METHOD(
        EndUndoUnit, endundounit, (
            ));

    //
    // Internal version of Markup Services routines which don't take
    // interfaces.
    //
    
    HRESULT CreateMarkupPointer (
            CMarkupPointer * * ppPointer );
    
    HRESULT CreateMarkupContainer (
            CMarkup * * ppMarkupContainer );
    
    HRESULT CreateElement (
            ELEMENT_TAG_ID tagID,
            OLECHAR *      pchAttributes,
            CElement * *   ppElement );
    
    HRESULT CloneElement (
            CElement *   pElemCloneThis,
            CElement * * ppElemClone );
    
    HRESULT InsertElement (
            CElement *       pElementInsert,
            CMarkupPointer * pPointerStart,
            CMarkupPointer * pPointerFinish,
            DWORD            dwFlags = NULL );
    
    HRESULT RemoveElement (
            CElement *       pElementRemove,
            DWORD            dwFlags = NULL );
    
    HRESULT Remove (
            CMarkupPointer * pPointerStart,
            CMarkupPointer * pPointerFinish,
            DWORD            dwFlags = NULL );
    
    HRESULT Copy (
            CMarkupPointer * pPointerSourceStart,
            CMarkupPointer * pPointerSourceFinish,
            CMarkupPointer * pPointerTarget,
            DWORD            dwFlags = NULL );
    
    HRESULT Move (
            CMarkupPointer * pPointerSourceStart,
            CMarkupPointer * pPointerSourceFinish,
            CMarkupPointer * pPointerTarget,
            DWORD            dwFlags = NULL );
    
    HRESULT InsertText (
            CMarkupPointer * pPointerTarget,
            const OLECHAR *  pchText,
            long             cch,
            DWORD            dwFlags = NULL );
    
    HRESULT ParseString (
            OLECHAR *        pchHTML,
            DWORD            dwFlags,
            CMarkup * *      ppContainerResult,
            CMarkupPointer * pPointerStart,
            CMarkupPointer * pPointerFinish );
    
    HRESULT ParseGlobal (
            HGLOBAL          hGlobal,
            DWORD            dwFlags,
            CMarkup * *      ppContainerResult,
            CMarkupPointer * pPointerStart,
            CMarkupPointer * pPointerFinish );

    HRESULT ValidateElements (
            CMarkupPointer *   pPointerStart,
            CMarkupPointer *   pPointerFinish,
            CMarkupPointer *   pPointerTarget,
            DWORD              dwFlags,
            CMarkupPointer * * ppPointerStatus,
            CTreeNode * *      pNodeFailBottom,
            CTreeNode * *      pNodeFailTop );

    HRESULT GetElementTagId (
            CElement *       pElement,
            ELEMENT_TAG_ID * ptagId );

    //
    // IHTMLViewServices methods
    //

    NV_DECLARE_TEAROFF_METHOD(MoveMarkupPointerToPoint, movemarkuppointertopoint, (
                POINT pt,
                IMarkupPointer *pPointer,
                BOOL * pfNotAtBOL,
                BOOL * pfAtLogicalBOL,
                BOOL * pfRightOfCp, 
                BOOL fScrollIntoView));
    NV_DECLARE_TEAROFF_METHOD(MoveMarkupPointerToPointEx, movemarkuppointertopointex, (
                POINT pt,
                IMarkupPointer *pPointer,
                BOOL fGlobalCoordinates,
                BOOL * pfNotAtBOL,
                BOOL * pfAtLogicalBOL,
                BOOL * pfRightOfCp, 
                BOOL fScrollIntoView));
    NV_DECLARE_TEAROFF_METHOD(MoveMarkupPointerToMessage, movemarkuppointertomessage, (
                IMarkupPointer *pPointer,
                SelectionMessage *pMessage,
                BOOL * pfNotAtBOL,
                BOOL * pfAtLogicalBOL,
                BOOL * pfRightOfCp,
                BOOL * pfValidTree,
                BOOL fScrollIntoView,
                IHTMLElement* pIContainerElement, 
                BOOL* pfSameLayout,
                BOOL  fHitTestEndOfLine ));
    NV_DECLARE_TEAROFF_METHOD(GetCharFormatInfo, getcharformatinfo, (
                IMarkupPointer *pPointer,
                WORD family,
                HTMLCharFormatData *pInfo));
    NV_DECLARE_TEAROFF_METHOD(GetLineInfo, getlineinfo, (
                IMarkupPointer *pPointer,
                BOOL fAtEndOfLine,
                HTMLPtrDispInfoRec *pInfo));
    NV_DECLARE_TEAROFF_METHOD( IsPointerBetweenLines, ispointerbetweenlines, (
                IMarkupPointer* pPointer,
                BOOL* pfBetweenLines ));
    NV_DECLARE_TEAROFF_METHOD(GetElementsInZOrder, getelementsinzorder, (
                POINT pt,
                IHTMLElement **rgElements,
                DWORD *pCount));
    NV_DECLARE_TEAROFF_METHOD(GetTopElement, gettopelement, (
                POINT pt,
                IHTMLElement **ppElement));
    NV_DECLARE_TEAROFF_METHOD(MoveMarkupPointer, movemarkuppointer, (
                IMarkupPointer *pPointer,
                LAYOUT_MOVE_UNIT eUnit,
                LONG lXCurReally,
                BOOL * pfNotAtBOL,
                BOOL * pfAtLogicalBOL));
    NV_DECLARE_TEAROFF_METHOD(RegionFromMarkupPointers, regionfrommarkuppointers, (
                IMarkupPointer *pPointerStart,
                IMarkupPointer *pPointerEnd,
                HRGN *phrgn));

    NV_DECLARE_TEAROFF_METHOD ( GetCurrentSelectionRenderingServices, getcurrentselectionrenderingservices, (
        ISelectionRenderingServices ** ppSelRenSvc ));

    NV_DECLARE_TEAROFF_METHOD(GetCurrentSelectionSegmentList, getcurrentsegmentlist , (
                ISegmentList **ppSegmentList ));


    NV_DECLARE_TEAROFF_METHOD(FireOnSelectStart, fireonselectstart , (
                IHTMLElement* pIElement )); 

    NV_DECLARE_TEAROFF_METHOD(FireCancelableEvent, firecancelableevent, (
                IHTMLElement * pIElement,
                LONG dispidMethod,
                LONG dispidProp,
                BSTR bstrEventType,
                BOOL * pfResult));

    NV_DECLARE_TEAROFF_METHOD( GetCaret, getcaret, (
            IHTMLCaret ** ppCaret ));

    NV_DECLARE_TEAROFF_METHOD(ConvertVariantFromHTMLToTwips,
                convertvariantfromhtmltotwips,
                (VARIANT *pvar));

    NV_DECLARE_TEAROFF_METHOD(ConvertVariantFromTwipsToHTML,
                convertvariantfromtwipstohtml,
                (VARIANT *pvar));

    NV_DECLARE_TEAROFF_METHOD( IsBlockElement, isblockelement, (
            IHTMLElement * pIElement,
            BOOL * pfResult ));

    NV_DECLARE_TEAROFF_METHOD( IsLayoutElement, islayoutelement, (
            IHTMLElement * pIElement,
            BOOL * pfResult ));

    NV_DECLARE_TEAROFF_METHOD( IsContainerElement, iscontainerelement, (
            IHTMLElement * pIElement,
            BOOL         * pfContainer,
            BOOL         * pfHTML) );

    NV_DECLARE_TEAROFF_METHOD( GetFlowElement, getflowelement, (
            IMarkupPointer * pIPointer,
            IHTMLElement  ** ppIElement ));

    NV_DECLARE_TEAROFF_METHOD( GetElementFromCookie, getelementfromcookie , (
            void* elementCookie,
            IHTMLElement  ** ppIElement ));


    NV_DECLARE_TEAROFF_METHOD(
        AddElementAdorner, addelementadorner, (
                            IHTMLElement* pElement,
                            IElementAdorner * pAdorner,
                            DWORD_PTR* ppeCookie ) ) ;

    NV_DECLARE_TEAROFF_METHOD(
        RemoveElementAdorner, removeelementadorner, ( DWORD_PTR eCookie ) ) ;

    NV_DECLARE_TEAROFF_METHOD(
        GetElementAdornerBounds, getelementadornerbounds, ( DWORD_PTR eCookie ,RECT* pRectBounds ) ) ;

    NV_DECLARE_TEAROFF_METHOD( InflateBlockElement, inflateblockelement, (
            IHTMLElement * pElem ));

    NV_DECLARE_TEAROFF_METHOD( IsInflatedBlockElement, isinflatedblockelement, (
            IHTMLElement * pElem,
            BOOL * pfIsInflated ));

    NV_DECLARE_TEAROFF_METHOD( IsMultiLineFlowElement, ismultilineflowelement, (
            IHTMLElement * pIElement,
            BOOL         * fMultiLine ));

    NV_DECLARE_TEAROFF_METHOD( GetElementAttributeCount, getelementattributecount, (
            IHTMLElement * pIElement, 
            UINT         * pCount ));

    NV_DECLARE_TEAROFF_METHOD( IsEditableElement, iseditableelement, (
            IHTMLElement * pIElement,
            BOOL * pfResult ));
    NV_DECLARE_TEAROFF_METHOD( GetOuterContainer, getoutercontainer , ( 
            IHTMLElement* pIElement, 
            IHTMLElement** ppIOuterElement, 
            BOOL fIgnoreOutermostContainer, 
            BOOL * pfHitContainer    ));        

   NV_DECLARE_TEAROFF_METHOD( IsNoScopeElement, isnoscopeelement, (
            IHTMLElement * pIElement,
            BOOL * pfResult ));

    NV_DECLARE_TEAROFF_METHOD( ShouldObjectHaveBorder, shouldobjecthaveborder , (
            IHTMLElement * pIElement,
            BOOL * pfResult ));

    NV_DECLARE_TEAROFF_METHOD( DoTheDarnPasteHTML, dothedarnpastehtml , ( 
            IMarkupPointer*,  IMarkupPointer*, HGLOBAL ));

    NV_DECLARE_TEAROFF_METHOD( ConvertRTFToHTML, convertrtftohtml, (
            LPOLESTR pszRtf, HGLOBAL* phglobalHTML));

    NV_DECLARE_TEAROFF_METHOD( GetViewHWND, getviewhwnd , (
            HWND* pHWND ));

    NV_DECLARE_TEAROFF_METHOD( GetActiveIMM, getactiveimm, (
            IActiveIMMApp** ppActiveIMM ));

    NV_DECLARE_TEAROFF_METHOD( IsRtfConverterEnabled, isrtfconvertenabled, (
            BOOL* pfEnabled ));

    NV_DECLARE_TEAROFF_METHOD( ScrollElement, scrollelement, ( 
            IHTMLElement* pIElement, 
            LONG lPercentToScroll, 
            POINT * pScrollDelta    ));

    NV_DECLARE_TEAROFF_METHOD( GetScrollingElement, getscrollingelement, (
            IMarkupPointer* pPosition,
            IHTMLElement* pBoundary,
            IHTMLElement** ppElement ));
            
    NV_DECLARE_TEAROFF_METHOD( StartHTMLEditorDblClickTimer, starthtmleditordblclicktimer , ());
    NV_DECLARE_TEAROFF_METHOD( StopHTMLEditorDblClickTimer, stophtmleditordblclicktimer , ());
    NV_DECLARE_TEAROFF_METHOD( HTMLEditorTakeCapture, htmleditortakecapture , ());
    NV_DECLARE_TEAROFF_METHOD( HTMLEditorReleaseCapture, htmleditorreleasecapture , ());  
    NV_DECLARE_TEAROFF_METHOD( SetHTMLEditorMouseMoveTimer, sethtmleditormousemovetimer , ()); 
   
    NV_DECLARE_TEAROFF_METHOD( GetEditContext, geteditcontext, ( 
                                                                IHTMLElement* pIStartElement, 
                                                                IHTMLElement** ppIEditThisElement, 
                                                                IMarkupPointer* pIStart, 
                                                                IMarkupPointer* pIEnd, 
                                                                BOOL fDrillingIn,  
                                                                BOOL * pfEditThisEditable ,
                                                                BOOL * pfEditParentEditable ,
                                                                BOOL * pfNoScopeElement));                                                                  

    NV_DECLARE_TEAROFF_METHOD( EnsureEditContext, ensureeditcontext, ( IMarkupPointer* pIPointer ));
                                                                              
    NV_DECLARE_TEAROFF_METHOD( ScrollPointerIntoView, scrollpointerintoview, (
            IMarkupPointer * pPointer,
            BOOL fNotAtBOL,
            POINTER_SCROLLPIN eScrollAmount ));

    NV_DECLARE_TEAROFF_METHOD( ScrollPointIntoView, scrollpointintoview, (
            IHTMLElement* pIElement,
            POINT* ptGlobal  ));
            
    NV_DECLARE_TEAROFF_METHOD( ArePointersInSameMarkup, arepointersinsamemarkup, ( 
            IMarkupPointer * pP1, 
            IMarkupPointer * pP2, 
            BOOL * pfSameMarkup ));

    NV_DECLARE_TEAROFF_METHOD( DragElement, dragelement, ( 
            IHTMLElement* pIElement, 
            DWORD dwKeyState ));

    NV_DECLARE_TEAROFF_METHOD( BecomeCurrent, becomecurrent , ( 
            IHTMLElement* pIElement,
            SelectionMessage* pSelMessage));

#if TREE_SYNC
    //
    // IMarkupSyncServices methods
    //
    // this is a big HACK, to temporarily get markup-sync working for netdocs.  it
    // will totally change in the future.
    //

    NV_DECLARE_TEAROFF_METHOD( GetCpFromPointer, getcpfrompointer, (
            IMarkupPointer * pIPointer,
            long * pcp ));

    NV_DECLARE_TEAROFF_METHOD( MovePointerToCp, movepointertocp, (
            IMarkupPointer * pIPointer,
            long cp,
            IMarkupContainer * pIContainer ));

    NV_DECLARE_TEAROFF_METHOD( RegisterLogSink, registerlogsink, (
            IMarkupSyncLogSink * pILogSink ));

    NV_DECLARE_TEAROFF_METHOD( UnregisterLogSink, unregisterlogsink, (
            IMarkupSyncLogSink * pILogSink ));

#endif // TREE_SYNC


    NV_DECLARE_TEAROFF_METHOD( TransformPoint, transformpoint, (
        POINT        * pPoint,
        COORD_SYSTEM eSource,
        COORD_SYSTEM eDestination,
        IHTMLElement * pIElement ));

    NV_DECLARE_TEAROFF_METHOD( GetElementDragBounds, getelementdragbounds, (
        IHTMLElement* pIElement,
        RECT* pIElementDragBounds));
  
    NV_DECLARE_TEAROFF_METHOD( UpdateAdorner, updateadorner, (
        DWORD_PTR eAdornerCookie));

    NV_DECLARE_TEAROFF_METHOD( InvalidateAdorner, invalidateadorner, (
        DWORD_PTR eAdornerCookie));

    NV_DECLARE_TEAROFF_METHOD( ScrollIntoView, scrollintoview, ( 
        DWORD_PTR peCookie));        
        
    NV_DECLARE_TEAROFF_METHOD( IsElementLocked, iselementlocked, (
        IHTMLElement* pIElement, 
        BOOL* pfLocked));      

    NV_DECLARE_TEAROFF_METHOD(GetLineDirection, getlinedirection, (
                IMarkupPointer *pPointer,
                BOOL fAtEndOfLine,
                long *peHTMLDir));

#ifdef XMV_PARSE
    //
    // IXMLGenericParse methods
    //

    NV_DECLARE_TEAROFF_METHOD(SetGenericParse, setgenericparse, (
                 VARIANT_BOOL fDoGeneric));
#endif // XMV_PARSE
        
    NV_DECLARE_TEAROFF_METHOD( MakeParentCurrent, makeparentcurrent, (
        IHTMLElement* pIElement ));
        

    NV_DECLARE_TEAROFF_METHOD(FireOnBeforeEditFocus, fireonbeforeeditfocus, (
                IHTMLElement *pINextActiveElem,
                BOOL *pfRet));
                
    NV_DECLARE_TEAROFF_METHOD( IsEqualElement, isequalelement, (
        IHTMLElement* pIElement1, IHTMLElement* pIElement2));                                                         

    NV_DECLARE_TEAROFF_METHOD( GetOuterMostEditableElement, getoutermosteditableelement, ( 
        IHTMLElement * pElement, 
        IHTMLElement ** ppIOuterElement ));
                                                      
    NV_DECLARE_TEAROFF_METHOD( IsSite, issite, ( 
        IHTMLElement * pElement, 
        BOOL* pfSite, 
        BOOL* pfText, 
        BOOL* pfMultiLine, 
        BOOL* pfScrollable ));

    NV_DECLARE_TEAROFF_METHOD( QueryBreaks, querybreaks, (
        IMarkupPointer* pPointer,
        DWORD* pdwBreaks,
        BOOL   fWantPendingBreak));

    NV_DECLARE_TEAROFF_METHOD( MergeDeletion, mergedeletion, (
        IMarkupPointer* pPointer ));

    NV_DECLARE_TEAROFF_METHOD( GetElementForSelection, getelementforselection, ( 
        IHTMLElement * pElement, 
        IHTMLElement** ppISiteSelectableElement ));

    NV_DECLARE_TEAROFF_METHOD( IsContainedBy, iscontainedby, ( IMarkupContainer* pIOuterContainer, 
                                                               IMarkupContainer* pIInnerContainer ));

#if DBG == 1
    //
    // IEditDebugServices Methods
    //

    NV_DECLARE_TEAROFF_METHOD ( GetCp, getcp, ( IMarkupPointer* pIPointer,
                                                long* pcp));
                                                
    NV_DECLARE_TEAROFF_METHOD( SetDebugName, setdebugname, ( IMarkupPointer* pIPointer, LPCTSTR strDebugName ));

    NV_DECLARE_TEAROFF_METHOD( DumpTree , dumptree, ( IMarkupPointer* pIPointer));
#endif // IEditDebugServices

    NV_DECLARE_TEAROFF_METHOD( CurrentScopeOrSlave, currentscopeorslave, (
        IMarkupPointer* pPointer, 
        IHTMLElement** ppElemCurrent ));
        
    NV_DECLARE_TEAROFF_METHOD( LeftOrSlave, leftorslave, (
        IMarkupPointer * pPointer,
        BOOL fMove,
        MARKUP_CONTEXT_TYPE *pContext,
        IHTMLElement** ppElement,
        long *pcch,
        OLECHAR* pchText));
        
    NV_DECLARE_TEAROFF_METHOD( RightOrSlave, rightorslave, (
        IMarkupPointer * pPointer,
        BOOL fMove,
        MARKUP_CONTEXT_TYPE *pContext,
        IHTMLElement** ppElement,
        long *pcch,
        OLECHAR* pchText));
        
    NV_DECLARE_TEAROFF_METHOD( MoveToContainerOrSlave, movetocontainerorslave, (
        IMarkupPointer *pPointer,
        IMarkupContainer* pContainer,
        BOOL fAtStart));

    NV_DECLARE_TEAROFF_METHOD( MergeAttributes, mergeattributes, (
                IHTMLElement *pIHTMLElementMergeSrc,
                IHTMLElement *pIHTMLElementMergeTarget,
                BOOL fCopyId));

    NV_DECLARE_TEAROFF_METHOD( FindUrl, findurl, (
                IMarkupPointer* pStart, 
                IMarkupPointer* pEnd,   
                IMarkupPointer* pUrlStart, 
                IMarkupPointer* pUrlEnd ));
                
    NV_DECLARE_TEAROFF_METHOD( IsEnabled, isenabled , (
                IHTMLElement *pIHTMLElement,
                BOOL *pfEnabled ));

    NV_DECLARE_TEAROFF_METHOD(
        GetElementBlockDirection, getelementblockdirection, (
                IHTMLElement * pElement,
                BSTR * pbstrDir));

    NV_DECLARE_TEAROFF_METHOD(
        SetElementBlockDirection, setelementblockdirection, (
                IHTMLElement * pElement,
                LONG eHTMLDir));

    NV_DECLARE_TEAROFF_METHOD( IsBidiEnabled, isbidienabled , (
                BOOL *pfEnabled ));

    NV_DECLARE_TEAROFF_METHOD( SetDocDirection, setdocdirection , (
                LONG eHTMLDir));

    NV_DECLARE_TEAROFF_METHOD( AllowSelection , allowselection , (
                IHTMLElement *pIHTMLElement,
                SelectionMessage* peMessage ));

    NV_DECLARE_TEAROFF_METHOD( MoveWord, moveword, (
                       IMarkupPointer * pPointerToMove,
                       MOVEUNIT_ACTION  muAction,
                       IMarkupPointer * pLeftBoundary,
                       IMarkupPointer * pRightBoundary ));

    NV_DECLARE_TEAROFF_METHOD( GetAdjacencyForPoint, getadjacencyforpoint, ( 
                                    IHTMLElement* pIElement, 
                                    POINT ptGlobal, 
                                    ELEMENT_ADJACENCY *peAdjacent  ));

#ifndef UNIX
    NV_DECLARE_TEAROFF_METHOD( SaveSegmentsToClipboard, savesegmentstoclipboard, (
                       ISegmentList * pSegmentList ));
#else
    NV_DECLARE_TEAROFF_METHOD( SaveSegmentsToClipboard, savesegmentstoclipboard, (
                       ISegmentList * pSegmentList, VARIANTARG * pvarargOut ));
#endif

    NV_DECLARE_TEAROFF_METHOD( InsertMaximumText, insertmaximumtext, (                                    
                                    OLECHAR* pstrText, 
                                    LONG cch,
                                    IMarkupPointer* pMarkupPointer ));
    
    NV_DECLARE_TEAROFF_METHOD( IsInsideURL, isinsideurl, ( IMarkupPointer*, IMarkupPointer*, BOOL * ));
    NV_DECLARE_TEAROFF_METHOD( GetDocHostUIHandler, getdochostuihandler, ( IDocHostUIHandler** ));

    NV_DECLARE_TEAROFF_METHOD( GetClientRect, getclientrect, ( IHTMLElement* pIElement,
                                                               COORD_SYSTEM eSource,
                                                               RECT* pRect  ) );

    NV_DECLARE_TEAROFF_METHOD( GetContentRect, getcontentrect, ( IHTMLElement* pIElement,
                                                               COORD_SYSTEM eSource,
                                                               RECT* pRect  ) );
    NV_DECLARE_TEAROFF_METHOD( IsElementSized, iselementsized, ( IHTMLElement* pIElement, 
                                                                 BOOL* pfSized ));
    // Binding service helper
    //---------------------------------------------
    void GetWindowForBinding(HWND * phwnd);


    // Palette helpers
    //---------------------------------------------
    HPALETTE GetPalette(HDC hdc = 0, BOOL *pfHtPal = NULL);
    HRESULT UpdateColors(CColorInfo *pCI);
    HRESULT GetColors(CColorInfo *pCI);
    void InvalidateColors();
    NV_DECLARE_ONCALL_METHOD(OnRecalcColors, onrecalccolors, (DWORD_PTR dwContext));

    // focus/default site helper

    BOOL        TakeFocus();
    BOOL        HasFocus();
    HRESULT     InvalidateDefaultSite();

    // HDL helper, temporary
    CAttrArray **GetAttrArray() const { return CBase::GetAttrArray(); }

    #define _CDoc_
    #include "document.hdl"

    //
    // baseimplementation overrides
    //
    NV_DECLARE_TEAROFF_METHOD(get_bgColor, GET_bgColor, (VARIANT * p));
    NV_DECLARE_TEAROFF_METHOD(put_bgColor, PUT_bgColor, (VARIANT p));
    NV_DECLARE_TEAROFF_METHOD(get_fgColor, GET_fgColor, (VARIANT * p));
    NV_DECLARE_TEAROFF_METHOD(put_fgColor, PUT_fgColor, (VARIANT p));
    NV_DECLARE_TEAROFF_METHOD(get_linkColor, GET_linkColor, (VARIANT * p));
    NV_DECLARE_TEAROFF_METHOD(put_linkColor, PUT_linkColor, (VARIANT p));
    NV_DECLARE_TEAROFF_METHOD(get_alinkColor, GET_alinkColor, (VARIANT * p));
    NV_DECLARE_TEAROFF_METHOD(put_alinkColor, PUT_alinkColor, (VARIANT p));
    NV_DECLARE_TEAROFF_METHOD(get_vlinkColor, GET_vlinkColor, (VARIANT * p));
    NV_DECLARE_TEAROFF_METHOD(put_vlinkColor, PUT_vlinkColor, (VARIANT p));
    NV_DECLARE_TEAROFF_METHOD(put_dir, PUT_dir, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_dir, GET_dir, (BSTR*p));

    // for faulting (JIT) in USP10
    NV_DECLARE_ONCALL_METHOD(FaultInUSP, faultinusp, (DWORD_PTR));
    NV_DECLARE_ONCALL_METHOD(FaultInJG, faultinjg, (DWORD_PTR));

    //
    //  Constructors and destructors
    //

    // CHROME
    // Additional flag added to indicate that the document is being created by the private Chrome
    // CLASID

    enum DOCTYPE
    {
        DOCTYPE_NORMAL          =   0,
        DOCTYPE_FULLWINDOWEMBED =   1,
        DOCTYPE_MHTML           =   2,
        DOCTYPE_CHROME          =   3,
        DOCTYPE_HTA             =   4,
        DOCTYPE_SERVER          =   5,
    };

    CDoc(IUnknown * pUnkOuter, DOCTYPE doctype = DOCTYPE_NORMAL);
    virtual ~CDoc();

    virtual HRESULT Init();
    void SetLoadfFromPrefs();
#ifdef WIN16
    void AddToHistoryStg(TCHAR *pstrTitle = NULL);

#endif // WIN16

    HRESULT CreateElement (
        ELEMENT_TAG etag, CElement * * ppElementNew,
        TCHAR * pch = NULL, long cch = 0 );

    //
    // Called to traverse the site hierarchy
    //   call Notify on registered elements and on super
    //

    void    BroadcastNotify(CNotification *pNF);
    void    ClearCachedDialogs ();

    //
    //  CBase overrides
    //

    virtual BOOL DesignMode() { return _fDesignMode; }

    virtual CAtomTable * GetAtomTable (BOOL *pfExpando = NULL)
       { if (pfExpando) *pfExpando = _fExpando; return &_AtomTable; }

    //  Group helper
    HRESULT DocTraverseGroup(
                LPCTSTR strGroupName,
                PFN_VISIT pfn,
                DWORD_PTR dw,
                BOOL  fForward);

    CElement * FindDefaultElem(BOOL fDefault, BOOL fCurrent=FALSE);

    HRESULT HandleKeyNavigate(CMessage * pmsg, BOOL fAccessKeyCycle);

    // Message Helpers

    HRESULT __cdecl ShowMessage(                
                int   * pnResult,
                DWORD dwFlags,
                DWORD dwHelpContext,
                UINT  idsMessage, ...);
       
    HRESULT ShowMessageV(               
                int   * pnResult,
                DWORD   dwFlags,
                DWORD   dwHelpContext,
                UINT    idsMessage,
                void  * pvArgs);                   
    
    HRESULT ShowMessageEx(                
                int   * pnResult,
                DWORD   dwStyle,
                TCHAR * pstrHelpFile,
                DWORD   dwHelpContext,
                TCHAR * pstrText);        

     HRESULT ShowLastErrorInfo(HRESULT hr, int iIDSDefault=0);   

    // Help helpers
    HRESULT ShowHelp(TCHAR * szHelpFile, DWORD dwData, UINT uCmd, POINT pt);       

    // Ambient Property Helpers

    void GetLoadFlag(DISPID dispidProp);

    BOOL IsFrameOffline(DWORD *pdwBindf = NULL);
    BOOL IsOffline();

    // Helper function to return script errors.
    HRESULT ReportScriptError(ErrorRecord &errRecord);

    //
    //  CServer overrides
    //

    const CBase::CLASSDESC *GetClassDesc() const;

    virtual HRESULT EnsureInPlaceObject();
    virtual void Passivate();
    virtual HRESULT OnPropertyChange(DISPID, DWORD);
    virtual HRESULT Draw(CDrawInfo *pDI, RECT * prc);
    HRESULT DoTranslateAccelerator(LPMSG lpmsg);
    HRESULT FireEventHelper(DISPID dispidMethod, DISPID dispidProp,
                      BYTE * pbTypes, ...);
    HRESULT FireEvent(DISPID dispidMethod, DISPID dispidProp,
                      LPCTSTR pchEventType,
                      BYTE * pbTypes, ...);

    //  IOleInPlaceActiveObject methods
    HRESULT STDMETHODCALLTYPE EnableModeless(BOOL fEnable);
    HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(BOOL fActivate);
    HRESULT STDMETHODCALLTYPE OnDocWindowActivate(BOOL fActivate);

    HRESULT STDMETHODCALLTYPE ResizeBorder(LPCOLERECT lprc,
                                 LPOLEINPLACEUIWINDOW pUIWindow,
                                 BOOL fFrameWindow);
    HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG lpmsg);

    // Helper function
    HRESULT HostTranslateAccelerator(LPMSG lpmsg);
    HRESULT FireAccesibilityEvents(DISPID dispidEvent, long lElemID);

    //
    //  Callbacks
    //

    //  State transition callbacks

    virtual HRESULT RunningToLoaded(void);
    virtual HRESULT RunningToInPlace(LPMSG lpmsg);
    virtual HRESULT InPlaceToRunning(void);
    virtual HRESULT UIActiveToInPlace(void);
    virtual HRESULT InPlaceToUIActive(LPMSG lpmsg);

    virtual HRESULT AttachWin(HWND hwndParent, RECT * prc, HWND * phWnd);
    virtual void    DetachWin();

    DECLARE_GET_SET_METHODS(CDoc, MouseIcon)

    //
    //  Internal
    //

    void UnloadContents( BOOL fPrecreated, BOOL fRestartLoad );

    struct LOADINFO
    {
        IStream *       pstm;           // [S] If loading from a stream
        BOOL            fSync;          // [S] If loading stream synchonously
        IStream *       pstmDirty;      // [D] If loading from dirty stream
        TCHAR *         pchFile;        // [F] If loading from a file
        IMoniker *      pmk;            // [M] If loading from a moniker
        TCHAR *         pchDisplayName; // [M] Display name (URL)
        IBindCtx *      pbctx;          // [M] Bind context for moniker
        CDwnPost *      pDwnPost;       // [M] Post data for moniker
        CDwnBindData *  pDwnBindData;   // [R] Ongoing binding
        IStream *       pstmLeader;     // [R] Initial bytes before continuing pDwnBindData
        DWORD           dwRefresh;      // Refresh level
        DWORD           dwBindf;        // Initial bind flags
        BOOL            fKeepOpen;      // TRUE if processing document.open
        IStream *       pstmHistory;    // Stream which contains history substreams
        TCHAR *         pchHistoryUserData; // string which contains the userdata information
        FILETIME        ftHistory;      // Last-mod date for history
        TCHAR *         pchDocReferer;  // Referer for the document download
        TCHAR *         pchSubReferer;  // Referer for document sub downloads
        MIMEINFO *      pmi;            // MIMEINFO if processing document.open
        CODEPAGE        codepageURL;    // cp of URL (not necessarily doc's)
        CODEPAGE        codepage;       // intial cp of document (may be overridden by bindctx, meta, etc)
        TCHAR *         pchFailureUrl;  // Url to load in case of failure
        IStream *       pstmRefresh;    // Stream for refresh if history post fails from cache
        BOOL            fKeepRefresh;   // If pstmRefresh should be kept on success
        ULONG           cbRequestHeaders; // Length of the following field
        BYTE *          pbRequestHeaders; // Raw request headers to use (ascii bytes)
        BOOL            fBindOnApt;     // Force bind on apartment
        BOOL            fUnsecureSource;// Source is not secure (even if URL is https)
        BOOL            fHyperlink;     // A true hyperlink rather than an initial subframe load (for cache early-attach)
        BOOL            fNoMetaCharset; // Override (ignore) the META charset
        BOOL            fRTLDocDirection; // The document direction that the user last had the page in is right to left
    };

    HRESULT     LoadFromInfo(struct LOADINFO * pLoadInfo);
    void        InitDownloadInfo(DWNLOADINFO * pdli);

    HRESULT     QueryVersionHost();

    HRESULT     canPaste(VARIANT_BOOL * pfCanPaste);
    HRESULT     paste();
    HRESULT     cut();
    HRESULT     copy();

    HRESULT     SetDataObjectSecurity(IDataObject * pDataObj);
    HRESULT     CheckDataObjectSecurity(IDataObject * pDataObj);
    HRESULT     AllowPaste(IDataObject * pDO);
    HRESULT     SetClipboard(IDataObject * pDO);


    HRESULT     NavigateOutside(DWORD grfHLNF, LPCWSTR wzJumpLocation);
    HRESULT     NavigateHere   (DWORD   grfHLNF,
                                LPCWSTR wzJumpLocation,
                                DWORD   dwScrollPos,
                                BOOL    fScrollBits);
    void        NavigateNow(BOOL fScrollBits);

    HRESULT     AddToFavorites(TCHAR * pszURL, TCHAR * pszTitle);
    HRESULT     InvokeEditor(LPCTSTR pszFileName);

    //
    // IPersistHistory support
    //

    HRESULT     GetLoadHistoryStream(ULONG index, DWORD dwCheck, IStream **ppStream);
    void        ClearLoadHistoryStreams();

#if DBG == 1
    void UpdateDocTreeVersion ( );
    void UpdateDocContentsVersion ( );
#else
    void UpdateDocTreeVersion ( ) { __lDocTreeVersion++; UpdateDocContentsVersion(); }
    void UpdateDocContentsVersion ( ) { __lDocContentsVersion++; }
#endif

    HRESULT EnsureHostStyleSheets();
    

    //  Site management

    HTC HitTestPoint(
        CMessage *   pMessage,
        CTreeNode ** ppNodeElement,
        DWORD        dwFlags );

    NV_DECLARE_ONCALL_METHOD ( OnControlInfoChanged, oncontrolinfochanged, (DWORD_PTR) );

    void            AmbientPropertyChange(DISPID dispidProp);

    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, LPVOID * ppv));
    HRESULT         CreateService(REFGUID guidService, REFIID iid, LPVOID * ppv);

    HRESULT         GetUniqueIdentifier(CStr *pstr);

    CElement *      GetPrimaryElementClient();
    CElement *      GetPrimaryElementTop();

    HRESULT         GetOmWindow (LONG nFrame, IHTMLWindow2 ** ppOmWindow2);
    HRESULT         GetFramesCount (LONG * pcFrames);

    HRESULT         SetHostUIHandler(IOleClientSite * pClientSite);

    LPCTSTR         GetPluginContentType() { return (_cstrPluginContentType); }
    LPCTSTR         GetPluginCacheFilename() { return (_cstrPluginCacheFilename); }
    BOOL            IsFullWindowEmbed() { return (_fFullWindowEmbed); }

    //  Coordinates

    void            DocumentFromWindow(POINTL *pptlDocOut, long xWinIn, long yWinIn);
    void            DocumentFromWindow(POINTL *pptlDocOut, POINT pptWinIn);
    void            DocumentFromWindow(SIZEL *psizelDocOut, SIZE sizeWinIn);
#ifdef WIN16
    void            DocumentFromWindow(SIZEL *pptlDocOut, SIZES ptWinIn);
#endif
    void            DocumentFromWindow(SIZEL *psizelDocOut, long cxWinin, long cyWinIn);
    void            DocumentFromWindow(RECTL  *prclDocOut, const RECT *prcWinIn);

    void            DocumentFromScreen(POINTL *pptlDocOut,   POINT pptScreenIn);

    void            ScreenFromWindow (POINT *ppt, POINT pt);

    void            HimetricFromDevice(POINTL *pptlDocOut, int xWinIn, int yWinIn);
    void            HimetricFromDevice(POINTL *pptlDocOut, POINT pptWinIn);
    void            HimetricFromDevice(SIZEL *psizelDocOut, SIZE sizeWinIn);
    void            HimetricFromDevice(SIZEL *psizelDocOut, int cxWinin, int cyWinIn);
    void            HimetricFromDevice(RECTL  *prclDocOut, const RECT *prcWinIn);

    void            DeviceFromHimetric(POINT *pptWinOut,  int xlDocIn, int ylDocIn);
    void            DeviceFromHimetric(POINT *pptWinOut,  POINTL pptlDocIn);
    void            DeviceFromHimetric(RECT *prcWinOut,   const RECTL *prclDocIn);
    void            DeviceFromHimetric(SIZE *psizeWinOut, SIZEL sizelDocIn);
    void            DeviceFromHimetric(SIZE *psizeWinOut, int cxlDocIn, int cylDocIn);

    //  Rendering

    void            Invalidate();
    void            Invalidate(const RECT *prc, const RECT *prcClip, HRGN hrgn, DWORD dwFlags);
    void            UpdateForm();
    void            UpdateInterval(LONG interval);
    LONG            GetUpdateInterval();
                    // Tristate property used for Set/GetoffscreenBuffering
                    // from Object Model for Nav4 compat (auto, true, false).
                    // OM prop overrides what is set thru DOCKHOSTUIFLAG_DISABLE_OFFSCREEN.
    void            SetOffscreenBuffering(BOOL fBuffer)
                            {!fBuffer ? _triOMOffscreenOK=0 : _triOMOffscreenOK=1;}
    INT             GetOffscreenBuffering() {return _triOMOffscreenOK;}

    // UI

    void            DeferUpdateUI();
    void            DeferUpdateTitle();
    void            SetUpdateTimer();
    void            OnUpdateUI();
    void            UpdateTitle();

    //  Persistence

    DWORD           StgMode(void);

    HRESULT         PromptSave(BOOL fSaveAs, BOOL fShowUI = TRUE, TCHAR * pchPathName = NULL);
    HRESULT         PromptSaveImgCtx(const TCHAR * pchCachedFile, MIMEINFO * pmi,
                                     TCHAR * pchFileName, int cchFileName);

    void            SaveImgCtxAs(
                        CImgCtx *   pImgCtx,
                        CBitsCtx *  pBitsCtx,
                        int         iAction,
                        TCHAR *     pchFileName = NULL,
                        int         cchFileName = 0);

    HRESULT         RequestSaveFileName(LPTSTR pszFileName, int cchFileName,
                                        CODEPAGE * pCodePage);

    HRESULT         EnsureDirtyStream(void);

    HRESULT         GetFile(TCHAR **pszFilename);
    HRESULT         GetViewSourceFileName(TCHAR * pszPath);
    HRESULT         SaveSelection(TCHAR *pszFileName);
    HRESULT         SaveSelection(IStream *pstm);
    HRESULT         GetHtmSourceText(ULONG ulStartOffset, ULONG ulCharCount, WCHAR *pOutText, ULONG *pulActualCharCount);
    HRESULT         NewDwnCtx(UINT dt, LPCTSTR pchSrc, CElement * pel, CDwnCtx ** ppDwnCtx, BOOL fSilent = FALSE, DWORD dwProgsinkClass = 0);

    HRESULT         SetDownloadNotify(IUnknown *punk);

    // Security
    HRESULT         EnsureSecurityManager();
    HRESULT         AllowClipboardAccess(TCHAR *pchCmdId, BOOL *pfAllow);
    HRESULT         ProcessURLAction(
                        DWORD dwAction,
                        BOOL *pfReturn,
                        DWORD dwPuaf = 0,
                        DWORD *dwPolicy = NULL,
                        LPCTSTR pchURL = NULL,
                        BYTE *pbArg = NULL,
                        DWORD cbArg = 0,
                        BOOL fDisableNoShowElement = FALSE);
    HRESULT         GetSecurityID(BYTE *pbSID, DWORD *pcb, LPCTSTR pchURL = NULL);
    void            UpdateSecurityID();
    HRESULT         GetActiveXSafetyProvider(IActiveXSafetyProvider **ppProvider);
    HRESULT         GetFrameZone(VARIANT *pVar);
    BOOL            AccessAllowed (LPCTSTR pchUrl);

    // MarkupServices helpers
    
    BOOL IsOwnerOf ( IHTMLElement *     pIElement  );
    BOOL IsOwnerOf ( IMarkupPointer *   pIPointer  );
    BOOL IsOwnerOf ( IHTMLTxtRange *    pIRange    );
    BOOL IsOwnerOf ( IMarkupContainer * pContainer );
    
    HRESULT CutCopyMove (
        IMarkupPointer * pIPointerStart,
        IMarkupPointer * pIPointerFinish,
        IMarkupPointer * pIPointerTarget,
        BOOL             fRemove );
    
    HRESULT CutCopyMove (
        CMarkupPointer * pPointerStart,
        CMarkupPointer * pPointerFinish,
        CMarkupPointer * pPointerTarget,
        BOOL             fRemove,
        DWORD            dwFlags );
    
    HRESULT         CreateMarkup( CMarkup ** ppMarkup, CElement * pElementMaster = NULL );
    HRESULT         CreateMarkupWithElement( CMarkup ** ppMarkup, CElement * pElement, CElement * pElementMaster = NULL );

    // Option settings change

    HRESULT         OnSettingsChange(BOOL fForce=FALSE);

    //  Stop handling

    HRESULT ExecStop(BOOL fFireOnStop = TRUE);

    //  Refresh handling

    virtual HRESULT QueryRefresh(DWORD * pdwFlag);
    virtual HRESULT ExecRefresh(LONG lOleCmdidf = OLECMDIDF_REFRESH_RELOAD);
    NV_DECLARE_ONCALL_METHOD(ExecRefreshCallback, execrefreshcallback, (DWORD_PTR dwOleCmdidf));

    // Meta-charset restart-load handling
    HRESULT RestartLoad(IStream *pstmLeader, CDwnBindData *pDwnBindData, CODEPAGE codepage);

    //  Layout

    void            EnsureFormatCacheChange(DWORD dwFlags);

    //
    //
    //

    HRESULT OnCssChange();
    HRESULT ForceRelayout();

    //  Window message handling

    void            OnPaint();
    BOOL            OnEraseBkgnd(HDC hdc);
#ifndef NO_MENU
    void            OnMenuSelect(UINT uItem, UINT fuFlags, HMENU hmenu);
#endif
    void            OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    void            OnTimer(UINT id);
    HRESULT         OnHelp(HELPINFO *);

    HRESULT         PumpMessage(CMessage * pMessage, CTreeNode * pNodeTarget, BOOL fPerformTA=FALSE);
    HRESULT         PerformTA(CMessage *pMessage);
    BOOL            AreWeSaneAfterEventFiring(CMessage *pMessage, ULONG cDie);
    BOOL            FCaretInPre();

    HRESULT         OnMouseMessage(UINT, WPARAM, LPARAM, LRESULT *, int, int);
    NV_DECLARE_ONTICK_METHOD(DetectMouseExit, detectmouseexit, ( UINT uTimerID));
    void            DeferSetCursor();
    NV_DECLARE_ONCALL_METHOD(SendSetCursor, SendSetCursor, (DWORD_PTR));

    // Handle the accessibility WM_GETOBJECT message.
    void OnAccGetObject          (UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);
    void OnAccGetObjectInContext (UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

    // Accessibility: Should we move system caret when focus moves?
    BOOL MoveSystemCaret();

    // REgister/revoke drag-drop as appropriate
    NV_DECLARE_ONCALL_METHOD(EnableDragDrop, enabledragdrop, (DWORD_PTR));

    // Set a new capture object, and/or clear an old one and send it a
    //  WM_CAPTURECHANGED message
    void SetMouseCapture(PFN_VOID_MOUSECAPTURE pfnTo, void *pvObject, BOOL fElement=TRUE);
    // Clear old capture object with no notification
    // (properly used only by the old capture object itself)
    void ClearMouseCapture(void *pvObject);
    void *GetCaptureObject( ) { return _pvCaptureObject; }

    // Misc helpers
    CDoc *GetRootDoc();
    CDoc *GetTopDoc();
    BOOL IsRootDoc() { return(!_pDocParent); }
    void TerminateLookForBookmarkTask();

    HRESULT OnFrameOptionScrollChange();

protected:
    static UINT _g_msgHtmlGetobject;     // Registered window message for WM_HTML_GETOBJECT
#if !defined(NO_IME) && defined(IME_RECONVERSION)
    static UINT _g_msgImeReconvert;      // for IME reconversion.
#endif // !NO_IME

private:
    DWORD GetTargetTime(DWORD dwTimeoutLenMs) { return GetTickCount() + dwTimeoutLenMs;}
    HRESULT ExecuteTimeoutScript(TIMEOUTEVENTINFO * pTimeout);

public:

    HRESULT         SetUIActiveElement(CElement *pElemNext);
    HRESULT         SetCurrentElem(CElement *pElemNext, long lSubNext, BOOL *pfYieldFailed = NULL);
    void BUGCALL    DeferSetCurrency(DWORD_PTR dwContext);

#ifndef NO_OLEUI
    virtual HRESULT InstallFrameUI();
    virtual void    RemoveFrameUI();
#endif // NO_OLEUI

#if DBG == 1
    HRESULT         InsertSharedMenus(
            HMENU hmenuShared,
            HMENU hmenuObject,
            LPOLEMENUGROUPWIDTHS lpmgw);
     void            RemoveSharedMenus(
            HMENU hmenuShared,
            LPOLEMENUGROUPWIDTHS lpmgw);

    HRESULT CreateMenuUI();
    HRESULT InstallMenuUI();
    void    DestroyMenuUI();
#endif // DBG==1

    // CCP/Delete

    HRESULT         Delete();

    //  Context menus

    HRESULT         ShowPropertyDialog(int cElements, CElement **apElement);

    HRESULT         ShowErrorDialog(VARIANT_BOOL *pfRet);

    HRESULT         InsertMenuExt(HMENU hMenu, int id);
    HRESULT         GetContextMenu(HMENU *phmenu, int id);
    HRESULT         ShowContextMenu(
                        int x,
                        int y,
                        int id,
                        CElement * pMenuObject);

    HRESULT         ShowDragContextMenu(
                        POINTL ptl,
                        DWORD dwAllowed,
                        int *piSelection,
                        LPTSTR  lptszFileType);

    HRESULT         EditUndo();
    HRESULT         EditRedo();
    HRESULT         EditPasteSpecial(BOOL fCtxtMnu, POINTL ptl);
    HRESULT         EditInsert(void);

    //  Keyboard interface

    void            ReflectDefaultButton(void);

    HRESULT         ActivateDefaultButton(LPMSG lpmsg = NULL);
    HRESULT         ActivateCancelButton(LPMSG lpmsg = NULL);

    HRESULT         CallParentTA(CMessage * pmsg);
    HRESULT         ActivateFirstObject(LPMSG lpmsg);

    //  Z order

    HRESULT         SetZOrder(int zorder);
    void            FixZOrder();

    //  Tools

    HWND            CreateOverlayWindow(HWND hwnd);

    //  Selection tracking and misc selection stuff

    HRESULT         OnSelectChange(void);

    //  Misc

    void            WhereAmI();
    void            SetMouseMoveTimer(UINT uTimeOUt);
    HRESULT         GetBodyElement(IHTMLBodyElement ** ppBody);
    HRESULT         GetBodyElement(CBodyElement **ppBody);

    virtual void    PreSetErrorInfo();

    //  User Option Settings

    HRESULT         UpdateFromRegistry(DWORD dwFlags = 0, BOOL *pfNeedLayout = NULL);
    BOOL            RtfConverterEnabled();

    // Helper functions to read from the registry

    HRESULT         ReadOptionSettingsFromRegistry( DWORD dwFlags = 0);
    HRESULT         ReadCodepageSettingsFromRegistry( CODEPAGE cp, UINT uiFamilyCodePage, DWORD dwFlags = 0 );
    HRESULT         ReadContextMenuExtFromRegistry( DWORD dwFlags = 0 );

    // The following functions should only be called from ReadOptionSettingsFromRegistry or
    //  from ReadCodepageSettingsFromRegistry():
    HRESULT         EnsureOptionSettings();
    HRESULT         EnsureCodepageSettings(CODEPAGE codepage);

    // returns the "parent" frame site if we're hosted in a frameset
    CFrameSite*     ParentFrameSite();

    // returns the "parent" frame site if we're hosted in an IFrame
    CFrameSite*     ParentIFrameSite();

    // Code to set the parent doc pointer
    HRESULT         SetDocParent(LPOLECLIENTSITE pClientSite);

    // Mode helper functions.

    HRESULT         UpdateDesignMode(BOOL fMode);

    HRESULT         SetDesignMode ( htmlDesignMode Mode );

    // focus\blur method helpers for the window object
    HRESULT         WindowBlurHelper();
    HRESULT         WindowFocusHelper();

    // in-line function,
    CFormInPlace * InPlace() { return (CFormInPlace *) _pInPlace; }

    //
    // URL helpers
    //

    // Use this next define as the pElementContext if you don't want to scan
    // the HEAD for <BASE> tags.

    #define         EXPANDIGNOREBASETAGS    ((CElement *)(INT_PTR)(-1))

    HRESULT         SetUrl(const TCHAR *pchUrl, BOOL fKeepDwnPost = FALSE);
    HRESULT         SetFilename(const TCHAR *pchFile);
    void            ClearDwnPost();

    HRESULT         GetBaseUrl(
                        TCHAR  * * ppszBase,
                        CElement * pElementContext = NULL,
                        BOOL     * pfDefault = NULL,
                        TCHAR    * pszAlternateDocUrl = NULL );

    HRESULT         GetBaseTarget(TCHAR ** ppszTarget, CElement *pElementContext);

    HRESULT         ExpandUrl(
                        const TCHAR * pszRel,
                        LONG        dwUrlSize,
                        TCHAR       * ppszUrl,
                        CElement    * pElementContext,
                        DWORD         dwFlags = 0xFFFFFFFF,
                        TCHAR       * pszAlternateDocUrl = NULL );

    BOOL            IsUrlRecursive( TCHAR * pchUrl );
    
    BOOL            ValidateSecureUrl(TCHAR *pchUrlAbsolute, BOOL fReprompt = FALSE, BOOL fSilent = FALSE, BOOL fUnsecureSource = FALSE);
    BOOL            AllowFrameUnsecureRedirect();
    void            OnHtmDownloadSecFlags(DWORD dwFlags);
    void            OnSubDownloadSecFlags(const TCHAR *pchUrl, DWORD dwFlags);
    void            OnIgnoreFrameSslSecurity();
    void            GetRootSslState(SSL_SECURITY_STATE *psss, SSL_PROMPT_STATE *psps);
    void            SetRootSslState(SSL_SECURITY_STATE sss, SSL_PROMPT_STATE sps, BOOL fInit = FALSE);
    void            EnterRootSslPrompt();
    void            LeaveRootSslPrompt();
    BOOL            InRootSslPrompt();
    HRESULT         GetMyDomain(CStr * pstrUrl, CStr * pcstrOut);
    FILETIME        GetLastModDate();

    // Hyperlinking
    HRESULT         FollowHyperlink(
                        LPCTSTR             pchURL,
                        LPCTSTR             pchTarget           = NULL,
                        CElement *          pElementContext     = NULL,
                        CDwnPost *          pDwnPost            = NULL,
                        BOOL                fSendAsPost         = FALSE,
                        BOOL                fOpenInNewWindow    = FALSE,
                        IUnknown *          pUnkFrame           = NULL,
                        DWORD               dwBindOptions       = 0,
                        DWORD               dwSecurityCode      = ERROR_SUCCESS);

    HRESULT         DetermineExpandedUrl(
                        LPCTSTR             pchURL,
                        CElement *          pElementContext,
                        CDwnPost *          pDwnPost,
                        BOOL                fSendAsPost,
                        BOOL                fFrameNavigate,
                        DWORD               dwSecurityCode,
                        CStr *              pcstrExpandedUrl,
                        CStr *              pcstrLocation,
                        BOOL *              pfProtocolNavigates);

    HRESULT         DetermineHyperlinkFrameTarget(
                        IUnknown *          pUnkFrame,
                        CElement *          pElementContext,
                        CStr *              pcstrExpandedUrl,
                        BOOL                fProtocolNavigates,
                        IHlinkFrame **      ppHlinkFrameTarget,
                        LPCTSTR *           ppchTarget,
                        TCHAR **            ppchBaseTarget,
                        TCHAR **            ppchTargetAlias,
                        BOOL *              pfOpenInNewWindow,
                        BOOL *              pfOpenWithFrameName,
                        DWORD *             pdwBindf);

    HRESULT         HlinkFrameTargetFromUnkTarget(
                        IUnknown *          pUnkFrame,
                        IUnknown *          pUnkTarget,
                        CStr *              pcstrExpandedUrl,
                        IHlinkFrame **      ppHlinkFrameTarget,
                        DWORD *             pdwBindf);

    HRESULT         SetupDwnBindInfoAndBindCtx(
                        LPCTSTR             pchExpandedUrl,
                        LPCTSTR             pchTarget,
                        CElement *          pElementContext,
                        CDwnPost *          pDwnPost,
                        BOOL                fSendAsPost,
                        BOOL                fProtocolNavigates,
                        BOOL                fFrameLoad,
                        DWORD *             pdwBindf,
                        CDwnBindInfo **     ppDwnBindInfo,
                        IBindCtx **         ppBindCtx);

    HRESULT         DoNavigate(
                        CStr *              pcstrExpandedUrl,
                        CStr *              pcstrLocation,
                        IHlinkFrame *       pHlinkFrameTarget,
                        CDwnBindInfo *      pDwnBindInfo,
                        IBindCtx *          pBindCtx,
                        LPCTSTR             pchURL,
                        LPCTSTR             pchTarget,
                        IUnknown *          pUnkFrame,
                        BOOL                fOpenInNewWindow,
                        BOOL                fOpenWithFrameName,
                        BOOL                fProtocolNavigates);

    HRESULT         ParseHyperlink(TCHAR *pchURL, CStr *pcstrURL, CStr *pcstrLocation);
    HRESULT         NavigateTargetFrame(
                        IUnknown *pUnkFrame,
                        DWORD dwFlags,
                        IBindCtx *pBindCtx,
                        IBindStatusCallback *pBSC,
                        TCHAR *pchURL,
                        TCHAR *pchLocation,
                        TCHAR *pchName);
    HRESULT         NavigateHlinkFrame(
                        IUnknown *pUnkFrame,
                        DWORD dwFlags,
                        IBindCtx *pBindCtx,
                        IBindStatusCallback *pBSC,
                        TCHAR *pchURL,
                        TCHAR *pchLocation,
                        TCHAR *pchName);
    BOOL            IsFrameInsideWindow(IUnknown *pUnkFrame);
    BOOL            IsVisitedHyperlink(LPCTSTR szURL, CElement *pElementContext);
    BOOL            IsAvailableOffline(LPCTSTR szURL, CElement *pElementContext);
    static TCHAR *  PchUrlLocationPart(LPOLESTR szURL);
    HRESULT         FollowHistory(BOOL fForward);

    // Progress and status UI

    HRESULT         SetReadyState(long readyState);
    void            SetSpin(BOOL fSpin);
    virtual void    OnLoadStatus(LOADSTATUS LoadStatus);
    NV_DECLARE_ONCALL_METHOD(OnLoadStatusCallback, onloadstatuscallback, (DWORD_PTR));
    HRESULT         SetProgress(DWORD dwFlags, TCHAR *pchText, ULONG ulPos, ULONG ulMax, BOOL fFlash = FALSE);
    HRESULT         SetStatusText(TCHAR *pchStatusText, LONG statusLayer);
    HRESULT         UpdateStatusText();
    HRESULT         UpdateLoadStatusUI();
    void            RefreshStatusUI();
    NV_DECLARE_ONCALL_METHOD(SetInteractive, setinteractive, (DWORD_PTR));
    HRESULT         ShowLoadError(CHtmCtx *pHtmCtx);
    void            WaitForRecalc();

    CProgSink *     GetProgSinkC();
    IProgSink *     GetProgSink();
    CHtmCtx *       HtmCtx();

    // HTTP header / Wininet flag processing

    void            ProcessHttpEquiv(LPCTSTR pchHttpEquiv, LPCTSTR pchContent);
	void			ProcessMetaName(LPCTSTR pchName, LPCTSTR pchContent);
    HRESULT         SetPicsCommandTarget(IOleCommandTarget *pctPics);

    // Html persistence

    // Saves head section of html file
    HRESULT WriteDocHeader(CStreamWriteBuff *pStreamWriteBuff);

    // Persistence to a stream

    HRESULT LoadFromStream(IStream *pStm);
    HRESULT LoadFromStream(IStream *pStm, BOOL fSync, CODEPAGE cp = 0);
    HRESULT SaveToStream(IStream *pStm);
    HRESULT SaveToStream(IStream *pStm, DWORD dwStmFlags, CODEPAGE codepage);
    HRESULT SetMetaToTrident(void);
    HRESULT SaveHtmlHead(CStreamWriteBuff *pStreamWriteBuff);
    HRESULT WriteTagToStream(CStreamWriteBuff *pStreamWriteBuff, LPTSTR szTag);
    HRESULT WriteTagNameToStream(CStreamWriteBuff *pStreamWriteBuff, LPTSTR szTagName, BOOL fEnd, BOOL bClose);

    HRESULT SaveSnapShotDocument(IStream * pStm);
    HRESULT SaveSnapshotHelper( IUnknown * pDocUnk, BOOL fVerifyParameters = false );


    // Scripting member functions

    // default "script hoogging detector" based on threshold of total number of statements executed
    enum {
        MEDIUM_OPERATION_SCALE_FACTOR = 400,
        HEAVY_OPERATION_SCALE_FACTOR  = 800,           // stmt count adder, for heavy operatoins
        RUNAWAY_SCRIPT_STATEMENTCOUNT = 5000000         // Max stmt count
    };

    void  ScaleHeavyStatementCount() {
        _dwTotalStatementCount += HEAVY_OPERATION_SCALE_FACTOR;
    }
    void  ScaleMediumStatementCount() {
        _dwTotalStatementCount += MEDIUM_OPERATION_SCALE_FACTOR;
    }


    HRESULT EnsureOmWindow();

    HRESULT EnsureObjectTypeInfo();
    HRESULT BuildObjectTypeInfo(
        CCollectionCache *pCollCache,
        long    lIndex,
        DISPID  dispidMin,
        DISPID  dispidMax,
        ITypeInfo **ppTI,
        ITypeInfo **ppTICoClass,
        BOOL fDocument = FALSE);

    NV_DECLARE_ONTICK_METHOD(FireTimeOut, firetimeout, (UINT timerID));

    HRESULT AddTimeoutCode(VARIANT *theCode, BSTR strLanguage, LONG lDelay,
                           LONG lInterval, UINT * uTimerID);
    HRESULT ClearTimeout(long lTimerID);
    HRESULT SetTimeout(VARIANT *pCode, LONG lMSec, BOOL fInterval, VARIANT *pvarLang, LONG * plTimerID);

    HRESULT CommitScripts(CBase *pelTarget = NULL, BOOL fHookup = TRUE);

    HRESULT DeferScript(CScriptElement * pScript);
    HRESULT CommitDeferredScripts(BOOL fEarly);

    HRESULT EnsureScriptCookieTable(CScriptCookieTable ** ppScriptCookieTable);

    CScriptCookieTable * _pScriptCookieTable;

    // Data transfer support

    static HRESULT GetPlaintext(CServer *, LPFORMATETC, LPSTGMEDIUM, BOOL,
                                CODEPAGE codepage);

    static HRESULT GetTEXT(CServer *, LPFORMATETC, LPSTGMEDIUM, BOOL);
    static HRESULT GetUNICODETEXT(CServer *, LPFORMATETC, LPSTGMEDIUM, BOOL);
    static HRESULT GetRTF(CServer *, LPFORMATETC, LPSTGMEDIUM, BOOL);

#ifndef NO_DATABINDING
    // Databinding support
    CDataBindTask* GetDataBindTask();
    ISimpleDataConverter* GetSimpleDataConverter();
    BOOL    SetDataBindingEnabled(BOOL fEnabled);
    void    TickleDataBinding();
    void    ReleaseDataBinding();
#endif // ndef NO_DATABINDING

    // Print support

    HRESULT DoPrint(const TCHAR *pchInput, long lRecursive=0, DWORD dwFlags=PRINT_DEFAULT, SAFEARRAY *psaHeaderFooter=0);
    HRESULT PrintPlugInSite();
    HRESULT PrintExternalIPrintObject(struct PRINTINFOBAG *pPrintInfoBag, IPrint *pPrint=NULL);
    HRESULT PageSetup();
    void    UpdateDefaultPrinter(void);
    void    UpdatePrintStatusBar(BOOL fPrinting);
    void    UpdatePrintNotifyWindow(HWND hwnd);

    HRESULT EnumContainedURLs(CURLAry * paryURLs, CURLAry * paryStrings);
    HRESULT EnumFrameURLs(CURLAry *paryURLs, CURLAry *paryPrettyURLs, CPrintInfoFlagsAry *paryPrintInfos);
    HRESULT EnumFrameIPrintObjects(CIPrintAry *paryIPrint);
    HRESULT GetActiveFrame(TCHAR *pchUrl, DWORD cchUrl, CDoc **ppActiveFrameDoc, IPrint **ppPrint=NULL);
    HRESULT GetAlternatePrintDoc(TCHAR *pchUrl, DWORD cchUrl);

    virtual BOOL IsPrintDoc();
    virtual LPTSTR GetDocumentSecurityUrl() { return _cstrUrl; };
    BOOL    PaintBackground();
    BOOL    DontRunScripts();
    BOOL    TrustSecurityUI();
    BOOL    PrintJobsPending();
    BOOL    WaitingForNothingButControls();
    void    UnwedgeFromPrinting();
    HDC     GetHDC();
    BOOL    TiledPaintDisabled();

    // TAB order and accessKey support

    BOOL    FindNextTabOrder(
                FOCUS_DIRECTION dir,
                CElement *pElemFirst,
                long lSubFirst,
                CElement **ppElemNext,
                long *plSubNext);
    HRESULT OnElementEnter(CElement *pElement);
    HRESULT OnElementExit(CElement *pElement, DWORD dwExitFlags );
    void    ClearCachedNodeOnElementExit(CTreeNode ** ppNodeToTest, CElement * pElement);
    BOOL    SearchFocusArray(
                FOCUS_DIRECTION dir,
                CElement *pElemFirst,
                long lSubFirst,
                CElement **ppElemNext,
                long *plSubNext);
    BOOL    SearchFocusTree(
                FOCUS_DIRECTION dir,
                CElement *pElemFirst,
                long lSubFirst,
                CElement **ppElemNext,
                long *plSubNext);
    void    SearchAccessKeyArray(
                FOCUS_DIRECTION   dir,
                CElement        * pElemFirst,
                CElement       ** ppElemNext,
                CMessage        * pmsg);
    HRESULT InsertAccessKeyItem(CElement * pElement);
    HRESULT InsertFocusArrayItem(CElement *pElement);
    
    // 3D border drawing helper
    //
    void CheckDoc3DBorder(CDoc * pDoc);

    // Support for deferred script execution [while not yet active]
    // and deferred deactivation [while in a script]

    BOOL    IsInScript();
    HRESULT EnterScript();
    HRESULT LeaveScript();

    HRESULT QueryContinueScript(ULONG ulStatementCount);

    HRESULT RegisterMarkupForInPlace  (CMarkup * pMarkup);
    HRESULT UnregisterMarkupForInPlace(CMarkup * pMarkup);
    HRESULT NotifyMarkupsInPlace();

    void    EnterStylesheetDownload(DWORD * pdwCookie);
    void    LeaveStylesheetDownload(DWORD * pdwCookie);
    BOOL    IsStylesheetDownload()  { return(_cStylesheetDownloading > 0); }

    // Host integration
    void UpdateHostInformation();        // helper function
    HRESULT EnsureBackupUIHandler();

    // Aggregration helper for XML MimeViewer
    BOOL    IsAggregatedByXMLMime();
    HRESULT SavePretransformedSource(BSTR bstrPath);
    
    // onblur and onfocus and Event firing helpers
    void    Fire_onfocus();
    void    Fire_onblur(BOOL fOnblurFiredFromWM = FALSE);
    void    Fire_onpropertychange(LPCTSTR strPropName);
    HRESULT Fire_PropertyChangeHelper(DISPID dispidProperty, DWORD dwFlags);
    void BUGCALL FirePostedOnPropertyChange(DWORD_PTR dwContext)
    {
        // This is only for DISPID_CDoc_activeElement anyway.
        // If the document is not parse done, then there is a chance that we will 
        // not have the scripts ready. Wait for parse done. The parse done handler
        // will fire the event.
        if (LoadStatus() >= LOADSTATUS_PARSE_DONE)
        {
            Fire_PropertyChangeHelper(DISPID_CDoc_activeElement, FORMCHNG_NOINVAL);
            _fOnPropertyChangePending = FALSE;
        }
        else
        {
            _fOnPropertyChangePending = TRUE;
        }
    }
    void    SetFocusWithoutFiringOnfocus();

    // HTA support
    BOOL    IsTrustedDoc() { return _fTrustedDoc; }
    
    // Peer persistence helpers
    HRESULT PersistFavoritesData(INamedPropertyBag * pINPB, LPCWSTR strDomain);
    BSTR    GetPersistID();
    void    FirePersistOnloads();

    // Undo Data and helpers
    virtual BOOL QueryCreateUndo(BOOL fRequiresParent, BOOL fDirtyChange = TRUE);

    CParentUndo *       _pMarkupServicesParentUndo;
    long                _uOpenUnitsCounter;
    void                FlushUndoData();

    HRESULT CreateRoot ( );

    CMarkup *       PrimaryMarkup()         { return _pPrimaryMarkup; }
    CRootElement *  PrimaryRoot();

    LOADSTATUS      LoadStatus();

    BOOL NeedRegionCollection() { return _fRegionCollection; }
    

    //------------------------------------------------------------------------
    //
    //  Member variables
    //
    //------------------------------------------------------------------------

    // Lookaside storage for elements

    void   * GetLookasidePtr(void * pvKey)
                            { return(_HtPvPv.Lookup(pvKey)); }
    HRESULT  SetLookasidePtr(void * pvKey, void * pvVal)
                            { return(_HtPvPv.Insert(pvKey, pvVal)); }
    void   * DelLookasidePtr(void * pvKey)
                            { return(_HtPvPv.Remove(pvKey)); }
    CHtPvPv             _HtPvPv;

#if DBG==1
    BOOL    AreLookasidesClear( void *pvKey, int nLookasides);
#endif

    // Lookaside storage for DOM text Nodes
    // Key is the TextID
    CHtPvPv             _HtPvPvDOMTextNodes;
    HRESULT CreateDOMTextNodeHelper ( CMarkupPointer *pmkpStart, CMarkupPointer *pmkpEnd,
                                       IHTMLDOMNode **ppTextNode);

    DWORD _dwTID;

    CFakeDocUIWindow    _FakeDocUIWindow;
    CFakeInPlaceFrame   _FakeInPlaceFrame;

    CStr                _cstrCntrApp;   //  top-level container app & obj names
    CStr                _cstrCntrObj;   //      from IOleObject::SetHostNames

public:

    //
    // The default site is used when there is no other appropriate site
    // available.
    //

    CDefaultElement * _pElementDefault;

    CMarkup * _pPrimaryMarkup;         // Pointer to the current root site

private:

    //
    // This member is the rootsite meant for display purposes.  Do not, under
    // any circumstances, use this member, use the _pSiteRoot member instead.
    //

    //
    // The order here is important.  The node must be initialized before the site.
    // The site calls SetElement on the node with itself.
    //

    friend class CSite;

    //
    // _EditRouter provides the plumbing for appropriate routing of editing commands
    //
    CEditRouter         _EditRouter;

public:
    //
    //
    //

    //
    // _pCaret is the CCaret object. Use GetCaret to access the IHTMLCaret object.
    //
    CCaret *            _pCaret;

    CDocInfo            _dci;           //  Document transform


    CElement *          _pElemEditContext;  // element that contains the caret/current selection;
                                            // not necessarily the same as_pElemCurrent
    CElement *          _pElemUIActive; //  ptr to that's showing UI.  Need not
                                        //  be the same as the current site.
    CElement *          _pElemCurrent;  //  ptr to current element. Owner of focus
                                        //  and commands get routed here.
    CElement *          _pElemDefault;  //  ptr to default element.
    long                _lSubCurrent;   //  Subdivision within element that is current
    CElement *          _pElemNext;     //  the next element to be current.
                                        //  used by SetCurrentElem during currency transitions.
    CRect *             _pRectFocus;    //  Points to the bounding rect of the last rendered focus region
    int                 _cSurface;      //  Number of requests to use offscreen surfaces
    int                 _c3DSurface;    //  Number of requests to use 3D surfaces (this count is included in _cSurface)

    OPTIONSETTINGS *    _pOptionSettings;       // Points to current user-configurable settings, like color, etc.
    CODEPAGESETTINGS *  _pCodepageSettings;     // Settings which have to do
    long                _icfDefault;            // Default CharFormat index based on option/codepage settings
    const CCharFormat * _pcfDefault;            // Default CharFormat based on option/codepage settings

    void ClearDefaultCharFormat();
    HRESULT CacheDefaultCharFormat();

    unsigned            _cInval;        //  Number of calls to CDoc::Invalidate()
    unsigned            _cProcessingTimeout;    // blocks clearing timeouts while exec'ing script

    SHORT               _iWheelDeltaRemainder;  // IntelliMouse wheel rotate zDelta
                                                // accumulation for back/forward or
                                                // zoomin/zoomout.

    CStr                _cstrPasteUrl;      // The URL during paste

    RADIOGRPNAME        *_pRadioGrpName;    // names of radio groups having checked radio

    LONG                _lRecursionLevel;   // the recursion level of the measurer

    DWORD               _dwHistoryIndex;    // the next available history index
    
    //
    //  View support
    //

    CView               _view;
    CView *             GetView()
                        {
                            return &_view;
                        }

    BOOL                OpenView(BOOL fBackground = FALSE)
                        {
                            return _view.OpenView(fBackground);
                        }


    //
    // Text ID pool
    //
    
    long _lLastTextID;

    //
    // The following version numbers are incremented when modifications
    // are made to any markup associated with this doc.
    //
    // The _lDocTreeVersion is incremented when the element structure of markups
    // are altered.  Modifications only to text between markup does not increment
    // this version number.
    //
    // The _lDocContentsVersion is incremented when any content is modified (text
    // or markup).
    //

    long GetDocTreeVersion() { return __lDocTreeVersion; }
    long GetDocContentsVersion() { return __lDocContentsVersion; }

    //
    // Do NOT modify these version numbers unless the document structure
    // or content is being modified.
    //
    // In particular, incrementing these to get a cache to rebuild is
    // BAD because it causes all sorts of other stuff to rebuilt.
    //
    
    long __lDocTreeVersion;         // Element structure
    long __lDocContentsVersion;     // Any content

    //
    // Contents version = tree version + character version
    //

    // Collections management - (tomfakes) moved to CMarkup

    // Object model SelectionObject

    CSelectionObject * _pCSelectionObject;    // selection object.
    CAtomTable         _AtomTable;            // Mapping of elements to names

    // Host intergration
    IDocHostUIHandler *     _pHostUIHandler;
    IDocHostUIHandler *     _pBackupHostUIHandler;   // To route calls when our primary fails us.
    IOleCommandTarget *     _pHostUICommandHandler;  // Command Target of UI handler
    DWORD                   _dwFlagsHostInfo;
    DWORD                   _dwFrameOptions;
    CStr                    _cstrHostCss;   // css rules sent down by host.
    CStr                    _cstrHostNS;    // semi-colon delimited namespace list
    
    //  User input event management

    // This pair manages mouse capture

    CElement *          _pElementOMCapture;

    // BUGBUG (garybu) Move _pfnCapture/pvCapture/_fnMenu/_pvMenuObect to TLS.
private:
    PFN_VOID_MOUSECAPTURE _pfnCapture; // The fn to call
    void *             _pvCaptureObject;// The object on which to call it


public:
    CElement *          _pMenuObject;          // The site on which to call it
    CTreeNode *         _pNodeLastMouseOver;   // element last fired mouseOver event
    long                _lSubDivisionLast;  // Last subdivision over the mouse
    CTreeNode *         _pNodeGotButtonDown;   // Site that last got a button down.

#ifndef NO_DATABINDING
    // Member used for databinding
    CDataBindTask *     _pDBTask;       // deferred databinding task
#endif

    HMENU               _hMenuCtx;      //  Context menus

    USHORT              _usNumVerbs;    //  Number of verbs on context menu.

    HWND                _hwndCached;    // window which hangs around while we are in running state

    ULONG               _cFreeze;       // Count of the freeze factor

    IUnknown *          _punkMimeOle;   // Used to keep the MimeOle object alive

    IStream *           _pStmDirty;     // Dirty document storage, used by refresh.
    IMoniker *          _pmkName;       // Current moniker for IPersistMoniker
    CDwnPost *          _pDwnPost;      // Post data used to get to current doc
    CStr                _cstrUrl;       // Default base Url for document (used internally)
    CStr                _cstrCOMPAT_OMUrl; // OM compat URL reported for the CDoc
    CStr                _cstrSetDomain; // OM set domain is subset of URLHostname

    SAFETYLEVEL         _safetylevel;   // user-defined safety setting for this page
    SSL_SECURITY_STATE  _sslSecurity;   // unsecure/mixed/secure
    SSL_PROMPT_STATE    _sslPrompt;     // allow/query/deny
    LONG                _cInSslPrompt;  // currently prompting about security

#ifndef NO_IME
    // Input method context cache

    HIMC                _himcCache;     // Cached if window's context is temporarily disabled
#endif

    // hyperlinking info
    IHlinkBrowseContext *_phlbc;        // Hyperlink browse context (history etc)
    DWORD               _dwLoadf;       // Load flags (DLCTL_ + offline + silent)

    // History Storage
    IUrlHistoryStg      *_pUrlHistoryStg;

    // This task wakes up every 1 second and checks if the
    // bookmark to which the user has hyperlinked has come in.
    CTaskLookForBookmark *_pTaskLookForBookmark;

    // List of all CMapElement in the tree in no particular order
    CMapElement *         _pMapHead;

    // Progress and ReadyState

    long                _readyState;                // ready state (e.g., loading, interactive, complete)
    ULONG               _ulProgressPos;
    ULONG               _ulProgressMax;

    long                _iStatusTop;                // Topmost active status
    CStr                _acstrStatus[STL_LAYERS];   // Four layers of status text

    CStr                _cstrPluginContentType;     // Full-window plugin content-type
    CStr                _cstrPluginCacheFilename;   // Full-window plugin cache filename

    ULONG               _cScriptNesting;            // Counts nesting of Enter/Leave script
    ULONG               _cStylesheetDownloading;    // Counts stylesheets being downloaded
    DWORD               _dwStylesheetDownloadingCookie;
    ULONG               _cAsyncDownloading;         // Count of async operations going on
    DWORD               _dwAsyncCookie;             // ProgSink cookie for async operations

    DECLARE_CPtrAry(CAryMarkupNotifyInPlace, CMarkup*, Mt(Mem), Mt(CDoc_aryMarkupNotifyInPlace))
    CAryMarkupNotifyInPlace _aryMarkupNotifyInPlace;

    DECLARE_CPtrAry(CAryElementDeferredScripts, CScriptElement *, Mt(Mem), Mt(CDoc_aryElementDeferredScripts_pv))
    CAryElementDeferredScripts _aryElementDeferredScripts;

    ULONG               _cIncludes;                 // number of <script language=include>s

    ULONG               _badStateErrLine;   // Error line # for stack overflow or out of memory.

    DECLARE_CDataAry(CAryFocusItem, FOCUS_ITEM, Mt(Mem), Mt(CDoc_aryFocusItems_pv))
    DECLARE_CPtrAry(CAryAccessKey, CElement *, Mt(Mem), Mt(CDoc_aryAccessKeyItems_pv))
    CAryFocusItem       _aryFocusItems;
    CAryAccessKey       _aryAccessKeyItems;
    long                _lFocusTreeVersion;
    
    // Parent doc pointers
    CDoc*               _pDocParent;

    DECLARE_CPtrAry(CAryElementReleaseNotify, CElement *, Mt(Mem), Mt(CDoc_aryElementReleaseNotify_pv))
    CAryElementReleaseNotify   _aryElementReleaseNotify;

    HRESULT ReleaseNotify();
    HRESULT RequestReleaseNotify(CElement * pElement);
    HRESULT RevokeRequestReleaseNotify(CElement * pElement);

    // Modeless Dialog support...  when this document is unloadcontents, the 
    //  active Modeless dialogs need to be closed.
    //-------------------------------------------------------------------------
    DECLARE_CDataAry(CAryActiveModelessDlgs, HWND, Mt(Mem), Mt(CDoc_aryActiveModelessDlgs))
    CAryActiveModelessDlgs     _aryActiveModeless;

    IOleCommandTarget * _pctPics;       // Call _pctPics back with PICS labels
    void *              _pvPics;

    BSTR                _bstrUserAgent;
    CDwnDoc *           _pDwnDoc;
    IDownloadNotify *   _pDownloadNotify;

    // Security
    IInternetSecurityManager *  _pSecurityMgr;
    CSecurityMgrSite    _SecuritySite;
    IActiveXSafetyProvider *_pActiveXSafetyProvider;

    // Drag-drop
    CDragDropSrcInfo *      _pDragDropSrcInfo;
    CDragDropTargetInfo *   _pDragDropTargetInfo;
    CDragStartInfo *        _pDragStartInfo;

    //  Bit fields

    unsigned            _fEnabled:1;
    unsigned            _fOKEmbed:1;        // TRUE if can drop as embedding
    unsigned            _fOKLink:1;         // TRUE if can drop as link
    unsigned            _fDragFeedbackVis:1;// Feedback rect has been drawn
    unsigned            _fIsDragDropSrc:1;  // Originated the current drag-drop operation
    unsigned            _fDisableTiledPaint:1;
    unsigned            _fUpdateUIPending:1;
    unsigned            _fNeedUpdateUI:1;
    unsigned            _fNeedUpdateTitle:1;
    unsigned            _fNoUndoActivate:1;
    unsigned            _fInPlaceActivating:1;
    unsigned            _fReadOnly:1;
    unsigned            _fFromCtrlPalette:1;
    unsigned            _fRightBtnDrag:1;   // TRUE if right button drag occuring
    unsigned            _fOwnPackage:1;     // TRUE if this form created the
                                            //   package object
    unsigned            _fSlowClick:1;      // TRUE if the user started
                                            //   a right drag but didn't move
                                            //   the mouse when ending the drag
    unsigned            _fElementCapture:1; // TRUE if _pvCaptureObject is
                                            //   a CElement
    unsigned            _fOnLoseCapture:1;  // TRUE if we are in the process
                                            // of firing onlosecapture

    unsigned            _fFiredOnLoad:1;    // TRUE if OnLoad has been fired
    unsigned            _fShownSpin:1;      // TRUE if animation state has been shown
    unsigned            _fShownProgPos:1;   // TRUE if progress pos has been shown
    unsigned            _fShownProgMax:1;   // TRUE if progress max has been shown
    unsigned            _fShownProgText:1;  // TRUE if progress text has been shown
    unsigned            _fProgressFlash:1;  // TRUE if progress should be cleared by next setstatustext
    unsigned            _fGotKeyDown:1;     // TRUE if we got a key down
    unsigned            _fGotKeyUp:1;       // TRUE if we got a key up
    unsigned            _fGotLButtonDown:1; // TRUE if we got a left button down
    unsigned            _fGotMButtonDown:1; // TRUE if we got a middle button down
    unsigned            _fGotRButtonDown:1; // TRUE if we got a right button down
    unsigned            _fGotDblClk:1;      // TRUE if we got a double click message

    unsigned            _fMouseOverTimer:1; // TRUE if MouseMove timer is set for detecting exit event
    unsigned            _fSuspendTimeout:1; // TRUE if timeout firing is off
    unsigned            _fForceCurrentElem:1;// TRUE if SetCurrentElem must succeed
    unsigned            _fCurrencySet:1;    // TRUE if currency has been set at least once (to an element other than the root)
    unsigned            _fOnPropertyChangePending :1;   // The first activeElement property change is waiting for parse done.
    unsigned            _fSafetyInformed:1; // TRUE if user has been notified unsafe content avoided.
    unsigned            _fSpin:1;           // Spin state of document
    unsigned            _fInheritDesignMode:1;
    unsigned            _fDesignMode:1;
    unsigned            _fSaveTempfileForPrinting:1; // TRUE iff we are saving a temporary file for printing
                                                     // - needed for recursive frame-doc saving.
    unsigned            _fPrintedDocSavedPlugins:1;  // TRUE if CDoc::DoPrint saved a plugin ocx with marshaled punk to original instance (60771).
    unsigned            _fUnwedgeFromPrinting:1;     // TRUE if OnMethodCall might have to be unwedged due to untreated messages (53416).
    unsigned            _fExpando:1;
    unsigned            _fImageFile:1;
    unsigned            _fFrameSet:1;
    unsigned            _fFullWindowEmbed:1; // TRUE if we are hosting a plugin handled object
                                             // in a synthsized <embed...> html document.
    unsigned            _fInhibitFocusFiring:1;  // TRUE if we shouldn't Fire_onfocus() when
                                                 // when handling the WM_SETFOCUS event
    unsigned            _fFirstTimeTab:1;    // TRUE if this is the first time
                                             // Trident receives TAB key,
                                             // should not process this message
                                             // and move focus to address bar.

    unsigned            _fNeedTabOut:1;      // TRUE if we should not handle
                                             // this SHIFT VK_TAB WM_KEYDOWN
                                             // message, this is Raid 61972
    unsigned            _fGotAmbientDlcontrol:1;
    unsigned            _fGotHttpExpires:1;
    unsigned            _fInHTMLDlg:1;       // there are cases that we need to know this.
    unsigned            _fInTrustedHTMLDlg:1;       // there are cases that we also need to know this.
    unsigned            _fInHTMLPropPage:1;  // used by CDoc::DetachWin for Win9x bug workaround
    unsigned            _fFiredWindowFocus:1;   // TRUE if Window onfocus event has been fired,
                                                // FALSE if Window onblur has been fired.

    unsigned            _fMhtmlDoc:1;        // We have been instantiated as an MHTML handler.

    unsigned            _fEnableInteraction:1;  // FALSE when the browser window is minimized or
                                                // totally covered by other windows.

    unsigned            _fModalDialogInScript:1;    // Exclusively for use by PumpMessage() to
                                                    // figure out if an event handler put up a
                                                    // a modal dialog
    unsigned            _fModalDialogUp:1;          // A modal dialog has been thrown up from this doc
    unsigned            _fInvalInScript:1;          // Exclusively for use by PumpMessage() to
                                                    // figure out if an event handler caused an
                                                    // invalidation.
    unsigned            _fInPumpMessage:1;          // Exclusively for use by PumpMessage() to
                                                    // to handle recursive calls

    unsigned            _fDeferredScripts:1;        // TRUE iff there are deferred scripts not commited yet

    unsigned            _fUserInteracted:1;         // Set to TRUE as soon as the user pressed
                                                    // a key or a mouse button.
    unsigned            _fForceSetCurrent:1;        // Force CElement::BecomeCurrent() to execution
                                                    // even if CDoc::_pElemNext is itself,
                                                    // This is for SETFOCUS/KILLFOCUS

    unsigned            _fDelayLoadHistoryDone:1;   // Set to TRUE once SN_DELAYLOADHISTORY
                                                    // notification has been sent.

    unsigned            _fUserStopRunawayScript:1;  // Set to TRUE when the user has decided to stop
                                                    // "CPU hogging" scripts

    unsigned            _fYieldCurrencyFailed:1;    // hack to allow OleSite to figure out if a
                                                    // UIActivation failed becuase of YieldCurrency.
                                                    // Bit is only valid across an attempt to UIActivate
                                                    // an OleSite.

    unsigned            _fUIHandlerSet:1;           // Set to TRUE when ICustomDoc::SetUIHandler is called successfully

    unsigned            _fNeedInPlaceActivation:1;   // TRUE for script and objects, FALSE for LINK stylesheets
    unsigned            _fInHTMLEditMode:1;          // Are we in HTML edit mode or plain text edit mode?

    unsigned            _fIdentifiedRootDocAsCPrintDoc:1;// TRUE only if we KNOW that root doc is CPrintDoc, else FALSE
    unsigned            _fIdentifiedRootDocAsCDoc:1;     // TRUE only if we KNOW that root doc is CDoc, else FALSE
    unsigned            _fQueryContinueDialogIsUp:1;     // TRUE if the QueryContinue dialog is up
    unsigned            _fColorsInvalid:1;           // Our colors are invalid, we need to recompute the palette
    unsigned            _fGotAuthorPalette:1;        // We have an author defined palette (stored in CDwnDoc)
    unsigned            _fHtAmbientPalette:1;        // TRUE if ambient palette is the halftone palette
    unsigned            _fHtDocumentPalette:1;       // TRUE if document palette is the halftone palette
    unsigned            _fTagsInFrameset:1;          // TRUE if the parser has read non-<FRAME><FRAMESET><!--> tags in the frameset
    unsigned            _fFramesetInBody:1;          // TRUE if the parser has read <FRAMESET> tags in the body

    unsigned            _fStackOverflow: 1;          // Script engine reported stack overflow
    unsigned            _fOutOfMemory: 1;            // Script engine reported out of memory
    unsigned            _fEngineSuspended: 1;        // Script engines have been suspended due to stack overflow or out of memory.

    unsigned            _fSslSuppressedLoad : 1;     // For SSL security reasons, this page was not loaded (an about: moniker was substituted)
    unsigned            _fPersistStreamSync : 1;     // Next LoadFromStream should be synchronous
    unsigned            _fDesktopHtml : 1;           // Are we used on the Active Desktop?
    unsigned            _fTopBrowserDocument : 1;    // Are we in the top document in the browser hierarchy.
    unsigned            _fMarkupServicesParsing : 1; // Are we parsing for MarkupServices or download
    unsigned            _fPasteIE40Absolutify : 1;   // When parsing for paste, absolutify certain URLs
    unsigned            _fContainerCapture : 1;      // TRUE if elements inside containers should ignore OM capture
    unsigned            _fHasScriptForEvent : 1;     // TRUE if there is a script tag with FOR/EVENT
    unsigned            _fCodePageWasAutoDetect : 1; // Was codepage autodetected?

    unsigned            _fRegionCollection : 1;      // TRUE if region collection should be built
    unsigned            _fPlaintextSave : 1;         // TRUE if currently saving as .txt
    unsigned            _fDisableReaderMode : 1;     // auto-scroll reader mode should be disabled.
    unsigned            _fIsUpToDate : 1;            // TRUE if doc is uptodate
    unsigned            _fDataBindingEnabled : 1;    // TRUE if databinding is allowed
    unsigned            _fThumbNailView : 1;         // Are we in ThumbNailView? (if so, use the EnsureFormats() hack)
    unsigned            _fOutlook98 : 1;             // Use hack for Outlook98?
    unsigned            _fOE4 : 1;                   // hosted in Outlook Express 4?
    unsigned            _fDontFireOnFocusForOutlook98:1; // Focus hack for Outlook98
    unsigned            _fDontUIActivateOleSite:1;   // Do not UIActivate an olesite,but make it inplace active, if the Doc is not UIActive
    unsigned            _fVID : 1;                   // Call CDoc::OnControlInfoChanged when TRUE
    unsigned            _fVB : 1;                    // TRUE if Trident's parent window class is "HTMLPageDesignerWndClass"
    unsigned            _fDefView : 1;               // Use hack for defview?
    unsigned            _fActiveDesktop:1;           // TRUE if this is the Trident instance in the Active Desktop.
    unsigned            _fProgressStatus : 1;        // TRUE if progress status text should be transmitted to host
    unsigned            _fPeersPossible : 1;         // true if there is a chance that peers could be on this page
    unsigned            _fHasOleSite : 1;            // There is an olesite somewhere
    unsigned            _fHasBaseTag : 1;            // There is a base tag somewhere in the tree
    unsigned            _fNoFixupURLsOnPaste : 1;    // TRUE if we shouldn't make URLs absolute on images and anchors when they are pasted
    unsigned            _fHostDrivesEditing: 1;      // The Host has chosen to implement IHTMLEditor
    unsigned            _fModalDialogInOnblur : 1;   // TRUE if a modal dialog pops up in script in any onblur event handler.
                                                     // Enables firing of onfocus again in such cases.
    unsigned            _fTrustedDoc : 1;            // TRUE if this doc is trusted                                                    
    unsigned            _fDelegateWindowOM : 1;      // TRUE if this OM window calls should be delegated to shdocvw                                                    
    unsigned            _fInEditCapture:1 ;          // TRUE if mshtmled has Capture
    unsigned            _fOnControlInfoChangedPosted:1; // TRUE if GWPostMethodCall was called for CDoc::OnControlInfoChanged
    unsigned            _fNeedUrlImgCtxDeferredDownload:1;// TRUE if OnUrlImgCtxDeferredDownload was delayed because of ValidateSecureUrl
    unsigned            _fSelectionHidden:1;            // TRUE if We have hidden the seletion from a WM_KILLFOCUS
    unsigned            _fPrintEvent:1;              // TRUE is we are firing onbeforeprint or onafterprint
    unsigned            _fVirtualHitTest:1;         // TRUE if virtual hit testing enabled
    unsigned            _fNotifyBeginSelection:1;   // TRUE if we are broadcasting a WM_BEGINSELECTION Notification
    unsigned            _fInhibitOnMouseOver:1;     // TRUE if onmouseover event should not be fired, like when over a popup window.
    unsigned            _fBroadcastInteraction:1;   // TRUE if broadcast of EnableInteraction is needed
    unsigned            _fBroadcastStop:1;          // TRUE if broadcast of Stop is needed
    unsigned            _fInTraverseGroup:1;        // TRUE if inside TraverseGroup for input radio
    unsigned            _fVisualOrder:1;            // the document is RTL visual (logical is LTR)
                                                    // This is used for ISO-8859-8 and ISO-8859-6
                                                    // visually ordered documents.
    unsigned            _fRTLDocDirection:1;        // TRUE if the document is right to left <HTML DIR=RTL>
    unsigned            _fInvalNoninteractive:1;    // TRUE if inavlidated while not interactive
    
#ifdef VSTUDIO7
    unsigned            _fHasIdentityPeerFactory:1;     // TRUE if we have a identity peer factory.
    unsigned            _fHasHostIdentityPeerFactory:1; // TRUE if we have a host supplied identity peer factory.
    unsigned            _fNeedBaseTags:1;               // TRUE if we need to get base tags after scripts are commited.
#endif //VSTUDIO7
    unsigned            _fSafeToUseCalcSizeHistory: 1;  // set to TRUE if it is safe to use calc size history
    unsigned            _fSeenDefaultStatus: 1;     // TRUE if window.defaultStatus has been set before

    // WARNING: THIS FLAG MUST ONLY BE USED TO ENFORCE LAYOUT COMPAT CHANGES NECESSARY FOR HOME PUBLISHER 98.
    //          ALL OTHER USES ARE ILLEGAL AND LIKELY TO BREAK. (brendand)
    unsigned            _fInHomePublisherDoc: 1;    // TRUE if this doc was created by HomePublisher98
    unsigned            _fDontDownloadCSS: 1;       // TRUE then don't download CSS files when a put_URL is called during
                                                    // designMode.
    unsigned            _fPendingFilterCallback:1;    // A filter callback is pending
    unsigned            _fPassivateCalled:1;        // Trap multiple passivate calls
    unsigned            _fDeleteUrlCacheEntry : 1;   // Need to delete cache entry upon LOADSTATUS_PARSE_DONE

    WORD                _wUIState;

    DWORD               _dwCompat;  // URL compat DWORD

#ifdef QUILL
    CQDocGlue           *_pqdocGlue;                // Pointer to aggregated subobject.
                                                    // Kept as a pointer to avoid build dependencies.
    inline CQDocGlue *GetLayoutGroup() { return _pqdocGlue; }
    BOOL FExternalLayout();
#endif // QUILL

#if TREE_SYNC
    //
    // treesync state object
    //
    // this is a big HACK, to temporarily get markup-sync working for netdocs.  it
    // will totally change in the future.
    //

    CMkpSyncLogger      _SyncLogger;
#endif // TREE_SYNC

public:

    IHTMLEditor* GetHTMLEditor( BOOL fForceCreate = TRUE ); // Get the html editor, if we have to load mshtmled, should we force it?

    HRESULT GetEditingServices( IHTMLEditingServices** ppIServices );
    
    HRESULT Select( IMarkupPointer* pStart, IMarkupPointer* pEnd, SELECTION_TYPE eType );

    BOOL IsElementSiteSelectable( CElement* pCurElement );

    BOOL IsElementUIActivatable( CElement* pCurElement );
    
    BOOL IsElementSiteSelected( CElement* pCurElement );

    HRESULT ProcessFollowUpAction( CMessage* pMessage, DWORD dwFollowUpAction );
    
private:

    IHTMLEditor*   _pIHTMLEditor;        // Selection Manager.


#if DBG ==1
    BOOL _fInEditTimer:1;    
#endif
    BOOL    ShouldCreateHTMLEditor(CMessage *pMessage);
    
    BOOL    ShouldCreateHTMLEditor(SELECTION_NOTIFICATION eNotify );

    BOOL    ShouldSetEditContext(CMessage *pMessage);

    NV_DECLARE_MOUSECAPTURE_METHOD( HandleEditMessageCapture, handleeditmessagecapture,
                                    ( CMessage *pMessage ));

    NV_DECLARE_ONTICK_METHOD( OnEditDblClkTimer, onselectdblclicktimer,
                                          (UINT idMessage ));

    VOID SetClick( CMessage* inMessage );

    HRESULT DragElement(CMessage* pMessage);   


public:
    CSelDragDropSrcInfo* GetSelectionDragDropSource();
    
    HRESULT UpdateCaret(
        BOOL        fScrollIntoView,    //@parm If TRUE, scroll caret into view if we have
                                        // focus or if not and selection isn't hidden
        BOOL        fForceScrollCaret,  //@parm If TRUE, scroll caret into view regardless
        CDocInfo *  pdci = NULL );
        
    BOOL IsCaretVisible(BOOL * pfPositioned = NULL );
    
    HRESULT HandleSelectionMessage(CMessage* inMessage, BOOL fForceCreation );

    HRESULT SetEditContext( CElement* pElement, BOOL fForceCreation, BOOL fSetSelection, BOOL fDrillingIn = FALSE );

    HRESULT NotifySelection( SELECTION_NOTIFICATION eSelectionNotification,
                             IUnknown* pUnknown,
                             DWORD dword = 0 );

    SELECTION_TYPE GetSelectionType();

    BOOL    HasTextSelection();

    BOOL HasSelection();

    BOOL IsPointInSelection(POINT pt, CTreeNode* pNode = NULL, BOOL fPtIsContent = FALSE);

    HRESULT ScrollPointersIntoView(IMarkupPointer *    pStart, IMarkupPointer *    pEnd);

    LPCTSTR GetCursorForHTC( HTC inHTC );
    
    HRESULT AdjustEditContext( 
                            CElement* pStartElement, 
                            CElement** ppEditThisElement, 
                            IMarkupPointer* pStart,
                            IMarkupPointer* pEnd , 
                            BOOL * pfEditThisEditable,
                            BOOL * pfEditParentEditable,
                            BOOL * pfNoScopeElement ,
                            BOOL fSetCurrencyIfInvalid = FALSE,
                            BOOL fSetElemEditContext = TRUE,
                            BOOL fDrillingIn = FALSE);

    HRESULT GetEditContext( 
                            CElement* pStartElement, 
                            CElement** ppEditThisElement, 
                            IMarkupPointer* pIStart  = NULL ,
                            IMarkupPointer* pIEnd = NULL ,
                            BOOL fDrillingIn = FALSE,
                            BOOL * pfEditThisEditable = NULL,
                            BOOL * pfEditParentEditable  = NULL,
                            BOOL * pfNoScopeElement = NULL );

    HRESULT EnsureEditContext( CElement* pElement, BOOL fDrillingIn = TRUE );
    
    CElement* GetOutermostTableElement( CElement* pElement);                            

    BOOL IsTablePart( CElement* pElement);

    
    // Enum Modes



    SIZEL               _sizelGrid;
    BYTE                _modeClick;
    BYTE                _modeDblClick;

    // state needed across two successive messages by PumpMessage
    BYTE                _bLeadByte;

    // 3D border setting
    //
    BYTE                _b3DBorder;

#define GRIDX_DEFAULTVALUE      HimetricFromHPix(8)
#define GRIDY_DEFAULTVALUE      HimetricFromVPix(8)

    //  Project Model support

    CClassTable         _clsTab;            //  The class table
    ILocalRegistry *    _pLicenseMgr;       //  License manager for page.

    // Rendering properties
    LONG                _bufferDepth;       // sets bits-per-pixel for offscreen buffer
    friend class CDocUpdateIntSink;
    CDocUpdateIntSink * _pUpdateIntSink;    // Sink for updateInterval timer
    ITimer *            _pTimerDraw;        // NAMEDTIMER_DRAW, for sync'ing control with paint
    INT                 _triOMOffscreenOK;  // Silly tristate for Nav 4 parity of offscreenBuffering prop.

    //  Persistent state
    ULONG               _ID;

    // Recalc support

    friend class CRecalcHost;
    //
    // This little helper object hosts the recalc engine so that it doesn't ref the doc
    // The host also owns the ref count on the recalc engine so there is seldom a need
    // to AddRef the engine internally.
    //
    class CRecalcHost : CVoid , public IRecalcHost , public IServiceProvider
#if DBG == 1
        , public IRecalcHostDebug
#endif
    {
        friend class CDoc;
    public:
        CDoc *MyDoc() { return CONTAINING_RECORD(this, CDoc, _recalcHost); }

        //
        // BUGBUG 41567 (michaelw) - The recalc engine starts out suspended and 
        //                           is only unsuspended when the view is initialized
        //                           This is a temporary hack to deal with the fact the
        //                           many bugs that show up if you try to access layout
        //                           properties without a view
        //
        CRecalcHost() : _pEngine(0) , _ulSuspend(1) {};
        void Detach();

        BOOL HasEngine() { return _pEngine != 0; }
        BOOL InSetValue() { return _pElemSetValue != 0; }
        DISPID GetSetValueDispid(CElement *pElem) { return (pElem == _pElemSetValue) ? _dispidSetValue : 0; }

        // Helpers that initialize the recalc engine as needed
        HRESULT SuspendRecalc(VARIANT_BOOL fSuspend);


        //
        // Helpers that automatically initialize the engine as needed
        //
        HRESULT EngineRecalcAll(BOOL fForce);
        HRESULT setExpression(CBase *pBase, BSTR bstrProperty, BSTR bstrExpression, BSTR bstrLanguage);
        HRESULT setExpressionDISPID(CBase *pBase, DISPID dispid, BSTR bstrExpression, BSTR bstrLanguage);
        HRESULT getExpression(CBase *pBase, BSTR bstrProperty, VARIANT *pvExpression);
        HRESULT removeExpression(CBase *pBase, BSTR bstrProperty, VARIANT_BOOL *pfSuccess);
        HRESULT setStyleExpressions(CElement *pElement);

        // IUnknown methods
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IRecalcHost methods
        STDMETHOD(CompileExpression)(IUnknown *pUnk, DISPID dispid, LPOLESTR szExpression, LPOLESTR szLanguage, IDispatch **ppExpressionObject, IDispatch **ppThis);
        STDMETHOD(EvalExpression)(IUnknown *pUnk, DISPID dispid, LPOLESTR szExpression, LPOLESTR szLanguage, VARIANT *pvResult);
        STDMETHOD(ResolveNames)(IUnknown *pUnk, DISPID dispid, unsigned cNames, LPOLESTR *pszNames, IDispatch **ppObjects, DISPID *pDispids);
        STDMETHOD(SetValue)(IUnknown *pUnk, DISPID dispid, VARIANT *pv, BOOL fStyle);
        STDMETHOD(RemoveValue)(IUnknown *pUnk, DISPID dispid);
        STDMETHOD(GetScriptTextAttributes)(LPCOLESTR szLanguage, LPCOLESTR pchCode, ULONG cchCode, LPCOLESTR szDelim, DWORD dwFlags, WORD *pwAttr);
        STDMETHOD(RequestRecalc)();

        // IServiceProvider methods
        STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppv);

        // IRecalcHostDebug methods
#if DBG == 1
        STDMETHOD(GetObjectInfo)(IUnknown *pUnk, DISPID dispid, BSTR *pbstrID, BSTR *pbstrMember, BSTR *pbstrTag);
        int Dump(DWORD dwFlags);
#endif

    private:
        HRESULT Init();
        HRESULT resolveName(IDispatch *pDispatchThis, DISPID dispid, LPOLESTR szName, IDispatch **ppDispatch, DISPID *pdispid);
        IRecalcEngine *_pEngine;
        CElement *_pElemSetValue;       // Non zero if we're doing a SetValue
        DISPID _dispidSetValue;         // The dispid we're setting
        BOOL _fRecalcRequested;         // Have we already issued a GWPostMethodCall for recalc?
        BOOL _fInRecalc;                // Are we currently doing a recalc?
        unsigned _ulSuspend;            // Currrent suspend count
    };

    public:

    CRecalcHost _recalcHost;
    HRESULT STDMETHODCALLTYPE suspendRecalc(BOOL fSuspend);
    friend class CRecalcHost;


    //  Palette stuff

    HPALETTE            _hpalAmbient;       // The palette we get back from the container
    HPALETTE            _hpalDocument;      // The palette we create as needed
    LOGPALETTE *        _pColors;           // A cache of our current color set

    //  Version stuff
    CVersions *         _pVersions;

    //  Load stuff

    ULONG               _cDie;              // Incremented whenever UnloadContents is called

    BOOL IsLoading();

    // IPersistHistory & UserData load support
    CHistoryLoadCtx *   _pHistoryLoadCtx;
    CStr                _cstrHistoryUserData;    // the string for history userdata
    IXMLDOMDocument *   _pXMLHistoryUserData;    // Sad but true, a temp place to hang this while
                                                 //    firing the persistence events
    INamedPropertyBag * _pShortcutUserData;      // the INPB object from shortcut userdata/navigation
    CStr                _cstrShortcutProfile;    // the name of the file to hook up to the INPB
    struct
    {
        long    lIndex;
        DWORD   dwCode;
        long    lSubDivision;
    }                   _historyCurElem;

    // list of objects that failed to initialize
    DECLARE_CPtrAry(CAryDefunctObjects, CObjectElement *, Mt(Mem), Mt(CDoc_aryDefunctObjects_pv))
    CAryDefunctObjects  _aryDefunctObjects;

    // list of child downloads with wrapped bsc's.
    DECLARE_CPtrAry(CAryChildDownloads, CProgressBindStatusCallback *, Mt(Mem), Mt(CDoc_aryChildDownloads_pv))
    CAryChildDownloads _aryChildDownloads;

    //------------------------------------------------------------------------
    // Scripting

    CScriptCollection * _pScriptCollection; // A collection of script sites
    COmWindowProxy *    _pOmWindow;         // Ptr to the script window
    ITypeInfo *         _pTypInfo;          // Typ info created on the fly.
    ITypeInfo *         _pTypInfoCoClass;   // Coclass created on the fly
    CTimeoutEventList   _TimeoutEvents;     // List for active timeouts
    EVENTPARAM *        _pparam;            // Ptr to event params
    DWORD               _dwTotalStatementCount;  // How many statements have we executed
    DWORD               _dwMaxStatements;   // Max number of statements before alert

#ifdef WIN16
    DWORD               _msecsScriptFragmentStarted; // what sys time did it start?
    DWORD               _msecsScriptFragmentTotalRunningTime; // how long has it been running?
#endif // ndef WIN16

    // Helper to clean up script timers
    void        CleanupScriptTimers ( void );

    CStyleSheetArray    *_pHostStyleSheets; // All stylesheets given by host
    
    // No scope tags visibility

#if !defined(MW_MSCOMPATIBLE_STRUCT)
    union {
        DWORD           _dwMiscFlagsVar; // Use () version for now
        struct {
#endif
            unsigned    _fShowAlignedSiteTags:1;    // Show align sites at design time
            unsigned    _fShowScriptTags:1;         // Show SCRIPT at design time
            unsigned    _fShowCommentTags:1;        // Show COMMENT and <!--...--> at design time
            unsigned    _fShowStyleTags:1;          // Show STYLE tags at design time
            unsigned    _fShowAreaTags:1;           // Show AREA tags at design time
            unsigned    _fShowUnknownTags:1;        // Show unknown tags at design time
            unsigned    _fShowMiscTags:1;           // Show other misc no scope tags at design time
            unsigned    _fShowZeroBorderAtDesignTime:1;   // Show zero borders at design time            
            unsigned    _fNoActivateNormalOleControls:1;  // Don't activate OLE controls at design time
            unsigned    _fNoActivateDesignTimeControls:1; // Don't activate design time controls at design time
            unsigned    _fNoActivateJavaApplets:1;        // Don't activate Java applets at design time
            unsigned    _fShowWbrTags:1;            // Show WBR tags at design time
            unsigned    _fCPSelChange:1;            // 0 = No, 1 = Yes (last user charset selection
                                                    //                  change is still not served)
            unsigned    _fDefaultPeerFactoryEnsured:1; // true when default peer factory was attempted to create
            unsigned    _fFrameBorderCacheValid:1;
            unsigned    _fUnused:15;

#if !defined(MW_MSCOMPATIBLE_STRUCT)
        };
    };
    DWORD& _dwMiscFlags() { return _dwMiscFlagsVar; }
#else
    DWORD& _dwMiscFlags() { return *(((DWORD *)&_pHostStyleSheets) +1); }
#endif


    //-------------------------------------------------------------------------
    // Glyph Management (in addition to predefined flags above)
    public:
        HRESULT GetTagInfo (
                            CTreePos *              ptp, 
                            int                     gAlign, 
                            int                     gPositioning, 
                            int                     gOrientation, 
                            void *                  invalidateInfo, 
                            CGlyphRenderInfoType *  ptagInfo
                            );

    private:
        HRESULT EnsureGlyphTableExistsAndExecute (
                            GUID * pguidCmdGroup,
                            UINT idm,
                            DWORD nCmdexecopt,
                            VARIANTARG * pvarargIn,
                            VARIANTARG * pvarargOut);

        CGlyph *      _pGlyphTable; 


private:
    //-------------------------------------------------------------------------
    // Cache of loaded images (background, list bullets, etc)

    DECLARE_CPtrAry(CAryUrlImgCtxElems, CElement *, Mt(Mem), Mt(CDoc_aryUrlImgCtx_aryElems_pv))

    struct URLIMGCTX
    {
        LONG                lAnimCookie;
        ULONG               ulRefs;
        CImgCtx *           pImgCtx;
        CStr                cstrUrl;
        CAryUrlImgCtxElems  aryElems;
    };

    DECLARE_CDataAry(CAryUrlImgCtx, URLIMGCTX, Mt(Mem), Mt(CDoc_aryUrlImgCtx_pv))
    CAryUrlImgCtx           _aryUrlImgCtx;
    DWORD                   _dwCookieUrlImgCtxDef;
    
public:

    HRESULT        AddRefUrlImgCtx(LPCTSTR lpszUrl, CElement * pElemContext, LONG * plCookie);
    HRESULT        AddRefUrlImgCtx(LONG lCookie, CElement * pElem);
    void BUGCALL   OnUrlImgCtxDeferredDownload(DWORD_PTR dwContext);
    CImgCtx *      GetUrlImgCtx(LONG lCookie);
    IMGANIMSTATE * GetImgAnimState(LONG lCookie);
    void           ReleaseUrlImgCtx(LONG lCookie, CElement * pElem);
    void           StopUrlImgCtx();
    void           UnregisterUrlImgCtxCallbacks();
    static void CALLBACK OnUrlImgCtxCallback(void *, void *);
    BOOL           OnUrlImgCtxChange(URLIMGCTX * purlimgctx, ULONG ulState);
    static void    OnAnimSyncCallback(void * pvObj, DWORD dwReason,
                                      void * pvArg,
                                      void ** ppvDataOut,
                                      IMGANIMSTATE * pAnimState);


    //------------------------------------------------------------------------
    //
    //  Static members
    //

    static OLEMENUGROUPWIDTHS       s_amgw[];

    // move up from CHtmlDoc

    static const CLASSDESC              s_classdesc;
    static const CLSID *                s_apClsidPages[];
    static PROP_DESC                    s_apropdesc[];
    static FORMATETC                    s_GetFormatEtc[];
    static CServer::LPFNGETDATA         s_GetFormatFuncs[];

    // Scaling factor for text
    SHORT _sBaselineFont;
    SHORT GetBaselineFont() { return _sBaselineFont; }

    // Internationalization
    CODEPAGE _codepage;
    CODEPAGE _codepageURL;
    CODEPAGE _codepageFamily;
    BOOL     _fCodepageOverridden;
    CODEPAGE GetCodePage() { return _codepage; }
    CODEPAGE GetURLCodePage();
    CODEPAGE GetFamilyCodePage() { return _codepageFamily; }
    HRESULT  SwitchCodePage( CODEPAGE codepage );
    HRESULT  UpdateCodePageMetaTag( CODEPAGE codepage );
    BOOL     HaveCodePageMetaTag();
    void     BubbleDownCodePage( CODEPAGE codepage );

    HRESULT GetDocDirection(BOOL *pfRTL);

    HRESULT SetCpAutoDetect(BOOL fAuto);
    BOOL    IsCpAutoDetect(void);
    HRESULT SaveDefaultCodepage(CODEPAGE codepage);

    // Default block element.
    ELEMENT_TAG _etagBlockDefault;
    HRESULT SetupDefaultBlockTag(VARIANTARG *pvarvargIn);
    VOID    SetDefaultBlockTag(ELEMENT_TAG etag)
                { _etagBlockDefault = etag; }
    ELEMENT_TAG GetDefaultBlockTag()
                { return _etagBlockDefault; }

    // BUGBUG (tomfakes) Is this necessary now that sheets have moved to the Markup?
    BOOL    HasHostStyleSheets()
    {
        return !!(_pHostStyleSheets);
    }

    //
    // behaviors support
    //

    void    SetPeersPossible();

    HRESULT FindDefaultBehavior1(
        LPTSTR                      pchName,
        LPTSTR                      pchUrl,
        IElementBehaviorFactory **  ppFactory,
        LPTSTR                      pchUrlDownload,
        UINT                        cchUrlDownload);

    HRESULT FindDefaultBehavior2(
        LPTSTR                      pchName,
        LPTSTR                      pchUrl,
        IElementBehaviorSite *      pSite,
        IElementBehavior **         ppPeer);

    HRESULT FindHostBehavior(
        LPTSTR                      pchName,
        LPTSTR                      pchUrl,
        IElementBehaviorSite *      pSite,
        IElementBehavior **         ppPeer);

    HRESULT AttachPeersCss(
        CElement *              pElement,
        CAtomTable *            pacstrBehaviorUrls);

    HRESULT AttachPeerCss(
        CPeerHolder *           pPeerHolder,
        LPTSTR                  pchUrl);

    HRESULT AttachPeer(
        CElement *              pElement,
        LPTSTR                  pchUrl,
        CPeerFactoryBinary *    pFactoryBinary = NULL,
        LONG *                  pCookie = NULL);

    HRESULT RemovePeer(
        CElement *              pElement,
        LONG                    cookie,
        VARIANT_BOOL *          pfResult);

    
#ifdef VSTUDIO7
    ELEMENT_TAG IsGenericElement         (LPTSTR pchFullName, LPTSTR pchColon, BOOL *pfDerived = FALSE);
#else
    ELEMENT_TAG IsGenericElement         (LPTSTR pchFullName, LPTSTR pchColon);
#endif //VSTUDIO7
    
    ELEMENT_TAG IsGenericElementHost     (LPTSTR pchFullName, LPTSTR pchColon);

#ifdef VSTUDIO7
    ELEMENT_TAG IsGenericElementFactory(LPTSTR pchFullName, LPTSTR pchColon);
    HRESULT     EnsureIdentityFactory(LPTSTR pchCodebase = NULL);
    HRESULT     GetIdentityFactory(IIdentityBehaviorFactory **ppFactory);
    HRESULT     GetBaseTagsFromFactory(LPTSTR pchCodebase = NULL);
#endif

    void    StopPeerFactoriesDownloads();

    IElementBehaviorFactory *   _pDefaultPeerFactory;   // default peer factory
    IElementBehaviorFactory *   _pHostPeerFactory;      // factory supplied by host

    // peer task queue

    class CPeerQueueItem
    {
    public:
        void Init (CBase * pTarget, PEERTASK task)
        {
            _pTarget  = pTarget;
            _task     = task;
        }
        
        CBase *     _pTarget;
        PEERTASK    _task;
    };

    DECLARE_CDataAry(CAryPeerQueue, CPeerQueueItem, Mt(Mem), Mt(CDoc_aryPeerQueue_pv))
    CAryPeerQueue               _aryPeerQueue;   // list of elements for which we know a peer should be attached

    DWORD                       _dwPeerQueueProgressCookie; // taken while there are elements in the PeerQueue

    HRESULT PeerEnqueueTask (CBase * pTarget, PEERTASK task);
    NV_DECLARE_ONCALL_METHOD(PeerDequeueTasks, peerdequeuetasks, (DWORD_PTR fDocUnloadingNow));

    // misc behaviors

    DECLARE_CPtrAry(CAryPeerFactoriesUrl, CPeerFactoryUrl*, Mt(Mem), Mt(CDoc_aryPeerFactoriesUrl_pv))
    CAryPeerFactoriesUrl        _aryPeerFactoriesUrl;

#ifdef VSTUDIO7
    CStr                        _cstrIdentityFactoryUrl;
    CStr                        _cstrIdentityDocumentUrl;
#endif //VSTUDIO7

    HRESULT EnsureXmlUrnAtomTable(CXmlUrnAtomTable ** ppXmlUrnAtomTable);

    CXmlUrnAtomTable *          _pXmlUrnAtomTable;

    // eo behaviors

    // Filter tasks
    // These are elements that need filter hookup

    DECLARE_CPtrAry(CPendingFilterElementArray, CElement *, Mt(Mem), Mt(CFilter))
    CPendingFilterElementArray _aryPendingFilterElements;

    BOOL ExecuteSingleFilterTask(CElement *pElement);
    BOOL AddFilterTask(CElement *pElement);
    void RemoveFilterTask(CElement *pElement);

    BOOL ExecuteFilterTasks();
    void PostFilterCallback();
    NV_DECLARE_ONCALL_METHOD(FilterCallback, filtercallback, (DWORD_PTR unused));

    //ACCESSIBILITY Support
    CAccWindow *  _pAccWindow;
    ITypeInfo  *  _pAccTypeInfo;

    //
    // Helper function for view services methods.
    //
    
    HRESULT RegionFromMarkupPointers(   CMarkupPointer  *   pStart,
                                        CMarkupPointer  *   pEnd,
                                        CDataAry<RECT>  *   paryRects,
                                        RECT            *   pBoundingRect );

    CTreeNode *
    GetNodeFromPoint(
        const POINT &   pt,
        BOOL            fGlobalCoordinates,
        POINT *         pptGlobalPoint = NULL,
        LONG *          plCpHitMaybe = NULL ,
        BOOL*           pfEmptySpace = NULL );

    CFlowLayout * 
    GetFlowLayoutForSelection(
        CTreeNode *         pNode );

    CLayout * GetLayoutForSelection( CTreeNode * pNode );

    HRESULT
    MovePointerToPointInternal(
        POINT               tContentPoint,
        CTreeNode *         pNode,
        CMarkupPointer *    pPointer,
        BOOL *              pfNotAtBOL,
        BOOL *              pfAtLogicalBOL,
        BOOL *              pfRightOfCp,
        BOOL                fScrollIntoView,
        CLayout*            pContainingLayout, 
        BOOL*               pfSameLayout = NULL,
        BOOL                fHitTestEOL = TRUE );

    CMarkup *
    GetCurrentMarkup();
    
    // CHROME

    // Several events use ::GetCursorPos() and ::ScreenToClient()
    // to get clientX,Y coords. This is bad when Chrome hosted
    // (and hence running windowless) because the host has passed
    // mapped coords in the lParam of the mouse-message,
    // etc., and the code ignores them and uses generated
    // coordinates based on window handles instead (as they
    // event may not have been triggered by the mouse).
    // When running windowless we store the last set
    // of mouse coordinates away and use them in these cases.
    // When the event is not triggered by the mouse the
    // coordinates will not reflect the actual mouse
    // position but in that case very little can be guaranteed
    // about the position of the mouse anyway.
public:
    BOOL GetChromeCursorPos(POINT * ppt)
    {
        if (_chromeCursorPos._fInUse)
        {
            ppt->x = _chromeCursorPos._clientX;
            ppt->y = _chromeCursorPos._clientY;
        }
        return _chromeCursorPos._fInUse;
    }
private:
    struct ChromeCursorPos
    {
        int  _clientX;
        int  _clientY;
        BOOL _fInUse;
    } _chromeCursorPos;


    HRESULT SetDirtyFlag(BOOL fDirty);

public:
    long    _iFontHistoryVersion;      // for font history
    long    _iDocDotWriteVersion;      // for doc.write history
};


inline void
CDoc::DocumentFromWindow(POINTL *pptlDocOut, long xWinIn, long yWinIn)
{
    _dci.DocumentFromWindow(pptlDocOut, xWinIn, yWinIn);
}

inline void
CDoc::DocumentFromWindow(POINTL *pptlDocOut, POINT ptWinIn)
{
    _dci.DocumentFromWindow(pptlDocOut, ptWinIn.x, ptWinIn.y);
}

inline void
CDoc::DocumentFromWindow(SIZEL *pptlDocOut, SIZE ptWinIn)
{
    _dci.DocumentFromWindow(pptlDocOut, ptWinIn.cx, ptWinIn.cy);
}

#ifdef WIN16
inline void
CDoc::DocumentFromWindow(SIZEL *pptlDocOut, SIZES ptWinIn)
{
    _dci.DocumentFromWindow(pptlDocOut, ptWinIn.cx, ptWinIn.cy);
}
#endif

inline void
CDoc::DocumentFromWindow(SIZEL *psizelDocOut, long cxWinin, long cyWinIn)
{
    _dci.DocumentFromWindow(psizelDocOut, cxWinin, cyWinIn);
}

inline void
CDoc::HimetricFromDevice(POINTL *pptlDocOut, int xWinIn, int yWinIn)
{
    _dci.HimetricFromDevice(pptlDocOut, xWinIn, yWinIn);
}

inline void
CDoc::HimetricFromDevice(POINTL *pptlDocOut, POINT ptWinIn)
{
    _dci.HimetricFromDevice(pptlDocOut, ptWinIn.x, ptWinIn.y);
}

inline void
CDoc::HimetricFromDevice(SIZEL *psizelDocOut, int cxWinin, int cyWinIn)
{
    _dci.HimetricFromDevice(psizelDocOut, cxWinin, cyWinIn);
}

inline void
CDoc::HimetricFromDevice(SIZEL *pptlDocOut, SIZE ptWinIn)
{
    _dci.HimetricFromDevice(pptlDocOut, ptWinIn.cx, ptWinIn.cy);
}

inline void
CDoc::DeviceFromHimetric(POINT *pptWinOut, int xlDocIn, int ylDocIn)
{
    _dci.DeviceFromHimetric(pptWinOut, xlDocIn, ylDocIn);
}

inline void
CDoc::DeviceFromHimetric(POINT *pptPhysOut, POINTL ptlLogIn)
{
    _dci.DeviceFromHimetric(pptPhysOut, ptlLogIn.x, ptlLogIn.y);
}

inline void
CDoc::DeviceFromHimetric(SIZE *psizeWinOut, int cxlDocIn, int cylDocIn)
{
    _dci.DeviceFromHimetric(psizeWinOut, cxlDocIn, cylDocIn);
}

inline void
CDoc::DeviceFromHimetric(SIZE *pptPhysOut, SIZEL ptlLogIn)
{
    _dci.DeviceFromHimetric(pptPhysOut, ptlLogIn.cx, ptlLogIn.cy);
}


HRESULT
ExpandUrlWithBaseUrl(LPCTSTR pchBaseUrl, LPCTSTR pchRel, TCHAR ** ppchUrl);

HRESULT  ReadSettingsFromRegistry( TCHAR * pchKeyPath,
                                   const REGKEYINFORMATION* pAryKeys, int iKeyCount, void* pBase,
                                   DWORD dwFlags, BOOL fSettingsRead, void* pUserData );

#endif  //_FORMKRNL_HXX_
