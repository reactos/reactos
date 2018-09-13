#include "priv.h"

#include "apithk.h"
#include "sccls.h"
#include "shbrows2.h"
#include "commonsb.h"
#include "resource.h"
#include <fsmenu.h>
#include "bindcb.h"
#include "explore2.h"
#include <shdguid.h>
#include <isguids.h>
#include "desktop.h"
#include <ntverp.h>
#include "bands.h"
#include "browbar.h"
#include "itbdrop.h"
#include "inpobj.h"
#include "theater.h"
#include "itbar.h"
#include "idispids.h"
#include "bsmenu.h"
#include "menuband.h"
#include "dbgmem.h"
#include "mmhelper.h"
#include "mshtmcid.h"
#include <desktray.h>   // IDeskTray
#include "commonsb.h"
#include "onetree.h"
#include "cnctnpt.h"
#include "comcatex.h"
#include "util.h"
#include "uemapp.h"
#include <shlobjp.h>
#include <subsmgr.h>
#include "trayp.h"
#include "oleacc.h"
// BUGBUG (lamadio): Conflicts with one defined in winuserp.h
#undef WINEVENT_VALID       //It's tripping on this...
#include "winable.h"
#include <htmlhelp.h>


#ifdef UNIX
#include <mainwin.h>
#include "unixstuff.h"
EXTERN_C void unixGetWininetCacheLockStatus(BOOL *pBool, char **ppsz);
EXTERN_C const GUID CLSID_MsgBand;
#endif /* UNIX */

#include "mluisupp.h"

#define CWM_THEATERMODE                 (WM_USER + 400)
#define CWM_UPDATEBACKFORWARDSTATE      (WM_USER + 401)

#define SUPERCLASS CCommonBrowser

HRESULT IUnknown_GetClientDB(IUnknown *punk, IUnknown **ppdbc);

// Timer IDs
#define SHBTIMER_MENUSELECT     100

#define MENUSELECT_TIME         500     // .5 seconds for the menuselect delay

// Command group for private communication with CITBar
// 67077B95-4F9D-11D0-B884-00AA00B60104
const GUID CGID_PrivCITCommands = { 0x67077B95L, 0x4F9D, 0x11D0, 0xB8, 0x84, 0x00, 0xAA, 0x00, 0xB6, 0x01, 0x04 };
// Guid of Office's discussion band
// {BDEADE7F-C265-11d0-BCED-00A0C90AB50F}
EXTERN_C const GUID CLSID_DiscussionBand = { 0xbdeade7fL, 0xc265, 0x11d0, 0xbc, 0xed, 0x00, 0xa0, 0xc9, 0x0a, 0xb5, 0x0f };
// Guid of the Tip of the Day
//{4D5C8C25-D075-11d0-B416-00C04FB90376}
const GUID CLSID_TipOfTheDay =    { 0x4d5c8c25L, 0xd075, 0x11d0, 0xb4, 0x16, 0x00, 0xc0, 0x4f, 0xb9, 0x03, 0x76 };

// Used to see if the discussion band is registered for the CATID_CommBand
const LPCTSTR c_szDiscussionBandReg = TEXT("CLSID\\{BDEADE7F-C265-11d0-BCED-00A0C90AB50F}\\Implemented Categories\\{00021494-0000-0000-C000-000000000046}");


// BUGBUG: 17-May-1997  [ralphw] Once HTML Help supports window definitions,
//      the ">iedefault" should be removed

const TCHAR c_szHtmlHelpFile[]  = TEXT("%SYSTEMROOT%\\Help\\iexplore.chm>iedefault");

// Increment this when the saved structure changes
const WORD c_wVersion = 0x8002;

// This value will be initialized to 0 only when we are under IExplorer.exe
UINT g_tidParking = 0;

#define MAX_NUM_ZONES_ICONS         12
#define MAX_ZONE_DISPLAYNAME        260
extern PZONEICONNAMECACHE g_pZoneIconNameCache ; // now in commonbs.cpp

UINT_PTR g_sysmenuTimer = 0;

void ITBar_ShowDW(IDockingWindow * pdw, BOOL fTools, BOOL fAddress, BOOL fLinks);
void RestrictItbarViewMenu(HMENU hmenu, IUnknown *punkBar );
BOOL IsExplorerWindow(HWND hwnd);
void _SetWindowIcon(HWND hwnd, HICON hIcon, BOOL bLarge);
extern "C" void ShowJavaConsole(void);

//
// A named mutex is being used to determine if a critical operation exist, such as a file download.
// When we detect this we can prevent things like going offline while a download is in progress.
// To start the operation Create the named mutex. When the op is complete, close the handle.
// To see if any pending operations are in progress, Open the named mutex. Success/fail will indicate
// if any pending operations exist.  This mechanism is being used to determine if a file download is
// in progress when the user attempts to go offline.  If so, we prompt them to let them know that going 
// offline will cancel the download(s).
HANDLE g_hCritOpMutex = NULL;
const LPCSTR c_szCritOpMutexName = "CritOpMutex";
#define StartCriticalOperation()     ((g_hCritOpMutex = CreateMutexA(NULL, TRUE, c_szCritOpMutexName)) != (HANDLE)NULL)
#define EndCriticalOperation()       (CloseHandle(g_hCritOpMutex))
#define IsCriticalOperationPending() (((g_hCritOpMutex = OpenMutexA(MUTEX_ALL_ACCESS, TRUE, c_szCritOpMutexName)) != (HANDLE)NULL) && CloseHandle(g_hCritOpMutex))


#define MAX_FILECONTEXT_STRING (40)

#define VALIDATEPENDINGSTATE() ASSERT((_pbbd->_psvPending && _pbbd->_psfPending) || (!_pbbd->_psvPending && !_pbbd->_psfPending))

#define DM_NAV              TF_SHDNAVIGATE
#define DM_ZONE             TF_SHDNAVIGATE
#define DM_IEDDE            TF_SHDAUTO
#define DM_CANCELMODE       0
#define DM_UIWINDOW         0
#define DM_ENABLEMODELESS   0
#define DM_EXPLORERMENU     0
#define DM_BACKFORWARD      0
#define DM_PROTOCOL         0
#define DM_ITBAR            0
#define DM_STARTUP          0
#define DM_AUTOLIFE         0
#define DM_PALETTE          0
#define DM_SESSIONCOUNT     0
#define DM_FOCUS            0
#define DM_PREMERGEDMENU    DM_TRACE
#define DM_ONSIZE           DM_TRACE
#define DM_SSL              0
#define DM_SHUTDOWN         DM_TRACE
#define DM_MISC             0    // misc/tmp

extern IDeskTray * g_pdtray;
#define ISRECT_EQUAL(rc1, rc2) (((rc1).top == (rc2).top) && ((rc1).bottom == (rc2).bottom) && ((rc1).left == (rc2).left) && ((rc1).right == (rc2).right))

BOOL ViewIDFromViewMode(UINT uViewMode, SHELLVIEWID *pvid);

typedef struct _NAVREQUEST
{
    int cbNavData;
    BYTE *lpNavData;
    struct _NAVREQUEST *pnext;
} NAVREQUEST;

// copied from explore/cabwnd.h
#define MH_POPUP        0x0010
#define MH_TOOLBAR      0x0020

#define TBOFFSET_NONE   50
#define TBOFFSET_STD    0
#define TBOFFSET_HIST   1
#define TBOFFSET_VIEW   2

extern DWORD g_dwStopWatchMode;  // Shell performance mode


// Suite Apps Registry keys duplicated from dochost.cpp
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


#define SHELLBROWSER_FSNOTIFY_FLAGS (SHCNE_DRIVEADDGUI | SHCNE_SERVERDISCONNECT |     \
                                     SHCNE_MEDIAREMOVED | SHCNE_RMDIR |               \
                                     SHCNE_UPDATEDIR | SHCNE_NETUNSHARE |             \
                                     SHCNE_DRIVEREMOVED | SHCNE_UPDATEITEM |          \
                                     SHCNE_RENAMEFOLDER | SHCNE_UPDATEIMAGE |         \
                                     SHCNE_MEDIAINSERTED | SHCNE_DRIVEADD)

#define FAV_FSNOTIFY_FLAGS          (SHCNE_DISKEVENTS | SHCNE_UPDATEIMAGE)

#define GOMENU_RECENT_ITEMS         15
//
// Prototypes for "reset web settings" code
//
extern "C" HRESULT ResetWebSettings(HWND hwnd, BOOL *pfHomePageChanged);
extern "C" BOOL IsResetWebSettingsRequired(void);

const TCHAR c_szMenuItemCust[]      = TEXT("Software\\Policies\\Microsoft\\Internet Explorer");
const TCHAR c_szWindowUpdateName[]  = TEXT("Windows Update Menu Text");



#pragma warning(disable:4355)  // using 'this' in constructor

void CShellBrowser2::_PruneGoSubmenu(HMENU hmenu)
{
    // get by position since SHGetMenuFromID does a DFS and we are interested
    // in the one that is a direct child of hmenu and not some random menu
    // elsewhere in the hierarchy who might happen to have the same ID.

    int iPos = SHMenuIndexFromID(hmenu, FCIDM_MENU_EXPLORE);
    MENUITEMINFO mii;
    mii.cbSize = SIZEOF(mii);
    mii.fMask = MIIM_SUBMENU;

    if (iPos >= 0 && GetMenuItemInfo(hmenu, iPos, TRUE, &mii) && mii.hSubMenu) {
        HMENU hmenuGo = mii.hSubMenu;

        // Remove everything after the first separator

        MENUITEMINFO mii;
        int iItem = 0;

        while (TRUE) {

            TCHAR szTmp[100];
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_TYPE;
            mii.dwTypeData = szTmp;
            mii.cch = ARRAYSIZE(szTmp);
        
            if (!GetMenuItemInfoWrap(hmenuGo, iItem++, TRUE, &mii))
                break;
        
            if (mii.fType == MFT_SEPARATOR) {
                // we must have hit the first seperator, delete the rest of the menu...
                for (int iDel = GetMenuItemCount(hmenuGo) - 1; iDel >= iItem; iDel--)
                    RemoveMenu(hmenuGo, iDel, MF_BYPOSITION);

                break;
            }
        }
    }
}

//
//  Okay, menus are weird because of the fifteen bazillion scenarios we
//  need to support.
//
//  There are several functions involved in menu editing.  _MenuTemplate,
//  and all the _OnXxxMenuPopup functions.
//
//  The job of _MenuTemplate is to do global menu munging.  These munges
//  once performed are permanent, so don't munge anything that changes
//  based on some random ambient condition.  The job of the _OnXxxMenuPopup
//  functions is to do per-instance last-minute munging.
//
//  Also, _MenuTemplate is the only place you can add or remove top-level
//  menu items.
//
//  fShell = TRUE means that this menu will be used for shell objects.
//  fShell = FALSE means that this menu will be used for web objects.
//
//  Now the rules...
//
//  NT5:
//      Tools present.
//      Shell: "Folder Options" on Tools (not View).
//      Web: "Internet Options" on Tools (not View).
//      FTP: "Internet Options" & "Folder Options" on Tools (not View).
//      Go under View (not top-level).
//
//  Non-NT5, fShell = TRUE, IsCShellBrowser() = TRUE (Single-pane)
//      Tools removed.
//      Shell: "Folder Options" on View (not Tools).
//      Web: "Internet Options" on View (not Tools).
//      FTP: "Internet Options" & "Folder Options" on View (not Tools).
//      Go on top-level (not under View).
//
//  Non-NT5, fShell = TRUE, IsCShellBrowser() = FALSE (Dual-pane)
//      Tools present.
//      Shell: "Folder Options" on View (not Tools).
//      Web: "Internet Options" on View (not Tools).
//      FTP: "Internet Options" & "Folder Options" on View (not Tools).
//      Go on top-level (not under View).
//
//  Non-NT5, fShell = FALSE, viewing web page:
//      Tools present.
//      Shell: "Folder Options" on Tools (not View).
//      Web: "Internet Options" on Tools (not View).
//      FTP: "Internet Options" & "Folder Options" on Tools (not View).
//      Go under View (not top-level).
//
//  Bonus details:
//      Restrictions.
//      Shell Options disabled if browser-only.
//

HMENU CShellBrowser2::_MenuTemplate(int id, BOOL fShell)
{
    HMENU hmenu = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(id));
    if (hmenu)
    {
        //
        //  According to the chart, there is only one scenario where
        //  we need to nuke the Tools menu:  Non-NT5 shell single-pane
        //
        if (IsCShellBrowser2() && fShell && GetUIVersion() < 5)
            DeleteMenu(hmenu, FCIDM_MENU_TOOLS, MF_BYCOMMAND);

        //
        //  According to the chart, Go vanishes from top-level on NT5
        //  and on non-shell scenarios.  It also vanishes if restricted.
        //
        if (GetUIVersion() >= 5 || !fShell || SHRestricted(REST_CLASSICSHELL)) {
            // get by position since DeleteMenu does a DFS & there are dup FCIDM_MENU_EXPLORE's
            int iPos = SHMenuIndexFromID(hmenu, FCIDM_MENU_EXPLORE);

            if (iPos >= 0)
                DeleteMenu(hmenu, iPos, MF_BYPOSITION);

        }

        // Nuke file menu if restricted
        if (SHRestricted(REST_NOFILEMENU))
            DeleteMenu(hmenu, FCIDM_MENU_FILE, MF_BYCOMMAND);

        // Nuke favorites menu if a rooted explorer or shell menu and classic shell is set
        // or if restricted
        if ((fShell && SHRestricted(REST_CLASSICSHELL)) 
            || SHRestricted2(REST_NoFavorites, NULL, 0))
            DeleteMenu(hmenu, FCIDM_MENU_FAVORITES, MF_BYCOMMAND);

        HMENU hmenuView = SHGetMenuFromID(hmenu, FCIDM_MENU_VIEW);
        if (hmenuView) {
            // Go appears in only one place, so this test is just
            // the reverse of the one that decided if Go stays
            // at top-level.
            if (fShell && GetUIVersion() < 5) {
                DeleteMenu(hmenuView, FCIDM_MENU_EXPLORE, MF_BYCOMMAND);
            }
        }

        // Folder Options requires integrated shell
        if (fShell && WhichPlatform() != PLATFORM_INTEGRATED)
        {
            if (hmenuView)
            {
                _EnableMenuItem(hmenuView, FCIDM_BROWSEROPTIONS, FALSE);
            }
            HMENU hmenuTools = SHGetMenuFromID(hmenu, FCIDM_MENU_TOOLS);
            if (hmenuTools)
            {
                _EnableMenuItem(hmenuTools, FCIDM_BROWSEROPTIONS, FALSE);
            }
        }
    }

    return hmenu;
}

// Determine if we need to add the Fortezza menu
// For perf reasons, do not call this function unless user is 
// browsing outside the local machine--- it will load WININET
bool NeedFortezzaMenu()
{
    static bool fChecked = false,
                fNeed = false;
    
    // Never show the Fortezza option when offline
    if (SHIsGlobalOffline())
        return false;
    else if (fChecked)
        return fNeed;
    else
    {
        fChecked = true;
        DWORD  fStatus  = 0;
        BOOL   fQuery   = InternetQueryFortezzaStatus(&fStatus, 0);
        return (fNeed = fQuery && (fStatus&FORTSTAT_INSTALLED));
    }
}

// Create and return the Fortezza menu
HMENU FortezzaMenu()
{
    HMENU hfm = NULL;

    static TCHAR  szLogInItem[32]   = TEXT(""), // Initialize to empty strings
                  szLogOutItem[32]  = TEXT(""), 
                  szChangeItem[32]  = TEXT("");
    static bool   fInit = false;

    if (!fInit)             // Load the strings only once
    {
        MLLoadString(IDS_FORTEZZA_LOGIN, szLogInItem, ARRAYSIZE(szLogInItem)-1);
        MLLoadString(IDS_FORTEZZA_LOGOUT, szLogOutItem, ARRAYSIZE(szLogOutItem)-1);
        MLLoadString(IDS_FORTEZZA_CHANGE, szChangeItem, ARRAYSIZE(szChangeItem)-1);
        fInit = true;
    }
    
    if (hfm = CreatePopupMenu())
    {
        AppendMenu(hfm, MF_STRING, FCIDM_FORTEZZA_LOGIN, szLogInItem);
        AppendMenu(hfm, MF_STRING, FCIDM_FORTEZZA_LOGOUT, szLogOutItem);
        AppendMenu(hfm, MF_STRING, FCIDM_FORTEZZA_CHANGE, szChangeItem);
    }
    return hfm;
}

// Configure the menu depending on card state
// This function is called only if Fortezza has been detected
void SetFortezzaMenu(HMENU hfm)
{
    if (hfm==NULL)
        return;

    DWORD fStatus = 0;
    if (InternetQueryFortezzaStatus(&fStatus, 0))
    {
        // If the query succeeds, the items are enabled depending
        // on whether the user is logged in to Fortezza.
        _EnableMenuItem(hfm, FCIDM_FORTEZZA_CHANGE, (fStatus&FORTSTAT_LOGGEDON) ? TRUE  : FALSE);
        _EnableMenuItem(hfm, FCIDM_FORTEZZA_LOGIN,  (fStatus&FORTSTAT_LOGGEDON) ? FALSE : TRUE);
        _EnableMenuItem(hfm, FCIDM_FORTEZZA_LOGOUT, (fStatus&FORTSTAT_LOGGEDON) ? TRUE  : FALSE);
    }
    else
    {
        // If the query fails, all items are grayed out.
        _EnableMenuItem(hfm, FCIDM_FORTEZZA_CHANGE, FALSE);
        _EnableMenuItem(hfm, FCIDM_FORTEZZA_LOGIN, FALSE);
        _EnableMenuItem(hfm, FCIDM_FORTEZZA_LOGOUT, FALSE);
    }
    return;
}

DWORD DoNetConnect(HWND hwnd)
{
    return (DWORD)SHStartNetConnectionDialog(NULL, NULL, RESOURCETYPE_DISK);
}

DWORD DoNetDisconnect(HWND hwnd)
{
    DWORD ret = WNetDisconnectDialog(NULL, RESOURCETYPE_DISK);

    SHChangeNotifyHandleEvents();       // flush any drive notifications

    TraceMsg(DM_TRACE, "shell:CNet - TRACE: DisconnectDialog returned (%lx)", ret);
    if (ret == WN_EXTENDED_ERROR)
    {
        // BUGBUG: is this still needed
        // There has been a bug with this returning this but then still
        // doing the disconnect.  For now lets bring up a message and then
        // still do the notify to have the shell attempt to cleanup.
        TCHAR szErrorMsg[MAX_PATH];  // should be big enough
        TCHAR szName[80];            // The name better not be any bigger.
        DWORD dwError;
        WNetGetLastError(&dwError, szErrorMsg, ARRAYSIZE(szErrorMsg),
                szName, ARRAYSIZE(szName));

        MLShellMessageBox(NULL,
               MAKEINTRESOURCE(IDS_NETERROR), MAKEINTRESOURCE(IDS_DISCONNECTERROR),
               MB_ICONHAND | MB_OK, dwError, szName, szErrorMsg);
    }

    // BUGBUG: deal with error, perhaps open a window on this drive
    return ret;
}



CShellBrowser2::CShellBrowser2() :
#ifdef NO_MARSHALLING
        _fDelayedClose(FALSE),
        _fOnIEThread(TRUE),
#endif
        _fStatusBar(TRUE),
        _fShowMenu(TRUE),
        _fValidComCatCache(FALSE),
        _fShowSynchronize(TRUE),
        _iSynchronizePos(-1),
        CSBSUPERCLASS(NULL)
{
    // warning: can't call SUPERCLASS until _Initialize has been called
    // (since that's what does the aggregation)

    ASSERT(IsEqualCLSID(_clsidThis, CLSID_NULL));
}
#pragma warning(default:4355)  // using 'this' in constructor

HRESULT CShellBrowser2::_Initialize(HWND hwnd, IUnknown *pauto)
{
    HRESULT hr;
    SHELLSTATE ss = {0};

    hr = SUPERCLASS::_Initialize(hwnd, pauto);
    if (SUCCEEDED(hr)) {
        SetTopBrowser();
        int i = _AllocToolbarItem();
        ASSERT(i == ITB_ITBAR);
        _GetToolbarItem(ITB_ITBAR)->fShow = TRUE;
        _put_itbLastFocus(ITB_VIEW);
        InitializeDownloadManager();
        _nTBTextRows = -1;
        
        SHGetSetSettings(&ss, SSF_MAPNETDRVBUTTON, FALSE);
        _fShowNetworkButtons = ss.fMapNetDrvBtn;

        // Initialize the base class transition site pointer.
        InitializeTransitionSite();

        // Invalidate icon cache in case non-IE browser took over .htm icons.
        IEInvalidateImageList();
        _UpdateRegFlags();
        
        _nMBIgnoreNextDeselect = RegisterWindowMessage(TEXT("CMBIgnoreNextDeselect"));

        _fShowFortezza = FALSE;
        _hfm = NULL;
    }

    return hr;
}

HRESULT CShellBrowser2_CreateInstance(HWND hwnd, void **ppsb)
{
    CShellBrowser2 *psb = new CShellBrowser2();
    if (psb)
    {
        HRESULT hr = psb->_Initialize(hwnd, NULL);      // aggregation, etc.
        if (FAILED(hr)) {
            ASSERT(0);    // shouldn't happen
            ATOMICRELEASE(psb);
        }
        *ppsb = (void *)psb;
        return hr;
    }
    return E_OUTOFMEMORY;
}

CShellBrowser2::~CShellBrowser2()
{
    _TheaterMode(FALSE, FALSE);
    
    // If automation was enabled, kill it now
    ATOMICRELEASE(_pbsmInfo);
    ATOMICRELEASE(_poctNsc);
    ATOMICRELEASE(_pcmNsc);
    ATOMICRELEASE(_pism);
    ATOMICRELEASE(_pizm);
    ATOMICRELEASE(_pcmSearch);
    ASSERT(0 == _punkMsgLoop);
    
    ILFree(_pidlLastHist);

    if (_hmenuPreMerged)
        DestroyMenu(_hmenuPreMerged);

    if (_hmenuTemplate)
        DestroyMenu(_hmenuTemplate);

    if (_hmenuFull)
        DestroyMenu(_hmenuFull);

    if (_hfm)
        DestroyMenu(_hfm);

    if (_lpPendingButtons)
        LocalFree(_lpPendingButtons);

    if (_lpButtons)
        LocalFree(_lpButtons);

    if (_hZoneIcon)
        DestroyIcon(_hZoneIcon);

    Str_SetPtr(&_pszSynchronizeText, NULL);

    TraceMsg(TF_SHDLIFE, "dtor CShellBrowser2 %x", this);
}

void CShellBrowser2::v_FillCabStateHeader(CABSH* pcabsh, FOLDERSETTINGS* pfs)
{
    WINDOWPLACEMENT wp;
    OLECMD rgCmds[3] = {0};

    LPTOOLBARITEM ptbi = _GetToolbarItem(ITB_ITBAR);
    if (ptbi)
    {
        rgCmds[0].cmdID = CITIDM_VIEWTOOLS;
        rgCmds[1].cmdID = CITIDM_VIEWADDRESS;
        rgCmds[2].cmdID = CITIDM_VIEWLINKS;

        IUnknown_QueryStatus(ptbi->ptbar, &CGID_PrivCITCommands, ARRAYSIZE(rgCmds), rgCmds, NULL);
    }

    pcabsh->wv.bStdButtons = BOOLIFY(rgCmds[0].cmdf);
    pcabsh->wv.bAddress = BOOLIFY(rgCmds[1].cmdf);
    pcabsh->wv.bLinks = BOOLIFY(rgCmds[2].cmdf);    
    pcabsh->wv.bStatusBar = _fStatusBar;

    wp.length = SIZEOF(WINDOWPLACEMENT);
    GetWindowPlacement(_pbbd->_hwnd, &wp);

    pcabsh->dwHotkey = (UINT)SendMessage(_pbbd->_hwnd, WM_GETHOTKEY, 0, 0);

    //
    // Now Lets convert all of this common stuff into a
    // non 16/32 bit dependant data structure, such that both
    // can us it.
    //
    pcabsh->dwSize = SIZEOF(*pcabsh);
    pcabsh->flags = wp.flags;

    // 99/05/26 #345915 vtan: Don't mess with this. It's BY DESIGN.
    // #169839 caused #345915. When a window is minimized and closed
    // it should NEVER be opened minimized. The code that was here
    // has now caused a one month period where persistence of window
    // placement can be with SW_SHOWMINIMIZED which will cause the
    // window to restore minimized. This will go away when the window
    // is next closed.

    if ((wp.showCmd == SW_SHOWMINIMIZED) || (wp.showCmd == SW_MINIMIZE))
        pcabsh->showCmd = SW_SHOWNORMAL;
    else
        pcabsh->showCmd = wp.showCmd;

    pcabsh->ptMinPosition.x = wp.ptMinPosition.x;
    pcabsh->ptMinPosition.y = wp.ptMinPosition.y;
    pcabsh->ptMaxPosition.x = wp.ptMaxPosition.x;
    pcabsh->ptMaxPosition.y = wp.ptMaxPosition.y;

    pcabsh->rcNormalPosition = *((RECTL*)&wp.rcNormalPosition);

    // Now the folder settings
    pcabsh->ViewMode = pfs->ViewMode;
    // NB Don't ever preserve the best-fit flag or the nosubfolders flag.
    pcabsh->fFlags = pfs->fFlags & ~FWF_NOSUBFOLDERS & ~FWF_BESTFITWINDOW;

    pcabsh->fMask = CABSHM_VERSION;
    pcabsh->dwVersionId = CABSH_VER;

}

BOOL CShellBrowser2::_GetVID(SHELLVIEWID *pvid)
{
    BOOL bGotVID = FALSE;

    if (_pbbd->_psv && pvid) {
        IShellView2 *psv2;
        
        if (SUCCEEDED(_pbbd->_psv->QueryInterface(IID_IShellView2,
                                           (void **)&psv2)))
        {
            if (NOERROR == psv2->GetView(pvid, SV2GV_CURRENTVIEW))
            {
                bGotVID = TRUE;
            }
        
           psv2->Release();
        }
    }
    return bGotVID;
    
}

HRESULT CShellBrowser2::SetAsDefFolderSettings()
{
    HRESULT hres;

    if (_pbbd->_psv) {
        SHELLVIEWID  vid;
        BOOL bGotVID = _GetVID(&vid);
        FOLDERSETTINGS fs;

        _pbbd->_psv->GetCurrentInfo(&fs);

        CABINETSTATE cabstate;
        GetCabState(&cabstate);

        if (cabstate.fNewWindowMode)
            g_dfs.bDefToolBarMulti = FALSE;
        else
            g_dfs.bDefToolBarSingle = FALSE;
        
        g_dfs.fFlags = fs.fFlags & ( FWF_AUTOARRANGE ); // choose the ones we case about
        g_dfs.uDefViewMode = fs.ViewMode;
        g_dfs.bDefStatusBar = _fStatusBar;
        
        g_dfs.bUseVID = bGotVID;
        if (bGotVID)
        {
            g_dfs.vid = vid;
        }
        else
        {
            ViewIDFromViewMode(g_dfs.uDefViewMode, &g_dfs.vid);
        }
        
        SaveDefaultFolderSettings(GFSS_SETASDEFAULT);

//  99/02/10 #226140 vtan: Get DefView to set default view

        IUnknown_Exec(_pbbd->_psv, &CGID_DefView, DVID_SETASDEFAULT, OLECMDEXECOPT_DODEFAULT, NULL, NULL);

        hres = S_OK;
    } else {
        hres = E_FAIL;
    }

    return hres;
}

//---------------------------------------------------------------------------
// Closing a cabinet window.
//
// save it's local view info in the directory it is looking at
//
// NOTE: this will fail on read only media like net or cdrom
//
// REVIEW: we may not want to save this info on removable media
// (but if we don't allow a switch to force this!)
//
void CShellBrowser2::_SaveState()
{
    CABINETSTATE cabstate;
    GetCabState(&cabstate);

    // Don't save any state info if restrictions are in place.

    // We are trying to give a way for automation scripts to run that do not
    // update the view state.  To handle this we say if the window is not visible
    // (the script can set or unset visibility) than do not save the state (unless
    // falways add...)
    // Notwithstanding the above comments, suppress updating view state if UI
    // was set by automation
    if (_fUISetByAutomation ||
        !cabstate.fSaveLocalView ||
        SHRestricted(REST_NOSAVESET) || !IsWindowVisible(_pbbd->_hwnd) || _ptheater)
        return;

    if (_pbbd->_psv)
    {
        WINDOWPLACEMENT     currentWindowPlacement;
        WINVIEW             winView;

        currentWindowPlacement.length = 0;

        // if these keys are down, save the current states
        if (IsCShellBrowser2()  &&
            GetAsyncKeyState(VK_CONTROL) < 0) 
        {
           SetAsDefFolderSettings();
        }

        // Now get the view information
        FOLDERSETTINGS fs;
        _pbbd->_psv->GetCurrentInfo(&fs);


        IStream* pstm = NULL;

        // 99/05/07 #291358 vtan: Temporary solution to a problem dating back to IE4 days.
        
        // Here's where the window frame state if saved. This also saves the FOLDERSETTINGS
        // view information. Ideally it's best to separate the two but it seems reasonable
        // to only save frame state if this is the initial navigation. Once navigated away
        // only save view information and preserve the current frame state by reading what's
        // there and copying it. If there's no frame state then use what the current frame
        // state is. It could be possible to write out an empty frame state but if this
        // state is roamed to a down-level platform it may cause unexpected results.

        if (_fSBWSaved)
        {
            IStream     *pIStream;
            CABSH       cabinetStateHeader;

            pIStream = v_GetViewStream(_pbbd->_pidlCur, STGM_READ, L"CabView");
            if (pIStream != NULL)
            {
                if (SUCCEEDED(_FillCabinetStateHeader(pIStream, &cabinetStateHeader)))
                {

                    // If an old frame state exists then save it and mark it as valid.

                    currentWindowPlacement.length = sizeof(currentWindowPlacement);
                    currentWindowPlacement.flags = cabinetStateHeader.flags;
                    currentWindowPlacement.showCmd = cabinetStateHeader.showCmd;
                    currentWindowPlacement.ptMinPosition = *(reinterpret_cast<POINT*>(&cabinetStateHeader.ptMinPosition));
                    currentWindowPlacement.ptMaxPosition = *(reinterpret_cast<POINT*>(&cabinetStateHeader.ptMaxPosition));
                    currentWindowPlacement.rcNormalPosition = *(reinterpret_cast<RECT*>(&cabinetStateHeader.rcNormalPosition));
                    winView = cabinetStateHeader.wv;
                }
                pIStream->Release();
            }
        }

        if (!(_fSBWSaved && _fWin95ViewState))
        {
            pstm = v_GetViewStream(_pbbd->_pidlCur, STGM_CREATE | STGM_WRITE, L"CabView");
            _fSBWSaved = TRUE;
        }
        if (pstm)
        {
            CABSH cabsh;
            SHELLVIEWID  vid;
            BOOL bGotVID = _GetVID(&vid);

            v_FillCabStateHeader(&cabsh, &fs);

            if (currentWindowPlacement.length == sizeof(currentWindowPlacement))
            {

                // If an old frame state exists then put it back over the current frame state.

                cabsh.flags = currentWindowPlacement.flags;
                cabsh.showCmd = currentWindowPlacement.showCmd;
                cabsh.ptMinPosition = *(reinterpret_cast<POINTL*>(&currentWindowPlacement.ptMinPosition));
                cabsh.ptMaxPosition = *(reinterpret_cast<POINTL*>(&currentWindowPlacement.ptMaxPosition));
                cabsh.rcNormalPosition = *(reinterpret_cast<RECTL*>(&currentWindowPlacement.rcNormalPosition));
                cabsh.wv = winView;
            }

            if (bGotVID)
            {
                cabsh.vid = vid;
                cabsh.fMask |= CABSHM_VIEWID;
            }

            cabsh.fMask |= CABSHM_REVCOUNT;
            cabsh.dwRevCount = _dwRevCount;     // save out the rev count of when we were opened
            
            //
            // First output the common non view specific information
            //
            pstm->Write(&cabsh, SIZEOF(cabsh), NULL);

            // And release it, which will commit it to disk..
            pstm->Release();

            // NOTE (toddb): The DefView view state is saved by the base class so we don't need
            // to explicitly save it here.  If you do it gets called twice which is wasted time.
            // Do not call _pbbd->_psv->SaveViewState(); from this function.
        }

#ifdef DEBUG
        if (g_dwPrototype & 0x00000010) {
            //
            // Save toolbars
            //
            pstm = v_GetViewStream(_pbbd->_pidlCur, STGM_CREATE | STGM_WRITE, L"Toolbars");
            if (pstm) {
                _SaveToolbars(pstm);
                pstm->Release();
            }
        }
#endif
    }
}

STDAPI_(LPITEMIDLIST) IEGetInternetRootID(void);

IStream *CShellBrowser2::v_GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCWSTR pwszName)
{
    IStream *pstm = NULL;
    LPITEMIDLIST pidlToFree = NULL;
    BOOL    bCabView = (0 == StrCmpIW( pwszName, L"CabView" ));
    
    if (((NULL == pidl) && IsEqualCLSID(_clsidThis, CLSID_InternetExplorer)) ||
        IsBrowserFrameOptionsPidlSet(pidl, BFO_BROWSER_PERSIST_SETTINGS))
    {
        //  If this is a child of the URL or we're looking at an unititialized IE frame,
        //  then save all in the IE stream
        pidlToFree = IEGetInternetRootID();
        pidl = pidlToFree;
    }
    else if (bCabView && _fNilViewStream)
    {
        // if we loaded cabview settings from the 'unknown pidl' view stream,
        // we've got to stick with it, whether or not we now have a pidl.
        pidl = NULL; 
    }
   
    if (pidl)
    {
        pstm = SHGetViewStream(pidl, grfMode, pwszName, REGSTR_KEY_STREAMMRU, REGVALUE_STREAMS);
    }
    else if (bCabView)
    {
        //  So we don't have a pidl for which to grab a stream, so we'll just
        //  make up a stream to cover the situation.   A hack no doubt, but before we 
        //  were handling this case by always loading from the IE stream. (doh!)
        
        //  Actually, the whole thing is busted because we're
        //  creating the window (and trying to restore the windowpos) as part of
        //  CoCreateInstance(), before the client can navigate the browser.
        pstm = OpenRegStream( HKEY_CURRENT_USER, 
                              REGSTR_PATH_EXPLORER TEXT("\\Streams\\<nil>"), 
                              TEXT("CabView"), 
                              grfMode );
        _fNilViewStream = TRUE; // cabview settings initialized from the 'unknown pidl' view stream.
    }
    ILFree(pidlToFree);
    return pstm;
}
 
HRESULT CShellBrowser2::_FillCabinetStateHeader (IStream *pIStream, CABSH *cabsh)

{
    HRESULT hResult;

    // Now read in the state from the stream file.
    // read the old header first.

    hResult = IStream_Read(pIStream, cabsh, SIZEOF(CABSHOLD));

    // Sanity test to make the structure is sane

    if (FAILED(hResult) || (cabsh->dwSize < SIZEOF(CABSHOLD)))
        hResult = E_OUTOFMEMORY;        // bogus but good enough

    // Read the remainder of the structure if we can.  If not, then
    // set the mask equal to zero so we don't get confused later.

    if (cabsh->dwSize < SIZEOF(CABSH) ||
        FAILED(IStream_Read(pIStream, ((LPBYTE)cabsh) + SIZEOF(CABSHOLD), SIZEOF(CABSH) - SIZEOF(CABSHOLD))))
    {
        cabsh->fMask = 0;
    }
    return(hResult);
}

BOOL CShellBrowser2::_ReadSettingsFromStream(IStream *pstm, IETHREADPARAM *piei)
{
    CABSH cabsh;
    BOOL fUpgradeToWebView = FALSE;
    bool bInvalidWindowPlacement;

    if (FAILED(_FillCabinetStateHeader(pstm, &cabsh)))
        return(FALSE);

    // Now extract the data and put it into appropriate structures

    // first the window placement info
    piei->wp.length = SIZEOF(piei->wp);
    piei->wp.flags = (UINT)cabsh.flags;
    piei->wp.showCmd = (UINT)cabsh.showCmd;
    
    ASSERT(SIZEOF(piei->wp.ptMinPosition) == SIZEOF(cabsh.ptMinPosition));
    piei->wp.ptMinPosition = *((LPPOINT)&cabsh.ptMinPosition);
    piei->wp.ptMaxPosition = *((LPPOINT)&cabsh.ptMaxPosition);

    ASSERT(SIZEOF(piei->wp.rcNormalPosition) == SIZEOF(cabsh.rcNormalPosition));
    piei->wp.rcNormalPosition = *((RECT*)&cabsh.rcNormalPosition);

    // Do some simple sanity checks to make sure that the returned
    // information appears to be reasonable and not random garbage
    // We want the Show command to be normal or minimize or maximize.
    // Only need one test as they are consectutive and start at zero
    // DON'T try to validate too much of the WINDOWPLACEMENT--
    // SetWindowPlacement does a much better job, especially in
    // multiple-monitor scenarios...

    // 99/03/09 #303300 vtan: Sanity check for zero/negative width or
    // height. SetWindowPlacement doesn't sanity check for this -
    // only for whether the rectangle left and top are in the visible
    // screen area. If this condition is detected then reset to default
    // and force DefView to best fit the window.

    {
        LONG    lWidth, lHeight;

        lWidth = piei->wp.rcNormalPosition.right - piei->wp.rcNormalPosition.left;
        lHeight = piei->wp.rcNormalPosition.bottom - piei->wp.rcNormalPosition.top;
        bInvalidWindowPlacement = ((lWidth <= 0) || (lHeight <= 0));
        if (bInvalidWindowPlacement)
            piei->wp.length = 0;
    }

    if (piei->wp.showCmd > SW_MAX)
        return FALSE;

    piei->fs.ViewMode = (UINT)cabsh.ViewMode;
    piei->fs.fFlags = (UINT)cabsh.fFlags;

    if (cabsh.fMask & CABSHM_VIEWID)
    {
        // There was code here to revert to large icon mode if fWin95Classic
        // mode was turned on. This is completely busted because fWin95Classic
        // just affects the DEFAULT view, not any PERSISTED view.
        piei->m_vidRestore = cabsh.vid;
        piei->m_dwViewPriority = VIEW_PRIORITY_CACHEHIT; // we have a cache hit!
    }

    // If there was a revcount, check if we've been overridden by
    // a subsequent "use these settings as the default for all future
    // windows".
    if (cabsh.fMask & CABSHM_REVCOUNT)
    {
        if (g_dfs.dwDefRevCount != cabsh.dwRevCount)
        {
            if (g_dfs.bUseVID)
            {
                piei->m_vidRestore = g_dfs.vid;
            }
            else
            {
                ViewIDFromViewMode(g_dfs.uDefViewMode, &(piei->m_vidRestore));
            }
            
            // cache was overridden by default, so use VIEW_PRIORITY_STALECACHEHIT
            piei->m_dwViewPriority = VIEW_PRIORITY_STALECACHEHIT; 

            piei->fs.ViewMode  = g_dfs.uDefViewMode;
            piei->fs.fFlags    = g_dfs.fFlags;
        }
    }

    _dwRevCount = g_dfs.dwDefRevCount;      // save this with the browser so we can save it out later

    if (!(cabsh.fMask & CABSHM_VERSION) || (cabsh.dwVersionId < CABSH_VER))
    {
        SHELLSTATE ss = {0};

        // old version of stream....

        SHGetSetSettings(&ss, SSF_WIN95CLASSIC, FALSE);

        // we have either a cache miss (or an older dwVersionId), or we are restricting to win95 mode,
        // so set the priority accordingly.
        piei->m_dwViewPriority = ss.fWin95Classic ? VIEW_PRIORITY_RESTRICTED : VIEW_PRIORITY_CACHEMISS; 
        
        if (ss.fWin95Classic)
        {
            // Hey, it's a Win95 CABSH structure and we're in Win95 mode,
            // so don't change the defaults!
            ViewIDFromViewMode(cabsh.ViewMode, &(piei->m_vidRestore));
        }
        else
        {
            // Upgrade scenario:
            //   My Computer in List should wind up in Web View/List
            //   C:\ in List should wind up in List
            // If this fails (C:\ winds up in Large Icon), we can try
            // to comment out this code altogether. Hopefully defview's
            // default view stuff will realize Web View should be
            // selected and My Computer will go to Web View instead
            // of staying in List.
            //
            piei->m_vidRestore = DFS_VID_Default;

            // Note: if we upgrade to web view, we better let the
            // view recalc window space or the window will be TOO SMALL
            fUpgradeToWebView = TRUE;
        }

        if ( cabsh.wv.bStdButtons ) // win95 called this bToolbar
        {
            // Win95 called bStdButtons bToolbar. IE4 separates this
            // into bAddress and bStdButtons. Set bAddress for upgrade.
            cabsh.wv.bAddress = TRUE;

#define RECT_YADJUST    18
            // bump up the rect slightly to account for new toolbar size....
            // 18 is an approximately random number which assumes that the default
            // configuration is a single height toolbar which is approx twice as high as the
            // old toobar....
            //
            // NOTE: old browser streams are always for the primary monitor, so we just
            //       check to see if we are going to fit on the screen. If not, then don't bother.
            //
            // NOTE: when we rev the version number, we'll want to do this
            //       rect adjustment for the CABSH_WIN95_VER version...
            //
            int iMaxYSize = GetSystemMetrics( SM_CYFULLSCREEN );
            if ( piei->wp.rcNormalPosition.bottom + piei->wp.rcNormalPosition.top + RECT_YADJUST < iMaxYSize )
            {
                piei->wp.rcNormalPosition.bottom += RECT_YADJUST;
            }
#undef RECT_YADJUST
        }
    }

    // After all that upgrade work, check the classic shell restriction
    if (SHRestricted(REST_CLASSICSHELL))
    {
        // It doesn't matter what vid was specified, use the ViewMode
        ViewIDFromViewMode(cabsh.ViewMode, &(piei->m_vidRestore));
        piei->m_dwViewPriority = VIEW_PRIORITY_RESTRICTED; // use highest priority because of the restriction.

        // Oops, we can't upgrade...
        fUpgradeToWebView = FALSE;
    }

    // And the Hotkey
    piei->wHotkey = (UINT)cabsh.dwHotkey;

    piei->wv = cabsh.wv;

    // if we upgraded to web view, then any persisted window sizes will
    // probably be too small -- let them get resized by the view...
    if (fUpgradeToWebView || bInvalidWindowPlacement)
        piei->fs.fFlags |= FWF_BESTFITWINDOW;
    else
        piei->fs.fFlags &= ~FWF_BESTFITWINDOW;
    
    return TRUE;
}

void CShellBrowser2::_FillIEThreadParam(LPCITEMIDLIST pidl, IETHREADPARAM *piei)
{
    IStream* pstm = v_GetViewStream(pidl, STGM_READ, L"CabView");

    if (GetSystemMetrics(SM_CLEANBOOT) || !pstm || !_ReadSettingsFromStream(pstm, piei))
        v_GetDefaultSettings(piei);

    ATOMICRELEASE(pstm);
}

void CShellBrowser2::_UpdateFolderSettings(LPCITEMIDLIST pidl)
{
    if (!_fWin95ViewState)
    {
        IETHREADPARAM iei;

        ZeroMemory(&iei, SIZEOF(iei));
    
        _FillIEThreadParam(pidl, &iei);
    
        _fsd._vidRestore = iei.m_vidRestore;
        _fsd._dwViewPriority = iei.m_dwViewPriority;
        _fsd._fs = iei.fs;
    }
    else if (_pbbd->_psv)
    {
        IShellView2     *pISV2;

        // 99/04/16 #323726 vtan: Make sure that both the FOLDERSETTINGS (in _fsd._fs)
        // and the VID (in _fsd.vidRestore) is set up properly for shdocvw to make a
        // decision. This fixes Win95 browse in single window mode inheriting the view
        // from the source of navigation.

        _pbbd->_psv->GetCurrentInfo(&_fsd._fs);
        if (SUCCEEDED(_pbbd->_psv->QueryInterface(IID_IShellView2, reinterpret_cast<void**>(&pISV2))))
        {
            if (SUCCEEDED(pISV2->GetView(&_fsd._vidRestore, SV2GV_CURRENTVIEW)))
                _fsd._dwViewPriority = VIEW_PRIORITY_INHERIT;
            else
                _fsd._dwViewPriority = VIEW_PRIORITY_DESPERATE;
            pISV2->Release();
        }
    }
}

void CShellBrowser2::_LoadBrowserWindowSettings(IETHREADPARAM *piei, LPCITEMIDLIST pidl)
{
    _FillIEThreadParam(pidl, piei);

    //Copy the two restore settings from piei to ShellBrowser.
    _fsd._vidRestore = piei->m_vidRestore;
    _fsd._dwViewPriority = piei->m_dwViewPriority;
    _fsd._fs = piei->fs;

    // Now that the ITBar has the menu on it, it must always be shown. We turn
    // on/off bands individually now...
    LPTOOLBARITEM ptbi = _GetToolbarItem(ITB_ITBAR);
    ptbi->fShow = TRUE;
        
    _fStatusBar = piei->wv.bStatusBar;

    // BUGBUG nt5 #173735: never allow VK_MENU to be our hot key
    if (piei->wHotkey != VK_MENU)
        SendMessage(_pbbd->_hwnd, WM_SETHOTKEY, piei->wHotkey, 0);

#ifdef DEBUG
    if (g_dwPrototype & 0x00000010) {
        //
        // Load toolbars
        //
        IStream* pstm = v_GetViewStream(pidl, STGM_READ, L"Toolbars");
        if (pstm) {
            _LoadToolbars(pstm);
            pstm->Release();
        }
    }
#endif
}

void CShellBrowser2::_UpdateChildWindowSize(void)
{
    if (!_fKioskMode) {
        if (_hwndStatus && _fStatusBar) {
            SendMessage(_hwndStatus, WM_SIZE, 0, 0L);
        }
    }
}


/*----------------------------------------------------------
Purpose: Helper function to do ShowDW on the internet toolbar.

         We can show all the bands in the toolbar, but we must
         never accidentally hide the menuband.  CShellBrowser2
         should call this function rather than IDockingWindow::ShowDW 
         directly if there is any chance fShow would be FALSE.

*/
void ITBar_ShowDW(IDockingWindow * pdw, BOOL fTools, BOOL fAddress, BOOL fLinks)
{
    IUnknown_Exec(pdw, &CGID_PrivCITCommands, CITIDM_SHOWTOOLS, fTools, NULL, NULL);
    IUnknown_Exec(pdw, &CGID_PrivCITCommands, CITIDM_SHOWADDRESS, fAddress, NULL, NULL);
    IUnknown_Exec(pdw, &CGID_PrivCITCommands, CITIDM_SHOWLINKS, fLinks, NULL, NULL);
}   

void CShellBrowser2::_HideToolbar(LPUNKNOWN punk)
{
    for (UINT itb=0; itb < (UINT)_GetToolbarCount(); itb++) {
        LPTOOLBARITEM ptbi = _GetToolbarItem(itb);

        if (ptbi->ptbar && SHIsSameObject(ptbi->ptbar, punk)) 
        {
            if (ITB_ITBAR == itb)
                ITBar_ShowDW(ptbi->ptbar, FALSE, FALSE, FALSE);
            else
                ptbi->ptbar->ShowDW(FALSE);
        }
    }
}

HRESULT CShellBrowser2::v_ShowHideChildWindows(BOOL fChildOnly)
{
    // BUGBUG (scotth): _hwndStatus is bogus when closing a window
    if (_hwndStatus && IS_VALID_HANDLE(_hwndStatus, WND))
        ShowWindow(_hwndStatus, (!_fKioskMode && _fStatusBar) ? SW_SHOW : SW_HIDE);

    Exec(NULL, OLECMDID_UPDATECOMMANDS, 0, NULL, NULL);
    _UpdateChildWindowSize();

    SUPERCLASS::v_ShowHideChildWindows(fChildOnly);

    // We should call _UpdateBackForwardState after the parent show/hide
    // toolbars. 
    UpdateBackForwardState();

    return S_OK;
}

#define MAX_BROWSER_WINDOW_TEMPLATE  (MAX_BROWSER_WINDOW_TITLE - 20)

void CShellBrowser2::v_GetAppTitleTemplate(LPTSTR pszBuffer, LPTSTR pszTitle)
{
    if (_fAppendIEToCaptionBar) 
    {
        TCHAR szBuffer[MAX_BROWSER_WINDOW_TEMPLATE];
        _GetAppTitle(szBuffer, ARRAYSIZE(szBuffer));
        wnsprintf(pszBuffer, 80/* Supid params*/, TEXT("%%s - %s"), szBuffer);
    } 
    else 
    {
        // don't tack on "intenet explorer" if we didn't start there
        StrCpy(pszBuffer, TEXT("%s"));
    }
}


/*----------------------------------------------------------
Purpose: Intercept messages for menuband.

         Menuband messages must be intercepted at two points:

         1) the main message pump (IsMenuMessage method)
         2) the wndproc of a window that has a menuband 
            (TranslateMenuMessage method)

         The reason is sometimes a message will be received
         by the wndproc which did not pass thru the apps main
         message pump, but must be dealt with.  There are other
         messages which must be handled in the main message 
         pump, before TranslateMessage or DispatchMessage.

Returns: TRUE if the message was handled

*/
HRESULT CShellBrowser2::v_MayTranslateAccelerator(MSG* pmsg)
{
    HRESULT hres = S_FALSE;
    
    // BUGBUG (scotth): for some unknown reason (aka ActiveX init), we are 
    // receiving a null hwnd with WM_DESTROY when the user scrolls a page
    // that causes the ticker control to appear.  Check the pmsg->hwnd here
    // so we don't mistake a rogue WM_DESTROY msg for the real thing.
    
    IMenuBand* pmb = _GetMenuBand(_pbbd->_hwnd == pmsg->hwnd && WM_DESTROY == pmsg->message);

    if (pmb && _fActivated)
    {
        hres = pmb->IsMenuMessage(pmsg);

        // don't need to release pmb
    }

    if (hres != S_OK)
    {
        // BUGBUG cleanup -- move menuband stuff & this check to v_MayTranslateAccelerator's caller
        if (WM_KEYFIRST <= pmsg->message && pmsg->message <= WM_KEYLAST)
        {
            hres = SUPERCLASS::v_MayTranslateAccelerator(pmsg);

            if (hres != S_OK)
            {
                //
                // Our SUPERCLASS didn't handle it.
                //
                if (_ShouldTranslateAccelerator(pmsg))
                {
                    //
                    // Okay, it's one of ours.  Let the toolbars have a crack at
                    // translating it.
                    //
                    for (UINT itb=0; (itb < (UINT)_GetToolbarCount()) && (hres != S_OK); itb++)
                    {
                        LPTOOLBARITEM ptbi = _GetToolbarItem(itb);

                        if (ptbi->fShow && (NULL != ptbi->ptbar))
                        {
                            IUnknown *pUnk;
                            
                            if (SUCCEEDED(IUnknown_GetClientDB(ptbi->ptbar, &pUnk)))
                            {
                                ASSERT(NULL != pUnk);
                                
                                hres = UnkTranslateAcceleratorIO(pUnk, pmsg);

                                pUnk->Release();
                            }
                        }
                    }
                }
            }
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: Take a whack at translating any messages.

         CShellBrowser2 uses this to translate messages needed
         for menu bands.

Returns: TRUE if we handled it

*/
BOOL CShellBrowser2::_TranslateMenuMessage(HWND hwnd, UINT uMsg, 
    WPARAM * pwParam, LPARAM * plParam, LRESULT * plRet)
{
    BOOL bRet = FALSE;
    IMenuBand* pmb = _GetMenuBand(WM_DESTROY == uMsg);

    if (pmb)
    {
        MSG msg;

        msg.hwnd = hwnd;
        msg.message = uMsg;
        msg.wParam = *pwParam;
        msg.lParam = *plParam;
        
        bRet = (S_OK == pmb->TranslateMenuMessage(&msg, plRet));

        *pwParam = msg.wParam;
        *plParam = msg.lParam;

        // don't need to release pmb
    }

    return bRet;
}    

static TCHAR g_szWorkingOffline[MAX_BROWSER_WINDOW_TEMPLATE]=TEXT("");
static TCHAR g_szWorkingOfflineTip[MAX_BROWSER_WINDOW_TEMPLATE]=TEXT("");
static TCHAR g_szAppName[MAX_BROWSER_WINDOW_TEMPLATE]=TEXT("");

void InitTitleStrings()
{
    if (!g_szWorkingOffline[0])
    {
        DWORD dwAppNameSize = SIZEOF(g_szAppName);
        
        // Load this stuff only once per process for perf.
        if (SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_MAIN, TEXT("Window Title"), NULL,
                            g_szAppName, &dwAppNameSize) != ERROR_SUCCESS)
            MLLoadString(IDS_TITLE, g_szAppName, ARRAYSIZE(g_szAppName));

        MLLoadString(IDS_WORKINGOFFLINETIP, g_szWorkingOfflineTip, ARRAYSIZE(g_szWorkingOfflineTip));
        MLLoadString(IDS_WORKINGOFFLINE, g_szWorkingOffline, ARRAYSIZE(g_szWorkingOffline));
        SHTruncateString(g_szWorkingOffline, ARRAYSIZE(g_szWorkingOffline) - (lstrlen(g_szAppName) + 4)); // give room for separator & EOL
    }
}

void CShellBrowser2::_ReloadTitle()
{
    g_szWorkingOffline[0] = 0;
    _fTitleSet = FALSE;
    _SetTitle(NULL);
}


HICON OfflineIcon()
{
    static HICON s_hiconOffline = NULL;
    if (!s_hiconOffline) 
    {
        s_hiconOffline = (HICON)LoadImage(HinstShdocvw(), MAKEINTRESOURCE(IDI_OFFLINE), IMAGE_ICON,
                             GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
        
    }
    return s_hiconOffline;
}


void CShellBrowser2::_ReloadStatusbarIcon()
{
    BOOL fIsOffline;
    VARIANTARG var = {0};
    var.vt = VT_I4;
    
    if (_pbbd && SUCCEEDED(IUnknown_Exec(_pbbd->_psv, &CGID_Explorer, SBCMDID_GETPANE, PANE_OFFLINE, NULL, &var)) &&
        (var.lVal != PANE_NONE))
    {    
        if (_pbbd->_pidlCur && IsBrowserFrameOptionsSet(_pbbd->_psf, BFO_USE_IE_OFFLINE_SUPPORT))
            fIsOffline = SHIsGlobalOffline();
        else
            fIsOffline = FALSE;
    
        SendControlMsg(FCW_STATUS, SB_SETICON, var.lVal, fIsOffline ? (LPARAM) OfflineIcon() : NULL, NULL);
        if (fIsOffline) 
        {
            InitTitleStrings();
            SendControlMsg(FCW_STATUS, SB_SETTIPTEXT, var.lVal, (LPARAM)g_szWorkingOfflineTip, NULL);
        }
    }
}

void CShellBrowser2::_GetAppTitle(LPTSTR pszBuffer, DWORD cchSize)
{
    BOOL fOffline = SHIsGlobalOffline();
    
    pszBuffer[0] = 0;

    InitTitleStrings();

    if (fOffline)
    {
        wnsprintf(pszBuffer, cchSize, TEXT("%s - %s"), g_szAppName, g_szWorkingOffline); 
    }
    else
    {
#ifdef DEBUG

#ifdef UNICODE
#define DLLNAME TEXT("(BrowseUI UNI)")
#else
#define DLLNAME TEXT("(BrowseUI)")
#endif

#ifndef UNIX
        wnsprintf(pszBuffer, cchSize, TEXT("%s - %s"), g_szAppName, DLLNAME); 
#else
        wnsprintf(pszBuffer, cchSize, TEXT("%s%s - %s"), g_szAppName, UNIX_TITLE_SUFFIX, DLLNAME); 
#endif

#else

#ifndef UNIX
        lstrcpyn(pszBuffer, g_szAppName, cchSize);
#else
        wnsprintf(pszBuffer, cchSize, TEXT("%s%s"), g_szAppName, UNIX_TITLE_SUFFIX); 
#endif

#endif
    }

}

HWND CShellBrowser2::_GetCaptionWindow()
{
    return _pbbd->_hwnd;
}


/*----------------------------------------------------------
Purpose: Gets the cached menu band.  If the menu band hasn't
         been acquired yet, attempt to get it.  If bDestroy
         is TRUE, the menu band will be released.

         This does not AddRef because an AddRef/Release for each
         message is not necessary -- as long as callers beware!
*/
IMenuBand* CShellBrowser2::_GetMenuBand(BOOL bDestroy)
{
    // Don't bother to create it if we're about to go away.
    if (_fReceivedDestroy)
    {
        ASSERT(NULL == _pmb);
    }
    else if (bDestroy)
    {
        ATOMICRELEASE(_pmb);

        // Make it so we don't re-create the _pmb after the WM_DESTROY
        _fReceivedDestroy = TRUE;
    }

    // The menuband is created sometime after WM_CREATE is sent.  Keep
    // trying to get the menuband interface until we get it.

    else if (!_pmb)
    {
        IBandSite *pbs;
        IUnknown_QueryService(_GetITBar(), IID_IBandSite, IID_IBandSite, (void **)&pbs);
        if (pbs) 
        {
            IDeskBand * pdbMenu;

            pbs->QueryBand(CBIDX_MENU, &pdbMenu, NULL, NULL, 0);
            if (pdbMenu)
            {
                pdbMenu->QueryInterface(IID_IMenuBand, (void **)&_pmb);
                // Cache _pmb, so don't release it here

                pdbMenu->Release();
            }
            pbs->Release();
        }
    }

    return _pmb;
}    


void CShellBrowser2::_SetMenu(HMENU hmenu)
{
    // Create a top-level menuband given this hmenu.  Add it to 
    // the bandsite.

    if (!_pmb) 
    {
        _GetMenuBand(FALSE);      // this does not AddRef

        if (!_pmb)
            return;
    }

    IShellMenu* psm;

    if (SUCCEEDED(_pmb->QueryInterface(IID_IShellMenu, (void**)&psm)))
    {
        HMENU hCurMenu = NULL;
        psm->GetMenu( &hCurMenu, NULL, NULL );

        // only call setmenu if we know it is not the menu we have currently or it is not one of our precached standards...
        if (( hmenu != hCurMenu ) || 
            ( hmenu != _hmenuFull && hmenu != _hmenuTemplate  && hmenu != _hmenuPreMerged ))
        {
            psm->SetMenu(hmenu, NULL, SMSET_DONTOWN | SMSET_MERGE);
        }
        psm->Release();
    }
}

STDAPI SHFlushClipboard(void);

HRESULT CShellBrowser2::OnDestroy()
{
    SUPERCLASS::OnDestroy();
    
    SHFlushClipboard();

    if (_uFSNotify)
        SHChangeNotifyDeregister(_uFSNotify);

    if (_fAutomation)
        IECleanUpAutomationObject();

    _DecrNetSessionCount();

    return S_OK;
}

BOOL CShellBrowser2::_CreateToolbar()
{
    return TRUE;
}

void CShellBrowser2::v_InitMembers()
{
    _hmenuTemplate =  _MenuTemplate(MENU_TEMPLATE, TRUE);
    _hmenuFull =      _MenuTemplate(MENU_FULL, TRUE);
    _hmenuPreMerged = _MenuTemplate(MENU_PREMERGED, FALSE);

    if (_fRunningInIexploreExe)
        _hmenuCur = _hmenuPreMerged;
    else
        _hmenuCur = _hmenuTemplate;
}


// REVIEW UNDONE - Stuff in programs defaults to save positions ???
void CShellBrowser2::v_GetDefaultSettings(IETHREADPARAM *piei)
{
    // set the flags

    // Best fit window means get the window to size according to the
    // contents of the view so that windows without existing settings
    // come up looking OK.
    piei->fs.fFlags = FWF_BESTFITWINDOW | g_dfs.fFlags;
    piei->wv.bStatusBar = g_dfs.bDefStatusBar;

    CABINETSTATE cabstate;
    GetCabState(&cabstate);
    if (cabstate.fSimpleDefault && cabstate.fNewWindowMode)
    {
        piei->wv.bStdButtons = piei->wv.bAddress = g_dfs.bDefToolBarMulti;
    }
    else
    {
        piei->wv.bStdButtons = piei->wv.bAddress = g_dfs.bDefToolBarSingle;
    }

    // For Win95 classic view, ITBar should be hidden by default.
    SHELLSTATE ss = {0};
    SHGetSetSettings(&ss, SSF_WIN95CLASSIC, FALSE);

    //  SHGetSetSettings checks SHRestricted(REST_CLASSICSHELL) for us
    if (ss.fWin95Classic)
    {
        piei->fs.ViewMode = FVM_ICON;
        piei->m_vidRestore = VID_LargeIcons;
        piei->m_dwViewPriority = VIEW_PRIORITY_RESTRICTED; // use highest priority because of the restriction.
    }
    else
    {
        piei->fs.ViewMode = g_dfs.uDefViewMode;
        piei->m_vidRestore = g_dfs.vid;
        // this function only gets called if _ReadSettingsFromStream fails (meaning its not in the cache),
        // so we treat this as a cache miss.
        piei->m_dwViewPriority = (g_dfs.bUseVID ? VIEW_PRIORITY_CACHEMISS : VIEW_PRIORITY_NONE);
    }

    _dwRevCount = g_dfs.dwDefRevCount;      // save this with the browser so we can save it out later

    ASSERT(piei->wp.length == 0);
}

void CShellBrowser2::_DecrNetSessionCount()
{
    TraceMsg(DM_SESSIONCOUNT, "_DecrNetSessionCount");

    if (_fVisitedNet) {
        SetQueryNetSessionCount(SESSION_DECREMENT);
        _fVisitedNet = FALSE;
    }
}

void CShellBrowser2::_IncrNetSessionCount()
{
    TraceMsg(DM_SESSIONCOUNT, "_IncrNetSessionCount");

    if (!_fVisitedNet) {
        BOOL fDontDoDefaultCheck = (BOOLIFY(_fAutomation) || (!(BOOLIFY(_fAddDialUpRef))));
        if (!SetQueryNetSessionCount(fDontDoDefaultCheck ? SESSION_INCREMENT_NODEFAULTBROWSERCHECK : SESSION_INCREMENT)) {
            g_szWorkingOffline[0] = 0;
#ifdef NO_MARSHALLING
            if (!_fOnIEThread)
                SetQueryNetSessionCount(fDontDoDefaultCheck ? SESSION_INCREMENT_NODEFAULTBROWSERCHECK : SESSION_INCREMENT);
#endif
      }
        _fVisitedNet = TRUE;
    }
}


// Initialize the Internet Toolbar. Create a dummy class to trap all the Messages that
// are sent to the old toolbar
BOOL  CShellBrowser2::_PrepareInternetToolbar(IETHREADPARAM* piei)
{
    HRESULT hresT;

    if (!_GetITBar())
    {
        DWORD dwServerType = CLSCTX_INPROC_SERVER;
#ifdef FULL_DEBUG
        if (!(g_dwPrototype & PF_NOBROWSEUI))
            /// this will cause us to use OLE's co-create intance and not short circuit it.
            dwServerType = CLSCTX_INPROC;
#endif
        HRESULT hresT = CoCreateInstance(CLSID_InternetToolbar, NULL,
                                         dwServerType,
                                         IID_IDockingWindow, (void **) &_GetToolbarItem(ITB_ITBAR)->ptbar);

        TraceMsg(DM_ITBAR|DM_STARTUP, "CSB::OnCreate CoCreate(CLS_ITBAR) returned %x", hresT);

        if (SUCCEEDED(hresT)) {
            IUnknown_SetSite(_GetITBar(), SAFECAST(this, IShellBrowser*));
            // Look at the type of folder using "pidlInitial" and 
            // see if we have a stream for this type.
            // If so, open it and call IPersistStreamInit::Load(pstm);
            // else, call IPersistStreamInit::InitNew(void);

            IPersistStreamInit  *pITbarPSI;

            //Get the pointer to 
            if (SUCCEEDED(_GetITBar()->QueryInterface(IID_IPersistStreamInit, (void **)&pITbarPSI)))
            {
                // The initial toolbar needs to be the Web toolbar
                IUnknown_Exec(pITbarPSI, &CGID_PrivCITCommands, CITIDM_ONINTERNET, (_fUseIEToolbar ? CITE_INTERNET : CITE_SHELL), NULL, NULL);

                IStream *pstm = _GetITBarStream(_fUseIEToolbar, STGM_READ);
                if (pstm)
                {
                    //Stream exists. Let's load it from there.
                    pITbarPSI->Load(pstm);
                    pstm->Release();
                }
                else
                {
                    //No stream already exists. Initialize from the old location!
                    pITbarPSI->InitNew();
                }

                pITbarPSI->Release();
            }

        } else {
            TraceMsg(DM_ERROR, "ief ER: OnCreate failed to create Shell Toolbar!!!");
            return FALSE;
        }
        SUPERCLASS::v_ShowHideChildWindows(TRUE);
        
        if (!_hwndDummyTB)
        {
            _hwndDummyTB = SHCreateWorkerWindow(DummyTBWndProc, _pbbd->_hwnd, 0, WS_CHILD, (HMENU)9999, this);
        }
    }

    if (!_pxtb)
        hresT = QueryService(SID_SExplorerToolbar, IID_IExplorerToolbar, (void **)&_pxtb);

    return TRUE;
}

BOOL LoadWindowPlacement(WINDOWPLACEMENT * pwndpl)
{
    BOOL fRetVal = FALSE;

    if (pwndpl)
    {
        DWORD dwSize = sizeof(WINDOWPLACEMENT);
        if (SHGetValueGoodBoot(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Explorer\\Main"),
                TEXT("Window_Placement"), NULL, (PBYTE)pwndpl, &dwSize) == ERROR_SUCCESS)
        {
           fRetVal = TRUE;
            // Is the default value invalid?
            if ((pwndpl->rcNormalPosition.left >= pwndpl->rcNormalPosition.right) ||
                (pwndpl->rcNormalPosition.top >= pwndpl->rcNormalPosition.bottom))
            {
                // Yes, so fix it.  We worry about the normal size being zero or negative.
                // This fixes the munged case. 
                ASSERT(0); // BUGBUG: the stream is corrupted.
                fRetVal = FALSE;
            }
        }
    }
    return fRetVal;
}


BOOL StoreWindowPlacement(WINDOWPLACEMENT *pwndpl)
{
    if (pwndpl)
    {
        // Don't store us as minimized - that isn't what the user intended.
        // I.E. right click on minimized IE 3.0 in tray, pick close.  Since
        // we are minmized in that scenario we want to force normal
        // instead so we at least show up.
    
        if (pwndpl->showCmd == SW_SHOWMINIMIZED ||
            pwndpl->showCmd == SW_MINIMIZE)
            pwndpl->showCmd = SW_SHOWNORMAL;

        // Are about to save a corrupted window size?
        if ((pwndpl->rcNormalPosition.left >= pwndpl->rcNormalPosition.right) ||
            (pwndpl->rcNormalPosition.top >= pwndpl->rcNormalPosition.bottom))
        {
            // Yes, so fix it.
            ASSERT(0); // BUGBUG: the size is invalid or corrupted.
        }
        else
        {
            return SHSetValue(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Explorer\\Main"),
                TEXT("Window_Placement"), REG_BINARY, (const BYTE *)pwndpl, sizeof(WINDOWPLACEMENT)) == ERROR_SUCCESS;
        }
    }
    return FALSE;
}



BOOL StorePlacementOfWindow(HWND hwnd)
{
    WINDOWPLACEMENT wndpl;
    wndpl.length = sizeof(WINDOWPLACEMENT);
    
    if (GetWindowPlacement(hwnd, &wndpl)) 
    {
        return StoreWindowPlacement(&wndpl);
    }
    return FALSE;
}



// forward declaration
void EnsureWindowIsCompletelyOnScreen (RECT *prc);



// The rect will be offset slightly below and to the right of its current position.
// If this would cause it to move partly off the nearest monitor, then it is 
// instead placed at the top left of the same monitor.

void CascadeWindowRect(RECT *pRect)
{
    int delta = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYSIZEFRAME) - 1;

    OffsetRect(pRect, delta, delta);
    
    // test if the new rect will end up getting moved later on
    RECT rc = *pRect;
    EnsureWindowIsCompletelyOnScreen(&rc);

    if (!EqualRect(pRect, &rc))
    {
        // rc had to be moved, so we'll restart the cascade using the best monitor
        MONITORINFO minfo;
        minfo.cbSize = sizeof(minfo);
        GetMonitorInfo(MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST), &minfo);

        // And we do mean rcMonitor, not rcWork.  For example, if the taskbar is
        // at the top, then using rcMonitor with top-left = (0,0) will place it on
        // the edge of the taskbar.  Using rcWork with top-left = (0,y) will put
        // it y pixels below the taskbar's bottom edge, which is wrong.

        if (rc.bottom < pRect->bottom && rc.left == pRect->left)
        {
            // Too tall to cascade further down, but we can keep the X and just
            // reset the Y.  This fixes the bug of having a tall windows piling up
            // on the top left corner -- instead they will be offset to the right
            OffsetRect(pRect, 0, minfo.rcMonitor.top - pRect->top);   
        }
        else
        {
            // we've really run out of room, so restart cascade at top left
            OffsetRect(pRect, 
                minfo.rcMonitor.left - pRect->left,
                minfo.rcMonitor.top - pRect->top);
        }
    }
}




void CalcWindowPlacement(BOOL fInternetStart, HWND hwnd, IETHREADPARAM *piei, WINDOWPLACEMENT *pwndpl)
{
    static RECT s_rcExplorer = {-1, -1, -1, -1};

    // We don't load the window placement for shell windows

    if (!fInternetStart || LoadWindowPlacement(pwndpl)) 
    {
        // If the show command specifies a normal show or default (i.e., our initial
        // display setting is not being overridden by the command line or
        // CreateProcess setting) then use the saved window state show command.
        // Otherwise, use the show command passed in to us.
        if (fInternetStart && ((piei->nCmdShow == SW_SHOWNORMAL) || (piei->nCmdShow == SW_SHOWDEFAULT)))
            piei->nCmdShow = pwndpl->showCmd;
        
        // Cascade if there is window of the same kind directly under us.

        HWND hwndT = NULL;
        ATOM atomClass = (ATOM) GetClassLong(hwnd, GCW_ATOM);

        while (hwndT = FindWindowEx(NULL, hwndT, (LPCTSTR) atomClass, NULL))
        {
            // Don't use GetWindowRect here because we load window placements
            // from the registry and they use the workspace coordinate system

            WINDOWPLACEMENT wp;
            wp.length = sizeof(wp);
            GetWindowPlacement(hwndT, &wp);        

            if (wp.rcNormalPosition.left == pwndpl->rcNormalPosition.left &&
                wp.rcNormalPosition.top == pwndpl->rcNormalPosition.top)
            {
                if ((piei->uFlags & COF_EXPLORE) &&
                    (s_rcExplorer.left != -1) && (s_rcExplorer.top != -1))
                {
                    // An explorer window is trying to appear on top of
                    // another one.  We'll use our stored rect's top-left 
                    // to make it cascade like IE windows.

                    OffsetRect(&pwndpl->rcNormalPosition,
                       s_rcExplorer.left - pwndpl->rcNormalPosition.left,
                       s_rcExplorer.top - pwndpl->rcNormalPosition.top);                    
                }

                // do the cascade for all windows
                CascadeWindowRect(&pwndpl->rcNormalPosition);
            }
        }

        // for IE and explorer, save the current location
        if (piei->uFlags & COF_EXPLORE)
            s_rcExplorer = pwndpl->rcNormalPosition;
        else if (fInternetStart)
            StoreWindowPlacement(pwndpl);
    } 
    else 
    {
        pwndpl->length = 0;
    }
}

class   CRGN
{
    public:
                CRGN (void)                     {   mRgn = CreateRectRgn(0, 0, 0, 0);                               }
                CRGN (const RECT& rc)           {   mRgn = CreateRectRgnIndirect(&rc);                              }
                ~CRGN (void)                    {   TBOOL(DeleteObject(mRgn));                                      }

                operator HRGN (void)    const   {   return(mRgn);                                                   }
        void    SetRegion (const RECT& rc)      {   TBOOL(SetRectRgn(mRgn, rc.left, rc.top, rc.right, rc.bottom));  }
    private:
        HRGN    mRgn;
};

BOOL    CALLBACK    GetDesktopRegionEnumProc (HMONITOR hMonitor, HDC hdcMonitor, RECT* prc, LPARAM lpUserData)

{
    MONITORINFO     monitorInfo;

    monitorInfo.cbSize = sizeof(monitorInfo);
    if (GetMonitorInfo(hMonitor, &monitorInfo) != 0)
    {
        HRGN    hRgnDesktop;
        CRGN    rgnMonitorWork(monitorInfo.rcWork);

        hRgnDesktop = *reinterpret_cast<CRGN*>(lpUserData);
        TINT(CombineRgn(hRgnDesktop, hRgnDesktop, rgnMonitorWork, RGN_OR));
    }
    return(TRUE);
}

void    EnsureWindowIsCompletelyOnScreen (RECT *prc)

//  99/04/13 #321962 vtan: This function exists because user32 only determines
//  whether ANY part of the window is visible on the screen. It's possible to
//  place a window without an accessible title. Pretty useless when using the
//  mouse and forces the user to use the VERY un-intuitive alt-space.

{
    HMONITOR        hMonitor;
    MONITORINFO     monitorInfo;

    // First find the monitor that the window resides on using GDI.

    hMonitor = MonitorFromRect(prc, MONITOR_DEFAULTTONEAREST);
    ASSERT(hMonitor);           // get vtan - GDI should always return a result
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (GetMonitorInfo(hMonitor, &monitorInfo) != 0)
    {
        LONG    lOffsetX, lOffsetY;
        RECT    *prcWorkArea, rcIntersect;
        CRGN    rgnDesktop, rgnIntersect, rgnWindow;

        // Because the WINDOWPLACEMENT rcNormalPosition field is in WORKAREA
        // co-ordinates this causes a displacement problem. If the taskbar is
        // at the left or top of the primary monitor the RECT passed even though
        // at (0, 0) may be at (100, 0) on the primary monitor in GDI co-ordinates
        // and GetMonitorInfo() will return a MONITORINFO in GDI co-ordinates.
        // The safest generic algorithm is to offset the WORKAREA RECT into GDI
        // co-ordinates and apply the algorithm in that system. Then offset the
        // WORKAREA RECT back into WORKAREA co-ordinates.

        prcWorkArea = &monitorInfo.rcWork;
        if (EqualRect(&monitorInfo.rcMonitor, &monitorInfo.rcWork) == 0)
        {

            // Taskbar is on this monitor - offset required.

            lOffsetX = prcWorkArea->left - monitorInfo.rcMonitor.left;
            lOffsetY = prcWorkArea->top - monitorInfo.rcMonitor.top;
        }
        else
        {

            // Taskbar is NOT on this monitor - no offset required.

            lOffsetX = lOffsetY = 0;
        }
        TBOOL(OffsetRect(prc, lOffsetX, lOffsetY));

        // WORKAREA RECT is in GDI co-ordinates. Apply the algorithm.

        // Check to see if this window already fits the current visible screen
        // area. This is a direct region comparison.

        // This enumeration may cause a performance problem. In the event that
        // a cheap and simple solution is required it would be best to do a
        // RECT intersection with the monitor and the window before resorting
        // to the more expensive region comparison. Get vtan if necessary.

        TBOOL(EnumDisplayMonitors(NULL, NULL, GetDesktopRegionEnumProc, reinterpret_cast<LPARAM>(&rgnDesktop)));
        rgnWindow.SetRegion(*prc);
        TINT(CombineRgn(rgnIntersect, rgnDesktop, rgnWindow, RGN_AND));
        if (EqualRgn(rgnIntersect, rgnWindow) == 0)
        {
            LONG    lDeltaX, lDeltaY;

            // Some part of the window is not within the visible desktop region
            // Move it until it all fits. Size it if it's too big.

            lDeltaX = lDeltaY = 0;
            if (prc->left < prcWorkArea->left)
                lDeltaX = prcWorkArea->left - prc->left;
            if (prc->top < prcWorkArea->top)
                lDeltaY = prcWorkArea->top - prc->top;
            if (prc->right > prcWorkArea->right)
                lDeltaX = prcWorkArea->right - prc->right;
            if (prc->bottom > prcWorkArea->bottom)
                lDeltaY = prcWorkArea->bottom - prc->bottom;
            TBOOL(OffsetRect(prc, lDeltaX, lDeltaY));
            TBOOL(IntersectRect(&rcIntersect, prc, prcWorkArea));
            TBOOL(CopyRect(prc, &rcIntersect));
        }

        // Put WORKAREA RECT back into WORKAREA co-ordinates.

        TBOOL(OffsetRect(prc, -lOffsetX, -lOffsetY));
    }
}

LPITEMIDLIST MyDocsIDList(void);

#define FRAME_OPTIONS_TO_TEST      (BFO_ADD_IE_TOCAPTIONBAR | BFO_USE_DIALUP_REF | BFO_USE_IE_TOOLBAR | \
                                    BFO_BROWSER_PERSIST_SETTINGS | BFO_USE_IE_OFFLINE_SUPPORT)
HRESULT CShellBrowser2::_SetBrowserFrameOptions(LPCITEMIDLIST pidl)
{
    BROWSERFRAMEOPTIONS dwOptions = FRAME_OPTIONS_TO_TEST;
    if (FAILED(GetBrowserFrameOptionsPidl(pidl, dwOptions, &dwOptions)))
    {
        // GetBrowserFrameOptionsPidl() will fail if pidl is NULL.
        // in that case we want to use _fInternetStart to determine
        // if we want these bits set or not.
        if (_fInternetStart)
            dwOptions = FRAME_OPTIONS_TO_TEST;   // Assume None.
        else
            dwOptions = BFO_NONE;   // Assume None.
    }
        
    _fAppendIEToCaptionBar = BOOLIFY(dwOptions & BFO_ADD_IE_TOCAPTIONBAR);
    _fAddDialUpRef = BOOLIFY(dwOptions & BFO_USE_DIALUP_REF);
    _fUseIEToolbar = BOOLIFY(dwOptions & BFO_USE_IE_TOOLBAR);
    _fEnableOfflineFeature = BOOLIFY(dwOptions & BFO_USE_IE_OFFLINE_SUPPORT);
    _fUseIEPersistence = BOOLIFY(dwOptions & BFO_BROWSER_PERSIST_SETTINGS);


    return S_OK;
}
    

HRESULT CShellBrowser2::OnCreate(LPCREATESTRUCT pcs)
{
    HRESULT hres = S_OK;
    IETHREADPARAM* piei = (IETHREADPARAM*)pcs->lpCreateParams;
    BOOL    fUseHomePage = (piei->piehs ? FALSE : TRUE ); // intentionally reversered
    DWORD dwExStyle = IS_BIDI_LOCALIZED_SYSTEM() ? dwExStyleRTLMirrorWnd : 0L;

    _clsidThis = (piei->uFlags & COF_IEXPLORE) ? 
                 CLSID_InternetExplorer : CLSID_ShellBrowserWindow;

#ifdef NO_MARSHALLING
    if (!piei->fOnIEThread) 
        _fOnIEThread = FALSE;
#endif

    //
    //  Make this thread foreground here so that any UI from
    // USERCLASS::OnCreate will be on the top of other windows.
    // We used to call this in _AfterWindowCreate, but it is
    // too late for dialog boxes we popup while processing WM_CREATE
    // message.
    //
    // Note that we do call SetForegroundWindow even if this window
    // is not created as a result of CoCreateInstance. The automation
    // client is supposed to make it visible and bring it to the
    // foreground if it needs to. 
    //
    if (!piei->piehs) 
    {
        // On UNIX it is not common to force the window to the top of Z-order.
#ifndef UNIX
        SetForegroundWindow(_pbbd->_hwnd);  
#endif
    }

    SUPERCLASS::OnCreate(pcs);
    
    EnsureWebViewRegSettings();

    ASSERT(piei);
    _fRunningInIexploreExe = BOOLIFY(piei->uFlags & COF_IEXPLORE);
    v_InitMembers();

    TCHAR szVeryFirstPage[MAX_URL_STRING]; // must be with pszCmdLine
    TCHAR szCmdLine[MAX_URL_STRING];
    if (piei->pszCmdLine)
        SHUnicodeToTChar(piei->pszCmdLine, szCmdLine, ARRAYSIZE(szCmdLine));
    else
        szCmdLine[0] = TCHAR('\0');
    LPTSTR pszCmdLine = szCmdLine;

    if (piei->fCheckFirstOpen) 
    {
        ASSERT(!ILIsRooted(piei->pidl));
        //
        // We don't want to go to the very first page, if this window
        // is created as the result of CoCreateInstnace.
        //
        if (!piei->piehs && (piei->uFlags & COF_IEXPLORE))
        {
            HRESULT hresT = _GetStdLocation(szVeryFirstPage, ARRAYSIZE(szVeryFirstPage), DVIDM_GOFIRSTHOME);
            TraceMsg(DM_NAV, "CSB::OnCreate _GetStdLocation(DVIDM_GOFIRSTHOME) returned %x", hresT);
            if (SUCCEEDED(hresT)) 
            {
                pszCmdLine = szVeryFirstPage;
                TraceMsg(DM_NAV, "CSB::OnCreate _GetStdLocation(DVIDM_GOFIRSTHOME) returned %s", pszCmdLine);
                _fInternetStart = TRUE;
            }
        }
        piei->fCheckFirstOpen = FALSE;
    }

    // NOTE: These flags and corresponding ones in IETHREADPARAM are set to FALSE at creation, 
    // ParseCommandLine() is the only place where they are set.  -- dli
    _fNoLocalFileWarning = piei->fNoLocalFileWarning;
    _fKioskMode = piei->fFullScreen;
    if (piei->fNoDragDrop)
        SetFlags(0, BSF_REGISTERASDROPTARGET);
    _fAutomation = piei->fAutomation;
    
    // If someone deliberately tell us not to use home page. 
    if (piei->fDontUseHomePage) 
    {
        fUseHomePage = 0;
        
        // only the IE path sets this flag.
        // this is used by iexplorer.exe -nohome
        // and by ie dde to start out blank then navigate next
        _fInternetStart = TRUE;
    }

    if (piei->ptl)
        InitializeTravelLog(piei->ptl, piei->dwBrowserIndex);

    LPITEMIDLIST pidl = NULL;
    BOOL fCloning = FALSE;

    if (((pszCmdLine && pszCmdLine[0]) || piei->pidl)  && !_fAutomation)
    {
        if (piei->pidl) 
        {
            pidl = ILClone(piei->pidl);
        } 
        else 
        {
            if (SUCCEEDED(WrapSpecialUrlFlat(pszCmdLine, lstrlen(pszCmdLine) + 1)))
            {
                HRESULT hresT = _ConvertPathToPidl(this, _pbbd->_hwnd, pszCmdLine, &pidl);
                TraceMsg(DM_STARTUP, "OnCreate ConvertPathToPidl(pszCmdLine) returns %x", hresT);
            }
        }
    }
    

    if (pidl) 
    {
         fUseHomePage = FALSE;
    } 
    else if (_pbbd->_ptl && SUCCEEDED(_pbbd->_ptl->GetTravelEntry((IShellBrowser *)this, 0, NULL))) 
    {
        pidl = ILClone(&s_idlNULL);
        fCloning = TRUE;
        fUseHomePage = FALSE;
        // NOTE: if we ever hit this code when opening a window at non-web address
        // we'll need to be more selective about setting this flag
        _fInternetStart = TRUE;
    } 
    else if (fUseHomePage) 
    {
        // if we're not top level, assume we're going to be told
        // where to browse
        WCHAR szPath[MAX_URL_STRING];

        if (piei->uFlags & COF_IEXPLORE)
            hres = _GetStdLocation(szPath, ARRAYSIZE(szPath), DVIDM_GOHOME);
        else
        {
            //  we need to get the default location for an explorer window
            //  which classically has been the root drive of the windows install
            GetModuleFileName(GetModuleHandle(NULL), szPath, SIZECHARS(szPath));
            PathStripToRoot(szPath);
        }
        
        if (SUCCEEDED(hres)) 
        {
            IECreateFromPath(szPath, &pidl);
        }
    }

    // do this here after we've found what pidl we're looking at
    // but do it before the CalcWindowPlacement because
    // it might need to override
    _LoadBrowserWindowSettings(piei, pidl);

    // call this before PrepareInternetToolbar because it needs to know
    // _fInternetStart to know which toolbar config to use
    if (!_fInternetStart) 
    {
        if (pidl) 
        {
            if (fUseHomePage || IsURLChild(pidl, TRUE)) 
            {
                _fInternetStart = TRUE;
            } 
            else 
            {
                DWORD dwAttrib = SFGAO_FOLDER | SFGAO_BROWSABLE;

                // if it's on the file system, we'll still consider it to be an
                // internet folder if it's a docobj (including .htm file)
                IEGetAttributesOf(pidl, &dwAttrib);

                if ((dwAttrib & (SFGAO_FOLDER | SFGAO_BROWSABLE)) == SFGAO_BROWSABLE)
                    _fInternetStart = TRUE;
            }
        } 
        else if (!(piei->uFlags & COF_SHELLFOLDERWINDOW))
            _fInternetStart = TRUE;            
    }

    _SetBrowserFrameOptions(pidl);
    CalcWindowPlacement(_fUseIEPersistence, _pbbd->_hwnd, piei, &piei->wp);
    
    if (!_PrepareInternetToolbar(piei))
        return E_FAIL;

    _CreateToolbar();

    // We must create _hwndStatus before navigating, because the
    // first navigate will go synchronous and the shell sends
    // status messages during that time. If the status window hasn't
    // been created, they drop on the floor.
    //
    _hwndStatus = CreateWindowEx(dwExStyle, STATUSCLASSNAME, NULL,
                                 WS_CHILD | SBARS_SIZEGRIP | WS_CLIPSIBLINGS | WS_VISIBLE | SBT_TOOLTIPS
                                 & ~(WS_BORDER | CCS_NODIVIDER),
                                -100, -100, 10, 10, _pbbd->_hwnd, (HMENU)FCIDM_STATUS, HINST_THISDLL, NULL);
#ifdef DEBUG
    if (g_dwPrototype & 0x00000004) 
    {
        HRESULT hres = E_FAIL;
        if (_SaveToolbars(NULL) == S_OK) 
        {
            // _LoadBrowserWindowSettings did a v_GetViewStream/ _LoadToolbars
            // if it succeeded (i.e. if we have > 0 toolbars), we're done
            // BUGBUG actually, even 0 toolbars could mean success, oh well...
            hres = S_OK;
        }
        ASSERT(SUCCEEDED(hres));
    }
#endif
    
    // BUGBUG: do this early to let these objects see the first
    // navigate. but this causes a deadlock if the objects require
    // marshalling back to the main thread.
    _LoadBrowserHelperObjects();

#ifdef UNIX
    {
        BOOL bReadOnly = FALSE;
        static s_bMsgShown = FALSE;
        if ( !s_bMsgShown ) 
        {
            s_bMsgShown = TRUE;
            unixGetWininetCacheLockStatus ( &bReadOnly, NULL );

            if ( bReadOnly ) 
            {
                _SetBrowserBarState( -1, &CLSID_MsgBand, 1 );
            }
        }
    }

#endif
    BOOL fDontIncrementSessionCounter = FALSE;
    if (pidl)    // paranoia
    {
        if (fCloning)
        {
            ASSERT(_pbbd->_ptl);
            hres = _pbbd->_ptl->Travel((IShellBrowser*)this, 0);
        } 
        else
#ifndef UNIX
        {
            if (!_fAddDialUpRef)
                fDontIncrementSessionCounter = TRUE;
                
            hres = _NavigateToPidl(pidl, 0, 0);
            if (FAILED(hres)) 
            {
                fDontIncrementSessionCounter = TRUE; // We're going to windows\blank.htm or fail ...
                if (_fAddDialUpRef) 
                {
                    // if we failed, but this was a URL child, 
                    // we should still activate and go to blank.htm
                    hres = S_FALSE;
                }
                else if (piei->uFlags & COF_EXPLORE)
                {
                    // If an explorer browser, fall back to the desktop.
                    //
                    // The reason is that we want Start->Windows Explorer to
                    // bring up a browser even if MyDocs is inaccessible;
                    // however we don't want Start->Run "<path>" to bring up
                    // a browser if <path> is inaccessible.
                    //
                    
                    BOOL fNavDesktop = (hres != HRESULT_FROM_WIN32(ERROR_CANCELLED));

                    if (!fNavDesktop)
                    {
                        LPITEMIDLIST pidlDocs = MyDocsIDList();
                        if (pidlDocs)
                        {
                            fNavDesktop = ILIsEqual(pidl, pidlDocs);
                            ILFree(pidlDocs);
                        }
                    }
                    if (fNavDesktop)
                        hres = _NavigateToPidl(&s_idlNULL, 0, 0);
                }
            }
        }
#else
        {
            hres = S_FALSE;
            if (pidl)
            {
                LPITEMIDLIST pidlCopy = ILClone( pidl );
                PostMessage( _pbbd->_hwnd, WM_COMMAND, FCIDM_STARTPAGE, (LPARAM)pidlCopy);
            }
        }
#endif     
        ILFree(pidl);
    }

    if (_fAddDialUpRef && !fDontIncrementSessionCounter)
        _IncrNetSessionCount();

    // 99/04/07 #141049 vtan: If hMonitor was given then use this as the basis
    // for placement of a new window. Move the window position from the primary
    // monitor (where user32 placed it) to the specified HMONITOR. If this
    // results in a placement off screen then SetWindowPlacement() will fix
    // this up for us.

    if ((piei->wp.length == 0) && ((piei->uFlags & COF_HASHMONITOR) != 0) && (piei->pidlRoot != NULL))
    {
        MONITORINFO     monitorInfo;

        piei->wp.length = sizeof(piei->wp);
        TBOOL(GetWindowPlacement(_pbbd->_hwnd, &piei->wp));
        monitorInfo.cbSize = sizeof(monitorInfo);
        TBOOL(GetMonitorInfo(reinterpret_cast<HMONITOR>(piei->pidlRoot), &monitorInfo));
        TBOOL(OffsetRect(&piei->wp.rcNormalPosition, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top));
    }

    if (piei->wp.length == SIZEOF(piei->wp)) 
    {
        BOOL fSetWindowPosition = TRUE;

        // we do a SetWindowPlacement first with SW_HIDE
        // to get the size right firsth
        // then we really show it.
        if ((piei->nCmdShow == SW_SHOWNORMAL) || 
            (piei->nCmdShow == SW_SHOWDEFAULT)) 
            piei->nCmdShow = piei->wp.showCmd;
        piei->wp.showCmd = SW_HIDE;

        HWND hwndTray = GetTrayWindow();
        if (hwndTray)
        {
            RECT rc;
            if (GetWindowRect(hwndTray, &rc) && ISRECT_EQUAL(rc, piei->wp.rcNormalPosition))
            {
                // In this case, we want to ignore the position because
                // it's equal to the tray. (Came from Win95/OSR2 days)
                fSetWindowPosition = FALSE;
            }
        }

        if (fSetWindowPosition)
        {
            EnsureWindowIsCompletelyOnScreen(&piei->wp.rcNormalPosition);
            SetWindowPlacement(_pbbd->_hwnd, &piei->wp);
        }
    }

    v_ShowHideChildWindows(TRUE);

    if (piei->piehs)
    {
        // this thread was created to be the browser automation object

        // turn this on to prove CoCreateInstance does not cause a dead lock.
#ifdef MAX_DEBUG
        SendMessage(HWND_BROADCAST, WM_WININICHANGE, 0, 0);
        Sleep(5);
        SendMessage(HWND_BROADCAST, WM_WININICHANGE, 0, 0);
#endif
        //
        // WARNING: Note that we must SetEvent even though we can't return
        //  the marshalled automation object for some reason. Not signaling
        //  the event will block the caller thread for a long time.
        //
        if (SUCCEEDED(hres)) 
        {
            hres = CoMarshalInterface(piei->piehs->GetStream(), IID_IUnknown, _pbbd->_pautoWB2,
                                                 MSHCTX_INPROC, 0, MSHLFLAGS_NORMAL);
        }
        piei->piehs->PutHresult(hres);
        SetEvent(piei->piehs->GetHevent());
    }

    if (g_tidParking == GetCurrentThreadId()) 
    {
        IEOnFirstBrowserCreation(_fAutomation ? _pbbd->_pautoWB2 : NULL);
    }

    SHGetThreadRef(&_punkMsgLoop);  // pick up the ref to this thread

    TraceMsg(DM_STARTUP, "ief OnCreate returning hres=%x", hres);
    if (FAILED(hres))
    {
        _SetMenu(NULL);
        return E_FAIL;
    }
    return S_OK;
}

//***    InfoIdmToTBIdm -- convert btwn browserbar IDM and TBIDM
//
int InfoIdmToTBIdm(int val, BOOL fToTB)
{
    static const int menutab[] = {
        FCIDM_VBBSEARCHBAND, 
        FCIDM_VBBFAVORITESBAND, 
        FCIDM_VBBHISTORYBAND, 
#ifdef ENABLE_CHANNELPANE
        FCIDM_VBBCHANNELSBAND,
#endif
        FCIDM_VBBEXPLORERBAND,
    };

    static const int tbtab[] = {
        TBIDM_SEARCH, 
        TBIDM_FAVORITES,
        TBIDM_HISTORY,
#ifdef ENABLE_CHANNELPANE
        TBIDM_CHANNELS,
#endif
        TBIDM_ALLFOLDERS,
    };

    return SHSearchMapInt(fToTB ? menutab : tbtab, fToTB ? tbtab : menutab,
        ARRAYSIZE(menutab), val);
}

void _CheckSearch(UINT idmInfo, BOOL fCheck, IExplorerToolbar* _pxtb)
{
    idmInfo = InfoIdmToTBIdm(idmInfo, TRUE);
    if (idmInfo == -1)
        return;

    if (_pxtb)
    {
        UINT uiState;
        if (SUCCEEDED(_pxtb->GetState(&CLSID_CommonButtons, idmInfo, &uiState)))
        {
            if (fCheck)
                uiState |= TBSTATE_CHECKED;
            else
                uiState &= ~TBSTATE_CHECKED;
            _pxtb->SetState(&CLSID_CommonButtons, idmInfo, uiState);
        }
    }
}


//
// Implementation of CShellBrowser2::ShowToolbar
//
// Make toolbar visible or not and update our conception of whether it
// should be shown.  The toolbar by definition is strictly the toolbar.
// The caller has no idea that part of our internet toolbar also has
// the menu.  We must make sure we don't hide the menuband -- only the
// other bands on the internet toolbar.
//
// Returns: S_OK, if successfully done.
//          E_INVALIDARG, duh.
//
HRESULT CShellBrowser2::ShowToolbar(IUnknown* punkSrc, BOOL fShow)
{
    HRESULT hres;
    UINT itb = _FindTBar(punkSrc);

    if (ITB_ITBAR == itb)
    {
        LPTOOLBARITEM ptbi = _GetToolbarItem(itb);

        ITBar_ShowDW(ptbi->ptbar, fShow, fShow, fShow); // BUGBUG: WE LOSE WIN95 COMPAT MODE!!!
        ptbi->fShow = fShow;

        hres = S_OK;
    }
    else
        hres = SUPERCLASS::ShowToolbar(punkSrc, fShow);

    return S_OK;
}


extern IDeskBand * _GetInfoBandBS(IBandSite *pbs, const CLSID *pclsid);

#ifdef DEBUG // {
//***
// NOTES
//  WARNING: dtor frees up CBandSiteMenu on exit!
//  BUGBUG should we make sure some minimum set is created?
HRESULT CShellBrowser2::_AddInfoBands(IBandSite *pbs)
{
    if (!_pbsmInfo)
        return E_FAIL;

    BANDCLASSINFO *pbci;
    for (int i = 0; (pbci = _pbsmInfo->GetBandClassDataStruct(i)) != NULL; i++) {
        IDeskBand *pband;

        pband = _GetInfoBandBS(pbs, &(pbci->clsid));
        if (pband != NULL)
            pband->Release();
    }

    {
        // BUGBUG Exec -> Select or SetBandState
        // BUGBUG chrisfra 5/23/97 - ever heard of variants?  this should set
        // vt = VT_I4 and lVal = 1.  this is horrible, I assume this is being ripped
        // out when Select funcs added, if not, it should be recoded.
        VARIANTARG vaIn = { 0 };
        //VariantInit();
        vaIn.vt = VT_UNKNOWN;
        vaIn.punkVal = (IUnknown *)1;   // show all
        IUnknown_Exec(pbs, &CGID_DeskBand, DBID_SHOWONLY, OLECMDEXECOPT_PROMPTUSER, &vaIn, NULL);
        //VariantClear();
    }

    return S_OK;
}
#endif // }

// this is for ie4 shell compat - the ie4 shell menus have a Explorer bar popup

// BUGBUG: need to make a pass through & fix all _GetBrowserBarMenu ref's,
// since we're back to having view->explorer bars-> on all platforms
HMENU CShellBrowser2::_GetBrowserBarMenu()
{
    HMENU hmenu = _GetMenuFromID(FCIDM_VIEWBROWSERBARS);

    if (hmenu == NULL)
    {
        hmenu = _GetMenuFromID(FCIDM_MENU_VIEW);
        if (hmenu == NULL)
        {
            //if we're here, someone has taken our view menu (docobj)
            hmenu = SHGetMenuFromID(_hmenuPreMerged, FCIDM_VIEWBROWSERBARS);
            ASSERT(hmenu);
        }
    }
    ASSERT(IsMenu(hmenu));
    return hmenu;
}

void CShellBrowser2::_AddBrowserBarMenuItems(HMENU hmenu)
{
    // Find the placeholder item, so we can add items before it
    int iPos = SHMenuIndexFromID(hmenu, FCIDM_VBBPLACEHOLDER);
    if (iPos < 0)
    {
        // we've already had our way with this menu
        ASSERT(_pbsmInfo);
        return;
    }

    //_pbsmInfo is shared across all views in the view menu
    BOOL fCreatedNewBSMenu = FALSE;

    if (!_pbsmInfo) 
    {
        HRESULT hres;
        IUnknown *punk;

        hres = CBandSiteMenu_CreateInstance(NULL, &punk, NULL);
        if (SUCCEEDED(hres)) 
        {
            hres = punk->QueryInterface(CLSID_BandSiteMenu, (void **)&_pbsmInfo);
            punk->Release();
            fCreatedNewBSMenu = TRUE;
        }
    }

    if (!_pbsmInfo)
        return;

    int idCmdNext;
    UINT cBands = 0;

    if (fCreatedNewBSMenu) 
    {
        //  Load up infobands
        cBands = _pbsmInfo->LoadFromComCat(&CATID_InfoBand);

        // nuke any infoband entries that are already in the fixed list
        for (int i = FCIDM_VBBFIXFIRST; i < FCIDM_VBBFIXLAST; i++) 
        {
            const CLSID *pclsid = _InfoIdmToCLSID(i);
            if (pclsid)
            {
                if( _pbsmInfo->DeleteBandClass( *pclsid ) )
                    cBands--;
            }
        }

        // merge the additional infobands contiguously
        idCmdNext = _pbsmInfo->CreateMergeMenu(hmenu, VBBDYN_MAXBAND, iPos - 1, FCIDM_VBBDYNFIRST,0);

        //  Load up commbands
        _iCommOffset = cBands;
        cBands = _pbsmInfo->LoadFromComCat(&CATID_CommBand);
    }
    else
    {
        // Add the additional infobands contiguously
        int cMergedInfoBands = _pbsmInfo->GetBandClassCount( &CATID_InfoBand, TRUE /*merged*/ ); 
        idCmdNext = _pbsmInfo->CreateMergeMenu(hmenu, cMergedInfoBands, iPos - 1, FCIDM_VBBDYNFIRST,0);
        cBands = _pbsmInfo->LoadFromComCat(NULL);
    }

    // placeholder position may have changed at this point
    iPos = SHMenuIndexFromID(hmenu, FCIDM_VBBPLACEHOLDER);

    // Add comm bands.
    if (_iCommOffset != cBands)
    {
        //  Insert a separator if there are comm bands
        InsertMenu(hmenu, iPos + _iCommOffset + 1, MF_BYPOSITION | MF_SEPARATOR, -1, NULL);

        // Now merge the comm bands
        _pbsmInfo->CreateMergeMenu(hmenu, VBBDYN_MAXBAND, iPos + _iCommOffset + 2, idCmdNext, _iCommOffset);
    }

    DeleteMenu(hmenu, FCIDM_VBBPLACEHOLDER, MF_BYCOMMAND);
}

int CShellBrowser2::_IdBarFromCmdID(UINT idCmd)
{
    const CATID* pcatid = _InfoIdmToCATID(idCmd);

    if (pcatid)
    {
        if (IsEqualCATID(*pcatid, CATID_InfoBand))
        {
            // It's a vertical bar
            return IDBAR_VERTICAL;
        }
        else
        {
            // It's a horizontal bar
            ASSERT(IsEqualCATID(*pcatid, CATID_CommBand));
            return IDBAR_HORIZONTAL;
        }
    }

    // Command doesn't correspond to any bar
    return IDBAR_INVALID;
}

int CShellBrowser2::_eOnOffNotMunge(int eOnOffNot, UINT idCmd, UINT idBar)
{
    // BUGBUG: todo -- drive an ashen stake through the foul heart of this function

    if (eOnOffNot == -1) 
    {
        // toggle
        // 'special' guys are set; 'real' guys are toggled
        ASSERT(idCmd != FCIDM_VBBNOVERTICALBAR && idCmd != FCIDM_VBBNOHORIZONTALBAR);

        if (idCmd == FCIDM_VBBNOVERTICALBAR || idCmd == FCIDM_VBBNOHORIZONTALBAR)
            eOnOffNot = 0;
        else if ((idCmd >= FCIDM_VBBDYNFIRST) && (idCmd <= FCIDM_VBBDYNLAST))
            eOnOffNot = (idBar == IDBAR_VERTICAL) ? (idCmd != _idmInfo) : (idCmd != _idmComm);
        else
            eOnOffNot = (idCmd != _idmInfo);
    }

    return eOnOffNot;
}

//***   csb::_SetBrowserBarState -- handle menu/toolbar/exec command, *and* update UI
// ENTRY/EXIT
//  idCmd           FCIDM_VBB* or -1 (if want to use pclsid instead)
//  pclsid          clsid or NULL (if want to use idCmd instead)
//  eOnOffToggle    1=on, 0=off, -1=not (off/not only for fixed bands for now)
// NOTES
//  menu code calls w/ idCmd, Exec code calls w/ pclsid
//
void CShellBrowser2::_SetBrowserBarState(UINT idCmd, const CLSID *pclsid, int eOnOffNot, LPCITEMIDLIST pidl)
{
    if (idCmd == -1)
        idCmd = _InfoCLSIDToIdm(pclsid);

    if (pclsid == NULL)
        pclsid = _InfoIdmToCLSID(idCmd);

    ASSERT(_InfoCLSIDToIdm(pclsid) == idCmd);

    int idBar = _IdBarFromCmdID(idCmd);
    if (idBar == IDBAR_INVALID)
    {
        // We don't recognize this bubby, bail
        return;
    }

    // Munge the unholy eOnOffNot
    eOnOffNot = _eOnOffNotMunge(eOnOffNot, idCmd, idBar);

    if (eOnOffNot == 0 && (idCmd != _idmInfo) && (idCmd != _idmComm)) 
    {
        // already off
        return;
    }

    ASSERT(eOnOffNot == 0 || eOnOffNot == 1);

    pclsid = _ShowHideBrowserBar(idBar, pclsid, eOnOffNot, pidl);

    if (IDBAR_VERTICAL == idBar)
        v_SetIcon();

    //it's bad that we muck with the menus here, but it's the most efficient place
    HMENU hmenu = _GetBrowserBarMenu();

    if (IDBAR_VERTICAL == idBar) 
    {
        // Vertical bar

        SHCheckMenuItem(hmenu, _idmInfo, FALSE);
        // since we support multiple searches in the same band
        // it is possible to have search band open when we clicked on
        // a different search so to avoid flicker we don't "unpress" the button
        if (_idmInfo != idCmd)
            _CheckSearch(_idmInfo, FALSE, _pxtb);
        _idmInfo = eOnOffNot ? idCmd : FCIDM_VBBNOVERTICALBAR;
        _CheckSearch(_idmInfo, TRUE, _pxtb);
        SHCheckMenuItem(hmenu, _idmInfo, TRUE);
    }
    else 
    {
        // Horizontal bar
        SHCheckMenuItem(hmenu, _idmComm, FALSE);
        _idmComm = eOnOffNot ? idCmd : FCIDM_VBBNOHORIZONTALBAR;
        SHCheckMenuItem(hmenu, _idmComm, TRUE);
    }

    // Make sure that the toolbar is updated
    Exec(NULL, OLECMDID_UPDATECOMMANDS, 0, NULL, NULL);

    //set the dirty bit on itbar and save
    Exec(&CGID_PrivCITCommands, CITIDM_SET_DIRTYBIT, TRUE, NULL, NULL);
    Exec(&CGID_ShellBrowser, FCIDM_PERSISTTOOLBAR, 0, NULL, NULL);
}

//***   csb::_ShowHideBrowserBar -- do the op, but do *not* update the UI
// ENTRY/EXIT
//  return      guy who's now visible (pclsid, 0 [VBBNONE], or 1 [VBBALL])
// NOTES
//  don't call this directly, it's just a helper.
//  don't reference UI stuff (_idmInfo etc.) (except for ASSERTs).
const CLSID * CShellBrowser2::_ShowHideBrowserBar(int idBar, const CLSID *pclsid, int eOnOff, LPCITEMIDLIST pidl)
{
    IBandSite *pbs;
    IDeskBand *pband = NULL;

    if (eOnOff == 0 || pclsid == NULL)
    {
        // If pclsid, hide that bar
        // Else, hide everyone

        if (IDBAR_VERTICAL == idBar)
        {
            ASSERT(pclsid == NULL || _InfoCLSIDToIdm(pclsid) == _idmInfo);
            _GetBrowserBar(IDBAR_VERTICAL, FALSE, NULL, NULL);
        }
        else
        {
            ASSERT(idBar == IDBAR_HORIZONTAL);

            ASSERT(pclsid == NULL || _InfoCLSIDToIdm(pclsid) == _idmComm);
            _GetBrowserBar(IDBAR_HORIZONTAL, FALSE, NULL, NULL);
        }

        return NULL;
    }

    IDeskBar *pdbBar = NULL;
    HRESULT hres = E_FAIL;

    //QI for IDeskBand instead?
    LPCWSTR pwszItem;

    if (IDBAR_VERTICAL == idBar) {
        pwszItem = INFOBAR_TBNAME;
    } else {
        ASSERT(IDBAR_HORIZONTAL == idBar)
        pwszItem = COMMBAR_TBNAME;
    }

    hres = FindToolbar(pwszItem, IID_IDeskBar, (void **)&pdbBar);
    if (hres == S_OK) 
    {
        //if a bar is being shown, tell CBrowserBar which clsid it is
        SA_BSTRGUID strClsid;

        StringFromGUID2(*pclsid, strClsid.wsz, ARRAYSIZE(strClsid.wsz));
        strClsid.cb = lstrlenW(strClsid.wsz) * SIZEOF(WCHAR);

        VARIANT varClsid;
        varClsid.vt = VT_BSTR;
        varClsid.bstrVal = strClsid.wsz;
    
        IUnknown_Exec(pdbBar, &CGID_DeskBarClient, DBCID_CLSIDOFBAR, eOnOff, &varClsid, NULL);

        pdbBar->Release();
    }

    // show CLSID

    // get bar (create/cache or retrieve from cache)
    if (FAILED(_GetBrowserBar(idBar, TRUE, &pbs, pclsid)))
        return NULL;

    // get band (create/add or find)
    {
        if (_pbbd->_pautoWB2) 
        {
            SA_BSTRGUID strClsid;
            // Check if this object can be found through automation.
            StringFromGUID2(*pclsid, strClsid.wsz, ARRAYSIZE(strClsid.wsz));
            strClsid.cb = lstrlenW(strClsid.wsz) * SIZEOF(WCHAR);

            VARIANT varUnknown = {0};
            HRESULT hrT = _pbbd->_pautoWB2->GetProperty(strClsid.wsz, &varUnknown);

            if (SUCCEEDED(hrT)) {
                if ((varUnknown.vt == VT_UNKNOWN) && varUnknown.punkVal) {
                    hrT =varUnknown.punkVal->QueryInterface(IID_IDeskBand, (void **)&pband);
                    varUnknown.punkVal->Release();
                }
                else
                    hrT = E_FAIL;
            }

            // This property doesn't exist yet, so create a new band
            if (FAILED(hrT)) {                
                pband = _GetInfoBandBS(pbs, pclsid);
                if (pband) {
                    // Add to the property so that it can be found later
                    varUnknown.vt = VT_UNKNOWN;
                    varUnknown.punkVal = pband;
                    _pbbd->_pautoWB2->PutProperty(strClsid.wsz, varUnknown);
                    // varUnknown.vt = VT_NULL; // We don't want to release pband throug varUnKnown 
                }
            }
        }
        
        if (pband == NULL) {    // Not getting registered
            ASSERT(0);
            pband = _GetInfoBandBS(pbs, pclsid);
        }

        if (pband != NULL) {
            IBandNavigate *pbnav;

            if (pidl && SUCCEEDED(pband->QueryInterface(IID_IBandNavigate, (void **)&pbnav)))
            {
                pbnav->Select(pidl);
                pbnav->Release();
            }
            // BUGBUG Exec -> Select or SetBandState
            VARIANTARG vaIn = { 0 };
            //VariantInit();
            vaIn.vt = VT_UNKNOWN;
            // show me, hide everyone else
            vaIn.punkVal = pband;
            IUnknown_Exec(pbs, &CGID_DeskBand, DBID_SHOWONLY, OLECMDEXECOPT_PROMPTUSER, &vaIn, NULL);
            //VariantClear();

            pband->Release();
        }
        pbs->Release();
    }
    return pclsid;
}

BANDCLASSINFO* CShellBrowser2::_BandClassInfoFromCmdID(UINT idCmd)
{
    if (IsInRange(idCmd, FCIDM_VBBDYNFIRST, FCIDM_VBBDYNLAST))
    {
        if (_pbsmInfo)
        {
            int i, cnt = _pbsmInfo->GetBandClassCount( NULL, FALSE );
            for ( i = 0; i < cnt; i++ )
            {
                BANDCLASSINFO *pbci = _pbsmInfo->GetBandClassDataStruct(i);
                if (pbci && idCmd == pbci->idCmd )
                    return pbci;
            }
        }
    }

    return NULL;
}

//***   _InfoIdmToCLSID -- map FCIDM_VBB*'s to corresponding CLSID
// NOTES
//  handles both 'fixed' and dynamic guys
const CLSID *CShellBrowser2::_InfoIdmToCLSID(UINT idCmd)
{
    const CLSID *pclsid = NULL;

    if (IsInRange(idCmd, FCIDM_VBBFIXFIRST, FCIDM_VBBFIXLAST))
    {
        switch (idCmd) {
        case FCIDM_VBBSEARCHBAND:       pclsid = &CLSID_SearchBand; break;
        case FCIDM_VBBFAVORITESBAND:    pclsid = &CLSID_FavBand; break;
        case FCIDM_VBBHISTORYBAND:      pclsid = &CLSID_HistBand; break;
#ifdef ENABLE_CHANNELPANE
        case FCIDM_VBBCHANNELSBAND:     pclsid = &CLSID_ChannelBand; break;
#endif
        case FCIDM_VBBEXPLORERBAND:     pclsid = &CLSID_ExplorerBand; break;

        }
    }
    else
    {
        BANDCLASSINFO* pbci = _BandClassInfoFromCmdID(idCmd);
        if (pbci)
            pclsid = &pbci->clsid;
    }

    return pclsid;
}

const CATID *CShellBrowser2::_InfoIdmToCATID(UINT idCmd)
{
    const CATID* pcatid = NULL;

    if (IsInRange(idCmd, FCIDM_VBBFIXFIRST, FCIDM_VBBFIXLAST))
    {
        // The fixed bars are all in the vertical comcat
        pcatid = &CATID_InfoBand;
    }
    else
    {
        // Dynamic bar, have to look up the catid
        BANDCLASSINFO* pbci = _BandClassInfoFromCmdID(idCmd);
        if (pbci)
            pcatid = &pbci->catid;
    }

    return pcatid;
}

UINT CShellBrowser2::_InfoCLSIDToIdm(const CLSID *pguid)
{
    if (pguid == NULL)
        return 0;
    else if (IsEqualIID(*pguid, CLSID_ExplorerBand))
        return FCIDM_VBBEXPLORERBAND;
    else if (IsEqualIID(*pguid, CLSID_SearchBand))
        return FCIDM_VBBSEARCHBAND;
    else if (IsEqualIID(*pguid, CLSID_FileSearchBand))
        return FCIDM_VBBSEARCHBAND;
    else if (IsEqualIID(*pguid, CLSID_FavBand))
        return FCIDM_VBBFAVORITESBAND;
    else if (IsEqualIID(*pguid, CLSID_HistBand)) 
        return FCIDM_VBBHISTORYBAND;
#ifdef UNIX
    else if (IsEqualIID(*pguid, CLSID_MsgBand))
        return FCIDM_VBBMSGBAND;
#endif

#ifdef ENABLE_CHANNELPANE
    else if (IsEqualIID(*pguid, CLSID_ChannelBand))         
        return FCIDM_VBBCHANNELSBAND;               
#endif

    else {
        if (!_pbsmInfo){
            // Load the Browser Bar Menu to load the class ids of all Component Categories dynamic Browser bars
            _AddBrowserBarMenuItems(_GetBrowserBarMenu());

            // Unable to load the clsids from dynamic bars.
            if (!_pbsmInfo)
                return -1;
        }

        BANDCLASSINFO *pbci;
        for (int i = 0; NULL != (pbci = _pbsmInfo->GetBandClassDataStruct(i)); i++)
            if (IsEqualIID(*pguid, pbci->clsid))
                return (pbci->idCmd);

        // BUGBUG todo: look up in _pbsmInfo->LoadFromComCat's HDPA
//        ASSERT(0);
    }
    return -1;
}

HBITMAP CreateColorBitmap(int cx, int cy)
{
    HDC hdc;
    HBITMAP hbm;

    hdc = GetDC(NULL);
    hbm = CreateCompatibleBitmap(hdc, cx, cy);
    ReleaseDC(NULL, hdc);

    return hbm;
}

HRESULT CShellBrowser2::_GetBSForBar(LPCWSTR pwszItem, IBandSite **ppbs)
{
    HRESULT hres;
    IDeskBar *pdbBar = NULL;
    IUnknown *punkBS = NULL;

    *ppbs = NULL;
    hres = FindToolbar(pwszItem, IID_IDeskBar, (void **)&pdbBar);
    if (hres==S_OK) {
        hres = pdbBar->GetClient((IUnknown**) &punkBS);
        ASSERT(SUCCEEDED(hres));
        if (punkBS) {
            hres = punkBS->QueryInterface(IID_IBandSite, (void **)ppbs);
            ASSERT(SUCCEEDED(hres));
            punkBS->Release();
        }
        pdbBar->Release();
    }
    return hres;
}

void CShellBrowser2::_ExecAllBands(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, 
                            VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hres;
    IBandSite *pbsBS;

    hres = _GetBSForBar(INFOBAR_TBNAME, &pbsBS);
    if (hres==S_OK) {
        DWORD dwBandID;
        IDeskBand *pstb;
        
        for (int i = 0; SUCCEEDED(pbsBS->EnumBands(i, &dwBandID)); i++) {
            hres = pbsBS->QueryBand(dwBandID, &pstb, NULL, NULL, 0);
            if (SUCCEEDED(hres)) {
                IUnknown_Exec(pstb, pguidCmdGroup, nCmdID, nCmdexecopt, 
                                pvarargIn, pvarargOut);
                pstb->Release();
            }
        }
        pbsBS->Release();
    }
 }

HRESULT CShellBrowser2::_GetBrowserBar(int idBar, BOOL fShow, IBandSite** ppbs, const CLSID* pclsid)
{
    HRESULT hres;
    IUnknown *punkBar;
    IDeskBar *pdbBar = NULL;
    IUnknown *punkBS = NULL;

    if (ppbs) 
        *ppbs = NULL;
    
     _fShowBrowserBar = BOOLIFY(fShow);
    
    if (IDBAR_VERTICAL == idBar)
        hres = FindToolbar(INFOBAR_TBNAME, IID_IDeskBar, (void **)&pdbBar);
    else if (IDBAR_HORIZONTAL == idBar)
        hres = FindToolbar(COMMBAR_TBNAME, IID_IDeskBar, (void **)&pdbBar);
    else
        ASSERT(0);  // No other bars right now.

    TraceMsg(DM_MISC, "CSB::_ToggleBrowserBar FindToolbar returned %x", hres);

    BOOL fTurnOffAutoHide = FALSE;
    if (hres==S_OK) {
        // already have one
        hres = pdbBar->GetClient((IUnknown**) &punkBS);
        ASSERT(SUCCEEDED(hres));
        punkBar = pdbBar;
        // punkBar->Release() down below        
    } else 
    {
        //if there's not a bar, don't bother creating one so it can be hidden
        if (!fShow)
            return S_OK;

        // 1st time, create a new one
        CBrowserBar* pdb = new CBrowserBar();
        if (NULL ==pdb)
        {
            return E_OUTOFMEMORY;
        }
        else
        {
            // add it
            pdb->QueryInterface(IID_IUnknown, (void **)&punkBar);

            //if a bar is being shown, tell CBrowserBar which clsid it is
            SA_BSTRGUID strClsid;

            StringFromGUID2(*pclsid, strClsid.wsz, ARRAYSIZE(strClsid.wsz));
            strClsid.cb = lstrlenW(strClsid.wsz) * SIZEOF(WCHAR);

            VARIANT varClsid;
            varClsid.vt = VT_BSTR;
            varClsid.bstrVal = strClsid.wsz;
        
            IUnknown_Exec(punkBar, &CGID_DeskBarClient, DBCID_CLSIDOFBAR, fShow, &varClsid, NULL);

            UINT uiWidthOrHeight = pdb->_PersistState(NULL, FALSE);

            //don't let anyone be 0 width
            if (uiWidthOrHeight == 0)
                uiWidthOrHeight = (IDBAR_VERTICAL == idBar) ? INFOBAR_WIDTH : COMMBAR_HEIGHT;

            fTurnOffAutoHide = !(GetSystemMetrics(SM_CXSCREEN) <= 800);

            CBrowserBarPropertyBag* ppb;

            //  BUGBUG - this needs to be persisted and restored
            //  when % widths are implemented, should use that
            ppb = new CBrowserBarPropertyBag();
            if (ppb) {

                if (IDBAR_VERTICAL == idBar) {
                    ppb->SetDataDWORD(PROPDATA_SIDE, ABE_LEFT);     // LEFT
                    ppb->SetDataDWORD(PROPDATA_LEFT, uiWidthOrHeight);
                    ppb->SetDataDWORD(PROPDATA_RIGHT, uiWidthOrHeight);
                } else {    // We know (IDBAR_VERTICAL == idBar) 
                    ppb->SetDataDWORD(PROPDATA_SIDE, ABE_BOTTOM);     // BOTTOM
                    ppb->SetDataDWORD(PROPDATA_TOP, uiWidthOrHeight);
                    ppb->SetDataDWORD(PROPDATA_BOTTOM, uiWidthOrHeight);
                }

                ppb->SetDataDWORD(PROPDATA_MODE, WBM_BBOTTOMMOST);

                SHLoadFromPropertyBag(punkBar, ppb);
                ppb->Release();

                
            }
          
            HRESULT hres = AddToolbar(punkBar, (IDBAR_VERTICAL == idBar) ? INFOBAR_TBNAME : COMMBAR_TBNAME, DWFAF_HIDDEN);

            if (EVAL(SUCCEEDED(hres))) 
            {                
                if (SUCCEEDED(hres))
                    hres = BrowserBar_Init(pdb, &punkBS, idBar);
            }
            pdb->Release();
        }
        // punkBar->Release() down below                                                
    }
    // note: must call _SetTheaterBrowserBar BEFORE ShowToolbar when showing bar
    if (IDBAR_VERTICAL == idBar && fShow)
        _SetTheaterBrowserBar();    
    ShowToolbar(punkBar, fShow);      
    // note: must call _SetTheaterBrowserBar AFTER ShowToolbar when hiding bar
    if (IDBAR_VERTICAL == idBar && !fShow)
        _SetTheaterBrowserBar();
    
    //tell CBrowserBar about the new bar, must be AFTER it is shown (to get the size right)
    if (SUCCEEDED(hres) && fShow)
    {
        //if a bar is being shown, tell CBrowserBar which clsid it is
        SA_BSTRGUID strClsid;

        StringFromGUID2(*pclsid, strClsid.wsz, ARRAYSIZE(strClsid.wsz));
        strClsid.cb = lstrlenW(strClsid.wsz) * SIZEOF(WCHAR);

        VARIANT varClsid;
        varClsid.vt = VT_BSTR;
        varClsid.bstrVal = strClsid.wsz;
    
        IUnknown_Exec(punkBar, &CGID_DeskBarClient, DBCID_CLSIDOFBAR, fShow, &varClsid, NULL);
    }
    else
    {
        IUnknown_Exec(punkBar, &CGID_DeskBarClient, DBCID_CLSIDOFBAR, 0, NULL, NULL);
    }

    // note: must have called ShowToolbar BEFORE setting pin button state
    if (fTurnOffAutoHide) {
        VARIANT v = { VT_I4 };
        v.lVal = FALSE;
        IUnknown_Exec(punkBar, &CGID_Theater, THID_SETBROWSERBARAUTOHIDE, 0, &v, &v);
    }

    punkBar->Release();

    // BUGBUG What should we do for CommBar in Theatre Mode?
    
    if (punkBS) {
        
        if (ppbs) {
            punkBS->QueryInterface(IID_IBandSite, (void **)ppbs);
        }
        punkBS->Release();
        
        return S_OK;
    }
    
    return E_FAIL;
}

#ifdef DEBUG
//***   DBCheckCLSID -- make sure class tells truth about its CLSID
//
BOOL DBCheckCLSID(IUnknown *punk, const CLSID *pclsid)
{
    HRESULT hres;
    CLSID clsid;

    hres = IUnknown_GetClassID(punk, &clsid);
    if (SUCCEEDED(hres) && IsEqualGUID(*pclsid, clsid))
        return TRUE;

    TraceMsg(DM_ERROR, "dbcc: CLSID mismatch! &exp=%x &act=%x", pclsid, clsid);
    return FALSE;
}
#endif

IDeskBand * _FindBandByClsidBS(IBandSite *pbs, const CLSID *pclsid)
{
    HRESULT hres;
    DWORD dwBandID;
    IDeskBand *pstb;

    for (int i = 0; SUCCEEDED(pbs->EnumBands(i, &dwBandID)); i++) {
        hres = pbs->QueryBand(dwBandID, &pstb, NULL, NULL, 0);
        if (SUCCEEDED(hres)) {
            CLSID clsid;

            hres = IUnknown_GetClassID(pstb, &clsid);
            if (SUCCEEDED(hres) && IsEqualGUID(*pclsid, clsid)) {
                return pstb;
            }

            pstb->Release();
        }
    }

    return NULL;
}

IDeskBand * _GetInfoBandBS(IBandSite *pbs, const CLSID *pclsid)
{
    IDeskBand *pstb = _FindBandByClsidBS(pbs, pclsid);
    if (pstb == NULL) {
        TraceMsg(DM_MISC, "_gib: create band");
        HRESULT hres = CoCreateInstance(*pclsid, NULL, CLSCTX_INPROC_SERVER, IID_IDeskBand, (void **)&pstb);
        if (EVAL(SUCCEEDED(hres))) {
            // hide all bands before adding new band
            VARIANTARG vaIn = { 0 };
            vaIn.vt = VT_UNKNOWN;
            vaIn.punkVal = 0;
            IUnknown_Exec(pbs, &CGID_DeskBand, DBID_SHOWONLY, OLECMDEXECOPT_PROMPTUSER, &vaIn, NULL);

            pbs->AddBand(pstb);
        }
    }

    return pstb;
}
void CShellBrowser2::_OrganizeFavorites()
{
    TCHAR szPath[MAX_PATH];

    if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_FAVORITES, TRUE))
    {
#ifndef UNIX
        if (GetKeyState(VK_SHIFT) < 0)
        {
            OpenFolderPath(szPath);
        }
        else
#endif
            DoOrganizeFavDlgW(_pbbd->_hwnd, NULL);
    }
}

/*----------------------------------------------------------
Purpose: Handle WM_COMMAND for favorites menu

*/
void CShellBrowser2::_FavoriteOnCommand(HMENU hmenu, UINT idCmd)
{
    switch (idCmd) 
    {
    case FCIDM_ORGANIZEFAVORITES:
        _OrganizeFavorites();
        break;

    case FCIDM_ADDTOFAVORITES:
        Exec(&CGID_Explorer, SBCMDID_ADDTOFAVORITES, OLECMDEXECOPT_PROMPTUSER, NULL, NULL);
        // Instrument add to favorites from menu
        UEMFireEvent(&UEMIID_BROWSER, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_ADDTOFAV, UIBL_MENU);        
        break;

    case FCIDM_UPDATESUBSCRIPTIONS:
        UpdateSubscriptions();
        break;
    }
}

HRESULT CShellBrowser2::CreateBrowserPropSheetExt(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    IUnknown *punk;
    HRESULT hres = CoCreateInstance(CLSID_ShellFldSetExt, NULL, CLSCTX_INPROC_SERVER, riid, (void **)&punk);
    if (SUCCEEDED(hres)) 
    {
        IShellExtInit *psxi;
        hres = punk->QueryInterface(IID_IShellExtInit, (void **)&psxi);
        if (SUCCEEDED(hres)) 
        {
            hres = psxi->Initialize(NULL, NULL, 0);
            if (SUCCEEDED(hres)) 
            {
                IUnknown_SetSite(punk, SAFECAST(this, IShellBrowser*));
                IUnknown_Set((IUnknown **)ppvObj, punk);
                hres = S_OK;            // All happy
            }
            psxi->Release();
        }
        punk->Release();
    }
    return hres;
}

LPITEMIDLIST CShellBrowser2::_GetSubscriptionPidl()
{
    LPITEMIDLIST        pidlSubscribe = NULL;
    IDispatch *         pDispatch = NULL;
    IHTMLDocument2 *    pHTMLDocument = NULL;

    // Search HTML for <LINK REL="Subscription" HREF="{URL}">
    if  (
        SUCCEEDED(_pbbd->_pautoWB2->get_Document(&pDispatch))
        &&
        SUCCEEDED(pDispatch->QueryInterface(IID_IHTMLDocument2,
                                            (void **)&pHTMLDocument))
        )
    {
        IHTMLElementCollection * pLinksCollection;

        if (SUCCEEDED(GetDocumentTags(pHTMLDocument, OLESTR("LINK"), &pLinksCollection)))
        {
            long lItemCnt;

            // Step through each of the LINKs in the
            // collection looking for REL="Subscription".
            EVAL(SUCCEEDED(pLinksCollection->get_length(&lItemCnt)));
            for (long lItem = 0; lItem < lItemCnt; lItem++)
            {
                IDispatch *         pDispItem = NULL;
                IHTMLLinkElement *  pLinkElement = NULL;

                VARIANT vEmpty = { 0 };
                VARIANT vIndex; V_VT(&vIndex) = VT_I4; V_I4(&vIndex) = lItem;

                if  (
                    SUCCEEDED(pLinksCollection->item(vIndex, vEmpty, &pDispItem))
                    &&
                    SUCCEEDED(pDispItem->QueryInterface(IID_IHTMLLinkElement,
                                                        (void **)&pLinkElement))
                    )
                {
                    BSTR bstrREL = NULL;
                    BSTR bstrHREF = NULL;

                    // Finally! We have a LINK element, check its REL type.
                    if  (
                        SUCCEEDED(pLinkElement->get_rel(&bstrREL))
                        &&
                        (bstrREL != NULL)
                        &&
                        SUCCEEDED(pLinkElement->get_href(&bstrHREF))
                        &&
                        (bstrHREF != NULL)
                        )
                    {
                        // Check for REL="Subscription"
                        if (StrCmpIW(bstrREL, OLESTR("Subscription")) == 0)
                        {
                            TCHAR szName[MAX_URL_STRING];

                            SHUnicodeToTChar(bstrHREF, szName, ARRAYSIZE(szName));
                            EVAL(SUCCEEDED(IECreateFromPath(szName, &pidlSubscribe)));
                        }
                    }

                    if (bstrHREF != NULL)
                        SysFreeString(bstrHREF);

                    if (bstrREL != NULL)
                        SysFreeString(bstrREL);
                }

                VariantClear(&vIndex);
                VariantClear(&vEmpty);

                SAFERELEASE(pLinkElement);
                SAFERELEASE(pDispItem);

                // If we found a correctl REL type, quit searching.
                if (pidlSubscribe != NULL)
                    break;
            }

            pLinksCollection->Release();
        }
    }

    SAFERELEASE(pHTMLDocument);
    SAFERELEASE(pDispatch);

    return pidlSubscribe;
}

LPITEMIDLIST CShellBrowser2::_TranslateRoot(LPCITEMIDLIST pidl)
{
    LPCITEMIDLIST pidlChild = ILFindChild(ILRootedFindIDList(_pbbd->_pidlCur), pidl);

    ASSERT(pidlChild);

    LPITEMIDLIST pidlRoot = ILCloneFirst(_pbbd->_pidlCur);

    if (pidlRoot)
    {
        LPITEMIDLIST pidlRet = ILCombine(pidlRoot, pidlChild);
        ILFree(pidlRoot);
        return pidlRet;
    }

    return NULL;
}

BOOL CShellBrowser2::_ValidTargetPidl(LPCITEMIDLIST pidl, BOOL *pfTranslateRoot)
{
    // validate that this is an allowable target to browse to.
    // check that it is a child of our root.
    if (pfTranslateRoot)
        *pfTranslateRoot = FALSE;
        
    if (ILIsRooted(_pbbd->_pidlCur)) 
    {
        BOOL fRet = ILIsEqualRoot(_pbbd->_pidlCur, pidl);

        if (!fRet && pfTranslateRoot 
        && ILIsParent(ILRootedFindIDList(_pbbd->_pidlCur), pidl, FALSE))
        {
            fRet = TRUE;
            *pfTranslateRoot = TRUE;
        }
                
        return fRet;
    }
    
    return TRUE;
}

IStream* CShellBrowser2::_GetITBarStream(BOOL fWebBrowser, DWORD grfMode)
{
    return GetITBarStream(fWebBrowser ? ITBS_WEB : ITBS_SHELL, grfMode);
}

HRESULT CShellBrowser2::_SaveITbarLayout(void)
{
    HRESULT hres = E_FAIL;

#ifdef NO_MARSHALLING
    if (!_fOnIEThread)
      return S_OK;
#endif

    if (_fUISetByAutomation || _ptheater)
    {
        return S_OK;
    }
    if (_GetITBar())
    {
        IPersistStreamInit  *pITbarPSI;

        //Yes! It's a different type. We may need to save the stream
        if (SUCCEEDED(_GetITBar()->QueryInterface(IID_IPersistStreamInit, (void **)&pITbarPSI)))
        {
            //Do we need to save the stream?
            if (pITbarPSI->IsDirty() == S_OK)
            {
                BOOL fInternet = (CITE_INTERNET == 
                    GetScode(IUnknown_Exec(pITbarPSI, &CGID_PrivCITCommands, CITIDM_ONINTERNET, CITE_QUERY, NULL, NULL)));
                IStream *pstm = _GetITBarStream(fInternet, STGM_WRITE);
                if (pstm)
                {
                    //Stream exists. Save it there!.
                    hres = pITbarPSI->Save(pstm, TRUE);
                    pstm->Release();
                }
                else
                {
                    //Stream creation failed! Why?
                    TraceMsg(DM_ITBAR, "CSB::_NavigateToPidl ITBar Stream creation failed");
                    ASSERT(0);
                }
            }
            else
                hres = S_OK; // No need to save. Return success!

            pITbarPSI->Release();
        }
        else
        {
            //ITBar doesn't support IPersistStreamInit?
            AssertMsg(0, TEXT("CSB::_NavigateToPidl ITBar doesn't support IPersistStreamInit"));
        }
    }

    return(hres);
}

HRESULT CShellBrowser2::_NavigateToPidl(LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD dwFlags)
{
    if (pidl) 
    {
        ASSERT(_ValidTargetPidl(pidl, NULL)); 

        if (!g_fICWCheckComplete && IsBrowserFrameOptionsPidlSet(pidl, BFO_USE_DIALUP_REF))
        {
            TCHAR szURL[MAX_URL_STRING];

            EVAL(SUCCEEDED(IEGetNameAndFlags(pidl, SHGDN_FORPARSING, szURL, SIZECHARS(szURL), NULL)));
            if (UrlHitsNetW(szURL) && !UrlIsInstalledEntry(szURL)) 
            {
                if ((CheckRunICW(szURL)) || CheckSoftwareUpdateUI( _pbbd->_hwnd, SAFECAST(this, IShellBrowser *))) // see if ICW needs to run
                {
                    // ICW ran and this was first navigate, shut down now.
                    // Or the user wants a software update, so we're launching a new browser
                    // to the update page
                    _pbbd->_pautoWB2->put_Visible(FALSE);
                    _pbbd->_pautoWB2->Quit();
                    return E_FAIL;
                }                 
            }
        }

        if (!_fVisitedNet && IsBrowserFrameOptionsPidlSet(pidl, BFO_USE_DIALUP_REF))
            _IncrNetSessionCount();
    }

    // See if we are about to navigate to a pidl of a different type. If so, 
    // open the stream and call the ITBar's IPersistStreamInit::save to save.
    if (_GetITBar())
    {
        //Check if we are about to navigate to a different "type" of folder
        if (((INT_PTR)_pbbd->_pidlCur && !IsBrowserFrameOptionsSet(_pbbd->_psf, BFO_BROWSER_PERSIST_SETTINGS)) != 
           ((INT_PTR)pidl && !IsBrowserFrameOptionsPidlSet(pidl, BFO_BROWSER_PERSIST_SETTINGS)))
        {
            _SaveITbarLayout();
        }
    }
    
    return SUPERCLASS::_NavigateToPidl(pidl, grfHLNF, dwFlags);
}

HRESULT CShellBrowser2::BrowseObject(LPCITEMIDLIST pidl, UINT wFlags)
{
    HRESULT hr;
    LPITEMIDLIST pidlFree = NULL;
    // if we're about to go to a new browser, save the layout so that they'll pick it up
    if (wFlags & SBSP_NEWBROWSER) 
        _SaveITbarLayout();

    // 99/03/30 vtan: part of #254171 addition
    // explore with explorer band     visible = same window
    // explore with explorer band NOT visible =  new window
    if (wFlags & SBSP_EXPLOREMODE)
    {
        BOOL    fExplorerBandVisible;

        if (FAILED(IsControlWindowShown(FCW_TREE, &fExplorerBandVisible)))
            fExplorerBandVisible = FALSE;
        if (fExplorerBandVisible)
        {
            wFlags &= ~SBSP_NEWBROWSER;
            wFlags |= SBSP_SAMEBROWSER;
        }
        else
        {
            wFlags &= ~SBSP_SAMEBROWSER;
            wFlags |= SBSP_NEWBROWSER;
        }
    }

    BOOL fTranslate = FALSE;
    // REVIEW: do this only if NEWBROWSER is not set?
    if (pidl && pidl != (LPCITEMIDLIST)-1 && !_ValidTargetPidl(pidl, &fTranslate))
    {
        OpenFolderPidl(pidl);   // we can't navigate to it...  create a new top level dude
        return E_FAIL;
    }

    if (fTranslate)
    {
        pidl = pidlFree = _TranslateRoot(pidl);
    }

    if ((wFlags & SBSP_PARENT) && !_ShouldAllowNavigateParent())
    {
        hr =  E_FAIL;
        goto exit;
    }

#ifdef BROWSENEWPROCESS_STRICT // "Nav in new process" has become "Launch in new process", so this is no longer needed
    // If we want to be strict about BrowseNewProcess (apparently not,
    // given recent email discussions), we'd have to try this one in
    // a new process.  However, don't do that if the window would wind up
    // completely blank.
    //
    if ((_pbbd->_pidlCur || _pbbd->_pidlPending) && TryNewProcessIfNeeded(pidl))
    {
        hr = S_OK;
        goto exit;
    }
#endif

    hr = SUPERCLASS::BrowseObject(pidl, wFlags);

exit:
    ILFree(pidlFree);
    return hr;
}


void CShellBrowser2::_ToolTipFromCmd(LPTOOLTIPTEXT pnm)
{
    UINT idCommand = (UINT)pnm->hdr.idFrom;
    LPTSTR pszText = pnm->szText;
    int cchText = ARRAYSIZE(pnm->szText);
    DWORD dwStyle;

    ITravelLog *ptl;

    if (pnm->hdr.hwndFrom)
        dwStyle = GetWindowLong(pnm->hdr.hwndFrom, GWL_STYLE);

    switch (idCommand) {
    case FCIDM_NAVIGATEBACK:
    case FCIDM_NAVIGATEFORWARD:
        if (SUCCEEDED(GetTravelLog(&ptl)))
        {
            WCHAR wzText[MAX_PATH];

            ASSERT(ptl);
            if (S_OK == ptl->GetToolTipText(SAFECAST(this, IShellBrowser *), idCommand == FCIDM_NAVIGATEBACK ? TLOG_BACK : TLOG_FORE, 0, wzText, ARRAYSIZE(wzText)))
            {
                SHUnicodeToTChar(wzText, pszText, cchText);
                if (pnm->hdr.hwndFrom)
                    SetWindowLong(pnm->hdr.hwndFrom, GWL_STYLE, dwStyle | TTS_NOPREFIX);
            }
            ptl->Release();
            return;
        }
        break;
    }

    if (pnm->hdr.hwndFrom)
        SetWindowLong(pnm->hdr.hwndFrom, GWL_STYLE, dwStyle & ~(TTS_NOPREFIX));
    if (!MLLoadString(idCommand + MH_TTBASE, pszText, cchText))
        *pszText = 0;
}

void CShellBrowser2::v_ParentFolder()
{
    if (_ShouldAllowNavigateParent()) 
    {
        IETHREADPARAM* piei = SHCreateIETHREADPARAM(NULL, 0, NULL, NULL);
        if (piei) 
        {
            piei->hwndCaller = _pbbd->_hwnd;
            piei->pidl = ILClone(_pbbd->_pidlCur);
            ILRemoveLastID(piei->pidl);
            piei->uFlags = COF_NORMAL;
            piei->nCmdShow = SW_SHOW;
            piei->psbCaller = this;
            AddRef();
            SHOpenFolderWindow(piei);
        }
    }
}

LRESULT CShellBrowser2::v_ForwardMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return ForwardViewMsg(uMsg, wParam, lParam);
}

HRESULT CShellBrowser2::_GetCodePage(UINT *puiCodePage, DWORD dwCharSet)
{
    HRESULT hres = E_FAIL;
    VARIANT varIn = { 0 };
    VARIANT varResult = { 0 };
    VARIANT *pvarIn;
    
    if (_pbbd->_pctView)
    {
        if (dwCharSet == SHDVID_DOCFAMILYCHARSET)
        {
            // need varIn
            varIn.vt = VT_I4;
            // varIn.lVal is already inited to zero which is what we want
            pvarIn = &varIn;
        }
        else
        {
            pvarIn = NULL;
        }

        _pbbd->_pctView->Exec(&CGID_ShellDocView, dwCharSet, 0, pvarIn, &varResult);
        *puiCodePage = (UINT)varResult.lVal;
    }
    
    return hres;
}


#if defined(UNIX)
HRESULT _UnixSendDocToOE(LPCITEMIDLIST, UINT, DWORD);
#endif

void CShellBrowser2::_SendCurrentPage(DWORD dwSendAs)
{
    if (_pbbd->_pidlCur && !ILIsEmpty(_pbbd->_pidlCur))
    {
        UINT uiCodePage;
        _GetCodePage(&uiCodePage, SHDVID_DOCCHARSET);
#ifdef UNIX
        if ( OEHandlesMail() )
            _UnixSendDocToOE(_pbbd->_pidlCur, uiCodePage, dwSendAs);
        else
#endif
        {
           IOleCommandTarget *pcmdt = NULL;
           if (_pbbd->_pautoWB2)
           {
               (_pbbd->_pautoWB2)->QueryInterface(IID_IOleCommandTarget, (void **)&pcmdt);
               ASSERT(pcmdt);
           }
           SendDocToMailRecipient(_pbbd->_pidlCur, uiCodePage, dwSendAs, pcmdt);
           if (pcmdt)
               pcmdt->Release();
        }
    }
}

LRESULT CShellBrowser2::OnCommand(WPARAM wParam, LPARAM lParam)
{
    int id;
    DWORD dwError;

    if (_ShouldForwardMenu(WM_COMMAND, wParam, lParam)) {
        ForwardViewMsg(WM_COMMAND, wParam, lParam);
        return S_OK;
    }
    
    UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);
    
#if 1
    //UEMFireEvent(&UEMIID_BROWSER, UEME_UIMENU, UEMF_XEVENT, UIG_NIL, idCmd);
    UEMFireEvent(&UEMIID_BROWSER, UEME_RUNWMCMD, UEMF_XEVENT, UIM_BROWSEUI, idCmd);
#endif

    switch(idCmd)
    {
    case FCIDM_MOVE:
    case FCIDM_COPY:
    case FCIDM_PASTE:
    case FCIDM_SELECTALL:
        {
            IOleCommandTarget* pcmdt;
            HRESULT hres = _FindActiveTarget(IID_IOleCommandTarget, (void **)&pcmdt);
            if (SUCCEEDED(hres)) {
                const static UINT c_mapEdit[] = {
                    OLECMDID_CUT, OLECMDID_COPY, OLECMDID_PASTE, OLECMDID_SELECTALL };
    
                pcmdt->Exec(NULL, c_mapEdit[idCmd-FCIDM_MOVE], OLECMDEXECOPT_PROMPTUSER, NULL, NULL);
                pcmdt->Release();
            }
        }
        return S_OK;

    case FCIDM_DELETE:
    case FCIDM_PROPERTIES:
    case FCIDM_RENAME:
        if (_HasToolbarFocus())
        {
            static const int tbtab[] = {
                FCIDM_DELETE,       FCIDM_PROPERTIES,       FCIDM_RENAME    };
            static const int cttab[] = {
                SBCMDID_FILEDELETE, SBCMDID_FILEPROPERTIES, SBCMDID_FILERENAME };

            DWORD nCmdID = SHSearchMapInt(tbtab, cttab, ARRAYSIZE(tbtab), idCmd);

            IDockingWindow* ptbar = _GetToolbarItem(_itbLastFocus)->ptbar;
            if (SUCCEEDED(IUnknown_Exec(ptbar, &CGID_Explorer, nCmdID, 0, NULL, NULL)))
                return S_OK;
        }

        SUPERCLASS::OnCommand(wParam, lParam);
        break;

    case FCIDM_VIEWAUTOHIDE:
        IUnknown_Exec(_GetITBar(), &CGID_PrivCITCommands, CITIDM_VIEWAUTOHIDE, 0, NULL, NULL);
        break;

    case FCIDM_VIEWTOOLBARCUSTOMIZE:
        IUnknown_Exec(_GetITBar(), &CGID_PrivCITCommands, CITIDM_VIEWTOOLBARCUSTOMIZE, 0, NULL, NULL);
        break;
        
    case FCIDM_VIEWTEXTLABELS:
        if (!SHIsRestricted2W(_pbbd->_hwnd, REST_NoToolbarOptions, NULL, 0))
            IUnknown_Exec(_GetITBar(), &CGID_PrivCITCommands, CITIDM_TEXTLABELS, 0, NULL, NULL);
        break;
        
    case FCIDM_EDITPAGE:
        IUnknown_Exec(_GetITBar(), &CGID_PrivCITCommands, CITIDM_EDITPAGE, 0, NULL, NULL);
        break;

    case FCIDM_FINDFILES:
        // Call Exec on ourselfes -- it's handled there
        if (!SHIsRestricted2W(_pbbd->_hwnd, REST_NoFindFiles, NULL, 0))
        {
            IDockingWindow* ptbar = _GetToolbarItem(ITB_ITBAR)->ptbar;
            VARIANT  var = {0};
            VARIANT *pvar = NULL;

            if (ptbar)
            {
                pvar = &var;
                var.vt = VT_UNKNOWN;
                var.punkVal = ptbar;
                ptbar->AddRef();
            }
            Exec(NULL, OLECMDID_FIND, OLECMDEXECOPT_PROMPTUSER, pvar, NULL);
            if (ptbar)
                ptbar->Release();
        }
        break;

    case FCIDM_CONNECT:
        DoNetConnect(_pbbd->_hwnd);
        break;

    case FCIDM_DISCONNECT:
        DoNetDisconnect(_pbbd->_hwnd);
        break;

    case FCIDM_FORTEZZA_LOGIN:
        dwError = InternetFortezzaCommand(FORTCMD_LOGON, _pbbd->_hwnd, 0);
        break;

    case FCIDM_FORTEZZA_LOGOUT:
        dwError = InternetFortezzaCommand(FORTCMD_LOGOFF, _pbbd->_hwnd, 0);
        break;

    case FCIDM_FORTEZZA_CHANGE:
        dwError = InternetFortezzaCommand(FORTCMD_CHG_PERSONALITY, _pbbd->_hwnd, 0);
        break;

    case FCIDM_BACKSPACE:
        // NT #216896: We want to use FCIDM_PREVIOUSFOLDER even for URL PIDLs if they
        //             have the folder attribute set because they could be using delegate
        //             pidls thru DefView. (FTP & Web Folders) -BryanSt
        if (_pbbd->_pidlCur && IsBrowserFrameOptionsSet(_pbbd->_psf, BFO_NO_PARENT_FOLDER_SUPPORT))
        {
            ITravelLog *ptl;
            if (SUCCEEDED(GetTravelLog(&ptl)))
            {
                ASSERT(ptl);
                if (S_OK == ptl->GetTravelEntry(SAFECAST(this, IShellBrowser *), TLOG_FORE, NULL))
                {
                    OnCommand(GET_WM_COMMAND_MPS(FCIDM_NAVIGATEBACK,
                                                  GET_WM_COMMAND_HWND(wParam, lParam),
                                                  GET_WM_COMMAND_CMD(wParam, lParam)));
                }
                ptl->Release();
            }
        } else {
            OnCommand(GET_WM_COMMAND_MPS(FCIDM_PREVIOUSFOLDER,
                                          GET_WM_COMMAND_HWND(wParam, lParam),
                                          GET_WM_COMMAND_CMD(wParam, lParam)));
        }
        break;
        
    case FCIDM_PREVIOUSFOLDER:
        // missnamed...  is really parent folder
        v_ParentFolder();
        break;

    case FCIDM_FILECLOSE:
        PostMessage(_pbbd->_hwnd, WM_CLOSE, 0, 0);
        break;

    case FCIDM_FTPOPTIONS:
        {
            VARIANT varArgs = {0};

            varArgs.vt = VT_I4;
            varArgs.lVal = SBO_NOBROWSERPAGES;
            Exec(&CGID_Explorer, SBCMDID_OPTIONS, 0, &varArgs, NULL);
        }
        break;

    case FCIDM_BROWSEROPTIONS:
        if (!SHIsRestricted2W(_pbbd->_hwnd, REST_NoBrowserOptions, NULL, 0))
            Exec(&CGID_Explorer, SBCMDID_OPTIONS, 0, NULL, NULL);
        break;

    case FCIDM_RESETWEBSETTINGS:
        ResetWebSettings(_pbbd->_hwnd, NULL);
        break;

    case FCIDM_MAIL:
#ifdef UNIX
        if ( !OEHandlesMail() )
        {
             SendDocToMailRecipient(NULL, 0, MAIL_ACTION_READ, NULL);
        }
        else
#endif
        SHRunIndirectRegClientCommand(_pbbd->_hwnd, MAIL_DEF_KEY);
        break;

    case FCIDM_MYCOMPUTER:
        {
            LPITEMIDLIST pidlMyComputer;

            SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlMyComputer);
            if (pidlMyComputer)
            {
                BrowseObject(pidlMyComputer, SBSP_SAMEBROWSER);
                ILFree(pidlMyComputer);
            }
        }
        break;

    case FCIDM_CONTACTS:    
        SHRunIndirectRegClientCommand(_pbbd->_hwnd, CONTACTS_DEF_KEY);
        break;

    case FCIDM_NEWS:
#ifdef UNIX
        if ( !CheckAndExecNewsScript( _pbbd->_hwnd ) )
#endif
        SHRunIndirectRegClientCommand(_pbbd->_hwnd, NEWS_DEF_KEY);
        break;

    case FCIDM_CALENDAR:
        SHRunIndirectRegClientCommand(_pbbd->_hwnd, CALENDAR_DEF_KEY);
        break;
    
    case FCIDM_TASKS:
        SHRunIndirectRegClientCommand(_pbbd->_hwnd, TASKS_DEF_KEY);
        break;
    
    case FCIDM_JOURNAL:
        SHRunIndirectRegClientCommand(_pbbd->_hwnd, JOURNAL_DEF_KEY);
        break;
    
    case FCIDM_NOTES:
        SHRunIndirectRegClientCommand(_pbbd->_hwnd, NOTES_DEF_KEY);
        break;
    
    case FCIDM_CALL:
        SHRunIndirectRegClientCommand(_pbbd->_hwnd, CALL_DEF_KEY);
        break;

    case FCIDM_NEWMESSAGE:
#ifdef UNIX
        if ( OEHandlesMail() )
        {
            // PORT QSY should be SendDocToMailRecipient()
            // But the function is now used to invoke native unix
            // mailers such as emacs script
            _UnixSendDocToOE(NULL, 0,  MAIL_ACTION_SEND);
        }
        else
        {
            SendDocToMailRecipient(NULL, 0, MAIL_ACTION_SEND, NULL);
        }
#else
        DropOnMailRecipient(NULL, 0);
#endif
        break;

    case FCIDM_SENDLINK:
    case FCIDM_SENDDOCUMENT:
        _SendCurrentPage(idCmd == FCIDM_SENDDOCUMENT ? FORCE_COPY : FORCE_LINK);
        break;

    case FCIDM_STARTPAGE:
    case FCIDM_UPDATEPAGE:
    case FCIDM_CHANNELGUIDE:
        {
            LPITEMIDLIST pidl = NULL;

#ifdef UNIX
            if (idCmd == FCIDM_STARTPAGE && lParam != 0)
            {
                pidl = (LPITEMIDLIST)lParam;
                BrowseObject(pidl, SBSP_SAMEBROWSER);
                ILFree(pidl);
                break;
            }
#endif

            ASSERT(IDP_START == 0);
            ASSERT(FCIDM_STARTPAGE+IDP_START == FCIDM_STARTPAGE);
            ASSERT(FCIDM_STARTPAGE+IDP_UPDATE == FCIDM_UPDATEPAGE);
            ASSERT(FCIDM_STARTPAGE+IDP_CHANNELGUIDE == FCIDM_CHANNELGUIDE);

            HRESULT hres = SHDGetPageLocation(_pbbd->_hwnd, idCmd-FCIDM_STARTPAGE, NULL, 0, &pidl);
            if (SUCCEEDED(hres)) {
                hres = BrowseObject(pidl, SBSP_SAMEBROWSER);
                ILFree(pidl);
            }
        }
        break;

    case FCIDM_SEARCHPAGE:
        {
            // This command from the Windows Explorer's Go menu used to be handled by navigating to 
            // a search page on MSN.  We now maintain consistency with the shell's handling of
            // Start->Find->On the Internet, by invoking the extension directly.

            ASSERT(FCIDM_STARTPAGE+IDP_SEARCH == FCIDM_SEARCHPAGE);

            IContextMenu *pcm; 

            HRESULT hres = CoCreateInstance(CLSID_WebSearchExt, NULL, 
                CLSCTX_INPROC_SERVER, IID_IContextMenu, (void **) &pcm);
                    
            if (SUCCEEDED(hres))
            {
                CMINVOKECOMMANDINFO ici = {0};            
                ici.cbSize = SIZEOF(ici);
                ici.nShow  = SW_NORMAL;
                pcm->InvokeCommand(&ici);
                pcm->Release();
            }
        }
        break;


    case FCIDM_HELPABOUT:
#ifdef UNIX
        IEAboutBox( _pbbd->_hwnd );
        break;
#else
    {
        TCHAR szWindows[64];
        MLLoadString(IDS_WINDOWSNT, szWindows, ARRAYSIZE(szWindows));
        ShellAbout(_pbbd->_hwnd, szWindows, NULL, NULL);
        break;
    }
#endif

    case FCIDM_HELPTIPOFTHEDAY:
        _SetBrowserBarState(-1, &CLSID_TipOfTheDay, -1);
        break;

    case FCIDM_NAVIGATEBACK:
        if (_pbbd->_psvPending)
        {
            _CancelPendingView();
        }
        else 
        {
            if (g_dwStopWatchMode & (SPMODE_BROWSER | SPMODE_JAVA))
            {
                DWORD dwTime = GetPerfTime();

                if (g_dwStopWatchMode & SPMODE_BROWSER)  // Used to get browser total download time
                    StopWatch_StartTimed(SWID_BROWSER_FRAME, TEXT("Browser Frame Back"), SPMODE_BROWSER | SPMODE_DEBUGOUT, dwTime);
                if (g_dwStopWatchMode & SPMODE_JAVA)  // Used to get java applet load time
                    StopWatch_StartTimed(SWID_JAVA_APP, TEXT("Java Applet Back"), SPMODE_JAVA | SPMODE_DEBUGOUT, dwTime);
            }
            NavigateToPidl(NULL, HLNF_NAVIGATINGBACK);
        }
        break;

    case FCIDM_NAVIGATEFORWARD:
        NavigateToPidl(NULL, HLNF_NAVIGATINGFORWARD);
        break;

    case FCIDM_ADDTOFAVNOUI:
        Exec(&CGID_Explorer, SBCMDID_ADDTOFAVORITES, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
        // Instrument this, add to favorites called by keyboard 
        UEMFireEvent(&UEMIID_BROWSER, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_ADDTOFAV, UIBL_KEYBOARD);
        break;

    // Some tools relied on the old Command ID...
    case FCIDM_W95REFRESH:
        idCmd = FCIDM_REFRESH;
        // Fall through...
    case FCIDM_REFRESH:
    case FCIDM_STOP:
    {
        SHELLSTATE ss = {0};
        SHGetSetSettings(&ss, SSF_MAPNETDRVBUTTON, FALSE);
        if ((!_fShowNetworkButtons && ss.fMapNetDrvBtn) ||
            (_fShowNetworkButtons && !ss.fMapNetDrvBtn))
        {
            UINT uiBtnState = 0;
            _fShowNetworkButtons = ss.fMapNetDrvBtn;
            _pxtb->GetState(&CLSID_CommonButtons, TBIDM_CONNECT, &uiBtnState);
            if (ss.fMapNetDrvBtn)
                uiBtnState &= ~TBSTATE_HIDDEN;
            else    
                uiBtnState |= TBSTATE_HIDDEN;
            _pxtb->SetState(&CLSID_CommonButtons, TBIDM_CONNECT, uiBtnState);
            _pxtb->SetState(&CLSID_CommonButtons, TBIDM_DISCONNECT, uiBtnState);
        }

        if (idCmd == FCIDM_REFRESH)
        {
            VARIANT v = {0};
            v.vt = VT_I4;
            v.lVal = OLECMDIDF_REFRESH_NO_CACHE|OLECMDIDF_REFRESH_PROMPTIFOFFLINE;
            Exec(NULL, OLECMDID_REFRESH, OLECMDEXECOPT_DONTPROMPTUSER, &v, NULL);

            // Refresh the toolbar
            if (_pxtb)
            {
                IServiceProvider* psp;
                if (SUCCEEDED(_pxtb->QueryInterface(IID_IServiceProvider, (void**)&psp)))
                {
                    IAddressBand *pab = NULL;
                    if (SUCCEEDED(psp->QueryService(IID_IAddressBand, IID_IAddressBand, (void**)&pab)))
                    {
                        VARIANTARG varType = {0};
                        varType.vt = VT_I4;
                        varType.lVal = OLECMD_REFRESH_TOPMOST;
                        pab->Refresh(&varType);
                        pab->Release();
                    }
                    psp->Release();
                }
            }
        }
        else
        {
            if (g_dwStopWatchMode & SPMODE_BROWSER)
                StopWatch_Lap(SWID_BROWSER_FRAME | SWID_MASK_BROWSER_STOPBTN, TEXT("Browser Frame Esc"), SPMODE_BROWSER | SPMODE_DEBUGOUT);

            Exec(NULL, OLECMDID_STOP, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
        }
        break;
    }
    

#ifndef DISABLE_FULLSCREEN

    case FCIDM_THEATER:
        if(!SHRestricted2(REST_NoTheaterMode, NULL, 0))
        {
            // Toggle theater mode.  Don't allow theater mode if we're in kiosk mode.
            if (_ptheater || _fKioskMode) {
                _TheaterMode(FALSE, TRUE);
            } else {
                _TheaterMode(TRUE, FALSE);
            }
        }
        break;

#endif

    case FCIDM_NEXTCTL:
        _CycleFocus(NULL);
        break;

    case FCIDM_VIEWOFFLINE:
        if ((!SHIsGlobalOffline()) && (IsCriticalOperationPending()))
        {
            if (MLShellMessageBox(_pbbd->_hwnd,
                    MAKEINTRESOURCE(IDS_CANCELFILEDOWNLOAD),
                    MAKEINTRESOURCE(IDS_FILEDOWNLOADCAPTION),
                    MB_YESNO | MB_ICONSTOP) == IDNO)
                break;
        }
    
        Offline(SBSC_TOGGLE);
        if (_pbbd->_pszTitleCur)
            _SetTitle(_pbbd->_pszTitleCur);
            
        break;


#ifdef TEST_AMBIENTS
    case FCIDM_VIEWLOCALOFFLINE:
        _LocalOffline(SBSC_TOGGLE);
        break;

    case FCIDM_VIEWLOCALSILENT:
        _LocalSilent(SBSC_TOGGLE);
        break;
#endif // TEST_AMBIENTS


    case FCIDM_VIEWTOOLBAR:
        v_ShowControl(FCW_INTERNETBAR, SBSC_TOGGLE);
        break;

    case FCIDM_VIEWTOOLS:
        id = CITIDM_VIEWTOOLS;
        goto ITBarShowBand;
        
    case FCIDM_VIEWADDRESS:
        id = CITIDM_VIEWADDRESS;
        goto ITBarShowBand;
        
    case FCIDM_VIEWLINKS:
        id = CITIDM_VIEWLINKS;
        goto ITBarShowBand;
            
ITBarShowBand:
        if (!SHIsRestricted2W(_pbbd->_hwnd, REST_NoToolbarOptions, NULL, 0))
            IUnknown_Exec(_GetITBar(), &CGID_PrivCITCommands, id, 0, NULL, NULL);
        break;
                
    case FCIDM_VIEWSTATUSBAR:    
        v_ShowControl(FCW_STATUS, SBSC_TOGGLE);                    
        break;
        
    case FCIDM_VBBSEARCHBAND:
        {
            IDockingWindow* ptbar = _GetToolbarItem(ITB_ITBAR)->ptbar;
            VARIANT  var = {0};

            var.vt = VT_I4;
            var.lVal = -1;
            IUnknown_Exec(ptbar, &CLSID_CommonButtons, TBIDM_SEARCH, 0, NULL, &var);
        }
        break;
    
    case FCIDM_VBBEXPLORERBAND:
    case FCIDM_VBBFAVORITESBAND:
    case FCIDM_VBBHISTORYBAND:
#ifdef ENABLE_CHANNELPANE
    case FCIDM_VBBCHANNELSBAND:
#endif
        if (g_dwStopWatchMode)
        {
            StopWatch_Start(SWID_EXPLBAR, TEXT("Shell bar Start"), SPMODE_SHELL | SPMODE_DEBUGOUT);
        }
        
        if ((idCmd != FCIDM_VBBFAVORITESBAND) 
            || (!SHIsRestricted2W(_pbbd->_hwnd, REST_NoFavorites, NULL, 0)))
            _SetBrowserBarState(idCmd, NULL, -1);
        
        if (g_dwStopWatchMode)
        {
            TCHAR szText[100];
            TCHAR szMenu[32];
            DWORD dwTime = GetPerfTime();
            GetMenuString(_GetMenuFromID(FCIDM_MENU_VIEW), idCmd, szMenu, ARRAYSIZE(szMenu) - 1, MF_BYCOMMAND);
            wnsprintf(szText, ARRAYSIZE(szText) - 1, TEXT("Shell %s bar Stop"), szMenu);
            StopWatch_StopTimed(SWID_EXPLBAR, (LPCTSTR)szText, SPMODE_SHELL | SPMODE_DEBUGOUT, dwTime);
        }
        break;

    case FCIDM_JAVACONSOLE:
        ShowJavaConsole();
        break;

    case FCIDM_SHOWSCRIPTERRDLG:
        {
            HRESULT hr;

            hr = Exec(&CGID_ShellDocView,
                      SHDVID_DISPLAYSCRIPTERRORS,
                      0,
                      NULL,
                      NULL);

            return hr;
        }
        break;

    default:
        if (IsInRange(idCmd, FCIDM_FAVORITECMDFIRST, FCIDM_FAVORITECMDLAST) 
            && !SHIsRestricted2W(_pbbd->_hwnd, REST_NoFavorites, NULL, 0)) {
            _FavoriteOnCommand(NULL, idCmd);
        } else if (IsInRange(idCmd, FCIDM_RECENTFIRST, FCIDM_RECENTLAST)) {
            ITravelLog *ptl;
            GetTravelLog(&ptl);
            if (ptl)
            {
                ptl->Travel(SAFECAST(this, IShellBrowser *), idCmd - (FCIDM_RECENTFIRST + GOMENU_RECENT_ITEMS) + GOMENU_RECENT_ITEMS / 2);
                ptl->Release();
                UpdateBackForwardState();
            }
        } else if (IsInRange(idCmd, FCIDM_SEARCHFIRST, FCIDM_SEARCHLAST)) {
            if (_pcmSearch)
            {
                CMINVOKECOMMANDINFO ici = {0};
            
                ici.cbSize = SIZEOF(ici);
                //ici.hwnd = NULL; // no need for hwnd for search cm InvokeCommand
                ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - FCIDM_SEARCHFIRST);
                ici.nShow  = SW_NORMAL;
                _pcmSearch->InvokeCommand(&ici);
            }
            else
                TraceMsg(DM_TRACE, "find cmd with NULL pcmFind");
        } else if (IsInRange(idCmd, FCIDM_MENU_TOOLS_FINDFIRST, FCIDM_MENU_TOOLS_FINDLAST)) {
            if (GetUIVersion() < 5 && _pcmFind)
            {
                LPITEMIDLIST pidl = (_pbbd->_pidlPending) ? _pbbd->_pidlPending : _pbbd->_pidlCur;
                TCHAR szPath[MAX_PATH];

                SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, SIZECHARS(szPath), NULL);

                // Handle cases like "desktop" (make it default to My Computer)
                if (!PathIsDirectory(szPath))
                {
                    szPath[0] = TEXT('\0');
                }

                CMINVOKECOMMANDINFO ici = {0};
            
                ici.cbSize = SIZEOF(ici);
                //ici.hwnd = NULL; // no need for hwnd for search cm InvokeCommand
                ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - FCIDM_MENU_TOOLS_FINDFIRST);
                ici.nShow  = SW_NORMAL;

                // Set the root of the search
                char szAnsiPath[MAX_PATH];
                szAnsiPath[0] = '\0';
                SHTCharToAnsi(szPath, szAnsiPath, ARRAYSIZE(szAnsiPath));                
                ici.lpDirectory = szAnsiPath;

                _pcmFind->InvokeCommand(&ici);
            }
        } else if (IsInRange(idCmd, FCIDM_VBBDYNFIRST, FCIDM_VBBDYNLAST)) {
            _SetBrowserBarState(idCmd, NULL, -1);
        } else if (IsInRange(idCmd, FCIDM_FILECTX_FIRST, FCIDM_FILECTX_LAST)) {
            _ExecFileContext(idCmd);
        } else if (IsInRange( idCmd, FCIDM_EXTERNALBANDS_FIRST, FCIDM_EXTERNALBANDS_LAST )) {
            id = idCmd - FCIDM_EXTERNALBANDS_FIRST + CITIDM_VIEWEXTERNALBAND_FIRST;
            IUnknown_Exec(_GetITBar(), &CGID_PrivCITCommands, id, 0, NULL, NULL);
        } else {
            SUPERCLASS::OnCommand(wParam, lParam);
        }
        break;
    }
    return S_OK;
}

HMENU CShellBrowser2::_GetMenuFromID(UINT uID)
{
    return SHGetMenuFromID(_hmenuCur, uID);
}


BOOL AddItemToGoMenu(HMENU hMenu, UINT uiCmd, LPCTSTR pszRegKey)
{
    if (!SHIsRegisteredClient(pszRegKey) || SHRestricted2(REST_GoMenu, NULL, 0))
    {
        DeleteMenu(hMenu, uiCmd, MF_BYCOMMAND);
        return FALSE;
    }
    return TRUE;
}    
        
// BUGBUG: fix me tjgreen        
void CShellBrowser2::_AddMailNewsMenu(HMENU hmenu)
{
    // actually, this removes the mail & news menu if it's not registered
    // and if neither are there, remove the separator too

    BOOL fAddSeparator = FALSE;
    fAddSeparator |= AddItemToGoMenu(hmenu, FCIDM_MAIL, MAIL_DEF_KEY);
    fAddSeparator |= AddItemToGoMenu(hmenu, FCIDM_NEWS, NEWS_DEF_KEY);
    fAddSeparator |= AddItemToGoMenu(hmenu, FCIDM_CONTACTS, CONTACTS_DEF_KEY);
    fAddSeparator |= AddItemToGoMenu(hmenu, FCIDM_CALENDAR, CALENDAR_DEF_KEY);
    fAddSeparator |= AddItemToGoMenu(hmenu, FCIDM_TASKS, TASKS_DEF_KEY);
    fAddSeparator |= AddItemToGoMenu(hmenu, FCIDM_JOURNAL, JOURNAL_DEF_KEY);
    fAddSeparator |= AddItemToGoMenu(hmenu, FCIDM_NOTES, NOTES_DEF_KEY);
    fAddSeparator |= AddItemToGoMenu(hmenu, FCIDM_CALL, CALL_DEF_KEY);

    if (!fAddSeparator)
        DeleteMenu(hmenu, FCIDM_MAILNEWSSEPARATOR, MF_BYCOMMAND);
}

void CShellBrowser2::_ExecFileContext(UINT idCmd)
{
    if (_pcmNsc)
    {
        CMINVOKECOMMANDINFO ici = {
            sizeof(CMINVOKECOMMANDINFO),
                0L,
                _pbbd->_hwnd,
                MAKEINTRESOURCEA(idCmd-FCIDM_FILECTX_FIRST),
                NULL, 
                NULL,
                SW_NORMAL,
        };
        _pcmNsc->InvokeCommand(&ici);
   }
}

void CShellBrowser2::_EnableFileContext(HMENU hmenuPopup)
{
    IContextMenu2 *pcm = NULL;
    MENUITEMINFO mii = {0};
    int nPos;
    int nLast;
    OLECMDTEXTV<MAX_FILECONTEXT_STRING> cmdtv;
    OLECMDTEXT *pcmdText = &cmdtv;
    HRESULT hres;
    BOOL fAddSeparator = TRUE;
    BOOL fFound = FALSE;
    USES_CONVERSION;

    ATOMICRELEASE(_pcmNsc);
    //  If we have a name space control visible, use QueryStatus to find out if it has
    //  a selected item that we can put a context menu for.
    if (_poctNsc)
    {
        OLECMD rgcmd = { SBCMDID_INITFILECTXMENU, 0 };
        

        pcmdText->cwBuf = MAX_FILECONTEXT_STRING;
        pcmdText->cmdtextf = OLECMDTEXTF_NAME;
        pcmdText->rgwz[0] = 0;
        _poctNsc->QueryStatus(&CGID_Explorer, 1, &rgcmd, pcmdText);
        if (rgcmd.cmdf & OLECMDF_ENABLED) 
        {
            VARIANT var;

            VariantInit (&var);
            hres = _poctNsc->Exec(&CGID_Explorer, SBCMDID_INITFILECTXMENU, OLECMDEXECOPT_PROMPTUSER, NULL, &var);
            if (SUCCEEDED(hres) && VT_UNKNOWN == var.vt && NULL != var.punkVal)
            {
                var.punkVal->QueryInterface(IID_IContextMenu2, (void **) &pcm);
            }
            VariantClearLazy(&var);
        }

    }

    mii.cbSize = SIZEOF(MENUITEMINFO);
    mii.fMask = MIIM_ID | MIIM_TYPE;

    nLast = GetMenuItemCount(hmenuPopup);
    for (nPos = 0; nPos < nLast ; nPos++)
    {
        mii.cch = 0;
        GetMenuItemInfoWrap(hmenuPopup, nPos, TRUE, &mii);
        if (mii.wID == FCIDM_FILENSCBANDSEP)
        {
            //  Delete the old menu
            nPos--;
            DeleteMenu(hmenuPopup, nPos, MF_BYPOSITION);
            // nPos now points at FCIDM_NSCBANDSEP
            //  Delete the separator, too, if no context menu to add
            if (NULL == pcm)
            {
                DeleteMenu(hmenuPopup, FCIDM_FILENSCBANDSEP, MF_BYCOMMAND);
                ASSERT(!pcm);       // o.w. need nPos-- (i think...)
                // (no pcm so don't use nPos so punt nPos--);
            }
            else
            {
                fAddSeparator = FALSE;
            }
            fFound = TRUE;
            break;
        }
        else if (mii.wID == FCIDM_VIEWOFFLINE || mii.wID == FCIDM_FILECLOSE)
        {
            //  there wasn't a context item
            fFound = TRUE;
            break;
        }

#ifdef TEST_AMBIENTS
       else if ((mii.wID == FCIDM_VIEWLOCALOFFLINE) || 
                (mii.wID == FCIDM_VIEWLOCALSILENT))
       {
            fFound = TRUE;
            break;
       }


#endif // TEST_AMBIENTS


       
    }

    if (pcm)
    {
        if (fFound)
        {
            HMENU hmenu = CreatePopupMenu();
            if (hmenu)
            {
                pcm->QueryContextMenu(hmenu,
                    0, 
                    FCIDM_FILECTX_FIRST, 
                    FCIDM_FILECTX_LAST,
                    CMF_EXPLORE);
                if (fAddSeparator)
                {
#ifdef DEBUG
                    MENUITEMINFO mii = {SIZEOF(MENUITEMINFO), MIIM_ID};
                    GetMenuItemInfoWrap(hmenuPopup, nPos, TRUE, &mii);
                    TraceMsg(DM_TRACE, "nPos=%d idm=%x i_vo=%x i_fc=%x", nPos, mii.wID, FCIDM_VIEWOFFLINE, FCIDM_FILECLOSE);
                    ASSERT(mii.wID == FCIDM_VIEWOFFLINE || mii.wID == FCIDM_FILECLOSE);
#endif
                    InsertMenu(hmenuPopup,
                               nPos,
                               MF_BYPOSITION|MF_SEPARATOR,
                               FCIDM_FILENSCBANDSEP,
                               NULL);

#ifdef TEST_AMBIENTS
                    InsertMenu(hmenuPopup,
                               FCIDM_VIEWLOCALOFFLINE,
                               MF_BYCOMMAND|MF_SEPARATOR,
                               FCIDM_FILENSCBANDSEP,
                               NULL);
                    InsertMenu(hmenuPopup,
                               FCIDM_VIEWLOCALSILENT,
                               MF_BYCOMMAND|MF_SEPARATOR,
                               FCIDM_FILENSCBANDSEP,
                               NULL);
#endif // TEST_AMBIENTS

                }
                InsertMenu(hmenuPopup,
                           FCIDM_FILENSCBANDSEP,
                           MF_BYCOMMAND|MF_POPUP,
                           (UINT_PTR) hmenu,
                           W2T(pcmdText->rgwz));
            }
            _pcmNsc = pcm;
        }
        else
        {
            pcm->Release();
        }
    }
}

void CShellBrowser2::_MungeGoMyComputer(HMENU hmenuPopup)
{
    //need to have a menu item My Computer but a user might have changed
    //it to something else so go get the new name
    LPITEMIDLIST pidlMyComputer;
    TCHAR szBuffer[MAX_PATH]; //buffer to hold menu item string
    TCHAR szMenuText[MAX_PATH+1+6];

    SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlMyComputer);
    if (pidlMyComputer)
    {
        if (SUCCEEDED(SHGetNameAndFlags(pidlMyComputer, SHGDN_NORMAL, szMenuText, SIZECHARS(szMenuText), NULL)))
        {   
            MENUITEMINFO mii;

            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_TYPE;
            mii.fType = MFT_STRING;
            mii.dwTypeData = szBuffer;
            mii.cch = ARRAYSIZE(szBuffer);
            if (GetMenuItemInfoWrap(hmenuPopup, FCIDM_MYCOMPUTER, FALSE, &mii)) 
            {
                LPTSTR  pszHot;
                LPTSTR pszMenuItem = (LPTSTR) mii.dwTypeData;
                
                //before we get rid of the old name, need to get 
                //the hot key for it
                //check if the old name had a hot key
                //StrChr is defined in shlwapi (strings.c) and accepts word even in ascii version
                if (NULL != (pszHot = StrChr(pszMenuItem, (WORD)TEXT('&'))))
                {   //yes
                    LPTSTR   psz;
                    DWORD   cch;

                    pszHot++; //make it point to the hot key, not &
                    //try to find the key in the new string
                    if (NULL == (psz = StrChr(szMenuText, (WORD)*pszHot)))
                    {   //not found, then we'll insert & at the beginning of the new string
                        psz = szMenuText;
                    }

                    // can't put hotkey to full width characters
                    // and some of japanese specific half width chars.
                    // the comparison 
                    BOOL fFEmnemonic = FALSE;
                    if (g_fRunOnFE)
                    {
                        WORD wCharType[2];
                        // if built Ansi it takes max. 2 bytes to determine if 
                        // the given character is full width.
                        // DEFAULT_SYSTEM_LOCALE has to change when we have a way
                        // to get current UI locale.
                        //
                        GetStringTypeEx(LOCALE_SYSTEM_DEFAULT, CT_CTYPE3, psz, 
                                        sizeof(WCHAR)/sizeof(TCHAR), wCharType);

                        if ((wCharType[0] & C3_FULLWIDTH)
                           ||(wCharType[0] & C3_KATAKANA ) 
                           ||(wCharType[0] & C3_IDEOGRAPH )
                           ||((wCharType[0] & C3_ALPHA) && !(wCharType[0] & C3_HALFWIDTH)))
                        {
                            fFEmnemonic = TRUE;
                        }
                    }

                    if (fFEmnemonic)
                    {
                        // assume we have room for hotkey...
                        ASSERT(lstrlen(szMenuText) < ARRAYSIZE(szMenuText));
                        StrCat(szMenuText, TEXT("(&"));
                        cch = lstrlen(szMenuText);
                        szMenuText[cch] = *pszHot;
                        StrCpy(&szMenuText[cch+1], TEXT(")"));
                    }
                    else
                    {
                        cch = lstrlen(psz) + 1;
                        //make space for & to be inserted
                        memmove(psz+1, psz, cch * sizeof(TCHAR));
                        psz[0] = TEXT('&');
                    }
                }

                mii.dwTypeData = szMenuText;
                SetMenuItemInfo(hmenuPopup, FCIDM_MYCOMPUTER, FALSE, &mii);
            }
        }
        ILFree(pidlMyComputer);
    }
}

inline BOOL IsWebPidl(LPCITEMIDLIST pidl)
{
    return (!pidl || ILIsWeb(pidl));
}

void CShellBrowser2::_InsertTravelLogItems(HMENU hmenu, int nPos)
{
    ITravelLog *ptl;
            
    GetTravelLog(&ptl);
    if (!ptl)
        return;

    //add the back items to the menu
    MENUITEMINFO mii = {0};
    mii.cbSize = SIZEOF(MENUITEMINFO);
    mii.fMask = MIIM_ID | MIIM_TYPE;

    //delete all back menu items after the separator

    for (int i=GetMenuItemCount(hmenu); i >=0; i--)
    {
        mii.cch = 0;
        if (GetMenuItemInfoWrap(hmenu, i, TRUE, &mii) &&
            IsInRange(mii.wID, FCIDM_RECENTMENU, FCIDM_RECENTLAST))
        {
            DeleteMenu(hmenu, i, MF_BYPOSITION);
            if (i < nPos)
                nPos--;
        }
    }       

    //add the items
    if (S_OK == ptl->InsertMenuEntries(SAFECAST(this, IShellBrowser*), hmenu, nPos, FCIDM_RECENTFIRST, 
                           FCIDM_RECENTFIRST + GOMENU_RECENT_ITEMS, TLMENUF_CHECKCURRENT | TLMENUF_BACKANDFORTH))
    {
        //if something was added, insert a separator
        MENUITEMINFO mii = {0};
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask  = MIIM_ID | MIIM_TYPE;
        mii.fType  = MFT_SEPARATOR;
        mii.wID    = FCIDM_RECENTMENU;
        
        InsertMenuItem(hmenu, nPos, TRUE, &mii);
    }
    
    ptl->Release();
}

void CShellBrowser2::_OnGoMenuPopup(HMENU hmenuPopup)
{
    ITravelLog *ptl;

    GetTravelLog(&ptl);
    // if we've got a site or if we're trying to get to a site,
    // enable the back button
    BOOL fBackward = (ptl ? S_OK == ptl->GetTravelEntry(SAFECAST(this, IShellBrowser *), TLOG_BACK, NULL) : FALSE);
    _EnableMenuItem(hmenuPopup, FCIDM_NAVIGATEBACK, fBackward);

    BOOL fForeward = (ptl ? S_OK == ptl->GetTravelEntry(SAFECAST(this, IShellBrowser *), TLOG_FORE, NULL) : FALSE);
    _EnableMenuItem(hmenuPopup, FCIDM_NAVIGATEFORWARD, fForeward);

    if (IsBrowserFrameOptionsSet(_pbbd->_psf, BFO_NO_PARENT_FOLDER_SUPPORT) ||
        SHRestricted2W(REST_GoMenu, NULL, 0)) 
    {
        DeleteMenu(hmenuPopup, FCIDM_PREVIOUSFOLDER, MF_BYCOMMAND);
    }
    else
        _EnableMenuItem(hmenuPopup, FCIDM_PREVIOUSFOLDER, _ShouldAllowNavigateParent());

    ATOMICRELEASE(ptl);

    if (SHRestricted2(REST_NoChannelUI, NULL, 0))
        DeleteMenu(hmenuPopup, FCIDM_CHANNELGUIDE, MF_BYCOMMAND);

    _MungeGoMyComputer(hmenuPopup);

    _AddMailNewsMenu(hmenuPopup);
    
    // if in ie4 shell browser we leave travel log in the file menu
    if ((GetUIVersion() >= 5 || !_pbbd->_psf || IsBrowserFrameOptionsSet(_pbbd->_psf, BFO_GO_HOME_PAGE)))
    {
        // The travel log goes after "Home Page".
        int nPos = GetMenuPosFromID(hmenuPopup, FCIDM_STARTPAGE) + 1;

        // If "Home Page" isn't there, then just append to the end.
        if (nPos <= 0)
        {
            nPos = GetMenuItemCount(hmenuPopup);
        }

        _InsertTravelLogItems(hmenuPopup, nPos);
    }
}


HRESULT AssureFtpOptionsMenuItem(HMENU hmenuPopup)
{
    HRESULT hr = S_OK;

    // Append the item if it is missing.  It can be missing because
    // sometimes the menu will be displayed before the shell merges it's
    // menus, so we are modifying the template that will be used for other
    // pages.
    if (GetMenuPosFromID(hmenuPopup, FCIDM_FTPOPTIONS) == 0xFFFFFFFF)
    {
        // Yes, it's missing so we need to add it.
        int nToInsert = GetMenuPosFromID(hmenuPopup, FCIDM_BROWSEROPTIONS);

        if (EVAL(0xFFFFFFFF != nToInsert))
        {
            TCHAR szInternetOptions[64];
            MLLoadString(IDS_INTERNETOPTIONS, szInternetOptions, ARRAYSIZE(szInternetOptions));

            MENUITEMINFO mii = {0};
            mii.cbSize = sizeof(mii);
            mii.fMask = (MIIM_TYPE | MIIM_STATE | MIIM_ID);
            mii.fType = MFT_STRING;
            mii.fState = MFS_ENABLED | MFS_UNCHECKED;
            mii.wID = FCIDM_FTPOPTIONS;
            mii.dwTypeData = szInternetOptions;
            mii.cch   = lstrlen(szInternetOptions);

            // We want to go right after "Folder Options" so we found
            // the spot.
            TBOOL(InsertMenuItem(hmenuPopup, (nToInsert + 1), TRUE, &mii));

            // BUGBUG: ALL of this code just sucks.  The PMs finally decided that
            //         it would be good to always have "Inet Options" & "Folder Options"
            //         in all views. (FTP, Shell, & Web)  However doing it now
            //         is too late so we want to do this later.  When that is done
            //         we can get ride of all this crap.

            // Now we just want to make sure FCIDM_BROWSEROPTIONS is "Folder Options"
            // because some users over load it to say "Internet Options" which
            // just added above.  So we want to force it back to "Folder Options".
            if (GetMenuItemInfo(hmenuPopup, FCIDM_BROWSEROPTIONS, FALSE, &mii))
            {
                TCHAR szFolderOptions[MAX_PATH];

                MLLoadString(IDS_FOLDEROPTIONS, szFolderOptions, ARRAYSIZE(szFolderOptions));
                mii.dwTypeData = szFolderOptions;
                mii.cch   = lstrlen(szFolderOptions);
                SetMenuItemInfo(hmenuPopup, FCIDM_BROWSEROPTIONS, FALSE, &mii);
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}


HRESULT UpdateOptionsMenuItem(IShellFolder * psf, HMENU hmenuPopup, BOOL fForNT5)
{
    BOOL fCorrectVersion;

    if (fForNT5)
        fCorrectVersion = (GetUIVersion() >= 5);
    else
        fCorrectVersion = (GetUIVersion() < 5);

    // We want "Internet Options" in addition to "Folder Options" on
    // NT5's Tools menu for FTP Folders.  Is this the case?
    if (fCorrectVersion &&
        IsBrowserFrameOptionsSet(psf, BFO_BOTH_OPTIONS))
    {
        EVAL(SUCCEEDED(AssureFtpOptionsMenuItem(hmenuPopup)));
    }
    else
    {
        // No, so delete the item.
        DeleteMenu(hmenuPopup, FCIDM_FTPOPTIONS, MF_BYCOMMAND);
    }

    return S_OK;
}

void CShellBrowser2::_OnViewMenuPopup(HMENU hmenuPopup)
{
    OLECMD rgcmd[] = {
        { CITIDM_VIEWTOOLS, 0 },
        { CITIDM_VIEWADDRESS, 0 },
        { CITIDM_VIEWLINKS, 0 },
        { CITIDM_VIEWTOOLBARCUSTOMIZE, 0 },
        { CITIDM_VIEWMENU, 0 },
        { CITIDM_VIEWAUTOHIDE, 0 },
        { CITIDM_TEXTLABELS, 0 },
    };
    
    UpdateOptionsMenuItem(_pbbd->_psf, hmenuPopup, FALSE);

    //  See _MenuTemplate for the kooky enable/disable scenarios.
    //  Today's kookiness:  The ever-changing "Options" menuitem.
    //  According to the table, we want Options under View on
    //  Non-NT5, in the shell or FTP scenarios.  Therefore, we want
    //  options deleted in the opposite scenario.  And for good measure,
    //  we also delete it if we don't know who we are yet, or if we
    //  are restricted.
    if (SHRestricted(REST_NOFOLDEROPTIONS) ||
        (GetUIVersion() >= 5) || !_pbbd->_pidlCur || 
        IsBrowserFrameOptionsSet(_pbbd->_psf, BFO_RENAME_FOLDER_OPTIONS_TOINTERNET))
    {
        DeleteMenu(hmenuPopup, FCIDM_BROWSEROPTIONS, MF_BYCOMMAND);
    }

    if(SHRestricted2(REST_NoViewSource, NULL, 0))
        _EnableMenuItem(hmenuPopup, DVIDM_MSHTML_FIRST+IDM_VIEWSOURCE, FALSE);

    if (_GetToolbarItem(ITB_ITBAR)->fShow) {
        IUnknown_QueryStatus(_GetITBar(), &CGID_PrivCITCommands, ARRAYSIZE(rgcmd), rgcmd, NULL);
    }

    HMENU hmenuToolbar = LoadMenuPopup(MENU_ITOOLBAR);

    MENUITEMINFO mii;
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_SUBMENU;
    mii.hSubMenu = hmenuToolbar;
    SetMenuItemInfo(_hmenuCur, FCIDM_VIEWTOOLBAR, FALSE, &mii); // BUGBUG: why _hmenuCur?  why not hmenuPopup?

    _CheckMenuItem(hmenuToolbar, FCIDM_VIEWADDRESS, rgcmd[1].cmdf & OLECMDF_ENABLED);
    _CheckMenuItem(hmenuToolbar, FCIDM_VIEWLINKS, rgcmd[2].cmdf & OLECMDF_ENABLED);

    int cItemsBelowSep = 3;
    BOOL fCustomizeAvailable = TRUE;
    if (!(rgcmd[3].cmdf & OLECMDF_ENABLED)) {
        DeleteMenu(hmenuToolbar, FCIDM_VIEWTOOLBARCUSTOMIZE, MF_BYCOMMAND);
        fCustomizeAvailable = FALSE;
        cItemsBelowSep--;
    }

    DeleteMenu(hmenuToolbar, FCIDM_VIEWGOBUTTON, MF_BYCOMMAND);

    if (fCustomizeAvailable || _ptheater || 
        SHRestricted2(REST_LOCKICONSIZE, NULL, 0)) {
        DeleteMenu(hmenuToolbar, FCIDM_VIEWTEXTLABELS, MF_BYCOMMAND);
        cItemsBelowSep--;
    } else {
        _CheckMenuItem (hmenuToolbar, FCIDM_VIEWTEXTLABELS, rgcmd[6].cmdf);
    }
    
    if (_ptheater) {
        _CheckMenuItem (hmenuToolbar, FCIDM_VIEWMENU, rgcmd[4].cmdf);
        _CheckMenuItem (hmenuToolbar, FCIDM_VIEWAUTOHIDE, rgcmd[5].cmdf);
        DeleteMenu(hmenuToolbar, FCIDM_VIEWTOOLS, MF_BYCOMMAND);
    } else {
        _CheckMenuItem(hmenuToolbar, FCIDM_VIEWTOOLS, rgcmd[0].cmdf & OLECMDF_ENABLED);
        DeleteMenu(hmenuToolbar, FCIDM_VIEWMENU, MF_BYCOMMAND);
        DeleteMenu(hmenuToolbar, FCIDM_VIEWAUTOHIDE, MF_BYCOMMAND);
        cItemsBelowSep--;
    }
    
    _CheckMenuItem(hmenuPopup, FCIDM_VIEWSTATUSBAR,
                  v_ShowControl(FCW_STATUS, SBSC_QUERY) == SBSC_SHOW);

#ifndef DISABLE_FULLSCREEN
    if(SHRestricted2(REST_NoTheaterMode, NULL, 0))
        _EnableMenuItem(hmenuPopup, FCIDM_THEATER, FALSE);
    else
        _CheckMenuItem(hmenuPopup, FCIDM_THEATER, (_ptheater ? TRUE : FALSE ) );
#endif

    // if we're on nt5 OR we're not integrated and we're not in explorer
    // add browser bars to the view menu
    if (_GetBrowserBarMenu() == hmenuPopup)
    {
        _AddBrowserBarMenuItems(hmenuPopup);
    }
    // else it gets added only on view/explorer bars

    RestrictItbarViewMenu( hmenuPopup, _GetITBar() );
    if (!cItemsBelowSep) 
        DeleteMenu(hmenuToolbar, FCIDM_VIEWCONTEXTMENUSEP, MF_BYCOMMAND);

    DWORD dwValue;
    DWORD dwSize = SIZEOF(dwValue);
    BOOL  fDefault = FALSE;

    // Check the registry to see if we need to show the "Java Console" menu item.
    //
    SHRegGetUSValue(TEXT("Software\\Microsoft\\Java VM"),
        TEXT("EnableJavaConsole"), NULL, (LPBYTE)&dwValue, &dwSize, FALSE, 
        (void *) &fDefault, SIZEOF(fDefault));

    // If the value is false or absent, remove the menu item.
    //
    if (!dwValue)
    {
        RemoveMenu(hmenuPopup, FCIDM_JAVACONSOLE, MF_BYCOMMAND);
    }

    //  Component categories cache can be passively (and efficiently) kept consistent through
    //  a registry change notification in integrated platforms on NT and Win=>98.
    //  both of these do an async update as necessary.
    if (GetUIVersion() > 4)
    {
        _QueryHKCRChanged();
    }
    else if (!_fValidComCatCache)
    {
        //  With browser-only, we'll refresh only if we haven't done so already.
        _fValidComCatCache = 
            S_OK == _FreshenComponentCategoriesCache( TRUE /* unconditional update */) ;
    }

    // prettify the menu (make sure first and last items aren't
    // separators and that there are no runs of >1 separator)
    _SHPrettyMenu(hmenuPopup);
}


void CShellBrowser2::_OnToolsMenuPopup(HMENU hmenuPopup)
{
    // Party on tools->options
    //
    //  Again _MenuTemplate has the gory details.  We want to lose
    //  "Options" in the non-NT5, shell or FTP scenarios, so we want
    //  to keep it in the opposite case.  (FTP is a freebie since FTP
    //  doesn't have a Tools menu to begin with.)
    //
    //  And don't forget restrictions.
    //  And as a bonus, we have to change the name of the menuitem
    //  in the web scenario to "Internet &Options".
    BOOL fWeb = IsWebPidl(_pbbd->_pidlCur);

    UpdateOptionsMenuItem(_pbbd->_psf, hmenuPopup, TRUE);

    //
    // Figure out whether or not "reset web settings" is needed
    //
    if (!fWeb ||                            // only visible in web mode
        !IsResetWebSettingsEnabled() ||     // only if not disabled by the ieak
        !IsResetWebSettingsRequired())      // and only needed if someone clobbered our reg keys
    {
        DeleteMenu(hmenuPopup, FCIDM_RESETWEBSETTINGS, MF_BYCOMMAND);
    }


    DWORD dwOptions;
    GetBrowserFrameOptions(_pbbd->_psf, (BFO_RENAME_FOLDER_OPTIONS_TOINTERNET | BFO_BOTH_OPTIONS), &dwOptions);   

    DWORD rgfAttrib = SFGAO_FOLDER;
    //Qfe 3316: Check folder option restriction only if we have new shell
    if ((SHRestricted(REST_NOFOLDEROPTIONS) && GetUIVersion() > 3)||
        ((GetUIVersion() < 5) && !(BFO_NONE == dwOptions) &&
         SUCCEEDED(IEGetAttributesOf(_pbbd->_pidlCur, &rgfAttrib)) && (rgfAttrib & SFGAO_FOLDER)))
    {
        DeleteMenu(hmenuPopup, FCIDM_BROWSEROPTIONS, MF_BYCOMMAND);
    }
    else
    {
        // Only do this if the NSE wants it named "Internet Options" but doesn't want "Folder Options"
        // also.
        if (IsBrowserFrameOptionsSet(_pbbd->_psf, BFO_RENAME_FOLDER_OPTIONS_TOINTERNET))
        {
            TCHAR szInternetOptions[64];

            MLLoadString(IDS_INTERNETOPTIONS, szInternetOptions, ARRAYSIZE(szInternetOptions));

            MENUITEMINFO mii;
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
            mii.fType = MFT_STRING;
            mii.fState = MFS_ENABLED | MFS_UNCHECKED;
            mii.dwTypeData = szInternetOptions;
            mii.cch   = lstrlen(szInternetOptions);
            mii.wID   = FCIDM_BROWSEROPTIONS;

            // Append the item if it is missing, else just set it
            if (GetMenuState(hmenuPopup, FCIDM_BROWSEROPTIONS, MF_BYCOMMAND) == 0xFFFFFFFF)
            {
                AppendMenu(hmenuPopup, MF_SEPARATOR, -1, NULL);
                InsertMenuItem(hmenuPopup, 0xFFFFFFFF, TRUE, &mii);
            }
            else
            {
                SetMenuItemInfo(hmenuPopup, FCIDM_BROWSEROPTIONS, FALSE, &mii);
            }
        }
        else
        {
            // The only remaining scenario is GetUIVersion >= 5
            ASSERT(WhichPlatform() == PLATFORM_INTEGRATED);
        }
    }

    // Nuke tools->connect through tools->disconnect if restricted or if no net
    if ((!(GetSystemMetrics(SM_NETWORK) & RNC_NETWORKS)) ||
         SHRestricted(REST_NONETCONNECTDISCONNECT))
    {
        for (int i = FCIDM_CONNECT; i <= FCIDM_CONNECT_SEP; i++)
            DeleteMenu(hmenuPopup, i, MF_BYCOMMAND);
    }

    // Nuke tools->find + sep if restricted or if UI version >= 5
    // or if running rooted explorer (since the Find extensions assume
    // unrooted)
    if (SHRestricted(REST_NOFIND) || (GetUIVersion() >= 5)) {
        DeleteMenu(hmenuPopup, FCIDM_TOOLSSEPARATOR, MF_BYCOMMAND);
        DeleteMenu(hmenuPopup, FCIDM_MENU_FIND, MF_BYCOMMAND);
    }

    BOOL fAvailable;
    uCLSSPEC ucs;
    QUERYCONTEXT qc = { 0 };
    MENUITEMINFO mii;

    ucs.tyspec = TYSPEC_CLSID;
    ucs.tagged_union.clsid = CLSID_SubscriptionMgr;

    // see if this option is available
    fAvailable = (SUCCEEDED(FaultInIEFeature(NULL, &ucs, &qc, FIEF_FLAG_PEEK)));

    if (fAvailable && !_fShowSynchronize)
    {
        //  Turn it back on
        
        if (NULL != _pszSynchronizeText)
        {
            _fShowSynchronize = TRUE;


            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_TYPE | MIIM_ID;
            mii.fType = MFT_STRING;
            mii.wID = FCIDM_UPDATESUBSCRIPTIONS;
            mii.dwTypeData = _pszSynchronizeText;

            InsertMenuItem(hmenuPopup, _iSynchronizePos, MF_BYPOSITION, &mii);
        }
    }
    else if (!fAvailable && _fShowSynchronize)
    {
        //  Turn it off
        int iSyncPos = GetMenuPosFromID(hmenuPopup, FCIDM_UPDATESUBSCRIPTIONS);

        if (NULL == _pszSynchronizeText)
        {
            _iSynchronizePos = iSyncPos;

            MENUITEMINFO mii;
            TCHAR szBuf[MAX_PATH];
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_TYPE | MIIM_ID;
            mii.dwTypeData = szBuf;
            mii.cch = ARRAYSIZE(szBuf);

            if (GetMenuItemInfo(hmenuPopup, FCIDM_UPDATESUBSCRIPTIONS, MF_BYCOMMAND, &mii))
            {
                Str_SetPtr(&_pszSynchronizeText, (LPTSTR)mii.dwTypeData);
            }
        }

        DeleteMenu(hmenuPopup, FCIDM_UPDATESUBSCRIPTIONS, MF_BYCOMMAND);
        
        _fShowSynchronize = FALSE;
    }

    ASSERT((fAvailable && _fShowSynchronize) || (!fAvailable && !_fShowSynchronize));

    if (SHRestricted2(REST_NoWindowsUpdate, NULL, 0))
    {
        DeleteMenu(hmenuPopup, (DVIDM_HELPMSWEB+2), MF_BYCOMMAND);
    }
    else
    {
        DWORD   dwRet;
        DWORD   dwType;
        DWORD   dwSize;
        TCHAR   szNewUpdateName[MAX_PATH];

        // check to see if "Windows Update" should be called
        // something different in the menu

        dwSize = sizeof(szNewUpdateName);

        dwRet = SHRegGetUSValue(c_szMenuItemCust,
                                c_szWindowUpdateName,
                                &dwType,
                                (LPVOID)szNewUpdateName,
                                &dwSize,
                                FALSE,
                                NULL,
                                0);

        if (dwRet == ERROR_SUCCESS)
        {
            ASSERT(dwSize <= sizeof(szNewUpdateName));
            ASSERT(szNewUpdateName[(dwSize/sizeof(TCHAR))-1] == TEXT('\0'));

            // if we got anything, replace the menu item's text, or delete
            // the item if the text was NULL. we can tell if the text was
            // an empty string by seeing whether we got back more
            // bytes than just a null terminator

            if (dwSize > sizeof(TCHAR))
            {
                MENUITEMINFO mii;

                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_TYPE;
                mii.fType = MFT_STRING;
                mii.dwTypeData = szNewUpdateName;

                SetMenuItemInfo(hmenuPopup, FCIDM_PRODUCTUPDATES, FALSE, &mii);
            }
            else
            {
                ASSERT(dwSize == 0);

                DeleteMenu(hmenuPopup, FCIDM_PRODUCTUPDATES, MF_BYCOMMAND);
            }
        }
    }

    // Disable Mail and News submenu if we don't support it
    OLECMD rgcmd[] = {
       { SBCMDID_DOMAILMENU, 0 },
    };

    HRESULT hr = QueryStatus(&CGID_Explorer, ARRAYSIZE(rgcmd), rgcmd, NULL);  
    _EnableMenuItem(hmenuPopup, FCIDM_MAILANDNEWS, SUCCEEDED(hr) && (rgcmd[0].cmdf & OLECMDF_ENABLED));

    // prettify the menu (make sure first and last items aren't
    // separators and that there are no runs of >1 separator)
    _SHPrettyMenu(hmenuPopup);
}

void CShellBrowser2::_OnFileMenuPopup(HMENU hmenuPopup)
{
    // disable create shortcut, rename, delete, and properties
    // we'll enable them bellow if they are available
    _EnableMenuItem(hmenuPopup, FCIDM_DELETE, FALSE);
    _EnableMenuItem(hmenuPopup, FCIDM_PROPERTIES, FALSE);
    _EnableMenuItem(hmenuPopup, FCIDM_RENAME, FALSE);
    _EnableMenuItem(hmenuPopup, FCIDM_LINK, FALSE);
    
    if (SHRestricted2(REST_NoExpandedNewMenu, NULL, 0)
        && (GetMenuState(hmenuPopup, DVIDM_NEW, MF_BYCOMMAND) != 0xFFFFFFFF))
    {
        TCHAR szNewWindow[64];

        MLLoadString(IDS_NEW_WINDOW, szNewWindow, ARRAYSIZE(szNewWindow));

        MENUITEMINFO mii;
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.fType = MFT_STRING;
        mii.fState = MFS_ENABLED;
        mii.wID = DVIDM_NEWWINDOW;
        mii.dwTypeData = szNewWindow;
        mii.cch = lstrlen(szNewWindow);
        InsertMenuItem(hmenuPopup, DVIDM_NEW, FALSE, &mii);
        DeleteMenu(hmenuPopup, DVIDM_NEW, MF_BYCOMMAND);
    }

    if (_HasToolbarFocus())
    {
        OLECMD rgcmd[] = {
            { SBCMDID_FILEDELETE, 0 },
            { SBCMDID_FILEPROPERTIES, 0 },
            { SBCMDID_FILERENAME, 0},
            { SBCMDID_CREATESHORTCUT, 0},
        };
        IDockingWindow* ptbar = _GetToolbarItem(_itbLastFocus)->ptbar;
        if (SUCCEEDED(IUnknown_QueryStatus(ptbar, &CGID_Explorer, ARRAYSIZE(rgcmd), rgcmd, NULL)))
        {
            _EnableMenuItem(hmenuPopup, FCIDM_DELETE, rgcmd[0].cmdf & OLECMDF_ENABLED);
            _EnableMenuItem(hmenuPopup, FCIDM_PROPERTIES, rgcmd[1].cmdf & OLECMDF_ENABLED);
            _EnableMenuItem(hmenuPopup, FCIDM_RENAME, rgcmd[2].cmdf & OLECMDF_ENABLED);
            _EnableMenuItem(hmenuPopup, FCIDM_LINK, rgcmd[3].cmdf & OLECMDF_ENABLED);
        }
    }
    _EnableMenuItem(hmenuPopup, FCIDM_FILECLOSE, S_FALSE == _DisableModeless());

    _EnableFileContext(hmenuPopup);

    if (_fEnableOfflineFeature || (GetUIVersion() < 5))
    {
        _CheckMenuItem(hmenuPopup, FCIDM_VIEWOFFLINE, (Offline(SBSC_QUERY) == S_OK));
    }
    else
        RemoveMenu(hmenuPopup, FCIDM_VIEWOFFLINE, MF_BYCOMMAND);


    if (_fVisitedNet && NeedFortezzaMenu()) // Do not load WININET.DLL in explorer mode
    {
        // The logic here ensures that the menu is created once per instance
        // and only if there is a need to display a Fortezza menu.
        if (!_fShowFortezza)
        {
            static TCHAR szItemText[16] = TEXT("");
            if (!szItemText[0]) // The string will be loaded only once
                MLLoadString(IDS_FORTEZZA_MENU, szItemText, ARRAYSIZE(szItemText));
            if (_hfm==NULL)
                _hfm = FortezzaMenu();
            InsertMenu(hmenuPopup, FCIDM_FILECLOSE, MF_POPUP, (UINT_PTR) _hfm, szItemText);
            _fShowFortezza = TRUE;
        }
        SetFortezzaMenu(hmenuPopup);
    }
    else if (_fShowFortezza)    // Don't need the menu but already displayed?
    {                           // Remove without destroying the handle
        int cbItems = GetMenuItemCount(hmenuPopup);
        RemoveMenu(hmenuPopup, cbItems-2, MF_BYPOSITION);
        _fShowFortezza = FALSE;
    }


    // See if we can edit the page
    OLECMD rgcmd[] = {
        { CITIDM_EDITPAGE, 0 },
    };
    struct {
        OLECMDTEXT ct;
        wchar_t rgwz[128];
    } cmdText = {0};

    cmdText.ct.cwBuf = ARRAYSIZE(cmdText.rgwz) + ARRAYSIZE(cmdText.ct.rgwz);
    cmdText.ct.cmdtextf = OLECMDTEXTF_NAME;

    IDockingWindow* ptbar = _GetITBar();
    IUnknown_QueryStatus(ptbar, &CGID_PrivCITCommands, ARRAYSIZE(rgcmd), rgcmd, &cmdText.ct);

    _EnableMenuItem(hmenuPopup, FCIDM_EDITPAGE, rgcmd[0].cmdf & OLECMDF_ENABLED);

    // Update the name of the edit menu item
    TCHAR szText[80];
    MENUITEMINFO mii = {0};
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_TYPE;
    mii.dwTypeData = szText;
    if (cmdText.ct.cwActual > 1)
    {
        SHUnicodeToTChar(cmdText.ct.rgwz, szText, ARRAYSIZE(szText));
    }
    else
    {
        // Use default edit text
        MLLoadString(IDS_EDITPAGE, szText, ARRAYSIZE(szText));
    }
    SetMenuItemInfo(hmenuPopup, FCIDM_EDITPAGE, FALSE, &mii);



#ifdef TEST_AMBIENTS
   _CheckMenuItem(hmenuPopup, FCIDM_VIEWLOCALOFFLINE,
                  _LocalOffline(SBSC_QUERY) == TRUE);      
   _CheckMenuItem(hmenuPopup, FCIDM_VIEWLOCALSILENT,
                  _LocalSilent(SBSC_QUERY) == TRUE);    
#endif // TEST_AMBIENTS

    // must not change ie4 shell experience
    // so travel log still goes to the file menu   
    if ((GetUIVersion() < 5) && !IsWebPidl(_pbbd->_pidlCur))
    {
        int nPos = GetMenuItemCount(hmenuPopup) - 1; // Start with the last item
        MENUITEMINFO mii = {0};
        BOOL fFound = FALSE;

        mii.cbSize = SIZEOF(MENUITEMINFO);
        mii.fMask = MIIM_ID | MIIM_TYPE;

        // Find the last separator separator
        while (!fFound && nPos > 0)
        {
            mii.cch = 0;
            GetMenuItemInfo(hmenuPopup, nPos, TRUE, &mii);
            if (mii.fType & MFT_SEPARATOR)
                fFound = TRUE;
            else
                nPos --;
        }

        if (fFound)
        {
            _InsertTravelLogItems(hmenuPopup, nPos);
        }
    }

    HMENU hmFileNew = SHGetMenuFromID(hmenuPopup, DVIDM_NEW);

    if (hmFileNew)
    {
        // remove menu items for unregistered components
        // this code is duplicated in shdocvw\dochost.cpp and is necessary here 
        // so that unwanted items are not present before dochost has fully loaded

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

    if (!SHIsRegisteredClient(MAIL_DEF_KEY))
    {
        // disable Send Page by Email, Send Link by Email
        HMENU hmFileSend = SHGetMenuFromID(hmenuPopup, DVIDM_SEND);

        if (hmFileSend)
        {
            EnableMenuItem(hmFileSend, DVIDM_SENDPAGE, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hmFileSend, DVIDM_SENDSHORTCUT, MF_BYCOMMAND | MF_GRAYED);
        }
    }
}

void CShellBrowser2::_OnSearchMenuPopup(HMENU hmenuPopup)
{
    if (!_pcmSearch)
        _pxtb->QueryInterface(IID_IContextMenu3, (void **)&_pcmSearch);

    if (_pcmSearch)
        _pcmSearch->QueryContextMenu(hmenuPopup, 0, FCIDM_SEARCHFIRST, FCIDM_SEARCHLAST, 0);
}

void CShellBrowser2::_OnHelpMenuPopup(HMENU hmenuPopup)
{
    RIP(IS_VALID_HANDLE(hmenuPopup, MENU));

    // Do nothing if this is the DocHost version of the Help menu,
    // which always says "About Internet Explorer", and bring up
    // IE about dlg.

    // If we're running in native browser mode,
    // it says "About Windows" and bring up shell about dlg.
    // Change the "About Windows" to "About Windows NT" if running on NT.
    // Not sure what to do for Memphis yet.

    //
    // remove menu items which have been marked for removal
    // via the IEAK restrictions
    //

    if (SHRestricted2(REST_NoHelpItem_TipOfTheDay, NULL, 0))
    {
        DeleteMenu(hmenuPopup, FCIDM_HELPTIPOFTHEDAY, MF_BYCOMMAND);
    }

    if (SHRestricted2(REST_NoHelpItem_NetscapeHelp, NULL, 0))
    {
        DeleteMenu(hmenuPopup, FCIDM_HELPNETSCAPEUSERS, MF_BYCOMMAND);
    }

    if (SHRestricted2(REST_NoHelpItem_Tutorial, NULL, 0))
    {
        DeleteMenu(hmenuPopup, DVIDM_HELPTUTORIAL, MF_BYCOMMAND);
    }

    if (SHRestricted2(REST_NoHelpItem_SendFeedback, NULL, 0))
    {
        DeleteMenu(hmenuPopup, FCIDM_HELPSENDFEEDBACK, MF_BYCOMMAND);
    }
    
    UINT ids = IDS_ABOUTWINDOWS;
    if (IsOS(OS_NT4)  && !IsOS(OS_NT5))
    {
        ids = IDS_ABOUTWINDOWSNT;
    }
    else if (IsOS(OS_MEMPHIS))
    {
        ids = IDS_ABOUTWINDOWS98;
    }


    if (ids)
    {
        MENUITEMINFO mii;
        TCHAR szName[80];            // The name better not be any bigger.

        memset(&mii, 0, sizeof(MENUITEMINFO));
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_TYPE;

        // We only try to get FCIDM_HELPABOUT, which will fail if this
        // is the DocHost version of help menu (who has DVIDM_HELPABOUT.)

        if (GetMenuItemInfoWrap(hmenuPopup, FCIDM_HELPABOUT, FALSE, &mii) &&
            MLLoadString(ids, szName, ARRAYSIZE(szName)))
        {
            mii.dwTypeData = szName;
            SetMenuItemInfo(hmenuPopup, FCIDM_HELPABOUT, FALSE, &mii);
        }
    }

    SHCheckMenuItem(hmenuPopup, FCIDM_HELPTIPOFTHEDAY, (_idmComm == _InfoCLSIDToIdm(&CLSID_TipOfTheDay)));
}

void CShellBrowser2::_OnMailMenuPopup(HMENU hmenuPopup)
{
    if (!SHIsRegisteredClient(MAIL_DEF_KEY))
    {
        DeleteMenu(hmenuPopup, FCIDM_MAIL, MF_BYCOMMAND);
        DeleteMenu(hmenuPopup, FCIDM_NEWMESSAGE, MF_BYCOMMAND);
        DeleteMenu(hmenuPopup, FCIDM_SENDLINK, MF_BYCOMMAND);
        DeleteMenu(hmenuPopup, FCIDM_SENDDOCUMENT, MF_BYCOMMAND);
        DeleteMenu(hmenuPopup, FCIDM_MAILNEWSSEPARATOR, MF_BYCOMMAND);
    }

    if (!SHIsRegisteredClient(NEWS_DEF_KEY))
    {
        DeleteMenu(hmenuPopup, FCIDM_MAILNEWSSEPARATOR, MF_BYCOMMAND);
        DeleteMenu(hmenuPopup, FCIDM_NEWS, MF_BYCOMMAND);
    }
}

void CShellBrowser2::_OnEditMenuPopup(HMENU hmenuPopup)
{
    OLECMD rgcmdEdit[] = {{CITIDM_EDITPAGE, 0 }};

    OLECMD rgcmd[] = {
        { OLECMDID_CUT, 0 },
        { OLECMDID_COPY, 0 },
        { OLECMDID_PASTE, 0 },
        { OLECMDID_SELECTALL, 0 }
    };
    ASSERT(FCIDM_COPY==FCIDM_MOVE+1);
    ASSERT(FCIDM_PASTE==FCIDM_MOVE+2);
    ASSERT(FCIDM_SELECTALL==FCIDM_MOVE+3);

    TraceMsg(DM_PREMERGEDMENU, "CSB::v_OnInitMP got FCIDM_MENU_EDIT");
    IOleCommandTarget* pcmdt;
    HRESULT hres = _FindActiveTarget(IID_IOleCommandTarget, (void **)&pcmdt);
    if (SUCCEEDED(hres)) {
        pcmdt->QueryStatus(NULL, ARRAYSIZE(rgcmd), rgcmd, NULL);
        pcmdt->Release();
    }

    for (int i=0; i<ARRAYSIZE(rgcmd); i++) {
        _EnableMenuItem(hmenuPopup, FCIDM_MOVE+i, rgcmd[i].cmdf & OLECMDF_ENABLED);
    }

    if (SUCCEEDED(_GetITBar()->QueryInterface(IID_IOleCommandTarget, (void **)&pcmdt)))
    {
        pcmdt->QueryStatus(&CGID_PrivCITCommands, ARRAYSIZE(rgcmdEdit), rgcmdEdit, NULL);
        pcmdt->Release();
    }    
    _EnableMenuItem(hmenuPopup, FCIDM_EDITPAGE, rgcmdEdit[0].cmdf & OLECMDF_ENABLED);

    // prettify the menu (make sure first and last items aren't
    // separators and that there are no runs of >1 separator)
    _SHPrettyMenu(hmenuPopup);
}

void CShellBrowser2::_OnFindMenuPopup(HMENU hmenuPopup)
{
    TraceMsg(DM_TRACE, "cabinet InitMenuPopup of Find commands");

    ASSERT(GetUIVersion() < 5); // otherwise the menu is deleted when we load it from resources
    ASSERT(!SHRestricted(REST_NOFIND)); // otherwise the menu is deleted when we load it from resources

    ATOMICRELEASE(_pcmFind);
    _pcmFind = SHFind_InitMenuPopup(hmenuPopup, _pbbd->_hwnd, FCIDM_MENU_TOOLS_FINDFIRST, FCIDM_MENU_TOOLS_FINDLAST);
}

void CShellBrowser2::_OnExplorerBarMenuPopup(HMENU hmenuPopup)
{
    _AddBrowserBarMenuItems(hmenuPopup);

    // Disable the channel band if the NoChannelUI restriction is in place.
#ifdef ENABLE_CHANNELPANE
    if (SHRestricted2(REST_NoChannelUI, NULL, 0))
        _EnableMenuItem(hmenu, FCIDM_VBBCHANNELSBAND, FALSE);
#endif

    if (SHRestricted2(REST_NoFavorites, NULL, 0))
        _EnableMenuItem(hmenuPopup, FCIDM_VBBFAVORITESBAND, FALSE);

    for (int idCmd = FCIDM_VBBFIXFIRST; idCmd <= FCIDM_VBBDYNLAST; idCmd++)
        SHCheckMenuItem(hmenuPopup, idCmd, FALSE);

    SHCheckMenuItem(hmenuPopup, _idmInfo, TRUE);
    SHCheckMenuItem(hmenuPopup, _idmComm, TRUE);

    // if we have pre-ie4 shell32, remove the folders bar option
    if (GetUIVersion() < 4)
        DeleteMenu(hmenuPopup, FCIDM_VBBEXPLORERBAND, MF_BYCOMMAND);
}

LRESULT CShellBrowser2::v_OnInitMenuPopup(HMENU hmenuPopup, int nIndex, BOOL fSystemMenu)
{
    if (hmenuPopup == _GetMenuFromID(FCIDM_MENU_EXPLORE)) {
        _OnGoMenuPopup(hmenuPopup);
    } 
    else if (hmenuPopup == _GetMenuFromID(FCIDM_MENU_VIEW)) {
        _OnViewMenuPopup(hmenuPopup);
    }
    else if (hmenuPopup == _GetMenuFromID(FCIDM_MENU_TOOLS)) {
        _OnToolsMenuPopup(hmenuPopup);
    }
    else if (hmenuPopup == _GetMenuFromID(FCIDM_MENU_FILE)) {
        _OnFileMenuPopup(hmenuPopup);
    }
    else if (hmenuPopup == _GetMenuFromID(FCIDM_SEARCHMENU)) {
        _OnSearchMenuPopup(hmenuPopup);
    }
    else if ((hmenuPopup == _GetMenuFromID(FCIDM_MENU_HELP)) ||
             (hmenuPopup == SHGetMenuFromID(_hmenuFull, FCIDM_MENU_HELP))) {
        // For the help menu we try both the current menu and by chance the FullSB menu
        // as if we get here before the menu merge we will not have set the current menu
        // and that would leave Help about windows95 for all platforms.
        _OnHelpMenuPopup(hmenuPopup);
    }
    else if (GetMenuItemID(hmenuPopup, 0) == FCIDM_MAIL) {
        _OnMailMenuPopup(hmenuPopup);
    }
    else if (hmenuPopup == _GetMenuFromID(FCIDM_MENU_EDIT)) {
        _OnEditMenuPopup(hmenuPopup);
    }
    else if (hmenuPopup == _GetMenuFromID(FCIDM_MENU_FIND)) {
        _OnFindMenuPopup(hmenuPopup);
    }
    else if (hmenuPopup == _GetMenuFromID(FCIDM_VIEWBROWSERBARS)) {
        _OnExplorerBarMenuPopup(hmenuPopup);
    }
    else {
        if (!(_pcm && (_pcm->HandleMenuMsg(WM_INITMENUPOPUP, (WPARAM)hmenuPopup, (LPARAM)MAKELONG(nIndex, fSystemMenu)) == NOERROR))) {        
            if (_pcmNsc) {
                _pcmNsc->HandleMenuMsg(WM_INITMENUPOPUP, (WPARAM)hmenuPopup, (LPARAM)MAKELONG(nIndex, fSystemMenu));
            }
        }
    }
    
    return S_OK;
}

#pragma warning (disable:4200)
typedef struct {
    int nItemOffset;
    int nPopupOffset;
    struct {
        UINT uID;
        HMENU hPopup;
#ifndef UNIX
    } sPopupIDs[];
#else
    } sPopupIDs[2];
#endif
} MENUHELPIDS;
#pragma warning (default:4200)


void CShellBrowser2::_SetMenuHelp(HMENU hmenu, UINT wID, LPCTSTR pszHelp)
{
    if (pszHelp && pszHelp[0])
    {
        UINT flags = SBT_NOBORDERS | 255;

        // If the menu text is RTL, then so too will the status text be
        MENUITEMINFO mii = { sizeof(mii) };
        mii.fMask = MIIM_TYPE;
        if (GetMenuItemInfo(hmenu, wID, FALSE, &mii) &&
            (mii.fType & MFT_RIGHTORDER))
            flags |= SBT_RTLREADING;

        SendMessage(_hwndStatus, SB_SETTEXT, flags, (LPARAM)pszHelp);
        SendMessage(_hwndStatus, SB_SIMPLE, 1, 0);
    }
}

void CShellBrowser2::_SetExternalBandMenuHelp(HMENU hmenu, UINT wID)
{
    USES_CONVERSION;
    OLECMD cmd = { CITIDM_VIEWEXTERNALBAND_FIRST + (wID - FCIDM_EXTERNALBANDS_FIRST), 0 };
    OLECMDTEXTV<MAX_PATH> cmdtv;
    cmdtv.cwBuf = MAX_PATH;
    cmdtv.cmdtextf = OLECMDTEXTF_STATUS;
    cmdtv.rgwz[0] = 0;

    IUnknown_QueryStatus(_GetITBar(), &CGID_PrivCITCommands, 1, &cmd, &cmdtv);

    _SetMenuHelp(hmenu, wID, W2CT(cmdtv.rgwz));
}

void CShellBrowser2::_SetBrowserBarMenuHelp(HMENU hmenu, UINT wID)
{
    if (_pbsmInfo)
    {
        BANDCLASSINFO *pbci = _pbsmInfo->GetBandClassDataStruct(wID - FCIDM_VBBDYNFIRST);
        if (pbci)
        {
            LPCTSTR pszHelp = pbci->pszHelpPUI ? pbci->pszHelpPUI : pbci->pszHelp;
            _SetMenuHelp(hmenu, wID, pszHelp);
        }
    }
}

// Handles WM_MENUSELECT.  Returns FALSE if this menu item isn't handled by
// the frame.
LRESULT CShellBrowser2::_OnMenuSelect(WPARAM wParam, LPARAM lParam, UINT uHelpFlags)
{
    MENUHELPIDS sMenuHelpIDs = {
        MH_ITEMS, MH_POPUPS,
        0, NULL,        // Placeholder for specific menu 
        0, NULL         // This list must be NULL terminated 
    };
    TCHAR szHint[MAX_PATH];     // OK with MAX_PATH
    UINT uMenuFlags = GET_WM_MENUSELECT_FLAGS(wParam, lParam);
    WORD wID = GET_WM_MENUSELECT_CMD(wParam, lParam);
    HMENU hMenu = GET_WM_MENUSELECT_HMENU(wParam, lParam);

    /* 
       HACKHACK
       USER32 TrackPopup menus send a menu deselect message which clears our
       status text at inopportune times.  We work around this with a private
       MBIgnoreNextDeselect message.
    */

    // Did someone ask us to clear the status text?
    if (!hMenu && LOWORD(uMenuFlags)==0xffff)
    {
        // Yes

        // Should we honour that request?
        if (!_fIgnoreNextMenuDeselect)
            // Yes
            SendMessage(_hwndStatus, SB_SIMPLE, 0, 0L);
        else
            // No
            _fIgnoreNextMenuDeselect = FALSE;

        return 1L;
    }
    
    // Clear this out just in case, but don't update yet
    SendMessage(_hwndStatus, SB_SETTEXT, SBT_NOBORDERS|255, (LPARAM)(LPTSTR)c_szNULL);
    SendMessage(_hwndStatus, SB_SIMPLE, 1, 0L);

    if (uMenuFlags & MF_SYSMENU)
    {
        // We don't put special items on the system menu any more, so go
        // straight to the MenuHelp
        goto DoMenuHelp;
    }

    if (uMenuFlags & MH_POPUP)
    {
        MENUITEMINFO miiSubMenu;

        if (!_hmenuCur)
        {
            return(0L);
        }

        miiSubMenu.cbSize = SIZEOF(MENUITEMINFO);
        miiSubMenu.fMask = MIIM_SUBMENU|MIIM_ID;
        if (!GetMenuItemInfoWrap(GET_WM_MENUSELECT_HMENU(wParam, lParam), wID, TRUE, &miiSubMenu))
        {
            // Check if this was a top level menu
            return(0L);
        }

        // Change the parameters to simulate a "normal" menu item
        wParam = miiSubMenu.wID;
        wID = (WORD)miiSubMenu.wID;
//
// NOTES: We are eliminating this range check so that we can display
//  help-text on sub-menus. I'm not sure why explorer.exe has this check.
//
#if 0
        if (!IsInRange(wID, FCIDM_GLOBALFIRST, FCIDM_GLOBALLAST))
            return 0L;
#endif
        uMenuFlags = 0;
    }

    //  No menu help for context menu stuck in File Menu or menus that aren't ours
    //  BUGBUG chrisfra 9/2/97, in IE 5.0, might want to write code to use context
    //  menu to get help to work
    if (IsInRange(wID, FCIDM_FILECTX_FIRST, FCIDM_FILECTX_LAST) ||
        !IsInRange(wID, FCIDM_FIRST, FCIDM_LAST))
        return 0L;

    if (_pcmSearch && IsInRange(wID, FCIDM_SEARCHFIRST, FCIDM_SEARCHLAST))
    {
        _pcmSearch->HandleMenuMsg(WM_MENUSELECT, wParam, lParam);
       return 0L;
    }

#if 0
    if (IsInRange(wID, FCIDM_RECENTFIRST, FCIDM_RECENTLAST)) {
        wID = FCIDM_RECENTFIRST;
    }
#endif

    // Menu help for plug-in explorer bars.
    if (IsInRange(wID, FCIDM_VBBDYNFIRST, FCIDM_VBBDYNLAST))
    {
        _SetBrowserBarMenuHelp(hMenu, wID);
        return 0L;
    }

    // Menu help for plug-in itbar bands
    if (IsInRange(wID, FCIDM_EXTERNALBANDS_FIRST, FCIDM_EXTERNALBANDS_LAST))
    {
        _SetExternalBandMenuHelp(hMenu, wID);
        return 0L;
    }

    szHint[0] = 0;

    sMenuHelpIDs.sPopupIDs[0].uID = 0;
    sMenuHelpIDs.sPopupIDs[0].hPopup = NULL;

DoMenuHelp:
    MenuHelp(WM_MENUSELECT, wParam, lParam, _hmenuCur, MLGetHinst(),
             _hwndStatus, (UINT *)&sMenuHelpIDs);

    return 1L;
}

void CShellBrowser2::_DisplayFavoriteStatus(LPCITEMIDLIST pidl)
{
    LPTSTR pszURL = NULL;

    IUniformResourceLocator * pURL;
    if (SUCCEEDED(CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
        IID_IUniformResourceLocator, (void **)&pURL)))
    {
        IPersistFile *ppf;
        if (SUCCEEDED(pURL->QueryInterface(IID_IPersistFile, (void **)&ppf)))
        {
            WCHAR wszPath[MAX_PATH];

            // Get the full path to the .lnk
            SHGetPathFromIDListW(pidl, wszPath);

            // Attempt to connect the storage of the IURL to the pidl
            if (SUCCEEDED(ppf->Load(wszPath, STGM_READ)))
            {
                pURL->GetURL(&pszURL);
            }

            ppf->Release();
        }

        pURL->Release();
    }

    SendMessage(_hwndStatus, SB_SIMPLE, 1, 0L);
    SendMessage(_hwndStatus, SB_SETTEXT, SBT_NOBORDERS|255, (LPARAM)pszURL);

    if (pszURL)
        SHFree(pszURL);
}

LRESULT CShellBrowser2::_ThunkTTNotify(LPTOOLTIPTEXTA pnmTTTA)
{
    TOOLTIPTEXTW tttw = {0};

    tttw.hdr = pnmTTTA->hdr;
    tttw.hdr.code = TTN_NEEDTEXTW;

    tttw.lpszText = tttw.szText;
    tttw.hinst    = pnmTTTA->hinst;
    tttw.uFlags   = pnmTTTA->uFlags;
    tttw.lParam   = pnmTTTA->lParam;

    LRESULT lRes = SUPERCLASS::OnNotify(&tttw.hdr);

    pnmTTTA->hdr = tttw.hdr;
    pnmTTTA->hdr.code = TTN_NEEDTEXTA;

    pnmTTTA->hinst = tttw.hinst;
    pnmTTTA->uFlags = tttw.uFlags;
    pnmTTTA->lParam = tttw.lParam;

    if (tttw.lpszText == LPSTR_TEXTCALLBACKW)
        pnmTTTA->lpszText = LPSTR_TEXTCALLBACKA;
    else if (!tttw.lpszText)
        pnmTTTA->lpszText = NULL;
    else if (!HIWORD(tttw.lpszText))
        pnmTTTA->lpszText = (LPSTR)tttw.lpszText;
    else {
        WideCharToMultiByte(CP_ACP, 0, tttw.lpszText, -1,
                            pnmTTTA->szText, ARRAYSIZE(pnmTTTA->szText), NULL, NULL);
    }
    
    return lRes;
}

UINT GetDDEExecMsg()
{
    static UINT uDDEExec = 0;

    if (!uDDEExec)
        uDDEExec = RegisterWindowMessage(TEXT("DDEEXECUTESHORTCIRCUIT"));

    return uDDEExec;
}

LRESULT CShellBrowser2::OnNotify(LPNMHDR pnm)
{
    switch (pnm->code)
    {
        case NM_DBLCLK:
        {
            int idCmd = -1;
            LPNMCLICK pnmc = (LPNMCLICK)pnm;
            switch(pnmc->dwItemSpec)
            {
                case STATUS_PANE_NAVIGATION:
                    idCmd = SHDVID_NAVIGATIONSTATUS;
                    break;
                        
                case STATUS_PANE_PROGRESS:
                    idCmd = SHDVID_PROGRESSSTATUS;
                    break;
                    
                case STATUS_PANE_OFFLINE:
                    idCmd = SHDVID_ONLINESTATUS;
                    break;
                
                //case STATUS_PANE_PRINTER:
                //    idCmd = SHDVID_PRINTSTATUS;
                //    break;
                
                case STATUS_PANE_ZONE:
                    idCmd = SHDVID_ZONESTATUS;
                    break;
                    
                case STATUS_PANE_SSL:
                    idCmd = SHDVID_SSLSTATUS;
                    break;
                    
                default:
                    break;
            }
            if (_pbbd->_pctView && (idCmd != -1))
            {
                HRESULT hr = _pbbd->_pctView->Exec(&CGID_ShellDocView, idCmd, NULL, NULL, NULL);

                // If the parent couldn't handle it, maybe we can.
                if (FAILED(hr)) {
                    if (pnmc->dwItemSpec == _uiZonePane &&
                        _pbbd->_pidlCur)
                    {
                        WCHAR wszUrl[MAX_URL_STRING];
                        if (SUCCEEDED(::IEGetDisplayName(_pbbd->_pidlCur, wszUrl, SHGDN_FORPARSING)))
                            ZoneConfigureW(_pbbd->_hwnd, wszUrl);
                    }
                }
            }
            
            break;
        }
        
        case TTN_NEEDTEXTA:
        case TTN_NEEDTEXTW:
            if (IsInRange(pnm->idFrom, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST)) {
                if (pnm->code == TTN_NEEDTEXTA && _fUnicode) 
                    return _ThunkTTNotify((LPTOOLTIPTEXTA)pnm);
                else
                    return SUPERCLASS::OnNotify(pnm);
            }         
            return 0;

        case SEN_DDEEXECUTE:
            if (pnm->idFrom == 0) 
            {
                LPNMVIEWFOLDER pnmPost = DDECreatePostNotify((LPNMVIEWFOLDER)pnm) ;

                if (pnmPost)
                {
                    PostMessage(_pbbd->_hwnd, GetDDEExecMsg(), 0, (LPARAM)pnmPost);
                    return TRUE;
                }

            }
            break;
            
        case SBN_SIMPLEMODECHANGE:
            if ((pnm->idFrom == FCIDM_STATUS) && _hwndProgress) 
                _ShowHideProgress();
            break;
            
        default:
            break;
    }        
    return 0;
}

DWORD CShellBrowser2::_GetTempZone()
{
    LPCITEMIDLIST pidlChild;
    IShellFolder* psfParent;
    WCHAR szURL[MAX_URL_STRING];
    
    szURL[0] = 0;   // parse name for zone goes here
    
    if (SUCCEEDED(IEBindToParentFolder(_pbbd->_pidlCur, &psfParent, &pidlChild)))
    {
        // see if this is a folder shortcut, if so we use it's path for the zone
        IShellLink *psl;
        if (SUCCEEDED(psfParent->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*)&pidlChild, IID_IShellLink, NULL, (void **)&psl)))
        {
            LPITEMIDLIST pidlTarget;
            if (S_OK == psl->GetIDList(&pidlTarget))
            {
                ::IEGetDisplayName(pidlTarget, szURL, SHGDN_FORPARSING);
                ILFree(pidlTarget);
            }
            psl->Release();
        }
        psfParent->Release();
    }
    
    if (NULL == _pism)
        CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IInternetSecurityManager, &_pism));

    DWORD nZone = ZONE_UNKNOWN;
    if (_pism && (szURL[0] || SUCCEEDED(::IEGetDisplayName(_pbbd->_pidlCur, szURL, SHGDN_FORPARSING))))
    {
        _pism->MapUrlToZone(szURL, &nZone, 0);
    }
    return nZone;
}

void Exec_GetZone(IUnknown * punk, VARIANTARG *pvar)
{
    IUnknown_Exec(punk, &CGID_Explorer, SBCMDID_MIXEDZONE, 0, NULL, pvar);

    if (pvar->vt == VT_UI4) // MSHTML was able to figure out what zone we are in
        pvar->ulVal = MAKELONG(STATUS_PANE_ZONE, pvar->ulVal);    
    else if (pvar->vt == VT_NULL)  // MSHTML has figured us to be in a mixed zone
        pvar->ulVal = MAKELONG(STATUS_PANE_ZONE, ZONE_MIXED);    
    else // We don't have zone info
        pvar->ulVal = MAKELONG(STATUS_PANE_ZONE, ZONE_UNKNOWN);    

    pvar->vt = VT_UI4;
}

//
//  in:
//          pvar    if NULL, we query the view for the zone
//                  non NULL, contains a VT_UI4 that encodes the zone pane and the zone value

void CShellBrowser2::_UpdateZonesPane(VARIANT *pvar)
{
    LONG lZone = ZONE_UNKNOWN;
    BOOL fMixed = FALSE;
    TCHAR szDisplayName[MAX_ZONE_DISPLAYNAME];
    VARIANTARG var = {0};

    if (NULL == pvar)
    {
        pvar = &var;
        Exec_GetZone(_pbbd->_pctView, &var);
    }

    // Do we already have the zone and fMixed info?
    if (pvar->vt == VT_UI4)
    {
        lZone = (signed short)HIWORD(pvar->lVal);
        _uiZonePane = (int)(signed short)LOWORD(pvar->lVal);
        if (lZone == ZONE_MIXED)
        {
            lZone = ZONE_UNKNOWN;
            fMixed = TRUE;
        }

        if (lZone < 0 && !IS_SPECIAL_ZONE(lZone))
        {
            // sometimes we're getting back an invalid zone index from urlmon,
            // and if we don't bound the index, we'll crash later
            lZone = ZONE_UNKNOWN;
        }
    }

    var.vt = VT_EMPTY;
    if (_pbbd->_pctView && SUCCEEDED(_pbbd->_pctView->Exec(&CGID_Explorer, SBCMDID_GETPANE, PANE_ZONE, NULL, &var)))
        _uiZonePane = var.ulVal;
    else
        _uiZonePane = STATUS_PANE_ZONE;

    // Sanity Check the zone
    if (_pbbd->_pidlCur)
    {
        DWORD nTempZone = _GetTempZone();
        if (nTempZone != ZONE_UNKNOWN)
            if (nTempZone > (DWORD)lZone)
                lZone = nTempZone;
    }

    szDisplayName[0] = 0;

    if (ZONE_UNKNOWN == lZone)
        MLLoadStringW(IDS_UNKNOWNZONE, szDisplayName, ARRAYSIZE(szDisplayName));
    else
    {
        ENTERCRITICAL;
        
        if (lZone < (LONG)_CacheZonesIconsAndNames(FALSE))
        {
            if (_hZoneIcon)
                DestroyIcon(_hZoneIcon);

            ICONINFO  IconInfo;

            // cache a copy of the icon
            if (!GetIconInfo((HICON)(g_pZoneIconNameCache + lZone)->hiconZones, &IconInfo))
            {
                _hZoneIcon=NULL;
            }
            else
            {
                _hZoneIcon = CreateIconIndirect(&IconInfo);
                DeleteObject(IconInfo.hbmMask);
                DeleteObject(IconInfo.hbmColor);
            }

            StrNCpy(szDisplayName, (g_pZoneIconNameCache + lZone)->pszZonesName, ARRAYSIZE(szDisplayName));
        }

        LEAVECRITICAL;
    }
    
    if (fMixed)
    {
        TCHAR szMixed[32];
        MLLoadString(IDS_MIXEDZONE, szMixed, ARRAYSIZE(szMixed));
        StrCatBuff(szDisplayName, szMixed, ARRAYSIZE(szDisplayName));
    }

    SendControlMsg(FCW_STATUS, SB_SETTEXTW, _uiZonePane, (LPARAM)szDisplayName, NULL);
    SendControlMsg(FCW_STATUS, SB_SETICON, _uiZonePane, (LPARAM)_hZoneIcon, NULL);
}

HRESULT CShellBrowser2::ReleaseShellView()
{
    // Give the current view a chance to save before we navigate away
    if (!_fClosingWindow)
    {
        // only try to save if we actually have a current view (this gets
        // called multiple times in a row on destruction, and it gets called
        // before the first view is created)
        //
        if (_pbbd->_psv)
            _SaveState();
    }

    return SUPERCLASS::ReleaseShellView();
}

bool IsWin95ClassicViewState (void)
{
    DWORD dwValue;
    DWORD cbSize = SIZEOF(dwValue);
    return ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER,
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
            TEXT("ClassicViewState"), NULL, &dwValue, &cbSize) && dwValue;
}

// For Folder Advanced Options flags that we check often, it's better
// to cache the values as flags in CBaseBrowser2. Update them here.
void CShellBrowser2::_UpdateRegFlags()
{
    _fWin95ViewState = IsWin95ClassicViewState();
}


HRESULT CShellBrowser2::CreateViewWindow(IShellView* psvNew, IShellView* psvOld, LPRECT prcView, HWND* phwnd)
{
    if (_pbbd->_psv)
    {
        // snag the current view's setting so we can pass it along
        _UpdateFolderSettings(_pbbd->_pidlPending);
        
        // this is not a valid flag to pass along after the first one
        _fsd._fs.fFlags &= ~FWF_BESTFITWINDOW;
    }

    return SUPERCLASS::CreateViewWindow(psvNew, psvOld, prcView, phwnd);
}

HRESULT CShellBrowser2::ActivatePendingView(void)
{
    _fTitleSet = FALSE;
 
    // NOTES: (SatoNa)
    //
    //  Notice that we no longer call SetRect(&_rcBorderDoc, 0, 0, 0, 0)
    // here. We call it in CShellBrowser2::ReleaseShellView instead.
    // See my comment over there for detail.
    //
    HRESULT hres = SUPERCLASS::ActivatePendingView();
    if (FAILED(hres))
        return hres;

    _ReloadStatusbarIcon();   
       
    _SetTitle(NULL);
    v_SetIcon();
    VALIDATEPENDINGSTATE();

    if (_pxtb)
        _pxtb->SendToolbarMsg(&CLSID_CommonButtons, TB_ENABLEBUTTON, TBIDM_PREVIOUSFOLDER, _ShouldAllowNavigateParent(), NULL);

    UpdateWindowList();
    
    if (!_HasToolbarFocus()) 
    {
        HWND hwndFocus = GetFocus();
        //
        // Trident may take the input focus when they are being UIActivated.
        // In that case, don't mess with the focus.
        //
        if (_pbbd->_hwndView && (hwndFocus==NULL || !IsChild(_pbbd->_hwndView, hwndFocus))) 
            SetFocus(_pbbd->_hwndView);
    }

    // Let's profile opening time
    if (g_dwProfileCAP & 0x00010000)
        StopCAP();

    // Let's profile opening time
    if (g_dwProfileCAP & 0x00000020)
        StartCAP();

    return S_OK;
}

void CShellBrowser2::_UpdateBackForwardStateNow()
{
    _fUpdateBackForwardPosted = FALSE;
    SUPERCLASS::UpdateBackForwardState();
}

HRESULT CShellBrowser2::UpdateBackForwardState()
{
    if (!_fUpdateBackForwardPosted) {
        PostMessage(_pbbd->_hwnd, CWM_UPDATEBACKFORWARDSTATE, 0, 0);
        _fUpdateBackForwardPosted = TRUE;
    }
    return S_OK;
}

HRESULT CShellBrowser2::_TryShell2Rename(IShellView* psv, LPCITEMIDLIST pidlNew)
{
    HRESULT hres = SUPERCLASS::_TryShell2Rename(psv, pidlNew); 
    if (SUCCEEDED(hres)) {
        _SetTitle(NULL);
        
        //if (_aTBar[ITB_STBAR].fShow)
        //DriveList_UpdatePath(this, FALSE);
        
    }
    return hres;
}


/*----------------------------------------------------------
Purpose: Determines if this message should be forwarded onto
         the dochost frame.

Returns: TRUE if the message needs to be forwarded
*/
BOOL CShellBrowser2::_ShouldForwardMenu(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!_fDispatchMenuMsgs)
        return FALSE;
    
    switch (uMsg) 
    {
    case WM_MENUSELECT:
    {
        // See CDocObjectHost::_ShouldForwardMenu for more details
        // about how this works.
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


DWORD CShellBrowser2::v_RestartFlags()
{
    return COF_CREATENEWWINDOW;
}

void CShellBrowser2::_CloseAllParents()
{
    LPITEMIDLIST pidl = ILClone(_pbbd->_pidlCur);
    if (pidl) 
    {
        for (ILRemoveLastID(pidl); !ILIsEmpty(pidl); ILRemoveLastID(pidl)) 
        {
            HWND hwnd;
            if (WinList_FindFolderWindow(pidl, NULL, &hwnd, NULL) == S_OK) 
            {
                TraceMsg(DM_SHUTDOWN, "csb.cap: post WM_CLOSE hwnd=%x", hwnd);
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
        }
        ILFree(pidl);
    }
}


BOOL CShellBrowser2::_ShouldSaveWindowPlacement()
{
    // If this is done by automation, maybe we should not update the defaults, so
    // to detect this we say if the window is not visible, don't save away the defaults
    
    // For the internet, save one setting for all, otherwise use the win95
    // view stream mru
    
    return (IsWindowVisible(_pbbd->_hwnd) && !_fUISetByAutomation &&
            _pbbd->_pidlCur && IsBrowserFrameOptionsSet(_pbbd->_psf, BFO_BROWSER_PERSIST_SETTINGS));
}


void CShellBrowser2::_OnConfirmedClose()
{
    if (_pbbd->_pidlCur && IsCShellBrowser2() && (GetKeyState(VK_SHIFT) < 0)) {
        _CloseAllParents();
    }
    
    if (_fUseIEPersistence && IsCShellBrowser2())
    {
        // save off whether we should launch in fullscreen or not
        SHRegSetUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Main"),
                        TEXT("FullScreen"), REG_SZ, 
                        _ptheater ? TEXT("yes") : TEXT("no"), 
                        _ptheater ? SIZEOF(TEXT("yes")) : SIZEOF(TEXT("no")), 
                        SHREGSET_DEFAULT);
    }

    if (_ptheater)
    {
        ShowWindow(_pbbd->_hwnd, SW_HIDE);
        _TheaterMode(FALSE, FALSE);
        _fDontSaveViewOptions = TRUE;
    } 
    else 
    {
        if (_ShouldSaveWindowPlacement())
        {
            StorePlacementOfWindow(_pbbd->_hwnd);
        }
        else
            _fDontSaveViewOptions = TRUE;
    }

    // for now we use the same 12-hour (SessionTime) rule
    // possibly we should just do it always?
    UEMFireEvent(&UEMIID_BROWSER, UEME_CTLSESSION, UEMF_XEVENT, FALSE, -1);
    if (!g_bRunOnNT5) {
        // for down-level guys (old explorer), fake a shell end session too
        UEMFireEvent(&UEMIID_SHELL, UEME_CTLSESSION, UEMF_XEVENT, FALSE, -1);
    }

    // Save view states before closing all toolbars
    // Remember that we saved so we don't do it again
    // during _ReleaseShellView
    _SaveState();
    _fClosingWindow = TRUE;

    // To prevent flashing, we move the window off the screen, unfortunately
    // we can't hide it as shockwave briefly puts up dialog which causes
    // an ugly blank taskbar icon to appear.
    // do this after _SaveState() because that saves window pos info

    // BUGBUG: this won't look too good with multi monitors...
    SetWindowPos(_pbbd->_hwnd, NULL, 10000, 10000, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
    
    // Save the Internet toolbar before we close it!
    if (!_fDontSaveViewOptions)
        _SaveITbarLayout();

    _CloseAndReleaseToolbars(TRUE);
    ATOMICRELEASE(_pxtb);
    
    // If you wait until WM_DESTROY (DestroyWindow below) to do this, under some
    // circumstances (eg an html page with an iframe whose src is an UNC based directory)
    // Ole will not Release the CShellBrowser2 that it addref'ed on RegisterDragDrop
    // (chrisfra 7/22/97)
    SetFlags(0, BSF_REGISTERASDROPTARGET);

    // If you wait until WM_DESTROY to do this, some OCs (like shockwave)
    // will hang (probably posting themselves a message)
    _CancelPendingView();
    ReleaseShellView();

    ATOMICRELEASE(_pmb);
    
    // Destroy the icons we created while we still can
    _SetWindowIcon(_pbbd->_hwnd, NULL, ICON_SMALL);
    _SetWindowIcon(_pbbd->_hwnd, NULL, ICON_BIG);

    // BUGBUG:: getting a random fault on rooted explorer on shutdown on this Destroy, maybe
    // somehow getting reentered.  So atomic destroy it...
    // NOTE: chrisg removed this at one point - is it dead?
    HWND hwndT = _pbbd->_hwnd;
    PutBaseBrowserData()->_hwnd = NULL;
    DestroyWindow(hwndT);

}


// these three functions, CommonHandleFielSysChange,
// v_HandleFileSysChange and this one
// may seem strange, but the idea is that the notify may come in from
// different sources (OneTree vs, win95 style fsnotify vs nt style)
// _OnFSNotify cracks the input, unifies it and calls CommonHnaldeFileSysChange
//  that calls to v_HandleFIleSysChange.  The Common...() is for stuff both needs
// the v_Handle...() is for overridden ones
void CShellBrowser2::_OnFSNotify(WPARAM wParam, LPARAM lParam)
{
    LPSHChangeNotificationLock  pshcnl = NULL;
    LONG lEvent;
    LPITEMIDLIST *ppidl = NULL; // on error, SHChangeNotification_Lock doesn't zero this out!
    IShellChangeNotify * pIFSN;
    
    if (g_fNewNotify && (wParam || lParam))
    {
        // New style of notifications need to lock and unlock in order to free the memory...
        pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);
    } else {
        lEvent = (LONG)lParam;
        ppidl = (LPITEMIDLIST*)wParam;
    }

    if (ppidl)
    {
        //
        //  If we haven't initialized "this" yet, we should ignore all the
        // notification.
        //
        if (_pbbd->_pidlCur)
        {
            _CommonHandleFileSysChange(lEvent, ppidl[0], ppidl[1]);

            //
            //  Forward to ITBar too...
            //
            if (_GetITBar() && SUCCEEDED(_GetITBar()->QueryInterface(IID_IShellChangeNotify, (void **)&pIFSN)))
            {
                pIFSN->OnChange(lEvent, ppidl[0], ppidl[1]);
                pIFSN->Release();
            }
        }
    }

    if (pshcnl)
    {
        SHChangeNotification_Unlock(pshcnl);
    }
}

// fDisconnectAlways means we shouldn't try to re-open the folder (like when
// someone logs off of a share, reconnecting would ask them for
// a password again when they just specified that they want to log off)
void CShellBrowser2::_FSChangeCheckClose(LPCITEMIDLIST pidl, BOOL fDisconnect)
{
    if (ILIsParent(pidl, _pbbd->_pidlCur, FALSE))
    {
        if (!fDisconnect)
        {
            //  APPCOMPAT: FileNet IDMDS (Panagon)'s shell folder extension
            //  incorrectly reports itself as a file system folder, so sniff the
            //  pidl to see if we should ignore the bit.  (B#359464: tracysh)

            TCHAR szPath[MAX_PATH];
            DWORD dwAttrib = SFGAO_FILESYSTEM | SFGAO_BROWSABLE;
            if (SUCCEEDED(SHGetNameAndFlags(_pbbd->_pidlCur, SHGDN_FORPARSING, szPath, SIZECHARS(szPath), &dwAttrib))
            && (dwAttrib & SFGAO_FILESYSTEM)
            && !(dwAttrib & SFGAO_BROWSABLE)
            && IsFlagClear(SHGetObjectCompatFlagsFromIDList(_pbbd->_pidlCur), OBJCOMPATF_NOTAFILESYSTEM)
            && !PathFileExistsAndAttributes(szPath, NULL))
            {
                fDisconnect = TRUE;
            }
        }
        
        if (fDisconnect)
            _pbbd->_pautoWB2->Quit();
    }
}

void CShellBrowser2::v_HandleFileSysChange(LONG lEvent, LPITEMIDLIST pidl1, LPITEMIDLIST pidl2)
{
    BOOL fDisconnectAlways = FALSE;

    //
    //  If we are in the middle of changing folders,
    // ignore this event.
    //
    if (_pbbd->_psvPending) {
        return;
    }

    // README:
    // If you need to add events here, then you must change SHELLBROWSER_FSNOTIFY_FLAGS in
    // order to get the notifications
    switch(lEvent)
    {
    case SHCNE_DRIVEADDGUI:
        if (ILIsParent(pidl1, _pbbd->_pidlCur, FALSE)) {
            PostMessage(_pbbd->_hwnd, WM_COMMAND, FCIDM_REFRESH, 0L);
        }
        break;

    case SHCNE_RMDIR:
        if (g_fRunningOnNT)
            fDisconnectAlways = TRUE;
        goto CheckClose;

    case SHCNE_SERVERDISCONNECT:
    case SHCNE_MEDIAREMOVED:
        fDisconnectAlways = TRUE;
        // fall thru


    case SHCNE_UPDATEDIR:
    case SHCNE_NETUNSHARE:
    case SHCNE_DRIVEREMOVED:
CheckClose:
        // preserve old behavior of when the explorer (all folders) bar is up,
        // go to nearest parent folder
        if (_idmInfo == FCIDM_VBBEXPLORERBAND)
            break;
        _FSChangeCheckClose(pidl1, fDisconnectAlways);
        break;
    }

}

// converts a simple pidl to a full pidl

LPITEMIDLIST _SimpleToReal(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlReturn = NULL;
    IShellFolder *psf;
    LPCITEMIDLIST pidlChild;
    if (SUCCEEDED(IEBindToParentFolder(pidl, &psf, &pidlChild)))
    {
        LPITEMIDLIST pidlRealChild;
        if (SUCCEEDED(SHGetRealIDL(psf, pidlChild, &pidlRealChild)))
        {
            LPITEMIDLIST pidlParent = ILCloneParent(pidl);
            if (pidlParent)
            {
                pidlReturn = ILCombine(pidlParent, pidlRealChild);
                
                ILFree(pidlParent);
            }
            
            ILFree(pidlRealChild);
        }
        
        psf->Release();
    }

    return pidlReturn;
}



void CShellBrowser2::_CommonHandleFileSysChange(LONG lEvent, LPITEMIDLIST pidl1, LPITEMIDLIST pidl2)
{
    v_HandleFileSysChange(lEvent, pidl1, pidl2);

    // WARNING: In all cases _pbbd will have NULL contents before first navigate.

    if (_pbbd->_psvPending) {
        return;
    }

    // stuff that needs to be done tree or no tree
    switch (lEvent) {

    // README:
    // If you need to add events here, then you must change SHELLBROWSER_FSNOTIFY_FLAGS in
    // order to get the notifications
        
    case SHCNE_UPDATEITEM:
        // the name could have changed
        if (ILIsEqual(_pbbd->_pidlCur, pidl1))
            _SetTitle(NULL);
        break;

    case SHCNE_RENAMEFOLDER:
    {
        // the rename might be ourselfs or our parent... if it's
        // our parent, we want to tack on the child idl's from the renamed
        // parent to us onto the new pidl (pidlExtra).
        // then show that result.
        LPITEMIDLIST pidlChild;

        pidlChild = ILFindChild(pidl1, _pbbd->_pidlCur);
        if (pidlChild) 
        {
            LPITEMIDLIST pidlTarget;

            pidlTarget = ILCombine(pidl2, pidlChild);
            if (pidlTarget)
            {
                LPITEMIDLIST pidlReal = _SimpleToReal(pidlTarget);
                
                if (pidlReal) 
                {
                    BrowseObject(pidlReal,  SBSP_WRITENOHISTORY | SBSP_SAMEBROWSER);

                    ILFree(pidlReal);
                }

                ILFree(pidlTarget);
            }
        }
        break;
    }

    case SHCNE_UPDATEIMAGE:
        IUnknown_CPContainerInvokeParam(_pbbd->_pautoEDS,
                DIID_DWebBrowserEvents2, DISPID_TITLEICONCHANGE, NULL, 0);
#ifdef DEBUG
        if (_pbbd->_pautoEDS)
        {
            // Verify that every IExpDispSupport also supports IConnectionPointContainer
            IConnectionPointContainer *pcpc;
            IExpDispSupport* peds;

            if (SUCCEEDED(_pbbd->_pautoEDS->QueryInterface(IID_IConnectionPointContainer, (void **)&pcpc)))
            {
                pcpc->Release();
            }
            else if (SUCCEEDED(_pbbd->_pautoEDS->QueryInterface(IID_IExpDispSupport, (void **)&peds)))
            {
                peds->Release();
                AssertMsg(0, TEXT("IExpDispSupport without IConnectionPointContainer for %08x"), _pbbd->_pautoEDS);
            }
        }
#endif
        v_SetIcon();
        break;
    }
}

//---------------------------------------------------------------------------
// Helper Function to see if a pidl is on a network drive which is not
// persistent.  This is useful if we are shuting down and saving a list
// of the open windows to restore as we won't be able to restore these.
#define AnsiUpperChar(c) ( (TCHAR)LOWORD(CharUpper((LPTSTR)MAKELONG(c, 0))) )

BOOL FPidlOnNonPersistentDrive(LPCITEMIDLIST pidl)
{
    TCHAR szPath[MAX_PATH];
    HANDLE hEnum;
    BOOL fRet = TRUE;

    TraceMsg(DM_SHUTDOWN, "csb.wp: FPidlOnNonPersistentDrive(pidl=%x)", pidl);
    if (!SHGetPathFromIDList(pidl, szPath) || (szPath[0] == TEXT('\0')))
        return(FALSE);  // not file system pidl assume ok.

    TraceMsg(DM_SHUTDOWN, "csb.wp: FPidlOnNonPersistentDrive - After GetPath=%s)", szPath);
    if (PathIsUNC(szPath) || !IsNetDrive(DRIVEID(szPath)))
    {
        fRet = FALSE;
        goto End;
    }

    // Ok we got here so now we have a network drive ...
    // we will have to enumerate over
    //
    if (WNetOpenEnum(RESOURCE_REMEMBERED, RESOURCETYPE_DISK,
            RESOURCEUSAGE_CONTAINER | RESOURCEUSAGE_ATTACHED,
            NULL, &hEnum) == WN_SUCCESS)
    {
        DWORD dwCount=1;
        union
        {
            NETRESOURCE nr;         // Large stack usage but I
            TCHAR    buf[1024];      // Dont think it is thunk to 16 bits...
        }nrb;

        DWORD   dwBufSize = SIZEOF(nrb);

        while (WNetEnumResource(hEnum, &dwCount, &nrb.buf,
                &dwBufSize) == WN_SUCCESS)
        {
            // We only want to add items if they do not have a local
            // name.  If they had a local name we would have already
            // added them!
            if ((nrb.nr.lpLocalName != NULL) &&
                    (AnsiUpperChar(*(nrb.nr.lpLocalName)) == AnsiUpperChar(szPath[0])))
            {
                fRet = FALSE;
                break;
            }
        }
        WNetCloseEnum(hEnum);
    }

End:
    TraceMsg(DM_TRACE, "c.c_arl: %s, is Persistent? %d", szPath, fRet);
    return(fRet);


}

void HackToPrepareForEndSession(LPCITEMIDLIST pidl)
{
    TCHAR szPath[MAX_PATH];

    TraceMsg(DM_SHUTDOWN, "csb.wp: HackToPrepareForEndSession(pidl=%x)", pidl);
    SHGetPathFromIDList(pidl, szPath);
}

//---------------------------------------------------------------------------
// returns:
//      TRUE if the user wants to abort the startup sequence
//      FALSE keep going
//
// note: this is a switch, once on it will return TRUE to all
// calls so these keys don't need to be pressed the whole time
BOOL AbortStartup()
{
    static BOOL bAborted = FALSE;       // static so it sticks!

    // TraceMsg(DM_TRACE, "Abort Startup?");

    if (bAborted)
        return TRUE;    // don't do funky startup stuff
    else {
        bAborted = (GetSystemMetrics(SM_CLEANBOOT) || ((GetAsyncKeyState(VK_CONTROL) < 0) || (GetAsyncKeyState(VK_SHIFT) < 0)));
        return bAborted;
    }
}

//---------------------------------------------------------------------------
// Restore all of the window that asked to save a command line to be
// restarted when windows was exited.
//
BOOL AddToRestartList(DWORD dwFlags, LPCITEMIDLIST pidl)
{
    int cItems = 0;
    DWORD cbData = SIZEOF(cItems);
    TCHAR szSubKey[80];
    BOOL fRet = FALSE;
    IStream *pstm;

    // cases that we don't want to save window state for...

    if (SHRestricted(REST_NOSAVESET) || FPidlOnNonPersistentDrive(pidl))
        return FALSE;

    if (ERROR_SUCCESS != (SHGetValueGoodBoot(SHGetExplorerHkey(), TEXT("RestartCommands"), TEXT("Count"), NULL, (BYTE *)&cItems, &cbData)))
    cItems = 0;

    // Now Lets Create a registry Stream for this guy...
    wnsprintf(szSubKey, ARRAYSIZE(szSubKey), TEXT("%d"), cItems);
    pstm = OpenRegStream(SHGetExplorerHkey(), TEXT("RestartCommands"), szSubKey, STGM_WRITE);
    TraceMsg(DM_SHUTDOWN, "csb.wp: AddToRestartList(pstm=%x)", pstm);
    if (pstm)
    {
        WORD wType = (WORD)-1;    // SIZEOF of cmd line == -1 implies pidl...

        // Now Write a preamble to the start of the line that
        // tells wType that this is an explorer
        pstm->Write(&wType, SIZEOF(wType), NULL);

        // Now Write out the version number of this stream
        // Make sure to inc the version number if the structure changes
        pstm->Write(&c_wVersion, SIZEOF(c_wVersion), NULL);

        // Now Write out the dwFlags
        pstm->Write(&dwFlags, SIZEOF(dwFlags), NULL);
        
        // And the pidl;
        ILSaveToStream(pstm, pidl);

        // And Release the stream;
        pstm->Release();

        cItems++;   // Say that there are twice as many items...

        fRet = (ERROR_SUCCESS == SHSetValue(SHGetExplorerHkey(), 
            TEXT("RestartCommands"), TEXT("Count"), REG_BINARY, (LPBYTE)&cItems, SIZEOF(cItems)));
    }

    return fRet;
}

//---------------------------------------------------------------------------
void SHCreateSavedWindows(void)
{
    int cItems = 0;
    DWORD cbData = SIZEOF(cItems);

    SHGetValueGoodBoot(SHGetExplorerHkey(), TEXT("RestartCommands"), TEXT("Count"), NULL, (PBYTE)&cItems, &cbData);

    // walk in the reverse order that they were added.
    for (cItems--; cItems >= 0; cItems--)
    {
        TCHAR szName[80];
        IStream *pstm;

        if (AbortStartup())
            break;

        wnsprintf(szName, ARRAYSIZE(szName), TEXT("%d"), cItems);
        pstm = OpenRegStream(SHGetExplorerHkey(), TEXT("RestartCommands"), szName, STGM_READ);
        if (pstm)
        {
            WORD wType;
            if (SUCCEEDED(pstm->Read(&wType, SIZEOF(wType), NULL)))
            {
                if (wType == (WORD)-1)
                {
                    WORD wVersion;
                    DWORD dwFlags;
                    LPITEMIDLIST pidl = NULL;       // need to be inited for ILLoadFromStream()

                    // We have a folder serialized so get:
                    //     WORD:wVersion, DWORD:dwFlags, PIDL:pidlRoot, PIDL:pidl

                    if (SUCCEEDED(pstm->Read(&wVersion, SIZEOF(wVersion), NULL)) &&
                        (wVersion == c_wVersion) &&
                        SUCCEEDED(pstm->Read(&dwFlags, SIZEOF(dwFlags), NULL)) && 
                        SUCCEEDED(ILLoadFromStream(pstm, &pidl)) && pidl)
                    {
                        // this call does window instance management 
                        IETHREADPARAM* piei = SHCreateIETHREADPARAM(NULL, 0, NULL, NULL);
                        if (piei) 
                        {
                            piei->pidl = pidl;
                            pidl = NULL;     // so this is not freed below
                            piei->uFlags = dwFlags;
                            piei->nCmdShow = SW_SHOWDEFAULT;
                            SHOpenFolderWindow(piei);
                        }
                        ILFree(pidl);
                    }
                }
                else if (wType < MAX_PATH)
                {
                    // BUGBUG: I don't think this is ever used.
                    CHAR aszScratch[MAX_PATH];
                    pstm->Read(aszScratch, wType, NULL);
                    WinExec(aszScratch, SW_SHOWNORMAL);      // the show cmd will be ignored
                }
            }
            pstm->Release();
        }
    }

    SHDeleteKey(SHGetExplorerHkey(), TEXT("RestartCommands"));
}


/****************************************************\
    FUNCTION: PrepContextMenuForSelfView

    DESCRIPTION:
        Sometimes we get the context menu of our own
    folder.  In this case, the current window is looking
    and folder x and the user clicked on the icon in
    the caption bar for the folder x.  This means that
    the context menu will be for the current folder
    but will behave differently than a background
    context menu.  It can't act completely like a 
    normal Context Menu because we are already
    in the folder, so we don't want Open.

    NOTE:
        This code is duplicated in
    \shell\ext\html\winnt\webvw\fldricon.cpp.
\****************************************************/
HRESULT PrepContextMenuForSelfView(IContextMenu * pcm, HMENU hpopup, UINT idFirst, UINT idLast)
{
    HRESULT hr = S_OK;
    UINT citems = GetMenuItemCount(hpopup);
    int ipos;
    UINT idCmd;

    // We need to remove "open"
    // Notes: We assume the context menu handles
    //  GetCommandString AND they are top-level menuitems.
    for (ipos = citems - 1; ipos >= 0; ipos--)
    {
        idCmd = GetMenuItemID(hpopup, ipos);
        if (IsInRange(idCmd, idFirst, idLast))
        {
            TCHAR szVerb[40];

            hr = ContextMenu_GetCommandStringVerb(pcm, (idCmd - idFirst), szVerb, ARRAYSIZE(szVerb));
            if (SUCCEEDED(hr))
            {
                if (StrCmpI(szVerb, TEXT("open"))==0)
                {
                    DeleteMenu(hpopup, ipos, MF_BYPOSITION);
                }
            }
        }
    }
        
    return hr;
}


//
//  This code intercept the WM_CONTEXTMENU message from USER and popups
// up the context menu of the folder itself when the user clicks the icon
// at the left-top corner of the frame (only when it is in the folder mode).
//
BOOL CShellBrowser2::v_OnContextMenu(WPARAM wParam, LPARAM lParam)
{
    BOOL fProcessed = FALSE;

    TraceMsg(DM_TRACE, "CABWND.C Got WM_CONTEXTMENU");
    
    if (_pbbd->_pidlCur && !ILIsEmpty(_pbbd->_pidlCur) && SendMessage(_pbbd->_hwnd, WM_NCHITTEST, 0, lParam) == HTSYSMENU)
    {
        IShellFolder *psfParent;
        LPCITEMIDLIST pidlChild;

        if (SUCCEEDED(IEBindToParentFolder(_pbbd->_pidlCur, &psfParent, &pidlChild))) 
        {
            IContextMenu * pcm;
            HRESULT hres = psfParent->GetUIObjectOf(_pbbd->_hwnd, 1, (LPCITEMIDLIST*)&pidlChild, IID_IContextMenu, NULL, (void **)&pcm);
            if (SUCCEEDED(hres))
            {
                HMENU hpopup = LoadMenuPopup(MENU_SYSPOPUP);
                if (hpopup)
                {
                    UINT idCmd;
                    UINT citems = GetMenuItemCount(hpopup);
                    pcm->QueryContextMenu(hpopup, citems, IDSYSPOPUP_FIRST, IDSYSPOPUP_LAST, 0);

                    hres = PrepContextMenuForSelfView(pcm, hpopup, IDSYSPOPUP_FIRST, IDSYSPOPUP_LAST);

                   // For sendto menu, we go on even if this fails
                    pcm->QueryInterface(IID_IContextMenu3, (void **)&_pcm);
                    
                    if (GetMenuItemCount(hpopup) > 1) {
                        // only do this if the context menu added something...
                        // otherwise we end up with a dorky "close" menu
                        
                        fProcessed=TRUE;
                        
                        idCmd = TrackPopupMenu(hpopup,
                                               TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                                               GET_X_LPARAM(lParam),
                                               GET_Y_LPARAM(lParam),
                                               0, _pbbd->_hwnd, NULL);

                        switch(idCmd)
                        {
                        case 0:
                            break;  // canceled

                        case IDSYSPOPUP_CLOSE:
                            _pbbd->_pautoWB2->Quit();
                            break;

                        default:
                        {
                            TCHAR szPath[MAX_PATH];
                            // unless we KNOW that our target can handle CommandInfoEx, we cannot send it to them
                            CMINVOKECOMMANDINFO ici = {
                                sizeof(ici),
                                0L,
                                _pbbd->_hwnd,
                                (LPSTR)MAKEINTRESOURCE(idCmd - IDSYSPOPUP_FIRST),
                                NULL, NULL,
                                SW_NORMAL
                            };
#ifdef UNICODE
                            CHAR szPathAnsi[MAX_PATH];
                            SHGetPathFromIDListA(_pbbd->_pidlCur, szPathAnsi);
                            SHGetPathFromIDList(_pbbd->_pidlCur, szPath);
                            ici.lpDirectory = szPathAnsi;
//                            ici.lpDirectoryW = szPath;
                            ici.fMask |= CMIC_MASK_UNICODE;
#else
                            SHGetPathFromIDList(_pbbd->_pidlCur, szPath);
                            ici.lpDirectory = szPath;
#endif
                            pcm->InvokeCommand(&ici);
                            break;
                        }
                        }
                    }

                    ATOMICRELEASE(_pcm);
                    DestroyMenu(hpopup);
                }
                pcm->Release();
            }
            psfParent->Release();

        }
    }
    return fProcessed;
}

void CShellBrowser2::_OnClose(BOOL fPushed)
{
    // We can't close if it's nested.
    if (fPushed) 
    {
#ifdef NO_MARSHALLING
        // IEUNIX : Mark this window  for delayed deletion from the main message
        // pump. The problem is , if scripting closes a window and immediately 
        // opens a modal dialog. The WM_CLOSE message  for the browser window is
        // dispatched from the modal loop and we end up being called from the 
        // window proc. This  happens a lot on UNIX because we have multiple 
        // browser windows on the same thread.
        if (!_fDelayedClose)
            _fDelayedClose = TRUE;
        else
#endif
        MessageBeep(0);
        return;
    }

    if (SHIsRestricted2W(_pbbd->_hwnd, REST_NoBrowserClose, NULL, 0))
        return;

    // We are not supposed to process WM_CLOSE if modeless operation is
    // disabled.
    if (S_OK == _DisableModeless()) 
    {
        TraceMsg(DM_ERROR, "CShellBrowser2::_OnClose called when _DisableModeless() is TRUE. Ignored.");
        MessageBeep(0);
        UINT id = MLShellMessageBox(_pbbd->_hwnd,
               MAKEINTRESOURCE(IDS_CLOSEANYWAY),
               MAKEINTRESOURCE(IDS_TITLE),
               MB_OKCANCEL | MB_SETFOREGROUND | MB_ICONSTOP);
        if (id == IDCANCEL) 
        {
#ifdef NO_MARSHALLING
            _fReallyClosed = FALSE;
#endif
            return;
        }
    }

#ifdef NO_MARSHALLING
    _fReallyClosed = TRUE;
#endif

    // we cannot close in the middle of creating view window.  
    // someone dispatched messages and it wasn't us...
    // we WILL fault.
    // bugbug:  after ie3, we can flag this and close when we're done tryingto create the
    // viewwindow
    if (_pbbd->_fCreatingViewWindow)
        return;

    if (_MaySaveChanges() != S_FALSE) 
    {
        // Close the browse context and release it.
        IHlinkBrowseContext * phlbc = NULL;
        
        if (_pbbd->_phlf)
            _pbbd->_phlf->GetBrowseContext(&phlbc);
        
        if (phlbc) 
        {
            _pbbd->_phlf->SetBrowseContext(NULL);
            phlbc->Close(0);
            phlbc->Release();
        }

        FireEvent_Quit(_pbbd->_pautoEDS);
        
        // this is once we KNOW that we're going to close down
        // give subclasses a chance to clean up
#ifdef NO_MARSHALLING
        RemoveBrowserFromList(this);
#endif
        _OnConfirmedClose();
    }


    //
    // NOTES: Originally, this call was made only for RISC platform.
    //  We, however, got a request from ISVs that their OCs should be
    //  unloaded when the user closes the window.
    //
    //  On risc NT we need to call CoFreeUnusedLibraries in case any x86 dlls
    //  were loaded by Ole32. We call this after calling _OnClose so that
    //  we can even unload the OC on the current page. (SatoNa)
    //
    CoFreeUnusedLibraries();
}

//
// stolen from comctl32
//
// in:
//      hwnd    to do check on
//      x, y    in client coordinates
//
// returns:
//      TRUE    the user began to drag (moved mouse outside double click rect)
//      FALSE   mouse came up inside click rect
//
// BUGBUG, should support VK_ESCAPE to cancel

BOOL CheckForDragBegin(HWND hwnd, int x, int y)
{
    RECT rc;
    int dxClickRect = GetSystemMetrics(SM_CXDRAG);
    int dyClickRect = GetSystemMetrics(SM_CYDRAG);

    ASSERT((dxClickRect > 1) && (dyClickRect > 1));

    // See if the user moves a certain number of pixels in any direction

    SetRect(&rc, x - dxClickRect, y - dyClickRect, x + dxClickRect, y + dyClickRect);

    MapWindowRect(hwnd, NULL, &rc);

    SetCapture(hwnd);

    do 
    {
        MSG msg;

        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // See if the application wants to process the message...
            if (CallMsgFilter(&msg, MSGF_COMMCTRL_BEGINDRAG) != 0)
                continue;

            switch (msg.message) {
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
                ReleaseCapture();
                return FALSE;

            case WM_MOUSEMOVE:
                if (!PtInRect(&rc, msg.pt)) 
                {
                    ReleaseCapture();
                    return TRUE;
                }
                break;

            default:
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                break;
            }
        }

        // WM_CANCELMODE messages will unset the capture, in that
        // case I want to exit this loop
    } while (GetCapture() == hwnd);

    return FALSE;
}

void CShellBrowser2::_SetTheaterBrowserBar()
{
#ifndef DISABLE_FULLSCREEN
    if (_ptheater) {
        IDeskBar *pdbBar = NULL;
        
        if (_fShowBrowserBar)
            FindToolbar(INFOBAR_TBNAME, IID_IDeskBar, (void **)&pdbBar);
        
        _ptheater->SetBrowserBar(pdbBar, 120, 200);
        
        if (pdbBar)
            pdbBar->Release();
    }
#endif
}

void CShellBrowser2::_TheaterMode(BOOL fShow, BOOL fRestorePrevious)
{
#ifndef DISABLE_FULLSCREEN
    if (BOOLIFY(fShow) == BOOLIFY(_ptheater))
        return;
    
    WINDOWPLACEMENT wp;
    RECT rc;
    if (fRestorePrevious && !fShow) {
        _ptheater->GetPreviousWindowPlacement(&wp, &rc);
    } else 
        fRestorePrevious = FALSE;
        
    HRESULT hresResize = _pbsInner->AllowViewResize(FALSE);
    
    if (!fShow) 
    {    
        if (_ptheater) {            
            if (fRestorePrevious) {
                PutBaseBrowserData()->_hwnd = _ptheater->GetMasterWindow();
#ifndef UNIX
                SHSetWindowBits(_pbbd->_hwnd, GWL_STYLE, WS_DLGFRAME | WS_THICKFRAME | WS_BORDER, WS_DLGFRAME | WS_THICKFRAME | WS_BORDER);
#else
                SHSetWindowBits(_pbbd->_hwnd, GWL_STYLE, WS_DLGFRAME | WS_THICKFRAME | WS_BORDER, 0);
#endif
                SetWindowPos(_pbbd->_hwnd, NULL, 0, 0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
            }
            delete _ptheater;
            _ptheater = NULL;
        }
    } 
    else 
    {
        _SaveITbarLayout();
        _SaveState();
        SHSetWindowBits(_pbbd->_hwnd, GWL_STYLE, WS_DLGFRAME | WS_THICKFRAME, 0);
        // if we're going to theater mode, don't allow best fit stuff
        _fsd._fs.fFlags &= ~FWF_BESTFITWINDOW;
        
        HWND hwndToolbar = NULL;
        
        if (_GetITBar())
            _GetITBar()->GetWindow(&hwndToolbar);

        _ptheater = new CTheater(_pbbd->_hwnd, hwndToolbar, (IOleCommandTarget*)this);
        if (_ptheater) 
        {
            _SetTheaterBrowserBar();

            // the progress control is a bit special in this mode.  we pull this out and make it topmost.
            _ShowHideProgress();
        }
    }
    
    // the itbar is special in that it stays with the auto-hide window.
    // it needs to know about theater mode specially
    // Also, set _ptheater->_fAutoHideToolbar to _pitbar->_fAutoHide
    VARIANT vOut = { VT_I4 };
    vOut.lVal = FALSE;  // default: no auto hide explorer toolbar
    IUnknown_Exec(_GetITBar(), &CGID_PrivCITCommands, CITIDM_THEATER, fShow ? THF_ON : THF_OFF, &vOut, &vOut);

    if (_ptheater)
    {
        _ptheater->SetAutoHideToolbar(vOut.lVal);
    }

    _pbsInner->AllowViewResize(hresResize == S_OK);
    SUPERCLASS::OnSize(SIZE_RESTORED);

    if (_ptheater)
        _ptheater->Begin();     // kick start!

    // notify trident that it's ambients are invalid to force
    // it to re-query us for the flat property
    if (_pbbd->_pctView) 
    {
        VARIANTARG vaIn;
        vaIn.vt = VT_I4;
        vaIn.lVal = DISPID_UNKNOWN;

        _pbbd->_pctView->Exec(&CGID_ShellDocView, SHDVID_AMBIENTPROPCHANGE, NULL, &vaIn, NULL);
    }

    if (_pxtb) 
    {
        UINT uiState;
        if (SUCCEEDED(_pxtb->GetState(&CLSID_CommonButtons, TBIDM_THEATER, &uiState))) 
        {
            if (_ptheater)
                uiState |= TBSTATE_CHECKED;
            else
                uiState &= ~TBSTATE_CHECKED;
            
            _pxtb->SetState(&CLSID_CommonButtons, TBIDM_THEATER, uiState);
        }
    }

    if (!_ptheater && !_fShowMenu)
        IUnknown_Exec(_GetITBar(), &CGID_PrivCITCommands, CITIDM_SHOWMENU, FALSE, NULL, NULL);

    if (fRestorePrevious) 
    {
        SetWindowPos(_pbbd->_hwnd, NULL, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), 0);
        if (IsWindowVisible(_pbbd->_hwnd)) 
        {
            ShowWindow(_pbbd->_hwnd, wp.showCmd);
            SetWindowPlacement(_pbbd->_hwnd, &wp);
        }
    }
#endif /* !DISABLE_FULLSCREEN */
}

BOOL CShellBrowser2::_OnSysMenuClick(BOOL bLeftClick, WPARAM wParam, LPARAM lParam)
{
    POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    DWORD dwStart = GetTickCount();

    MapWindowPoints(NULL, _pbbd->_hwnd, &pt, 1);

    if (!CheckForDragBegin(_pbbd->_hwnd, pt.x, pt.y))
    {
        if (bLeftClick)
        {
            DWORD dwDelta = (GetTickCount() - dwStart);
            DWORD dwDblClick = GetDoubleClickTime();

            if (dwDelta < dwDblClick)
            {
                // HACK: use the lParam (coords) as the timer ID to communicate
                // that to the WM_TIMER handler
                //
                // HACK: store the timer id in a global. Since there's only one
                // double-click on a sysmenu at a time, this should be fine.
                if (g_sysmenuTimer)
                    KillTimer(_GetCaptionWindow(), g_sysmenuTimer);

                // We are special casing 0 as meaning there is no timer, so if the coords come in at
                // 0 then cheat them to 1.
                if (lParam == 0)
                    lParam++;

                g_sysmenuTimer = SetTimer(_GetCaptionWindow(), lParam, dwDblClick - dwDelta, NULL);
            }
            else
                DefWindowProcWrap(_pbbd->_hwnd, WM_CONTEXTMENU, (WPARAM)_pbbd->_hwnd, lParam);
        }
        else
            SendMessage(_pbbd->_hwnd, WM_CONTEXTMENU, (WPARAM)_pbbd->_hwnd, lParam);
        return FALSE;
    }
    IOleCommandTarget *pcmdt = NULL;
    if (_pbbd->_pautoWB2)
    {
        (_pbbd->_pautoWB2)->QueryInterface(IID_IOleCommandTarget, (void **)&pcmdt);
        ASSERT(pcmdt);
    }
    
    BOOL fRet = DoDragDropWithInternetShortcut(pcmdt, _pbbd->_pidlCur, _pbbd->_hwnd);
    
    if (pcmdt)
        pcmdt->Release();

    return fRet;
}

void _SetWindowsListMarshalled(IWebBrowser2 *pautoWB2)
{
    IEFrameAuto* pief;
    if (SUCCEEDED(pautoWB2->QueryInterface(IID_IEFrameAuto, (void **)&pief))) 
    {
        pief->OnWindowsListMarshalled();
        pief->Release();
    }
}

void CShellBrowser2::_OnTimer(UINT_PTR idTimer)
{
    // HACK: _OnSysMenuClick uses the cursor coords as the timer ID.
    // So first check if g_sysmenuTimer is set before checking for
    // standard timer IDs.
    
    if (g_sysmenuTimer)
    {
        KillTimer(_GetCaptionWindow(), g_sysmenuTimer);
        g_sysmenuTimer = 0;

        // the timer ID is the lParam from the left click!
        SendMessage(_GetCaptionWindow(), WM_SYSMENU, 0, idTimer);
    }
    else
    {
        switch (idTimer)
        {
        case SHBTIMER_MENUSELECT:
            KillTimer(_pbbd->_hwnd, SHBTIMER_MENUSELECT);

            if (_pidlMenuSelect)
            {
                _DisplayFavoriteStatus(_pidlMenuSelect);
                Pidl_Set(&_pidlMenuSelect, NULL);
            }
            break;
        }
    }
}


void CShellBrowser2::_OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (LOWORD(wParam) == WA_INACTIVE)
        _fActivated = FALSE;
    else
        _fActivated = TRUE;

    if (_pbbd->_hwndView)
        SendMessage(_pbbd->_hwndView, uMsg, wParam, lParam);

    if (LOWORD(wParam) != WA_INACTIVE)
    {
        // remember who had focus last, since trident will
        // grab focus on an OnFrameWindowActivate(TRUE)
        int itbLast = _pbsOuter->_get_itbLastFocus();

        _pbsOuter->OnFrameWindowActivateBS(TRUE);

        if (itbLast != ITB_VIEW)
        {
            // restore focus to its rightful owner
            LPTOOLBARITEM ptbi = _GetToolbarItem(itbLast);
            if (ptbi)
                UnkUIActivateIO(ptbi->ptbar, TRUE, NULL);
        }
    }
    else
    {
#ifdef KEYBOARDCUES
        if (_pbbd->_hwnd)
        {
            SendMessage(_pbbd->_hwnd, WM_CHANGEUISTATE,
                MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS | UISF_HIDEACCEL), 0);
        }
#endif
        _pbsOuter->OnFrameWindowActivateBS(FALSE);
    }
}


// Main window proc for CShellBrowser2

LRESULT CShellBrowser2::WndProcBS(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet = 0;

    if (_TranslateMenuMessage(hwnd, uMsg, &wParam, &lParam, &lRet))
        return lRet;

    switch(uMsg)
    {
    case WMC_DISPATCH:
        BSTR Url;
        {

            HRESULT hres;
            switch(wParam) {
            case DSID_NOACTION:
                return S_OK;
           
            case DSID_NAVIGATEIEBROWSER:
//
//  BUGBUG: To be fully compatible with IE 2.0, we don't want to use
// the window that has a navigation in progress. Enabling this code,
// however, causes some problem with the very first DDE. I need to
// investigate more, IF we need that level of compatibility. (SatoNa)
//
                
                // this is only used for IE Browser.
                // if this is not in iemode, then fail.
                // this prevents us from reusing C:\ to navigate to www
                if  (!v_IsIEModeBrowser())
                    return E_FAIL;

                ASSERT(lParam);
                if (!lParam)
                    break;
                Url = ((DDENAVIGATESTRUCT*)lParam)->wszUrl;
                hres = _pbbd->_pautoWB2->Navigate(Url, NULL, NULL, NULL, NULL);

                return hres;
                break;

            case DSID_GETLOCATIONURL:
                return _pbbd->_pautoWB2->get_LocationURL((BSTR*)lParam);

            case DSID_GETLOCATIONTITLE:
                return _pbbd->_pautoWB2->get_LocationName((BSTR*)lParam);
            
            case DSID_GETHWND:
                *(HWND*)lParam = hwnd;
                return S_OK;
#if 0
            case DSID_CANCEL:
                return _pbbd->_pautoWB2->Stop();
#endif
            case DSID_EXIT:
                return _pbbd->_pautoWB2->Quit();
            }
        }
        return (LRESULT)HRESULT_FROM_WIN32(ERROR_BUSY);
        
    case CWM_CLONEPIDL:
        if (_pbbd->_pidlCur)
        {
            return (LRESULT)SHAllocShared(_pbbd->_pidlCur, ILGetSize(_pbbd->_pidlCur), (DWORD)wParam);
        }
        break;
        
    case CWM_SELECTITEM:
    {
        LPITEMIDLIST pidl = (LPITEMIDLIST)SHLockShared((HANDLE)lParam, GetCurrentProcessId());
        if (pidl)
        {
            if (_pbbd->_psv) 
                _pbbd->_psv->SelectItem(pidl, (UINT)wParam);
            SHUnlockShared(pidl);
        }
        SHFreeShared((HANDLE)lParam, GetCurrentProcessId());   // Receiver responsible for freeing
        break;
    }
    
    case CWM_THEATERMODE:
        _TheaterMode(BOOLFROMPTR(wParam), !wParam);
        break;
        
    case CWM_GLOBALSTATECHANGE:
        // need to update the title
        if (wParam == CWMF_GLOBALSTATE)
            _SetTitle(NULL);
        else if (wParam == CWMF_SECURITY)
        {
            _UpdateZonesPane(NULL);
        }
        break;

        
    case CWM_FSNOTIFY:
        _OnFSNotify(wParam, lParam);
        break;
        
    case CWM_UPDATEBACKFORWARDSTATE:
        _UpdateBackForwardStateNow();
        break;

    case CWM_SHOWDRAGIMAGE:
        return DAD_ShowDragImage((BOOL)lParam);

    case WM_ENDSESSION:
        TraceMsg(DM_SHUTDOWN, "csb.wp: WM_ENDSESSION wP=%d lP=%d", wParam, lParam);
        if (wParam && IsWindowVisible(_pbbd->_hwnd) && _pbbd->_pidlCur && !_fUISetByAutomation) 
        {
            TraceMsg(DM_SHUTDOWN, "csb.wp: call AddToRestartList");
            if (!IsBrowserFrameOptionsSet(_pbbd->_psf, BFO_NO_REOPEN_NEXT_RESTART))
                AddToRestartList(v_RestartFlags(), _pbbd->_pidlCur);

            // for now we use the same 12-hour (SessionTime) rule
            // possibly we should just do it always?
            UEMFireEvent(&UEMIID_BROWSER, UEME_CTLSESSION, UEMF_XEVENT, FALSE, -1);
            if (!g_bRunOnNT5) {
                // for down-level guys (old explorer), fake a shell end session too
                UEMFireEvent(&UEMIID_SHELL, UEME_CTLSESSION, UEMF_XEVENT, FALSE, -1);
            }
            // And make sure we have saved it's state out
            TraceMsg(DM_SHUTDOWN, "csb.wp: call _SaveState");
            _SaveState();
        }
        TraceMsg(DM_SHUTDOWN, "csb.wp: WM_ENDSESSION return 0");
        break;

    case WM_TIMER:
        _OnTimer(wParam);
        break;

    case WM_NCLBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
        if (wParam != HTSYSMENU)
            goto DoDefault;

        _OnSysMenuClick(uMsg == WM_NCLBUTTONDOWN, wParam, lParam);
        break;

    case WM_NCLBUTTONDBLCLK:
        // We fault while shutting down the window if the timing is bad.
        // We're not sure why, but USER get's mightily confused. The only
        // difference between this scheme and a normal double-click-on-sysmenu
        // is the timer we have hanging around. Kill the timer before
        // processing this message. Hopefully that will work [mikesh/cheechew]
        //
        // HACK: remember this timer id is stored in a global variable
        //
        if (g_sysmenuTimer)
        {
            KillTimer(_GetCaptionWindow(), g_sysmenuTimer);
            g_sysmenuTimer = 0;
        }

        // We still want to process this DBLCLK
        goto DoDefault;
        
    case WM_CONTEXTMENU:
        if (!v_OnContextMenu(wParam, lParam))
            goto DoDefault;
        break;

    case WM_WININICHANGE:
        {
            DWORD dwSection = SHIsExplorerIniChange(wParam, lParam);

            // Hack for NT4 and Win95, where there is no SPI_GETMENUSHOWDELAY
            // Don't need to check wParam == SPI_SETMENUSHOWDELAY since we
            // always query afresh on NT5/Win98.
            if (dwSection & EICH_SWINDOWS)
                g_lMenuPopupTimeout = -1; /* in case MenuShowDelay changed */

            // Transitioning to/from "Working Offline" just broadcasts (0,0)
            // so that's all we listen for
            if (dwSection == EICH_UNKNOWN)
            {
                _ReloadTitle();
                _ReloadStatusbarIcon();
            }
        }
        goto DoDefault;

    case WM_INITMENUPOPUP:
        v_OnInitMenuPopup((HMENU)wParam, LOWORD(lParam), HIWORD(lParam));
        v_ForwardMenuMsg(uMsg, wParam, lParam);
        break;

    case WM_MENUSELECT:
#ifdef UNIX
        /* IEUNIX:  Avoid getting delayed timer messages, which screws up 
           statusbar. */
        if (_pidlMenuSelect) {
            KillTimer(_pbbd->_hwnd, SHBTIMER_MENUSELECT);
            Pidl_Set(&_pidlMenuSelect, NULL);
        }
#endif

        if (_ShouldForwardMenu(uMsg, wParam, lParam)) 
        {
            ForwardViewMsg(uMsg, wParam, lParam);
        } 
        else
        {
            BOOL fIsPopup = GET_WM_MENUSELECT_FLAGS(wParam, lParam) & MF_POPUP;

            if ((!_OnMenuSelect(wParam, lParam, 0) &&
                 (fIsPopup || IsInRange(LOWORD(wParam), FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST)))

                || (_fDispatchMenuMsgs && fIsPopup))
            {
                ForwardViewMsg(uMsg, wParam, lParam);
            }

        }
        
        break;
        
    case WM_EXITSIZEMOVE:
        _fDisallowSizing = FALSE;
        break;

    case WM_WINDOWPOSCHANGING:
        if (_fDisallowSizing) 
        {
            LPWINDOWPOS pwp = (LPWINDOWPOS)lParam;
            pwp->flags |= SWP_NOMOVE | SWP_NOSIZE;
        }
        break;
        
    case WM_ENTERSIZEMOVE:
        if (_ptheater)
            _fDisallowSizing = TRUE;
        break;

    case WM_EXITMENULOOP:
    case WM_ENTERMENULOOP:
        ForwardViewMsg(uMsg, wParam, lParam);
        break;

    case WM_DRAWITEM:
    case WM_MEASUREITEM:
        if (_ShouldForwardMenu(uMsg, wParam, lParam))
            ForwardViewMsg(uMsg, wParam, lParam);
        else
        {
            UINT  idCmd;
        
            switch (uMsg)
            {
                case WM_MEASUREITEM:
                    idCmd = GET_WM_COMMAND_ID(((MEASUREITEMSTRUCT *)lParam)->itemID, 0);
                    break;
                case WM_DRAWITEM:
                    idCmd = GET_WM_COMMAND_ID(((LPDRAWITEMSTRUCT)lParam)->itemID, 0);
                    break;
            }
        
            if (InRange(idCmd, FCIDM_SEARCHFIRST, FCIDM_SEARCHLAST) && _pcmSearch)
                _pcmSearch->HandleMenuMsg(uMsg, wParam, lParam);
            else
            {                
                if ((!(_pcm && (_pcm->HandleMenuMsg(uMsg, wParam, lParam) == NOERROR))) &&
                    (!(_pcmNsc && (_pcmNsc->HandleMenuMsg(uMsg, wParam, lParam) == NOERROR))))
                        v_ForwardMenuMsg(uMsg, wParam, lParam);
            }
        }            
        break;

    case WM_QUERYENDSESSION:
        TraceMsg(DM_SHUTDOWN, "csb.wp: WM_QUERYENDSESSION");
        if (S_OK == _DisableModeless()) {
            TraceMsg(DM_WARNING, "CSB::WndProc got WM_QUERYENDSESSION when disabled");
            MessageBeep(0);
            UINT id = MLShellMessageBox(_pbbd->_hwnd,
                   MAKEINTRESOURCE(IDS_CLOSEANYWAY),
                   MAKEINTRESOURCE(IDS_TITLE),
                   MB_OKCANCEL | MB_SETFOREGROUND | MB_ICONSTOP);
            if (id==IDCANCEL) {
                return FALSE;
            }
        }
        // BUGBUG:: Try calling something that will call SHGetPathFromIDList to make sure we won't
        // call GetProcAddress while processing the WM_ENDSESSION...
        if (_pbbd->_pidlCur)
            HackToPrepareForEndSession(_pbbd->_pidlCur);

        return TRUE;    // OK to close

    case WM_CLOSE:
#ifdef NO_MARSHALLING
        _OnClose(_fOnIEThread);
#else
        _OnClose(TRUE);
#endif
        break;      

    case PUI_OFFICE_COMMAND:
    {
        switch (wParam)
        {
        case PLUGUI_CMD_SHUTDOWN:
        {
            HRESULT hr;
            VARIANT v;

            // first, kill the internet options modal
            // property sheet if it exists
            // it might be open because that's one of
            // out UI lang change scenarios

            V_VT(&v) = VT_BYREF;
            v.byref = NULL;

            if (_pbbd != NULL && _pbbd->_pctView != NULL)
            {
                hr = _pbbd->_pctView->Exec(&CGID_ShellDocView, SHDVID_GETOPTIONSHWND, 0, NULL, &v);
                if (SUCCEEDED(hr))
                {
                    ASSERT(V_VT(&v) == VT_BYREF);

                    if (v.byref != NULL)
                    {
                        // close the lang change modal property sheet
                        SendMessage((HWND)v.byref, WM_CLOSE, NULL, NULL);
                    }
                }
            }

            // now try to close the browser in general
            ASSERT(_pbbd != NULL && _pbbd->_pautoWB2 != NULL);
            _pbbd->_pautoWB2->Quit();

            break;
        }

        case PLUGUI_CMD_QUERY:
        {
            HMODULE hMod;

            // answer if we're an iexplore.exe process because
            // that means we're not sharing any dlls with the shell

            hMod = GetModuleHandle(TEXT("IEXPLORE.EXE"));

            if (hMod != NULL && !g_bRunOnNT5)
            {
                PLUGUI_QUERY    puiQuery;

                ASSERT(!g_bRunOnNT5);

                // we indicate that we participate in plugUI shutdown by
                // returning the version number for Office 9

                puiQuery.uQueryVal = 0;
                puiQuery.PlugUIInfo.uMajorVersion = OFFICE_VERSION_9;
                puiQuery.PlugUIInfo.uOleServer = FALSE;
                return puiQuery.uQueryVal;
            }
            break;
        }
        } // switch (wParam)

        break; // PUI_OFFICE_COMMAND
    }

    case WM_SYSCOMMAND:
        //
        // WARNING: User uses low four bits for some undocumented feature
        //  (only for SC_*). We need to mask those bits to make this case
        //  statement work.
        //
        switch (wParam & 0xfff0) {
        case SC_MAXIMIZE:                        
            if (GetKeyState(VK_CONTROL) < 0)
            {
                LPTOOLBARITEM ptbi = _GetToolbarItem(ITB_ITBAR);
                if (ptbi->fShow)    
                    PostMessage(_pbbd->_hwnd, CWM_THEATERMODE, TRUE, 0);
                else
                    goto DoDefault;
            }
            else
                goto DoDefault;
            break;
            
        case SC_CLOSE:
            // Make it posted so that we can detect if it's nested.
            PostMessage(_pbbd->_hwnd, WM_CLOSE, 0, 0);
            break;
            
        case SC_MINIMIZE:
            goto DoDefault;

        case SC_RESTORE:
            if (_ptheater && !_fMinimized)
            {
                PostMessage(_pbbd->_hwnd, CWM_THEATERMODE, FALSE, 0);
                return 0;
            }                         
            goto DoDefault;        

        default:
            goto DoDefault;
        }
        break;

    case WM_SIZE:
        // WARNING: Remember that we won't get WM_SIZE if we directly process
        // WM_WINDOWPOSCHANGED.
        {
            BOOL fMinimized = (wParam == SIZE_MINIMIZED);

            if (BOOLIFY(_fMinimized) != fMinimized)
            {
                TraceMsg(DM_ONSIZE, "SB::_WndProc WM_SIZE _fMinimized %d -> %d",
                         _fMinimized, fMinimized);
    
                _fMinimized = fMinimized;

                // Pause/Resume toolbars (intentionally ignores _pbbd->_psvPending). 
                VARIANT var = { 0 };
                var.vt = VT_I4;
                var.lVal = !_fMinimized;
                _ExecChildren(NULL, TRUE, NULL, OLECMDID_ENABLE_INTERACTION, OLECMDEXECOPT_DONTPROMPTUSER, &var, NULL);

                // Pause/Resule the view (refrelcts _pbbd->_psvPending too). 
                _PauseOrResumeView(_fMinimized);
            }
        }
#ifndef DISABLE_FULLSCREEN
        if (_ptheater && !_fMinimized)
            _ptheater->RecalcSizing();
#endif
        goto DoDefault;

    case WM_ACTIVATE:
#ifdef UNIX
        if (_HandleActivation( wParam ) == TRUE)
        {
            _OnActivate(uMsg, wParam, lParam);
            break;
        }
#endif
        _OnActivate(uMsg, wParam, lParam);
        break;

    case WM_SETFOCUS:
        goto DoDefault;

    case WM_MENUCHAR:
        {
            LRESULT lres;
            // Forwarding for IContextMenu3. 
            if (_pcm && _pcm->HandleMenuMsg2(uMsg, wParam, lParam, &lres) == S_OK)
                ; // do nothing
            else if (_pcmSearch && _pcmSearch->HandleMenuMsg2(uMsg, wParam, lParam, &lres) == S_OK)
                ; // do nothing
            else
                lres = v_ForwardMenuMsg(uMsg, wParam, lParam);
            return lres;
        }
        break;

    case WM_CREATE:
#ifdef KEYBOARDCUES
        SendMessage(hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_INITIALIZE, 0), 0);
#endif
        lRet = SUPERCLASS::WndProcBS(hwnd, uMsg, wParam, lParam);
        if (lRet)
        {
            _OnClose(FALSE);
            _GetMenuBand(TRUE);
        }
        return lRet;

    case WMC_MARSHALIDISPATCHSLOW:
        {
#ifndef NO_MARSHALLING
            IStream *pIStream;
            HRESULT hres = CreateStreamOnHGlobal(NULL, TRUE, &pIStream);
            if (SUCCEEDED(hres)) 
            {
                HANDLE hShared = NULL;
                _fMarshalledDispatch = TRUE;
                hres = CoMarshalInterface(pIStream, IID_IDispatch,
                    _pbbd->_pautoWB2, MSHCTX_NOSHAREDMEM, NULL, MSHLFLAGS_NORMAL);
                if (SUCCEEDED(hres))
                {
                    _SetWindowsListMarshalled(_pbbd->_pautoWB2);

                    ULARGE_INTEGER uliPos;
                    const LARGE_INTEGER li = {0,0};
                    pIStream->Seek(li, STREAM_SEEK_CUR, &uliPos);

                    // And point back to the beginning...
                    pIStream->Seek(li, STREAM_SEEK_SET, NULL);
    
                    hShared = SHAllocShared(NULL, uliPos.LowPart + sizeof(DWORD), (DWORD)lParam);
                    if (hShared)
                    {
                        LPBYTE pv = (LPBYTE)SHLockShared(hShared, (DWORD)lParam);
                        if (pv)
                        {
                            *((DWORD*)pv) = uliPos.LowPart;
                            pIStream->Read(pv + sizeof(DWORD), uliPos.LowPart, NULL);
                            SHUnlockShared(pv);
                        }
                        else
                        {
                            SHFreeShared(hShared, (DWORD)lParam);
                            hShared = NULL;
                        }
                    }
                }
                pIStream->Release();
                return (LRESULT)hShared;
            }
#else
            IDispatch **idispTemp = (IDispatch**)lParam;
            *idispTemp = _pbbd->_pautoWB2;
            return S_OK;
#endif
        }

    case WM_GETOBJECT:
        if (OBJID_MENU == lParam)
        {
            IAccessible* pacc;
            IMenuBand* pmb = _GetMenuBand(FALSE);

            if (pmb && SUCCEEDED(IUnknown_QueryService(pmb, SID_SMenuBandChild, 
                IID_IAccessible, (LPVOID*)&pacc)))
            {

                lRet = LresultFromObject(IID_IAccessible, wParam, pacc);

                if (FAILED((HRESULT)lRet))
                    pacc->Release();

                return lRet;
            }
        }
        break;

    default:
        lRet = _WndProcBSNT5(hwnd, uMsg, wParam, lParam);
        if (lRet)
            return lRet;

        if (_nMBIgnoreNextDeselect == uMsg)
        {
            _fIgnoreNextMenuDeselect = TRUE;
            TraceMsg(TF_MENUBAND, "MenuBand: Shbrowse.cpp received our private MBIgnoreNextDeselect");
            break;
        }
        else if (GetDDEExecMsg() == uMsg)
        {
            ASSERT(lParam && 0 == ((LPNMHDR)lParam)->idFrom);
            DDEHandleViewFolderNotify(this, _pbbd->_hwnd, (LPNMVIEWFOLDER)lParam);
            LocalFree((LPNMVIEWFOLDER)lParam);
            return TRUE;
        }
        else if (g_msgMSWheel == uMsg)
        {
            // Frame doesn't have scrollbar, let the view have a crack at it (309709)
            return SendMessage(_pbbd->_hwndView, uMsg, wParam, lParam);
        }

DoDefault:
        lRet = SUPERCLASS::WndProcBS(hwnd, uMsg, wParam, lParam);
        if (WM_COMMAND == uMsg)
        {
            ATOMICRELEASE(_pcmNsc);
        }

        return lRet;
    }

    return 0;
}

HRESULT CShellBrowser2::OnSetFocus()
{
    TraceMsg(DM_FOCUS, "csb.osf: hf()=%d itbLast=%d", _HasToolbarFocus(), _get_itbLastFocus());
    // forward to whoever had focus last (view or toolbar).  i think this
    // was added for ie4:55511 to fix pblm w/ tabbing away from IE and
    // then back.  note the check of _get_itbLastFocus w/o the usual
    // _HasToolbarFocus/_FixToolbarFocus magic...
    //
    // this used to be in CCB::OSF but that's bogus since in the desktop
    // case, this means once a deskbar (e.g. address) has focus, we can
    // never get focus back on the desktop (nt5:167864).
    if (_get_itbLastFocus() == ITB_VIEW) {
        // forward it on to view (in basesb)
        _pbsInner->OnSetFocus();
    } else {
        LPTOOLBARITEM ptbi = _GetToolbarItem(_get_itbLastFocus());
        if (ptbi)
            UnkUIActivateIO(ptbi->ptbar, TRUE, NULL);
    }
    return 0;
}


LRESULT CALLBACK IEFrameWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    CShellBrowser2* psb = (CShellBrowser2*)GetWindowPtr0(hwnd);

    switch(uMsg)
    {
    case WM_NCCREATE:
    {
        IETHREADPARAM* piei = (IETHREADPARAM*)((LPCREATESTRUCT)lParam)->lpCreateParams;

        ASSERT(psb == NULL);
#ifndef UNIX
        if (piei->uFlags & COF_EXPLORE) 
        {
            CExplorerBrowser_CreateInstance(hwnd, (void **)&psb);
        } 
        else
#endif
        {
            CShellBrowser2_CreateInstance(hwnd, (void **)&psb);
        }

        if (psb)
        {
            SetWindowLongPtr(hwnd, 0, (LONG_PTR)psb);

            // Hack: Let's try only registering dde on iexplorer windows for
            // shell speed.  (ie ignore shell folders)
            DWORD dwAttr = SFGAO_FOLDER;

            if ((!(piei->pidl &&
                   SUCCEEDED(IEGetAttributesOf(piei->pidl, &dwAttr)) &&
                   (dwAttr & SFGAO_FOLDER))) ||
                (piei->uFlags & COF_FIREEVENTONDDEREG))
            {
                //
                // Tell IEDDE that a new browser window is available.
                //
                IEDDE_NewWindow(hwnd);

                //
                // Fire the DdeRegistered event if necessary.
                //
                if (piei->uFlags & COF_FIREEVENTONDDEREG)
                {
                    ASSERT(piei->szDdeRegEvent[0]);
                    FireEventSzW(piei->szDdeRegEvent);
                }
            }
        }

        return (LRESULT)psb;
    }

    case WM_CREATE:
    {
        lResult = psb ? psb->WndProcBS(hwnd, uMsg, wParam, lParam) : DefWindowProc(hwnd, uMsg, wParam, lParam);
 
        // if we have a psb and WndProc() failed (!lResult), then fall through
        // and process the WM_NCDESTROY.
        if (!psb || !lResult)
            break;

        if (psb)
            psb->_OnClose(FALSE);
        // Fall Thru because we need to clean up since the create failed.
        // fall through
    }

    case WM_NCDESTROY:

        //
        // Tell IEDDE that a browser window is no longer available.
        //
        IEDDE_WindowDestroyed(hwnd);

        // WM_NCDESTROY is supposed to be the last message we ever
        // receive, but let's be paranoid just in case...
        SetWindowLong(hwnd, 0, (LONG)NULL);

        // psb may be NULL if we failed to create the window.  An
        // Example includes using Start->Run to open a window to
        // a UNC share that the user doesn't have permissions to see.
        if (psb) 
        {
            psb->PutBaseBrowserData()->_hwnd = NULL;
            if (psb->_dwRegisterWinList)
            {
                if (psb->_fMarshalledDispatch)
                {
                    IShellWindows* psw = WinList_GetShellWindows(TRUE);
                    if (psw)
                    {
                        psw->Revoke(psb->_dwRegisterWinList);
                        psw->Release();
                    }
                }
                else
                {
                    if (psb->_psw)
                        psb->_psw->Revoke(psb->_dwRegisterWinList);
                } 
            }
            ATOMICRELEASE(psb->_psw);
            psb->_fMarshalledDispatch = 0;
            psb->_dwRegisterWinList = 0;
            ATOMICRELEASE(psb->_punkMsgLoop); // Release the message loop if the browser is going away
            psb->Release();
        }
        
        break;

#ifdef UNIX
    case WM_COPYDATA: 
        return psb ? HandleCopyDataUnix(psb, hwnd, uMsg, wParam, lParam) : DefWindowProc(hwnd, uMsg, wParam, lParam); 
#endif
        break;

    default:
        return psb ? psb->WndProcBS(hwnd, uMsg, wParam, lParam) : DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return lResult;
}


// *** IShellBrowser methods *** (same as IOleInPlaceFrame)

/*----------------------------------------------------------
Purpose: IShellBrowser::InsertMenusSB method

*/
HRESULT CShellBrowser2::InsertMenusSB(HMENU hmenu, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    RIP(IS_VALID_HANDLE(hmenu, MENU));

    if (_hmenuTemplate) 
    {
        HMENU hmenuSrc;
        if (_get_itbLastFocus() == ITB_VIEW && 
            _pbbd->_uActivateState == SVUIA_ACTIVATE_FOCUS ) 
        {
            hmenuSrc = _hmenuTemplate;
        }
        else
        {
            hmenuSrc = _hmenuFull;
        }

        Shell_MergeMenus(hmenu, hmenuSrc, 0, 0, FCIDM_BROWSERLAST, MM_SUBMENUSHAVEIDS);
        lpMenuWidths->width[0] = 1;     // File
        lpMenuWidths->width[2] = 2;     // Edit, View
        lpMenuWidths->width[4] = 1;     // Help
    }

    // Save this away so we can correctly build the menu list object
    _hmenuBrowser = hmenu;

    DEBUG_CODE( _DumpMenus(TEXT("InsertMenusSB"), TRUE); )

    return S_OK;
}


/*----------------------------------------------------------
Purpose: IShellBrowser::SetMenuSB method

*/
HRESULT CShellBrowser2::SetMenuSB(HMENU hmenu, HOLEMENU hmenuRes, HWND hwnd)
{
    RIP(NULL == hmenu || IS_VALID_HANDLE(hmenu, MENU));

    // A NULL hmenu means to reinstate the container's original menu
    if (hmenu) {
        _hmenuCur = hmenu;
    } else {
        if (_fRunningInIexploreExe)
            _hmenuCur = _hmenuPreMerged;
        else
            _hmenuCur = _hmenuTemplate;
    }

    _fDispatchMenuMsgs = FALSE;
    _fForwardMenu = FALSE;

    // Normally _hmenuBrowser is set by the caller of InsertMenusSB.
    // However, with the actual web browser, InsertMenusSB is never called.
    // That means _hmenuBrowse is either NULL or non-NULL but invalid.
    // So in that case assume hmenu is equivalent.  This essentially makes
    // all messages get sent to the frame, which is what we want.

    HMENU hmenuBrowser;

    if (!IsMenu(_hmenuBrowser)) // We're calling IsMenu on purpose
        _hmenuBrowser = NULL;

    if (NULL != _hmenuBrowser)
        hmenuBrowser = _hmenuBrowser;
    else
        hmenuBrowser = hmenu;

    _menulist.Set(hmenu, hmenuBrowser);

    // Was the help menu merged?
    HMENU hmenuHelp = NULL;
    
    if (_pbbd->_pctView)
    {
        VARIANTARG vaOut = {0};

        if (S_OK == _pbbd->_pctView->Exec(&CGID_ShellDocView, SHDVID_QUERYMERGEDHELPMENU, 0, NULL, &vaOut))
        {
            // Yes; remove it from the list so it isn't accidentally
            // forwarded on.

            if (VT_INT_PTR == vaOut.vt)
            {
                hmenuHelp = (HMENU)vaOut.byref;

                ASSERT(IS_VALID_HANDLE(hmenuHelp, MENU));
                _menulist.RemoveMenu(hmenuHelp);
            }
            VariantClearLazy(&vaOut);
            
            vaOut.vt = VT_EMPTY;
            vaOut.byref = NULL;
            
            if (S_OK == _pbbd->_pctView->Exec(&CGID_ShellDocView, SHDVID_QUERYOBJECTSHELPMENU, 0, NULL, &vaOut))
            {
                if (VT_INT_PTR == vaOut.vt)
                {
                    // Add the object's help submenu to the list so it gets forwarded
                    HMENU hmenuObjHelp = (HMENU)vaOut.byref;

                    ASSERT(IS_VALID_HANDLE(hmenuObjHelp, MENU));
                    _menulist.AddMenu(hmenuObjHelp);
                }
                VariantClearLazy(&vaOut);
            }
            
        }
    }
    
    // 80734: Was the Go To menu taken from the View menu and grafted onto the
    // main menu by DocHost?  The menulist won't detect this graft, so we have
    // to check ourselves and make sure it's not marked as belonging to the 
    // docobject.
    //
    // This test is duplicated in CDocObjectHost::_SetMenu

    MENUITEMINFO mii;
    mii.cbSize = SIZEOF(mii);
    mii.fMask = MIIM_SUBMENU;

    if (hmenu && _hmenuBrowser && 
        GetMenuItemInfo(hmenu, FCIDM_MENU_EXPLORE, FALSE, &mii))
    {
        HMENU hmenuGo = mii.hSubMenu;

        if (GetMenuItemInfo(_hmenuBrowser, FCIDM_MENU_EXPLORE, FALSE, &mii) &&
            mii.hSubMenu == hmenuGo && _menulist.IsObjectMenu(hmenuGo))
        {
            _menulist.RemoveMenu(hmenuGo);
        }
    }

    DEBUG_CODE( _hmenuHelp = hmenuHelp; )

    if (!_fKioskMode)
    {
        if (_fShowMenu)
            _SetMenu(_hmenuCur);
        else
            _SetMenu(NULL);
    }

    DEBUG_CODE( _DumpMenus(TEXT("SetMenuSB"), TRUE); );

    return S_OK;
}


/*----------------------------------------------------------
Purpose: Remove menus that are shared with other menus from 
         the given browser menu.

*/
HRESULT CShellBrowser2::RemoveMenusSB(HMENU hmenuShared)
{
    // Generally, there is no need to remove most of the menus because 
    // they were cloned and inserted into this menu.  However, the 
    // Favorites menu is an exception because it is shared with
    // _hmenuFav.

    return S_OK;
}

void CShellBrowser2::_ShowHideProgress()
{
    if (_hwndProgress) {
        
        UINT uShow = SW_SHOW;
        if (SendMessage(_hwndProgress, PBM_GETPOS, 0, 0) == 0)
            uShow = SW_HIDE;
        
        ShowWindow(_hwndProgress, uShow);
        
        TraceMsg(TF_SHDPROGRESS, "CShellBrowser2::ShowHideProgress() uShow = %X", uShow);
    }
}

HRESULT CShellBrowser2::SendControlMsg(UINT id, UINT uMsg, WPARAM wParam,
            LPARAM lParam, LRESULT *pret)
{
    HRESULT hres = SUPERCLASS::SendControlMsg(id, uMsg, wParam, lParam, pret);
    
    if (id == FCW_PROGRESS) {
        if (uMsg == PBM_SETRANGE32 || uMsg == PBM_SETPOS)
            _ShowHideProgress();
        
        if (_ptheater && _ptheater->_hwndProgress)
            SendMessage(_ptheater->_hwndProgress, uMsg, wParam, lParam);
    }
    return hres;
}

HRESULT CShellBrowser2::_QIExplorerBand(REFIID riid, LPVOID* ppvObj)
{
    HRESULT hres = E_FAIL;
    IDeskBar* pdbBar = NULL;
    IUnknown* punkBS;
    IBandSite* pbs;
    IDeskBand* pdbBand;

    if (SUCCEEDED(FindToolbar(INFOBAR_TBNAME, IID_IDeskBar, (LPVOID*)&pdbBar)) && pdbBar)
    {
        if (SUCCEEDED(pdbBar->GetClient((IUnknown**)&punkBS)))
        {
            if (SUCCEEDED(punkBS->QueryInterface(IID_IBandSite, (LPVOID*)&pbs)))
            {
                if (pdbBand = _FindBandByClsidBS(pbs, &CLSID_ExplorerBand))
                {
                    hres = pdbBand->QueryInterface(riid, ppvObj);
                    pdbBand->Release();
                }
                pbs->Release();
            }
            punkBS->Release();
        }
        pdbBar->Release();
    }
    return hres;
}

HRESULT CShellBrowser2::GetControlWindow(UINT id, HWND * lphwnd)
{
    // the defaults
    HRESULT hres = E_FAIL;
    *lphwnd = NULL;

    switch (id)
    {
    case FCW_INTERNETBAR:
        if (_GetITBar() && _GetToolbarItem(ITB_ITBAR)->fShow)
            hres = _GetITBar()->GetWindow(lphwnd);
        break;
        
    case FCW_TOOLBAR:
        *lphwnd = _hwndDummyTB;
        break;

    case FCW_STATUS:
        *lphwnd = _hwndStatus;
        break;
        
    case FCW_PROGRESS:
        if (!_hwndProgress && _hwndStatus) {
            _hwndProgress = CreateWindowEx(0, PROGRESS_CLASS, NULL,
                                           WS_CHILD | WS_CLIPSIBLINGS | PBS_SMOOTH,
                                           0, 0, 1, 1,
                                           _hwndStatus, (HMENU)1,
                                           HINST_THISDLL, NULL);

            // we yank off this bit because we REALLY don't want it because
            // the status bar already draws this for us when we specify rects
            //
            // but the progress bar forces this bit on during creation
            if (_hwndProgress)
                SHSetWindowBits(_hwndProgress, GWL_EXSTYLE, WS_EX_STATICEDGE, 0);
        }
        *lphwnd = _hwndProgress;
        break;

    case FCW_TREE:
        {
            BOOL fExplorerBandVisible;
            if (SUCCEEDED(IsControlWindowShown(FCW_TREE, &fExplorerBandVisible)) && fExplorerBandVisible)
            {
                IOleWindow* pow;
                if (SUCCEEDED(_QIExplorerBand(IID_IOleWindow, (void**)&pow)))
                {
                    pow->GetWindow(lphwnd);
                    pow->Release();
                }
            }
        }
        break;
    }

    if (*lphwnd) {
        hres = S_OK;
    }
    return hres;
}


//==========================================================================
//
//==========================================================================
HRESULT CShellBrowser2::SetToolbarItems(LPTBBUTTON pViewButtons, UINT nButtons,
            UINT uFlags)
{
    LPTBBUTTON pStart= NULL, pbtn= NULL;
    int nFirstDiff = 0, nTotalButtons = 0;
    BOOL bVisible = FALSE;

    if (uFlags & FCT_CONFIGABLE)
    {
        return NOERROR;
    }

    // Allocate buffer for the default buttons plus the ones passed in
    //
    pStart = (LPTBBUTTON)LocalAlloc(LPTR, nButtons * SIZEOF(TBBUTTON));
    if (!pStart)
        return NOERROR;

    pbtn = pStart;
    nTotalButtons = 0;

    if (pViewButtons)
    {
        int i;
        for (i = nButtons - 1; i >= 0; --i)
        {
            // copy in the callers buttons
            //
            pbtn[i] = pViewButtons[i];
            // make sure this is properly set to -1.
            // in win95, we had no strings so extensions couldn't set it, but some didn't initialize to -1
            if ((!pbtn[i].iString || (pbtn[i].iString <= (MAX_TB_BUTTONS + NUMBER_SHELLGLYPHS - 1))))
            {
                // We should not set our own shell iString to -1
                ASSERT(pbtn[i].iString != pbtn[i].iBitmap);
                // comment about Hummingbird passing 0xc always
                COMPILETIME_ASSERT(MAX_TB_BUTTONS + NUMBER_SHELLGLYPHS >= 0xc);
                pbtn[i].iString = -1;
            }
        }

        pbtn += nButtons;
        nTotalButtons += nButtons;
    }

    
    if (_pxtb)
    {
        // for right now, disable customize for all old style views
        DWORD dwFlags = VBF_TOOLS | VBF_ADDRESS | VBF_BRAND | VBF_NOCUSTOMIZE; 
        TCHAR szScratch[32];
    
        if (_nTBTextRows  == -1) {
            if (MLLoadString(IDS_SHELL_TB_TEXTROWS, szScratch, ARRAYSIZE(szScratch)))
                _nTBTextRows   = (UINT)StrToInt(szScratch);
            else    
                _nTBTextRows   = 0;
        }

        if (_nTBTextRows   == 1)
            dwFlags |= VBF_ONELINETEXT;
        else if (_nTBTextRows   == 2)
            dwFlags |= VBF_TWOLINESTEXT;
            
        
        _pxtb->SetCommandTarget((IUnknown *)SAFECAST(this, IOleCommandTarget *), &CGID_ShellBrowser, dwFlags);
        if (_lpPendingButtons)
            LocalFree(_lpPendingButtons );
        _lpPendingButtons = (TBBUTTON*)pStart;
        _nButtonsPending =  nTotalButtons;
    }    
    
    return NOERROR;
}


#ifdef DEBUG
//---------------------------------------------------------------------------
// Copy the exception info so we can get debug info for Raised exceptions
// which don't go through the debugger.
void _CopyExceptionInfo(LPEXCEPTION_POINTERS pep)
{
    PEXCEPTION_RECORD per;

    per = pep->ExceptionRecord;
    TraceMsg(DM_ERROR, "Exception %x at %#08x.", per->ExceptionCode, per->ExceptionAddress);

    if (per->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        // If the first param is 1 then this was a write.
        // If the first param is 0 then this was a read.
        if (per->ExceptionInformation[0])
        {
            TraceMsg(DM_ERROR, "Invalid write to %#08x.", per->ExceptionInformation[1]);
        }
        else
        {
            TraceMsg(DM_ERROR, "Invalid read of %#08x.", per->ExceptionInformation[1]);
        }
    }
}
#else
#define _CopyExceptionInfo(x) TRUE
#endif

#define SVSI_SELFLAGS (SVSI_SELECT|SVSI_FOCUSED|SVSI_DESELECTOTHERS|SVSI_ENSUREVISIBLE)

void CShellBrowser2::_AfterWindowCreated(IETHREADPARAM *piei)
{
    //
    // Let interested people know we are alive
    //
    if (piei->uFlags & COF_SELECT)
    {
        IShellView* psv = _pbbd->_psv ? _pbbd->_psv : _pbbd->_psvPending;
        if (psv)
            psv->SelectItem(piei->pidlSelect, SVSI_SELFLAGS);
    }

#ifdef UNIX
    //
    // Since on Unix we are using browser window to display
    // help instead of HTML Help Check for Help mode and 
    // remove internet decorations and statusbar.
    //
    // Also remove any bars on the browser.
    
    if ( piei->uFlags & COF_HELPMODE )
    {
#ifdef NO_MARSHALLING
        if (piei->fOnIEThread)
        {
#endif 
            v_ShowControl(FCW_INTERNETBAR, SBSC_HIDE);
            v_ShowControl(FCW_STATUS, SBSC_HIDE);
#ifdef NO_MARSHALLING
        } 
        else 
        {
            v_ShowControl(FCW_MENUBAR, SBSC_HIDE);
            v_ShowControl(FCW_ADDRESSBAR, SBSC_HIDE);
            v_ShowControl(FCW_LINKSBAR, SBSC_HIDE);
            v_ShowControl(FCW_TOOLBAND, SBSC_HIDE);
            IUnknown_Exec(_GetITBar(),&CGID_PrivCITCommands, CITIDM_SHOWBRAND, FALSE, NULL, NULL);
        }
#endif 
        _SetBrowserBarState(_idmInfo, NULL, 0);
        _SetBrowserBarState(_idmComm, NULL, 0);
    }
#endif // UNIX

    //
    //  Keep it hidden if this is the first instance which is started
    // with "/automation" flag or this object is being created as the
    // result of our CreateInstance.
    //
    if (!_fAutomation && !piei->piehs)
    {
        if (_fKioskMode)
        {
            // Turn off flag as we need to let the next function set it...
            _fKioskMode = FALSE;

            // Hack -1 implies
            ShowControlWindow((UINT)-1, TRUE);
        }
        
        UINT nCmdShow = piei->nCmdShow;
        BOOL fSetForeground = FALSE;
        BOOL fStartTheater = FALSE;

        // we need to do this setforegroundwindow 
        // because of a bug in user.  if the previous thread didn't have
        // activation, ShowWindow will NOT give us activation even though
        // it's supposed to
        switch (nCmdShow) {
        case SW_SHOWNORMAL:
        case SW_SHOWMAXIMIZED:
        case SW_SHOW:
            fSetForeground = TRUE;
            break;
        }
        
        if (_fUseIEPersistence) 
        {
            fStartTheater = SHRegGetBoolUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Main"),
                TEXT("FullScreen"), FALSE, FALSE);
        }
        
        if (fStartTheater) 
        {
            _TheaterMode(TRUE, FALSE);
            if (fSetForeground)
                nCmdShow = SW_SHOW;
        }

        MSG msg;
        if (!PeekMessage(&msg, _pbbd->_hwnd, WM_CLOSE, WM_CLOSE, PM_NOREMOVE)) 
        {
            ShowWindow(_pbbd->_hwnd, nCmdShow);
            //
            // AT THIS POINT ALL DDE TRANSACTIONS SHOULD SUCCEED. THE BROWSER
            // WINDOW WAS ADDED TO THE DDE WINITEM LIST ON WM_NCCREATE,
            // AUTOMATION WAS REGISTERED AS STARTED ON THE OnCreate,
            // AND THE WINDOW IS NOW VISIBLE AS A PART OF THE SHOWWINDOW.
            // 99% OF ALL DDE STARTUP BUGS ARE CAUSED BY SOMEBODY CHECKING FOR
            // MESSAGES (PEEKMESSAGE, GETMESSAGE, INTERPROC SENDMESSAGE, ETC.)
            // BEFORE THIS POINT.
            //
            // On UNIX it is not common to force the window to the top of Z-order.
            //      
#ifndef UNIX
            if (fSetForeground)
                SetForegroundWindow(_pbbd->_hwnd);
#endif
        } 
        else 
        {
            ASSERT(msg.hwnd == _pbbd->_hwnd);
        }            
    }

    _SetTitle(NULL);

    //
    // Delay register our window now.
    //  Note that we need to do it after SetEvent(piei->piehs->GetHevent()) to
    //  avoid soft dead-lock in OLE, and we need to do it after the
    //  ShowWindow above because this will allow DDE messages to get
    //  sent to us.
    //
    //  RegisterWindow() shouldnt have been called yet, but if it has, we dont want
    //  to change its registration class from here.  zekel 9-SEP-97
    //
    ASSERT(!_fDidRegisterWindow);

    RegisterWindow(FALSE, (piei->uFlags & COF_EXPLORE) ? SWC_EXPLORER : SWC_BROWSER);


    // Delay loading accelerators from v_initmembers
    ASSERT(MLGetHinst());
    HACCEL hacc = LoadAccelerators(MLGetHinst(), MAKEINTRESOURCE(ACCEL_MERGE));
    ASSERT(hacc);
    SetAcceleratorMenu(hacc);

    // Send size so status bar shows
    SendMessage(_pbbd->_hwnd, WM_SIZE, 0, 0);

    // delay doing a bunch of registrations
    // things we don't want our subclass to inherit
    if (v_InitMembers == CShellBrowser2::v_InitMembers) 
    {
        // register to get filesys notifications
        _uFSNotify = RegisterNotify(_pbbd->_hwnd, CWM_FSNOTIFY, NULL, SHELLBROWSER_FSNOTIFY_FLAGS,
                                    SHCNRF_ShellLevel | SHCNRF_InterruptLevel, TRUE);
    }
    
    SignalFileOpen(piei->pidl);
}

//
//  RegisterWindow() should only be called with Unregister if the caller
//  wants to insure that the new ShellWindowClass is used.  this is used
//  by CIEFrameAuto to force the browser window in the 3rdParty winlist.
//
HRESULT CShellBrowser2::RegisterWindow(BOOL fForceReregister, int swc)
{
    if (!_psw) 
        _psw = WinList_GetShellWindows(FALSE);
    
    if (_psw)
    {
        if (fForceReregister && _fDidRegisterWindow)
        {
            _psw->Revoke(_dwRegisterWinList);
            _fDidRegisterWindow = FALSE;
        }

        if (!_fDidRegisterWindow)
        {
            // BUGBUG raymondc
            // HandleToLong should really be HANDLE_PTR or something - Browser folks need to fix the IDL
            _psw->Register(NULL, HandleToLong(_pbbd->_hwnd), swc, &_dwRegisterWinList);
            _fDidRegisterWindow = TRUE;
            _swcRegistered = swc;

            UpdateWindowList();
        }
        return S_OK;
    }
    return E_FAIL;
}

LRESULT CALLBACK IEFrameWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void InitializeExplorerClass()
{
    static BOOL fInited = FALSE;

    if (!fInited) 
    {
        ENTERCRITICAL;
        if (!fInited) {

            WNDCLASS  wc;

            ZeroMemory(&wc, SIZEOF(wc));
            wc.style            = CS_BYTEALIGNWINDOW;
            wc.lpfnWndProc      = IEFrameWndProc;
            wc.cbWndExtra       = SIZEOF(void *);
            wc.hInstance        = HINST_THISDLL;
            wc.hIcon            = LoadIcon(HinstShdocvw(), MAKEINTRESOURCE(ICO_TREEUP));
            wc.hCursor          = LoadCursor(NULL, IDC_SIZEWE);
            wc.hbrBackground    = (HBRUSH) (COLOR_WINDOW + 1);
            wc.lpszClassName    = c_szExploreClass;

            RegisterClass(&wc);

#ifndef UNIX
            wc.hIcon            = LoadIcon(HinstShdocvw(), MAKEINTRESOURCE(IDI_STATE_NORMAL));
#else
            if ( SHGetCurColorRes() < 2 )
                wc.hIcon        = LoadIcon(HinstShdocvw(), MAKEINTRESOURCE(IDI_MONOFRAME));
            else
                wc.hIcon        = LoadIcon(HinstShdocvw(), MAKEINTRESOURCE(IDI_STATE_NORMAL));
#endif
            wc.hCursor          = LoadCursor(NULL, IDC_ARROW);

            wc.lpszClassName    = c_szIExploreClass;
            RegisterClass(&wc);
            
            // shell32 is stuck with this id forever since it came out on win95
#define IDI_FOLDEROPEN          5      // open folder
            wc.hIcon            = LoadIcon(HinstShell32(), MAKEINTRESOURCE(IDI_FOLDEROPEN));
            wc.lpszClassName    = c_szCabinetClass;
            RegisterClass(&wc);

            // this needs to be set at the END..
            // because otherwise a race condition occurs and some guys run throught the
            // outter most check and try to create before we're registered.
            fInited = TRUE;
        }
        LEAVECRITICAL;
    }
}

// compatability: we need to have the right class name so people can find our window
//
LPCTSTR _GetExplorerClassName(UINT uFlags)
{
    if (uFlags & COF_EXPLORE)
        return c_szExploreClass;
    else if (uFlags & COF_IEXPLORE || WhichPlatform() == PLATFORM_BROWSERONLY)
        return c_szIExploreClass;
    else
        return c_szCabinetClass;
}

void TimedDispatchMessage(MSG *pmsg)
{
    DWORD dwTime;
    if (g_dwStopWatchMode & SPMODE_MSGTRACE)
        dwTime = StopWatch_DispatchTime(TRUE, *pmsg, 0);
        
    DispatchMessage(pmsg);
    
    if (g_dwStopWatchMode)
    {
        if (g_dwStopWatchMode & SPMODE_MSGTRACE)
            StopWatch_DispatchTime(FALSE, *pmsg, dwTime);

        if ((g_dwStopWatchMode & SPMODE_SHELL) && (pmsg->message == WM_PAINT))
            StopWatch_TimerHandler(pmsg->hwnd, 1, SWMSG_PAINT, pmsg); // Save tick count for paint msg
    }
}


void DesktopChannel();

void BrowserThreadProc(IETHREADPARAM* piei)
{
    HMENU hmenu;
    HWND hwnd;
    DWORD dwExStyle = WS_EX_WINDOWEDGE;
    LONG cRefMsgLoop;           // the ref count for this thread
    IUnknown *punkMsgLoop;      // the ref object (wraps cRefMsgLoop) for this thread
    IUnknown *punkRefProcess;   // the process ref this thread holds (may be none)
    
#ifdef NO_MARSHALLING
    THREADWINDOWINFO *lpThreadWindowInfo = InitializeThreadInfoStructs();
    if (!lpThreadWindowInfo)
       goto Done;
#endif

    UINT tidCur = GetCurrentThreadId();
    UINT uFlags = piei->uFlags;

    if (g_dwStopWatchMode & (SPMODE_SHELL | SPMODE_BROWSER | SPMODE_JAVA | SPMODE_MSGTRACE))
        StopWatch_MarkFrameStart(piei->uFlags & COF_EXPLORE ? " (explore)" : "");

    punkRefProcess = piei->punkRefProcess;
    piei->punkRefProcess = NULL;        // we took ownership
   
    LPWSTR pszCloseEvent = (piei->uFlags & COF_FIREEVENTONCLOSE) ? StrDupW(piei->szCloseEvent) : NULL;

    // if we're going to do desktop channel stuff, do it and return
#ifdef ENABLE_CHANNELS
    if (piei->fDesktopChannel) 
    {
        if (piei->pSplash)
        {
            piei->pSplash->Dismiss();
            ATOMICRELEASE(piei->pSplash);
        }

        if (piei->uFlags & COF_FIREEVENTONDDEREG) 
        {
            ASSERT(piei->szDdeRegEvent[0]);
            FireEventSzW(piei->szDdeRegEvent);
        }

        DesktopChannel();
        goto Done;
    }
#endif  / ENABLE_CHANNELS
    TraceMsg(TF_SHDTHREAD, "IE_ThreadProc(%x) just started.", tidCur);

    InitializeExplorerClass();

    if (SUCCEEDED(SHCreateThreadRef(&cRefMsgLoop, &punkMsgLoop)))
    {
        if (tidCur == g_tidParking)
        {
            SHSetInstanceExplorer(punkMsgLoop);   // we are process reference
        }
        SHSetThreadRef(punkMsgLoop);
    }

    //
    // APPCOMPAT - apps like WebCD require a non-null menu on the
    // browser.  Thankfully USER won't draw a menuband on an empty hmenu.
    //
    hmenu = CreateMenu();
    dwExStyle |= IS_BIDI_LOCALIZED_SYSTEM() ? dwExStyleRTLMirrorWnd | dwExStyleNoInheritLayout: 0L;

    hwnd = CreateWindowEx(dwExStyle, _GetExplorerClassName(piei->uFlags), NULL, 
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, 
        hmenu, HINST_THISDLL, piei);

    if (punkMsgLoop)
        punkMsgLoop->Release();     // browser (in open state) holds the ref

    if (piei->pSplash)
    {
        piei->pSplash->Dismiss();
        ATOMICRELEASE(piei->pSplash);
    }
    
    if (hwnd)
    {
#ifdef UNIX
        UnixAdjustWindowSize(hwnd, piei);
#endif
        if (g_dwStopWatchMode & SPMODE_SHELL)   // Create the timer to start watching for paint messages
            StopWatch_TimerHandler(hwnd, 0, SWMSG_CREATE, NULL);

        CShellBrowser2* psb = (CShellBrowser2*)GetWindowPtr0(hwnd);
        if (psb)
        {
#ifdef NO_MARSHALLING
            AddFirstBrowserToList( psb );
#endif
            psb->AddRef();
            psb->_AfterWindowCreated(piei);

            SHDestroyIETHREADPARAM(piei);
            piei = NULL;

            TraceMsg(TF_SHDTHREAD, "IE_ThreadProc(%x) about to start the message loop", tidCur);
#ifdef UNIX
            BOOL fNoSignalUI = !!getenv("IE_NO_SIGNAL_UI");
            for ( int nMessagePumpCount = 0;  nMessagePumpCount < 2;  nMessagePumpCount += 2 )
            {
            __try
            {
#endif
            while (1)
            {
                MSG  msg;

                if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
#ifdef DEBUG
                    if (msg.message == mem_thread_message())
                        received_for_thread_memlist((DWORD) msg.wParam, (void *) msg.lParam);
#endif
                    if (g_dwStopWatchMode)
                        StopWatch_CheckMsg(hwnd, msg, uFlags == COF_EXPLORE ? " (explore) " : "");  // Key off of WM_KEYDOWN to start timing
#ifdef NO_MARSHALLING
                    CShellBrowser2 *psbOld = psb;
                    CShellBrowser2 *psb = CheckAndForwardMessage(lpThreadWindowInfo, psbOld, msg);
                    if (!psb) 
                        psb = psbOld;
#endif
                    if (psb->_pbbd->_hwnd && IsWindow(psb->_pbbd->_hwnd))
                    {
                        //
                        // Directly dispatch WM_CLOSE message to distinguish nested
                        // message loop case.
                        //
                        if ((msg.message == WM_CLOSE) && (msg.hwnd == psb->_pbbd->_hwnd)) 
                        {
                            psb->_OnClose(FALSE);
                            continue;
                        }
#ifdef NO_MARSHALLING
                        HWND hwnd = GetActiveWindow();
                        if (!IsNamedWindow(hwnd, TEXT("Internet Explorer_TridentDlgFrame")))
                        {
                            if (S_OK == psb->v_MayTranslateAccelerator(&msg))
                                continue;
                        }
                        else
                        {
                            DWORD dwExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
                            if (dwExStyle & WS_EX_MWMODAL_POPUP)
                            {
                                if (S_OK == psb->v_MayTranslateAccelerator(&msg))
                                    continue;
                            }
                            else
                            {
                                if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST && 
                                    (TranslateModelessAccelerator(&msg, hwnd) == S_OK))
                                    continue;
                            }
                        }
#else
                        if (S_OK == psb->v_MayTranslateAccelerator(&msg))
                            continue;
#endif
                    }

                    TranslateMessage(&msg);
                    TimedDispatchMessage(&msg);
                }
#ifdef NO_MARSHALLING
                else if ((cRefMsgLoop == 0) && (lpThreadWindowInfo->cWindowCount == 0))
#else
                else if (cRefMsgLoop == 0)
#endif
                {
                    TraceMsg(TF_SHDTHREAD, "cRefMsgLoop == 0, done");
                    break;  // exit while (1), no more refs on this thread
                } 
                else 
                {
                    WaitMessage();
                    if (g_dwStopWatchMode & SPMODE_MSGTRACE)
                        StopWatch_SetMsgLastLocation(2);
                }
            }
#ifdef UNIX
            }
            __except( ( fNoSignalUI || nMessagePumpCount > 0 )
                      ? EXCEPTION_CONTINUE_SEARCH 
                      : EXCEPTION_EXECUTE_HANDLER )
            {
                // we will try to display a message box to tell the user
                // that a thread has died...
                //
                int result = MLShellMessageBox(NULL, MAKEINTRESOURCE(IDS_EXCEPTIONMSGSH),
                                               MAKEINTRESOURCE(IDS_TITLE),
                                               MB_OK |
                                               MB_APPLMODAL |
                                               MB_ICONEXCLAMATION |
                                               MB_SETFOREGROUND);
                if ( result != IDNO ) 
                {
                    MwExecuteAtExit(); // why the hell doesn't ExitProcess do this!?!?!?
                    ExitProcess(0);
                }
                nMessagePumpCount -= 2; // -=2 for no limit, -=1 to only show panel once
            }
            __endexcept
            }
#endif

            TraceMsg(TF_SHDTHREAD, "IE_ThreadProc(%x) end of message loop", tidCur);
            psb->Release();
        }
    } 
    else 
    {
        // Unregister any pending that may be there
        WinList_Revoke(piei->dwRegister);
        TraceMsg(TF_WARNING, "IE_ThreadProc CreateWindow failed");
    }
#if defined(ENABLE_CHANNELS) || defined(NO_MARSHALLING)
Done:
#endif
    if (pszCloseEvent) 
    {
        FireEventSzW(pszCloseEvent);
        LocalFree(pszCloseEvent);
    }

    SHDestroyIETHREADPARAM(piei);

    if (punkRefProcess)
        punkRefProcess->Release();

#ifndef UNIX
    // if this was the explorer class, then the onetree main window could
    // have been created on this thread and then this thread needs to live.
    OTAssumeThread();
#endif

#ifdef NO_MARSHALLING
    FreeThreadInfoStructs();
#endif
}

#ifdef DEBUG
// BUGBUG: shdocvw debug only export!
#else
#define TLTransferToThreadMemlist(ptl, id)
#endif

DWORD CALLBACK BrowserProtectedThreadProc(void *pv)
{
    if (g_dwProfileCAP & 0x00000004)
        StartCAP();

    OleInitialize(NULL);

    DebugMemLeak(DML_TYPE_THREAD | DML_BEGIN);

#if !defined(FULL_DEBUG) && (!defined(UNIX) || ( defined(UNIX) && defined(GOLDEN) ))

    EXCEPTION_RECORD exr;
    
    _try
    {
        BrowserThreadProc((IETHREADPARAM*)pv);
    }
    _except((exr = *((GetExceptionInformation())->ExceptionRecord),
            _CopyExceptionInfo(GetExceptionInformation()),
            UnhandledExceptionFilter(GetExceptionInformation())))
    {
        LPCTSTR pszMsg;
        //  we will try to display a message box to tell the user
        // that a thread has died...
        //
        if (GetExceptionCode() == STATUS_NO_MEMORY)
            pszMsg = MAKEINTRESOURCE(IDS_EXCEPTIONNOMEMORY);
        else if (WhichPlatform() == PLATFORM_BROWSERONLY)
            pszMsg =  MAKEINTRESOURCE(IDS_EXCEPTIONMSG);
        else
            pszMsg = MAKEINTRESOURCE(IDS_EXCEPTIONMSGSH);

        MLShellMessageBox(NULL, pszMsg,
                          MAKEINTRESOURCE(IDS_TITLE), MB_ICONEXCLAMATION|MB_SETFOREGROUND);

        if (GetExceptionCode() != STATUS_NO_MEMORY)
            IEWriteErrorLog(&exr);
    }
    __endexcept
#else
    // IEUNIX : This exception handler should only be used in Release 
    // version of the product. We are disabling it for debugging purposes.

    BrowserThreadProc((IETHREADPARAM*)pv);
#endif

    OleUninitialize();

    // See if this helps any
    ShrinkWorkingSet();

    DebugMemLeak(DML_TYPE_THREAD | DML_END);
    return 0;
}


// Check if this IETHREADPARAM/LPITEMIDLIST requires launch in a new process
// and if so launch it and return TRUE.
//
BOOL TryNewProcessIfNeeded(LPCITEMIDLIST pidl)
{
    if (pidl && IsBrowseNewProcessAndExplorer() 
    && IsBrowserFrameOptionsPidlSet(pidl, BFO_PREFER_IEPROCESS))
    {
        TCHAR szURL[MAX_URL_STRING];

        HRESULT hres = ::IEGetDisplayName(pidl, szURL, SHGDN_FORPARSING);
        if (SUCCEEDED(hres))
        {
            hres = IENavigateIEProcess(szURL, FALSE);
            if (SUCCEEDED(hres))
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}


BOOL TryNewProcessIfNeeded(IETHREADPARAM * piei)
{
    BOOL bRet = TryNewProcessIfNeeded(piei->pidl);
    if (bRet)
    {
        SHDestroyIETHREADPARAM(piei);
    }
    return bRet;
}


// NOTE: this is a ThreadProc (shdocvw creates this as a thread)
//
// this takes ownership of piei and will free it
//
BOOL SHOpenFolderWindow(IETHREADPARAM* piei)
{
    BOOL fSuccess = FALSE;

    bool    bForceSameWindow;
    OLECMD  rgCmds[1] = {0};

    _InitAppGlobals();

    CABINETSTATE cabstate;
    GetCabState(&cabstate);

    //
    // If "/select" switch is specified, but we haven't splitted
    // the pidl yet (because the path is passed rather than pidl),
    // split it here.
    //
    if ((piei->uFlags & COF_SELECT) && piei->pidlSelect == NULL) 
    {
        LPITEMIDLIST pidlLast = ILFindLastID(piei->pidl);
        piei->pidlSelect = ILClone(pidlLast);
        pidlLast->mkid.cb = 0;
    }

    if (GetAsyncKeyState(VK_CONTROL) < 0)
        cabstate.fNewWindowMode = !cabstate.fNewWindowMode;

    //
    // Check to see if we can reuse the existing window first
    //
    // 99/03/23 #228369 vtan: Use the following logic.
    // 99/04/21 #326078 vtan: jasonba says that we should respect the preference regardless.
    //
    // 1) Cannot reuse the current window if there is no psbCaller.
    // 2) Always reuse the same window if tree view is present (explorer mode) in
    //      Windows 95 classic view mode or GetUIVersion() < 5 otherwise...
    // 3) Honor preferences for creating new windows regardless of explorer mode.
    //

    rgCmds[0].cmdID = SBCMDID_EXPLORERBAR;
    bForceSameWindow = (piei->uFlags & COF_EXPLORE) &&
        SUCCEEDED(IUnknown_QueryStatus(piei->psbCaller, &CGID_Explorer, ARRAYSIZE(rgCmds), rgCmds, NULL)) && 
        (rgCmds[0].cmdf & OLECMDF_LATCHED);
    if (bForceSameWindow ||
        piei->psbCaller &&
        !(piei->uFlags & COF_CREATENEWWINDOW) &&
        !cabstate.fNewWindowMode)
    {
        if (piei->pidl)
        {
            UINT uFlags = SBSP_SAMEBROWSER;
            
            if (piei->uFlags & COF_EXPLORE)
                uFlags |= SBSP_EXPLOREMODE;
            else 
                uFlags |= SBSP_OPENMODE;

            HRESULT hres = piei->psbCaller->BrowseObject(piei->pidl, uFlags);
            
            if (SUCCEEDED(hres))
                goto ReusedWindow;
        }
    }

    //
    // Don't look for an existing window if we are opening an explorer.
    //
    if (!((piei->uFlags & COF_EXPLORE) || (piei->uFlags & COF_NOFINDWINDOW))) 
    {
        HWND hwnd;
        IWebBrowserApp* pwba;
        HRESULT hres = WinList_FindFolderWindow(piei->pidl, NULL, &hwnd, &pwba);
        if (hres == S_OK)
        {
            SetForegroundWindow(hwnd);

            //
            //  IE30COMPAT - we need to refresh when we hunt and pick up these windows - zekel 31-JUL-97
            //  this will happen if we just do a navigate to ourself.  this is the
            //  same kind of behavior we see when we do a shellexec of an URL.
            //  
            //  we dont use the pwb from FindFolderWindow because it turns out that RPC will
            //  fail the QI for it when we are being called from another process.  this
            //  occurs when IExplore.exe Sends the WM_COPYDATA to the desktop to navigate
            //  a window to an URL that is already there.  so instead of using the pwb
            //  we do a CDDEAuto_Navigate().  which actually goes and uses pwb itself.
            //
            TCHAR szUrl[MAX_URL_STRING];
            if (SUCCEEDED(IEGetNameAndFlags(piei->pidl, SHGDN_FORPARSING, szUrl, SIZECHARS(szUrl), NULL)))
            {
                ASSERT(pwba); // WinList_FindFolerWindow should not return S_OK if this fails
                BSTR bstrUrl = SysAllocString(szUrl);
                if (bstrUrl)
                {
                    pwba->Navigate(bstrUrl, NULL, NULL, NULL, NULL);
                    SysFreeString(bstrUrl);
                }   
            }
            ATOMICRELEASE(pwba);

            if (IsIconic(hwnd))
                ShowWindow(hwnd, SW_RESTORE);
            if (piei->nCmdShow)
                ShowWindow(hwnd, piei->nCmdShow);
            goto ReusedWindow;
        }
        else if (hres == E_PENDING)
            goto ReusedWindow;        // Assume it will come up sooner or later
    }

    // Okay, we're opening up a new window, let's honor
    // the BrowseNewProcess flag, even though we lose
    // other info (COF_EXPLORE, etc).
    //
    if (TryNewProcessIfNeeded(piei))
        return TRUE;

    if (((piei->uFlags & (COF_INPROC | COF_IEXPLORE)) == (COF_INPROC | COF_IEXPLORE)) &&
        g_tidParking == 0) 
    {
        // we're starting from iexplore.exe 
        g_tidParking = GetCurrentThreadId();
    }
    
    if (piei->pidl && IsURLChild(piei->pidl, TRUE))
        piei->uFlags |= COF_IEXPLORE;

    ASSERT(piei->punkRefProcess == NULL);
    SHGetInstanceExplorer(&piei->punkRefProcess);     // pick up the process ref (if any)

    if (piei->uFlags & COF_INPROC)
    {
        BrowserProtectedThreadProc(piei);
        fSuccess = TRUE;
    } 
    else 
    {
#ifndef NO_MARSHALLING

        DWORD idThread;
        HANDLE hThread = CreateThread(NULL, 0, BrowserProtectedThreadProc, piei, CREATE_SUSPENDED, &idThread);
        if (hThread) 
        {
            remove_from_memlist(piei);

            WinList_RegisterPending(idThread, piei->pidl, NULL, &piei->dwRegister);

            ResumeThread(hThread);
            CloseHandle(hThread);
            fSuccess = TRUE;
        } 
        else 
        {
            SHDestroyIETHREADPARAM(piei);
        }
#else
        IEFrameNewWindowSameThread(piei);
#endif
    }
    return fSuccess;
    
ReusedWindow:
    SHDestroyIETHREADPARAM(piei);
    return TRUE;
}


//
// Notes: pidlNew will be freed
//
STDAPI SHOpenNewFrame(LPITEMIDLIST pidlNew, ITravelLog *ptl, DWORD dwBrowserIndex, UINT uFlags)
{
    HRESULT hres;

    IETHREADPARAM* piei = SHCreateIETHREADPARAM(NULL, SW_SHOWNORMAL, ptl, NULL);
    if (piei) 
    {
        if (ptl)
            piei->dwBrowserIndex = dwBrowserIndex;

        if (pidlNew)
        {
            piei->pidl = pidlNew;
            pidlNew = NULL;
        }

        piei->uFlags = uFlags;

        if (!TryNewProcessIfNeeded(piei))
        {
#ifndef NO_MARSHALLING
            ASSERT(piei->punkRefProcess == NULL);
            SHGetInstanceExplorer(&piei->punkRefProcess);     // pick up the process ref (if any)

            DWORD idThread;
            HANDLE hThread = CreateThread(NULL, 0, BrowserProtectedThreadProc, piei, 0, &idThread);
            if (hThread)
            {
                remove_from_memlist(piei);
                //  this handles removing this from the debug memory list of
                //  the opening thread.
                if (piei->ptl)
                {
                    TLTransferToThreadMemlist(piei->ptl, idThread);
                }
                CloseHandle(hThread);
                hres = S_OK;
            } 
            else 
            {
                SHDestroyIETHREADPARAM(piei);
                hres = E_FAIL;
            }
#else
            IEFrameNewWindowSameThread(piei);
#endif
        }
    } 
    else 
        hres = E_OUTOFMEMORY;

    ILFree(pidlNew);

    return hres;
}


HRESULT CShellBrowser2::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, 
                                   OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    HRESULT hres = SUPERCLASS::QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
    
    if (pguidCmdGroup == NULL) 
    {
        //
        //  If the curretly focused toolbar suppors the IOleCommandTarget
        // ask it the status of clipboard commands.
        // BUGBUG actually we should ask *both* ourselves (the active target)
        // and then the view (the same way our IOCT::Exec does...)
        //
        if (_HasToolbarFocus())
        {
            LPTOOLBARITEM ptbi;
            ptbi = _GetToolbarItem(_get_itbLastFocus());
            if (ptbi && ptbi->ptbar)
            {
                IOleCommandTarget* pcmd;
                HRESULT hresT = ptbi->ptbar->QueryInterface(
                                    IID_IOleCommandTarget, (void**)&pcmd);
                if (SUCCEEDED(hresT)) 
                {
                    for (ULONG i = 0; i < cCmds; i++) 
                    {
                        switch (rgCmds[i].cmdID)
                        {
                        case OLECMDID_CUT:
                        case OLECMDID_COPY:
                        case OLECMDID_PASTE:
                            pcmd->QueryStatus(pguidCmdGroup, 1, &rgCmds[i], pcmdtext);
                            break;
                        }
                    }
                    pcmd->Release();
                }
            }
        }
    }
    else if (IsEqualGUID(CGID_ShellBrowser, *pguidCmdGroup)) 
    {
        if (pcmdtext) {
            ASSERT(cCmds == 1);
            switch (pcmdtext->cmdtextf)
            {
            case OLECMDTEXTF_NAME:
            {
                TOOLTIPTEXTA ttt = { NULL };
                CHAR *pszBuffer = ttt.szText;
                CHAR szTemp[MAX_TOOLTIP_STRING];

                ttt.hdr.code = TTN_NEEDTEXTA;
                ttt.hdr.idFrom = rgCmds[0].cmdID;
                ttt.lpszText = ttt.szText;
                OnNotify((LPNMHDR)&ttt);

                if (ttt.hinst)
                {
                    LoadStringA(ttt.hinst, (UINT) PtrToUlong( ttt.lpszText ), szTemp, ARRAYSIZE(szTemp));
                    pszBuffer = szTemp;
                }
                else if (ttt.lpszText)
                    pszBuffer = ttt.lpszText;

                pcmdtext->cwActual = SHAnsiToUnicode(pszBuffer, pcmdtext->rgwz, pcmdtext->cwBuf);
                pcmdtext->cwActual -= 1;
                hres = S_OK;
                break;
            }

            default:    
                hres = E_FAIL;
                break;
            }        
        } else {
            for (ULONG i = 0 ; i < cCmds ; i++) {

                switch(rgCmds[i].cmdID) {
                case FCIDM_PREVIOUSFOLDER:
                    if (_ShouldAllowNavigateParent())
                        rgCmds[i].cmdf |= OLECMDF_ENABLED;
                    else
                        rgCmds[i].cmdf &= OLECMDF_ENABLED;

                    hres = S_OK;
                    break;
                }
            }
        }
    }
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        // BUGBUG: should fill pcmdtext as well

        for (ULONG i = 0 ; i < cCmds ; i++)
        {
            switch (rgCmds[i].cmdID)
            {
            case SBCMDID_SHOWCONTROL:
            case SBCMDID_DOFAVORITESMENU:
            case SBCMDID_ACTIVEOBJECTMENUS:            
            case SBCMDID_SETMERGEDWEBMENU:
                rgCmds[i].cmdf = OLECMDF_ENABLED;   // support these unconditionally
                break;

            case SBCMDID_DOMAILMENU:
                rgCmds[i].cmdf = SHIsRegisteredClient(NEWS_DEF_KEY) || SHIsRegisteredClient(MAIL_DEF_KEY) ? OLECMDF_ENABLED : 0;
                //If go menu is restricted, disable mail menu on toolbar also
                if(SHRestricted2(REST_GoMenu, NULL, 0))
                    rgCmds[i].cmdf = 0;
                break;

            case SBCMDID_SEARCHBAR:
            case SBCMDID_FAVORITESBAR:
            case SBCMDID_HISTORYBAR:
#ifdef ENABLE_CHANNELPANE
            case SBCMDID_CHANNELSBAR:
#endif
            case SBCMDID_EXPLORERBAR:
            case SBCMDID_DISCUSSIONBAND:
                {
                UINT idm;

                rgCmds[i].cmdf |= OLECMDF_SUPPORTED|OLECMDF_ENABLED;
                
                switch (rgCmds[i].cmdID) {
                case SBCMDID_DISCUSSIONBAND: 
                    {
                        // Perf: Avoid calling _InfoCLSIDToIdm unless we have loaded
                        // the band info already because it is very expensive!
                        if (_pbsmInfo && FCIDM_VBBNOHORIZONTALBAR != _idmComm)
                        {
                            idm = _InfoCLSIDToIdm(&CLSID_DiscussionBand);
                            if (idm == -1)
                            {
                                // The discussion band is not registered
                                ClearFlag(rgCmds[i].cmdf, OLECMDF_SUPPORTED|OLECMDF_ENABLED);
                            }
                            else if (idm == _idmComm)
                            {
                                rgCmds[i].cmdf |= OLECMDF_LATCHED;
                            }
                        }
                        else
                        {
                            //
                            // Since the band info has not been loaded or idmComm is FCIDM_VBBNONE,
                            // we know that the discussion band is not up and is not latched.  So
                            // we can check the registry instead and avoid defer the cost of
                            // initializing _pbsmInfo
                            //
                            // See if the discussions band is registered for the CATID_CommBand
                            HKEY hkey = NULL;
                            static BOOL fDiscussionBand = -1;

                            // We get called a lot, so only read the registry once.
                            if (-1 == fDiscussionBand)
                            {
                                if (RegOpenKeyEx(HKEY_CLASSES_ROOT, c_szDiscussionBandReg, NULL, KEY_READ, &hkey) == ERROR_SUCCESS)
                                {
                                    // We found the discussions band
                                    fDiscussionBand = 1;
                                    RegCloseKey(hkey);
                                }
                                else
                                {
                                    fDiscussionBand = 0;
                                }
                            }

                            if (!fDiscussionBand)
                            {
                                // The discussions band is not registered
                                ClearFlag(rgCmds[i].cmdf, OLECMDF_SUPPORTED|OLECMDF_ENABLED);
                            }
                        }
                        break;
                    }
                case SBCMDID_SEARCHBAR:      idm=FCIDM_VBBSEARCHBAND     ; break;
                case SBCMDID_FAVORITESBAR:  
                    idm=FCIDM_VBBFAVORITESBAND;
                    if (SHRestricted2(REST_NoFavorites, NULL, 0))
                        ClearFlag(rgCmds[i].cmdf, OLECMDF_ENABLED);
                    break;
                case SBCMDID_HISTORYBAR:     idm=FCIDM_VBBHISTORYBAND    ; break;
                case SBCMDID_EXPLORERBAR:    idm=FCIDM_VBBEXPLORERBAND   ; break;
                
#ifdef ENABLE_CHANNELPANE
                case SBCMDID_CHANNELSBAR:    
                    idm = FCIDM_VBBCHANNELSBAND;
                    // If the NoChannelUI restriction is in place, disable the channel button
                    if (SHRestricted2(REST_NoChannelUI, NULL, 0))
                        ClearFlag(rgCmds[i].cmdf, OLECMDF_ENABLED);
                    break;
#endif
                default:
                    ASSERT(FALSE);
                    return E_FAIL;
                }

                if (idm == _idmInfo)
                    rgCmds[i].cmdf |= OLECMDF_LATCHED;

                break;
                }
            }
        }
        hres = S_OK;
    }
        
    return hres;
}

// BUGBUG (980710 adp) should clean up to do consistent routing.  this
// ad-hoc per-nCmdID stuff is too error-prone.
HRESULT CShellBrowser2::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, 
                            VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (pguidCmdGroup == NULL) 
    {
        switch (nCmdID) 
        {
        case OLECMDID_SETTITLE:
            // NT #282632: This bug is caused when the current IShellView is a web page and
            //    the pending view is a shell folder w/Web View that has certain timing charataristics.
            //    MSHTML from the Web View will fire OLECMDID_SETTITLE with the URL of the
            //    web view template.  The problem is that there is a bug in IE4 SI's shell32
            //    that let that message go to the browser (here), even though the view wasn't
            //    active.  Since we don't know who it came from, we forward it down to the current
            //    view who squirls away the title.  This title is then given in the Back toolbar
            //    button drop down history.  This is hack around the IE4 SI shell32 bug is to kill
            //    that message. -BryanSt
            if (GetUIVersion() == 4)
                return S_OK;    // Kill the message
            break;  // continue.

        // Clipboard commands and common operations will be dispatched 
        // to the currently focused toolbar.
        case OLECMDID_CUT:
        case OLECMDID_COPY:
        case OLECMDID_PASTE:
        case OLECMDID_DELETE:
        case OLECMDID_PROPERTIES:
            if (_HasToolbarFocus()) {
                LPTOOLBARITEM ptbi;
                if (ptbi = _GetToolbarItem(_get_itbLastFocus()))
                {
                    HRESULT hres = IUnknown_Exec(ptbi->ptbar, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
                    if (SUCCEEDED(hres))
                        return hres;
                }
            }
            break;  // give the view a chance
        }
    }
    else if (IsEqualGUID(CGID_DefView, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
            case DVID_RESETDEFAULT:

//  99/02/09 #226140 vtan: Exec command issued from
//  CFolderOptionsPsx::ResetDefFolderSettings()
//  when user clicks "Reset All Folders" in folder
//  options "View" tab. Pass this thru to DefView.

                ASSERTMSG(nCmdexecopt == OLECMDEXECOPT_DODEFAULT, "nCmdexecopt must be OLECMDEXECOPT_DODEFAULT");
                ASSERTMSG(pvarargIn == NULL, "pvarargIn must be NULL");
                ASSERTMSG(pvarargOut == NULL, "pvarargOut must be NULL");
                IUnknown_Exec(_pbbd->_psv, &CGID_DefView, DVID_RESETDEFAULT, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
                return(S_OK);
                break;
            default:
                break;
        }
    }
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup)) 
    {
        switch (nCmdID) 
        {
#ifdef UNIX
        case SBCMDID_MSGBAND:
            if (_pbbd->_hwnd)
            {
                pvarargOut->vt = VT_I4;
                pvarargOut->lVal = (LONG)_pbbd->_hwnd;
                return S_OK;
            }
            return S_FALSE;
#endif
        case SBCMDID_MAYSAVEVIEWSTATE:
            return _fDontSaveViewOptions ? S_FALSE : S_OK;  // May need to save out earlier...
            
        case SBCMDID_CANCELNAVIGATION:
            //
            // Special code to close the window if the very first navigation caused
            // an asynchronous download (e.g. from athena).
            //
            if (!_pbbd->_pidlCur && pvarargIn && pvarargIn->vt == VT_I4 && pvarargIn->lVal == FALSE) 
            {
                _CancelPendingNavigationAsync();
                PostMessage(_pbbd->_hwnd, WM_CLOSE, 0, 0);
                return S_OK;
            }
            break; // fall through

        case SBCMDID_MIXEDZONE:
            _UpdateZonesPane(pvarargIn);
            break;

        case SBCMDID_ISIEMODEBROWSER:
            return v_IsIEModeBrowser() ? S_OK : S_FALSE;

        case SBCMDID_STARTEDFORINTERNET:
            return _fInternetStart ? S_OK : S_FALSE;

        case SBCMDID_ISBROWSERACTIVE:
            return _fActivated ? S_OK : S_FALSE;

        case SBCMDID_SUGGESTSAVEWINPOS:
            if (_ShouldSaveWindowPlacement())
            {
                StorePlacementOfWindow(_pbbd->_hwnd);
                return S_OK;
            }
            return S_FALSE;

        case SBCMDID_ONVIEWMOVETOTOP:
            if (_ptheater)
            {
                return _ptheater->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            }
            return S_FALSE;
        }    
    } 
    else if (IsEqualGUID(CGID_MenuBandHandler, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case MBHANDCID_PIDLSELECT:
            {
                LPCITEMIDLIST pidl = VariantToConstIDList(pvarargIn);
                if (pidl && !ILIsEmpty(pidl))
                {
                    if (_pidlMenuSelect)
                        KillTimer(_pbbd->_hwnd, SHBTIMER_MENUSELECT);
                    
                    // Opening the favorites' shortcuts can be slow.  So set a
                    // timer so we don't open needlessly as the mouse runs over
                    // the menu quickly.
                    if (Pidl_Set(&_pidlMenuSelect, pidl))
                    {
                        if (!SetTimer(_pbbd->_hwnd, SHBTIMER_MENUSELECT, MENUSELECT_TIME, NULL))
                            Pidl_Set(&_pidlMenuSelect, NULL);
                    }
                }
            }
            return S_OK;
        }
    }
    else if (IsEqualGUID(CGID_MenuBand, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case MBANDCID_EXITMENU:
            // we're done with the menu band (favorites)
            // kill the timer
            // this is to fix bug #61917 where the status bar would get stack
            // in the simple mode (actually the timer would go off after we 
            // navigated to the page with the proper status bar mode, but then
            // we would call _DisplayFavoriteStatus that would put it back in
            // the simple mode)
            if (_pidlMenuSelect)
                KillTimer(_pbbd->_hwnd, SHBTIMER_MENUSELECT);
            
            return S_OK;
        }
    }
    else if (IsEqualGUID(CGID_FilterObject, *pguidCmdGroup))
    {
        HRESULT hres = E_INVALIDARG;

        switch (nCmdID)
        {
        case PHID_FilterOutPidl:
            {
                LPCITEMIDLIST pidl = VariantToConstIDList(pvarargIn);
                if (pidl)
                {
                    // We are filtering out everything except folders, shortcuts, and 
                    // internet shortcuts
                    DWORD dwAttribs = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_LINK;

                    // include everthing by default
                    VariantClearLazy(pvarargOut); 
                    pvarargOut->vt = VT_BOOL;
                    pvarargOut->boolVal = VARIANT_FALSE;    

                    IEGetAttributesOf(pidl, &dwAttribs);

                    // include all non file system objects, folders and links
                    // (non file system things like the Channel folders contents)
                    if (!(dwAttribs & (SFGAO_FOLDER | SFGAO_LINK)) &&
                         (dwAttribs & SFGAO_FILESYSTEM))
                        pvarargOut->boolVal = VARIANT_TRUE; // don't include

                    hres = S_OK;
                }
            }
            break;

        default:
            TraceMsg(TF_WARNING, "csb.e: Received unknown CGID_FilterObject cmdid (%d)", nCmdID);
            break;
        }

        return hres;
    } 
    else if (IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup)) 
    {
        switch (nCmdID) 
        {
        // we reflect AMBIENTPROPCHANGE down because this is how iedisp notifies dochost
        // that an ambient property has changed. we don't need to reflect this down in
        // cwebbrowsersb because only the top-level iwebbrowser2 is allowed to change props
        case SHDVID_AMBIENTPROPCHANGE:
        case SHDVID_PRINTFRAME:
        case SHDVID_MIMECSETMENUOPEN:
        case SHDVID_FONTMENUOPEN:
            if (pvarargIn)
            {
                if ((VT_I4 == pvarargIn->vt) && (DISPID_AMBIENT_OFFLINEIFNOTCONNECTED == pvarargIn->lVal))
                {
                    VARIANT_BOOL fIsOffline;
                    if (_pbbd->_pautoWB2)
                    {
                        _pbbd->_pautoWB2->get_Offline(&fIsOffline);
                        if (fIsOffline)
                        {
                            // this top level frame just went Offline 
                            //  - so decr net session count
                            _DecrNetSessionCount();
                        }
                        else
                        {
                            // this top level frame went online. 
                            _IncrNetSessionCount();
                        }     
                    }
                }
            }
            break;
        }
    }

    HRESULT hres = SUPERCLASS::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);

    if (pguidCmdGroup == NULL)
    {
DefaultCommandGroup:

        switch (nCmdID) 
        {
        case OLECMDID_REFRESH:
            // FolderOptions.Advanced refreshes all browser windows
            _UpdateRegFlags();
            _SetTitle(NULL); // maybe "full path in title bar" changed

            v_ShowHideChildWindows(FALSE);
            
            // pass this to the browserbar
            IDeskBar* pdb;
            FindToolbar(INFOBAR_TBNAME, IID_IDeskBar, (void **)&pdb);
            if (pdb) 
            {
                IUnknown_Exec(pdb, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
                pdb->Release();
            }            
            hres = S_OK;
            break;
        }
    }
    else if (IsEqualGUID(CGID_ShellBrowser, *pguidCmdGroup))
    {
        // toolbar buttons we put there ourselves

        hres = S_OK;    // assume accepted

        switch (nCmdID)
        {
        case FCIDM_PREVIOUSFOLDER:
            v_ParentFolder();
            break;

        case FCIDM_CONNECT:
            DoNetConnect(_pbbd->_hwnd);
            break;
    
        case FCIDM_DISCONNECT:
            DoNetDisconnect(_pbbd->_hwnd);
            break;

        case FCIDM_GETSTATUSBAR:
            if (pvarargOut->vt == VT_I4)
                pvarargOut->lVal = _fStatusBar;
            break;

        case FCIDM_SETSTATUSBAR:
            if (pvarargIn->vt == VT_I4) 
            {
                _fStatusBar = pvarargIn->lVal;
                v_ShowHideChildWindows(FALSE);
            }
            break;

        case FCIDM_PERSISTTOOLBAR:
            _SaveITbarLayout();
            break;

        default:
            // pass off the nCmdID to the view for processing / translation.
            DFVCMDDATA cd;

            cd.pva = pvarargIn;
            cd.hwnd = _pbbd->_hwnd;
            cd.nCmdIDTranslated = 0;
            SendMessage(_pbbd->_hwndView, WM_COMMAND, nCmdID, (LONG_PTR)&cd);

            if (cd.nCmdIDTranslated)
            {
                // We sent the private nCmdID to the view.  The view did not
                // process it (probably because it didn't have the focus),
                // but instead translated it into a standard OLECMDID for
                // further processing by the toolbar with the focus.

                pguidCmdGroup = NULL;
                nCmdID = cd.nCmdIDTranslated;
                hres = OLECMDERR_E_NOTSUPPORTED;
                goto DefaultCommandGroup;
            }
            break;
        }
    }
    else if (IsEqualGUID(IID_IExplorerToolbar, *pguidCmdGroup))
    {
        // these are commands to intercept
        if (_HasToolbarFocus() && _get_itbLastFocus() != ITB_ITBAR)
        {
            int idCmd = -1;

            // certain shell commands we should pick off
            switch (nCmdID)
            {
            case OLECMDID_DELETE:
            case SFVIDM_FILE_DELETE:
                idCmd = FCIDM_DELETE;
                break;

            case OLECMDID_PROPERTIES:
            case SFVIDM_FILE_PROPERTIES:
                idCmd = FCIDM_PROPERTIES;
                break;

            case OLECMDID_CUT:
            case SFVIDM_EDIT_CUT:
                idCmd = FCIDM_MOVE;
                break;

            case OLECMDID_COPY:
            case SFVIDM_EDIT_COPY:
                idCmd = FCIDM_COPY;
                break;

            case OLECMDID_PASTE:
            case SFVIDM_EDIT_PASTE:
                idCmd = FCIDM_PASTE;
                break;
            }

            if (idCmd != -1)
            {
                OnCommand(GET_WM_COMMAND_MPS(idCmd, 0, NULL));
                return S_OK;
            }
        }
    } 
    else if (IsEqualGUID(CGID_PrivCITCommands, *pguidCmdGroup)) 
    {
        return IUnknown_Exec(_GetITBar(), pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
    }
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        hres = S_OK;        // assume we will get it

        switch(nCmdID) 
        {
        case SBCMDID_SETMERGEDWEBMENU:
            if (!_hmenuPreMerged)
                _hmenuPreMerged = _MenuTemplate(MENU_PREMERGED, FALSE);

            SetMenuSB(_hmenuPreMerged, NULL, NULL);
            if (pvarargOut) {
                pvarargOut->vt = VT_INT_PTR;
                pvarargOut->byref = _hmenuPreMerged;
            }
            break;

        case SBCMDID_ACTIVEOBJECTMENUS:
            _fDispatchMenuMsgs = TRUE;
            break;

        case SBCMDID_SENDPAGE:
        case SBCMDID_SENDSHORTCUT:
            _SendCurrentPage(nCmdID == SBCMDID_SENDPAGE ? FORCE_COPY : FORCE_LINK);
            break;

        case SBCMDID_SEARCHBAR:
        case SBCMDID_FAVORITESBAR:
        case SBCMDID_HISTORYBAR:
#ifdef ENABLE_CHANNELPANE
        case SBCMDID_CHANNELSBAR:
#endif
        case SBCMDID_EXPLORERBAR:
        case SBCMDID_DISCUSSIONBAND:
            {
            UINT idm;
            int i;
            LPCITEMIDLIST pidl;

            switch (nCmdID) {
            case SBCMDID_SEARCHBAR:      idm = FCIDM_VBBSEARCHBAND      ; break;
            case SBCMDID_FAVORITESBAR:   idm = FCIDM_VBBFAVORITESBAND   ; break;
            case SBCMDID_HISTORYBAR:     idm = FCIDM_VBBHISTORYBAND     ; break;
#ifdef ENABLE_CHANNELPANE
            case SBCMDID_CHANNELSBAR:    idm = FCIDM_VBBCHANNELSBAND    ; break;
#endif
            case SBCMDID_EXPLORERBAR:    idm = FCIDM_VBBEXPLORERBAND    ; break;

            // The discussion band maps to one of the dynamic bands
            case SBCMDID_DISCUSSIONBAND: idm = _InfoCLSIDToIdm(&CLSID_DiscussionBand)  ; break;
            default:                     idm = -1; break;
            }

            if (idm != -1)
            {
                // default is toggle (-1)
                i = (pvarargIn && EVAL(pvarargIn->vt == VT_I4)) ? pvarargIn->lVal : -1;
                pidl = (pvarargOut ? VariantToConstIDList(pvarargOut) : NULL);
                _SetBrowserBarState(idm, NULL, i, pidl);
            }

            hres = S_OK;

            break;
            }

        case SBCMDID_SHOWCONTROL:
        {
            DWORD dwRet;
            int iControl, iCmd;
            
            if (nCmdexecopt == OLECMDEXECOPT_DODEFAULT &&
                pvarargIn &&
                pvarargIn->vt == VT_I4) 
            {
                iControl = (int)(short)LOWORD(pvarargIn->lVal);
                iCmd = (int)(short)HIWORD(pvarargIn->lVal);
            } 
            else 
            {
                iControl = (int)(short)LOWORD(nCmdexecopt);
                iCmd = (HIWORD(nCmdexecopt) ? SBSC_SHOW : SBSC_HIDE);
            }                

            dwRet = v_ShowControl(iControl, iCmd);
            if (dwRet == (DWORD)-1)
                break;

            if (pvarargOut) 
            {
                pvarargOut->vt = VT_I4;
                pvarargOut->lVal = dwRet;
            }
            break;
        }

        case SBCMDID_SETSECURELOCKICON:
            // do something like this...
            {
                //  if this is a SET, then just obey.
                int lock = pvarargIn->lVal;
                TraceMsg(DM_SSL, "SB::Exec() SETSECURELOCKICON lock = %d", lock);

                if (lock >= SECURELOCK_FIRSTSUGGEST)
                {
                    //
                    // if this was ever secure, then the lowest we can be
                    //  suggested to is MIXED.  otherwise we just choose the 
                    //  lowest level of security suggested.
                    //
                    if (lock == SECURELOCK_SUGGEST_UNSECURE && 
                        _pbbd->_eSecureLockIcon != SECURELOCK_SET_UNSECURE)
                        lock = SECURELOCK_SET_MIXED;
                    else
                        lock = min(lock - SECURELOCK_FIRSTSUGGEST, _pbbd->_eSecureLockIcon);
                }

                UpdateSecureLockIcon(lock);
            }
            break;        

        case SBCMDID_DOFAVORITESMENU:
        case SBCMDID_DOMAILMENU:
            {
                HMENU hmenu = NULL;

                if (nCmdID == SBCMDID_DOFAVORITESMENU)
                {
                    HMENU hmenuWnd = NULL;
                   
                    IShellMenu* psm;
                    if (SUCCEEDED(_pmb->QueryInterface(IID_IShellMenu, (void**)&psm)))
                    {
                        psm->GetMenu(&hmenuWnd, NULL, NULL);
                        psm->Release();
                    }
                    
                    if (hmenuWnd)
                    {
                        hmenu = SHGetMenuFromID(hmenuWnd, FCIDM_MENU_FAVORITES);
                    }
                }
                else
                    hmenu = LoadMenuPopup(MENU_MAILNEWS);

                if (hmenu)
                {
                    if (pvarargIn && pvarargIn->vt == VT_I4) 
                    {
                        TrackPopupMenu(hmenu, 0, 
                            (int)(short)LOWORD(pvarargIn->lVal), (int)(short)HIWORD(pvarargIn->lVal), 
                            0, _GetCaptionWindow(), NULL);
                    }
                    if (nCmdID == SBCMDID_DOMAILMENU)
                        DestroyMenu(hmenu);
                }
            }
            break;

        case SBCMDID_SELECTHISTPIDL:
            {
                //  remember most recent hist pidl.  if we have history band visible, tell it
                //  the hist pidl.  if the band is not visible, it is it's responsibility to
                //  query the most recent hist pidl via SBCMDID_GETHISTPIDL
                //  the semantics of the call from UrlStorage is that if we respond S_OK to
                //  to this command, we take ownership of pidl and must ILFree it.
                //  when we call the band, on the other hand, they must ILClone it if they want
                //  to use it outside of the Exec call.
                if (_pidlLastHist) 
                {
                    ILFree(_pidlLastHist);
                    _pidlLastHist = NULL;
                }
                _pidlLastHist = VariantToIDList(pvarargIn); // take ownership

                if (_poctNsc)
                {
                    _poctNsc->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
                }
            }
            break;

        case SBCMDID_GETHISTPIDL:
            if (pvarargOut) {
                VariantClearLazy(pvarargOut);
                if (_pidlLastHist)
                {
                    InitVariantFromIDList(pvarargOut, _pidlLastHist);
                }
            }
            break;

        case SBCMDID_UNREGISTERNSCBAND:
            if (pvarargIn && pvarargIn->vt == VT_UNKNOWN && pvarargIn->punkVal)
            {
                if (_poctNsc && SHIsSameObject(_poctNsc, pvarargIn->punkVal))
                {
                    ATOMICRELEASE(_poctNsc);
                    ATOMICRELEASE(_pcmNsc);
                }
            }
            break;

        case SBCMDID_REGISTERNSCBAND:
            ATOMICRELEASE(_poctNsc);
            ATOMICRELEASE(_pcmNsc);
            if (pvarargIn && pvarargIn->vt == VT_UNKNOWN && pvarargIn->punkVal)
            {
                pvarargIn->punkVal->QueryInterface(IID_IOleCommandTarget, (void **)&_poctNsc);
            }
            break;
                    
        case SBCMDID_TOOLBAREMPTY:
            if (pvarargIn && VT_UNKNOWN == pvarargIn->vt && pvarargIn->punkVal) 
            {
                if (_IsSameToolbar(INFOBAR_TBNAME, pvarargIn->punkVal))
                    _SetBrowserBarState(_idmInfo, NULL, 0);
                else if (_IsSameToolbar(COMMBAR_TBNAME, pvarargIn->punkVal))
                    _SetBrowserBarState(_idmComm, NULL, 0);
                else
                    _HideToolbar(pvarargIn->punkVal);
            }
            break;

        case SBCMDID_GETTEMPLATEMENU:
            if (pvarargOut) 
            {
                pvarargOut->vt = VT_INT_PTR;
                pvarargOut->byref = (LPVOID)_hmenuTemplate;
            }
            break;

        case SBCMDID_GETCURRENTMENU:
            if (pvarargOut) 
            {
                pvarargOut->vt = VT_INT_PTR;
                pvarargOut->byref = (LPVOID)_hmenuCur;
            }
            break;

        default:
            //
            // Note that we should return hres from the super class as-is.
            //
            // hres = OLECMDERR_E_NOTSUPPORTED;
            break;
        }
    } 
    else if (IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup))
    {
        switch(nCmdID) 
        {
        case SHDVID_GETSYSIMAGEINDEX:
            pvarargOut->vt = VT_I4;
            pvarargOut->lVal = _GetIconIndex();
            hres = (pvarargOut->lVal==-1) ? E_FAIL : S_OK;
            break;
            
        case SHDVID_HELP:
            SHHtmlHelpOnDemandWrap(_pbbd->_hwnd, TEXT("iexplore.chm > iedefault"), HH_DISPLAY_TOPIC, 0, ML_CROSSCODEPAGE);
            hres = S_OK;
            break;
            
        case SHDVID_GETBROWSERBAR:
        {
            IBandSite *pbs;

            hres = E_FAIL;
            const CLSID *pclsid = _InfoIdmToCLSID(_idmInfo);

            VariantInit(pvarargOut);

            _GetBrowserBar(IDBAR_VERTICAL, TRUE, &pbs, pclsid);
            if (pbs) 
            {
                IDeskBand *pband = _GetInfoBandBS(pbs,pclsid);
                if (pband) 
                {
                    pvarargOut->vt = VT_UNKNOWN;
                    pvarargOut->punkVal = pband;
                    hres = S_OK;
                }
                pbs->Release();
            }

            break;
        }
        
        case SHDVID_SHOWBROWSERBAR:
            {
                CLSID *pclsid;
                CLSID guid;
                if (pvarargIn->vt == VT_BSTR) 
                {
                    if (SUCCEEDED(IUnknown_Exec(_GetITBar(), &CGID_PrivCITCommands, CITIDM_VIEWEXTERNALBAND_BYCLASSID, nCmdexecopt, pvarargIn, NULL))) 
                    {
                        hres = S_OK;
                        break;
                    }

                    // external (can marshal)
                    if (!GUIDFromString(pvarargIn->bstrVal, &guid))
                        return S_OK; // Invalid CLSID; IE5 returned S_OK so I guess it's ok

                    pclsid = &guid;
                }
                else if (pvarargIn->vt == VT_I4) 
                {
                    // internal (can't marshal)
                    ASSERT(0);  // dead code?  let's make sure...
                    TraceMsg(TF_WARNING, "csb.e: SHDVID_SHOWBROWSERBAR clsid !marshall (!)");
                    pclsid = (CLSID *) (pvarargIn->lVal);
                }
                else 
                {
                    ASSERT(0);
                    break;
                }

                _SetBrowserBarState(-1, pclsid, nCmdexecopt ? 1 : 0);
                hres = S_OK;
                break;
            }

        case SHDVID_ISBROWSERBARVISIBLE:
            // Quickly return false if both bars are hidden
            hres = S_FALSE;
            if (_idmComm != FCIDM_VBBNOHORIZONTALBAR || _idmInfo != FCIDM_VBBNOVERTICALBAR)
            {
                CLSID clsid;

                if (pvarargIn->vt == VT_BSTR)
                {
                    // Get the associated id
                    if (GUIDFromString(pvarargIn->bstrVal, &clsid))
                    {
                        UINT idm = _InfoCLSIDToIdm(&clsid);
                        if (_idmComm == idm || _idmInfo == idm)
                        {
                            // It's visible!
                            hres = S_OK; 
                        }
                    }
                }
            }
            return hres;

        case SHDVID_NAVIGATEBB:
        {
            LPCITEMIDLIST pidl = VariantToConstIDList(pvarargIn);
            if (pidl && !ILIsEmpty(pidl))
            {
                IBandSite *pbs;
                const CLSID *pclsid = _InfoIdmToCLSID(_idmInfo);
                _GetBrowserBar(IDBAR_VERTICAL, TRUE, &pbs, pclsid);
                if (pbs) 
                {
                    IDeskBand *pband = _GetInfoBandBS(pbs,pclsid);
                    if (pband) 
                    {
                        IBandNavigate *pbn;
                        pband->QueryInterface(IID_IBandNavigate, (void **)&pbn);
                        if (pbn) 
                        {
                            pbn->Select(pidl);
                            pbn->Release();
                        }
                        pband->Release();
                    }
                    pbs->Release();
                }
                hres = S_OK;        
            }
            break;
        }

        case SHDVID_CLSIDTOIDM:
            ASSERT(pvarargIn && pvarargIn->vt == VT_BSTR && pvarargOut);
            CLSID clsid;

            GUIDFromString(pvarargIn->bstrVal, &clsid);

            pvarargOut->vt = VT_I4;
            pvarargOut->lVal = _InfoCLSIDToIdm(&clsid);
            
            hres = S_OK;
            break;
        }
    } 
    else if (IsEqualGUID(CGID_Theater, *pguidCmdGroup)) 
    {
        if (_ptheater)
            hres = _ptheater->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
    }
    else if (IsEqualGUID(CGID_ExplorerBarDoc, *pguidCmdGroup)) 
    {
        //  These are commands that should be applied to all the explorer bar bands.  for example
        //  to reflect font size changes to the search band. they should be applied to the
        //  contained doc object, changing guid to CGID_MSTHML
        _ExecAllBands(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
    }
    return hres;
}

//***   _IsSameToolbar -- does punkBar have specified name?
// ENTRY/EXIT
//  fRet    TRUE if matches, o.w. FALSE
//
BOOL CShellBrowser2::_IsSameToolbar(LPWSTR wszBarName, IUnknown *punkBar)
{
    BOOL fRet;
    HRESULT hres;
    IUnknown *punk;

    fRet = FALSE;
    hres = FindToolbar(wszBarName, IID_IUnknown, (void **)&punk);
    ASSERT((hres == S_OK) == (punk != NULL));
    if (punk) {
        if (SHIsSameObject(punkBar, punk)) {
            fRet = TRUE;
        }
        punk->Release();
    }
    return fRet;
}

STDAPI SHGetWindowTitle(LPCITEMIDLIST pidl, LPTSTR pszFullName, DWORD cchSize)
{
    CABINETSTATE cabstate;
    GetCabState(&cabstate);

    return SHTitleFromPidl(pidl, pszFullName, cchSize, cabstate.fFullPathTitle);
}

/**********************************************\
    FUNCTION:   CShellBrowser2::_SetTitle

    PARAMETERS:
        LPCSTR pszName - The Name of the URL, not
            limited in size, but the result will
            be truncated if the imput is too long.

    DESCRIPTION:
        The title will be generated by taking the
    title of the HTML page (pszName) and appending
    the name of the browser to the end in the
    following format:
    "HTML_TITLE - BROWSER_NAME", like:
    "My Web Page - Microsoft Internet Explorer"
\**********************************************/
void CShellBrowser2::_SetTitle(LPCWSTR pwszName)
{
    TCHAR szTitle[MAX_BROWSER_WINDOW_TITLE];
    TCHAR szFullName[MAX_PATH];
    BOOL fNotDisplayable = FALSE;

    if (!pwszName && _fTitleSet) 
    {
        // if the content has once set our title,
        // don't revert to our own mocked up name
        return;
    }
    else if (pwszName)
    {
        _fTitleSet = TRUE;
    }

    BOOL fDisplayable = SHIsDisplayable(pwszName, g_fRunOnFE, g_bRunOnNT5);

    if (pwszName && fDisplayable) 
    {
        StrCpyN(szFullName, pwszName, ARRAYSIZE(szFullName));
    }
    else if (_pbbd->_pidlCur)
    {
        SHGetWindowTitle(_pbbd->_pidlCur, szFullName, ARRAYSIZE(szFullName));
    }
    else if (_pbbd->_pidlPending)
    {
        SHGetWindowTitle(_pbbd->_pidlPending, szFullName, ARRAYSIZE(szFullName));
    }
    else
        szFullName[0] = 0;

    // Before adding on the app title truncate the szFullName so that if it is 
    // really long then when the app title " - Microsoft Internet Explorer" is
    // appended it fits.
    //
    // Used to be 100, but that wasn't quite enough to display default title.
    //
    ASSERT(96 <= ARRAYSIZE(szFullName));
    SHTruncateString(szFullName, 96); // any more than 60 is useless anyways

    if (szFullName[0]) 
    {
        TCHAR szBuf[MAX_URL_STRING];

        v_GetAppTitleTemplate(szBuf, szFullName);

        wnsprintf(szTitle, ARRAYSIZE(szTitle), szBuf, szFullName); 
    }
    else if (_fInternetStart)
    {
        _GetAppTitle(szTitle, ARRAYSIZE(szTitle));
    } 
    else
        szTitle[0] = 0;

    SendMessage(_pbbd->_hwnd, WM_SETTEXT, 0, (LPARAM)szTitle);
}

void _SetWindowIcon (HWND hwnd, HICON hIcon, BOOL bLarge)

{
    HICON   hOldIcon;
    //
    // If the shell window is RTL mirrored, then flip the icon now,
    // before inserting them into the system cache, so that they end up
    // normal (not mirrrored) on the shell. This is mainly a concern for
    // 3rd party components. [samera]
    //
    if (IS_PROCESS_RTL_MIRRORED())
    {        
        SHMirrorIcon(&hIcon, NULL);
    }

    hOldIcon = (HICON)SendMessage(hwnd, WM_SETICON, bLarge, (LPARAM)hIcon);
    if (hOldIcon &&
        (hOldIcon != hIcon))
    {
        DestroyIcon(hOldIcon);
    }
}

void _WindowIconFromImagelist(HWND hwndMain, int nIndex, BOOL bLarge)
{
    HICON hIcon;
    HIMAGELIST himlSysLarge = NULL;
    HIMAGELIST himlSysSmall = NULL;

    Shell_GetImageLists(&himlSysLarge, &himlSysSmall);


    // if we're using the def open icon or if extracting fails,
    // use the icon we've already created.

    hIcon = ImageList_ExtractIcon(g_hinst, bLarge ? himlSysLarge : himlSysSmall, nIndex);
    if (!hIcon)
        return;

    _SetWindowIcon(hwndMain, hIcon, bLarge);
}

int CShellBrowser2::_GetIconIndex(void)
{

    int iSelectedImage = -1;
    if (_pbbd->_pidlCur) 
    {
        if (_pbbd->_pctView) // we must check!
        {
            VARIANT var = {0};
            HRESULT hresT = _pbbd->_pctView->Exec(&CGID_ShellDocView, SHDVID_GETSYSIMAGEINDEX, 0, NULL, &var);
            if (SUCCEEDED(hresT)) {
                if (var.vt==VT_I4) {
                    iSelectedImage= var.lVal;
                } else {
                    ASSERT(0);
                    VariantClearLazy(&var);
                }
            }
        }

        if (iSelectedImage==-1)
        {
            //
            // Put Optimization here. 
            //
            IShellFolder *psfParent;
            LPCITEMIDLIST pidlChild;

            if (SUCCEEDED(IEBindToParentFolder(_pbbd->_pidlCur, &psfParent, &pidlChild))) 
            {
                // set the small one first to prevent user stretch blt on the large one
                SHMapPIDLToSystemImageListIndex(psfParent, pidlChild, &iSelectedImage);
                psfParent->Release();
            }
        }
    }
    return iSelectedImage;
}

bool    CShellBrowser2::_IsExplorerBandVisible (void)

//  99/02/10 #254171 vtan: This function determines whether
//  the explorer band is visible. This class should not really
//  care but it needs to know to change the window's icon.
//  This is a hack and should be moved or re-architected.

//  99/02/12 #292249 vtan: Re-worked algorithm to use
//  IBandSite::QueryBand and IPersistStream::GetClassID to
//  recognize the explorer band. Note that the old routine
//  used IDeskBand's inclusion of IOleWindow to get the HWND
//  and use the Win32 API IsWindowVisible().

//  99/06/25 #359477 vtan: Put this code back.
//  IsControlWindowShown uses QueryStatus for the explorer
//  band which doesn't work when v_SetIcon is called. The
//  band is NOT considered latched and therefore not visible.

{
    bool        bVisible;
    HRESULT     hResult;
    IDeskBar    *pIDeskBar;

    bVisible = false;
    hResult = FindToolbar(INFOBAR_TBNAME, IID_IDeskBar, reinterpret_cast<void**>(&pIDeskBar));
    if (SUCCEEDED(hResult) && (pIDeskBar != NULL))
    {
        UINT    uiToolBar;

        uiToolBar = _FindTBar(pIDeskBar);
        if (uiToolBar != static_cast<UINT>(-1))
        {
            LPTOOLBARITEM   pToolBarItem;

            pToolBarItem = _GetToolbarItem(uiToolBar);
            if ((pToolBarItem != NULL) && pToolBarItem->fShow)  // check this state
            {
                IUnknown    *pIUnknown;

                hResult = pIDeskBar->GetClient(reinterpret_cast<IUnknown**>(&pIUnknown));
                if (SUCCEEDED(hResult) && (pIUnknown != NULL))
                {
                    IBandSite   *pIBandSite;

                    hResult = pIUnknown->QueryInterface(IID_IBandSite, reinterpret_cast<void**>(&pIBandSite));
                    if (SUCCEEDED(hResult) && (pIBandSite != NULL))
                    {
                        UINT    uiBandIndex;
                        DWORD   dwBandID;

                        uiBandIndex = 0;
                        hResult = pIBandSite->EnumBands(uiBandIndex, &dwBandID);
                        while (SUCCEEDED(hResult))
                        {
                            DWORD       dwState;
                            IDeskBand   *pIDeskBand;

                            dwState = 0;
                            hResult = pIBandSite->QueryBand(dwBandID, &pIDeskBand, &dwState, NULL, 0);
                            if (SUCCEEDED(hResult))
                            {
                                CLSID   clsid;

                                hResult = IUnknown_GetClassID(pIDeskBand, &clsid);
                                if (SUCCEEDED(hResult) && IsEqualGUID(clsid, CLSID_ExplorerBand))
                                    bVisible = ((dwState & BSSF_VISIBLE) != 0);     // and this state
                                pIDeskBand->Release();
                                hResult = pIBandSite->EnumBands(++uiBandIndex, &dwBandID);
                            }
                        }
                        pIBandSite->Release();
                    }
                    pIUnknown->Release();
                }
            }
        }
        pIDeskBar->Release();
    }
    return(bVisible);
}

void CShellBrowser2::v_SetIcon()
{
    if (_IsExplorerBandVisible())
    {
        #define IDI_STFLDRPROP          46
        //  Explorer tree view pane is visible - use the magnifying glass icon
        HICON hIcon = reinterpret_cast<HICON>(LoadImage(HinstShell32(), MAKEINTRESOURCE(IDI_STFLDRPROP), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
        _SetWindowIcon(_pbbd->_hwnd, hIcon, ICON_SMALL);
        hIcon = reinterpret_cast<HICON>(LoadImage(HinstShell32(), MAKEINTRESOURCE(IDI_STFLDRPROP), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CXICON), 0));
        _SetWindowIcon(_pbbd->_hwnd, hIcon, ICON_BIG);
    }
    else
    {
        //  Otherwise use whatever icon it really should be
        int iSelectedImage = _GetIconIndex();
        if (iSelectedImage != -1) 
        {
            _WindowIconFromImagelist(_pbbd->_hwnd, iSelectedImage, ICON_SMALL);
            _WindowIconFromImagelist(_pbbd->_hwnd, iSelectedImage, ICON_BIG);
        }
    }
}

HRESULT CShellBrowser2::SetTitle(IShellView * psv, LPCWSTR lpszName)
{
    // BUGBUG: If the pending view had it's title set immediately and waits
    // in pending state for a while. And if the current view has script updating
    // the title. Then the current title will be displayed after the navigate
    // is complete. I added psv to fix this problem to CBaseBrowser2, but I
    // didn't fix it here. [mikesh]
    //
    //  Don't set title if view is still pending (or you'll show unrated titles)
        // Figure out which object is changing.

    if (SHIsSameObject(_pbbd->_psv, psv))
    {
        _SetTitle(lpszName);
    }

    SUPERCLASS::SetTitle(psv, lpszName);
    return NOERROR;
}

HRESULT CShellBrowser2::UpdateWindowList(void)
{
    if (_psw) {
        WinList_NotifyNewLocation(_psw, _dwRegisterWinList, _pbbd->_pidlCur);
    }
    return S_OK;
}

HRESULT CShellBrowser2::SetFlags(DWORD dwFlags, DWORD dwFlagMask)
{
    if (dwFlagMask & BSF_THEATERMODE)
        _TheaterMode(dwFlags & BSF_THEATERMODE, TRUE);

    if (dwFlagMask & BSF_RESIZABLE)
        SHSetWindowBits( _pbbd->_hwnd, GWL_STYLE, WS_SIZEBOX, (dwFlags & BSF_RESIZABLE) ? WS_SIZEBOX : 0 );

    if ((dwFlagMask & BSF_UISETBYAUTOMATION) && (dwFlags & BSF_UISETBYAUTOMATION))
    {
        _fDontSaveViewOptions = TRUE;
        _fUISetByAutomation = TRUE;
        //HACK to hide any visible browser bar
        //BUGBUG this will be removed when explorer bars become 1st class toolbars
        _SetBrowserBarState(_idmInfo, NULL, 0);
        _SetBrowserBarState(_idmComm, NULL, 0);
    }

    return SUPERCLASS::SetFlags(dwFlags, dwFlagMask);
}

HRESULT CShellBrowser2::GetFlags(DWORD *pdwFlags)
{
    DWORD dwFlags;
    
    SUPERCLASS::GetFlags(&dwFlags);

    if (_fUISetByAutomation)
        dwFlags |= BSF_UISETBYAUTOMATION;
    if (_fNoLocalFileWarning)
        dwFlags |= BSF_NOLOCALFILEWARNING;
    if (_ptheater)
        dwFlags |= BSF_THEATERMODE;
    *pdwFlags = dwFlags;

    return S_OK;
}

DWORD CShellBrowser2::v_ShowControl(UINT iControl, int iCmd)
{
    int iShowing = -1;
    int nWhichBand;

    switch (iControl) 
    {
    case FCW_STATUS:
        iShowing = (_fStatusBar ? SBSC_SHOW : SBSC_HIDE);
        if (iCmd != SBSC_QUERY && (iShowing != iCmd)) 
        {
            _fStatusBar = !_fStatusBar;
            // let itbar know that a change has occurred
            IUnknown_Exec(_GetITBar(), &CGID_PrivCITCommands, CITIDM_STATUSCHANGED, 0, NULL, NULL);
            v_ShowHideChildWindows(FALSE);
        }
        break;

    case FCW_INTERNETBAR:
        {
            LPTOOLBARITEM ptbi = _GetToolbarItem(ITB_ITBAR);
            iShowing = (ptbi->fShow ? SBSC_SHOW : SBSC_HIDE);
            if (iCmd != SBSC_QUERY &&
                (iShowing != iCmd)) 
            {
                ptbi->fShow = !ptbi->fShow;
                v_ShowHideChildWindows(FALSE);
            }
        }
        break;

    case FCW_ADDRESSBAR:
    case FCW_TOOLBAND:
    case FCW_LINKSBAR:
    case FCW_MENUBAR:
        switch(iControl)
        {
            case FCW_ADDRESSBAR:
                nWhichBand = CITIDM_SHOWADDRESS;
                break;

            case FCW_TOOLBAND:
                nWhichBand = CITIDM_SHOWTOOLS;
                break;

            case FCW_LINKSBAR:
                nWhichBand = CITIDM_SHOWLINKS;
                break;

            case FCW_MENUBAR:
                nWhichBand = CITIDM_SHOWMENU;
                break;
        }
        if (iCmd != SBSC_QUERY)
        {
            IUnknown_Exec(_GetITBar(), &CGID_PrivCITCommands, 
                nWhichBand, (iCmd == SBSC_SHOW), NULL, NULL);
        }
        break;

    default:
        break;
    }

    return iShowing;
}

HRESULT CShellBrowser2::ShowControlWindow(UINT id, BOOL fShow)
{
    switch (id)
    {
    case (UINT)-1:  // Set into Kiosk mode...
        if (BOOLIFY(_fKioskMode) != fShow)
        {
            _fKioskMode = fShow;
            if (_fKioskMode)
            {
                _wndpl.length = SIZEOF(WINDOWPLACEMENT);
                GetWindowPlacement(_pbbd->_hwnd, &_wndpl);
                SHSetWindowBits(_pbbd->_hwnd, GWL_STYLE, WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX, 0);
                SHSetWindowBits(_pbbd->_hwnd, GWL_EXSTYLE, WS_EX_WINDOWEDGE, 0);
                _SetMenu(NULL);

                LPTOOLBARITEM ptbi = _GetToolbarItem(ITB_ITBAR);
                if (ptbi)
                {
                    ptbi->fShow = FALSE;
                }

                HMONITOR hmon = MonitorFromRect(&_wndpl.rcNormalPosition, MONITOR_DEFAULTTONEAREST);
                RECT rcMonitor;
                GetMonitorRect(hmon, &rcMonitor);
                
                SetWindowPos(_pbbd->_hwnd, NULL, rcMonitor.top, rcMonitor.left, RECTWIDTH(rcMonitor),
                        RECTHEIGHT(rcMonitor), SWP_NOZORDER);
            }
            else
            {
                if (_fShowMenu)
                    _SetMenu(_hmenuCur);
                else
                    _SetMenu(NULL);

                SHSetWindowBits(_pbbd->_hwnd, GWL_STYLE, WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX, WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX);
                SHSetWindowBits(_pbbd->_hwnd, GWL_EXSTYLE, WS_EX_WINDOWEDGE, WS_EX_WINDOWEDGE);
                SetWindowPlacement(_pbbd->_hwnd, &_wndpl);
            }

            // Let window manager know to recalculate things...
            v_ShowHideChildWindows(FALSE);

            SetWindowPos(_pbbd->_hwnd, NULL, 0, 0, 0, 0,
                    SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
        }
        break;

    case FCW_INTERNETBAR:
    case FCW_STATUS:
    case FCW_ADDRESSBAR:
        v_ShowControl(id, fShow ? SBSC_SHOW : SBSC_HIDE);
        break;
        
    case FCW_MENUBAR:
        if (BOOLIFY(_fShowMenu) != BOOLIFY(fShow))
        {
            _fShowMenu = BOOLIFY(fShow);
            if (_fShowMenu)
            {
                _SetMenu(_hmenuCur);
            }
            else
            {
                _SetMenu(NULL);
            }

            // Let window manager know to recalculate things...
            v_ShowControl(id, fShow ? SBSC_SHOW : SBSC_HIDE);
            // Let ITBar know whether to allow selection of menu bar or no
            IUnknown_Exec(_GetITBar(), &CGID_PrivCITCommands, CITIDM_DISABLESHOWMENU, !_fShowMenu, NULL, NULL);

            SetWindowPos(_pbbd->_hwnd, NULL, 0, 0, 0, 0,
                    SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
        }
        break;

    default:
        return E_INVALIDARG;   // Not one of the ones we support...
    }
    return NOERROR;
}

BOOL CShellBrowser2::_ShouldAllowNavigateParent()
{
    LPCITEMIDLIST pidl = ILIsRooted(_pbbd->_pidlCur) ? ILGetNext(_pbbd->_pidlCur) : _pbbd->_pidlCur;
    return !ILIsEmpty(pidl);
}

HRESULT CShellBrowser2::IsControlWindowShown(UINT id, BOOL *pfShown)
{
    switch (id)
    {
    case (UINT)-1:  // Set into Kiosk mode...
        *pfShown = _fKioskMode;
        break;

    case FCW_INTERNETBAR:
        *pfShown = _GetToolbarItem(ITB_ITBAR)->fShow;
        break;

    case FCW_STATUS:
        *pfShown = _fStatusBar;
        break;
        
    case FCW_MENUBAR:
        *pfShown = _fShowMenu;
        break;
        
    case FCW_TREE:
    {
        BOOL    fShown;

        OLECMD rgCmds[1] = {0};
        rgCmds[0].cmdID = SBCMDID_EXPLORERBAR;
        QueryStatus(&CGID_Explorer, ARRAYSIZE(rgCmds), rgCmds, NULL);
        fShown = (rgCmds[0].cmdf & OLECMDF_LATCHED);
        if (pfShown != NULL)
            *pfShown = fShown;
        break;
    }

    default:
        return E_INVALIDARG;   // Not one of the ones we support...
    }

    return NOERROR;
}

HRESULT CShellBrowser2::SetReferrer(LPITEMIDLIST pidl)
{
    //
    //  this is only used when we create a new window, and
    //  we need some sort of ZoneCrossing context.
    //
    Pidl_Set(&_pidlReferrer, pidl);

    return (_pidlReferrer || !pidl) ? S_OK :E_FAIL;
}

HRESULT CShellBrowser2::_CheckZoneCrossing(LPCITEMIDLIST pidl)
{
    HRESULT hr = S_OK;

    //
    //  NOTE - right now we only handle having one or the other - zekel 8-AUG-97
    //  we should only have pidlReferrer if we have been freshly 
    //  created.  if we decide to use it on frames that already exist,
    //  then we need to decide whether we should show pidlCur or pidlReferrer.
    //  by default we give Referrer preference.
    //
    AssertMsg((!_pidlReferrer && !_pbbd->_pidlCur) || 
        (_pidlReferrer && !_pbbd->_pidlCur) || 
        (!_pidlReferrer && _pbbd->_pidlCur), 
        TEXT("REVIEW: should this be allowed?? -zekel"));

    LPITEMIDLIST pidlRef = _pidlReferrer ? _pidlReferrer : _pbbd->_pidlCur;

    //
    //  Call InternetConfirmZoneCrossingA API only if this is the top-level
    // browser (not a browser control) AND there is a current page.
    //
    if (pidlRef) {
        
        HRESULT hresT = S_OK;
        // Get the URL of the current page.
        WCHAR szURLPrev[MAX_URL_STRING];

        const WCHAR c_szURLFile[] = L"file:///c:\\";  // dummy one
        LPCWSTR pszURLPrev = c_szURLFile;    // assume file:

        // BUGBUG: We should get the display name first and then only use
        //         the default value if the szURLPrev doesn't have a scheme.
        //         Also do this for szURLNew below.  This will fix Folder Shortcuts
        //         especially to Web Folders.  We also need to use the pidlTarget
        //         Folder Shortcut pidl.
        if (IsURLChild(pidlRef, FALSE))
        {
            hresT = ::IEGetDisplayName(pidlRef, szURLPrev, SHGDN_FORPARSING);
            pszURLPrev = szURLPrev;
        }

        if (SUCCEEDED(hresT))
        {
            // Get the URL of the new page.
            WCHAR szURLNew[MAX_URL_STRING];
            LPCWSTR pszURLNew = c_szURLFile;
            if (IsURLChild(pidl, FALSE)) {
                hresT = ::IEGetDisplayName(pidl, szURLNew, SHGDN_FORPARSING);
                pszURLNew = szURLNew;
            }

            if (pszURLPrev != pszURLNew && SUCCEEDED(hresT))
            {
                // HACK: This API takes LPTSTR instead of LPCTSTR. 
                DWORD err = InternetConfirmZoneCrossing(_pbbd->_hwnd, (LPWSTR)pszURLPrev, (LPWSTR) pszURLNew, FALSE);

                TraceMsg(DM_ZONE, "CSB::_Nav InetConfirmZoneXing %hs %hs returned %d", pszURLPrev, pszURLNew, err);
                if (err != ERROR_SUCCESS) {
                    //
                    //  Either the user cancelled it or there is not enough
                    // memory. Abort the navigation.
                    //
                    TraceMsg(DM_ERROR, "CSB::_CheckZoneCrossing ICZC returned error (%d)", err);
                    hr = HRESULT_FROM_WIN32(err);
                }
            } else {
                TraceMsg(DM_ZONE, "CSB::_Nav IEGetDisplayName(pidl) failed %x", hresT);
            }
        } else {
            TraceMsg(DM_ZONE, "CSB::_Nav IEGetDisplayName(pidlRef) failed %x", hresT);
        }
    }

    SetReferrer(NULL);

    return hr;
}

BOOL CShellBrowser2::v_IsIEModeBrowser()
{
    //
    // if we didnt register the window or if it is not registered as 3rdparty,
    //  then it is allowed to be an IEModeBrowser.
    //
    return (!_fDidRegisterWindow || (_swcRegistered != SWC_3RDPARTY)) && 
        (_fInternetStart || (_pbbd->_pidlCur && IsURLChild(_pbbd->_pidlCur, TRUE)));
}


// IServiceProvider::QueryService

STDMETHODIMP CShellBrowser2::QueryService(REFGUID guidService, REFIID riid, void **ppvObj)
{
    if (IsEqualGUID(guidService, SID_SExplorerToolbar)) 
    {
        LPTOOLBARITEM ptbi = _GetToolbarItem(ITB_ITBAR);
        if (ptbi->ptbar) 
        {
            return ptbi->ptbar->QueryInterface(riid, ppvObj);
        }
    } 
    else if (IsEqualGUID(guidService, SID_SMenuBandHandler) || 
             IsEqualGUID(guidService, SID_SHostProxyFilter))
    {
        return QueryInterface(riid, ppvObj);
    }
    
    return SUPERCLASS::QueryService(guidService, riid, ppvObj);
}

HRESULT CShellBrowser2::EnableModelessSB(BOOL fEnable)
{
    HRESULT hres = SUPERCLASS::EnableModelessSB(fEnable);
//
//  We don't want to leave the frame window disabled if a control left
// us disabled because of its bug. Instead, we'll put a warning dialog
// box -- IDS_CLOSEANYWAY. (SatoNa)
//
#if 0
    EnableMenuItem(GetSystemMenu(_pbbd->_hwnd, FALSE), SC_CLOSE, (S_OK == _DisableModeless()) ?
                         (MF_BYCOMMAND | MF_GRAYED) : ( MF_BYCOMMAND| MF_ENABLED));
#endif
    return hres;
}

HRESULT CShellBrowser2::GetViewStateStream(DWORD grfMode, IStream **ppstm)

//  99/02/05 #226140 vtan: DefView doesn't perist the dwDefRevCount
//  like ShellBrowser does. When DefView asks ShellBrowser for the
//  view state stream ShellBrowser would blindly return the stream
//  (implemented in the super class CCommonBrowser). In order to
//  ensure the stream's validity the method is now replaced with
//  this code which verifies the dwDefRevCount matches. If there
//  is a mismatch the function bails with an error otherwise it
//  calls the regularly scheduled program (super CCommonBrowser).

//  BUGBUG: This will have to be revisited when separating frame
//  state from view state.

{
    HRESULT         hResult;
    IStream*        pIStream;
    LPCITEMIDLIST   pIDL;

    pIDL = _pbbd->_pidlNewShellView;
    if (pIDL == NULL)
        pIDL = _pbbd->_pidlPending;
    if (pIDL == NULL)
        pIDL = _pbbd->_pidlCur;
    pIStream = v_GetViewStream(pIDL, STGM_READ, L"CabView");
    if (pIStream != NULL)
    {
        CABSH   cabinetStateHeader;

        hResult = _FillCabinetStateHeader(pIStream, &cabinetStateHeader);
        pIStream->Release();
        if (SUCCEEDED(hResult) &&
            ((cabinetStateHeader.fMask & CABSHM_REVCOUNT) != 0) &&
            (g_dfs.dwDefRevCount != cabinetStateHeader.dwRevCount))
        {
            *ppstm = NULL;
            return(E_FAIL);
        }
    }
    return(SUPERCLASS::GetViewStateStream(grfMode, ppstm));
}

LRESULT CALLBACK CShellBrowser2::DummyTBWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CShellBrowser2* pSB = (CShellBrowser2*)GetWindowPtr0(hwnd);    // GetWindowLong(hwnd, 0)
    LRESULT lRes = 0L;
    
    if (uMsg < WM_USER)
        return(::DefWindowProcWrap(hwnd, uMsg, wParam, lParam));
    else    
    {
        switch (uMsg) {
            
        case TB_ADDBITMAP:
            pSB->_pxtb->AddBitmap(&CGID_ShellBrowser, BITMAP_NORMAL, (UINT)wParam, (TBADDBITMAP*)lParam, &lRes, RGB(192,192,192));
            pSB->_pxtb->AddBitmap(&CGID_ShellBrowser, BITMAP_HOT, (UINT)wParam, (TBADDBITMAP*)lParam, &lRes, RGB(192,192,192));
            break;
            
        default:
            if (pSB->_pxtb)
                pSB->_pxtb->SendToolbarMsg(&CGID_ShellBrowser, uMsg, wParam, lParam, &lRes);
            return lRes;
        }
    }
    return lRes;
}    

// When a view adds buttons with no text, the CInternet toolbar may call back and ask
// for the tooltip strings. Unfortunately, at that time _pbbd->_hwndView is not yet set and therefore
// the tooltip texts will not be set. 
//
// So, to get around that, we do not add the buttons if there is no _pbbd->_hwndView (see ::SetToolbarItem)
// The CBaseBrowser2's ::_SwitchActivationNow() is the one that sets the _pbbd->_hwndView. So when this hwnd is
// set then we add the buttons
// 
// We send the WM_NOTIFYFORMAT because when CInternetToolbar::AddButtons calls back with the WM_NOTIFYs 
// for the tooltips, we need to know whether or not the view is UNICODE or not.
HRESULT CShellBrowser2::_SwitchActivationNow()
{
    ASSERT(_pbbd->_psvPending);

#if 0
    // if we have a progress control, make sure it's off before we switch activation
    if (_hwndProgress)
        SendControlMsg(FCW_PROGRESS, PBM_SETRANGE32, 0, 0, NULL);
#endif

    SUPERCLASS::_SwitchActivationNow();

    // need to do this as close to the assign of _pbbd->_hwndView as possible
    _fUnicode = (SendMessage (_pbbd->_hwndView, WM_NOTIFYFORMAT,
                                 (WPARAM)_pbbd->_hwnd, NF_QUERY) == NFR_UNICODE);
    
    if (_lpButtons) {
        LocalFree(_lpButtons);
        _lpButtons = NULL;
        _nButtons = 0;
    }
    
    if (_lpPendingButtons)
    {
        
        _lpButtons = _lpPendingButtons;
        _nButtons = _nButtonsPending;
        _lpPendingButtons = NULL;
        _nButtonsPending = 0;
        
        if ((_pxtb) && (_pbbd->_hwndView))
            _pxtb->AddButtons(&CGID_ShellBrowser, _nButtons, _lpButtons);
        
    }    
    return S_OK;
}


#ifdef DEBUG
/*----------------------------------------------------------
Purpose: Dump the menu handles for this browser.  Optionally
         breaks after dumping handles.

*/
void
CShellBrowser2::_DumpMenus(
    IN LPCTSTR pszMsg,
    IN BOOL    bBreak)
{
    if (IsFlagSet(g_dwDumpFlags, DF_DEBUGMENU))
    {
        ASSERT(pszMsg);

        TraceMsg(TF_ALWAYS, "CShellBrowser2: Dumping menus for %#08x %s", (void *)this, pszMsg);
        TraceMsg(TF_ALWAYS, "   _hmenuTemplate = %x, _hmenuFull = %x, _hmenuBrowser = %x",
                 _hmenuTemplate, _hmenuFull, _hmenuBrowser);
        TraceMsg(TF_ALWAYS, "   _hmenuCur = %x, _hmenuPreMerged = %x, _hmenuHelp = %x",
                 _hmenuCur, _hmenuPreMerged, _hmenuHelp);

        _menulist.Dump(pszMsg);
        
        if (bBreak && IsFlagSet(g_dwBreakFlags, BF_ONDUMPMENU))
            DebugBreak();
    }
}
#endif

HRESULT CShellBrowser2::SetBorderSpaceDW(IUnknown* punkSrc, LPCBORDERWIDTHS pborderwidths)
{
    return SUPERCLASS::SetBorderSpaceDW(punkSrc, pborderwidths);
}

//
//  This is a helper member of CBaseBroaser class (non-virtual), which
// returns the effective client area. We get this rectangle by subtracting
// the status bar area from the real client area.
//
HRESULT CShellBrowser2::_GetEffectiveClientArea(LPRECT lprectBorder, HMONITOR hmon)
{
    static const int s_rgnViews[] =  {1, 0, 1, FCIDM_STATUS, 0, 0};

    // n.b. do *not* call SUPER/_psbInner

    ASSERT(hmon == NULL);
    GetEffectiveClientRect(_pbbd->_hwnd, lprectBorder, (LPINT)s_rgnViews);
    return NOERROR;
}

// Should we return more informative return values?
// "Browser Helper Objects"

BOOL CShellBrowser2::_LoadBrowserHelperObjects(void)
{
    BOOL bRet = FALSE;
    HKEY hkey;
    if (_pbbd->_pautoWB2 &&
        RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_EXPLORER TEXT("\\Browser Helper Objects"), &hkey) == ERROR_SUCCESS)
    {
        TCHAR szGUID[64];
        DWORD cb = ARRAYSIZE(szGUID);
        for (int i = 0;
             RegEnumKeyEx(hkey, i, szGUID, &cb, NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
             i++)
        {
            CLSID clsid;
            IObjectWithSite *pows;
            if (GUIDFromString(szGUID, &clsid) &&
                SUCCEEDED(CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IObjectWithSite, (void **)&pows)))
            {
                pows->SetSite(_pbbd->_pautoWB2);    // give the poinetr to IWebBrowser2

                SA_BSTRGUID strClsid;
                // now register this object so that it can be found through automation.
                SHTCharToUnicode(szGUID, strClsid.wsz, ARRAYSIZE(strClsid.wsz));
                strClsid.cb = lstrlenW(strClsid.wsz) * SIZEOF(WCHAR);

                VARIANT varUnknown = {0};
                varUnknown.vt = VT_UNKNOWN;
                varUnknown.punkVal = pows;
                _pbbd->_pautoWB2->PutProperty(strClsid.wsz, varUnknown);

                pows->Release(); // Instead of calling variantClear() 

                bRet = TRUE;
            }
            cb = ARRAYSIZE(szGUID);
        }
        RegCloseKey(hkey);
    }
    return bRet;
}

HRESULT CShellBrowser2::OnViewWindowActive(IShellView * psv)
{
    _pbsInner->SetActivateState(SVUIA_ACTIVATE_FOCUS);
    return SUPERCLASS::OnViewWindowActive(psv);
}

void CShellBrowser2::_PositionViewWindow(HWND hwnd, LPRECT prc)
{
    RECT rc = *prc;

    if (_ptheater) {
        InflateRect(&rc, GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE));
    }
    
    SetWindowPos(hwnd, NULL,
                 rc.left, rc.top, 
                 rc.right - rc.left, 
                 rc.bottom - rc.top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

HRESULT CShellBrowser2::OnFocusChangeIS(IUnknown* punkSrc, BOOL fSetFocus)
{
    if (fSetFocus && _ptheater && SHIsSameObject(punkSrc, _GetITBar())) {
        _ptheater->Exec(&CGID_Theater, THID_TOOLBARACTIVATED, 0, NULL, NULL);
    }
    return SUPERCLASS::OnFocusChangeIS(punkSrc, fSetFocus);
}


HRESULT CShellBrowser2::Offline(int iCmd)
{
    HRESULT hresIsOffline = SUPERCLASS::Offline(iCmd);
        
    if (iCmd == SBSC_TOGGLE)
    {        
        VARIANTARG var = {0};
        if (_pbbd->_pctView && SUCCEEDED(_pbbd->_pctView->Exec(&CGID_Explorer, SBCMDID_GETPANE, PANE_OFFLINE, NULL, &var))
            && V_UI4(&var) != PANE_NONE)
        {
            SendControlMsg(FCW_STATUS, SB_SETICON, V_UI4(&var), 
                (hresIsOffline == S_OK) ? (LPARAM) OfflineIcon() : NULL, NULL);
            if (hresIsOffline == S_OK) {
                InitTitleStrings();
                SendControlMsg(FCW_STATUS, SB_SETTIPTEXT, V_UI4(&var), 
                               (LPARAM) g_szWorkingOfflineTip, NULL);
            }
        } 
    }
        
    return hresIsOffline;
}

HRESULT CShellBrowser2::_FreshenComponentCategoriesCache( BOOL bForceUpdate )
{
    CATID catids[2] ;
    ULONG cCatids = 0 ;
    catids[0] = CATID_InfoBand ;
    catids[1] = CATID_CommBand ;

    //  Check if our CATIDs are cached...
    if ( !bForceUpdate )
    {
        for( ULONG i=0; i< ARRAYSIZE(catids); i++ )
        {
            if ( S_OK != SHDoesComCatCacheExist( catids[i], TRUE ) )
            {
                bForceUpdate = TRUE ;
                break ;
            }
        }
    }

    if ( bForceUpdate )
        return SHWriteClassesOfCategories( ARRAYSIZE(catids), catids, 0, NULL, 
                                           TRUE, FALSE /*no wait*/ ) ;

    return S_FALSE ;
}

void CShellBrowser2::_QueryHKCRChanged()
{
    ASSERT(GetUIVersion() > 4);
    //  In an integrated shell/browser installation, we have the benefit
    //  of an HKCR change notification.  Posting this message will cause
    //  the desktop to check to see if HKCR was modified recently; if so, 
    //  the _SetupAppRan handler will execute in the desktop process.   
    //  This causes, among other evil, freshening of our component categories cache.
    //  it is ok that this is isnt synchronous because the update
    //  is async regardless.
    PostMessage( GetShellWindow(), DTM_QUERYHKCRCHANGED, 
                 QHKCRID_VIEWMENUPOPUP, (LPARAM)NULL) ;
}

#ifdef UNIX

BOOL CShellBrowser2::_HandleActivation( WPARAM wParam )
{
     LPTOOLBARITEM ptbi;
     IDockingWindow *ptbTmp;
     IOleCommandTarget* pcmdt;

     // Activation is explicitly handled only in MOTIF LOOK
     if ( MwCurrentLook() != LOOK_MOTIF )
          return (FALSE);

     // Get InetnetToolbar IDockingWindow interface
     ptbi = _GetToolbarItem(ITB_ITBAR);
     if ( !ptbi )
          return (FALSE);
    
     ptbTmp = ptbi->ptbar;
          
     if (ptbTmp)
     {
          HRESULT hres = S_FALSE;

          // Exec Command. IOleCommandTarget::Exec for CInternetToobar window
          // forwards the commands to the address band.
          hres = ptbTmp->QueryInterface(IID_IOleCommandTarget, (void **)&pcmdt);

          if (SUCCEEDED(hres) && pcmdt) 
          {
               if ( wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE )
               {
                   // Did we have toobar focus when deactivated.
                   if ( _fSetAddressBarFocus )
                   {
                        VARIANTARG vaOut = {0};
                        hres = pcmdt->Exec(&CGID_Explorer, SBCMDID_SETADDRESSBARFOCUS, 0, NULL, &vaOut);
                     
                        // SERADDRESSBARFOCUS command handled by the retrieved IOleCommandTarget.
                        // No need to process any furthur.
                        if (SUCCEEDED(hres)) 
                        {
                            pcmdt->Release();
                            return (TRUE);
                        }
                    }
               }
               else
               {
                    // Focus should only be set to the Addressbar if we already had
                    // focus there before losing activation, otherwise do default 
                    // processing.
                    VARIANTARG vaOut = {0};
                    hres = pcmdt->Exec(&CGID_Explorer, SBCMDID_HASADDRESSBARFOCUS, 0, NULL, &vaOut);
                     
                    _fSetAddressBarFocus = SUCCEEDED(hres);
               }

               pcmdt->Release();
          }
     }

     return (FALSE);
}

#endif
