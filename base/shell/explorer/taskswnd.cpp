/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Handles all taskbar related stuff like taskbar grouping and taskbar buttons
 * COPYRIGHT:   Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
 *              Copyright 2023 Jesús Sanz del Rey <jesussanz2003@gmail.com>
 */

#include "precomp.h"
#include <commoncontrols.h>
#include <psapi.h>

/* Set DUMP_TASKS to 1 to enable a dump of the tasks and task groups every
   5 seconds */
#define DUMP_TASKS  0
#define DEBUG_SHELL_HOOK 0

#define MAX_TASKS_COUNT (0x7FFF)
#define TASK_ITEM_ARRAY_ALLOC   64

const WCHAR szTaskSwitchWndClass[] = L"MSTaskSwWClass";
const WCHAR szRunningApps[] = L"Running Applications";

#if DEBUG_SHELL_HOOK
const struct {
    INT msg;
    LPCWSTR msg_name;
} hshell_msg [] = {
        { HSHELL_WINDOWCREATED, L"HSHELL_WINDOWCREATED" },
        { HSHELL_WINDOWDESTROYED, L"HSHELL_WINDOWDESTROYED" },
        { HSHELL_ACTIVATESHELLWINDOW, L"HSHELL_ACTIVATESHELLWINDOW" },
        { HSHELL_WINDOWACTIVATED, L"HSHELL_WINDOWACTIVATED" },
        { HSHELL_GETMINRECT, L"HSHELL_GETMINRECT" },
        { HSHELL_REDRAW, L"HSHELL_REDRAW" },
        { HSHELL_TASKMAN, L"HSHELL_TASKMAN" },
        { HSHELL_LANGUAGE, L"HSHELL_LANGUAGE" },
        { HSHELL_SYSMENU, L"HSHELL_SYSMENU" },
        { HSHELL_ENDTASK, L"HSHELL_ENDTASK" },
        { HSHELL_ACCESSIBILITYSTATE, L"HSHELL_ACCESSIBILITYSTATE" },
        { HSHELL_APPCOMMAND, L"HSHELL_APPCOMMAND" },
        { HSHELL_WINDOWREPLACED, L"HSHELL_WINDOWREPLACED" },
        { HSHELL_WINDOWREPLACING, L"HSHELL_WINDOWREPLACING" },
        { HSHELL_RUDEAPPACTIVATED, L"HSHELL_RUDEAPPACTIVATED" },
};
#endif

typedef struct _TASK_GROUP
{
    /* We have to use a linked list instead of an array so we don't have to
       update all pointers to groups in the task item array when removing
       groups. */
    struct _TASK_GROUP *Next;

    DWORD dwTaskCount;
    DWORD dwProcessId;
    INT Index;
    INT IconIndex;
    union
    {
        DWORD dwFlags;
        struct
        {

            DWORD IsCollapsed : 1;
            DWORD IsRightClick : 1;
            DWORD IsTextInited : 1;
        };
    };
} TASK_GROUP, *PTASK_GROUP;

typedef struct _TASK_ITEM
{
    HWND hWnd;
    PTASK_GROUP Group;
    INT Index;
    INT GroupIndex;
    INT IconIndex;
    INT GroupIconIndex;

    union
    {
        DWORD dwFlags;
        struct
        {

            /* IsFlashing is TRUE when the task bar item should be flashing. */
            DWORD IsFlashing : 1;

            /* RenderFlashed is only TRUE if the task bar item should be
               drawn with a flash. */
            DWORD RenderFlashed : 1;
        };
    };
} TASK_ITEM, *PTASK_ITEM;


class CHardErrorThread
{
    DWORD m_ThreadId;
    HANDLE m_hThread;
    LONG m_bThreadRunning;
    DWORD m_Status;
    DWORD m_dwType;
    CStringW m_Title;
    CStringW m_Text;
public:

    CHardErrorThread():
        m_ThreadId(0),
        m_hThread(NULL),
        m_bThreadRunning(FALSE),
        m_Status(NULL),
        m_dwType(NULL)
    {
    }

    ~CHardErrorThread()
    {
        if (m_bThreadRunning)
        {
            /* Try to unstuck Show */
            PostThreadMessage(m_ThreadId, WM_QUIT, 0, 0);
            DWORD ret = WaitForSingleObject(m_hThread, 3*1000);
            if (ret == WAIT_TIMEOUT)
                TerminateThread(m_hThread, 0);
            CloseHandle(m_hThread);
        }
    }

    HRESULT ThreadProc()
    {
        HRESULT hr;
        CComPtr<IUserNotification> pnotification;

        hr = OleInitialize(NULL);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = CoCreateInstance(CLSID_UserNotification,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_PPV_ARG(IUserNotification, &pnotification));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = pnotification->SetBalloonInfo(m_Title, m_Text, NIIF_WARNING);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = pnotification->SetIconInfo(NULL, NULL);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        /* Show will block until the balloon closes */
        hr = pnotification->Show(NULL, 0);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return S_OK;
    }

    static DWORD CALLBACK s_HardErrorThreadProc(IN OUT LPVOID lpParameter)
    {
        CHardErrorThread* pThis = reinterpret_cast<CHardErrorThread*>(lpParameter);
        pThis->ThreadProc();
        CloseHandle(pThis->m_hThread);
        OleUninitialize();
        InterlockedExchange(&pThis->m_bThreadRunning, FALSE);
        return 0;
    }

    void StartThread(PBALLOON_HARD_ERROR_DATA pData)
    {
        BOOL bIsRunning = InterlockedExchange(&m_bThreadRunning, TRUE);

        /* Ignore the new message if we are already showing one */
        if (bIsRunning)
            return;

        m_Status = pData->Status;
        m_dwType = pData->dwType;
        m_Title = (PWCHAR)((ULONG_PTR)pData + pData->TitleOffset);
        m_Text = (PWCHAR)((ULONG_PTR)pData + pData->MessageOffset);
        m_hThread = CreateThread(NULL, 0, s_HardErrorThreadProc, this, 0, &m_ThreadId);
        if (!m_hThread)
        {
            m_bThreadRunning = FALSE;
            CloseHandle(m_hThread);
        }
    }
};

class CTaskSwitchWnd;

HRESULT CGroupMenuSite_CreateInstance(IN OUT CTaskSwitchWnd *pWnd, const IID & riid, PVOID * ppv);

class CShellMenuCallback :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellMenuCallback
{
private:
    IShellMenu* pShellMenu;
    HMENU hMenu;
    CTaskSwitchWnd *pTaskSwitchWnd;

public:

    DECLARE_NOT_AGGREGATABLE(CShellMenuCallback)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CShellMenuCallback)
        COM_INTERFACE_ENTRY_IID(IID_IShellMenuCallback, IShellMenuCallback)
    END_COM_MAP()

    void Initialize(
        IShellMenu* pShellMenu,
        HMENU hMenu,
        CTaskSwitchWnd *pTaskSwitchWnd)
    {
        this->pShellMenu = pShellMenu;
        this->hMenu = hMenu;
        this->pTaskSwitchWnd = pTaskSwitchWnd;
    }

    ~CShellMenuCallback()
    {
    }

    HRESULT OnGetObject(
        LPSMDATA psmd,
        REFIID iid,
        void ** pv);

    HRESULT STDMETHODCALLTYPE CallbackSM(
        LPSMDATA psmd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam);
};

class CTaskToolbar :
    public CWindowImplBaseT< CToolbar<TASK_ITEM>, CControlWinTraits >
{
public:
    INT UpdateTbButtonSpacing(IN BOOL bHorizontal, IN BOOL bThemed, IN UINT uiRows = 0, IN UINT uiBtnsPerLine = 0)
    {
        TBMETRICS tbm;

        tbm.cbSize = sizeof(tbm);
        tbm.dwMask = TBMF_BARPAD | TBMF_BUTTONSPACING;

        tbm.cxBarPad = tbm.cyBarPad = 0;

        if (bThemed)
        {
            tbm.cxButtonSpacing = 0;
            tbm.cyButtonSpacing = 0;
        }
        else
        {
            if (bHorizontal || uiBtnsPerLine > 1)
                tbm.cxButtonSpacing = (3 * GetSystemMetrics(SM_CXEDGE) / 2);
            else
                tbm.cxButtonSpacing = 0;

            if (!bHorizontal || uiRows > 1)
                tbm.cyButtonSpacing = (3 * GetSystemMetrics(SM_CYEDGE) / 2);
            else
                tbm.cyButtonSpacing = 0;
        }

        SetMetrics(&tbm);

        return tbm.cxButtonSpacing;
    }

    VOID BeginUpdate()
    {
        SetRedraw(FALSE);
    }

    VOID EndUpdate()
    {
        SendMessageW(WM_SETREDRAW, TRUE);
        InvalidateRect(NULL, TRUE);
    }

    BOOL SetButtonCommandId(IN INT iButtonIndex, IN INT iCommandId)
    {
        TBBUTTONINFO tbbi;

        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_BYINDEX | TBIF_COMMAND;
        tbbi.idCommand = iCommandId;

        return SetButtonInfo(iButtonIndex, &tbbi) != 0;
    }

    LRESULT OnNcHitTestToolbar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        POINT pt;

        /* See if the mouse is on a button */
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ScreenToClient(&pt);

        INT index = HitTest(&pt);
        if (index < 0)
        {
            /* Make the control appear to be transparent outside of any buttons */
            return HTTRANSPARENT;
        }

        bHandled = FALSE;
        return 0;
    }

public:
    BEGIN_MSG_MAP(CNotifyToolbar)
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTestToolbar)
    END_MSG_MAP()

    BOOL Initialize(HWND hWndParent)
    {
        DWORD styles = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN |
            TBSTYLE_TOOLTIPS | TBSTYLE_WRAPABLE | TBSTYLE_LIST | TBSTYLE_TRANSPARENT |
            CCS_TOP | CCS_NORESIZE | CCS_NODIVIDER;

        // HACK & FIXME: CORE-18016
        HWND toolbar = CToolbar::Create(hWndParent, styles);
        m_hWnd = NULL;
        return SubclassWindow(toolbar);
    }
};

BOOL
GetVersionInfoString(IN LPCWSTR szFileName,
                     IN LPCWSTR szVersionInfo,
                     OUT LPWSTR szBuffer,
                     IN UINT cbBufLen);

BOOL GetProcessPath(IN DWORD dwProcessId,
                    OUT LPWSTR szBuffer,
                    IN DWORD cbBufLen);

class CTaskSwitchWnd :
    public CComCoClass<CTaskSwitchWnd>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl < CTaskSwitchWnd, CWindow, CControlWinTraits >,
    public IOleWindow
{
    CTaskToolbar m_TaskBar;

    CComPtr<ITrayWindow> m_Tray;

    UINT m_ShellHookMsg;

    WORD m_TaskItemCount;
    WORD m_AllocatedTaskItems;

    PTASK_GROUP m_TaskGroups;
    PTASK_ITEM m_TaskItems;
    PTASK_ITEM m_ActiveTaskItem;

    HTHEME m_Theme;
    UINT m_ButtonsPerLine;
    WORD m_ButtonCount;
    WORD m_DefaultButtonCount;

    HIMAGELIST m_ImageList;

    BOOL m_IsGroupingEnabled;
    BOOL m_IsDestroying;

    BOOL m_CloseTaskGroupOpen;

    SIZE m_ButtonSize;

    HIMAGELIST m_TaskGroupImageList;

    UINT m_uHardErrorMsg;
    CHardErrorThread m_HardErrorThread;

    INT TaskGroupOpened = -1;
    CComPtr<IShellMenu2> shellMenu;
    CComPtr<IMenuPopup> menuPopup;
    CComPtr<IBandSite> bandSite;

public:
    CTaskSwitchWnd() :
        m_ShellHookMsg(NULL),
        m_TaskItemCount(0),
        m_AllocatedTaskItems(0),
        m_TaskGroups(NULL),
        m_TaskItems(NULL),
        m_ActiveTaskItem(NULL),
        m_Theme(NULL),
        m_ButtonsPerLine(0),
        m_ButtonCount(0),
        m_DefaultButtonCount(0),
        m_ImageList(NULL),
        m_IsGroupingEnabled(FALSE),
        m_IsDestroying(FALSE),
        m_CloseTaskGroupOpen(FALSE),
        m_TaskGroupImageList(NULL)
    {
        ZeroMemory(&m_ButtonSize, sizeof(m_ButtonSize));
        m_uHardErrorMsg = RegisterWindowMessageW(L"HardError");
    }
    virtual ~CTaskSwitchWnd() { }

    INT GetWndTextFromTaskItem(IN PTASK_ITEM TaskItem, LPWSTR szBuf, DWORD cchBuf)
    {
        /* Get the window text without sending a message so we don't hang if an
           application isn't responding! */
        return InternalGetWindowText(TaskItem->hWnd, szBuf, cchBuf);
    }


#if DUMP_TASKS != 0
    VOID DumpTasks()
    {
        PTASK_GROUP CurrentGroup;
        PTASK_ITEM CurrentTaskItem, LastTaskItem;

        TRACE("Tasks dump:\n");
        if (m_IsGroupingEnabled)
        {
            CurrentGroup = m_TaskGroups;
            while (CurrentGroup != NULL)
            {
                TRACE("- Group PID: 0x%p Tasks: %d Index: %d\n", CurrentGroup->dwProcessId, CurrentGroup->dwTaskCount, CurrentGroup->Index);

                CurrentTaskItem = m_TaskItems;
                LastTaskItem = CurrentTaskItem + m_TaskItemCount;
                while (CurrentTaskItem != LastTaskItem)
                {
                    if (CurrentTaskItem->Group == CurrentGroup)
                    {
                        TRACE("  + Task hwnd: 0x%p Index: %d\n", CurrentTaskItem->hWnd, CurrentTaskItem->Index);
                    }
                    CurrentTaskItem++;
                }

                CurrentGroup = CurrentGroup->Next;
            }

            CurrentTaskItem = m_TaskItems;
            LastTaskItem = CurrentTaskItem + m_TaskItemCount;
            while (CurrentTaskItem != LastTaskItem)
            {
                if (CurrentTaskItem->Group == NULL)
                {
                    TRACE("- Task hwnd: 0x%p Index: %d\n", CurrentTaskItem->hWnd, CurrentTaskItem->Index);
                }
                CurrentTaskItem++;
            }
        }
        else
        {
            CurrentTaskItem = m_TaskItems;
            LastTaskItem = CurrentTaskItem + m_TaskItemCount;
            while (CurrentTaskItem != LastTaskItem)
            {
                TRACE("- Task hwnd: 0x%p Index: %d\n", CurrentTaskItem->hWnd, CurrentTaskItem->Index);
                CurrentTaskItem++;
            }
        }
    }
#endif

    VOID UpdateIndexesAfter(IN INT iIndex, BOOL bInserted)
    {
        PTASK_GROUP CurrentGroup;
        PTASK_ITEM CurrentTaskItem, LastTaskItem;
        INT NewIndex;

        int offset = bInserted ? +1 : -1;

        if (m_IsGroupingEnabled)
        {
            /* Update all affected groups */
            CurrentGroup = m_TaskGroups;
            while (CurrentGroup != NULL)
            {
                if (CurrentGroup->IsCollapsed &&
                    CurrentGroup->Index >= iIndex)
                {
                    /* Update the toolbar buttons */
                    NewIndex = CurrentGroup->Index + offset;
                    if (m_TaskBar.SetButtonCommandId(CurrentGroup->Index + offset, NewIndex))
                    {
                        CurrentGroup->Index = NewIndex;
                    }
                    else
                        CurrentGroup->Index = -1;
                }

                CurrentGroup = CurrentGroup->Next;
            }
        }

        /* Update all affected task items */
        CurrentTaskItem = m_TaskItems;
        LastTaskItem = CurrentTaskItem + m_TaskItemCount;
        while (CurrentTaskItem != LastTaskItem)
        {
            CurrentGroup = CurrentTaskItem->Group;
            if (CurrentGroup != NULL)
            {
                if (!CurrentGroup->IsCollapsed &&
                    CurrentTaskItem->Index >= iIndex)
                {
                    goto UpdateTaskItemBtn;
                }
            }
            else if (CurrentTaskItem->Index >= iIndex)
            {
            UpdateTaskItemBtn:
                /* Update the toolbar buttons */
                NewIndex = CurrentTaskItem->Index + offset;
                if (m_TaskBar.SetButtonCommandId(CurrentTaskItem->Index + offset, NewIndex))
                {
                    CurrentTaskItem->Index = NewIndex;
                }
                else
                    CurrentTaskItem->Index = -1;
            }

            CurrentTaskItem++;
        }
    }

    VOID RegenerateTaskGroupMenu(PTASK_GROUP TaskGroup) {

        if(TaskGroupOpened != -1 || TaskGroup == NULL) {
            CloseOpenedTaskGroup(FALSE);
            TaskGroupOpened = -1;
        }

        if(m_IsDestroying) return;

        HMENU hMenu;
        if ((hMenu = CreatePopupMenu()) == NULL)
        {
            TRACE("Failed to create Taskbar popup menu(CreatePopupMenu)\n");
            return;
        }

        RECTL test;
        RECTL test2;

        m_TaskBar.GetWindowRect((RECT*)&test);
        m_TaskBar.GetItemRect(TaskGroup->Index, (RECT*)&test2);

        PTASK_ITEM TaskItem, LastTaskItem = NULL;

        TaskItem = m_TaskItems;
        LastTaskItem = TaskItem + m_TaskItemCount;

        INT CurrentIdx = 0;
        while (TaskItem != LastTaskItem)
        {
            if (TaskItem->Group == TaskGroup && TaskItem->Index != -2)
            {
                WCHAR windowText[255];

                GetWndTextFromTaskItem(TaskItem, windowText, _countof(windowText));

                InsertMenuW(hMenu, 0, MF_BYCOMMAND | MF_STRING | MF_UNCHECKED | MF_ENABLED, CurrentIdx, windowText);
                TaskItem->GroupIndex = CurrentIdx;

                CurrentIdx++;
            }

            TaskItem++;
        }

        /* Create the popup */
        HRESULT hr;
        if(shellMenu == NULL) {
            hr = CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellMenu2, &shellMenu));
            hr = CoCreateInstance(CLSID_MenuDeskBar, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IMenuPopup, &menuPopup));
            hr = CoCreateInstance(CLSID_MenuBandSite, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IBandSite, &bandSite));

            if(!shellMenu || !menuPopup || !SUCCEEDED(hr))
            {
                TRACE("Failed to create Taskbar popup menu(CoCreateInstance)\n");
                return;
            }

            CComPtr<IUnknown> pSms;
            hr = CGroupMenuSite_CreateInstance(this, IID_PPV_ARG(IUnknown, &pSms));
            if (FAILED_UNEXPECTEDLY(hr))
                return;

            CComObject<CShellMenuCallback> *pCallback;
            hr = CComObject<CShellMenuCallback>::CreateInstance(&pCallback);
            if (FAILED_UNEXPECTEDLY(hr))
                return;

            TaskGroupOpened = TaskGroup->Index;

            pCallback->AddRef();
            pCallback->Initialize(shellMenu, hMenu, this); //TODO

            shellMenu->Initialize(pCallback, (UINT)-1, 0, SMINIT_TOPLEVEL | SMINIT_VERTICAL);

            shellMenu->SetMenu(hMenu, NULL, SMSET_TOP);

            menuPopup->SetClient(bandSite);
            bandSite->AddBand(shellMenu);

            IUnknown_SetSite(menuPopup, pSms);

            CComPtr<IInitializeObject> pIo;
            hr = menuPopup->QueryInterface(IID_PPV_ARG(IInitializeObject, &pIo));
            if (SUCCEEDED(hr))
                pIo->Initialize();
        } else {
            TaskGroupOpened = TaskGroup->Index;

            menuPopup->OnSelect(MPOS_FULLCANCEL);

            shellMenu->SetMenu(hMenu, NULL, SMSET_BOTTOM);
        }

        CComPtr<IDeskBand> pIDB;
        hr = shellMenu->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
        if(!SUCCEEDED(hr))
            return;

        HWND hWnd;
        if (!SUCCEEDED(pIDB->GetWindow(&hWnd)))
            return;

        ::PostMessageW(hWnd, TB_SETIMAGELIST, 0, (LPARAM)m_TaskGroupImageList);

        POINTL p = {test.left + test2.left,test.top};
        menuPopup->Popup(&p, &test, MPPF_TOP | MPPF_SETFOCUS);

        m_ActiveTaskItem = NULL;

        m_TaskBar.CheckButton(TaskGroup->Index, TRUE);
    }

    INT GetTaskGroupItemIconIndex(IN INT Index) {
        if (shellMenu == NULL)
            return -1;

        PTASK_ITEM TaskItem, LastTaskItem = NULL;

        TaskItem = m_TaskItems;
        LastTaskItem = TaskItem + m_TaskItemCount;

        while (TaskItem != LastTaskItem)
        {
            if (TaskItem->Group->Index == TaskGroupOpened && TaskItem->GroupIndex == Index)
            {
                if (TaskItem->GroupIconIndex < 0)
                {
                    HICON icon = GetWndIcon(TaskItem->hWnd);
                    if (!icon)
                        icon = static_cast<HICON>(LoadImageW(NULL, MAKEINTRESOURCEW(OIC_SAMPLE), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE));

                    TaskItem->GroupIconIndex = ImageList_ReplaceIcon(m_TaskGroupImageList, -1, icon);
                }

                return TaskItem->GroupIconIndex;
            }

            TaskItem++;
        }

        return -1;
    }

    INT UpdateTaskGroupButton(IN PTASK_GROUP TaskGroup)
    {
        ASSERT(TaskGroup->Index >= 0);

        TBBUTTONINFO tbbi = { 0 };
        HICON icon;
        WCHAR windowText[255];

        /* Open process to retrieve the filename of the executable */
        WCHAR ExePath[MAX_PATH] = {};
        if(GetProcessPath(TaskGroup->dwProcessId, ExePath, _countof(ExePath)))
        {
            if(GetVersionInfoString(ExePath, L"FileDescription", windowText, _countof(windowText)))
                tbbi.pszText = windowText;

            if (ExtractIconExW(ExePath, 0, NULL, &icon, 1) <= 0)
                icon = static_cast<HICON>(LoadImageW(NULL, MAKEINTRESOURCEW(OIC_SAMPLE), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE));
        }

        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_BYINDEX | TBIF_STATE | TBIF_TEXT | TBIF_IMAGE;
        tbbi.fsState = TBSTATE_ENABLED;
        if ((m_ActiveTaskItem != NULL && m_ActiveTaskItem->Group == TaskGroup) || TaskGroup->IsRightClick)
            tbbi.fsState |= TBSTATE_CHECKED;

        /* Check if we're updating a button that is the last one in the
           line. If so, we need to set the TBSTATE_WRAP flag! */
        if (!m_Tray->IsHorizontal() || (m_ButtonsPerLine != 0 &&
            (TaskGroup->Index + 1) % m_ButtonsPerLine == 0))
        {
            tbbi.fsState |= TBSTATE_WRAP;
        }

        TaskGroup->IconIndex = ImageList_ReplaceIcon(m_ImageList, TaskGroup->IconIndex, icon);
        tbbi.iImage = TaskGroup->IconIndex;

        TaskGroup->IsTextInited = TRUE;

        if (!m_TaskBar.SetButtonInfo(TaskGroup->Index, &tbbi))
        {
            TaskGroup->Index = -1;
            return -1;
        }

        TRACE("Updated button %d for task group 0x%p\n", TaskGroup->Index, TaskGroup);

        return TaskGroup->Index;
    }

    VOID ExpandTaskGroup(IN PTASK_GROUP TaskGroup)
    {
        ASSERT(TaskGroup->dwTaskCount > 0);
        ASSERT(TaskGroup->IsCollapsed);
        ASSERT(TaskGroup->Index >= 0);

        if (TaskGroupOpened != -1) {
            if (TaskGroup->Index != TaskGroupOpened)
            {
                RegenerateTaskGroupMenu(FindTaskGroupByIndex(TaskGroupOpened));
            }
            else
            {
                CloseOpenedTaskGroup(FALSE);
                TaskGroupOpened = -1;
            }
        }

        /* Get the items associated to this group and delete the index */
        PTASK_ITEM TaskItem, LastTaskItem = NULL;
        INT iIndex;

        TaskGroup->IsCollapsed = FALSE;

        m_TaskBar.BeginUpdate();

        //ICON TODO Remove it
        iIndex = TaskGroup->Index;
        if (m_TaskBar.DeleteButton(iIndex))
        {
            TaskGroup->Index = -1;
            m_ButtonCount--;

            UpdateIndexesAfter(iIndex, FALSE);

            /* Update button sizes and fix the button wrapping */
            UpdateButtonsSize(TRUE);

            TaskItem = m_TaskItems;
            LastTaskItem = TaskItem + m_TaskItemCount;

            while (TaskItem != LastTaskItem)
            {
                if (TaskItem->Group == TaskGroup &&
                    TaskItem->Index < 0 && TaskItem->Index != -2)
                {
                    AddTaskItemButton(TaskItem, &iIndex);
                    iIndex++;
                }

                TaskItem++;
            }

            return;
        }

        m_TaskBar.EndUpdate();
    }

    HICON GetWndIcon(HWND hwnd)
    {
        HICON hIcon = NULL;

        /* Retrieve icon by sending a message */
#define GET_ICON(type) \
    SendMessageTimeout(hwnd, WM_GETICON, (type), 0, SMTO_NOTIMEOUTIFNOTHUNG, 100, (PDWORD_PTR)&hIcon)

        LRESULT bAlive = GET_ICON(g_TaskbarSettings.bSmallIcons ? ICON_SMALL2 : ICON_BIG);
        if (hIcon)
            return hIcon;

        if (bAlive)
        {
            bAlive = GET_ICON(ICON_SMALL);
            if (hIcon)
                return hIcon;
        }

        if (bAlive)
        {
            GET_ICON(g_TaskbarSettings.bSmallIcons ? ICON_BIG : ICON_SMALL2);
            if (hIcon)
                return hIcon;
        }
#undef GET_ICON

        /* If we failed, retrieve icon from the window class */
        hIcon = (HICON)GetClassLongPtr(hwnd, g_TaskbarSettings.bSmallIcons ? GCLP_HICONSM : GCLP_HICON);
        if (hIcon)
            return hIcon;

        return (HICON)GetClassLongPtr(hwnd, g_TaskbarSettings.bSmallIcons ? GCLP_HICON : GCLP_HICONSM);
    }

    INT UpdateTaskItemButton(IN PTASK_ITEM TaskItem)
    {
        TBBUTTONINFO tbbi = { 0 };
        HICON icon;
        WCHAR windowText[255];

        ASSERT(TaskItem->Index >= 0);

        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_BYINDEX | TBIF_STATE | TBIF_TEXT | TBIF_IMAGE;
        tbbi.fsState = TBSTATE_ENABLED;
        if (m_ActiveTaskItem == TaskItem)
            tbbi.fsState |= TBSTATE_CHECKED;

        if (TaskItem->RenderFlashed)
            tbbi.fsState |= TBSTATE_MARKED;

        /* Check if we're updating a button that is the last one in the
           line. If so, we need to set the TBSTATE_WRAP flag! */
        if (!m_Tray->IsHorizontal() || (m_ButtonsPerLine != 0 &&
            (TaskItem->Index + 1) % m_ButtonsPerLine == 0))
        {
            tbbi.fsState |= TBSTATE_WRAP;
        }

        if (GetWndTextFromTaskItem(TaskItem, windowText, _countof(windowText)) > 0)
        {
            tbbi.pszText = windowText;
        }

        icon = GetWndIcon(TaskItem->hWnd);
        if (!icon)
            icon = static_cast<HICON>(LoadImageW(NULL, MAKEINTRESOURCEW(OIC_SAMPLE), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE));
        TaskItem->IconIndex = ImageList_ReplaceIcon(m_ImageList, TaskItem->IconIndex, icon);
        tbbi.iImage = TaskItem->IconIndex;

        if (!m_TaskBar.SetButtonInfo(TaskItem->Index, &tbbi))
        {
            TaskItem->Index = -1;
            return -1;
        }

        TRACE("Updated button %d for hwnd 0x%p\n", TaskItem->Index, TaskItem->hWnd);
        return TaskItem->Index;
    }

    VOID RemoveIcon(IN PTASK_ITEM TaskItem)
    {
        TBBUTTONINFO tbbi;
        PTASK_ITEM currentTaskItem, LastItem;

        if (TaskItem->IconIndex == -1)
            return;

        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_IMAGE;

        currentTaskItem = m_TaskItems;
        LastItem = currentTaskItem + m_TaskItemCount;
        while (currentTaskItem != LastItem)
        {
            if (currentTaskItem->IconIndex > TaskItem->IconIndex)
            {
                currentTaskItem->IconIndex--;
                tbbi.iImage = currentTaskItem->IconIndex;

                m_TaskBar.SetButtonInfo(currentTaskItem->Index, &tbbi);
            }
            currentTaskItem++;
        }

        ImageList_Remove(m_ImageList, TaskItem->IconIndex);
    }

    PTASK_ITEM FindLastTaskItemOfGroup(
        IN PTASK_GROUP TaskGroup  OPTIONAL,
        IN PTASK_ITEM NewTaskItem  OPTIONAL)
    {
        PTASK_ITEM TaskItem, LastTaskItem, FoundTaskItem = NULL;
        DWORD dwTaskCount;

        ASSERT(m_IsGroupingEnabled);

        TaskItem = m_TaskItems;
        LastTaskItem = TaskItem + m_TaskItemCount;

        dwTaskCount = (TaskGroup != NULL ? TaskGroup->dwTaskCount : MAX_TASKS_COUNT);

        ASSERT(dwTaskCount > 0);

        while (TaskItem != LastTaskItem)
        {
            if (TaskItem->Group == TaskGroup)
            {
                if ((NewTaskItem != NULL && TaskItem != NewTaskItem) || NewTaskItem == NULL)
                {
                    FoundTaskItem = TaskItem;
                }

                if (--dwTaskCount == 0)
                {
                    /* We found the last task item in the group! */
                    break;
                }
            }

            TaskItem++;
        }

        return FoundTaskItem;
    }

    INT CalculateTaskItemNewButtonIndex(IN PTASK_ITEM TaskItem)
    {
        PTASK_GROUP TaskGroup;
        PTASK_ITEM LastTaskItem;

        /* NOTE: This routine assumes that the group is *not* collapsed! */

        TaskGroup = TaskItem->Group;
        if (m_IsGroupingEnabled)
        {
            if (TaskGroup != NULL)
            {
                ASSERT(TaskGroup->Index < 0);
                ASSERT(!TaskGroup->IsCollapsed);

                if (TaskGroup->IsCollapsed && TaskGroup->dwTaskCount > 1)
                {
                    LastTaskItem = FindLastTaskItemOfGroup(TaskGroup, TaskItem);
                    if (LastTaskItem != NULL)
                    {
                        /* Since the group is expanded the task items must have an index */
                        ASSERT(LastTaskItem->Index >= 0);

                        return LastTaskItem->Index + 1;
                    }
                }
            }
            else
            {
                /* Find the last NULL group button. NULL groups are added at the end of the
                   task item list when grouping is enabled */
                LastTaskItem = FindLastTaskItemOfGroup(NULL, TaskItem);
                if (LastTaskItem != NULL)
                {
                    ASSERT(LastTaskItem->Index >= 0);

                    return LastTaskItem->Index + 1;
                }
            }
        }

        return m_ButtonCount;
    }

    BOOL CollapseTaskGroup(PTASK_GROUP TaskGroup) {
        ASSERT(m_IsGroupingEnabled);

        if (TaskGroup->IsCollapsed)
            return FALSE;

        /* Get the items associated to this group and delete the index */
        PTASK_ITEM TaskItem, LastTaskItem = NULL;
        WCHAR windowText[255];
        TBBUTTON tbBtn = { 0 };
        INT iIndex = __INT_MAX__;
        HICON icon;

        TaskItem = m_TaskItems;
        LastTaskItem = TaskItem + m_TaskItemCount;

        while (TaskItem != LastTaskItem)
        {
            if (TaskItem->Group == TaskGroup)
            {
                if (TaskItem->Index >= 0)
                {
                    if (TaskItem->Index < iIndex)
                        iIndex = TaskItem->Index;
                    DeleteTaskItemButton(TaskItem);
                }
            }

            TaskItem++;
        }

        /* Open process to retrieve the filename of the executable */
        WCHAR ExePath[MAX_PATH] = {};
        if(GetProcessPath(TaskGroup->dwProcessId, ExePath, _countof(ExePath)))
        {
            if(GetVersionInfoString(ExePath, L"FileDescription", windowText, _countof(windowText))) {
                tbBtn.iString = (DWORD_PTR) windowText;
            }

            if (ExtractIconExW(ExePath, -1, NULL, &icon, 1) <= 0)
                icon = static_cast<HICON>(LoadImageW(NULL, MAKEINTRESOURCEW(OIC_SAMPLE), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE));
        }

        TaskGroup->IconIndex = ImageList_ReplaceIcon(m_ImageList, -1, icon);

        tbBtn.iBitmap = TaskGroup->IconIndex;
        tbBtn.fsState = TBSTATE_ENABLED | TBSTATE_ELLIPSES;
        tbBtn.fsStyle = BTNS_CHECK | BTNS_NOPREFIX | BTNS_SHOWTEXT | BTNS_DROPDOWN | BTNS_WHOLEDROPDOWN;
        tbBtn.dwData = -1;
        tbBtn.idCommand = iIndex;

        m_TaskBar.BeginUpdate();

        if (!m_TaskBar.InsertButton(iIndex, &tbBtn))
        {
            m_TaskBar.EndUpdate();

            return FALSE;
        }

        UpdateIndexesAfter(iIndex, TRUE);

        TaskGroup->Index = iIndex;
        m_ButtonCount++;

        /* Update button sizes and fix the button wrapping */
        UpdateButtonsSize(TRUE);

        TaskGroup->IsCollapsed = TRUE;

        m_TaskBar.EndUpdate();

        return TRUE;
    }

    VOID CollapseOrExpand(BOOL bAdding)
    {
        INT iMaxButtons = GetMaxButtons();

        BOOL bIsLower = m_ButtonCount < iMaxButtons;
        BOOL bIsGreater = bAdding ? (m_ButtonCount >= iMaxButtons) : (m_ButtonCount > iMaxButtons);

        if(m_IsGroupingEnabled && (bIsLower || bIsGreater))
        {
            while(bIsGreater ? (m_ButtonCount >= iMaxButtons) :
                 (m_ButtonCount < iMaxButtons))
            {
                // We expanded the taskbar size, we need to collapse if grouping is enabled

                PTASK_GROUP CurrentGroup = m_TaskGroups;
                PTASK_GROUP MaxOrMinGroup = NULL;
                while (CurrentGroup != NULL)
                {
                    if (bIsGreater != CurrentGroup->IsCollapsed &&
                        bIsGreater == (CurrentGroup->Index < 0) &&
                        CurrentGroup->dwTaskCount > 1)
                    {
                        if(MaxOrMinGroup == NULL ||
                           (bIsGreater && MaxOrMinGroup->dwTaskCount < CurrentGroup->dwTaskCount) ||
                           (bIsLower && MaxOrMinGroup->dwTaskCount > CurrentGroup->dwTaskCount))
                           MaxOrMinGroup = CurrentGroup;
                    }

                    CurrentGroup = CurrentGroup->Next;
                }

                if(MaxOrMinGroup == NULL) return; // We can't collapse more groups

                if(bIsGreater)
                {
                    CollapseTaskGroup(MaxOrMinGroup);
                }
                else if (MaxOrMinGroup->dwTaskCount <= (DWORD)(iMaxButtons - m_ButtonCount + 1))
                {
                    ExpandTaskGroup(MaxOrMinGroup);
                }
                else
                {
                    return;
                }
            }
        }
        else if(!m_IsGroupingEnabled)
        {
            // Expand all the groups

            PTASK_GROUP CurrentGroup = m_TaskGroups;
            while (CurrentGroup != NULL)
            {
                if (CurrentGroup->IsCollapsed)
                {
                    /* Collapse the group */
                    ExpandTaskGroup(CurrentGroup);
                }

                CurrentGroup = CurrentGroup->Next;
            }
        }
    }

    INT AddTaskItemButton(IN OUT PTASK_ITEM TaskItem, IN OPTIONAL PINT pRequiredIndex)
    {
        WCHAR windowText[255];
        TBBUTTON tbBtn = { 0 };
        INT iIndex;
        HICON icon;

        if (TaskItem->Index >= 0)
        {
            return UpdateTaskItemButton(TaskItem);
        }

        /*if (m_IsGroupingEnabled &&
            TaskItem->Group != NULL && !TaskItem->Group->IsCollapsed &&
            TaskItem->Group->dwTaskCount > 2)
        {
            //TODO: Check if a resize is needed
            CollapseTaskGroup(TaskItem->Group);
        }*/

        CollapseOrExpand(TRUE);

        if (TaskItem->Group != NULL &&
            TaskItem->Group->IsCollapsed)
        {
            /* The task group is collapsed, we only need to update the group button */
            return UpdateTaskGroupButton(TaskItem->Group);
        }

        icon = GetWndIcon(TaskItem->hWnd);
        if (!icon)
            icon = static_cast<HICON>(LoadImageW(NULL, MAKEINTRESOURCEW(OIC_SAMPLE), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE));
        TaskItem->IconIndex = ImageList_ReplaceIcon(m_ImageList, -1, icon);

        tbBtn.iBitmap = TaskItem->IconIndex;
        tbBtn.fsState = TBSTATE_ENABLED | TBSTATE_ELLIPSES;
        tbBtn.fsStyle = BTNS_CHECK | BTNS_NOPREFIX | BTNS_SHOWTEXT;
        tbBtn.dwData = TaskItem->Index;

        if (GetWndTextFromTaskItem(TaskItem, windowText, _countof(windowText)) > 0)
        {
            tbBtn.iString = (DWORD_PTR) windowText;
        }

        /* Find out where to insert the new button */
        if (pRequiredIndex != NULL)
            iIndex = *pRequiredIndex;
        else
            iIndex = CalculateTaskItemNewButtonIndex(TaskItem);
        ASSERT(iIndex >= 0);
        tbBtn.idCommand = iIndex;

        m_TaskBar.BeginUpdate();

        if (m_TaskBar.InsertButton(iIndex, &tbBtn))
        {
            UpdateIndexesAfter(iIndex, TRUE);

            TRACE("Added button %d for hwnd 0x%p\n", iIndex, TaskItem->hWnd);

            TaskItem->Index = iIndex;
            m_ButtonCount++;

            /* Update button sizes and fix the button wrapping */
            UpdateButtonsSize(TRUE);
            return iIndex;
        }

        m_TaskBar.EndUpdate();

        return -1;
    }

    BOOL DeleteTaskItemButton(IN OUT PTASK_ITEM TaskItem)
    {
        PTASK_GROUP TaskGroup;
        INT iIndex;

        TaskGroup = TaskItem->Group;

        if (TaskItem->Index >= 0)
        {
            if ((TaskGroup != NULL && !TaskGroup->IsCollapsed) ||
                TaskGroup == NULL)
            {
                m_TaskBar.BeginUpdate();

                RemoveIcon(TaskItem);
                iIndex = TaskItem->Index;
                if (m_TaskBar.DeleteButton(iIndex))
                {
                    TaskItem->Index = -1;
                    m_ButtonCount--;

                    UpdateIndexesAfter(iIndex, FALSE);

                    /* Update button sizes and fix the button wrapping */
                    UpdateButtonsSize(TRUE);
                    return TRUE;
                }

                m_TaskBar.EndUpdate();
            }
        }

        return FALSE;
    }

    PTASK_GROUP AddToTaskGroup(IN HWND hWnd)
    {
        DWORD dwProcessId;
        PTASK_GROUP TaskGroup, *PrevLink;

        if (!GetWindowThreadProcessId(hWnd,
            &dwProcessId))
        {
            TRACE("Cannot get process id of hwnd 0x%p\n", hWnd);
            return NULL;
        }

        WCHAR ItemExePath[MAX_PATH] = {0};
        GetProcessPath(dwProcessId, ItemExePath, _countof(ItemExePath));

        /* Try to find an existing task group */
        TaskGroup = m_TaskGroups;
        PrevLink = &m_TaskGroups;
        while (TaskGroup != NULL)
        {
            if (TaskGroup->dwProcessId == dwProcessId)
            {
                TaskGroup->dwTaskCount++;
                return TaskGroup;
            }
            else
            {
                WCHAR ExePath[MAX_PATH] = {0};
                GetProcessPath(dwProcessId, ExePath, _countof(ExePath));

                if(!lstrcmpW(ExePath, ItemExePath))
                {
                    TaskGroup->dwTaskCount++;
                    return TaskGroup;
                }
            }

            PrevLink = &TaskGroup->Next;
            TaskGroup = TaskGroup->Next;
        }

        /* Allocate a new task group */
        TaskGroup = (PTASK_GROUP) HeapAlloc(hProcessHeap,
            HEAP_ZERO_MEMORY,
            sizeof(*TaskGroup));
        if (TaskGroup != NULL)
        {
            TaskGroup->dwTaskCount = 1;
            TaskGroup->dwProcessId = dwProcessId;
            TaskGroup->Index = -1;

            /* Add the task group to the list */
            *PrevLink = TaskGroup;
        }

        return TaskGroup;
    }

    VOID RemoveTaskFromTaskGroup(IN OUT PTASK_ITEM TaskItem)
    {
        PTASK_GROUP TaskGroup, CurrentGroup, *PrevLink;

        TaskGroup = TaskItem->Group;
        if (TaskGroup != NULL)
        {
            DWORD dwNewTaskCount = --TaskGroup->dwTaskCount;
            if (dwNewTaskCount == 0)
            {
                /* Find the previous pointer in the chain */
                CurrentGroup = m_TaskGroups;
                PrevLink = &m_TaskGroups;
                while (CurrentGroup != TaskGroup)
                {
                    PrevLink = &CurrentGroup->Next;
                    CurrentGroup = CurrentGroup->Next;
                }

                /* Remove the group from the list */
                ASSERT(TaskGroup == CurrentGroup);
                *PrevLink = TaskGroup->Next;

                /* Free the task group */
                HeapFree(hProcessHeap,
                    0,
                    TaskGroup);

                CollapseOrExpand(FALSE);
            }
            else if (TaskGroup->IsCollapsed &&
                TaskGroup->Index >= 0)
            {
                TaskItem->Index = -2;

                if (dwNewTaskCount > 1 && m_IsGroupingEnabled)
                {
                    CollapseOrExpand(FALSE);
                    if (TaskGroup->Index == TaskGroupOpened) {
                        if (TaskGroup->IsCollapsed)
                        {
                            RegenerateTaskGroupMenu(TaskItem->Group);
                        }
                        else
                        {
                            CloseOpenedTaskGroup(FALSE);
                            TaskGroupOpened = -1;
                        }
                    }
                }
                else if (TaskGroup->IsCollapsed)
                {
                    ExpandTaskGroup(TaskGroup);
                }
            }
        }
    }

    PTASK_ITEM FindTaskItem(IN HWND hWnd)
    {
        PTASK_ITEM TaskItem, LastItem;

        TaskItem = m_TaskItems;
        LastItem = TaskItem + m_TaskItemCount;
        while (TaskItem != LastItem)
        {
            if (TaskItem->hWnd == hWnd)
                return TaskItem;

            TaskItem++;
        }

        return NULL;
    }

    PTASK_ITEM FindOtherTaskItem(IN HWND hWnd)
    {
        PTASK_ITEM LastItem, TaskItem;
        PTASK_GROUP TaskGroup;
        DWORD dwProcessId;

        if (!GetWindowThreadProcessId(hWnd, &dwProcessId))
        {
            return NULL;
        }

        /* Try to find another task that belongs to the same
           process as the given window */
        TaskItem = m_TaskItems;
        LastItem = TaskItem + m_TaskItemCount;
        while (TaskItem != LastItem)
        {
            TaskGroup = TaskItem->Group;
            if (TaskGroup != NULL)
            {
                if (TaskGroup->dwProcessId == dwProcessId)
                    return TaskItem;
            }
            else
            {
                DWORD dwProcessIdTask;

                if (GetWindowThreadProcessId(TaskItem->hWnd,
                    &dwProcessIdTask) &&
                    dwProcessIdTask == dwProcessId)
                {
                    return TaskItem;
                }
            }

            TaskItem++;
        }

        return NULL;
    }

    PTASK_ITEM AllocTaskItem()
    {
        if (m_TaskItemCount >= MAX_TASKS_COUNT)
        {
            /* We need the most significant bit in 16 bit command IDs to indicate whether it
               is a task group or task item. WM_COMMAND limits command IDs to 16 bits! */
            return NULL;
        }

        ASSERT(m_AllocatedTaskItems >= m_TaskItemCount);

        if (m_TaskItemCount == 0)
        {
            m_TaskItems = (PTASK_ITEM) HeapAlloc(hProcessHeap,
                0,
                TASK_ITEM_ARRAY_ALLOC * sizeof(*m_TaskItems));
            if (m_TaskItems != NULL)
            {
                m_AllocatedTaskItems = TASK_ITEM_ARRAY_ALLOC;
            }
            else
                return NULL;
        }
        else if (m_TaskItemCount >= m_AllocatedTaskItems)
        {
            PTASK_ITEM NewArray;
            SIZE_T NewArrayLength, ActiveTaskItemIndex;

            NewArrayLength = m_AllocatedTaskItems + TASK_ITEM_ARRAY_ALLOC;

            NewArray = (PTASK_ITEM) HeapReAlloc(hProcessHeap,
                0,
                m_TaskItems,
                NewArrayLength * sizeof(*m_TaskItems));
            if (NewArray != NULL)
            {
                if (m_ActiveTaskItem != NULL)
                {
                    /* Fixup the ActiveTaskItem pointer */
                    ActiveTaskItemIndex = m_ActiveTaskItem - m_TaskItems;
                    m_ActiveTaskItem = NewArray + ActiveTaskItemIndex;
                }
                m_AllocatedTaskItems = (WORD) NewArrayLength;
                m_TaskItems = NewArray;
            }
            else
                return NULL;
        }

        return m_TaskItems + m_TaskItemCount++;
    }

    VOID FreeTaskItem(IN OUT PTASK_ITEM TaskItem)
    {
        WORD wIndex;

        if (TaskItem == m_ActiveTaskItem)
            m_ActiveTaskItem = NULL;

        wIndex = (WORD) (TaskItem - m_TaskItems);
        if (wIndex + 1 < m_TaskItemCount)
        {
            MoveMemory(TaskItem,
                TaskItem + 1,
                (m_TaskItemCount - wIndex - 1) * sizeof(*TaskItem));
        }

        m_TaskItemCount--;
    }

    VOID DeleteTaskItem(IN OUT PTASK_ITEM TaskItem)
    {
        if (!m_IsDestroying)
        {
            /* Delete the task button from the toolbar */
            DeleteTaskItemButton(TaskItem);
        }

        /* Remove the task from it's group */
        RemoveTaskFromTaskGroup(TaskItem);

        /* Free the task item */
        FreeTaskItem(TaskItem);
    }

    VOID CheckActivateTaskItem(IN OUT PTASK_ITEM TaskItem)
    {
        PTASK_ITEM CurrentTaskItem;
        PTASK_GROUP TaskGroup = NULL;

        CurrentTaskItem = m_ActiveTaskItem;

        if (TaskItem != NULL)
            TaskGroup = TaskItem->Group;

        if (m_IsGroupingEnabled &&
            TaskGroup != NULL &&
            TaskGroup->IsCollapsed)
        {
            m_ActiveTaskItem = TaskItem;

            UpdateTaskGroupButton(TaskGroup);
            /* FIXME */
            return;
        }

        if (CurrentTaskItem != NULL)
        {
            PTASK_GROUP CurrentTaskGroup;

            if (CurrentTaskItem == TaskItem)
                return;

            CurrentTaskGroup = CurrentTaskItem->Group;

            if (m_IsGroupingEnabled &&
                CurrentTaskGroup != NULL &&
                CurrentTaskGroup->IsCollapsed)
            {
                if (CurrentTaskGroup == TaskGroup)
                    return;

                m_ActiveTaskItem = NULL;
                if (CurrentTaskGroup->Index >= 0) // Just to be sure
                {
                    UpdateTaskGroupButton(CurrentTaskGroup);
                }
            }
            else
            {
                m_ActiveTaskItem = NULL;
                if (CurrentTaskItem->Index >= 0)
                {
                    UpdateTaskItemButton(CurrentTaskItem);
                }
            }
        }

        m_ActiveTaskItem = TaskItem;

        if (TaskItem != NULL && TaskItem->Index >= 0)
        {
            UpdateTaskItemButton(TaskItem);
        }
        else if (TaskItem == NULL)
        {
            TRACE("Active TaskItem now NULL\n");
        }
    }

    PTASK_ITEM FindTaskItemByIndex(IN INT Index)
    {
        PTASK_ITEM TaskItem, LastItem;

        TaskItem = m_TaskItems;
        LastItem = TaskItem + m_TaskItemCount;
        while (TaskItem != LastItem)
        {
            if (TaskItem->Index == Index)
                return TaskItem;

            TaskItem++;
        }

        return NULL;
    }

    PTASK_GROUP FindTaskGroupByIndex(IN INT Index)
    {
        PTASK_GROUP CurrentGroup;

        CurrentGroup = m_TaskGroups;
        while (CurrentGroup != NULL)
        {
            if (CurrentGroup->Index == Index)
                break;

            CurrentGroup = CurrentGroup->Next;
        }

        return CurrentGroup;
    }

    PTASK_ITEM FindTaskItemOnOpenedGroup(IN INT Index)
    {
        PTASK_ITEM TaskItem, LastItem;

        TaskItem = m_TaskItems;
        LastItem = TaskItem + m_TaskItemCount;
        while (TaskItem != LastItem)
        {
            if (TaskItem->Group->Index == TaskGroupOpened && TaskItem->GroupIndex == Index)
                return TaskItem;

            TaskItem++;
        }

        return NULL;
    }

    BOOL AddTask(IN HWND hWnd)
    {
        PTASK_ITEM TaskItem;

        if (!::IsWindow(hWnd) || m_Tray->IsSpecialHWND(hWnd))
            return FALSE;

        TaskItem = FindTaskItem(hWnd);
        if (TaskItem == NULL)
        {
            TRACE("Add window 0x%p\n", hWnd);
            TaskItem = AllocTaskItem();
            if (TaskItem != NULL)
            {
                ZeroMemory(TaskItem, sizeof(*TaskItem));
                TaskItem->hWnd = hWnd;
                TaskItem->Index = -1;
                TaskItem->GroupIndex = -1;
                TaskItem->Group = AddToTaskGroup(hWnd);
                TaskItem->GroupIconIndex = -1;

                if (!m_IsDestroying)
                {
                    AddTaskItemButton(TaskItem, NULL);
                }
            }
        }

        return TaskItem != NULL;
    }

    BOOL ActivateTaskItem(IN OUT PTASK_ITEM TaskItem  OPTIONAL)
    {
        if (TaskItem != NULL)
        {
            TRACE("Activate window 0x%p on button %d\n", TaskItem->hWnd, TaskItem->Index);
        }

        CheckActivateTaskItem(TaskItem);
        return FALSE;
    }

    BOOL ActivateTask(IN HWND hWnd)
    {
        PTASK_ITEM TaskItem;

        if (!hWnd)
        {
            return ActivateTaskItem(NULL);
        }

        TaskItem = FindTaskItem(hWnd);
        if (TaskItem == NULL)
        {
            TaskItem = FindOtherTaskItem(hWnd);
        }

        if (TaskItem == NULL)
        {
            WARN("Activate window 0x%p, could not find task\n", hWnd);
            RefreshWindowList();
        }

        return ActivateTaskItem(TaskItem);
    }

    BOOL DeleteTask(IN HWND hWnd)
    {
        PTASK_ITEM TaskItem;

        TaskItem = FindTaskItem(hWnd);
        if (TaskItem != NULL)
        {
            TRACE("Delete window 0x%p on button %d\n", hWnd, TaskItem->Index);
            DeleteTaskItem(TaskItem);
            return TRUE;
        }
        //else
        //TRACE("Failed to delete window 0x%p\n", hWnd);

        return FALSE;
    }

    VOID DeleteAllTasks()
    {
        PTASK_ITEM CurrentTask;

        if (m_TaskItemCount > 0)
        {
            CurrentTask = m_TaskItems + m_TaskItemCount;
            do
            {
                DeleteTaskItem(--CurrentTask);
            } while (CurrentTask != m_TaskItems);
        }
    }

    VOID FlashTaskItem(IN OUT PTASK_ITEM TaskItem)
    {
        TaskItem->RenderFlashed = 1;
        UpdateTaskItemButton(TaskItem);
    }

    BOOL FlashTask(IN HWND hWnd)
    {
        PTASK_ITEM TaskItem;

        TaskItem = FindTaskItem(hWnd);
        if (TaskItem != NULL)
        {
            TRACE("Flashing window 0x%p on button %d\n", hWnd, TaskItem->Index);
            FlashTaskItem(TaskItem);
            return TRUE;
        }

        return FALSE;
    }

    VOID RedrawTaskItem(IN OUT PTASK_ITEM TaskItem)
    {
        PTASK_GROUP TaskGroup;

        TaskGroup = TaskItem->Group;
        if (m_IsGroupingEnabled && TaskGroup != NULL)
        {
            if (TaskGroup->IsCollapsed && TaskGroup->Index >= 0)
            {
                UpdateTaskGroupButton(TaskGroup);
            }
            else if (TaskItem->Index >= 0)
            {
                goto UpdateTaskItem;
            }
        }
        else if (TaskItem->Index >= 0)
        {
        UpdateTaskItem:
            TaskItem->RenderFlashed = 0;
            UpdateTaskItemButton(TaskItem);
        }
    }


    BOOL RedrawTask(IN HWND hWnd)
    {
        PTASK_ITEM TaskItem;

        TaskItem = FindTaskItem(hWnd);
        if (TaskItem != NULL)
        {
            RedrawTaskItem(TaskItem);
            return TRUE;
        }

        return FALSE;
    }

    INT GetMaxButtons() {
        RECT rcClient;
        UINT uiRows, uiBtnsPerLine;
        BOOL Horizontal;

        if (GetClientRect(&rcClient) && !IsRectEmpty(&rcClient))
        {
            Horizontal = m_Tray->IsHorizontal();

            if (Horizontal)
            {
                TBMETRICS tbm = { 0 };
                tbm.cbSize = sizeof(tbm);
                tbm.dwMask = TBMF_BUTTONSPACING;
                m_TaskBar.GetMetrics(&tbm);

                if (m_ButtonSize.cy + tbm.cyButtonSpacing != 0)
                    uiRows = (rcClient.bottom + tbm.cyButtonSpacing) / (m_ButtonSize.cy + tbm.cyButtonSpacing);
                else
                    uiRows = 1;

                if (uiRows == 0)
                    uiRows = 1;

                uiBtnsPerLine = 1;
            }
            else
            {
                uiBtnsPerLine = 1;
                uiRows = m_ButtonCount;
            }

            int cxButtonSpacing = m_TaskBar.UpdateTbButtonSpacing(
                Horizontal, m_Theme != NULL,
                uiRows, m_ButtonsPerLine);

            /* Determine the minimum width of a button */
            if (Horizontal)
            {
                MINIMIZEDMETRICS _mmMetrics;
                _mmMetrics.cbSize = sizeof(MINIMIZEDMETRICS);
                SystemParametersInfo(SPI_GETMINIMIZEDMETRICS, sizeof(_mmMetrics), &_mmMetrics, 0);

                /* Recalculate how many buttons actually fit into one line */
                uiBtnsPerLine = rcClient.right / (_mmMetrics.iWidth + cxButtonSpacing);
                if (uiBtnsPerLine == 0)
                    uiBtnsPerLine++;
            }

            return uiBtnsPerLine * uiRows;
        }

        return -1;
    }

    VOID UpdateButtonsSize(IN BOOL bRedrawDisabled)
    {
        RECT rcClient;
        UINT uiRows, uiMax, uiMin, uiBtnsPerLine, ui;
        LONG NewBtnSize;
        BOOL Horizontal;

        /* Update the size of the image list if needed */
        int cx, cy;
        ImageList_GetIconSize(m_ImageList, &cx, &cy);
        if (cx != GetSystemMetrics(g_TaskbarSettings.bSmallIcons ? SM_CXSMICON : SM_CXICON) ||
            cy != GetSystemMetrics(g_TaskbarSettings.bSmallIcons ? SM_CYSMICON : SM_CYICON))
        {
            ImageList_SetIconSize(m_ImageList,
                                  GetSystemMetrics(g_TaskbarSettings.bSmallIcons ? SM_CXSMICON : SM_CXICON),
                                  GetSystemMetrics(g_TaskbarSettings.bSmallIcons ? SM_CYSMICON : SM_CYICON));

            /* SetIconSize removes all icons so we have to reinsert them */
            PTASK_ITEM TaskItem = m_TaskItems;
            PTASK_ITEM LastTaskItem = m_TaskItems + m_TaskItemCount;
            while (TaskItem != LastTaskItem)
            {
                TaskItem->IconIndex = -1;
                UpdateTaskItemButton(TaskItem);

                TaskItem++;
            }
            m_TaskBar.SetImageList(m_ImageList);
        }

        if (GetClientRect(&rcClient) && !IsRectEmpty(&rcClient))
        {
            if (m_ButtonCount > 0)
            {
                Horizontal = m_Tray->IsHorizontal();

                if (Horizontal)
                {
                    TBMETRICS tbm = { 0 };
                    tbm.cbSize = sizeof(tbm);
                    tbm.dwMask = TBMF_BUTTONSPACING;
                    m_TaskBar.GetMetrics(&tbm);

                    if (m_ButtonSize.cy + tbm.cyButtonSpacing != 0)
                        uiRows = (rcClient.bottom + tbm.cyButtonSpacing) / (m_ButtonSize.cy + tbm.cyButtonSpacing);
                    else
                        uiRows = 1;

                    if (uiRows == 0)
                        uiRows = 1;

                    uiBtnsPerLine = (m_ButtonCount + uiRows - 1) / uiRows;
                }
                else
                {
                    uiBtnsPerLine = 1;
                    uiRows = m_ButtonCount;
                }

                if (!bRedrawDisabled)
                    m_TaskBar.BeginUpdate();

                /* We might need to update the button spacing */
                int cxButtonSpacing = m_TaskBar.UpdateTbButtonSpacing(
                    Horizontal, m_Theme != NULL,
                    uiRows, uiBtnsPerLine);

                /* Determine the minimum and maximum width of a button */
                uiMin = GetSystemMetrics(SM_CXSIZE) + (2 * GetSystemMetrics(SM_CXEDGE));
                if (Horizontal)
                {
                    uiMax = GetSystemMetrics(SM_CXMINIMIZED);

                    /* Calculate the ideal width and make sure it's within the allowed range */
                    NewBtnSize = (rcClient.right - (uiBtnsPerLine * cxButtonSpacing)) / uiBtnsPerLine;

                    if (NewBtnSize < (LONG) uiMin)
                        NewBtnSize = uiMin;
                    if (NewBtnSize >(LONG)uiMax)
                        NewBtnSize = uiMax;

                    /* Recalculate how many buttons actually fit into one line */
                    uiBtnsPerLine = rcClient.right / (NewBtnSize + cxButtonSpacing);
                    if (uiBtnsPerLine == 0)
                        uiBtnsPerLine++;
                }
                else
                {
                    NewBtnSize = uiMax = rcClient.right;
                }

                m_ButtonSize.cx = NewBtnSize;

                m_ButtonsPerLine = uiBtnsPerLine;

                for (ui = 0; ui != m_ButtonCount; ui++)
                {
                    TBBUTTONINFOW tbbi = { 0 };
                    tbbi.cbSize = sizeof(tbbi);
                    tbbi.dwMask = TBIF_BYINDEX | TBIF_SIZE | TBIF_STATE;
                    tbbi.cx = (INT) NewBtnSize;
                    tbbi.fsState = TBSTATE_ENABLED;

                    /* Check if we're updating a button that is the last one in the
                       line. If so, we need to set the TBSTATE_WRAP flag! */
                    if (Horizontal)
                    {
                        if ((ui + 1) % uiBtnsPerLine == 0)
                            tbbi.fsState |= TBSTATE_WRAP;
                    }
                    else
                    {
                        tbbi.fsState |= TBSTATE_WRAP;
                    }

                    PTASK_GROUP TaskGroup = FindTaskGroupByIndex((INT)ui);

                    if ((TaskGroup != NULL && TaskGroup->IsRightClick) ||
                        (m_ActiveTaskItem != NULL &&
                        m_ActiveTaskItem->Index == (INT)ui))
                    {
                        tbbi.fsState |= TBSTATE_CHECKED;
                    }

                    m_TaskBar.SetButtonInfo(ui, &tbbi);
                }
            }
            else
            {
                m_ButtonsPerLine = 0;
                m_ButtonSize.cx = 0;
            }
        }

        // FIXME: This seems to be enabling redraws prematurely, but moving it to its right place doesn't work!
        m_TaskBar.EndUpdate();
    }

    BOOL CALLBACK EnumWindowsProc(IN HWND hWnd)
    {
        if (m_Tray->IsTaskWnd(hWnd))
        {
            TRACE("Adding task for %p...\n", hWnd);
            AddTask(hWnd);
        }
        return TRUE;
    }

    static BOOL CALLBACK s_EnumWindowsProc(IN HWND hWnd, IN LPARAM lParam)
    {
        CTaskSwitchWnd * This = (CTaskSwitchWnd *) lParam;

        return This->EnumWindowsProc(hWnd);
    }

    BOOL RefreshWindowList()
    {
        TRACE("Refreshing window list...\n");
        /* Add all windows to the toolbar */
        return EnumWindows(s_EnumWindowsProc, (LPARAM)this);
    }

    LRESULT OnThemeChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        TRACE("OmThemeChanged\n");

        if (m_Theme)
            CloseThemeData(m_Theme);

        if (IsThemeActive())
            m_Theme = OpenThemeData(m_hWnd, L"TaskBand");
        else
            m_Theme = NULL;

        return TRUE;
    }

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (!m_TaskBar.Initialize(m_hWnd))
            return FALSE;

        SetWindowTheme(m_TaskBar.m_hWnd, L"TaskBand", NULL);

        m_ImageList = ImageList_Create(GetSystemMetrics(g_TaskbarSettings.bSmallIcons ? SM_CXSMICON : SM_CXICON),
                                       GetSystemMetrics(g_TaskbarSettings.bSmallIcons ? SM_CYSMICON : SM_CYICON),
                                       ILC_COLOR32 | ILC_MASK, 0, 1000);
        m_TaskBar.SetImageList(m_ImageList);

        m_TaskGroupImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32 | ILC_MASK, 0, 1000);

        /* Set proper spacing between buttons */
        m_TaskBar.UpdateTbButtonSpacing(m_Tray->IsHorizontal(), m_Theme != NULL);

        /* Register the shell hook */
        m_ShellHookMsg = RegisterWindowMessageW(L"SHELLHOOK");

        TRACE("ShellHookMsg got assigned number %d\n", m_ShellHookMsg);

        RegisterShellHook(m_hWnd, 3); /* 1 if no NT! We're targeting NT so we don't care! */

        RefreshWindowList();

        /* Recalculate the button size */
        UpdateButtonsSize(FALSE);

#if DUMP_TASKS != 0
        SetTimer(hwnd, 1, 5000, NULL);
#endif
        return TRUE;
    }

    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        menuPopup->OnSelect(MPOS_FULLCANCEL);
        shellMenu->SetMenu(NULL, NULL, NULL);

        m_IsDestroying = TRUE;

        /* Unregister the shell hook */
        RegisterShellHook(m_hWnd, FALSE);

        CloseThemeData(m_Theme);
        DeleteAllTasks();
        return TRUE;
    }

    VOID SendPulseToTray(BOOL bDelete, HWND hwndActive)
    {
        HWND hwndTray = m_Tray->GetHWND();
        ::SendMessage(hwndTray, TWM_PULSE, bDelete, (LPARAM)hwndActive);
    }

    BOOL HandleAppCommand(IN WPARAM wParam, IN LPARAM lParam)
    {
        BOOL Ret = FALSE;

        switch (GET_APPCOMMAND_LPARAM(lParam))
        {
        case APPCOMMAND_BROWSER_SEARCH:
            Ret = SHFindFiles(NULL,
                NULL);
            break;

        case APPCOMMAND_BROWSER_HOME:
        case APPCOMMAND_LAUNCH_MAIL:
        default:
            TRACE("Shell app command %d unhandled!\n", (INT) GET_APPCOMMAND_LPARAM(lParam));
            break;
        }

        return Ret;
    }

    LRESULT OnShellHook(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        BOOL Ret = FALSE;

        /* In case the shell hook wasn't registered properly, ignore WM_NULLs*/
        if (uMsg == 0)
        {
            bHandled = FALSE;
            return 0;
        }

        TRACE("Received shell hook message: wParam=%08lx, lParam=%08lx\n", wParam, lParam);

        switch ((INT) wParam)
        {
        case HSHELL_APPCOMMAND:
            Ret = HandleAppCommand(wParam, lParam);
            break;

        case HSHELL_WINDOWCREATED:
            SendPulseToTray(FALSE, (HWND)lParam);
            AddTask((HWND) lParam);
            break;

        case HSHELL_WINDOWDESTROYED:
            /* The window still exists! Delay destroying it a bit */
            SendPulseToTray(TRUE, (HWND)lParam);
            DeleteTask((HWND)lParam);
            break;

        case HSHELL_RUDEAPPACTIVATED:
        case HSHELL_WINDOWACTIVATED:
            SendPulseToTray(FALSE, (HWND)lParam);
            ActivateTask((HWND)lParam);
            break;

        case HSHELL_FLASH:
            FlashTask((HWND) lParam);
            break;

        case HSHELL_REDRAW:
            RedrawTask((HWND) lParam);
            break;

        case HSHELL_TASKMAN:
            ::PostMessage(m_Tray->GetHWND(), TWM_OPENSTARTMENU, 0, 0);
            break;

        case HSHELL_ACTIVATESHELLWINDOW:
            ::SwitchToThisWindow(m_Tray->GetHWND(), TRUE);
            ::SetForegroundWindow(m_Tray->GetHWND());
            break;

        case HSHELL_LANGUAGE:
        case HSHELL_SYSMENU:
        case HSHELL_ENDTASK:
        case HSHELL_ACCESSIBILITYSTATE:
        case HSHELL_WINDOWREPLACED:
        case HSHELL_WINDOWREPLACING:

        case HSHELL_GETMINRECT:
        default:
        {
#if DEBUG_SHELL_HOOK
            int i, found;
            for (i = 0, found = 0; i != _countof(hshell_msg); i++)
            {
                if (hshell_msg[i].msg == (INT) wParam)
                {
                    TRACE("Shell message %ws unhandled (lParam = 0x%p)!\n", hshell_msg[i].msg_name, lParam);
                    found = 1;
                    break;
                }
            }
            if (found)
                break;
#endif
            TRACE("Shell message %d unhandled (lParam = 0x%p)!\n", (INT) wParam, lParam);
            break;
        }
        }

        return Ret;
    }

    VOID HandleTaskItemClick(IN OUT PTASK_ITEM TaskItem)
    {
        BOOL bIsMinimized;
        BOOL bIsActive;

        if (::IsWindow(TaskItem->hWnd))
        {
            bIsMinimized = ::IsIconic(TaskItem->hWnd);
            bIsActive = (TaskItem == m_ActiveTaskItem);

            TRACE("Active TaskItem %p, selected TaskItem %p\n", m_ActiveTaskItem, TaskItem);
            if (m_ActiveTaskItem)
                TRACE("Active TaskItem hWnd=%p, TaskItem hWnd %p\n", m_ActiveTaskItem->hWnd, TaskItem->hWnd);

            TRACE("Valid button clicked. HWND=%p, IsMinimized=%s, IsActive=%s...\n",
                TaskItem->hWnd, bIsMinimized ? "Yes" : "No", bIsActive ? "Yes" : "No");

            if (!bIsMinimized && bIsActive)
            {
                if (!::IsHungAppWindow(TaskItem->hWnd))
                    ::ShowWindowAsync(TaskItem->hWnd, SW_MINIMIZE);
                TRACE("Valid button clicked. App window Minimized.\n");
            }
            else
            {
                ::SwitchToThisWindow(TaskItem->hWnd, TRUE);

                TRACE("Valid button clicked. App window Restored.\n");
            }
        }
    }

    VOID HandleTaskGroupClick(IN OUT PTASK_GROUP TaskGroup)
    {
        if(m_CloseTaskGroupOpen) {
            m_CloseTaskGroupOpen = FALSE;

            m_TaskBar.CheckButton(TaskGroup->Index, FALSE);

            TaskGroupOpened = -1;
        }

        if(TaskGroupOpened != TaskGroup->Index) {
            RegenerateTaskGroupMenu(TaskGroup);

            TaskGroupOpened = TaskGroup->Index;
        } else {
            CloseOpenedTaskGroup(FALSE);
            TaskGroupOpened = -1;
        }
    }

    BOOL HandleButtonClick(IN WORD wIndex)
    {
        PTASK_ITEM TaskItem;
        PTASK_GROUP TaskGroup;

        if (m_IsGroupingEnabled)
        {
            TaskGroup = FindTaskGroupByIndex((INT) wIndex);
            if (TaskGroup != NULL && TaskGroup->IsCollapsed)
            {
                HandleTaskGroupClick(TaskGroup);
                return TRUE;
            }
        }

        TaskItem = FindTaskItemByIndex((INT) wIndex);
        if (TaskItem != NULL)
        {
            SendPulseToTray(FALSE, TaskItem->hWnd);
            HandleTaskItemClick(TaskItem);
            return TRUE;
        }

        return FALSE;
    }

    HRESULT CloseOpenedTaskGroup(BOOL bDontRemoveIndices)
    {
        if(shellMenu == NULL || menuPopup == NULL || bandSite == NULL || TaskGroupOpened < 0) return E_FAIL;

        PTASK_GROUP TaskGroup = FindTaskGroupByIndex(TaskGroupOpened);
        if(TaskGroup == NULL)
            return E_FAIL;

        /*shellMenu->SetSubMenu(menuPopup, FALSE);

        // remove the popup
        shellMenu = NULL;
        menuPopup = NULL;
        bandSite = NULL;*/

        if(!bDontRemoveIndices)
        {
            PTASK_ITEM TaskItem, LastTaskItem = NULL;

            TaskItem = m_TaskItems;
            LastTaskItem = TaskItem + m_TaskItemCount;
            if(TaskGroupOpened != -1) {
                while (TaskItem != LastTaskItem)
                {
                    if (TaskItem->Group == TaskGroup)
                    {
                        TaskItem->GroupIndex = -1;
                    }

                    TaskItem++;
                }
            }

            // Avoid a stack overflow
            menuPopup->OnSelect(MPOS_FULLCANCEL);
            shellMenu->SetMenu(NULL, NULL, NULL);
        } else {
            m_CloseTaskGroupOpen = TRUE;
        }

        // Unmark the button
        m_TaskBar.CheckButton(TaskGroup->Index, FALSE);

        return S_OK;
    }

    BOOL HandleTaskGroupSelection(IN INT iIndex)
    {
        if (m_IsGroupingEnabled && TaskGroupOpened >= 0)
        {
            // You can only have one menu open at the same time
            // so, we can expect that the first non null task group menu is the menu clicked

            // wIndex is the handle of the window, TODO: check this behaviour
            PTASK_GROUP TaskGroup = FindTaskGroupByIndex(TaskGroupOpened);

            PTASK_ITEM TaskItem, LastTaskItem = NULL;
            LastTaskItem = TaskItem + m_TaskItemCount;

            TaskItem = m_TaskItems;

            while (TaskItem != LastTaskItem)
            {
                if (TaskItem->Group == TaskGroup && TaskItem->GroupIndex == iIndex)
                {
                    HandleTaskItemClick(TaskItem);
                    break;
                }

                TaskItem++;
            }

            CloseOpenedTaskGroup(FALSE);
            TaskGroupOpened = -1;
        }

        return FALSE;
    }

    static VOID CALLBACK
    SendAsyncProc(HWND hwnd, UINT uMsg, DWORD_PTR dwData, LRESULT lResult)
    {
        ::PostMessageW(hwnd, WM_NULL, 0, 0);
    }

    VOID HandleTaskItemRightClick(IN OUT PTASK_ITEM TaskItem)
    {
        POINT pt;
        GetCursorPos(&pt);

        SetForegroundWindow(TaskItem->hWnd);

        ActivateTask(TaskItem->hWnd);

        if (GetForegroundWindow() != TaskItem->hWnd)
            ERR("HandleTaskItemRightClick detected the window did not become foreground\n");

        ::SendMessageCallbackW(TaskItem->hWnd, WM_POPUPSYSTEMMENU, 0, MAKELPARAM(pt.x, pt.y),
                               SendAsyncProc, (ULONG_PTR)TaskItem);
    }

    VOID HandleTaskGroupRightClick(IN OUT PTASK_GROUP TaskGroup)
    {
        /* TODO: Show task group right click menu */
        PTASK_ITEM TaskItem, LastTaskItem = NULL;
        BOOL bEnableMinimizeButton = FALSE;
        HMENU hMenu;

        hMenu = LoadPopupMenu(hExplorerInstance, MAKEINTRESOURCEW(IDM_TASKGRP));
        if(!hMenu)
            return;

        // Check if all windows are minimized

        TaskItem = m_TaskItems;
        LastTaskItem = TaskItem + m_TaskItemCount;

        while (TaskItem != LastTaskItem)
        {
            if (TaskItem->Group == TaskGroup && ::IsWindow(TaskItem->hWnd) &&
                !::IsIconic(TaskItem->hWnd))
                bEnableMinimizeButton = TRUE;

            TaskItem++;
        }

        if (!bEnableMinimizeButton)
        {
            EnableMenuItem(hMenu, ID_SHELL_CMD_MINIMIZE_GRP, MF_DISABLED | MF_GRAYED);
        }

        // Check the button
        TaskGroup->IsRightClick = TRUE;
        UpdateTaskGroupButton(TaskGroup);

        POINT pt;
        GetCursorPos(&pt);

        SetForegroundWindow(m_hWnd);

        INT iSelection = TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                               pt.x, pt.y, m_hWnd, NULL);

        TaskGroup->IsRightClick = FALSE;
        UpdateTaskGroupButton(TaskGroup);

        if(!iSelection) return;

        CSimpleArray<HWND> kids;

        TaskItem = m_TaskItems;

        while (TaskItem != LastTaskItem)
        {
            if (TaskItem->Group == TaskGroup)
            {
                if (iSelection != ID_SHELL_CMD_MINIMIZE_GRP && iSelection != ID_SHELL_CMD_CLOSE_GRP)
                {
                    ::ShowWindowAsync(TaskItem->hWnd, SW_RESTORE);
                    kids.Add(TaskItem->hWnd);
                }
                else if (iSelection == ID_SHELL_CMD_MINIMIZE_GRP &&
                    ::IsWindow(TaskItem->hWnd) && !::IsIconic(TaskItem->hWnd))
                {
                    ::ShowWindowAsync(TaskItem->hWnd, SW_MINIMIZE);
                }
                else
                {
                    ::PostMessageW(TaskItem->hWnd, WM_CLOSE, NULL, NULL);
                }
            }

            TaskItem++;
        }

        if (iSelection == ID_SHELL_CMD_CASCADE_WND)
            CascadeWindows(NULL, MDITILE_SKIPDISABLED, NULL, kids.GetSize(), kids.GetData());
        else if (iSelection == ID_SHELL_CMD_TILE_WND_H)
            TileWindows(NULL, MDITILE_HORIZONTAL, NULL, kids.GetSize(), kids.GetData());
        else if (iSelection == ID_SHELL_CMD_TILE_WND_V)
            TileWindows(NULL, MDITILE_VERTICAL, NULL, kids.GetSize(), kids.GetData());
    }

    BOOL HandleButtonRightClick(IN WORD wIndex)
    {
        PTASK_ITEM TaskItem;
        PTASK_GROUP TaskGroup;
        if (m_IsGroupingEnabled)
        {
            TaskGroup = FindTaskGroupByIndex((INT) wIndex);
            if (TaskGroup != NULL && TaskGroup->IsCollapsed)
            {
                HandleTaskGroupRightClick(TaskGroup);
                return TRUE;
            }
        }

        TaskItem = FindTaskItemByIndex((INT) wIndex);

        if (TaskItem != NULL)
        {
            HandleTaskItemRightClick(TaskItem);
            return TRUE;
        }

        return FALSE;
    }


    LRESULT HandleItemPaint(IN OUT NMTBCUSTOMDRAW *nmtbcd)
    {
        LRESULT Ret = CDRF_DODEFAULT;
        PTASK_GROUP TaskGroup;
        PTASK_ITEM TaskItem;

        TaskItem = FindTaskItemByIndex((INT) nmtbcd->nmcd.dwItemSpec);
        TaskGroup = FindTaskGroupByIndex((INT) nmtbcd->nmcd.dwItemSpec);
        if (TaskGroup == NULL && TaskItem != NULL)
        {
            ASSERT(TaskItem != NULL);

            if (TaskItem != NULL && ::IsWindow(TaskItem->hWnd))
            {
                /* Make the entire button flashing if necessary */
                if (nmtbcd->nmcd.uItemState & CDIS_MARKED)
                {
                    Ret = TBCDRF_NOBACKGROUND;
                    if (!m_Theme)
                    {
                        SelectObject(nmtbcd->nmcd.hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                        Rectangle(nmtbcd->nmcd.hdc,
                            nmtbcd->nmcd.rc.left,
                            nmtbcd->nmcd.rc.top,
                            nmtbcd->nmcd.rc.right,
                            nmtbcd->nmcd.rc.bottom);
                    }
                    else
                    {
                        DrawThemeBackground(m_Theme, nmtbcd->nmcd.hdc, TDP_FLASHBUTTON, 0, &nmtbcd->nmcd.rc, 0);
                    }
                    nmtbcd->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                    return Ret;
                }
            }
        }
        else if (TaskGroup != NULL)
        {
            Ret = CDRF_SKIPDEFAULT;
            /* Make the entire button flashing if necessary */
            if (nmtbcd->nmcd.uItemState & CDIS_MARKED)
            {
                if (!m_Theme)
                {
                    SelectObject(nmtbcd->nmcd.hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                    Rectangle(nmtbcd->nmcd.hdc,
                        nmtbcd->nmcd.rc.left,
                        nmtbcd->nmcd.rc.top,
                        nmtbcd->nmcd.rc.right,
                        nmtbcd->nmcd.rc.bottom);
                }
                else
                {
                    DrawThemeBackground(m_Theme, nmtbcd->nmcd.hdc, TDP_FLASHBUTTONGROUPMENU, 0, &nmtbcd->nmcd.rc, 0);
                }
                nmtbcd->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
            }

            WCHAR buttonText[256];

            TBBUTTONINFOW btni;
            btni.cbSize = sizeof(TBBUTTONINFOW);
            btni.dwMask = TBIF_STATE | TBIF_TEXT;
            btni.cchText = 256;
            btni.pszText = buttonText;
            m_TaskBar.GetButtonInfo(TaskGroup->Index, &btni);

            DWORD padding = m_TaskBar.GetPadding();
            WORD paddingCy = HIWORD(padding);
            WORD paddingCx = LOWORD(padding);
            INT bitmapWidth;
            INT bitmapHeight;

            ImageList_GetIconSize(m_ImageList, &bitmapWidth, &bitmapHeight);

            if (!m_Theme)
            {
                if (nmtbcd->nmcd.uItemState & (CDIS_SELECTED | CDIS_CHECKED))
                    DrawEdge (nmtbcd->nmcd.hdc, &nmtbcd->nmcd.rc, EDGE_SUNKEN, BF_RECT | BF_MIDDLE);
                else
                    DrawEdge (nmtbcd->nmcd.hdc, &nmtbcd->nmcd.rc, EDGE_RAISED,
                    BF_SOFT | BF_RECT | BF_MIDDLE);
            }

            RECT rcText = nmtbcd->nmcd.rc;
            RECT rcBitmap = nmtbcd->nmcd.rc;
            RECT rcArrow = nmtbcd->nmcd.rc;
            InflateRect(&rcText, -GetSystemMetrics(SM_CXEDGE), 0);
            rcText.left += bitmapWidth + 6; //TODO Default list gap

            if(btni.fsState & (TBSTATE_PRESSED | TBSTATE_CHECKED))
                OffsetRect(&rcText, 1, 1);

            rcBitmap.left += GetSystemMetrics(SM_CXEDGE) + paddingCx / 2;
            rcBitmap.top += paddingCy / 2;

            if(nmtbcd->nmcd.uItemState & (CDIS_SELECTED | CDIS_CHECKED))
            {
                rcBitmap.left++;
                rcBitmap.top++;
            }

            rcArrow.left = max(rcArrow.left, rcArrow.right - 13);
            rcText.right = rcArrow.left;

            ImageList_Draw(m_ImageList, TaskGroup->Index, nmtbcd->nmcd.hdc, rcBitmap.left, rcBitmap.top, (btni.fsState & CDIS_MARKED) ? ILD_BLEND50 | ILD_TRANSPARENT : ILD_TRANSPARENT);

            WCHAR TaskCountText[20];
            StringCbPrintfW(TaskCountText, 20, L"(%d)", TaskGroup->dwTaskCount);

            NONCLIENTMETRICS ncm = {sizeof(ncm)};
            if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE))
                return Ret;

            ncm.lfCaptionFont.lfWeight = FW_BOLD;
            HFONT newFont = CreateFontIndirect(&ncm.lfCaptionFont);

            HFONT normalFont = m_TaskBar.GetFont();

            int oldBkMode = SetBkMode(nmtbcd->nmcd.hdc, nmtbcd->nStringBkMode);

            HGDIOBJ oldFont = NULL;
            if (!m_Theme)
            {
                oldFont = SelectObject(nmtbcd->nmcd.hdc, newFont);

                SIZE TaskCountTextWidth;
                GetTextExtentPoint32W(nmtbcd->nmcd.hdc, TaskCountText, wcslen(TaskCountText), &TaskCountTextWidth);

                DrawTextW(nmtbcd->nmcd.hdc, TaskCountText, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                rcText.left += TaskCountTextWidth.cx + paddingCx;

                if(!(btni.fsState & (TBSTATE_PRESSED | TBSTATE_CHECKED)))
                    SelectObject(nmtbcd->nmcd.hdc, normalFont);

                DrawTextW(nmtbcd->nmcd.hdc, buttonText, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            }
            else
            {
                RECT textRect;
                GetThemeTextExtent(m_Theme, nmtbcd->nmcd.hdc, TDP_GROUPCOUNT, 0, TaskCountText, -1, DT_LEFT | DT_VCENTER | DT_SINGLELINE, &rcText, &textRect);

                DrawThemeText(m_Theme, nmtbcd->nmcd.hdc, TDP_GROUPCOUNT, 0, TaskCountText, -1, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 0, &rcText);

                // Shrink the text rect
                rcText.left += textRect.right - textRect.left + paddingCx;

                if(btni.fsState & (TBSTATE_PRESSED | TBSTATE_CHECKED))
                    oldFont = SelectObject(nmtbcd->nmcd.hdc, newFont);
                else
                    oldFont = SelectObject(nmtbcd->nmcd.hdc, normalFont);

                DrawThemeText(m_Theme, nmtbcd->nmcd.hdc, TP_BUTTON, 0, buttonText, -1, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, 0, &rcText);
            }
            SetBkMode(nmtbcd->nmcd.hdc, oldBkMode);

            if(oldFont != NULL)
                SelectObject(nmtbcd->nmcd.hdc, oldFont);

            DeleteObject(newFont);

            {
                INT x, y;
                HPEN hPen, hOldPen;

                if (!(hPen = CreatePen( PS_SOLID, 1, nmtbcd->clrText)))
                    return Ret;

                hOldPen = (HPEN)SelectObject(nmtbcd->nmcd.hdc, hPen);
                INT offset = (nmtbcd->nmcd.uItemState & (CDIS_SELECTED | CDIS_CHECKED)) ? 1 : 0;
                x = rcArrow.left + offset + 2;
                y = rcArrow.top + offset + (rcArrow.bottom - rcArrow.top - 3) / 2;
                MoveToEx(nmtbcd->nmcd.hdc, x, y, NULL);
                LineTo(nmtbcd->nmcd.hdc, x+5, y++); x++;
                MoveToEx(nmtbcd->nmcd.hdc, x, y, NULL);
                LineTo(nmtbcd->nmcd.hdc, x+3, y++); x++;
                MoveToEx(nmtbcd->nmcd.hdc, x, y, NULL);
                LineTo(nmtbcd->nmcd.hdc, x+1, y);
                SelectObject(nmtbcd->nmcd.hdc, hOldPen);
                DeleteObject(hPen);
            }
        }
        return Ret;
    }

    LRESULT HandleToolbarNotification(IN const NMHDR *nmh)
    {
        LRESULT Ret = 0;

        switch (nmh->code)
        {
            case NM_CUSTOMDRAW:
            {
                LPNMTBCUSTOMDRAW nmtbcd = (LPNMTBCUSTOMDRAW) nmh;

                switch (nmtbcd->nmcd.dwDrawStage)
                {

                case CDDS_ITEMPREPAINT:
                    Ret = HandleItemPaint(nmtbcd);
                    break;

                case CDDS_PREPAINT:
                    Ret = CDRF_NOTIFYITEMDRAW;
                    break;

                default:
                    Ret = CDRF_DODEFAULT;
                    break;
                }
                break;
            }
            case TBN_DROPDOWN:
            {
                Ret = TBDDRET_TREATPRESSED;
                break;
            }
        }

        return Ret;
    }

    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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

    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SIZE szClient;

        szClient.cx = LOWORD(lParam);
        szClient.cy = HIWORD(lParam);
        if (m_TaskBar.m_hWnd != NULL)
        {
            m_TaskBar.SetWindowPos(NULL, 0, 0, szClient.cx, szClient.cy, SWP_NOZORDER);

            UpdateButtonsSize(FALSE);
        }
        return TRUE;
    }

    LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = TRUE;
        /* We want the tray window to be draggable everywhere, so make the control
        appear transparent */
        Ret = DefWindowProc(uMsg, wParam, lParam);
        if (Ret != HTVSCROLL && Ret != HTHSCROLL)
            Ret = HTTRANSPARENT;
        return Ret;
    }

    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = TRUE;
        if (lParam != 0 && (HWND) lParam == m_TaskBar.m_hWnd)
        {
            Ret = HandleButtonClick(LOWORD(wParam));
        }
        return Ret;
    }

    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = TRUE;
        const NMHDR *nmh = (const NMHDR *) lParam;

        if (nmh->hwndFrom == m_TaskBar.m_hWnd)
        {
            Ret = HandleToolbarNotification(nmh);
        }
        return Ret;
    }

    LRESULT OnUpdateTaskbarPos(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /* Update the button spacing */
        m_TaskBar.UpdateTbButtonSpacing(m_Tray->IsHorizontal(), m_Theme != NULL);

        CollapseOrExpand(FALSE);

        return TRUE;
    }

    LRESULT OnTaskbarSettingsChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        BOOL bSettingsChanged = FALSE;
        TaskbarSettings* newSettings = (TaskbarSettings*)lParam;

        if (newSettings->bGroupButtons != g_TaskbarSettings.bGroupButtons)
        {
            bSettingsChanged = TRUE;
            g_TaskbarSettings.bGroupButtons = newSettings->bGroupButtons;
            m_IsGroupingEnabled = g_TaskbarSettings.bGroupButtons;
        }

        if (newSettings->bSmallIcons != g_TaskbarSettings.bSmallIcons)
        {
            bSettingsChanged = TRUE;
            g_TaskbarSettings.bSmallIcons = newSettings->bSmallIcons;
        }

        if (bSettingsChanged)
        {
            /* Refresh each task item view */
            /* Collapse or expand groups if necessary */
            CollapseOrExpand(FALSE);

            RefreshWindowList();
            UpdateButtonsSize(FALSE);
        }

        return 0;
    }

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = 0;
        INT_PTR iBtn = -1;

        if (m_TaskBar.m_hWnd != NULL)
        {
            POINT pt;

            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            ::ScreenToClient(m_TaskBar.m_hWnd, &pt);

            iBtn = m_TaskBar.HitTest(&pt);
            if (iBtn >= 0)
            {
                HandleButtonRightClick(iBtn);
            }
        }
        if (iBtn < 0)
        {
            /* Not on a taskbar button, so forward message to tray */
            Ret = SendMessage(m_Tray->GetHWND(), uMsg, wParam, lParam);
        }
        return Ret;
    }

    LRESULT OnKludgeItemRect(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        PTASK_ITEM TaskItem = FindTaskItem((HWND) wParam);
        if (TaskItem)
        {
            RECT* prcMinRect = (RECT*) lParam;
            RECT rcItem, rcToolbar;
            m_TaskBar.GetItemRect(TaskItem->Index, &rcItem);
            m_TaskBar.GetWindowRect(&rcToolbar);

            OffsetRect(&rcItem, rcToolbar.left, rcToolbar.top);

            *prcMinRect = rcItem;
            return TRUE;
        }
        return FALSE;
    }

    LRESULT OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return MA_NOACTIVATE;
    }

    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
#if DUMP_TASKS != 0
        switch (wParam)
        {
        case 1:
            DumpTasks();
            break;
        }
#endif
        return TRUE;
    }

    LRESULT OnSetFont(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return m_TaskBar.SendMessageW(uMsg, wParam, lParam);
    }

    LRESULT OnSettingChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (wParam == SPI_SETNONCLIENTMETRICS)
        {
            /*  Don't update the font, this will be done when we get a WM_SETFONT from our parent */
            UpdateButtonsSize(FALSE);
        }

        return 0;
    }

    LRESULT OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        PCOPYDATASTRUCT cpData = (PCOPYDATASTRUCT)lParam;
        if (cpData->dwData == m_uHardErrorMsg)
        {
            /* A hard error balloon message */
            PBALLOON_HARD_ERROR_DATA pData = (PBALLOON_HARD_ERROR_DATA)cpData->lpData;
            ERR("Got balloon data 0x%x, 0x%x, '%S', '%S'\n", pData->Status, pData->dwType, (WCHAR*)((ULONG_PTR)pData + pData->TitleOffset), (WCHAR*)((ULONG_PTR)pData + pData->MessageOffset));
            if (pData->cbHeaderSize == sizeof(BALLOON_HARD_ERROR_DATA))
                m_HardErrorThread.StartThread(pData);
            return TRUE;
        }

        return FALSE;
    }

    HRESULT Initialize(IN HWND hWndParent, IN OUT ITrayWindow *tray)
    {
        m_Tray = tray;
        m_IsGroupingEnabled = g_TaskbarSettings.bGroupButtons;
        Create(hWndParent, 0, szRunningApps, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP);
        if (!m_hWnd)
            return E_FAIL;
        return S_OK;
    }

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

    DECLARE_WND_CLASS_EX(szTaskSwitchWndClass, CS_DBLCLKS, COLOR_3DFACE)

    BEGIN_MSG_MAP(CTaskSwitchWnd)
        MESSAGE_HANDLER(WM_THEMECHANGED, OnThemeChanged)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(TSWM_UPDATETASKBARPOS, OnUpdateTaskbarPos)
        MESSAGE_HANDLER(TWM_SETTINGSCHANGED, OnTaskbarSettingsChanged)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_SETFONT, OnSetFont)
        MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChanged)
        MESSAGE_HANDLER(m_ShellHookMsg, OnShellHook)
        MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate)
        MESSAGE_HANDLER(WM_KLUDGEMINRECT, OnKludgeItemRect)
        MESSAGE_HANDLER(WM_COPYDATA, OnCopyData)
    END_MSG_MAP()

    DECLARE_NOT_AGGREGATABLE(CTaskSwitchWnd)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CTaskSwitchWnd)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
    END_COM_MAP()
};

HRESULT CShellMenuCallback::OnGetObject(
    LPSMDATA psmd,
    REFIID iid,
    void ** pv)
{
    if(IsEqualIID(iid, IID_IContextMenu) && psmd->uId >= 0) {
        PTASK_ITEM pItem = pTaskSwitchWnd->FindTaskItemOnOpenedGroup(psmd->uId);

        if(pItem != NULL)
            pTaskSwitchWnd->HandleTaskItemRightClick(pItem);
    }

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CShellMenuCallback::CallbackSM(
        LPSMDATA psmd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam)
{
    switch(uMsg) {
        case SMC_EXEC:
            {
                pTaskSwitchWnd->HandleTaskGroupSelection(psmd->uId);

                return S_OK;
            }
        case SMC_GETOBJECT:
            {
                return OnGetObject(psmd, *reinterpret_cast<IID *>(wParam), reinterpret_cast<void **>(lParam));
            }
        case SMC_GETINFO:
            {
                SMINFO *pInfo = reinterpret_cast<SMINFO *>(lParam);

                if ((pInfo->dwMask & SMIM_TYPE) != 0)
                    pInfo->dwType = SMIT_STRING;
                if ((pInfo->dwMask & SMIM_FLAGS) != 0)
                    pInfo->dwFlags = SMIF_ICON;
                if ((pInfo->dwMask & SMIM_ICON) != 0)
                    pInfo->iIcon = pTaskSwitchWnd->GetTaskGroupItemIconIndex(psmd->uId);

                return S_OK;
            }
    }

    return S_FALSE;
}

HRESULT CTaskSwitchWnd_CreateInstance(IN HWND hWndParent, IN OUT ITrayWindow *Tray, REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CTaskSwitchWnd>(hWndParent, Tray, riid, ppv);
}


class CGroupMenuSite :
    public CComCoClass<CGroupMenuSite>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IServiceProvider,
    public IOleCommandTarget,
    public IMenuPopup
{
    CComPtr<CTaskSwitchWnd> m_TaskSwitchWnd;

public:
    CGroupMenuSite()
    {
    }

    virtual ~CGroupMenuSite() {}

    /*******************************************************************/

    virtual HRESULT STDMETHODCALLTYPE QueryService(
        IN REFGUID guidService,
        IN REFIID riid,
        OUT PVOID *ppvObject)
    {
        if (IsEqualGUID(guidService, SID_SMenuPopup))
        {
            return QueryInterface(riid, ppvObject);
        }

        return E_NOINTERFACE;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryStatus(
        IN const GUID *pguidCmdGroup  OPTIONAL,
        IN ULONG cCmds,
        IN OUT OLECMD *prgCmds,
        IN OUT OLECMDTEXT *pCmdText  OPTIONAL)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Exec(
        IN const GUID *pguidCmdGroup  OPTIONAL,
        IN DWORD nCmdID,
        IN DWORD nCmdExecOpt,
        IN VARIANTARG *pvaIn  OPTIONAL,
        IN VARIANTARG *pvaOut  OPTIONAL)
    {
        return E_NOTIMPL;
    }

    /*******************************************************************/

    virtual HRESULT STDMETHODCALLTYPE GetWindow(
        OUT HWND *phwnd)
    {
        TRACE("ITrayPriv::GetWindow\n");

        return m_TaskSwitchWnd->GetWindow(phwnd);
    }

    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(
        IN BOOL fEnterMode)
    {
        TRACE("ITrayPriv::ContextSensitiveHelp\n");
        return E_NOTIMPL;
    }

    /*******************************************************************/

    virtual HRESULT STDMETHODCALLTYPE SetClient(IUnknown *punkClient)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetClient(IUnknown ** ppunkClient)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE OnPosRectChangeDB(RECT *prc)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE OnSelect(DWORD dwSelectType)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE SetSubMenu(IMenuPopup *pmp, BOOL fSet)
    {
        if(!fSet) {
            return m_TaskSwitchWnd->CloseOpenedTaskGroup(TRUE);
        }

        return S_OK;
    }

    /*******************************************************************/

    HRESULT Initialize(IN CTaskSwitchWnd *wnd)
    {
        m_TaskSwitchWnd = wnd;
        return S_OK;
    }

    DECLARE_NOT_AGGREGATABLE(CGroupMenuSite)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CGroupMenuSite)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IMenuPopup, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
    END_COM_MAP()
};

HRESULT CGroupMenuSite_CreateInstance(IN OUT CTaskSwitchWnd *pWnd, const IID & riid, PVOID * ppv)
{
    return ShellObjectCreatorInit<CGroupMenuSite>(pWnd, riid, ppv);
}
