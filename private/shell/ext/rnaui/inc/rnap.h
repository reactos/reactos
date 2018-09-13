/*****************************************************************************\
*                                                                             *
* rnap.h -      Remote Network Access (RNA) private interface                 *
*                                                                             *
*               Version 1.00                                                  *
*                                                                             *
*               Copyright (c) 1992-1993, Microsoft Corp. All rights reserved. *
*                                                                             *
******************************************************************************/

#ifndef _RNAP_H_
#define _RNAP_H_

#define RAS_MaxPortName     128
#define RAS_MaxComment      255
#define RAS_MaxCountry      3
#define RAS_MaxAreaCode     10
#define RAS_MaxLocal        36
#define RAS_MaxExtension    5
#define RAS_MaxSMMDesc      63
#define RAS_MaxProtocolDesc 63

/******************************************************************************
 Private RASCONNSTATE
******************************************************************************/

#define RASCS_PrivateStart  RASCS_Disconnected+1

#define RASCS_StartClosing  RASCS_PrivateStart
#define RASCS_LoggingOff    RASCS_PrivateStart+1
#define RASCS_Closing       RASCS_PrivateStart+2
#define RASCS_Closed        RASCS_PrivateStart+3
#define RASCS_Terminating   RASCS_PrivateStart+4

/******************************************************************************
 Registry key paths
******************************************************************************/

#define REGSTR_KEY_RNA        "RemoteAccess"
#define REGSTR_PATH_RNA       REGSTR_PATH_SERVICES"\\"REGSTR_KEY_RNA
#define REGSTR_VAL_IMPLICIT   "EnableImplicit"
#define REGSTR_VAL_DIALUI     "DialUI"

#define REGSTR_KEY_PROF       REGSTR_KEY_RNA"\\Profile"
#define REGSTR_VAL_USER       "User"
#define REGSTR_VAL_DOMAIN     "Domain"
#define REGSTR_VAL_NUMBER     "Number"
#define REGSTR_VAL_SCRIPT     "PPPScript"
#define REGSTR_VAL_MODE       "Mode"
#define REGSTR_VAL_TERM       "Terminal"
#define REGSTR_VAL_AUTODIALDLL "AutodialDllName"
#define REGSTR_VAL_AUTODIALFN  "AutodialFcnName"
#define REGSTR_VAL_ML          "MultiLink"

/******************************************************************************
 Related RNA module names and internal exported
******************************************************************************/

#define RNA_SERVER_MOD_NAME     "RNASERV.DLL"
#define CALLER_ACCESS_FUNC_NAME "CallerAccess"

#define RNADLL_MODULE_NAME      "RASAPI32.DLL"

#define RNASCRIPT_MODULE_NAME   "SMMSCRPT.DLL"
#define RUN_SCRIPT_FUNC_NAME    "RunRnaScript"

#define RNAUI_MODULE_NAME       "RNAUI.DLL"
#define CREATE_ENTRY_FUNC_NAME  "Remote_CreateEntry"
#define EDIT_ENTRY_FUNC_NAME    "Remote_EditEntry"
#define NOTIFY_FUNC_NAME        "Remote_Notify"

/******************************************************************************
 Asynchronous event notification from RNA engine via WM_RASDIALEVENT
******************************************************************************/

#define     RNA_ASYNCEVENT      0xFFFFFFFF  // wParam value

#define     RNA_ADD_DEVICE      0           // A new device was added
#define     RNA_DEL_DEVICE      1           // A new device was removed
#define     RNA_REINIT_DEVICE   2           // The device needs to be reintied
#define     RNA_SHUTDOWN        3           // The engine needs to shutdown
#define     RNA_TRANSLATECHANGE 4           // Translate address caps changed

/******************************************************************************
 Communication with RNA engine
******************************************************************************/

#define     CLIENT_CONNECTION   1           // Client connection
#define     SERVER_CONNECTION   2           // Server connection

#define     WM_RNAMSG       WM_USER+10

#define     RL_MINMSG       WM_RNAMSG
#define     RL_CREATE       RL_MINMSG       // create a connection
#define     RL_TERMINATE    RL_MINMSG+1     // terminate the connection
#define     RL_CONNECTED    RL_MINMSG+2     // a connection is active
#define     RL_DISCONNECTED RL_MINMSG+3     // a connection is not active
#define     RL_SUPRV        RL_MINMSG+4     // initialize supervisor
#define     RL_ACTIVATE     RL_MINMSG+5     // activate supervisor change
#define     RL_REINIT       RL_MINMSG+6     // Reinitialize the engine
#define     RL_REG_DEVCHG   RL_MINMSG+7     // Register device notification
#define     RL_GET_COUNT    RL_MINMSG+8     // Get the count
#define     RL_GW_NOTIFY    RL_MINMSG+9     // Gateway callback

/******************************************************************************
 Communication with Dial engine
******************************************************************************/

#define     DL_MINMSG       WM_RNAMSG
#define     DL_DIALEVENT    DL_MINMSG       // continue dial-up sequence
#define     DL_DISCONNECTED DL_MINMSG+1     // a connection is not active
#define     DL_AUTHENTICATE DL_MINMSG+2     // auth module sent a message
#define     DL_CONNECTED    DL_MINMSG+3     // a new connection is active
#define     DL_CLOSE        DL_MINMSG+4     // Terminate the connection window
#define     DL_MAC_MSG      DL_MINMSG+5     // MAC requests disconnection
#define     DL_RECONNECT    DL_MINMSG+6     // a connection needs reconnect
#define     DL_NOTIFYICON   DL_MINMSG+7     // tray icon notification
#define     DL_SHOW_STAT    DL_MINMSG+8     // display the status dialog

//****************************************************************************
// Communication with gateway manager
//****************************************************************************

#define     GW_MINMSG       WM_RNAMSG
#define     GW_DIALEVENT    DL_DIALEVENT    // !!WARNING!! shared message
#define     GW_CONNECTED    GW_MINMSG + 1
#define     GW_DISCONNECTED GW_MINMSG + 2
#define     GW_LOG          GW_MINMSG + 3
#define     GW_CLOSE        DL_CLOSE        // !!WARNING!! shared message
#define     GW_MAC_MSG      DL_MAC_MSG      // MAC requests disconnection

#define MAX_AUTODISCONNECT  60000       // server autodisconnect sec.

//
// Gateway to Supervisor Communication
//
LRESULT WINAPI DialInMessage(HWND, UINT, WPARAM, LPARAM);

/******************************************************************************
 Request for services from RNADLL
******************************************************************************/

DWORD WINAPI RnaEngineRequest  (UINT uCommand, DWORD dwParam);
DWORD WINAPI DialEngineRequest (UINT uCommand, DWORD dwParam1, DWORD dwParam2);
DWORD WINAPI SuprvRequest      (UINT uCommand, DWORD dwParam1, DWORD dwParam2);

#define     RA_MINCMD       0
#define     RA_LOAD         RA_MINCMD       // Notify Rna process is a loader
#define	    RA_INIT         RA_MINCMD+1	    // Initialize the rna engine
#define     RA_TERMINATE    RA_MINCMD+2     // rna engine terminated
#define     RA_REG_DEVCHG   RA_MINCMD+3     // register dev change notification
#define     RA_DEREG_DEVCHG RA_MINCMD+4     // deregister dev change notification
#define	    RA_REINIT       RA_MINCMD+5	    // Reinitialize the rna engine
#define     RA_ADD_DEVICE   RA_MINCMD+6     // Device added
#define     RA_DEL_DEVICE   RA_MINCMD+7     // Device removed
#define     RA_GET_PROP     RA_MINCMD+8     // Get property page
#define     RA_MAXCMD       RA_MINCMD+50

#define     GA_MINCMD       RA_MAXCMD+1
#define     GA_INIT_SUPRV   GA_MINCMD       // Initialize supervisor
#define     GA_ACTIVE_SUPRV GA_MINCMD+1     // Activate supervisor
#define     GA_SHUTDOWN     RA_MINCMD+2     // Shutdown the system
#define     GA_LOG_SUPRV    GA_MINCMD+3     // Register log event
#define     GA_LOGON_INFO   GA_MINCMD+4     // Logon session information
#define     GA_DISCONNECT   GA_MINCMD+5     // Terminate logon session
#define     GA_START_SUPRV  GA_MINCMD+6     // Start the host
#define     GA_STOP_SUPRV   GA_MINCMD+7     // Stop the host
#define     GA_DEV_CHANGE   GA_MINCMD+8     // A device changed
#define     GA_GET_STATS    GA_MINCMD+9     // Get the server information
#define     GA_COUNT_ACTIVE GA_MINCMD+10    // Count the active connection
#define     GA_MAXCMD       GA_MINCMD+50

#define     DA_MINCMD       GA_MAXCMD+1
#define     DA_CONNINFO     DA_MINCMD       // connection information
#define     DA_DISCONNECT   DA_MINCMD+1     // disconnect an active connection
#define     DA_RECONNECT    DA_MINCMD+2     // reconnect a dropped connection
#define	    DA_NEWCONN      DA_MINCMD+3	    // a new connection is added
#define     DA_SHUTDOWN     DA_MINCMD+4     // Shutdown the system
#define     DA_NEWEVENT     DA_MINCMD+5     // Notify a new event
#define     DA_COMPASYN     DA_MINCMD+6     // Notify a async operation completion
#define     DA_DEV_CHANGE   DA_MINCMD+7     // A device changed
#define     DA_GET_UI_WND   DA_MINCMD+8     // Get the UI window
#define     DA_SET_WND_POS  DA_MINCMD+9     // Set the status window position
#define     DA_GETSTATS     DL_MINMSG+10    // a connection needs stats
#define     DA_GET_CONNSTAT DL_MINMSG+11    // Get current connection status
#define     DA_GET_SUBENTRY DL_MINMSG+12    // Get sub-entry information
#define     DA_DIAL_SUBENTRY DL_MINMSG+13   // Dial a sub-entry
#define     DA_MAXCMD       DA_MINCMD+50

typedef struct  tagConnInfo {
    char        szEntryName[RAS_MaxEntryName+1];
    DWORD       dwRate;
    char        szSMMDesc[RAS_MaxSMMDesc+1];
    DWORD       fdwProtocols;
    char        szDeviceType[RAS_MaxDeviceType+1];
    char        szDeviceName[RAS_MaxDeviceName+1];
} CONNINFO, *PCONNINFO, FAR* LPCONNINFO;

typedef struct  tagMLInfo {
    BOOL        fEnabled;
    DWORD       cSubEntries;
} MLINFO, *PMLINFO, FAR* LPMLINFO;

typedef struct  tagSubConnInfo {
    DWORD       iSubEntry;
    MLINFO      mli;
} SUBCONNINFO, *PSUBCONNINFO, FAR* LPSUBCONNINFO;

#define PARENT_ENTRY_ID 0xFFFFFFFF

// Flags for protocols in fdwProtocols
//
#define PROTOCOL_AMB            0x00000001
#define PROTOCOL_PPPNBF         0x00000002
#define PROTOCOL_PPPIPX         0x00000004
#define PROTOCOL_PPPIP          0x00000008

//****************************************************************************
// RNAAPP Command line interface
//****************************************************************************

#define RNAAPP_EXE_NAME             "rnaapp"
#define LOAD_REQ                    "-l"      // RnaDll loads Rnaapp

//****************************************************************************
// RNAUI Private Interface
//****************************************************************************

#define DT_NULLMODEM    "null"
#define DT_MODEM        "modem"
#define DT_ISDN         "isdn"
#define DT_UNKNOWN      "unknown"

#define DIRECTCC        "Direct Cable Connection Host Logon"

typedef HICON   FAR* LPHICON;

// SMM Usage type flags
//
#define CLIENT_SMM          0x00000001
#define SERVER_SMM          0x00000002

typedef struct  tagSMMCFG  {
    DWORD       dwSize;
    DWORD       fdwOptions;
    DWORD       fdwProtocols;
}   SMMCFG, *PSMMCFG, FAR* LPSMMCFG;

typedef struct  tagSMMINFO  {
    char        szSMMType[RNA_MaxSMMType+1];
    SMMCFG      SMMConfig;
}   SMMINFO, *PSMMINFO, FAR* LPSMMINFO;

typedef struct tagIPData   {
    DWORD     dwSize;
    DWORD     fdwTCPIP;
    DWORD     dwIPAddr;
    DWORD     dwDNSAddr;
    DWORD     dwDNSAddrAlt;
    DWORD     dwWINSAddr;
    DWORD     dwWINSAddrAlt;
}   IPDATA, *PIPDATA, FAR *LPIPDATA;

typedef struct tagSMMData   {
    struct    tagSMMData *pNext;
    SMMINFO   smmi;
    DWORD     fdwOptAllow;
    DWORD     fdwProtAllow;
    IPDATA    ipData;
}   SMMDATA, *PSMMDATA, FAR *LPSMMDATA;

// Flags for the fdwTCPIP field
//

#define IPF_IP_SPECIFIED    0x00000001
#define IPF_NAME_SPECIFIED  0x00000002
#define IPF_NO_COMPRESS     0x00000004
#define IPF_NO_WAN_PRI      0x00000008

typedef struct  tagDEVICEINFO  {
    DWORD       dwVersion;
    UINT        uSize;
    char        szDeviceName[RAS_MaxDeviceName+1];
    char        szDeviceType[RAS_MaxDeviceType+1];
}   DEVICEINFO, *PDEVICEINFO, FAR* LPDEVICEINFO;

typedef struct  tagDevConfig    {
    HICON       hIcon;
    SMMINFO     smmi;
    DEVICEINFO  di;
}   DEVCONFIG, *PDEVCONFIG, FAR* LPDEVCONFIG;

typedef struct tagPhoneNum  {
    DWORD       dwCountryID;
    DWORD       dwCountryCode;
    char        szAreaCode[RAS_MaxAreaCode+1];
    char        szLocal[RAS_MaxLocal+1];
    char        szExtension[RAS_MaxExtension+1];
} PHONENUM, *PPHONENUM, FAR *LPPHONENUM;

typedef struct  tagCountryInfo {
    DWORD       dwCountryID;
    DWORD       dwNextCountryID;
    DWORD       dwCountryCode;
    DWORD       dwCountryNameOffset;
}   COUNTRYINFO, *PCOUNTRYINFO, FAR* LPCOUNTRYINFO;

typedef struct  tagConnEntry    {
    PSTR        pszEntry;
    PHONENUM    pn;
    PDEVCONFIG  pDevConfig;
    DWORD       dwFlags;
    DWORD       cSubEntry;
}   CONNENTRY, *PCONNENTRY, FAR* LPCONNENTRY;

#define CESZENTRY(pCE)         ((PSTR)(((PBYTE)pCE)+sizeof(CONNENTRY)))

#define DEVCONFIGSIZE(diSize)  (diSize+(sizeof(DEVCONFIG)-sizeof(DEVICEINFO)))

typedef struct  tagSubConnEntry    {
    DWORD       dwSize;
    DWORD       dwFlags;
    char        szDeviceType[RAS_MaxDeviceType+1];
    char        szDeviceName[RAS_MaxDeviceName+1];
    char        szLocal[RAS_MaxPhoneNumber+1];
}   SUBCONNENTRY, *PSUBCONNENTRY, FAR* LPSUBCONNENTRY;

typedef struct tagConnEntDlg    {
    PCONNENTRY  pConnEntry;
    PDEVCONFIG  pDevConfig;
    PSMMDATA    pSMMList;
    PSMMDATA    pSMMType;
    PMLINFO     pmli;
    DWORD       cmlChannel;
} CONNENTDLG, *PCONNENTDLG, FAR *LPCONNENTDLG;

typedef struct tagRNAPropPage {
    UINT      idPage;
    HMODULE   hModule;
    UINT      idRes;
    DLGPROC   pfn;
} RNAPROPPAGE, *PRNAPROPPAGE, FAR *LPRNAPROPPAGE;

#define SRV_TYPE_PAGE       0   // Server Type page id

DWORD      WINAPI RnaEnumConnEntries(LPSTR lpBuf, UINT cb, LPDWORD lpcEntries);
PCONNENTRY WINAPI RnaGetConnEntry(LPSTR szEntry, BOOL bNeedIcon, BOOL fDevice);
BOOL       WINAPI RnaFreeConnEntry(PCONNENTRY);
DWORD      WINAPI RnaSaveConnEntry(PCONNENTRY lpConnEntry);
BOOL       WINAPI RnaDeleteConnEntry(LPSTR szEntry);
DWORD      WINAPI RnaRenameConnEntry(LPSTR szOldEntry, LPSTR szNewEntry);
DWORD      WINAPI RnaValidateEntryName (LPSTR szEntry, BOOL fNew);

DWORD      WINAPI RnaEnumCountryInfo (LPCOUNTRYINFO, LPDWORD);
DWORD      WINAPI RnaGetAreaCodeList (LPSTR, LPDWORD);
DWORD      WINAPI RnaGetCurrentCountry (LPDWORD);

DWORD      WINAPI RnaEnumDevices (LPBYTE lpBuff, LPDWORD lpcbSize,
                                  LPDWORD lpcEntries);
PDEVCONFIG WINAPI RnaGetDefaultDevConfig (LPSTR szDeviceName);
DWORD      WINAPI RnaGetDeviceInfo(LPSTR szDeviceName, LPDEVICEINFO lpdi);
DWORD      WINAPI RnaGetDeviceChannel (LPSTR szDeviceName);
PDEVCONFIG WINAPI RnaBuildDevConfig (PDEVCONFIG pDevConfig, HICON hIcon, BOOL bNeedIcon);
DWORD      WINAPI RnaDevConfigDlg(HWND hWnd, PDEVCONFIG pDevConfig);
BOOL       WINAPI RnaFreeDevConfig(PDEVCONFIG pDevConfig);

DWORD      WINAPI RnaSMMInfoDialog(HWND hWnd, LPSTR szEntryName,
                                   LPSTR szDeviceName,
                                   LPSMMINFO lpsmmi, DWORD dwUsage);
DWORD      WINAPI RnaEnumerateMacNames (LPSTR szDeviceName, LPBYTE lpBuff,
                                        LPDWORD lpcb);
DWORD      WINAPI RnaEnumerateSMMNames (LPSTR szDeviceName, LPBYTE lpBuff,
                                        LPDWORD lpcb,   DWORD dwType);
DWORD      WINAPI RnaGetDefaultSMMInfo(LPSTR szDeviceName, LPSMMINFO lpsmmi,
                                       BOOL fClient);
DWORD      WINAPI RnaGetIPInfo(LPSTR szEntryName, PIPDATA pIpData,
                               BOOL fDefault);
DWORD      WINAPI RnaSetIPInfo(LPSTR szEntryName, PIPDATA pIpData);


DWORD      WINAPI RnaLogon(HWND);
DWORD      WINAPI RnaActivateEngine();
DWORD      WINAPI RnaDeactivateEngine();
DWORD      WINAPI RnaUIDial(HWND, LPSTR);
DWORD      WINAPI RnaImplicitDial(HWND, LPSTR);

DWORD      WINAPI RnaFindDriver(HWND hwnd, LPSTR lpszDriverList);
DWORD      WINAPI RnaInstallDriver (HWND hwnd, LPSTR lpszDriverList);

DWORD      WINAPI RnaExportEntry (LPSTR szEntryName, LPSTR szFileName);
DWORD      WINAPI RnaImportEntry (LPSTR szFileName, LPBYTE lpBuff, DWORD cb);
DWORD      WINAPI RnaValidateImportEntry (LPSTR szFileName);

DWORD      WINAPI RnaGetMultiLinkInfo (LPSTR, LPMLINFO);
DWORD      WINAPI RnaSetMultiLinkInfo (LPSTR, LPMLINFO);
DWORD      WINAPI RnaGetSubEntry (LPSTR, DWORD, PSUBCONNENTRY, LPDWORD);
DWORD      WINAPI RnaSetSubEntry (LPSTR, DWORD, PSUBCONNENTRY, DWORD);

#define    LOG_INACTIVE            0
#define    LOG_DISCONNECTION       1
#define    LOG_ACTIVE              2
#define    LOG_LISTEN              3
#define    LOG_AUTH_TIMEOUT        4
#define    LOG_CALLBACK            5
#define    LOG_ANSWERED            6
#define    LOG_ERROR               7
    
typedef struct tagLOGINFO {
    LPSTR   szPortName;
    HWND    hwnd;
    UINT    uLogEvent;
} LOGINFO, *PLOGINFO, FAR* LPLOGINFO;

typedef struct tagGWLOGONINFO {
    LPSTR   szPortName;
    char    szUserName[UNLEN+1];
    SYSTEMTIME sysTime;
} GWLOGONINFO, *PGWLOGONINFO, FAR* LPGWLOGONINFO;

typedef enum tagRNASECURITY { RNAPASSWORD, RNAPASSTHROUGH } RNASECURITY;

#define RNAADMIN_DIALIN 1

#define	USERTYPE_USER		0x00000001
#define USERTYPE_GROUP		0x00000002
#define	USERTYPE_WORLD		0x00000004

typedef struct tagUSER_ACCESS {
        RNAACCESSTYPE accesstype;
        BOOL        fUseCallbacks;
        DWORD       dwUsertype;
} USERACCESS, *PUSERACCESS, FAR *LPUSERACCESS;

typedef struct tagPORTSTATS
{
    char            szPortName[RAS_MaxPortName+1];
    char            szDeviceType[RAS_MaxDeviceType+1];
    BOOL            fAccessEnabled;
    UINT            TimeOut;
    SMMINFO         smmi;
    char            szComment[RAS_MaxComment+1];
    BOOL            AdminAccess;
    RNASECURITY     security;
    union {
      struct        tagPassthru {
        DWORD       cUsers;
        DWORD       dwOffsetUsers; }
                    Passthru;

      struct        tagPassword {
        USERACCESS  shareaccess;
        char        szPassword[PWLEN+1]; }
                    Password;
    };
} RNAPORTSTATS, *PRNAPORTSTATS, FAR *LPRNAPORTSTATS;

DWORD WINAPI SuprvEnumAccessInfo (LPBYTE, LPINT, LPINT);
DWORD WINAPI SuprvGetAccessInfo (LPSTR, LPRNAPORTSTATS, LPDWORD);
DWORD WINAPI SuprvSetAccessInfo (LPRNAPORTSTATS);
DWORD WINAPI SuprvInitialize (LPDWORD);
DWORD WINAPI SuprvDeInitialize();
DWORD WINAPI SuprvGetAdminConfig();

DWORD WINAPI RnaSetCallbackList (DWORD, LPSTR, LPSTR, int);

DWORD WINAPI RnaAllocateLana(HANDLE hConn, LPDWORD lpLana);
DWORD WINAPI RnaDeallocateLana(HANDLE hConn);

DWORD WINAPI RnaRunScript(HANDLE hConn, PSESS_CONFIGURATION_INFO lpSCI,
                          BOOL fForce);

typedef DWORD (WINAPI * LPFNCREATE)(HWND);
typedef DWORD (WINAPI * LPFNEDIT)(HWND, LPSTR);
typedef void  (WINAPI * LPFNUINOTIFY)(LPSTR);

DWORD WINAPI Remote_CreateEntry (HWND);
DWORD WINAPI Remote_EditEntry (HWND, LPSTR);
void  WINAPI Remote_Notify (LPSTR);

//***************************************************************************
// Dial global setting for redial and implicit connection
//***************************************************************************

#define MAX_REDIAL_COUNT        100
#define MIN_REDIAL_COUNT        1
#define MAX_REDIAL_MINUTE       119
#define MIN_REDIAL_MINUTE       0
#define MAX_REDIAL_SECOND       59
#define MIN_REDIAL_SECOND       0

typedef struct tagRnaSetting    {
    BOOL    fRedial;
    DWORD   cRetry;
    DWORD   dwMin;
    DWORD   dwSec;
    BOOL    fImplicit;
    DWORD   dwDialUI;
} RNASETTING, *PRNASETTING, FAR *LPRNASETTING;

#define DIALUI_NO_PROMPT        0x00000001  // Do not display connect prompt
#define DIALUI_NO_CONFIRM       0x00000002  // Do not display connect confirm
#define DIALUI_NO_TRAY          0x00000004  // No tray icon

DWORD WINAPI RnaGetDialSettings(LPRNASETTING);
DWORD WINAPI RnaSetDialSettings(LPRNASETTING);

//***************************************************************************
// The following are SMM Interface we cut from version 1.0
//***************************************************************************

// Callback request information
//
typedef struct tagCALLBACKINFO
{
    DWORD            dwSize;
    BOOL             fUseCallbackDelay;
    DWORD            dwCallbackDelay;
    char             szPhoneNumber[RAS_MaxPhoneNumber+1];
} CALLBACKINFO, *PCALLBACKINFO, FAR* LPCALLBACKINFO;

DWORD WINAPI RnaSetCallbackInfo( HANDLE hConn, LPCALLBACKINFO lpcbinfo );

DWORD WINAPI RnaReportLinkSpeed( HANDLE hConn, DWORD dwLinkSpeed );
DWORD WINAPI RnaUIStatus( HANDLE hConn, LPSTR lpszStatusMsg );
DWORD WINAPI RnaLog( HANDLE hConn, LPSTR lpszLogMsg );

//***************************************************************************
// The following are connection statistics
//***************************************************************************

#define NUM_RAS_SERIAL_STATS    14

#define BYTES_XMITED            0       //Generic Stats
#define BYTES_RCVED             1
#define FRAMES_XMITED           2
#define FRAMES_RCVED            3

#define CRC_ERR                 4       //Serial Stats
#define TIMEOUT_ERR             5
#define ALIGNMENT_ERR           6
#define SERIAL_OVERRUN_ERR      7
#define FRAMING_ERR             8
#define BUFFER_OVERRUN_ERR      9

#define BYTES_XMITED_UNCOMP     10      //Compression Stats
#define BYTES_RCVED_UNCOMP      11
#define BYTES_XMITED_COMP       12
#define BYTES_RCVED_COMP        13

typedef struct  tagRAS_STATISTICS
{
    DWORD   S_NumOfStatistics ;
    DWORD   S_Statistics[NUM_RAS_SERIAL_STATS] ;
} RAS_STATISTICS, FAR* LPRAS_STATISTICS;

//**************************************************************************
//  Script support
//**************************************************************************

// Script processing mode
//
#define NORMAL_MODE         0
#define TEST_MODE           1

typedef DWORD (WINAPI * PFN_RUN_SCRIPT)(HANDLE, LPSESS_CONFIGURATION_INFO);

#endif // _RNAP_H_
