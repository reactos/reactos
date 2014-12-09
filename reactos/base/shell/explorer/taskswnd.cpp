/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
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
#include <commoncontrols.h>

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

/* Set DUMP_TASKS to 1 to enable a dump of the tasks and task groups every
   5 seconds */
#define DUMP_TASKS  0
#define DEBUG_SHELL_HOOK 0

const WCHAR szTaskSwitchWndClass [] = TEXT("MSTaskSwWClass");
const WCHAR szRunningApps [] = TEXT("Running Applications");

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

#define TASK_ITEM_ARRAY_ALLOC   64

class CTaskToolbar :
    public CToolbar<TASK_ITEM>
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

public:
    BEGIN_MSG_MAP(CNotifyToolbar)
    END_MSG_MAP()

    BOOL Initialize(HWND hWndParent)
    {
        DWORD styles = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN |
            TBSTYLE_TOOLTIPS | TBSTYLE_WRAPABLE | TBSTYLE_LIST | TBSTYLE_TRANSPARENT |
            CCS_TOP | CCS_NORESIZE | CCS_NODIVIDER;

        return SubclassWindow(Create(hWndParent, styles));
    }
};

class CTaskSwitchWnd :
    public CWindowImpl < CTaskSwitchWnd, CWindow, CControlWinTraits >
{
    CTaskToolbar TaskBar;

    HWND hWndNotify;

    UINT ShellHookMsg;
    CComPtr<ITrayWindow> Tray;

    PTASK_GROUP TaskGroups;

    WORD TaskItemCount;
    WORD AllocatedTaskItems;
    PTASK_ITEM TaskItems;
    PTASK_ITEM ActiveTaskItem;

    HTHEME TaskBandTheme;
    UINT TbButtonsPerLine;
    WORD ToolbarBtnCount;

    HIMAGELIST TaskIcons;

    BOOL IsGroupingEnabled;
    BOOL IsDestroying;

    SIZE ButtonSize;
    WCHAR szBuf[255];

public:
    CTaskSwitchWnd() :
        hWndNotify(NULL),
        ShellHookMsg(NULL),
        TaskGroups(NULL),
        TaskItemCount(0),
        AllocatedTaskItems(0),
        TaskItems(0),
        ActiveTaskItem(0),
        TaskBandTheme(NULL),
        TbButtonsPerLine(0),
        ToolbarBtnCount(0),
        TaskIcons(NULL),
        IsGroupingEnabled(FALSE),
        IsDestroying(FALSE)
    {
        ZeroMemory(&ButtonSize, sizeof(ButtonSize));
        szBuf[0] = 0;
    }
    virtual ~CTaskSwitchWnd() { }

#define MAX_TASKS_COUNT (0x7FFF)

    VOID TaskSwitchWnd_UpdateButtonsSize(IN BOOL bRedrawDisabled);

    LPTSTR GetWndTextFromTaskItem(IN PTASK_ITEM TaskItem)
    {
        /* Get the window text without sending a message so we don't hang if an
           application isn't responding! */
        if (InternalGetWindowText(TaskItem->hWnd,
            szBuf,
            sizeof(szBuf) / sizeof(szBuf[0])) > 0)
        {
            return szBuf;
        }

        return NULL;
    }


#if DUMP_TASKS != 0
    VOID DumpTasks()
    {
        PTASK_GROUP CurrentGroup;
        PTASK_ITEM CurrentTaskItem, LastTaskItem;

        TRACE("Tasks dump:\n");
        if (IsGroupingEnabled)
        {
            CurrentGroup = TaskGroups;
            while (CurrentGroup != NULL)
            {
                TRACE("- Group PID: 0x%p Tasks: %d Index: %d\n", CurrentGroup->dwProcessId, CurrentGroup->dwTaskCount, CurrentGroup->Index);

                CurrentTaskItem = TaskItems;
                LastTaskItem = CurrentTaskItem + TaskItemCount;
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

            CurrentTaskItem = TaskItems;
            LastTaskItem = CurrentTaskItem + TaskItemCount;
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
            CurrentTaskItem = TaskItems;
            LastTaskItem = CurrentTaskItem + TaskItemCount;
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

        if (IsGroupingEnabled)
        {
            /* Update all affected groups */
            CurrentGroup = TaskGroups;
            while (CurrentGroup != NULL)
            {
                if (CurrentGroup->IsCollapsed &&
                    CurrentGroup->Index >= iIndex)
                {
                    /* Update the toolbar buttons */
                    NewIndex = CurrentGroup->Index + offset;
                    if (TaskBar.SetButtonCommandId(CurrentGroup->Index + offset, NewIndex))
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
        CurrentTaskItem = TaskItems;
        LastTaskItem = CurrentTaskItem + TaskItemCount;
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
                if (TaskBar.SetButtonCommandId(CurrentTaskItem->Index + offset, NewIndex))
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
        HICON hIcon = 0;

        SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG, 1000, (PDWORD_PTR) &hIcon);
        if (hIcon)
            return hIcon;

        SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 1000, (PDWORD_PTR) &hIcon);
        if (hIcon)
            return hIcon;

        SendMessageTimeout(hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 1000, (PDWORD_PTR) &hIcon);
        if (hIcon)
            return hIcon;
        
        hIcon = (HICON) GetClassLongPtr(hwnd, GCL_HICONSM);
        if (hIcon)
            return hIcon;
        
        hIcon = (HICON) GetClassLongPtr(hwnd, GCL_HICON);

        return hIcon;
    }

    INT UpdateTaskItemButton(IN PTASK_ITEM TaskItem)
    {
        TBBUTTONINFO tbbi;
        HICON icon;

        ASSERT(TaskItem->Index >= 0);

        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_BYINDEX | TBIF_STATE | TBIF_TEXT | TBIF_IMAGE;
        tbbi.fsState = TBSTATE_ENABLED;
        if (ActiveTaskItem == TaskItem)
            tbbi.fsState |= TBSTATE_CHECKED;

        if (TaskItem->RenderFlashed)
            tbbi.fsState |= TBSTATE_MARKED;

        /* Check if we're updating a button that is the last one in the
           line. If so, we need to set the TBSTATE_WRAP flag! */
        if (!Tray->IsHorizontal() || (TbButtonsPerLine != 0 &&
            (TaskItem->Index + 1) % TbButtonsPerLine == 0))
        {
            tbbi.fsState |= TBSTATE_WRAP;
        }

        tbbi.pszText = GetWndTextFromTaskItem(TaskItem);

        icon = GetWndIcon(TaskItem->hWnd);
        TaskItem->IconIndex = ImageList_ReplaceIcon(TaskIcons, TaskItem->IconIndex, icon);
        tbbi.iImage = TaskItem->IconIndex;

        if (!TaskBar.SetButtonInfo(TaskItem->Index, &tbbi))
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

        currentTaskItem = TaskItems;
        LastItem = currentTaskItem + TaskItemCount;
        while (currentTaskItem != LastItem)
        {
            if (currentTaskItem->IconIndex > TaskItem->IconIndex)
            {
                currentTaskItem->IconIndex--;
                tbbi.iImage = currentTaskItem->IconIndex;

                TaskBar.SetButtonInfo(currentTaskItem->Index, &tbbi);
            }
            currentTaskItem++;
        }

        ImageList_Remove(TaskIcons, TaskItem->IconIndex);
    }

    PTASK_ITEM FindLastTaskItemOfGroup(
        IN PTASK_GROUP TaskGroup  OPTIONAL,
        IN PTASK_ITEM NewTaskItem  OPTIONAL)
    {
        PTASK_ITEM TaskItem, LastTaskItem, FoundTaskItem = NULL;
        DWORD dwTaskCount;

        ASSERT(IsGroupingEnabled);

        TaskItem = TaskItems;
        LastTaskItem = TaskItem + TaskItemCount;

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
        if (IsGroupingEnabled)
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

        return ToolbarBtnCount;
    }

    INT AddTaskItemButton(IN OUT PTASK_ITEM TaskItem)
    {
        TBBUTTON tbBtn;
        INT iIndex;
        HICON icon;

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
        TaskItem->IconIndex = ImageList_ReplaceIcon(TaskIcons, -1, icon);

        tbBtn.iBitmap = TaskItem->IconIndex;
        tbBtn.fsState = TBSTATE_ENABLED | TBSTATE_ELLIPSES;
        tbBtn.fsStyle = BTNS_CHECK | BTNS_NOPREFIX | BTNS_SHOWTEXT;
        tbBtn.dwData = TaskItem->Index;

        tbBtn.iString = (DWORD_PTR) GetWndTextFromTaskItem(TaskItem);

        /* Find out where to insert the new button */
        iIndex = CalculateTaskItemNewButtonIndex(TaskItem);
        ASSERT(iIndex >= 0);
        tbBtn.idCommand = iIndex;

        TaskBar.BeginUpdate();

        if (TaskBar.InsertButton(iIndex, &tbBtn))
        {
            UpdateIndexesAfter(iIndex, TRUE);

            TRACE("Added button %d for hwnd 0x%p\n", iIndex, TaskItem->hWnd);

            TaskItem->Index = iIndex;
            ToolbarBtnCount++;

            /* Update button sizes and fix the button wrapping */
            UpdateButtonsSize(TRUE);
            return iIndex;
        }

        TaskBar.EndUpdate();

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
                TaskBar.BeginUpdate();

                RemoveIcon(TaskItem);
                iIndex = TaskItem->Index;
                if (TaskBar.DeleteButton(iIndex))
                {
                    TaskItem->Index = -1;
                    ToolbarBtnCount--;

                    UpdateIndexesAfter(iIndex, FALSE);

                    /* Update button sizes and fix the button wrapping */
                    UpdateButtonsSize(TRUE);
                    return TRUE;
                }

                TaskBar.EndUpdate();
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
        TaskGroup = TaskGroups;
        PrevLink = &TaskGroups;
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
                CurrentGroup = TaskGroups;
                PrevLink = &TaskGroups;
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

        TaskItem = TaskItems;
        LastItem = TaskItem + TaskItemCount;
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
        TaskItem = TaskItems;
        LastItem = TaskItem + TaskItemCount;
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
        if (TaskItemCount >= MAX_TASKS_COUNT)
        {
            /* We need the most significant bit in 16 bit command IDs to indicate whether it
               is a task group or task item. WM_COMMAND limits command IDs to 16 bits! */
            return NULL;
        }

        ASSERT(AllocatedTaskItems >= TaskItemCount);

        if (TaskItemCount == 0)
        {
            TaskItems = (PTASK_ITEM) HeapAlloc(hProcessHeap,
                0,
                TASK_ITEM_ARRAY_ALLOC * sizeof(*TaskItems));
            if (TaskItems != NULL)
            {
                AllocatedTaskItems = TASK_ITEM_ARRAY_ALLOC;
            }
            else
                return NULL;
        }
        else if (TaskItemCount >= AllocatedTaskItems)
        {
            PTASK_ITEM NewArray;
            SIZE_T NewArrayLength, ActiveTaskItemIndex;

            NewArrayLength = AllocatedTaskItems + TASK_ITEM_ARRAY_ALLOC;

            NewArray = (PTASK_ITEM) HeapReAlloc(hProcessHeap,
                0,
                TaskItems,
                NewArrayLength * sizeof(*TaskItems));
            if (NewArray != NULL)
            {
                if (ActiveTaskItem != NULL)
                {
                    /* Fixup the ActiveTaskItem pointer */
                    ActiveTaskItemIndex = ActiveTaskItem - TaskItems;
                    ActiveTaskItem = NewArray + ActiveTaskItemIndex;
                }
                AllocatedTaskItems = (WORD) NewArrayLength;
                TaskItems = NewArray;
            }
            else
                return NULL;
        }

        return TaskItems + TaskItemCount++;
    }

    VOID FreeTaskItem(IN OUT PTASK_ITEM TaskItem)
    {
        WORD wIndex;

        if (TaskItem == ActiveTaskItem)
            ActiveTaskItem = NULL;

        wIndex = (WORD) (TaskItem - TaskItems);
        if (wIndex + 1 < TaskItemCount)
        {
            MoveMemory(TaskItem,
                TaskItem + 1,
                (TaskItemCount - wIndex - 1) * sizeof(*TaskItem));
        }

        TaskItemCount--;
    }

    VOID DeleteTaskItem(IN OUT PTASK_ITEM TaskItem)
    {
        if (!IsDestroying)
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

        CurrentTaskItem = ActiveTaskItem;

        if (TaskItem != NULL)
            TaskGroup = TaskItem->Group;

        if (IsGroupingEnabled &&
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

            if (IsGroupingEnabled &&
                CurrentTaskGroup != NULL &&
                CurrentTaskGroup->IsCollapsed)
            {
                if (CurrentTaskGroup == TaskGroup)
                    return;

                /* FIXME */
            }
            else
            {
                ActiveTaskItem = NULL;
                if (CurrentTaskItem->Index >= 0)
                {
                    UpdateTaskItemButton(CurrentTaskItem);
                }
            }
        }

        ActiveTaskItem = TaskItem;

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

        TaskItem = TaskItems;
        LastItem = TaskItem + TaskItemCount;
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

        CurrentGroup = TaskGroups;
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

        if (!::IsWindow(hWnd) || Tray->IsSpecialHWND(hWnd))
            return FALSE;

        TaskItem = FindTaskItem(hWnd);
        if (TaskItem == NULL)
        {
            TRACE("Add window 0x%p\n", hWnd);
            TaskItem = AllocTaskItem();
            if (TaskItem != NULL)
            {
                ZeroMemory(TaskItem,
                    sizeof(*TaskItem));
                TaskItem->hWnd = hWnd;
                TaskItem->Index = -1;
                TaskItem->Group = AddToTaskGroup(hWnd);

                if (!IsDestroying)
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

        if (TaskItemCount > 0)
        {
            CurrentTask = TaskItems + TaskItemCount;
            do
            {
                DeleteTaskItem(--CurrentTask);
            } while (CurrentTask != TaskItems);
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
        if (IsGroupingEnabled && TaskGroup != NULL)
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

    VOID UpdateButtonsSize(IN BOOL bRedrawDisabled)
    {
        RECT rcClient;
        UINT uiRows, uiMax, uiMin, uiBtnsPerLine, ui;
        LONG NewBtnSize;
        BOOL Horizontal;

        if (GetClientRect(&rcClient) && !IsRectEmpty(&rcClient))
        {
            if (ToolbarBtnCount > 0)
            {
                Horizontal = Tray->IsHorizontal();

                if (Horizontal)
                {
                    DbgPrint("HORIZONTAL!\n");
                    TBMETRICS tbm = { 0 };
                    tbm.cbSize = sizeof(tbm);
                    tbm.dwMask = TBMF_BUTTONSPACING;
                    TaskBar.GetMetrics(&tbm);

                    uiRows = (rcClient.bottom + tbm.cyButtonSpacing) / (ButtonSize.cy + tbm.cyButtonSpacing);
                    if (uiRows == 0)
                        uiRows = 1;

                    uiBtnsPerLine = (ToolbarBtnCount + uiRows - 1) / uiRows;
                }
                else
                {
                    DbgPrint("VERTICAL!\n");
                    uiBtnsPerLine = 1;
                    uiRows = ToolbarBtnCount;
                }

                if (!bRedrawDisabled)
                    TaskBar.BeginUpdate();

                /* We might need to update the button spacing */
                int cxButtonSpacing = TaskBar.UpdateTbButtonSpacing(
                    Horizontal, TaskBandTheme != NULL,
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

                ButtonSize.cx = NewBtnSize;

                TbButtonsPerLine = uiBtnsPerLine;

                for (ui = 0; ui != ToolbarBtnCount; ui++)
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

                    if (ActiveTaskItem != NULL &&
                        ActiveTaskItem->Index == (INT)ui)
                    {
                        tbbi.fsState |= TBSTATE_CHECKED;
                    }

                    TaskBar.SetButtonInfo(ui, &tbbi);
                }
            }
            else
            {
                TbButtonsPerLine = 0;
                ButtonSize.cx = 0;
            }
        }

        // FIXME: This seems to be enabling redraws prematurely, but moving it to its right place doesn't work!
        TaskBar.EndUpdate();
    }

    BOOL CALLBACK EnumWindowsProc(IN HWND hWnd)
    {
        /* Only show windows that still exist and are visible and none of explorer's
        special windows (such as the desktop or the tray window) */
        if (::IsWindow(hWnd) && ::IsWindowVisible(hWnd) &&
            !Tray->IsSpecialHWND(hWnd))
        {
            DWORD exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
            /* Don't list popup windows and also no tool windows */
            if ((GetWindow(hWnd, GW_OWNER) == NULL || exStyle & WS_EX_APPWINDOW) &&
                !(exStyle & WS_EX_TOOLWINDOW))
            {
                TRACE("Adding task for %p...\n", hWnd);
                AddTask(hWnd);
            }

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

    LRESULT OnThemeChanged()
    {
        TRACE("OmThemeChanged\n");

        if (TaskBandTheme)
            CloseThemeData(TaskBandTheme);

        if (IsThemeActive())
            TaskBandTheme = OpenThemeData(m_hWnd, L"TaskBand");
        else
            TaskBandTheme = NULL;

        return TRUE;
    }

    LRESULT OnThemeChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {        
        return OnThemeChanged();
    }

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (!TaskBar.Initialize(m_hWnd))
            return FALSE;

        SetWindowTheme(TaskBar.m_hWnd, L"TaskBand", NULL);
        OnThemeChanged();

        TaskIcons = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 1000);
        TaskBar.SetImageList(TaskIcons);

        /* Calculate the default button size. Don't save this in ButtonSize.cx so that
        the actual button width gets updated correctly on the first recalculation */
        int cx = GetSystemMetrics(SM_CXMINIMIZED);
        int cy = ButtonSize.cy = GetSystemMetrics(SM_CYSIZE) + (2 * GetSystemMetrics(SM_CYEDGE));
        TaskBar.SetButtonSize(cx, cy);

        /* Set proper spacing between buttons */
        TaskBar.UpdateTbButtonSpacing(Tray->IsHorizontal(), TaskBandTheme != NULL);

        /* Register the shell hook */
        ShellHookMsg = RegisterWindowMessage(TEXT("SHELLHOOK"));

        TRACE("ShellHookMsg got assigned number %d\n", ShellHookMsg);

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
        IsDestroying = TRUE;

        /* Unregister the shell hook */
        RegisterShellHook(m_hWnd, FALSE);

        CloseThemeData(TaskBandTheme);
        DeleteAllTasks();
        return TRUE;
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
    
    LRESULT HandleShellHookMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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
            HandleAppCommand(wParam, lParam);
            Ret = TRUE;
            break;

        case HSHELL_WINDOWCREATED:
            Ret = AddTask((HWND) lParam);
            break;

        case HSHELL_WINDOWDESTROYED:
            /* The window still exists! Delay destroying it a bit */
            DeleteTask((HWND) lParam);
            Ret = TRUE;
            break;

        case HSHELL_RUDEAPPACTIVATED:
        case HSHELL_WINDOWACTIVATED:
            if (lParam)
            {
                ActivateTask((HWND) lParam);
                Ret = TRUE;
            }
            break;

        case HSHELL_FLASH:
            FlashTask((HWND) lParam);
            Ret = TRUE;
            break;

        case HSHELL_REDRAW:
            RedrawTask((HWND) lParam);
            Ret = TRUE;
            break;

        case HSHELL_TASKMAN:
            PostMessage(Tray->GetHWND(), TWM_OPENSTARTMENU, 0, 0);
            break;

        case HSHELL_ACTIVATESHELLWINDOW:
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
            for (i = 0, found = 0; i != sizeof(hshell_msg) / sizeof(hshell_msg[0]); i++)
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

    VOID EnableGrouping(IN BOOL bEnable)
    {
        IsGroupingEnabled = bEnable;

        /* Collapse or expand groups if neccessary */
        UpdateButtonsSize(FALSE);
    }

    VOID HandleTaskItemClick(IN OUT PTASK_ITEM TaskItem)
    {
        BOOL bIsMinimized;
        BOOL bIsActive;

        if (::IsWindow(TaskItem->hWnd))
        {
            bIsMinimized = IsIconic(TaskItem->hWnd);
            bIsActive = (TaskItem == ActiveTaskItem);

            TRACE("Active TaskItem %p, selected TaskItem %p\n", ActiveTaskItem, TaskItem);
            if (ActiveTaskItem)
                TRACE("Active TaskItem hWnd=%p, TaskItem hWnd %p\n", ActiveTaskItem->hWnd, TaskItem->hWnd);

            TRACE("Valid button clicked. HWND=%p, IsMinimized=%s, IsActive=%s...\n",
                TaskItem->hWnd, bIsMinimized ? "Yes" : "No", bIsActive ? "Yes" : "No");

            if (!bIsMinimized && bIsActive)
            {
                PostMessage(TaskItem->hWnd,
                    WM_SYSCOMMAND,
                    SC_MINIMIZE,
                    0);
                TRACE("Valid button clicked. App window Minimized.\n");
            }
            else
            {
                if (bIsMinimized)
                {
                    PostMessage(TaskItem->hWnd,
                        WM_SYSCOMMAND,
                        SC_RESTORE,
                        0);
                    TRACE("Valid button clicked. App window Restored.\n");
                }

                SetForegroundWindow(TaskItem->hWnd);
                TRACE("Valid button clicked. App window Activated.\n");
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

        if (IsGroupingEnabled)
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


    VOID HandleTaskItemRightClick(IN OUT PTASK_ITEM TaskItem)
    {

        HMENU hmenu = GetSystemMenu(TaskItem->hWnd, FALSE);

        if (hmenu) 
        {
            POINT pt;
            int cmd;
            GetCursorPos(&pt);
            cmd = TrackPopupMenu(hmenu, TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, TaskBar.m_hWnd, NULL);
            if (cmd)
            {
                SetForegroundWindow(TaskItem->hWnd);    // reactivate window after the context menu has closed
                PostMessage(TaskItem->hWnd, WM_SYSCOMMAND, cmd, 0);
            }
        }
    }

    VOID HandleTaskGroupRightClick(IN OUT PTASK_GROUP TaskGroup)
    {
        /* TODO: Show task group right click menu */
    }

    BOOL HandleButtonRightClick(IN WORD wIndex)
    {
        PTASK_ITEM TaskItem;
        PTASK_GROUP TaskGroup;
        if (IsGroupingEnabled)
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
                /* Make the entire button flashing if neccessary */
                if (nmtbcd->nmcd.uItemState & CDIS_MARKED)
                {
                    Ret = TBCDRF_NOBACKGROUND;
                    if (!TaskBandTheme)
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
                        DrawThemeBackground(TaskBandTheme, nmtbcd->nmcd.hdc, TDP_FLASHBUTTON, 0, &nmtbcd->nmcd.rc, 0);
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

    LRESULT DrawBackground(HDC hdc)
    {
        RECT rect;

        GetClientRect(&rect);
        DrawThemeParentBackground(m_hWnd, hdc, &rect);

        return TRUE;
    }

    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HDC hdc = (HDC) wParam;

        if (!IsAppThemed())
        {
            bHandled = FALSE;
            return 0;
        }

        return DrawBackground(hdc);
    }

    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SIZE szClient;

        szClient.cx = LOWORD(lParam);
        szClient.cy = HIWORD(lParam);
        if (TaskBar.m_hWnd != NULL)
        {
            TaskBar.SetWindowPos(NULL, 0, 0, szClient.cx, szClient.cy, SWP_NOZORDER);

            UpdateButtonsSize(FALSE);
        }
        return TRUE;
    }

    LRESULT OnNcHitTestToolbar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        POINT pt;

        /* See if the mouse is on a button */
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ScreenToClient(&pt);

        INT index = TaskBar.SendMessage(TB_HITTEST, 0, (LPARAM) &pt);
        if (index < 0)
        {
            /* Make the control appear to be transparent outside of any buttons */
            return HTTRANSPARENT;
        }

        bHandled = FALSE;
        return 0;
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
        if (lParam != 0 && (HWND) lParam == TaskBar.m_hWnd)
        {
            HandleButtonClick(LOWORD(wParam));
        }
        return Ret;
    }

    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = TRUE;
        const NMHDR *nmh = (const NMHDR *) lParam;

        if (nmh->hwndFrom == TaskBar.m_hWnd)
        {
            Ret = HandleToolbarNotification(nmh);
        }
        return Ret;
    }

    LRESULT OnEnableGrouping(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = IsGroupingEnabled;
        if ((BOOL)wParam != IsGroupingEnabled)
        {
            EnableGrouping((BOOL) wParam);
        }
        return Ret;
    }

    LRESULT OnUpdateTaskbarPos(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /* Update the button spacing */
        TaskBar.UpdateTbButtonSpacing(Tray->IsHorizontal(), TaskBandTheme != NULL);
        return TRUE;
    }

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret;
        if (TaskBar.m_hWnd != NULL)
        {
            POINT pt;
            INT_PTR iBtn;

            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            ::ScreenToClient(TaskBar.m_hWnd, &pt);

            iBtn = TaskBar.HitTest(&pt);
            if (iBtn >= 0)
            {
                HandleButtonRightClick(iBtn);
            }
            else
                goto ForwardContextMenuMsg;
        }
        else
        {
        ForwardContextMenuMsg:
            /* Forward message */
            Ret = SendMessage(Tray->GetHWND(), uMsg, wParam, lParam);
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
            TaskBar.GetItemRect(TaskItem->Index, &rcItem);
            GetWindowRect(TaskBar.m_hWnd, &rcToolbar);

            OffsetRect(&rcItem, rcToolbar.left, rcToolbar.top);

            *prcMinRect = rcItem;
            return TRUE;
        }
        return FALSE;
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
        MESSAGE_HANDLER(TSWM_ENABLEGROUPING, OnEnableGrouping)
        MESSAGE_HANDLER(TSWM_UPDATETASKBARPOS, OnUpdateTaskbarPos)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(ShellHookMsg, HandleShellHookMsg)
    ALT_MSG_MAP(1)
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTestToolbar)
    END_MSG_MAP()

    HWND _Init(IN HWND hWndParent, IN OUT ITrayWindow *tray)
    {
        Tray = tray;
        hWndNotify = GetParent();
        IsGroupingEnabled = TRUE; /* FIXME */
        return Create(hWndParent, 0, szRunningApps, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP);
    }
};

HWND
CreateTaskSwitchWnd(IN HWND hWndParent, IN OUT ITrayWindow *Tray)
{
    CTaskSwitchWnd * instance;

    // TODO: Destroy after the window is destroyed
    instance = new CTaskSwitchWnd();

    return instance->_Init(hWndParent, Tray);
}
