
typedef unsigned long HDDER;
typedef unsigned long HIPC;

/*
    D D E P K T
        
        DDEPKT is the unit of "message" in the netdde environment.
        Each DDEPKT contains the information pertaining to one DDE message.
 */
typedef struct ddepkt {
    DWORD               dp_size;        /* size of DDEPKT including this structure */
    struct ddepkt FAR  *dp_prev;        /* previous pointer */
    struct ddepkt FAR  *dp_next;        /* next pointer */
    DWORD               dp_hDstDder;    /* handle to destination DDER */
    DWORD               dp_hDstRouter;  /* handle to destination Router */
    DWORD               dp_routerCmd;   /* command for final Router */
} DDEPKT;
typedef DDEPKT FAR *LPDDEPKT;



#define SZ_NDDEAGNT_SERVICE    TEXT("NDDEAgnt")
#define SZ_NDDEAGNT_TOPIC      TEXT("Start NetDDE Services")
#define SZ_NDDEAGNT_TITLE      TEXT("NetDDE Agent")
#define SZ_NDDEAGNT_CLASS      TEXT("NDDEAgnt")

#define NETDDE_PIPE            TEXT("\\\\.\\pipe\\NetDDE")

#define START_NETDDE_SERVICES(hwnd)                                 \
{                                                                   \
    ATOM aService, aTopic;                                          \
                                                                    \
    aService = GlobalAddAtom(SZ_NDDEAGNT_SERVICE);                  \
    aTopic = GlobalAddAtom(SZ_NDDEAGNT_TOPIC);                      \
    SendMessage(FindWindow(SZ_NDDEAGNT_CLASS, SZ_NDDEAGNT_TITLE),   \
            WM_DDE_INITIATE,                                        \
            (WPARAM)hwnd, MAKELPARAM(aService, aTopic));            \
    GlobalDeleteAtom(aService);                                     \
    GlobalDeleteAtom(aTopic);                                       \
}

typedef struct {
    DWORD dwOffsetDesktop;
    WCHAR awchNames[64];
} NETDDE_PIPE_MESSAGE, *PNETDDE_PIPE_MESSAGE;


typedef struct _THREADDATA {
    struct _THREADDATA  *ptdNext;
    HWINSTA             hwinsta;
    HDESK               hdesk;
    HWND                hwndDDE;
    HWND                hwndDDEAgent;
    HANDLE              heventReady;
    DWORD               dwThreadId;
    BOOL                bInitiating;
} THREADDATA, *PTHREADDATA;

typedef struct _IPCINIT {
    HDDER       hDder;
    LPDDEPKT    lpDdePkt;
    BOOL        bStartApp;
    LPSTR       lpszCmdLine;
    WORD        dd_type;
} IPCINIT, *PIPCINIT;

typedef struct _IPCXMIT {
    HIPC        hIpc;
    HDDER       hDder;
    LPDDEPKT    lpDdePkt;
} IPCXMIT, *PIPCXMIT;

extern PTHREADDATA ptdHead;

extern UINT    wMsgIpcXmit;
extern UINT    wMsgDoTerminate;

extern DWORD tlsThreadData;


// resources

#define MAXFONT                 10
#define IDC_NDDEAGNT            21
#define IDD_ABOUT               1
#define IDM_NDDEAGNT            20
#define IDM_EXIT                200
#define IDM_ABOUT               205
#define CI_VERSION              206

// function prototypes

LRESULT APIENTRY    NDDEAgntWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL    APIENTRY    About(HWND, UINT, WPARAM, LONG);
int                 NDDEAgntInit(VOID);
VOID                CleanupTrustedShares(void);
BOOL                HandleNetddeCopyData(HWND, WPARAM, PCOPYDATASTRUCT);

/*
    NetDDE will fill in the following structure and pass it to NetDDE
    Agent whenever it wants to have an app started in the user's
    context.  The reason for the sharename and modifyId is to that
    a user must explicitly permit NetDDE to start an app on behalf of
    other users.
 */

#define NDDEAGT_CMD_REV         1
#define NDDEAGT_CMD_MAGIC       0xDDE1DDE1

/*      commands        */
#define NDDEAGT_CMD_WINEXEC     0x1
#define NDDEAGT_CMD_WININIT     0x2

/*      return status   */
#define NDDEAGT_START_NO        0x0

#define NDDEAGT_INIT_NO         0x0
#define NDDEAGT_INIT_OK         0x1

typedef struct {
    DWORD       dwMagic;        // must be NDDEAGT_CMD_MAGIC
    DWORD       dwRev;          // must be 1
    DWORD       dwCmd;          // one of above NDDEAGT_CMD_*
    DWORD       qwModifyId[2];  // modify Id of the share
    UINT        fuCmdShow;      // fuCmdShow to use with WinExec()
    char        szData[1];      // sharename\0 cmdline\0
} NDDEAGTCMD;
typedef NDDEAGTCMD *PNDDEAGTCMD;
