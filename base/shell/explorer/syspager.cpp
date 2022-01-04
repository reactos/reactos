/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
 * Copyright 2018 Ged Murphy <gedmurphy@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

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

class CNotifyToolbar;

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
            StringCchCopy(szInfo, _countof(szInfo), source->szInfo);
            StringCchCopy(szInfoTitle, _countof(szInfoTitle), source->szInfoTitle);
            uIcon = source->dwInfoFlags & NIIF_ICON_MASK;
            if (source->dwInfoFlags == NIIF_USER)
                uIcon = reinterpret_cast<WPARAM>(source->hIcon);
            uTimeout = source->uTimeout;
        }
    };

    HWND m_hwndParent;

    CTooltips * m_tooltips;

    CAtlList<Info> m_queue;

    CNotifyToolbar * m_toolbar;

    InternalIconData * m_current;
    bool m_currentClosed;

    int m_timer;

public:
    CBalloonQueue();

    void Init(HWND hwndParent, CNotifyToolbar * toolbar, CTooltips * balloons);
    void Deinit();

    bool OnTimer(int timerId);
    void UpdateInfo(InternalIconData * notifyItem);
    void RemoveInfo(InternalIconData * notifyItem);
    void CloseCurrent();

private:

    int IndexOf(InternalIconData * pdata);
    void SetTimer(int length);
    void Show(Info& info);
    void Close(IN OUT InternalIconData * notifyItem, IN UINT uReason);
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
    bool SendNotifyCallback(InternalIconData* notifyItem, UINT uMsg);

private:
    LRESULT OnCtxMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    VOID SendMouseEvent(IN WORD wIndex, IN UINT uMsg, IN WPARAM wParam);
    LRESULT OnMouseEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTooltipShow(INT uCode, LPNMHDR hdr, BOOL& bHandled);

public:
    BEGIN_MSG_MAP(CNotifyToolbar)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenu)
        MESSAGE_RANGE_HANDLER(WM_MOUSEFIRST, WM_MOUSELAST, OnMouseEvent)
        NOTIFY_CODE_HANDLER(TTN_SHOW, OnTooltipShow)
    END_MSG_MAP()

    void Initialize(HWND hWndParent, CBalloonQueue * queue);
};


static const WCHAR szSysPagerWndClass[] = L"SysPager";

class CSysPagerWnd :
    public CComCoClass<CSysPagerWnd>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl < CSysPagerWnd, CWindow, CControlWinTraits >,
    public IOleWindow,
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
    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCtxMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnBalloonPop(UINT uCode, LPNMHDR hdr, BOOL& bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSettingChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnGetMinimumSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

public:

    HRESULT WINAPI GetWindow(HWND* phwnd)
    {
        if (!phwnd)
            return E_INVALIDARG;
        *phwnd = m_hWnd;
        return S_OK;
    }

    HRESULT WINAPI ContextSensitiveHelp(BOOL fEnterMode)
    {
        return E_NOTIMPL;
    }

    DECLARE_NOT_AGGREGATABLE(CSysPagerWnd)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CSysPagerWnd)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
    END_COM_MAP()

    BOOL NotifyIcon(DWORD dwMessage, _In_ CONST NOTIFYICONDATA *iconData);
    void GetSize(IN BOOL IsHorizontal, IN PSIZE size);

    DECLARE_WND_CLASS_EX(szSysPagerWndClass, CS_DBLCLKS, COLOR_3DFACE)

    BEGIN_MSG_MAP(CSysPagerWnd)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenu)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_COPYDATA, OnCopyData)
        MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChanged)
        MESSAGE_HANDLER(TNWM_GETMINIMUMSIZE, OnGetMinimumSize)
        NOTIFY_CODE_HANDLER(TTN_POP, OnBalloonPop)
        NOTIFY_CODE_HANDLER(TBN_GETINFOTIPW, OnGetInfoTip)
        NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnCustomDraw)
    END_MSG_MAP()

    HRESULT Initialize(IN HWND hWndParent);
};

/*
 * IconWatcher
 */

CIconWatcher::CIconWatcher() :
    m_hWatcherThread(NULL),
    m_WakeUpEvent(NULL),
    m_hwndSysTray(NULL),
    m_Loop(false)
{
}

CIconWatcher::~CIconWatcher()
{
    Uninitialize();
    DeleteCriticalSection(&m_ListLock);

    if (m_WakeUpEvent)
        CloseHandle(m_WakeUpEvent);
    if (m_hWatcherThread)
        CloseHandle(m_hWatcherThread);
}

bool CIconWatcher::Initialize(_In_ HWND hWndParent)
{
    m_hwndSysTray = hWndParent;

    InitializeCriticalSection(&m_ListLock);
    m_WakeUpEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (m_WakeUpEvent == NULL)
        return false;

    m_hWatcherThread = (HANDLE)_beginthreadex(NULL,
                                                0,
                                                WatcherThread,
                                                (LPVOID)this,
                                                0,
                                                NULL);
    if (m_hWatcherThread == NULL)
        return false;

    return true;
}

void CIconWatcher::Uninitialize()
{
    m_Loop = false;
    if (m_WakeUpEvent)
        SetEvent(m_WakeUpEvent);

    EnterCriticalSection(&m_ListLock);

    POSITION Pos;
    for (size_t i = 0; i < m_WatcherList.GetCount(); i++)
    {
        Pos = m_WatcherList.FindIndex(i);
        if (Pos)
        {
            IconWatcherData *Icon;
            Icon = m_WatcherList.GetAt(Pos);
            delete Icon;
        }
    }
    m_WatcherList.RemoveAll();

    LeaveCriticalSection(&m_ListLock);
}

bool CIconWatcher::AddIconToWatcher(_In_ CONST NOTIFYICONDATA *iconData)
{
    DWORD ProcessId;
    (void)GetWindowThreadProcessId(iconData->hWnd, &ProcessId);

    HANDLE hProcess;
    hProcess = OpenProcess(SYNCHRONIZE, FALSE, ProcessId);
    if (hProcess == NULL)
    {
        return false;
    }

    IconWatcherData *Icon = new IconWatcherData(iconData);
    Icon->hProcess = hProcess;
    Icon->ProcessId = ProcessId;

    bool Added = false;
    EnterCriticalSection(&m_ListLock);

    // The likelyhood of someone having more than 64 icons in their tray is
    // pretty slim. We could spin up a new thread for each multiple of 64, but
    // it's not worth the effort, so we just won't bother watching those icons
    if (m_WatcherList.GetCount() < MAXIMUM_WAIT_OBJECTS)
    {
        m_WatcherList.AddTail(Icon);
        SetEvent(m_WakeUpEvent);
        Added = true;
    }

    LeaveCriticalSection(&m_ListLock);

    if (!Added)
    {
        delete Icon;
    }

    return Added;
}

bool CIconWatcher::RemoveIconFromWatcher(_In_ CONST NOTIFYICONDATA *iconData)
{
    EnterCriticalSection(&m_ListLock);

    IconWatcherData *Icon;
    Icon = GetListEntry(iconData, NULL, true);

    SetEvent(m_WakeUpEvent);
    LeaveCriticalSection(&m_ListLock);

    delete Icon;
    return true;
}

IconWatcherData* CIconWatcher::GetListEntry(_In_opt_ CONST NOTIFYICONDATA *iconData, _In_opt_ HANDLE hProcess, _In_ bool Remove)
{
    IconWatcherData *Entry = NULL;
    POSITION NextPosition = m_WatcherList.GetHeadPosition();
    POSITION Position;
    do
    {
        Position = NextPosition;

        Entry = m_WatcherList.GetNext(NextPosition);
        if (Entry)
        {
            if ((iconData && ((Entry->IconData.hWnd == iconData->hWnd) && (Entry->IconData.uID == iconData->uID))) ||
                    (hProcess && (Entry->hProcess == hProcess)))
            {
                if (Remove)
                    m_WatcherList.RemoveAt(Position);
                break;
            }
        }
        Entry = NULL;

    } while (NextPosition != NULL);

    return Entry;
}

UINT WINAPI CIconWatcher::WatcherThread(_In_opt_ LPVOID lpParam)
{
    CIconWatcher* This = reinterpret_cast<CIconWatcher *>(lpParam);
    HANDLE *WatchList = NULL;

    This->m_Loop = true;
    while (This->m_Loop)
    {
        EnterCriticalSection(&This->m_ListLock);

        DWORD Size;
        Size = This->m_WatcherList.GetCount() + 1;
        ASSERT(Size <= MAXIMUM_WAIT_OBJECTS);

        if (WatchList)
            delete[] WatchList;
        WatchList = new HANDLE[Size];
        WatchList[0] = This->m_WakeUpEvent;

        POSITION Pos;
        for (size_t i = 0; i < This->m_WatcherList.GetCount(); i++)
        {
            Pos = This->m_WatcherList.FindIndex(i);
            if (Pos)
            {
                IconWatcherData *Icon;
                Icon = This->m_WatcherList.GetAt(Pos);
                WatchList[i + 1] = Icon->hProcess;
            }
        }

        LeaveCriticalSection(&This->m_ListLock);

        DWORD Status;
        Status = WaitForMultipleObjects(Size,
                                        WatchList,
                                        FALSE,
                                        INFINITE);
        if (Status == WAIT_OBJECT_0)
        {
            // We've been kicked, we have updates to our list (or we're exiting the thread)
            if (This->m_Loop)
                TRACE("Updating watched icon list\n");
        }
        else if ((Status >= WAIT_OBJECT_0 + 1) && (Status < Size))
        {
            IconWatcherData *Icon;
            Icon = This->GetListEntry(NULL, WatchList[Status], false);

            TRACE("Pid %lu owns a notification icon and has stopped without deleting it. We'll cleanup on its behalf\n", Icon->ProcessId);

            TRAYNOTIFYDATAW tnid = {0};
            tnid.dwSignature = NI_NOTIFY_SIG;
            tnid.dwMessage   = NIM_DELETE;
            CopyMemory(&tnid.nid, &Icon->IconData, Icon->IconData.cbSize);

            COPYDATASTRUCT data;
            data.dwData = 1;
            data.cbData = sizeof(tnid);
            data.lpData = &tnid;

            BOOL Success = ::SendMessage(This->m_hwndSysTray, WM_COPYDATA,
                                         (WPARAM)&Icon->IconData, (LPARAM)&data);
            if (!Success)
            {
                // If we failed to handle the delete message, forcibly remove it
                This->RemoveIconFromWatcher(&Icon->IconData);
            }
        }
        else
        {
            if (Status == WAIT_FAILED)
            {
                Status = GetLastError();
            }
            ERR("Failed to wait on process handles : %lu\n", Status);
            This->Uninitialize();
        }
    }

    if (WatchList)
        delete[] WatchList;

    return 0;
}

/*
 * BalloonQueue
 */

CBalloonQueue::CBalloonQueue() :
    m_hwndParent(NULL),
    m_tooltips(NULL),
    m_toolbar(NULL),
    m_current(NULL),
    m_currentClosed(false),
    m_timer(-1)
{
}

void CBalloonQueue::Init(HWND hwndParent, CNotifyToolbar * toolbar, CTooltips * balloons)
{
    m_hwndParent = hwndParent;
    m_toolbar = toolbar;
    m_tooltips = balloons;
}

void CBalloonQueue::Deinit()
{
    if (m_timer >= 0)
    {
        ::KillTimer(m_hwndParent, m_timer);
    }
}

bool CBalloonQueue::OnTimer(int timerId)
{
    if (timerId != m_timer)
        return false;

    ::KillTimer(m_hwndParent, m_timer);
    m_timer = -1;

    if (m_current && !m_currentClosed)
    {
        Close(m_current, NIN_BALLOONTIMEOUT);
    }
    else
    {
        m_current = NULL;
        m_currentClosed = false;
        if (!m_queue.IsEmpty())
        {
            Info info = m_queue.RemoveHead();
            Show(info);
        }
    }

    return true;
}

void CBalloonQueue::UpdateInfo(InternalIconData * notifyItem)
{
    size_t len = 0;
    HRESULT hr = StringCchLength(notifyItem->szInfo, _countof(notifyItem->szInfo), &len);
    if (SUCCEEDED(hr) && len > 0)
    {
        Info info(notifyItem);

        // If m_current == notifyItem, we want to replace the previous balloon even if there is a queue.
        if (m_current != notifyItem && (m_current != NULL || !m_queue.IsEmpty()))
        {
            m_queue.AddTail(info);
        }
        else
        {
            Show(info);
        }
    }
    else
    {
        Close(notifyItem, NIN_BALLOONHIDE);
    }
}

void CBalloonQueue::RemoveInfo(InternalIconData * notifyItem)
{
    Close(notifyItem, NIN_BALLOONHIDE);

    POSITION position = m_queue.GetHeadPosition();
    while(position != NULL)
    {
        Info& info = m_queue.GetNext(position);
        if (info.pSource == notifyItem)
        {
            m_queue.RemoveAt(position);
        }
    }
}

void CBalloonQueue::CloseCurrent()
{
    if (m_current != NULL)
    {
        Close(m_current, NIN_BALLOONTIMEOUT);
    }
}

int CBalloonQueue::IndexOf(InternalIconData * pdata)
{
    int count = m_toolbar->GetButtonCount();
    for (int i = 0; i < count; i++)
    {
        if (m_toolbar->GetItemData(i) == pdata)
            return i;
    }
    return -1;
}

void CBalloonQueue::SetTimer(int length)
{
    m_timer = ::SetTimer(m_hwndParent, BalloonsTimerId, length, NULL);
}

void CBalloonQueue::Show(Info& info)
{
    TRACE("ShowBalloonTip called for flags=%x text=%ws; title=%ws\n", info.uIcon, info.szInfo, info.szInfoTitle);

    // TODO: NIF_REALTIME, NIIF_NOSOUND, other Vista+ flags

    const int index = IndexOf(info.pSource);
    RECT rc;
    m_toolbar->GetItemRect(index, &rc);
    m_toolbar->ClientToScreen(&rc);
    const WORD x = (rc.left + rc.right) / 2;
    const WORD y = (rc.top + rc.bottom) / 2;

    m_tooltips->SetTitle(info.szInfoTitle, info.uIcon);
    m_tooltips->TrackPosition(x, y);
    m_tooltips->UpdateTipText(m_hwndParent, reinterpret_cast<LPARAM>(m_toolbar->m_hWnd), info.szInfo);
    m_tooltips->TrackActivate(m_hwndParent, reinterpret_cast<LPARAM>(m_toolbar->m_hWnd));

    m_current = info.pSource;
    int timeout = info.uTimeout;
    if (timeout < MinTimeout) timeout = MinTimeout;
    if (timeout > MaxTimeout) timeout = MaxTimeout;

    SetTimer(timeout);

    m_toolbar->SendNotifyCallback(m_current, NIN_BALLOONSHOW);
}

void CBalloonQueue::Close(IN OUT InternalIconData * notifyItem, IN UINT uReason)
{
    TRACE("HideBalloonTip called\n");

    if (m_current == notifyItem && !m_currentClosed)
    {
        m_toolbar->SendNotifyCallback(m_current, uReason);

        // Prevent Re-entry
        m_currentClosed = true;
        m_tooltips->TrackDeactivate();
        SetTimer(CooldownBetweenBalloons);
    }
}

/*
 * NotifyToolbar
 */

CNotifyToolbar::CNotifyToolbar() :
    m_ImageList(NULL),
    m_VisibleButtonCount(0),
    m_BalloonQueue(NULL)
{
}

CNotifyToolbar::~CNotifyToolbar()
{
}

int CNotifyToolbar::GetVisibleButtonCount()
{
    return m_VisibleButtonCount;
}

int CNotifyToolbar::FindItem(IN HWND hWnd, IN UINT uID, InternalIconData ** pdata)
{
    int count = GetButtonCount();

    for (int i = 0; i < count; i++)
    {
        InternalIconData * data = GetItemData(i);

        if (data->hWnd == hWnd &&
            data->uID == uID)
        {
            if (pdata)
                *pdata = data;
            return i;
        }
    }

    return -1;
}

int CNotifyToolbar::FindExistingSharedIcon(HICON handle)
{
    int count = GetButtonCount();
    for (int i = 0; i < count; i++)
    {
        InternalIconData * data = GetItemData(i);
        if (data->hIcon == handle)
        {
            TBBUTTON btn;
            GetButton(i, &btn);
            return btn.iBitmap;
        }
    }

    return -1;
}

BOOL CNotifyToolbar::AddButton(_In_ CONST NOTIFYICONDATA *iconData)
{
    TBBUTTON tbBtn = { 0 };
    InternalIconData * notifyItem;
    WCHAR text[] = L"";

    TRACE("Adding icon %d from hWnd %08x flags%s%s state%s%s",
        iconData->uID, iconData->hWnd,
        (iconData->uFlags & NIF_ICON) ? " ICON" : "",
        (iconData->uFlags & NIF_STATE) ? " STATE" : "",
        (iconData->dwState & NIS_HIDDEN) ? " HIDDEN" : "",
        (iconData->dwState & NIS_SHAREDICON) ? " SHARED" : "");

    int index = FindItem(iconData->hWnd, iconData->uID, &notifyItem);
    if (index >= 0)
    {
        TRACE("Icon %d from hWnd %08x ALREADY EXISTS!", iconData->uID, iconData->hWnd);
        return FALSE;
    }

    notifyItem = new InternalIconData();
    ZeroMemory(notifyItem, sizeof(*notifyItem));

    notifyItem->hWnd = iconData->hWnd;
    notifyItem->uID = iconData->uID;

    tbBtn.fsState = TBSTATE_ENABLED;
    tbBtn.fsStyle = BTNS_NOPREFIX;
    tbBtn.dwData = (DWORD_PTR)notifyItem;
    tbBtn.iString = (INT_PTR) text;
    tbBtn.idCommand = GetButtonCount();
    tbBtn.iBitmap = -1;

    if (iconData->uFlags & NIF_STATE)
    {
        notifyItem->dwState = iconData->dwState & iconData->dwStateMask;
    }

    if (iconData->uFlags & NIF_MESSAGE)
    {
        notifyItem->uCallbackMessage = iconData->uCallbackMessage;
    }

    if (iconData->uFlags & NIF_ICON)
    {
        notifyItem->hIcon = iconData->hIcon;
        BOOL hasSharedIcon = notifyItem->dwState & NIS_SHAREDICON;
        if (hasSharedIcon)
        {
            INT iIcon = FindExistingSharedIcon(notifyItem->hIcon);
            if (iIcon < 0)
            {
                notifyItem->hIcon = NULL;
                TRACE("Shared icon requested, but HICON not found!!!");
            }
            tbBtn.iBitmap = iIcon;
        }
        else
        {
            tbBtn.iBitmap = ImageList_AddIcon(m_ImageList, notifyItem->hIcon);
        }
    }

    if (iconData->uFlags & NIF_TIP)
    {
        StringCchCopy(notifyItem->szTip, _countof(notifyItem->szTip), iconData->szTip);
    }

    if (iconData->uFlags & NIF_INFO)
    {
        // NOTE: In Vista+, the uTimeout value is disregarded, and the accessibility settings are used always.
        StringCchCopy(notifyItem->szInfo, _countof(notifyItem->szInfo), iconData->szInfo);
        StringCchCopy(notifyItem->szInfoTitle, _countof(notifyItem->szInfoTitle), iconData->szInfoTitle);
        notifyItem->dwInfoFlags = iconData->dwInfoFlags;
        notifyItem->uTimeout = iconData->uTimeout;
    }

    if (notifyItem->dwState & NIS_HIDDEN)
    {
        tbBtn.fsState |= TBSTATE_HIDDEN;
    }
    else
    {
        m_VisibleButtonCount++;
    }

    /* TODO: support VERSION_4 (NIF_GUID, NIF_REALTIME, NIF_SHOWTIP) */

    CToolbar::AddButton(&tbBtn);
    SetButtonSize(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));

    if (iconData->uFlags & NIF_INFO)
    {
        m_BalloonQueue->UpdateInfo(notifyItem);
    }

    return TRUE;
}

BOOL CNotifyToolbar::SwitchVersion(_In_ CONST NOTIFYICONDATA *iconData)
{
    InternalIconData * notifyItem;
    int index = FindItem(iconData->hWnd, iconData->uID, &notifyItem);
    if (index < 0)
    {
        WARN("Icon %d from hWnd %08x DOES NOT EXIST!", iconData->uID, iconData->hWnd);
        return FALSE;
    }

    if (iconData->uVersion != 0 && iconData->uVersion != NOTIFYICON_VERSION)
    {
        WARN("Tried to set the version of icon %d from hWnd %08x, to an unknown value %d. Vista+ program?", iconData->uID, iconData->hWnd, iconData->uVersion);
        return FALSE;
    }

    // We can not store the version in the uVersion field, because it's union'd with uTimeout,
    // which we also need to keep track of.
    notifyItem->uVersionCopy = iconData->uVersion;

    return TRUE;
}

BOOL CNotifyToolbar::UpdateButton(_In_ CONST NOTIFYICONDATA *iconData)
{
    InternalIconData * notifyItem;
    TBBUTTONINFO tbbi = { 0 };

    TRACE("Updating icon %d from hWnd %08x flags%s%s state%s%s\n",
        iconData->uID, iconData->hWnd,
        (iconData->uFlags & NIF_ICON) ? " ICON" : "",
        (iconData->uFlags & NIF_STATE) ? " STATE" : "",
        (iconData->dwState & NIS_HIDDEN) ? " HIDDEN" : "",
        (iconData->dwState & NIS_SHAREDICON) ? " SHARED" : "");

    int index = FindItem(iconData->hWnd, iconData->uID, &notifyItem);
    if (index < 0)
    {
        WARN("Icon %d from hWnd %08x DOES NOT EXIST!\n", iconData->uID, iconData->hWnd);
        return AddButton(iconData);
    }

    TBBUTTON btn;
    GetButton(index, &btn);
    int oldIconIndex = btn.iBitmap;

    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_BYINDEX | TBIF_COMMAND;
    tbbi.idCommand = index;

    if (iconData->uFlags & NIF_STATE)
    {
        if (iconData->dwStateMask & NIS_HIDDEN &&
            (notifyItem->dwState & NIS_HIDDEN) != (iconData->dwState & NIS_HIDDEN))
        {
            tbbi.dwMask |= TBIF_STATE;
            if (iconData->dwState & NIS_HIDDEN)
            {
                tbbi.fsState |= TBSTATE_HIDDEN;
                m_VisibleButtonCount--;
            }
            else
            {
                tbbi.fsState &= ~TBSTATE_HIDDEN;
                m_VisibleButtonCount++;
            }
        }

        notifyItem->dwState &= ~iconData->dwStateMask;
        notifyItem->dwState |= (iconData->dwState & iconData->dwStateMask);
    }

    if (iconData->uFlags & NIF_MESSAGE)
    {
        notifyItem->uCallbackMessage = iconData->uCallbackMessage;
    }

    if (iconData->uFlags & NIF_ICON)
    {
        BOOL hasSharedIcon = notifyItem->dwState & NIS_SHAREDICON;
        if (hasSharedIcon)
        {
            INT iIcon = FindExistingSharedIcon(iconData->hIcon);
            if (iIcon >= 0)
            {
                notifyItem->hIcon = iconData->hIcon;
                tbbi.dwMask |= TBIF_IMAGE;
                tbbi.iImage = iIcon;
            }
            else
            {
                TRACE("Shared icon requested, but HICON not found!!! IGNORING!");
            }
        }
        else
        {
            notifyItem->hIcon = iconData->hIcon;
            tbbi.dwMask |= TBIF_IMAGE;
            tbbi.iImage = ImageList_ReplaceIcon(m_ImageList, oldIconIndex, notifyItem->hIcon);
        }
    }

    if (iconData->uFlags & NIF_TIP)
    {
        StringCchCopy(notifyItem->szTip, _countof(notifyItem->szTip), iconData->szTip);
    }

    if (iconData->uFlags & NIF_INFO)
    {
        // NOTE: In Vista+, the uTimeout value is disregarded, and the accessibility settings are used always.
        StringCchCopy(notifyItem->szInfo, _countof(notifyItem->szInfo), iconData->szInfo);
        StringCchCopy(notifyItem->szInfoTitle, _countof(notifyItem->szInfoTitle), iconData->szInfoTitle);
        notifyItem->dwInfoFlags = iconData->dwInfoFlags;
        notifyItem->uTimeout = iconData->uTimeout;
    }

    /* TODO: support VERSION_4 (NIF_GUID, NIF_REALTIME, NIF_SHOWTIP) */

    SetButtonInfo(index, &tbbi);

    if (iconData->uFlags & NIF_INFO)
    {
        m_BalloonQueue->UpdateInfo(notifyItem);
    }

    return TRUE;
}

BOOL CNotifyToolbar::RemoveButton(_In_ CONST NOTIFYICONDATA *iconData)
{
    InternalIconData * notifyItem;

    TRACE("Removing icon %d from hWnd %08x", iconData->uID, iconData->hWnd);

    int index = FindItem(iconData->hWnd, iconData->uID, &notifyItem);
    if (index < 0)
    {
        TRACE("Icon %d from hWnd %08x ALREADY MISSING!\n", iconData->uID, iconData->hWnd);

        return FALSE;
    }

    if (!(notifyItem->dwState & NIS_HIDDEN))
    {
        m_VisibleButtonCount--;
    }

    if (!(notifyItem->dwState & NIS_SHAREDICON))
    {
        TBBUTTON btn;
        GetButton(index, &btn);
        int oldIconIndex = btn.iBitmap;
        ImageList_Remove(m_ImageList, oldIconIndex);

        // Update other icons!
        int count = GetButtonCount();
        for (int i = 0; i < count; i++)
        {
            TBBUTTON btn;
            GetButton(i, &btn);

            if (btn.iBitmap > oldIconIndex)
            {
                TBBUTTONINFO tbbi2 = { 0 };
                tbbi2.cbSize = sizeof(tbbi2);
                tbbi2.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
                tbbi2.iImage = btn.iBitmap-1;
                SetButtonInfo(i, &tbbi2);
            }
        }
    }

    m_BalloonQueue->RemoveInfo(notifyItem);

    DeleteButton(index);

    delete notifyItem;

    return TRUE;
}

VOID CNotifyToolbar::ResizeImagelist()
{
    int cx, cy;
    HIMAGELIST iml;

    if (!ImageList_GetIconSize(m_ImageList, &cx, &cy))
        return;

    if (cx == GetSystemMetrics(SM_CXSMICON) && cy == GetSystemMetrics(SM_CYSMICON))
        return;

    iml = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32 | ILC_MASK, 0, 1000);
    if (!iml)
        return;

    ImageList_Destroy(m_ImageList);
    m_ImageList = iml;
    SetImageList(m_ImageList);

    int count = GetButtonCount();
    for (int i = 0; i < count; i++)
    {
        InternalIconData * data = GetItemData(i);
        BOOL hasSharedIcon = data->dwState & NIS_SHAREDICON;
        INT iIcon = hasSharedIcon ? FindExistingSharedIcon(data->hIcon) : -1;
        if (iIcon < 0)
            iIcon = ImageList_AddIcon(iml, data->hIcon);
        TBBUTTONINFO tbbi = { sizeof(tbbi), TBIF_BYINDEX | TBIF_IMAGE, 0, iIcon};
        SetButtonInfo(i, &tbbi);
    }

    SetButtonSize(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
}

LRESULT CNotifyToolbar::OnCtxMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = FALSE;

    /*
     * WM_CONTEXTMENU message can be generated either by the mouse,
     * in which case lParam encodes the mouse coordinates where the
     * user right-clicked the mouse, or can be generated by (Shift-)F10
     * keyboard press, in which case lParam equals -1.
     */
    INT iBtn = GetHotItem();
    if (iBtn < 0)
        return 0;

    InternalIconData* notifyItem = GetItemData(iBtn);

    if (!::IsWindow(notifyItem->hWnd))
        return 0;

    if (notifyItem->uVersionCopy >= NOTIFYICON_VERSION)
    {
        /* Transmit the WM_CONTEXTMENU message if the notification icon supports it */
        ::SendNotifyMessage(notifyItem->hWnd,
                            notifyItem->uCallbackMessage,
                            notifyItem->uID,
                            WM_CONTEXTMENU);
    }
    else if (lParam == -1)
    {
        /*
         * Otherwise, and only if the WM_CONTEXTMENU message was generated
         * from the keyboard, simulate right-click mouse messages. This is
         * not needed if the message came from the mouse because in this
         * case the right-click mouse messages were already sent together.
         */
        ::SendNotifyMessage(notifyItem->hWnd,
                            notifyItem->uCallbackMessage,
                            notifyItem->uID,
                            WM_RBUTTONDOWN);
        ::SendNotifyMessage(notifyItem->hWnd,
                            notifyItem->uCallbackMessage,
                            notifyItem->uID,
                            WM_RBUTTONUP);
    }

    return 0;
}

bool CNotifyToolbar::SendNotifyCallback(InternalIconData* notifyItem, UINT uMsg)
{
    if (!::IsWindow(notifyItem->hWnd))
    {
        // We detect and destroy icons with invalid handles only on mouse move over systray, same as MS does.
        // Alternatively we could search for them periodically (would waste more resources).
        TRACE("Destroying icon %d with invalid handle hWnd=%08x\n", notifyItem->uID, notifyItem->hWnd);

        RemoveButton(notifyItem);

        /* Ask the parent to resize */
        NMHDR nmh = {GetParent(), 0, NTNWM_REALIGN};
        GetParent().SendMessage(WM_NOTIFY, 0, (LPARAM) &nmh);

        return true;
    }

    DWORD pid;
    GetWindowThreadProcessId(notifyItem->hWnd, &pid);

    if (pid == GetCurrentProcessId() ||
        (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST))
    {
        ::PostMessage(notifyItem->hWnd,
                      notifyItem->uCallbackMessage,
                      notifyItem->uID,
                      uMsg);
    }
    else
    {
        ::SendMessage(notifyItem->hWnd,
                      notifyItem->uCallbackMessage,
                      notifyItem->uID,
                      uMsg);
    }
    return false;
}

VOID CNotifyToolbar::SendMouseEvent(IN WORD wIndex, IN UINT uMsg, IN WPARAM wParam)
{
    static LPCWSTR eventNames [] = {
        L"WM_MOUSEMOVE",
        L"WM_LBUTTONDOWN",
        L"WM_LBUTTONUP",
        L"WM_LBUTTONDBLCLK",
        L"WM_RBUTTONDOWN",
        L"WM_RBUTTONUP",
        L"WM_RBUTTONDBLCLK",
        L"WM_MBUTTONDOWN",
        L"WM_MBUTTONUP",
        L"WM_MBUTTONDBLCLK",
        L"WM_MOUSEWHEEL",
        L"WM_XBUTTONDOWN",
        L"WM_XBUTTONUP",
        L"WM_XBUTTONDBLCLK"
    };

    InternalIconData * notifyItem = GetItemData(wIndex);

    if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
    {
        TRACE("Sending message %S from button %d to %p (msg=%x, w=%x, l=%x)...\n",
            eventNames[uMsg - WM_MOUSEFIRST], wIndex,
            notifyItem->hWnd, notifyItem->uCallbackMessage, notifyItem->uID, uMsg);
    }

    SendNotifyCallback(notifyItem, uMsg);
}

LRESULT CNotifyToolbar::OnMouseEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    INT iBtn = HitTest(&pt);

    if (iBtn >= 0)
    {
        SendMouseEvent(iBtn, uMsg, wParam);
    }

    bHandled = FALSE;
    return FALSE;
}

static VOID GetTooltipText(LPARAM data, LPTSTR szTip, DWORD cchTip)
{
    InternalIconData * notifyItem = reinterpret_cast<InternalIconData *>(data);
    if (notifyItem)
    {
        StringCchCopy(szTip, cchTip, notifyItem->szTip);
    }
    else
    {
        StringCchCopy(szTip, cchTip, L"");
    }
}

LRESULT CNotifyToolbar::OnTooltipShow(INT uCode, LPNMHDR hdr, BOOL& bHandled)
{
    RECT rcTip, rcItem;
    ::GetWindowRect(hdr->hwndFrom, &rcTip);

    SIZE szTip = { rcTip.right - rcTip.left, rcTip.bottom - rcTip.top };

    INT iBtn = GetHotItem();

    if (iBtn >= 0)
    {
        MONITORINFO monInfo = { 0 };
        HMONITOR hMon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);

        monInfo.cbSize = sizeof(monInfo);

        if (hMon)
            GetMonitorInfo(hMon, &monInfo);
        else
            ::GetWindowRect(GetDesktopWindow(), &monInfo.rcMonitor);

        GetItemRect(iBtn, &rcItem);

        POINT ptItem = { rcItem.left, rcItem.top };
        SIZE szItem = { rcItem.right - rcItem.left, rcItem.bottom - rcItem.top };
        ClientToScreen(&ptItem);

        ptItem.x += szItem.cx / 2;
        ptItem.y -= szTip.cy;

        if (ptItem.x + szTip.cx > monInfo.rcMonitor.right)
            ptItem.x = monInfo.rcMonitor.right - szTip.cx;

        if (ptItem.y + szTip.cy > monInfo.rcMonitor.bottom)
            ptItem.y = monInfo.rcMonitor.bottom - szTip.cy;

        if (ptItem.x < monInfo.rcMonitor.left)
            ptItem.x = monInfo.rcMonitor.left;

        if (ptItem.y < monInfo.rcMonitor.top)
            ptItem.y = monInfo.rcMonitor.top;

        TRACE("ptItem { %d, %d }\n", ptItem.x, ptItem.y);

        ::SetWindowPos(hdr->hwndFrom, NULL, ptItem.x, ptItem.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        return TRUE;
    }

    bHandled = FALSE;
    return 0;
}

void CNotifyToolbar::Initialize(HWND hWndParent, CBalloonQueue * queue)
{
    m_BalloonQueue = queue;

    DWORD styles =
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN |
        TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_WRAPABLE | TBSTYLE_TRANSPARENT |
        CCS_TOP | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER;

    // HACK & FIXME: CORE-17505
    SubclassWindow(CToolbar::Create(hWndParent, styles));

    // Force the toolbar tooltips window to always show tooltips even if not foreground
    HWND tooltipsWnd = (HWND)SendMessageW(TB_GETTOOLTIPS);
    if (tooltipsWnd)
    {
        ::SetWindowLong(tooltipsWnd, GWL_STYLE, ::GetWindowLong(tooltipsWnd, GWL_STYLE) | TTS_ALWAYSTIP);
    }

    SetWindowTheme(m_hWnd, L"TrayNotify", NULL);

    m_ImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32 | ILC_MASK, 0, 1000);
    SetImageList(m_ImageList);

    TBMETRICS tbm = {sizeof(tbm)};
    tbm.dwMask = TBMF_BARPAD | TBMF_BUTTONSPACING | TBMF_PAD;
    tbm.cxPad = 1;
    tbm.cyPad = 1;
    tbm.cxBarPad = 1;
    tbm.cyBarPad = 1;
    tbm.cxButtonSpacing = 1;
    tbm.cyButtonSpacing = 1;
    SetMetrics(&tbm);

    SetButtonSize(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
}

/*
 * SysPagerWnd
 */

CSysPagerWnd::CSysPagerWnd() {}

CSysPagerWnd::~CSysPagerWnd() {}

LRESULT CSysPagerWnd::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HDC hdc = (HDC) wParam;

    if (!IsAppThemed())
    {
        bHandled = FALSE;
        return 0;
    }

    RECT rect;
    GetClientRect(&rect);
    DrawThemeParentBackground(m_hWnd, hdc, &rect);

    return TRUE;
}

LRESULT CSysPagerWnd::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    Toolbar.Initialize(m_hWnd, &m_BalloonQueue);
    CIconWatcher::Initialize(m_hWnd);

    HWND hWndTop = GetAncestor(m_hWnd, GA_ROOT);

    m_Balloons.Create(hWndTop, TTS_NOPREFIX | TTS_BALLOON | TTS_CLOSE);

    TOOLINFOW ti = { 0 };
    ti.cbSize = TTTOOLINFOW_V1_SIZE;
    ti.uFlags = TTF_TRACK | TTF_IDISHWND;
    ti.uId = reinterpret_cast<UINT_PTR>(Toolbar.m_hWnd);
    ti.hwnd = m_hWnd;
    ti.lpszText = NULL;
    ti.lParam = NULL;

    BOOL ret = m_Balloons.AddTool(&ti);
    if (!ret)
    {
        WARN("AddTool failed, LastError=%d (probably meaningless unless non-zero)\n", GetLastError());
    }

    m_BalloonQueue.Init(m_hWnd, &Toolbar, &m_Balloons);

    // Explicitly request running applications to re-register their systray icons
    ::SendNotifyMessageW(HWND_BROADCAST,
                            RegisterWindowMessageW(L"TaskbarCreated"),
                            0, 0);

    return TRUE;
}

LRESULT CSysPagerWnd::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_BalloonQueue.Deinit();
    CIconWatcher::Uninitialize();
    return TRUE;
}

BOOL CSysPagerWnd::NotifyIcon(DWORD dwMessage, _In_ CONST NOTIFYICONDATA *iconData)
{
    BOOL ret = FALSE;

    int VisibleButtonCount = Toolbar.GetVisibleButtonCount();

    TRACE("NotifyIcon received. Code=%d\n", dwMessage);
    switch (dwMessage)
    {
    case NIM_ADD:
        ret = Toolbar.AddButton(iconData);
        if (ret == TRUE)
        {
            (void)AddIconToWatcher(iconData);
        }
        break;

    case NIM_MODIFY:
        ret = Toolbar.UpdateButton(iconData);
        break;

    case NIM_DELETE:
        ret = Toolbar.RemoveButton(iconData);
        if (ret == TRUE)
        {
            (void)RemoveIconFromWatcher(iconData);
        }
        break;

    case NIM_SETFOCUS:
        Toolbar.SetFocus();
        ret = TRUE;
        break;

    case NIM_SETVERSION:
        ret = Toolbar.SwitchVersion(iconData);
        break;

    default:
        TRACE("NotifyIcon received with unknown code %d.\n", dwMessage);
        return FALSE;
    }

    if (VisibleButtonCount != Toolbar.GetVisibleButtonCount())
    {
        /* Ask the parent to resize */
        NMHDR nmh = {GetParent(), 0, NTNWM_REALIGN};
        GetParent().SendMessage(WM_NOTIFY, 0, (LPARAM) &nmh);
    }

    return ret;
}

void CSysPagerWnd::GetSize(IN BOOL IsHorizontal, IN PSIZE size)
{
    /* Get the ideal height or width */
#if 0
    /* Unfortunately this doens't work correctly in ros */
    Toolbar.GetIdealSize(!IsHorizontal, size);

    /* Make the reference dimension an exact multiple of the icon size */
    if (IsHorizontal)
        size->cy -= size->cy % GetSystemMetrics(SM_CYSMICON);
    else
        size->cx -= size->cx % GetSystemMetrics(SM_CXSMICON);

#else
    INT rows = 0;
    INT columns = 0;
    INT cyButton = GetSystemMetrics(SM_CYSMICON) + 2;
    INT cxButton = GetSystemMetrics(SM_CXSMICON) + 2;
    int VisibleButtonCount = Toolbar.GetVisibleButtonCount();

    if (IsHorizontal)
    {
        rows = max(size->cy / cyButton, 1);
        columns = (VisibleButtonCount + rows - 1) / rows;
    }
    else
    {
        columns = max(size->cx / cxButton, 1);
        rows = (VisibleButtonCount + columns - 1) / columns;
    }
    size->cx = columns * cxButton;
    size->cy = rows * cyButton;
#endif
}

LRESULT CSysPagerWnd::OnGetInfoTip(INT uCode, LPNMHDR hdr, BOOL& bHandled)
{
    NMTBGETINFOTIPW * nmtip = (NMTBGETINFOTIPW *) hdr;
    GetTooltipText(nmtip->lParam, nmtip->pszText, nmtip->cchTextMax);
    return TRUE;
}

LRESULT CSysPagerWnd::OnCustomDraw(INT uCode, LPNMHDR hdr, BOOL& bHandled)
{
    NMCUSTOMDRAW * cdraw = (NMCUSTOMDRAW *) hdr;
    switch (cdraw->dwDrawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT:
        return TBCDRF_NOBACKGROUND | TBCDRF_NOEDGES | TBCDRF_NOOFFSET | TBCDRF_NOMARK | TBCDRF_NOETCHEDEFFECT;
    }
    return TRUE;
}

LRESULT CSysPagerWnd::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = FALSE;

    /* Handles the BN_CLICKED notifications sent by the CNotifyToolbar member */
    if (HIWORD(wParam) != BN_CLICKED)
        return 0;

    INT iBtn = LOWORD(wParam);
    if (iBtn < 0)
        return 0;

    InternalIconData* notifyItem = Toolbar.GetItemData(iBtn);

    if (!::IsWindow(notifyItem->hWnd))
        return 0;

    // TODO: Improve keyboard handling by looking whether one presses
    // on ENTER, etc..., which roughly translates into "double-clicking".

    if (notifyItem->uVersionCopy >= NOTIFYICON_VERSION)
    {
        /* Use new-style notifications if the notification icon supports them */
        ::SendNotifyMessage(notifyItem->hWnd,
                            notifyItem->uCallbackMessage,
                            notifyItem->uID,
                            NIN_SELECT); // TODO: Distinguish with NIN_KEYSELECT
    }
    else if (lParam == -1)
    {
        /*
         * Otherwise, and only if the icon was selected via the keyboard,
         * simulate right-click mouse messages. This is not needed if the
         * selection was done by mouse because in this case the mouse
         * messages were already sent.
         */
        ::SendNotifyMessage(notifyItem->hWnd,
                            notifyItem->uCallbackMessage,
                            notifyItem->uID,
                            WM_LBUTTONDOWN); // TODO: Distinguish with double-click WM_LBUTTONDBLCLK
        ::SendNotifyMessage(notifyItem->hWnd,
                            notifyItem->uCallbackMessage,
                            notifyItem->uID,
                            WM_LBUTTONUP);
    }

    return 0;
}

LRESULT CSysPagerWnd::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT Ret = TRUE;
    SIZE szClient;
    szClient.cx = LOWORD(lParam);
    szClient.cy = HIWORD(lParam);

    Ret = DefWindowProc(uMsg, wParam, lParam);

    if (Toolbar)
    {
        Toolbar.SetWindowPos(NULL, 0, 0, szClient.cx, szClient.cy, SWP_NOZORDER);
        Toolbar.AutoSize();

        RECT rc;
        Toolbar.GetClientRect(&rc);

        SIZE szBar = { rc.right - rc.left, rc.bottom - rc.top };

        INT xOff = (szClient.cx - szBar.cx) / 2;
        INT yOff = (szClient.cy - szBar.cy) / 2;

        Toolbar.SetWindowPos(NULL, xOff, yOff, szBar.cx, szBar.cy, SWP_NOZORDER);
    }
    return Ret;
}

LRESULT CSysPagerWnd::OnCtxMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = TRUE;
    return 0;
}

LRESULT CSysPagerWnd::OnBalloonPop(UINT uCode, LPNMHDR hdr , BOOL& bHandled)
{
    m_BalloonQueue.CloseCurrent();
    bHandled = TRUE;
    return 0;
}

LRESULT CSysPagerWnd::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_BalloonQueue.OnTimer(wParam))
    {
        bHandled = TRUE;
    }

    return 0;
}

LRESULT CSysPagerWnd::OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PCOPYDATASTRUCT cpData = (PCOPYDATASTRUCT)lParam;
    if (cpData->dwData == TABDMC_NOTIFY)
    {
        /* A taskbar NotifyIcon notification */
        PTRAYNOTIFYDATAW pData = (PTRAYNOTIFYDATAW)cpData->lpData;
        if (pData->dwSignature == NI_NOTIFY_SIG)
            return NotifyIcon(pData->dwMessage, &pData->nid);
    }
    else if (cpData->dwData == TABDMC_LOADINPROC)
    {
        FIXME("Taskbar Load In Proc\n");
    }

    return FALSE;
}

LRESULT CSysPagerWnd::OnSettingChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == SPI_SETNONCLIENTMETRICS)
    {
        Toolbar.ResizeImagelist();
    }
    return 0;
}

LRESULT CSysPagerWnd::OnGetMinimumSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    GetSize((BOOL)wParam, (PSIZE)lParam);
    return 0;
}

HRESULT CSysPagerWnd::Initialize(IN HWND hWndParent)
{
    /* Create the window. The tray window is going to move it to the correct
        position and resize it as needed. */
    DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE;
    Create(hWndParent, 0, NULL, dwStyle);
    if (!m_hWnd)
        return E_FAIL;

    SetWindowTheme(m_hWnd, L"TrayNotify", NULL);

    return S_OK;
}

HRESULT CSysPagerWnd_CreateInstance(HWND hwndParent, REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CSysPagerWnd>(hwndParent, riid, ppv);
}
