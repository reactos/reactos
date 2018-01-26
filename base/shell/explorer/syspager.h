#pragma once

struct InternalIconData : NOTIFYICONDATA
{
    // Must keep a separate copy since the original is unioned with uTimeout.
    UINT uVersionCopy;
};

struct IconWatcherData
{
    HANDLE hProcess;
    DWORD ProcessId;
    NOTIFYICONDATA IconData;

    IconWatcherData(CONST NOTIFYICONDATA *iconData) :
        hProcess(NULL), ProcessId(0)
    {
        IconData.cbSize = sizeof(NOTIFYICONDATA);
        IconData.hWnd = iconData->hWnd;
        IconData.uID = iconData->uID;
        IconData.guidItem = iconData->guidItem;
    }

    ~IconWatcherData()
    {
        if (hProcess)
        {
            CloseHandle(hProcess);
        }
    }
};

class CIconWatcher
{
    CAtlList<IconWatcherData *> m_WatcherList;
    CRITICAL_SECTION m_ListLock;
    HANDLE m_hWatcherThread;
    HANDLE m_WakeUpEvent;
    HWND m_hwndSysTray;
    bool m_Loop;

public:
    CIconWatcher();

    virtual ~CIconWatcher();

    bool Initialize(_In_ HWND hWndParent);
    void Uninitialize();

    bool AddIconToWatcher(_In_ CONST NOTIFYICONDATA *iconData);
    bool RemoveIconFromWatcher(_In_ CONST NOTIFYICONDATA *iconData);

    IconWatcherData* GetListEntry(_In_opt_ CONST NOTIFYICONDATA *iconData, _In_opt_ HANDLE hProcess, _In_ bool Remove);

private:

    static UINT WINAPI WatcherThread(_In_opt_ LPVOID lpParam);
};

class CBalloonQueue
{
public:
    static const int TimerInterval = 2000;
    static const int BalloonsTimerId = 1;
    static const int MinTimeout = 10000;
    static const int MaxTimeout = 30000;
    static const int CooldownBetweenBalloons = 2000;

private:
    struct Info
    {
        InternalIconData * pSource;
        WCHAR szInfo[256];
        WCHAR szInfoTitle[64];
        WPARAM uIcon;
        UINT uTimeout;

        Info(InternalIconData * source)
        {
            pSource = source;
            StrNCpy(szInfo, source->szInfo, _countof(szInfo));
            StrNCpy(szInfoTitle, source->szInfoTitle, _countof(szInfoTitle));
            uIcon = source->dwInfoFlags & NIIF_ICON_MASK;
            if (source->dwInfoFlags == NIIF_USER)
                uIcon = reinterpret_cast<WPARAM>(source->hIcon);
            uTimeout = source->uTimeout;
        }
    };

    HWND m_hwndParent;

    CTooltips * m_tooltips;

    CAtlList<Info> m_queue;

    CToolbar<InternalIconData> * m_toolbar;

    InternalIconData * m_current;
    bool m_currentClosed;

    int m_timer;

public:
    CBalloonQueue();

    void Init(HWND hwndParent, CToolbar<InternalIconData> * toolbar, CTooltips * balloons);
    void Deinit();

    bool OnTimer(int timerId);
    void UpdateInfo(InternalIconData * notifyItem);
    void RemoveInfo(InternalIconData * notifyItem);
    void CloseCurrent();

private:

    int IndexOf(InternalIconData * pdata);
    void SetTimer(int length);
    void Show(Info& info);
    void Close(IN OUT InternalIconData * notifyItem);
};

class CNotifyToolbar :
    public CWindowImplBaseT< CToolbar<InternalIconData>, CControlWinTraits >
{
    HIMAGELIST m_ImageList;
    int m_VisibleButtonCount;

    CBalloonQueue * m_BalloonQueue;

public:
    CNotifyToolbar();
    virtual ~CNotifyToolbar();

    int GetVisibleButtonCount();
    int FindItem(IN HWND hWnd, IN UINT uID, InternalIconData ** pdata);
    int FindExistingSharedIcon(HICON handle);
    BOOL AddButton(IN CONST NOTIFYICONDATA *iconData);
    BOOL SwitchVersion(IN CONST NOTIFYICONDATA *iconData);
    BOOL UpdateButton(IN CONST NOTIFYICONDATA *iconData);
    BOOL RemoveButton(IN CONST NOTIFYICONDATA *iconData);
    VOID ResizeImagelist();

private:
    VOID SendMouseEvent(IN WORD wIndex, IN UINT uMsg, IN WPARAM wParam);
    LRESULT OnMouseEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTooltipShow(INT uCode, LPNMHDR hdr, BOOL& bHandled);

public:
    BEGIN_MSG_MAP(CNotifyToolbar)
        MESSAGE_RANGE_HANDLER(WM_MOUSEFIRST, WM_MOUSELAST, OnMouseEvent)
        NOTIFY_CODE_HANDLER(TTN_SHOW, OnTooltipShow)
    END_MSG_MAP()

    void Initialize(HWND hWndParent, CBalloonQueue * queue);
};

extern const WCHAR szSysPagerWndClass[];

class CSysPagerWnd :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl < CSysPagerWnd, CWindow, CControlWinTraits >,
    public CIconWatcher
{
    CNotifyToolbar Toolbar;
    CTooltips m_Balloons;
    CBalloonQueue m_BalloonQueue;

public:
    CSysPagerWnd();
    virtual ~CSysPagerWnd();

    LRESULT DrawBackground(HDC hdc);
    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnGetInfoTip(INT uCode, LPNMHDR hdr, BOOL& bHandled);
    LRESULT OnCustomDraw(INT uCode, LPNMHDR hdr, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCtxMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnBalloonPop(UINT uCode, LPNMHDR hdr, BOOL& bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSettingChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

public:

    BOOL NotifyIcon(DWORD notify_code, _In_ CONST NOTIFYICONDATA *iconData);
    void GetSize(IN BOOL IsHorizontal, IN PSIZE size);

    DECLARE_WND_CLASS_EX(szSysPagerWndClass, CS_DBLCLKS, COLOR_3DFACE)

    BEGIN_MSG_MAP(CSysPagerWnd)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenu)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_COPYDATA, OnCopyData)
        MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChanged)
        NOTIFY_CODE_HANDLER(TTN_POP, OnBalloonPop)
        NOTIFY_CODE_HANDLER(TBN_GETINFOTIPW, OnGetInfoTip)
        NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnCustomDraw)
    END_MSG_MAP()

    HWND _Init(IN HWND hWndParent);
};
