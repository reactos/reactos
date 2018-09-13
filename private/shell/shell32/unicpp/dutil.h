#ifndef _DUTIL_H_
#define _DUTIL_H_

#include "local.h"
#include "deskstat.h"

extern "C" LONG g_cRefThisDll;

typedef struct _EnumMonitorsArea
{
    int iMonitors;
    RECT rcWorkArea[LV_MAX_WORKAREAS];
    RECT rcMonitor[LV_MAX_WORKAREAS];
    RECT rcVirtualMonitor;
    RECT rcVirtualWorkArea;         //Excluding the tray/toolbar areas
} EnumMonitorsArea;

void SaveDefaultFolderSettings();
BOOL GetFileName(HWND hdlg, LPTSTR pszFileName, int iSize, int iTypeId[], DWORD dwFlags[]);
void PatternToDwords(LPTSTR psz, DWORD *pdwBits);
void PatternToWords(LPTSTR psz, WORD *pwBits);
BOOL IsValidPattern(LPCTSTR pszPat);
BOOL IsNormalWallpaper(LPCTSTR pszFileName);
BOOL IsWallpaperPicture(LPCTSTR pszWallpaper);
void CheckAndResolveLocalUrlFile(LPTSTR pszFileName, int cchFileName);
void GetMyCurHomePageStartPos(int *piLeft, int *piTop, DWORD *pdwWidth, DWORD *pdwHeight);
BOOL AddDesktopComponentNoUI(DWORD dwApplyFlags, LPCTSTR pszUrl, LPCTSTR pszFriendlyName, int iCompType, int iLeft, int iTop, int iWidth, int iHeight, BOOL fChecked, DWORD dwCurItemState, BOOL fNoScroll = FALSE, BOOL fCanResize = TRUE);
void InitDeskHtmlGlobals(void);
HBITMAP LoadMonitorBitmap(void);
void PositionComponent(COMPONENTA *pcomp, COMPPOS *pcp, int iCompType, BOOL fCheckItemState);
BOOL UpdateDesktopPosition(LPTSTR pszCompId, int iLeft, int iTop, DWORD dwWidth, DWORD dwHeight, int izIndex, BOOL fSaveState, BOOL fSaveOriginal, DWORD dwNewState);
BOOL GetSavedStateInfo(LPTSTR pszCompId, LPCOMPSTATEINFO    pCompState, BOOL fRestoredState);
DWORD GetCurrentState(LPTSTR pszCompId);
BOOL UpdateComponentFlags(LPCTSTR pszCompId, DWORD dwMask, DWORD dwNewFlags);
void GetRegLocation(LPTSTR lpszResult, DWORD cchResult, LPCTSTR lpszKey, LPCTSTR lpszScheme);
BOOL ValidateFileName(HWND hwnd, LPCTSTR pszFilename, int iTypeString);

#ifdef WE_WANT_DEFAULT_WALLPAPER
void SetDefaultWallpaper();
void GetDefaultWallpaper(LPTSTR psz, DWORD cch);
#endif //WE_WANT_DEFAULT_WALLPAPER

BOOL EnableADifHtmlWallpaper(HWND hwnd);
void GetWallpaperDirName(LPTSTR lpszWallPaperDir, int iBuffSize);
BOOL _AddDesktopComponentA(HWND hwnd, LPCSTR pszUrlA, int iCompType, int iLeft, int iTop, int iWidth, int iHeight, DWORD dwFlags);
void GetMonitorSettings(EnumMonitorsArea* ema);
int GetWorkAreaIndex(COMPPOS *pcp, LPCRECT prect, int crect, LPPOINT lpptVirtualTopLeft);
int GetWorkAreaIndexFromPoint(POINT pt, LPCRECT prect, int crect);
void ReadWallpaperStyleFromReg(LPCTSTR pszRegKey, DWORD *pdwWallpaperStyle, BOOL fIgnorePlatforms);
void GetWallpaperWithPath(LPCTSTR szWallpaper, LPTSTR szWallpaperWithPath, int iBufSize);
BOOL GetViewAreas(LPRECT lprcViewAreas, int* pnViewAreas);
BOOL GetZoomRect(BOOL fFullScreen, BOOL fAdjustListview, int tridentLeft, int tridentTop, DWORD componentWidth, DWORD componentHeight, LPRECT prcZoom, LPRECT prcWork);
void GetNextComponentPosition(COMPPOS *pcp);
void IncrementComponentsPositioned(void);
void ValidateComponentPosition(COMPPOS *pcp, DWORD dwComponentState, int iComponentType, bool *pbChangedPosition, bool *pbChangedSize);
int GetcyCaption();
BOOL IsICWCompleted(void);
void LaunchICW(void);
BOOL IsLocalPicture(LPCTSTR pszURL);
BOOL DisableUndisplayableComponents(IActiveDesktop *pIAD);

// Top level reg keys
#define REG_DESKCOMP                        TEXT("Software\\Microsoft\\Internet Explorer\\Desktop")
#define REG_DESKCOMP_GENERAL                TEXT("Software\\Microsoft\\Internet Explorer\\Desktop%sGeneral")
#define REG_DESKCOMP_GENERAL_SUFFIX         TEXT("General")
#define REG_DESKCOMP_COMPONENTS             TEXT("Software\\Microsoft\\Internet Explorer\\Desktop%sComponents")
#define REG_DESKCOMP_COMPONENTS_SUFFIX      TEXT("Components")
#define REG_DESKCOMP_SAFEMODE               TEXT("Software\\Microsoft\\Internet Explorer\\Desktop\\SafeMode")
#define REG_DESKCOMP_SAFEMODE_SUFFIX        TEXT("SafeMode")
#define REG_DESKCOMP_SAFEMODE_SUFFIX_L      L"SafeMode"
#define REG_DESKCOMP_SCHEME                 TEXT("Software\\Microsoft\\Internet Explorer\\Desktop\\Scheme")
#define REG_DESKCOMP_SCHEME_LOCATION        TEXT("Software\\Microsoft\\Internet Explorer\\Desktop\\Scheme\\Location")
#define REG_DESKCOMP_SCHEME_SUFFIX          TEXT("Scheme")
#define REG_DESKCOMP_COMPONENTS_ROOT        TEXT("Software\\Microsoft\\Internet Explorer\\Desktop\\Components")
#define REG_DESKCOMP_GENERAL_ROOT           TEXT("Software\\Microsoft\\Internet Explorer\\Desktop\\General")
#define REG_DESKCOMP_OLDWORKAREAS           TEXT("Software\\Microsoft\\Internet Explorer\\Desktop\\Old WorkAreas")
#define REG_DESKCOMP_ADMINCOMP_ROOT         TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\ActiveDesktop\\AdminComponent")

// values for toplevel (misc)
#define REG_VAL_MISC_CHANNELSIZE            TEXT("ChannelSize")

// values for General
#define REG_VAL_GENERAL_CCOMPPOS            TEXT("ComponentsPositioned")
#define REG_VAL_GENERAL_DESKTOPFILE         TEXT("HTMLFile")
#define REG_VAL_GENERAL_TILEWALLPAPER       TEXT("TileWallpaper")
#define REG_VAL_GENERAL_WALLPAPER           TEXT("Wallpaper")
#define REG_VAL_GENERAL_BACKUPWALLPAPER     TEXT("BackupWallpaper")
#define REG_VAL_GENERAL_WALLPAPERTIME       TEXT("WallpaperFileTime")
#define REG_VAL_GENERAL_WALLPAPERSTYLE      TEXT("WallpaperStyle")
#define REG_VAL_GENERAL_VISITGALLERY        TEXT("VisitGallery")
#define REG_VAL_GENERAL_RESTRICTUPDATE      TEXT("RestrictChannelUI")

// values for Components
#define REG_VAL_COMP_VERSION                TEXT("DeskHtmlVersion")
#define REG_VAL_COMP_MINOR_VERSION          TEXT("DeskHtmlMinorVersion")
#define REG_VAL_COMP_GENFLAGS               TEXT("GeneralFlags")
#define REG_VAL_COMP_SETTINGS               TEXT("Settings")
#define REG_VAL_COMP_UPGRADED_FROM          TEXT("UpgradedFrom")

// values for each component entry
#define REG_VAL_COMP_FLAGS                  TEXT("Flags")
#define REG_VAL_COMP_NAME                   TEXT("FriendlyName")
#define REG_VAL_COMP_POSITION               TEXT("Position")
#define REG_VAL_COMP_SOURCE                 TEXT("Source")
#define REG_VAL_COMP_SUBSCRIBED_URL         TEXT("SubscribedURL")
#define REG_VAL_COMP_CURSTATE               TEXT("CurrentState")
#define REG_VAL_COMP_ORIGINALSTATEINFO      TEXT("OriginalStateInfo")
#define REG_VAL_COMP_RESTOREDSTATEINFO      TEXT("RestoredStateInfo")

// values for Scheme
#define REG_VAL_SCHEME_DISPLAY              TEXT("Display")
#define REG_VAL_SCHEME_EDIT                 TEXT("Edit")

// values for old work areas
#define REG_VAL_OLDWORKAREAS_COUNT          TEXT("NoOfOldWorkAreas")
#define REG_VAL_OLDWORKAREAS_RECTS          TEXT("OldWorkAreaRects")

// values for Admin Component
#define REG_VAL_ADMINCOMP_ADD               TEXT("Add")
#define REG_VAL_ADMINCOMP_DELETE            TEXT("Delete")

TCHAR g_szNone[];
EXTERN_C const TCHAR c_szControlIni[];
EXTERN_C const TCHAR c_szPatterns[];
EXTERN_C const TCHAR c_szComponentPreview[];
EXTERN_C const TCHAR c_szRegDeskHtmlProp[];
EXTERN_C const TCHAR c_szBackgroundPreview2[];
EXTERN_C const TCHAR c_szZero[];
EXTERN_C const TCHAR c_szWallpaper[];

#define EnableApplyButton(hdlg) PropSheet_Changed(GetParent(hdlg), hdlg)

// Note: Incrementing the CUR_DESKHTML_VERSION will blow away all the existing
// components already in the registry. So, do this with caution!
#define CUR_DESKHTML_VERSION 0x110

// Note: Incrementing the CUR_DESKHTM_MINOR_VERSION can be used to do two things:
// 1. It will simply set the dirty bit sothat the desktop.htt gets re-generated. 
//    For example, do this whenever template file deskmovr.htt changes.
// 2. It can be used to add a new default component (for example "MyCurrentHome") without
//    destroying any of the existing components in the user's registry.
//
#define CUR_DESKHTML_MINOR_VERSION 0x0003

//The following are the Major and minor version numbers stamped on the registry for IE4.0x
#define IE4_DESKHTML_VERSION        0x010e
#define IE4_DESKHTML_MINOR_VERSION  0x0001

//The following are the Major and minor version numbers stamped on the registry for IE5.0x
#define IE5_DESKHTML_VERSION        0x010f
#define IE5_DESKHTML_MINOR_VERSION  0x0001

#define  COMPON_FILENAME              TEXT("\\Web\\Compon.htm")
#define  COMPONENTHTML_FILENAME       TEXT("\\Web\\TryIt.htm")
#define  DESKTOPHTML_DEFAULT_SAFEMODE TEXT("\\Web\\SafeMode.htt")
#define  DESKTOPHTML_DEFAULT_WALLPAPER TEXT("Wallpapr.htm")
#define  DESKTOPHTML_DEFAULT_MEMPHIS_WALLPAPER TEXT("Windows98.htm")
#define  DESKTOPHTML_DEFAULT_NT5_WALLPAPER TEXT("Active Desktop Wallpaper.htm")
//#define  DESKTOPHTML_DEFAULT_NT5_WALLPAPER TEXT("Windows 2000 Wallpaper.bmp")
#define  PREVIEW_PICTURE_FILENAME      TEXT("PrePict.htm")
#define  DESKTOPHTML_WEB_DIR           TEXT("\\Web")

#define GFN_PICTURE         0x00000001
#define GFN_LOCALHTM        0x00000002      // local *.htm and *.html files only
#define GFN_URL             0x00000004
#define GFN_CDF             0x00000008
#define GFN_LOCALMHTML      0x00000010      // local *.mht and *.mhtml files only
#define GFN_ALL             (GFN_PICTURE | GFN_LOCALHTM | GFN_URL | GFN_CDF | GFN_LOCALMHTML)

#define CXYDESKPATTERN 8

// Valid bits for REG_VAL_COMP_SETTINGS
#define COMPSETTING_ENABLE      0x00000001    

//
// Dimensions of the monitor contents in the monitor bitmap.
// Used in the desk property sheet "preview" controls.
//
#define MON_X 16
#define MON_Y 17
#define MON_DX 152
#define MON_DY 112

//
// Attributes of default components.
//
#define EGG_LEFT            130
#define EGG_TOP             180
#define EGG_WIDTH           160
#define EGG_HEIGHT          160

#define CBAR_SOURCE         TEXT("131A6951-7F78-11D0-A979-00C04FD705A2")
#define CBAR_TOP            6
#define CBAR_WIDTH          84
#define CBAR_BUTTON_HEIGHT  35 // height of one button

// My Current Home page component's default values.
#define MY_HOMEPAGE_SOURCE  TEXT("About:Home")
#define MY_HOMEPAGE_SOURCEW L"About:Home"
#define MYCURHOME_TOP       6
#define MYCURHOME_WIDTH     160
#define MYCURHOME_HEIGHT    160

#define COMPONENT_PER_ROW 3
#define COMPONENT_PER_COL 2

BOOL GetInfoTip(IShellFolder* psf, LPCITEMIDLIST pidl, LPTSTR pszText, int cchTextMax);

int SHLoadString(HINSTANCE hInstance, UINT uID, LPTSTR szBuffer, int nBufferMax);
BSTR StrRetToBStr(LPCITEMIDLIST pidl, STRRET *pstr);

#define  LoadMenuPopup(id) SHLoadMenuPopup(HINST_THISDLL, id)

#define VALIDATESTATE(x)            ((((x) & ~IS_VALIDSTATEBITS) == 0) && ((((x) & IS_VALIDSIZESTATEBITS) == IS_NORMAL) || (((x) & IS_VALIDSIZESTATEBITS) == IS_SPLIT) || (((x) & IS_VALIDSIZESTATEBITS) == IS_FULLSCREEN)))
#define ISZOOMED(x)                 (((x)->dwCurItemState & IS_SPLIT) || ((x)->dwCurItemState & IS_FULLSCREEN))
#define IsZoomedState(itemState)    ((((itemState) & IS_SPLIT) != 0) || (((itemState) & IS_FULLSCREEN) != 0))

// dvutil.cpp
#include <webcheck.h>
#include <mshtml.h>
typedef struct IHTMLElement IHTMLElement;
HRESULT CSSOM_TopLeft(IHTMLElement * pIElem, POINT * ppt);
HRESULT GetHTMLElementStrMember(IHTMLElement *pielem, LPTSTR pszName, DWORD cchSize, BSTR bstrMember);
HRESULT IElemCheckForExistingSubscription(IHTMLElement *pielem);
HRESULT IElemCloseDesktopComp(IHTMLElement *pielem);
HRESULT IElemGetSubscriptionsDialog(IHTMLElement *pielem, HWND hwnd);
HRESULT IElemSubscribeDialog(IHTMLElement *pielem, HWND hwnd);
HRESULT IElemUnsubscribe(IHTMLElement *pielem);
HRESULT IElemUpdate(IHTMLElement *pielem);
HRESULT IElemOpenInNewWindow(IHTMLElement *pielem, IOleClientSite *piOCSite, BOOL fShowFrame, LONG width, LONG height);
HRESULT ShowSubscriptionProperties(LPCTSTR pszUrl, HWND hwnd);
HRESULT CreateSubscriptionsWizard(SUBSCRIPTIONTYPE subType, LPCTSTR pszUrl, SUBSCRIPTIONINFO *pInfo, HWND hwnd);
BOOL CheckForExistingSubscription(LPCTSTR lpcszURL);
void ZoomComponent(COMPPOS * pcp, DWORD dwItemState, BOOL fAdjustListview);

#define GET_CYCAPTION   (GetcyCaption())
#define GET_CXSIZE      (GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXBORDER))
#define GET_CYSIZE      (GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYBORDER))

#endif // _DUTIL_H_
