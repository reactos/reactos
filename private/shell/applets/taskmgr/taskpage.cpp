//+-------------------------------------------------------------------------
//
//  TaskMan - NT TaskManager
//  Copyright (C) Microsoft
//
//  File:       taskpage.cpp
//
//  History:    Nov-29-95   DavePl  Created
//
//--------------------------------------------------------------------------

#include "precomp.h"

//
// Project-scope globals
//

DWORD       g_cTasks        = 0;

//
// Local file prototypes
//

BOOL CALLBACK EnumWindowStationsFunc(LPTSTR  lpstr, LPARAM lParam);
BOOL CALLBACK EnumDesktopsFunc(LPTSTR  lpstr, LPARAM lParam);
BOOL CALLBACK EnumWindowsProc(HWND    hwnd, LPARAM   lParam);

//
// Column ID enumeration
//

typedef enum TASKCOLUMNID
{
    COL_TASKNAME            = 0,
    COL_TASKSTATUS          = 1,
    COL_TASKWINSTATION      = 2,
    COL_TASKDESKTOP         = 3,
};

#define MAX_TASK_COLUMN      3
#define NUM_TASK_COLUMN      (MAX_TASK_COLUMN + 1)

#define IDS_FIRSTTASKCOL    21000       // 21000 is first column name ID in rc file

//
// Column ID on which to sort in the listview, and for
// compares in general
//

TASKCOLUMNID g_iTaskSortColumnID  = COL_TASKNAME;
INT          g_iTaskSortDirection = 1;          // 1 = asc, -1 = desc

//
// Column Default Info
//

struct 
{
    INT Format;
    INT Width;
} TaskColumnDefaults[NUM_TASK_COLUMN] =
{
    { LVCFMT_LEFT,       250},       // COL_TASKNAME
    { LVCFMT_LEFT,       97 },      // COL_TASKSTATUS       
    { LVCFMT_LEFT,       70 },       // COL_TASKWINSTATION
    { LVCFMT_LEFT,       70 },       // COL_TASKDESKTOP   
};


//
// Active Columns
//

TASKCOLUMNID g_ActiveTaskCol[NUM_TASK_COLUMN + 1] =
{
    COL_TASKNAME,     
//  COL_TASKDESKTOP,
    COL_TASKSTATUS,

    (TASKCOLUMNID) -1
};

/*++ class CTaskInfo

Class Description:

    Represents the last known information about a running task

Arguments:

Return Value:

Revision History:

      Nov-29-95 Davepl  Created

--*/

class CTaskInfo
{
public:

    HWND            m_hwnd;
    LPTSTR          m_pszWindowTitle;
    LPTSTR          m_lpWinsta;
    LPTSTR          m_lpDesktop;
    BOOL            m_fHung;
    LARGE_INTEGER   m_uPassCount;
    INT             m_iSmallIcon;
    HICON           m_hSmallIcon;
    INT             m_iLargeIcon;
    HICON           m_hLargeIcon;

    //
    // This is a union of which attribute is dirty.  You can look at
    // or set any particular column's bit, or just inspect m_fDirty
    // to see if anyone at all is dirty.  Used to optimize listview
    // painting
    //

    union
    {
        DWORD                m_fDirty;
        struct 
        {
            DWORD            m_fDirty_COL_HWND           :1;
            DWORD            m_fDirty_COL_TITLE          :1;
            DWORD            m_fDirty_COL_STATUS         :1;
            DWORD            m_fDirty_COL_WINSTA         :1;
            DWORD            m_fDirty_COL_DESKTOP        :1;
        };                                                
    };

    HRESULT        SetData(HWND                         hwnd,
                           LPTSTR                       lpTitle,
                           LPTSTR                       lpWinsta,
                           LPTSTR                       lpDesktop,
                           LARGE_INTEGER                uPassCount,
                           BOOL                         fUpdateOnly);

    CTaskInfo()
    {
        ZeroMemory(this, sizeof(*this));
    }

    ~CTaskInfo()
    {
        if (m_pszWindowTitle)
        {
            LocalFree(m_pszWindowTitle);
        }

        if (m_lpWinsta)
        {
            LocalFree(m_lpWinsta);
        }

        if (m_lpDesktop)
        {
            LocalFree(m_lpDesktop);
        }
    }

    INT Compare(CTaskInfo * pOther);

};

/*++ class CTaskInfo::Compare

Class Description:

    Compares this CTaskInfo object to another, and returns its ranking
    based on the g_iTaskSortColumnID field.

    Note that if the objects are equal based on the current sort column,
    the HWND is used as a secondary sort key to prevent items from 
    jumping around in the listview

Arguments:

    pOther  - the CTaskInfo object to compare this to

Return Value:

    < 0      - This CTaskInfo is "less" than the other
      0      - Equal (Can't happen, since HWND is secondary sort)
    > 0      - This CTaskInfo is "greater" than the other

Revision History:

      Nov-29-95 Davepl  Created

--*/

INT CTaskInfo::Compare(CTaskInfo * pOther)
{
    INT iRet;

    switch (g_iTaskSortColumnID)
    {
        case COL_TASKNAME:
            iRet = lstrcmpi(this->m_pszWindowTitle, pOther->m_pszWindowTitle);
            break;

        case COL_TASKWINSTATION:
            iRet = lstrcmpi(this->m_lpWinsta, pOther->m_lpWinsta);
            break;

        case COL_TASKDESKTOP:
            iRet = lstrcmpi(this->m_lpDesktop, pOther->m_lpDesktop);
            break;

        case COL_TASKSTATUS:
            iRet = Compare64(this->m_fHung, pOther->m_fHung);
            break;

        default:
            
            Assert(0 && "Invalid task sort column");
            iRet = 0;
    }

    // If objects look equal, compare on HWND as secondary sort column
    // so that items don't jump around in the listview

    if (0 == iRet)
    {
        iRet = Compare64((LPARAM)this->m_hwnd, (LPARAM)pOther->m_hwnd);
    }

    return (iRet * g_iTaskSortDirection);
}

// REVIEW (Davepl) The next three functions have very close parallels
// in the process page code.  Consider generalizing them to eliminate
// duplication

/*++ InsertIntoSortedArray

Class Description:

    Sticks a CTaskInfo ptr into the ptrarray supplied at the
    appropriate location based on the current sort column (which
    is used by the Compare member function)

Arguments:

    pArray      - The CPtrArray to add to
    pProc       - The CTaskInfo object to add to the array

Return Value:

    TRUE if successful, FALSE if fails

Revision History:

      Nov-20-95 Davepl  Created

--*/

// REVIEW (davepl) Use binary insert here, not linear

BOOL InsertIntoSortedArray(CPtrArray * pArray, CTaskInfo * pTask)
{
    
    INT cItems = pArray->GetSize();
    
    for (INT iIndex = 0; iIndex < cItems; iIndex++)
    {
        CTaskInfo * pTmp = (CTaskInfo *) pArray->GetAt(iIndex);
        
        if (pTask->Compare(pTmp) < 0)
        {
            return pArray->InsertAt(iIndex, pTask);
        }
    }

    return pArray->Add(pTask);
}

/*++ ResortTaskArray

Function Description:

    Creates a new ptr array sorted in the current sort order based
    on the old array, and then replaces the old with the new

Arguments:

    ppArray     - The CPtrArray to resort

Return Value:

    TRUE if successful, FALSE if fails

Revision History:

      Nov-21-95 Davepl  Created

--*/

BOOL ResortTaskArray(CPtrArray ** ppArray)
{
    // Create a new array which will be sorted in the new 
    // order and used to replace the existing array

    CPtrArray * pNew = new CPtrArray(GetProcessHeap());
    if (NULL == pNew)
    {
        return FALSE;
    }

    // Insert each of the existing items in the old array into
    // the new array in the correct spot

    INT cItems = (*ppArray)->GetSize();
    for (int i = 0; i < cItems; i++)
    {
        CTaskInfo * pItem = (CTaskInfo *) (*ppArray)->GetAt(i);
        if (FALSE == InsertIntoSortedArray(pNew, pItem))
        {
            delete pNew;
            return FALSE;
        }
    }

    // Kill off the old array, replace it with the new

    delete (*ppArray);
    (*ppArray) = pNew;
    return TRUE;
}

/*++ CTaskPage::~CTaskPage()

     Destructor

*/

CTaskPage::~CTaskPage()
{
    RemoveAllTasks();
    delete m_pTaskArray;
}

void CTaskPage::RemoveAllTasks()
{
    if (m_pTaskArray)
    {
        INT c = m_pTaskArray->GetSize();

        while (c)
        {
            delete (CTaskInfo *) (m_pTaskArray->GetAt(c - 1));
            c--;
        }
    }
}

/*++ CTaskPage::UpdateTaskListview

Class Description:

    Walks the listview and checks to see if each line in the
    listview matches the corresponding entry in our process
    array.  Those which differe by HWND are replaced, and those
    that need updating are updated.

    Items are also added and removed to/from the tail of the
    listview as required.
    
Arguments:

Return Value:

    HRESULT

Revision History:

      Nov-29-95 Davepl  Created

--*/

HRESULT CTaskPage::UpdateTaskListview()
{
    HWND hListView = GetDlgItem(m_hPage, IDC_TASKLIST);

    // Stop repaints while we party on the listview

    SendMessage(hListView, WM_SETREDRAW, FALSE, 0);

     // If the view mode has changed, update it now

    if (m_vmViewMode != g_Options.m_vmViewMode)
    {
        m_vmViewMode = g_Options.m_vmViewMode;

        DWORD dwStyle = GetWindowLong(hListView, GWL_STYLE);
        dwStyle &= ~(LVS_TYPEMASK);
        
        if (g_Options.m_vmViewMode == VM_SMALLICON)
        {
            ListView_SetImageList(hListView, m_himlSmall, LVSIL_SMALL);
            dwStyle |= LVS_SMALLICON | LVS_AUTOARRANGE;
        }
        else if (g_Options.m_vmViewMode == VM_DETAILS)
        {
            ListView_SetImageList(hListView, m_himlSmall, LVSIL_SMALL);
            dwStyle |= LVS_REPORT;
        }
        else 
        {
            Assert(g_Options.m_vmViewMode == VM_LARGEICON);
            ListView_SetImageList(hListView, m_himlLarge, LVSIL_NORMAL);
            dwStyle |= LVS_ICON | LVS_AUTOARRANGE;
        }

        ListView_DeleteAllItems(hListView);
        SetWindowLong(hListView, GWL_STYLE, dwStyle);
    }

    INT cListViewItems = ListView_GetItemCount(hListView);
    INT CTaskArrayItems = m_pTaskArray->GetSize();
    
    //
    // Walk the existing lines in the listview and replace/update
    // them as needed
    //


    for (INT iCurrent = 0; 
         iCurrent < cListViewItems && iCurrent < CTaskArrayItems; 
         iCurrent++)
    {
        LV_ITEM lvitem = { 0 };
        lvitem.mask = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE;
        lvitem.iItem = iCurrent;

        if (FALSE == ListView_GetItem(hListView, &lvitem))
        {
            SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
            return E_FAIL;
        }

        CTaskInfo * pTmp = (CTaskInfo *) lvitem.lParam;
        CTaskInfo * pTask = (CTaskInfo *) m_pTaskArray->GetAt(iCurrent);        
        
        if (pTmp != pTask || pTask->m_fDirty)
        {
            // If the objects aren't the same, we need to replace this line

            lvitem.pszText = pTask->m_pszWindowTitle;
            lvitem.lParam  = (LPARAM) pTask;
            
            if (g_Options.m_vmViewMode == VM_LARGEICON)
            {
                lvitem.iImage  = pTask->m_iLargeIcon;
            }
            else
            {
                lvitem.iImage  = pTask->m_iSmallIcon;
            }
            
            ListView_SetItem(hListView, &lvitem);
            ListView_RedrawItems(hListView, iCurrent, iCurrent);
            pTask->m_fDirty = 0;
        }
    }

    // 
    // We've either run out of listview items or run out of Task array
    // entries, so remove/add to the listview as appropriate
    //

    while (iCurrent < cListViewItems)
    {
        // Extra items in the listview (processes gone away), so remove them

        ListView_DeleteItem(hListView, iCurrent);
        cListViewItems--;
    }

    while (iCurrent < CTaskArrayItems)
    {
        // Need to add new items to the listview (new tasks appeared)

        CTaskInfo * pTask = (CTaskInfo *)m_pTaskArray->GetAt(iCurrent);
        LV_ITEM lvitem  = { 0 };
        lvitem.mask     = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE;
        lvitem.iItem    = iCurrent;
        lvitem.pszText  = pTask->m_pszWindowTitle;
        lvitem.lParam   = (LPARAM) pTask;
        lvitem.iImage   = pTask->m_iLargeIcon;

        // The first item added (actually, every 0 to 1 count transition) gets
        // selected and focused

        if (iCurrent == 0)
        {
            lvitem.state = LVIS_SELECTED | LVIS_FOCUSED;
            lvitem.stateMask = lvitem.state;
            lvitem.mask |= LVIF_STATE;
        }
                
        ListView_InsertItem(hListView, &lvitem);
        pTask->m_fDirty = 0;
        iCurrent++;        
    }    

    // Let the listview paint again

    SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
    return S_OK;
}


/*++ CTasKPage::EnsureWindowsNotMinimized

Routine Description:

    Walks an array of HWNDS and ensure the windows are not
    minimized, which would prevent them from being 
    cascaded to tiles properly
    
Arguments:

    aHwnds - Array of window handles
    dwCount- Number of HWNDS in table

Return Value:

Revision History:

      Dec-06-95 Davepl  Created

--*/

void CTaskPage::EnsureWindowsNotMinimized(HWND aHwnds[], DWORD dwCount)
{
    for (UINT i = 0; i < dwCount; i++)
    {
        if (IsIconic(aHwnds[i]))
        {
            ShowWindow(aHwnds[i], SW_RESTORE);
        }
    }
}

/*++ CTaslPage::GetSelectedHWNDS

Routine Description:

    Returns a dynamically allocated array of HWNDS based on the
    ones selected in the task list
    
Arguments:

    pdwCount- Number of HWNDS in t`able

Return Value:

    HWND[], or NULL on failure

Revision History:

      Dec-05-95 Davepl  Created

--*/

HWND * CTaskPage::GetHWNDS(BOOL fSelectedOnly, DWORD * pdwCount)
{
    CPtrArray * pArray = NULL;

    if (fSelectedOnly)
    {
        // If we only want selected tasks, go and get the array
        // of selected listview tasks

        pArray = GetSelectedTasks();
        if (NULL == pArray)
        {
            return NULL;
        }
    }
    else
    {
        // If we want everything, just make a copy of the TaskArray

        pArray = new CPtrArray(GetProcessHeap());
        if (FALSE == pArray->Copy(*m_pTaskArray))
        {
            delete pArray;
            *pdwCount = 0;
            return FALSE;
        }
    }

    //
    // No windows to speak of, so bail
    //

    *pdwCount = pArray->GetSize();
    if (*pdwCount == 0)
    {
        delete pArray;
        return NULL;
    }

    HWND * pHwnds = (HWND *) LocalAlloc(0, *pdwCount * sizeof(HWND));

    if (NULL == pHwnds)
    {
        *pdwCount = 0;
    }
    else
    {
        for (UINT i = 0; i < *pdwCount; i++)
        {
            pHwnds[i] = (((CTaskInfo *) (pArray->GetAt(i)) )->m_hwnd);
        }
    }

    delete pArray;

    return pHwnds;
}


/*++ CTaskPage::GetSelectedTasks

Routine Description:

    Returns a CPtrArray of the selected tasks
    
Arguments:

Return Value:

    CPtrArray on success, NULL on failure

Revision History:

      Dec-01-95 Davepl  Created

--*/

CPtrArray * CTaskPage::GetSelectedTasks()
{
    BOOL fSuccess = TRUE;

    //
    // Get the count of selected items
    //

    HWND hTaskList = GetDlgItem(m_hPage, IDC_TASKLIST);
    INT cItems = ListView_GetSelectedCount(hTaskList);
    if (0 == cItems)
    {
        return NULL;
    }

    //
    // Create a CPtrArray to hold the task items
    //

    CPtrArray * pArray = new CPtrArray(GetProcessHeap());
    if (NULL == pArray)
    {
        return NULL;
    }

    INT iLast = -1;
    for (INT i = 0; i < cItems; i++)
    {
        //
        // Get the Nth selected item
        // 

        INT iItem = ListView_GetNextItem(hTaskList, iLast, LVNI_SELECTED);

        if (-1 == iItem)
        {
            fSuccess = FALSE;
            break;
        }

        iLast = iItem;

        //
        // Pull the item from the listview and add it to the selected array
        //

        LV_ITEM lvitem = { LVIF_PARAM };
        lvitem.iItem = iItem;
    
        if (ListView_GetItem(hTaskList, &lvitem))
        {
            LPVOID pTask = (LPVOID) (lvitem.lParam);
            if (FALSE == pArray->Add(pTask))
            {
                fSuccess = FALSE;
                break;
            }
        }
        else
        {
            fSuccess = FALSE;
            break;
        }
    }

    //
    // Any errors, clean up the array and bail.  We don't release the
    // tasks in the array, since they are owned by the listview.
    //

    if (FALSE == fSuccess && NULL != pArray)
    {
        delete pArray;
        return NULL;
    }

    return pArray;
}

/*++ CProcPage::HandleTaskListContextMenu

Routine Description:

    Handles right-clicks (context menu) in the task list
    
Arguments:

    xPos, yPos  - coords of where the click occurred

Return Value:

Revision History:

      Dec-01-95 Davepl  Created

--*/

void CTaskPage::HandleTaskListContextMenu(INT xPos, INT yPos)
{
    HWND hTaskList = GetDlgItem(m_hPage, IDC_TASKLIST);

    CPtrArray * pArray = GetSelectedTasks();

    if (pArray)
    {
        // If non-mouse-based context menu, use the currently selected
        // item as the coords

        if (0xFFFF == LOWORD(xPos) && 0xFFFF == LOWORD(yPos))
        {
            int iSel = ListView_GetNextItem(hTaskList, -1, LVNI_SELECTED);
            RECT rcItem;
            ListView_GetItemRect(hTaskList, iSel, &rcItem, LVIR_ICON);
            MapWindowRect(hTaskList, NULL, &rcItem);
            xPos = rcItem.right;
            yPos = rcItem.bottom;
        }

        HMENU hPopup = LoadPopupMenu(g_hInstance, IDR_TASK_CONTEXT);

        if (hPopup)
        {
            SetMenuDefaultItem(hPopup, IDM_TASK_SWITCHTO, FALSE);

            //
            // If single-selection, disable the items that require multiple
            // selections to make sense
            //

            if (pArray->GetSize() < 2)
            {
                EnableMenuItem(hPopup, IDM_TASK_CASCADE, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                EnableMenuItem(hPopup, IDM_TASK_TILEHORZ, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                EnableMenuItem(hPopup, IDM_TASK_TILEVERT, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
            }

            EnableMenuItem(hPopup, IDM_TASK_BRINGTOFRONT, MF_BYCOMMAND | ((pArray->GetSize() == 1) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));


            Pause();
            g_fInPopup = TRUE;
            TrackPopupMenuEx(hPopup, 0, xPos, yPos, m_hPage, NULL);
            g_fInPopup = FALSE;

            // Note that we don't "unpause" until one of the menu commands (incl CANCEL) is
            // selected or the menu is dismissed
        
            DestroyMenu(hPopup);
        }

        delete pArray;
    }
    else
    {
        HMENU hPopup = LoadPopupMenu(g_hInstance, IDR_TASKVIEW);

        if (hPopup && SHRestricted(REST_NORUN))
        {
            DeleteMenu(hPopup, IDM_RUN, MF_BYCOMMAND);
        }

        UINT id;
        if (m_vmViewMode == VM_LARGEICON)
        {
            id = IDM_LARGEICONS;
        } 
        else if (m_vmViewMode == VM_SMALLICON)
        {
            id = IDM_SMALLICONS;
        }
        else
        {
            Assert(m_vmViewMode == VM_DETAILS);
            id = IDM_DETAILS;
        }

        if (hPopup)
        {
            CheckMenuRadioItem(hPopup, IDM_LARGEICONS, IDM_DETAILS, id, MF_BYCOMMAND);
            g_fInPopup = TRUE;
            TrackPopupMenuEx(hPopup, 0, xPos, yPos, m_hPage, NULL);
            g_fInPopup = FALSE;
            DestroyMenu(hPopup);
        }
    }
}

/*++ CTaskPage::UpdateUIState

Routine Description:

    Updates the enabled/disabled states, etc., of the task UI
    
Arguments:

Return Value:

Revision History:

      Dec-04-95 Davepl  Created

--*/

// Controls which are enabled only for any selection

static const UINT g_aSingleIDs[] =
{
    IDC_ENDTASK,
    IDC_SWITCHTO,

};

void CTaskPage::UpdateUIState()
{
    INT i;
    
    // Set the state for controls which require a selection (1 or more items)

    for (i = 0; i < ARRAYSIZE(g_aSingleIDs); i++)
    {
        EnableWindow(GetDlgItem(m_hPage, g_aSingleIDs[i]), m_cSelected > 0);
    }    

    if (g_Options.m_iCurrentPage == 0)
    {
        CPtrArray * pArray = GetSelectedTasks();

        if (pArray)
        {
            UINT state;
            if (pArray->GetSize() == 1)
            {
                state = MF_GRAYED | MF_DISABLED;
            }
            else
            {
                state = MF_ENABLED;
            }

            HMENU hMain  = GetMenu(g_hMainWnd);

            EnableMenuItem(hMain , IDM_TASK_CASCADE, state | MF_BYCOMMAND);
            EnableMenuItem(hMain , IDM_TASK_TILEHORZ, state | MF_BYCOMMAND);
            EnableMenuItem(hMain , IDM_TASK_TILEVERT, state | MF_BYCOMMAND);

            EnableMenuItem(hMain, IDM_TASK_BRINGTOFRONT, MF_BYCOMMAND | ((pArray->GetSize() == 1) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
            delete pArray;
        }
    }
}

/*++ CTaskPage::HandleTaskPageNotify

Routine Description:

    Processes WM_NOTIFY messages received by the taskpage dialog
    
Arguments:

    hWnd    - Control that generated the WM_NOTIFY
    pnmhdr  - Ptr to the NMHDR notification stucture

Return Value:

    BOOL "did we handle it" code

Revision History:

      Nov-29-95 Davepl  Created

--*/

INT CTaskPage::HandleTaskPageNotify(HWND hWnd, LPNMHDR pnmhdr)
{
    switch(pnmhdr->code)
    {
        case NM_DBLCLK:
        {
            SendMessage(m_hPage, WM_COMMAND, IDC_SWITCHTO, 0);
            break;    
        }

        // If the (selection) state of an item is changing, see if
        // the count has changed, and if so, update the UI

        case LVN_ITEMCHANGED:
        {
            const NM_LISTVIEW * pnmv = (const NM_LISTVIEW *) pnmhdr;
            if (pnmv->uChanged & LVIF_STATE)
            {
                UINT cSelected = ListView_GetSelectedCount(GetDlgItem(m_hPage, IDC_TASKLIST));
                if (cSelected != m_cSelected)
                {
                    m_cSelected = cSelected;
                    UpdateUIState();
                }
            }
            break;
        }

        case LVN_COLUMNCLICK:
        {
            // User clicked a header control, so set the sort column.  If its the
            // same as the current sort column, just invert the sort direction in
            // the column.  Then resort the task array

            const NM_LISTVIEW * pnmv = (const NM_LISTVIEW *) pnmhdr;
            
            if (g_iTaskSortColumnID == g_ActiveTaskCol[pnmv->iSubItem])
            {
                g_iTaskSortDirection  *= -1;
            }
            else
            {
                g_iTaskSortColumnID = g_ActiveTaskCol[pnmv->iSubItem];
                g_iTaskSortDirection  = -1;
            }
            ResortTaskArray(&m_pTaskArray);
            TimerEvent();
            break;
        }

        case LVN_GETDISPINFO:
        {
            LV_ITEM * plvitem = &(((LV_DISPINFO *) pnmhdr)->item);
            
            // Listview needs a text string

            if (plvitem->mask & LVIF_TEXT)
            {
                TASKCOLUMNID columnid = (TASKCOLUMNID) g_ActiveTaskCol[plvitem->iSubItem];
                const CTaskInfo  * pTaskInfo   = (const CTaskInfo *)   plvitem->lParam;

                switch(columnid)
                {
                    case COL_TASKNAME:
                        lstrcpyn(plvitem->pszText, pTaskInfo->m_pszWindowTitle, plvitem->cchTextMax);
                        plvitem->mask |= LVIF_DI_SETITEM;
                        break;

                    case COL_TASKSTATUS:
                    {
                        if (pTaskInfo->m_fHung)
                        {
                            lstrcpyn(plvitem->pszText, g_szHung, plvitem->cchTextMax);
                        }
                        else
                        {
                            lstrcpyn(plvitem->pszText, g_szRunning, plvitem->cchTextMax);
                        }
                        break;
                    }

                    case COL_TASKWINSTATION:
                        lstrcpyn(plvitem->pszText, pTaskInfo->m_lpWinsta, plvitem->cchTextMax);
                        plvitem->mask |= LVIF_DI_SETITEM;
                        break;

                    case COL_TASKDESKTOP:
                        lstrcpyn(plvitem->pszText, pTaskInfo->m_lpDesktop, plvitem->cchTextMax);
                        plvitem->mask |= LVIF_DI_SETITEM;
                        break;


                    default:
                        Assert( 0 && "Unknown listview subitem" );
                        break;

                } // end switch(columnid)

            } // end LVIF_TEXT case

        } // end LVN_GETDISPINFO case
    
    } // end switch(pnmhdr->code)

    return 1;
}

/*++ DoEnumWindowStations

Routine Description:

    Does an EnumWindowStations on a new thread, since the thread needs
    to bop around to various window stations, which isn't allow for the
    main thread since it owns windows.

    This app is really single-threaded, and written with assumptions
    based on that, so the calling thread blocks until the new thread
    has completed the job.
    
Arguments:

    Same as EnumWindowStations

Return Value:

    Same as EnumWindowStations

Revision History:

      Nov-29-95 Davepl  Created

--*/

DWORD WorkerThread(LPVOID pv)
{
    THREADPARAM * ptp = (THREADPARAM *) pv;

    while(1)
    {
        // Wait for a signal from the main thread before proceeding

        WaitForSingleObject(ptp->m_hEventChild, INFINITE);

        // If we are flagged for shutdown, exit now.  Main thread will
        // be waiting on the event for us to signal that we are done with
        // the THREADPARAM block

        if (ptp->m_fThreadExit)
        {
            SetEvent(ptp->m_hEventParent);
            return 0;
        }

        ptp->m_fSuccess = EnumWindowStations(ptp->m_lpEnumFunc, ptp->m_lParam);
        SetEvent(ptp->m_hEventParent);
    }
}

BOOL CTaskPage::DoEnumWindowStations(WINSTAENUMPROC lpEnumFunc, LPARAM lParam)
{
    DWORD dwThreadId;
    
    if (NULL == m_hEventChild)
    {
        m_hEventChild = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (NULL == m_hEventChild)
        {
            return FALSE;
        }
    }

    if (NULL == m_hEventParent)
    {
        m_hEventParent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (NULL == m_hEventParent)
        {
            return FALSE;
        }
    }

    // Save the args away for the worker thread to pick up when it starts
    
    m_tp.m_lpEnumFunc   = lpEnumFunc;
    m_tp.m_lParam       = lParam;
    m_tp.m_hEventChild  = m_hEventChild;
    m_tp.m_hEventParent = m_hEventParent;
    m_tp.m_fThreadExit  = FALSE;

    if (NULL == m_hThread)
    {
        // Run the function call on this new thread, and wait for completion

        m_hThread = CreateThread(NULL, 0, WorkerThread, (LPVOID) &m_tp, 0, &dwThreadId);
        if (NULL == m_hThread)
        {
            return FALSE;
        }
    }

    SetEvent(m_hEventChild);
    WaitForSingleObject(m_hEventParent, INFINITE);

    // Return the result from the worker thread

    return (BOOL) m_tp.m_fSuccess;
}
   
/*++ CTaskPage::TimerEvent

Routine Description:

    Called by main app when the update time fires.  Walks every window
    in the system (on every desktop, in every windowstation) and adds
    or updates it in the task array, then removes any stale processes,
    and filters the results into the listview
    
Arguments:

Return Value:

Revision History:

      Nov-29-95 Davepl  Created

--*/

VOID CTaskPage::TimerEvent()
{
    //
    // If this page is paused (ie: it has a context menu up, etc), we do
    // nothing
    //

    if (m_fPaused)
    {
        return;
    }

    static LARGE_INTEGER uPassCount = {0, 0};

    TASK_LIST_ENUM te;
    
    te.m_pTasks = m_pTaskArray;
    te.m_pPage  = this;
    te.lpWinsta = NULL;
    te.lpDesk   = NULL;
    te.uPassCount.QuadPart = uPassCount.QuadPart;

    //
    // enumerate all windows and try to get the window
    // titles for each task
    //
    
    if ( DoEnumWindowStations( EnumWindowStationsFunc, (LPARAM) &te ))
    {
        INT i = 0;
        while (i < m_pTaskArray->GetSize())
        {
            CTaskInfo * pTaskInfo = (CTaskInfo *)(m_pTaskArray->GetAt(i));
            ASSERT(pTaskInfo);

            //
            // If passcount doesn't match, delete the CTaskInfo instance and remove
            // its pointer from the array.  Note that we _don't_ increment the index
            // if we remove an element, since the next element would now live at
            // the current index after the deletion
            //

            if (pTaskInfo->m_uPassCount.QuadPart != uPassCount.QuadPart)
            {
                // Find out what icons this task was using

                INT iLargeIcon = pTaskInfo->m_iLargeIcon;
                INT iSmallIcon = pTaskInfo->m_iSmallIcon;

                // Remove the task from the task array

                delete pTaskInfo;
                m_pTaskArray->RemoveAt(i, 1);

                // Remove its images from the imagelist

                if (iSmallIcon > 0)
                {
                    VERIFY( ImageList_Remove(m_himlSmall, iSmallIcon) );
                }
                if (iLargeIcon > 0)
                {
                    VERIFY( ImageList_Remove(m_himlLarge, iLargeIcon) );
                }

                // Fix up the icon indexes for any other tasks (whose icons were
                // at a higher index than the deleted process, and hence now shifted)

                for (int iTmp = 0; iTmp < m_pTaskArray->GetSize(); iTmp++)
                {
                    CTaskInfo * pTaskTmp = (CTaskInfo *)(m_pTaskArray->GetAt(iTmp));
                    
                    if (iLargeIcon && pTaskTmp->m_iLargeIcon > iLargeIcon)
                    {
                        pTaskTmp->m_iLargeIcon--;
                    }

                    if (iSmallIcon && pTaskTmp->m_iSmallIcon > iSmallIcon)
                    {
                        pTaskTmp->m_iSmallIcon--;
                    }
                }
            }
            else
            {
                i++;
            }
        }

        // Selectively filter the new array into the task listview

        UpdateTaskListview();
    }

    if (te.lpWinsta)
    {
        LocalFree(te.lpWinsta);
    }

    if (te.lpDesk)
    {
        LocalFree(te.lpDesk);
    }

    g_cTasks = m_pTaskArray->GetSize();

    uPassCount.QuadPart++;
}

/*++ class CTaskInfo::SetData

Class Description:

    Updates (or initializes) the info about a running task

Arguments:


    hwnd      - taks's hwnd
    lpTitle   - Window title
    uPassCount- Current passcount, used to timestamp the last update of 
                this object
    lpDesktop - task's current desktop
    lpWinsta  - task's current windowstation
    fUpdate   - only worry about information that can change during a
                task's lifetime

Return Value:

    HRESULT

Revision History:

      Nov-16-95 Davepl  Created

--*/

HRESULT CTaskInfo::SetData(HWND                         hwnd,
                           LPTSTR                       lpTitle,
                           LPTSTR                       lpWinsta,
                           LPTSTR                       lpDesktop,
                           LARGE_INTEGER                uPassCount,
                           BOOL                         fUpdateOnly)
{
    HRESULT hr = S_OK;

        // Touch this CTaskInfo to indicate that it's still alive

        m_uPassCount.QuadPart = uPassCount.QuadPart;

    //
    // For each of the fields, we check to see if anything has changed, and if
    // so, we mark that particular column as having changed, and update the value.
    // This allows me to opimize which fields of the listview to repaint, since
    // repainting an entire listview column causes flicker and looks bad in
    // general
    //

    // Window Station

    if (!fUpdateOnly || lstrcmp(m_lpWinsta, lpWinsta))
    {
        if (m_lpWinsta)
            LocalFree(m_lpWinsta);

        m_lpWinsta = (LPTSTR) LocalAlloc( 0, (lstrlen(lpWinsta) + 1) * sizeof(TCHAR));
        if (NULL == m_lpWinsta)
        {
            return E_OUTOFMEMORY;
        }
        else
        {
            lstrcpy(m_lpWinsta, lpWinsta);
        }
        m_fDirty_COL_WINSTA = TRUE;
        // dprintf(TEXT("Winsta changed: %s from %s to %s\n"), m_pszWindowTitle, m_lpWinsta, lpWinsta);
    }

    // Desktop

    if (!fUpdateOnly || lstrcmp(m_lpDesktop, lpDesktop))
    {
        if (m_lpDesktop)
            LocalFree(m_lpDesktop);

        m_lpDesktop = (LPTSTR) LocalAlloc( 0, (lstrlen(lpDesktop) + 1) * sizeof(TCHAR));
        if (NULL == m_lpDesktop)
        {
            return E_OUTOFMEMORY;
        }
        else
        {
            lstrcpy(m_lpDesktop, lpDesktop);
        }
        m_fDirty_COL_DESKTOP = TRUE;
        // dprintf(TEXT("Desktop changed: %s from %s to %s\n"), m_pszWindowTitle, m_lpDesktop, lpDesktop);
    }

    // Title

    if (!fUpdateOnly || lstrcmp(m_pszWindowTitle, lpTitle))
    {
        if (m_pszWindowTitle)
            LocalFree(m_pszWindowTitle);

        m_pszWindowTitle = (LPTSTR) LocalAlloc( 0, (lstrlen(lpTitle) + 1) * sizeof(TCHAR));
        if (NULL == m_pszWindowTitle)
        {
            return E_OUTOFMEMORY;
        }
        else
        {
            lstrcpy(m_pszWindowTitle, lpTitle);
        }
        m_fDirty_COL_TITLE = TRUE;
        // dprintf(TEXT("Title changed: %s from %s to %s\n"), m_pszWindowTitle, m_pszWindowTitle, lpTitle);
    }

    // App status (hung / not hung)

    BOOL fHung = IsHungAppWindow(hwnd);
    if (fHung != m_fHung)
    {
        m_fHung = fHung;
        m_fDirty_COL_STATUS = TRUE;
        // dprintf(TEXT("Status changed: %s\n"), m_pszWindowTitle);
    }

    // Window handle

    if (m_hwnd != hwnd)
    {
        m_hwnd = hwnd;
        m_fDirty_COL_HWND = TRUE;
        // dprintf(TEXT("Handle changed: %s\n"), m_pszWindowTitle);

    }

    // Icons
    
    #define ICON_FETCH_TIMEOUT 100

    if (!fUpdateOnly)
    {
        m_hSmallIcon = NULL;
        m_hLargeIcon = NULL;

        if (!SendMessageTimeout(hwnd, WM_GETICON, 0, 0, 
                SMTO_BLOCK | SMTO_ABORTIFHUNG, ICON_FETCH_TIMEOUT, (PULONG_PTR) &m_hSmallIcon)
            || NULL == m_hSmallIcon)
        {
            m_hSmallIcon = (HICON) GetClassLongPtr(hwnd, GCLP_HICONSM);
        }
        if (!SendMessageTimeout(hwnd, WM_GETICON, 1, 0, 
                SMTO_BLOCK | SMTO_ABORTIFHUNG, ICON_FETCH_TIMEOUT, (PULONG_PTR) &m_hLargeIcon)
            || NULL == m_hLargeIcon)
        {
            m_hLargeIcon = (HICON) GetClassLongPtr(hwnd, GCLP_HICON);
        }
    }
    
    return S_OK;
}

/*++

Routine Description:

    Callback function for windowstation enumeration.

Arguments:

    lpstr            - windowstation name
    lParam           - ** not used **

Return Value:

    TRUE  - continues the enumeration

--*/

BOOL CALLBACK EnumWindowStationsFunc(LPTSTR  lpstr, LPARAM lParam)
{
    PTASK_LIST_ENUM   te = (PTASK_LIST_ENUM)lParam;
    HWINSTA           hwinsta;
    HWINSTA           hwinstaSave;
    DWORD             ec;

    //
    // open the windowstation
    //

    hwinsta = OpenWindowStation( lpstr, FALSE, MAXIMUM_ALLOWED );
    if (!hwinsta) 
    {
        // If we fail because we don't have sufficient access to this
        // desktop, we should continue the enumeration anyway.

        return TRUE;
    }

    //
    // save the current windowstation
    //

    hwinstaSave = GetProcessWindowStation();

    //
    // change the context to the new windowstation
    //

    if (!SetProcessWindowStation( hwinsta )) 
    {
        ec = GetLastError();
        SetProcessWindowStation( hwinstaSave );
        CloseWindowStation( hwinsta );
        
        if (hwinsta != hwinstaSave)
                CloseWindowStation( hwinstaSave );
        
        return TRUE;
    }

    //
    // Update the windowstation in the enumerator
    //

    if (te->lpWinsta)
    {
        LocalFree(te->lpWinsta);
    }

    te->lpWinsta = (LPTSTR) LocalAlloc( 0, (lstrlen(lpstr) + 1) * sizeof(TCHAR));
    if (NULL == te->lpWinsta)
    {
        if (hwinsta != hwinstaSave) 
        {
            SetProcessWindowStation( hwinstaSave );
            CloseWindowStation( hwinsta );
        }
        CloseWindowStation( hwinstaSave );

        // We technically could continue, but if we're this strapped for
        // memory, there's not much point.  Let's bail on the winsta enumeration.
        return FALSE;
    }
    else
    {
        lstrcpy(te->lpWinsta, lpstr);
    }

    //
    // enumerate all the desktops for this windowstation
    //
    
    EnumDesktops( hwinsta, EnumDesktopsFunc, lParam );

    //
    // restore the context to the previous windowstation
    //

    if (hwinsta != hwinstaSave) 
    {
        SetProcessWindowStation( hwinstaSave );
        CloseWindowStation( hwinsta );
    }

    //
    // continue the enumeration
    //

    return TRUE;
}

/*++

Routine Description:

    Callback function for desktop enumeration.

Arguments:

    lpstr            - desktop name
    lParam           - ** not used **

Return Value:

    TRUE  - continues the enumeration

--*/

BOOL CALLBACK EnumDesktopsFunc(LPTSTR  lpstr, LPARAM lParam)
{
    PTASK_LIST_ENUM   te = (PTASK_LIST_ENUM)lParam;
    HDESK             hdeskSave;
    HDESK             hdesk;
    DWORD             ec;


    //
    // open the desktop
    //

    hdesk = OpenDesktop( lpstr, 0, FALSE, MAXIMUM_ALLOWED );
    if (!hdesk) 
    {
        return FALSE;
    }

    //
    // save the current desktop
    //

    hdeskSave = GetThreadDesktop( GetCurrentThreadId() );

    //
    // change the context to the new desktop
    //

    if (!SetThreadDesktop( hdesk )) 
    {
        ec = GetLastError();
        SetThreadDesktop( hdeskSave );
        if (g_hMainDesktop != hdesk)
        {
            CloseDesktop( hdesk );
        }
        if (g_hMainDesktop != hdeskSave)
        {
            CloseDesktop( hdeskSave );
        }
        return TRUE;
    }

    //
    // Update the desktop in the enumerator
    //

    if (te->lpDesk)
    {
        LocalFree(te->lpDesk);
    }

    te->lpDesk = (LPTSTR) LocalAlloc( 0, (lstrlen(lpstr) + 1) * sizeof(TCHAR));
    if (NULL == te->lpDesk)
    {   
        if (hdesk != hdeskSave) 
        {
            SetThreadDesktop( hdeskSave );
        }
        if (g_hMainDesktop != hdesk)
        {
            CloseDesktop( hdesk );
        }
        if (g_hMainDesktop != hdeskSave)
        {
            CloseDesktop( hdeskSave );
        }
        return FALSE;
    }
    else
    {
        lstrcpy(te->lpDesk, lpstr);
    }

    //
    // enumerate all windows in the new desktop
    //

    EnumWindows( (WNDENUMPROC) EnumWindowsProc, lParam ); 

    //
    // restore the previous desktop
    //

    if (hdesk != hdeskSave)
    {
        SetThreadDesktop( hdeskSave );
    }
    
    if (g_hMainDesktop != hdesk)
    {
        CloseDesktop( hdesk );
    }
    if (g_hMainDesktop != hdeskSave)
    {
        CloseDesktop( hdeskSave );
    }

    return TRUE;
}

/*++

Routine Description:

    Callback function for window enumeration.

Arguments:

    hwnd             - window handle
    lParam           - ** not used **

Return Value:

    TRUE  - continues the enumeration

--*/

BOOL CALLBACK EnumWindowsProc(HWND    hwnd, LPARAM   lParam)
{
    DWORD             pid = 0;
    DWORD             i;
    PTASK_LIST_ENUM   te = (PTASK_LIST_ENUM)lParam;
    DWORD             numTasks = te->m_pTasks->GetSize();
    TCHAR             szTitle[MAX_PATH];

    if ((GetWindow( hwnd, GW_OWNER ))   || 
        (!IsWindowVisible(hwnd)))
        
    {
        //
        // not a top level window, or not visible
        //

        return TRUE;
    }

    if (FALSE == InternalGetWindowText(hwnd, szTitle, ARRAYSIZE(szTitle)))
    {
        // Can't get the title - something weird going on.. but continue anyway

        return TRUE;
    }

    if (TEXT('\0') == szTitle[0])
    {
        // Empty title - of little value in the task list

        return TRUE;
    }

    if (hwnd == g_hMainWnd)
    {
        // Don't show the Task Manager in the list

        return TRUE;
    }

    if (0 == lstrcmpi(szTitle, TEXT("Program Manager")))
    {
        // Don't show the Program Manager (explorer) in the list

        return TRUE;
    }

    //
    // look for the task in the task list for this window
    //

    for (i=0; i < numTasks; i++) 
    {
        CTaskInfo * pTask = (CTaskInfo *) te->m_pTasks->GetAt(i);

        if (pTask->m_hwnd == hwnd)
        {
            //
            // Update the task info
            //

            if (FAILED(pTask->SetData(hwnd, szTitle, te->lpWinsta, te->lpDesk, te->uPassCount, TRUE)))
            {
                return FALSE;
            }
            pTask->m_uPassCount.QuadPart = te->uPassCount.QuadPart;

            break;
        }
    }

    if (i >= numTasks)
    {
        // Didn't find the task, it must be a new one

        CTaskInfo * pTask = new CTaskInfo;
        if (NULL == pTask)
        {
            return FALSE;
        }

        // Init the task data.  If fails, delete and bail

        if (FAILED(pTask->SetData(hwnd, szTitle, te->lpWinsta, te->lpDesk, te->uPassCount, FALSE)))
        {
            delete pTask;
            return FALSE;
        }
        else
        {
            // Add the icons to the page's imagelist

            if (!pTask->m_hLargeIcon && !pTask->m_hSmallIcon)
            {
                pTask->m_iLargeIcon = 0;
                pTask->m_iSmallIcon = 0;
            }
            else
            {
                // The indices to the small and large icons for a task must
                // always be the same; so, if one size is missing, use the icon
                // of the other size (stretched).  All the resizing is taken
                // care of for us by ImageList_AddIcon(), since it's already
                // had a fixed size set on it and will force any added icon
                // into that size.                
                pTask->m_iLargeIcon = ImageList_AddIcon(te->m_pPage->m_himlLarge, 
                                                        pTask->m_hLargeIcon ? 
                                                                pTask->m_hLargeIcon
                                                            :   pTask->m_hSmallIcon);
                if (-1 == pTask->m_iLargeIcon)
                {
                    delete pTask;
                    return FALSE;
                }

                pTask->m_iSmallIcon = ImageList_AddIcon(te->m_pPage->m_himlSmall, 
                                                        pTask->m_hSmallIcon ? 
                                                                pTask->m_hSmallIcon
                                                            :   pTask->m_hLargeIcon);
                if (-1 == pTask->m_iSmallIcon)
                {
                    ImageList_Remove(te->m_pPage->m_himlLarge, pTask->m_iLargeIcon);
                    delete pTask;
                    return FALSE;
                }           
            }

            // All went well, so add it to the array

            if (!(te->m_pTasks->Add( (LPVOID) pTask)))
            {
                delete pTask;
                return FALSE;
            }
        }
    }

    //
    // continue the enumeration
    //

    return TRUE;
}

/*++ CTaskPage::SizeTaskPage

Routine Description:

    Sizes its children based on the size of the
    tab control on which it appears.  

Arguments:

Return Value:

Revision History:

      Nov-29-95 Davepl  Created

--*/

static const INT aTaskControls[] =
{
    IDC_SWITCHTO,
    IDC_ENDTASK,
    IDM_RUN
};

void CTaskPage::SizeTaskPage()
{
    // Get the coords of the outer dialog

    RECT rcParent;
    GetClientRect(m_hPage, &rcParent);

    HDWP hdwp = BeginDeferWindowPos(10);

    // Calc the deltas in the x and y positions that we need to
    // move each of the child controls

    RECT rcMaster;
    HWND hwndMaster = GetDlgItem(m_hPage, IDM_RUN);
    GetWindowRect(hwndMaster, &rcMaster);
    MapWindowPoints(HWND_DESKTOP, m_hPage, (LPPOINT) &rcMaster, 2);

    INT dx = ((rcParent.right - g_DefSpacing * 2) - rcMaster.right);
    INT dy = ((rcParent.bottom - g_DefSpacing * 2) - rcMaster.bottom);

    // Size the listbox

    HWND hwndListbox = GetDlgItem(m_hPage, IDC_TASKLIST);
    RECT rcListbox;
    GetWindowRect(hwndListbox, &rcListbox);
    MapWindowPoints(HWND_DESKTOP, m_hPage, (LPPOINT) &rcListbox, 2);

    INT lbX = rcMaster.right - rcListbox.left + dx;
    INT lbY = rcMaster.top - rcListbox.top + dy - g_DefSpacing;

    DeferWindowPos(hdwp, hwndListbox, NULL,
                        0, 0,
                        lbX, 
                        lbY,
                        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);


#if 0   // This is handy, but the listview repaint is horrible
    
    // Adjust the first column width to be the width of the listbox
    // less the size of the status column

    INT cxStatus = ListView_GetColumnWidth(hwndListbox, 1);

    if (lbX - cxStatus > 0)
    {
        ListView_SetColumnWidth(hwndListbox, 0, lbX - cxStatus);
    }

#endif

    // Move each of the child controls by the above delta

    for (int i = 0; i < ARRAYSIZE(aTaskControls); i++)
    {
        HWND hwndCtrl = GetDlgItem(m_hPage, aTaskControls[i]);
        RECT rcCtrl;
        GetWindowRect(hwndCtrl, &rcCtrl);
        MapWindowPoints(HWND_DESKTOP, m_hPage, (LPPOINT) &rcCtrl, 2);

        DeferWindowPos(hdwp, hwndCtrl, NULL, 
                         rcCtrl.left + dx, 
                         rcCtrl.top + dy,
                         0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }


    EndDeferWindowPos(hdwp);
}

/*++ CTaskPage::HandleWMCOMMAND

Routine Description:

    Handles WM_COMMANDS received at the main page dialog
    
Arguments:

    id - Command id of command received

Return Value:

Revision History:

      Dec-01-95 Davepl  Created

--*/

void CTaskPage::HandleWMCOMMAND(INT id)
{
    switch(id)
    {
        case IDM_TASK_FINDPROCESS:
        {
            DWORD dwCount;
            HWND * pHwnds = GetHWNDS(TRUE, &dwCount);

            // Send a message to the main window telling it to
            // switch pages and select the process in question in
            // the process view

            if (pHwnds)
            {
                DWORD pid = 0;
                DWORD tid;

                tid = GetWindowThreadProcessId(pHwnds[0], &pid);
                if (pid)
                {
                    PostMessage(g_hMainWnd, WM_FINDPROC, tid, pid);
                }
                LocalFree(pHwnds);
            }
            break;
        }

        // These menu items (from the popup) have matching ones in the main menu,
        // so just pass them along to the main menu

        case IDM_LARGEICONS:
        case IDM_SMALLICONS:
        case IDM_DETAILS:
        case IDM_RUN:
        {
            SendMessage(g_hMainWnd, WM_COMMAND, MAKELPARAM(id, 0), 0);
            break;
        }

        case IDM_TASK_SWITCHTO:
        case IDC_SWITCHTO:
        {
            DWORD dwCount;
            HWND * pHwnds = GetHWNDS(m_cSelected, &dwCount);

            if (pHwnds)
            {
                // If target is minimized, restore it

                if (IsIconic(pHwnds[0]))
                {
                    ShowWindow(pHwnds[0], SW_RESTORE);
                }

                // Switch to the target window, and if the options dictate,
                // minimize the taskmanager

                HWND hwndLastActive = GetLastActivePopup(pHwnds[0]);
                if (!IsWindow(hwndLastActive)) {
                    MessageBeep(0);
                    LocalFree(pHwnds);
                    break;
                }

                // Can really only switch if the window is not disabled

                LONG lTemp = GetWindowLong(hwndLastActive, GWL_STYLE);
                if (0 == (lTemp & WS_DISABLED)) 
                {
                    //  Use SwitchToThisWindow() to bring dialog parents as well.
                    SwitchToThisWindow(hwndLastActive, TRUE);
                    if (g_Options.m_fMinimizeOnUse)
                    {
                        ShowWindow(g_hMainWnd, SW_MINIMIZE);
                    }
                } 
                else 
                {
                    MessageBeep(0);
                }
                LocalFree(pHwnds);
            }
            break;
        }

        case IDC_TILEHORZ:
        case IDM_TASK_TILEHORZ:
        {
            DWORD dwCount;
            HWND * pHwnds = GetHWNDS(m_cSelected, &dwCount);

            if (pHwnds)
            {
                EnsureWindowsNotMinimized(pHwnds, dwCount);
            }

            TileWindows(GetDesktopWindow(),
                        MDITILE_HORIZONTAL,
                        NULL,
                        dwCount,
                        pHwnds);
            if (pHwnds)
            {
                LocalFree(pHwnds);
            }
            break;
        }

        case IDM_TASK_TILEVERT:
        {
            DWORD dwCount;
            HWND * pHwnds = GetHWNDS(m_cSelected, &dwCount);

            if (pHwnds)
            {
                EnsureWindowsNotMinimized(pHwnds, dwCount);
            }

            TileWindows(GetDesktopWindow(),
                        MDITILE_VERTICAL,
                        NULL,
                        dwCount,
                        pHwnds);
            if (pHwnds)
            {
                LocalFree(pHwnds);
            }
            break;
        }

        case IDM_TASK_CASCADE:
        {
            DWORD dwCount;

            HWND * pHwnds = GetHWNDS(m_cSelected, &dwCount);

            if (pHwnds)
            {
                EnsureWindowsNotMinimized(pHwnds, dwCount);
            }

            CascadeWindows(GetDesktopWindow(),
                   0,
                   NULL,
                   dwCount,
                   pHwnds);
            if (pHwnds)
            {
                LocalFree(pHwnds);
            }
            break;
        }

        case IDM_TASK_MINIMIZE:
        case IDM_TASK_MAXIMIZE:
        {
            DWORD dwCount;
            
            // If some selected, just get them, else get all
             
            HWND * pHwnds = GetHWNDS(m_cSelected, &dwCount);

            if (pHwnds)
            {
                for (UINT i = 0; i < dwCount; i++)
                {
                    ShowWindowAsync(pHwnds[i], (id == IDC_MINIMIZE || id == IDM_TASK_MINIMIZE) ?
                                                SW_MINIMIZE : SW_MAXIMIZE);
                }
                LocalFree(pHwnds);
            }
            break;
        }

        case IDM_TASK_BRINGTOFRONT:
        {
            DWORD dwCount;
            HWND * pHwnds = GetHWNDS(TRUE, &dwCount);
            if (pHwnds)
            {
                EnsureWindowsNotMinimized(pHwnds, dwCount);
                                
                // Walk backwards through the list so that the first window selected
                // in on top

                for (INT i = (INT) dwCount - 1; i >= 0 ; i--)
                {
                    SetWindowPos(pHwnds[i], HWND_TOP, 0, 0, 0, 0,
                                 SWP_NOSIZE | SWP_NOMOVE);
                }
                DWORD dwProc;
                if (GetWindowThreadProcessId(pHwnds[0], &dwProc))
                    AllowSetForegroundWindow(dwProc);
                SetForegroundWindow(pHwnds[0]);
                LocalFree(pHwnds);
            }
            break;
            
        }

        case IDC_ENDTASK:
        case IDM_TASK_ENDTASK:
        {
            DWORD dwCount;
            HWND * pHwnds = GetHWNDS(TRUE, &dwCount);
            if (pHwnds)
            {
                BOOL fForce = GetKeyState(VK_CONTROL) & ( 1 << 16) ? TRUE : FALSE;
                for(UINT i = 0; i < dwCount; i++)
                {
                    // SetActiveWindow(aHwnds[i]);
                    EndTask(pHwnds[i], FALSE, fForce);
                }

                LocalFree(pHwnds);
            }
            break;
        }

        default:
            break;
    }

    Unpause();
}

/*++ TaskPageProc

Routine Description:

    Dialogproc for the task manager page.  
    
Arguments:

    hwnd        - handle to dialog box
    uMsg        - message
    wParam      - first message parameter
    lParam      - second message parameter

Return Value:
    
    For WM_INITDIALOG, TRUE == success
    For others, TRUE == this proc handles the message

Revision History:

      Nov-28-95 Davepl  Created

--*/

INT_PTR CALLBACK TaskPageProc(
                HWND        hwnd,               // handle to dialog box
                UINT        uMsg,                   // message
                WPARAM      wParam,                 // first message parameter
                LPARAM      lParam                  // second message parameter
                )
{
    CTaskPage * thispage = (CTaskPage *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    // See if the parent wants this message

    if (TRUE == CheckParentDeferrals(uMsg, wParam, lParam))
    {
        return TRUE;
    }

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
            CTaskPage * thispage = (CTaskPage *) lParam;

            thispage->m_hPage = hwnd;

            HWND hTaskList = GetDlgItem(hwnd, IDC_TASKLIST);
            ListView_SetImageList(hTaskList, thispage->m_himlSmall, LVSIL_SMALL);

            // Turn on SHOWSELALWAYS so that the selection is still highlighted even
            // when focus is lost to one of the buttons (for example)

            SetWindowLong(hTaskList, GWL_STYLE, GetWindowLong(hTaskList, GWL_STYLE) | LVS_SHOWSELALWAYS);

            if (SHRestricted(REST_NORUN))
            {
                EnableWindow (GetDlgItem(hwnd, IDM_RUN), FALSE);
            }
            
            SetFocus(GetDlgItem(hwnd, IDC_TASKLIST));

        SubclassListView(GetDlgItem(hwnd, IDC_PROCLIST));
        
            return FALSE;
        }

        
        // We need to fake client mouse clicks in this child to appear as nonclient
        // (caption) clicks in the parent so that the user can drag the entire app
        // when the title bar is hidden by dragging the client area of this child

        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
        {
            if (g_Options.m_fNoTitle)
            {
                SendMessage(g_hMainWnd, 
                            uMsg == WM_LBUTTONUP ? WM_NCLBUTTONUP : WM_NCLBUTTONDOWN, 
                            HTCAPTION, 
                            lParam);
            }
            break;
        }
 
        case WM_COMMAND:
        {
            thispage->HandleWMCOMMAND(LOWORD(wParam));
            break;
        }

        case WM_NOTIFY:
        {
            return thispage->HandleTaskPageNotify((HWND) wParam, (LPNMHDR) lParam);
        }

        case WM_MENUSELECT:
        {
            if ((UINT) HIWORD(wParam) == 0xFFFF)
            {
                // Menu dismissed, resume display

                thispage->Unpause();
            }
            break;
        }
        case WM_CONTEXTMENU:
        {
            if ((HWND) wParam == GetDlgItem(hwnd, IDC_TASKLIST))
            {
                thispage->HandleTaskListContextMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                return TRUE;
            }
            break;
        }

        // Size our kids

        case WM_SIZE:
        {
            thispage->SizeTaskPage();
            return TRUE;
        }

        case WM_SETTINGCHANGE:
            thispage->OnSettingsChange();
            // fall through
        case WM_SYSCOLORCHANGE:
            SendMessage(GetDlgItem(hwnd, IDC_TASKLIST), uMsg, wParam, lParam);
            return TRUE;

        default:
            return FALSE;
    }
    return FALSE;
}

void CTaskPage::OnSettingsChange()
{
    // in going between large-font settings and normal settings, the size of small 
    // icons changes; so throw away all our icons and change the size of images in 
    // our lists
    
    BOOL fPaused = m_fPaused; // pause the page so we can get through
    m_fPaused = TRUE;         // the below without being updated  

    RemoveAllTasks();
    m_pTaskArray->RemoveAll();
    
    m_vmViewMode = VM_INVALID;      // cause an update to the list view
    
    // you'd think that since SetIconSize does a RemoveAll anyway, the
    // explicit RemoveAll calls are redundant; however, if SetIconSize
    // gets size parameters which aren't different from what it has,
    // it fails without doing a RemoveAll!
    ImageList_RemoveAll(m_himlLarge);
    ImageList_RemoveAll(m_himlSmall);
    ImageList_SetIconSize(m_himlLarge, GetSystemMetrics(SM_CXICON),
                                        GetSystemMetrics(SM_CYICON));
    ImageList_SetIconSize(m_himlSmall, GetSystemMetrics(SM_CXSMICON),
                                        GetSystemMetrics(SM_CYSMICON));

    LoadDefaultIcons();     // this could return an error, but if it does,
                            // we just have to press on

    m_fPaused = fPaused;            // restore the paused state
    TimerEvent();           // even if we're paused, we'll want to redraw
}

/*++ CTaskPage::GetTitle

Routine Description:

    Copies the title of this page to the caller-supplied buffer
    
Arguments:

    pszText     - the buffer to copy to
    bufsize     - size of buffer, in characters

Return Value:

Revision History:

      Nov-28-95 Davepl  Created

--*/

void CTaskPage::GetTitle(LPTSTR pszText, size_t bufsize)
{
    LoadString(g_hInstance, IDS_TASKPAGETITLE, pszText, bufsize);
}

/*++ CTaskPage::Activate

Routine Description:

    Brings this page to the front, sets its initial position,
    and shows it
    
Arguments:

Return Value:

    HRESULT (S_OK on success)

Revision History:

      Nov-28-95 Davepl  Created

--*/
 
HRESULT CTaskPage::Activate()
{
    // Make this page visible

    ShowWindow(m_hPage, SW_SHOW);

    SetWindowPos(m_hPage,
                 HWND_TOP,
                 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE);

    SetFocus(GetDlgItem(m_hPage, IDC_TASKLIST));

    // Change the menu bar to be the menu for this page

    HMENU hMenuOld = GetMenu(g_hMainWnd);
    HMENU hMenuNew = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MAINMENU_TASK));

    if (hMenuNew && SHRestricted(REST_NORUN))
    {
        DeleteMenu(hMenuNew, IDM_RUN, MF_BYCOMMAND);
    }

    g_hMenu = hMenuNew;
    if (g_Options.m_fNoTitle == FALSE)
    {
        SetMenu(g_hMainWnd, hMenuNew);
    }

    if (hMenuOld)
    {
        DestroyMenu(hMenuOld);
    }

    return S_OK;
}


/*++ class CTaskPage::SetupColumns

Class Description:

    Removes any existing columns from the taskmanager listview and
    adds all of the columns listed in the g_ActiveTaskCol array.

Arguments:

Return Value:

    HRESULT

Revision History:

      Nov-29-95 Davepl  Created

--*/

HRESULT CTaskPage::SetupColumns()
{
    HWND hwndList = GetDlgItem(m_hPage, IDC_TASKLIST);
    if (NULL == hwndList)
    {
        return E_UNEXPECTED;
    }    

    ListView_DeleteAllItems(hwndList);

    // Remove all existing columns

    LV_COLUMN lvcolumn;
    while(ListView_DeleteColumn(hwndList, 0))
    {
        NULL;
    }

    // Add all of the new columns

    INT iColumn = 0;
    while (g_ActiveTaskCol[iColumn] >= 0)
    {
        INT idColumn = g_ActiveTaskCol[iColumn];

        TCHAR szTitle[MAX_PATH];
        LoadString(g_hInstance, IDS_FIRSTTASKCOL + idColumn, szTitle, ARRAYSIZE(szTitle));

        lvcolumn.mask       = LVCF_FMT | LVCF_TEXT | LVCF_TEXT | LVCF_WIDTH;
        lvcolumn.fmt        = TaskColumnDefaults[ idColumn ].Format;
        lvcolumn.cx         = TaskColumnDefaults[ idColumn ].Width;
        lvcolumn.pszText    = szTitle;
        lvcolumn.iSubItem   = iColumn;

        if (-1 == ListView_InsertColumn(hwndList, iColumn, &lvcolumn))
        {
            return E_FAIL;
        }
        iColumn++;
    }

    return S_OK;
}

/*++ CTaskPage::Initialize

Routine Description:

    Initializes the task manager page

Arguments:

    hwndParent  - Parent on which to base sizing on: not used for creation,
                  since the main app window is always used as the parent in
                  order to keep tab order correct
                  
Return Value:

Revision History:

      Nov-28-95 Davepl  Created

--*/

HRESULT CTaskPage::Initialize(HWND hwndParent)
{
    HRESULT hr = S_OK;
    UINT flags = ILC_MASK;
    //
    // Create the ptr array used to hold the info on running tasks
    //

    m_pTaskArray = new CPtrArray(GetProcessHeap());
    if (NULL == m_pTaskArray)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // Our pseudo-parent is the tab contrl, and is what we base our
        // sizing on.  However, in order to keep tab order right among
        // the controls, we actually create ourselves with the main
        // window as the parent

        m_hwndTabs = hwndParent;

        //
        // Create the image lists
        //
    if(IS_WINDOW_RTL_MIRRORED(hwndParent))
    {
        flags |= ILC_MIRROR;
    }
    m_himlSmall = ImageList_Create(
                    GetSystemMetrics(SM_CXSMICON),
                    GetSystemMetrics(SM_CYSMICON),
                    flags,
                    1,
                    1
                    );
    
        if (NULL == m_himlSmall)
        {
            hr = E_FAIL;    
        }
    }

    if (SUCCEEDED(hr))
    {
        m_himlLarge = ImageList_Create(
                    GetSystemMetrics(SM_CXICON),
                    GetSystemMetrics(SM_CYICON),
                    flags,
                    1,
                    1
                    );
        if (NULL == m_himlLarge)
        {
            hr = E_FAIL;
        }
    }

    // Load the default icons
    hr = LoadDefaultIcons();

    if (SUCCEEDED(hr))
    {
        //
        // Create the dialog which represents the body of this page
        //

        m_hPage = CreateDialogParam(
                        g_hInstance,                    // handle to application instance
                        MAKEINTRESOURCE(IDD_TASKPAGE),  // identifies dialog box template name  
                        g_hMainWnd,                     // handle to owner window
                        TaskPageProc,                   // pointer to dialog box procedure
                        (LPARAM) this );                // User data (our this pointer)

        if (NULL == m_hPage)
        {
            hr = GetLastHRESULT();
        }
    }

    if (SUCCEEDED(hr))
    {
        // Set up the columns in the listview

        hr = SetupColumns();
    }

    if (SUCCEEDED(hr))
    {
        TimerEvent();
    }

    //
    // If any failure along the way, clean up what got allocated
    // up to that point
    //

    if (FAILED(hr))
    {
        if (m_hPage)
        {
            DestroyWindow(m_hPage);
        }

        m_hwndTabs = NULL;
    }

    return hr;
}

HRESULT CTaskPage::LoadDefaultIcons()
{
    HICON   hDefLarge;
    HICON   hDefSmall;
    HRESULT hr = S_OK;
    
    hDefSmall = (HICON) LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_DEFAULT), IMAGE_ICON, 
                            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    if (!hDefSmall)
    {
        return GetLastHRESULT();
    }
    if (-1 == ImageList_AddIcon(m_himlSmall, hDefSmall))
    {
        hr = E_FAIL;
    }
    DestroyIcon(hDefSmall);

    hDefLarge = (HICON) LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_DEFAULT), IMAGE_ICON, 
                            GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
    if (!hDefLarge)
    {
        return GetLastHRESULT();
    }
    if (-1 == ImageList_AddIcon(m_himlLarge, hDefLarge))
    {
        hr = E_FAIL;
    }
    DestroyIcon(hDefLarge);
    
    return hr;
}

/*++ CTaskPage::Destroy

Routine Description:

    Frees whatever has been allocated by the Initialize call
    
Arguments:

Return Value:

Revision History:

      Nov-28-95 Davepl  Created

--*/

HRESULT CTaskPage::Destroy()
{
    if (m_hPage)
    {
        DestroyWindow(m_hPage);
        m_hPage = NULL;
    }

    if (m_hThread)
    {
        // Signal the child thead to exit, and wait for it to do so

        m_tp.m_fThreadExit = TRUE;
        SetEvent(m_hEventChild);
        WaitForSingleObject(m_hEventParent, INFINITE);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }

    if (m_hEventChild)
    {
        CloseHandle(m_hEventChild);
        m_hEventChild = NULL;
    }

    if (m_hEventParent)
    {
        CloseHandle(m_hEventParent);
        m_hEventParent = NULL;
    }

    // These are freed automatically by listview

    m_himlSmall = NULL;
    m_himlLarge = NULL;

    return S_OK;
}

/*++ CTaskPage::Deactivate

Routine Description:

    Called when this page is losing its place up front
    
Arguments:

Return Value:

Revision History:

      Nov-28-95 Davepl  Created

--*/

void CTaskPage::Deactivate()
{
    if (m_hPage)
    {
        ShowWindow(m_hPage, SW_HIDE);
    }
}
