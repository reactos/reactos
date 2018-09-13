#define UA_STATUS_IDLE      0
#define UA_STATUS_UPDATEALL 1
#define UA_STATUS_MULTIPLE  2
#define UPDATEALL_PENDING   3
#define MULTIPLE_PENDING    4
#define U_STATUS_ABORTED    5

const UM_ONREQUEST    = WM_USER + 4;
const UM_ONSKIP       = WM_USER + 5;
const UM_ONABORT      = WM_USER + 6;
const UM_ONSKIPSINGLE = WM_USER + 7;
const UM_ONADDSINGLE  = WM_USER + 8;
//  At CleanUp msg we call CleanUp()/Release().
const UM_CLEANUP      = WM_USER + 3;
//  At Ready msg we call AddRef().
const UM_READY        = WM_USER + 10;
const UM_BACKGROUND   = WM_USER + 11;
const UM_DECREASE     = WM_USER + 12;
const UM_BEGINREPORT  = WM_USER + 13;
const UM_ENDREPORT    = WM_USER + 14;
//  Note: This is redefined in urlmon\download
const UM_NEEDREBOOT   = WM_USER + 16;

//  BUGBUG
//  Who is sending us WM_USER + 9?

#define IS_UPDATING(l) (((l)==UA_STATUS_UPDATEALL) || ((l)==UA_STATUS_MULTIPLE))
#define IS_PENDING(l)  (((l)==UPDATEALL_PENDING) || ((l)==MULTIPLE_PENDING))
#define IS_IDLE(l)     ((l)==UA_STATUS_IDLE)

#define DIALER_OFFLINE      0
#define DIALER_ONLINE       1
#define DIALER_CONNECTING   2

#define IDD_START   IDD_RESET

typedef enum    {
    ITEM_STAT_IDLE,
    ITEM_STAT_PENDING,
    ITEM_STAT_SKIPPED,
    ITEM_STAT_FAILED,
    ITEM_STAT_UPDATING,
    ITEM_STAT_SUCCEEDED,
    ITEM_STAT_ABORTED,
    ITEM_STAT_QUEUED,
    E_ATTEMPT_FAILED,
    E_CONNECTION_LOST
} STATUS, CONNECT_ERROR;

#define MAX_COLUMN      16

#define CHECKBOX_NONE   16384
#define CHECKBOX_OFF    4096
#define CHECKBOX_ON     8192

class CDialHelper;
class CUpdateAgent;
class CUpdateController;

struct CookieItemMapEntry   {
    CLSID   _cookie;
    LPARAM  _id;
};

class CCookieItemMap    {
private:
    CookieItemMapEntry* _map;
    UINT                _count;
    UINT                _capacity;
    LPARAM              _lParamNext;
public:
    CCookieItemMap();
    ~CCookieItemMap();
    
    STDMETHODIMP    Init(UINT size = 0);
    STDMETHODIMP    DelCookie(CLSID *);
    STDMETHODIMP    FindLParam(CLSID *, LPARAM *);
    STDMETHODIMP    FindCookie(LPARAM, CLSID *);
    STDMETHODIMP    AddCookie(CLSID *, LPARAM *);
    STDMETHODIMP    ResetMap();
};

extern DWORD WINAPI UpdateThreadProc(LPVOID);
extern BOOL CALLBACK UpdateDlgProc(HWND hDlg, UINT iMsg, WPARAM, LPARAM);
extern BOOL ListView_OnNotify(HWND hDlg, NM_LISTVIEW *, CUpdateController *);

class CUpdateDialog
{
friend BOOL CALLBACK UpdateDlgProc(HWND, UINT, WPARAM, LPARAM);
friend DWORD WINAPI UpdateThreadProc(LPVOID);
public:
    CUpdateDialog();
    ~CUpdateDialog();

    // Helper APIs
    //  Get columns set up.
    STDMETHODIMP        Init(HWND hParent, CUpdateController *);
    //  Show/Hide the page.
    STDMETHODIMP        Show(BOOL);
    //  Change the appearance of item accord to new stat and return old stat
    //  if needed.
    STDMETHODIMP        RefreshStatus(CLSID *, LPTSTR, STATUS, LPTSTR = NULL);
    //  Add new item with the status.
    STDMETHODIMP        AddItem(CLSID *, LPTSTR, STATUS);  
    //  Reset the dialog.
    STDMETHODIMP        ResetDialog(void);
    //  Get a list of cookies that are selected.
    STDMETHODIMP        GetSelectedCookies(CLSID *, UINT *);
    //  Get the count of selected cookies.
    STDMETHODIMP        GetSelectionCount(UINT *);
    //  Get the cookie according to the iItem;
    STDMETHODIMP        IItem2Cookie(const int iItem, CLSID *);
    STDMETHODIMP        CleanUp(void);

    BOOL                SelectFirstUpdatingSubscription();
    DWORD               SetSiteDownloadSize (CLSID *, DWORD);

    HWND    m_hDlg;
    DWORD   m_ThreadID;
    int     colMap[MAX_COLUMN];
    CUpdateController * m_pController;
    static  BOOL    m_bDetail;
    int     m_cDlKBytes;
    int     m_cDlDocs;

    int     m_cxWidget;
    int     m_cyWidget;

private:
    static int CALLBACK SortUpdatingToTop  (LPARAM, LPARAM, LPARAM);
    BOOL    PersistStateToRegistry (HWND hDlg);
    BOOL    GetPersistentStateFromRegistry (struct _PROG_PERSIST_STATE& state, int iCharWidth);

    HWND    m_hLV;
    HWND    m_hParent;
    BOOL    m_bInitialized;
    CCookieItemMap  cookieMap;
};


//----------------------------------------------------------------------------
// Update controller class
//----------------------------------------------------------------------------

typedef struct ReportMapEntry   {
    NOTIFICATIONCOOKIE  startCookie;
    STATUS              status;
    DWORD               progress;
    LPTSTR              name;
    LPTSTR              url;
    SUBSCRIPTIONTYPE    subType;
} * PReportMap;


class CUpdateController : public INotificationSink
{
friend BOOL CALLBACK UpdateDlgProc(HWND, UINT, WPARAM, LPARAM);
friend DWORD WINAPI UpdateThreadProc(LPVOID);
friend BOOL ListView_OnNotify(HWND hDlg, NM_LISTVIEW *, CUpdateController *);
friend class CDialHelper;
private:
    ULONG           m_cRef;         // OLE ref count

    enum            {CUC_ENTRY_INCRE = 32, CUC_MAX_ENTRY = 1024};

    PReportMap      m_aReport;
    UINT            m_cReportCount;
    UINT            m_cReportCapacity;

    INotificationMgr    *m_pNotMgr;
    CUpdateDialog       *m_pDialog;
    CDialHelper         *m_pDialer;

    DWORD           m_ThreadID;     // Update thread ID
    LONG            m_count;        // Current active updates.

    UINT            m_cTotal;
    UINT            m_cFinished;

    BOOL            m_fInit;
    BOOL            m_fSessionEnded;

//  private Helper APIs
    STDMETHODIMP    AddEntry(NOTIFICATIONITEM *, STATUS);
    STDMETHODIMP    DispatchRequest(PReportMap);
    STDMETHODIMP    CancelRequest(PReportMap);
    STDMETHODIMP    IncreaseCount();
    STDMETHODIMP    DecreaseCount(CLSID *);
    STDMETHODIMP    StartPending(void);
    STDMETHODIMP    GetItemList(UINT *);
    STDMETHODIMP    GetLocationOf(CLSID *, LPTSTR, UINT);
    BOOL            IsSkippable(CLSID *);
    STDMETHODIMP    ResyncData();
    STDMETHODIMP    OnBeginReportFromTray(INotification *);
    STDMETHODIMP    OnEndReportFromTray(INotification *);

public:
    CUpdateController();
    ~CUpdateController();

    STDMETHODIMP         QueryInterface(REFIID riid, void **punk);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // INotificationSink members
    STDMETHODIMP         OnNotification(
        LPNOTIFICATION         pNotification,
        LPNOTIFICATIONREPORT   pNotificationReport,
        DWORD                  dwReserved
    );

    //  Helper APIs:
    STDMETHODIMP    StartService(void);
    STDMETHODIMP    StopService(STATUS);
    STDMETHODIMP    CleanUp();
    STDMETHODIMP    OnRequest(INotification *);
    STDMETHODIMP    Skip(void);
    STDMETHODIMP    SkipSingle(CLSID *);
    STDMETHODIMP    AddSingle(CLSID *);
    STDMETHODIMP    Restart(UINT count);
    STDMETHODIMP    Abort(void);
    STDMETHODIMP    Init(CUpdateDialog *);

    PReportMap      FindReportEntry(CLSID *);
    SUBSCRIPTIONTYPE    GetSubscriptionType(CLSID *);
};

class CUpdateAgent
{
protected:
    CUpdateController   *m_pController;     
    CUpdateDialog       *m_pDialog;
    
public:
    DWORD               m_DialogThreadID;
    DWORD               m_ThreadID;
    CUpdateAgent(void);
    ~CUpdateAgent(void);

    // other functions
    STDMETHODIMP        Init(void);
};

class CDialHelper : public INotificationSink
{
    friend class CUpdateController;
protected:
    ULONG               m_cRef;
    CUpdateController   * m_pController;
    INotificationMgr    * m_pNotMgr;
    INotificationReport * m_pConnAgentReport;
    UINT                m_cConnection;
    DWORD               m_ThreadID;
public:
    INT m_iDialerStatus;

public:
    CDialHelper(void);
    ~CDialHelper(void)  {}

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID riid, void **punk);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // INotificationSink members
    STDMETHODIMP         OnNotification(
        LPNOTIFICATION         pNotification,
        LPNOTIFICATIONREPORT   pNotificationReport,
        DWORD                  dwReserved
    );

    STDMETHODIMP        OnInetOnline(INotification *);
    STDMETHODIMP        OnInetOffline(INotification *);
    STDMETHODIMP        NotifyAutoDialer(NOTIFICATIONTYPE);
    STDMETHODIMP        Init(CUpdateController *);
    STDMETHODIMP        CleanUp();
    STDMETHODIMP        HangUp();
    STDMETHODIMP        DialOut();
    BOOL                IsOffline()   {
                            return (DIALER_OFFLINE == m_iDialerStatus);
                        }
    BOOL                IsConnecting()  {
                            return (DIALER_CONNECTING == m_iDialerStatus);
                        }
};
