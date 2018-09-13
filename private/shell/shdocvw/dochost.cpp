//
// NOTES:
//
//  This is the code which enables the explorer hosting (being a container)
// a DocObject (a super set of OLE in-place object). In a nut shell, this
// code creates an object (class CDocObjectHost) which can be plugged into
// the explorer's right pane (by supporting IShellView) is also a DocObject
// container (by supporting IOleClientSite, IOleInPlaceSite, ...).
//
//  This CDocObjectHost directly supports following interfaces:
//
// Group 1 (to be plugged in):
//   IShellView, IDropTarget
// Group 2 (to be a Doc site):
//   IOleClientSite, IOleDocumentSite
// Group 3 (to be a View Site)
//   IOleInPlaceSite
//
//  It also supports following interfaces indirectly via contained object,
// CDocObjectFrame.
//
//  IOleInPlaceFrame, IOleCommandTarget
//
//  The reason we export them separately is because we may need to return
// a different hwnd for GetWindow method. The CDocObjectHost object always
// returns hwnd of the view window, but the CDocObjectFrame returns hwnd
// of the explorer in case the explorer support IOleInPlaceUIWindow.
//
//  It also supports following interface indirectly via contained object,
// CProxyActiveObject.
//
//  IOleInPlaceActiveObject
//
//
//  --------------------------------------------------------
//      Explorer (browser)
//  --------------------------------------------------------
//        ^          |          |
//        |          |          |
//   ISB (+IOIUI)   ISV       IOIAO
//        |          |          |
//        |          V          |
//  ----------------------------V---------------------------
//       CDocObjectHost  CProxyActiveObject CDocObjectFrame
//  ----------------------------------------------^---------
//        ^                |                      |
//        |                |                      |
//  IOCS/IOIPS/IMDS   IO/IOIPO/IMV/IMCT    IOIUI/IOIF/IMCT
//        |                |                      |
//        |                V                      |
//  --------------------------------------------------------
//       DocObject (Doc + View)
//  --------------------------------------------------------
//

#include "priv.h"
#include "iehelpid.h"
#include "bindcb.h"
#include "winlist.h"
#include "droptgt.h"
#include <mshtml.h>     // CLSID_HTMLDocument
#include <mshtmcid.h>
#include "resource.h"
#include <htmlhelp.h>
#include <prsht.h>
#include <inetcpl.h>
#include <optary.h>
#include "impexp.h"
#include "impexpwz.h"
#include "thicket.h"
#include "uemapp.h"
#include "iextag.h"   // web folders
#include "browsext.h"

#include <mluisupp.h>

// temp, going away once itbar edit stuff moves here
#define  CITIDM_EDITPAGE  10
// Command group for private communication with CITBar
// 67077B95-4F9D-11D0-B884-00AA00B60104
const GUID CGID_PrivCITCommands = { 0x67077B95L, 0x4F9D, 0x11D0, 0xB8, 0x84,
0x00, 0xAA, 0x00, 0xB6, 0x01, 0x04 };
// end temp itbar stuff

#define  DBG_ACCELENTRIES 2
#define  OPT_ACCELENTRIES 1

#ifdef UNIX

#include <unixstuff.h>
#define  EXPLORER_EXE "explorer"
#define  IEXPLORE_EXE "iexplorer"
#define  DBG_ACCELENTRIES_WITH_FILEMENU 8
#define  OPT_ACCELENTRIES_WITH_FILEMENU 7

#else

#define  EXPLORER_EXE "explorer.exe"
#define  IEXPLORE_EXE "iexplore.exe"
#define  DBG_ACCELENTRIES_WITH_FILEMENU 6
#define  OPT_ACCELENTRIES_WITH_FILEMENU 5

#endif /* UNIX */

EXTERN_C const GUID IID_IDocHostObject  = {0x67431840L, 0xC511, 0x11CF, 0x89, 0xA9, 0x00, 0xA0, 0xC9, 0x05, 0x41, 0x29};
EXTERN_C const GUID IID_IMimeInfo       = {0xF77459A0L, 0xBF9A, 0x11cf, 0xBA, 0x4E, 0x00, 0xC0, 0x4F, 0xD7, 0x08, 0x16};
EXTERN_C const GUID IID_IsPicsBrowser   = {0xF114C2C0L, 0x90BE, 0x11D0, 0x83, 0xB1, 0x00, 0xC0, 0x4F, 0xD7, 0x05, 0xB2};

#include <shlwapi.h>
#include <ratings.h>

#define DM_ZONECROSSING 0
#define DM_SAVEASHACK   0
#define DM_MIMEMAPPING  0
#define DM_SELFASC      TF_SHDBINDING
#define DM_ACCEPTHEADER 0
#define DM_DEBUGTFRAME  0
#define DM_DOCHOSTUIHANDLER 0
#define DM_PREMERGEDMENU    0
#define DM_FOCUS        0
#define DM_DOCCP        0
#define DM_PICS         0
#define DM_SSL              0

// WARNING: Never define it in shipping product.
#ifdef DEBUG
// #define TEST_DELAYED_SHOWMSOVIEW
#endif

void CShdAdviseSink_Advise(IBrowserService* pwb, IOleObject* pole);
UINT MayOpenSafeOpenDialog(HWND hwndOwner, LPCTSTR pszFileClass, LPCTSTR pszURL, LPCTSTR pszCacheName, LPCTSTR pszDisplay, UINT uiCP);
LONG _GetSearchFormatString(DWORD dwIndex, LPTSTR psz, DWORD cbpsz);
DWORD _GetErrorThreshold(DWORD dwError);
HRESULT GetSearchKeys (LPDWORD pdwSearchForExtensions, LPDWORD pdwDo404Search);
BOOL IsRegisteredClient(LPCTSTR pszClient);

// macros
#define DO_SEARCH_ON_STATUSCODE(x) ((x == 0) || (x == HTTP_STATUS_BAD_GATEWAY) || (x == HTTP_STATUS_GATEWAY_TIMEOUT))

// Suite Apps Registry keys
#define NEW_MAIL_DEF_KEY            TEXT("Mail")
#define NEW_NEWS_DEF_KEY            TEXT("News")
#define NEW_CONTACTS_DEF_KEY        TEXT("Contacts")
#define NEW_CALL_DEF_KEY            TEXT("Internet Call")
#define NEW_APPOINTMENT_DEF_KEY     TEXT("Appointment")
#define NEW_MEETING_DEF_KEY         TEXT("Meeting")
#define NEW_TASK_DEF_KEY            TEXT("Task")
#define NEW_TASKREQUEST_DEF_KEY     TEXT("Task Request")
#define NEW_JOURNAL_DEF_KEY         TEXT("Journal")
#define NEW_NOTE_DEF_KEY            TEXT("Note")

#ifdef DEBUG
DWORD g_dwPerf = 0;
#endif

// #include "..\shell32\fstreex.h"              // for IDFOLDER
// HACK:
struct IDFOLDERA
{
    WORD    cb;
    BYTE    bFlags;
};
typedef IDFOLDERA* LPIDFOLDERA;

const ITEMIDLIST s_idNull = { {0} };

//
// Icons are globally shared among multiple threads.
//
HICON g_hiconSSL = NULL;
HICON g_hiconFortezza = NULL;
HICON g_hiconOffline = NULL;
HICON g_hiconPrinter = NULL;
HICON g_hiconScriptErr = NULL;

HICON g_ahiconState[IDI_STATE_LAST-IDI_STATE_FIRST+1] = { NULL };
#define MAX_MIXED_STR_LEN   32

// OpenUIURL is just a wrapper for OpenUI, calling CreateURLMoniker() if the
// caller only has an URL.

extern BOOL __cdecl _FormatMessage(LPCSTR szTemplate, LPSTR szBuf, UINT cchBuf, ...);

#include "asyncrat.h"



#define MAX_STATUS_SIZE 128

//
// Set this flag if we are going to use IHlinkBrowseContext in HLINK.DLL
// #define HLINK_EXTRA
//

#include "dochost.h"


#define DM_RECYCLE      DM_TRACE
#define DM_BINDAPPHACK  TF_SHDAPPHACK
#define DM_ADVISE       TF_SHDLIFE
#define DM_APPHACK      DM_WARNING

#define NAVMSG3(psz, x, y)      TraceMsg(0, "shdv NAV::%s %x %x", psz, x, y)
#define PAINTMSG(psz,x)         TraceMsg(0, "shd TR-PAINT::%s %x", psz, x)
#define JMPMSG(psz, psz2)       TraceMsg(0, "shd TR-CDOV::%s %s", psz, psz2)
#define JMPMSG2(psz, x)         TraceMsg(0, "shd TR-CDOV::%s %x", psz, x)
#define DOFMSG(psz)             TraceMsg(0, "shd TR-DOF::%s", psz)
#define DOFMSG2(psz, x)         TraceMsg(0, "shd TR-DOF::%s %x", psz, x)
#define URLMSG(psz)             TraceMsg(TF_SHDBINDING, "shd TR-DOF::%s", psz)
#define URLMSG2(psz, x)         TraceMsg(TF_SHDBINDING, "shd TR-DOF::%s %x", psz, x)
#define URLMSG3(psz, x, y)      TraceMsg(TF_SHDBINDING, "shd TR-DOF::%s %x %x", psz, x, y)
#define OIPSMSG(psz)            TraceMsg(0, "shd TR-OIPS::%s", psz)
#define OIPSMSG3(psz, sz, p)    TraceMsg(0, "shd TR-OIPS::%s %s,%x", psz, sz,p)
#define VIEWMSG(psz)            TraceMsg(0, "sdv TR CDOV::%s", psz)
#define VIEWMSG2(psz,xx)        TraceMsg(0, "sdv TR CDOV::%s %x", psz,xx)
#define OPENMSG(psz)            TraceMsg(TF_SHDBINDING, "shd OPENING %s", psz)
#define OPENMSG2(psz, x)        TraceMsg(TF_SHDBINDING, "shd OPENING %s %x", psz, x)
#define HFRMMSG(psz)            TraceMsg(0, "shd HFRM::%s", psz)
#define HFRMMSG2(psz, x, y)     TraceMsg(0, "shd HFRM::%s %x %x", psz, x, y)
#define MNKMSG(psz, psz2)       TraceMsg(0, "shd MNK::%s (%s)", psz, psz2)
#define CHAINMSG(psz, x)        TraceMsg(0, "shd CHAIN::%s %x", psz, x)
#define SHVMSG(psz, x, y)       TraceMsg(0, "shd SHV::%s %x %x", psz, x, y)
#define HOMEMSG(psz, psz2, x)   TraceMsg(TF_SHDNAVIGATE, "shd HOME::%s %s %x", psz, psz2, x)
#define SAVEMSG(psz, x)         TraceMsg(0, "shd SAVE::%s %x", psz, x)
#define PERFMSG(psz, x)         TraceMsg(TF_SHDPERF, "PERF::%s %d msec", psz, x)

static const TCHAR  szRegKey_SMIEM[] =              TEXT("Software\\Microsoft\\Internet Explorer\\Main");
static const TCHAR  szRegVal_ErrDlgPerErr[] =       TEXT("Error Dlg Displayed On Every Error");
static const TCHAR  szRegVal_ErrDlgDetailsOpen[] =  TEXT("Error Dlg Details Pane Open");

////////////////////////////////////////////////////////////
// ShabbirS (980917) - BugFix# 34259
// Repair menuitem in the Help Menu.

typedef HRESULT (* FIXIEPROC) (BOOL, DWORD);

void RepairIE()
{
    HINSTANCE   hIESetup;
    FIXIEPROC   fpFixIE;

    hIESetup = LoadLibrary(L"IESetup.dll");
    if (hIESetup)
    {
        fpFixIE = (FIXIEPROC) GetProcAddress(hIESetup,"FixIE");
        if (fpFixIE)
        {
            fpFixIE(TRUE,0);
        }
        FreeLibrary(hIESetup);
    }
}


BOOL _IsDesktopItem(CDocObjectHost * pdoh)
{
    BOOL fIsDesktopItem = FALSE;
    IServiceProvider  * psb;

    ASSERT(pdoh);
    //Check if we are a desktop component.
    if(SUCCEEDED(pdoh->QueryService(SID_STopLevelBrowser, IID_IServiceProvider, (void **)&psb)))
    {
        LPTARGETFRAME2  ptgf;
        if(SUCCEEDED(psb->QueryService(IID_ITargetFrame2, IID_ITargetFrame2, (void **)&ptgf)))
        {
            DWORD dwOptions;

            if(SUCCEEDED(ptgf->GetFrameOptions(&dwOptions)))
            {
                //Is this a desktop component?
                if(IsFlagSet(dwOptions, FRAMEOPTIONS_DESKTOP))
                    fIsDesktopItem = TRUE;
            }
            ptgf->Release();
        }
        psb->Release();
    }

    return fIsDesktopItem;
}

BOOL _IsImmediateParentDesktop(CDocObjectHost *pdoh, IServiceProvider *psp)
{
    BOOL    fImmediateParentIsDesktop = FALSE;
    LPTARGETFRAME2  ptgf;

    //First check if this is hosted on desktop.
    if(!_IsDesktopItem(pdoh))
        return FALSE;     //This is not a desktop item. So, the immediate parent can't be desktop!

    //We know that this is a desktop item. Check if the immediate parent is desktop
    // or it is hosted too deep on desktop!
    if(psp && SUCCEEDED(psp->QueryService(IID_ITargetFrame2, IID_ITargetFrame2, (void **)&ptgf)))
    {
        LPUNKNOWN pUnkParent;

        //Get it's immediate parent.
        if(SUCCEEDED(ptgf->GetParentFrame(&pUnkParent)))
        {
            if(pUnkParent)
            {
                //Has a parent. So, the immediate parent can't be desktop!
                pUnkParent->Release();

                fImmediateParentIsDesktop = FALSE;
            }
            else
                fImmediateParentIsDesktop = TRUE; //No parent. Must be a desktop comp.
        }
        ptgf->Release();
    }
    return(fImmediateParentIsDesktop);
}

//Gets the current display name in wide char
//
// If fURL is TRUE, it returns file-URL with file: prefix.
//
HRESULT CDocObjectHost::_GetCurrentPageW(LPOLESTR * ppszDisplayName, BOOL fURL)
{
    HRESULT hres = E_FAIL;
    ASSERT(_pmkCur);

    *ppszDisplayName = NULL;

    if (_pmkCur) {
        IBindCtx* pbc;
        hres = CreateBindCtx(0, &pbc);
        if (SUCCEEDED(hres))
        {
            hres = _pmkCur->GetDisplayName(pbc, NULL, ppszDisplayName);

            //
            //  special handling just for file: urls.
            //
            if (SUCCEEDED(hres) && _fFileProtocol)
            {
                ASSERT(*ppszDisplayName);

                WCHAR szText[MAX_URL_STRING];
                DWORD cchText = SIZECHARS(szText);
                if (!fURL)
                {
                     hres = PathCreateFromUrlW(*ppszDisplayName, szText, &cchText, 0);
                }
                else
                {
                    //  we need this to be in the normalized form of the URL
                    //  for internal usage.  urlmon keeps them in the funny PATHURL style
                    hres = UrlCanonicalizeW(*ppszDisplayName, szText, &cchText, 0);
                }

                if (SUCCEEDED(hres))
                {
                    UINT cchDisplayName = lstrlenW(*ppszDisplayName);

                    if (cchText > cchDisplayName)
                    {
                        //  need to resize
                        CoTaskMemFree(*ppszDisplayName);
                        *ppszDisplayName = (WCHAR *)CoTaskMemAlloc((cchText + 1) * SIZEOF(WCHAR));

                        if (*ppszDisplayName)
                        {
                            //  go ahead and copy it in
                            StrCpyNW(*ppszDisplayName, szText, cchText + 1);
                        }
                        else
                            hres = E_OUTOFMEMORY;
                    }
                    else
                    {
                        StrCpyNW(*ppszDisplayName, szText, cchDisplayName + 1);
                    }
                }
                else
                    OleFree(*ppszDisplayName);
            }
            pbc->Release();
        }
    }

    return hres;
}


HRESULT CDocObjectHost::_GetCurrentPage(LPTSTR szBuf, UINT cchMax, BOOL fURL)
{
    szBuf[0] = 0;   // zero out buffer

    WCHAR *pszDisplayName;
    HRESULT hres = _GetCurrentPageW(&pszDisplayName, fURL);
    if (SUCCEEDED(hres))
    {
        StrCpyN(szBuf, pszDisplayName, cchMax);
        OleFree(pszDisplayName);
    }

    return hres;
}

void CDocObjectHost_GetCurrentPage(LPARAM that, LPTSTR szBuf, UINT cchMax)
{
    CDocObjectHost* pdoh = (CDocObjectHost*)that;
    pdoh->_GetCurrentPage(szBuf, cchMax);
}

//========================================================================
// CDocObjectHost members
//========================================================================

CDocObjectHost::CDocObjectHost() : _cRef(1), _uState(SVUIA_DEACTIVATE)
{
    DllAddRef();
    TraceMsg(TF_SHDLIFE, "ctor CDocObjectHost %x", this);
    TraceMsg(DM_DEBUGTFRAME, "ctor CDocObjectHost %x, %x", this, &_bsc);

    // Initialize proxy objects (which are contained)
    _dof.Initialize(this);
    _xao.Initialize(this);

#ifdef HLINK_EXTRA
    HRESULT hres = HlinkCreateBrowseContext(NULL, IID_IHlinkBrowseContext, (LPVOID*)&_pihlbc);
    TraceMsg(0, "sdv TR CDOV::constructor HlinkCreateBrowseContext returned %x", hres);
#endif // HLINK_EXTRA

    _fPicsAccessAllowed = 1;     /* assume no ratings checks unless we download */
    _fbPicsWaitFlags = 0;

    ::_RefPicsQueries();    /* we'll free PICS async query list when last dochost is destroyed */

    _pScriptErrList = NULL;
    _fScriptErrDlgOpen = FALSE;

    _strPriorityStatusText = NULL;

    _iString = -1;
    _uiCP = CP_ACP;
}

CDocObjectHost::~CDocObjectHost()
{
    ASSERT(_pole==NULL);    // to catch extra release.
    ASSERT(_psp==NULL);     // to cache extra release.
    ASSERT(_hwnd==NULL);
    ASSERT(_pmsoc==NULL);
    ASSERT(_pmsot==NULL);
    ASSERT(_pmsov==NULL);
    ASSERT(_pcmdMergedMenu==NULL);

    if (_pScriptErrList != NULL)
    {
        _pScriptErrList->Release();
    }

    if (_strPriorityStatusText != NULL)
    {
        SysFreeString(_strPriorityStatusText);
    }

#ifdef HLINK_EXTRA
    ASSERT(_phls == NULL);
    ATOMICRELEASE(_pihlbc);
#endif // HLINK_EXTRA

    if (_pRatingDetails) {
        ::RatingFreeDetails(_pRatingDetails);
        _pRatingDetails = NULL;
    }

    if (_dwPicsSerialNumber) {
        ::_RemovePicsQuery(_dwPicsSerialNumber);
        _dwPicsSerialNumber = 0;
    }

    if (_hPicsQuery)
    {
        RatingObtainCancel(_hPicsQuery);
        _hPicsQuery = NULL;
    }

    delete _pszPicsURL;     /* safe if already NULL */
    _pszPicsURL = NULL;

    ::_ReleasePicsQueries();

    if (_pRootDownload != NULL) {
        ASSERT(0);  /* need to destroy this earlier to prevent Trident problems */
        ATOMICRELEASET(_pRootDownload,CPicsRootDownload);
    }

    if (_padvise) {
        _padvise->OnClose();
        ATOMICRELEASE(_padvise);
    }

    if (_pwszRefreshUrl)
        OleFree(_pwszRefreshUrl);

    if (_hmenuBrowser) {
        AssertMsg(0, TEXT("_hmenuBrowser should be NULL!"));
        DestroyMenu(_hmenuBrowser);
    }

    if (_hmenuFrame) {
        DestroyMenu(_hmenuFrame);
    }

    if (_hacc)
    {
        DestroyAcceleratorTable(_hacc);
        _hacc = NULL;
    }

    if (_hinstInetCpl)
        FreeLibrary(_hinstInetCpl);

    if (_ptbStd)
        delete [] _ptbStd;

    if (_pBrowsExt)
    {
        _pBrowsExt->Release();
    }

    // Make it sure that View Window is released (and _psb)
    DestroyHostWindow();        // which will call _CloseMsoView and _UnBind

    _ResetOwners();

    TraceMsg(TF_SHDLIFE, "dtor CDocObjectHost %x", this);
    DllRelease();
}


#ifdef DEBUG
/*----------------------------------------------------------
Purpose: Dump the menu handles for this docobj.  Optionally
         breaks after dumping handles.

Returns:
Cond:    --
*/
void
CDocObjectHost::_DumpMenus(
    IN LPCTSTR pszMsg,
    IN BOOL    bBreak)
{
    if (IsFlagSet(g_dwDumpFlags, DF_DEBUGMENU))
    {
        ASSERT(pszMsg);

        TraceMsg(TF_ALWAYS, "DocHost: Dumping menus for %#08x %s", (LPVOID)this, pszMsg);
        TraceMsg(TF_ALWAYS, "   _hmenuBrowser = %x, _hmenuSet = %x, _hmenuFrame = %x",
                 _hmenuBrowser, _hmenuSet, _hmenuFrame);
        TraceMsg(TF_ALWAYS, "   _hmenuCur = %x, _hmenuMergedHelp = %x, _hmenuObjHelp = %x",
                 _hmenuCur, _hmenuMergedHelp, _hmenuObjHelp);

        _menulist.Dump(pszMsg);

        if (bBreak && IsFlagSet(g_dwBreakFlags, BF_ONDUMPMENU))
            DebugBreak();
    }
}
#endif

HRESULT CDocObjectHost::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDocObjectHost, IOleInPlaceSite),
        QITABENTMULTI(CDocObjectHost, IOleWindow, IOleInPlaceSite),
        QITABENT(CDocObjectHost, IOleClientSite),
        QITABENT(CDocObjectHost, IOleDocumentSite),
        QITABENT(CDocObjectHost, IOleCommandTarget),
        QITABENT(CDocObjectHost, IServiceProvider),
        QITABENT(CDocObjectHost, IViewObject),
        QITABENT(CDocObjectHost, IAdviseSink),
        QITABENT(CDocObjectHost, IDocHostObject),
        QITABENT(CDocObjectHost, IDocHostUIHandler),
        QITABENT(CDocObjectHost, IDocHostShowUI),
        QITABENT(CDocObjectHost, IDispatch),
        QITABENT(CDocObjectHost, IPropertyNotifySink),
        QITABENT(CDocObjectHost, IOleControlSite),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

void CDocObjectHost::_ResetOwners()
{
    _pszLocation = NULL;
    _uiCP = CP_ACP;

    _ReleasePendingObject();

    ATOMICRELEASE(_psv);
    ATOMICRELEASE(_pmsoctView);
    ATOMICRELEASE(_pdvs);
    ATOMICRELEASE(_psb);
    ATOMICRELEASE(_pwb);
    ATOMICRELEASE(_phf);
    ATOMICRELEASE(_pocthf);
    ATOMICRELEASE(_punkSFHistory);
    ATOMICRELEASE(_pmsoctBrowser);
    ATOMICRELEASE(_psp);
    ATOMICRELEASE(_peds);
    ATOMICRELEASE(_pedsHelper);
    ATOMICRELEASE(_pWebOCUIHandler);
    ATOMICRELEASE(_pWebOCShowUI);

    // Release cached OleInPlaceUIWindow of the browser
    ATOMICRELEASE(_pipu);

    // Tell embedded CDocHostUIHandler object to release its references on us.
    _dhUIHandler.SetSite(NULL);
}



ULONG CDocObjectHost::AddRef()
{
    _cRef++;
    TraceMsg(TF_SHDREF, "CDocObjectHost(%x)::AddRef called, new _cRef=%d", this, _cRef);
    return _cRef;
}

ULONG CDocObjectHost::Release()
{
    _cRef--;
    TraceMsg(TF_SHDREF, "CDocObjectHost(%x)::Release called, new _cRef=%d", this, _cRef);
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

// cut & paste from browseui\itbar.cpp
int RemoveHiddenButtons(TBBUTTON* ptbn, int iCount)
{
    int i;
    int iTotal = 0;
    TBBUTTON* ptbn1 = ptbn;
    for (i = 0; i < iCount; i++, ptbn1++) {
        if (!(ptbn1->fsState & TBSTATE_HIDDEN)) {
            if (ptbn1 != ptbn) {
                *ptbn = *ptbn1;
            }
            ptbn++;
            iTotal++;
        }
    }
    return iTotal;
}

// We use two different image lists in the TBBUTTON array.  The bitmaps for browser-specific buttons
// cut/copy/paste have been moved to shdocvw, and are therefore obtained from a second image list.
// MAKELONG(0,1) accesses the first image from this second list.  Without a call to MAKELONG there is
// a 0 in the upper integer, thereby referencing the first list by default.
static const TBBUTTON c_tbStd[] = {
    {10, DVIDM_SHOWTOOLS,       TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, 10},
    {13, DVIDM_MAILNEWS,        TBSTATE_ENABLED, BTNS_WHOLEDROPDOWN, {0,0}, 0, 13 },
    { 8, DVIDM_FONTS,           TBSTATE_ENABLED, BTNS_WHOLEDROPDOWN, {0,0}, 0, 8 },
    { 7, DVIDM_PRINT,           TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, 7 },
    { 9, DVIDM_EDITPAGE,        TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, 9 },
    {15, DVIDM_DISCUSSIONS,     TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, 15 },
    {MAKELONG(0,1), DVIDM_CUT,             TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, 0 },
    {MAKELONG(1,1), DVIDM_COPY,            TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, 0 },
    {MAKELONG(2,1), DVIDM_PASTE,           TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, 0 },
    {MAKELONG(3,1), DVIDM_ENCODING,        TBSTATE_ENABLED, BTNS_WHOLEDROPDOWN, {0,0}, 0, 0 },
};

// c_tbStd and c_rest need to match exactly
static const BROWSER_RESTRICTIONS c_rest[] = {
    REST_BTN_TOOLS,
    REST_BTN_MAIL,
    REST_BTN_FONTS,
    REST_BTN_PRINT,
    REST_BTN_EDIT,
    REST_BTN_DISCUSSIONS,
    REST_BTN_CUT,
    REST_BTN_COPY,
    REST_BTN_PASTE,
    REST_BTN_ENCODING,
};

#ifdef DEBUG
void _AssertRestrictionOrderIsCorrect()
{
    COMPILETIME_ASSERT(ARRAYSIZE(c_tbStd) == ARRAYSIZE(c_rest));

    for (UINT i = 0; i < ARRAYSIZE(c_tbStd); i++)
    {
        // If any of these rip, it means that c_rest and c_tbStd have
        // gotten out of sync.  Need to fix up c_rest to match c_tbStd.
        switch (c_tbStd[i].idCommand)
        {
            case DVIDM_SHOWTOOLS:       ASSERT(c_rest[i] == REST_BTN_TOOLS);        break;
            case DVIDM_MAILNEWS:        ASSERT(c_rest[i] == REST_BTN_MAIL);         break;
            case DVIDM_FONTS:           ASSERT(c_rest[i] == REST_BTN_FONTS);        break;
            case DVIDM_PRINT:           ASSERT(c_rest[i] == REST_BTN_PRINT);        break;
            case DVIDM_EDITPAGE:        ASSERT(c_rest[i] == REST_BTN_EDIT);         break;
            case DVIDM_DISCUSSIONS:     ASSERT(c_rest[i] == REST_BTN_DISCUSSIONS);  break;
            case DVIDM_CUT:             ASSERT(c_rest[i] == REST_BTN_CUT);          break;
            case DVIDM_COPY:            ASSERT(c_rest[i] == REST_BTN_COPY);         break;
            case DVIDM_PASTE:           ASSERT(c_rest[i] == REST_BTN_PASTE);        break;
            case DVIDM_ENCODING:        ASSERT(c_rest[i] == REST_BTN_ENCODING);     break;
            default:                    ASSERT(0);                                  break;
        }
    }
}
#endif

BYTE _BtnStateFromRestIfAvailable(BOOL fAvailable, DWORD dwRest)
{
    if (fAvailable)
        return SHBtnStateFromRestriction(dwRest, TBSTATE_ENABLED);

    return TBSTATE_HIDDEN;
}

BOOL CDocObjectHost::_ToolsButtonAvailable()
{
    OLECMD rgcmd = { OLECMDID_HIDETOOLBARS, 0 };

    if (_pmsoctBrowser)
        _pmsoctBrowser->QueryStatus(NULL, 1, &rgcmd, NULL);

    return (rgcmd.cmdf & OLECMDF_SUPPORTED);
}

__inline BYTE CDocObjectHost::_DefToolsButtonState(DWORD dwRest)
{
    BOOL fAvailable = _ToolsButtonAvailable();
    return _BtnStateFromRestIfAvailable(fAvailable, dwRest);
}

static const TCHAR c_szRegKeyCoolbar[] = TEXT("Software\\Microsoft\\Internet Explorer\\Toolbar");

BYTE CDocObjectHost::_DefFontsButtonState(DWORD dwRest)
{
    BYTE fsState = TBSTATE_ENABLED;

    // default to whatever the IE4 reg key specifies,
    // or FALSE if reg key not present (clean install)
    if (!SHRegGetBoolUSValue(c_szRegKeyCoolbar, TEXT("ShowFonts"), FALSE, FALSE))
        fsState |= TBSTATE_HIDDEN;

    return SHBtnStateFromRestriction(dwRest, fsState);
}

DWORD CDocObjectHost::_DiscussionsButtonCmdf()
{
    if (SHRegGetBoolUSValue(c_szRegKeyCoolbar,
                                TEXT("ShowDiscussionButton"), FALSE, TRUE) &&
       _pmsoctBrowser) {

        OLECMD rgcmds[] = {
            { SBCMDID_DISCUSSIONBAND, 0 },
        };
        static const int buttonsInternal[] = {
            DVIDM_DISCUSSIONS,
        };
        _pmsoctBrowser->QueryStatus(&CGID_Explorer, ARRAYSIZE(rgcmds), rgcmds, NULL);
        return rgcmds[0].cmdf;
    }

    return 0;
}

__inline BOOL CDocObjectHost::_DiscussionsButtonAvailable()
{
    return (_DiscussionsButtonCmdf() & OLECMDF_SUPPORTED);
}

__inline BYTE CDocObjectHost::_DefDiscussionsButtonState(DWORD dwRest)
{
    BOOL fAvailable = _DiscussionsButtonAvailable();
    return _BtnStateFromRestIfAvailable(fAvailable, dwRest);
}

BOOL CDocObjectHost::_MailButtonAvailable()
{
    OLECMD rgcmdMailFavs[] = { { SBCMDID_DOMAILMENU, 0} };

    if (_pmsoctBrowser)
        _pmsoctBrowser->QueryStatus(&CGID_Explorer, ARRAYSIZE(rgcmdMailFavs), rgcmdMailFavs, NULL);

    if (rgcmdMailFavs[0].cmdf & OLECMDF_ENABLED)
        return TRUE;

    return FALSE;
}

__inline BYTE CDocObjectHost::_DefMailButtonState(DWORD dwRest)
{
    BOOL fAvailable = _MailButtonAvailable();
    return _BtnStateFromRestIfAvailable(fAvailable, dwRest);
}


// We default the edit button to visible if there is an html editer registered
BOOL CDocObjectHost::_EditButtonAvailable()
{
    BOOL fRet = FALSE;
    HKEY hkey = NULL;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT(".htm\\shell\\edit\\command"), 0, KEY_READ, &hkey) ||
        ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("htmlfile\\shell\\edit\\command"), 0, KEY_READ, &hkey))
    {
        RegCloseKey(hkey);
        fRet = TRUE;
    }
    return fRet;
}

__inline BYTE CDocObjectHost::_DefEditButtonState(DWORD dwRest)
{
    BYTE fsState;

    if (_EditButtonAvailable())
        fsState = TBSTATE_ENABLED;
    else
        fsState = TBSTATE_HIDDEN;

    return SHBtnStateFromRestriction(dwRest, fsState);
}

void CDocObjectHost::_MarkDefaultButtons(PTBBUTTON tbStd)
{
    // We're assuming tbStd is the same size as c_tbStd

#ifdef DEBUG
    _AssertRestrictionOrderIsCorrect();
#endif

    DWORD dwRest[ARRAYSIZE(c_tbStd)];

    BOOL fCheckRestriction = SHRestricted2(REST_SPECIFYDEFAULTBUTTONS, NULL, 0);
    for (UINT i = 0; i < ARRAYSIZE(c_rest); i++) {
        if (fCheckRestriction)
            dwRest[i] = SHRestricted2(c_rest[i], NULL, 0);
        else
            dwRest[i] = RESTOPT_BTN_STATE_DEFAULT;
    }

    // We want the Cut, Copy, Paste buttons to default off of the toolbar
    // (but available in the view-toolbars-customize dialog)
    // We set the state of the buttons to TBSTATE_HIDDEN here, but leave them alone
    // in ETCMDID_GETBUTTONS so that they appear in the customize dialog.

    ASSERT(tbStd[6].idCommand == DVIDM_CUT);
    ASSERT(tbStd[7].idCommand == DVIDM_COPY);
    ASSERT(tbStd[8].idCommand == DVIDM_PASTE);
    ASSERT(tbStd[9].idCommand == DVIDM_ENCODING);

    for (i = 6; i <= 9; i++)
        tbStd[i].fsState = SHBtnStateFromRestriction(dwRest[i], tbStd[i].fsState | TBSTATE_HIDDEN);

    ASSERT(tbStd[0].idCommand == DVIDM_SHOWTOOLS);
    tbStd[0].fsState = _DefToolsButtonState(dwRest[0]);

    ASSERT(tbStd[1].idCommand == DVIDM_MAILNEWS);
    tbStd[1].fsState = _DefMailButtonState(dwRest[1]);

    ASSERT(tbStd[2].idCommand == DVIDM_FONTS);
    tbStd[2].fsState = _DefFontsButtonState(dwRest[2]);

    ASSERT(tbStd[3].idCommand == DVIDM_PRINT);
    tbStd[3].fsState = SHBtnStateFromRestriction(dwRest[3], TBSTATE_ENABLED);

    ASSERT(tbStd[4].idCommand == DVIDM_EDITPAGE);
    tbStd[4].fsState = _DefEditButtonState(dwRest[4]);

    ASSERT(tbStd[5].idCommand == DVIDM_DISCUSSIONS);
    tbStd[5].fsState = _DefDiscussionsButtonState(dwRest[5]);
}

const GUID* CDocObjectHost::_GetButtonCommandGroup()
{
    if (_ToolsButtonAvailable())
        return &CLSID_MSOButtons;
    else
        return &CLSID_InternetButtons;
}

void CDocObjectHost::_AddButtons(BOOL fForceReload)
{
    if (!_pBrowsExt)
        return;

    IExplorerToolbar* pxtb;
    if (_psp && SUCCEEDED(_psp->QueryService(SID_SExplorerToolbar, IID_IExplorerToolbar, (LPVOID*)&pxtb)))
    {
        const GUID* pguid = _GetButtonCommandGroup();

        HRESULT hr = pxtb->SetCommandTarget((IOleCommandTarget*)this, pguid,
                        VBF_TOOLS | VBF_ADDRESS | VBF_LINKS | VBF_BRAND);

        if (!fForceReload && hr == S_FALSE) {
            // Another dochost already merged the buttons into the toolbar under the
            // same command group, so don't bother re-merging.  We just need to initialize
            // _iString, since we're skipping the call to _pBrowsExt->InitButtons below.
            VARIANT var = { VT_I4 };
            IUnknown_Exec(_pBrowsExt, &CLSID_PrivBrowsExtCommands, PBEC_GETSTRINGINDEX, 0, &var, NULL);   // should always succeed
            _iString = var.lVal;
        } else {

            UINT nNumExtButtons = 0;

            _pBrowsExt->GetNumButtons(&nNumExtButtons);

            int nNumButtons = nNumExtButtons + ARRAYSIZE(c_tbStd);

            // GetTBArray insures that tbStd != NULL, so we don't need that check here
            TBBUTTON    *tbStd = new TBBUTTON[nNumButtons];

            if (tbStd != NULL)
            {
                memcpy(tbStd, c_tbStd, SIZEOF(TBBUTTON) * ARRAYSIZE(c_tbStd));

                UINT iStringIndex = (UINT)-1;  // result of adding the string buffer to the toolbar string list
                HRESULT hr = _pBrowsExt->InitButtons(pxtb, &iStringIndex, pguid);

                ASSERT(tbStd[6].idCommand == DVIDM_CUT);
                ASSERT(tbStd[7].idCommand == DVIDM_COPY);
                ASSERT(tbStd[8].idCommand == DVIDM_PASTE);
                ASSERT(tbStd[9].idCommand == DVIDM_ENCODING);

                if (SUCCEEDED(hr) && iStringIndex != -1)
                {
                    tbStd[6].iString = iStringIndex;
                    tbStd[7].iString = iStringIndex + 1;
                    tbStd[8].iString = iStringIndex + 2;
                    tbStd[9].iString = iStringIndex + 3;
                    _iString = (int)iStringIndex;
                }
                else
                {
                    tbStd[6].iString = tbStd[7].iString = tbStd[8].iString = tbStd[9].iString = -1;
                    _iString = -1;
                }

                // Add custom buttons to the toolbar array.  We pass in the nNumButtons
                // as a *sanity check*...
                _pBrowsExt->GetButtons(&tbStd[ARRAYSIZE(c_tbStd)], nNumExtButtons, TRUE);

                _MarkDefaultButtons(tbStd);

                nNumButtons = RemoveHiddenButtons(tbStd, nNumButtons);

                pxtb->AddButtons(pguid, nNumButtons, tbStd);

                delete [] tbStd;
            }
        }

        pxtb->Release();
    }
}

HRESULT CDocObjectHost::UIActivate(UINT uState, BOOL fPrevViewIsDocView)
{
    TraceMsg(TF_SHDUIACTIVATE, "DOH::UIActivate called %d->%d (this=%x)",
             _uState, uState, this);

    HRESULT hres = S_OK;
    UINT uStatePrev = _uState;

    // We are supposed to update the menu
    if (uState != _uState)
    {
        // There was a state transition.
        //
        _uState = uState;

        // If the new state is SVUIA_DEACTIVATE
        //
        if (_uState == SVUIA_DEACTIVATE)
        {
            //
            //  When we are deactivating (we are navigating away)
            // we UIDeactivate the current MsoView.
            //
            _UIDeactivateMsoView();

            _IPDeactivateMsoView(_pmsov);
            _DestroyBrowserMenu();
        }
        else if (_uState == SVUIA_INPLACEACTIVATE && uStatePrev == SVUIA_ACTIVATE_FOCUS)
        {
            // Transition from SVUIA_ACTIVATE_FOCUS->SVUIA_INPLACEACTIVATE
            //
            // BUGBUG: If we set this DONT_UIDEACTIVATE, then we stop calling
            //  UIActivate(FALSE) when a DocObject in a frameset looses a focus.
            //  It will solve some problems with Office apps (Excel, PPT), which
            //  InPlaceDeactivate when we call UIActivate(FALSE). We want to treat
            //  it as a bug, but unfortunately DocObject spec says that's OK.
            //
            //   Putting this work around, however, slightly confuses MSHTML
            //  (both classic and Trident). Once it's UIActivated, it keep
            //  thinking that it's UIActivated and never calls onUIActivate.
            //  Until we figure out what's the right implementation,
            //  we can't turn this on.             (SatoNa -- 11/04/96).
            //
            _GetAppHack(); // get if we don't have it yet.
            if (_dwAppHack & BROWSERFLAG_DONTUIDEACTIVATE) {
                //
                // HACK: We are supposed to just call UIActivate(FALSE) when
                //  another DocObject (in the case of a frame set) became
                //  UIActivated. Excel/PPT, however, InplaceDeactivate instead.
                //  To work around, SriniK suggested us to call
                //  OnDocWindowActivate(FALSE). (SatoNa)
                //
                IOleInPlaceActiveObject* piact = _xao.GetObject(); // no AddRef
                TraceMsg(TF_SHDAPPHACK, "DOH::UIActivate APPHACK calling %x->OnDocWindowActivate (this=%x)",
                         piact, this);
                if (piact)
                {
                    piact->OnDocWindowActivate(FALSE);
                }
            }
            else if (!(_dwAppHack & BROWSERFLAG_DONTDEACTIVATEMSOVIEW))
            {
                // HACK: In Excel, if we deactiveate the view, it never gets focus again
                // fix for the bug from hell: #20906
                _UIDeactivateMsoView();
            }
            else
            {
                // We're transitioning from SVUIA_ACTIVATE_FOCUS->SVUIA_INPLACEACTIVATE
                // and BROWSERFLAG_DONTDEACTIVATEMSOVIEW is set.
                // call the object's IOleInPlactActiveObject::OnFrameWindowActivate(FALSE);
                IOleInPlaceActiveObject* piact = _xao.GetObject(); // no AddRef
                if (piact)
                {
                    piact->OnFrameWindowActivate(FALSE);
                }
            }
        }
        else if (uStatePrev == SVUIA_DEACTIVATE)
        {

            //
            //  If UIActivate is called either
            // (1) when the binding is pending; _bsc._pbc!=NULL
            // (2) when the async binding is done; _bsc._pole!=NULL
            //
            SHVMSG("UIActivate about to call _Bind", _bsc._pbc, NULL);
            if (_pole == NULL && _bsc._pbc)
            {
                ASSERT(_pmkCur);
                IBindCtx* pbc = _bsc._pbc;
                pbc->AddRef();
                HRESULT hresT = _BindSync(_pmkCur, _bsc._pbc, _bsc._psvPrev);
                pbc->Release();
                ASSERT(_bsc._pbc==NULL);
                ASSERT(_bsc._psvPrev==NULL);
                ASSERT(_bsc._pbscChained==NULL);
            }

            hres = _EnsureActivateMsoView();

            _AddButtons(FALSE);

        } else {
            // opening a new document for 1st time (to UIActive or IPActive)
            goto GoSetFocus;
        }
    }
    else
    {
        TraceMsg(TF_SHDUIACTIVATE, "DOH:::UIActivate -- same uState (%x)", _uState);
GoSetFocus:
        if ((_uState == SVUIA_ACTIVATE_FOCUS)) {
            // see if object is already UIActive.
            if (_ActiveHwnd()) {
                // if it is, we have an hwnd and all we need to do
                // is SetFocus (for compatibility w/ weird guys...)

                if ( IsChildOrSelf( _ActiveHwnd(), GetFocus() ) != S_OK )
                {
                    TraceMsg(TF_SHDUIACTIVATE, "DOH:::UIActivate -- calling SetFocus(%x)", _ActiveHwnd());
                    SetFocus(_ActiveHwnd());
                }
            }
            else {
                // we're in the OC, and it's IPActive not UIActive.
                // (either that or it's the very 1st time for the main view).
                // NOTE: Due to CBaseBrowser code that defers SVUIA_ACTIVATE_FOCUS until
                // application is active, we can have a top level docobject go
                // SVUIA_INPLACEACTIVE and then on activation of the window,
                // we transition to SVUIA_ACTIVATE_FOCUS, thus never UIActivating
                // the docobject (cf: BUG 62138)

                hres = _DoVerbHelper(FALSE);
            }
        }
    }
    if ((_uState == SVUIA_INPLACEACTIVATE) || (_uState  == SVUIA_ACTIVATE_FOCUS))
        _PlaceProgressBar();

    return hres;
}

//***   _DoVerbHelper -- DoVerb w/ various hacks
// NOTES
//  BUGBUG do comments in _OnSetFocus apply here?
HRESULT CDocObjectHost::_DoVerbHelper(BOOL fOC)
{
    HRESULT hres = E_FAIL;
    LONG iVerb = OLEIVERB_SHOW;
    MSG msg;
    LPMSG pmsg = NULL;

    if (_uState == SVUIA_INPLACEACTIVATE) {
        iVerb = OLEIVERB_INPLACEACTIVATE;
    }
    else if ((_uState == SVUIA_ACTIVATE_FOCUS)) {
        iVerb = OLEIVERB_UIACTIVATE;
    }
    else {
        TraceMsg(TF_ERROR, "DOC::_DoVerbHelper untested (and probably the wrong iVerb mapping)");
    }
    if (_pedsHelper)
    {
        if (SUCCEEDED(_pedsHelper->GetDoVerbMSG(&msg)))
        {
            pmsg = &msg;
        }
    }
    hres = _pole->DoVerb(iVerb, pmsg, this, (UINT)-1, _hwnd, &_rcView);
    if (hres == OLEOBJ_E_INVALIDVERB && iVerb == OLEIVERB_INPLACEACTIVATE) {
        hres = _pole->DoVerb(OLEIVERB_SHOW, pmsg, this, (UINT)-1, _hwnd, &_rcView);
    }

    if (FAILED(hres)) {
        TraceMsg(DM_ERROR, "DOC::_DoVerbHelper _pole->DoVerb ##FAILED## %x", hres);
    }

    return hres;
}

class CDocObjAssoc : public IUnknown
{
public:
    // *** IUnknown methods ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    CDocObjAssoc(DWORD dwDOCHF) : _dwDOCHF(dwDOCHF), _cRef(1) {
        TraceMsg(TF_SHDLIFE, "ctor CDocObjAssoc(%x) being constructed", this);
    }
    DWORD GetDOCHF(void) { return _dwDOCHF; }
protected:
    ~CDocObjAssoc() {
        TraceMsg(TF_SHDLIFE, "dtor CDocObjAssoc(%x) being destroyed", this);
    }

    UINT    _cRef;
    DWORD   _dwDOCHF;
};

HRESULT CDocObjAssoc::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IUnknown*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

ULONG CDocObjAssoc::AddRef()
{
    _cRef++;
    TraceMsg(TF_SHDREF, "CDocObjAssoc(%x)::AddRef called, new _cRef=%d", this, _cRef);
    return _cRef;
}

ULONG CDocObjAssoc::Release()
{
    _cRef--;
    TraceMsg(TF_SHDREF, "CDocObjAssoc(%x)::Release called, new _cRef=%d", this, _cRef);
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

void CDocObjectHost::_IPDeactivateMsoView(IOleDocumentView* pmsov)
{
    TraceMsg(TF_SHDUIACTIVATE, "DOH::_IPDeactivateMsoView called (this==%x)", this);

    if (pmsov)
    {
        IOleInPlaceObject* pipo = NULL;
        HRESULT hresT = _pole->QueryInterface(IID_IOleInPlaceObject, (LPVOID*)&pipo);
        if (SUCCEEDED(hresT))
        {
            pipo->InPlaceDeactivate();
            pipo->Release();
        }
        pmsov->Show(FALSE);
    }
}

void CDocObjectHost::_UIDeactivateMsoView(void)
{
    TraceMsg(TF_SHDUIACTIVATE, "DOH::_UIDeactivateMsoView called (this==%x)", this);

    if (_pmsov)
    {
        _pmsov->UIActivate(FALSE);
    }
}

//
// Hide the office toolbar
//
void CDocObjectHost::_HideOfficeToolbars(void)
{
    if (_pmsot) {
        OLECMD rgcmd = { OLECMDID_HIDETOOLBARS, 0 };

        _pmsot->QueryStatus(NULL, 1, &rgcmd, NULL);

        // LATCHED means hidden
        rgcmd.cmdf &= (OLECMDF_SUPPORTED | OLECMDF_LATCHED);

        // If it's supported and visible (not LATCHED), toggle it.
        if (rgcmd.cmdf == OLECMDF_SUPPORTED) {
            _pmsot->Exec(NULL, OLECMDID_HIDETOOLBARS, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
        }
    }
}

void CDocObjectHost::_ShowMsoView(void)
{
    HRESULT hres;

    //
    // HACK: Word97 UIDeactivate when we call SetInPlaceSite even with the
    //  same in-place site.
    //
    IOleInPlaceSite* psite;
    hres = _pmsov->GetInPlaceSite(&psite);
    if (SUCCEEDED(hres) && psite) {
        if (psite!=this) {
            _pmsov->SetInPlaceSite(this);
        } else {
            TraceMsg(TF_SHDAPPHACK, "DOH::_ShowMsoView not calling SetInPlaceSite because it's already set");
        }
        psite->Release();

    } else {
        _pmsov->SetInPlaceSite(this);
    }

    GetClientRect(_hwnd, &_rcView);

    ASSERT(_uState != SVUIA_DEACTIVATE);
    if (_uState != SVUIA_INPLACEACTIVATE       // Do this if we're not already SVUIA_INPLACEACTIVE
        || !(_dwAppHack & BROWSERFLAG_MSHTML)  //   or if it's not Trident (office apps expect this call)
        )
    {
        // Trident is sending progress changed messages here -- and causing Compuserve a problem.
        // Flag the fact that we're UIActivating them, and suppress forwarding ProgressChanged
        // messages to our container when this flag is true.  (IE v4.1 bug 54787)
        //
        _fUIActivatingView = TRUE;
        _pmsov->UIActivate(TRUE);
        _fUIActivatingView = FALSE;
    }

    //
    // HACK:
    //
    //  We call _HideOfficeToolbars when our OnUIActivate is called.
    // SriniK suggested us to do it in order to avoid flashing.
    // It works well with Excel (3404), but does not work with Word.
    // Word does not hide its toolbars correctly. To work around that
    // bug, we call _HideofficeToolbars here again.
    //
    _HideOfficeToolbars();

    hres = _pmsov->SetRect(&_rcView);

    if (FAILED(hres)) {
        TraceMsg(DM_ERROR, "DOC::_ShowMsoView _pmsov->SetRect ##FAILED## %x", hres);
    }

    if (FAILED(hres) && _uState == SVUIA_INPLACEACTIVATE) {
        TraceMsg(TF_SHDAPPHACK, "APPHACK# DOH::_ShowMsoView calling UIActivate");
        // HACKHACK: for word.  they refuse to show if they aren't UIActivated.
        // if the setrect fails, and we didn't do a UIActivate, do it now.
        _fDontInplaceActivate = TRUE;
        TraceMsg(TF_SHDAPPHACK, "HACK: CDOH::_ShowMsoView calling UIActive(TRUE) to work around Word bug");
        _pmsov->UIActivate(TRUE);
        _pmsov->SetRect(&_rcView);
    }

    // This is the other case where Trident sends Progress changed messages.
    //
    _fUIActivatingView = TRUE;
    hres = _pmsov->Show(TRUE);
    _fUIActivatingView = FALSE;

    if (FAILED(hres)) {
        TraceMsg(DM_ERROR, "DOH::_ShowMsoView _pmsov->Show ##FAILED## %x", hres);
    }

    _fDrawBackground = FALSE;   /* now that we've shown the object, no need to paint our own background */
}

HRESULT CDocObjectHost::_ActivateMsoView()
{
    _EnableModeless(FALSE);

#ifdef DEBUG
    PERFMSG(TEXT("_ActivateMsoView"), GetCurrentTime() - g_dwPerf);
    g_dwPerf = GetCurrentTime();
#endif


    HRESULT hres = NOERROR;
    if (!_phls) {
        _pole->QueryInterface(IID_IHlinkSource, (LPVOID*)&_phls);
    }

    if (_phls && !_fIsHistoricalObject) {
//
// Special test case for IHlinkFrame marshaling.
//

        hres = _phls->Navigate(0, _pszLocation);

        //
        // If this is one of our internal error pages, we can ignore the
        // failure on the bogus location.  In this case pwszLocation will
        // be the original url that failed preceeded with a '#'.
        //
        LPOLESTR pwszUrl;
        if (FAILED(hres) && SUCCEEDED(_GetCurrentPageW(&pwszUrl, TRUE)))
        {
            // if it begins with res: it may be our erro page
            if (pwszUrl[0] == L'r' && pwszUrl[1] == L'e' && IsErrorUrl(pwszUrl))
            {
                // It's our internal error page, so ignore the error
                hres = S_OK;
            }

            OleFree(pwszUrl);
        }

        if (FAILED(hres)) {
            TraceMsg(DM_ERROR, "DOC::_ActivateMsoView _phls->Navigate(%s) ##FAILED## %x",
                     _pszLocation ? _pszLocation : TEXT(""), hres);
        }
    } else {
        // BUGBUG todo: use _DoVerbHelper? (but careful! ACT_FOCUS different)
        LONG iVerb = OLEIVERB_SHOW;
        MSG msg;
        LPMSG pmsg = NULL;

        if (_uState == SVUIA_INPLACEACTIVATE) {
            iVerb = OLEIVERB_INPLACEACTIVATE;
        }
        if (_pedsHelper)
        {
            if (SUCCEEDED(_pedsHelper->GetDoVerbMSG(&msg)))
            {
                pmsg = &msg;
            }
        }
        hres = _pole->DoVerb(iVerb, pmsg, this, (UINT)-1, _hwnd, &_rcView);
        if (hres == OLEOBJ_E_INVALIDVERB && iVerb == OLEIVERB_INPLACEACTIVATE)
            hres = _pole->DoVerb(OLEIVERB_SHOW, pmsg, this, (UINT)-1, _hwnd, &_rcView);

        if (FAILED(hres)) {
            TraceMsg(DM_ERROR, "DOC::_ActivateMsoView _pole->DoVerb ##FAILED## %x", hres);
        }
    }


    // the doc is activated
    if (SUCCEEDED(hres)) {

        _ReleasePendingObject();

        if (_fHaveParentSite) {
            _HideOfficeToolbars();
        }
    }

    _EnableModeless(TRUE);

    return hres;
}

HRESULT CDocObjectHost::_EnsureActivateMsoView()
{
    HRESULT hres = E_FAIL;

    // if we've got an ole object and
    // either we don't have a view, or we don't have an active view..
    // do the activation
    if (_pole)
    {

        if (!_pmsov || !_ActiveObject()) {

            hres = _ActivateMsoView();

            // Note that we should not UIActivate it here. We should wait
            // until the DocObject calls our ActivateMe
            // _ShowMsoView();
        }
    }

    return hres;
}

//
// This member closes the MsoView window and releases interface
// pointers. This is essentially the reverse of _CreateMsoView.
//
void CDocObjectHost::_CloseMsoView(void)
{
    ATOMICRELEASE(_pmsot);

    if (_pmsov)
    {
        VIEWMSG(TEXT("_CloseMsoView calling pmsov->UIActivate(FALSE)"));
        IOleDocumentView* pmsov = _pmsov;
        _pmsov = NULL;
        _fDontInplaceActivate = FALSE;

#ifdef DONT_UIDEACTIVATE
        if (_uState != SVUIA_DEACTIVATE)
            pmsov->UIActivate(FALSE);
#else // DONT_UIDEACTIVATE
        if (_uState == SVUIA_ACTIVATE_FOCUS)
            pmsov->UIActivate(FALSE);
#endif // DONT_UIDEACTIVATE

        _IPDeactivateMsoView(pmsov);
        pmsov->CloseView(0);
        pmsov->SetInPlaceSite(NULL);
        pmsov->Release();
        VIEWMSG(TEXT("_CloseMsoView called pmsov->Release()"));
    }

    ATOMICRELEASE(_pmsoc);


}

void CDocObjectHost::_OnPaint(HDC hdc)
{
    if (_pmsov && !_ActiveObject())
    {
        HRESULT hres;
        RECT rcClient;
        GetClientRect(_hwnd, &rcClient);
        hres = OleDraw(_pmsov, DVASPECT_CONTENT, hdc, &rcClient);
        TraceMsg(0, "shd TR ::_OnPaint OleDraw returns %x", hres);
    }
}

HRESULT _GetDefaultLocation(LPWSTR pszPath, DWORD cchPathSizeIn, UINT id)
{
    WCHAR szPath[MAX_URL_STRING];
    DWORD cbSize = SIZEOF(szPath);
    DWORD cchPathSize = cchPathSizeIn;
    HRESULT hres = E_FAIL;
    HKEY hkey;

    // BUGBUG: Share this code!!!
    // BUGBUG: This is Internet Explorer Specific

    HKEY hkeyroot = id == IDP_CHANNELGUIDE ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
    if (RegOpenKeyW(hkeyroot,
            L"Software\\Microsoft\\Internet Explorer\\Main",
            &hkey)==ERROR_SUCCESS)
    {
        DWORD dwType;

        LPCWSTR pszName;

        switch(id) {
        default:
            ASSERT(0);
        case DVIDM_GOHOME:
            pszName = L"Default_Page_URL";
            break;

        case DVIDM_GOSEARCH:
            pszName = L"Default_Search_URL";
            break;

        case IDP_UPDATE:
            pszName = L"Default_Update_URL";
            break;

        case IDP_CHANNELGUIDE:
            pszName = L"ChannelsURL";
            break;

        }

        if (RegQueryValueExW(hkey, pszName,
            0, &dwType, (LPBYTE)szPath, &cbSize)==ERROR_SUCCESS)
        {
            // When reading a URL from registry, treat it like it was typed
            // in on the address bar.

            hres = S_OK;

            if(!ParseURLFromOutsideSourceW(szPath, pszPath, &cchPathSize, NULL))
                StrCpyNW(pszPath, szPath, cchPathSizeIn);

            if(IsFileUrlW(pszPath))
            {
                cchPathSize = cchPathSizeIn;
                hres = PathCreateFromUrlW(pszPath, pszPath, &cchPathSize, 0);
            }
        }
        RegCloseKey(hkey);
    }



    HOMEMSG("_GetStdLocation returning",
            (SUCCEEDED(hres) ? pszPath : TEXT("Error")), hres);

    return hres;
}

HRESULT _GetStdLocation(LPTSTR pszPath, DWORD cchPathSize, UINT id)
{
    TCHAR szPathTemp[MAX_URL_STRING];
    DWORD cchTempSize = ARRAYSIZE(szPathTemp);
    HRESULT hres = E_FAIL;
    LPCWSTR pszName = NULL;

    ASSERT(cchPathSize >= cchTempSize);     // If we hit this, we will truncate the URL in some cases.
    ASSERT(pszPath && (cchPathSize > 0)); // Not Optional
    pszPath[0] = TEXT('\0');

    // BUGBUG: Share this code!!!
    // BUGBUG: This is Internet Explorer Specific
    switch(id) {
    default:
        ASSERT(0);
    case DVIDM_GOHOME:
        pszName = L"Start Page";
        break;

    case DVIDM_GOFIRSTHOME:
    case DVIDM_GOFIRSTHOMERO:
        pszName = L"First Home Page";
        break;

    case DVIDM_GOSEARCH:
        pszName = L"Search Page";
        break;

    case DVIDM_SEARCHBAR:
        pszName = L"Search Bar";
        break;

    case DVIDM_GOLOCALPAGE:
        pszName = L"Local Page";
        break;
    }

    hres = URLSubRegQuery(szRegKey_SMIEM, pszName, TRUE,
                       szPathTemp, cchTempSize, URLSUB_ALL);
    if (FAILED(hres) &&
        ((DVIDM_GOFIRSTHOME == id) || (DVIDM_GOFIRSTHOMERO == id)))
    {
        // The First Home key doesn't exist so use the home key.
        pszName = TEXT("Start Page");
        hres = URLSubRegQuery(szRegKey_SMIEM, pszName, TRUE,
                           szPathTemp, cchTempSize, URLSUB_ALL);
        id = DVIDM_GOHOME;
    }

    if (SUCCEEDED(hres))
    {
        // When reading a URL from registry, treat it like it was typed
        // in on the address bar.

        if(ParseURLFromOutsideSourceW(szPathTemp, pszPath, &cchPathSize, NULL))
        {
            if(IsFileUrlW(pszPath))
                hres = PathCreateFromUrlW(pszPath, pszPath, &cchPathSize, 0);
        }
    }

    if (DVIDM_GOFIRSTHOME == id)    // Delete that FIRSTHOME key
    {
        HUSKEY hUSKey;

        if (ERROR_SUCCESS == SHRegOpenUSKey(szRegKey_SMIEM, KEY_WRITE, NULL, &hUSKey, FALSE))
        {
            SHRegDeleteUSValue(hUSKey, TEXT("First Home Page"), SHREGDEL_DEFAULT);
            SHRegCloseUSKey(hUSKey);
        }
        hres = S_OK;
    }

    HOMEMSG("_GetStdLocation returning",
            (SUCCEEDED(hres) ? pszPath : TEXT("Error")), hres);

    return hres;
}


HRESULT WINAPI _GetDefaultLocation(UINT idp, LPWSTR pszPath, UINT cchMax)
{
    switch (idp)
    {
    case IDP_UPDATE:
    case IDP_CHANNELGUIDE:
        URLSubLoadString(NULL, IDS_DEF_UPDATE+(idp-IDP_UPDATE), pszPath, cchMax, URLSUB_ALL);
        break;

    default:
        _GetDefaultLocation(pszPath, cchMax, (idp == IDP_SEARCH) ? DVIDM_GOSEARCH : DVIDM_GOHOME);
        break;
    }

    return S_OK;
}


HRESULT WINAPI SHDGetPageLocation(HWND hwndOwner, UINT idp, LPWSTR pszPath, UINT cchMax, LPITEMIDLIST *ppidlOut)
{
    TCHAR szBuf[MAX_URL_STRING];
    if (pszPath==NULL) {
        pszPath = szBuf;
        cchMax = ARRAYSIZE(szBuf);
    }
    *ppidlOut = NULL;
    HRESULT hres = S_OK;
    switch (idp) {
    case IDP_UPDATE:
    case IDP_CHANNELGUIDE:
        ASSERT(IDP_CHANNELGUIDE-IDP_UPDATE == IDS_DEF_CHANNELGUIDE-IDS_DEF_UPDATE);
        if (FAILED(_GetDefaultLocation(pszPath, cchMax, idp)))
        {
            _GetDefaultLocation(idp, pszPath, cchMax);
        }
        break;

    default:
        ASSERT(idp==IDP_START || idp==IDP_SEARCH);
        hres = _GetStdLocation(pszPath, cchMax,
                    (idp == IDP_SEARCH) ? DVIDM_GOSEARCH : DVIDM_GOHOME);
        if (FAILED(hres))
        {
            _GetDefaultLocation(idp, pszPath, cchMax);
        }
        break;
    }

    if (SUCCEEDED(hres))
    {
        hres = IECreateFromPath(pszPath, ppidlOut);
        if (FAILED(hres))
        {
            // IECreateFromPath() above could have failed if the default location
            // was invalid. (Like file://server_no_exist/
            _GetDefaultLocation(idp, pszPath, cchMax);
            hres = IECreateFromPath(pszPath, ppidlOut);
        }

        HOMEMSG("SHDGetPageLocation SHILCreateFromPage returned", pszPath, hres);
    }

    return hres;
}


void CDocObjectHost::_ChainBSC()
{
    if (!_bsc._pbscChained && _phf) {
        // Get "chaigned" bind status, if any
        IServiceProvider* psp = NULL;
        HRESULT hres = _phf->QueryInterface(IID_IServiceProvider, (LPVOID*)&psp);
        CHAINMSG("_StartAsyncBinding hlf->QI returns", hres);

        if (SUCCEEDED(hres)) {
            ASSERT(NULL==_bsc._pbscChained);
            hres = psp->QueryService(SID_SHlinkFrame, IID_IBindStatusCallback, (LPVOID*)&_bsc._pbscChained);
            CHAINMSG("_StartAsyncBinding psp(hlf)->QS returns", hres);
            psp->Release();
            if (SUCCEEDED(hres))
            {
                ASSERT(NULL==_bsc._pnegotiateChained);
                _bsc._pbscChained->QueryInterface(IID_IHttpNegotiate, (LPVOID *)&_bsc._pnegotiateChained);
            }
        }
    }
}

//
// WARNING: Following two global variables are shared among multiple-threads
//  in a thread. Therefore, all right-access must be serialized and all read
//  access should be blocked when right is going on.
//
//   Right now, we just initialize them once (based on the registry setting)
//  and never update. It allows us to simplify the code quite a bit. If we
//  need to update, then _RegisterMediaTypeClass should be changed significantlly
//  so that we can safely handle multiple access to those hdsa's. (SatoNa)
//
HDSA g_hdsaCls = NULL;
HDSA g_hdsaStr = NULL;

BOOL CDocObjectHost::_BuildClassMapping(void)
{
    if (g_hdsaCls) {
        return (DSA_GetItemCount(g_hdsaCls)==DSA_GetItemCount(g_hdsaStr));
    }

    ENTERCRITICAL;
    if (!g_hdsaCls) {
        g_hdsaStr = DSA_Create(SIZEOF(LPCSTR), 32);
        if (g_hdsaStr)
        {
            HDSA hdsaCls = DSA_Create(SIZEOF(CLSID), 32);
            if (hdsaCls)
            {
                HKEY hkey;
                if (RegOpenKey(HKEY_LOCAL_MACHINE,
                        TEXT("Software\\Microsoft\\Internet Explorer\\MediaTypeClass"),
                        &hkey) == ERROR_SUCCESS)
                {
                    TCHAR szCLSID[64];  // enough for "{CLSID}"
                    for (int iKey=0;
                         RegEnumKey(hkey, iKey, szCLSID, SIZEOF(szCLSID))==ERROR_SUCCESS;
                         iKey++)
                    {
                        CLSID clsid;
                        if (FAILED(CLSIDFromString(szCLSID, &clsid))) {
                            TraceMsg(DM_WARNING, "CDOH::_RMTC CLSIDFromString(%x) failed", szCLSID);
                            continue;
                        }

                        TraceMsg(DM_MIMEMAPPING, "CDOH::_RMTC RegEnumKey found %s", szCLSID);
                        HKEY hkeyCLSID;
                        if (RegOpenKey(hkey, szCLSID, &hkeyCLSID) == ERROR_SUCCESS)
                        {
                            for (int iValue=0; ; iValue++)
                            {
                                CHAR szFormatName[128];
                                DWORD dwType;
                                DWORD cchValueName = ARRAYSIZE(szFormatName);
                                //
                                // Keep the name ansi because it needs to get
                                // passed to urlmon's RegisterMediaTypeClass as
                                // ansi.
                                //
                                if (RegEnumValueA(hkeyCLSID, iValue, szFormatName, &cchValueName, NULL,
                                                 &dwType, NULL, NULL)==ERROR_SUCCESS)
                                {
                                    TraceMsg(DM_MIMEMAPPING, "CDOH::_RMTC RegEnumValue found %s", szFormatName);
                                    LPSTR psz = StrDupA(szFormatName);
                                    if (psz) {
                                        DSA_InsertItem(hdsaCls, 0xffff, &clsid);
                                        if (DSA_InsertItem(g_hdsaStr, 0xffff, &psz)<0) {
                                            LocalFree(psz);
                                            break;
                                        }
                                    }
                                } else {
                                    break;
                                }
                            }
                            RegCloseKey(hkeyCLSID);
                        } else {
                            TraceMsg(DM_WARNING, "CDOH::_RMTC RegOpenKey(%s) failed", szCLSID);
                        }
                    }
                    RegCloseKey(hkey);
                } else {
                    TraceMsg(0, "CDOH::_RMTC RegOpenKey(MediaTypeClass) failed");
                }

                //
                // Update g_hdsaCls at the end so that other thread won't
                // access while we are adding items.
                //
                g_hdsaCls = hdsaCls;
                ASSERT(DSA_GetItemCount(g_hdsaCls)==DSA_GetItemCount(g_hdsaStr));
            }
        }
    }

    LEAVECRITICAL;

    return (g_hdsaCls && DSA_GetItemCount(g_hdsaCls)==DSA_GetItemCount(g_hdsaStr));
}

HRESULT CDocObjectHost::_RegisterMediaTypeClass(IBindCtx* pbc)
{
    HRESULT         hres    = S_FALSE; // Assume no mapping

    if (_BuildClassMapping() && DSA_GetItemCount(g_hdsaCls)) {
        //
        // WARNING: This code assumes that g_hdsaCls/g_hdsaStr never
        //  changes once they are initializes. Read notes above
        //  those global variables for detail.
        //
        hres = RegisterMediaTypeClass(pbc,
                        DSA_GetItemCount(g_hdsaCls),
                        (LPCSTR*)DSA_GetItemPtr(g_hdsaStr, 0),
                        (CLSID*)DSA_GetItemPtr(g_hdsaCls, 0), 0);

        TraceMsg(DM_MIMEMAPPING, "CDOH::_StartAsyncBinding RegisterMTC returns %x", hres);
    }

    // Now see if the container has anything that needs to be registered
    //
    if (_psp)
    {
        IMimeInfo       *pIMimeInfo;
        hres = _psp->QueryService(SID_IMimeInfo, IID_IMimeInfo, (LPVOID*)&pIMimeInfo);

        if (SUCCEEDED(hres))
        {
            UINT            cTypes  = 0;
            LPCSTR          *ppszTypes = NULL;
            CLSID           *pclsIDs= NULL;
            ASSERT(pIMimeInfo);
            hres = pIMimeInfo->GetMimeCLSIDMapping(&cTypes, &ppszTypes, &pclsIDs);

            if(SUCCEEDED(hres)) {
                if(cTypes && ppszTypes && pclsIDs) {
                    // Last one to register wins, so if the container wants to override what is
                    // already registered this should do it.
                    //  URLMon will handle the duplicates corectly.
                    //
                    hres = RegisterMediaTypeClass(pbc, cTypes, ppszTypes, pclsIDs, 0);

                    TraceMsg(DM_MIMEMAPPING, "CDOH::_StartAsyncBinding RegisterMTC for Container returns %x", hres);
                }
                // RegisterMediaTypeClass should have made copies
                // so free the containers allocations as it expects us to do
                //
                //      CoTaskMemFree(NULL) is OK
                //
                CoTaskMemFree(ppszTypes);
                CoTaskMemFree(pclsIDs);
            }
            pIMimeInfo->Release();
        } else {
            hres = S_FALSE;
        }
    }
    return hres;
}

HRESULT _RegisterAcceptHeaders(IBindCtx* pbc, IShellBrowser* psb)
{
    return RegisterDefaultAcceptHeaders(pbc, psb);
}

HRESULT GetAmbientBoolProp(IExpDispSupport* peds, DISPID dispid, BOOL *pb)
{
    VARIANT var = {0};

    // Assume failure
    *pb = FALSE;

    HRESULT hres = peds->OnInvoke(dispid, IID_NULL, NULL, DISPATCH_PROPERTYGET, (DISPPARAMS *)&g_dispparamsNoArgs, &var, NULL, NULL);
    if (SUCCEEDED(hres))
    {
        // VB returns success with VT_EMPTY, so we can't assert here
        if (var.vt == VT_BOOL)
        {
            *pb = (var.boolVal) ? TRUE : FALSE;
        }
        else
        {
            // Even though VB says VT_EMPTY, we don't know what other containers
            // might shove in here. Make sure we clean up.
            //
            VariantClear(&var);
        }
    }
    else
    {
        hres = E_FAIL;
    }

    return hres;
}

HRESULT CDocObjectHost::_GetOfflineSilent(BOOL *pbIsOffline, BOOL *pbIsSilent)
{
    if (_peds)
    {
        if (pbIsOffline)
            GetAmbientBoolProp(_peds, DISPID_AMBIENT_OFFLINEIFNOTCONNECTED, pbIsOffline);

        if (pbIsSilent)
            GetAmbientBoolProp(_peds, DISPID_AMBIENT_SILENT, pbIsSilent);
    }
    else
    {
        if (pbIsOffline)
            *pbIsOffline = FALSE;
        if (pbIsSilent)
            *pbIsSilent = FALSE;
    }

    return S_OK;
}



/*
    Callback function for RatingObtainQuery
*/
void RatingObtainQueryCallback(DWORD dwUserData, HRESULT hr, LPCSTR pszRating, LPVOID lpvInpageRating)
{
    TraceMsg(DM_PICS, "RatingObtainQueryCallback given result %x", hr);

    /* WARNING: This function is called by MSRATING.DLL on a separate thread,
     * not the main message loop thread.  Touch nothing in important data
     * structures not protected by critical sections!
     *
     * Merely format up a windows message with the info we have;  we'll handle
     * this in the main thread, if we ever get there.
     *
     * Note that pszRating is ignored, we count on the ratings engine to have
     * called RatingCheckUserAccess for us and provide the HRESULT.
     */
    if (!::_PostPicsMessage(dwUserData, hr, lpvInpageRating))
        ::RatingFreeDetails(lpvInpageRating);
}


void CDocObjectHost::_StartPicsQuery(void)
{
    if (IS_RATINGS_ENABLED() && ::RatingEnabledQuery() == S_OK) {
        TraceMsg(DM_PICS, "CDOH::_StartPicsQuery entered with ratings enabled");

        BOOL fEnforce = TRUE;
        if (_pszPicsURL != NULL) {
            delete _pszPicsURL;
            _pszPicsURL = NULL;
        }
        LPOLESTR pwszRawURL = NULL;
        if (SUCCEEDED(_GetCurrentPageW(&pwszRawURL, TRUE))) {
            /* We have to call CoInternetGetSecurityUrl to convert pluggable
             * protocols into known schemes, so we know whether we need to
             * enforce ratings on them.
             */
            LPOLESTR pwszSecurityURL = NULL;

            if (SUCCEEDED(CoInternetGetSecurityUrl(pwszRawURL, &pwszSecurityURL,
                                                   PSU_SECURITY_URL_ONLY, 0)))
            {
                // List of protocols for which we never enforce ratings.
                if (!StrCmpNIW(pwszSecurityURL, L"file:", 5) ||
                    !StrCmpNIW(pwszSecurityURL, L"about:", 6) ||
                    !StrCmpNIW(pwszSecurityURL, L"mk:", 3)) {
                    fEnforce = FALSE;
                }
                else {
                    Str_SetPtr(&_pszPicsURL, pwszSecurityURL);
                }

                OleFree(pwszSecurityURL);
            }

            OleFree(pwszRawURL);
        }

        if (fEnforce) {

            TraceMsg(DM_PICS, "CDOH::_StartPicsQuery (%s) turning on wait flags", _pszPicsURL);

            _fbPicsWaitFlags = PICS_WAIT_FOR_ASYNC
                             | PICS_WAIT_FOR_INDOC
                             | PICS_WAIT_FOR_END
                             | PICS_WAIT_FOR_ROOT
                             ;
            _fPicsAccessAllowed = 0;

            HRESULT hr;
            _dwPicsSerialNumber = ::_AddPicsQuery(_hwnd);
            if (_dwPicsSerialNumber == 0)
                hr = E_OUTOFMEMORY;
            else
            {
                //
                // The ratings apis are ansi.
                //

                CHAR szURL[MAX_URL_STRING];

                SHUnicodeToAnsi(_pszPicsURL, szURL, ARRAYSIZE(szURL));
                hr = RatingObtainQuery(szURL, _dwPicsSerialNumber, RatingObtainQueryCallback, &_hPicsQuery);
            }
            if (FAILED(hr)) {
                TraceMsg(DM_PICS, "CDOH::_StartPicsQuery no async query queued");
                ::_RemovePicsQuery(_dwPicsSerialNumber);
                _dwPicsSerialNumber = 0;
                _fbPicsWaitFlags &= ~PICS_WAIT_FOR_ASYNC;
            }
            else {
                TraceMsg(DM_PICS, "CDOH::_StartPicsQuery async query queued");
            }
        }
    }
}


void CDocObjectHost::_GotLabel(HRESULT hres, LPVOID pDetails, BYTE bfSource)
{
    TraceMsg(DM_PICS, "CDOH::_GotLabel hres=%x, source=%x, waitflags=%x", hres, (DWORD)bfSource, (DWORD)_fbPicsWaitFlags);

    /* If we've already gotten a result from this or a more significant source,
     * ignore this one.
     */
    if (!(_fbPicsWaitFlags & bfSource)) {
        TraceMsg(DM_PICS, "CDOH::_GotLabel already got label from that source");

        if (pDetails != NULL)
            ::RatingFreeDetails(pDetails);
    }
    else {
        /* If the result is an error somehow (label doesn't apply, etc.), and
         * we can expect more labels from this source, then we don't do anything
         * except save the rating details if we haven't got any yet.
         */
        if (FAILED(hres) && (PICS_MULTIPLE_FLAGS & bfSource)) {
            TraceMsg(DM_PICS, "CDOH::_GotLabel label error and may be multiple");

            if (_pRatingDetails == NULL) {
                _pRatingDetails = pDetails;
            }
            else {
                ::RatingFreeDetails(pDetails);
            }
        }
        else {
            /* Either we got a definitive answer from this rating source, or
             * this is the only answer we'll get from it.  We clear at least
             * the flag for this source so we know we've heard from it.  If
             * the response was not an error, then clear flags for all less
             * significant sources as well, so that we'll ignore them.  On
             * the other hand, if this source returned an error, it didn't
             * give us anything useful, so we keep looking at other sources.
             */
            if (SUCCEEDED(hres))
                _fbPicsWaitFlags &= bfSource - 1;
            else
                _fbPicsWaitFlags &= ~bfSource;

            TraceMsg(DM_PICS, "CDOH::_GotLabel, waitflags now %x", (DWORD)_fbPicsWaitFlags);

            if (hres == S_OK) {
                TraceMsg(DM_PICS, "CDOH::_GotLabel allowing access");
                ::RatingFreeDetails(pDetails);  /* don't need this if access allowed */
                _fPicsAccessAllowed = 1;
            }
            else {
                /* Access denied or error.  Meaningful details from this result
                 * can override details from an earlier, less significant
                 * result.  Only explicitly deny access if not an error,
                 * though (this handles the valid root label followed by
                 * invalid in-document label, for example).
                 */
                if (pDetails != NULL) {
                    if (_pRatingDetails != NULL)
                        ::RatingFreeDetails(_pRatingDetails);
                    _pRatingDetails = pDetails;
                }
                if (SUCCEEDED(hres))
                    _fPicsAccessAllowed = 0;
            }
        }
    }

    if (_fPicsBlockLate && !_fbPicsWaitFlags)
        _HandlePicsChecksComplete();
}


void CDocObjectHost::_HandleInDocumentLabel(LPCTSTR pszLabel)
{
    BYTE bFlag = (_pRootDownload != NULL) ? PICS_WAIT_FOR_ROOT : PICS_WAIT_FOR_INDOC;

    TraceMsg(DM_PICS, "CDOH::_HandleInDocumentLabel source %x gave label %s", (DWORD)bFlag, pszLabel);

    if (!(_fbPicsWaitFlags & bFlag)) {
        TraceMsg(DM_PICS, "CDOH::_HandleInDocumentLabel rejecting based on waitflags %x", (DWORD)_fbPicsWaitFlags);
        return;
    }

    LPVOID pDetails = NULL;
    //
    // Ratings has only ansi apis!
    //
    CHAR szURL[MAX_URL_STRING];
    SHUnicodeToAnsi(_pszPicsURL, szURL, ARRAYSIZE(szURL));

    UINT cbMultiByte = WideCharToMultiByte(CP_ACP, 0, pszLabel,
                                           -1, NULL, 0, NULL, NULL);
    if (cbMultiByte > 0) {
        char *pszLabelAnsi = new char[cbMultiByte+1];
        if (pszLabelAnsi != NULL)
        {
            if (WideCharToMultiByte(CP_ACP, 0, pszLabel, -1, pszLabelAnsi,
                                    cbMultiByte+1, NULL, NULL))
            {
                HRESULT hres = ::RatingCheckUserAccess(NULL, szURL,
                                                       pszLabelAnsi, NULL, _dwPicsLabelSource,
                                                       &pDetails);
                _GotLabel(hres, pDetails, bFlag);
            }

            delete [] pszLabelAnsi;
        }
    }
}


void CDocObjectHost::_HandleDocumentEnd(void)
{
    BYTE bFlag = (_pRootDownload != NULL) ? PICS_WAIT_FOR_ROOT : PICS_WAIT_FOR_END;

    TraceMsg(DM_PICS, "CDOH::_HandleDocumentEnd -- no more PICS labels from source %x", (DWORD)bFlag);

    if (_pRootDownload != NULL) {
        ::PostMessage(_hwnd, WM_PICS_ROOTDOWNLOADCOMPLETE, 0, 0);
    }
    else {
        /* End of document;  revoke the IOleCommandTarget we gave to the document,
         * so it won't send us any more notifications.
         */
        VARIANTARG v;
        v.vt = VT_UNKNOWN;
        v.lVal = 0;

        IUnknown_Exec(_pole, &CGID_ShellDocView, SHDVID_CANSUPPORTPICS, 0, &v, NULL);
    }

    if (!(_fbPicsWaitFlags & bFlag)) {
        TraceMsg(DM_PICS, "CDOH::_HandleDocumentEnd skipping due to waitflags %x", (DWORD)_fbPicsWaitFlags);
        return;
    }

    _fbPicsWaitFlags &= ~PICS_WAIT_FOR_INDOC;   /* we know we won't get any more indoc labels */

    LPVOID pDetails = NULL;

    //
    // Ratings has only ansi apis!
    //
    CHAR szURL[MAX_URL_STRING];
    SHUnicodeToAnsi(_pszPicsURL, szURL, ARRAYSIZE(szURL));

    HRESULT hres = ::RatingCheckUserAccess(NULL, szURL, NULL, NULL, _dwPicsLabelSource, &pDetails);

    _GotLabel(hres, pDetails, bFlag);

    if (_pRootDownload == NULL) {
        if (_fbPicsWaitFlags)
            _StartPicsRootQuery(_pszPicsURL);
    }
}

/* This function parses the URL being downloaded and, if the URL doesn't
 * already refer to the root document of the site, sets up a subordinate
 * CDocObjectHost to download that root document, so we can get ratings
 * out of it.
 */
void CDocObjectHost::_StartPicsRootQuery(LPCTSTR pszURL)
{
    if (_fbPicsWaitFlags & PICS_WAIT_FOR_ROOT) {
        BOOL fQueued = FALSE;

        TraceMsg(DM_PICS, "CDOH::_StartPicsRootQuery parsing %s", pszURL);

        WCHAR wszRootURL[MAX_URL_STRING+1];
        DWORD cchResult;

        /* The pszURL we're passed is actually the result of calling
         * CoInternetGetSecurityUrl, and so may not be the scheme that
         * the caller is browsing to.  To support pluggable protocols
         * determining the root location themselves, we first use the
         * URL reported by _GetCurrentPage, which may refer to a
         * pluggable protocol; if that fails, we use the more standard
         * URL.
         */
        HRESULT hres = INET_E_DEFAULT_ACTION;

        LPOLESTR pwszURL = NULL;
        if (SUCCEEDED(_GetCurrentPageW(&pwszURL, TRUE))) {
            hres = CoInternetParseUrl(pwszURL, PARSE_ROOTDOCUMENT, 0, wszRootURL,
                                      ARRAYSIZE(wszRootURL), &cchResult, 0);
            OleFree(pwszURL);
        }

        if (pszURL != NULL && (hres == INET_E_DEFAULT_ACTION || hres == E_FAIL)) {
            /* Pluggable protocol doesn't support PARSE_ROOTDOCUMENT.  Use the
             * more standard URL we were supplied with.
             */
            hres = CoInternetParseUrl(pszURL, PARSE_ROOTDOCUMENT, 0, wszRootURL,
                                      ARRAYSIZE(wszRootURL), &cchResult, 0);
        }

        if (SUCCEEDED(hres)) {
            IMoniker *pmk = NULL;
            hres = MonikerFromURL(wszRootURL, &pmk);

            if (SUCCEEDED(hres)) {
                BOOL fFrameIsSilent = FALSE;
                BOOL fFrameIsOffline = FALSE;

                this->_GetOfflineSilent(&fFrameIsOffline, &fFrameIsSilent);

                _pRootDownload = new CPicsRootDownload(this, &_ctPics, fFrameIsOffline, fFrameIsSilent);
                if (_pRootDownload != NULL) {
                    TraceMsg(DM_PICS, "CDOH::_StartPicsRootQuery starting download");
                    hres = _pRootDownload->StartDownload(pmk);
                    if (SUCCEEDED(hres))
                        fQueued = TRUE;
                }
            }
            if (pmk != NULL)
                pmk->Release();
        }
        if (!fQueued) {
            _fbPicsWaitFlags &= ~PICS_WAIT_FOR_ROOT;
            TraceMsg(DM_PICS, "CDOH::_StartPicsRootQuery queueing failed, waitflags now %x", (DWORD)_fbPicsWaitFlags);
            if (!_fbPicsWaitFlags) {
                _HandlePicsChecksComplete();
            }
        }
    }
    else {
        TraceMsg(DM_PICS, "CDOH::_StartPicsRootQuery no query queued, waitflags=%x", (DWORD)_fbPicsWaitFlags);
    }
}




HRESULT CDocObjectHost::_StartAsyncBinding(IMoniker* pmk, IBindCtx* pbc, IShellView* psvPrev)
{
    URLMSG(TEXT("_StartAsyncBinding called"));
    HRESULT hres;

    ASSERT(_bsc._pbc==NULL && _pole==NULL);
    _bsc._RegisterObjectParam(pbc);

    //
    //  Associate the client site as an object parameter to this
    // bind context so that Trident can pick it up while processing
    // IPersistMoniker::Load().
    //
    pbc->RegisterObjectParam(WSZGUID_OPID_DocObjClientSite,
        SAFECAST(this, IOleClientSite*));

    _ChainBSC();

    IUnknown* punk = NULL;
    _bsc._pbc = pbc;
    pbc->AddRef();


    // Decide right here whether or not this frame is offline
    BOOL bFrameIsOffline = FALSE;
    BOOL bFrameIsSilent = FALSE;


    this->_GetOfflineSilent(&bFrameIsOffline, &bFrameIsSilent);

    _bsc._bFrameIsOffline = bFrameIsOffline ? TRUE : FALSE;
    _bsc._bFrameIsSilent  = bFrameIsSilent ? TRUE : FALSE;
    BOOL bSuppressUI = (_bsc._bFrameIsSilent || _IsDesktopItem(SAFECAST(this, CDocObjectHost*))) ? TRUE : FALSE;

#ifdef DEBUG
    PERFMSG(TEXT("_StartAsyncBinding Calling pmk->BindToObject"), GetCurrentTime()-g_dwPerf);
    g_dwPerf = GetCurrentTime();
#endif

#ifdef DEBUG
    if (g_dwPrototype & 0x00000800) {
        TraceMsg(DM_TRACE, "CDOH::_StartAsyncBinding skipping CLSID mapping");
    }
    else
#endif
    {
        // Register overriding mime->CLSID mapping
        _RegisterMediaTypeClass(pbc);
    }

    // Register accept headers
    _RegisterAcceptHeaders(pbc, _psb);

    if (_pwb)
        _pwb->SetNavigateState(BNS_BEGIN_NAVIGATE);

    _StartPicsQuery();
    //
    //  BUGBUG - Crazy sync/async behavior of URLMON.  - zekel - 6-AUG-97
    //  any of the following may occur:
    //
    //  1.  SUCCESS or FAILURE:  we receive sync E_PENDING from BindToObject,
    //      and then get an Async HRESULT on OnStopBinding().
    //      this is the most common case and the basic design.
    //
    //  2.  SUCCESS:  we receive sync S_OK from BindToObject and
    //      need to complete the async behavior on our BSCB ourself
    //      since urlmon started but did not finish.
    //
    //  3.  SUCCESS:  while inside BindToObject(), we receive sync S_OK
    //      from OnStopBinding(), and then BindToObject returns with S_OK.
    //
    //  4.  FAILURE:  simplest case is an error being returned from BindToObject()
    //      but without an any OnStopBinding() so we need to complete
    //      the async behavior on our BSCB ourself since urlmon started but did not finish.
    //      this usually occurs when accessing local files.
    //
    //  5.  FAILURE:  while inside BindToObject(), we receive sync S_OK from OnStopBinding(),
    //      and then BindToObject returns with some other error that needs to be handled.
    //      this occurs with some malformed urls.
    //
    //  6.  FAILURE:  while inside BindToObject(), we receive a sync error from OnStopBinding(),
    //      and then BindToObject returns with some other error (usually E_FAIL).
    //      we need to trust the first one.  this occurs when wininet
    //      returns syncronous errors, and its error is the one returned in OnStopBinding()
    //
    //  7.  FAILURE:  while inside BindToObject(), we receive a sync error from OnStopBinding(),
    //      and then BindToObject returns with E_PENDING.  which we think means everything
    //      is going great, and urlmon thinks it is done.  this happens with a file: to
    //      a resource that is not hostable.  we need to show the download UI.
    //
    //  in order to support all the errors in the most consistent and safe manner,
    //  we defer any errors in OnStopBinding() if they are delivered synchronously
    //  on BindToObject().  the OnStopBinding() error always overrides the BindToObject()
    //  error, but any error will always override any success.
    //


    ASSERT(S_OK == _hrOnStopBinding);

    _fSyncBindToObject = TRUE;

    URLMSG(TEXT("_StartAsyncBinding calling pmk->BindToObject"));
    hres = pmk->BindToObject(pbc, NULL, IID_IUnknown, (LPVOID*)&punk);
    URLMSG3(TEXT("_StartAsyncBinding pmk->BindToObject returned"), hres, punk);

    _fSyncBindToObject = FALSE;

    if (SUCCEEDED(_hrOnStopBinding) && (SUCCEEDED(hres) || hres==E_PENDING))
    {
        hres = S_OK;

        if (_bsc._pbc) {
            //
            // In case OnStopBinding hasn't been called.
            //
            if (!_pole) {
                if (psvPrev) {
                    _bsc._psvPrev = psvPrev;
                    psvPrev->AddRef();
                }
            } else {
                URLMSG3(TEXT("_StartAsyncBinding we've already got _pole"), hres, _pole);
            }

            //
            // If moniker happen to return the object synchronously, emulate
            // OnDataAvailable callback and OnStopBinding.
            //
            if (punk)
            {
                _bsc.OnObjectAvailable(IID_IUnknown, punk);
                _bsc.OnStopBinding(hres, NULL);
                punk->Release();
                ASSERT(_bsc._pbc==NULL);

            }
        } else {
            //
            // OnStopBinding has been already called.
            //
            if (punk)
            {
                AssertMsg(0, TEXT("CDOH::_StartAsyncBinding pmk->BindToObject returned punk after calling OnStopBinding")); // Probably URLMON bug.
                punk->Release();
            }
        }
    }
    else
    {
        // Binding failed.
        TraceMsg(DM_WARNING, "CDOH::_StartAsyncBinding failed (%x)", hres);

        //
        //  BUGBUG - Urlmon is inconsistent in it's error handling - zekel - 4-AUG-97
        //  urlmon can return errors in three different ways from BindToObject()
        //  1.  it can return back a simple syncronous error.  without calling OnStopBinding()
        //
        //  2.  it can return a sync error,
        //          but call OnStopBinding() with S_OK first on the same thread;
        //
        //  3.  it can return a sync error,
        //          but also call OnStopBinding() with the real Error first on the same thread.
        //
        //  4.  it can return E_PENDING,
        //          but already have called OnStopBinding() with the real error.
        //
        //  SOLUTIONS:
        //  in all cases of error in OnStopBinding(), we will now postpone the OnStopBinding processing util after
        //  we have returned from the BindToObject().  we try to use the best error.
        //  we allow successful OnStopBinding() to pass through unmolested, and trap
        //  the error here if necessary.
        //

        if (FAILED(_hrOnStopBinding))
            hres = _hrOnStopBinding;

        if (_bsc._pbc)
            _bsc.OnStopBinding(hres, NULL);
        else if (!bSuppressUI)
        {
            //
            //  OnStopBinding was already called, but with a success
            //  so we need to handle the error here.  this happens
            //  with some invalid URLs like http:/server
            //

            // Fix for W98 webtv app.  If we're in a frame don't
            // blow away the frame set to dispaly the error.
            //
            if (!_fHaveParentSite)
                _bsc._NavigateToErrorPage(ERRORPAGE_SYNTAX, this, FALSE);
        }

        ASSERT(_bsc._pbc==NULL);
    }

    return hres;
}

void CDocObjectHost::_ReleasePendingObject(BOOL fIfInited)
{
    HRESULT hres;
    IOleObject *polePending;
#ifdef TRIDENT_NEEDS_LOCKRUNNING
    IRunnableObject *pro;
#endif

    if (fIfInited == FALSE && _fPendingWasInited == FALSE)
        return;

    if (_punkPending)
    {
        if (_fCreatingPending)
        {
            _fAbortCreatePending = 1;
            return;
        }

        if (!_fPendingNeedsInit && !IsSameObject(_punkPending, _pole))
        {
            hres = _punkPending->QueryInterface(IID_IOleObject, (LPVOID *) &polePending);
            if (SUCCEEDED(hres)) {
                LPOLECLIENTSITE pcs;
                if (SUCCEEDED(polePending->GetClientSite(&pcs)) && pcs)
                {
                    if (pcs == SAFECAST(this, LPOLECLIENTSITE))
                    {
                        polePending->SetClientSite(NULL);
                    }
                    pcs->Release();
                }
                polePending->Release();
            }
        }
#ifdef TRIDENT_NEEDS_LOCKRUNNING
        //  TRIDENT NO LONGER SUPPORTS IRunnableObject
        hres = _punkPending->QueryInterface(IID_IRunnableObject, (LPVOID *) &pro);
        if (SUCCEEDED(hres))
        {
            hres = pro->LockRunning(FALSE, TRUE);
            pro->Release();
        }
#endif
        SAFERELEASE(_punkPending);
        _fPendingWasInited = FALSE;
    }
}

void CDocObjectHost::_ReleaseOleObject(BOOL fIfInited)
{
    TraceMsg(DM_DEBUGTFRAME, "CDocObjectHost::_ReleaseOleObject called %x (%x)", _pole, this);

    // Minimize impact by cleaning up in affected cases only.
    if (fIfInited == FALSE && _fPendingWasInited == FALSE)
        return;

    // release _pole object and all the associated QI'ed pointers
    if (_phls) {
        _phls->SetBrowseContext(NULL); // probably no need
        ATOMICRELEASE(_phls);
    }

    if (_pvo) {
        IAdviseSink *pSink;
        // paranoia: only blow away the advise sink if it is still us
        if (SUCCEEDED(_pvo->GetAdvise(NULL, NULL, &pSink)) && pSink) {
            if (pSink == (IAdviseSink *)this) {
                _pvo->SetAdvise(0, 0, NULL);
            } else {
                ASSERT(0);  // do we really hit this case?
            }

            pSink->Release();
        }
        ATOMICRELEASE(_pvo);
    }

    if (_pole) {
        LPOLECLIENTSITE pcs;
        if (SUCCEEDED(_pole->GetClientSite(&pcs)) && pcs) {
            if (IsSameObject(pcs, SAFECAST(this, LPOLECLIENTSITE))) {
                _pole->SetClientSite(NULL);
            }
            pcs->Release();
        }

        // Notes: Make it sure that we don't hold a bogus _pole even
        //  for a moment (while we call Release).
        ATOMICRELEASE(_pole);
    }
}


//
// This member releases all the interfaces to the DocObject, which is
// essentially the reverse of _Bind.
//
void CDocObjectHost::_UnBind(void)
{
    ATOMICRELEASE(_pmsot);

    ASSERT(!_pmsov); // paranoia
    ATOMICRELEASE(_pmsov);

    ASSERT(!_pmsoc); // paranoia
    ATOMICRELEASE(_pmsoc);

    _xao.SetActiveObject(NULL);

    if (_pole) {

        // Just in case we're destroyed while we were waiting
        // for the docobj to display itself.
        //
        _RemoveTransitionCapability();

        //
        //  If this is NOT MSHTML, cache the OLE server so that we don't
        // need to restart or load the OLE server again.
        //
        if (!(_dwAppHack & (BROWSERFLAG_MSHTML | BROWSERFLAG_DONTCACHESERVER)))
        {
            IBrowserService *pbs;
            if(SUCCEEDED(QueryService(SID_STopLevelBrowser, IID_IBrowserService, (LPVOID *)&pbs)))
            {
                pbs->CacheOLEServer(_pole);
                pbs->Release();
            }
        }

        TraceMsg(DM_ADVISE, "CDocObjectHost::_UnBind about to call Close of %x", _pole);
        _pole->Close(OLECLOSE_NOSAVE);

        _ReleaseOleObject();
    }

    _ReleasePendingObject();


    ATOMICRELEASE(_pstg);
    ATOMICRELEASE(_pbcCur);
    ATOMICRELEASE(_pmkCur);
}

//
// HACK: If we open Excel95 objects directly, Excel goes crazy and eventually
//  hit GPF. Here is the background info, I've got Office guys (SatoNa).
//
// From:        Rajeev Misra (Xenix)
//
//   1) Excel does not handle the foll. case very well. Taking a normal file
//   loading it through IPersistFile:Load and then bringing it up as an
//   embedded object. The code was always tested so that the embedded
//   objects always got loaded through ScPrsLoad. I am seeing a bunch of
//   asserts in Excel that say that this assumption is being destroyed.
//   ASSERT(_pole);
//
// From:        Srini Koppolu
//
//   For you, there is only one case, i.e. you always deal with the files. Then your code should look like this
//
//     CreateFileMoniker from the file
//     pUIActiveObject->OnFrameWindowActivate(FALSE);
//     pmk->BindToObject(IID_IDataObject, &pdobj)
//     pUIActiveObject->OnFrameWindowActivate(TRUE);
//     OleCreateFromData()
//
//   OnFrameWindowActivate is done to take care of another excel problem.
//   If you currently have and Excel object UIActive in you and you try to
//   do IPersistFile::Load on Excel, then it will cause problems.
//

void CDocObjectHost::_AppHackForExcel95(void)
{
    ASSERT(_pole);

    HRESULT hres;
    IDataObject* pdt = NULL;
    hres = _pole->QueryInterface(IID_IDataObject, (LPVOID*)&pdt);
    TraceMsg(DM_BINDAPPHACK, "_PostBindAppHack -- QI(IOleDataObject) returned %x", hres);

    if (SUCCEEDED(hres))
    {
        ASSERT(_pstg==NULL);
        hres = StgCreateDocfile(NULL,
                STGM_DIRECT | STGM_CREATE | STGM_READWRITE
                | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE,
                0, &_pstg);
        TraceMsg(DM_BINDAPPHACK, "_PostBindAppHack StgCreateDocFile(NULL) returned %x", hres);
        if (SUCCEEDED(hres))
        {
            IOleObject* poleCopy = NULL;
            hres = OleCreateFromData(pdt, IID_IOleObject, OLERENDER_NONE,
                                     NULL, this, _pstg, (LPVOID*)&poleCopy);
            TraceMsg(DM_BINDAPPHACK, "_PostBindAppHack OleCreateFromData(IOleObject) returned %x", hres);

            if (SUCCEEDED(hres)) {
                _fCantSaveBack = TRUE;
                ATOMICRELEASE(_pole);
                _pole = poleCopy;
            }
        }

        pdt->Release();
    }
}

//
//  This function get the UserClassID from the object and opens the regkey
// for that CLSID and returns. If pdwAppHack is non-NULL AND CLSID is
// CLSID_HTMLDocument, we skip all and returns the default apphack flag.
// This is a perf optimization, but prevents us from setting browser
// flags for Trident, which is fine. (SatoNa)
//
HKEY _GetUserCLSIDKey(IOleObject* pole, const CLSID* pclsid, DWORD* pdwAppHack)
{
    HKEY hkey = NULL;   // assume error
    HRESULT hres;
    CLSID clsid = CLSID_NULL;
    if (pole) {
        hres = pole->GetUserClassID(&clsid);
        //  GetUserClassID is optional, can return E_FAIL, then is defined to be
        //  the same as that returned by IPersist::GetClassID. cf, msdev documentation
        //  for GetUserClassID
        if (FAILED(hres))
        {
            hres = IUnknown_GetClassID(pole, &clsid);
        }
    } else if (pclsid) {
        clsid = *pclsid;
        hres = S_OK;
    }
    else
    {
        return NULL;
    }

    //
    // Notice that we check for two CLSIDs to see if this is MSHTML.
    //
    if (pdwAppHack)
    {
        static const IID IID_IVBOleObj =
            {0xb88c9640, 0x14e0, 0x11d0, { 0xb3, 0x49, 0x0, 0xa0, 0xc9, 0xa, 0xea, 0x82 } };
        LPUNKNOWN    pVBOleObj;

        if(     IsEqualGUID(clsid, CLSID_HTMLDocument)
             || IsEqualGUID(clsid, CLSID_MHTMLDocument)
             || IsEqualGUID(clsid, CLSID_HTMLPluginDocument) )
        {
            TraceMsg(TF_SHDAPPHACK, "_GetUserCLSID this is Trident. Skip opening reg key");
            *pdwAppHack = BROWSERFLAG_NEVERERASEBKGND | BROWSERFLAG_SUPPORTTOP
                            | BROWSERFLAG_MSHTML;
            return NULL;
        }
        else if (pole && SUCCEEDED(pole->QueryInterface(IID_IVBOleObj, (void**)&pVBOleObj) ))
        {
            // If the object answers to IID_IVBOleObj, it's a VB doc object and shouldn't be cached.
            //
            pVBOleObj->Release();
            *pdwAppHack = BROWSERFLAG_DONTCACHESERVER;
        }

    }

    //
    // HACK: MSHTML.DLL does not implement GetUserClassID, but
    //  returns S_OK. That's why we need to check for CLSID_NULL.
    //
    if (SUCCEEDED(hres) && !IsEqualGUID(clsid, CLSID_NULL)) {
        TCHAR szBuf[50];        // 50 is enough for GUID
        SHStringFromGUID(clsid, szBuf, ARRAYSIZE(szBuf));

        TraceMsg(DM_BINDAPPHACK, "_PostBindAppHack GetUserClassID = %s", szBuf);
        TCHAR szKey[60];    // 60 is enough for CLSID\\{CLSID_XX}
        wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("CLSID\\%s"), szBuf);

        if (RegOpenKey(HKEY_CLASSES_ROOT, szKey, &hkey)!=ERROR_SUCCESS)
        {
            TraceMsg(DM_WARNING, "_GetUserCLSIDKey RegOpenKey(%s) failed", szKey);
            // I don't trust RegOpenKey.
            hkey = NULL;
        }
    }
    return hkey;
}


BOOL _GetAppHackKey(LPCTSTR pszProgID, DWORD* pdwData)
{
    BOOL fSuccess = FALSE;
    HKEY hkey;
    if (RegOpenKey(HKEY_CLASSES_ROOT, pszProgID, &hkey)==ERROR_SUCCESS)
    {
        DWORD dwType;
        DWORD cbSize = SIZEOF(*pdwData);
        if (RegQueryValueEx(hkey, TEXT("BrowserFlags"), NULL,
            &dwType, (LPBYTE)pdwData, &cbSize)==ERROR_SUCCESS
            && (dwType==REG_DWORD || (dwType==REG_BINARY && cbSize==SIZEOF(*pdwData))))
        {
            fSuccess = TRUE;
        }
        else
        {
            //
            // Unlike IE3, we make it absolutely sure that the type of object
            // has either "DocObject" key or "BrowseInPlace" key under the
            // ProgID. We can't rely on QI(IID_IOleDocument) because MFC 4.2
            // has a bug and returns S_OK to it. As far as I know, MS-Paint
            // and OmniPage pro are affected by this. We could individually
            // address each of them, but it's probably impossible to catch
            // all. This change has a small risk of breaking existing DocObject
            // server which does not have neither key. If we find such a
            // server, we'll address those individually (which is much easier
            // than covering all MFC apps). (SatoNa)
            //
            TCHAR ach[MAX_PATH];
            LONG cb;
            BOOL fBrowsable = FALSE;
            if ((cb=SIZEOF(ach)) && RegQueryValue(hkey, TEXT("DocObject"), ach, &cb) == ERROR_SUCCESS)
                fBrowsable = TRUE;
            else if ((cb=SIZEOF(ach)) && RegQueryValue(hkey, TEXT("BrowseInPlace"), ach, &cb) == ERROR_SUCCESS)
                fBrowsable = TRUE;

            if (!fBrowsable) {
                TraceMsg(DM_WARNING, "_GetAppHackKey this is neither DocObject or BrowseInPlace");
                *pdwData = BROWSERFLAG_DONTINPLACE;
            }
        }
        RegCloseKey(hkey);
    }
    return fSuccess;
}

void GetAppHackFlags(IOleObject* pole, const CLSID* pclsid, DWORD* pdwAppHack)
{
    HKEY hkey = _GetUserCLSIDKey(pole, pclsid, pdwAppHack);
    if (hkey)
    {
        TCHAR szValue[MAX_PATH];
        LONG cb = SIZEOF(szValue);
        if (RegQueryValue(hkey, TEXT("ProgID"), szValue, &cb) == ERROR_SUCCESS)
        {
            //
            // First, check if we have an BrowserFlags flag in the registry.
            // If there is, use it. Otherwise, try hard-coded progIDs as
            // we did in IE 3.0
            //
            _GetAppHackKey(szValue, pdwAppHack);
            if (!(*pdwAppHack & BROWSERFLAG_REPLACE)) {
                typedef struct _APPHACK {
                    LPCTSTR pszProgID;
                    DWORD   dwAppHack;
                } APPHACK;

                //
                // We no longer need to disable in-place activation of
                // MS-PAINT because we look for "BrowseInPlace" or
                // "DocObject" key
                //
                // { "Paint.Picture", BROWSERFLAG_DONTINPLACE },
                //
                const static APPHACK s_aah[] = {
                    { TEXT("Excel.Sheet.5"), BROWSERFLAG_OPENCOPY },
                    { TEXT("Excel.Chart.5"), BROWSERFLAG_OPENCOPY },
                    { TEXT("SoundRec"), BROWSERFLAG_OPENVERB },
                    { TEXT("Word.Document.6"), BROWSERFLAG_SETHOSTNAME },
                    { TEXT("Word.Document.8"), BROWSERFLAG_DONTUIDEACTIVATE | BROWSERFLAG_SETHOSTNAME },
                    { TEXT("PowerPoint.Show.8"), BROWSERFLAG_DONTUIDEACTIVATE | BROWSERFLAG_PRINTPROMPTUI },
                    { TEXT("Excel.Sheet.8"), BROWSERFLAG_DONTDEACTIVATEMSOVIEW | BROWSERFLAG_INITNEWTOKEEP },
                    { TEXT("Excel.Chart.8"), BROWSERFLAG_DONTDEACTIVATEMSOVIEW | BROWSERFLAG_INITNEWTOKEEP },
                    { TEXT("ABCFlowCharter6.Document"), BROWSERFLAG_DONTINPLACE },
                    { TEXT("ABCFlowCharter7.Document"), BROWSERFLAG_DONTINPLACE },
                    { TEXT("FlowCharter7.Document"), BROWSERFLAG_DONTINPLACE },
                    { TEXT("ChannelFile"), BROWSERFLAG_DONTAUTOCLOSE },
                    { TEXT("Visio.Drawing.5"), BROWSERFLAG_ENABLETOOLSBTN | BROWSERFLAG_SAVEASWHENCLOSING },
                    { TEXT("Visio.Drawing.4"), BROWSERFLAG_ENABLETOOLSBTN | BROWSERFLAG_SAVEASWHENCLOSING }
                };
                const static TCHAR s_ActiveMoveCtx[] = TEXT("AMOVIE.ActiveMovieControl");

                if (!StrCmpN(szValue, s_ActiveMoveCtx, ARRAYSIZE(s_ActiveMoveCtx)-1))
                {
                    *pdwAppHack = BROWSERFLAG_DONTAUTOCLOSE;
                }
                else
                {
                    for (int i=0; i<ARRAYSIZE(s_aah); i++) {
                        if (StrCmp(szValue, s_aah[i].pszProgID)==0)
                        {
                            *pdwAppHack |= s_aah[i].dwAppHack;
                            break;
                        }
                    }
                }
            }

            TraceMsg(DM_BINDAPPHACK, "_GetAppHack ProgID=%s, *pdwAppHack=%x",
                     szValue, *pdwAppHack);

        } else {
            TraceMsg(DM_BINDAPPHACK, "_GetAppHack RegQueryValue(ProgID) failed");
        }

        RegCloseKey(hkey);
    }
}

DWORD CDocObjectHost::_GetAppHack(void)
{
    ASSERT(_pole);
    if (!_fHaveAppHack && _pole)
    {
        _dwAppHack = 0;     // Assume no hack
        _fHaveAppHack = TRUE;
        ::GetAppHackFlags(_pole, NULL, &_dwAppHack);
    }
    return _pole ? _dwAppHack : 0;
}

void CDocObjectHost::_PostBindAppHack(void)
{
    _GetAppHack();

    if (_fAppHackForExcel()) {
        _AppHackForExcel95();
    }
}


//
// This member binds to the object specified by a moniker.
//
HRESULT CDocObjectHost::_BindSync(IMoniker* pmk, IBindCtx* pbc, IShellView* psvPrev)
{
    ASSERT(pbc || !_pole);

    HRESULT hres = S_OK;
    ASSERT(_pole==NULL);

    // Check if we are in the middle of asynchronous binding
    if (_bsc._fBinding) {
        // Yes, wait until it's done or cancled/stopped
        URLMSG(TEXT("_Bind called in the middle of async-binding. Wait in a message loop"));
        while(_bsc._fBinding) {
           MSG msg;
           if (GetMessage(&msg, NULL, 0, 0)) {
               TranslateMessage(&msg);
               DispatchMessage(&msg);
           }
        }

        if (!_pole) {
            hres = E_FAIL;      // BUGBUG: Get the error code from OnStopBinding
        }
    } else {
        // No, bind synchronously
        URLMSG(TEXT("_Bind. Performing syncronous binding"));
        hres = pmk->BindToObject(pbc, NULL, IID_IOleObject, (LPVOID*)&_pole);
    }

    TraceMsg(0, "sdv TR : _Bind -- pmk->BindToObject(IOleObject) returned %x", hres);

    _OnBound(hres);

    return hres;
}

void CDocObjectHost::_OnBound(HRESULT hres)
{
    if (SUCCEEDED(hres)) {
        _PostBindAppHack();
        _InitOleObject();
    }
}

//
//  This function returns TRUE if the specified file's open command is
// associated with "explorer.exe" or "iexplore.exe".
//
// NOTES: It does not check if the "open" command is actually the default
//  or not, but that's sufficient in 99.99 cases.
//
BOOL IsAssociatedWithIE(LPCWSTR szPath)
{
    LPCTSTR pszExtension = PathFindExtension(szPath);

    BOOL bRet = FALSE;
    CHAR szBuf[MAX_PATH];
    CHAR szExt[_MAX_EXT];

    //
    // SHVerbExistsN is ansi only.
    //
    SHUnicodeToAnsi(pszExtension, szExt, ARRAYSIZE(szExt));

    if (SHVerbExistsNA(szExt, "open", szBuf, SIZECHARS (szBuf))) {
        TraceMsg(TF_SHDBINDING, "IsAssociatedWithIE(%s) found %s as open command", szPath, szBuf);
        LPCSTR pszFound;
        if ( (pszFound=StrStrIA(szBuf, IEXPLORE_EXE))
             || (pszFound=StrStrIA(szBuf, EXPLORER_EXE)) ) {
            if (pszFound==szBuf || *AnsiPrev(szBuf, pszFound)=='\\') {
                bRet = TRUE;
            }
        }
    }

    TraceMsg(DM_SELFASC, "IsAssociatedWithIE(%s) returning %d", szPath, bRet);

    return bRet;
}



UINT CDocObjectHost::_PicsBlockingDialog(LPCTSTR pszURL)
{
    pszURL = _pszPicsURL;

    TraceMsg(DM_PICS, "CDOH::_PicsBlockingDialog(%s)", pszURL);

    _StartPicsRootQuery(pszURL);

    _fDrawBackground = TRUE;
    ::InvalidateRect(_hwnd, NULL, TRUE);    /* mega cheesy, but only way to get browser window erased */

    /* This message loop is used to block in non-HTML cases, where we really
     * want to block the download process until ratings are checked.  In the
     * HTML case, this function is never called until the wait flags are all
     * clear, so the message loop is skipped and we go straight to the denial
     * dialog.
     */
    while (_fbPicsWaitFlags) {
        TraceMsg(DM_PICS, "CDOH::_PicsBlockingDialog entering msg loop, waitflags=%x", (DWORD)_fbPicsWaitFlags);

        MSG msg;
        if (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (!_fPicsAccessAllowed) {

        TraceMsg(DM_PICS, "CDOH::_PicsBlockingDialog, access denied");

        // If this is silent-mode (no UI == screensaver), always deny access
        // without any dialog.

        BOOL fFrameIsSilent = FALSE;    // Assume non-silent
        _GetOfflineSilent(NULL, &fFrameIsSilent);
        if (fFrameIsSilent) {
            TraceMsg(DM_PICS, "CDOH::_PicsBlockingDialog access denied in silent mode, aborting");
            return IDCANCEL;
        }

        _EnableModeless(FALSE);

        HRESULT hres = S_OK;
        IOleCommandTarget *pcmdtTop;
        if (SUCCEEDED(QueryService(SID_STopLevelBrowser, IID_IOleCommandTarget, (void **)&pcmdtTop))) {
            VARIANTARG v = { 0 };
            v.vt = VT_INT_PTR;
            v.byref = _pRatingDetails;
            hres = pcmdtTop->Exec(&CGID_ShellDocView, SHDVID_PICSBLOCKINGUI, 0, &v, NULL);
            pcmdtTop->Release();
        }

        UINT uRet = (hres == S_OK) ? IDOK : IDCANCEL;
        _EnableModeless(TRUE);
        _fPicsAccessAllowed = (uRet == IDOK);

        TraceMsg(DM_PICS, "CDOH::_PicsBlockingDialog returning %d", uRet);

        return uRet;
    }
    else {
        TraceMsg(DM_PICS, "CDOH::_PicsBlockingDialog, access allowed");

        return IDOK;
    }
}


HRESULT CDocObjectHost::_MayHaveVirus(REFCLSID rclsid)
{
    //
    // We'll call this function twice if the file is associated
    // with a bogus CLSID (such as ImageComposer).
    //
    if (_fConfirmed) {
        TraceMsg(TF_SHDAPPHACK, "CDOH::_MayHaveVirus called twice. Return S_OK");
        return S_OK;
    }

    TraceMsg(TF_SHDPROGRESS, "DOH::_MayHaveVirus called");
    LPWSTR pwzProgID = NULL;
    HRESULT hresT = E_FAIL;
    if (SUCCEEDED(ProgIDFromCLSID(rclsid, &pwzProgID)))
    {
        if (StrCmpI(pwzProgID, TEXT("htmlfile"))!=0
            && StrCmpI(pwzProgID, TEXT("htmlfile_FullWindowEmbed"))!=0
            && StrCmpI(pwzProgID, TEXT("mhtmlfile"))!=0
            && StrCmpI(pwzProgID, TEXT("xmlfile"))!=0
            && StrCmpI(pwzProgID, TEXT("xslfile"))!=0)
        {
            TCHAR szURL[MAX_URL_STRING];
            TCHAR * pszURL = szURL;

            hresT = _GetCurrentPage(szURL, ARRAYSIZE(szURL), TRUE);
            if (SUCCEEDED(hresT)) {
                UINT uRet = IDOK;
                if (_fbPicsWaitFlags || !_fPicsAccessAllowed) {
                    _fbPicsWaitFlags &= ~(PICS_WAIT_FOR_INDOC | PICS_WAIT_FOR_END);   /* indoc ratings only on htmlfile */
                    TraceMsg(DM_PICS, "CDOH::_MayHaveVirus found non-HTML, waitflags now %x", (DWORD)_fbPicsWaitFlags);
                    uRet = _PicsBlockingDialog(szURL);
                }
                if (uRet == IDOK) {
                    TraceMsg(TF_SHDPROGRESS, "DOH::_MayHaveVirus calling MayOpenSafeDialogOpenDialog(%s)", pwzProgID);
                    if (_bsc._pszRedirectedURL && *_bsc._pszRedirectedURL)
                        pszURL = _bsc._pszRedirectedURL;

                    uRet = MayOpenSafeOpenDialog(_hwnd, pwzProgID, pszURL, NULL, NULL, _uiCP);
                    _fCalledMayOpenSafeDlg = TRUE;
                }

                switch(uRet) {
                 case IDOK:
                    //
                    // Set this flag to avoid poppping this dialog box twice.
                    //
                    _fConfirmed = TRUE;
                    break;  // continue download
                 case IDD_SAVEAS:
                    CDownLoad_OpenUI(_pmkCur, _bsc._pbc, FALSE, TRUE, NULL, NULL, NULL, NULL, NULL, _bsc._pszRedirectedURL, _uiCP);
                    // fall through to abort binding.
                 case IDCANCEL:
                    hresT = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                    break;
                }
            } else {
                TraceMsg(DM_ERROR, "DOH::_MayHaveVirus _GetCurrentPage failed %x", hresT);
            }
        } else {
            TraceMsg(TF_SHDPROGRESS, "DOH::_MayHaveVirus this is htmlfile -- don't call MayOpenSafeDialogOpenDialog");
            _fPicsBlockLate = TRUE;
        }

        OleFree(pwzProgID);
    }
    return hresT;
}

STDMETHODIMP CDocObjectHost::SaveObject(void)
{
    TraceMsg(0, "sdv TR: CDOV::SaveObject called");
    // BUGBUG: Implemente it later.
    return S_OK;
}

STDMETHODIMP CDocObjectHost::GetMoniker(DWORD dwAssign,
    DWORD dwWhichMoniker,
    IMoniker **ppmk)
{
    HRESULT hres = E_INVALIDARG;
    *ppmk = NULL;
    TraceMsg(TF_SHDBINDING, "CDOH::GetMoniker called dwWhichMoniker=%x", dwWhichMoniker);

    switch(dwWhichMoniker)
    {
    case OLEWHICHMK_OBJREL:
    case OLEWHICHMK_OBJFULL:
        if (_pmkCur)
        {
            *ppmk = _pmkCur;
            _pmkCur->AddRef();
            hres = S_OK;
        }
        else
        {
            hres = E_UNEXPECTED;
        }
        break;
    }

    return hres;
}

STDMETHODIMP CDocObjectHost::GetContainer(
    IOleContainer **ppContainer)
{
    // BUGBUG: According to CKindel, we should implement this method
    //  as the way for a DocObject to access IDispatch interface of
    //  the container (i.e., frame). I'm currently thinking leaving
    //  all it's non-IUnknown memeber unimplemented. If there is no
    //  need to enumerates objects, we can simply QI from IShellBrowser
    //  to IOleContainer and return it. (SatoNa)
    //
    // NOTE: If trident calls this after DestroyHostWindow, we have nothing
    //  to give out. Hopefully this is not bad. (MikeSh)

    TraceMsg(0, "sdv TR: CDOV::GetContainer called");
    if (_psb)
        return _psb->QueryInterface(IID_IOleContainer, (LPVOID*)ppContainer);
    return E_FAIL;
}

STDMETHODIMP CDocObjectHost::ShowObject(void)
{
    TraceMsg(0, "sdv TR: CDOV::ShowObject called");
    return E_NOTIMPL;   // As specified in Kraig's document
}

STDMETHODIMP CDocObjectHost::OnShowWindow(BOOL fShow)
{
    TraceMsg(TF_SHDUIACTIVATE, "DOH::OnShowWindow(%d) called (this=%x)", fShow, this);
    return E_NOTIMPL;   // As specified in Kraig's document
}

STDMETHODIMP CDocObjectHost::RequestNewObjectLayout(void)
{
    TraceMsg(0, "sdv TR: CDOV::RequestNewObjectLayout called");
    return E_NOTIMPL;   // As specified in Kraig's document
}



//
//  This is the standard way for non-active embedding to access
// the IHlinkFrame interface. We happened to use our QI to implement
// this, but the semantics of QueryService is different from QI.
// It does not necessary return the same object.
//
HRESULT CDocObjectHost::QueryService(REFGUID guidService, REFIID riid, void **ppvObj)
{
    // In order for the context menu to work correctly inside IFrames, we
    // need to fail a certain query ONLY for IFrames on desktop.
    if(!IsEqualGUID(guidService, CLSID_HTMLDocument) || !_IsImmediateParentDesktop(this, _psp))
    {
        //
        //  Delegate ISP to the _psb.
        //
        if (_psb && _psp)
            return _psp->QueryService(guidService, riid, ppvObj);
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

/*----------------------------------------------------------
Purpose: Remove the submenu(s) that are in _hmenuFrame
         from _hmenuBrowser.

*/
void CDocObjectHost::_RemoveFrameSubMenus(void)
{
    HMENU hmenu;

    ASSERT(IS_VALID_HANDLE(_hmenuBrowser, MENU));
    ASSERT(IS_VALID_HANDLE(_hmenuFrame, MENU));

    // The file menu in _hmenuBrowser consists of the file menu from
    // _hmenuFrame and IShellBrowser.  The part added by _hmenuFrame
    // includes a submenu (Send To), which must be removed before
    // _hmenuBrowser is destroyed.

    // We could just explicitly remove the Send To submenu.  But to
    // prevent the expensive bug hunt that it took to find this in the
    // first place, we're going to iterate thru the menu and, for
    // any submenus that belong to our template, we'll remove them.

    int citemFile = 0;
    UINT nID = 0;

    // Get the count of menu items in our template's File menu and
    // the ID of the first menu item.
    hmenu = GetMenuFromID(_hmenuFrame, FCIDM_MENU_FILE);
    if (hmenu)
    {
        citemFile = GetMenuItemCount(hmenu);
        nID = GetMenuItemID(hmenu, 0);
    }

    // Now look at the browser menu's File menu and, starting at
    // nID, remove any submenus.
    hmenu = GetMenuFromID(_hmenuBrowser, FCIDM_MENU_FILE);
    if (hmenu)
    {
        int citem = GetMenuItemCount(hmenu);
        int iTop;
        int i;

        // Where does our template file menu start?
        for (iTop = 0; iTop < citem; iTop++)
        {
            if (GetMenuItemID(hmenu, iTop) == nID)
            {
                // Start at where our template file menu ends and work up
                for (i = iTop + citemFile - 1; 0 < citemFile ; i--, citemFile--)
                {
                    HMENU hmenuSub = GetSubMenu(hmenu, i);

                    if (hmenuSub)
                        RemoveMenu(hmenu, i, MF_BYPOSITION);
                }
                break;
            }
        }
    }
}


/*----------------------------------------------------------
Purpose: Destroy the browser menu.

*/
HRESULT CDocObjectHost::_DestroyBrowserMenu(void)
{
    TraceMsg(TF_SHDUIACTIVATE, "DOH::_DestroyBrowserMenu called");

    if (_hmenuBrowser) {
        // First remove any submenus that are held by other menus,
        // so we don't blow them away.

        _RemoveFrameSubMenus();

        if (EVAL(_psb)) {
            _psb->RemoveMenusSB(_hmenuBrowser);
        }

        DestroyMenu(_hmenuBrowser);
        _hmenuBrowser = NULL;
    }
    return S_OK;
}


HRESULT CDocObjectHost::_CreateBrowserMenu(LPOLEMENUGROUPWIDTHS pmw)
{
    TraceMsg(TF_SHDUIACTIVATE, "DOH::_CreateBrowserMenu called");

    if (_hmenuBrowser) {
        return S_OK;
    }

    _hmenuBrowser = CreateMenu();
    if (!_hmenuBrowser) {
        return E_OUTOFMEMORY;
    }

    HRESULT hres = E_FAIL;

    // Allow IShellBrowser a chance to add its menus
    if (EVAL(_psb))
        hres = _psb->InsertMenusSB(_hmenuBrowser, pmw);

    // HACK: Win95 explorer returns E_NOTIMPL
    if (hres==E_NOTIMPL) {
        hres = S_OK;
    }

    if (SUCCEEDED(hres)) {
        // Load our menu if not loaded yet
        if (!_hmenuFrame)
        {
            _hmenuFrame = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(MID_FOCUS));
        }

        // Get the "File" sub-menu from the shell browser.
        MENUITEMINFO mii;
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_SUBMENU;

        if (GetMenuItemInfo(_hmenuBrowser, FCIDM_MENU_FILE, FALSE, &mii))
        {
            HMENU hmenuFileBrowse = mii.hSubMenu;

            // Merge our menuitems into this submenu.
            if (_hmenuFrame)
            {
                MENUITEMINFO miiItem;
                miiItem.cbSize = SIZEOF(MENUITEMINFO);
                miiItem.fMask = MIIM_SUBMENU;

                if (GetMenuItemInfo(_hmenuFrame, FCIDM_MENU_FILE, FALSE, &miiItem))
                {
                    TCHAR szItem[128];
                    HMENU hmenuFileT = miiItem.hSubMenu;
                    UINT citem = GetMenuItemCount(hmenuFileT);
                    for (int i=citem-1; i>=0 ; i--)
                    {
                        // We need to reset for each item.
                        miiItem.fMask = MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_TYPE | MIIM_DATA;
                        miiItem.fType = MFT_STRING;
                        miiItem.cch = ARRAYSIZE(szItem);
                        miiItem.dwTypeData = szItem;
                        miiItem.dwItemData = 0;
                        if (GetMenuItemInfo(hmenuFileT, i, TRUE, &miiItem)) {
                            InsertMenuItem(hmenuFileBrowse, 0, TRUE, &miiItem);
                        }
                    }
                }
            }
        }
        else
        {
            TraceMsg(TF_SHDUIACTIVATE, "DOH::_CreateBrowseMenu parent has no File menu (it's probably a browser OC)");
            ASSERT(0); // DocObject in OC is not supposed to call InsertMenus.
        }
    }

    DEBUG_CODE( _DumpMenus(TEXT("after _CreateBrowserMenu"), TRUE); )

    return hres;
}

//
// IOleInPlaceFrame::InsertMenus equivalent
//
HRESULT CDocObjectHost::_InsertMenus(
    /* [in] */ HMENU hmenuShared,
    /* [out][in] */ LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    HRESULT hres = S_OK;
    int nMenuOffset = 0;
    TraceMsg(TF_SHDUIACTIVATE, "DOH::InsertMenus called (this=%x)", this);

    // Assume error (no menu merge)
    lpMenuWidths->width[0] = 0;
    lpMenuWidths->width[2] = 0;
    lpMenuWidths->width[4] = 0;
    lpMenuWidths->width[5] = 0;

    // be extra safe and don't attempt menu merging if we're not top level
    if (_fHaveParentSite)
        return S_OK;

    OLEMENUGROUPWIDTHS mw = { {0} };
    hres = _CreateBrowserMenu(&mw);
    if (FAILED(hres)) {
        TraceMsg(DM_ERROR, "DOH::InsertMenus _CreateBrpwserMenu failed");
        return hres;
    }

    // Get the "File" sub-menu from the shell browser.
    MENUITEMINFO mii;
    TCHAR szSubMenu[128];

    mii.cbSize = SIZEOF(mii);
    mii.fMask = MIIM_SUBMENU|MIIM_TYPE|MIIM_ID;
    mii.cch = ARRAYSIZE(szSubMenu);
    mii.dwTypeData = szSubMenu;

    if (EVAL(GetMenuItemInfo(_hmenuBrowser, FCIDM_MENU_FILE, FALSE, &mii)))
    {
        ASSERT(szSubMenu == mii.dwTypeData);
        InsertMenuItem(hmenuShared, nMenuOffset++, TRUE, &mii);
        lpMenuWidths->width[0] = 1;
    }

    // Note that we need to re-initialize mii
    mii.cch = ARRAYSIZE(szSubMenu);

    if (EVAL(GetMenuItemInfo(_hmenuBrowser, FCIDM_MENU_EXPLORE, FALSE, &mii)))
    {
        // BUGBUG: GetMenuItemInfo is recursive (why?).  The item it retrieves 
        // for FCIDM_MENU_EXPLORE can either be the top level Go menu, or if that
        // does not exist (NT5 case), it returns the Go To submenu of View.  
        // 
        // Code has been added in in the SetMenu implementations of Shell Browser 
        // and Dochost to detect the second case, because the menu dispatch list
        // does not recognize this kind of menu merging (80734).

        DeleteMenu(mii.hSubMenu, FCIDM_PREVIOUSFOLDER, MF_BYCOMMAND);
        InsertMenuItem(hmenuShared, nMenuOffset++, TRUE, &mii);
        lpMenuWidths->width[4]++;
    }

    mii.cch = ARRAYSIZE(szSubMenu);

    if (EVAL(GetMenuItemInfo(_hmenuBrowser, FCIDM_MENU_FAVORITES, FALSE, &mii)))
    {
        InsertMenuItem(hmenuShared, nMenuOffset++, TRUE, &mii);
        lpMenuWidths->width[4]++;
    }

    if (_hmenuFrame)
    {
        // Micro-merge the help menu.
        mii.cch = ARRAYSIZE(szSubMenu);

        if (EVAL(GetMenuItemInfo(_hmenuFrame, FCIDM_MENU_HELP, FALSE, &mii)))
        {
            InsertMenuItem(hmenuShared, nMenuOffset++, TRUE, &mii);
            lpMenuWidths->width[5]++;
        }
    }

    DEBUG_CODE( _DumpMenus(TEXT("after InsertMenus"), TRUE); )

    return hres;
}


/*----------------------------------------------------------
Purpose: Different objects may add their own Help menu (like
         Word and Excel).  This function detects if the object
         added its own help menu, or if it added items to our
         help menu, or if it is just using our help menu.

         If they added their own help menu, we remove ours.

*/
void CDocObjectHost::_CompleteHelpMenuMerge(HMENU hmenu)
{
    HMENU hmenuHelp;
    MENUITEMINFO mii;
    TCHAR szSubMenu[80];

    mii.cbSize = SIZEOF(mii);
    mii.fMask = MIIM_SUBMENU;

    // see if they added anything to our menu
    if (GetMenuItemInfo(_hmenuFrame, FCIDM_MENU_HELP, FALSE, &mii))
    {
        hmenuHelp = mii.hSubMenu;
        int iMenuCount = GetMenuItemCount(mii.hSubMenu);

        // Did the number of items in the help menu change?
        if (iMenuCount != HELP_ITEM_COUNT) {
            // Yes; that means they added something.  This has been micro-merged.
            _hmenuMergedHelp = mii.hSubMenu;
            _hmenuObjHelp = GetSubMenu(mii.hSubMenu, iMenuCount -1);
            goto Bail;
        }

        // Our menu didn't change.  Now find out if they added their own
        // help menu or if we ARE the help.  If they added their own, we need
        // to remove our help menu.

        _hmenuMergedHelp = NULL;
        _hmenuObjHelp = NULL;

        int iCount = GetMenuItemCount(hmenu) - 1;
        int i;
        for (i = iCount ; i >= 0 ; i--) {

            mii.fMask = MIIM_SUBMENU|MIIM_TYPE;
            mii.cch = ARRAYSIZE(szSubMenu);
            mii.dwTypeData = szSubMenu;

            if (GetMenuItemInfo(hmenu, i, TRUE, &mii)) {
                if (mii.hSubMenu == hmenuHelp) {

                    BOOL bRemove = FALSE;

                    if (iCount != i) {
                        // if we're not the last one, then we're not it
                        bRemove = TRUE;
                    } else {
                        // if we are the last one see if the help menu was added
                        // right before us
                        TCHAR szMenuTitle[80];
                        mii.cch = ARRAYSIZE(szMenuTitle);
                        mii.dwTypeData = szMenuTitle;
                        if (GetMenuItemInfo(hmenu, i-1, TRUE, &mii)) {
                            if (!StrCmpI(szMenuTitle, szSubMenu)) {
                                // same menu string yank ours
                                bRemove = TRUE;
                            }
                        }
                    }

                    if (bRemove) {
                        RemoveMenu(hmenu, i, MF_BYPOSITION);
                    }
                }
            }
        }
    }

Bail:;
    DEBUG_CODE( _DumpMenus(TEXT("after _CompleteHelpMenuMerge"), TRUE); )
}


//
// IOleInPlaceFrame::SetMenu equivalent
//
HRESULT CDocObjectHost::_SetMenu(
    /* [in] */ HMENU hmenuShared,           OPTIONAL
    /* [in] */ HOLEMENU holemenu,           OPTIONAL
    /* [in] */ HWND hwndActiveObject)
{
    TraceMsg(TF_SHDUIACTIVATE, "DOH::SetMenus(%x) called (this=%x)",
             hmenuShared, this);

    // be extra safe and don't attempt menu merging if we're not top level
    if (_fHaveParentSite)
        return S_OK;

    // A NULL hmenuShared means to reinstate the container's original menu.
    if (hmenuShared)
    {
        // Clean up duplicate help menus
        _CompleteHelpMenuMerge(hmenuShared);
    }

    // Simply forwarding it to IShellBrowser
    _hmenuSet = hmenuShared;
    HRESULT hres = E_FAIL;
    if (EVAL(_psb))
        hres = _psb->SetMenuSB(hmenuShared, holemenu, hwndActiveObject);

    if (SUCCEEDED(hres))
    {
        // need to tell the shell browser that we want doc obj style menu merging
        if (_pmsoctBrowser)
            _pmsoctBrowser->Exec(&CGID_Explorer, SBCMDID_ACTIVEOBJECTMENUS, 0, NULL, NULL);

        // Compose our list of object/frame menus, so our menuband
        // can dispatch the messages correctly.  Essentially this is
        // the same as the contents of holemenu, but since we don't
        // have access to the internal struct, we must derive this
        // info ourselves.
        _menulist.Set(hmenuShared, _hmenuBrowser);

        if (_hmenuMergedHelp)
            _menulist.RemoveMenu(_hmenuMergedHelp);

        if (_hmenuObjHelp)
            _menulist.AddMenu(_hmenuObjHelp);

        _hmenuCur = hmenuShared;
        HWND hwndFrame;
        _psb->GetWindow(&hwndFrame);

        // 80734: Was the Go To menu taken from the View menu and grafted onto the
        // main menu by DocHost?  The menulist won't detect this graft, so we have
        // to check ourselves and make sure it's not marked as belonging to the 
        // docobject.
        //
        // This test is duplicated in CShellBrowser2::SetMenuSB

        MENUITEMINFO mii;
        mii.cbSize = SIZEOF(mii);
        mii.fMask = MIIM_SUBMENU;

        if (hmenuShared && _hmenuBrowser && 
            GetMenuItemInfo(hmenuShared, FCIDM_MENU_EXPLORE, FALSE, &mii))
        {
            HMENU hmenuGo = mii.hSubMenu;

            if (GetMenuItemInfo(_hmenuBrowser, FCIDM_MENU_EXPLORE, FALSE, &mii) &&
                mii.hSubMenu == hmenuGo && _menulist.IsObjectMenu(hmenuGo))
            {
                _menulist.RemoveMenu(hmenuGo);
            }
        }

        // BUGBUG (scotth): why are we calling this, since this isn't compatible
        // with menubands?  That's the whole reason we have _menulist.
        hres = OleSetMenuDescriptor(holemenu, hwndFrame, hwndActiveObject, &_dof, _ActiveObject());
    }

    DEBUG_CODE( _DumpMenus(TEXT("after SetMenu"), TRUE); )

    return hres;
}


void CDocObjectHost::_SetStatusText(LPCSTR pszText)
{
    if (_psb)
    {
        WPARAM wParam = STATUS_PANE_NAVIGATION;


        if (g_bBiDiW95Loc && *pszText)
        {
            char szBuf[256];

            szBuf[0] = szBuf[1] = TEXT('\t');
            StrCpyNA(&szBuf[2], pszText, ARRAYSIZE(szBuf)-2);
            pszText = szBuf;
            wParam = SBT_RTLREADING;
        }

        _psb->SendControlMsg(FCW_STATUS, SB_SETTEXT, wParam | SBT_NOTABPARSING, (LPARAM)pszText, NULL);
        _psb->SendControlMsg(FCW_STATUS, SB_SETTIPTEXT, wParam, (LPARAM)pszText, NULL);
    }
}


/*----------------------------------------------------------
Purpose: Returns TRUE if the given menu belongs to the browser
         (as opposed to the object)

*/
BOOL CDocObjectHost::_IsMenuShared(HMENU hmenu)
{
    ASSERT(hmenu);

    // BUGBUG (scotth): can we use _menulist here? (it would be faster)

    if (_hmenuBrowser) {
        for (int i = GetMenuItemCount(_hmenuBrowser) - 1 ; i >= 0; i--) {
            if (GetSubMenu(_hmenuBrowser, i) == hmenu)
                return TRUE;
        }
    }

    // We have to special case the help menu.  It's possible that the
    // help menu in the shared menu actually came from _hmenuFrame
    // (not _hmenuBrowser).  We need to detect this case, otherwise
    // the help menu gets destroyed but it is still referenced in
    // _hmenuFrame.

    MENUITEMINFO mii;

    mii.cbSize = SIZEOF(mii);
    mii.fMask = MIIM_SUBMENU;

    ASSERT(IS_VALID_HANDLE(_hmenuFrame, MENU));

    // Is this our help menu from _hmenuFrame?
    if (GetMenuItemInfo(_hmenuFrame, FCIDM_MENU_HELP, FALSE, &mii) &&
        mii.hSubMenu == hmenu)
    {
        // Yes
        return TRUE;
    }

    return FALSE;
}

//
// IOleInPlaceFrame::RemoveMenus equivalent
//
HRESULT CDocObjectHost::_RemoveMenus(/* [in] */ HMENU hmenuShared)
{
    TraceMsg(TF_SHDUIACTIVATE, "DOH::RemoveMenus called (this=%x)", this);

    // be extra safe and don't attempt menu merging if we're not top level
    if (_fHaveParentSite)
        return S_OK;

    ASSERT(GetMenuItemCount(hmenuShared) != (UINT)-1);

    //
    // It is ok to simply remove sub-menus here.
    // because ours are shared with the _hmenuBrowser
    // and destroying that below will take care of cleanup.
    // However, we need to only remove menus that are ours.
    //
    for (int i = (int)GetMenuItemCount(hmenuShared) - 1 ; i >= 0; i--)
    {
        // TraceMsg(0, "sdv TR - ::RemoveMenus calling RemoveMenu(0)");
        HMENU hmenu = GetSubMenu(hmenuShared, i);

        if (_IsMenuShared(hmenu)) {
            RemoveMenu(hmenuShared, i, MF_BYPOSITION);
        }
    }

    // TraceMsg(0, "sdv TR - ::RemoveMenus exiting");
    return S_OK;
}

//
// IOleInPlaceFrame::SetStatusText equivalent
//
HRESULT CDocObjectHost::_SetStatusText(/* [in] */ LPCOLESTR pszStatusText)
{
    LPCOLESTR   pszForward;

    // Simply forward it.
    if (_psb != NULL)
    {
        // if it's NULL or just "" then give precedence to
        // _strPriorityStatusText, otherwise we display
        // whatever we're given

        if (pszStatusText != NULL && pszStatusText[0] != TEXT('\0') ||
            _strPriorityStatusText == NULL)
        {
            pszForward = pszStatusText;
        }
        else
        {
            pszForward = _strPriorityStatusText;
        }

        _psb->SetStatusTextSB(pszForward);
    }

    // Always return S_OK or scripting will put up an error dialog.
    return S_OK;
}

void CDocObjectHost::_SetPriorityStatusText(LPCOLESTR pszPriorityStatusText)
{
    // if they gave us a new string, replace the old one,
    // otherwise just NULL out the old one

    if (_strPriorityStatusText != NULL)
    {
        SysFreeString(_strPriorityStatusText);
    }

    if (pszPriorityStatusText != NULL)
    {
        _strPriorityStatusText = SysAllocString(pszPriorityStatusText);
    }
    else
    {
        _strPriorityStatusText = NULL;
    }

    _SetStatusText(_strPriorityStatusText);
}

HRESULT CDocObjectHost::_EnableModeless(/* [in] */ BOOL fEnable)
{
    TraceMsg(0, "sdv TR - ::EnableModeless called");

    // Note that we used call _CancelPendingNavigation here.
    // We do it in CBaseBrowser:::EnableModelesSB intead. (Satona)

    // Simply forwarding it (BUGBUG: which is not implemented)
    if (EVAL(_psb))
        return _psb->EnableModelessSB(fEnable);

    return E_FAIL;
}

HRESULT CDocObjectHost::TranslateHostAccelerators(LPMSG lpmsg)
{
    if (_hacc && ::TranslateAccelerator(_hwnd, _hacc, lpmsg)) {
        return S_OK;
    }
    return S_FALSE;
}

// IOleInPlaceFrame equivalent ::TranslateAccelerator
//  Forwarding it from DocObject -> Browser
HRESULT CDocObjectHost::_TranslateAccelerator(
    /* [in] */ LPMSG lpmsg,
    /* [in] */ WORD wID)
{
    // TranslateAccelerator goes to the guy with the focus first
    if (EVAL(_psb))
        if (S_OK == _psb->TranslateAcceleratorSB(lpmsg, wID))
            return S_OK;

#ifdef DEBUG
    if (lpmsg->message == WM_KEYDOWN) {
        TraceMsg(0, "CDocObjectHost::TrAcc(UP) called");
    }
#endif
    return TranslateHostAccelerators(lpmsg);
}

// IViewObject
HRESULT CDocObjectHost::Draw(DWORD dwDrawAspect, LONG lindex, void *pvAspect,
    DVTARGETDEVICE *ptd, HDC hicTargetDev, HDC hdcDraw,
    const RECTL *lprcBounds, const RECTL *lprcWBounds,
    BOOL (*pfnContinue)(ULONG_PTR), ULONG_PTR dwContinue)
{
    if (_pvo && lprcBounds)
    {
        if (_uState == SVUIA_DEACTIVATE && _hwnd)
        {
            HRESULT hresT = S_OK;
            RECT rcClient;
            GetClientRect(_hwnd, &rcClient);

            //
            // We should not call SetExtent with an empty rectangle.
            // It happens when we print a page with a floating frame.
            //
            if (rcClient.right > 0 && rcClient.bottom > 0)
            {
                SIZEL sizel;
                sizel.cx = MulDiv( rcClient.right, 2540, GetDeviceCaps( hdcDraw, LOGPIXELSX ) );
                sizel.cy = MulDiv( rcClient.bottom, 2540, GetDeviceCaps( hdcDraw, LOGPIXELSY ) );
                hresT = _pole->SetExtent(DVASPECT_CONTENT, &sizel);
            }

#ifdef DEBUG
            MoveToEx(hdcDraw, lprcBounds->left, lprcBounds->top, NULL);
            LineTo(hdcDraw, lprcBounds->right, lprcBounds->bottom);
            LineTo(hdcDraw, lprcBounds->left, lprcBounds->bottom);
            LineTo(hdcDraw, lprcBounds->right, lprcBounds->top);
#endif

            if (hresT!=S_OK) {
                TraceMsg(DM_ERROR, "CDOH::Draw SetExtent returns non S_OK %x", hresT);
            }
        }
        return _pvo->Draw(dwDrawAspect, lindex, pvAspect, ptd, hicTargetDev,
            hdcDraw, lprcBounds, lprcWBounds, pfnContinue, dwContinue);
    }

    return OLE_E_BLANK;
}

HRESULT CDocObjectHost::GetColorSet(DWORD dwAspect, LONG lindex,
    void *pvAspect, DVTARGETDEVICE *ptd, HDC hicTargetDev,
    LOGPALETTE **ppColorSet)
{
    if (_pvo)
    {
        return _pvo->GetColorSet(dwAspect, lindex, pvAspect, ptd, hicTargetDev,
            ppColorSet);
    }

    if (ppColorSet)
        *ppColorSet = NULL;

    return S_FALSE;
}

HRESULT CDocObjectHost::Freeze(DWORD, LONG, void *, DWORD *pdwFreeze)
{
    if (pdwFreeze)
        *pdwFreeze = 0;

    return E_NOTIMPL;
}

HRESULT CDocObjectHost::Unfreeze(DWORD)
{
    return E_NOTIMPL;
}

HRESULT CDocObjectHost::SetAdvise(DWORD dwAspect, DWORD advf,
    IAdviseSink *pSink)
{
    if (dwAspect != DVASPECT_CONTENT)
        return DV_E_DVASPECT;

    if (advf & ~(ADVF_PRIMEFIRST | ADVF_ONLYONCE))
        return E_INVALIDARG;

    if (pSink != _padvise)
    {
        ATOMICRELEASE(_padvise);

        _padvise = pSink;

        if (_padvise)
            _padvise->AddRef();
    }

    if (_padvise)
    {
        _advise_aspect = dwAspect;
        _advise_advf = advf;

        if (advf & ADVF_PRIMEFIRST)
            OnViewChange(_advise_aspect, -1);
    }
    else
        _advise_aspect = _advise_advf = 0;

    return S_OK;
}

HRESULT CDocObjectHost::GetAdvise(DWORD *pdwAspect, DWORD *padvf,
    IAdviseSink **ppSink)
{
    if (pdwAspect)
        *pdwAspect = _advise_aspect;

    if (padvf)
        *padvf = _advise_advf;

    if (ppSink)
    {
        if (_padvise)
            _padvise->AddRef();

        *ppSink = _padvise;
    }

    return S_OK;
}

// IAdviseSink
void CDocObjectHost::OnDataChange(FORMATETC *, STGMEDIUM *)
{
}

void CDocObjectHost::OnViewChange(DWORD dwAspect, LONG lindex)
{
    dwAspect &= _advise_aspect;

    if (dwAspect && _padvise)
    {
        IAdviseSink *pSink = _padvise;
        IUnknown *punkRelease;

        if (_advise_advf & ADVF_ONLYONCE)
        {
            punkRelease = pSink;
            _padvise = NULL;
            _advise_aspect = _advise_advf = 0;
        }
        else
            punkRelease = NULL;

        pSink->OnViewChange(dwAspect, lindex);

        if (punkRelease)
            punkRelease->Release();
    }
}

void CDocObjectHost::OnRename(IMoniker *)
{
}

void CDocObjectHost::OnSave()
{
}

void CDocObjectHost::OnClose()
{
    //
    // the doc object below went away so tell our advisee something changed
    //
    if (_padvise)
        OnViewChange(_advise_aspect, -1);
}

// IOleWindow
HRESULT CDocObjectHost::GetWindow(HWND * lphwnd)
{
    *lphwnd = _hwnd;
    return S_OK;
}

HRESULT CDocObjectHost::ContextSensitiveHelp(BOOL fEnterMode)
{
    // NOTES: This is optional
    return E_NOTIMPL;   // As specified in Kraig's document (optional)
}

// IOleInPlaceSite
HRESULT CDocObjectHost::CanInPlaceActivate(void)
{
    OIPSMSG(TEXT("CanInPlaceActivate called"));
    return S_OK;
}

HRESULT CDocObjectHost::OnInPlaceActivate(void)
{
    OIPSMSG(TEXT("OnInPlaceActivate called"));
    return S_OK;
}

HRESULT CDocObjectHost::OnUIActivate( void)
{
    TraceMsg(TF_SHDUIACTIVATE, "-----------------------------------");
    TraceMsg(TF_SHDUIACTIVATE, "OH::OnUIActivate called (this=%x)", this);

    //
    //  Hide Office toolbars early enough so that it won't flash.
    //
    _HideOfficeToolbars();

    // REVIEW:
    //  Should we remove 'our' menu here instead?
    //
    // [Copied from OLE 2.01 Spec]
    //  The container should remove any UI associated with its own
    // activation. This is significant if the container is itself
    // an embedded object.
    //
    OIPSMSG(TEXT("OnUIActivate called"));
    if (EVAL(_psb))
    {
        // If we had the DocObject in SVUIA_INPLACEACTIVATE send it to SVUIA_ACTIVATE_FOCUS
        //
        // NOTES: Unlike IE3.0, we don't call _psv->UIActivate which has a side
        //  effect. We just update the _uState.
        //
        // _psv->UIActivate(SVUIA_ACTIVATE_FOCUS);
        //
        _uState = SVUIA_ACTIVATE_FOCUS;

        return _psb->OnViewWindowActive(_psv);
    }

    return E_FAIL;
}

void CDocObjectHost::_GetClipRect(RECT* prc)
{
    GetClientRect(_hwnd, prc);
    prc->right -= _bwTools.right;
    prc->bottom -= _bwTools.bottom;
}

IOleInPlaceSite* CDocObjectHost::_GetParentSite()
{
    IOleInPlaceSite* pparentsite = NULL; // the parent's inplace site
    if (_pwb) {
        _pwb->GetParentSite(&pparentsite);
    }
    return pparentsite;

}

HRESULT CDocObjectHost::GetWindowContext(
    /* [out] */ IOleInPlaceFrame **ppFrame,
    /* [out] */ IOleInPlaceUIWindow **ppDoc,
    /* [out] */ LPRECT lprcPosRect,
    /* [out] */ LPRECT lprcClipRect,
    /* [out][in] */ LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    OIPSMSG(TEXT("GetWindowContext called"));

    // BUGBUG: verify that lpFrameInfo->cb is the correct size!

    // TraceMsg(0, "sdv TR - ::GetWindowContext called with lpFI->cb=%d (%d)",
    //           lpFrameInfo->cb, sizeof(*lpFrameInfo));
    *ppFrame = &_dof; AddRef();
    *ppDoc = NULL; // indicating that doc window == frame window

    _GetClipRect(lprcClipRect);

    lpFrameInfo->fMDIApp = FALSE;

    //
    //  If the frame has IOleInPlaceUIWindow (both IE and Shell have),
    // return that hwnd as hwndFrame so that OLE's menu dispatching
    // code works correctly.
    //
    _dof.GetWindow(&lpFrameInfo->hwndFrame);

    //
    // BUGBUG: We need to find out (from SriniK or KraigB), what is the
    //  implecation of this accelerator. Dealing with Word, it seems that
    //  Word does not call our TranslateAccelerator at all, unless the key
    //  stroke is the accelerator. If that's the spec. (of DocObject),
    //  there is no way to process the accelerator of the browser.
    //
    lpFrameInfo->haccel = _hacc;            // BUGBUG

    if (!SHRestricted(REST_NOFILEMENU))
    {
#ifdef DEBUG
        lpFrameInfo->cAccelEntries = DBG_ACCELENTRIES_WITH_FILEMENU; // WARNING: see shdocvw.rc, ACCELL_DOCVIEW
#else
        lpFrameInfo->cAccelEntries = OPT_ACCELENTRIES_WITH_FILEMENU; // WARNING: see shdocvw.rc, ACCELL_DOCVIEW
#endif
    }
    else
    {
#ifdef DEBUG
        lpFrameInfo->cAccelEntries = DBG_ACCELENTRIES; // WARNING: see shdocvw.rc, ACCELL_DOCVIEW
#else
        lpFrameInfo->cAccelEntries = OPT_ACCELENTRIES; // WARNING: see shdocvw.rc, ACCELL_DOCVIEW
#endif
    }

    *lprcPosRect = _rcView;
    return S_OK;
}

HRESULT CDocObjectHost::Scroll(
    /* [in] */ SIZE scrollExtant)
{
    TraceMsg(0, "sdv TR - ::Scroll called");
    return E_NOTIMPL;   // As specified in Kraig's document
}

HRESULT CDocObjectHost::OnUIDeactivate(
    /* [in] */ BOOL fUndoable)
{
    TraceMsg(TF_SHDUIACTIVATE, "DOH::OnUIDeactivate called (this=%x)", this);

    DEBUG_CODE( _DumpMenus(TEXT("on OnUIDeactivate"), TRUE); )

    if (_hmenuSet) {
        OIPSMSG(TEXT("OnUIDeactivate We need to SetMenu(NULL, NULL, NULL)"));
        _SetMenu(NULL, NULL, NULL);
    }

    return S_OK;
}

HRESULT CDocObjectHost::OnInPlaceDeactivate( void)
{
    OIPSMSG(TEXT("OnInPlaceDeactivate called"));
    // BUGBUG: Not implemented
    return S_OK;
}


HRESULT CDocObjectHost::DiscardUndoState( void)
{
    TraceMsg(0, "sdv TR - ::DiscardUndoState called");
    return S_OK;
}

HRESULT CDocObjectHost::DeactivateAndUndo( void)
{
    TraceMsg(0, "sdv TR - ::DeactivateAndUndo called");
    return S_OK;
}

HRESULT CDocObjectHost::OnPosRectChange(
    /* [in] */ LPCRECT lprcPosRect)
{
    return E_NOTIMPL;   // As specified in Kraig's document
}

void CDocObjectHost::_OnNotify(LPNMHDR lpnm)
{
    switch(lpnm->code) {

    case TBN_BEGINDRAG:
#define ptbn ((LPTBNOTIFY)lpnm)
        _OnMenuSelect(ptbn->iItem, 0, NULL);
        break;
    }
}

void MapAtToNull(LPTSTR psz)
{
    while (*psz)
    {
        if (*psz == TEXT('@'))
        {
            LPTSTR pszNext = CharNext(psz);
            *psz = 0;
            psz = pszNext;
        }
        else
        {
            psz = CharNext(psz);
        }
    }
}

void BrowsePushed(HWND hDlg)
{
    TCHAR szText[MAX_PATH];
    DWORD cchText = ARRAYSIZE(szText);
    TCHAR szFilter[MAX_PATH];
    TCHAR szTitle[MAX_PATH];
    LPITEMIDLIST pidl;
    LPCITEMIDLIST pidlChild;
    IShellFolder * pSF;

    // load the filter and then replace all the @ characters with NULL.  The end of the string will be doubly
    // null-terminated
    MLLoadShellLangString(IDS_BROWSEFILTER, szFilter, ARRAYSIZE(szFilter));
    MapAtToNull(szFilter);

    GetDlgItemText(hDlg, IDD_COMMAND, szText, ARRAYSIZE(szText));
    PathUnquoteSpaces(szText);

    // eliminate the "file://" stuff if necessary
    if(IsFileUrlW(szText))
        PathCreateFromUrl(szText, szText, &cchText, 0);

    MLLoadShellLangString(IDS_TITLE, szTitle, ARRAYSIZE(szTitle));

    if (GetFileNameFromBrowse(hDlg, szText, ARRAYSIZE(szText), NULL,
            TEXT(".htm"), szFilter, szTitle))
    {
        if (SUCCEEDED(IECreateFromPath(szText, &pidl)))
        {
            if (SUCCEEDED(IEBindToParentFolder(pidl, &pSF, &pidlChild)))
            {
                HWND hWndCombo = GetDlgItem(hDlg, IDD_COMMAND);

                COMBOBOXEXITEM cbexItem = {0};
                cbexItem.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
                cbexItem.pszText = szText;
                cbexItem.cchTextMax = ARRAYSIZE(szText);
                cbexItem.iItem = -1;
                cbexItem.iImage = IEMapPIDLToSystemImageListIndex(pSF, pidlChild, &cbexItem.iSelectedImage);
                INT_PTR iPosition = SendMessage(hWndCombo, CBEM_INSERTITEM, (WPARAM)0, (LPARAM)(LPVOID)&cbexItem);
                SendMessage(hWndCombo, CB_SETCURSEL, (WPARAM)iPosition, (LPARAM)0);
                pSF->Release();
            }
            ILFree(pidl);
        }
        else
        {
            PathUnquoteSpaces(szText);
            SetDlgItemText(hDlg, IDD_COMMAND, szText);
        }

        EnableOKButtonFromID(hDlg, IDD_COMMAND);
        // place the focus on OK
        SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDOK), TRUE);
    }
}

struct SOpenDlg {
    TCHAR           szURL[MAX_URL_STRING];
    IAddressEditBox *paebox;   // Object that controls ComboBoxEx
    IBandSite       *pbs;   // Used in AEBox Init call (used as a Connection Site)
    IWinEventHandler *pweh;    // Used to funnel IDD_COMMAND messages to the AEBox
};

const DWORD c_mapCtrlToContextIds[] = { 0, 0 };

const DWORD c_aRunHelpIds[] = {
        IDD_ICON,             NO_HELP,
        IDD_PROMPT,           NO_HELP,
        IDD_RUNDLGOPENPROMPT, IDH_IE_RUN_COMMAND,
        IDD_COMMAND,          IDH_IE_RUN_COMMAND,
        IDD_BROWSE,           IDH_RUNBROWSE,
        IDC_ASWEBFOLDER,      IDH_WEB_FOLDERS_CKBOX,

        0, 0
};

#ifndef UNIX
HRESULT OpenDlgOnWebFolderOK(HWND hDlg, SOpenDlg * podlg)
{
    ASSERT(podlg);

    HRESULT hr = S_OK;

    HWND hWndOpenBox = GetDlgItem(hDlg, IDD_COMMAND);
    ComboBox_GetText(hWndOpenBox, podlg->szURL, ARRAYSIZE(podlg->szURL));
    PathRemoveBlanks(podlg->szURL);

//    int iScheme = GetUrlScheme(podlg->szURL);

//    if ((URL_SCHEME_HTTP != iScheme) &&
//        (URL_SCHEME_HTTPS != iScheme))
//    {
        // no, we don't support that protocol!!
//        WCHAR wszMessage[MAX_PATH];
//        WCHAR wszTitle[MAX_PATH];
//        WCHAR wszErrorMessage[MAX_PATH + MAX_URL_STRING + 1];

//        MLLoadShellLangString(IDS_ERRORBADURL, wszMessage, ARRAYSIZE(wszMessage));
//        MLLoadShellLangString(IDS_ERRORBADURLTITLE, wszTitle, ARRAYSIZE(wszTitle));
//        wnsprintf(wszErrorMessage, ARRAYSIZE(wszErrorMessage), wszMessage, podlg->szURL);
//        MessageBox(hDlg, wszErrorMessage, wszTitle, MB_OK | MB_ICONERROR);
//        hr = E_FAIL;
//    }
    return hr;
}
#endif // UNIX

HRESULT OpenDlgOnOK(HWND hDlg, SOpenDlg * podlg)
{
    ASSERT(podlg);

    HRESULT hr = S_OK;
    /*
        Try to use newer parsing code if we have an AddressEditBox object
    */
    if (podlg->paebox)
        hr = podlg->paebox->ParseNow(SHURL_FLAGS_NONE);
    else
    {
        HWND hWndOpenBox = GetDlgItem(hDlg, IDD_COMMAND);
        ComboBox_GetText(hWndOpenBox, podlg->szURL, ARRAYSIZE(podlg->szURL));
        PathRemoveBlanks(podlg->szURL);
    }

    return hr;
}

void CleanUpAutoComplete(SOpenDlg *podlg)
{
    ATOMICRELEASE(podlg->paebox);
    ATOMICRELEASE(podlg->pweh);
    ATOMICRELEASE(podlg->pbs);

    ZeroMemory((PVOID)podlg, SIZEOF(SOpenDlg));
}


BOOL_PTR CALLBACK CDocObjectHost::s_RunDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SOpenDlg* podlg = (SOpenDlg*)GetWindowLongPtr(hDlg, DWLP_USER);
    switch (uMsg)
    {
    case WM_DESTROY:
        SHRemoveDefaultDialogFont(hDlg);
        return FALSE;

    case WM_INITDIALOG:
    {
        ASSERT(lParam);

        HWND hWndOpenBox = GetDlgItem(hDlg, IDD_COMMAND);
        HWND hWndEditBox = (HWND)SendMessage(hWndOpenBox, CBEM_GETEDITCONTROL, 0,0);
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        podlg = (SOpenDlg *)lParam;

        // cross-lang platform support
        SHSetDefaultDialogFont(hDlg, IDD_COMMAND);

        if (podlg->paebox)
        {
            if ( FAILED(podlg->paebox->Init(hWndOpenBox, hWndEditBox, AEB_INIT_DEFAULT, podlg->pbs)) ||
                 FAILED(IUnknown_SetOwner(podlg->paebox, podlg->pbs)))
                CleanUpAutoComplete(podlg);
        }
#ifdef UNIX
        // BUG BUG : Win32 should also verify and take this change.
        // IEUNIX : Limiting text, faulting under stress test. Need to set
        // this limit.
        SendMessage(hWndOpenBox, CB_LIMITTEXT, CBEMAXSTRLEN-1, 0L);
#endif
        EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
        if(SHRestricted(REST_NORUN))
            EnableWindow(GetDlgItem(hDlg, IDC_ASWEBFOLDER), FALSE); // disable open as web folder
            
        break;

    }

    case WM_HELP:
        SHWinHelpOnDemandWrap((HWND) ((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile,
            HELP_WM_HELP, (DWORD_PTR)(LPTSTR) c_aRunHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        SHWinHelpOnDemandWrap((HWND) wParam, c_szHelpFile, HELP_CONTEXTMENU,
            (DWORD_PTR)(LPTSTR) c_aRunHelpIds);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
       {
        case IDHELP:
            break;

        case IDD_BROWSE:
            BrowsePushed(hDlg);
            break;

        case IDD_COMMAND:
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case CBN_SELCHANGE:
                break;

            case CBN_EDITCHANGE:
            case CBN_SELENDOK:
                if (podlg->pweh)
                    podlg->pweh->OnWinEvent(hDlg, uMsg, wParam, lParam, NULL);
                EnableOKButtonFromID(hDlg, IDD_COMMAND);
                break;

            default:
                if (podlg->pweh)
                    podlg->pweh->OnWinEvent(hDlg, uMsg, wParam, lParam, NULL);
                break;
            }
            break;

        case IDOK:
            {
                // UNIX doesn't support this checkbox -- so we don't need
                // to check.
#ifndef UNIX
                HWND hwndCheckBox = GetDlgItem(hDlg, IDC_ASWEBFOLDER);
                if (hwndCheckBox)
                {
                    LRESULT lrState = SendMessage(hwndCheckBox, BM_GETCHECK,
                        0, 0);
                    if (lrState == BST_CHECKED)
                    {
                        if (SUCCEEDED(OpenDlgOnWebFolderOK(hDlg, podlg)))
                            EndDialog(hDlg, IDC_ASWEBFOLDER);
                        break;
                    }
                    else
#endif
                        if (FAILED(OpenDlgOnOK(hDlg, podlg)))
                            break;
#ifndef UNIX
                }
#endif
            }
            // Fall through to IDCANCEL to close dlg

        case IDCANCEL:
            EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

void CDocObjectHost::_Navigate(LPCWSTR pwszURL)
{
    IWebBrowser2* pwb2;
    if (SUCCEEDED(IUnknown_QueryService(_psb, SID_SContainerDispatch, IID_IWebBrowser2, (LPVOID*)&pwb2)))
    {
        //
        // HACK: We are not passing BSTR, but LPWSTR, which
        //  will work as far as IWebBrowser2 can handle
        //  NULL-terminated string correctly.
        //
        pwb2->Navigate((BSTR)pwszURL, NULL, NULL, NULL, NULL);
        pwb2->Release();
    }
}


HRESULT CDocObjectHost::_PrepFileOpenAddrBand(IAddressEditBox ** ppaeb, IWinEventHandler ** ppweh, IBandSite ** ppbs)
{
    HRESULT hr;

    *ppaeb = NULL;
    *ppweh = NULL;
    *ppbs = NULL;

    //  If our CoCreateInstance fails, s_rundlgproc will know because paebox
    //  will be NULL
    hr = CoCreateInstance(CLSID_AddressEditBox, NULL, CLSCTX_INPROC_SERVER, IID_IAddressEditBox, (void **)ppaeb);
    if (EVAL(SUCCEEDED(hr)))
    {
        IServiceProvider *pspT;
        hr = (*ppaeb)->QueryInterface(IID_IWinEventHandler, (LPVOID*)ppweh);

        //  Travel up the object hierarchy, and obtain the same pointer that
        //  the address bar was ::Init'ed with
        //  WARNING: This is not optional.  The addressband will fault if this fails.
        if (EVAL(SUCCEEDED(hr) && _psp))
        {
            hr = _psp->QueryService(SID_SExplorerToolbar, IID_IServiceProvider, (LPVOID*)&pspT);
            // In framed cases, CBaseBrowser2::QueryService() will filter out SID_SExplorerToolbar
            // because it's afraid of Toolbars appearing in the frame.  We won't have that problem,
            // so we may need to go the TopLevelBrowser first and then ask around there.
            if (FAILED(hr))
            {
                IServiceProvider *pspT2;

                hr = _psp->QueryService(SID_STopLevelBrowser, IID_IServiceProvider, (LPVOID*)&pspT2);
                if (EVAL(SUCCEEDED(hr)))
                {
                    hr = pspT2->QueryService(SID_SExplorerToolbar, IID_IServiceProvider, (LPVOID*)&pspT);
                    pspT2->Release();
                }
            }

            if (EVAL(SUCCEEDED(hr)))
            {
                if (EVAL(SUCCEEDED(hr = pspT->QueryService(IID_IBandSite, IID_IBandSite, (LPVOID*)ppbs))))
                {
                    IDeskBand *pdbT;
                    // Had to include "ITBAR.H" to access CBIDX_ADDDRESS
// HACKHACK
#define CBIDX_ADDRESS           4
                    // If any of the following fails, I don't care because the MRU can be out of
                    // synch.
                    if (SUCCEEDED((*ppbs)->QueryBand(CBIDX_ADDRESS, &pdbT, NULL, NULL, 0)))
                    {
                        IUnknown_Exec(pdbT, &CGID_AddressEditBox, AECMDID_SAVE, 0, NULL, NULL);
                        pdbT->Release();
                    }
                }
                pspT->Release();
            }
        }

    }

    if (FAILED(hr))
    {
        ATOMICRELEASE(*ppaeb);
        ATOMICRELEASE(*ppweh);
        ATOMICRELEASE(*ppbs);
    }

    return hr;
}

void CDocObjectHost::_OnOpen(void)
{
    HWND hwndFrame;
    SOpenDlg odlg ={0};

    _psb->GetWindow(&hwndFrame);

    if(SHIsRestricted2W(_hwnd, REST_NoFileOpen, NULL, 0))
        return;

    if (EVAL(SUCCEEDED(_PrepFileOpenAddrBand(&(odlg.paebox), &odlg.pweh, &odlg.pbs))))
    {
        // BUGBUG: Make it a helper member, which notifies up and down.
        _psb->EnableModelessSB(FALSE);

#ifdef UNIX
        UINT iRet;
        if (TRUE || MwCurrentLook() == LOOK_MOTIF)
           iRet = DialogBoxParam(MLGetHinst(),
                                 MAKEINTRESOURCE(DLG_RUNMOTIF),
                                 hwndFrame,
                                 s_RunDlgProc,
                                 (LPARAM)&odlg);
        else
           iRet = DialogBoxParam(MLGetHinst(),
                                 MAKEINTRESOURCE(DLG_RUN),
                                 hwndFrame,
                                 s_RunDlgProc,
                                 (LPARAM)&odlg);
#else
        INT_PTR iRet = DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(DLG_RUN), hwndFrame, s_RunDlgProc, (LPARAM)&odlg);
#endif /* UNIX */

        _psb->EnableModelessSB(TRUE);

        if (iRet==IDOK)
        {
            if(g_dwStopWatchMode)   // Perf mode to mark start time
                StopWatch_MarkSameFrameStart(hwndFrame);

            if (odlg.paebox)
                odlg.paebox->Execute(SHURL_EXECFLAGS_NONE);
            else
                _Navigate(odlg.szURL);
        }

#ifndef UNIX

        if (iRet == IDC_ASWEBFOLDER)
        {
            BSTR bstrUrl = SysAllocString(odlg.szURL);
            if (bstrUrl != NULL)
            {
                _NavigateFolder(bstrUrl);
                SysFreeString(bstrUrl);
            }
        }

#endif /*!UNIX*/

        IUnknown_SetOwner(odlg.paebox, NULL);
    }

    // Cleanup ref counts
    CleanUpAutoComplete(&odlg);
}

void CDocObjectHost::_OnImportExport(HWND hwnd)
{
    RunImportExportFavoritesWizard(hwnd);
}

UINT_PTR CALLBACK DocHostSaveAsOFNHook(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            /* Hide the "Save as Type" text box */
            CommDlg_OpenSave_HideControl(GetParent(hDlg), stc2);
            /* Hide the listbox with save type extensions */
            CommDlg_OpenSave_HideControl(GetParent(hDlg), cmb1);
            /* Hide the Open as read-only control */
            CommDlg_OpenSave_HideControl(GetParent(hDlg), chx1);
            break;
        }

        default:
                break;
    }
    return FALSE;
}

#define IDS_HELPURL_SUPPORT         IDS_HELPMSWEB+4
#define SZ_REGKEY_HELPURL_OVERRIDE  TEXT("Software\\Microsoft\\Internet Explorer\\Help_Menu_URLs")
#define SZ_REGVAL_HELPURL_SUPPORT   TEXT("Online_Support")
#define SZ_REGVAL_HELPURL_TEMPLATE  TEXT("%d")

void CDocObjectHost::_OnHelpGoto(UINT idRes)
{
    HRESULT hr = E_FAIL;
    WCHAR szURL[MAX_PATH];  // this is enough for our own

    // First try to get a copy from the registry because this is where Admins (with the IEAK) over ride
    // our default values.

    // We special case the Online_Support URL because it was supported in IE3.
    if (IDS_HELPURL_SUPPORT == idRes)
    {
        hr = URLSubRegQuery(SZ_REGKEY_HELPURL_OVERRIDE, SZ_REGVAL_HELPURL_SUPPORT, TRUE, szURL, ARRAYSIZE(szURL), URLSUB_ALL);
    }
    else
    {
        WCHAR szValue[MAX_PATH];

        wnsprintfW(szValue, ARRAYSIZE(szValue), SZ_REGVAL_HELPURL_TEMPLATE, (idRes - IDS_HELPMSWEB + 1));
        hr = URLSubRegQuery(SZ_REGKEY_HELPURL_OVERRIDE, szValue, TRUE, szURL, ARRAYSIZE(szURL), URLSUB_ALL);
    }

    if (FAILED(hr))
        hr = URLSubLoadString(NULL, idRes, szURL, ARRAYSIZE(szURL), URLSUB_ALL);

    if (SUCCEEDED(hr))
    {
        _Navigate(szURL);
    }
}

STDAPI_(void) IEAboutBox( HWND hWnd );


// WM_COMMAND from _WndProc - execs are going down
void CDocObjectHost::_OnCommand(UINT wNotify, UINT id, HWND hwndControl)
{
    if (_ShouldForwardMenu(WM_COMMAND, MAKEWPARAM(id, wNotify), (LPARAM)hwndControl))
    {
        _ForwardObjectMsg(WM_COMMAND, MAKEWPARAM(id, wNotify), (LPARAM)hwndControl);
        return;
    }

    switch(id)
    {
    case DVIDM_HELPTUTORIAL:
        _OnHelpGoto(IDS_HELPTUTORIAL);
        break;

    // ShabbirS (980917): BugFix# 34259 - Repair IE option.

    case DVIDM_HELPREPAIR:
        RepairIE();
        break;

    case DVIDM_HELPABOUT:
        IEAboutBox( _hwnd );
        break;

    case DVIDM_HELPSEARCH:
    {
#ifdef UNIX
        ContentHelp(_psb);
        break;
#else
        uCLSSPEC ucs;
        QUERYCONTEXT qc = { 0 };
        ucs.tyspec = TYSPEC_CLSID;
        ucs.tagged_union.clsid = CLSID_IEHelp;

        if (SUCCEEDED(FaultInIEFeature(_hwnd, &ucs, &qc, FIEF_FLAG_FORCE_JITUI)))
        {
            // MLHtmlHelp runs on a separate thread and should therefore be
            // safe against the kinds of message loops problems indicated above

            MLHtmlHelp(_hwnd, TEXT("iexplore.chm > iedefault"), HH_DISPLAY_TOPIC, 0, ML_CROSSCODEPAGE);
        }
        break;
#endif
    }

    case DVIDM_DHFAVORITES:
        _pmsoctBrowser->Exec(&CGID_Explorer, SBCMDID_ADDTOFAVORITES, OLECMDEXECOPT_PROMPTUSER, NULL, NULL);
        break;

    case DVIDM_GOHOME:
    case DVIDM_GOSEARCH:
        {
            TCHAR szPath[MAX_URL_STRING];
            LPITEMIDLIST pidl;
            HRESULT hres = SHDGetPageLocation(_hwnd,
                                      (id==DVIDM_GOSEARCH) ? IDP_SEARCH : IDP_START,
                                      szPath, ARRAYSIZE(szPath), &pidl);
            if (SUCCEEDED(hres))
            {
                _psb->BrowseObject(pidl, SBSP_ABSOLUTE | SBSP_SAMEBROWSER);
                ILFree(pidl);
            }
            else
            {
                TCHAR szMessage[256];
                BOOL fSuccess = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL, hres, 0, szMessage, ARRAYSIZE(szMessage), (va_list *)&szPath);
                if (!fSuccess)
                    szMessage[0] = 0;

                MLShellMessageBox(_hwnd,
                                MAKEINTRESOURCE(IDS_CANTACCESSDOCUMENT),
                                szPath, MB_OK | MB_SETFOREGROUND | MB_ICONSTOP, szMessage);
            }
        }
        break;

    case DVIDM_STOPDOWNLOAD:
        // We need to tell the container to cancel a pending navigation
        // if any. Notice that the Cancel button serves for two purposes:
        //  (1) canceling a pending navigation
        //  (2) cancel any downloading
        if (_psb)
            _CancelPendingNavigation(FALSE);
        goto TryDocument;

    case DVIDM_NEWWINDOW:
        // make sure the top level browser gets cloned, not an explorer bar
        IShellBrowser* psbTop;
        if (!SHIsRestricted2W(_hwnd, REST_NoFileNew, NULL, 0) &&
            EVAL(SUCCEEDED(_psp->QueryService(SID_STopLevelBrowser, IID_IShellBrowser, (void **)&psbTop))) && psbTop)
        {
            // tell the top level browser to save its window size to the registry so 
            // that our new window can pick it up and cascade properly
            IUnknown_Exec(psbTop, &CGID_Explorer, SBCMDID_SUGGESTSAVEWINPOS, 0, NULL, NULL);
            
            psbTop->BrowseObject(&s_idNull, SBSP_RELATIVE|SBSP_NEWBROWSER);
            psbTop->Release();
        }
        break;

    case DVIDM_OPEN:
        _OnOpen();
        break;

    case DVIDM_SAVE:
        if(!SHIsRestricted2W(_hwnd, REST_NoBrowserSaveAs, NULL, 0))
            _OnSave();
        break;

    case DVIDM_DESKTOPSHORTCUT:
        IUnknown_Exec(_psb, &CGID_Explorer, SBCMDID_CREATESHORTCUT, 0, NULL, NULL);
        break;

    case DVIDM_SENDPAGE:
        IUnknown_Exec(_psb, &CGID_Explorer, SBCMDID_SENDPAGE, 0, NULL, NULL);
        break;

    case DVIDM_SENDSHORTCUT:
        IUnknown_Exec(_psb, &CGID_Explorer, SBCMDID_SENDSHORTCUT, 0, NULL, NULL);
        break;

    case DVIDM_NEWMESSAGE:
#ifdef UNIX
        if( OEHandlesMail() )
            _UnixSendDocToOE(NULL, 0,  MAIL_ACTION_SEND);
        else
            SendDocToMailRecipient(NULL, 0, MAIL_ACTION_SEND);
#else
        if (FAILED(DropOnMailRecipient(NULL, 0))) {
            SHRunIndirectRegClientCommand(_hwnd, NEW_MAIL_DEF_KEY);
        }
#endif
        break;

    case DVIDM_NEWPOST:
#ifdef UNIX
        if( !CheckAndExecNewsScript(_hwnd) )
#endif
        SHRunIndirectRegClientCommand(_hwnd, NEW_NEWS_DEF_KEY);
        break;

    case DVIDM_NEWCONTACT:
        SHRunIndirectRegClientCommand(_hwnd, NEW_CONTACTS_DEF_KEY);
        break;

    case DVIDM_NEWAPPOINTMENT:
        SHRunIndirectRegClientCommand(_hwnd, NEW_APPOINTMENT_DEF_KEY);
        break;

    case DVIDM_NEWMEETING:
        SHRunIndirectRegClientCommand(_hwnd, NEW_MEETING_DEF_KEY);
        break;

    case DVIDM_NEWTASK:
        SHRunIndirectRegClientCommand(_hwnd, NEW_TASK_DEF_KEY);
        break;

    case DVIDM_NEWTASKREQUEST:
        SHRunIndirectRegClientCommand(_hwnd, NEW_TASKREQUEST_DEF_KEY);
        break;

    case DVIDM_NEWJOURNAL:
        SHRunIndirectRegClientCommand(_hwnd, NEW_JOURNAL_DEF_KEY);
        break;

    case DVIDM_NEWNOTE:
        SHRunIndirectRegClientCommand(_hwnd, NEW_NOTE_DEF_KEY);
        break;

    case DVIDM_CALL:
        SHRunIndirectRegClientCommand(_hwnd, NEW_CALL_DEF_KEY);
        break;

    case DVIDM_SAVEASFILE:
        //
        //  Handle the case where DocObject does not support "SaveAs"
        // and we have enabled the menuitem anyway.
        //
        if(SHIsRestricted2W(_hwnd, REST_NoBrowserSaveAs, NULL, 0))
            break;

        if (_pmsot)
        {
            OLECMD rgcmds[] = { { OLECMDID_SAVEAS, 0 }, };

            _pmsot->QueryStatus(NULL, ARRAYSIZE(rgcmds), rgcmds, NULL);

            ASSERT(rgcmds[0].cmdf & OLECMDF_ENABLED);
            if (!(rgcmds[0].cmdf & OLECMDF_ENABLED))
                _OnSaveAs();
            else
                goto TryDocument;
        }
        break;

    case DVIDM_IMPORTEXPORT:
        _OnImportExport(_hwnd);
        break;

    default:
        if (IsInRange(id, DVIDM_HELPMSWEB, DVIDM_HELPMSWEBLAST))
        {
        #ifndef UNIX
            if (id == FCIDM_HELPNETSCAPEUSERS)
                SHHtmlHelpOnDemandWrap(_hwnd, TEXT("iexplore.chm > iedefault"), HH_DISPLAY_TOPIC, (DWORD_PTR) TEXT("lvg_nscp.htm"), ML_CROSSCODEPAGE);
            else
        #else
            if (id == FCIDM_HELPNETSCAPEUSERS)
                UnixHelp(L"Netscape User Help", _psb);
            else
        #endif
                _OnHelpGoto(IDS_HELPMSWEB + (id - DVIDM_HELPMSWEB));
        }
        else if (IsInRange(id, DVIDM_MSHTML_FIRST, DVIDM_MSHTML_LAST))
        {
            TraceMsg(DM_PREMERGEDMENU, "Processing merged menuitem %d", id - DVIDM_MSHTML_FIRST);
            ASSERT(_pcmdMergedMenu);
            if (_pcmdMergedMenu) {
                HRESULT hresT=_pcmdMergedMenu->Exec(&CGID_MSHTML, id - DVIDM_MSHTML_FIRST, 0, NULL, NULL);
                if (FAILED(hresT)) {
                    TraceMsg(DM_ERROR, "CDOH::_OnCommand _pcmdMergedMenu->Exec(%d) failed %x",
                             id - DVIDM_MSHTML_FIRST, hresT);
                }
            }
        }
        else if (IsInRange (id, DVIDM_MENUEXT_FIRST, DVIDM_MENUEXT_LAST))
        {
            // Menu Extensions
            IUnknown_Exec(_pBrowsExt, &CLSID_ToolbarExtButtons, id, 0, NULL, NULL);
        }
        else
        {
TryDocument:
            if (_pmsot)
            {
                // Check if we need to call object's Exec.
                UINT idMso = _MapToMso(id);
                if (idMso != (UINT)-1)
                {
                    // Yes. Call it.
                    _pmsot->Exec(NULL, idMso, OLECMDEXECOPT_PROMPTUSER, NULL, NULL);
                }
                else if (id == DVIDM_PRINTFRAME)
                {
                    _pmsot->Exec(&CGID_ShellDocView, SHDVID_PRINTFRAME, OLECMDEXECOPT_PROMPTUSER, NULL, NULL);
                }
            }
        }
        break;
    }
}

HRESULT CDocObjectHost::_OnSaveAs(void)
{
    HRESULT hres = S_OK;

    TraceMsg(DM_SAVEASHACK, "DOH::_OnSaveAs called");

    ASSERT(_pole);

    if (_dwAppHack & BROWSERFLAG_MSHTML)
    {
        SaveBrowserFile( _hwnd, _pole );
    }
    else // old dochost stuff
    {
        TCHAR szSaveTo[MAX_PATH];   // ok with MAX_PATH
        MLLoadString(IDS_DOCUMENT, szSaveTo, ARRAYSIZE(szSaveTo));
        TCHAR szDesktop[MAX_PATH];

        SHGetSpecialFolderPath(_hwnd, szDesktop, CSIDL_DESKTOPDIRECTORY, FALSE);

        OPENFILENAME OFN;
        OFN.lStructSize        = sizeof(OPENFILENAME);
        OFN.hwndOwner          = _hwnd;
        OFN.lpstrFileTitle     = 0;
        OFN.nMaxCustFilter     = 0;
        OFN.nFilterIndex       = 0;

        OFN.nMaxFile           = ARRAYSIZE(szSaveTo);
        OFN.lpfnHook           = DocHostSaveAsOFNHook;
        OFN.Flags              = 0L;/* for now, since there's no readonly support */
        OFN.lpstrTitle         = NULL;
        OFN.lpstrInitialDir    = szDesktop;

        OFN.lpstrFile = szSaveTo;
        OFN.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ENABLEHOOK | OFN_EXPLORER |
                    OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST;
        OFN.lpstrFilter      = NULL;
        OFN.lpstrCustomFilter = NULL;


        OFN.lpstrDefExt = TEXT("");     // no extension
        TCHAR szValue[MAX_PATH+1];      // +1 for for double-null
        TCHAR szExt[40];

        HKEY hkey = _GetUserCLSIDKey(_pole, NULL, NULL);
        if (hkey)
        {
            LONG cb = SIZEOF(szValue);
            if (RegQueryValue(hkey, TEXT("DefaultExtension"), szValue, &cb) == ERROR_SUCCESS)
            {
                TraceMsg(DM_SAVEASHACK, "DOH::_OnSaveAs DefExt is %s", szValue);

                // It is suposed to be like ".xls, Excel Workbook (*.xls)"
                if (szValue[0]==TEXT('.')) {
                    StrCpyN(szExt, szValue+1, ARRAYSIZE(szExt));
                    LPTSTR pszEnd = StrChr(szExt, TEXT(','));
                    if (pszEnd) {
                        *pszEnd = 0;
                    }

                    OFN.lpstrDefExt = szExt;
                    OFN.lpstrFilter = szValue;
                    OFN.Flags &= ~OFN_ENABLEHOOK;

                    TraceMsg(DM_SAVEASHACK, "DOH::_OnSaveAs OFN.lpstrDefExt is %s", OFN.lpstrDefExt);
                }
            }
        }

        if (GetSaveFileName(&OFN)) {
            IPersistFile* ppf;
            ASSERT(_pole);
            hres = _pole->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
            if (SUCCEEDED(hres)) {
                TraceMsg(DM_APPHACK, "APPHACK DOH SaveAs'ing to %s", szSaveTo);
                hres = ppf->Save(szSaveTo, FALSE);
                ppf->Release();
            } else {
                ASSERT(0);
            }
        }
        else
            hres = S_FALSE;
    }

    return hres;
}

#ifndef POSTPOSTSPLIT
// BUGBUG: this should be in a header
// Mail Recipient drop target implementation...
// {9E56BE60-C50F-11CF-9A2C-00A0C90A90CE}
EXTERN_C const GUID CLSID_MailRecipient = {0x9E56BE60L, 0xC50F, 0x11CF, 0x9A, 0x2C, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xCE};

HRESULT DropOnMailRecipient(IDataObject *pdtobj, DWORD grfKeyState)
{
    IDropTarget *pdrop;
    HRESULT hres = CoCreateInstance(CLSID_MailRecipient,
        NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
        IID_IDropTarget, (void**)&pdrop);

    if (SUCCEEDED(hres))
    {
        hres = SimulateDrop(pdrop, pdtobj, grfKeyState, NULL, NULL);
        pdrop->Release();
    }
    return hres;
}


HRESULT SendDocToMailRecipient(LPCITEMIDLIST pidl, UINT uiCodePage, DWORD grfKeyState)
{
#ifndef UNIX
    IDataObject *pdtobj;
    HRESULT hres = GetDataObjectForPidl(pidl, &pdtobj);
    if (SUCCEEDED(hres))
    {
        IQueryCodePage * pQcp;
        if (SUCCEEDED(pdtobj->QueryInterface(IID_IQueryCodePage, (LPVOID *)&pQcp)))
        {
            pQcp->SetCodePage(uiCodePage);
            pQcp->Release();
        }
        hres = DropOnMailRecipient(pdtobj, grfKeyState);
        pdtobj->Release();
    }

    return hres;
#else
    return UnixSendDocToMailRecipient(pidl, uiCodePage, grfKeyState);
#endif
}
#endif

void _EnableRemoveMenuItem(HMENU hmenu, DWORD cmdf, UINT uCmd)
{
    if (!(cmdf & (OLECMDF_SUPPORTED | OLECMDF_ENABLED)))
        RemoveMenu(hmenu, uCmd, MF_BYCOMMAND);
    else
        _EnableMenuItem(hmenu, uCmd,
                        cmdf & OLECMDF_ENABLED);
}

void CDocObjectHost::_OnInitMenuPopup(HMENU hmInit, int nIndex, BOOL fSystemMenu)
{
    if (!_hmenuCur)
        return;

    DEBUG_CODE( _DumpMenus(TEXT("on _OnInitMenuPopup"), TRUE); )

    if (GetMenuFromID(_hmenuCur, FCIDM_MENU_VIEW) == hmInit) {
        OLECMD rgcmd1[] = {
            { IDM_SCRIPTDEBUGGER, 0 },
        };

        DeleteMenu (hmInit, DVIDM_MSHTML_FIRST+IDM_SCRIPTDEBUGGER, MF_BYCOMMAND);
        if (SUCCEEDED(QueryStatusDown(&CGID_MSHTML, ARRAYSIZE(rgcmd1), rgcmd1, NULL)) && (rgcmd1[0].cmdf & OLECMDF_ENABLED)) {
            //
            // We need the script debugger popup menu.  We should check to see if this
            // needs to be loaded.
            //

            HMENU           hMenuDebugger;
            MENUITEMINFO   mii;
            const UINT      cchBuf = 128;
            TCHAR           szItem[cchBuf];

            hMenuDebugger = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(MENU_SCRDEBUG));

            mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
            mii.fType = MFT_STRING;
            mii.cch = cchBuf;
            mii.dwTypeData = szItem;
            mii.cbSize = sizeof(mii);
            GetMenuItemInfo(hMenuDebugger, 0, TRUE, &mii);

            mii.fMask |= MIIM_STATE;
            mii.fState = MFS_ENABLED;

            InsertMenuItem(hmInit, FCIDM_THEATER, FALSE, &mii);


            OLECMD rgcmd[] = {
                { IDM_BREAKATNEXT, 0 },
                { IDM_LAUNCHDEBUGGER, 0 },
            };

            HRESULT hr = QueryStatusDown(&CGID_MSHTML, ARRAYSIZE(rgcmd), rgcmd, NULL);
            _EnableMenuItem(mii.hSubMenu, DVIDM_MSHTML_FIRST+rgcmd[0].cmdID, SUCCEEDED(hr) && (rgcmd[0].cmdf & OLECMDF_ENABLED));
            _EnableMenuItem(mii.hSubMenu, DVIDM_MSHTML_FIRST+rgcmd[1].cmdID, SUCCEEDED(hr) && (rgcmd[1].cmdf & OLECMDF_ENABLED));

        }

        if (_pmsot)
        {
            OLECMD rgcmd2[] = {
                { IDM_VIEWSOURCE, 0 },
            };

            if(SHRestricted2(REST_NoViewSource, NULL, 0) == 0)
            {
                // we only want to modify the state of the view source item
                // if it isn't restricted by the IEAK. if it's restricted, we
                // need to leave it disabled regardles of what the object
                // we're hosting says

                HRESULT hr = _pmsot->QueryStatus(&CGID_MSHTML, ARRAYSIZE(rgcmd2), rgcmd2, NULL);
        
                _EnableMenuItem(hmInit, DVIDM_MSHTML_FIRST + rgcmd2[0].cmdID, 
                    SUCCEEDED(hr) && (rgcmd2[0].cmdf & OLECMDF_ENABLED));
            }
        }

    }
    else if (GetMenuFromID(_hmenuCur, FCIDM_MENU_FILE) == hmInit)
    {
        if (_pmsot)
        {
            TraceMsg(0, "sdv TR _OnInitMenuPopup : step 5");
            OLECMD rgcmds[] = {
                { OLECMDID_PRINT, 0 },
                { OLECMDID_PAGESETUP, 0 },
                { OLECMDID_PROPERTIES, 0 },
                { OLECMDID_SAVE, 0 },
                { OLECMDID_SAVEAS, 0 },
                { OLECMDID_PRINTPREVIEW, 0 },
            };

            _pmsot->QueryStatus(NULL, ARRAYSIZE(rgcmds), rgcmds, NULL);

            // Adding a comment for my sanity: we use SHDVID_PRINTFRAME instead
            // of OLECMDID_PRINT because IE40 is going to support the printing
            // of entire framesets, instead of the current behavior or forwarding
            // the command to the active frame.
            //
            OLECMD rgcmds1[] = {
                { SHDVID_PRINTFRAME, 0 },
            };

            _pmsot->QueryStatus(&CGID_ShellDocView, ARRAYSIZE(rgcmds1), rgcmds1, NULL);

            //
            //  If OLECMDID_SAVEAS is not supported (neither ENABLED nor
            // SUPPORTED is set) by the DocObject, check if the object
            // support IPersistFile. If it does, enable it. Note that
            // this mechanism allows the DocObject to disable this menu
            // item (by setting only OLECMDF_SUPPORTED). (SatoNa)
            //
            ASSERT(rgcmds[4].cmdID == OLECMDID_SAVEAS);

            // Only apply the save as restriction to the browser. If it is the
            // browser, and save as is restricted, then make the item disappear.
            if ( (_dwAppHack & BROWSERFLAG_MSHTML) &&
                 SHRestricted2( REST_NoBrowserSaveAs, NULL, 0 ))
                rgcmds[4].cmdf &= ~(OLECMDF_ENABLED | OLECMDF_SUPPORTED);
            else if (!(rgcmds[4].cmdf & (OLECMDF_ENABLED | OLECMDF_SUPPORTED)))
            {
                IPersistFile* ppf;
                ASSERT(_pole);
                HRESULT hresT = _pole->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
                if (SUCCEEDED(hresT))
                {
                    TraceMsg(DM_APPHACK, "APPHACK DOH Enabling SaveAs menu for Excel95");
                    rgcmds[4].cmdf |= OLECMDF_ENABLED;
                    ppf->Release();
                }
            }

            //
            // APPHACK: Office apps do not enable "Save" correctly.
            //  Automatically enable it if the moniker is a FILE moniker
            //  AND the document has been altered by the user.
            //
            if (_fFileProtocol && _IsDirty(NULL))
            {
                if (!(rgcmds[3].cmdf & OLECMDF_ENABLED))
                {
                    TraceMsg(DM_APPHACK, "APPHACK DOH Enabling Save for Office Apps");
                }
                rgcmds[3].cmdf |= OLECMDF_ENABLED;
            }

            // Remove/disable/enable the "Print" command as appropriate.
            // Excel doesn't set SUPPORTED bit when it sets ENABLED bit
            // so we have to check both bits.
            _EnableRemoveMenuItem(hmInit, rgcmds[0].cmdf, DVIDM_PRINT);

            _EnableMenuItem(hmInit, DVIDM_PAGESETUP,
                    (rgcmds[1].cmdf & OLECMDF_ENABLED));
            _EnableMenuItem(hmInit, DVIDM_PROPERTIES,
                    (rgcmds[2].cmdf & OLECMDF_ENABLED));

            _EnableRemoveMenuItem(hmInit, rgcmds[3].cmdf, DVIDM_SAVE);
            _EnableRemoveMenuItem(hmInit, rgcmds[4].cmdf, DVIDM_SAVEASFILE);
            _EnableRemoveMenuItem(hmInit, rgcmds[5].cmdf, DVIDM_PRINTPREVIEW);
            _EnableRemoveMenuItem(hmInit, rgcmds1[0].cmdf, DVIDM_PRINTFRAME);


            HMENU hmFileNew = SHGetMenuFromID(hmInit, DVIDM_NEW);

            if (hmFileNew) 
            {
                const static struct {
                    LPCTSTR pszClient;
                    UINT idCmd;
                } s_Clients[] = {
                    { NEW_MAIL_DEF_KEY, DVIDM_NEWMESSAGE },
                    { NEW_CONTACTS_DEF_KEY, DVIDM_NEWCONTACT },
                    { NEW_NEWS_DEF_KEY, DVIDM_NEWPOST },
                    { NEW_APPOINTMENT_DEF_KEY, DVIDM_NEWAPPOINTMENT },
                    { NEW_MEETING_DEF_KEY, DVIDM_NEWMEETING },
                    { NEW_TASK_DEF_KEY, DVIDM_NEWTASK },
                    { NEW_TASKREQUEST_DEF_KEY, DVIDM_NEWTASKREQUEST },
                    { NEW_JOURNAL_DEF_KEY, DVIDM_NEWJOURNAL },
                    { NEW_NOTE_DEF_KEY, DVIDM_NEWNOTE },
                    { NEW_CALL_DEF_KEY, DVIDM_CALL }
                };

                BOOL bItemRemoved = FALSE;

                for (int i = 0; i < ARRAYSIZE(s_Clients); i++)
                {
                    if (!SHIsRegisteredClient(s_Clients[i].pszClient))
                    {
                        if (RemoveMenu(hmFileNew, s_Clients[i].idCmd, MF_BYCOMMAND))
                          bItemRemoved = TRUE;
                    }
                }

                if (bItemRemoved) // ensure the last item is not a separator
                    _SHPrettyMenu(hmFileNew);
            }
        }

    }
    else if (GetMenuFromID(_hmenuCur, FCIDM_VIEWFONTS) == hmInit
            || GetMenuFromID(_hmenuCur, FCIDM_ENCODING) == hmInit)
    {
        if (_pmsot)
        {
            // Handling fonts popup in view menu
            OLECMD rgcmd[] = {
                { SHDVID_GETFONTMENU,  0 },
                { SHDVID_GETMIMECSETMENU, 0 },
            };

            _pmsot->QueryStatus(&CGID_ShellDocView, ARRAYSIZE(rgcmd), rgcmd, NULL);

            int idx = (GetMenuFromID(_hmenuCur, FCIDM_VIEWFONTS) == hmInit ? 0 : 1);

            if (rgcmd[idx].cmdf & OLECMDF_ENABLED)
            {
                VARIANTARG v = {0};
                HRESULT hr;

                hr = _pmsot->Exec(&CGID_ShellDocView, rgcmd[idx].cmdID, 0, NULL, &v);
                if (S_OK == hr)
                {
                    // (on NT/Unix) DestroyMenu(hmInit) shouldn't work, because
                    // we're inside the processing of WM_INITMENUPOPUP message
                    // for hmInit. DestroyMenu will make the hmInit handle
                    // invalid.
                    //
                    // Instead of that we'll empty hmInit and copy hmenuFonts
                    // over. hmenuFonts will be destroyed to prevent the
                    // memory leak.
                    //
                    //
                    MENUITEMINFO mii;
                    UINT uItem = 0;
//$ WIN64: mshtml\src\site\base\formmso.cxx needs to return VT_INT_PTR instead
//                  HMENU hmenuFonts = (HMENU)v.byref;
                    HMENU hmenuFonts = (HMENU)LongToHandle(v.lVal);

#ifndef UNIX
                    // deleting menu while processing WM_INITMENUPOPUP
                    // can cause assertion failure on NT. However, copying
                    // submenu using InsertMenuItem() doesn't work on Win9x.
                    // see the comments above and Menu_Replace() in menu.cpp
                    //
                    if (!g_fRunningOnNT)
                        DestroyMenu(hmInit);
#endif

                    mii.cbSize = sizeof(mii);
                    mii.fMask = MIIM_ID|MIIM_SUBMENU;
                    while (GetMenuItemInfo(hmenuFonts, uItem, TRUE, &mii))
                    {
                        if (idx == 1 && mii.hSubMenu != NULL)
                        {
                            UINT uItemSub = 0;
                            HMENU hMenuSub = mii.hSubMenu;
                            while (GetMenuItemInfo(hMenuSub, uItemSub, TRUE, &mii))
                            {
                                mii.wID += DVIDM_MSHTML_FIRST;
                                SetMenuItemInfo(hMenuSub, uItemSub++, TRUE, &mii);
                            }
                        }
                        else
                        {
                            mii.wID += DVIDM_MSHTML_FIRST;
                            SetMenuItemInfo(hmenuFonts, uItem, TRUE, &mii);
                        }
                        uItem++;
                    }

#ifndef UNIX
                    if (!g_fRunningOnNT)
                    {
                        mii.cbSize = sizeof(mii);
                        mii.fMask = MIIM_SUBMENU;
                        mii.hSubMenu = hmenuFonts;
                        SetMenuItemInfo(_hmenuCur,
                                       (idx == 0 ? FCIDM_VIEWFONTS:FCIDM_ENCODING),
                                        FALSE, &mii);
                    }
                    else
#endif
                    {
                        Menu_Replace(hmInit, hmenuFonts);
                        DestroyMenu(hmenuFonts);
                    }
                }
            }
        }
    }
    else  if (GetMenuFromID(_hmenuCur, FCIDM_MENU_TOOLS) == hmInit ||
              GetMenuFromID(_hmenuCur, FCIDM_MENU_HELP) == hmInit)
    {
        // Add Tools and help Menu Extensions
        if (_pBrowsExt)
        {
            _pBrowsExt->OnCustomizableMenuPopup(_hmenuCur, hmInit);
        }
    }
}

//
// ATTEMPT: Handling WM_SETFOCUS message here caused several problems
//  under IE 3.0. Since we can't find any code scenario that requires
//  this code, I'm yanking out. If might introduce a new bug, but dealing
//  with those bugs is probably better than dealing with this code.
//  (SatoNa)
//

/*----------------------------------------------------------
Purpose: Determines if this message should be forwarded onto
         the object.

Returns: TRUE if the message needs to be forwarded
*/
BOOL CDocObjectHost::_ShouldForwardMenu(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_MENUSELECT:
    {
        // In USER menu bars, the first menuselect will be sent for the
        // selected top-level menu item, in which case hmenu == _hmenuCur.
        // We expect menubands to behave similarly.
        //
        // We check that hmenu == _hmenuCur because we only keep a list
        // of the top-level popup menus.  We don't keep track of any
        // cascading submenus.  We should only need to check who owns
        // the menu at the initial popup, all subsequent messages for
        // that menu should go to the same destination (frame or object).
        //
        // The same goes for CShellBrowser::_ShouldForwardMenu().
        //
        HMENU hmenu = GET_WM_MENUSELECT_HMENU(wParam, lParam);
        if (hmenu && (MF_POPUP & GET_WM_MENUSELECT_FLAGS(wParam, lParam)))
        {
            HMENU hmenuSub = GetSubMenu(hmenu, GET_WM_MENUSELECT_CMD(wParam, lParam));

            if (hmenu == _hmenuCur)
            {
                // Normal case, where we just look at the topmost popdown menus
                _fForwardMenu = _menulist.IsObjectMenu(hmenuSub);
            }
            else if (_menulist.IsObjectMenu(hmenuSub))
            {
                // This happens if the cascading submenu (micro-merged help menu for
                // example) should be forwarded on, but the parent menu should
                // not.
                _fForwardMenu = TRUE;
            }
            else if (GetMenuFromID(_hmenuCur, FCIDM_MENU_HELP) == hmenu 
                     && !_menulist.IsObjectMenu(hmenu) )
            {
                // 80430 Appcompat: notice that our menu fowarding doesn't work for the 
                // micro-merged Help menu.  If the user previously selected the merged
                // submenu, and we end up here, it means a non-merged submenu was just
                // selected and our _fForwardMenu was still set to TRUE.  If we don't 
                // reset it, the next WM_INITMENUPOPUP gets forwarded, which crashes Visio.
                //
                // We know that a submenu of the Help menu has just popped up, and we know
                // the submenu belongs to us.  So don't forward to the docobj until the
                // next popup.

                _fForwardMenu = FALSE;
            }
        }
        break;
    }

    case WM_COMMAND:
        if (_fForwardMenu)
        {
            // Stop forwarding menu messages after WM_COMMAND
            _fForwardMenu = FALSE;

            // If it wasn't from an accelerator, forward it
            if (0 == GET_WM_COMMAND_CMD(wParam, lParam))
                return TRUE;
        }
        break;
    }
    return _fForwardMenu;
}


/*----------------------------------------------------------
Purpose: Forwards messages to the in-place object.

         This is used to forward menu messages to the object for
         menu bands, since the menu bands do not work with the
         standard OLE FrameFilterWndProc.

         Also, the help menu is sometimes a combination of the
         object and the frame.  This function will forward as
         appropriate.

*/
LRESULT CDocObjectHost::_ForwardObjectMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet = 0L;
    IOleInPlaceActiveObject *piact = _xao.GetObject();
    ASSERT(IS_VALID_CODE_PTR(piact, IOleInPlaceActiveObject));

    if (piact)
    {
        HWND hwnd;

        piact->GetWindow(&hwnd);
        ASSERT(IS_VALID_HANDLE(hwnd, WND));

        if (hwnd)
        {
            if (uMsg == WM_COMMAND)
                PostMessage(hwnd, uMsg, wParam, lParam);
            else
                lRet = SendMessage(hwnd, uMsg, wParam, lParam);
        }
    }

    return lRet;
}


void CDocObjectHost::_OnMenuSelect(UINT id, UINT mf, HMENU hmenu)
{
    if (_psb)
    {
        if (IsInRange(id, DVIDM_MSHTML_FIRST, DVIDM_MSHTML_LAST))
        {
            if (_pcmdMergedMenu) {
                OLECMD rgcmd = { id - DVIDM_MSHTML_FIRST, 0 };
                struct {
                    OLECMDTEXT  cmdtxt;
                    WCHAR       szExtra[MAX_PATH];
                } cmdt;

                cmdt.cmdtxt.cmdtextf = OLECMDTEXTF_STATUS;
                cmdt.cmdtxt.cwActual = 0;
                cmdt.cmdtxt.cwBuf    = MAX_PATH;
                cmdt.cmdtxt.rgwz[0]  = 0;

                HRESULT hresT=_pcmdMergedMenu->QueryStatus(&CGID_MSHTML, 1, &rgcmd, &cmdt.cmdtxt);
                if (SUCCEEDED(hresT) && cmdt.cmdtxt.rgwz[0]) {
                    _psb->SetStatusTextSB(cmdt.cmdtxt.rgwz);
                } else {
                    TraceMsg(DM_ERROR, "CDOH::_OnMenuSelect QueryStatus failed %x %d",
                        hresT, cmdt.cmdtxt.cwActual);
                }
            }
            else
                // An ASSERT was replaced with this TraceMsg to allow testing on Win9x.
                // 70240 which reported the assert was pushed to IE6.
                TraceMsg(TF_WARNING, "CDocObjectHost::_OnMenuSelect   _pcmdMergedMenu == NULL");
        }
        else if (IsInRange(id, DVIDM_MENUEXT_FIRST, DVIDM_MENUEXT_LAST))
        {
            // Menu Extensions go here
            if (_pBrowsExt)
            {
                _pBrowsExt->OnMenuSelect(id);
            }
        }
        else
        {
            WCHAR wszT[MAX_STATUS_SIZE];
            if (MLLoadStringW(IDS_HELP_OF(id), wszT, ARRAYSIZE(wszT)))
            {
                _psb->SetStatusTextSB(wszT);
            }
        }
    }
}


BOOL CDocObjectHost::_HandlePicsChecksComplete(void)
{
    if (!_fPicsAccessAllowed) {
        TraceMsg(DM_PICS, "CDOH::_HandlePicsChecksComplete access denied, posting WM_PICS_DOBLOCKINGUI to hwnd %x", (DWORD_PTR)_hwnd);

        /* Allow download of this and other frames to continue while we post
         * the denial UI.
         */
        if (!PostMessage(_hwnd, WM_PICS_DOBLOCKINGUI, 0, 0)) {
            TraceMsg(DM_PICS, "CDOH::_HandlePicsChecksComplete couldn't post message!");
        }

        return FALSE;
    }
    else {
        TraceMsg(DM_PICS, "CDOH::_HandlePicsChecksComplete access allowed, execing ACTIVATEMENOW");
        if (!_fSetTarget && _pmsoctBrowser)
        {
            _pmsoctBrowser->Exec(&CGID_ShellDocView, SHDVID_ACTIVATEMENOW, NULL, NULL, NULL);
        }

        return TRUE;
    }
}


LRESULT CDocObjectHost::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet = 0L;

    switch(uMsg)
    {
    case WM_TIMER:
        if(wParam == IDTIMER_PROGRESS)
        {
            _OnSetProgressPos(0, PROGRESS_TICK);
            break;
        }
        else if (wParam == IDTIMER_PROGRESSFULL)
        {
            _OnSetProgressPos(-2, PROGRESS_RESET);
            break;
        }
        else
        {
#ifdef TEST_DELAYED_SHOWMSOVIEW
            MessageBeep(0);
            KillTimer(_hwnd, 100);
            ActivateMe(NULL);
            break;
#else
            ASSERT(FALSE);
            break;
#endif // TEST_DELAYED_SHOWMSOVIEW
        }

    /* WM_PICS_ASYNCCOMPLETE is posted by the async thread fetching ratings
     * from label bureaus, etc.
     */
    case WM_PICS_ASYNCCOMPLETE:
    {
        TraceMsg(DM_PICS, "CDOH::v_WndProc got WM_PICS_ASYNCCOMPLETE");

        PicsQuery pq;
        HRESULT hr;
        LPVOID lpvRatingDetails;

        if (::_GetPicsQuery((DWORD)lParam, &pq)) {
            ::_RemovePicsQuery((DWORD)lParam);
            hr = (HRESULT)wParam;
            lpvRatingDetails = pq.lpvRatingDetails;
        }
        else {
            hr = E_FAIL;
            lpvRatingDetails = NULL;
        }
        _GotLabel(hr, lpvRatingDetails, PICS_WAIT_FOR_ASYNC);

        break;
    }

    case WM_PICS_ROOTDOWNLOADCOMPLETE:
    {
        TraceMsg(DM_PICS, "CDOH::v_WndProc got WM_PICS_ROOTDOWNLOADCOMPLETE");

        if (_pRootDownload != NULL) {
            _pRootDownload->CleanUp();
            ATOMICRELEASET(_pRootDownload,CPicsRootDownload);
        }

        break;
    }

    /* WM_PICS_ALLCHECKSCOMPLETE is posted when we finally want to either
     * cancel the navigation or go through with it, according to ratings
     * checks.  Posting a message allows all denial blocking message loops
     * to unwind before we cancel navigation, which could otherwise delete
     * objects that still have functions operating on them.
     */
    case WM_PICS_ALLCHECKSCOMPLETE:
        TraceMsg(DM_PICS, "CDOH::v_WndProc got WM_PICS_ALLCHECKSCOMPLETE, lParam=%x", lParam);

        if (lParam == IDOK) {
            if (!_fSetTarget)
            {
                TraceMsg(DM_PICS, "CDOH::v_WndProc(WM_PICS_ASYNCCOMPLETE) execing SHDVID_ACTIVATEMENOW");
                _pmsoctBrowser->Exec(&CGID_ShellDocView, SHDVID_ACTIVATEMENOW, NULL, NULL, NULL);
            }
            else {
                TraceMsg(DM_PICS, "CDOH::v_WndProc(WM_PICS_ASYNCCOMPLETE) not execing SHDVID_ACTIVATEMENOW");
            }
        }
        else {
            ASSERT(!_fSetTarget);
            TraceMsg(DM_PICS, "CDOH::v_WndProc(WM_PICS_ASYNCCOMPLETE) calling _CancelPendingNavigation");
            _CancelPendingNavigation(FALSE);
//        _pmsoctBrowser->Exec(NULL, OLECMDID_STOP, NULL, NULL, NULL);
        }
        break;

    /* WM_PICS_DOBLOCKINGUI is posted when we decide we need to put up
     * denial UI.  Posting a message allows download of this object and
     * other frames to continue while we post the UI, which in turn allows
     * any denials from other frames to be coalesced into the one dialog.
     */
    case WM_PICS_DOBLOCKINGUI:
        {
            TraceMsg(DM_PICS, "CDOH::v_WndProc got WM_PICS_DOBLOCKINGUI");
            UINT id = _PicsBlockingDialog(NULL);
            TraceMsg(DM_PICS, "CDOH::v_WndProc(WM_PICS_DOBLOCKINGUI) posting WM_PICS_ALLCHECKSCOMPLETE");
            if (!PostMessage(_hwnd, WM_PICS_ALLCHECKSCOMPLETE, 0, id)) {
                TraceMsg(DM_PICS, "CDOH::v_WndProc(WM_PICS_DOBLOCKINGUI) couldn't post message!");
            }
        }
        break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        MessageBeep(0);
        break;


    case WM_MENUSELECT:
        if (_ShouldForwardMenu(uMsg, wParam, lParam))
            lRet = _ForwardObjectMsg(uMsg, wParam, lParam);
        else
        {
            UINT uMenuFlags = GET_WM_MENUSELECT_FLAGS(wParam, lParam);
            WORD wID = GET_WM_MENUSELECT_CMD(wParam, lParam);
            HMENU hMenu = GET_WM_MENUSELECT_HMENU(wParam, lParam);

            // Check for popup menus so we can display help strings for them
            if (uMenuFlags & MF_POPUP)
            {
                MENUITEMINFO miiSubMenu;

                miiSubMenu.cbSize = SIZEOF(MENUITEMINFO);
                miiSubMenu.fMask = MIIM_SUBMENU|MIIM_ID;
                if (GetMenuItemInfoWrap(hMenu, wID, TRUE, &miiSubMenu))
                {
                    // Change the parameters to simulate a "normal" menu item
                    wID = (WORD)miiSubMenu.wID;
                }
            }

            _OnMenuSelect(wID, uMenuFlags, hMenu);
        }
        break;

    case WM_INITMENUPOPUP:
        if (_ShouldForwardMenu(uMsg, wParam, lParam))
            lRet = _ForwardObjectMsg(uMsg, wParam, lParam);
        else
            _OnInitMenuPopup((HMENU)wParam, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_DRAWITEM:
    case WM_MEASUREITEM:
        if (_ShouldForwardMenu(uMsg, wParam, lParam))
            lRet = _ForwardObjectMsg(uMsg, wParam, lParam);
        else
            goto DoDefault;
        break;

    case WM_NOTIFY:
        _OnNotify((LPNMHDR)lParam);
        break;

    case WM_COMMAND:
        _OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
        break;

    case WM_SIZE:
        if (_pmsov)
        {
            RECT rcClient;
            GetClientRect(_hwnd, &rcClient);
            //
            // We should call ResizeBorder only if the browser is
            // not an IOleInPlaceUIWindow.
            //
            if (_pipu==NULL)
            {
                TraceMsg(TF_SHDUIACTIVATE, "DOH::WM_SIZE calling _piact->ResizeBorder");
                _xao.ResizeBorder(&rcClient, &_dof, TRUE);
            }

            _rcView.left = _bwTools.left;
            _rcView.top = _bwTools.top;
            _rcView.right = rcClient.right - _bwTools.right;
            _rcView.bottom = rcClient.bottom - _bwTools.bottom;

            TraceMsg(TF_SHDUIACTIVATE, "DOH::WM_SIZE calling SetRect (%d, %d, %d, %d)", _rcView.left, _rcView.top, _rcView.right, _rcView.bottom);
            _pmsov->SetRect(&_rcView);
        }

        _PlaceProgressBar(TRUE);

        break;

//
// ATTEMPT: Handling WM_SETFOCUS message here caused several problems
//  under IE 3.0. Since we can't find any code scenario that requires
//  this code, I'm yanking out. If might introduce a new bug, but dealing
//  with those bugs is probably better than dealing with this code.
//  (SatoNa)
//

    case WM_PRINT:
        _OnPaint((HDC)wParam);
        break;

    case WM_QUERYNEWPALETTE:
    case WM_PALETTECHANGED:
    case WM_SYSCOLORCHANGE:
    case WM_DISPLAYCHANGE:
    case WM_ENTERSIZEMOVE:
    case WM_EXITSIZEMOVE:
    {
        HWND hwndT;
        if (_pole && SUCCEEDED(IUnknown_GetWindow(_pole, &hwndT)) && hwndT)
            return SendMessage(hwndT, uMsg, wParam, lParam);
        return 0;
    }

    case WM_PAINT:
        PAINTSTRUCT ps;
        HDC hdc;
        hdc = BeginPaint(_hwnd, &ps);

        // we don't need them to paint into our dc...
        // docobj has own hwnd.
        //_OnPaint(hdc);

        EndPaint(_hwnd, &ps);
        break;

    case WM_LBUTTONUP:
        if (_uState != SVUIA_ACTIVATE_FOCUS) {
            SetFocus(_hwnd);
        }
        break;

    case WM_ERASEBKGND:
        // Checking _bsc._fBinding will catch the first page case.
        if (_fDrawBackground ||
            (!(_dwAppHack & BROWSERFLAG_NEVERERASEBKGND)
             && ((_pmsov && _uState!=SVUIA_DEACTIVATE)
                 || _bsc._fBinding)))
        {
            PAINTMSG("WM_ERASEBKGND", this);
            goto DoDefault;
        }
        // Don't draw WM_ERASEBKGND if we have no view activated.
        return TRUE; // TRUE = fErased

    case WM_HELP:
        //
        // Give it to the parent to do.  we need to do this in case we're hosted as a
        // control
        //
    {
        IOleCommandTarget *pcmdtTop;
        if (SUCCEEDED(QueryService(SID_STopLevelBrowser, IID_IOleCommandTarget, (void **)&pcmdtTop))) {
            pcmdtTop->Exec(&CGID_ShellDocView, SHDVID_HELP, 0, NULL, NULL);
            pcmdtTop->Release();
        }
        // do nothing in failure...  let the parent own completely
    }
        break;

    case WM_WININICHANGE:
        _PlaceProgressBar(TRUE);
        break;

    default:
        // Handle the MSWheel message
        if ((uMsg == GetWheelMsg()) && _pole)
        {
            HWND hwndT;

            // If for some reason our window has focus we just need to
            // swallow the message. If we don't we may create an infinite loop
            // because most clients send the message to the focus window.
            if (GetFocus() == _hwnd)
                return 1;

            //
            // try to find a window to forward along to
            //
            if (SUCCEEDED(IUnknown_GetWindow(_pole, &hwndT)))
            {
                PostMessage(hwndT, uMsg, wParam, lParam);
                return 1;
            }
            // Fall through...
        }
DoDefault:

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return lRet;
}

const TCHAR c_szViewClass[] = TEXT("Shell DocObject View");

void CDocObjectHost::_RegisterWindowClass(void)
{
    WNDCLASS wc = {0};

    wc.style         = CS_PARENTDC;
    wc.lpfnWndProc   = s_WndProc ;
    //wc.cbClsExtra    = 0;
    wc.cbWndExtra    = SIZEOF(CDocObjectHost*);
    wc.hInstance     = g_hinst ;
    //wc.hIcon         = NULL ;
    //wc.hCursor       = NULL;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    //wc.lpszMenuName  = NULL ;
    wc.lpszClassName = c_szViewClass;

    SHRegisterClass(&wc);

}

void CDocObjectHost::_InitOleObject()
{
    if (!_fClientSiteSet) {
        _fClientSiteSet = TRUE;

#ifdef DEBUG
        IOleClientSite* pcliT = NULL;
        if (SUCCEEDED(_pole->GetClientSite(&pcliT)) && pcliT)
        {
            //
            // Trident now grabs the client site from the bind context.
            // We don't want to hit this assertin this case (pcliT==this).
            //
            AssertMsg(IsSameObject(pcliT, SAFECAST(this, IOleClientSite*)),
                TEXT("CDocObjectHost::_InitOleObject _pole (%x) already has a client site (%x) (this=%x)"),
              _pole, pcliT, this);
            pcliT->Release();
        }
#endif

        HRESULT hresT = _pole->SetClientSite(this);
        if (FAILED(hresT)) {
            TraceMsg(TF_SHDAPPHACK, "DOH::_InitOleObject SetClientSite failed (%x). Don't in-place navigate", hresT);
            _dwAppHack |= BROWSERFLAG_DONTINPLACE;
        }

        ASSERT(NULL==_pvo);
        _pole->QueryInterface(IID_IViewObject, (LPVOID*)&_pvo);

        if (_pvo) {
            TraceMsg(DM_DEBUGTFRAME, "CDocObjectHost::_InitOleObject about call SetAdvise on %x (%x)", _pole, this);
            _pvo->SetAdvise(DVASPECT_CONTENT, ADVF_PRIMEFIRST, this);
        }
        //
        //  According to SteveRa (Word developer), a word object has an
        // internal flag which indicates whether or not it is created
        // from a file. If that flag is set, UIActivate will open the
        // window in Word. Calling SetHostName will reset that flag.
        //
        _GetAppHack(); // Make it sure that we have _dwAppHack

        if (_fCallSetHostName()) {
            TraceMsg(TF_SHDAPPHACK, "DOH::_InitOleObject calling SetHostName for Word95");
            WCHAR wszTitle[128];
            MLLoadStringW(IDS_TITLE, wszTitle, ARRAYSIZE(wszTitle));
            _pole->SetHostNames(wszTitle, wszTitle);
        }
    }
}

BOOL CDocObjectHost::_OperationIsHyperlink()
{
    _ChainBSC();

    DWORD dw = 0;
    BINDINFO bindinfo;

    ZeroMemory(&bindinfo, sizeof(BINDINFO));
    bindinfo.cbSize = sizeof(BINDINFO);

    HRESULT hr = _bsc.GetBindInfo(&dw, &bindinfo);
    if ( SUCCEEDED(hr) )
    {
        ReleaseBindInfo( &bindinfo );
        return BOOLIFY( dw & BINDF_HYPERLINK );
    }

    return FALSE;
}


HRESULT CDocObjectHost::SetTarget(IMoniker* pmk, UINT uiCP, LPCTSTR pszLocation, LPITEMIDLIST pidlKey,
                                  IShellView* psvPrev, BOOL fFileProtocol)
{
    HRESULT hres = NOERROR;

    ATOMICRELEASE(_pmkCur);

    _pmkCur = pmk;
    pmk->AddRef();

    _fFileProtocol = fFileProtocol;
    _pszLocation = pszLocation;
    _uiCP = uiCP;

    //  this is only set if we did a successful LoadHistory()
    _fIsHistoricalObject = FALSE;

    if (_bsc._hszPostData)
    {
        GlobalFree(_bsc._hszPostData);
        _bsc._hszPostData = NULL;
    }
    if (_bsc._pszHeaders)
    {
        LocalFree(_bsc._pszHeaders);
        _bsc._pszHeaders = NULL;
    }

    if (_bsc._pbc)
        ATOMICRELEASE(_bsc._pbc);


    //
    //  this replaces the old style of caching.
    //  if something has been cached, it was cached
    //  way above us before we ever existed.  now it is
    //  waiting for us.
    //
    IBrowserService *pbs;
    IStream *pstm = NULL;
    IBindCtx *pbcHistory = NULL;
    if(SUCCEEDED(QueryService(SID_SShellBrowser, IID_IBrowserService, (LPVOID *)&pbs)))
    {
        ASSERT(pbs);

        //  just in case there is one already there, like in the case of local anchor navigates
        ATOMICRELEASE(_pole);

        pbs->GetHistoryObject(&_pole, &pstm, &pbcHistory);
        TraceMsg(TF_TRAVELLOG, "DOH::SetTarget History object: _pole = %X, pstm = %X, pbc = %X", _pole, pstm, pbcHistory);
        pbs->Release();

    }

    if (_pole) {
        //
        //  some objects (MSHTML for one)  need their clientsite before anything else.
        //  so we need to init first
        //
        _InitOleObject();

        if(pstm)
        {
            IPersistHistory *pph;
            if(SUCCEEDED(_pole->QueryInterface(IID_IPersistHistory, (LPVOID *)&pph)))
            {
                ASSERT(pph);

                if(SUCCEEDED(pph->LoadHistory(pstm, pbcHistory)))
                {
                    //
                    //  this is to make sure that we wait for
                    //  the pole to tell us when it is ready.
                    //  when there is a pstm, that means that they may have
                    //  to do a full reparse or whatever, and we cant make
                    //  any assumptions about the readystate.
                    //
                    hres = S_FALSE;
                    _fIsHistoricalObject = TRUE;
                    _SetUpTransitionCapability();
                    // we may need to redo the pics stuff too.
                    // PrepPicsForAsync();
                    TraceMsg(TF_TRAVELLOG, "DOH::SetTarget pph->LoadHistory Successful");
                }
                else
                    ATOMICRELEASE(_pole);

                pph->Release();
            }

            ATOMICRELEASE(pstm);
        }
        else
            hres = S_OK;

        ATOMICRELEASE(pbcHistory);

        //  we shouldnt fail a load history, because the data in
        //  is just what the document gave us in SaveHistory...
        AssertMsg(NULL != _pole, TEXT("DOH::SetTarget pph->LoadHistory Failed"));

        // if we were already up and created, just scroll to it.
        // we if we were created DEACTIVATED, (possible in the ocx case)
        // don't do this activation
        if (_uState != SVUIA_DEACTIVATE) {
            hres = _ActivateMsoView();
        }
    }

    if(!_pole)
    {
        ASSERT(!pstm);
        ASSERT(!pbcHistory);

        IBindCtx* pbc = NULL;

        TraceMsg(TF_TRAVELLOG, "DOH::SetTarget No obj from TravelLog, calling pmk->BindToObject");

        if (_psp) {
            hres = _psp->QueryService(SID_SShellBrowser, IID_IBindCtx, (LPVOID*)&pbc);
        }
        if (pbc==NULL) {
            hres = CreateBindCtx(0, &pbc);
        } else {
            hres = S_OK;
        }

        if (SUCCEEDED(hres))
        {
            IBindCtx* pbcWrapper = BCW_Create(pbc);

            if (pbcWrapper == NULL)
            {
                pbcWrapper = pbc;
            }
            else
            {
                pbc->Release();
            }

            pbc = NULL;

            IBindCtx* pbcAsync = NULL;

            hres = CreateAsyncBindCtxEx(pbcWrapper, 0, NULL, NULL, &pbcAsync,0);


            if (SUCCEEDED(hres))
            {

                ASSERT(pbcAsync);
                ATOMICRELEASE(_pbcCur);

                _pbcCur = pbcAsync;
                _pbcCur->AddRef();

                pbcWrapper->Release();
                pbcWrapper = pbcAsync;
            }

            if (SUCCEEDED(hres))
            {
#ifdef DEBUG
                DWORD dwMksys;
                hres = pmk->IsSystemMoniker(&dwMksys);
                ASSERT((SUCCEEDED(hres) && dwMksys!=MKSYS_FILEMONIKER));
#endif
                ASSERT(FALSE == _fSetTarget);

                // Hack: The AddRef & Release protect against an error page
                // navigation from freeing the pdoh out from under us (edwardp)
                AddRef();

                _fSetTarget = TRUE;
                hres = _StartAsyncBinding(pmk, _pbcCur, psvPrev);
                _fSetTarget = FALSE;

                // Hack: Matching Release()
                //
                Release();

                if (SUCCEEDED(hres)) {
                    hres = S_FALSE;
                }
            }

            pbcWrapper->Release();
        }

    }

    return hres;
}

#define USE_HISTBMOFFSET 0
#define USE_MYBMOFFSET   1
#define USE_STDBMOFFSET  2

void CDocObjectHost::_MergeToolbarSB()
{
}

HICON _LoadSmallIcon(int id)
{
    return (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(id),
                                IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
}

void _InitIcons(void)
{
    ENTERCRITICAL;

    if (g_hiconScriptErr == NULL)
    {
        g_hiconScriptErr = _LoadSmallIcon(IDI_STATE_SCRIPTERROR);
        if (IS_BIDI_LOCALIZED_SYSTEM())
            MirrorIcon(&g_hiconScriptErr, NULL);
    }


    if (!g_hiconSSL)
    {
        g_hiconSSL = _LoadSmallIcon(IDI_SSL);
        if (IS_BIDI_LOCALIZED_SYSTEM())
            MirrorIcon(&g_hiconSSL, NULL);
    }


    if (!g_hiconFortezza)
    {
        g_hiconFortezza = _LoadSmallIcon(IDI_FORTEZZA);
        if (IS_BIDI_LOCALIZED_SYSTEM())
            MirrorIcon(&g_hiconFortezza, NULL);
    }

    for (UINT id = IDI_STATE_FIRST; id <= IDI_STATE_LAST; id++)
    {
        if (!g_ahiconState[id-IDI_STATE_FIRST])
        {
            g_ahiconState[id-IDI_STATE_FIRST]= _LoadSmallIcon(id);
            if (IS_BIDI_LOCALIZED_SYSTEM())
                 MirrorIcon(&g_ahiconState[id-IDI_STATE_FIRST], NULL);
        }
    }


    if (!g_hiconOffline)
    {
        g_hiconOffline = _LoadSmallIcon(IDI_OFFLINE);
        if (IS_BIDI_LOCALIZED_SYSTEM())
            MirrorIcon(&g_hiconOffline, NULL);
    }


    if (!g_hiconPrinter)
    {
        g_hiconPrinter = _LoadSmallIcon(IDI_PRINTER);
        if (IS_BIDI_LOCALIZED_SYSTEM())
            MirrorIcon(&g_hiconPrinter, NULL);
    }


    LEAVECRITICAL;
}

// This function initializes whatever the Class needs for manipulating the history
// we try to delay this till absolutely needed in order to not load
// wininet till the end

IUnknown *
CDocObjectHost::get_punkSFHistory()
{
    if (_pocthf && !_punkSFHistory)
    {
        VARIANT var;

        VariantInit(&var);
        if (SUCCEEDED(_pocthf->Exec(&CGID_Explorer, SBCMDID_HISTSFOLDER, TRUE, NULL, &var)))
        {
            if (VT_UNKNOWN == var.vt && NULL != var.punkVal)
            {
                _punkSFHistory = var.punkVal;
                _punkSFHistory->AddRef();
            }
        }
        VariantClearLazy(&var);
    }
    return _punkSFHistory;
}


//
//  This function (re)initializes CDocObjectHost object with the buddy
// IShellView (which is always CShellDocView) and the IShellBrowser.
// If this is the first time (_hwnd==NULL), it creates the view window
// and other associated windows as well. Otherwise (_hwnd!=NULL) -- it
// means this object is passed from one CDocViewObject to another because
// of intra-page jump -- we move it to the specified location (prcView)
// to make it really sure that we show it at the right place.
//
BOOL CDocObjectHost::InitHostWindow(IShellView* psv, IShellBrowser* psb,
                                    LPRECT prcView)
{
    HWND hwndParent;
    IServiceProvider  * pspTop;
    IOleObject        * pTopOleObject;
    IOleClientSite    * pOleClientSite;

    _ResetOwners();

    ASSERT(psv);
    _psv = psv;
    _psv->AddRef();
    ASSERT(NULL==_pmsoctView);
    _psv->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&_pmsoctView);
    ASSERT(NULL==_pdvs);
    _psv->QueryInterface(IID_IDocViewSite, (LPVOID*)&_pdvs);

    ASSERT(psb);
    _psb = psb;
    _psb->AddRef();

    ASSERT(NULL==_pwb);
    _psb->QueryInterface(IID_IBrowserService, (LPVOID*)&_pwb);
    ASSERT(NULL==_pmsoctBrowser);
    _psb->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&_pmsoctBrowser);
    ASSERT(NULL==_psp);
    _psb->QueryInterface(IID_IServiceProvider, (LPVOID*)&_psp);
    ASSERT(NULL==_pipu);
    _psb->QueryInterface(IID_IOleInPlaceUIWindow, (LPVOID*)&_pipu);
    ASSERT(_pipu);

    ASSERT(_psp);
    if (_psp)
    {

        // Get the object that manages the extended buttons from the top-level browser
        // But only if we don't already have it.
        if (NULL == _pBrowsExt)
            _psp->QueryService(SID_STopLevelBrowser, IID_IToolbarExt, (LPVOID*)&_pBrowsExt);

        //
        // LATER: I don't like that CDocObjectHost is directly accessing
        //  the automation service object to fire events. We should
        //  probably move all the progress UI code above IShellBrowser
        //  so that we don't need to do this shortcut. (SatoNa)
        //
        ASSERT(NULL==_peds);
        _psp->QueryService(IID_IExpDispSupport, IID_IExpDispSupport, (LPVOID*)&_peds);
        ASSERT(_peds);
        ASSERT(NULL==_pedsHelper);
        _peds->QueryInterface(IID_IExpDispSupportOC, (LPVOID*)&_pedsHelper);
        ASSERT(NULL==_phf);
        _psp->QueryService(SID_SHlinkFrame, IID_IHlinkFrame, (LPVOID*)&_phf);
        if (_phf)
        {
            _phf->QueryInterface(IID_IUrlHistoryNotify, (LPVOID *)&_pocthf);
        }
        // _punkSFHistory was being initialized here - but in order to delay the load of wininet.dll
        // we initialize it just before we use it

        ASSERT(_pWebOCUIHandler == NULL);
        ASSERT(_fWebOC == FALSE);
        if (SUCCEEDED(_psp->QueryService(SID_STopLevelBrowser, IID_IServiceProvider, (void **)&pspTop)) && pspTop)
        {
            if (SUCCEEDED(pspTop->QueryService(SID_SContainerDispatch, IID_IOleObject, (void **)&pTopOleObject)) && pTopOleObject)
            {
                _fWebOC = TRUE; // there was a container so we're a WebOC

                pTopOleObject->GetClientSite(&pOleClientSite);
                if (pOleClientSite)
                {
                    pOleClientSite->QueryInterface(IID_IDocHostUIHandler, (void**)&_pWebOCUIHandler);
                    pOleClientSite->QueryInterface(IID_IDocHostShowUI, (void**)&_pWebOCShowUI);
                    pOleClientSite->Release();
                }
                pTopOleObject->Release();
            }
            pspTop->Release();
        }
    }

    _dhUIHandler.SetSite( (IDocHostUIHandler *) this); // Apparently we need to disamiguate the IUnknown reference.

    _psb->GetWindow(&hwndParent);

    if (!_hwnd) {
        // There are several things we don't attempt to do when
        // we're not toplevel. Frameset type DOH should never
        // try to menu merge or dork with the statusbar.
        // Do this before the CreateWindowEx call 'cuz during
        // creation we party on the status bar.
        {
            IOleInPlaceSite* pparentsite = _GetParentSite();

            if (pparentsite) {
                _fHaveParentSite = TRUE;
                pparentsite->Release();
            }
        }

        _RegisterWindowClass();

        // really create the window
        DWORD dwStyle = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | WS_TABSTOP;
        //
        // BUGBUG: In Office 95, Excel and PowerPoint don't draw the client edge,
        //  while Word does draw the client edge. To avoid having double edges,
        //  we remove it for now. SriniK (Office) will find out which will be
        //  the standard for Office 96. (SatoNa)
        //
        _hwnd = CreateWindowEx(0 /* WS_EX_CLIENTEDGE */,
                                c_szViewClass, NULL,
                                dwStyle,
                                prcView->left, prcView->top, prcView->right-prcView->left, prcView->bottom-prcView->top,
                                hwndParent,
                                (HMENU)0,
                                HINST_THISDLL,
                                (LPVOID)SAFECAST(this, CImpWndProc*));

        if (!_hwnd) {
            goto Bail;
        }

        UINT uiAcc = ACCEL_DOCVIEW;
        if (SHRestricted(REST_NOFILEMENU))
            uiAcc = ACCEL_DOCVIEW_NOFILEMENU;

        if (_hacc)
        {
            DestroyAcceleratorTable(_hacc);
            _hacc = NULL;
        }

        _hacc = LoadAccelerators(MLGetHinst(), MAKEINTRESOURCE(uiAcc));
        _InitIcons();

    } else {
        ASSERT(GetParent(_hwnd) == hwndParent);
        MoveWindow(_hwnd, prcView->left, prcView->top,
                   prcView->right-prcView->left, prcView->bottom - prcView->top, TRUE);
    }



Bail:

    return (bool) _hwnd;
}

void CDocObjectHost::_CleanupProgress(void)
{
    TraceMsg(TF_SHDPROGRESS, "CDOH::CleanupProgress fTimer = %d, fFull = %d, hwndProg = %X", _fProgressTimer, _fProgressTimerFull, _hwndProgress);

    if(_fProgressTimer)
    {
        KillTimer(_hwnd, IDTIMER_PROGRESS);
        _fProgressTimer = FALSE;
    }

    if (_fProgressTimerFull)
    {
        //  we are being stopped, and the hwnd is destroyed
        //  before we clear the status bar.  zekel - 22-JUL-97
        _OnSetProgressPos(-2, PROGRESS_RESET);
        KillTimer(_hwnd, IDTIMER_PROGRESSFULL);
        ASSERT(!_fProgressTimerFull);
    }

    _OnSetProgressMax(0);

    _hwndProgress = NULL;
}

void CDocObjectHost::DestroyHostWindow()
{
    // Turn off the simple mode when we are leaving.
    if (_psb)
        _psb->SendControlMsg(FCW_STATUS, SB_SIMPLE, 0, 0, NULL);

    // really destroy the window

    _fCanceledByBrowser = TRUE;
    _bsc.AbortBinding();

    /* We're aborting somehow, abort the root document download too */
    if (_pRootDownload != NULL) {
        TraceMsg(DM_PICS, "CDOH::DestroyHostWindow cleaning up root download");
        _pRootDownload->CleanUp();
        ATOMICRELEASET(_pRootDownload,CPicsRootDownload);
    }

    _CloseMsoView();

    //
    // Notes: We need to delete OLE object from this side (container),
    //  otherwise, we leak because of circular reference.
    //
    _UnBind();

    _CleanupProgress();

    if (_hwndTooltip) {
        DestroyWindow(_hwndTooltip);
        _hwndTooltip = NULL;
    }

    //
    // Note that we need to destroy the parent after destroying children.
    //
    // OLE seems to recurse back into this function when we destroy the hwnd
    // and we try to destroy it a second time causing a RIP. Avoid this RIP
    // by NULLing out our internal variables before we destroy the hwnds.
    if (_hwnd) {
        HWND hwndT = _hwnd;
        _hwnd = NULL;
        DestroyWindow(hwndT);
    }

    ATOMICRELEASE(_psp);

    _ResetOwners();
}


//
// This member creates a view (IOleDocumentView) of the DocObject we have (_pole).
// This function is called only once from ::CreateViewWindow.
//
HRESULT CDocObjectHost::_CreateMsoView(void)
{
    ASSERT(_pmsov == NULL);
    ASSERT(_pmsoc == NULL);
    HRESULT hres = OleRun(_pole);
    if (SUCCEEDED(hres))
    {

        //// WARNING:
        // if you add anything to here, you should also pass it along
        // in _CreateDocObjHost
        //

        IOleDocument* pmsod = NULL;
        hres = _pole->QueryInterface(IID_IOleDocument, (LPVOID*)&pmsod);
        if (SUCCEEDED(hres)) {
            hres = pmsod->CreateView(this, NULL /* BUGBUG */ ,0,&_pmsov);

            if (SUCCEEDED(hres)) {
                //
                // BUGBUG/HACK: Working about MSHTML bug (#28756). We really
                //  want to take this hack out before we ship. (SatoNa)
                //
                _pmsov->SetInPlaceSite(this);
            } else {
                TraceMsg(DM_ERROR, "DOH::_CreateMsoView pmsod->CreateView() ##FAILED## %x", hres);
            }

            if (SUCCEEDED(hres) && !_pmsot) {
                _pmsov->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&_pmsot);
            }

            if (SUCCEEDED(hres) && !_pmsoc) {
                _pmsov->QueryInterface(IID_IOleControl, (LPVOID*)&_pmsoc);
            }
#ifdef HLINK_EXTRA
            if (_pihlbc)
            {
                if (_phls)
                {
                    _phls->SetBrowseContext(_pihlbc);
                }

                ASSERT(_pmkCur);
                hres = HlinkOnNavigate(this, _pihlbc, 0,
                                       _pmkCur, NULL, NULL);
                // TraceMsg(0, "sdv TR : _CreateMsoView HlinkOnNavigate returned %x", hres);
            }
#endif // HLINK_EXTRA
            pmsod->Release();
        } else {
            TraceMsg(DM_ERROR, "DOH::_CreateMsoView _pole->QI(IOleDocument) ##FAILED## %x", hres);
        }
    } else {
        TraceMsg(DM_ERROR, "DOH::_CreateMsoView OleRun ##FAILED## %x", hres);
    }

    return hres;
}

HRESULT CDocObjectHost::_ForwardSetSecureLock(int lock)
{
    HRESULT hr = E_FAIL;
    TraceMsg(DM_SSL, "[%X}DOH::ForwardSecureLock() lock = %d",this, lock, hr);

    VARIANT va = {0};
    va.vt = VT_I4;
    va.lVal = lock;

    //  we should only suggest if we are not the topframe
    if (_psp && _psb && !IsTopFrameBrowser(_psp, _psb))
    {
        IOleCommandTarget *pmsoct;

        if (SUCCEEDED(_psp->QueryService(SID_STopFrameBrowser, IID_IOleCommandTarget, (LPVOID *)&pmsoct)))
        {
            ASSERT(pmsoct);
            if (lock < SECURELOCK_FIRSTSUGGEST)
                va.lVal += SECURELOCK_FIRSTSUGGEST;

            hr = pmsoct->Exec(&CGID_Explorer, SBCMDID_SETSECURELOCKICON, 0, &va, NULL);
            pmsoct->Release();
        }
    }
    else
        if (_pmsoctBrowser)
            hr = _pmsoctBrowser->Exec(&CGID_Explorer, SBCMDID_SETSECURELOCKICON, 0, &va, NULL);

    return hr;
}

//
// This is the only method of IOleDocumentSite, which we MUST implement.
//
HRESULT CDocObjectHost::ActivateMe(IOleDocumentView *pviewToActivate)
{
    TraceMsg(TF_SHDUIACTIVATE, "DOC::ActivateMe called when _pmsov is %x", _pmsov);

    HRESULT hres = S_OK;
    if (_pmsov==NULL) {



        hres = _CreateMsoView();

#ifdef TEST_DELAYED_SHOWMSOVIEW
        SetTimer(_hwnd, 100, 1500, NULL);
        MessageBeep(0);
        return hres;
#endif // TEST_DELAYED_SHOWMSOVIEW
    }

    if (SUCCEEDED(hres)) {
        _ShowMsoView();
        _MergeToolbarSB();
        _InitToolbarButtons();

        ASSERT(_pmsoctBrowser);
        if(_fSetSecureLock)
            _ForwardSetSecureLock(_eSecureLock);
    }

    return hres;
}

//Helper routine for QueryStatus for status messages
ULONG ulBufferSizeNeeded(wchar_t *wsz, int ids, ULONG ulBufferLen)
{
    TraceMsg(0, "sdv TR ulBufferSizeNeeded called with (%x)", ids);

    DWORD dwLen;
    WCHAR szTemp[MAX_STATUS_SIZE+1];
    dwLen = MLLoadStringW(ids, szTemp, MAX_STATUS_SIZE);
    dwLen += 1; // for NULL terminator
    if (dwLen <= (DWORD)ulBufferLen)
        MoveMemory(wsz, szTemp, dwLen * sizeof(WCHAR));
    else
        *wsz = 0;
    return ((ULONG)dwLen);
}

HRESULT CDocObjectHost::OnQueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext, HRESULT hres)
{
    if (pguidCmdGroup == NULL)
    {
        ULONG i;

        if (rgCmds == NULL)
            return E_INVALIDARG;

        for (i=0 ; i<cCmds ; i++)
        {
            // ONLY say that we support the stuff we support in ::OnExec
            switch (rgCmds[i].cmdID)
            {
            case OLECMDID_OPEN:
            case OLECMDID_SAVE:
            case OLECMDID_UPDATECOMMANDS:
            case OLECMDID_SETPROGRESSMAX:
            case OLECMDID_SETPROGRESSPOS:
            case OLECMDID_SETPROGRESSTEXT:
            case OLECMDID_SETTITLE:
                rgCmds[i].cmdf = OLECMDF_ENABLED;
                break;

            default:
                if (SUCCEEDED(hres))
                {
                    // _pmsoctBrowser already filled this in
                }
                else
                {
                    rgCmds[i].cmdf = 0;
                }
                break;
            }
        }

        /* for now we deal only with status text*/
        if (pcmdtext)
        {
            switch (rgCmds[i].cmdID)
            {
            case OLECMDID_OPEN:
            case OLECMDID_SAVE:
                pcmdtext->cwActual = ulBufferSizeNeeded(pcmdtext->rgwz,
                        IDS_HELP_OF(_MapFromMso(rgCmds[0].cmdID)),
                        pcmdtext->cwBuf);
                break;

            default:
                if (SUCCEEDED(hres))
                {
                    // _pmsoctBrowser already filled this in
                }
                else
                {
                    pcmdtext->cmdtextf = OLECMDTEXTF_NONE;
                    pcmdtext->cwActual = 0;
                    if (pcmdtext->rgwz && pcmdtext->cwBuf>0)
                        *pcmdtext->rgwz = TEXT('\0');
                }
                break;
            }
        }

        hres = S_OK;
    }
    else if (IsEqualGUID(*pguidCmdGroup, CLSID_InternetButtons) ||
             IsEqualGUID(*pguidCmdGroup, CLSID_MSOButtons))
    {
        for (UINT i = 0 ; i < cCmds ; i++)
        {
            // CommandIDs from DVIDM_MENUEXT_FIRST to DVIDM_MENUEXT_LAST are reserved for toolbar extension buttons
            // Do NOT use this range for constants within the scope of CLSID_InternetButtons/CLSID_MSOButtons!
            if (IsInRange(rgCmds[i].cmdID, DVIDM_MENUEXT_FIRST, DVIDM_MENUEXT_LAST))
            {
                // We'll pass specificially this OLECMD through to the custom button
                IUnknown_QueryStatus(_pBrowsExt, &CLSID_ToolbarExtButtons, 1, &rgCmds[i], pcmdtext);
            }
            else
            {
                switch (rgCmds[i].cmdID)
                {
                case DVIDM_PRINT:
//                case DVIDM_FONTS:     // bugbug: always returns disabled
                    if (_pmsoctBrowser)
                    {
                        OLECMD ocButton;
                        static const int tbtab[] =
                        {
                            DVIDM_PRINT,
                            DVIDM_FONTS,
                        };
                        static const int cttab[] =
                        {
                            OLECMDID_PRINT,
                            OLECMDID_ZOOM,
                        };
                        ocButton.cmdID = SHSearchMapInt(tbtab, cttab, ARRAYSIZE(tbtab), rgCmds[i].cmdID);
                        ocButton.cmdf = 0;
                        _pmsoctBrowser->QueryStatus(NULL, 1, &ocButton, NULL);
                        rgCmds[i].cmdf = ocButton.cmdf;
                    }
                    break;

                case DVIDM_FONTS:   // bugbug: Always enable for IE5B2
                case DVIDM_CUT:
                case DVIDM_COPY:
                case DVIDM_PASTE:
                case DVIDM_ENCODING:
                    rgCmds[i].cmdf = OLECMDF_ENABLED;
                    break;

                case DVIDM_SHOWTOOLS:
                    if (_ToolsButtonAvailable())
                        rgCmds[i].cmdf = OLECMDF_ENABLED;
                    break;

                case DVIDM_MAILNEWS:
                    if (_MailButtonAvailable())
                        rgCmds[i].cmdf = OLECMDF_ENABLED;
                    break;

                case DVIDM_DISCUSSIONS:
                    // In addition to enabled/disabled, discussions button is checked/unchecked
                    rgCmds[i].cmdf = _DiscussionsButtonCmdf();
                    break;

                case DVIDM_EDITPAGE:
                    if (_psp)
                    {
                        // BUGBUG: temp code -- forward to itbar
                        // itbar edit code is moving here soon
                        IExplorerToolbar* pxtb;
                        if (SUCCEEDED(_psp->QueryService(SID_SExplorerToolbar, IID_IExplorerToolbar, (LPVOID*)&pxtb)))
                        {
                            OLECMD ocButton = { CITIDM_EDITPAGE, 0 };
                            IUnknown_QueryStatus(pxtb, &CGID_PrivCITCommands, 1, &ocButton, NULL);
                            rgCmds[i].cmdf = ocButton.cmdf;
                            pxtb->Release();
                        }
                    }
                    break;
                }
            }
        }
        hres = S_OK;
    }
    return hres;
}

HRESULT CDocObjectHost::QueryStatus(
    /* [unique][in] */ const GUID *pguidCmdGroup,
    /* [in] */ ULONG cCmds,
    /* [out][in][size_is] */ OLECMD rgCmds[  ],
    /* [unique][out][in] */ OLECMDTEXT *pcmdtext)
{
    HRESULT hres = OLECMDERR_E_UNKNOWNGROUP;

    // Now that BaseBrowser understands that CGID_MSHTML should be directed to the DocObject, we'll
    // get caught in a loop if we send those Execs through here.  Cut it off at the pass.
    if (pguidCmdGroup && IsEqualGUID(CGID_MSHTML, *pguidCmdGroup))
        return hres;

    if (_pmsoctBrowser)
        hres = _pmsoctBrowser->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);

    return OnQueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext, hres);
}

void CDocObjectHost::_OnSave(void)
{
    if (_pole && _fFileProtocol)
    {
        IPersistFile* ppf = 0;
        HRESULT hres = _pole->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if (SUCCEEDED(hres))
        {
            LPOLESTR pszDisplayName = NULL;
            hres = _GetCurrentPageW(&pszDisplayName);
            if (SUCCEEDED(hres))
            {
                // fRemember = TRUE for normal case
                hres = ppf->Save(pszDisplayName, !_fCantSaveBack);
                if (FAILED(hres)) {
                    TraceMsg(DM_ERROR, "DOH::_OnSave ppf->Save(psz, FALSE) failed with %x", hres);
                }
                OleFree(pszDisplayName);
            }
            ppf->Release();
        }
    }
}


void CDocObjectHost::_OnSetProgressPos(DWORD dwPos, DWORD state)
{
    //  trident will reset with -1
    if(dwPos == -1)
        state = PROGRESS_RESET;

    switch(state)
    {
    case PROGRESS_RESET:
        TraceMsg(TF_SHDPROGRESS, "DOH::OnSetProgressPos() RESET, timer = %d", _fProgressTimer);
        if(_fProgressTimer)
        {
            KillTimer(_hwnd, IDTIMER_PROGRESS);
            _fProgressTimer = FALSE;
        }

        if(_dwProgressMax)
        {
            // this will always finish up the progress bar
            //  so that when trident doesnt send us the last update
            //  we do it anyway
            if(_fProgressTimerFull && dwPos == -2)
            {
                _fProgressTimerFull = FALSE;
                KillTimer(_hwnd, IDTIMER_PROGRESSFULL);
                _dwProgressPos = 0;
                _OnSetProgressMax(0);
                _fShowProgressCtl = FALSE;
                _PlaceProgressBar(TRUE);
            }
            else if (!_fProgressTimerFull)
            {
                _OnSetProgressPos(0, PROGRESS_FULL);
                _fProgressTimerFull = TRUE;
                SetTimer(_hwnd, IDTIMER_PROGRESSFULL, 500, NULL);
            }
        }
        else
        {
            _fShowProgressCtl = FALSE;
            _PlaceProgressBar(TRUE);
        }

        break;

    case PROGRESS_FINDING:
        //this covers the first 10%
        TraceMsg(TF_SHDPROGRESS, "DOH::OnSetProgressPos() FINDING, timer = %d", _fProgressTimer);
        ASSERT(!dwPos);
        if(!_fProgressTimer)
            SetTimer(_hwnd, IDTIMER_PROGRESS, 500, NULL);
        _fProgressTimer = TRUE;
        _OnSetProgressMax(10000);
        _dwProgressInc = PROGRESS_INCREMENT;
        _dwProgressPos = 100;
        _dwProgressTicks = 0;
        _dwProgressMod = (PROGRESS_FINDMAX - _dwProgressPos) / (2 * _dwProgressInc);
        break;

    case PROGRESS_SENDING:
        TraceMsg(TF_SHDPROGRESS, "DOH::OnSetProgressPos() SENDING, timer = %d, dwPos = %d", _fProgressTimer, dwPos);
        ASSERT(!dwPos);
        if(!_fProgressTimer)
            SetTimer(_hwnd, IDTIMER_PROGRESS, 500, NULL);
        _fProgressTimer = TRUE;
        _OnSetProgressMax(10000);
        _dwProgressInc = PROGRESS_INCREMENT;
        _dwProgressTicks = 0;
        //dwProgressPos is already set from FINDING
        _dwProgressMod = (PROGRESS_SENDMAX - _dwProgressPos) / (2 * _dwProgressInc);
        break;

    case PROGRESS_RECEIVING:
        TraceMsg(TF_SHDPROGRESS, "DOH::OnSetProgressPos() RECEIVING, timer = %d, dwPos = %d", _fProgressTimer, dwPos);
        if(_fProgressTimer)
        {
            KillTimer(_hwnd, IDTIMER_PROGRESS);
            _fProgressTimer = FALSE;

            //  this is the base spot on the progress bar for trident
            _dwProgressBase = _dwProgressPos / PROGRESS_REBASE;
            TraceMsg(TF_SHDPROGRESS, "DOH::OnSetProgressPos() Rebasing at %d%%", _dwProgressPos * 100/ PROGRESS_TOTALMAX);
        }
        // progress max should be set from outside of here....
        _dwProgressPos = ADJUSTPROGRESSPOS(dwPos);
        break;

    case PROGRESS_TICK:
        if (_fProgressTimer)
        {
            if(_dwProgressInc)
                _dwProgressPos += _dwProgressInc;

            // else we post the still waiting message...
            AssertMsg(_dwProgressMod, TEXT("I want to see this - Get ZekeL"));
            if(_dwProgressMod && 0 == (++_dwProgressTicks % _dwProgressMod))
            {
                // this means we are about half way.
                _dwProgressInc /= 2;
            }

            TraceMsg(TF_SHDPROGRESS, "DOH::OnSetProgressPos() TICK, dwPos = %d, ticks = %d, inc = %d", _dwProgressPos, _dwProgressTicks, _dwProgressInc);
        }
        else
            TraceMsg(TF_SHDPROGRESS, "DOH::OnSetProgressPos() TICKNOT");
        break;

    case PROGRESS_FULL:
        {
            _dwProgressPos = _dwProgressMax;

            // if there are script errors, make sure the status
            // bar is properly set (re: icon and text)
            if (_pScriptErrList != NULL &&
                !_pScriptErrList->IsEmpty())
            {
                TCHAR   szMsg[MAX_PATH];

                // set the script error icon
                if (g_hiconScriptErr != NULL)
                {
                    if (_psb != NULL)
                    {
                        _psb->SendControlMsg(FCW_STATUS,
                                             SB_SETICON,
                                             STATUS_PANE_NAVIGATION,
                                             (LPARAM)g_hiconScriptErr,
                                             NULL);
                    }
                }

                // set the script error text
                MLLoadString(IDS_DONE_WITH_SCRIPT_ERRORS, szMsg, ARRAYSIZE(szMsg));
                _SetPriorityStatusText(szMsg);
            }

            TraceMsg(TF_SHDPROGRESS, "DOH::OnSetProgressPos() FULL");
        }
        break;

    default:
        ASSERT(FALSE);
    }

    if(_hwndProgress)
    {
        _psb->SendControlMsg(FCW_PROGRESS, PBM_SETPOS, _dwProgressPos, 0, NULL);
        TraceMsg(TF_SHDPROGRESS, "DOH::OnSetProgressPos() updating, pos = %d, %d%% full", _dwProgressPos, _dwProgressMax ? _dwProgressPos * 100/ _dwProgressMax : 0);

    }

    // fire an event that progress has changed
    if (_peds)
    {
        //  if we are sent a -1, we must forward the event on so that
        //  our host gets it too.  some containers rely on this.
        //  specifically DevStudio's HTMLHelp
        //
        if (dwPos != -1)
            dwPos = _dwProgressPos;

        if (!_fUIActivatingView)
        {
            FireEvent_DoInvokeDwords(_peds,DISPID_PROGRESSCHANGE,dwPos,_dwProgressMax);
        }
    }
}


void CDocObjectHost::_OnSetProgressMax(DWORD dwMax)
{
    // remember the maximum range so we have it when we want to fire progress events
    if (_dwProgressMax != dwMax && _psb)
    {
        _dwProgressMax = dwMax;

        TraceMsg(TF_SHDPROGRESS, "DOH::OnSetProgressMax() max = %d", _dwProgressMax);

        if (!_hwndProgress) {
            _psb->GetControlWindow(FCW_PROGRESS, &_hwndProgress);
        }

        if (_hwndProgress) {
            _psb->SendControlMsg(FCW_PROGRESS, PBM_SETRANGE32, 0, dwMax, NULL);
            TraceMsg(TF_SHDPROGRESS, "DOH::OnSetProgressMax() updating (%d of %d)", _dwProgressPos, _dwProgressMax);
        }
        else
            TraceMsg(TF_SHDPROGRESS, "DOH::OnSetProgressMax() No hwndProgress");
    }
}

UINT CDocObjectHost::_MapCommandID(UINT id, BOOL fToMsoCmd)
{
    // HEY, this maps OLECMDID commands *only*
    static const UINT s_aicmd[][2] = {
        { DVIDM_PROPERTIES, OLECMDID_PROPERTIES },
        { DVIDM_PRINT,      OLECMDID_PRINT },
        { DVIDM_PRINTPREVIEW, OLECMDID_PRINTPREVIEW },
        { DVIDM_PAGESETUP,  OLECMDID_PAGESETUP},
        { DVIDM_SAVEASFILE, OLECMDID_SAVEAS },
        { DVIDM_CUT,        OLECMDID_CUT },
        { DVIDM_COPY,       OLECMDID_COPY },
        { DVIDM_PASTE,      OLECMDID_PASTE },
        { DVIDM_REFRESH,          OLECMDID_REFRESH },
        { DVIDM_STOPDOWNLOAD,     OLECMDID_STOP },
        // subset - above this line document handles
        { DVIDM_OPEN,       OLECMDID_OPEN },
        { DVIDM_SAVE,       OLECMDID_SAVE },
        { DVIDM_SHOWTOOLS,  OLECMDID_HIDETOOLBARS },
    };
#define CCMD_MAX        (sizeof(s_aicmd)/sizeof(s_aicmd[0]))

    UINT iFrom = fToMsoCmd ? 0 : 1;

    for (UINT i = 0; i < CCMD_MAX; i++) {
        if (s_aicmd[i][iFrom]==id) {
            return s_aicmd[i][1-iFrom];
        }
    }
    return (UINT)-1;
#undef CCMD_MAX
}

void CDocObjectHost::_InitToolbarButtons()
{
    OLECMD acmd[] = {
        { OLECMDID_ZOOM,  0 },  // Notes: This must be the first one
        { OLECMDID_PRINT, 0 },
        { OLECMDID_CUT,   0 },
        { OLECMDID_COPY,  0 },
        { OLECMDID_PASTE, 0 },
        { OLECMDID_REFRESH, 0 },
        { OLECMDID_STOP,  0 },  // Notes: This must be the last one
    };

    if (_pmsot) {
        _pmsot->QueryStatus(NULL, ARRAYSIZE(acmd), acmd, NULL);
    }
    if (_pmsoctBrowser) {
        // the browser may support stop also, so override the document
        // with what the browser says. this is okay because the browser
        // forwards stop back down the chain.
        _pmsoctBrowser->QueryStatus(NULL, 1, &acmd[ARRAYSIZE(acmd)-1], NULL);
    }

    if (_psb)
    {
        for (int i=1; i<ARRAYSIZE(acmd); i++)
        {
            UINT idCmd = _MapFromMso(acmd[i].cmdID);
            _psb->SendControlMsg(FCW_TOOLBAR, TB_ENABLEBUTTON, idCmd,
                        (LPARAM)acmd[i].cmdf, NULL);
        }
    }

    // Check if ZOOM command is supported.
    if (acmd[0].cmdf) {

        VARIANTARG var;
        VariantInit(&var);
        var.vt = VT_I4;
        var.lVal = 0;

        // get the current zoom depth
        _pmsot->Exec(NULL, OLECMDID_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER, NULL, &var);
        if (var.vt == VT_I4) {
            _iZoom = var.lVal;
        } else {
            VariantClear(&var);
        }

        // get the current zoom range
        var.vt = VT_I4;
        var.lVal = 0;
        _pmsot->Exec(NULL, OLECMDID_GETZOOMRANGE, OLECMDEXECOPT_DONTPROMPTUSER, NULL, &var);
        if (var.vt == VT_I4) {
            _iZoomMin = (int)(short)LOWORD(var.lVal);
            _iZoomMax = (int)(short)HIWORD(var.lVal);
        } else {
            VariantClear(&var);
        }
    }
}

void CDocObjectHost::_OnSetStatusText(VARIANTARG* pvarIn)
{
    if (pvarIn->vt == (VT_BSTR) && _psb) {
        IShellView *psvActive;

        _psb->QueryActiveShellView(&psvActive);
        if (psvActive)
        {
            //  Suppress sending status messages if we aren't the active view - else
            //  we could be reporting nasties from unapproved PICS pages
            if (IsSameObject(_psv, psvActive))
            {
                LPCOLESTR pwch = (LPCOLESTR)pvarIn->bstrVal;
                TCHAR szHint[256];

                szHint[0]= TEXT('\0');
                if (pwch) {
                    OleStrToStrN(szHint, ARRAYSIZE(szHint), pwch, (UINT)-1);
                }
                _SetStatusText(szHint);
            }
            psvActive->Release();
        }
    }
}

//
// This function returns TRUE if
//  (1) the DocObject supports IPersistFile and
//  (2) IPersistFile::IsDirty returns S_OK.
// Caller may pass pppf to retrieve IPersistFile*, which will be AddRef'ed
// and returned only when this function returns TRUE.
//
BOOL CDocObjectHost::_IsDirty(IPersistFile** pppf)
{
    BOOL fDirty = FALSE;    // Assume non-dirty
    if (pppf) {
        *pppf = NULL;
    }

    if (_pole) {
        IPersistFile* ppf;
        HRESULT hresT = _pole->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if (SUCCEEDED(hresT))
        {
            if (ppf->IsDirty()==S_OK) {
                fDirty = TRUE;
                if (pppf) {
                    *pppf = ppf;
                    ppf->AddRef();
                }
            }
            ppf->Release();
        }
    }
    return fDirty;
}

void CDocObjectHost::_OnSetTitle(VARIANTARG *pvTitle)
{
    if (EVAL(pvTitle->vt == VT_BSTR)) {
        LPCOLESTR pwch = (LPCOLESTR)pvTitle->bstrVal;

        if (pwch) {
            if (_pwb)
                _pwb->SetTitle(_psv, pwch);
        }
    }

    // tell our parent DocObjectView about this as well
    if (_pdvs)
        _pdvs->OnSetTitle(pvTitle);

}


void CDocObjectHost::_OnCodePageChange(const VARIANTARG* pvarargIn)
{
    if (pvarargIn && pvarargIn->vt == VT_I4) {
        TraceMsg(DM_DOCCP, "CDOH::OnExec SHDVID_ONCOEPAGECHANGE got %d", pvarargIn->lVal);
        VARIANT var = *pvarargIn;

        //
        // Since the UI (View->Fond) does not say "default codepage",
        // we don't need to be smart about it.
        //
        // if ((UINT)var.lVal == GetACP()) {
        //     var.lVal = CP_ACP;
        // }

        //
        // Change the 'current' codepage.
        //
        IBrowserService *pbs;
        if(SUCCEEDED(QueryService(SID_STopLevelBrowser, IID_IBrowserService, (LPVOID *)&pbs)))
        {
            pbs->GetSetCodePage(&var, NULL);
            pbs->Release();
        }

        //
        // Write the codepage to the URL history
        //
        IUniformResourceLocator *   purl = NULL;
        HRESULT hresT = CoCreateInstance(CLSID_InternetShortcut, NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_IUniformResourceLocator, (LPVOID*)&purl);

        if(SUCCEEDED(hresT)) {
            TCHAR szURL[MAX_URL_STRING];
            _GetCurrentPage(szURL, ARRAYSIZE(szURL), TRUE);
            _ValidateURL(szURL, UQF_DEFAULT);

            hresT = purl->SetURL(szURL, 0);
            if (SUCCEEDED(hresT)) {
                IPropertySetStorage *       ppropsetstg;
                hresT = purl->QueryInterface(IID_IPropertySetStorage,
                        (LPVOID *)&ppropsetstg);
                if (SUCCEEDED(hresT)) {
                    IPropertyStorage *          ppropstg;
                    hresT = ppropsetstg->Open(FMTID_InternetSite, STGM_READWRITE, &ppropstg);
                    if (SUCCEEDED(hresT)) {
                        const static PROPSPEC c_aprop[] = {
                            { PRSPEC_PROPID, PID_INTSITE_CODEPAGE},
                        };
                        PROPVARIANT prvar = { 0 };
                        prvar.vt = VT_UI4;
                        prvar.lVal = var.lVal;
                        hresT = ppropstg->WriteMultiple(1, c_aprop, &prvar, 0);
                        TraceMsg(DM_DOCCP, "CDOH::_OnCodePageChange WriteMultile returned %x", hresT);

                        ppropstg->Commit(STGC_DEFAULT);
                        ppropstg->Release();
                    } else {
                        TraceMsg(DM_WARNING, "CDOH::_OnCodePageChange Open failed %x", hresT);
                    }

                    ppropsetstg->Release();
                } else {
                    TraceMsg(DM_WARNING, "CDOH::_OnCodePageChange QI failed %x", hresT);
                }
            } else {
                TraceMsg(DM_WARNING, "CDOH::_OnCodePageChange SetURL failed %x", hresT);
            }
            purl->Release();
        } else {
            TraceMsg(DM_WARNING, "CDOH::_OnCodePageChange CoCreate failed %x", hresT);
        }

    } else {
        ASSERT(0);
    }
}


void CDocObjectHost::_MappedBrowserExec(DWORD nCmdID, DWORD nCmdexecopt)
{
    if (_pmsoctBrowser)
    {
        DWORD nCmdIDCT = _MapToMso(nCmdID);
        ASSERT(nCmdIDCT != -1);     // if this rips, need to add missing case to _MapCommandID

        OLECMD rgcmd = {nCmdIDCT, 0};

        // bugbug - Trident sometimes executes commands that are disabled (cut, paste) so
        // ensure that the command is enabled first
        
        BOOL fEnabled = (S_OK == _pmsoctBrowser->QueryStatus(NULL, 1, &rgcmd, NULL)) &&
                        (rgcmd.cmdf & OLECMDF_ENABLED);

        // APPHACK - 80104 Visio doesn't return OLECMDF_ENABLED, but we need to 
        // be able to execute the command to show the toolbars because they start off hidden. 

        if (!fEnabled && (nCmdID == DVIDM_SHOWTOOLS) && 
            (_GetAppHack() & BROWSERFLAG_ENABLETOOLSBTN))
        {
            fEnabled = TRUE;
        }

        if (fEnabled)
        {
            _pmsoctBrowser->Exec(NULL, nCmdIDCT, nCmdexecopt, NULL, NULL);
        }
    }
}

HRESULT CDocObjectHost::OnExec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (pguidCmdGroup == NULL)
    {
        // _InitToolbarButtons and _OnSetStatusText reference _psb directly
        ASSERT(_psb);
        if (!_psb)
            return E_FAIL;

        switch (nCmdID)
        {
        //
        // The containee has found an http-equiv meta tag; handle it
        // appropriately (client pull, PICS, etc)
        //
        case OLECMDID_HTTPEQUIV:
        case OLECMDID_HTTPEQUIV_DONE:
            if (_pwb) {
                _pwb->OnHttpEquiv(_psv, (nCmdID == OLECMDID_HTTPEQUIV_DONE), pvarargIn, pvarargOut);

                // Always return S_OK so that we don't try other codepath.
            }
            return S_OK;

        case OLECMDID_PREREFRESH:
            _fShowProgressCtl = TRUE;
            _PlaceProgressBar(TRUE);
            _OnSetProgressPos(0, PROGRESS_FINDING);
            if (IsGlobalOffline())
            {
                // This is pointing to a web address and we're offline
                // Ask the user if (s)he wants to go online
                TCHAR szURL[MAX_URL_STRING];
                if (SUCCEEDED(_GetCurrentPage(szURL, ARRAYSIZE(szURL), TRUE)) &&
                    UrlHitsNet(szURL))
                {
                    if (InternetGoOnline(szURL, _hwnd, TRUE) && _psb)
                    {
                        // Tell all browser windows to update their title and status pane
                        SendShellIEBroadcastMessage(WM_WININICHANGE,0,0, 1000);
                    }
                }
            }
            return S_OK;

        case OLECMDID_REFRESH:
            if (_pmsot)
                _pmsot->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            return S_OK;

        case OLECMDID_OPEN:
            _OnOpen();
            return S_OK;

        case OLECMDID_SAVE:
            _OnSave();
            return S_OK;

        case OLECMDID_UPDATECOMMANDS:
            _InitToolbarButtons();
            return E_FAIL; // lie and say we don't do anything to forward the command on

        case OLECMDID_SETPROGRESSMAX:
            ASSERT(pvarargIn->vt == VT_I4);
            TraceMsg(TF_SHDPROGRESS, "DOH::Exec() SETPROGRESSMAX = %d", pvarargIn->lVal );
            if(pvarargIn->lVal)
                _OnSetProgressMax(ADJUSTPROGRESSMAX((DWORD) pvarargIn->lVal));
            return S_OK;

        case OLECMDID_SETPROGRESSPOS:
            ASSERT(pvarargIn->vt == VT_I4);
            TraceMsg(TF_SHDPROGRESS, "DOH::Exec() SETPROGRESSPOS = %d", pvarargIn->lVal );
            if(pvarargIn->lVal)
                _OnSetProgressPos((DWORD) pvarargIn->lVal, PROGRESS_RECEIVING);
            return S_OK;

        case OLECMDID_SETPROGRESSTEXT:
            _OnSetStatusText(pvarargIn);
            return S_OK;

        case OLECMDID_SETTITLE:
            if (!pvarargIn)
                return E_INVALIDARG;

            _OnSetTitle(pvarargIn); // We are guaranteed to get atleast 1 OLECMDID_SETTITLE.
            return S_OK;

        // case OLECMDID_PRINT:
        //   In the up direction, this case is handled by the outermost frame as
        // a request to print from the docobj. It handles it by sending an OLECMDID_PRINT
        // back to the docobj to print. (Or, as in Binder, to all the docobjects.)

        default:
            return OLECMDERR_E_NOTSUPPORTED;
        }
    }
    else if (IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
            case SHDVID_SSLSTATUS:
            {
                // Ask the user if (s)he wants to go online
                TCHAR szURL[MAX_URL_STRING];
                if (SUCCEEDED(_GetCurrentPage(szURL, ARRAYSIZE(szURL), TRUE)))
                {
                    if (_bsc._pszRedirectedURL && *_bsc._pszRedirectedURL)
                        StrCpyN(szURL, _bsc._pszRedirectedURL, ARRAYSIZE(szURL));

                   InternetShowSecurityInfoByURL(szURL, _hwnd);
                }

                break;
            }

            case SHDVID_ZONESTATUS:
            {
                // Load the current url into the properties page
                if (!SHRestricted2W(REST_NoBrowserOptions, NULL, 0))
                {
                    TCHAR szBuf[MAX_URL_STRING];
                    _GetCurrentPage(szBuf, ARRAYSIZE(szBuf));
                    ZoneConfigureW(_hwnd, szBuf);
                }
                return S_OK;
            }

            case SHDVID_QUERYMERGEDHELPMENU:
                if (_hmenuMergedHelp)
                {
                    pvarargOut->vt = VT_INT_PTR;
                    pvarargOut->byref = _hmenuMergedHelp;
                    return S_OK;
                }
                return S_FALSE;

            case SHDVID_QUERYOBJECTSHELPMENU:
                if (_hmenuObjHelp)
                {
                    pvarargOut->vt = VT_INT_PTR;
                    pvarargOut->byref = _hmenuObjHelp;
                    return S_OK;
                }
                return S_FALSE;

            case SHDVID_GETSYSIMAGEINDEX:
                if (_dwAppHack & BROWSERFLAG_MSHTML) {
                    ASSERT(pvarargOut->vt==0);
                    pvarargOut->vt = VT_I4;
                    pvarargOut->lVal = _GetIEHTMLImageIndex();
                    return S_OK;
                }
                return E_FAIL;

            case SHDVID_AMBIENTPROPCHANGE:
                // An ambient property above us has changed, let the docobj know
                if (_pmsoc)
                {
                    ASSERT(pvarargIn->vt == VT_I4);
                    return(_pmsoc->OnAmbientPropertyChange(pvarargIn->lVal));
                }
                return E_FAIL;

            case SHDVID_CANDOCOLORSCHANGE:
                return IUnknown_Exec(_pole, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);

            case SHDVID_ONCOLORSCHANGE:
                // this comes from trident and needs passing back up to our parent ...
                if ( _pmsoctBrowser )
                {
                    return _pmsoctBrowser->Exec( pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut );
                }
                else
                    return E_FAIL;

            case SHDVID_GETOPTIONSHWND:
                if ( _pmsoctBrowser )
                {
                    return _pmsoctBrowser->Exec( pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut );
                }
                else
                {
                    return E_FAIL;
                }

            case SHDVID_DOCWRITEABORT:
                //  pending DocObject wants to us to abort any binding and activate
                //  it directly
                if (_bsc._pib && _bsc._fBinding && _punkPending && !_pole)
                {
                    _bsc._fDocWriteAbort = 1;
                    _bsc.OnObjectAvailable(IID_IUnknown, _punkPending);
                    _bsc.AbortBinding();
                }
                //  report READYSTATE_COMPLETE so that when document.open() falls
                //  back to READYSTATE_INTERACTIVE Trident doesn't get confused...
                //
                //  BUGBUG chrisfra 4/15/97, is this the only way to force TRIDENT
                //  to not lose fact of download complete when document.open()
                //  falls back to READYSTATE_INTERACTIVE.
                //
                //  During the above OnObjectAvailable call, we fire a READYSTATE_COMPLETE
                //  event if (1) object doesn't support it or (2) object already at it.
                //  (Neither of these should be the case, but we should be careful, eh?)
                //  We want to force a READYSTATE_COMPLETE here in other cases, so unhook
                //  the IPropertyNotifySink (to prevent multiple _COMPLETE events). If we
                //  unhook the sink, then we didn't fire _COMPLETE above, so fire it now.
                //
                if (_RemoveTransitionCapability())
                    _OnReadyState(READYSTATE_COMPLETE);
                return S_OK;

            case SHDVID_CANACTIVATENOW:
            {
                HRESULT hres = (_fPicsAccessAllowed && !_fbPicsWaitFlags && _pole && _fReadystateInteractiveProcessed) ? S_OK : S_FALSE;
                TraceMsg(DM_PICS, "CDOH::OnExec(SHDVID_CANACTIVATENOW) returning %s", (hres == S_OK) ? "S_OK" : "S_FALSE");
                return hres;
            }

            case SHDVID_SETSECURELOCK:
                {
                    //
                    //  if we are already active, then we need to go ahead
                    //  and forward this up the browser. otherwise, cache it
                    //  and wait until activated to forward it
                    //
                    TraceMsg(DM_SSL, "[%X]DOH::Exec() SETSECURELOCK lock = %d", this, pvarargIn->lVal);

                    _fSetSecureLock = TRUE;
                    _eSecureLock = pvarargIn->lVal;

                    IShellView *psvActive;
                    if (_psb && SUCCEEDED(_psb->QueryActiveShellView(&psvActive) ))
                    {
                        if (psvActive && IsSameObject(_psv, psvActive))
                            _ForwardSetSecureLock(pvarargIn->lVal);

                        ATOMICRELEASE(psvActive);
                    }
                    return S_OK;
                }

            case SHDVID_ONCODEPAGECHANGE:
                _OnCodePageChange(pvarargIn);
                return S_OK;

            case SHDVID_DISPLAYSCRIPTERRORS:
            case SHDVID_NAVIGATIONSTATUS:
            {
                // if we're a weboc then this script err list should be null
                ASSERT(!_fWebOC || _pScriptErrList == NULL);

                if (_pScriptErrList != NULL && !_pScriptErrList->IsEmpty())
                {
                    // do the script error info dialog
                    _ScriptErr_Dlg(TRUE);
                }

                return S_OK;
            }
            break;

            case SHDVID_RESETSTATUSBAR:
                {
                    _ResetStatusBar();
                    return S_OK;
                }
                break;

            default:
                return OLECMDERR_E_NOTSUPPORTED;
        }
    }
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        switch (nCmdID) {
        case SBCMDID_MAYSAVECHANGES:
            return _OnMaySaveChanges();

        case SBCMDID_GETPANE:
            switch(nCmdexecopt)
            {
                case PANE_NAVIGATION:
                    V_I4(pvarargOut) = STATUS_PANE_NAVIGATION;
                    return S_OK;

                case PANE_PROGRESS:
                    V_I4(pvarargOut) = STATUS_PANE_PROGRESS;
                    return S_OK;

                case PANE_ZONE:
                    V_I4(pvarargOut) = STATUS_PANE_ZONE;
                    return S_OK;

                case PANE_OFFLINE:
                    V_I4(pvarargOut) = STATUS_PANE_OFFLINE;
                    return S_OK;

//                case PANE_PRINTER:
//                    V_I4(pvarargOut) = STATUS_PANE_PRINTER;
//                    return S_OK;

                case PANE_SSL:
                    V_I4(pvarargOut) = STATUS_PANE_SSL;
                    return S_OK;

                default:
                    V_I4(pvarargOut) = PANE_NONE;
                    return S_OK;
            }

        default:
            return OLECMDERR_E_NOTSUPPORTED;
        } // switch
    }
    else if (IsEqualGUID(CGID_DocHostCommandHandler, *pguidCmdGroup))
    {
        switch(nCmdID)
        {
        case OLECMDID_SAVEAS:
            _OnSaveAs();
            return S_OK;

        case OLECMDID_SHOWSCRIPTERROR:
            {
                HRESULT hr;

                hr = S_OK;

                if (_fWebOC)
                {
                    // we're a web oc.
                    // pass the handling of this script error to
                    // an appropriate CDocHostUIHandler

                    if (_pWebOCUIHandler != NULL)
                    {
                        IOleCommandTarget * pioct;

                        ASSERT(IS_VALID_CODE_PTR(_pWebOCUIHandler, IDocHostUIHandler));

                        hr = _pWebOCUIHandler->QueryInterface(IID_IOleCommandTarget, (LPVOID *) &pioct);
                        if (SUCCEEDED(hr))
                        {
                            hr = pioct->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);

                            pioct->Release();
                        }
                    }
                    else
                    {
                        hr = _dhUIHandler.Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
                    }
                }
                else
                {
                    ASSERT(IS_VALID_READ_PTR(pvarargIn, VARIANTARG));
                    ASSERT(IS_VALID_WRITE_PTR(pvarargOut, VARIANTARG));

                    // we're not a web oc so we have to handle this
                    // ourselves, so cache the errors for later
                    // display in the new script error dialog

                    if (pvarargIn == NULL || pvarargOut == NULL)
                    {
                        hr = E_INVALIDARG;
                    }

                    if (SUCCEEDED(hr))
                    {
                        if (_pScriptErrList == NULL)
                        {

                            // create a new script error list
                            _pScriptErrList = new CScriptErrorList;
                            if (_pScriptErrList == NULL)
                            {
                                hr = E_OUTOFMEMORY;
                            }
                        }

                        if (SUCCEEDED(hr))
                        {
                            TCHAR   szMsg[MAX_PATH];

                            // stuff the error icon into the status bar
                            if (g_hiconScriptErr != NULL)
                            {
                                if (_psb != NULL)
                                {
                                    _psb->SendControlMsg(FCW_STATUS,
                                                         SB_SETICON,
                                                         STATUS_PANE_NAVIGATION,
                                                         (LPARAM)g_hiconScriptErr,
                                                         NULL);
                                }
                            }

                            // stuff the error text into the status bar
                            MLLoadString(IDS_SCRIPT_ERROR_ON_PAGE, szMsg, ARRAYSIZE(szMsg));
                            _SetPriorityStatusText(szMsg);

                            // stuff the error data into the cache
                            _ScriptErr_CacheInfo(pvarargIn);

                            // pop up the dialog
                            _ScriptErr_Dlg(FALSE);

                            V_VT(pvarargOut) = VT_BOOL;
                            if (_pScriptErrList->IsFull())
                            {
                                // stop running scripts
                                V_BOOL(pvarargOut) = VARIANT_FALSE;
                            }
                            else
                            {
                                // keep running scripts
                                V_BOOL(pvarargOut) = VARIANT_TRUE;
                            }
                        }
                    }
                }

                return hr;
            }
            break;

        case OLECMDID_SHOWMESSAGE:
        case OLECMDID_SHOWFIND:
        case OLECMDID_SHOWPAGESETUP:
        case OLECMDID_SHOWPRINT:
        case OLECMDID_PROPERTIES:
            {
                return _dhUIHandler.Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            }
            break;

        //
        // Refresh the original page if an error page is dispalyed.
        //

        case IDM_REFRESH:
        case IDM_REFRESH_TOP:
        case IDM_REFRESH_TOP_FULL:
        case IDM_REFRESH_THIS:
        case IDM_REFRESH_THIS_FULL:
        {
            HRESULT hr = OLECMDERR_E_NOTSUPPORTED;

            if (_pScriptErrList != NULL)
            {
                // clear out the script error list
                _pScriptErrList->ClearErrorList();
                _SetPriorityStatusText(NULL);

                // reset the text and icon
                _ResetStatusBar();
            }

            //
            // If there is a refresh url for this object use it for the refresh.
            // Otherwise fall through and let the client handle it.
            //

            if (_pwszRefreshUrl)
            {
                _DoAsyncNavigation(_pwszRefreshUrl);
                hr = S_OK;
            }
            else
            {
                //
                // Non http errors (syntax, DNS, etc) are handled by a async nav
                // to res://shdocvw/error.htm#originalurl.  Handle the refresh
                // for those pages here.
                //
                if (_pmkCur)
                {
                    LPOLESTR pstrUrl;

                    if (SUCCEEDED(_pmkCur->GetDisplayName(_pbcCur, NULL, &pstrUrl)))
                    {

                        if (IsErrorUrl(pstrUrl) && _pszLocation && *_pszLocation)
                        {
                            //
                            // The error url has the form:
                            // "res://shdocvw.dll/http404.htm#http://foo.bar"
                            // Where foo.bar is the the url the user tried to navigate to.
                            // _pszLocation points to "#foo.bar"
                            DWORD dwScheme = GetUrlScheme(_pszLocation + 1);
                            BOOL fDoNavigation = ((URL_SCHEME_HTTP == dwScheme) ||
                               (URL_SCHEME_HTTPS == dwScheme) ||
                               (URL_SCHEME_FTP == dwScheme) ||
                               (URL_SCHEME_GOPHER == dwScheme));

                            //
                            if(fDoNavigation) // otherwise it's a security problem !
                            {
                                _DoAsyncNavigation(_pszLocation + 1);
                            }
                            hr = S_OK;
                        }

                        OleFree(pstrUrl);
                    }
                }
            }

            return hr;
            break;
        }
        default:
            return OLECMDERR_E_NOTSUPPORTED;
        }
    }
    else if (IsEqualGUID(*pguidCmdGroup, CLSID_InternetButtons) ||
             IsEqualGUID(*pguidCmdGroup, CLSID_MSOButtons))
    {
        UEMFireEvent(&UEMIID_BROWSER, UEME_UITOOLBAR, UEMF_XEVENT, UIG_OTHER, nCmdID);
        if (nCmdexecopt == OLECMDEXECOPT_PROMPTUSER) {
            // the user hit the drop down
            if (_pmsoctBrowser && pvarargIn && pvarargIn->vt == VT_INT_PTR)
            {
                // v.vt = VT_INT_PTR;
                POINT pt;
                RECT* prc = (RECT*)pvarargIn->byref;
                pt.x = prc->left;
                pt.y = prc->bottom;

                switch (nCmdID)
                {
                case DVIDM_MAILNEWS:
                    {
                        VARIANTARG v = {VT_I4};
                        v.lVal = MAKELONG(prc->left, prc->bottom);
                        _pmsoctBrowser->Exec(&CGID_Explorer, SBCMDID_DOMAILMENU, 0, &v, NULL);
                        break;
                    }

                case DVIDM_FONTS:
                    {
                        VARIANTARG v = {VT_I4};
                        v.lVal = MAKELONG(prc->left, prc->bottom);
                        _pmsoctBrowser->Exec(&CGID_ShellDocView, SHDVID_FONTMENUOPEN, 0, &v, NULL);
                        break;
                    }

                case DVIDM_ENCODING:
                    {
                        VARIANTARG v = {VT_I4};
                        v.lVal = MAKELONG(prc->left, prc->bottom);
                        _pmsoctBrowser->Exec(&CGID_ShellDocView, SHDVID_MIMECSETMENUOPEN, 0, &v, NULL);
                        break;
                    }
                }
            }
            return S_OK;
        }

        // CommandIDs from DVIDM_MENUEXT_FIRST to DVIDM_MENUEXT_LAST are reserved for toolbar extension buttons
        // Do NOT use this range for constants within the scope of CLSID_InternetButtons/CLSID_MSOButtons!
        if (InRange(nCmdID, DVIDM_MENUEXT_FIRST, DVIDM_MENUEXT_LAST))
        {
            IUnknown_Exec(_pBrowsExt, &CLSID_ToolbarExtButtons, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
        }
        else
        {
            switch(nCmdID) {

            case DVIDM_DISCUSSIONS:
                if (_pmsoctBrowser)
                    _pmsoctBrowser->Exec(&CGID_Explorer, SBCMDID_DISCUSSIONBAND, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
                break;

            case DVIDM_CUT:
            case DVIDM_COPY:
            case DVIDM_PASTE:
                _MappedBrowserExec(nCmdID, 0);
                break;

            case DVIDM_PRINT:
            case DVIDM_SHOWTOOLS:
                _MappedBrowserExec(nCmdID, OLECMDEXECOPT_DONTPROMPTUSER);
                break;

            case DVIDM_EDITPAGE:
                if (_psp) {
                    // BUGBUG: temp code -- forward to itbar
                    // itbar edit code is moving here soon
                    IExplorerToolbar* pxtb;
                    if (SUCCEEDED(_psp->QueryService(SID_SExplorerToolbar, IID_IExplorerToolbar, (LPVOID*)&pxtb))) {
                        IUnknown_Exec(pxtb, &CGID_PrivCITCommands, CITIDM_EDITPAGE, nCmdexecopt, pvarargIn, pvarargOut);
                        pxtb->Release();
                    }
                }
                break;
            }
        }
        return S_OK;
    }
    else if (IsEqualGUID(IID_IExplorerToolbar, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case ETCMDID_GETBUTTONS:
        {
            int nNumExtButtons = 0;

            if (_pBrowsExt)
            {
                _pBrowsExt->GetNumButtons((UINT*)&nNumExtButtons);
            }

            int nNumButtons = nNumExtButtons + ARRAYSIZE(c_tbStd);

            if ((_nNumButtons != nNumButtons) && (_ptbStd != NULL))
            {
                delete [] _ptbStd;
                _ptbStd = NULL;
            }

            if (_ptbStd == NULL)
            {
                _ptbStd = new TBBUTTON[nNumButtons];
                if (_ptbStd == NULL)
                {
                    return E_OUTOFMEMORY;
                }
                _nNumButtons = nNumButtons;
            }

            memcpy(_ptbStd, c_tbStd, SIZEOF(TBBUTTON) * ARRAYSIZE(c_tbStd));

            // Init the string ids
            ASSERT(_ptbStd[6].idCommand == DVIDM_CUT);
            ASSERT(_ptbStd[7].idCommand == DVIDM_COPY);
            ASSERT(_ptbStd[8].idCommand == DVIDM_PASTE);
            ASSERT(_ptbStd[9].idCommand == DVIDM_ENCODING);

            if (-1 != _iString)
            {
                _ptbStd[6].iString = _iString;
                _ptbStd[7].iString = _iString + 1;
                _ptbStd[8].iString = _iString + 2;
                _ptbStd[9].iString = _iString + 3;
            }
            else
            {
                _ptbStd[6].iString = _ptbStd[7].iString = _ptbStd[8].iString = _ptbStd[9].iString = -1;
            }

            if (_pBrowsExt)
            {
                _pBrowsExt->GetButtons(&_ptbStd[ARRAYSIZE(c_tbStd)], nNumExtButtons, FALSE);
            }

            ASSERT(_ptbStd[0].idCommand == DVIDM_SHOWTOOLS);
            if (!_ToolsButtonAvailable())
                _ptbStd[0].fsState |= TBSTATE_HIDDEN;

            ASSERT(_ptbStd[1].idCommand == DVIDM_MAILNEWS);
            if (!_MailButtonAvailable())
                _ptbStd[1].fsState |= TBSTATE_HIDDEN;

            ASSERT(_ptbStd[5].idCommand == DVIDM_DISCUSSIONS);
            if (!_DiscussionsButtonAvailable())
                _ptbStd[5].fsState |= TBSTATE_HIDDEN;

            nNumButtons = RemoveHiddenButtons(_ptbStd, nNumButtons);

            pvarargOut->vt = VT_BYREF;
            pvarargOut->byref = (LPVOID)_ptbStd;
            *pvarargIn->plVal = nNumButtons;
            break;
        }
        case ETCMDID_RELOADBUTTONS:
            _AddButtons(TRUE);
            break;
        }
        return S_OK;
    }

    return OLECMDERR_E_UNKNOWNGROUP;
}


HRESULT CDocObjectHost::_OnMaySaveChanges(void)
{
   HRESULT hres = S_OK;

    //
    // ASSUMPTIONS:
    //  1. Not supporting IPersistFile indicates we don't need to worry
    //   about prompting the user for "save as".
    //  2. DocObject which returns S_OK for IPersistFile::S_OK implements
    //   OLECMDID_SAVEAS.
    //
    if (_fFileProtocol || _pmsot)
    {
        IPersistFile* ppf;
        if (_IsDirty(&ppf))
        {
            ASSERT(ppf);

            TCHAR szBuf[MAX_URL_STRING];
            UINT id;

            _GetCurrentPage(szBuf, ARRAYSIZE(szBuf));
            id = MLShellMessageBox(_hwnd,
                MAKEINTRESOURCE(IDS_MAYSAVEDOCUMENT), szBuf, MB_YESNOCANCEL);
            switch(id) {
            case IDCANCEL:
                hres = S_FALSE;
                break;

            case IDYES:
                if (_fFileProtocol) {
                    // 80105 APPHACK: Due to valid fixes in Urlmon, Visio is unable to save
                    // because we are loading the object with read-only flags.  So we show
                    // the Save As dialog to let the user choose another filename.

                    if (_GetAppHack() & BROWSERFLAG_SAVEASWHENCLOSING)
                    {
                        if (_OnSaveAs() != S_OK)
                            hres = S_FALSE;
                    }
                    else
                        _OnSave();

                } else {
                    HRESULT hresT=_pmsot->Exec(NULL, OLECMDID_SAVEAS, OLECMDEXECOPT_PROMPTUSER, NULL, NULL);
                    SAVEMSG("Exec(OLECMDID_SAVEAS) returned", hresT);

                    // Cancel the navigation if it failed.
                    if (FAILED(hresT)) {
                        // Beep if it is not canceled by the end user.
                        TraceMsg(DM_WARNING, "CDOH::_OnMaySaveChanges Exec(OELCMDID_SAVEAS) returned %x", hresT);
                        if (hresT != OLECMDERR_E_CANCELED) {
                            MessageBeep(0);
                        }
                        hres = S_FALSE;
                    }
                }

                break;

            case IDNO:
                //
                //  If user says 'No' to save changes to this page,
                // we should remove it from the cache so that
                // the user won't see that discarded change.
                //
                //  BUGBUG: (pri-2) This object discarding mechanism
                // does not work for POSTed result, which is cached
                // in the travel log.
                //
                break;
            }

            ppf->Release();
        } else {
            ASSERT(ppf==NULL);
        }
    }

    //
    //  In addition, we give a chance to save the contents of the page (when
    // the document is acted as a form -- data-bound Trident page is a good
    // example) to the backend database.
    //
    
    if (hres==S_OK && _pmsot) {
        VARIANT varOut = {0};
        HRESULT hresT = _pmsot->Exec(NULL, OLECMDID_ONUNLOAD, OLECMDEXECOPT_PROMPTUSER, NULL, &varOut);
        if (varOut.vt == VT_BOOL && varOut.boolVal != VARIANT_TRUE) {
            hres = S_FALSE;
        }
    }

    return hres;
}

BOOL _ExecNearest(const GUID *pguidCmdGroup, DWORD nCmdID, BOOL fDown)
{
    // Some commands we want to do in the closest frame to the docobj,
    // some in the farthest-away frame, and some we want to handle
    // in the top-most dochost. Look at the command to figure out
    // the routing and then do it.
    BOOL fNearest = FALSE; // most everything goes to the farthest-away frame
    if (pguidCmdGroup==NULL)
    {
        switch(nCmdID)
        {
        case OLECMDID_OPEN:
        case OLECMDID_SAVE:
        case OLECMDID_SETTITLE:
        case OLECMDID_HTTPEQUIV:
        case OLECMDID_HTTPEQUIV_DONE:
            fNearest = TRUE;
            break;

        // some are top-most down, so nearest depends on direction.
        case OLECMDID_REFRESH:
        // say top-most for commands that only work on the topmost guy.
        // (ie, these probably should be implemented in CShellBrowser!)
        // do this even though these are really "upwards-only" commands.
        case OLECMDID_UPDATECOMMANDS:
        case OLECMDID_SETPROGRESSMAX:
        case OLECMDID_SETPROGRESSPOS:
        case OLECMDID_SETPROGRESSTEXT:
        case OLECMDID_SHOWSCRIPTERROR:
            fNearest = fDown;
            break;
        }
    }
    else if (IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case SHDVID_AMBIENTPROPCHANGE:
        case SHDVID_GETSYSIMAGEINDEX:
        case SHDVID_DOCWRITEABORT:
        case SHDVID_ONCODEPAGECHANGE:
        case SHDVID_CANDOCOLORSCHANGE:
        case SHDVID_SETSECURELOCK:
        case SHDVID_QUERYMERGEDHELPMENU:
        case SHDVID_QUERYOBJECTSHELPMENU:
            fNearest = TRUE;
            break;

        case SHDVID_DISPLAYSCRIPTERRORS:
            fNearest = fDown;
            break;
        }
    }
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        switch(nCmdID)
        {
        case SBCMDID_MAYSAVECHANGES:    // since OLECMDID_SAVE is to the nearest frame
            fNearest = TRUE;
            break;
        }
    }
    else if (IsEqualGUID(IID_IExplorerToolbar, *pguidCmdGroup) ||
             IsEqualGUID(CLSID_InternetButtons, *pguidCmdGroup) ||
             IsEqualGUID(CLSID_MSOButtons, *pguidCmdGroup))
    {
        fNearest = TRUE;
    }

    return fNearest;
}

HRESULT CDocObjectHost::Exec(
    /* [unique][in] */ const GUID *pguidCmdGroup,
    /* [in] */ DWORD nCmdID,
    /* [in] */ DWORD nCmdexecopt,
    /* [unique][in] */ VARIANTARG *pvarargIn,
    /* [unique][out][in] */ VARIANTARG *pvarargOut)
{
    HRESULT hres = OLECMDERR_E_UNKNOWNGROUP;

    // Now that BaseBrowser understands that CGID_MSHTML should be directed to the DocObject, we'll
    // get caught in a loop if we send those Execs through here.  Cut it off at the pass.
    if (pguidCmdGroup && IsEqualGUID(CGID_MSHTML, *pguidCmdGroup))
        return hres;

    BOOL fNearest = _ExecNearest(pguidCmdGroup, nCmdID, FALSE);

    if (fNearest)
        hres = OnExec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);

    if (FAILED(hres) && _pmsoctBrowser)
        hres = _pmsoctBrowser->Exec(pguidCmdGroup, nCmdID, nCmdexecopt,
                                    pvarargIn, pvarargOut);

    // BUGBUG: if this is a command that puts up UI and the user presses
    // cancel in the above call, we may try to handle the call here,
    // and that would be bad. Steal OleCmdHRHandled() from MSHTML.
    if (FAILED(hres) && !fNearest)
        hres = OnExec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);

    return hres;
}

HRESULT CDocObjectHost::_GetUrlVariant(VARIANT *pvarargOut)
{
    ASSERT( pvarargOut);
    if (_pmkCur)
    {
        LPOLESTR pszDisplayName = NULL;
        LPTSTR pszRedirectedURL = NULL;

        if (_bsc._pszRedirectedURL && *_bsc._pszRedirectedURL)
            pszRedirectedURL = _bsc._pszRedirectedURL;

        if (pszRedirectedURL || SUCCEEDED(_GetCurrentPageW(&pszDisplayName, TRUE)))
        {
            pvarargOut->bstrVal = SysAllocString(pszRedirectedURL ? pszRedirectedURL : pszDisplayName);
            if (pvarargOut->bstrVal)
                pvarargOut->vt = VT_BSTR;
            if (pszDisplayName)
                OleFree(pszDisplayName);
        }
    }
    return (pvarargOut->bstrVal == NULL) ? E_FAIL : S_OK;
}

HRESULT CDocObjectHost::_CoCreateHTMLDocument(REFIID riid, LPVOID* ppvOut)
{
    IOleCommandTarget* pcmd;
    HRESULT hres = QueryService(SID_STopLevelBrowser, IID_IOleCommandTarget, (void **)&pcmd);
    if (SUCCEEDED(hres)) {
        VARIANT varOut = { 0 };
        hres = pcmd->Exec(&CGID_Explorer, SBCMDID_COCREATEDOCUMENT, 0, NULL, &varOut);
        if (SUCCEEDED(hres) && varOut.vt == VT_UNKNOWN) {
            hres = varOut.punkVal->QueryInterface(riid, ppvOut);
            // Clean it up by ourself so that we don't load OLEAUT32
            varOut.punkVal->Release();
        } else {
            ASSERT(varOut.vt == VT_EMPTY);
            VariantClear(&varOut);
        }
        pcmd->Release();
    }
    return hres;
}

HRESULT CDocObjectHost::_CreatePendingDocObject(BOOL fMustInit)
{
    HRESULT hres = S_OK;

    if (_punkPending == NULL)
    {
        hres = _CoCreateHTMLDocument(IID_IUnknown, (LPVOID*)&_punkPending);
        _fPendingNeedsInit = 1;   // lazy InitNew only if absolutely necessary
    }
    if (_fPendingNeedsInit && fMustInit && SUCCEEDED(hres))
    {
        IPersistStreamInit *pipsi;
        IOleObject *polePending;
#ifdef TRIDENT_NEEDS_LOCKRUNNING
        IRunnableObject *pro;
#endif
        _fCreatingPending = 1;    // we are creating _punkPending
        _fAbortCreatePending = 0;
        _fPendingNeedsInit = 0;
        hres = _punkPending->QueryInterface(IID_IPersistStreamInit, (LPVOID *) &pipsi);
        if (SUCCEEDED(hres))
        {
            hres = pipsi->InitNew();
            pipsi->Release();
        }
        // if the InitNew is a re-entrant request (such as doing execDown to get a securityctx
        //  while in the process of loading the document), trident will respond with E_PENDING
        //  since there is already a load in progress, this call/init is a timing issue, and
        //  we can use the exisitng one.
        if (SUCCEEDED(hres) || hres==E_PENDING)
        {
            hres = _punkPending->QueryInterface(IID_IOleObject, (LPVOID *) &polePending);
            if (SUCCEEDED(hres))
            {
                hres = polePending->SetClientSite(this);
                polePending->Release();
            }
#ifdef TRIDENT_NEEDS_LOCKRUNNING
        //  TRIDENT NO LONGER SUPPORTS IRunnableObject
            //  RegisterObjectParam/RevokeObjectParam calls LockRunning on object being
            //  registered.  LockRunning(FALSE,FALSE) implied in the Revoke will result
            //  in OleClose being called on _punkPending if we haven't activated it
            //  by end of binding.  Thus we must call LockRunning ourself
            if (SUCCEEDED(hres))
            {
                hres = _punkPending->QueryInterface(IID_IRunnableObject, (LPVOID *) &pro);
                if (SUCCEEDED(hres))
                {
                    hres = pro->LockRunning(TRUE, TRUE);
                    pro->Release();
                }
            }
#endif
        }
        _fCreatingPending = 0;
        _fPendingWasInited = 1;
        if (FAILED(hres))
        {
            SAFERELEASE(_punkPending);
        }
        else if (_fAbortCreatePending)
        {
            //  Detect AOL pumping messages and reentering and attempting to release
            //  _punkPending
            _fAbortCreatePending = 0;
            _ReleasePendingObject();
            hres = E_FAIL;
        }
        else
        {
            //  Pass URL for pending object to it in advance of IPersistMoniker::Load

            //
            // Notes: We don't want to call _GetUrlVariant which will load
            // OLEAUT32.DLL
            //

            LPOLESTR pszDisplayName = NULL;
            LPTSTR pszURL = NULL;

            if (_bsc._pszRedirectedURL && *_bsc._pszRedirectedURL)
                pszURL = _bsc._pszRedirectedURL;

            if (pszURL || SUCCEEDED(_GetCurrentPageW(&pszDisplayName, TRUE))) {
                SA_BSTR sbstr; // stack allocated BSTR
                StrCpyNW(sbstr.wsz, pszURL ? pszURL : pszDisplayName, ARRAYSIZE(sbstr.wsz));
                sbstr.cb = lstrlenW(sbstr.wsz) * SIZEOF(WCHAR);

                VARIANT varIn;
                varIn.vt = VT_BSTR;
                varIn.bstrVal = sbstr.wsz;

                IUnknown_Exec(_punkPending, &CGID_ShellDocView, SHDVID_SETPENDINGURL, 0, &varIn, NULL);

                if (pszDisplayName)
                    OleFree(pszDisplayName);
            }
        }
        _fAbortCreatePending = 0;
    }
    return hres;
}

// called from CDocObjectView to exec and forward these calls down
//
HRESULT CDocObjectHost::ExecDown(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hres = OLECMDERR_E_UNKNOWNGROUP;

    //  Special case Exec's that are used to fetch info on pending docobject
    //  for scripting access before OnObjectAvailable
    if (pguidCmdGroup && IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup))
    {
        switch(nCmdID)
        {
        case SHDVID_GETPENDINGOBJECT:
            ASSERT( pvarargOut);
            VariantClearLazy(pvarargOut);
            if (_pole)
            {
                _pole->QueryInterface(IID_IUnknown, (LPVOID *) &(pvarargOut->punkVal));
            }
            else
            {
                _CreatePendingDocObject(TRUE);
                if (_punkPending)
                {
                    pvarargOut->punkVal = _punkPending;
                    _punkPending->AddRef();
                }
                else if (_pole)
                {
                    _pole->QueryInterface(IID_IUnknown, (LPVOID *) &(pvarargOut->punkVal));
                }
            }
            if (pvarargOut->punkVal != NULL)
            {
                pvarargOut->vt = VT_UNKNOWN;
                hres = S_OK;

            }
            else
            {
                hres = E_FAIL;
            }
            return hres;
            break;

        case SHDVID_GETPENDINGURL:
            ASSERT( pvarargOut);
            VariantClearLazy(pvarargOut);
            hres = _GetUrlVariant(pvarargOut);
            return hres;
            break;
        }
    }

    BOOL fNearest = _ExecNearest(pguidCmdGroup, nCmdID, TRUE);

    if (fNearest)
        hres = OnExec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);

    if (FAILED(hres) && _pmsot) {
        hres = _pmsot->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);

        //
        // APPHACK:
        //  PPT in Office 97 fails to print if we pass PRINTFLAG_PROMPTUSER
        // and returns E_INVALIDARG. If we detect this case, we should retry
        // without this flag. PPT will popup the print dialog. (SatoNa)
        //
        if (hres == E_INVALIDARG
            && (_dwAppHack & BROWSERFLAG_PRINTPROMPTUI)
            && pguidCmdGroup == NULL
            && nCmdID == OLECMDID_PRINT)
        {
            TraceMsg(TF_SHDAPPHACK, "DOH::ExecDown(OLECMDID_PRINT) removing PRINTFLAG_PROMPTUSER");
            nCmdexecopt &= ~OLECMDEXECOPT_DONTPROMPTUSER;
        hres = _pmsot->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
        }
    }

    if (FAILED(hres) && !fNearest)
        hres = OnExec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);

    return hres;
}
HRESULT CDocObjectHost::QueryStatusDown(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    HRESULT hres;

    if (_pmsot)
        hres = _pmsot->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
    else if (pguidCmdGroup && IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup))
        hres = IUnknown_QueryStatus(_pole, pguidCmdGroup, cCmds, rgCmds, pcmdtext);

    return OnQueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext, hres);
}


HRESULT CDocObjectHost::Invoke(DISPID dispidMember, REFIID iid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pdispparams,
                        VARIANT FAR* pVarResult,EXCEPINFO FAR* pexcepinfo,UINT FAR* puArgErr)
{
    if (!_peds)
        return(E_NOTIMPL);

    return _peds->OnInvoke(dispidMember, iid, lcid, wFlags, pdispparams, pVarResult,pexcepinfo,puArgErr);
}

//*** IOleControlSite {

HRESULT CDocObjectHost::OnControlInfoChanged()
{
    HRESULT hres = E_NOTIMPL;

    if (_pedsHelper)
    {
        hres = _pedsHelper->OnOnControlInfoChanged();
    }

    return(hres);
}

//***   CDOH::TranslateAccelerator (IOCS::TranslateAccelerator)
// NOTES
//  trident (or any other DO that uses IOCS::TA) calls us back when TABing
//  off the last link.  to handle it, we flag it for our original caller
//  (IOIPAO::TA), and then pretend we handled it by telling trident S_OK.
//  trident returns S_OK to IOIPAO::TA, which checks the flag and says
//  'trident did *not* handle it' by returning S_FALSE.  that propagates
//  way up to the top where it sees it was a TAB so it does a CycleFocus.
//
//  that's how we do it when we're top-level.  when we're a frameset, we
//  need to do it the 'real' way, sending it up to our parent IOCS.
HRESULT CDocObjectHost::TranslateAccelerator(MSG __RPC_FAR *pmsg,DWORD grfModifiers)
{

    HRESULT hres = S_FALSE;

    if (_peds) {
        // try it the real way in case we're in a frameset
        // top level: we'll do CImpIExpDispSupport::OnTA which does E_NOTIMPL,
        // frameset:  we'll do CWebBrowserOC::OnTA which talks to trident
        // BUGBUG what if trident (or OC?) gives back E_NOTIMPL too?
        TraceMsg(DM_FOCUS, "DOH::IOCS::TA peds!=NULL forward");
        hres = _peds->OnTranslateAccelerator(pmsg, grfModifiers);
    }
    if (hres != S_OK) {
        // we're at top level (E_NOTIMPL), so we can fake it
        // (or alternately we're not, but our parent said S_FALSE)
#ifdef DEBUG
        if (_peds && SUCCEEDED(hres)) {
            // i'm curious if we ever hit this
            TraceMsg(DM_WARNING, "DOH::IOCS::TA parent hres=%x (!=S_OK)", hres);
        }
#endif
        hres = S_FALSE;
        if (IsVK_TABCycler(pmsg)) {
            TraceMsg(TF_SHDUIACTIVATE, "DOH::TranslateAccelerator called with VK_TAB");
            TraceMsg(DM_FOCUS, "DOH::IOCS::TA(wParam=VK_TAB) ret _fCycleFocus=TRUE hr=S_OK (lie)");
            // defer it, set flag for CDOH::IOIPAO::TA, and pretend we handled it
            ASSERT(!_fCycleFocus);
            _fCycleFocus = TRUE;
            hres = S_OK;
        }
    }

    return hres;
}

// }


STDMETHODIMP CDocObjectHost::CPicsCommandTarget::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IOleCommandTarget) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IOleCommandTarget *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _ctPics, this);

    return pdoh->AddRef();
}

STDMETHODIMP_(ULONG) CDocObjectHost::CPicsCommandTarget::AddRef(void)
{
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _ctPics, this);

    return pdoh->AddRef();
}

STDMETHODIMP_(ULONG) CDocObjectHost::CPicsCommandTarget::Release(void)
{
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _ctPics, this);

    return pdoh->Release();
}

STDMETHODIMP CDocObjectHost::CPicsCommandTarget::QueryStatus(const GUID *pguidCmdGroup,
            ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDocObjectHost::CPicsCommandTarget::Exec(const GUID *pguidCmdGroup,
            DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _ctPics, this);

    if (IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case SHDVID_PICSLABELFOUND:
            if (pvarargIn->vt == (VT_BSTR)) {
                pdoh->_HandleInDocumentLabel(pvarargIn->bstrVal);
            }
            return NOERROR;

        case SHDVID_NOMOREPICSLABELS:
            pdoh->_HandleDocumentEnd();
            return NOERROR;

        default:
            return OLECMDERR_E_NOTSUPPORTED;
        }
    }

    return OLECMDERR_E_UNKNOWNGROUP;
}



HRESULT CDocObjectFrame::QueryService(REFGUID guidService,
                                    REFIID riid, void **ppvObj)
{
    return _pdoh->QueryService(guidService, riid, ppvObj);
}

HRESULT CDocObjectFrame::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IOleInPlaceFrame) ||
        IsEqualIID(riid, IID_IOleInPlaceUIWindow) ||
        IsEqualIID(riid, IID_IOleWindow) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IOleInPlaceFrame*);
    }
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
    {
        *ppvObj = SAFECAST(this, IOleCommandTarget*);
    }
    else if (IsEqualIID(riid, IID_IServiceProvider))
    {
        *ppvObj = SAFECAST(this, IServiceProvider*);
    }
    else if (IsEqualIID(riid, IID_IInternetSecurityMgrSite))
    {
        *ppvObj = SAFECAST(this, IInternetSecurityMgrSite*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    _pdoh->AddRef();
    return NOERROR;
}

ULONG CDocObjectFrame::AddRef(void)
{
    return _pdoh->AddRef();
}

ULONG CDocObjectFrame::Release(void)
{
    return _pdoh->Release();
}

HRESULT CDocObjectFrame::GetWindow(HWND * lphwnd)
{
    DOFMSG(TEXT("GetWindow called"));
    return _pdoh->_pipu ?
        _pdoh->_pipu->GetWindow(lphwnd) :
        _pdoh->_psb ? _pdoh->_psb->GetWindow(lphwnd) :
        _pdoh->GetWindow(lphwnd);
}

HRESULT CDocObjectFrame::ContextSensitiveHelp(BOOL fEnterMode)
{
    DOFMSG(TEXT("ContextSensitiveHelp called"));
    return _pdoh->ContextSensitiveHelp(fEnterMode);
}

HRESULT CDocObjectFrame::GetBorder(LPRECT lprectBorder)
{
    // DOFMSG(TEXT("GetBorder called"));
    return _pdoh->_pipu ?
        _pdoh->_pipu->GetBorder(lprectBorder) : E_UNEXPECTED;
}

HRESULT CDocObjectFrame::RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    DOFMSG(TEXT("RequestBorderSpace called"));
    return _pdoh->_pipu ?
        _pdoh->_pipu->RequestBorderSpace(pborderwidths) : E_UNEXPECTED;
}

HRESULT CDocObjectFrame::SetBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    // DOFMSG(TEXT("SetBorderSpace called"));
    return _pdoh->_pipu ?
        _pdoh->_pipu->SetBorderSpace(pborderwidths) : E_UNEXPECTED;
}

HRESULT CDocObjectFrame::SetActiveObject(
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    DOFMSG(TEXT("SetActiveObject called"));

    // Note that we need to call both.
    _pdoh->_xao.SetActiveObject(pActiveObject);

    if (_pdoh->_pipu) {
        //
        // Note that we should pass proxy IOleActiveObject pointer instead.
        //
        _pdoh->_pipu->SetActiveObject(pActiveObject ? &_pdoh->_xao : NULL, pszObjName);
    }
    return S_OK;
}

HRESULT CDocObjectFrame::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    DOFMSG2(TEXT("InsertMenus called with"), hmenuShared);
    return _pdoh->_InsertMenus(hmenuShared, lpMenuWidths);
}

HRESULT CDocObjectFrame::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    DOFMSG2(TEXT("SetMenu called with"), hmenuShared);
    return _pdoh->_SetMenu(hmenuShared, holemenu, hwndActiveObject);
}

HRESULT CDocObjectFrame::RemoveMenus(HMENU hmenuShared)
{
    DOFMSG(TEXT("RemoveMenus called"));
    return _pdoh->_RemoveMenus(hmenuShared);
}

HRESULT CDocObjectFrame::SetStatusText(LPCOLESTR pszStatusText)
{
    DOFMSG(TEXT("SetStatusText called"));
    return _pdoh->_SetStatusText(pszStatusText);
}

HRESULT CDocObjectFrame::EnableModeless(BOOL fEnable)
{
    DOFMSG(TEXT("EnableModeless called"));
    return _pdoh->_EnableModeless(fEnable);
}

// IOleInPlaceFrame::TranslateAccelerator
HRESULT CDocObjectFrame::TranslateAccelerator(LPMSG lpmsg, WORD wID)
{
    // NOTES: This code remains as-is. If we have something special
    //  it should be done in CDocObjectHost::TranslateAccelerator
    return _pdoh->_TranslateAccelerator(lpmsg, wID);
}

HRESULT CDocObjectFrame::QueryStatus(const GUID *pguidCmdGroup,
    ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    DOFMSG(TEXT("QueryStatus called"));
    return _pdoh->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
}

HRESULT CDocObjectFrame::Exec(const GUID *pguidCmdGroup,
    DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    DOFMSG(TEXT("Exec called"));
    return _pdoh->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}

//***   CPAO::TranslateAccelerator (IOIPAO::TranslateAccelerator)
//
HRESULT CProxyActiveObject::TranslateAccelerator(
    LPMSG lpmsg)
{
    HRESULT hres = E_FAIL;

    // IShellBrowser is supporsed to call ISV::TranslateAcceleratorSV,
    // but why not be nice?
    ASSERT(!_pdoh->_fCycleFocus);

    //
    // Don't call DocObject's TranslateAccelarator with non-key messages.
    // It's better to be IE compatible.
    //
    BOOL fKeybrdMsg = IsInRange(lpmsg->message, WM_KEYFIRST, WM_KEYLAST);
    if (fKeybrdMsg && _piact && (hres = _piact->TranslateAccelerator(lpmsg)) == S_OK) {
        if (_pdoh->_fCycleFocus) {
            // we got called back by trident (IOCS::TA), but deferred it.
            // time to pay the piper.
            TraceMsg(DM_FOCUS, "DOH::IOIPAO::TA piao->TA==S_OK ret _fCycleFocus=FALSE hr=S_FALSE (piper)");
            _pdoh->_fCycleFocus = FALSE;
            return S_FALSE;     // time to pay the piper
        }
        return S_OK;
    }
    if (_pdoh->_fCycleFocus) {
        TraceMsg(DM_ERROR, "DOH::IOIPAO::TA _fCycleFocus && hres=%x (!=S_OK)", hres);
        _pdoh->_fCycleFocus = FALSE;
        return S_FALSE;
    }

    return _pdoh->TranslateHostAccelerators(lpmsg);
}

HRESULT CProxyActiveObject::OnFrameWindowActivate(
    BOOL fActivate)
{
    TraceMsg(TF_SHDUIACTIVATE, "CProxyAO::OnFrameWindowActivate called with %d (_piact=%x)",
             fActivate, _piact);
    if (_piact) {
        return _piact->OnFrameWindowActivate(fActivate);
    }
    return S_OK;
}

HRESULT CProxyActiveObject::OnDocWindowActivate(
    BOOL fActivate)
{
    // BUGBUG: Implement it later!
    return S_OK;
}

HRESULT CProxyActiveObject::ResizeBorder(
    LPCRECT prcBorder,
    IOleInPlaceUIWindow *pUIWindow,
    BOOL fFrameWindow)
{
    if (_piact) {
        //
        // Note that we must pass our proxy frame instead!
        //
        return _piact->ResizeBorder(prcBorder, &_pdoh->_dof, TRUE);
    }
    return E_FAIL;
}
void CProxyActiveObject::SetActiveObject(IOleInPlaceActiveObject *piact )
{
    if (_piact)
    {
        ATOMICRELEASE(_piact);
        _hwnd = NULL;
    }

    if (piact) {
        _piact = piact;
        _piact->AddRef();
        _piact->GetWindow(&_hwnd);
    }
}

HRESULT CProxyActiveObject::EnableModeless(
    BOOL fEnable)
{
    // IShellBrowser is supporsed to call ISV::EnableModelessSV,
    // but why not be nice?
    HRESULT hres = S_OK;
    if (_piact)
        hres = _piact->EnableModeless(fEnable);
    return hres;
}

HRESULT CProxyActiveObject::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IOleInPlaceActiveObject) ||
        IsEqualIID(riid, IID_IOleWindow) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IOleInPlaceActiveObject*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    _pdoh->AddRef();
    return NOERROR;
}

ULONG CProxyActiveObject::AddRef(void)
{
    return _pdoh->AddRef();
}

ULONG CProxyActiveObject::Release(void)
{
    return _pdoh->Release();
}

HRESULT CProxyActiveObject::GetWindow(HWND * lphwnd)
{
    return _pdoh->GetWindow(lphwnd);
}

HRESULT CProxyActiveObject::ContextSensitiveHelp(BOOL fEnterMode)
{
    return _pdoh->ContextSensitiveHelp(fEnterMode);
}

#define ANIMATION_WND_WIDTH     (100+3)

void CDocObjectHost::_PlaceProgressBar(BOOL fForcedLayout)
{
    if (_psb) {
        HWND hwndStatus = NULL;
        _psb->GetControlWindow(FCW_STATUS, &hwndStatus);
        if (hwndStatus) {
            RECT rc;
            INT_PTR fSimple = SendMessage(hwndStatus, SB_ISSIMPLE, 0, 0);

            if (!fSimple || fForcedLayout) {
                // While processing WM_SIZE, turn off the simple mode temporarily.
                if (fSimple)
                    _psb->SendControlMsg(FCW_STATUS, SB_SIMPLE, 0, 0, NULL);

                GetClientRect(hwndStatus, &rc);
                const UINT cxZone = ZoneComputePaneSize(hwndStatus);
                UINT cxProgressBar = (_fShowProgressCtl) ? 100 : 0;

                INT nSBWidth = rc.right - rc.left;
                INT arnRtEdge[STATUS_PANES] = {1};
                INT nIconPaneWidth = GetSystemMetrics(SM_CXSMICON) +
                                     (GetSystemMetrics(SM_CXEDGE) * 4);
                INT nWidthReqd = cxZone + cxProgressBar + (nIconPaneWidth * 2);

                arnRtEdge[STATUS_PANE_NAVIGATION] = max(1, nSBWidth - nWidthReqd);

                nWidthReqd -= cxProgressBar;
                arnRtEdge[STATUS_PANE_PROGRESS] = max(1, nSBWidth - nWidthReqd);

                nWidthReqd -= (nIconPaneWidth);
                arnRtEdge[STATUS_PANE_OFFLINE] = max(1, nSBWidth - nWidthReqd);

//                nWidthReqd -= (nIconPaneWidth);
//                arnRtEdge[STATUS_PANE_PRINTER] = max(1, nSBWidth - nWidthReqd);

                nWidthReqd -= (nIconPaneWidth);
                arnRtEdge[STATUS_PANE_SSL] = max(1, nSBWidth - nWidthReqd);

                arnRtEdge[STATUS_PANE_ZONE] = -1;

                LRESULT nParts = 0;
                nParts = SendMessage(hwndStatus, SB_GETPARTS, 0, 0L);
                if (nParts != STATUS_PANES)
                {
                    for ( int n = 0; n < nParts; n++)
                    {
                        SendMessage(hwndStatus, SB_SETTEXT, n | SBT_NOTABPARSING, NULL);
                        SendMessage(hwndStatus, SB_SETICON, n, NULL);
                    }
                    SendMessage(hwndStatus, SB_SETPARTS, 0, 0L);
                }

                SendMessage(hwndStatus, SB_SETPARTS, STATUS_PANES, (LPARAM)arnRtEdge);

                if (!_hwndProgress) {
                    _psb->GetControlWindow(FCW_PROGRESS, &_hwndProgress);
                }

                if (_hwndProgress) {
                    if (SendMessage(hwndStatus, SB_GETRECT, 1, (LPARAM)&rc))
                    {
                        InflateRect(&rc, -GetSystemMetrics(SM_CXEDGE), -GetSystemMetrics(SM_CYEDGE));
                    }
                    else
                    {
                        rc.left = rc.top = rc.right = rc.bottom = 0;
                    }

                    SetWindowPos(_hwndProgress, NULL,
                                 rc.left, rc.top,
                                 rc.right-rc.left, rc.bottom-rc.top,
                                 SWP_NOZORDER | SWP_NOACTIVATE);
                }

                SendMessage(hwndStatus, SB_SETTEXT, 1 | SBT_NOTABPARSING, (LPARAM)TEXT(""));
                SendMessage(hwndStatus, SB_SETMINHEIGHT, GetSystemMetrics(SM_CYSMICON) +
                                                         GetSystemMetrics(SM_CYBORDER) * 2, 0L);

                // Restore
                if (fSimple)
                     SendMessage(hwndStatus, SB_SIMPLE, TRUE, 0);
            }
        }
    } else {
        TraceMsg(TF_ERROR, "_PlaceProgressBar ASSERT(_psb) this=%x", this);
    }
}

void CDocObjectHost::_ActivateOleObject(void)
{
    HRESULT hres;
    _pole->SetClientSite(NULL);

    if (_fDontInPlaceNavigate()) {
        TraceMsg(TF_SHDAPPHACK, "CDOH::_ActivateOleObject calling DoVerb because of _fDontInPlaceNavigate()");
    }

    _EnableModeless(FALSE);
    hres = _pole->DoVerb(
        _fUseOpenVerb() ? OLEIVERB_OPEN : OLEIVERB_PRIMARY,
        NULL, NULL, (UINT)-1, NULL, NULL);
    _EnableModeless(TRUE);

    if (SUCCEEDED(hres)) {
        CShdAdviseSink_Advise(_pwb, _pole);
    } else {
        TraceMsg(DM_ERROR, "CDOH::_ActivateOleObject DoVerb failed %x.", hres);
    }

    // We must release the OLE object to avoid calling Close
    // from _UnBind.
    _ReleaseOleObject();

    _ReleasePendingObject();

}

//
// The docobject's READYSTATE property may have changed
//
void CDocObjectHost::_OnReadyState(long lVal)
{
    // Forward this to the browser so we can source ReadyState events properly
    //  TRACE this zekel
    if (_psb)
    {
        IDocNavigate *pdn;
        if (SUCCEEDED(_psb->QueryInterface(IID_IDocNavigate, (LPVOID*)&pdn)))
        {
            ASSERT(_psv);
            pdn->OnReadyStateChange(_psv, lVal);
            pdn->Release();
        }
    }

    // NOTE: The below code is rather wasteful. The OmWindow stuff
    // should trigger off the above ReadyState code.
    //
    IShellHTMLWindowSupport *phtmlWS;
    if (_psp && SUCCEEDED(_psp->QueryService(SID_SOmWindow, IID_IShellHTMLWindowSupport, (void**)&phtmlWS)))
    {
        phtmlWS->ReadyStateChangedTo(lVal, _psv);
        phtmlWS->Release();
    }

    if (lVal >= READYSTATE_INTERACTIVE)
    {
        // Technically we can get this value multiple times,
        // so make sure we call _Navigate only once.
        //
        if (!_fReadystateInteractiveProcessed)
        {
            _fReadystateInteractiveProcessed = TRUE;

            _Navigate();
        }

        if (lVal == READYSTATE_COMPLETE)
        {
            _OnSetProgressPos(0, PROGRESS_RESET);

            if (_pwb)
            {
                WCHAR szTitle[MAX_PATH]; // titles are only stored up to this size

                if (SUCCEEDED(_pwb->GetTitle(_psv, szTitle, ARRAYSIZE(szTitle))))
                {
                    // BharatS : 01/09/97 : There is no need to tie the updating of the title in the
                    // history to the updating of the INTSITE database. Thus the INTSITE database
                    // update can be moved out of AddUrlToUrlHistoryStg() in history.cpp when time permits
                    // to a more logical place such as someplace in dochost.cpp
                    //
                    _UpdateHistoryAndIntSiteDB(szTitle);
                } else {
                    _UpdateHistoryAndIntSiteDB(NULL);
                }
            }
        }
    }
}

HRESULT CDocObjectHost::_OnChangedReadyState()
{
    IDispatch * p_idispatch;

    ASSERT(_pole || _fFriendlyError);
    if (!_pole)
        return E_UNEXPECTED;

    if (SUCCEEDED(_pole->QueryInterface( IID_IDispatch, (LPVOID*) &p_idispatch)))
    {
        VARIANTARG va;
        EXCEPINFO exInfo;

        va.vt = 0;
        if (EVAL(SUCCEEDED(p_idispatch->Invoke( DISPID_READYSTATE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, (DISPPARAMS *)&g_dispparamsNoArgs, &va, &exInfo, NULL))
          && va.vt == VT_I4))
        {
            _OnReadyState(va.lVal);

            if (va.lVal == READYSTATE_COMPLETE)
            {
                _RemoveTransitionCapability();
            }

        }
        p_idispatch->Release();
    }

    return( NOERROR );
}

HRESULT CDocObjectHost::OnRequestEdit(DISPID dispid)
{
    return E_NOTIMPL;
}

//
// OnChanged
//
//   Notification from the DocObject that one of its
//   properties has changed.
//
HRESULT CDocObjectHost::OnChanged(DISPID dispid)
{
    if (DISPID_READYSTATE == dispid || DISPID_UNKNOWN == dispid)
        return _OnChangedReadyState();

    return S_OK;
}

extern BOOL _ValidateURL(LPTSTR pszName);

void CDocObjectHost::_UpdateHistoryAndIntSiteDB(LPCWSTR pwszTitle)
{
    TCHAR szUrl[MAX_URL_STRING];

    if (SUCCEEDED(_GetCurrentPage(szUrl, MAX_URL_STRING, TRUE)) &&
        _ValidateURL(szUrl, UQF_DEFAULT))
    {
        // update history and intsite if this isn't a silent browse
        BOOL bSilent = FALSE;
        HRESULT hr = _GetOfflineSilent(NULL, &bSilent);

        if(SUCCEEDED(hr) && (!bSilent))
        {

            AddUrlToUrlHistoryStg(szUrl,
                      pwszTitle,
                      _psb,
                      _fWriteHistory,
                      _fSelectHistory ? _pocthf:NULL,
                      get_punkSFHistory(), NULL);

            //
            //  Satona had the redirect code ifdef'd out, but for
            //  netscape compatibility, we need to update the history
            //  for the redirected URL as well.  - zekel - 22-JUL-97
            //

            // If this page is a redirect, update intsite for destination too
            INTERNET_CACHE_ENTRY_INFO *pCacheEntry = NULL;

#ifndef UNIX
            WCHAR    chBuf[MAX_CACHE_ENTRY_INFO_SIZE];

            // Find entry in cache using redirect map
            pCacheEntry = (INTERNET_CACHE_ENTRY_INFO *)chBuf;
#else
            union
            {
                double _alignOn8;
                WCHAR   _chBuf[MAX_CACHE_ENTRY_INFO_SIZE];
            } chBuf;

            // Find entry in cache using redirect map
            pCacheEntry = (INTERNET_CACHE_ENTRY_INFO *)&chBuf;
#endif /* !UNIX */

            DWORD dwSize = SIZEOF(chBuf);
            BOOL fSuccess = GetUrlCacheEntryInfoEx(szUrl, pCacheEntry, &dwSize, NULL, 0, NULL, 0);
            if(fSuccess)
            {
                // If we have a different url than we started with, update it too
                if(StrCmp(szUrl, pCacheEntry->lpszSourceUrlName)) {
                    AddUrlToUrlHistoryStg(pCacheEntry->lpszSourceUrlName,
                              pwszTitle,
                              _psb,
                              _fWriteHistory,
                              _fSelectHistory ? _pocthf:NULL,
                              get_punkSFHistory(), NULL);
                }
            }
        }
    }
}



//
// CDocObjectHost::_SetUpTransitionCapability()
//
//   Returns TRUE iff all the following hold true:
//      - object has readystate property
//      - readystate property is currently < interactive
//      - Object supports IPropertyNotifySink
//   Then this object supports delayed switching when it
//   it tells us that it is ready...
//
//   This is how we switch pages only when the new page is ready to be
//   switched to.    Also, by doing this we can also make the switch smooth
//   by applying graphical transitions.
//

BOOL CDocObjectHost::_SetUpTransitionCapability()
{
    // By default DocObject doesn't have gray-flash communication
    BOOL fSupportsReadystate = FALSE;
    long lReadyState = 0;   // Init to avoid a bogus C4701 warning

    // Sanity Check
    if (!_pole)
        return(fSupportsReadystate);

    // Check for proper readystate support
    BOOL fReadyStateOK = FALSE;
    IDispatch * p_idispatch;
    if (SUCCEEDED(_pole->QueryInterface( IID_IDispatch, (LPVOID*) &p_idispatch )))
    {
        VARIANTARG va;
        EXCEPINFO exInfo;

        if (SUCCEEDED(p_idispatch->Invoke( DISPID_READYSTATE, IID_NULL, LOCALE_USER_DEFAULT,  DISPATCH_PROPERTYGET, (DISPPARAMS *)&g_dispparamsNoArgs, &va, &exInfo, NULL)))
        {
            if ((va.vt == VT_I4) && (va.lVal < READYSTATE_COMPLETE))
            {
                lReadyState = va.lVal;
                fReadyStateOK = TRUE;
            }
        }

        p_idispatch->Release();
    }

    if (fReadyStateOK)
    {
        // Check and Set-Up IPropertyNotifySink
        if (SUCCEEDED(ConnectToConnectionPoint(SAFECAST(this, IPropertyNotifySink*), IID_IPropertyNotifySink, TRUE, _pole, &_dwPropNotifyCookie, NULL)))
        {
            fSupportsReadystate = TRUE;
            _OnReadyState(lReadyState);
        }
    }

    // If no ReadyState, we simulate it
    if (!fSupportsReadystate)
    {
        _OnReadyState(READYSTATE_COMPLETE);
    }

    return(fSupportsReadystate);
}

// This removes any property notify sink we set up
//
BOOL CDocObjectHost::_RemoveTransitionCapability()
{
    BOOL fRet = FALSE;

    if (_dwPropNotifyCookie)
    {
        ConnectToConnectionPoint(NULL, IID_IPropertyNotifySink, FALSE, _pole, &_dwPropNotifyCookie, NULL);
        fRet = TRUE;
    }

    return(fRet);
}

HRESULT _GetRequestFlagFromPIB(IBinding *pib, DWORD *pdwOptions)
{
    HRESULT hres = E_FAIL;
    *pdwOptions = 0;
    if (pib)
    {
        IWinInetInfo* pwinet;
        hres = pib->QueryInterface(IID_IWinInetInfo, (LPVOID*)&pwinet);
        if (SUCCEEDED(hres)) {
            DWORD cbSize = SIZEOF(*pdwOptions);
            hres = pwinet->QueryOption(INTERNET_OPTION_REQUEST_FLAGS,
                                (LPVOID)pdwOptions, &cbSize);
            TraceMsg(TF_SHDNAVIGATE, "DOH::BSC::_HFNS() pwinet->QueryOptions hres=%x dwOptions=%x", hres, *pdwOptions);

            pwinet->Release();
        }
    }
    return hres;
}

void CDocObjectHost::_Navigate()
{
    NAVMSG3(TEXT("_Navigate calling SHDVID_ACTIVATEMENOW"), 0, NULL);
    if (_pmsoctBrowser)
        _pmsoctBrowser->Exec(&CGID_ShellDocView, SHDVID_ACTIVATEMENOW, NULL, NULL, NULL);
}

#ifndef UNIX
void CDocObjectHost::_NavigateFolder(BSTR bstrUrl)
{
    // This code accesses one of IE's default behaviors which
    // allows for navigation to a web folder.
    // ------------------------------------------------------

    Iwfolders * pWF = NULL;
    IElementBehaviorFactory * pebf = NULL;
    IElementBehavior * pPeer = NULL;
    HWND hwndOwner = NULL;
    IServiceProvider * psp = NULL;
    IUnknown * punkwb = NULL;

    // Make the peer factory
    if  ( !_psb || (FAILED(_psb->GetWindow (&hwndOwner))) ||
          (FAILED(CoCreateInstance(CLSID_PeerFactory, NULL, CLSCTX_INPROC,
                          IID_IElementBehaviorFactory, (LPVOID *)&pebf))) ||
          (FAILED(pebf->FindBehavior(L"httpFolder", NULL, NULL, &pPeer))) ||
          (FAILED(pPeer->QueryInterface(IID_Iwfolders, (void **)&pWF))) ||
          (FAILED(QueryService(SID_STopLevelBrowser, IID_IServiceProvider, (LPVOID *)&psp))) ||
          (FAILED(psp->QueryService(SID_SContainerDispatch, IID_IUnknown, (LPVOID*)&punkwb))) )
    {
        WCHAR wszMessage[MAX_PATH];
        WCHAR wszTitle[MAX_PATH];

        MLLoadShellLangString(IDS_ERRORINTERNAL, wszMessage, ARRAYSIZE(wszMessage));
        MLLoadShellLangString(IDS_NAME, wszTitle, ARRAYSIZE(wszTitle));
        MessageBox(hwndOwner, wszMessage, wszTitle, MB_OK | MB_ICONERROR);
        goto done;
    }

    // Sundown: coercion to unsigned long is valid for HWNDs
    pWF->navigateNoSite(bstrUrl, NULL, PtrToUlong(hwndOwner), punkwb);

done:
    if (pebf)
        pebf->Release();
    if (pPeer)
        pPeer->Release();
    if (pWF)
        pWF->Release();
    if (punkwb)
        punkwb->Release();
    if (psp)
        psp->Release();
}
#endif //UNIX

void CDocObjectHost::_CancelPendingNavigation(BOOL fAsyncDownload)
{
    ASSERT(_phf);

    // the hlframe no longer knows if the clear was a cancel or a start of navigation
    // because we don't call anything tosignal a successfull navigation
    if (_pmsoctBrowser) {
        TraceMsg(DM_TRACE, "DOH::_CancelPendingNavigation calling _pmsc->Exec");
        if (fAsyncDownload) {
            VARIANT var = { 0 };
            var.vt = VT_I4;
            ASSERT(var.lVal == FALSE);    // asynd download is done.
            _pmsoctBrowser->Exec(&CGID_Explorer, SBCMDID_CANCELNAVIGATION, 0, &var, NULL);
        } else {
            _pmsoctBrowser->Exec(&CGID_Explorer, SBCMDID_CANCELNAVIGATION, 0, NULL, NULL);
        }
    }

    // release our navigation state
    _phf->Navigate(0, NULL, NULL, (IHlink*)-1);

}

void CDocObjectHost::_ResetStatusBar()
{
    _SetStatusText(TEXT(""));
    if (_psb)
        _psb->SendControlMsg(FCW_STATUS, SB_SETICON, STATUS_PANE_NAVIGATION,
                              (LPARAM)g_ahiconState[IDI_STATE_NORMAL-IDI_STATE_FIRST], NULL);
    return;
}

void CDocObjectHost::_DoAsyncNavigation(LPCTSTR pszURL)
{
    if (_pmsoctBrowser) {
        VARIANT vararg = {0};

        vararg.vt = VT_BSTR;
        vararg.bstrVal = SysAllocStringT(pszURL);
        if (vararg.bstrVal)
        {
            TraceMsg(DM_TRACE, "DOH::_DoAsyncNavigation calling _pmsc->Exec");
            _pmsoctBrowser->Exec(&CGID_Explorer, SBCMDID_ASYNCNAVIGATION, 0, &vararg, NULL);
            VariantClear(&vararg);
        }
    }
}

// note: szError is never used, so don't waste time setting it
UINT SHIEErrorMsgBox(IShellBrowser* psb,
                    HWND hwndOwner, HRESULT hrError, LPCWSTR szError, LPCTSTR pszURLparam,
                    UINT idResource, UINT wFlags)
{
    UINT uRet = IDCANCEL;
    TCHAR szMsg[MAX_PATH];
    LPCTSTR pszURL = TEXT("");
    HWND hwndParent = hwndOwner;
    IShellBrowser *psbParent = NULL;

    // if a URL was specified, use it; otherwise use empty string
    if (pszURLparam)
        pszURL = pszURLparam;

    //
    // NOTES: This table of error code will be mapped to (IDS_ERRMSG_FIRST +
    //  offset) and we MLLoadString it.
    //
    const static c_ahres[] = {
        HRESULT_FROM_WIN32(ERROR_INTERNET_INVALID_URL),
        HRESULT_FROM_WIN32(ERROR_INTERNET_NAME_NOT_RESOLVED),
        INET_E_UNKNOWN_PROTOCOL,
        INET_E_REDIRECT_FAILED,
        INET_E_DATA_NOT_AVAILABLE,
    };

    for (int i=0; i<ARRAYSIZE(c_ahres); i++) {
        if (c_ahres[i]==hrError) {
            MLLoadString(IDS_ERRMSG_FIRST+i, szMsg, ARRAYSIZE(szMsg));
            break;
        }
    }

    if (i >= ARRAYSIZE(c_ahres))
    {
        // Default message if FormatMessage doesn't recognize dwLastError
        MLLoadString(IDS_UNDEFINEDERR, szMsg, ARRAYSIZE(szMsg));

        if (hrError >= HRESULT_FROM_WIN32(INTERNET_ERROR_BASE)
            && hrError <= HRESULT_FROM_WIN32(INTERNET_ERROR_LAST))
        {
            HMODULE hmod = GetModuleHandle(TEXT("WININET"));
            ASSERT(hmod);
            FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, (LPCVOID)hmod, HRESULT_CODE(hrError), 0L,
                szMsg, ARRAYSIZE(szMsg), NULL);

        } else {
            // See if one of the system components has an error message
            // for this error.  If not, szMsg will retain our default
            // message to handle this.
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hrError, 0L,
                szMsg, ARRAYSIZE(szMsg), NULL);
        }
    }

    psbParent = psb;
    if (psbParent)
    {
        psbParent->AddRef();
    }

    //  Here we make an heroic effort to find a visible window to run the dialog against
    //  If we can't, then we bail, to avoid weird UI effect (particularly when the frametop
    //  browser is in kiosk mode
    if (!IsWindowVisible(hwndParent))
    {
        if (NULL == psb || FAILED(psb->GetWindow(&hwndParent)) || !IsWindowVisible(hwndParent))
        {
            hwndParent = NULL;
            ATOMICRELEASE(psbParent);
        }
        if (NULL == hwndParent && psb)
        {
            IUnknown_QueryService(psb, SID_STopFrameBrowser, IID_IShellBrowser, (LPVOID*)&psbParent);
            if (NULL == psbParent || FAILED(psbParent->GetWindow(&hwndParent)) || !IsWindowVisible(hwndParent))
            {
                hwndParent = NULL;
            }
        }
    }

    if (hwndParent)
    {
        if (psbParent) {
            psbParent->EnableModelessSB(FALSE);
        }

        uRet = MLShellMessageBox(hwndParent,
                               MAKEINTRESOURCE(idResource),
                               MAKEINTRESOURCE(IDS_TITLE),
                               wFlags, pszURL,szMsg);

        if (psbParent) {
            psbParent->EnableModelessSB(TRUE);
        }
    }

    if (psbParent)
    {
        UINT cRef = psbParent->Release();

        AssertMsg(cRef>0, TEXT("IE_ErrorMsgBox psb->Release returned 0."));
    }

    return uRet;
}

//
// See if the URL is of a type that we should
// ShellExecute()
//
BOOL ShouldShellExecURL( LPTSTR pszURL )
{
    BOOL fRet = FALSE;
    TCHAR sz[MAX_PATH];
    DWORD cch = SIZECHARS(sz);
    HKEY hk;

    if(SUCCEEDED(UrlGetPart(pszURL, sz, &cch, URL_PART_SCHEME, 0))
    && SUCCEEDED(AssocQueryKey(0, ASSOCKEY_CLASS, sz, NULL, &hk)))
    {
        //
        //  HACKHACK - telnet.exe will fault on buffer overrun
        //      if the url is > 230. we special case here.
        //
        if (lstrlen(pszURL) <= 230 ||
                (StrCmpI(sz, TEXT("telnet")) && 
                 StrCmpI(sz, TEXT("rlogin")) &&
                 StrCmpI(sz, TEXT("tn3270"))
                )
           )
        {
            fRet = (NOERROR == RegQueryValueEx(hk, TEXT("URL Protocol"), NULL, NULL, NULL, NULL));
        }

        RegCloseKey(hk);
    }
    return fRet;
}


//========================================================================
// class CShdAdviseSink
//========================================================================

class CShdAdviseSink : public IAdviseSink
{
public:
    // *** IUnknown methods ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IAdviseSink methods ***
    virtual void __stdcall OnDataChange(
        FORMATETC *pFormatetc,
        STGMEDIUM *pStgmed);
    virtual void __stdcall OnViewChange(
        DWORD dwAspect,
        LONG lindex);
    virtual void __stdcall OnRename(
        IMoniker *pmk);
    virtual void __stdcall OnSave( void);
    virtual void __stdcall OnClose( void);

    CShdAdviseSink(IBrowserService* pwb, IOleObject* pole);
    ~CShdAdviseSink();

protected:
    UINT _cRef;
    IOleObject* _pole;
    DWORD       _dwConnection;
};

//
// BUGBUG: We'd better maintain the list of those CShdAdviseSink
//  per-thread so that we don't leak all those oleobjects when
//  the thread is terminated before those objects are closed.
//
void CShdAdviseSink_Advise(IBrowserService* pwb, IOleObject* pole)
{
    IAdviseSink* padv = new CShdAdviseSink(pwb, pole);
    // If pole->Advise succeeds, it will addreff to IAdviseSink.
    if (padv != NULL)
    {
        padv->Release();
    }
}

CShdAdviseSink::CShdAdviseSink(IBrowserService* pwb, IOleObject* pole)
    : _cRef(1)
{
    ASSERT(pole);
    TraceMsg(DM_ADVISE, "CShdAdviseSink(%x) being constructed", this);
    HRESULT hres = pole->Advise(this, &_dwConnection);
    if (SUCCEEDED(hres)) {
        _pole = pole;
        pole->AddRef();

        TraceMsg(DM_ADVISE, "CShdAdviseSink(%x) called pole->Advise. new _cRef=%d (%x)", this, _cRef, _dwConnection);
    }
}

CShdAdviseSink::~CShdAdviseSink()
{
    TraceMsg(DM_ADVISE, "CShdAdviseSink(%x) being destroyed", this);
    ATOMICRELEASE(_pole);
}

ULONG CShdAdviseSink::AddRef()
{
    _cRef++;
    TraceMsg(TF_SHDREF, "CShdAdviseSink(%x)::AddRef called, new _cRef=%d", this, _cRef);
    return _cRef;
}

ULONG CShdAdviseSink::Release()
{
    _cRef--;
    TraceMsg(TF_SHDREF, "CShdAdviseSink(%x)::Release called, new _cRef=%d", this, _cRef);
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CShdAdviseSink::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IAdviseSink) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IAdviseSink*)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

void CShdAdviseSink::OnDataChange(
    FORMATETC *pFormatetc,
    STGMEDIUM *pStgmed)
{
}

void CShdAdviseSink::OnViewChange(
    DWORD dwAspect,
    LONG lindex)
{
}

void CShdAdviseSink::OnRename(
    IMoniker *pmk)
{
}

void CShdAdviseSink::OnSave( void)
{
}

void CShdAdviseSink::OnClose( void)
{
    TraceMsg(DM_ADVISE, "CShdAdviseSink(%x)::OnClose called. Calling Unadvise. _cRef=%d", this, _cRef);
    HRESULT hres;
    AddRef();
    ASSERT(_pole);
    if (_pole)  // paranoia
    {
        hres = _pole->Unadvise(_dwConnection);
        ATOMICRELEASE(_pole);
        TraceMsg(DM_ADVISE, "CShdAdviseSink(%x)::OnClose. Called Unadvise(%x). new _cRef=%d", this, hres, _cRef);
    }
    Release();
}

/// adding property sheet pages

HRESULT CDocObjectHost::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    HRESULT hres = S_OK;
    IShellPropSheetExt *pspse;
    /*
     * Create a property sheet page for required page, including imported File
     * Types property sheet.
     */
    // add stuff that the docobj itself has.
    if (_pole)
    {
        if (SUCCEEDED(_pole->QueryInterface(IID_IShellPropSheetExt, (LPVOID*)&pspse)))
        {
            hres = pspse->AddPages(lpfnAddPage, lParam);
            pspse->Release();
        }
        else
        {
            // Some docobjects don't know about IShellPropSheetExt (ie, Visual Basic),
            // so do it ourselves.

            if (NULL == _hinstInetCpl)
                _hinstInetCpl = LoadLibrary(TEXT("inetcpl.cpl"));

            if (_hinstInetCpl)
            {
                PFNADDINTERNETPROPERTYSHEETSEX pfnAddSheet = (PFNADDINTERNETPROPERTYSHEETSEX)GetProcAddress(_hinstInetCpl, STR_ADDINTERNETPROPSHEETSEX);
                if (pfnAddSheet)
                {
                    IEPROPPAGEINFO iepi = {0};

                    // we just want the security page.
                    iepi.cbSize = sizeof(iepi);
                    iepi.dwFlags = (DWORD)-1;       // all pages

                    hres = pfnAddSheet(lpfnAddPage, lParam, 0, 0, &iepi);
                }
                // Don't FreeLibrary here, otherwise PropertyPage will GP-fault!
            }
        }
    }

    return hres;
}


//==========================================================================
// IDocHostUIHandler implementation
//==========================================================================

HRESULT CDocObjectHost::TranslateAccelerator(LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
    if (_pWebOCUIHandler)
        return _pWebOCUIHandler->TranslateAccelerator(lpMsg, pguidCmdGroup, nCmdID);
    return _dhUIHandler.TranslateAccelerator(lpMsg, pguidCmdGroup, nCmdID);
}


HRESULT CDocObjectHost::GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    // REVIEW: Does this apply anymore?
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::GetDropTarget called");

    if (_pWebOCUIHandler)
        return _pWebOCUIHandler->GetDropTarget(pDropTarget, ppDropTarget);

    HRESULT hres = S_OK;

    if (pDropTarget) {
        IDropTarget *pdtFrame;
        IDropTarget *pdt3;
        IDropTarget *pdtBlocking;

        QueryService(SID_STopFrameBrowser, IID_IDropTarget, (LPVOID*)&pdtFrame);

        // hack: this is because we need to look all the way through to top parents for a containing drop target
        // what we really need is a per dataobject drop target
        //
        // this is not required to be obtained
        QueryService(SID_ITopViewHost, IID_IDropTarget, (LPVOID*)&pdt3);
        if (IsSameObject(pdt3, pdtFrame)) {
            ATOMICRELEASE(pdt3);
        }

        // allow constrained browser bands like Search to prevent drop
        QueryService(SID_SDropBlocker, IID_IUnknown, (LPVOID*)&pdtBlocking);
        if (pdtBlocking)
        {
            ATOMICRELEASE(pdt3);
            pDropTarget = NULL;
        }

        if (pdtFrame) {

            *ppDropTarget = DropTargetWrap_CreateInstance(pDropTarget, pdtFrame, _hwnd, pdt3);
            if (!*ppDropTarget)
                hres = E_OUTOFMEMORY;

            TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::GetDropTarget returning S_OK");
            ASSERT(hres == S_OK);

            pdtFrame->Release();
        } else {
            ASSERT(0);
            hres = E_UNEXPECTED;
        }

        ATOMICRELEASE(pdtBlocking);
        ATOMICRELEASE(pdt3);

    } else {
        hres = E_INVALIDARG;
    }

    return hres;
}

HRESULT CDocObjectHost::ShowUI(
    DWORD dwID, IOleInPlaceActiveObject *pActiveObject,
    IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame,
    IOleInPlaceUIWindow *pDoc)
{
    if (_pWebOCUIHandler)
        return _pWebOCUIHandler->ShowUI(dwID, pActiveObject, pCommandTarget, pFrame, pDoc);

    if (_dwAppHack & BROWSERFLAG_MSHTML) // Who else will call on this interface?
    {
        if (_pmsoctBrowser)
        {
            TraceMsg(DM_PREMERGEDMENU, "DOH::ShowUI called this=%x pcmd=%x",
                     this,pCommandTarget);
            VARIANT var = { 0 };
            HRESULT hresT=_pmsoctBrowser->Exec(&CGID_Explorer, SBCMDID_SETMERGEDWEBMENU, 0, NULL, &var);
            if (SUCCEEDED(hresT))
            {
                if (_pcmdMergedMenu)
                {
                    // BUGBUG: Tell Trident to stop calling us twice
                    TraceMsg(DM_WARNING, "DOH::ShowUI called twice! "
                             "this=%x pcmdCur=%x pcmdNew=%x",
                             this, _pcmdMergedMenu, pCommandTarget);
                    _pcmdMergedMenu->Release();
                }
                _pcmdMergedMenu = pCommandTarget;
                _pcmdMergedMenu->AddRef();
                ASSERT(var.vt == VT_INT_PTR);
                _hmenuCur = (HMENU)var.byref;

                DEBUG_CODE( _DumpMenus(TEXT("after ShowUI"), TRUE); )
                return S_OK;
            }
        }
    }

    return S_FALSE;
}


HRESULT CDocObjectHost::HideUI(void)
{
    if (_pWebOCUIHandler)
        return _pWebOCUIHandler->HideUI();

    if (_pcmdMergedMenu) {
        _pcmdMergedMenu->Release();
        _pcmdMergedMenu = NULL;
    }

    return S_FALSE;
}

HRESULT CDocObjectHost::GetHostInfo(DOCHOSTUIINFO *pInfo)
{
    IServiceProvider * psp = NULL;
    IWebBrowser2     * pwb = NULL;
    VARIANT_BOOL       b = VARIANT_FALSE;
    DWORD              dwFlagsWebOC = 0;
    HRESULT            hr;

    if (_pWebOCUIHandler
        && SUCCEEDED(_pWebOCUIHandler->GetHostInfo(pInfo))
        )
    {
        dwFlagsWebOC = pInfo->dwFlags;
    }

    _dhUIHandler.GetHostInfo(pInfo);

    // Merge flags
    //
    pInfo->dwFlags |= dwFlagsWebOC;

    // Get the top level browser
    //
    hr = QueryService(SID_STopLevelBrowser, IID_IServiceProvider, (LPVOID *)&psp);
    if (hr)
        goto Cleanup;

    // Get the IWebBrowser2 object/interface
    //
    hr = psp->QueryService(SID_SContainerDispatch, IID_IWebBrowser2, (LPVOID*)&pwb);
    if (hr)
        goto Cleanup;

    // Tell the browser what our dochost flags are
    IEFrameAuto *pIEFrameAuto;
    if (SUCCEEDED(pwb->QueryInterface(IID_IEFrameAuto, (void **)&pIEFrameAuto)) && pIEFrameAuto)
    {
        pIEFrameAuto->SetDocHostFlags(pInfo->dwFlags);
        pIEFrameAuto->Release();
    }

    // Is the browser in Theater Mode?
    //
    hr = pwb->get_TheaterMode(&b);
    if (hr)
        goto Cleanup;

    // If so, turn on flat scrollbars.
    //
    if (b == VARIANT_TRUE)
        pInfo->dwFlags |= DOCHOSTUIFLAG_FLAT_SCROLLBAR;

Cleanup:
    ATOMICRELEASE(psp);
    ATOMICRELEASE(pwb);
    return S_OK;
}

HRESULT CDocObjectHost::ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
    HRESULT             hr;
    OLECMD              rgcmd = { IDM_BROWSEMODE, 0 };

    // If we're in the WebOC and it has a IDocHostUIHandler, use it.
    //
    if (_pWebOCUIHandler)
    {
        hr = _pWebOCUIHandler->ShowContextMenu(dwID, ppt, pcmdtReserved, pdispReserved);
        if (hr == S_OK)
            goto Cleanup;
    }

    // Find out if the DocObject is in Edit mode
    // Don't need apphack here as only Trident responds to CGID_MSHTML
    //

    hr = IUnknown_QueryStatus(pcmdtReserved, &CGID_MSHTML, 1, &rgcmd, NULL);
    if (    hr == S_OK
        &&  !(rgcmd.cmdf & OLECMDF_LATCHED))   // if not LATCHED means we're in edit mode.
    {
        hr = S_FALSE;
    }
    else
    {
        hr = _dhUIHandler.ShowContextMenu(dwID, ppt, pcmdtReserved, pdispReserved);
    }

Cleanup:
    return hr;
}

HRESULT CDocObjectHost::UpdateUI(void)
{
    if (_pWebOCUIHandler)
        return _pWebOCUIHandler->UpdateUI();
    return _dhUIHandler.UpdateUI();
}

HRESULT CDocObjectHost::EnableModeless(BOOL fEnable)
{
    if (_pWebOCUIHandler)
        return _pWebOCUIHandler->EnableModeless(fEnable);
    return _dhUIHandler.EnableModeless(fEnable);
}

HRESULT CDocObjectHost::OnDocWindowActivate(BOOL fActivate)
{
    if (_pWebOCUIHandler)
        return _pWebOCUIHandler->OnDocWindowActivate(fActivate);
    return _dhUIHandler.OnDocWindowActivate(fActivate);
}

HRESULT CDocObjectHost::OnFrameWindowActivate(BOOL fActivate)
{
    if (_pWebOCUIHandler)
        return _pWebOCUIHandler->OnFrameWindowActivate(fActivate);
    return _dhUIHandler.OnFrameWindowActivate(fActivate);
}

HRESULT CDocObjectHost::ResizeBorder( LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    if (_pWebOCUIHandler)
        return _pWebOCUIHandler->ResizeBorder(prcBorder, pUIWindow, fRameWindow);
    return _dhUIHandler.ResizeBorder(prcBorder, pUIWindow, fRameWindow);
}

HRESULT CDocObjectHost::GetOptionKeyPath(BSTR *pbstrKey, DWORD dw)
{
    if (_pWebOCUIHandler)
        return _pWebOCUIHandler->GetOptionKeyPath(pbstrKey, dw);
    return _dhUIHandler.GetOptionKeyPath(pbstrKey, dw);
}

HRESULT CDocObjectHost::GetExternal(IDispatch **ppDisp)
{
    if (_pWebOCUIHandler)
        return _pWebOCUIHandler->GetExternal(ppDisp);
    return _dhUIHandler.GetExternal(ppDisp);
}

HRESULT CDocObjectHost::TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    if (_pWebOCUIHandler)
        return _pWebOCUIHandler->TranslateUrl(dwTranslate, pchURLIn, ppchURLOut);
    return _dhUIHandler.TranslateUrl(dwTranslate, pchURLIn, ppchURLOut);
}

HRESULT CDocObjectHost::FilterDataObject(IDataObject *pDO, IDataObject **ppDORet)
{
    if (_pWebOCUIHandler)
        return _pWebOCUIHandler->FilterDataObject(pDO, ppDORet);
    return _dhUIHandler.FilterDataObject(pDO, ppDORet);
}

HRESULT CDocObjectHost::ShowMessage(HWND hwnd, LPOLESTR lpstrText, LPOLESTR lpstrCaption,
           DWORD dwType, LPOLESTR lpstrHelpFile, DWORD dwHelpContext, LRESULT __RPC_FAR *plResult)
{
    if (_pWebOCShowUI)
    {
        return _pWebOCShowUI->ShowMessage(hwnd, lpstrText, lpstrCaption, dwType,
                                            lpstrHelpFile, dwHelpContext, plResult);
    }

    return E_FAIL;
}

HRESULT CDocObjectHost::ShowHelp(HWND hwnd, LPOLESTR pszHelpFile, UINT uCommand, DWORD dwData,
           POINT ptMouse, IDispatch __RPC_FAR *pDispatchObjectHit)
{
    if (_pWebOCShowUI)
    {
        return _pWebOCShowUI->ShowHelp(hwnd, pszHelpFile, uCommand, dwData, ptMouse,
                                                pDispatchObjectHit);
    }

    return E_FAIL;
}

//
// support for script error caching and status bar notification
//

void
CDocObjectHost::_ScriptErr_Dlg(BOOL fOverridePerErrorMode)
{
    // we can get reentered through the message pump ShowHTMLDialog runs
    // therefore we might already have a dialog open when a second dialog
    // is requested

    if (_fScriptErrDlgOpen)
    {
        // a dialog is already open lower in the callstack
        // request an up-to-date dialog be shown
        // we have to do this because otherwise we might
        // be in per-error-mode and miss some errors which
        // arrived while the dialog lower in the callstack
        // was open. note that we only do this if we're set
        // to show notifications for every error.

        _fShowScriptErrDlgAgain = SHRegGetBoolUSValue(szRegKey_SMIEM,
                                                      szRegVal_ErrDlgPerErr,
                                                      FALSE,
                                                      TRUE);
    }
    else
    {
        _fScriptErrDlgOpen = TRUE;

        // keep showing dialogs as long as someone farther up the
        // call stack keeps requesting them

        do
        {
            BOOL    fShowDlg;

            _fShowScriptErrDlgAgain = FALSE;

            // if the user double clicked on the status bar, then we
            // show the dialog regardless of per-error-mode settings

            if (fOverridePerErrorMode)
            {
                fShowDlg = TRUE;

                // because of other script errors hitting the
                // _fScriptErrDlgOpen code above, we can arrive
                // here multiple times. The first time we show a
                // dialog can be because the user requested it,
                // but all subsequent times must be because we're
                // in "show every error" mode.

                fOverridePerErrorMode = FALSE;
            }
            else
            {
                fShowDlg = SHRegGetBoolUSValue(szRegKey_SMIEM,
                                               szRegVal_ErrDlgPerErr,
                                               FALSE,
                                               TRUE);
            }

            if (fShowDlg)
            {
                HRESULT hr;
                TCHAR   szResURL[MAX_URL_STRING];

                hr = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                                       HINST_THISDLL,
                                       ML_CROSSCODEPAGE,
                                       TEXT("ieerror.dlg"),
                                       szResURL,
                                       ARRAYSIZE(szResURL),
                                       TEXT("shdocvw.dll"));
                if (SUCCEEDED(hr))
                {
                    IMoniker *  pmk;
                    HWND        hwnd;

                    hr = CreateURLMoniker(NULL, szResURL, &pmk);
                    if (SUCCEEDED(hr))
                    {
                        VARIANT varErrorCache;

                        V_VT(&varErrorCache) = VT_DISPATCH;
                        V_DISPATCH(&varErrorCache) = _pScriptErrList;

                        GetWindow(&hwnd);
                        ShowHTMLDialog(hwnd, pmk, &varErrorCache, L"help:no", NULL);
                        ATOMICRELEASE(pmk);
                    }
                }
            }
        } while (_fShowScriptErrDlgAgain);

        _fScriptErrDlgOpen = FALSE;
    }
}

HRESULT
CDocObjectHost::_ScriptErr_CacheInfo(VARIANTARG *pvarIn)

{
    IHTMLDocument2 *    pOmDoc;
    IHTMLWindow2 *      pOmWindow;
    IHTMLEventObj *     pEventObj;
    HRESULT             hr;

    TCHAR *       apchNames[] =
                            { TEXT("errorLine"),
                              TEXT("errorCharacter"),
                              TEXT("errorCode"),
                              TEXT("errorMessage"),
                              TEXT("errorUrl")
                            };
    DISPID              aDispid[ARRAYSIZE(apchNames)];
    VARIANT             varOut[ARRAYSIZE(apchNames)];
    int                 i;

    pOmDoc = NULL;
    pOmWindow = NULL;
    pEventObj = NULL;

    // load the script error object

    hr = V_UNKNOWN(pvarIn)->QueryInterface(IID_IHTMLDocument2, (void **) &pOmDoc);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = pOmDoc->get_parentWindow(&pOmWindow);
    ATOMICRELEASE(pOmDoc);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = pOmWindow->get_event(&pEventObj);
    ATOMICRELEASE(pOmWindow);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // copy the interesting data out of the event object
    //

    for (i = 0; i < ARRAYSIZE(apchNames); i++)
    {
        DISPPARAMS  params;

        // get the property's dispid
        hr = pEventObj->GetIDsOfNames(IID_NULL, &apchNames[i], 1, LOCALE_SYSTEM_DEFAULT, &aDispid[i]);
        if (hr != S_OK)
        {
            ATOMICRELEASE(pEventObj);
            return hr;
        }

        params.cArgs = 0;
        params.cNamedArgs = 0;

        hr = pEventObj->Invoke(aDispid[i], IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &params, &varOut[i], NULL, NULL);
        if (hr != S_OK)
        {
            ATOMICRELEASE(pEventObj);
            return hr;
        }
    }

    ATOMICRELEASE(pEventObj);

    ASSERT(V_VT(&varOut[0]) == VT_I4);
    ASSERT(V_VT(&varOut[1]) == VT_I4);
    ASSERT(V_VT(&varOut[2]) == VT_I4);
    ASSERT(V_VT(&varOut[3]) == VT_BSTR);
    ASSERT(V_VT(&varOut[4]) == VT_BSTR);
    ASSERT(ARRAYSIZE(apchNames) == 5);

    hr = _pScriptErrList->AddNewErrorInfo(V_I4(&varOut[0]),         // line
                                          V_I4(&varOut[1]),       // char
                                          V_I4(&varOut[2]),       // code
                                          V_BSTR(&varOut[3]),     // message
                                          V_BSTR(&varOut[4]));    // url

    return hr;
}

//
// CScriptErrorList manages an array of _CScriptErrInfo objects
// the script error handler dialogs access this information
// when requested by the user
//

CScriptErrorList::CScriptErrorList() : CImpIDispatch(&IID_IScriptErrorList)
{
    ASSERT(_lDispIndex == 0);

    _ulRefCount = 1;

    _hdpa = DPA_Create(4);
}

CScriptErrorList::~CScriptErrorList()
{
    if (_hdpa != NULL)
    {
        ClearErrorList();
        DPA_Destroy(_hdpa);
    }
}

HRESULT
CScriptErrorList::AddNewErrorInfo(LONG lLine,
                                  LONG lChar,
                                  LONG lCode,
                                  BSTR strMsg,
                                  BSTR strUrl)
{
    HRESULT             hr;
    _CScriptErrInfo *   pNewData;

    if (strMsg == NULL || strUrl == NULL)
    {
        return E_INVALIDARG;
    }

    pNewData = new _CScriptErrInfo;
    if (pNewData != NULL)
    {
        hr = pNewData->Init(lLine, lChar, lCode, strMsg, strUrl);
        if (SUCCEEDED(hr))
        {
            if (_hdpa != NULL)
            {
                DPA_AppendPtr(_hdpa, (LPVOID)pNewData);
                _lDispIndex = DPA_GetPtrCount(_hdpa)-1;
            }
            else
            {
                hr = E_FAIL;
            }
        }
        else
        {
            delete pNewData;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

void
CScriptErrorList::ClearErrorList()
{
    if (_hdpa != NULL)
    {
        int iDel;
        int cPtr;

        cPtr = DPA_GetPtrCount(_hdpa);

        // delete from end to beginning to avoid unnecessary packing
        for (iDel = cPtr-1; iDel >= 0; iDel--)
        {
            delete ((_CScriptErrInfo *)DPA_GetPtr(_hdpa, iDel));
            DPA_DeletePtr(_hdpa, iDel);
        }

        _lDispIndex = 0;
    }
}

STDMETHODIMP
CScriptErrorList::QueryInterface(REFIID iid, void ** ppObj)
{
    ASSERT(ppObj != NULL);

    if (IsEqualIID(iid, IID_IUnknown) ||
        IsEqualIID(iid, IID_IDispatch) ||
        IsEqualIID(iid, IID_IScriptErrorList))
    {
        *ppObj = (IScriptErrorList *)this;
    }
    else
    {
        *ppObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();

    return S_OK;
}

STDMETHODIMP_(ULONG)
CScriptErrorList::AddRef()
{
    _ulRefCount++;
    return _ulRefCount;
}

STDMETHODIMP_(ULONG)
CScriptErrorList::Release()
{
    _ulRefCount--;
    if (_ulRefCount > 0)
    {
        return _ulRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
CScriptErrorList::advanceError()
{
    HRESULT hr;

    hr = E_FAIL;

    if (_hdpa != NULL)
    {
        int cPtr;

        cPtr = DPA_GetPtrCount(_hdpa);

        if (_lDispIndex < cPtr-1)
        {
            _lDispIndex++;
            hr = S_OK;
        }
    }

    return hr;
}

STDMETHODIMP
CScriptErrorList::retreatError()
{
    if (_lDispIndex < 1)
    {
        return E_FAIL;
    }

    _lDispIndex--;

    return S_OK;
}

STDMETHODIMP
CScriptErrorList::canAdvanceError(BOOL * pfCanAdvance)
{
    HRESULT hr;

    ASSERT(pfCanAdvance != NULL);

    hr = E_FAIL;

    if (_hdpa != NULL)
    {
        int cPtr;

        cPtr = DPA_GetPtrCount(_hdpa);
        *pfCanAdvance = _lDispIndex < cPtr-1;

        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP
CScriptErrorList::canRetreatError(BOOL * pfCanRetreat)
{
    ASSERT(pfCanRetreat != NULL);

    *pfCanRetreat = _lDispIndex > 0;

    return S_OK;
}

STDMETHODIMP
CScriptErrorList::getErrorLine(LONG * plLine)
{
    HRESULT hr;

    ASSERT(plLine != NULL);
    ASSERT(_lDispIndex >= 0);

    hr = E_FAIL;
    if (_hdpa != NULL)
    {
        int cPtr;

        cPtr = DPA_GetPtrCount(_hdpa);

        ASSERT(_lDispIndex < cPtr || _lDispIndex == 0);

        if (cPtr > 0)
        {
            _CScriptErrInfo *    pInfo;

            pInfo = (_CScriptErrInfo *)DPA_GetPtr(_hdpa, _lDispIndex);
            *plLine = pInfo->_lLine;
            hr = S_OK;
        }
    }

    return hr;
}

STDMETHODIMP
CScriptErrorList::getErrorChar(LONG * plChar)
{
    HRESULT hr;

    ASSERT(plChar != NULL);
    ASSERT(_lDispIndex >= 0);

    hr = E_FAIL;
    if (_hdpa != NULL)
    {
        int cPtr;

        cPtr = DPA_GetPtrCount(_hdpa);

        ASSERT(_lDispIndex < cPtr || _lDispIndex == 0);

        if (cPtr > 0)
        {
            _CScriptErrInfo *   pInfo;

            pInfo = (_CScriptErrInfo *)DPA_GetPtr(_hdpa, _lDispIndex);
            *plChar  = pInfo->_lChar;
            hr = S_OK;
        }
    }

    return hr;
}

STDMETHODIMP
CScriptErrorList::getErrorCode(LONG * plCode)
{
    HRESULT hr;

    ASSERT(plCode != NULL);
    ASSERT(_lDispIndex >= 0);

    hr = E_FAIL;
    if (_hdpa != NULL)
    {
        int cPtr;

        cPtr = DPA_GetPtrCount(_hdpa);

        ASSERT(_lDispIndex < cPtr || _lDispIndex == 0);

        if (cPtr > 0)
        {
            _CScriptErrInfo *   pInfo;

            pInfo = (_CScriptErrInfo *)DPA_GetPtr(_hdpa, _lDispIndex);
            *plCode = pInfo->_lCode;
            hr = S_OK;
        }
    }

    return hr;
}

STDMETHODIMP
CScriptErrorList::getErrorMsg(BSTR * pstrMsg)
{
    HRESULT hr;

    ASSERT(pstrMsg != NULL);
    ASSERT(_lDispIndex >= 0);

    hr = E_FAIL;
    if (_hdpa != NULL)
    {
        int cPtr;

        cPtr = DPA_GetPtrCount(_hdpa);

        ASSERT(_lDispIndex < cPtr || _lDispIndex == 0);

        if (cPtr > 0)
        {
            _CScriptErrInfo *   pInfo;

            pInfo = (_CScriptErrInfo *)DPA_GetPtr(_hdpa, _lDispIndex);
            *pstrMsg = SysAllocString(pInfo->_strMsg);

            if (*pstrMsg != NULL)
            {
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}

STDMETHODIMP
CScriptErrorList::getErrorUrl(BSTR * pstrUrl)
{
    HRESULT hr;

    ASSERT(pstrUrl != NULL);
    ASSERT(_lDispIndex >= 0);

    hr = E_FAIL;
    if (_hdpa != NULL)
    {
        int     cPtr;

        cPtr = DPA_GetPtrCount(_hdpa);

        ASSERT(_lDispIndex < cPtr || _lDispIndex == 0);

        if (cPtr > 0)
        {
            _CScriptErrInfo *   pInfo;

            pInfo = (_CScriptErrInfo *)DPA_GetPtr(_hdpa, _lDispIndex);
            *pstrUrl = SysAllocString(pInfo->_strUrl);

            if (*pstrUrl != NULL)
            {
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}

STDMETHODIMP
CScriptErrorList::getAlwaysShowLockState(BOOL * pfAlwaysShowLocked)
{
    *pfAlwaysShowLocked = IsInetcplRestricted(TEXT("Advanced"));

    return S_OK;
}

STDMETHODIMP
CScriptErrorList::getDetailsPaneOpen(BOOL * pfDetailsPaneOpen)
{
    *pfDetailsPaneOpen =
        SHRegGetBoolUSValue(szRegKey_SMIEM,
                            szRegVal_ErrDlgDetailsOpen,
                            FALSE,
                            FALSE);
    return S_OK;
}

STDMETHODIMP
CScriptErrorList::setDetailsPaneOpen(BOOL fDetailsPaneOpen)
{
    TCHAR   szYes[] = TEXT("yes");
    TCHAR   szNo[] = TEXT("no");
    LPTSTR  pszVal;
    int     cbSize;

    if (fDetailsPaneOpen)
    {
        pszVal = szYes;
        cbSize = sizeof(szYes);
    }
    else
    {
        pszVal = szNo;
        cbSize = sizeof(szNo);
    }

    SHRegSetUSValue(szRegKey_SMIEM,
                    szRegVal_ErrDlgDetailsOpen,
                    REG_SZ,
                    pszVal,
                    cbSize,
                    SHREGSET_HKCU | SHREGSET_FORCE_HKCU);

    // even if it failed, we can't do anything about it...
    return S_OK;
}

STDMETHODIMP
CScriptErrorList::getPerErrorDisplay(BOOL * pfPerErrorDisplay)
{
    *pfPerErrorDisplay =
        SHRegGetBoolUSValue(szRegKey_SMIEM,
                            szRegVal_ErrDlgPerErr,
                            FALSE,
                            FALSE);
    return S_OK;
}

STDMETHODIMP
CScriptErrorList::setPerErrorDisplay(BOOL fPerErrorDisplay)
{
    TCHAR   szYes[] = TEXT("yes");
    TCHAR   szNo[] = TEXT("no");
    LPTSTR  pszVal;
    int     cbSize;

    if (fPerErrorDisplay)
    {
        pszVal = szYes;
        cbSize = sizeof(szYes);
    }
    else
    {
        pszVal = szNo;
        cbSize = sizeof(szNo);
    }

    SHRegSetUSValue(szRegKey_SMIEM,
                    szRegVal_ErrDlgPerErr,
                    REG_SZ,
                    pszVal,
                    cbSize,
                    SHREGSET_HKCU | SHREGSET_FORCE_HKCU);

    // even if it failed, we can't do anything about it...
    return S_OK;
}

HRESULT
CScriptErrorList::_CScriptErrInfo::Init(LONG lLine,
                                        LONG lChar,
                                        LONG lCode,
                                        BSTR strMsg,
                                        BSTR strUrl)
{
    ASSERT(_strMsg == NULL);
    ASSERT(_strUrl == NULL);

    _strMsg = SysAllocString(strMsg);
    if (_strMsg == NULL)
    {
        return E_OUTOFMEMORY;
    }

    _strUrl = SysAllocString(strUrl);
    if (_strUrl == NULL)
    {
        SysFreeString(_strMsg);
        return E_OUTOFMEMORY;
    }

    _lLine = lLine;
    _lChar = lChar;
    _lCode = lCode;

    return S_OK;
}

CScriptErrorList::_CScriptErrInfo::~_CScriptErrInfo()
{
    if (_strMsg != NULL)
    {
        SysFreeString(_strMsg);
    }
    if (_strUrl != NULL)
    {
        SysFreeString(_strUrl);
    }
}
