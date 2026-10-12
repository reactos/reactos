/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Task window implementation
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller <w3seek@reactos.org>
 *              Copyright 2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */

#include "precomp.h"
#include "mediabtns.h"
#include <commoncontrols.h>
#include <regstr.h>
#include <shlwapi_undoc.h>
#include <psapi.h>
#include <winver.h>

/* Set DUMP_TASKS to 1 to enable a dump of the tasks and task groups every
   5 seconds */
#define DUMP_TASKS  0
#define DEBUG_SHELL_HOOK 0

#define MAX_TASKS_COUNT (0x7FFF)
#define TASK_ITEM_ARRAY_ALLOC   64

//************************************************************************
// Fullscreen windows (a.k.a. rude apps) checker

#define TIMER_ID_VALIDATE_RUDE_APP 5
#define VALIDATE_RUDE_INTERVAL 1000
#define VALIDATE_RUDE_MAX_COUNT 5

static BOOL
SHELL_GetMonitorRect(
    _In_opt_ HMONITOR hMonitor,
    _Out_opt_ PRECT prcDest,
    _In_ BOOL bWorkAreaOnly)
{
    MONITORINFO mi = { sizeof(mi) };
    if (!hMonitor || !::GetMonitorInfoW(hMonitor, &mi))
    {
        if (!prcDest)
            return FALSE;

        if (bWorkAreaOnly)
            ::SystemParametersInfoW(SPI_GETWORKAREA, 0, prcDest, 0);
        else
            ::SetRect(prcDest, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

        return FALSE;
    }

    if (prcDest)
        *prcDest = (bWorkAreaOnly ? mi.rcWork : mi.rcMonitor);
    return TRUE;
}

static BOOL
SHELL_IsParentOwnerOrSelf(_In_ HWND hwndTarget, _In_ HWND hWnd)
{
    for (; hWnd; hWnd = ::GetParent(hWnd))
    {
        if (hWnd == hwndTarget)
            return TRUE;
    }
    return FALSE;
}

static BOOL
SHELL_IsRudeWindowActive(_In_ HWND hWnd)
{
    HWND hwndFore = ::GetForegroundWindow();
    DWORD dwThreadId = ::GetWindowThreadProcessId(hWnd, NULL);
    return dwThreadId == ::GetWindowThreadProcessId(hwndFore, NULL) ||
           SHELL_IsParentOwnerOrSelf(hWnd, hwndFore);
}

static BOOL
SHELL_IsRudeWindow(_In_opt_ HMONITOR hMonitor, _In_ HWND hWnd, _In_ BOOL bDontCheckActive)
{
    if (!::IsWindowVisible(hWnd) || hWnd == ::GetDesktopWindow())
        return FALSE;

    RECT rcMonitor;
    SHELL_GetMonitorRect(hMonitor, &rcMonitor, FALSE);

    DWORD style = ::GetWindowLongPtrW(hWnd, GWL_STYLE);

    RECT rcWnd;
    enum { CHECK_STYLE = WS_THICKFRAME | WS_DLGFRAME | WS_BORDER };
    if ((style & CHECK_STYLE) == CHECK_STYLE)
    {
        ::GetClientRect(hWnd, &rcWnd); // Ignore frame
        ::MapWindowPoints(hWnd, NULL, (PPOINT)&rcWnd, sizeof(RECT) / sizeof(POINT));
    }
    else
    {
        ::GetWindowRect(hWnd, &rcWnd);
    }

    RECT rcUnion;
    ::UnionRect(&rcUnion, &rcWnd, &rcMonitor);

    return ::EqualRect(&rcUnion, &rcWnd) && (bDontCheckActive || SHELL_IsRudeWindowActive(hWnd));
}

static BOOL
SHELL_GetImageFileNameByWindow(_In_ HWND hWnd, _Out_cap_(cchPath) LPWSTR pszPath, _In_ DWORD cchPath)
{
    DWORD dwProcessId;
    HANDLE hProcess;
    BOOL bSuccess;
    if (GetWindowThreadProcessId(hWnd, &dwProcessId) == 0)
        return FALSE;
    // TODO: Switch to PROCESS_QUERY_LIMITED_INFORMATION once NT6 kernel32 support is implemented.
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessId);
    if (hProcess == NULL)
        return FALSE;
    bSuccess = GetModuleFileNameExW(hProcess, NULL, pszPath, cchPath) != 0;
    CloseHandle(hProcess);
    return bSuccess;
}

////////////////////////////////////////////////////////////////

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
    union
    {
        DWORD dwFlags;
        struct
        {

            DWORD IsCollapsed : 1;
        };
    };
} TASK_GROUP, *PTASK_GROUP;

typedef struct _TASK_ITEM
{
    HWND hWnd;
    PTASK_GROUP Group;
    INT Index;
    INT IconIndex;
    INT PinIndex;   /* >= 0: shown by that pinned app's button instead of its own */

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
        }
    }
};

/*
 * Pinned apps (Windows 7/11 style): pinned programs keep a button at the start of
 * the taskbar. While one runs, its windows are represented by that button (running
 * pill, active tile, click to switch) instead of getting buttons of their own.
 * Stored as program paths in HKCU\...\Explorer\Taskband\ReactOSPinnedApps.
 */
#define MAX_PINS        32
#define PIN_CMD_BASE    0x4000
#define PIN_REG_KEY     L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Taskband"
#define PIN_REG_VALUE   L"ReactOSPinnedApps"
#define PIN_DEFAULTS    L"%SystemRoot%\\explorer.exe\0%SystemRoot%\\system32\\settings.exe\0"

typedef struct _PINNED_APP
{
    WCHAR szPath[MAX_PATH];
    WCHAR szName[128];
    INT IconIndex;
} PINNED_APP;

/* Full path of the program that owns a window */
static BOOL GetWindowExePath(HWND hWnd, LPWSTR pszPath, DWORD cchPath)
{
    DWORD dwPid = 0;
    HANDLE hProcess;
    BOOL bOk = FALSE;

    GetWindowThreadProcessId(hWnd, &dwPid);
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPid);
    if (hProcess)
    {
        bOk = GetModuleFileNameExW(hProcess, NULL, pszPath, cchPath) > 0;
        CloseHandle(hProcess);
    }
    return bOk;
}

/* "Notepad" from the program's version info, or its file name */
static VOID GetAppDisplayName(LPCWSTR pszPath, LPWSTR pszName, DWORD cchName)
{
    DWORD dwHandle, cbInfo = GetFileVersionInfoSizeW(pszPath, &dwHandle);
    PCWSTR pszFile;

    if (cbInfo)
    {
        PVOID pInfo = HeapAlloc(GetProcessHeap(), 0, cbInfo);
        if (pInfo && GetFileVersionInfoW(pszPath, 0, cbInfo, pInfo))
        {
            WORD *pLang;
            UINT cb;
            if (VerQueryValueW(pInfo, L"\\VarFileInfo\\Translation", (PVOID *)&pLang, &cb) && cb >= 4)
            {
                WCHAR szSub[64];
                PWSTR pszDesc;
                StringCchPrintfW(szSub, _countof(szSub), L"\\StringFileInfo\\%04x%04x\\FileDescription",
                                 pLang[0], pLang[1]);
                if (VerQueryValueW(pInfo, szSub, (PVOID *)&pszDesc, &cb) && cb > 1 && *pszDesc)
                {
                    StringCchCopyW(pszName, cchName, pszDesc);
                    HeapFree(GetProcessHeap(), 0, pInfo);
                    return;
                }
            }
        }
        if (pInfo)
            HeapFree(GetProcessHeap(), 0, pInfo);
    }

    pszFile = PathFindFileNameW(pszPath);
    StringCchCopyW(pszName, cchName, pszFile);
    PathRemoveExtensionW(pszName);
}

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

class CTaskSwitchWnd :
    public CComCoClass<CTaskSwitchWnd>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl < CTaskSwitchWnd, CWindow, CControlWinTraits >,
    public IOleWindow
{
    CTaskToolbar m_TaskBar;

    CComPtr<ITrayWindow> m_Tray;

    UINT m_ShellHookMsg;
    UINT m_WinMMDevChgMsg;

    WORD m_TaskItemCount;
    WORD m_AllocatedTaskItems;

    PTASK_GROUP m_TaskGroups;
    PTASK_ITEM m_TaskItems;
    PTASK_ITEM m_ActiveTaskItem;

    HTHEME m_Theme;
    UINT m_ButtonsPerLine;
    WORD m_ButtonCount;

    HIMAGELIST m_ImageList;

    BOOL m_IsGroupingEnabled;
    BOOL m_IsDestroying;

    INT m_nRudeAppValidationCounter;

    SIZE m_ButtonSize;
    INT m_cyLastButton = 0;

    /* Pinned apps */
    CTaskToolbar m_PinBar;
    HIMAGELIST m_PinImageList = NULL;
    PINNED_APP m_Pins[MAX_PINS];
    INT m_PinCount = 0;

    UINT m_uHardErrorMsg;
    CHardErrorThread m_HardErrorThread;
    CMultimediaBackend m_Mixer;

public:
    CTaskSwitchWnd() :
        m_ShellHookMsg(NULL),
        m_WinMMDevChgMsg(NULL),
        m_TaskItemCount(0),
        m_AllocatedTaskItems(0),
        m_TaskGroups(NULL),
        m_TaskItems(NULL),
        m_ActiveTaskItem(NULL),
        m_Theme(NULL),
        m_ButtonsPerLine(0),
        m_ButtonCount(0),
        m_ImageList(NULL),
        m_IsGroupingEnabled(FALSE),
        m_IsDestroying(FALSE),
        m_nRudeAppValidationCounter(0)
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


    INT UpdateTaskGroupButton(IN PTASK_GROUP TaskGroup)
    {
        ASSERT(TaskGroup->Index >= 0);

        /* FIXME: Implement */

        return TaskGroup->Index;
    }

    VOID ExpandTaskGroup(IN PTASK_GROUP TaskGroup)
    {
        ASSERT(TaskGroup->dwTaskCount > 0);
        ASSERT(TaskGroup->IsCollapsed);
        ASSERT(TaskGroup->Index >= 0);

        /* FIXME: Implement */
    }

    HICON GetWndIcon(HWND hwnd)
    {
        HICON hIcon = NULL;

        /* Retrieve icon by sending a message */
#define GET_ICON(type) \
    SendMessageTimeout(hwnd, WM_GETICON, (type), 0, SMTO_NOTIMEOUTIFNOTHUNG, 100, (PDWORD_PTR)&hIcon)

        LRESULT bAlive = GET_ICON(UseSmallIcons() ? ICON_SMALL2 : ICON_BIG);
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
            GET_ICON(UseSmallIcons() ? ICON_BIG : ICON_SMALL2);
            if (hIcon)
                return hIcon;
        }
#undef GET_ICON

        /* If we failed, retrieve icon from the window class */
        hIcon = (HICON)GetClassLongPtr(hwnd, UseSmallIcons() ? GCLP_HICONSM : GCLP_HICON);
        if (hIcon)
            return hIcon;

        return (HICON)GetClassLongPtr(hwnd, UseSmallIcons() ? GCLP_HICON : GCLP_HICONSM);
    }

    INT UpdateTaskItemButton(IN PTASK_ITEM TaskItem)
    {
        TBBUTTONINFO tbbi = { 0 };
        HICON icon;
        WCHAR windowText[255];

        if (TaskItem->Index < 0 && TaskItem->PinIndex >= 0)
        {
            m_PinBar.InvalidateRect(NULL, TRUE);
            return -1;
        }

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

                if (TaskGroup->dwTaskCount > 1)
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

    INT AddTaskItemButton(IN OUT PTASK_ITEM TaskItem)
    {
        WCHAR windowText[255];
        TBBUTTON tbBtn = { 0 };
        INT iIndex;
        HICON icon;

        if (TaskItem->PinIndex >= 0)
        {
            /* Represented by its pinned app button */
            m_PinBar.InvalidateRect(NULL, TRUE);
            return -1;
        }

        if (TaskItem->Index >= 0)
        {
            return UpdateTaskItemButton(TaskItem);
        }

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
        tbBtn.fsStyle = BTNS_CHECK | BTNS_NOPREFIX | (IsIconOnly() ? 0 : BTNS_SHOWTEXT);
        tbBtn.dwData = TaskItem->Index;

        if (GetWndTextFromTaskItem(TaskItem, windowText, _countof(windowText)) > 0)
        {
            tbBtn.iString = (DWORD_PTR) windowText;
        }

        /* Find out where to insert the new button */
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
            }
            else if (TaskGroup->IsCollapsed &&
                TaskGroup->Index >= 0)
            {
                if (dwNewTaskCount > 1)
                {
                    /* FIXME: Check if we should expand the group */
                    /* Update the task group button */
                    UpdateTaskGroupButton(TaskGroup);
                }
                else
                {
                    /* Expand the group of one task button to a task button */
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

        /* A pinned app may have lost its last window */
        if (m_PinBar.m_hWnd)
            m_PinBar.InvalidateRect(NULL, TRUE);
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

                /* FIXME */
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

        /* The active window may belong to a pinned app */
        if (m_PinBar.m_hWnd)
            m_PinBar.InvalidateRect(NULL, TRUE);

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
                TaskItem->PinIndex = GetPinIndexForWindow(hWnd);
                TaskItem->Group = AddToTaskGroup(hWnd);

                if (!m_IsDestroying)
                {
                    AddTaskItemButton(TaskItem);
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

    /* Windows 11 style: icon-only buttons (title shown as tooltip) on a horizontal
     * taskbar. The alignment setting (left/center) only moves them, like on Windows 11. */
    BOOL IsIconOnly()
    {
        return m_Tray->IsHorizontal();
    }

    /* The toolbar recomputes its button height as icon height + vertical padding,
     * overriding TB_SETBUTTONSIZE: set the padding so that this gives S too */
    VOID SetIconOnlyButtonSize(CTaskToolbar &Toolbar, INT S)
    {
        INT cyIcon = GetTaskIconSize().cy;
        /* (TB_SETPADDING would clamp the padding to SM_CYEDGE; TB_SETMETRICS doesn't) */
        TBMETRICS tbm = { sizeof(tbm), TBMF_PAD };
        tbm.cxPad = max(0, S - cyIcon);
        tbm.cyPad = max(0, S - cyIcon);
        Toolbar.SendMessageW(TB_SETMETRICS, 0, (LPARAM)&tbm);
        Toolbar.SendMessageW(TB_SETBUTTONSIZE, 0, MAKELONG(S, S));
    }

    /* Size of a task button image: the same as the Start button's */
    INT GetButtonImageSize()
    {
        if (g_StartImageSize.cy > 0)
            return g_StartImageSize.cy;
        HDC hdc = GetDC();
        INT cy = MulDiv(48, GetDeviceCaps(hdc, LOGPIXELSY), 96);
        ReleaseDC(hdc);
        return cy;
    }

    /*
     * Windows 11 style taskbar button: the theme tile drawn at exactly the Start
     * button's size, centered on the taskbar's vertical center (like the Start
     * button), the icon centered on it, and the pill under the icon:
     * PillWidth 0 = not running, small = running, wide = active window.
     */
    VOID DrawTaskTile(HWND hwndToolbar, HDC hdc, const RECT *prc, UINT uItemState, BOOL bActive,
                      INT PillWidth, HIMAGELIST himl, INT IconIndex)
    {
        HTHEME hTheme = GetWindowTheme(hwndToolbar);
        INT S = GetButtonImageSize(), cxIcon = GetTaskIconSize().cx, iState;
        RECT rcTray, rcImage;
        POINT ptCenter;
        HWND hwndTray = GetAncestor(m_hWnd, GA_ROOT);

        /* Vertical center of the taskbar, in toolbar coordinates */
        ::GetClientRect(hwndTray, &rcTray);
        ptCenter.x = 0;
        ptCenter.y = rcTray.bottom / 2;
        ::MapWindowPoints(hwndTray, hwndToolbar, &ptCenter, 1);

        rcImage.left = (prc->left + prc->right - S) / 2;
        rcImage.top = ptCenter.y - S / 2;
        rcImage.right = rcImage.left + S;
        rcImage.bottom = rcImage.top + S;

        if (uItemState & CDIS_MARKED)
        {
            /* Flashing: the theme's flash tile */
            DrawThemeBackground(m_Theme, hdc, TDP_FLASHBUTTON, 0, &rcImage, NULL);
        }
        else if (hTheme)
        {
            if (bActive)
                iState = (uItemState & CDIS_HOT) ? TS_HOTCHECKED : TS_CHECKED;
            else if (uItemState & CDIS_SELECTED)
                iState = TS_PRESSED;
            else if (uItemState & CDIS_HOT)
                iState = TS_HOT;
            else
                iState = TS_NORMAL;
            DrawThemeBackground(hTheme, hdc, TP_BUTTON, iState, &rcImage, NULL);
        }

        /* Icon centered on the tile, a little above the pill */
        if (IconIndex >= 0)
        {
            ImageList_Draw(himl, IconIndex, hdc,
                           rcImage.left + (S - cxIcon) / 2,
                           rcImage.top + (S - cxIcon) / 2 - S / 24,
                           ILD_TRANSPARENT);
        }

        /* The pill: gray when running, accent colored and wider when active */
        if (PillWidth > 0)
        {
            INT cy = max(3, S / 16), inset = S / 12;
            RECT rcPill;
            HBRUSH hbr = CreateSolidBrush(GetSysColor(bActive ? COLOR_HOTLIGHT : COLOR_GRAYTEXT));
            HGDIOBJ hOldBrush = SelectObject(hdc, hbr), hOldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
            rcPill.left = rcImage.left + (S - PillWidth) / 2;
            rcPill.right = rcPill.left + PillWidth;
            rcPill.bottom = rcImage.bottom - inset - 1;
            rcPill.top = rcPill.bottom - cy;
            RoundRect(hdc, rcPill.left, rcPill.top, rcPill.right + 1, rcPill.bottom + 1, cy, cy);
            SelectObject(hdc, hOldBrush);
            SelectObject(hdc, hOldPen);
            DeleteObject(hbr);
        }
    }

    INT PillWidthFor(BOOL bRunning, BOOL bActive)
    {
        INT S = GetButtonImageSize();
        return !bRunning ? 0 : bActive ? S / 3 : S / 8;
    }

    /* Returns FALSE to let the toolbar draw (classic look, groups) */
    BOOL DrawIconOnlyButton(IN OUT NMTBCUSTOMDRAW *nmtbcd, PTASK_ITEM TaskItem)
    {
        BOOL bActive = (nmtbcd->nmcd.uItemState & CDIS_CHECKED) != 0;

        if (!IsIconOnly() || !GetWindowTheme(m_TaskBar.m_hWnd) || !TaskItem)
            return FALSE;

        DrawTaskTile(m_TaskBar.m_hWnd, nmtbcd->nmcd.hdc, &nmtbcd->nmcd.rc, nmtbcd->nmcd.uItemState,
                     bActive, PillWidthFor(TRUE, bActive), m_ImageList, TaskItem->IconIndex);
        return TRUE;
    }

    /* ---- Pinned apps ------------------------------------------------------ */

    INT FindPin(LPCWSTR pszPath)
    {
        for (INT i = 0; i < m_PinCount; i++)
        {
            if (!_wcsicmp(m_Pins[i].szPath, pszPath))
                return i;
        }
        return -1;
    }

    INT GetPinIndexForWindow(HWND hWnd)
    {
        WCHAR szPath[MAX_PATH];
        if (!m_PinCount || !IsIconOnly() || !GetWindowExePath(hWnd, szPath, _countof(szPath)))
            return -1;
        return FindPin(szPath);
    }

    INT PinBarWidth()
    {
        return (m_PinCount && IsIconOnly()) ? m_PinCount * GetButtonImageSize() : 0;
    }

    BOOL AddPinEntry(LPCWSTR pszPath)
    {
        if (m_PinCount >= MAX_PINS || FindPin(pszPath) >= 0 || GetFileAttributesW(pszPath) == INVALID_FILE_ATTRIBUTES)
            return FALSE;
        PINNED_APP *Pin = &m_Pins[m_PinCount++];
        StringCchCopyW(Pin->szPath, _countof(Pin->szPath), pszPath);
        GetAppDisplayName(pszPath, Pin->szName, _countof(Pin->szName));
        Pin->IconIndex = -1;
        return TRUE;
    }

    VOID LoadPins()
    {
        WCHAR szList[MAX_PINS * MAX_PATH];
        DWORD cbList = sizeof(szList) - 2 * sizeof(WCHAR), dwType = 0;

        ZeroMemory(szList, sizeof(szList));
        if (SHGetValueW(HKEY_CURRENT_USER, PIN_REG_KEY, PIN_REG_VALUE, &dwType, szList, &cbList) != ERROR_SUCCESS ||
            dwType != REG_MULTI_SZ)
        {
            /* Default pins: File Explorer and Settings */
            CopyMemory(szList, PIN_DEFAULTS, sizeof(PIN_DEFAULTS));
        }

        m_PinCount = 0;
        for (PCWSTR psz = szList; *psz; psz += wcslen(psz) + 1)
        {
            WCHAR szPath[MAX_PATH];
            if (ExpandEnvironmentStringsW(psz, szPath, _countof(szPath)))
                AddPinEntry(szPath);
        }
    }

    VOID SavePins()
    {
        WCHAR szList[MAX_PINS * MAX_PATH];
        PWSTR psz = szList;

        for (INT i = 0; i < m_PinCount; i++)
        {
            StringCchCopyW(psz, _countof(szList) - (psz - szList) - 1, m_Pins[i].szPath);
            psz += wcslen(psz) + 1;
        }
        *psz++ = UNICODE_NULL;
        SHSetValueW(HKEY_CURRENT_USER, PIN_REG_KEY, PIN_REG_VALUE, REG_MULTI_SZ,
                    szList, (DWORD)((psz - szList) * sizeof(WCHAR)));
    }

    VOID LayoutBars()
    {
        RECT rc;
        INT cxPins = PinBarWidth();

        if (!m_TaskBar.m_hWnd || !GetClientRect(&rc))
            return;
        if (m_PinBar.m_hWnd)
        {
            m_PinBar.SetWindowPos(NULL, 0, 0, cxPins, rc.bottom,
                                  SWP_NOZORDER | SWP_NOACTIVATE | (cxPins ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
        }
        m_TaskBar.SetWindowPos(NULL, cxPins, 0, rc.right - cxPins, rc.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    /* (Re)create the pinned buttons: after loading, pin/unpin, icon size changes */
    VOID RebuildPinBar()
    {
        if (!m_PinBar.m_hWnd)
            return;

        SIZE IconSize = GetTaskIconSize();
        m_PinBar.BeginUpdate();
        while (m_PinBar.GetButtonCount() > 0)
            m_PinBar.DeleteButton(0);
        ImageList_SetIconSize(m_PinImageList, IconSize.cx, IconSize.cy);   /* also empties it */
        m_PinBar.SetImageList(m_PinImageList);
        SetIconOnlyButtonSize(m_PinBar, GetButtonImageSize());

        for (INT i = 0; i < m_PinCount; i++)
        {
            HICON hIcon = NULL;
            TBBUTTON tbBtn = { 0 };

            PrivateExtractIconsW(m_Pins[i].szPath, 0, IconSize.cx, IconSize.cy, &hIcon, NULL, 1, 0);
            m_Pins[i].IconIndex = hIcon ? ImageList_ReplaceIcon(m_PinImageList, -1, hIcon) : -1;
            if (hIcon)
                DestroyIcon(hIcon);

            tbBtn.iBitmap = m_Pins[i].IconIndex;
            tbBtn.idCommand = PIN_CMD_BASE + i;
            tbBtn.fsState = TBSTATE_ENABLED;
            tbBtn.fsStyle = BTNS_BUTTON | BTNS_NOPREFIX;    /* no BTNS_SHOWTEXT: name = tooltip */
            tbBtn.dwData = i;
            tbBtn.iString = (INT_PTR)m_Pins[i].szName;
            m_PinBar.InsertButton(i, &tbBtn);
        }
        m_PinBar.EndUpdate();
        LayoutBars();
    }

    /* Recompute which pin represents each window (after pins changed) */
    VOID RematchPins()
    {
        for (INT i = 0; i < m_TaskItemCount; i++)
        {
            PTASK_ITEM TaskItem = &m_TaskItems[i];
            INT PinIndex = GetPinIndexForWindow(TaskItem->hWnd);
            if (PinIndex == TaskItem->PinIndex)
                continue;

            TaskItem->PinIndex = PinIndex;
            if (PinIndex >= 0)
                DeleteTaskItemButton(TaskItem);     /* now shown by the pin */
            else
                AddTaskItemButton(TaskItem);        /* gets its own button again */
        }
        UpdateButtonsSize(FALSE);
    }

    VOID PinApp(LPCWSTR pszPath)
    {
        if (!AddPinEntry(pszPath))
            return;
        SavePins();
        RebuildPinBar();
        RematchPins();
    }

    VOID UnpinApp(INT PinIndex)
    {
        if (PinIndex < 0 || PinIndex >= m_PinCount)
            return;
        MoveMemory(&m_Pins[PinIndex], &m_Pins[PinIndex + 1], (m_PinCount - PinIndex - 1) * sizeof(PINNED_APP));
        m_PinCount--;
        SavePins();
        /* Pin indexes changed: windows will be matched again */
        for (INT i = 0; i < m_TaskItemCount; i++)
        {
            if (m_TaskItems[i].PinIndex == PinIndex)
                m_TaskItems[i].PinIndex = -2;   /* force a re-match */
            else if (m_TaskItems[i].PinIndex > PinIndex)
                m_TaskItems[i].PinIndex--;
        }
        RebuildPinBar();
        RematchPins();
    }

    /* Windows of a pinned app, in taskbar order; returns how many */
    INT GetPinWindows(INT PinIndex, PTASK_ITEM *pFirst, PTASK_ITEM *pNextAfterActive)
    {
        INT Count = 0;
        BOOL bAfterActive = FALSE;
        *pFirst = *pNextAfterActive = NULL;
        for (INT i = 0; i < m_TaskItemCount; i++)
        {
            PTASK_ITEM TaskItem = &m_TaskItems[i];
            if (TaskItem->PinIndex != PinIndex)
                continue;
            Count++;
            if (!*pFirst)
                *pFirst = TaskItem;
            if (bAfterActive && !*pNextAfterActive)
                *pNextAfterActive = TaskItem;
            if (TaskItem == m_ActiveTaskItem)
                bAfterActive = TRUE;
        }
        return Count;
    }

    VOID HandlePinClick(INT PinIndex)
    {
        PTASK_ITEM First, Next, Target;
        INT Count;

        if (PinIndex < 0 || PinIndex >= m_PinCount)
            return;

        Count = GetPinWindows(PinIndex, &First, &Next);
        if (!Count)
        {
            /* Not running: start it */
            ShellExecuteW(NULL, NULL, m_Pins[PinIndex].szPath, NULL, NULL, SW_SHOWNORMAL);
            return;
        }

        /* Running: switch to it, cycle through its windows, or minimize the only one */
        if (m_ActiveTaskItem && m_ActiveTaskItem->PinIndex == PinIndex)
            Target = Next ? Next : (Count > 1 ? First : m_ActiveTaskItem);
        else
            Target = First;
        HandleTaskItemClick(Target);
    }

    LRESULT HandlePinNotification(IN const NMHDR *nmh)
    {
        if (nmh->code != NM_CUSTOMDRAW)
            return 0;

        LPNMTBCUSTOMDRAW nmtbcd = (LPNMTBCUSTOMDRAW)nmh;
        if (nmtbcd->nmcd.dwDrawStage == CDDS_PREPAINT)
            return CDRF_NOTIFYITEMDRAW;
        if (nmtbcd->nmcd.dwDrawStage != CDDS_ITEMPREPAINT)
            return CDRF_DODEFAULT;

        INT PinIndex = (INT)nmtbcd->nmcd.lItemlParam;
        PTASK_ITEM First, Next;
        BOOL bRunning = (PinIndex >= 0 && PinIndex < m_PinCount) && GetPinWindows(PinIndex, &First, &Next) > 0;
        BOOL bActive = bRunning && m_ActiveTaskItem && m_ActiveTaskItem->PinIndex == PinIndex;

        DrawTaskTile(m_PinBar.m_hWnd, nmtbcd->nmcd.hdc, &nmtbcd->nmcd.rc, nmtbcd->nmcd.uItemState & ~CDIS_CHECKED,
                     bActive, PillWidthFor(bRunning, bActive), m_PinImageList,
                     (PinIndex >= 0 && PinIndex < m_PinCount) ? m_Pins[PinIndex].IconIndex : -1);
        return CDRF_SKIPDEFAULT;
    }

    /*
     * Windows 11 style menu for a taskbar button: the app (starts a new one),
     * pin/unpin, and close. pTaskItem: a window without a pin; PinIndex: a pin.
     */
    VOID ShowAppMenu(PTASK_ITEM pTaskItem, INT PinIndex)
    {
        enum { CMD_LAUNCH = 1, CMD_PIN, CMD_UNPIN, CMD_CLOSE };
        WCHAR szPath[MAX_PATH], szName[128], szText[128];
        PTASK_ITEM First = NULL, Next = NULL;
        INT Count = 0, Cmd;
        POINT pt;
        HMENU hMenu;

        if (PinIndex >= 0)
        {
            StringCchCopyW(szPath, _countof(szPath), m_Pins[PinIndex].szPath);
            StringCchCopyW(szName, _countof(szName), m_Pins[PinIndex].szName);
            Count = GetPinWindows(PinIndex, &First, &Next);
        }
        else
        {
            if (!GetWindowExePath(pTaskItem->hWnd, szPath, _countof(szPath)))
                return;
            GetAppDisplayName(szPath, szName, _countof(szName));
            First = pTaskItem;
            Count = 1;
        }

        hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, CMD_LAUNCH, szName);
        SetMenuDefaultItem(hMenu, CMD_LAUNCH, FALSE);
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        LoadStringW(hExplorerInstance, PinIndex >= 0 ? IDS_UNPIN_FROM_TASKBAR : IDS_PIN_TO_TASKBAR, szText, _countof(szText));
        AppendMenuW(hMenu, MF_STRING, PinIndex >= 0 ? CMD_UNPIN : CMD_PIN, szText);
        if (Count)
        {
            LoadStringW(hExplorerInstance, Count > 1 ? IDS_CLOSE_ALL_WINDOWS : IDS_CLOSE_WINDOW, szText, _countof(szText));
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, CMD_CLOSE, szText);
        }

        /* No tooltips while the menu is open: they would cover it (the mouse is
         * still over the button, so hiding one just makes it come back) */
        HWND hwndTips[2] = { (HWND)m_TaskBar.SendMessageW(TB_GETTOOLTIPS, 0, 0),
                             (HWND)m_PinBar.SendMessageW(TB_GETTOOLTIPS, 0, 0) };
        for (INT i = 0; i < 2; i++)
        {
            if (hwndTips[i])
            {
                ::SendMessageW(hwndTips[i], TTM_ACTIVATE, FALSE, 0);
                ::SendMessageW(hwndTips[i], TTM_POP, 0, 0);
            }
        }

        GetCursorPos(&pt);
        ::SetForegroundWindow(GetAncestor(m_hWnd, GA_ROOT));
        Cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, m_hWnd, NULL);
        DestroyMenu(hMenu);

        for (INT i = 0; i < 2; i++)
        {
            if (hwndTips[i])
                ::SendMessageW(hwndTips[i], TTM_ACTIVATE, TRUE, 0);
        }

        switch (Cmd)
        {
            case CMD_LAUNCH:
                ShellExecuteW(NULL, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
                break;
            case CMD_PIN:
                PinApp(szPath);
                break;
            case CMD_UNPIN:
                UnpinApp(PinIndex);
                break;
            case CMD_CLOSE:
                for (INT i = m_TaskItemCount - 1; i >= 0; i--)
                {
                    PTASK_ITEM TaskItem = &m_TaskItems[i];
                    if (TaskItem == pTaskItem || (PinIndex >= 0 && TaskItem->PinIndex == PinIndex))
                        ::PostMessageW(TaskItem->hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
                }
                break;
        }
    }

    LRESULT OnGetContentWidth(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /* Width of the pinned buttons and the task buttons (for the centered layout) */
        INT cx = PinBarWidth(), n = m_TaskBar.GetButtonCount();
        RECT rcItem;
        if (n > 0 && m_TaskBar.GetItemRect(n - 1, &rcItem))
            cx += rcItem.right;
        return cx;
    }

    BOOL UseSmallIcons()
    {
        return g_TaskbarSettings.bSmallIcons && !IsIconOnly();
    }

    /* Icon-only buttons use 32px icons at 100% (96 DPI): the programs' native large
     * icons, the same size as the Start logo */
    SIZE GetTaskIconSize()
    {
        SIZE Size;
        if (IsIconOnly())
        {
            HDC hdc = GetDC();
            Size.cx = Size.cy = MulDiv(32, GetDeviceCaps(hdc, LOGPIXELSY), 96);
            ReleaseDC(hdc);
        }
        else
        {
            Size.cx = GetSystemMetrics(g_TaskbarSettings.bSmallIcons ? SM_CXSMICON : SM_CXICON);
            Size.cy = GetSystemMetrics(g_TaskbarSettings.bSmallIcons ? SM_CYSMICON : SM_CYICON);
        }
        return Size;
    }

    VOID UpdateButtonsSize(IN BOOL bRedrawDisabled)
    {
        RECT rcClient;
        UINT uiRows, uiMax, uiMin, uiBtnsPerLine, ui;
        LONG NewBtnSize;
        BOOL Horizontal;

        /* Update the size of the image list if needed */
        int cx, cy;
        SIZE IconSize = GetTaskIconSize();
        ImageList_GetIconSize(m_ImageList, &cx, &cy);
        if (cx != IconSize.cx || cy != IconSize.cy)
        {
            ImageList_SetIconSize(m_ImageList, IconSize.cx, IconSize.cy);

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

            /* The pinned buttons too */
            RebuildPinBar();
        }

        if (GetClientRect(&rcClient) && !IsRectEmpty(&rcClient))
        {
            rcClient.right -= PinBarWidth();
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

                    if (IsIconOnly())
                    {
                        /* Square buttons exactly the size of the Start button image; they are
                         * custom drawn (HandleItemPaint) at the same place as the Start button */
                        NewBtnSize = GetButtonImageSize();
                        SetIconOnlyButtonSize(m_TaskBar, NewBtnSize);

                        /* The band must be as tall as the buttons, or their tiles get cut off:
                         * have the taskbar re-read the band size when it changes */
                        if (NewBtnSize != m_cyLastButton)
                        {
                            m_cyLastButton = NewBtnSize;
                            ::PostMessageW(GetAncestor(m_hWnd, GA_ROOT), TWM_REALIGNCONTROLS, 1, 0);
                        }
                    }

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

                    if (m_ActiveTaskItem != NULL &&
                        m_ActiveTaskItem->Index == (INT)ui)
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

        /* The centered taskbar layout depends on the number of buttons */
        if (IsIconOnly() && g_TaskbarSettings.bCenterTaskbar)
            ::PostMessageW(GetAncestor(m_hWnd, GA_ROOT), TWM_REALIGNCONTROLS, 0, 0);
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
        /* Pinned apps toolbar. Created first: other code finds the task toolbar as
         * this window's first child (the most recently created one) */
        if (m_PinBar.Initialize(m_hWnd))
        {
            SIZE PinIconSize = GetTaskIconSize();
            SetWindowTheme(m_PinBar.m_hWnd, L"TaskBand", NULL);
            m_PinBar.SendMessageW(TB_SETEXTENDEDSTYLE, 0,
                                  m_PinBar.SendMessageW(TB_GETEXTENDEDSTYLE, 0, 0) | TBSTYLE_EX_MIXEDBUTTONS);
            m_PinImageList = ImageList_Create(PinIconSize.cx, PinIconSize.cy, ILC_COLOR32 | ILC_MASK, 0, MAX_PINS);
            LoadPins();
        }

        if (!m_TaskBar.Initialize(m_hWnd))
            return FALSE;

        SetWindowTheme(m_TaskBar.m_hWnd, L"TaskBand", NULL);

        /* Icon-only buttons are as large as the Start button from the start, so the
         * band is tall enough even before the first window appears */
        if (IsIconOnly())
            SetIconOnlyButtonSize(m_TaskBar, GetButtonImageSize());

        /* Buttons without BTNS_SHOWTEXT (icon-only mode) use their text as tooltip */
        m_TaskBar.SendMessageW(TB_SETEXTENDEDSTYLE, 0,
                               m_TaskBar.SendMessageW(TB_GETEXTENDEDSTYLE, 0, 0) | TBSTYLE_EX_MIXEDBUTTONS);

        SIZE IconSize = GetTaskIconSize();
        m_ImageList = ImageList_Create(IconSize.cx, IconSize.cy, ILC_COLOR32 | ILC_MASK, 0, 1000);
        m_TaskBar.SetImageList(m_ImageList);

        /* Set proper spacing between buttons */
        m_TaskBar.UpdateTbButtonSpacing(m_Tray->IsHorizontal(), m_Theme != NULL);

        /* Register the shell hook */
        m_ShellHookMsg = RegisterWindowMessageW(L"SHELLHOOK");

        TRACE("ShellHookMsg got assigned number %d\n", m_ShellHookMsg);

        RegisterShellHook(m_hWnd, 3); /* 1 if no NT! We're targeting NT so we don't care! */

        RebuildPinBar();
        RefreshWindowList();

        /* Recalculate the button size */
        UpdateButtonsSize(FALSE);

#if DUMP_TASKS != 0
        SetTimer(hwnd, 1, 5000, NULL);
#endif

        /* WinMM is needed to change system volume via media buttons */
        m_WinMMDevChgMsg = RegisterWindowMessageW(L"winmm_devicechange");
        m_Mixer.Initialize();

        return TRUE;
    }

    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        m_IsDestroying = TRUE;

        KillTimer(TIMER_ID_VALIDATE_RUDE_APP);

        /* Unregister the shell hook */
        RegisterShellHook(m_hWnd, FALSE);

        CloseThemeData(m_Theme);
        DeleteAllTasks();

        if (m_PinImageList)
        {
            ImageList_Destroy(m_PinImageList);
            m_PinImageList = NULL;
        }

        if (m_ImageList)
        {
            ImageList_Destroy(m_ImageList);
            m_ImageList = NULL;
        }

        return TRUE;
    }

    static BOOL InvokeRegistryAppKeyCommand(UINT uAppCmd)
    {
        BOOL bResult = FALSE;
        WCHAR szBuf[MAX_PATH * 2];
        wsprintfW(szBuf, L"%s\\AppKey\\%u", REGSTR_PATH_EXPLORER, uAppCmd);
        HUSKEY hKey;
        if (SHRegOpenUSKeyW(szBuf, KEY_READ, NULL, &hKey, FALSE) != ERROR_SUCCESS)
            return bResult;

        DWORD cb = sizeof(szBuf);
        if (!bResult && SHRegQueryUSValueW(hKey, L"ShellExecute", NULL, szBuf, &cb, FALSE, NULL, 0) == ERROR_SUCCESS)
        {
            bResult = TRUE;
        }
        cb = sizeof(szBuf);
        if (!bResult && SHRegQueryUSValueW(hKey, L"Association", NULL, szBuf, &cb, FALSE, NULL, 0) == ERROR_SUCCESS)
        {
            bResult = TRUE;
            cb = _countof(szBuf);
            if (AssocQueryString(ASSOCF_NOTRUNCATE, ASSOCSTR_EXECUTABLE, szBuf, NULL, szBuf, &cb) != S_OK)
                *szBuf = UNICODE_NULL;
        }
        cb = sizeof(szBuf);
        if (!bResult && SHRegQueryUSValueW(hKey, L"RegisteredApp", NULL, szBuf, &cb, FALSE, NULL, 0) == ERROR_SUCCESS)
        {
            bResult = TRUE;
            SHRunIndirectRegClientCommand(NULL, szBuf);
            *szBuf = UNICODE_NULL; // Don't execute again
        }
        SHRegCloseUSKey(hKey);

        // Note: Tweak UI uses an empty string for its "Do nothing" option.
        if (bResult && *szBuf)
            ShellExec_RunDLLW(NULL, NULL, szBuf, SW_SHOW);
        return bResult;
    }

    BOOL HandleAppCommand(IN WPARAM wParam, IN LPARAM lParam)
    {
        const UINT uAppCmd = GET_APPCOMMAND_LPARAM(lParam);
        if (InvokeRegistryAppKeyCommand(uAppCmd))
            return TRUE;
        switch (uAppCmd)
        {
            // TODO: When MMDevAPI arrives, try IMMDeviceEnumerator::GetDefaultAudioEndpoint first and then fall back to WinMM.
            case APPCOMMAND_VOLUME_MUTE:
                return m_Mixer.Mute();
            case APPCOMMAND_VOLUME_DOWN:
            case APPCOMMAND_VOLUME_UP:
                return m_Mixer.AdjustVolume(uAppCmd, hProcessHeap);
            case APPCOMMAND_BROWSER_SEARCH:
                return SHFindFiles(NULL, NULL);
        }
        return FALSE;
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
            Ret = HandleAppCommand(0, lParam);
            break;

        case HSHELL_WINDOWCREATED:
            AddTask((HWND) lParam);
            break;

        case HSHELL_WINDOWDESTROYED:
            /* The window still exists! Delay destroying it a bit */
            OnWindowDestroyed((HWND)lParam);
            DeleteTask((HWND)lParam);
            break;

        case HSHELL_RUDEAPPACTIVATED:
        case HSHELL_WINDOWACTIVATED:
            OnWindowActivated((HWND)lParam);
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

    LRESULT OnWinMMDeviceChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        TRACE("Audio device was added or removed, invalidating mixer\n");
        m_Mixer.Initialize();
        return TRUE;
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
        /* TODO: Show task group menu */
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
            HandleTaskItemClick(TaskItem);
            return TRUE;
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

        /* Windows 11 style menu (pin, close); Shift+right-click gives the window menu */
        if (IsIconOnly() && !(GetKeyState(VK_SHIFT) & 0x8000))
        {
            ShowAppMenu(TaskItem, -1);
            return;
        }

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
                if (DrawIconOnlyButton(nmtbcd, TaskItem))
                    return CDRF_SKIPDEFAULT;

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
            /* FIXME: Implement painting for task groups */
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
        }

        return Ret;
    }

    // Internal structure for IsRudeEnumProc
    typedef struct tagRUDEAPPDATA
    {
        HMONITOR hTargetMonitor;
        HWND hwndFound;
        HWND hwndFirstCheck;
    } RUDEAPPDATA, *PRUDEAPPDATA;

    // Find any rude app
    static BOOL CALLBACK
    IsRudeEnumProc(_In_ HWND hwnd, _In_ LPARAM lParam)
    {
        PRUDEAPPDATA pData = (PRUDEAPPDATA)lParam;

        HMONITOR hMon = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        if (!hMon ||
            (pData->hTargetMonitor && pData->hTargetMonitor != hMon) ||
            !SHELL_IsRudeWindow(hMon, hwnd, (hwnd == pData->hwndFirstCheck)))
        {
            return TRUE; // Continue
        }

        pData->hwndFound = hwnd;
        return FALSE; // Finish
    }

    // Internal structure for FullScreenEnumProc
    typedef struct tagFULLSCREENDATA
    {
        const RECT *pRect;
        HMONITOR hTargetMonitor;
        ITrayWindow *pTray;
    } FULLSCREENDATA, *PFULLSCREENDATA;

    // Notify ABN_FULLSCREENAPP for each monitor
    static BOOL CALLBACK
    FullScreenEnumProc(_In_ HMONITOR hMonitor, _In_opt_ HDC hDC, _In_ LPRECT prc, _In_ LPARAM lParam)
    {
        PFULLSCREENDATA pData = (PFULLSCREENDATA)lParam;

        BOOL bFullOpening = (pData->hTargetMonitor == hMonitor);
        if (!bFullOpening && pData->pRect)
        {
            RECT rc, rcMon;
            SHELL_GetMonitorRect(hMonitor, &rcMon, FALSE);
            ::IntersectRect(&rc, &rcMon, pData->pRect);
            bFullOpening = ::EqualRect(&rc, &rcMon);
        }

        // Notify ABN_FULLSCREENAPP to appbars
        pData->pTray->NotifyFullScreenToAppBars(hMonitor, bFullOpening);
        return TRUE;
    }

    void HandleFullScreenApp(_In_opt_ HWND hwndRude)
    {
        // Notify ABN_FULLSCREENAPP for every monitor
        RECT rc;
        FULLSCREENDATA Data = { NULL, NULL, NULL };
        if (hwndRude && ::GetWindowRect(hwndRude, &rc))
        {
            Data.pRect = &rc;
            Data.hTargetMonitor = ::MonitorFromWindow(hwndRude, MONITOR_DEFAULTTONULL);
        }
        Data.pTray = m_Tray;
        ::EnumDisplayMonitors(NULL, NULL, FullScreenEnumProc, (LPARAM)&Data);

        if (hwndRude)
        {
            // Make the taskbar bottom
            UINT uFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER;
            HWND hwndTray = m_Tray->GetHWND();
            ::SetWindowPos(hwndTray, HWND_BOTTOM, 0, 0, 0, 0, uFlags);

            // Switch to the rude app if necessary
            DWORD exstyle = (DWORD)::GetWindowLongPtrW(hwndRude, GWL_EXSTYLE);
            if (!(exstyle & WS_EX_TOPMOST) && !SHELL_IsRudeWindowActive(hwndRude))
                ::SwitchToThisWindow(hwndRude, TRUE);
        }
    }

    HWND FindRudeApp(_In_opt_ HWND hwndFirstCheck)
    {
        // Quick check
        HMONITOR hMon = MonitorFromWindow(hwndFirstCheck, MONITOR_DEFAULTTONEAREST);
        RUDEAPPDATA data = { hMon, NULL, hwndFirstCheck };
        if (::IsWindow(hwndFirstCheck) && !IsRudeEnumProc(hwndFirstCheck, (LPARAM)&data))
            return hwndFirstCheck;

        // Slow check
        ::EnumWindows(IsRudeEnumProc, (LPARAM)&data);

        return data.hwndFound;
    }

    // WM_WINDOWPOSCHANGED
    LRESULT OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        // Re-start rude app validation
        KillTimer(TIMER_ID_VALIDATE_RUDE_APP);
        SetTimer(TIMER_ID_VALIDATE_RUDE_APP, VALIDATE_RUDE_INTERVAL, NULL);
        m_nRudeAppValidationCounter = 0;
        bHandled = FALSE;
        return 0;
    }

    // HSHELL_WINDOWACTIVATED, HSHELL_RUDEAPPACTIVATED
    void OnWindowActivated(_In_ HWND hwndTarget)
    {
        // Re-start rude app validation
        KillTimer(TIMER_ID_VALIDATE_RUDE_APP);
        SetTimer(TIMER_ID_VALIDATE_RUDE_APP, VALIDATE_RUDE_INTERVAL, NULL);
        m_nRudeAppValidationCounter = 0;
    }

    // HSHELL_WINDOWDESTROYED
    void OnWindowDestroyed(_In_ HWND hwndTarget)
    {
        if (!FindTaskItem(hwndTarget))
            return;
        HWND hwndRude = FindRudeApp(hwndTarget);
        HandleFullScreenApp(hwndRude);
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
        if (m_TaskBar.m_hWnd != NULL)
        {
            LayoutBars();

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
            HandleButtonClick(LOWORD(wParam));
        }
        else if (lParam != 0 && (HWND) lParam == m_PinBar.m_hWnd)
        {
            HandlePinClick(LOWORD(wParam) - PIN_CMD_BASE);
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
        else if (nmh->hwndFrom == m_PinBar.m_hWnd)
        {
            Ret = HandlePinNotification(nmh);
        }
        return Ret;
    }

    LRESULT OnUpdateTaskbarPos(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /* Update the button spacing */
        m_TaskBar.UpdateTbButtonSpacing(m_Tray->IsHorizontal(), m_Theme != NULL);
        return TRUE;
    }

    LRESULT OnActivateTaskIndex(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        DWORD dwAccelerators = HIWORD(wParam);
        WCHAR szExecutablePath[MAX_PATH];

        PTASK_ITEM pTaskItem = FindTaskItemByIndex(LOWORD(wParam));
        if (pTaskItem && ::IsWindow(pTaskItem->hWnd))
        {
            // Win+Shift: New instance
            if ((dwAccelerators & MOD_SHIFT) == MOD_SHIFT)
            {
                if (!SHELL_GetImageFileNameByWindow(pTaskItem->hWnd,
                                                    szExecutablePath,
                                                    _countof(szExecutablePath)))
                {
                    return FALSE;
                }
                // Win+Shift+Ctrl is used for creating a new instance of a task as Administrator
                LPCWSTR szVerb = L"open";
                if ((dwAccelerators & MOD_CONTROL) == MOD_CONTROL)
                    szVerb = L"runas";
                ShellExecuteW(NULL, szVerb, szExecutablePath, NULL, NULL, SW_SHOWNORMAL);
                return TRUE;
            }
            // Win+Alt: Jump list
            else if ((dwAccelerators & MOD_ALT) == MOD_ALT)
            {
                // TODO: Use JumpList once implemented

                // Find TaskItem's position on the screen
                RECT rcItem, rcToolbar;
                m_TaskBar.GetItemRect(pTaskItem->Index, &rcItem);
                m_TaskBar.GetWindowRect(&rcToolbar);
                OffsetRect(&rcItem, rcToolbar.left, rcToolbar.top);

                ::SendMessageCallbackW(pTaskItem->hWnd, WM_POPUPSYSTEMMENU, 0, MAKELPARAM(rcItem.left, rcItem.top),
                               SendAsyncProc, (ULONG_PTR)pTaskItem);
            }
            // Win+Ctrl: Switch to last window of a task
            else if ((dwAccelerators & MOD_CONTROL) == MOD_CONTROL)
            {
                // TODO: Implement once Superbar is implemented.
                return FALSE;
            }
            // No modifier: just switch to the task
            else
            {
                // TODO: This should launch a pinned task once Superbar is implemented

                BOOL bIsMinimized = ::IsIconic(pTaskItem->hWnd);
                BOOL bIsActive = (pTaskItem == m_ActiveTaskItem);

                if (!bIsMinimized && bIsActive)
                {
                    if (!::IsHungAppWindow(pTaskItem->hWnd))
                        ::ShowWindowAsync(pTaskItem->hWnd, SW_MINIMIZE);
                }
                else
                {
                    ::SwitchToThisWindow(pTaskItem->hWnd, TRUE);
                }
            }
        }
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

        if (newSettings->bCenterTaskbar != g_TaskbarSettings.bCenterTaskbar)
        {
            bSettingsChanged = TRUE;
            g_TaskbarSettings.bCenterTaskbar = newSettings->bCenterTaskbar;
        }

        if (bSettingsChanged)
        {
            /* Refresh each task item view */
            RefreshWindowList();
            UpdateButtonsSize(FALSE);
        }

        return 0;
    }

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = 0;
        INT_PTR iBtn = -1;

        if (m_PinBar.m_hWnd != NULL && m_PinBar.IsWindowVisible())
        {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ::ScreenToClient(m_PinBar.m_hWnd, &pt);
            INT_PTR iPin = m_PinBar.HitTest(&pt);
            if (iPin >= 0 && iPin < m_PinCount)
            {
                ShowAppMenu(NULL, (INT)iPin);
                return 0;
            }
        }

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
        switch (wParam)
        {
#if DUMP_TASKS != 0
            case 1:
                DumpTasks();
                break;
#endif
            case TIMER_ID_VALIDATE_RUDE_APP:
            {
                // Real activation of rude app might take some time after HSHELL_...ACTIVATED.
                // Wait up to 5 seconds with validating the rude app at each second.
                HWND hwndRude = FindRudeApp(NULL);
                HandleFullScreenApp(hwndRude);

                KillTimer(wParam);
                ++m_nRudeAppValidationCounter;
                if (m_nRudeAppValidationCounter < VALIDATE_RUDE_MAX_COUNT && !hwndRude)
                    SetTimer(wParam, VALIDATE_RUDE_INTERVAL, NULL);
                break;
            }
            default:
            {
                WARN("Unknown timer ID: %p\n", wParam);
                break;
            }
        }
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

    // *** IOleWindow methods ***

    STDMETHODIMP
    GetWindow(HWND* phwnd) override
    {
        if (!phwnd)
            return E_INVALIDARG;
        *phwnd = m_hWnd;
        return S_OK;
    }

    STDMETHODIMP
    ContextSensitiveHelp(BOOL fEnterMode) override
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
        MESSAGE_HANDLER(TSWM_ACTIVATETASKINDEX, OnActivateTaskIndex)
        MESSAGE_HANDLER(TSWM_GETCONTENTWIDTH, OnGetContentWidth)
        MESSAGE_HANDLER(TWM_SETTINGSCHANGED, OnTaskbarSettingsChanged)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_SETFONT, OnSetFont)
        MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChanged)
        MESSAGE_HANDLER(m_ShellHookMsg, OnShellHook)
        MESSAGE_HANDLER(m_WinMMDevChgMsg, OnWinMMDeviceChange)
        MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate)
        MESSAGE_HANDLER(WM_KLUDGEMINRECT, OnKludgeItemRect)
        MESSAGE_HANDLER(WM_COPYDATA, OnCopyData)
        MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
    END_MSG_MAP()

    DECLARE_NOT_AGGREGATABLE(CTaskSwitchWnd)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CTaskSwitchWnd)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
    END_COM_MAP()
};

HRESULT CTaskSwitchWnd_CreateInstance(IN HWND hWndParent, IN OUT ITrayWindow *Tray, REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CTaskSwitchWnd>(hWndParent, Tray, riid, ppv);
}
