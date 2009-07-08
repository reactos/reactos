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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <precomp.h>

/* By default we don't use DrawCaptionTemp() because it causes some minimal
   drawing glitches with the toolbar custom painting code */
#define TASK_USE_DRAWCAPTIONTEMP    1

/* Set DUMP_TASKS to 1 to enable a dump of the tasks and task groups every
   5 seconds */
#define DUMP_TASKS  0

static const TCHAR szTaskSwitchWndClass[] = TEXT("MSTaskSwWClass");
static const TCHAR szRunningApps[] = TEXT("Running Applications");

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

#if TASK_USE_DRAWCAPTIONTEMP != 0

            /* DisplayTooltip is TRUE when the group button text didn't fit into
               the button. */
            DWORD DisplayTooltip : 1;

#endif

            DWORD IsCollapsed : 1;
        };
    };
} TASK_GROUP, *PTASK_GROUP;

typedef struct _TASK_ITEM
{
    HWND hWnd;
    PTASK_GROUP Group;
    INT Index;

#if !(TASK_USE_DRAWCAPTIONTEMP != 0)

    INT IconIndex;

#endif

    union
    {
        DWORD dwFlags;
        struct
        {

#if TASK_USE_DRAWCAPTIONTEMP != 0

            /* DisplayTooltip is TRUE when the window text didn't fit into the
               button. */
            DWORD DisplayTooltip : 1;

#endif

            /* IsFlashing is TRUE when the task bar item should be flashing. */
            DWORD IsFlashing : 1;

            /* RenderFlashed is only TRUE if the task bar item should be
               drawn with a flash. */
            DWORD RenderFlashed : 1;
        };
    };
} TASK_ITEM, *PTASK_ITEM;

#define TASK_ITEM_ARRAY_ALLOC   64

typedef struct _TASK_SWITCH_WND
{
    HWND hWnd;
    HWND hWndNotify;

    UINT ShellHookMsg;
    ITrayWindow *Tray;

    PTASK_GROUP TaskGroups;

    WORD TaskItemCount;
    WORD AllocatedTaskItems;
    PTASK_ITEM TaskItems;
    PTASK_ITEM ActiveTaskItem;

    HWND hWndToolbar;
    UINT TbButtonsPerLine;
    WORD ToolbarBtnCount;

    union
    {
        DWORD dwFlags;
        struct
        {
            DWORD IsGroupingEnabled : 1;
            DWORD IsDestroying : 1;
            DWORD IsToolbarSubclassed : 1;
        };
    };

    SIZE ButtonSize;
    TCHAR szBuf[255];
} TASK_SWITCH_WND, *PTASK_SWITCH_WND;

#define TSW_TOOLBAR_SUBCLASS_ID 1

#define MAX_TASKS_COUNT (0x7FFF)

static VOID TaskSwitchWnd_UpdateButtonsSize(IN OUT PTASK_SWITCH_WND This,
                                            IN BOOL bRedrawDisabled);

#if TASK_USE_DRAWCAPTIONTEMP != 0

#define TaskSwitchWnd_GetWndTextFromTaskItem(a,b) NULL

#else /* !TASK_USE_DRAWCAPTIONTEMP */

static LPTSTR
TaskSwitchWnd_GetWndTextFromTaskItem(IN OUT PTASK_SWITCH_WND This,
                                     IN PTASK_ITEM TaskItem)
{
    /* Get the window text without sending a message so we don't hang if an
       application isn't responding! */
    if (InternalGetWindowText(TaskItem->hWnd,
                              This->szBuf,
                              sizeof(This->szBuf) / sizeof(This->szBuf[0])) > 0)
    {
        return This->szBuf;
    }

    return NULL;
}

#endif

#if DUMP_TASKS != 0
static VOID
TaskSwitchWnd_DumpTasks(IN OUT PTASK_SWITCH_WND This)
{
    PTASK_GROUP CurrentGroup;
    PTASK_ITEM CurrentTaskItem, LastTaskItem;

    DbgPrint("Tasks dump:\n");
    if (This->IsGroupingEnabled)
    {
        CurrentGroup = This->TaskGroups;
        while (CurrentGroup != NULL)
        {
            DbgPrint("- Group PID: 0x%p Tasks: %d Index: %d\n", CurrentGroup->dwProcessId, CurrentGroup->dwTaskCount, CurrentGroup->Index);

            CurrentTaskItem = This->TaskItems;
            LastTaskItem = CurrentTaskItem + This->TaskItemCount;
            while (CurrentTaskItem != LastTaskItem)
            {
                if (CurrentTaskItem->Group == CurrentGroup)
                {
                    DbgPrint("  + Task hwnd: 0x%p Index: %d\n", CurrentTaskItem->hWnd, CurrentTaskItem->Index);
                }
                CurrentTaskItem++;
            }

            CurrentGroup = CurrentGroup->Next;
        }

        CurrentTaskItem = This->TaskItems;
        LastTaskItem = CurrentTaskItem + This->TaskItemCount;
        while (CurrentTaskItem != LastTaskItem)
        {
            if (CurrentTaskItem->Group == NULL)
            {
                DbgPrint("- Task hwnd: 0x%p Index: %d\n", CurrentTaskItem->hWnd, CurrentTaskItem->Index);
            }
            CurrentTaskItem++;
        }
    }
    else
    {
        CurrentTaskItem = This->TaskItems;
        LastTaskItem = CurrentTaskItem + This->TaskItemCount;
        while (CurrentTaskItem != LastTaskItem)
        {
            DbgPrint("- Task hwnd: 0x%p Index: %d\n", CurrentTaskItem->hWnd, CurrentTaskItem->Index);
            CurrentTaskItem++;
        }
    }
}
#endif

static VOID
TaskSwitchWnd_BeginUpdate(IN OUT PTASK_SWITCH_WND This)
{
    SendMessage(This->hWndToolbar,
                WM_SETREDRAW,
                FALSE,
                0);
}

static VOID
TaskSwitchWnd_EndUpdate(IN OUT PTASK_SWITCH_WND This)
{
    SendMessage(This->hWndToolbar,
                WM_SETREDRAW,
                TRUE,
                0);
    InvalidateRect(This->hWndToolbar,
                   NULL,
                   TRUE);
}

static BOOL
TaskSwitchWnd_SetToolbarButtonCommandId(IN OUT PTASK_SWITCH_WND This,
                                        IN INT iButtonIndex,
                                        IN INT iCommandId)
{
    TBBUTTONINFO tbbi;

    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_BYINDEX | TBIF_COMMAND;
    tbbi.idCommand = iCommandId;

    return SendMessage(This->hWndToolbar,
                       TB_SETBUTTONINFO,
                       (WPARAM)iButtonIndex,
                       (LPARAM)&tbbi) != 0;
}

static VOID
TaskSwitchWnd_UpdateIndexesAfterButtonInserted(IN OUT PTASK_SWITCH_WND This,
                                               IN INT iIndex)
{
    PTASK_GROUP CurrentGroup;
    PTASK_ITEM CurrentTaskItem, LastTaskItem;
    INT NewIndex;

    if (This->IsGroupingEnabled)
    {
        /* Update all affected groups */
        CurrentGroup = This->TaskGroups;
        while (CurrentGroup != NULL)
        {
            if (CurrentGroup->IsCollapsed &&
                CurrentGroup->Index >= iIndex)
            {
                /* Update the toolbar buttons */
                NewIndex = CurrentGroup->Index + 1;
                if (TaskSwitchWnd_SetToolbarButtonCommandId(This,
                                                            CurrentGroup->Index + 1,
                                                            NewIndex))
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
    CurrentTaskItem = This->TaskItems;
    LastTaskItem = CurrentTaskItem + This->TaskItemCount;
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
            NewIndex = CurrentTaskItem->Index + 1;
            if (TaskSwitchWnd_SetToolbarButtonCommandId(This,
                                                        CurrentTaskItem->Index + 1,
                                                        NewIndex))
            {
                CurrentTaskItem->Index = NewIndex;
            }
            else
                CurrentTaskItem->Index = -1;
        }

        CurrentTaskItem++;
    }
}

static VOID
TaskSwitchWnd_UpdateIndexesAfterButtonDeleted(IN OUT PTASK_SWITCH_WND This,
                                              IN INT iIndex)
{
    PTASK_GROUP CurrentGroup;
    PTASK_ITEM CurrentTaskItem, LastTaskItem;
    INT NewIndex;

    if (This->IsGroupingEnabled)
    {
        /* Update all affected groups */
        CurrentGroup = This->TaskGroups;
        while (CurrentGroup != NULL)
        {
            if (CurrentGroup->IsCollapsed &&
                CurrentGroup->Index > iIndex)
            {
                /* Update the toolbar buttons */
                NewIndex = CurrentGroup->Index - 1;
                if (TaskSwitchWnd_SetToolbarButtonCommandId(This,
                                                            CurrentGroup->Index - 1,
                                                            NewIndex))
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
    CurrentTaskItem = This->TaskItems;
    LastTaskItem = CurrentTaskItem + This->TaskItemCount;
    while (CurrentTaskItem != LastTaskItem)
    {
        CurrentGroup = CurrentTaskItem->Group;
        if (CurrentGroup != NULL)
        {
            if (!CurrentGroup->IsCollapsed &&
                CurrentTaskItem->Index > iIndex)
            {
                goto UpdateTaskItemBtn;
            }
        }
        else if (CurrentTaskItem->Index > iIndex)
        {
UpdateTaskItemBtn:
            /* Update the toolbar buttons */
            NewIndex = CurrentTaskItem->Index - 1;
            if (TaskSwitchWnd_SetToolbarButtonCommandId(This,
                                                        CurrentTaskItem->Index - 1,
                                                        NewIndex))
            {
                CurrentTaskItem->Index = NewIndex;
            }
            else
                CurrentTaskItem->Index = -1;
        }

        CurrentTaskItem++;
    }
}

static INT
TaskSwitchWnd_UpdateTaskGroupButton(IN OUT PTASK_SWITCH_WND This,
                                    IN PTASK_GROUP TaskGroup)
{
    ASSERT(TaskGroup->Index >= 0);

    /* FIXME: Implement */

    return TaskGroup->Index;
}

static VOID
TaskSwitchWnd_ExpandTaskGroup(IN OUT PTASK_SWITCH_WND This,
                              IN PTASK_GROUP TaskGroup)
{
    ASSERT(TaskGroup->dwTaskCount > 0);
    ASSERT(TaskGroup->IsCollapsed);
    ASSERT(TaskGroup->Index >= 0);

    /* FIXME: Implement */
}

static INT
TaskSwitchWnd_UpdateTaskItemButton(IN OUT PTASK_SWITCH_WND This,
                                   IN PTASK_ITEM TaskItem)
{
    TBBUTTONINFO tbbi;

    ASSERT(TaskItem->Index >= 0);

    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_BYINDEX | TBIF_STATE | TBIF_TEXT;
    tbbi.fsState = TBSTATE_ENABLED;
    if (This->ActiveTaskItem == TaskItem)
        tbbi.fsState |= TBSTATE_CHECKED;

    /* Check if we're updating a button that is the last one in the
       line. If so, we need to set the TBSTATE_WRAP flag! */
    if (This->TbButtonsPerLine != 0 &&
        (TaskItem->Index + 1) % This->TbButtonsPerLine == 0)
    {
        tbbi.fsState |= TBSTATE_WRAP;
    }

    tbbi.pszText = TaskSwitchWnd_GetWndTextFromTaskItem(This,
                                                        TaskItem);

    if (!SendMessage(This->hWndToolbar,
                     TB_SETBUTTONINFO,
                     (WPARAM)TaskItem->Index,
                     (LPARAM)&tbbi))
    {
        TaskItem->Index = -1;
        return -1;
    }

    DbgPrint("Updated button %d for hwnd 0x%p\n", TaskItem->Index, TaskItem->hWnd);
    return TaskItem->Index;
}

static PTASK_ITEM
TaskSwitchWnd_FindLastTaskItemOfGroup(IN OUT PTASK_SWITCH_WND This,
                                      IN PTASK_GROUP TaskGroup  OPTIONAL,
                                      IN PTASK_ITEM NewTaskItem  OPTIONAL)
{
    PTASK_ITEM TaskItem, LastTaskItem, FoundTaskItem = NULL;
    DWORD dwTaskCount;

    ASSERT(This->IsGroupingEnabled);

    TaskItem = This->TaskItems;
    LastTaskItem = TaskItem + This->TaskItemCount;

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

static INT
TaskSwitchWnd_CalculateTaskItemNewButtonIndex(IN OUT PTASK_SWITCH_WND This,
                                              IN PTASK_ITEM TaskItem)
{
    PTASK_GROUP TaskGroup;
    PTASK_ITEM LastTaskItem;

    /* NOTE: This routine assumes that the group is *not* collapsed! */

    TaskGroup = TaskItem->Group;
    if (This->IsGroupingEnabled)
    {
        if (TaskGroup != NULL)
        {
            ASSERT(TaskGroup->Index < 0);
            ASSERT(!TaskGroup->IsCollapsed);

            if (TaskGroup->dwTaskCount > 1)
            {
                LastTaskItem = TaskSwitchWnd_FindLastTaskItemOfGroup(This,
                                                                     TaskGroup,
                                                                     TaskItem);
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
            LastTaskItem = TaskSwitchWnd_FindLastTaskItemOfGroup(This,
                                                                 NULL,
                                                                 TaskItem);
            if (LastTaskItem != NULL)
            {
                ASSERT(LastTaskItem->Index >= 0);

                return LastTaskItem->Index + 1;
            }
        }
    }

    return This->ToolbarBtnCount;
}

static INT
TaskSwitchWnd_AddTaskItemButton(IN OUT PTASK_SWITCH_WND This,
                                IN OUT PTASK_ITEM TaskItem)
{
    TBBUTTON tbBtn;
    INT iIndex;

    if (TaskItem->Index >= 0)
    {
        return TaskSwitchWnd_UpdateTaskItemButton(This,
                                                  TaskItem);
    }

    if (TaskItem->Group != NULL &&
        TaskItem->Group->IsCollapsed)
    {
        /* The task group is collapsed, we only need to update the group button */
        return TaskSwitchWnd_UpdateTaskGroupButton(This,
                                                   TaskItem->Group);
    }

    tbBtn.iBitmap = 0;
    tbBtn.fsState = TBSTATE_ENABLED | TBSTATE_ELLIPSES;
    tbBtn.fsStyle = BTNS_CHECK | BTNS_NOPREFIX | BTNS_SHOWTEXT;
    tbBtn.dwData = TaskItem->Index;

    tbBtn.iString = (DWORD_PTR)TaskSwitchWnd_GetWndTextFromTaskItem(This,
                                                                    TaskItem);

    /* Find out where to insert the new button */
    iIndex = TaskSwitchWnd_CalculateTaskItemNewButtonIndex(This,
                                                           TaskItem);
    ASSERT(iIndex >= 0);
    tbBtn.idCommand = iIndex;

    TaskSwitchWnd_BeginUpdate(This);

    if (SendMessage(This->hWndToolbar,
                    TB_INSERTBUTTON,
                    (WPARAM)iIndex,
                    (LPARAM)&tbBtn))
    {
        TaskSwitchWnd_UpdateIndexesAfterButtonInserted(This,
                                                       iIndex);

        DbgPrint("Added button %d for hwnd 0x%p\n", iIndex, TaskItem->hWnd);

        TaskItem->Index = iIndex;
        This->ToolbarBtnCount++;

        /* Update button sizes and fix the button wrapping */
        TaskSwitchWnd_UpdateButtonsSize(This,
                                        TRUE);
        return iIndex;
    }

    TaskSwitchWnd_EndUpdate(This);

    return -1;
}

static BOOL
TaskSwitchWnd_DeleteTaskItemButton(IN OUT PTASK_SWITCH_WND This,
                                   IN OUT PTASK_ITEM TaskItem)
{
    PTASK_GROUP TaskGroup;
    INT iIndex;

    TaskGroup = TaskItem->Group;

    if (TaskItem->Index >= 0)
    {
        if ((TaskGroup != NULL && !TaskGroup->IsCollapsed) ||
            TaskGroup == NULL)
        {
            TaskSwitchWnd_BeginUpdate(This);

            iIndex = TaskItem->Index;
            if (SendMessage(This->hWndToolbar,
                            TB_DELETEBUTTON,
                            (WPARAM)iIndex,
                            0))
            {
                TaskItem->Index = -1;
                This->ToolbarBtnCount--;

                TaskSwitchWnd_UpdateIndexesAfterButtonDeleted(This,
                                                              iIndex);

                /* Update button sizes and fix the button wrapping */
                TaskSwitchWnd_UpdateButtonsSize(This,
                                                TRUE);
                return TRUE;
            }

            TaskSwitchWnd_EndUpdate(This);
        }
    }

    return FALSE;
}

static PTASK_GROUP
TaskSwitchWnd_AddToTaskGroup(IN OUT PTASK_SWITCH_WND This,
                             IN HWND hWnd)
{
    DWORD dwProcessId;
    PTASK_GROUP TaskGroup, *PrevLink;

    if (!GetWindowThreadProcessId(hWnd,
                                  &dwProcessId))
    {
        DbgPrint("Cannot get process id of hwnd 0x%p\n", hWnd);
        return NULL;
    }

    /* Try to find an existing task group */
    TaskGroup = This->TaskGroups;
    PrevLink = &This->TaskGroups;
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
    TaskGroup = HeapAlloc(hProcessHeap,
                          0,
                          sizeof(*TaskGroup));
    if (TaskGroup != NULL)
    {
        ZeroMemory(TaskGroup,
                   sizeof(*TaskGroup));

        TaskGroup->dwTaskCount = 1;
        TaskGroup->dwProcessId = dwProcessId;
        TaskGroup->Index = -1;

        /* Add the task group to the list */
        *PrevLink = TaskGroup;
    }

    return TaskGroup;
}

static VOID
TaskSwitchWnd_RemoveTaskFromTaskGroup(IN OUT PTASK_SWITCH_WND This,
                                      IN OUT PTASK_ITEM TaskItem)
{
    PTASK_GROUP TaskGroup, CurrentGroup, *PrevLink;

    TaskGroup = TaskItem->Group;
    if (TaskGroup != NULL)
    {
        DWORD dwNewTaskCount = --TaskGroup->dwTaskCount;
        if (dwNewTaskCount == 0)
        {
            /* Find the previous pointer in the chain */
            CurrentGroup = This->TaskGroups;
            PrevLink = &This->TaskGroups;
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
                TaskSwitchWnd_UpdateTaskGroupButton(This,
                                                    TaskGroup);
            }
            else
            {
                /* Expand the group of one task button to a task button */
                TaskSwitchWnd_ExpandTaskGroup(This,
                                              TaskGroup);
            }
        }
    }
}

static PTASK_ITEM
TaskSwitchWnd_FindTaskItem(IN OUT PTASK_SWITCH_WND This,
                           IN HWND hWnd)
{
    PTASK_ITEM TaskItem, LastItem;

    TaskItem = This->TaskItems;
    LastItem = TaskItem + This->TaskItemCount;
    while (TaskItem != LastItem)
    {
        if (TaskItem->hWnd == hWnd)
            return TaskItem;

        TaskItem++;
    }

    return NULL;
}

static PTASK_ITEM
TaskSwitchWnd_FindOtherTaskItem(IN OUT PTASK_SWITCH_WND This,
                                IN HWND hWnd)
{
    PTASK_ITEM LastItem, TaskItem;
    PTASK_GROUP TaskGroup;
    DWORD dwProcessId;

    if (!GetWindowThreadProcessId(hWnd,
                                  &dwProcessId))
    {
        return NULL;
    }

    /* Try to find another task that belongs to the same
       process as the given window */
    TaskItem = This->TaskItems;
    LastItem = TaskItem + This->TaskItemCount;
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

static PTASK_ITEM
TaskSwitchWnd_AllocTaskItem(IN OUT PTASK_SWITCH_WND This)
{
    if (This->TaskItemCount >= MAX_TASKS_COUNT)
    {
        /* We need the most significant bit in 16 bit command IDs to indicate whether it
           is a task group or task item. WM_COMMAND limits command IDs to 16 bits! */
        return NULL;
    }

    ASSERT(This->AllocatedTaskItems >= This->TaskItemCount);

    if (This->TaskItemCount != 0)
    {
        PTASK_ITEM NewArray;
        SIZE_T NewArrayLength, ActiveTaskItemIndex;

        NewArrayLength = This->AllocatedTaskItems + TASK_ITEM_ARRAY_ALLOC;

        NewArray = HeapReAlloc(hProcessHeap,
                               0,
                               This->TaskItems,
                               NewArrayLength * sizeof(*This->TaskItems));
        if (NewArray != NULL)
        {
            if (This->ActiveTaskItem != NULL)
            {
                /* Fixup the ActiveTaskItem pointer */
                ActiveTaskItemIndex = This->ActiveTaskItem - This->TaskItems;
                This->ActiveTaskItem = NewArray + ActiveTaskItemIndex;
            }
            This->AllocatedTaskItems = (WORD)NewArrayLength;
            This->TaskItems = NewArray;
        }
        else
            return NULL;
    }
    else
    {
        This->TaskItems = HeapAlloc(hProcessHeap,
                                    0,
                                    TASK_ITEM_ARRAY_ALLOC * sizeof(*This->TaskItems));
        if (This->TaskItems != NULL)
        {
            This->AllocatedTaskItems = TASK_ITEM_ARRAY_ALLOC;
        }
        else
            return NULL;
    }

    return This->TaskItems + This->TaskItemCount++;
}

static VOID
TaskSwitchWnd_FreeTaskItem(IN OUT PTASK_SWITCH_WND This,
                           IN OUT PTASK_ITEM TaskItem)
{
    WORD wIndex;

    if (TaskItem == This->ActiveTaskItem)
        This->ActiveTaskItem = NULL;

    wIndex = (WORD)(TaskItem - This->TaskItems);
    if (wIndex + 1 < This->TaskItemCount)
    {
        MoveMemory(TaskItem,
                   TaskItem + 1,
                   (This->TaskItemCount - wIndex - 1) * sizeof(*TaskItem));
    }

    This->TaskItemCount--;
}

static VOID
TaskSwitchWnd_DeleteTaskItem(IN OUT PTASK_SWITCH_WND This,
                             IN OUT PTASK_ITEM TaskItem)
{
    if (!This->IsDestroying)
    {
        /* Delete the task button from the toolbar */
        TaskSwitchWnd_DeleteTaskItemButton(This,
                                           TaskItem);
    }

    /* Remove the task from it's group */
    TaskSwitchWnd_RemoveTaskFromTaskGroup(This,
                                          TaskItem);

    /* Free the task item */
    TaskSwitchWnd_FreeTaskItem(This,
                               TaskItem);
}

static VOID
TaskSwitchWnd_CheckActivateTaskItem(IN OUT PTASK_SWITCH_WND This,
                                    IN OUT PTASK_ITEM TaskItem)
{
    PTASK_ITEM ActiveTaskItem;
    PTASK_GROUP TaskGroup = NULL;

    ActiveTaskItem = This->ActiveTaskItem;

    if (TaskItem != NULL)
        TaskGroup = TaskItem->Group;

    if (This->IsGroupingEnabled && TaskGroup != NULL)
    {
        if (TaskGroup->IsCollapsed)
        {
            /* FIXME */
        }
        else
            goto ChangeTaskItemButton;
    }
    else
    {
ChangeTaskItemButton:
        if (ActiveTaskItem != NULL)
        {
            PTASK_GROUP ActiveTaskGroup;

            if (ActiveTaskItem == TaskItem)
                return;

            ActiveTaskGroup = ActiveTaskItem->Group;

            if (This->IsGroupingEnabled && ActiveTaskGroup != NULL)
            {
                if (ActiveTaskGroup->IsCollapsed)
                {
                    if (ActiveTaskGroup == TaskGroup)
                        return;

                    /* FIXME */
                }
                else
                    goto ChangeActiveTaskItemButton;
            }
            else
            {
ChangeActiveTaskItemButton:
                This->ActiveTaskItem = NULL;
                if (ActiveTaskItem->Index >= 0)
                {
                    TaskSwitchWnd_UpdateTaskItemButton(This,
                                                       ActiveTaskItem);
                }
            }
        }

        This->ActiveTaskItem = TaskItem;

        if (TaskItem != NULL && TaskItem->Index >= 0)
        {
            TaskSwitchWnd_UpdateTaskItemButton(This,
                                               TaskItem);
        }
    }
}

static PTASK_ITEM
FindTaskItemByIndex(IN OUT PTASK_SWITCH_WND This,
                    IN INT Index)
{
    PTASK_ITEM TaskItem, LastItem;

    TaskItem = This->TaskItems;
    LastItem = TaskItem + This->TaskItemCount;
    while (TaskItem != LastItem)
    {
        if (TaskItem->Index == Index)
            return TaskItem;

        TaskItem++;
    }

    return NULL;
}

static PTASK_GROUP
FindTaskGroupByIndex(IN OUT PTASK_SWITCH_WND This,
                     IN INT Index)
{
    PTASK_GROUP CurrentGroup;

    CurrentGroup = This->TaskGroups;
    while (CurrentGroup != NULL)
    {
        if (CurrentGroup->Index == Index)
            break;

        CurrentGroup = CurrentGroup->Next;
    }

    return CurrentGroup;
}

static BOOL
TaskSwitchWnd_AddTask(IN OUT PTASK_SWITCH_WND This,
                      IN HWND hWnd)
{
    PTASK_ITEM TaskItem;

    TaskItem = TaskSwitchWnd_FindTaskItem(This,
                                          hWnd);
    if (TaskItem == NULL)
    {
        DbgPrint("Add window 0x%p\n", hWnd);
        TaskItem = TaskSwitchWnd_AllocTaskItem(This);
        if (TaskItem != NULL)
        {
            ZeroMemory(TaskItem,
                       sizeof(*TaskItem));
            TaskItem->hWnd = hWnd;
            TaskItem->Index = -1;
            TaskItem->Group = TaskSwitchWnd_AddToTaskGroup(This,
                                                           hWnd);

            if (!This->IsDestroying)
            {
                TaskSwitchWnd_AddTaskItemButton(This,
                                                TaskItem);
            }
        }
    }

    return TaskItem != NULL;
}

static BOOL
TaskSwitchWnd_ActivateTaskItem(IN OUT PTASK_SWITCH_WND This,
                               IN OUT PTASK_ITEM TaskItem)
{
    if (TaskItem != NULL)
    {
        DbgPrint("Activate window 0x%p on button %d\n", TaskItem->hWnd, TaskItem->Index);
    }

    TaskSwitchWnd_CheckActivateTaskItem(This,
                                        TaskItem);

    return FALSE;
}

static BOOL
TaskSwitchWnd_ActivateTask(IN OUT PTASK_SWITCH_WND This,
                           IN HWND hWnd)
{
    PTASK_ITEM TaskItem;

    TaskItem = TaskSwitchWnd_FindTaskItem(This,
                                          hWnd);
    if (TaskItem == NULL)
    {
        TaskItem = TaskSwitchWnd_FindOtherTaskItem(This,
                                                   hWnd);
    }
    
    if (TaskItem == NULL)
    {
        DbgPrint("Activate window 0x%p, could not find task\n", hWnd);
    }

    return TaskSwitchWnd_ActivateTaskItem(This,
                                          TaskItem);
}

static BOOL
TaskSwitchWnd_DeleteTask(IN OUT PTASK_SWITCH_WND This,
                         IN HWND hWnd)
{
    PTASK_ITEM TaskItem;

    TaskItem = TaskSwitchWnd_FindTaskItem(This,
                                          hWnd);
    if (TaskItem != NULL)
    {
        DbgPrint("Delete window 0x%p on button %d\n", hWnd, TaskItem->Index);
        TaskSwitchWnd_DeleteTaskItem(This,
                                     TaskItem);
        return TRUE;
    }
    //else
        //DbgPrint("Failed to delete window 0x%p\n", hWnd);

    return FALSE;
}

static VOID
TaskSwitchWnd_DeleteAllTasks(IN OUT PTASK_SWITCH_WND This)
{
    PTASK_ITEM CurrentTask;

    if (This->TaskItemCount > 0)
    {
        CurrentTask = This->TaskItems + This->TaskItemCount;
        do
        {
            TaskSwitchWnd_DeleteTaskItem(This,
                                         --CurrentTask);
        } while (CurrentTask != This->TaskItems);
    }
}

static VOID
TaskSwitchWnd_FlashTaskItem(IN OUT PTASK_SWITCH_WND This,
                            IN OUT PTASK_ITEM TaskItem)
{
    /* FIXME: Implement */
}

static BOOL
TaskSwitchWnd_FlashTask(IN OUT PTASK_SWITCH_WND This,
                        IN HWND hWnd)
{
    PTASK_ITEM TaskItem;

    TaskItem = TaskSwitchWnd_FindTaskItem(This,
                                          hWnd);
    if (TaskItem != NULL)
    {
        DbgPrint("Flashing window 0x%p on button %d\n", hWnd, TaskItem->Index);
        TaskSwitchWnd_FlashTaskItem(This,
                                    TaskItem);
        return TRUE;
    }

    return FALSE;
}

static VOID
TaskSwitchWnd_RedrawTaskItem(IN OUT PTASK_SWITCH_WND This,
                             IN OUT PTASK_ITEM TaskItem)
{
    PTASK_GROUP TaskGroup;

    TaskGroup = TaskItem->Group;
    if (This->IsGroupingEnabled && TaskGroup != NULL)
    {
        if (TaskGroup->IsCollapsed && TaskGroup->Index >= 0)
        {
            TaskSwitchWnd_UpdateTaskGroupButton(This,
                                                TaskGroup);
        }
        else if (TaskItem->Index >= 0)
        {
            goto UpdateTaskItem;
        }
    }
    else if (TaskItem->Index >= 0)
    {
UpdateTaskItem:
        TaskSwitchWnd_UpdateTaskItemButton(This,
                                           TaskItem);
    }
}


static BOOL
TaskSwitchWnd_RedrawTask(IN OUT PTASK_SWITCH_WND This,
                         IN HWND hWnd)
{
    PTASK_ITEM TaskItem;

    TaskItem = TaskSwitchWnd_FindTaskItem(This,
                                          hWnd);
    if (TaskItem != NULL)
    {
        TaskSwitchWnd_RedrawTaskItem(This,
                                     TaskItem);
        return TRUE;
    }

    return FALSE;
}

static INT
TaskSwitchWnd_UpdateTbButtonSpacing(IN OUT PTASK_SWITCH_WND This,
                                    IN BOOL bHorizontal,
                                    IN UINT uiRows,
                                    IN UINT uiBtnsPerLine)
{
    TBMETRICS tbm;

    tbm.cbSize = sizeof(tbm);
    tbm.dwMask = TBMF_BARPAD | TBMF_BUTTONSPACING;

    tbm.cxBarPad = tbm.cyBarPad = 0;

    if (bHorizontal || uiBtnsPerLine > 1)
        tbm.cxButtonSpacing = (3 * GetSystemMetrics(SM_CXEDGE) / 2);
    else
        tbm.cxButtonSpacing = 0;

    if (!bHorizontal || uiRows > 1)
        tbm.cyButtonSpacing = (3 * GetSystemMetrics(SM_CYEDGE) / 2);
    else
        tbm.cyButtonSpacing = 0;

    SendMessage(This->hWndToolbar,
                TB_SETMETRICS,
                0,
                (LPARAM)&tbm);

    return tbm.cxButtonSpacing;
}


static VOID
TaskSwitchWnd_UpdateButtonsSize(IN OUT PTASK_SWITCH_WND This,
                                IN BOOL bRedrawDisabled)
{
    RECT rcClient;
    UINT uiRows, uiMax, uiMin, uiBtnsPerLine, ui;
    LONG NewBtnSize;
    BOOL Horizontal;
    TBBUTTONINFO tbbi;
    TBMETRICS tbm;

    if (GetClientRect(This->hWnd,
                      &rcClient) &&
        !IsRectEmpty(&rcClient))
    {
        if (This->ToolbarBtnCount > 0)
        {
            ZeroMemory (&tbm, sizeof (tbm));
            tbm.cbSize = sizeof(tbm);
            tbm.dwMask = TBMF_BUTTONSPACING;
            SendMessage(This->hWndToolbar,
                        TB_GETMETRICS,
                        0,
                        (LPARAM)&tbm);

            uiRows = (rcClient.bottom + tbm.cyButtonSpacing) / (This->ButtonSize.cy + tbm.cyButtonSpacing);
            if (uiRows == 0)
                uiRows = 1;

            uiBtnsPerLine = (This->ToolbarBtnCount + uiRows - 1) / uiRows;

            Horizontal = ITrayWindow_IsHorizontal(This->Tray);

            if (!bRedrawDisabled)
                TaskSwitchWnd_BeginUpdate(This);

            /* We might need to update the button spacing */
            tbm.cxButtonSpacing = TaskSwitchWnd_UpdateTbButtonSpacing(This,
                                                                      Horizontal,
                                                                      uiRows,
                                                                      uiBtnsPerLine);

            /* Calculate the ideal width and make sure it's within the allowed range */
            NewBtnSize = (rcClient.right - (uiBtnsPerLine * tbm.cxButtonSpacing)) / uiBtnsPerLine;

            /* Determine the minimum and maximum width of a button */
            if (Horizontal)
                uiMax = GetSystemMetrics(SM_CXMINIMIZED);
            else
                uiMax = rcClient.right;

            uiMin = GetSystemMetrics(SM_CXSIZE) + (2 * GetSystemMetrics(SM_CXEDGE));

            if (NewBtnSize < (LONG)uiMin)
                NewBtnSize = uiMin;
            if (NewBtnSize > (LONG)uiMax)
                NewBtnSize = uiMax;

            This->ButtonSize.cx = NewBtnSize;

            /* Recalculate how many buttons actually fit into one line */
            uiBtnsPerLine = rcClient.right / (NewBtnSize + tbm.cxButtonSpacing);
            if (uiBtnsPerLine == 0)
                uiBtnsPerLine++;
            This->TbButtonsPerLine = uiBtnsPerLine;

            tbbi.cbSize = sizeof(tbbi);
            tbbi.dwMask = TBIF_BYINDEX | TBIF_SIZE | TBIF_STATE;
            tbbi.cx = (INT)NewBtnSize;

            for (ui = 0; ui != This->ToolbarBtnCount; ui++)
            {
                tbbi.fsState = TBSTATE_ENABLED;

                /* Check if we're updating a button that is the last one in the
                   line. If so, we need to set the TBSTATE_WRAP flag! */
                if ((ui + 1) % uiBtnsPerLine == 0)
                    tbbi.fsState |= TBSTATE_WRAP;

                if (This->ActiveTaskItem != NULL &&
                    This->ActiveTaskItem->Index == ui)
                {
                    tbbi.fsState |= TBSTATE_CHECKED;
                }

                SendMessage(This->hWndToolbar,
                            TB_SETBUTTONINFO,
                            (WPARAM)ui,
                            (LPARAM)&tbbi);
            }

#if 0
            /* FIXME: Force the window to the correct position in case some idiot
                      did something to us */
            SetWindowPos(This->hWndToolbar,
                         NULL,
                         0,
                         0,
                         rcClient.right, /* FIXME */
                         rcClient.bottom, /* FIXME */
                         SWP_NOACTIVATE | SWP_NOZORDER);
#endif
        }
        else
        {
            This->TbButtonsPerLine = 0;
            This->ButtonSize.cx = 0;
        }
    }

    TaskSwitchWnd_EndUpdate(This);
}

static BOOL CALLBACK
TaskSwitchWnd_EnumWindowsProc(IN HWND hWnd,
                              IN LPARAM lParam)
{
    PTASK_SWITCH_WND This = (PTASK_SWITCH_WND)lParam;

    /* Only show windows that still exist and are visible and none of explorer's
       special windows (such as the desktop or the tray window) */
    if (IsWindow(hWnd) && IsWindowVisible(hWnd) &&
        !ITrayWindow_IsSpecialHWND(This->Tray,
                                   hWnd))
    {
        /* Don't list popup windows and also no tool windows */
        if (GetWindow(hWnd,
                      GW_OWNER) == NULL &&
            !(GetWindowLongPtr(hWnd,
                               GWL_EXSTYLE) & WS_EX_TOOLWINDOW))
        {
            TaskSwitchWnd_AddTask(This,
                                  hWnd);
        }
    }

    return TRUE;
}

static LRESULT CALLBACK
TaskSwichWnd_ToolbarSubclassedProc(IN HWND hWnd,
                                   IN UINT msg,
                                   IN WPARAM wParam,
                                   IN LPARAM lParam,
                                   IN UINT_PTR uIdSubclass,
                                   IN DWORD_PTR dwRefData)
{
    LRESULT Ret;

    Ret = DefSubclassProc(hWnd,
                          msg,
                          wParam,
                          lParam);

    if (msg == WM_NCHITTEST && Ret == HTCLIENT)
    {
        POINT pt;

        /* See if the mouse is on a button */
        pt.x = (SHORT)LOWORD(lParam);
        pt.y = (SHORT)HIWORD(lParam);

        if (MapWindowPoints(HWND_DESKTOP,
                            hWnd,
                            &pt,
                            1) != 0 &&
            (INT)SendMessage(hWnd,
                             TB_HITTEST,
                             0,
                             (LPARAM)&pt) < 0)
        {
            /* Make the control appear to be transparent outside of any buttons */
            Ret = HTTRANSPARENT;
        }
    }

    return Ret;
}

static VOID
TaskSwitchWnd_Create(IN OUT PTASK_SWITCH_WND This)
{
    This->hWndToolbar = CreateWindowEx(0,
                                       TOOLBARCLASSNAME,
                                       szRunningApps,
                                       WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN |
                                           TBSTYLE_TOOLTIPS | TBSTYLE_WRAPABLE | TBSTYLE_LIST |
                                           TBSTYLE_TRANSPARENT |
                                           CCS_TOP | CCS_NORESIZE | CCS_NODIVIDER,
                                       0,
                                       0,
                                       0,
                                       0,
                                       This->hWnd,
                                       NULL,
                                       hExplorerInstance,
                                       NULL);

    if (This->hWndToolbar != NULL)
    {
        HMODULE hShell32;
        SIZE BtnSize;

        /* Identify the version we're using */
        SendMessage(This->hWndToolbar,
                    TB_BUTTONSTRUCTSIZE,
                    sizeof(TBBUTTON),
                    0);

        /* Calculate the default button size. Don't save this in This->ButtonSize.cx so that
           the actual button width gets updated correctly on the first recalculation */
        BtnSize.cx = GetSystemMetrics(SM_CXMINIMIZED);
        This->ButtonSize.cy = BtnSize.cy = GetSystemMetrics(SM_CYSIZE) + (2 * GetSystemMetrics(SM_CYEDGE));
        SendMessage(This->hWndToolbar,
                    TB_SETBUTTONSIZE,
                    0,
                    (LPARAM)MAKELONG(BtnSize.cx,
                                     BtnSize.cy));

        /* We don't want to see partially clipped buttons...not that we could see them... */
#if 0
        SendMessage(This->hWndToolbar,
                    TB_SETEXTENDEDSTYLE,
                    0,
                    TBSTYLE_EX_HIDECLIPPEDBUTTONS);
#endif

        /* Set proper spacing between buttons */
        TaskSwitchWnd_UpdateTbButtonSpacing(This,
                                            ITrayWindow_IsHorizontal(This->Tray),
                                            0,
                                            0);

        /* Register the shell hook */
        This->ShellHookMsg = RegisterWindowMessage(TEXT("SHELLHOOK"));
        hShell32 = GetModuleHandle(TEXT("SHELL32.DLL"));
        if (hShell32 != NULL)
        {
            REGSHELLHOOK RegShellHook;

            /* RegisterShellHook */
            RegShellHook = (REGSHELLHOOK)GetProcAddress(hShell32,
                                                        (LPCSTR)((LONG)181));
            if (RegShellHook != NULL)
            {
                RegShellHook(This->hWnd,
                             3); /* 1 if no NT! We're targeting NT so we don't care! */
            }
        }

        /* Add all windows to the toolbar */
        EnumWindows(TaskSwitchWnd_EnumWindowsProc,
                    (LPARAM)This);

        /* Recalculate the button size */
        TaskSwitchWnd_UpdateButtonsSize(This,
                                        FALSE);

        /* Subclass the toolbar control because it doesn't provide a
           NM_NCHITTEST notification */
        This->IsToolbarSubclassed = SetWindowSubclass(This->hWndToolbar,
                                                      TaskSwichWnd_ToolbarSubclassedProc,
                                                      TSW_TOOLBAR_SUBCLASS_ID,
                                                      (DWORD_PTR)This);
    }
}

static VOID
TaskSwitchWnd_NCDestroy(IN OUT PTASK_SWITCH_WND This)
{
    HMODULE hShell32;

    This->IsDestroying = TRUE;

    /* Unregister the shell hook */
    hShell32 = GetModuleHandle(TEXT("SHELL32.DLL"));
    if (hShell32 != NULL)
    {
        REGSHELLHOOK RegShellHook;

        /* RegisterShellHook */
        RegShellHook = (REGSHELLHOOK)GetProcAddress(hShell32,
                                                    (LPCSTR)((LONG)181));
        if (RegShellHook != NULL)
        {
            RegShellHook(This->hWnd,
                         FALSE);
        }
    }

    TaskSwitchWnd_DeleteAllTasks(This);
}

static BOOL
TaskSwitchWnd_HandleAppCommand(IN OUT PTASK_SWITCH_WND This,
                               IN WPARAM wParam,
                               IN LPARAM lParam)
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
            DbgPrint("Shell app command %d unhandled!\n", (INT)GET_APPCOMMAND_LPARAM(lParam));
            break;
    }

    return Ret;
}


static LRESULT
TaskSwitchWnd_HandleShellHookMsg(IN OUT PTASK_SWITCH_WND This,
                                 IN WPARAM wParam,
                                 IN LPARAM lParam)
{
    BOOL Ret = FALSE;

    switch ((INT)wParam)
    {
        case HSHELL_APPCOMMAND:
            TaskSwitchWnd_HandleAppCommand(This,
                                           wParam,
                                           lParam);
            Ret = TRUE;
            break;

        case HSHELL_WINDOWCREATED:
            TaskSwitchWnd_AddTask(This,
                                  (HWND)lParam);
            Ret = TRUE;
            break;

        case HSHELL_WINDOWDESTROYED:
            /* The window still exists! Delay destroying it a bit */
            TaskSwitchWnd_DeleteTask(This,
                                     (HWND)lParam);
            Ret = TRUE;
            break;

        case HSHELL_ACTIVATESHELLWINDOW:
            goto UnhandledShellMessage;

        case HSHELL_RUDEAPPACTIVATED:
            goto UnhandledShellMessage;

        case HSHELL_WINDOWACTIVATED:
            TaskSwitchWnd_ActivateTask(This,
                                       (HWND)lParam);
            Ret = TRUE;
            break;

        case HSHELL_GETMINRECT:
            goto UnhandledShellMessage;

        case HSHELL_FLASH:
            TaskSwitchWnd_FlashTask(This,
                                    (HWND)lParam);
            Ret = TRUE;
            break;

        case HSHELL_REDRAW:
            TaskSwitchWnd_RedrawTask(This,
                                     (HWND)lParam);
            Ret = TRUE;
            break;

        case HSHELL_TASKMAN:
        case HSHELL_LANGUAGE:
        case HSHELL_SYSMENU:
        case HSHELL_ENDTASK:
        case HSHELL_ACCESSIBILITYSTATE:
        case HSHELL_WINDOWREPLACED:
        case HSHELL_WINDOWREPLACING:
        default:
        {
            static const struct {
                INT msg;
                LPCWSTR msg_name;
            } hshell_msg[] = {
                {HSHELL_WINDOWCREATED, L"HSHELL_WINDOWCREATED"},
                {HSHELL_WINDOWDESTROYED, L"HSHELL_WINDOWDESTROYED"},
                {HSHELL_ACTIVATESHELLWINDOW, L"HSHELL_ACTIVATESHELLWINDOW"},
                {HSHELL_WINDOWACTIVATED, L"HSHELL_WINDOWACTIVATED"},
                {HSHELL_GETMINRECT, L"HSHELL_GETMINRECT"},
                {HSHELL_REDRAW, L"HSHELL_REDRAW"},
                {HSHELL_TASKMAN, L"HSHELL_TASKMAN"},
                {HSHELL_LANGUAGE, L"HSHELL_LANGUAGE"},
                {HSHELL_SYSMENU, L"HSHELL_SYSMENU"},
                {HSHELL_ENDTASK, L"HSHELL_ENDTASK"},
                {HSHELL_ACCESSIBILITYSTATE, L"HSHELL_ACCESSIBILITYSTATE"},
                {HSHELL_APPCOMMAND, L"HSHELL_APPCOMMAND"},
                {HSHELL_WINDOWREPLACED, L"HSHELL_WINDOWREPLACED"},
                {HSHELL_WINDOWREPLACING, L"HSHELL_WINDOWREPLACING"},
                {HSHELL_RUDEAPPACTIVATED, L"HSHELL_RUDEAPPACTIVATED"},
            };
            int i, found;
UnhandledShellMessage:
            for (i = 0, found = 0; i != sizeof(hshell_msg) / sizeof(hshell_msg[0]); i++)
            {
                if (hshell_msg[i].msg == (INT)wParam)
                {
                    DbgPrint("Shell message %ws unhandled (lParam = 0x%p)!\n", hshell_msg[i].msg_name, lParam);
                    found = 1;
                    break;
                }
            }
            if (!found)
            {
                DbgPrint("Shell message %d unhandled (lParam = 0x%p)!\n", (INT)wParam, lParam);
            }
            break;
        }
    }

    return Ret;
}

static VOID
TaskSwitchWnd_EnableGrouping(IN OUT PTASK_SWITCH_WND This,
                             IN BOOL bEnable)
{
    This->IsGroupingEnabled = bEnable;

    /* Collapse or expand groups if neccessary */
    TaskSwitchWnd_UpdateButtonsSize(This,
                                    FALSE);
}

static VOID
TaskSwitchWnd_HandleTaskItemClick(IN OUT PTASK_SWITCH_WND This,
                                  IN OUT PTASK_ITEM TaskItem)
{
    BOOL bMinimize;
    
    if (IsWindow(TaskItem->hWnd))
    {
        bMinimize = !IsIconic(TaskItem->hWnd) &&
                    TaskItem == This->ActiveTaskItem;
        
        if (!bMinimize && IsIconic(TaskItem->hWnd))
        {
             PostMessage(TaskItem->hWnd,
                         WM_SYSCOMMAND,
                         SC_RESTORE,
                         0);
        }

        SetForegroundWindow(TaskItem->hWnd);
        
        if (bMinimize)
        {
            PostMessage(TaskItem->hWnd,
                        WM_SYSCOMMAND,
                        SC_MINIMIZE,
                        0);
        }
    }
}

static VOID
TaskSwitchWnd_HandleTaskGroupClick(IN OUT PTASK_SWITCH_WND This,
                                   IN OUT PTASK_GROUP TaskGroup)
{
    /* TODO: Show task group menu */
}

static BOOL
TaskSwitchWnd_HandleButtonClick(IN OUT PTASK_SWITCH_WND This,
                                IN WORD wIndex)
{
    PTASK_ITEM TaskItem;
    PTASK_GROUP TaskGroup;
    
    if (This->IsGroupingEnabled)
    {
        TaskGroup = FindTaskGroupByIndex(This,
                                         (INT)wIndex);
        if (TaskGroup != NULL && TaskGroup->IsCollapsed)
        {
            TaskSwitchWnd_HandleTaskGroupClick(This,
                                               TaskGroup);
            return TRUE;
        }
    }
    
    TaskItem = FindTaskItemByIndex(This,
                                   (INT)wIndex);
    if (TaskItem != NULL)
    {
        TaskSwitchWnd_HandleTaskItemClick(This,
                                          TaskItem);
        return TRUE;
    }
    
    return FALSE;
}

static LRESULT
TaskSwichWnd_HandleItemPaint(IN OUT PTASK_SWITCH_WND This,
                             IN OUT NMTBCUSTOMDRAW *nmtbcd)
{
    HFONT hCaptionFont, hBoldCaptionFont;
    LRESULT Ret = CDRF_DODEFAULT;
    PTASK_GROUP TaskGroup;
    PTASK_ITEM TaskItem;

#if TASK_USE_DRAWCAPTIONTEMP != 0

    UINT uidctFlags = DC_TEXT | DC_ICON | DC_NOSENDMSG;

#endif
    TaskItem = FindTaskItemByIndex(This,
                                   (INT)nmtbcd->nmcd.dwItemSpec);
    TaskGroup = FindTaskGroupByIndex(This,
                                     (INT)nmtbcd->nmcd.dwItemSpec);
    if (TaskGroup == NULL && TaskItem != NULL)
    {
        ASSERT(TaskItem != NULL);

        if (TaskItem != NULL && IsWindow(TaskItem->hWnd))
        {
            hCaptionFont = ITrayWindow_GetCaptionFonts(This->Tray,
                                                       &hBoldCaptionFont);
            if (nmtbcd->nmcd.uItemState & CDIS_CHECKED)
                hCaptionFont = hBoldCaptionFont;

#if TASK_USE_DRAWCAPTIONTEMP != 0

            /* Make sure we don't draw on the button edges */
            InflateRect(&nmtbcd->nmcd.rc,
                        -GetSystemMetrics(SM_CXEDGE),
                        -GetSystemMetrics(SM_CYEDGE));

            if ((nmtbcd->nmcd.uItemState & CDIS_MARKED) && TaskItem->RenderFlashed)
            {
                /* This is a slight glitch. We have to move the rectangle so that
                   the button content appears to be pressed. However, when flashing
                   is enabled, we can see a light line at the top and left inner
                   border. We need to fill that area with the flashing color. Note
                   that since we're using DrawCaptionTemp() the flashing color is
                   COLOR_ACTIVECAPTION, not COLOR_HIGHLIGHT! */
                FillRect(nmtbcd->nmcd.hdc,
                         &nmtbcd->nmcd.rc,
                         (HBRUSH)(COLOR_ACTIVECAPTION + 1));

                /* Make the button content appear pressed. This however draws a bit
                   into the right and bottom border of the button edge, making it
                   look a bit odd. However, selecting a clipping region to prevent
                   that from happening causes problems with DrawCaptionTemp()! */
                OffsetRect(&nmtbcd->nmcd.rc,
                           1,
                           1);

                /* Render flashed */
                uidctFlags |= DC_ACTIVE;
            }
            else
            {
                uidctFlags |= DC_INBUTTON;
                if (nmtbcd->nmcd.uItemState & CDIS_CHECKED)
                    uidctFlags |= DC_ACTIVE;
            }

            if (DrawCapTemp != NULL)
            {
                /* Draw the button content */
                TaskItem->DisplayTooltip = !DrawCapTemp(TaskItem->hWnd,
                                                        nmtbcd->nmcd.hdc,
                                                        &nmtbcd->nmcd.rc,
                                                        hCaptionFont,
                                                        NULL,
                                                        NULL,
                                                        uidctFlags);
            }

            return CDRF_SKIPDEFAULT;

#else /* !TASK_USE_DRAWCAPTIONTEMP */

            /* Make the entire button flashing if neccessary */
            if (nmtbcd->nmcd.uItemState & CDIS_MARKED)
            {
                if (TaskItem->RenderFlashed)
                {
                    nmtbcd->hbrMonoDither = GetSysColorBrush(COLOR_HIGHLIGHT);
                    nmtbcd->clrTextHighlight = GetSysColor(COLOR_HIGHLIGHTTEXT);
                    nmtbcd->nHLStringBkMode = TRANSPARENT;

                    /* We don't really need to set clrMark because we set the
                       background mode to TRANSPARENT! */
                    nmtbcd->clrMark = GetSysColor(COLOR_HIGHLIGHT);

                    Ret |= TBCDRF_USECDCOLORS;
                }
                else
                    Ret |= TBCDRF_NOMARK;
            }

            /* Select the font we want to use */
            SelectObject(nmtbcd->nmcd.hdc,
                         hCaptionFont);
            return Ret | CDRF_NEWFONT;

#endif

        }
    }
    else if (TaskGroup != NULL)
    {
        /* FIXME: Implement painting for task groups */
    }

    return Ret;
}

static LRESULT
TaskSwitchWnd_HandleToolbarNotification(IN OUT PTASK_SWITCH_WND This,
                                        IN const NMHDR *nmh)
{
    LRESULT Ret = 0;

    switch (nmh->code)
    {
        case NM_CUSTOMDRAW:
        {
            LPNMTBCUSTOMDRAW nmtbcd = (LPNMTBCUSTOMDRAW)nmh;

            switch (nmtbcd->nmcd.dwDrawStage)
            {

#if TASK_USE_DRAWCAPTIONTEMP != 0

                case CDDS_ITEMPREPAINT:
                    /* We handle drawing in the post-paint stage so that we
                       don't have to draw the button edges, etc */
                    Ret = CDRF_NOTIFYPOSTPAINT;
                    break;

                case CDDS_ITEMPOSTPAINT:

#else /* !TASK_USE_DRAWCAPTIONTEMP */

                case CDDS_ITEMPREPAINT:

#endif

                    Ret = TaskSwichWnd_HandleItemPaint(This,
                                                       nmtbcd);
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

static LRESULT CALLBACK
TaskSwitchWndProc(IN HWND hwnd,
                  IN UINT uMsg,
                  IN WPARAM wParam,
                  IN LPARAM lParam)
{
    PTASK_SWITCH_WND This = NULL;
    LRESULT Ret = FALSE;

    if (uMsg != WM_NCCREATE)
    {
        This = (PTASK_SWITCH_WND)GetWindowLongPtr(hwnd,
                                                  0);
    }

    if (This != NULL || uMsg == WM_NCCREATE)
    {
        switch (uMsg)
        {
            case WM_SIZE:
            {
                SIZE szClient;

                szClient.cx = LOWORD(lParam);
                szClient.cy = HIWORD(lParam);
                if (This->hWndToolbar != NULL)
                {
                    SetWindowPos(This->hWndToolbar,
                                 NULL,
                                 0,
                                 0,
                                 szClient.cx,
                                 szClient.cy,
                                 SWP_NOZORDER);

                    TaskSwitchWnd_UpdateButtonsSize(This,
                                                    FALSE);
                }
                break;
            }

            case WM_NCHITTEST:
            {
                /* We want the tray window to be draggable everywhere, so make the control
                   appear transparent */
                Ret = DefWindowProc(hwnd,
                                    uMsg,
                                    wParam,
                                    lParam);
                if (Ret != HTVSCROLL && Ret != HTHSCROLL)
                    Ret = HTTRANSPARENT;
                break;
            }

            case WM_COMMAND:
            {
                if (lParam != 0 && (HWND)lParam == This->hWndToolbar)
                {
                    TaskSwitchWnd_HandleButtonClick(This,
                                                    LOWORD(wParam));
                }
                break;
            }

            case WM_NOTIFY:
            {
                const NMHDR *nmh = (const NMHDR *)lParam;

                if (nmh->hwndFrom == This->hWndToolbar)
                {
                    Ret = TaskSwitchWnd_HandleToolbarNotification(This,
                                                                  nmh);
                }
                break;
            }

            case TSWM_ENABLEGROUPING:
            {
                Ret = This->IsGroupingEnabled;
                if (wParam != This->IsGroupingEnabled)
                {
                    TaskSwitchWnd_EnableGrouping(This,
                                                 (BOOL)wParam);
                }
                break;
            }

            case TSWM_UPDATETASKBARPOS:
            {
                /* Update the button spacing */
                TaskSwitchWnd_UpdateTbButtonSpacing(This,
                                                    ITrayWindow_IsHorizontal(This->Tray),
                                                    0,
                                                    0);
                break;
            }

            case WM_CONTEXTMENU:
            {
                if (This->hWndToolbar != NULL)
                {
                    POINT pt;
                    INT_PTR iBtn;

                    pt.x = (LONG)LOWORD(lParam);
                    pt.y = (LONG)HIWORD(lParam);

                    MapWindowPoints(NULL,
                                    This->hWndToolbar,
                                    &pt,
                                    1);

                    iBtn = (INT_PTR)SendMessage(This->hWndToolbar,
                                                TB_HITTEST,
                                                0,
                                                (LPARAM)&pt);
                    if (iBtn >= 0)
                    {
                        /* FIXME: Display the system menu of the window */
                    }
                    else
                        goto ForwardContextMenuMsg;
                }
                else
                {
ForwardContextMenuMsg:
                    /* Forward message */
                    Ret = SendMessage(ITrayWindow_GetHWND(This->Tray),
                                      uMsg,
                                      wParam,
                                      lParam);
                }
                break;
            }

            case WM_NCCREATE:
            {
                LPCREATESTRUCT CreateStruct = (LPCREATESTRUCT)lParam;
                This = (PTASK_SWITCH_WND)HeapAlloc(hProcessHeap,
                                                   0,
                                                   sizeof(*This));
                if (This == NULL)
                    return FALSE;

                ZeroMemory(This,
                           sizeof(*This));
                This->hWnd = hwnd;
                This->hWndNotify = CreateStruct->hwndParent;
                This->Tray = (ITrayWindow*)CreateStruct->lpCreateParams;
                This->IsGroupingEnabled = TRUE; /* FIXME */
                SetWindowLongPtr(hwnd,
                                 0,
                                 (LONG_PTR)This);

                return TRUE;
            }

            case WM_CREATE:
                TaskSwitchWnd_Create(This);

#if DUMP_TASKS != 0
                SetTimer(hwnd,
                         1,
                         5000,
                         NULL);
#endif

                break;

            case WM_DESTROY:
                if (This->IsToolbarSubclassed)
                {
                    if (RemoveWindowSubclass(This->hWndToolbar,
                                             TaskSwichWnd_ToolbarSubclassedProc,
                                             TSW_TOOLBAR_SUBCLASS_ID))
                    {
                        This->IsToolbarSubclassed = FALSE;
                    }
                }
                break;

            case WM_NCDESTROY:
                TaskSwitchWnd_NCDestroy(This);
                HeapFree(hProcessHeap,
                         0,
                         This);
                SetWindowLongPtr(hwnd,
                                 0,
                                 0);
                break;

#if DUMP_TASKS != 0
            case WM_TIMER:
                switch(wParam)
                {
                    case 1:
                        TaskSwitchWnd_DumpTasks(This);
                        break;
                }
                break;
#endif

            default:
/* HandleDefaultMessage: */
                if (uMsg == This->ShellHookMsg && This->ShellHookMsg != 0)
                {
                    /* Process shell messages */
                    Ret = (LRESULT)TaskSwitchWnd_HandleShellHookMsg(This,
                                                                    wParam,
                                                                    lParam);
                    break;
                }

                Ret = DefWindowProc(hwnd,
                                    uMsg,
                                    wParam,
                                    lParam);
                break;
        }
    }
    else
    {
        Ret = DefWindowProc(hwnd,
                            uMsg,
                            wParam,
                            lParam);
    }

    return Ret;
}


HWND
CreateTaskSwitchWnd(IN HWND hWndParent,
                    IN OUT ITrayWindow *Tray)
{
    HWND hwndTaskBar;

    hwndTaskBar = CreateWindowEx(0,
                                 szTaskSwitchWndClass,
                                 szRunningApps,
                                 WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP,
                                 0,
                                 0,
                                 0,
                                 0,
                                 hWndParent,
                                 NULL,
                                 hExplorerInstance,
                                 (LPVOID)Tray);

    return hwndTaskBar;
}

BOOL
RegisterTaskSwitchWndClass(VOID)
{
    WNDCLASS wc;

    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = TaskSwitchWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(PTASK_SWITCH_WND);
    wc.hInstance = hExplorerInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL,
                            IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szTaskSwitchWndClass;

    return RegisterClass(&wc) != 0;
}

VOID
UnregisterTaskSwitchWndClass(VOID)
{
    UnregisterClass(szTaskSwitchWndClass,
                    hExplorerInstance);
}
