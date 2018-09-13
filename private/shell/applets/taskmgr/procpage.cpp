//+-------------------------------------------------------------------------
//
//  TaskMan - NT TaskManager
//  Copyright (C) Microsoft
//
//  File:       procpage.cpp
//
//  History:    Nov-16-95   DavePl  Created
//
//--------------------------------------------------------------------------

#include "precomp.h"
//#include "tchar.h"  // maks_todo: required for ValidateProductSuite, remove later.


//
// Project-scope globals
//

DWORD       g_cProcesses    = 0;

//--------------------------------------------------------------------------
// TERMINAL SERVICES

BOOL TestUserForAdmin( VOID );  // tests if current process is being run in admin context.

//-- cache this state
BOOL IsUserAdmin( )
{
    static BOOL sbIsUserAdmin = TestUserForAdmin();

    return sbIsUserAdmin;
}

// get/set current session id.

// we use this session id to filter the processes for current session.

DWORD        gdwSessionId = -1;

inline DWORD GetCurrentSessionID( )
{
    return gdwSessionId;
}

inline VOID SetCurrentSessionID( DWORD dwSessionId )
{
    gdwSessionId = dwSessionId;
}

// END OF TERMINAL SERVICES DECLs
//--------------------------------------------------------------------------

//
// File-scope globals
//

SYSTEM_BASIC_INFORMATION g_BasicInfo;

//
// Table of which resource IDs in the column selection dialog
// correspond to which columns
//

const int g_aDlgColIDs[] =
{
    IDC_IMAGENAME,
    IDC_PID,
// #ifdef _HYDRA_
    IDC_USERNAME,
    IDC_SESSIONID,
// #endif

    IDC_CPU,
    IDC_CPUTIME,
    IDC_MEMUSAGE,
    IDC_MEMPEAK,
    IDC_MEMUSAGEDIFF,
    IDC_PAGEFAULTS,
    IDC_PAGEFAULTSDIFF,
    IDC_COMMITCHARGE,
    IDC_PAGEDPOOL,
    IDC_NONPAGEDPOOL,
    IDC_BASEPRIORITY,
    IDC_HANDLECOUNT,
    IDC_THREADCOUNT,
    IDC_USEROBJECTS,
    IDC_GDIOBJECTS,
    IDC_READOPERCOUNT,
    IDC_WRITEOPERCOUNT,
    IDC_OTHEROPERCOUNT,
    IDC_READXFERCOUNT,
    IDC_WRITEXFERCOUNT,
    IDC_OTHERXFERCOUNT
};

//
// Column ID on which to sort in the listview, and for
// compares in general
//

// #ifdef _HYDRA_
//COLUMNID g_iProcSortColumnID = COL_SESSIONID;
//#else
COLUMNID g_iProcSortColumnID = COL_PID;
//#endif
INT      g_iProcSortDirection = 1;          // 1 = asc, -1 = desc

//
// Column Default Info
//

struct
{
    INT Format;
    INT Width;
} ColumnDefaults[NUM_COLUMN] =
{
    { LVCFMT_LEFT,     0x6B },       // COL_IMAGENAME
    { LVCFMT_RIGHT,      50 },       // COL_PID
//#ifdef _HYDRA_
    { LVCFMT_LEFT,     0x6B },       // COL_USERNAME
    { LVCFMT_RIGHT,      70 },       // COL_SESSIONID
//#endif
    { LVCFMT_RIGHT,      35},        // COL_CPU
    { LVCFMT_RIGHT,      70 },       // COL_CPUTIME
    { LVCFMT_RIGHT,      70 },       // COL_MEMUSAGE
    { LVCFMT_RIGHT,     100 },       // COL_MEMPEAK
    { LVCFMT_RIGHT,      70 },       // COL_MEMUSAGEDIFF
    { LVCFMT_RIGHT,      70 },       // COL_PAGEFAULTS
    { LVCFMT_RIGHT,      70 },       // COL_PAGEFAULTSDIFF
    { LVCFMT_RIGHT,      70 },       // COL_COMMITCHARGE
    { LVCFMT_RIGHT,      70 },       // COL_PAGEDPOOL
    { LVCFMT_RIGHT,      70 },       // COL_NONPAGEDPOOL
    { LVCFMT_RIGHT,      60 },       // COL_BASEPRIORITY
    { LVCFMT_RIGHT,      60 },       // COL_HANDLECOUNT
    { LVCFMT_RIGHT,      60 },       // COL_THREADCOUNT
    { LVCFMT_RIGHT,      60 },       // COL_USEROBJECTS
    { LVCFMT_RIGHT,      60 },       // COL_GDIOBJECTS
    { LVCFMT_RIGHT,      70 },       // COL_READOPERCOUNT
    { LVCFMT_RIGHT,      70 },       // COL_WRITEOPERCOUNT
    { LVCFMT_RIGHT,      70 },       // COL_OTHEROPERCOUNT
    { LVCFMT_RIGHT,      70 },       // COL_READXFERCOUNT
    { LVCFMT_RIGHT,      70 },       // COL_WRITEXFERCOUNT
    { LVCFMT_RIGHT,      70 }        // COL_OTHERXFERCOUNT
};


/*++ class CProcInfo

Class Description:

    Represents the last known information about a running process

Arguments:

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

class CProcInfo
{
public:

    LARGE_INTEGER     m_uPassCount;
    DWORD             m_UniqueProcessId;
// #ifdef _HYDRA_
    LPTSTR            m_pszUserName;
    ULONG             m_SessionId;
// #endif
    BYTE              m_CPU;
    BYTE              m_DisplayCPU;
    LARGE_INTEGER     m_CPUTime;
    LARGE_INTEGER     m_DisplayCPUTime;
    ULONG             m_MemUsage;
    LONG              m_MemDiff;
    ULONG             m_PageFaults;
    LONG              m_PageFaultsDiff;
    ULONG_PTR         m_CommitCharge;
    ULONG_PTR         m_PagedPool;
    ULONG_PTR         m_NonPagedPool;
    KPRIORITY         m_PriClass;
    ULONG             m_HandleCount;
    ULONG             m_ThreadCount;
    ULONG             m_GDIObjectCount;
    ULONG             m_USERObjectCount;
    LONGLONG          m_IoReadOperCount;
    LONGLONG          m_IoWriteOperCount;
    LONGLONG          m_IoOtherOperCount;
    LONGLONG          m_IoReadXferCount;
    LONGLONG          m_IoWriteXferCount;
    LONGLONG          m_IoOtherXferCount;
    LPTSTR            m_pszImageName;
    CProcInfo *       m_pWowParentProcInfo;    // non-NULL for WOW tasks
    WORD              m_htaskWow;              // non-zero for WOW tasks
    BOOL              m_fWowProcess:1;         // TRUE for real WOW process
    BOOL              m_fWowProcessTested:1;   // TRUE once fWowProcess is valid
    ULONG             m_MemPeak;


    //
    // This is a union of who (which column) is dirty.  You can look at
    // or set any particular column's bit, or just inspect m_fDirty
    // to see if anyone at all is dirty.  Used to optimize listview
    // painting
    //

    union
    {
        DWORD                m_fDirty;
        struct
        {
            DWORD            m_fDirty_COL_CPU            :1;
            DWORD            m_fDirty_COL_CPUTIME        :1;
            DWORD            m_fDirty_COL_MEMUSAGE       :1;
            DWORD            m_fDirty_COL_MEMUSAGEDIFF   :1;
            DWORD            m_fDirty_COL_PAGEFAULTS     :1;
            DWORD            m_fDirty_COL_PAGEFAULTSDIFF :1;
            DWORD            m_fDirty_COL_COMMITCHARGE   :1;
            DWORD            m_fDirty_COL_PAGEDPOOL      :1;
            DWORD            m_fDirty_COL_NONPAGEDPOOL   :1;
            DWORD            m_fDirty_COL_BASEPRIORITY   :1;
            DWORD            m_fDirty_COL_HANDLECOUNT    :1;
            DWORD            m_fDirty_COL_IMAGENAME      :1;
            DWORD            m_fDirty_COL_PID            :1;
// #ifdef _HYDRA_
            DWORD            m_fDirty_COL_SESSIONID      :1;
            DWORD            m_fDirty_COL_USERNAME       :1;
// #endif
            DWORD            m_fDirty_COL_THREADCOUNT    :1;
            DWORD            m_fDirty_COL_GDIOBJECTS     :1;
            DWORD            m_fDirty_COL_USEROBJECTS    :1;
            DWORD            m_fDirty_COL_MEMPEAK        :1;
            DWORD            m_fDirty_COL_READOPERCOUNT  :1;
            DWORD            m_fDirty_COL_WRITEOPERCOUNT :1;
            DWORD            m_fDirty_COL_OTHEROPERCOUNT :1;
            DWORD            m_fDirty_COL_READXFERCOUNT  :1;
            DWORD            m_fDirty_COL_WRITEXFERCOUNT :1;
            DWORD            m_fDirty_COL_OTHERXFERCOUNT :1;
        };
    };

    HRESULT SetData(LARGE_INTEGER                TotalTime,
                    PSYSTEM_PROCESS_INFORMATION  pInfo,
                    LARGE_INTEGER                uPassCount,
                    CProcPage *                  pProcPage,
                    BOOL                         fUpdateOnly);

//#ifdef _HYDRA_
    HRESULT SetProcessUsername(const FILETIME *CreationTime);
//#endif

    HRESULT SetDataWowTask(LARGE_INTEGER  TotalTime,
                           DWORD          dwThreadId,
                           CHAR *         pszFilePath,
                           LARGE_INTEGER  uPassCount,
                           CProcInfo *    pParentProcInfo,
                           LARGE_INTEGER *pTimeLeft,
                           WORD           htask,
                           BOOL           fUpdateOnly);

    CProcInfo()
    {
        ZeroMemory(this, sizeof(*this));

// #ifdef _HYDRA_

        m_pszUserName = 0;

        m_SessionId = 832;

// #endif
    }

    ~CProcInfo()
    {
        if (m_pszImageName)
        {
            delete [] m_pszImageName;
        }

// #ifdef _HYDRA_

        if( m_pszUserName != NULL )
        {
            delete [] m_pszUserName;
        }
// #endif

    }

    BOOL OkToShowThisProcess ()
    {
        // this function determines if the process should be listed in the view.

        return GetCurrentSessionID() == m_SessionId;
    }


    // Invalidate() marks this proc with a bogus pid so that it is removed
    // on the next cleanup pass

    void Invalidate()
    {
        m_UniqueProcessId = PtrToUlong(INVALID_HANDLE_VALUE);
    }

    LONGLONG GetCPUTime() const
    {
        return m_CPUTime.QuadPart;
    }

    INT Compare(CProcInfo * pOther);

    //
    // Is this a WOW task psuedo-process?
    //

    INT_PTR IsWowTask(void) const
    {
        return (INT_PTR) m_pWowParentProcInfo;
    }

    //
    // Get the Win32 PID for this task
    //

    DWORD GetRealPID(void) const
    {
        return m_pWowParentProcInfo
               ? m_pWowParentProcInfo->m_UniqueProcessId
               : m_UniqueProcessId;
    }

    void SetCPU(LARGE_INTEGER CPUTimeDelta,
                LARGE_INTEGER TotalTime,
                BOOL          fDisplayOnly);
};

/*++ ColSelectDlgProc

Function Description:

    Dialog Procedure for the column selection dialog

Arguments:

    Standard wndproc stuff

Revision History:

      Jan-05-96 Davepl  Created

--*/

INT_PTR CALLBACK ColSelectDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static CProcPage * pPage = NULL;

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {


            // Looks scary, but we're single threaded

            pPage = (CProcPage *) lParam;

            // Start with none of the boxes checked

            for (int i = 0; i < NUM_COLUMN; i++)
            {
                CheckDlgButton(hwndDlg, g_aDlgColIDs[i], BST_UNCHECKED);
            }

//#ifdef _HYDRA_
           
            // HIDE the Username and SessionId if its not Terminal Server.

            if( !IsTerminalServer( ) )
            {
                ShowWindow( GetDlgItem( hwndDlg , IDC_USERNAME ) , SW_HIDE );

                ShowWindow( GetDlgItem( hwndDlg , IDC_SESSIONID ) , SW_HIDE );

            }

//#endif //_HYDRA_

            // Then turn on the ones for the columns we have active

            for (i = 0; i < NUM_COLUMN + 1; i++)
            {
                if (g_Options.m_ActiveProcCol[i] == -1)
                {
                    break;
                }

                CheckDlgButton(hwndDlg, g_aDlgColIDs[g_Options.m_ActiveProcCol[i]], BST_CHECKED);
            }

            return TRUE;
        }

        case WM_COMMAND:
        {
            // If user clicked OK, add the columns to the array and reset the listview

            if (LOWORD(wParam) == IDOK)
            {
                // First, make sure the column width array is up to date

                pPage->SaveColumnWidths();

                INT iCol = 1;

                g_Options.m_ActiveProcCol[0] = COL_IMAGENAME;

                for (int i = 1; i < NUM_COLUMN && g_aDlgColIDs[i] >= 0; i++)
                {
                    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, g_aDlgColIDs[i]))
                    {
                        // It is checked

                        if (g_Options.m_ActiveProcCol[iCol] != (COLUMNID) i)
                        {
                            // If the column wasn't already there, insert its column
                            // width into the column width array

                            ShiftArray(g_Options.m_ColumnWidths, iCol, SHIFT_UP);
                            ShiftArray(g_Options.m_ActiveProcCol, iCol, SHIFT_UP);
                            g_Options.m_ColumnWidths[iCol] = ColumnDefaults[ i ].Width;
                            g_Options.m_ActiveProcCol[iCol] = (COLUMNID) i;
                        }
                        iCol++;
                    }
                    else
                    {
                        // Not checked, column not active.  If it used to be active,
                        // remove its column width from the column width array

                        if (g_Options.m_ActiveProcCol[iCol] == (COLUMNID) i)
                        {
                            ShiftArray(g_Options.m_ColumnWidths, iCol, SHIFT_DOWN);
                            ShiftArray(g_Options.m_ActiveProcCol, iCol, SHIFT_DOWN);
                        }
                    }
                }

                // Terminate the column list
                                
                g_Options.m_ActiveProcCol[iCol] = (COLUMNID) -1;
                pPage->SetupColumns();
                pPage->TimerEvent();
                EndDialog(hwndDlg, IDOK);

            }
            else if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hwndDlg, IDCANCEL);
            }
        }
    }
    return FALSE;
}

/*++ CProcPage::~CProcPage()

        - Destructor
*/

CProcPage::~CProcPage()
{
    if (m_pProcArray)
    {
        INT c = m_pProcArray->GetSize();

        while (c)
        {
            delete (CProcInfo *) (m_pProcArray->GetAt(c - 1));
            c--;
        }

        delete m_pProcArray;
    }
}

/*++ CProcPage::PickColumns()

Function Description:

    Puts up UI that lets the user select what columns to display in the
    process page, and then resets the listview with the new column list

Arguments:

    none

Return Value:

    none

Revision History:

    Jan-05-96 Davepl  Created

--*/

void CProcPage::PickColumns()
{
    DialogBoxParam(g_hInstance, 
                   MAKEINTRESOURCE(IDD_SELECTPROCCOLS), 
                   g_hMainWnd, 
                   (DLGPROC)ColSelectDlgProc,
                   (LPARAM) this);
}

/*++ GetPriRanking

Function Description:

    Since the priority class defines aren't in order, this helper
    exists to make comparisons between pri classes easier.  It returns
    a larger number for "higher" priority classes

Arguments:

Return Value:

    rank of priority (0 to 5)

Revision History:

      Nov-27-95 Davepl  Created

--*/


DWORD GetPriRanking(DWORD dwClass)
{
    switch(dwClass)
    {
        case REALTIME_PRIORITY_CLASS:
            return 5;

        case HIGH_PRIORITY_CLASS:
            return 4;

        case ABOVE_NORMAL_PRIORITY_CLASS:
            return 3;

        case NORMAL_PRIORITY_CLASS:
            return 2;

        case BELOW_NORMAL_PRIORITY_CLASS:
            return 1;


        default:
            return 0;
    }
}

/*++ QuickConfirm

Function Description:

    Gets a confirmation for things like terminating/debugging processes

Arguments:

    idtitle - string ID of title for message box
    idmsg   - string ID of message body

Return Value:

    IDNO/IDYES, whatever comes back from MessageBox

Revision History:

      Nov-28-95 Davepl  Created

--*/

UINT CProcPage::QuickConfirm(UINT idTitle, UINT idBody)
{
    /* We have removed the ability to disable confirmations

    if (FALSE == g_Options.m_fConfirmations)
    {
        return IDYES;
    }
    */

    // Get confirmation before we dust the process, or something similar

    TCHAR szTitle[MAX_PATH];
    TCHAR szBody[MAX_PATH];

    if (0 == LoadString(g_hInstance, idTitle, szTitle, ARRAYSIZE(szTitle)) ||
        0 == LoadString(g_hInstance, idBody,    szBody,  ARRAYSIZE(szBody)))
    {
        return IDNO;
    }


    if (IDYES == MessageBox(m_hPage, szBody, szTitle, MB_ICONEXCLAMATION | MB_YESNO))
    {
        return IDYES;
    }

    return IDNO;
}

/*++ class CProcPage::SetupColumns

Class Description:

    Removes any existing columns from the process listview and
    adds all of the columns listed in the g_Options.m_ActiveProcCol array.

Arguments:

Return Value:

    HRESULT

Revision History:

      Nov-16-95 Davepl  Created

--*/

static const _aIDColNames[NUM_COLUMN] =
{
    IDS_COL_IMAGENAME,     
    IDS_COL_PID,
// #ifdef _HYDRA_

    IDS_COL_USERNAME,

    IDS_COL_SESSIONID,

// #endif
    IDS_COL_CPU,           
    IDS_COL_CPUTIME,       
    IDS_COL_MEMUSAGE,      
    IDS_COL_MEMPEAK,       
    IDS_COL_MEMUSAGEDIFF,  
    IDS_COL_PAGEFAULTS,    
    IDS_COL_PAGEFAULTSDIFF,
    IDS_COL_COMMITCHARGE,  
    IDS_COL_PAGEDPOOL,     
    IDS_COL_NONPAGEDPOOL,  
    IDS_COL_BASEPRIORITY,  
    IDS_COL_HANDLECOUNT,   
    IDS_COL_THREADCOUNT,   
    IDS_COL_USEROBJECTS,   
    IDS_COL_GDIOBJECTS,
    IDS_COL_READOPERCOUNT,
    IDS_COL_WRITEOPERCOUNT,
    IDS_COL_OTHEROPERCOUNT,
    IDS_COL_READXFERCOUNT,
    IDS_COL_WRITEXFERCOUNT,
    IDS_COL_OTHERXFERCOUNT
};

HRESULT CProcPage::SetupColumns()
{
    HWND hwndList = GetDlgItem(m_hPage, IDC_PROCLIST);
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
    while (g_Options.m_ActiveProcCol[iColumn] >= 0)
    {
        
        INT idColumn = g_Options.m_ActiveProcCol[iColumn];

// #ifdef _HYDRA_ 
        
        // idc_username or IDC_SESSIONID are available only for terminalserver.

        ASSERT((idColumn != COL_USERNAME && idColumn != COL_SESSIONID) || IsTerminalServer());

// #endif

        TCHAR szTitle[MAX_PATH];
        LoadString(g_hInstance, _aIDColNames[idColumn], szTitle, ARRAYSIZE(szTitle));

        lvcolumn.mask       = LVCF_FMT | LVCF_TEXT | LVCF_TEXT | LVCF_WIDTH;
        lvcolumn.fmt        = ColumnDefaults[ idColumn ].Format;

        // If no width preference has been recorded for this column, use the
        // default

        if (-1 == g_Options.m_ColumnWidths[iColumn])
        {
            lvcolumn.cx = ColumnDefaults[ idColumn ].Width;
        }
        else
        {
            lvcolumn.cx = g_Options.m_ColumnWidths[iColumn];
        }

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

//
//  Take two unsigned 64-bit values and compare them in a manner
//  that CProcInfo::Compare likes.
//
int Compare64(unsigned __int64 First, unsigned __int64 Second)
{
    if (First < Second)
        return -1;
    else if (First > Second)
        return 1;
    else
        return 0;
}

/*++ class CProcInfo::Compare

Class Description:

    Compares this CProcInfo object to another, and returns its ranking
    based on the g_iProcSortColumnID field.

    Note that if the objects are equal based on the current sort column,
    the PID is used as a secondary sort key to prevent items from 
    jumping around in the listview

    WOW psuedo-processes always sort directly after their parent
    ntvdm.exe process.  So really the sort order is:

    1. WOW task psuedo-processes under parent in alpha order
    2. User's selected order.
    3. PID

Arguments:

    pOther  - the CProcInfo object to compare this to

Return Value:

    < 0      - This CProcInfo is "less" than the other
      0      - Equal (Can't happen, since PID is used to sort)
    > 0      - This CProcInfo is "greater" than the other

Revision History:

      Nov-20-95 Davepl  Created

--*/

INT CProcInfo::Compare(CProcInfo * pOther)
{
    CProcInfo * pMyThis;
    CProcInfo * pMyOther;
    INT iRet = 0;

    //
    // Wow psuedo-processes don't have any performance information,
    // so use the parent "real" ntvdm.exe CProcInfo for sorting.
    //

    ASSERT(this != pOther);

    pMyThis = this->IsWowTask()
              ? this->m_pWowParentProcInfo
              : this;

    pMyOther = pOther->IsWowTask()
               ? pOther->m_pWowParentProcInfo
               : pOther;

    if (pMyThis == pMyOther) {

        //
        // This implies one or the other or both this and pOther
        // are WOW tasks, and they're in the same WOW VDM.  Sort
        // the "real" process entry first, followed by its associated
        // WOW task entries alphabetical.
        //

        if (this->IsWowTask()) {

            if (pOther->IsWowTask()) {

                //
                // They are siblings and we sort by
                // image name.
                //

                ASSERT(this->m_pWowParentProcInfo ==
                       pOther->m_pWowParentProcInfo);

                iRet = lstrcmpi(this->m_pszImageName, pOther->m_pszImageName);

            } else {

                //
                // pOther is not a Wow task, it must be ntvdm.exe
                // the parent of this.  this sorts after pOther.
                //

                ASSERT(pOther == this->m_pWowParentProcInfo);

                iRet = 1;
            }
        } else {

            //
            // this is not a Wow task, pOther must be and
            // this must be pOther's parent.
            //

            ASSERT(pOther->IsWowTask());

            iRet = -1;
        }
    }


    if (0 == iRet) {

        switch (g_iProcSortColumnID)
        {
            case COL_CPU:
                iRet = Compare64(pMyThis->m_CPU, pMyOther->m_CPU);
            break;

            case COL_CPUTIME:
                iRet = Compare64(pMyThis->m_CPUTime.QuadPart, pMyOther->m_CPUTime.QuadPart);
                break;

            case COL_MEMUSAGE:
                iRet = Compare64(pMyThis->m_MemUsage, pMyOther->m_MemUsage);
                break;

            case COL_MEMUSAGEDIFF:
                iRet = Compare64(pMyThis->m_MemDiff, pMyOther->m_MemDiff);
                break;

            case COL_MEMPEAK:
                iRet = Compare64(pMyThis->m_MemPeak, pMyOther->m_MemPeak);
                break;

            case COL_PAGEFAULTS:
                iRet = Compare64(pMyThis->m_PageFaults, pMyOther->m_PageFaults);
                break;

            case COL_PAGEFAULTSDIFF:
                iRet = Compare64(pMyThis->m_PageFaultsDiff, pMyOther->m_PageFaultsDiff);
                break;

            case COL_COMMITCHARGE:
                iRet = Compare64(pMyThis->m_CommitCharge, pMyOther->m_CommitCharge);
                break;

            case COL_PAGEDPOOL:
                iRet = Compare64(pMyThis->m_PagedPool, pMyOther->m_PagedPool);
                break;

            case COL_NONPAGEDPOOL:
                iRet = Compare64(pMyThis->m_NonPagedPool, pMyOther->m_NonPagedPool);
                break;

            case COL_BASEPRIORITY:
                iRet = Compare64(GetPriRanking(pMyThis->m_PriClass), GetPriRanking(pMyOther->m_PriClass));
                break;

            case COL_HANDLECOUNT:
                iRet = Compare64(pMyThis->m_HandleCount, pMyOther->m_HandleCount);
                break;

            case COL_THREADCOUNT:
                iRet = Compare64(pMyThis->m_ThreadCount, pMyOther->m_ThreadCount);
                break;

            case COL_PID:
                iRet = Compare64(pMyThis->m_UniqueProcessId, pMyOther->m_UniqueProcessId);
                break;
// #ifdef _HYDRA_

            case COL_SESSIONID:
                
                iRet = Compare64(pMyThis->m_SessionId, pMyOther->m_SessionId);

                break;

            case COL_USERNAME:
                
                iRet = lstrcmpi( pMyThis->m_pszUserName , pMyOther->m_pszUserName );

                break;
// #endif
            case COL_IMAGENAME:
                iRet = lstrcmpi(pMyThis->m_pszImageName, pMyOther->m_pszImageName);
                break;

            case COL_USEROBJECTS:
                iRet = Compare64(pMyThis->m_USERObjectCount, pMyOther->m_USERObjectCount);
                break;

            case COL_GDIOBJECTS:
                iRet = Compare64(pMyThis->m_GDIObjectCount, pMyOther->m_GDIObjectCount);
                break;

            case COL_READOPERCOUNT:
                iRet = Compare64(pMyThis->m_IoReadOperCount, pMyOther->m_IoReadOperCount);
                break;

            case COL_WRITEOPERCOUNT:
                iRet = Compare64(pMyThis->m_IoWriteOperCount, pMyOther->m_IoWriteOperCount);
                break;

            case COL_OTHEROPERCOUNT:
                iRet = Compare64(pMyThis->m_IoOtherOperCount, pMyOther->m_IoOtherOperCount);
                break;

            case COL_READXFERCOUNT:
                iRet = Compare64(pMyThis->m_IoReadXferCount, pMyOther->m_IoReadXferCount);
                break;

            case COL_WRITEXFERCOUNT:
                iRet = Compare64(pMyThis->m_IoWriteXferCount, pMyOther->m_IoWriteXferCount);
                break;

            case COL_OTHERXFERCOUNT:
                iRet = Compare64(pMyThis->m_IoOtherXferCount, pMyOther->m_IoOtherXferCount);
                break;

            default:

                #ifdef DEBUG
                    DebugBreak();
                #endif

                iRet = 0;
        }

        iRet *= g_iProcSortDirection;
    }

    // If objects look equal, compare on PID as secondary sort column
    // so that items don't jump around in the listview

    if (0 == iRet)
    {
        iRet = Compare64(pMyThis->m_UniqueProcessId,
                      pMyOther->m_UniqueProcessId) *
                     g_iProcSortDirection;
    }

    return iRet;
}


/*++ class CProcInfo::SetCPU

Method Description:

    Sets the CPU percentage.

Arguments:

    CPUTime   - Time for this process
    TotalTime - Total elapsed time, used as the denominator in calculations

Return Value:

Revision History:

      19-Feb-96  DaveHart  Created

--*/

void CProcInfo::SetCPU(LARGE_INTEGER CPUTimeDelta,
                       LARGE_INTEGER TotalTime,
                       BOOL fDisplayOnly)
{
    // Calc CPU time based on this process's ratio of the total process time used

    INT cpu = (BYTE) (((CPUTimeDelta.QuadPart / ((TotalTime.QuadPart / 1000) ?
                                                 (TotalTime.QuadPart / 1000) : 1)) + 5)
                                              / 10);
    if (100 == cpu)
    {
        cpu = 99;
    }

// BUGBUG (jeffreys) disabling this assert to let BVT's run. DavePl needs
// to find a real fix for this.  Raid #236356, 231880
//    ASSERT( cpu <= 100);

    if (m_DisplayCPU != cpu)
    {
        m_fDirty_COL_CPU = TRUE;
        m_DisplayCPU = (BYTE) cpu;

        if ( ! fDisplayOnly )
        {
            m_CPU = (BYTE) cpu;
        }
    }

}
/*++ CProcPage::GetProcessInfo

Class Description:

    Reads the process info table into a virtual alloc'd buffer, resizing
    the buffer if needed

Arguments:

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

static const int PROCBUF_GROWSIZE = 4096;

HRESULT CProcPage::GetProcessInfo()
{
    HRESULT  hr = S_OK;
    NTSTATUS status;

    while(hr == S_OK)
    {
        if (m_pvBuffer)
        {
            status = NtQuerySystemInformation(SystemProcessInformation,
                                              m_pvBuffer,
                                              m_cbBuffer,
                                              NULL);

            //
            // If we succeeded, great, get outta here.  If not, any error other
            // than "buffer too small" is fatal, in which case we bail
            //

            if (NT_SUCCESS(status))
            {
                break;
            }

            if (status != STATUS_INFO_LENGTH_MISMATCH)
            {
                hr = E_FAIL;
                break;
            }
        }

        //
        // Buffer wasn't large enough to hold the process info table, so resize it
        // to be larger, then retry.
        //

        if (m_pvBuffer)
        {
            VirtualFree(m_pvBuffer, 0, MEM_RELEASE);
            m_pvBuffer = NULL;
        }

        m_cbBuffer += PROCBUF_GROWSIZE;

        m_pvBuffer = VirtualAlloc (NULL,
                                   m_cbBuffer,
                                   MEM_COMMIT,
                                   PAGE_READWRITE);
        if (m_pvBuffer == NULL)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
    }

    return hr;
}


/*++ CProcPage::Int64ToCommaSepString

Class Description:

    Convert a 64-bit integer to a string with commas.

    (2^64)-1 = "18,446,744,073,709,600,000"  (27 chars).

Arguments:

    n           - 64-bit integer.
    pszOut      - Destination character buffer.
    cchOut      - Size of destination buffer in characters.

Return Value:

    None.

Revision History:

      Jan-11-99 BrianAu  Created

--*/

NTSTATUS
MyMoBettaRtlInt64ToUnicodeString (
    IN LONGLONG Value,
    IN ULONG Base OPTIONAL,
    IN OUT PUNICODE_STRING String
    )

{

    NTSTATUS Status;
    char ResultBuffer[32];
    ANSI_STRING AnsiString;
    LARGE_INTEGER Temp;

    if (Value < 0)
    {
        Temp.QuadPart = -Value;
        Status = RtlLargeIntegerToChar(&Temp,
                                       Base,
                                       sizeof(ResultBuffer) - sizeof(CHAR),
                                       &ResultBuffer[sizeof(CHAR)]);
        *(ResultBuffer)=L'-';    
    }
    else
    {
        Temp.QuadPart = Value;
        Status = RtlLargeIntegerToChar(&Temp,
                                       Base,
                                       sizeof(ResultBuffer),
                                       ResultBuffer);
    }

    if (NT_SUCCESS(Status)) {
        AnsiString.Buffer = ResultBuffer;
        AnsiString.MaximumLength = sizeof(ResultBuffer);
        AnsiString.Length = (USHORT)strlen(ResultBuffer);
        Status = RtlAnsiStringToUnicodeString(String, &AnsiString, FALSE);
    }

    return Status;
}
void
CProcPage::Int64ToCommaSepString(
    LONGLONG n,
    LPTSTR pszOut,
    int cchOut
    )
{
    ULONG ulTemp;
    UNICODE_STRING s;
    NUMBERFMTW nfmtW; 
    LPWSTR pszFmtOutW;
    int cchFmtOut;
    WCHAR szTextW[32];
    //
    // Convert the 64-bit int to a text string.
    //
    s.Length        = 0;
    s.MaximumLength = sizeof(szTextW) - sizeof(TCHAR);
    s.Buffer        = szTextW;
    MyMoBettaRtlInt64ToUnicodeString(n, 10, &s);
    //
    // Format the number with commas according to locale conventions.
    //
    nfmtW.NumDigits     = 0;
    nfmtW.LeadingZero   = 0;
    RtlInitUnicodeString(&s, m_wszGroupSep);
    RtlUnicodeStringToInteger(&s, 10, &ulTemp);
    nfmtW.Grouping      = UINT(ulTemp);
    nfmtW.lpDecimalSep  = nfmtW.lpThousandSep = m_wszGroupThousSep;
    nfmtW.NegativeOrder = 0;

#ifndef UNICODE
    //
    // If ansi build we need a wide-char buffer as format destination.
    //
    WCHAR szNumW[30];
    pszFmtOutW = szNumW;
    cchFmtOut  = ARRAYSIZE(szNumW);
#else
    //
    // If unicode build we can format directly to the destination buffer.
    //
    pszFmtOutW = pszOut;
    cchFmtOut  = cchOut;
#endif

    GetNumberFormatW(LOCALE_USER_DEFAULT,
                     0,
                     szTextW,
                     &nfmtW,
                     pszFmtOutW,
                     cchFmtOut);
#ifndef UNICODE
    //
    // If ansi build, need extra step to convert formatted number string
    // (wide char) back to ansi.
    //
    WideCharToMultiByte(CP_ACP,
                        0,
                        szFmtOutW,
                        -1,
                        pszOut,
                        cchOut,
                        NULL,
                        NULL);
#endif
}


/*++ CProcPage::Int64ToCommaSepKString

Class Description:

    Convert a 64-bit integer to a string with commas appended
    with the "K" units designator.

    (2^64)-1 = "18,446,744,073,709,600,000 K"  (29 chars).

Arguments:

    n           - 64-bit integer.
    pszOut      - Destination character buffer.

Return Value:

    None.

Revision History:

      Jan-11-99 BrianAu  Created

--*/

void
CProcPage::Int64ToCommaSepKString(
    LONGLONG n,
    LPTSTR pszOut,
    int cchOut
    )
{
    TCHAR szText[40];

    Int64ToCommaSepString(n, szText, ARRAYSIZE(szText));

    LPTSTR pszEnd = szText + lstrlen(szText);

    *pszEnd++ = TEXT(' ');
    lstrcpy(pszEnd, g_szK);
    lstrcpyn(pszOut, szText, cchOut);
}


/*++ CProcPage::RestoreColumnOrder

Routine Description:

    Sets the column order from the per-user preference data stored 
    in the global COptions object.
    
Arguments:

    hwndList - Listview window handle.

Return Value:

Revision History:

      Jan-11/99 BrianAu  Created

--*/

void
CProcPage::RestoreColumnOrder(
    HWND hwndList
    )
{
    INT rgOrder[ARRAYSIZE(g_Options.m_ColumnPositions)];
    INT cOrder = 0;
    INT iOrder = 0;

    for (int i = 0; i < ARRAYSIZE(g_Options.m_ColumnPositions); i++)
    {
        iOrder = g_Options.m_ColumnPositions[i];
        if (-1 == iOrder)
            break;

        rgOrder[cOrder++] = iOrder;
    }
    if (0 < cOrder)
    {
        const HWND hwndHeader = ListView_GetHeader(hwndList);
        ASSERT(Header_GetItemCount(hwndHeader) == cOrder);
        Header_SetOrderArray(hwndHeader, Header_GetItemCount(hwndHeader), rgOrder);
    }
}


/*++ CProcPage::RememberColumnOrder

Routine Description:

    Saves the current column order to the global COptions object
    which is later saved to the registry for per-user preferences.
    
Arguments:

    hwndList - Listview window handle.

Return Value:

Revision History:

      Jan-11/99 BrianAu  Created

--*/

void
CProcPage::RememberColumnOrder(
    HWND hwndList
    )
{
    const HWND hwndHeader = ListView_GetHeader(hwndList);

    ASSERT(Header_GetItemCount(hwndHeader) <= ARRAYSIZE(g_Options.m_ColumnPositions));

    FillMemory(&g_Options.m_ColumnPositions, sizeof(g_Options.m_ColumnPositions), 0xFF);
    Header_GetOrderArray(hwndHeader, 
                         Header_GetItemCount(hwndHeader),
                         g_Options.m_ColumnPositions);
}



/*++ FindProcInArrayByPID

Class Description:

    Walks the ptrarray given and looks for the CProcInfo object
    that has the PID supplied.  If not found, returns NULL

Arguments:

    pArray      - The CPtrArray where the CProcInfos could live
    pid         - The pid to search for

Return Value:

    CProcInfo * in the array, if found, or NULL if not

Revision History:

      Nov-20-95 Davepl  Created

--*/

// REVIEW (DavePl) could provide a static search hint here so
// that it doesn't always need to start back at zero, or could
// do a binary search

CProcInfo * FindProcInArrayByPID(CPtrArray * pArray, DWORD pid)
{
    for (int i = 0; i < pArray->GetSize(); i++)
    {
        CProcInfo * pTmp = (CProcInfo *) (pArray->GetAt(i));
        
        if (pTmp->m_UniqueProcessId == pid)
        {
            // Found it

            return pTmp;
        }
    }

    // Not found

    return NULL;
}

/*++ InsertIntoSortedArray

Class Description:

    Sticks a CProcInfo ptr into the ptrarray supplied at the
    appropriate location based on the current sort column (which
    is used by the Compare member function)

Arguments:

    pArray      - The CPtrArray to add to
    pProc       - The CProcInfo object to add to the array

Return Value:

    TRUE if successful, FALSE if fails

Revision History:

      Nov-20-95 Davepl  Created

--*/

// REVIEW (davepl) Use binary insert here, not linear

BOOL InsertIntoSortedArray(CPtrArray * pArray, CProcInfo * pProc)
{
    
    INT cItems = pArray->GetSize();
    
    for (INT iIndex = 0; iIndex < cItems; iIndex++)
    {
        CProcInfo * pTmp = (CProcInfo *) pArray->GetAt(iIndex);
        
        if (pProc->Compare(pTmp) < 0)
        {
            return pArray->InsertAt(iIndex, pProc);
        }
    }

    return pArray->Add(pProc);
}

/*++ ResortArray

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

BOOL ResortArray(CPtrArray ** ppArray)
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
        CProcInfo * pItem = (CProcInfo *) (*ppArray)->GetAt(i);

        // _TS_
        
        if (IsTerminalServer())
        {
            // Check to see if we have the option to show all processed.  If we
            // don't then check to see if the item (process) is running in our
            // own session (which means we can add the item into the sorted array).
            // If not, then just skip to the next item

            if (!g_Options.m_bShowAllProcess && !pItem->OkToShowThisProcess())
            {
                // Skip this one.  Since we will orphan it (leave its pointer behind in the
                // old array that we're about to nuke), delete it now.

                delete pItem;
                continue;
            }
        }

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


typedef struct
{
    LARGE_INTEGER               uPassCount;
    CProcPage *                 pProcPage;
    CProcInfo *                 pParentProcInfo;
    LARGE_INTEGER               TotalTime;
    LARGE_INTEGER               TimeLeft;
} WOWTASKCALLBACKPARMS, *PWOWTASKCALLBACKPARMS;


BOOL WINAPI WowTaskCallback(
    DWORD dwThreadId,
    WORD hMod16,
    WORD hTask16,
    CHAR *pszModName,
    CHAR *pszFileName,
    LPARAM lparam
    )
{
    PWOWTASKCALLBACKPARMS pParms = (PWOWTASKCALLBACKPARMS)lparam;
    HRESULT hr;

    //
    // See if this task is already in the list.
    //
    
    CProcInfo * pOldProcInfo;
    pOldProcInfo = FindProcInArrayByPID(
                       pParms->pProcPage->m_pProcArray,
                       dwThreadId);

    if (NULL == pOldProcInfo)
    {
        //
        // We don't already have this process in our array, so create a new one
        // and add it to the array
        //

        CProcInfo * pNewProcInfo = new CProcInfo;
        if (NULL == pNewProcInfo)
        {
            goto done;
        }

        hr = pNewProcInfo->SetDataWowTask(pParms->TotalTime,
                                                    dwThreadId,
                                                    pszFileName,
                                                    pParms->uPassCount,
                                                    pParms->pParentProcInfo,
                                                    &pParms->TimeLeft,
                                                    hTask16,
                                                    FALSE);

        if (FAILED(hr) ||
            FALSE == pParms->pProcPage->m_pProcArray->Add(pNewProcInfo))
        {
            delete pNewProcInfo;
            goto done;
        }
    }
    else
    {
        //
        // This process already existed in our array, so update its info
        //

        pOldProcInfo->SetDataWowTask(pParms->TotalTime,
                                     dwThreadId,
                                     pszFileName,
                                     pParms->uPassCount,
                                     pParms->pParentProcInfo,
                                     &pParms->TimeLeft,
                                     hTask16,
                                     TRUE);
    }

done:
    return FALSE;  // continue enumeration
}


/*++ class CProcInfo::SetDataWowTask

Method Description:

    Sets up a single CProcInfo object based on the parameters.
    This is a WOW task pseudo-process entry.

Arguments:

    dwThreadId

    pszFilePath    Fully-qualified path from VDMEnumTaskWOWEx.

Return Value:

Revision History:

      18-Feb-96  DaveHart  created

--*/

HRESULT CProcInfo::SetDataWowTask(LARGE_INTEGER  TotalTime,
                                  DWORD          dwThreadId,
                                  CHAR *         pszFilePath,
                                  LARGE_INTEGER  uPassCount,
                                  CProcInfo *    pParentProcInfo,
                                  LARGE_INTEGER *pTimeLeft,
                                  WORD           htask,
                                  BOOL           fUpdateOnly)
{
    CHAR *pchExe;

    //
    // Touch this CProcInfo to indicate the process is still alive
     //

    m_uPassCount.QuadPart = uPassCount.QuadPart;

    //
    // Update the thread's execution times.
    //

    HANDLE             hThread;
    NTSTATUS           Status;
    OBJECT_ATTRIBUTES  obja;
    CLIENT_ID          cid;

    InitializeObjectAttributes(
            &obja,
            NULL,
            0,
            NULL,
            0 );

    cid.UniqueProcess = 0;      // 0 means any process
    cid.UniqueThread  = (HANDLE) dwThreadId;

    Status = NtOpenThread(
                &hThread,
                THREAD_QUERY_INFORMATION,
                &obja,
                &cid );

    ULONGLONG ullCreation, ullExit, ullKernel, ullUser;
    if ( NT_SUCCESS(Status) )
    {
        LARGE_INTEGER TimeDelta, Time;

        if (GetThreadTimes(
                hThread,
                (LPFILETIME) &ullCreation,
                (LPFILETIME) &ullExit,
                (LPFILETIME) &ullKernel,
                (LPFILETIME) &ullUser
                ) )
        {

            Time.QuadPart = (LONGLONG)(ullUser + ullKernel);

            TimeDelta.QuadPart = Time.QuadPart - m_CPUTime.QuadPart;

            if (TimeDelta.QuadPart < 0)
            {
// BUGBUG (jeffreys) disabling this assert to let BVT's run. Someone needs
// to figure out what's really going on.  Raid #247473
//                ASSERT(0 && "WOW tasks's cpu total usage went DOWN since last refresh.");
                Invalidate();
                return E_FAIL;
            }

            if (TimeDelta.QuadPart)
            {
                m_fDirty_COL_CPUTIME = TRUE;
                m_CPUTime.QuadPart = Time.QuadPart;
            }

            //
            // Don't allow sum of WOW child task times to
            // exceed ntvdm.exe total.  We call GetThreadTimes
            // substantially after we get process times, so
            // this can happen.
            //

            if (TimeDelta.QuadPart > pTimeLeft->QuadPart)
            {
                TimeDelta.QuadPart = pTimeLeft->QuadPart;
                pTimeLeft->QuadPart = 0;
            }
            else
            {
                pTimeLeft->QuadPart -= TimeDelta.QuadPart;
            }

            SetCPU( TimeDelta, TotalTime, FALSE );

            //
            // When WOW tasks are being displayed, the line for ntvdm.exe
            // should show times only for overhead or historic threads,
            // not including any active task threads.
            //

            if (pParentProcInfo->m_DisplayCPUTime.QuadPart > m_CPUTime.QuadPart)
            {
                pParentProcInfo->m_DisplayCPUTime.QuadPart -= m_CPUTime.QuadPart;
            }
            else
            {
                pParentProcInfo->m_DisplayCPUTime.QuadPart = 0;
            }

            m_DisplayCPUTime.QuadPart = m_CPUTime.QuadPart;
        }

        NtClose(hThread);
    }

    if (m_PriClass != pParentProcInfo->m_PriClass) {
        m_fDirty_COL_BASEPRIORITY = TRUE;
        m_PriClass = pParentProcInfo->m_PriClass;
    }

// #ifdef _HYDRA_

    if( m_SessionId != pParentProcInfo->m_SessionId )
    {
        m_fDirty_COL_SESSIONID = TRUE;

        m_SessionId = pParentProcInfo->m_SessionId;

    }

// #endif

    if (FALSE == fUpdateOnly)
    {
        UINT uLen;

        //
        // Set the task's image name, thread ID, thread count,
        // htask, and parent CProcInfo which do not change over
        // time.
        //

        m_htaskWow = htask;

        m_fDirty_COL_PID = TRUE;
        m_fDirty_COL_IMAGENAME = TRUE;
        m_fDirty_COL_THREADCOUNT = TRUE;
// #ifdef _HYDRA_

        m_fDirty_COL_USERNAME = TRUE;

        m_fDirty_COL_SESSIONID = TRUE;

// #endif


        m_UniqueProcessId = dwThreadId;
        m_ThreadCount = 1;

        //
        // We're only interested in the filename of the EXE
        // with the path stripped.
        //

        pchExe = strrchr(pszFilePath, '\\');
        if (NULL == pchExe) {
            pchExe = pszFilePath;
        }
        else
        {
            // skip backslash
            pchExe++;
        }

        uLen = strlen(pchExe);

        //
        // Indent the EXE name by two spaces
        // so WOW tasks look subordinate to
        // their ntvdm.exe
        //
        m_pszImageName = new TCHAR[uLen + 3];
        if (NULL == m_pszImageName)
        {
            return E_OUTOFMEMORY;
        }

        m_pszImageName[0] = m_pszImageName[1] = TEXT(' ');

        MultiByteToWideChar(
            CP_ACP,
            0,
            pchExe,
            uLen,
            &m_pszImageName[2],
            uLen
            );
        m_pszImageName[uLen + 2] = 0;

        //
        // WOW EXE filenames are always uppercase, so lowercase it.
        //

        CharLowerBuff(&m_pszImageName[2], uLen);

        m_pWowParentProcInfo = pParentProcInfo;
        
//#ifdef _HYDRA_

        if( IsTerminalServer( ) )
        {
            SetProcessUsername( LPFILETIME( &ullCreation ) );
        }

//#endif
      
    }

    return S_OK;
}


/*++ class CProcInfo::SetData

Class Description:

    Sets up a single CProcInfo object based on the data contained in a
    SYSTEM_PROCESS_INFORMATION block.

    If fUpdate is set, the imagename and icon fields are not processed, 
    since they do not change throughout the lifetime of the process

Arguments:

    TotalTime - Total elapsed time, used as the denominator in calculations
                for the process' CPU usage, etc
    pInfo     - The SYSTEM_PROCESS_INFORMATION block for this process
    uPassCount- Current passcount, used to timestamp the last update of 
                this objectg
    fUpdate   - See synopsis

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/


HRESULT CProcInfo::SetData(LARGE_INTEGER                TotalTime, 
                           PSYSTEM_PROCESS_INFORMATION  pInfo, 
                           LARGE_INTEGER                uPassCount,
                           CProcPage *                  pProcPage,
                           BOOL                         fUpdateOnly)
{
    HRESULT hr = S_OK;
    DWORD dwTemp;
    HANDLE hProcess;

    // Touch this CProcInfo to indicate the process is still alive

    m_uPassCount.QuadPart = uPassCount.QuadPart;

    // Calc this process's total time as the sum of its user and kernel time

    LARGE_INTEGER TimeDelta;
    LARGE_INTEGER Time;

    if (pInfo->UserTime.QuadPart + pInfo->KernelTime.QuadPart < m_CPUTime.QuadPart)
    {
// BUGBUG (jeffreys) disabling this assert to let BVT's run. See comments in
// SetDataWowTask above.  Note that this assertion hasn't actually fired, but
// given the other weirdness that's been going on, I'm disabling it anyway.
//        ASSERT(0 && "Proc's cpu total usage went DOWN since last refresh.");
        Invalidate();
        return hr = E_FAIL;
    }

    Time.QuadPart = pInfo->UserTime.QuadPart +
                    pInfo->KernelTime.QuadPart;

    TimeDelta.QuadPart = Time.QuadPart - m_CPUTime.QuadPart;

    if (TimeDelta.QuadPart)
    {
        m_CPUTime.QuadPart = m_DisplayCPUTime.QuadPart = Time.QuadPart;
        m_fDirty_COL_CPUTIME = TRUE;
    }

    SetCPU( TimeDelta, TotalTime, FALSE );

    //
    // For each of the fields, we check to see if anything has changed, and if
    // so, we mark that particular column as having changed, and update the value.
    // This allows me to opimize which fields of the listview to repaint, since
    // repainting an entire listview column causes flicker and looks bad in
    // general
    //

    // Miscellaneous fields

    if (m_UniqueProcessId != PtrToUlong(pInfo->UniqueProcessId))
    {
        m_fDirty_COL_PID = TRUE;
        m_UniqueProcessId = PtrToUlong(pInfo->UniqueProcessId);
    }

// #ifdef _HYDRA_
    
    if( m_SessionId != pInfo->SessionId )
    {
        m_fDirty_COL_SESSIONID = TRUE;

        m_SessionId = pInfo->SessionId;

    }

// #endif


    if (m_MemDiff != ((LONG)pInfo->WorkingSetSize / 1024) - (LONG)m_MemUsage )
    {
        m_fDirty_COL_MEMUSAGEDIFF = TRUE;
        m_MemDiff =  ((LONG)pInfo->WorkingSetSize / 1024) - (LONG)m_MemUsage;
    }

    if (m_MemPeak != (pInfo->PeakWorkingSetSize / 1024))
    {
        m_fDirty_COL_MEMPEAK = TRUE;
        m_MemPeak = pInfo->PeakWorkingSetSize / 1024;
    }

    if (m_MemUsage != pInfo->WorkingSetSize / 1024)
    {
        m_fDirty_COL_MEMUSAGE = TRUE;
        m_MemUsage = pInfo->WorkingSetSize / 1024;
    }

    if (m_PageFaultsDiff != ((LONG)(pInfo->PageFaultCount) - (LONG)m_PageFaults))
    {
        m_fDirty_COL_PAGEFAULTSDIFF = TRUE;
        m_PageFaultsDiff = ((LONG)(pInfo->PageFaultCount) - (LONG)m_PageFaults);
    }

    if (m_PageFaults != (pInfo->PageFaultCount))
    {
        m_fDirty_COL_PAGEFAULTS = TRUE;
        m_PageFaults = (pInfo->PageFaultCount);
    }

    if (m_CommitCharge != pInfo->PrivatePageCount / 1024)
    {
        m_fDirty_COL_COMMITCHARGE = TRUE;
        m_CommitCharge = pInfo->PrivatePageCount / 1024;
    }

    if (m_PagedPool != pInfo->QuotaPagedPoolUsage / 1024)
    {
        m_fDirty_COL_PAGEDPOOL = TRUE;
        m_PagedPool = pInfo->QuotaPagedPoolUsage / 1024;
    }

    if (m_NonPagedPool != pInfo->QuotaNonPagedPoolUsage / 1024)
    {
        m_fDirty_COL_NONPAGEDPOOL = TRUE;
        m_NonPagedPool = pInfo->QuotaNonPagedPoolUsage / 1024;
    }

    if (m_PriClass != pInfo->BasePriority)
    {
        m_fDirty_COL_BASEPRIORITY = TRUE;
        m_PriClass = pInfo->BasePriority;
    }

    if (m_HandleCount != pInfo->HandleCount)
    {
        m_fDirty_COL_HANDLECOUNT = TRUE;
        m_HandleCount = pInfo->HandleCount;
    }

    if (m_ThreadCount != pInfo->NumberOfThreads)
    {
        m_fDirty_COL_HANDLECOUNT = TRUE;
        m_ThreadCount = pInfo->NumberOfThreads;
    }

    if (m_IoReadOperCount != pInfo->ReadOperationCount.QuadPart)
    {
        m_fDirty_COL_READOPERCOUNT = TRUE;
        m_IoReadOperCount = pInfo->ReadOperationCount.QuadPart;
    }

    if (m_IoWriteOperCount != pInfo->WriteOperationCount.QuadPart)
    {
        m_fDirty_COL_WRITEOPERCOUNT = TRUE;
        m_IoWriteOperCount = pInfo->WriteOperationCount.QuadPart;
    }

    if (m_IoOtherOperCount != pInfo->OtherOperationCount.QuadPart)
    {
        m_fDirty_COL_OTHEROPERCOUNT = TRUE;
        m_IoOtherOperCount = pInfo->OtherOperationCount.QuadPart;
    }

    if (m_IoReadXferCount != pInfo->ReadTransferCount.QuadPart)
    {
        m_fDirty_COL_READXFERCOUNT = TRUE;
        m_IoReadXferCount = pInfo->ReadTransferCount.QuadPart;
    }

    if (m_IoWriteXferCount != pInfo->WriteTransferCount.QuadPart)
    {
        m_fDirty_COL_WRITEXFERCOUNT = TRUE;
        m_IoWriteXferCount = pInfo->WriteTransferCount.QuadPart;
    }

    if (m_IoOtherXferCount != pInfo->OtherTransferCount.QuadPart)
    {
        m_fDirty_COL_OTHERXFERCOUNT = TRUE;
        m_IoOtherXferCount = pInfo->OtherTransferCount.QuadPart;
    }

    hProcess = OpenProcess( PROCESS_QUERY_INFORMATION , FALSE, m_UniqueProcessId);
    
    if (hProcess && (m_USERObjectCount != (dwTemp = GetGuiResources(hProcess, GR_USEROBJECTS))))
    {
        m_fDirty_COL_USEROBJECTS = TRUE;
        m_USERObjectCount = dwTemp;
    }

    if (hProcess && (m_GDIObjectCount != (dwTemp = GetGuiResources(hProcess, GR_GDIOBJECTS))))
    {
        m_fDirty_COL_GDIOBJECTS = TRUE;
        m_GDIObjectCount = dwTemp;
    }


    if (hProcess)
        CloseHandle(hProcess);

    if (FALSE == fUpdateOnly)
    {
        //
        // Set the process' image name.  If its NULL it could be the "Idle Process" or simply
        // a process whose image name is unknown.  In both cases we load a string resource
        // with an appropriate replacement name.
        //

        m_fDirty_COL_IMAGENAME = TRUE;

        if (pInfo->ImageName.Buffer == NULL)
        {
            // No image name, so replace it with "Unknown"

            TCHAR szTmp[MAX_PATH];
            UINT  uLen = LoadString(g_hInstance, IDS_SYSPROC, szTmp, MAX_PATH);


            m_pszImageName = new TCHAR[uLen + 1];
            if (NULL == m_pszImageName)
            {
                    return hr = E_OUTOFMEMORY;
            }

            lstrcpy(m_pszImageName, szTmp);
        }
        else
        {
            //
            // We have a valid image name, so allocate enough space and then
            // make a copy of it
            //
            m_pszImageName = new TCHAR[(pInfo->ImageName.Length / sizeof(WCHAR))+ 1];
            if (NULL == m_pszImageName)
            {
                    return hr = E_OUTOFMEMORY;
            }

            lstrcpyn(m_pszImageName, pInfo->ImageName.Buffer, (pInfo->ImageName.Length / sizeof(WCHAR)) + 1);
            m_pszImageName[(pInfo->ImageName.Length / sizeof(WCHAR))] = TEXT('\0');
        }

// #ifdef _HYDRA_

        if( IsTerminalServer( ) )
        {
            SetProcessUsername(LPFILETIME(&(pInfo->CreateTime)));
        }

// #endif

    }

    //
    // Check if this process is a WOW process.  There is some latency
    // between the time a WOW process is created and the time
    // the shared memory used by VDMEnumTaskWOWEx reflects the new
    // process and tasks.  However, once a process becomes a WOW
    // process, it is always a WOW process until it dies.
    //

    if (g_Options.m_fShow16Bit)
    {
        if ( m_fWowProcess ||
             ! m_fWowProcessTested)
        {
#if !defined (_WIN64)

            if ( ( m_pszImageName != NULL ) && ( ! _wcsicmp(m_pszImageName, TEXT("ntvdm.exe")) ) )
            {

                WOWTASKCALLBACKPARMS WowTaskCallbackParms;

                WowTaskCallbackParms.uPassCount = uPassCount;
                WowTaskCallbackParms.pProcPage = pProcPage;
                WowTaskCallbackParms.pParentProcInfo = this;
                WowTaskCallbackParms.TotalTime.QuadPart = TotalTime.QuadPart;
                WowTaskCallbackParms.TimeLeft.QuadPart = TimeDelta.QuadPart;

                if (VDMEnumTaskWOWEx(m_UniqueProcessId,
                                     WowTaskCallback,
                                     (LPARAM) &WowTaskCallbackParms))
                {
                    if ( ! m_fWowProcess )
                    {
                        m_fWowProcessTested =
                            m_fWowProcess = TRUE;
                    }

                    SetCPU( WowTaskCallbackParms.TimeLeft, TotalTime, TRUE );
                }
                else
                {
                    //
                    // We avoid calling VDMEnumTaskWOWEx if the process has an
                    // execution time of more than 10 seconds and has not so
                    // far been seen as a WOW process.
                    //

                    if (GetCPUTime() > (10 * 10 * 1000 * 1000))
                    {
                        m_fWowProcessTested = TRUE;
                    }
                }
            }
            else
            {
                m_fWowProcessTested = TRUE;
            }
#else
            m_fWowProcessTested = TRUE;
#endif
        }
    }


    //
    // If this is a sandboxed version of dllhost.exe, tack on the URL it
    // goes with to the image name
    //

    if (g_Options.m_fShowSandboxNames
        && 0 == lstrcmpi(m_pszImageName, TEXT("dllhost.exe")))
    {
        HANDLE hProcess = OpenProcess(
                                PROCESS_ALL_ACCESS, 
                                FALSE, 
                                m_UniqueProcessId);

        if (NULL != hProcess)
        {
            HANDLE hToken;

            if (OpenProcessToken(hProcess, TOKEN_READ, &hToken))
            {
                if (IsTokenRestricted(hToken))
                {
                    PSID psid = GetSiteSidFromToken(hToken);

                    if (NULL != psid)
                    {
                        TCHAR   szName[256];
                        TCHAR   szDomain[256];
                        DWORD   cchName   = sizeof(szName) / sizeof(TCHAR);
                        DWORD   cchDomain = sizeof(szDomain) / sizeof(TCHAR);
                        SID_NAME_USE use;

                        if (LookupAccountSid(
                                    NULL,
                                    psid,
                                    szName,
                                    &cchName,
                                    szDomain,
                                    &cchDomain,
                                    &use))
                        {
                            int cch = (lstrlen(m_pszImageName)
                                       + lstrlen(szName)) * sizeof(WCHAR)
                                       + sizeof(TEXT("  ()"));

                            LPTSTR pszImageName = new TCHAR[cch];

                            if (NULL != pszImageName)
                            {
                                wsprintf(
                                        pszImageName, 
                                        TEXT("%s  (%s)"),
                                        m_pszImageName,
                                        szName);

                                delete [] m_pszImageName;
                                m_pszImageName = pszImageName;
                            }
                        }
                    }

                    FreeSid(psid);
                }

                CloseHandle(hToken);
            }

            CloseHandle(hProcess);
        }
    }

    return S_OK;
}

// #ifdef _HYDRA_
//----------------------------------------------------------------
//
// No creation info
//
// Reviewed by alhen 9 - 3 - 98
//
HRESULT CProcInfo::SetProcessUsername(const FILETIME *pCreateTime)
{
    DWORD dwError = NO_ERROR;
    
    // SetProcessUsername should get called only for terminal servers.

    if( !IsTerminalServer() )
    {
        return E_FAIL;
    }

    // in case we could not get the proc address from winsta.dll
    // set the user name to error or something.

    __try
    {
        // in case of wow tasks assign username same as its parent process's

        if( IsWowTask( ) )
        {
            if( m_pWowParentProcInfo->m_pszUserName != NULL )
            {
                m_pszUserName = ( LPTSTR )new TCHAR[ lstrlen( m_pWowParentProcInfo->m_pszUserName ) + 1 ];

                if( m_pszUserName != NULL )
                {
                    lstrcpy( m_pszUserName , m_pWowParentProcInfo->m_pszUserName );

                    return S_OK;
                }
                else
                {
                    return E_OUTOFMEMORY;
                }
            }
            else
            {

                return E_FAIL;
            }
        }

        if( m_UniqueProcessId == 0 )     // this is a system idle process.
        {
            const TCHAR *szIdleProcessOwner = TEXT( "SYSTEM" );
            
            m_pszUserName = ( LPTSTR )new TCHAR[ 7 ];

            if( m_pszUserName != NULL )
            {
                lstrcpy(m_pszUserName, szIdleProcessOwner);
            }
        }
        else
        {
           
            PSID pUserSid = NULL;

            DWORD dwSize = 0;

            if( !(*gpfnWinStationGetProcessSid)( NULL , GetRealPID( ) , *pCreateTime, ( PBYTE )pUserSid , &dwSize ) )
            {
                pUserSid = ( PSID ) new BYTE[ dwSize ];

                if( pUserSid != NULL )
                {
                    if( (*gpfnWinStationGetProcessSid)( NULL , GetRealPID( ) , *pCreateTime, ( PBYTE )pUserSid , &dwSize ) )
                    {

                        if( IsValidSid( pUserSid ) )
                        {
                            TCHAR szTmpName[MAX_PATH];

                            DWORD dwTmpNameSize = MAX_PATH;

                            (*gpfnCachedGetUserFromSid)( pUserSid , szTmpName , &dwTmpNameSize );

                            m_pszUserName = ( LPTSTR )new TCHAR[ sizeof( szTmpName ) + 1 ];

                            if( m_pszUserName != NULL )
                            {
                                lstrcpy(m_pszUserName, szTmpName);
                            }
                        }
                    }

                    delete [] pUserSid;
                }
                else
                {
                    dwError = GetLastError();
                }

                
            } // this would mean that a sid of size zero was returned
        }


    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        // dprintf(TEXT("exception occured: %d",), GetExceptionCode());
        dwError = GetExceptionCode();
    }

    return HRESULT_FROM_WIN32(dwError);

}
// #endif


/*++ CProcPage::UpdateProcListview

Class Description:

    Walks the listview and checks to see if each line in the
    listview matches the corresponding entry in our process
    array.  Those which differe by PID are replaced, and those
    that need updating are updated.

    Items are also added and removed to/from the tail of the
    listview as required.

Arguments:

Return Value:

    HRESULT

Revision History:

      Nov-20-95 Davepl  Created

--*/

HRESULT CProcPage::UpdateProcListview ()
{
    HWND hListView = GetDlgItem(m_hPage, IDC_PROCLIST);

    // Stop repaints while we party on the listview

    SendMessage(hListView, WM_SETREDRAW, FALSE, 0);

    INT cListViewItems = ListView_GetItemCount(hListView);
    INT cProcArrayItems = m_pProcArray->GetSize();

    //
    // Walk the existing lines in the listview and replace/update
    // them as needed
    //

    CProcInfo * pSelected = GetSelectedProcess();



    for (int iCurrent = 0, iCurrListViewItem = 0;
          iCurrListViewItem < cListViewItems  && iCurrent < cProcArrayItems;
         iCurrent++) // for each process
    {
        LV_ITEM lvitem = { 0 };
        lvitem.mask = LVIF_PARAM | LVIF_TEXT | LVIF_STATE;
        lvitem.iItem = iCurrListViewItem;

        if (FALSE == ListView_GetItem(hListView, &lvitem))
        {
            SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
            return E_FAIL;
        }

        CProcInfo * pTmp = (CProcInfo *) lvitem.lParam;
        CProcInfo * pProc = (CProcInfo *) m_pProcArray->GetAt(iCurrent);

        if (pTmp != pProc)
        {
            // If the objects aren't the same, we need to replace this line

            lvitem.pszText = pProc->m_pszImageName;
            lvitem.lParam = (LPARAM) pProc;

            if (pProc == pSelected)
            {
                lvitem.state |= LVIS_SELECTED | LVIS_FOCUSED;
            }
            else
            {
                lvitem.state &= ~(LVIS_SELECTED | LVIS_FOCUSED);
            }

            lvitem.stateMask |= LVIS_SELECTED | LVIS_FOCUSED;

            ListView_SetItem(hListView, &lvitem);
            ListView_RedrawItems(hListView, iCurrListViewItem, iCurrListViewItem);
        }
        else if (pProc->m_fDirty)
        {
            // Same PID, but item needs updating

            ListView_RedrawItems(hListView, iCurrListViewItem, iCurrListViewItem);
            pProc->m_fDirty = 0;
        }

        iCurrListViewItem++;
    }

    //
    // We've either run out of listview items or run out of proc array
    // entries, so remove/add to the listview as appropriate
    //

    while (iCurrListViewItem < cListViewItems)
    {
        // Extra items in the listview (processes gone away), so remove them

        ListView_DeleteItem(hListView, iCurrListViewItem);
        cListViewItems--;
    }

    while (iCurrent < cProcArrayItems)
    {
        // Need to add new items to the listview (new processes appeared)

        CProcInfo * pProc = (CProcInfo *)m_pProcArray->GetAt(iCurrent++);

        LV_ITEM lvitem  = { 0 };
        lvitem.mask     = LVIF_PARAM | LVIF_TEXT;
        lvitem.iItem    = iCurrListViewItem++;
        lvitem.pszText  = pProc->m_pszImageName;
        lvitem.lParam   = (LPARAM) pProc;

        ListView_InsertItem(hListView, &lvitem);

    }

    ASSERT(iCurrListViewItem == ListView_GetItemCount(hListView));
    ASSERT(iCurrent == cProcArrayItems);

    // Let the listview paint again

    SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
    return S_OK;
}


/*++ class CProcPage::UpdateProcInfoArray

Class Description:

    Retrieves the list of process info blocks from the system,
    and runs through our array of CProcInfo items.  Items which
    already exist are updated, and those that do not are added.
    At the end, any process which has not been touched by this
    itteration of the function are considered to have completed
    and are removed from the array.

Arguments:

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

// See comments near the usage of this table below for info on why it exists

static struct
{
    size_t cbOffset;
    UINT   idString;
}
g_OffsetMap[] =
{
    { FIELD_OFFSET(CSysInfo, m_cHandles),         IDC_TOTAL_HANDLES   },
    { FIELD_OFFSET(CSysInfo, m_cThreads),         IDC_TOTAL_THREADS   },
    { FIELD_OFFSET(CSysInfo, m_cProcesses),       IDC_TOTAL_PROCESSES },
    { FIELD_OFFSET(CSysInfo, m_dwPhysicalMemory), IDC_TOTAL_PHYSICAL  },
    { FIELD_OFFSET(CSysInfo, m_dwPhysAvail),      IDC_AVAIL_PHYSICAL  },
    { FIELD_OFFSET(CSysInfo, m_dwFileCache),      IDC_FILE_CACHE      },
    { FIELD_OFFSET(CSysInfo, m_dwCommitTotal),    IDC_COMMIT_TOTAL    },
    { FIELD_OFFSET(CSysInfo, m_dwCommitLimit),    IDC_COMMIT_LIMIT    },
    { FIELD_OFFSET(CSysInfo, m_dwCommitPeak),     IDC_COMMIT_PEAK     },
    { FIELD_OFFSET(CSysInfo, m_dwKernelPaged),    IDC_KERNEL_PAGED    },
    { FIELD_OFFSET(CSysInfo, m_dwKernelNP),       IDC_KERNEL_NONPAGED },
    { FIELD_OFFSET(CSysInfo, m_dwKernelTotal),    IDC_KERNEL_TOTAL    },
};

HRESULT CProcPage::UpdateProcInfoArray()
{
    HRESULT  hr;
    INT      i;
    INT      iField;
    ULONG    cbOffset   = 0;
    CSysInfo SysInfoTemp;
    NTSTATUS Status;

    SYSTEM_BASIC_INFORMATION        BasicInfo;
    PSYSTEM_PROCESS_INFORMATION     pCurrent;
    SYSTEM_PERFORMANCE_INFORMATION  PerfInfo;
    SYSTEM_FILECACHE_INFORMATION    FileCache;

    LARGE_INTEGER TotalTime = {0,0};
    LARGE_INTEGER LastTotalTime = {0,0};

    //
    // Pass-count for this function.  It ain't thread-safe, of course, but I
    // can't imagine a scenario where we'll have mode than one thread running
    // through this (the app is currently single threaded anyway).  If we
    // overflow LARGE_INTEGER updates, I'll already be long gone, so don't bug me.
    //

    static LARGE_INTEGER uPassCount = {0,0};

    //
    // Get some non-process specific info, like memory status
    //

    Status = NtQuerySystemInformation(
                SystemBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
             );

    if (!NT_SUCCESS(Status))
    {
        return E_FAIL;
    }

    SysInfoTemp.m_dwPhysicalMemory = BasicInfo.NumberOfPhysicalPages * 
                                          (BasicInfo.PageSize / 1024);

    Status = NtQuerySystemInformation(
                SystemPerformanceInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL
                );

    if (!NT_SUCCESS(Status))
    {
        return E_FAIL;
    }

    SysInfoTemp.m_dwPhysAvail   = PerfInfo.AvailablePages    * (g_BasicInfo.PageSize / 1024);
    SysInfoTemp.m_dwCommitTotal = PerfInfo.CommittedPages    * (g_BasicInfo.PageSize / 1024);
    SysInfoTemp.m_dwCommitLimit = PerfInfo.CommitLimit       * (g_BasicInfo.PageSize / 1024);
    SysInfoTemp.m_dwCommitPeak  = PerfInfo.PeakCommitment    * (g_BasicInfo.PageSize / 1024);
    SysInfoTemp.m_dwKernelPaged = PerfInfo.PagedPoolPages    * (g_BasicInfo.PageSize / 1024);
    SysInfoTemp.m_dwKernelNP    = PerfInfo.NonPagedPoolPages * (g_BasicInfo.PageSize / 1024);
    SysInfoTemp.m_dwKernelTotal = SysInfoTemp.m_dwKernelNP + SysInfoTemp.m_dwKernelPaged;

    g_MEMMax = SysInfoTemp.m_dwCommitLimit;

    Status = NtQuerySystemInformation(
                SystemFileCacheInformation,
                &FileCache,
                sizeof(FileCache),
                NULL
                );

    if (!NT_SUCCESS(Status))
    {
        return E_FAIL;
    }

    SysInfoTemp.m_dwFileCache = FileCache.CurrentSizeIncludingTransitionInPages * (g_BasicInfo.PageSize / 1024);

    //
    // Read the process info structures into the flat buffer
    //

    hr = GetProcessInfo();
    if (FAILED(hr))
    {
        goto done;
    }

    //
    // First walk all of the process info blocks and sum their times, so that we can 
    // calculate a CPU usage ratio (%) for each individual process
    //

    cbOffset = 0;
    do
    {
        CProcInfo * pOldProcInfo;
        pCurrent = (PSYSTEM_PROCESS_INFORMATION)&((LPBYTE)m_pvBuffer)[cbOffset];
        ASSERT( FALSE == IsBadReadPtr((LPVOID)pCurrent, sizeof(PSYSTEM_PROCESS_INFORMATION)));

        if (pCurrent->UniqueProcessId == NULL && pCurrent->NumberOfThreads == 0)
        {
            // Zombie process, just skip it

            goto next;
        }

        pOldProcInfo = FindProcInArrayByPID(m_pProcArray, PtrToUlong(pCurrent->UniqueProcessId));
        if (pOldProcInfo)
        {
            if (pOldProcInfo->GetCPUTime() > pCurrent->KernelTime.QuadPart + pCurrent->UserTime.QuadPart)
            {
                // If CPU has gone DOWN, its because the PID has been reused, so invalidate this
                // CProcInfo such that it is removed and the new one added

                pOldProcInfo->Invalidate();
                dprintf(TEXT("Invalidating %08x\n"), pOldProcInfo);
                goto next;
            }
            else if (pCurrent->UniqueProcessId == 0 &&
                     pCurrent->KernelTime.QuadPart == 0 && 
                     pCurrent->UserTime.QuadPart == 0)
            {
                dprintf(TEXT("System idle process has 0 times\n"));
                pOldProcInfo->Invalidate();
                goto next;
            }
            else
            {
                LastTotalTime.QuadPart += pOldProcInfo->GetCPUTime();
            }
        }

        TotalTime.QuadPart += pCurrent->KernelTime.QuadPart + pCurrent->UserTime.QuadPart;
    
        SysInfoTemp.m_cHandles += pCurrent->HandleCount;
        SysInfoTemp.m_cThreads += pCurrent->NumberOfThreads;
        SysInfoTemp.m_cProcesses++;

        next:

        cbOffset += pCurrent->NextEntryOffset;

        //#ifdef _HYDRA_
        // if current session id is not set yet, set it now
        //
        // REVIEWER:  Previous dev didnot document this, but taskmgr session id
        // is cached so that when the user deselects "show all the processes", only
        // processes with session id's equal to taskmgr session id are listed
        // --alhen

        if( ( GetCurrentSessionID() == -1 ) && ( PtrToUlong(pCurrent->UniqueProcessId) == GetCurrentProcessId( ) ) )
        {
            SetCurrentSessionID( ( DWORD )pCurrent->SessionId );
        }
        // #endif

    } while (pCurrent->NextEntryOffset);


    LARGE_INTEGER TimeDelta;
    TimeDelta.QuadPart = TotalTime.QuadPart - LastTotalTime.QuadPart;

    ASSERT(TimeDelta.QuadPart >= 0);

    // Update the global count (visible to the status bar)

    g_cProcesses = SysInfoTemp.m_cProcesses;

    //
    // We have a number of text fields in the dialog that are based on counts we accumulate
    // here.  Rather than painting all of the time, we only change the ones whose values have
    // really changed.  We have a table up above of the offsets into the CSysInfo object
    // where these values live (the same offset in the real g_SysInfo object and the temp
    // working copy, of course), and what control ID they correspond to.  We then loop through
    // and compare each real one to the temp working copy, updating as needed.  Hard to
    // read, but smaller than a dozen if() statements.
    //

    // BUGBUG Gross and dirty hack
    //
    // At the last moment we moved all these controls to the performance page.  They're
    // sort of inextricably tied to the data collected on this page, so as a temp measure,
    // I have this (the process page) update the controls on the performance page.

    extern CPage * g_pPages[];

    if (g_pPages[2])
    {
        for (iField = 0; iField < ARRAYSIZE(g_OffsetMap); iField++)
        {
            DWORD * pdwRealCopy = (DWORD *)(((LPBYTE)&m_SysInfo)   + g_OffsetMap[iField].cbOffset);
            DWORD * pdwTempCopy = (DWORD *)(((LPBYTE)&SysInfoTemp) + g_OffsetMap[iField].cbOffset);

//            if (*pdwRealCopy != *pdwTempCopy)
            {
                *pdwRealCopy = *pdwTempCopy;

                TCHAR szText[32];
                wsprintf(szText, TEXT("%d"), *pdwRealCopy);

                HWND hPage = g_pPages[2]->GetPageWindow();
                
                // Updates can come through before page is created, so verify
                // that it exists before we party on its children

                if (hPage)
                {
                    SetWindowText(GetDlgItem(hPage, g_OffsetMap[iField].idString), szText);
                }
            }
        }
    }

    //
    // Now walk the process info blocks again and refresh the CProcInfo array for each
    // individual process
    //

    cbOffset = 0;
    do
    {
        
        //
        // Grab a PROCESS_INFORMATION struct from the buffer
        //

        pCurrent = (PSYSTEM_PROCESS_INFORMATION)&((LPBYTE)m_pvBuffer)[cbOffset];
        ASSERT( FALSE == IsBadReadPtr((LPVOID)pCurrent, sizeof(PSYSTEM_PROCESS_INFORMATION)));

        if (pCurrent->UniqueProcessId == NULL && pCurrent->NumberOfThreads == 0)
        {
            // Zombie process, just skip it

            goto nextprocinfo;
        }

        //
        // This is really ugly, but... NtQuerySystemInfo has too much latency, and if you
        // change a process' priority, you don't see it reflected right away.  And, if you
        // don't have autoupdate on, you never do.  So, we use GetPriorityClass() to get
        // the value instead.  This means BasePriority is now the pri class, not the pri value.
        //

        if (pCurrent->UniqueProcessId)
        {
            HANDLE hProcess;
            hProcess = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, PtrToUlong(pCurrent->UniqueProcessId) );
            DWORD dwPriClass;
            dwPriClass = 0;

            if (hProcess) 
            {
                dwPriClass = GetPriorityClass(hProcess);
                if (dwPriClass)
                {
                    pCurrent->BasePriority = dwPriClass;
                }
                CloseHandle( hProcess );
            }

            if (NULL == hProcess || dwPriClass == 0)
            {
                // We're not allowed to open this process, so convert what NtQuerySystemInfo
                // gave us into a priority class... its the next bexthing

                if (pCurrent->BasePriority <= 4)
                {
                    pCurrent->BasePriority = IDLE_PRIORITY_CLASS;
                }
                else if (pCurrent->BasePriority <= 6)
                {
                    pCurrent->BasePriority = BELOW_NORMAL_PRIORITY_CLASS;
                }
                else if (pCurrent->BasePriority <= 8)
                {
                    pCurrent->BasePriority = NORMAL_PRIORITY_CLASS;
                }
                else if (pCurrent->BasePriority <=  10)
                {
                    pCurrent->BasePriority = ABOVE_NORMAL_PRIORITY_CLASS;
                }
                else if (pCurrent->BasePriority <=  13)
                {
                    pCurrent->BasePriority = HIGH_PRIORITY_CLASS;
                }
                else
                {
                    pCurrent->BasePriority = REALTIME_PRIORITY_CLASS;
                }
            }
        }

        //
        // Try to find an existing CProcInfo instance which corresponds to this process
        //

        CProcInfo * pProcInfo;
        pProcInfo = FindProcInArrayByPID(m_pProcArray, PtrToUlong(pCurrent->UniqueProcessId));

        if (NULL == pProcInfo)
        {
            //
            // We don't already have this process in our array, so create a new one
            // and add it to the array
            //

            pProcInfo = new CProcInfo;
            if (NULL == pProcInfo)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            hr = pProcInfo->SetData(TimeDelta,
                                    pCurrent,
                                    uPassCount,
                                    this,
                                    FALSE);

            if (FAILED(hr) || FALSE == m_pProcArray->Add(pProcInfo))
            {
                delete pProcInfo;
                goto done;
            }
        }
        else
        {
            //
            // This process already existed in our array, so update its info
            //

            hr = pProcInfo->SetData(TimeDelta,
                                    pCurrent,
                                    uPassCount,
                                    this,
                                    TRUE);
            if (FAILED(hr))
            {
                goto done;
            }
        }

        nextprocinfo:

        cbOffset += pCurrent->NextEntryOffset;

    } while (pCurrent->NextEntryOffset);

    //
    // Run through the CProcInfo array and remove anyone that hasn't been touched
    // by this pass through this function (which indicates the process is no
    // longer alive)
    //

    i = 0;
    while (i < m_pProcArray->GetSize())
    {
        CProcInfo * pProcInfo = (CProcInfo *)(m_pProcArray->GetAt(i));
        ASSERT(pProcInfo);

        //
        // If passcount doesn't match, delete the CProcInfo instance and remove
        // its pointer from the array.  Note that we _don't_ increment the index
        // if we remove an element, since the next element would now live at
        // the current index after the deletion
        //

        if (pProcInfo->m_uPassCount.QuadPart != uPassCount.QuadPart)
        {
            delete pProcInfo;
            m_pProcArray->RemoveAt(i, 1);
        }
        else
        {
            i++;
        }
    }

done:

    ResortArray(&m_pProcArray);
    uPassCount.QuadPart++;

    return hr;
}

/*++ CPerfPage::SizeProcPage

Routine Description:

    Sizes its children based on the size of the
    tab control on which it appears.  

Arguments:

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

void CProcPage::SizeProcPage()
{
    // Get the coords of the outer dialog

    RECT rcParent;
    GetClientRect(m_hPage, &rcParent);

    HDWP hdwp = BeginDeferWindowPos(10);

    // Calc the deltas in the x and y positions that we need to
    // move each of the child controls

    RECT rcTerminate;
    HWND hwndTerminate = GetDlgItem(m_hPage, IDC_TERMINATE);
    GetWindowRect(hwndTerminate, &rcTerminate);
    MapWindowPoints(HWND_DESKTOP, m_hPage, (LPPOINT) &rcTerminate, 2);

    INT dx = ((rcParent.right - g_DefSpacing * 2) - rcTerminate.right);
    INT dy = ((rcParent.bottom - g_DefSpacing * 2) - rcTerminate.bottom);

    // Move the EndProcess button

    DeferWindowPos(hdwp, hwndTerminate, NULL, 
                     rcTerminate.left + dx, 
                     rcTerminate.top + dy,
                     0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    // _HYDRA_
    HWND hwndShowall = GetDlgItem(m_hPage, IDC_SHOWALL);

    if( IsWindow( hwndShowall ) )
    {
        if( IsTerminalServer() )
        {
            RECT rcShowall;
            
            GetWindowRect(hwndShowall, &rcShowall);
            
            MapWindowPoints(HWND_DESKTOP, m_hPage, (LPPOINT) &rcShowall, 2);
            
            DeferWindowPos(hdwp, hwndShowall, NULL, rcShowall.left, rcShowall.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        else
        {
            // this window must be hidden.
        
            ShowWindow(hwndShowall, SW_HIDE);
        }
    }
    
    // _HYDRA_

    // Size the listbox

    HWND hwndListbox = GetDlgItem(m_hPage, IDC_PROCLIST);
    RECT rcListbox;
    GetWindowRect(hwndListbox, &rcListbox);
    MapWindowPoints(HWND_DESKTOP, m_hPage, (LPPOINT) &rcListbox, 2);
    DeferWindowPos(hdwp, hwndListbox, NULL,
                        0, 0,
                        rcTerminate.right - rcListbox.left + dx,
                        rcTerminate.top - rcListbox.top + dy - g_DefSpacing,
                        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    EndDeferWindowPos(hdwp);
}

/*++ CProcPage::HandleTaskManNotify

Routine Description:

    Processes WM_NOTIFY messages received by the procpage dialog
    
Arguments:

    hWnd    - Control that generated the WM_NOTIFY
    pnmhdr  - Ptr to the NMHDR notification stucture

Return Value:

    BOOL "did we handle it" code

Revision History:

      Nov-20-95 Davepl  Created

--*/
INT CProcPage::HandleProcPageNotify(HWND hWnd, LPNMHDR pnmhdr)
{
    switch(pnmhdr->code)
    {
        case LVN_COLUMNCLICK:
        {
            // User clicked a header control, so set the sort column.  If its the
            // same as the current sort column, just invert the sort direction in
            // the column.  Then resort the task array

            const NM_LISTVIEW * pnmv = (const NM_LISTVIEW *) pnmhdr;
            
            if (g_iProcSortColumnID == g_Options.m_ActiveProcCol[pnmv->iSubItem])
            {
                g_iProcSortDirection  *= -1;
            }
            else
            {
                g_iProcSortColumnID = g_Options.m_ActiveProcCol[pnmv->iSubItem];
                g_iProcSortDirection  = -1;
            }
            ResortArray(&m_pProcArray);
            TimerEvent();
            break;
        }

        case LVN_ITEMCHANGED:
        {
            const NM_LISTVIEW * pnmv = (const NM_LISTVIEW *) pnmhdr;
            if (pnmv->uChanged & LVIF_STATE)
            {
                UINT cSelected = ListView_GetSelectedCount(GetDlgItem(m_hPage, IDC_PROCLIST));
                EnableWindow(GetDlgItem(m_hPage, IDC_TERMINATE), cSelected ? TRUE : FALSE);   
            }
            break;
        }

        case LVN_GETDISPINFO:
        {
            LV_ITEM * plvitem = &(((LV_DISPINFO *) pnmhdr)->item);
            
            // Listview needs a text string

            if (plvitem->mask & LVIF_TEXT)
            {
                COLUMNID columnid = (COLUMNID) g_Options.m_ActiveProcCol[plvitem->iSubItem];
                const CProcInfo  * pProcInfo   = (const CProcInfo *)   plvitem->lParam;

                //
                // Most columns are blank for WOW tasks.
                //

                if (pProcInfo->IsWowTask() &&
                    columnid != COL_IMAGENAME &&
                    columnid != COL_BASEPRIORITY &&
                    columnid != COL_THREADCOUNT &&
                    columnid != COL_CPUTIME &&
//#ifdef _HYDRA_
                    columnid != COL_USERNAME &&
                    columnid != COL_SESSIONID &&
//#endif
                    columnid != COL_CPU) {

                    plvitem->pszText[0] = 0;
                    goto done;
                }

                switch(columnid)
                {
                    case COL_PID:
                        wsprintf(plvitem->pszText, TEXT("%d"), (ULONG) (pProcInfo->m_UniqueProcessId));
                        break;

//#ifdef _HYDRA_
                    case COL_USERNAME:

                        if( pProcInfo->m_pszUserName )
                        {
                            lstrcpyn(plvitem->pszText, pProcInfo->m_pszUserName, plvitem->cchTextMax);

                        }
                        
                        break;

                    case COL_SESSIONID:

                        wsprintf( plvitem->pszText, TEXT( "%d" ) , pProcInfo->m_SessionId );

                        break;

//#endif
                    case COL_CPU:
                        wsprintf(plvitem->pszText, TEXT("%02d %"), pProcInfo->m_DisplayCPU);
                        break;

                    case COL_IMAGENAME:
                        lstrcpyn(plvitem->pszText, pProcInfo->m_pszImageName, plvitem->cchTextMax);
                        //plvitem->mask |= LVIF_DI_SETITEM;
                        break;
                    
                    case COL_CPUTIME:
                    {
                        TIME_FIELDS TimeOut;
                        
                        // BUGBUG This Rtl should take a const...

                        RtlTimeToElapsedTimeFields ( (LARGE_INTEGER *)&(pProcInfo->m_DisplayCPUTime), &TimeOut);
                        TimeOut.Hour += (SHORT)(TimeOut.Day * 24);
                        
                        wsprintf(plvitem->pszText, 
                                 TEXT("%2d%s%02d%s%02d"), 
                                 TimeOut.Hour, 
                                 m_szTimeSep, 
                                 TimeOut.Minute, 
                                 m_szTimeSep, 
                                 TimeOut.Second);
                        break;
                    }
                    case COL_MEMUSAGE:
                        Int64ToCommaSepKString(LONGLONG(pProcInfo->m_MemUsage), plvitem->pszText, plvitem->cchTextMax);
                        break;

                    case COL_MEMUSAGEDIFF:
                        Int64ToCommaSepKString(LONGLONG(pProcInfo->m_MemDiff), plvitem->pszText, plvitem->cchTextMax);
                        break;

                    case COL_MEMPEAK:
                        Int64ToCommaSepKString(LONGLONG(pProcInfo->m_MemPeak), plvitem->pszText, plvitem->cchTextMax);
                        break;

                    case COL_PAGEFAULTS:
                        Int64ToCommaSepString(LONGLONG(pProcInfo->m_PageFaults), plvitem->pszText, plvitem->cchTextMax);
                        break;

                    case COL_PAGEFAULTSDIFF:
                        Int64ToCommaSepString(LONGLONG(pProcInfo->m_PageFaultsDiff), plvitem->pszText, plvitem->cchTextMax);
                        break;

                    case COL_COMMITCHARGE:
                        Int64ToCommaSepKString(LONGLONG(pProcInfo->m_CommitCharge), plvitem->pszText, plvitem->cchTextMax);
                        break;

                    case COL_PAGEDPOOL:
                        Int64ToCommaSepKString(LONGLONG(pProcInfo->m_PagedPool), plvitem->pszText, plvitem->cchTextMax);
                        break;
                        
                    case COL_NONPAGEDPOOL:
                        Int64ToCommaSepKString(LONGLONG(pProcInfo->m_NonPagedPool), plvitem->pszText, plvitem->cchTextMax);
                        break;

                    case COL_BASEPRIORITY:
                    {
                        LPCTSTR pszClass = NULL;

                        switch(pProcInfo->m_PriClass)
                        {
                            case REALTIME_PRIORITY_CLASS:
                                pszClass = g_szRealtime;
                                break;

                            case HIGH_PRIORITY_CLASS:
                                pszClass = g_szHigh;
                                break;

                            case ABOVE_NORMAL_PRIORITY_CLASS:
                                pszClass = g_szAboveNormal;
                                break;

                            case NORMAL_PRIORITY_CLASS:
                                pszClass = g_szNormal;
                                break;

                            case BELOW_NORMAL_PRIORITY_CLASS:
                                pszClass = g_szBelowNormal;
                                break;

                            case IDLE_PRIORITY_CLASS:
                                pszClass = g_szLow;
                                break;

                            default:
                                pszClass = g_szUnknown;
                                break;
                        }

                        lstrcpyn(plvitem->pszText, pszClass, plvitem->cchTextMax);
                        break;
                    }

                    case COL_HANDLECOUNT:
                        Int64ToCommaSepString(LONGLONG(pProcInfo->m_HandleCount), plvitem->pszText, plvitem->cchTextMax);
                        break;

                    case COL_THREADCOUNT:
                        Int64ToCommaSepString(LONGLONG(pProcInfo->m_ThreadCount), plvitem->pszText, plvitem->cchTextMax);
                        break;

                    case COL_USEROBJECTS:
                        Int64ToCommaSepString(LONGLONG(pProcInfo->m_USERObjectCount), plvitem->pszText, plvitem->cchTextMax);
                        break;

                    case COL_GDIOBJECTS:
                        Int64ToCommaSepString(LONGLONG(pProcInfo->m_GDIObjectCount), plvitem->pszText, plvitem->cchTextMax);
                        break;

                    case COL_READOPERCOUNT:
                        Int64ToCommaSepString(pProcInfo->m_IoReadOperCount, plvitem->pszText, plvitem->cchTextMax);
                        break;

                    case COL_WRITEOPERCOUNT:
                        Int64ToCommaSepString(pProcInfo->m_IoWriteOperCount, plvitem->pszText, plvitem->cchTextMax);
                        break;

                    case COL_OTHEROPERCOUNT:
                        Int64ToCommaSepString(pProcInfo->m_IoOtherOperCount, plvitem->pszText, plvitem->cchTextMax);
                        break;

                    case COL_READXFERCOUNT:
                        Int64ToCommaSepString(pProcInfo->m_IoReadXferCount, plvitem->pszText, plvitem->cchTextMax);
                        break;

                    case COL_WRITEXFERCOUNT:
                        Int64ToCommaSepString(pProcInfo->m_IoWriteXferCount, plvitem->pszText, plvitem->cchTextMax);
                        break;

                    case COL_OTHERXFERCOUNT:
                        Int64ToCommaSepString(pProcInfo->m_IoOtherXferCount, plvitem->pszText, plvitem->cchTextMax);
                        break;

                    default:
                        Assert( 0 && "Unknown listview subitem" );
                        break;

                } // end switch(columnid)

            } // end LVIF_TEXT case

        } // end LVN_GETDISPINFO case
    
    } // end switch(pnmhdr->code)

done:
    return 1;
}



/*++ CProcPage::TimerEvent

Routine Description:

    Called by main app when the update time fires
    
Arguments:

Return Value:

Revision History:

      Nov-20-95 Davepl  Created

--*/

void CProcPage::TimerEvent()
{
    // REVIEW (DavePl)
    // We might want to optimize the amount of calculation we do when
    // we are iconic, but we still need to track the deltas (mem usage,
    // faults, etc) so might as well just calc it all.  Listview won't
    // repaint anyway until its visible, which is the real work

    if (FALSE == m_fPaused)
    {
        // We only process updates when the display is not paused, ie:
        // not during trackpopupmenu loop
        UpdateProcInfoArray();
        UpdateProcListview();
    }
}


/*++ CProcPage::HandleProcListContextMenu

Routine Description:

    Handles right-clicks (context menu) in the proc list
    
Arguments:

    xPos, yPos  - coords of where the click occurred

Return Value:

Revision History:

      Nov-22-95 Davepl  Created

--*/

void CProcPage::HandleProcListContextMenu(INT xPos, INT yPos)
{
    HWND hTaskList = GetDlgItem(m_hPage, IDC_PROCLIST);

    INT iItem = ListView_GetNextItem(hTaskList, -1, LVNI_SELECTED);

    if (-1 != iItem)
    {
        if (0xFFFF == LOWORD(xPos) && 0xFFFF == LOWORD(yPos))
        {
            RECT rcItem;
            ListView_GetItemRect(hTaskList, iItem, &rcItem, LVIR_ICON);
            MapWindowRect(hTaskList, NULL, &rcItem);
            xPos = rcItem.right;
            yPos = rcItem.bottom;
        }


        HMENU hPopup = LoadPopupMenu(g_hInstance, IDR_PROC_CONTEXT);
        if (hPopup)
        {
            if (hPopup && SHRestricted(REST_NORUN))
            {
                DeleteMenu(hPopup, IDM_RUN, MF_BYCOMMAND);
            }

            CProcInfo * pProc = GetSelectedProcess();
            if (NULL == pProc)
            {
                return;
            }

            //
            // If no debugger is installed or it's a 16-bit app
            // ghost the debug menu item
            //

            if (NULL == m_pszDebugger || pProc->IsWowTask())
            {
                EnableMenuItem(hPopup, IDM_PROC_DEBUG, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
            }

            //
            // If it's a 16-bit task grey the priority choices
            //

            if (pProc->IsWowTask())
            {
                EnableMenuItem(hPopup, IDM_PROC_REALTIME,   MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                EnableMenuItem(hPopup, IDM_PROC_ABOVENORMAL,MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                EnableMenuItem(hPopup, IDM_PROC_NORMAL,     MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                EnableMenuItem(hPopup, IDM_PROC_BELOWNORMAL,MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                EnableMenuItem(hPopup, IDM_PROC_HIGH,       MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                EnableMenuItem(hPopup, IDM_PROC_LOW,        MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
            }

            //
            // If not an MP machine, remove the affinity option
            //

            if (1 == g_cProcessors || pProc->IsWowTask())
            {
                DeleteMenu(hPopup, IDM_AFFINITY, MF_BYCOMMAND);
            }
                    
            DWORD dwPri   = pProc->m_PriClass;
            INT   idCheck = 0;

            // These constants are listed in the SDK

            if (dwPri == IDLE_PRIORITY_CLASS)
            {
                idCheck = IDM_PROC_LOW;    
            }
            else if (dwPri == BELOW_NORMAL_PRIORITY_CLASS)
            {
                idCheck = IDM_PROC_BELOWNORMAL;
            }
            else if (dwPri == NORMAL_PRIORITY_CLASS)
            {
                idCheck = IDM_PROC_NORMAL;
            }
            else if (dwPri == ABOVE_NORMAL_PRIORITY_CLASS)
            {
                idCheck = IDM_PROC_ABOVENORMAL;
            }
            else if (dwPri == HIGH_PRIORITY_CLASS)
            {
                idCheck = IDM_PROC_HIGH;
            }
            else
            {
                Assert(dwPri == REALTIME_PRIORITY_CLASS);
                idCheck = IDM_PROC_REALTIME;
            }

            // Check the appropriate radio menu for this process' priority class

            CheckMenuRadioItem(hPopup, IDM_PROC_REALTIME, IDM_PROC_LOW, idCheck, MF_BYCOMMAND);

            m_fPaused = TRUE;
            g_fInPopup = TRUE;
            TrackPopupMenuEx(hPopup, 0, xPos, yPos, m_hPage, NULL);
            g_fInPopup = FALSE;
            m_fPaused = FALSE;
            
            //
            // If one of the context menu actions (ie: Kill) requires that the display
            // get updated, do it now
            //

            DestroyMenu(hPopup);
        }
    }
}

/*++ AffinityDlgProc

Routine Description:

    Dialog procedure for the affinity mask dialog.  Basically just tracks 32 check
    boxes that represent the processors
    
Arguments:

    standard dlgproc fare 0 - initial lParam is pointer to affinity mask

Revision History:

      Jan-17-96 Davepl  Created

--*/

INT_PTR CALLBACK AffinityDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static DWORD * pdwAffinity = NULL;      // One of the joys of single threadedness

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pdwAffinity = (DWORD *) lParam;

            for (int i = 0; i < MAX_PROCESSOR; i++)
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_CPU0 + i), i < g_cProcessors);
                CheckDlgButton(hwndDlg, IDC_CPU0 + i, i < g_cProcessors && ((*pdwAffinity & (1 << i)) != 0));
            }
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    break;

                case IDOK:
                {
                    *pdwAffinity = 0;
                    for (int i = 0; i < g_cProcessors; i++)
                    {
                        if (IsDlgButtonChecked(hwndDlg, IDC_CPU0 + i))
                        {
                            *pdwAffinity |= 1 << i;
                        }
                    }

                    if (*pdwAffinity == 0)
                    {
                        // Can't set affinity to "none"

                        TCHAR szTitle[MAX_PATH];
                        TCHAR szBody[MAX_PATH];

                        if (0 == LoadString(g_hInstance, IDS_INVALIDOPTION, szTitle, ARRAYSIZE(szTitle)) ||
                            0 == LoadString(g_hInstance, IDS_NOAFFINITYMASK, szBody,  ARRAYSIZE(szBody)))
                        {
                            break;
                        }
                        MessageBox(hwndDlg, szBody, szTitle, MB_ICONERROR);
                        break;
                    }
                    EndDialog(hwndDlg, IDOK);
                }
            }
        }
    }
    return FALSE;
}

/*++ SetAffinity

Routine Description:

    Puts up a dialog that lets the user adjust the processor affinity
    for a process
    
Arguments:

    pid - process Id of process to modify

Return Value:

    boolean success

Revision History:

      Jan-17-96 Davepl  Created

--*/

BOOL CProcPage::SetAffinity(DWORD pid)
{
    BOOL fSuccess = FALSE;

    // REVIEW (Davepl) may only need get/set info access here

    HANDLE hProcess = OpenProcess( PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION, FALSE, pid );
    if (hProcess) 
    {
        DWORD_PTR dwAffinity;
        DWORD_PTR dwUnusedSysAfin;
        if (GetProcessAffinityMask(hProcess, &dwAffinity, &dwUnusedSysAfin))
        {
            if (IDOK == DialogBoxParam(g_hInstance, 
                                       MAKEINTRESOURCE(IDD_AFFINITY), 
                                       m_hPage, 
                                       AffinityDlgProc, 
                                       (LPARAM) &dwAffinity))
            {
                if (SetProcessAffinityMask(hProcess, dwAffinity))
                    fSuccess = TRUE;
            }
            else
                fSuccess = TRUE;        // Cancel, so no failure
        }
   
        CloseHandle(hProcess);
    }

    if (!fSuccess)
    {
        DWORD dwError = GetLastError();
        DisplayFailureMsg(m_hPage, IDS_CANTSETAFFINITY, dwError);
    }
   
    return fSuccess;
}

BOOL CProcPage::IsSystemProcess(DWORD pid, CProcInfo * pProcInfo)
{
    // We don't allow the following set of critical system processes to be terminated,
    // since the system would bugcheck immediately, no matter who you are.

    static const LPCTSTR apszCantKill[] =
    {
        TEXT("csrss.exe"), TEXT("winlogon.exe"), TEXT("smss.exe"), TEXT("services.exe")
    };

    // if they pass in a pProcInfo we'll use it, otherwise find it ourselves
    if (!pProcInfo)
        pProcInfo = FindProcInArrayByPID(m_pProcArray, pid);
    if (!pProcInfo)
        return FALSE;

    for (int i = 0; i < ARRAYSIZE(apszCantKill); ++i)
    {
        if (0 == lstrcmpi(pProcInfo->m_pszImageName, apszCantKill[i]))
        {
            TCHAR szTitle[MAX_PATH];
            TCHAR szBody[MAX_PATH];

            if (0 != LoadString(g_hInstance, IDS_CANTKILL, szTitle, ARRAYSIZE(szTitle)) &&
                0 != LoadString(g_hInstance, IDS_KILLSYS,  szBody,  ARRAYSIZE(szBody)))
            {
                MessageBox(m_hPage, szBody, szTitle, MB_ICONEXCLAMATION | MB_OK);
            }
            return TRUE;
        }
    }
    return FALSE;
}

/*++ KillProcess

Routine Description:

    Kills a process 
    
Arguments:

    pid - process Id of process to kill

Return Value:

Revision History:

      Nov-22-95 Davepl  Created

--*/

BOOL CProcPage::KillProcess(DWORD pid, BOOL bBatchKill)
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // Special-case killing WOW tasks
    //

    CProcInfo * pProcInfo;
    pProcInfo = FindProcInArrayByPID(m_pProcArray, pid);

    if (NULL == pProcInfo)
        return FALSE;

    if (IsSystemProcess(pid, pProcInfo))
        return FALSE;

    // Grab info from pProcInfo (because once we call QuickConfirm(), the
    // pProcInfo pointer may be invalid)
    INT_PTR fWowTask = pProcInfo->IsWowTask();
    DWORD dwRealPID = pProcInfo->GetRealPID();
    WORD hTaskWow = pProcInfo->m_htaskWow;

    // OK so far, now confirm that the user really wants to do this.

    if (!bBatchKill && (IDYES != QuickConfirm(IDS_WARNING, IDS_KILL)))
    {
        return FALSE;
    }

    // We can't use this pointer after QuickConfirm() is called.
    // NULL it out to prevent subtle bugs.
    pProcInfo = NULL;

    
    if (fWowTask) {

        HWND hwnd = NULL;
        BOOL fDone = FALSE;

#if defined (_WIN64)
        return FALSE;
#else
        return VDMTerminateTaskWOW(dwRealPID, hTaskWow);
#endif
    }

    if( !IsTerminalServer( ) )
    {

    
        HANDLE hProcess = OpenProcess( PROCESS_TERMINATE, FALSE, pid );
        if (hProcess) 
        {
            if (FALSE == TerminateProcess( hProcess, 1 )) 
            {
                dwError = GetLastError();
                dprintf(TEXT("Can't terminate process: %08x\n"), dwError);
            }
            else
            {
                TimerEvent();
            }
            CloseHandle( hProcess );
        }
        else
        {
            dwError = GetLastError();
        }
    
    } else {

        // 
        // This change is neccessary to allow admins to terminate cross session
        // processes.
        // Admins are never the owner of a process - it is always owned by the 
        // process that it is running as.  If we want to open the process for 
        // terminate, either we must open it with WRITE_DAC first to change the 
        // security descriptor, then open it for terminate, or open it with 
        // debug privilege enabled. The latter is done through this RPC call into
        // Termsrv.exe



        if ( !gpfnWinStationTerminateProcess( (HANDLE)NULL, pid, 0 ) ) {

                dwError = GetLastError();
        }
    }

    if (ERROR_SUCCESS != dwError)
    {
        DisplayFailureMsg(m_hPage, IDS_CANTKILL, dwError);
        return FALSE;
    }
    else
    {
        return TRUE;
    }

}

/*++ AttachDebugger

Routine Description:

    Attaches the debugger listed in the AeDebug reg key to the specified
    running process
    
Arguments:

    pid - process Id of process to debug

Return Value:

Revision History:

      Nov-27-95 Davepl  Created

--*/

BOOL CProcPage::AttachDebugger(DWORD pid)
{
    DWORD dwError = ERROR_SUCCESS;

    if (IDYES != QuickConfirm(IDS_WARNING, IDS_DEBUG))
    {
        return FALSE;
    }

    TCHAR szCmdline[MAX_PATH * 2];

    wsprintf(szCmdline, TEXT("%s -p %ld"), m_pszDebugger, pid);

    STARTUPINFO sinfo =
    {
        sizeof(STARTUPINFO),
    };
    PROCESS_INFORMATION pinfo;

    if (FALSE == CreateProcess(NULL, //m_pszDebugger,
                               szCmdline,
                               NULL,
                               NULL,
                               FALSE,
                               CREATE_NEW_CONSOLE,
                               NULL,
                               NULL,
                               &sinfo,
                               &pinfo))
    {
        dwError = GetLastError();
    }
    else
    {
        CloseHandle(pinfo.hThread);
        CloseHandle(pinfo.hProcess);
    }

    if (ERROR_SUCCESS != dwError)
    {
        DisplayFailureMsg(m_hPage, IDS_CANTDEBUG, dwError);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/*++ SetPriority

Routine Description:

    Sets a process' priority class
    
Arguments:

    pid - process Id of process to change
    pri - ID_CMD_XXXXXX menu choice of priority

Return Value:

Revision History:

      Nov-27-95 Davepl  Created

--*/

BOOL CProcPage::SetPriority(DWORD pid, DWORD idCmd)
{
    DWORD dwError = ERROR_SUCCESS;

    DWORD pri;

    // Determine which priority class we need to use based
    // on the menu selection

    switch (idCmd)
    {
        case IDM_PROC_LOW:
            pri = IDLE_PRIORITY_CLASS;
            break;

        case IDM_PROC_BELOWNORMAL:
            pri = BELOW_NORMAL_PRIORITY_CLASS;
            break;

        case IDM_PROC_ABOVENORMAL:
            pri = ABOVE_NORMAL_PRIORITY_CLASS;
            break;

        case IDM_PROC_HIGH:
            pri = HIGH_PRIORITY_CLASS;
            break;

        case IDM_PROC_REALTIME:
            pri = REALTIME_PRIORITY_CLASS;
            break;

        default:
            Assert(idCmd == IDM_PROC_NORMAL);
            pri = NORMAL_PRIORITY_CLASS;
    }

    // Get confirmation before we change the priority

    if (IDYES != QuickConfirm(IDS_WARNING, IDS_PRICHANGE))
    {
        return FALSE;
    }

    HANDLE hProcess = OpenProcess( PROCESS_SET_INFORMATION, FALSE, pid );
    if (hProcess) 
    {
        if (FALSE == SetPriorityClass( hProcess, pri )) 
        {
            dwError = GetLastError();
            dprintf(TEXT("Cant open process for pri change: %08x\n"), dwError);
        }
        else
        {
            TimerEvent();
        }
        CloseHandle( hProcess );
    }
    else
    {
        dwError = GetLastError();
    }

    if (ERROR_SUCCESS != dwError)
    {
        DisplayFailureMsg(m_hPage, IDS_CANTCHANGEPRI, dwError);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


/*++ CProcPage::GetSelectedProcess

Routine Description:

    Returns the CProcInfo * of the currently selected process
    
Arguments:

Return Value:

    CProcInfo * on success, NULL on error or nothing selected

Revision History:

      Nov-22-95 Davepl  Created

--*/

CProcInfo * CProcPage::GetSelectedProcess()
{
    HWND hTaskList = GetDlgItem(m_hPage, IDC_PROCLIST);
    INT iItem = ListView_GetNextItem(hTaskList, -1, LVNI_SELECTED);

    CProcInfo * pProc;

    if (-1 != iItem)
    {
        LV_ITEM lvitem = { LVIF_PARAM };
        lvitem.iItem = iItem;
    
        if (ListView_GetItem(hTaskList, &lvitem))
        {
            pProc = (CProcInfo *) (lvitem.lParam);
        }
        else
        {
            pProc = NULL;
        }
    }
    else
    {
        pProc = NULL;
    }

    return pProc;
}

/*++ CProcPage::HandleWMCOMMAND

Routine Description:

    Handles WM_COMMANDS received at the main page dialog
    
Arguments:

    id - Command id of command received

Return Value:

Revision History:

      Nov-22-95 Davepl  Created

--*/

void CProcPage::HandleWMCOMMAND( WORD id , HWND hCtrl )
{
    CProcInfo * pProc = GetSelectedProcess();

    switch(id)
    {
        case IDC_DEBUG:
        case IDM_PROC_DEBUG:
        {
            if (pProc && m_pszDebugger)
            {
                AttachDebugger( pProc->m_UniqueProcessId);
            }
            break;
        }

        case IDC_TERMINATE:
        case IDM_PROC_TERMINATE:
        {
            if (pProc)
            {
                KillProcess( pProc->m_UniqueProcessId);
            }
            break;
        }

        case IDM_ENDTREE:
        {
            if (pProc)
            {
                RecursiveKill( pProc->m_UniqueProcessId);
            }
            break;
        }

        case IDM_AFFINITY:
        {
            if (pProc)
            {
                SetAffinity( pProc->m_UniqueProcessId);
            }
            break;
        }

        case IDM_PROC_REALTIME:
        case IDM_PROC_HIGH:
        case IDM_PROC_ABOVENORMAL:
        case IDM_PROC_NORMAL:
        case IDM_PROC_BELOWNORMAL:
        case IDM_PROC_LOW:
        {
            if (pProc)
            {
                SetPriority( pProc->m_UniqueProcessId, id);
            }
            break;
        }

        case IDC_SHOWALL:

                g_Options.m_bShowAllProcess = SendMessage( hCtrl , BM_GETCHECK , 0 , 0 ) == BST_CHECKED;

                break;
    }
}



/*++ ProcPageProc

Routine Description:

    Dialogproc for the process page.  
    
Arguments:

    hwnd        - handle to dialog box
    uMsg        - message
    wParam      - first message parameter
    lParam      - second message parameter

Return Value:

    For WM_INITDIALOG, TRUE == success
    For others, TRUE == this proc handles the message

Revision History:

      Nov-16-95 Davepl  Created

--*/

INT_PTR CALLBACK ProcPageProc(
                HWND        hwnd,               // handle to dialog box
                UINT        uMsg,                   // message
                WPARAM      wParam,                 // first message parameter
                LPARAM      lParam                  // second message parameter
                )
{
    CProcPage * thispage = (CProcPage *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

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
            CProcPage * thispage = (CProcPage *) lParam;

            thispage->m_hPage = hwnd;

            // Turn on SHOWSELALWAYS so that the selection is still highlighted even
            // when focus is lost to one of the buttons (for example)

            HWND hTaskList = GetDlgItem(hwnd, IDC_PROCLIST);

            SetWindowLong(hTaskList, GWL_STYLE, GetWindowLong(hTaskList, GWL_STYLE) | LVS_SHOWSELALWAYS);
            ListView_SetExtendedListViewStyle(hTaskList, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

            SubclassListView(GetDlgItem(hwnd, IDC_PROCLIST));

            //
            // This was removed from CProcPage::Activate
            // 
            HWND hchk = GetDlgItem( hwnd , IDC_SHOWALL );

            if( hchk != NULL )
            {
                if( IsTerminalServer( ) )
                {
                    // Disable the IDC_SHOWALL checkbox for non-admin. YufengZ  03/23/98

                    ShowWindow(hchk, TRUE);

                    if( !IsUserAdmin( ) )
                    {
                        EnableWindow( hchk, FALSE );
                    }
                    else
                    {
                        WPARAM wp = g_Options.m_bShowAllProcess ? BST_CHECKED : BST_UNCHECKED;

                        SendMessage( hchk , BM_SETCHECK , wp  , 0 );
                        //Button_SetCheck( hchk , BST_CHECKED );
                    }
                }
                else
                {
                    // hide the IDC_SHOWALL checkbox if its not terminal server.

                    ShowWindow( hchk , SW_HIDE );
                }
            }


            return TRUE;
        }

        case WM_DESTROY:
            thispage->RememberColumnOrder(GetDlgItem(hwnd, IDC_PROCLIST));
            break;

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

        case WM_NCLBUTTONDBLCLK:
        case WM_LBUTTONDBLCLK:
        {
            SendMessage(g_hMainWnd, uMsg, wParam, lParam);
            break;
        }

        // We have been asked to find and select a process 

        case WM_FINDPROC:
        {
            DWORD cProcs = thispage->m_pProcArray->GetSize();
            DWORD dwProcessId;

            for (INT iPass = 0; iPass < 2; iPass++)
            {
                //
                // On the first pass we try to find a WOW
                // task with a thread ID which matches the
                // one given in wParam.  If we don't find
                // such a task, we look for a process which
                // matches the PID in lParam.
                //

                for (UINT i = 0; i < cProcs; i++)
                {
                    dwProcessId = ((CProcInfo *)(thispage->m_pProcArray->GetAt(i)))->m_UniqueProcessId;

                    if ((!iPass && wParam == (WPARAM) dwProcessId) ||
                        ( iPass && lParam == (LPARAM) dwProcessId))
                    {
                        ListView_SetItemState (GetDlgItem(hwnd, IDC_PROCLIST),
                                               i,
                                               LVIS_FOCUSED | LVIS_SELECTED,
                                               0x000F);
                        ListView_EnsureVisible(GetDlgItem(hwnd, IDC_PROCLIST), i, FALSE);
                        goto FoundProc;
                    }
                }
            }

FoundProc:
            break;
        }

        case WM_COMMAND:
        {
            thispage->HandleWMCOMMAND( LOWORD(wParam) , ( HWND )lParam );

            break;
        }

        case WM_CONTEXTMENU:
        {
            CProcInfo * pProc = thispage->GetSelectedProcess();
            if (pProc && pProc->m_UniqueProcessId)
            {
            
                if ((HWND) wParam == GetDlgItem(hwnd, IDC_PROCLIST))
                {
                    thispage->HandleProcListContextMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                    return TRUE;
                }
            }
            break;
        }

        case WM_NOTIFY:
        {
            return thispage->HandleProcPageNotify((HWND) wParam, (LPNMHDR) lParam);
        }

        // Size our kids

        case WM_SIZE:
        {
            thispage->SizeProcPage();
            return TRUE;
        }

        case WM_SETTINGCHANGE:
            thispage->OnSettingsChange();
            // fall through
        case WM_SYSCOLORCHANGE:
            SendMessage(GetDlgItem(hwnd, IDC_PROCLIST), uMsg, wParam, lParam);
            return TRUE;

        default:
            return FALSE;
    }
    return FALSE;
}

/*++ CProcPage::GetTitle

Routine Description:

    Copies the title of this page to the caller-supplied buffer
    
Arguments:

    pszText     - the buffer to copy to
    bufsize     - size of buffer, in characters

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

void CProcPage::GetTitle(LPTSTR pszText, size_t bufsize)
{
    LoadString(g_hInstance, IDS_PROCPAGETITLE, pszText, bufsize);
}

/*++ CProcPage::Activate

Routine Description:

    Brings this page to the front, sets its initial position,
    and shows it

Arguments:

Return Value:

    HRESULT (S_OK on success)

Revision History:

      Nov-16-95 Davepl  Created

--*/

HRESULT CProcPage::Activate()
{
    // Make this page visible

    ShowWindow(m_hPage, SW_SHOW);

    SetWindowPos(m_hPage,
                 HWND_TOP,
                 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE);

    // Newly created dialogs seem to steal the focus, so give it back to the
    // tab control (which must have had it if we got here in the first place)

    SetFocus(m_hwndTabs);

    // Change the menu bar to be the menu for this page

    HMENU hMenuOld = GetMenu(g_hMainWnd);
    HMENU hMenuNew = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MAINMENU_PROC));

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

/*****************************************************************************
 *
 *  TestUserForAdmin
 *
 *   Returns whether the current thread is running under admin
 *   security.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 * REVIEWED by alhen
 * 
 * Code is similar to
 *   HOWTO: Determine if Running In User Context of Local Admin Acct
 *          Last reviewed: March 4, 1998
 *          Article ID: Q118626 
 *
 ****************************************************************************/
BOOL TestUserForAdmin( VOID )
{
    HANDLE Token;

    ULONG InfoLength;

    PTOKEN_GROUPS TokenGroupList;

    ULONG GroupIndex;

    BOOL FoundAdmin;

    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

    PSID AdminSid;

    NTSTATUS Status;

    //
    // Open current process token
    //

    Status = NtOpenProcessToken( NtCurrentProcess(), TOKEN_QUERY, &Token );

    if( !NT_SUCCESS( Status ) )
    {
        return( FALSE );
    }

    //
    // Initialize the admin sid for later
    //

    Status = RtlAllocateAndInitializeSid(

                    &SystemSidAuthority,
                    
                    2,

                    SECURITY_BUILTIN_DOMAIN_RID,

                    DOMAIN_ALIAS_RID_ADMINS,

                    0, 0, 0, 0, 0, 0,

                    &AdminSid

                    );

    if( !NT_SUCCESS( Status ) )
    {
        NtClose( Token );

        return( FALSE );
    }

    //
    // Get a list of groups in the token
    //

    Status = NtQueryInformationToken(

                 Token,                    // Handle

                 TokenGroups,              // TokenInformationClass

                 NULL,                     // TokenInformation

                 0,                        // TokenInformationLength

                 &InfoLength               // ReturnLength

                 );

    if( ( Status != STATUS_SUCCESS )&&( Status != STATUS_BUFFER_TOO_SMALL ) )
    {
        RtlFreeSid( AdminSid );

        NtClose( Token );

        return( FALSE );
    }

    TokenGroupList = ( PTOKEN_GROUPS )LocalAlloc( LPTR , InfoLength );

    if( TokenGroupList == NULL )
    {
        RtlFreeSid( AdminSid );

        NtClose( Token );

        return( FALSE );
    }

    Status = NtQueryInformationToken(

                 Token,                    // Handle

                 TokenGroups,              // TokenInformationClass

                 TokenGroupList,           // TokenInformation

                 InfoLength,               // TokenInformationLength

                 &InfoLength               // ReturnLength

                 );

    if( !NT_SUCCESS( Status ) )
    {
        LocalFree( TokenGroupList );

        RtlFreeSid( AdminSid );

        NtClose( Token );

        return( FALSE );
    }

    //
    // Search group list for admin alias
    //

    FoundAdmin = FALSE;

    for( GroupIndex = 0 ; GroupIndex < TokenGroupList->GroupCount ; GroupIndex++ )
    {
        if( RtlEqualSid( TokenGroupList->Groups[ GroupIndex ].Sid , AdminSid ) )
        {
            FoundAdmin = TRUE;

            break;
        }
    }

    //
    // Tidy up
    //

    LocalFree( TokenGroupList );

    RtlFreeSid( AdminSid );

    NtClose( Token );

    return( FoundAdmin );
}


/*++ CProcPage::Initialize

Routine Description:

    Initializes the process page

Arguments:

    hwndParent  - Parent on which to base sizing on: not used for creation,
                  since the main app window is always used as the parent in
                  order to keep tab order correct
                  
Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

HRESULT CProcPage::Initialize(HWND hwndParent)
{
    //
    // Find out what debbuger is configured on this system
    //

    HKEY hkDebug;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                      TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug"),
                                      0, KEY_READ, &hkDebug))
    {
        TCHAR szDebugger[MAX_PATH * 2];
        DWORD cbString = sizeof(szDebugger);

        if (ERROR_SUCCESS == RegQueryValueEx(hkDebug, TEXT("Debugger"), NULL, 
                                             NULL, (LPBYTE) szDebugger, &cbString))
        {
            // Find the first token (which is the debugger exe name/path)
               
            LPTSTR pszCmdLine = szDebugger;
            
            if ( *pszCmdLine == TEXT('\"') ) 
            {
                //
                // Scan, and skip over, subsequent characters until
                // another double-quote or a null is encountered.
                //
                
                while ( *++pszCmdLine && (*pszCmdLine != TEXT('\"')) )
                {
                    NULL;
                }

                //
                // If we stopped on a double-quote (usual case), skip
                // over it.
                //
                
                if ( *pszCmdLine == TEXT('\"') )
                {
                    pszCmdLine++;
                }
            }
            else 
            {
                while (*pszCmdLine > TEXT(' '))
                {
                    pszCmdLine++;
                }
            }
            *pszCmdLine = TEXT('\0');   // Don't need the rest of the args, etc

            // If the doctor is in, we don't allow the Debug action

            if (lstrlen(szDebugger) && lstrcmpi(szDebugger, TEXT("drwtsn32")) && lstrcmpi(szDebugger, TEXT("drwtsn32.exe")))
            {
                m_pszDebugger = (LPTSTR) LocalAlloc( 0, (lstrlen(szDebugger) + 1) * sizeof(TCHAR));
                if (NULL == m_pszDebugger)
                {
                    return E_OUTOFMEMORY;
                }
                lstrcpy(m_pszDebugger, szDebugger);
            }
        }

        RegCloseKey(hkDebug);
    }

    //
    // Get basic info like page size, etc.
    //

    NTSTATUS Status = NtQuerySystemInformation(
                SystemBasicInformation,
                &g_BasicInfo,
                sizeof(g_BasicInfo),
                NULL
                );

    if (!NT_SUCCESS(Status))
    {
        return E_FAIL;
    }

    //
    // Create the ptr array used to hold the info on running processes
    //

    m_pProcArray = new CPtrArray(GetProcessHeap());
    if (NULL == m_pProcArray)
    {
        return E_OUTOFMEMORY;
    }

    // Our pseudo-parent is the tab contrl, and is what we base our
    // sizing on.  However, in order to keep tab order right among
    // the controls, we actually create ourselves with the main
    // window as the parent

    m_hwndTabs = hwndParent;

    //
    // Create the dialog which represents the body of this page
    //

    m_hPage = CreateDialogParam(
                    g_hInstance,                        // handle to application instance
                    MAKEINTRESOURCE(IDD_PROCPAGE),      // identifies dialog box template name  
                    g_hMainWnd,                     // handle to owner window
                    ProcPageProc,                   // pointer to dialog box procedure
                    (LPARAM) this );                // User data (our this pointer)

    if (NULL == m_hPage)
    {
        return GetLastHRESULT();
    }

    // Set up the columns in the listview

    if (FAILED(SetupColumns()))
    {
        DestroyWindow(m_hPage);
        m_hPage = NULL;
        return E_FAIL;
    }

    // Restore the column positions.

    RestoreColumnOrder(GetDlgItem(m_hPage, IDC_PROCLIST));

    // Do one initial calculation

    TimerEvent();

    return S_OK;
}

/*++ CProcPage::Destroy

Routine Description:

    Frees whatever has been allocated by the Initialize call
    
Arguments:

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

// BUGBUG cleanup ptr array, etc

HRESULT CProcPage::Destroy()
{
    if (m_hPage)
    {
        DestroyWindow(m_hPage);
        m_hPage = NULL;
    }

    if (m_pszDebugger)
    {
        LocalFree(m_pszDebugger);
        m_pszDebugger = NULL;
    }
    return S_OK;
}

/*++ CProcPage::SaveColumnWidths

Routine Description:

    Saves the widths of all of the columns in the global options structure
    
Revision History:

    Jan-26-95 Davepl  Created

--*/

void CProcPage::SaveColumnWidths()
{
    UINT i = 0;
    LV_COLUMN col = { 0 };

    while (g_Options.m_ActiveProcCol[i] != (COLUMNID) -1)
    {
        col.mask = LVCF_WIDTH;
        if (ListView_GetColumn(GetDlgItem(m_hPage, IDC_PROCLIST), i, &col) )
        {
            g_Options.m_ColumnWidths[i] = col.cx;
        }
        else
        {
            ASSERT(0 && "Couldn't get the column width");
        }
        i++;
    }
}

/*++ CProcPage::Deactivate

Routine Description:

    Called when this page is losing its place up front

Arguments:

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

void CProcPage::Deactivate()
{

    SaveColumnWidths();

    if (m_hPage)
    {
        ShowWindow(m_hPage, SW_HIDE);
    }
}

/*++ CProcPage::RecursiveKill

Routine Description:

    Given a pid, recursively kills it and all of its descendants
    
Arguments:

Return Value:

Revision History:

      8-4-98 Davepl  Created

--*/

#define MAX_TASKS 4096

BOOL CProcPage::RecursiveKill(DWORD pid)
{
    TASK_LIST   tlist[MAX_TASKS];
    DWORD       IndexDeferKill[MAX_TASKS];
    DWORD       NumDefer;
    DWORD       numTasks;
    BOOL        bDeferKill;
    BOOL        bDeferThisKill;
    BOOL        rval = TRUE;

    if (IsSystemProcess(pid, NULL))
        return FALSE;

    if (IDYES != QuickConfirm(IDS_WARNING, IDS_KILLTREE))
        return FALSE;

    //
    // get the task list for the system
    //

    numTasks = GetTaskListEx( tlist, MAX_TASKS );
    DetectOrphans( tlist, numTasks );

    //
    // Walk our task tree branch, so we can avoid killing ourself until everyone else is gone
    //

    WalkUpTaskTree(GetCurrentProcessId(), IndexDeferKill, &NumDefer, tlist, numTasks);    

    //
    // Build list of processes to be recursively killed (if applicable)
    //

    BOOL bFound = FALSE;
    for (DWORD i=0; i<numTasks; i++) {
        if (pid == tlist[i].dwProcessId) {
            tlist[i].flags = 1;
            SetupToKillChildren(tlist[i].dwProcessId, tlist, numTasks);
            bFound = TRUE;
            break;
        }
    }
    if (!bFound)
        return FALSE;

    // 
    // Kill the processes
    //

    bDeferKill=FALSE;
    for (i=0; i<numTasks; i++) {        
        if (tlist[i].flags) {

            //
            // Defer killing processes in our branch until later
            //

            bDeferThisKill=FALSE;
            for(DWORD j=0;j<NumDefer;j++) {
                if (i == IndexDeferKill[j]) {
                    bDeferKill=TRUE;
                    bDeferThisKill=TRUE;
                    continue;
                }
            }

            if (bDeferThisKill==TRUE) {
                continue;
            }

            if (KillProcess( tlist[i].dwProcessId, TRUE)) {
                // printf( "process #%d [%s] killed\n", tlist[i].dwProcessId, tlist[i].ProcessName );            
            } else {
                // printf( "process #%d [%s] could not be killed\n", tlist[i].dwProcessId, tlist[i].ProcessName );
                rval = FALSE;
            }
        }
    }

    //
    // Kill our branch now, if necessary.  Do this in reverse order to kill everything we can.
    //

    if (bDeferKill == TRUE) {
        for(INT j=NumDefer-1 ; j >= 0 ; j-- ) 
        {
            if (tlist[IndexDeferKill[j]].flags)
            {
                if (KillProcess( tlist[IndexDeferKill[j]].dwProcessId, TRUE)) 
                {
                    // printf( "process #%d [%s] killed\n", tlist[j].dwProcessId, tlist[j].ProcessName );     
                } 
                else 
                {
                    // printf( "process #%d [%s] could not be killed\n", tlist[j].dwProcessId, tlist[j].ProcessName );
                    rval = FALSE;
                }
            }            
        }
    }

    if (rval != TRUE)
    {
        // We failed to kill at least one of the processes

        TCHAR szTitle[MAX_PATH];
        TCHAR szBody[MAX_PATH];

        if (0 != LoadString(g_hInstance, IDS_KILLTREEFAIL, szTitle, ARRAYSIZE(szTitle)) &&
            0 != LoadString(g_hInstance, IDS_KILLTREEFAILBODY, szBody,  ARRAYSIZE(szBody)))
        {
            MessageBox(m_hPage, szBody, szTitle, MB_ICONERROR);
        }
    }

    return rval;
}

/*++ CProcPage::WalkUpTaskTree

Routine Description:

    Walks up the task tree to make sure we don't kill ourselves in the
    midst of the carnage.
    
Revision History:

      8-4-98 Davepl  Created

--*/

void CProcPage::WalkUpTaskTree(
    DWORD pid,
    DWORD Array[MAX_TASKS],
    PDWORD NArray,
    PTASK_LIST  pTasks,
    DWORD       numTasks)
{
    DWORD currentpid=pid; 
    DWORD i,j;
    BOOL done;

    *NArray=0;
    done=FALSE;
    i=0;

    do
    {
        ++i;

        for (j=0; j<numTasks; j++) {
            if (currentpid == pTasks[j].dwProcessId) {
                Array[*NArray] = j;
                *NArray = *NArray + 1;          

                currentpid = pTasks[j].dwInheritedFromProcessId;
                
                if (currentpid == 0)
                    done=TRUE;
                else
                    j = 0;
            }
        }
    } while ((i < MAX_TASKS) && (done==FALSE));
}

/*++ CProcPage::SetupToKillChildren

Routine Description:

    Marks all descendants of a pid for future termination.

Arguments:

    dwNumTasks       - maximum number of tasks that the pTask array can hold

Return Value:

    Number of tasks placed into the pTask array.

--*/

void CProcPage::SetupToKillChildren(
    DWORD id,
    PTASK_LIST  pTasks,
    DWORD       numTasks
    )
{
    DWORD i;

    for (i=0; i<numTasks; i++) {
        if (!pTasks[i].flags) {
            if (pTasks[i].dwInheritedFromProcessId == id) {
                pTasks[i].flags = TRUE;

                if (pTasks[i].dwProcessId != 0) {
                    SetupToKillChildren( pTasks[i].dwProcessId, pTasks, numTasks );
                }
            }
        }
    }   
}        

/*++  CProcPage::GetTaskListEx

Routine Description:

    Provides an API for getting a list of tasks running at the time of the
    API call.  This function uses internal NT apis and data structures.  This
    api is MUCH faster that the non-internal version that uses the registry.

Arguments:

    dwNumTasks       - maximum number of tasks that the pTask array can hold

Return Value:

    Number of tasks placed into the pTask array.

--*/


DWORD CProcPage::GetTaskListEx(
    PTASK_LIST  pTask,
    DWORD       dwNumTasks
    )
{
    PSYSTEM_PROCESS_INFORMATION  ProcessInfo;
    NTSTATUS                     status;
    ULONG                        TotalOffset;
    ULONG                        totalTasks = 0;

retry:

    BYTE * CommonLargeBuffer = NULL;
    DWORD  CommonLargeBufferSize;

    if (CommonLargeBuffer == NULL) {
        CommonLargeBufferSize = 64*1024;
        CommonLargeBuffer = (BYTE *) VirtualAlloc (NULL,
                                                   CommonLargeBufferSize,
                                                   MEM_COMMIT,
                                                   PAGE_READWRITE);
        if (CommonLargeBuffer == NULL) {
            return 0;
        }
    }
    status = NtQuerySystemInformation(
                SystemProcessInformation,
                CommonLargeBuffer,
                CommonLargeBufferSize,
                NULL
                );

    if (status == STATUS_INFO_LENGTH_MISMATCH) {
        CommonLargeBufferSize += 8192;
        VirtualFree (CommonLargeBuffer, 0, MEM_RELEASE);
        CommonLargeBuffer = NULL;
        goto retry;
    }

    if (!NT_SUCCESS(status))
        return 0;

    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) CommonLargeBuffer;
    TotalOffset = 0;
    while (TRUE) {
        pTask->flags = 0;
        pTask->dwProcessId = PtrToUlong(ProcessInfo->UniqueProcessId);
        pTask->dwInheritedFromProcessId = PtrToUlong(ProcessInfo->InheritedFromUniqueProcessId);
        pTask->CreateTime.QuadPart = (ULONGLONG)ProcessInfo->CreateTime.QuadPart;

        pTask++;
        totalTasks++;
        if (totalTasks == dwNumTasks) {
            break;
        }
        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&CommonLargeBuffer[TotalOffset];
    }

    VirtualFree (CommonLargeBuffer, 0, MEM_RELEASE);

    return totalTasks;
}

/*++  CProcPage::DetectOrphans

Routine Description:

    Eliminates children that predate their parents by marking their parent as
    zero, since these processes must by definition actually be orphans.

Arguments:

    dwNumTasks       - maximum number of tasks that the pTask array can hold

Return Value:

    Number of tasks placed into the pTask array.

--*/

BOOL CProcPage::DetectOrphans(
    PTASK_LIST  pTask,
    DWORD       dwNumTasks
    )
{
    DWORD i, j;
    BOOL Result = FALSE;

    for (i=0; i<dwNumTasks; i++) {
        if (pTask[i].dwInheritedFromProcessId != 0) {
            for (j=0; j<dwNumTasks; j++) {
                if (i != j && pTask[i].dwInheritedFromProcessId == pTask[j].dwProcessId) {
                    if (pTask[i].CreateTime.QuadPart <= pTask[j].CreateTime.QuadPart) {
                        pTask[i].dwInheritedFromProcessId = 0;
                        Result = TRUE;
                        }

                    break;
                    }
                }
            }
        }

    return Result;
}

/*++ IsTerminalServer

Routine Description:

    One time call to ValidateProductSuite to
    determine if we're running terminalServer

Arguments:
    
    < None >

Return Value:

    BOOL

Revision History:

      Jun-30-98 alhen  revised ( no creation info supplied )

--*/
BOOL IsTerminalServer()
{
    static BOOL s_bIsTerminalServer = ValidateProductSuite( );

    // if the exports are not available it's just the same that terminal server
    // is not properly configured and terminal services options should not be available

    return( s_bIsTerminalServer && gpfnWinStationGetProcessSid != NULL && gpfnCachedGetUserFromSid != NULL );
}


/* ++ ValidateProductSuite
Routine Description:
    
      Retrieves OSVERSIONINFO,  but we're only interested
      in the wSuiteMask

Arguments:
    
    < None >

Return Value:

    BOOL

Revision History:

      Jul-1-98 alhen
 -- */

BOOL ValidateProductSuite( )
{
    OSVERSIONINFOEXW osVersionInfo;

    DWORDLONG dwlConditionMask = 0;

    ZeroMemory( &osVersionInfo , sizeof( OSVERSIONINFOEX ) );

    osVersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

    osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL;

    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

    return VerifyVersionInfo( &osVersionInfo , VER_SUITENAME , dwlConditionMask );

}
