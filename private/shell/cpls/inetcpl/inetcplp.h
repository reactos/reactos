//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************
//
//      INETCPL.H - central header file for Internet control panel
//
//      HISTORY:
//
//      4/3/95      jeremys         Created.
//      6/25/96     t-ashlem        condensed most header files into here
//                                    and other cleanup
//

#ifndef _INETCPL_H_
#define _INETCPL_H_

// Extra error checking (catches false errors, but useful to run every so often)
#if 1
#pragma warning(3:4701)   // local may be used w/o init
#pragma warning(3:4702)   // Unreachable code
#pragma warning(3:4705)   // Statement has no effect
#pragma warning(3:4709)   // command operator w/o index expression
#endif

// IEUNIX - removing warning for redefinition of STRICT
#ifdef STRICT
#undef STRICT
#endif

#define STRICT                      // Use strict handle types
#define _SHELL32_

#define _CRYPT32_    // get DECLSPEC_IMPORT stuff right for Cert API


#include <windows.h>
#include <windowsx.h>

#ifdef WINNT
#include <shellapi.h>
#endif // WINNT


#include <shlobj.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <prsht.h>
#include <cpl.h>
#include <regstr.h>
#include <ccstock.h>
#include <validate.h>
#include <debug.h>
#include <mshtml.h>
#include <shsemip.h>
#include <wincrypt.h>

    //
    // All HKEYs are defined in this library: INETREG.LIB
    // please change/add any HKEYs to this library. Thanks!
    //
#include <inetreg.h>

    //
    // Delay load DLLs' globals (see DLYLDDLL.C for details)
    //
#include "dlylddll.h"

#include <ratings.h>

#include <commdlg.h>
#include <olectl.h>

#define _WINX32_  // get DECLSPEC_IMPORT stuff right for WININET API
#include <urlmon.h>
#include <wininet.h>

#define _URLCACHEAPI_  // get DECLSPEC_IMPORT stuff right for wininet urlcache
#ifdef WINNT
#include <winineti.h>
#endif // WINNT

#define MAX_URL_STRING    INTERNET_MAX_URL_LENGTH
#include <shlwapi.h>

#include "shellp.h"
#include "shguidp.h"

#include "oleacc.h"
// Hack. winuserp.h and winable.h both define the flag WINEVENT_VALID. Since
// no one in inetcpl uses this value we undef it to make the compile work.
#undef WINEVENT_VALID
#include "winable.h"

#ifdef UNICODE
#define POST_IE5_BETA
#include <w95wraps.h>
#endif


#undef WINVER
#define WINVER 0x400
#include <ras.h>
#include <raserror.h>

//
// When a user clicks "reset web defaults" (on the programs tab), we may need
// to update the url shown in the general tab.  The general tab will check this
// flag each time it's made active, and also when we hit ok or apply.
//
extern BOOL g_fReloadHomePage;

///////////////////////////////////////////////////////////////////////
//
// Structure Defintions and other typedefs
//
///////////////////////////////////////////////////////////////////////

typedef struct _RESTRICT_FLAGS
{
    BOOL fGeneralTab;               // Enable/disable "General" tab
    BOOL fSecurityTab;              // Enable/disable "Security" tab
    BOOL fContentTab;               // Enable/disable "Content" tab
    BOOL fConnectionsTab;           // Enable/disable "Programs" tab
    BOOL fProgramsTab;              // Enable/disable "Connections" tab
    BOOL fAdvancedTab;              // Enable/disable "Advanced" tab
    BOOL fColors;                   // Colors section of Colors dialog
    BOOL fLinks;                    // Links section of Links dialog
    BOOL fFonts;                    // Fonts dialog
    BOOL fInternational;            // Languages dialog
    BOOL fDialing;                  // Connection section of Connection tab (incl Settings subdialog)
    BOOL fProxy;                    // Proxy section of Connection tab (incl Advanced subdialog)
    BOOL fPlaces;                   // Home page section of General tab
    BOOL fHistory;                  // History section of General tab
    BOOL fMailNews;                 // Messaging section of the Programs tab
    BOOL fRatings;                  // Ratings buttons on Content tab
    BOOL fCertif;                   // Certificate section of Content tab
    BOOL fCertifPers;               // Personal Cert button
    BOOL fCertifSite;               // Site Cert button
    BOOL fCertifPub;                // Publishers button
    BOOL fCache;                    // Temporary Internet Files section of General tab
    BOOL fAutoConfig;               // Autoconig section of Connection tab
    BOOL fAccessibility;            // Accessibility dialog
    BOOL fSecChangeSettings;        // can't change level
    BOOL fSecAddSites;              // can't add/remove sites
    BOOL fProfiles;                 // Profile Asst section of Content tab
    BOOL fFormSuggest;              // AutoSuggest for forms on Content tab
    BOOL fFormPasswords;            // AutoSuggest for form passwords on Content tab
#ifdef WALLET
    BOOL fWallet;                   // MS Wallet section of Content tab
#endif
    BOOL fConnectionWizard;         // Connection Wizard section of Connection tab
    BOOL fCalContact;               // Cal/Contact section of Programs tab
    BOOL fAdvanced;                 // Advanced page
    BOOL fCacheReadOnly;            // Disables the delete and Settings buttons on the general panel
    BOOL fResetWebSettings;         // Disables the "reset web settings" feature
    BOOL fDefault;                  // IE should check if it's the default browser

#if 0
    BOOL fMultimedia;               // OBSOLETE: do not use
    BOOL fToolbar;                  // OBSOLETE: do not use
    BOOL fFileTypes;                // OBSOLETE: do not use
    BOOL fActiveX;                  // OBSOLETE: do not use
    BOOL fActiveDownload;           // OBSOLETE: do not use
    BOOL fActiveControls;           // OBSOLETE: do not use
    BOOL fActiveScript;             // OBSOLETE: do not use
    BOOL fActiveJava;               // OBSOLETE: do not use
    BOOL fActiveSafety;             // OBSOLETE: do not use
    BOOL fWarnings;                 // OBSOLETE: do not use
    BOOL fOther;                    // OBSOLETE: do not use
    BOOL fCrypto;                   // OBSOLETE: do not use
    BOOL fPlacesDefault;            // OBSOLETE: do not use
#endif

} RESTRICT_FLAGS, *LPRESTRICT_FLAGS;

typedef struct tagPROXYINFO
{
    BOOL    fEnable;
    BOOL    fEditCurrentProxy;
    BOOL    fOverrideLocal;
    BOOL    fReadProxyFromRegistry;
    BOOL    fCustomHandler;
    TCHAR   szProxy[MAX_URL_STRING];
    TCHAR   szOverride[MAX_URL_STRING];
} PROXYINFO, *LPPROXYINFO;


// function pointer typedefs
typedef DWORD   (WINAPI * RASENUMENTRIESA) (LPSTR, LPSTR, LPRASENTRYNAMEA, LPDWORD, LPDWORD);
typedef DWORD   (WINAPI * RASENUMENTRIESW) (LPSTR, LPSTR, LPRASENTRYNAMEW, LPDWORD, LPDWORD);
typedef DWORD   (WINAPI * RASCREATEPHONEBOOKENTRYA) (HWND,LPSTR);
typedef DWORD   (WINAPI * RASEDITPHONEBOOKENTRYA) (HWND,LPSTR,LPSTR);
typedef DWORD   (WINAPI * RASEDITPHONEBOOKENTRYW) (HWND,LPWSTR,LPWSTR);
typedef DWORD   (WINAPI * RASGETENTRYDIALPARAMSA) (LPSTR, LPRASDIALPARAMSA, LPBOOL);
typedef DWORD   (WINAPI * RASGETENTRYDIALPARAMSW) (LPWSTR, LPRASDIALPARAMSW, LPBOOL);
typedef DWORD   (WINAPI * RASSETENTRYDIALPARAMSA) (LPSTR, LPRASDIALPARAMSA, BOOL);
typedef DWORD   (WINAPI * RASSETENTRYDIALPARAMSW) (LPWSTR, LPRASDIALPARAMSW, BOOL);
typedef DWORD   (WINAPI * RASDELETEENTRYA) (LPSTR, LPSTR);
typedef DWORD   (WINAPI * RASDELETEENTRYW) (LPWSTR, LPWSTR);
typedef DWORD   (WINAPI * RASGETENTRYPROPERTIESW) (LPCWSTR, LPCWSTR, LPRASENTRYW, LPDWORD, LPBYTE, LPDWORD);
typedef DWORD   (WINAPI * RNAACTIVATEENGINE) (void);
typedef DWORD   (WINAPI * RNADEACTIVATEENGINE) (void);
typedef DWORD   (WINAPI * RNADELETEENTRY) (LPSTR);

///////////////////////////////////////////////////////////////////////
//
// #defines
//
///////////////////////////////////////////////////////////////////////

#define IDC_NOTUSED             ((unsigned) IDC_UNUSED)
#define INM_UPDATE              (WM_USER + 100)

#define MAX_RES_LEN             255
#define SMALL_BUF_LEN           48


// NOTE: If you change these max values to something other than two digits, then you'll need to change
// the call in connectn.cpp:DialupDlgInit which sets the limittext to 2 chars.
#define DEF_AUTODISCONNECT_TIME 20      // default disconnect timeout is 20 mins
#define MIN_AUTODISCONNECT_TIME 3       // minimum disconnect timeout is 3 mins
#define MAX_AUTODISCONNECT_TIME 59      // maximum disconnect timeout is 59 mins

// NOTE: If you change these max values to something other than two digits, then you'll need to change
// the call in connectn.cpp:DialupDlgInit which sets the limittext to 2 chars.
#define DEF_REDIAL_TRIES        10
#define MAX_REDIAL_TRIES        99
#define MIN_REDIAL_TRIES        1

#define CO_INTERNET             1
#define CO_INTRANET             2

// NOTE: If you change these max values to something other than two digits, then you'll need to change
// the call in connectn.cpp:DialupDlgInit which sets the limittext to 2 chars.
#define DEF_REDIAL_WAIT         5
#define MAX_REDIAL_WAIT         99
#define MIN_REDIAL_WAIT         5

#define MESSAGE_SIZE    255
#define BITMAP_WIDTH    16
#define BITMAP_HEIGHT   16
#define NUM_BITMAPS     5
#define MAX_KEY_NAME    64
#define COLOR_BG        0
//
#define IDCHECKED       0
#define IDUNCHECKED     1
#define IDRADIOON       2
#define IDRADIOOFF      3
#define IDUNKNOWN       4
//
#define SZDEFAULTBITMAP TEXT("DefaultBitmap")
#define SZHT_RADIO      TEXT("radio")
#define SZHT_CHECKBOX   TEXT("checkbox")
//
#define RET_CHECKBOX    0
#define RET_RADIO       1
//
#define TREE_NEITHER    1
#define TREE_CHECKBOX   2
#define TREE_RADIO      4
//
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
//
#define RCS_GETSTATE    1
#define RCS_SETSTATE    2

// Used with the various registry functions to detect when a value isn't
// present
#define VALUE_NOT_PRESENT   -255

#define DEFAULT_CPL_PAGE    -1

///////////////////////////////////////////////////////////////////////
//
// Macros
//
///////////////////////////////////////////////////////////////////////

#define ENABLEAPPLY(hDlg) SendMessage( GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L )
#define SetPropSheetResult( hwnd, result ) SetWindowLongPtr(hwnd, DWLP_MSGRESULT, result)

#undef DATASEG_READONLY
#define DATASEG_READONLY        ".rdata"
#include "resource.h"
#include "clsutil.h"

///////////////////////////////////////////////////////////////////////
//
// Read-Only Global Variables
//
///////////////////////////////////////////////////////////////////////

extern HINSTANCE      ghInstance;       // global module instance handle
extern const DWORD    mapIDCsToIDHs[];  // Help IDC to IDH map
extern RESTRICT_FLAGS g_restrict;       // var to restrict access to pages


// functions in UTIL.C
int MsgBox(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons);
int MsgBoxSz(HWND hWnd,LPTSTR szText,UINT uIcon,UINT uButtons);
int _cdecl MsgBoxParam(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons,...);
LPTSTR LoadSz(UINT idString,LPTSTR lpszBuf,UINT cbBuf);
BOOL EnableDlgItem(HWND hDlg,UINT uID,BOOL fEnable);
VOID _cdecl DisplayErrorMessage(HWND hWnd,UINT uStrID,UINT uError,
                                UINT uErrorClass,UINT uIcon,...);
BOOL WarnFieldIsEmpty(HWND hDlg,UINT uCtrlID,UINT uStrID);
VOID DisplayFieldErrorMsg(HWND hDlg,UINT uCtrlID,UINT uStrID);
VOID GetErrorDescription(CHAR * pszErrorDesc,UINT cbErrorDesc,
                         UINT uError,UINT uErrorClass);

// functions in RNACALL.C
BOOL InitRNA(HWND hWnd);
VOID DeInitRNA();

// structure for getting proc addresses of api functions
typedef struct APIFCN {
    PVOID * ppFcnPtr;
    LPCSTR pszName;
} APIFCN;


#undef  DATASEG_PERINSTANCE
#define DATASEG_PERINSTANCE     ".instance"
#undef  DATASEG_SHARED
#define DATASEG_SHARED          ".data"
#define DATASEG_DEFAULT         DATASEG_SHARED


///////////////////////////////////////////////////////////////////////
//
// Global Variables
//
///////////////////////////////////////////////////////////////////////

extern TCHAR g_szCurrentURL[INTERNET_MAX_URL_LENGTH];
extern HWND g_hwndUpdate;
extern HWND g_hwndPropSheet;
extern BOOL g_fChangedMime;

///////////////////////////////////////////////////////////////////////
//
// Dialog Procs
//
///////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK AdvancedDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                              LPARAM lParam);

INT_PTR CALLBACK TemporaryDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                               LPARAM lParam);

INT_PTR CALLBACK ConnectionDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                LPARAM lParam);

INT_PTR CALLBACK General_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                              LPARAM lParam);

#ifdef UNIX
BOOL CALLBACK AssocDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                              LPARAM lParam);
BOOL CALLBACK AliasDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                              LPARAM lParam);
#endif

STDAPI_(INT_PTR) OpenFontsDialog(HWND hDlg, LPCSTR lpszKeyPath);
STDAPI_(INT_PTR) OpenFontsDialogEx(HWND hDlg, LPCTSTR lpszKeyPath);

INT_PTR CALLBACK FontsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK LanguageDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK PlacesDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);

INT_PTR CALLBACK ProgramsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                              LPARAM lParam);

INT_PTR CALLBACK ProxyDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                           LPARAM lParam);

INT_PTR CALLBACK SafetyDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);

INT_PTR CALLBACK SecurityDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);

INT_PTR CALLBACK PrintDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK ContentDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

extern "C" void CALLBACK OpenLanguageDialog(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);
void KickLanguageDialog(HWND hDlg);
///////////////////////////////////////////////////////////////////////
//
// Dialog Proc Helpers
//
///////////////////////////////////////////////////////////////////////

// hunts down all windows and notifies them that they should update themselves
void UpdateAllWindows();

// Windows Help helper
void ResWinHelp( HWND hwnd, int ids, int id2, DWORD_PTR dwp);

#ifdef UNIX

void FindEditClient(LPTSTR szProtocol, HWND hwndDlg, int nIDDlgItem, LPTSTR szPath);
BOOL EditScript(HKEY hkeyProtocol);
BOOL FindScript(HWND hwndLable, HKEY hkeyProtocol);
#define DIR_SEPR FILENAME_SEPARATOR

#include <tchar.h>
#include <platform.h>
#include "unixstuff.h"

inline
BOOL
HAS_DRIVE_LETTER(LPCTSTR pszPath)
{
    ASSERT(pszPath!=NULL);
    return (pszPath[0] == '/');
}

#else

#define DIR_SEPR '\\'
inline
BOOL
HAS_DRIVE_LETTER(LPCTSTR pszPath)
{
    ASSERT(pszPath!=NULL);
    ASSERT(pszPath[0]!='\0');
    return (pszPath[1] == ':');
}

#endif

//
// We can be hung if we use sendMessage, and you can not use pointers with asynchronous
// calls such as PostMessage or SendNotifyMessage.  So we resort to using a timeout.
// This function should be used to broadcast notification messages, such as WM_SETTINGCHANGE,
// that pass pointers.
//
inline LRESULT SendBroadcastMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return SHSendMessageBroadcastW(uMsg, wParam, lParam);
}

// struct used in security.cpp as tls in a hack to get around bad dialog creation situation
struct SECURITYINITFLAGS
{
    DWORD    dwZone;
    BOOL     fForceUI;
    BOOL     fDisableAddSites;
    SECURITYINITFLAGS()
    {
        dwZone = 0;
        fForceUI = FALSE;
        fDisableAddSites = FALSE;
    }
};

#endif // _INETCPL_H_
