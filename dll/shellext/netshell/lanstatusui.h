
/// CLSID
/// HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{7007ACCF-3202-11D1-AAD2-00805FC1270E}
// IID B722BCCB-4E68-101B-A2BC-00AA00404770

#define WM_SHOWSTATUSDLG    (WM_USER+10)

typedef struct tagNotificationItem
{
    struct tagNotificationItem *pNext;
    CLSID guidItem;
    UINT uID;
    HWND hwndDlg;
    INetConnection *pNet;
} NOTIFICATION_ITEM;

typedef struct
{
    INetConnection *pNet;
    HWND hwndStatusDlg;         /* LanStatusDlg window */
    HWND hwndDlg;               /* status dialog window */
    DWORD dwAdapterIndex;
    UINT_PTR nIDEvent;
    UINT DHCPEnabled;
    DWORD dwInOctets;
    DWORD dwOutOctets;
    DWORD IpAddress;
    DWORD SubnetMask;
    DWORD Gateway;
    UINT uID;
    UINT Status;
} LANSTATUSUI_CONTEXT;

class CLanStatus:
    public CComCoClass<CLanStatus, &CLSID_ConnectionTray>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IOleCommandTarget
{
    public:
        CLanStatus();

        // IOleCommandTarget
        virtual HRESULT WINAPI QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD *prgCmds, OLECMDTEXT *pCmdText);
        virtual HRESULT WINAPI Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    private:
        HRESULT InitializeNetTaskbarNotifications();
        HRESULT ShowStatusDialogByCLSID(const GUID *pguidCmdGroup);

        CComPtr<INetConnectionManager> m_lpNetMan;
        NOTIFICATION_ITEM *m_pHead;

    public:
        DECLARE_NO_REGISTRY()
        DECLARE_CENTRAL_INSTANCE_NOT_AGGREGATABLE(CLanStatus)
        DECLARE_PROTECT_FINAL_CONSTRUCT()

        BEGIN_COM_MAP(CLanStatus)
            COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        END_COM_MAP()

};
