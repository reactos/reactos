#ifndef __SCHEDUI_H__
#define __SCHEDUI_H__

/////////////////////////////////////////////////////////////////////////////
// External functions
/////////////////////////////////////////////////////////////////////////////
HRESULT GetRunTimes(TASK_TRIGGER & jt, TASK_DATA * ptd, LPSYSTEMTIME pstBracketBegin, LPSYSTEMTIME pstBracketEnd, WORD * pCount, FILETIME * pRunList);
typedef HRESULT (* GRTFUNCTION)(TASK_TRIGGER & jt, TASK_DATA * ptd, LPSYSTEMTIME pstBracketBegin, LPSYSTEMTIME pstBracketEnd, WORD * pCount, FILETIME * pRunList);

/////////////////////////////////////////////////////////////////////////////
// Structs
/////////////////////////////////////////////////////////////////////////////
struct SSUIDLGINFO
{
    // Set outside ShowScheduleUIDlgProc
    DWORD               dwFlags;

    // Used inside ShowScheduleUIDlgProc
    BOOL                bScheduleChanged;
    BOOL                bScheduleNameChanged;
    BOOL                bDataChanged;
    BOOL                bInitializing;
    TASK_TRIGGER        ttTaskTrigger;
    PNOTIFICATIONCOOKIE pGroupCookie;

    HINSTANCE           hinstURLMON;
    GRTFUNCTION         pfnGetRunTimes;

    DWORD               dwRepeatHrsAreMins;
};

struct CTLGRPITEM
{
    int idContainer;
    int idFirst;
    int idLast;
};

/////////////////////////////////////////////////////////////////////////////
// Functions
/////////////////////////////////////////////////////////////////////////////
HRESULT ScheduleSummaryFromGroup(
    /* [in] */      PNOTIFICATIONCOOKIE pGroupCookie,
    /* [in][out] */ LPTSTR              pszSummary,
    /* [in] */      UINT                cchSummary);

HRESULT ScheduleSummaryFromTaskTrigger(
    /* [in] */      TASK_TRIGGER *  pTaskTrigger,
    /* [in][out] */ LPTSTR          pszSummary,
    /* [in] */      UINT            cchSummary);

#define SSUI_CREATENEWSCHEDULE      0x0001
#define SSUI_EDITSCHEDULE           0x0002
#define SSUI_INFLAGMASK             0x00FF

#define SSUI_SCHEDULECREATED        0x1000
#define SSUI_SCHEDULECHANGED        0x2000
#define SSUI_SCHEDULEREMOVED        0x3000
#define SSUI_SCHEDULELISTUPDATED    (SSUI_SCHEDULECREATED | SSUI_SCHEDULECHANGED | SSUI_SCHEDULEREMOVED)
#define SSUI_OUTFLAG_MASK           0xFF00

HRESULT ShowScheduleUI(
    /* [in] */      HWND                hwndParent,
    /* [in][out] */ PNOTIFICATIONCOOKIE pGroupCookie,
    /* [in][out] */ DWORD *             pdwFlags);

HRESULT CreateScheduleGroup(
    /* [in] */  PTASK_TRIGGER       pTaskTrigger,
    /* [in] */  PTASK_DATA          pTaskData,
    /* [in] */  PGROUPINFO          pGroupInfo,
    /* [in] */  GROUPMODE           grfGroupMode,
    /* [out] */ PNOTIFICATIONCOOKIE pGroupCookie);

HRESULT ModifyScheduleGroup(
    /* [in] */  PNOTIFICATIONCOOKIE pGroupCookie,
    /* [in] */  PTASK_TRIGGER       pTaskTrigger,
    /* [in] */  PTASK_DATA          pTaskData,
    /* [in] */  PGROUPINFO          pGroupInfo,
    /* [in] */  GROUPMODE           grfGroupMode);

HRESULT DeleteScheduleGroup(
    /* [in] */  PNOTIFICATIONCOOKIE pGroupCookie);

BOOL ScheduleGroupExists(
    /* [in] */  LPCTSTR pszGroupName);

/////////////////////////////////////////////////////////////////////////////
// Schedule group combo box helpers
/////////////////////////////////////////////////////////////////////////////
HRESULT SchedGroupComboBox_Fill(HWND hwndCombo);
HRESULT SchedGroupComboBox_Clear(HWND hwndCombo);
HRESULT SchedGroupComboBox_AddGroup(HWND hwndCombo, LPCTSTR pszGroupName, PNOTIFICATIONCOOKIE pGroupCookie);
HRESULT SchedGroupComboBox_RemoveGroup(HWND hwndCombo, PNOTIFICATIONCOOKIE pGroupCookie);
HRESULT SchedGroupComboBox_SetCurGroup(HWND hwndCombo, PNOTIFICATIONCOOKIE pGroupCookie);
HRESULT SchedGroupComboBox_GetCurGroup(HWND hwndCombo, PNOTIFICATIONCOOKIE pGroupCookie);

//wrappers for SchedGrupComboBox functions (fill and setcurgroup) that need to peek at the OOEBuf
HRESULT FillScheduleList (HWND hwndCombo, POOEBuf pBuf);
HRESULT SetScheduleGroup (HWND hwndCombo, CLSID* pGroupCookie, POOEBuf pBuf);

/////////////////////////////////////////////////////////////////////////////
// Helper macros
/////////////////////////////////////////////////////////////////////////////
#define UpDown_GetRange(hwndCtl)                    ((DWORD)SendMessage((hwndCtl), UDM_GETRANGE, 0, 0))
#define UpDown_SetRange(hwndCtl, posMin, posMax)    ((int)(DWORD)SendMessage((hwndCtl), UDM_SETRANGE, 0, MAKELPARAM((posMax), (posMin))))
#define UpDown_GetBuddy(hwndCtl)                    ((HWND)SendMessage((hwndCtl), UDM_GETBUDDY, 0, 0))
#define UpDown_SetBuddy(hwndCtl, hwndBuddy)         ((HWND)SendMessage((hwndCtl), UDM_SETBUDDY, (WPARAM)(hwndBuddy), 0))
#define UpDown_SetAccel(hwndCtl, nAccels, aAccels)  ((BOOL)SendMessage((hwndCtl), UDM_SETACCEL, nAccels, (LPARAM)(aAccels)))

#endif  // __SCHEDUI_H__
