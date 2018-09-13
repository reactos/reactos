#ifndef _AUTODIAL_H_
#define _AUTODIAL_H_

#include <regstr.h>
#include <inetreg.h>
#include <windowsx.h>
#include <rasdlg.h>

// initialization for autodial
void InitAutodialModule(BOOL fGlobalDataNeeded);
void ExitAutodialModule(void);
void ResetAutodialModule(void);
void SetAutodialEnable(BOOL);

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

// connection mutex name
#define CONNECTION_MUTEX TEXT("WininetConnectionMutex")
// proxy registry mutex name, used to serialze access to proxy settings
#define PROXY_REG_MUTEX TEXT("WininetProxyRegistryMutex")

// typedefs for function pointers for Internet wizard functions
typedef VOID    (WINAPI * INETPERFORMSECURITYCHECK) (HWND,LPBOOL);

#define SMALLBUFLEN     48 // convenient size for small buffers

// callback prototype
extern "C"
VOID
InternetAutodialCallback(
    IN DWORD dwOpCode,
    IN LPCVOID lpParam
    );

// opcode ordinals for dwOpCode parameter in hook
#define WINSOCK_CALLBACK_CONNECT        1
#define WINSOCK_CALLBACK_GETHOSTBYADDR  2
#define WINSOCK_CALLBACK_GETHOSTBYNAME  3
#define WINSOCK_CALLBACK_LISTEN         4
#define WINSOCK_CALLBACK_RECVFROM       5
#define WINSOCK_CALLBACK_SENDTO         6

// maximum length of local host name
#define MAX_LOCAL_HOST          255

// max length of exported autodial handler function
#define MAX_AUTODIAL_FCNNAME    48

INT_PTR CALLBACK OnlineDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
    LPARAM lParam);
BOOL EnsureRasLoaded(BOOL *pfInstalled);
INT_PTR CALLBACK GoOfflinePromptDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ConnectDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

extern const CHAR szRegPathRemoteAccess[];
extern const CHAR szRegPathInternetSettings[];
extern const CHAR szRegValEnableAutodial[];
extern const CHAR szRegValInternetEntry[];

extern HANDLE g_hAutodialMutex;
extern HANDLE g_hProxyRegMutex;

extern BOOL g_fGetHostByNameNULLFails;

//
// Formerly dialmsg.h
//

#define WM_DIALMON_FIRST        (WM_USER + 100)

// message sent to dial monitor app window indicating that there has been
// winsock activity and dial monitor should reset its idle timer
#define WM_WINSOCK_ACTIVITY     (WM_DIALMON_FIRST + 0)

// message sent to dial monitor app window when user changes timeout through
// UI, indicating that timeout value or status has changed
#define WM_REFRESH_SETTINGS     (WM_DIALMON_FIRST + 1)

// message sent to dial monitor app window to set the name of the connectoid
// to monitor and eventually disconnect.  lParam should be an LPSTR that
// points to the name of the connectoid.
#define WM_SET_CONNECTOID_NAME  (WM_DIALMON_FIRST + 2)

// message sent to dial monitor app window when app exits
#define WM_IEXPLORER_EXITING    (WM_DIALMON_FIRST + 3)

// Types for ras functions
typedef DWORD (WINAPI* _RASHANGUP) (HRASCONN);

typedef DWORD (WINAPI* _RASDIALA) (LPRASDIALEXTENSIONS, LPSTR, LPRASDIALPARAMSA,  DWORD, LPVOID, LPHRASCONN);
typedef DWORD (WINAPI* _RASENUMENTRIESA) (LPSTR, LPSTR, LPRASENTRYNAMEA, LPDWORD, LPDWORD);
typedef DWORD (WINAPI* _RASGETENTRYDIALPARAMSA) (LPCSTR, LPRASDIALPARAMSA, LPBOOL);
typedef DWORD (WINAPI* _RASSETENTRYDIALPARAMSA) (LPCSTR, LPRASDIALPARAMSA, BOOL);
typedef DWORD (WINAPI* _RASEDITPHONEBOOKENTRYA) (HWND, LPSTR, LPSTR);
typedef DWORD (WINAPI* _RASCREATEPHONEBOOKENTRYA) (HWND, LPSTR);
typedef DWORD (WINAPI* _RASGETERRORSTRINGA) (UINT, LPSTR, DWORD);
typedef DWORD (WINAPI* _RASGETCONNECTSTATUSA) (HRASCONN, LPRASCONNSTATUSA);
typedef DWORD (WINAPI* _RASENUMCONNECTIONSA) (LPRASCONNA, LPDWORD, LPDWORD);
typedef DWORD (WINAPI* _RASGETENTRYPROPERTIESA) ( LPSTR, LPSTR, LPRASENTRYA, LPDWORD, LPBYTE, LPDWORD );
typedef DWORD (WINAPI* _RASDIALDLGA) (LPSTR, LPSTR, LPSTR, LPRASDIALDLG);

typedef DWORD (WINAPI* _RASDIALW) (LPRASDIALEXTENSIONS, LPWSTR, LPRASDIALPARAMSW,  DWORD, LPVOID, LPHRASCONN);
typedef DWORD (WINAPI* _RASENUMENTRIESW) (LPWSTR, LPWSTR, LPRASENTRYNAMEW, LPDWORD, LPDWORD);
typedef DWORD (WINAPI* _RASGETENTRYDIALPARAMSW) (LPCWSTR, LPRASDIALPARAMSW, LPBOOL);
typedef DWORD (WINAPI* _RASSETENTRYDIALPARAMSW) (LPCWSTR, LPRASDIALPARAMSW, BOOL);
typedef DWORD (WINAPI* _RASEDITPHONEBOOKENTRYW) (HWND, LPWSTR, LPWSTR);
typedef DWORD (WINAPI* _RASCREATEPHONEBOOKENTRYW) (HWND, LPWSTR);
typedef DWORD (WINAPI* _RASGETERRORSTRINGW) (UINT, LPWSTR, DWORD);
typedef DWORD (WINAPI* _RASGETCONNECTSTATUSW) (HRASCONN, LPRASCONNSTATUSW);
typedef DWORD (WINAPI* _RASENUMCONNECTIONSW) (LPRASCONNW, LPDWORD, LPDWORD);
typedef DWORD (WINAPI* _RASGETENTRYPROPERTIESW) ( LPWSTR, LPWSTR, LPRASENTRYW, LPDWORD, LPBYTE, LPDWORD );
typedef DWORD (WINAPI* _RASDIALDLGW) (LPWSTR, LPWSTR, LPWSTR, LPRASDIALDLG);

// Ras ansi prototypes
DWORD _RasDialA(LPRASDIALEXTENSIONS, LPSTR, LPRASDIALPARAMSA,  DWORD, LPVOID, LPHRASCONN);
DWORD _RasEnumEntriesA(LPTSTR, LPSTR, LPRASENTRYNAMEA, LPDWORD, LPDWORD);
DWORD _RasGetEntryDialParamsA(LPCSTR, LPRASDIALPARAMSA, LPBOOL);
DWORD _RasSetEntryDialParamsA(LPCSTR, LPRASDIALPARAMSA, BOOL);
DWORD _RasEditPhonebookEntryA(HWND, LPSTR, LPSTR);
DWORD _RasCreatePhonebookEntryA(HWND, LPSTR);
DWORD _RasGetErrorStringA(UINT, LPSTR, DWORD);
DWORD _RasGetConnectStatusA(HRASCONN, LPRASCONNSTATUSA);
DWORD _RasEnumConnectionsA(LPRASCONNA, LPDWORD, LPDWORD);
DWORD _RasGetEntryPropertiesA(LPSTR, LPSTR, LPRASENTRYA, LPDWORD, LPBYTE, LPDWORD );

// Ras wide prototypes
DWORD _RasDialW(LPRASDIALEXTENSIONS, LPWSTR, LPRASDIALPARAMSW,  DWORD, LPVOID, LPHRASCONN);
DWORD _RasEnumEntriesW(LPWSTR, LPWSTR, LPRASENTRYNAMEW, LPDWORD, LPDWORD);
DWORD _RasGetEntryDialParamsW(LPCWSTR, LPRASDIALPARAMSW, LPBOOL);
DWORD _RasSetEntryDialParamsW(LPCWSTR, LPRASDIALPARAMSW, BOOL);
DWORD _RasEditPhonebookEntryW(HWND, LPWSTR, LPWSTR);
DWORD _RasCreatePhonebookEntryW(HWND, LPWSTR);
DWORD _RasGetErrorStringW(UINT, LPWSTR, DWORD);
DWORD _RasGetConnectStatusW(HRASCONN, LPRASCONNSTATUSW);
DWORD _RasEnumConnectionsW(LPRASCONNW, LPDWORD, LPDWORD);
DWORD _RasGetEntryPropertiesW(LPWSTR, LPWSTR, LPRASENTRYW, LPDWORD, LPBYTE, LPDWORD);

DWORD _RasHangUp(HRASCONN);


// how many ras connections do we care about?
#define MAX_CONNECTION          4

#define CI_SAVE_PASSWORD        0x01
#define CI_DIAL_UNATTENDED      0x02
#define CI_AUTO_CONNECT         0x04
#define CI_SHOW_OFFLINE         0x08
#define CI_SHOW_DETAILS         0x10

// Types of network coverage settable from the ui
#define CO_INTERNET             1
#define CO_INTRANET             2

// definition to call RestartDialog in shell32
typedef int (* _RESTARTDIALOG) (HWND, LPCTSTR, DWORD);

// dialstate - passed to DialAndShowProgress
typedef struct _dialstate {
    DWORD       dwResult;       // final result
    DWORD       dwTry;          // number of dial attempts
    DWORD       dwTryCurrent;   // current attempt
    DWORD       dwWait;         // time to wait between them
    DWORD       dwWaitCurrent;  // current time
    HRASCONN    hConn;          // ras connection
    UINT_PTR    uTimerId;       // timer for redial
    HANDLE      hEvent;         // event when dialing is complete
    DWORD       dwFlags;
    RASDIALPARAMSW params;
} DIALSTATE;

BOOL
GetRedialParameters(
    IN LPWSTR pszConn,
    OUT LPDWORD pdwDialAttempts,
    OUT LPDWORD pdwDialInterval
    );

#define DEFAULT_DIAL_ATTEMPTS 10
#define DEFAULT_DIAL_INTERVAL 5

// When dealing with custom dial handler, it will inform us of disconnections.
// We keep an internal state
#define STATE_NONE          0
#define STATE_CONNECTED     1
#define STATE_DISCONNECTED  2

// info relevant to a custom dial handler
typedef struct _autodial {
    BOOL    fEnabled;
    BOOL    fHasEntry;
    BOOL    fUnattended;
    BOOL    fSecurity;
    BOOL    fForceDial;
    WCHAR   pszEntryName[RAS_MaxEntryName + 1];
} AUTODIAL;

typedef struct __cdhinfo {
    DWORD   dwHandlerFlags;
    WCHAR   pszDllName[MAX_PATH];
    WCHAR   pszFcnName[MAX_PATH];
    BOOL    fHasHandler;
} CDHINFO;

// dummy connection handle used to mean custom dial handler
#define CDH_HCONN   0xf00fffff

// CDH prototypes
BOOL
IsCDH(
    IN LPWSTR pszEntryName,
    IN CDHINFO *pcdh
    );

BOOL
CallCDH(
    IN HWND hwndParent,
    IN LPWSTR pszEntryName,
    IN CDHINFO *pcdh,
    IN DWORD dwOperation,
    OUT LPDWORD lpdwResult
    );

BOOL
IsAutodialEnabled(
    OUT BOOL    *pfForceDial,
    IN AUTODIAL *pConfig
    );

BOOL
FixProxySettingsForCurrentConnection(
    IN BOOL fForceUpdate
    );

VOID
GetConnKeyA(
    IN LPSTR pszConn,
    IN LPSTR pszKey,
    IN int iLen
    );
    
VOID
GetConnKeyW(
    IN LPWSTR pszConn,
    IN LPWSTR pszKey,
    IN int iLen
    );

BOOL
InternetAutodialIfNotLocalHost(
    IN LPSTR OPTIONAL pszURL,
    IN LPSTR OPTIONAL pszHostName
    );

BOOL
DialIfWin2KCDH(
    LPWSTR              pszEntry,
    HWND                hwndParent,
    BOOL                fHideParent,
    DWORD               *lpdwResult,
    DWORD_PTR           *lpdwConnection
    );

#endif // _AUTODIAL_H_
