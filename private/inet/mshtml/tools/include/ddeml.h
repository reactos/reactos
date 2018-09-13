/*++ BUILD Version: 0001 // Increment this if a change has global effects --*/

/*****************************************************************************\
*                                                                             *
*  ddeml.h -    DDEML API header file                                         *
*                                                                             *
*               Version 3.10                                                  *
*                                                                             *
*               Copyright (c) 1993-1996, Microsoft Corp.  All rights reserved.  *
*                                                                             *
\*****************************************************************************/
#ifndef _INC_DDEMLH
#define _INC_DDEMLH

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/******** public types ********/

DECLARE_HANDLE(HCONVLIST);
DECLARE_HANDLE(HCONV);
DECLARE_HANDLE(HSZ);
DECLARE_HANDLE(HDDEDATA);
#define EXPENTRY        CALLBACK

/* the following structure is for use with XTYP_WILDCONNECT processing. */

typedef struct tagHSZPAIR {
    HSZ hszSvc;
    HSZ hszTopic;
} HSZPAIR;
typedef HSZPAIR FAR *PHSZPAIR;

/* The following structure is used by DdeConnect() and DdeConnectList() and
   by XTYP_CONNECT and XTYP_WILDCONNECT callbacks. */

typedef struct tagCONVCONTEXT {
    UINT        cb;             /* set to sizeof(CONVCONTEXT) */
    UINT        wFlags;         /* none currently defined. */
    UINT        wCountryID;     /* country code for topic/item strings used. */
    int         iCodePage;      /* codepage used for topic/item strings. */
    DWORD       dwLangID;       /* language ID for topic/item strings. */
    DWORD       dwSecurity;     /* Private security code. */
    SECURITY_QUALITY_OF_SERVICE qos;  /* client side's quality of service */
} CONVCONTEXT;
typedef CONVCONTEXT FAR *PCONVCONTEXT;


/* The following structure is used by DdeQueryConvInfo(): */

typedef struct tagCONVINFO {
    DWORD   cb;            /* sizeof(CONVINFO)  */
    DWORD   hUser;         /* user specified field  */
    HCONV   hConvPartner;  /* hConv on other end or 0 if non-ddemgr partner  */
    HSZ     hszSvcPartner; /* app name of partner if obtainable  */
    HSZ     hszServiceReq; /* AppName requested for connection  */
    HSZ     hszTopic;      /* Topic name for conversation  */
    HSZ     hszItem;       /* transaction item name or NULL if quiescent  */
    UINT    wFmt;          /* transaction format or NULL if quiescent  */
    UINT    wType;         /* XTYP_ for current transaction  */
    UINT    wStatus;       /* ST_ constant for current conversation  */
    UINT    wConvst;       /* XST_ constant for current transaction  */
    UINT    wLastError;    /* last transaction error.  */
    HCONVLIST hConvList;   /* parent hConvList if this conversation is in a list */
    CONVCONTEXT ConvCtxt;  /* conversation context */
    HWND    hwnd;          /* window handle for this conversation */
    HWND    hwndPartner;   /* partner window handle for this conversation */
} CONVINFO;
typedef CONVINFO FAR *PCONVINFO;

/***** conversation states (usState) *****/

#define     XST_NULL              0  /* quiescent states */
#define     XST_INCOMPLETE        1
#define     XST_CONNECTED         2
#define     XST_INIT1             3  /* mid-initiation states */
#define     XST_INIT2             4
#define     XST_REQSENT           5  /* active conversation states */
#define     XST_DATARCVD          6
#define     XST_POKESENT          7
#define     XST_POKEACKRCVD       8
#define     XST_EXECSENT          9
#define     XST_EXECACKRCVD      10
#define     XST_ADVSENT          11
#define     XST_UNADVSENT        12
#define     XST_ADVACKRCVD       13
#define     XST_UNADVACKRCVD     14
#define     XST_ADVDATASENT      15
#define     XST_ADVDATAACKRCVD   16

/* used in LOWORD(dwData1) of XTYP_ADVREQ callbacks... */
#define     CADV_LATEACK         0xFFFF

/***** conversation status bits (fsStatus) *****/

#define     ST_CONNECTED            0x0001
#define     ST_ADVISE               0x0002
#define     ST_ISLOCAL              0x0004
#define     ST_BLOCKED              0x0008
#define     ST_CLIENT               0x0010
#define     ST_TERMINATED           0x0020
#define     ST_INLIST               0x0040
#define     ST_BLOCKNEXT            0x0080
#define     ST_ISSELF               0x0100


/* DDE constants for wStatus field */

#define DDE_FACK                0x8000
#define DDE_FBUSY               0x4000
#define DDE_FDEFERUPD           0x4000
#define DDE_FACKREQ             0x8000
#define DDE_FRELEASE            0x2000
#define DDE_FREQUESTED          0x1000
#define DDE_FAPPSTATUS          0x00ff
#define DDE_FNOTPROCESSED       0x0000

#define DDE_FACKRESERVED        (~(DDE_FACK | DDE_FBUSY | DDE_FAPPSTATUS))
#define DDE_FADVRESERVED        (~(DDE_FACKREQ | DDE_FDEFERUPD))
#define DDE_FDATRESERVED        (~(DDE_FACKREQ | DDE_FRELEASE | DDE_FREQUESTED))
#define DDE_FPOKRESERVED        (~(DDE_FRELEASE))

/***** message filter hook types *****/

#define     MSGF_DDEMGR             0x8001

/***** codepage constants ****/

#define CP_WINANSI      1004    /* default codepage for windows & old DDE convs. */
#define CP_WINUNICODE   1200
#ifdef UNICODE
#define CP_WINNEUTRAL   CP_WINUNICODE
#else  // !UNICODE
#define CP_WINNEUTRAL   CP_WINANSI
#endif // !UNICODE

/***** transaction types *****/

#define     XTYPF_NOBLOCK            0x0002  /* CBR_BLOCK will not work */
#define     XTYPF_NODATA             0x0004  /* DDE_FDEFERUPD */
#define     XTYPF_ACKREQ             0x0008  /* DDE_FACKREQ */

#define     XCLASS_MASK              0xFC00
#define     XCLASS_BOOL              0x1000
#define     XCLASS_DATA              0x2000
#define     XCLASS_FLAGS             0x4000
#define     XCLASS_NOTIFICATION      0x8000

#define     XTYP_ERROR              (0x0000 | XCLASS_NOTIFICATION | XTYPF_NOBLOCK )
#define     XTYP_ADVDATA            (0x0010 | XCLASS_FLAGS         )
#define     XTYP_ADVREQ             (0x0020 | XCLASS_DATA | XTYPF_NOBLOCK )
#define     XTYP_ADVSTART           (0x0030 | XCLASS_BOOL          )
#define     XTYP_ADVSTOP            (0x0040 | XCLASS_NOTIFICATION)
#define     XTYP_EXECUTE            (0x0050 | XCLASS_FLAGS         )
#define     XTYP_CONNECT            (0x0060 | XCLASS_BOOL | XTYPF_NOBLOCK)
#define     XTYP_CONNECT_CONFIRM    (0x0070 | XCLASS_NOTIFICATION | XTYPF_NOBLOCK)
#define     XTYP_XACT_COMPLETE      (0x0080 | XCLASS_NOTIFICATION  )
#define     XTYP_POKE               (0x0090 | XCLASS_FLAGS         )
#define     XTYP_REGISTER           (0x00A0 | XCLASS_NOTIFICATION | XTYPF_NOBLOCK)
#define     XTYP_REQUEST            (0x00B0 | XCLASS_DATA          )
#define     XTYP_DISCONNECT         (0x00C0 | XCLASS_NOTIFICATION | XTYPF_NOBLOCK)
#define     XTYP_UNREGISTER         (0x00D0 | XCLASS_NOTIFICATION | XTYPF_NOBLOCK)
#define     XTYP_WILDCONNECT        (0x00E0 | XCLASS_DATA | XTYPF_NOBLOCK)

#define     XTYP_MASK                0x00F0
#define     XTYP_SHIFT               4  /* shift to turn XTYP_ into an index */

/***** Timeout constants *****/

#define     TIMEOUT_ASYNC           0xFFFFFFFF

/***** Transaction ID constants *****/

#define     QID_SYNC                0xFFFFFFFF

/****** public strings used in DDE ******/

#ifdef UNICODE
#define SZDDESYS_TOPIC         L"System"
#define SZDDESYS_ITEM_TOPICS   L"Topics"
#define SZDDESYS_ITEM_SYSITEMS L"SysItems"
#define SZDDESYS_ITEM_RTNMSG   L"ReturnMessage"
#define SZDDESYS_ITEM_STATUS   L"Status"
#define SZDDESYS_ITEM_FORMATS  L"Formats"
#define SZDDESYS_ITEM_HELP     L"Help"
#define SZDDE_ITEM_ITEMLIST    L"TopicItemList"
#else
#define SZDDESYS_TOPIC         "System"
#define SZDDESYS_ITEM_TOPICS   "Topics"
#define SZDDESYS_ITEM_SYSITEMS "SysItems"
#define SZDDESYS_ITEM_RTNMSG   "ReturnMessage"
#define SZDDESYS_ITEM_STATUS   "Status"
#define SZDDESYS_ITEM_FORMATS  "Formats"
#define SZDDESYS_ITEM_HELP     "Help"
#define SZDDE_ITEM_ITEMLIST    "TopicItemList"
#endif


/****** API entry points ******/

typedef HDDEDATA CALLBACK FNCALLBACK(UINT wType, UINT wFmt, HCONV hConv,
        HSZ hsz1, HSZ hsz2, HDDEDATA hData, DWORD dwData1, DWORD dwData2);
typedef HDDEDATA (CALLBACK *PFNCALLBACK)(UINT wType, UINT wFmt, HCONV hConv,
        HSZ hsz1, HSZ hsz2, HDDEDATA hData, DWORD dwData1, DWORD dwData2);

#define     CBR_BLOCK           ((HDDEDATA)0xffffffffL)

/* DLL registration functions */

UINT WINAPI DdeInitializeA(LPDWORD pidInst, PFNCALLBACK pfnCallback,
        DWORD afCmd, DWORD ulRes);
UINT WINAPI DdeInitializeW(LPDWORD pidInst, PFNCALLBACK pfnCallback,
        DWORD afCmd, DWORD ulRes);
#ifdef UNICODE
#define DdeInitialize  DdeInitializeW
#else
#define DdeInitialize  DdeInitializeA
#endif // !UNICODE

/*
 * Callback filter flags for use with standard apps.
 */

#define     CBF_FAIL_SELFCONNECTIONS     0x00001000
#define     CBF_FAIL_CONNECTIONS         0x00002000
#define     CBF_FAIL_ADVISES             0x00004000
#define     CBF_FAIL_EXECUTES            0x00008000
#define     CBF_FAIL_POKES               0x00010000
#define     CBF_FAIL_REQUESTS            0x00020000
#define     CBF_FAIL_ALLSVRXACTIONS      0x0003f000

#define     CBF_SKIP_CONNECT_CONFIRMS    0x00040000
#define     CBF_SKIP_REGISTRATIONS       0x00080000
#define     CBF_SKIP_UNREGISTRATIONS     0x00100000
#define     CBF_SKIP_DISCONNECTS         0x00200000
#define     CBF_SKIP_ALLNOTIFICATIONS    0x003c0000

/*
 * Application command flags
 */
#define     APPCMD_CLIENTONLY            0x00000010L
#define     APPCMD_FILTERINITS           0x00000020L
#define     APPCMD_MASK                  0x00000FF0L

/*
 * Application classification flags
 */
#define     APPCLASS_STANDARD            0x00000000L
#define     APPCLASS_MASK                0x0000000FL



BOOL WINAPI DdeUninitialize(DWORD idInst);

/*
 * conversation enumeration functions
 */

HCONVLIST WINAPI DdeConnectList(DWORD idInst, HSZ hszService, HSZ hszTopic,
        HCONVLIST hConvList, PCONVCONTEXT pCC);
HCONV WINAPI DdeQueryNextServer(HCONVLIST hConvList, HCONV hConvPrev);
BOOL WINAPI DdeDisconnectList(HCONVLIST hConvList);

/*
 * conversation control functions
 */

HCONV WINAPI DdeConnect(DWORD idInst, HSZ hszService, HSZ hszTopic,
        PCONVCONTEXT pCC);
BOOL WINAPI DdeDisconnect(HCONV hConv);
HCONV WINAPI DdeReconnect(HCONV hConv);
UINT WINAPI DdeQueryConvInfo(HCONV hConv, DWORD idTransaction, PCONVINFO pConvInfo);
BOOL WINAPI DdeSetUserHandle(HCONV hConv, DWORD id, DWORD hUser);
BOOL WINAPI DdeAbandonTransaction(DWORD idInst, HCONV hConv, DWORD idTransaction);


/*
 * app server interface functions
 */

BOOL WINAPI DdePostAdvise(DWORD idInst, HSZ hszTopic, HSZ hszItem);
BOOL WINAPI DdeEnableCallback(DWORD idInst, HCONV hConv, UINT wCmd);
BOOL WINAPI DdeImpersonateClient(HCONV hConv);

#define EC_ENABLEALL            0
#define EC_ENABLEONE            ST_BLOCKNEXT
#define EC_DISABLE              ST_BLOCKED
#define EC_QUERYWAITING         2


HDDEDATA WINAPI DdeNameService(DWORD idInst, HSZ hsz1, HSZ hsz2, UINT afCmd);

#define DNS_REGISTER        0x0001
#define DNS_UNREGISTER      0x0002
#define DNS_FILTERON        0x0004
#define DNS_FILTEROFF       0x0008

/*
 * app client interface functions
 */

HDDEDATA WINAPI DdeClientTransaction(LPBYTE pData, DWORD cbData,
        HCONV hConv, HSZ hszItem, UINT wFmt, UINT wType,
        DWORD dwTimeout, LPDWORD pdwResult);

/*
 *data transfer functions
 */

HDDEDATA WINAPI DdeCreateDataHandle(DWORD idInst, LPBYTE pSrc, DWORD cb,
        DWORD cbOff, HSZ hszItem, UINT wFmt, UINT afCmd);
HDDEDATA WINAPI DdeAddData(HDDEDATA hData, LPBYTE pSrc, DWORD cb, DWORD cbOff);
DWORD WINAPI DdeGetData(HDDEDATA hData, LPBYTE pDst, DWORD cbMax, DWORD cbOff);
LPBYTE WINAPI DdeAccessData(HDDEDATA hData, LPDWORD pcbDataSize);
BOOL WINAPI DdeUnaccessData(HDDEDATA hData);
BOOL WINAPI DdeFreeDataHandle(HDDEDATA hData);

#define     HDATA_APPOWNED          0x0001


UINT WINAPI DdeGetLastError(DWORD idInst);

#define     DMLERR_NO_ERROR                    0       /* must be 0 */

#define     DMLERR_FIRST                       0x4000

#define     DMLERR_ADVACKTIMEOUT               0x4000
#define     DMLERR_BUSY                        0x4001
#define     DMLERR_DATAACKTIMEOUT              0x4002
#define     DMLERR_DLL_NOT_INITIALIZED         0x4003
#define     DMLERR_DLL_USAGE                   0x4004
#define     DMLERR_EXECACKTIMEOUT              0x4005
#define     DMLERR_INVALIDPARAMETER            0x4006
#define     DMLERR_LOW_MEMORY                  0x4007
#define     DMLERR_MEMORY_ERROR                0x4008
#define     DMLERR_NOTPROCESSED                0x4009
#define     DMLERR_NO_CONV_ESTABLISHED         0x400a
#define     DMLERR_POKEACKTIMEOUT              0x400b
#define     DMLERR_POSTMSG_FAILED              0x400c
#define     DMLERR_REENTRANCY                  0x400d
#define     DMLERR_SERVER_DIED                 0x400e
#define     DMLERR_SYS_ERROR                   0x400f
#define     DMLERR_UNADVACKTIMEOUT             0x4010
#define     DMLERR_UNFOUND_QUEUE_ID            0x4011

#define     DMLERR_LAST                        0x4011

HSZ  WINAPI DdeCreateStringHandleA(DWORD idInst, LPCSTR psz, int iCodePage);
HSZ  WINAPI DdeCreateStringHandleW(DWORD idInst, LPCWSTR psz, int iCodePage);
#ifdef UNICODE
#define DdeCreateStringHandle  DdeCreateStringHandleW
#else
#define DdeCreateStringHandle  DdeCreateStringHandleA
#endif // !UNICODE
DWORD WINAPI DdeQueryStringA(DWORD idInst, HSZ hsz, LPSTR psz, DWORD cchMax, int iCodePage);
DWORD WINAPI DdeQueryStringW(DWORD idInst, HSZ hsz, LPWSTR psz, DWORD cchMax, int iCodePage);
#ifdef UNICODE
#define DdeQueryString  DdeQueryStringW
#else
#define DdeQueryString  DdeQueryStringA
#endif // !UNICODE
BOOL WINAPI DdeFreeStringHandle(DWORD idInst, HSZ hsz);
BOOL WINAPI DdeKeepStringHandle(DWORD idInst, HSZ hsz);
int WINAPI DdeCmpStringHandles(HSZ hsz1, HSZ hsz2);


#ifndef NODDEMLSPY
/*
 * DDEML public debugging header file info
 */

typedef struct tagDDEML_MSG_HOOK_DATA {    // new for NT
    UINT uiLo;      // unpacked lo and hi parts of lParam
    UINT uiHi;
    DWORD cbData;   // amount of data in message, if any. May be > than 32 bytes.
    DWORD Data[8];  // data peeking by DDESPY is limited to 32 bytes.
} DDEML_MSG_HOOK_DATA, *PDDEML_MSG_HOOK_DATA;


typedef struct tagMONMSGSTRUCT {
    UINT    cb;
    HWND    hwndTo;
    DWORD   dwTime;
    HANDLE  hTask;
    UINT    wMsg;
    WPARAM  wParam;
    LPARAM  lParam;
    DDEML_MSG_HOOK_DATA dmhd;       // new for NT
} MONMSGSTRUCT, *PMONMSGSTRUCT;

typedef struct tagMONCBSTRUCT {
    UINT   cb;
    DWORD  dwTime;
    HANDLE hTask;
    DWORD  dwRet;
    UINT   wType;
    UINT   wFmt;
    HCONV  hConv;
    HSZ    hsz1;
    HSZ    hsz2;
    HDDEDATA hData;
    DWORD  dwData1;
    DWORD  dwData2;
    CONVCONTEXT cc;                 // new for NT for XTYP_CONNECT callbacks
    DWORD  cbData;                  // new for NT for data peeking
    DWORD  Data[8];                 // new for NT for data peeking
} MONCBSTRUCT, *PMONCBSTRUCT;

typedef struct tagMONHSZSTRUCTA {
    UINT   cb;
    BOOL   fsAction;    /* MH_ value */
    DWORD  dwTime;
    HSZ    hsz;
    HANDLE hTask;
    CHAR    str[1];
} MONHSZSTRUCTA, *PMONHSZSTRUCTA;
typedef struct tagMONHSZSTRUCTW {
    UINT   cb;
    BOOL   fsAction;    /* MH_ value */
    DWORD  dwTime;
    HSZ    hsz;
    HANDLE hTask;
    WCHAR   str[1];
} MONHSZSTRUCTW, *PMONHSZSTRUCTW;
#ifdef UNICODE
typedef MONHSZSTRUCTW MONHSZSTRUCT;
typedef PMONHSZSTRUCTW PMONHSZSTRUCT;
#else
typedef MONHSZSTRUCTA MONHSZSTRUCT;
typedef PMONHSZSTRUCTA PMONHSZSTRUCT;
#endif // UNICODE

#define MH_CREATE   1
#define MH_KEEP     2
#define MH_DELETE   3
#define MH_CLEANUP  4

typedef struct tagMONERRSTRUCT {
    UINT    cb;
    UINT    wLastError;
    DWORD   dwTime;
    HANDLE  hTask;
} MONERRSTRUCT, *PMONERRSTRUCT;

typedef struct tagMONLINKSTRUCT {
    UINT    cb;
    DWORD   dwTime;
    HANDLE  hTask;
    BOOL    fEstablished;
    BOOL    fNoData;
    HSZ     hszSvc;
    HSZ     hszTopic;
    HSZ     hszItem;
    UINT    wFmt;
    BOOL    fServer;
    HCONV   hConvServer;
    HCONV   hConvClient;
} MONLINKSTRUCT, *PMONLINKSTRUCT;

typedef struct tagMONCONVSTRUCT {
    UINT    cb;
    BOOL    fConnect;
    DWORD   dwTime;
    HANDLE  hTask;
    HSZ     hszSvc;
    HSZ     hszTopic;
    HCONV   hConvClient;        // Globally unique value != apps local hConv
    HCONV   hConvServer;        // Globally unique value != apps local hConv
} MONCONVSTRUCT, *PMONCONVSTRUCT;

#define     MAX_MONITORS            4
#define     APPCLASS_MONITOR        0x00000001L
#define     XTYP_MONITOR            (0x00F0 | XCLASS_NOTIFICATION | XTYPF_NOBLOCK)

/*
 * Callback filter flags for use with MONITOR apps - 0 implies no monitor
 * callbacks.
 */
#define     MF_HSZ_INFO                  0x01000000
#define     MF_SENDMSGS                  0x02000000
#define     MF_POSTMSGS                  0x04000000
#define     MF_CALLBACKS                 0x08000000
#define     MF_ERRORS                    0x10000000
#define     MF_LINKS                     0x20000000
#define     MF_CONV                      0x40000000

#define     MF_MASK                      0xFF000000
#endif /* NODDEMLSPY */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* _INC_DDEMLH */


